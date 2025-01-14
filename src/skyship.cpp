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
 *  Skyship Module for Overland - based off of Ymris' Dragonflight Module   *
 *        Original Dragonflight Module written by Ymris of Terahoun         *
 ****************************************************************************/

#include <fstream>
#include "mud.h"
#include "event.h"
#include "mobindex.h"
#include "overland.h"

void continent_data::fread_landing_site( ifstream & stream )
{
   landing_data *landing_site = new landing_data;

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

      if( key == "Coordinates" )
      {
         string coord;

         value = one_argument( value, coord );
         landing_site->map_x = atoi( coord.c_str(  ) );

         landing_site->map_y = atoi( value.c_str(  ) );
      }
      else if( key == "Area" )
         landing_site->area = value;
      else if( key == "Cost" )
         landing_site->cost = atoi( value.c_str(  ) );
      else if( key == "End" )
      {
         this->landing_sites.push_back( landing_site );
         return;
      }
      else
         log_printf( "%s: %s - Bad line reading landing sites: %s %s", __func__, this->name.c_str(  ), key.c_str(  ), value.c_str(  ) );
   }
   while( !stream.eof(  ) );

   bug( "%s: Filestream reached premature EOF reading landing sites for continent %s - FATAL ERROR: Aborting file read.", __func__, this->name.c_str(  ) );
   shutdown_mud( "Corrupt continent file." );
}

/*
 * Remove a skyship when it is no longer needed
 */
void purge_skyship( char_data * ch, char_data * skyship )
{
   ch->print_room( "The skyship pilot ascends and takes to the wind.\r\n" );
   skyship->map_x = 0;
   skyship->map_y = 0;

   /*
    * Release the player from the skyship 
    */
   ch->unset_pcflag( PCFLAG_BOARDED );
   ch->has_skyship = false;
   ch->my_skyship = nullptr;

   /*
    * After this short timer runs out, it'll be extracted properly 
    */
   skyship->timer = 1;
   cancel_event( ev_skyship, ch );
}

/*
 * Skyship landing function
 */
void land_skyship( char_data * ch, char_data * skyship, bool arrived )
{
   if( !ch->isnpc(  ) && arrived )
   {
      ch->map_x = skyship->dcoordx;
      ch->map_y = skyship->dcoordy;
      ch->inflight = false;
      cancel_event( ev_skyship, ch );
   }

   skyship->continent = skyship->my_rider->continent;
   skyship->map_x = skyship->my_rider->map_x;
   skyship->map_y = skyship->my_rider->map_y;
   skyship->backtracking = false;
   skyship->inflight = false;

   if( skyship->has_actflag( ACT_BOARDED ) )
   {
      interpret( ch, "look" );
      skyship->print_room( "&CThe skyship descends and lands on the platform.\r\n" );
      skyship->print_room( "Everyone aboard the ship disembarks.\r\n" );
      purge_skyship( ch, skyship );
   }
   else
      skyship->print_room( "&CA skyship descends from above and lands on the platform.\r\n" );
}

/*
 * Skyship flight function
 */
void fly_skyship( char_data * ch, char_data * skyship )
{
   char_data *pair;
   double dist, angle;
   int speed = 10;   /* Speed of the skyships on the overland */

   /*
    * Reset the boredom counter 
    */
   skyship->zzzzz = 0;

   if( ch && skyship->has_actflag( ACT_BOARDED ) )
      pair = ch;
   else
   {
      pair = skyship;
      speed *= 5;
   }

   /*
    * If skyship is close to the landing site... 
    */
   if( ( ( skyship->map_y - skyship->dcoordy ) <= speed )
       && ( skyship->map_y - skyship->dcoordy ) >= -speed && ( ( skyship->map_x - skyship->dcoordx ) <= speed ) && ( skyship->map_x - skyship->dcoordx ) >= -speed )
   {
      land_skyship( pair, skyship, true );
      return;
   }

   /*
    * up up and away 
    */
   dist = distance( skyship->map_x, skyship->map_y, skyship->dcoordx, skyship->dcoordy );
   angle = calc_angle( skyship->map_x, skyship->map_y, skyship->dcoordx, skyship->dcoordy, &dist );

   if( angle == -1 )
      skyship->heading = -1;
   else if( angle >= 360 )
      skyship->heading = DIR_NORTH;
   else if( angle >= 315 )
      skyship->heading = DIR_NORTHWEST;
   else if( angle >= 270 )
      skyship->heading = DIR_WEST;
   else if( angle >= 225 )
      skyship->heading = DIR_SOUTHWEST;
   else if( angle >= 180 )
      skyship->heading = DIR_SOUTH;
   else if( angle >= 135 )
      skyship->heading = DIR_SOUTHEAST;
   else if( angle >= 90 )
      skyship->heading = DIR_EAST;
   else if( angle >= 45 )
      skyship->heading = DIR_NORTHEAST;
   else if( angle >= 0 )
      skyship->heading = DIR_NORTH;

   /*
    * move towards dest in steps of "speed" rooms  (salt to taste) 
    */
   switch ( skyship->heading )
   {
      case DIR_NORTH:
         pair->map_y = pair->map_y - speed;
         if( pair == ch )
            skyship->map_y = skyship->map_y - speed;
         break;

      case DIR_EAST:
         pair->map_x = pair->map_x + speed;
         if( pair == ch )
            skyship->map_x = skyship->map_x + speed;
         break;

      case DIR_SOUTH:
         pair->map_y = pair->map_y + speed;
         if( pair == ch )
            skyship->map_y = skyship->map_y + speed;
         break;

      case DIR_WEST:
         pair->map_x = pair->map_x - speed;
         if( pair == ch )
            skyship->map_x = skyship->map_x - speed;
         break;

      case DIR_NORTHEAST:
         pair->map_x = pair->map_x + speed;
         pair->map_y = pair->map_y - speed;
         if( pair == ch )
         {
            skyship->map_x = skyship->map_x + speed;
            skyship->map_y = skyship->map_y - speed;
         }
         break;

      case DIR_NORTHWEST:
         pair->map_x = pair->map_x - speed;
         pair->map_y = pair->map_y - speed;
         if( pair == ch )
         {
            skyship->map_x = skyship->map_x - speed;
            skyship->map_y = skyship->map_y - speed;
         }
         break;

      case DIR_SOUTHEAST:
         pair->map_x = pair->map_x + speed;
         pair->map_y = pair->map_y + speed;
         if( pair == ch )
         {
            skyship->map_x = skyship->map_x + speed;
            skyship->map_y = skyship->map_y + speed;
         }
         break;

      case DIR_SOUTHWEST:
         pair->map_x = pair->map_x - speed;
         pair->map_y = pair->map_y + speed;
         if( pair == ch )
         {
            skyship->map_x = skyship->map_x - speed;
            skyship->map_y = skyship->map_y + speed;
         }
         break;

      default:
         break;
   }

   collect_followers( ch, ch->in_room, ch->in_room );

   /*
    * Reversed the order of these calls because of how the Overland clears the screen - Samson 
    */
   if( skyship->has_actflag( ACT_BOARDED ) )
      interpret( pair, "look" );

   /*
    * If skyship is looking for a better spot....
    */
   if( skyship->backtracking )
      land_skyship( pair, skyship, false );
}

/*
 * Create a skyship 
 */
void create_skyship( char_data * ch )
{
   mob_index *vskyship = nullptr;
   char_data *skyship;

   if( !( vskyship = get_mob_index( MOB_VNUM_SKYSHIP ) ) )
   {
      bug( "%s: Vnum %d not found for skyship", __func__, MOB_VNUM_SKYSHIP );
      return;
   }

   skyship = vskyship->create_mobile(  );

   /*
    * pick a random set of coordinates 
    */
   /*
    * and create skyship at the coords 
    */
   /*
    * skyship can be safely spawned using the same room as the PC calling it - Samson 
    */
   if( !skyship->to_room( ch->in_room ) )
      log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __func__, __LINE__ );
   skyship->set_actflag( ACT_ONMAP );
   skyship->inflight = true;
   skyship->heading = -1;
   skyship->continent = ch->continent;
   skyship->map_x = ch->map_x;
   skyship->map_y = ch->map_y;

   /*
    * Bond the player and skyship together 
    */
   ch->my_skyship = skyship;
   skyship->my_rider = ch;

   /*
    * Set the launch coords for backtracking, if needed later 
    */
   skyship->lcoordx = ch->map_x;
   skyship->lcoordy = ch->map_y;

   /*
    * fly skyship to player location 
    */
   skyship->dcoordx = ch->map_x;
   skyship->dcoordy = ch->map_y;

   add_event( 3, ev_skyship, skyship );
   fly_skyship( nullptr, skyship );
}

/*
 * Call a skyship 
 */
CMDF( do_call )
{
   short sector; 

   /*
    * Sanity checks Reasons why a skyship wouldn't want to answer.
    * You a smelly mobbie?? 
    */
   if( ch->isnpc(  ) )
   {
      ch->print( "Sorry, mobs cannot use skyships to get around.\r\n" );
      return;
   }

   /*
    * Simplifies things to keep skyships on the Overland only - Samson 
    */
   if( !ch->has_pcflag( PCFLAG_ONMAP ) )
   {
      ch->print( "Skyships may only be called on the Overland.\r\n" );
      return;
   }

   /*
    * already has a skyship 
    */
   if( ch->my_skyship )
   {
      ch->print( "You have already sent for a skyship, be patient.\r\n" );
      return;
   }

   sector = ch->continent->get_terrain( ch->map_x, ch->map_y );
   if( sector != SECT_LANDING )
   {
      ch->print( "A skyship will not land except at a designated landing site.\r\n" );
      return;
   }
   ch->print( "You send for a skyship.\r\n" );
   act( AT_PLAIN, "$n sends for a skyship.", ch, nullptr, nullptr, TO_ROOM );
   create_skyship( ch );
}

landing_data *continent_data::check_landing_site( short x, short y )
{
   list < landing_data * >::iterator ilanding;

   for( ilanding = this->landing_sites.begin(  ); ilanding != landing_sites.end(  ); ++ilanding )
   {
      landing_data *landing = *ilanding;

      if( landing->map_x == x && landing->map_y == y )
         return landing;
   }
   return nullptr;
}

/*
 * Command to Fly a skyship 
 */
CMDF( do_fly )
{
   list < landing_data * >::iterator ilanding;
   char_data *skyship = nullptr;
   landing_data *lsite = nullptr;
   int cost = 0;

   if( !ch->has_pcflag( PCFLAG_ONMAP ) )
   {
      ch->print( "This command can only be used from the Overland.\r\n" );
      return;
   }

   if( !ch->my_skyship )
   {
      ch->print( "You have not called for a skyship, how do you expect to do that?\r\n" );
      return;
   }

   if( !ch->has_pcflag( PCFLAG_BOARDED ) )
   {
      ch->print( "You aren't on a skyship.\r\n" );
      return;
   }

   skyship = ch->my_skyship;

   if( !is_same_char_map( skyship, ch ) )
   {
      ch->print( "The skyship you boarded is not here?!?!?\r\n" );
      return;
   }

   if( argument.empty(  ) )
   {
      ch->print( "You need to specify a destination first.\r\n" );
      return;
   }

   lsite = ch->continent->check_landing_site( ch->map_x, ch->map_y );

   for( ilanding = ch->continent->landing_sites.begin(  ); ilanding != ch->continent->landing_sites.end(  ); ++ilanding )
   {
      landing_data *landing = *ilanding;

      if( !landing->area.empty(  ) && !str_prefix( argument, landing->area ) )
      {
         if( lsite && !str_cmp( landing->area, lsite->area ) )
         {
            ch->printf( "You are already at %s though!\r\n", argument.c_str(  ) );
            return;
         }

         cost = landing->cost;

         if( ch->gold < cost )
         {
            ch->printf( "A flight to %s will cost you %d, which you cannot afford right now.\r\n", landing->area.c_str(  ), landing->cost );
            return;
         }
         ch->gold -= cost;

         ch->printf( "The skyship pilot takes your gold and charts a course to %s.\r\n", landing->area.c_str(  ) );
         skyship->dcoordx = landing->map_x;
         skyship->dcoordy = landing->map_y;
         skyship->backtracking = false;
         ch->inflight = true;
         add_event( 3, ev_skyship, ch );
         ch->print_room( "The skyship ascends and takes to the wind.\r\n" );
         return;
      }
   }
   ch->printf( "There is no landing site in the vicinity of %s.\r\n", argument.c_str(  ) );
   skyship->dcoordx = ch->map_x;
   skyship->dcoordy = ch->map_y;
}

CMDF( do_board )
{
   char_data *skyship = ch->my_skyship;

   if( !ch->has_pcflag( PCFLAG_ONMAP ) )
   {
      ch->print( "This command can only be used from the Overland.\r\n" );
      return;
   }

   if( !skyship )
   {
      ch->print( "You have not called for a skyship, how do you expect to do that?\r\n" );
      return;
   }

   if( !is_same_char_map( ch, skyship ) )
   {
      ch->print( "You cannot board yet, the skyship hasn't arrived.\r\n" );
      return;
   }

   if( ch->has_pcflag( PCFLAG_BOARDED ) )
   {
      ch->print( "You have already boarded the skyship.\r\n" );
      return;
   }
   skyship->set_actflag( ACT_BOARDED );
   ch->set_pcflag( PCFLAG_BOARDED );
   ch->print( "You climb abord the skyship and settle into your seat.\r\n" );
}

/*
 * Landing Site Stuff
 *   I take no credit for originality here on down.  This is 
 *   DIRECTLY "ADAPTED" from Samson's landmark code in overland.c
 */
landing_data::landing_data(  )
{
   init_memory( &cost, &map_y, sizeof( map_y ) );
}

landing_data::~landing_data(  )
{
}

void continent_data::add_landing_site( short x, short y )
{
   landing_data *landing;

   landing = new landing_data;
   landing->map_x = x;
   landing->map_y = y;
   landing->cost = 50000;

   this->landing_sites.push_back( landing );
   this->save( );
   this->save_png_file( );
}

void continent_data::delete_landing_site( landing_data * landing )
{
   this->landing_sites.remove( landing );

   deleteptr( landing );

   this->save(  );
   this->save_png_file(  );
}

/* Support command to list all landing sites currently loaded */
CMDF( do_landing_sites )
{
   list < continent_data * >::iterator cont;
   list < landing_data * >::iterator ilanding;

   ch->pager( "Continent | Coordinates | Area             | Cost     \r\n" );
   ch->pager( "------------------------------------------------------\r\n" );

   for( cont = continent_list.begin(  ); cont != continent_list.end(  ); ++cont )
   {
      continent_data *continent = *cont;

      if( continent->landing_sites.empty(  ) && continent->nogrid == false )
      {
         ch->pagerf( "%s: No landing sites defined.\r\n", continent->name.c_str( ) );
         continue;
      }

      for( ilanding = continent->landing_sites.begin(  ); ilanding != continent->landing_sites.end(  ); ++ilanding )
      {
         landing_data *landing = *ilanding;

         ch->pagerf( "%-10s  %-4dX %-4dY   %-15s   %d\r\n", continent->name.c_str(  ), landing->map_x, landing->map_y, landing->area.c_str(  ), landing->cost );
      }
   }
}

/* OLC command to add/delete/edit landing site information */
CMDF( do_setlanding )
{
   landing_data *landing = nullptr;
   string arg;

   if( ch->isnpc(  ) )
   {
      ch->print( "Sorry, NPCs have to walk.\r\n" );
      return;
   }

   if( !ch->has_pcflag( PCFLAG_ONMAP ) )
   {
      ch->print( "This command can only be used from the overland.\r\n" );
      return;
   }

   argument = one_argument( argument, arg );

   if( arg.empty(  ) || !str_cmp( arg, "help" ) )
   {
      ch->print( "Usage: setlanding add\r\n" );
      ch->print( "Usage: setlanding delete\r\n" );
      ch->print( "Usage: setlanding area <area name>\r\n" );
      ch->print( "Usage: setlanding cost <cost>\r\n" );
      return;
   }

   landing = ch->continent->check_landing_site( ch->map_x, ch->map_y );

   if( !str_cmp( arg, "add" ) )
   {
      if( landing )
      {
         ch->print( "There's already a landing site at this location.\r\n" );
         return;
      }
      ch->continent->add_landing_site( ch->map_x, ch->map_y );
      ch->continent->putterr( ch->map_x, ch->map_y, SECT_LANDING );
      ch->print( "Landing site added.\r\n" );
      return;
   }

   if( !landing )
   {
      ch->print( "There is no landing site here.\r\n" );
      return;
   }

   if( !str_cmp( arg, "delete" ) )
   {
      ch->continent->delete_landing_site( landing );
      ch->continent->putterr( ch->map_x, ch->map_y, SECT_OCEAN );
      ch->print( "Landing site deleted.\r\n" );
      return;
   }

   if( !str_cmp( arg, "area" ) )
   {
      landing->area = argument;
      ch->continent->save(  );
      ch->printf( "Area set to %s.\r\n", argument.c_str(  ) );
      return;
   }

   if( !str_cmp( arg, "cost" ) )
   {
      landing->cost = atoi( argument.c_str(  ) );
      ch->continent->save(  );
      ch->printf( "Landing site cost set to %d\r\n", landing->cost );
      return;
   }
   do_setlanding( ch, "" );
}
