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
BuildArch:	x86_64
Source0:	windscribe.tar

Requires:	bash
Requires:	iptables
Requires:	glibc >= 2.28
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
Requires:	net-tools
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

%description
Windscribe client.

%prep

%build

%install
mv -f %{_sourcedir}/* %{buildroot}

%posttrans
systemctl enable windscribe-helper
systemctl start windscribe-helper

%post
ln -sf /opt/windscribe/windscribe-cli /usr/bin/windscribe-cli
update-desktop-database
echo linux_rpm_x64 > ../etc/windscribe/platform

%postun
if [ $1 -eq 0 ]; then
    killall -q Windscribe || true
    systemctl stop windscribe-helper || true
    systemctl disable windscribe-helper || true
    userdel -f windscribe || true
    groupdel -f windscribe || true
    rm -f /usr/bin/windscribe-cli
    rm -rf /etc/windscribe/rules.*
    rm -rf /etc/windscribe/*.ovpn
    rm -rf /etc/windscribe/stunnel.conf
    rm -f /opt/windscribe/helper_log.txt
fi

%files
%config /etc/systemd/system-preset/50-windscribe-helper.preset
%config /etc/systemd/system/windscribe-helper.service
%config /etc/windscribe/*
/opt/windscribe/*
/usr/polkit-1/actions/com.windscribe.authhelper.policy
/usr/share/applications/windscribe.desktop
/usr/share/icons/hicolor/*/apps/windscribe.png
