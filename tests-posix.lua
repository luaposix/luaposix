-- test posix library

local ox = require 'posix'

function testing(s)
 print""
 print("-------------------------------------------------------",s)
end

function myassert(w,c,s)
 if not c then print(w,s) end
 return c
end

function myprint(s,...)
 for i=1,#arg do
  io.write(arg[i],s)
 end
 io.write"\n"
end


------------------------------------------------------------------------------
print(ox.version)

------------------------------------------------------------------------------
testing"uname"
function f(x) print(ox.uname(x)) end
f()
f("Machine %n is a %m running %s %r")

------------------------------------------------------------------------------
testing"terminal"
print(ox.getlogin(),ox.ttyname(),ox.ctermid(),ox.getenv"TERM")
print(ox.getlogin(),ox.ttyname(1),ox.ctermid(),ox.getenv"TERM")
print(ox.getlogin(),ox.ttyname(2),ox.ctermid(),ox.getenv"TERM")

------------------------------------------------------------------------------
testing"getenv"
function f(v) print(v,ox.getenv(v)) end
f"USER"
f"HOME"
f"SHELL"
f"absent"
for k,v in pairs(ox.getenv()) do print(k,'=',v) end io.write"\n"

------------------------------------------------------------------------------
testing"setenv"
function f(n,v) print("setenv",n,'=',v) ox.setenv(n,v) end
function g(n) print("now",n,'=',ox.getenv(n)) end
f("MYVAR","123");	g"MYVAR"
f("MYVAR",nil);     g"MYVAR"
--f"MYVAR"	g"MYVAR"

------------------------------------------------------------------------------
testing"getcwd, chdir, mkdir, rmdir"
function f(d) myassert("chdir",ox.chdir(d)) g() end
function g() local d=ox.getcwd() print("now at",d) return d end
myassert("rmdir",ox.rmdir"x")
d=g()
f".."
f"xxx"
f"/etc/uucp"
f"/var/spool/mqueue"
f(d)
assert(ox.mkdir"x")
f"x"
f"../x"
assert("rmdir",ox.rmdir"../x")
myassert("mkdir",ox.mkdir".")
myassert("rmdir",ox.rmdir".")
g()
f(d)
myassert("rmdir",ox.rmdir"x")

------------------------------------------------------------------------------
testing"fork, execp"
io.flush()
pid=assert(ox.fork())
if pid==0 then
  pid=ox.getpid"pid"
  ppid=ox.getpid"ppid"
  io.write("in child process ",pid," from ",ppid,".\nnow executing date... ")
  io.flush()
  assert(ox.execp("date","+[%c]"))
  print"should not get here"
else
  io.write("process ",ox.getpid"pid"," forked child process ",pid,". waiting...\n")
  p,msg,ret = ox.wait(pid)
  assert(p == pid and msg == "exited" and ret == 0)
  io.write("child process ",pid," done\n")
end

------------------------------------------------------------------------------
testing"dir, stat"
function g() local d=ox.getcwd() print("now at",d) return d end
g()
for f in ox.files"." do
  local T=assert(ox.stat(f))
  local p=assert(ox.getpasswd(T.uid))
  local g=assert(ox.getgroup(T.gid))
  print(T.mode,p.name.."/"..g.name,T.size,os.date("%b %d %H:%M",T.mtime),f,T.type)
end

------------------------------------------------------------------------------
testing"glob"
function g() local d=ox.getcwd() print("now at",d) return d end
g()
for _,f in pairs(ox.glob "*.o") do
  local T=assert(ox.stat(f))
  local p=assert(ox.getpasswd(T.uid))
  local g=assert(ox.getgroup(T.gid))
  print(T.mode,p.name.."/"..g.name,T.size,os.date("%b %d %H:%M",T.mtime),f,T.type)
end

------------------------------------------------------------------------------
testing"umask"
-- assert(not ox.access("xxx"),"`xxx' already exists")
ox.unlink"xxx"
print(ox.umask())
--print(ox.umask("a-r,o+w"))
print(ox.umask("a=r"))
print(ox.umask("o+w"))
io.close(io.open("xxx","w"))
os.execute"ls -l xxx"
ox.unlink"xxx"

------------------------------------------------------------------------------
testing"chmod, access"
ox.unlink"xxx"
print(ox.access("xxx"))
io.close(io.open("xxx","w"))
print(ox.access("xxx"))
os.execute"ls -l xxx"
print(ox.access("xxx","r"))
assert(ox.chmod("xxx","a-rwx,o+x"))
print(ox.access("xxx","r"))
os.execute"ls -l xxx"
ox.unlink"xxx"

------------------------------------------------------------------------------
testing"utime"
io.close(io.open("xxx","w"))
io.flush()
--os.execute"ls -l xxx test.lua"
os.execute( string.format("ls -l xxx %s", arg[0]))
--a=ox.stat"test.lua"
a=ox.stat( arg[0] )
ox.utime("xxx",a.mtime)
os.execute( string.format("ls -l xxx %s", arg[0]))
ox.unlink"xxx"
io.flush()

------------------------------------------------------------------------------
testing"links"
ox.unlink"xxx"
io.close(io.open("xxx","w"))
print(ox.link("xxx","yyy"))       --> hardlink
print(ox.link("xxx","zzz", true)) --> symlink
os.execute"ls -l xxx yyy zzz"
print("zzz ->",ox.readlink"zzz")
print("zzz ->",ox.readlink"xxx")
ox.unlink"xxx"
ox.unlink"yyy"
ox.unlink"zzz"

------------------------------------------------------------------------------
testing"getpasswd"
function f(x)
 local a
 if x==nil then a=ox.getpasswd() else a=ox.getpasswd(x) end
 if a==nil then
   print(x,"no such user")
  else
   myprint(":",a.name,a.passwd,a.uid,a.gid,a.dir,a.shell)
 end
end

f()
f(ox.getenv"USER")
f(ox.getenv"LOGNAME")
f"root"
f(0)
f(1234567)
f"xxx"
function f(x) print(ox.getpasswd(x,"name"),ox.getpasswd(x,"shell")) end
f()
f(nil)
--ox.putenv"USER=root"
ox.setenv("USER","root")
f(ox.getenv"USER")

------------------------------------------------------------------------------
testing"sysconf"
local function prtab(a) for k,v in pairs(a) do print( k, v ); end end
prtab( ox.sysconf() );
testing"pathconf"
prtab( ox.pathconf(".") );

------------------------------------------------------------------------------
if arg[1] ~= "--no-times" then
  testing"times"
  a=ox.times()
  for k,v in pairs(a) do print(k,v) end
  print"sleeping 1 second..."
  ox.sleep(1)
  b=ox.times()
  for k,v in pairs(b) do print(k,v) end
  print""
  print("elapsed",b.elapsed-a.elapsed)
  print("clock",os.clock())
end

------------------------------------------------------------------------------
io.stderr:write("\n\n==== ", ox.version, " tests completed ====\n\n")
