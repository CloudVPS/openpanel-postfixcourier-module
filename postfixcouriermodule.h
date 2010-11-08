// This file is part of OpenPanel - The Open Source Control Panel
// OpenPanel is free software: you can redistribute it and/or modify it 
// under the terms of the GNU General Public License as published by the Free 
// Software Foundation, using version 3 of the License.
//
// Please note that use of the OpenPanel trademark may be subject to additional 
// restrictions. For more information, please visit the Legal Information 
// section of the OpenPanel website on http://www.openpanel.com/

#ifndef _postfixcouriermodule_H
#define _postfixcouriermodule_H 1

#include <opencore/moduleapp.h>
#include <grace/system.h>
#include <grace/configdb.h>

typedef configdb<class postfixcourierModule> appconfig;

//  -------------------------------------------------------------------------
/// Main application class.
//  -------------------------------------------------------------------------
class postfixcourierModule : public moduleapp
{
public:
		 	 postfixcourierModule (void) :
				moduleapp ("openpanel.module.postfixcourier"), conf(this)
			 {
			 }
			~postfixcourierModule (void)
			 {
			 }

	int		 main (void);
	
protected:
		appconfig		conf; 		///< Module configuration data
		value			domains; 	///< domain cache
		value			aliases;	///< alias cache
		value			mailboxes;	///< mailbox cache
		
		/// check basic conditions of incoming request from opencore
		bool validaterequest();
		/// add/replace a user in the courier-database
		bool setCourierUser (const string &userid, const string &password,
							 bool imap, bool pop3, bool smtpauth);
		/// remove a user from the courier database
		bool delCourierUser (const string &userid);
		/// rewrite courier plain text files
		bool rewriteCourier (void);
		/// put a domain in the virtual domain list
		bool setPostfixDomain (const string &domain);
		/// remove a domain from the virtual domain list
		bool delPostfixDomain (const string &domain);
		/// set remote delivery for a domain
		bool setRemoteHost (const string &domain, const string &remotehost);
		/// remove remote delivery for a domain
		bool delRemoteHost (const string &domain);
		/// put an email address in the virtual box list
		bool setPostfixBox (const string &address);
		/// remove a virtual box address
		bool delPostfixBox (const string &address);
		/// put an email address in the virtual alias list
		bool setPostfixAlias (const string &address, const value &destlist);
		/// remove an alias address
		bool delPostfixAlias (const string &address);
		/// rewrite postfix plain text files
		bool rewritePostfix (void);
		/// rewrite a single file postfix-style
		bool rewritePostfixFile (const string &fname);
		
		/// handle create/update for a Mail:Address
		bool updateAddress (void);
		/// handle create/update for a Mail:Destination
		bool updateDestination (void);
		/// handle delete for a Mail:Address
		bool deleteAddress (void);
		
		bool pf_rewrite_alias;           ///< does alias.db need a rewrite?
		bool pf_rewrite_mailbox;		 ///< does mailbox.db need a rewrite?
		bool pf_rewrite_mailbox_domains; ///< does mailbox_domains.db need a rewrite?
		bool pf_rewrite_transport_map;   ///< does transport_map.db need a rewrite?
		
		/// confwatcher stub
		bool confSystem (config::action act, keypath &path, 
                             const value &nval, const value &oval);
};

#endif
