local _MODREV, _SPECREV = 'dev', '-1'

rockspec_format = '3.0'
package = 'luaposix'
version = _MODREV .. _SPECREV

description = {
   summary = 'Lua bindings for POSIX',
   detailed = [[
      A library binding various POSIX APIs. POSIX is the IEEE Portable
      Operating System Interface standard. luaposix is based on lposix.
   ]],
   homepage = 'https://github.com/luaposix/luaposix/',
   license = 'MIT/X11',
}

dependencies = {
   'lua >= 5.1, < 5.6',
}

do
   -- We only want to install a bit32 module for Lua 5.1.
   local _ENV={package=nil, dependencies=dependencies}
   if package then
      dependencies[#dependencies + 1] = 'bit32'
   end
end

source = {
   url = 'https://github.com/luaposix/luaposix/archive/v' .. _MODREV .. '.zip',
   dir = 'luaposix-' .. _MODREV,  
}

build = {
   type = 'command',
   build_command = '$(LUA) build-aux/luke'
      .. ' package="' .. package .. '"'
      .. ' version="' .. _MODREV .. '"'
      .. ' PREFIX="$(PREFIX)"'
      .. ' LUA="$(LUA)"'
      .. ' LUA_INCDIR="$(LUA_INCDIR)"'
      .. ' CC="$(CC)"'
      .. ' LD="$(LD)"'
      .. ' CFLAGS="$(CFLAGS)"'
      .. ' LIBFLAG="$(LIBFLAG)"'
      .. ' LIB_EXTENSION="$(LIB_EXTENSION)"'
      .. ' OBJ_EXTENSION="$(OBJ_EXTENSION)"'
      .. ' INST_LIBDIR="$(LIBDIR)"'
      .. ' INST_LUADIR="$(LUADIR)"'
      ,
   install_command = '$(LUA) build-aux/luke install --quiet'
      .. ' INST_LIBDIR="$(LIBDIR)"'
      .. ' INST_LUADIR="$(LUADIR)"'
      ,
}

-- The bad argument specs expect typecheck's runtime argument checks.
test_dependencies = {
   'ansicolors',
   'luacov',
   'specl >= 16.1',
   'typecheck >= 3.1',
}

test = {
   type = 'command',
   command = 'specl -vfreport --coverage spec/*_spec.yaml',
}

if _MODREV == 'dev' then
   -- luarocks.org's ldoc fails on Lua 5.5; gvvaughan's works.
   build_dependencies = {'gvvaughan/ldoc'}

   source = {
      url = 'git+https://github.com/luaposix/luaposix.git',
   }
else
   -- A release commits its LDocs, so don't build them again.
   build.build_command = build.build_command .. ' LDOC=true'
end
