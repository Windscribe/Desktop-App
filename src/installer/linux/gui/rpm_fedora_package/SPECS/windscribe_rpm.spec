# make sure rpmbuild does not attempt to strip our binaries, which sometimes causes corruption
%define debug_package %{nil}
%define __strip /bin/true

%global __provides_exclude_from .*
%global __requires_exclude_from .*

Name:		windscribe
Version:	2.6.0
Release:	0
Summary:	Windscribe Client

Group:		Applications/Internet
License:	GPLv2
URL:		https://www.windscribe.com
Vendor:		Windscribe Limited
Source0:	windscribe.tar
Conflicts:	windscribe-cli

Requires:	bash
Requires:	iptables
Requires:	glibc >= 2.35
Requires:	libstdc++
Requires:	glib2
Requires:	zlib
Requires:	libglvnd-egl
Requires:	libglvnd-glx
Requires:	libglvnd-opengl
Requires:	libX11
Requires:	libxkbcommon
Requires:	dbus-libs
Requires:	fontconfig
Requires:	freetype
Requires:	systemd-libs
Requires:	libxcb
Requires:	xcb-util-wm
Requires:	xcb-util-image
Requires:	xcb-util-keysyms
Requires:	xcb-util-renderutil
Requires:	xcb-util-cursor
Requires:	sudo
Requires:	shadow-utils
Requires:	procps-ng
Requires:	polkit
Requires:	iproute
Requires:	iputils
Requires:	psmisc
Requires:	libxkbcommon-x11
Requires:	coreutils
Requires:	desktop-file-utils

%description
Windscribe client.

%prep

%build

%install
rm -rf %{buildroot}
mkdir -p %{buildroot}
mv -f %{_builddir}/* %{buildroot}

%posttrans
systemctl daemon-reload || true
systemctl preset windscribe-helper || true
systemctl restart windscribe-helper || true

%post
ln -sf /opt/windscribe/windscribe-cli /usr/bin/windscribe-cli
update-desktop-database
setcap cap_setgid+ep /opt/windscribe/Windscribe
mkdir -p /etc/windscribe
if [ "`uname -m`" = "aarch64" ]; then
    echo linux_rpm_arm64 > /etc/windscribe/platform
else
    echo linux_rpm_x64 > /etc/windscribe/platform
fi

%preun
if [ $1 -eq 0 ]; then
    /opt/windscribe/helper --reset-mac-addresses
    systemctl disable windscribe-helper || true
fi

%postun
if [ $1 -eq 0 ]; then
    killall -q Windscribe || true
    systemctl stop windscribe-helper || true
    userdel -f windscribe || true
    groupdel -f windscribe || true
    rm -f /usr/bin/windscribe-cli
    rm -rf /etc/windscribe
    rm -rf /opt/windscribe
    rm -rf /var/log/windscribe
else
    rm -rf /etc/windscribe/stunnel.conf
    rm -rf /etc/windscribe/config.ovpn
    mkdir -p /var/tmp/windscribe
    if [ -d "/etc/windscribe/windscribe_servers" ]; then
        mv -f /etc/windscribe/windscribe_servers /var/tmp/windscribe
    fi
fi

%files
%config /etc/windscribe/autostart/windscribe.desktop
/opt/windscribe/*
/usr/polkit-1/actions/com.windscribe.authhelper.policy
/usr/lib/systemd/system-preset/69-windscribe-helper.preset
/usr/lib/systemd/system/windscribe-helper.service
/usr/share/applications/windscribe.desktop
/usr/share/icons/hicolor/*/apps/Windscribe.png
