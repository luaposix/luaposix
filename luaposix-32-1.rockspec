package = "luaposix"
version = "32-1"
description = {
  detailed = "A library binding various POSIX APIs, including curses. POSIX is the IEEE Portable Operating System Interface standard. luaposix is based on lposix and lcurses.",
  homepage = "http://github.com/luaposix/luaposix/",
  license = "MIT/X11",
  summary = "Lua bindings for POSIX (including curses)",
}
source = {
  dir = "luaposix-release-v32",
  url = "http://github.com/luaposix/luaposix/archive/release-v32.zip",
}
dependencies = {
  "lua >= 5.1",
  "luabitop >= 1.0.2",
}
external_dependencies = nil
build = {
  build_command = "./configure LUA='$(LUA)' LUA_INCLUDE='-I$(LUA_INCDIR)' --prefix='$(PREFIX)' --libdir='$(LIBDIR)' --datadir='$(LUADIR)' --datarootdir='$(PREFIX)' && make clean all",
  copy_directories = {},
  install_command = "make install luadir='$(LUADIR)'",
  type = "command",
}
