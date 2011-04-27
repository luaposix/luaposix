# makefile for POSIX library for Lua

# change these to reflect your Lua installation
PREFIX=		/usr/local
LUAVERSION=	5.1
LUAINC= 	$(PREFIX)/include
LUALIB= 	$(PREFIX)/lib/lua/$(LUAVERSION)
LUABIN= 	$(PREFIX)/bin

# other executables
LUA=		lua
INSTALL=	install

# no need to change anything below here
PACKAGE=	luaposix
LIBVERSION=	11
RELEASE=	$(LUAVERSION).$(LIBVERSION)

GIT_REV		:= $(shell test -d .git && git describe --always)
ifeq ($(GIT_REV),)
FULL_VERSION	:= $(RELEASE)
else
FULL_VERSION	:= $(GIT_REV)
endif

WARN=		-pedantic -Wall
INCS=		-I$(LUAINC)
CFLAGS+=	-fPIC $(INCS) $(WARN) -DVERSION=\"$(FULL_VERSION)\"

MYNAME=		posix
MYLIB= 		$(MYNAME)

OBJS=		l$(MYLIB).o

T= 		$(MYLIB).so

OS=$(shell uname)
ifeq ($(OS),Darwin)
  LDFLAGS_SHARED=-bundle -undefined dynamic_lookup
  LIBS=
	CFLAGS += -D_POSIX_C_SOURCE
else
  LDFLAGS_SHARED=-shared
  # FIXME: Make -lrt conditional on _XOPEN_REALTIME
  LIBS=-lcrypt -lrt
  CFLAGS += -D_XOPEN_SOURCE=700
endif

# targets
all:	$T

test:	all
	$(LUA) test.lua

$T:	$(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(LDFLAGS_SHARED) $(OBJS) $(LIBS)

tree:	$T
	$(LUA) -l$(MYNAME) tree.lua .

clean:
	rm -f $(OBJS) $T core core.* a.out $(TARGZ)

install: $T
	$(INSTALL) -d $(DESTDIR)/$(LUALIB)
	$(INSTALL) $T $(DESTDIR)/$(LUALIB)/$T

show-funcs:
	@echo "$(MYNAME) library:"
	@fgrep '/**' l$(MYLIB).c | cut -f2 -d/ | tr -d '*' | sort

# eof
