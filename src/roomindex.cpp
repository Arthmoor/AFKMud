/****************************************************************************
 *                   ^     +----- |  / ^     ^ |     | +-\                  *
 *                  / \    |      | /  |\   /| |     | |  \                 *
 *                 /   \   +---   |<   | \ / | |     | |  |                 *
 *                /-----\  |      | \  |  v  | |     | |  /                 *
 *               /       \ |      |  \ |     | +-----+ +-/                  *
 ****************************************************************************
 * AFKMud Copyright 1997-2008 by Roger Libiez (Samson),                     *
 * Levi Beckerson (Whir), Michael Ward (Tarl), Erik Wolfe (Dwip),           *
 * Cameron Carroll (Cam), Cyberfox, Karangi, Rathian, Raine,                *
 * Xorith, and Adjani.                                                      *
 * All Rights Reserved.                                                     *
 * Registered with the United States Copyright Office: TX 5-877-286         *
 *                                                                          *
 * External contributions from Remcon, Quixadhal, Zarius, and many others.  *
 *                                                                          *
 * Original SMAUG 1.4a written by Thoric (Derek Snider) with Altrag,        *
 * Blodkai, Haus, Narn, Scryn, Swordbearer, Tricops, Gorog, Rennard,        *
 * Grishnakh, Fireblade, and Nivek.                                         *
 *                                                                          *
 * Original MERC 2.1 code by Hatchet, Furey, and Kahn.                      *
 *                                                                          *
 * Original DikuMUD code by: Hans Staerfeldt, Katja Nyboe, Tom Madsen,      *
 * Michael Seifert, and Sebastian Hammer.                                   *
 ****************************************************************************
 *                         Room Index Support Functions                     *
 ****************************************************************************/

#include "mud.h"
#include "area.h"
#include "descriptor.h"
#include "mobindex.h"
#include "mud_prog.h"
#include "objindex.h"
#include "overland.h"
#include "roomindex.h"

room_index *room_index_hash[MAX_KEY_HASH];
list<teleport_data*> teleportlist;

extern int top_exit;
extern int top_reset;
extern int top_affect;

reset_data *make_reset( char, int, int, int, short, short, short, short, short, short, short, short );
void update_room_reset( char_data *, bool );
void delete_reset( reset_data * );
void name_generator( char * );
void pick_name( char *name, char * );
void boot_log( const char *, ... );
void fix_exits( );

obj_data *generate_random( reset_data *, char_data * );

char *const dir_name[] = {
   "north", "east", "south", "west", "up", "down",
   "northeast", "northwest", "southeast", "southwest", "somewhere"
};

char *const short_dirname[] = {
   "n", "e", "s", "w", "u", "d", "ne", "nw", "se", "sw", "?"
};

const int trap_door[] = {
   TRAP_N, TRAP_E, TRAP_S, TRAP_W, TRAP_U, TRAP_D,
   TRAP_NE, TRAP_NW, TRAP_SE, TRAP_SW
};

const short rev_dir[] = {
   2, 3, 0, 1, 5, 4, 9, 8, 7, 6, 10
};

void free_teleports( void )
{
   list<teleport_data*>::iterator tele;

   for( tele = teleportlist.begin(); tele != teleportlist.end(); )
   {
      teleport_data *teleport = (*tele);
      ++tele;

      teleportlist.remove( teleport );
      deleteptr( teleport );
   }
}

reset_data::reset_data()
{
   init_memory( &command, &arg6, sizeof( arg6 ) );
   resets.clear();
}

room_index::~room_index(  )
{
   area->rooms.remove( this );

   list<char_data*>::iterator ich;
   for( ich = people.begin(); ich != people.end(); )
   {
      char_data *ch = *ich;
      ++ich;

      if( !ch->isnpc(  ) )
      {
         room_index *limbo = get_room_index( ROOM_VNUM_LIMBO );

         ch->from_room(  );
         if( !ch->to_room( limbo ) )
            log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
      }
      else
         ch->extract( true );
   }
   people.clear();

   for( ich = pclist.begin(); ich != pclist.end(); ++ich )
   {
      char_data *ch = *ich;

      if( ch->was_in_room == this )
         ch->was_in_room = ch->in_room;
      if( ch->substate == SUB_ROOM_DESC && ch->pcdata->dest_buf == this )
      {
         ch->print( "The room is no more.\r\n" );
         ch->stop_editing( );
         ch->substate = SUB_NONE;
         ch->pcdata->dest_buf = NULL;
      }
      else if( ch->substate == SUB_ROOM_EXTRA && ch->pcdata->dest_buf )
      {
         list<extra_descr_data*>::iterator ex;

         for( ex = extradesc.begin(); ex != extradesc.end(); ++ex )
         {
            extra_descr_data *ed = *ex;

            if( ed == ch->pcdata->dest_buf )
            {
               ch->print( "The room is no more.\r\n" );
               ch->stop_editing( );
               ch->substate = SUB_NONE;
               ch->pcdata->dest_buf = NULL;
               break;
            }
         }
      }
   }

   list<obj_data*>::iterator iobj;
   for( iobj = objects.begin(); iobj != objects.end(); )
   {
      obj_data *obj = (*iobj);
      ++iobj;

      obj->extract();
   }
   objects.clear();

   wipe_resets();

   list<extra_descr_data*>::iterator ed;
   for( ed = extradesc.begin(); ed != extradesc.end(); )
   {
      extra_descr_data *desc = *ed;
      ++ed;

      extradesc.remove( desc );
      deleteptr( desc );
      --top_ed;
   }
   extradesc.clear();

   list<exit_data*>::iterator ex;
   if( !mud_down )
   {
      for( ex = exits.begin(); ex != exits.end(); ++ex )
      {
         exit_data *pexit = (*ex);
         exit_data *aexit;
         area_data *pArea = pexit->to_room->area;

         if( ( ( aexit = pexit->rexit ) != NULL ) && aexit != pexit )
         {
            pexit->to_room->extract_exit( aexit );

            if( pArea != area )
            {
               char filename[256];

               if( !pArea->flags.test( AFLAG_PROTOTYPE ) )
                  mudstrlcpy( filename, pArea->filename, 256 );
               else
                  snprintf( filename, 256, "%s%s", BUILD_DIR, pArea->filename );
               pArea->fold( filename, false );
            }
         }
      }
   }

   list<affect_data*>::iterator paf;
   for( paf = affects.begin(); paf != affects.end(); )
   {
      affect_data *af = *paf;
      ++paf;

      affects.remove( af );
      deleteptr( af );
   }
   affects.clear();

   for( paf = indexaffects.begin(); paf != indexaffects.end(); )
   {
      affect_data *af = *paf;
      ++paf;

      indexaffects.remove( af );
      deleteptr( af );
   }
   indexaffects.clear();

   for( ex = exits.begin(); ex != exits.end(); )
   {
      exit_data *pexit = *ex;
      ++ex;

      extract_exit( pexit );
   }
   exits.clear();

   list<mprog_act_list*>::iterator pd;
   for( pd = mpact.begin(); pd != mpact.end(); )
   {
      mprog_act_list *rpact = (*pd);
      ++pd;

      mpact.remove( rpact );
      deleteptr( rpact );
   }
   mpact.clear();

   list<mud_prog_data*>::iterator mpg;
   for( mpg = mudprogs.begin(); mpg != mudprogs.end(); )
   {
      mud_prog_data *mprog = (*mpg);
      ++mpg;

      mudprogs.remove( mprog );
      deleteptr( mprog );
   }
   mudprogs.clear();

   STRFREE( name );
   DISPOSE( roomdesc );
   DISPOSE( nitedesc );

   int hash = vnum % MAX_KEY_HASH;
   if( this == room_index_hash[hash] )
      room_index_hash[hash] = next;
   else
   {
      room_index *prev;

      for( prev = room_index_hash[hash]; prev; prev = prev->next )
         if( prev->next == this )
            break;
      if( prev )
         prev->next = next;
      else
         bug( "%s: room %d not in hash bucket %d.", __FUNCTION__, vnum, hash );
   }
   --top_room;
}

room_index::room_index(  )
{
   init_memory( &next, &mpscriptpos, sizeof( mpscriptpos ) );
   people.clear();
   objects.clear();
}

/*
 * clean out a room ( leave list pointers intact ) - Thoric
 */
void room_index::clean_room(  )
{
   mud_prog_data *mprog;
   list<mud_prog_data*>::iterator mpg;
   list<extra_descr_data*>::iterator ed;
   list<exit_data*>::iterator iexit;

   DISPOSE( roomdesc );
   DISPOSE( nitedesc );
   STRFREE( name );
   for( ed = extradesc.begin(); ed != extradesc.end(); )
   {
      extra_descr_data *desc = (*ed);
      ++ed;

      extradesc.remove( desc );
      deleteptr( desc );
      --top_ed;
   }
   extradesc.clear();

   for( mpg = mudprogs.begin(); mpg != mudprogs.end(); )
   {
      mprog = (*mpg);
      ++mpg;

      mudprogs.remove( mprog );
      deleteptr( mprog );
   }
   mudprogs.clear();

   for( iexit = exits.begin(); iexit != exits.end(); )
   {
      exit_data *pexit = *iexit;

      extract_exit( pexit );
      --top_exit;
   }
   exits.clear();

   wipe_resets();
   flags.reset(  );
   sector_type = 0;
   light = 0;
}

/*
 * (prelude...) This is going to be fun... NOT!
 * (conclusion) QSort is f*cked!
 */
int exit_comp( exit_data ** xit1, exit_data ** xit2 )
{
   int d1, d2;

   d1 = ( *xit1 )->vdir;
   d2 = ( *xit2 )->vdir;

   if( d1 < d2 )
      return -1;
   if( d1 > d2 )
      return 1;
   return 0;
}

void room_index::randomize_exits( short maxdir )
{
   list<exit_data*>::iterator iexit;
   int nexits, /* maxd, */ d1, count, door; /* Maxd unused */
   int vdirs[MAX_REXITS];

   nexits = 0;
   for( iexit = exits.begin(); iexit != exits.end(); ++iexit )
   {
      exit_data *pexit = (*iexit);

      vdirs[nexits++] = pexit->vdir;
   }

   for( int d0 = 0; d0 < nexits; ++d0 )
   {
      if( vdirs[d0] > maxdir )
         continue;
      count = 0;
      while( vdirs[( d1 = number_range( d0, nexits - 1 ) )] > maxdir || ++count < 5 );
      if( vdirs[d1] > maxdir )
         continue;
      door = vdirs[d0];
      vdirs[d0] = vdirs[d1];
      vdirs[d1] = door;
   }
   count = 0;
   for( iexit = exits.begin(); iexit != exits.end(); ++iexit )
   {
      exit_data *pexit = (*iexit);
      pexit->vdir = vdirs[count++];
   }
}

exit_data::exit_data()
{
   init_memory( &rexit, &my, sizeof( my ) );
}

exit_data::~exit_data()
{
   if( rexit )
      rexit->rexit = NULL;
   STRFREE( keyword );
   STRFREE( exitdesc );
}

/*
 * Creates a simple exit with no fields filled but rvnum and optionally
 * to_room and vnum. - Thoric
 */
exit_data *room_index::make_exit( room_index * to_room, short door )
{
   exit_data *pexit = new exit_data;
   
   pexit->vdir = door;
   pexit->rvnum = vnum;
   pexit->to_room = to_room;
   pexit->flags.reset(  );
   pexit->mx = 0;
   pexit->my = 0;

   if( to_room )
   {
      pexit->vnum = to_room->vnum;
      exit_data *texit = get_exit_to( rev_dir[door], vnum );
      if( texit ) /* assign reverse exit pointers */
      {
         texit->rexit = pexit;
         pexit->rexit = texit;
      }
   }

   list<exit_data*>::iterator iexit;
   for( iexit = exits.begin(); iexit != exits.end(); ++iexit )
   {
      exit_data *texit = (*iexit);

      if( door < texit->vdir )
      {
         exits.insert( iexit, pexit );
         ++top_exit;
         return pexit;
      }
   }
   exits.push_back( pexit );
   ++top_exit;
   return pexit;
}

/*
 * Function to get the equivelant exit of DIR 0-MAXDIR out of linked list.
 * Made to allow old-style diku-merc exit functions to work. - Thoric
 */
exit_data *room_index::get_exit( short dir )
{
   list<exit_data*>::iterator xit;

   for( xit = exits.begin(); xit != exits.end(); ++xit )
   {
      exit_data *pexit = (*xit);

      if( pexit->vdir == dir )
         return pexit;
   }
   return NULL;
}

/*
 * Function to get the nth exit of a room - Thoric
 */
exit_data *room_index::get_exit_num( short count )
{
   list<exit_data*>::iterator xit;
   int cnt;

   for( cnt = 0, xit = exits.begin(); xit != exits.end(); ++xit )
   {
      exit_data *pexit = (*xit);

      if( ++cnt == count )
         return pexit;
   }
   return NULL;
}

/*
 * Function to get an exit, leading the the specified room
 */
exit_data *room_index::get_exit_to( short dir, int evnum )
{
   list<exit_data*>::iterator xit;

   for( xit = exits.begin(); xit != exits.end(); ++xit )
   {
      exit_data *pexit = (*xit);

      if( pexit->vdir == dir && pexit->vnum == evnum )
         return pexit;
   }
   return NULL;
}

/*
 * Remove an exit from a room - Thoric
 */
void room_index::extract_exit( exit_data *pexit )
{
   exits.remove( pexit );
   deleteptr( pexit );
}

void room_index::wipe_resets( )
{
   reset_data *pReset;
   list<reset_data*>::iterator rst;

   for( rst = resets.begin(); rst != resets.end(); )
   {
      pReset = (*rst);
      ++rst;

      resets.remove( pReset );
      delete_reset( pReset );
   }
   resets.clear();
   return;
}

/*
 * Creat a new room (for online building) - Thoric
 */
room_index *make_room( int vnum, area_data *area )
{
   room_index *pRoomIndex;
   int iHash;

   pRoomIndex = new room_index;
   pRoomIndex->exits.clear();
   pRoomIndex->extradesc.clear();
   pRoomIndex->resets.clear();
   pRoomIndex->area = area;
   pRoomIndex->vnum = vnum;
   pRoomIndex->winter_sector = -1;
   pRoomIndex->name = STRALLOC( "Engulfed in a Swirling Mass of Void Space" );
   pRoomIndex->flags.reset(  );
   pRoomIndex->flags.set( ROOM_PROTOTYPE );
   pRoomIndex->sector_type = 0;
   pRoomIndex->baselight = 0;
   pRoomIndex->light = 0;

   iHash = vnum % MAX_KEY_HASH;
   pRoomIndex->next = room_index_hash[iHash];
   room_index_hash[iHash] = pRoomIndex;
   area->rooms.push_back( pRoomIndex );
   ++top_room;

   return pRoomIndex;
}

/* This procedure is responsible for reading any in_file ROOMprograms.
 */
void room_index::rprog_read_programs( FILE * fp )
{
   mud_prog_data *mprg;
   char letter;
   const char *word;

   for( ; ; )
   {
      letter = fread_letter( fp );

      if( letter == '|' )
         return;

      if( letter != '>' )
      {
         bug( "%s: vnum %d MUDPROG char", __FUNCTION__, vnum );
         exit( 1 );
      }
      mprg = new mud_prog_data;
      mudprogs.push_back( mprg );

      word = fread_word( fp );
      mprg->type = mprog_name_to_type( word );

      switch( mprg->type )
      {
         case ERROR_PROG:
            bug( "%s: vnum %d MUDPROG type.", __FUNCTION__, vnum );
            exit( 1 );

         case IN_FILE_PROG:
            mprg->arglist = fread_string( fp );
            mprg->fileprog = false;
            mprog_file_read( this, mprg->arglist );
            break;

         default:
            progtypes.set( mprg->type );
            mprg->fileprog = false;
            mprg->arglist = fread_string( fp );
            mprg->comlist = fread_string( fp );
            break;
      }
   }
   return;
}

/*
 * True if room is dark.
 */
bool room_index::is_dark( char_data *ch )
{
   if( !this )
   {
      char buf[MSL];

      if( ch->isnpc(  ) )
         snprintf( buf, MSL, "Mob #%d", ch->pIndexData->vnum );
      else
         snprintf( buf, MSL, "Player %s", ch->name );

      bug( "%s: NULL pRoomIndex. Occupant: %s", __FUNCTION__, ch->name );

      if( ch->char_died(  ) )
         log_printf( "%s was probably dead when this happened.", buf );

      return true;
   }

   if( light > 0 )
      return false;

   if( flags.test( ROOM_DARK ) )
      return true;

   if( sector_type == SECT_INDOORS || sector_type == SECT_CITY )
      return false;

   if( time_info.sunlight == SUN_SET || time_info.sunlight == SUN_DARK )
      return true;

   return false;
}

/*
 * True if room is private.
 */
bool room_index::is_private(  )
{
   if( !this )
   {
      bug( "%s: NULL pRoomIndex", __FUNCTION__ );
      return false;
   }

   int count = people.size();

   if( flags.test( ROOM_PRIVATE ) && count >= 2 )
      return true;

   if( flags.test( ROOM_SOLITARY ) && count >= 1 )
      return true;

   return false;
}

void room_index::olc_remove_affect( char_data *ch, bool indexaffect, char *argument )
{
   list<affect_data*>::iterator paf;
   short loc = atoi( argument );

   if( !argument || argument[0] == '\0' )
   {
      if( !indexaffect )
         ch->print( "Usage: redit rmaffect <affect#>\r\n" );
      else
         ch->print( "Usage: redit rmindexaffect <affect#>\r\n" );
      return;
   }

   if( loc < 1 )
   {
      ch->print( "Invalid number.\r\n" );
      return;
   }

   short count = 0;
   if( !indexaffect )
   {
      for( paf = affects.begin(); paf != affects.end(); )
      {
         affect_data *aff = *paf;
         ++paf;

         if( ++count == loc )
         {
            affects.remove( aff );
            deleteptr( aff );
            ch->print( "Room affect removed.\r\n" );
            --top_affect;
            return;
         }
      }
   }
   else
   {
      for( paf = indexaffects.begin(); paf != indexaffects.end(); )
      {
         affect_data *aff = *paf;
         ++paf;

         if( ++count == loc )
         {
            indexaffects.remove( aff );
            deleteptr( aff );
            ch->print( "Room index affect removed.\r\n" );
            --top_affect;
            return;
         }
      }
   }
   ch->print( "Room affect not found.\r\n" );
   return;
}

/*
 * Crash fix and name support by Shaddai 
 */
void room_index::olc_add_affect( char_data *ch, bool indexaffect, char *argument )
{
   affect_data *paf;
   char arg2[MIL];
   bitset < MAX_RIS_FLAG > risabit;
   int value = 0;
   short loc;
   bool found = false;

   risabit.reset(  );

   argument = one_argument( argument, arg2 );
   if( !arg2 || arg2[0] == '\0' || !argument || argument[0] == '\0' )
   {
      if( !indexaffect )
         ch->print( "Usage: redit affect <field> <value>\r\n" );
      else
         ch->print( "Usage: redit indexaffect <field> <value>\r\n" );
      return;
   }

   loc = get_atype( arg2 );
   if( loc < 1 )
   {
      ch->printf( "Unknown field: %s\r\n", arg2 );
      return;
   }

   if( loc == APPLY_AFFECT )
   {
      char arg3[MIL];

      argument = one_argument( argument, arg3 );
      if( loc == APPLY_AFFECT )
      {
         value = get_aflag( arg3 );

         if( value < 0 || value >= MAX_AFFECTED_BY )
            ch->printf( "Unknown affect: %s\r\n", arg3 );
         else
            found = true;
      }
   }
   else if( loc == APPLY_RESISTANT || loc == APPLY_IMMUNE || loc == APPLY_SUSCEPTIBLE || loc == APPLY_ABSORB )
   {
      char *risa = argument;
      char flag[MIL];

      while( risa[0] != '\0' )
      {
         risa = one_argument( risa, flag );
         value = get_risflag( flag );

         if( value < 0 || value >= MAX_RIS_FLAG )
            ch->printf( "Unknown flag: %s\r\n", flag );
         else
         {
            risabit.set( value );
            found = true;
         }
      }
   }
   else if( loc == APPLY_WEAPONSPELL
            || loc == APPLY_WEARSPELL
            || loc == APPLY_REMOVESPELL || loc == APPLY_STRIPSN || loc == APPLY_RECURRINGSPELL || loc == APPLY_EAT_SPELL )
   {
      value = skill_lookup( argument );

      if( !IS_VALID_SN( value ) )
         ch->printf( "Invalid spell: %s", argument );
      else
         found = true;
   }
   else
   {
      value = atoi( argument );
      found = true;
   }
   if( !found )
      return;

   paf = new affect_data;
   paf->type = -1;
   paf->duration = -1;
   paf->location = loc;
   paf->modifier = value;
   paf->rismod = risabit;
   paf->bit = 0;

   if( !indexaffect )
      affects.push_back( paf );
   else
      indexaffects.push_back( paf );
   ++top_affect;
   ch->print( "Room affect added.\r\n" );
   return;
}

void room_index::room_affect( affect_data * paf, bool fAdd )
{
   if( fAdd )
   {
      switch ( paf->location )
      {
         case APPLY_ROOMLIGHT:
            light += paf->modifier;
            break;

         default:
         case APPLY_TELEVNUM:
         case APPLY_TELEDELAY:
         case APPLY_ROOMFLAG:
         case APPLY_SECTORTYPE:
            break;
      }
   }
   else
   {
      switch ( paf->location )
      {
         case APPLY_ROOMLIGHT:
            light -= paf->modifier;
            break;

         default:
         case APPLY_TELEVNUM:
         case APPLY_TELEDELAY:
         case APPLY_ROOMFLAG:
         case APPLY_SECTORTYPE:
            break;
      }
   }
}

/*
 * Translates room virtual number to its room index struct.
 * Hash table lookup.
 */
room_index *get_room_index( int vnum )
{
   room_index *pRoomIndex;

   if( vnum < 0 )
      vnum = 0;

   for( pRoomIndex = room_index_hash[vnum % MAX_KEY_HASH]; pRoomIndex; pRoomIndex = pRoomIndex->next )
      if( pRoomIndex->vnum == vnum )
         return pRoomIndex;

   if( fBootDb )
      bug( "%s: bad vnum %d.", __FUNCTION__, vnum );

   return NULL;
}

void room_index::echo( char *argument )
{
   list<char_data*>::iterator ich;

   for( ich = people.begin(); ich != people.end(); ++ich )
   {
      char_data *victim = (*ich);

      if( victim->MXP_ON(  ) )
         victim->print( MXP_TAG_SECURE );
      victim->printf( "%s\r\n", argument );
   }
}

/*
 * Remove all resets from an area - Thoric
 */
void room_index::clean_resets(  )
{
   reset_data *pReset;
   list<reset_data*>::iterator rst;

   for( rst = resets.begin(); rst != resets.end(); )
   {
      pReset = (*rst);
      ++rst;

      delete_reset( pReset );
      --top_reset;
   }
   resets.clear();
}

int generate_itemlevel( area_data *pArea, obj_index *pObjIndex )
{
   int olevel;
   int min = UMAX( pArea->low_soft_range, 1 );
   int max = UMIN( pArea->hi_soft_range, min + 15 );

   if( pObjIndex->level > 0 )
      olevel = UMIN( pObjIndex->level, MAX_LEVEL );
   else
      switch ( pObjIndex->item_type )
      {
         default:
            olevel = 0;
            break;
         case ITEM_PILL:
            olevel = number_range( min, max );
            break;
         case ITEM_POTION:
            olevel = number_range( min, max );
            break;
         case ITEM_SCROLL:
            olevel = pObjIndex->value[0];
            break;
         case ITEM_WAND:
            olevel = number_range( min + 4, max + 1 );
            break;
         case ITEM_STAFF:
            olevel = number_range( min + 9, max + 5 );
            break;
         case ITEM_ARMOR:
            olevel = number_range( min + 4, max + 1 );
            break;
         case ITEM_WEAPON:
            olevel = number_range( min + 4, max + 1 );
            break;
      }
   return olevel;
}

/*
 * Count occurrences of an obj in a list.
 */
int count_obj_list( reset_data *pReset, obj_index *pObjIndex, list<obj_data*> source )
{
   list<obj_data*>::iterator iobj;
   int nMatch = 0;

   for( iobj = source.begin(); iobj != source.end(); ++iobj )
   {
      obj_data *obj = (*iobj);
      if( obj->pIndexData == pObjIndex )
      {
         if( pReset->command == 'M' || pReset->command == 'O' )
         {
            if( pReset->arg4 == obj->map && pReset->arg5 == obj->mx && pReset->arg6 == obj->my )
            {
               if( obj->count > 1 )
                  nMatch += obj->count;
               else
                  ++nMatch;
            }
         }
         else
         {
            if( obj->count > 1 )
               nMatch += obj->count;
            else
               ++nMatch;
         }
      }
   }
   return nMatch;
}

/*
 * Find some object with a given index data.
 * Used by area-reset 'P', 'T' and 'H' commands.
 */
obj_data *get_obj_type( obj_index * pObjIndex )
{
   list<obj_data*>::iterator iobj;

   for( iobj = objlist.begin(); iobj != objlist.end(); ++iobj )
   {
      obj_data *obj = (*iobj);

      if( obj->pIndexData == pObjIndex )
         return obj;
   }
   return NULL;
}

/* Find an object in a room so we can check it's dependents. Used by 'O' resets. */
obj_data *get_obj_room( obj_index *pObjIndex, room_index *pRoomIndex )
{
   list<obj_data*>::iterator iobj;

   for( iobj = pRoomIndex->objects.begin(); iobj != pRoomIndex->objects.end(); ++iobj )
   {
      obj_data *obj = (*iobj);

      if( obj->pIndexData == pObjIndex )
         return obj;
   }
   return NULL;
}

/*
 * Make a trap.
 */
obj_data *make_trap( int v0, int v1, int v2, int v3 )
{
   obj_data *trap;

   if( !( trap = get_obj_index( OBJ_VNUM_TRAP )->create_object( 1 ) ) )
   {
      log_printf( "create_object: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
      return NULL;
   }
   trap->timer = 0;
   trap->value[0] = v0;
   trap->value[1] = v1;
   trap->value[2] = v2;
   trap->value[3] = v3;
   return trap;
}

/*
 * Add a reset to a room
 */
reset_data *room_index::add_reset( char letter, int arg1, int arg2, int arg3, short arg4, short arg5, short arg6, short arg7, short arg8, short arg9, short arg10, short arg11 )
{
   reset_data *pReset;

   if( !this )
   {
      bug( "%s: NULL room!", __FUNCTION__ );
      return NULL;
   }

   letter = UPPER( letter );
   pReset = make_reset( letter, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11 );
   switch ( letter )
   {
      default:
         break;

      case 'M':
         last_mob_reset = pReset;
         break;

      case 'E':
      case 'G':
      case 'X':
      case 'Y':
         if( !last_mob_reset )
         {
            bug( "%s: Can't add '%c' reset to room: last_mob_reset is NULL.", __FUNCTION__, letter );
            return NULL;
         }
         last_obj_reset = pReset;
         last_mob_reset->resets.push_back( pReset );
         return pReset;

      case 'P':
      case 'W':
         if( !last_obj_reset )
         {
            bug( "%s: Can't add '%c' reset to room: last_obj_reset is NULL.", __FUNCTION__, letter );
            return NULL;
         }
         last_obj_reset->resets.push_back( pReset );
         return pReset;

      case 'O':
      case 'Z':
         last_obj_reset = pReset;
         break;

      case 'T':
         if( IS_SET( arg1, TRAP_OBJ ) )
         {
            last_obj_reset->resets.push_front( pReset );
            return pReset;
         }
         break;

      case 'H':
         last_obj_reset->resets.push_front( pReset );
         return pReset;
   }
   resets.push_back( pReset );
   return pReset;
}

/* Setup put nesting levels, regardless of whether or not the resets will
   actually reset, or if they're bugged. */
void room_index::renumber_put_resets(  )
{
   reset_data *lastobj = NULL;
   list<reset_data*>::iterator rst, dst;

   for( rst = resets.begin(); rst != resets.end(); ++rst )
   {
      reset_data *pReset = (*rst);

      switch( pReset->command )
      {
         default:
            break;

         case 'O':
            lastobj = pReset;
            for( dst = pReset->resets.begin(); dst != pReset->resets.end(); ++dst )
            {
               reset_data *tReset = (*dst);

               switch( tReset->command )
               {
                  default:
                     break;

                  case 'P':
                     if( tReset->arg4 == 0 )
                     {
                        if( !lastobj )
                           tReset->arg1 = 1000000;
                        else if( lastobj->command != 'P' || lastobj->arg4 > 0 )
                           tReset->arg1 = 0;
                        else
                           tReset->arg1 = lastobj->arg1 + 1;
                        lastobj = tReset;
                     }
                     break;
               }
            }
            break;
      }
   }
   return;
}

/*
 * Reset one room.
 */
void room_index::reset(  )
{
   map<int,obj_data*> nestmap;
   char_data *mob;
   obj_data *obj, *lastobj, *to_obj;
   room_index *pRoomIndex = NULL;
   mob_index *pMobIndex = NULL;
   obj_index *pObjIndex = NULL, *pObjToIndex;
   exit_data *pexit;
   char *filename = area->filename;
   int level = 0, num, lastnest, onreset = 0;

   mob = NULL;
   obj = NULL;
   lastobj = NULL;
   if( resets.empty() )
      return;
   level = 0;

   list<reset_data*>::iterator rst;
   for( rst = resets.begin(); rst != resets.end(); ++rst )
   {
      reset_data *pReset = (*rst);

      ++onreset;
      switch( pReset->command )
      {
         default:
            bug( "%s: %s: bad command %c.", __FUNCTION__, filename, pReset->command );
            break;

         case 'M':
         {
            // Failed percentage check, don't bother processing. Move along.
            if( number_percent() > pReset->arg7 )
               break;

            if( !( pMobIndex = get_mob_index( pReset->arg1 ) ) )
            {
               bug( "%s: %s: 'M': bad mob vnum %d.", __FUNCTION__, filename, pReset->arg1 );
               break;
            }
            if( !( pRoomIndex = get_room_index( pReset->arg3 ) ) )
            {
               bug( "%s: %s: 'M': bad room vnum %d.", __FUNCTION__, filename, pReset->arg3 );
               break;
            }
            if( !pReset->sreset )
            {
               mob = NULL;
               break;
            }
            mob = pMobIndex->create_mobile(  );
            if( pReset->arg4 != -1 && pReset->arg5 != -1 && pReset->arg6 != -1 )
            {
               mob->set_actflag( ACT_ONMAP );
               mob->map = pReset->arg4;
               mob->mx = pReset->arg5;
               mob->my = pReset->arg6;
            }

            room_index *pRoomPrev = get_room_index( pReset->arg3 - 1 );
            if( pRoomPrev && pRoomPrev->flags.test( ROOM_PET_SHOP ) )
               mob->set_actflag( ACT_PET );

            if( pRoomIndex->is_dark( mob ) )
               mob->set_aflag( AFF_INFRARED );
            mob->resetvnum = pRoomIndex->vnum;
            mob->resetnum = onreset;
            pReset->sreset = false;
            if( !mob->to_room( pRoomIndex ) )
               log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
            level = URANGE( 0, mob->level - 2, LEVEL_AVATAR );

            /*
             * Added by Tarl 4 Dec 02 so that if a mob is 'flagged' namegen in
             * his name, it will auto assign a random name to it. Similarly,
             * occurances of namegen in the long_descr and description will be
             * replaced with the name.
             */

            string namegenCheckString = mob->name;

            /*
             * Modified by Tarl 5 Dec 02 to add extra namegen options. ie, namegen_gr will pick a name
             * suitable for Graecian mobs.
             * 
             * To add more, edit the line below this, and add an if-check similar to the one starting
             * with if( namegenCheckString.find( "namegen_gr" ) != string::npos )
             *
             * And then Samson shows up and cleans up the code, kills off the memory leaks, and life was good.
             * Or something like that anyway. Merry Christmas 2005!
             */
            if( namegenCheckString.find( "namegen" ) != string::npos )
            {
               string genstring = "namegen";
               char nameg[MSL], mob_keywords[MSL];
               char file[256];
               bool namePicked = false;

               if( namegenCheckString.find( "namegen_gr" ) != string::npos )
               {
                  genstring = "namegen_gr";
                  namePicked = true;
                  if( mob->sex == SEX_FEMALE )
                     snprintf( file, 256, "%s%s", SYSTEM_DIR, "namegen_gr_female.txt" );
                  else
                     snprintf( file, 256, "%s%s", SYSTEM_DIR, "namegen_gr_other.txt" );
               }
               else if( namegenCheckString.find( "namegen_ven" ) != string::npos )
               {
                  genstring = "namegen_ven";
                  namePicked = true;
                  if( mob->sex == SEX_FEMALE )
                     snprintf( file, 256, "%s%s", SYSTEM_DIR, "namegen_ven_female.txt" );
                  else
                     snprintf( file, 256, "%s%s", SYSTEM_DIR, "namegen_ven_other.txt" );
               }
               else if( namegenCheckString.find( "namegen_orc" ) != string::npos )
               {
                  genstring = "namegen_orc";
                  namePicked = true;
                  if( mob->sex == SEX_FEMALE )
                     snprintf( file, 256, "%s%s", SYSTEM_DIR, "namegen_orc_female.txt" );
                  else
                     snprintf( file, 256, "%s%s", SYSTEM_DIR, "namegen_orc_other.txt" );
               }

               nameg[0] = '\0';
               if( !namePicked )
                  name_generator( nameg );
               else
                  pick_name( nameg, file );

               STRFREE( mob->name );
               STRFREE( mob->short_descr );
               mudstrlcpy( mob_keywords, namegenCheckString.c_str(), MSL );
               mudstrlcat( mob_keywords, " ", MSL );
               mudstrlcat( mob_keywords, nameg, MSL );
               mob->name = STRALLOC( mob_keywords );
               mob->short_descr = STRALLOC( nameg );

               string long_desc = mob->long_descr;
               string::size_type token = 0;

               while( ( token = long_desc.find( genstring ) ) != string::npos )
                  long_desc = long_desc.replace( token, genstring.size(), nameg );
               STRFREE( mob->long_descr );
               mob->long_descr = STRALLOC( long_desc.c_str() );

               if( mob->chardesc )
               {
                  token = 0;
                  string char_desc = mob->chardesc;
                  while( ( token = char_desc.find( genstring ) ) != string::npos )
                     char_desc = char_desc.replace( token, genstring.size(), nameg );
                  STRFREE( mob->chardesc );
                  mob->chardesc = STRALLOC( char_desc.c_str() );
               }
            }

            if( !pReset->resets.empty() )
            {
               list<reset_data*>::iterator dst;
               for( dst = pReset->resets.begin(); dst != pReset->resets.end(); ++dst )
               {
                  reset_data *tReset = (*dst);

                  ++onreset;
                  switch( tReset->command )
                  {
                     default:
                        break;

                     case 'X':
                     case 'Y':
                     {
                        if( tReset->command == 'X' && number_percent() > tReset->arg8 )
                           break;
                        if( tReset->command == 'Y' && number_percent() > tReset->arg7 )
                           break;

                        obj_data *objnew = generate_random( tReset, mob );
                        if( objnew )
                        {
                           objnew->to_char( mob );
                           if( tReset->command == 'X' )
                           {
                              if( objnew->carried_by != mob )
                              {
                                 bug( "'X' reset: can't give object %d to mob %d.", objnew->pIndexData->vnum, mob->pIndexData->vnum );
                                 break;
                              }
                              mob->equip( objnew, tReset->arg7 );
                           }
                        }
                        break;
                     }

                     case 'G':
                     case 'E':
                        if( tReset->command == 'G' && number_percent() > tReset->arg3 )
                           break;
                        if( tReset->command == 'E' && number_percent() > tReset->arg4 )
                           break;
                        if( !( pObjIndex = get_obj_index( tReset->arg1 ) ) )
                        {
                           bug( "%s: %s: 'E' or 'G': bad obj vnum %d.", __FUNCTION__, filename, tReset->arg1 );
                           break;
                        }
                        if( !mob )
                        {
                           lastobj = NULL;
                           break;
                        }

                        if( pObjIndex->count >= pObjIndex->limit )
                        {
                           obj = NULL;
                           lastobj = NULL;
                           break;
                        }

                        if( mob->pIndexData->pShop )
                        {
                           int olevel = generate_itemlevel( area, pObjIndex );
                           obj = pObjIndex->create_object( olevel );
                           obj->extra_flags.set( ITEM_INVENTORY );
                        }
                        else
                           obj = pObjIndex->create_object( number_fuzzy( level ) );
                        obj->level = URANGE( 0, obj->level, LEVEL_AVATAR );
                        obj = obj->to_char( mob );
                        if( tReset->command == 'E' )
                        {
                           if( obj->carried_by != mob )
                           {
                              bug( "'E' reset: can't give object %d to mob %d.", obj->pIndexData->vnum, mob->pIndexData->vnum );
                              break;
                           }
                           mob->equip( obj, tReset->arg3 );
                        }
                        nestmap.clear();
                        nestmap[0] = obj;
                        lastobj = nestmap[0];
                        lastnest = 0;

                        if( !tReset->resets.empty() )
                        {
                           list<reset_data*>::iterator gst;
                           for( gst = tReset->resets.begin(); gst != tReset->resets.end(); ++gst )
                           {
                              reset_data *gReset = (*gst);
                              int iNest;
                              to_obj = lastobj;

                              ++onreset;
                              switch( gReset->command )
                              {
                                 default:
                                    break;

                                 case 'H':
                                    // Failed percentage check, don't bother processing. Move along.
                                    if( number_percent() > gReset->arg1 )
                                       break;
                                    if( !lastobj )
                                       break;
                                    lastobj->extra_flags.set( ITEM_HIDDEN );
                                    break;

                                 case 'P':
                                    // Failed percentage check, don't bother processing. Move along.
                                    if( number_percent() > gReset->arg5 )
                                       break;
                                    if( !( pObjIndex = get_obj_index( gReset->arg2 ) ) )
                                    {
                                       bug( "%s: %s: 'P': bad obj vnum %d.", __FUNCTION__, filename, gReset->arg2 );
                                       break;
                                    }

                                    if( !( pObjToIndex = get_obj_index( gReset->arg4 ) ) )
                                    {
                                       bug( "%s: %s: 'P': bad objto vnum %d.", __FUNCTION__, filename, gReset->arg4 );
                                       break;
                                    }
                                    if( pObjIndex->count >= pObjIndex->limit
                                      || count_obj_list( gReset, pObjIndex, to_obj->contents ) > 0 )
                                    {
                                       obj = NULL;
                                       break;
                                    }

                                    iNest = gReset->arg1;
                                    if( iNest < lastnest )
                                       to_obj = nestmap[iNest];
                                    else if( iNest == lastnest )
                                       to_obj = nestmap[lastnest];
                                    else
                                       to_obj = lastobj;

                                    if( pObjIndex->count + gReset->arg3 > pObjIndex->limit )
                                    {
                                       num = pObjIndex->limit - gReset->arg3;
                                       if( num < 1 )
                                       {
                                          obj = NULL;
                                          break;
                                       }
                                    }
                                    else
                                       num = gReset->arg3;

                                    obj = pObjIndex->create_object( number_fuzzy( UMAX( generate_itemlevel( area, pObjIndex ), to_obj->level ) ) );
                                    if( num > 1 )
                                       pObjIndex->count += ( num - 1 );
                                    obj->count = num;
                                    obj->level = UMIN( obj->level, LEVEL_AVATAR );
                                    obj->count = gReset->arg3;
                                    obj->to_obj( to_obj );
                                    if( iNest > lastnest )
                                    {
                                       nestmap[iNest] = to_obj;
                                       lastnest = iNest;
                                    }
                                    lastobj = obj;
                                    // Hackish fix for nested puts
                                    if( gReset->arg4 == OBJ_VNUM_DUMMYOBJ )
                                       gReset->arg4 = to_obj->pIndexData->vnum;
                                    break;
                              }
                           }
                        }
                        break;
                  }
               }
            }
            break;
         }

         case 'Z':
         {
            // Failed percentage check, don't bother processing. Move along.
            if( number_percent() > pReset->arg11 )
               break;

            if( !( pRoomIndex = get_room_index( pReset->arg7 ) ) )
            {
               bug( "%s: %s: 'Z': bad room vnum %d.", __FUNCTION__, filename, pReset->arg7 );
               break;
            }

            obj = generate_random( pReset, NULL );

            nestmap.clear();
            nestmap[0] = obj;
            lastobj = nestmap[0];
            lastnest = 0;
            obj->to_room( pRoomIndex, NULL );
         }
         break;

         case 'O':
         {
            // Failed percentage check, don't bother processing. Move along.
            if( number_percent() > pReset->arg7 )
               break;

            if( !( pObjIndex = get_obj_index( pReset->arg1 ) ) )
            {
               bug( "%s: %s: 'O': bad obj vnum %d.", __FUNCTION__, filename, pReset->arg1 );
               break;
            }
            if( !( pRoomIndex = get_room_index( pReset->arg3 ) ) )
            {
               bug( "%s: %s: 'O': bad room vnum %d.", __FUNCTION__, filename, pReset->arg3 );
               break;
            }

            /*
             * Item limits here 
             */
            bool limited = false;
            // Index count already exceeds rare item limit
            if( pObjIndex->count >= pObjIndex->limit )
               limited = true;

            // Index count might exceed rare item limit
            if( pObjIndex->count + pReset->arg2 > pObjIndex->limit )
            {
               num = pObjIndex->limit - pReset->arg2;
               if( num < 1 )
                  limited = true;
            }
            // In-room count might exceed the maximum per room in the reset
            else
            {
               num = pReset->arg2 - count_obj_list( pReset, pObjIndex, pRoomIndex->objects );
               if( num < 1 )
                  limited = true;
            }

            if( !limited )
            {
               obj = pObjIndex->create_object( number_fuzzy( generate_itemlevel( area, pObjIndex ) ) );
               if( num > 1 )
                  pObjIndex->count += ( num - 1 );
               obj->count = num;
               obj->level = UMIN( obj->level, LEVEL_AVATAR );
               obj->cost = 0;
               if( pReset->arg4 != -1 && pReset->arg5 != -1 && pReset->arg6 != -1 )
               {
                  obj->extra_flags.set( ITEM_ONMAP );
                  obj->map = pReset->arg4;
                  obj->mx = pReset->arg5;
                  obj->my = pReset->arg6;
               }
               obj->to_room( pRoomIndex, NULL );
            }
            else
            {
               if( !( obj = get_obj_room( pObjIndex, pRoomIndex ) ) )
               {
                  obj = NULL;
                  lastobj = NULL;
                  break;
               }
               obj->extra_flags = pObjIndex->extra_flags;
               if( pReset->arg4 != -1 && pReset->arg5 != -1 && pReset->arg6 != -1 )
               {
                  obj->extra_flags.set( ITEM_ONMAP );
                  obj->map = pReset->arg4;
                  obj->mx = pReset->arg5;
                  obj->my = pReset->arg6;
               }
               for( int x = 0; x < 11; ++x )
                  obj->value[x] = pObjIndex->value[x];
            }
            nestmap.clear();
            nestmap[0] = obj;
            lastobj = nestmap[0];
            lastnest = 0;

            if( !pReset->resets.empty() )
            {
               list<reset_data*>::iterator dst;
               for( dst = pReset->resets.begin(); dst != pReset->resets.end(); ++dst )
               {
                  int iNest;

                  reset_data *tReset = (*dst);
                  to_obj = lastobj;
                  ++onreset;

                  switch( tReset->command )
                  {
                     default:
                        break;

                     case 'H':
                        // Failed percentage check, don't bother processing. Move along.
                        if( number_percent() > tReset->arg1 || !lastobj )
                           break;
                        lastobj->extra_flags.set( ITEM_HIDDEN );
                        break;

                     case 'T':
                        // Failed percentage check, don't bother processing. Move along.
                        if( number_percent() > tReset->arg5 )
                           break;
                        if( !IS_SET( tReset->arg1, TRAP_OBJ ) )
                        {
                           bug( "%s: Room reset found on object reset list", __FUNCTION__ );
                           break;
                        }
                        else
                        {
                           /*
                            * We need to preserve obj for future 'T' checks 
                            */
                           obj_data *pobj;

                           if( tReset->arg4 > 0 )
                           {
                              if( !( pObjToIndex = get_obj_index( tReset->arg4 ) ) )
                              {
                                 bug( "%s: %s: 'T': bad objto vnum %d.", __FUNCTION__, filename, tReset->arg4 );
                                 continue;
                              }
                              if( area->nplayer > 0 || !( to_obj = get_obj_type( pObjToIndex ) ) ||
                                ( to_obj->carried_by && !to_obj->carried_by->isnpc(  ) ) || to_obj->is_trapped(  ) )
                                 break;
                           }
                           else
                           {
                              if( !lastobj || !obj )
                                 break;
                              to_obj = obj;
                           }
                           pobj = make_trap( tReset->arg3, tReset->arg2, number_fuzzy( to_obj->level ), tReset->arg1 );
                           pobj->to_obj( to_obj );
                        }
                        break;

                     case 'W':
                     {
                        if( number_percent() > tReset->arg9 )
                           break;

                        obj_data *newobj = generate_random( tReset, NULL );

                        iNest = tReset->arg1;
                        if( iNest < lastnest )
                           to_obj = nestmap[iNest];
                        else if( iNest == lastnest )
                           to_obj = nestmap[lastnest];
                        else
                           to_obj = lastobj;

                        newobj->to_obj( to_obj );
                        if( iNest > lastnest )
                        {
                           nestmap[iNest] = to_obj;
                           lastnest = iNest;
                        }
                        lastobj = newobj;
                        break;
                     }

                     case 'P':
                        // Failed percentage check, don't bother processing. Move along.
                        if( number_percent() > tReset->arg5 )
                           break;

                        if( !( pObjIndex = get_obj_index( tReset->arg2 ) ) )
                        {
                           bug( "%s: %s: 'P': bad obj vnum %d.", __FUNCTION__, filename, tReset->arg2 );
                           break;
                        }

                        if( !( pObjToIndex = get_obj_index( tReset->arg4 ) ) )
                        {
                           bug( "%s: %s: 'P': bad objto vnum %d.", __FUNCTION__, filename, tReset->arg4 );
                           break;
                        }

                        if( pObjIndex->count >= pObjIndex->limit
                            || count_obj_list( tReset, pObjIndex, to_obj->contents ) > 0 )
                        {
                           obj = NULL;
                           break;
                        }

                        iNest = tReset->arg1;
                        if( iNest < lastnest )
                           to_obj = nestmap[iNest];
                        else if( iNest == lastnest )
                           to_obj = nestmap[lastnest];
                        else
                           to_obj = lastobj;

                        if( pObjIndex->count + tReset->arg3 > pObjIndex->limit )
                        {
                           num = pObjIndex->limit - tReset->arg3;
                           if( num < 1 )
                           {
                              obj = NULL;
                              break;
                           }
                        }
                        else
                           num = tReset->arg3;

                        obj = pObjIndex->create_object( number_fuzzy( UMAX( generate_itemlevel( area, pObjIndex ), to_obj->level ) ) );
                        if( num > 1 )
                           pObjIndex->count += ( num - 1 );
                        obj->count = num;
                        obj->level = UMIN( obj->level, LEVEL_AVATAR );
                        obj->count = tReset->arg3;
                        obj->to_obj( to_obj );
                        if( iNest > lastnest )
                        {
                           nestmap[iNest] = to_obj;
                           lastnest = iNest;
                        }
                        lastobj = obj;
                        // Hackish fix for nested puts
                        if( tReset->arg4 == OBJ_VNUM_DUMMYOBJ )
                           tReset->arg4 = to_obj->pIndexData->vnum;
                        break;
                  }
               }
            }
            break;
         }

         case 'T':
            // Failed percentage check, don't bother processing. Move along.
            if( number_percent() > pReset->arg5 )
               break;
            if( IS_SET( pReset->arg1, TRAP_OBJ ) )
            {
               bug( "%s: Object trap found in room %d reset list", __FUNCTION__, vnum );
               break;
            }
            else
            {
               if( !( pRoomIndex = get_room_index( pReset->arg4 ) ) )
               {
                  bug( "%s: %s: 'T': bad room %d.", __FUNCTION__, filename, pReset->arg4 );
                  break;
               }
               if( area->nplayer > 0 || count_obj_list( pReset, get_obj_index( OBJ_VNUM_TRAP ), pRoomIndex->objects ) > 0 )
                  break;
               to_obj = make_trap( pReset->arg2, pReset->arg2, 10, pReset->arg1 );
               to_obj->to_room( pRoomIndex, NULL );
            }
            break;

         case 'D':
            // Failed percentage check, don't bother processing. Move along.
            if( number_percent() > pReset->arg4 )
               break;
            if( !( pRoomIndex = get_room_index( pReset->arg1 ) ) )
            {
               bug( "%s: %s: 'D': bad room vnum %d.", __FUNCTION__, filename, pReset->arg1 );
               break;
            }
            if( !( pexit = pRoomIndex->get_exit( pReset->arg2 ) ) )
               break;
            switch ( pReset->arg3 )
            {
               default:
               case 0:
                  REMOVE_EXIT_FLAG( pexit, EX_CLOSED );
                  REMOVE_EXIT_FLAG( pexit, EX_LOCKED );
                  break;

               case 1:
                  SET_EXIT_FLAG( pexit, EX_CLOSED );
                  REMOVE_EXIT_FLAG( pexit, EX_LOCKED );
                  if( IS_EXIT_FLAG( pexit, EX_xSEARCHABLE ) )
                     SET_EXIT_FLAG( pexit, EX_SECRET );
                  break;

               case 2:
                  SET_EXIT_FLAG( pexit, EX_CLOSED );
                  SET_EXIT_FLAG( pexit, EX_LOCKED );
                  if( IS_EXIT_FLAG( pexit, EX_xSEARCHABLE ) )
                     SET_EXIT_FLAG( pexit, EX_SECRET );
                  break;
            }
            break;

         case 'R':
            // Failed percentage check, don't bother processing. Move along.
            if( number_percent() > pReset->arg3 )
               break;
            if( !( pRoomIndex = get_room_index( pReset->arg1 ) ) )
            {
               bug( "%s: %s: 'R': bad room vnum %d.", __FUNCTION__, filename, pReset->arg1 );
               break;
            }
            pRoomIndex->randomize_exits( pReset->arg2 - 1 );
            break;
      }
   }
   return;
}

void room_index::load_reset( FILE *fp, bool newformat )
{
   exit_data *pexit;
   char letter;
   char *line;
   int extra, arg1, arg2, arg3;
   short arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11;
   bool not01 = false;
   int count = 0;

   letter = fread_letter( fp );
   line = fread_line( fp );

   // Useful to ferret out bad stuff
   extra = arg1 = arg2 = arg3 = arg4 = arg5 = arg6 = arg7 = arg8 = arg9 = arg10 = arg11 = -2;

   switch( letter )
   {
      default:
      case 'M':
      case 'O':
         arg7 = 100;
         break;

      case 'Z':
         arg11 = 100;
         break;

      case 'P':
      case 'T':
         arg5 = 100;
         break;

      case 'E':
      case 'D':
         arg4 = 100;
         break;

      case 'H':
         arg1 = 100;
         break;

      case 'G':
      case 'R':
         arg3 = 100;
         break;

      case 'X':
         arg8 = 100;
         break;

      case 'W':
         arg9 = 100;
         break;

      case 'Y':
         arg7 = 100;
         break;
   }
   if( !newformat && area->version < 21 )
   {
      if( letter == 'P' || letter == 'T' || letter == 'W' )
         sscanf( line, "%d %d %d %hd %hd %hd %hd %hd %hd %hd %hd",
            &arg1, &arg2, &arg3, &arg4, &arg5, &arg6, &arg7, &arg8, &arg9, &arg10, &arg11 );
      else
         sscanf( line, "%d %d %d %d %hd %hd %hd %hd %hd %hd %hd %hd",
            &extra, &arg1, &arg2, &arg3, &arg4, &arg5, &arg6, &arg7, &arg8, &arg9, &arg10, &arg11 );
   }
   else
      sscanf( line, "%d %d %d %hd %hd %hd %hd %hd %hd %hd %hd",
         &arg1, &arg2, &arg3, &arg4, &arg5, &arg6, &arg7, &arg8, &arg9, &arg10, &arg11 );
   ++count;

   /*
    * Validate parameters.
    * We're calling the index functions for the side effect.
    */
   switch( letter )
   {
      default:
         bug( "%s: bad command '%c'.", __FUNCTION__, letter );
         if( fBootDb )
            boot_log( "%s: %s (%d) bad command '%c'.", __FUNCTION__, area->filename, count, letter );
         return;

      case 'M':
         // Resets will have a 1-100 chance. Anything out of range is fixed.
         if( arg7 < 0 )
            arg7 = 1;
         if( arg7 > 100 )
            arg7 = 100;

         if( get_mob_index( arg1 ) == NULL && fBootDb )
            boot_log( "%s: %s (%d) 'M': mobile %d doesn't exist.", __FUNCTION__, area->filename, count, arg1 );
         if( arg4 != -1 && ( arg4 < 0 || arg4 >= MAP_MAX ) )
         {
            boot_log( "%s: %s (%d) 'M': Map %d does not exist.", __FUNCTION__, area->filename, count, arg4 );
            arg4 = -1;
         }
         if( arg5 != -1 && ( arg5 < 0 || arg5 >= MAX_X ) )
         {
            boot_log( "%s: %s (%d) 'M': X coordinate %d is out of range.", __FUNCTION__, area->filename, count, arg5 );
            arg5 = -1;
         }
         if( arg6 != -1 && ( arg6 < 0 || arg6 >= MAX_Y ) )
         {
            boot_log( "%s: %s (%d) 'M': Y coordinate %d is out of range.", __FUNCTION__, area->filename, count, arg6 );
            arg6 = -1;
         }
         break;

      case 'O':
         if( arg7 < 0 )
            arg7 = 1;
         if( arg7 > 100 )
            arg7 = 100;

         if( get_obj_index( arg1 ) == NULL && fBootDb )
            boot_log( "%s: %s (%d) '%c': object %d doesn't exist.", __FUNCTION__, area->filename, count, letter, arg1 );
         if( arg4 != -1 && ( arg4 < 0 || arg4 >= MAP_MAX ) )
         {
            boot_log( "%s: %s (%d) 'O': Map %d does not exist.", __FUNCTION__, area->filename, count, arg4 );
            arg4 = -1;
         }
         if( arg5 != -1 && ( arg5 < 0 || arg5 >= MAX_X ) )
         {
            boot_log( "%s: %s (%d) 'O': X coordinate %d is out of range.", __FUNCTION__, area->filename, count, arg5 );
            arg5 = -1;
         }
         if( arg6 != -1 && ( arg6 < 0 || arg6 >= MAX_Y ) )
         {
            boot_log( "%s: %s (%d) 'O': Y coordinate %d is out of range.", __FUNCTION__, area->filename, count, arg6 );
            arg6 = -1;
         }
         break;

      case 'Z':
         if( arg11 < 0 )
            arg11 = 1;
         if( arg11 > 100 )
            arg11 = 100;

         if( arg8 != -1 && ( arg8 < 0 || arg8 >= MAP_MAX ) )
         {
            boot_log( "%s: %s (%d) 'Z': Map %d does not exist.", __FUNCTION__, area->filename, count, arg8 );
            arg8 = -1;
         }
         if( arg9 != -1 && ( arg9 < 0 || arg9 >= MAX_X ) )
         {
            boot_log( "%s: %s (%d) 'Z': X coordinate %d is out of range.", __FUNCTION__, area->filename, count, arg9 );
            arg9 = -1;
         }
         if( arg10 != -1 && ( arg10 < 0 || arg10 >= MAX_Y ) )
         {
            boot_log( "%s: %s (%d) 'Z': Y coordinate %d is out of range.", __FUNCTION__, area->filename, count, arg10 );
            arg10 = -1;
         }
         break;

      case 'P':
         if( arg5 < 0 )
            arg5 = 1;
         if( arg5 > 100 )
            arg5 = 100;

         if( get_obj_index( arg2 ) == NULL && fBootDb )
            boot_log( "%s: %s (%d) '%c': object %d doesn't exist.", __FUNCTION__, area->filename, count, letter, arg2 );

         if( arg4 <= 0 )
            arg4 = OBJ_VNUM_DUMMYOBJ; // This may look stupid, but for some reason it works.
         if( get_obj_index( arg4 ) == NULL && fBootDb )
            boot_log( "%s: %s (%d) 'P': destination object %d doesn't exist.", __FUNCTION__, area->filename, count, arg4 );
         if( !newformat && area->version < 21 )
         {
            if( extra > 0 )
               not01 = true;
         }
         else
         {
            if( arg1 > 0 )
               not01 = true;
         }
         break;

      case 'G':
         if( arg3 < 0 )
            arg3 = 1;
         if( arg3 > 100 )
            arg3 = 100;
         if( get_obj_index( arg1 ) == NULL && fBootDb )
            boot_log( "%s: %s (%d) '%c': object %d doesn't exist.", __FUNCTION__, area->filename, count, letter, arg1 );
         break;

      case 'E':
         if( arg4 < 0 )
            arg4 = 1;
         if( arg4 > 100 )
            arg4 = 100;

         if( get_obj_index( arg1 ) == NULL && fBootDb )
            boot_log( "%s: %s (%d) '%c': object %d doesn't exist.", __FUNCTION__, area->filename, count, letter, arg1 );
         break;

      case 'X':
         if( arg8 < 0 )
            arg8 = 1;
         if( arg8 > 100 )
            arg8 = 100;
         break;

      case 'W':
         if( arg9 < 0 )
            arg9 = 1;
         if( arg9 > 100 )
            arg9 = 100;
         break;

      case 'Y':
         if( arg7 < 0 )
            arg7 = 1;
         if( arg7 > 100 )
            arg7 = 100;
         break;

      case 'T':
         if( arg5 < 0 )
            arg5 = 1;
         if( arg5 > 100 )
            arg5 = 100;
         break;

      case 'H':
         if( arg1 < 0 )
            arg1 = 1;
         if( arg1 > 100 )
            arg1 = 100;
         break;

      case 'D':
         if( arg4 < 0 )
            arg4 = 1;
         if( arg4 > 100 )
            arg4 = 100;

         if( arg2 < 0 || arg2 > MAX_DIR + 1
             || !( pexit = get_exit( arg2 ) ) || !IS_EXIT_FLAG( pexit, EX_ISDOOR ) )
         {
            bug( "%s: 'D': exit %d not door.", __FUNCTION__, arg2 );
            log_printf( "Reset: %c %d %d %d %d %d", letter, extra, arg1, arg2, arg3, arg4 );
            if( fBootDb )
               boot_log( "%s: %s (%d) 'D': exit %d not door.", __FUNCTION__, area->filename, count, arg2 );
         }

         if( arg3 < 0 || arg3 > 2 )
         {
            bug( "%s: 'D': bad 'locks': %d.", __FUNCTION__, arg3 );
            if( fBootDb )
               boot_log( "%s: %s (%d) 'D': bad 'locks': %d.", __FUNCTION__, area->filename, count, arg3 );
         }
         break;

      case 'R':
         if( arg3 < 0 )
            arg3 = 1;
         if( arg3 > 100 )
            arg3 = 100;

         if( arg2 < 0 || arg2 > 10 )
         {
            bug( "%s: 'R': bad exit %d.", __FUNCTION__, arg2 );
            if( fBootDb )
               boot_log( "%s: %s (%d) 'R': bad exit %d.", __FUNCTION__, area->filename, count, arg2 );
            break;
         }
         break;
   }
   add_reset( letter, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11 );

   if( !not01 )
      renumber_put_resets();
   return;
}

int get_dirnum( char *flag )
{
   size_t x;

   for( x = 0; x < ( sizeof( dir_name ) / sizeof( dir_name[0] ) ); ++x )
      if( !str_cmp( flag, dir_name[x] ) )
         return x;

   for( x = 0; x < ( sizeof( short_dirname ) / sizeof( short_dirname[0] ) ); ++x )
      if( !str_cmp( flag, short_dirname[x] ) )
         return x;

   return -1;
}

char *rev_exit( short vdir )
{
   switch ( vdir )
   {
      default:
         return "somewhere";
      case 0:
         return "the south";
      case 1:
         return "the west";
      case 2:
         return "the north";
      case 3:
         return "the east";
      case 4:
         return "below";
      case 5:
         return "above";
      case 6:
         return "the southwest";
      case 7:
         return "the southeast";
      case 8:
         return "the northwest";
      case 9:
         return "the northeast";
   }
}

CMDF( do_recho )
{
   ch->set_color( AT_IMMORT );

   if( ch->has_pcflag( PCFLAG_NO_EMOTE ) )
   {
      ch->print( "You can't do that right now.\r\n" );
      return;
   }
   if( !argument || argument[0] == '\0' )
   {
      ch->print( "Recho what?\r\n" );
      return;
   }
   ch->in_room->echo( argument );
}

CMDF( do_rdelete )
{
   room_index *location;

   if( ch->substate == SUB_RESTRICTED )
   {
      ch->print( "You can't do that while in a subprompt.\r\n" );
      return;
   }

   if( !argument || argument[0] == '\0' )
   {
      ch->print( "Delete which room?\r\n" );
      return;
   }

   /*
    * Find the room. 
    */
   if( !( location = ch->find_location( argument ) ) )
   {
      ch->print( "No such location.\r\n" );
      return;
   }

   /*
    * Does the player have the right to delete this room? 
    */
   if( ch->get_trust(  ) < sysdata->level_modify_proto
       && ( location->vnum < ch->pcdata->low_vnum || location->vnum > ch->pcdata->hi_vnum ) )
   {
      ch->print( "That room is not in your assigned range.\r\n" );
      return;
   }
   deleteptr( location );
   fix_exits(); /* Need to call this to solve a crash */
   ch->printf( "Room %s has been deleted.\r\n", argument );
   return;
}
