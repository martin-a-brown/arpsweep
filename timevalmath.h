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

#include <sys/time.h>

/* We have to let the compiler learn what types to use for the elements of a
   struct timeval since on linux, it's time_t and suseconds_t, but on *BSD,
   they are just a long. */
extern struct timeval tv;
typedef typeof(tv.tv_sec) ast_time_t;
typedef typeof(tv.tv_usec) ast_suseconds_t;

/*!
 * \brief Computes the difference (in milliseconds) between two \c struct \c timeval instances.
 * \param end the beginning of the time period
 * \param start the end of the time period
 * \return the difference in milliseconds
 */
int ast_tvdiff_us(struct timeval end, struct timeval start)
{
  /* the offset by 1,000,000 below is intentional...
     it avoids differences in the way that division
     is handled for positive and negative numbers, by ensuring
     that the divisor is always positive
  */
  return  ( ( (end.tv_sec - start.tv_sec) * ONE_MILLION ) +
    ( ONE_MILLION + end.tv_usec - start.tv_usec ) - ONE_MILLION );
}

/*!
 * \brief Computes the difference (in milliseconds) between two \c struct \c timeval instances.
 * \param end the beginning of the time period
 * \param start the end of the time period
 * \return the difference in milliseconds
 */
int ast_tvdiff_ms(struct timeval end, struct timeval start)
{
  /* the offset by 1,000,000 below is intentional...
     it avoids differences in the way that division
     is handled for positive and negative numbers, by ensuring
     that the divisor is always positive
  */
  return  ( (end.tv_sec - start.tv_sec) * ONE_THOUSAND ) +
    ((( ONE_MILLION + end.tv_usec - start.tv_usec ) / ONE_THOUSAND ) 
                                                           - ONE_THOUSAND );
}

/*!
 * \brief Returns true if the argument is 0,0
 */
int ast_tvzero(const struct timeval t)
{
	return (t.tv_sec == 0 && t.tv_usec == 0);
}

/*!
 * \brief Compres two \c struct \c timeval instances returning
 * -1, 0, 1 if the first arg is smaller, equal or greater to the second.
 */
int ast_tvcmp(struct timeval _a, struct timeval _b)
{
	if (_a.tv_sec < _b.tv_sec)
		return -1;
	if (_a.tv_sec > _b.tv_sec)
		return 1;
	/* now seconds are equal */
	if (_a.tv_usec < _b.tv_usec)
		return -1;
	if (_a.tv_usec > _b.tv_usec)
		return 1;
	return 0;
}

/*!
 * \brief Returns true if the two \c struct \c timeval arguments are equal.
 */
int ast_tveq(struct timeval _a, struct timeval _b)
{
	return (_a.tv_sec == _b.tv_sec && _a.tv_usec == _b.tv_usec);
}

/*!
 * \brief Returns current timeval. Meant to replace calls to gettimeofday().
 */
struct timeval ast_tvnow(void)
{
	struct timeval t;
	gettimeofday(&t, NULL);
	return t;
}

/*!
 * \brief Returns a zeroed timeval struct.
 */
struct timeval ast_set_tvzero( void )
{
	struct timeval t;
	t.tv_sec = 0;
        t.tv_usec = 0;
	return t;
}

/*!
 * \brief Returns the sum of two timevals a + b
 */
//struct timeval ast_tvadd(struct timeval a, struct timeval b);

/*!
 * \brief Returns the difference of two timevals a - b
 */
//struct timeval ast_tvsub(struct timeval a, struct timeval b);

/*!
 * \brief Returns a timeval from sec, usec
 */
struct timeval ast_tv(ast_time_t sec, ast_suseconds_t usec)
{
	struct timeval t;
	t.tv_sec = sec;
	t.tv_usec = usec;
	return t;
}

/*!
 * \brief Returns a timeval corresponding to the duration of n samples at rate r.
 * Useful to convert samples to timevals, or even milliseconds to timevals
 * in the form ast_samp2tv(milliseconds, 1000)
 */
struct timeval ast_samp2tv(unsigned int _nsamp, unsigned int _rate)
{
	return ast_tv(_nsamp / _rate, (_nsamp % _rate) * (1000000 / _rate));
}



/*
 * put timeval in a valid range. usec is 0..999999
 * negative values are not allowed and truncated.
 */
static struct timeval tvfix(struct timeval a)
{
   if (a.tv_usec >= ONE_MILLION) {
      //DEBUG( "warning too large timestamp %ld.%ld\n",
      //    a.tv_sec, (long int) a.tv_usec);
      a.tv_sec += a.tv_usec / ONE_MILLION;
      a.tv_usec %= ONE_MILLION;
   } else if (a.tv_usec < 0) {
      //DEBUG( "warning negative timestamp %ld.%ld\n",
      //   a.tv_sec, (long int) a.tv_usec);
      a.tv_usec = 0;
   }
   return a;
}

struct timeval ast_tvadd(struct timeval a, struct timeval b)
{
   /* consistency checks to guarantee usec in 0..999999 */
   a = tvfix(a);
   b = tvfix(b);
   a.tv_sec += b.tv_sec;
   a.tv_usec += b.tv_usec;
   if (a.tv_usec >= ONE_MILLION) {
      a.tv_sec++;
      a.tv_usec -= ONE_MILLION;
   }
   return a;
}

struct timeval ast_tvsub(struct timeval a, struct timeval b)
{
   /* consistency checks to guarantee usec in 0..999999 */
   a = tvfix(a);
   b = tvfix(b);
   a.tv_sec -= b.tv_sec;
   a.tv_usec -= b.tv_usec;
   if (a.tv_usec < 0) {
      a.tv_sec-- ;
      a.tv_usec += ONE_MILLION;
   }
   return a;
}

#endif /* _ASTERISK_TIME_H */
