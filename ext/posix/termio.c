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

#include <termios.h>

#include "_helpers.h"


/***
Terminal Handling Tables.
@section terminaltables
*/


/***
Control characters.
The field names below are not strings, but index constants each
referring to a terminal control character.
@table ccs
@int VINTR interrupt control character
@int VQUIT quit control character
@int WERASE erase control character
@int VKILL kill control character
@int VEOF end-of-file control character
@int VEOL end-of-line control charactor
@int VEOL2 another end-of-line control charactor
@int VMIN 1
@int VTIME 0
@int VSTART xon/xoff start control character
@int VSTOP xon/xoff stop control character
@int VSUSP suspend control character
*/

/***
Terminal attributes.
@table termios
@int cflag bitwise OR of zero or more of `B0`, `B50`, `B75`, `B110`,
  `B134`, `B150`, `B200`, `B300`, `B600`, `B1200`, `B1800`, `B2400`,
  `B4800`, `B9600`, `B19200`, `B38400`, `B57600`, `B115200`, `CSIZE`,
  `CS5`, `CS6`, `CS7`, `CS8`, `CSTOPB`, `CREAD`, `PARENB`, `PARODD`,
  `HUPCL`, `CLOCAL` and `CRTSCTS`
@int iflag input flags; bitwise OR of zero or more of `IGNBRK`, `BRKINT`,
  `IGNPAR`, `PARMRK`, `INPCK`, `ISTRIP`, `INLCR`, `IGNCR`, `ICRNL`,
  `IXON`, `IXOFF`, `IXANY`, `IMAXBEL`, `IUTF8`
@int lflags local flags; bitwise OR of zero or more of `ISIG`, `ICANON`,
  `ECHO`, `ECHOE`, `ECHOK', 'ECHONL`, `NOFLSH`, `IEXTEN`, 'TOSTOP'
@int oflag output flags; bitwise OR of zero or more of `OPOST`, `ONLCR`,
  `OXTABS`, `ONOEOT`, `OCRNL`, `ONOCR`, `ONLRET`, `OFILL`, `NLDLY`,
  `TABDLY`, `CRDLY`, `FFDLY`, `BSDLY`, `VTDLY`, `OFDEL`
@tfield ccs cc array of terminal control characters
*/

/***
Terminal Handling Functions.
@section terminalhandling
*/


/***
Wait for all written output to reach the terminal.
@function tcdrain
@int fd terminal descriptor to act on
@treturn[1] int `0`, if successful
@return[2] nil
@treturn[2] string error message
@see tcdrain(3)
*/
static int
Ptcdrain(lua_State *L)
{
	int fd = checkint(L, 1);
	checknargs(L, 1);
	return pushresult(L, tcdrain(fd), NULL);
}


/***
Suspend transmission or receipt of data.
@function tcflow
@int fd terminal descriptor to act on
@int action one of `TCOOFF`, `TCOON`, `TCIOFF` or `TCION`
@treturn[1] int `0`, if successful
@return[2] nil
@treturn[2] string error message
@see tcflow(3)
*/
static int
Ptcflow(lua_State *L)
{
	int fd = checkint(L, 1);
	int action = checkint(L, 2);
	checknargs(L, 2);
	return pushresult(L, tcflow(fd, action), NULL);
}


/***
Discard any data already written but not yet sent to the terminal.
@function tcflush
@int fd terminal descriptor to act on
@int action one of `TCIFLUSH`, `TCOFLUSH`, `TCIOFLUSH`
@treturn[1] int `0`, if successful
@return[2] nil
@treturn[2] string error message
@see tcflush(3)
*/
static int
Ptcflush(lua_State *L)
{
	int fd = checkint(L, 1);
	int qs = checkint(L, 2);
	checknargs(L, 2);
	return pushresult(L, tcflush(fd, qs), NULL);
}


/***
Get termios state.
@function tcgetattr
@int fd terminal descriptor
@treturn[1] termios terminal attributes, if successful
@return[2] nil
@treturn[2] string error message
@return error message if failed
@see tcgetattr(3)
@usage
local termios, errmsg = tcgetattr (fd)
*/
static int
Ptcgetattr(lua_State *L)
{
	int r, i;
	struct termios t;
	int fd = checkint(L, 1);

	checknargs(L, 1);
	r = tcgetattr(fd, &t);
	if (r == -1) return pusherror(L, NULL);

	lua_newtable(L);
	lua_pushnumber(L, t.c_iflag); lua_setfield(L, -2, "iflag");
	lua_pushnumber(L, t.c_oflag); lua_setfield(L, -2, "oflag");
	lua_pushnumber(L, t.c_lflag); lua_setfield(L, -2, "lflag");
	lua_pushnumber(L, t.c_cflag); lua_setfield(L, -2, "cflag");

	lua_newtable(L);
	for (i=0; i<NCCS; i++)
	{
		lua_pushnumber(L, i);
		lua_pushnumber(L, t.c_cc[i]);
		lua_settable(L, -3);
	}
	lua_setfield(L, -2, "cc");

	return 1;
}


/***
Send a stream of zero valued bits.
@function tcsendbreak
@see tcsendbreak(3)
@int fd terminal descriptor
@int duration if non-zero, stream for some implementation defined time
@return 0 if successful, otherwise nil
@return error message if failed
*/
static int
Ptcsendbreak(lua_State *L)
{
	int fd = checkint(L, 1);
	int duration = checkint(L, 2);
	checknargs(L, 2);
	return pushresult(L, tcsendbreak(fd, duration), NULL);
}


/***
Set termios state.
@function tcsetattr
@int fd terminal descriptor to act on
@int actions bitwise OR of one or more of `TCSANOW`, `TCSADRAIN`,
  `TCSAFLUSH` and `TSASOFT`
@tparam termios a table with fields from iflag, oflag, cflag, lflag and cc,
 each formed by `bor` operations with various posix constants
@return 0 if successful, otherwise nil
@return error message if failed
@see tcsetattr(3)
@usage
ok, errmsg = tcsetattr (fd, 0, { cc = { [P.VTIME] = 0, [P.VMIN] = 1 })
*/
static int
Ptcsetattr(lua_State *L)
{
	struct termios t;
	int i;
	int fd = checkint(L, 1);
	int act = checkint(L, 2);
	luaL_checktype(L, 3, LUA_TTABLE);
	checknargs(L, 3);

	lua_getfield(L, 3, "iflag"); t.c_iflag = luaL_optint(L, -1, 0); lua_pop(L, 1);
	lua_getfield(L, 3, "oflag"); t.c_oflag = luaL_optint(L, -1, 0); lua_pop(L, 1);
	lua_getfield(L, 3, "cflag"); t.c_cflag = luaL_optint(L, -1, 0); lua_pop(L, 1);
	lua_getfield(L, 3, "lflag"); t.c_lflag = luaL_optint(L, -1, 0); lua_pop(L, 1);

	lua_getfield(L, 3, "cc");
	for (i=0; i<NCCS; i++)
	{
		lua_pushnumber(L, i);
		lua_gettable(L, -2);
		t.c_cc[i] = luaL_optint(L, -1, 0);
		lua_pop(L, 1);
	}

	return pushresult(L, tcsetattr(fd, act, &t), NULL);
}


static void
termio_setconst(lua_State *L)
{
	/* tcsetattr */
	PCONST( TCSANOW		);
	PCONST( TCSADRAIN	);
	PCONST( TCSAFLUSH	);

	/* tcflush */
	PCONST( TCIFLUSH	);
	PCONST( TCOFLUSH	);
	PCONST( TCIOFLUSH	);

	/* tcflow() */
	PCONST( TCOOFF		);
	PCONST( TCOON		);
	PCONST( TCIOFF		);
	PCONST( TCION		);

	/* cflag */
#ifdef B0
	PCONST( B0		);
#endif
#ifdef B50
	PCONST( B50		);
#endif
#ifdef B75
	PCONST( B75		);
#endif
#ifdef B110
	PCONST( B110		);
#endif
#ifdef B134
	PCONST( B134		);
#endif
#ifdef B150
	PCONST( B150		);
#endif
#ifdef B200
	PCONST( B200		);
#endif
#ifdef B300
	PCONST( B300		);
#endif
#ifdef B600
	PCONST( B600		);
#endif
#ifdef B1200
	PCONST( B1200		);
#endif
#ifdef B1800
	PCONST( B1800		);
#endif
#ifdef B2400
	PCONST( B2400		);
#endif
#ifdef B4800
	PCONST( B4800		);
#endif
#ifdef B9600
	PCONST( B9600		);
#endif
#ifdef B19200
	PCONST( B19200		);
#endif
#ifdef B38400
	PCONST( B38400		);
#endif
#ifdef B57600
	PCONST( B57600		);
#endif
#ifdef B115200
	PCONST( B115200		);
#endif
#ifdef CSIZE
	PCONST( CSIZE		);
#endif
#ifdef BS5
	PCONST( CS5		);
#endif
#ifdef CS6
	PCONST( CS6		);
#endif
#ifdef CS7
	PCONST( CS7		);
#endif
#ifdef CS8
	PCONST( CS8		);
#endif
#ifdef CSTOPB
	PCONST( CSTOPB		);
#endif
#ifdef CREAD
	PCONST( CREAD		);
#endif
#ifdef PARENB
	PCONST( PARENB		);
#endif
#ifdef PARODD
	PCONST( PARODD		);
#endif
#ifdef HUPCL
	PCONST( HUPCL		);
#endif
#ifdef CLOCAL
	PCONST( CLOCAL		);
#endif
#ifdef CRTSCTS
	PCONST( CRTSCTS		);
#endif

	/* lflag */
#ifdef ISIG
	PCONST( ISIG		);
#endif
#ifdef ICANON
	PCONST( ICANON		);
#endif
#ifdef ECHO
	PCONST( ECHO		);
#endif
#ifdef ECHOE
	PCONST( ECHOE		);
#endif
#ifdef ECHOK
	PCONST( ECHOK		);
#endif
#ifdef ECHONL
	PCONST( ECHONL		);
#endif
#ifdef NOFLSH
	PCONST( NOFLSH		);
#endif
#ifdef IEXTEN
	PCONST( IEXTEN		);
#endif
#ifdef TOSTOP
	PCONST( TOSTOP		);
#endif

	/* iflag */
#ifdef INPCK
	PCONST( INPCK		);
#endif
#ifdef IGNPAR
	PCONST( IGNPAR		);
#endif
#ifdef PARMRK
	PCONST( PARMRK		);
#endif
#ifdef ISTRIP
	PCONST( ISTRIP		);
#endif
#ifdef IXON
	PCONST( IXON		);
#endif
#ifdef IXOFF
	PCONST( IXOFF		);
#endif
#ifdef IXANY
	PCONST( IXANY		);
#endif
#ifdef IGNBRK
	PCONST( IGNBRK		);
#endif
#ifdef BRKINT
	PCONST( BRKINT		);
#endif
#ifdef INLCR
	PCONST( INLCR		);
#endif
#ifdef IGNCR
	PCONST( IGNCR		);
#endif
#ifdef ICRNL
	PCONST( ICRNL		);
#endif
#ifdef IMAXBEL
	PCONST( IMAXBEL		);
#endif

	/* oflag */
#ifdef OPOST
	PCONST( OPOST		);
#endif
#ifdef ONLCR
	PCONST( ONLCR		);
#endif
#ifdef OCRNL
	PCONST( OCRNL		);
#endif
#ifdef ONLRET
	PCONST( ONLRET		);
#endif
#ifdef OFILL
	PCONST( OFILL		);
#endif
#ifdef OFDEL
	PCONST( OFDEL		);
#endif
#ifdef NLDLY
	PCONST( NLDLY		);
#endif
#ifdef NL0
	PCONST( NL0		);
#endif
#ifdef NL1
	PCONST( NL1		);
#endif
#ifdef CRDLY
	PCONST( CRDLY		);
#endif
#ifdef CR0
	PCONST( CR0		);
#endif
#ifdef CR1
	PCONST( CR1		);
#endif
#ifdef CR2
	PCONST( CR2		);
#endif
#ifdef CR3
	PCONST( CR3		);
#endif
#ifdef TABDLY
	PCONST( TABDLY		);
#endif
#ifdef TAB0
	PCONST( TAB0		);
#endif
#ifdef TAB1
	PCONST( TAB1		);
#endif
#ifdef TAB2
	PCONST( TAB2		);
#endif
#ifdef TAB3
	PCONST( TAB3		);
#endif
#ifdef BSDLY
	PCONST( BSDLY		);
#endif
#ifdef BS0
	PCONST( BS0		);
#endif
#ifdef BS1
	PCONST( BS1		);
#endif
#ifdef VTDLY
	PCONST( VTDLY		);
#endif
#ifdef VT0
	PCONST( VT0		);
#endif
#ifdef VT1
	PCONST( VT1		);
#endif
#ifdef FFDLY
	PCONST( FFDLY		);
#endif
#ifdef FF0
	PCONST( FF0		);
#endif
#ifdef FF1
	PCONST( FF1		);
#endif

	/* cc */
#ifdef VINTR
	PCONST( VINTR		);
#endif
#ifdef VQUIT
	PCONST( VQUIT		);
#endif
#ifdef VERASE
	PCONST( VERASE		);
#endif
#ifdef VKILL
	PCONST( VKILL		);
#endif
#ifdef VEOF
	PCONST( VEOF		);
#endif
#ifdef VEOL
	PCONST( VEOL		);
#endif
#ifdef VEOL2
	PCONST( VEOL2		);
#endif
#ifdef VMIN
	PCONST( VMIN		);
#endif
#ifdef VTIME
	PCONST( VTIME		);
#endif
#ifdef VSTART
	PCONST( VSTART		);
#endif
#ifdef VSTOP
	PCONST( VSTOP		);
#endif
#ifdef VSUSP
	PCONST( VSUSP		);
#endif

	/* XSI extensions - don't use these if you care about portability
	 * to strict POSIX conforming machines, such as Mac OS X.
	 */
#ifdef CBAUD
	PCONST( CBAUD		);
#endif
#ifdef EXTA
	PCONST( EXTA		);
#endif
#ifdef EXTB
	PCONST( EXTB		);
#endif
#ifdef DEFECHO
	PCONST( DEFECHO		);
#endif
#ifdef ECHOCTL
	PCONST( ECHOCTL		);
#endif
#ifdef ECHOPRT
	PCONST( ECHOPRT		);
#endif
#ifdef ECHOKE
	PCONST( ECHOKE		);
#endif
#ifdef FLUSHO
	PCONST( FLUSHO		);
#endif
#ifdef PENDIN
	PCONST( PENDIN		);
#endif
#ifdef LOBLK
	PCONST( LOBLK		);
#endif
#ifdef SWTCH
	PCONST( SWTCH		);
#endif
#ifdef VDISCARD
	PCONST( VDISCARD	);
#endif
#ifdef VDSUSP
	PCONST( VDSUSP		);
#endif
#ifdef VLNEXT
	PCONST( VLNEXT		);
#endif
#ifdef VREPRINT
	PCONST( VREPRINT	);
#endif
#ifdef VSTATUS
	PCONST( VSTATUS		);
#endif
#ifdef VWERASE
	PCONST( VWERASE		);
#endif
}
