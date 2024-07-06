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
Requires:	libstdc++6
Requires:	glib2
Requires:	zlib
Requires:	libsystemd0
Requires:	sudo
Requires:	shadow
Requires:	procps
Requires:	polkit
Requires:	iproute2

%description
Windscribe CLI client.

%prep

%build

%install
rm -rf %{buildroot}
mkdir -p %{buildroot}
mv -f %{_sourcedir}/* %{buildroot}

%posttrans
systemctl disable firewalld
systemctl stop firewalld
systemctl enable windscribe-helper
systemctl restart windscribe-helper

%post
ln -sf /opt/windscribe/windscribe-cli /usr/bin/windscribe-cli
update-desktop-database
echo linux_rpm_opensuse_x64_headless > ../etc/windscribe/platform

%postun
if [ $1 -eq 0 ]; then
    killall -q Windscribe || true
    systemctl stop windscribe-helper || true
    systemctl disable windscribe-helper || true
    systemctl enable firewalld
    systemctl start firewalld
    userdel -f windscribe || true
    groupdel -f windscribe || true
    rm -f /usr/bin/windscribe-cli
    rm -f /opt/windscribe/helper_log.txt
    rm -rf /etc/windscribe
    rm -rf /opt/windscribe
fi

%files
%config /etc/systemd/system-preset/50-windscribe-helper.preset
%config /etc/systemd/system/windscribe-helper.service
%config /etc/systemd/user/windscribe.service
%config /etc/windscribe/*
/opt/windscribe/*
/usr/polkit-1/actions/com.windscribe.authhelper.policy
