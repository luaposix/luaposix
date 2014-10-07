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

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "_helpers.h"

#ifndef errno
extern int errno;
#endif


/***
Error Handling Functions.
@section errorhandling
*/


/***
Describe an error code/and or read `errno`
@function errno
@int[opt=current errno] n optional error code
@return description
@return error code
@see strerror(3)
@see errno
@usage
local strerr, nerr = P.errno ()
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
P.errno (P.EBADF)
*/
static int
Pset_errno(lua_State *L)
{
	errno = checkint(L, 1);
	checknargs(L, 1);
	return 0;
}


static void
errno_setconst(lua_State *L)
{
	/* errno values */
#ifdef E2BIG
	PCONST( E2BIG		);
#endif
#ifdef EACCES
	PCONST( EACCES		);
#endif
#ifdef EADDRINUSE
	PCONST( EADDRINUSE	);
#endif
#ifdef EADDRNOTAVAIL
	PCONST( EADDRNOTAVAIL	);
#endif
#ifdef EAFNOSUPPORT
	PCONST( EAFNOSUPPORT	);
#endif
#ifdef EAGAIN
	PCONST( EAGAIN		);
#endif
#ifdef EALREADY
	PCONST( EALREADY	);
#endif
#ifdef EBADF
	PCONST( EBADF		);
#endif
#ifdef EBADMSG
	PCONST( EBADMSG		);
#endif
#ifdef EBUSY
	PCONST( EBUSY		);
#endif
#ifdef ECANCELED
	PCONST( ECANCELED	);
#endif
#ifdef ECHILD
	PCONST( ECHILD		);
#endif
#ifdef ECONNABORTED
	PCONST( ECONNABORTED	);
#endif
#ifdef ECONNREFUSED
	PCONST( ECONNREFUSED	);
#endif
#ifdef ECONNRESET
	PCONST( ECONNRESET	);
#endif
#ifdef EDEADLK
	PCONST( EDEADLK		);
#endif
#ifdef EDESTADDRREQ
	PCONST( EDESTADDRREQ	);
#endif
#ifdef EDOM
	PCONST( EDOM		);
#endif
#ifdef EEXIST
	PCONST( EEXIST		);
#endif
#ifdef EFAULT
	PCONST( EFAULT		);
#endif
#ifdef EFBIG
	PCONST( EFBIG		);
#endif
#ifdef EHOSTUNREACH
	PCONST( EHOSTUNREACH	);
#endif
#ifdef EIDRM
	PCONST( EIDRM		);
#endif
#ifdef EILSEQ
	PCONST( EILSEQ		);
#endif
#ifdef EINPROGRESS
	PCONST( EINPROGRESS	);
#endif
#ifdef EINTR
	PCONST( EINTR		);
#endif
#ifdef EINVAL
	PCONST( EINVAL		);
#endif
#ifdef EIO
	PCONST( EIO		);
#endif
#ifdef EISCONN
	PCONST( EISCONN		);
#endif
#ifdef EISDIR
	PCONST( EISDIR		);
#endif
#ifdef ELOOP
	PCONST( ELOOP		);
#endif
#ifdef EMFILE
	PCONST( EMFILE		);
#endif
#ifdef EMLINK
	PCONST( EMLINK		);
#endif
#ifdef EMSGSIZE
	PCONST( EMSGSIZE	);
#endif
#ifdef ENAMETOOLONG
	PCONST( ENAMETOOLONG	);
#endif
#ifdef ENETDOWN
	PCONST( ENETDOWN	);
#endif
#ifdef ENETRESET
	PCONST( ENETRESET	);
#endif
#ifdef ENETUNREACH
	PCONST( ENETUNREACH	);
#endif
#ifdef ENFILE
	PCONST( ENFILE		);
#endif
#ifdef ENOBUFS
	PCONST( ENOBUFS		);
#endif
#ifdef ENODEV
	PCONST( ENODEV		);
#endif
#ifdef ENOENT
	PCONST( ENOENT		);
#endif
#ifdef ENOEXEC
	PCONST( ENOEXEC		);
#endif
#ifdef ENOLCK
	PCONST( ENOLCK		);
#endif
#ifdef ENOMEM
	PCONST( ENOMEM		);
#endif
#ifdef ENOMSG
	PCONST( ENOMSG		);
#endif
#ifdef ENOPROTOOPT
	PCONST( ENOPROTOOPT	);
#endif
#ifdef ENOSPC
	PCONST( ENOSPC		);
#endif
#ifdef ENOSYS
	PCONST( ENOSYS		);
#endif
#ifdef ENOTCONN
	PCONST( ENOTCONN	);
#endif
#ifdef ENOTDIR
	PCONST( ENOTDIR		);
#endif
#ifdef ENOTEMPTY
	PCONST( ENOTEMPTY	);
#endif
#ifdef ENOTSOCK
	PCONST( ENOTSOCK	);
#endif
#ifdef ENOTSUP
	PCONST( ENOTSUP		);
#endif
#ifdef ENOTTY
	PCONST( ENOTTY		);
#endif
#ifdef ENXIO
	PCONST( ENXIO		);
#endif
#ifdef EOPNOTSUPP
	PCONST( EOPNOTSUPP	);
#endif
#ifdef EOVERFLOW
	PCONST( EOVERFLOW	);
#endif
#ifdef EPERM
	PCONST( EPERM		);
#endif
#ifdef EPIPE
	PCONST( EPIPE		);
#endif
#ifdef EPROTO
	PCONST( EPROTO		);
#endif
#ifdef EPROTONOSUPPORT
	PCONST( EPROTONOSUPPORT	);
#endif
#ifdef EPROTOTYPE
	PCONST( EPROTOTYPE	);
#endif
#ifdef ERANGE
	PCONST( ERANGE		);
#endif
#ifdef EROFS
	PCONST( EROFS		);
#endif
#ifdef ESPIPE
	PCONST( ESPIPE		);
#endif
#ifdef ESRCH
	PCONST( ESRCH		);
#endif
#ifdef ETIMEDOUT
	PCONST( ETIMEDOUT	);
#endif
#ifdef ETXTBSY
	PCONST( ETXTBSY		);
#endif
#ifdef EWOULDBLOCK
	PCONST( EWOULDBLOCK	);
#endif
#ifdef EXDEV
	PCONST( EXDEV		);
#endif

}
