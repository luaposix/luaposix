local P = require 'posix'
for k,v in pairs(P) do _G[k] = v end

function go(fn,...)
  local cpid = P.fork()
  if cpid == 0 then -- run function as child
    local res = fn(...)
    P._exit(res or 0)
  else
    return cpid
  end
end

local verbose = #arg > 0

function sleepx(secs)
  while true do
    secs = sleep(secs)
    if verbose then print('sleep',secs) end
    if secs == 0 then return end
  end
end

local nchild, nsig = 0,0

signal(SIGCHLD,function()
         local  pid,status,code = wait(-1,WNOHANG)
         while pid do
           if pid ~= 0 then
             if verbose then print('wait',pid,status,code) end
             nchild = nchild + 1
           end
           pid,status,code = wait(-1,WNOHANG)
         end
               end)

function handler(signo)
  if verbose then print('handled',signo) end
  nsig = nsig + 1
end

signal(SIGUSR1,handler)
signal(SIGUSR2,handler)
signal(60,handler)

function killp(nsig) kill(getpid 'ppid',nsig) end

c1 = go(function() sleep(1); killp(SIGUSR1); killp(SIGUSR2) end)
c2 = go(function() sleep(2); killp(SIGUSR2); end)
c3 = go(function() sleep(2); killp(SIGUSR1) end)

sleepx(3)

if verbose then print('children',nchild,'signals',nsig)
else
  assert (nchild == 3)
  assert (nsig ==  4)
  print '+++ tests OK +++'
end
