-- test posix library

local bit
if _VERSION == "Lua 5.1" then bit = require 'bit' else bit = require 'bit32' end

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
function test_fork (use_table)
  local pid=assert(ox.fork())
  if pid==0 then
    pid=ox.getpid"pid"
    local ppid=ox.getpid"ppid"
    io.write("in child process ",pid," from ",ppid,".\nnow executing date... ")
    io.flush()
    assert(ox.execp("date",use_table and {"+[%c]"} or "+[%c]"))
    print"should not get here"
  else
    io.write("process ",ox.getpid"pid"," forked child process ",pid,". waiting...\n")
    local p,msg,ret = ox.wait(pid)
    assert(p == pid and msg == "exited" and ret == 0)
    io.write("child process ",pid," done\n")
  end
end
test_fork(false) -- test passing command arguments as scalars
test_fork(true) -- test passing command arguments in a table
-- FIXME: test setting argv[0]

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
testing"basename, dirname"
local s = "/foo/bar"
assert(ox.basename(s)=="bar")
assert(ox.dirname(s)=="/foo")

------------------------------------------------------------------------------
testing"statvfs"
local st = ox.statvfs("/")
print("bsize=" .. st.bsize)
print("frsize=" .. st.frsize)
print("blocks=" .. st.blocks)
print("bfree=" .. st.bfree)
print("bavail=" .. st.bavail)
print("files=" .. st.files)
print("ffree=" .. st.ffree)
print("favail=" .. st.favail)
print("fsid=" .. st.fsid)
print("flag=" .. st.flag)
print("namemax=" .. st.namemax)
------------------------------------------------------------------------------
testing"fnmatch"
assert(ox.fnmatch("test", "test"))
assert(ox.fnmatch("tes*", "test"))
assert(ox.fnmatch("tes*", "tes"))
assert(ox.fnmatch("tes*", "test2"))
assert(ox.fnmatch("tes?", "test"))
assert(ox.fnmatch("tes?", "tes") == false)
assert(ox.fnmatch("tes?", "test2") == false)
assert(ox.fnmatch("*test", "/test", ox.FNM_PATHNAME) == false)
assert(ox.fnmatch("*test", ".test", ox.FNM_PERIOD) == false)

------------------------------------------------------------------------------
testing"mkdtemp,mkstemp,glob,isatty"
local tmpdir=ox.getenv("TMPDIR") or "/tmp"
local testdir,err=ox.mkdtemp(tmpdir.."/luaposix-test-XXXXXX")
assert(testdir, err)
local st=ox.stat(testdir)
assert(st.type=="directory")
assert(st.mode=="rwx------")

-- test mkstemp() ...
local filename_template=testdir.."/test.XXXXXX"
local first_fd,first_filename=ox.mkstemp(filename_template)
assert(first_fd, first_filename) -- on error, first_filename contain error

-- ... ensure fd was connected to _this_ filename, write something to fd ...
ox.write(first_fd, "12345")
ox.close(first_fd)
st=ox.stat(first_filename)
assert(st.mode=="rw-------")
assert(st.size==5)

-- ... then read and compare
first_fd, err =ox.open(first_filename, ox.O_RDONLY)
assert(first_fd, err)
assert(not ox.isatty(first_fd))
assert(ox.read(first_fd, 5) == "12345")
local second_fd,second_filename=ox.mkstemp(filename_template)
assert(second_filename)
assert(second_filename~=first_filename)
st=ox.stat(second_filename)
assert(st.mode=="rw-------")

-- clean up after tests
ox.close(first_fd)
ox.close(second_fd)

-- create extra empty file, to check glob()
local extra_filename=testdir.."/extra_file"
local extra_file_fd, err=ox.open(extra_filename, bit.bor (ox.O_RDWR, ox.O_CREAT), "r--------")
assert(extra_file_fd, err)
ox.close(extra_file_fd)

local saved_cwd=ox.getcwd()
ox.chdir(testdir)
local globlist, err = ox.glob("test.*")
assert(globlist, err)
for _,f in pairs(globlist) do
  local T=assert(ox.stat(f))
  local p=assert(ox.getpasswd(T.uid))
  local g=assert(ox.getgroup(T.gid))
  assert(ox.basename(f):sub(1,5) == "test.") -- ensure extra_file NOT included
  print(T.mode,p.name.."/"..g.name,T.size,os.date("%b %d %H:%M",T.mtime),f,T.type)
end
ox.chdir(saved_cwd)
ox.unlink(extra_filename)
ox.unlink(first_filename)
ox.unlink(second_filename)
ox.rmdir(testdir)
st=ox.stat(testdir)
assert(st==nil) -- ensure directory is removed

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
testing"pseudoterminal"
do
    local master = assert(ox.openpt(ox.O_RDWR+ox.O_NOCTTY))
    assert(ox.grantpt(master))
    assert(ox.unlockpt(master))
    local slavename = assert(ox.ptsname(master))
    local slave = assert(ox.open(slavename, ox.O_RDWR+ox.O_NOCTTY))
    ox.close(slave)
    ox.close(master)
end

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
ox.setenv("USER","root")
f(ox.getenv"USER")

------------------------------------------------------------------------------
testing"sysconf"
local function prtab(a) for k,v in pairs(a) do print( k, v ); end end
prtab( ox.sysconf() );
testing"pathconf"
prtab( ox.pathconf(".") );

------------------------------------------------------------------------------
testing "pipe"
local rpipe, wpipe = ox.pipe()
ox.write(wpipe, "test")
local testdata = ox.read(rpipe, 4)
assert(testdata == "test")
ox.close(rpipe)
ox.close(wpipe)
------------------------------------------------------------------------------
testing "crypt"
local r = ox.crypt("hello", "pl")
assert(r == ox.crypt("hello", "pl"))
print(r)

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
testing"gettimeofday"
x=ox.gettimeofday()
for k,v in pairs(x) do print(k,v) end
y=ox.timeradd(x,{usec=999999})
assert (ox.timercmp(x,x) == 0)
assert (ox.timercmp(x,y) < 0)
assert (ox.timercmp(y,x) > 0)
y=ox.timersub(y,{usec=999999})
assert (ox.timercmp(x,y) == 0)

------------------------------------------------------------------------------
testing"msgget/msgsnd/msgrcv"
mq, err, errno = ox.msgget(100, bit.bor(ox.IPC_CREAT, ox.IPC_EXCL), "rwxrwxrwx")
if errno == ox.EEXIST then
	mq, err = ox.msgget(100, 0, "rwxrwxrwx")
end
assert (mq, err)
a, err = ox.msgsnd(mq, 42, 'Answer to the Ultimate Question of Life')
assert(a, err)
mtype, mtext, err = ox.msgrcv(mq, 128)
assert(mtype == 42)
assert(mtext == 'Answer to the Ultimate Question of Life')
assert(err == nil)

------------------------------------------------------------------------------
io.stderr:write("\n\n==== ", ox.version, " tests completed ====\n\n")
