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
 Sys V Message Queue Operations.

 Where supported by the underlying system, functions to send and receive
 interprocess messages.  If the module loads successfully, but there is
 no system support, then `posix.sys.msg.version` will be set, but the
 unsupported APIs wil be `nil`.

@module posix.sys.msg
*/

#if HAVE_SYS_MSG_H && HAVE_MSGRCV && HAVE_MSGSND
# define HAVE_SYSV_MESSAGING 1
#else
# define HAVE_SYSV_MESSAGING 0
#endif

#include "_helpers.c"

#if HAVE_SYSV_MESSAGING
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/types.h>


/***
Message queue record.
@table PosixMsqid
@int msg_qnum number of messages on the queue
@int msg_qbytes number of bytes allowed on the queue
@int msg_lspid process id of last msgsnd
@int msg_lrpid process id of last msgrcv
@int msg_stime time of last msgsnd
@int msg_rtime time of last msgrcv
@int msg_ctime time of last change
*/
static int
pushmsqid(lua_State *L, struct msqid_ds *msqid)
{
	if (!msqid)
		return lua_pushnil(L), 1;

	lua_createtable(L, 0, 8);
	setintegerfield(msqid, msg_qnum);
	setintegerfield(msqid, msg_qbytes);
	setintegerfield(msqid, msg_lspid);
	setintegerfield(msqid, msg_lrpid);
	setintegerfield(msqid, msg_stime);
	setintegerfield(msqid, msg_rtime);
	setintegerfield(msqid, msg_ctime);

	lua_createtable(L, 0, 5);
	pushintegerfield("uid", msqid->msg_perm.uid);
	pushintegerfield("gid", msqid->msg_perm.gid);
	pushintegerfield("cuid", msqid->msg_perm.cuid);
	pushintegerfield("cgid", msqid->msg_perm.cgid);
	pushintegerfield("mode", msqid->msg_perm.mode);

	lua_setfield(L, -2, "msg_perm");
	settypemetatable("PosixMsqid");
	return 1;
}


static const char *Smsqid_fields[] = { "msg_qbytes", "msg_perm" };


static const char *Sipcperm_fields[] = { "uid", "gid", "mode" };


static void
tomsqid(lua_State *L, int index, struct msqid_ds *msqid)
{
	int subindex;
	luaL_checktype(L, index, LUA_TTABLE);

	/* Copy fields to msqid struct */
	msqid->msg_qbytes = (msglen_t)checkintegerfield(L, index, "msg_qbytes");

	checkfieldtype(L, index, "msg_perm", LUA_TTABLE, "table");
	subindex = lua_gettop(L);
	msqid->msg_perm.uid  = (uid_t)checkintegerfield(L, subindex, "uid");
	msqid->msg_perm.gid  = (gid_t)checkintegerfield(L, subindex, "gid");
	msqid->msg_perm.mode = checkintfield(L, subindex, "mode");

        checkfieldnames(L, index, Smsqid_fields);
	checkfieldnames(L, subindex, Sipcperm_fields);
}



/***
@function msgctl
@int id message queue identifier returned by @{msgget}
@int cmd one of `IPC_STAT`, `IPC_SET` or `IPC_RMID`
@PosixMsqid[opt=nil] values to set with `IPC_SET`
@treturn[1] PosixMsqid table for *id*, with `IPC_STAT`, if successful
@treturn[2] non-nil, with `IPC_SET` or `IPC_RMID`, if successful
@treturn[3] nil otherwise
@treturn[3] string error message
@treturn[3] int errnum
@see msgctl(2)
@usage
  local sysvmsg = require 'posix.sys.msg'
  local msq = sysvmsg.msgget(sysvmsg.IPC_PRIVATE)
  local msqid, errmsg = sysvmsg.msgctl(msq, sysvmsg.IPC_STAT)
  assert(msqid, errmsg)
  assert(sysvmsg.msgctl(msq, sysvmsg.IPC_RMID))
*/
static int
Pmsgctl(lua_State *L)
{
	int id = checkint(L, 1);
	int cmd = checkint(L, 2);
	struct msqid_ds msqid;

	switch (cmd)
	{
		case IPC_RMID:
			checknargs(L, 2);
			return pushresult(L, msgctl(id, cmd, NULL), "msgctl");

		case IPC_SET:
			checknargs(L, 3);
			tomsqid(L, 3, &msqid);
			return pushresult(L, msgctl(id, cmd, &msqid), "msgctl");

		case IPC_STAT:
			checknargs(L, 2);
			if (msgctl(id, cmd, &msqid) < 0)
				return pusherror(L, "msgctl");
			return pushmsqid(L, &msqid);

		default:
			checknargs(L, 3);
			return pusherror(L, "unsupported cmd value");
	}
}


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
	return pushresult(L, msgget((key_t)checkinteger(L, 1), optint(L, 2, 0)), "msgget");
}


/***
Send message to a message queue
@function msgsnd
@int id message queue identifier returned by @{msgget}
@int type arbitrary message type
@string message content
@int[opt=0] flags optionally `IPC_NOWAIT`
@treturn[1] int `0`, if successful
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
	size_t msgsz = (size_t)checkinteger(L, 2);
	long msgtyp = optlong(L, 3, 0);
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
	LPOSIX_FUNC( Pmsgctl		),
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
@int IPC_STAT return a Msqid table from msgctl
@int IPC_SET set the Msqid fields from msgctl
@int IPC_RMID remove a message queue with msgctl
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
	luaL_newlib(L, posix_sys_msg_fns);
	lua_pushstring(L, LPOSIX_VERSION_STRING("sys.msg"));
	lua_setfield(L, -2, "version");

#if HAVE_SYSV_MESSAGING
	LPOSIX_CONST( IPC_CREAT		);
#  ifdef MSG_EXCEPT
	LPOSIX_CONST( MSG_EXCEPT	);
#  endif
	LPOSIX_CONST( IPC_EXCL		);
#  ifdef MSG_NOERROR
	LPOSIX_CONST( MSG_NOERROR	);
#  endif
	LPOSIX_CONST( IPC_NOWAIT	);
	LPOSIX_CONST( IPC_PRIVATE	);
	LPOSIX_CONST( IPC_RMID		);
	LPOSIX_CONST( IPC_SET		);
	LPOSIX_CONST( IPC_STAT		);
#endif

	return 1;
}
