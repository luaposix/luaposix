/*************************************************************************
 * Library: lcurses - Lua 5.1 interface to the curses library            *
 *                                                                       *
 * (c) Reuben Thomas <rrt@sc3d.org> 2009-2012                            *
 * (c) Tiago Dionizio <tiago.dionizio AT gmail.com> 2004-2007            *
 *                                                                       *
 * Permission is hereby granted, free of charge, to any person obtaining *
 * a copy of this software and associated documentation files (the       *
 * "Software"), to deal in the Software without restriction, including   *
 * without limitation the rights to use, copy, modify, merge, publish,   *
 * distribute, sublicense, and/or sell copies of the Software, and to    *
 * permit persons to whom the Software is furnished to do so, subject to *
 * the following conditions:                                             *
 *                                                                       *
 * The above copyright notice and this permission notice shall be        *
 * included in all copies or substantial portions of the Software.       *
 *                                                                       *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,       *
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF    *
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.*
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY  *
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,  *
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE     *
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                *
 ************************************************************************/

#include "config.h"

#include <stdlib.h>
#include <string.h>
#include <lua.h>
#include <lauxlib.h>
#include "lua52compat.h"
#if defined(HAVE_NCURSESW_CURSES_H)
#  include <ncursesw/curses.h>
#elif defined(HAVE_NCURSESW_H)
#  include <ncursesw.h>
#elif defined(HAVE_NCURSES_CURSES_H)
#  include <ncurses/curses.h>
#elif defined(HAVE_NCURSES_H)
#  include <ncurses.h>
#elif defined(HAVE_CURSES_H)
#  include <curses.h>
#else
#  error "SysV or X/Open-compatible Curses header file required"
#endif
#include <term.h>

/* The extra indirection to these macros is required so that if the
   arguments are themselves macros, they will get expanded too.  */
#define LCURSES__SPLICE(_s, _t)	_s##_t
#define LCURSES_SPLICE(_s, _t)	LCURSES__SPLICE(_s, _t)

#define LCURSES__STR(_s)	#_s
#define LCURSES_STR(_s)		LCURSES__STR(_s)

/* The +1 is to step over the leading '_' that is required to prevent
   premature expansion of MENTRY arguments if we didn't add it.  */
#define LCURSES__STR_1(_s)	(#_s + 1)
#define LCURSES_STR_1(_s)	LCURSES__STR_1(_s)

/* strlcpy() implementation for non-BSD based Unices.
   strlcpy() is a safer less error-prone replacement for strncpy(). */
#include "strlcpy.c"


/*
** =======================================================
** defines
** =======================================================
*/
static const char *STDSCR_REGISTRY     = "curses:stdscr";
static const char *WINDOWMETA          = "curses:window";
static const char *CHSTRMETA           = "curses:chstr";
static const char *RIPOFF_TABLE        = "curses:ripoffline";

#define B(v) ((((int) (v)) == OK))

/* ======================================================= */

#define LC_NUMBER(v)                        \
    static int C ## v(lua_State *L)         \
    {                                       \
        lua_pushinteger(L, v());            \
        return 1;                           \
    }

#define LC_NUMBER2(n,v)                     \
    static int n(lua_State *L)              \
    {                                       \
        lua_pushinteger(L, v);              \
        return 1;                           \
    }

/* ======================================================= */

#define LC_STRING(v)                        \
    static int C ## v(lua_State *L)         \
    {                                       \
        lua_pushstring(L, v());             \
        return 1;                           \
    }

#define LC_STRING2(n,v)                     \
    static int n(lua_State *L)              \
    {                                       \
        lua_pushstring(L, v);               \
        return 1;                           \
    }

/* ======================================================= */

#define LC_BOOL(v)                          \
    static int C ## v(lua_State *L)         \
    {                                       \
        lua_pushboolean(L, v());            \
        return 1;                           \
    }

/* ======================================================= */

#define LC_BOOLOK(v)                        \
    static int C ## v(lua_State *L)         \
    {                                       \
        lua_pushboolean(L, B(v()));         \
        return 1;                           \
    }

/* ======================================================= */

#define LCW_BOOLOK(n)                       \
    static int W ## n(lua_State *L)         \
    {                                       \
        WINDOW *w = checkwin(L, 1);         \
        lua_pushboolean(L, B(n(w)));        \
        return 1;                           \
    }

#define LCW_BOOLOK2(n,v)                    \
    static int n(lua_State *L)              \
    {                                       \
        WINDOW *w = checkwin(L, 1);         \
        lua_pushboolean(L, B(v(w)));        \
        return 1;                           \
    }


/*
** =======================================================
** privates
** =======================================================
*/
static void lc_newwin(lua_State *L, WINDOW *nw)
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
    if (w == NULL) luaL_argerror(L, offset, "bad curses window");
    return w;
}

static WINDOW *checkwin(lua_State *L, int offset)
{
    WINDOW **w = lc_getwin(L, offset);
    if (*w == NULL) luaL_argerror(L, offset, "attempt to use closed curses window");
    return *w;
}

static int W__tostring(lua_State *L)
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

/*
** =======================================================
** chtype handling
** =======================================================
*/
static chtype checkch(lua_State *L, int offset)
{
    if (lua_type(L, offset) == LUA_TNUMBER)
        return luaL_checknumber(L, offset);
    if (lua_type(L, offset) == LUA_TSTRING)
        return *lua_tostring(L, offset);

    luaL_typerror(L, offset, "chtype");
    /* never executes */
    return 0;
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
    if (cs) return cs;

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
static int Cnew_chstr(lua_State *L)
{
    int len = luaL_checkint(L, 1);
    chstr* ncs = chstr_new(L, len);
    memset(ncs->str, ' ', len*sizeof(chtype));
    return 1;
}

/* change the contents of the chstr */
static int chstr_set_str(lua_State *L)
{
    chstr *cs = checkchstr(L, 1);
    int offset = luaL_checkint(L, 2);
    const char *str = luaL_checkstring(L, 3);
    int len = lua_strlen(L, 3);
    int attr = luaL_optnumber(L, 4, A_NORMAL);
    int rep = luaL_optint(L, 5, 1);
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
 *       size = 10
 *       str = curses.new_chstr(10)
 *       str:set_ch(0, 'A', curses.A_BOLD)
 *       str:set_ch(1, 'a', curses.A_NORMAL, size - 1)
 ****/
static int chstr_set_ch(lua_State *L)
{
    chstr* cs = checkchstr(L, 1);
    int offset = luaL_checkint(L, 2);
    chtype ch = checkch(L, 3);
    int attr = luaL_optnumber(L, 4, A_NORMAL);
    int rep = luaL_optint(L, 5, 1);

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
static int chstr_get(lua_State *L)
{
    chstr* cs = checkchstr(L, 1);
    int offset = luaL_checkint(L, 2);
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
static int chstr_len(lua_State *L)
{
    chstr *cs = checkchstr(L, 1);
    lua_pushinteger(L, cs->len);
    return 1;
}

/* duplicate chstr */
static int chstr_dup(lua_State *L)
{
    chstr *cs = checkchstr(L, 1);
    chstr *ncs = chstr_new(L, cs->len);

    memcpy(ncs->str, cs->str, CHSTR_SIZE(cs->len));
    return 1;
}

/*
** =======================================================
** initscr
** =======================================================
*/

#define CCR(n, v)                       \
    lua_pushstring(L, n);               \
    lua_pushinteger(L, v);              \
    lua_settable(L, lua_upvalueindex(1));

#define CC(s)       CCR(#s, s)
#define CF(i)       CCR(LCURSES_STR(LCURSES_SPLICE(KEY_F, i)), KEY_F(i))

/*
** these values may be fixed only after initialization, so this is
** called from Cinitscr, after the curses driver is initialized
**
** curses table is kept at upvalue position 1, in case the global
** name is changed by the user or even in the registration phase by
** the developer
**
** some of these values are not constant so need to register
** them directly instead of using a table
*/
static void register_curses_constants(lua_State *L)
{
    /* colors */
    CC(COLOR_BLACK)     CC(COLOR_RED)       CC(COLOR_GREEN)
    CC(COLOR_YELLOW)    CC(COLOR_BLUE)      CC(COLOR_MAGENTA)
    CC(COLOR_CYAN)      CC(COLOR_WHITE)

    /* alternate character set */
    CC(ACS_BLOCK)       CC(ACS_BOARD)

    CC(ACS_BTEE)        CC(ACS_TTEE)
    CC(ACS_LTEE)        CC(ACS_RTEE)
    CC(ACS_LLCORNER)    CC(ACS_LRCORNER)
    CC(ACS_URCORNER)    CC(ACS_ULCORNER)

    CC(ACS_LARROW)      CC(ACS_RARROW)
    CC(ACS_UARROW)      CC(ACS_DARROW)

    CC(ACS_HLINE)       CC(ACS_VLINE)

    CC(ACS_BULLET)      CC(ACS_CKBOARD)     CC(ACS_LANTERN)
    CC(ACS_DEGREE)      CC(ACS_DIAMOND)

    CC(ACS_PLMINUS)     CC(ACS_PLUS)
    CC(ACS_S1)          CC(ACS_S9)

    /* attributes */
    CC(A_NORMAL)        CC(A_STANDOUT)      CC(A_UNDERLINE)
    CC(A_REVERSE)       CC(A_BLINK)         CC(A_DIM)
    CC(A_BOLD)          CC(A_PROTECT)       CC(A_INVIS)
    CC(A_ALTCHARSET)    CC(A_CHARTEXT)
    CC(A_ATTRIBUTES)

    /* key functions */
    CC(KEY_BREAK)       CC(KEY_DOWN)        CC(KEY_UP)
    CC(KEY_LEFT)        CC(KEY_RIGHT)       CC(KEY_HOME)
    CC(KEY_BACKSPACE)

    CC(KEY_DL)          CC(KEY_IL)          CC(KEY_DC)
    CC(KEY_IC)          CC(KEY_EIC)         CC(KEY_CLEAR)
    CC(KEY_EOS)         CC(KEY_EOL)         CC(KEY_SF)
    CC(KEY_SR)          CC(KEY_NPAGE)       CC(KEY_PPAGE)
    CC(KEY_STAB)        CC(KEY_CTAB)        CC(KEY_CATAB)
    CC(KEY_ENTER)       CC(KEY_SRESET)      CC(KEY_RESET)
    CC(KEY_PRINT)       CC(KEY_LL)          CC(KEY_A1)
    CC(KEY_A3)          CC(KEY_B2)          CC(KEY_C1)
    CC(KEY_C3)          CC(KEY_BTAB)        CC(KEY_BEG)
    CC(KEY_CANCEL)      CC(KEY_CLOSE)       CC(KEY_COMMAND)
    CC(KEY_COPY)        CC(KEY_CREATE)      CC(KEY_END)
    CC(KEY_EXIT)        CC(KEY_FIND)        CC(KEY_HELP)
    CC(KEY_MARK)        CC(KEY_MESSAGE)     CC(KEY_MOUSE)
    CC(KEY_MOVE)        CC(KEY_NEXT)        CC(KEY_OPEN)
    CC(KEY_OPTIONS)     CC(KEY_PREVIOUS)    CC(KEY_REDO)
    CC(KEY_REFERENCE)   CC(KEY_REFRESH)     CC(KEY_REPLACE)
    CC(KEY_RESIZE)      CC(KEY_RESTART)     CC(KEY_RESUME)
    CC(KEY_SAVE)        CC(KEY_SBEG)        CC(KEY_SCANCEL)
    CC(KEY_SCOMMAND)    CC(KEY_SCOPY)       CC(KEY_SCREATE)
    CC(KEY_SDC)         CC(KEY_SDL)         CC(KEY_SELECT)
    CC(KEY_SEND)        CC(KEY_SEOL)        CC(KEY_SEXIT)
    CC(KEY_SFIND)       CC(KEY_SHELP)       CC(KEY_SHOME)
    CC(KEY_SIC)         CC(KEY_SLEFT)       CC(KEY_SMESSAGE)
    CC(KEY_SMOVE)       CC(KEY_SNEXT)       CC(KEY_SOPTIONS)
    CC(KEY_SPREVIOUS)   CC(KEY_SPRINT)      CC(KEY_SREDO)
    CC(KEY_SREPLACE)    CC(KEY_SRIGHT)      CC(KEY_SRSUME)
    CC(KEY_SSAVE)       CC(KEY_SSUSPEND)    CC(KEY_SUNDO)
    CC(KEY_SUSPEND)     CC(KEY_UNDO)

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
static void cleanup(void)
{
    if (!isendwin())
    {
        wclear(stdscr);
        wrefresh(stdscr);
        endwin();
    }
}

static int Cinitscr(lua_State *L)
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
static int Cendwin(lua_State *L)
{
    (void) L;
    endwin();
    return 0;
}

LC_BOOL(isendwin)

static int Cstdscr(lua_State *L)
{
    lua_pushstring(L, STDSCR_REGISTRY);
    lua_rawget(L, LUA_REGISTRYINDEX);
    return 1;
}

LC_NUMBER2(Ccols, COLS)
LC_NUMBER2(Clines, LINES)

/*
** =======================================================
** color
** =======================================================
*/

LC_BOOLOK(start_color)
LC_BOOL(has_colors)
LC_BOOLOK(use_default_colors)

static int Cinit_pair(lua_State *L)
{
    short pair = luaL_checkint(L, 1);
    short f = luaL_checkint(L, 2);
    short b = luaL_checkint(L, 3);

    lua_pushboolean(L, B(init_pair(pair, f, b)));
    return 1;
}

static int Cpair_content(lua_State *L)
{
    short pair = luaL_checkint(L, 1);
    short f;
    short b;
    int ret = pair_content(pair, &f, &b);

    if (ret == ERR)
        return 0;

    lua_pushinteger(L, f);
    lua_pushinteger(L, b);
    return 2;
}

LC_NUMBER2(Ccolors, COLORS)
LC_NUMBER2(Ccolor_pairs, COLOR_PAIRS)

static int Ccolor_pair(lua_State *L)
{
    int n = luaL_checkint(L, 1);
    lua_pushinteger(L, COLOR_PAIR(n));
    return 1;
}

/*
** =======================================================
** termattrs
** =======================================================
*/

LC_NUMBER(baudrate)
LC_NUMBER(erasechar)
LC_BOOL(has_ic)
LC_BOOL(has_il)
LC_NUMBER(killchar)

static int Ctermattrs(lua_State *L)
{
    if (lua_gettop(L) < 1)
        lua_pushinteger(L, termattrs());
    else
    {
        int a = luaL_checkint(L, 1);
        lua_pushboolean(L, termattrs() & a);
    }
    return 1;
}

LC_STRING(termname)
LC_STRING(longname)

/*
** =======================================================
** kernel
** =======================================================
*/

/* there is no easy way to implement this... */
static lua_State *rip_L = NULL;
static int ripoffline_cb(WINDOW* w, int cols)
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

    lua_rawgeti(rip_L, -1, ++line); /* function to be called */
    lc_newwin(rip_L, w);            /* create window object */
    lua_pushinteger(rip_L, cols);   /* push number of columns */

    lua_pcall(rip_L, 2,  0, 0);     /* call the lua function */

    lua_settop(rip_L, top);
    return 1;
}

static int Cripoffline(lua_State *L)
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
    lua_pushboolean(L, B(ripoffline(top_line ? 1 : -1, ripoffline_cb)));
    return 1;
}

static int Ccurs_set(lua_State *L)
{
    int vis = luaL_checkint(L, 1);
    int state = curs_set(vis);
    if (state == ERR)
        return 0;

    lua_pushinteger(L, state);
    return 1;
}

static int Cnapms(lua_State *L)
{
    int ms = luaL_checkint(L, 1);
    lua_pushboolean(L, B(napms(ms)));
    return 1;
}

/*
** =======================================================
** resizeterm
** =======================================================
*/

static int Cresizeterm(lua_State *L)
{
    int nlines  = luaL_checkint(L, 1);
    int ncols   = luaL_checkint(L, 2);
#if HAVE_RESIZETERM
    lua_pushboolean(L, B(resizeterm (nlines, ncols)));
    return 1;
#else
    return luaL_error (L, "`resizeterm' is not implemented by your curses library");
#endif
}

/*
** =======================================================
** beep
** =======================================================
*/
LC_BOOLOK(beep)
LC_BOOLOK(flash)


/*
** =======================================================
** window
** =======================================================
*/

static int Cnewwin(lua_State *L)
{
    int nlines  = luaL_checkint(L, 1);
    int ncols   = luaL_checkint(L, 2);
    int begin_y = luaL_checkint(L, 3);
    int begin_x = luaL_checkint(L, 4);

    lc_newwin(L, newwin(nlines, ncols, begin_y, begin_x));
    return 1;
}

static int Wclose(lua_State *L)
{
    WINDOW **w = lc_getwin(L, 1);
    if (*w != NULL && *w != stdscr)
    {
        delwin(*w);
        *w = NULL;
    }
    return 0;
}

static int Wmove_window(lua_State *L)
{
    WINDOW *w = checkwin(L, 1);
    int y = luaL_checkint(L, 2);
    int x = luaL_checkint(L, 3);
    lua_pushboolean(L, B(mvwin(w, y, x)));
    return 1;
}

static int Wsub(lua_State *L)
{
    WINDOW *orig = checkwin(L, 1);
    int nlines  = luaL_checkint(L, 2);
    int ncols   = luaL_checkint(L, 3);
    int begin_y = luaL_checkint(L, 4);
    int begin_x = luaL_checkint(L, 5);

    lc_newwin(L, subwin(orig, nlines, ncols, begin_y, begin_x));
    return 1;
}

static int Wderive(lua_State *L)
{
    WINDOW *orig = checkwin(L, 1);
    int nlines  = luaL_checkint(L, 2);
    int ncols   = luaL_checkint(L, 3);
    int begin_y = luaL_checkint(L, 4);
    int begin_x = luaL_checkint(L, 5);

    lc_newwin(L, derwin(orig, nlines, ncols, begin_y, begin_x));
    return 1;
}

static int Wmove_derived(lua_State *L)
{
    WINDOW *w = checkwin(L, 1);
    int par_y = luaL_checkint(L, 2);
    int par_x = luaL_checkint(L, 3);
    lua_pushboolean(L, B(mvderwin(w, par_y, par_x)));
    return 1;
}

#define LCW_WIN2(n, v)                      \
    static int n(lua_State *L)              \
    {                                       \
        WINDOW *w = checkwin(L, 1);         \
        v(w);                               \
        return 0;                           \
    }

static int Wclone(lua_State *L)
{
    WINDOW *w = checkwin(L, 1);
    lc_newwin(L, dupwin(w));
    return 1;
}

LCW_WIN2(Wsyncup, wsyncup)

static int Wsyncok(lua_State *L)
{
    WINDOW *w = checkwin(L, 1);
    int bf = lua_toboolean(L, 2);
    lua_pushboolean(L, B(syncok(w, bf)));
    return 1;
}

LCW_WIN2(Wcursyncup, wcursyncup)
LCW_WIN2(Wsyncdown, wsyncdown)


/*
** =======================================================
** refresh
** =======================================================
*/
LCW_BOOLOK2(Wrefresh, wrefresh)
LCW_BOOLOK2(Wnoutrefresh, wnoutrefresh)
LCW_BOOLOK(redrawwin)

static int Wredrawln(lua_State *L)
{
    WINDOW *w = checkwin(L, 1);
    int beg_line = luaL_checkint(L, 2);
    int num_lines = luaL_checkint(L, 3);
    lua_pushboolean(L, B(wredrawln(w, beg_line, num_lines)));
    return 1;
}

LC_BOOLOK(doupdate)

/*
** =======================================================
** move
** =======================================================
*/

static int Wmove(lua_State *L)
{
    WINDOW *w = checkwin(L, 1);
    int y = luaL_checkint(L, 2);
    int x = luaL_checkint(L, 3);
    lua_pushboolean(L, B(wmove(w, y, x)));
    return 1;
}

/*
** =======================================================
** scroll
** =======================================================
*/

static int Wscrl(lua_State *L)
{
    WINDOW *w = checkwin(L, 1);
    int n = luaL_checkint(L, 2);
    lua_pushboolean(L, B(wscrl(w, n)));
    return 1;
}

/*
** =======================================================
** touch
** =======================================================
*/

static int Wtouch(lua_State *L)
{
    WINDOW *w = checkwin(L, 1);
    int changed;
    if (lua_isnoneornil(L, 2))
        changed = TRUE;
    else
        changed = lua_toboolean(L, 2);

    if (changed)
        lua_pushboolean(L, B(touchwin(w)));
    else
        lua_pushboolean(L, B(untouchwin(w)));
    return 1;
}

static int Wtouchline(lua_State *L)
{
    WINDOW *w = checkwin(L, 1);
    int y = luaL_checkint(L, 2);
    int n = luaL_checkint(L, 3);
    int changed;
    if (lua_isnoneornil(L, 4))
        changed = TRUE;
    else
        changed = lua_toboolean(L, 4);
    lua_pushboolean(L, B(wtouchln(w, y, n, changed)));
    return 1;
}

static int Wis_linetouched(lua_State *L)
{
    WINDOW *w = checkwin(L, 1);
    int line = luaL_checkint(L, 2);
    lua_pushboolean(L, is_linetouched(w, line));
    return 1;
}

static int Wis_wintouched(lua_State *L)
{
    WINDOW *w = checkwin(L, 1);
    lua_pushboolean(L, is_wintouched(w));
    return 1;
}

/*
** =======================================================
** getyx
** =======================================================
*/

#define LCW_WIN2YX(v)                       \
    static int W ## v(lua_State *L)         \
    {                                       \
        WINDOW *w = checkwin(L, 1);         \
        int y, x;                           \
        v(w, y, x);                         \
        lua_pushinteger(L, y);              \
        lua_pushinteger(L, x);              \
        return 2;                           \
    }

LCW_WIN2YX(getyx)
LCW_WIN2YX(getparyx)
LCW_WIN2YX(getbegyx)
LCW_WIN2YX(getmaxyx)

/*
** =======================================================
** border
** =======================================================
*/

static int Wborder(lua_State *L)
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

    lua_pushinteger(L, B(wborder(w, ls, rs, ts, bs, tl, tr, bl, br)));
    return 1;
}

static int Wbox(lua_State *L)
{
    WINDOW *w = checkwin(L, 1);
    chtype verch = checkch(L, 2);
    chtype horch = checkch(L, 3);

    lua_pushinteger(L, B(box(w, verch, horch)));
    return 1;
}

static int Whline(lua_State *L)
{
    WINDOW *w = checkwin(L, 1);
    chtype ch = checkch(L, 2);
    int n = luaL_checkint(L, 3);

    lua_pushboolean(L, B(whline(w, ch, n)));
    return 1;
}

static int Wvline(lua_State *L)
{
    WINDOW *w = checkwin(L, 1);
    chtype ch = checkch(L, 2);
    int n = luaL_checkint(L, 3);

    lua_pushboolean(L, B(wvline(w, ch, n)));
    return 1;
}


static int Wmvhline(lua_State *L)
{
    WINDOW *w = checkwin(L, 1);
    int y = luaL_checkint(L, 2);
    int x = luaL_checkint(L, 3);
    chtype ch = checkch(L, 4);
    int n = luaL_checkint(L, 5);

    lua_pushboolean(L, B(mvwhline(w, y, x, ch, n)));
    return 1;
}

static int Wmvvline(lua_State *L)
{
    WINDOW *w = checkwin(L, 1);
    int y = luaL_checkint(L, 2);
    int x = luaL_checkint(L, 3);
    chtype ch = checkch(L, 4);
    int n = luaL_checkint(L, 5);

    lua_pushboolean(L, B(mvwvline(w, y, x, ch, n)));
    return 1;
}

/*
** =======================================================
** clear
** =======================================================
*/

LCW_BOOLOK2(Werase, werase)
LCW_BOOLOK2(Wclear, wclear)
LCW_BOOLOK2(Wclrtobot, wclrtobot)
LCW_BOOLOK2(Wclrtoeol, wclrtoeol)

/*
** =======================================================
** slk
** =======================================================
*/
static int Cslk_init(lua_State *L)
{
    int fmt = luaL_checkint(L, 1);
    lua_pushboolean(L, B(slk_init(fmt)));
    return 1;
}

static int Cslk_set(lua_State *L)
{
    int labnum = luaL_checkint(L, 1);
    const char* label = luaL_checkstring(L, 2);
    int fmt = luaL_checkint(L, 3);

    lua_pushboolean(L, B(slk_set(labnum, label, fmt)));
    return 1;
}

LC_BOOLOK(slk_refresh)
LC_BOOLOK(slk_noutrefresh)

static int Cslk_label(lua_State *L)
{
    int labnum = luaL_checkint(L, 1);
    lua_pushstring(L, slk_label(labnum));
    return 1;
}

LC_BOOLOK(slk_clear)
LC_BOOLOK(slk_restore)
LC_BOOLOK(slk_touch)

#define LC_ATTROK(v)                        \
    static int C ## v(lua_State *L)         \
    {                                       \
        chtype attrs = checkch(L, 1);       \
        lua_pushboolean(L, B(v(attrs)));    \
        return 1;                           \
    }

LC_ATTROK(slk_attron)
LC_ATTROK(slk_attroff)
LC_ATTROK(slk_attrset)


/*
** =======================================================
** addch
** =======================================================
*/

static int Waddch(lua_State *L)
{
    WINDOW *w = checkwin(L, 1);
    chtype ch = checkch(L, 2);
    lua_pushboolean(L, B(waddch(w, ch)));
    return 1;
}

static int Wmvaddch(lua_State *L)
{
    WINDOW *w = checkwin(L, 1);
    int y = luaL_checkint(L, 2);
    int x = luaL_checkint(L, 3);
    chtype ch = checkch(L, 4);

    lua_pushboolean(L, B(mvwaddch(w, y, x, ch)));
    return 1;
}

static int Wechoch(lua_State *L)
{
    WINDOW *w = checkwin(L, 1);
    chtype ch = checkch(L, 2);

    lua_pushboolean(L, B(wechochar(w, ch)));
    return 1;
}

/*
** =======================================================
** addchstr
** =======================================================
*/

static int Waddchstr(lua_State *L)
{
    WINDOW *w = checkwin(L, 1);
    int n = luaL_optint(L, 3, -1);
    chstr *cs = checkchstr(L, 2);

    if (n < 0 || n > (int) cs->len)
        n = cs->len;

    lua_pushboolean(L, B(waddchnstr(w, cs->str, n)));
    return 1;
}

static int Wmvaddchstr(lua_State *L)
{
    WINDOW *w = checkwin(L, 1);
    int y = luaL_checkint(L, 2);
    int x = luaL_checkint(L, 3);
    int n = luaL_optint(L, 5, -1);
    chstr *cs = checkchstr(L, 4);

    if (n < 0 || n > (int) cs->len)
        n = cs->len;

    lua_pushboolean(L, B(mvwaddchnstr(w, y, x, cs->str, n)));
    return 1;
}

/*
** =======================================================
** addstr
** =======================================================
*/

static int Waddstr(lua_State *L)
{
    WINDOW *w = checkwin(L, 1);
    const char *str = luaL_checkstring(L, 2);
    int n = luaL_optint(L, 3, -1);
    lua_pushboolean(L, B(waddnstr(w, str, n)));
    return 1;
}

static int Wmvaddstr(lua_State *L)
{
    WINDOW *w = checkwin(L, 1);
    int y = luaL_checkint(L, 2);
    int x = luaL_checkint(L, 3);
    const char *str = luaL_checkstring(L, 4);
    int n = luaL_optint(L, 5, -1);
    lua_pushboolean(L, B(mvwaddnstr(w, y, x, str, n)));
    return 1;
}

/*
** =======================================================
** bkgd
** =======================================================
*/

static int Wwbkgdset(lua_State *L)
{
    WINDOW *w = checkwin(L, 1);
    chtype ch = checkch(L, 2);
    wbkgdset(w, ch);
    return 0;
}

static int Wwbkgd(lua_State *L)
{
    WINDOW *w = checkwin(L, 1);
    chtype ch = checkch(L, 2);
    lua_pushboolean(L, B(wbkgd(w, ch)));
    return 1;
}

static int Wgetbkgd(lua_State *L)
{
    WINDOW *w = checkwin(L, 1);
    lua_pushboolean(L, B(getbkgd(w)));
    return 1;
}

/*
** =======================================================
** inopts
** =======================================================
*/

#define LC_TOGGLEOK(v)                                   \
    static int C ## v(lua_State *L)                      \
    {                                                    \
        if (lua_isnoneornil(L, 1) || lua_toboolean(L, 1))\
            lua_pushboolean(L, B(v()));                  \
        else                                             \
            lua_pushboolean(L, B(no ## v()));            \
        return 1;                                        \
    }

LC_TOGGLEOK(cbreak)
LC_TOGGLEOK(echo)
LC_TOGGLEOK(raw)

static int Chalfdelay(lua_State *L)
{
    int tenths = luaL_checkint(L, 1);
    lua_pushboolean(L, B(halfdelay(tenths)));
    return 1;
}
#define LCW_WINBOOLOK(v)                   \
    static int W ## v(lua_State *L)        \
    {                                      \
        WINDOW *w = checkwin(L, 1);        \
        int bf = lua_toboolean(L, 2);      \
        lua_pushboolean(L, B(v(w, bf)));   \
        return 1;                          \
    }

LCW_WINBOOLOK(intrflush)

static int Wkeypad(lua_State *L)
{
    WINDOW *w = checkwin(L, 1);
    int bf = lua_isnoneornil(L, 2) ? 1 : lua_toboolean(L, 2);
    lua_pushboolean(L, B(keypad(w, bf)));
    return 1;
}

LCW_WINBOOLOK(meta)
LCW_WINBOOLOK(nodelay)

static int Wtimeout(lua_State *L)
{
    WINDOW *w = checkwin(L, 1);
    int delay = luaL_checkint(L, 2);
    wtimeout(w, delay);
    return 0;
}

static int Wnotimeout(lua_State *L)
{
    WINDOW *w = checkwin(L, 1);
    int bf = lua_toboolean(L, 2);
    lua_pushboolean(L, B(notimeout(w, bf)));
    return 1;
}

/*
** =======================================================
** outopts
** =======================================================
*/

LC_TOGGLEOK(nl)
LCW_WINBOOLOK(clearok)
LCW_WINBOOLOK(idlok)
LCW_WINBOOLOK(leaveok)
LCW_WINBOOLOK(scrollok)

static int Widcok(lua_State *L)
{
    WINDOW *w = checkwin(L, 1);
    int bf = lua_toboolean(L, 2);
    idcok(w, bf);
    return 0;
}

static int Wimmedok(lua_State *L)
{
    WINDOW *w = checkwin(L, 1);
    int bf = lua_toboolean(L, 2);
    immedok(w, bf);
    return 0;
}

static int Wwsetscrreg(lua_State *L)
{
    WINDOW *w = checkwin(L, 1);
    int top = luaL_checkint(L, 2);
    int bot = luaL_checkint(L, 3);
    lua_pushboolean(L, B(wsetscrreg(w, top, bot)));
    return 1;
}

/*
** =======================================================
** overlay
** =======================================================
*/

static int Woverlay(lua_State *L)
{
    WINDOW *srcwin = checkwin(L, 1);
    WINDOW *dstwin = checkwin(L, 2);

    lua_pushboolean(L, B(overlay(srcwin, dstwin)));
    return 1;
}

static int Woverwrite(lua_State *L)
{
    WINDOW *srcwin = checkwin(L, 1);
    WINDOW *dstwin = checkwin(L, 2);

    lua_pushboolean(L, B(overwrite(srcwin, dstwin)));
    return 1;
}

static int Wcopywin(lua_State *L)
{
    WINDOW *srcwin = checkwin(L, 1);
    WINDOW *dstwin = checkwin(L, 2);
    int sminrow = luaL_checkint(L, 3);
    int smincol = luaL_checkint(L, 4);
    int dminrow = luaL_checkint(L, 5);
    int dmincol = luaL_checkint(L, 6);
    int dmaxrow = luaL_checkint(L, 7);
    int dmaxcol = luaL_checkint(L, 8);
    int woverlay = lua_toboolean(L, 9);

    lua_pushboolean(L, B(copywin(srcwin, dstwin, sminrow,
        smincol, dminrow, dmincol, dmaxrow, dmaxcol, woverlay)));

    return 1;
}

/*
** =======================================================
** util
** =======================================================
*/

static int Cunctrl(lua_State *L)
{
    chtype c = luaL_checknumber(L, 1);
    lua_pushstring(L, unctrl(c));
    return 1;
}

static int Ckeyname(lua_State *L)
{
    int c = luaL_checkint(L, 1);
    lua_pushstring(L, keyname(c));
    return 1;
}

static int Cdelay_output(lua_State *L)
{
    int ms = luaL_checkint(L, 1);
    lua_pushboolean(L, B(delay_output(ms)));
    return 1;
}

LC_BOOL(flushinp)

/*
** =======================================================
** delch
** =======================================================
*/

LCW_BOOLOK2(Wdelch, wdelch)

static int Wmvdelch(lua_State *L)
{
    WINDOW *w = checkwin(L, 1);
    int y = luaL_checkint(L, 2);
    int x = luaL_checkint(L, 3);

    lua_pushboolean(L, B(mvwdelch(w, y, x)));
    return 1;
}

/*
** =======================================================
** deleteln
** =======================================================
*/

LCW_BOOLOK2(Wdeleteln, wdeleteln)
LCW_BOOLOK2(Winsertln, winsertln)

static int Wwinsdelln(lua_State *L)
{
    WINDOW *w = checkwin(L, 1);
    int n = luaL_checkint(L, 2);
    lua_pushboolean(L, B(winsdelln(w, n)));
    return 1;
}

/*
** =======================================================
** getch
** =======================================================
*/

static int Wgetch(lua_State *L)
{
    WINDOW *w = checkwin(L, 1);
    int c = wgetch(w);

    if (c == ERR) return 0;

    lua_pushinteger(L, c);
    return 1;
}

static int Wmvgetch(lua_State *L)
{
    WINDOW *w = checkwin(L, 1);
    int y = luaL_checkint(L, 2);
    int x = luaL_checkint(L, 3);
    int c;

    if (wmove(w, y, x) == ERR) return 0;

    c = wgetch(w);

    if (c == ERR) return 0;

    lua_pushinteger(L, c);
    return 1;
}

static int Cungetch(lua_State *L)
{
    int c = luaL_checkint(L, 1);
    lua_pushboolean(L, B(ungetch(c)));
    return 1;
}

/*
** =======================================================
** getstr
** =======================================================
*/

static int Wgetstr(lua_State *L)
{
    WINDOW *w = checkwin(L, 1);
    int n = luaL_optint(L, 2, 0);
    char buf[LUAL_BUFFERSIZE];

    if (n == 0 || n >= LUAL_BUFFERSIZE) n = LUAL_BUFFERSIZE - 1;
    if (wgetnstr(w, buf, n) == ERR)
        return 0;

    lua_pushstring(L, buf);
    return 1;
}

static int Wmvgetstr(lua_State *L)
{
    WINDOW *w = checkwin(L, 1);
    int y = luaL_checkint(L, 2);
    int x = luaL_checkint(L, 3);
    int n = luaL_optint(L, 4, -1);
    char buf[LUAL_BUFFERSIZE];

    if (n == 0 || n >= LUAL_BUFFERSIZE) n = LUAL_BUFFERSIZE - 1;
    if (mvwgetnstr(w, y, x, buf, n) == ERR)
        return 0;

    lua_pushstring(L, buf);
    return 1;
}

/*
** =======================================================
** inch
** =======================================================
*/

static int Wwinch(lua_State *L)
{
    WINDOW *w = checkwin(L, 1);
    lua_pushinteger(L, winch(w));
    return 1;
}

static int Wmvwinch(lua_State *L)
{
    WINDOW *w = checkwin(L, 1);
    int y = luaL_checkint(L, 2);
    int x = luaL_checkint(L, 3);
    lua_pushinteger(L, mvwinch(w, y, x));
    return 1;
}

/*
** =======================================================
** inchstr
** =======================================================
*/

static int Wwinchnstr(lua_State *L)
{
    WINDOW *w = checkwin(L, 1);
    int n = luaL_checkint(L, 2);
    chstr *cs = chstr_new(L, n);

    if (winchnstr(w, cs->str, n) == ERR)
        return 0;

    return 1;
}

static int Wmvwinchnstr(lua_State *L)
{
    WINDOW *w = checkwin(L, 1);
    int y = luaL_checkint(L, 2);
    int x = luaL_checkint(L, 3);
    int n = luaL_checkint(L, 4);
    chstr *cs = chstr_new(L, n);

    if (mvwinchnstr(w, y, x, cs->str, n) == ERR)
        return 0;

    return 1;
}

/*
** =======================================================
** instr
** =======================================================
*/

static int Wwinnstr(lua_State *L)
{
    WINDOW *w = checkwin(L, 1);
    int n = luaL_checkint(L, 2);
    char buf[LUAL_BUFFERSIZE];

    if (n >= LUAL_BUFFERSIZE) n = LUAL_BUFFERSIZE - 1;
    if (winnstr(w, buf, n) == ERR)
        return 0;

    lua_pushlstring(L, buf, n);
    return 1;
}

static int Wmvwinnstr(lua_State *L)
{
    WINDOW *w = checkwin(L, 1);
    int y = luaL_checkint(L, 2);
    int x = luaL_checkint(L, 3);
    int n = luaL_checkint(L, 4);
    char buf[LUAL_BUFFERSIZE];

    if (n >= LUAL_BUFFERSIZE) n = LUAL_BUFFERSIZE - 1;
    if (mvwinnstr(w, y, x, buf, n) == ERR)
        return 0;

    lua_pushlstring(L, buf, n);
    return 1;
}

/*
** =======================================================
** insch
** =======================================================
*/

static int Wwinsch(lua_State *L)
{
    WINDOW *w = checkwin(L, 1);
    chtype ch = checkch(L, 2);
    lua_pushboolean(L, B(winsch(w, ch)));
    return 1;
}

static int Wmvwinsch(lua_State *L)
{
    WINDOW *w = checkwin(L, 1);
    int y = luaL_checkint(L, 2);
    int x = luaL_checkint(L, 3);
    chtype ch = checkch(L, 4);
    lua_pushboolean(L, B(mvwinsch(w, y, x, ch)));
    return 1;
}

/*
** =======================================================
** insstr
** =======================================================
*/

static int Wwinsstr(lua_State *L)
{
    WINDOW *w = checkwin(L, 1);
    const char *str = luaL_checkstring(L, 2);
    lua_pushboolean(L, B(winsnstr(w, str, lua_strlen(L, 2))));
    return 1;
}

static int Wmvwinsstr(lua_State *L)
{
    WINDOW *w = checkwin(L, 1);
    int y = luaL_checkint(L, 2);
    int x = luaL_checkint(L, 3);
    const char *str = luaL_checkstring(L, 4);
    lua_pushboolean(L, B(mvwinsnstr(w, y, x, str, lua_strlen(L, 2))));
    return 1;
}

static int Wwinsnstr(lua_State *L)
{
    WINDOW *w = checkwin(L, 1);
    const char *str = luaL_checkstring(L, 2);
    int n = luaL_checkint(L, 3);
    lua_pushboolean(L, B(winsnstr(w, str, n)));
    return 1;
}

static int Wmvwinsnstr(lua_State *L)
{
    WINDOW *w = checkwin(L, 1);
    int y = luaL_checkint(L, 2);
    int x = luaL_checkint(L, 3);
    const char *str = luaL_checkstring(L, 4);
    int n = luaL_checkint(L, 5);
    lua_pushboolean(L, B(mvwinsnstr(w, y, x, str, n)));
    return 1;
}

/*
** =======================================================
** pad
** =======================================================
*/

static int Cnewpad(lua_State *L)
{
    int nlines = luaL_checkint(L, 1);
    int ncols = luaL_checkint(L, 2);
    lc_newwin(L, newpad(nlines, ncols));
    return 1;
}

static int Wsubpad(lua_State *L)
{
    WINDOW *orig = checkwin(L, 1);
    int nlines  = luaL_checkint(L, 2);
    int ncols   = luaL_checkint(L, 3);
    int begin_y = luaL_checkint(L, 4);
    int begin_x = luaL_checkint(L, 5);

    lc_newwin(L, subpad(orig, nlines, ncols, begin_y, begin_x));
    return 1;
}

static int Wprefresh(lua_State *L)
{
    WINDOW *p = checkwin(L, 1);
    int pminrow = luaL_checkint(L, 2);
    int pmincol = luaL_checkint(L, 3);
    int sminrow = luaL_checkint(L, 4);
    int smincol = luaL_checkint(L, 5);
    int smaxrow = luaL_checkint(L, 6);
    int smaxcol = luaL_checkint(L, 7);

    lua_pushboolean(L, B(prefresh(p, pminrow, pmincol,
        sminrow, smincol, smaxrow, smaxcol)));
    return 1;
}

static int Wpnoutrefresh(lua_State *L)
{
    WINDOW *p = checkwin(L, 1);
    int pminrow = luaL_checkint(L, 2);
    int pmincol = luaL_checkint(L, 3);
    int sminrow = luaL_checkint(L, 4);
    int smincol = luaL_checkint(L, 5);
    int smaxrow = luaL_checkint(L, 6);
    int smaxcol = luaL_checkint(L, 7);

    lua_pushboolean(L, B(pnoutrefresh(p, pminrow, pmincol,
        sminrow, smincol, smaxrow, smaxcol)));
    return 1;
}


static int Wpechochar(lua_State *L)
{
    WINDOW *p = checkwin(L, 1);
    chtype ch = checkch(L, 2);

    lua_pushboolean(L, B(pechochar(p, ch)));
    return 1;
}

/*
** =======================================================
** attr
** =======================================================
*/

#define LCW_WININTOK(v)                         \
    static int W ## v(lua_State *L)             \
    {                                           \
        WINDOW *w = checkwin(L, 1);             \
        int bf = luaL_checkint(L, 2);           \
        lua_pushboolean(L, B(w ## v(w, bf)));   \
        return 1;                               \
    }

LCW_WININTOK(attroff)
LCW_WININTOK(attron)
LCW_WININTOK(attrset)

LCW_BOOLOK2(Wstandend, wstandend)
LCW_BOOLOK2(Wstandout, wstandout)


/*
** =======================================================
** query terminfo database
** =======================================================
*/

static char ti_capname[32];

static int Ctigetflag (lua_State *L)
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

static int Ctigetnum (lua_State *L)
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

static int Ctigetstr (lua_State *L)
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


/*
** =======================================================
** register functions
** =======================================================
*/
/* chstr members */
static const luaL_Reg chstrlib[] =
{
#define MENTRY(_f) { LCURSES_STR(_f), LCURSES_SPLICE(chstr_, _f) }
    MENTRY( len		),
    MENTRY( set_ch	),
    MENTRY( set_str	),
    MENTRY( get		),
    MENTRY( dup		),
#undef MENTRY

    { NULL, NULL }
};

static const luaL_Reg windowlib[] =
{
#define MENTRY(_f) { LCURSES_STR_1(_f), (_f) }
    /* window */
    MENTRY( Wclose		),
    MENTRY( Wsub		),
    MENTRY( Wderive		),
    MENTRY( Wmove_window	),
    MENTRY( Wmove_derived	),
    MENTRY( Wclone		),
    MENTRY( Wsyncup		),
    MENTRY( Wsyncdown		),
    MENTRY( Wsyncok		),
    MENTRY( Wcursyncup		),

    /* inopts */
    MENTRY( Wintrflush	),
    MENTRY( Wkeypad	),
    MENTRY( Wmeta	),
    MENTRY( Wnodelay	),
    MENTRY( Wtimeout	),
    MENTRY( Wnotimeout	),

    /* outopts */
    MENTRY( Wclearok	),
    MENTRY( Widlok	),
    MENTRY( Wleaveok	),
    MENTRY( Wscrollok	),
    MENTRY( Widcok	),
    MENTRY( Wimmedok	),
    MENTRY( Wwsetscrreg	),

    /* pad */
    MENTRY( Wsubpad		),
    MENTRY( Wprefresh		),
    MENTRY( Wpnoutrefresh	),
    MENTRY( Wpechochar		),

    /* move */
    MENTRY( Wmove	),

    /* scroll */
    MENTRY( Wscrl	),

    /* refresh */
    MENTRY( Wrefresh		),
    MENTRY( Wnoutrefresh	),
    MENTRY( Wredrawwin		),
    MENTRY( Wredrawln		),

    /* clear */
    MENTRY( Werase	),
    MENTRY( Wclear	),
    MENTRY( Wclrtobot	),
    MENTRY( Wclrtoeol	),

    /* touch */
    MENTRY( Wtouch		),
    MENTRY( Wtouchline		),
    MENTRY( Wis_linetouched	),
    MENTRY( Wis_wintouched	),

    /* attrs */
    MENTRY( Wattroff	),
    MENTRY( Wattron	),
    MENTRY( Wattrset	),
    MENTRY( Wstandout	),
    MENTRY( Wstandend	),

    /* getch */
    MENTRY( Wgetch	),
    MENTRY( Wmvgetch	),

    /* getyx */
    MENTRY( Wgetyx	),
    MENTRY( Wgetparyx	),
    MENTRY( Wgetbegyx	),
    MENTRY( Wgetmaxyx	),

    /* border */
    MENTRY( Wborder	),
    MENTRY( Wbox	),
    MENTRY( Whline	),
    MENTRY( Wvline	),
    MENTRY( Wmvhline	),
    MENTRY( Wmvvline	),

    /* addch */
    MENTRY( Waddch	),
    MENTRY( Wmvaddch	),
    MENTRY( Wechoch	),

    /* addchstr */
    MENTRY( Waddchstr	),
    MENTRY( Wmvaddchstr	),

    /* addstr */
    MENTRY( Waddstr	),
    MENTRY( Wmvaddstr	),

    /* bkgd */
    MENTRY( Wwbkgdset	),
    MENTRY( Wwbkgd	),
    MENTRY( Wgetbkgd	),

    /* overlay */
    MENTRY( Woverlay	),
    MENTRY( Woverwrite	),
    MENTRY( Wcopywin	),

    /* delch */
    MENTRY( Wdelch	),
    MENTRY( Wmvdelch	),

    /* deleteln */
    MENTRY( Wdeleteln	),
    MENTRY( Winsertln	),
    MENTRY( Wwinsdelln	),

    /* getstr */
    MENTRY( Wgetstr	),
    MENTRY( Wmvgetstr	),

    /* inch */
    MENTRY( Wwinch		),
    MENTRY( Wmvwinch		),
    MENTRY( Wwinchnstr		),
    MENTRY( Wmvwinchnstr	),

    /* instr */
    MENTRY( Wwinnstr	),
    MENTRY( Wmvwinnstr	),

    /* insch */
    MENTRY( Wwinsch	),
    MENTRY( Wmvwinsch	),

    /* insstr */
    MENTRY( Wwinsstr	),
    MENTRY( Wwinsnstr	),
    MENTRY( Wmvwinsstr	),
    MENTRY( Wmvwinsnstr	),

    /* misc */
    MENTRY( W__tostring	),
#undef MENTRY
    {"__gc",        Wclose  }, /* rough safety net */

    {NULL, NULL}
};

static const luaL_Reg curseslib[] =
{
#define MENTRY(_f) { LCURSES_STR_1(_f), (_f) }
    /* chstr helper function */
    MENTRY( Cnew_chstr	),

    /* initscr */
    MENTRY( Cendwin	),
    MENTRY( Cisendwin	),
    MENTRY( Cstdscr	),
    MENTRY( Ccols	),
    MENTRY( Clines	),

    /* color */
    MENTRY( Cstart_color	),
    MENTRY( Chas_colors		),
    MENTRY( Cuse_default_colors	),
    MENTRY( Cinit_pair		),
    MENTRY( Cpair_content	),
    MENTRY( Ccolors		),
    MENTRY( Ccolor_pairs	),
    MENTRY( Ccolor_pair		),

    /* termattrs */
    MENTRY( Cbaudrate	),
    MENTRY( Cerasechar	),
    MENTRY( Ckillchar	),
    MENTRY( Chas_ic	),
    MENTRY( Chas_il	),
    MENTRY( Ctermattrs	),
    MENTRY( Ctermname	),
    MENTRY( Clongname	),

    /* kernel */
    MENTRY( Cripoffline	),
    MENTRY( Cnapms	),
    MENTRY( Ccurs_set	),

    /* resize */
    MENTRY( Cresizeterm	),

    /* beep */
    MENTRY( Cbeep	),
    MENTRY( Cflash	),

    /* window */
    MENTRY( Cnewwin	),

    /* pad */
    MENTRY( Cnewpad	),

    /* refresh */
    MENTRY( Cdoupdate	),

    /* inopts */
    MENTRY( Ccbreak	),
    MENTRY( Cecho	),
    MENTRY( Craw	),
    MENTRY( Chalfdelay	),

    /* util */
    MENTRY( Cunctrl		),
    MENTRY( Ckeyname		),
    MENTRY( Cdelay_output	),
    MENTRY( Cflushinp		),

    /* getch */
    MENTRY( Cungetch	),

    /* outopts */
    MENTRY( Cnl	),

    /* query terminfo database */
    MENTRY( Ctigetflag	),
    MENTRY( Ctigetnum	),
    MENTRY( Ctigetstr	),

    /* slk */
    MENTRY( Cslk_init		),
    MENTRY( Cslk_set		),
    MENTRY( Cslk_refresh	),
    MENTRY( Cslk_noutrefresh	),
    MENTRY( Cslk_label		),
    MENTRY( Cslk_clear		),
    MENTRY( Cslk_restore	),
    MENTRY( Cslk_touch		),
    MENTRY( Cslk_attron		),
    MENTRY( Cslk_attroff	),
    MENTRY( Cslk_attrset	),
#undef MENTRY

    /* terminator */
    {NULL, NULL}
};


/* Prototype to keep compiler happy. */
LUALIB_API int luaopen_curses_c (lua_State *L);

int luaopen_curses_c (lua_State *L)
{
    /*
    ** create new metatable for window objects
    */
    luaL_newmetatable(L, WINDOWMETA);
    lua_pushliteral(L, "__index");
    lua_pushvalue(L, -2);               /* push metatable */
    lua_rawset(L, -3);                  /* metatable.__index = metatable */
    luaL_openlib(L, NULL, windowlib, 0);

    lua_pop(L, 1);                      /* remove metatable from stack */

    /*
    ** create new metatable for chstr objects
    */
    luaL_newmetatable(L, CHSTRMETA);
    lua_pushliteral(L, "__index");
    lua_pushvalue(L, -2);               /* push metatable */
    lua_rawset(L, -3);                  /* metatable.__index = metatable */
    luaL_openlib(L, NULL, chstrlib, 0);

    lua_pop(L, 1);                      /* remove metatable from stack */

    /*
    ** create global table with curses methods/variables/constants
    */
    luaL_register(L, "curses", curseslib);

    lua_pushstring(L, "initscr");
    lua_pushvalue(L, -2);
    lua_pushcclosure(L, Cinitscr, 1);
    lua_settable(L, -3);

    return 1;
}

/* Local Variables: */
/* c-basic-offset: 4 */
/* End:             */
