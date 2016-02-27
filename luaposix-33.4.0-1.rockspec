package = "luaposix"
version = "33.4.0-1"
description = {
  detailed = "A library binding various POSIX APIs. POSIX is the IEEE Portable Operating System Interface standard. luaposix is based on lposix.",
  homepage = "http://github.com/luaposix/luaposix/",
  license = "MIT/X11",
  summary = "Lua bindings for POSIX",
}
source = {
  dir = "luaposix-release-v33.4.0",
  url = "http://github.com/luaposix/luaposix/archive/release-v33.4.0.zip",
}
dependencies = {
  "lua >= 5.1, < 5.4",
  "bit32",
}
external_dependencies = nil
build = {
  build_command = "./configure LUA='$(LUA)' LUA_INCLUDE='-I$(LUA_INCDIR)' --prefix='$(PREFIX)' --libdir='$(LIBDIR)' --datadir='$(LUADIR)' --datarootdir='$(PREFIX)' && make clean all",
  copy_directories = {},
  install_command = "make install luadir='$(LUADIR)' luaexecdir='$(LIBDIR)'",
  type = "command",
}
