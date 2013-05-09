package = "luaposix"
version = "29-1"
description = {
  homepage = "http://github.com/luaposix/luaposix/",
  license = "MIT/X11",
  summary = "Lua bindings for POSIX (including curses)",
  detailed = "A library binding various POSIX APIs, including curses. POSIX is the IEEE Portable Operating System Interface standard. luaposix is based on lposix and lcurses.",
}
source = {
  url = "http://github.com/luaposix/luaposix/archive/release-v29.zip",
  dir = "luaposix-release-v29",
}
dependencies = {
  "lua >= 5.1",
  "luabitop >= 1.0.2",
}
external_dependencies = nil
build = {
  build_command = "./configure LUA='$(LUA)' LUA_INCLUDE='-I$(LUA_INCDIR)' --prefix='$(PREFIX)' --libdir='$(LIBDIR)' --datadir='$(LUADIR)' && make clean all",
  type = "command",
  copy_directories = {},
  install_command = "make install luadir='$(LUADIR)'",
}
