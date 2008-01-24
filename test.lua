-- test posix library

function testing(s)
 print""
 print("-------------------------------------------------------",s)
end

function myassert(w,c,s)
 if not c then print(w,s) end
 return c
end

function myprint(s,...)
 for i=1,table.getn(arg) do
  io.write(arg[i],s)
 end
 io.write"\n"
end

ox=posix

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
for k in ox.getenv() do io.write(k,"\t") end io.write"\n"

------------------------------------------------------------------------------
testing"putenv"
function f(s) print("putenv",s,ox.putenv(s)) end
function g(v) print("now",v,ox.getenv(v)) end
f"MYVAR=123"	g"MYVAR"
f"MYVAR="	g"MYVAR"
f"MYVAR"	g"MYVAR"

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
testing"fork, exec"
io.flush()
pid=assert(ox.fork())
if pid==0 then
	pid=ox.getprocessid"pid"
	ppid=ox.getprocessid"ppid"
	io.write("in child process ",pid," from ",ppid,".\nnow executing date... ")
	io.flush()
	assert(ox.exec("date","+[%c]"))
	print"should not get here"
else
	io.write("process ",ox.getprocessid"pid"," forked child process ",pid,". waiting...\n")
	ox.wait(pid)
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
testing"umask"
-- assert(not ox.access("xxx"),"`xxx' already exists")
ox.unlink"xxx"
print(ox.umask())
print(ox.umask("a-r,o+w"))
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
os.execute"ls -l xxx test.lua"
a=ox.stat"test.lua"
ox.utime("xxx",a.mtime)
os.execute"ls -l xxx test.lua"
ox.unlink"xxx"

------------------------------------------------------------------------------
testing"links"
ox.unlink"xxx"
io.close(io.open("xxx","w"))
print(ox.link("xxx","yyy"))
print(ox.symlink("xxx","zzz"))
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
   myprint(":",a.name,a.passwd,a.uid,a.gid,a.gecos,a.dir,a.shell)
 end
end

f()
f(ox.getenv"USER")
f(ox.getenv"LOGNAME")
f"root"
f(0)
f(1234567)
f"xxx"
function f(x) print(ox.getpasswd(x,"name"),ox.getpasswd(x,"gecos")) end
f()
f(nil)
ox.putenv"USER=root"
f(ox.getenv"USER")

------------------------------------------------------------------------------
testing"sysconf"
a=ox.sysconf() table.foreach(a,print)
testing"pathconf"
a=ox.pathconf(".") table.foreach(a,print)

------------------------------------------------------------------------------
testing"times"
a=ox.times()
for k,v in a do print(k,v) end
print"sleeping 10 seconds..."
ox.sleep(10)
b=ox.times()
for k,v in b do print(k,v) end
print""
print("elapsed",b.elapsed-a.elapsed)
print("clock",os.clock())

------------------------------------------------------------------------------
print""
print(ox.version)
