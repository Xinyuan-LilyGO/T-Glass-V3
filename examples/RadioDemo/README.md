# T-Glass V3 RadioDemo

这是一个不依赖 LVGL 的网络广播播放示例，目标板为 LilyGo T-Glass V3。

播放链路为：

```text
Wi-Fi HTTP/ICY MP3 -> ESP8266Audio MP3 decoder -> ES8311 -> speaker
```

## 配置

Wi-Fi 和电台列表位于 `RadioDemoConfig.h`：

```cpp
static const char kWifiSsid[] = "YOUR_WIFI_SSID";
static const char kWifiPassword[] = "YOUR_WIFI_PASSWORD";
```

当前内置 22 个中国境内的 HTTP MP3 示例流：

- Shanghai Dynamic 101: `http://lhttp.qingting.fm/live/274/64k.mp3`
- Qingting FM 5022: `http://lhttp.qingting.fm/live/5022/64k.mp3`
- Beijing Literary Radio: `http://lhttp.qingting.fm/live/333/64k.mp3`
- 其余电台列表见 `RadioDemoConfig.h`，包括怀集音乐之声、两广之声音乐台、上海流行音乐 LoveRadio、北京音乐广播等。

电台地址由用户提供，来源参考：[CSDN 电台列表原文](https://blog.csdn.net/weixin_48969002/article/details/151379970)，遵循原文 CC 4.0 BY-SA 版权声明。

如果某个公共流不可用，直接在配置头文件替换对应 URL 即可。当前只支持 HTTP/ICY MP3，不支持 HTTPS 或 AAC。

## 内存与流畅度

播放器使用板载 PSRAM 保存 64 KB 网络环形缓冲和 32 KB MP3 解码预分配区，切台与重连时复用，不反复申请大块内部堆内存。解码器会先等待 24 KB 网络数据，最多等待 6 秒后才开始播放，并保留 8 KB 的最低启动水位。RadioDemo 同时关闭 Wi-Fi 省电模式，使用有限重连和兼容性更好的 I2S DMA 配置，减少网络流的周期性延迟。

ES8311 初始配置为 44.1 kHz；MP3 解码器识别到 16/22.05/24/32/44.1/48 kHz 等采样率后，会同步更新 codec 和 I2S，避免电台采样率与 codec 配置不一致造成卡顿。

## 按键

- GPIO1 触摸短按：切换到下一个电台并重新播放。
- GPIO0 BOOT 短按：播放/暂停。
- GPIO0 BOOT 长按：音量增加 10%，到 100% 后回到 10%。

播放状态、当前电台、音量和 ICY 标题通过 115200 波特率串口输出。

## 编译入口

RadioDemo 是独立的 PlatformIO 环境，不需要修改根目录的全局 `src_dir`。根目录默认环境和其他示例可以保持原样，使用以下环境名选择 RadioDemo：

```powershell
pio run -e RadioDemo
pio device monitor -e RadioDemo
```

该环境通过 `examples/RadioDemo/platformio_src.py` 将源目录固定到当前示例，因此不会误编译 `examples/LilyGo3D` 或其他全局 `src_dir` 指向的目录。

RadioDemo 的播放器核心不包含 LVGL、U8g2 或 Astra 头文件，后续可以直接由 `examples/AstraPort` 的 `AstraGlassServices` 持有和轮询，再由 Astra 页面负责绘制状态。

本次修改按要求未执行 PlatformIO 编译、烧录、上传或设备运行验证；仅执行了静态回归检查。
