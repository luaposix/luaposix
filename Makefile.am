# Non-recursive Make rules.
#
# Copyright (C) 2013-2014 Gary V. Vaughan
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
install_exec_hooks	=
lib_LTLIBRARIES		=
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
