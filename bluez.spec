# 
# Do NOT Edit the Auto-generated Part!
# Generated by: spectacle version 0.22
# 
# >> macros
# << macros

Name:       bluez
Summary:    Bluetooth utilities
Version:    4.96
Release:    1
Group:      Applications/System
License:    GPLv2+
URL:        http://www.bluez.org/
Source0:    http://www.kernel.org/pub/linux/bluetooth/%{name}-%{version}.tar.gz
Source1:    bluetooth.service
Source100:  bluez.yaml
Patch0:     bluez-fsync.patch
Patch1:     remove-duplicate-wrong-udev-rule-for-dell-mice.patch
Patch2:     enable_HFP.patch
Patch3:     powered.patch
Patch4:     install-test-scripts.patch
Patch5:     install-more-binary-test.patch
Patch6:     disable-hal-plugin.patch
Requires:   bluez-libs = %{version}
Requires:   dbus >= 0.60
Requires:   hwdata >= 0.215
Requires(post):   systemd 
Requires(postun): systemd 
Requires(preun):  systemd 
BuildRequires:  pkgconfig(dbus-1)
BuildRequires:  pkgconfig(libusb)
BuildRequires:  pkgconfig(alsa)
BuildRequires:  pkgconfig(udev)
BuildRequires:  pkgconfig(sndfile)
BuildRequires:  pkgconfig(glib-2.0)
BuildRequires:  pkgconfig(gstreamer-plugins-base-0.10)
BuildRequires:  pkgconfig(gstreamer-0.10)
BuildRequires:  flex


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
Group:      System/Libraries
Requires:   %{name} = %{version}-%{release}
Requires(post): /sbin/ldconfig
Requires(postun): /sbin/ldconfig

%description libs
Libraries for use in Bluetooth applications.

%package libs-devel
Summary:    Development libraries for Bluetooth applications
Group:      Development/Libraries
Requires:   %{name} = %{version}-%{release}
Requires:   bluez-libs = %{version}

%description libs-devel
bluez-libs-devel contains development libraries and headers for
use in Bluetooth applications.


%package cups
Summary:    CUPS printer backend for Bluetooth printers
Group:      System/Daemons
Requires:   %{name} = %{version}-%{release}
Requires:   bluez-libs = %{version}
Requires:   cups

%description cups
This package contains the CUPS backend

%package alsa
Summary:    ALSA support for Bluetooth audio devices
Group:      System/Daemons
Requires:   %{name} = %{version}-%{release}
Requires:   bluez-libs = %{version}

%description alsa
This package contains ALSA support for Bluetooth audio devices

%package gstreamer
Summary:    GStreamer support for SBC audio format
Group:      System/Daemons
Requires:   %{name} = %{version}-%{release}
Requires:   bluez-libs = %{version}

%description gstreamer
This package contains gstreamer plugins for the Bluetooth SBC audio format

%package test
Summary:    Test Programs for BlueZ
Group:      Development/Tools
Requires:   %{name} = %{version}-%{release}
Requires:   bluez-libs = %{version}
Requires:   dbus-python
Requires:   pygobject2

%description test
Scripts for testing BlueZ and its functionality


%prep
%setup -q -n %{name}-%{version}

# bluez-fsync.patch
%patch0 -p1
# remove-duplicate-wrong-udev-rule-for-dell-mice.patch
%patch1 -p1
# enable_HFP.patch
%patch2 -p1
# powered.patch
%patch3 -p1
# install-test-scripts.patch
%patch4 -p1
# install-more-binary-test.patch
%patch5 -p1
# disable-hal-plugin.patch
%patch6 -p1
# >> setup
# << setup

%build
# >> build pre
# << build pre

%reconfigure --disable-static \
    --enable-cups \
    --enable-hid2hci \
    --enable-dfutool \
    --enable-bccmd \
    --enable-hidd \
    --enable-pand \
    --enable-dund \
    --enable-gstreamer \
    --enable-alsa \
    --enable-usb \
    --enable-tools \
    --enable-test \
    --with-telephony=dummy

make %{?jobs:-j%jobs}

# >> build post
# << build post
%install
rm -rf %{buildroot}
# >> install pre
# << install pre
%make_install

# >> install post
install -D -m 0644 %SOURCE1 $RPM_BUILD_ROOT/%{_lib}/systemd/system/bluetooth.service
mkdir -p $RPM_BUILD_ROOT/%{_lib}/systemd/system/bluetooth.target.wants
mkdir -p $RPM_BUILD_ROOT/%{_lib}/systemd/system/basic.target.wants
ln -s ../bluetooth.service $RPM_BUILD_ROOT/%{_lib}/systemd/system/bluetooth.target.wants/bluetooth.service
ln -s ../bluetooth.target $RPM_BUILD_ROOT/%{_lib}/systemd/system/basic.target.wants/bluetooth.target

# Remove the cups backend from libdir, and install it in /usr/lib whatever the install
rm -rf ${RPM_BUILD_ROOT}%{_libdir}/cups
install -D -m 0755 cups/bluetooth ${RPM_BUILD_ROOT}/usr/lib/cups/backend/bluetooth

install -d -m 0755 $RPM_BUILD_ROOT/%{_localstatedir}/lib/bluetooth

# << install post

%post libs -p /sbin/ldconfig
%postun libs -p /sbin/ldconfig



%post 
systemctl daemon-reload
systemctl restart bluetooth.service
 
%preun
systemctl stop bluetooth.service
 
%postun
systemctl daemon-reload

%files
%defattr(-,root,root,-)
# >> files
%defattr(-, root, root)
%{_bindir}/ciptool
%{_bindir}/dfutool
%{_bindir}/dund
%{_bindir}/hcitool
%{_bindir}/hidd
%{_bindir}/l2ping
%{_bindir}/pand
%{_bindir}/rfcomm
%{_bindir}/sdptool
%{_sbindir}/*
%doc %{_mandir}/man1/*
%doc %{_mandir}/man8/*
%config(noreplace) %{_sysconfdir}/bluetooth/*
%config %{_sysconfdir}/dbus-1/system.d/bluetooth.conf
%{_localstatedir}/lib/bluetooth
/%{_lib}/udev/*
/%{_lib}/systemd/system/bluetooth.service
/%{_lib}/systemd/system/basic.target.wants/bluetooth.target
/%{_lib}/systemd/system/bluetooth.target.wants/bluetooth.service

# << files


%files libs
%defattr(-,root,root,-)
# >> files libs
%defattr(-, root, root)
%{_libdir}/libbluetooth.so.*
%doc AUTHORS COPYING INSTALL README
# << files libs

%files libs-devel
%defattr(-,root,root,-)
# >> files libs-devel
%defattr(-, root, root)
%{_libdir}/libbluetooth.so
%dir %{_includedir}/bluetooth
%{_includedir}/bluetooth/*
%{_libdir}/pkgconfig/bluez.pc
# << files libs-devel

%files cups
%defattr(-,root,root,-)
# >> files cups
%defattr(-, root, root)
/usr/lib/cups/backend/bluetooth
# << files cups

%files alsa
%defattr(-,root,root,-)
# >> files alsa
%defattr(-, root, root)
%{_libdir}/alsa-lib/*.so
%{_datadir}/alsa/bluetooth.conf
# << files alsa

%files gstreamer
%defattr(-,root,root,-)
# >> files gstreamer
%defattr(-, root, root)
%{_libdir}/gstreamer-*/*.so
# << files gstreamer

%files test
%defattr(-,root,root,-)
# >> files test
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
# << files test

