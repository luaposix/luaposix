local P = require 'posix'

local byte = string.byte

local short = "ha:s:"
local help, aleph, start = byte 'h', byte 'a', byte 's'
local que = byte '?'

local long = {
  {"help",  "none", help},
  {"aleph", "required", aleph},
  {"start", "required", start}
}

local last_index = 1
for r, li, optind, optarg in P.getopt_long (arg, short, long) do
  if r == que then
    return print  'unrecognized option'
  end
  last_index = optind
  if r == help then
    print 'help'
  elseif r == aleph or r == start then
    print ('we were passed', r, optarg)
  end
end

for i = last_index, #arg do
  print (i, arg[i])
end
