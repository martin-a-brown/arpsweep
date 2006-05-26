/*
 * log.h
 * 
 * Martin A. Brown <martin@linux-ip.net>
 *
 */

/*
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 */

#ifndef _LOG_H                                             /* _LOG_H */
#define _LOG_H

extern void short_usage(int ret);
extern void print_version(int ret);
extern void long_usage(int ret);
extern int add_loglevel(int level_change );
extern int loglevel(char * v_option );
extern void arpsweep_info_header();


#endif                                                     /* _LOG_H */

/* eof */
