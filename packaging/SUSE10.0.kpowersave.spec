#
# spec file for package kpowersave (Version 0.6.2)
#
# Copyright (c) 2006 SUSE LINUX Products GmbH, Nuernberg, Germany.
# This file and all modifications and additions to the pristine
# package are under the same license as the package itself.
#
# Please submit bugfixes or comments via http://bugs.opensuse.org
#

# norootforbuild
# neededforbuild  dbus-1 dbus-1-devel dbus-1-qt3 hal hal-devel kde3-devel-packages kdelibs3 lsb pkgconfig powersave powersave-devel powersave-libs suse-release

Name:           kpowersave
BuildRequires:  dbus-1-qt3-devel hal-devel kdelibs3-devel powersave-devel lsb suse-release
License:        GPL
Group:          System/GUI/KDE
BuildRoot:      %{_tmppath}/%{name}-%{version}-build
Summary:        KDE Front-End to powersave Package, Battery Monitor, and General Power Management Support
Version:        0.6.2
Release:        1
ExclusiveArch:  %ix86 x86_64 ia64 ppc
Requires:       powersave >= 0.12.18 powersave-libs >= 0.12.18 yast2-power-management /sbin/pidof /usr/X11R6/bin/xset
Source:         %{name}-%{version}.tar.bz2

%description
The package provides battery monitoring and suspend and standby
triggers. It is based on the powersave package and therefore supports
APM and ACPI. Together with the powersave package and the YaST power
management module, it is the preferred package that should be used for
battery monitoring and control of power management related tasks. See
the powersave package for additional features, such as CPU frequency
scaling (SpeedStep and PowerNow).



Authors:
--------
    Thomas Renninger (trenn@suse.de, mail@renninger.de)
    Danny Kukawka (dkukawka@suse.de, danny.kukawka@web.de)

%prep
%setup -n %{name}-%{version} -q
. /etc/opt/kde3/common_options
update_admin

%build
. /etc/opt/kde3/common_options
./configure --enable-yast-entry
  
make

%install
. /etc/opt/kde3/common_options
make DESTDIR=$RPM_BUILD_ROOT $INSTALL_TARGET
%suse_update_desktop_file %name Utility TrayIcon
%find_lang %name

%post
%{run_ldconfig} 

%postun
%{run_ldconfig} 

%clean
rm -rf $RPM_BUILD_ROOT

%files -f %name.lang
%defattr(-,root,root)
%doc README AUTHORS ChangeLog COPYING INSTALL NEWS
/opt/kde3/share/doc/*
/opt/kde3/bin/*
/opt/kde3/share/config/kpowersaverc
/opt/kde3/share/autostart/kpowersave-autostart.desktop
/opt/kde3/share/apps/kpowersave
/opt/kde3/share/icons/??color
/opt/kde3/share/applications/kde/kpowersave.desktop
/opt/kde3/lib*/kde3/kpowersave.*
/opt/kde3/lib*/libkdeinit_kpowersave.*

%changelog -n kpowersave

