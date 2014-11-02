/****************************************************************************
 *                   ^     +----- |  / ^     ^ |     | +-\                  *
 *                  / \    |      | /  |\   /| |     | |  \                 *
 *                 /   \   +---   |<   | \ / | |     | |  |                 *
 *                /-----\  |      | \  |  v  | |     | |  /                 *
 *               /       \ |      |  \ |     | +-----+ +-/                  *
 ****************************************************************************
 * AFKMud Copyright 1997-2015 by Roger Libiez (Samson),                     *
 * Levi Beckerson (Whir), Michael Ward (Tarl), Erik Wolfe (Dwip),           *
 * Cameron Carroll (Cam), Cyberfox, Karangi, Rathian, Raine,                *
 * Xorith, and Adjani.                                                      *
 * All Rights Reserved.                                                     *
 *                                                                          *
 *                                                                          *
 * External contributions from Remcon, Quixadhal, Zarius, and many others.  *
 *                                                                          *
 * Original SMAUG 1.8b written by Thoric (Derek Snider) with Altrag,        *
 * Blodkai, Haus, Narn, Scryn, Swordbearer, Tricops, Gorog, Rennard,        *
 * Grishnakh, Fireblade, Edmond, Conran, and Nivek.                         *
 *                                                                          *
 * Original MERC 2.1 code by Hatchet, Furey, and Kahn.                      *
 *                                                                          *
 * Original DikuMUD code by: Hans Staerfeldt, Katja Nyboe, Tom Madsen,      *
 * Michael Seifert, and Sebastian Hammer.                                   *
 ****************************************************************************
 *                             Event Processing                             *
 *      This code is derived from the IMC2 0.10 event processing code.      *
 *      Originally written by Oliver Jowett. Licensed under the LGPL.       *
 *               See the document COPYING.LGPL for details.                 *
 ****************************************************************************/

#include "mud.h"
#include "event.h"

list < event_info * >eventlist;
long events_served = 0;

void free_event( event_info * e )
{
   eventlist.remove( e );
   deleteptr( e );
}

void free_all_events( void )
{
   list < event_info * >::iterator e;

   for( e = eventlist.begin(  ); e != eventlist.end(  ); )
   {
      event_info *ev = *e;
      ++e;

      free_event( ev );
   }
}

void add_event( time_t when, void ( *callback ) ( void * ), void *data )
{
   event_info *e;
   list < event_info * >::iterator cur;

   e = new event_info;

   e->when = current_time + when;
   e->callback = callback;
   e->data = data;

   for( cur = eventlist.begin(  ); cur != eventlist.end(  ); ++cur )
   {
      if( ( *cur )->when > e->when )
      {
         eventlist.insert( cur, e );
         return;
      }
   }
   eventlist.push_back( e );
}

void cancel_event( void ( *callback ) ( void * ), void *data )
{
   list < event_info * >::iterator e;

   for( e = eventlist.begin(  ); e != eventlist.end(  ); )
   {
      event_info *ev = *e;
      ++e;

      if( ( !callback ) && ev->data == data )
         free_event( ev );

      else if( ( callback ) && ev->data == data && data != NULL )
         free_event( ev );

      else if( ev->callback == callback && data == NULL )
         free_event( ev );
   }
}

event_info *find_event( void ( *callback ) ( void * ), void *data )
{
   list < event_info * >::iterator e;

   for( e = eventlist.begin(  ); e != eventlist.end(  ); ++e )
   {
      event_info *ev = *e;

      if( ev->callback == callback && ev->data == data )
         return ev;
   }
   return NULL;
}

time_t next_event( void ( *callback ) ( void * ), void *data )
{
   list < event_info * >::iterator e;

   for( e = eventlist.begin(  ); e != eventlist.end(  ); ++e )
   {
      event_info *ev = *e;

      if( ev->callback == callback && ev->data == data )
         return ev->when - current_time;
   }
   return -1;
}

void run_events( time_t newtime )
{
   event_info *e;
   void ( *callback ) ( void * );
   void *data;

   while( !eventlist.empty(  ) )
   {
      e = ( *eventlist.begin(  ) );

      if( e->when > newtime )
         break;

      callback = e->callback;
      data = e->data;
      current_time = e->when;

      free_event( e );
      ++events_served;

      if( callback )
         ( *callback ) ( data );
      else
         bug( "%s: NULL callback", __FUNCTION__ );
   }
   current_time = newtime;
}

CMDF( do_eventinfo )
{
   ch->printf( "&BPending events&c: %d\r\n", eventlist.size(  ) );
   ch->printf( "&BEvents served &c: %ld\r\n", events_served );
}
