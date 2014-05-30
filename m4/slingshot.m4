dnl slingshot.m4
dnl
dnl Copyright (c) 2013-2014 Free Software Foundation, Inc.
dnl Written by Gary V. Vaughan, 2013
dnl
dnl This program is free software; you can redistribute it and/or modify
dnl it under the terms of the GNU General Public License as published
dnl by the Free Software Foundation; either version 3, or (at your
dnl option) any later version.
dnl
dnl This program is distributed in the hope that it will be useful, but
dnl WITHOUT ANY WARRANTY; without even the implied warranty of
dnl MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
dnl General Public License for more details.
dnl
dnl You should have received a copy of the GNU General Public License
dnl along with this program.  If not, see <http://www.gnu.org/licenses/>.

# SS_CONFIG_TRAVIS(LUAROCKS)
# --------------------------
# Generate .travis.yml, ensuring LUAROCKS are installed.
AC_DEFUN([SS_CONFIG_TRAVIS], [
  # is required by all slingshot clients for mkrockspecs.
  EXTRA_ROCKS=-
  for _ss_rock in lyaml $1; do
    case $EXTRA_ROCKS in
      *" $_ss_rock;"*) ;; # ignore duplicates
      *)
        test "x$PACKAGE_NAME" != "x$_ss_rock" \
            && EXTRA_ROCKS="$EXTRA_ROCKS"' $LUAROCKS install '"$_ss_rock;"
        ;;
    esac
  done

  # Avoid empty travis commands.
  test "x$EXTRA_ROCKS" != "x-" || EXTRA_ROCKS='# No extra rocks needed here;'

  AC_SUBST([EXTRA_ROCKS])
  AC_CONFIG_FILES([.travis.yml:travis.yml.in], [
    # Remove trailing blanks so as not to trip sc_trailing_blank in syntax check
    sed 's|  *$||' < .travis.yml > ss_tmp && mv ss_tmp .travis.yml; rm -f ss_tmp])
])
