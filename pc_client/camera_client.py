#!/usr/bin/env python3
"""
WiFi Camera TFTLCD — PC Camera Client
Captures from PC camera, converts to RGB565, sends via TCP to ESP8266/STM32.

Usage:
  python camera_client.py                    # single shot mode
  python camera_client.py --live             # continuous live mode
  python camera_client.py --ip 192.168.4.1   # specify ESP8266 IP
  python camera_client.py --port 8080        # specify port
"""

import socket
import struct
import sys
import argparse
import time

# Image dimensions sent to STM32
IMG_WIDTH = 160
IMG_HEIGHT = 120
IMG_SIZE = IMG_WIDTH * IMG_HEIGHT * 2  # 38400 bytes RGB565

# ESP8266 AP defaults
DEFAULT_IP = "192.168.4.1"
DEFAULT_PORT = 8080


def bgr_to_rgb565(frame):
    """Convert OpenCV BGR frame to RGB565 little-endian bytes.

    RGB565 pixel layout (16-bit):
      RRRRRGGGGGGBBBBB
    Stored little-endian: [low_byte, high_byte]
      low_byte  = G2..G0 + B4..B0  (GGGBBBBB)
      high_byte = R4..R0 + G5..G3  (RRRRRGGG)
    """
    b = frame[:, :, 0].astype('uint16')
    g = frame[:, :, 1].astype('uint16')
    r = frame[:, :, 2].astype('uint16')

    r = (r >> 3) & 0x1F
    g = (g >> 2) & 0x3F
    b = (b >> 3) & 0x1F

    rgb565 = (r << 11) | (g << 5) | b

    # Pack as little-endian bytes: low byte first
    return struct.pack('<' + 'H' * (IMG_WIDTH * IMG_HEIGHT), *rgb565.flatten())


def send_image(sock, rgb565_data):
    """Send raw RGB565 bytes over TCP."""
    total = len(rgb565_data)
    sent = 0
    while sent < total:
        chunk = rgb565_data[sent:sent + 4096]
        n = sock.send(chunk)
        if n == 0:
            raise ConnectionError("TCP send failed")
        sent += n
    return total


def connect_tcp(ip, port, retries=5):
    """Connect to ESP8266 TCP server with retries."""
    for i in range(retries):
        try:
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.settimeout(5)
            sock.connect((ip, port))
            sock.settimeout(None)
            print(f"[TCP] Connected to {ip}:{port}")
            return sock
        except (ConnectionRefusedError, OSError, socket.timeout) as e:
            print(f"[TCP] Attempt {i+1}/{retries}: {e}")
            sock.close()
            if i < retries - 1:
                time.sleep(1)
    raise ConnectionError(f"Cannot connect to {ip}:{port}")


def main():
    parser = argparse.ArgumentParser(description="WiFi Camera TFTLCD Client")
    parser.add_argument("--ip", default=DEFAULT_IP, help=f"ESP8266 IP (default: {DEFAULT_IP})")
    parser.add_argument("--port", type=int, default=DEFAULT_PORT, help=f"TCP port (default: {DEFAULT_PORT})")
    parser.add_argument("--live", action="store_true", help="Continuous live mode (send every 3s)")
    parser.add_argument("--interval", type=float, default=3.0, help="Live mode interval in seconds (default: 3.0)")
    parser.add_argument("--camera", type=int, default=0, help="Camera index (default: 0)")
    parser.add_argument("--width", type=int, default=IMG_WIDTH, help=f"Image width (default: {IMG_WIDTH})")
    parser.add_argument("--height", type=int, default=IMG_HEIGHT, help=f"Image height (default: {IMG_HEIGHT})")
    args = parser.parse_args()

    # Validate custom dimensions
    img_width = args.width
    img_height = args.height

    try:
        import cv2
    except ImportError:
        print("Error: OpenCV (cv2) not installed.")
        print("Install with: pip install opencv-python")
        sys.exit(1)

    print(f"[Camera] Opening camera #{args.camera}...")
    cap = cv2.VideoCapture(args.camera)
    if not cap.isOpened():
        print(f"Error: Cannot open camera #{args.camera}")
        sys.exit(1)

    cap.set(cv2.CAP_PROP_FRAME_WIDTH, 640)
    cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 480)

    print(f"[Image] Resolution: {img_width}x{img_height} RGB565")
    print(f"[Image] Data size: {img_width * img_height * 2} bytes")
    print()
    print("=" * 50)
    if args.live:
        print(f"LIVE mode — sending every {args.interval}s")
        print("Press 'q' to quit")
    else:
        print("Press SPACE to capture & send a photo")
        print("Press 'q' to quit")
    print("=" * 50)

    sock = None
    frame_count = 0

    try:
        while True:
            ret, frame = cap.read()
            if not ret:
                print("[Camera] Frame read error")
                break

            # Show preview with guide lines
            preview = frame.copy()
            h, w = preview.shape[:2]
            # Draw target area
            x1 = (w - img_width) // 2
            y1 = (h - img_height) // 2
            x2 = x1 + img_width
            y2 = y1 + img_height
            cv2.rectangle(preview, (x1, y1), (x2, y2), (0, 255, 0), 2)

            if args.live:
                cv2.putText(preview, f"LIVE | Sent: {frame_count}", (10, 30),
                            cv2.FONT_HERSHEY_SIMPLEX, 0.7, (0, 255, 0), 2)
            else:
                cv2.putText(preview, "SPACE: Send | Q: Quit", (10, 30),
                            cv2.FONT_HERSHEY_SIMPLEX, 0.7, (0, 255, 0), 2)

            cv2.imshow("WiFi Camera TFTLCD", preview)

            key = cv2.waitKey(1 if args.live else 30) & 0xFF

            if key == ord('q'):
                print("[Quit] Exiting...")
                break

            should_send = False
            if args.live:
                # In live mode, simply use the key check as a signal
                # Actual sending is triggered by time interval below
                pass
            elif key == ord(' '):  # SPACE
                should_send = True

            # Live mode time-based trigger
            if args.live:
                should_send = True

            if should_send:
                # Crop center region
                if img_width != IMG_WIDTH or img_height != IMG_HEIGHT:
                    h, w = frame.shape[:2]
                    x1 = (w - img_width) // 2
                    y1 = (h - img_height) // 2
                    cropped = frame[y1:y1 + img_height, x1:x1 + img_width]
                else:
                    cropped = cv2.resize(frame, (img_width, img_height))

                # Resize if dimensions changed
                if cropped.shape[1] != img_width or cropped.shape[0] != img_height:
                    cropped = cv2.resize(cropped, (img_width, img_height))

                # Convert to RGB565
                rgb565_data = bgr_to_rgb565(cropped)

                # Connect TCP (if not already connected)
                if sock is None:
                    try:
                        sock = connect_tcp(args.ip, args.port)
                    except ConnectionError:
                        print("[TCP] Connection failed. Retry on next capture.")
                        if args.live:
                            time.sleep(args.interval)
                        continue

                # Send
                try:
                    sent = send_image(sock, rgb565_data)
                    frame_count += 1
                    print(f"[Send] #{frame_count}: {sent} bytes sent -> {img_width}x{img_height}")
                except (ConnectionError, BrokenPipeError, OSError) as e:
                    print(f"[TCP] Error: {e}, reconnecting...")
                    sock.close()
                    sock = None
                    if args.live:
                        time.sleep(1)
                    continue

                if args.live:
                    time.sleep(args.interval)

    except KeyboardInterrupt:
        print("\n[Quit] Interrupted")
    finally:
        if sock:
            sock.close()
        cap.release()
        cv2.destroyAllWindows()
        print("[Done]")


if __name__ == "__main__":
    main()
