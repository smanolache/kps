#
# spec file for package kpowersave (Version 0.6.2)
#
# Copyright (c) hmacht@suse,de, danny.kukawka@web.de 
# This file and all modifications and additions to the pristine
# package are under the same license as the package itself.
#

# norootforbuild


Name:         kpowersave
License:      GPL
Group:        System/GUI/KDE
BuildRoot:    %{_tmppath}/%{name}-%{version}-build
Summary:      KDE Frontend to powersave Package, Battery Monitor and General Power Management Support
Version:      0.6.2
Release:      1
ExclusiveArch: %ix86 x86_64 ia64 ppc
Requires:     powersave >= 0.12.18 powersave-libs >= 0.12.18 /sbin/pidof /usr/X11R6/bin/xset
Source:       %{name}-%{version}.tar.bz2

%description
The package provides battery monitoring and suspend/standby triggers.
It is based on the powersave package and therefore supports APM and
ACPI. Together with the powersave package and the YaST Powermanagement
module it is the preferred package that should be used for battery
monitoring and control of power management related tasks. See powersave
package for additional features such as CPU frequency scaling(SpeedStep
and PowerNow) and more.



Authors:
--------
    Thomas Renninger (trenn@suse.de, mail@renninger.de)
    Danny Kukawka (dkukawka@suse.de, danny.kukawka@web.de)

%prep
%setup -n %{name}-%{version} -q

%build
make -f admin/Makefile.common cvs
./configure --prefix=/usr
  
make

%install
make DESTDIR=$RPM_BUILD_ROOT install 

%post

%postun

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root)
%doc README AUTHORS ChangeLog COPYING INSTALL
/usr/bin/kpowersave
/usr/share/config/kpowersaverc
/usr/share/autostart/kpowersave-autostart.desktop
/usr/share/apps/kpowersave
/usr/share/icons/??color
/usr/share/applications/kde/kpowersave.desktop
/usr/share/doc/HTML/*/kpowersave
/usr/lib*/kde3/kpowersave.*
/usr/lib*/libkdeinit_kpowersave.*
/usr/share/locale/*

%changelog -n kpowersave
