# LCD4Linux for QTKeJi SmatCool USB LCD (AX206-based)

Fork of [ukoda/lcd4linux-ax206](https://github.com/ukoda/lcd4linux-ax206) with support for
**QTKeJi SmatCool** USB LCD display (320x240), available on AliExpress.

## Device

- **Name**: QTKeJi SmatCool
- **Resolution**: 320x240
- **USB ID**: `2a55:9990`
- **Protocol**: AX206-compatible (Bulk USB, Vendor Specific class)
- **Works with**: AIDA64 on Windows, lcd4linux on Linux

## Features added over upstream

- Support for `2a55:9990` USB ID (SmatCool display)
- `widget_histogram` — scrolling history graph widget
- Build fixes for modern GCC (gnu17, incompatible pointer types)
- Build fix for `plugin_mysql.c`
- Example config for AMD CPU + AMD GPU monitoring

## Dependencies (Arch/CachyOS)
```bash
sudo pacman -S gettext autoconf automake libtool libusb-compat dbus gd sqlite ncurses
```

## Build
```bash
git clone https://github.com/trenersolomon/lcd4linux-ax206-smatcool.git
cd lcd4linux-ax206-smatcool
./bootstrap
./configure --with-drivers=DPF
make CFLAGS="-DHAVE_CONFIG_H -I. -I/usr/include/dbus-1.0 -I/usr/lib/dbus-1.0/include \
  -D_GNU_SOURCE -Wall -fno-strict-aliasing -g -O2 \
  -Wno-incompatible-pointer-types -std=gnu17"
sudo make install
```

## Config
```bash
sudo chmod 600 /etc/lcd4linux.conf
sudo lcd4linux -f /etc/lcd4linux.conf -F
```

### Autostart with systemd
```bash
sudo nano /etc/systemd/system/lcd4linux.service
```
```ini
[Unit]
Description=LCD4Linux SmatCool display
After=multi-user.target

[Service]
Type=simple
ExecStart=/usr/local/bin/lcd4linux -f /etc/lcd4linux.conf -F
User=root
Restart=on-failure
RestartSec=10

[Install]
WantedBy=multi-user.target
```
```bash
sudo systemctl daemon-reload
sudo systemctl enable --now lcd4linux
```

## Data sources (AMD Ryzen CPU + AMD RX 7900)

| Data | Path |
|------|------|
| CPU usage | `proc_stat::cpu('busy', 1)` |
| CPU temperature | `/sys/class/hwmon/hwmon7/temp1_input` ÷ 1000 |
| CPU frequency | `/sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq` ÷ 1000 |
| RAM free | `meminfo('MemAvailable')` |
| GPU usage | `/sys/class/drm/card1/device/gpu_busy_percent` |
| GPU temperature | `/sys/class/hwmon/hwmon4/temp1_input` ÷ 1000 |
| GPU frequency | `/sys/class/drm/card1/device/hwmon/hwmon4/freq1_input` ÷ 1000000 |
| VRAM used | `/sys/class/drm/card1/device/mem_info_vram_used` |
| VRAM total | `/sys/class/drm/card1/device/mem_info_vram_total` |

> **Note**: hwmon numbers may differ on your system.
> Check with: `cat /sys/class/hwmon/hwmon*/name`

## Credits

- [lcd4linux](https://github.com/jmccrohan/lcd4linux) — original project
- [ukoda/lcd4linux-ax206](https://github.com/ukoda/lcd4linux-ax206) — upstream fork
- [dpf-ax](https://github.com/dreamlayers/dpf-ax) — AX206 protocol
