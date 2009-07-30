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

#include "postfixcouriermodule.h"
#include <dbfile/gdbmfile.h>
#include <dbfile/db4file.h>

APPOBJECT(postfixcourierModule);

// TODO: move postfix rmval to one method
// TODO: lots of errorchecking (trac:19)

#define PATH_STAGING "/var/opencore/conf/staging/PostfixCourier"

#define TRAP(foo) sendresult (moderr::err_module, "Uncaught error condition: " foo)

//  =========================================================================
/// Main method.
//  =========================================================================
int postfixcourierModule::main (void)
{
	pf_rewrite_alias = pf_rewrite_mailbox = 
					   pf_rewrite_mailbox_domains = pf_rewrite_transport_map = 
					   false;

	chdir (PATH_STAGING);
	fs.cd (PATH_STAGING);
	
	string conferr;
	
	conf.addwatcher ("config", &postfixcourierModule::confSystem);
	
	if(!conf.load ("openpanel.module.postfixcourier", conferr))
	{
		TRAP ("loadconfig: %s" %format (conferr));
		return 1;
	}

	if(!validaterequest())
	{
		TRAP ("validaterequest");
		return 1;
	}

	string maildom = data["Mail"]["id"];
    string subdom = data["Mail"]["id"];
    string remotehost = data["Mail"]["remotehost"];
    bool smtpforward = data["Mail"]["smtpforward"];
	string pdom = data["Domain"]["id"];
	subdom = subdom.left(subdom.strlen() - pdom.strlen() - 1);
	
	if (subdom != "")
	        subdom.strcat(".");

	if (command == "create" || command == "update")
	{
		caseselector (classid)
		{
			incaseof ("Mail"):
				// TODO: add @ in front of domain like we do with aliasdomains
				// unless this breaks something
				if (smtpforward && remotehost)
				{
					if (! setRemoteHost(maildom, remotehost))
					{
						TRAP ("setRemoteHost");
						return 0;
					}
						
					if (! delPostfixDomain (maildom))
					{
						TRAP ("delPostfixDomain");
						return 0;
					}
				}
				else
				{
					if (! delRemoteHost(maildom))
					{
						TRAP ("delRemoteHost");
						return 0;
					}
						
					if (! setPostfixDomain (maildom))
					{
						TRAP ("setPostfixDomain");
						return 0;
					}
				}
				
				if (!rewritePostfix ())
				{
					TRAP ("rewritePostfix");
					return 0;
				}
				break;
				
			incaseof ("Mail:Box"):
				if (!updateAddress ())
				{
					TRAP ("updateAddress");
					return 0;
				}
				break;
				
			incaseof ("Mail:Alias"):
				if (!updateAddress ())
				{
					TRAP ("updateAddress");
					return 0;
				}
				break;
			
			incaseof ("Mail:Destination"):
				if (! updateDestination ())
				{
					// if we get false, an error has already been sent
					authd.quit();
					TRAP ("updateDestination");
					return 0;
				}
				break;
				
			defaultcase: 
				// Other classes are not supported
				sendresult (moderr::err_command, "unknown class in request");
				return 0;
		}
		
		string dstdom;
		dstdom.printf("@%s", data["Mail"]["id"].str());
		value dstlist;
		dstlist.newval() = dstdom;
		
		foreach (aliasdom, data["Domain"]["Domain:Alias"])
		{
			string srcdom = "@%s%s" %format (subdom, aliasdom["id"]);

			if (! setPostfixAlias(srcdom, dstlist))
			{
				TRAP ("setPostfixAlias");
				return 0;
			}
				
			if (! setPostfixDomain(aliasdom["id"].str()))
			{
				TRAP ("setPostfixDomain");
				return 0;
			}
		}
		
		if (! rewritePostfix())
		{
			TRAP ("rewritePostfix");
			return 0;
		}
	}
	else if (command == "delete")
	{
		caseselector (classid)
		{
			incaseof ("Mail") :
				if (! delPostfixDomain (data["Mail"]["id"].sval()))
				{
					TRAP ("delPostfixDomain");
					return 0;
				}
					
				foreach (aliasdom, data["Domain"]["Domain:Alias"])
				{
					if (! delPostfixDomain(aliasdom["id"].str()))
					{
						TRAP ("delPostfixDomain (alias)");
						return 0;
					}
				}
				
				if (! delRemoteHost(data["Mail"]["id"].sval()))
				{
					TRAP ("delRemoteHost");
					return 0;
				}
					
				if (! rewritePostfix ())
				{
					TRAP ("rewritePostfix");
					return 0;
				}
					
				break;
				
			incaseof ("Mail:Box") :
				if (! deleteAddress())
				{
					TRAP ("deleteAddress");
					return 0;
				}
				break;
				
			incaseof ("Mail:Alias") :
				if (! deleteAddress())
				{
					TRAP ("deleteAddress");
					return 0;
				}
				break;
				
			incaseof ("Mail:Destination") :
				if (! updateDestination ())
				{
					authd.quit();
					TRAP ("updateDestination");
					return 0;
				}
				break;
				
			defaultcase : 
				// Other classes are not supported
				sendresult (moderr::err_command, "unknown class in request");
				return 0;
		}
	}
	else
	{
		sendresult (moderr::err_command, "unknown command");
		return 0;
	}
	
	authd.quit();
	sendresult (moderr::ok, "");
	return 0;
}

// ==========================================================================
// METHOD ::validaterequest
// ==========================================================================
bool postfixcourierModule::validaterequest()
{
	// verify classtypes
	// verify body contents
	// etc.
	return true;
}

// ==========================================================================
// METHOD updateAddress
// ==========================================================================
bool postfixcourierModule::updateAddress (void)
{
	
	if (data.exists ("Mail:Box"))
	{
		value &obj = data["Mail:Box"];
		string address = obj["id"];
		if(obj["forwardto"].sval().strlen())
		{
			value dstlist;
			dstlist.newval() = obj["forwardto"];
			if (! setPostfixAlias (address, dstlist))
				return false;
		}
		else
		{
			if (! delPostfixAlias (address))
				return false;
			if (! setPostfixBox (address))
				return false;
		}
		if (!rewritePostfix ())
			return false;
		if (! setCourierUser (address, obj["password"],
						obj["allowimap"], obj["allowpop3"],
						obj["allowrelay"]))
			return false;
			
		if (! rewriteCourier ())
			return false;
			
		if (obj["autorespond"] == false)
		{
			string fn = "/etc/vacationdrop/recipients/%s" %format (address);
			if (fs.exists (fn))
			{
				authd.deletefile (fn);
				fn = "/var/db/vacationdrop/%s" %format (address);
				authd.deletefile (fn);
			}
		}
		else
		{
			value rspdat = $("subject", "Automatic reply message") ->
						   $("body", obj["automessage"]);
			
			rspdat.savexml (address);
			
			if (authd.installfile (address, "/etc/vacationdrop/recipients"))
			{
				value E = authd.getlasterror ();
				sendresult (moderr::err_authdaemon, E["resultmsg"]);
				return false;
			}
		}
	}
	
	return true;
	// if it's an alias, we'll write it out once we get Destinations
}

// ==========================================================================
// METHOD ::deleteAddress
// ==========================================================================
bool postfixcourierModule::deleteAddress (void)
{
	value &obj = data.exists ("Mail:Box") ? data["Mail:Box"] : data["Mail:Alias"];
	string address = obj["id"].sval().lower();
	if (! delPostfixBox (address) ||
		! delPostfixAlias(address) ||
		! delCourierUser (address) ||
		! rewriteCourier () ||
		! rewritePostfix())
		return false;
	
	return true;
}

// ==========================================================================
// METHOD ::updateDestination
// ==========================================================================
bool postfixcourierModule::updateDestination (void)
{
	if (! data.exists ("Mail:Alias"))
	{
		sendresult (moderr::err_command, "only alias addresses can have destinations");
		return false;
	}

	value dstlist;
	string address = data["Mail:Alias"]["id"].sval().lower();
	
	if (data["Mail:Alias"]["pridest"].sval())
	{
		string dst = data["Mail:Alias"]["pridest"];
		if (dst.strchr('@', 0) == -1)
		{
			dst.strcat("@");
			dst.strcat(data["Mail"]["id"].sval().lower());
		}
		dstlist.newval() = dst;
	}
	
	foreach (dest, data["Mail:Alias"]["Mail:Destination"])
	{
		string dst = dest["address"];
		if (dst.strchr('@', 0) == -1)
		{
			dst.strcat("@");
			dst.strcat(data["Mail"]["id"].sval().lower());
		}
		dstlist.newval() = dst;
	}
	
	if (! setPostfixAlias(address, dstlist))
		return false;
		
	if (! rewritePostfix())
		return false;
	return true;
}

// ==========================================================================
// METHOD ::confSystem
// ==========================================================================
bool postfixcourierModule::confSystem (config::action act, keypath &kp,
	const value &nval, const value &oval)
{
    switch (act)
    {
            case config::isvalid:
                    return true;

            case config::create:
                    return true;            
    }

    return false;		
}

// ==========================================================================
// METHOD ::setCourierUser
// ==========================================================================
bool postfixcourierModule::setCourierUser (const string &in_userid,
										   const string &password,
										   bool imap,
										   bool pop3,
										   bool smtpauth)
{
	gdbmfile DB;
	gdbmfile SDB;
	static string validmd5pass ("abcdefghijklmnopqrstuvwxyz0123456789"
	                            "ABCDEFGHIJKLMNOPQRSTUVWXYZ/.$");
	
	statstring userid = in_userid.lower();
	
	DB.setencoding (dbfile::courierdb);
	SDB.setencoding (dbfile::courierdb);
	
	// Validate password
    if (! password.validate (validmd5pass))
    {
        sendresult (moderr::err_command, "invalid crypted password");
		return false;
    }
	
	// Open the public database file. Bail out on failure.
	if (! DB.open ("userdb.dat"))
	{
		sendresult (moderr::err_command, "failed to open userdb.dat");
		return false;
	}
	
	// Open the shadow database file. Bail out on failure.
	if (! SDB.open ("userdbshadow.dat"))
	{
		sendresult (moderr::err_command, "failed to open userdbshadow.dat");
		DB.close ();
		return false;
	}
	
	// Make sure the shadow database is not world-readable.
	fs.chmod ("userdbshadow.dat", 0600);
	
	// Get information about the vmail user.
	value pw;
	pw = kernel.userdb.getpwnam ("vmail");
	if (! pw)
	{
		// FIXME: why is this even here?
		pw = $("uid", 103) ->
			 $("gid", 106);
	}
	
	string hmdir; // Constructed home directory
	string mldir; // Constructor homedir directory
	
	// Extract the domain name part of the id.
	string dom = userid;
	string us = userid;
	us.cropatlast ('@');
	dom.cropafterlast ('@');
	
	
	// TODO: verify returnvalues; check permissions on created dirs
	// (module.xml says 640 which makes no sense for directories)
	hmdir = "/var/vmail/%s/" %format (dom);
	if (authd.makedir(hmdir))
	{
		sendresult (moderr::err_command, "mkdir failed");
		return false;
	}
	hmdir.strcat(us);
	
	if (authd.makedir(hmdir))
	{
		sendresult (moderr::err_command, "mkdir failed");
		return false;
	}
	
	mldir = hmdir;
	mldir.strcat ("/Maildir");
	
	if (authd.makedir(mldir))
	{
		sendresult (moderr::err_command, "mkdir failed");
		return false;
	}
	
	string mldirsub;
	mldirsub = mldir;
	mldirsub.strcat("/tmp");
	if (authd.makedir(mldirsub))
	{
		sendresult (moderr::err_command, "mkdir failed");
		return false;
	}
	mldirsub = mldir;
	mldirsub.strcat("/new");
	if (authd.makedir(mldirsub))
	{
		sendresult (moderr::err_command, "mkdir failed");
		return false;
	}
	mldirsub = mldir;
	mldirsub.strcat("/cur");
	if (authd.makedir(mldirsub))
	{
		sendresult (moderr::err_command, "mkdir failed");
		return false;
	}
	
	value rec; // Records for userdb.dat
	value srec; // Records for userdbshadow.dat
	
	rec = $("uid", pw["uid"]) ->
		  $("gid", pw["gid"]) ->
		  $("home", hmdir) ->
		  $("mail", mldir);
	
	// any services we do not cover are -allowed- by default in this setup
	// FIXME: make this configurable?
	srec = $("systempw", password) ->
		   $("imappw", imap ? password : "*") ->
		   $("smtppw", smtpauth ? password : "*") ->
		   $("pop3pw", pop3 ? password : "*");
		   
	// Write the new records to the userdb and userdbshadow
	DB.db[userid] = rec;
	DB.commit ();
	SDB.db[userid] = srec;
	SDB.commit ();
	
	DB.close ();
	SDB.close ();
	return true;
}

// ==========================================================================
// METHOD ::rewriteCourier
// ==========================================================================
bool postfixcourierModule::rewriteCourier(void)
{	
	gdbmfile DB;
	gdbmfile SDB;
	
	// Open the staging file for writing. Bail out on failure.
	file f;
	if (! f.openwrite ("userdb", 0600))
	{
		sendresult (moderr::err_command, "failed to open userdb");
		return false;
	}

	// Open the public database file. Bail out on failure.
	if (! DB.open ("userdb.dat"))
	{
		sendresult (moderr::err_command, "failed to open userdb.dat");
		f.close ();
		DB.close ();
		return false;
	}
	
	// Open the shadow database file. Bail out on failure.
	if (! SDB.open ("userdbshadow.dat"))
	{
		sendresult (moderr::err_command, "failed to open userdbshadow.dat");
		f.close ();
		DB.close ();
		return false;
	}
	
	// Make sure the shadow database is not world-readable.
	fs.chmod ("userdbshadow.dat", 0600);
	// Switch encoding to flat for the database dump.
	DB.setencoding (dbfile::flat);
	SDB.setencoding (dbfile::flat);
	
	// Prevent the shadowdb from growing to full size through queries.
	SDB.setcachesize (8);
	
	// Loop over the entries.
	foreach (dumpuser, DB.db)
	{
		f.printf ("%s\t%s|%s\n", dumpuser.id().str(),
				  dumpuser.cval(), SDB.db[dumpuser.id()].cval());
	}
	
	// Close all databases and files.
	f.close ();
	DB.close ();
	SDB.close ();
	
	if (authd.installfile("userdb", conf["config"]["courierpath"]) ||
		authd.installfile("userdb.dat", conf["config"]["courierpath"]) ||
		authd.installfile("userdbshadow.dat", conf["config"]["courierpath"]))
	{
		sendresult (moderr::err_command, "failed to install userdb files");
		return false;
	}
	
	return true;
}

// ==========================================================================
// METHOD ::delCourierUser
// ==========================================================================
bool postfixcourierModule::delCourierUser (const string &in_userid)
{
	gdbmfile DB;
	gdbmfile SDB;
	
	string userid = in_userid.lower();
	
	DB.setencoding (dbfile::courierdb);
	SDB.setencoding (dbfile::courierdb);
	
	// Open the public database file. Bail out on failure.
	if (! DB.open ("userdb.dat"))
	{
		sendresult (moderr::err_command, "failed to open userdb.dat");
		DB.close ();
		return false;
	}
	
	// Open the shadow database file. Bail out on failure.
	if (! SDB.open ("userdbshadow.dat"))
	{
		sendresult (moderr::err_command, "failed to open userdbshadow.dat");
		DB.close ();
		return false;
	}
	
	// Make sure the shadow database is not world-readable.
	fs.chmod ("userdbshadow.dat", 0600);
	
	// Write the new records to the userdb and userdbshadow
	DB.rmval(userid);
	DB.commit ();
	SDB.rmval(userid);
	SDB.commit ();
	
	DB.close ();
	SDB.close ();
	return true;
}

// ==========================================================================
// METHOD ::setPostfixDomain
// ==========================================================================
bool postfixcourierModule::setPostfixDomain (const string &in_domain)
{
	db4file DB;
	
	statstring domain = in_domain.lower();
	
	DB.setencoding (dbfile::flat);
	
	// Open the public database file. Bail out on failure.
	if (! DB.open ("virtual_mailbox_domains.db"))
	{
		sendresult (moderr::err_command, "failed to open virtual_mailbox_domains.db");
		DB.close ();
		return false;
	}
	
	// FIXME: sanity check domain (no newlines etc.)
	DB.db[domain] = (string) "VIRTUAL";
	DB.commit ();

	pf_rewrite_mailbox_domains = true;
	
	// Close all databases and files.
	DB.close ();
	return true;
}

// ==========================================================================
// METHOD ::delPostfixDomain
// ==========================================================================
bool postfixcourierModule::delPostfixDomain (const string &in_domain)
{
	db4file DB;
	
	string domain = in_domain.lower();
	
	DB.setencoding (dbfile::flat);
	
	// Open the public database file. Bail out on failure.
	if (! DB.open ("virtual_mailbox_domains.db"))
	{
		sendresult (moderr::err_command, "failed to open virtual_mailbox_domains.db");
		DB.close ();
		return false;
	}
	
	// FIXME: sanity check domain (no newlines etc.)
	DB.rmval(domain);
	DB.commit ();
	
	pf_rewrite_mailbox_domains = true;

	DB.close ();
	return true;
}

// ==========================================================================
// METHOD ::setRemoteHost
// ==========================================================================
bool postfixcourierModule::setRemoteHost (const string &in_domain, const string &in_remotehost)
{
	db4file DB;
	
	statstring domain = in_domain.lower();
	string remotehost = "smtp:";
	remotehost.strcat(in_remotehost);
	
	DB.setencoding (dbfile::flat);
	
	// Open the public database file. Bail out on failure.
	if (! DB.open ("transport_map.db"))
	{
		sendresult (moderr::err_command, "failed to open transport_map.db");
		DB.close ();
		return false;
	}
	
	// FIXME: sanity check domain (no newlines etc.)
	DB.db[domain] = remotehost;
	DB.commit ();

	pf_rewrite_transport_map = true;
	
	// Close all databases and files.
	DB.close ();
	return true;
}

// ==========================================================================
// METHOD ::delRemoteHost
// ==========================================================================
bool postfixcourierModule::delRemoteHost (const string &in_domain)
{
	db4file DB;
	
	string domain = in_domain.lower();
	
	DB.setencoding (dbfile::flat);
	
	// Open the public database file. Bail out on failure.
	if (! DB.open ("transport_map.db"))
	{
		sendresult (moderr::err_command, "failed to open transport_map.db");
		DB.close ();
		return false;
	}
	
	// FIXME: sanity check domain (no newlines etc.)
	DB.rmval(domain);
	DB.commit ();
	
	pf_rewrite_transport_map = true;

	DB.close ();
	return true;
}

// ==========================================================================
// METHOD ::setPostfixBox
// ==========================================================================
bool postfixcourierModule::setPostfixBox (const string &in_address)
{
	db4file DB;
	
	statstring address = in_address.lower();

	DB.setencoding (dbfile::flat);
	
	// Open the public database file. Bail out on failure.
	if (! DB.open ("virtual_mailbox.db"))
	{
		sendresult (moderr::err_command, "failed to open virtual_mailbox.db");
		DB.close ();
		return false;
	}
	
	// FIXME: sanity check address (no newlines etc.)
	DB.db[address] = (string) "virtual";
	DB.commit ();
	
	pf_rewrite_mailbox = true;
	// Close all databases and files.
	DB.close ();
	
	db4file ADB;
	ADB.setencoding (dbfile::flat);
	if (ADB.open ("virtual_alias.db"))
	{
		ADB.db[address] = (string) address;
		ADB.commit ();
		pf_rewrite_alias = true;
	}
	ADB.close ();
	return true;
}

// ==========================================================================
// METHOD ::delPostfixBox
// ==========================================================================
bool postfixcourierModule::delPostfixBox (const string &in_address)
{
	db4file DB;
	
	statstring address = in_address.lower();

	DB.setencoding (dbfile::flat);
	
	// Open the public database file. Bail out on failure.
	if (! DB.open ("virtual_mailbox.db"))
	{
		sendresult (moderr::err_command, "failed to open virtual_mailbox.db");
		DB.close ();
		return false;
	}
	
	// FIXME: sanity check address (no newlines etc.)
	DB.rmval (address);
	DB.commit ();
	
	pf_rewrite_mailbox = true;
	// Close all databases and files.
	DB.close ();
	
	db4file ADB;
	ADB.setencoding (dbfile::flat);
	if (ADB.open ("virtual_alias.db"))
	{
		ADB.rmval (address);
		ADB.commit ();
		pf_rewrite_alias = true;
	}
	ADB.close ();
	return true;
}

// ==========================================================================
// METHOD ::rewritePostfix
// ==========================================================================
bool postfixcourierModule::rewritePostfix (void)
{
	if(pf_rewrite_alias)
		if(!rewritePostfixFile("virtual_alias"))
			return false;
	if(pf_rewrite_mailbox)
		if(!rewritePostfixFile("virtual_mailbox"))
			return false;
	if(pf_rewrite_mailbox_domains)
		if(!rewritePostfixFile("virtual_mailbox_domains"))
			return false;
	if(pf_rewrite_transport_map)
		if(!rewritePostfixFile("transport_map"))
			return false;

	pf_rewrite_alias = pf_rewrite_mailbox = pf_rewrite_mailbox_domains = pf_rewrite_transport_map = false;

	return true;
}

// ==========================================================================
// METHOD ::rewritePostfixFile
// ==========================================================================
bool postfixcourierModule::rewritePostfixFile (const string &fname)
{
	string path;
	db4file DB;
	
	path = fname;
	
	// Open the staging file for writing. Bail out on failure.
	file f;
	if (! f.openwrite (path, 0600))
	{
		sendresult (moderr::err_command, "failed to open file in rewritePostfixFile()");
		return false;
	}
	
	string dbpath;
	dbpath = path;
	dbpath.strcat(".db");
	
	// Open the public database file. Bail out on failure.
	if (! DB.open (dbpath))
	{
		sendresult (moderr::err_command, "failed to open DB in rewritePostfixFile()");
		f.close ();
		return false;
	}
	
	// read flat plaintext data from db file
	DB.setencoding (dbfile::flat);
	
	// Loop over the entries.
	foreach (dumprow, DB.db)
	{
		f.printf ("%s\t%s\n", dumprow.id().str(), dumprow.cval());
	}
	
	f.close();
	DB.close();
	
	if(authd.installfile(path, conf["config"]["postfixpath"]) ||
	   authd.installfile(dbpath, conf["config"]["postfixpath"]))
	{
		sendresult (moderr::err_command, "authd installfile failure in rewritePostfixFile()");
		return false;
	}
	return true;
}

// ==========================================================================
// METHOD ::setPostfixAlias
// ==========================================================================
bool postfixcourierModule::setPostfixAlias (const string &in_address, const value &destlist)
{
	db4file DB;
	
	statstring address = in_address.lower();

	DB.setencoding (dbfile::valuelist);
	
	// Open the public database file. Bail out on failure.
	if (! DB.open ("virtual_alias.db"))
	{
		sendresult (moderr::err_command, "failed to open virtual_alias.db");
		DB.close ();
		return false;
	}
	
	// FIXME: sanity check address (no newlines etc.)
	DB.db[address] = destlist;
	DB.commit ();
	
	pf_rewrite_alias = true;

	// Close all databases and files.
	DB.close ();
	return true;
}

// ==========================================================================
// METHOD ::delPostfixAlias
// ==========================================================================
bool postfixcourierModule::delPostfixAlias (const string &in_address)
{
	db4file DB;
	
	statstring address = in_address.lower();

	DB.setencoding (dbfile::valuelist);
	
	// Open the public database file. Bail out on failure.
	if (! DB.open ("virtual_alias.db"))
	{
		sendresult (moderr::err_command, "failed to open virtual_alias.db");
		DB.close ();
		return false;
	}
	
	// FIXME: sanity check address (no newlines etc.)
	DB.rmval(address);
	DB.commit ();
	
	pf_rewrite_alias = true;

	// Close all databases and files.
	DB.close ();
	return true;
}
