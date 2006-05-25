/*
 * log.c
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

#include "config.h"

#define _GNU_SOURCE
#include "arpsweep.h"

extern const float       arpsweep_version;
extern const char        progname[];
extern const char        arpsweep_copyright[];
extern const char        arpsweep_time[];
extern struct            a_options o;

#ifndef _DEFINES_H
#include "defines.h"
#endif

#define MSG(    D, F, ...) fprintf( D, F , ##__VA_ARGS__ )
#define LOGGER( L, F, ...) if ( o.verbose & L ) MSG( stderr, F , ##__VA_ARGS__ )

#define ERR(   F, ...) LOGGER( LOG_LEVEL_ERR   , F , ##__VA_ARGS__)
#define WARN(  F, ...) LOGGER( LOG_LEVEL_WARN  , F , ##__VA_ARGS__)
#define INFO(  F, ...) LOGGER( LOG_LEVEL_INFO  , F , ##__VA_ARGS__)
#define DEBUG( F, ...) LOGGER( LOG_LEVEL_DEBUG , F , ##__VA_ARGS__)

#define FATAL( F, ...) { MSG( stderr, F, ##__VA_ARGS__ ); exit( EXIT_FAILURE ); }
// #define FATAL( F, ...) { MSG( stderr, "File: %s; Line: %d; Function: %s\n", __FILE__, __LINE__, __func__ ); MSG( stderr, F, ##__VA_ARGS__ ); exit( EXIT_FAILURE ); }

#define USAGE_FATAL( F, ... ) { short_usage( EXIT_FAILURE ); FATAL( F, ##__VA_ARGS__ ); }


void
short_usage(int ret)
{
  fprintf( ( ret ? stderr : stdout ),
    "usage: %s [ OPTIONS ] [ -i <interface> ] [ <ipaddress>[,<mac>] ... ]\n",
   progname );
}

void
print_version(int ret)
{
  fprintf( ( ret ? stderr : stdout ), "%s\n%s-%.2f (compiled %s)\n",
           arpsweep_copyright, progname, arpsweep_version, arpsweep_time );
  exit( ret );
}

void
long_usage(int ret)
{
  short_usage( ret );
  fprintf( ( ret ? stderr : stdout ),
    "\n"
    "Options (defaults in parentheses):\n"
    "  -i, --interface=NAME    specify interface (%s)\n"
    "  -h, --help, --usage     display this help and exit.\n"
    "  -V, --version           display version information and exit.\n"
    "  -v, --verbose[=LEVEL]   increase verbosity, can be used more than once\n"
    "  -q, --quiet             just fatal errors and reporting output\n"
    "  -c, --count=NUM         ARP requests to send to each host (%d)\n"
    "  -p, --pending=NUM       max parallel ARP requests in flight (%d)\n"
    "  -w, --wait=MSEC         milliseconds to wait for ARP reply (%d)\n"
    "  -F, --first-reply       stop sending to a target after any reply\n"
    "  -B, --broadcast-only    send only broadcast frames\n"
    "      --no-unicast        synonym for --broadcast-only\n"
    "\nReporting options:\n"
    "  -m, --mac-format=STR    output MAC format (Linux)\n"
    "  -N, --no-header         do not print the column header on the report\n"
    "  -A, --alive             report on hosts that respond (default)\n"
    "  -M, --missing           report on hosts failing to respond\n"
    "  -W, --weird             report on mismatched responses\n"
    "  -f, --format=STR        output format string (unimplemented)\n"
    "\n",
    DEFAULT_INTERFACE_NAME,
    DEFAULT_ARPS_TO_SEND,
    DEFAULT_INFLIGHT,
    DEFAULT_PATIENCE
         );
  exit( ret );
}

int
add_loglevel(int level_change )
{
  int newlevel;

  if ( level_change == 0 )
  {
    newlevel = o.verbose | ( o.verbose << 1 );
  }
  else
  {
    newlevel = o.verbose | level_change;
  }
  return( newlevel );
}

int
loglevel(char * v_option )
{
  /* User just supplied another -v or --verbose */

  if ( v_option == NULL ) return ( add_loglevel( 0 ) );

  switch ( v_option[0] )
  {

    case 'D':                                /* fall through */
    case 'd': return( o.verbose |= LOG_LEVEL_DEBUG )   ; break ;

    case 'W':                                /* fall through */
    case 'w': return( o.verbose |= LOG_LEVEL_WARN )    ; break ;

    case 'I':                                /* fall through */
    case 'i': return( o.verbose |= LOG_LEVEL_INFO )    ; break ;

    case 'E':                                /* fall through */
    case 'e': return( o.verbose |= LOG_LEVEL_ERR )     ; break ;

    default: USAGE_FATAL(
              "Invalid option \"%s\" to verbose (-v|--verbose).\n", v_option );
  }

}

void
arpsweep_info_header()
{
  INFO("Output produced by version: arpsweep-%.2f.\n", arpsweep_version );
  DEBUG("Output produced by version: arpsweep-%.2f.\n", arpsweep_version );
  return;
}
