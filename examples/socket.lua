local sig    = require "posix.signal"
local sock   = require "posix.sys.socket"
local unistd = require "posix.unistd"

sig.signal (sig.SIGPIPE, function () print "pipe" end)

-- Get Lua web site title
local r, err = sock.getaddrinfo ("www.lua.org", "http", { family = sock.AF_INET, socktype = sock.SOCK_STREAM })
if not r then error (err) end

local fd = sock.socket (sock.AF_INET, sock.SOCK_STREAM, 0)
local ok, err, e = sock.connect (fd, r[1])
local sa = sock.getsockname(fd)
print("Local socket bound to " .. sa.addr .. ":" .. tostring(sa.port))
if err then error (err) end

sock.send (fd, "GET / HTTP/1.0\r\nHost: www.lua.org\r\n\r\n")
local data = {}
while true do
	local b = sock.recv (fd, 1024)
	if not b or #b == 0 then
		break
	end
	table.insert (data, b)
end
unistd.close (fd)
data = table.concat (data)
print (data:match "<TITLE>(.+)</TITLE>")

-- Loopback UDP test, IPV4 and IPV6
local fd = sock.socket (sock.AF_INET6, sock.SOCK_DGRAM, 0)
sock.bind (fd, { family = sock.AF_INET6, addr = "::", port = 9999 })
sock.sendto (fd, "Test ipv4", { family = sock.AF_INET, addr = "127.0.0.1", port = 9999 })
sock.sendto (fd, "Test ipv6", { family = sock.AF_INET6, addr = "::", port = 9999 })
for i = 1, 2 do
	local ok, r = sock.recvfrom (fd, 1024)
	if ok then
		print (ok, r.addr, r.port)
	else
		print (ok, r)
	end
end
unistd.close (fd)

os.exit (0)
