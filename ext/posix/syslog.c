/***
@module posix
*/
/*
 * POSIX library for Lua 5.1/5.2.
 * (c) Gary V. Vaughan <gary@vaughan.pe>, 2013-2014
 * (c) Reuben Thomas <rrt@sc3d.org> 2010-2013
 * (c) Natanael Copa <natanael.copa@gmail.com> 2008-2010
 * Clean up and bug fixes by Leo Razoumov <slonik.az@gmail.com> 2006-10-11
 * Luiz Henrique de Figueiredo <lhf@tecgraf.puc-rio.br> 07 Apr 2006 23:17:49
 * Based on original by Claudio Terra for Lua 3.x.
 * With contributions by Roberto Ierusalimschy.
 * With documentation from Steve Donovan 2012
 */


#if _POSIX_VERSION >= 200112L

#include <config.h>

#include <syslog.h>

#include "_helpers.h"


/***
System Log Functions.
@section syslog
*/


/***
Open the system logger.
@function openlog
@string ident all messages will start with this
@string[opt] option any combination of 'c' (directly to system console if an error sending),
'n' (no delay) or 'p' (show PID)
@int[opt=`LOG_USER`] facility one of `LOG_AUTH`, `LOG_AUTHPRIV`, `LOG_CRON`, `LOG_DAEMON`, `LOG_FTP`, `LOG_KERN`, `LOG_LPR`, `LOG_MAIL`, `LOG_NEWS`, `LOG_SECURITY`, `LOG_SYSLOG`, 'LOG_USER`, `LOG_UUCP` or `LOG_LOCAL0` through `LOG_LOCAL7`
@see syslog(3)
*/
static int
Popenlog(lua_State *L)
{
	const char *ident = luaL_checkstring(L, 1);
	const char *s = optstring(L, 2, "");
	int facility = optint(L, 3, LOG_USER);
	int option = 0;

	checknargs(L, 3);
	while (*s)
	{
		switch (*s)
		{
			case ' ':	break;
			case 'c':	option |= LOG_CONS; break;
			case 'n':	option |= LOG_NDELAY; break;
			case 'p':	option |= LOG_PID; break;
			default:	badoption(L, 2, "openlog", *s); break;
		}
		s++;
	}
	openlog(ident, option, facility);
	return 0;
}


/***
Write to the system logger.
@function syslog
@int priority one of these values:

 * 1  Alert - immediate action
 * 2  Critcal
 * 3  Error
 * 4  Warning
 * 5  Notice
 * 6  Informational
 * 7  Debug

@string message log message
@see syslog(3)
*/
static int
Psyslog(lua_State *L)
{
	int priority = checkint(L, 1);
	const char *msg = luaL_checkstring(L, 2);
	checknargs(L, 2);
	syslog(priority, "%s", msg);
	return 0;
}

/***
Close system log.
@function closelog
@see syslog(3)
*/
static int
Pcloselog(lua_State *L)
{
	checknargs(L, 0);
	closelog();
	return 0;
}

/***
Set log priority mask.
@function setlogmask
@int ...  values from `LOG_EMERG`, `LOG_ALERT`, `LOG_CRIT`, `LOG_WARNING`, `LOG_NOTICE`, `LOG_INFO`, `LOG_DEBUG`
@return 0 on success, nil otherwise
@return error message if failed.
*/
static int
Psetlogmask(lua_State *L)
{
	int argno = lua_gettop(L);
	int mask = 0;
	int i;

	for (i=1; i <= argno; i++)
		mask |= LOG_MASK(optint(L, i, 0));  /*for `int or nil` error*/

	return pushresult(L, setlogmask(mask),"setlogmask");
}


static void
syslog_setconst(lua_State *L)
{
	PCONST( LOG_AUTH	);
	PCONST( LOG_AUTHPRIV	);
	PCONST( LOG_CRON	);
	PCONST( LOG_DAEMON	);
	PCONST( LOG_FTP		);
	PCONST( LOG_KERN	);
	PCONST( LOG_LOCAL0	);
	PCONST( LOG_LOCAL1	);
	PCONST( LOG_LOCAL2	);
	PCONST( LOG_LOCAL3	);
	PCONST( LOG_LOCAL4	);
	PCONST( LOG_LOCAL5	);
	PCONST( LOG_LOCAL6	);
	PCONST( LOG_LOCAL7	);
	PCONST( LOG_LPR		);
	PCONST( LOG_MAIL	);
	PCONST( LOG_NEWS	);
	PCONST( LOG_SYSLOG	);
	PCONST( LOG_USER	);
	PCONST( LOG_UUCP	);

	PCONST( LOG_EMERG	);
	PCONST( LOG_ALERT	);
	PCONST( LOG_CRIT	);
	PCONST( LOG_ERR		);
	PCONST( LOG_WARNING	);
	PCONST( LOG_NOTICE	);
	PCONST( LOG_INFO	);
	PCONST( LOG_DEBUG	);
}

#endif
