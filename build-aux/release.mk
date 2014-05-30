# Slingshot release rules for GNU Make.

# ======================================================================
# Copyright (C) 2001-2014 Free Software Foundation, Inc.
# Originally by Jim Meyering, Simon Josefsson, Eric Blake,
#               Akim Demaille, Gary V. Vaughan, and others.
# This version by Gary V. Vaughan, 2013.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
# ======================================================================

NOTHING_ELSE ?=


## --------------- ##
## GNU Make magic. ##
## --------------- ##

# This file uses GNU Make extensions. Include it from GNUmakefile with:
#
#   include build-aux/release.mk

# Make tar archive easier to reproduce.
export TAR_OPTIONS = --owner=0 --group=0 --numeric-owner

# Helper variables.
_empty =
_sp = $(_empty) $(_empty)

# member-check,VARIABLE,VALID-VALUES
# ----------------------------------
# Check that $(VARIABLE) is in the space-separated list of VALID-VALUES, and
# return it.  Die otherwise.
member-check =								\
  $(strip								\
    $(if $($(1)),							\
      $(if $(findstring $(_sp),$($(1))),				\
          $(error invalid $(1): '$($(1))', expected $(2)),		\
          $(or $(findstring $(_sp)$($(1))$(_sp),$(_sp)$(2)$(_sp)),	\
            $(error invalid $(1): '$($(1))', expected $(2)))),		\
      $(error $(1) undefined)))

include Makefile

## --------- ##
## Defaults. ##
## --------- ##

GIT	?= git
LUA	?= lua
TAR	?= tar

# Override this in cfg.mk if you are using a different format in your
# NEWS file.
today ?= $(shell date +%Y-%m-%d)

# Old releases are stored here.
release_archive_dir ?= ../release

# Override this in cfg.mk if you follow different procedures.
release-prep-hook  ?= release-prep

_build-aux         ?= build-aux
my_distdir	   ?= $(PACKAGE)-$(VERSION)
prev_version_file  ?= $(srcdir)/.prev-version
old_NEWS_hash-file ?= $(srcdir)/local.mk
gl_noteworthy_news_ = * Noteworthy changes in release ?.? (????-??-??) [?]

PREV_VERSION        = $(shell cat $(prev_version_file) 2>/dev/null)
VERSION_REGEXP      = $(subst .,\.,$(VERSION))
PREV_VERSION_REGEXP = $(subst .,\.,$(PREV_VERSION))


## ------------- ##
## Distribution. ##
## ------------- ##

gitlog_to_changelog = $(srcdir)/build-aux/gitlog-to-changelog

dist-hook: ChangeLog
.PHONY: ChangeLog
ChangeLog:
	$(AM_V_GEN)if test -d '$(srcdir)/.git'; then		\
	  $(gitlog_to_changelog) $(gitlog_args) > '$@T';	\
	  rm -f '$@'; mv '$@T' '$@';				\
	fi

# Override this in GNUmakefile if you don't want to automatically
# redistribute all the maintainer support files (take care that
# Travis CI is finicky about this, and will likely need tweaking
# to cope with missing any of these if you decide to omit them).

_travis_yml ?= .travis.yml travis.yml.in

release_extra_dist ?=					\
	.autom4te.cfg					\
	GNUmakefile					\
	bootstrap					\
	bootstrap.conf					\
	local.mk					\
	$(_travis_yml)					\
	$(NOTHING_ELSE)

EXTRA_DIST +=						\
	$(_build-aux)/release.mk			\
	$(gitlog_to_changelog)				\
	$(release_extra_dist)				\
	$(NOTHING_ELSE)

all-am: $(_travis_yml)


## -------- ##
## Release. ##
## -------- ##

# The vast majority of what follows is preparation -in the form
# of early bail-out if something is not right yet- for the final
# check-in-release-branch rule that makes the tip of the release
# branch match the contents of a 'make distcheck' tarball.

# Validate and return $(RELEASE_TYPE), or die.
RELEASE_TYPES = alpha beta stable
release-type = $(call member-check,RELEASE_TYPE,$(RELEASE_TYPES))

# This will actually make the release, including sending release
# announcements, and pushing changes back to the origin.
# Use it like this, eg:
#				make RELEASE_TYPE=beta
.PHONY: release
release:
	$(AM_V_GEN)$(MAKE) $(release-type)
	$(AM_V_GEN)$(MAKE) push
	$(AM_V_GEN)$(MAKE) mail

submodule-checks ?= no-submodule-changes public-submodule-commit

.PHONY: no-submodule-changes
no-submodule-changes:
	$(AM_V_GEN)if test -d $(srcdir)/.git				\
		&& git --version >/dev/null 2>&1; then			\
	  diff=$$(cd $(srcdir) && git submodule -q foreach		\
		  git diff-index --name-only HEAD);			\
	  case $$diff in '') ;;						\
	    *) echo '$(ME): submodule files are locally modified:';	\
		echo "$$diff"; exit 1;; esac;				\
	else								\
	  : ;								\
	fi

# Ensure that each sub-module commit we're using is public.
# Without this, it is too easy to tag and release code that
# cannot be built from a fresh clone.
.PHONY: public-submodule-commit
public-submodule-commit:
	$(AM_V_GEN)if test -d $(srcdir)/.git				\
		&& git --version >/dev/null 2>&1; then			\
	  cd $(srcdir) &&						\
	  git submodule --quiet foreach					\
	      'test "$$(git rev-parse "$$sha1")"			\
	      = "$$(git merge-base origin "$$sha1")"'			\
	    || { echo '$(ME): found non-public submodule commit' >&2;	\
		 exit 1; };						\
	else								\
	  : ;								\
	fi
# This rule has a high enough utility/cost ratio that it should be a
# dependent of "check" by default.  However, some of us do occasionally
# commit a temporary change that deliberately points to a non-public
# submodule commit, and want to be able to use rules like "make check".
# In that case, run e.g., "make check gl_public_submodule_commit="
# to disable this test.
gl_public_submodule_commit ?= public-submodule-commit
check: $(gl_public_submodule_commit)

# These targets do all the file shuffling necessary for a release, but
# purely locally, so you can rewind and redo before pushing anything
# to origin or sending release announcements. Use it like this, eg:
#
#				make beta
.PHONY: alpha beta stable
alpha beta stable: $(submodule-checks)
	$(AM_V_GEN)test $@ = stable &&					\
	  { echo $(VERSION) |$(EGREP) '^[0-9]+(\.[0-9]+)*$$' >/dev/null	\
	    || { echo "invalid version string: $(VERSION)" 1>&2; exit 1;};}\
	  || :
	$(AM_V_at)$(MAKE) prev-version-check
	$(AM_V_at)$(MAKE) vc-diff-check
	$(AM_V_at)$(MAKE) release-commit RELEASE_TYPE=$@
	$(AM_V_at)$(MAKE) news-check
	$(AM_V_at)$(MAKE) distcheck
	$(AM_V_at)$(MAKE) check
	$(AM_V_at)$(MAKE) $(release-prep-hook) RELEASE_TYPE=$@
	$(AM_V_at)$(MAKE) check-in-release-branch

prev-version-check:
	$(AM_V_at)if test -z "`$(GIT) ls-files $(prev_version_file)`";	\
	then								\
	  echo "error: checked in $(prev_version_file) required." >&2;	\
	  exit 1;							\
	fi

# Abort the release if there are unchecked in changes remaining.
vc-diff-check:
	$(AM_V_at)if ! $(GIT) diff --exit-code; then		\
	  $(GIT) diff >/dev/null;				\
	  echo "error: Some files are locally modified" >&2;	\
	  exit 1;						\
	fi

# Select which lines of NEWS are searched for $(news-check-regexp).
# This is a sed line number spec.  The default says that we search
# only line 3 of NEWS for $(news-check-regexp), to match the behaviour
# of '$(_build-aux)/do-release-commit-and-tag'.
# If you want to search only lines 1-10, use "1,10".
news-check-lines-spec ?= 3
news-check-regexp ?= '^\*.* $(VERSION_REGEXP) \($(today)\)'

news-check: NEWS
	$(AM_V_GEN)if $(SED) -n $(news-check-lines-spec)p $<		\
	    | $(EGREP) $(news-check-regexp) >/dev/null; then		\
	  :;								\
	else								\
	  echo 'NEWS: $$(news-check-regexp) failed to match' 1>&2;	\
	  exit 1;							\
	fi

.PHONY: release-commit
release-commit:
	$(AM_V_GEN)cd $(srcdir)						\
	  && $(_build-aux)/do-release-commit-and-tag			\
	       -C $(abs_builddir) $(VERSION) $(RELEASE_TYPE)

define emit-commit-log
  printf '%s\n' 'maint: post-release administrivia.' ''			\
    '* NEWS: Add header line for next release.'				\
    '* .prev-version: Record previous version.'				\
    '* $(old_NEWS_hash-file) (old_NEWS_hash): Auto-update.'
endef

.PHONY: release-prep
release-prep: $(scm_rockspec)
	$(AM_V_GEN)$(MAKE) --no-print-directory -s announcement		\
	  > ~/announce-$(my_distdir)
	$(AM_V_at)if test -d $(release_archive_dir); then		\
	  ln $(rel-files) $(release_archive_dir);			\
	  chmod a-w $(rel-files);					\
	fi
	$(AM_V_at)echo $(VERSION) > $(prev_version_file)
	$(AM_V_at)$(MAKE) update-old-NEWS-hash
	$(AM_V_at)perl -pi						\
	  -e '$$. == 3 and print "$(gl_noteworthy_news_)\n\n\n"'	\
	  $(srcdir)/NEWS
	$(AM_V_at)msg=$$($(emit-commit-log)) || exit 1;			\
	cd $(srcdir) && $(GIT) commit -s -m "$$msg" -a
	@echo '**** Release announcement in ~/announce-$(my_distdir)'

# Strip out copyright messages with years, so that changing those (e.g.
# with 'make update-copyight') doesn't change the old_NEWS_hash.
NEWS_hash =								\
  $$(sed -n '/^\*.* $(PREV_VERSION_REGEXP) ([0-9-]*)/,$$p'		\
       $(srcdir)/NEWS							\
     | perl -0777 -pe 's/^Copyright.+?[12][0-9]{3}.+?\n//ms'		\
     | md5sum -								\
     | sed 's/ .*//')

# Update the hash stored above.  Do this after each release and
# for any corrections to old entries.

old-NEWS-regexp = '^old_NEWS_hash[ \t]+?=[ \t]+'
update-old-NEWS-hash: NEWS
	$(AM_V_GEN)if $(EGREP) $(old-NEWS-regexp) $(old_NEWS_hash-file); then \
	  perl -pi -e 's/^(old_NEWS_hash[ \t]+:?=[ \t]+).*/$${1}'"$(NEWS_hash)/" \
	    $(old_NEWS_hash-file);					\
	else								\
	  printf '%s\n' '' "old_NEWS_hash = $(NEWS_hash)"		\
	    >> $(old_NEWS_hash-file); \
	fi

ANNOUNCE_ENV	= LUA_INIT= LUA_PATH='$(abs_srcdir)/?-git-1.rockspec'
ANNOUNCE_PRINT	= $(ANNOUNCE_ENV) $(LUA) -l$(PACKAGE) -e

_PRE	= "    https://raw.githubusercontent"
_POST	= "/release-v$(VERSION)/$(PACKAGE)-$(VERSION)-$(rockspec_revision).rockspec"
GITHUB_ROCKSPEC	= (source.url:gsub ("^git://github", $(_PRE)):gsub ("%.git$$", $(_POST)))

announcement: NEWS
# Not $(AM_V_GEN) since the output of this command serves as
# announcement message: else, it would start with " GEN announcement".
	$(AM_V_at)$(ANNOUNCE_PRINT) 'print (description.summary)'
	$(AM_V_at)printf '%s\n'	''					\
	  'I am happy to announce release $(VERSION) of $(PACKAGE_NAME).' \
	  ''
	$(AM_V_at)$(ANNOUNCE_PRINT)					\
	  'print ("$(PACKAGE_NAME)'\''s home page is at " .. description.homepage)'
	$(AM_V_at)printf '\n'
	$(AM_V_at)$(SED) -n						\
	    -e '/^\* Noteworthy changes in release $(PREV_VERSION)/q'	\
	    -e p NEWS |$(SED) -e 1,2d
	$(AM_V_at)printf '%s\n'						\
	  'Install it with LuaRocks, using:' ''				\
	  '    luarocks install $(PACKAGE) $(VERSION)' ''		\
	  'Until the rocks are available from the official repository in a few days,' \
	  'you can install directly from the $(PACKAGE) release branch, with:' \
	  '' '    $$ luarocks install '\\
	$(AM_V_at)$(ANNOUNCE_PRINT) 'print ($(GITHUB_ROCKSPEC))'


branch		 = $(shell $(GIT) branch |$(SED) -ne '/^\* /{s///;p;q;}')
GCO		 = $(GIT) checkout
release-tarball	 = $(my_distdir).tar.gz

# Anything in $(_save-files) is not removed after switching to the
# release branch, and is thus "in the release". Add addtional partial
# filenames to save in save_release_files, for example:
#    save_release_files = RELEASE-NOTES-
_save-files =						\
		$(release-tarball)			\
		$(save_release_files)			\
		$(NOTHING_ELSE)


list-to-rexp     = $(SED) -e 's|^|(|' -e 's/|$$/)/'
git-clean-files  = `printf -- '-e %s ' $(_save-files)`
grep-clean-files = `printf -- '%s|' $(_save-files) |$(list-to-rexp)`

# Switch to (or create) 'release' branch, remove all files, except the
# newly generated dist tarball, then unpack the dist tarball and check
# in all the files it creates, and tag that as the next release.
# Github creates automatic zipballs of tagged git revisions, so we can
# safely use this tag in the rockspecs we distribute.
submodule-regexp ?= '^\[submodule "'
submodule-extract-spec ?= 's|^.*"\([^"]*\)".*$$|\1|'

.PHONY: check-in-release-branch
check-in-release-branch:
	$(AM_V_GEN)$(GCO) -b release v$(VERSION) 2>/dev/null || $(GCO) release
	$(AM_V_at)$(GIT) pull origin release 2>/dev/null || true
	$(AM_V_at)if $(EGREP) $(submodule-regexp) .gitmodules >/dev/null 2>&1; then \
	    $(EGREP) $(submodule-regexp) .gitmodules			\
	    | $(SED) $(submodule-extract-spec) | xargs rm -rf;		\
	fi
	$(AM_V_at)$(GIT) clean -dfx $(git-clean-files)
	$(AM_V_at)remove_re=$(grep-clean-files);			\
	    $(GIT) rm -f `$(GIT) ls-files |$(EGREP) -v "$$remove_re"`
	$(AM_V_at)ln -s . '$(my_distdir)'
	$(AM_V_at)$(TAR) zxf '$(release-tarball)'
	$(AM_V_at)rm -f '$(my_distdir)' '$(release-tarball)'
	$(AM_V_at)$(GIT) add .
	$(AM_V_at)$(GIT) commit -s -a -m "Release v$(VERSION)."
	$(AM_V_at)$(GIT) tag -s -a -m "Full source $(VERSION) release" release-v$(VERSION)
	$(AM_V_at)$(GCO) $(branch)

.PHONY: push
push:
	$(AM_V_at)$(GIT) push origin master
	$(AM_V_at)$(GIT) push origin release
	$(AM_V_at)$(GIT) push origin v$(VERSION)
	$(AM_V_at)$(GIT) push origin release-v$(VERSION)

announce_emails ?= lua-l@lists.lua.org
rockspec_emails ?= luarocks-developers@lists.sourceforge.net

.PHONY: mail
mail: rockspecs
	$(AM_V_at)cat ~/announce-$(my_distdir)				\
	  | mail -s '[ANN] $(PACKAGE) $(VERSION) released' --		\
	    $(announce_emails)
	$(AM_V_at)printf '%s\n'						\
	  'Rockspec for $(PACKAGE) version $(VERSION) at:'		\
	  `$(ANNOUNCE_PRINT) 'print ($(GITHUB_ROCKSPEC))'`		\
	  | mail -s '[ANN] $(PACKAGE) $(VERSION) released; rockspec url included' -- \
	    $(rockspec_emails)
