# Maintainer rules.
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

ME = GNUmakefile

# If the user runs GNU make but didn't ./configure yet, do it for them.
dont-forget-to-bootstrap = $(wildcard Makefile.in)

ifeq ($(dont-forget-to-bootstrap),)

## Don't redo any pedantic rock version checks, incase they derail
## a subdirectory bootstrap of slingshot.

%::
	@echo '$(ME): rebootstrap'
	@test -f Makefile.in || ./bootstrap --skip-rock-checks
	@test -f Makefile || ./configure
	$(MAKE) $@

else


include build-aux/release.mk
include build-aux/sanity.mk

# Run sanity checks as part of distcheck.
distcheck: $(local-check)


## ------------------------- ##
## Copyright Notice Updates. ##
## ------------------------- ##

# If you want to set UPDATE_COPYRIGHT_* environment variables,
# for the build-aux/update-copyright script: set the assignments
# in this variable in local.mk.
update_copyright_env ?=

# Run this rule once per year (usually early in January)
# to update all FSF copyright year lists in your project.
# If you have an additional project-specific rule,
# add it in local.mk along with a line 'update-copyright: prereq'.
# By default, exclude all variants of COPYING; you can also
# add exemptions (such as ChangeLog..* for rotated change logs)
# in the file .x-update-copyright.
.PHONY: update-copyright
update-copyright:
	$(AM_V_GEN)grep -l -w Copyright \
	$$(export VC_LIST_EXCEPT_DEFAULT=COPYING && $(VC_LIST_EXCEPT)) \
	| $(update_copyright_env) xargs $(srcdir)/build-aux/$@


## ------ ##
## Specl. ##
## ------ ##

# Use 'make check V=1' for verbose output, or set SPECL_OPTS to
# pass alternative options to specl command.

SPECL_OPTS     ?=
SPECL_OPTS     += $(specl_verbose_$(V))
specl_verbose_  = $(specl_verbose_$(AM_DEFAULT_VERBOSITY))
specl_verbose_0 =
specl_verbose_1 = --verbose --formatter=report


endif
