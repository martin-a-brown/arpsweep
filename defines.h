/* defines.h */
/* $Id:$ */

#ifndef _DEFINES_H                                             /* _DEFINES_H */
#define _DEFINES_H

#define PROGRAM_NAME                         "arpsweep"
#define PCAP_NONBLOCK                       1
#define PCAP_NONBLOCK                       1
#define PCAP_BLOCK                          0

#define ARP_HDRLEN                          8
#define ARP_FRAME_SIZE                     60
#define ETHER_HDRLEN                       14
#define IFNAME_MAXLENGTH                   15
#define LL_ADDR_STRLEN                     18
#define IP_ADDR_STRLEN                     15

#define HRD_MAXLEN                          8
#define IPADDR_LEN                          4
#define IP_ADDR_LEN                         4

#define DEFAULT_ARPS_TO_SEND                3
#define DEFAULT_INFLIGHT                   10
#define DEFAULT_OUTPUT_FORMAT                "%T %i %p\n"
#define DEFAULT_INTERFACE_NAME               "eth0"

//#ifdef _LINUX_LINUX_LINUX
#define BPF_ONLY_ARP_REPLIES                 "arp and ether dst"
//#else
//#define BPF_ONLY_ARP_REPLIES                 "arp"
//#endif
#define SLEEP_TIME                         10
#define DEFAULT_PATIENCE                  250  /* a quarter of a second */

/* Flags used in o.flags to describe command line options */

#define AO_USAGE                       ( 1 << 0 )
#define AO_VERSION                     ( 1 << 1 )
#define AO_ALIVE                       ( 1 << 2 )
#define AO_MISSING                     ( 1 << 3 )
#define AO_WEIRD                       ( 1 << 4 )
#define AO_FIRST_REPLY                 ( 1 << 5 )
#define AO_NO_HEADER                   ( 1 << 6 )
#define AO_NO_UNICAST                  ( 1 << 7 )
#define AO_RANDOMIZE_MAC               ( 1 << 8 )
#define AO_RANDOMIZE_IP                ( 1 << 9 )

/* Flags used in arp_records current->flags */

#define AR_MULTI_REPLY                 ( 1 << 0 )
#define AR_FRAME_MAC_MISMATCH          ( 1 << 1 )
#define AR_FRAME_TRANSMIT_ERR          ( 1 << 2 )

/* Variety of log levels */

#define LOG_LEVEL_ERR                  ( 1 << 0 )
#define LOG_LEVEL_WARN                 ( 1 << 1 )
#define LOG_LEVEL_INFO                 ( 1 << 2 )
#define LOG_LEVEL_DEBUG                ( 1 << 3 )

/* Logging macros */

#define MSG(    D, F, ...) fprintf( D, F , ##__VA_ARGS__ )
#define LOGGER( L, F, ...) if ( o.verbose & L ) MSG( stderr, F , ##__VA_ARGS__ )

#define ERR(   F, ...) LOGGER( LOG_LEVEL_ERR   , F , ##__VA_ARGS__)
#define WARN(  F, ...) LOGGER( LOG_LEVEL_WARN  , F , ##__VA_ARGS__)
#define INFO(  F, ...) LOGGER( LOG_LEVEL_INFO  , F , ##__VA_ARGS__)
#define DEBUG( F, ...) LOGGER( LOG_LEVEL_DEBUG , F , ##__VA_ARGS__)

// #define FATAL( F, ...) { MSG( stderr, "File: %s; Line: %d; Function: %s\n", __FILE__, __LINE__, __func__ ); MSG( stderr, F, ##__VA_ARGS__ ); exit( EXIT_FAILURE ); }
#define FATAL( F, ...) { MSG( stderr, F, ##__VA_ARGS__ ); exit( EXIT_FAILURE ); }

#define USAGE_FATAL( F, ... ) { short_usage( EXIT_FAILURE ); FATAL( F, ##__VA_ARGS__ ); }


#endif                                                         /* _DEFINES_H */
