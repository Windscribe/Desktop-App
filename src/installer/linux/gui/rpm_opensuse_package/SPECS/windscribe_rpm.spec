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
Conflicts:	windscribe-cli

Requires:	bash
Requires:	iptables
Requires:	glibc >= 2.35
Requires:	libstdc++6
Requires:	glib2
Requires:	zlib
Requires:	libglvnd
Requires:	libX11-6
Requires:	fontconfig
Requires:	freetype2
Requires:	libsystemd0
Requires:	libxcb1
Requires:	libxcb-util1
Requires:	xcb-util-cursor-devel
Requires:	sudo
Requires:	shadow
Requires:	procps
Requires:	polkit
Requires:	pkexec
Requires:	iproute2
Requires:	libcap-progs
Requires:	psmisc
Requires:	coreutils
Requires:	libgthread-2_0-0

%description
Windscribe client.

%prep

%build

%install
rm -rf %{buildroot}
mkdir -p %{buildroot}
mv -f %{_sourcedir}/* %{buildroot}

%posttrans
systemctl daemon-reload || true
systemctl disable firewalld || true
systemctl stop firewalld || true
systemctl preset windscribe-helper || true
systemctl restart windscribe-helper || true

%post
ln -sf /opt/windscribe/windscribe-cli /usr/bin/windscribe-cli
update-desktop-database
setcap cap_setgid+ep /opt/windscribe/Windscribe
mkdir -p /etc/windscribe
echo linux_rpm_opensuse_x64 > /etc/windscribe/platform

%preun
if [ $1 -eq 0 ]; then
    /opt/windscribe/helper --reset-mac-addresses
    systemctl disable windscribe-helper || true
fi

%postun
if [ $1 -eq 0 ]; then
    killall -q Windscribe || true
    systemctl stop windscribe-helper || true
    systemctl enable firewalld || true
    systemctl start firewalld || true
    userdel -f windscribe || true
    groupdel -f windscribe || true
    rm -f /usr/bin/windscribe-cli
    rm -rf /var/log/windscribe
    rm -rf /etc/windscribe
    rm -rf /opt/windscribe
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
