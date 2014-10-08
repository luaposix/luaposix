/***
@module posix.sys.socket
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

#include <unistd.h>	/* for _POSIX_VERSION */

#if _POSIX_VERSION >= 200112L
#include <arpa/inet.h>
#if HAVE_LINUX_NETLINK_H
#include <linux/netlink.h>
#endif
#ifdef HAVE_NET_IF_H
#include <net/if.h>
#endif
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>
#endif

#include "_helpers.c"


#if _POSIX_VERSION >= 200112L

/***
Socket address.
@table sockaddr
@int socktype one of `SOCK_STREAM`, `SOCK_DGRAM` or `SOCK_RAW`
@string canonname canonical name for service location
@int protocol one of `IPPROTO_TCP` or `IPPROTO_UDP`
*/


/***
Address information hints.
@table addrinfo
@int family one of `AF_INET`, `AF_INET6`, `AF_UNIX` or `AF_NETLINK`
@int flags bitwise OR of zero or more of `AI_ADDRCONFIG`, `AI_ALL`,
  `AI_CANONNAME`, `AI_NUMERICHOST`, `AI_NUMERICSERV`, `AI_PASSIVE` and
  `AI_V4MAPPED`
@int socktype one of `SOCK_STREAM`, `SOCK_DGRAM` or `SOCK_RAW`
@int protocol one of `IPPROTO_TCP` or `IPPROTO_UDP`
*/


/* strlcpy() implementation for non-BSD based Unices.
   strlcpy() is a safer less error-prone replacement for strncpy(). */
#include "strlcpy.c"


/***
Create an endpoint for communication.
@function socket
@int domain one of `AF_INET`, `AF_INET6`, `AF_UNIX` or `AF_NETLINK`
@int type one of `SOCK_STREAM`, `SOCK_DGRAM` or `SOCK_RAW`
@int options usually 0, but some socket types might implement other protocols.
@treturn[1] int socket descriptor, if successful
@return[2] nil
@treturn[2] string error message
@see socket(2)
@usage sockd = P.socket (P.AF_INET, P.SOCK_STREAM, 0)
*/
static int
Psocket(lua_State *L)
{
	int domain = checkint(L, 1);
	int type = checkint(L, 2);
	int options = checkint(L, 3);
	checknargs(L, 3);
	return pushresult(L, socket(domain, type, options), NULL);
}


/***
Create a pair of connected sockets.
@function socketpair
@int domain one of `AF_INET`, `AF_INET6`, `AF_UNIX` or `AF_NETLINK`
@int socktype one of `SOCK_STREAM`, `SOCK_DGRAM` or `SOCK_RAW`
@int options usually 0, but some socket types might implement other protocols.
@treturn[1] int descriptor of one end of the socket pair
@treturn[1] int descriptor of the other end of the pair, if successful
@return[2] nil
@treturn[2] string error message
@usage sockr, sockw = P.socketpair (P.AF_INET, P.SOCK_STREAM, 0)
*/
static int
Psocketpair(lua_State *L)
{
	int domain = checkint(L, 1);
	int socktype = checkint(L, 2);
	int options = checkint(L, 3);
	int fd[2];
	int rc;
	checknargs(L, 3);
	if ((rc = socketpair(domain, socktype, options, fd)) < 0)
		return pusherror(L, "socketpair");
	lua_pushinteger(L, fd[0]);
	lua_pushinteger(L, fd[1]);
	return 2;
}


/* Push a new lua table populated with the fields describing the passed sockaddr */
static int
sockaddr_to_lua(lua_State *L, int family, struct sockaddr *sa)
{
	char addr[INET6_ADDRSTRLEN];
	int port;
	struct sockaddr_in *sa4;
	struct sockaddr_in6 *sa6;
	struct sockaddr_un *sau;
#if HAVE_LINUX_NETLINK_H
	struct sockaddr_nl *san;
#endif

	lua_newtable(L);
	lua_pushnumber(L, family); lua_setfield(L, -2, "family");

	switch (family)
	{
		case AF_INET:
			sa4 = (struct sockaddr_in *)sa;
			inet_ntop(family, &sa4->sin_addr, addr, sizeof addr);
			port = ntohs(sa4->sin_port);
			lua_pushnumber(L, port); lua_setfield(L, -2, "port");
			lua_pushstring(L, addr); lua_setfield(L, -2, "addr");
			break;
		case AF_INET6:
			sa6 = (struct sockaddr_in6 *)sa;
			inet_ntop(family, &sa6->sin6_addr, addr, sizeof addr);
			port = ntohs(sa6->sin6_port);
			lua_pushnumber(L, port); lua_setfield(L, -2, "port");
			lua_pushstring(L, addr); lua_setfield(L, -2, "addr");
			break;
		case AF_UNIX:
			sau = (struct sockaddr_un *)sa;
			lua_pushstring(L, sau->sun_path); lua_setfield(L, -2, "path");
			break;
#if HAVE_LINUX_NETLINK_H
		case AF_NETLINK:
			san = (struct sockaddr_nl *)sa;
			lua_pushnumber(L, san->nl_pid); lua_setfield(L, -2, "pid");
			lua_pushnumber(L, san->nl_groups); lua_setfield(L, -2, "groups");
			break;
#endif
	}

	return 1;
}


static const char *Safinet_fields[] = { "family", "port", "addr" };
static const char *Safunix_fields[] = { "family", "path" };
static const char *Safnetlink_fields[] = { "family", "pid", "groups" };


/* Populate a sockaddr_storage with the info from the given lua table */
static int
sockaddr_from_lua(lua_State *L, int index, struct sockaddr_storage *sa, socklen_t *addrlen)
{
	int family, r = -1;

	luaL_checktype(L, index, LUA_TTABLE);
	family = checkintfield(L, index, "family");

	memset(sa, 0, sizeof *sa);

	switch (family)
	{
		case AF_INET:
		{
			struct sockaddr_in *sa4 = (struct sockaddr_in *)sa;
			int port		= checkintfield(L, index, "port");
			const char *addr	= checkstringfield(L, index, "addr");

			checkfieldnames (L, index, Safinet_fields);

			if (inet_pton(AF_INET, addr, &sa4->sin_addr) == 1)
			{
				sa4->sin_family	= family;
				sa4->sin_port	= htons(port);
				*addrlen	= sizeof(*sa4);
				r		= 0;
			}
			break;
		}
		case AF_INET6:
		{
			struct sockaddr_in6 *sa6= (struct sockaddr_in6 *)sa;
			int port		= checkintfield(L, index, "port");
			const char *addr	= checkstringfield(L, index, "addr");

			checkfieldnames (L, index, Safinet_fields);

			if (inet_pton(AF_INET6, addr, &sa6->sin6_addr) == 1)
			{
				sa6->sin6_family= family;
				sa6->sin6_port	= htons(port);
				*addrlen	= sizeof(*sa6);
				r		= 0;
			}
			break;
		}
		case AF_UNIX:
		{
			struct sockaddr_un *sau	= (struct sockaddr_un *)sa;
			const char *path	= checkstringfield(L, index, "path");

			checkfieldnames (L, index, Safunix_fields);

			sau->sun_family	= family;
			strlcpy(sau->sun_path, path, sizeof(sau->sun_path));
			sau->sun_path[sizeof(sau->sun_path) - 1]= '\0';
			*addrlen	= sizeof(*sau);
			r		= 0;
			break;
		}
#if HAVE_LINUX_NETLINK_H
		case AF_NETLINK:
		{
			struct sockaddr_nl *san	= (struct sockaddr_nl *)sa;
			san->nl_family	= family;
			san->nl_pid	= checkintfield(L, index, "pid");
			san->nl_groups	= checkintfield(L, index, "groups");
			*addrlen	= sizeof(*san);

			checkfieldnames (L, index, Safnetlink_fields);

			r		= 0;
			break;
		}
#endif
		default:
			lua_pushfstring(L, "unsupported family type %d", family);
			luaL_argcheck(L, 0, index, lua_tostring (L, -1));
			lua_pop (L, 1);
			break;

	}
	return r;
}


static const char *Sai_fields[] = { "family", "socktype", "protocol", "flags" };


/***
Network address and service translation.
@function getaddrinfo
@string host name of a host
@string service name of service
@tparam[opt] addrinfo hints table
@treturn[1] list of @{sockaddr} tables
@return[2] nil
@treturn[2] string error message
@treturn[2] int error code if failed
@see getaddrinfo(2)
@usage
local res, errmsg, errcode = posix.getaddrinfo ("www.lua.org", "http",
  { family = P.IF_INET, socktype = P.SOCK_STREAM }
)
*/
static int
Pgetaddrinfo(lua_State *L)
{
	int n = 1;
	const char *host = optstring(L, 1, NULL);
	const char *service = NULL;
	struct addrinfo *res, hints;
	hints.ai_family = PF_UNSPEC;

	checknargs(L, 3);

	switch (lua_type(L, 2))
	{
		case LUA_TNONE:
		case LUA_TNIL:
			if (host == NULL)
				argtypeerror(L, 2, "string or int");
			break;
		case LUA_TNUMBER:
		case LUA_TSTRING:
			service = lua_tostring(L, 2);
			break;
		default:
			argtypeerror(L, 2, "string, int or nil");
			break;
	}

	switch (lua_type(L, 3))
	{
		case LUA_TNONE:
		case LUA_TNIL:
			break;
		case LUA_TTABLE:
			checkfieldnames (L, 3, Sai_fields);
			hints.ai_family   = optintfield(L, 3, "family", PF_UNSPEC);
			hints.ai_socktype = optintfield(L, 3, "socktype", 0);
			hints.ai_protocol = optintfield(L, 3, "protocol", 0);
			hints.ai_flags    = optintfield(L, 3, "flags", 0);
			break;
		default:
			argtypeerror(L, 3, "table or nil");
			break;
	}

	{
		int r;
		if ((r = getaddrinfo(host, service, &hints, &res)) != 0)
		{
			lua_pushnil(L);
			lua_pushstring(L, gai_strerror(r));
			lua_pushinteger(L, r);
			return 3;
		}
	}

	/* Copy getaddrinfo() result into Lua table */
	{
		struct addrinfo *p;
		lua_newtable(L);
		for (p = res; p != NULL; p = p->ai_next)
		{
			lua_pushnumber(L, n++);
			sockaddr_to_lua(L, p->ai_family, p->ai_addr);
			pushnumberfield(L, -2, "socktype", p->ai_socktype);
			pushstringfield(L, -2, "canonname", p->ai_canonname);
			pushnumberfield(L, -2, "protocol",  p->ai_protocol);
			lua_settable(L, -3);
		}
	}
	freeaddrinfo(res);


	return 1;
}


/***
Initiate a connection on a socket.
@function connect
@int fd socket descriptor to act on
@tparam sockaddr addr socket address
@treturn[1] bool `true`, if successful
@return[2] nil
@treturn[2] string error message
@see connect(2)
*/
static int
Pconnect(lua_State *L)
{
	struct sockaddr_storage sa;
	socklen_t salen;
	int fd = checkint(L, 1), r;
	checknargs (L, 2);
	if ((r = sockaddr_from_lua(L, 2, &sa, &salen)) != 0)
		return pusherror (L, "not a valid IPv4 dotted-decimal or IPv6 address string");

	r = connect(fd, (struct sockaddr *)&sa, salen);
	if (r < 0 && errno != EINPROGRESS)
		return pusherror(L, NULL);

	lua_pushboolean(L, 1);
	return 1;
}


/***
Bind an address to a socket.
@function bind
@int fd socket descriptor to act on
@tparam sockaddr addr socket address
@treturn[1] bool `true`, if successful
@return[2] nil
@treturn[2] string error message
@see bind(2)
*/
static int
Pbind(lua_State *L)
{
	struct sockaddr_storage sa;
	socklen_t salen;
	int fd, r;
	checknargs (L, 2);
	fd = checkint(L, 1);
	if ((r = sockaddr_from_lua(L, 2, &sa, &salen)) != 0)
		return pusherror (L, "not a valid IPv4 dotted-decimal or IPv6 address string");

	r = bind(fd, (struct sockaddr *)&sa, salen);
	if (r < 0)
		return pusherror(L, NULL);
	lua_pushboolean(L, 1);
	return 1;
}


/***
Listen for connections on a socket.
@function listen
@int fd socket descriptor to act on
@int backlog maximum length for queue of pending connections
@treturn[1] int `0`, if successful
@return[2] nil
@treturn[2] string  error message
@see listen(2)
*/
static int
Plisten(lua_State *L)
{
	int fd = checkint(L, 1);
	int backlog = checkint(L, 2);
	checknargs(L, 2);

	return pushresult(L, listen(fd, backlog), NULL);
}


/***
Accept a connection on a socket.
@function accept
@int fd socket descriptor to act on
@treturn[1] int connection descriptor
@treturn[1] table connection address, if successful
@return[2] nil
@treturn[2] string error message
@see accept(2)
*/
static int
Paccept(lua_State *L)
{
	int fd_client;
	struct sockaddr_storage sa;
	unsigned int salen;

	int fd = checkint(L, 1);

	checknargs(L, 1);

	salen = sizeof(sa);
	fd_client = accept(fd, (struct sockaddr *)&sa, &salen);
	if (fd_client == -1)
		return pusherror(L, NULL);

	lua_pushnumber(L, fd_client);
	sockaddr_to_lua(L, sa.ss_family, (struct sockaddr *)&sa);

	return 2;
}


/***
Receive a message from a socket.
@function recv
@int fd socket descriptor to act on
@int count maximum number of bytes to receive
@treturn[1] int received bytes, if successful
@return[2] nil
@treturn[2] string error message
@see recv(2)
*/
static int
Precv(lua_State *L)
{
	int fd = checkint(L, 1);
	int count = checkint(L, 2), ret;
	void *ud, *buf;
	lua_Alloc lalloc;

	checknargs(L, 2);
	lalloc = lua_getallocf(L, &ud);

	/* Reset errno in case lalloc doesn't set it */
	errno = 0;
	if ((buf = lalloc(ud, NULL, 0, count)) == NULL && count > 0)
		return pusherror(L, "lalloc");

	ret = recv(fd, buf, count, 0);
	if (ret < 0)
	{
		lalloc(ud, buf, count, 0);
		return pusherror(L, NULL);
	}

	lua_pushlstring(L, buf, ret);
	lalloc(ud, buf, count, 0);
	return 1;
}


/***
Receive a message from a socket.
@function recvfrom
@int fd socket descriptor to act on
@int count maximum number of bytes to receive
@treturn[1] int received bytes
@treturn[1] sockaddr address of message source, if successful
@return[2] nil
@treturn[2] string error message
@see recvfrom(2)
*/
static int
Precvfrom(lua_State *L)
{
	void *ud, *buf;
	socklen_t salen;
	struct sockaddr_storage sa;
	int r;
	int fd = checkint(L, 1);
	int count = checkint(L, 2);
	lua_Alloc lalloc;

	checknargs(L, 2);
	lalloc = lua_getallocf(L, &ud);

	/* Reset errno in case lalloc doesn't set it */
	errno = 0;
	if ((buf = lalloc(ud, NULL, 0, count)) == NULL && count > 0)
		return pusherror(L, "lalloc");

	salen = sizeof(sa);
	r = recvfrom(fd, buf, count, 0, (struct sockaddr *)&sa, &salen);
	if (r < 0)
	{
		lalloc(ud, buf, count, 0);
		return pusherror(L, NULL);
	}

	lua_pushlstring(L, buf, r);
	lalloc(ud, buf, count, 0);
	sockaddr_to_lua(L, sa.ss_family, (struct sockaddr *)&sa);

	return 2;
}


/***
Send a message from a socket.
@function send
@int fd socket descriptor to act on
@string buffer message bytes to send
@treturn[1] int number of bytes sent, if successful
@return[2] nil
@treturn[2] string error message
@see send(2)
*/
static int
Psend(lua_State *L)
{
	int fd = checkint (L, 1);
	size_t len;
	const char *buf = luaL_checklstring(L, 2, &len);

	checknargs(L, 2);
	return pushresult(L, send(fd, buf, len, 0), NULL);
}


/***
Send a message from a socket.
@function sendto
@int fd socket descriptor to act on
@string buffer message bytes to send
@tparam sockaddr destination socket address
@treturn[1] int number of bytes sent, if successful
@return[2] nil
@treturn[2] string error message
@see sendto(2)
*/
static int
Psendto(lua_State *L)
{
	size_t len;
	struct sockaddr_storage sa;
	socklen_t salen;
	int r, fd = checkint(L, 1);
	const char *buf = luaL_checklstring(L, 2, &len);
	checknargs (L, 3);
	if ((r = sockaddr_from_lua(L, 3, &sa, &salen)) != 0)
		return pusherror (L, "not a valid IPv4 dotted-decimal or IPv6 address string");

	r = sendto(fd, buf, len, 0, (struct sockaddr *)&sa, salen);
	return pushresult(L, r, NULL);
}


/***
Shut down part of a full-duplex connection.
@function shutdown
@int fd socket descriptor to act on
@int how one of `SHUT_RD`, `SHUT_WR` or `SHUT_RDWR`
@treturn[1] int `0`, if successful
@return[2] nil
@treturn[2] string error message
@see shutdown(2)
@usage ok, errmsg = P.shutdown (sock, P.SHUT_RDWR)
*/
static int
Pshutdown(lua_State *L)
{
	int fd = checkint(L, 1);
	int how = checkint(L, 2);
	checknargs(L, 2);
	return pushresult(L, shutdown(fd, how), NULL);
}


/***
Get and set options on sockets.
@function setsockopt
@int fd socket descriptor
@int level one of `SOL_SOCKET`, `IPPROTO_IPV6`, `IPPROTO_TCP`
@int name option name, varies according to `level` value
@param value1 option value to set
@param[opt] value2 some option *name*s need an additional value
@treturn[1] int `0`, if successful
@return[2] nil
@treturn[2] string error message
@see setsockopt(2)
@usage ok, errmsg = P.setsockopt (sock, P.SOL_SOCKET, P.SO_SNDTIMEO, 1, 0)
*/
static int
Psetsockopt(lua_State *L)
{
	int fd = checkint(L, 1);
	int level = checkint(L, 2);
	int optname = checkint(L, 3);
	struct linger linger;
	struct timeval tv;
	struct ipv6_mreq mreq6;
#ifdef SO_BINDTODEVICE
	struct ifreq ifr;
#endif
	int vint = 0;
	void *val = NULL;
	socklen_t len = sizeof(vint);

	switch (level)
	{
		case SOL_SOCKET:
			switch (optname)
			{
				case SO_LINGER:
					checknargs(L, 5);

					linger.l_onoff = checkint(L, 4);
					linger.l_linger = checkint(L, 5);
					val = &linger;
					len = sizeof(linger);
					break;
				case SO_RCVTIMEO:
				case SO_SNDTIMEO:
					checknargs(L, 5);

					tv.tv_sec = checkint(L, 4);
					tv.tv_usec = checkint(L, 5);
					val = &tv;
					len = sizeof(tv);
					break;
#ifdef SO_BINDTODEVICE
				case SO_BINDTODEVICE:
					checknargs(L, 4);

					strlcpy(ifr.ifr_name, luaL_checkstring(L, 4), IFNAMSIZ);
					val = &ifr;
					len = sizeof(ifr);
#endif
				default:
					checknargs(L, 4);
					break;
			}
			break;
#if defined IPV6_JOIN_GROUP && defined IPV6_LEAVE_GROUP
		case IPPROTO_IPV6:
			switch (optname)
			{
				case IPV6_JOIN_GROUP:
				case IPV6_LEAVE_GROUP:
					checknargs(L, 4);

					memset(&mreq6, 0, sizeof mreq6);
					inet_pton(AF_INET6, luaL_checkstring(L, 4), &mreq6.ipv6mr_multiaddr);
					val = &mreq6;
					len = sizeof(mreq6);
					break;
				default:
					checknargs(L, 4);
					break;
			}
			break;
#endif
		case IPPROTO_TCP:
			switch (optname)
			{
				default:
					checknargs(L, 4);
					break;
			}
			break;
		default:
			break;
	}

	/* Default fallback to int if no specific handling of type above */

	if (val == NULL)
	{
		vint = checkint(L, 4);
		val = &vint;
		len = sizeof(vint);
	}

	return pushresult(L, setsockopt(fd, level, optname, val, len), NULL);
}
#endif


static const luaL_Reg posix_sys_socket_fns[] =
{
#if _POSIX_VERSION >= 200112L
	LPOSIX_FUNC( Psocket		),
	LPOSIX_FUNC( Psocketpair	),
	LPOSIX_FUNC( Pgetaddrinfo	),
	LPOSIX_FUNC( Pconnect		),
	LPOSIX_FUNC( Pbind		),
	LPOSIX_FUNC( Plisten		),
	LPOSIX_FUNC( Paccept		),
	LPOSIX_FUNC( Precv		),
	LPOSIX_FUNC( Precvfrom		),
	LPOSIX_FUNC( Psend		),
	LPOSIX_FUNC( Psendto		),
	LPOSIX_FUNC( Pshutdown		),
	LPOSIX_FUNC( Psetsockopt	),
#endif
	{NULL, NULL}
};


LUALIB_API int
luaopen_posix_sys_socket(lua_State *L)
{
	luaL_register(L, "posix.sys.socket", posix_sys_socket_fns);
	lua_pushliteral(L, "posix.sys.socket for " LUA_VERSION " / " PACKAGE_STRING);
	lua_setfield(L, -2, "version");

#if _POSIX_VERSION >= 200112L
	LPOSIX_CONST( SOMAXCONN		);
	LPOSIX_CONST( AF_UNSPEC		);
	LPOSIX_CONST( AF_INET		);
	LPOSIX_CONST( AF_INET6		);
	LPOSIX_CONST( AF_UNIX		);
# if HAVE_LINUX_NETLINK_H
	LPOSIX_CONST( AF_NETLINK	);
# endif
	LPOSIX_CONST( SOL_SOCKET	);
	LPOSIX_CONST( IPPROTO_TCP	);
	LPOSIX_CONST(	IPPROTO_UDP	);
	LPOSIX_CONST( IPPROTO_IP	);
	LPOSIX_CONST( IPPROTO_IPV6	);
# ifdef IPPROTO_ICMP
	LPOSIX_CONST( IPPROTO_ICMP	);
# endif
	LPOSIX_CONST( SOCK_STREAM	);
	LPOSIX_CONST( SOCK_DGRAM	);
# ifdef SOCK_RAW
	LPOSIX_CONST( SOCK_RAW		);
# endif
	LPOSIX_CONST( SHUT_RD		);
	LPOSIX_CONST( SHUT_WR		);
	LPOSIX_CONST( SHUT_RDWR		);

	LPOSIX_CONST( SO_ACCEPTCONN	);
	LPOSIX_CONST( SO_BROADCAST	);
	LPOSIX_CONST( SO_LINGER	);
	LPOSIX_CONST( SO_RCVTIMEO	);
	LPOSIX_CONST( SO_SNDTIMEO	);
# ifdef SO_BINDTODEVICE
	LPOSIX_CONST( SO_BINDTODEVICE	);
# endif
	LPOSIX_CONST( SO_DEBUG	);
	LPOSIX_CONST( SO_DONTROUTE	);
	LPOSIX_CONST( SO_ERROR	);
	LPOSIX_CONST( SO_KEEPALIVE	);
	LPOSIX_CONST( SO_OOBINLINE	);
	LPOSIX_CONST( SO_RCVBUF	);
	LPOSIX_CONST( SO_RCVLOWAT	);
	LPOSIX_CONST( SO_REUSEADDR	);
	LPOSIX_CONST( SO_SNDBUF	);
	LPOSIX_CONST( SO_SNDLOWAT	);
	LPOSIX_CONST( SO_TYPE		);

	LPOSIX_CONST( TCP_NODELAY	);

	LPOSIX_CONST( AI_ADDRCONFIG	);
	LPOSIX_CONST( AI_ALL		);
	LPOSIX_CONST( AI_CANONNAME	);
	LPOSIX_CONST( AI_NUMERICHOST	);
	LPOSIX_CONST( AI_NUMERICSERV	);
	LPOSIX_CONST( AI_PASSIVE	);
	LPOSIX_CONST( AI_V4MAPPED	);

# ifdef IPV6_JOIN_GROUP
	LPOSIX_CONST( IPV6_JOIN_GROUP		);
# endif
# ifdef IPV6_LEAVE_GROUP
	LPOSIX_CONST( IPV6_LEAVE_GROUP		);
# endif
# ifdef IPV6_MULTICAST_HOPS
	LPOSIX_CONST( IPV6_MULTICAST_HOPS	);
# endif
# ifdef IPV6_MULTICAST_IF
	LPOSIX_CONST( IPV6_MULTICAST_IF		);
# endif
# ifdef IPV6_MULTICAST_LOOP
	LPOSIX_CONST( IPV6_MULTICAST_LOOP	);
# endif
# ifdef IPV6_UNICAST_HOPS
	LPOSIX_CONST( IPV6_UNICAST_HOPS		);
# endif
# ifdef IPV6_V6ONLY
	LPOSIX_CONST( IPV6_V6ONLY		);
# endif
# if HAVE_LINUX_NETLINK_H
	LPOSIX_CONST( NETLINK_ROUTE		);
	LPOSIX_CONST( NETLINK_UNUSED		);
	LPOSIX_CONST( NETLINK_USERSOCK		);
	LPOSIX_CONST( NETLINK_FIREWALL		);
	LPOSIX_CONST( NETLINK_NFLOG		);
	LPOSIX_CONST( NETLINK_XFRM		);
	LPOSIX_CONST( NETLINK_SELINUX		);
	LPOSIX_CONST( NETLINK_ISCSI		);
	LPOSIX_CONST( NETLINK_AUDIT		);
	LPOSIX_CONST( NETLINK_FIB_LOOKUP	);
	LPOSIX_CONST( NETLINK_CONNECTOR		);
	LPOSIX_CONST( NETLINK_NETFILTER		);
	LPOSIX_CONST( NETLINK_IP6_FW		);
	LPOSIX_CONST( NETLINK_DNRTMSG		);
	LPOSIX_CONST( NETLINK_KOBJECT_UEVENT	);
	LPOSIX_CONST( NETLINK_GENERIC		);
	LPOSIX_CONST( NETLINK_SCSITRANSPORT	);
	LPOSIX_CONST( NETLINK_ECRYPTFS		);
# endif
#endif

	return 1;
}
