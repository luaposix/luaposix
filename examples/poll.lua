local P = require 'posix'

local fd1 = P.open(arg[1], P.O_RDONLY)
local fd2 = P.open(arg[2], P.O_RDONLY)

local fds = {
  [fd1] = { events = {IN=true} },
  [fd2] = { events = {IN=true} }
}

while true do
  P.poll(fds,-1)
  for fd in pairs(fds) do
    if  fds[fd].revents.IN then
      local res = P.read(fd,1024)
      P.write(1,res);
    end
    if  fds[fd].revents.HUP then
      P.close(fd)
      fds[fd] = nil
      if not next(fds) then return end
    end
  end
end
