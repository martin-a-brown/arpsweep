/*
 * list.h
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

#ifndef _LIST_H                                             /* _LIST_H */
#define _LIST_H

#include <sys/time.h>
#include <netinet/in.h>
#include <net/ethernet.h>

#include "defines.h"

struct arp_record {
  struct arp_record               *next;
  struct arp_record               *prev;
  unsigned long            member_count;
  int                             flags;
  long                          numsent;
  long                          numrecv;
  int                         last_time;
  int                             delay;
  struct timeval              sent_time;
  struct timeval            expire_time;
  struct in_addr                     ip; /* ip.s_addr is network byte order */
  u_int32_t                      ipaddr;
  u_int8_t       lladdr[ETHER_ADDR_LEN];
  u_int8_t   src_lladdr[ETHER_ADDR_LEN];
};


extern void inline list_prepend( struct arp_record ** head, struct arp_record * cur );
extern void inline  list_append( struct arp_record ** head, struct arp_record * cur );
extern void inline  list_remove( struct arp_record ** head, struct arp_record * cur );
extern void inline    list_move( struct arp_record ** src,  struct arp_record ** dst );

extern int list_length( struct arp_record * cur );

#endif                                                          /* _LIST_H */

/* eof */
