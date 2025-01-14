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
 *                      Online Reset Editing Module                         *
 ****************************************************************************/

/*
 * This file relies heavily on the fact that your linked lists are correct,
 * and that pArea->reset_first is the first reset in pArea.  Likewise,
 * pArea->reset_last *MUST* be the last reset in pArea.  Weird and
 * wonderful things will happen if any of your lists are messed up, none
 * of them good.  The most important are your pRoom->contents,
 * pRoom->people, rch->carrying, obj->contains, and pArea->reset_first ..
 * pArea->reset_last.  -- Altrag
 */
#include "mud.h"
#include "area.h"
#include "mobindex.h"
#include "objindex.h"
#include "overland.h"
#include "roomindex.h"

/* Externals */
extern int top_reset;

bool can_rmodify( char_data *, room_index * );

char *sprint_reset( reset_data * pReset, short &num )
{
   list < reset_data * >::iterator rst;
   static char buf[MSL];
   char mobname[MIL], roomname[MIL], objname[MIL];
   static room_index *room;
   static obj_index *obj, *obj2;
   static mob_index *mob;

   switch ( pReset->command )
   {
      default:
         snprintf( buf, MSL, "%2d) *** BAD RESET: %c %d %d %d %d ***\r\n", num, pReset->command, pReset->arg1, pReset->arg2, pReset->arg3, pReset->arg4 );
         break;

      case 'M':
         mob = get_mob_index( pReset->arg1 );
         room = get_room_index( pReset->arg3 );
         if( mob )
            mudstrlcpy( mobname, mob->player_name, MIL );
         else
            mudstrlcpy( mobname, "Mobile: *BAD VNUM*", MIL );
         if( room )
            mudstrlcpy( roomname, room->name, MIL );
         else
            mudstrlcpy( roomname, "Room: *BAD VNUM*", MIL );
         if( pReset->arg4 != -1 && pReset->arg5 != -1 && pReset->arg6 != -1 )
            snprintf( buf, MSL, "%2d) %s (%d) (%d%%) -> Overland: %s %d %d [%d]\r\n", num, mobname, pReset->arg1,
                      pReset->arg7, room->area->continent->name.c_str( ), pReset->arg5, pReset->arg6, pReset->arg2 );
         else
            snprintf( buf, MSL, "%2d) %s (%d) (%d%%) -> %s Room: %d [%d]\r\n", num, mobname, pReset->arg1, pReset->arg7, roomname, pReset->arg3, pReset->arg2 );

         for( rst = pReset->resets.begin(  ); rst != pReset->resets.end(  ); ++rst )
         {
            reset_data *tReset = *rst;

            ++num;
            switch ( tReset->command )
            {
               default:
                  snprintf( buf + strlen( buf ), MSL - strlen( buf ), "Bad Command: %c", tReset->command );
                  break;

               case 'X':
                  if( !mob )
                     mudstrlcpy( mobname, "* ERROR: NO MOBILE! *", MIL );
                  snprintf( buf + strlen( buf ), MSL - strlen( buf ), "%2d) (equip) <RT> (%d%%) -> %s (%s)\r\n", num, tReset->arg8, mobname, wear_locs[tReset->arg7] );
                  break;

               case 'Y':
                  if( !mob )
                     mudstrlcpy( mobname, "* ERROR: NO MOBILE! *", MIL );
                  snprintf( buf + strlen( buf ), MSL - strlen( buf ), "%2d) (carry) <RT> (%d%%) -> %s\r\n", num, tReset->arg7, mobname );
                  break;

               case 'E':
                  if( !mob )
                     mudstrlcpy( mobname, "* ERROR: NO MOBILE! *", MIL );
                  if( !( obj = get_obj_index( tReset->arg1 ) ) )
                     mudstrlcpy( objname, "Object: *BAD VNUM*", MIL );
                  else
                     mudstrlcpy( objname, obj->name, MIL );
                  snprintf( buf + strlen( buf ), MSL - strlen( buf ), "%2d) (equip) %s (%d) (%d%%) -> %s (%s) [%d]\r\n",
                            num, objname, tReset->arg1, tReset->arg4, mobname, wear_locs[tReset->arg3], tReset->arg2 );
                  break;

               case 'G':
                  if( !mob )
                     mudstrlcpy( mobname, "* ERROR: NO MOBILE! *", MIL );
                  if( !( obj = get_obj_index( tReset->arg1 ) ) )
                     mudstrlcpy( objname, "Object: *BAD VNUM*", MIL );
                  else
                     mudstrlcpy( objname, obj->name, MIL );
                  snprintf( buf + strlen( buf ), MSL - strlen( buf ), "%2d) (carry) %s (%d) (%d%%) -> %s [%d]\r\n",
                            num, objname, tReset->arg1, tReset->arg3, mobname, tReset->arg2 );
                  break;
            }

            if( !tReset->resets.empty(  ) )
            {
               list < reset_data * >::iterator gst;

               for( gst = tReset->resets.begin(  ); gst != tReset->resets.end(  ); ++gst )
               {
                  reset_data *gReset = *gst;

                  ++num;
                  switch ( gReset->command )
                  {
                     default:
                        snprintf( buf + strlen( buf ), MSL - strlen( buf ), "Bad Command: %c", tReset->command );
                        break;

                     case 'P':
                        if( !( obj2 = get_obj_index( gReset->arg2 ) ) )
                           mudstrlcpy( objname, "Object1: *BAD VNUM*", MIL );
                        else
                           mudstrlcpy( objname, obj2->name, MIL );
                        if( gReset->arg4 > 0 && ( obj = get_obj_index( gReset->arg4 ) ) == nullptr )
                           mudstrlcpy( roomname, "Object2: *BAD VNUM*", MIL );
                        else if( !obj )
                           mudstrlcpy( roomname, "Object2: *nullptr obj*", MIL );
                        else
                           mudstrlcpy( roomname, obj->name, MIL );
                        snprintf( buf + strlen( buf ), MSL - strlen( buf ), "%2d) (put) %s (%d) (%d%%) -> %s (%d) [%d]\r\n",
                                  num, objname, gReset->arg2, gReset->arg5, roomname, obj ? obj->vnum : gReset->arg4, gReset->arg3 );
                        break;
                  }
               }
            }
         }
         break;

      case 'Z':
         mudstrlcpy( objname, "<RT>", MIL );
         room = get_room_index( pReset->arg7 );
         if( !room )
            mudstrlcpy( roomname, "Room: *BAD VNUM*", MIL );
         else
            mudstrlcpy( roomname, room->name, MIL );
         if( pReset->arg8 != -1 && pReset->arg9 != -1 && pReset->arg10 != -1 )
            snprintf( buf, MSL, "%2d) (RT object) %s (%d%%) -> Overland: %s %d %d\r\n", num, objname, pReset->arg11, room->area->continent->name.c_str( ), pReset->arg9, pReset->arg10 );
         else
            snprintf( buf, MSL, "%2d) (RT object) %s (%d%%) -> %s Room: %d\r\n", num, objname, pReset->arg11, roomname, pReset->arg7 );
         break;

      case 'O':
         if( !( obj = get_obj_index( pReset->arg1 ) ) )
            mudstrlcpy( objname, "Object: *BAD VNUM*", MIL );
         else
            mudstrlcpy( objname, obj->name, MIL );
         room = get_room_index( pReset->arg3 );
         if( !room )
            mudstrlcpy( roomname, "Room: *BAD VNUM*", MIL );
         else
            mudstrlcpy( roomname, room->name, MIL );
         if( pReset->arg4 != -1 && pReset->arg5 != -1 && pReset->arg6 != -1 )
            snprintf( buf, MSL, "%2d) (object) %s (%d) (%d%%) -> Overland: %s %d %d [%d]\r\n", num, objname, pReset->arg1,
                      pReset->arg7, room->area->continent->name.c_str( ), pReset->arg5, pReset->arg6, pReset->arg2 );
         else
            snprintf( buf, MSL, "%2d) (object) %s (%d) (%d%%) -> %s Room: %d [%d]\r\n", num, objname, pReset->arg1, pReset->arg7, roomname, pReset->arg3, pReset->arg2 );

         for( rst = pReset->resets.begin(  ); rst != pReset->resets.end(  ); ++rst )
         {
            reset_data *tReset = *rst;

            ++num;
            switch ( tReset->command )
            {
               default:
                  snprintf( buf + strlen( buf ), MSL - strlen( buf ), "Bad Command: %c\r\n", tReset->command );
                  break;

               case 'P':
                  if( !( obj2 = get_obj_index( tReset->arg2 ) ) )
                     mudstrlcpy( objname, "Object1: *BAD VNUM*", MIL );
                  else
                     mudstrlcpy( objname, obj2->name, MIL );
                  if( tReset->arg4 > 0 && ( obj = get_obj_index( tReset->arg4 ) ) == nullptr )
                     mudstrlcpy( roomname, "Object2: *BAD VNUM*", MIL );
                  else if( !obj )
                     mudstrlcpy( roomname, "Object2: *nullptr obj*", MIL );
                  else
                     mudstrlcpy( roomname, obj->name, MIL );
                  snprintf( buf + strlen( buf ), MSL - strlen( buf ), "%2d) (put) %s (%d) (%d%%) -> %s (%d) [%d]\r\n",
                            num, objname, tReset->arg2, tReset->arg5, roomname, obj ? obj->vnum : tReset->arg4, tReset->arg3 );
                  break;

               case 'W':
                  mudstrlcpy( objname, "<RT>", MIL );
                  if( tReset->arg8 > 0 && ( obj = get_obj_index( tReset->arg8 ) ) == nullptr )
                     mudstrlcpy( roomname, "Object2: *BAD VNUM*", MIL );
                  else if( !obj )
                     mudstrlcpy( roomname, "Object2: *nullptr obj*", MIL );
                  else
                     mudstrlcpy( roomname, obj->name, MIL );
                  snprintf( buf + strlen( buf ), MSL - strlen( buf ), "%2d) (RT put) %s (%d%%) -> %s (%d)\r\n",
                            num, objname, tReset->arg9, roomname, obj ? obj->vnum : tReset->arg8 );
                  break;

               case 'T':
                  snprintf( buf + strlen( buf ), MSL - strlen( buf ), "%2d) (trap) %d %d %d %d (%s) (%d%%) -> %s (%d)\r\n",
                            num, tReset->arg1, tReset->arg2, tReset->arg3, tReset->arg4, flag_string( tReset->arg1, trap_flags ), tReset->arg5, objname, obj ? obj->vnum : 0 );
                  break;

               case 'H':
                  snprintf( buf + strlen( buf ), MSL - strlen( buf ), "%2d) (hide) (%d%%) -> %s\r\n", num, tReset->arg3, objname );
                  break;
            }
         }
         break;

      case 'D':
         if( pReset->arg2 < 0 || pReset->arg2 > MAX_DIR + 1 )
            pReset->arg2 = 0;
         if( !( room = get_room_index( pReset->arg1 ) ) )
         {
            mudstrlcpy( roomname, "Room: *BAD VNUM*", MIL );
            snprintf( objname, MIL, "%s (no exit)", dir_name[pReset->arg2] );
         }
         else
         {
            mudstrlcpy( roomname, room->name, MIL );
            snprintf( objname, MIL, "%s%s", dir_name[pReset->arg2], room->get_exit( pReset->arg2 ) ? "" : " (NO EXIT!)" );
         }
         switch ( pReset->arg3 )
         {
            default:
               mudstrlcpy( mobname, "(* ERROR *)", MIL );
               break;
            case 0:
               mudstrlcpy( mobname, "Open", MIL );
               break;
            case 1:
               mudstrlcpy( mobname, "Close", MIL );
               break;
            case 2:
               mudstrlcpy( mobname, "Close and lock", MIL );
               break;
         }
         snprintf( buf, MSL, "%2d) %s [%d] the %s [%d] door %s (%d) (%d%%)\r\n", num, mobname, pReset->arg3, objname, pReset->arg2, roomname, pReset->arg1, pReset->arg4 );
         break;

      case 'R':
         if( !( room = get_room_index( pReset->arg1 ) ) )
            mudstrlcpy( roomname, "Room: *BAD VNUM*", MIL );
         else
            mudstrlcpy( roomname, room->name, MIL );
         snprintf( buf, MSL, "%2d) Randomize exits 0 to %d -> %s (%d)\r\n", num, pReset->arg2, roomname, pReset->arg1 );
         break;

      case 'T':
         if( !( room = get_room_index( pReset->arg4 ) ) )
            mudstrlcpy( roomname, "Room: *BAD VNUM*", MIL );
         else
            mudstrlcpy( roomname, room->name, MIL );
         snprintf( buf, MSL, "%2d) Trap: %d %d %d %d (%s) (%d%%) -> %s (%d)\r\n",
                   num, pReset->arg1, pReset->arg2, pReset->arg3, pReset->arg4, flag_string( pReset->arg1, trap_flags ), pReset->arg5, roomname, room ? room->vnum : 0 );
         break;
   }
   return buf;
}

/*
 * Create a new reset (for online building) - Thoric
 */
reset_data *make_reset( char letter, int arg1, int arg2, int arg3, int arg4, int arg5, int arg6, int arg7, int arg8, int arg9, int arg10, int arg11 )
{
   reset_data *pReset;

   pReset = new reset_data;
   pReset->command = letter;
   pReset->arg1 = arg1;
   pReset->arg2 = arg2;
   pReset->arg3 = arg3;
   pReset->arg4 = arg4;
   pReset->arg5 = arg5;
   pReset->arg6 = arg6;
   pReset->arg7 = arg7;
   pReset->arg8 = arg8;
   pReset->arg9 = arg9;
   pReset->arg10 = arg10;
   pReset->arg11 = arg11;
   ++top_reset;
   return pReset;
}

void add_obj_reset( room_index * room, char cm, obj_data * obj, int v2, int v3 )
{
   static int iNest;

   if( ( cm == 'O' || cm == 'P' ) && obj->pIndexData->vnum == OBJ_VNUM_TRAP )
   {
      if( cm == 'O' )
         room->add_reset( 'T', obj->value[3], obj->value[1], obj->value[0], v3, 100, -2, -2, -2, -2, -2, -2 );
      return;
   }
   if( cm == 'O' )
   {
      // The old obj->map number is no longer relevant so just set it to 0 [value after the v3 argument].
      room->add_reset( cm, obj->pIndexData->vnum, v2, v3, 0, obj->map_x, obj->map_y, 100, -2, -2, -2, -2 );
      if( obj->extra_flags.test( ITEM_HIDDEN ) && !obj->wear_flags.test( ITEM_TAKE ) )
         room->add_reset( 'H', 0, 0, 100, -2, -2, -2, -2, -2, -2, -2, -2 );
   }
   else if( cm == 'P' )
      room->add_reset( cm, iNest, obj->pIndexData->vnum, v2, v3, 100, 100, 100, 100, -2, -2, -2 );
   else
      room->add_reset( cm, obj->pIndexData->vnum, v2, v3, 100, 100, 100, 100, -2, -2, -2, -2 );

   list < obj_data * >::iterator iobj;
   for( iobj = obj->contents.begin(  ); iobj != obj->contents.end(  ); ++iobj )
   {
      obj_data *inobj = *iobj;

      if( inobj->pIndexData->vnum == OBJ_VNUM_TRAP )
         add_obj_reset( room, 'O', inobj, 0, 0 );
   }
   if( cm == 'P' )
      ++iNest;
   for( iobj = obj->contents.begin(  ); iobj != obj->contents.end(  ); ++iobj )
   {
      obj_data *inobj = *iobj;

      add_obj_reset( room, 'P', inobj, inobj->count, obj->pIndexData->vnum );
   }
   if( cm == 'P' )
      --iNest;
}

void delete_reset( reset_data * pReset )
{
   list < reset_data * >::iterator rst;

   for( rst = pReset->resets.begin(  ); rst != pReset->resets.end(  ); )
   {
      reset_data *tReset = *rst;
      ++rst;

      pReset->resets.remove( tReset );
      delete_reset( tReset );
   }
   pReset->resets.clear(  );
   deleteptr( pReset );
}

void instaroom( char_data * ch, room_index * pRoom, bool dodoors )
{
   list < char_data * >::iterator ich;

   for( ich = pRoom->people.begin(  ); ich != pRoom->people.end(  ); ++ich )
   {
      char_data *rch = *ich;

      if( !rch->isnpc(  ) )
         continue;

      bool added = false;
      if( pRoom->flags.test( ROOM_MAP ) && is_same_char_map( ch, rch ) )
      {
         // The old ch->map number is no longer relevant so just set it to 0 [value after the pRoom->vnum argument].
         pRoom->add_reset( 'M', rch->pIndexData->vnum, rch->pIndexData->count, pRoom->vnum, 0, ch->map_x, ch->map_y, 100, -2, -2, -2, -2 );
         added = true;
      }
      else if( !pRoom->flags.test( ROOM_MAP ) )
      {
         pRoom->add_reset( 'M', rch->pIndexData->vnum, rch->pIndexData->count, pRoom->vnum, -1, -1, -1, 100, -2, -2, -2, -2 );
         added = true;
      }
      if( added )
      {
         list < obj_data * >::iterator iobj;
         for( iobj = rch->carrying.begin(  ); iobj != rch->carrying.end(  ); ++iobj )
         {
            obj_data *obj = *iobj;

            if( obj->wear_loc == WEAR_NONE )
               add_obj_reset( pRoom, 'G', obj, 1, 100 ); // we want obj to load 100% of the time in mob inv, not 0%. - Parsival 2017-1214
            else
               add_obj_reset( pRoom, 'E', obj, 1, obj->wear_loc );
         }
      }
   }

   list < obj_data * >::iterator iobj;
   for( iobj = pRoom->objects.begin(  ); iobj != pRoom->objects.end(  ); ++iobj )
   {
      obj_data *obj = *iobj;

      if( pRoom->flags.test( ROOM_MAP ) && is_same_obj_map( ch, obj ) )
         add_obj_reset( pRoom, 'O', obj, obj->count, pRoom->vnum );
      else if( !pRoom->flags.test( ROOM_MAP ) )
         add_obj_reset( pRoom, 'O', obj, obj->count, pRoom->vnum );
   }

   if( dodoors )
   {
      list < exit_data * >::iterator ex;

      for( ex = pRoom->exits.begin(  ); ex != pRoom->exits.end(  ); ++ex )
      {
         exit_data *pexit = *ex;
         int state = 0;

         if( !IS_EXIT_FLAG( pexit, EX_ISDOOR ) && !IS_EXIT_FLAG( pexit, EX_DIG ) )
            continue;

         if( IS_EXIT_FLAG( pexit, EX_CLOSED ) )
         {
            if( IS_EXIT_FLAG( pexit, EX_LOCKED ) )
               state = 2;
            else
               state = 1;
         }
         pRoom->add_reset( 'D', pRoom->vnum, pexit->vdir, state, 100, -2, -2, -2, -2, -2, -2, -2 );
      }
   }
}

CMDF( do_instaroom )
{
   bool dodoors;

#ifdef MULTIPORT
   if( mud_port == MAINPORT )
   {
      ch->print( "Instaroom is disabled on this port.\r\n" );
      return;
   }
#endif

   if( ch->isnpc(  ) || ch->get_trust(  ) < LEVEL_SAVIOR || !ch->pcdata->area )
   {
      ch->print( "You don't have an assigned area to create resets for.\r\n" );
      return;
   }

   if( !str_cmp( argument, "nodoors" ) )
      dodoors = false;
   else
      dodoors = true;

   if( !can_rmodify( ch, ch->in_room ) )
      return;
   if( ch->in_room->area != ch->pcdata->area && ch->get_trust(  ) < LEVEL_GREATER )
   {
      ch->print( "You cannot reset this room.\r\n" );
      return;
   }
   if( !ch->in_room->resets.empty() && ch->has_pcflag( PCFLAG_ONMAP ) )
      ch->in_room->wipe_coord_resets( ch->map_x, ch->map_y );
   else if( !ch->in_room->resets.empty() )
      ch->in_room->wipe_resets();
   if( !ch->in_room->resets.empty(  ) )
      ch->in_room->wipe_resets(  );
   instaroom( ch, ch->in_room, dodoors );
   ch->print( "Room resets installed.\r\n" );
}

CMDF( do_instazone )
{
   bool dodoors;

#ifdef MULTIPORT
   if( mud_port == MAINPORT )
   {
      ch->print( "Instazone is disabled on this port.\r\n" );
      return;
   }
#endif

   if( ch->isnpc(  ) || ch->get_trust(  ) < LEVEL_SAVIOR || !ch->pcdata->area )
   {
      ch->print( "You don't have an assigned area to create resets for.\r\n" );
      return;
   }
   if( !str_cmp( argument, "nodoors" ) )
      dodoors = false;
   else
      dodoors = true;

   area_data *pArea = ch->pcdata->area;
   pArea->wipe_resets(  );

   list < room_index * >::iterator iroom;
   for( iroom = pArea->rooms.begin(  ); iroom != pArea->rooms.end(  ); ++iroom )
   {
      room_index *pRoom = *iroom;

      instaroom( ch, pRoom, dodoors );
   }
   ch->print( "Area resets installed.\r\n" );
}

reset_data *find_oreset( room_index * room, const string & oname )
{
   list < reset_data * >::iterator rst;
   obj_index *pobj;
   string arg;
   int cnt = 0, num = number_argument( oname, arg );

   for( rst = room->resets.begin(  ); rst != room->resets.end(  ); ++rst )
   {
      reset_data *pReset = *rst;

      // Only going to allow traps/hides on room reset objects. Unless someone can come up with a better way to do this.
      if( pReset->command != 'O' )
         continue;

      if( !( pobj = get_obj_index( pReset->arg1 ) ) )
         continue;

      if( hasname( pobj->name, arg ) && ++cnt == num )
         return pReset;
   }
   return nullptr;
}

CMDF( do_reset )
{
   string arg;

   if( argument.empty(  ) )
   {
      ch->print( "Usage: reset area\r\n" );
      ch->print( "Usage: reset randomize <direction>\r\n" );
      ch->print( "Usage: reset delete <number>\r\n" );
      ch->print( "Usage: reset percent <number> <value>\r\n" );
      ch->print( "Usage: reset hide <objname>\r\n" );
      ch->print( "Usage: reset trap room <type> <charges> <mindamage> <maxdamage> [flags]\r\n" );
      ch->print( "Usage: reset trap obj <name> <type> <charges> <mindamage> <maxdamage> [flags]\r\n\r\n" );
      ch->print( "If mindamage and maxdamage are set to 0, trap damage will be determined automatically.\r\n" );
      return;
   }

   argument = one_argument( argument, arg );
   if( !str_cmp( arg, "area" ) )
   {
      ch->in_room->area->reset(  );
      ch->print( "Area has been reset.\r\n" );
      return;
   }

   // Yeah, I know, this function is mucho ugly... but...
   if( !str_cmp( arg, "delete" ) )
   {
      list < reset_data * >::iterator rst;
      reset_data *pReset;
      int num, nfind = 0;

      if( argument.empty(  ) )
      {
         ch->print( "You must specify a reset # in this room to delete one.\r\n" );
         return;
      }

      if( !is_number( argument ) )
      {
         ch->print( "Specified reset must be designated by number. See &Wredit rlist&D.\r\n" );
         return;
      }
      num = atoi( argument.c_str(  ) );

      for( rst = ch->in_room->resets.begin(  ); rst != ch->in_room->resets.end(  ); )
      {
         list < reset_data * >::iterator dst;
         reset_data *tReset;
         pReset = *rst;
         ++rst;

         ++nfind;
         if( nfind == num )
         {
            ch->in_room->resets.remove( pReset );
            delete_reset( pReset );
            ch->print( "Reset deleted.\r\n" );
            return;
         }

         for( dst = pReset->resets.begin(  ); dst != pReset->resets.end(  ); )
         {
            list < reset_data * >::iterator gst;
            reset_data *gReset;
            tReset = *dst;
            ++dst;

            ++nfind;
            if( nfind == num )
            {
               pReset->resets.remove( tReset );
               delete_reset( tReset );
               ch->print( "Reset deleted.\r\n" );
               return;
            }

            for( gst = tReset->resets.begin(  ); gst != tReset->resets.end(  ); )
            {
               gReset = *gst;
               ++gst;

               ++nfind;
               if( nfind == num )
               {
                  tReset->resets.remove( gReset );
                  delete_reset( gReset );
                  ch->print( "Reset deleted.\r\n" );
                  return;
               }
            }
         }
      }
      ch->print( "No reset matching that number was found in this room.\r\n" );
      return;
   }

   // Yeah, I know, this function is mucho ugly... but...
   if( !str_cmp( arg, "percent" ) )
   {
      list < reset_data * >::iterator rst;
      reset_data *pReset;
      int num, value = 100, nfind = 0;

      argument = one_argument( argument, arg );

      if( arg.empty(  ) )
      {
         ch->print( "You must specify a reset # in this room.\r\n" );
         return;
      }
      if( !is_number( arg ) )
      {
         ch->print( "Specified reset must be designated by number. See &Wredit rlist&D.\r\n" );
         return;
      }
      num = atoi( arg.c_str(  ) );

      if( argument.empty(  ) )
      {
         ch->print( "You must specify a percentage value.\r\n" );
         return;
      }
      if( !is_number( argument ) )
      {
         ch->print( "Specified value must be numeric.\r\n" );
         return;
      }
      value = atoi( argument.c_str(  ) );

      for( rst = ch->in_room->resets.begin(  ); rst != ch->in_room->resets.end(  ); )
      {
         list < reset_data * >::iterator dst;
         reset_data *tReset;
         pReset = *rst;
         ++rst;

         ++nfind;
         if( nfind == num )
         {
            if( pReset->command == 'O' || pReset->command == 'M' )
            {
               pReset->arg7 = value;
               ch->print( "Percentage set.\r\n" );
            }
            else
               ch->print( "Only top level resets ( O/M ) are supported (for now).\r\n" );
            return;
         }

         for( dst = pReset->resets.begin(  ); dst != pReset->resets.end(  ); )
         {
            list < reset_data * >::iterator gst;
            tReset = *dst;
            ++dst;

            ++nfind;
            if( nfind == num )
            {
               ch->print( "Only top level resets ( O/M ) are supported (for now).\r\n" );
               return;
            }

            for( gst = tReset->resets.begin(  ); gst != tReset->resets.end(  ); ++gst )
            {
               ++nfind;
               if( nfind == num )
               {
                  ch->print( "Only top level resets ( O/M ) are supported (for now).\r\n" );
                  return;
               }
            }
         }
      }
      ch->print( "No reset matching that number was found in this room.\r\n" );
      return;
   }

   if( !str_cmp( arg, "random" ) )
   {
      argument = one_argument( argument, arg );
      int door = get_dir( arg );

      if( door < 0 || door > 9 )
      {
         ch->print( "Reset which directions randomly?\r\n" );
         ch->print( "3 would randomize north, south, east, west.\r\n" );
         ch->print( "5 would do those plus up, down.\r\n" );
         ch->print( "9 would do those, plus ne, nw, se, sw as well.\r\n" );
         return;
      }

      if( door == 0 )
      {
         ch->print( "There is no point in randomizing one door.\r\n" );
         return;
      }

      reset_data *pReset = make_reset( 'R', ch->in_room->vnum, door, 100, -2, -2, -2, -2, -2, -2, -2, -2 );
      ch->in_room->resets.push_front( pReset );
      ch->print( "Reset random doors created.\r\n" );
      return;
   }

   if( !str_cmp( arg, "trap" ) )
   {
      reset_data *pReset = nullptr;
      string arg2, oname;
      int type, chrg, flags = 0, vnum, min, max;

      argument = one_argument( argument, arg2 );

      if( !str_cmp( arg2, "room" ) )
      {
         vnum = ch->in_room->vnum;
         flags = TRAP_ROOM;

         argument = one_argument( argument, arg );
         type = is_number( arg ) ? atoi( arg.c_str(  ) ) : -1;

         argument = one_argument( argument, arg );
         chrg = is_number( arg ) ? atoi( arg.c_str(  ) ) : -1;

         argument = one_argument( argument, arg );
         min = is_number( arg ) ? atoi( arg.c_str(  ) ) : -1;

         argument = one_argument( argument, arg );
         max = is_number( arg ) ? atoi( arg.c_str(  ) ) : -1;
      }
      else if( !str_cmp( arg2, "obj" ) )
      {
         argument = one_argument( argument, oname );
         if( !( pReset = find_oreset( ch->in_room, oname ) ) )
         {
            ch->print( "No matching reset found to set a trap on.\r\n" );
            return;
         }
         vnum = 0;
         flags = TRAP_OBJ;

         argument = one_argument( argument, arg );
         type = is_number( arg ) ? atoi( arg.c_str(  ) ) : -1;

         argument = one_argument( argument, arg );
         chrg = is_number( arg ) ? atoi( arg.c_str(  ) ) : -1;

         argument = one_argument( argument, arg );
         min = is_number( arg ) ? atoi( arg.c_str(  ) ) : -1;

         argument = one_argument( argument, arg );
         max = is_number( arg ) ? atoi( arg.c_str(  ) ) : -1;
      }
      else
      {
         ch->print( "Trap reset must be on 'room' or 'obj'\r\n" );
         return;
      }

      if( type < 1 || type > MAX_TRAPTYPE )
      {
         ch->print( "Invalid trap type.\r\n" );
         return;
      }

      if( chrg < 0 || chrg > 10000 )
      {
         ch->print( "Invalid trap charges. Must be between 1 and 10000.\r\n" );
         return;
      }

      if( min <= 0 )
         min = 1;
      else if( min >= max )
      {
         ch->printf( "Min of %d must be smaller than Max of %d\r\n", min, max );
         return;
      }

      if( max <= 0 )
         max = 6;
      else if( max <= min )
      {
         ch->printf( "Max of %d must be larger than Min of %d\r\n", max, min );
         return;
      }

      while( !argument.empty(  ) )
      {
         argument = one_argument( argument, arg );
         int value = get_trapflag( arg );

         if( value < 0 || value > 31 )
         {
            ch->printf( "Bad trap flag: %s\r\n", arg.c_str(  ) );
            continue;
         }
         SET_BIT( flags, 1 << value );
      }
      reset_data *tReset = make_reset( 'T', flags, type, chrg, vnum, 100, min, max, -2, -2, -2, -2 );
      if( pReset )
         pReset->resets.push_front( tReset );
      else
         ch->in_room->resets.push_front( tReset );
      ch->print( "Trap created.\r\n" );
      return;
   }

   if( !str_cmp( arg, "hide" ) )
   {
      reset_data *pReset = nullptr;

      if( !( pReset = find_oreset( ch->in_room, argument ) ) )
      {
         ch->print( "No such object to hide in this room.\r\n" );
         return;
      }
      reset_data *tReset = make_reset( 'H', 100, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2 );
      if( pReset )
         pReset->resets.push_front( tReset );
      else
         ch->in_room->resets.push_front( tReset );
      ch->print( "Hide reset created.\r\n" );
      return;
   }
   do_reset( ch, "" );
}

// Update the mobile resets to let it know to reset it again
void update_room_reset( char_data * ch, bool setting )
{
   room_index *room;
   list < reset_data * >::iterator pst;
   int nfind = 0;

   if( !ch )
      return;

   if( !( room = get_room_index( ch->resetvnum ) ) )
      return;

   for( pst = room->resets.begin(  ); pst != room->resets.end(  ); ++pst )
   {
      list < reset_data * >::iterator tst;
      reset_data *pReset = *pst;

      if( ++nfind == ch->resetnum )
      {
         pReset->sreset = setting;
         return;
      }

      for( tst = pReset->resets.begin(  ); tst != pReset->resets.end(  ); ++tst )
      {
         list < reset_data * >::iterator gst;
         reset_data *tReset = *tst;

         if( ++nfind == ch->resetnum )
         {
            tReset->sreset = setting;
            return;
         }

         for( gst = tReset->resets.begin(  ); gst != tReset->resets.end(  ); ++gst )
         {
            reset_data *gReset = *gst;

            if( ++nfind == ch->resetnum )
            {
               gReset->sreset = setting;
               return;
            }
         }
      }
   }
}
