-- limit.lua
-- Limiting the CPU time used by a child process;
-- it will be killed and we don't get the final message

local posix = require 'posix'

posix.setrlimit ('cpu',1,1)

local t = posix.times 'elapsed'

local pid = posix.fork ()
if pid == 0 then -- child
  print 'start'
  for i = 1, 1e9 do
  end
  print 'finish'
else
  print (posix.wait (pid))
  print (posix.times 'elapsed' - t)
end
