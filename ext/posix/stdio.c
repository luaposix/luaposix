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

#include <config.h>

#include <stdlib.h>
#if HAVE_SYS_STATVFS_H
#  include <sys/statvfs.h>
#endif
#include <sys/time.h>

#include "_helpers.h"


/***
Standard IO Functions.
@section stdio
*/


/***
Name of controlling terminal.
@function ctermid
@treturn string controlling terminal for current process
@see ctermid(3)
*/
static int
Pctermid(lua_State *L)
{
	char b[L_ctermid];
	checknargs(L, 0);
	lua_pushstring(L, ctermid(b));
	return 1;
}


/***
File descriptor corresponding to a Lua file object.
@function fileno
@tparam file file Lua file object
@treturn[1] int file descriptor for *file*, if successful
@return[2] nil
@treturn[2] string error message
@usage
STDOUT_FILENO = P.fileno (io.stdout)
*/
static int
Pfileno(lua_State *L)
{
	FILE *f = *(FILE**) luaL_checkudata(L, 1, LUA_FILEHANDLE);
	checknargs(L, 1);
	return pushresult(L, fileno(f), NULL);
}



static void
stdio_setconst(lua_State *L)
{
	/* stdio.h constants */
	/* Those that are omitted already have a Lua interface, or alternative. */
	PCONST( _IOFBF		);
	PCONST( _IOLBF		);
	PCONST( _IONBF		);
	PCONST( BUFSIZ		);
	PCONST( EOF		);
	PCONST( FOPEN_MAX	);
	PCONST( FILENAME_MAX	);
}
