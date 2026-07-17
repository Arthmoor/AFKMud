/****************************************************************************
 *                   ^     +----- |  / ^     ^ |     | +-\                  *
 *                  / \    |      | /  |\   /| |     | |  \                 *
 *                 /   \   +---   |<   | \ / | |     | |  |                 *
 *                /-----\  |      | \  |  v  | |     | |  /                 *
 *               /       \ |      |  \ |     | +-----+ +-/                  *
 ****************************************************************************
 * AFKMud Copyright 1997-2026 by Roger Libiez (Samson),                     *
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
 *                          MUD Specific Functions                          *
 ****************************************************************************/

/* This file will not be covered by future patches. Any specific code you don't
 * want changed later should go here.
 */
#include <fstream>
#include "mud.h"
#include "area.h"
#include "clans.h"
#include "deity.h"
#include "overland.h"
#include "roomindex.h"

bool can_use_mprog( char_data * );

const char *SPELL_SILENT_MARKER = "silent";  /* No OK. or Failed. */
extern int astral_target;

/* New continent and plane based death relocation - Samson 3-29-98 */
// This should be expanded upon with room vnums on different continents.
// Right now it's pretty silly looking.
room_index *check_room( char_data * ch, room_index * dieroom )
{
   room_index *location = nullptr;

   if( !str_cmp( dieroom->area->continent->name, "One" ) )
      location = get_room_index( ROOM_VNUM_ALTAR );

   if( !location )
      location = get_room_index( ROOM_VNUM_ALTAR );

   return location;
}

room_index *recall_room( char_data * ch )
{
   room_index *location = nullptr;

   if( ch->isnpc(  ) )
   {
      location = get_room_index( ROOM_VNUM_TEMPLE );
      if( !location )
         location = get_room_index( ROOM_VNUM_LIMBO );
      return location;
   }

   if( ch->pcdata->clan )
      location = get_room_index( ch->pcdata->clan->recall );
   if( !location && ch->pcdata->home )
      location = get_room_index( ch->pcdata->home );

   if( !location )
   {
      if( !str_cmp( ch->in_room->area->continent->name, "One" ) )
      {
         location = get_room_index( ch->pcdata->one );

         if( !location )
            location = get_room_index( ROOM_VNUM_TEMPLE );
      }

      if( !str_cmp( ch->in_room->area->continent->name, "Astral" ) && astral_target != -1 )
         location = get_room_index( astral_target );
   }

   if( !location && ch->pcdata->deity && ch->pcdata->deity->recallroom )
      location = get_room_index( ch->pcdata->deity->recallroom );

   if( !location )
      location = get_room_index( ROOM_VNUM_TEMPLE );

   /*
    * Hey, look, if you get *THIS* damn far and still come up with nothing, you *DESERVE* to crash! 
    */
   if( !location )
      location = get_room_index( ROOM_VNUM_LIMBO );

   return location;
}

bool beacon_check( char_data * ch, room_index * beacon )
{
   return true;
}

// List of recall room definitions. Should match the data for set_rented_room, save_rented_rooms, and load_rented_rooms.
void pc_data::init_recall_rooms( void )
{
   this->one = ROOM_VNUM_TEMPLE;
}

void set_initial_recall_room( char_data * ch )
{
   ch->pcdata->one = ROOM_VNUM_TEMPLE;
}

void show_towngate_destinations( char_data * ch )
{
   ch->print( "Where do you wish to go??\r\n" );
   ch->print( "NO DESTINATIONS HAVE BEEN PROVIDED YET!\r\n" );
}

room_index *set_towngate_destination_room( std::string_view target )
{
   room_index *room = nullptr;

   // Someone needs to add some room VNUMs here cause the spell won't work without them.
   // Example of how the thing should work:

   // if( !str_cmp( target_name, "bywater" ) )
   //   room = get_room_index( 7035 );
   return room;
}

void set_rented_room( char_data * ch )
{
   if( !str_cmp( ch->in_room->area->continent->name, "One" ) )
      ch->pcdata->one = ch->in_room->vnum;
}

// This MUST match the number of rooms you assign in load_rented_rooms or shit will break!
void save_rented_rooms( char_data * ch, std::ofstream & stream )
{
   stream << std::format( "RentRooms    {}\n", ch->pcdata->one );
}

void load_rented_rooms( char_data * ch, std::ifstream & stream )
{
   // Ten for now, but can be expanded as needed. Just make sure you also alter save_rented_rooms to match the number of rooms you need.
   int x1 = 0, x2 = 0, x3 = 0, x4 = 0, x5 = 0, x6 = 0, x7 = 0, x8 = 0, x9 = 0, x10 = 0;
   std::string ln;
   std::getline( stream, ln );
   std::istringstream( ln ) >> x1 >> x2 >> x3 >> x4 >> x5 >> x6 >> x7 >> x8 >> x9 >> x10;

   ch->pcdata->one = x1;
}

void speech_tricks( char_data * ch, std::string & argument )
{
   // Nothing here yet.
}
