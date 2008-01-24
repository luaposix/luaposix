# makefile for POSIX library for Lua

# change these to reflect your Lua installation
#LUA= /tmp/lhf/lua-5.0
LUA= /pkg/lua-5.1.1-LR1
LUAINC= $(LUA)/include
LUALIB= $(LUA)/lib
LUABIN= $(LUA)/bin

# no need to change anything below here
CPPFLAGS= -fPIC $(INCS) $(WARN)
#LDFLAGS= -O2
WARN= -pedantic -Wall
INCS= -I$(LUAINC)

MYNAME= posix
MYLIB= $(MYNAME)

OBJS= l$(MYLIB).o

T= $(MYLIB).so

all:	test

test:	$T
	$(LUABIN)/lua test.lua

$T:	$(OBJS)
	$(CC) $(LDFLAGS) -o $@ -shared $(OBJS)

$(OBJS): modemuncher.c

tree:	$T
	$(LUABIN)/lua -l$(MYNAME) tree.lua .

clean:
	rm -f $(OBJS) $T core core.* a.out

x:
	@echo "$(MYNAME) library:"
	@grep '},' $(MYLIB).c | cut -f2 | tr -d '{",' | sort | column

xx:
	@echo "$(MYNAME) library:"
	@fgrep '/**' $(MYLIB).c | cut -f2 -d/ | tr -d '*' | sort | column

# distribution

FTP= $(HOME)/public/ftp/lua
D= $(MYNAME)
A= $(MYLIB).tar.gz
TOTAR= Makefile,README,$(MYLIB).c,$(MYNAME).lua,test.lua,modemuncher.c,tree.lua

tar:	clean
	tar zcvf $A -C .. $D/{$(TOTAR)}

distr:	tar
	mv $A $(FTP)

diff:	clean
	tar zxf $(FTP)/$A
	diff $D .

.stamp:	$(FTP)/$A
	touch --reference=$(FTP)/$A $@

# eof
