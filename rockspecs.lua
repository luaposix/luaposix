-- Rockspec data

-- Variables to be interpolated:
--
-- package
-- version

local default = {
  package = package_name,
  version = version.."-1",
  source = {
    url = "git://github.com/luaposix/"..package_name..".git",
  },
  description = {
    summary = "Lua bindings for POSIX (including curses)",
    detailed = [[
      A library binding various POSIX APIs, including curses.
      POSIX is the IEEE Portable Operating System Interface standard.
      luaposix is based on lposix and lcurses.
     ]],
    homepage = "http://github.com/luaposix/"..package_name.."/",
    license = "MIT/X11",
  },
  dependencies = {
    "lua >= 5.1",
    "luabitop >= 1.0.2",
  },
  build = {
    type = "command",
    build_command = "LUA=$(LUA) LUA_INCLUDE=-I$(LUA_INCDIR) ./configure --prefix=$(PREFIX) --libdir=$(LIBDIR) --datadir=$(LUADIR) && make clean && make",
    install_command = "make install",
    copy_directories = {},
  },
}

if version ~= "git" then
  default.source.branch = "release-v"..version
else
  default.build.build_command = "./bootstrap && " .. default.build.build_command
  table.insert (default.dependencies, "ldoc")
end

return {default=default, [""]={}}
