-- Rockspec data

-- Variables to be interpolated:
--
-- package
-- version

local version_dashed = version:gsub ("%.", "-")

local default = {
  package = package_name,
  version = version.."-1",
  source = {
    url = "git://github.com/luaposix/"..package..".git",
  },
  description = {
    summary = "Lua bindings for POSIX (including curses)",
    detailed = [[
      A library binding various POSIX APIs, including curses.
      POSIX is the IEEE Portable Operating System Interface standard.
      luaposix is based on lposix and lcurses.
     ]],
    homepage = "http://github.com/luaposix/"..package.."/",
    license = "MIT/X11",
  },
  dependencies = {
    "lua >= 5.1",
    "bit32",
  },
  build = {
    type = "command",
    build_command = "LUA=$(LUA) LUA_INCLUDE=-I$(LUA_INCDIR) ./configure --prefix=$(PREFIX) --libdir=$(LIBDIR) --datadir=$(LUADIR) && make clean && make",
    install_command = "make install",
    copy_directories = {},
  },
}

if version ~= "git" then
  default.source.branch = "release-v"..version_dashed
else
  default.build.build_command = "./bootstrap && " .. default.build.build_command
  table.insert (default.dependencies, "ldoc")
end

return {default=default, [""]={}}
