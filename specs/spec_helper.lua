local std = require "specl.std"

hell = require "specl.shell"
bit  = bit32 or require "bit"

band, bnot, bor = bit.band, bit.bnot, bit.bor

package.path  = std.package.normalize ("lib/?.lua", package.path)
package.cpath = std.package.normalize ("ext/curses/.libs/?.so",
			"ext/posix/.libs/?.so", package.cpath)

posix = require "posix"


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
