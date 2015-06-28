%{!?_rel:%{expand:%%global _rel 0.enl%{?dist}}}
%define _missing_doc_files_terminate_build 0

Summary: Enlightenment Foundation Libraries
Name: efl
Version: 1.14.2
Release: %{_rel}
License: LGPLv2.1 GPLv2.1 BSD
Group: System Environment/Libraries
URL: http://www.enlightenment.org/
Source: http://download.enlightenment.org/releases/%{name}-%{version}.tar.gz
Packager: %{?_packager:%{_packager}}%{!?_packager:The Enlightenment Project <enlightenment-devel@lists.sourceforge.net>}
Vendor: %{?_vendorinfo:%{_vendorinfo}}%{!?_vendorinfo:The Enlightenment Project (http://www.enlightenment.org/)}
Distribution: %{?_distribution:%{_distribution}}%{!?_distribution:%{_vendor}}
BuildRequires: libjpeg-devel, zlib-devel, giflib-devel
BuildRequires: fribidi-devel, mesa-libGL-devel
BuildRequires: libX11-devel, libXinerama-devel, libXrender-devel, libXScrnSaver-devel
Provides: eo = %{version}-%{release}
Obsoletes: eo < %{version}-%{release}
Provides: eina = %{version}-%{release}
Obsoletes: eina < %{version}-%{release}
Provides: eet = %{version}-%{release}
Obsoletes: eet < %{version}-%{release}
Provides: embryo = %{version}-%{release}
Obsoletes: embryo < %{version}-%{release}
Provides: evas = %{version}-%{release}
Obsoletes: evas < %{version}-%{release}
Provides: eio = %{version}-%{release}
Obsoletes: eio < %{version}-%{release}
Provides: ecore = %{version}-%{release}
Obsoletes: ecore < %{version}-%{release}
BuildRoot: %{_tmppath}/%{name}-%{version}-root

%description
The Enlightenment Foundation Libraries are a collection of libraries
and tools upon which sophisticated graphical applications can be
built.  Included are a data structure library (Eina), a C-based object
engine (EO), a data storage library (EET), an object canvas (Evas),
and more.

%package devel
Summary: EFL headers, static libraries, documentation and test programs
Group: System Environment/Libraries
Requires: %{name} = %{version}
Provides: eo-devel = %{version}-%{release}
Obsoletes: eo-devel < %{version}-%{release}
Provides: eina-devel = %{version}-%{release}
Obsoletes: eina-devel < %{version}-%{release}
Provides: eet-devel = %{version}-%{release}
Obsoletes: eet-devel < %{version}-%{release}
Provides: embryo-devel = %{version}-%{release}
Obsoletes: embryo-devel < %{version}-%{release}
Provides: evas-devel = %{version}-%{release}
Obsoletes: evas-devel < %{version}-%{release}
Provides: eio-devel = %{version}-%{release}
Obsoletes: eio-devel < %{version}-%{release}
Provides: ecore-devel = %{version}-%{release}
Obsoletes: ecore-devel < %{version}-%{release}

%description devel
Headers, static libraries, test programs and documentation for EFL


%prep
%setup -q


%build
%{configure} --prefix=%{_prefix}
### use this if you have build problems
#./configure --prefix=%{_prefix}
%{__make} %{?_smp_mflags} %{?mflags}


%install
%{__make} %{?mflags_install} -j1 DESTDIR=$RPM_BUILD_ROOT install

%{find_lang} %{name}


%clean
test "x$RPM_BUILD_ROOT" != "x/" && rm -rf $RPM_BUILD_ROOT


%post
/sbin/ldconfig


%postun
/sbin/ldconfig


%files -f %{name}.lang
%defattr(-, root, root)
%doc AUTHORS README NEWS COPYING licenses/COPYING.BSD licenses/COPYING.LGPL licenses/COPYING.GPL
%{_bindir}/*
%{_libdir}/*.so.*
%{_libdir}/evas/cserve2/loaders/*/linux-gnu-*/*.so
%{_libdir}/evas/modules/engines/*/linux-gnu-*/*.so
%{_libdir}/evas/modules/image_loaders/*/linux-gnu-*/*.so
%{_libdir}/evas/modules/image_savers/*/linux-gnu-*/*.so
%{_libdir}/ecore/*/*.so
%{_libdir}/ecore_evas/*/*/*/*.so
%{_libexecdir}/*
%{_datadir}/eo/

%files devel
%defattr(-, root, root)
%{_includedir}/*
%{_libdir}/pkgconfig/*
%{_libdir}/*.a
%{_libdir}/*.la
%{_libdir}/*.so
%{_libdir}/ecore/*/*.la
%{_libdir}/ecore_evas/*/*/*/*.la
%{_libdir}/evas/cserve2/loaders/*/linux-gnu-*/*.la
%{_libdir}/evas/modules/engines/*/linux-gnu-*/*.la
%{_libdir}/evas/modules/image_loaders/*/linux-gnu-*/*.la
%{_libdir}/evas/modules/image_savers/*/linux-gnu-*/*.la
%{_datadir}/embryo/
%{_datadir}/evas/


%changelog
