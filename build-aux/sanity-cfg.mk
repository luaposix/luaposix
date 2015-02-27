exclude_file_name_regexp--sc_require_config_h = ^ext/include/(compat-5\.2|strlcpy)\.c|ext/posix/posix\.c$$
exclude_file_name_regexp--sc_require_config_h_first = ^ext/include/(compat-5\.2|strlcpy)\.c|ext/posix/posix\.c$$
exclude_file_name_regexp--sc_error_message_uppercase = ^(examples/socket|lib/posix/deprecated)\.lua|ext/posix/stdlib\.c$$
exclude_file_name_regexp--sc_file_system = ^ext/posix/stdio\.c$$
exclude_file_name_regexp--sc_useless_cpp_parens = ^ext/include/compat-5\.2\.[ch]$$

## Our license precludes linking with gnulib/lib/stat-size.h :(
local-checks-to-skip = sc_prohibit_stat_st_blocks

EXTRA_DIST += build-aux/sanity-cfg.mk
