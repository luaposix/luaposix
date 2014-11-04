local posix = require "posix"

local M = {}

-- For backwards compatibility with release 32, copy previous
-- entries into `posix.sys` namespace.
for _, fn in ipairs {
  "euidaccess", "pipeline", "pipeline_iterator", "pipeline_slurp",
  "spawn", "timeradd", "timercmp", "timersub"
} do
  M[fn] = posix[fn]
end

return M
