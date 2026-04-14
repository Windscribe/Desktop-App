# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

**Windscribe Desktop VPN** - A production-grade, cross-platform VPN client for Windows, macOS, and Linux written in C++17 with Qt.

### What It Does

This application provides secure, private internet access by routing your traffic through encrypted VPN tunnels. It protects your physical location, blocks ads and trackers, prevents DNS leaks, and helps bypass censorship through multiple connection strategies.

**Current Version**: 2.21.1 (guinea pig build - see `src/client/client-common/version/windscribe_version.h`)

**Supported Platforms**:
- Windows 10/11 (amd64, arm64), Windows Server 2022
- macOS 13+ (amd64, arm64)
- Ubuntu 22.04+, Fedora 36+, openSUSE Leap 15.6, Arch, immutable distros (amd64, arm64)

### Key Features

- **Multiple VPN Protocols**: WireGuard (with Post-Quantum encryption), OpenVPN (UDP/TCP), IKEv2, Stealth, WSTunnel - over 20 connection ports
- **Privacy & Security**: Always-on firewall eliminates leaks, MAC address spoofing, DNS-based content filtering (R.O.B.E.R.T.), custom DNS support
- **Advanced Networking**: Network-wide and per-app split tunneling, create HTTP/SOCKS5 proxy servers, WiFi sharing (ICS/NAT)
- **Interfaces**: Full-featured Qt Widgets GUI and comprehensive CLI (pure CLI mode available on Linux)
- **Anti-Censorship**: Built-in failover with 16+ strategies including DoH, ECH, CDN fronting for bypassing censorship
- **Custom Configs**: Import any OpenVPN or WireGuard configuration
- **Network Rules**: Auto-connect/disconnect based on network conditions
- **No Limits**: No per-device or simultaneous connection restrictions

## Folder Structure

### Root Level
```
client-desktop/
├── src/                    # All source code
├── libs/                   # External/internal libraries (wsnet, wssecure)
├── tools/                  # Build scripts and utilities
├── cmake/                  # CMake configuration files
├── data/                   # Application resources and test data
├── build/                  # Build output (created during build)
├── build-libs/            # Compiled dependencies (WireGuard, WSTunnel, etc.)
├── build-exe/             # Final installers/packages (created during build)
└── CMakeLists.txt         # Root build configuration
```

### Source Code Structure (`src/`)

#### Main Application (`src/client/`)
- **`client-common/`** - Shared code between GUI and CLI
  - `ipc/` - Inter-process communication (QLocalSocket-based)
  - `types/` - Common types (Protocol, Location, ConnectionSettings, etc.)
  - `api_responses/` - API response parsing
  - `utils/` - Utilities (logging, executable signatures, etc.)
  - `version/` - Version definitions (`windscribe_version.h`)

- **`frontend/`** - User-facing components
  - `frontend-common/` - Shared frontend code
    - `backend/` - Backend coordinator (UI ↔ Engine bridge)
    - `locations/` - Location management
    - `localipcserver/` - IPC server for CLI communication
  - `gui/` - Qt-based GUI application (NOT QML - uses traditional Qt widgets)
    - `application/` - Main window and app entry point
    - `connectwindow/` - Connection interface
    - `locationswindow/` - Server location picker
    - `loginwindow/` - Login/authentication UI
    - `preferenceswindow/` - Settings/preferences UI
    - `systemtray/` - System tray integration
    - `commonwidgets/` - Reusable UI components
    - `tests/` - GUI component tests
  - `cli/` - CLI-specific frontend code

- **`engine/`** - Core VPN engine (runs in separate QThread)
  - `engine/engine/` - Main engine implementation
    - `connectionmanager/` - VPN protocol management
      - `openvpnconnection.*` - OpenVPN implementation
      - `wireguardconnection_*` - WireGuard (platform-specific)
      - `ikev2connection_*` - IKEv2 (platform-specific)
      - `ctrldmanager/` - Control-D DNS integration
    - `firewall/` - Firewall control (platform-specific)
    - `helper/` - Helper process communication
    - `apiresources/` - Server list, locations model
    - `ping/` - Ping/latency measurement
    - `proxy/` - HTTP/SOCKS5 proxy server
    - `vpnshare/` - WiFi sharing features
    - `customconfigs/` - Custom VPN config import
    - `emergencycontroller/` - Fallback emergency connection
    - `networkdetectionmanager/` - Network change detection
    - `macaddresscontroller/` - MAC spoofing
    - `splittunnelextension/` - Split tunneling coordination

#### Helper Process (`src/helper/`)
Privileged helper process that runs with elevated permissions:
- **`common/`** - Shared helper code
  - `helper_commands.h` - 100+ command definitions
- **`windows/`** - Windows Service implementation
- **`macos/`** - macOS LaunchDaemon implementation
- **`linux/`** - Linux systemd service implementation

#### CLI Application (`src/windscribe-cli/`)
Standalone CLI that can communicate with GUI or run independently:
- `backendcommander.h` - CLI → GUI IPC communication
- `translations/` - CLI translation files

#### Platform Utilities (`src/utils/`)
- **`windows/`** - Windows-specific utilities
  - `windscribe_install_helper/` - Installation helper service
  - `wireguard_service/` - WireGuard service management
- **`macos/`** - macOS utilities
  - `launcher/` - Launch helper
- **`linux/`** - Linux utilities
  - `authhelper/` - PAM authentication helper

#### Split Tunneling (`src/splittunneling/`)
- **`windows/`** - WMI filter driver
- **`macos/`** - Network Extension

#### Installers (`src/installer/`)
- **`windows/`** - Windows installer/uninstaller/bootstrap
- **`mac/`** - macOS DMG installer
- **`linux/`** - Linux package scripts

### Libraries (`libs/`)

#### wsnet (`libs/wsnet/`)
Windscribe's custom networking library with anti-censorship capabilities:
- `src/api/` - API interfaces (ServerAPI, BridgeAPI)
- `src/dnsresolver/` - Custom DNS resolver
- `src/httpnetworkmanager/` - HTTP client with TLS modifications
- `src/failover/` - Anti-censorship failover strategies
- `src/pingmanager/` - Ping implementation
- `src/private/tests/` - wsnet unit tests (Google Test)

### Tools (`tools/`)
- **`build_all.py`** - Main Python build orchestrator
- **`base/`** - Python build utilities
- **`deps/`** - Dependency installation scripts
  - `install_wireguard` - WireGuard library installer
  - `install_wstunnel` - WSTunnel binary installer
  - `install_openvpn_dco` - OpenVPN DCO driver (Windows)
  - `install_wintun` - Wintun driver (Windows)
- **`vcpkg/`** - vcpkg package manager configuration
  - `install_ci/` - vcpkg installation scripts
  - `ports/` - Custom vcpkg ports
- **`translations/`** - Translation management tools

### Platform-Specific Naming Pattern
Most components have separate implementations:
- `*_win.h/cpp` - Windows
- `*_mac.h/cpp` - macOS
- `*_linux.h/cpp` - Linux
- `*_posix.h/cpp` - macOS + Linux shared code

## Technologies Used

### Core Languages
- **C++17** - Primary language (MSVC on Windows, Clang on macOS, GCC/Clang on Linux)
- **Python 3.6.8+** - Build scripts (3.11+ recommended)
- **Go 1.23+** - For building WSTunnel dependency
- **Shell Scripts** - Bash (macOS/Linux), PowerShell (Windows)

### GUI Framework
- **Qt** - Cross-platform GUI framework (traditional widgets, NOT QML)
  - Qt Widgets for all UI components
  - Qt Network (QLocalSocket for IPC)
  - Qt Core (threading, signals/slots, event loop)

### Build System
- **CMake 3.28+** - Primary build system (100+ CMake projects)
- **vcpkg** - C++ package manager
- **Ninja** - Build backend (Windows)
- **Visual Studio 2022** - Windows compiler
- **Xcode 15+** - macOS compiler
- **GCC** - Linux compiler

### VPN Protocols & Implementations

**WireGuard**:
- Wintun driver (Windows userspace)
- wireguard-nt (Windows native driver)
- WireGuard kernel module (Linux)
- WireGuard-go (macOS userspace)

**OpenVPN**:
- OpenVPN 2.x process
- OpenVPN DCO (Data Channel Offload) driver for Windows

**IKEv2**:
- WAN API (Windows native IPSec)
- NEVPNManager (macOS Network Extension VPN)
- strongSwan (Linux IPSec)

**Obfuscation**:
- Stunnel (TLS tunnel wrapper)
- WSTunnel (custom WebSocket tunnel in Go)

### Core C++ Libraries

**Boost Libraries**:
- Boost.Archive - Binary serialization for Helper IPC
- Boost.ASIO - Asynchronous I/O for wsnet
- Boost.Filesystem - File operations
- Boost.Serialization - Object serialization

**Other Libraries**:
- spdlog - High-performance logging
- nlohmann/json - JSON parsing
- Google Test (gtest) - Unit testing framework
- CMakeRC - Resource embedding
- OpenSSL - TLS/SSL with custom ECH (Encrypted Client Hello) patches
- libcurl - HTTP client with custom ECH patches
- c-ares - Asynchronous DNS resolution
- Protocol Buffers (protobuf) - Binary serialization
- zlib, lz4 - Compression

### Platform-Specific Technologies

**Windows**:
- Windows API (Win32)
- Windows Firewall API
- WMI (Windows Management Instrumentation) - Split tunneling
- Named Pipes - Service communication
- Windows Services - Helper runs as NT SERVICE
- ICS (Internet Connection Sharing) - WiFi sharing
- Registry API - Settings and configuration
- BFE (Base Filtering Engine) - Network filtering
- RAS (Remote Access Service) - VPN management
- Authenticode - Code signing with PFX certificates

**macOS**:
- Cocoa/Foundation frameworks
- NEVPNManager - VPN management (requires entitlement)
- NENetworkExtension - Split tunneling
- LaunchDaemons/LaunchAgents - Background services
- XPC - Inter-process communication
- pfctl - Packet Filter firewall
- IOKit - Network adapter management
- Keychain Services - Credential storage
- System Configuration Framework
- create-dmg - DMG installer creation
- Apple Developer ID signing and notarization

**Linux**:
- systemd - Service management
- iptables/nftables - Firewall rules
- netfilter - Packet filtering
- NetworkManager/systemd-resolved - DNS
- PAM (Pluggable Authentication Modules)
- D-Bus - Inter-process communication
- libnl (netlink) - Network configuration
- libcap-ng - Capability management
- X11/Wayland - Display server protocols
- dpkg/RPM - Package formats (.deb, .rpm, .pkg.tar.zst)

### Development & Testing Tools
- clang-tidy - Static analysis
- flake8 - Python linting
- pproxy - Python proxy server for network tests
- GitLab CI - Continuous integration
- JUnit XML - Test report format

### Network & Security Technologies
- TLS 1.3 with Encrypted Client Hello (ECH)
- Post-Quantum Cryptography (WireGuard enhancement)
- DNS-over-HTTPS (DoH) - Cloudflare, Google
- Certificate pinning for API security
- CDN fronting - Anti-censorship technique
- SNI spoofing - TLS privacy

## Build Commands

### Prerequisites

Install vcpkg dependencies:
- **Windows**: `tools\vcpkg\install_ci\vcpkg_install.bat`
- **macOS/Linux**: `tools/vcpkg/install_ci/vcpkg_install.sh`

Install Python build dependencies:
```bash
python3 -m pip install -r tools/requirements.txt
```

### Building Libraries (Required First)

Navigate to `tools/deps/` and run installation scripts **in order**:

**Windows**:
```bash
install_openvpn_dco [--arm64]
install_wintun [--arm64]
install_wireguard [--arm64]
install_wstunnel [--arm64]
```

**macOS/Linux**:
```bash
./install_wireguard
./install_wstunnel
```

Libraries are placed in `build-libs/` (or `build-libs-arm64/` on Windows ARM64).

### Building the Application

From the `tools/` directory:

```bash
# Standard build (GUI + installer)
./build_all

# Debug build
./build_all --debug

# Build app only (no installer)
./build_all --build-app

# Build with tests
./build_all --build-tests

# Signed build (requires certificate setup)
./build_all --sign --use-local-secrets

# Windows ARM64 build
./build_all --arm64

# Linux CLI-only build
./build_all --build-cli-only

# Static analysis (clang-tidy)
./build_all --static-analysis

# See all options
./build_all --help
```

**Output**: Installers are placed in `build-exe/`.

### Running Tests

Tests are built with `--build-tests` flag and placed in `build/test/`:

**Windows**:
```powershell
cd build/test
pip3 install pproxy
pproxy  # Run proxy server in background
./wsnet_test.exe --gtest_output=xml:report_win.xml
```

**macOS/Linux**:
```bash
cd build/test
pip3 install pproxy
pproxy &  # Run proxy server in background
./wsnet_test --gtest_output=xml:report.xml
```

Tests use Google Test framework and output JUnit XML reports.

### Linting

```bash
# Python linting (from root)
python3 -m flake8 --max-line-length=120 tools/

# Translation checks
python3 tools/translations/check_translations.py
```

## High-Level Architecture

### Component Overview

The application follows a **layered architecture with privilege separation**. The codebase contains 100+ CMake projects organized into distinct layers:

```
┌──────────────────────────────────────────────────────────┐
│ FRONTEND LAYER (src/client/frontend/)                    │
│  ┌─────────────────────────────────────────────────┐    │
│  │ GUI (Qt Widgets) │ CLI (windscribe-cli)         │    │
│  │ • MainWindow     │ • BackendCommander           │    │
│  │ • PreferencesUI  │ • IPC to GUI or standalone   │    │
│  │ • Dialogs        │ • Terminal interface         │    │
│  └─────────────────────────────────────────────────┘    │
└────────────────┬─────────────────────────────────────────┘
                 │ IPC via QLocalSocket (GUI ↔ CLI)
┌────────────────┴─────────────────────────────────────────┐
│ BACKEND LAYER (src/client/frontend/frontend-common/)     │
│  • Central coordination between Frontend and Engine      │
│  • Manages persistent state and preferences              │
│  • Handles wsnet integration for API calls               │
│  • LocationsModel management                             │
│  • LocalIPCServer for CLI connections                    │
└────────────────┬─────────────────────────────────────────┘
                 │ Direct Qt signals/slots (same process, different thread)
┌────────────────┴─────────────────────────────────────────┐
│ ENGINE LAYER (src/client/engine/)                        │
│  • Runs in dedicated QThread                             │
│  • ConnectionManager: VPN protocol implementations       │
│  • FirewallController: OS-specific firewall rules        │
│  • ApiResources: Server list, session status             │
│  • PingManager: Server latency measurement               │
│  • ProxyShare: HTTP/SOCKS5 server                        │
│  • VpnShare: WiFi sharing (ICS/NAT)                      │
│  • SplitTunneling: Per-app/network routing               │
│  • EmergencyController: Hardcoded fallback endpoints     │
│  • MacAddressController: MAC spoofing                    │
└────────────────┬─────────────────────────────────────────┘
                 │ Binary serialization via Boost.Archive
┌────────────────┴─────────────────────────────────────────┐
│ HELPER LAYER (src/helper/) - SEPARATE PROCESS           │
│  • Elevated/daemon process with root/SYSTEM privileges   │
│  • Platform implementations:                             │
│    - helper_win.cpp: Windows Service (Named Pipes)       │
│    - helper_mac.cpp: LaunchDaemon (Unix sockets)         │
│    - helper_linux.cpp: systemd service (Unix sockets)    │
│  • Executes 100+ privileged commands:                    │
│    - Firewall manipulation                               │
│    - VPN tunnel start/stop                               │
│    - DNS configuration                                   │
│    - Routing table modifications                         │
│    - Network adapter control                             │
└────────────────┬─────────────────────────────────────────┘
                 │ System calls
┌────────────────┴─────────────────────────────────────────┐
│ OS LAYER (Operating System & Kernel)                     │
│  • WireGuard: Wintun/wireguard-nt/kernel module          │
│  • OpenVPN: External process spawned by helper           │
│  • IKEv2: NEVPNManager/WAN API/strongSwan                │
│  • Firewall: Windows Firewall/pfctl/iptables/nftables    │
│  • DNS: System resolvers, hosts file                     │
│  • Routing: Kernel routing tables                        │
└──────────────────────────────────────────────────────────┘
```

**Key Architectural Notes**:
- The GUI uses **Qt Widgets** (NOT QML) - traditional widget-based UI
- Engine runs in a separate QThread but same process as Frontend
- Helper is a completely separate process with elevated privileges
- Communication: Frontend ↔ Engine (Qt signals/slots), Engine ↔ Helper (binary serialization)
- wsnet library integrates at the Backend and Engine level for API communication

### Key Subsystems

**IPC Communication** (`src/client/client-common/ipc/`):
- QLocalSocket-based bidirectional communication
- `Connection` class manages client-server connections
- `Command` base class with binary serialization
- `BackendCommander` for CLI → GUI communication (10s timeout)

**Protocol Implementations** (`src/client/engine/connectionmanager/`):
- `OpenVPNConnection`: Manages OpenVPN process lifecycle, config generation
- `WireGuardConnection_win/posix`: Kernel driver integration, handshake monitoring
- `IKEv2Connection_win/mac/linux`: Platform-native IPSec implementations
- Obfuscation: Stunnel (TLS wrapper), WSTunnel (WebSocket tunnel)

**Helper Communication** (`src/helper/common/helper_commands.h`):
- 100+ commands for privileged operations
- Binary serialization over platform-specific channels
- Platform-specific implementations: `Helper_win`, `Helper_mac`, `Helper_linux`
- Commands include: firewall control, DNS config, VPN protocol start/stop, split tunneling

**wsnet Library** (`libs/wsnet/`):
- Cross-platform API access with anti-censorship bypass
- Built-in failover mechanisms (16+ strategies)
- ServerAPI interface for Windscribe backend
- DNS resolver with custom server support
- HTTP manager with TLS modifications (SNI spoofing, ECH)
- Integration: `Backend` → `Engine` → `wsnet::WSNet::instance()`

### Platform-Specific Code

**Windows** (`src/helper/windows/`, `src/client/engine/`):
- Service architecture (Named Pipes communication)
- Windows Firewall API integration
- Split tunneling via WMI filter driver
- IKEv2 via WAN API
- WireGuard via Wintun driver
- Code signing verification

**macOS** (`src/helper/macos/`, `src/client/engine/mac/`):
- LaunchDaemon helper process
- pfctl-based firewall (Packet Filter)
- Split tunneling via NENetworkExtension
- IKEv2 via NEVPNManager (requires `com.apple.developer.networking.vpn.api` entitlement)
- Installation helpers for daemon updates
- **Note**: IKEv2 only works in Windscribe-signed builds with embedded provisioning profile

**Linux** (`src/helper/linux/`, `src/client/engine/`):
- systemd service architecture
- iptables/nftables firewall rules
- WireGuard via kernel module
- IKEv2 via strongSwan
- Split tunneling via routing rules
- Optional CLI-only mode (`BUILD_CLI_ONLY`)

### Threading Model

- **Main GUI Thread**: Qt event loop
- **Engine Thread**: Separate `QThread` for all engine operations
- **Helper Process**: Separate OS process with elevated privileges
- **wsnet**: Internal thread pool via Boost.ASIO

Communication uses Qt signals/slots for thread-safe async operations.

## Important File Locations

**Version Management**:
- `src/client/client-common/version/windscribe_version.h` - Version numbers and build type (beta/guinea_pig/stable)

**Protocol Types**:
- `src/client/client-common/types/protocol.h` - Protocol enum definitions

**Helper Commands**:
- `src/helper/common/helper_commands.h` - All privileged operation command definitions

**Main Components**:
- `src/client/engine/engine/engine.h` - Central engine orchestration
- `src/client/frontend/frontend-common/backend/backend.h` - Frontend ↔ Engine coordinator
- `src/client/frontend/gui/application/mainwindow.h` - Main GUI window
- `src/windscribe-cli/backendcommander.h` - CLI entry point

**Build System**:
- `CMakeLists.txt` - Root CMake configuration
- `tools/build_all.py` - Main build script
- `cmake/architectures.cmake` - Platform/architecture detection
- `cmake/signing.cmake` - Code signing configuration

## Development Workflow

### Making Changes to Helper

If modifying helper code on macOS (`src/helper/macos`), you **must** increment `CFBundleVersion` in `src/helper/macos/helper-info.plist`. The installer only updates the helper when this version changes.

### Code Signing

**Windows** (optional):
1. Place PFX certificate: `installer/windows/signing/code_signing.pfx`
2. Create `tools/notarize.yml` with password:
   ```yaml
   windows-signing-cert-password: your-password
   ```
3. Update `client/common/utils/executable_signature/executable_signature_defs.h` with certificate subject name

**macOS** (required):
1. Install Developer ID Application certificate in Keychain
2. Update `client/common/utils/executable_signature/executable_signature_defs.h` with `MACOS_CERT_DEVELOPER_ID`

### Adding New Protocol Support

1. Create connection class in `src/client/engine/connectionmanager/` inheriting from `IConnection`
2. Add protocol enum to `src/client/client-common/types/protocol.h`
3. Add helper commands to `src/helper/common/helper_commands.h` for privileged operations
4. Implement platform-specific helper handlers in `src/helper/[platform]/`
5. Update `ConnectionManager` to instantiate new connection type
6. Add UI elements in `src/client/frontend/gui/protocolwindow/`

### Split Tunneling Implementation

- **Windows**: WMI filter driver (`src/splittunneling/windows/`)
- **macOS**: Network Extension (`src/splittunneling/macos/`)
- **Linux**: Routing rules via helper

Configuration managed through helper commands and engine settings.

## Log File Locations

**Windows**:
- Client/installer helper: `C:\Users\<user>\AppData\Local\Windscribe\Windscribe2`
- Service: `C:\Program Files\Windscribe\windscribeservice.log`
- Installer: `C:\Program Files\Windscribe\log_installer.txt`

**macOS**:
- Client: `~/Library/Application Support/Windscribe/Windscribe2`
- Helper: `/Library/Logs/com.windscribe.helper.macos/helper_log.txt`
- Installer: `~/Library/Application Support/Windscribe/Windscribe/log_installer.txt`

**Linux**:
- Client: `~/.local/share/Windscribe/Windscribe2`
- Helper: `/var/log/windscribe/helper_log.txt`

## Testing

**Test Locations**:
- `libs/wsnet/src/private/tests/` - wsnet library tests (Google Test)
- `src/client/engine/engine/ping/tests/` - Ping subsystem tests
- `src/client/engine/engine/apiresources/tests/` - API resources tests
- `src/client/frontend/gui/tests/` - GUI component tests
- `data/tests/locationsmodel/` - LocationsModel JSON test data

**Running Specific Tests**:
```bash
# Build tests
cd tools
./build_all --build-tests

# Run wsnet tests with proxy
cd ../build/test
pip3 install pproxy
pproxy &
./wsnet_test --gtest_filter="TestName.*"
```

## Common Pitfalls

1. **Missing vcpkg dependencies**: Run vcpkg install scripts before building
2. **Helper not updating on macOS**: Increment `CFBundleVersion` in helper-info.plist
3. **IKEv2 not working**: Requires proper entitlements and provisioning profile (Windscribe-signed builds only on macOS)
4. **CMake cache issues**: Delete `build/CMakeCache.txt` and `build/CMakeFiles/` if experiencing configure errors
5. **Code signing failures**: Ensure certificates are properly installed and paths in `executable_signature_defs.h` are correct
6. **Test failures**: Ensure `pproxy` is running before executing wsnet tests

## Contributing

This is a mirror of Windscribe's internal repository. Before submitting PRs, discuss changes via GitHub issues to ensure alignment with internal development and avoid duplicate work. See `CONTRIBUTING.md` for details.
