# LoRa 868/915 MHz BOOT 模式切换

这是一个用于 T-Glass V3 板载 SX1262 的简单收发示例。设备上电后默认进入接收模式，短按 BOOT 可在发送模式和接收模式之间切换。

- `TX MODE`：只按固定周期发送数据，不会在发射完成后自动进入接收模式。
- `RX MODE`：只持续监听和接收数据，不会在收到数据后发送回复。

两台设备可以烧录同一份固件。让一台保持默认 RX，短按另一台的 BOOT 将其切换到 TX，即可进行单向通信测试。

## BOOT 操作

| 当前模式 | 短按 BOOT 后 | 行为 |
| --- | --- | --- |
| RX MODE | TX MODE | 停止接收，开始周期发送 |
| TX MODE | RX MODE | 停止发送，开始持续接收 |

切换模式时程序会先让 SX1262 进入待机并清除旧中断，屏幕标题和串口会显示当前模式。

## 频率选择

两台设备必须使用相同频率。修改文件顶部的频率宏：

```cpp
// 868 MHz
#define LORA_FREQUENCY_MHZ 868.0

// 915 MHz
#define LORA_FREQUENCY_MHZ 915.0
```

只能保留一个 `LORA_FREQUENCY_MHZ` 定义，并应根据当地无线电法规选择允许使用的频段。

默认参数：

| 参数 | 默认值 |
| --- | --- |
| Frequency | 868.0 MHz |
| Bandwidth | 125.0 kHz |
| Spreading Factor | 9 |
| Coding Rate | 4/7 |
| Sync Word | 0x12 |
| TX Power | 10 dBm |
| Preamble Length | 8 |
| TX Interval | 1500 ms |

## 屏幕显示

- 当前 `LoRa TX` 或 `LoRa RX` 模式及工作频率。
- 带宽、SF、CR、同步字、发射功率和前导码。
- 最近一次发送和接收的数据。
- 最近一次接收数据的 RSSI 和 SNR。
- 当前发送、等待接收或错误状态，以及 BOOT 的目标模式。

## 使用步骤

1. 给两台 T-Glass V3 安装与所选频段匹配的 LoRa 天线，禁止无天线发射。
2. 将相同频率和参数的程序烧录到两台设备。
3. 两台设备上电后均显示 `LoRa RX` 并持续监听。
4. 短按其中一台的 BOOT，使其显示 `LoRa TX`。
5. TX 设备每 1500 ms 发送一次；RX 设备显示接收数据、RSSI 和 SNR。
6. 再次短按 TX 设备的 BOOT，该设备停止发送并切换回 RX。
7. 使用 `115200 baud` 串口可查看模式切换、收发数据及错误代码。

设备使用 ESP32 芯片 ID 的后四位生成节点名，例如 `TG-A1B2`。发送数据格式为：

```text
TG-A1B2 #0
TG-A1B2 #1
```

## PlatformIO

在项目根目录的 `platformio.ini` 中，将 `src_dir` 设置为：

```ini
src_dir = examples/LoRa_868_915_PingPong
```

如需 915 MHz，可在 `build_flags` 中增加：

```ini
build_flags =
    ${env.build_flags}
    -DLORA_FREQUENCY_MHZ=915.0
```

确保其他 `src_dir` 行均已注释，再编译和烧录。
