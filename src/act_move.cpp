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
 *                        Player movement module                            *
 ****************************************************************************/

#include "mud.h"
#include "area.h"
#include "mud_prog.h"
#include "objindex.h"
#include "overland.h"
#include "roomindex.h"

ch_ret move_ship( char_data *, exit_data *, int ); /* Movement if your on a ship */
ch_ret check_room_for_traps( char_data *, int );

/* Return the hide affect for sneaking PCs when they enter a new room */
void check_sneaks( char_data * ch )
{
   if( ch->is_affected( gsn_sneak ) && !ch->has_aflag( AFF_HIDE ) )
   {
      ch->set_aflag( AFF_HIDE );
      ch->set_aflag( AFF_SNEAK );
   }
}

/*
 * Modify movement due to encumbrance				-Thoric
 */
short encumbrance( char_data * ch, short move )
{
   int cur, max;

   max = ch->can_carry_w(  );
   cur = ch->carry_weight;
   if( cur >= max )
      return move * 4;
   else if( cur >= max * 0.95 )
      return ( short )( move * 3.5 );
   else if( cur >= max * 0.90 )
      return move * 3;
   else if( cur >= max * 0.85 )
      return ( short )( move * 2.5 );
   else if( cur >= max * 0.80 )
      return move * 2;
   else if( cur >= max * 0.75 )
      return ( short )( move * 1.5 );
   else
      return move;
}

/*
 * Check to see if a character can fall down, checks for looping   -Thoric
 */
bool will_fall( char_data * ch, int fall )
{
   if( !ch )
   {
      bug( "%s: nullptr *ch!!", __func__ );
      return false;
   }

   if( !ch->in_room )
   {
      bug( "%s: Character in nullptr room: %s", __func__, ch->name ? ch->name : "Unknown?!?" );
      return false;
   }

   if( ch->in_room->flags.test( ROOM_NOFLOOR ) && CAN_GO( ch, DIR_DOWN ) && ( !ch->has_aflag( AFF_FLYING ) || ( ch->mount && !ch->mount->has_aflag( AFF_FLYING ) ) ) )
   {
      if( fall > 80 )
      {
         bug( "%s: Falling (in a loop?) more than 80 rooms: vnum %d", __func__, ch->in_room->vnum );
         leave_map( ch, nullptr, get_room_index( ROOM_VNUM_TEMPLE ) );
         fall = 0;
         return true;
      }
      ch->print( "&[falling]You're falling down...\r\n" );
      move_char( ch, ch->in_room->get_exit( DIR_DOWN ), ++fall, DIR_DOWN, false );
      return true;
   }
   return false;
}

/* Run command taken from DOTD codebase - Samson 2-25-99 */
CMDF( do_run )
{
   string arg;
   continent_data *from_cont;
   room_index *from_room;
   exit_data *pexit;
   int amount = 0, x, fromx, fromy;
   bool limited = false;

   argument = one_argument( argument, arg );

   if( arg.empty(  ) )
   {
      ch->print( "Run where?\r\n" );
      return;
   }

   if( ch->position != POS_STANDING && ch->position != POS_MOUNTED )
   {
      ch->print( "You are not in the correct position for that.\r\n" );
      return;
   }

   if( !argument.empty(  ) )
   {
      if( is_number( argument ) )
      {
         limited = true;
         amount = atoi( argument.c_str(  ) );
      }
   }

   from_room = ch->in_room;
   from_cont = ch->continent;
   fromx = ch->map_x;
   fromy = ch->map_y;

   if( limited )
   {
      for( x = 1; x <= amount; ++x )
      {
         if( ( pexit = find_door( ch, arg, true ) ) != nullptr )
         {
            if( ch->move < 1 )
            {
               ch->print( "You are too exhausted to run anymore.\r\n" );
               ch->move = 0;
               break;
            }

            if( move_char( ch, pexit, 0, pexit->vdir, true ) == rSTOP )
               break;

            if( ch->position != POS_STANDING && ch->position != POS_MOUNTED )
            {
               ch->print( "Your run has been interrupted!\r\n" );
               break;
            }
         }
      }
   }
   else
   {
      while( ( pexit = find_door( ch, arg, true ) ) != nullptr )
      {
         if( ch->move < 1 )
         {
            ch->print( "You are too exhausted to run anymore.\r\n" );
            ch->move = 0;
            break;
         }

         if( move_char( ch, pexit, 0, pexit->vdir, true ) == rSTOP )
            break;

         if( ch->position != POS_STANDING && ch->position != POS_MOUNTED )
         {
            ch->print( "Your run has been interrupted!\r\n" );
            break;
         }
      }
   }

   if( ch->has_pcflag( PCFLAG_ONMAP ) || ch->has_actflag( ACT_ONMAP ) )
   {
      if( ch->map_x == fromx && ch->map_y == fromy && ch->continent == from_cont )
      {
         ch->print( "You try to run but don't get anywhere.\r\n" );
         act( AT_ACTION, "$n tries to run but doesn't get anywhere.", ch, nullptr, nullptr, TO_ROOM );
         return;
      }
   }
   else
   {
      if( ch->in_room == from_room )
      {
         ch->print( "You try to run but don't get anywhere.\r\n" );
         act( AT_ACTION, "$n tries to run but doesn't get anywhere.", ch, nullptr, nullptr, TO_ROOM );
         return;
      }
   }

   ch->print( "You slow down after your run.\r\n" );
   act( AT_ACTION, "$n slows down after $s run.", ch, nullptr, nullptr, TO_ROOM );

   interpret( ch, "look" );
}

ch_ret move_char( char_data * ch, exit_data * pexit, int fall, int direction, bool running )
{
   room_index *in_room, *to_room, *from_room;
   const char *txt, *dtxt;
   ch_ret retcode;
   short door;
   bool drunk = false, brief = false;

   retcode = rNONE;
   txt = nullptr;

   if( ch->has_pcflag( PCFLAG_ONSHIP ) && ch->on_ship != nullptr )
   {
      retcode = move_ship( ch, pexit, direction );
      return retcode;
   }

   if( ch->has_pcflag( PCFLAG_ONMAP ) || ch->has_actflag( ACT_ONMAP ) )
   {
      int newx = ch->map_x;
      int newy = ch->map_y;

      if( ch->inflight )
      {
         ch->print( "Sit still! You cannot go anywhere until the skyship has landed.\r\n" );
         return rSTOP;
      }

      if( ch->in_room->flags.test( ROOM_WATCHTOWER ) )
      {
         if( direction != DIR_DOWN )
         {
            ch->print( "Alas, you cannot go that way.\r\n" );
            return rSTOP;
         }

         pexit = ch->in_room->get_exit( DIR_DOWN );

         if( !pexit || !pexit->to_room )
         {
            log_printf( "Broken Watchtower exit in room %d!", ch->in_room->vnum );
            ch->printf( "Ooops! The watchtower here is broken. Please contact the immortals. You are in room %d.\r\n", ch->in_room->vnum );
            return rSTOP;
         }

         leave_map( ch, nullptr, pexit->to_room );
         return rSTOP;
      }

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

      retcode = process_exit( ch, newx, newy, direction, running );
      return retcode;
   }

   if( !ch->isnpc(  ) )
   {
      if( ch->IS_DRUNK( 2 ) && ( ch->position != POS_SHOVE ) && ( ch->position != POS_DRAG ) )
         drunk = true;
   }

   if( drunk && !fall )
   {
      door = number_door(  );
      pexit = ch->in_room->get_exit( door );
   }

   in_room = ch->in_room;
   from_room = in_room;
   if( !pexit || ( to_room = pexit->to_room ) == nullptr )
   {
      if( drunk && ch->position != POS_MOUNTED
          && ch->in_room->sector_type != SECT_WATER_SWIM
          && ch->in_room->sector_type != SECT_WATER_NOSWIM
          && ch->in_room->sector_type != SECT_RIVER && ch->in_room->sector_type != SECT_UNDERWATER && ch->in_room->sector_type != SECT_OCEANFLOOR )
      {
         switch ( number_bits( 4 ) )
         {
            default:
               act( AT_ACTION, "You drunkenly stumble into some obstacle.", ch, nullptr, nullptr, TO_CHAR );
               act( AT_ACTION, "$n drunkenly stumbles into a nearby obstacle.", ch, nullptr, nullptr, TO_ROOM );
               break;

            case 3:
               act( AT_ACTION, "In your drunken stupor you trip over your own feet and tumble to the ground.", ch, nullptr, nullptr, TO_CHAR );
               act( AT_ACTION, "$n stumbles drunkenly, trips and tumbles to the ground.", ch, nullptr, nullptr, TO_ROOM );
               ch->position = POS_RESTING;
               break;

            case 4:
               act( AT_SOCIAL, "You utter a string of slurred obscenities.", ch, nullptr, nullptr, TO_CHAR );
               act( AT_ACTION, "Something blurry and immovable has intercepted you as you stagger along.", ch, nullptr, nullptr, TO_CHAR );
               act( AT_HURT, "Oh geez... THAT really hurt.  Everything slowly goes dark and numb...", ch, nullptr, nullptr, TO_CHAR );
               act( AT_ACTION, "$n drunkenly staggers into something.", ch, nullptr, nullptr, TO_ROOM );
               act( AT_SOCIAL, "$n utters a string of slurred obscenities: @*&^%@*&!", ch, nullptr, nullptr, TO_ROOM );
               act( AT_ACTION, "$n topples to the ground with a thud.", ch, nullptr, nullptr, TO_ROOM );
               ch->position = POS_INCAP;
               break;
         }
      }
      else if( drunk )
         act( AT_ACTION, "You stare around trying to make sense of things through your drunken stupor.", ch, nullptr, nullptr, TO_CHAR );
      else
         ch->print( "Alas, you cannot go that way.\r\n" );

      check_sneaks( ch );
      return rSTOP;
   }

   door = pexit->vdir;

   if( pexit->to_room->sector_type == SECT_TREE && !ch->has_aflag( AFF_TREE_TRAVEL ) )
   {
      ch->print( "The forest is too thick for you to pass through that way.\r\n" );
      check_sneaks( ch );
      return rSTOP;
   }

   /*
    * Exit is only a "window", there is no way to travel in that direction
    * unless it's a door with a window in it    -Thoric
    */
   if( IS_EXIT_FLAG( pexit, EX_WINDOW ) && !IS_EXIT_FLAG( pexit, EX_ISDOOR ) )
   {
      ch->print( "There is a window blocking the way.\r\n" );
      check_sneaks( ch );
      return rSTOP;
   }

   /*
    * Keeps people from walking through walls 
    */
   if( ( IS_EXIT_FLAG( pexit, EX_FORTIFIED )
         || IS_EXIT_FLAG( pexit, EX_HEAVY )
         || IS_EXIT_FLAG( pexit, EX_MEDIUM )
         || IS_EXIT_FLAG( pexit, EX_LIGHT ) || IS_EXIT_FLAG( pexit, EX_CRUMBLING ) ) && ( ch->isnpc(  ) || !ch->has_pcflag( PCFLAG_PASSDOOR ) ) )
   {
      act( AT_PLAIN, "There is a $d blocking the way.", ch, nullptr, pexit->keyword, TO_CHAR );
      check_sneaks( ch );
      return rSTOP;
   }

   /*
    * Overland Map stuff - Samson 7-31-99 
    */
   /*
    * Upgraded 4-28-00 to allow mounts and charmies to follow PC - Samson 
    */
   if( IS_EXIT_FLAG( pexit, EX_OVERLAND ) )
   {
      if( !valid_coordinates( pexit->map_x, pexit->map_y ) )
      {
         log_printf( "%s: Room #%d - Invalid exit coordinates: %d %d", __func__, in_room->vnum, pexit->map_x, pexit->map_y );
         ch->print( "Oops. Something is wrong with this map exit - notify the immortals.\r\n" );
         check_sneaks( ch );
         return rSTOP;
      }

      if( !ch->isnpc(  ) )
      {
         enter_map( ch, pexit, pexit->map_x, pexit->map_y, "-1" );

         size_t chars = from_room->people.size(  );
         size_t count = 0;
         for( auto ich = from_room->people.begin(  ); ( ich != from_room->people.end(  ) ) && ( count < chars ); )
         {
            char_data *fch = *ich;
            ++ich;
            ++count;

            if( fch != ch  /* loop room bug fix here by Thoric */
                && fch->master == ch && ( fch->position == POS_STANDING || fch->position == POS_MOUNTED ) )
            {
               if( !fch->isnpc(  ) )
               {
                  /*
                   * Added checks so morts don't blindly follow the leader into DT's. 
                   * -- Tarl 16 July 2002 
                   */
                  if( pexit->to_room->flags.test( ROOM_DEATH ) )
                     fch->print( "You stand your ground.\r\n" );
                  else
                  {
                     if( !from_room->get_exit( direction ) )
                     {
                        act( AT_ACTION, "The entrance closes behind $N, preventing you from following!", fch, nullptr, ch, TO_CHAR );
                        continue;
                     }
                     act( AT_ACTION, "You follow $N.", fch, nullptr, ch, TO_CHAR );
                     move_char( fch, pexit, 0, direction, running );
                  }
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

            size_t chars = from_room->people.size(  );
            size_t count = 0;
            for( auto ich = from_room->people.begin(  ); ( ich != from_room->people.end( ) ) && ( count < chars ); )
            {
               char_data *fch = *ich;
               ++ich;
               ++count;

               if( fch != ch  /* loop room bug fix here by Thoric */
                   && fch->master == ch && ( fch->position == POS_STANDING || fch->position == POS_MOUNTED ) )
               {
                  if( !fch->isnpc(  ) )
                  {
                     /*
                      * Added checks so morts don't blindly follow the leader into DT's. 
                      * -- Tarl 16 July 2002 
                      */
                     if( pexit->to_room->flags.test( ROOM_DEATH ) )
                        fch->print( "You stand your ground.\r\n" );
                     else
                     {
                        if( !from_room->get_exit( direction ) )
                        {
                           act( AT_ACTION, "The entrance closes behind $N, preventing you from following!", fch, nullptr, ch, TO_CHAR );
                           continue;
                        }
                        act( AT_ACTION, "You follow $N.", fch, nullptr, ch, TO_CHAR );
                        move_char( fch, pexit, 0, direction, running );
                     }
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

   if( IS_EXIT_FLAG( pexit, EX_PORTAL ) )
   {
      if( ch->isnpc(  ) )
      {
         act( AT_PLAIN, "Mobs can't use portals.", ch, nullptr, nullptr, TO_CHAR );
         check_sneaks( ch );
         return rSTOP;
      }
      else
      {
         if( !ch->has_visited( pexit->to_room->area ) )
         {
            ch->print( "Magic from the portal repulses your attempt to enter!\r\n" );
            check_sneaks( ch );
            return rSTOP;
         }
      }
   }

   if( IS_EXIT_FLAG( pexit, EX_NOMOB ) && ch->isnpc(  ) && !ch->is_pet(  ) )
   {
      act( AT_PLAIN, "Mobs can't enter there.", ch, nullptr, nullptr, TO_CHAR );
      check_sneaks( ch );
      return rSTOP;
   }

   if( to_room->flags.test( ROOM_NO_MOB ) && ch->isnpc(  ) && !ch->is_pet(  ) )
   {
      act( AT_PLAIN, "Mobs can't enter there.", ch, nullptr, nullptr, TO_CHAR );
      check_sneaks( ch );
      return rSTOP;
   }

   if( IS_EXIT_FLAG( pexit, EX_CLOSED )
       && ( !ch->has_aflag( AFF_PASS_DOOR ) || IS_EXIT_FLAG( pexit, EX_NOPASSDOOR ) ) && ( ch->isnpc(  ) || !ch->has_pcflag( PCFLAG_PASSDOOR ) ) )
   {
      if( !IS_EXIT_FLAG( pexit, EX_SECRET ) && !IS_EXIT_FLAG( pexit, EX_DIG ) )
      {
         if( drunk )
         {
            act( AT_PLAIN, "$n runs into the $d in $s drunken state.", ch, nullptr, pexit->keyword, TO_ROOM );
            act( AT_PLAIN, "You run into the $d in your drunken state.", ch, nullptr, pexit->keyword, TO_CHAR );
         }
         else
            act( AT_PLAIN, "The $d is closed.", ch, nullptr, pexit->keyword, TO_CHAR );
      }
      else
      {
         if( drunk )
            ch->print( "You stagger around in your drunken state.\r\n" );
         else
            ch->print( "Alas, you cannot go that way.\r\n" );
      }
      check_sneaks( ch );
      return rSTOP;
   }

   if( !fall && ch->has_aflag( AFF_CHARM ) && ch->master && in_room == ch->master->in_room )
   {
      ch->print( "What? And leave your beloved master?\r\n" );
      check_sneaks( ch );
      return rSTOP;
   }

   if( to_room->is_private(  ) )
   {
      ch->print( "That room is private right now.\r\n" );
      check_sneaks( ch );
      return rSTOP;
   }

   /*
    * Room flag to set TOTAL isolation, for those absolutely private moments - Samson 3-26-01 
    */
   if( to_room->flags.test( ROOM_ISOLATED ) && ch->level < LEVEL_SUPREME )
   {
      ch->print( "Go away! That room has been sealed for privacy!\r\n" );
      check_sneaks( ch );
      return rSTOP;
   }

   if( !ch->is_immortal(  ) && !ch->isnpc(  ) && ch->in_room->area != to_room->area )
   {
      if( ch->level < to_room->area->low_hard_range )
      {
         ch->set_color( AT_TELL );
         switch ( to_room->area->low_hard_range - ch->level )
         {
            case 1:
               ch->print( "A voice in your mind says, 'You are nearly ready to go that way...'\r\n" );
               break;

            case 2:
               ch->print( "A voice in your mind says, 'Soon you shall be ready to travel down this path... soon.'\r\n" );
               break;

            case 3:
               ch->print( "A voice in your mind says, 'You are not ready to go down that path... yet.'\r\n" );
               break;

            default:
               ch->print( "A voice in your mind says, 'You are not ready to go down that path.'\r\n" );
         }
         check_sneaks( ch );
         return rSTOP;
      }
      else if( ch->level > to_room->area->hi_hard_range )
      {
         ch->set_color( AT_TELL );
         ch->print( "A voice in your mind says, 'There is nothing more for you down that path.'\r\n" );
         check_sneaks( ch );
         return rSTOP;
      }
   }

   if( !fall )
   {
      int move = 0;

      if( in_room->sector_type == SECT_AIR || to_room->sector_type == SECT_AIR || IS_EXIT_FLAG( pexit, EX_FLY ) )
      {
         if( ch->mount && !ch->mount->has_aflag( AFF_FLYING ) )
         {
            ch->print( "Your mount can't fly.\r\n" );
            check_sneaks( ch );
            return rSTOP;
         }
         if( !ch->mount && !ch->has_aflag( AFF_FLYING ) )
         {
            ch->print( "You'd need to fly to go there.\r\n" );
            check_sneaks( ch );
            return rSTOP;
         }
      }

      /*
       * Water_swim sector information added by Samson on unknown date 
       */
      if( ( in_room->sector_type == SECT_WATER_SWIM && to_room->sector_type == SECT_WATER_SWIM )
          || ( in_room->sector_type != SECT_WATER_SWIM && to_room->sector_type == SECT_WATER_SWIM ) )
      {
         if( !ch->IS_FLOATING(  ) )
         {
            if( ( ch->mount && !ch->mount->IS_FLOATING(  ) ) || !ch->mount )
            {
               /*
                * Look for a boat.
                */
               if( get_objtype( ch, ITEM_BOAT ) != nullptr )
               {
                  if( drunk )
                     txt = "paddles unevenly";
                  else
                     txt = "paddles";
               }
               else
               {
                  if( ch->mount )
                     ch->print( "Your mount would drown!\r\n" );
                  else if( !ch->isnpc(  ) && number_percent(  ) > ch->pcdata->learned[gsn_swim] && ch->pcdata->learned[gsn_swim] > 0 )
                  {
                     ch->print( "Your swimming skills need improvement first.\r\n" );
                     ch->learn_from_failure( gsn_swim );
                  }
                  else
                     ch->print( "You'd need a boat to go there.\r\n" );
                  check_sneaks( ch );
                  return rSTOP;
               }
            }
         }
      }

      /*
       * River sector information added by Samson on unknown date 
       */
      if( ( in_room->sector_type == SECT_RIVER && to_room->sector_type == SECT_RIVER ) || ( in_room->sector_type != SECT_RIVER && to_room->sector_type == SECT_RIVER ) )
      {
         if( !ch->IS_FLOATING(  ) )
         {
            if( ( ch->mount && !ch->mount->IS_FLOATING(  ) ) || !ch->mount )
            {
               /*
                * Look for a boat.
                */
               if( get_objtype( ch, ITEM_BOAT ) != nullptr )
               {
                  if( drunk )
                     txt = "paddles unevenly";
                  else
                     txt = "paddles";
               }
               else
               {
                  if( ch->mount )
                     ch->print( "Your mount would drown!\r\n" );
                  else
                     ch->print( "You'd need a boat to go there.\r\n" );
                  check_sneaks( ch );
                  return rSTOP;
               }
            }
         }
      }

      /*
       * Water_noswim sector information fixed by Samson on unknown date 
       */
      if( ( in_room->sector_type == SECT_WATER_NOSWIM && to_room->sector_type == SECT_WATER_NOSWIM )
          || ( in_room->sector_type != SECT_WATER_NOSWIM && to_room->sector_type == SECT_WATER_NOSWIM ) )
      {
         if( !ch->IS_FLOATING(  ) )
         {
            if( ( ch->mount && !ch->mount->IS_FLOATING(  ) ) || !ch->mount )
            {
               /*
                * Look for a boat.
                */
               if( get_objtype( ch, ITEM_BOAT ) != nullptr )
               {
                  if( drunk )
                     txt = "paddles unevenly";
                  else
                     txt = "paddles";
               }
               else
               {
                  if( ch->mount )
                     ch->print( "Your mount would drown!\r\n" );
                  else
                     ch->print( "You'd need a boat to go there.\r\n" );
                  check_sneaks( ch );
                  return rSTOP;
               }
            }
         }
      }

      if( IS_EXIT_FLAG( pexit, EX_CLIMB ) )
      {
         bool found = false;

         if( ch->mount && ch->mount->has_aflag( AFF_FLYING ) )
            found = true;
         else if( ch->has_aflag( AFF_FLYING ) )
            found = true;

         if( !found && !ch->mount )
         {
            if( ( !ch->isnpc(  ) && number_percent(  ) > ch->LEARNED( gsn_climb ) ) || drunk || ch->mental_state < -90 )
            {
               ch->print( "You start to climb... but lose your grip and fall!\r\n" );
               ch->learn_from_failure( gsn_climb );
               if( pexit->vdir == DIR_DOWN )
               {
                  retcode = move_char( ch, pexit, 1, DIR_DOWN, false );
                  return retcode;
               }
               ch->print( "&[hurt]OUCH! You hit the ground!\r\n" );
               ch->WAIT_STATE( 20 );
               retcode = damage( ch, ch, ( pexit->vdir == DIR_UP ? 10 : 5 ), TYPE_UNDEFINED );
               return retcode;
            }
            found = true;
            ch->WAIT_STATE( skill_table[gsn_climb]->beats );
            txt = "climbs";
         }

         if( !found )
         {
            ch->print( "You can't climb.\r\n" );
            check_sneaks( ch );
            return rSTOP;
         }
      }

      if( ch->mount )
      {
         switch ( ch->mount->position )
         {
            case POS_DEAD:
               ch->print( "Your mount is dead!\r\n" );
               check_sneaks( ch );
               return rSTOP;

            case POS_MORTAL:
            case POS_INCAP:
               ch->print( "Your mount is hurt far too badly to move.\r\n" );
               check_sneaks( ch );
               return rSTOP;

            case POS_STUNNED:
               ch->print( "Your mount is too stunned to do that.\r\n" );
               check_sneaks( ch );
               return rSTOP;

            case POS_SLEEPING:
               ch->print( "Your mount is sleeping.\r\n" );
               check_sneaks( ch );
               return rSTOP;

            case POS_RESTING:
               ch->print( "Your mount is resting.\r\n" );
               check_sneaks( ch );
               return rSTOP;

            case POS_SITTING:
               ch->print( "Your mount is sitting down.\r\n" );
               check_sneaks( ch );
               return rSTOP;

            default:
               break;
         }

         if( !ch->mount->IS_FLOATING(  ) )
            move = sect_show[in_room->sector_type].move;
         else
            move = 1;
         if( ch->mount->move < move )
         {
            ch->print( "Your mount is too exhausted.\r\n" );
            check_sneaks( ch );
            return rSTOP;
         }
      }
      else
      {
         if( !ch->IS_FLOATING(  ) )
            move = encumbrance( ch, sect_show[in_room->sector_type].move );
         else
            move = 1;
         if( ch->move < move )
         {
            ch->print( "You are too exhausted.\r\n" );
            check_sneaks( ch );
            return rSTOP;
         }
      }

      if( !ch->is_immortal(  ) )
         ch->WAIT_STATE( move );
      if( ch->mount )
         ch->mount->move -= move;
      else
         ch->move -= move;
   }

   /*
    * Check if player can fit in the room
    */
   if( to_room->tunnel > 0 )
   {
      list < char_data * >::iterator ich;
      int count = ch->mount ? 1 : 0;

      for( ich = to_room->people.begin(  ); ich != to_room->people.end(  ); ++ich )
         if( ++count >= to_room->tunnel )
         {
            if( ch->mount && count == to_room->tunnel )
               ch->print( "There is no room for both you and your mount there.\r\n" );
            else
               ch->print( "There is no room for you there.\r\n" );
            check_sneaks( ch );
            return rSTOP;
         }
   }

   /*
    * check for traps on exit - later 
    */
   if( !ch->has_aflag( AFF_SNEAK ) && !ch->has_pcflag( PCFLAG_WIZINVIS ) )
   {
      if( fall )
         txt = "falls";
      else if( !txt )
      {
         if( ch->mount )
         {
            if( ch->mount->has_aflag( AFF_FLOATING ) )
               txt = "floats";
            else if( ch->mount->has_aflag( AFF_FLYING ) )
               txt = "flies";
            else
               txt = "rides";
         }
         else
         {
            if( ch->has_aflag( AFF_FLOATING ) )
            {
               if( drunk )
                  txt = "floats unsteadily";
               else
                  txt = "floats";
            }
            else if( ch->has_aflag( AFF_FLYING ) )
            {
               if( drunk )
                  txt = "flies shakily";
               else
                  txt = "flies";
            }
            else if( ch->position == POS_SHOVE )
               txt = "is shoved";
            else if( ch->position == POS_DRAG )
               txt = "is dragged";
            else
            {
               if( drunk )
                  txt = "stumbles drunkenly";
               else
                  txt = "leaves";
            }
         }
      }
      if( !running )
      {
         if( ch->mount )
            act_printf( AT_ACTION, ch, nullptr, ch->mount, TO_NOTVICT, "$n %s %s upon $N.", txt, dir_name[door] );
         else
            act_printf( AT_ACTION, ch, nullptr, dir_name[door], TO_ROOM, "$n %s $T.", txt );
      }
   }

   rprog_leave_trigger( ch );
   if( ch->char_died(  ) )
      return global_retcode;

   ch->from_room(  );
   if( !ch->to_room( to_room ) )
      log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __func__, __LINE__ );
   check_sneaks( ch );
   if( ch->mount )
   {
      rprog_leave_trigger( ch->mount );
      if( ch->mount->char_died(  ) )
         return global_retcode;
      if( ch->mount )
      {
         ch->mount->from_room(  );
         if( !ch->mount->to_room( to_room ) )
            log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __func__, __LINE__ );
      }
   }

   if( !ch->has_aflag( AFF_SNEAK ) && ( ch->isnpc(  ) || !ch->has_pcflag( PCFLAG_WIZINVIS ) ) )
   {
      if( fall )
         txt = "falls";
      else if( ch->mount )
      {
         if( ch->mount->has_aflag( AFF_FLOATING ) )
            txt = "floats in";
         else if( ch->mount->has_aflag( AFF_FLYING ) )
            txt = "flies in";
         else
            txt = "rides in";
      }
      else
      {
         if( ch->has_aflag( AFF_FLOATING ) )
         {
            if( drunk )
               txt = "floats in unsteadily";
            else
               txt = "floats in";
         }
         else if( ch->has_aflag( AFF_FLYING ) )
         {
            if( drunk )
               txt = "flies in shakily";
            else
               txt = "flies in";
         }
         else if( ch->position == POS_SHOVE )
            txt = "is shoved in";
         else if( ch->position == POS_DRAG )
            txt = "is dragged in";
         else
         {
            if( drunk )
               txt = "stumbles drunkenly in";
            else
               txt = "arrives";
         }
      }
      dtxt = rev_exit( door );
      if( !running )
      {
         if( ch->mount )
            act_printf( AT_ACTION, ch, nullptr, ch->mount, TO_ROOM, "$n %s from %s upon $N.", txt, dtxt );
         else
            act_printf( AT_ACTION, ch, nullptr, nullptr, TO_ROOM, "$n %s from %s.", txt, dtxt );
      }
   }

   /*
    * Make sure everyone sees the room description of death traps. 
    */
   if( ch->in_room->flags.test( ROOM_DEATH ) && !ch->is_immortal(  ) )
   {
      if( ch->has_pcflag( PCFLAG_BRIEF ) )
         brief = true;
      ch->unset_pcflag( PCFLAG_BRIEF );
   }

   /*
    * BIG ugly looping problem here when the character is mptransed back
    * to the starting room.  To avoid this, check how many chars are in 
    * the room at the start and stop processing followers after doing
    * the right number of them.  -- Narn
    */
   if( !fall )
   {
      size_t chars = from_room->people.size(  );
      size_t count = 0;
      for( auto ich = from_room->people.begin(  ); ( ich != from_room->people.end( ) ) && ( count < chars ); )
      {
         char_data *fch = *ich;
         ++ich;
         ++count;

         if( fch != ch  /* loop room bug fix here by Thoric */
             && fch->master == ch && ( fch->position == POS_STANDING || fch->position == POS_MOUNTED ) )
         {
            if( !running )
            {
               /*
                * Added checks so morts don't blindly follow the leader into DT's.
                * -- Tarl 16 July 2002 
                */
               if( pexit->to_room->flags.test( ROOM_DEATH ) )
                  fch->print( "You stand your ground.\r\n" );
               else
                  act( AT_ACTION, "You follow $N.", fch, nullptr, ch, TO_CHAR );
            }
            if( pexit->to_room->flags.test( ROOM_DEATH ) )
               fch->print( "You decide to wait here.\r\n" );
            else
            {
               if( !from_room->get_exit( direction ) )
               {
                  act( AT_ACTION, "The entrance closes behind $N, preventing you from following!", fch, nullptr, ch, TO_CHAR );
                  continue;
               }
               move_char( fch, pexit, 0, direction, running );
            }
         }
      }
   }

   if( !running )
      interpret( ch, "look" );

   if( brief )
      ch->set_pcflag( PCFLAG_BRIEF );

   /*
    * Put good-old EQ-munching death traps back in!      -Thoric
    */
   if( ch->in_room->flags.test( ROOM_DEATH ) && !ch->is_immortal(  ) )
   {
      act( AT_DEAD, "$n falls prey to a terrible death!", ch, nullptr, nullptr, TO_ROOM );
      ch->print( "&[dead]Oopsie... you're dead!\r\n" );
      log_printf_plus( LOG_INFO, LEVEL_IMMORTAL, "%s hit a DEATH TRAP in room %d!", ch->name, ch->in_room->vnum );
      if( ch->isnpc(  ) )
         ch->extract( true );
      else
         ch->extract( false );
      return rCHAR_DIED;
   }

   /*
    * Do damage to PC for the hostile sector types - Samson 6-1-00 ( Where the hell does time go?!?!? ) 
    */
   if( ch->in_room->sector_type == SECT_LAVA )
   {
      ch->set_color( AT_FIRE );

      if( ch->has_immune( RIS_FIRE ) )
      {
         ch->print( "The lava beneath your feet burns hot, but does you no harm.\r\n" );
         retcode = damage( ch, ch, 0, TYPE_UNDEFINED );
         return retcode;
      }

      if( ch->has_resist( RIS_FIRE ) && !ch->has_immune( RIS_FIRE ) )
      {
         ch->print( "The lava beneath your feet burns hot, but you are partially protected from it.\r\n" );
         retcode = damage( ch, ch, 10, TYPE_UNDEFINED );
         return retcode;
      }

      if( !ch->has_immune( RIS_FIRE ) && !ch->has_resist( RIS_FIRE ) )
      {
         ch->print( "The lava beneath your feet burns you!!\r\n" );
         retcode = damage( ch, ch, 20, TYPE_UNDEFINED );
         return retcode;
      }
   }

   if( ch->in_room->sector_type == SECT_TUNDRA )
   {
      ch->set_color( AT_WHITE );

      if( ch->has_immune( RIS_COLD ) )
      {
         ch->print( "The air is freezing cold, but does you no harm.\r\n" );
         retcode = damage( ch, ch, 0, TYPE_UNDEFINED );
         return retcode;
      }

      if( ch->has_resist( RIS_COLD ) && !ch->has_immune( RIS_COLD ) )
      {
         ch->print( "The icy chill bites deep, but you are partially protected from it.\r\n" );
         retcode = damage( ch, ch, 10, TYPE_UNDEFINED );
         return retcode;
      }

      if( !ch->has_immune( RIS_COLD ) && !ch->has_resist( RIS_COLD ) )
      {
         ch->print( "The icy chill of the tundra bites deep!!\r\n" );
         retcode = damage( ch, ch, 20, TYPE_UNDEFINED );
         return retcode;
      }
   }

   if( !ch->in_room->objects.empty(  ) )
      retcode = check_room_for_traps( ch, TRAP_ENTER_ROOM );
   if( retcode != rNONE )
      return retcode;

   if( ch->char_died(  ) )
      return retcode;

   mprog_entry_trigger( ch );
   if( ch->char_died(  ) )
      return retcode;

   rprog_enter_trigger( ch );
   if( ch->char_died(  ) )
      return retcode;

   mprog_greet_trigger( ch );
   if( ch->char_died(  ) )
      return retcode;

   oprog_greet_trigger( ch );
   if( ch->char_died(  ) )
      return retcode;

   if( !will_fall( ch, fall ) && fall > 0 )
   {
      if( !ch->has_aflag( AFF_FLOATING ) || ( ch->mount && !ch->mount->has_aflag( AFF_FLOATING ) ) )
      {
         ch->print( "&[hurt]OUCH! You hit the ground!\r\n" );
         ch->WAIT_STATE( 20 );
         retcode = damage( ch, ch, 20 * fall, TYPE_UNDEFINED );
      }
      else
         ch->print( "&[magic]You lightly float down to the ground.\r\n" );
   }
   return retcode;
}

CMDF( do_north )
{
   move_char( ch, ch->in_room->get_exit( DIR_NORTH ), 0, DIR_NORTH, false );
}

CMDF( do_east )
{
   move_char( ch, ch->in_room->get_exit( DIR_EAST ), 0, DIR_EAST, false );
}

CMDF( do_south )
{
   move_char( ch, ch->in_room->get_exit( DIR_SOUTH ), 0, DIR_SOUTH, false );
}

CMDF( do_west )
{
   move_char( ch, ch->in_room->get_exit( DIR_WEST ), 0, DIR_WEST, false );
}

CMDF( do_up )
{
   move_char( ch, ch->in_room->get_exit( DIR_UP ), 0, DIR_UP, false );
}

CMDF( do_down )
{
   move_char( ch, ch->in_room->get_exit( DIR_DOWN ), 0, DIR_DOWN, false );
}

CMDF( do_northeast )
{
   move_char( ch, ch->in_room->get_exit( DIR_NORTHEAST ), 0, DIR_NORTHEAST, false );
}

CMDF( do_northwest )
{
   move_char( ch, ch->in_room->get_exit( DIR_NORTHWEST ), 0, DIR_NORTHWEST, false );
}

CMDF( do_southeast )
{
   move_char( ch, ch->in_room->get_exit( DIR_SOUTHEAST ), 0, DIR_SOUTHEAST, false );
}

CMDF( do_southwest )
{
   move_char( ch, ch->in_room->get_exit( DIR_SOUTHWEST ), 0, DIR_SOUTHWEST, false );
}

exit_data *find_door( char_data * ch, const string & arg, bool quiet )
{
   exit_data *pexit = nullptr;

   if( arg.empty(  ) )
      return nullptr;

   int door = get_dirnum( arg );
   if( door < 0 || door > MAX_DIR )
   {
      list < exit_data * >::iterator iexit;
      for( iexit = ch->in_room->exits.begin(  ); iexit != ch->in_room->exits.end(  ); ++iexit )
      {
         pexit = *iexit;

         if( ( quiet || IS_EXIT_FLAG( pexit, EX_ISDOOR ) ) && pexit->keyword && hasname( pexit->keyword, arg ) )
            return pexit;
      }
      if( !quiet )
         ch->printf( "You see no %s here.\r\n", arg.c_str(  ) );
      return nullptr;
   }

   if( !( pexit = ch->in_room->get_exit( door ) ) )
   {
      if( !quiet )
         ch->printf( "You see no %s here.\r\n", arg.c_str(  ) );
      return nullptr;
   }

   if( quiet )
      return pexit;

   if( IS_EXIT_FLAG( pexit, EX_SECRET ) )
   {
      ch->printf( "You see no %s here.\r\n", arg.c_str(  ) );
      return nullptr;
   }

   if( !IS_EXIT_FLAG( pexit, EX_ISDOOR ) )
   {
      ch->print( "You can't do that.\r\n" );
      return nullptr;
   }
   return pexit;
}

void set_bexit_flag( exit_data * pexit, int flag )
{
   exit_data *pexit_rev;

   SET_EXIT_FLAG( pexit, flag );
   if( ( pexit_rev = pexit->rexit ) != nullptr && pexit_rev != pexit )
      SET_EXIT_FLAG( pexit_rev, flag );
}

void remove_bexit_flag( exit_data * pexit, int flag )
{
   exit_data *pexit_rev;

   REMOVE_EXIT_FLAG( pexit, flag );
   if( ( pexit_rev = pexit->rexit ) != nullptr && pexit_rev != pexit )
      REMOVE_EXIT_FLAG( pexit_rev, flag );
}

CMDF( do_open )
{
   obj_data *obj;
   exit_data *pexit;
   int door;

   if( argument.empty(  ) )
   {
      ch->print( "Open what?\r\n" );
      return;
   }

   if( ( pexit = find_door( ch, argument, true ) ) != nullptr )
   {
      /*
       * 'open door' 
       */
      exit_data *pexit_rev;

      /*
       * Added by Tarl 11 July 2002 so mobs don't attempt to open doors to nomob rooms. 
       */
      if( ch->isnpc(  ) )
      {
         if( pexit->to_room->flags.test( ROOM_NO_MOB ) )
            return;
      }
      if( IS_EXIT_FLAG( pexit, EX_SECRET ) && pexit->keyword && !hasname( pexit->keyword, argument ) )
      {
         ch->printf( "You see no %s here.\r\n", argument.c_str(  ) );
         return;
      }
      if( !IS_EXIT_FLAG( pexit, EX_ISDOOR ) || IS_EXIT_FLAG( pexit, EX_DIG ) )
      {
         ch->print( "You can't do that.\r\n" );
         return;
      }
      if( !IS_EXIT_FLAG( pexit, EX_CLOSED ) )
      {
         ch->print( "It's already open.\r\n" );
         return;
      }
      if( IS_EXIT_FLAG( pexit, EX_LOCKED ) && IS_EXIT_FLAG( pexit, EX_BOLTED ) )
      {
         ch->print( "The bolt is locked.\r\n" );
         return;
      }
      if( IS_EXIT_FLAG( pexit, EX_BOLTED ) )
      {
         ch->print( "It's bolted shut.\r\n" );
         return;
      }
      if( IS_EXIT_FLAG( pexit, EX_LOCKED ) )
      {
         ch->print( "It's locked.\r\n" );
         return;
      }

      if( !IS_EXIT_FLAG( pexit, EX_SECRET ) || ( pexit->keyword && hasname( pexit->keyword, argument ) ) )
      {
         act( AT_ACTION, "$n opens the $d.", ch, nullptr, pexit->keyword, TO_ROOM );
         act( AT_ACTION, "You open the $d.", ch, nullptr, pexit->keyword, TO_CHAR );
         if( ( pexit_rev = pexit->rexit ) != nullptr && pexit_rev->to_room == ch->in_room && !pexit->to_room->people.empty(  ) )
            act( AT_ACTION, "The $d opens.", ( *pexit->to_room->people.begin(  ) ), nullptr, pexit_rev->keyword, TO_ROOM );
         remove_bexit_flag( pexit, EX_CLOSED );
         if( ( door = pexit->vdir ) >= 0 && door < DIR_SOMEWHERE )
            check_room_for_traps( ch, trap_door[door] );
         return;
      }
   }

   if( ( obj = ch->get_obj_here( argument ) ) != nullptr )
   {
      /*
       * 'open object' 
       */
      if( obj->item_type != ITEM_CONTAINER )
      {
         ch->printf( "%s is not a container.\r\n", capitalize( obj->short_descr ) );
         return;
      }
      if( !IS_SET( obj->value[1], CONT_CLOSED ) )
      {
         ch->printf( "%s is already open.\r\n", capitalize( obj->short_descr ) );
         return;
      }
      if( !IS_SET( obj->value[1], CONT_CLOSEABLE ) )
      {
         ch->printf( "%s cannot be opened or closed.\r\n", capitalize( obj->short_descr ) );
         return;
      }
      if( IS_SET( obj->value[1], CONT_LOCKED ) )
      {
         ch->printf( "%s is locked.\r\n", capitalize( obj->short_descr ) );
         return;
      }
      REMOVE_BIT( obj->value[1], CONT_CLOSED );
      act( AT_ACTION, "You open $p.", ch, obj, nullptr, TO_CHAR );
      act( AT_ACTION, "$n opens $p.", ch, obj, nullptr, TO_ROOM );
      check_for_trap( ch, obj, TRAP_OPEN );
      return;
   }
   ch->printf( "You see no %s here.\r\n", argument.c_str(  ) );
}

CMDF( do_close )
{
   obj_data *obj;
   exit_data *pexit;
   int door;

   if( argument.empty(  ) )
   {
      ch->print( "Close what?\r\n" );
      return;
   }

   if( ( pexit = find_door( ch, argument, true ) ) != nullptr )
   {
      /*
       * 'close door' 
       */
      exit_data *pexit_rev;

      if( IS_EXIT_FLAG( pexit, EX_SECRET ) && pexit->keyword && !hasname( pexit->keyword, argument ) )
      {
         ch->printf( "You see no %s here.\r\n", argument.c_str(  ) );
         return;
      }
      if( !IS_EXIT_FLAG( pexit, EX_ISDOOR ) )
      {
         ch->print( "You can't do that.\r\n" );
         return;
      }
      if( IS_EXIT_FLAG( pexit, EX_CLOSED ) )
      {
         ch->print( "It's already closed.\r\n" );
         return;
      }
      act( AT_ACTION, "$n closes the $d.", ch, nullptr, pexit->keyword, TO_ROOM );
      act( AT_ACTION, "You close the $d.", ch, nullptr, pexit->keyword, TO_CHAR );

      /*
       * close the other side 
       */
      if( ( pexit_rev = pexit->rexit ) != nullptr && pexit_rev->to_room == ch->in_room )
      {
         SET_EXIT_FLAG( pexit_rev, EX_CLOSED );
         if( !pexit->to_room->people.empty(  ) )
            act( AT_ACTION, "The $d closes.", ( *pexit->to_room->people.begin(  ) ), nullptr, pexit_rev->keyword, TO_ROOM );
      }
      set_bexit_flag( pexit, EX_CLOSED );
      if( ( door = pexit->vdir ) >= 0 && door < 10 )
         check_room_for_traps( ch, trap_door[door] );
      return;
   }

   if( ( obj = ch->get_obj_here( argument ) ) != nullptr )
   {
      /*
       * 'close object' 
       */
      if( obj->item_type != ITEM_CONTAINER )
      {
         ch->printf( "%s is not a container.\r\n", capitalize( obj->short_descr ) );
         return;
      }
      if( IS_SET( obj->value[1], CONT_CLOSED ) )
      {
         ch->printf( "%s is already closed.\r\n", capitalize( obj->short_descr ) );
         return;
      }
      if( !IS_SET( obj->value[1], CONT_CLOSEABLE ) )
      {
         ch->printf( "%s cannot be opened or closed.\r\n", capitalize( obj->short_descr ) );
         return;
      }
      SET_BIT( obj->value[1], CONT_CLOSED );
      act( AT_ACTION, "You close $p.", ch, obj, nullptr, TO_CHAR );
      act( AT_ACTION, "$n closes $p.", ch, obj, nullptr, TO_ROOM );
      check_for_trap( ch, obj, TRAP_CLOSE );
      return;
   }
   ch->printf( "You see no %s here.\r\n", argument.c_str(  ) );
}

/*
 * Keyring support added by Thoric
 * Idea suggested by Onyx <MtRicmer@worldnet.att.net> of Eldarion
 *
 * New: returns pointer to key/nullptr instead of true/false
 *
 * If you want a feature like having immortals always have a key... you'll
 * need to code in a generic key, and make sure extract_obj doesn't extract it
 */
obj_data *has_key( char_data * ch, int key )
{
   list < obj_data * >::iterator iobj;

   for( iobj = ch->carrying.begin(  ); iobj != ch->carrying.end(  ); ++iobj )
   {
      obj_data *obj = *iobj;

      if( obj->pIndexData->vnum == key || ( obj->item_type == ITEM_KEY && obj->value[0] == key ) )
         return obj;
      else if( obj->item_type == ITEM_KEYRING )
      {
         list < obj_data * >::iterator iobj2;

         for( iobj2 = obj->contents.begin(  ); iobj2 != obj->contents.end(  ); ++iobj2 )
         {
            obj_data *obj2 = *iobj2;
            if( obj2->pIndexData->vnum == key || obj2->value[0] == key )
               return obj2;
         }
      }
   }
   return nullptr;
}

CMDF( do_lock )
{
   obj_data *obj, *key;
   exit_data *pexit;
   int count;

   if( argument.empty(  ) )
   {
      ch->print( "Lock what?\r\n" );
      return;
   }

   if( ( pexit = find_door( ch, argument, true ) ) != nullptr )
   {
      /*
       * 'lock door' 
       */
      if( IS_EXIT_FLAG( pexit, EX_SECRET ) && pexit->keyword && !hasname( pexit->keyword, argument ) )
      {
         ch->printf( "You see no %s here.\r\n", argument.c_str(  ) );
         return;
      }
      if( !IS_EXIT_FLAG( pexit, EX_ISDOOR ) )
      {
         ch->print( "You can't do that.\r\n" );
         return;
      }
      if( !IS_EXIT_FLAG( pexit, EX_CLOSED ) )
      {
         ch->print( "It's not closed.\r\n" );
         return;
      }
      if( pexit->key < 0 )
      {
         ch->print( "It can't be locked.\r\n" );
         return;
      }
      if( ( key = has_key( ch, pexit->key ) ) == nullptr )
      {
         ch->print( "You lack the key.\r\n" );
         return;
      }
      if( IS_EXIT_FLAG( pexit, EX_LOCKED ) )
      {
         ch->print( "It's already locked.\r\n" );
         return;
      }
      if( !IS_EXIT_FLAG( pexit, EX_SECRET ) || ( pexit->keyword && hasname( pexit->keyword, argument ) ) )
      {
         ch->print( "*Click*\r\n" );
         count = key->count;
         key->count = 1;
         act( AT_ACTION, "$n locks the $d with $p.", ch, key, pexit->keyword, TO_ROOM );
         key->count = count;
         set_bexit_flag( pexit, EX_LOCKED );
         return;
      }
   }
   if( ( obj = ch->get_obj_here( argument ) ) != nullptr )
   {
      /*
       * 'lock object' 
       */
      if( obj->item_type != ITEM_CONTAINER )
      {
         ch->print( "That's not a container.\r\n" );
         return;
      }
      if( !IS_SET( obj->value[1], CONT_CLOSED ) )
      {
         ch->print( "It's not closed.\r\n" );
         return;
      }
      if( obj->value[2] < 0 )
      {
         ch->print( "It can't be locked.\r\n" );
         return;
      }
      if( !( key = has_key( ch, obj->value[2] ) ) )
      {
         ch->print( "You lack the key.\r\n" );
         return;
      }
      if( IS_SET( obj->value[1], CONT_LOCKED ) )
      {
         ch->print( "It's already locked.\r\n" );
         return;
      }
      SET_BIT( obj->value[1], CONT_LOCKED );
      ch->print( "*Click*\r\n" );
      count = key->count;
      key->count = 1;
      act( AT_ACTION, "$n locks $p with $P.", ch, obj, key, TO_ROOM );
      key->count = count;
      return;
   }
   ch->printf( "You see no %s here.\r\n", argument.c_str(  ) );
}

CMDF( do_unlock )
{
   obj_data *obj, *key;
   exit_data *pexit;
   int count;

   if( argument.empty(  ) )
   {
      ch->print( "Unlock what?\r\n" );
      return;
   }

   if( ( pexit = find_door( ch, argument, true ) ) != nullptr )
   {
      /*
       * 'unlock door' 
       */
      if( IS_EXIT_FLAG( pexit, EX_SECRET ) && pexit->keyword && !hasname( pexit->keyword, argument ) )
      {
         ch->printf( "You see no %s here.\r\n", argument.c_str(  ) );
         return;
      }
      if( !IS_EXIT_FLAG( pexit, EX_ISDOOR ) )
      {
         ch->print( "You can't do that.\r\n" );
         return;
      }
      if( !IS_EXIT_FLAG( pexit, EX_CLOSED ) )
      {
         ch->print( "It's not closed.\r\n" );
         return;
      }
      if( pexit->key < 0 )
      {
         ch->print( "It can't be unlocked.\r\n" );
         return;
      }
      if( !( key = has_key( ch, pexit->key ) ) )
      {
         ch->print( "You lack the key.\r\n" );
         return;
      }
      if( !IS_EXIT_FLAG( pexit, EX_LOCKED ) )
      {
         ch->print( "It's already unlocked.\r\n" );
         return;
      }
      if( !IS_EXIT_FLAG( pexit, EX_SECRET ) || ( pexit->keyword && hasname( pexit->keyword, argument ) ) )
      {
         ch->print( "*Click*\r\n" );
         count = key->count;
         key->count = 1;
         act( AT_ACTION, "$n unlocks the $d with $p.", ch, key, pexit->keyword, TO_ROOM );
         key->count = count;
         if( IS_EXIT_FLAG( pexit, EX_EATKEY ) )
         {
            key->separate(  );
            key->extract(  );
         }
         remove_bexit_flag( pexit, EX_LOCKED );
         return;
      }
   }

   if( ( obj = ch->get_obj_here( argument ) ) != nullptr )
   {
      /*
       * 'unlock object' 
       */
      if( obj->item_type != ITEM_CONTAINER )
      {
         ch->print( "That's not a container.\r\n" );
         return;
      }
      if( !IS_SET( obj->value[1], CONT_CLOSED ) )
      {
         ch->print( "It's not closed.\r\n" );
         return;
      }
      if( obj->value[2] < 0 )
      {
         ch->print( "It can't be unlocked.\r\n" );
         return;
      }
      if( !( key = has_key( ch, obj->value[2] ) ) )
      {
         ch->print( "You lack the key.\r\n" );
         return;
      }
      if( !IS_SET( obj->value[1], CONT_LOCKED ) )
      {
         ch->print( "It's already unlocked.\r\n" );
         return;
      }
      REMOVE_BIT( obj->value[1], CONT_LOCKED );
      ch->print( "*Click*\r\n" );
      count = key->count;
      key->count = 1;
      act( AT_ACTION, "$n unlocks $p with $P.", ch, obj, key, TO_ROOM );
      key->count = count;
      if( IS_SET( obj->value[1], CONT_EATKEY ) )
      {
         key->separate(  );
         key->extract(  );
      }
      return;
   }
   ch->printf( "You see no %s here.\r\n", argument.c_str(  ) );
}

/*
 * This function bolts a door. Written by Blackmane
 */
CMDF( do_bolt )
{
   exit_data *pexit;

   if( argument.empty(  ) )
   {
      ch->print( "Bolt what?\r\n" );
      return;
   }

   if( ( pexit = find_door( ch, argument, true ) ) != nullptr )
   {
      if( IS_EXIT_FLAG( pexit, EX_SECRET ) && pexit->keyword && !hasname( pexit->keyword, argument ) )
      {
         ch->printf( "You see no %s here.\r\n", argument.c_str(  ) );
         return;
      }
      if( !IS_EXIT_FLAG( pexit, EX_ISDOOR ) )
      {
         ch->print( "You can't do that.\r\n" );
         return;
      }
      if( !IS_EXIT_FLAG( pexit, EX_CLOSED ) )
      {
         ch->print( "It's not closed.\r\n" );
         return;
      }
      if( !IS_EXIT_FLAG( pexit, EX_ISBOLT ) )
      {
         ch->print( "You don't see a bolt.\r\n" );
         return;
      }
      if( IS_EXIT_FLAG( pexit, EX_BOLTED ) )
      {
         ch->print( "It's already bolted.\r\n" );
         return;
      }
      if( !IS_EXIT_FLAG( pexit, EX_SECRET ) || ( pexit->keyword && hasname( pexit->keyword, argument ) ) )
      {
         ch->print( "*Clunk*\r\n" );
         act( AT_ACTION, "$n bolts the $d.", ch, nullptr, pexit->keyword, TO_ROOM );
         set_bexit_flag( pexit, EX_BOLTED );
         return;
      }
   }
   ch->printf( "You see no %s here.\r\n", argument.c_str(  ) );
}

/*
 * This function unbolts a door.  Written by Blackmane
 */
CMDF( do_unbolt )
{
   exit_data *pexit;

   if( argument.empty(  ) )
   {
      ch->print( "Unbolt what?\r\n" );
      return;
   }

   if( ( pexit = find_door( ch, argument, true ) ) != nullptr )
   {
      if( IS_EXIT_FLAG( pexit, EX_SECRET ) && pexit->keyword && !hasname( pexit->keyword, argument ) )
      {
         ch->printf( "You see no %s here.\r\n", argument.c_str(  ) );
         return;
      }
      if( !IS_EXIT_FLAG( pexit, EX_ISDOOR ) )
      {
         ch->print( "You can't do that.\r\n" );
         return;
      }
      if( !IS_EXIT_FLAG( pexit, EX_CLOSED ) )
      {
         ch->print( "It's not closed.\r\n" );
         return;
      }
      if( !IS_EXIT_FLAG( pexit, EX_ISBOLT ) )
      {
         ch->print( "You don't see a bolt.\r\n" );
         return;
      }
      if( !IS_EXIT_FLAG( pexit, EX_BOLTED ) )
      {
         ch->print( "It's already unbolted.\r\n" );
         return;
      }
      if( !IS_EXIT_FLAG( pexit, EX_SECRET ) || ( pexit->keyword && hasname( pexit->keyword, argument ) ) )
      {
         ch->print( "*Clunk*\r\n" );
         act( AT_ACTION, "$n unbolts the $d.", ch, nullptr, pexit->keyword, TO_ROOM );
         remove_bexit_flag( pexit, EX_BOLTED );
         return;
      }
   }
   ch->printf( "You see no %s here.\r\n", argument.c_str(  ) );
}

CMDF( do_bashdoor )
{
   if( !ch->isnpc(  ) && ch->level < skill_table[gsn_bashdoor]->skill_level[ch->Class] )
   {
      ch->print( "You're not enough of a warrior to bash doors!\r\n" );
      return;
   }

   if( argument.empty(  ) )
   {
      ch->print( "Bash what?\r\n" );
      return;
   }

   if( ch->fighting )
   {
      ch->print( "You can't break off your fight.\r\n" );
      return;
   }

   exit_data *pexit;
   if( ( pexit = find_door( ch, argument, false ) ) != nullptr )
   {
      room_index *to_room;
      exit_data *pexit_rev;
      int bashchance;
      const char *keyword;

      if( !IS_EXIT_FLAG( pexit, EX_CLOSED ) )
      {
         ch->print( "Calm down.  It is already open.\r\n" );
         return;
      }
      ch->WAIT_STATE( skill_table[gsn_bashdoor]->beats );

      if( IS_EXIT_FLAG( pexit, EX_SECRET ) )
         keyword = "wall";
      else
         keyword = pexit->keyword;
      if( !ch->isnpc(  ) )
         bashchance = ch->LEARNED( gsn_bashdoor ) / 2;
      else
         bashchance = 90;
      if( IS_EXIT_FLAG( pexit, EX_LOCKED ) )
         bashchance /= 3;

      if( !IS_EXIT_FLAG( pexit, EX_BASHPROOF ) && ch->move >= 15 && number_percent(  ) < ( bashchance + 4 * ( ch->get_curr_str(  ) - 19 ) ) )
      {
         REMOVE_EXIT_FLAG( pexit, EX_CLOSED );
         if( IS_EXIT_FLAG( pexit, EX_LOCKED ) )
            REMOVE_EXIT_FLAG( pexit, EX_LOCKED );
         SET_EXIT_FLAG( pexit, EX_BASHED );

         act( AT_SKILL, "Crash!  You bashed open the $d!", ch, nullptr, keyword, TO_CHAR );
         act( AT_SKILL, "$n bashes open the $d!", ch, nullptr, keyword, TO_ROOM );

         if( ( to_room = pexit->to_room ) != nullptr && ( pexit_rev = pexit->rexit ) != nullptr && pexit_rev->to_room == ch->in_room )
         {
            REMOVE_EXIT_FLAG( pexit_rev, EX_CLOSED );
            if( IS_EXIT_FLAG( pexit_rev, EX_LOCKED ) )
               REMOVE_EXIT_FLAG( pexit_rev, EX_LOCKED );
            SET_EXIT_FLAG( pexit_rev, EX_BASHED );
            if( !to_room->people.empty(  ) )
               act( AT_SKILL, "The $d crashes open!", ( *to_room->people.begin(  ) ), nullptr, pexit_rev->keyword, TO_ROOM );
         }
         damage( ch, ch, ( ch->max_hit / 20 ), gsn_bashdoor );
      }
      else
      {
         act( AT_SKILL, "WHAAAAM!!!  You bash against the $d, but it doesn't budge.", ch, nullptr, keyword, TO_CHAR );
         act( AT_SKILL, "WHAAAAM!!!  $n bashes against the $d, but it holds strong.", ch, nullptr, keyword, TO_ROOM );
         ch->learn_from_failure( gsn_bashdoor );
         damage( ch, ch, ( ch->max_hit / 20 ) + 10, gsn_bashdoor );
      }
   }
   else
   {
      act( AT_SKILL, "WHAAAAM!!!  You bash against the wall, but it doesn't budge.", ch, nullptr, nullptr, TO_CHAR );
      act( AT_SKILL, "WHAAAAM!!!  $n bashes against the wall, but it holds strong.", ch, nullptr, nullptr, TO_ROOM );
      ch->learn_from_failure( gsn_bashdoor );
      damage( ch, ch, ( ch->max_hit / 20 ) + 10, gsn_bashdoor );
   }
}

/* Orginal furniture taken from Russ Walsh's Rot copyright 1996-1997
   Furniture 1.0 is provided by Xerves
   Allows you to stand/sit/rest/sleep on/at/in objects -- Xerves */
CMDF( do_stand )
{
   obj_data *obj = nullptr;
   int aon = 0;

   if( ch->position > POS_SITTING && ch->position < POS_STANDING )
   {
      ch->print( "Maybe you should finish this fight first?\r\n" );
      return;
   }

   if( ch->position == POS_MOUNTED )
   {
      ch->print( "Try dismounting first.\r\n" );
      return;
   }

   /*
    * okay, now that we know we can sit, find an object to sit on 
    */
   if( !argument.empty(  ) )
   {
      if( !( obj = get_obj_list( ch, argument, ch->in_room->objects ) ) )
      {
         ch->print( "You don't see that here.\r\n" );
         return;
      }
      if( obj->item_type != ITEM_FURNITURE )
      {
         ch->print( "It has to be furniture silly.\r\n" );
         return;
      }
      if( !IS_SET( obj->value[2], STAND_ON ) && !IS_SET( obj->value[2], STAND_IN ) && !IS_SET( obj->value[2], STAND_AT ) )
      {
         ch->print( "You can't stand on that.\r\n" );
         return;
      }
      if( ch->on == obj )
         aon = 1;
      else
         ch->on = obj;
   }

   switch ( ch->position )
   {
      case POS_SLEEPING:
         if( ch->has_aflag( AFF_SLEEP ) )
         {
            ch->print( "You can't wake up!\r\n" );
            return;
         }

         if( obj == nullptr )
         {
            ch->print( "You wake and stand up.\r\n" );
            act( AT_ACTION, "$n wakes and stands up.", ch, nullptr, nullptr, TO_ROOM );
            ch->on = nullptr;
         }
         else if( IS_SET( obj->value[2], STAND_AT ) )
         {
            act( AT_ACTION, "You wake and stand at $p.", ch, obj, nullptr, TO_CHAR );
            act( AT_ACTION, "$n wakes and stands at $p.", ch, obj, nullptr, TO_ROOM );
         }
         else if( IS_SET( obj->value[2], STAND_ON ) )
         {
            act( AT_ACTION, "You wake and stand on $p.", ch, obj, nullptr, TO_CHAR );
            act( AT_ACTION, "$n wakes and stands on $p.", ch, obj, nullptr, TO_ROOM );
         }
         else
         {
            act( AT_ACTION, "You wake and stand in $p.", ch, obj, nullptr, TO_CHAR );
            act( AT_ACTION, "$n wakes and stands in $p.", ch, obj, nullptr, TO_ROOM );
         }
         ch->position = POS_STANDING;
         interpret( ch, "look" );
         break;

      case POS_RESTING:
      case POS_SITTING:
         if( obj == nullptr )
         {
            ch->print( "You stand up.\r\n" );
            act( AT_ACTION, "$n stands up.", ch, nullptr, nullptr, TO_ROOM );
            ch->on = nullptr;
         }
         else if( IS_SET( obj->value[2], STAND_AT ) )
         {
            act( AT_ACTION, "You stand at $p.", ch, obj, nullptr, TO_CHAR );
            act( AT_ACTION, "$n stands at $p.", ch, obj, nullptr, TO_ROOM );
         }
         else if( IS_SET( obj->value[2], STAND_ON ) )
         {
            act( AT_ACTION, "You stand on $p.", ch, obj, nullptr, TO_CHAR );
            act( AT_ACTION, "$n stands on $p.", ch, obj, nullptr, TO_ROOM );
         }
         else
         {
            act( AT_ACTION, "You stand in $p.", ch, obj, nullptr, TO_CHAR );
            act( AT_ACTION, "$n stands on $p.", ch, obj, nullptr, TO_ROOM );
         }
         ch->position = POS_STANDING;
         break;

      default:
      case POS_STANDING:
         if( obj != nullptr && aon != 1 )
         {
            if( IS_SET( obj->value[2], STAND_AT ) )
            {
               act( AT_ACTION, "You stand at $p.", ch, obj, nullptr, TO_CHAR );
               act( AT_ACTION, "$n stands at $p.", ch, obj, nullptr, TO_ROOM );
            }
            else if( IS_SET( obj->value[2], STAND_ON ) )
            {
               act( AT_ACTION, "You stand on $p.", ch, obj, nullptr, TO_CHAR );
               act( AT_ACTION, "$n stands on $p.", ch, obj, nullptr, TO_ROOM );
            }
            else
            {
               act( AT_ACTION, "You stand in $p.", ch, obj, nullptr, TO_CHAR );
               act( AT_ACTION, "$n stands on $p.", ch, obj, nullptr, TO_ROOM );
            }
         }
         else if( aon == 1 )
            act( AT_ACTION, "You are already using $p for furniture.", ch, obj, nullptr, TO_CHAR );
         else if( ch->on != nullptr && obj == nullptr )
         {
            act( AT_ACTION, "You hop off of $p and stand on the ground.", ch, ch->on, nullptr, TO_CHAR );
            act( AT_ACTION, "$n hops off of $p and stands on the ground.", ch, ch->on, nullptr, TO_ROOM );
            ch->on = nullptr;
         }
         else
            ch->print( "You are already standing.\r\n" );
         break;
   }
}

CMDF( do_sit )
{
   obj_data *obj = nullptr;
   int aon = 0;

   if( ch->position > POS_SITTING && ch->position < POS_STANDING )
   {
      ch->print( "Maybe you should finish this fight first?\r\n" );
      return;
   }

   if( ch->position == POS_MOUNTED )
   {
      ch->print( "You are already sitting - on your mount.\r\n" );
      return;
   }

   /*
    * okay, now that we know we can sit, find an object to sit on 
    */
   if( !argument.empty(  ) )
   {
      if( !( obj = get_obj_list( ch, argument, ch->in_room->objects ) ) )
      {
         ch->print( "You don't see that here.\r\n" );
         return;
      }
      if( obj->item_type != ITEM_FURNITURE )
      {
         ch->print( "It has to be furniture silly.\r\n" );
         return;
      }
      if( !IS_SET( obj->value[2], SIT_ON ) && !IS_SET( obj->value[2], SIT_IN ) && !IS_SET( obj->value[2], SIT_AT ) )
      {
         ch->print( "You can't sit on that.\r\n" );
         return;
      }
      if( ch->on == obj )
         aon = 1;
      else
         ch->on = obj;
   }

   switch ( ch->position )
   {
      case POS_SLEEPING:
         if( !obj )
         {
            ch->print( "You wake and sit up.\r\n" );
            act( AT_ACTION, "$n wakes and sits up.", ch, nullptr, nullptr, TO_ROOM );
         }
         else if( IS_SET( obj->value[2], SIT_AT ) )
         {
            act( AT_ACTION, "You wake up and sit at $p.", ch, obj, nullptr, TO_CHAR );
            act( AT_ACTION, "$n wakes and sits at $p.", ch, obj, nullptr, TO_ROOM );
         }
         else if( IS_SET( obj->value[2], SIT_ON ) )
         {
            act( AT_ACTION, "You wake and sit on $p.", ch, obj, nullptr, TO_CHAR );
            act( AT_ACTION, "$n wakes and sits at $p.", ch, obj, nullptr, TO_ROOM );
         }
         else
         {
            act( AT_ACTION, "You wake and sit in $p.", ch, obj, nullptr, TO_CHAR );
            act( AT_ACTION, "$n wakes and sits in $p.", ch, obj, nullptr, TO_ROOM );
         }
         ch->position = POS_SITTING;
         break;

      case POS_RESTING:
         if( !obj )
            ch->print( "You stop resting.\r\n" );
         else if( IS_SET( obj->value[2], SIT_AT ) )
         {
            act( AT_ACTION, "You sit at $p.", ch, obj, nullptr, TO_CHAR );
            act( AT_ACTION, "$n sits at $p.", ch, obj, nullptr, TO_ROOM );
         }
         else if( IS_SET( obj->value[2], SIT_ON ) )
         {
            act( AT_ACTION, "You sit on $p.", ch, obj, nullptr, TO_CHAR );
            act( AT_ACTION, "$n sits on $p.", ch, obj, nullptr, TO_ROOM );
         }
         ch->position = POS_SITTING;
         break;

      case POS_SITTING:
         if( obj != nullptr && aon != 1 )
         {
            if( IS_SET( obj->value[2], SIT_AT ) )
            {
               act( AT_ACTION, "You sit at $p.", ch, obj, nullptr, TO_CHAR );
               act( AT_ACTION, "$n sits at $p.", ch, obj, nullptr, TO_ROOM );
            }
            else if( IS_SET( obj->value[2], STAND_ON ) )
            {
               act( AT_ACTION, "You sit on $p.", ch, obj, nullptr, TO_CHAR );
               act( AT_ACTION, "$n sits on $p.", ch, obj, nullptr, TO_ROOM );
            }
            else
            {
               act( AT_ACTION, "You sit in $p.", ch, obj, nullptr, TO_CHAR );
               act( AT_ACTION, "$n sits on $p.", ch, obj, nullptr, TO_ROOM );
            }
         }
         else if( aon == 1 )
            act( AT_ACTION, "You are already using $p for furniture.", ch, obj, nullptr, TO_CHAR );
         else if( ch->on != nullptr && obj == nullptr )
         {
            act( AT_ACTION, "You hop off of $p and sit on the ground.", ch, ch->on, nullptr, TO_CHAR );
            act( AT_ACTION, "$n hops off of $p and sits on the ground.", ch, ch->on, nullptr, TO_ROOM );
            ch->on = nullptr;
         }
         else
            ch->print( "You are already sitting.\r\n" );
         break;

      default:
      case POS_STANDING:
         if( obj == nullptr )
         {
            ch->print( "You sit down.\r\n" );
            act( AT_ACTION, "$n sits down on the ground.", ch, nullptr, nullptr, TO_ROOM );
         }
         else if( IS_SET( obj->value[2], SIT_AT ) )
         {
            act( AT_ACTION, "You sit down at $p.", ch, obj, nullptr, TO_CHAR );
            act( AT_ACTION, "$n sits down at $p.", ch, obj, nullptr, TO_ROOM );
         }
         else if( IS_SET( obj->value[2], SIT_ON ) )
         {
            act( AT_ACTION, "You sit on $p.", ch, obj, nullptr, TO_CHAR );
            act( AT_ACTION, "$n sits on $p.", ch, obj, nullptr, TO_ROOM );
         }
         else
         {
            act( AT_ACTION, "You sit down in $p.", ch, obj, nullptr, TO_CHAR );
            act( AT_ACTION, "$n sits down in $p.", ch, obj, nullptr, TO_ROOM );
         }
         ch->position = POS_SITTING;
         break;
   }
}

CMDF( do_rest )
{
   obj_data *obj = nullptr;
   int aon = 0;

   if( ch->position > POS_SITTING && ch->position < POS_STANDING )
   {
      ch->print( "Maybe you should finish this fight first?\r\n" );
      return;
   }

   if( ch->position == POS_MOUNTED )
   {
      ch->print( "You are already sitting - on your mount.\r\n" );
      return;
   }

   /*
    * okay, now that we know we can sit, find an object to sit on 
    */
   if( !argument.empty(  ) )
   {
      if( !( obj = get_obj_list( ch, argument, ch->in_room->objects ) ) )
      {
         ch->print( "You don't see that here.\r\n" );
         return;
      }
      if( obj->item_type != ITEM_FURNITURE )
      {
         ch->print( "It has to be furniture silly.\r\n" );
         return;
      }
      if( !IS_SET( obj->value[2], REST_ON ) && !IS_SET( obj->value[2], REST_IN ) && !IS_SET( obj->value[2], REST_AT ) )
      {
         ch->print( "You can't rest on that.\r\n" );
         return;
      }
      if( ch->on == obj )
         aon = 1;
      else
         ch->on = obj;
   }

   switch ( ch->position )
   {
      case POS_SLEEPING:
         if( obj == nullptr )
         {
            ch->print( "You wake up and start resting.\r\n" );
            act( AT_ACTION, "$n wakes up and starts resting.", ch, nullptr, nullptr, TO_ROOM );
         }
         else if( IS_SET( obj->value[2], REST_AT ) )
         {
            act( AT_ACTION, "You wake up and rest at $p.", ch, obj, nullptr, TO_CHAR );
            act( AT_ACTION, "$n wakes up and rests at $p.", ch, obj, nullptr, TO_ROOM );
         }
         else if( IS_SET( obj->value[2], REST_ON ) )
         {
            act( AT_ACTION, "You wake up and rest on $p.", ch, obj, nullptr, TO_CHAR );
            act( AT_ACTION, "$n wakes up and rests on $p.", ch, obj, nullptr, TO_ROOM );
         }
         else
         {
            act( AT_ACTION, "You wake up and rest in $p.", ch, obj, nullptr, TO_CHAR );
            act( AT_ACTION, "$n wakes up and rests in $p.", ch, obj, nullptr, TO_ROOM );
         }
         ch->position = POS_RESTING;
         break;

      case POS_RESTING:
         if( obj != nullptr && aon != 1 )
         {
            if( IS_SET( obj->value[2], REST_AT ) )
            {
               act( AT_ACTION, "You rest at $p.", ch, obj, nullptr, TO_CHAR );
               act( AT_ACTION, "$n rests at $p.", ch, obj, nullptr, TO_ROOM );
            }
            else if( IS_SET( obj->value[2], REST_ON ) )
            {
               act( AT_ACTION, "You rest on $p.", ch, obj, nullptr, TO_CHAR );
               act( AT_ACTION, "$n rests on $p.", ch, obj, nullptr, TO_ROOM );
            }
            else
            {
               act( AT_ACTION, "You rest in $p.", ch, obj, nullptr, TO_CHAR );
               act( AT_ACTION, "$n rests on $p.", ch, obj, nullptr, TO_ROOM );
            }
         }
         else if( aon == 1 )
            act( AT_ACTION, "You are already using $p for furniture.", ch, obj, nullptr, TO_CHAR );
         else if( ch->on != nullptr && obj == nullptr )
         {
            act( AT_ACTION, "You hop off of $p and start resting on the ground.", ch, ch->on, nullptr, TO_CHAR );
            act( AT_ACTION, "$n hops off of $p and starts to rest on the ground.", ch, ch->on, nullptr, TO_ROOM );
            ch->on = nullptr;
         }
         else
            ch->print( "You are already resting.\r\n" );
         break;

      default:
      case POS_STANDING:
         if( obj == nullptr )
         {
            ch->print( "You rest.\r\n" );
            act( AT_ACTION, "$n sits down and rests.", ch, nullptr, nullptr, TO_ROOM );
         }
         else if( IS_SET( obj->value[2], REST_AT ) )
         {
            act( AT_ACTION, "You sit down at $p and rest.", ch, obj, nullptr, TO_CHAR );
            act( AT_ACTION, "$n sits down at $p and rests.", ch, obj, nullptr, TO_ROOM );
         }
         else if( IS_SET( obj->value[2], REST_ON ) )
         {
            act( AT_ACTION, "You sit on $p and rest.", ch, obj, nullptr, TO_CHAR );
            act( AT_ACTION, "$n sits on $p and rests.", ch, obj, nullptr, TO_ROOM );
         }
         else
         {
            act( AT_ACTION, "You rest in $p.", ch, obj, nullptr, TO_CHAR );
            act( AT_ACTION, "$n rests in $p.", ch, obj, nullptr, TO_ROOM );
         }
         ch->position = POS_RESTING;
         break;

      case POS_SITTING:
         if( obj == nullptr )
         {
            ch->print( "You rest.\r\n" );
            act( AT_ACTION, "$n rests.", ch, nullptr, nullptr, TO_ROOM );
         }
         else if( IS_SET( obj->value[2], REST_AT ) )
         {
            act( AT_ACTION, "You rest at $p.", ch, obj, nullptr, TO_CHAR );
            act( AT_ACTION, "$n rests at $p.", ch, obj, nullptr, TO_ROOM );
         }
         else if( IS_SET( obj->value[2], REST_ON ) )
         {
            act( AT_ACTION, "You rest on $p.", ch, obj, nullptr, TO_CHAR );
            act( AT_ACTION, "$n rests on $p.", ch, obj, nullptr, TO_ROOM );
         }
         else
         {
            act( AT_ACTION, "You rest in $p.", ch, obj, nullptr, TO_CHAR );
            act( AT_ACTION, "$n rests in $p.", ch, obj, nullptr, TO_ROOM );
         }
         ch->position = POS_RESTING;
         break;
   }
   rprog_rest_trigger( ch );
}

CMDF( do_sleep )
{
   obj_data *obj = nullptr;
   int aon = 0;

   if( ch->position > POS_SITTING && ch->position < POS_STANDING )
   {
      ch->print( "Maybe you should finish this fight first?\r\n" );
      return;
   }

   if( ch->position == POS_MOUNTED )
   {
      ch->print( "If you wish to go to sleep, get off of your mount first.\r\n" );
      return;
   }

   /*
    * okay, now that we know we can sit, find an object to sit on 
    */
   if( !argument.empty(  ) )
   {
      if( !( obj = get_obj_list( ch, argument, ch->in_room->objects ) ) )
      {
         ch->print( "You don't see that here.\r\n" );
         return;
      }
      if( obj->item_type != ITEM_FURNITURE )
      {
         ch->print( "It has to be furniture silly.\r\n" );
         return;
      }
      if( !IS_SET( obj->value[2], SLEEP_ON ) && !IS_SET( obj->value[2], SLEEP_IN ) && !IS_SET( obj->value[2], SLEEP_AT ) )
      {
         ch->print( "You can't sleep on that.\r\n" );
         return;
      }
      if( ch->on == obj )
         aon = 1;
      else
         ch->on = obj;
   }

   switch ( ch->position )
   {
      case POS_SLEEPING:
         if( obj != nullptr && aon != 1 )
         {
            if( IS_SET( obj->value[2], SLEEP_AT ) )
            {
               act( AT_ACTION, "You sleep at $p.", ch, obj, nullptr, TO_CHAR );
               act( AT_ACTION, "$n sleeps at $p.", ch, obj, nullptr, TO_ROOM );
            }
            else if( IS_SET( obj->value[2], SLEEP_ON ) )
            {
               act( AT_ACTION, "You sleep on $p.", ch, obj, nullptr, TO_CHAR );
               act( AT_ACTION, "$n sleeps on $p.", ch, obj, nullptr, TO_ROOM );
            }
            else
            {
               act( AT_ACTION, "You sleep in $p.", ch, obj, nullptr, TO_CHAR );
               act( AT_ACTION, "$n sleeps on $p.", ch, obj, nullptr, TO_ROOM );
            }
         }
         else if( aon == 1 )
            act( AT_ACTION, "You are already using $p for furniture.", ch, obj, nullptr, TO_CHAR );
         else if( ch->on != nullptr && obj == nullptr )
         {
            act( AT_ACTION, "You hop off of $p and try to sleep on the ground.", ch, ch->on, nullptr, TO_CHAR );
            act( AT_ACTION, "$n hops off of $p and falls quickly asleep on the ground.", ch, ch->on, nullptr, TO_ROOM );
            ch->on = nullptr;
         }
         else
            ch->print( "You are already sleeping.\r\n" );
         break;

      case POS_RESTING:
         if( obj == nullptr )
         {
            ch->print( "You lean your head back more and go to sleep.\r\n" );
            act( AT_ACTION, "$n lies back and falls asleep on the ground.", ch, nullptr, nullptr, TO_ROOM );
         }
         else if( IS_SET( obj->value[2], SLEEP_AT ) )
         {
            act( AT_ACTION, "You sleep at $p.", ch, obj, nullptr, TO_CHAR );
            act( AT_ACTION, "$n sleeps at $p.", ch, obj, nullptr, TO_ROOM );
         }
         else if( IS_SET( obj->value[2], SLEEP_ON ) )
         {
            act( AT_ACTION, "You sleep on $p.", ch, obj, nullptr, TO_CHAR );
            act( AT_ACTION, "$n sleeps on $p.", ch, obj, nullptr, TO_ROOM );
         }
         else
         {
            act( AT_ACTION, "You sleep in $p.", ch, obj, nullptr, TO_CHAR );
            act( AT_ACTION, "$n sleeps in $p.", ch, obj, nullptr, TO_ROOM );
         }
         ch->position = POS_SLEEPING;
         break;

      case POS_SITTING:
         if( obj == nullptr )
         {
            ch->print( "You lay down and go to sleep.\r\n" );
            act( AT_ACTION, "$n lies back and falls asleep on the ground.", ch, nullptr, nullptr, TO_ROOM );
         }
         else if( IS_SET( obj->value[2], SLEEP_AT ) )
         {
            act( AT_ACTION, "You sleep at $p.", ch, obj, nullptr, TO_CHAR );
            act( AT_ACTION, "$n sleeps at $p.", ch, obj, nullptr, TO_ROOM );
         }
         else if( IS_SET( obj->value[2], SLEEP_ON ) )
         {
            act( AT_ACTION, "You sleep on $p.", ch, obj, nullptr, TO_CHAR );
            act( AT_ACTION, "$n sleeps on $p.", ch, obj, nullptr, TO_ROOM );
         }
         else
         {
            act( AT_ACTION, "You sleep in $p.", ch, obj, nullptr, TO_CHAR );
            act( AT_ACTION, "$n sleeps in $p.", ch, obj, nullptr, TO_ROOM );
         }
         ch->position = POS_SLEEPING;
         break;

      default:
      case POS_STANDING:
         if( obj == nullptr )
         {
            ch->print( "You drop down and fall asleep on the ground.\r\n" );
            act( AT_ACTION, "$n drops down and falls asleep on the ground.", ch, nullptr, nullptr, TO_ROOM );
         }
         else if( IS_SET( obj->value[2], SLEEP_AT ) )
         {
            act( AT_ACTION, "You sleep at $p.", ch, obj, nullptr, TO_CHAR );
            act( AT_ACTION, "$n sleeps at $p.", ch, obj, nullptr, TO_ROOM );
         }
         else if( IS_SET( obj->value[2], SLEEP_ON ) )
         {
            act( AT_ACTION, "You sleep on $p.", ch, obj, nullptr, TO_CHAR );
            act( AT_ACTION, "$n sleeps on $p.", ch, obj, nullptr, TO_ROOM );
         }
         else
         {
            act( AT_ACTION, "You sleep down in $p.", ch, obj, nullptr, TO_CHAR );
            act( AT_ACTION, "$n sleeps down in $p.", ch, obj, nullptr, TO_ROOM );
         }
         ch->position = POS_SLEEPING;
         break;
   }
   rprog_sleep_trigger( ch );
}

CMDF( do_wake )
{
   char_data *victim;

   if( argument.empty(  ) )
   {
      interpret( ch, "stand" );
      interpret( ch, "look auto" );
      return;
   }

   if( !ch->IS_AWAKE(  ) )
   {
      ch->print( "You are asleep yourself!\r\n" );
      return;
   }

   if( !( victim = ch->get_char_room( argument ) ) )
   {
      ch->print( "They aren't here.\r\n" );
      return;
   }

   if( victim->IS_AWAKE(  ) )
   {
      act( AT_PLAIN, "$N is already awake.", ch, nullptr, victim, TO_CHAR );
      return;
   }

   if( victim->has_aflag( AFF_SLEEP ) || victim->position < POS_SLEEPING )
   {
      act( AT_PLAIN, "You can't seem to wake $M!", ch, nullptr, victim, TO_CHAR );
      return;
   }
   act( AT_ACTION, "You wake $M.", ch, nullptr, victim, TO_CHAR );
   victim->position = POS_STANDING;
   act( AT_ACTION, "$n wakes you.", ch, nullptr, victim, TO_VICT );
   interpret( victim, "look auto" );
}

/*
 * teleport a character to another room
 */
void teleportch( char_data * ch, room_index * room, bool show )
{
   if( room->is_private(  ) )
      return;

   if( ch->has_aflag( AFF_FLYING ) && ch->in_room->flags.test( ROOM_TELENOFLY ) )
      return;

   act( AT_ACTION, "$n disappears suddenly!", ch, nullptr, nullptr, TO_ROOM );
   ch->from_room(  );
   if( !ch->to_room( room ) )
      log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __func__, __LINE__ );
   act( AT_ACTION, "$n arrives suddenly!", ch, nullptr, nullptr, TO_ROOM );
   if( show )
      interpret( ch, "look" );
   if( ch->in_room->flags.test( ROOM_DEATH ) && !ch->is_immortal(  ) )
   {
      ch->print( "&[dead]Oopsie... you're dead!\r\n" );
      log_printf_plus( LOG_INFO, LEVEL_IMMORTAL, "%s hit a DEATH TRAP in room %d!", ch->name, ch->in_room->vnum );
      if( ch->isnpc(  ) )
         ch->extract( true );
      else
         ch->extract( false );
   }
}

void teleport( char_data * ch, int room, int flags )
{
   room_index *start = ch->in_room, *dest;
   bool show;

   if( !( dest = get_room_index( room ) ) )
   {
      bug( "%s: bad room vnum %d", __func__, room );
      return;
   }

   if( IS_SET( flags, TELE_SHOWDESC ) )
      show = true;
   else
      show = false;
   if( !IS_SET( flags, TELE_TRANSALL ) )
   {
      teleportch( ch, dest, show );
      return;
   }

   /*
    * teleport everybody in the room 
    */
   list < char_data * >::iterator ich;
   for( ich = start->people.begin(  ); ich != start->people.end(  ); )
   {
      char_data *nch = *ich;
      ++ich;

      teleportch( nch, dest, show );
   }

   /*
    * teleport the objects on the ground too 
    */
   if( IS_SET( flags, TELE_TRANSALLPLUS ) )
   {
      list < obj_data * >::iterator iobj;

      for( iobj = start->objects.begin(  ); iobj != start->objects.end(  ); )
      {
         obj_data *obj = *iobj;
         ++iobj;

         obj->from_room(  );
         obj->to_room( dest, nullptr );
      }
   }
}

/*
 * "Climb" in a certain direction. - Thoric
 */
CMDF( do_climb )
{
   exit_data *pexit;

   if( argument.empty(  ) )
   {
      list < exit_data * >::iterator iexit;
      for( iexit = ch->in_room->exits.begin(  ); iexit != ch->in_room->exits.end(  ); ++iexit )
      {
         pexit = *iexit;

         if( IS_EXIT_FLAG( pexit, EX_xCLIMB ) )
         {
            move_char( ch, pexit, 0, pexit->vdir, false );
            return;
         }
      }
      ch->print( "You cannot climb here.\r\n" );
      return;
   }

   if( ( pexit = find_door( ch, argument, true ) ) != nullptr
      && ( IS_EXIT_FLAG( pexit, EX_xCLIMB ) || IS_EXIT_FLAG( pexit, EX_CLIMB ) ) )
   {
      move_char( ch, pexit, 0, pexit->vdir, false );
      return;
   }
   ch->print( "You cannot climb there.\r\n" );
}

/*
 * "enter" something (moves through an exit)			-Thoric
 */
CMDF( do_enter )
{
   exit_data *pexit;

   if( argument.empty(  ) )
   {
      list < exit_data * >::iterator iexit;
      for( iexit = ch->in_room->exits.begin(  ); iexit != ch->in_room->exits.end(  ); ++iexit )
      {
         pexit = *iexit;

         if( IS_EXIT_FLAG( pexit, EX_xENTER ) )
         {
            if( IS_EXIT_FLAG( pexit, EX_PORTAL ) && !ch->has_visited( pexit->to_room->area ) )
            {
               ch->print( "Magic from the portal repulses your attempt to enter!\r\n" );
               return;
            }
            move_char( ch, pexit, 0, DIR_SOMEWHERE, false );
            return;
         }
      }

      if( ch->in_room->sector_type != SECT_INDOORS && ch->IS_OUTSIDE(  ) )
      {
         for( iexit = ch->in_room->exits.begin(  ); iexit != ch->in_room->exits.end(  ); ++iexit )
         {
            pexit = *iexit;

            if( pexit->to_room && ( pexit->to_room->sector_type == SECT_INDOORS || pexit->to_room->flags.test( ROOM_INDOORS ) ) )
            {
               move_char( ch, pexit, 0, DIR_SOMEWHERE, false );
               return;
            }
         }
      }
      ch->print( "You cannot find an entrance here.\r\n" );
      return;
   }

   if( ( pexit = find_door( ch, argument, true ) ) != nullptr && IS_EXIT_FLAG( pexit, EX_xENTER ) )
   {
      if( IS_EXIT_FLAG( pexit, EX_PORTAL ) && !ch->has_visited( pexit->to_room->area ) )
      {
         ch->print( "Magic from the portal repulses your attempt to enter!\r\n" );
         return;
      }
      move_char( ch, pexit, 0, DIR_SOMEWHERE, false );
      return;
   }
   ch->print( "You cannot enter that.\r\n" );
}

/*
 * Leave through an exit.					-Thoric
 */
CMDF( do_leave )
{
   exit_data *pexit;

   if( argument.empty(  ) )
   {
      list < exit_data * >::iterator iexit;
      for( iexit = ch->in_room->exits.begin(  ); iexit != ch->in_room->exits.end(  ); ++iexit )
      {
         pexit = *iexit;

         if( IS_EXIT_FLAG( pexit, EX_xLEAVE ) )
         {
            if( IS_EXIT_FLAG( pexit, EX_PORTAL ) && !ch->has_visited( pexit->to_room->area ) )
            {
               ch->print( "Magic from the portal repulses your attempt to leave!\r\n" );
               return;
            }
            move_char( ch, pexit, 0, DIR_SOMEWHERE, false );
            return;
         }
      }

      if( ch->in_room->sector_type == SECT_INDOORS || !ch->IS_OUTSIDE(  ) )
      {
         for( iexit = ch->in_room->exits.begin(  ); iexit != ch->in_room->exits.end(  ); ++iexit )
         {
            pexit = *iexit;

            if( pexit->to_room && pexit->to_room->sector_type != SECT_INDOORS && !pexit->to_room->flags.test( ROOM_INDOORS ) )
            {
               move_char( ch, pexit, 0, DIR_SOMEWHERE, false );
               return;
            }
         }
      }
      ch->print( "You cannot find an exit here.\r\n" );
      return;
   }

   if( ( pexit = find_door( ch, argument, true ) ) != nullptr && IS_EXIT_FLAG( pexit, EX_xLEAVE ) )
   {
      if( IS_EXIT_FLAG( pexit, EX_PORTAL ) && !ch->has_visited( pexit->to_room->area ) )
      {
         ch->print( "Magic from the portal repulses your attempt to leave!\r\n" );
         return;
      }
      move_char( ch, pexit, 0, DIR_SOMEWHERE, false );
      return;
   }
   ch->print( "You cannot leave that way.\r\n" );
}

/*
 * Check to see if an exit in the room is pulling (or pushing) players around.
 * Some types may cause damage.					-Thoric
 *
 * People kept requesting currents (like SillyMUD has), so I went all out
 * and added the ability for an exit to have a "pull" or a "push" force
 * and to handle different types much beyond a simple water current.
 *
 * This check is called by violence_update().  I'm not sure if this is the
 * best way to do it, or if it should be handled by a special queue.
 *
 * Future additions to this code may include equipment being blown away in
 * the wind (mostly headwear), and people being hit by flying objects
 *
 * TODO:
 *	handle more pulltypes
 *	give "entrance" messages for players and objects
 *	proper handling of player resistance to push/pulling
 */
ch_ret pullcheck( char_data * ch, int pulse )
{
   room_index *room;
   bool move = false, moveobj = true, showroom = true;
   int resistance;
   const char *tochar = nullptr, *toroom = nullptr, *objmsg = nullptr;
   const char *destrm = nullptr, *destob = nullptr, *dtxt = "somewhere";

   if( !( room = ch->in_room ) )
   {
      bug( "%s: %s not in a room?!?", __func__, ch->name );
      return rNONE;
   }

   /*
    * Find the exit with the strongest force (if any) 
    */
   exit_data *xit = nullptr;
   list < exit_data * >::iterator iexit;
   for( iexit = room->exits.begin(  ); iexit != room->exits.end(  ); ++iexit )
   {
      exit_data *pexit = *iexit;

      if( pexit->pull && pexit->to_room && ( !xit || abs( pexit->pull ) > abs( xit->pull ) ) )
         xit = pexit;
   }

   if( !xit )
      return rNONE;

   int pull = xit->pull;

   /*
    * strength also determines frequency 
    */
   int pullfact = URANGE( 1, 20 - ( abs( pull ) / 5 ), 20 );

   /*
    * strongest pull not ready yet... check for one that is 
    */
   if( ( pulse % pullfact ) != 0 )
   {
      bool found = false;
      for( iexit = room->exits.begin(  ); iexit != room->exits.end(  ); ++iexit )
      {
         exit_data *pexit = *iexit;

         if( pexit->pull && pexit->to_room )
         {
            pull = pexit->pull;
            pullfact = URANGE( 1, 20 - ( abs( pull ) / 5 ), 20 );
            if( ( pulse % pullfact ) == 0 )
            {
               found = true;
               break;
            }
         }
      }
      if( found )
         return rNONE;
   }

   /*
    * negative pull = push... get the reverse exit if any 
    */
   if( pull < 0 )
      if( !( xit = room->get_exit( rev_dir[xit->vdir] ) ) )
         return rNONE;

   if( IS_EXIT_FLAG( xit, EX_CLOSED ) )
      return rNONE;

   /*
    * check for tunnel 
    */
   if( xit->to_room->tunnel > 0 )
   {
      list < char_data * >::iterator ich;
      int count = ch->mount ? 1 : 0;

      for( ich = xit->to_room->people.begin(  ); ich != xit->to_room->people.end(  ); ++ich )
      {
         if( ++count >= xit->to_room->tunnel )
            return rNONE;
      }
   }

   dtxt = rev_exit( xit->vdir );

   /*
    * First determine if the player should be moved or not
    * Check various flags, spells, the players position and strength vs.
    * the pull, etc... any kind of checks you like.
    */
   switch ( xit->pulltype )
   {
      case PULL_CURRENT:
      case PULL_WHIRLPOOL:
         switch ( room->sector_type )
         {
               /*
                * allow whirlpool to be in any sector type 
                */
            default:
               if( xit->pulltype == PULL_CURRENT )
                  break;
            case SECT_WATER_SWIM:
            case SECT_WATER_NOSWIM:
            case SECT_RIVER: /* River drift currents added - Samson 8-2-98 */
               if( ( ch->mount && !ch->mount->IS_FLOATING(  ) ) || ( !ch->mount && !ch->IS_FLOATING(  ) ) )
                  move = true;
               break;

            case SECT_UNDERWATER:
            case SECT_OCEANFLOOR:
               move = true;
               break;
         }
         break;
      case PULL_GEYSER:
      case PULL_WAVE:
         move = true;
         break;

      case PULL_WIND:
      case PULL_STORM:
         /*
          * if not flying... check weight, position & strength 
          */
         move = true;
         break;

      case PULL_COLDWIND:
         /*
          * if not flying... check weight, position & strength 
          */
         /*
          * also check for damage due to bitter cold 
          */
         move = true;
         break;

      case PULL_HOTAIR:
         /*
          * if not flying... check weight, position & strength 
          */
         /*
          * also check for damage due to heat 
          */
         move = true;
         break;

         /*
          * light breeze -- very limited moving power 
          */
      case PULL_BREEZE:
         move = false;
         break;

         /*
          * exits with these pulltypes should also be blocked from movement
          * ie: a secret locked pickproof door with the name "_sinkhole_", etc
          */
      case PULL_EARTHQUAKE:
      case PULL_SINKHOLE:
      case PULL_QUICKSAND:
      case PULL_LANDSLIDE:
      case PULL_SLIP:
      case PULL_LAVA:
         if( ( ch->mount && !ch->mount->IS_FLOATING(  ) ) || ( !ch->mount && !ch->IS_FLOATING(  ) ) )
            move = true;
         break;

         /*
          * as if player moved in that direction him/herself 
          */
      case PULL_UNDEFINED:
         return move_char( ch, xit, 0, xit->vdir, false );

         /*
          * all other cases ALWAYS move 
          */
      default:
         move = true;
         break;
   }

   /*
    * assign some nice text messages 
    */
   switch ( xit->pulltype )
   {
      case PULL_MYSTERIOUS:
         /*
          * no messages to anyone 
          */
         showroom = false;
         break;
      case PULL_WHIRLPOOL:
      case PULL_VACUUM:
         tochar = "You are sucked $T!";
         toroom = "$n is sucked $T!";
         destrm = "$n is sucked in from $T!";
         objmsg = "$p is sucked $T.";
         destob = "$p is sucked in from $T!";
         break;
      case PULL_CURRENT:
      case PULL_LAVA:
         tochar = "You drift $T.";
         toroom = "$n drifts $T.";
         destrm = "$n drifts in from $T.";
         objmsg = "$p drifts $T.";
         destob = "$p drifts in from $T.";
         break;
      case PULL_BREEZE:
         tochar = "You drift $T.";
         toroom = "$n drifts $T.";
         destrm = "$n drifts in from $T.";
         objmsg = "$p drifts $T in the breeze.";
         destob = "$p drifts in from $T.";
         break;
      case PULL_GEYSER:
      case PULL_WAVE:
         tochar = "You are pushed $T!";
         toroom = "$n is pushed $T!";
         destrm = "$n is pushed in from $T!";
         destob = "$p floats in from $T.";
         break;
      case PULL_EARTHQUAKE:
         tochar = "The earth opens up and you fall $T!";
         toroom = "The earth opens up and $n falls $T!";
         destrm = "$n falls from $T!";
         objmsg = "$p falls $T.";
         destob = "$p falls from $T.";
         break;
      case PULL_SINKHOLE:
         tochar = "The ground suddenly gives way and you fall $T!";
         toroom = "The ground suddenly gives way beneath $n!";
         destrm = "$n falls from $T!";
         objmsg = "$p falls $T.";
         destob = "$p falls from $T.";
         break;
      case PULL_QUICKSAND:
         tochar = "You begin to sink $T into the quicksand!";
         toroom = "$n begins to sink $T into the quicksand!";
         destrm = "$n sinks in from $T.";
         objmsg = "$p begins to sink $T into the quicksand.";
         destob = "$p sinks in from $T.";
         break;
      case PULL_LANDSLIDE:
         tochar = "The ground starts to slide $T, taking you with it!";
         toroom = "The ground starts to slide $T, taking $n with it!";
         destrm = "$n slides in from $T.";
         objmsg = "$p slides $T.";
         destob = "$p slides in from $T.";
         break;
      case PULL_SLIP:
         tochar = "You lose your footing!";
         toroom = "$n loses $s footing!";
         destrm = "$n slides in from $T.";
         objmsg = "$p slides $T.";
         destob = "$p slides in from $T.";
         break;
      case PULL_VORTEX:
         tochar = "You are sucked into a swirling vortex of colors!";
         toroom = "$n is sucked into a swirling vortex of colors!";
         toroom = "$n appears from a swirling vortex of colors!";
         objmsg = "$p is sucked into a swirling vortex of colors!";
         objmsg = "$p appears from a swirling vortex of colors!";
         break;
      case PULL_HOTAIR:
         tochar = "A blast of hot air blows you $T!";
         toroom = "$n is blown $T by a blast of hot air!";
         destrm = "$n is blown in from $T by a blast of hot air!";
         objmsg = "$p is blown $T.";
         destob = "$p is blown in from $T.";
         break;
      case PULL_COLDWIND:
         tochar = "A bitter cold wind forces you $T!";
         toroom = "$n is forced $T by a bitter cold wind!";
         destrm = "$n is forced in from $T by a bitter cold wind!";
         objmsg = "$p is blown $T.";
         destob = "$p is blown in from $T.";
         break;
      case PULL_WIND:
         tochar = "A strong wind pushes you $T!";
         toroom = "$n is blown $T by a strong wind!";
         destrm = "$n is blown in from $T by a strong wind!";
         objmsg = "$p is blown $T.";
         destob = "$p is blown in from $T.";
         break;
      case PULL_STORM:
         tochar = "The raging storm drives you $T!";
         toroom = "$n is driven $T by the raging storm!";
         destrm = "$n is driven in from $T by a raging storm!";
         objmsg = "$p is blown $T.";
         destob = "$p is blown in from $T.";
         break;
      default:
         if( pull > 0 )
         {
            tochar = "You are pulled $T!";
            toroom = "$n is pulled $T.";
            destrm = "$n is pulled in from $T.";
            objmsg = "$p is pulled $T.";
            objmsg = "$p is pulled in from $T.";
         }
         else
         {
            tochar = "You are pushed $T!";
            toroom = "$n is pushed $T.";
            destrm = "$n is pushed in from $T.";
            objmsg = "$p is pushed $T.";
            objmsg = "$p is pushed in from $T.";
         }
         break;
   }

   /*
    * Do the moving 
    */
   if( move )
   {
      /*
       * display an appropriate exit message 
       */
      if( tochar )
      {
         act( AT_PLAIN, tochar, ch, nullptr, dir_name[xit->vdir], TO_CHAR );
         ch->print( "\r\n" );
      }
      if( toroom )
         act( AT_PLAIN, toroom, ch, nullptr, dir_name[xit->vdir], TO_ROOM );

      /*
       * display an appropriate entrance message 
       */
      if( destrm && !xit->to_room->people.empty(  ) )
         act( AT_PLAIN, destrm, ( *xit->to_room->people.begin(  ) ), nullptr, dtxt, TO_ROOM );

      /*
       * move the char 
       */
      if( xit->pulltype == PULL_SLIP )
         return move_char( ch, xit, 1, xit->vdir, false );
      ch->from_room(  );
      if( !ch->to_room( xit->to_room ) )
         log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __func__, __LINE__ );

      if( showroom )
         interpret( ch, "look" );

      /*
       * move the mount too 
       */
      if( ch->mount )
      {
         ch->mount->from_room(  );
         if( !ch->mount->to_room( xit->to_room ) )
            log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __func__, __LINE__ );
         if( showroom )
            interpret( ch->mount, "look" );
      }
   }

   /*
    * move objects in the room 
    */
   if( moveobj )
   {
      list < obj_data * >::iterator iobj;
      for( iobj = room->objects.begin(  ); iobj != room->objects.end(  ); )
      {
         obj_data *obj = *iobj;
         ++iobj;

         if( obj->extra_flags.test( ITEM_BURIED ) || !obj->wear_flags.test( ITEM_TAKE ) )
            continue;

         resistance = obj->get_weight(  );
         if( obj->extra_flags.test( ITEM_METAL ) )
            resistance = ( resistance * 6 ) / 5;
         switch ( obj->item_type )
         {
            default:
               break;

            case ITEM_SCROLL:
            case ITEM_TRASH:
               resistance >>= 2;
               break;

            case ITEM_SCRAPS:
            case ITEM_CONTAINER:
               resistance >>= 1;
               break;

            case ITEM_PEN:
            case ITEM_WAND:
               resistance = ( resistance * 5 ) / 6;
               break;

            case ITEM_CORPSE_PC:
            case ITEM_CORPSE_NPC:
            case ITEM_FOUNTAIN:
               resistance <<= 2;
               break;
         }

         /*
          * is the pull greater than the resistance of the object? 
          */
         if( ( abs( pull ) * 10 ) > resistance )
         {
            if( objmsg && !room->people.empty(  ) )
               act( AT_PLAIN, objmsg, ( *room->people.begin(  ) ), obj, dir_name[xit->vdir], TO_ROOM );
            if( destob && !xit->to_room->people.empty(  ) )
               act( AT_PLAIN, destob, ( *xit->to_room->people.begin(  ) ), obj, dtxt, TO_ROOM );
            obj->from_room(  );
            obj->to_room( xit->to_room, nullptr );
         }
      }
   }
   return rNONE;
}
