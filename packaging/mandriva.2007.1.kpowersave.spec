%define name kpowersave
%define version 0.7.2
%define release %mkrel 1

Summary: 	KDE power management applet
Name: 		%{name}
Version:	%{version}
Release: 	%{release}
Source0: 	http://prdownloads.sourceforge.net/powersave/%{name}-%{version}.tar.bz2

License: 	GPL
Group: 		Graphical desktop/KDE
Url: 		http://powersave.sourceforge.net/
BuildRoot: 	%{_tmppath}/%{name}-%{version}-%{release}-buildroot
BuildRequires: 	hal-devel, libdbus-qt-1-devel, kdelibs-devel
%ifarch x86_64
BuildRequires:  lib64qt3-devel, lib64qt3-static-devel lib64gamin-1_0-devel lib64art_lgpl2-devel lib64kdecore4-devel
%else
BuildRequires:  libqt3-devel libqt3-static-devel libgamin-1_0-devel libart_lgpl2-devel libkdecore4-devel
%endif
BuildRequires: 	autoconf2.5 automake1.8
%if %mdkversion > 200600
BuildRequires:	libxscrnsaver-devel libxtst-devel
%endif
Requires:     	hal >= 0.5.8.1 /sbin/pidof /usr/bin/xset
%ifarch x86_64  
Requires:     	lib64dbus-qt-1_1 
%else
Requires:     	libdbus-qt-1_1 
%endif
%description
KPowersave provides battery monitoring, CPU frequency control and suspend/standby
triggers and more power management features for KDE. It uses HAL (formerly the
powersave daemon) and supports APM and ACPI for several architectures.

Authors:
--------
    Danny Kukawka (dkukawka@suse.de, danny.kukawka@web.de)
    Thomas Renninger (trenn@suse.de, mail@renninger.de)


#--------------------------------------------------------------------

%prep
%setup -q -n %{name}-%{version}

%build
%make -f admin/Makefile.common cvs
%configure --disable-rpath
%make

%install
rm -rf %{buildroot}
%makeinstall_std

%find_lang %{name}

mkdir -p $RPM_BUILD_ROOT%{_menudir}
kdedesktop2mdkmenu.pl %{name} "System/Monitoring" $RPM_BUILD_ROOT%{_datadir}/applications/kde/kpowersave.desktop $RPM_BUILD_ROOT%{_menudir}/%{name}
install -D -m 644 %{buildroot}%{_iconsdir}/hicolor/32x32/apps/%{name}.png %{buildroot}%{_iconsdir}/%{name}.png
install -D -m 644 %{buildroot}%{_iconsdir}/hicolor/48x48/apps/%{name}.png %{buildroot}%{_liconsdir}/%{name}.png
install -D -m 644 %{buildroot}%{_iconsdir}/hicolor/16x16/apps/%{name}.png %{buildroot}%{_miconsdir}/%{name}.png

%post
%{update_menus}
%if %mdkversion > 200600
%update_icon_cache hicolor
%endif

%postun
%{clean_menus}
%if %mdkversion > 200600
%clean_icon_cache hicolor
%endif

%clean
rm -rf %{buildroot}

%files -f %{name}.lang
%defattr(-,root,root)
%doc README AUTHORS ChangeLog COPYING INSTALL
%{_bindir}/%{name}
%{_datadir}/config/kpowersaverc
%{_datadir}/autostart/kpowersave-autostart.desktop
%{_datadir}/apps/%{name}
%{_iconsdir}/hicolor/*/apps/*.png
%{_datadir}/icons/*.png
%{_datadir}/applications/kde/kpowersave.desktop
%doc %{_docdir}/HTML/*/kpowersave
%{_libdir}/kde3/*
%{_libdir}/*.so
%{_libdir}/*.la
%{_miconsdir}/%{name}.png
%{_iconsdir}/%{name}.png
%{_liconsdir}/%{name}.png
%{_menudir}/%{name}

%changelog
