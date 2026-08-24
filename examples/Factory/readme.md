# Factory Test Example

[Chinese](readme_cn.md)

`Factory.ino` is the T-Glass V3 factory test firmware. It provides quick checks for the camera, display, touch button, BOOT button, speaker, microphones, time, LoRa, battery, and Wi-Fi. It can also adjust and save the visible UI position.

## Preparation

1. Connect a LoRa antenna matching the test frequency and connect the test battery.
2. Connect the board to a computer with a USB data cable. The serial baud rate is `115200`.
3. Start at least one of these Wi-Fi access points:

   | SSID | Password |
   | --- | --- |
   | `xinyuandianzi` | `AA15994823428` |
   | `LilyGo-AABB` | `xinyuandianzi` |

4. Power on the board. The firmware opens the T0 camera page after Wi-Fi connects. If neither access point is available, startup waits for a connection.

## Test Pages

Short-touch the touch button to select the next page. A tone plays after every page change.

| Page | Display | Test |
| --- | --- | --- |
| T0 | Live camera image | Camera and display |
| T1 | `Mic Level` | Left and right microphones |
| T2 | Time, weekday, and month | Wi-Fi/NTP time |
| T3 | `LoRa Tx` | Periodic LoRa transmission |
| T4 | `LoRa Rx` | Received data, RSSI, and SNR |
| T5 | `Volts` | Battery voltage and level |
| T6 | `RSSI` | Wi-Fi connection and signal strength |
| T7 | Four animated direction icons | Image and animation |
| T8 | `Screen Move` | Visible UI position adjustment |

## Controls

- **Touch button**: Selects the next page from T0 through T7. On T8, it returns to T0 when adjustment mode is not active.
- **BOOT single-click**: In T8 adjustment mode, cycles through `UP -> DOWN -> LEFT -> RIGHT`.
- **BOOT double-click**: On T8, enters adjustment mode. Double-click again to save the position and return to T0.
- **BOOT long-press**: Shows `Sleep.`, enters sleep, and wakes from a touch-button press.

### T8 Position Adjustment

1. Open T8 and double-click BOOT. The title changes from `Screen Move` to `Adjusting`.
2. Single-click BOOT to select a direction.
3. Short-touch the touch button to move 5 units in the selected direction.
4. Double-click BOOT to save and exit.
5. Reboot and verify that the saved position is restored.

Both X and Y offsets are limited to `-63` through `63`.

## LoRa Test

LoRa testing requires a known-good second device with matching parameters. Use the actual frequency, bandwidth, SF, CR, sync word, and preamble shown in the startup serial log.

- T3 periodically transmits `Hello #n`. Confirm reception on the second device.
- T4 displays the received data, RSSI, and SNR.
- LoRa parameters are stored in NVS and remain active after reboot.

## Common Serial Commands

| Command | Function |
| --- | --- |
| `next` | Select the next page |
| `send` | Open LoRa Tx |
| `recv` | Open LoRa Rx |
| `wifi` | Scan nearby Wi-Fi networks, then reconnect automatically |
| `touchRead` | Read the raw touch value |
| `freq:<MHz>` | Set the LoRa frequency |
| `bw:<kHz>` | Set the LoRa bandwidth |
| `sf:<value>` | Set the spreading factor |
| `cr:<value>` | Set the coding rate |
| `sw:<decimal>` | Set the sync word; `sw:18` means `0x12` |
| `tp:<dBm>` | Set the transmit power |
| `pl:<value>` | Set the preamble length |

See [factory_test_guide_cn.md](test/factory_test_guide_cn.md) for the complete factory test procedure and acceptance criteria.
