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
 System error codes and messages.

 Usually, you'll be able to work with the error messages returned as the
 second value from failed calls directly, without having to manually save
 and stringify the system messages as you would in C.  For completeness,
 the functions are still available here.

@module posix.errno
*/

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "_helpers.c"

#ifndef errno
extern int errno;
#endif


/***
Describe an error code/and or read `errno`
@function errno
@int[opt=current errno] n optional error code
@return description
@return error code
@see strerror(3)
@see errno
@usage
local strerr, nerr = posix.errno ()
*/
static int
Perrno(lua_State *L)
{
	int n = optint(L, 1, errno);
	checknargs(L, 1);
	lua_pushstring(L, strerror(n));
	lua_pushinteger(L, n);
	return 2;
}


/***
Set errno.
@function set_errno
@int n error code
@see errno(3)
@usage
posix.errno (posix.EBADF)
*/
static int
Pset_errno(lua_State *L)
{
	errno = checkint(L, 1);
	checknargs(L, 1);
	return 0;
}


static const luaL_Reg posix_errno_fns[] =
{
	LPOSIX_FUNC( Perrno		),
	LPOSIX_FUNC( Pset_errno		),
	{NULL, NULL}
};


/***
Constants.
@section constants
*/

/***
Error constants.
Any constants not available in the underlying system will be `nil` valued.

If you find one of the luaposix APIs returns an error code not listed here,
please raise an issue [here](http://github.com/luaposix/luaposixissues), stating
the symbolic name of the constant (from `/usr/include/errno.h` or equivalent).
@table posix.errno
@int E2BIG argument list too long
@int EACCES permission denied
@int EADDRINUSE address already in use
@int EADDRNOTAVAIL can't assign requested address
@int EAFNOSUPPORT address family not supported by protocol family
@int EAGAIN resource temporarily unavailable
@int EALREADY operation already in progress
@int EBADF bad file descriptor
@int EBADMSG bad message
@int EBUSY resource busy
@int ECANCELED operation canceled
@int ECHILD no child processes
@int ECONNABORTED software caused connection abort
@int ECONNREFUSED connection refused
@int ECONNRESET connection reset by peer
@int EDEADLK resource deadlock avoided
@int EDESTADDRREQ destination address required
@int EDOM numerical argument out of domain
@int EEXIST file exists
@int EFAULT bad address
@int EFBIG file too large
@int EHOSTUNREACH no route to host
@int EIDRM identifier removed
@int EILSEQ illegal byte sequence
@int EINPROGRESS operation now in progress
@int EINTR interrupted system call
@int EINVAL invalid argument
@int EIO input/output error
@int EISCONN socket is already connected
@int EISDIR is a directory
@int ELOOP too many levels of symbolic links
@int EMFILE too many open files
@int EMLINK too many links
@int EMSGSIZE message too long
@int ENAMETOOLONG file name too long
@int ENETDOWN network is down
@int ENETRESET network dropped connection on reset
@int ENETUNREACH network is unreachable
@int ENFILE too many open files in system
@int ENOBUFS no buffer space available
@int ENODEV operation not supported by device
@int ENOENT no such file or directory
@int ENOEXEC exec format error
@int ENOLCK no locks available
@int ENOMEM cannot allocate memory
@int ENOMSG no message of desired type
@int ENOPROTOOPT protocol not available
@int ENOSPC no space left on device
@int ENOSYS function not implemented
@int ENOTCONN socket is not connected
@int ENOTDIR not a directory
@int ENOTEMPTY directory not empty
@int ENOTSOCK socket operation on non-socket
@int ENOTSUP operation not supported
@int ENOTTY inappropriate ioctl for device
@int ENXIO device not configured
@int EOPNOTSUPP operation not supported on socket
@int EOVERFLOW value too large to be stored in data type
@int EPERM operation not permitted
@int EPIPE broken pipe
@int EPROTO protocol error
@int EPROTONOSUPPORT protocol not supported
@int EPROTOTYPE protocol wrong type for socket
@int ERANGE result too large
@int EROFS read-only file system
@int ESPIPE illegal seek
@int ESRCH no such process
@int ETIMEDOUT operation timed out
@int ETXTBSY text file busy
@int EWOULDBLOCK operation would block
@int EXDEV cross-device link
@usage
  -- Print errno constants supported on this host.
  for name, value in pairs (require "posix.errno") do
    if type (value) == "number" then
      print (name, value)
    end
  end
*/
LUALIB_API int
luaopen_posix_errno(lua_State *L)
{
	luaL_newlib(L, posix_errno_fns);
	lua_pushstring(L, LPOSIX_VERSION_STRING("errno"));
	lua_setfield(L, -2, "version");


#ifdef E2BIG
	LPOSIX_CONST( E2BIG		);
#endif
#ifdef EACCES
	LPOSIX_CONST( EACCES		);
#endif
#ifdef EADDRINUSE
	LPOSIX_CONST( EADDRINUSE	);
#endif
#ifdef EADDRNOTAVAIL
	LPOSIX_CONST( EADDRNOTAVAIL	);
#endif
#ifdef EAFNOSUPPORT
	LPOSIX_CONST( EAFNOSUPPORT	);
#endif
#ifdef EAGAIN
	LPOSIX_CONST( EAGAIN		);
#endif
#ifdef EALREADY
	LPOSIX_CONST( EALREADY		);
#endif
#ifdef EBADF
	LPOSIX_CONST( EBADF		);
#endif
#ifdef EBADMSG
	LPOSIX_CONST( EBADMSG		);
#endif
#ifdef EBUSY
	LPOSIX_CONST( EBUSY		);
#endif
#ifdef ECANCELED
	LPOSIX_CONST( ECANCELED		);
#endif
#ifdef ECHILD
	LPOSIX_CONST( ECHILD		);
#endif
#ifdef ECONNABORTED
	LPOSIX_CONST( ECONNABORTED	);
#endif
#ifdef ECONNREFUSED
	LPOSIX_CONST( ECONNREFUSED	);
#endif
#ifdef ECONNRESET
	LPOSIX_CONST( ECONNRESET	);
#endif
#ifdef EDEADLK
	LPOSIX_CONST( EDEADLK		);
#endif
#ifdef EDESTADDRREQ
	LPOSIX_CONST( EDESTADDRREQ	);
#endif
#ifdef EDOM
	LPOSIX_CONST( EDOM		);
#endif
#ifdef EEXIST
	LPOSIX_CONST( EEXIST		);
#endif
#ifdef EFAULT
	LPOSIX_CONST( EFAULT		);
#endif
#ifdef EFBIG
	LPOSIX_CONST( EFBIG		);
#endif
#ifdef EHOSTUNREACH
	LPOSIX_CONST( EHOSTUNREACH	);
#endif
#ifdef EIDRM
	LPOSIX_CONST( EIDRM		);
#endif
#ifdef EILSEQ
	LPOSIX_CONST( EILSEQ		);
#endif
#ifdef EINPROGRESS
	LPOSIX_CONST( EINPROGRESS	);
#endif
#ifdef EINTR
	LPOSIX_CONST( EINTR		);
#endif
#ifdef EINVAL
	LPOSIX_CONST( EINVAL		);
#endif
#ifdef EIO
	LPOSIX_CONST( EIO		);
#endif
#ifdef EISCONN
	LPOSIX_CONST( EISCONN		);
#endif
#ifdef EISDIR
	LPOSIX_CONST( EISDIR		);
#endif
#ifdef ELOOP
	LPOSIX_CONST( ELOOP		);
#endif
#ifdef EMFILE
	LPOSIX_CONST( EMFILE		);
#endif
#ifdef EMLINK
	LPOSIX_CONST( EMLINK		);
#endif
#ifdef EMSGSIZE
	LPOSIX_CONST( EMSGSIZE		);
#endif
#ifdef ENAMETOOLONG
	LPOSIX_CONST( ENAMETOOLONG	);
#endif
#ifdef ENETDOWN
	LPOSIX_CONST( ENETDOWN		);
#endif
#ifdef ENETRESET
	LPOSIX_CONST( ENETRESET		);
#endif
#ifdef ENETUNREACH
	LPOSIX_CONST( ENETUNREACH	);
#endif
#ifdef ENFILE
	LPOSIX_CONST( ENFILE		);
#endif
#ifdef ENOBUFS
	LPOSIX_CONST( ENOBUFS		);
#endif
#ifdef ENODEV
	LPOSIX_CONST( ENODEV		);
#endif
#ifdef ENOENT
	LPOSIX_CONST( ENOENT		);
#endif
#ifdef ENOEXEC
	LPOSIX_CONST( ENOEXEC		);
#endif
#ifdef ENOLCK
	LPOSIX_CONST( ENOLCK		);
#endif
#ifdef ENOMEM
	LPOSIX_CONST( ENOMEM		);
#endif
#ifdef ENOMSG
	LPOSIX_CONST( ENOMSG		);
#endif
#ifdef ENOPROTOOPT
	LPOSIX_CONST( ENOPROTOOPT	);
#endif
#ifdef ENOSPC
	LPOSIX_CONST( ENOSPC		);
#endif
#ifdef ENOSYS
	LPOSIX_CONST( ENOSYS		);
#endif
#ifdef ENOTCONN
	LPOSIX_CONST( ENOTCONN		);
#endif
#ifdef ENOTDIR
	LPOSIX_CONST( ENOTDIR		);
#endif
#ifdef ENOTEMPTY
	LPOSIX_CONST( ENOTEMPTY		);
#endif
#ifdef ENOTSOCK
	LPOSIX_CONST( ENOTSOCK		);
#endif
#ifdef ENOTSUP
	LPOSIX_CONST( ENOTSUP		);
#endif
#ifdef ENOTTY
	LPOSIX_CONST( ENOTTY		);
#endif
#ifdef ENXIO
	LPOSIX_CONST( ENXIO		);
#endif
#ifdef EOPNOTSUPP
	LPOSIX_CONST( EOPNOTSUPP	);
#endif
#ifdef EOVERFLOW
	LPOSIX_CONST( EOVERFLOW		);
#endif
#ifdef EPERM
	LPOSIX_CONST( EPERM		);
#endif
#ifdef EPIPE
	LPOSIX_CONST( EPIPE		);
#endif
#ifdef EPROTO
	LPOSIX_CONST( EPROTO		);
#endif
#ifdef EPROTONOSUPPORT
	LPOSIX_CONST( EPROTONOSUPPORT	);
#endif
#ifdef EPROTOTYPE
	LPOSIX_CONST( EPROTOTYPE	);
#endif
#ifdef ERANGE
	LPOSIX_CONST( ERANGE		);
#endif
#ifdef EROFS
	LPOSIX_CONST( EROFS		);
#endif
#ifdef ESPIPE
	LPOSIX_CONST( ESPIPE		);
#endif
#ifdef ESRCH
	LPOSIX_CONST( ESRCH		);
#endif
#ifdef ETIMEDOUT
	LPOSIX_CONST( ETIMEDOUT		);
#endif
#ifdef ETXTBSY
	LPOSIX_CONST( ETXTBSY		);
#endif
#ifdef EWOULDBLOCK
	LPOSIX_CONST( EWOULDBLOCK	);
#endif
#ifdef EXDEV
	LPOSIX_CONST( EXDEV		);
#endif

	return 1;
}
