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

#include <signal.h>

#include "_helpers.h"


/***
Signal Handling Functions.
@section signalhandling
*/


/***
Send a signal to the given process.
@function kill
@int pid process to act on
@int[opt=`SIGTERM` sig signal to send
@treturn[1] int `0`, if successful
@return[2] nil
@treturn[2] string error message
@see kill(2)
*/
static int
Pkill(lua_State *L)
{
	pid_t pid = checkint(L, 1);
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
@see killpg(2)
*/
static int
Pkillpg(lua_State *L)
{
	int pgrp = checkint(L, 1);
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
@see raise(3)
*/
static int
Praise(lua_State *L)
{
	int sig = checkint(L, 1);
	checknargs(L, 1);
	lua_pop(L, 1);
	lua_pushinteger(L, raise(sig));
	return 1;
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
sig_handle (lua_State *L, lua_Debug *UNUSED (ar))
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
			fprintf(stderr,"error in signal handler %d: %s\n",signalno,lua_tostring(L,-1));
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
@see signal.lua
@int signum
@tparam[opt="SIG_DFL"] function|string handler function, "SIG_IGN" or "SIG_DFL"
@param[opt] flags the `sa_flags` element of `struct sigaction`
@return previous handler function
@see sigaction(2)
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
			argtypeerror(L, 2, "function, string or nil");
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


static void
signal_setconst(lua_State *L)
{
	/* Signals */
#ifdef SIGABRT
	PCONST( SIGABRT		);
#endif
#ifdef SIGALRM
	PCONST( SIGALRM		);
#endif
#ifdef SIGBUS
	PCONST( SIGBUS		);
#endif
#ifdef SIGCHLD
	PCONST( SIGCHLD		);
#endif
#ifdef SIGCONT
	PCONST( SIGCONT		);
#endif
#ifdef SIGFPE
	PCONST( SIGFPE		);
#endif
#ifdef SIGHUP
	PCONST( SIGHUP		);
#endif
#ifdef SIGILL
	PCONST( SIGILL		);
#endif
#ifdef SIGINT
	PCONST( SIGINT		);
#endif
#ifdef SIGKILL
	PCONST( SIGKILL		);
#endif
#ifdef SIGPIPE
	PCONST( SIGPIPE		);
#endif
#ifdef SIGQUIT
	PCONST( SIGQUIT		);
#endif
#ifdef SIGSEGV
	PCONST( SIGSEGV		);
#endif
#ifdef SIGSTOP
	PCONST( SIGSTOP		);
#endif
#ifdef SIGTERM
	PCONST( SIGTERM		);
#endif
#ifdef SIGTSTP
	PCONST( SIGTSTP		);
#endif
#ifdef SIGTTIN
	PCONST( SIGTTIN		);
#endif
#ifdef SIGTTOU
	PCONST( SIGTTOU		);
#endif
#ifdef SIGUSR1
	PCONST( SIGUSR1		);
#endif
#ifdef SIGUSR2
	PCONST( SIGUSR2		);
#endif
#ifdef SIGSYS
	PCONST( SIGSYS		);
#endif
#ifdef SIGTRAP
	PCONST( SIGTRAP		);
#endif
#ifdef SIGURG
	PCONST( SIGURG		);
#endif
#ifdef SIGVTALRM
	PCONST( SIGVTALRM	);
#endif
#ifdef SIGXCPU
	PCONST( SIGXCPU		);
#endif
#ifdef SIGXFSZ
	PCONST( SIGXFSZ		);
#endif

	/* Signal flags */
#ifdef SA_NOCLDSTOP
	PCONST( SA_NOCLDSTOP	);
#endif
#ifdef SA_NOCLDWAIT
	PCONST( SA_NOCLDWAIT	);
#endif
#ifdef SA_RESETHAND
	PCONST( SA_RESETHAND	);
#endif
#ifdef SA_NODEFER
	PCONST( SA_NODEFER	);
#endif

	/* Signals table stored in registry for Psignal and sig_handle */
	lua_pushlightuserdata(L, &signalL);
	lua_newtable(L);
	lua_rawset(L, LUA_REGISTRYINDEX);

	signalL = L; /* For sig_postpone */
}
