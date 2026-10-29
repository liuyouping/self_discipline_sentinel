# mpu6050 — 六轴传感器驱动 + 姿态解算

> MPU6050 集成 3 轴加速度计（±4g，8192 LSB/g）与 3 轴陀螺仪（±2000°/s，65.5 LSB/(°/s)）。本目录提供 I²C 寄存器读写与四元数互补滤波两部分，由上层 `MpuTask` 任务 100Hz 循环调度。

## 文件列表

| 文件 | 说明 |
| --- | --- |
| [mpu6050.h] | 寄存器地址、数据结构体 `MPU6050_Data_TypeDef`、对外接口 |
| [mpu6050.c] | I²C 底层 + WHO_AM_I 校验 + 初始化 + 原始数据采集 |
| [imu.h] | IMU 算法头（当前几乎为空） |
| [imu.c] | 目前为空实现，算法主体在 `app.c` 的 `MpuTask` 中 |

## 硬件接线

| MPU6050 | WS63 |
| --- | --- |
| VCC | 3.3V |
| GND | GND |
| SDA | GPIO15（I²C1_SDA） |
| SCL | GPIO16（I²C1_SCL） |
| A0 | GND（故设备地址为 `0x68`） |

I²C 使用 `IoTI2cWrite` / `IoTI2cRead`，挂载于 `I2C_IDX=1`，波特率 400KHz。

## 数据模型

```c
typedef struct {
    short Temperature;   // 摄氏温度，已换算
    short Accel[3];      // 原始加速度（LSB）
    short Gyro[3];       // 原始角速度（LSB）
} MPU6050_Data_TypeDef;
```

## 初始化链路

```
MPU6050_GPIO_Init()
    ├── GPIO_MPU6050_Init()       // 配置 I²C1 管脚
    └── MPU6050_Init()
            ├── 写 PWR_MGMT_1=0x80  // 复位
            ├── 写 PWR_MGMT_1=0x00  // 唤醒
            └── 写 ACCEL_CONFIG=0x08 // ±4g 量程
```

`MPU6050ReadID()` 读取 `WHO_AM_I (0x75)`，返回 `0x68` 表示芯片在位，否则上层会打印 `MPU6050 not detected!`。

## 姿态解算

算法目前集成在 [app.c → MpuTask] 中而非独立到 `imu.c`，关键参数：

| 参数 | 值 | 含义 |
| --- | --- | --- |
| KP | 0.8 | 互补滤波比例增益 |
| KI | 0.001 | 积分增益（抑制陀螺仪零偏） |
| 采样率 | 100Hz | `osDelay(1)` + `halfT = dt/2` |
| 陀螺仪零偏 | 静态校准 | 上电采集 50 次平均 |

输出的欧拉角单位为 **度**：

```c
Pitch = asinf(2*(q0*q2 - q1*q3)) * 57.2957795f;
Roll  = atan2f(2*(q2*q3 + q0*q1), ...) * 57.2957795f;
Yaw   = atan2f(2*(q1*q2 + q0*q3), ...) * 57.2957795f;
```

## 后续可优化

- [ ] 把 `calibrate_gyro` / `IMUupdate` 从 `app.c` 搬到 `imu.c`，让 `BUILD.gn` 中 `imu.c` 真的参与编译
- [ ] 增加 MPU6050 自带的 DMP（数字运动处理器）路径，减轻 CPU 占用
- [ ] 增加 Free-Fall / Motion / Zero-Motion 中断触发的事件回调

## 许可

Apache License 2.0，移植自南京小熊派智能科技例程。
