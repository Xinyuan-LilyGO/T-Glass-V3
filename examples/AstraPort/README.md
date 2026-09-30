# Factory_Astra

## English

### 1. Project highlights

- **Astra-style UI** with tiles, lists, selectors, XBM icons, and hierarchical navigation.
- **Status bar**: the top 10 pixels of normal pages show the WiFi icon, local time, and battery percentage.
- **Camera module**: the camera always uses JPEG and reports QVGA 320x240. Camera streaming and gesture recognition live in separate module directories.
- **3D gesture recognition**: recognizes gestures 1 through 5 and renders a white 3D digit with gentle pitch, yaw, and roll motion.
- **Gesture control**: gestures 1 through 5 can operate the menu and Internet radio.
- **JPEG camera streaming**: provides a browser control page and an MJPEG stream from Camera -> Camera Stream.
- **WiFi management**: two WiFi credential pairs are supported. WiFi can be enabled or disabled from Settings, and reconnect attempts rotate between both networks.
- **LoRa test tools**: continuous transmit, receive monitor, parameter display, and wireless status. Continuous transmit sends Hello 1, Hello 2, Hello 3, and so on about once per second.
- **Internet radio**: plays multiple network stations and displays the station, stream title, playback state, and volume.
- **Device settings**: Chinese/English, display calibration, brightness, sleep, and factory diagnostics.
- **Resource lifecycle management**: microphone, LoRa, radio playback, the camera server, and gesture recognition are activated only when their pages need them.

### 2. Display layout

- The physical T-Glass V3 display is 126x126.
- Normal Factory_Astra pages use a 126x74 area: a 10-pixel status bar at the top and a 126x64 Astra content area below it.
- 3D Gesture and Dino Jump use the full display without the normal status bar.
- Frames are not stretched, and Chinese and English use the same page structure.

### 3. Menu tree

~~~text
Astra Home
├── Camera
│   ├── Camera Status
│   ├── 3D Gesture
│   ├── Gesture Control
│   └── Camera Stream
├── Audio
│   ├── Microphone
│   ├── Speaker Test
│   └── Input Test
├── Internet Radio
│   └── Station list
├── LoRa
│   ├── Continuous Transmit
│   ├── Receive Monitor
│   ├── LoRa Parameters
│   └── Wireless Status
├── Network
│   ├── WiFi Status
│   ├── WiFi Scan
│   └── Time & NTP
├── Games
│   └── Dino Jump
└── Device
    ├── Battery
    ├── Display Calibration
    ├── Brightness
    ├── Language
    │   ├── Chinese
    │   └── English
    ├── Settings
    │   ├── WiFi
    │   └── Gesture Recognition
    ├── Sleep
    └── Factory Diagnostics
~~~

### 4. Controls

| Input | Short press | Long press, about 1 second |
| --- | --- | --- |
| GPIO1 / TOUCH | Move to the next item | Open, confirm, or execute |
| GPIO0 / BOOT | Move to the previous item | Hold about 1 second and release to return; hold continuously for 5 seconds to power off |

Page-specific behavior:

- **Dino Jump**: TOUCH starts or restarts the game and jumps while running. BOOT jumps. A long BOOT press exits the game. The current physics gives the dino a higher jump, longer airtime, and longer horizontal coverage.
- **Internet Radio**: short presses move through the station list, a long TOUCH press selects a station, and a long BOOT press exits and stops playback.
- **BOOT timing**: hold BOOT for about 1 second and release to return to the previous page. Keeping it held continuously for 5 seconds requests an immediate power off; USB/VBUS connected blocks power off.
- **Display Calibration**: select Move Up, Move Down, Move Left, or Move Right and long-press TOUCH to adjust by 5 pixels; select Save to persist the change.
- **Brightness**: choose 25%, 50%, 75%, or 100%.
- **Sleep**: enters ESP32 deep sleep; GPIO1 is configured as the wake source.

### 5. Language and settings

The language path is **Device -> Language**. Select Chinese or English; the check mark updates immediately and menu titles, lists, and status text change at once. The choice is stored in the astra_ui Preferences namespace and survives reboot.

WiFi and gesture switches are under **Device -> Settings**:

- WiFi is enabled by default. The device tries WIFI_SSID first and switches to WIFI_SSID2 after a failed attempt.
- Gesture recognition is disabled by default. If the 3D Gesture or Gesture Control page is opened while it is disabled, the display asks the user to enable it in Settings first.
- Both settings are persisted across reboot.

### 6. 3D Gesture and Gesture Control

1. Open **Device -> Settings -> Gesture Recognition** and enable it.
2. Open **Camera -> 3D Gesture**.
3. The page initially shows “Show gesture 1-5”. After recognition, it shows only the corresponding 3D digit without overlapping recognition text.
4. The supported digits are 1, 2, 3, 4, and 5.

In **Camera -> Gesture Control**, the mapping is:

| Gesture | Action |
| --- | --- |
| 1 | Previous item |
| 2 | Next item |
| 3 | Open/confirm |
| 4 | Back |
| 5 | Play/pause Internet radio |

After leaving Gesture Control, the control mode may remain active until gesture recognition is disabled or another camera-owned mode, such as 3D Gesture or Camera Stream, is entered. 3D Gesture and Camera Stream do not use the camera and model memory at the same time.

### 7. JPEG camera streaming

Steps:

1. Confirm WiFi is enabled under **Device -> Settings -> WiFi**.
2. Wait until **Network -> WiFi Status** reports a connection.
3. Open **Camera -> Camera Stream**.
4. The display shows the web and stream addresses; the serial monitor prints the complete URLs.
5. Open the address from a browser on the same LAN.

Service URLs:

~~~text
Web control page: http://<device-ip>/
Single JPEG frame: http://<device-ip>/capture
Status endpoint:   http://<device-ip>/status
MJPEG stream:      http://<device-ip>:81/stream
~~~

The camera uses JPEG QVGA 320x240. The web, capture, status, and control server uses port 80; the MJPEG stream uses port 81. The servers run only while the Camera Stream page is open and stop when the page is left. Entering Gesture Recognition also stops the camera server to release contiguous PSRAM.

If WiFi is not connected, the stream page shows the connection state and configured SSID and does not start the server. WiFi retries every 30 seconds by default. Reconnect attempts are paused while gesture recognition is active and resume after leaving the gesture page.

### 8. LoRa

Open **LoRa -> Continuous Transmit** to send:

~~~text
Hello 1
Hello 2
Hello 3
...
~~~

The counter increments after each completed transmission, with a default interval of about 1000 ms. The serial monitor prints the TX payload and completion count. On another device, **LoRa -> Receive Monitor** shows the received text, RSSI, and SNR.

Current defaults:

~~~text
Frequency: 868.0 MHz
Bandwidth: 125.0 kHz
Spreading factor: SF10
Coding rate: 4/6
Sync word: 0x12
TX power: 22 dBm
Preamble: 15
CRC: disabled
~~~

Both devices must use the same frequency, bandwidth, spreading factor, coding rate, sync word, and CRC setting. Leaving the LoRa page stops the active transmit or receive task.

### 9. Audio and Internet radio

- **Audio -> Microphone** initializes I2S and shows left/right levels while the page is open; I2S is released on exit.
- **Audio -> Speaker Test** runs the speaker test.
- **Audio -> Input Test** shows BOOT and TOUCH input states.
- **Internet Radio** selects and plays an MP3 stream over WiFi and shows the station, stream title, playback state, and volume. Playback stops when leaving the radio page.

### 10. Build, upload, and serial monitor

platformio.ini points src_dir to examples/AstraPort. The T-Glass environment is recommended:

~~~powershell
pio run -e T-Glass
pio run -e T-Glass -t upload
pio device monitor -b 115200
~~~

PlatformIO IDE can use the T-Glass environment for Build, Upload, and Monitor. The Factory_Astra environment is also kept in platformio.ini and can be selected with `pio run -e Factory_Astra`.

Gesture recognition uses the pinned ESP-DL source under third_party/esp-dl. PlatformIO checks this dependency during the pre-build step. If the repository was downloaded as a ZIP, the missing ESP-DL source is downloaded and prepared automatically; a normal Git clone may still initialize the submodule with `git submodule update --init --recursive`. The detector and classifier models are embedded through board_build.embed_files; do not remove or rename files under examples/AstraPort/models.

WiFi supports two build-time credential pairs. Factory_Astra defaults to the placeholder values shown in this example, and local build_flags can override them:

~~~cpp examples\AstraPort\AstraGlassServices.h
#define WIFI_SSID "YOUR_WIFI_SSID"
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"
#define WIFI_SSID2 "YOUR_WIFI_SSID_2"
#define WIFI_PASSWORD2 "YOUR_WIFI_PASSWORD_2"
~~~

At startup the device tries the first pair, then switches to the other pair every 30 seconds if it cannot connect. After a connection is lost, it continues rotating through both pairs. Set your own SSIDs and passwords locally and do not commit real credentials. The serial speed is 115200.

### 11. Troubleshooting

- **WiFi does not connect**: check the SSID and password used at build time, inspect the status bar, the Network page, and the serial monitor; reconnect is automatic.
- **No streaming URL**: ensure WiFi is connected and remain on Camera -> Camera Stream.
- **3D Gesture does not start**: leave Camera Stream, enable gesture recognition, and ensure enough contiguous PSRAM is available. Streaming and gesture recognition are mutually exclusive.
- **LoRa receives nothing**: verify that both devices use identical LoRa settings, the antennas are connected, the transmitter is on Continuous Transmit, and the receiver is on Receive Monitor.
- **Display is offset or too dark**: use Device -> Display Calibration and Device -> Brightness; saved calibration is stored in Preferences.

### 12. Directory layout

~~~text
examples/AstraPort/
├── camera/
│   ├── CameraWebServer/    JPEG web page, capture, and MJPEG stream
│   └── Gesture3D/          ESP-DL recognition and 3D renderer
├── game/                   Dino Jump game logic and renderer
├── models/                 Gesture detector and classifier models
├── AstraPort.ino           Application entry point
├── AstraPortModel.cpp      Menu tree
├── AstraGlassServices.cpp  WiFi, camera, audio, LoRa, and settings services
└── platformio_src.py       Factory_Astra build source configuration
~~~

## 中文说明

### 1. 项目特色

- **Astra 风格界面**：保留磁贴、列表、选择器、XBM 图标和页面导航。
- **状态栏**：普通界面顶部 10 像素显示 WiFi 图标、时间和电池百分比。
- **相机模块**：相机始终使用 JPEG，当前状态页显示 QVGA 320x240；相机、图传和手势识别分别放在独立模块目录中。
- **3D 手势识别**：识别手势 1 到 5，并在屏幕上显示白色 3D 数字，数字带轻微上下、左右和前后旋转。
- **手势控制**：可使用手势 1 到 5 控制菜单和网络电台。
- **JPEG 图传推流**：通过浏览器访问相机网页或 MJPEG 流，推流菜单位于“相机 -> 图传推流”。
- **WiFi 管理**：支持两组 WiFi 凭据；可以在设置中开启或关闭 WiFi，断开后程序会在两个网络之间轮换重连。
- **LoRa 测试**：支持连续发送、接收监视、参数显示和无线状态；连续发送内容为 Hello 1、Hello 2、Hello 3……，间隔约 1 秒。
- **网络电台**：使用 WiFi 播放多个网络电台，并显示电台名称、流标题、播放状态和音量。
- **设备设置**：支持中文/English、屏幕校准、亮度、休眠和工厂诊断。
- **资源生命周期管理**：麦克风、LoRa 收发、网络电台、图传服务器和手势识别只在对应页面需要时运行，降低资源冲突。

### 2. 屏幕布局

- T-Glass V3 物理屏幕为 126x126。
- Factory_Astra 普通界面使用 126x74 区域：顶部 10 像素是状态栏，下面 126x64 是 Astra 内容区。
- 3D 手势识别和恐龙跳一跳使用完整屏幕，不叠加普通状态栏。
- 画面不拉伸，中文和英文菜单共用同一套页面结构。

### 3. 菜单结构

~~~text
Astra 首页
├── 相机
│   ├── 摄像头状态
│   ├── 3D 手势识别
│   ├── 手势控制
│   └── 图传推流
├── 音频
│   ├── 麦克风音量
│   ├── 扬声器测试
│   └── 输入按键测试
├── 网络电台
│   └── 电台列表
├── LoRa
│   ├── 连续发送
│   ├── 接收监视
│   ├── LoRa 参数
│   └── 无线状态
├── 网络
│   ├── WiFi 状态
│   ├── WiFi 扫描
│   └── 时间与 NTP
├── 游戏
│   └── 恐龙跳一跳
└── 设备
    ├── 电池
    ├── 屏幕校准
    ├── 亮度
    ├── 语言
    │   ├── 中文
    │   └── English
    ├── 设置
    │   ├── WiFi
    │   └── 手势识别
    ├── 休眠
    └── 工厂诊断
~~~

### 4. 按键操作

| 输入 | 短按 | 长按约 1 秒 |
| --- | --- | --- |
| GPIO1 / TOUCH | 下移、选择下一项 | 进入页面、确认或执行操作 |
| GPIO0 / BOOT | 上移、选择上一项 | 按住约 1 秒后松开返回上一级；持续按住 5 秒关机 |

特殊页面：

- **恐龙跳一跳**：TOUCH 短按开始/重新开始，游戏中可跳跃；BOOT 短按跳跃；BOOT 长按退出游戏。当前恐龙跳得更高、滞空时间更长，可以跨越更远距离。
- **网络电台**：短按移动电台列表，长按选择电台；BOOT 长按退出并停止播放。
- **BOOT 时序**：BOOT 按住约 1 秒后松开返回上一级；持续按住 5 秒请求立即关机，检测到 USB/VBUS 接入时会阻止关机。
- **屏幕校准**：选择上移、下移、左移或右移后长按 TOUCH，每次调整 5 像素；选择保存后写入设置。
- **亮度**：选择 25%、50%、75% 或 100%。
- **休眠**：进入 ESP32 深度睡眠，GPIO1 可用于唤醒。

### 5. 语言和设置

语言菜单路径为 **设备 -> 语言**。选择中文或 English 后，右侧复选标识会立即更新，菜单标题、列表和状态文字会立即切换。语言选择保存在 Preferences 的 astra_ui 命名空间中，重启后保留。

WiFi 和手势识别开关路径为 **设备 -> 设置**：

- WiFi 默认开启，程序首先尝试 WIFI_SSID，连接失败后切换到 WIFI_SSID2。
- 手势识别默认关闭。进入“3D 手势识别”或“手势控制”时，如果开关关闭，屏幕会提示先到设置中开启。
- WiFi 和手势识别设置都会保存，重启后继续生效。

### 6. 3D 手势识别和手势控制

1. 进入 **设备 -> 设置 -> 手势识别**，选择开启。
2. 进入 **相机 -> 3D 手势识别**。
3. 刚进入页面时会显示“请显示手势 1-5”；识别成功后只显示对应的 3D 数字，不叠加识别文字。
4. 可识别数字 1、2、3、4、5。

进入 **相机 -> 手势控制** 后，手势含义如下：

| 手势 | 功能 |
| --- | --- |
| 1 | 上一项 |
| 2 | 下一项 |
| 3 | 确认/打开 |
| 4 | 返回 |
| 5 | 网络电台播放/暂停 |

手势控制页退出后控制模式仍可能保持运行，直到关闭手势识别，或进入 3D 手势、图传推流等其他相机模式。3D 手势识别和图传推流不会同时占用相机和模型资源。

### 7. JPEG 图传推流

使用步骤：

1. 在 **设备 -> 设置 -> WiFi** 中确认 WiFi 已开启。
2. 等待 **网络 -> WiFi 状态** 显示已连接。
3. 进入 **相机 -> 图传推流**。
4. 设备屏幕会显示网页和推流地址，串口也会输出完整地址。
5. 在同一局域网的浏览器中打开地址。

服务地址：

~~~text
网页控制台： http://<设备IP>/
单帧截图：   http://<设备IP>/capture
状态接口：   http://<设备IP>/status
MJPEG 推流：  http://<设备IP>:81/stream
~~~

图传使用 JPEG QVGA 320x240。HTTP 控制网页和截图服务使用 80 端口，MJPEG 推流使用 81 端口。只有停留在“图传推流”页面时服务器才会运行，退出页面后会停止服务。进入手势识别时图传服务器也会停止，以释放连续 PSRAM。

如果 WiFi 未连接，图传页面会显示连接状态和配置的 SSID，不会启动服务器。WiFi 断开时程序默认每 30 秒发起一次重连；手势识别活动期间会暂停重连，退出手势页面后继续处理。

### 8. LoRa

进入 **LoRa -> 连续发送** 后，设备会按以下格式连续发送：

~~~text
Hello 1
Hello 2
Hello 3
...
~~~

每次发送完成后计数器递增，默认发送间隔约 1000 ms。串口会输出 TX 内容和完成计数。另一台设备进入 **LoRa -> 接收监视** 后，可以看到接收到的内容、RSSI 和 SNR。

当前默认参数：

~~~text
Frequency: 868.0 MHz
Bandwidth: 125.0 kHz
Spreading factor: SF10
Coding rate: 4/6
Sync word: 0x12
TX power: 22 dBm
Preamble: 15
CRC: disabled
~~~

收发两台设备必须使用相同的频率、带宽、扩频因子、编码率、同步字和 CRC 设置。LoRa 页面离开后会停止当前收发任务。

### 9. 音频和网络电台

- **音频 -> 麦克风音量**：进入页面后初始化 I2S 并显示左右声道音量，退出页面后释放 I2S。
- **音频 -> 扬声器测试**：执行扬声器测试。
- **音频 -> 输入按键测试**：显示 BOOT 和 TOUCH 输入状态。
- **网络电台**：选择电台后使用 WiFi 播放 MP3 流；页面显示电台、流标题、播放状态和音量。离开网络电台页面后停止播放。

### 10. 编译、烧录和串口

Factory_Astra 已经通过 platformio.ini 的 src_dir 指向 examples/AstraPort，推荐使用 T-Glass 环境。源码目录名保留为 `examples/AstraPort`，以兼容现有内部类名和模型链接符号：

~~~powershell
pio run -e T-Glass
pio run -e T-Glass -t upload
pio device monitor -b 115200
~~~

也可以在 PlatformIO IDE 中选择 T-Glass 环境执行 Build、Upload 和 Monitor。Factory_Astra 环境也保留在 platformio.ini 中，可使用 `pio run -e Factory_Astra`。

手势识别使用 `third_party/esp-dl` 下固定版本的 ESP-DL 源码。PlatformIO 会在预编译阶段检查依赖；如果用户下载的是 GitHub ZIP，缺少的 ESP-DL 源码会自动下载并准备，普通 Git 克隆仍然可以使用 `git submodule update --init --recursive` 初始化子模块。手势检测模型和手势分类模型会通过 board_build.embed_files 嵌入固件，不要删除或重命名 examples/AstraPort/models 下的模型文件。

WiFi 支持以下两组编译宏。Factory_Astra 默认使用当前示例中的占位值，也可以在本地 build_flags 中覆盖：

~~~cpp examples\AstraPort\AstraGlassServices.h
#define WIFI_SSID "YOUR_WIFI_SSID"
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"
#define WIFI_SSID2 "YOUR_WIFI_SSID_2"
#define WIFI_PASSWORD2 "YOUR_WIFI_PASSWORD_2"
~~~

程序启动时先连接第一组，连接失败后每 30 秒切换到另一组；已经连接的 WiFi 断开后也会继续轮换尝试。建议在本地环境中设置自己的网络名称和密码，不要将真实密码提交到仓库。串口速率为 115200。

### 11. 常见问题

- **WiFi 未连接**：检查编译配置中的 SSID 和密码，观察顶部 WiFi 图标、网络状态页和串口输出；程序会自动定时重连。
- **图传没有地址**：确认 WiFi 已连接，并且当前停留在“相机 -> 图传推流”页面。
- **3D 手势无法启动**：先退出图传页面，确认手势开关已开启，并保证设备有足够连续 PSRAM。图传和手势识别不会并行运行。
- **LoRa 收不到**：确认两台设备使用相同的 LoRa 参数、天线连接正常，并让发送端进入连续发送、接收端进入接收监视。
- **画面偏移或太暗**：使用“设备 -> 屏幕校准”和“设备 -> 亮度”修正，校准保存后会写入 Preferences。

### 12. 目录结构

~~~text
examples/AstraPort/
├── camera/
│   ├── CameraWebServer/    JPEG 网页、截图和 MJPEG 推流
│   └── Gesture3D/          ESP-DL 手势识别和 3D 渲染
├── game/                   恐龙跳一跳游戏逻辑和渲染
├── models/                 手势检测与分类模型
├── AstraPort.ino           程序入口
├── AstraPortModel.cpp      菜单树
├── AstraGlassServices.cpp  WiFi、相机、音频、LoRa 和设置服务
└── platformio_src.py       Factory_Astra 编译源文件配置
~~~
