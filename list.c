/*
 * util.c
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

void inline 
list_link( struct arp_record *a, struct arp_record *b )
{
        a->next = b;
        b->prev = a;
}

void inline
list_init( struct arp_record * cur)
{
  list_link( cur, cur );
}

void inline
list_prepend( struct arp_record ** head, struct arp_record * cur )
{
  if ( ! *head ) { list_init( cur ) ; *head = cur ; return ; }

  struct arp_record * first = ( *head )->next ;

  list_link( *head, cur );
  list_link( cur, first );

}

void inline
list_append( struct arp_record ** head, struct arp_record * cur)
{
  if ( ! *head ) {
    *head = cur ;
    (*head)->member_count = 0;
    list_init( cur ) ;
  }

  struct arp_record * last = ( *head )->prev ;

  list_link( cur, *head );
  list_link( last, cur  );

  ++(*head)->member_count;
}

void inline
list_remove( struct arp_record ** head, struct arp_record * cur)
{

/*
  DEBUG( "  cur=%p,     cur->next=%p,     cur->prev=%p.\n\n",
            cur   ,     cur->next   ,     cur->prev );
  DEBUG( "*head=%p, (*head)->next=%p, (*head)->prev=%p.\n\n",
          *head   , (*head)->next   , (*head)->prev );
 */

  --(*head)->member_count;
  if ( *head == cur ) { 
    if ( *head == cur->next ) {
      *head = NULL;
    } else {
      cur->next->member_count = (*head)->member_count;
      *head = cur->next;
    }
  }
  list_link( cur->prev, cur->next );
  cur->next = NULL;
  cur->prev = NULL;

/*
  DEBUG( "  cur=%p,     cur->next=%p,     cur->prev=%p.\n\n",
            cur   ,     cur->next   ,     cur->prev );
  DEBUG( "*head=%p, (*head)->next=%p, (*head)->prev=%p.\n\n",
          *head   , (*head)->next   , (*head)->prev );
 */

}

void inline
list_move( struct arp_record ** src, struct arp_record ** dst )
{
  struct arp_record *cur ;
  for ( cur = *src ; *src ; cur = *src )
  {
    list_remove( src, cur );
    list_append( dst, cur );
  }
}

// int list_length( struct arp_record_head * cur )
// {
//   return cur->length ;
// }

int
list_length( struct arp_record * cur )
{
  if (! cur ) return 0;

  return cur->member_count;
}


int
list_lengthX( struct arp_record * cur )
{
  struct arp_record * head = cur;

  int i = 0;

  if ( ! cur ) return i ; /* NULL pointer, zero items */

  ++i; /* Argh!  Silly lines of code ahead. */

  if ( head == cur->next ) return i ;

  cur = cur->next;

  do
  {
    ++i;
    cur = cur->next ;
  } while ( cur != head );

  // DEBUG( "%s about to return %d.\n", __func__, i );

  return  i ;

}

/* eof */
