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
 *                          Area Renumbering Module                         *
 ****************************************************************************/

/*
 *  Renumber Imm command
 *  Author: Cronel (cronel_kal@hotmail.com)
 *  of FrozenMUD (empire.digiunix.net 4000)
 *
 *  Permission to use and distribute this code is granted provided
 *  this header is retained and unaltered, and the distribution
 *  package contains all the original files unmodified.
 *  If you modify this code and use/distribute modified versions
 *  you must give credit to the original author(s).
 */
#include "mud.h"
#include "area.h"
#include "mobindex.h"
#include "mud_prog.h"
#include "objindex.h"
#include "overland.h"
#include "roomindex.h"
#include "shops.h"

const int NOT_FOUND = -1;
enum
{ REN_ROOM, REN_OBJ, REN_MOB };

struct renumber_data
{
   int old_vnum;
   int new_vnum;

   renumber_data *next;
};

struct renumber_areas
{
   renumber_data *r_obj;
   renumber_data *r_mob;
   renumber_data *r_room;
   int low_obj, hi_obj;
   int low_mob, hi_mob;
   int low_room, hi_room;
};

/* from db.c */
void save_sysdata(  );

/* returns the new vnum for the old vnum "vnum" according to the info in r_data */
int find_translation( int vnum, renumber_data * r_data )
{
   renumber_data *r_temp;

   for( r_temp = r_data; r_temp; r_temp = r_temp->next )
   {
      if( r_temp->old_vnum == vnum )
         return r_temp->new_vnum;
   }
   return NOT_FOUND;
}

void translate_exits( char_data * ch, area_data * area, renumber_areas * r_area )
{
   int new_vnum;
   room_index *room;
   int old_vnum;

   for( int i = area->low_vnum; i <= area->hi_vnum; ++i )
   {
      if( !( room = get_room_index( i ) ) )
         continue;

      list < exit_data * >::iterator iexit;
      for( iexit = room->exits.begin(  ); iexit != room->exits.end(  ); ++iexit )
      {
         exit_data *pexit = ( *iexit );

         /*
          * translate the exit destination, if it was moved 
          */
         new_vnum = find_translation( pexit->vnum, r_area->r_room );
         if( new_vnum != NOT_FOUND )
            pexit->vnum = new_vnum;
         /*
          * if this room was moved 
          */
         if( pexit->rvnum != i )
         {
            old_vnum = pexit->rvnum;
            pexit->rvnum = i;
            /*
             * all reverse exits in other areas will be wrong 
             */
            exit_data *rv_exit = pexit->to_room->get_exit_to( rev_dir[pexit->vdir], old_vnum );
            if( rv_exit && pexit->to_room->area != area )
            {
               if( rv_exit->vnum != i )
               {
                  ch->pagerf( "...    fixing reverse exit in area %s.\r\n", pexit->to_room->area->filename );
                  rv_exit->vnum = i;
               }
            }
         }

         /*
          * translate the key 
          */
         if( pexit->key != -1 )
         {
            new_vnum = find_translation( pexit->key, r_area->r_obj );
            if( new_vnum == NOT_FOUND )
               continue;
            pexit->key = new_vnum;
         }
      }

      list < mapexit_data * >::iterator imexit;
      for( imexit = mapexitlist.begin(  ); imexit != mapexitlist.end(  ); ++imexit )
      {
         mapexit_data *mexit = ( *imexit );

         new_vnum = find_translation( mexit->vnum, r_area->r_room );
         if( new_vnum != NOT_FOUND )
         {
            mexit->vnum = new_vnum;
            ch->pager( "...    fixing overland exit to area.\r\n" );
         }
      }
   }
}

/* this function translates a reset according to the renumber data in r_data */
void translate_reset( reset_data * reset, renumber_areas * r_data )
{
   /*
    * a list based approach to fixing the resets. instead of having a bunch of several instances of very 
    * similar code, i just made this array that tells the code what to do. it's pretty straightforward 
    */
   const char *action_table[] = { "Mm1r3", "Oo1r3", "Ho1", "Po2o4", "Go1", "Eo1", "Dr1", "Rr1", "Wo8", "Zr7", NULL };
   const char *p;
   renumber_data *r_table;
   int *parg, new_vnum, i;

   /*
    * T is a special case 
    */
   if( reset->command == 'T' )
   {
      if( IS_SET( reset->arg1, TRAP_ROOM ) )
         r_table = r_data->r_room;
      else if( IS_SET( reset->arg1, TRAP_OBJ ) )
         r_table = r_data->r_obj;
      else
      {
         bug( "%s: Invalid 'T' reset found.", __FUNCTION__ );
         return;
      }
      new_vnum = find_translation( reset->arg4, r_table );
      if( new_vnum != NOT_FOUND )
         reset->arg4 = new_vnum;
      return;
   }

   for( i = 0; action_table[i] != NULL; ++i )
   {
      if( reset->command == action_table[i][0] )
      {
         p = action_table[i] + 1;
         while( *p )
         {
            if( *p == 'm' )
               r_table = r_data->r_mob;
            else if( *p == 'o' )
               r_table = r_data->r_obj;
            else if( *p == 'r' )
               r_table = r_data->r_room;
            else
            {
               bug( "%s: Invalid action found in action table.", __FUNCTION__ );
               p += 2;
               continue;
            }
            ++p;

            if( *p == '1' )
               parg = &( reset->arg1 );
            else if( *p == '2' )
               parg = &( reset->arg2 );
            else if( *p == '3' )
               parg = &( reset->arg3 );
            else
            {
               bug( "%s: Invalid argument number found in action table.", __FUNCTION__ );
               ++p;
               continue;
            }
            ++p;

            new_vnum = find_translation( *parg, r_table );
            if( new_vnum != NOT_FOUND )
               *parg = new_vnum;
         }
         return;
      }
   }

   if( action_table[i] == NULL )
      bug( "%s: Invalid reset '%c' found.", __FUNCTION__, reset->command );
}

void translate_objvals( char_data * ch, area_data * area, renumber_areas * r_area, bool verbose )
{
   int i, new_vnum;
   obj_index *obj;

   for( i = area->low_vnum; i <= area->hi_vnum; ++i )
   {
      if( !( obj = get_obj_index( i ) ) )
         continue;

      if( obj->item_type == ITEM_CONTAINER )
      {
         new_vnum = find_translation( obj->value[2], r_area->r_obj );
         if( new_vnum != NOT_FOUND )
         {
            if( verbose )
               ch->pagerf( "...   container %d; fixing objval2 (key vnum) %d -> %d\r\n", i, obj->value[2], new_vnum );
            obj->value[2] = new_vnum;
         }
         else if( verbose )
            ch->pagerf( "...    container %d; no need to fix.\r\n", i );
      }
      else if( obj->item_type == ITEM_SWITCH || obj->item_type == ITEM_LEVER || obj->item_type == ITEM_PULLCHAIN || obj->item_type == ITEM_BUTTON )
      {
         /*
          * levers might have room vnum references in their objvals 
          */
         if( IS_SET( obj->value[0], TRIG_TELEPORT )
             || IS_SET( obj->value[0], TRIG_TELEPORTALL )
             || IS_SET( obj->value[0], TRIG_TELEPORTPLUS )
             || IS_SET( obj->value[0], TRIG_RAND4 ) || IS_SET( obj->value[0], TRIG_RAND6 ) || IS_SET( obj->value[0], TRIG_DOOR ) )
         {
            new_vnum = find_translation( obj->value[1], r_area->r_room );
            if( new_vnum != NOT_FOUND )
            {
               if( verbose )
                  ch->pagerf( "...    lever %d: fixing source room (%d -> %d)\r\n", i, obj->value[1], new_vnum );
               obj->value[1] = new_vnum;
            }
            if( IS_SET( obj->value[0], TRIG_DOOR ) && IS_SET( obj->value[0], TRIG_PASSAGE ) )
            {
               new_vnum = find_translation( obj->value[2], r_area->r_room );
               if( new_vnum != NOT_FOUND )
               {
                  if( verbose )
                     ch->pagerf( "...    lever %d: fixing dest room (passage) (%d -> %d)\r\n", i, obj->value[2], new_vnum );
                  obj->value[2] = new_vnum;
               }
            }
         }
      }
   }
}

void warn_in_prog( char_data * ch, int low, int high, const char *where, int vnum, mud_prog_data * mprog, renumber_areas * r_area )
{
   char *p, *start_number, cTmp;
   int num;

   p = mprog->comlist;
   while( *p )
   {
      if( isdigit( *p ) )
      {
         start_number = p;
         while( isdigit( *p ) && *p )
            ++p;
         cTmp = *p;
         *p = 0;
         num = atoi( start_number );
         *p = cTmp;
         if( num >= low && num <= high )
         {
            ch->
               pagerf
               ( "Warning! %s prog in %s vnum %d might contain a reference to %d.\r\n(Translation: Room %d, Obj %d, Mob %d)\r\n",
                 mprog_type_to_name( mprog->type ).c_str(  ), where, vnum, num, find_translation( num, r_area->r_room ),
                 find_translation( num, r_area->r_obj ), find_translation( num, r_area->r_mob ) );
         }
         if( *p == '\0' )
            break;
      }
      ++p;
   }
}

void warn_progs( char_data * ch, int low, int high, area_data * area, renumber_areas * r_area )
{
   room_index *room;
   obj_index *obj;
   mob_index *mob;
   mud_prog_data *mprog;
   list < mud_prog_data * >::iterator mpg;
   int i;

   for( i = area->low_vnum; i <= area->hi_vnum; ++i )
   {
      if( !( room = get_room_index( i ) ) )
         continue;

      for( mpg = room->mudprogs.begin(  ); mpg != room->mudprogs.end(  ); ++mpg )
      {
         mprog = ( *mpg );

         warn_in_prog( ch, low, high, "room", i, mprog, r_area );
      }
   }

   for( i = area->low_vnum; i <= area->hi_vnum; ++i )
   {
      if( !( obj = get_obj_index( i ) ) )
         continue;

      for( mpg = obj->mudprogs.begin(  ); mpg != obj->mudprogs.end(  ); ++mpg )
      {
         mprog = ( *mpg );

         warn_in_prog( ch, low, high, "obj", i, mprog, r_area );
      }
   }

   for( i = area->low_vnum; i <= area->hi_vnum; ++i )
   {
      if( !( mob = get_mob_index( i ) ) )
         continue;

      for( mpg = mob->mudprogs.begin(  ); mpg != mob->mudprogs.end(  ); ++mpg )
      {
         mprog = ( *mpg );

         warn_in_prog( ch, low, high, "mob", i, mprog, r_area );
      }
   }
}

/* this is the function that actualy does the renumbering of "area" according
 * to the renumber data in "r_area". "ch" is to show messages.
 */
void renumber_area( char_data * ch, area_data * area, renumber_areas * r_area, bool verbose )
{
   renumber_data *r_data;
   room_index *room;
   mob_index *mob;
   obj_index *obj;
   list < mob_index * >mob_list;
   list < obj_index * >obj_list;
   list < room_index * >room_list;

   int high = area->hi_vnum, low = area->low_vnum;

   ch->pager( "(Room) Renumbering...\r\n" );

   /*
    * what we do here is, for each list (room/obj/mob) first we
    * * take each element out of the hash array, change the vnum,
    * * and move it to our own list. after everything's moved out
    * * we put it in again. this is to avoid problems in situations
    * * where where room A is being moved to position B, but theres
    * * already a room B wich is also being moved to position C.
    * * a straightforward approach would result in us moving A to
    * * position B first, and then again to position C, and room
    * * B being lost inside the hash array, still there, but not
    * * foundable (its "covered" by A because they'd have the same vnum). 
    */

   room_list.clear();
   for( r_data = r_area->r_room; r_data; r_data = r_data->next )
   {
      if( verbose )
         ch->pagerf( "(Room) %d -> %d\r\n", r_data->old_vnum, r_data->new_vnum );

      room = get_room_index( r_data->old_vnum );
      if( !room )
      {
         bug( "%s: NULL room %d", __FUNCTION__, r_data->old_vnum );
         continue;
      }

      /*
       * remove it from the hash list 
       */
      map < int, room_index * >::iterator mroom;
      if( ( mroom = room_index_table.find( r_data->old_vnum ) ) != room_index_table.end(  ) )
         room_index_table.erase( mroom );

      /*
       * change the vnum 
       */
      room->vnum = r_data->new_vnum;

      /*
       * move it to the temporary list 
       */
      room_list.push_back( room );
   }

   /*
    * now move everything back into the hash array 
    */
   list < room_index * >::iterator iroom;
   for( iroom = room_list.begin(  ); iroom != room_list.end(  ); ++iroom )
   {
      room = *iroom;

      /*
       * add it to the hash list again (new position) 
       */
      room_index_table.insert( map < int, room_index * >::value_type( room->vnum, room ) );
   }

   /*
    * if nothing was moved, dont change this 
    */
   if( r_area->r_room != NULL )
   {
      area->low_vnum = r_area->low_room;
      area->hi_vnum = r_area->hi_room;
   }

   mob_list.clear(  );
   ch->pager( "(Mobs) Renumbering...\r\n" );
   for( r_data = r_area->r_mob; r_data; r_data = r_data->next )
   {
      if( verbose )
         ch->pagerf( "(Mobs) %d -> %d\r\n", r_data->old_vnum, r_data->new_vnum );

      mob = get_mob_index( r_data->old_vnum );
      if( !mob )
      {
         bug( "%s: NULL mob %d", __FUNCTION__, r_data->old_vnum );
         continue;
      }

      /*
       * fix references to this mob from shops while renumbering this mob 
       */
      if( mob->pShop )
      {
         if( verbose )
            ch->pagerf( "(Mobs) Fixing shop for mob %d -> %d\r\n", r_data->old_vnum, r_data->new_vnum );
         mob->pShop->keeper = r_data->new_vnum;
      }
      if( mob->rShop )
      {
         if( verbose )
            ch->pagerf( "(Mobs) Fixing repair shop for mob %d -> %d\r\n", r_data->old_vnum, r_data->new_vnum );
         mob->rShop->keeper = r_data->new_vnum;
      }

      /*
       * remove it from the hash list 
       */
      map < int, mob_index * >::iterator mmob;
      if( ( mmob = mob_index_table.find( r_data->old_vnum ) ) != mob_index_table.end(  ) )
         mob_index_table.erase( mmob );

      /*
       * change the vnum 
       */
      mob->vnum = r_data->new_vnum;

      /*
       * move to private list 
       */
      mob_list.push_back( mob );
   }

   list < mob_index * >::iterator imob;
   for( imob = mob_list.begin(  ); imob != mob_list.end(  ); ++imob )
   {
      mob = *imob;

      /*
       * add it to the hash list again 
       */
      mob_index_table.insert( map < int, mob_index * >::value_type( mob->vnum, mob ) );
   }

   if( r_area->r_mob )
   {
      if( r_area->low_mob < r_area->low_room )
         area->low_vnum = r_area->low_mob;
      if( r_area->hi_mob > r_area->hi_room )
         area->hi_vnum = r_area->hi_mob;
   }

   ch->pager( "(Objs) Renumbering...\r\n" );
   obj_list.clear();
   for( r_data = r_area->r_obj; r_data; r_data = r_data->next )
   {
      if( verbose )
         ch->pagerf( "(Objs) %d -> %d\r\n", r_data->old_vnum, r_data->new_vnum );
      obj = get_obj_index( r_data->old_vnum );
      if( !obj )
      {
         bug( "%s: NULL obj %d", __FUNCTION__, r_data->old_vnum );
         continue;
      }

      /*
       * remove it from the hash list 
       */
      map < int, obj_index * >::iterator mobj;
      if( ( mobj = obj_index_table.find( r_data->old_vnum ) ) != obj_index_table.end(  ) )
         obj_index_table.erase( mobj );

      /*
       * change the vnum 
       */
      obj->vnum = r_data->new_vnum;

      /*
       * to our list 
       */
      obj_list.push_back( obj );
   }

   list < obj_index * >::iterator iobj;
   for( iobj = obj_list.begin(  ); iobj != obj_list.end(  ); ++iobj )
   {
      obj = *iobj;

      /*
       * add it to the hash list again 
       */
      obj_index_table.insert( map < int, obj_index * >::value_type( obj->vnum, obj ) );
   }

   if( r_area->r_obj )
   {
      if( r_area->low_obj < r_area->low_room && r_area->low_obj < r_area->low_mob )
         area->low_vnum = r_area->low_obj;
      if( r_area->hi_obj > r_area->hi_room && r_area->hi_obj > r_area->hi_mob )
         area->hi_vnum = r_area->hi_obj;
   }

   ch->pager( "Fixing references...\r\n" );

   ch->pager( "... fixing objvals...\r\n" );
   translate_objvals( ch, area, r_area, verbose );

   ch->pager( "... fixing exits...\r\n" );
   translate_exits( ch, area, r_area );

   ch->pager( "... fixing resets...\r\n" );

   for( iroom = area->rooms.begin(  ); iroom != area->rooms.end(  ); ++iroom )
   {
      room = *iroom;

      list < reset_data * >::iterator rst;
      for( rst = room->resets.begin(  ); rst != room->resets.end(  ); ++rst )
      {
         reset_data *preset = *rst;

         translate_reset( preset, r_area );
         list < reset_data * >::iterator dst;
         for( dst = preset->resets.begin(  ); dst != preset->resets.end(  ); ++dst )
         {
            reset_data *treset = *dst;

            translate_reset( treset, r_area );
         }
      }
   }

   if( verbose )
   {
      ch->pager( "Searching progs for references to renumbered vnums...\r\n" );
      warn_progs( ch, low, high, area, r_area );
   }
}

/* this function builds a list of renumber data for a type (obj, room, or mob) */
renumber_data *gather_one_list( short type, int low, int high, int new_base, bool fill_gaps, int *max_vnum )
{
   renumber_data *r_data, root;
   bool found;
   room_index *room;
   obj_index *obj;
   mob_index *mob;
   int i, highest, cur_vnum;

   memset( &root, 0, sizeof( renumber_data ) );
   r_data = &root;

   cur_vnum = new_base;
   highest = -1;
   for( i = low; i <= high; ++i )
   {
      found = false;
      switch ( type )
      {
         default:
            break;
         case REN_ROOM:
            room = get_room_index( i );
            if( room != NULL )
               found = true;
            break;
         case REN_OBJ:
            obj = get_obj_index( i );
            if( obj != NULL )
               found = true;
            break;
         case REN_MOB:
            mob = get_mob_index( i );
            if( mob != NULL )
               found = true;
            break;
      }

      if( found )
      {
         if( cur_vnum > highest )
            highest = cur_vnum;
         if( cur_vnum != i )
         {
            CREATE( r_data->next, renumber_data, 1 );
            r_data = r_data->next;
            r_data->old_vnum = i;
            r_data->new_vnum = cur_vnum;
         }
         ++cur_vnum;
      }
      else if( !fill_gaps )
         ++cur_vnum;
   }
   *max_vnum = highest;
   return root.next;
}

renumber_areas *gather_renumber_data( area_data * area, int new_base, bool fill_gaps )
/* this function actualy gathers all the renumber data for an area */
{
   renumber_areas *r_area;
   int max;

   r_area = new renumber_areas;

   r_area->r_mob = gather_one_list( REN_MOB, area->low_vnum, area->hi_vnum, new_base, fill_gaps, &max );
   r_area->low_mob = new_base;
   r_area->hi_mob = max;

   r_area->r_obj = gather_one_list( REN_OBJ, area->low_vnum, area->hi_vnum, new_base, fill_gaps, &max );
   r_area->low_obj = new_base;
   r_area->hi_obj = max;

   r_area->r_room = gather_one_list( REN_ROOM, area->low_vnum, area->hi_vnum, new_base, fill_gaps, &max );
   r_area->low_room = new_base;
   r_area->hi_room = max;

   return r_area;
}

bool check_vnums( char_data * ch, area_data * tarea, renumber_areas * r_area )
{
   /*
    * this function assumes all the lows are always gonna be lower or equal to all the highs .. 
    */
   int high = UMAX( r_area->hi_room, UMAX( r_area->hi_obj, r_area->hi_mob ) );
   int low = UMIN( r_area->low_room, UMIN( r_area->low_obj, r_area->low_mob ) );

   if( high > sysdata->maxvnum )
   {
      ch->printf( "This operation would raise the maximum allowed vnum beyond %d.\r\n", sysdata->maxvnum );
      ch->print( "Pick a lower base, or have sysdata->maxvnum raised.\r\n" );
      return true;
   }

   /*
    * in do_check_vnums they use first_bsort, first_asort but.. i dunno.. 
    */
   list < area_data * >::iterator iarea;
   for( iarea = arealist.begin(  ); iarea != arealist.end(  ); ++iarea )
   {
      area_data *area = ( *iarea );

      if( tarea == area )
         continue;

      if( !( high < area->low_vnum || low > area->hi_vnum ) )
      {
         ch->printf( "This operation would overwrite area %s! Use checkvnums first.\r\n", area->filename );
         return true;
      }
   }
   return false;
}

/* disposes of a list of renumber data items */
void free_renumber_data( renumber_data * r_data )
{
   renumber_data *r_next;

   while( r_data != NULL )
   {
      r_next = r_data->next;
      DISPOSE( r_data );
      r_data = r_next;
   }
}

CMDF( do_renumber )
{
   renumber_areas *r_area;
   area_data *area;
   string arg1;
   int new_base;
   bool fill_gaps, verbose;

   if( ch->isnpc(  ) )
   {
      ch->print( "Yeah, right.\r\n" );
      return;
   }

   /*
    * parse the first two parameters 
    * first, area 
    */
   argument = one_argument( argument, arg1 );
   if( arg1.empty(  ) )
   {
      ch->print( "What area do you want to renumber?\r\n" );
      return;
   }

   area = find_area( arg1 );
   if( area == NULL )
   {
      ch->printf( "No such area '%s'.\r\n", arg1.c_str(  ) );
      return;
   }

   /*
    * and new vnum base 
    */
   argument = one_argument( argument, arg1 );
   if( arg1.empty(  ) )
   {
      ch->print( "What will be the new vnum base for this area?\r\n" );
      return;
   }

   if( !is_number( arg1 ) )
   {
      ch->printf( "Sorry, '%s' is not a valid vnum base number!\r\n", arg1.c_str(  ) );
      return;
   }

   new_base = atoi( arg1.c_str(  ) );

   /*
    * parse the flags 
    */
   fill_gaps = false;
   verbose = false;
   for( ;; )
   {
      argument = one_argument( argument, arg1 );
      if( arg1.empty(  ) )
         break;
      else if( !str_prefix( arg1, "fillgaps" ) )
         fill_gaps = true;
      else if( !str_prefix( arg1, "verbose" ) )
         verbose = true;
      else
      {
         ch->printf( "Invalid flag '%s'.\r\n", arg1.c_str(  ) );
         return;
      }
   }

   /*
    * sanity check 
    */
   if( new_base == area->low_vnum && !fill_gaps )
   {
      ch->print( "You don't want to change the base vnum and you don't want to fill gaps.\r\n" );
      ch->print( "So what DO you want to do?\r\n" );
      return;
   }

   /*
    * some restrictions 
    */
   if( ch->level < LEVEL_SAVIOR )
   {
      ch->print( "You don't have enough privileges.\r\n" );
      return;
   }

   if( ch->level == LEVEL_SAVIOR )
   {
      if( area->low_vnum < ch->pcdata->low_vnum || area->hi_vnum > ch->pcdata->hi_vnum )
      {
         ch->printf( "You can't renumber that area ('%s').\r\n", area->filename );
         return;
      }
   }

   /*
    * get the renumber data 
    */
   r_area = gather_renumber_data( area, new_base, fill_gaps );

   /*
    * one more restriction 
    */
   if( ch->level == LEVEL_SAVIOR )
   {
      if( r_area->low_room < ch->pcdata->low_vnum || r_area->hi_room > ch->pcdata->hi_vnum ||
          r_area->low_obj < ch->pcdata->low_vnum || r_area->hi_obj > ch->pcdata->hi_vnum || r_area->low_mob < ch->pcdata->low_vnum || r_area->hi_mob > ch->pcdata->hi_vnum )
      {
         ch->print( "The renumbered area would be outside your assigned vnum range.\r\n" );
         return;
      }
   }

   /*
    * OOO! A new sanity check :) - Samson 10-7-02 
    */
   if( new_base >= sysdata->maxvnum )
   {
      ch->printf( "%d is beyond the maximum allowed vnum of %d\r\n", new_base, sysdata->maxvnum );
      return;
   }

   /*
    * no overwriting of dest vnums 
    */
   if( check_vnums( ch, area, r_area ) )
      return;

   /*
    * another sanity check :) 
    */
   if( r_area == NULL || ( r_area->r_obj == NULL && r_area->r_mob == NULL && r_area->r_room == NULL ) )
   {
      ch->print( "No changes to make.\r\n" );
      deleteptr( r_area );
      return;
   }

   /*
    * ok, do it! 
    */
   ch->pagerf( "Renumbering area '%s' to new base %d, filling gaps: %s\r\n", area->filename, new_base, fill_gaps ? "yes" : "no" );
   renumber_area( ch, area, r_area, verbose );
   ch->pager( "Done.\r\n" );

   if( area->hi_vnum > sysdata->maxvnum )
   {
      sysdata->maxvnum = area->hi_vnum + 1;
      save_sysdata(  );
      ch->pagerf( "Maximum vnum was raised to %d by this operation.\r\n", sysdata->maxvnum );
   }

   /*
    * clean up and goodbye 
    */
   if( r_area->r_room != NULL )
      free_renumber_data( r_area->r_room );
   if( r_area->r_obj != NULL )
      free_renumber_data( r_area->r_obj );
   if( r_area->r_mob != NULL )
      free_renumber_data( r_area->r_mob );
   deleteptr( r_area );
}
