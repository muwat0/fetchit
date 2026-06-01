# fetchit

`fetchit` is a Linux terminal system information viewer written in C++. It's a single-file program (`main.cpp`) that prints a colored distro logo (when available) alongside system and hardware info, similar to neofetch/fastfetch.

## Features

- Shows: distro, kernel, uptime, shell, CPU, GPU, RAM, and "OS Date".
- Distro detection via `/etc/os-release` (`PRETTY_NAME` for display, `ID` for logo matching).
- Colored ASCII distro logos for supported distros.
- GPU detection by scanning PCI devices (`/sys/bus/pci/devices/`) for class `0x03*`.
- GPU name resolution via `/usr/share/hwdata/pci.ids` (falls back to hex IDs if missing).
- Accounts for Unicode character width for better alignment in terminals.

## Configuration

`fetchit` can be configured via a TOML file.

### Config File Location (XDG)

`fetchit` searches for a config file in the following location:

- `$XDG_CONFIG_HOME/fetchit/config.toml`
- If `XDG_CONFIG_HOME` is not set: `~/.config/fetchit/config.toml`

If the config file is missing, defaults are used (no warning).

If the config file exists but is invalid TOML or has invalid values, `fetchit` prints warnings to `stderr` and continues using built-in defaults.

### Precedence

1. Built-in defaults
2. Config file
3. CLI flags (`--config`, `--no-config`)

### CLI Flags

- `-h`, `--help`: show help
- `-c <path>`, `--config <path>`: use an explicit config path
- `--no-config`: do not load any config file

### Supported Settings

- `modules = [...]` (array of strings)
  - Controls module order and visibility.
  - Supported module names:
    - `distro`, `kernel`, `uptime`, `shell`, `cpu`, `gpu`, `ram`, `os_date`

- `[logo]`
  - `enabled = true|false`

- `[labels]` (table)
  - Per-module label strings.
  - Icons are part of the label string (e.g. `" distro:"`).

- `[colors]` (table)
  - Per-module color names (strings).
  - Supported colors:
    - `red`, `green`, `blue`, `yellow`, `magenta`, `purple`, `cyan`, `gray`/`grey`, `dark`

### Example `config.toml`

```toml
modules = ["kernel", "distro", "cpu", "gpu", "ram"]

[logo]
enabled = true

[labels]
kernel = " kernel:"
distro = " distro:"
cpu = "󰍛 CPU:"
gpu = "󰾲 GPU:"
ram = " RAM:"

[colors]
kernel = "magenta"
distro = "green"
cpu = "yellow"
gpu = "yellow"
ram = "red"
```

### Default Config

This is the built-in default configuration (equivalent to running with no config file):

```toml
modules = ["distro", "kernel", "uptime", "shell", "cpu", "gpu", "ram", "os_date"]

[logo]
enabled = true

[labels]
distro  = " distro:"
kernel  = " kernel:"
uptime  = " uptime:"
shell   = " shell:"
cpu     = "󰍛 CPU:"
gpu     = "󰾲 GPU:"
ram     = " RAM:"
os_date = " OS Date:"

[colors]
distro  = "green"
kernel  = "magenta"
uptime  = "blue"
shell   = "magenta"
cpu     = "yellow"
gpu     = "yellow"
ram     = "red"
os_date = "blue"
```

## Supported Distro Logos

Logos are matched against `/etc/os-release` `ID`:

`arch`, `cachyos`, `debian`, `ubuntu`, `fedora`, `manjaro`, `opensuse`, `pop`, `linuxmint`, `endeavouros`, `void`, `alpine`

## Requirements

- Linux
- A C++17-capable compiler (default: `g++`)
- UTF-8 locale + ANSI color support (recommended for icons/colors)
- Reads the following files/paths:
  - `/etc/os-release`
  - `/etc/hostname`
  - `/proc/uptime`
  - `/proc/cpuinfo`
  - `/proc/meminfo`
  - `/sys/bus/pci/devices/`
  - `/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq`
  - `/usr/share/hwdata/pci.ids` (optional)

## Build

### Using the Makefile (recommended)

```bash
make
```

Binary output: `build/fetchit`

Run:

```bash
make run
```

### Manual build

```bash
g++ -O2 -Wall -Wextra -std=c++17 -pipe -o fetchit main.cpp
```

## Install / Uninstall

Install:

```bash
sudo make install
```

Defaults:

- `PREFIX=/usr/local`
- `BINDIR=$(PREFIX)/bin`
- Installs to: `/usr/local/bin/fetchit`

Uninstall:

```bash
sudo make uninstall
```

Packaging-friendly variables:

- `DESTDIR` (staging root)
- `PREFIX`, `BINDIR`

Example:

```bash
make install PREFIX=/usr BINDIR=/usr/bin DESTDIR=/tmp/pkgroot
```

## Distribution Archive

Create a tar.gz containing the built binary:

```bash
make dist
```

Output: `dist/fetchit-<version>-linux-x86_64-gnu.tar.gz`

`<version>` is derived from:
`git describe --tags --dirty --always` (when available)

Full release pipeline:

```bash
make release
```

## Notes

- `shell` is taken from `$SHELL` and printed as the basename (e.g. `bash`, not `/usr/bin/bash`).
- `OS Date` is computed from `/etc/hostname` file metadata (`ctime`) as an elapsed duration; it may not equal the OS install date.
- If `pci.ids` is unavailable, GPU(s) are printed as hex IDs like `0xVVVV:0xDDDD`.
- If no distro logo matches, the logo section will be empty and only info lines are shown.

## Sample Output

```text
                               --- user@host ---

      /\             distro:      Arch Linux
     /  \            kernel:      7.0.10-arch1-1
    /    \           uptime:      12 minutes
   /      \          shell:       bash
  /   ,,   \        󰍛 CPU:         ... (N) @ X.XX GHz
 /   |  |   \       󰾲 GPU:         ...
/_-''    ''-_\       RAM:         15.25 GB
                     OS Date:     6 months 17 days 3 hours 40 minutes
```
