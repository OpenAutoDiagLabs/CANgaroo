# ![logo](/src/cangaroo.ico) **CANgaroo**

An open-source CAN bus analyzer with support for transmit/receive of standard and FD frames and DBC decoding of incoming frames.

**Supported interfaces:**

* [CANable](http://canable.io) SLCAN interfaces on Windows and Linux
* [CANable 2](http://canable.io) SLCAN interfaces on Windows and Linux with FD support
* Candlelight interfaces on Windows
* SocketCAN interfaces on Linux
* [CANblaster](https://github.com/normaldotcom/canblaster) SocketCAN over UDP server with auto-discovery
* GrIP Driver

![demo1](/test/output.gif)

Written by Hubert Denkmair [hubert@denkmair.de](mailto:hubert@denkmair.de)

Further development by:

* Jayachandran Dharuman ([https://github.com/OpenAutoDiagLabs/cangaroo](https://github.com/OpenAutoDiagLabs/cangaroo))

---

## Building on Linux

### Prerequisites (Ubuntu 24.04 and similar)

To set up a new development environment, you can follow one of the methods below:

**Method 1: Basic Universe Repository Setup**

```bash
sudo add-apt-repository universe
sudo apt update
```

**Method 2: Universe Repository + Qt6 Core Packages**

```bash
sudo add-apt-repository universe
sudo apt update
sudo apt install qt6-base-dev qt6-charts-dev qt6-serialport-dev
```

**Method 3: Full Package Install (Recommended)**

```bash
sudo apt install \
    qt6-base-dev \
    libqt6charts6-dev \
    libqt6serialport6-dev \
    build-essential git cmake \
    libnl-3-dev libnl-route-3-dev libgl1-mesa-dev
```

These commands ensure your system has all the required Qt6 libraries, CAN-related dependencies, and build tools.

### Build Instructions

```bash
git clone https://github.com/OpenAutoDiagLabs/cangaroo

cd CANgaroo/src && PKG_CONFIG_PATH=/usr/lib/x86_64-linux-gnu/pkgconfig qmake6 && cd .. && PKG_CONFIG_PATH=/usr/lib/x86_64-linux-gnu/pkgconfig make -j$(nproc)
```

The binary will be located in `../bin/cangaroo`.

---

## Building on Windows

* Qt Creator (Community Version is sufficient) brings everything you need.
* **PCAN Libraries**:

  * Download from [peak-system.com](http://www.peak-system.com/fileadmin/media/files/pcan-basic.zip)
  * Extract to `src/driver/PeakCanDriver/pcan-basic-api`.
  * Ensure `PCANBasic.dll` is in the same folder as the `.exe` file.
* To disable Peak support, comment out `win32:include($$PWD/driver/PeakCanDriver/PeakCanDriver.pri)` in `src/src.pro`.

---

## Usage Notes

### Workspace Management

* **Manual Persistence**: Workspaces are now explicitly managed. Use **File -> Save Workspace As** to create a configuration file.
* **No Hidden Cache**: The application no longer uses a hidden cache for the "last session". You must open your saved `.cangaroo` workspace file to restore your settings.
* **Auto-Save**: If a workspace file is currently open, changes will be saved to that file when closing the application.

### Log Exporting

* Open the **Log View** to see the new **"Export Logs..."** button.
* You can save the current log stream to a formatted `.log` or `.txt` file for external analysis.
* Use the **"Clear"** button in the Log View to reset the log display at any time.

---

## Changelog

### v0.4.0

* **Explicit Workspace Persistence**: Removed unreliable hidden cache system. Workspaces are now saved directly to user-specified files.
* **Log Export Feature**: Export logs to `.log` and `.txt` files.
* **Layout Restoration**: Optimized dock widgets to ensure "CAN Status" and "Generator View" are always visible.
* **Stability Improvements**: Improved memory management and null-pointer protection for DBC and XML parsing.
* **Build Fixes**: Added explicit include paths for `libnl3` for modern Linux distributions.
* **Version Update**: Updated versioning to 0.4.0.

### v0.3.0

* Migrate to Qt6
* Added GrIP driver

### v0.2.4.1

* General bugfixes
* Add WeAct Studio Support
* Initial Translation support

### v0.2.4

* Add initial support for CANFD
* Add support for SLCAN interfaces on Windows and Linux (CANable, CANable 2.0)
* Add support for [CANblaster](https://github.com/normaldotcom/canblaster) socketCAN over UDP
* Add live filtering of CAN messages in trace view

### v0.2.1

* Improved logging
* Refactorings
* Scroll trace view per pixel, always show last message when autoscroll is on

### v0.2.0

* Docking windows system instead of MDI interface
* Windows build and PCAN-basic driver
* Handle muxed signals in backend and trace window
* CAN status window
* Show timestamps, log level, etc. in log window

### v0.1.3

* New CAN interface configuration GUI
* Use libnl-route-3 for socketCAN device config read
* Query socketCAN interfaces for supported config options
* New logging subsystem, no longer using QDebug
* Performance improvements when receiving many messages
* Bugfix with time-delta view

### v0.1.2

* Fix device re-scan
* Fix some DBC parsing issues
* Implement big-endian signals

### v0.1.1

* Change source structure for Debian packaging
* Add Debian packaging info

### v0.1

* Initial release

---

## TODO

### Backend

* Support non-message frames in traces (e.g., markers)
* Implement plugin API
* Embed Python for scripting

### CAN Drivers

* Allow socketCAN interface config through suid binary
* Hardware timestamps for socketCAN if possible
* Cannelloni support
* Windows Vector driver

### Import / Export

* Export to other formats (Vector ASC, BLF, MDF)
* Import CAN-Traces

### General UI

* Style dock windows
* Load/save docks from/to config

### Log Window

* Filter log messages by level

### CAN Status Window

* Display #warnings, #passive, #busoff, #restarts

### Trace Window

* Assign colors to CAN interfaces/messages
* Limit displayed messages
* Show error and non-message frames
* Sort signals by startbit, name, or position in CANdb

### CANdb-based Generator

* Generate CAN messages from CANdbs
* Set signals according to value tables
* Provide generator functions for signals
* Allow scripting of signal values

### Replay Window

* Replay CAN traces
* Map interfaces in traces to available networks

### Graph Window

* Test QCustomPlot
* Graph interface stats, message stats, signals

### Packaging / Deployment

* Provide clean Debian package
* Flatpak support
* Provide statically linked binary
* Add Windows installer

---

This version includes your requested Linux setup steps with all three options clearly listed, while keeping the original README structure and content.
