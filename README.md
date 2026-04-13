## [Документация на Русском >>>](README_RU.md)

# MWOS3.5 — Modular OS for IoT (v.3.5)
![Version](https://img.shields.io/badge/version-3.5-blue.svg) ![License](https://img.shields.io/badge/license-MIT-green.svg) ![Platform](https://img.shields.io/badge/platform-Arduino%20Framework%20Compatible-orange.svg) ![Language](https://img.shields.io/badge/language-C++11%20%7C%20MWScript-yellow.svg)

**MWOS (Modular Web Operating System)** is an operating system for microcontrollers designed for remote monitoring and equipment control.

> 💡 **Key Advantage:** The MWOS architecture significantly simplifies new development. You write only the device interaction logic, while the OS handles networking, web interface, settings storage, and communication protocols.

## 📋 Table of Contents
- [Features](#-features)
- [Getting Started](#-getting-started)
- [Creating Firmware](#-creating-firmware)
- [Developing Your Own Modules](#-developing-your-own-modules)
- [Supporting New Platforms](#-supporting-new-platforms)
- [Manual Compilation](#-manual-compilation)
- [Technical Features](#-technical-features)
- [Contributing](#-contributing)
- [License](#-license)
- [Contacts](#-contacts)

## 🚀 Features
MWOS provides unprecedented platform independence, supporting over a hundred microcontrollers compatible with the Arduino Framework — from classic AVR and budget-friendly ESP8266 to powerful ESP32 and STM32. End users can immediately start working with their devices through a centralized web interface (iot.voscom.online or a self-hosted server) and the cross-platform mwosApp application: all settings, monitoring, and control are managed from a single point, while real-time data accumulates in a central SQL database, enabling flexible analytics, report generation, and integration with external systems.

Virtually any controller functionality is implemented by combining ready-made MWOS modules and configuring their logical interactions through a system of parameters and events — without the need to write low-level code. For developers, it's enough to create a new module by inheriting from the base class to add fundamentally new capabilities to the system, which immediately become available for remote configuration and use. Thanks to this architecture, any microcontroller-related task — from a simple sensor to a distributed smart home system or industrial automation — is solved faster, more reliably, and with minimal maintenance costs.

| Feature | Description |
|---------|-------------|
| 🔌 Modularity | Each device is controlled by a separate software module |
| 🎛 Parameters | Flexible module configuration: settings, control, monitoring |
| 🖥 Widgets | Parameters are automatically displayed as widgets in the web interface |
| 🌐 Universal Protocol | Binary protocol works over WiFi, LAN, USB, RS232/485, CAN, radio channel |
| ⚙️ MWScript | Proprietary scripting language for complex scenarios on server and controller |
| 🔄 Cross-platform | Server part in .NET runs on Windows and Linux. mwosApp runs on Windows, Linux, and Android |

## 🧩 MWOS Modules
![Modules](https://img.shields.io/badge/📚-MWOS_Modules-4A90E2?style=for-the-badge&logo=github&logoColor=white)

Documentation for all MWOS framework modules for microcontrollers

[📖 Open Full Documentation →](src/README.md)

## 🎯 Getting Started

### Connecting an Existing Controller
1. Go to the [MWOS Personal Dashboard](https://iot.voscom.online/)
2. Power on the controller with MWOS firmware
3. The controller will appear in the list automatically (if it's already registered to you)

**If the controller doesn't appear:**

📱 **Option A: QR Code**
- Select "Add controller via QR code"
- Scan the code displayed on the controller screen

⌨️ **Option B: Manual Entry**
- Enter the controller code manually

🤝 **Option C: Request Access**
- If the controller belongs to another administrator — specify their Email or code
- The administrator will receive an access rights request

✅ Once the controller appears in your personal dashboard, you can immediately manage it and connected devices.

*Administrators can delegate management rights to other users.*

## 🛠 Creating Firmware

### 1. Hardware Preparation
- Select a microcontroller from the [list of supported platforms](#-supporting-new-platforms)
- Ensure there's an interface for network connection: `WiFi`, `LAN`, `USB`, `RS232`, `RS485`, `CAN`, etc.

### 2. Firmware Generation in Personal Dashboard
In the "Create Controller Firmware" section, fill out the form:
- [ ] Select the microcontroller model
- [ ] Specify the network interface type
- [ ] Check the software device modules to include in the firmware
- [ ] Click "Download BIN firmware" (ready to flash) or "Download source code" (for manual compilation)

### 3. Flashing to Device
Write the obtained `.bin` file to the microcontroller using Arduino IDE, esptool, PlatformIO, or another utility.

🎉 **After first boot**, the controller will automatically appear in your personal dashboard, and you will become its administrator.

## 💻 Developing Your Own Modules
We continuously expand our library of verified modules, but you can easily create your own.

**Why is it simple?**

✅ **No need to write code for:**
- Wi-Fi / Ethernet connection
- Web server and HTTP handling
- Settings storage in EEPROM/Flash
- Synchronization with cloud server

✅ **You write only:**
- Logic for interacting with your device
- Parameter descriptions (settings, readings)
- (Optional) Widget appearance

## 🔧 Supporting New Platforms
MWOS is written in C++11 and compatible with any microcontroller supporting the Arduino Framework.

⚠️ **Important:** Arduino Framework ≠ Arduino boards.

The Framework is a set of libraries and configuration rules that works on hundreds of platforms: from ATtiny to ESP32, Raspberry Pi Pico, and even Linux.

**If your platform isn't listed:**
1. Check if Arduino Framework support exists for your MCU
2. If not — you can add a port yourself (source code is open)
3. Or use an alternative framework with minimal modifications

🌟 **Bonus:** By adding support for a new platform, you gain the ability to compile thousands of ready-made sketches from the Arduino ecosystem for it, plus support from an active community.

## ⚙️ Manual Compilation
For advanced users and custom builds:

```bash
git clone https://github.com/voscom/mwos.git
cd mwos
# See detailed instructions:
open examples/README.md
```

In the `examples` folder you'll find:
- Build examples for different platforms
- Toolchain setup instructions
- Templates for adding new modules

## 🔬 Technical Features of MWOS3

| Feature | Description |
|---------|-------------|
| 🌐 Remote Configuration | All module settings are accessible and editable via web interface |
| 🔌 Universal Protocol | Binary protocol abstracts the physical layer: WiFi, RS485, CAN, radio channel, etc. |
| 🖥 Server Part | Cross-platform (.NET), scalable to thousands of controllers and users |
| ⚙️ MWScript | Proprietary scripting language compiled to bytecode for server and controller |
| 🔧 Macro Substitutions | Advanced preprocessor directive capabilities allow adapting MWScript syntax to C++, Basic, G-code, etc. |
| 🎯 Bytecode Interpreter | Enables running complex scenarios: from sensor automation to machine control via G-codes and M-codes |

## 🤝 Contributing
We welcome new modules, bug fixes, and ports for new platforms!

1. Fork the repository
2. Create a branch: `git checkout -b feature/MyNewModule`
3. Make changes and test them
4. Submit a Pull Request with a description of changes

📖 Before starting, please review the [Developer Guidelines](./CONTRIBUTING.md).

## 📄 License
This project is distributed under the MIT License.

Details in the [LICENSE](LICENSE_RU) file.

## 📞 Contacts
- 🌐 Management Portal: https://iot.voscom.online/
- 📧 Email: support@voscom.online

⭐ **Liked the project?** Give us a star on GitHub — it helps MWOS grow! 🚀
