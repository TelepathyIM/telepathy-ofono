Name:       telepathy-ofono
Summary:    Telepathy oFono connection manager
Version:    0.2.0
Release:    1
Group:      Applications/Communications
License:    LGPL-3
URL:        https://github.com/Kaffeine/telepathy-ofono
Source0:    https://github.com/Kaffeine/telepathy-ofono/archive/%{name}-%{version}.tar.bz2
Requires:   telepathy-mission-control
Requires(post): /sbin/ldconfig
Requires(postun): /sbin/ldconfig
Requires: qt5-qtcore
Requires: qt5-qtnetwork
Requires: openssl
BuildRequires: pkgconfig(dbus-1) >= 1.1.0
BuildRequires: pkgconfig(openssl)
BuildRequires: pkgconfig(sqlite3)
BuildRequires: pkgconfig(Qt5Core)
BuildRequires: pkgconfig(Qt5DBus)
BuildRequires: pkgconfig(Qt5Network)
BuildRequires: pkgconfig(Qt5Sql)
BuildRequires: pkgconfig(Qt5Test)
BuildRequires: pkgconfig(ofono-qt)
BuildRequires: libphonenumber-devel
BuildRequires: pkgconfig(TelepathyQt5) >= 0.9.6
BuildRequires: pkgconfig(TelepathyQt5Service) >= 0.9.6
BuildRequires: pkgconfig(TelepathyQt5Farstream) >= 0.9.6
BuildRequires: pkgconfig(mission-control-plugins) >= 5
BuildRequires: cmake >= 2.8

#Build-Depends: debhelper (>= 9),
# dbus-test-runner, dconf-cli, gnome-keyring, libmission-control-plugins-dev (>= 1:5.14.0),
# libofono-qt-dev (>= 1.5), libphonenumber-dev, libqt5sql5-sqlite, libtelepathy-qt5-dev (>= 0.9.3),
# libsqlite3-dev, qt5-default (>= 5.0), qtbase5-dev (>= 5.0), sqlite3, telepathy-mission-control-5

%description
Telepathy-ofono is a Telepathy connection manager that makes it possible for
Telepathy clients to communicate using oFono modems, enabling features like
real phone calls and send and receive SMSs.

%package ril-mc-plugin
Requires: telepathy-ofono
Summary: Telepathy oFono mission control plugin for ril modems
%description ril-mc-plugin
This telepathy mission-control plugin is used to automatically provision
telepathy-ofono accounts for each available ril modem.

%prep
%setup -q -n %{name}-%{version}

%build
cmake . \
    -DLIBEXEC_DIR=libexec \
    -DCMAKE_INSTALL_PREFIX=%{_prefix} \
    -DCMAKE_INSTALL_LIBEXECDIR=%{_libexecdir} \
    -DCMAKE_INSTALL_DATADIR=%{_datadir}

make %{?jobs:-j%jobs}

%install
rm -rf %{buildroot}
%make_install

%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig

%files
%defattr(-,root,root,-)
# >> files
%{_libexecdir}/%{name}
%{_datadir}/dbus-1/services/*.service
%{_datadir}/telepathy/managers/*.manager
%{_libdir}/mission-control-plugins.0/mcp-account-manager-ofono.so
