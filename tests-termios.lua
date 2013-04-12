local p = require "posix"
local b = bit32 or require "bit"
local dev = "/dev/tty"

local rates = {
	[p.B0] = 0, [p.B50] = 50, [p.B75] = 75, [p.B110] = 110, [p.B134] = 134,
	[p.B150] = 150, [p.B200] = 200, [p.B300] = 300, [p.B600] = 600,
	[p.B1200] = 1200, [p.B1800] = 1800, [p.B2400] = 2400, [p.B4800] = 4800,
	[p.B9600] = 9600, [p.B19200] = 19200, [p.B38400] = 38400, [p.B115200] =
	115200,
}

local fd, err = p.open(dev, p.O_RDWR + p.O_NONBLOCK);
if fd then
	local t, err = p.tcgetattr(fd)
	assert(t)
	local baudrate = rates[b.band(t.cflag, p.CBAUD)] or "unknown"
	print(dev .. " baudrate is " .. baudrate)
end
