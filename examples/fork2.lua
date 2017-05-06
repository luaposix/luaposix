local posix = require "posix"

print ("parent: my pid is: " .. posix.getpid ("pid"))

local pid = posix.fork ()

if pid == -1 then
  print ("parent: The fork failed.")
elseif pid == 0 then
  print ("child: Hello World! I am pid: " .. posix.getpid ("pid"))
  print ("child: I'll sleep for 1 second ... ")
  posix.sleep (1)
  print ("child: Good bye");
else
  print ("parent: While the child sleeps, I'm still running.")
  print ("parent: waiting for child (pid:" .. pid .. ") to die...")
  posix.wait (pid)
  print ("parent: child died, but I'm still alive.")
  print ("parent: Good bye")
end
