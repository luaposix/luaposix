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
 Wait for Process Termination.

@module posix.sys.wait
*/

#include <sys/wait.h>

#include "_helpers.c"


/***
Wait for child process to terminate.
@function wait
@int[opt=-1] pid child process id to wait for, or -1 for any child process
@int[opt] options bitwise OR of `WNOHANG` and `WUNTRACED`
@treturn[1] int pid of running child, if not exited yet and called with `WNOHANG`
@treturn[1] string "running"
@treturn[2] int pid of terminated child, if successful
@treturn[2] string "exited", "killed" or "stopped"
@treturn[2] int exit status, or signal number responsible for "killed" or "stopped"
@return[3] nil
@treturn[3] string error message
@treturn[3] int errnum
@see waitpid(2)
@see posix.unistd.fork
*/
static int
Pwait(lua_State *L)
{
	int status = 0;
	pid_t pid = (pid_t)optinteger(L, 1, -1);
	int options = optint(L, 2, 0);
	checknargs(L, 2);

	pid = waitpid(pid, &status, options);
	if (pid == -1)
		return pusherror(L, NULL);
	lua_pushinteger(L, pid);
	if (pid == 0)
	{
		lua_pushliteral(L,"running");
		return 2;
	}
	else if (WIFEXITED(status))
	{
		lua_pushliteral(L,"exited");
		lua_pushinteger(L, WEXITSTATUS(status));
		return 3;
	}
	else if (WIFSIGNALED(status))
	{
		lua_pushliteral(L,"killed");
		lua_pushinteger(L, WTERMSIG(status));
		return 3;
	}
	else if (WIFSTOPPED(status))
	{
		lua_pushliteral(L,"stopped");
		lua_pushinteger(L, WSTOPSIG(status));
		return 3;
	}
	return 1;
}


static const luaL_Reg posix_sys_wait_fns[] =
{
	LPOSIX_FUNC( Pwait		),
	{NULL, NULL}
};


/***
Constants.
@section constants
*/

/***
Wait constants.
Any constants not available in the underlying system will be `nil` valued.
@table posix.sys.wait
@int WNOHANG don't block waiting
@int WUNTRACED report status of stopped children
@usage
  -- Print wait constants supported on this host.
  for name, value in pairs (require "posix.sys.wait") do
    if type (value) == "number" then
      print (name, value)
     end
  end
*/

LUALIB_API int
luaopen_posix_sys_wait(lua_State *L)
{
	luaL_newlib(L, posix_sys_wait_fns);

	lua_pushstring(L, LPOSIX_VERSION_STRING("sys.wait"));
	lua_setfield(L, -2, "version");

	LPOSIX_CONST( WNOHANG		);
	LPOSIX_CONST( WUNTRACED		);

	return 1;
}
