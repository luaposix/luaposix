# makefile for POSIX library for Lua

# change these to reflect your Lua installation
PREFIX=		/usr/local
LUAVERSION=	5.1
LUAINC= 	$(PREFIX)/include
LUALIB= 	$(PREFIX)/lib/lua/$(LUAVERSION)
LUABIN= 	$(PREFIX)/bin

# other executables
TAR=		tar
LUA=		lua
INSTALL=	install

# no need to change anything below here
PACKAGE=	luaposix
LIBVERSION=	2
VERSION=	$(LUAVERSION).$(LIBVERSION)

SRCS=		lposix.c modemuncher.c test.lua tree.lua
EXTRADIST=	Makefile README ChangeLog
DISTFILES=	$(SRCS) $(EXTRADIST)
DISTDIR=	$(PACKAGE)-$(VERSION)
TARGZ=		$(PACKAGE)-$(VERSION).tar.gz

CPPFLAGS=	-fPIC $(INCS) $(WARN)
WARN=		-pedantic -Wall
INCS=		-I$(LUAINC)

MYNAME=		posix
MYLIB= 		$(MYNAME)

OBJS=		l$(MYLIB).o

T= 		$(MYLIB).so

# targets
phony += all
all:	$T

phony += test
test:	all
	$(LUA) test.lua

$T:	$(OBJS)
	$(CC) $(LDFLAGS) -o $@ -shared $(OBJS)

$(OBJS): modemuncher.c

phony += tree
tree:	$T
	$(LUA) -l$(MYNAME) tree.lua .

phony += clean
clean:
	rm -f $(OBJS) $T core core.* a.out $(TARGZ)

phony += install
install: $T
	$(INSTALL) -D $T $(DESTDIR)/$(LUALIB)/$T
	
phony += show-funcs
show-funcs:
	@echo "$(MYNAME) library:"
	@fgrep '/**' l$(MYLIB).c | cut -f2 -d/ | tr -d '*' | sort

# distribution

phony += distdir
distdir: $(DISTFILES)
	if [ -d "$(DISTDIR)" ]; then	\
		rm -r "$(DISTDIR)";	\
	fi
	mkdir "$(DISTDIR)"
	cp -a $(DISTFILES) "$(DISTDIR)/"


phony += tar
tar:	distdir	
	$(TAR) zcf $(TARGZ) $(DISTDIR)
	rm -r $(DISTDIR)

phony += dist
dist:	tar

.PHONY: $(phony)
# eof
