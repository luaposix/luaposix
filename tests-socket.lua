local p = require "posix"

-- Loopback UDP test, IPV4 and IPV6 

local fd = p.socket(p.AF_INET6, p.SOCK_DGRAM, 0)
p.setsockopt(fd, p.SOL_SOCKET, p.SO_RCVTIMEO, 1, 0)
p.bind(fd, { family = p.AF_INET6, addr = "::", port = 9999 })
p.sendto(fd, "Test ipv4", { family = p.AF_INET, addr = "127.0.0.1", port = 9999 })
local data, sa = p.recvfrom(fd, 1024)
assert(data)
