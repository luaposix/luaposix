--- Lua bindings for curses
module ("curses", package.seeall)

require "curses_c"

-- These Lua functions detect number of args, like Unified Funcs in Perl Curses
-- see http://pjb.com.au/comp/lua/lcurses.html
-- see http://search.cpan.org/perldoc?Curses

function addch (...)
  if #{...} == 3 then
    return curses.stdscr():mvaddch(...)
  else
    return curses.stdscr():addch(...)
  end
end

function addstr(...) -- detect number of args, like Unified Funcs in Perl Curses
  if #{...} == 3 then
    return curses.stdscr():mvaddstr(...)
  else
    return curses.stdscr():addstr(...)
  end
end

function attrset (a) return curses.stdscr():attrset(a) end
function clear ()    return curses.stdscr():clear() end
function clrtobot () return curses.stdscr():clrtobot() end
function clrtoeol () return curses.stdscr():clrtoeol() end

function getch (...)
  local c
  if #{...} == 2 then
    c = curses.stdscr():mvgetch(...)
  else
    c = curses.stdscr():getch()
  end
  if c < 256 then
    return string.char(c)
  end
  -- could kludge-test for utf8, e.g. c3 a9 20  c3 aa 20  c3 ab 20  e2 82 ac 0a
  return c
end

function getstr (...)
  if #{...} > 1 then
    return curses.stdscr():mvgetstr(...)
  else
    return curses.stdscr():getstr(...)
  end
end
getnstr = getstr

function getyx ()    return curses.stdscr():getyx() end
function keypad (b)  return curses.stdscr():keypad(b) end
function move (y,x)  return curses.stdscr():move(y,x) end
function refresh ()  return curses.stdscr():refresh() end
function timeout (t) return curses.stdscr():timeout(t) end
