#! /usr/bin/env lua

local M = require 'posix.sys.socket'


local sig = require 'posix.signal'
sig.signal(sig.SIGPIPE, function() print 'pipe' end)

-- Get Lua web site title
local r, err = M.getaddrinfo('www.lua.org', 'http', {family=M.AF_INET, socktype=M.SOCK_STREAM})
if not r then
   error(err)
end

local fd = M.socket(M.AF_INET, M.SOCK_STREAM, 0)
local ok, err, e = M.connect(fd, r[1])
local sa = M.getsockname(fd)
print('Local socket bound to ' .. sa.addr .. ':' .. tostring(sa.port))
if err then
   error(err)
end

M.send(fd, 'GET / HTTP/1.0\r\nHost: www.lua.org\r\n\r\n')
local data = {}
while true do
   local b = M.recv(fd, 1024)
   if not b or #b == 0 then
      break
   end
   table.insert(data, b)
end
require 'posix.unistd'.close(fd)

data = table.concat(data)
print(string.match(data, '<TITLE>(.+)</TITLE>'))

-- Loopback UDP test, IPV4 and IPV6
local fd = M.socket(M.AF_INET6, M.SOCK_DGRAM, 0)
M.bind(fd, {family=M.AF_INET6, addr='::', port=9999})
M.sendto(fd, 'Test ipv4', {family=M.AF_INET, addr='127.0.0.1', port=9999})
M.sendto(fd, 'Test ipv6', {family=M.AF_INET6, addr='::', port=9999})
for i = 1, 2 do
   local ok, r = M.recvfrom(fd, 1024)
   if ok then
      print(ok, r.addr, r.port)
   else
      print(ok, r)
   end
end
require 'posix.unistd'.close(fd)

os.exit(0)
