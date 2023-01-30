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
 File Control.

 Low-level control over file descriptors, including creating new file
 descriptors with `open`.

@module posix.fcntl
*/

#include "_helpers.c"

#include <fcntl.h>


/* Darwin fails to define O_RSYNC. */
#ifndef O_RSYNC
#define O_RSYNC 0
#endif
/* FreeBSD 10 fails to define O_DSYNC. */
#ifndef O_DSYNC
#define O_DSYNC 0
#endif
/* POSIX.2001 uses FD_CLOEXEC instead. */
#ifndef O_CLOEXEC
#define O_CLOEXEC 0
#endif



/***
Advisory file locks.
Passed as *arg* to @{fcntl} when *cmd* is `F_GETLK`, `F_SETLK` or `F_SETLKW`.
@table flock
@int l_start starting offset
@int l_len len = 0 means until end of file
@int l_pid lock owner
@int l_type lock type
@int l_whence one of `SEEK_SET`, `SEEK_CUR` or `SEEK_END`
*/


/***
Manipulate file descriptor.
@function fcntl
@int fd file descriptor to act on
@int cmd operation to perform
@tparam[opt=0] int|flock arg when *cmd* is `F_GETLK`, `F_SETLK` or `F_SETLKW`,
  then *arg* is a @{flock} table, otherwise an integer with meaning dependent
  upon the value of *cmd*.
@return[1] integer return value depending on *cmd*, if successful
@return[2] nil
@treturn[2] string error message
@treturn[2] int errnum
@see fcntl(2)
@see lock.lua
@usage
local flag = posix.fcntl (fd, posix.F_GETFL)
*/
static int
Pfcntl(lua_State *L)
{
	int fd = checkint(L, 1);
	int cmd = checkint(L, 2);
	int arg;
	struct flock lockinfo;
	int r;
	checknargs(L, 3);
	switch (cmd)
	{
		case F_SETLK:
		case F_SETLKW:
		case F_GETLK:
			luaL_checktype(L, 3, LUA_TTABLE);

			/* Copy fields to flock struct */
			lua_getfield(L, 3, "l_type");
			lockinfo.l_type = (short)lua_tointeger(L, -1);
			lua_getfield(L, 3, "l_whence");
			lockinfo.l_whence = (short)lua_tointeger(L, -1);
			lua_getfield(L, 3, "l_start");
			lockinfo.l_start = (off_t)lua_tointeger(L, -1);
			lua_getfield(L, 3, "l_len");
			lockinfo.l_len = (off_t)lua_tointeger(L, -1);

			/* Lock */
			r = fcntl(fd, cmd, &lockinfo);

			/* Copy fields from flock struct */
			lua_pushinteger(L, lockinfo.l_type);
			lua_setfield(L, 3, "l_type");
			lua_pushinteger(L, lockinfo.l_whence);
			lua_setfield(L, 3, "l_whence");
			lua_pushinteger(L, lockinfo.l_start);
			lua_setfield(L, 3, "l_start");
			lua_pushinteger(L, lockinfo.l_len);
			lua_setfield(L, 3, "l_len");
			lua_pushinteger(L, lockinfo.l_pid);
			lua_setfield(L, 3, "l_pid");

			break;
		default:
			arg = optint(L, 3, 0);
			r = fcntl(fd, cmd, arg);
			break;
	}
	return pushresult(L, r, "fcntl");
}


/***
Open a file.
@function open
@string path
@int oflags bitwise OR of zero or more of `O_RDONLY`, `O_WRONLY`, `O_RDWR`,
  `O_APPEND`, `O_CREAT`, `O_DSYNC`, `O_EXCL`, `O_NOCTTY`, `O_NONBLOCK`,
  `O_RSYNC`, `O_SYNC` and `O_TRUNC` (and `O_CLOEXEC`, where supported)
@int[opt=511] mode access modes used by `O_CREAT`
@treturn[1] int file descriptor for *path*, if successful
@return[2] nil
@treturn[2] string error message
@treturn[2] int errnum
@see open(2)
@usage
local P = require "posix.fcntl"
fd = P.open ("data", bit.bor (P.O_CREAT, P.O_RDWR), bit.bor (P.S_IRWXU, P.S_IRGRP))
*/
static int
Popen(lua_State *L)
{
	const char *path = luaL_checkstring(L, 1);
	int oflags = checkint(L, 2);
	checknargs(L, 3);
	return pushresult(L, open(path, oflags, (mode_t)optinteger(L, 3, 511)), path);
}


#if HAVE_POSIX_FADVISE
/***
Instruct kernel on appropriate cache behaviour for a file or file segment.
@function posix_fadvise
@int fd open file descriptor
@int offset start of region
@int len number of bytes in region
@int advice one of `POSIX_FADV_NORMAL`, `POSIX_FADV_SEQUENTIAL`,
  `POSIX_FADV_RANDOM`, `POSIX_FADV_NOREUSE`, `POSIX_FADV_WILLNEED` or
  `POSIX_FADV_DONTNEED`
@treturn[1] int `0`, if successful
@return[2] nil
@treturn[2] string error message
@treturn[2] int errnum
@see posix_fadvise(2)
*/
static int
Pposix_fadvise(lua_State *L)
{
	int fd       = checkint(L, 1);
	off_t offset = (off_t)checkinteger(L, 2);
	off_t len    = (off_t)checkinteger(L, 3);
	int advice   = checkint(L, 4);
	int r;
	checknargs(L, 4);
	r = posix_fadvise(fd, offset, len, advice);
	return pushresult(L, r == 0 ? 0 : -1, "posix_fadvise");
}
#endif


static const luaL_Reg posix_fcntl_fns[] =
{
	LPOSIX_FUNC( Pfcntl		),
	LPOSIX_FUNC( Popen		),
#if HAVE_POSIX_FADVISE
	LPOSIX_FUNC( Pposix_fadvise	),
#endif
	{NULL, NULL}
};


/***
Constants.
@section constants
*/

/***
Fcntl constants.
Any constants not available in the underlying system will be `0` valued,
if they are usually bitwise ORed with other values, otherwise `nil`.
@table posix.fcntl
@int AT_EACCESS test access permitted for effective IDs, not real IDs
@int AT_FDCWD indicate *at functions to use current directory
@int AT_REMOVEDIR remove directory instead of unlinking file
@int AT_SYMLINK_FOLLOW follow symbolic links
@int AT_SYMLINK_NOFOLLOW do not follow symbolic links
@int FD_CLOEXEC close file descriptor on exec flag
@int F_DUPFD duplicate file descriptor
@int F_GETFD get file descriptor flags
@int F_SETFD set file descriptor flags
@int F_GETFL get file status flags
@int F_SETFL set file status flags
@int F_GETLK get record locking information
@int F_SETLK set record locking information
@int F_SETLKW set lock, and wait if blocked
@int F_GETOWN get SIGIO/SIGURG process owner
@int F_SETOWN set SIGIO/SIGURG process owner
@int F_RDLCK shared or read lock
@int F_WRLCK exclusive or write lock
@int F_UNLCK unlock
@int O_RDONLY open for reading only
@int O_WRONLY open for writing only
@int O_RDWR open for reading and writing
@int O_APPEND set append mode
@int O_CLOEXEC set FD_CLOEXEC atomically
@int O_CREAT create if nonexistent
@int O_DSYNC synchronise io data integrity
@int O_EXCL error if file already exists
@int O_NOCTTY don't assign controlling terminal
@int O_NONBLOCK no delay
@int O_RSYNC synchronise file read integrity
@int O_SYNC synchronise file write integrity
@int O_TRUNC truncate to zero length
@int POSIX_FADV_NORMAL no advice
@int POSIX_FADV_SEQUENTIAL expecting to access data sequentially
@int POSIX_FADV_RANDOM expecting to access data randomly
@int POSIX_FADV_NOREUSE expecting to access data once only
@int POSIX_FADV_WILLNEED expecting to access data in the near future
@int POSIX_FADV_DONTNEED not expecting to access the data in the near future
@usage
  -- Print fcntl constants supported on this host.
  for name, value in pairs (require "posix.fcntl") do
    if type (value) == "number" then
      print (name, value)
     end
  end
*/

LUALIB_API int
luaopen_posix_fcntl(lua_State *L)
{
	luaL_newlib(L, posix_fcntl_fns);
	lua_pushstring(L, LPOSIX_VERSION_STRING("fcntl"));
	lua_setfield(L, -2, "version");

	/* posix *at flags */
#ifdef AT_EACCESS
	LPOSIX_CONST( AT_EACCESS		);
#endif
#ifdef AT_FDCWD
	LPOSIX_CONST( AT_FDCWD			);
#endif
#ifdef AT_REMOVEDIR
	LPOSIX_CONST( AT_REMOVEDIR		);
#endif
#ifdef AT_SYMLINK_FOLLOW
	LPOSIX_CONST( AT_SYMLINK_FOLLOW		);
#endif
#ifdef AT_SYMLINK_NOFOLLOW
	LPOSIX_CONST( AT_SYMLINK_NOFOLLOW	);
#endif
	/* Linux-specific *at flags */
#ifdef AT_EMPTY_PATH
	LPOSIX_CONST( AT_EMPTY_PATH	);
#endif
#ifdef AT_NO_AUTOMOUNT
	LPOSIX_CONST( AT_NO_AUTOMOUNT	);
#endif

	/* fcntl flags */
	LPOSIX_CONST( FD_CLOEXEC	);
	LPOSIX_CONST( F_DUPFD		);
	LPOSIX_CONST( F_GETFD		);
	LPOSIX_CONST( F_SETFD		);
	LPOSIX_CONST( F_GETFL		);
	LPOSIX_CONST( F_SETFL		);
	LPOSIX_CONST( F_GETLK		);
	LPOSIX_CONST( F_SETLK		);
	LPOSIX_CONST( F_SETLKW		);
	LPOSIX_CONST( F_GETOWN		);
	LPOSIX_CONST( F_SETOWN		);
	LPOSIX_CONST( F_RDLCK		);
	LPOSIX_CONST( F_WRLCK		);
	LPOSIX_CONST( F_UNLCK		);

	/* file creation & status flags */
	LPOSIX_CONST( O_RDONLY		);
	LPOSIX_CONST( O_WRONLY		);
	LPOSIX_CONST( O_RDWR		);
	LPOSIX_CONST( O_APPEND		);
	LPOSIX_CONST( O_CREAT		);
	LPOSIX_CONST( O_DSYNC		);
	LPOSIX_CONST( O_EXCL		);
	LPOSIX_CONST( O_NOCTTY		);
	LPOSIX_CONST( O_NONBLOCK	);
	LPOSIX_CONST( O_RSYNC		);
	LPOSIX_CONST( O_SYNC		);
	LPOSIX_CONST( O_TRUNC		);
	LPOSIX_CONST( O_CLOEXEC		);

	/* Linux 3.11 and above */
#ifdef O_TMPFILE
	LPOSIX_CONST( O_TMPFILE		);
#endif

	/* posix_fadvise flags */
#ifdef POSIX_FADV_NORMAL
	LPOSIX_CONST( POSIX_FADV_NORMAL		);
#endif
#ifdef POSIX_FADV_SEQUENTIAL
	LPOSIX_CONST( POSIX_FADV_SEQUENTIAL	);
#endif
#ifdef POSIX_FADV_RANDOM
	LPOSIX_CONST( POSIX_FADV_RANDOM		);
#endif
#ifdef POSIX_FADV_NOREUSE
	LPOSIX_CONST( POSIX_FADV_NOREUSE	);
#endif
#ifdef POSIX_FADV_WILLNEED
	LPOSIX_CONST( POSIX_FADV_WILLNEED	);
#endif
#ifdef POSIX_FADV_DONTNEED
	LPOSIX_CONST( POSIX_FADV_DONTNEED	);
#endif

	return 1;
}
