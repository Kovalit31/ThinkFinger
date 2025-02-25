################################################################################
#
# configure.in for ThinkFinger, process with autoconf to create a configure
# script.
#
# Copyright (C) 2006, 2007 Novell, Inc.
# Author: Timo Hoenig <thoenig@suse.de>, <thoenig@nouse.net>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#
################################################################################

# Initialize auto{conf,make}
AC_PREREQ([2.71])
AC_INIT([ThinkFinger],[0.3],[http://thinkfinger.sourceforge.net/],[thinkfinger])
AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE
AC_CONFIG_SRCDIR([libthinkfinger/libthinkfinger.c])

# Checks for required programs
AC_PROG_CC
AC_PROG_INSTALL
LT_INIT
AM_PROG_CC_C_O

# Checks for header files.
AC_CHECK_HEADERS([errno.h stdio.h stdlib.h string.h syslog.h unistd.h usb.h])

# AC_ARG_ENABLE USB_DEBUG
AC_MSG_CHECKING([whether to build with USB hooks for debugging])
AC_ARG_ENABLE(usb-debug, AS_HELP_STRING([--enable-usb-debug],[build with USB hooks for debugging]),enable_usb_debug=$enableval,enable_usb_debug=no)
AC_MSG_RESULT([$enable_usb_debug])

if test "x$enable_usb_debug" = "xyes"; then
	AC_DEFINE_UNQUOTED(USB_DEBUG, 1, [Define to 1 if you want to build with USB hooks for debugging.])
fi

# AC_ARG_ENABLE PAM
AC_MSG_CHECKING([whether to build the pluggable authentication module (PAM)])
AC_ARG_ENABLE(pam, AS_HELP_STRING([--enable-pam],[build PAM module]),enable_pam=$enableval,enable_pam=yes)
AC_MSG_RESULT([$enable_pam])

if test "x$enable_pam" != "xno"; then
	AC_CHECK_HEADERS(security/pam_appl.h security/pam_modules.h,, AC_MSG_ERROR(PAM header files not found))
	AC_CHECK_LIB(pam, pam_start, [PAM_LIBS="-lpam"], AC_MSG_ERROR([libpam missing]))
	AC_DEFINE(BUILD_PAM, 1, [Define to 1 if you want to build with PAM support.])
	AC_CHECK_LIB(pam, pam_prompt, , HAVE_OLD_PAM=yes
				AC_DEFINE(HAVE_OLD_PAM,1,
				[Defined if pam_prompt function does not exist in libpam]))

	# Check for Linux input and uinput headers
	AC_CHECK_HEADER(linux/input.h, HAVE_LINUX_INPUT=1,,)
	if test "$HAVE_LINUX_INPUT" = "1"; then
		AC_CHECK_HEADER(linux/uinput.h, HAVE_LINUX_UINPUT=1,, [#include <linux/input.h>])
	else
		AC_MSG_ERROR("Required header file not found.")
	fi

	if test "$HAVE_LINUX_UINPUT" != "1"; then
		AC_MSG_ERROR("Required header file not found.")
	fi
fi

# AC_ARG_ENABLE_BASH
AC_MSG_CHECKING([whether to install the BASH completion for tf-tool])
AC_ARG_ENABLE(bash, AS_HELP_STRING([--enable-bash],[install BASH completion for tf-tool in $sysconfdir/bash_completion.d]),enable_bash=$enableval,enable_bash=no)
AC_MSG_RESULT([$enable_bash])

# AC_ARG_ENABLE SECUREDIR
AC_ARG_ENABLE(securedir, AS_HELP_STRING([--with-securedir=dir],[Where to put PAM module @<:@default=$libdir/security@:>@]))

# AC_ARG_ENABLE_BIR_DIR
AC_ARG_ENABLE(birdir, AS_HELP_STRING([--with-birdir=dir],[Where to put the biometric identification records (bir files) @<:@default=$sysconfdir/pam_thinkfinger@:>@]))

# Check for pthread
AC_CHECK_LIB(pthread, pthread_create, [PTHREAD_LIBS="-lpthread"], AC_MSG_ERROR([libpthread missing]))

# Check for libusb using pkg-config
PKG_CHECK_MODULES(USB, libusb >= 0.1.11, usb_found=yes, AC_MSG_ERROR([libusb missing]))

# Check for Doxygen
AC_CHECK_PROG(DOXYGEN, [doxygen], [doxygen], [])

# Find out prefix
PREFIX=
if test "x$prefix" = "xNONE" ; then
	PREFIX="/usr"
else
	PREFIX=$prefix
fi
prefix=$PREFIX

# Find out exec_prefix
EXEC_PREFIX=
if test "x$exec_prefix" = xNONE ; then
	EXEC_PREFIX=$prefix
else
	EXEC_PREFIX=$exec_prefix
fi
exec_prefix=$EXEC_PREFIX

# AC_SUBST PREFIX, LIBDIR, BINDIR, SBINDIR, MANDIR, SECUREDIR and BIRDIR
PREFIX_TMP="$prefix"
PREFIX=`eval echo $PREFIX_TMP`
AC_SUBST(PREFIX)

LIBDIR_TMP="$libdir"
LIBDIR=`eval echo $LIBDIR_TMP`
AC_SUBST(LIBDIR)

BINDIR_TMP="$bindir"
BINDIR=`eval echo $BINDIR_TMP`
AC_SUBST(BINDIR)

SBINDIR_TMP="$sbindir"
SBINDIR=`eval echo $SBINDIR_TMP`
AC_SUBST(SBINDIR)

if ! test -z "$with_securedir" ; then
	SECUREDIR_TMP="$with_securedir"
else
	SECUREDIR_TMP=$LIBDIR/$build_cpu-$build_os/security
fi
SECUREDIR=`eval echo $SECUREDIR_TMP`
AC_SUBST(SECUREDIR)

if ! test -z "$with_birdir" ; then
	BIRDIR_TMP="$with_birdir"
else
	BIRDIR_TMP=/etc/pam_thinkfinger
fi
BIRDIR=`eval echo $BIRDIR_TMP`
AC_SUBST(BIRDIR)

if ! test -z "$mandir" ; then
	MANDIR_TMP=`eval echo "$mandir"`
else
	MANDIR_TMP=`eval echo $datadir/man`
fi
MANDIR=`eval echo \"$MANDIR_TMP\"`
AC_SUBST(MANDIR)

# AC_DEFINE PAM_BIRDIR
AC_DEFINE_UNQUOTED(PAM_BIRDIR,"${BIRDIR}",[Define to the directory where biometric identification records (bir files) are being stored.])

# AC_SUBST CFLAGS
CFLAGS="$CFLAGS -Wall"
CFLAGS="$CFLAGS -fno-common"
CFLAGS="$CFLAGS -fPIC"
CFLAGS="$CFLAGS -Wchar-subscripts"
CFLAGS="$CFLAGS -Wmissing-declarations"
CFLAGS="$CFLAGS -Wmissing-prototypes"
CFLAGS="$CFLAGS -Wnested-externs"
CFLAGS="$CFLAGS -Wpointer-arith"
CFLAGS="$CFLAGS -Wcast-align"
CFLAGS="$CFLAGS -Wsign-compare"
CFLAGS="$CFLAGS -Wdeclaration-after-statement"
AC_SUBST(CFLAGS)

# AC_SUBST USB_LIBS
AC_SUBST([USB_LIBS])

# AC_SUBST PAM_LIBS
AC_SUBST([PAM_LIBS])

# AC_SUBST_PTHREAD_LIBS
AC_SUBST([PTHREAD_LIBS])

# AM_CONDITIONAL
AM_CONDITIONAL(BUILD_PAM, test "x$enable_pam" = "xyes")
AM_CONDITIONAL(HAVE_OLD_PAM, test "x$HAVE_OLD_PAM" = "xyes")
AM_CONDITIONAL(HAVE_BASH, test "x$enable_bash" = "xyes")

# AC_CONFIG_FILES
AC_CONFIG_FILES([Makefile
		README
		INSTALL
		docs/Makefile
		docs/autodocs/Makefile
		libthinkfinger/Makefile
		libthinkfinger/libthinkfinger.pc
		pam/Makefile
		tf-tool/Makefile
])

# Configuration
echo "
 $PACKAGE_NAME $VERSION
 =================

 + prefix:		${PREFIX}
 + libdir:		${LIBDIR}
 + bindir:		${BINDIR}
 + sbindir:		${SBINDIR}
 + mandir:		${MANDIR}

 + cflags:		${CFLAGS}
 + libusb:		${USB_LIBS}

 Debugging
 =========

 + enable USB hooks:	$enable_usb_debug

"

if test  "x$enable_pam" = "xyes" ; then
echo " Build PAM module:	$enable_pam

 + libpam:		${PAM_LIBS}
 + libpthread:		${PTHREAD_LIBS}
 + securedir:		${SECUREDIR}
 + birdir:		${BIRDIR}
"
fi

if test  "x$enable_bash" = "xyes" ; then
echo " BASH completion:	$enable_bash

 + sysconfdir:          `eval echo ${sysconfdir}`
"
fi

# AC_OUTPUT
AC_OUTPUT
