local curses = require "curses"


local function printf (fmt, ...)
  return print (string.format (fmt, ...))
end


local function main ()
  local stdscr = curses.initscr ()

  curses.cbreak ()
  curses.echo (false)	-- not noecho !
  curses.nl (false)	-- not nonl !

  stdscr:clear ()

  local a = {}
  for k, v in pairs (curses) do
    if type (v) == "number" then a[#a+1] = k end
  end

  stdscr:mvaddstr (15, 20, "print out curses constants (y/n) ? ")
  stdscr:refresh ()

  local c = stdscr:getch ()
  if c < 256 then c = string.char (c) end

  curses.endwin ()

  if c == "y" then
    table.sort (a, cmp)
    for i, k in ipairs (a) do
      printf (" %03d. %20s = 0x%08x (%d)",
        i, "curses." .. k, curses[k], curses[k])
    end
  end
end


-- To display Lua errors, we must close curses to return to
-- normal terminal mode, and then write the error to stdout.
local function err (err)
  curses.endwin ()
  print "Caught an error:"
  print (debug.traceback (err, 2))
  os.exit (2)
end

xpcall (main, err)
