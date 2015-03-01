# Specl specs make rules.


## ------ ##
## Specs. ##
## ------ ##

specl_SPECS =						\
	$(srcdir)/specs/posix_version_spec.yaml		\
	$(srcdir)/specs/curses_spec.yaml		\
	$(srcdir)/specs/lposix_spec.yaml		\
	$(srcdir)/specs/posix_compat_spec.yaml		\
	$(srcdir)/specs/posix_ctype_spec.yaml		\
	$(srcdir)/specs/posix_deprecated_spec.yaml	\
	$(srcdir)/specs/posix_dirent_spec.yaml		\
	$(srcdir)/specs/posix_fcntl_spec.yaml		\
	$(srcdir)/specs/posix_fnmatch_spec.yaml		\
	$(srcdir)/specs/posix_grp_spec.yaml		\
	$(srcdir)/specs/posix_pwd_spec.yaml		\
	$(srcdir)/specs/posix_signal_spec.yaml		\
	$(srcdir)/specs/posix_stdio_spec.yaml		\
	$(srcdir)/specs/posix_stdlib_spec.yaml		\
	$(srcdir)/specs/posix_sys_msg_spec.yaml		\
	$(srcdir)/specs/posix_sys_resource_spec.yaml	\
	$(srcdir)/specs/posix_sys_socket_spec.yaml	\
	$(srcdir)/specs/posix_sys_stat_spec.yaml	\
	$(srcdir)/specs/posix_sys_statvfs_spec.yaml	\
	$(srcdir)/specs/posix_sys_time_spec.yaml	\
	$(srcdir)/specs/posix_sys_times_spec.yaml	\
	$(srcdir)/specs/posix_sys_utsname_spec.yaml	\
	$(srcdir)/specs/posix_syslog_spec.yaml		\
	$(srcdir)/specs/posix_time_spec.yaml		\
	$(srcdir)/specs/posix_unistd_spec.yaml		\
	$(srcdir)/specs/posix_spec.yaml			\
	$(NOTHING_ELSE)

EXTRA_DIST +=						\
	$(srcdir)/specs/spec_helper.lua			\
	$(NOTHING_ELSE)

include build-aux/specl.mk
