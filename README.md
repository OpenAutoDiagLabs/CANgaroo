# <img src="src/cangaroo.ico" width="48" height="48"> CANgaroo - Professional CAN Bus Analysis

CANgaroo is a powerful, open-source CAN bus analyzer designed for engineers working in **Automotive**, **Robotics**, and **Industrial Automation**. Whether you are debugging a vehicle network, developing robot sensors, or monitoring industrial machinery, CANgaroo provides the tools you need to capture, decode, and analyze CAN traffic in real-time.

![Cangaroo Demo](test/output.gif)

---

## 🚀 Key Features

- **Wide Hardware Support**: Works with SocketCAN (Linux), CANable (SLCAN), Candlelight, and CANblaster (UDP).
- **CAN & CAN-FD**: Full support for both standard CAN and high-speed CAN-FD frames.
- **Real-time DBC Decoding**: Load multiple DBC files to see signals and messages decoded instantly in the trace view.
- **Integrated Graphics**: Graph signals over time to visualize sensor data and bus behavior.
- **Advanced Filtering**: Quickly isolate relevant traffic with powerful live filters.
- **Flexible Logging**: Export traces and logs to standard formats for offline analysis.
- **Modern UI**: Streamlined interface with dockable windows for a customizable workspace.

---

## 🛠️ Installation (Linux)

The easiest way to get started on Linux is to use our automated installation script:

```bash
git clone https://github.com/OpenAutoDiagLabs/cangaroo
cd cangaroo
./install_linux.sh
```

### Manual Installation by Distro

**Ubuntu / Debian / Mint**
```bash
sudo apt update
sudo apt install qt6-base-dev libqt6charts6-dev libqt6serialport6-dev build-essential libnl-3-dev libnl-route-3-dev
```

**Fedora**
```bash
sudo dnf install qt6-qtbase-devel qt6-qtcharts-devel qt6-qtserialport-devel libnl3-devel
```

**Arch Linux**
```bash
sudo pacman -S qt6-base qt6-charts qt6-serialport libnl
```

---

## 🏗️ Building from Source

Once dependencies are installed, follow these steps:

```bash
cd src
PKG_CONFIG_PATH=/usr/lib/x86_64-linux-gnu/pkgconfig qmake6 
make -j$(nproc)
```

The binary will be generated in `bin/cangaroo`.

---

## 🚦 Getting Started

### 1. Set up a Virtual CAN Interface (Linux)
If you don't have hardware yet, you can test CANgaroo using a virtual interface:

```bash
sudo modprobe vcan
sudo ip link add dev vcan0 type vcan
sudo ip link set up vcan0
```

### 2. Connect to an Interface
Launch CANgaroo, go to **Setup -> Interfaces**, and select your device (e.g., `vcan0` or a physical `can0`).

### 3. Load a DBC File
To decode messages, go to **Measurement -> Add CAN Database** and select your `.dbc` file. Signals will now appear in the Trace window.

---

## 📖 Examples

### Recording Traffic
- Connect to your interface.
- Hit the **Start** button.
- Use **File -> Export Logs** to save the captured data.

### Transmitting Data
- Open the **Generator View**.
- Define your message ID and data payload.
- Click **Send** to inject the frame into the bus.

---

## 📜 Credits & Contributors

Original Author: Hubert Denkmair ([hubert@denkmair.de](mailto:hubert@denkmair.de))
Lead Maintainer: [Jayachandran Dharuman](https://github.com/OpenAutoDiagLabs/cangaroo)

---

## 📝 Changelog Summary (v0.4.1)
- **Qt6 Migration**: Fully updated for Qt 6.10 compatibility.
- **Enhanced Persistence**: Explicit workspace management (Save/Load).
- **Improved Stability**: Fixed several critical segmentation faults in SocketCAN drivers.
- **Trace Simplification**: Streamlined Trace View by removing redundant AutoScroll and Aggregation toggles (now active by default).

*For a full history, see the [Changelog Section below](#full-changelog).*

---

<details>
<summary><b>Click to expand Full Changelog</b></summary>

### v0.4.1
* Qt 6.10 Migration
* Explicit Workspace Persistence
* Fixed SocketCAN stability issues
* UI Simplification (removed AutoScroll/Aggregate checkboxes)

### v0.3.0
* Migrate to Qt6
* Added GrIP driver

### v0.2.4.1
* Add WeAct Studio Support
* Initial Translation support

### v0.2.4
* Add initial support for CANFD
* Add live filtering of CAN messages in trace view
</details>
