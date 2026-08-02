---
name: build-firmware
description: Build, flash, and debug the Slate ESP32-S3 firmware using ESP-IDF.
allowed-tools: Bash(idf.py:*) Bash(gcc:*) Bash(esptool:*) Bash(arm-none-eabi-*) Bash(riscv32-esp-elf-*)
---

# Firmware Build Skill

Slate 固件基于 ESP-IDF v5.5.x，目标芯片 ESP32-S3。

## Quick start

```bash
# Source ESP-IDF environment (required once per shell session)
source $IDF_PATH/export.sh

# Build firmware
idf.py -C firmware build

# Flash to device
idf.py -C firmware -p <serial> flash monitor
```

如果 ESP-IDF 未安装，先安装 v5.5.2：

```bash
git clone --branch v5.5.2 --depth 1 https://github.com/espressif/esp-idf.git /home/kevin/espressif/esp-idf
/home/kevin/espressif/esp-idf/install.sh
source /home/kevin/espressif/esp-idf/export.sh
```

## Build commands

```bash
# Full build (from repo root)
idf.py -C firmware build

# Clean rebuild
idf.py -C firmware clean build

# Reconfigure (change ESP-IDF settings)
idf.py -C firmware menuconfig

# Show size info
idf.py -C firmware size

# Generate merged binary (CI release artifact)
idf.py -C firmware build
idf.py -C firmware merge-bin -o slate-full.bin
```

产物位于 `firmware/build/`：
- `slate.bin` — 完整固件镜像（含 app + partition table）
- `slate-ota.bin` — OTA 用 app 镜像（仅 app 分区内容）
- `slate.elf` — ELF 调试符号

## Flash and monitor

```bash
# Flash all partitions + enter monitor
idf.py -C firmware -p /dev/ttyUSB0 flash monitor

# Flash only (no monitor)
idf.py -C firmware -p /dev/ttyUSB0 flash

# Monitor only (serial log output)
idf.py -C firmware -p /dev/ttyUSB0 monitor

# Erase flash completely
idf.py -C firmware -p /dev/ttyUSB0 erase-flash
```

按 `Ctrl+]` 退出 monitor。

## Target and configuration

- Target、Flash 大小、PSRAM、分区表等已在 `firmware/sdkconfig.defaults` 固化，无需 `idf.py set-target`。
- 修改配置：`idf.py -C firmware menuconfig`，或编辑 `sdkconfig.defaults`。
- 关键配置在 `firmware/main/Kconfig.projbuild`。

## Common issues

### ESP-IDF not found

```bash
# Check if IDF_PATH is set
echo $IDF_PATH

# If empty, source the export script
source /home/kevin/espressif/esp-idf/export.sh
```

### Build failures

- 确保使用 ESP-IDF v5.5.x（当前测试版本 v5.5.2）。
- 清理缓存重试：`idf.py -C firmware fullclean build`。

### Flash failures

- 检查串口权限：`ls -la /dev/ttyUSB*` 或 `/dev/ttyACM*`。
- 进入下载模式：按住 BOOT 键，复位设备，再 flash。
- 降低波特率：`idf.py -C firmware -p /dev/ttyUSB0 flash --baud 115200`

### Debug tips

- 串口日志在 monitor 模式下实时输出，日志级别可在 menuconfig 中调整。
- 使用 `esp-idf-monitor` 自动解码 panic 和 backtrace。
- GDB 调试：`idf.py -C firmware debug` 或连接 JTAG。

## Hardware reference

| 组件 | 规格 |
| --- | --- |
| MCU | ESP32-S3-WROOM-1 N16R8V, 16 MB Flash + 8 MB PSRAM |
| 显示 | 4.2" EPD 400x300, SSD2683 |
| 音频 | ES8311 + MEMS mic |
| 按键 | ENTER(GPIO0) / UP(GPIO39) / DOWN(GPIO18) |

## Partitions

```
nvs       0x9000     4 KB   NVS storage
phy_init  0xf000    4 KB   PHY params
factory   0x10000   4 MB   Factory app
storage   0x410000  12 MB  LittleFS cache
```

## Specific tasks

* **Flash firmware to device** [references/flash.md](references/flash.md)
* **Debug with GDB/JTAG** [references/debug.md](references/debug.md)
* **Configure ESP-IDF options** [references/menuconfig.md](references/menuconfig.md)
* **Release build artifacts** [references/release.md](references/release.md)
