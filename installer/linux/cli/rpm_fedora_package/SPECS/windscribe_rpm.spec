# make sure rpmbuild does not attempt to strip our binaries, which sometimes causes corruption
%define debug_package %{nil}
%define __strip /bin/true

%global __provides_exclude_from .*
%global __requires_exclude_from .*

Name:		windscribe-cli
Version:	2.6.0
Release:	0
Summary:	Windscribe CLI Client

Group:		Applications/Internet
License:	GPLv2
URL:		https://www.windscribe.com
Vendor:		Windscribe Limited
BuildArch:	x86_64
Source0:	windscribe.tar
Conflicts:	windscribe

Requires:	bash
Requires:	iptables
Requires:	glibc >= 2.28
Requires:	libstdc++
Requires:	glib2
Requires:	zlib
Requires:	dbus-libs
Requires:	systemd-libs
Requires:	sudo
Requires:	shadow-utils
Requires:	procps-ng
Requires:	polkit
Requires:	iproute
Requires:	iputils

%description
Windscribe CLI client.

%prep

%build

%install
rm -rf %{buildroot}
mkdir -p %{buildroot}
mv -f %{_sourcedir}/* %{buildroot}

%posttrans
killall -q Windscribe || true
systemctl daemon-reload || true
systemctl preset windscribe-helper || true
systemctl start windscribe-helper || true

%post
ln -sf /opt/windscribe/windscribe-cli /usr/bin/windscribe-cli
setcap cap_setgid+ep /opt/windscribe/Windscribe
echo linux_rpm_x64_cli > ../etc/windscribe/platform

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
    rm -rf /var/log/windscribe
fi

%files
%config /etc/windscribe/*
/opt/windscribe/*
/usr/lib/systemd/system-preset/69-windscribe-helper.preset
/usr/lib/systemd/system/windscribe-helper.service
/usr/lib/systemd/user/windscribe.service
/usr/polkit-1/actions/com.windscribe.authhelper.policy
