Name:       bluez

%define _system_groupadd() getent group %{1} >/dev/null || groupadd -g 1002 %{1}

Summary:    Bluetooth utilities
Version:    4.101
Release:    1
License:    GPLv2+
URL:        http://www.bluez.org/
Source0:    http://www.kernel.org/pub/linux/bluetooth/%{name}-%{version}.tar.gz
Source1:    bluez.tracing
Requires:   bluez-libs = %{version}
Requires:   dbus >= 0.60
Requires:   hwdata >= 0.215
Requires:   bluez-configs
Requires:   systemd
Requires:   ofono
Requires:   oneshot
Requires(pre): /usr/sbin/groupadd
Requires(preun): systemd
Requires(post): systemd
Requires(postun): systemd
BuildRequires:  pkgconfig(dbus-1)
BuildRequires:  pkgconfig(libusb)
BuildRequires:  pkgconfig(alsa)
BuildRequires:  pkgconfig(udev)
BuildRequires:  pkgconfig(sndfile)
BuildRequires:  pkgconfig(glib-2.0)
BuildRequires:  pkgconfig(check)
BuildRequires:  bison
BuildRequires:  flex
BuildRequires:  readline-devel

%description
Utilities for use in Bluetooth applications:
	--ciptool
	--dfutool
	--hcitool
	--l2ping
	--rfcomm
	--sdptool
	--hciattach
	--hciconfig
	--hid2hci

The BLUETOOTH trademarks are owned by Bluetooth SIG, Inc., U.S.A.


%package libs
Summary:    Libraries for use in Bluetooth applications
Requires(post): /sbin/ldconfig
Requires(postun): /sbin/ldconfig

%description libs
Libraries for use in Bluetooth applications.

%package libs-devel
Summary:    Development libraries for Bluetooth applications
Requires:   bluez-libs = %{version}

%description libs-devel
bluez-libs-devel contains development libraries and headers for
use in Bluetooth applications.


%package cups
Summary:    CUPS printer backend for Bluetooth printers
Requires:   %{name} = %{version}-%{release}
Requires:   bluez-libs = %{version}
Requires:   cups

%description cups
This package contains the CUPS backend

%package alsa
Summary:    ALSA support for Bluetooth audio devices
Requires:   %{name} = %{version}-%{release}
Requires:   bluez-libs = %{version}

%description alsa
This package contains ALSA support for Bluetooth audio devices

%package test
Summary:    Test Programs for BlueZ
Requires:   %{name} = %{version}-%{release}
Requires:   bluez-libs = %{version}
Requires:   dbus-python
Requires:   pygobject2 >= 3.10.2

%description test
Scripts for testing BlueZ and its functionality

%package doc
Summary:    Documentation for bluez
Requires:   %{name} = %{version}-%{release}

%description doc
This package provides man page documentation for bluez

%package configs-mer
Summary:    Default configuration for bluez
Requires:   %{name} = %{version}-%{release}
Provides:   bluez-configs

%description configs-mer
This package provides default configs for bluez

%package tracing
Summary:    Configuration for bluez to enable tracing
Requires:   %{name} = %{version}-%{release}

%description tracing
Will enable tracing for BlueZ

%prep
%setup -q -n %{name}-%{version}/%{name}

%build

./bootstrap
%reconfigure --disable-static \
    --enable-cups \
    --enable-hid2hci \
    --enable-dfutool \
    --enable-bccmd \
    --enable-hidd \
    --enable-pand \
    --enable-dund \
    --disable-gstreamer \
    --enable-alsa \
    --enable-usb \
    --enable-tools \
    --enable-test \
    --enable-hal=no \
    --with-telephony=ofono \
    --with-systemdunitdir=%{_unitdir} \
    --enable-jolla-dbus-access \
    --enable-gatt \
    --enable-jolla-did \
    --enable-jolla-wakelock \
    --enable-jolla-logcontrol \
    --enable-jolla-voicecall

make %{?jobs:-j%jobs}


%install
rm -rf %{buildroot}
%make_install

mkdir -p ${RPM_BUILD_ROOT}%{_unitdir}/network.target.wants
ln -s ../bluetooth.service ${RPM_BUILD_ROOT}%{_unitdir}/network.target.wants/bluetooth.service
(cd ${RPM_BUILD_ROOT}%{_unitdir} && ln -s bluetooth.service dbus-org.bluez.service)

# Remove the cups backend from libdir, and install it in /usr/lib whatever the install
rm -rf ${RPM_BUILD_ROOT}%{_libdir}/cups
install -D -m 0755 cups/bluetooth ${RPM_BUILD_ROOT}/usr/lib/cups/backend/bluetooth

install -d -m 0755 ${RPM_BUILD_ROOT}%{_localstatedir}/lib/bluetooth

# Install configuration files
for CONFFILE in audio input network serial ; do
install -v -m644 ${CONFFILE}/${CONFFILE}.conf ${RPM_BUILD_ROOT}%{_sysconfdir}/bluetooth/${CONFFILE}.conf
done

mkdir -p %{buildroot}%{_sysconfdir}/tracing/bluez/
cp -a %{SOURCE1} %{buildroot}%{_sysconfdir}/tracing/bluez/

# there is no macro for /lib/udev afaict
%define udevlibdir /lib/udev

%pre
%_system_groupadd bluetooth

%preun
if [ "$1" -eq 0 ]; then
systemctl stop bluetooth.service ||:
fi

%post
%{_bindir}/groupadd-user bluetooth
systemctl daemon-reload ||:
systemctl reload-or-try-restart bluetooth.service ||:

%postun
systemctl daemon-reload ||:

%post libs -p /sbin/ldconfig

%postun libs -p /sbin/ldconfig

%files
%defattr(-,root,root,-)
%{_bindir}/ciptool
%{_bindir}/dfutool
%{_bindir}/dund
%{_bindir}/gatttool
%{_bindir}/hcitool
%{_bindir}/hidd
%{_bindir}/l2ping
%{_bindir}/pand
%{_bindir}/rfcomm
%{_bindir}/sdptool
%{_bindir}/mpris-player
%{_sbindir}/*
%config %{_sysconfdir}/dbus-1/system.d/bluetooth.conf
%{_localstatedir}/lib/bluetooth
%{_udevrulesdir}/*
%{udevlibdir}/hid2hci
%{_datadir}/dbus-1/system-services/org.bluez.service
%{_unitdir}/bluetooth.service
%{_unitdir}/network.target.wants/bluetooth.service
%{_unitdir}/dbus-org.bluez.service

%files libs
%defattr(-,root,root,-)
%{_libdir}/libbluetooth.so.*
%doc AUTHORS COPYING INSTALL README

%files libs-devel
%defattr(-,root,root,-)
%{_libdir}/libbluetooth.so
%dir %{_includedir}/bluetooth
%{_includedir}/bluetooth/*
%{_libdir}/pkgconfig/bluez.pc

%files cups
%defattr(-,root,root,-)
%{_libdir}/cups/backend/bluetooth

%files alsa
%defattr(-,root,root,-)
%{_libdir}/alsa-lib/*.so
%{_datadir}/alsa/bluetooth.conf

%files test
%defattr(-,root,root,-)
%{_libdir}/%{name}/test/*
%{_bindir}/hstest
%{_bindir}/gaptest
%{_bindir}/sdptest
%{_bindir}/l2test
%{_bindir}/btiotest
%{_bindir}/avtest
%{_bindir}/bdaddr
%{_bindir}/scotest
%{_bindir}/lmptest
%{_bindir}/attest
%{_bindir}/agent
%{_bindir}/test-textfile
%{_bindir}/rctest
%{_bindir}/ipctest
%{_bindir}/uuidtest

%files doc
%defattr(-,root,root,-)
%doc %{_mandir}/man1/*
%doc %{_mandir}/man8/*

%files configs-mer
%defattr(-,root,root,-)
%config %{_sysconfdir}/bluetooth/*

%files tracing
%defattr(-,root,root,-)
%dir %{_sysconfdir}/tracing/bluez
%config %{_sysconfdir}/tracing/bluez/bluez.tracing
