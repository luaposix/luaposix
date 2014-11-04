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
/***
 A few Standard I/O functions not already in Lua core.

@module posix.stdio
*/

#include <config.h>

#include <stdio.h>

#include "_helpers.c"


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
	return pushstringresult(ctermid(b));
}


/***
File descriptor corresponding to a Lua file object.
@function fileno
@tparam file file Lua file object
@treturn[1] int file descriptor for *file*, if successful
@return[2] nil
@treturn[2] string error message
@treturn[2] int errnum
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


static const luaL_Reg posix_stdio_fns[] =
{
	LPOSIX_FUNC( Pctermid		),
	LPOSIX_FUNC( Pfileno		),
	{NULL, NULL}
};


/***
Constants.
@section constants
*/

/***
Stdio constants.
Any constants not available in the underlying system will be `nil` valued.
@table posix.stdio
@int _IOFBF fully buffered
@int _IOLBF line buffered
@int _IONBF unbuffered
@int BUFSIZ size of buffer
@int EOF end of file
@int FOPEN_MAX maximum number of open files
@int FILENAME_MAX maximum length of filename
@usage
  -- Print stdio constants supported on this host.
  for name, value in pairs (require "posix.stdio") do
    if type (value) == "number" then
      print (name, value)
     end
  end
*/

LUALIB_API int
luaopen_posix_stdio(lua_State *L)
{
	luaL_register(L, "posix.stdio", posix_stdio_fns);
	lua_pushliteral(L, "posix.stdio for " LUA_VERSION " / " PACKAGE_STRING);
	lua_setfield(L, -2, "version");

	/* stdio.h constants */
	/* Those that are omitted already have a Lua interface, or alternative. */
	LPOSIX_CONST( _IOFBF		);
	LPOSIX_CONST( _IOLBF		);
	LPOSIX_CONST( _IONBF		);
	LPOSIX_CONST( BUFSIZ		);
	LPOSIX_CONST( EOF		);
	LPOSIX_CONST( FOPEN_MAX		);
	LPOSIX_CONST( FILENAME_MAX	);

	return 1;
}
