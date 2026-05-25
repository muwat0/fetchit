# fetchit

fetchit is a terminal-based system information viewer written in C++. It works like neofetch/fastfetch and consists of a single file (`main.cpp`).

## Features

- Displays distro, kernel, uptime, shell, CPU, GPU, RAM, and OS date information.
- Detects distro name via `/etc/os-release`.
- Shows ASCII logos for supported distros.
- Reads `/usr/share/hwdata/pci.ids` for GPU names; prints hex IDs if missing.
- Accounts for Unicode character width (for icon alignment).

## Supported Distro Logos

`arch`, `cachyos`, `debian`, `ubuntu`, `fedora`, `manjaro`, `opensuse`, `pop`, `linuxmint`, `endeavouros`, `void`, `alpine`

## Requirements

- Linux
- A C++17-capable compiler (e.g., g++)
- A UTF-8 terminal with ANSI color support (for icons and colors)
- The following files/paths are read:
  - `/etc/os-release`
  - `/etc/hostname`
  - `/proc/uptime`
  - `/proc/cpuinfo`
  - `/proc/meminfo`
  - `/sys/bus/pci/devices/`
  - `/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq`
  - `/usr/share/hwdata/pci.ids` (optional)

## Build

```bash
g++ -std=c++17 -O2 -o fetchit main.cpp
```

## Run

```bash
./fetchit
```

## Sample Output

```text
                              --- murat@baklava ---

       /\           distro:     Arch Linux
      /  \          kernel:     7.0.10-arch1-1
     /    \         uptime:     1 minutes
    /      \        shell:      /usr/bin/bash
   /   ,,   \      󰍛 CPU:        12th Gen Intel(R) Core(TM) i5-12500H (16) @ 4.5 GHz
  /   |  |   \     󰾲 GPU:        AD107M [GeForce RTX 4060 Max-Q / Mobile] / Alder Lake-P GT2 [Iris Xe Graphics]
 /_-''    ''-_\     RAM:        15.2529 GB
                    OS Date:    6 months 17 days 3 hours 40 minutes
```

## Notes

- `OS Date` is based on the creation time of `/etc/hostname`.
- If no ASCII logo is found, only the info list is printed.
- The distro logo is displayed in color via ANSI escape codes.
- This tool targets Linux systems and is not expected to work on non-Linux OSes.
