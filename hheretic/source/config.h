/* include/config.h.in.  Generated from configure.in by autoheader.  */

/* Define if building universal (internal helper macro) */
#undef AC_APPLE_UNIVERSAL_BUILD

/* Define to one of `_getb67', `GETB67', `getb67' for Cray-2 and Cray-YMP
   systems. This function is required for `alloca.c' support on those systems.
   */
#undef CRAY_STACKSEG_END

/* Define to 1 if using `alloca.c'. */
#undef C_ALLOCA

/* Define if want to run fullscreen by default instead of windowed */
#define FULLSCREEN_DEFAULT

/* Define to 1 if you have `alloca', as a function or macro. */
#undef HAVE_ALLOCA

/* Define to 1 if you have <alloca.h> and it should be used (not on Ultrix).
   */
#undef HAVE_ALLOCA_H

/* Define to 1 if you have the <dlfcn.h> header file. */
#undef HAVE_DLFCN_H

/* Define to 1 if you have the <inttypes.h> header file. */
#undef HAVE_INTTYPES_H

/* Define to 1 if you have the `m' library (-lm). */
#define HAVE_LIBM

/* Define to 1 if you have the <linux/cdrom.h> header file. */
#undef HAVE_LINUX_CDROM_H

/* Define to 1 if you have the <linux/soundcard.h> header file. */
#undef HAVE_LINUX_SOUNDCARD_H

/* Define to 1 if you have the <machine/soundcard.h> header file. */
#undef HAVE_MACHINE_SOUNDCARD_H

/* Define to 1 if you have the <memory.h> header file. */
#define HAVE_MEMORY_H

/* Define to 1 if you have the <soundcard.h> header file. */
#undef HAVE_SOUNDCARD_H

/* Define to 1 if you have the <stdint.h> header file. */
#define HAVE_STDINT_H

/* Define to 1 if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H

/* Define to 1 if you have the <strings.h> header file. */
#undef HAVE_STRINGS_H

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H

/* Define this if C symbols are prefixed with an underscore */
#undef HAVE_SYM_PREFIX_UNDERSCORE

/* Define to 1 if you have the <sys/cdio.h> header file. */
#undef HAVE_SYS_CDIO_H

/* Define to 1 if you have the <sys/soundcard.h> header file. */
#undef HAVE_SYS_SOUNDCARD_H

/* Define to 1 if you have the <sys/stat.h> header file. */
#define HAVE_SYS_STAT_H

/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H

/* Define to 1 if you have the <unistd.h> header file. */
#undef HAVE_UNISTD_H

/* Define this if the GCC __builtin_expect keyword is available */
#undef HAVE___BUILTIN_EXPECT
#ifndef HAVE___BUILTIN_EXPECT
# define __builtin_expect(x,c) x
#endif

/* Define to not compile most parameter validation debugging code */
#undef NORANGECHECKING

/* Define to the address where bug reports for this package should be sent. */
#undef PACKAGE_BUGREPORT

/* Define to the full name of this package. */
#undef PACKAGE_NAME

/* Define to the full name and version of this package. */
#undef PACKAGE_STRING

/* Define to the one symbol short name of this package. */
#undef PACKAGE_TARNAME

/* Define to the home page for this package. */
#undef PACKAGE_URL

/* Define to the version of this package. */
#undef PACKAGE_VERSION

/* Define if building with OpenGL support */
#undef RENDER3D

/* Define this to the shared game directory root */
#undef SHARED_DATAPATH

/* The size of `char', as computed by sizeof. */
#define SIZEOF_CHAR sizeof(char)

/* The size of `double', as computed by sizeof. */
#define SIZEOF_DOUBLE sizeof(double)


/* The size of `float', as computed by sizeof. */
#define SIZEOF_FLOAT sizeof(float)

/* The size of `int', as computed by sizeof. */
#define SIZEOF_INT sizeof(int)

/* The size of `long', as computed by sizeof. */
#define SIZEOF_LONG sizeof(long int)

/* The size of `long double', as computed by sizeof. */
#undef SIZEOF_LONG_DOUBLE

/* The size of `long long', as computed by sizeof. */
#define SIZEOF_LONG_LONG sizeof(long long)

/* The size of `short', as computed by sizeof. */
#define SIZEOF_SHORT sizeof(short)

/* The size of `size_t', as computed by sizeof. */
#define SIZEOF_SIZE_T sizeof(size_t)

/* The size of `void *', as computed by sizeof. */
#define SIZEOF_VOID_P sizeof(void *)

/* If using the C implementation of alloca, define if you know the
   direction of stack growth for your system; otherwise it will be
   automatically deduced at runtime.
	STACK_DIRECTION > 0 => grows toward higher addresses
	STACK_DIRECTION < 0 => grows toward lower addresses
	STACK_DIRECTION = 0 => direction of growth unknown */
#undef STACK_DIRECTION

/* Define to 1 if the `S_IS*' macros in <sys/stat.h> do not work properly. */
#undef STAT_MACROS_BROKEN

/* Define to 1 if you have the ANSI C header files. */
#undef STDC_HEADERS

/* Define WORDS_BIGENDIAN to 1 if your processor stores words with the most
   significant byte first (like Motorola and SPARC, unlike Intel). */
#if defined AC_APPLE_UNIVERSAL_BUILD
# if defined __BIG_ENDIAN__
#  define WORDS_BIGENDIAN 1
# endif
#else
# ifndef WORDS_BIGENDIAN
#  undef WORDS_BIGENDIAN
# endif
#endif

#define WORDS_BIGENDIAN 1


/* Define if not want to use assembly language */
#undef _DISABLE_ASM

/* Define if want to disable user directories */
#undef _NO_USERDIRS

/* For pthread support in OSS audio code. */
#ifndef _REENTRANT
# undef _REENTRANT
#endif

/* For pthread support in OSS audio code. */
#ifndef _THREAD_SAFE
# undef _THREAD_SAFE
#endif

/* Define to 1 if type `char' is unsigned and you are not using gcc.  */
#ifndef __CHAR_UNSIGNED__
# undef __CHAR_UNSIGNED__
#endif

/* Define to empty if `const' does not conform to ANSI C. */
#undef const

/* Define to `__inline__' or `__inline' if that's what the C compiler
   calls it, or to nothing if 'inline' is not supported under any name.  */
#ifndef __cplusplus
#undef inline
#endif

/* Define to `unsigned int' if <sys/types.h> does not define. */
#undef size_t
