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
 File Status Querying and Setting.

@module posix.sys.stat
*/

#include <config.h>

#include <sys/stat.h>

#include "_helpers.c"


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


static const char *
filetype(mode_t m)
{
	if (S_ISREG(m))
		return "regular";
	else if (S_ISLNK(m))
		return "link";
	else if (S_ISDIR(m))
		return "directory";
	else if (S_ISCHR(m))
		return "character device";
	else if (S_ISBLK(m))
		return "block device";
	else if (S_ISFIFO(m))
		return "fifo";
	else if (S_ISSOCK(m))
		return "socket";
	else
		return "?";
}


static void
Fstat(lua_State *L, int i, const void *data)
{
	const struct stat *s=data;
	switch (i)
	{
		case 0:
			pushmode(L, s->st_mode);
			break;
		case 1:
			lua_pushinteger(L, s->st_ino);
			break;
		case 2:
			lua_pushinteger(L, s->st_dev);
			break;
		case 3:
			lua_pushinteger(L, s->st_nlink);
			break;
		case 4:
			lua_pushinteger(L, s->st_uid);
			break;
		case 5:
			lua_pushinteger(L, s->st_gid);
			break;
		case 6:
			lua_pushinteger(L, s->st_size);
			break;
		case 7:
			lua_pushinteger(L, s->st_atime);
			break;
		case 8:
			lua_pushinteger(L, s->st_mtime);
			break;
		case 9:
			lua_pushinteger(L, s->st_ctime);
			break;
		case 10:
			lua_pushstring(L, filetype(s->st_mode));
			break;
	}
}


static const char *const Sstat[] =
{
	"mode", "ino", "dev", "nlink", "uid", "gid",
	"size", "atime", "mtime", "ctime", "type",
	NULL
};


/***
Information about an existing file path.
If file is a symbolic link return information about the link itself.
@function lstat
@string path file to act on
@string ... field names, each one of "mode", "ino", "dev", "nlink", "uid", "gid",
"size", "atime", "mtime", "ctime", "type"
@return ... values, or table of all fields if no option given
@see lstat(2)
@see stat
@usage for a, b in pairs(P.lstat("/etc/")) do print(a, b) end
*/
static int
Plstat(lua_State *L)
{
	struct stat s;
	const char *path=luaL_checkstring(L, 1);
	if (lstat(path,&s)==-1)
		return pusherror(L, path);
	return doselection(L, 2, Sstat, Fstat, &s);
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
Information about an existing file path.
If file is a symbolic link return information about the file the link points to.
@function stat
@string path file to act on
@string ... field names, each one of "mode", "ino", "dev", "nlink", "uid", "gid",
"size", "atime", "mtime", "ctime", "type"
@return ... values, or table of all fields if no option given
@see stat(2)
@see lstat
@usage for a, b in pairs(P.stat("/etc/")) do print(a, b) end
*/
static int
Pstat(lua_State *L)
{
	struct stat s;
	const char *path=luaL_checkstring(L, 1);
	if (stat(path,&s)==-1)
		return pusherror(L, path);
	return doselection(L, 2, Sstat, Fstat, &s);
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


static const luaL_Reg posix_sys_stat_fns[] =
{
	LPOSIX_FUNC( Pchmod		),
	LPOSIX_FUNC( Plstat		),
	LPOSIX_FUNC( Pmkdir		),
	LPOSIX_FUNC( Pmkfifo		),
	LPOSIX_FUNC( Pstat		),
	LPOSIX_FUNC( Pumask		),
	{NULL, NULL}
};


LUALIB_API int
luaopen_posix_sys_stat(lua_State *L)
{
	luaL_register(L, "posix.sys.stat", posix_sys_stat_fns);
	lua_pushliteral(L, "posix.sys.stat for " LUA_VERSION " / " PACKAGE_STRING);
	lua_setfield(L, -2, "version");

	return 1;
}
