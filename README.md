                                 Apache License
                           Version 2.0, January 2004
                        http://www.apache.org/licenses/
                        
# 自律哨兵（Self-Discipline Sentinel）

> 一款基于 OpenHarmony 5.0 + WS63E 开发板的智能设备，集成姿态感知、本地显示与远程 Web 交互。

## 项目简介

**自律哨兵** 是一个面向个人自律管理场景的 IoT 原型：设备端通过 MPU6050 六轴传感器实时解算姿态角，在 ST7789 LCD 屏幕上显示运行状态，并生成二维码引导用户扫描跳转到远程网页查看更详细的设备界面。

远程网页标题为 **"赛博自律哨兵"**，采用赛博朋克暗色系风格，作为移动端配套前端。

---

## 硬件平台

| 项目 | 说明 |
| --- | --- |
| 主控 | WS63E（HiSilicon Hi3863） |
| 操作系统 | OpenHarmony 5.0（LiteOS 内核） |
| 显示屏 | ST7789，240×320，RGB565，SPI 接口（硬件 / 软件可切换） |
| 姿态传感器 | MPU6050（3 轴加速度计 + 3 轴陀螺仪），I²C 接口 |
| 编译环境 | Ubuntu + hb build（DevEco / OpenHarmony 编译工具链） |

### 引脚连接

**LCD（SPI）：**

| 功能 | 引脚 |
| --- | --- |
| VCC | 3.3V |
| GND | GND |
| SCK | GPIO7 |
| MOSI(SDA) | GPIO8 |
| DC | GPIO10 |
| RST | GPIO6 |
| BLK | GPIO4 |

**MPU6050（I²C）：**

| 功能 | 引脚 |
| --- | --- |
| VCC | 3.3V |
| GND | GND |
| SCL / SDA | 参考板载 I²C 引脚分配 |

> 具体引脚请以 `toy_sle_client/app.c` 和 `toy_sle_client/mpu6050/mpu6050.h` 中定义为准。

---

## 目录结构

```
self_discipline_sentinel/
├── README.md                     # 本文件：根目录说明
├── toy_sle_client/               # WS63E 固件（C 语言，OpenHarmony 组件）
│   ├── README.md                 #     固件侧编译配置说明
│   ├── BUILD.gn                  #     OpenHarmony GN 构建脚本
│   ├── app.c                     #     主应用：LCD 驱动 + 二维码 + IMU 算法
│   ├── app.h                     #     预留业务接口声明
│   ├── app_font.h                #     字体点阵数据（ASCII 8x16 + 汉字 32x32 GB2312）
│   ├── home.h                    #     首页资源引用
│   ├── iot_ex/                   #     IoT HAL 扩展
│   │   ├── hal_iot_gpio_ex.c
│   │   └── iot_gpio_ex.h
│   ├── mpu6050/                  #     六轴传感器驱动与 IMU 姿态解算
│   │   ├── mpu6050.c / mpu6050.h #       寄存器读写与数据采集
│   │   └── imu.c / imu.h         #       互补滤波 + 四元数 -> 欧拉角
│   └── qrcodegen/                #     二维码生成库（公共域）
│       ├── qrcodegen.c
│       └── qrcodegen.h
└── web_file/                     # 配套移动端网页
    ├── README.md
    └── sleep.html                #     赛博自律哨兵 Web 界面
```

---

## 核心功能

### 1. LCD 显示驱动

- 支持 **硬件 SPI** 与 **软件模拟 SPI** 切换（`SPI_Enable` 宏）
- 实现了初始化、窗口设置、填充、矩形、像素、直线、图片显示等基础绘图 API
- RGB565 像素格式，直接调用 `uapi_spi_master_write` 批量传输

### 2. 中文字库显示

- ASCII 英文字符：8×16 点阵（`ascii_8x16`）
- GB2312 汉字：32×32 点阵，通过码表匹配查找
- 支持前景 / 背景色自定义，自动换行

### 3. MPU6050 姿态解算

- 上电后陀螺仪静态零偏校准（50 次采样求平均）
- 使用 **Mahony / 互补滤波 + 四元数积分**（KP=0.8, KI=0.001）
- 输出 `Pitch` / `Roll` / `Yaw` 欧拉角（度），串口实时打印
- 独立 FreeRTOS / CMSIS-OS 任务运行，100Hz 采样率

### 4. 二维码生成与屏幕显示

- 使用公共域 `qrcodegen` 库在片上生成 QR Code
- 将二维码数据 **居中缩放** 到 240×320 RGB565 图像缓冲区
- 自动调用 LCD 接口显示，串口同步输出 ASCII 艺术二维码便于调试

当前二维码指向：
```
https://iot.wit-iot.com/web/sleep.html
```
> 如需修改，见 `toy_sle_client/app.c` 中 `doBasicDemo()` 的 `text` 常量。

### 5. 远程 Web 界面

- `web_file/sleep.html` 是移动端配套前端，标题 **"赛博自律哨兵"**
- 赛博朋克暗色主题，使用毛玻璃、径向渐变等视觉风格
- 设备屏幕上的二维码即指向此页面，用户扫码即可在手机上查看更丰富的设备视图

---

## 编译与烧录

> 固件侧详细步骤请参考 [toy_sle_client/README.md](file:///d:/桌面/2610_source_code/self_discipline_sentinel/toy_sle_client/README.md)。

简要流程：

1. 将 `toy_sle_client` 目录放入 OpenHarmony 源码树对应路径
2. 在应用级 `BUILD.gn` 中启用 `"ws63e/self_discripline_sentinel:zhwl_app"` 组件
3. 在 `ws63` 的 `config.py` 与 `ohos.cmake` 中添加 `zhwl_app` 到组件列表
4. 在 Ubuntu 编译机上执行：
   ```bash
   rm -rf out/* && hb set -p nearlink_dk_3863 && hb build -f
   ```
5. 使用 BurnTool 将生成的 `.fwpkg` 烧录到开发板
6. sscomm 打开串口即可观察姿态角、二维码 ASCII 等调试输出

---

## 运行入口

`toy_sle_client/app.c` 通过 `APP_FEATURE_INIT(LcdEntry)` 注册启动入口，在系统启动时依次创建两个任务：

| 任务 | 栈大小 | 优先级 | 作用 |
| --- | --- | --- | --- |
| `LcdTask` | 10×4096 | Normal | 初始化 LCD → 显示开机画面 → 生成并显示二维码 |
| `MpuTask` | 4096 | Normal | 校准陀螺仪 → 100Hz 循环解算姿态角并打印 |

---

## 依赖说明

| 依赖 | 说明 |
| --- | --- |
| OpenHarmony 5.0 LiteOS | 提供 CMSIS-OS2、uapi SPI / GPIO / I²C |
| ST7789 LCD Controller | 通过 SPI 驱动 |
| MPU6050 | I²C 读取加速度与角速度原始数据 |
| qrcodegen | 公共域二维码生成库（已包含于 `toy_sle_client/qrcodegen`） |

---


