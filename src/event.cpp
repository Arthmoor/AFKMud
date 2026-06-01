/****************************************************************************
 *                   ^     +----- |  / ^     ^ |     | +-\                  *
 *                  / \    |      | /  |\   /| |     | |  \                 *
 *                 /   \   +---   |<   | \ / | |     | |  |                 *
 *                /-----\  |      | \  |  v  | |     | |  /                 *
 *               /       \ |      |  \ |     | +-----+ +-/                  *
 ****************************************************************************
 * AFKMud Copyright 1997-2025 by Roger Libiez (Samson),                     *
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
   for( auto it = eventlist.begin(  ); it != eventlist.end(  ); )
   {
      event_info *ev = *it;
      ++it;

      free_event( ev );
   }
}

void add_event( time_t when, void ( *callback ) ( void * ), void *data )
{
   event_info *e = new event_info;

   e->when = current_time + std::chrono::seconds( when );
   e->callback = callback;
   e->data = data;

   for( auto it = eventlist.begin(); it != eventlist.end(); ++it )
   {
      event_info *cur = *it;

      if( cur->when > e->when )
      {
         eventlist.insert( it, e );
         return;
      }
   }
   eventlist.push_back( e );
}

void cancel_event( void ( *callback ) ( void * ), void *data )
{
   for( auto it = eventlist.begin(  ); it != eventlist.end(  ); )
   {
      event_info *ev = *it;
      ++it;

      if( ( !callback ) && ev->data == data )
         free_event( ev );

      else if( ( callback ) && ev->data == data && data != nullptr )
         free_event( ev );

      else if( ev->callback == callback && data == nullptr )
         free_event( ev );
   }
}

event_info *find_event( void ( *callback ) ( void * ), void *data )
{
   for( auto* ev : eventlist )
   {
      if( ev->callback == callback && ev->data == data )
         return ev;
   }
   return nullptr;
}

void run_events( std::chrono::system_clock::time_point newtime )
{
   while( !eventlist.empty(  ) )
   {
      event_info *e = eventlist.front();

      if( e->when > newtime )
         break;

      auto callback = e->callback;
      void *data = e->data;

      // Temporarily changed in case the callback function needs to know if the event matches current real world time.
      current_time = e->when;

      eventlist.pop_front();
      ++events_served;

      if( callback )
         callback ( data );
      else
         bug( "%s: nullptr callback", __func__ );

      deleteptr (e );
   }
   current_time = newtime;
}

CMDF( do_eventinfo )
{
   ch->printf( "&BPending events&c: %zu\r\n", eventlist.size(  ) );
   ch->printf( "&BEvents served &c: %ld\r\n", events_served );
}
