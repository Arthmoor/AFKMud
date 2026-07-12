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

constexpr int NOT_FOUND = -1;

enum { REN_ROOM, REN_OBJ, REN_MOB };

struct renumber_data
{
   int old_vnum = 0;
   int new_vnum = 0;
};

struct renumber_areas
{
   std::vector<renumber_data> r_obj;
   std::vector<renumber_data> r_mob;
   std::vector<renumber_data> r_room;
   int low_obj = 0, hi_obj = 0;
   int low_mob = 0, hi_mob = 0;
   int low_room = 0, hi_room = 0;
};

/* from db.cpp */
void save_sysdata(  );

/* returns the new vnum using std::ranges */
int find_translation( int vnum, const std::vector<renumber_data>& r_data )
{
   auto it = std::ranges::find_if( r_data, [vnum](const auto& r) { return r.old_vnum == vnum; });

   if( it != r_data.end() )
      return it->new_vnum;

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

      for( auto* pexit: room->exits )
      {
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
                  ch->pager_fmt( "...    fixing reverse exit in area {}.\r\n", pexit->to_room->area->filename );
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

      if( area->continent )
      {
         for( auto* mexit : area->continent->exits )
         {
            new_vnum = find_translation( mexit->vnum, r_area->r_room );
            if( new_vnum != NOT_FOUND )
            {
               mexit->vnum = new_vnum;
               ch->pager( "...    fixing overland exit to area.\r\n" );
            }
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
   const char *action_table[] = { "Mm1r3", "Oo1r3", "Ho1", "Po2o4", "Go1", "Eo1", "Dr1", "Rr1", "Wo8", "Zr7", nullptr };
   const char *p;
   const std::vector<renumber_data> *r_table = nullptr;
   int *parg, new_vnum, i;

   /*
    * T is a special case
    */
   if( reset->command == 'T' )
   {
      if( IS_SET( reset->arg1, TRAP_ROOM ) )
         r_table = &(r_data->r_room);
      else if( IS_SET( reset->arg1, TRAP_OBJ ) )
         r_table = &(r_data->r_obj);
      else
      {
         bug( "{}: Invalid 'T' reset found.", __func__ );
         return;
      }

      new_vnum = find_translation( reset->arg4, *r_table );
      if( new_vnum != NOT_FOUND )
         reset->arg4 = new_vnum;
      return;
   }

   for( i = 0; action_table[i] != nullptr; ++i )
   {
      if( reset->command == action_table[i][0] )
      {
         p = action_table[i] + 1;
         while( *p )
         {
            if( *p == 'm' )
               r_table = &(r_data->r_mob);
            else if( *p == 'o' )
               r_table = &(r_data->r_obj);
            else if( *p == 'r' )
               r_table = &(r_data->r_room);
            else
            {
               bug( "{}: Invalid action found in action table.", __func__ );
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
               bug( "{}: Invalid argument number found in action table.", __func__ );
               ++p;
               continue;
            }
            ++p;

            new_vnum = find_translation( *parg, *r_table );
            if( new_vnum != NOT_FOUND )
               *parg = new_vnum;
         }
         return;
      }
   }

   if( action_table[i] == nullptr )
      bug( "{}: Invalid reset '{}' found.", __func__, reset->command );
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
               ch->pager_fmt( "...   container {}; fixing objval2 (key vnum) {} -> {}\r\n", i, obj->value[2], new_vnum );
            obj->value[2] = new_vnum;
         }
         else if( verbose )
            ch->pager_fmt( "...    container {}; no need to fix.\r\n", i );
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
                  ch->pager_fmt( "...    lever {}: fixing source room ({} -> {})\r\n", i, obj->value[1], new_vnum );
               obj->value[1] = new_vnum;
            }
            if( IS_SET( obj->value[0], TRIG_DOOR ) && IS_SET( obj->value[0], TRIG_PASSAGE ) )
            {
               new_vnum = find_translation( obj->value[2], r_area->r_room );
               if( new_vnum != NOT_FOUND )
               {
                  if( verbose )
                     ch->pager_fmt( "...    lever {}: fixing dest room (passage) ({} -> {})\r\n", i, obj->value[2], new_vnum );
                  obj->value[2] = new_vnum;
               }
            }
         }
      }
   }
}

void warn_in_prog( char_data * ch, int low, int high, std::string_view where, int vnum, mud_prog_data * mprog, renumber_areas * r_area )
{
   std::string_view text{mprog->comlist};

   while( !text.empty() )
   {
      auto pos = text.find_first_of( "0123456789" );
      if( pos == std::string_view::npos )
         break;

      text.remove_prefix( pos );

      std::string_view::size_type end = 0;
      while( end < text.size() && std::isdigit( static_cast<unsigned char>( text[end] ) ) )
         ++end;

      std::string_view num_str = text.substr( 0, end );
      int num = 0;

      auto [ptr, ec] = std::from_chars( num_str.data(), num_str.data() + num_str.size(), num );

      if( ec == std::errc{} )
      {
         if( num >= low && num <= high )
         {
            ch->pager_fmt(
               "Warning! {} prog in {} vnum {} might contain a reference to {}.\r\n"
               "(Translation: Room {}, Obj {}, Mob {})\r\n",
                          mprog_type_to_name(mprog->type), where, vnum, num,
                          find_translation( num, r_area->r_room ),
                          find_translation( num, r_area->r_obj ),
                          find_translation( num, r_area->r_mob ) );
         }
      }
      text.remove_prefix( end );
   }
}

void warn_progs( char_data * ch, int low, int high, area_data * area, renumber_areas * r_area )
{
   room_index *room;
   obj_index *obj;
   mob_index *mob;
   mud_prog_data *mprog;
   std::list<mud_prog_data *>::iterator mpg;
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

/*
 * This is the function that actually does the renumbering of "area" according
 * to the renumber data in "r_area". "ch" is to show messages.
 */
/* This is the function that actually does the renumbering of "area" according
 * to the renumber data in "r_area". "ch" is to show messages.
 */
void renumber_area( char_data * ch, area_data * area, renumber_areas * r_area, bool verbose )
{
   room_index *room;
   mob_index *mob;
   obj_index *obj;
   std::list<mob_index *> mob_list;
   std::list<obj_index *> obj_list;
   std::list<room_index *> room_list;

   int high = area->hi_vnum, low = area->low_vnum;

   ch->pager( "(Room) Renumbering...\r\n" );

   /*
    * What we do here is, for each list (room/obj/mob) first we
    * take each element out of the hash array, change the vnum,
    * and move it to our own list. after everything's moved out
    * we put it in again. this is to avoid problems in situations
    * where where room A is being moved to position B, but there's
    * already a room B which is also being moved to position C.
    * a straightforward approach would result in us moving A to
    * position B first, and then again to position C, and room
    * B being lost inside the hash array, still there, but not
    * findable (its "covered" by A because they'd have the same vnum).
    */
   room_list.clear();
   for( const auto& r_data : r_area->r_room )
   {
      if( verbose )
         ch->pager_fmt( "(Room) {} -> {}\r\n", r_data.old_vnum, r_data.new_vnum );

      room = get_room_index( r_data.old_vnum );
      if( !room )
      {
         bug( "{}: nullptr room {}", __func__, r_data.old_vnum );
         continue;
      }

      /*
       * remove it from the hash list
       */
      std::map<int, room_index *>::iterator mroom;
      if( ( mroom = room_index_table.find( r_data.old_vnum ) ) != room_index_table.end( ) )
         room_index_table.erase( mroom );

      /*
       * change the vnum
       */
      room->vnum = r_data.new_vnum;

      /*
       * move it to the temporary list
       */
      room_list.push_back( room );
   }

   /*
    * now move everything back into the hash array
    */
   std::list<room_index *>::iterator iroom;
   for( iroom = room_list.begin( ); iroom != room_list.end( ); ++iroom )
   {
      room = *iroom;

      /*
       * add it to the hash list again
       */
      room_index_table.insert( std::map<int, room_index *>::value_type( room->vnum, room ) );
   }

   /*
    * if nothing was moved, don't change this
    */
   if( !r_area->r_room.empty() )
   {
      area->low_vnum = r_area->low_room;
      area->hi_vnum = r_area->hi_room;
   }

   mob_list.clear( );
   ch->pager( "(Mobs) Renumbering...\r\n" );
   for( const auto& r_data : r_area->r_mob )
   {
      if( verbose )
         ch->pager_fmt( "(Mobs) {} -> {}\r\n", r_data.old_vnum, r_data.new_vnum );

      mob = get_mob_index( r_data.old_vnum );
      if( !mob )
      {
         bug( "{}: nullptr mob {}", __func__, r_data.old_vnum );
         continue;
      }

      /*
       * fix references to this mob from shops while renumbering this mob
       */
      if( mob->pShop )
      {
         if( verbose )
            ch->pager_fmt( "(Mobs) Fixing shop for mob {} -> {}\r\n", r_data.old_vnum, r_data.new_vnum );
         mob->pShop->keeper = r_data.new_vnum;
      }
      if( mob->rShop )
      {
         if( verbose )
            ch->pager_fmt( "(Mobs) Fixing repair shop for mob {} -> {}\r\n", r_data.old_vnum, r_data.new_vnum );
         mob->rShop->keeper = r_data.new_vnum;
      }

      /*
       * remove it from the hash list
       */
      std::map<int, mob_index *>::iterator mmob;
      if( ( mmob = mob_index_table.find( r_data.old_vnum ) ) != mob_index_table.end( ) )
         mob_index_table.erase( mmob );

      /*
       * change the vnum
       */
      mob->vnum = r_data.new_vnum;

      /*
       * move to private list
       */
      mob_list.push_back( mob );
   }

   std::list<mob_index *>::iterator imob;
   for( imob = mob_list.begin( ); imob != mob_list.end( ); ++imob )
   {
      mob = *imob;

      /*
       * add it to the hash list again
       */
      mob_index_table.insert( std::map<int, mob_index *>::value_type( mob->vnum, mob ) );
   }

   if( !r_area->r_mob.empty() )
   {
      if( r_area->low_mob < r_area->low_room )
         area->low_vnum = r_area->low_mob;
      if( r_area->hi_mob > r_area->hi_room )
         area->hi_vnum = r_area->hi_mob;
   }

   ch->pager( "(Objs) Renumbering...\r\n" );
   obj_list.clear();
   for( const auto& r_data : r_area->r_obj )
   {
      if( verbose )
         ch->pager_fmt( "(Objs) {} -> {}\r\n", r_data.old_vnum, r_data.new_vnum );
      obj = get_obj_index( r_data.old_vnum );
      if( !obj )
      {
         bug( "{}: nullptr obj {}", __func__, r_data.old_vnum );
         continue;
      }

      /*
       * remove it from the hash list
       */
      std::map<int, obj_index *>::iterator mobj;
      if( ( mobj = obj_index_table.find( r_data.old_vnum ) ) != obj_index_table.end( ) )
         obj_index_table.erase( mobj );

      /*
       * change the vnum
       */
      obj->vnum = r_data.new_vnum;

      /*
       * to our list
       */
      obj_list.push_back( obj );
   }

   std::list<obj_index *>::iterator iobj;
   for( iobj = obj_list.begin( ); iobj != obj_list.end( ); ++iobj )
   {
      obj = *iobj;

      /*
       * add it to the hash list again
       */
      obj_index_table.insert( std::map<int, obj_index *>::value_type( obj->vnum, obj ) );
   }

   if( !r_area->r_obj.empty() )
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

   for( iroom = area->rooms.begin( ); iroom != area->rooms.end( ); ++iroom )
   {
      room = *iroom;

      for( auto* preset : room->resets )
      {
         translate_reset( preset, r_area );

         for( auto* treset: preset->resets )
            translate_reset( treset, r_area );
      }
   }

   if( verbose )
   {
      ch->pager( "Searching progs for references to renumbered vnums...\r\n" );
      warn_progs( ch, low, high, area, r_area );
   }
}

/* this function builds a vector of renumber data for a type */
std::vector<renumber_data> gather_one_list( short type, int low, int high, int new_base, bool fill_gaps, int *max_vnum )
{
   std::vector<renumber_data> r_data;
   bool found;
   int i, highest, cur_vnum;

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
            if( get_room_index( i ) != nullptr )
               found = true;
         break;
         case REN_OBJ:
            if( get_obj_index( i ) != nullptr )
               found = true;
         break;
         case REN_MOB:
            if( get_mob_index( i ) != nullptr )
               found = true;
         break;
      }

      if( found )
      {
         if( cur_vnum > highest )
            highest = cur_vnum;
         if( cur_vnum != i )
            r_data.push_back( { i, cur_vnum } );

         ++cur_vnum;
      }
      else if( !fill_gaps )
         ++cur_vnum;
   }
   *max_vnum = highest;
   return r_data;
}

/* this function actually gathers all the renumber data for an area */
renumber_areas *gather_renumber_data( area_data * area, int new_base, bool fill_gaps )
{
   int max;

   auto r_area = std::make_unique<renumber_areas>();

   r_area->r_mob = gather_one_list( REN_MOB, area->low_vnum, area->hi_vnum, new_base, fill_gaps, &max );
   r_area->low_mob = new_base;
   r_area->hi_mob = max;

   r_area->r_obj = gather_one_list( REN_OBJ, area->low_vnum, area->hi_vnum, new_base, fill_gaps, &max );
   r_area->low_obj = new_base;
   r_area->hi_obj = max;

   r_area->r_room = gather_one_list( REN_ROOM, area->low_vnum, area->hi_vnum, new_base, fill_gaps, &max );
   r_area->low_room = new_base;
   r_area->hi_room = max;

   return r_area.release();
}

bool check_vnums( char_data * ch, area_data * tarea, renumber_areas * r_area )
{
   if( !r_area )
   {
      bug( "{}: nullptr r_area!", __func__ );
      return true;
   }

   /*
    * this function assumes all the lows are always gonna be lower or equal to all the highs .. 
    */
   int high = umax( r_area->hi_room, umax( r_area->hi_obj, r_area->hi_mob ) );
   int low = umin( r_area->low_room, umin( r_area->low_obj, r_area->low_mob ) );

   if( high > sysdata->maxvnum )
   {
      ch->print_fmt( "This operation would raise the maximum allowed vnum beyond {}.\r\n", sysdata->maxvnum );
      ch->print( "Pick a lower base, or have sysdata->maxvnum raised.\r\n" );
      return true;
   }

   /*
    * in do_check_vnums they use first_bsort, first_asort but.. i dunno.. 
    */
   for( auto* area : arealist )
   {
      if( tarea == area )
         continue;

      if( !( high < area->low_vnum || low > area->hi_vnum ) )
      {
         ch->print_fmt( "This operation would overwrite area {}! Use checkvnums first.\r\n", area->filename );
         return true;
      }
   }
   return false;
}

CMDF( do_renumber )
{
   renumber_areas *r_area;
   area_data *area;
   std::string arg1;
   int new_base;
   bool fill_gaps, verbose;

   if( ch->isnpc( ) )
   {
      ch->print( "Yeah, right.\r\n" );
      return;
   }

   /*
    * parse the first two parameters
    * first, area
    */
   argument = one_argument( argument, arg1 );
   if( arg1.empty( ) )
   {
      ch->print( "What area do you want to renumber?\r\n" );
      return;
   }

   area = find_area( arg1 );
   if( area == nullptr )
   {
      ch->print_fmt( "No such area '{}'.\r\n", arg1 );
      return;
   }

   /*
    * and new vnum base
    */
   argument = one_argument( argument, arg1 );
   if( arg1.empty( ) )
   {
      ch->print( "What will be the new vnum base for this area?\r\n" );
      return;
   }

   if( !is_number( arg1 ) )
   {
      ch->print_fmt( "Sorry, '{}' is not a valid vnum base number!\r\n", arg1 );
      return;
   }

   new_base = std::stoi( arg1 );

   /*
    * parse the flags
    */
   fill_gaps = false;
   verbose = false;
   for( ;; )
   {
      argument = one_argument( argument, arg1 );
      if( arg1.empty( ) )
         break;
      else if( !str_prefix( arg1, "fillgaps" ) )
         fill_gaps = true;
      else if( !str_prefix( arg1, "verbose" ) )
         verbose = true;
      else
      {
         ch->print_fmt( "Invalid flag '{}'.\r\n", arg1 );
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
         ch->print_fmt( "You can't renumber that area ('{}').\r\n", area->filename );
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
         r_area->low_obj < ch->pcdata->low_vnum || r_area->hi_obj > ch->pcdata->hi_vnum ||
         r_area->low_mob < ch->pcdata->low_vnum || r_area->hi_mob > ch->pcdata->hi_vnum )
      {
         ch->print( "The renumbered area would be outside your assigned vnum range.\r\n" );

         // Bugfix - Memory leak if r_area was valid.
         deleteptr( r_area );
         return;
      }
   }

   /*
    * OOO! A new sanity check :) - Samson 10-7-02
    */
   if( new_base >= sysdata->maxvnum )
   {
      ch->print_fmt( "{} is beyond the maximum allowed vnum of {}\r\n", new_base, sysdata->maxvnum );

      // Bugfix - Memory leak if r_area was valid.
      deleteptr( r_area );
      return;
   }

   /*
    * no overwriting of dest vnums
    */
   if( check_vnums( ch, area, r_area ) )
   {
      // Bugfix - Memory leak if r_area was valid.
      deleteptr( r_area );
      return;
   }

   /*
    * another sanity check :)
    * Updated to use .empty() instead of checking for nullptr on raw pointers.
    */
   if( !r_area || ( r_area->r_obj.empty() && r_area->r_mob.empty() && r_area->r_room.empty() ) )
   {
      ch->print( "No changes to make.\r\n" );
      deleteptr( r_area );
      return;
   }

   /*
    * ok, do it!
    */
   ch->pager_fmt( "Renumbering area '{}' to new base {}, filling gaps: {}\r\n", area->filename, new_base, fill_gaps ? "yes" : "no" );
   renumber_area( ch, area, r_area, verbose );
   ch->pager( "Done.\r\n" );

   if( area->hi_vnum > sysdata->maxvnum )
   {
      sysdata->maxvnum = area->hi_vnum + 1;
      save_sysdata( );
      ch->pager_fmt( "Maximum vnum was raised to {} by this operation.\r\n", sysdata->maxvnum );
   }

   /*
    * clean up and goodbye
    */
   deleteptr( r_area );
}
