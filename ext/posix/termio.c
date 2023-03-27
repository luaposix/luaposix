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
 Control Terminal I/O.

 Functions and constants for controlling terminal behaviour.

@module posix.termio
*/

#include <termios.h>

#include "_helpers.c"


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
The constants named below are all available in this submodule's namespace,
as long as they are supported by the underlying system.
@table termios
@int cflag bitwise OR of zero or more of
  `CSIZE`, `CS5`, `CS6`, `CS7`, `CS8`, `CSTOPB`, `CREAD`, `PARENB`,
  `PARODD`, `HUPCL`, `CLOCAL` and `CRTSCTS`
@int iflag input flags; bitwise OR of zero or more of `IGNBRK`, `BRKINT`,
  `IGNPAR`, `PARMRK`, `INPCK`, `ISTRIP`, `INLCR`, `IGNCR`, `ICRNL`,
  `IXON`, `IXOFF`, `IXANY`, and `IMAXBEL`
@int lflag local flags; bitwise OR of zero or more of `ISIG`, `ICANON`,
  `ECHO`, `ECHOE`, `ECHOK', 'ECHONL`, `NOFLSH`, `IEXTEN` and `TOSTOP`
@int oflag output flags; bitwise OR of zero or more of `OPOST`, `ONLCR`,
  `OXTABS`, `ONOEOT`, `OCRNL`, `ONOCR`, `ONLRET`, `OFILL`, `NLDLY`,
  `TABDLY`, `CRDLY`, `FFDLY`, `BSDLY`, `VTDLY` and `OFDEL`
@tfield ccs cc array of terminal control characters
@int ispeed the input baud rate, one of `B0`, `B50`, `B75`, `B110`,
  `B134`, `B150`, `B200`, `B300`, `B600`, `B1200`, `B1800`, `B2400`,
  `B4800`, `B9600`, `B19200`, `B38400`, `B57600`, `B115200`,
@int ospeed the output baud rate (see ispeed for possible values)
*/


/***
Wait for all written output to reach the terminal.
@function tcdrain
@int fd terminal descriptor to act on
@treturn[1] int `0`, if successful
@return[2] nil
@treturn[2] string error message
@treturn[2] int errnum
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
@treturn[2] int errnum
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
@treturn[2] int errnum
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
@treturn[2] int errnum
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
	pushintegerfield("iflag", t.c_iflag);
	pushintegerfield("oflag", t.c_oflag);
	pushintegerfield("lflag", t.c_lflag);
	pushintegerfield("cflag", t.c_cflag);
	pushintegerfield("ispeed",cfgetispeed(&t));
	pushintegerfield("ospeed",cfgetospeed(&t));

	lua_newtable(L);
	for (i=0; i<NCCS; i++)
	{
		lua_pushinteger(L, i);
		lua_pushinteger(L, t.c_cc[i]);
		lua_settable(L, -3);
	}
	lua_setfield(L, -2, "cc");

	return 1;
}


/***
Send a stream of zero valued bits.
@function tcsendbreak
@int fd terminal descriptor
@int duration if non-zero, stream for some implementation defined time
@treturn[1] int `0`, if successful
@return[2] nil
@treturn[2] string error message
@treturn[2] int errnum
@see tcsendbreak(3)
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
@treturn[1] int `0`, if successful
@return[2] nil
@treturn[2] string error message
@treturn[2] int errnum
@see tcsetattr(3)
@usage
  local termio = require "posix.termio"
  ok, errmsg = tcsetattr (fd, 0, { cc = { [termio.VTIME] = 0, [termio.VMIN] = 1 })
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

	lua_getfield(L, 3, "iflag"); t.c_iflag = optint(L, -1, 0); lua_pop(L, 1);
	lua_getfield(L, 3, "oflag"); t.c_oflag = optint(L, -1, 0); lua_pop(L, 1);
	lua_getfield(L, 3, "cflag"); t.c_cflag = optint(L, -1, 0); lua_pop(L, 1);
	lua_getfield(L, 3, "lflag"); t.c_lflag = optint(L, -1, 0); lua_pop(L, 1);
	lua_getfield(L, 3, "ispeed"); cfsetispeed( &t, optint(L, -1, B0) ); lua_pop(L, 1);
	lua_getfield(L, 3, "ospeed"); cfsetospeed( &t, optint(L, -1, B0) ); lua_pop(L, 1);

	lua_getfield(L, 3, "cc");
	for (i=0; i<NCCS; i++)
	{
		lua_pushinteger(L, i);
		lua_gettable(L, -2);
		t.c_cc[i] = optint(L, -1, 0);
		lua_pop(L, 1);
	}

	return pushresult(L, tcsetattr(fd, act, &t), NULL);
}


static const luaL_Reg posix_termio_fns[] =
{
	LPOSIX_FUNC( Ptcdrain		),
	LPOSIX_FUNC( Ptcflow		),
	LPOSIX_FUNC( Ptcflush		),
	LPOSIX_FUNC( Ptcgetattr		),
	LPOSIX_FUNC( Ptcsendbreak	),
	LPOSIX_FUNC( Ptcsetattr		),
	{NULL, NULL}
};


LUALIB_API int
luaopen_posix_termio(lua_State *L)
{
	luaL_newlib(L, posix_termio_fns);
	lua_pushstring(L, LPOSIX_VERSION_STRING("termio"));
	lua_setfield(L, -2, "version");

	/* tcsetattr */
	LPOSIX_CONST( TCSANOW		);
	LPOSIX_CONST( TCSADRAIN		);
	LPOSIX_CONST( TCSAFLUSH		);

	/* tcflush */
	LPOSIX_CONST( TCIFLUSH		);
	LPOSIX_CONST( TCOFLUSH		);
	LPOSIX_CONST( TCIOFLUSH		);

	/* tcflow() */
	LPOSIX_CONST( TCOOFF		);
	LPOSIX_CONST( TCOON		);
	LPOSIX_CONST( TCIOFF		);
	LPOSIX_CONST( TCION		);

	/* cflag */
#ifdef B0
	LPOSIX_CONST( B0		);
#endif
#ifdef B50
	LPOSIX_CONST( B50		);
#endif
#ifdef B75
	LPOSIX_CONST( B75		);
#endif
#ifdef B110
	LPOSIX_CONST( B110		);
#endif
#ifdef B134
	LPOSIX_CONST( B134		);
#endif
#ifdef B150
	LPOSIX_CONST( B150		);
#endif
#ifdef B200
	LPOSIX_CONST( B200		);
#endif
#ifdef B300
	LPOSIX_CONST( B300		);
#endif
#ifdef B600
	LPOSIX_CONST( B600		);
#endif
#ifdef B1200
	LPOSIX_CONST( B1200		);
#endif
#ifdef B1800
	LPOSIX_CONST( B1800		);
#endif
#ifdef B2400
	LPOSIX_CONST( B2400		);
#endif
#ifdef B4800
	LPOSIX_CONST( B4800		);
#endif
#ifdef B9600
	LPOSIX_CONST( B9600		);
#endif
#ifdef B19200
	LPOSIX_CONST( B19200		);
#endif
#ifdef B38400
	LPOSIX_CONST( B38400		);
#endif
#ifdef B57600
	LPOSIX_CONST( B57600		);
#endif
#ifdef B115200
	LPOSIX_CONST( B115200		);
#endif
#ifdef CSIZE
	LPOSIX_CONST( CSIZE		);
#endif
#ifdef BS5
	LPOSIX_CONST( CS5		);
#endif
#ifdef CS6
	LPOSIX_CONST( CS6		);
#endif
#ifdef CS7
	LPOSIX_CONST( CS7		);
#endif
#ifdef CS8
	LPOSIX_CONST( CS8		);
#endif
#ifdef CSTOPB
	LPOSIX_CONST( CSTOPB		);
#endif
#ifdef CREAD
	LPOSIX_CONST( CREAD		);
#endif
#ifdef PARENB
	LPOSIX_CONST( PARENB		);
#endif
#ifdef PARODD
	LPOSIX_CONST( PARODD		);
#endif
#ifdef HUPCL
	LPOSIX_CONST( HUPCL		);
#endif
#ifdef CLOCAL
	LPOSIX_CONST( CLOCAL		);
#endif
#ifdef CRTSCTS
	LPOSIX_CONST( CRTSCTS		);
#endif

	/* lflag */
#ifdef ISIG
	LPOSIX_CONST( ISIG		);
#endif
#ifdef ICANON
	LPOSIX_CONST( ICANON		);
#endif
#ifdef ECHO
	LPOSIX_CONST( ECHO		);
#endif
#ifdef ECHOE
	LPOSIX_CONST( ECHOE		);
#endif
#ifdef ECHOK
	LPOSIX_CONST( ECHOK		);
#endif
#ifdef ECHONL
	LPOSIX_CONST( ECHONL		);
#endif
#ifdef NOFLSH
	LPOSIX_CONST( NOFLSH		);
#endif
#ifdef IEXTEN
	LPOSIX_CONST( IEXTEN		);
#endif
#ifdef TOSTOP
	LPOSIX_CONST( TOSTOP		);
#endif

	/* iflag */
#ifdef INPCK
	LPOSIX_CONST( INPCK		);
#endif
#ifdef IGNPAR
	LPOSIX_CONST( IGNPAR		);
#endif
#ifdef PARMRK
	LPOSIX_CONST( PARMRK		);
#endif
#ifdef ISTRIP
	LPOSIX_CONST( ISTRIP		);
#endif
#ifdef IXON
	LPOSIX_CONST( IXON		);
#endif
#ifdef IXOFF
	LPOSIX_CONST( IXOFF		);
#endif
#ifdef IXANY
	LPOSIX_CONST( IXANY		);
#endif
#ifdef IGNBRK
	LPOSIX_CONST( IGNBRK		);
#endif
#ifdef BRKINT
	LPOSIX_CONST( BRKINT		);
#endif
#ifdef INLCR
	LPOSIX_CONST( INLCR		);
#endif
#ifdef IGNCR
	LPOSIX_CONST( IGNCR		);
#endif
#ifdef ICRNL
	LPOSIX_CONST( ICRNL		);
#endif
#ifdef IMAXBEL
	LPOSIX_CONST( IMAXBEL		);
#endif

	/* oflag */
#ifdef OPOST
	LPOSIX_CONST( OPOST		);
#endif
#ifdef ONLCR
	LPOSIX_CONST( ONLCR		);
#endif
#ifdef OCRNL
	LPOSIX_CONST( OCRNL		);
#endif
#ifdef ONLRET
	LPOSIX_CONST( ONLRET		);
#endif
#ifdef OFILL
	LPOSIX_CONST( OFILL		);
#endif
#ifdef OFDEL
	LPOSIX_CONST( OFDEL		);
#endif
#ifdef NLDLY
	LPOSIX_CONST( NLDLY		);
#endif
#ifdef NL0
	LPOSIX_CONST( NL0		);
#endif
#ifdef NL1
	LPOSIX_CONST( NL1		);
#endif
#ifdef CRDLY
	LPOSIX_CONST( CRDLY		);
#endif
#ifdef CR0
	LPOSIX_CONST( CR0		);
#endif
#ifdef CR1
	LPOSIX_CONST( CR1		);
#endif
#ifdef CR2
	LPOSIX_CONST( CR2		);
#endif
#ifdef CR3
	LPOSIX_CONST( CR3		);
#endif
#ifdef TABDLY
	LPOSIX_CONST( TABDLY		);
#endif
#ifdef TAB0
	LPOSIX_CONST( TAB0		);
#endif
#ifdef TAB1
	LPOSIX_CONST( TAB1		);
#endif
#ifdef TAB2
	LPOSIX_CONST( TAB2		);
#endif
#ifdef TAB3
	LPOSIX_CONST( TAB3		);
#endif
#ifdef BSDLY
	LPOSIX_CONST( BSDLY		);
#endif
#ifdef BS0
	LPOSIX_CONST( BS0		);
#endif
#ifdef BS1
	LPOSIX_CONST( BS1		);
#endif
#ifdef VTDLY
	LPOSIX_CONST( VTDLY		);
#endif
#ifdef VT0
	LPOSIX_CONST( VT0		);
#endif
#ifdef VT1
	LPOSIX_CONST( VT1		);
#endif
#ifdef FFDLY
	LPOSIX_CONST( FFDLY		);
#endif
#ifdef FF0
	LPOSIX_CONST( FF0		);
#endif
#ifdef FF1
	LPOSIX_CONST( FF1		);
#endif

	/* cc */
#ifdef VINTR
	LPOSIX_CONST( VINTR		);
#endif
#ifdef VQUIT
	LPOSIX_CONST( VQUIT		);
#endif
#ifdef VERASE
	LPOSIX_CONST( VERASE		);
#endif
#ifdef VKILL
	LPOSIX_CONST( VKILL		);
#endif
#ifdef VEOF
	LPOSIX_CONST( VEOF		);
#endif
#ifdef VEOL
	LPOSIX_CONST( VEOL		);
#endif
#ifdef VEOL2
	LPOSIX_CONST( VEOL2		);
#endif
#ifdef VMIN
	LPOSIX_CONST( VMIN		);
#endif
#ifdef VTIME
	LPOSIX_CONST( VTIME		);
#endif
#ifdef VSTART
	LPOSIX_CONST( VSTART		);
#endif
#ifdef VSTOP
	LPOSIX_CONST( VSTOP		);
#endif
#ifdef VSUSP
	LPOSIX_CONST( VSUSP		);
#endif

	/* XSI extensions - don't use these if you care about portability
	 * to strict POSIX conforming machines, such as Mac OS X.
	 */
#ifdef CBAUD
	LPOSIX_CONST( CBAUD		);
#endif
#ifdef EXTA
	LPOSIX_CONST( EXTA		);
#endif
#ifdef EXTB
	LPOSIX_CONST( EXTB		);
#endif
#ifdef DEFECHO
	LPOSIX_CONST( DEFECHO		);
#endif
#ifdef ECHOCTL
	LPOSIX_CONST( ECHOCTL		);
#endif
#ifdef ECHOPRT
	LPOSIX_CONST( ECHOPRT		);
#endif
#ifdef ECHOKE
	LPOSIX_CONST( ECHOKE		);
#endif
#ifdef FLUSHO
	LPOSIX_CONST( FLUSHO		);
#endif
#ifdef IUTF8
	LPOSIX_CONST( IUTF8		);
#endif
#ifdef PENDIN
	LPOSIX_CONST( PENDIN		);
#endif
#ifdef LOBLK
	LPOSIX_CONST( LOBLK		);
#endif
#ifdef SWTCH
	LPOSIX_CONST( SWTCH		);
#endif
#ifdef VDISCARD
	LPOSIX_CONST( VDISCARD		);
#endif
#ifdef VDSUSP
	LPOSIX_CONST( VDSUSP		);
#endif
#ifdef VLNEXT
	LPOSIX_CONST( VLNEXT		);
#endif
#ifdef VREPRINT
	LPOSIX_CONST( VREPRINT		);
#endif
#ifdef VSTATUS
	LPOSIX_CONST( VSTATUS		);
#endif
#ifdef VWERASE
	LPOSIX_CONST( VWERASE		);
#endif

	return 1;
}
