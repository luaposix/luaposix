/***
 Curses binding for Lua 5.1/5.2.

 In the C library, the following functions:

     getstr()   (and wgetstr(), mvgetstr(), and mvwgetstr())
     inchstr()  (and winchstr(), mvinchstr(), and mvwinchstr())
     instr()    (and winstr(), mvinstr(), and mvwinstr())

 are subject to buffer overflow attack. This is because you pass in the
 buffer to be filled in, which has to be of finite length. But in this
 Lua module, a buffer is assigned automatically and the function returns
 the string, so there is no security issue. You may still use the alternate
 functions:

     s = stdscr:getnstr()
     s = stdscr:inchnstr()
     s = stdscr:innstr()

 which take an extra "size of buffer" argument, in order to impose a maximum
 length on the string the user may type in.

 Some of the C functions beginning with "no" do not exist in Lua. You should
 use `curses.nl(0)` and `curses.nl(1)` instead of `nonl()` and `nl()`, and
 likewise `curses.echo(0)` and `curses.echo(1)` instead of `noecho()` and
 `echo()` .

 In this Lua module the `stdscr:getch()` function always returns an integer.
 In C, a single character is an integer, but in Lua (and Perl) a single
 character is a short string. The Perl Curses function `getch()` returns a
 char if it was a char, and a number if it was a constant; to get this
 behaviour in Lua you have to convert explicitly, e.g.:

     if c < 256 then c = string.char(c) end

 Some Lua functions take a different set of parameters than their C
 counterparts; for example, you should use `str = stdscr.getstr()` and
 `y, x = stdscr.getyx()` instead of `getstr(str)` and `getyx(y, x)`, and
 likewise for `getbegyx` and `getmaxyx` and `getparyx` and `pair_content`.
 The Perl Curses module now uses the C-compatible parameters, so be aware of
 this difference when translating code from Perl into Lua, as well as from C
 into Lua.

 Many curses functions have variants starting with the prefixes `w-`, `mv-`,
 and/or `wmv-`. These variants differ only in the explicit addition of a
 window, or by the addition of two coordinates that are used to move the
 cursor first. For example, `addch()` has three other variants: `waddch()`,
 `mvaddch()`, and `mvwaddch()`.

@module posix.curses
*/
/*
 * Curses binding for Lua 5.1/5.2.
 *
 * (c) Gary V. Vaughan <gary@vaughan.pe> 2013-2014
 * (c) Reuben Thomas <rrt@sc3d.org> 2009-2012
 * (c) Tiago Dionizio <tiago.dionizio AT gmail.com> 2004-2007
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <config.h>

#if HAVE_CURSES

#include "_helpers.c"
#include "strlcpy.c"

#include "curses/chstr.c"
#include "curses/window.c"


static const char *STDSCR_REGISTRY	= "curses:stdscr";
static const char *RIPOFF_TABLE		= "curses:ripoffline";


/***
Create a new line drawing buffer instance.
@function new_chstr
@int len number of element to allocate
@treturn chstr a new char buffer object
@see posix.curses.chstr
*/
static int
Pnew_chstr(lua_State *L)
{
	int len = checkint(L, 1);
	chstr* ncs = chstr_new(L, len);  /* defined in curses/chstr.c */
	memset(ncs->str, ' ', len*sizeof(chtype));
	return 1;
}



#define CCR(n, v)				\
	lua_pushstring(L, n);			\
	lua_pushinteger(L, v);			\
	lua_settable(L, lua_upvalueindex(1));

#define CC(s)	   CCR(#s, s)
#define CF(i)	   CCR(LPOSIX_STR(LPOSIX_SPLICE(KEY_F, i)), KEY_F(i))

/*
** these values may be fixed only after initialization, so this is
** called from Pinitscr, after the curses driver is initialized
**
** curses table is kept at upvalue position 1, in case the global
** name is changed by the user or even in the registration phase by
** the developer
**
** some of these values are not constant so need to register
** them directly instead of using a table
*/

static void
register_curses_constants(lua_State *L)
{
	/* colors */
	CC(COLOR_BLACK)		CC(COLOR_RED)		CC(COLOR_GREEN)
	CC(COLOR_YELLOW)	CC(COLOR_BLUE)		CC(COLOR_MAGENTA)
	CC(COLOR_CYAN)		CC(COLOR_WHITE)

	/* alternate character set */
	CC(ACS_BLOCK)		CC(ACS_BOARD)

	CC(ACS_BTEE)		CC(ACS_TTEE)
	CC(ACS_LTEE)		CC(ACS_RTEE)
	CC(ACS_LLCORNER)	CC(ACS_LRCORNER)
	CC(ACS_URCORNER)	CC(ACS_ULCORNER)

	CC(ACS_LARROW)		CC(ACS_RARROW)
	CC(ACS_UARROW)		CC(ACS_DARROW)

	CC(ACS_HLINE)		CC(ACS_VLINE)

	CC(ACS_BULLET)		CC(ACS_CKBOARD)		CC(ACS_LANTERN)
	CC(ACS_DEGREE)		CC(ACS_DIAMOND)

	CC(ACS_PLMINUS)		CC(ACS_PLUS)
	CC(ACS_S1)		CC(ACS_S9)

	/* attributes */
	CC(A_NORMAL)		CC(A_STANDOUT)		CC(A_UNDERLINE)
	CC(A_REVERSE)		CC(A_BLINK)		CC(A_DIM)
	CC(A_BOLD)		CC(A_PROTECT)		CC(A_INVIS)
	CC(A_ALTCHARSET)	CC(A_CHARTEXT)
	CC(A_ATTRIBUTES)

	/* key functions */
	CC(KEY_BREAK)		CC(KEY_DOWN)		CC(KEY_UP)
	CC(KEY_LEFT)		CC(KEY_RIGHT)		CC(KEY_HOME)
	CC(KEY_BACKSPACE)

	CC(KEY_DL)		CC(KEY_IL)		CC(KEY_DC)
	CC(KEY_IC)		CC(KEY_EIC)		CC(KEY_CLEAR)
	CC(KEY_EOS)		CC(KEY_EOL)		CC(KEY_SF)
	CC(KEY_SR)		CC(KEY_NPAGE)		CC(KEY_PPAGE)
	CC(KEY_STAB)		CC(KEY_CTAB)		CC(KEY_CATAB)
	CC(KEY_ENTER)		CC(KEY_SRESET)		CC(KEY_RESET)
	CC(KEY_PRINT)		CC(KEY_LL)		CC(KEY_A1)
	CC(KEY_A3)		CC(KEY_B2)		CC(KEY_C1)
	CC(KEY_C3)		CC(KEY_BTAB)		CC(KEY_BEG)
	CC(KEY_CANCEL)		CC(KEY_CLOSE)		CC(KEY_COMMAND)
	CC(KEY_COPY)		CC(KEY_CREATE)		CC(KEY_END)
	CC(KEY_EXIT)		CC(KEY_FIND)		CC(KEY_HELP)
	CC(KEY_MARK)		CC(KEY_MESSAGE)		CC(KEY_MOUSE)
	CC(KEY_MOVE)		CC(KEY_NEXT)		CC(KEY_OPEN)
	CC(KEY_OPTIONS)		CC(KEY_PREVIOUS)	CC(KEY_REDO)
	CC(KEY_REFERENCE)	CC(KEY_REFRESH)		CC(KEY_REPLACE)
	CC(KEY_RESIZE)		CC(KEY_RESTART)		CC(KEY_RESUME)
	CC(KEY_SAVE)		CC(KEY_SBEG)		CC(KEY_SCANCEL)
	CC(KEY_SCOMMAND)	CC(KEY_SCOPY)		CC(KEY_SCREATE)
	CC(KEY_SDC)		CC(KEY_SDL)		CC(KEY_SELECT)
	CC(KEY_SEND)		CC(KEY_SEOL)		CC(KEY_SEXIT)
	CC(KEY_SFIND)		CC(KEY_SHELP)		CC(KEY_SHOME)
	CC(KEY_SIC)		CC(KEY_SLEFT)		CC(KEY_SMESSAGE)
	CC(KEY_SMOVE)		CC(KEY_SNEXT)		CC(KEY_SOPTIONS)
	CC(KEY_SPREVIOUS)	CC(KEY_SPRINT)		CC(KEY_SREDO)
	CC(KEY_SREPLACE)	CC(KEY_SRIGHT)		CC(KEY_SRSUME)
	CC(KEY_SSAVE)		CC(KEY_SSUSPEND)	CC(KEY_SUNDO)
	CC(KEY_SUSPEND)		CC(KEY_UNDO)

	/* KEY_Fx  0 <= x <= 63 */
	CC(KEY_F0)
	CF(1)  CF(2)  CF(3)  CF(4)  CF(5)  CF(6)  CF(7)  CF(8)
	CF(9)  CF(10) CF(11) CF(12) CF(13) CF(14) CF(15) CF(16)
	CF(17) CF(18) CF(19) CF(20) CF(21) CF(22) CF(23) CF(24)
	CF(25) CF(26) CF(27) CF(28) CF(29) CF(30) CF(21) CF(32)
	CF(33) CF(34) CF(35) CF(36) CF(37) CF(38) CF(39) CF(40)
	CF(41) CF(42) CF(43) CF(44) CF(45) CF(46) CF(47) CF(48)
	CF(49) CF(50) CF(51) CF(52) CF(53) CF(54) CF(55) CF(56)
	CF(57) CF(58) CF(59) CF(60) CF(61) CF(62) CF(63)
}

/*
** make sure screen is restored (and cleared) at exit
** (for the situations where program is aborted without a
** proper cleanup)
*/

static void
cleanup(void)
{
	if (!isendwin())
	{
		wclear(stdscr);
		wrefresh(stdscr);
		endwin();
	}
}


/***
Initialise screen.
@function initscr
@treturn window main screen
@see curs_initscr(3x)
*/
static int
Pinitscr(lua_State *L)
{
	WINDOW *w;

	/* initialize curses */
	w = initscr();

	/* no longer used, so clean it up */
	lua_pushstring(L, RIPOFF_TABLE);
	lua_pushnil(L);
	lua_settable(L, LUA_REGISTRYINDEX);

	/* failed to initialize */
	if (w == NULL)
		return 0;

	/* return stdscr - main window */
	lc_newwin(L, w);

	/* save main window on registry */
	lua_pushstring(L, STDSCR_REGISTRY);
	lua_pushvalue(L, -2);
	lua_rawset(L, LUA_REGISTRYINDEX);

	/* setup curses constants - curses.xxx numbers */
	register_curses_constants(L);

	/* install cleanup handler to help in debugging and screen trashing */
	atexit(cleanup);

	return 1;
}


/***
Clean up terminal prior to exiting or escaping curses.
@function endwin
@treturn bool `true`, if successful
@see curs_initscr(3x)
*/
static int
Pendwin(lua_State *L)
{
	return pushokresult(endwin());
}


/***
@function isendwin
@treturn bool whether @{endwin} has been called
@see curs_initscr(3x)
*/
static int
Pisendwin(lua_State *L)
{
	return pushboolresult(isendwin());
}


/***
@function stdscr
@treturn window main screen
*/
static int
Pstdscr(lua_State *L)
{
	lua_pushstring(L, STDSCR_REGISTRY);
	lua_rawget(L, LUA_REGISTRYINDEX);
	return 1;
}


/***
@function cols
@treturn int number of columns in the main screen
*/
static int
Pcols(lua_State *L)
{
	return pushintresult(COLS);
}


/***
@function lines
@treturn int number of lines in the main screen
*/
static int
Plines(lua_State *L)
{
	return pushintresult(LINES);
}


/***
@function start_color
@treturn bool `true`, if successful
@see curs_color(3x)
*/
static int
Pstart_color(lua_State *L)
{
	return pushokresult(start_color());
}


/***
@function has_colors
@treturn bool `true`, if the terminal supports colors
@see curs_color(3x)
*/
static int
Phas_colors(lua_State *L)
{
	return pushboolresult(has_colors());
}


/***
@function use_default_colors
@treturn bool `true`, if successful
@see default_colors(3x)
*/
static int
Puse_default_colors(lua_State *L)
{
	return pushokresult(use_default_colors());
}


/***
@function init_pair
@int pair color pair id to act on
@int f foreground color to assign
@int b background color to assign
@treturn bool `true`, if successful
@see curs_color(3x)
*/
static int
Pinit_pair(lua_State *L)
{
	short pair = checkint(L, 1);
	short f = checkint(L, 2);
	short b = checkint(L, 3);
	return pushokresult(init_pair(pair, f, b));
}


/***
@function pair_content
@int pair color pair id to act on
@treturn int foreground color of *pair*
@treturn int background color of *pair*
@see curs_color(3x)
*/
static int
Ppair_content(lua_State *L)
{
	short pair = checkint(L, 1);
	short f;
	short b;
	int ret = pair_content(pair, &f, &b);

	if (ret == ERR)
		return 0;

	lua_pushinteger(L, f);
	lua_pushinteger(L, b);
	return 2;
}


/***
@function colors
@treturn int total number of available colors
@see curs_color(3x)
*/
static int
Pcolors(lua_State *L)
{
	return pushintresult(COLORS);
}


/***
@function color_pairs
@treturn int total number of available color pairs
@see curs_color(3x)
*/
static int
Pcolor_pairs(lua_State *L)
{
	return pushintresult(COLOR_PAIRS);
}


/***
@function color_pair
@int pair color pair id to act on
@treturn int attributes for color pair *pair*
@see curs_color(3x)
*/
static int
Pcolor_pair(lua_State *L)
{
	int n = checkint(L, 1);
	return pushintresult(COLOR_PAIR(n));
}


/***
@function baudrate
@treturn int output speed of the terminal in bits-per-second
@see curs_termattrs(3x)
*/
static int
Pbaudrate(lua_State *L)
{
	return pushintresult(baudrate());
}


/***
@function erasechar
@treturn int current erase character
@see curs_termattrs(3x)
*/
static int
Perasechar(lua_State *L)
{
	return pushintresult(erasechar());
}


/***
@function has_ic
@treturn bool `true`, if the terminal has insert and delete character
  operations
@see curs_termattrs(3x)
*/
static int
Phas_ic(lua_State *L)
{
	return pushboolresult(has_ic());
}


/***
@function has_il
@treturn bool `true`, if the terminal has insert and delete line operations
@see curs_termattrs(3x)
*/
static int
Phas_il(lua_State *L)
{
	return pushboolresult(has_il());
}


/***
@function killchar
@treturn int current line kill character
@see curs_termattrs(3x)
*/
static int
Pkillchar(lua_State *L)
{
	return pushintresult(killchar());
}


/***
@function termattrs
@int[opt] a terminal attribute bits
@treturn[1] bool `true`, if the terminal supports attribute *a*
@treturn[2] int bitarray of supported terminal attributes
@see curs_termattrs(3x)
*/
static int
Ptermattrs(lua_State *L)
{
	if (lua_gettop(L) > 0)
	{
		int a = checkint(L, 1);
		return pushboolresult(termattrs() & a);
	}
	return pushintresult(termattrs());
}


/***
@function termname
@treturn string terminal name
@see curs_termattrs(3x)
*/
static int
Ptermname(lua_State *L)
{
	return pushstringresult(termname());
}


/***
@function longname
@treturn string verbose description of the current terminal
@see curs_termattrs(3x)
*/
static int
Plongname(lua_State *L)
{
	return pushstringresult(longname());
}


/* there is no easy way to implement this... */

static lua_State *rip_L = NULL;

static int
ripoffline_cb(WINDOW* w, int cols)
{
	static int line = 0;
	int top = lua_gettop(rip_L);

	/* better be safe */
	if (!lua_checkstack(rip_L, 5))
		return 0;

	/* get the table from the registry */
	lua_pushstring(rip_L, RIPOFF_TABLE);
	lua_gettable(rip_L, LUA_REGISTRYINDEX);

	/* get user callback function */
	if (lua_isnil(rip_L, -1)) {
		lua_pop(rip_L, 1);
		return 0;
	}

	lua_rawgeti(rip_L, -1, ++line);	/* function to be called */
	lc_newwin(rip_L, w);		/* create window object */
	lua_pushinteger(rip_L, cols);   /* push number of columns */

	lua_pcall(rip_L, 2,  0, 0);	/* call the lua function */

	lua_settop(rip_L, top);
	return 1;
}


/***
@function ripoffline
@bool top_line
@func callback
@treturn bool `true`, if successful
@see curs_kernel(3x)
*/
static int
Pripoffline(lua_State *L)
{
	static int rip = 0;
	int top_line = lua_toboolean(L, 1);

	if (!lua_isfunction(L, 2))
	{
		lua_pushliteral(L, "invalid callback passed as second parameter");
		lua_error(L);
	}

	/* need to save the lua state somewhere... */
	rip_L = L;

	/* get the table where we are going to save the callbacks */
	lua_pushstring(L, RIPOFF_TABLE);
	lua_gettable(L, LUA_REGISTRYINDEX);

	if (lua_isnil(L, -1))
	{
		lua_pop(L, 1);
		lua_newtable(L);

		lua_pushstring(L, RIPOFF_TABLE);
		lua_pushvalue(L, -2);
		lua_settable(L, LUA_REGISTRYINDEX);
	}

	/* save function callback in registry table */
	lua_pushvalue(L, 2);
	lua_rawseti(L, -2, ++rip);

	/* and tell curses we are going to take the line */
	return pushokresult(ripoffline(top_line ? 1 : -1, ripoffline_cb));
}


/***
@function curs_set
@int vis cursor state
@treturn int previous cursor state
@see curs_kernel(3x)
*/
static int
Pcurs_set(lua_State *L)
{
	int vis = checkint(L, 1);
	int state = curs_set(vis);
	if (state == ERR)
		return 0;

	return pushintresult(state);
}


/***
@function napms
@int ms time to wait in milliseconds
@treturn bool `true`, if successful
@see curs_kernel(3x)
*/
static int
Pnapms(lua_State *L)
{
	int ms = checkint(L, 1);
	return pushokresult(napms(ms));
}


/***
@function resizeterm
@int nlines number of lines
@int ncols number of columns
@treturn bool `true`, if successful
@raise unimplemented
*/
static int
Presizeterm(lua_State *L)
{
	int nlines  = checkint(L, 1);
	int ncols   = checkint(L, 2);
#if HAVE_RESIZETERM
	return pushokresult(resizeterm (nlines, ncols));
#else
	return luaL_error (L, "'resizeterm' is not implemented by your curses library");
#endif
}


/***
@function beep
@treturn bool `true`, if successful
@see curs_beep(3x)
*/
static int
Pbeep(lua_State *L)
{
	return pushokresult(beep());
}


/***
@function flash
@treturn bool `true`, if successful
@see curs_beep(3x)
*/
static int
Pflash(lua_State *L)
{
	return pushokresult(flash());
}


/***
@function newwin
@int nlines number of window lines
@int ncols number of window columns
@int begin_y top line of window
@int begin_x leftmost column of window
@treturn window a new window object
@see curs_window(3x)
*/
static int
Pnewwin(lua_State *L)
{
	int nlines  = checkint(L, 1);
	int ncols   = checkint(L, 2);
	int begin_y = checkint(L, 3);
	int begin_x = checkint(L, 4);

	lc_newwin(L, newwin(nlines, ncols, begin_y, begin_x));
	return 1;
}


/***
@function doupdate
@treturn bool `true`, if successful
@see curs_refresh(3x)
*/
static int
Pdoupdate(lua_State *L)
{
	return pushokresult(doupdate());
}


/***
@function slk_init
@int fmt
@treturn bool `true`, if successful
@see curs_slk(3x)
*/
static int
Pslk_init(lua_State *L)
{
	int fmt = checkint(L, 1);
	return pushokresult(slk_init(fmt));
}


/***
@function slk_set
@int labnum
@string label
@int fmt
@treturn bool `true`, if successful
@see curs_slk(3x)
*/
static int
Pslk_set(lua_State *L)
{
	int labnum = checkint(L, 1);
	const char* label = luaL_checkstring(L, 2);
	int fmt = checkint(L, 3);
	return pushokresult(slk_set(labnum, label, fmt));
}


/***
@function slk_refresh
@treturn bool `true`, if successful
@see curs_slk(3x)
*/
static int
Pslk_refresh(lua_State *L)
{
	return pushokresult(slk_refresh());
}


/***
@function slk_noutrefresh
@treturn bool `true`, if successful
@see curs_slk(3x)
*/
static int
Pslk_noutrefresh(lua_State *L)
{
	return pushokresult(slk_noutrefresh());
}


/***
@function slk_label
@int labnum
@treturn string current label for *labnum*
@see curs_slk(3x)
*/
static int
Pslk_label(lua_State *L)
{
	int labnum = checkint(L, 1);
	return pushstringresult(slk_label(labnum));
}


/***
@function slk_clear
@treturn bool `true`, if successful
@see curs_slk(3x)
*/
static int
Pslk_clear(lua_State *L)
{
	return pushokresult(slk_clear());
}


/***
@function slk_restore
@treturn bool `true`, if successful
@see curs_slk(3x)
*/
static int
Pslk_restore(lua_State *L)
{
	return pushokresult(slk_restore());
}


/***
@function slk_touch
@treturn bool `true`, if successful
@see curs_slk(3x)
*/
static int
Pslk_touch(lua_State *L)
{
	return pushokresult(slk_touch());
}


/***
@function slk_attron
@int attrs
@treturn bool `true`, if successful
@see curs_slk(3x)
*/
static int
Pslk_attron(lua_State *L)
{
	chtype attrs = checkch(L, 1);
	return pushokresult(slk_attron(attrs));
}


/***
@function slk_attroff
@int attrs
@treturn bool `true`, if successful
@see curs_slk(3x)
*/
static int
Pslk_attroff(lua_State *L)
{
	chtype attrs = checkch(L, 1);
	return pushokresult(slk_attroff(attrs));
}


/***
@function slk_attrset
@int attrs
@treturn bool `true`, if successful
@see curs_slk(3x)
*/
static int
Pslk_attrset(lua_State *L)
{
	chtype attrs = checkch(L, 1);
	return pushokresult(slk_attrset(attrs));
}


/***
@function cbreak
@bool[opt] on
@treturn bool `true`, if successful
@see curs_inopts(3x)
*/
static int
Pcbreak(lua_State *L)
{
	if (lua_isnoneornil(L, 1) || lua_toboolean(L, 1))
		return pushokresult(cbreak());
	return pushokresult(nocbreak());
}


/***
@function echo
@bool[opt] on
@treturn bool `true`, if successful
@see curs_inopts(3x)
*/
static int
Pecho(lua_State *L)
{
	if (lua_isnoneornil(L, 1) || lua_toboolean(L, 1))
		return pushokresult(echo());
	return pushokresult(noecho());
}


/***
@function raw
@bool[opt] on
@treturn bool `true`, if successful
@see curs_inopts(3x)
*/
static int
Praw(lua_State *L)
{
	if (lua_isnoneornil(L, 1) || lua_toboolean(L, 1))
		return pushokresult(raw());
	return pushokresult(noraw());
}


/***
@function halfdelay
@int tenths
@treturn bool `true`, if successful
@see curs_inopts(3x)
*/
static int
Phalfdelay(lua_State *L)
{
	int tenths = checkint(L, 1);
	return pushokresult(halfdelay(tenths));
}


/***
@function nl
@bool[opt] on
@treturn bool `true`, if successful
@see curs_outopts(3x)
*/
static int
Pnl(lua_State *L)
{
	if (lua_isnoneornil(L, 1) || lua_toboolean(L, 1))
		return pushokresult(nl());
	return pushokresult(nonl());
}


/***
@function unctrl
@int c character to act on
@treturn string printable representation of *c*
@see curs_util(3x)
*/
static int
Punctrl(lua_State *L)
{
	chtype c = luaL_checknumber(L, 1);
	return pushstringresult(unctrl(c));
}


/***
@function keyname
@int c a key
@treturn string key name of *c*
@see curs_util(3x)
*/
static int
Pkeyname(lua_State *L)
{
	int c = checkint(L, 1);
	return pushstringresult(keyname(c));
}


/***
@function delay_output
@int ms
@treturn bool `true`, if successful
@see curs_util(3x)
*/
static int
Pdelay_output(lua_State *L)
{
	int ms = checkint(L, 1);
	return pushokresult(delay_output(ms));
}


/***
@function flushinp
@treturn bool `true`, if successful
@see curs_util(3x)
*/
static int
Pflushinp(lua_State *L)
{
	return pushboolresult(flushinp());
}


/***
@function ungetch
@int c
@treturn bool `true`, if successful
@see curs_deleteln(3x)
*/
static int
Pungetch(lua_State *L)
{
	int c = checkint(L, 1);
	return pushokresult(ungetch(c));
}


/***
@function newpad
@int nlines
@int ncols
@treturn bool `true`, if successful
@see cur_pad(3x)
*/
static int
Pnewpad(lua_State *L)
{
	int nlines = checkint(L, 1);
	int ncols = checkint(L, 2);
	lc_newwin(L, newpad(nlines, ncols));
	return 1;
}


static char ti_capname[32];

/***
@function tigetflag
@string capname
@treturn bool content of terminal boolean capability
@see curs_terminfo(3x)
*/
static int
Ptigetflag (lua_State *L)
{
	int r;

	strlcpy (ti_capname, luaL_checkstring (L, 1), sizeof (ti_capname));
	r = tigetflag (ti_capname);
	if (-1 == r)
		return luaL_error (L, "`%s' is not a boolean capability", ti_capname);
	return pushboolresult (r);
}


/***
@function tigetnum
@string capname
@treturn int content of terminal numeric capability
@see curs_terminfo(3x)
*/
static int
Ptigetnum (lua_State *L)
{
	int res;

	strlcpy (ti_capname, luaL_checkstring (L, 1), sizeof (ti_capname));
	res = tigetnum (ti_capname);
	if (-2 == res)
		return luaL_error (L, "`%s' is not a numeric capability", ti_capname);
	else if (-1 == res)
		lua_pushnil (L);
	else
		lua_pushinteger(L, res);
	return 1;
}


/***
@function tigetstr
@string capname
@treturn string content of terminal string capability
@see curs_terminfo(3x)
*/
static int
Ptigetstr (lua_State *L)
{
	const char *res;

	strlcpy (ti_capname, luaL_checkstring (L, 1), sizeof (ti_capname));
	res = tigetstr (ti_capname);
	if ((char *) -1 == res)
		return luaL_error (L, "`%s' is not a string capability", ti_capname);
	else if (NULL == res)
		lua_pushnil (L);
	else
		lua_pushstring(L, res);
	return 1;
}


static const luaL_Reg curseslib[] =
{
	LPOSIX_FUNC( Pbaudrate		),
	LPOSIX_FUNC( Pbeep		),
	LPOSIX_FUNC( Pcbreak		),
	LPOSIX_FUNC( Pcolor_pair	),
	LPOSIX_FUNC( Pcolor_pairs	),
	LPOSIX_FUNC( Pcolors		),
	LPOSIX_FUNC( Pcols		),
	LPOSIX_FUNC( Pcurs_set		),
	LPOSIX_FUNC( Pdelay_output	),
	LPOSIX_FUNC( Pdoupdate		),
	LPOSIX_FUNC( Pecho		),
	LPOSIX_FUNC( Pendwin		),
	LPOSIX_FUNC( Perasechar		),
	LPOSIX_FUNC( Pflash		),
	LPOSIX_FUNC( Pflushinp		),
	LPOSIX_FUNC( Phalfdelay		),
	LPOSIX_FUNC( Phas_colors	),
	LPOSIX_FUNC( Phas_ic		),
	LPOSIX_FUNC( Phas_il		),
	LPOSIX_FUNC( Pinit_pair		),
	LPOSIX_FUNC( Pisendwin		),
	LPOSIX_FUNC( Pkeyname		),
	LPOSIX_FUNC( Pkillchar		),
	LPOSIX_FUNC( Plines		),
	LPOSIX_FUNC( Plongname		),
	LPOSIX_FUNC( Pnapms		),
	LPOSIX_FUNC( Pnew_chstr		),
	LPOSIX_FUNC( Pnewpad		),
	LPOSIX_FUNC( Pnewwin		),
	LPOSIX_FUNC( Pnl		),
	LPOSIX_FUNC( Ppair_content	),
	LPOSIX_FUNC( Praw		),
	LPOSIX_FUNC( Presizeterm	),
	LPOSIX_FUNC( Pripoffline	),
	LPOSIX_FUNC( Pslk_attroff	),
	LPOSIX_FUNC( Pslk_attron	),
	LPOSIX_FUNC( Pslk_attrset	),
	LPOSIX_FUNC( Pslk_clear		),
	LPOSIX_FUNC( Pslk_init		),
	LPOSIX_FUNC( Pslk_label		),
	LPOSIX_FUNC( Pslk_noutrefresh	),
	LPOSIX_FUNC( Pslk_refresh	),
	LPOSIX_FUNC( Pslk_restore	),
	LPOSIX_FUNC( Pslk_set		),
	LPOSIX_FUNC( Pslk_touch		),
	LPOSIX_FUNC( Pstart_color	),
	LPOSIX_FUNC( Pstdscr		),
	LPOSIX_FUNC( Ptermattrs		),
	LPOSIX_FUNC( Ptermname		),
	LPOSIX_FUNC( Ptigetflag		),
	LPOSIX_FUNC( Ptigetnum		),
	LPOSIX_FUNC( Ptigetstr		),
	LPOSIX_FUNC( Punctrl		),
	LPOSIX_FUNC( Pungetch		),
	LPOSIX_FUNC( Puse_default_colors),
	{NULL, NULL}
};
#endif

LUALIB_API int
luaopen_posix_curses(lua_State *L)
{
	luaopen_posix_curses_window(L);
	luaopen_posix_curses_chstr(L);

	luaL_register(L, "curses", curseslib);
	lua_pushliteral(L, "posix curses for " LUA_VERSION " / " PACKAGE_STRING);
	lua_setfield(L, -2, "version");

#if HAVE_CURSES
	lua_pushstring(L, "initscr");
	lua_pushvalue(L, -2);
	lua_pushcclosure(L, Pinitscr, 1);
	lua_settable(L, -3);
#endif

	return 1;
}
