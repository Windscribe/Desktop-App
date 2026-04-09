# Installation

If Windscribe is not already installed, download and install it first.

## Download Sources (in order of preference)

1. **Windscribe Download Page (recommended):** https://windscribe.com/download
   - Direct install links per platform (always latest stable release)
   - **Windows:** https://windscribe.com/install/desktop/windows
   - **macOS (Universal):** https://windscribe.com/install/desktop/osx
   - **Linux (Desktop App):**
     - Debian/Ubuntu (AMD64): https://windscribe.com/install/desktop/linux_deb_x64
     - Debian/Ubuntu (ARM64): https://windscribe.com/install/desktop/linux_deb_arm64
     - Fedora/CentOS (AMD64): https://windscribe.com/install/desktop/linux_rpm_x64
     - Arch (AMD64): https://windscribe.com/install/desktop/linux_zst_x64
     - openSUSE (AMD64): https://windscribe.com/install/desktop/linux_rpm_opensuse_x64
   - **Linux (CLI-only):**
     - Debian/Ubuntu (AMD64): https://windscribe.com/install/desktop/linux_deb_x64_cli
     - Debian/Ubuntu (ARM64): https://windscribe.com/install/desktop/linux_deb_arm64_cli
     - Fedora/CentOS (AMD64): https://windscribe.com/install/desktop/linux_rpm_x64_cli
     - Arch (AMD64): https://windscribe.com/install/desktop/linux_zst_x64_cli
     - openSUSE (AMD64): https://windscribe.com/install/desktop/linux_rpm_opensuse_x64_cli

2. **Windscribe Changelog:** https://windscribe.com/changelog
   - Offers **Guinea Pig** (early testing), **Beta**, and **Release** (stable) builds
   - Platform-specific pages: [Windows](https://windscribe.com/changelog/windows) · [macOS](https://windscribe.com/changelog/mac)
   - Linux changelog pages:
     - GUI: [deb x64](https://windscribe.com/changelog/linux_deb_x64) · [deb arm64](https://windscribe.com/changelog/linux_deb_arm64) · [rpm x64](https://windscribe.com/changelog/linux_rpm_x64) · [rpm openSUSE x64](https://windscribe.com/changelog/linux_rpm_opensuse_x64) · [zst x64](https://windscribe.com/changelog/linux_zst_x64)
     - CLI: [deb x64](https://windscribe.com/changelog/linux_deb_x64_cli) · [deb arm64](https://windscribe.com/changelog/linux_deb_arm64_cli) · [rpm x64](https://windscribe.com/changelog/linux_rpm_x64_cli) · [rpm openSUSE x64](https://windscribe.com/changelog/linux_rpm_opensuse_x64_cli) · [zst x64](https://windscribe.com/changelog/linux_zst_x64_cli)

3. **GitHub Releases:** https://github.com/Windscribe/Desktop-App/releases
   - Open-source release artifacts

## Linux

```bash
# Option A: Direct download from windscribe.com (recommended — always latest stable)
# Desktop App:
curl -LO "https://windscribe.com/install/desktop/linux_deb_x64"      # Debian/Ubuntu AMD64
curl -LO "https://windscribe.com/install/desktop/linux_deb_arm64"     # Debian/Ubuntu ARM64
curl -LO "https://windscribe.com/install/desktop/linux_rpm_x64"       # Fedora/CentOS AMD64
curl -LO "https://windscribe.com/install/desktop/linux_zst_x64"       # Arch AMD64
curl -LO "https://windscribe.com/install/desktop/linux_rpm_opensuse_x64"  # openSUSE AMD64

# CLI-only (headless/server — no GUI needed):
curl -LO "https://windscribe.com/install/desktop/linux_deb_x64_cli"   # Debian/Ubuntu AMD64
curl -LO "https://windscribe.com/install/desktop/linux_deb_arm64_cli"  # Debian/Ubuntu ARM64
curl -LO "https://windscribe.com/install/desktop/linux_rpm_x64_cli"   # Fedora/CentOS AMD64
curl -LO "https://windscribe.com/install/desktop/linux_zst_x64_cli"   # Arch AMD64
curl -LO "https://windscribe.com/install/desktop/linux_rpm_opensuse_x64_cli"  # openSUSE AMD64

# Install .deb (Debian/Ubuntu):
sudo apt install ./windscribe_*.deb

# Install .rpm (Fedora/CentOS):
sudo dnf install ./windscribe_*.rpm

# Install .zst (Arch):
sudo pacman -U windscribe_*.pkg.tar.zst

# Install .rpm (openSUSE):
sudo zypper install ./windscribe_*.rpm

# Option B: From GitHub Releases
# Filenames include the version number, e.g.:
#   windscribe_2.21.6_beta_amd64.deb
#   windscribe_2.21.5_guinea_pig_amd64.deb
# Browse https://github.com/Windscribe/Desktop-App/releases for the latest.
```

> **Note:** GitHub release filenames always include the version number — there is no static "latest" URL. Check the [releases page](https://github.com/Windscribe/Desktop-App/releases) for the correct filename.

## macOS

Download the `.dmg` from [windscribe.com](https://windscribe.com/install/desktop/osx) (Universal) or the [changelog](https://windscribe.com/changelog/mac) and install normally. The CLI is bundled with the desktop app.

## Windows

Download the `.exe` installer from [windscribe.com/download](https://windscribe.com/download) or the [changelog](https://windscribe.com/changelog/windows) and run it. The CLI is bundled with the desktop app.

> ⚠️ **Windows limitation in sandboxed AI environments:** Some AI agent sandboxes run Linux containers, so they cannot execute `windscribe-cli.exe` directly. On Windows, the agent should **provide commands for the user to run** in PowerShell/CMD rather than attempting to execute them. On Linux and macOS, the CLI can be invoked directly if installed on the host.

## Verify Installation

```bash
windscribe-cli status
```

If the command is not found, check the binary locations in the main [SKILL.md](../SKILL.md).
