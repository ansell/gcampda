/** 
 *
 *   Common stdio function implementation.
 *
 *   (c) Copyright 2004 Federico Bareilles <fede@iar.unlp.edu.ar>,
 *   Instituto Argentino de Radio Astronomia (IAR).
 *
 *   This program is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License as
 *   published by the Free Software Foundation; either version 2 of
 *   the License, or (at your option) any later version.
 *     
 *   The author does NOT admit liability nor provide warranty for any
 *   of this software. This material is provided "AS-IS" in the hope
 *   that it may be useful for others.
 *
 **/

#ifndef _MY_STDIO_H
#define _MY_STDIO_H

#include <stdio.h>
#ifndef WIN32
# include <syslog.h>
#endif

extern int syslog_flag;  /* in flags.c */
#ifdef VERBOSE_MODE
extern int verbose_flag; /* in flags.c */
#endif
#ifdef DEBUG_MODE
extern int debug_flag;   /* in flags.c */
#endif

#ifdef VERBOSE_MODE
#  define INCREMENT_VERBOSE_FLAG() verbose_flag = (verbose_flag << 1) | 1
#else
#  define INCREMENT_VERBOSE_FLAG()
#endif

#undef SET_VERBOSE_FLAG
#ifdef VERBOSE_MODE
#  define SET_VERBOSE_FLAG(level) {                               \
        verbose_flag = 0;                                         \
        if (level) while(!(verbose_flag & level))                 \
                        INCREMENT_VERBOSE_FLAG();                 \
}
#else
#  define SET_VERBOSE_FLAG(level)
#endif

#undef PVERB
#define PERR   1
#define PWARN  2
#define PINFO  4
#ifdef VERBOSE_MODE
#  ifdef DEBUG_MODE 
#    define PVERB(level, fmt, args...) \
     if (verbose_flag & level) \
         fprintf(stderr, __FILE__ ":%s - " fmt "\n", __FUNCTION__ , ## args)
/**
 http://unixpapa.com/incnote/variadic.html: 
 Here the ## causes the preceding group of non-space characters (in
   older versions of gcc) or the preceding comma (in newer versions)
   to be deleted if args is empty. For maximum portability, the comma
   should be surrounded by white space.
**/
#  else
#    define PVERB(level, fmt, args...) \
     if (verbose_flag & level) \
         fprintf(stderr, fmt "\n", ## args)
#  endif
#else
#  define PVERB(level, fmt, args...)
#endif


#undef PDBG
#ifdef DEBUG_MODE
#  define PDBG(fmt, args...) \
          if (debug_flag) \
             fprintf(stderr, __FILE__ ":%s - " fmt "\n", __FUNCTION__ , ## args)
#else
#  define PDBG(fmt, args...);
#endif


#ifndef WIN32 
#define SET_SYSLOG_FLAG(flag) syslog_flag = flag;

#undef OPENLOG
#define OPENLOG(ident, option, facility) { \
	openlog(ident, option, facility);  \
	SET_SYSLOG_FLAG(1);                \
}

#undef CLOSELOG
#define CLOSELOG() { closelog(); SET_SYSLOG_FLAG(0);}

#undef SYSLOG
#ifdef VERBOSE_MODE
#  define SYSLOG(type, fmt, args...) {                            \
        int __level_;                                             \
        if(syslog_flag) syslog(type, fmt , ## args);               \
	switch( type ) {					  \
	case LOG_EMERG:   /* system is unusable */		  \
	case LOG_ERR:     /* error conditions */		  \
	case LOG_CRIT:    /* critical conditions */		  \
	case LOG_ALERT:   /* action must be taken immediately */  \
		__level_ = PERR; 			          \
		break;						  \
	case LOG_WARNING: /* warning conditions */		  \
		__level_ = PWARN;			          \
		break;						  \
	case LOG_NOTICE:  /* normal, but significant, condition */\
	case LOG_INFO:    /* informational message */		  \
	case LOG_DEBUG:   /* debug-level mess */		  \
        default:                                                  \
		__level_ = PINFO;			          \
		break;						  \
	}                                                         \
	PVERB(__level_, fmt , ## args);	 		          \
	/* PVERB(__level_, "\n"); */	                          \
}
#else
#  define SYSLOG(type, fmt, args...) if(syslog_flag) syslog(type, fmt , ## args)
#endif

#endif /* WIN32 */

#ifndef WHITESPACE
#  define WHITESPACE(c) (((c) == ' ') || ((c) == '\t'))
#endif


int fd_printf(int fd, const char *fmt, ...);
char * stripwhite (char *string);

#endif  /* ifndef _MY_STDIO_H */
