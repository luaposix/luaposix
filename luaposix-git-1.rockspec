package = "luaposix"
version = "git-1"
description = {
  detailed = "A library binding various POSIX APIs, including curses. POSIX is the IEEE Portable Operating System Interface standard. luaposix is based on lposix and lcurses.",
  homepage = "http://github.com/luaposix/luaposix/",
  license = "MIT/X11",
  summary = "Lua bindings for POSIX (including curses)",
}
source = {
  url = "git://github.com/luaposix/luaposix.git",
}
dependencies = {
  "lua >= 5.1, < 5.4",
  "bit32",
}
external_dependencies = nil
build = {
  build_command = "./bootstrap && ./configure LUA='$(LUA)' LUA_INCLUDE='-I$(LUA_INCDIR)' --prefix='$(PREFIX)' --libdir='$(LIBDIR)' --datadir='$(LUADIR)' --datarootdir='$(PREFIX)' && make clean all",
  copy_directories = {},
  install_command = "make install luadir='$(LUADIR)' luaexecdir='$(LIBDIR)'",
  type = "command",
}
