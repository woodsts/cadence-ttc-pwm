AC_INIT([cadence-ttc-pwm],[20150308-1456.0],[obd@xiphos.ca])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([foreign -Wall -Werror subdir-objects])
AC_SUBST(LIBDIR)
AC_PROG_CC
AM_PROG_CC_C_O
m4_pattern_allow([AM_PROG_AR])
AM_PROG_AR
LT_INIT
AC_C_BIGENDIAN

AC_LANG_WERROR
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([Makefile src/Makefile src/kernel/Makefile])

AC_ARG_WITH([kernel_module],
	[AS_HELP_STRING([--with-kernel-module],
			   [Activates module compilation.],)],
	[
	 	KERNEL_SUBDIR=kernel
	 	with_kernel_module=yes
	],
	[
		KERNEL_SUBDIR=""
	 	with_kernel_module=no
	])
AC_SUBST(KERNEL_SUBDIR)

AC_ARG_WITH([kernel_path],
	[AS_HELP_STRING([--with-kernel-path],
		[Path to kernel directory.],)],
	[
		KERNEL_PATH=$with_kernel_path
		AC_SUBST(KERNEL_PATH)
	],
	[])

AC_ARG_WITH([kernel_arch],
	[AS_HELP_STRING([--with-kernel-arch],
			   [ARCH value for compiling the kernel module],)],
	[
	 	KERNEL_ARCH=$with_kernel_arch
		AC_SUBST(KERNEL_ARCH)
	],
	[])
AC_ARG_WITH([kernel_cross_compile],
	[AS_HELP_STRING([--with-kernel-cross-compile],
			   [CROSS_COMPILE value for the kernel module],)],
	[
	 	KERNEL_CROSS_COMPILE=$with_kernel_cross_compile
		AC_SUBST(KERNEL_CROSS_COMPILE)
	],
	[])

AS_IF([test "x$with_kernel_module" != xno ],
	  [
		  AS_IF([test "x$KERNEL_PATH" = x],
				[AC_MSG_FAILURE(
				 [Kernel path must be specified using --with-kernel-path])])
		  AS_IF([test "x$KERNEL_ARCH" = x],
				[AC_MSG_FAILURE(
				 [Kernel arch must be specified using --with-kernel-arch])])
	  ]
)
AC_OUTPUT
