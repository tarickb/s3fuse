%define name s3fuse
%define version 0.1
%define release 1

%define debug_package %{nil}

Summary: FUSE Driver for AWS S3
Name: %{name}
Version: %{version}
Release: %{release}
Source: %{name}-%{version}.tar.gz
Group: Applications/System
BuildRoot: %{_builddir}/%{name}-root
License: BSD

%description
Provides a FUSE filesystem driver for Amazon AWS S3 buckets.

%prep
%setup -q -n %{name}-%{version}

%build
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
/usr/bin/s3fuse
/usr/bin/xattr
