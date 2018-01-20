#
# spec file for package kpowersave (Version 0.3.9)
#
# Copyright (c) 2004 SUSE LINUX AG, Nuernberg, Germany.
# This file and all modifications and additions to the pristine
# package are under the same license as the package itself.
#
# Please submit bugfixes or comments via http://www.suse.de/feedback/
#

# norootforbuild
# neededforbuild  kde3-devel-packages kdelibs3 powersave

BuildRequires: aaa_base acl attr bash bind-utils bison bzip2 coreutils cpio cpp cracklib cvs cyrus-sasl db devs diffutils e2fsprogs file filesystem fillup findutils flex gawk gdbm-devel glibc glibc-devel glibc-locale gpm grep groff gzip info insserv less libacl libattr libgcc libnscd libselinux libstdc++ libxcrypt libzio m4 make man mktemp module-init-tools ncurses ncurses-devel net-tools netcfg openldap2-client openssl pam pam-modules patch permissions popt procinfo procps psmisc pwdutils rcs readline sed strace syslogd sysvinit tar tcpd texinfo timezone unzip util-linux vim zlib zlib-devel arts arts-devel autoconf automake binutils expat fam fam-devel fontconfig fontconfig-devel freeglut freeglut-devel freetype2 freetype2-devel gcc gcc-c++ gdbm gettext glib2 glib2-devel gnome-filesystem jack jack-devel kdelibs3 kdelibs3-devel kdelibs3-doc libart_lgpl libart_lgpl-devel libidn libidn-devel libjpeg libjpeg-devel liblcms liblcms-devel libmng libmng-devel libpng libpng-devel libstdc++-devel libtiff libtiff-devel libtool libxml2 libxml2-devel libxslt libxslt-devel openssl-devel pcre pcre-devel perl powersave qt3 qt3-devel rpm update-desktop-files xorg-x11-Mesa xorg-x11-Mesa-devel xorg-x11-devel xorg-x11-libs

Name:         kpowersave
License:      LGPL
Group:        System/GUI/KDE
BuildRoot:    %{_tmppath}/%{name}-%{version}-build
Summary:      KDE Frontend to powersave Package, Battery Monitor and General Power Management Support
Version:      0.3.8
Release:      2
Requires:     powersave >= 0.8.14 yast2-power-management
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
. /etc/opt/kde3/common_options
update_admin

%build
. /etc/opt/kde3/common_options
./configure $configkde
  
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
/opt/kde3/bin/*
/opt/kde3/share/autostart/kpowersave.desktop
/opt/kde3/share/apps/kpowersave
/opt/kde3/share/icons/??color
/opt/kde3/share/applications/kde/kpowersave.desktop
/opt/kde3/lib*/kde3/kpowersave.*
/opt/kde3/lib*/libkdeinit_kpowersave.*

%changelog -n kpowersave
