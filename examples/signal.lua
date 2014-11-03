local sig     = require "posix.signal"
local unistd  = require "posix.unistd"
local syswait = require "posix.sys.wait"

local function go (fn, ...)
  local cpid = unistd.fork ()
  if cpid == 0 then -- run function as child
    unistd._exit (fn (...) or 0)
  else
    return cpid
  end
end

local verbose = #arg > 0

local function sleepx (secs)
  while true do
    secs = unistd.sleep (secs)
    if verbose then print ("sleep", secs) end
    if secs == 0 then return end
  end
end

local nchild, nsig = 0, 0

sig.signal (sig.SIGCHLD, function()
  local pid, status, code = syswait.wait (-1, syswait.WNOHANG)
  while pid do
    if pid ~= 0 then
      if verbose then print ("wait", pid, status, code) end
      nchild = nchild + 1
    end
    pid, status, code = syswait.wait (-1, syswait.WNOHANG)
  end
end)

local function handler (signo)
  if verbose then print ("handled", signo) end
  nsig = nsig + 1
end

sig.signal (sig.SIGUSR1, handler)
sig.signal (sig.SIGUSR2, handler)
sig.signal (60, handler)

local function killp (nsig)
  return sig.kill (unistd.getppid (), nsig)
end

c1 = go (function() unistd.sleep (1); killp (sig.SIGUSR1); killp (sig.SIGUSR2) end)
c2 = go (function() unistd.sleep (2); killp (sig.SIGUSR2); end)
c3 = go (function() unistd.sleep (2); killp (sig.SIGUSR1) end)

sleepx(3)

if verbose then
  print ("children", nchild, "signals", nsig)
else
  assert (nchild == 3)
  assert (nsig ==  4)
  print '+++ tests OK +++'
end
