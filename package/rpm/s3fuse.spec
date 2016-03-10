%define name s3fuse
%define version 0.16
%define release 1

Summary: FUSE Driver for AWS S3 and Google Storage and IIJ GIO storage and analysis
Name: %{name}
Version: %{version}
Release: %{release}%{?dist}
Source: %{name}-%{version}.tar.gz
BuildRoot: %{_builddir}/%{name}-root
Group: Applications/System
License: Apache-2.0
BuildRequires: libstdc++-devel
BuildRequires: boost-devel >= 1.41
BuildRequires: fuse-devel >= 2.7.3
BuildRequires: libxml2-devel >= 2.7.6
BuildRequires: libcurl >= 7.0.0
Requires: libstdc++
Requires: boost >= 1.41
Requires: libxml2 >= 2.7.6
Requires: libcurl >= 7.0.0
Requires: fuse >= 2.7.3

%description
Provides a FUSE filesystem driver for Amazon AWS S3 and Google Storage buckets and IIJ GIO storage and analysis.

%prep
%setup -q -n %{name}-%{version}

%build
autoreconf --force --install
./configure --prefix=/usr --sysconfdir=/etc
make

%install
rm -rf $RPM_BUILD_ROOT
make DESTDIR=$RPM_BUILD_ROOT install

%clean
rm -rf $RPM_BUILD_ROOT

%post

%preun

%files
%defattr(-,root,root)
%config /etc/s3fuse.conf
%doc ChangeLog
%doc COPYING
%doc INSTALL
%doc README
%{_mandir}/man5/*
/usr/bin/s3fuse
/usr/bin/s3fuse_gs_get_token 
/usr/bin/s3fuse_sha256_sum
/usr/bin/s3fuse_vol_key
