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
 *                             Player ships                                 *
 *                   Brought over from Lands of Altanos                     *
 ***************************************************************************/

/* This code is still in the development stages and probably has bugs - be warned */

#include <fstream>
#include "mud.h"
#include "area.h"
#include "mud_prog.h"
#include "overland.h"
#include "roomindex.h"
#include "ships.h"

void check_sneaks( char_data * );

const int SHIPFILEVERSION = 1;

const char *ship_type[] = {
   "None", "Skiff", "Coaster", "Caravel", "Galleon", "Warship"
};

const char *ship_flags[] = {
   "anchored", "onmap", "airship"
};

list < ship_data * >shiplist;

int get_shiptype( const string & type )
{
   for( int x = 0; x < SHIP_MAX; ++x )
      if( !str_cmp( type, ship_type[x] ) )
         return x;
   return -1;
}

int get_shipflag( const string & flag )
{
   for( size_t x = 0; x < ( sizeof( ship_flags ) / sizeof( ship_flags[0] ) ); ++x )
      if( !str_cmp( flag, ship_flags[x] ) )
         return x;
   return -1;
}

ship_data *ship_lookup_by_vnum( int vnum )
{
   list < ship_data * >::iterator iship;

   for( iship = shiplist.begin(  ); iship != shiplist.end(  ); ++iship )
   {
      ship_data *ship = *iship;

      if( ship->vnum == vnum )
         return ship;
   }
   return nullptr;
}

ship_data *ship_lookup( const string & name )
{
   list < ship_data * >::iterator iship;

   for( iship = shiplist.begin(  ); iship != shiplist.end(  ); ++iship )
   {
      ship_data *ship = *iship;

      if( !str_cmp( name, ship->name ) )
         return ship;
   }
   return nullptr;
}

CMDF( do_shiplist )
{
   list < ship_data * >::iterator iship;
   int count = 1;

   if( shiplist.empty(  ) )
   {
      ch->print( "&RThere are no ships created yet.\r\n" );
      return;
   }

   ch->print( "&RNumbr Ship Name          Ship Type        Owner           Hull Status\r\n" );
   ch->print( "&r===== =========          =========        =====           ===========\r\n" );
   for( iship = shiplist.begin(  ); iship != shiplist.end(  ); ++iship )
   {
      ship_data *ship = *iship;

      ch->printf( "&Y&W%4d&b&w>&g %-15s&Y    %-15s &G %-15s &Y%d/%d&g\r\n",
                  count, capitalize( ship->name ).c_str(  ), ship_type[ship->type], ship->owner.c_str(  ), ship->hull, ship->max_hull );
      count += 1;
   }
}

CMDF( do_shipstat )
{
   ship_data *ship;

   if( !( ship = ship_lookup( argument ) ) )
   {
      ch->print( "Stat which ship?\r\n" );
      return;
   }

   ch->printf( "Name:  %s\r\n", ship->name.c_str(  ) );
   ch->printf( "Owner: %s\r\n", ship->owner.c_str(  ) );
   ch->printf( "Vnum:  %d\r\n", ship->vnum );
   if( ship->flags.test( SHIP_ONMAP ) )
   {
      ch->printf( "On map: %s\r\n", ship->continent->name.c_str( ) );
      ch->printf( "Coords: %4dX %4dY\r\n", ship->map_x, ship->map_y );
   }
   else
      ch->printf( "In room: %d\r\n", ship->room );
   ch->printf( "Hull:  %d/%d\r\n", ship->hull, ship->max_hull );
   ch->printf( "Fuel:  %d/%d\r\n", ship->fuel, ship->max_fuel );
   ch->printf( "Type:  %d (%s)\r\n", ship->type, ship_type[ship->type] );
   ch->printf( "Flags: %s\r\n", bitset_string( ship->flags, ship_flags ) );
}

ship_data::ship_data(  )
{
   init_memory( &flags, &map_y, sizeof( map_y ) );
}

ship_data::~ship_data(  )
{
   shiplist.remove( this );
}

void save_ships( void )
{
   ofstream stream;
   list < ship_data * >::iterator iship;

   stream.open( SHIP_FILE );
   if( !stream.is_open(  ) )
   {
      perror( SHIP_FILE );
      bug( "%s: can't open ship file", __func__ );
      return;
   }

   for( iship = shiplist.begin(  ); iship != shiplist.end(  ); ++iship )
   {
      ship_data *ship = *iship;

      stream << "#SHIP" << endl;
      stream << "Version   " << SHIPFILEVERSION << endl;
      stream << "Name      " << ship->name << endl;
      stream << "Owner     " << ship->owner << endl;
      stream << "Flags     " << bitset_string( ship->flags, ship_flags ) << endl;
      stream << "Vnum      " << ship->vnum << endl;
      stream << "Room      " << ship->room << endl;
      stream << "Type      " << ship_type[ship->type] << endl;
      stream << "Hull      " << ship->hull << endl;
      stream << "Max_hull  " << ship->max_hull << endl;
      stream << "Fuel      " << ship->fuel << endl;
      stream << "Max_fuel  " << ship->max_fuel << endl;
      stream << "Coordinates " << ship->map_x << " " << ship->map_y << endl;
      stream << "End" << endl << endl;
   }
   stream.close(  );
}

void load_ships( void )
{
   ifstream stream;
   ship_data *ship;
   int file_ver = 0;

   shiplist.clear(  );

   stream.open( SHIP_FILE );
   if( !stream.is_open(  ) )
   {
      bug( "%s: Unable to open ships file.", __func__ );
      return;
   }

   do
   {
      string key, value;
      char buf[MIL];

      stream >> key;
      stream.getline( buf, MIL );
      value = buf;

      strip_lspace( key );
      strip_lspace( value );
      strip_tilde( value );

      if( key.empty(  ) )
         continue;

      if( key == "#SHIP" )
         ship = new ship_data;
      else if( key == "Version" )
         file_ver = atoi( value.c_str(  ) );
      else if( key == "Name" )
         ship->name = value;
      else if( key == "Owner" )
         ship->owner = value;
      else if( key == "Flags" )
         flag_string_set( value, ship->flags, ship_flags );
      else if( key == "Vnum" )
         ship->vnum = atoi( value.c_str(  ) );
      else if( key == "Room" )
         ship->room = atoi( value.c_str(  ) );
      else if( key == "Type" )
      {
         ship->type = get_shiptype( value );

         ship->type = URANGE( SHIP_NONE, ship->type, SHIP_WARSHIP );
      }
      else if( key == "Hull" )
         ship->hull = atoi( value.c_str(  ) );
      else if( key == "Max_hull" )
         ship->max_hull = atoi( value.c_str(  ) );
      else if( key == "Fuel" )
         ship->fuel = atoi( value.c_str(  ) );
      else if( key == "Max_fuel" )
         ship->max_fuel = atoi( value.c_str(  ) );
      else if( key == "Coordinates" && file_ver < 1 )
      {
         string coord;

         // Eat this first one.
         value = one_argument( value, coord );

         value = one_argument( value, coord );
         ship->map_x = atoi( coord.c_str(  ) );

         ship->map_y = atoi( value.c_str(  ) );
      }
      else if( key == "Coordinates" && file_ver >= 1 )
      {
         string coord;

         value = one_argument( value, coord );
         ship->map_x = atoi( coord.c_str(  ) );

         ship->map_y = atoi( value.c_str(  ) );
      }
      else if( key == "End" )
         shiplist.push_back( ship );
      else
         log_printf( "%s: Bad line in ships file: %s %s", __func__, key.c_str(  ), value.c_str(  ) );
   }
   while( !stream.eof(  ) );
   stream.close(  );
}

void free_ships( void )
{
   list < ship_data * >::iterator sh;

   for( sh = shiplist.begin(  ); sh != shiplist.end(  ); )
   {
      ship_data *ship = *sh;
      ++sh;

      deleteptr( ship );
   }
   shiplist.clear();
}

CMDF( do_shipset )
{
   ship_data *ship;
   string arg, mod;

   argument = one_argument( argument, arg );
   if( arg.empty(  ) )
   {
      ch->print( "Syntax: shipset <ship> create\r\n" );
      ch->print( "        shipset <ship> delete\r\n" );
      ch->print( "        shipset <ship> <field> <value>\r\n\r\n" );
      ch->print( "  Where <field> is one of:\r\n" );
      ch->print( "    name owner vnum type fuel max_fuel\r\n" );
      ch->print( "    hull max_hull coords room flags\r\n" );
      return;
   }

   argument = one_argument( argument, mod );
   ship = ship_lookup( arg );

   if( !str_cmp( mod, "create" ) )
   {
      if( ship )
      {
         ch->print( "Ship already exists.\r\n" );
         return;
      }
      ship = new ship_data;
      ship->continent = nullptr;
      ship->map_x = -1;
      ship->map_y = -1;
      ship->name = arg;
      shiplist.push_back( ship );
      save_ships(  );
      ch->print( "Ship created.\r\n" );
      return;
   }

   if( !ship )
   {
      ch->print( "Ship doesn't exist.\r\n" );
      return;
   }

   if( !str_cmp( mod, "delete" ) )
   {
      deleteptr( ship );
      save_ships(  );
      ch->print( "Ship deleted.\r\n" );
      return;
   }

   if( !str_cmp( mod, "name" ) )
   {
      if( ship_lookup( argument ) )
      {
         ch->print( "Another ship has that name.\r\n" );
         return;
      }
      ship->name = argument;
      save_ships(  );
      ch->print( "Name changed.\r\n" );
      return;
   }

   if( !str_cmp( mod, "owner" ) )
   {
      ship->owner = argument;
      save_ships(  );
      ch->print( "Ownership shifted.\r\n" );
      return;
   }

   if( !str_cmp( mod, "vnum" ) )
   {
      ship->vnum = atoi( argument.c_str(  ) );
      save_ships(  );
      ch->print( "Vnum modified.\r\n" );
      return;
   }

   if( !str_cmp( mod, "type" ) )
   {
      ship->type = atoi( argument.c_str(  ) );
      save_ships(  );
      ch->print( "Type modified.\r\n" );
      return;
   }

   if( !str_cmp( mod, "hull" ) )
   {
      ship->hull = atoi( argument.c_str(  ) );
      save_ships(  );
      ch->print( "Hull changed.\r\n" );
      return;
   }

   if( !str_cmp( mod, "max_hull" ) )
   {
      ship->max_hull = atoi( argument.c_str(  ) );
      save_ships(  );
      ch->print( "Maximum hull changed.\r\n" );
      return;
   }

   if( !str_cmp( mod, "fuel" ) )
   {
      ship->fuel = atoi( argument.c_str(  ) );
      save_ships(  );
      ch->print( "Fuel changed.\r\n" );
      return;
   }

   if( !str_cmp( mod, "max_fuel" ) )
   {
      ship->max_fuel = atoi( argument.c_str(  ) );
      save_ships(  );
      ch->print( "Maximum fuel changed.\r\n" );
      return;
   }

   if( !str_cmp( mod, "room" ) )
   {
      ship->room = atoi( argument.c_str(  ) );
      save_ships(  );
      ch->print( "Room set.\r\n" );
      return;
   }

   if( !str_cmp( mod, "coords" ) )
   {
      string arg3;

      argument = one_argument( argument, arg3 );

      if( arg3.empty(  ) || argument.empty(  ) )
      {
         ch->print( "You must specify both coordinates.\r\n" );
         return;
      }

      ship->map_x = atoi( arg3.c_str(  ) );
      ship->map_y = atoi( argument.c_str(  ) );
      save_ships(  );
      ch->print( "Coordinates changed.\r\n" );
      return;
   }

   if( !str_cmp( mod, "flags" ) )
   {
      string arg3;
      int value;

      while( !argument.empty(  ) )
      {
         argument = one_argument( argument, arg3 );
         value = get_shipflag( arg3 );
         if( value < 0 || value >= MAX_SHIP_FLAG )
            ch->printf( "Unknown flag: %s\r\n", arg3.c_str(  ) );
         else
            ship->flags.flip( value );
      }
      save_ships(  );
      return;
   }
   do_shipset( ch, "" );
}

bool can_move_ship( char_data * ch, int sector )
{
   /*
    * Water shallow enough to swim in, and land ( duh! ) cannot be sailed into 
    */
   if( sector != SECT_WATER_NOSWIM && sector != SECT_OCEAN && sector != SECT_RIVER )
   {
      if( sector == SECT_WATER_SWIM )
      {
         ch->print( "The waters are too shallow to sail into.\r\n" );
         return false;
      }
      if( sector == SECT_UNDERWATER )
      {
         ch->print( "Are you daft? Your vessel would sink!\r\n" );
         return false;
      }
      if( sector == SECT_AIR && ch->on_ship->type != SHIP_AIRSHIP )
      {
         ch->print( "Your vessel is not capable of flying.\r\n" );
         return false;
      }
      ch->print( "That's land! You'll run your ship aground!\r\n" );
      return false;
   }

   /*
    * Skiffs and Coasters can't sail into deep ocean 
    */
   if( sector == SECT_OCEAN && ch->on_ship->type <= SHIP_COASTER )
   {
      ch->printf( "Your %s is not large enough to sail into deep oceans.\r\n", ship_type[ch->on_ship->type] );
      return false;
   }

   /*
    * Bigger than a caravel, you gotta stay in ocean sectors 
    */
   if( sector != SECT_OCEAN && ch->on_ship->type > SHIP_CARAVEL )
   {
      ch->printf( "Your %s is too large to sail into anything but oceans.\r\n", ship_type[ch->on_ship->type] );
      return false;
   }
   return true;
}

/* Cheap hack of process_exit from overland.c for ships */
ch_ret process_shipexit( char_data * ch, short x, short y, int dir )
{
   int sector = ch->continent->get_terrain( x, y ), move;
   room_index *from_room;
   ship_data *ship = ch->on_ship;
   ch_ret retcode;
   continent_data *from_cont;
   short fx, fy;

   from_room = ch->in_room;
   fx = ch->map_x;
   fy = ch->map_y;
   from_cont = ch->continent;

   retcode = rNONE;
   if( ch->has_pcflag( PCFLAG_MAPEDIT ) )
   {
      ch->print( "Get off the ship before you start editing.\r\n" );
      return rSTOP;
   }

   if( sector == SECT_EXIT )
   {
      mapexit_data *mexit;
      room_index *toroom = nullptr;

      mexit = ch->continent->check_mapexit( x, y );

      if( mexit != nullptr )
      {
         if( !str_cmp( mexit->tomap, "-1" ) )  /* Means exit goes to another map */
         {
            continent_data *dest_cont = find_continent_by_name( mexit->tomap );

            if( !can_move_ship( ch, dest_cont->get_terrain( mexit->therex, mexit->therey ) ) )
               return rSTOP;

            enter_map( ch, nullptr, mexit->therex, mexit->therey, mexit->tomap );
            if( ch->mount )
               enter_map( ch->mount, nullptr, mexit->therex, mexit->therey, mexit->tomap );

            size_t chars = from_room->people.size(  );
            size_t count = 0;
            for( auto ich = from_room->people.begin(  ); ich != from_room->people.end(  ) && ( count < chars ); )
            {
               char_data *fch = *ich;
               ++ich;
               ++count;

               if( fch != ch  /* loop room bug fix here by Thoric */
                   && fch->master == ch && ( fch->position == POS_STANDING || fch->position == POS_MOUNTED ) && fch->map_x == fx && fch->map_y == fy && fch->continent == from_cont )
               {
                  if( !fch->isnpc(  ) )
                  {
                     act( AT_ACTION, "The ship sails $T.", fch, nullptr, dir_name[dir], TO_CHAR );
                     process_exit( fch, x, y, dir, false );
                  }
                  else
                     enter_map( fch, nullptr, mexit->therex, mexit->therey, mexit->tomap );
               }
            }
            return rSTOP;
         }

         if( !( toroom = get_room_index( mexit->vnum ) ) )
         {
            bug( "%s: Target vnum %d for map exit does not exist!", __func__, mexit->vnum );
            ch->print( "Ooops. Something bad happened. Contact the immortals ASAP.\r\n" );
            return rSTOP;
         }

         if( !can_move_ship( ch, toroom->sector_type ) )
            return rSTOP;

         if( !str_cmp( ch->name, ship->owner ) )
            act_printf( AT_ACTION, ch, nullptr, dir_name[dir], TO_ROOM, "%s sails off to the $T.", ship->name.c_str(  ) );

         ch->on_ship->room = toroom->vnum;

         leave_map( ch, nullptr, toroom );

         size_t chars = from_room->people.size(  );
         size_t count = 0;
         for( auto ich = from_room->people.begin(  ); ich != from_room->people.end(  ) && ( count < chars ); )
         {
            char_data *fch = *ich;
            ++ich;
            ++count;

            if( fch != ch  /* loop room bug fix here by Thoric */
                && fch->master == ch && fch->position == POS_STANDING && fch->map_x == fx && fch->map_y == fy && fch->continent == from_cont )
            {
               if( !fch->isnpc(  ) )
               {
                  act( AT_ACTION, "The ship sails $T.", fch, nullptr, dir_name[dir], TO_CHAR );
                  process_shipexit( fch, x, y, dir );
               }
               else
                  leave_map( fch, ch, toroom );
            }
         }
         return rSTOP;
      }
   }

   switch ( dir )
   {
      default:
         ch->print( "Alas, you cannot go that way...\r\n" );
         return rSTOP;

      case DIR_NORTH:
         if( y == -1 )
         {
            ch->print( "You cannot sail any further north!\r\n" );
            return rSTOP;
         }
         break;

      case DIR_EAST:
         if( x == MAX_X )
         {
            ch->print( "You cannot sail any further east!\r\n" );
            return rSTOP;
         }
         break;

      case DIR_SOUTH:
         if( y == MAX_Y )
         {
            ch->print( "You cannot sail any further south!\r\n" );
            return rSTOP;
         }
         break;

      case DIR_WEST:
         if( x == -1 )
         {
            ch->print( "You cannot sail any further west!\r\n" );
            return rSTOP;
         }
         break;

      case DIR_NORTHEAST:
         if( x == MAX_X || y == -1 )
         {
            ch->print( "You cannot sail any further northeast!\r\n" );
            return rSTOP;
         }
         break;

      case DIR_NORTHWEST:
         if( x == -1 || y == -1 )
         {
            ch->print( "You cannot sail any further northwest!\r\n" );
            return rSTOP;
         }
         break;

      case DIR_SOUTHEAST:
         if( x == MAX_X || y == MAX_Y )
         {
            ch->print( "You cannot sail any further southeast!\r\n" );
            return rSTOP;
         }
         break;

      case DIR_SOUTHWEST:
         if( x == -1 || y == MAX_Y )
         {
            ch->print( "You cannot sail any further southwest!\r\n" );
            return rSTOP;
         }
         break;
   }

   if( !can_move_ship( ch, sector ) )
      return rSTOP;

   move = sect_show[sector].move;

   if( ship->fuel < move && !ch->is_immortal(  ) )
   {
      ch->print( "Your ship is too low on magical energy to sail further ahead.\r\n" );
      return rSTOP;
   }

   if( !ch->is_immortal(  ) && !str_cmp( ch->name, ship->owner ) )
      ship->fuel -= move;

   if( !str_cmp( ch->name, ship->owner ) )
      act_printf( AT_ACTION, ch, nullptr, dir_name[dir], TO_ROOM, "%s sails off to the $T.", ship->name.c_str(  ) );

   ch->map_x = x;
   ch->map_y = y;
   ship->map_x = x;
   ship->map_y = y;

   if( !str_cmp( ch->name, ship->owner ) )
   {
      const char *txt = rev_exit( dir );
      act_printf( AT_ACTION, ch, nullptr, nullptr, TO_ROOM, "%s sails in from the %s.", ship->name.c_str(  ), txt );
   }

   size_t chars = from_room->people.size(  );
   size_t count = 0;
   for( auto ich = from_room->people.begin(  ); ich != from_room->people.end(  ) && ( count < chars ); )
   {
      char_data *fch = *ich;
      ++ich;
      ++count;

      if( fch != ch  /* loop room bug fix here by Thoric */
          && fch->master == ch && ( fch->position == POS_STANDING || fch->position == POS_MOUNTED ) && fch->map_x == fx && fch->map_y == fy )
      {
         if( !fch->isnpc(  ) )
         {
            act( AT_ACTION, "The ship sails $T.", fch, nullptr, dir_name[dir], TO_CHAR );
            process_exit( fch, x, y, dir, false );
         }
         else
         {
            fch->map_x = x;
            fch->map_y = y;
         }
      }
   }
   interpret( ch, "look" );
   return retcode;
}

/* Hacked code from the move_char function -Shatai */
/* Rehacked by Samson - had to clean up the mess */
ch_ret move_ship( char_data * ch, exit_data * pexit, int direction )
{
   ship_data *ship = ch->on_ship;
   ch_ret retcode;
   short door;
   int newx, newy, move;

   retcode = rNONE;

   if( ch->has_pcflag( PCFLAG_ONMAP ) || ch->has_actflag( ACT_ONMAP ) )
   {
      newx = ch->map_x;
      newy = ch->map_y;

      switch ( direction )
      {
         default:
            break;
         case DIR_NORTH:
            newy = ch->map_y - 1;
            break;
         case DIR_EAST:
            newx = ch->map_x + 1;
            break;
         case DIR_SOUTH:
            newy = ch->map_y + 1;
            break;
         case DIR_WEST:
            newx = ch->map_x - 1;
            break;
         case DIR_NORTHEAST:
            newx = ch->map_x + 1;
            newy = ch->map_y - 1;
            break;
         case DIR_NORTHWEST:
            newx = ch->map_x - 1;
            newy = ch->map_y - 1;
            break;
         case DIR_SOUTHEAST:
            newx = ch->map_x + 1;
            newy = ch->map_y + 1;
            break;
         case DIR_SOUTHWEST:
            newx = ch->map_x - 1;
            newy = ch->map_y + 1;
            break;
      }
      if( newx == ch->map_x && newy == ch->map_y )
         return rSTOP;

      retcode = process_shipexit( ch, newx, newy, direction );
      return retcode;
   }

   room_index *in_room = ch->in_room;
   room_index *from_room = in_room;
   room_index *to_room;
   if( !pexit || !( to_room = pexit->to_room ) )
   {
      ch->print( "Alas, you cannot sail that way.\r\n" );
      check_sneaks( ch );
      return rSTOP;
   }

   door = pexit->vdir;

   /*
    * Overland Map stuff - Samson 7-31-99 
    * Upgraded 4-28-00 to allow mounts and charmies to follow PC - Samson 
    */
   if( IS_EXIT_FLAG( pexit, EX_OVERLAND ) )
   {
      if( !valid_coordinates( pexit->map_x, pexit->map_y ) )
      {
         bug( "%s: Room #%d - Invalid exit coordinates: %d %d", __func__, in_room->vnum, pexit->map_x, pexit->map_y );
         ch->print( "Oops. Something is wrong with this map exit - notify the immortals.\r\n" );
         check_sneaks( ch );
         return rSTOP;
      }

      if( !ch->isnpc(  ) )
      {
         enter_map( ch, pexit, pexit->map_x, pexit->map_y, "-1" );
         if( ch->mount )
            enter_map( ch->mount, pexit, pexit->map_x, pexit->map_y, "-1" );

         size_t chars = from_room->people.size(  );
         size_t count = 0;
         for( auto ich = from_room->people.begin(  ); ich != from_room->people.end(  ) && ( count < chars ); )
         {
            char_data *fch = *ich;
            ++ich;
            ++count;

            if( fch != ch  /* loop room bug fix here by Thoric */
                && fch->master == ch && ( fch->position == POS_STANDING || fch->position == POS_MOUNTED ) )
            {
               if( !fch->isnpc(  ) )
               {
                  act( AT_ACTION, "The ship sails $T.", fch, nullptr, dir_name[direction], TO_CHAR );
                  move_char( fch, pexit, 0, direction, false );
               }
               else
                  enter_map( fch, pexit, pexit->map_x, pexit->map_y, "-1" );
            }
         }
      }
      else
      {
         if( !IS_EXIT_FLAG( pexit, EX_NOMOB ) )
         {
            enter_map( ch, pexit, pexit->map_x, pexit->map_y, "-1" );
            if( ch->mount )
               enter_map( ch->mount, pexit, pexit->map_x, pexit->map_y, "-1" );

            size_t chars = from_room->people.size(  );
            size_t count = 0;
            for( auto ich = from_room->people.begin(  ); ich != from_room->people.end(  ) && ( count < chars ); )
            {
               char_data *fch = *ich;
               ++ich;
               ++count;

               if( fch != ch  /* loop room bug fix here by Thoric */
                   && fch->master == ch && ( fch->position == POS_STANDING || fch->position == POS_MOUNTED ) )
               {
                  if( !fch->isnpc(  ) )
                  {
                     act( AT_ACTION, "The ship sails $T.", fch, nullptr, dir_name[direction], TO_CHAR );
                     move_char( fch, pexit, 0, direction, false );
                  }
                  else
                     enter_map( fch, pexit, pexit->map_x, pexit->map_y, "-1" );
               }
            }
         }
      }
      check_sneaks( ch );
      return rSTOP;
   }

   if( !can_move_ship( ch, pexit->to_room->sector_type ) )
   {
      check_sneaks( ch );
      return rSTOP;
   }

   if( IS_EXIT_FLAG( pexit, EX_PORTAL ) )
   {
      ch->print( "You cannot sail a ship through that!!\r\n" );
      check_sneaks( ch );
      return rSTOP;
   }

   if( !ch->is_immortal(  ) && !ch->isnpc(  ) && ch->in_room->area != to_room->area )
   {
      if( ch->level < to_room->area->low_hard_range )
      {
         switch ( to_room->area->low_hard_range - ch->level )
         {
            case 1:
               ch->print( "&[tell]A voice in your mind says, 'You are nearly ready to go that way...'\r\n" );
               break;
            case 2:
               ch->print( "&[tell]A voice in your mind says, 'Soon you shall be ready to travel down this path... soon.'\r\n" );
               break;
            case 3:
               ch->print( "&[tell]A voice in your mind says, 'You are not ready to go down that path... yet.'\r\n" );
               break;
            default:
               ch->print( "&[tell]A voice in your mind says, 'You are not ready to go down that path.'\r\n" );
         }
         check_sneaks( ch );
         return rSTOP;
      }
      else if( ch->level > to_room->area->hi_hard_range )
      {
         ch->print( "&[tell]A voice in your mind says, 'There is nothing more for you down that path.'" );
         check_sneaks( ch );
         return rSTOP;
      }
   }

   /*
    * Tunnels in water sectors only check for ships, not people since they're generally too small to matter 
    */
   if( to_room->tunnel > 0 )
   {
      int count = 0;
      list < ship_data * >::iterator other;

      for( other = shiplist.begin(  ); other != shiplist.end(  ); ++other )
      {
         ship_data *shp = *other;

         if( shp->room == to_room->vnum )
            ++count;

         if( count >= to_room->tunnel )
         {
            ch->print( "There are too many ships ahead to pass.\r\n" );
            check_sneaks( ch );
            return rSTOP;
         }
      }
   }

   move = sect_show[in_room->sector_type].move;

   if( ship->fuel < move && !ch->is_immortal(  ) )
   {
      ch->print( "Your ship is too low on magical energy to sail further ahead.\r\n" );
      return rSTOP;
   }

   if( !str_cmp( ch->name, ship->owner ) && !ch->is_immortal(  ) )
      ship->fuel -= move;

   rprog_leave_trigger( ch );
   if( ch->char_died(  ) )
      return global_retcode;

   if( !str_cmp( ch->name, ship->owner ) )
      act_printf( AT_ACTION, ch, nullptr, dir_name[door], TO_ROOM, "%s sails off to the $T.", ship->name.c_str(  ) );

   ch->from_room(  );
   if( ch->mount )
   {
      rprog_leave_trigger( ch->mount );
      if( ch->char_died(  ) )
         return global_retcode;
      if( ch->mount )
      {
         ch->mount->from_room(  );
         if( !ch->mount->to_room( to_room ) )
            log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __func__, __LINE__ );
      }
   }
   if( !ch->to_room( to_room ) )
      log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __func__, __LINE__ );
   ship->room = to_room->vnum;
   check_sneaks( ch );

   if( !str_cmp( ch->name, ship->owner ) )
   {
      const char *txt = rev_exit( door );

      act_printf( AT_ACTION, ch, nullptr, nullptr, TO_ROOM, "%s sails in from the %s.", ship->name.c_str(  ), txt );
   }

   size_t chars = from_room->people.size(  );
   size_t count = 0;
   for( auto ich = from_room->people.begin(  ); ich != from_room->people.end(  ) && ( count < chars ); )
   {
      char_data *fch = *ich;
      ++ich;
      ++count;

      if( fch != ch  /* loop room bug fix here by Thoric */
          && fch->master == ch && ( fch->position == POS_STANDING || fch->position == POS_MOUNTED ) )
      {
         act( AT_ACTION, "The ship sails $T.", fch, nullptr, dir_name[door], TO_CHAR );
         move_char( fch, pexit, 0, direction, false );
      }
   }
   interpret( ch, "look" );
   return retcode;
}
