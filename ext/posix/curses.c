/***
@module posix.curses
*/
/*
 * Library: lcurses - Lua 5.1 interface to the curses library
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
#if HAVE_NCURSESW_CURSES_H
#  include <ncursesw/curses.h>
#elif HAVE_NCURSESW_H
#  include <ncursesw.h>
#elif HAVE_NCURSES_CURSES_H
#  include <ncurses/curses.h>
#elif HAVE_NCURSES_H
#  include <ncurses.h>
#elif HAVE_CURSES_H
#  include <curses.h>
#else
#  error "SysV or X/Open-compatible Curses header file required"
#endif
#include <term.h>

#include "_helpers.c"
#include "strlcpy.c"


#define B(v)		((((int) (v)) == OK))
#define pushokresult(b)	pushboolresult(B(b))


static const char *STDSCR_REGISTRY	= "curses:stdscr";
static const char *WINDOWMETA		= "curses:window";
static const char *CHSTRMETA		= "curses:chstr";
static const char *RIPOFF_TABLE		= "curses:ripoffline";


static void
lc_newwin(lua_State *L, WINDOW *nw)
{
	if (nw)
	{
		WINDOW **w = lua_newuserdata(L, sizeof(WINDOW*));
		luaL_getmetatable(L, WINDOWMETA);
		lua_setmetatable(L, -2);
		*w = nw;
	}
	else
	{
		lua_pushliteral(L, "failed to create window");
		lua_error(L);
	}
}


static WINDOW **lc_getwin(lua_State *L, int offset)
{
	WINDOW **w = (WINDOW**)luaL_checkudata(L, offset, WINDOWMETA);
	if (w == NULL)
		luaL_argerror(L, offset, "bad curses window");
	return w;
}


static WINDOW *checkwin(lua_State *L, int offset)
{
	WINDOW **w = lc_getwin(L, offset);
	if (*w == NULL)
		luaL_argerror(L, offset, "attempt to use closed curses window");
	return *w;
}


static int
W__tostring(lua_State *L)
{
	WINDOW **w = lc_getwin(L, 1);
	char buff[34];
	if (*w == NULL)
		strcpy(buff, "closed");
	else
		sprintf(buff, "%p", lua_touserdata(L, 1));
	lua_pushfstring(L, "curses window (%s)", buff);
	return 1;
}


static chtype checkch(lua_State *L, int offset)
{
	if (lua_type(L, offset) == LUA_TNUMBER)
		return luaL_checknumber(L, offset);
	if (lua_type(L, offset) == LUA_TSTRING)
		return *lua_tostring(L, offset);

	return luaL_typerror(L, offset, "chtype");
}


static chtype optch(lua_State *L, int offset, chtype def)
{
	if (lua_isnoneornil(L, offset))
		return def;
	return checkch(L, offset);
}

/****c* classes/chstr
 * FUNCTION
 *   Line drawing buffer.
 *
 * SEE ALSO
 *   curses.new_chstr
 ****/

typedef struct
{
	unsigned int len;
	chtype str[1];
} chstr;
#define CHSTR_SIZE(len) (sizeof(chstr) + len * sizeof(chtype))


/* create new chstr object and leave it in the lua stack */

static chstr* chstr_new(lua_State *L, int len)
{
	if (len < 1)
	{
		lua_pushliteral(L, "invalid chstr length");
		lua_error(L);
	}
	{
		chstr *cs = lua_newuserdata(L, CHSTR_SIZE(len));
		luaL_getmetatable(L, CHSTRMETA);
		lua_setmetatable(L, -2);
		cs->len = len;
		return cs;
	}
}

/* get chstr from lua (convert if needed) */

static chstr* checkchstr(lua_State *L, int offset)
{
	chstr *cs = (chstr*)luaL_checkudata(L, offset, CHSTRMETA);
	if (cs)
		return cs;

	luaL_argerror(L, offset, "bad curses chstr");
	return NULL;
}

/****f* curses/curses.new_chstr
 * FUNCTION
 *   Create a new line drawing buffer instance.
 *
 * SEE ALSO
 *   chstr
 ****/

static int
Pnew_chstr(lua_State *L)
{
	int len = checkint(L, 1);
	chstr* ncs = chstr_new(L, len);
	memset(ncs->str, ' ', len*sizeof(chtype));
	return 1;
}

/* change the contents of the chstr */

static int
chstr_set_str(lua_State *L)
{
	chstr *cs = checkchstr(L, 1);
	int offset = checkint(L, 2);
	const char *str = luaL_checkstring(L, 3);
	int len = lua_strlen(L, 3);
	int attr = optint(L, 4, A_NORMAL);
	int rep = optint(L, 5, 1);
	int i;

	if (offset < 0)
		return 0;

	while (rep-- > 0 && offset <= (int)cs->len)
	{
		if (offset + len - 1 > (int)cs->len)
			len = cs->len - offset + 1;

		for (i = 0; i < len; ++i)
			cs->str[offset + i] = str[i] | attr;
		offset += len;
	}

	return 0;
}


/****m* chstr/set_ch
 * FUNCTION
 *   Set a character in the buffer.
 *
 * SYNOPSIS
 *   chstr:set_ch(offset, char, attribute [, repeat])
 *
 * EXAMPLE
 *   Set the buffer with 'a's where the first one is capitalized
 *   and has bold.
 *	   size = 10
 *	   str = curses.new_chstr(10)
 *	   str:set_ch(0, 'A', curses.A_BOLD)
 *	   str:set_ch(1, 'a', curses.A_NORMAL, size - 1)
 ****/

static int
chstr_set_ch(lua_State *L)
{
	chstr* cs = checkchstr(L, 1);
	int offset = checkint(L, 2);
	chtype ch = checkch(L, 3);
	int attr = optint(L, 4, A_NORMAL);
	int rep = optint(L, 5, 1);

	while (rep-- > 0)
	{
		if (offset < 0 || offset >= (int) cs->len)
			return 0;

		cs->str[offset] = ch | attr;

		++offset;
	}
	return 0;
}

/* get information from the chstr */

static int
chstr_get(lua_State *L)
{
	chstr* cs = checkchstr(L, 1);
	int offset = checkint(L, 2);
	chtype ch;

	if (offset < 0 || offset >= (int) cs->len)
		return 0;

	ch = cs->str[offset];

	lua_pushinteger(L, ch & A_CHARTEXT);
	lua_pushinteger(L, ch & A_ATTRIBUTES);
	lua_pushinteger(L, ch & A_COLOR);
	return 3;
}

/* retrieve chstr length */

static int
chstr_len(lua_State *L)
{
	chstr *cs = checkchstr(L, 1);
	return pushintresult(cs->len);
}

/* duplicate chstr */

static int
chstr_dup(lua_State *L)
{
	chstr *cs = checkchstr(L, 1);
	chstr *ncs = chstr_new(L, cs->len);

	memcpy(ncs->str, cs->str, CHSTR_SIZE(cs->len));
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

/* FIXME: Avoid cast to void. */

static int
Pendwin(lua_State *L)
{
	(void) L;
	endwin();
	return 0;
}


static int
Pisendwin(lua_State *L)
{
	return pushboolresult(isendwin());
}


static int
Pstdscr(lua_State *L)
{
	lua_pushstring(L, STDSCR_REGISTRY);
	lua_rawget(L, LUA_REGISTRYINDEX);
	return 1;
}


static int
Pcols(lua_State *L)
{
	return pushintresult(COLS);
}


static int
Plines(lua_State *L)
{
	return pushintresult(LINES);
}


static int
Pstart_color(lua_State *L)
{
	return pushokresult(start_color());
}


static int
Phas_colors(lua_State *L)
{
	return pushboolresult(has_colors());
}


static int
Puse_default_colors(lua_State *L)
{
	return pushokresult(use_default_colors());
}


static int
Pinit_pair(lua_State *L)
{
	short pair = checkint(L, 1);
	short f = checkint(L, 2);
	short b = checkint(L, 3);
	return pushokresult(init_pair(pair, f, b));
}


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


static int
Pcolors(lua_State *L)
{
	return pushintresult(COLORS);
}


static int
Pcolor_pairs(lua_State *L)
{
	return pushintresult(COLOR_PAIRS);
}


static int
Pcolor_pair(lua_State *L)
{
	int n = checkint(L, 1);
	return pushintresult(COLOR_PAIR(n));
}


static int
Pbaudrate(lua_State *L)
{
	return pushintresult(baudrate());
}


static int
Perasechar(lua_State *L)
{
	return pushintresult(erasechar());
}


static int
Phas_ic(lua_State *L)
{
	return pushboolresult(has_ic());
}


static int
Phas_il(lua_State *L)
{
	return pushboolresult(has_il());
}


static int
Pkillchar(lua_State *L)
{
	return pushintresult(killchar());
}


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


static int
Ptermname(lua_State *L)
{
	return pushstringresult(termname());
}


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


static int
Pcurs_set(lua_State *L)
{
	int vis = checkint(L, 1);
	int state = curs_set(vis);
	if (state == ERR)
		return 0;

	return pushintresult(state);
}


static int
Pnapms(lua_State *L)
{
	int ms = checkint(L, 1);
	return pushokresult(napms(ms));
}


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


static int
Pbeep(lua_State *L)
{
	return pushokresult(beep());
}


static int
Pflash(lua_State *L)
{
	return pushokresult(flash());
}


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


static int
Wclose(lua_State *L)
{
	WINDOW **w = lc_getwin(L, 1);
	if (*w != NULL && *w != stdscr)
	{
		delwin(*w);
		*w = NULL;
	}
	return 0;
}


static int
Wmove_window(lua_State *L)
{
	WINDOW *w = checkwin(L, 1);
	int y = checkint(L, 2);
	int x = checkint(L, 3);
	return pushokresult(mvwin(w, y, x));
}


static int
Wsub(lua_State *L)
{
	WINDOW *orig = checkwin(L, 1);
	int nlines  = checkint(L, 2);
	int ncols   = checkint(L, 3);
	int begin_y = checkint(L, 4);
	int begin_x = checkint(L, 5);

	lc_newwin(L, subwin(orig, nlines, ncols, begin_y, begin_x));
	return 1;
}


static int
Wderive(lua_State *L)
{
	WINDOW *orig = checkwin(L, 1);
	int nlines  = checkint(L, 2);
	int ncols   = checkint(L, 3);
	int begin_y = checkint(L, 4);
	int begin_x = checkint(L, 5);

	lc_newwin(L, derwin(orig, nlines, ncols, begin_y, begin_x));
	return 1;
}


static int
Wmove_derived(lua_State *L)
{
	WINDOW *w = checkwin(L, 1);
	int par_y = checkint(L, 2);
	int par_x = checkint(L, 3);
	return pushokresult(mvderwin(w, par_y, par_x));
}


static int
Wresize(lua_State *L)
{
	WINDOW *w = checkwin(L, 1);
	int height = checkint(L, 2);
	int width = checkint(L, 3);

	int c = wresize(w, height, width);
	if (c == ERR) return 0;

	return pushokresult(true);
}


static int
Wclone(lua_State *L)
{
	WINDOW *w = checkwin(L, 1);
	lc_newwin(L, dupwin(w));
	return 1;
}


static int
Wsyncup(lua_State *L)
{
	wsyncup(checkwin(L, 1));
	return 0;
}


static int
Wsyncok(lua_State *L)
{
	WINDOW *w = checkwin(L, 1);
	int bf = lua_toboolean(L, 2);
	lua_pushboolean(L, B(syncok(w, bf)));
	return 1;
}


static int
Wcursyncup(lua_State *L)
{
	wcursyncup(checkwin(L, 1));
	return 0;
}


static int
Wsyncdown(lua_State *L)
{
	wsyncdown(checkwin(L, 1));
	return 0;
}


static int
Wrefresh(lua_State *L)
{
	return pushokresult(wrefresh(checkwin(L, 1)));
}


static int
Wnoutrefresh(lua_State *L)
{
	return pushokresult(wnoutrefresh(checkwin(L, 1)));
}


static int
Wredrawwin(lua_State *L)
{
	return pushokresult(redrawwin(checkwin(L, 1)));
}


static int
Wredrawln(lua_State *L)
{
	WINDOW *w = checkwin(L, 1);
	int beg_line = checkint(L, 2);
	int num_lines = checkint(L, 3);
	return pushokresult(wredrawln(w, beg_line, num_lines));
}


static int
Pdoupdate(lua_State *L)
{
	return pushokresult(doupdate());
}


static int
Wmove(lua_State *L)
{
	WINDOW *w = checkwin(L, 1);
	int y = checkint(L, 2);
	int x = checkint(L, 3);
	return pushokresult(wmove(w, y, x));
}


static int
Wscrl(lua_State *L)
{
	WINDOW *w = checkwin(L, 1);
	int n = checkint(L, 2);
	return pushokresult(wscrl(w, n));
}


static int
Wtouch(lua_State *L)
{
	WINDOW *w = checkwin(L, 1);
	int changed;
	if (lua_isnoneornil(L, 2))
		changed = TRUE;
	else
		changed = lua_toboolean(L, 2);

	if (changed)
		return pushokresult(touchwin(w));
	return pushokresult(untouchwin(w));
}


static int
Wtouchline(lua_State *L)
{
	WINDOW *w = checkwin(L, 1);
	int y = checkint(L, 2);
	int n = checkint(L, 3);
	int changed;
	if (lua_isnoneornil(L, 4))
		changed = TRUE;
	else
		changed = lua_toboolean(L, 4);
	return pushokresult(wtouchln(w, y, n, changed));
}


static int
Wis_linetouched(lua_State *L)
{
	WINDOW *w = checkwin(L, 1);
	int line = checkint(L, 2);
	return pushboolresult(is_linetouched(w, line));
}


static int
Wis_wintouched(lua_State *L)
{
	WINDOW *w = checkwin(L, 1);
	return pushboolresult(is_wintouched(w));
}


static int
Wgetyx(lua_State *L)
{
	WINDOW *w = checkwin(L, 1);
	int y, x;
	getyx(w, y, x);
	lua_pushinteger(L, y);
	lua_pushinteger(L, x);
	return 2;
}


static int
Wgetparyx(lua_State *L)
{
	WINDOW *w = checkwin(L, 1);
	int y, x;
	getparyx(w, y, x);
	lua_pushinteger(L, y);
	lua_pushinteger(L, x);
	return 2;
}


static int
Wgetbegyx(lua_State *L)
{
	WINDOW *w = checkwin(L, 1);
	int y, x;
	getbegyx(w, y, x);
	lua_pushinteger(L, y);
	lua_pushinteger(L, x);
	return 2;
}


static int
Wgetmaxyx(lua_State *L)
{
	WINDOW *w = checkwin(L, 1);
	int y, x;
	getmaxyx(w, y, x);
	lua_pushinteger(L, y);
	lua_pushinteger(L, x);
	return 2;
}


static int
Wborder(lua_State *L)
{
	WINDOW *w = checkwin(L, 1);
	chtype ls = optch(L, 2, 0);
	chtype rs = optch(L, 3, 0);
	chtype ts = optch(L, 4, 0);
	chtype bs = optch(L, 5, 0);
	chtype tl = optch(L, 6, 0);
	chtype tr = optch(L, 7, 0);
	chtype bl = optch(L, 8, 0);
	chtype br = optch(L, 9, 0);

	return pushintresult(B(wborder(w, ls, rs, ts, bs, tl, tr, bl, br)));
}


static int
Wbox(lua_State *L)
{
	WINDOW *w = checkwin(L, 1);
	chtype verch = checkch(L, 2);
	chtype horch = checkch(L, 3);

	return pushintresult(B(box(w, verch, horch)));
}


static int
Whline(lua_State *L)
{
	WINDOW *w = checkwin(L, 1);
	chtype ch = checkch(L, 2);
	int n = checkint(L, 3);

	return pushokresult(whline(w, ch, n));
}


static int
Wvline(lua_State *L)
{
	WINDOW *w = checkwin(L, 1);
	chtype ch = checkch(L, 2);
	int n = checkint(L, 3);

	return pushokresult(wvline(w, ch, n));
}


static int
Wmvhline(lua_State *L)
{
	WINDOW *w = checkwin(L, 1);
	int y = checkint(L, 2);
	int x = checkint(L, 3);
	chtype ch = checkch(L, 4);
	int n = checkint(L, 5);

	return pushokresult(mvwhline(w, y, x, ch, n));
}


static int
Wmvvline(lua_State *L)
{
	WINDOW *w = checkwin(L, 1);
	int y = checkint(L, 2);
	int x = checkint(L, 3);
	chtype ch = checkch(L, 4);
	int n = checkint(L, 5);

	return pushokresult(mvwvline(w, y, x, ch, n));
}


static int
Werase(lua_State *L)
{
	return pushokresult(werase(checkwin(L, 1)));
}


static int
Wclear(lua_State *L)
{
	return pushokresult(wclear(checkwin(L, 1)));
}


static int
Wclrtobot(lua_State *L)
{
	return pushokresult(wclrtobot(checkwin(L, 1)));
}


static int
Wclrtoeol(lua_State *L)
{
	return pushokresult(wclrtoeol(checkwin(L, 1)));
}


static int
Pslk_init(lua_State *L)
{
	int fmt = checkint(L, 1);
	return pushokresult(slk_init(fmt));
}


static int
Pslk_set(lua_State *L)
{
	int labnum = checkint(L, 1);
	const char* label = luaL_checkstring(L, 2);
	int fmt = checkint(L, 3);
	return pushokresult(slk_set(labnum, label, fmt));
}


static int
Pslk_refresh(lua_State *L)
{
	return pushokresult(slk_refresh());
}


static int
Pslk_noutrefresh(lua_State *L)
{
	return pushokresult(slk_noutrefresh());
}


static int
Pslk_label(lua_State *L)
{
	int labnum = checkint(L, 1);
	return pushstringresult(slk_label(labnum));
}


static int
Pslk_clear(lua_State *L)
{
	return pushokresult(slk_clear());
}


static int
Pslk_restore(lua_State *L)
{
	return pushokresult(slk_restore());
}


static int
Pslk_touch(lua_State *L)
{
	return pushokresult(slk_touch());
}


static int
Pslk_attron(lua_State *L)
{
	chtype attrs = checkch(L, 1);
	return pushokresult(slk_attron(attrs));
}


static int
Pslk_attroff(lua_State *L)
{
	chtype attrs = checkch(L, 1);
	return pushokresult(slk_attroff(attrs));
}


static int
Pslk_attrset(lua_State *L)
{
	chtype attrs = checkch(L, 1);
	return pushokresult(slk_attrset(attrs));
}


static int
Waddch(lua_State *L)
{
	WINDOW *w = checkwin(L, 1);
	chtype ch = checkch(L, 2);
	return pushokresult(waddch(w, ch));
}


static int
Wmvaddch(lua_State *L)
{
	WINDOW *w = checkwin(L, 1);
	int y = checkint(L, 2);
	int x = checkint(L, 3);
	chtype ch = checkch(L, 4);
	return pushokresult(mvwaddch(w, y, x, ch));
}


static int
Wechoch(lua_State *L)
{
	WINDOW *w = checkwin(L, 1);
	chtype ch = checkch(L, 2);
	return pushokresult(wechochar(w, ch));
}


static int
Waddchstr(lua_State *L)
{
	WINDOW *w = checkwin(L, 1);
	int n = optint(L, 3, -1);
	chstr *cs = checkchstr(L, 2);

	if (n < 0 || n > (int) cs->len)
		n = cs->len;

	return pushokresult(waddchnstr(w, cs->str, n));
}


static int
Wmvaddchstr(lua_State *L)
{
	WINDOW *w = checkwin(L, 1);
	int y = checkint(L, 2);
	int x = checkint(L, 3);
	int n = optint(L, 5, -1);
	chstr *cs = checkchstr(L, 4);

	if (n < 0 || n > (int) cs->len)
		n = cs->len;

	return pushokresult(mvwaddchnstr(w, y, x, cs->str, n));
}


static int
Waddstr(lua_State *L)
{
	WINDOW *w = checkwin(L, 1);
	const char *str = luaL_checkstring(L, 2);
	int n = optint(L, 3, -1);
	return pushokresult(waddnstr(w, str, n));
}


static int
Wmvaddstr(lua_State *L)
{
	WINDOW *w = checkwin(L, 1);
	int y = checkint(L, 2);
	int x = checkint(L, 3);
	const char *str = luaL_checkstring(L, 4);
	int n = optint(L, 5, -1);
	return pushokresult(mvwaddnstr(w, y, x, str, n));
}


static int
Wwbkgdset(lua_State *L)
{
	WINDOW *w = checkwin(L, 1);
	chtype ch = checkch(L, 2);
	wbkgdset(w, ch);
	return 0;
}


static int
Wwbkgd(lua_State *L)
{
	WINDOW *w = checkwin(L, 1);
	chtype ch = checkch(L, 2);
	return pushokresult(wbkgd(w, ch));
}


static int
Wgetbkgd(lua_State *L)
{
	WINDOW *w = checkwin(L, 1);
	return pushokresult(getbkgd(w));
}


static int
Pcbreak(lua_State *L)
{
	if (lua_isnoneornil(L, 1) || lua_toboolean(L, 1))
		return pushokresult(cbreak());
	return pushokresult(nocbreak());
}


static int
Pecho(lua_State *L)
{
	if (lua_isnoneornil(L, 1) || lua_toboolean(L, 1))
		return pushokresult(echo());
	return pushokresult(noecho());
}


static int
Praw(lua_State *L)
{
	if (lua_isnoneornil(L, 1) || lua_toboolean(L, 1))
		return pushokresult(raw());
	return pushokresult(noraw());
}


static int
Phalfdelay(lua_State *L)
{
	int tenths = checkint(L, 1);
	return pushokresult(halfdelay(tenths));
}


static int
Wintrflush(lua_State *L)
{
	WINDOW *w = checkwin(L, 1);
	int bf = lua_toboolean(L, 2);
	return pushokresult(intrflush(w, bf));
}


static int
Wkeypad(lua_State *L)
{
	WINDOW *w = checkwin(L, 1);
	int bf = lua_isnoneornil(L, 2) ? 1 : lua_toboolean(L, 2);
	return pushokresult(keypad(w, bf));
}


static int
Wmeta(lua_State *L)
{
	WINDOW *w = checkwin(L, 1);
	int bf = lua_toboolean(L, 2);
	return pushokresult(meta(w, bf));
}


static int
Wnodelay(lua_State *L)
{
	WINDOW *w = checkwin(L, 1);
	int bf = lua_toboolean(L, 2);
	return pushokresult(nodelay(w, bf));
}


static int
Wtimeout(lua_State *L)
{
	WINDOW *w = checkwin(L, 1);
	int delay = checkint(L, 2);
	wtimeout(w, delay);
	return 0;
}


static int
Wnotimeout(lua_State *L)
{
	WINDOW *w = checkwin(L, 1);
	int bf = lua_toboolean(L, 2);
	return pushokresult(notimeout(w, bf));
}


static int
Pnl(lua_State *L)
{
	if (lua_isnoneornil(L, 1) || lua_toboolean(L, 1))
		return pushokresult(nl());
	return pushokresult(nonl());
}


static int
Wclearok(lua_State *L)
{
	WINDOW *w = checkwin(L, 1);
	int bf = lua_toboolean(L, 2);
	return pushokresult(clearok(w, bf));
}


static int
Widlok(lua_State *L)
{
	WINDOW *w = checkwin(L, 1);
	int bf = lua_toboolean(L, 2);
	return pushokresult(idlok(w, bf));
}


static int
Wleaveok(lua_State *L)
{
	WINDOW *w = checkwin(L, 1);
	int bf = lua_toboolean(L, 2);
	return pushokresult(leaveok(w, bf));
}


static int
Wscrollok(lua_State *L)
{
	WINDOW *w = checkwin(L, 1);
	int bf = lua_toboolean(L, 2);
	return pushokresult(scrollok(w, bf));
}


static int
Widcok(lua_State *L)
{
	WINDOW *w = checkwin(L, 1);
	int bf = lua_toboolean(L, 2);
	idcok(w, bf);
	return 0;
}


static int
Wimmedok(lua_State *L)
{
	WINDOW *w = checkwin(L, 1);
	int bf = lua_toboolean(L, 2);
	immedok(w, bf);
	return 0;
}


static int
Wwsetscrreg(lua_State *L)
{
	WINDOW *w = checkwin(L, 1);
	int top = checkint(L, 2);
	int bot = checkint(L, 3);
	return pushokresult(wsetscrreg(w, top, bot));
}


static int
Woverlay(lua_State *L)
{
	WINDOW *srcwin = checkwin(L, 1);
	WINDOW *dstwin = checkwin(L, 2);
	return pushokresult(overlay(srcwin, dstwin));
}


static int
Woverwrite(lua_State *L)
{
	WINDOW *srcwin = checkwin(L, 1);
	WINDOW *dstwin = checkwin(L, 2);
	return pushokresult(overwrite(srcwin, dstwin));
}


static int
Wcopywin(lua_State *L)
{
	WINDOW *srcwin = checkwin(L, 1);
	WINDOW *dstwin = checkwin(L, 2);
	int sminrow = checkint(L, 3);
	int smincol = checkint(L, 4);
	int dminrow = checkint(L, 5);
	int dmincol = checkint(L, 6);
	int dmaxrow = checkint(L, 7);
	int dmaxcol = checkint(L, 8);
	int woverlay = lua_toboolean(L, 9);
	return pushokresult(copywin(srcwin, dstwin, sminrow,
		smincol, dminrow, dmincol, dmaxrow, dmaxcol, woverlay));
}


static int
Punctrl(lua_State *L)
{
	chtype c = luaL_checknumber(L, 1);
	return pushstringresult(unctrl(c));
}


static int
Pkeyname(lua_State *L)
{
	int c = checkint(L, 1);
	return pushstringresult(keyname(c));
}


static int
Pdelay_output(lua_State *L)
{
	int ms = checkint(L, 1);
	return pushokresult(delay_output(ms));
}


static int
Pflushinp(lua_State *L)
{
	return pushboolresult(flushinp());
}


static int
Wdelch(lua_State *L)
{
	return pushokresult(wdelch(checkwin(L, 1)));
}


static int
Wmvdelch(lua_State *L)
{
	WINDOW *w = checkwin(L, 1);
	int y = checkint(L, 2);
	int x = checkint(L, 3);
	return pushokresult(mvwdelch(w, y, x));
}


static int
Wdeleteln(lua_State *L)
{
	return pushokresult(wdeleteln(checkwin(L, 1)));
}


static int
Winsertln(lua_State *L)
{
	return pushokresult(winsertln(checkwin(L, 1)));
}


static int
Wwinsdelln(lua_State *L)
{
	WINDOW *w = checkwin(L, 1);
	int n = checkint(L, 2);
	return pushokresult(winsdelln(w, n));
}


static int
Wgetch(lua_State *L)
{
	WINDOW *w = checkwin(L, 1);
	int c = wgetch(w);

	if (c == ERR)
		return 0;

	return pushintresult(c);
}


static int
Wmvgetch(lua_State *L)
{
	WINDOW *w = checkwin(L, 1);
	int y = checkint(L, 2);
	int x = checkint(L, 3);
	int c;

	if (wmove(w, y, x) == ERR)
		return 0;

	c = wgetch(w);

	if (c == ERR)
		return 0;

	return pushintresult(c);
}


static int
Pungetch(lua_State *L)
{
	int c = checkint(L, 1);
	return pushokresult(ungetch(c));
}


static int
Wgetstr(lua_State *L)
{
	WINDOW *w = checkwin(L, 1);
	int n = optint(L, 2, 0);
	char buf[LUAL_BUFFERSIZE];

	if (n == 0 || n >= LUAL_BUFFERSIZE)
		n = LUAL_BUFFERSIZE - 1;
	if (wgetnstr(w, buf, n) == ERR)
		return 0;

	return pushstringresult(buf);
}


static int
Wmvgetstr(lua_State *L)
{
	WINDOW *w = checkwin(L, 1);
	int y = checkint(L, 2);
	int x = checkint(L, 3);
	int n = optint(L, 4, -1);
	char buf[LUAL_BUFFERSIZE];

	if (n == 0 || n >= LUAL_BUFFERSIZE)
		n = LUAL_BUFFERSIZE - 1;
	if (mvwgetnstr(w, y, x, buf, n) == ERR)
		return 0;

	return pushstringresult(buf);
}


static int
Wwinch(lua_State *L)
{
	WINDOW *w = checkwin(L, 1);
	return pushintresult(winch(w));
}


static int
Wmvwinch(lua_State *L)
{
	WINDOW *w = checkwin(L, 1);
	int y = checkint(L, 2);
	int x = checkint(L, 3);
	return pushintresult(mvwinch(w, y, x));
}


static int
Wwinchnstr(lua_State *L)
{
	WINDOW *w = checkwin(L, 1);
	int n = checkint(L, 2);
	chstr *cs = chstr_new(L, n);

	if (winchnstr(w, cs->str, n) == ERR)
		return 0;

	return 1;
}


static int
Wmvwinchnstr(lua_State *L)
{
	WINDOW *w = checkwin(L, 1);
	int y = checkint(L, 2);
	int x = checkint(L, 3);
	int n = checkint(L, 4);
	chstr *cs = chstr_new(L, n);

	if (mvwinchnstr(w, y, x, cs->str, n) == ERR)
		return 0;

	return 1;
}


static int
Wwinnstr(lua_State *L)
{
	WINDOW *w = checkwin(L, 1);
	int n = checkint(L, 2);
	char buf[LUAL_BUFFERSIZE];

	if (n >= LUAL_BUFFERSIZE)
		n = LUAL_BUFFERSIZE - 1;
	if (winnstr(w, buf, n) == ERR)
		return 0;

	lua_pushlstring(L, buf, n);
	return 1;
}


static int
Wmvwinnstr(lua_State *L)
{
	WINDOW *w = checkwin(L, 1);
	int y = checkint(L, 2);
	int x = checkint(L, 3);
	int n = checkint(L, 4);
	char buf[LUAL_BUFFERSIZE];

	if (n >= LUAL_BUFFERSIZE)
		n = LUAL_BUFFERSIZE - 1;
	if (mvwinnstr(w, y, x, buf, n) == ERR)
		return 0;

	lua_pushlstring(L, buf, n);
	return 1;
}


static int
Wwinsch(lua_State *L)
{
	WINDOW *w = checkwin(L, 1);
	chtype ch = checkch(L, 2);
	return pushokresult(winsch(w, ch));
}


static int
Wmvwinsch(lua_State *L)
{
	WINDOW *w = checkwin(L, 1);
	int y = checkint(L, 2);
	int x = checkint(L, 3);
	chtype ch = checkch(L, 4);
	return pushokresult(mvwinsch(w, y, x, ch));
}


static int
Wwinsstr(lua_State *L)
{
	WINDOW *w = checkwin(L, 1);
	const char *str = luaL_checkstring(L, 2);
	return pushokresult(winsnstr(w, str, lua_strlen(L, 2)));
}


static int
Wmvwinsstr(lua_State *L)
{
	WINDOW *w = checkwin(L, 1);
	int y = checkint(L, 2);
	int x = checkint(L, 3);
	const char *str = luaL_checkstring(L, 4);
	return pushokresult(mvwinsnstr(w, y, x, str, lua_strlen(L, 2)));
}


static int
Wwinsnstr(lua_State *L)
{
	WINDOW *w = checkwin(L, 1);
	const char *str = luaL_checkstring(L, 2);
	int n = checkint(L, 3);
	return pushokresult(winsnstr(w, str, n));
}


static int
Wmvwinsnstr(lua_State *L)
{
	WINDOW *w = checkwin(L, 1);
	int y = checkint(L, 2);
	int x = checkint(L, 3);
	const char *str = luaL_checkstring(L, 4);
	int n = checkint(L, 5);
	return pushokresult(mvwinsnstr(w, y, x, str, n));
}


static int
Pnewpad(lua_State *L)
{
	int nlines = checkint(L, 1);
	int ncols = checkint(L, 2);
	lc_newwin(L, newpad(nlines, ncols));
	return 1;
}


static int
Wsubpad(lua_State *L)
{
	WINDOW *orig = checkwin(L, 1);
	int nlines  = checkint(L, 2);
	int ncols   = checkint(L, 3);
	int begin_y = checkint(L, 4);
	int begin_x = checkint(L, 5);

	lc_newwin(L, subpad(orig, nlines, ncols, begin_y, begin_x));
	return 1;
}


static int
Wprefresh(lua_State *L)
{
	WINDOW *p = checkwin(L, 1);
	int pminrow = checkint(L, 2);
	int pmincol = checkint(L, 3);
	int sminrow = checkint(L, 4);
	int smincol = checkint(L, 5);
	int smaxrow = checkint(L, 6);
	int smaxcol = checkint(L, 7);

	return pushokresult(prefresh(p, pminrow, pmincol,
		sminrow, smincol, smaxrow, smaxcol));
}


static int
Wpnoutrefresh(lua_State *L)
{
	WINDOW *p = checkwin(L, 1);
	int pminrow = checkint(L, 2);
	int pmincol = checkint(L, 3);
	int sminrow = checkint(L, 4);
	int smincol = checkint(L, 5);
	int smaxrow = checkint(L, 6);
	int smaxcol = checkint(L, 7);

	return pushokresult(pnoutrefresh(p, pminrow, pmincol,
		sminrow, smincol, smaxrow, smaxcol));
}


static int
Wpechochar(lua_State *L)
{
	WINDOW *p = checkwin(L, 1);
	chtype ch = checkch(L, 2);
	return pushokresult(pechochar(p, ch));
}


static int
Wattroff(lua_State *L)
{
	WINDOW *w = checkwin(L, 1);
	int bf = checkint(L, 2);
	return pushokresult(wattroff(w, bf));
}


static int
Wattron(lua_State *L)
{
	WINDOW *w = checkwin(L, 1);
	int bf = checkint(L, 2);
	return pushokresult(wattron(w, bf));
}


static int
Wattrset(lua_State *L)
{
	WINDOW *w = checkwin(L, 1);
	int bf = checkint(L, 2);
	return pushokresult(wattrset(w, bf));
}


static int
Wstandend(lua_State *L)
{
	return pushokresult(wstandend(checkwin(L, 1)));
}


static int
Wstandout(lua_State *L)
{
	return pushokresult(wstandout(checkwin(L, 1)));
}


static char ti_capname[32];

static int
Ptigetflag (lua_State *L)
{
	int res;

	strlcpy (ti_capname, luaL_checkstring (L, 1), sizeof (ti_capname));
	res = tigetflag (ti_capname);
	if (-1 == res)
		return luaL_error (L, "`%s' is not a boolean capability", ti_capname);
	else
		lua_pushboolean (L, res);
	return 1;
}


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


/* chstr members */
static const luaL_Reg chstrlib[] =
{
#define MENTRY(_f) { LPOSIX_STR(_f), LPOSIX_SPLICE(chstr_, _f) }
	MENTRY( len		),
	MENTRY( set_ch		),
	MENTRY( set_str		),
	MENTRY( get		),
	MENTRY( dup		),
#undef MENTRY
	{ NULL, NULL }
};


static const luaL_Reg windowlib[] =
{
	LPOSIX_FUNC( W__tostring	),
	LPOSIX_FUNC( Waddch		),
	LPOSIX_FUNC( Waddchstr		),
	LPOSIX_FUNC( Waddstr		),
	LPOSIX_FUNC( Wattroff		),
	LPOSIX_FUNC( Wattron		),
	LPOSIX_FUNC( Wattrset		),
	LPOSIX_FUNC( Wborder		),
	LPOSIX_FUNC( Wbox		),
	LPOSIX_FUNC( Wclear		),
	LPOSIX_FUNC( Wclearok		),
	LPOSIX_FUNC( Wclone		),
	LPOSIX_FUNC( Wclose		),
	LPOSIX_FUNC( Wclrtobot		),
	LPOSIX_FUNC( Wclrtoeol		),
	LPOSIX_FUNC( Wcopywin		),
	LPOSIX_FUNC( Wcursyncup		),
	LPOSIX_FUNC( Wdelch		),
	LPOSIX_FUNC( Wdeleteln		),
	LPOSIX_FUNC( Wderive		),
	LPOSIX_FUNC( Wechoch		),
	LPOSIX_FUNC( Werase		),
	LPOSIX_FUNC( Wgetbegyx		),
	LPOSIX_FUNC( Wgetbkgd		),
	LPOSIX_FUNC( Wgetch		),
	LPOSIX_FUNC( Wgetmaxyx		),
	LPOSIX_FUNC( Wgetparyx		),
	LPOSIX_FUNC( Wgetstr		),
	LPOSIX_FUNC( Wgetyx		),
	LPOSIX_FUNC( Whline		),
	LPOSIX_FUNC( Widcok		),
	LPOSIX_FUNC( Widlok		),
	LPOSIX_FUNC( Wimmedok		),
	LPOSIX_FUNC( Winsertln		),
	LPOSIX_FUNC( Wintrflush		),
	LPOSIX_FUNC( Wis_linetouched	),
	LPOSIX_FUNC( Wis_wintouched	),
	LPOSIX_FUNC( Wkeypad		),
	LPOSIX_FUNC( Wleaveok		),
	LPOSIX_FUNC( Wmeta		),
	LPOSIX_FUNC( Wmove		),
	LPOSIX_FUNC( Wmove_derived	),
	LPOSIX_FUNC( Wmove_window	),
	LPOSIX_FUNC( Wmvaddch		),
	LPOSIX_FUNC( Wmvaddchstr	),
	LPOSIX_FUNC( Wmvaddstr		),
	LPOSIX_FUNC( Wmvdelch		),
	LPOSIX_FUNC( Wmvgetch		),
	LPOSIX_FUNC( Wmvgetstr		),
	LPOSIX_FUNC( Wmvhline		),
	LPOSIX_FUNC( Wmvvline		),
	LPOSIX_FUNC( Wmvwinch		),
	LPOSIX_FUNC( Wmvwinchnstr	),
	LPOSIX_FUNC( Wmvwinnstr		),
	LPOSIX_FUNC( Wmvwinsch		),
	LPOSIX_FUNC( Wmvwinsnstr	),
	LPOSIX_FUNC( Wmvwinsstr		),
	LPOSIX_FUNC( Wnodelay		),
	LPOSIX_FUNC( Wnotimeout		),
	LPOSIX_FUNC( Wnoutrefresh	),
	LPOSIX_FUNC( Woverlay		),
	LPOSIX_FUNC( Woverwrite		),
	LPOSIX_FUNC( Wpechochar		),
	LPOSIX_FUNC( Wpnoutrefresh	),
	LPOSIX_FUNC( Wprefresh		),
	LPOSIX_FUNC( Wredrawln		),
	LPOSIX_FUNC( Wredrawwin		),
	LPOSIX_FUNC( Wrefresh		),
	LPOSIX_FUNC( Wresize		),
	LPOSIX_FUNC( Wscrl		),
	LPOSIX_FUNC( Wscrollok		),
	LPOSIX_FUNC( Wstandend		),
	LPOSIX_FUNC( Wstandout		),
	LPOSIX_FUNC( Wsub		),
	LPOSIX_FUNC( Wsubpad		),
	LPOSIX_FUNC( Wsyncdown		),
	LPOSIX_FUNC( Wsyncok		),
	LPOSIX_FUNC( Wsyncup		),
	LPOSIX_FUNC( Wtimeout		),
	LPOSIX_FUNC( Wtouch		),
	LPOSIX_FUNC( Wtouchline		),
	LPOSIX_FUNC( Wvline		),
	LPOSIX_FUNC( Wwbkgd		),
	LPOSIX_FUNC( Wwbkgdset		),
	LPOSIX_FUNC( Wwinch		),
	LPOSIX_FUNC( Wwinchnstr		),
	LPOSIX_FUNC( Wwinnstr		),
	LPOSIX_FUNC( Wwinsch		),
	LPOSIX_FUNC( Wwinsdelln		),
	LPOSIX_FUNC( Wwinsnstr		),
	LPOSIX_FUNC( Wwinsstr		),
	LPOSIX_FUNC( Wwsetscrreg	),
	{"__gc",     Wclose		}, /* rough safety net */
	{NULL, NULL}
};


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

/* Prototype to keep compiler happy. */
LUALIB_API int luaopen_curses_c (lua_State *L);

int luaopen_posix_curses (lua_State *L)
{
#if HAVE_CURSES
	/*
	** create new metatable for window objects
	*/
	luaL_newmetatable(L, WINDOWMETA);
	lua_pushliteral(L, "__index");
	lua_pushvalue(L, -2);			   /* push metatable */
	lua_rawset(L, -3);				  /* metatable.__index = metatable */
	luaL_openlib(L, NULL, windowlib, 0);

	lua_pop(L, 1);					  /* remove metatable from stack */

	/*
	** create new metatable for chstr objects
	*/
	luaL_newmetatable(L, CHSTRMETA);
	lua_pushliteral(L, "__index");
	lua_pushvalue(L, -2);			   /* push metatable */
	lua_rawset(L, -3);				  /* metatable.__index = metatable */
	luaL_openlib(L, NULL, chstrlib, 0);

	lua_pop(L, 1);					  /* remove metatable from stack */
#endif

	/*
	** create global table with curses methods/variables/constants
	*/
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
