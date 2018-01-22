#! /usr/bin/env lua

local M = require 'posix.termio'


local F = require 'posix.fcntl'
local U = require 'posix.unistd'


local dev = arg[1] or '/dev/ttyUSB0'

-- Open serial port and do settings
local fds, err = F.open(dev, F.O_RDWR + F.O_NONBLOCK)
if not fds then
   print('Could not open serial port ' .. dev .. ':', err)
   os.exit(1)
end

M.tcsetattr(fds, 0, {
   cflag = M.B115200 + M.CS8 + M.CLOCAL + M.CREAD,
   iflag = M.IGNPAR,
   oflag = M.OPOST,
   cc = {
      [M.VTIME] = 0,
      [M.VMIN] = 1,
   }
})

-- Set stdin to non canonical mode. Save current settings
local save = M.tcgetattr(0)
M.tcsetattr(0, 0, {
   cc = {
      [M.VTIME] = 0,
      [M.VMIN] = 1
   }
})

-- Loop, reading and writing between ports. ^C stops
local set = {
   [0] = {events={IN=true}},
   [fds] = {events={IN =true}},
}

U.write(1, 'Starting terminal, hit ^C to exit\r\n')

local function exit(msg)
   M.tcsetattr(0, 0, save)
   print '\n'
   print(msg)
   os.exit(0)
end

while true do
   local r = require 'posix.poll'.poll(set, -1)
   for fd, d in pairs(set) do
      if d.revents and d.revents.IN then
         if fd == 0 then
            local d, err = U.read(0, 1024)
            if not d then
               exit(err)
            end
            if d == string.char(3) then
               exit('Bye')
            end
            local ok, err = U.write(fds, d)
            if not ok then
               exit(err)
            end
         end
         if fd == fds then
            local d, err = U.read(fds, 1024)
            if not d then
               exit(err)
            end
            local ok, err = U.write(1, d)
            if not ok then
               exit(err)
            end
         end
      end
   end
end
