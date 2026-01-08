# <img src="src/assets/cangaroo.png" width="48" height="48"> Cangaroo
**Open-source CAN bus analyzer for Linux, Automotive, Robotics, and Industrial Applications**

[![Try Cangaroo Now](https://img.shields.io/badge/Try-Cangaroo-blue?style=for-the-badge)](https://github.com/OpenAutoDiagLabs/cangaroo/releases/latest)

![GitHub stars](https://img.shields.io/github/stars/OpenAutoDiagLabs/cangaroo?style=social)
![GitHub forks](https://img.shields.io/github/forks/OpenAutoDiagLabs/cangaroo?style=social)
![GitHub issues](https://img.shields.io/github/issues/OpenAutoDiagLabs/cangaroo)
![License](https://img.shields.io/github/license/OpenAutoDiagLabs/cangaroo)
![Release](https://img.shields.io/github/v/release/OpenAutoDiagLabs/cangaroo)

Cangaroo is a professional-grade CAN bus analyzer designed for engineers in **Automotive**, **Robotics**, and **Industrial Automation**. It facilitates real-time capture, decoding, and analysis of CAN and CAN‑FD traffic.

> **Works with SocketCAN (Linux), CANable, Candlelight, and CANblaster for immediate testing and real hardware connections.**

---

## 🎥 Demo Gallery
*Real-time capture and decoding of CAN traffic using DBC databases.*
<br>![Cangaroo Trace View](test/trace_view.gif)<br>
<!-- slide -->
*Simulate CAN traffic with customizable periodic and manual transmissions.* 
<br>![Cangaroo Generator View](test/generator_view.gif)<br>
<!-- slide -->
*Flexible dockable workspace optimized for multi-monitor analysis.*
<br>![Cangaroo Overview](test/output.gif)<br>

---

## 🚀 Key Features

*   **Real-time CAN/CAN-FD Decoding**: Support for standard and high-speed flexible data-rate frames.
*   **Wide Hardware Compatibility**: Seamlessly works with **SocketCAN** (Linux), **CANable** (SLCAN), **Candlelight**, and **CANblaster** (UDP).
*   **DBC Database Support**: Load multiple `.dbc` files to instantly decode frames into human-readable signals.
*   **Data Visualization**: Integrated graphing tools to visualize signal changes over time.
*   **Advanced Filtering & Logging**: Isolate critical data with live filters and export captures for offline analysis.
*   **Modern Workspace**: A clean, dockable userinterface optimized for multi-monitor setups.

---

## 🛠️ Installation & Setup (Linux)

### Option 1: Build from Source
Getting started is as simple as running our automated setup script:

```bash
git clone https://github.com/OpenAutoDiagLabs/CANgaroo.git
cd CANgaroo
./install_linux.sh
```
Follow the interactive menu to install dependencies and build the project.

### Option 2: Using Release Package
If you downloaded a pre-compiled release tarball, use the included setup script to prepare your environment:

1. Extract the package: `tar -xzvf cangaroo-vX.Y.Z-linux-x86_64.tar.gz`
2. Run the setup script: `./setup_release.sh`
3. Select an option to install dependencies and/or install Cangaroo to `/usr/local/bin`.

### Manual Dependency Installation

| Distribution | Command |
| :--- | :--- |
| **Ubuntu / Debian** | `sudo apt install qt6-base-dev libqt6charts6-dev libqt6serialport6-dev build-essential libnl-3-dev libnl-route-3-dev` |
| **Fedora** | `sudo dnf install qt6-qtbase-devel qt6-qtcharts-devel qt6-qtserialport-devel libnl3-devel` |
| **Arch Linux** | `sudo pacman -S qt6-base qt6-charts qt6-serialport libnl` |

---

## 🚦 Quick Start Guide

### 1. Zero-Hardware Testing (Virtual CAN)
```bash
sudo modprobe vcan
sudo ip link add dev vcan0 type vcan
sudo ip link set up vcan0
```

### 2. Remote CAN Monitoring (SSH Pipe)
Monitor traffic from a remote machine (e.g., a Raspberry Pi on your vehicle) on your local PC:
```bash
# On your local machine, setup vcan0 as shown above, then:
ssh user@remote-ip "candump -L can0" | canplayer vcan0=can0 -t
```
*Now open Cangaroo and connect to `vcan0` to see the remote traffic.*

### 3. ARXML to DBC Conversion
Cangaroo natively supports DBC. If you have ARXML files, you can convert them using `canconvert`:
```bash
# Install canconvert
pip install canconvert

# Convert ARXML to DBC
canconvert TCU.arxml TCU.dbc
```

---

## 📥 Downloads & Releases

Download the latest release tarball from the [Releases Page](https://github.com/OpenAutoDiagLabs/cangaroo/releases).

---

## 🤝 Contribution & Community

### Verifying Your Download
All official releases include a SHA256 checksum. Verify your download using:

```bash
sha256sum cangaroo-v0.4.3-linux-x86_64.tar.gz
# Expected output: 
# abc123def456...  cangaroo-v0.4.3-linux-x86_64.tar.gz
```

---

## 🤝 Contribution & Community

We welcome contributions!
*   **Report Bugs**: Open an issue on our [GitHub Tracker](https://github.com/OpenAutoDiagLabs/cangaroo/issues).
*   **Suggest Features**: Start a discussion in the [Discussions Tab](https://github.com/OpenAutoDiagLabs/cangaroo/discussions).
*   **Contribute Code**: See [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines.

---

## 📜 Credits

*   **Original Author**: Hubert Denkmair ([hubert@denkmair.de](mailto:hubert@denkmair.de))
*   **Lead Maintainer**: [Jayachandran Dharuman](https://github.com/OpenAutoDiagLabs/cangaroo)

---

## 📝 Changelog Summary (v0.4.3)
* **Qt 6.10 Migration**: Full support for the latest Qt framework.
* **SocketCAN Stability**: Resolved driver crashes during interface discovery.
* **UI Streamlining**: Removed redundant toggles for a cleaner analysis experience.

---

**Keywords**: CAN bus analyzer Linux, SocketCAN GUI, CAN FD decoder, automotive diagnostic tool, open-source CAN tool.

**License**: [GPL-3.0+](LICENSE)
