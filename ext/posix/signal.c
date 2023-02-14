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
 Software Signal Facilities.

 Constants and functions for propagating signals among processes.

 Note that `posix.signal.signal` is implemented with sigaction(2) for
 consistent semantics across platforms. Also note that installed signal
 handlers are not called immediatly upon occurrence of a signal. Instead,
 in order to keep the interperter state clean, they are executed in the
 context of a debug hook which is called as soon as the interpreter enters
 a new function, returns from the currently executing function, or after the
 execution of the current instruction has ended.

@module posix.signal
*/

#include <signal.h>

#include "_helpers.c"


/***
Send a signal to the given process.
@function kill
@int pid process to act on
@int[opt=`SIGTERM` sig signal to send
@treturn[1] int `0`, if successful
@return[2] nil
@treturn[2] string error message
@treturn[2] int errnum
@see kill(2)
*/
static int
Pkill(lua_State *L)
{
	pid_t pid = (pid_t)checkinteger(L, 1);
	int sig = optint(L, 2, SIGTERM);
	checknargs(L, 2);
	return pushresult(L, kill(pid, sig), NULL);
}


/***
Send a signal to the given process group.
@function killpg
@int pgrp group id to act on, or `0` for the sending process`s group
@int[opt=`SIGTERM`] sig signal to send
@treturn[1] int `0`, if successful
@return[2] nil
@treturn[2] string error message
@treturn[2] int errnum
@see killpg(2)
*/
static int
Pkillpg(lua_State *L)
{
	pid_t pgrp = (pid_t)checkinteger(L, 1);
	int sig = optint(L, 2, SIGTERM);
	checknargs(L, 2);
	return pushresult(L, killpg(pgrp, sig), NULL);
}


/***
Raise a signal on this process.
@function raise
@int sig signal to send
@treturn[1] int `0`, if successful
@return[2] nil
@treturn[2] string error message
@treturn[2] int errnum
@see raise(3)
*/
static int
Praise(lua_State *L)
{
	int sig = checkint(L, 1);
	checknargs(L, 1);
	lua_pop(L, 1);
	return pushintegerresult(raise(sig));
}


static lua_State *signalL;

#define SIGNAL_QUEUE_MAX 25
static volatile sig_atomic_t signal_pending, defer_signal;
static volatile sig_atomic_t signal_count = 0;
static volatile sig_atomic_t signals[SIGNAL_QUEUE_MAX];

#define sigmacros_map \
	MENTRY( _DFL ) \
	MENTRY( _IGN )

static const char *const Ssigmacros[] =
{
#define MENTRY(_s) LPOSIX_STR_1(LPOSIX_SPLICE(_SIG, _s)),
	sigmacros_map
#undef MENTRY
	NULL
};

static void (*Fsigmacros[])(int) =
{
#define MENTRY(_s) LPOSIX_SPLICE(SIG, _s),
	sigmacros_map
#undef MENTRY
	NULL
};


static void
sig_handle (lua_State *L, lua_Debug *LPOSIX_UNUSED (ar))
{
	/* Block all signals until we have run the Lua signal handler */
	sigset_t mask, oldmask;
	sigfillset(&mask);
	sigprocmask(SIG_SETMASK, &mask, &oldmask);

	lua_sethook(L, NULL, 0, 0);

	/* Get signal handlers table */
	lua_pushlightuserdata(L, &signalL);
	lua_rawget(L, LUA_REGISTRYINDEX);

	/* Empty the signal queue */
	while (signal_count--)
	{
		sig_atomic_t signalno = signals[signal_count];
		/* Get handler */
		lua_pushinteger(L, signalno);
		lua_gettable(L, -2);

		/* Call handler with signal number */
		lua_pushinteger(L, signalno);
		if (lua_pcall(L, 1, 0, 0) != 0)
			fprintf(stderr,"error in signal handler %ld: %s\n", (long)signalno, lua_tostring(L,-1));
	}
	signal_count = 0;  /* reset global to initial state */

	/* Having run the Lua signal handler, restore original signal mask */
	sigprocmask(SIG_SETMASK, &oldmask, NULL);
}


static void
sig_postpone (int i)
{
	if (defer_signal)
	{
		signal_pending = i;
		return;
	}
	if (signal_count == SIGNAL_QUEUE_MAX)
		return;
	defer_signal++;
	/* Queue signals */
	signals[signal_count] = i;
	signal_count ++;
	lua_sethook(signalL, sig_handle, LUA_MASKCALL | LUA_MASKRET | LUA_MASKCOUNT, 1);
	defer_signal--;
	/* re-raise any pending signals */
	if (defer_signal == 0 && signal_pending != 0)
		raise (signal_pending);
}


static int
sig_handler_wrap (lua_State *L)
{
	int sig = luaL_checkinteger(L, lua_upvalueindex(1));
	void (*handler)(int) = lua_touserdata(L, lua_upvalueindex(2));
	handler(sig);
	return 0;
}


/***
Install a signal handler for this signal number.
Although this is the same API as signal(2), it uses sigaction for guaranteed semantics.
@function signal
@int signum
@tparam[opt=SIG_DFL] function handler function, or `SIG_IGN` or `SIG_DFL` constants
@param[opt] flags the `sa_flags` element of `struct sigaction`
@treturn function previous handler function
@see sigaction(2)
@see signal.lua
*/
static int
Psignal (lua_State *L)
{
	struct sigaction sa, oldsa;
	int sig = checkint(L, 1), ret;
	void (*handler)(int) = sig_postpone;

	checknargs(L, 3);

	/* Check handler is OK */
	switch (lua_type(L, 2))
	{
		case LUA_TNIL:
		case LUA_TSTRING:
			handler = Fsigmacros[luaL_checkoption(L, 2, "SIG_DFL", Ssigmacros)];
			break;
		case LUA_TFUNCTION:
			if (lua_tocfunction(L, 2) == sig_handler_wrap)
			{
				lua_getupvalue(L, 2, 1);
				handler = lua_touserdata(L, -1);
				lua_pop(L, 1);
			}
			break;
		default:
			argtypeerror(L, 2, "function, nil or string");
			break;
	}

	/* Set up C signal handler, getting old handler */
	sa.sa_handler = handler;
	sa.sa_flags = optint(L, 3, 0);
	sigfillset(&sa.sa_mask);
	ret = sigaction(sig, &sa, &oldsa);
	if (ret == -1)
		return 0;

	/* Set Lua handler if necessary */
	if (handler == sig_postpone)
	{
		lua_pushlightuserdata(L, &signalL); /* We could use an upvalue, but we need this for sig_handle anyway. */
		lua_rawget(L, LUA_REGISTRYINDEX);
		lua_pushvalue(L, 1);
		lua_pushvalue(L, 2);
		lua_rawset(L, -3);
		lua_pop(L, 1);
	}

	/* Push old handler as result */
	if (oldsa.sa_handler == sig_postpone)
	{
		lua_pushlightuserdata(L, &signalL);
		lua_rawget(L, LUA_REGISTRYINDEX);
		lua_pushvalue(L, 1);
		lua_rawget(L, -2);
	} else if (oldsa.sa_handler == SIG_DFL)
		lua_pushstring(L, "SIG_DFL");
	else if (oldsa.sa_handler == SIG_IGN)
		lua_pushstring(L, "SIG_IGN");
	else
	{
		lua_pushinteger(L, sig);
		lua_pushlightuserdata(L, oldsa.sa_handler);
		lua_pushcclosure(L, sig_handler_wrap, 2);
	}
	return 1;
}


static const luaL_Reg posix_signal_fns[] =
{
	LPOSIX_FUNC( Pkill		),
	LPOSIX_FUNC( Pkillpg		),
	LPOSIX_FUNC( Praise		),
	LPOSIX_FUNC( Psignal		),
	{NULL, NULL}
};


/***
Constants.
@section constants
*/

/***
Signal constants.
Any constants not available in the underlying system will be `nil` valued.
@table posix.signal
@int SIGABRT abort ()
@int SIGALRM alarm clock
@int SIGBUS bus error
@int SIGCHLD to parent on child stop or exit
@int SIGCONT continue a stopped process
@int SIGFPE floating point error
@int SIGHUP hangup
@int SIGILL illegal instruction
@int SIGINFO information request
@int SIGINT interrupt
@int SIGKILL kill
@int SIGPIPE write on pipe with no reader
@int SIGQUIT quit
@int SIGSEGV segmentation violation
@int SIGSTOP stop
@int SIGSYS bad argument to system call
@int SIGTERM terminate
@int SIGTRAP trace trap
@int SIGTSTP stop signal from tty
@int SIGTTIN to readers process group on background tty read
@int SIGTTOU to readers process group on background tty output
@int SIGURG urgent condition on i/o channel
@int SIGUSR1 user defined
@int SIGUSR2 user defined
@int SIGVTALRM virtual time alarm
@int SIGWINCH window size change
@int SIGXCPU exceeded cpu time limit
@int SIGXFSZ exceeded file size limit
@int SA_NOCLDSTOP do not generate a SIGCHLD on child stop
@int SA_NOCLDWAIT don't keep zombies child processes
@int SA_NODEFER don't mask the signal we're delivering
@int SA_RESETHAND reset to SIG_DFL when taking a signal
@int SA_RESTART allow syscalls to restart instead of returning EINTR
@usage
  -- Print signal constants supported on this host.
  for name, value in pairs (require "posix.signal") do
    if type (value) == "number" then
      print (name, value)
     end
  end
*/

LUALIB_API int
luaopen_posix_signal(lua_State *L)
{
	luaL_newlib(L, posix_signal_fns);
	lua_pushstring(L, LPOSIX_VERSION_STRING("signal"));
	lua_setfield(L, -2, "version");

	/* Signals table stored in registry for Psignal and sig_handle */
	lua_pushlightuserdata(L, &signalL);
	lua_newtable(L);
	lua_rawset(L, LUA_REGISTRYINDEX);

	signalL = L; /* For sig_postpone */

	/* Signals */
#ifdef SIGABRT
	LPOSIX_CONST( SIGABRT		);
#endif
#ifdef SIGALRM
	LPOSIX_CONST( SIGALRM		);
#endif
#ifdef SIGBUS
	LPOSIX_CONST( SIGBUS		);
#endif
#ifdef SIGCHLD
	LPOSIX_CONST( SIGCHLD		);
#endif
#ifdef SIGCONT
	LPOSIX_CONST( SIGCONT		);
#endif
#ifdef SIGFPE
	LPOSIX_CONST( SIGFPE		);
#endif
#ifdef SIGHUP
	LPOSIX_CONST( SIGHUP		);
#endif
#ifdef SIGILL
	LPOSIX_CONST( SIGILL		);
#endif
#ifdef SIGINFO
	LPOSIX_CONST( SIGINFO		);
#endif
#ifdef SIGINT
	LPOSIX_CONST( SIGINT		);
#endif
#ifdef SIGKILL
	LPOSIX_CONST( SIGKILL		);
#endif
#ifdef SIGPIPE
	LPOSIX_CONST( SIGPIPE		);
#endif
#ifdef SIGQUIT
	LPOSIX_CONST( SIGQUIT		);
#endif
#ifdef SIGSEGV
	LPOSIX_CONST( SIGSEGV		);
#endif
#ifdef SIGSTOP
	LPOSIX_CONST( SIGSTOP		);
#endif
#ifdef SIGTERM
	LPOSIX_CONST( SIGTERM		);
#endif
#ifdef SIGTSTP
	LPOSIX_CONST( SIGTSTP		);
#endif
#ifdef SIGTTIN
	LPOSIX_CONST( SIGTTIN		);
#endif
#ifdef SIGTTOU
	LPOSIX_CONST( SIGTTOU		);
#endif
#ifdef SIGUSR1
	LPOSIX_CONST( SIGUSR1		);
#endif
#ifdef SIGUSR2
	LPOSIX_CONST( SIGUSR2		);
#endif
#ifdef SIGSYS
	LPOSIX_CONST( SIGSYS		);
#endif
#ifdef SIGTRAP
	LPOSIX_CONST( SIGTRAP		);
#endif
#ifdef SIGURG
	LPOSIX_CONST( SIGURG		);
#endif
#ifdef SIGVTALRM
	LPOSIX_CONST( SIGVTALRM		);
#endif
#ifdef SIGWINCH
	LPOSIX_CONST( SIGWINCH		);
#endif
#ifdef SIGXCPU
	LPOSIX_CONST( SIGXCPU		);
#endif
#ifdef SIGXFSZ
	LPOSIX_CONST( SIGXFSZ		);
#endif

	/* String constants */
	lua_pushliteral(L, "SIG_DFL");
	lua_setfield(L, -2, "SIG_DFL");

	lua_pushliteral(L, "SIG_IGN");
	lua_setfield(L, -2, "SIG_IGN");


	/* Signal flags */
#ifdef SA_NOCLDSTOP
	LPOSIX_CONST( SA_NOCLDSTOP	);
#endif
#ifdef SA_NOCLDWAIT
	LPOSIX_CONST( SA_NOCLDWAIT	);
#endif
#ifdef SA_NODEFER
	LPOSIX_CONST( SA_NODEFER	);
#endif
#ifdef SA_RESETHAND
	LPOSIX_CONST( SA_RESETHAND	);
#endif
#ifdef SA_RESTART
	LPOSIX_CONST( SA_RESTART	);
#endif

	return 1;
}
