/*
 * POSIX library for Lua 5.1, 5.2 & 5.3.
 * (c) Gary V. Vaughan <gary@vaughan.pe>, 2013-2015
 * (c) Reuben Thomas <rrt@sc3d.org> 2010-2013
 * (c) Natanael Copa <natanael.copa@gmail.com> 2008-2010
 * Clean up and bug fixes by Leo Razoumov <slonik.az@gmail.com> 2006-10-11
 * Luiz Henrique de Figueiredo <lhf@tecgraf.puc-rio.br> 07 Apr 2006 23:17:49
 * Based on original by Claudio Terra for Lua 3.x.
 * With contributions by Roberto Ierusalimschy.
 * With documentation from Steve Donovan 2012
 */
/***
 Sys V Message Queue Operations.

 Where supported by the underlying system, functions to send and receive
 interprocess messages.  If the module loads successfully, but there is
 no system support, then `posix.sys.msg.version` will be set, but the
 unsupported APIs wil be `nil`.

@module posix.sys.msg
*/

#include <config.h>

#include "_helpers.c"	/* For LPOSIX_2001_COMPLIANT */

#if HAVE_SYSV_MESSAGING
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/types.h>

/***
Get a message queue identifier
@function msgget
@int key message queue id, or `IPC_PRIVATE` for a new queue
@int[opt=0] flags bitwise OR of zero or more from `IPC_CREAT` and `IPC_EXCL`,
  and access permissions `S_IRUSR`, `S_IWUSR`, `S_IRGRP`, `S_IWGRP`, `S_IROTH`
  and `S_IWOTH` (from @{posix.sys.stat})
@treturn[1] int message queue identifier, if successful
@return[2] nil
@treturn[2] string error message
@treturn[2] int errnum
@see msgget(2)
*/
static int
Pmsgget(lua_State *L)
{
	checknargs (L, 2);
	return pushresult(L, msgget(checkint(L, 1), optint(L, 2, 0)), "msgget");
}


/***
Send message to a message queue
@function msgsnd
@int id message queue identifier returned by @{msgget}
@int type arbitrary message type
@string message content
@int[opt=0] flags optionally `IPC_NOWAIT`
@treturn int 0, if successful
@return[2] nil
@treturn[2] string error message
@treturn[2] int errnum
@see msgsnd(2)
 */
static int
Pmsgsnd(lua_State *L)
{
	void *ud;
	lua_Alloc lalloc = lua_getallocf(L, &ud);
	struct {
		long mtype;
		char mtext[0];
	} *msg;
	size_t len;
	size_t msgsz;
	ssize_t r;

	int msgid = checkint(L, 1);
	long msgtype = checklong(L, 2);
	const char *msgp = luaL_checklstring(L, 3, &len);
	int msgflg = optint(L, 4, 0);

	checknargs(L, 4);

	msgsz = sizeof(long) + len;

	if ((msg = lalloc(ud, NULL, 0, msgsz)) == NULL)
		return pusherror(L, "lalloc");

	msg->mtype = msgtype;
	memcpy(msg->mtext, msgp, len);

	r = msgsnd(msgid, msg, msgsz, msgflg);
	lua_pushinteger(L, r);

	lalloc(ud, msg, msgsz, 0);

	return (r == -1 ? pusherror(L, NULL) : 1);
}


/***
Receive message from a message queue
@function msgrcv
@int id message queue identifier returned by @{msgget}
@int size maximum message size
@int type message type (optional, default - 0)
@int[opt=0] flags bitwise OR of zero or more of `IPC_NOWAIT`, `MSG_EXCEPT`
  and `MSG_NOERROR`
@treturn[1] int message type from @{msgsnd}
@treturn[1] string message text, if successful
@return[2] nil
@treturn[2] string error message
@treturn[2] int errnum
@see msgrcv(2)
 */
static int
Pmsgrcv(lua_State *L)
{
	int msgid = checkint(L, 1);
	size_t msgsz = checkint(L, 2);
	long msgtyp = optint(L, 3, 0);
	int msgflg = optint(L, 4, 0);

	void *ud;
	lua_Alloc lalloc;
	struct {
		long mtype;
		char mtext[0];
	} *msg;

	checknargs(L, 4);
	lalloc = lua_getallocf(L, &ud);

	if ((msg = lalloc(ud, NULL, 0, msgsz)) == NULL)
		return pusherror(L, "lalloc");

	int res = msgrcv(msgid, msg, msgsz, msgtyp, msgflg);
	if (res != -1)
	{
		lua_pushinteger(L, msg->mtype);
		lua_pushlstring(L, msg->mtext, res - sizeof(long));
	}
	lalloc(ud, msg, msgsz, 0);

	return (res == -1) ? pusherror(L, NULL) : 2;
}
#endif /*!HAVE_SYSV_MESSAGING*/


static const luaL_Reg posix_sys_msg_fns[] =
{
#if HAVE_SYSV_MESSAGING
	LPOSIX_FUNC( Pmsgget		),
	LPOSIX_FUNC( Pmsgsnd		),
	LPOSIX_FUNC( Pmsgrcv		),
#endif
	{NULL, NULL}
};


/***
Constants.
@section constants
*/

/***
Message constants.
Any constants not available in the underlying system will be `nil` valued.
@table posix.sys.msg
@int IPC_CREAT create entry if key does not exist
@int IPC_EXCL fail if key exists
@int IPC_PRIVATE private key
@int IPC_NOWAIT error if request must wait
@int MSG_EXCEPT read messages with differing type
@int MSG_NOERROR truncate received message rather than erroring
@usage
  -- Print msg constants supported on this host.
  for name, value in pairs (require "posix.sys.msg") do
    if type (value) == "number" then
      print (name, value)
     end
  end
*/

LUALIB_API int
luaopen_posix_sys_msg(lua_State *L)
{
	luaL_register(L, "posix.sys.msg", posix_sys_msg_fns);
	lua_pushliteral(L, "posix.sys.msg for " LUA_VERSION " / " PACKAGE_STRING);
	lua_setfield(L, -2, "version");

#if HAVE_SYSV_MESSAGING
	LPOSIX_CONST( IPC_CREAT		);
	LPOSIX_CONST( IPC_EXCL		);
	LPOSIX_CONST( IPC_PRIVATE	);
	LPOSIX_CONST( IPC_NOWAIT	);
#  ifdef MSG_EXCEPT
	LPOSIX_CONST( MSG_EXCEPT	);
#  endif
#  ifdef MSG_NOERROR
	LPOSIX_CONST( MSG_NOERROR	);
#  endif
#endif

	return 1;
}
