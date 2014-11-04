local std     = require "specl.std"

badargs = require "specl.badargs"
hell    = require "specl.shell"

local top_srcdir = os.getenv "top_srcdir" or "."
local top_builddir = os.getenv "top_builddir" or "."

package.path  = std.package.normalize (
		  top_builddir .. "/lib/?.lua",
		  top_srcdir .. "/lib/?.lua",
		  top_builddir .. "/lib/?/init.lua",
		  top_srcdir .. "/lib/?/init.lua",
		  package.path)
package.cpath = std.package.normalize (
		  top_builddir .. "/ext/posix/.libs/?.so",
		  top_srcdir .. "/ext/posix/.libs/?.so",
		  top_builddir .. "/ext/posix/_libs/?.dll",
		  top_srcdir .. "/ext/posix/_libs/?.dll",
		  package.cpath)

posix = require "posix"

bit   = bit32 or require "bit"

band, bnot, bor = bit.band, bit.bnot, bit.bor


-- Allow user override of LUA binary used by hell.spawn, falling
-- back to environment PATH search for "lua" if nothing else works.
local LUA = os.getenv "LUA" or "lua"


-- Easily check for std.object.type compatibility.
function prototype (o)
  return (getmetatable (o) or {})._type or io.type (o) or type (o)
end


local function mkscript (code)
  local f = os.tmpname ()
  local h = io.open (f, "w")
  h:write (code)
  h:close ()
  return f
end


--- Run some Lua code with the given arguments and input.
-- @string code valid Lua code
-- @tparam[opt={}] string|table arg single argument, or table of
--   arguments for the script invocation
-- @string[opt] stdin standard input contents for the script process
-- @treturn specl.shell.Process|nil status of resulting process if
--   execution was successful, otherwise nil
function luaproc (code, arg, stdin)
  local f = mkscript (code)
  if type (arg) ~= "table" then arg = {arg} end
  local cmd = {LUA, f, unpack (arg)}
  -- inject env and stdin keys separately to avoid truncating `...` in
  -- cmd constructor
  cmd.stdin = stdin
  cmd.env = {
    LUA_CPATH    = package.cpath,
    LUA_PATH     = package.path,
    LUA_INIT     = "",
    LUA_INIT_5_2 = ""
  }
  local proc = hell.spawn (cmd)
  os.remove (f)
  return proc
end


-- Use a consistent template for all temporary files.
TMPDIR = posix.getenv ("TMPDIR") or "/tmp"
template = TMPDIR .. "/luaposix-test-XXXXXX"

-- Allow comparison against the error message of a function call result.
function Emsg (_, msg) return msg or "" end

-- Collect stdout from a shell command, and strip surrounding whitespace.
function cmd_output (cmd)
  return hell.spawn (cmd).output:gsub ("^%s+", ""):gsub ("%s+$", "")
end


local st = require "posix.sys.stat"
local stat, S_ISDIR = st.lstat, st.S_ISDIR

-- Recursively remove a temporary directory.
function rmtmp (dir)
  for f in posix.files (dir) do
    if f ~= "." and f ~= ".." then
      local path = dir .. "/" .. f
      if S_ISDIR (stat (path).st_mode) ~= 0 then
        rmtmp (path)
      else
        os.remove (path)
      end
    end
  end
  os.remove (dir)
end


-- Create an empty file at PATH.
function touch (path) io.open (path, "w+"):close () end
