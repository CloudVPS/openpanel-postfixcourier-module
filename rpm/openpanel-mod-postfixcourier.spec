%define version 0.9.10

%define libpath /usr/lib
%ifarch x86_64
  %define libpath /usr/lib64
%endif

Summary: Mailserver configuration module
Name: openpanel-mod-postfixcourier
Version: %version
Release: 1
License: GPLv2
Group: Development
Source: http://packages.openpanel.com/archive/openpanel-mod-postfixcourier-%{version}.tar.gz
Patch1: openpanel-mod-postfixcourier-00-makefile
BuildRoot: /var/tmp/%{name}-buildroot
Requires: openpanel-core >= 0.8.3
Requires: openpanel-mod-user
Requires: openpanel-mod-domain
Requires: postfix
Requires: maildrop
Requires: courier-authlib
Requires: courier-authlib-userdb
Requires: courier-authlib-pipe
Requires: courier-imap
Requires: patch
Requires: cyrus-sasl-plain
Requires: sed

%description
Mailserver configuration module
OpenPanel module for driving Postfix+Courier+Maildrop

%prep
%setup -q -n openpanel-mod-postfixcourier-%version
%patch1 -p0 -b .buildroot

%build
BUILD_ROOT=$RPM_BUILD_ROOT
./configure
make

%install
BUILD_ROOT=$RPM_BUILD_ROOT
rm -rf ${BUILD_ROOT}
mkdir -p ${BUILD_ROOT}/var/opencore/modules/PostfixCourier.module
mkdir -p ${BUILD_ROOT}/etc/openpanel
mkdir -p ${BUILD_ROOT}/var/opencore/bin
cp -rf ./postfixcouriermodule.app ${BUILD_ROOT}/var/opencore/modules/PostfixCourier.module/
cp module.xml techsupport.* ${BUILD_ROOT}/var/opencore/modules/PostfixCourier.module/
cp *.png ${BUILD_ROOT}/var/opencore/modules/PostfixCourier.module/
ln -sf postfixcouriermodule.app/exec ${BUILD_ROOT}/var/opencore/modules/PostfixCourier.module/action
install -m 750 RedHat/postinst ${BUILD_ROOT}/var/opencore/modules/PostfixCourier.module/postinst
install -m 750 postarray ${BUILD_ROOT}/var/opencore/bin/postarray
install -m 755 masterconf ${BUILD_ROOT}/var/opencore/bin/masterconf
install -m 755 fixconfig ${BUILD_ROOT}/var/opencore/modules/PostfixCourier.module/fixconfig
install -m 755 verify ${BUILD_ROOT}/var/opencore/modules/PostfixCourier.module/verify
install -m 750 RedHat/postinst ${BUILD_ROOT}/var/opencore/modules/PostfixCourier.module/postinst

%post
/var/opencore/modules/PostfixCourier.module/postinst # TODO: move contents here
touch /etc/courier-auth/locallowercase
chgrp vmail /var/spool/authdaemon

%files
%defattr(-,root,root)
/
