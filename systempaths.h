// --------------------------------------------------------------------------
// OpenPanel - The Open Source Control Panel
// Copyright (c) 2006-2007 PanelSix
//
// This software and its source code are subject to version 2 of the
// GNU General Public License. Please be aware that use of the OpenPanel
// and PanelSix trademarks and the IconBase.com iconset may be subject
// to additional restrictions. For more information on these restrictions
// and for a copy of version 2 of the GNU General Public License, please
// visit the Legal and Privacy Information section of the OpenPanel
// website on http://www.openpanel.com/
// --------------------------------------------------------------------------

#ifndef _POSTFIXCOURIER_SYSTEMPATHS_H
#define _POSTFIXCOURIER_SYSTEMPATHS_H 1

#include "platform.h"

#ifdef __FLAVOR_LINUX_REDHAT

  #define PATH_SMTPD_SASL "/usr/lib/sasl2/smtpd.conf"
  #define PATH_AUTHDAEMOND_SOCK "/var/spool/authdaemon/socket"
  #define PATH_POSTFIX_MAINCF "/etc/postfix/main.cf"
  #define PATH_POSTFIX_MASTERCF "/etc/postfix/master.cf"
  
#elif __FLAVOR_LINUX_DEBIAN

  #define PATH_SMTPD_SASL "/etc/postfix/sasl/smtpd.conf"
  #define PATH_AUTHDAEMOND_SOCK "/var/run/courier/authdaemon/socket"
  #define PATH_POSTFIX_MAINCF "/etc/postfix/main.cf"
  #define PATH_POSTFIX_MASTERCF "/etc/postfix/master.cf"
  
#else

  #define PATH_SMTPD_SASL "smtpd.conf"
  #define PATH_AUTHDAEMOND_SOCK "/var/spool/authdaemon/socket"
  #define PATH_POSTFIX_MAINCF "main.cf"
  #define PATH_POSTFIX_MASTERCF "master.cf"
  
#endif

#endif
