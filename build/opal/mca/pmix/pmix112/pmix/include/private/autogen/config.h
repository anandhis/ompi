/* include/private/autogen/config.h.  Generated from config.h.in by configure.  */
/* include/private/autogen/config.h.in.  Generated from configure.ac by autoheader.  */

/* -*- c -*-
 *
 * Copyright (c) 2004-2005 The Trustees of Indiana University.
 *                         All rights reserved.
 * Copyright (c) 2004-2005 The Trustees of the University of Tennessee.
 *                         All rights reserved.
 * Copyright (c) 2004-2005 High Performance Computing Center Stuttgart,
 *                         University of Stuttgart.  All rights reserved.
 * Copyright (c) 2004-2005 The Regents of the University of California.
 *                         All rights reserved.
 * Copyright (c) 2013-2015 Intel, Inc. All rights reserved
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 *
 * This file is automatically generated by configure.  Edits will be lost
 * the next time you run configure!
 */

#ifndef PMIX_CONFIG_H
#define PMIX_CONFIG_H

#include <pmix/autogen/pmix_config_top.h>



/* Define if building universal (internal helper macro) */
/* #undef AC_APPLE_UNIVERSAL_BUILD */

/* Define to 1 if you have the <arpa/inet.h> header file. */
#define HAVE_ARPA_INET_H 1

/* Define to 1 if you have the `asprintf' function. */
#define HAVE_ASPRINTF 1

/* Define to 1 if you have the <crt_externs.h> header file. */
/* #undef HAVE_CRT_EXTERNS_H */

/* Define to 1 if you have the declaration of `AF_INET6', and to 0 if you
   don't. */
#define HAVE_DECL_AF_INET6 1

/* Define to 1 if you have the declaration of `AF_UNSPEC', and to 0 if you
   don't. */
#define HAVE_DECL_AF_UNSPEC 1

/* Define to 1 if you have the declaration of `PF_INET6', and to 0 if you
   don't. */
#define HAVE_DECL_PF_INET6 1

/* Define to 1 if you have the declaration of `PF_UNSPEC', and to 0 if you
   don't. */
#define HAVE_DECL_PF_UNSPEC 1

/* Define to 1 if you have the declaration of `__func__', and to 0 if you
   don't. */
#define HAVE_DECL___FUNC__ 1

/* Define to 1 if you have the <dlfcn.h> header file. */
#define HAVE_DLFCN_H 1

/* Define to 1 if you have the <event.h> header file. */
/* #undef HAVE_EVENT_H */

/* Define to 1 if you have the <fcntl.h> header file. */
#define HAVE_FCNTL_H 1

/* Define to 1 if you have the <hostLib.h> header file. */
/* #undef HAVE_HOSTLIB_H */

/* Define to 1 if you have the <hwloc.h> header file. */
/* #undef HAVE_HWLOC_H */

/* Define to 1 if the system has the type `int16_t'. */
#define HAVE_INT16_T 1

/* Define to 1 if the system has the type `int32_t'. */
#define HAVE_INT32_T 1

/* Define to 1 if the system has the type `int64_t'. */
#define HAVE_INT64_T 1

/* Define to 1 if the system has the type `int8_t'. */
#define HAVE_INT8_T 1

/* Define to 1 if the system has the type `intptr_t'. */
#define HAVE_INTPTR_T 1

/* Define to 1 if you have the <inttypes.h> header file. */
#define HAVE_INTTYPES_H 1

/* Define to 1 if you have the <ioLib.h> header file. */
/* #undef HAVE_IOLIB_H */

/* Define to 1 if you have the `event' library (-levent). */
/* #undef HAVE_LIBEVENT */

/* Define to 1 if you have the `libevent_global_shutdown' function. */
/* #undef HAVE_LIBEVENT_GLOBAL_SHUTDOWN */

/* Define to 1 if you have the `event_pthreads' library (-levent_pthreads). */
/* #undef HAVE_LIBEVENT_PTHREADS */

/* Define to 1 if you have the <libgen.h> header file. */
#define HAVE_LIBGEN_H 1

/* Define to 1 if you have the <limits.h> header file. */
#define HAVE_LIMITS_H 1

/* Define to 1 if the system has the type `long long'. */
#define HAVE_LONG_LONG 1

/* Define to 1 if you have the <memory.h> header file. */
#define HAVE_MEMORY_H 1

/* Define to 1 if you have the <munge.h> header file. */
/* #undef HAVE_MUNGE_H */

/* Define to 1 if you have the <netinet/in.h> header file. */
#define HAVE_NETINET_IN_H 1

/* Define to 1 if you have the <net/uio.h> header file. */
/* #undef HAVE_NET_UIO_H */

/* Define to 1 if the system has the type `ptrdiff_t'. */
#define HAVE_PTRDIFF_T 1

/* Define to 1 if you have the <sasl/sasl.h> header file. */
/* #undef HAVE_SASL_SASL_H */

/* Define to 1 if `si_band' is a member of `siginfo_t'. */
#define HAVE_SIGINFO_T_SI_BAND 1

/* Define to 1 if `si_fd' is a member of `siginfo_t'. */
#define HAVE_SIGINFO_T_SI_FD 1

/* Define to 1 if you have the <signal.h> header file. */
#define HAVE_SIGNAL_H 1

/* Define to 1 if you have the `snprintf' function. */
#define HAVE_SNPRINTF 1

/* Define to 1 if you have the `socketpair' function. */
#define HAVE_SOCKETPAIR 1

/* Define to 1 if the system has the type `socklen_t'. */
#define HAVE_SOCKLEN_T 1

/* Define to 1 if you have the <sockLib.h> header file. */
/* #undef HAVE_SOCKLIB_H */

/* Define to 1 if you have the <stdarg.h> header file. */
#define HAVE_STDARG_H 1

/* Define to 1 if you have the <stdbool.h> header file. */
#define HAVE_STDBOOL_H 1

/* Define to 1 if you have the <stddef.h> header file. */
#define HAVE_STDDEF_H 1

/* Define to 1 if you have the <stdint.h> header file. */
#define HAVE_STDINT_H 1

/* Define to 1 if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H 1

/* Define to 1 if you have the <strings.h> header file. */
#define HAVE_STRINGS_H 1

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* Define to 1 if you have the `strncpy_s' function. */
/* #undef HAVE_STRNCPY_S */

/* Define to 1 if you have the `strsignal' function. */
#define HAVE_STRSIGNAL 1

/* Define to 1 if `d_type' is a member of `struct dirent'. */
#define HAVE_STRUCT_DIRENT_D_TYPE 1

/* Define to 1 if the system has the type `struct sockaddr_in'. */
#define HAVE_STRUCT_SOCKADDR_IN 1

/* Define to 1 if the system has the type `struct sockaddr_in6'. */
#define HAVE_STRUCT_SOCKADDR_IN6 1

/* Define to 1 if `sa_len' is a member of `struct sockaddr'. */
/* #undef HAVE_STRUCT_SOCKADDR_SA_LEN */

/* Define to 1 if the system has the type `struct sockaddr_storage'. */
#define HAVE_STRUCT_SOCKADDR_STORAGE 1

/* Define to 1 if the system has the type `struct sockaddr_un'. */
#define HAVE_STRUCT_SOCKADDR_UN 1

/* Define to 1 if `f_fstypename' is a member of `struct statfs'. */
/* #undef HAVE_STRUCT_STATFS_F_FSTYPENAME */

/* Define to 1 if `f_type' is a member of `struct statfs'. */
/* #undef HAVE_STRUCT_STATFS_F_TYPE */

/* Define to 1 if `f_basetype' is a member of `struct statvfs'. */
/* #undef HAVE_STRUCT_STATVFS_F_BASETYPE */

/* Define to 1 if `f_fstypename' is a member of `struct statvfs'. */
/* #undef HAVE_STRUCT_STATVFS_F_FSTYPENAME */

/* Define to 1 if you have the <syslog.h> header file. */
#define HAVE_SYSLOG_H 1

/* Define to 1 if you have the <sys/param.h> header file. */
#define HAVE_SYS_PARAM_H 1

/* Define to 1 if you have the <sys/select.h> header file. */
#define HAVE_SYS_SELECT_H 1

/* Define to 1 if you have the <sys/socket.h> header file. */
#define HAVE_SYS_SOCKET_H 1

/* Define to 1 if you have the <sys/stat.h> header file. */
#define HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/time.h> header file. */
#define HAVE_SYS_TIME_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1

/* Define to 1 if you have the <sys/uio.h> header file. */
#define HAVE_SYS_UIO_H 1

/* Define to 1 if you have the <sys/un.h> header file. */
#define HAVE_SYS_UN_H 1

/* Define to 1 if you have the <sys/wait.h> header file. */
#define HAVE_SYS_WAIT_H 1

/* Define to 1 if you have the <time.h> header file. */
#define HAVE_TIME_H 1

/* Define to 1 if the system has the type `uint16_t'. */
#define HAVE_UINT16_T 1

/* Define to 1 if the system has the type `uint32_t'. */
#define HAVE_UINT32_T 1

/* Define to 1 if the system has the type `uint64_t'. */
#define HAVE_UINT64_T 1

/* Define to 1 if the system has the type `uint8_t'. */
#define HAVE_UINT8_T 1

/* Define to 1 if the system has the type `uintptr_t'. */
#define HAVE_UINTPTR_T 1

/* Define to 1 if you have the <unistd.h> header file. */
#define HAVE_UNISTD_H 1

/* whether unix byteswap routines -- htonl, htons, nothl, ntohs -- are
   available */
#define HAVE_UNIX_BYTESWAP 1

/* Define to 1 if you have the `usleep' function. */
#define HAVE_USLEEP 1

/* Define to 1 if you have the `vasprintf' function. */
#define HAVE_VASPRINTF 1

/* Define to 1 if you have the `vsnprintf' function. */
#define HAVE_VSNPRINTF 1

/* Define to the sub-directory in which libtool stores uninstalled libraries.
   */
#define LT_OBJDIR ".libs/"

/* Define to 1 if your C compiler doesn't accept -c and -o together. */
/* #undef NO_MINUS_C_MINUS_O */

/* Define to the address where bug reports for this package should be sent. */
#define PACKAGE_BUGREPORT "http://www.open-mpi.org/projects/pmix/"

/* Define to the full name of this package. */
#define PACKAGE_NAME "pmix"

/* Define to the full name and version of this package. */
#define PACKAGE_STRING "pmix 1.1.2"

/* Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME "pmix"

/* Define to the home page for this package. */
#define PACKAGE_URL ""

/* Define to the version of this package. */
#define PACKAGE_VERSION "1.1.2"

/* Alignment of type bool */
#define PMIX_ALIGNMENT_BOOL 1

/* Alignment of type char */
#define PMIX_ALIGNMENT_CHAR 1

/* Alignment of type double */
#define PMIX_ALIGNMENT_DOUBLE 8

/* Alignment of type float */
#define PMIX_ALIGNMENT_FLOAT 4

/* Alignment of type int */
#define PMIX_ALIGNMENT_INT 4

/* Alignment of type int16_t */
#define PMIX_ALIGNMENT_INT16 2

/* Alignment of type int32_t */
#define PMIX_ALIGNMENT_INT32 4

/* Alignment of type int64_t */
#define PMIX_ALIGNMENT_INT64 8

/* Alignment of type int8_t */
#define PMIX_ALIGNMENT_INT8 1

/* Alignment of type long */
#define PMIX_ALIGNMENT_LONG 8

/* Alignment of type long double */
/* #undef PMIX_ALIGNMENT_LONG_DOUBLE */

/* Alignment of type long long */
#define PMIX_ALIGNMENT_LONG_LONG 8

/* Alignment of type short */
#define PMIX_ALIGNMENT_SHORT 2

/* Alignment of type size_t */
#define PMIX_ALIGNMENT_SIZE_T 8

/* Alignment of type void * */
#define PMIX_ALIGNMENT_VOID_P 8

/* Alignment of type wchar_t */
#define PMIX_ALIGNMENT_WCHAR 4

/* The compiler $lower which PMIx was built with */
#define PMIX_BUILD_PLATFORM_COMPILER_FAMILYID 0

/* The compiler $lower which PMIX was built with */
#define PMIX_BUILD_PLATFORM_COMPILER_FAMILYNAME UNKNOWN

/* The compiler $lower which PMIx was built with */
#define PMIX_BUILD_PLATFORM_COMPILER_VERSION 0

/* The compiler $lower which PMIX was built with */
#define PMIX_BUILD_PLATFORM_COMPILER_VERSION_STR UNKNOWN

/* PMIx underlying C compiler */
#define PMIX_CC "gcc"

/* Use static const char[] strings for C files */
#define PMIX_CC_USE_CONST_CHAR_IDENT 0

/* Use #ident strings for C files */
#define PMIX_CC_USE_IDENT 1

/* Use #pragma comment for C files */
#define PMIX_CC_USE_PRAGMA_COMMENT 

/* Use #pragma ident strings for C files */
#define PMIX_CC_USE_PRAGMA_IDENT 0

/* Whether C compiler supports __builtin_clz */
#define PMIX_C_HAVE_BUILTIN_CLZ 1

/* Whether C compiler supports __builtin_expect */
#define PMIX_C_HAVE_BUILTIN_EXPECT 1

/* Whether C compiler supports __builtin_prefetch */
#define PMIX_C_HAVE_BUILTIN_PREFETCH 1

/* Whether C compiler supports symbol visibility or not */
#define PMIX_C_HAVE_VISIBILITY 1

/* Whether we are in debugging mode or not */
#define PMIX_ENABLE_DEBUG 0

/* Whether we want developer-level timing support or not */
#define PMIX_ENABLE_TIMING 0

/* Location of event2/thread.h */
#define PMIX_EVENT2_THREAD_HEADER "opal/mca/event/libevent2022/libevent2022.h"

/* Location of event.h */
#define PMIX_EVENT_HEADER "opal/mca/event/libevent2022/libevent2022.h"

/* Whether your compiler has __attribute__ or not */
#define PMIX_HAVE_ATTRIBUTE 1

/* Whether your compiler has __attribute__ aligned or not */
#define PMIX_HAVE_ATTRIBUTE_ALIGNED 1

/* Whether your compiler has __attribute__ always_inline or not */
#define PMIX_HAVE_ATTRIBUTE_ALWAYS_INLINE 1

/* Whether your compiler has __attribute__ cold or not */
#define PMIX_HAVE_ATTRIBUTE_COLD 1

/* Whether your compiler has __attribute__ const or not */
#define PMIX_HAVE_ATTRIBUTE_CONST 1

/* Whether your compiler has __attribute__ deprecated or not */
#define PMIX_HAVE_ATTRIBUTE_DEPRECATED 1

/* Whether your compiler has __attribute__ deprecated with optional argument
   */
#define PMIX_HAVE_ATTRIBUTE_DEPRECATED_ARGUMENT 1

/* Whether your compiler has __attribute__ destructor or not */
#define PMIX_HAVE_ATTRIBUTE_DESTRUCTOR 1

/* Whether your compiler has __attribute__ format or not */
#define PMIX_HAVE_ATTRIBUTE_FORMAT 1

/* Whether your compiler has __attribute__ format and it works on function
   pointers */
#define PMIX_HAVE_ATTRIBUTE_FORMAT_FUNCPTR 1

/* Whether your compiler has __attribute__ hot or not */
#define PMIX_HAVE_ATTRIBUTE_HOT 1

/* Whether your compiler has __attribute__ malloc or not */
#define PMIX_HAVE_ATTRIBUTE_MALLOC 1

/* Whether your compiler has __attribute__ may_alias or not */
#define PMIX_HAVE_ATTRIBUTE_MAY_ALIAS 1

/* Whether your compiler has __attribute__ nonnull or not */
#define PMIX_HAVE_ATTRIBUTE_NONNULL 1

/* Whether your compiler has __attribute__ noreturn or not */
#define PMIX_HAVE_ATTRIBUTE_NORETURN 1

/* Whether your compiler has __attribute__ noreturn and it works on function
   pointers */
#define PMIX_HAVE_ATTRIBUTE_NORETURN_FUNCPTR 1

/* Whether your compiler has __attribute__ no_instrument_function or not */
#define PMIX_HAVE_ATTRIBUTE_NO_INSTRUMENT_FUNCTION 1

/* Whether your compiler has __attribute__ packed or not */
#define PMIX_HAVE_ATTRIBUTE_PACKED 1

/* Whether your compiler has __attribute__ pure or not */
#define PMIX_HAVE_ATTRIBUTE_PURE 1

/* Whether your compiler has __attribute__ sentinel or not */
#define PMIX_HAVE_ATTRIBUTE_SENTINEL 1

/* Whether your compiler has __attribute__ unused or not */
#define PMIX_HAVE_ATTRIBUTE_UNUSED 1

/* Whether your compiler has __attribute__ visibility or not */
#define PMIX_HAVE_ATTRIBUTE_VISIBILITY 1

/* Whether your compiler has __attribute__ warn unused result or not */
#define PMIX_HAVE_ATTRIBUTE_WARN_UNUSED_RESULT 1

/* Whether your compiler has __attribute__ weak alias or not */
#define PMIX_HAVE_ATTRIBUTE_WEAK_ALIAS 

/* whether qsort is broken or not */
#define PMIX_HAVE_BROKEN_QSORT 0

/* whether ceil is found and available */
#define PMIX_HAVE_CEIL 1

/* whether dirname is found and available */
#define PMIX_HAVE_DIRNAME 1

/* Whether we have hwloc support or not */
#define PMIX_HAVE_HWLOC 1

/* Whether we have munge support or not */
#define PMIX_HAVE_MUNGE 0

/* Whether we have sasl support or not */
#define PMIX_HAVE_SASL 0

/* Whether we have SA_RESTART in <signal.h> or not */
#define PMIX_HAVE_SA_RESTART 1

/* whether socket is found and available */
#define PMIX_HAVE_SOCKET 1

/* Whether we have __va_copy or not */
#define PMIX_HAVE_UNDERSCORE_VA_COPY 1

/* Whether we have va_copy or not */
#define PMIX_HAVE_VA_COPY 1

/* Location of hwloc.h */
#define PMIX_HWLOC_HEADER "opal/mca/hwloc/hwloc1112/hwloc1112.h"

/* ident string for PMIX */
#define PMIX_IDENT_STRING ""

/* Whether the C compiler supports "bool" without any other help (such as
   <stdbool.h>) */
#define PMIX_NEED_C_BOOL 1

/* type to use for ptrdiff_t */
#define PMIX_PTRDIFF_TYPE ptrdiff_t

/* The pmix symbol prefix */
#define PMIX_SYM_PREFIX opal_pmix_pmix112_

/* The pmix symbol prefix in all caps */
#define PMIX_SYM_PREFIX_CAPS OPAL_PMIX_PMIX112_

/* Whether we need to re-define all the pmix public symbols or not */
#define PMIX_SYM_TRANSFORM 1

/* Whether to use <stdbool.h> or not */
#define PMIX_USE_STDBOOL_H 1

/* The library version is always available, contrary to VERSION */
#define PMIX_VERSION "1.1.2"

/* if want pretty-print stack trace feature */
#define PMIX_WANT_PRETTY_PRINT_STACKTRACE 1

/* The size of `char', as computed by sizeof. */
#define SIZEOF_CHAR 1

/* The size of `double', as computed by sizeof. */
#define SIZEOF_DOUBLE 8

/* The size of `float', as computed by sizeof. */
#define SIZEOF_FLOAT 4

/* The size of `int', as computed by sizeof. */
#define SIZEOF_INT 4

/* The size of `long', as computed by sizeof. */
#define SIZEOF_LONG 8

/* The size of `long long', as computed by sizeof. */
#define SIZEOF_LONG_LONG 8

/* The size of `pid_t', as computed by sizeof. */
#define SIZEOF_PID_T 4

/* The size of `ptrdiff_t', as computed by sizeof. */
#define SIZEOF_PTRDIFF_T 8

/* The size of `short', as computed by sizeof. */
#define SIZEOF_SHORT 2

/* The size of `size_t', as computed by sizeof. */
#define SIZEOF_SIZE_T 8

/* The size of `ssize_t', as computed by sizeof. */
/* #undef SIZEOF_SSIZE_T */

/* The size of `void *', as computed by sizeof. */
#define SIZEOF_VOID_P 8

/* The size of `wchar_t', as computed by sizeof. */
#define SIZEOF_WCHAR_T 4

/* The size of `_Bool', as computed by sizeof. */
#define SIZEOF__BOOL 1

/* Define to 1 if you have the ANSI C header files. */
#define STDC_HEADERS 1

/* Enable extensions on AIX 3, Interix.  */
#ifndef _ALL_SOURCE
# define _ALL_SOURCE 1
#endif
/* Enable GNU extensions on systems that have them.  */
#ifndef _GNU_SOURCE
# define _GNU_SOURCE 1
#endif
/* Enable threading extensions on Solaris.  */
#ifndef _POSIX_PTHREAD_SEMANTICS
# define _POSIX_PTHREAD_SEMANTICS 1
#endif
/* Enable extensions on HP NonStop.  */
#ifndef _TANDEM_SOURCE
# define _TANDEM_SOURCE 1
#endif
/* Enable general extensions on Solaris.  */
#ifndef __EXTENSIONS__
# define __EXTENSIONS__ 1
#endif


/* Define WORDS_BIGENDIAN to 1 if your processor stores words with the most
   significant byte first (like Motorola and SPARC, unlike Intel). */
#if defined AC_APPLE_UNIVERSAL_BUILD
# if defined __BIG_ENDIAN__
#  define WORDS_BIGENDIAN 1
# endif
#else
# ifndef WORDS_BIGENDIAN
/* #  undef WORDS_BIGENDIAN */
# endif
#endif

/* Enable GNU extensions on systems that have them.  */
#ifndef _GNU_SOURCE
# define _GNU_SOURCE 1
#endif

/* Define to 1 if on MINIX. */
/* #undef _MINIX */

/* Define to 2 if the system does not provide POSIX.1 features except with
   this defined. */
/* #undef _POSIX_1_SOURCE */

/* Define to 1 if you need to in order for `stat' and other things to work. */
/* #undef _POSIX_SOURCE */

/* Define to `__inline__' or `__inline' if that's what the C compiler
   calls it, or to nothing if 'inline' is not supported under any name.  */
#ifndef __cplusplus
#define inline __inline__
#endif


#include <pmix/autogen/pmix_config_bottom.h>
#endif /* PMIX_CONFIG_H */

