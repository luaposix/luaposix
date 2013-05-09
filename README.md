luaposix
========

by the [luaposix project][github]

[github]: https://github.com/luaposix/luaposix

[![travis-ci status](https://secure.travis-ci.org/luaposix/luaposix.png?branche=master)](http://travis-ci.org/luaposix/lua-posix/builds)

luaposix is a POSIX binding, including curses, for Lua 5.1 and 5.2;
like most libraries it simply binds to C APIs on the underlying
system, so it won't work on a non-POSIX system. However, it does try
to detect the level of POSIX conformance of the underlying system and
bind only available APIs.

luaposix is released under the MIT license, like Lua (see COPYING;
it's basically the same as the BSD license). There is no warranty.

Please report bugs and make suggestions by opening an issue on the
github tracker.


Installation
------------

The simplest way to install luaposix is with [LuaRocks][]. To install the
latest release (recommended):

    luarocks install luaposix

To install current git master (for testing):

    luarocks install https://raw.github.com/luaposix/luaposix/release/luaposix-git-1.rockspec

With Lua 5.1, luaposix requires the bitop library from http://bitop.luajit.org/
(On Lua 5.2 it will work whether bitop is installed or not.)

To install without LuaRocks, check out the sources from the
[repository][github], and then run the following commands: the
dependencies are listed in the dependencies entry of the file
`rockspec.conf`. You will also need Autoconf and Automake.

  cd luaposix
  ./bootstrap
  ./configure --prefix=INSTALLATION-ROOT-DIRECTORY
  make all check install

See [INSTALL][] for instructions for `configure`, and `configure --help`
for details of available command-line switches.

[luarocks]: http://www.luarocksporg "LuaRocks Project"
[install]: https://raw.github.com/luaposix/luaposix/release/INSTALL


Use
---

The library is split into two modules. The basic POSIX APIs are in
"posix"; the curses APIs in "curses".

There is HTML documentation; to obtain it, run:

  make doc

The posix module documentation requires LDoc, from

  https://github.com/stevedonovan/LDoc

It includes links to online man pages. All the HTML documentation is
online at:

  http://luaposix.github.com/luaposix/

You can find an authoritative online POSIX reference at:

  http://www.opengroup.org/onlinepubs/007904875/toc.htm


Example code
------------

See the example program tree.lua, along with the tests in
`tests-*.lua`.

For a complete application, see the lua branch of GNU Zile's (a
cut-down Emacs clone) git repository at:

    http://git.savannah.gnu.org/cgit/zile.git/log/?h=lua


Bugs reports & patches
----------------------

Bug reports and patches are most welcome. Please use the github issue
tracker (see URL at top). There is no strict coding style, but please
bear in mind the following points when writing new code:

0. Follow existing code. There are a lot of useful patterns and
   avoided traps there.

1. 8-character indentation using TABs. Not my favourite either, but
   better than reformatting the code and losing much of the ability to
   follow the version control history.

2. No non-POSIX APIs; no platform-specific code. When wrapping APIs
   introduced in POSIX 2001 or later, add an appropriate #if. If your
   platform isn't quite POSIX, you may find a gnulib module to bridge
   the gap. If absolutely necessary, use autoconf feature tests.

3. Thin wrappers: although some existing code contradicts this, wrap
   POSIX APIs in the simplest way possible. If necessary, more
   convenient wrappers can be added in Lua (posix.lua).
