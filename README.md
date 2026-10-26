luaposix
========

By the [luaposix project][github]

[![License](https://img.shields.io/:license-mit-blue.svg)](https://mit-license.org)
[![workflow status](https://github.com/luaposix/luaposix/actions/workflows/spec.yml/badge.svg?branch=release-v38)](https://github.com/luaposix/luaposix/actions)
[![codecov.io](https://codecov.io/github/luaposix/luaposix/coverage.svg?branch=release-v38)](https://codecov.io/github/luaposix/luaposix?branch=release-v38)

This is a POSIX binding for [LuaJIT][], [Lua][] 5.1, 5.2, 5.3, 5.4, and 5.5.
Like most libraries it simply binds to C APIs on the underlying system, so it
won't work on non-POSIX systems. However, it does try to detect the
level of POSIX conformance of the underlying system and bind only
available APIs.

For a while, luaposix contained support for curses functionality too,
but now that has its own [lcurses][] repository again, where it is being
maintained separately.

luaposix is released under the [MIT license][mit] (the same license as
Lua itself).  There is no warranty.

[github]: https://github.com/luaposix/luaposix "luaposix repository"
[lcurses]: https://github.com/lcurses/lcurses "lcurses repository"
[lua]: https://www.lua.org/ "The Lua Project"
[luajit]: https://luajit.org/ "The LuaJIT Project"
[mit]: https://mit-license.org "MIT license"


Installation
------------

The simplest and best way to install luaposix is with [LuaRocks][]. To
install the latest release (recommended):

```bash
    luarocks install luaposix
```

To install current git master (for testing, before submitting a bug
report for example):

```bash
    luarocks install https://raw.githubusercontent.com/luaposix/luaposix/master/luaposix-dev-1.rockspec
```

To install into a tree of your own from a clone of the [repository][luaposix],
and run the specs:

```sh
cd luaposix
luarocks --tree LUAPOSIX-INSTALL-TREE make luaposix-dev-1.rockspec
luarocks --tree LUAPOSIX-INSTALL-TREE test
```

[LuaRocks][] builds luapoisx with [luarocks-build-luke], which it installs for
the build.

If you are repackaging or redistributing [luaposix][github], it is better
to start from a [release tarball][releases], because the [master][github]
development branch is branch is unstable, and sometimes breaks subtly, or
does not build at all, or provides experimental new APIs that end up
being removed prior to the next official release.

Note that there are full logs of complete builds for every commit in
[Github Actions][actions], which you can compare with your progress if you
get stuck.

[actions]: https://github.com/luaposix/luaposix/actions
[luaposix]: https://github.com/luaposix/luaposix
[luarocks]: https://luarocks.org "Lua package manager"
[luarocks-build-luke]: https://luarocks.org/modules/gvvaughan/luarocks-build-luke
[releases]: https://github.com/luaposix/luaposix/releases


Use
---

The library is split into submodules according to the POSIX header file
API declarations, which you can require individually:

```lua
    local unistd = require 'posix.unistd'
```

The authoritative online POSIX reference is published at [SUSv3][].

[susv3]: https://pubs.opengroup.org/onlinepubs/007904875/toc.htm


Documentation
-------------

The latest release of this library is [documented in LDoc][github.io].
Pre-built HTML files are included in the release, and contain links to
the appropriate [SUSv3][] manual pages.

[github.io]: https://luaposix.github.io/luaposix


Example code
------------

See the example program `tree.lua`, along with the many small
examples in the generated documentation and BDD `specs/*_spec.yaml`.

For a complete application, see the [GNU Zile][].

[GNU Zile]: https://git.savannah.gnu.org/cgit/zile.git/log/?h=lua "A cut-down Emacs clone"


Bugs reports and code contributions
-----------------------------------

These libraries are maintained by their users.

Please make bug reports and suggestions as [GitHub issues][issues].
Pull requests are especially appreciated.

But first, please check that your issue has not already been reported by
someone else, and that it is not already fixed on [master][github] in
preparation for the next release (See Installation section above for how
to temporarily install master with [LuaRocks][]).

There is no strict coding style, but please bear in mind the following
points when proposing changes:

0. Follow existing code. There are a lot of useful patterns and
   avoided traps there.

1. 8-character indentation using TABs in C sources; 3-character
   indentation using SPACEs in Lua sources.

2. Simple strings are easiest to type using single-quote delimiters
   saving double-quotes for where a string contains apostrophes.

3. Save horizontal space by only using SPACEs where the parser requires
   them.

4. Use vertical space to separate out compound statements to help the
   coverage reports discover untested lines.

5. Prefer explicit string function calls over object methods, to mitigate
   issues with monkey-patching in caller environment. 

6. No non-POSIX APIs; no platform-specific code. When wrapping APIs
   introduced in POSIX 2001 or later, add an appropriate #if. If your
   platform isn't quite POSIX, you may find a gnulib module to bridge
   the gap. If absolutely necessary, use luke feature tests.

7. Thin wrappers: although some existing code contradicts this, wrap
   POSIX APIs in the simplest way possible. If necessary, more
   convenient wrappers can be added in Lua (posix.lua).

[issues]: https://github.com/luaposix/luaposix/issues
