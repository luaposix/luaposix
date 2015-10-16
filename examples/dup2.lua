local posix = require "posix"

local unistd = require "posix.unistd"
local sys_wait = require "posix.sys.wait"


local stdout_r, stdout_w = unistd.pipe ()
assert (stdout_r ~= nil, stdout_w)

local stderr_r, stderr_w = unistd.pipe ()
assert (stderr_r ~= nil, stderr_w)

local pid, errmsg = unistd.fork ()
assert (pid ~= nil, errmsg)

if pid == 0 then
  unistd.close (stdout_r)
  unistd.close (stderr_r)

  unistd.dup2 (stdout_w, unistd.STDOUT_FILENO)
  unistd.dup2 (stderr_w, unistd.STDERR_FILENO)

  -- Exec() a subprocess here instead if you like --

  io.stdout:write "output string"
  io.stderr:write "oh noes!"
  os.exit (42)
end

unistd.close (stdout_w)
unistd.close (stderr_w)

local outs, errmsg = unistd.read (stdout_r, 1024)
assert (outs ~= nil, errmsg)
print ("STDOUT:", outs)

local errs, errmsg = unistd.read (stderr_r, 1024)
assert (errs ~= nil, errmsg)
print ("STDERR:", errs)

local pid, reason, status = sys_wait.wait (pid)
assert (pid ~= nil, reason)
print ("child " .. reason, status)
