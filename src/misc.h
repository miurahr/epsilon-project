/*
 * $Id: misc.h,v 1.22 2010/02/05 23:50:22 simakov Exp $
 *
 * EPSILON - wavelet image compression library.
 * Copyright (C) 2006,2007,2010 Alexander Simakov, <xander@entropyware.info>
 *
 * This file is part of EPSILON
 *
 * EPSILON is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * EPSILON is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with EPSILON.  If not, see <http://www.gnu.org/licenses/>.
 *
 * http://epsilon-project.sourceforge.net
 */

#ifndef __MISC_H__
#define __MISC_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Use _snprintf instead of snprintf under MSVC compiler */
#if defined(_WIN32) && !defined(__MINGW32__)
#define snprintf    _snprintf
#endif

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#ifdef ENABLE_PTHREADS
# include <pthread.h>
#endif

/* Default suffix for e-PSI-lon files */
#define SUFFIX                  ".psi"

/* Maximal path length */
#define MAX_PATH                512

/* Maximal progress indicator length */
#define MAX_LINE                512

/* Maximal timer line */
#define MAX_TIMER_LINE          64

/* Directory separator */
#define DIR_SEPARATOR           '/'

/* fseeko jump size */
#define JUMBO_JUMP              2147483647

/* Threads settings */
#ifdef ENABLE_PTHREADS
/* Default number of threads */
# ifndef DEF_N_THREADS
#  define DEF_N_THREADS         2
# endif
/* Maximal number of threads */
# ifndef MAX_N_THREADS
#  define MAX_N_THREADS         512
# endif
#else
/* Thread-unaware version */
# undef DEF_N_THREADS
# define DEF_N_THREADS           1
# undef MAX_N_THREADS
# define MAX_N_THREADS           1
#endif

/* Maximal value */
#define MAX(_x, _y)             ((_x) > (_y) ? (_x) : (_y))
/* Minimal value */
#define MIN(_x, _y)             ((_x) < (_y) ? (_x) : (_y))
/* Absolute value */
#define ABS(_x)                 ((_x) >= 0 ? (_x) : -(_x))

/* Handy shortcuts */
#define EXIT_OR_RETURN(_x)      if (_x == OPT_YES) exit(1); else return
#define EXIT_OR_BREAK(_x)       if (_x == OPT_YES) exit(1); else break;
#define QUIET                   quiet == OPT_YES ? "" : "\n"

/* Locking mutexes */
#ifdef ENABLE_PTHREADS
# define LOCK(_x)               assert(!pthread_mutex_lock(&_x))
# define UNLOCK(_x)             assert(!pthread_mutex_unlock(&_x))
#else
# define LOCK(_x)
# define UNLOCK(_x)
#endif

/* Cast file offsets to off_t type */
#define _OFF(_x)                (off_t) (_x)

int power_of_two(int value);
int get_number_of_files(char **files);
char *format_time(int delta, char *buf, int nbytes);
void print_blank_line(int len);
void transform_2D_to_1D(unsigned char **src, unsigned char *dst, int w, int h);
void transform_1D_to_2D(unsigned char *src, unsigned char **dst, int w, int h);

#ifdef __cplusplus
}
#endif

#endif /* __MISC_H__ */
