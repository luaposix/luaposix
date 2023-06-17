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
 BSD Sockets.

 Where supported by the underlying system, functions and constants to create,
 connect and communicate over BSD sockets.  If the module loads successfully,
 but there is no kernel support, then `posix.sys.socket.version` will be set,
 but the unsupported APIs will be `nil`.

@module posix.sys.socket
*/

#include "_helpers.c"		/* For LPOSIX_2001_COMPLIANT */

#include <sys/types.h>
#if LPOSIX_2001_COMPLIANT
#include <arpa/inet.h>
#if HAVE_LINUX_NETLINK_H
#include <linux/netlink.h>
#endif
#if HAVE_LINUX_IF_PACKET_H
#include <linux/if_packet.h>
#endif
#include <sys/socket.h>		/* Needs to be before net/if.h on OpenBSD 5.6 */
#ifdef HAVE_NET_IF_H
#include <net/if.h>
#endif
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <sys/time.h>
#include <sys/un.h>


/***
Socket address.
All sockaddr tables have the *family* field, and depending on its value, also
a subset of the following fields too.
@table sockaddr
@int family one of `AF_INET`, `AF_INET6`, `AF_UNIX`, `AF_NETLINK` or `AF_PACKET`
@int[opt] port socket port number for `AF_INET` (and equivalently `AF_INET6`) *family*
@string[opt] addr socket host address in correct format, for `AF_INET` *family*
@int[opt] socktype one of `SOCK_STREAM`, `SOCK_DGRAM` or `SOCK_RAW` for `AF_INET` *family*
@string[opt] canonname canonical name for service location, for `AF_INET` *family*
@int[opt] protocol one of `IPPROTO_TCP` or `IPPROTO_UDP`, for `AF_INET` *family*
@string[opt] path location in file system, for `AF_UNIX` *family*
@int[opt] pid process identifier, for `AF_NETLINK` *family*
@int[opt] groups process group owner identifier, for `AF_NETLINK` *family*
@int[opt] ifindex interface index, for `AF_PACKET` *family*
*/


/***
Address information hints.
@table PosixAddrInfo
@int family one of `AF_INET`, `AF_INET6`, `AF_UNIX`, `AF_NETLINK` or
  `AF_PACKET`
@int flags bitwise OR of zero or more of `AI_ADDRCONFIG`, `AI_ALL`,
  `AI_CANONNAME`, `AI_NUMERICHOST`, `AI_NUMERICSERV`, `AI_PASSIVE` and
  `AI_V4MAPPED`
@int socktype one of `SOCK_STREAM`, `SOCK_DGRAM` or `SOCK_RAW`
@int protocol one of `IPPROTO_TCP` or `IPPROTO_UDP`
*/
static int
pushsockaddrinfo(lua_State *L, int family, struct sockaddr *sa)
{
	char addr[INET6_ADDRSTRLEN];

	lua_newtable(L);
	pushintegerfield("family", family);

	switch (family)
	{
		case AF_INET:
		{
			struct sockaddr_in *sa4 = (struct sockaddr_in *)sa;
			inet_ntop(family, &sa4->sin_addr, addr, sizeof addr);
			pushintegerfield("port", ntohs(sa4->sin_port));
			pushstringfield("addr", addr);
			break;
		}
		case AF_INET6:
		{
			struct sockaddr_in6 *sa6 = (struct sockaddr_in6 *)sa;
			inet_ntop(family, &sa6->sin6_addr, addr, sizeof addr);
			pushintegerfield("port", ntohs(sa6->sin6_port));
			pushstringfield("addr", addr);
			pushintegerfield("flowinfo", ntohl(sa6->sin6_flowinfo));
			pushintegerfield("scope_id", sa6->sin6_scope_id);
			break;
		}
		case AF_UNIX:
		{
			struct sockaddr_un *sau = (struct sockaddr_un *)sa;
			size_t path_len = sizeof sau->sun_path;
			char *end = memchr(sau->sun_path, 0, path_len);
			if (end)
				path_len = end - sau->sun_path;
			pushlstringfield("path", sau->sun_path, path_len);
			break;
		}
#if HAVE_LINUX_NETLINK_H
		case AF_NETLINK:
		{
			struct sockaddr_nl *san = (struct sockaddr_nl *)sa;
			pushintegerfield("pid", san->nl_pid);
			pushintegerfield("groups", san->nl_groups);
			break;
		}
#endif
#if HAVE_LINUX_IF_PACKET_H
		case AF_PACKET:
		{
			struct sockaddr_ll *sal = (struct sockaddr_ll *)sa;
			pushintegerfield("ifindex", sal->sll_ifindex);
			break;
		}
#endif
	}

	settypemetatable("PosixAddrInfo");
	return 1;
}


/***
Create an endpoint for communication.
@function socket
@int domain one of `AF_INET`, `AF_INET6`, `AF_UNIX`, `AF_NETLINK` or
  `AF_PACKET`
@int type one of `SOCK_STREAM`, `SOCK_DGRAM` or `SOCK_RAW`
@int options usually 0, but some socket types might implement other protocols.
@treturn[1] int socket descriptor, if successful
@return[2] nil
@treturn[2] string error message
@treturn[2] int errnum
@see socket(2)
@usage
  local sys_sock = require "posix.sys.socket"
  sockd = sys_sock.socket (sys_sock.AF_INET, sys_sock.SOCK_STREAM, 0)
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
@treturn[2] int errnum
@see socketpair(2)
@usage
  local sys_sock = require "posix.sys.socket"
  sockr, sockw = sys_sock.socketpair (sys_sock.AF_INET, sys_sock.SOCK_STREAM, 0)
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


static const char *Safinet_fields[] = { /* AF_INET6 only */
					"flowinfo", "scope_id",
					/* AF_INET4 and AF_INET6 shared */
					"family", "port", "addr",
					/* Also allow getaddrinfo result tables */
					"socktype", "canonname", "protocol" };
static const char *Safunix_fields[] = { "family", "path" };
static const char *Safnetlink_fields[] = { "family", "pid", "groups" };
static const char *Safpacket_fields[] = { "family", "ifindex" };

#define Safinet4_fields (Safinet_fields + 2)
#define Safinet6_fields Safinet_fields


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
			int nfields = (sizeof(Safinet_fields)/sizeof(*Safinet_fields))-2;

			(checkfieldnames)(L, index, nfields, Safinet4_fields);

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
			/* For compatibility, the next two fields are optional */
			uint32_t flowinfo	= (uint32_t)optintegerfield(L, index, "flowinfo", 0);
			uint32_t scope_id	= (uint32_t)optintegerfield(L, index, "scope_id", 0);

			checkfieldnames (L, index, Safinet6_fields);

			if (inet_pton(AF_INET6, addr, &sa6->sin6_addr) == 1)
			{
				sa6->sin6_family= family;
				sa6->sin6_port	= htons(port);
				sa6->sin6_flowinfo	= htonl(flowinfo);
				sa6->sin6_scope_id	= scope_id;
				*addrlen	= sizeof(*sa6);
				r		= 0;
			}
			break;
		}
		case AF_UNIX:
		{
			struct sockaddr_un *sau	= (struct sockaddr_un *)sa;
			size_t bufsize		= sizeof(sau->sun_path);
			size_t pathlen;
			const char *path	= checklstringfield(L, index, "path", &pathlen);

			checkfieldnames (L, index, Safunix_fields);

			if (pathlen > bufsize) pathlen = bufsize;
			memcpy(sau->sun_path, path, pathlen);

			sau->sun_family		= family;
			*addrlen		= sizeof(*sau) - bufsize + pathlen;
			r			= 0;
			break;
		}
#if HAVE_LINUX_NETLINK_H
		case AF_NETLINK:
		{
			struct sockaddr_nl *san	= (struct sockaddr_nl *)sa;
			san->nl_family		= family;
			san->nl_pid		= checkintfield(L, index, "pid");
			san->nl_groups		= checkintfield(L, index, "groups");
			*addrlen		= sizeof(*san);

			checkfieldnames (L, index, Safnetlink_fields);

			r			= 0;
			break;
		}
#endif
#if HAVE_LINUX_IF_PACKET_H
		case AF_PACKET:
		{
			struct sockaddr_ll *sal	= (struct sockaddr_ll *)sa;
			sal->sll_family		= family;
			sal->sll_ifindex	= checkintfield(L, index, "ifindex");
			*addrlen		= sizeof(*sal);

			checkfieldnames (L, index, Safpacket_fields);

			r			= 0;
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
@string host name of a host. For IPv6, link-local addresses like 'ff02::1%eth0' are supported.
@string service name of service
@tparam[opt] PosixAddrInfo hints table
@treturn[1] list of @{sockaddr} tables, if successful
@return[2] nil
@treturn[2] string error message
@treturn[2] int errnum
@see getaddrinfo(2)
@usage
  local sys_sock = require "posix.sys.socket"
  local res, errmsg, errcode = sys_sock.getaddrinfo ("www.lua.org", "http",
    { family = sys_sock.IF_INET, socktype = sys_sock.SOCK_STREAM }
)
*/
static int
Pgetaddrinfo(lua_State *L)
{
	int n = 1;
	const char *host = optstring(L, 1, NULL);
	const char *service = NULL;
	struct addrinfo *res, hints;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = PF_UNSPEC;

	checknargs(L, 3);

	switch (lua_type(L, 2))
	{
		case LUA_TNONE:
		case LUA_TNIL:
			if (host == NULL)
				argtypeerror(L, 2, "integer or string");
			break;
		case LUA_TNUMBER:
		case LUA_TSTRING:
			service = lua_tostring(L, 2);
			break;
		default:
			argtypeerror(L, 2, "integer, nil or string");
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
			argtypeerror(L, 3, "nil or table");
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
			lua_pushinteger(L, n++);
			pushsockaddrinfo(L, p->ai_family, p->ai_addr);
			pushintegerfield("socktype",  p->ai_socktype);
			pushstringfield("canonname", p->ai_canonname);
			pushintegerfield("protocol",  p->ai_protocol);
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
@treturn[1] int `0`, if successful
@return[2] nil
@treturn[2] string error message
@treturn[2] int errnum
@see connect(2)
*/
static int
Pconnect(lua_State *L)
{
	struct sockaddr_storage sa;
	socklen_t salen;
	int fd = checkint(L, 1);
	checknargs (L, 2);
	if (sockaddr_from_lua(L, 2, &sa, &salen) != 0)
		return pusherror(L, "not a valid IPv4 or IPv6 address argument");

	return pushresult(L, connect(fd, (struct sockaddr *)&sa, salen), "connect");
}


/***
Bind an address to a socket.
@function bind
@int fd socket descriptor to act on
@tparam sockaddr addr socket address
@treturn[1] int `0`, if successful
@return[2] nil
@treturn[2] string error message
@treturn[2] int errnum
@see bind(2)
*/
static int
Pbind(lua_State *L)
{
	struct sockaddr_storage sa;
	socklen_t salen;
	int fd;
	checknargs (L, 2);
	fd = checkint(L, 1);
	if (sockaddr_from_lua(L, 2, &sa, &salen) != 0)
		return pusherror(L, "not a valid IPv4 or IPv6 argument");

	return pushresult(L, bind(fd, (struct sockaddr *)&sa, salen), "bind");
}


/***
Listen for connections on a socket.
@function listen
@int fd socket descriptor to act on
@int backlog maximum length for queue of pending connections
@treturn[1] int `0`, if successful
@return[2] nil
@treturn[2] string  error message
@treturn[2] int errnum
@see listen(2)
*/
static int
Plisten(lua_State *L)
{
	int fd = checkint(L, 1);
	int backlog = checkint(L, 2);
	checknargs(L, 2);

	return pushresult(L, listen(fd, backlog), "listen");
}


/***
Accept a connection on a socket.
@function accept
@int fd socket descriptor to act on
@treturn[1] int connection descriptor
@treturn[1] table connection address, if successful
@return[2] nil
@treturn[2] string error message
@treturn[2] int errnum
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
		return pusherror(L, "accept");

	lua_pushinteger(L, fd_client);
	return 1 + pushsockaddrinfo(L, sa.ss_family, (struct sockaddr *)&sa);
}


/***
Receive a message from a socket.
@function recv
@int fd socket descriptor to act on
@int count maximum number of bytes to receive
@treturn[1] string received bytes, if successful
@return[2] nil
@treturn[2] string error message
@treturn[2] int errnum
@see recv(2)
*/
static int
Precv(lua_State *L)
{
	int fd = checkint(L, 1);
	size_t count = (size_t)checkinteger(L, 2);
	ssize_t ret;
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
@treturn[2] int errnum
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
	size_t count = (size_t)checkinteger(L, 2);
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
	return 1 + pushsockaddrinfo(L, sa.ss_family, (struct sockaddr *)&sa);
}


/***
Send a message from a socket.
@function send
@int fd socket descriptor to act on
@string buffer message bytes to send
@treturn[1] int number of bytes sent, if successful
@return[2] nil
@treturn[2] string error message
@treturn[2] int errnum
@see send(2)
*/
static int
Psend(lua_State *L)
{
	int fd = checkint (L, 1);
	size_t len;
	const char *buf = luaL_checklstring(L, 2, &len);

	checknargs(L, 2);
	return pushresult(L, send(fd, buf, len, 0), "send");
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
@treturn[2] int errnum
@see sendto(2)
*/
static int
Psendto(lua_State *L)
{
	size_t len;
	int fd = checkint(L, 1);
	const char *buf = luaL_checklstring(L, 2, &len);
	struct sockaddr_storage sa;
	socklen_t salen;
	checknargs (L, 3);
	if (sockaddr_from_lua(L, 3, &sa, &salen) != 0)
		return pusherror (L, "not a valid IPv4 or IPv6 argument");

	return pushresult(L, sendto(fd, buf, len, 0, (struct sockaddr *)&sa, salen), "sendto");
}


/***
Shut down part of a full-duplex connection.
@function shutdown
@int fd socket descriptor to act on
@int how one of `SHUT_RD`, `SHUT_WR` or `SHUT_RDWR`
@treturn[1] int `0`, if successful
@return[2] nil
@treturn[2] string error message
@treturn[2] int errnum
@see shutdown(2)
@usage
  local sys_sock = require "posix.sys.socket"
  ok, errmsg = sys_sock.shutdown (sock, sys_sock.SHUT_RDWR)
*/
static int
Pshutdown(lua_State *L)
{
	int fd = checkint(L, 1);
	int how = checkint(L, 2);
	checknargs(L, 2);
	return pushresult(L, shutdown(fd, how), "shutdown");
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
@treturn[2] int errnum
@see setsockopt(2)
@usage
  local sys_sock = require "posix.sys.socket"
  ok, errmsg = sys_sock.setsockopt (
    sock, sys_sock.SOL_SOCKET, sys_sock.SO_SNDTIMEO, 1, 0
  )
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
	char ifname[IFNAMSIZ];
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

					linger.l_onoff = checkinteger(L, 4);
					linger.l_linger = checkinteger(L, 5);
					val = &linger;
					len = sizeof(linger);
					break;
				case SO_RCVTIMEO:
				case SO_SNDTIMEO:
					checknargs(L, 5);

					tv.tv_sec = checkinteger(L, 4);
					tv.tv_usec = checkinteger(L, 5);
					val = &tv;
					len = sizeof(tv);
					break;
#ifdef SO_BINDTODEVICE
				case SO_BINDTODEVICE:
					checknargs(L, 4);

					strncpy(ifname, luaL_checkstring(L, 4), IFNAMSIZ - 1);
					ifname[IFNAMSIZ - 1] = '\0';
					val = ifname;
					len = strlen(ifname);
					break;
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
		vint = checkinteger(L, 4);
		val = &vint;
		len = sizeof(vint);
	}

	return pushresult(L, setsockopt(fd, level, optname, val, len), "setsockopt");
}


/***
Get options on sockets.
@function getsockopt
@int fd socket descriptor
@int level one of `SOL_SOCKET`, `IPPROTO_IPV6`, `IPPROTO_TCP`
@int name option name, varies according to `level` value
@return[1] the value of the requested socket option, if successful
@return[2] nil
@treturn[2] string error message
@treturn[2] int errnum
@see getsockopt(2)
@usage
  local sys_sock = require "posix.sys.socket"
  val, errmsg, errnum = sys_sock.getsockopt(
    sock, sys_sock.SOL_SOCKET, sys_sock.SO_SNDTIMEO
  )
  print('Send timeout ', val.tv_sec + val.tv_usec / 1000000)
*/
static int
Pgetsockopt(lua_State *L)
{
	int fd = checkint(L, 1);
	int level = checkint(L, 2);
	int optname = checkint(L, 3);
	checknargs(L, 3);
	struct linger linger;
	struct timeval tv;
	int err = 0;
#ifdef SO_BINDTODEVICE
	char ifname[IFNAMSIZ];
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
					val = &linger;
					len = sizeof(linger);
					break;
				case SO_RCVTIMEO:
				case SO_SNDTIMEO:
					val = &tv;
					len = sizeof(tv);
					break;
#ifdef SO_BINDTODEVICE
				case SO_BINDTODEVICE:
					val = ifname;
					len = IFNAMSIZ;
					break;
#endif
				default:
					break;
			}
			break;
		default:
			break;
	}

	/* Default fallback to int if no specific handling of type above */

	if (val == NULL)
	{
		val = &vint;
		len = sizeof(vint);
	}

	err = getsockopt(fd, level, optname, val, &len);
	if (err == -1)
	{
		return pusherror(L, "getsockopt");
	}

	if (val == &tv)
	{
		lua_createtable(L, 0, 2);
		pushintegerfield("tv_sec", tv.tv_sec);
		pushintegerfield("tv_usec", tv.tv_usec);
		settypemetatable("PosixTimeval");
	}
	else if (val == &linger)
	{
		lua_createtable(L, 0, 2);
		pushintegerfield("l_linger", linger.l_linger);
		pushintegerfield("l_onoff", linger.l_onoff);
		settypemetatable("PosixLinger");
	}
#ifdef SO_BINDTODEVICE
	else if (val == &ifname)
	{
		lua_pushlstring(L, ifname, len);
	}
#endif
	else {
		lua_pushinteger(L, vint);
	}

	return 1;
}


/***
Get socket name.
@function getsockname
@see getsockname(2)
@int sockfd socket descriptor
@treturn[1] sockaddr the current address to which the socket *sockfd* is bound, if successful
@return[2] nil
@treturn[2] string error message
@treturn[2] int errnum
@usage sa, err = posix.getsockname (sockfd)
*/
static int Pgetsockname(lua_State *L)
{
	int fd = checkint(L, 1);
	struct sockaddr_storage sa;
	socklen_t salen = sizeof sa;
	checknargs (L, 1);
	if (getsockname(fd, (struct sockaddr *)&sa, &salen) != 0)
		return pusherror(L, "getsockname");
	return pushsockaddrinfo(L, sa.ss_family, (struct sockaddr *)&sa);
}


/***
Get socket peer name.
@function getpeername
@int sockfd socket descriptor
@treturn[1] sockaddr the address to which the socket *sockfd* is connected to, if successful
@return[2] nil
@treturn[2] string error message
@treturn[2] int errnum
@see getpeername(2)
@usage sa, err = posix.getpeername (sockfd)
*/
static int Pgetpeername(lua_State *L)
{
	int fd = checkint(L, 1);
	struct sockaddr_storage sa;
	socklen_t salen = sizeof sa;
	checknargs (L, 1);
	if (getpeername(fd, (struct sockaddr *)&sa, &salen) != 0)
		return pusherror(L, "getpeername");
	return pushsockaddrinfo(L, sa.ss_family, (struct sockaddr *)&sa);
}


/***
Get network interface index by name.
Needed for packet sockets, since SO_BINDTODEVICE won't work on packet family.
@function if_nametoindex
@string ifname interface name
@treturn[1] int interface index, if successful
@return[2] nil
@treturn[2] string error message
@treturn[2] int errnum
@see if_nametoindex(3)
@usage
  local sys_sock = require "posix.sys.socket"
  ifindex, errmsg, errnum = sys_sock.if_nametoindex("eth0")
*/
static int Pif_nametoindex(lua_State *L)
{
	const char *ifname = luaL_checkstring(L, 1);
	checknargs (L, 1);
	int ifindex = (int)if_nametoindex(ifname);
	if (ifindex == 0)
		return pusherror(L, "if_nametoindex");
	lua_pushinteger(L, ifindex);
	return 1;
}
#endif


static const luaL_Reg posix_sys_socket_fns[] =
{
#if LPOSIX_2001_COMPLIANT
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
	LPOSIX_FUNC( Pgetsockopt	),
	LPOSIX_FUNC( Pgetsockname	),
	LPOSIX_FUNC( Pgetpeername	),
	LPOSIX_FUNC( Pif_nametoindex	),
#endif
	{NULL, NULL}
};


/***
Constants.
@section constants
*/

/***
Socket constants.
Any constants not available in the underlying system will be `nil` valued.
@table posix.sys.socket
@int AF_INET IP protocol family
@int AF_INET6 IP version 6
@int AF_NETLINK Netlink protocol family
@int AF_PACKET Packet protocol family
@int AF_UNIX local to host
@int AF_UNSPEC unspecified
@int AI_ADDRCONFIG use host configuration for returned address type
@int AI_ALL return IPv4 mapped and IPv6 addresses
@int AI_CANONNAME request canonical name
@int AI_NUMERICHOST don't use domain name resolution
@int AI_NUMERICSERV don't use service name resolution
@int AI_PASSIVE address is intended for @{bind}
@int AI_V4MAPPED IPv4 mapped addresses are acceptable
@int IPPROTO_ICMP internet control message protocol
@int IPPROTO_IP internet protocol
@int IPPROTO_IPV6 IPv6 header
@int IPPROTO_TCP transmission control protocol
@int IPPROTO_UDP user datagram protocol
@int IPV6_JOIN_GROUP
@int IPV6_LEAVE_GROUP
@int IPV6_MULTICAST_HOPS
@int IPV6_MULTICAST_IF
@int IPV6_MULTICAST_LOOP
@int IPV6_UNICAST_HOPS
@int IPV6_V6ONLY
@int NETLINK_AUDIT auditing
@int NETLINK_CONNECTOR
@int NETLINK_DNRTMSG decnet routing messages
@int NETLINK_ECRYPTFS
@int NETLINK_FIB_LOOKUP
@int NETLINK_FIREWALL firewalling hook
@int NETLINK_GENERIC
@int NETLINK_IP6_FW
@int NETLINK_ISCSI open iSCSI
@int NETLINK_KOBJECT_UEVENT kernel messages to userspace
@int NETLINK_NETFILTER netfilter subsystem
@int NETLINK_NFLOG netfilter/iptables ULOG
@int NETLINK_ROUTE routing/device hook
@int NETLINK_SCSITRANSPORT SCSI transports
@int NETLINK_SELINUX SELinux event notifications
@int NETLINK_UNUSED unused number
@int NETLINK_USERSOCK reserved for user mode socket protocols
@int NETLINK_XFRM ipsec
@int SHUT_RD no more receptions
@int SHUT_RDWR no more receptions or transmissions
@int SHUT_WR no more transmissions
@int SOCK_DGRAM connectionless unreliable datagrams
@int SOCK_RAW raw protocol interface
@int SOCK_STREAM connection based byte stream
@int SOL_SOCKET socket level
@int SOMAXCONN maximum concurrent connections
@int SO_ACCEPTCONN does this socket accept connections
@int SO_BINDTODEVICE bind to a particular device
@int SO_BROADCAST permit broadcasts
@int SO_DEBUG turn-on socket debugging
@int SO_DONTROUTE bypass standard routing
@int SO_ERROR set socket error flag
@int SO_KEEPALIVE periodically transmit keep-alive message
@int SO_LINGER linger on a @{posix.unistd.close} if data is still present
@int SO_OOBINLINE leave out-of-band data inline
@int SO_RCVBUF set receive buffer size
@int SO_RCVLOWAT set receive buffer low water mark
@int SO_RCVTIMEO set receive timeout
@int SO_REUSEADDR reuse local addresses
@int SO_SNDBUF set send buffer size
@int SO_SNDLOWAT set send buffer low water mark
@int SO_SNDTIMEO set send timeout
@int SO_TYPE get the socket type
@int TCP_NODELAY don't delay send for packet coalescing
@usage
  -- Print socket constants supported on this host.
  for name, value in pairs (require "posix.sys.socket") do
    if type (value) == "number" then
      print (name, value)
     end
  end
*/

LUALIB_API int
luaopen_posix_sys_socket(lua_State *L)
{
	luaL_newlib(L, posix_sys_socket_fns);
	lua_pushstring(L, LPOSIX_VERSION_STRING("sys.socket"));
	lua_setfield(L, -2, "version");

#if LPOSIX_2001_COMPLIANT
	LPOSIX_CONST( SOMAXCONN		);
	LPOSIX_CONST( AF_UNSPEC		);
	LPOSIX_CONST( AF_INET		);
	LPOSIX_CONST( AF_INET6		);
	LPOSIX_CONST( AF_UNIX		);
# if HAVE_LINUX_NETLINK_H
	LPOSIX_CONST( AF_NETLINK	);
# endif
# if HAVE_LINUX_IF_PACKET_H
	LPOSIX_CONST( AF_PACKET		);
# endif
	LPOSIX_CONST( SOL_SOCKET	);
	LPOSIX_CONST( IPPROTO_TCP	);
	LPOSIX_CONST( IPPROTO_UDP	);
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

# ifdef AI_ADDRCONFIG
	LPOSIX_CONST( AI_ADDRCONFIG	);
# endif
# ifdef AI_ALL
	LPOSIX_CONST( AI_ALL		);
# endif
	LPOSIX_CONST( AI_CANONNAME	);
	LPOSIX_CONST( AI_NUMERICHOST	);
	LPOSIX_CONST( AI_NUMERICSERV	);
	LPOSIX_CONST( AI_PASSIVE	);
# ifdef AI_V4MAPPED
	LPOSIX_CONST( AI_V4MAPPED	);
# endif

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
