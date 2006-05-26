/*
 * Note that this file is directly from the asterisk project (see header
 * below, although I removed some of the DEFINEs that are used by the code.
 *
 * asterisk version was:  asterisk-1.2.7.1 ; 2006-04-28
 * original filename was: include/asterisk/time.h
 */

/*
 * Asterisk -- An open source telephony toolkit.
 *
 * Copyright (C) 1999 - 2005, Digium, Inc.
 *
 * Mark Spencer <markster@digium.com>
 *
 * See http://www.asterisk.org for more information about
 * the Asterisk project. Please do not directly contact
 * any of the maintainers of this project for assistance;
 * the project provides a web site, mailing lists and IRC
 * channels for your use.
 *
 * This program is free software, distributed under the terms of
 * the GNU General Public License Version 2. See the LICENSE file
 * at the top of the source tree.
 */

/*! \file
 * \brief Time-related functions and macros
 */

#ifndef _ASTERISK_TIME_H
#define _ASTERISK_TIME_H

#define ONE_MILLION  1000000
#define ONE_THOUSAND    1000

#include <stdio.h>       /* amusingly, for NULL, one needs stdio.h */
#include <sys/time.h>

/* We have to let the compiler learn what types to use for the elements of a
   struct timeval since on linux, it's time_t and suseconds_t, but on *BSD,
   they are just a long. */
extern struct timeval tv;
typedef typeof(tv.tv_sec) ast_time_t;
typedef typeof(tv.tv_usec) ast_suseconds_t;

extern int ast_tvdiff_us(struct timeval end, struct timeval start);
extern int ast_tvdiff_ms(struct timeval end, struct timeval start);
extern int ast_tvzero(const struct timeval t);
extern int ast_tvcmp(struct timeval _a, struct timeval _b);
extern int ast_tveq(struct timeval _a, struct timeval _b);
extern struct timeval ast_tvnow(void);
extern struct timeval ast_set_tvzero( void );
extern struct timeval ast_tv(ast_time_t sec, ast_suseconds_t usec);
extern struct timeval ast_samp2tv(unsigned int _nsamp, unsigned int _rate);
extern struct timeval ast_tvadd(struct timeval a, struct timeval b);
extern struct timeval ast_tvsub(struct timeval a, struct timeval b);



#endif /* _ASTERISK_TIME_H */
