/*
 * POSIX library for Lua 5.1, 5.2, 5.3 & 5.4.
 * Copyright (C) 2013-2023 Gary V. Vaughan
 * Copyright (C) 2010-2013 Reuben Thomas <rrt@sc3d.org>
 * Copyright (C) 2008-2010 Natanael Copa <natanael.copa@gmail.com>
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

#include <sys/stat.h>

#include "_helpers.c"


/***
File state record.
@table PosixStat
@int st_dev device id
@int st_ino inode number
@int st_mode mode of file
@int st_nlink number of hardlinks to file
@int st_uid user id of file owner
@int st_gid group id of file owner
@int st_rdev additional device specific id for special files
@int st_size file size in bytes
@int st_atime time of last access
@int st_mtime time of last data modification
@int st_ctime time of last state change
@int st_blksize preferred block size
@int st_blocks number of blocks allocated
*/
static int
pushstat(lua_State *L, struct stat *st)
{
	if (!st)
		return lua_pushnil(L), 1;

	lua_createtable(L, 0, 13);

	setintegerfield(st, st_dev);
	setintegerfield(st, st_ino);
	setintegerfield(st, st_mode);
	setintegerfield(st, st_nlink);
	setintegerfield(st, st_uid);
	setintegerfield(st, st_gid);
	setintegerfield(st, st_rdev);
	setintegerfield(st, st_size);
	setintegerfield(st, st_blksize);
	setintegerfield(st, st_blocks);

	/* st_[amc]time is a macro on at least Mac OS, so we have to
	   assign field name strings manually. */
        pushintegerfield("st_atime", st->st_atime);
        pushintegerfield("st_mtime", st->st_mtime);
        pushintegerfield("st_ctime", st->st_ctime);

	settypemetatable("PosixStat");
	return 1;
}


/***
Test for a block special file.
@function S_ISBLK
@int mode the st_mode field of a @{PosixStat}
@treturn int non-zero if *mode* represents a block special file
*/
static int
PS_ISBLK(lua_State *L)
{
	checknargs(L, 1);
	return pushintegerresult(S_ISBLK((mode_t)checkinteger(L, 1)));
}


/***
Test for a character special file.
@function S_ISCHR
@int mode the st_mode field of a @{PosixStat}
@treturn int non-zero if *mode* represents a character special file
*/
static int
PS_ISCHR(lua_State *L)
{
	checknargs(L, 1);
	return pushintegerresult(S_ISCHR((mode_t)checkinteger(L, 1)));
}


/***
Test for a directory.
@function S_ISDIR
@int mode the st_mode field of a @{PosixStat}
@treturn int non-zero if *mode* represents a directory
*/
static int
PS_ISDIR(lua_State *L)
{
	checknargs(L, 1);
	return pushintegerresult(S_ISDIR((mode_t)checkinteger(L, 1)));
}


/***
Test for a fifo special file.
@function S_ISFIFO
@int mode the st_mode field of a @{PosixStat}
@treturn int non-zero if *mode* represents a fifo special file
*/
static int
PS_ISFIFO(lua_State *L)
{
	checknargs(L, 1);
	return pushintegerresult(S_ISFIFO((mode_t)checkinteger(L, 1)));
}


/***
Test for a symbolic link.
@function S_ISLNK
@int mode the st_mode field of a @{PosixStat}
@treturn int non-zero if *mode* represents a symbolic link
*/
static int
PS_ISLNK(lua_State *L)
{
	checknargs(L, 1);
	return pushintegerresult(S_ISLNK((mode_t)checkinteger(L, 1)));
}


/***
Test for a regular file.
@function S_ISREG
@int mode the st_mode field of a @{PosixStat}
@treturn int non-zero if *mode* represents a regular file
*/
static int
PS_ISREG(lua_State *L)
{
	checknargs(L, 1);
	return pushintegerresult(S_ISREG((mode_t)checkinteger(L, 1)));
}


/***
Test for a socket.
@function S_ISSOCK
@int mode the st_mode field of a @{PosixStat}
@treturn int non-zero if *mode* represents a socket
*/
static int
PS_ISSOCK(lua_State *L)
{
	checknargs(L, 1);
	return pushintegerresult(S_ISSOCK((mode_t)checkinteger(L, 1)));
}


/***
Change the mode of the path.
@function chmod
@string path existing file path to act on
@int mode access modes to set for *path*
@treturn[1] int `0`, if successful
@return[2] nil
@treturn[2] string error message
@treturn[2] int errnum
@see chmod(2)
@usage
  local sys_stat = require "posix.sys.stat"
  sys_stat.chmod ('bin/dof', bit.bor (sys_stat.S_IRWXU, sys_stat.S_IRGRP))
*/
static int
Pchmod(lua_State *L)
{
	const char *path = luaL_checkstring(L, 1);
	checknargs(L, 2);
	return pushresult(L, chmod(path, (mode_t)checkinteger(L, 2)), path);
}


/***
Information about an existing file path.
If file is a symbolic link, return information about the link itself.
@function lstat
@string path file to act on
@treturn[1] PosixStat information about *path*, if successful
@return[2] nil
@treturn[2] string error message
@treturn[2] int errnum
@see lstat(2)
@see stat
@usage
  local sys_stat = require "posix.sys.stat"
  for a, b in pairs (sys_stat.lstat "/etc/") do print (a, b) end
*/
static int
Plstat(lua_State *L)
{
	struct stat s;
	const char *path = luaL_checkstring(L, 1);
	checknargs(L, 1);
	if (lstat(path, &s) == -1)
		return pusherror(L, path);
	return pushstat(L, &s);
}


/***
Information about a file descriptor.
@function fstat
@int fd file descriptor to act on
@treturn[1] PosixStat information about *fd*, if successful
@return[2] nil
@treturn[2] string error message
@treturn[2] int errnum
@see fstat(2)
@see stat
@usage
  local fcntl = require "posix.fcntl"
  local sys_stat = require "posix.sys.stat"
  local fd = assert(fcntl.open("/etc/hostname", fcntl.O_RDONLY))
  for a, b in pairs (sys_stat.fstat(fd)) do print (a, b) end
*/
static int
Pfstat(lua_State *L)
{
	struct stat s;
	int fd = checkint(L, 1);
	checknargs(L, 1);
	if (fstat(fd, &s) == -1)
		return pusherror(L, "fstat");
	return pushstat(L, &s);
}


/***
Make a directory.
@function mkdir
@string path location in file system to create directory
@int[opt=511] mode access modes to set for *path*
@treturn[1] int `0`, if successful
@return[2] nil
@treturn[2] string error message
@treturn[2] int errnum
@see mkdir(2)
*/
static int
Pmkdir(lua_State *L)
{
	const char *path = luaL_checkstring(L, 1);
	checknargs(L, 2);
	return pushresult(L, mkdir(path, (mode_t)optinteger(L, 2, 0777)), path);
}


/***
Make a FIFO pipe.
@function mkfifo
@string path location in file system to create fifo
@int[opt=511] mode access modes to set for *path*
@treturn[1] int file descriptor for *path*, if successful
@return[2] nil
@treturn[2] string error message
@treturn[2] int errnum
@see mkfifo(2)
*/
static int
Pmkfifo(lua_State *L)
{
	const char *path = luaL_checkstring(L, 1);
	checknargs(L, 2);
	return pushresult(L, mkfifo(path, (mode_t)optinteger(L, 2, 0777)), path);
}


/***
Information about an existing file path.
If file is a symbolic link, return information about the file the link points to.
@function stat
@string path file to act on
@treturn[1] PosixStat information about *path*, if successful
@return[2] nil
@treturn[2] string error message
@treturn[2] int errnum
@see stat(2)
@see lstat
@usage
  local sys_stat = require "posix.sys.stat"
  for a, b in pairs (sys_stat.stat "/etc/") do print (a, b) end
*/
static int
Pstat(lua_State *L)
{
	struct stat s;
	const char *path = luaL_checkstring(L, 1);
	checknargs(L, 1);
	if (stat(path, &s) == -1)
		return pusherror(L, path);
	return pushstat(L, &s);
}


/***
Set file mode creation mask.
@function umask
@int mode new file creation mask
@treturn int previous umask
@see umask(2)
@see posix.umask
*/
static int
Pumask(lua_State *L)
{
	checknargs(L, 1);
	return pushintegerresult(umask((mode_t)checkinteger(L, 1)));
}


static const luaL_Reg posix_sys_stat_fns[] =
{
	LPOSIX_FUNC( PS_ISBLK		),
	LPOSIX_FUNC( PS_ISCHR		),
	LPOSIX_FUNC( PS_ISDIR		),
	LPOSIX_FUNC( PS_ISFIFO		),
	LPOSIX_FUNC( PS_ISLNK		),
	LPOSIX_FUNC( PS_ISREG		),
	LPOSIX_FUNC( PS_ISSOCK		),
	LPOSIX_FUNC( Pchmod		),
	LPOSIX_FUNC( Plstat		),
	LPOSIX_FUNC( Pfstat		),
	LPOSIX_FUNC( Pmkdir		),
	LPOSIX_FUNC( Pmkfifo		),
	LPOSIX_FUNC( Pstat		),
	LPOSIX_FUNC( Pumask		),
	{NULL, NULL}
};


/***
Constants.
@section constants
*/

/***
Stat constants.
Any constants not available in the underlying system will be `nil` valued.
@table posix.sys.stat
@int S_IFMT file type mode bitmask
@int S_IFBLK block special
@int S_IFCHR character special
@int S_IFDIR directory
@int S_IFIFO fifo
@int S_IFLNK symbolic link
@int S_IFREG regular file
@int S_IFSOCK socket
@int S_IRWXU user read, write and execute
@int S_IRUSR user read
@int S_IWUSR user write
@int S_IXUSR user execute
@int S_IRWXG group read, write and execute
@int S_IRGRP group read
@int S_IWGRP group write
@int S_IXGRP group execute
@int S_IRWXO other read, write and execute
@int S_IROTH other read
@int S_IWOTH other write
@int S_IXOTH other execute
@int S_ISGID set group id on execution
@int S_ISUID set user id on execution
@usage
  -- Print stat constants supported on this host.
  for name, value in pairs (require "posix.sys.stat") do
    if type (value) == "number" then
      print (name, value)
     end
  end
*/


LUALIB_API int
luaopen_posix_sys_stat(lua_State *L)
{
	luaL_newlib(L, posix_sys_stat_fns);
	lua_pushstring(L, LPOSIX_VERSION_STRING("sys.stat"));
	lua_setfield(L, -2, "version");

	LPOSIX_CONST( S_IFMT		);
	LPOSIX_CONST( S_IFBLK		);
	LPOSIX_CONST( S_IFCHR		);
	LPOSIX_CONST( S_IFDIR		);
	LPOSIX_CONST( S_IFIFO		);
	LPOSIX_CONST( S_IFLNK		);
	LPOSIX_CONST( S_IFREG		);
	LPOSIX_CONST( S_IFSOCK		);
	LPOSIX_CONST( S_IRWXU		);
	LPOSIX_CONST( S_IRUSR		);
	LPOSIX_CONST( S_IWUSR		);
	LPOSIX_CONST( S_IXUSR		);
	LPOSIX_CONST( S_IRWXG		);
	LPOSIX_CONST( S_IRGRP		);
	LPOSIX_CONST( S_IWGRP		);
	LPOSIX_CONST( S_IXGRP		);
	LPOSIX_CONST( S_IRWXO		);
	LPOSIX_CONST( S_IROTH		);
	LPOSIX_CONST( S_IWOTH		);
	LPOSIX_CONST( S_IXOTH		);
	LPOSIX_CONST( S_ISGID		);
	LPOSIX_CONST( S_ISUID		);

	return 1;
}
