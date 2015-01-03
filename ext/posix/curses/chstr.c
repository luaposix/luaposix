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
Curses attributed string buffers.

An array of characters, plus associated curses attributes and
colors at each position.

Although marginally useful alone, the constants used to set colors
and attributes in `chstr` buffers are not defined until **after**
`curses.initscr ()` has been called.

@classmod posix.curses.chstr
*/

#ifndef LUAPOSIX_CURSES_CHSTR_C
#define LUAPOSIX_CURSES_CHSTR_C 1

#include <config.h>

#if HAVE_CURSES

#include "_helpers.c"


static const char *CHSTRMETA = "posix.curses:chstr";


typedef struct
{
	unsigned int len;
	chtype str[1];
} chstr;
#define CHSTR_SIZE(len) (sizeof(chstr) + len * sizeof(chtype))


/* create new chstr object and leave it in the lua stack */
static chstr *
chstr_new(lua_State *L, int len)
{
	chstr *cs;

	if (len < 1)
		return luaL_error(L, "invalid chstr length"), NULL;

	cs = lua_newuserdata(L, CHSTR_SIZE(len));
	luaL_getmetatable(L, CHSTRMETA);
	lua_setmetatable(L, -2);
	cs->len = len;
	return cs;
}


/* get chstr from lua (convert if needed) */
static chstr *
checkchstr(lua_State *L, int narg)
{
	chstr *cs = (chstr*)luaL_checkudata(L, narg, CHSTRMETA);
	if (cs)
		return cs;

	luaL_argerror(L, narg, "bad curses chstr");

	/*NOTREACHED*/
	return NULL;
}


/***
Change the contents of the chstr.
@function set_str
@int o offset to start of change
@string s characters to insert into *cs* at *o*
@int[opt=A_NORMAL] attr attributes for changed elements
@int[opt=1] rep repeat count
@usage
  cs = curses.chstr (10)
  cs:set_str(0, "0123456789", curses.A_BOLD)
*/
static int
Cset_str(lua_State *L)
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


/***
Set a character in the buffer.
*ch* can be a one-character string, or an integer from `string.byte`
@function set_ch
@int o offset to start of change
@param int|string ch character to insert
@int[opt=A_NORMAL] attr attributes for changed elements
@int[opt=1] rep repeat count
@usage
  -- Write a bold 'A' followed by normal 'a' chars to a new buffer
  size = 10
  cs = curses.chstr (size)
  cs:set_ch(0, 'A', curses.A_BOLD)
  cs:set_ch(1, 'a', curses.A_NORMAL, size - 1)
*/
static int
Cset_ch(lua_State *L)
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


/***
Get information from the chstr.
@function get
@int o offset from start of *cs*
@treturn int character at offset *o* in *cs*
@treturn int bitwise-OR of attributes at offset *o* in *cs*
@treturn int colorpair at offset *o* in *cs*
@usage
  cs = curses.chstr (10)
  cs:set_ch(0, 'A', curses.A_BOLD, 10)
  --> 65 2097152 0
  print (cs:get (9))
*/
static int
Cget(lua_State *L)
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


/***
Retrieve chstr length.
@function len
@tparam chstr cs buffer to act on
@treturn int length of *cs*
@usage
  cs = curses.chstr (123)
  --> 123
  print (cs:len ())
*/
static int
Clen(lua_State *L)
{
	chstr *cs = checkchstr(L, 1);
	return pushintresult(cs->len);
}


/***
Duplicate chstr.
@function dup
@treturn chstr duplicate of *cs*
@usage
  dup = cs:dup ()
*/
static int
Cdup(lua_State *L)
{
	chstr *cs = checkchstr(L, 1);
	chstr *ncs = chstr_new(L, cs->len);

	memcpy(ncs->str, cs->str, CHSTR_SIZE(cs->len));
	return 1;
}


/***
Initialise a new chstr.
@function __call
@int len buffer length
@treturn chstr a new chstr filled with spaces
@usage
  cs = curses.chstr (10)
*/
static int
C__call(lua_State *L)
{
	int len = checkint(L, 2);
	chstr* ncs = chstr_new(L, len);
	memset(ncs->str, ' ', len * sizeof(chtype));
	return 1;
}
#endif /*!HAVE_CURSES*/


static const luaL_Reg posix_curses_chstr_fns[] =
{
#if HAVE_CURSES
	LPOSIX_FUNC( Clen		),
	LPOSIX_FUNC( Cset_ch		),
	LPOSIX_FUNC( Cset_str		),
	LPOSIX_FUNC( Cget		),
	LPOSIX_FUNC( Cdup		),
#endif
	{ NULL, NULL }
};



LUALIB_API int
luaopen_posix_curses_chstr(lua_State *L)
{
	int t, mt;

	luaL_register(L, "posix.curses.chstr", posix_curses_chstr_fns);
	t = lua_gettop(L);

#if HAVE_CURSES
	lua_createtable(L, 0, 1);		/* u = {} */
	lua_pushcfunction(L, C__call);
	lua_setfield(L, -2, "__call");		/* u.__call = C__call */
	lua_setmetatable(L, -2);		/* setmetatable (t, u) */

	luaL_newmetatable(L, CHSTRMETA);
	mt = lua_gettop(L);

	lua_pushvalue(L, mt);
	lua_setfield(L, -2, "__index");		/* mt.__index = mt */
	lua_pushliteral(L, "PosixChstr");
	lua_setfield(L, -2, "_type");		/* mt._type = "PosixChstr" */

	/* for k,v in pairs(t) do mt[k]=v end */
	for (lua_pushnil(L); lua_next(L, t) != 0;)
		lua_setfield(L, mt, lua_tostring(L, -2));

	lua_pop(L, 1);				/* pop mt */
#endif

	/* t.version = "posix.curses.chstr..." */
	lua_pushliteral(L, "posix.curses.chstr for " LUA_VERSION " / " PACKAGE_STRING);
	lua_setfield(L, t, "version");

	return 1;
}

#endif /*!LUAPOSIX_CURSES_CHSTR_C*/
