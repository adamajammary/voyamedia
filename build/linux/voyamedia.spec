%define _binaries_in_noarch_packages_terminate_build 0

Name:          voyamedia
Version:       __APP_VERSION_MAJOR__.__APP_VERSION_MINOR__
Release:       __APP_VERSION_PATCH__
Summary:       __APP_DESCRIPTION__
URL:           __APP_URL__
License:       GPL
Requires:      libgcc >= 4.8.3, libstdc++ >= 4.8.3
ExclusiveArch: __CPU_ARCH__

%description
__APP_DESCRIPTION__

%prep

%build

%install
cp -rf %{_builddir}/* %{_buildrootdir}/

%files
%doc /usr/share/doc/%{name}/README.txt
%license /usr/share/doc/%{name}/LICENSE.txt
%dir /usr/share/doc/%{name}/
%dir /opt/%{name}/
%{_bindir}/%{name}
/opt/%{name}/*
/usr/share/doc/%{name}/*
/usr/share/applications/%{name}.desktop
