Name: gqlplus
Version: 1.15
#Release: 1%{?dist}
Release: 1
Summary: gqlplus is a front-end for Oracle program sqlplus with command-line editing, history, table-name and column-name completion.

Group: System Environment/Libraries
License: GPLv2+
URL: http://gqlplus.sourceforge.net
Source0: http://sourceforge.net/projects/%{name}/files/%{name}/%{version}/%{name}-%{version}.tar.gz
BuildRoot:	%{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)

BuildRequires: ncurses-devel readline-devel
Requires: ncurses readline
%description
gqlplus is a UNIX front-end program for Oracle command-line utility sqlplus. 
gqlplus' functionally is nearly identical to sqlplus, with the addition of command-line editing, history, table-name 
and optional column-name completion. The editing is similar to tcsh and bash shells.

%prep
%setup -q 

%build
%configure
make -k %{?_smp_mflags}

%install
rm -rf $RPM_BUILD_ROOT
make install DESTDIR=$RPM_BUILD_ROOT INSTALL="install -p"

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root,-)
%{_bindir}/gqlplus

%doc LICENSE README ChangeLog


%changelog
* Tue Jan 31 2012 Jess Portnoy - 1.15-1
- Use mkstemp() instead tmpnam().
  This code also honors TMPDIR, TEMPDIR and TEMP ENV vars [in that order], otherwise,defaults to /tmp.
* Sun Dec 18 2011 Jess Portnoy - 1.13-1
- Initial package.
