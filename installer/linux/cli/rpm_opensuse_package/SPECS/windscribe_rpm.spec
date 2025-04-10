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
Requires:	glibc >= 2.35
Requires:	libstdc++6
Requires:	glib2
Requires:	zlib
Requires:	libsystemd0
Requires:	sudo
Requires:	shadow
Requires:	procps
Requires:	polkit
Requires:	pkexec
Requires:	iproute2
Requires:	libcap-progs
Requires:	iw
Requires:	ethtool
Requires:	psmisc

%description
Windscribe CLI client.

%prep

%build

%install
rm -rf %{buildroot}
mkdir -p %{buildroot}
mv -f %{_sourcedir}/* %{buildroot}

%posttrans
killall -q Windscribe
systemctl daemon-reload || true
systemctl disable firewalld || true
systemctl stop firewalld || true
systemctl preset windscribe-helper || true
systemctl restart windscribe-helper || true

%post
ln -sf /opt/windscribe/windscribe-cli /usr/bin/windscribe-cli
setcap cap_setgid+ep /opt/windscribe/Windscribe
echo linux_rpm_opensuse_x64_cli > ../etc/windscribe/platform

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
fi

%files
%config /etc/windscribe/*
/opt/windscribe/*
/usr/lib/systemd/system-preset/69-windscribe-helper.preset
/usr/lib/systemd/system/windscribe-helper.service
/usr/lib/systemd/user/windscribe.service
/usr/polkit-1/actions/com.windscribe.authhelper.policy
