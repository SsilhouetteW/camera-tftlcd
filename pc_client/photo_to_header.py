#!/usr/bin/env python3
"""
Capture a photo from PC camera and convert to picture.h for STM32 TFTLCD.

Usage:
  python photo_to_header.py                    # capture & generate
  python photo_to_header.py --camera 1         # use specific camera
  python photo_to_header.py --width 160 --height 120  # custom resolution
"""

import struct
import sys
import argparse
import os

try:
    import cv2
except ImportError:
    print("Error: OpenCV (cv2) not installed.")
    print("Install with: pip install opencv-python")
    sys.exit(1)


def bgr_to_rgb565_le(frame):
    """Convert OpenCV BGR frame to RGB565 little-endian bytes."""
    b = frame[:, :, 0].astype('uint16')
    g = frame[:, :, 1].astype('uint16')
    r = frame[:, :, 2].astype('uint16')

    r = (r >> 3) & 0x1F
    g = (g >> 2) & 0x3F
    b = (b >> 3) & 0x1F

    rgb565 = (r << 11) | (g << 5) | b
    return struct.pack('<' + 'H' * (frame.shape[1] * frame.shape[0]), *rgb565.flatten())


def write_picture_h(data, width, height, output_path):
    """Write RGB565 byte data as a C array to picture.h."""
    total = len(data)
    lines = []
    lines.append("const unsigned char gImage_picture[%d] = { /* %dX%d RGB565 */" % (total, width, height))

    for i in range(0, total, 16):
        chunk = data[i:i + 16]
        hex_str = ", ".join("0X%02X" % b for b in chunk)
        if i + 16 < total:
            hex_str += ","
        lines.append("\t" + hex_str)

    lines.append("};")

    with open(output_path, "w", encoding="ascii") as f:
        f.write("\n".join(lines) + "\n")

    print(f"[Done] Wrote {total} bytes ({width}x{height} RGB565) to {output_path}")


def main():
    parser = argparse.ArgumentParser(description="Photo to picture.h converter")
    parser.add_argument("--camera", type=int, default=0, help="Camera index (default: 0)")
    parser.add_argument("--width", type=int, default=160, help="Image width (default: 160)")
    parser.add_argument("--height", type=int, default=120, help="Image height (default: 120)")
    parser.add_argument("--output", default=None, help="Output path (default: ../APP/tftlcd/picture.h)")
    args = parser.parse_args()

    w, h = args.width, args.height

    cap = cv2.VideoCapture(args.camera)
    if not cap.isOpened():
        print(f"Error: Cannot open camera #{args.camera}")
        sys.exit(1)

    cap.set(cv2.CAP_PROP_FRAME_WIDTH, 640)
    cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 480)

    print(f"[Camera] Camera #{args.camera} opened")
    print(f"[Image] Target: {w}x{h} RGB565 ({w * h * 2} bytes)")
    print()
    print("=" * 50)
    print("Press SPACE to capture & generate picture.h")
    print("Press 'q' to quit")
    print("=" * 50)

    while True:
        ret, frame = cap.read()
        if not ret:
            print("[Camera] Frame read error")
            break

        preview = frame.copy()
        ph, pw = preview.shape[:2]
        x1 = (pw - w) // 2
        y1 = (ph - h) // 2
        x2 = x1 + w
        y2 = y1 + h
        cv2.rectangle(preview, (x1, y1), (x2, y2), (0, 255, 0), 2)
        cv2.putText(preview, "SPACE: Capture | Q: Quit", (10, 30),
                    cv2.FONT_HERSHEY_SIMPLEX, 0.7, (0, 255, 0), 2)
        cv2.imshow("Photo to picture.h", preview)

        key = cv2.waitKey(30) & 0xFF
        if key == ord('q'):
            print("[Quit]")
            break
        elif key == ord(' '):
            cropped = cv2.resize(frame, (w, h))
            rgb565_data = bgr_to_rgb565_le(cropped)

            if args.output:
                out = args.output
            else:
                script_dir = os.path.dirname(os.path.abspath(__file__))
                out = os.path.join(script_dir, "..", "APP", "tftlcd", "picture.h")
                out = os.path.normpath(out)

            write_picture_h(rgb565_data, w, h, out)
            print("[OK] picture.h generated. Rebuild Keil project and flash.")
            break

    cap.release()
    cv2.destroyAllWindows()


if __name__ == "__main__":
    main()
