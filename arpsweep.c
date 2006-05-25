/*
 * arpsweep.c
 * 
 * Conceived 2006-04-15, Martin A. Brown <martin@linux-ip.net>
 * 
 * Sends ARP queries to multiple hosts; reports responses received or
 * the absence of response.
 * 
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

#ifndef lint
static const char rcsid[] =
    "$Id:$";
#endif

#include "config.h"

#define _GNU_SOURCE
#include "arpsweep.h"
#include "log.c"
#include "list.c"
#include "util.c"

const float          arpsweep_version           = 0.46f;
const char           progname[]                 = PROGRAM_NAME;
const char           arpsweep_time[]            = __DATE__" "__TIME__;
const char           arpsweep_copyright[] 
                      = "arpsweep by Martin A. Brown, see http://linux-ip.net/";


static char          macformat_unix[]       = "%.2x:%.2x:%.2x:%.2x:%.2x:%.2x";
static char          macformat_3com[]       = "%.2x-%.2x-%.2x-%.2x-%.2x-%.2x";
static char          macformat_cisco[]      = "%.2x%.2x.%.2x%.2x.%.2x%.2x";

struct arp_record    our_addresses[1];
struct arp_record   *my                         = our_addresses;

static u_int8_t      ethnull[ETHER_ADDR_LEN];
static u_int8_t      ethbcast[ETHER_ADDR_LEN];

/*
 * All of the data recorded by this utility is stored in
 * arp_record structures which are stored as a set of linked
 * lists.
 *
 *    targets:  populated at the beginning of the program
 *              and contains the set of user-supplied IPs
 *    inflight: utility moves the arp_record to this linked
 *              list when it transmits a request
 *    complete: sent and received (or saw timeout) for all
 *              of the requests
 *
 */
struct arp_record   *targets                    = NULL;
struct arp_record   *current                    = NULL;
struct arp_record   *inflight                   = NULL;
struct arp_record   *complete                   = NULL;

struct a_options    o                           = {
                    .ifname                     = DEFAULT_INTERFACE_NAME,
                    .count                      = DEFAULT_ARPS_TO_SEND,
                    .ptr_count                  = NULL,
                    .infl                       = DEFAULT_INFLIGHT,
                    .ptr_infl                   = NULL,
                    .macf                       = macformat_unix,
                    .ptr_macf                   = NULL,
                    .format                     = DEFAULT_OUTPUT_FORMAT,
                    .wait                       = DEFAULT_PATIENCE,
                    .waittv.sec                 = 0,
                    .waittv.usec                = 0,
                    .verbose                    = LOG_LEVEL_ERR,
                    .flags                      = 0,
                    .outp                       = NULL
};


/*
 * Put a little signal handler in here so that we gracefully
 * exit when the user smacks ctrl-C, or we received an ALRM.
 * In case, we are running "forever", the user can terminate us
 * and still get a report aftre a SIGALRM or SIGINT.
 *
 */

bool bored = false ;
static void bored_user(int i)
{
  //struct arp_record   *cur                = NULL;
  INFO( "Received signal %u.\n", i );

  list_move( &targets, &complete );
  list_move( &inflight, &complete );

  bored = true;
}

static void
report_select_error(int err)
{
  switch( err )
  {
    case  EINTR: WARN("select returned errno=%d: %s ....continuing.\n", 
                                       err, strerror( err ) ); break;
    case  EBADF: FATAL("Internal %s error.\nerrno(%d): %s\n", 
                             progname, err, strerror( err ) ); break;
    case EINVAL: FATAL("Internal %s error:\nerrno(%d): %s\n", 
                             progname, err, strerror( err ) ); break;
    case ENOMEM: FATAL("Uh-oh %s ran out of memory.\nerrno(%d): %s\n", 
                             progname, err, strerror( err ) ); break;
  }
}


/* Taken from shuffle by M.J. Pomraning */

static bool 
str2int(const char *str, int *out) {
  char *endptr = NULL;
  long l;
    
  if (!str || str[0] == '\0' || !out)
    goto einval;  
    
  errno = 0;
  l = strtol(str, &endptr, 10);
  if (*endptr != '\0') goto einval;
  if ((errno == ERANGE && (l == LONG_MAX || l == LONG_MIN))
      ||
      (l > INT_MAX || l < INT_MIN)) {
    errno = ERANGE; /* may be redundant */
    return false;
  } 

  *out = (int) l;
      
  return true;
    
einval:
  errno = EINVAL;
  return false;
}   

/*
 * get_mac_addr()  adapted (or, more accurately, adopted)
 * from arping by Thomas Habets
 */

static bool get_mac_addr(const char *in, u_int8_t *n)
{
         if ( 6 == sscanf(in, "%x:%x:%x:%x:%x:%x", ( unsigned int ) n++,
                                                   ( unsigned int ) n++,
                                                   ( unsigned int ) n++,
                                                   ( unsigned int ) n++,
                                                   ( unsigned int ) n++,
                                                   ( unsigned int ) n++ ) )
  { return true;
//} else if ( 6 == sscanf(in, "%2x%x.%2x%x.%2x%x", n0, n1, n2, n3, n4, n5 ) )
//{ return true;
//} else if ( 6 == sscanf(in, "%x-%x-%x-%x-%x-%x", n0, n1, n2, n3, n4, n5 ) )
//{ return true;
  }
  return false;
}

struct arp_record *
find_arp_record(struct arp_record * cur, struct in_addr ip )
{
  /*
   * 
   * This is a lousy, iterative searching algorithm.  I start at the head
   * of the list, and walk the list.  Dumb.  Should be replaced by a hash
   * or something much smarter.
   * 
   */
  struct arp_record   *next;
  int                  i, x; 

  if ( ! cur ) return NULL ;

  for ( i = 0, x = list_length( cur ) ; i < x ; ++i, cur = next )
  {

    next = cur->next;

    /*
    DEBUG( "Comparing %s against (%p) %s in list, i=%d.\n",
           inet_ntoa( ip ),
           cur,
           inet_ntoa( cur->ip ),
           i );
     */

    if ( 0 == memcmp( &ip, &cur->ip, sizeof( struct in_addr ) ) ) return cur;

  }

  return NULL;
}

struct arp_record *
allocate_arp_record( )
{
  struct arp_record * new;

  new = malloc( sizeof( struct arp_record ) );
  if ( new == NULL ) FATAL( "Could not allocate memory.\n" );
  return( new );
}

void
remove_from_inflight( struct arp_record * cur )
{
  list_remove( &inflight, cur );

  if ( o.count && cur->numsent >= o.count )
  {
     list_append( &complete, cur );
  } else {
     list_append( &targets,  cur );
  }
}

// static inline char *str_ip(u32 ip)
// {
//         struct in_addr n;
//         n.s_addr = htonl(ip);
//         return inet_ntoa(n);
// }

void
expire_inflight( struct arp_record * cur )
{
  struct arp_record   *next;
  struct timeval       now;
  int                  i, x;
  int                  d;

  DEBUG( "targets length: %d, inflight length: %d complete length: %d.\n",
        list_length( targets  ),
        list_length( inflight ),
        list_length( complete ) );

  /*
   * We are cheating, we are going to check time once at the
   * beginning of this function, and just use that as the "now"
   * time.
   */
  now = ast_tvnow();

  for ( i = 0, x = list_length( cur ) ; i < x ; ++i, cur = next )
  {

    next = cur->next; /* Remember the next item, in case we change the list */

    d = ast_tvcmp( now, cur->expire_time );

    switch ( d )
    {
      case  0: /* fall through, weird, exact time? */
      case  1: WARN( "No response from %s for ARP number %d at %d.%d, expiring.\n",
                     inet_ntoa( cur->ip ),
                     cur->numsent,
                     now.tv_sec,
                     now.tv_usec
                    );

               remove_from_inflight( cur );

               break;

      case -1: DEBUG("Not expiring inflight %s at %d.%d (sent_time %d.%d).\n",
                     inet_ntoa( cur->ip ),
                     ( &cur->sent_time )->tv_sec,
                     ( &cur->sent_time )->tv_usec,
                     now.tv_sec,
                     now.tv_usec
                    );
               break;

    }
  }
}

/*
 * spit out a user-edible string of the ethernet address
 *
 * ARGS:  1:  character string containing the dotted quad IP
 *        2:  possibly NULL character string containing optional MAC
 * RTRN:  arp_record * containing the newly created record for this
 *        user specified target
 * S-FX:  allocates memory
 *
 */

struct arp_record *
parse_target( char * ip, char * mac )
{
  struct timeval           t;
  struct arp_record       *new;
  struct in_addr           tg_ip;
  bool                     ok = true;
  char                     buf0[128];

  INFO("Mac in %s: %s\n", __func__, mac );

  if ( ! inet_aton( ip, &tg_ip ) )
  {
    ok = false;
    ERR( "Unrecognized IP address %s, skipping.\n", ip );
  }
  
  /* 
    INFO("IP address %s supplied, checking for duplicates.\n",
          inet_ntoa( tg_ip ) );

    new = find_arp_record( targets, tg_ip );

    if ( new )
    {
      ERR("IP address %s specified multiple times, ignoring.\n",
           inet_ntoa( tg_ip ) );
      return NULL;
    }
  */
  
  new = allocate_arp_record();
  new->ipaddr = tg_ip.s_addr;
  new->ip.s_addr = tg_ip.s_addr;
  new->last_time = 0;
  new->delay = 0;
  //( &new->sent_time )->tv_sec  = 0;
  //( &new->sent_time )->tv_usec = 0;

  /* Now, let us see if the user supplied a MAC address */

  if ( ! mac ) /* switch between a provided MAC and broadcast */
  {
    DEBUG( "No MAC specified, setting broadcast address.\n" );

    memset( &new->lladdr, 0xff, ETHER_ADDR_LEN );
  } else {
    u_int8_t n[6];
    if ( ! get_mac_addr( mac, &n ) )
    {
      ok = false;
      ERR( "Unrecognized MAC address %s, skipping IP %s.\n", mac, ip );
    }
    memcpy( &new->lladdr, n, ETHER_ADDR_LEN );
  }

  if ( ! ok ) /* If NOT OK, MAC address is bogus; free mem */
  {
    INFO("%s: removing %s (%s).\n", __func__, ip, mac );
    free( new );
    return NULL;
  }

  INFO("Using MAC %s for %s.\n",
        ln_ether_ntoa( new->lladdr, o.macf, buf0, sizeof( buf0 ) ), ip );

  return new;
}

static void
file_input_target( FILE * f )
{
  struct arp_record                *new = NULL;
  char                              buf[1025];
  char                              s0[128];
  char                              s1[128];
  char                             *uip;
  char                             *umac;
  int                               line = 0;
  int                               i = 0;

  /* fgets can offer us a line at a time. */

  while ( NULL != fgets( buf, 1024, f ) )
  {
    ++line;
    
    uip = umac = NULL ;   /* reset variables */

    i = sscanf( buf, "%s %s", s0, s1 );

    DEBUG( "%s reading line %d: s0=%s, s0=%s.\n", __func__, line, s0, s1 );

    switch (i)
    {
      case 0: ERR("Blank line of input at line %d.\n", line ) ; break ;
      case 1: uip = s0 ; umac = NULL                          ; break ;
      case 2: uip = s0 ; umac = s1                            ; break ;
      default:
        USAGE_FATAL("Fatal in function %s on input line %d.\n",
                            __func__, line );
    }


    new = parse_target( uip, umac );
    
    if ( new ) list_append( &targets, new );
    
  }

}

static void
command_line_target( char * udata )
{
  char                             *uip;
  char                             *umac;
  struct arp_record                *new = NULL;
  char                              buf0[128];
  char                              buf1[128];
  int                               i;

  uip = udata;

  if ( ( umac = strchr( udata, ',' ) ) )
  {
    /*
     * There's probably a better way than iterating through the
     * string manually looking for the comma that we now know is
     * there.  Another improvement!
     *
     */

    ++umac; /* one byte "east" of comma is MAC */

    for ( i = 0 ; udata[i] ; ++i )
    {
      //DEBUG( "Array index %d, %c\n", i, udata[i] );
      if ( udata[i] == ',' ) udata[i] = '\0' ;
    }
  } else {
    umac = NULL ;
  }

  new = parse_target( uip, umac );
  
  if ( new ) list_append( &targets, new );

}

static void
process_input_file( FILE * f )
{
  DEBUG( "Entering function %s.\n", __func__ );
  file_input_target( f );

}

static void
caught_arp_reply(const char            *unused,
                 struct pcap_pkthdr    *h,
                 u_int8_t              *f )
{
  const struct libnet_ethernet_hdr *ef;  /* Ethernet Frame */
  const struct libnet_arp_hdr      *ap;  /* ARP frame */

  /*
   * Why so many variables?
   *
   *  fr_mac - the Ethernet frame source address
   *  src_ll - the link layer address reported inside the ARP reply
   *  src_ip - the IP address reported in the ARP reply
   *
   */
  u_int8_t                          fr_mac[ETHER_ADDR_LEN];
  u_int8_t                          src_ll[ETHER_ADDR_LEN];
  u_int32_t                         src_ip_u32;
  struct in_addr                    src_ip;

  struct timeval                    pkttime;
  struct arp_record                *cur = NULL;
  char                              buf0[128];
  char                              buf1[128];
  int                               i = 0;

  /*
   * This is really lame, but constantly get NULL pointer (or
   * negative times!!) when using h->ts.  I noticed that Thomas
   * Habets, author of arping, also uses gettimeofday() in
   * this fashion.
   *
   */
  pkttime = ast_tvnow();

  /* Ethernet header used to fetch the frame MAC address */
  ef = (const struct libnet_ethernet_hdr *)( f );

  /* ARP header isn't terribly interesting, but we'll use arp_hln */
  ap = (const struct libnet_arp_hdr *)( f + LIBNET_ETH_H );

  /* check to make sure that the ARP header makes sense */
  //if ( ap->ar_op != ARPOP_REPLY )
  //{
  //  DEBUG( "Received ARP message (%d) which was not a reply.\n", ap->ar_op ); 
  //  return;
  //}
  if ( ap->ar_hln != ETHER_ADDR_LEN )
  {
    WARN( "Received ARP reply with %d for hardware address length, Ignoring!\n",
               ap->ar_hln ); 
    return;
  }
  if ( ap->ar_pln != IP_ADDR_LEN )
  {
    WARN( "Received ARP reply with %d for protocol address length, Ignoring!\n",
               ap->ar_pln ); 
    return;
  }

  /*
   * Strange that the structs above or the frame pointer do not allow us
   * to simply memcpy, but that's what's happening.  So, instead, we
   * will just copy byte by byte into an array.
   *
   */

  for ( i = 0; i < ETHER_ADDR_LEN; i++ )
  {
    fr_mac[i] = ( ( u_int8_t *) f )[ ETHER_ADDR_LEN + i ];
  }

  memcpy(&src_ll,     (char *)ap + LIBNET_ARP_H,  ETHER_ADDR_LEN );
  memcpy(&src_ip_u32, (char *)ap + LIBNET_ARP_H + ETHER_ADDR_LEN, IP_ADDR_LEN);

  src_ip.s_addr = src_ip_u32;

  cur = find_arp_record( inflight, src_ip );

  if ( ! cur )
  {
    cur = find_arp_record( targets, src_ip );
    if ( ! cur )
    {
      WARN("Ignoring unexpected ARP reply from %s (%s).\n",
                 inet_ntoa( src_ip ),
                 ln_ether_ntoa( fr_mac, o.macf, buf0, sizeof( buf0 ) ) );
      return;
    }
    WARN("Received late/duplicate ARP reply from %s (%s).\n",
                 inet_ntoa( src_ip ),
                 ln_ether_ntoa( fr_mac, o.macf, buf0, sizeof( buf0 ) ) );

    cur->flags |= AR_MULTI_REPLY;

    /* don't fall through to the rest of the function! */
    return ;

  }

  remove_from_inflight( cur );

  /*
   * If cr->lladdr currently contains the link layer broadcast address
   * then, this is the first time we have seen a reply; let's remember
   * the MAC address!
   *
   */
     
  if (    -1 == ( memcmp( &fr_mac,   cur->lladdr, ETHER_ADDR_LEN ) )
       &&  0 == ( memcmp( &ethbcast, cur->lladdr, ETHER_ADDR_LEN ) ) )
  {
    WARN( "Storing MAC %s for IP %s.\n",
               ln_ether_ntoa( fr_mac, o.macf, buf0, sizeof( buf0 ) ),
               inet_ntoa( src_ip ) );
    memcpy( cur->lladdr, &fr_mac, ETHER_ADDR_LEN );
  }

  /* Weird!  MAC in ARP reply does not match frame header! */

  if ( -1 == ( memcmp( &fr_mac, &src_ll, ETHER_ADDR_LEN ) ) )
  {
    WARN( "ARP reply with mismatched ARP MAC %s and frame MAC %s.\n",
               ln_ether_ntoa( src_ll, o.macf, buf0, sizeof( buf0 ) ),
               ln_ether_ntoa( fr_mac, o.macf, buf1, sizeof( buf1 ) ) );
    cur->flags |= AR_FRAME_MAC_MISMATCH;
  }

  /* Link layer reply originates from unexpected neighbor. */

  if ( -1 == ( memcmp( &fr_mac, cur->lladdr, ETHER_ADDR_LEN ) ) )
  {
    ERR( "Reply for %s from %s (expected %s).\n",
               inet_ntoa( src_ip ),
               ln_ether_ntoa( src_ll, o.macf, buf0, sizeof( buf0 ) ),
               ln_ether_ntoa( cur->lladdr, o.macf, buf1, sizeof( buf1 ) ) );
    cur->flags |= AR_MULTI_REPLY;
  }

  DEBUG( "IP %s responded in %d us; sent_time: %d.%d, pkttime: %d.%d.\n",
          inet_ntoa( cur->ip ),
          ast_tvdiff_us( pkttime, cur->sent_time ),
          pkttime.tv_sec,
          pkttime.tv_usec,
          ( &cur->sent_time )->tv_sec,
          ( &cur->sent_time )->tv_usec
          );

  cur->last_time   = ast_tvdiff_us( pkttime, cur->sent_time );
  cur->delay      += cur->last_time;
  cur->numrecv    += 1      ; /* I could use cur->numrecv++ */

  DEBUG( "About to return from %s.\n", __func__);
}

/*
 * Create an ARP packet (using UNIX primitives) and transmit.
 */
int
transmit_arp_req_prim ( struct arp_record * cur )
{
  static struct arppkt         arp;
  u_int8_t                      *p; /* Just like Bobrowski */

  DEBUG( "About to build ARP request.\n" );

  arp.hrd = ARPHRD_ETHER;
  arp.pro = ETHERTYPE_IP;    /* should use value gathered from ifinfo */
  arp.hln = ETHER_ADDR_LEN;  /* should use value gathered from ifinfo */
  arp.pln = IPADDR_LEN;
  arp.op  = ARPOP_REQUEST;

  p       = arp.addrs; /* get pointer to the network address storage */

  /* drop in our source link layer and source IP address */

  memcpy( p, my->lladdr, ETHER_ADDR_LEN );
  p      += ETHER_ADDR_LEN;
  *( u_int32_t * )p = htonl( my->ipaddr );
  p      += IPADDR_LEN;
  
  /* drop in the destination link layer and destination IP address */
  

}

/*
 * Create an ARP packet (using libnet) and transmit.
 */
int
transmit_arp_req_lnet ( libnet_t * lnet, struct arp_record * cur )
{
  /* By declaring these ptags as static, we can reuse them on
     subsequent calls.  This should be much faster than building
     a new object every time. */
  static libnet_ptag_t         eth = 0;
  static libnet_ptag_t         arp = 0;

  u_int8_t                    *tg_ll;
  int                          i = 0;

  if (-1 == (arp = libnet_build_arp(ARPHRD_ETHER,
                                    ETHERTYPE_IP, 
                                    ETHER_ADDR_LEN,
                                    IPADDR_LEN,
                                    ARPOP_REQUEST,
                                    my->lladdr,
                                    (u_int8_t *) &my->ipaddr,
                                    ethnull,
                                    (u_int8_t *) &cur->ipaddr,
                                    NULL,
                                    0,
                                    lnet,
                                    arp)))
  {
    FATAL( "libnet_build_arp(): %s\n", libnet_geterror( lnet ) );
  }

  DEBUG( "About to build Ethernet frame.\n" );

  tg_ll = ( o.flags & AO_NO_UNICAST ) ? ethbcast : cur->lladdr ;

  if (-1 == (eth = libnet_build_ethernet(
                     (u_int8_t *) tg_ll,
                     (u_int8_t *) my->lladdr,
                     ETHERTYPE_ARP,
                     NULL,
                     0,
                     lnet,
                     eth ) ) )
  {
    FATAL( "libnet_build_ethernet(): %s\n", libnet_geterror( lnet ) );
  }

  DEBUG( "About to transmit ARP request to %s.\n",
         inet_ntoa( cur->ip ) );

  i = libnet_write( lnet );
  switch ( i )
  {
    case -1:
      INFO( "error transmitting frame to %s\nlibnet_write(): %s\n",
               inet_ntoa( cur->ip ),
               libnet_geterror( lnet ) );
      return 1;
    default:
      INFO( "libnet_write() reports transmitting %d bytes.\n", i );
  }

  list_remove( &targets,  cur );
  list_append( &inflight, cur );

  INFO( "Transmitted ARP request to %s.\n",
         inet_ntoa( cur->ip ) );

  cur->sent_time = ast_tvnow();
  cur->expire_time = ast_tvadd( cur->sent_time, o.waittv );
  DEBUG( "IP: %s, sent_time: %d.%d, expire_time: %d.%d.\n",
         inet_ntoa( cur->ip ),
         ( &cur->sent_time )->tv_sec,
         ( &cur->sent_time )->tv_usec,
         ( &cur->expire_time )->tv_sec,
         ( &cur->expire_time )->tv_usec
       );

  cur->numsent++;

  DEBUG( "function %s: cur (%p), cur->sent_time (%p).\n",
         __func__, cur, &cur->sent_time );

}

/*
 * Create an ARP packet (using libnet) and transmit.
 */
int
transmit_arp_req ( libnet_t * lnet, struct arp_record * cur )
{
  /* Return if we have already sent enough ARPs. */

  if ( o.count && cur->numsent >= o.count ) return 0 ;

  DEBUG( "About to build ARP request.\n" );

  transmit_arp_req_lnet( lnet, cur );
  // transmit_arp_req_prim( cur );
}


static void
fetch_frames_from_pcap( pcap_t * pcap )
{

  pcap_dispatch( pcap, -1, (pcap_handler)caught_arp_reply, NULL );

}

static void
select_on_pcap( pcap_t * pcap, struct timeval * timeout )
{
  fd_set               pfdset;
  int                  i;
  /* 
   *
   * According to the select manpage, any errors while calling
   * select may cause the fdsets and timevals to unknown states
   *
   * So, we'll happily rebuild a known state before firing off
   * our select call.
   *
   */
  while ( true )
  {
    FD_ZERO( &pfdset );
    FD_SET( pcap_fileno( pcap ), &pfdset );

    DEBUG( "About to select on pcap_fileno() in %s.\n", __func__ );

    i = select( pcap_fileno( pcap )+1, &pfdset, NULL, NULL, timeout );

    switch( i )
    {
      case -1:  report_select_error( errno )    ; break ;
      case  0:  return                          ; break ;
      default:  fetch_frames_from_pcap( pcap )  ; break ;
    }
  }
}


/*
 * spit out a user-edible string of the ethernet address
 *
 * ARGS:  yes
 * RTRN:  char * containing string version of ethernet address
 *        suitable for printing for human consumption
 * S-FX:  none
 *
 * Note:  This is pretty much a carbon copy of glibc's ether_ntoa,
 *        just using the libnet struct libnet_ether_addr.
 */
char *
ln_ether_ntoa(const u_int8_t *a, char * macf, char * buf, size_t s )
{
  
  snprintf ( buf, s, macf,
              a[0],a[1],a[2],a[3],a[4],a[5]);
  return buf;
}

/*
 * 
 *
 * ARGS:  a pointer to a libnet_ether_addr struct
 *        the libnet context
 * RTRN:  nothing
 * S-FX:  sets ethernet address in libnet_ether_addr struct
 *        might abort program
 *
 */

u_int8_t *
ln_etheraddr_fetch(u_int8_t * t, libnet_t *libnet )
{

  struct libnet_ether_addr    *ea;

  if ( ! ( ea = libnet_get_hwaddr( libnet ) ) )
  {
     FATAL( 
            "libnet_get_hwaddr(): %s\nCould not determine link layer address.\n",
              libnet_geterror(libnet) );
  }
  memcpy( t, ea, ETHER_ADDR_LEN );
}

static void
get_local_addresses( libnet_t * l )
{

  /* Yeah.  Ugly.  These are globals.  Oh well. */

  my->ipaddr = libnet_get_ipaddr4( l );

  ln_etheraddr_fetch( my->lladdr, l );
  ln_ether_ntoa( my->lladdr, macformat_unix, my->llstr, sizeof( my->llstr ) );

}

/*
 * Big wrapper function around pcap initialization.
 *
 * ARGS:  none
 * RTRN:  nothing
 * S-FX:  global a_option struct members may be modified
 *        program aborts if failure
 */
static void
validate_user_options()
{
  /*
   *
   * Run through a series of validations on user requested
   * command-line options.
   *
   */

  if ( o.flags & AO_USAGE ) long_usage( EXIT_SUCCESS );
  if ( o.flags & AO_VERSION ) print_version( EXIT_SUCCESS );

  /* No user specification between Missing, Weird and/or Alive?
     Then, just show responses from hosts that are alive. */

  if ( ! ( o.flags & ( AO_MISSING | AO_WEIRD | AO_ALIVE ) ) )
           o.flags |= AO_ALIVE;


  if ( o.ptr_count && ! str2int( o.ptr_count, &o.count ) )
    USAGE_FATAL(
      "Invalid integer \"%s\" specified for count option (-c|--count)\n",
       o.ptr_count );


  if ( ( o.ptr_infl  && ! str2int( o.ptr_infl, &o.infl ) ) || o.infl <= 0 )
    USAGE_FATAL(
      "Invalid specification for request count \"%s\" (-p|--pending).\n",
       o.ptr_infl );

  if ( ( o.ptr_wait  && ! str2int( o.ptr_wait, &o.wait ) ) || o.wait <= 0 )
    USAGE_FATAL(
      "Invalid wait interval \"%s\" specified (-w|--wait).\n",
       o.ptr_wait );

  /* easier to do the math if calculating only in microseconds */
  o.wait            *= ONE_THOUSAND;
  o.waittv.tv_sec    = ( o.wait / ONE_MILLION );
  o.waittv.tv_usec   = ( o.wait % ONE_MILLION );
  o.wait            /= ONE_THOUSAND;

  if ( o.ptr_macf )
  {
    bool ok = false; /* assume user has supplied a bad mac-format arg */

    switch ( o.ptr_macf[0] )  /* lots of fall through below */
    {
      case 'c':
      case 'C': o.macf = macformat_cisco       ; break ;
      case '3': 
      case 'w':
      case 'W': o.macf = macformat_3com        ; break ;
      case 'm': 
      case 'M': 
      case 'b': 
      case 'B': 
      case 'l': 
      case 'L': 
      case 'u': 
      case 'U': ok = true;
      default : 
                if ( ! ok )
                  WARN( "Invalid mac-format name (\"%s\"), using default.\n",
                        o.ptr_macf );
                o.macf = macformat_unix        ; break ;
    }
  }

}

/*
 * Big wrapper function around pcap initialization.
 *
 * ARGS:  none
 * RTRN:  nothing
 * S-FX:  global variable p (pcap_t) initialized
 *        program aborts if failure
 */
pcap_t *
initialize_pcap(pcap_t * pcap, char * ifname, struct arp_record * cur)
{

  char                         pc_errbuf[PCAP_ERRBUF_SIZE];
  char                         bp_text[50];
  struct bpf_program           bp;

  /* 
   * We only need to snag 60 bytes to get an entire ARP response
   * so, we'll use ARP_FRAME_SIZE
   *
   */

  DEBUG( "About to pcap_open_live() in %s.\n", __func__ );

  if ( !( pcap = pcap_open_live( ifname, ARP_FRAME_SIZE, 0, 10, pc_errbuf ) ) )
  {
    FATAL( "pcap_open_live(): %s\n", pc_errbuf );
  }

  /* Make sure we are only running on Ethernet */

  if ( pcap_datalink( pcap ) != DLT_EN10MB )
  {
    FATAL( "Sniffing arp on something other than Ethernet, eh?  Nice try.\n" );
  }

  /*
  snprintf( bp_text,
            sizeof( bp_text ),
            "%s",
            BPF_ONLY_ARP_REPLIES );

   */
  
  snprintf( bp_text,
            sizeof( bp_text ),
            "%s %s",
            BPF_ONLY_ARP_REPLIES,
            cur->llstr );
 

  DEBUG( "BPF: \"%s\" (in %s).\n", bp_text, __func__ );

  if (-1 == pcap_compile(pcap, &bp, bp_text, 0, -1 )) {
    FATAL( "pcap_compile(): %s\n", pcap_geterr( pcap ) );
  }

  if (-1 == pcap_setfilter( pcap, &bp)) {
    FATAL( "pcap_setfilter(): %s\n", pcap_geterr( pcap ) );
  }

  /* Don't fiddle with the PCAP after setting PCAP_NONBLOCK */

  /*
    if ( -1 == pcap_setnonblock( pcap, PCAP_NONBLOCK, pc_errbuf ) ) {
      FATAL( "pcap_setnonblock(): %s\n", pcap_geterr( pcap ) );
    }
   */
  INFO( "PCAP initialization success (%s); pcap_fileno = %d.\n",
         __func__, pcap_fileno( pcap ) );

  return( pcap );
}

/*
 * Big wrapper function around libnet initialization.
 *
 * ARGS:  none
 * RTRN:  nothing
 * S-FX:  global variable libnet initialized
 *        program aborts if failure
 */
libnet_t *
initialize_libnet( libnet_t * lnet, char * ifname )
{
  char                         ln_errbuf[LIBNET_ERRBUF_SIZE];

  if ( lnet )
  {
    libnet_destroy( lnet );  /* Start over with a new libnet context. */
    lnet = NULL ;
  }
  INFO( "About to create libnet object.\n" );

  if ( ! ( lnet = libnet_init( LIBNET_LINK, ifname, ln_errbuf ) ) )
    FATAL( "libnet_init(): %s\n", ln_errbuf);

  INFO( "Created libnet object.\n" );

  get_local_addresses( lnet );

  return( lnet );
}

static void
print_info_line(FILE * f, struct arp_record * cur )
{
    MSG( f,
        "%-15s %17s %6u %6u %12d %8d\n",
           inet_ntoa( cur->ip ),
           ln_ether_ntoa( cur->lladdr, o.macf, cur->llstr, sizeof( cur->llstr ) ), 
           cur->numrecv,
           cur->numsent,
           cur->delay,
           cur->delay / cur->numsent
         );
}

static void
report( struct arp_record * cur )
{
  struct arp_record   *next;
  int                  i, items;

  if ( ! cur ) return ; /* Good behaviour, but should never get here. */

  /* print out the header */

  if ( ! o.flags & AO_NO_HEADER )
  {
    MSG( o.outp,
           "%-15s %17s %6s %6s %12s %8s\n",
           "IP Address",
           "Link Layer",
           "Recv",
           "Sent",
           "Delay(us)",
           "Average"
           );
  }

  items = list_length ( cur );

  for ( i = 0 ; i < items ; ++i, cur = next )
  {
    next = cur->next;

    if ( o.flags & AO_ALIVE && cur->numrecv > 0 )
    {
      print_info_line( o.outp, cur );
    }

    if ( o.flags & AO_MISSING && cur->numrecv == 0  )
    {
      print_info_line( o.outp, cur );
    }

    if ( o.flags & AO_WEIRD && cur->flags )
    {
      print_info_line( o.outp, cur );
    }

  }
}

/*
 * main()
 *
 */
int
main (int argc, char *argv[] )
{

  /* We shall start with command line options! */

  enum
  {
     PREFER_FRAME_ADDR = CHAR_MAX + 1
  };

  static struct option long_opts[] = {
    {"help",            no_argument,       0, 'h'        },
    {"usage",           no_argument,       0, 'h'        },
    {"verbose",         optional_argument, 0, 'v'        },
    {"quiet",           no_argument,       0, 'q'        },
    {"version",         no_argument,       0, 'V'        },
    {"interface",       required_argument, 0, 'i'        },
    {"count",           required_argument, 0, 'c'        },
    {"pending",         required_argument, 0, 'p'        },
    {"mac-format",      required_argument, 0, 'm'        },
    {"format",          required_argument, 0, 'f'        },
    {"no-header",       no_argument,       0, 'N'        },
    {"alive",           no_argument,       0, 'A'        },
    {"missing",         no_argument,       0, 'M'        },
    {"weird",           no_argument,       0, 'W'        },
    {"wait",            required_argument, 0, 'w'        },
    {"broadcast-only",  no_argument,       0, 'B'        },
    {"no-unicast",      no_argument,       0, 'B'        },
    {0, 0, 0, 0},
  };

  int                  c, i;

  struct timeval       timeout;
  static libnet_t     *l                  = NULL;
  static pcap_t       *p                  = NULL;

  memset( ethnull, 0x00, ETHER_ADDR_LEN);
  memset( ethbcast, 0xff, ETHER_ADDR_LEN);

  /* just parse a pile of options, first, then we'll perform
     some validation */
  opterr = 0; /* Keep getopt quiet. */

  while ( -1 != (c = getopt_long(argc, argv, "qvhVBNAMWm:w:i:c:p:f:", long_opts, NULL) ) )
  {
    static bool opt_done = false;
    switch(c)
    {
      case '?'        : USAGE_FATAL( "Unrecognized option:  \"%s\"\n"
                               "Try \"%s --help\" for more information.\n",
                               argv[optind - 1], progname ); break ;
      case 'q'        : o.verbose      = 0;                  break ;
      case 'v'        : o.verbose      = loglevel( optarg ); break ;
      case 'i'        : o.ifname       = optarg;             break ;
      case 'c'        : o.ptr_count    = optarg;             break ;
      case 'p'        : o.ptr_infl     = optarg;             break ;
      case 'w'        : o.ptr_wait     = optarg;             break ;
      case 'm'        : o.ptr_macf     = optarg;             break ;
      case 'f'        : o.format       = optarg;             break ;
      case 'N'        : o.flags       |= AO_NO_HEADER;       break ;
      case 'A'        : o.flags       |= AO_ALIVE;           break ;
      case 'M'        : o.flags       |= AO_MISSING;         break ;
      case 'W'        : o.flags       |= AO_WEIRD;           break ;
      case 'h'        : o.flags       |= AO_USAGE;           break ;
      case 'V'        : o.flags       |= AO_VERSION;         break ;
      case 'B'        : o.flags       |= AO_NO_UNICAST;      break ;

      default         : opt_done = true ; --optind;          break ;

    }
    if (opt_done) break;
  }

  o.outp = stdout; /* until the --output command line switch is added */

  validate_user_options();

  arpsweep_info_header();

  INFO( "Parameters, count: %d, pending: %d, wait: %d, o.waittv(u): %d, flags: %d\n",
         o.count, o.infl, o.wait, (&o.waittv)->tv_usec, o.flags );

  /*
   * Now that we've delayed initializing the libraries as long as possible,
   * let's create the libnet context for use throughout the program.  We
   * need to use libnet's validation routines to convert IP addresses to
   * u_int32_t's.
   *
   */

  l = initialize_libnet( l, o.ifname );

  /* We have good command line options!  We are ready to parse IP/MAC pairs */

  if ( optind <= argc )
  {
    for (i = optind; i < argc; i++)
    {
      command_line_target( argv[i] );
      
    }
  }
  if ( ! isatty( STDIN_FILENO ) )   process_input_file( stdin );

  if ( targets == NULL ) USAGE_FATAL( "Missing IP address argument.\n" );

  p = initialize_pcap( p, o.ifname, my );

  signal(SIGINT,  bored_user);
  signal(SIGALRM, bored_user);

  INFO( "Signal handlers set; entering infinite loop.\n" );

  /*
   * Initialize current the head of the arp_record linked list
   * before starting the main loop.
   *
   */

  while ( /* user is */ ! bored && ( inflight || targets ) )
  {

    /*
     * If there are targets, let's keep spinning.
     *
     */

    while ( targets )
    {
      current = targets;

      INFO( "About to resume main transmit loop (current=%p).\n", current );
      DEBUG( "targets: %d items, inflight: %d items, complete: %d items, o.infl = %d.\n",
            list_length( targets  ),
            list_length( inflight ),
            list_length( complete ),
            o.infl );

      if ( list_length( inflight ) >= o.infl ) break ;

      INFO( "Going to send another ARP (current=%p).\n", current );
      transmit_arp_req( l, current );

    }
    /*
     * If anything is still inflight, then let's listen on pcap
     * and/or expire the timers on anything which has not yet
     * responded.
     *
     */
    DEBUG("About to jump to pcap file descriptor select subroutine.\n");

    /* need better timeout calculation */

    timeout.tv_sec = 0; timeout.tv_usec = SLEEP_TIME;
    select_on_pcap( p, &timeout );

    DEBUG("Checking to see if we need to expire inflight stragglers.\n");

    expire_inflight( inflight );

  }

  report( complete ) ;

  exit( EXIT_SUCCESS );

}

/* end of file */
