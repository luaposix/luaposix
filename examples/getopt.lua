local P = require 'posix'

local short = "ha:s:"

local long = {
  {"help",  "none", 'h'},
  {"aleph", "required", 'a'},
  {"start", "required", 's'}
}

local last_index = 1
for r, optarg, optind, li in P.getopt (arg, short, long) do
  if r == '?' then
    return print  'unrecognized option'
  end
  last_index = optind
  if r == 'h' then
    print 'help'
  elseif r == 'a' or r == 's' then
    print ('we were passed', r, optarg)
  end
end

for i = last_index, #arg do
  print (i, arg[i])
end
