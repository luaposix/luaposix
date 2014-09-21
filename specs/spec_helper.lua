local std = require "specl.std"

hell = require "specl.shell"
bit  = bit32 or require "bit"

band, bnot, bor = bit.band, bit.bnot, bit.bor

package.path  = std.package.normalize ("lib/?.lua", package.path)
package.cpath = std.package.normalize ("ext/curses/.libs/?.so",
			"ext/posix/.libs/?.so", package.cpath)

posix = require "posix"


-- Error message specifications use this to shorten argument lists.
local function bind (f, fix)
  return function (...)
	   local arg = {}
	   for i, v in ipairs (fix) do
	     arg[i] = v
	   end
	   local i = 1
	   for _, v in pairs {...} do
	     while arg[i] ~= nil do i = i + 1 end
	     arg[i] = v
	   end
	   return f (unpack (arg))
	 end
end


--- Return a formatted bad argument string.
-- @tparam table M module table
-- @string fname base-name of the erroring function
-- @int i argument number
-- @string want expected argument type
-- @string[opt] field name of field being type checked
-- @string[opt="no value"] got actual argument type
-- @usage
--   expect (f ()).to_error (badarg (fname, mname, 1, "function"))
local function badarg (mname, fname, i, want, field, got)
  if got == nil then field, got = nil, field end  -- field is optional
  if want == nil then i, want = i - 1, i end      -- numbers only for narg error

  local fqfname = (mname .. "." .. fname):gsub ("^%.", "")

  if got == nil and type (want) == "number" then
    local s = "bad argument #%d to '%s' (no more than %d arguments expected, got %d)"
    return s:format (i + 1, fqfname, i, want)
  elseif field ~= nil then
    local s = "bad argument #%d to '%s' (%s expected for field '%s', got %s)"
    return s:format (i, fqfname, want, field, got or "no value")
  end
  return string.format ("bad argument #%d to '%s' (%s expected, got %s)",
                        i, fqfname, want, got or "no value")
end


--- Initialise custom function and argument error handlers.
-- @tparam table M module table
-- @string fname function name to bind
-- @treturn string *fname*
-- @treturn function badarg with *M* and *fname* prebound
-- @treturn function toomanyarg with *M* and *fname* prebound
-- @usage
--   f, badarg, toomanyarg = init (M, "posix", "bind")
function init (M, mname, fname)
  return M[fname], bind (badarg, {mname, fname})
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
