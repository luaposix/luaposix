# Non-recursive Make rules.
#
# Copyright (C) 2013-2015 Gary V. Vaughan
# Written by Gary V. Vaughan, 2013
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3, or (at your option)
# any later version.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.


## ------------ ##
## Environment. ##
## ------------ ##

LUA_PATH  ?= ;
LUA_CPATH ?= ;

SPECL_ENV =


## ---------- ##
## Bootstrap. ##
## ---------- ##

ACLOCAL_AMFLAGS = -I m4

AM_CPPFLAGS = $(LUA_INCLUDE)


## ------------- ##
## Declarations. ##
## ------------- ##

EXTRA_DIST		=
EXTRA_LTLIBRARIES	=
CLEANFILES		=
DISTCLEANFILES		=
MAINTAINERCLEANFILES	=
NOTHING_ELSE		=

bin_SCRIPTS		=
check_local		=
dist_bin_SCRIPTS	=
dist_lua_DATA		=
doc_DATA		=
install_exec_hooks	= remove-luaexec-lafiles
uninstall_hooks		= uninstall-luaexec-modules
lib_LTLIBRARIES		=
luaexec_LTLIBRARIES	=
man_MANS		=
save_release_files	=

include local.mk
include build-aux/rockspecs.mk


## ------------ ##
## Local Tests. ##
## ------------ ##

check-local: $(check_local)


## ------------- ##
## Installation. ##
## ------------- ##

install-exec-hook: $(install_exec_hooks)

# Neither Lua itself, nor LuaRocks can use .la files, and LuaRocks
# actually moves such files aside anyway, so we just remove them from
# the installation directory.
remove-luaexec-lafiles:
	@for la in $(luaexec_LTLIBRARIES); do	\
	  f=`echo "$$la" |sed 's|^.*/||'`;	\
	  echo rm -f $(luaexecdir)/$$f;		\
	  rm -f $(luaexecdir)/$$f;		\
	done


## --------------- ##
## Uninstallation. ##
## --------------- ##

uninstall-hook: $(uninstall_hooks)

# We removed the .la files from luaexecdir, so the standard uninstall,
# with libtool --mode=uninstall, can't find everything anymore.
uninstall-luaexec-modules:
	@for la in $(luaexec_LTLIBRARIES); do	\
	  base=`echo "$$la"			\
		|sed 's|^.*/\(.*\)\.la|\1|'`;	\
	  echo rm -f $(luaexecdir)/$$base.so;	\
	  rm -f $(luaexecdir)/$$base.so;	\
	done
