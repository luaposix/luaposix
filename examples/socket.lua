local p = require "posix"
local b = bit32 or require "bit"

p.signal(p.SIGPIPE, function() print("pipe") end)

-- Get Lua web site title

local res, err = p.getaddrinfo("www.lua.org", "http", { family = p.AF_INET, socktype = p.SOCK_STREAM })
if not res then error(err) end

local fd = p.socket(p.AF_INET, p.SOCK_STREAM, 0)
local ok, err, e = p.connect(fd, res[1])
if err then error(err) end
p.send(fd, "GET / HTTP/1.0\r\nHost: www.lua.org\r\n\r\n")
local data = {}
while true do
	local b = p.recv(fd, 1024)
	if not b or #b == 0 then
		break
	end
	table.insert(data, b)
end
p.close(fd)
data = table.concat(data)
print(data:match("<TITLE>(.+)</TITLE>"))

-- Loopback UDP test, IPV4 and IPV6

local fd = p.socket(p.AF_INET6, p.SOCK_DGRAM, 0)
p.bind(fd, { family = p.AF_INET6, addr = "::", port = 9999 })
p.sendto(fd, "Test ipv4", { family = p.AF_INET, addr = "127.0.0.1", port = 9999 })
p.sendto(fd, "Test ipv6", { family = p.AF_INET6, addr = "::", port = 9999 })
for i = 1, 2 do
	local ok, r = p.recvfrom(fd, 1024)
	if ok then
		print(ok, r.addr, r.port)
	else
		print(ok, r)
	end
end
p.close(fd)
