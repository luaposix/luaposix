--[[
 POSIX library for Lua 5.1/5.2.
 (c) Gary V. Vaughan <gary@vaughan.pe>, 2014
]]
--[[--
 Legacy Lua POSIX bindings.

 Undocumented Legacy APIs for compatibility with previous releases.

 @module posix.deprecated
]]

local M = {}


--- Initiiate a connection on a socket.
-- @function connect
-- @int fd socket descriptor to act on
-- @tparam PosixSockaddr addr socket address
-- @treturn[1] bool `true`, if successful
-- @return[2] nil
-- @treturn[2] string error message
-- @see connect(2)

local sock = require "posix.sys.socket"

local connect = sock.connect

function M.connect (...)
  local rt = { connect (...) }
  if rt[1] == 0 then return true end
  return unpack (rt)
end


--- Bind an address to a socket.
-- @function bind
-- @int fd socket descriptor to act on
-- @tparam PosixSockaddr addr socket address
-- @treturn[1] bool `true`, if successful
-- @return[2] nil
-- @treturn[2] string error messag
-- @see bind(2)

local sock = require "posix.sys.socket"

local bind = sock.bind

function M.bind (...)
  local rt = { bind (...) }
  if rt[1] == 0 then return true end
  return unpack (rt)
end


return M
