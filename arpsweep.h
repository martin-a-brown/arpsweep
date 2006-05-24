/* arpsweep.h */
/* $Id:$ */

#ifndef _ARPSWEEP_H                                           /* _ARPSWEEP_H */
#define _ARPSWEEP_H


/* the usual includes for most apps and networking stuff */
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <strings.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>

#include <net/ethernet.h>
#include <netinet/in.h>
#include <arpa/inet.h>

/* all of the fancy packet-handling libraries for our needs */
#include <libnet.h>
#include <pcap.h>

#include "defines.h"
#include "timevalmath.h"

/* Yes, this boolean stuff was taken from GPL'd shuffle, by M.J. Pomraning. */

#if HAVE_STDBOOL_H
# include <stdbool.h>  /* truth */
#else
typedef bool _Bool;
typedef unsigned char _Bool;
# define bool _Bool
# define false 0
# define true 1
# define __bool_true_false_are_defined 1
#endif /* HAVE_STDBOOL_H */

/*
 * MAX and MIN taken from coretuils, a GNU project
 *
 * from coreutils-5.94, src/system.h
 */

#ifndef MAX
# define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif

#ifndef MIN
# define MIN(a,b) (((a) < (b)) ? (a) : (b))
#endif


/* function prototypes */

char * ln_ether_ntoa(const u_int8_t *a, char * macf,  char * buf, size_t s );
static void prog_abort(char * t );
static void short_usage(int ret);
static void long_usage(int ret);
static void usage(int ret);
struct arp_record * fetch_arp_record();

struct a_options {
  char                      *ifname;
  int                         count;  /* count pointer count (yuk-yuk) */
  char                   *ptr_count;
  int                          infl;
  char                    *ptr_infl;
  int                          wait;
  char                    *ptr_wait;
  char                        *macf;
  char                    *ptr_macf;
  FILE                        *outp;
  char                    *ptr_outp;
  struct timeval             waittv;
  char                      *format;
  int                       verbose;
  int                         flags;
};

struct arppkt {
  u_int16_t                     hrd;    /* hardware type */
  u_int16_t                     pro;    /* protocol */
  u_int8_t                      hln;    /* header length */
  u_int8_t                      pln;    /* protocol length */
  u_int16_t                      op;    /* ARP operation */
  u_int8_t  addrs[2*HRD_MAXLEN+2*IPADDR_LEN];
/*u_int8_t[ETH_ALEN]            sha;
  u_int8_t[4]                   sip;
  u_int8_t[ETH_ALEN]            tha;
  u_int8_t[4]                   tip;*/
};

/*
struct arp_record_head {
  struct arp_record           *next;
  struct arp_record           *prev;
  int                        length;
};
*/

struct arp_record {
  struct arp_record           *next;
  struct arp_record           *prev;
  unsigned long        member_count;    
  int                         flags;
  long                      numsent;
  long                      numrecv;
  int                     last_time;
  int                         delay;
  struct timeval          sent_time;
  struct timeval        expire_time;
  struct in_addr                 ip;  /* ip.s_addr is network byte order */
  u_int32_t                  ipaddr;
  u_int8_t   lladdr[ETHER_ADDR_LEN];
  char        llstr[LL_ADDR_STRLEN];
};

#endif                                                         /* _ARPSWEEP_H */
