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

#include <sys/stat.h>

#include "_helpers.h"


/***
System Status Functions.
@section sysstat
*/


/***
Change the mode of the path.
@function chmod
@string path existing file path
@string mode one of the following formats:

 * "rwxrwxrwx" (e.g. "rw-rw-w--")
 * "ugoa+-=rwx" (e.g. "u+w")
 * "+-=rwx" (e.g. "+w")

@treturn[1] int `0`, if successful
@return[2] nil
@treturn[2] string error message
@see chmod(2)
@usage P.chmod('bin/dof','+x')
*/
static int
Pchmod(lua_State *L)
{
	mode_t mode;
	struct stat s;
	const char *path = luaL_checkstring(L, 1);
	const char *modestr = luaL_checkstring(L, 2);
	checknargs(L, 2);
	if (stat(path, &s))
		return pusherror(L, path);
	mode = s.st_mode;
	if (mode_munch(&mode, modestr))
		luaL_argerror(L, 2, "bad mode");
	return pushresult(L, chmod(path, mode), path);
}


/***
Make a directory.
@function mkdir
@string path
@treturn[1] int `0`, if successful
@return[2] nil
@treturn[2] string error message
@see mkdir(2)
*/
static int
Pmkdir(lua_State *L)
{
	const char *path = luaL_checkstring(L, 1);
	checknargs(L, 1);
	return pushresult(L, mkdir(path, 0777), path);
}


/***
Make a FIFO pipe.
@function mkfifo
@string path location in file system to create fifo
@treturn[1] int file descriptor for *path*, if successful
@return[2] nil
@treturn[2] string error message
@see mkfifo(2)
*/
static int
Pmkfifo(lua_State *L)
{
	const char *path = luaL_checkstring(L, 1);
	checknargs(L, 1);
	return pushresult(L, mkfifo(path, 0777), path);
}


/***
Set file mode creation mask.
@function umask
@string[opt] mode file creation mask string (see @{chmod} for format)
@treturn int previous umask
@see umask(2)
*/
static int
Pumask(lua_State *L)
{
	const char *modestr = optstring(L, 1, NULL);
	mode_t mode;
	checknargs (L, 1);
	umask(mode=umask(0));
	mode=(~mode)&0777;
	if (modestr)
	{
		if (mode_munch(&mode, modestr))
		        luaL_argerror(L, 1, "bad mode");
		mode&=0777;
		umask(~mode);
	}
	pushmode(L, mode);
	return 1;
}
