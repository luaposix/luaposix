package = 'luaposix'
version = '34.0-1'

description = {
  summary = 'Lua bindings for POSIX',
  detailed = [[
    A library binding various POSIX APIs. POSIX is the IEEE Portable
    Operating System Interface standard. luaposix is based on lposix.
  ]],
  homepage = 'http://github.com/luaposix/luaposix/',
  license = 'MIT/X11',
}

source = {
  dir = 'luaposix-34.0',
  url = 'http://github.com/luaposix/luaposix/archive/v34.0.zip',
}

dependencies = {
  'lua >= 5.1, < 5.4',
  'bit32',
}

build = {
  type = 'command',
  build_command = 'build-aux/luke'
    .. ' package="' .. package .. '"'
    .. ' version="' .. version .. '"'
    .. ' PREFIX="$(PREFIX)"'
    .. ' LUA="$(LUA)"'
    .. ' LUA_INCDIR="$(LUA_INCDIR)"'
    .. ' CFLAGS="$(CFLAGS)"'
    .. ' LIBFLAG="$(LIBFLAG)"'
    .. ' LIB_EXTENSION="$(LIB_EXTENSION)"'
    .. ' OBJ_EXTENSION="$(OBJ_EXTENSION)"'
    .. ' INST_LIBDIR="$(LIBDIR)"'
    .. ' INST_LUADIR="$(LUADIR)"'
    ,
  install_command = 'build-aux/luke install --quiet'
    .. ' INST_LIBDIR="$(LIBDIR)"'
    .. ' INST_LUADIR="$(LUADIR)"'
    ,
  copy_directories = {'doc'},
}
