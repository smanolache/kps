#
# spec file for package kpowersave (Version 0.7.2)
#
# Copyright (c) hmacht@suse.de, danny.kukawka@web.de
# This file and all modifications and additions to the pristine
# package are under the same license as the package itself.
#
# Please submit bugfixes or comments via http://bugs.opensuse.org
#

# norootforbuild


Name:         kpowersave
License:      GPL
Group:        System/GUI/KDE
BuildRoot:    %{_tmppath}/%{name}-%{version}-build
Summary:      KDE Frontend for Power Management, Battery Monitoring and Suspend
Version:      0.7.2
Release:      2.FC6%{?dist}
ExclusiveArch: %ix86 x86_64 ia64 ppc
Requires:     hal >= 0.5.8.1 dbus-qt /sbin/pidof /usr/bin/xset
Source:       %{name}-%{version}.tar.bz2

BuildRequires:  kdelibs-devel
BuildRequires:  gettext
BuildRequires:  dbus-qt-devel
BuildRequires:  automake
BuildRequires:  libXext-devel libXScrnSaver-devel libXtst-devel
BuildRequires:  hal-devel desktop-file-utils

%description
KPowersave provides battery monitoring, CPU frequency control and suspend/standby 
triggers and more power management features for KDE. It uses HAL (formerly the 
powersave daemon) and supports APM and ACPI for several architectures.


Authors:
--------
    Danny Kukawka (dkukawka@suse.de, danny.kukawka@web.de)
    Thomas Renninger (trenn@suse.de, mail@renninger.de)

%prep
%setup -n %{name}-%{version} -q
make -f admin/Makefile.common cvs

%build
unset QTDIR || : ; . /etc/profile.d/qt.sh

%configure \
  --disable-rpath \
  --enable-new-ldflags \
  --disable-debug --disable-warnings \
  --disable-dependency-tracking

make %{?_smp_mflags}

%install
make DESTDIR=$RPM_BUILD_ROOT install 

desktop-file-install \
--dir $RPM_BUILD_ROOT%{_datadir}/applications/kde \
--vendor="" \
$RPM_BUILD_ROOT%{_datadir}/applications/kde/kpowersave.desktop

## File lists
# locale's
%find_lang %{name} || touch %{name}.lang
# HTML
HTML_DIR=$(kde-config --expandvars --install html)
if [ -d $RPM_BUILD_ROOT$HTML_DIR ]; then
for lang_dir in $RPM_BUILD_ROOT$HTML_DIR/* ; do
   lang=$(basename $lang_dir)
   echo "%lang($lang) %doc $HTML_DIR/$lang/*" >> %{name}.lang
done
fi


%post
touch --no-create %{_datadir}/icons/hicolor || :

%postun
touch --no-create %{_datadir}/icons/hicolor || :

%clean
rm -rf $RPM_BUILD_ROOT

%files -f %{name}.lang
%defattr(-,root,root,-)
%doc README AUTHORS ChangeLog COPYING INSTALL
%{_bindir}/kpowersave
%{_libdir}/kde3/kpowersave.*
%{_libdir}/libkdeinit_kpowersave.*
%{_datadir}/applications/kde/*kpowersave.desktop
%{_datadir}/apps/kpowersave/
%{_datadir}/autostart/kpowersave-autostart.desktop
%{_datadir}/config/kpowersaverc
%{_datadir}/icons/hicolor/*/*/*

%changelog -n kpowersave
