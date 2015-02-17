/*
 * Curses binding for Lua 5.1, 5.2 & 5.3.
 *
 * (c) Gary V. Vaughan <gary@vaughan.pe> 2013-2015
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
/***
 Full-screen Text Terminal Manipulation.

 In the underlying curses C library, the following functions:

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
 use `curses.nl(false)` and `curses.nl(true)` instead of `nonl()` and `nl()`,
 and likewise `curses.echo(false)` and `curses.echo(true)` instead of
 `noecho()` and `echo()` .

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
 cursor first. For example, in C `addch()` has three other variants:
 `waddch()`, `mvaddch()` and `mvwaddch()`.  The Lua equivalents,
 respectively being `stdscr:addch()`, `somewindow:addch()`,
 `stdscr:mvaddch()` and `somewindow:mvaddch()`, with the window argument
 passed implicitly with Lua's `:` syntax sugar.

@module posix.curses
*/


#include <config.h>

#include "curses/chstr.c"
#include "curses/window.c"


#if HAVE_CURSES

#include "_helpers.c"
#include "strlcpy.c"


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
	lua_settable(L, lua_upvalueindex(1))

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
	CC(COLOR_BLACK);	CC(COLOR_RED);		CC(COLOR_GREEN);
	CC(COLOR_YELLOW);	CC(COLOR_BLUE);		CC(COLOR_MAGENTA);
	CC(COLOR_CYAN);		CC(COLOR_WHITE);

	/* alternate character set */
	CC(ACS_BLOCK);		CC(ACS_BOARD);

	CC(ACS_BTEE);		CC(ACS_TTEE);
	CC(ACS_LTEE);		CC(ACS_RTEE);
	CC(ACS_LLCORNER);	CC(ACS_LRCORNER);
	CC(ACS_URCORNER);	CC(ACS_ULCORNER);

	CC(ACS_LARROW);		CC(ACS_RARROW);
	CC(ACS_UARROW);		CC(ACS_DARROW);

	CC(ACS_HLINE);		CC(ACS_VLINE);

	CC(ACS_BULLET);		CC(ACS_CKBOARD);	CC(ACS_LANTERN);
	CC(ACS_DEGREE);		CC(ACS_DIAMOND);

	CC(ACS_PLMINUS);	CC(ACS_PLUS);
	CC(ACS_S1);		CC(ACS_S9);

	/* attributes */
	CC(A_NORMAL);		CC(A_STANDOUT);		CC(A_UNDERLINE);
	CC(A_REVERSE);		CC(A_BLINK);		CC(A_DIM);
	CC(A_BOLD);		CC(A_PROTECT);		CC(A_INVIS);
	CC(A_ALTCHARSET);	CC(A_CHARTEXT);
	CC(A_ATTRIBUTES);
#ifdef A_COLOR
	CC(A_COLOR);
#endif

	/* key functions */
	CC(KEY_BREAK);		CC(KEY_DOWN);		CC(KEY_UP);
	CC(KEY_LEFT);		CC(KEY_RIGHT);		CC(KEY_HOME);
	CC(KEY_BACKSPACE);

	CC(KEY_DL);		CC(KEY_IL);		CC(KEY_DC);
	CC(KEY_IC);		CC(KEY_EIC);		CC(KEY_CLEAR);
	CC(KEY_EOS);		CC(KEY_EOL);		CC(KEY_SF);
	CC(KEY_SR);		CC(KEY_NPAGE);		CC(KEY_PPAGE);
	CC(KEY_STAB);		CC(KEY_CTAB);		CC(KEY_CATAB);
	CC(KEY_ENTER);		CC(KEY_SRESET);		CC(KEY_RESET);
	CC(KEY_PRINT);		CC(KEY_LL);		CC(KEY_A1);
	CC(KEY_A3);		CC(KEY_B2);		CC(KEY_C1);
	CC(KEY_C3);		CC(KEY_BTAB);		CC(KEY_BEG);
	CC(KEY_CANCEL);		CC(KEY_CLOSE);		CC(KEY_COMMAND);
	CC(KEY_COPY);		CC(KEY_CREATE);		CC(KEY_END);
	CC(KEY_EXIT);		CC(KEY_FIND);		CC(KEY_HELP);
	CC(KEY_MARK);		CC(KEY_MESSAGE); /* ncurses extension: CC(KEY_MOUSE); */
	CC(KEY_MOVE);		CC(KEY_NEXT);		CC(KEY_OPEN);
	CC(KEY_OPTIONS);	CC(KEY_PREVIOUS);	CC(KEY_REDO);
	CC(KEY_REFERENCE);	CC(KEY_REFRESH);	CC(KEY_REPLACE);
	CC(KEY_RESIZE);		CC(KEY_RESTART);	CC(KEY_RESUME);
	CC(KEY_SAVE);		CC(KEY_SBEG);		CC(KEY_SCANCEL);
	CC(KEY_SCOMMAND);	CC(KEY_SCOPY);		CC(KEY_SCREATE);
	CC(KEY_SDC);		CC(KEY_SDL);		CC(KEY_SELECT);
	CC(KEY_SEND);		CC(KEY_SEOL);		CC(KEY_SEXIT);
	CC(KEY_SFIND);		CC(KEY_SHELP);		CC(KEY_SHOME);
	CC(KEY_SIC);		CC(KEY_SLEFT);		CC(KEY_SMESSAGE);
	CC(KEY_SMOVE);		CC(KEY_SNEXT);		CC(KEY_SOPTIONS);
	CC(KEY_SPREVIOUS);	CC(KEY_SPRINT);		CC(KEY_SREDO);
	CC(KEY_SREPLACE);	CC(KEY_SRIGHT);		CC(KEY_SRSUME);
	CC(KEY_SSAVE);		CC(KEY_SSUSPEND);	CC(KEY_SUNDO);
	CC(KEY_SUSPEND);	CC(KEY_UNDO);

	/* KEY_Fx  0 <= x <= 63 */
	CC(KEY_F0);
	CF(1);  CF(2);  CF(3);  CF(4);  CF(5);  CF(6);  CF(7);  CF(8);
	CF(9);  CF(10); CF(11); CF(12); CF(13); CF(14); CF(15); CF(16);
	CF(17); CF(18); CF(19); CF(20); CF(21); CF(22); CF(23); CF(24);
	CF(25); CF(26); CF(27); CF(28); CF(29); CF(30); CF(31); CF(32);
	CF(33); CF(34); CF(35); CF(36); CF(37); CF(38); CF(39); CF(40);
	CF(41); CF(42); CF(43); CF(44); CF(45); CF(46); CF(47); CF(48);
	CF(49); CF(50); CF(51); CF(52); CF(53); CF(54); CF(55); CF(56);
	CF(57); CF(58); CF(59); CF(60); CF(61); CF(62); CF(63);
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
Has @{endwin} been called more recently than @{posix.curses.window:refresh}?
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
Retern the main screen window.
@function stdscr
@treturn window main screen
@see initscr
*/
static int
Pstdscr(lua_State *L)
{
	lua_pushstring(L, STDSCR_REGISTRY);
	lua_rawget(L, LUA_REGISTRYINDEX);
	return 1;
}


/***
Number of columns in the main screen window.
@function cols
@treturn int number of columns in the main screen
@see lines
@see stdscr
*/
static int
Pcols(lua_State *L)
{
	return pushintresult(COLS);
}


/***
Number of lines in the main screen window.
@function lines
@treturn int number of lines in the main screen
@see cols
@see stdscr
*/
static int
Plines(lua_State *L)
{
	return pushintresult(LINES);
}


/***
Initialise color output facility.
@function start_color
@treturn bool `true`, if successful
@see curs_color(3x)
@see has_colors
*/
static int
Pstart_color(lua_State *L)
{
	return pushokresult(start_color());
}


/***
Does the terminal have color capability?
@function has_colors
@treturn bool `true`, if the terminal supports colors
@see curs_color(3x)
@see start_color
@usage
if curses.has_colors () then
  curses.start_color ()
end
*/
static int
Phas_colors(lua_State *L)
{
	return pushboolresult(has_colors());
}


/***
Reserve `-1` to represent terminal default colors.
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
Associate a color pair id with a specific foreground and background color.
@function init_pair
@int pair color pair id to act on
@int f foreground color to assign
@int b background color to assign
@treturn bool `true`, if successful
@see curs_color(3x)
@see pair_content
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
Return the foreground and background colors associated with a color pair id.
@function pair_content
@int pair color pair id to act on
@treturn int foreground color of *pair*
@treturn int background color of *pair*
@see curs_color(3x)
@see init_pair
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
How many colors are available for this terminal?
@function colors
@treturn int total number of available colors
@see curs_color(3x)
@see color_pairs
*/
static int
Pcolors(lua_State *L)
{
	return pushintresult(COLORS);
}


/***
How may distinct color pairs are supported by this terminal?
@function color_pairs
@treturn int total number of available color pairs
@see curs_color(3x)
@see colors
*/
static int
Pcolor_pairs(lua_State *L)
{
	return pushintresult(COLOR_PAIRS);
}


/***
Return the attributes for the given color pair id.
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
Fetch the output speed of the terminal.
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
Fetch the terminal's current erase character.
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
Fetch the character insert and delete capability of the terminal.
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
Fetch the line insert and delete capability of the terminal.
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
Fetch the terminal's current kill character.
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
Bitwise OR of all (or selected) video attributes supported by the terminal.
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
Fetch the name of the terminal.
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
Fetch the verbose name of the terminal.
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
Reduce the available size of the main screen.
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

#if LPOSIX_CURSES_COMPLIANT
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
#else
	return binding_notimplemented(L, "ripoffline", "curses");
#endif
}


/***
Change the visibility of the cursor.
@function curs_set
@int vis one of `0` (invisible), `1` (visible) or `2` (very visible)
@treturn[1] int previous cursor state
@return[2] nil if *vis* is not supported
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
Sleep for a few milliseconds.
@function napms
@int ms time to wait in milliseconds
@treturn bool `true`, if successful
@see curs_kernel(3x)
@see delay_output
*/
static int
Pnapms(lua_State *L)
{
	int ms = checkint(L, 1);
	return pushokresult(napms(ms));
}


/***
Change the terminal size.
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
	return binding_notimplemented(L, "resizeterm", "curses");
#endif
}


/***
Send the terminal audible bell.
@function beep
@treturn bool `true`, if successful
@see curs_beep(3x)
@see flash
*/
static int
Pbeep(lua_State *L)
{
	return pushokresult(beep());
}


/***
Send the terminal visible bell.
@function flash
@treturn bool `true`, if successful
@see curs_beep(3x)
@see beep
*/
static int
Pflash(lua_State *L)
{
	return pushokresult(flash());
}


/***
Create a new window.
@function newwin
@int nlines number of window lines
@int ncols number of window columns
@int begin_y top line of window
@int begin_x leftmost column of window
@treturn window a new window object
@see curs_window(3x)
@see posix.curses.window
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
Refresh the visible terminal screen.
@function doupdate
@treturn bool `true`, if successful
@see curs_refresh(3x)
@see posix.curses.window:refresh
*/
static int
Pdoupdate(lua_State *L)
{
	return pushokresult(doupdate());
}


/***
Initialise the soft label keys area.
This must be called before @{initscr}.
@function slk_init
@int fmt
@treturn bool `true`, if successful
@see curs_slk(3x)
*/
static int
Pslk_init(lua_State *L)
{
	int fmt = checkint(L, 1);
#if LPOSIX_CURSES_COMPLIANT
	return pushokresult(slk_init(fmt));
#else
	return binding_notimplemented(L, "slk_init", "curses");
#endif
}


/***
Set the label for a soft label key.
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
#if LPOSIX_CURSES_COMPLIANT
	return pushokresult(slk_set(labnum, label, fmt));
#else
	return binding_notimplemented(L, "slk_set", "curses");
#endif
}


/***
Refresh the soft label key area.
@function slk_refresh
@treturn bool `true`, if successful
@see curs_slk(3x)
@see posix.curses.window:refresh
*/
static int
Pslk_refresh(lua_State *L)
{
#if LPOSIX_CURSES_COMPLIANT
	return pushokresult(slk_refresh());
#else
	return binding_notimplemented(L, "slk_refresh", "curses");
#endif
}


/***
Copy the soft label key area backing screen to the virtual screen.
@function slk_noutrefresh
@treturn bool `true`, if successful
@see curs_slk(3x)
@see posix.curses.window:refresh
*/
static int
Pslk_noutrefresh(lua_State *L)
{
#if LPOSIX_CURSES_COMPLIANT
	return pushokresult(slk_noutrefresh());
#else
	return binding_notimplemented(L, "slk_noutrefresh", "curses");
#endif
}


/***
Fetch the label for a soft label key.
@function slk_label
@int labnum
@treturn string current label for *labnum*
@see curs_slk(3x)
*/
static int
Pslk_label(lua_State *L)
{
	int labnum = checkint(L, 1);
#if LPOSIX_CURSES_COMPLIANT
	return pushstringresult(slk_label(labnum));
#else
	return binding_notimplemented(L, "slk_label", "curses");
#endif
}


/***
Clears the soft labels from the screen.
@function slk_clear
@treturn bool `true`, if successful
@see curs_slk(3x)
@see slk_restore
*/
static int
Pslk_clear(lua_State *L)
{
#if LPOSIX_CURSES_COMPLIANT
	return pushokresult(slk_clear());
#else
	return binding_notimplemented(L, "slk_clear", "curses");
#endif
}


/***
Restores the soft labels to the screen.
@function slk_restore
@treturn bool `true`, if successful
@see curs_slk(3x)
@see slk_clear
*/
static int
Pslk_restore(lua_State *L)
{
#if LPOSIX_CURSES_COMPLIANT
	return pushokresult(slk_restore());
#else
	return binding_notimplemented(L, "slk_restore", "curses");
#endif
}


/***
Mark the soft label key area for refresh.
@function slk_touch
@treturn bool `true`, if successful
@see curs_slk(3x)
*/
static int
Pslk_touch(lua_State *L)
{
#if LPOSIX_CURSES_COMPLIANT
	return pushokresult(slk_touch());
#else
	return binding_notimplemented(L, "slk_touch", "curses");
#endif
}


/***
Enable an attribute for soft labels.
@function slk_attron
@int attrs
@treturn bool `true`, if successful
@see curs_slk(3x)
*/
static int
Pslk_attron(lua_State *L)
{
	chtype attrs = checkch(L, 1);
#if LPOSIX_CURSES_COMPLIANT
	return pushokresult(slk_attron(attrs));
#else
	return binding_notimplemented(L, "slk_attron", "curses");
#endif
}


/***
Disable an attribute for soft labels.
@function slk_attroff
@int attrs
@treturn bool `true`, if successful
@see curs_slk(3x)
*/
static int
Pslk_attroff(lua_State *L)
{
	chtype attrs = checkch(L, 1);
#if LPOSIX_CURSES_COMPLIANT
	return pushokresult(slk_attroff(attrs));
#else
	return binding_notimplemented(L, "slk_attroff", "curses");
#endif
}


/***
Set the attributes for soft labels.
@function slk_attrset
@int attrs
@treturn bool `true`, if successful
@see curs_slk(3x)
*/
static int
Pslk_attrset(lua_State *L)
{
	chtype attrs = checkch(L, 1);
#if LPOSIX_CURSES_COMPLIANT
	return pushokresult(slk_attrset(attrs));
#else
	return binding_notimplemented(L, "slk_attrset", "curses");
#endif
}


/***
Put the terminal into cbreak mode.
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
Whether characters are echoed to the terminal as they are typed.
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
Put the terminal into raw mode.
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
Put the terminal into halfdelay mode.
@function halfdelay
@int tenths delay in tenths of a second
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
Whether to translate a return key to newline on input.
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
Return a printable representation of a character, ignoring attributes.
@function unctrl
@int c character to act on
@treturn string printable representation of *c*
@see curs_util(3x)
@see keyname
*/
static int
Punctrl(lua_State *L)
{
	chtype c = checkch(L, 1);
	return pushstringresult(unctrl(c));
}


/***
Return a printable representation of a key.
@function keyname
@int c a key
@treturn string key name of *c*
@see curs_util(3x)
@see unctrl
*/
static int
Pkeyname(lua_State *L)
{
	int c = checkint(L, 1);
	return pushstringresult(keyname(c));
}


/***
Insert padding characters to force a short delay.
@function delay_output
@int ms delay time in milliseconds
@treturn bool `true`, if successful
@see curs_util(3x)
@see napms
*/
static int
Pdelay_output(lua_State *L)
{
	int ms = checkint(L, 1);
	return pushokresult(delay_output(ms));
}


/***
Throw away any typeahead in the keyboard input buffer.
@function flushinp
@treturn bool `true`, if successful
@see curs_util(3x)
@see ungetch
*/
static int
Pflushinp(lua_State *L)
{
	return pushboolresult(flushinp());
}


/***
Return a character to the keyboard input buffer.
@function ungetch
@int c
@treturn bool `true`, if successful
@see curs_getch(3x)
@see flushinp
*/
static int
Pungetch(lua_State *L)
{
	int c = checkint(L, 1);
	return pushokresult(ungetch(c));
}


/***
Create a new pad.
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
Fetch terminfo boolean capability.
@function tigetflag
@string capname
@treturn bool content of terminal boolean capability
@see curs_terminfo(3x)
@see terminfo(5)
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
Fetch terminfo numeric capability.
@function tigetnum
@string capname
@treturn int content of terminal numeric capability
@see curs_terminfo(3x)
@see terminfo(5)
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
Fetch terminfo string capability.
@function tigetstr
@string capname
@treturn string content of terminal string capability
@see curs_terminfo(3x)
@see terminfo(5)
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
#endif


static const luaL_Reg curseslib[] =
{
#if HAVE_CURSES
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
#endif
	{NULL, NULL}
};

/***
Constants.
@section constants
*/

/***
Curses constants.
Any constants not available in the underlying system will be `nil` valued,
see @{curses.lua}. Many of the `KEY_` constants cannot be generated by
modern keyboards and are mostly for historical compatibility with ancient
terminal hardware keyboards.

Note that, unlike the other posix submodules, almost all of these constants
remain undefined (`nil`) until after @{posix.curses.initscr} has returned
successfully.
@table posix.curses
@int ACS_BLOCK alternate character set solid block
@int ACS_BOARD alternate character set board of squares
@int ACS_BTEE alternate character set bottom-tee
@int ACS_BULLET alternate character set bullet
@int ACS_CKBOARD alternate character set stipple
@int ACS_DARROW alternate character set down arrow
@int ACS_DEGREE alternate character set degrees mark
@int ACS_DIAMOND alternate character set diamond
@int ACS_HLINE alternate character set horizontal line
@int ACS_LANTERN alternate character set lantern
@int ACS_LARROW alternate character set left arrow
@int ACS_LLCORNER alternate character set lower left corner
@int ACS_LRCORNER alternate character set lower right corner
@int ACS_LTEE alternate character set left tee
@int ACS_PLMINUS alternate character set plus/minus
@int ACS_PLUS alternate character set plus
@int ACS_RARROW alternate character set right arrow
@int ACS_RTEE alternate character set right tee
@int ACS_S1 alternate character set scan-line 1
@int ACS_S9 alternate character set scan-line 9
@int ACS_TTEE alternate character set top tee
@int ACS_UARROW alternate character set up arrow
@int ACS_ULCORNER alternate character set upper left corner
@int ACS_URCORNER alternate character set upper right corner
@int ACS_VLINE alternate character set vertical line
@int A_ALTCHARSET alternatate character set attribute
@int A_ATTRIBUTES attributed character attributes bitmask
@int A_BLINK blinking attribute
@int A_BOLD bold attribute
@int A_CHARTEXT attributed character text bitmask
@int A_COLOR attributed character color-pair bitmask
@int A_DIM half-bright attribute
@int A_INVIS invisible attribute
@int A_NORMAL normal attribute (all attributes off)
@int A_PROTECT protected attribute
@int A_REVERSE reverse video attribute
@int A_STANDOUT standout attribute
@int A_UNDERLINE underline attribute
@int COLOR_BLACK black color attribute
@int COLOR_BLUE blue color attribute
@int COLOR_CYAN cyan color attribute
@int COLOR_GREEN green color attribute
@int COLOR_MAGENTA magenta color attribute
@int COLOR_RED red color attribute
@int COLOR_WHITE white color attribute
@int COLOR_YELLOW yellow color attribute
@int KEY_A1 upper-left of keypad key
@int KEY_A3 upper-right of keypad key
@int KEY_B2 center of keypad key
@int KEY_BACKSPACE backspace key
@int KEY_BEG beginning key
@int KEY_BREAK break key
@int KEY_BTAB backtab key
@int KEY_C1 bottom-left of keypad key
@int KEY_C3 bottom-right of keypad key
@int KEY_CANCEL cancel key
@int KEY_CATAB clear all tabs key
@int KEY_CLEAR clear screen key
@int KEY_CLOSE close key
@int KEY_COMMAND command key
@int KEY_COPY copy key
@int KEY_CREATE create key
@int KEY_CTAB clear tab key
@int KEY_DC delete character key
@int KEY_DL delete line key
@int KEY_DOWN down arrow key
@int KEY_EIC exit insert char mode key
@int KEY_END end key
@int KEY_ENTER enter key
@int KEY_EOL clear to end of line key
@int KEY_EOS clear to end of screen key
@int KEY_EXIT exit key
@int KEY_F0 f0 key
@int KEY_F1 f1 key
@int KEY_F2 f2 key
@int KEY_F3 f3 key
@int KEY_F4 f4 key
@int KEY_F5 f5 key
@int KEY_F6 f6 key
@int KEY_F7 f7 key
@int KEY_F8 f8 key
@int KEY_F9 f9 key
@int KEY_F10 f10 key
@int KEY_F11 f11 key
@int KEY_F12 f12 key
@int KEY_F13 f13 key
@int KEY_F14 f14 key
@int KEY_F15 f15 key
@int KEY_F16 f16 key
@int KEY_F17 f17 key
@int KEY_F18 f18 key
@int KEY_F19 f19 key
@int KEY_F20 f20 key
@int KEY_F21 f21 key
@int KEY_F22 f22 key
@int KEY_F23 f23 key
@int KEY_F24 f24 key
@int KEY_F25 f25 key
@int KEY_F26 f26 key
@int KEY_F27 f27 key
@int KEY_F28 f28 key
@int KEY_F29 f29 key
@int KEY_F30 f30 key
@int KEY_F31 f31 key
@int KEY_F32 f32 key
@int KEY_F33 f33 key
@int KEY_F34 f34 key
@int KEY_F35 f35 key
@int KEY_F36 f36 key
@int KEY_F37 f37 key
@int KEY_F38 f38 key
@int KEY_F39 f39 key
@int KEY_F40 f40 key
@int KEY_F41 f41 key
@int KEY_F42 f42 key
@int KEY_F43 f43 key
@int KEY_F44 f44 key
@int KEY_F45 f45 key
@int KEY_F46 f46 key
@int KEY_F47 f47 key
@int KEY_F48 f48 key
@int KEY_F49 f49 key
@int KEY_F50 f50 key
@int KEY_F51 f51 key
@int KEY_F52 f52 key
@int KEY_F53 f53 key
@int KEY_F54 f54 key
@int KEY_F55 f55 key
@int KEY_F56 f56 key
@int KEY_F57 f57 key
@int KEY_F58 f58 key
@int KEY_F59 f59 key
@int KEY_F60 f60 key
@int KEY_F61 f61 key
@int KEY_F62 f62 key
@int KEY_F63 f63 key
@int KEY_FIND find key
@int KEY_HELP help key
@int KEY_HOME home key
@int KEY_IC enter insert char mode key
@int KEY_IL insert line key
@int KEY_LEFT cursor left key
@int KEY_LL home down or bottom key
@int KEY_MARK mark key
@int KEY_MESSAGE message key
@int KEY_MOUSE mouse event available virtual key
@int KEY_MOVE move key
@int KEY_NEXT next object key
@int KEY_NPAGE next page key
@int KEY_OPEN open key
@int KEY_OPTIONS options key
@int KEY_PPAGE previous page key
@int KEY_PREVIOUS prewious object key
@int KEY_PRINT print key
@int KEY_REDO redo key
@int KEY_REFERENCE reference key
@int KEY_REFRESH refresh key
@int KEY_REPLACE replace key
@int KEY_RESET hard reset key
@int KEY_RESIZE resize event virtual key
@int KEY_RESTART restart key
@int KEY_RESUME resume key
@int KEY_RIGHT cursor right key
@int KEY_SAVE save key
@int KEY_SBEG shift beginning key
@int KEY_SCANCEL shift cancel key
@int KEY_SCOMMAND shift command key
@int KEY_SCOPY shift copy key
@int KEY_SCREATE shift create key
@int KEY_SDC shift delete character key
@int KEY_SDL shift delete line key
@int KEY_SELECT select key
@int KEY_SEND send key
@int KEY_SEOL shift clear to end of line key
@int KEY_SEXIT shift exit key
@int KEY_SF scroll one line forward key
@int KEY_SFIND shift find key
@int KEY_SHELP shift help key
@int KEY_SHOME shift home key
@int KEY_SIC shift enter insert mode key
@int KEY_SLEFT shift cursor left key
@int KEY_SMESSAGE shift message key
@int KEY_SMOVE shift move key
@int KEY_SNEXT shift next object key
@int KEY_SOPTIONS shift options key
@int KEY_SPREVIOUS shift previous object key
@int KEY_SPRINT shift print key
@int KEY_SR scroll one line backward key
@int KEY_SREDO shift redo key
@int KEY_SREPLACE shift replace key
@int KEY_SRESET soft reset key
@int KEY_SRIGHT shift cursor right key
@int KEY_SRSUME shift resume key
@int KEY_SSAVE shift save key
@int KEY_SSUSPEND shift suspend key
@int KEY_STAB shift tab key
@int KEY_SUNDO shift undo key
@int KEY_SUSPEND suspend key
@int KEY_UNDO undo key
@int KEY_UP cursor up key
@usage
  -- Print curses constants supported on this host.
  for name, value in pairs (require "posix.curses") do
    if type (value) == "number" then
      print (name, value)
     end
  end
*/

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
