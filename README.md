# WiFi Camera TFTLCD — 代码要点与操作指南

## 1. 项目架构

```
PC (Python/OpenCV)                    STM32F103ZE + ESP8266
┌─────────────────┐     WiFi TCP      ┌─────────────────────────┐
│ camera_client.py │ ──────────────→  │ ESP8266 (AP)            │
│                  │  192.168.4.1:8080│   ↓ UART3 (PB10/PB11)  │
│ BGR → RGB565 LE  │                  │ STM32                   │
│ 160×120 × 2bytes │                  │   ↓ FSMC Bank4          │
│ = 38,400 bytes   │                  │ TFT LCD (HX8357DN)      │
└─────────────────┘                  │   320×480, RGB565       │
                                     └─────────────────────────┘
```

- **ESP8266**：AP 模式，创建热点 `PZ103-Camera`（密码 `12345678`），TCP Server 端口 `8080`
- **图像格式**：160×120 像素 RGB565 little-endian，每帧 38,400 字节
- **传输协议**：ESP8266 以 `+IPD,0,N:<data>` 格式将 TCP 数据通过 UART3 转发给 STM32

---

## 2. 文件结构

```
wifi-camera-tftlcd/
├── User/main.c                  ← 主程序：WiFi + 图像接收 + LCD 显示
├── APP/
│   ├── tftlcd/
│   │   ├── tftlcd.h             ← LCD 驱动头文件（HX8357DN）
│   │   ├── tftlcd.c             ← LCD 驱动（FSMC 初始化 + 显示函数）
│   │   ├── font.h               ← ASCII 字库（12/16/24/32 像素）
│   │   └── picture.h            ← 编译时嵌入的图片（gImage_picture 数组）
│   ├── esp8266/
│   │   ├── wifi_config.h/c      ← ESP8266 配置与 UART3 接收框架
│   │   └── wifi_function.h/c    ← AT 指令封装函数
│   └── led/                     ← LED 驱动
├── pc_client/
│   ├── camera_client.py         ← PC 摄像头 → TCP 实时发送
│   └── photo_to_header.py       ← PC 拍照 → 生成 picture.h
└── Libraries/                   ← CMSIS + StdPeriph 库
```

---

## 3. 关键代码要点

### 3.1 LCD 驱动：FSMC 初始化（StdPeriph 移植）

```c
// tftlcd.c — TFTLCD_FSMC_Init()
RCC_AHBPeriphClockCmd(RCC_AHBPeriph_FSMC, ENABLE);
// GPIO: PD0,1,4,5,8,9,10,14,15 → AF_PP 50MHz
//       PE7-15 → AF_PP 50MHz
//       PG0,12 → AF_PP 50MHz (PG12 = NE4)
//       PB0    → Out_PP  (LCD 背光)

FSMC_NORSRAMInitStructure.FSMC_Bank = FSMC_Bank1_NORSRAM4;  // NE4
FSMC_NORSRAMInitStructure.FSMC_MemoryType = FSMC_MemoryType_SRAM;
FSMC_NORSRAMInitStructure.FSMC_MemoryDataWidth = FSMC_MemoryDataWidth_16b;
// 读时序: AddressSetup=15, DataSetup=60
// 写时序: AddressSetup=9,  DataSetup=8
FSMC_NORSRAMInit(&FSMC_NORSRAMInitStructure);
```

**LCD 命令/数据地址映射：**
```c
#define TFTLCD_BASE  ((u32)(0x6C000000 | 0x000007FE))
// A10 作为 RS（命令/数据选择线），FSMC 16位模式地址左移1位
// 所以 RS=0 (CMD): 0x6C000000 | 0x000007FE
//    RS=1 (DATA): 0x6C000000 | 0x00000800
```

### 3.2 LCD_ShowChar：列优先渲染（关键修复）

**字库数据格式**：PC2LCD2002 "列行式"取模——每个字节代表一列 8 个竖像素。

```
字符 '!' (12×6 点阵)，12 字节：
Byte 0-1  → 列 0 (上8行 + 下4行)
Byte 2-3  → 列 1
...
Byte 10-11 → 列 5
```

**正确的列优先渲染算法：**
```c
void LCD_ShowChar(u16 x, u16 y, u8 num, u8 size, u8 mode)
{
    u8 temp, t, t1;
    u16 y0 = y;
    u8 csize = (size/8 + ((size%8)?1:0)) * (size/2);  // 每字符字节数

    num = num - ' ';
    for (t = 0; t < csize; t++) {           // 遍历每个字节（列）
        temp = asc2_1206[num][t];            // 读取当前列的8个竖像素
        for (t1 = 0; t1 < 8; t1++) {
            if (temp & 0x80)                 // 最高位→最上方像素
                LCD_DrawFRONT_COLOR(x, y, FRONT_COLOR);
            else if (mode == 0)
                LCD_DrawFRONT_COLOR(x, y, BACK_COLOR);
            temp <<= 1;
            y++;                             // 向下移动
            if ((y - y0) == size) {          // 列画完，换下一列
                y = y0; x++; break;
            }
        }
    }
}
```

### 3.3 LCD_ShowPicture：图片显示

```c
void LCD_ShowPicture(u16 x, u16 y, u16 wide, u16 high, u8 *pic)
{
    // pic 格式：[低字节, 高字节, 低字节, 高字节, ...]  小端 RGB565
    // LCD_DrawFRONT_COLOR 逐个像素绘制
    for (i = 0; i < high; i++)
        for (j = 0; j < wide; j++) {
            temp = pic[tmp+1]; temp <<= 8; temp |= pic[tmp];
            LCD_DrawFRONT_COLOR(x+j, y+i, temp);
            tmp += 2;
        }
}
```

### 3.4 HX8357DN 初始化序列

```c
LCD_WriteCmd(0x11);                   // Sleep Out
LCD_WriteCmdData(0xB9, 0xFF, 0x83, 0x57);  // EXTC 使能
LCD_WriteCmdData(0xB1, ...);          // 电源设置 (7 字节)
LCD_WriteCmdData(0xB4, ...);          // 显示控制 (7 字节)
LCD_WriteCmdData(0x36, 0x4C);         // 内存访问控制 (MY=1, MX=0, BGR=1)
LCD_WriteCmdData(0xC0, ...);          // 面板驱动 (9 字节)
LCD_WriteCmdData(0xE0, ...);          // Gamma 校正 (34 字节)
LCD_WriteCmdData(0x3A, 0x05);         // 16-bit RGB565
LCD_WriteCmd(0x29);                   // Display ON
```

### 3.5 ESP8266 初始化（AP + TCP Server）

```c
// wifi_function.c — ESP8266_Init_AP_TCP_Server()
1. AT+CWMODE=2           → AP 模式
2. AT+CWSAP="PZ103-Camera","12345678",1,4 → 创建热点 (通道1, WPA2)
3. AT+RST                → 重启模块
4. AT                    → 测试通信（最多重试3次）
5. AT+CIPMUX=1           → 启用多连接
6. AT+CIPSERVER=1,8080   → 启动 TCP 服务器
7. AT+CIPSTO=180         → 超时 180 秒
```

### 3.6 图像接收与帧解析（main.c）

ESP8266 通过 UART3 向 STM32 发送 TCP 数据，格式为：

```
+IPD,<link_id>,<len>:<binary_data>
```

示例：`+IPD,0,38400:<38400 bytes of RGB565>`

STM32 解析流程：
1. UART3 接收中断将数据存入 `strEsp8266_Fram_Record.Data_RX_BUF`
2. 接收完成后 `FramFinishFlag` 置 1
3. main 循环检测到 `FramFinishFlag`，解析 `+IPD,0,N:` 头
4. 提取 `N`（数据长度），定位 `:` 后数据起始位置
5. 通过 `memcpy` 累积到 `image_buf[IMG_SIZE]`（38,400 字节）
6. 累积满一帧后 `image_complete = 1`
7. main 循环调用 `LCD_ShowPicture(x, y, 160, 120, image_buf)` 显示

### 3.7 PC 端：BGR → RGB565 转换

```python
# camera_client.py — bgr_to_rgb565()
r = (bgr[:,:,2] >> 3) & 0x1F    # 5 bits red
g = (bgr[:,:,1] >> 2) & 0x3F    # 6 bits green
b = (bgr[:,:,0] >> 3) & 0x1F    # 5 bits blue
rgb565 = (r << 11) | (g << 5) | b
# struct.pack('<H' * N, ...) → little-endian 字节流
```

---

## 4. 操作步骤

### 4.1 硬件连接

| STM32 引脚 | 连接 |
|-----------|------|
| PB10      | ESP8266 UART3 TX |
| PB11      | ESP8266 UART3 RX |
| PA4       | ESP8266 CH_PD (使能) |
| PA15      | ESP8266 RST |
| PD0,1,4,5,8,9,10,14,15 | TFT LCD 数据线 |
| PE7-15    | TFT LCD 数据线 |
| PG12      | TFT LCD NE4 (片选) |
| PB0       | TFT LCD 背光 |

### 4.2 编译与烧录

1. 打开 Keil MDK，加载 `Template.uvprojx`
2. 确认工程包含以下文件：
   - `User/main.c`
   - `APP/tftlcd/tftlcd.c`
   - `APP/esp8266/wifi_config.c`, `wifi_function.c`
3. 预处理器宏：`STM32F10X_HD, USE_STDPERIPH_DRIVER`
4. 编译 → 烧录到普中103开发板

### 4.3 运行

**方式 A：实时视频流（低帧率）**
```bash
# 1. PC 连接 WiFi 热点: PZ103-Camera (密码: 12345678)
# 2. 运行客户端
cd pc_client
pip install opencv-python
python camera_client.py --live --interval 0.5
#    --live      连续发送模式
#    --interval   发送间隔（秒），建议 0.3~1.0
```

**方式 B：单张照片（picture.h 嵌入）**
```bash
# 1. 拍照生成 picture.h
python photo_to_header.py
#    按 SPACE 拍照 → 自动写入 ../APP/tftlcd/picture.h

# 2. Keil 中重新编译烧录
# 3. 照片在屏幕中央显示
```

### 4.4 命令行参数

| 参数 | 默认值 | 说明 |
|------|--------|------|
| `--ip` | 192.168.4.1 | ESP8266 AP 的 IP |
| `--port` | 8080 | TCP 端口 |
| `--live` | (off) | 连续实时模式 |
| `--interval` | 3.0 | live 模式发送间隔（秒） |
| `--camera` | 0 | 摄像头索引 |
| `--width` | 160 | 图像宽度 |
| `--height` | 120 | 图像高度 |

---

## 5. 故障排查

| 现象 | 可能原因 | 解决方法 |
|------|----------|----------|
| LCD 白屏/无显示 | 背光未开启 | 检查 PB0 输出高电平 |
| LCD 显示乱码 | 字库算法不匹配 | 确认 LCD_ShowChar 为列优先渲染 |
| WiFi 热点搜不到 | ESP8266 未初始化 | 检查 PA4 拉高, PA15 复位时序 |
| TCP 连接被拒绝 | AT+CIPSERVER 失败 | 串口助手监控 ESP8266 TX，确认指令响应 |
| 连接成功但无图像 | 端口/IP 不匹配 | 检查 LCD 显示的 IP 是否正确 |
| 图像花屏/颜色错误 | RGB565 字节序不对 | 检查 `+IPD` 解析是否正确提取数据 |
| 实时帧率很低 | 丢帧/ESP8266 缓冲区溢出 | 增大 `--interval`，确保每帧完整传输 |
