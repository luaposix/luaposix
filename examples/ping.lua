local p = require "posix"

if p.SOCK_RAW and p.SO_BINDTODEVICE then
	-- Open raw socket

	local fd, err = p.socket(p.AF_INET, p.SOCK_RAW, p.IPPROTO_ICMP)
	assert(fd, err)

	-- Optionally, bind to specific device

	local ok, err = p.setsockopt(fd, p.SOL_SOCKET, p.SO_BINDTODEVICE, "wlan0")
	assert(ok, err)

	-- Create raw ICMP echo (ping) message

	local data = string.char(0x08, 0x00, 0x89, 0x98, 0x6e, 0x63, 0x00, 0x04, 0x00)

	-- Send message

	local ok, err = p.sendto(fd, data, { family = p.AF_INET, addr = "8.8.8.8", port = 0 })
	assert(ok, err)

	-- Read reply

	local data, sa = p.recvfrom(fd, 1024)
	assert(data, sa)

	if data then
		print("Received ICMP message from " .. sa.addr)
	end
end
