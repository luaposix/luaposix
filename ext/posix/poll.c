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
 Synchronous I/O Multiplexing.

 Examine file descriptors for events, such as readyness for I/O.

@module posix.poll
*/

#include <poll.h>

#include "_helpers.c"


static struct {
	short       bit;
	const char *name;
} poll_event_map[] = {
#define MAP(_NAME) \
	{POLL##_NAME, #_NAME}
	MAP(IN),
	MAP(PRI),
	MAP(OUT),
	MAP(ERR),
	MAP(HUP),
	MAP(NVAL),
#undef MAP
};

#define PPOLL_EVENT_NUM (sizeof(poll_event_map) / sizeof(*poll_event_map))

static void
poll_events_createtable(lua_State *L)
{
	lua_createtable(L, 0, PPOLL_EVENT_NUM);
}

static short
poll_events_from_table(lua_State *L, int table)
{
	short events = 0;
	size_t i;

	/* Convert to absolute index */
	if (table < 0)
		table = lua_gettop(L) + table + 1;

	for (i = 0; i < PPOLL_EVENT_NUM; i++)
	{
		lua_getfield(L, table, poll_event_map[i].name);
		if (lua_toboolean(L, -1))
			events |= poll_event_map[i].bit;
		lua_pop(L, 1);
	}

	return events;
}

static void
poll_events_to_table(lua_State *L, int table, short events)
{
	size_t i;

	/* Convert to absolute index */
	if (table < 0)
		table = lua_gettop(L) + table + 1;

	for (i = 0; i < PPOLL_EVENT_NUM; i++)
	{
		lua_pushboolean(L, events & poll_event_map[i].bit);
		lua_setfield(L, table, poll_event_map[i].name);
	}
}

static nfds_t
poll_fd_list_check_table(lua_State *L, int table)
{
	nfds_t fd_num = 0;

	/*
	 * Assume table is an argument number.
	 * Should be an assert(table > 0).
	 */

	luaL_checktype(L, table, LUA_TTABLE);

	/* Nil key - the one before first */
	lua_pushnil(L);

	/* Push each key/value pair, popping previous key */
	while (lua_next(L, 1) != 0)
	{
		/* Verify the fd key */
		luaL_argcheck(L, lua_isinteger(L, -2), table,
					  "contains non-integer key(s)");

		/* Verify the table value */
		luaL_argcheck(L, lua_istable(L, -1), table,
					  "contains non-table value(s)");
		lua_getfield(L, -1, "events");
		luaL_argcheck(L, lua_istable(L, -1), table,
					  "contains invalid value table(s)");
		lua_pop(L, 1);
		lua_getfield(L, -1, "revents");
		luaL_argcheck(L, lua_isnil(L, -1) || lua_istable(L, -1), table,
					  "contains invalid value table(s)");
		lua_pop(L, 1);

		/* Remove value (but leave the key) */
		lua_pop(L, 1);

		/* Count the fds */
		fd_num++;
	}

	return fd_num;
}

static void
poll_fd_list_from_table(lua_State *L, int table, struct pollfd *fd_list)
{
	struct pollfd *pollfd = fd_list;

	/*
	 * Assume the table didn't change since
	 * the call to poll_fd_list_check_table
	 */

	/* Convert to absolute index */
	if (table < 0)
		table = lua_gettop(L) + table + 1;

	/* Nil key - the one before first */
	lua_pushnil(L);

	/* Push each key/value pair, popping previous key */
	while (lua_next(L, table) != 0)
	{
		/* Transfer the fd key */
		pollfd->fd = lua_tointeger(L, -2);

		/* Transfer "events" field from the value */
		lua_getfield(L, -1, "events");
		pollfd->events = poll_events_from_table(L, -1);
		lua_pop(L, 1);

		/* Remove value (but leave the key) */
		lua_pop(L, 1);

		/* Proceed to next fd */
		pollfd++;
	}
}

static void
poll_fd_list_to_table(lua_State *L, int table, const struct pollfd *fd_list)
{
	const struct pollfd *pollfd = fd_list;

	/*
	 * Assume the table didn't change since
	 * the call to poll_fd_list_check_table.
	 */

	/* Convert to absolute index */
	if (table < 0)
		table = lua_gettop(L) + table + 1;

	/* Nil key - the one before first */
	lua_pushnil(L);

	/* Push each key/value pair, popping previous key */
	while (lua_next(L, 1) != 0)
	{
		/* Transfer "revents" field to the value */
		lua_getfield(L, -1, "revents");
		if (lua_isnil(L, -1))
		{
			lua_pop(L, 1);
			poll_events_createtable(L);
			lua_pushvalue(L, -1);
			lua_setfield(L, -3, "revents");
		}
		poll_events_to_table(L, -1, pollfd->revents);
		lua_pop(L, 1);

		/* Remove value (but leave the key) */
		lua_pop(L, 1);

		/* Proceed to next fd */
		pollfd++;
	}
}


/***
Wait for events on multiple file descriptors.
@function poll
@tparam table fds list of file descriptors
@int[opt=-1] timeout maximum timeout in milliseconds, or -1 to block indefinitely
@treturn[1] int `0` if timed out, number of *fd*'s ready if successful
@return[2] nil
@treturn[2] string error message
@treturn[2] int errnum
@see poll(2)
@see rpoll
@see poll.lua
*/
static int
Ppoll(lua_State *L)
{
	struct pollfd *fd_list, static_fd_list[16];
	nfds_t fd_num = poll_fd_list_check_table(L, 1);
	int r, timeout = optint(L, 2, -1);
	checknargs(L, 2);

	fd_list = (fd_num <= sizeof(static_fd_list) / sizeof(*static_fd_list))
					? static_fd_list
					: lua_newuserdata(L, sizeof(*fd_list) * fd_num);


	poll_fd_list_from_table(L, 1, fd_list);

	r = poll(fd_list, fd_num, timeout);

	/* If any of the descriptors changed state */
	if (r > 0)
		poll_fd_list_to_table(L, 1, fd_list);

	return pushresult(L, r, NULL);
}


/***
Wait for some event on a file descriptor.
Based on [a lua-l list post](http://lua-users.org/lists/lua-l/2007-11/msg00346.html).
@function rpoll
@int fd file descriptor
@int[opt=-1] timeout maximum timeout in milliseconds, or `-1` to block indefinitely
@treturn[1] int `0` if timed out, number of *fd*'s ready if successful
@return[2] nil
@return[2] error message
@treturn[2] int errnum
@see poll
@usage
fh = io.open "one"
while true do
  r = rpoll (fh, 500)
  if r == 0 then
    print 'timeout'
  elseif r == 1 then
    print (fh:read ())
  else
    print "finish!"
    break
  end
end
*/
static int
Prpoll(lua_State *L)
{
	struct pollfd fds;
	int file = checkint(L, 1);
	int timeout = checkint(L, 2);
	checknargs(L, 2);
	fds.fd = file;
	fds.events = POLLIN;
	return pushresult(L, poll(&fds, 1, timeout), NULL);
}


static const luaL_Reg posix_poll_fns[] =
{
	LPOSIX_FUNC( Ppoll		),
	LPOSIX_FUNC( Prpoll		),
	{NULL, NULL}
};


LUALIB_API int
luaopen_posix_poll(lua_State *L)
{
	luaL_newlib(L, posix_poll_fns);
	lua_pushstring(L, LPOSIX_VERSION_STRING("poll"));
	lua_setfield(L, -2, "version");

	return 1;
}
