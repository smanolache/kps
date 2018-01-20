#
# spec file for package kpowersave (Version 0.7.2_SVN20070830)
#
# Copyright (c) 2007 SUSE LINUX Products GmbH, Nuernberg, Germany.
# This file and all modifications and additions to the pristine
# package are under the same license as the package itself.
#
# Please submit bugfixes or comments via http://bugs.opensuse.org/
#

# norootforbuild

Name:           kpowersave
URL:            http://sourceforge.net/projects/powersave/
%if 0%{?sles_version} > 0
# needed to detect in configure if this is a Enterprise product and set the correct bugzilla website
BuildRequires:  dbus-1-qt3-devel hal-devel kdelibs3-devel lsb sles-release
%else
BuildRequires:  dbus-1-qt3-devel hal-devel kdelibs3-devel lsb openSUSE-release
%endif
License:        GPL v2 only
Group:          System/GUI/KDE
BuildRoot:      %{_tmppath}/%{name}-%{version}-build
Summary:        KDE Frontend for Battery Monitoring, Suspend, CPU Freq and General Power Management Support
Version:        0.7.3
Release:        1
ExclusiveArch:  %ix86 x86_64 ia64 ppc ppc64
Requires:       /sbin/pidof xorg-x11
Source:         %{name}-%{version}.tar.bz2
%if %suse_version > 1020
Requires:       %{name}-lang = %{version}
%endif

%description
KPowersave provides battery monitoring, CPU frequency control and
suspend/standby triggers, and more power management features for KDE.
It uses HAL (formerly the powersave daemon) and supports APM and ACPI
for several architectures. Together with the powersave package and the
YaST power management module, it is the preferred package that should
be used for battery monitoring and control of power management related
tasks.



Authors:
--------
    Thomas Renninger (trenn@suse.de, mail@renninger.de)
    Danny Kukawka (dkukawka@suse.de, danny.kukawka@web.de)

%if %suse_version > 1020
%lang_package
%endif
%prep
%setup -n %{name}-%{version} -q
. /etc/opt/kde3/common_options
update_admin

%build
. /etc/opt/kde3/common_options
%if 0%{?sles_version} > 0
./configure --enable-yast-entry --enable-suse-sles
%else
./configure 
%endif
make %{?jobs:-j %jobs}

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
%if %suse_version > 1020

%files lang -f %name.lang

%files
%else

%files -f %name.lang
%endif
%defattr(-,root,root)
%doc README AUTHORS ChangeLog COPYING INSTALL NEWS
/opt/kde3/bin/*
/opt/kde3/share/config/kpowersaverc
/opt/kde3/share/autostart/kpowersave-autostart.desktop
/opt/kde3/share/apps/kpowersave
/opt/kde3/share/icons/??color
/opt/kde3/share/applications/kde/kpowersave.desktop
/opt/kde3/lib*/kde3/kpowersave.*
/opt/kde3/lib*/libkdeinit_kpowersave.*

%changelog
