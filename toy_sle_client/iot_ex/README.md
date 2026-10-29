# iot_ex — WS63 芯片 GPIO / 延时 兼容层

> HiSilicon 官方提供的 IoT HAL 扩展接口，本项目复用其 **管脚复用枚举** 与 **拉电流配置**，让应用层代码在 WS63（新平台）与 Hi3861（老平台）之间可通过宏切换。

## 文件列表

| 文件 | 说明 |
| --- | --- |
| [iot_gpio_ex.h] | 对外头文件，定义 WS63 专属的 `IotIoName` / `IotIoFuncGpioXxx` 枚举 |
| [hal_iot_gpio_ex.c] | `IoSetPull` / `IoSetFunc` / `TaskMsleep` 的封装实现 |

## 核心 API

### 1. 管脚电平/复用

```c
unsigned int IoSetPull(unsigned int id, IotIoPull val);   // 上/下拉
unsigned int IoSetFunc(unsigned int id, unsigned char val); // 复用功能
unsigned int TaskMsleep(unsigned int ms);                   // 毫秒级延时
```

WS63 平台下实际走到 `uapi_pin_set_pull` / `uapi_pin_set_mode` / `LOS_Msleep`；Hi3861 则分别映射到 `hi_io_set_pull` / `hi_io_set_func` / `hi_sleep`。通过 `#define CHIP_WS63 1` 切换，默认指向 WS63。

### 2. 项目中的实际使用

[app.c] 里用 `IOT_IO_FUNC_GPIO_X_SPI0_SCK` 等枚举为 LCD 引脚配置 SPI 功能：

```c
uapi_pin_set_mode(LCD_SCK_GPIO, IOT_IO_FUNC_GPIO_7_SPI0_SCK);
uapi_pin_set_mode(LCD_DC_GPIO,  IOT_IO_FUNC_GPIO_10_GPIO);
```

[mpu6050/mpu6050.c] 里也通过 `iot_gpio_ex.h` 引入平台无关的 I²C 接口宏。

## WS63 可用管脚一览

`IotIoName` 枚举定义了 `GPIO_0` ~ `GPIO_16`（WS63 扩展到 16），其中本项目实际占用：

| GPIO | 复用 | 用途 |
| --- | --- | --- |
| 7 | `SPI0_SCK` | LCD SPI 时钟 |
| 8 | `SPI0_OUT` | LCD SPI 数据（MOSI） |
| 10 | `GPIO` | LCD DC 命令/数据 |
| 11 | `SPI0_IN` | LCD SPI MISO（未使用） |
| 15 / 16 | `I2C1_SDA / I2C1_SCL` | MPU6050 I²C |

## 平台切换

若要移植到 Hi3861 / OpenHarmony 旧版，把 `iot_gpio_ex.h` 中的 `#define CHIP_WS63 1` 改为 `0`，枚举表会自动切换到 `IOT_IO_NAME_SFC_*` / `IOT_IO_FUNC_SFC_*` 等 Hi3861 专用管脚定义。

## 许可

Apache License 2.0，HiSilicon (Shanghai) Technologies Co., Ltd.
