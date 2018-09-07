local posix = require 'posix'

local M = {}

-- For backwards compatibility with release 32, copy previous
-- entries into `posix.sys` namespace.
local fns = {
   'euidaccess', 'pipeline', 'pipeline_iterator', 'pipeline_slurp',
   'spawn', 'timeradd', 'timercmp', 'timersub'
}

for i = 1, #fns do
   local fn = fns[i]
   M[fn] = posix[fn]
end

return M
