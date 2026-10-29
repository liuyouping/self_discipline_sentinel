# qrcodegen — QR Code 生成库

> Project Nayuki 开源的 **纯 C 二维码生成库**，MIT License。本项目直接将其源码拷贝进仓库，无需额外依赖，在 WS63 片上实时生成 QR Code 并在 LCD 上显示。

## 文件列表

| 文件 | 说明 |
| --- | --- |
| [qrcodegen.h] | 枚举（纠错级别 / Mask / 版本号）、缓冲区宏、对外函数声明 |
| qrcodegen.c | 全部编解码实现（公共域） |

## 库特性

- 支持 QR Code **Model 2** 规范，版本 1–40（21×21 ~ 177×177）
- 四种纠错等级：`LOW` / `MEDIUM` / `QUARTILE` / `HIGH`
- 四种编码模式：数字 / 字母 / 字节 / 汉字（字节模式覆盖所有 UTF-8 内容）
- **无动态内存分配**：调用方提供 `qrcodegen_BUFFER_LEN_MAX`（版本 40 的最大缓冲区）作为 scratch
- MIT License，可自由商用、修改、分发

## 本项目中的使用位置

调用方位于 [app.c → doBasicDemo()]：

```c
const char *text = "https://iot.wit-iot.com/web/sleep.html";
uint8_t qrcode[qrcodegen_BUFFER_LEN_MAX];
uint8_t tempBuffer[qrcodegen_BUFFER_LEN_MAX];

bool ok = qrcodegen_encodeText(text, tempBuffer, qrcode,
    qrcodegen_Ecc_LOW,
    qrcodegen_VERSION_MIN, qrcodegen_VERSION_MAX,
    qrcodegen_Mask_AUTO, true);
```

生成后，`qrcode` 是一个 **方形模块数组**（不是像素图），通过 `qrcodegen_getSize()` 拿到边长，再由 `qrcode_to_rgb565()` 居中缩放成 240×320 RGB565 图像喂给 LCD。

## 常用 API 速查

| 函数 | 说明 |
| --- | --- |
| `qrcodegen_encodeText(text, temp, out, errCorLvl, minVer, maxVer, mask, boostEcc)` | 文本 → QR Code |
| `qrcodegen_encodeBinary(data, len, temp, out, ...)` | 二进制 → QR Code |
| `qrcodegen_getSize(qr)` | 返回 QR 正方形边长（模块数） |
| `qrcodegen_getModule(qr, x, y)` | 读取 `(x, y)` 模块颜色（`true`=黑，`false`=白） |

## 缓冲区大小

```c
#define qrcodegen_BUFFER_LEN_MAX 3912   // 版本 40 的最大字节数
```

调用 `qrcodegen_encodeText` 需要 **两个** 该尺寸的缓冲区（`temp` 用于编码中间态，`out` 用于最终图像）。片上 RAM 紧张时可按需估算更小的上限。

## 修改二维码内容

在 [app.c] 中修改 `text` 即可。若想根据设备 ID 动态生成 URL（如 `https://iot.wit-iot.com/web/sleep.html?devid=xxx`），建议：
- 在 `LcdTask` 启动时一次生成并缓存 RGB565，避免反复计算
- 若需运行时刷新，可把 `qrcode_to_rgb565` 封装成独立接口

## 参考链接

- 官方主页：<https://www.nayuki.io/page/qr-code-generator-library>
- 源码仓库：<https://github.com/nayuki/QR-Code-generator>

## 许可

MIT License，Project Nayuki。
