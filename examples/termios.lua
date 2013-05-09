local p = require "posix"
local dev = arg[1] or "/dev/ttyUSB0"

-- Open serial port and do settings

local fds, err = p.open(dev, p.O_RDWR + p.O_NONBLOCK);
if not fds then
	print("Could not open serial port " .. dev .. ":", err)
	os.exit(1)
end

p.tcsetattr(fds, 0, {
	cflag = p.B115200 + p.CS8 + p.CLOCAL + p.CREAD,
	iflag = p.IGNPAR,
	oflag = p.OPOST,
	cc = {
		[p.VTIME] = 0,
		[p.VMIN] = 1
	}
})

-- Set stdin to non canonical mode. Save current settings

local save = p.tcgetattr(0)
p.tcsetattr(0, 0, {
	cc = {
		[p.VTIME] = 0,
		[p.VMIN] = 1
	}
})

-- Loop, reading and writing between ports. ^C stops

local set = {
	[0] = { events = { IN = true } },
	[fds] = { events = { IN = true } },
}

p.write(1, "Starting terminal, hit ^C to exit\r\n")

local function exit(msg)
	p.tcsetattr(0, 0, save)
	print("\n")
	print(msg)
	os.exit(0)
end

while true do
	local r = p.poll(set, -1)
	for fd, d in pairs(set) do
		if d.revents and d.revents.IN then
			if fd == 0 then
				local d, err = p.read(0, 1024)
				if not d then exit(err) end
				if d == string.char(3) then exit("Bye") end
				local ok, err = p.write(fds, d)
				if not ok then exit(err) end
			end
			if fd == fds then
				local d, err = p.read(fds, 1024)
				if not d then exit(err) end
				local ok, err = p.write(1, d)
				if not ok then exit(err) end
			end
		end
	end
end
