luaposix
========

By the [luaposix project][github]

[![License](http://img.shields.io/:license-mit-blue.svg)](http://mit-license.org)
[![travis-ci status](https://secure.travis-ci.org/luaposix/luaposix.png?branche=master)](http://travis-ci.org/luaposix/luaposix/builds)
[![Stories in Ready](https://badge.waffle.io/luaposix/luaposix.png?label=ready&title=Ready)](https://waffle.io/luaposix/luaposix)

This is a POSIX binding for LuaJIT, [Lua][] 5.1, 5.2 and 5.3; like most
libraries it simply binds to C APIs on the underlying system, so it
won't work on non-POSIX systems. However, it does try to detect the
level of POSIX conformance of the underlying system and bind only
available APIs.

luaposix is released under the [MIT license][mit] (the same license as
Lua itsef).  There is no warranty.

[github]: https://github.com/luaposix/luaposix "Github repository"
[lua]: http://www.lua.org/ "The Lua Project"
[mit]: http://mit-license.org "MIT license"


Installation
------------

The simplest and best way to install luaposix is with [LuaRocks][]. To
install the
latest release (recommended):

```bash
    luarocks install luaposix
```

To install current git master (for testing, before submitting a bug
report for example):

```bash
    luarocks install http://raw.github.com/luaposix/luaposix/release/luaposix-git-1.rockspec
```

The best way to install without [LuaRocks][], is to download a github
[release tarball][releases] and follow the instructions in the included
[INSTALL][] fill.  Even if you are repackaging or redistributing
[luaposix][github], this is by far the most straight forward place to
begin.

Note that you'll be responsible for providing dependencies if you choose
not to let [LuaRocks][] handle them for you, though you can find a list
of the minimal dependencies in the [rockspec.conf][] file.

It is also possible to perform a complete bootstrap of the
[master][github] development branch, although this branch is unstable,
and sometimes breaks subtly, or does not build at all, or provides
experimental new APIs that end up being removed prior to the next
official release. Unfortunately, we don't have time to provide support
for taking this most difficult and dangerous option. It is presumed that
you already know enough to be aware of what you are getting yourself
into - however, there are full logs of complete bootstrap builds in
[Travis][] after every commit, that you can examine if you get stuck.
Also, the bootstrap script tries very hard to tell you why it is unhappy
and, sometimes, even how to fix things before trying again.

[install]: http://raw.githubusercontent.com/luaposix/luaposix/release/INSTALL
[luarocks]: http://www.luarocks.org "Lua package manager"
[releases]: http://github.com/luaposix/luaposix/releases
[rockspec.conf]: http://github.com/luaposix/luaposix/blob/release/rockspec.conf
[travis]: http://travis-ci.org/luaposix/luaposix/builds


Use
---

The library is split into submodules according to the POSIX header file
API declarations, which you can require individually:

```lua
    local unistd = require "posix.unistd"
```

The authoritative online POSIX reference is published at
[SUSv3][].

[susv3]: http://www.opengroup.org/onlinepubs/007904875/toc.htm


Documentation
-------------

The latest release of this library is [documented in LDoc][github.io].
Pre-built HTML files are included in the release, and contain links to
the appropriate [SUSv3][] manual pages.

[github.io]: http://luaposix.github.io/luaposix


Example code
------------

See the example program `tree.lua`, along with the many small
examples in the generated documentation and BDD `specs/*_spec.yaml`.

For a complete application, see the [GNU Zile][].

[GNU Zile]: http://git.savannah.gnu.org/cgit/zile.git/log/?h=lua "A cut-down Emacs clone"


Bugs reports and code contributions
-----------------------------------

These libraries are maintained by their users.

Please make bug reports and suggestions as [GitHub issues][issues].
Pull requests are especially appreciated.

But first, please check that you issue has not already been reported by
someone else, and that it is not already fixed by [master][github] in
preparation for the next release (See Installation section above for how
to temporarily install master with [LuaRocks][]).

There is no strict coding style, but please bear in mind the following
points when proposing changes:

0. Follow existing code. There are a lot of useful patterns and
   avoided traps there.

1. 8-character indentation using TABs in C sources; 2-character
   indentation using SPACEs in Lua sources.

2. No non-POSIX APIs; no platform-specific code. When wrapping APIs
   introduced in POSIX 2001 or later, add an appropriate #if. If your
   platform isn't quite POSIX, you may find a gnulib module to bridge
   the gap. If absolutely necessary, use autoconf feature tests.

3. Thin wrappers: although some existing code contradicts this, wrap
   POSIX APIs in the simplest way possible. If necessary, more
   convenient wrappers can be added in Lua (posix.lua).

[issues]: http://github.com/luaposix/luaposix/issues
