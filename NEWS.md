# luaposix NEWS - User visible changes

## Noteworthy changes in release 36.2 (2023-06-16) [stable]

### Incompatible Changes

  - Using latest typecheck and specl rocks requires that
    our argument errors list all typenames in full
    ("integer" not "int") and unions in order ("nil or
    string" not "string or nil".

    All luaposix APIs now report argument type errors
    accordingly.

### Bugs Fixed

  - `posix.unistd.read` and `posix.sys.socket.recv` no longer
    hide the underlying real error by casting a ssize_t result
    into an unsigned site_t local.

  - Fix a typo in the `lflag` argument name in the termios
    table LDocs.

  - `posix.sys.*` submodules are correctly preloaded into
    the module table returned by `require 'posix'` so that
    this undocumented, deprecated and slow way to access
    the functions therein continues to work for now:

        local posix = require 'posix'
        local stat  = posix.sys.stat.stat

    For new code, consider this fast and documented
    alternative, that does not load every available luaposix
    API from disk (especially if your project does not use
    APIs from every luaposix module):

        local stat  = require 'posix.sys.stat'.stat

  - Removed `IUTF8` from the documented list of supported `iflag`
    values in `posix.termios` because it was previously
    unimplemented. Implemented non-portable support as an XSI
    extension, as it isn't a part of the POSIX standard.

## Noteworthy changes in release 36.1 (2023-01-31) [stable]

### Bugs Fixed

  - `luke` now uses the same cpp defines for conftests as
    will be used for module compilation, otherwise we are
    potentially testing and compiling with different sets
    of features.

  - `tm.tm_gmtoff` and `tm.tm_zone` are populated on Linux
    distros that require compiling with `-D_BSD_SOURCE`.


## Noteworthy changes in release 36.0 (2023-01-29) [stable]

### Incompatible Changes

  - `posix.spawn` always returns `integer`, `string` whether
    `fork` fails immediately, or whatever `wait`ing for the
    spawned process reports.  When the second result value is
    "exited", the first is the exit status; for "killed" or
    "stopped" second value, the first is the signal number
    that caused it; otherwise first the `errno` error number,
    followed by the associated error string.

    The documentation for `posix.spawn` has always been wrong
    up until now, but this small change to simplify the returned
    results will require checking whether the second result
    value is "exited", "killed", "stopped" or any other string
    before interpreting the first result... which was also
    necessary in prior versions too, but now the interpretion
    of non-zero status by clients is less messy and somewhat
    compatible with the happy path of using previous releases.

  - Argument type errors for `posix.sys.msg.msgctl`,
    `posix.sys.resource.setrlimit`, `posix.sys.socket.bind`,
    `posix.sys.socket.connect`, `posix.sys.socket.getaddrinfo`,
    `posix.sys.socket.sendto` and `posix.time.nanosleep` all
    use "integer" in full rather than "int".

### Bugs Fixed

  - `luke` no longer crashes in `std.normalize` require loops
    occasionally in Lua 5.4.

  - Don't leak `err` into global scope from `compat.open`

  - Correct LDoc comments for `sys.resource.setrlimit`.

  - Where an integer argument is expected, for consistency with
    Lua 5.2 and older, always allow 0.0 to be accepted as if 0
    had been passed.  For example: `pwd.getpwuid(0.0)`.

  - `require 'posix'.glob '.*'` continues to invoke the
    backwards compatibility `glob` function, but that no longer
    prevents access to the preloaded `require 'posix'.glob.glob`
    binding.

  - The `fcntl` binding is still available from the top-level
    `posix` module as `require 'posix'.fcntl`, but that no longer
    prevents access to the preloaded `require 'posix'.fcntl.fcntl`
    binding.

  - `poll.poll` and `poll.rpoll` LDocs correctly describe the returned
    integer as being `0` for timeout or the number of ready file
    descriptors.

  - `stdio.fdopen` returned streams can be closed from Lua 5.1
    without a NULL pointer dereference crash.

  - Both `sys.resource.getrlimit` and `sys.resource.setrlimit`
    properly roundtrip `rlim_t` values.

  - `sys.socket.getpeername` and `sys.socket.getsockname` now return
    a correctly filled `sockaddr` table. In case of an AF_UNIX socket,
    the returned `path` is now cropped to the correct length.

  - `time` constants are `nil` valued, even on hosts which fail to
    define them when _POSIX_TIMERS is set as if they should be.

  - `time.gmtime` and `time.localtime` no longer wrap around with
    arguments of 2^31 and larger.

  - `time.gmtime` and `time.localtime` propagate any `tm_gmoffset` and
    `tm_zone` fields supported by the host's `struct tm`.

  - `time.strftime` reliably fills %z and %Z specifiers. Note that if
    your host POSIX library provides a `strftime` that assumes the
    local timezone, %z will always print the local UTC offset,
    regardless of the `tm_gmoffset` field value passed in.

    Consider (subject to host strftime implementation!):

        local t = require 'posix.time'
        local now = t.time()
        local zulu_t = t.strftime("%c UTC+0000", t.gmtime(now)))
        local localt = t.strftime("%c %Z UTC%z", t.localtime(now)))

  - `unistd.getcwd` no longer leaks memory if __GNU__ was defined at
    compile time.

  - `unistd.readlink` works reliably inside a /proc filesystem.

  - `unistd.write` takes an optional third argument `nbytes` to
    conform better to the SUSv3 specification.  For backwards
    compatibility, the entirety of `buf` is written when a third
    argument is not passed (or is `nil`).

  - Many bindings use now a lua_Integer (with at least 53 bits for
    magnitude, depending on the host Lua version) instead of a C int
    type to pass numeric arguments, so much larger values can be
    successfully passed to and from the following bindings:
    `fcntl.open`, `posix.fcntl.posix_fadvise`, `fnmatch.fnmatch`,
    `posix.grp.getgrgid`, `pwd.getpwuid`, `sched.sched_getscheduler`,
    `sched.sched_setscheduler`, `signal.kill`, `signal.killpg`,
    `signal.raise`, `sys.msg.msgget`, `sys.msg.msgrcv`,
    `sys.socket.recv`, `sys.socket.recvfrom`, `sys.socket.setsockopt`,
    `sys.stat.S_ISBLK`, `sys.stat.S_ISCHR`, `sys.stat.S_ISDIR`,
    `sys.stat.S_ISFIFO`, `sys.stat.S_ISLNK`, `sys.stat.S_ISREG`,
    `sys.stat.S_ISSOCK`, `sys.stat.chmod`, `sys.stat.mkdir`,
    `sys.stat.mkfifo`, `sys.stat.umask`, `sys.wait.wait`,
    `syslog.LOG_MASK`, `time.mktime`, `unistd._exit`, `unistd.alarm`,
    `unistd.ftruncate`, `unistd.getegid`, `unistd.geteguid`,
    `unistd.getgid`, `unistd.getopt`, `unistd.getpgrp`,
    `unistd.getpid`, `unistd.getppid`, `unistd.getuid`,
    `unistd.gethostid`, `unistd.lseek`, `unistd.pathconf`,
    `unistd.read`, `unistd.setpid`, `unistd.sleep`, `unistd.sysconf`,
    `unistd.truncate`, `utime.utime`.

  - Most of the spec examples run correctly on FreeBSD now!

### New Features

  - `unistd.write` takes an optional fourth argument `offset` to
    allow efficent writing of a substring of `buf`.  For backwards
    compatibility, the entirety of `buf` is written when the third and
    fourth arguments are not passed (or are `nil`).

  - When running in luajit, use luajit's internal `bit` functions
    instead of depending on the `bit32` module being available.


## Noteworthy changes in release 35.1 (2021-09-09) [stable]

### Bugs Fixed

  - Documentation cleanups: full and correct return docs including an
    errnum where necessary; make sure we have a link to SUSv3 wherever
    available; add an ', if successful' suffix to a normap return
    result followed by error return results; fixed several typos.

  - Update fork2.lua example to match current APIs

### New Features

  - Support AF_PACKET socket family, where available.

  - Support SIGINFO, SIGWINCH and SA_RESTART, where available.


## Noteworthy changes in release 35.0 (2020-07-06) [stable]

### Incompatible Changes

  - Internally, we use our own bitwise operation wrapper functions
    rather than bit32 where possible.  This means bitwise operations
    when running under Lua 5.3+ work on 64bit integers.

### Bugs Fixed

  - `posix.deprecated.getrlimit` and `posix.deprecated.setrlimit` pass
    tests again.

### New Features

  - Initial support for Lua 5.4.


## Noteworthy changes in release 34.1.1 (2019-07-27) [stable]

### Bugs Fixed

  - `posix.compat` really does work without `std.normalize` in the
    `package.path` now.


## Noteworthy changes in release 34.1 (2019-07-20) [stable]

### Bugs Fixed

  - `luke` works with upgraded bootstrap luarocks version of `require`.

  - `posix.glob.glob` does not return a random errno on failure now, but
    new module constants `GLOB_ABORTED`, `GLOB_NOMATCH` or `GLOB_NOSPACE`.

  - `posix.deprecated.clock_getres` and `posix.deprecated.clock_gettime`
    work again.

### New Features

  - `posix.glob.glob` supports now module constants as a second argument:
    `GLOB_NOCHECK` and `GLOB_ERR`.

  - `posix.sys.socket` now supports IPV6 link-local addresses.

  - use `std.strict`, `std._debug` and `typecheck` modules when available,
    otherwise behave normally without those additional runtime checks.


## Noteworthy changes in release 34.0.4 (2018-02-20) [stable]

### Bugs Fixed

  - `posix.time.clock_getres`, `posix.time.clock_gettime`
    `posix.time.CLOCK_MONOTONIC`, `posix.time.CLOCK_REALTIME`,
    `posix.time.CLOCK_PROCESS_CPUTIME_ID` and
    `posix.time.CLOCK_THREAD_CPUTIME_ID` are all properly elided
    on macOS again.

  - `spec/spec_helper.lua` now looks in the correct objdir
    for object modules built by luke, instead of hard-coding
    `./linux/?.so`, so specl examples work on macOS and others
    again.


## Noteworthy changes in release 34.0.3 (2018-02-19) [stable]

### Bugs Fixed

  - `posix.deprecated.fadvise` is properly elided on macOS and other
    hosts that have no `posix.fcntl.posix_fadvise` implementation.


## Noteworthy changes in release 34.0.2 (2018-02-17) [stable]

### Incompatible Changes

  - `require 'posix'` (or any of its submodules) always returns a
    populated module table, but no longer sets `_G.posix` or any
    other global symbol on Lua 5.1 (including LuaJIT).  If you were
    relying on this behaviour, please change your require statments
    to an assignment:

    ```lua
    local posix = require 'posix'
    ```

### Bugs Fixed

  - `posix.sys.wait.wait` returns `<childpid>, "running"` when the
    child has not exited yet, and `wait` was called with `WNOHANG`
    set.

  - specs don't fail on valid -1 return values from `posix.sysconf`.

  - loading posix (or any of its submodules) no longer leaks symbols
    into the global namespace on Lua 5.1 (including LuaJIT).


## Noteworthy changes in release 34.0.1 (2017-07-09) [stable]

### Bugs Fixed

  - `posix.sys.socket` works correctly on macOS again.

  - `posix.sys.socket` abstract namespaces work correctly on Linux.

  - `posix.syslog.openlog` keeps a copy of it's ident parameter string
    in the Lua registry to ensure we don't hang on to a dangling
    pointer.

  - latest `build-aux/luke` and upgraded lukefile work correctly on
    macOS now.


## Noteworthy changes in release 34.0 (2017-05-06) [stable]

### Incompatible Changes

  - `posix.syslog` constants `LOG_AUTHPRIV`, `LOG_FTP` and `LOG_SYSLOG`
    are no longer documented, and only defined when supported by the
    host C library.  They are not part of the POSIX API, and prevented
    compilation of luaposix on AIX and Solaris.

  - `getopt.getopt` was a binding to the GNU `getopt_long` extension,
    which is not a POSIX API, and prevents luaposix from compiling on
    hosts that do not provide the extended API.  Instead we now correctly
    bind POSIX getopt(3) in `posix.unistd`.

    If you still have code that uses the old non-POSIX binding, then
    LuaRocks has the Lua only `alt-getopt` module which is very similar,
    and works even when the host C library has no `getopt_long` API;
    alternatively, you should consider migrating to the far more
    powerful Lua-only `optparse` library, also available from LuaRocks.

### New Features

  - Replaced all Autotools and supporting Slingshot code with new Lua-
    based `build-aux/luke` and `lukefile` describing how to compile and
    install everything.  Building with LuaRocks now leverages this too.

  - New `posix.sys.socket.getsockopt` and `posix.sys.socket.getpeername`
    bindings.

  - New `posix.sys.msg.msgctl` binding, and `posix.sys.msg.IPC_RMID`,
    `posix.sys.msg.IPC_SET` and `posix.sys.msg.IPC_STAT` constants to
    use with it.

  - New `posix.unistd.ftruncate` and `posix.unistd.truncate` bindings.

  - `posix.fcntl.fcntl` now accepts undocumented `posix.fcntl.O_TMPFILE`
    value where supported by the host C library.

### Bugs Fixed

  - `posix.unistd.ttyname` now returns an error message in addition to
    `nil` when it fails.


## Noteworthy changes in release 33.4.0 (2016-02-27) [stable]

### Incompatible Changes

  - posix.curses has been split back out into its own separate
    project again.  If you want to upgrade your posix.curses using
    project, something like the following minimal change will work
    equivalently (assuming you have the new lcurses on your package
    path):

    ```lua
    local posix = require 'posix'
    posix.curses = posix.curses or require 'lcurses'
    ```

### New Features

  - New `posix.stdio.rename` binding.

  - New `posix.fcntl.FD_CLOEXEC` flag for `posix.fcntl.fcntl`.

  - New `posix.fcntl.O_CLOEXEC` flag for `posix.fcntl.open`, where
    supported by the underlying system.  Falling back to FD_CLOEXEC is
    an exercise to the caller, so that non-atomicity is surfaced:

    ```lua
    -- without error checking, for clarity
    fd = fcntl.open ("/foo/bar", fcntl.O_CLOEXEC, stat.S_IRWXU)
    if fcntl.O_CLOEXEC == 0 then
      local flags = fcntl.fcntl (fd, fcntl.F_GETFD)
      fcntl.fcntl (fd, fcntl.F_SETFD, bit.bor (flags, fcntl.FD_CLOEXEC)
    end
    ```

  - New `posix.unistd.tcgetpgrp` and `posix.unistd.tcsetpgrp` bindings
    where supported by host C library.

  - `posix.sys.socket` AF_UNIX path handling improvements enable use of
    Linux only abstract namespace sockets.

  - Documentation links now point at the newer OpenGroup issue 7 specs.

### Bugs Fixed

  - Workaround for https://sourceware.org/bugzilla/show_bug.cgi?id=15088.
    Predicate use of POSIX sched.h APIs on presence of sched.h, since glibc
    defines _POSIX_PRIORITY_SCHEDULING erroneously, and musl libc
    provides the sched.h APIs but doesn't define _POSIX_PRIORITY_SCHEDULING.

  - `posix.sys.socket` AF_UNIX APIs pass path arguments by length
    instead of terminating at the first \0 byte.

  - `posix.unistd.readlink` no longer fails when reading a link to an
    unsized buffer or character special device node.

  - `execx` is no longer leaked into the global namespace after loading
    the posix module.

  - `examples/fork2.lua` works correctly again.

  - `posix.timeradd`, `posix.timercmp` and `posix.timersub` correctly
    handle tables from the modern thin wrapper API (posix.sys.time),
    while retaining compatibility with the legacy compatibility wrappers
    (posix.gettimeofday).


## Noteworthy changes in release 33.3.1 (2015-03-01) [stable]

### Incompatible Changes

  - The briefly lived `posix.system` API has been removed.  It was renamed
    to `posix.spawn` shortly after introduction, and can still be accessed
    by the new symbol name.

### Bugs Fixed

  - `local posix = require "posix"` works again, fixing the regression
    introduced in the last release.  The automated Travis checks have been
    improved to catch this sort of bug in future.


## Noteworthy changes in release 33.3.0 (2015-02-28) [stable]

### New Features

  - Improved documentation of `sockaddr` tables for `posix.sys.socket` calls.

  - New `posix.sys.socket.getsockname` binding.

  - Remove the `posix.bit32` library, and use Lua’s built-in `bit32` library.

  - Can now be linked against NetBSD curses, albeit with several functions
    not implemented by that library returning a "not implemented" error as
    a consequence.

  - New functions `popen`, `popen_pipeline` and `pclose` mimic the POSIX
    functions of the same name while allowing tasks to be Lua functions.

  - `fdopen` has been re-added, working on all supported Lua versions.

  - `execx` allows a Lua function or command to be exec’d; `spawn` is now
    implemented in terms of it.

### Incompatible Changes

  - The ncurses-only `KEY_MOUSE` definition has been removed from
    `posix.curses`.

### Bugs Fixed

  - `posix.sys.resource` only provides RLIM_SAVED_CUR and RLIM_SAVED_MAX if
    they are defined by the C library (which FreeBSD 10 does not).

  - `posix.dirent.dir` and `posix.dirent.files` now raise a Lua `error()`
    when unable to open the path argument, for orthogonality with `io.lines`.

  - Workaround for manifest key clash between `posix.so` and `posix.lua` in
    LuaRocks.


## Noteworthy changes in release 33.2.1 (2015-01-04) [stable]

### Bugs Fixed

  - Install posix.curses.html documentation file correctly.


## Noteworthy changes in release 33.2.0 (2015-01-03) [stable]

### New Features

  - QNX support.

### Bugs Fixed

  - `posix.time.gmtime` and `posix.time.localtime` specifications now
    work correctly in January too!


## Noteworthy changes in release 33.1.0 (2014-12-19) [stable]

### New Features

  - New `posix.bit32` module with band, bnot and bor functions that can be
    used from any supported Lua release, without worrying about loading an
    external bit operations library.

  - Preliminary Lua 5.3.0 compatibility.

### Bugs Fixed

  - No more 'Bad Hints' errors from `posix.sys.socket.getaddrinfo` on many
    hosts.

  - `stdlib.setenv` accepts a 3rd argument again.


## Noteworthy changes in release 33.0.0 (2014-11-04) [stable]

### New Features

  - The curses library is fully integrated into luaposix, including reasonably
    comprehensive LDoc documentation (certainly much better than the single
    wooly web-page in previous releases).  For backwards compatibility, all
    APIs are re-exported from the `curses` module.

  - Most constants available through luaposix are now listed in the LDocs of
    the submodule that defines them.

  - For backwards compatibility, everything is still lumped together in the
    `posix.???` namespace, but, now raw APIs have been reorganised according to
    the POSIX header files they belong to:

    ```lua
    local posix  = require "posix"
    local fcntl  = require "posix.fcntl"
    local unistd = require "posix.unistd"

    local fd = fcntl.open ("x",
       bit32.bor (posix.O_WRONLY, posix.O_CREAT), "u=rw")
    unistd.write (fd, "Hello, World!\n")
    unistd.close (fd)
    ```

    This makes the documentation easier to navigate, and for a better mapping
    between luaposix APIs and the C functions they wrap, so translating from
    C is now easier than ever.

  - Each of the newly separated submodules is self-contained, and can be copied
    to another project for compiling and/or loading in a custom Lua runtime. If
    you want to make use of this, in addition to the source for the modules you
    copy, you'll also need at least the files `ext/posix/_helpers.c` and
    `ext/include/lua52compat.c`, and also `ext/include/strlcat.c` for one or
    two of them.

  - Where submodule calls return a table representation of a similar C struct
    from a POSIX API, the Lua return tables have an appropriate `_type` field
    metatable entry compatible with lua-stdlib `std.object.type`.

  - `posix.signal.signal` now accepts the constants `posix.signal.SIG_DFL` or
    `posix.signal.SIG_IGN` in place of the strings "SIG_DFL" and "SIG_IGN".

  - The submodule bindings `posix.time.gmtime`, `posix.time.localtime`,
    `posix.time.mktime`, `posix.time.strftime` and `posix.time.strptime` now
    accept or create PosixTm tables with 1-to-1 field name mappings with the
    POSIX `struct tm`.  The old APIs with custom field names is still available
    as `posix.gmtime`, `posix.localtime`, `posix.mktime`, `posix.strftime` and
    `posix.strptime`.

  - Similarly, `posix.time.nanosleep` now takes and returns a PosixTimespec
    table.  The old API is still available as `posix.nanosleep`.

  - Where supported by the underlying system, `posix.time.clock_getres` and
    `posix.time.clock_gettime` now require a constant inte argument (newly
    defined in the `posix.time` submodule), and returns a PosixTimespec table.
    The old APIs are still available as `posix.clock_getres` and
    `posix.clock_gettime`.

  - Add `posix.unistd.gethostid`. The old `posix.hostid` API is still available
    as an alias.

  - Add group APIs from grp.h: `posix.grp.endgrent`, `posix.grp.getgrent`,
    `posix.grp.getgrgid`, `posix.grp.getgrnam` and `posix.grp.setgrent`.
    Consequently, `posix.getgroup` is now reimplemented in Lua over the POSIX
    APIs.

  - `posix.getgroup` defaults to current effective group when called with no
    arguments, for consistency with `posix.getpasswd` API.

  - Add pwd APIs from pwd.h: `posix.pwd.endpwent`, `posix.grp.getpwent`,
    `posix.pwd.getpwnam`, `posix.pwd.getpwuid` and `posix.pwd.setpwent`.
    Consequently, `posix.getpasswd` is now reimplemented in Lua over the POSIX
    APIs.

  - Add missing constants from sys/resource.h:
    `posix.sys.resource.RLIM_INFINITY`, `posix.sys.resource.RLIM_SAVED_CUR`,
    `posix.sys.resource.RLIM_SAVED_MAX`, `posix.sys.resource.RLIMIT_CORE`,
    `posix.sys.resource.RLIMIT_CPU`, `posix.sys.resource.RLIMIT_DATA`,
    `posix.sys.resource.RLIMIT_FSIZE`, `posix.sys.resource.RLIMIT_NOFILE`,
    `posix.sys.resource.RLIMIT_STACK`, `posix.sys.resource.RLIMIT_AS`.

  - Add missing APIs from unistd.h: `posix.unistd.getegid`,
    `posix.unistd.geteuid`, `posix.unistd.getgid`, `posix.unistd.getuid`,
    `posix.unistd.getpgrp`, `posix.unistd.getpid`, `posix.unistd.getppid`.
    Consequently, `posix.getpid` is now reimplemented in Lua over the POSIX
    APIs.

  - Add missing constants from signal.h; `posix.signal.SIG_DFL` and
    `posix.signal.SIG_IGN'.

  - Add missing APIs from sys/stat.h: `posix.sys.stat.S_ISBLK`,
    `posix.sys.stat.S_ISCHR`, `posix.sys.stat.S_ISDIR`,
    `posix.sys.stat.S_ISFIFO`, `posix.sys.stat.S_ISLNK`,
    `posix.sys.stat.S_ISREG`, `posix.sys.stat.S_ISSOCK`.

  - Add missing constants from sys/stat.h: `posix.sys.stat.S_IFMT`,
    `posix.sys.stat.S_IFBLK`, `posix.sys.stat.S_IFCHR`,
    `posix.sys.stat.S_IFDIR`, `posix.sys.stat.S_IFIFO`,
    `posix.sys.stat.S_IFLNK`, `posix.sys.stat.S_IFREG`,
    `posix.sys.stat.S_IRWXU`, `posix.sys.stat.S_IRUSR`,
    `posix.sys.stat.S_IWUSR`, `posix.sys.stat.S_IXUSR`,
    `posix.sys.stat.S_IRWXG`, `posix.sys.stat.S_IRGRP`,
    `posix.sys.stat.S_IWGRP`, `posix.sys.stat.S_IXGRP`,
    `posix.sys.stat.S_IRWXO`, `posix.sys.stat.S_IROTH`,
    `posix.sys.stat.S_IWOTH`, `posix.sys.stat.S_IXOTH`,
    `posix.sys.stat.S_ISGID`, `posix.sys.stat.S_ISUID`.

  - Add missing constants from syslog.h: `posix.syslog.LOG_CONS`,
    `posix.syslog.LOG_NDELAY` and `posix.syslog.LOG_PID`.

  - Add missing API from syslog.h: `posix.syslog.LOG_MASK`.  Use this to
    convert syslog priority constants into mask bits suitable for bitwise
    ORing as the argument to `posix.syslog.setlogmask`.

  - Add missing constants from time.h: `posix.time.CLOCK_MONOTONIC`,
    `posix.time.CLOCK_PROCESS_CPUTIME_ID`, `posix.time.CLOCK_REALTIME` and
    `posix.time.CLOCK_THREAD_CPUTIME_ID`.

  - New `posix.unistd.exec` and `posix.unistd.execp` require a table of
    arguments, with [0] defaulting to the command name.  The old string
    tuple passing API is still available as `posix.exec` and `posix.execp`.

  - `posix.util.openpty` has moved to `posix.openpty`.  The old API is still
    available as an alias.

  - All posix APIs now fully and correctly diagnose extraneous and wrong
    type arguments with an error.

  - Add `posix.IPC_NOWAIT`, `posix.MSG_EXCEPT` and `posix.MSG_NOERROR`
    constants for message queues.

  - Add `posix.IPPROTO_UDP` for socket programming.

  - Add `posix.AI_NUMERICSERV` for posix.getaddrinfo hints flags.

  - Add `posix.WUNTRACED` for posix.wait flags.

  - Add `curses.A_COLOR` (where supported by the underlying curses library) for
    extracting color pair assignments from the results of `curses.window.winch`.

  - Add missing `curses.KEY_F31` constant.

### Bugs Fixed

  - `posix.fadvise` is now spelled `posix.fcntl.posix_fadvise` and takes a
    file descriptor first argument rather than a Lua file handle. The old
    misspelled bad argument type version is undocumented but still works.

  - `posix.getpasswd`, `posix.getpid`, `posix.pathconf`, `posix.stat`,
    `posix.statvfs`, `posix.sysconf` and `posix.times` process a single table
    argument with a list of types correctly.

  - `posix.syslog.openlog` now takes the bitwise OR of those constants.  The
    old string option specified API is still available as `posix.openlog`.

  - `posix.syslog.setlogmask` now takes the bitwise OR of bits returned by
    passing priority constants to `posix.syslog.LOG_MASK`.  The old API will
    continue to be available as `posix.setlogmask`.

  - `posix.readlink` is much more robust, and reports errors accurately.

  - configured installation installs `posix.so` into the lua cpath directory
    correctly.

  - fixed a long-standing bug where the stdio buffers were not restored after
    some posix.fcntl() examples, resulting in the `make check` output being
    truncated -- often before terminal colors were returned to normal.


## Noteworthy changes in release 32 (2014-05-30) [stable]

### New Features

  - Support for posix.socketpair call and posix.AF_UNIX constant.

  - Previously undocumented spawn, pipeline, pipeline_iterator, pipeline_slurp,
    euidaccess, timeradd, timercmp and timersub have been moved from the posix
    table, which is reserved for strictly POSIX APIs to the posix.sys subtable.
    The sys submodule automatically loads on first reference, so no need to
    require it manually if you already have the main posix module loaded.

  - posix api documentation is separated into groups for better discovery.

### Bugs Fixed

  - Builds correctly on hosts with no IPV6 capability.

  - Small improvements in organisation of generated html docs.

  - posix.openpty doesn't crash.

  - configure now detects Lua correctly with busybox grep.

  - Many fine portability fixes from latest gnulib.

  - Missing docs for accept, bind, connect, getaddrinfo, listen, recv,
    recvfrom, send, sendto, setsockopt, shutdown, socket and socketpair apis
    is now provided.

  - Missng docs for tcdrain, tcflow, tcflush, tcgetattr, tcsendbreak and
    tcsetattr terminal apis are now provided.

  - Docs for apis implemented in Lua are now shown correctly.


## Noteworthy changes in release 31 (2013-09-09) [stable]

### New Features

  - Missing termios cc flags are now available.

### Bugs Fixed

  - posix.tcgetattr and posix.tcsetattr no save and restore all flags,
    regardless of whether they are local extensions to POSIX.


## Noteworthy changes in release 30 (2013-08-29) [stable]

### New Features

  - Support for file locks with fcntl() using F_SETLK, F_SETLKW, F_GETLK,
    F_RDLCK, F_WRLCK and F_UNLCK.

  - Preliminary support for GNU Hurd, and OpenBSD.

### Bugs Fixed

  - posix.shutdown can actually be called now.

  - Report the correct argument number in posix function error messages.

  - Much reduced compiler warning noise.

  - Many small typos and inconsistencies, see ChangeLog for details.


## Noteworthy changes in release 29 (2013-06-28) [stable]

  - This release adds wresize to curses, and sync, fsync, fdatasync, nice,
    lseek as well as socket programming functions.  Several small improvements
    to the documentation were also added.

  - luaposix is compatible with Lua 5.1, Lua 5.2 and luajit 2.0, so the
    5.1 prefix to the release version has become an anachronism and has
    been dropped from this release onwards.

### New Features

  - Move to the Slingshot release system, which (among many other improvements)
    fixes release tarballs from github to work with the standard GNU-style:
    `./configure, make, make install`.  `bootstrap` is still distributed for
    those who need to re-bootstrap with a different version of gnulib and/or
    slingshot.

  - Much improved former lunit and ad-hoc test scripts to Specl.


## Noteworthy changes in release 5.1.28 (2013-03-23) [stable]

  - This release fixes the previously unannounced posix.pipeline_iterator and
    posix.pipeline_slurp functions, and adds a test for them. A workaround for
    having LUA_INIT_5_2 set has been added to the build system.


## Noteworthy changes in release 5.1.27 (2013-03-17) [stable]

  - This release fixes broken Lua 5.1 compatibility in release 5.1.26
    (sorry! And thanks to Nick McVeity for the bug report and patch); renames
    posix.system to posix.spawn (the old name is available for backwards
    compatibility), generalizing it to take a shell command, file and
    arguments, or Lua function; and adds posix.pipeline, which makes it easy
    to run a pipeline of processes, each a shell command, program, or Lua
    function.


## Noteworthy changes in release 5.1.26 (2013-03-04) [stable]

  - This release adds killpg, realpath and openpty, adds a flags parameter to
    signal, and improves some documentation.


## Noteworthy changes in release 5.1.25 (2013-02-20) [stable]

  - This release adds support for message queues and UNIX 98 pseudoterminals
    (thanks very much to the respective contributors), and allows argv[0] to
    be set in exec calls.


## Noteworthy changes in release 5.1.24 (2013-02-15) [stable]

  - This release adds isatty and constants STDIN_FILENO, STDOUT_FILENO and
    STDERR_FILENO, fixes a bug in readlink, adds a day field to time tables
    for compatibility with os.date, and overhauls the build and release system.


## Noteworthy changes in release 5.1.23 (2012-10-04) [stable]

  - This release fixes the curses module for Lua 5.2; previously it would not
    load with an unknown symbol error. The build process  for luarocks has been
    made more robust.


## Noteworthy changes in release 5.1.22 (2012-09-13) [stable]

  - This release fixes building on Mac OS X and some other OSes which don't
    like building empty libraries. Thanks to Robert McLay for the bug report.


## Noteworthy changes in release 5.1.21 (2012-09-10) [stable]

  - This release adds comprehensive documentation for the posix module, from
    Steve Donovan and Natanael Copa.

  - It makes one small change: rpoll now uses file descriptors, not Lua file
    objects (hence, via fileno, it can use both).

  - Perhaps most importantly, it marks a change of maintainer, from
    Reuben Thomas to Alexander Nikolaev. Thanks very much to Alexander for
    agreeing to take over. Luaposix has garnered considerable interest in
    recent months, and more contributors have stepped forwards with patches.
    Alexander will help to oversee a maturing API, coordinate ongoing
    improvements and additions, and help ensure that luaposix doesn't fall
    back into disrepair as it has several times in the past.


## Noteworthy changes in release 5.1.20 (2012-06-22) [stable]

### New Features

  - Improves signal handling.
  - Improves the posix.system and creat functions (all thanks to Steve Donovan).
  - Adds mkdtemp (thanks, 7hemroc).
  - Adds statvfs (thanks to Like Ma).
  - improves the tests.
  - Adds some code guidelines.

### Bugs Fixed

  - Fixes a bug in getgroup.
  - Fixes some space leaks (thanks, Alexander Nikolaev),
  - Copes with sysconf for _PC_PATH_MAX returning -1.

### Incompatible Changes

  - The API of posix.open has changed to be more like the C version: the file
    creation and status flags are now constants in the POSIX namespace. This
    enables them to be used outside calls to open, and makes posix.open less
    magic. posix.open will now raise an error if no creation flags are given
    when O_CREAT is used.


## Noteworthy changes in release 5.1.19 (2012-04-10) [stable]

  - This release avoids the use of PATH_MAX, and copes with arbitrarily-long
    paths. The implementation of strlcpy is changed to a BSD-licensed
    implementation; the previously-used implementation was LGPL-licensed, which
    is not MIT-compatible; thanks to Alexander Gladysh for bringing this
    problem to my attention. (This was just mis-released as 5.1.18; sorry!)


## Noteworthy changes in release 5.1.18 (2012-03-26) [stable]

  - This release implements full Lua 5.2 compatibility; thanks to Enrico Tassi
    for poking me to get this done.


## Noteworthy changes in release 5.1.17 (2012-02-29) [stable]

  - This release improves support for Lua 5.2; the curses module should now
    work fine (the posix module still needs updating). Signal handling has been
    improved to make it possible to chain to a C signal handler, and a bug in
    resetting the process's signal mask after running a Lua handler has been
    fixed.


## Noteworthy changes in release 5.1.16 (2012-02-18) [stable]

  - This release includes rewritten fcntl and signals support, and bug fixes
    for read, chmod, getgroups and waitpid. curses boolean return values are
    now Lua booleans rather than 0 for OK or ERR for not OK. fnmatch, strptime
    and mktime are now supported, chmod now supports octal modes, thereâs
    much expanded poll support, and some non-POSIX and obsolete features have
    been removed. There are more tests and the build system has been improved.
    Thanks go to the many contributors to this release.


## Noteworthy changes in release 5.1.15 (2011-09-29) [stable]

  - This release adds dup, pipe, pipe2 and more fcntl support (thanks to
    Alexander V. Nikolaev and Alexander Gladysh for the patches). Two bugs in
    the test code which used incorrect paths and caused only one set of tests
    to run have been fixed.


## Noteworthy changes in release 5.1.14 (2011-09-19) [stable]

  - This release allows some constants to be case-insensitive in Lua, and fixes
    a small build-system bug.


## Noteworthy changes in release 5.1.13 (2011-09-17) [stable]

This release adds a rockspec.


## Noteworthy changes in release 5.1.12 (2011-09-09) [stable]

  - This release adds some basic functions such as open, close, read and write,
    and integrates the pure Lua module which was previously in Lua stdlib. It
    also adds a whole new module, curses, which was previously in the separate
    lcurses project (curses is part of the POSIX standard).

### Incompatible Changes

  - Note that the C part of the POSIX module is now called posix_c.so (or
    similar), so if you have an old posix.so (or similar) you should delete it
    to avoid clashing with the new posix.lua.


## Noteworthy changes in release 5.1.11 (2011-04-27) [stable]

  - Apologies, 5.1.10, released earlier today, had a buffer overflow bug
    in the new mkstemp function. 5.1.11, just out, fixes it.


## Noteworthy changes in release 5.1.10 (2011-04-27) [stable]

### This release adds mkstemp, adds some fixes for building on Mac OS X
    (thanks to Gary Vaughan), removes some non-POSIX rlimit constants,
    guards some functions that were not correctly guarded, so that they
    will not be compiled on systems that don't support them, and makes
    other minor fixes.


## Noteworthy changes in release 5.1.9 (2011-03-24) [stable]

### New Features

  - support for signals and for getopt. See below for details.

  - Equally, there is still only the barest documentation: to use the various
    APIs you have to grep to see if the one you want is there and then read the
    C comment which gives the Lua API. If anyone is interested in adding better
    documentation, I'd be delighted to hear from them. (My work on luaposix is
    purely aimed at getting the support I need for GNU Zile, but as usual I
    welcome patches from others. luaposix is still far from complete, so please
    send patches for your favourite POSIX APIs!)

  - luaposix 5.1.9 improves compatibility with Darwin/Mac OS X, and adds
    various new API bindings, for signals, getgroups, setting errno and
    _exit, as well as some slight code cleanup.


## Noteworthy changes in release 5.1.8 (2013-03-23) [stable]

### Bugs Fixed

  - fix bugs for setrlimit and gettimeofday.
  - an improvement to test.lua.
  - better use of POSIX feature macros to determine what APIs to support.
  - removal of the obsolete timezone argument to gettimeofday,
  - remove the non-POSIX gecos field of struct passwd
  - improvements to the build system
  - some code tidy-up,
  - removal of Lua 5.0 compatibility

### New APIs

  - abort, raise, isprint, isgraph, errno and stdio.h constants, and getopt_long.


## Noteworthy changes in release 5.1.7 (2013-03-23) [stable]

A new minor bugfix release of luaposix is out.

### Bugs Fixed

  - make clock_* functions' argument optional
  - fixes posix.version string


## Noteworthy changes in release 5.1.6 (2010-08-11) [stable]

  - This release adds time functions: gettimeofday, clock_getres,
    clock_gettime, localtime, gmtime, time, strftime.


## Noteworthy changes in release 5.1.5 (20??-??-??) [stable]

The release notes for this release were lost in the mists of thyme.


## Noteworthy changes in release 5.1.4 (2008-07-18) [stable]

  - Includes a fix for rpoll() from debian[1] and a patch from openwrt[2]
    that adds crypt().


## Noteworthy changes in release 5.1.3 (2013-03-23) [stable]

No changes.


## Noteworthy changes in release 5.1.2 (2008-01-29) [stable]

### Incompatible changes

  - Please note that this release breakes the API for dup() and exec()

### New Features

  - dup() now takes and returns lua files rather than file descriptors
    (int).

  - exec() uses now execv(3) rather than execvp(3).  This means that the
    PATH environment variable is no longer used which means that all scripts
    currently using exec() without an absolute path will break. If you need
    the PATH variable, use the new execp() function.

  - Added openlog(), syslog() and closelog() functions.

  - The openlog(ident, [option], [facility]) function differs from the
    recently released luasyslog by giving the user possibility to set
    "option". The "option" parameter is a string containing one or more of
    the chars:

    ```
    'c' - LOG_CONS
    'n' - LOG_NDELAY
    'e' - LOG_PERROR
    'p' - LOG_PID
    ```

    It is possible to disable those funcs compile time by setting the
    ENABLE_SYSLOG define to 0.

  - fileno() function was added.


## Noteworthy changes in release 5.1.1 (2008-01-25) [stable]

  - I have forked lposix. First release includes some patches submitted on
    this list.

    This first release is basicly lposix with a cleaned up Makefile + the
    patches found here:

       http://lua-users.org/lists/lua-l/2006-10/msg00448.html
       http://lua-users.org/lists/lua-l/2007-11/msg00346.html

  - When the promised extened OS library[1] arrives I will most likely
    remove the overlapping functions in luaposix. posix specific functions
    that does not overlap will still be maintained and added. (e.g dup())

  - Releases numbered 5.1.x[.y] will work with lua-5.1 series. The 'x' will
    add/change features and .y releases will be strict bugfixes (no new
    features).

  - I had planned to add syslog functions and fix dup() to handle lua files
    (FILE*) rather than file descriptors (int). Now that luasyslog just
    released I will have to re-evaluate that.
