# Slingshot rockspec rules for make.

# This file is distributed with Slingshot, and licensed under the
# terms of the MIT license reproduced below.

# ==================================================================== #
# Copyright (C) 2013-2015 Reuben Thomas and Gary V. Vaughan                 #
#                                                                      #
# Permission is hereby granted, free of charge, to any person          #
# obtaining a copy of this software and associated documentation       #
# files (the "Software"), to deal in the Software without restriction, #
# including without limitation the rights to use, copy, modify, merge, #
# publish, distribute, sublicense, and/or sell copies of the Software, #
# and to permit persons to whom the Software is furnished to do so,    #
# subject to the following conditions:                                 #
#                                                                      #
# The above copyright notice and this permission notice shall be       #
# included in  all copies or substantial portions of the Software.     #
#                                                                      #
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,      #
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF   #
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGE-   #
# MENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE   #
# FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF   #
# CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION   #
# WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.      #
# ==================================================================== #


## --------- ##
## LuaRocks. ##
## --------- ##

# This file is suitable for use from a portable Makefile, you might
# include it into the top-level Makefile.am with:
#
#   include build-aux/rockspecs.mk

luarocks_config   = build-aux/luarocks-config.lua
rockspec_conf	  = $(srcdir)/rockspec.conf
mkrockspecs	  = $(srcdir)/build-aux/mkrockspecs
package_rockspec  = $(srcdir)/$(PACKAGE)-$(VERSION)-$(rockspec_revision).rockspec
scm_rockspec      = $(PACKAGE)-git-$(rockspec_revision).rockspec

# If you need a different rockspec revision, override this on the make
# command line:
#
#     make rockspecs rockspec_revision=2
rockspec_revision = 1

LUAROCKS	  = luarocks
MKROCKSPECS	  = $(MKROCKSPECS_ENV) $(LUA) $(mkrockspecs)

ROCKSPECS_DEPS =				\
	$(luarocks_config)			\
	$(mkrockspecs)				\
	$(rockspec_conf)			\
	$(NOTHING_ELSE)

set_LUA_BINDIR = LUA_BINDIR=`which $(LUA) |$(SED) 's|/[^/]*$$||'`
LUA_INCDIR = `cd $$LUA_BINDIR/../include && pwd`
LUA_LIBDIR = `cd $$LUA_BINDIR/../lib && pwd`

$(luarocks_config): Makefile.am
	@test -d build-aux || mkdir build-aux
	$(AM_V_GEN){						\
	  $(set_LUA_BINDIR);					\
	  echo 'rocks_trees = { "$(abs_srcdir)/luarocks" }';	\
	  echo 'variables = {';					\
	  echo '  LUA = "$(LUA)",';				\
	  echo '  LUA_BINDIR = "'$$LUA_BINDIR'",';		\
	  echo '  LUA_INCDIR = "'$(LUA_INCDIR)'",';		\
	  echo '  LUA_LIBDIR = "'$(LUA_LIBDIR)'",';		\
	  echo '}';						\
	} > '$@'

$(package_rockspec): $(ROCKSPECS_DEPS)
	$(AM_V_at)rm -f '$@' 2>/dev/null || :
	$(AM_V_GEN)test -f '$@' ||				\
	  $(MKROCKSPECS) $(mkrockspecs_args)			\
	    $(PACKAGE) $(VERSION) $(rockspec_revision) > '$@'
	$(AM_V_at)$(LUAROCKS) lint '$@'

$(scm_rockspec): $(ROCKSPECS_DEPS)
	$(AM_V_at)rm '$@' 2>/dev/null || :
	$(AM_V_GEN)test -f '$@' ||				\
	  $(MKROCKSPECS) $(mkrockspecs_args)			\
	    $(PACKAGE) git 1 > '$@'
	$(AM_V_at)$(LUAROCKS) lint '$@'

.PHONY: rockspecs
rockspecs:
	$(AM_V_at)rm -f *.rockspec
	$(AM_V_at)$(MAKE) $(package_rockspec) $(scm_rockspec)


## ------------- ##
## Distribution. ##
## ------------- ##

EXTRA_DIST +=						\
	$(mkrockspecs)					\
	$(package_rockspec)				\
	$(rockspec_conf)				\
	$(NOTHING_ELSE)

save_release_files += $(scm_rockspec)


## ------------ ##
## Maintenance. ##
## ------------ ##

DISTCLEANFILES +=					\
	$(luarocks_config)				\
	$(NOTHING_ELSE)
