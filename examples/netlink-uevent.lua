p = require "posix"

if p.AF_NETLINK ~= nil then
	local fd, err = p.socket(p.AF_NETLINK, p.SOCK_DGRAM, p.NETLINK_KOBJECT_UEVENT)
	assert(fd, err)

	local ok, err = p.bind(fd, { family = p.AF_NETLINK, pid = p.getpid("pid"), groups = -1 })
	assert(ok, err)

	while true do
		local data, err = p.recv(fd, 16384)
		assert(data, err)
		for k, v in data:gmatch("%z(%u+)=([^%z]+)") do
			print(k, v)
		end
		print("\n")
	end
end
