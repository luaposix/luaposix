local std     = require "specl.std"

badargs = require "specl.badargs"
hell    = require "specl.shell"
bit     = bit32 or require "bit"

band, bnot, bor = bit.band, bit.bnot, bit.bor

package.path  = std.package.normalize ("lib/?.lua", package.path)
package.cpath = std.package.normalize ("ext/curses/.libs/?.so",
			"ext/posix/.libs/?.so", package.cpath)

posix = require "posix"


-- Allow user override of LUA binary used by hell.spawn, falling
-- back to environment PATH search for "lua" if nothing else works.
local LUA = os.getenv "LUA" or "lua"


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

-- Recursively remove a temporary directory.
function rmtmp (dir)
  for f in posix.files (dir) do
    if f ~= "." and f ~= ".." then
      path = dir .. "/" .. f
      if posix.stat (path, "type") == "directory" then
        rmtmp (path)
      else
        os.remove (path)
      end
    end
  end
end

-- Create an empty file at PATH.
function touch (path) io.open (path, "w+"):close () end
