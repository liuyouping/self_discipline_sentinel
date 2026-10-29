## 1、项目位置：

> 对应基于OpenHarmony5.0的WS63E开发环境
```
applications/sample/wifi-iot/app/ws63e
```


## 2、项目列表： 

### 项目名称：自律哨兵（ws63e），self_discripline_sentinel

```
applications/sample/wifi-iot/app/ws63e/self_discripline_sentinel
```

在项目内的BUILD.gn文件中：
```
applications/sample/wifi-iot/app/ws63e/self_discripline_sentinel/BUILD.gn
```
如下修改：
```
static_library("zhwl_app")
```
 
 
## 3、编译配置

``` 
applications/sample/wifi-iot/app/BUILD.gn
```
修改BUILD.gn文件：
```
lite_component("app") {
  features = [ 
    ......
    #"hi3861/ams_demo:zhwl_app",
    #"hi3861/audio_demo:zhwl_app", 
    #"ws63e/lcd_wave_client:zhwl_app",
    #"ws63e/lcd_wave_server:zhwl_app",

    "ws63e/self_discripline_sentinel:zhwl_app",
  ]
}
```

## 4、组件配置：

```
device/soc/hisilicon/ws63v100/sdkv102/build/config/target_config/ws63/config.py
```

在以下位置添加新组件：
```
'ram_component': [
    ……,
    #添加新组件
    "zhwl_app"
    ]
```

```
device/soc/hisilicon/ws63v100/sdkv102/libs_url/ws63/cmake/ohos.cmake
```

在以下位置添加新组件：
```
elseif(${TARGET_COMMAND} MATCHES "ws63-liteos-app")
    set(COMPONENT_LIST
        ……,
        # 添加新组件 
        "zhwl_app"
    )
endif()
```

## 5、编译指令：

在cmd等命令行中登录ubuntu指定文件夹
```
ssh harmony@192.168.118.133
```
```
cd near/near_link_sdk/
```

```
rm -rf out/* && hb set -p nearlink_dk_3863 && hb build -f
```

编译生成的bin文件：
``` 
out/nearlink_dk_3863/nearlink_dk_3863/ws63-liteos-app/ws63-liteos-app_all.fwpkg
```

使用BurnTool（绿色）进行烧录，重启开发板后在sscomm工具中观察输出信息。

---

## 6、固件代码组织

```
toy_sle_client/
├── BUILD.gn               # GN 构建脚本，static_library("zhwl_app")
├── app.c                  # 主应用：LCD 驱动 + 二维码生成 + 姿态解算
├── app.h                  # 预留业务接口
├── app_font.h             # ASCII 8x16 + 汉字 32x32 GB2312 字库
├── home.h                 # 开机画面资源引用
├── iot_ex/                # WS63 / Hi3861 通用 GPIO 扩展接口
│   └── README.md
├── mpu6050/               # MPU6050 I²C 驱动 + 姿态解算
│   └── README.md
└── qrcodegen/             # Project Nayuki 公共域 QR Code 生成库
    └── README.md
```

## 7、运行入口与任务

通过 `APP_FEATURE_INIT(LcdEntry)` 注册系统启动入口，在 OS 拉起后创建两个 CMSIS-OS2 任务：

| 任务 | 栈大小 | 作用 |
| --- | --- | --- |
| `LcdTask` | 10×4096 | LCD 初始化 → 显示图片序列 → `doBasicDemo()` 生成并显示二维码 |
| `MpuTask` | 4096 | `IoTPwmInit` + `MPU6050_GPIO_Init` → 陀螺仪零偏校准 → 100Hz 循环解算欧拉角 |

## 8、已启用硬件

| 外设 | 接口 | 代码位置 |
| --- | --- | --- |
| ST7789 LCD（240×320） | 硬件 SPI1 | `app.c` → `spi_init()` / `lcd_*` |
| MPU6050 六轴 | I²C1 | `mpu6050/mpu6050.c` |
| PWM 输出 | PWM0 | `MpuTask` 入口（`IoTPwmInit(0)`） |

## 9、构建脚本中的源文件

`BUILD.gn` 中实际参与编译的文件（含注释掉的）：

```gn
sources = [
    #"iot_ex/hal_iot_gpio_ex.c",    # GPIO 扩展目前直接通过头引用
    "app.c",
    "qrcodegen/qrcodegen.c",
    "mpu6050/imu.c",
    "mpu6050/mpu6050.c",
    # v23.0：毫米波雷达 UART2 驱动
    "radar_uart.c",
    # v23.0：WiFi STA + AT 配网 + MQTT
    "wifi_mqtt.c",
]
```

> `radar_uart.c` 与 `wifi_mqtt.c` 当前源码尚未出现在仓库，使用前需补充。
