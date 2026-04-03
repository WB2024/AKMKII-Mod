#ifndef SCG_CONFIG_INCLUDED
#define SCG_CONFIG_INCLUDED

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <stdarg.h>


#ifdef LINUX_X86
  #define HOST_CPU x86
  #define UNIX_INCLUDES
  #define EXPORT 
  #define LOCAL static
  #define HAVE_LINUX_PG_H 1   
  #define u_char unsigned char
  #define BOOL int
#define TRUE 1
#define FALSE 0
#endif


#include "errno.h"

#ifdef _WIN32
	#include <Windows.h>
	#include "win32.h"

	#define PACKED 
	#define EXPORT 
	#define LOCAL static
	#define Ucbit BYTE
	#define caddr_t BYTE*
	#define u_char char
	#define HOST_CPU x86
#endif

#ifdef UNIX_INCLUDES
  #include <sys/types.h>
  #include "utypes.h"
  #include <time.h>
  #include <sys/time.h>
  #include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/io.h>


//  #include <btorder.h>
#endif



#include "scsierrs.h"

#if(HOST_CPU==x86)
	#define _BIT_FIELDS_LTOH 1
#endif



#if 0


#ifdef	HAVE_SYS_PARAM_H
#include <sys/param.h>	/* XXX nonportable to use u_char */
#endif
#include <standard.h>
#include <stdxlib.h>
#include <unixstd.h>
#include <timedefs.h>
#include <sys/ioctl.h>
#include <fctldefs.h>



#endif





#ifdef	sun
#	define	HAVE_SCG	/* Currently only on SunOS/Solaris */
#endif


#if 0

#define	HAVE_C_BIGENDIAN
/* #undef WORDS_BIGENDIAN */
#define	HAVE_C_BITFIELDS
/* #undef BITFIELDS_HTOL */
/*
 * Define as the return type of signal handlers (int or void).
 */
#define RETSIGTYPE void

/*
 * Header Files
 */
#define PROTOTYPES 1	/* if Compiler supports ANSI C prototypes */
#define HAVE_STDARG_H 1	/* to use stdarg.h, else use varargs.h NOTE: SaberC on a Sun has prototypes but no stdarg.h */
#define HAVE_STDLIB_H 1	/* to use general utility defines (malloc(), size_t ...) and general C library prototypes */
#define HAVE_STRING_H 1	/* to get NULL and ANSI C string function prototypes */
#define HAVE_STRINGS_H 1	/* to get BSD string function prototypes */
#define STDC_HEADERS 1	/* if ANSI compliant stdlib.h, stdarg.h, string.h, float.h are present */
#define HAVE_UNISTD_H 1	/* to get POSIX syscall prototypes XXX sys/file.h fcntl.h (unixstd/fctl)XXX*/
#define HAVE_LIMITS_H 1	/* to get POSIX numeric limits constants */
#define HAVE_A_OUT_H 1	/* if a.out.h is present (may be a system using a.out format) */
/* #undef HAVE_AOUTHDR_H */	/* if aouthdr.h is present. This is a COFF system */
/* #undef HAVE_ELF_H */	/* if elf.h is present. This is an ELF system */
#define HAVE_FCNTL_H 1	/* to access, O_XXX constants for open(), otherwise use sys/file.h */
#define HAVE_DIRENT_H 1	/* to use POSIX dirent.h */
/* #undef HAVE_SYS_DIR_H */	/* to use BSD sys/dir.h */
/* #undef HAVE_NDIR_H */	/* to use ndir.h */
/* #undef HAVE_SYS_NDIR_H */	/* to use sys/ndir.h */
#define HAVE_MALLOC_H 1	/* if malloc.h exists */
#define HAVE_TERMIOS_H 1	/* to use POSIX termios.h */
#define HAVE_TERMIO_H 1	/* to use SVR4 termio.h */
#define HAVE_SYS_TIME_H 1	/* may include sys/time.h for struct timeval */
/* #undef TIME_WITH_SYS_TIME_H */	/* may include both time.h and sys/time.h */
#define HAVE_UTIMES 1		/* to use BSD utimes() and sys/time.h */
#define HAVE_UTIME_H 1		/* to use utime.h for the utimbuf structure declaration, else declare struct utimbuf yourself */
#define HAVE_SYS_IOCTL_H 1		/* if sys/ioctl.h is present */
#define HAVE_SYS_PARAM_H 1		/* if sys/param.h is present */
#define HAVE_MNTENT_H 1		/* if mntent.h is present */
/* #undef HAVE_SYS_MNTENT_H */	/* if sys/mntent.h is present */
/* #undef HAVE_SYS_MNTTAB_H */	/* if sys/mnttab.h is present */
#define HAVE_SYS_MOUNT_H 1		/* if sys/mount.h is present */
/* #undef HAVE_WAIT_H */		/* to use wait.h for prototypes and union wait */
#define HAVE_SYS_WAIT_H 1		/* else use sys/wait.h */
#define HAVE_SYS_RESOURCE_H 1	/* to use sys/resource.h for rlimit() and wait3() */
/* #undef HAVE_SYS_PROCFS_H */	/* to use sys/procfs.h for wait3() emulation */
/* #undef HAVE_SYS_SYSTEMINFO_H */	/* to use SVr4 sysinfo() */
#define HAVE_SYS_UTSNAME_H 1	/* to use uname() */
/* #undef HAVE_SYS_PRIOCNTL_H */	/* to use SVr4 priocntl() instead of nice()/setpriority() */
/* #undef HAVE_SYS_RTPRIOCNTL_H */	/* if the system supports SVr4 real time classes */
/* #undef HAVE_SYS_MTIO_H */		/* to use mtio definitions from sys/mtio.h */
#define HAVE_SYS_MMAN_H 1		/* to use definitions for mmap()/madvise()... from sys/mman.h */
/* #undef MAJOR_IN_MKDEV */		/* if we should include sys/mkdev.h to get major()/minor()/makedev() */
#define MAJOR_IN_SYSMACROS 1	/* if we should include sys/sysmacros.h to get major()/minor()/makedev() */
/* #undef HAVE_SYS_DKIO_H */		/* if we may include sys/dkio.h for disk ioctls */
/* #undef HAVE_SYS_DKLABEL_H */	/* if we may include sys/dklabel.h for disk label */
/* #undef HAVE_SUN_DKIO_H */		/* if we may include sun/dkio.h for disk ioctls */
/* #undef HAVE_SUN_DKLABEL_H */	/* if we may include sun/dklabel.h for disk label */
/* #undef HAVE_POLL_H */		/* if we may include poll.h to use poll() */
/* #undef HAVE_SYS_POLL_H */		/* if we may include sys/poll.h to use poll() */
/* #undef HAVE_LINUX_PG_H */		/* if we may include linux/pg.h for PP ATAPI sypport */
/* #undef HAVE_CAMLIB_H */		/* if we may include camlib.h for CAM SCSI transport definitions */
#define HAVE_IEEEFP_H 1		/* if we may include ieeefp.h for FINITE()/IS_INF()/IS_NAN() */
/* #undef HAVE_FP_H */		/* if we may include fp.h for FINITE()/IS_INF()/IS_NAN() */
/* #undef HAVE_VALUES_H */		/* if we may include values.h for MAXFLOAT */
#define HAVE_FLOAT_H 1		/* if we may include float.h for FLT_MAX */
/* #undef HAVE_USG_STDIO */		/* if we have USG derived STDIO */
/* #undef HAVE_VFORK_H */		/* if we should include vfork.h for vfork() definitions */

/*
 * Convert to SCHILY name
 */
#ifdef	STDC_HEADERS
#	ifndef	HAVE_STDC_HEADERS
#		define	HAVE_STDC_HEADERS
#	endif
#endif

#ifdef	HAVE_ELF_H
#define	HAVE_ELF			/* This system uses ELF */
#else
#	ifdef	HAVE_AOUTHDR_H
#	define	HAVE_COFF		/* This system uses COFF */
#	else
#		ifdef HAVE_A_OUT_H
#		define HAVE_AOUT	/* This system uses AOUT */
#		endif
#	endif
#endif

/*
 * Library Functions
 */
#define HAVE_STRERROR 1		/* strerror() is present in libc */
#define HAVE_MEMMOVE 1		/* memmove() is present in libc */
/* #undef HAVE_MLOCKALL */		/* working mlockall() is present in libc */
/* #undef HAVE_MMAP */		/* working mmap() is present in libc */
/* #undef HAVE_FLOCK */		/* flock() is present in libc */
/* #undef HAVE_FCHDIR */		/* fchdir() is present in libc */
/* #undef HAVE_STATVFS */		/* statvfs() is present in libc */
/* #undef HAVE_QUOTACTL */		/* quotactl() is present in libc */
/* #undef HAVE_QUOTAIOCTL */		/* use ioctl(f, Q_QUOTACTL, &q) instead of quotactl() */
/* #undef HAVE_SETREUID */		/* setreuid() is present in libc */
/* #undef HAVE_SETRESUID */		/* setresuid() is present in libc */
#define HAVE_SETEUID 1		/* seteuid() is present in libc */
#define HAVE_SETUID 1		/* setuid() is present in libc */
/* #undef HAVE_SETREGID */		/* setregid() is present in libc */
/* #undef HAVE_SETRESGID */		/* setresgid() is present in libc */
#define HAVE_SETEGID 1		/* setegid() is present in libc */
#define HAVE_SETGID 1		/* setgid() is present in libc */
#define HAVE_SYNC 1		/* sync() is present in libc */
/* #undef HAVE_WAIT3 */		/* working wait3() is present in libc */
#define HAVE_WAITPID 1		/* waitpid() is present in libc */
/* #undef HAVE_GETHOSTID */		/* gethostid() is present in libc */
#define HAVE_GETHOSTNAME 1		/* gethostname() is present in libc */
#define HAVE_GETDOMAINNAME 1	/* getdomainname() is present in libc */
#define HAVE_GETPAGESIZE 1		/* getpagesize() is present in libc */
#define HAVE_GETRUSAGE 1		/* getrusage() is present in libc */
/* #undef HAVE_POLL */		/* poll() is present in libc */
#define HAVE_SELECT 1		/* select() is present in libc */
/* #undef HAVE_LCHOWN */		/* lchown() is present in libc */
/* #undef HAVE_BRK */			/* brk() is present in libc */
#define HAVE_SBRK 1		/* sbrk() is present in libc */
/* #undef HAVE_VA_COPY */		/* va_copy() is present in varargs.h/stdarg.h */
#define HAVE__VA_COPY 1		/* __va_copy() is present in varargs.h/stdarg.h */
/* #undef HAVE_DTOA */		/* BSD-4.4 __dtoa() is present in libc */
#define HAVE_GETCWD 1		/* POSIX getcwd() is present in libc */
#define HAVE_SMMAP 1		/* may map anonymous memory to get shared mem */
/* #undef HAVE_SHMAT */		/* shmat() is present in libc */
/* #undef HAVE_SEMGET */		/* semget() is present in libc */
#define HAVE_ECVT 1		/* ecvt() is present in libc */
#define HAVE_FCVT 1		/* fcvt() is present in libc */
#define HAVE_GCVT 1		/* gcvt() is present in libc */
/* #undef HAVE_SETPRIORITY */		/* setpriority() is present in libc */
#define HAVE_NICE 1		/* nice() is present in libc */
/* #undef HAVE_DOSSETPRIORITY */	/* DosSetPriority() is present in libc */
/* #undef HAVE_SEEKDIR */		/* seekdir() is present in libc */
#define HAVE_PUTENV 1		/* putenv() is present in libc (preferred function) */
#define HAVE_SETENV 1		/* setenv() is present in libc (use instead of putenv()) */
#define HAVE_UNAME 1		/* uname() is present in libc */
#define HAVE_STRCASECMP 1		/* strcasecmp() is present in libc */
#define HAVE_STRSIGNAL 1		/* strsignal() is present in libc */
/* #undef HAVE_STR2SIG */		/* str2sig() is present in libc */
/* #undef HAVE_SIG2STR */		/* sig2str() is present in libc */
/* #undef HAVE_SIGRELSE */		/* sigrelse() is present in libc */
/* #undef HAVE_SYS_SIGLIST */		/* char *sys_siglist[] is present in libc */
/* #undef HAVE_NANOSLEEP */		/* nanosleep() is present in libc */
#define HAVE_USLEEP 1		/* usleep() is present in libc */
/* #undef vfork */

#if	defined(HAVE_QUOTACTL) || defined(HAVE_QUOTAIOCTL)
#	define HAVE_QUOTA	/* The system inludes quota */
#endif

#ifdef HAVE_SHMAT
#	define	HAVE_USGSHM	/* USG shared memory is present */
#endif
#ifdef HAVE_SEMGET
#	define	HAVE_USGSEM	/* USG semaphores are present */
#endif

/*
 * Structures
 */
/* #undef HAVE_MTGET_DSREG */		/* if struct mtget contains mt_dsreg (drive status) */
/* #undef HAVE_MTGET_RESID */		/* if struct mtget contains mt_resid (residual count) */
/* #undef HAVE_MTGET_FILENO */	/* if struct mtget contains mt_fileno (file #) */
/* #undef HAVE_MTGET_BLKNO */		/* if struct mtget contains mt_blkno (block #0) */
/* #undef HAVE_UNION_SEMUN */		/* have an illegal definition for union semun in sys/sem.h */
#define HAVE_UNION_WAIT 1		/* have union wait in wait.h */
#define HAVE_ST_SPARE1 1		/* if struct stat contains st_spare1 (usecs) */
/* #undef HAVE_ST_NSEC */		/* if struct stat contains st_atim.st_nsec (nanosecs */
#define HAVE_ST_BLKSIZE 1		/* if struct stat contains st_blksize */
#define HAVE_ST_BLOCKS 1		/* if struct stat contains st_blocks */
#define HAVE_ST_RDEV 1		/* if struct stat contains st_rdev */
/* #undef STAT_MACROS_BROKEN */	/* if the macros S_ISDIR, S_ISREG .. don't work */

/*
 * Types
 */
//#define HAVE_LONGLONG 1		/* Compiler defines long long type */
/*#undef HAVE_SIZE_T*/
/*#undef NO_SIZE_T*/
/* #undef VA_LIST_IS_ARRAY */		/* va_list is an array */
//#define GETGROUPS_T gid_t
//#define GID_T		GETGROUPS_T

#define HOST_VENDOR pc
#define HOST_OS cygwin32
#endif




#endif
