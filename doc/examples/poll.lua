#! /usr/bin/env lua

local M = require 'posix.unistd'


local F = require 'posix.fcntl'


local fd1 = F.open(arg[1], F.O_RDONLY)
local fd2 = F.open(arg[2], F.O_RDONLY)

local fds = {
   [fd1] = {events={IN=true}},
   [fd2] = {events={IN=true}}
}

while true do
   require 'posix.poll'.poll(fds, -1)
   for fd in pairs(fds) do
      if fds[fd].revents.IN then
         local res = M.read(fd,1024)
         M.write(1,res);
      end
      if fds[fd].revents.HUP then
         M.close(fd)
         fds[fd] = nil
         if not next(fds) then
            return
         end
      end
   end
end
