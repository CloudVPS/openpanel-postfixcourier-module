// This file is part of OpenPanel - The Open Source Control Panel
// OpenPanel is free software: you can redistribute it and/or modify it 
// under the terms of the GNU General Public License as published by the Free 
// Software Foundation, using version 3 of the License.
//
// Please note that use of the OpenPanel trademark may be subject to additional 
// restrictions. For more information, please visit the Legal Information 
// section of the OpenPanel website on http://www.openpanel.com/

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
