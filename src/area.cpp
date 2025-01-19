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
 *                           Area Support Functions                         *
 ****************************************************************************/

#if defined(WIN32)
#include <unistd.h>
#endif
#include "mud.h"
#include "area.h"
#include "calendar.h"
#include "descriptor.h"
#include "mobindex.h"
#include "mud_prog.h"
#include "objindex.h"
#include "overland.h"
#include "roomindex.h"
#include "shops.h"
#include "weather.h"

int top_area;

extern int top_prog;
extern int top_affect;
extern int top_shop;
extern int top_repair;
extern FILE *fpArea;

list < area_data * >arealist;
list < area_data * >area_nsort;
list < area_data * >area_vsort;

int recall( char_data *, int );
void save_sysdata(  );
bool check_area_conflict( area_data *, int, int );
void web_arealist(  );
area_data *fread_smaugfuss_area( FILE * );
bool load_oldafk_area( FILE *, area_data *, int );
CMDF( do_areaconvert );

area_data::~area_data(  )
{
   list < char_data * >::iterator ich;
   for( ich = charlist.begin(  ); ich != charlist.end(  ); )
   {
      char_data *ech = *ich;
      ++ich;

      if( ech->fighting )
         ech->stop_fighting( true );

      if( ech->isnpc(  ) )
      {
         /*
          * if mob is in area, or part of area. 
          */
         if( URANGE( low_vnum, ech->pIndexData->vnum, hi_vnum ) == ech->pIndexData->vnum || ( ech->in_room && ech->in_room->area == this ) )
            ech->extract( true );
         continue;
      }

      if( ech->in_room && ech->in_room->area == this )
         recall( ech, -1 );

      if( !ech->isnpc(  ) && ech->pcdata->area == this )
      {
         ech->pcdata->area = nullptr;
         ech->pcdata->low_vnum = 0;
         ech->pcdata->hi_vnum = 0;
         ech->save(  );
      }
   }

   list < obj_data * >::iterator iobj;
   for( iobj = objlist.begin(  ); iobj != objlist.end(  ); )
   {
      obj_data *eobj = *iobj;
      ++iobj;

      /*
       * if obj is in area, or part of area. 
       */
      if( URANGE( low_vnum, eobj->pIndexData->vnum, hi_vnum ) == eobj->pIndexData->vnum || ( eobj->in_room && eobj->in_room->area == this ) )
         eobj->extract(  );
   }

   wipe_resets(  );

   list < room_index * >::iterator rid;
   list < mob_index * >::iterator mid;
   list < obj_index * >::iterator oid;
   for( rid = rooms.begin(  ); rid != rooms.end(  ); )
   {
      room_index *room = *rid;
      ++rid;

      deleteptr( room );
   }
   rooms.clear(  );

   for( mid = mobs.begin(  ); mid != mobs.end(  ); )
   {
      mob_index *mob = *mid;
      ++mid;

      deleteptr( mob );
   }
   mobs.clear(  );

   for( oid = objects.begin(  ); oid != objects.end(  ); )
   {
      obj_index *obj = *oid;
      ++oid;

      deleteptr( obj );
   }
   objects.clear(  );

   DISPOSE( name );
   DISPOSE( filename );
   DISPOSE( resetmsg );
   STRFREE( author );
   arealist.remove( this );
   area_nsort.remove( this );
   area_vsort.remove( this );
}

area_data::area_data(  )
{
   init_memory( &continent, &tg_armor, sizeof( tg_armor ) );
}

void area_data::fix_exits(  )
{
   room_index *pRoomIndex;
   list < room_index * >::iterator rindex;
   list < exit_data * >::iterator iexit;

   for( rindex = rooms.begin(  ); rindex != rooms.end(  ); ++rindex )
   {
      pRoomIndex = *rindex;

      for( iexit = pRoomIndex->exits.begin(  ); iexit != pRoomIndex->exits.end(  ); ++iexit )
      {
         exit_data *pexit = *iexit;

         pexit->rvnum = pRoomIndex->vnum;
         if( pexit->vnum <= 0 )
            pexit->to_room = nullptr;
         else
            pexit->to_room = get_room_index( pexit->vnum );
      }
   }

   for( rindex = rooms.begin(  ); rindex != rooms.end(  ); ++rindex )
   {
      pRoomIndex = *rindex;

      for( iexit = pRoomIndex->exits.begin(  ); iexit != pRoomIndex->exits.end(  ); ++iexit )
      {
         exit_data *pexit = *iexit;

         if( pexit->to_room && !pexit->rexit )
         {
            exit_data *rv_exit = pexit->to_room->get_exit_to( rev_dir[pexit->vdir], pRoomIndex->vnum );
            if( rv_exit )
            {
               pexit->rexit = rv_exit;
               rv_exit->rexit = pexit;
            }
         }
      }
   }
}

/*
 * Sort by names -Altrag & Thoric
 * Cut out all the extra damn lists - Samson
 */
void area_data::sort_name(  )
{
   list < area_data * >::iterator iarea;

   for( iarea = area_nsort.begin(  ); iarea != area_nsort.end(  ); ++iarea )
   {
      area_data *area = *iarea;

      if( strcmp( name, area->name ) < 0 )
      {
         area_nsort.insert( iarea, this );
         return;
      }
   }
   area_nsort.push_back( this );
}

/*
 * Sort by Vnums -Altrag & Thoric
 * Cut out all the extra damn lists - Samson
 */
void area_data::sort_vnums(  )
{
   list < area_data * >::iterator iarea;

   for( iarea = area_vsort.begin(  ); iarea != area_vsort.end(  ); ++iarea )
   {
      area_data *area = *iarea;

      if( low_vnum < area->low_vnum )
      {
         area_vsort.insert( iarea, this );
         return;
      }
   }
   area_vsort.push_back( this );
}

void area_data::reset(  )
{
   list < room_index * >::iterator iroom;

   if( rooms.empty(  ) )
      return;

   for( iroom = rooms.begin(  ); iroom != rooms.end(  ); ++iroom )
   {
      room_index *room = *iroom;

      room->reset(  );
   }
}

void area_data::wipe_resets(  )
{
   list < room_index * >::iterator iroom;

   if( !mud_down )
   {
      for( iroom = rooms.begin(  ); iroom != rooms.end(  ); ++iroom )
      {
         room_index *room = *iroom;

         room->wipe_resets(  );
      }
   }
}

area_data *create_area( void )
{
   area_data *pArea;

   pArea = new area_data;

   pArea->version = 0;
   pArea->rooms.clear(  );
   pArea->age = 15;
   pArea->reset_frequency = 15;
   pArea->hi_soft_range = MAX_LEVEL;
   pArea->hi_hard_range = MAX_LEVEL;
   pArea->tg_nothing = 20;
   pArea->tg_gold = 74;
   pArea->tg_item = 85;
   pArea->tg_gem = 93;
   pArea->tg_scroll = 20;
   pArea->tg_potion = 50;
   pArea->tg_wand = 60;
   pArea->tg_armor = 75;
   pArea->weatherx = 0;
   pArea->weathery = 0;

   arealist.push_back( pArea );
   ++top_area;

   return pArea;
}

void validate_treasure_settings( area_data * area )
{
   if( area->tg_nothing > area->tg_gold && area->tg_gold != 0 )
   {
      log_printf( "%s: Nothing setting is larger than gold setting.", area->filename );
      if( area->tg_nothing > 100 )
      {
         log_printf( "%s: Nothing setting exceeds 100%%, correcting.", area->filename );
         area->tg_nothing = 100;
      }
   }

   if( area->tg_gold > area->tg_item && area->tg_item != 0 )
   {
      log_printf( "%s: Gold setting is larger than item setting.", area->filename );
      if( area->tg_gold > 100 )
      {
         log_printf( "%s: Gold setting exceeds 100%%, correcting.", area->filename );
         area->tg_gold = 100;
      }
   }

   if( area->tg_item > area->tg_gem && area->tg_gem != 0 )
   {
      log_printf( "%s: Item setting is larger than gem setting.", area->filename );
      if( area->tg_item > 100 )
      {
         log_printf( "%s: Item setting exceeds 100%%, correcting.", area->filename );
         area->tg_item = 100;
      }
   }

   if( area->tg_gem > 100 )
   {
      log_printf( "%s: Gem setting exceeds 100%%, correcting.", area->filename );
      area->tg_gem = 100;
   }

   if( area->tg_scroll > area->tg_potion && area->tg_potion != 0 )
   {
      log_printf( "%s: Scroll setting is larger than potion setting.", area->filename );
      if( area->tg_scroll > 100 )
      {
         log_printf( "%s: Scroll setting exceeds 100%%, correcting.", area->filename );
         area->tg_scroll = 100;
      }
   }

   if( area->tg_potion > area->tg_wand && area->tg_wand != 0 )
   {
      log_printf( "%s: Potion setting is larger than wand setting.", area->filename );
      if( area->tg_wand > 100 )
      {
         log_printf( "%s: Wand setting exceeds 100%%, correcting.", area->filename );
         area->tg_wand = 100;
      }
   }

   if( area->tg_wand > area->tg_armor && area->tg_armor != 0 )
   {
      log_printf( "%s: Wand setting is larger than armor setting.", area->filename );
      if( area->tg_wand > 100 )
      {
         log_printf( "%s: Wand setting exceeds 100%%, correcting.", area->filename );
         area->tg_wand = 100;
      }
   }

   if( area->tg_armor > 100 )
   {
      log_printf( "%s: Armor setting exceeds 100%%, correcting.", area->filename );
      area->tg_armor = 100;
   }
}

void fread_afk_exit( FILE * fp, room_index * pRoomIndex )
{
   exit_data *pexit = nullptr;

   for( ;; )
   {
      const char *word = ( feof( fp ) ? "#ENDEXIT" : fread_word( fp ) );

      if( word[0] == '\0' )
      {
         log_printf( "%s: EOF encountered reading file!", __func__ );
         word = "#ENDEXIT";
      }

      switch ( word[0] )
      {
         default:
            log_printf( "%s: no match: %s", __func__, word );
            fread_to_eol( fp );
            break;

         case '#':
            if( !str_cmp( word, "#ENDEXIT" ) )
            {
               return;
            }
            break;

         case 'D':
            KEY( "Desc", pexit->exitdesc, fread_string( fp ) );
            if( !str_cmp( word, "Direction" ) )
            {
               int door = get_dir( fread_flagstring( fp ) );

               if( door < 0 || door > DIR_SOMEWHERE )
               {
                  bug( "%s: vnum %d has bad door number %d.", __func__, pRoomIndex->vnum, door );
                  if( fBootDb )
                     return;
               }
               pexit = pRoomIndex->make_exit( nullptr, door );
            }
            break;

         case 'F':
            if( !str_cmp( word, "Flags" ) )
            {
               flag_set( fp, pexit->flags, ex_flags );
               break;
            }
            break;

         case 'K':
            KEY( "Key", pexit->key, fread_number( fp ) );
            KEY( "Keywords", pexit->keyword, fread_string( fp ) );
            break;

         case 'P':
            if( !str_cmp( word, "Pull" ) )
            {
               pexit->pulltype = fread_number( fp );
               pexit->pull = fread_number( fp );
               break;
            }
            break;

         case 'T':
            KEY( "ToRoom", pexit->vnum, fread_number( fp ) );
            if( !str_cmp( word, "ToCoords" ) )
            {
               pexit->map_x = fread_number( fp );
               pexit->map_y = fread_number( fp );
               break;
            }
            break;
      }
   }

   // Reach this point, you fell through somehow. The data is no longer valid.
   bug( "%s: Reached fallout point! Exit data invalid.", __func__ );
   if( pexit )
      pRoomIndex->extract_exit( pexit );
}

affect_data *fread_afk_affect( FILE * fp )
{
   char *loc = nullptr;
   char *aff = nullptr;
   int value;
   bool setaff = true;

   affect_data *paf = new affect_data;
   paf->location = APPLY_NONE;
   paf->type = -1;
   paf->duration = -1;
   paf->bit = 0;
   paf->modifier = 0;
   paf->rismod.reset(  );

   loc = fread_word( fp );
   value = get_atype( loc );
   if( value < 0 || value >= MAX_APPLY_TYPE )
   {
      bug( "%s: Invalid apply type: %s", __func__, loc );
      setaff = false;
   }
   paf->location = value;

   if( paf->location == APPLY_WEAPONSPELL
       || paf->location == APPLY_WEARSPELL
       || paf->location == APPLY_REMOVESPELL || paf->location == APPLY_STRIPSN || paf->location == APPLY_RECURRINGSPELL || paf->location == APPLY_EAT_SPELL )
      paf->modifier = skill_lookup( fread_word( fp ) );
   else if( paf->location == APPLY_AFFECT )
   {
      aff = fread_word( fp );
      value = get_aflag( aff );
      if( value < 0 || value >= MAX_AFFECTED_BY )
      {
         bug( "%s: Unsupportable value for affect flag: %s", __func__, aff );
         setaff = false;
      }
      else
         paf->modifier = value;
   }
   else if( paf->location == APPLY_RESISTANT || paf->location == APPLY_IMMUNE || paf->location == APPLY_SUSCEPTIBLE || paf->location == APPLY_ABSORB )
      flag_set( fp, paf->rismod, ris_flags );
   else
      paf->modifier = fread_number( fp );

   paf->type = fread_short( fp );
   paf->duration = fread_number( fp );
   paf->bit = fread_number( fp );

   if( !setaff )
      deleteptr( paf );
   else
      ++top_affect;
   return paf;
}

extra_descr_data *fread_afk_exdesc( FILE * fp )
{
   extra_descr_data *ed = new extra_descr_data;

   for( ;; )
   {
      const char *word = ( feof( fp ) ? "#ENDEXDESC" : fread_word( fp ) );

      if( word[0] == '\0' )
      {
         log_printf( "%s: EOF encountered reading file!", __func__ );
         word = "#ENDEXDESC";
      }

      switch ( word[0] )
      {
         default:
            log_printf( "%s: no match: %s", __func__, word );
            fread_to_eol( fp );
            break;

         case '#':
            if( !str_cmp( word, "#ENDEXDESC" ) )
            {
               if( ed->keyword.empty(  ) )
               {
                  bug( "%s: Missing ExDesc keyword. Returning nullptr.", __func__ );
                  deleteptr( ed );
                  return nullptr;
               }
               return ed;
            }
            break;

         case 'E':
            STDSKEY( "ExDescKey", ed->keyword );
            STDSKEY( "ExDesc", ed->desc );
            break;
      }
   }

   // Reach this point, you fell through somehow. The data is no longer valid.
   bug( "%s: Reached fallout point! ExtraDesc data invalid.", __func__ );
   deleteptr( ed );
   return nullptr;
}

void fread_afk_areadata( FILE * fp, area_data * tarea )
{
   for( ;; )
   {
      const char *word = ( feof( fp ) ? "#ENDAREADATA" : fread_word( fp ) );

      if( word[0] == '\0' )
      {
         log_printf( "%s: EOF encountered reading file!", __func__ );
         word = "#ENDAREADATA";
      }

      switch ( word[0] )
      {
         default:
            log_printf( "%s: no match: %s", __func__, word );
            fread_to_eol( fp );
            break;

         case '#':
            if( !str_cmp( word, "#ENDAREADATA" ) )
            {
               tarea->age = tarea->reset_frequency;
               return;
            }
            break;

         case 'A':
            KEY( "Author", tarea->author, fread_string( fp ) );
            break;

         case 'C':
            KEY( "Credits", tarea->credits, fread_string( fp ) );

            if( !str_cmp( word, "Climate" ) )
            {
               // These values are no longer used with the new weather code.
               fread_to_eol( fp );
               break;
            }

            if( !str_cmp( word, "Continent" ) )
            {
               continent_data *continent = nullptr;
               string value;

               fread_string( value, fp );

               if( !( continent = find_continent_by_name( value ) ) )
               {
                  bug( "%s: Invalid area continent '%s' - Needs to be manually corrected.", __func__, value.c_str( ) );
                  tarea->continent = nullptr;
               }
               else
               {
                  tarea->continent = continent;
               }
               break;
            }

            if( !str_cmp( word, "Coordinates" ) )
            {
               short x, y;

               x = fread_short( fp );
               y = fread_short( fp );

               if( !is_valid_x( x ) )
               {
                  bug( "%s: Area has bad x coord - setting X to 0", __func__ );
                  x = 0;
               }

               if( !is_valid_y( y ) )
               {
                  bug( "%s: Area has bad y coord - setting Y to 0", __func__ );
                  y = 0;
               }

               tarea->map_x = x;
               tarea->map_y = y;
               break;
            }

         case 'D':
            if( !str_cmp( word, "Dates" ) )
            {
               tarea->creation_date = fread_long( fp );
               tarea->install_date = fread_long( fp );
               break;
            }
            break;

         case 'F':
            if( !str_cmp( word, "Flags" ) )
            {
               flag_set( fp, tarea->flags, area_flags );
               break;
            }
            break;

         case 'N':
            KEY( "Name", tarea->name, fread_string_nohash( fp ) );
            if( !str_cmp( word, "Neighbor" ) )
            {
               // This section is no longer used with the new weather code.
               fread_to_eol( fp );
               break;
            }
            break;

         case 'R':
            if( !str_cmp( word, "Ranges" ) )
            {
               int x1, x2, x3, x4;
               const char *ln = fread_line( fp );

               x1 = x2 = x3 = x4 = 0;
               sscanf( ln, "%d %d %d %d", &x1, &x2, &x3, &x4 );

               tarea->low_soft_range = x1;
               tarea->hi_soft_range = x2;
               tarea->low_hard_range = x3;
               tarea->hi_hard_range = x4;

               break;
            }
            KEY( "ResetMsg", tarea->resetmsg, fread_string_nohash( fp ) );
            KEY( "ResetFreq", tarea->reset_frequency, fread_short( fp ) );
            break;

         case 'T':
            if( !str_cmp( word, "Treasure" ) )
            {
               unsigned short x1, x2, x3, x4, x5, x6, x7, x8;
               const char *ln = fread_line( fp );

               x1 = 20;
               x2 = 74;
               x3 = 85;
               x4 = 93;
               x5 = 20;
               x6 = 50;
               x7 = 60;
               x8 = 75;
               sscanf( ln, "%hu %hu %hu %hu, %hu %hu %hu %hu", &x1, &x2, &x3, &x4, &x5, &x6, &x7, &x8 );

               tarea->tg_nothing = x1;
               tarea->tg_gold = x2;
               tarea->tg_item = x3;
               tarea->tg_gem = x4;
               tarea->tg_scroll = x5;
               tarea->tg_potion = x6;
               tarea->tg_wand = x7;
               tarea->tg_armor = x8;

               validate_treasure_settings( tarea );
               break;
            }
            break;

         case 'V':
            KEY( "Version", tarea->version, fread_short( fp ) );
            if( !str_cmp( word, "Vnums" ) )
            {
               tarea->low_vnum = fread_number( fp );
               tarea->hi_vnum = fread_number( fp );

               /*
                * Protection against forgetting to raise the MaxVnum value before adding a new zone that would exceed it.
                * Potentially dangerous if some blockhead makes insanely high vnums and then installs the area.
                */
               if( tarea->hi_vnum >= sysdata->maxvnum )
               {
                  sysdata->maxvnum = tarea->hi_vnum + 1;
                  log_printf( "MaxVnum value raised to %d to accomadate new zone.", sysdata->maxvnum );
                  save_sysdata(  );
               }
               break;
            }
            break;
      }
   }
}

void fread_afk_mobile( FILE * fp, area_data * tarea )
{
   mob_index *pMobIndex = nullptr;
   bool oldmob = false;

   for( ;; )
   {
      const char *word = ( feof( fp ) ? "#ENDMOBILE" : fread_word( fp ) );

      if( word[0] == '\0' )
      {
         log_printf( "%s: EOF encountered reading file!", __func__ );
         word = "#ENDMOBILE";
      }

      switch ( word[0] )
      {
         default:
            log_printf( "%s: no match: %s", __func__, word );
            fread_to_eol( fp );
            break;

         case '#':
            if( !str_cmp( word, "#MUDPROG" ) )
            {
               mud_prog_data *mprg = new mud_prog_data;
               fread_afk_mudprog( fp, mprg, pMobIndex );
               pMobIndex->mudprogs.push_back( mprg );
               ++top_prog;
               break;
            }

            if( !str_cmp( word, "#ENDMOBILE" ) )
            {
               if( !oldmob )
               {
                  mob_index_table.insert( map < int, mob_index * >::value_type( pMobIndex->vnum, pMobIndex ) );
                  tarea->mobs.push_back( pMobIndex );
                  ++top_mob_index;
               }
               return;
            }
            break;

         case 'A':
            if( !str_cmp( word, "Absorb" ) )
            {
               flag_set( fp, pMobIndex->absorb, ris_flags );
               break;
            }

            if( !str_cmp( word, "Actflags" ) )
            {
               flag_set( fp, pMobIndex->actflags, act_flags );
               break;
            }

            if( !str_cmp( word, "Affected" ) )
            {
               flag_set( fp, pMobIndex->affected_by, aff_flags );
               break;
            }

            if( !str_cmp( word, "Attacks" ) )
            {
               flag_set( fp, pMobIndex->attacks, attack_flags );
               break;
            }
            break;

         case 'B':
            if( !str_cmp( word, "Bodyparts" ) )
            {
               flag_set( fp, pMobIndex->body_parts, part_flags );
               break;
            }
            break;

         case 'C':
            if( !str_cmp( word, "Class" ) )
            {
               short Class = get_npc_class( fread_flagstring( fp ) );

               if( Class < 0 || Class >= MAX_NPC_CLASS )
               {
                  bug( "%s: vnum %d: Mob has invalid Class! Defaulting to warrior.", __func__, pMobIndex->vnum );
                  Class = get_npc_class( "warrior" );
               }

               pMobIndex->Class = Class;
               break;
            }
            break;

         case 'D':
            if( !str_cmp( word, "Defenses" ) )
            {
               flag_set( fp, pMobIndex->defenses, defense_flags );
               break;
            }

            if( !str_cmp( word, "DefPos" ) )
            {
               short position = get_npc_position( fread_flagstring( fp ) );

               if( position < 0 || position >= POS_MAX )
               {
                  bug( "%s: vnum %d: Mobile in invalid default position! Defaulting to standing.", __func__, pMobIndex->vnum );
                  position = POS_STANDING;
               }
               pMobIndex->defposition = position;
               break;
            }

            if( !str_cmp( word, "Desc" ) )
            {
               const char *desc = fread_flagstring( fp );

               if( desc && desc[0] != '\0' && str_cmp( desc, "(null)" ) )
               {
                  pMobIndex->chardesc = STRALLOC( desc );
                  if( str_prefix( "namegen", desc ) )
                     pMobIndex->chardesc[0] = UPPER( pMobIndex->chardesc[0] );
               }
               break;
            }
            break;

         case 'G':
            if( !str_cmp( word, "Gender" ) )
            {
               short sex = get_npc_sex( fread_flagstring( fp ) );

               if( sex < 0 || sex >= SEX_MAX )
               {
                  bug( "%s: vnum %d: Mobile has invalid gender! Defaulting to neuter.", __func__, pMobIndex->vnum );
                  sex = SEX_NEUTRAL;
               }
               pMobIndex->sex = sex;
               break;
            }
            break;

         case 'I':
            if( !str_cmp( word, "Immune" ) )
            {
               flag_set( fp, pMobIndex->immune, ris_flags );
               break;
            }
            break;

         case 'K':
            KEY( "Keywords", pMobIndex->player_name, fread_string( fp ) );
            break;

         case 'L':
            KEY( "Long", pMobIndex->long_descr, fread_string( fp ) );
            break;

         case 'N':
            KEY( "Nattacks", pMobIndex->numattacks, fread_float( fp ) );
            break;

         case 'P':
            if( !str_cmp( word, "Position" ) )
            {
               short position = get_npc_position( fread_flagstring( fp ) );

               if( position < 0 || position >= POS_MAX )
               {
                  bug( "%s: vnum %d: Mobile in invalid position! Defaulting to standing.", __func__, pMobIndex->vnum );
                  position = POS_STANDING;
               }
               pMobIndex->position = position;
               break;
            }
            break;

         case 'R':
            if( !str_cmp( word, "Race" ) )
            {
               const char *race_text = fread_flagstring( fp );
               short race = get_npc_race( race_text );

               if( race < 0 || race >= MAX_NPC_RACE )
               {
                  bug( "%s: vnum %d: Mob has invalid race: %s. Defaulting to monster.", __func__, pMobIndex->vnum, race_text );
                  race = get_npc_race( "monster" );
               }

               pMobIndex->race = race;
               break;
            }

            if( !str_cmp( word, "RepairData" ) )
            {
               int iFix;
               repair_data *rShop = new repair_data;

               rShop->keeper = pMobIndex->vnum;
               for( iFix = 0; iFix < MAX_FIX; ++iFix )
                  rShop->fix_type[iFix] = fread_number( fp );
               rShop->profit_fix = fread_number( fp );
               rShop->shop_type = fread_number( fp );
               rShop->open_hour = fread_number( fp );
               rShop->close_hour = fread_number( fp );

               pMobIndex->rShop = rShop;
               repairlist.push_back( rShop );
               ++top_repair;

               break;
            }

            if( !str_cmp( word, "Resist" ) )
            {
               flag_set( fp, pMobIndex->resistant, ris_flags );
               break;
            }
            break;

         case 'S':
            KEY( "Short", pMobIndex->short_descr, fread_string( fp ) );

            if( !str_cmp( word, "ShopData" ) )
            {
               int iTrade;
               shop_data *pShop = new shop_data;

               pShop->keeper = pMobIndex->vnum;
               for( iTrade = 0; iTrade < MAX_TRADE; ++iTrade )
                  pShop->buy_type[iTrade] = fread_number( fp );
               pShop->profit_buy = fread_number( fp );
               pShop->profit_sell = fread_number( fp );
               pShop->profit_buy = URANGE( pShop->profit_sell + 5, pShop->profit_buy, 1000 );
               pShop->profit_sell = URANGE( 0, pShop->profit_sell, pShop->profit_buy - 5 );
               pShop->open_hour = fread_number( fp );
               pShop->close_hour = fread_number( fp );

               pMobIndex->pShop = pShop;
               shoplist.push_back( pShop );
               ++top_shop;

               break;
            }

            if( !str_cmp( word, "Speaks" ) )
            {
               flag_set( fp, pMobIndex->speaks, lang_names );

               if( pMobIndex->speaks.none(  ) )
                  pMobIndex->speaks.set( LANG_COMMON );
               break;
            }

            if( !str_cmp( word, "Speaking" ) )
            {
               string speaking, flag;
               int value;

               speaking = fread_flagstring( fp );

               speaking = one_argument( speaking, flag );
               value = get_langnum( flag );
               if( value < 0 || value >= LANG_UNKNOWN )
                  bug( "Unknown speaking language: %s", flag.c_str() );
               else
                  pMobIndex->speaking = value;

               if( !pMobIndex->speaking )
                  pMobIndex->speaking = LANG_COMMON;
               break;
            }

            if( !str_cmp( word, "Specfun" ) )
            {
               const char *temp = fread_flagstring( fp );

               if( !pMobIndex )
               {
                  bug( "%s: Specfun: Invalid mob vnum!", __func__ );
                  break;
               }
               if( !( pMobIndex->spec_fun = m_spec_lookup( temp ) ) )
               {
                  bug( "%s: Specfun: vnum %d, no spec_fun called %s.", __func__, pMobIndex->vnum, temp );
                  pMobIndex->spec_funname.clear(  );
               }
               else
                  pMobIndex->spec_funname = temp;
               break;
            }

            if( !str_cmp( word, "Stats1" ) )
            {
               const char *ln = fread_line( fp );
               int x1, x2, x3, x4, x5 = 150, x6 = 100;

               x1 = x2 = x3 = x4 = 0;
               sscanf( ln, "%d %d %d %d %d %d", &x1, &x2, &x3, &x4, &x5, &x6 );

               pMobIndex->alignment = x1;
               pMobIndex->gold = x2;
               pMobIndex->height = x3;
               pMobIndex->weight = x4;
               pMobIndex->max_move = x5;
               pMobIndex->max_mana = x6;

               if( pMobIndex->max_move < 1 )
                  pMobIndex->max_move = 150;

               if( pMobIndex->max_mana < 1 )
                  pMobIndex->max_mana = 100;
               break;
            }

            if( !str_cmp( word, "Stats2" ) )
            {
               const char *ln = fread_line( fp );
               int x1, x2, x3, x4, x5, x6, x7;
               x1 = x2 = x3 = x4 = x5 = x6 = x7 = 0;
               sscanf( ln, "%d %d %d %d %d %d %d", &x1, &x2, &x3, &x4, &x5, &x6, &x7 );

               pMobIndex->level = x1;
               pMobIndex->mobthac0 = x2;
               pMobIndex->ac = x3;
               pMobIndex->hitplus = x4;
               pMobIndex->damnodice = x5;
               pMobIndex->damsizedice = x6;
               pMobIndex->damplus = x7;

               pMobIndex->hitnodice = pMobIndex->level;
               pMobIndex->hitsizedice = 8;

               break;
            }

            if( !str_cmp( word, "Suscept" ) )
            {
               flag_set( fp, pMobIndex->susceptible, ris_flags );
               break;
            }
            break;

         case 'V':
            if( !str_cmp( word, "Vnum" ) )
            {
               bool tmpBootDb = fBootDb;
               fBootDb = false;

               int vnum = fread_number( fp );

               if( get_mob_index( vnum ) )
               {
                  if( tmpBootDb )
                  {
                     fBootDb = tmpBootDb;
                     bug( "%s: vnum %d duplicated.", __func__, vnum );

                     // Try to recover, read to end of duplicated mobile and then bail out
                     for( ;; )
                     {
                        word = feof( fp ) ? "#ENDMOBILE" : fread_word( fp );

                        if( !str_cmp( word, "#ENDMOBILE" ) )
                           return;
                     }
                  }
                  else
                  {
                     pMobIndex = get_mob_index( vnum );
                     log_printf_plus( LOG_BUILD, sysdata->build_level, "Cleaning mobile: %d", vnum );
                     pMobIndex->clean_mob(  );
                     oldmob = true;
                  }
               }
               else
               {
                  pMobIndex = new mob_index;
                  pMobIndex->clean_mob(  );
               }
               pMobIndex->vnum = vnum;
               pMobIndex->area = tarea;
               fBootDb = tmpBootDb;

               if( fBootDb )
               {
                  if( !tarea->low_vnum )
                     tarea->low_vnum = vnum;
                  if( vnum > tarea->hi_vnum )
                     tarea->hi_vnum = vnum;
               }
               break;
            }
            break;
      }
   }
}

void fread_afk_object( FILE * fp, area_data * tarea )
{
   obj_index *pObjIndex = nullptr;
   bool oldobj = false;

   for( ;; )
   {
      const char *word = ( feof( fp ) ? "#ENDOBJECT" : fread_word( fp ) );

      if( word[0] == '\0' )
      {
         log_printf( "%s: EOF encountered reading file!", __func__ );
         word = "#ENDOBJECT";
      }

      switch ( word[0] )
      {
         default:
            bug( "%s: no match: %s", __func__, word );
            fread_to_eol( fp );
            break;

         case '#':
            if( !str_cmp( word, "#ENDOBJECT" ) )
            {
               if( !oldobj )
               {
                  obj_index_table.insert( map < int, obj_index * >::value_type( pObjIndex->vnum, pObjIndex ) );
                  tarea->objects.push_back( pObjIndex );
                  ++top_obj_index;
               }
               return;
            }

            if( !str_cmp( word, "#EXDESC" ) )
            {
               extra_descr_data *ed = fread_afk_exdesc( fp );
               if( ed )
               {
                  pObjIndex->extradesc.push_back( ed );
                  ++top_ed;
               }
               break;
            }

            if( !str_cmp( word, "#MUDPROG" ) )
            {
               mud_prog_data *mprg = new mud_prog_data;
               fread_afk_mudprog( fp, mprg, pObjIndex );
               pObjIndex->mudprogs.push_back( mprg );
               ++top_prog;
               break;
            }
            break;

         case 'A':
            if( !str_cmp( word, "Action" ) )
            {
               const char *desc = fread_flagstring( fp );

               if( desc && desc[0] != '\0' && str_cmp( desc, "(null)" ) )
                  pObjIndex->action_desc = STRALLOC( desc );
               break;
            }

            if( !str_cmp( word, "AffData" ) )
            {
               affect_data *af = fread_afk_affect( fp );

               if( af )
                  pObjIndex->affects.push_back( af );
               break;
            }
            break;

         case 'F':
            if( !str_cmp( word, "Flags" ) )
            {
               flag_set( fp, pObjIndex->extra_flags, o_flags );
               break;
            }
            break;

         case 'K':
            KEY( "Keywords", pObjIndex->name, fread_string( fp ) );
            break;

         case 'L':
            if( !str_cmp( word, "Long" ) )
            {
               const char *desc = fread_flagstring( fp );

               if( desc && desc[0] != '\0' && str_cmp( desc, "(null)" ) )
                  pObjIndex->objdesc = STRALLOC( desc );
               break;
            }
            break;

         case 'S':
            KEY( "Short", pObjIndex->short_descr, fread_string( fp ) );

            if( !str_cmp( word, "Spells" ) )
            {
               switch ( pObjIndex->item_type )
               {
                  default:
                     break;

                  case ITEM_PILL:
                  case ITEM_POTION:
                  case ITEM_SCROLL:
                     pObjIndex->value[1] = skill_lookup( fread_word( fp ) );
                     pObjIndex->value[2] = skill_lookup( fread_word( fp ) );
                     pObjIndex->value[3] = skill_lookup( fread_word( fp ) );
                     break;

                  case ITEM_STAFF:
                  case ITEM_WAND:
                     pObjIndex->value[3] = skill_lookup( fread_word( fp ) );
                     break;

                  case ITEM_SALVE:
                     pObjIndex->value[4] = skill_lookup( fread_word( fp ) );
                     pObjIndex->value[5] = skill_lookup( fread_word( fp ) );
                     break;
               }
               break;
            }

            if( !str_cmp( word, "Stats" ) )
            {
               char temp[3][MSL];
               const char *ln = fread_line( fp );
               int x1, x2, x3, x4, x5;

               x1 = x2 = x3 = x5 = 0;
               x4 = 9999;
               temp[0][0] = '\0';
               temp[1][0] = '\0';
               temp[2][0] = '\0';

               sscanf( ln, "%d %d %d %d %d %s %s %s", &x1, &x2, &x3, &x4, &x5, temp[0], temp[1], temp[2] );
               pObjIndex->weight = x1;
               pObjIndex->weight = UMAX( 1, pObjIndex->weight );
               pObjIndex->cost = x2;
               pObjIndex->ego = x3;
               pObjIndex->limit = x4;
               pObjIndex->layers = x5;

               if( temp[0][0] == '\0' )
                  pObjIndex->socket[0] = STRALLOC( "None" );
               else
                  pObjIndex->socket[0] = STRALLOC( temp[0] );

               if( temp[1][0] == '\0' )
                  pObjIndex->socket[1] = STRALLOC( "None" );
               else
                  pObjIndex->socket[1] = STRALLOC( temp[1] );

               if( temp[2][0] == '\0' )
                  pObjIndex->socket[2] = STRALLOC( "None" );
               else
                  pObjIndex->socket[2] = STRALLOC( temp[2] );

               if( pObjIndex->ego > 90 )
                  pObjIndex->ego = -2;

               break;
            }
            break;

         case 'T':
            if( !str_cmp( word, "Type" ) )
            {
               int value = get_otype( fread_flagstring( fp ) );

               if( value < 0 )
               {
                  bug( "%s: vnum %d: Object has invalid type! Defaulting to trash.", __func__, pObjIndex->vnum );
                  value = get_otype( "trash" );
               }
               pObjIndex->item_type = value;
               break;
            }
            break;

         case 'V':
            if( !str_cmp( word, "Values" ) )
            {
               const char *ln = fread_line( fp );
               int x1, x2, x3, x4, x5, x6, x7, x8, x9, x10, x11;
               x1 = x2 = x3 = x4 = x5 = x6 = x7 = x8 = x9 = x10 = x11 = 0;

               sscanf( ln, "%d %d %d %d %d %d %d %d %d %d %d", &x1, &x2, &x3, &x4, &x5, &x6, &x7, &x8, &x9, &x10, &x11 );

               if( x1 == 0 && ( pObjIndex->item_type == ITEM_WEAPON || pObjIndex->item_type == ITEM_MISSILE_WEAPON ) )
               {
                  x1 = sysdata->initcond;
                  x7 = x1;
               }

               if( x1 == 0 && pObjIndex->item_type == ITEM_PROJECTILE )
               {
                  x1 = sysdata->initcond;
                  x6 = x1;
               }

               pObjIndex->value[0] = x1;
               pObjIndex->value[1] = x2;
               pObjIndex->value[2] = x3;
               pObjIndex->value[3] = x4;
               pObjIndex->value[4] = x5;
               pObjIndex->value[5] = x6;
               pObjIndex->value[6] = x7;
               pObjIndex->value[7] = x8;
               pObjIndex->value[8] = x9;
               pObjIndex->value[9] = x10;
               pObjIndex->value[10] = x11;

               break;
            }

            if( !str_cmp( word, "Vnum" ) )
            {
               bool tmpBootDb = fBootDb;
               fBootDb = false;

               int vnum = fread_number( fp );

               if( get_obj_index( vnum ) )
               {
                  if( tmpBootDb )
                  {
                     fBootDb = tmpBootDb;
                     bug( "%s: vnum %d duplicated.", __func__, vnum );

                     // Try to recover, read to end of duplicated object and then bail out
                     for( ;; )
                     {
                        word = feof( fp ) ? "#ENDOBJECT" : fread_word( fp );

                        if( !str_cmp( word, "#ENDOBJECT" ) )
                           return;
                     }
                  }
                  else
                  {
                     pObjIndex = get_obj_index( vnum );
                     log_printf_plus( LOG_BUILD, sysdata->build_level, "Cleaning object: %d", vnum );
                     pObjIndex->clean_obj(  );
                     oldobj = true;
                  }
               }
               else
               {
                  pObjIndex = new obj_index;
                  pObjIndex->clean_obj(  );
               }
               pObjIndex->vnum = vnum;
               pObjIndex->area = tarea;
               fBootDb = tmpBootDb;

               if( fBootDb )
               {
                  if( !tarea->low_vnum )
                     tarea->low_vnum = vnum;
                  if( vnum > tarea->hi_vnum )
                     tarea->hi_vnum = vnum;
               }
               break;
            }
            break;

         case 'W':
            if( !str_cmp( word, "WFlags" ) )
            {
               flag_set( fp, pObjIndex->wear_flags, w_flags );
               break;
            }
            break;
      }
   }
}

void fread_afk_room( FILE * fp, area_data * tarea )
{
   room_index *pRoomIndex = nullptr;
   bool oldroom = false;

   for( ;; )
   {
      const char *word = ( feof( fp ) ? "#ENDROOM" : fread_word( fp ) );

      if( word[0] == '\0' )
      {
         log_printf( "%s: EOF encountered reading file!", __func__ );
         if( fBootDb )
            exit( 1 );

         word = "#ENDROOM";
      }

      switch ( word[0] )
      {
         default:
            bug( "%s: no match: %s", __func__, word );
            fread_to_eol( fp );
            break;

         case '#':
            if( !str_cmp( word, "#ENDROOM" ) )
            {
               if( !oldroom )
               {
                  room_index_table.insert( map < int, room_index * >::value_type( pRoomIndex->vnum, pRoomIndex ) );
                  tarea->rooms.push_back( pRoomIndex );
                  ++top_room;
               }
               return;
            }

            if( !str_cmp( word, "#EXIT" ) )
            {
               fread_afk_exit( fp, pRoomIndex );
               break;
            }

            if( !str_cmp( word, "#EXDESC" ) )
            {
               extra_descr_data *ed = fread_afk_exdesc( fp );
               if( ed )
                  pRoomIndex->extradesc.push_back( ed );
               break;
            }

            if( !str_cmp( word, "#MUDPROG" ) )
            {
               mud_prog_data *mprg = new mud_prog_data;
               fread_afk_mudprog( fp, mprg, pRoomIndex );
               pRoomIndex->mudprogs.push_back( mprg );
               ++top_prog;
               break;
            }
            break;

         case 'A':
            if( !str_cmp( word, "AffData" ) )
            {
               affect_data *af = fread_afk_affect( fp );

               if( af )
                  pRoomIndex->permaffects.push_back( af );
               break;
            }
            break;

         case 'D':
            KEY( "Desc", pRoomIndex->roomdesc, fread_string_nohash( fp ) );
            break;

         case 'F':
            if( !str_cmp( word, "Flags" ) )
            {
               flag_set( fp, pRoomIndex->flags, r_flags );
               break;
            }
            break;

         case 'N':
            KEY( "Name", pRoomIndex->name, fread_string( fp ) );
            KEY( "Nightdesc", pRoomIndex->nitedesc, fread_string_nohash( fp ) );
            break;

         case 'R':
            CLKEY( "Reset", pRoomIndex->load_reset( fp, true ) );
            break;

         case 'S':
            if( !str_cmp( word, "Sector" ) )
            {
               int sector = get_sectypes( fread_flagstring( fp ) );

               if( sector < 0 || sector >= SECT_MAX )
               {
                  bug( "%s: Room #%d has bad sector type.", __func__, pRoomIndex->vnum );
                  sector = 1;
               }

               pRoomIndex->sector_type = sector;
               pRoomIndex->winter_sector = -1;
               break;
            }

            if( !str_cmp( word, "Stats" ) )
            {
               const char *ln = fread_line( fp );
               int x1, x2, x3, x4, x5;

               x1 = x2 = x3 = x4 = 0;
               x5 = 100000;
               sscanf( ln, "%d %d %d %d %d", &x1, &x2, &x3, &x4, &x5 );

               pRoomIndex->tele_delay = x1;
               pRoomIndex->tele_vnum = x2;
               pRoomIndex->tunnel = x3;
               pRoomIndex->baselight = x4;
               pRoomIndex->light = x4;
               pRoomIndex->max_weight = x5;  // Imported from Smaug 1.8
               break;
            }
            break;

         case 'V':
            if( !str_cmp( word, "Vnum" ) )
            {
               bool tmpBootDb = fBootDb;
               fBootDb = false;

               int vnum = fread_number( fp );

               if( get_room_index( vnum ) )
               {
                  if( tmpBootDb )
                  {
                     fBootDb = tmpBootDb;
                     bug( "%s: vnum %d duplicated.", __func__, vnum );

                     // Try to recover, read to end of duplicated room and then bail out
                     for( ;; )
                     {
                        word = feof( fp ) ? "#ENDROOM" : fread_word( fp );

                        if( !str_cmp( word, "#ENDROOM" ) )
                           return;
                     }
                  }
                  else
                  {
                     pRoomIndex = get_room_index( vnum );
                     log_printf_plus( LOG_BUILD, sysdata->build_level, "Cleaning room: %d", vnum );
                     pRoomIndex->clean_room(  );
                     oldroom = true;
                  }
               }
               else
               {
                  pRoomIndex = new room_index;
                  pRoomIndex->clean_room(  );
               }
               pRoomIndex->vnum = vnum;
               pRoomIndex->area = tarea;
               fBootDb = tmpBootDb;

               if( fBootDb )
               {
                  if( !tarea->low_vnum )
                     tarea->low_vnum = vnum;
                  if( vnum > tarea->hi_vnum )
                     tarea->hi_vnum = vnum;
               }

               if( !pRoomIndex->resets.empty(  ) )
               {
                  if( fBootDb )
                  {
                     int count = 0;
                     list < reset_data * >::iterator rst;

                     bug( "%s: WARNING: resets already exist for this room.", __func__ );
                     for( rst = pRoomIndex->resets.begin(  ); rst != pRoomIndex->resets.end(  ); ++rst )
                     {
                        reset_data *rtmp = *rst;

                        ++count;
                        if( !rtmp->resets.empty(  ) )
                           count += rtmp->resets.size(  );
                     }
                  }
                  else
                  {
                     /*
                      * Clean out the old resets
                      */
                     log_printf_plus( LOG_BUILD, sysdata->build_level, "Cleaning resets: %s", tarea->name );
                     pRoomIndex->clean_resets(  );
                  }
               }
               break;
            }
            break;

         case 'W':
            if( !str_cmp( word, "WeatherCoords" ) )
            {
               tarea->weatherx = fread_number( fp );
               tarea->weathery = fread_number( fp );
            }
            break;
      }
   }
}

// AFKMud 2.0 Area file format. Liberal use of KEY macro support. Far more flexible.
area_data *fread_afk_area( FILE * fp )
{
   area_data *tarea = nullptr;

   for( ;; )
   {
      char letter;
      const char *word;

      letter = fread_letter( fp );
      if( letter == '*' )
      {
         fread_to_eol( fp );
         continue;
      }

      if( letter != '#' )
      {
         bug( "%s: # not found. Invalid format.", __func__ );
         if( fBootDb )
            exit( 1 );
         break;
      }

      word = ( feof( fp ) ? "ENDAREA" : fread_word( fp ) );

      if( word[0] == '\0' )
      {
         bug( "%s: EOF encountered reading file!", __func__ );
         word = "ENDAREA";
      }

      if( !str_cmp( word, "AREADATA" ) )
      {
         tarea = create_area(  );
         tarea->filename = str_dup( strArea );
         fread_afk_areadata( fp, tarea );
      }
      else if( !str_cmp( word, "MOBILE" ) )
         fread_afk_mobile( fp, tarea );
      else if( !str_cmp( word, "OBJECT" ) )
         fread_afk_object( fp, tarea );
      else if( !str_cmp( word, "ROOM" ) )
         fread_afk_room( fp, tarea );
      else if( !str_cmp( word, "ENDAREA" ) )
         break;
      else
      {
         bug( "%s: Bad section header: %s", __func__, word );
         fread_to_eol( fp );
      }
   }
   return tarea;
}

void process_sorting( area_data * tarea, bool isproto )
{
   tarea->sort_name(  );
   tarea->sort_vnums(  );
   if( isproto )
      tarea->flags.set( AFLAG_PROTOTYPE );
   log_printf( "%-20s: Version %-3d Vnums: %5d - %-5d", tarea->filename, tarea->version, tarea->low_vnum, tarea->hi_vnum );
   if( tarea->low_vnum < 0 || tarea->hi_vnum < 0 )
      log_printf( "%-20s: Bad Vnum Range", tarea->filename );
   if( !tarea->author )
      tarea->author = STRALLOC( "AFKMud" );
}

void load_area_file( const string & filename, bool isproto )
{
   area_data *tarea = nullptr;
   char *word;

   if( !( fpArea = fopen( filename.c_str(  ), "r" ) ) )
   {
      perror( filename.c_str(  ) );
      bug( "%s: error loading file (can't open) %s", __func__, filename.c_str(  ) );
      return;
   }

   if( fread_letter( fpArea ) != '#' )
   {
      if( fBootDb )
      {
         bug( "%s: No # found at start of area file.", __func__ );
         exit( 1 );
      }
      else
      {
         bug( "%s: No # found at start of area file.", __func__ );
         FCLOSE( fpArea );
         return;
      }
   }

   word = fread_word( fpArea );

   // New AFKMud area format support -- Samson 10/28/06
   if( !str_cmp( word, "AFKAREA" ) )
   {
      tarea = fread_afk_area( fpArea );
      FCLOSE( fpArea );

      if( tarea )
         process_sorting( tarea, isproto );
      return;
   }

   // Support conversions from other bases if encountered. -- Samson 12/31/06
   log_printf( "Area format conversion: %s", filename.c_str(  ) );

   if( !str_cmp( word, "FUSSAREA" ) )
   {
      log_string( "SmaugFUSS 1.7+ area encountered. Passing to SmaugFUSS area parser." );

      tarea = fread_smaugfuss_area( fpArea );
      FCLOSE( fpArea );

      if( tarea )
         process_sorting( tarea, isproto );
      return;
   }

   // Found #AREA, let the process begin!
   if( !str_cmp( word, "AREA" ) )
   {
      tarea = create_area();

      tarea->filename = str_dup( strArea );
      tarea->name = fread_string_nohash( fpArea );
   }
   else
   {
      // If you found #VERSION first though, it's a SmaugWiz zone or it's invalid.
      if( !str_cmp( word, "VERSION" ) )
      {
         int temp = fread_number( fpArea );
         if( temp >= 1000 )
         {
            FCLOSE( fpArea );
            do_areaconvert( nullptr, filename );
            return;
         }

         bug( "%s: Invalid header at start of area file: %s", __func__, word );
         if( fBootDb )
            exit( 1 );
         else
         {
            FCLOSE( fpArea );
            return;
         }
      }

      bug( "%s: Invalid header at start of area file: %s", __func__, word );
      if( fBootDb )
         exit( 1 );
      else
      {
         FCLOSE( fpArea );
         return;
      }
   }

   for( ;; )
   {
      if( !fpArea )  /* Should only happen if a stock conversion takes place */
         return;

      if( fread_letter( fpArea ) != '#' )
      {
         bug( "%s: # not found %s", __func__, tarea->filename );
         exit( 1 );
      }

      word = fread_word( fpArea );

      if( word[0] == '$' )
         break;

      else if( !str_cmp( word, "VERSION" ) )
      {
         int aversion = fread_number( fpArea );

         if( aversion <= 3 )
         {
            log_string( "Smaug 1.02a, 1.4a, or 1.8b area encountered. Attempting to pass to Area Convertor." );
            FCLOSE( fpArea );
            deleteptr( tarea );
            --top_area;
            do_areaconvert( nullptr, filename );
            return;
         }

         if( aversion == 1000 )
         {
            log_string( "SmaugWiz 2.02 area encountered. Attempting to pass to Area Convertor." );
            FCLOSE( fpArea );
            deleteptr( tarea );
            do_areaconvert( nullptr, filename );
            return;
         }

         // AFKMud 1.x files only went up to version 20. Anything higher is invalid.
         if( aversion > 3 && aversion <= 20 )
         {
            bool oldafk_fail = load_oldafk_area( fpArea, tarea, aversion );

            if( !oldafk_fail )
               break;
            else
            {
               bug( "%s: Bad conversion from AFKMud 1.x format. Cannot continue.", __func__ );
               if( fBootDb )
                  exit( 1 );
               else
               {
                  FCLOSE( fpArea );
               }
            }
            return;
         }

         bug( "%s: %s:%d - Bad version header. Format conversion failed.", __func__, word, aversion );
         if( fBootDb )
            exit( 1 );
         else
         {
            FCLOSE( fpArea );
            return;
         }
      }
      else
      {
         bug( "%s: %s bad section name %s", __func__, tarea->filename, word );
         if( fBootDb )
            exit( 1 );
         else
         {
            FCLOSE( fpArea );
            return;
         }
      }
   }
   FCLOSE( fpArea );

   if( tarea )
      process_sorting( tarea, isproto );
   else
      log_printf( "(%s)", filename.c_str(  ) );
}

void fwrite_afk_affect( FILE * fpout, affect_data * af )
{
   if( af->location == APPLY_AFFECT )
      fprintf( fpout, "AffData %s '%s' %d %d %d\n", a_types[af->location], aff_flags[af->modifier], af->type, af->duration, af->bit );
   else if( af->location == APPLY_WEAPONSPELL
            || af->location == APPLY_WEARSPELL
            || af->location == APPLY_REMOVESPELL || af->location == APPLY_STRIPSN || af->location == APPLY_RECURRINGSPELL || af->location == APPLY_EAT_SPELL )
      fprintf( fpout, "AffData %s '%s' %d %d %d\n", a_types[af->location],
               IS_VALID_SN( af->modifier ) ? skill_table[af->modifier]->name : "UNKNOWN", af->type, af->duration, af->bit );
   else if( af->location == APPLY_RESISTANT || af->location == APPLY_IMMUNE || af->location == APPLY_SUSCEPTIBLE || af->location == APPLY_ABSORB )
      fprintf( fpout, "AffData %s %s~ %d %d %d\n", a_types[af->location], bitset_string( af->rismod, ris_flags ), af->type, af->duration, af->bit );
   else
      fprintf( fpout, "AffData %s %d %d %d %d\n", a_types[af->location], af->modifier, af->type, af->duration, af->bit );
}

void fwrite_afk_exdesc( FILE * fpout, extra_descr_data * desc )
{
   fprintf( fpout, "%s", "#EXDESC\n" );
   fprintf( fpout, "ExDescKey    %s~\n", desc->keyword.c_str(  ) );
   if( !desc->desc.empty(  ) )
      fprintf( fpout, "ExDesc       %s~\n", strip_cr( desc->desc ).c_str(  ) );
   fprintf( fpout, "%s", "#ENDEXDESC\n\n" );
}

void fwrite_afk_exit( FILE * fpout, exit_data * pexit )
{
   fprintf( fpout, "%s", "#EXIT\n" );
   fprintf( fpout, "Direction %s~\n", strip_cr( dir_name[pexit->vdir] ) );
   fprintf( fpout, "ToRoom    %d\n", pexit->vnum );
   if( pexit->key > 0 )
      fprintf( fpout, "Key       %d\n", pexit->key );
   if( IS_EXIT_FLAG( pexit, EX_OVERLAND ) && pexit->map_x != -1 && pexit->map_y != -1 )
      fprintf( fpout, "ToCoords  %d %d\n", pexit->map_x, pexit->map_y );
   if( pexit->pull )
      fprintf( fpout, "Pull      %d %d\n", pexit->pulltype, pexit->pull );
   if( pexit->exitdesc && pexit->exitdesc[0] != '\0' )
      fprintf( fpout, "Desc      %s~\n", strip_cr( pexit->exitdesc ) );
   if( pexit->keyword && pexit->keyword[0] != '\0' )
      fprintf( fpout, "Keywords  %s~\n", strip_cr( pexit->keyword ) );
   if( pexit->flags.any(  ) )
      fprintf( fpout, "Flags     %s~\n", bitset_string( pexit->flags, ex_flags ) );
   fprintf( fpout, "%s", "#ENDEXIT\n\n" );
}

// Write a prog
bool mprog_write_prog( FILE * fpout, mud_prog_data * mprog )
{
   if( ( mprog->arglist && mprog->arglist[0] != '\0' ) )
   {
      fprintf( fpout, "%s", "#MUDPROG\n" );
      fprintf( fpout, "Progtype  %s~\n", mprog_type_to_name( mprog->type ).c_str(  ) );
      fprintf( fpout, "Arglist   %s~\n", mprog->arglist );

      if( mprog->comlist && mprog->comlist[0] != '\0' && !mprog->fileprog )
         fprintf( fpout, "Comlist   %s~\n", strip_cr( mprog->comlist ) );

      fprintf( fpout, "%s", "#ENDPROG\n\n" );
      return true;
   }
   return false;
}

void save_reset_level( FILE * fpout, list < reset_data * >source, const int level )
{
   list < reset_data * >::iterator rst;
   int spaces = level * 2;

   for( rst = source.begin(  ); rst != source.end(  ); ++rst )
   {
      reset_data *pReset = *rst;

      switch ( UPPER( pReset->command ) ) /* extra arg1 arg2 arg3 */
      {
         default:
            bug( "%s: Invalid reset type %c", __func__, UPPER( pReset->command ) );
            break;

         case '*':
            break;

         case 'Z':  // RT room object - no sub-resets
            fprintf( fpout, "%*.sReset %c %d %d %d %d %d %d %d %d %d %d %d\n", spaces, "", UPPER( pReset->command ),
                     pReset->arg1, pReset->arg2, pReset->arg3, pReset->arg4, pReset->arg5, pReset->arg6,
                     pReset->arg7, pReset->arg8, pReset->arg9, pReset->arg10, pReset->arg11 );
            break;

         case 'M':
         case 'O':
         case 'Y':  // RT give - has no sub-resets
            fprintf( fpout, "%*.sReset %c %d %d %d %d %d %d %d\n", spaces, "", UPPER( pReset->command ),
                     pReset->arg1, pReset->arg2, pReset->arg3, pReset->arg4, pReset->arg5, pReset->arg6, pReset->arg7 );
            break;

         case 'W':  // RT put - has no sub-resets
            fprintf( fpout, "%*.sReset %c %d %d %d %d %d %d %d %d %d\n", spaces, "", UPPER( pReset->command ),
                     pReset->arg1, pReset->arg2, pReset->arg3, pReset->arg4, pReset->arg5, pReset->arg6, pReset->arg7, pReset->arg8, pReset->arg9 );
            break;

         case 'P':
         case 'E':
            if( pReset->command == 'E' )
               fprintf( fpout, "%*.sReset %c %d %d %d %d\n", spaces, "", UPPER( pReset->command ), pReset->arg1, pReset->arg2, pReset->arg3, pReset->arg4 );
            else
               fprintf( fpout, "%*.sReset %c %d %d %d %d %d\n", spaces, "", UPPER( pReset->command ), pReset->arg1, pReset->arg2, pReset->arg3, pReset->arg4, pReset->arg5 );
            break;

         case 'G':
         case 'R':
            fprintf( fpout, "%*.sReset %c %d %d %d\n", spaces, "", UPPER( pReset->command ), pReset->arg1, pReset->arg2, pReset->arg3 );
            break;

         case 'X':  // RT equipped - has no sub-resets
            fprintf( fpout, "%*.sReset %c %d %d %d %d %d %d %d %d\n", spaces, "", UPPER( pReset->command ),
                     pReset->arg1, pReset->arg2, pReset->arg3, pReset->arg4, pReset->arg5, pReset->arg6, pReset->arg7, pReset->arg8 );
            break;

         case 'T':
         case 'H':
         case 'D':
            fprintf( fpout, "%*.sReset %c %d %d %d %d %d\n", spaces, "", UPPER( pReset->command ), pReset->arg1, pReset->arg2, pReset->arg3, pReset->arg4, pReset->arg5 );
            break;
      }  /* end of switch on command */

      /*
       * recurse to save nested resets 
       */
      if( !pReset->resets.empty(  ) )
         save_reset_level( fpout, pReset->resets, level + 1 );
   }  /* end of looping through resets */
}  /* end of save_reset_level */

// Write out the top header for the file.
void fwrite_area_header( area_data * area, FILE * fpout )
{
   fprintf( fpout, "%s", "#AREADATA\n" );
   fprintf( fpout, "Version         %d\n", area->version );
   fprintf( fpout, "Name            %s~\n", area->name );
   fprintf( fpout, "Author          %s~\n", area->author );
   if( area->credits )
      fprintf( fpout, "Credits         %s~\n", area->credits );
   fprintf( fpout, "Vnums           %d %d\n", area->low_vnum, area->hi_vnum );
   if( area->continent )
      fprintf( fpout, "Continent       %s~\n", area->continent->name.c_str( ) );
   fprintf( fpout, "Coordinates     %d %d\n", area->map_x, area->map_y );
   fprintf( fpout, "Dates           %ld %ld\n", area->creation_date, area->install_date );
   fprintf( fpout, "Ranges          %d %d %d %d\n", area->low_soft_range, area->hi_soft_range, area->low_hard_range, area->hi_hard_range );
   if( area->resetmsg ) /* Rennard */
      fprintf( fpout, "ResetMsg        %s~\n", area->resetmsg );
   if( area->reset_frequency )
      fprintf( fpout, "ResetFreq       %d\n", area->reset_frequency );
   if( area->flags.any(  ) )
      fprintf( fpout, "Flags           %s~\n", bitset_string( area->flags, area_flags ) );
   fprintf( fpout, "Treasure        %d %d %d %d %d %d %d %d\n",
            area->tg_nothing, area->tg_gold, area->tg_item, area->tg_gem, area->tg_scroll, area->tg_potion, area->tg_wand, area->tg_armor );
   fprintf( fpout, "WeatherCoords   %d %d\n\n", area->weatherx, area->weathery );

   fprintf( fpout, "%s", "#ENDAREADATA\n\n" );
}

// Write out an individual mob
void fwrite_afk_mobile( FILE * fpout, mob_index * pMobIndex, bool install )
{
   shop_data *pShop = nullptr;
   repair_data *pRepair = nullptr;

   if( install )
      pMobIndex->actflags.reset( ACT_PROTOTYPE );

   fprintf( fpout, "%s", "#MOBILE\n" );
   fprintf( fpout, "Vnum      %d\n", pMobIndex->vnum );
   fprintf( fpout, "Keywords  %s~\n", pMobIndex->player_name );
   fprintf( fpout, "Race      %s~\n", npc_race[pMobIndex->race] );
   fprintf( fpout, "Class     %s~\n", npc_class[pMobIndex->Class] );
   fprintf( fpout, "Gender    %s~\n", npc_sex[pMobIndex->sex] );
   fprintf( fpout, "Position  %s~\n", npc_position[pMobIndex->position] );
   fprintf( fpout, "DefPos    %s~\n", npc_position[pMobIndex->defposition] );
   if( pMobIndex->spec_fun && !pMobIndex->spec_funname.empty(  ) )
      fprintf( fpout, "Specfun   %s~\n", pMobIndex->spec_funname.c_str(  ) );
   fprintf( fpout, "Short     %s~\n", pMobIndex->short_descr );
   if( pMobIndex->long_descr && pMobIndex->long_descr[0] != '\0' )
      fprintf( fpout, "Long      %s~\n", strip_cr( pMobIndex->long_descr ) );
   if( pMobIndex->chardesc && pMobIndex->chardesc[0] != '\0' )
      fprintf( fpout, "Desc      %s~\n", strip_cr( pMobIndex->chardesc ) );
   fprintf( fpout, "Nattacks  %f\n", pMobIndex->numattacks );
   fprintf( fpout, "Stats1    %d %d %d %d %d %d\n", pMobIndex->alignment, pMobIndex->gold, pMobIndex->height, pMobIndex->weight, pMobIndex->max_move, pMobIndex->max_mana );
   fprintf( fpout, "Stats2    %d %d %d %d %d %d %d\n",
            pMobIndex->level, pMobIndex->mobthac0, pMobIndex->ac, pMobIndex->hitplus, pMobIndex->damnodice, pMobIndex->damsizedice, pMobIndex->damplus );
   if( pMobIndex->speaks.any(  ) )
      fprintf( fpout, "Speaks    %s~\n", bitset_string( pMobIndex->speaks, lang_names ) );
   fprintf( fpout, "Speaking  %s~\n", lang_names[pMobIndex->speaking] );
   if( pMobIndex->actflags.any(  ) )
      fprintf( fpout, "Actflags  %s~\n", bitset_string( pMobIndex->actflags, act_flags ) );
   if( pMobIndex->affected_by.any(  ) )
      fprintf( fpout, "Affected  %s~\n", bitset_string( pMobIndex->affected_by, aff_flags ) );
   if( pMobIndex->body_parts.any(  ) )
      fprintf( fpout, "Bodyparts %s~\n", bitset_string( pMobIndex->body_parts, part_flags ) );
   if( pMobIndex->resistant.any(  ) )
      fprintf( fpout, "Resist    %s~\n", bitset_string( pMobIndex->resistant, ris_flags ) );
   if( pMobIndex->immune.any(  ) )
      fprintf( fpout, "Immune    %s~\n", bitset_string( pMobIndex->immune, ris_flags ) );
   if( pMobIndex->susceptible.any(  ) )
      fprintf( fpout, "Suscept   %s~\n", bitset_string( pMobIndex->susceptible, ris_flags ) );
   if( pMobIndex->absorb.any(  ) )
      fprintf( fpout, "Absorb    %s~\n", bitset_string( pMobIndex->absorb, ris_flags ) );
   if( pMobIndex->attacks.any(  ) )
      fprintf( fpout, "Attacks   %s~\n", bitset_string( pMobIndex->attacks, attack_flags ) );
   if( pMobIndex->defenses.any(  ) )
      fprintf( fpout, "Defenses  %s~\n", bitset_string( pMobIndex->defenses, defense_flags ) );

   // Mob has a shop? Add that data to the mob index.
   if( ( pShop = pMobIndex->pShop ) != nullptr )
   {
      fprintf( fpout, "ShopData   %d %d %d %d %d %d %d %d %d\n",
               pShop->buy_type[0], pShop->buy_type[1], pShop->buy_type[2], pShop->buy_type[3], pShop->buy_type[4],
               pShop->profit_buy, pShop->profit_sell, pShop->open_hour, pShop->close_hour );
   }

   // Mob is a repair shop? Add that data to the mob index.
   if( ( pRepair = pMobIndex->rShop ) != nullptr )
   {
      fprintf( fpout, "RepairData %d %d %d %d %d %d %d\n",
               pRepair->fix_type[0], pRepair->fix_type[1], pRepair->fix_type[2], pRepair->profit_fix, pRepair->shop_type, pRepair->open_hour, pRepair->close_hour );
   }

   list < mud_prog_data * >::iterator mprg;
   for( mprg = pMobIndex->mudprogs.begin(  ); mprg != pMobIndex->mudprogs.end(  ); ++mprg )
   {
      mud_prog_data *mp = *mprg;
      mprog_write_prog( fpout, mp );
   }
   fprintf( fpout, "%s", "#ENDMOBILE\n\n" );
}

// Write out an individual obj
void fwrite_afk_object( FILE * fpout, obj_index * pObjIndex, bool install )
{
   if( install )
      pObjIndex->extra_flags.reset( ITEM_PROTOTYPE );

   fprintf( fpout, "%s", "#OBJECT\n" );
   fprintf( fpout, "Vnum      %d\n", pObjIndex->vnum );
   fprintf( fpout, "Keywords  %s~\n", pObjIndex->name );
   fprintf( fpout, "Type      %s~\n", o_types[pObjIndex->item_type] );
   fprintf( fpout, "Short     %s~\n", pObjIndex->short_descr );
   if( pObjIndex->objdesc && pObjIndex->objdesc[0] != '\0' )
      fprintf( fpout, "Long      %s~\n", strip_cr( pObjIndex->objdesc ) );
   if( pObjIndex->action_desc && pObjIndex->action_desc[0] != '\0' )
      fprintf( fpout, "Action    %s~\n", pObjIndex->action_desc );
   if( pObjIndex->extra_flags.any(  ) )
      fprintf( fpout, "Flags     %s~\n", bitset_string( pObjIndex->extra_flags, o_flags ) );
   if( pObjIndex->wear_flags.any(  ) )
      fprintf( fpout, "WFlags    %s~\n", bitset_string( pObjIndex->wear_flags, w_flags ) );

   int val0, val1, val2, val3, val4, val5, val6, val7, val8, val9, val10;
   val0 = pObjIndex->value[0];
   val1 = pObjIndex->value[1];
   val2 = pObjIndex->value[2];
   val3 = pObjIndex->value[3];
   val4 = pObjIndex->value[4];
   val5 = pObjIndex->value[5];
   val6 = pObjIndex->value[6];
   val7 = pObjIndex->value[7];
   val8 = pObjIndex->value[8];
   val9 = pObjIndex->value[9];
   val10 = pObjIndex->value[10];

   switch ( pObjIndex->item_type )
   {
      default:
         break;

      case ITEM_PILL:
      case ITEM_POTION:
      case ITEM_SCROLL:
         if( IS_VALID_SN( val1 ) )
            val1 = HAS_SPELL_INDEX;
         if( IS_VALID_SN( val2 ) )
            val2 = HAS_SPELL_INDEX;
         if( IS_VALID_SN( val3 ) )
            val3 = HAS_SPELL_INDEX;
         break;

      case ITEM_STAFF:
      case ITEM_WAND:
         if( IS_VALID_SN( val3 ) )
            val3 = HAS_SPELL_INDEX;
         break;

      case ITEM_SALVE:
         if( IS_VALID_SN( val4 ) )
            val4 = HAS_SPELL_INDEX;
         if( IS_VALID_SN( val5 ) )
            val5 = HAS_SPELL_INDEX;
         break;
   }
   fprintf( fpout, "Values    %d %d %d %d %d %d %d %d %d %d %d\n", val0, val1, val2, val3, val4, val5, val6, val7, val8, val9, val10 );

   fprintf( fpout, "Stats     %d %d %d %d %d %s %s %s\n",
            pObjIndex->weight, pObjIndex->cost, pObjIndex->ego,
            pObjIndex->limit, pObjIndex->layers,
            pObjIndex->socket[0] ? pObjIndex->socket[0] : "None", pObjIndex->socket[1] ? pObjIndex->socket[1] : "None", pObjIndex->socket[2] ? pObjIndex->socket[2] : "None" );

   list < affect_data * >::iterator paf;
   for( paf = pObjIndex->affects.begin(  ); paf != pObjIndex->affects.end(  ); ++paf )
   {
      affect_data *af = *paf;

      fwrite_afk_affect( fpout, af );
   }

   switch ( pObjIndex->item_type )
   {
      default:
         break;

      case ITEM_PILL:
      case ITEM_POTION:
      case ITEM_SCROLL:
         fprintf( fpout, "Spells       '%s' '%s' '%s'\n",
                  IS_VALID_SN( pObjIndex->value[1] ) ? skill_table[pObjIndex->value[1]]->name : "NONE",
                  IS_VALID_SN( pObjIndex->value[2] ) ? skill_table[pObjIndex->value[2]]->name : "NONE",
                  IS_VALID_SN( pObjIndex->value[3] ) ? skill_table[pObjIndex->value[3]]->name : "NONE" );
         break;

      case ITEM_STAFF:
      case ITEM_WAND:
         fprintf( fpout, "Spells       '%s'\n", IS_VALID_SN( pObjIndex->value[3] ) ? skill_table[pObjIndex->value[3]]->name : "NONE" );
         break;

      case ITEM_SALVE:
         fprintf( fpout, "Spells       '%s' '%s'\n",
                  IS_VALID_SN( pObjIndex->value[4] ) ? skill_table[pObjIndex->value[4]]->name : "NONE",
                  IS_VALID_SN( pObjIndex->value[5] ) ? skill_table[pObjIndex->value[5]]->name : "NONE" );
         break;
   }

   list < extra_descr_data * >::iterator ed;
   for( ed = pObjIndex->extradesc.begin(  ); ed != pObjIndex->extradesc.end(  ); ++ed )
   {
      extra_descr_data *desc = *ed;

      fwrite_afk_exdesc( fpout, desc );
   }

   list < mud_prog_data * >::iterator mprg;
   for( mprg = pObjIndex->mudprogs.begin(  ); mprg != pObjIndex->mudprogs.end(  ); ++mprg )
   {
      mud_prog_data *mp = *mprg;
      mprog_write_prog( fpout, mp );
   }
   fprintf( fpout, "%s", "#ENDOBJECT\n\n" );
}

// Write out an individual room
void fwrite_afk_room( FILE * fpout, room_index * room, bool install )
{
   if( install )
   {
      // remove prototype flag from room
      room->flags.reset( ROOM_PROTOTYPE );

      // purge room of (prototyped) mobiles
      list < char_data * >::iterator ich;
      for( ich = room->people.begin(  ); ich != room->people.end(  ); )
      {
         char_data *victim = *ich;
         ++ich;

         if( victim->isnpc(  ) && victim->has_actflag( ACT_PROTOTYPE ) )
            victim->extract( true );
      }

      // purge room of (prototyped) objects
      list < obj_data * >::iterator iobj;
      for( iobj = room->objects.begin(  ); iobj != room->objects.end(  ); )
      {
         obj_data *obj = *iobj;
         ++iobj;

         if( obj->extra_flags.test( ITEM_PROTOTYPE ) )
            obj->extract(  );
      }
   }

   fprintf( fpout, "%s", "#ROOM\n" );

   /*
    * Take off track flags before saving 
    */
   if( room->flags.test( ROOM_TRACK ) )
      room->flags.reset( ROOM_TRACK );

   fprintf( fpout, "Vnum      %d\n", room->vnum );
   fprintf( fpout, "Name      %s~\n", strip_cr( room->name ) );

   /*
    * Retain the ORIGINAL sector type to the area file - Samson 7-19-00 
    */
   if( time_info.season == SEASON_WINTER && room->winter_sector != -1 )
      room->sector_type = room->winter_sector;
   fprintf( fpout, "Sector    %s~\n", strip_cr( sect_types[room->sector_type] ) );

   /*
    * And change it back again so that the season is not disturbed in play - Samson 7-19-00 
    */
   if( time_info.season == SEASON_WINTER )
   {
      switch ( room->sector_type )
      {
         default:
            break;

         case SECT_WATER_NOSWIM:
         case SECT_WATER_SWIM:
            room->winter_sector = room->sector_type;
            room->sector_type = SECT_ICE;
            break;
      }
   }

   fprintf( fpout, "Flags     %s~\n", bitset_string( room->flags, r_flags ) );
   fprintf( fpout, "Stats     %d %d %d %d %d\n", room->tele_delay, room->tele_vnum, room->tunnel, room->baselight, room->max_weight );

   if( room->roomdesc && room->roomdesc[0] != '\0' )
      fprintf( fpout, "Desc      %s~\n", strip_cr( room->roomdesc ) );

   /*
    * write NiteDesc's -- Dracones 
    */
   if( room->nitedesc && room->nitedesc[0] != '\0' )
      fprintf( fpout, "Nightdesc %s~\n", strip_cr( room->nitedesc ) );

   // Save the list of room index affects.
   list < affect_data * >::iterator paf;
   for( paf = room->permaffects.begin(  ); paf != room->permaffects.end(  ); ++paf )
   {
      affect_data *af = *paf;

      fwrite_afk_affect( fpout, af );
   }

   list < exit_data * >::iterator ex;
   for( ex = room->exits.begin(  ); ex != room->exits.end(  ); ++ex )
   {
      exit_data *pexit = *ex;

      if( IS_EXIT_FLAG( pexit, EX_PORTAL ) ) // Don't fold portals
         continue;

      fwrite_afk_exit( fpout, pexit );
   }

   list < extra_descr_data * >::iterator ed;
   for( ed = room->extradesc.begin(  ); ed != room->extradesc.end(  ); ++ed )
   {
      extra_descr_data *desc = *ed;

      fwrite_afk_exdesc( fpout, desc );
   }

   list < mud_prog_data * >::iterator mprg;
   for( mprg = room->mudprogs.begin(  ); mprg != room->mudprogs.end(  ); ++mprg )
   {
      mud_prog_data *mp = *mprg;
      mprog_write_prog( fpout, mp );
   }

   // Recursive function that saves the nested resets.
   save_reset_level( fpout, room->resets, 0 );

   fprintf( fpout, "%s", "#ENDROOM\n\n" );
}

/*
 * The area file version in the new format. Resetting to 1 because we've defined a new header.
 * Areas written in this format should never be read by an editor expecting the old one.
 * The written file should never be altered to try and force it to read in an old system. It will fail.
 * Files read by the old system will eventually have to be saved this way so they'll all be caught at some point.
 *
 * This new format is far more flexible and does not require all settings to be written to the file.
 * It behaves in much the same way as the pfile read/write code in save.cpp.
 * As a result, it is very fault tolerant to formatting errors when edited offline.
 * It can also quite often tolerate the addition of new fields without breaking things.
 * Removing existing ones wil require updated version handling unless you like dirty logs.
 *
 * Version 1: Initial construction of new format -- Samson 10/28/06
 */
const int AREA_VERSION_WRITE = 1;

void area_data::fold( const char *fname, bool install )
{
   char buf[256];
   FILE *fpout;
   list < mob_index * >::iterator mindex;
   list < obj_index * >::iterator oindex;
   list < room_index * >::iterator rindex;

   log_printf_plus( LOG_BUILD, LEVEL_GREATER, "Saving %s...", this->filename );

   snprintf( buf, 256, "%s.bak", fname );
   rename( fname, buf );
   if( !( fpout = fopen( fname, "w" ) ) )
   {
      bug( "%s: fopen", __func__ );
      perror( fname );
      return;
   }

   this->version = AREA_VERSION_WRITE;

   fprintf( fpout, "%s", "#AFKAREA\n" );

   fwrite_area_header( this, fpout );

   for( mindex = mobs.begin(  ); mindex != mobs.end(  ); ++mindex )
   {
      mob_index *pMobIndex = *mindex;
      fwrite_afk_mobile( fpout, pMobIndex, install );
   }

   for( oindex = objects.begin(  ); oindex != objects.end(  ); ++oindex )
   {
      obj_index *pObjIndex = *oindex;
      fwrite_afk_object( fpout, pObjIndex, install );
   }

   for( rindex = rooms.begin(  ); rindex != rooms.end(  ); ++rindex )
   {
      room_index *pRoomIndex = *rindex;
      fwrite_afk_room( fpout, pRoomIndex, install );
   }

   fprintf( fpout, "%s", "#ENDAREA\n" );
   FCLOSE( fpout );
}

void close_all_areas( void )
{
   list < area_data * >::iterator iarea;

   for( iarea = arealist.begin(  ); iarea != arealist.end(  ); )
   {
      area_data *pArea = *iarea;
      ++iarea;

      deleteptr( pArea );
   }
}

CMDF( do_savearea )
{
   area_data *tarea;
   char fname[256];

   if( ch->isnpc(  ) || ch->get_trust(  ) < LEVEL_CREATOR || !ch->pcdata || ( argument[0] == '\0' && !ch->pcdata->area ) )
   {
      ch->print( "&[immortal]You don't have an assigned area to save.\r\n" );
      return;
   }

   if( argument.empty(  ) )
      tarea = ch->pcdata->area;
   else if( ch->is_imp(  ) && !str_cmp( argument, "all" ) )
   {
      list < area_data * >::iterator iarea;

      for( iarea = arealist.begin(  ); iarea != arealist.end(  ); ++iarea )
      {
         tarea = *iarea;

         if( !tarea->flags.test( AFLAG_PROTOTYPE ) )
            continue;
         snprintf( fname, 256, "%s%s", BUILD_DIR, tarea->filename );
         tarea->fold( fname, false );
      }
      ch->print( "&[immortal]Prototype areas saved.\r\n" );
      return;
   }
   else
   {
      if( ch->get_trust(  ) < LEVEL_GOD )
      {
         ch->print( "&[immortal]You can only save your own area.\r\n" );
         return;
      }

      if( !( tarea = find_area( argument ) ) )
      {
         ch->print( "&[immortal]Area not found.\r\n" );
         return;
      }
   }

   if( !tarea )
   {
      ch->print( "&[immortal]No area to save.\r\n" );
      return;
   }

   if( !tarea->flags.test( AFLAG_PROTOTYPE ) )
   {
      ch->printf( "&[immortal]Cannot savearea %s, use foldarea instead.\r\n", tarea->filename );
      return;
   }
   snprintf( fname, 256, "%s%s", BUILD_DIR, tarea->filename );
   ch->print( "&[immortal]Saving area...\r\n" );
   tarea->fold( fname, false );
   ch->print( "&[immortal]Done.\r\n" );
}

CMDF( do_foldarea )
{
   area_data *tarea;
   ch->set_color( AT_IMMORT );

   if( argument.empty(  ) )
   {
      ch->print( "Fold what?\r\n" );
      return;
   }

   if( !str_cmp( argument, "all" ) )
   {
      list < area_data * >::iterator iarea;

      for( iarea = arealist.begin(  ); iarea != arealist.end(  ); ++iarea )
      {
         tarea = *iarea;

         if( !tarea->flags.test( AFLAG_PROTOTYPE ) )
            tarea->fold( tarea->filename, false );
      }
      ch->print( "&[immortal]Folding completed.\r\n" );
      return;
   }

   if( !( tarea = find_area( argument ) ) )
   {
      ch->print( "No such area exists.\r\n" );
      return;
   }

   if( tarea->flags.test( AFLAG_PROTOTYPE ) )
   {
      ch->printf( "Cannot fold %s, use savearea instead.\r\n", tarea->filename );
      return;
   }
   ch->print( "Folding area...\r\n" );
   tarea->fold( tarea->filename, false );
   ch->print( "&[immortal]Done.\r\n" );
}

void write_area_list( void )
{
   list < area_data * >::iterator iarea;
   FILE *fpout;

   fpout = fopen( AREA_LIST, "w" );
   if( !fpout )
   {
      bug( "%s: cannot open area.lst for writing!", __func__ );
      return;
   }

   for( iarea = arealist.begin(  ); iarea != arealist.end(  ); ++iarea )
   {
      area_data *area = *iarea;

      if( !area->flags.test( AFLAG_PROTOTYPE ) )
         fprintf( fpout, "%s\n", area->filename );
   }
   fprintf( fpout, "%s", "$\n" );
   FCLOSE( fpout );
}

/*
 * returns area with name matching input string
 * Last Modified : July 21, 1997
 * Fireblade
 */
area_data *get_area( const string & name )
{
   if( name.empty(  ) )
      return nullptr;

   list < area_data * >::iterator iarea;
   for( iarea = arealist.begin(  ); iarea != arealist.end(  ); ++iarea )
   {
      area_data *area = *iarea;

      if( hasname( area->name, name ) )
         return area;
   }
   return nullptr;
}

/* Locate an area by its filename first, then fall back to other means if not found */
area_data *find_area( const string & filename )
{
   list < area_data * >::iterator iarea;

   for( iarea = arealist.begin(  ); iarea != arealist.end(  ); ++iarea )
   {
      area_data *area = *iarea;

      if( !str_cmp( area->filename, filename ) )
         return area;
   }
   return get_area( filename );
}

CMDF( do_adelete )
{
   area_data *tarea;
   string arg;
   char filename[256];

   if( argument.empty(  ) )
   {
      ch->print( "Usage: adelete <areafilename>\r\n" );
      return;
   }

   argument = one_argument( argument, arg );

   if( !( tarea = find_area( arg ) ) )
   {
      ch->printf( "No such area as %s\r\n", arg.c_str(  ) );
      return;
   }

   if( argument.empty(  ) || str_cmp( argument, "yes" ) )
   {
      ch->print( "&RThis action must be confirmed before executing. It is not reversable.\r\n" );
      ch->printf( "&RTo delete this area, type: &Wadelete %s yes&D", arg.c_str(  ) );
      return;
   }
   if( tarea->flags.test( AFLAG_PROTOTYPE ) )
      snprintf( filename, 256, "%s%s", BUILD_DIR, tarea->filename );
   else
      strlcpy( filename, tarea->filename, 256 );
   deleteptr( tarea );
   unlink( filename );
   write_area_list(  );
   web_arealist(  );
   ch->printf( "&W%s&R has been destroyed.&D\r\n", arg.c_str(  ) );
}

void assign_area( char_data * ch )
{
   char taf[256];
   area_data *tarea;

   if( ch->isnpc(  ) )
      return;

   if( ch->get_trust(  ) > LEVEL_IMMORTAL && ch->pcdata->low_vnum && ch->pcdata->hi_vnum )
   {
      tarea = ch->pcdata->area;
      snprintf( taf, 256, "%s.are", capitalize( ch->name ) );
      if( !tarea )
         tarea = find_area( taf );
      if( !tarea )
      {
         log_printf_plus( LOG_BUILD, ch->level, "Creating area entry for %s", ch->name );

         tarea = create_area(  );
         strdup_printf( &tarea->name, "[PROTO] %s's area in progress", ch->name );
         tarea->filename = str_dup( taf );
         stralloc_printf( &tarea->author, "%s", ch->name );
         tarea->sort_name(  );
         tarea->sort_vnums(  );
      }
      else
         log_printf_plus( LOG_BUILD, ch->level, "Updating area entry for %s", ch->name );

      tarea->low_vnum = ch->pcdata->low_vnum;
      tarea->hi_vnum = ch->pcdata->hi_vnum;
      ch->pcdata->area = tarea;
   }
}

/* Function mostly rewritten by Xorith */
CMDF( do_aassign )
{
   area_data *tarea;

   ch->set_color( AT_IMMORT );

   if( ch->isnpc(  ) )
      return;

   if( argument.empty(  ) )
   {
      ch->print( "Syntax: aassign <filename.are>  - Assigns you an area for building.\r\n" );
      ch->print( "        aassign none/null/clear - Clears your assigned area and restores your building area (if any).\r\n" );
      if( ch->get_trust(  ) < LEVEL_GOD )
         ch->print( "Note: You can only aassign areas bestowed upon you.\r\n" );
      if( ch->get_trust(  ) < sysdata->level_modify_proto )
         ch->print( "Note: You can only aassign areas that are marked as prototype.\r\n" );
      return;
   }

   if( !str_cmp( "none", argument ) || !str_cmp( "null", argument ) || !str_cmp( "clear", argument ) )
   {
      ch->pcdata->area = nullptr;
      assign_area( ch );
      if( !ch->pcdata->area )
         ch->print( "Area pointer cleared.\r\n" );
      else
         ch->print( "Originally assigned area restored.\r\n" );
      return;
   }

   if( ch->get_trust(  ) < LEVEL_GOD && !hasname( ch->pcdata->bestowments, argument ) )
   {
      ch->print( "You must have your desired area bestowed upon you by your superiors.\r\n" );
      return;
   }

   if( !( tarea = find_area( argument ) ) )
   {
      ch->printf( "The area '%s' does not exsist. Please use the 'zones' command for a list.\r\n", argument.c_str(  ) );
      return;
   }

   if( !tarea->flags.test( AFLAG_PROTOTYPE ) && ch->get_trust(  ) < sysdata->level_modify_proto )
   {
      ch->printf( "The area '%s' is not a proto area, and you're not authorized to work on non-proto areas.\r\n", tarea->name );
      return;
   }

   ch->pcdata->area = tarea;
   ch->printf( "Assigning you: %s\r\n", tarea->name );
   log_printf( "Assigning %s to %s.", tarea->name, ch->name );
}

/*
 * A complicated to use command as it currently exists.		-Thoric
 * Once area->author and area->name are cleaned up... it will be easier
 */
CMDF( do_installarea )
{
   area_data *tarea;
   string arg1, arg2;
   char oldfilename[256], buf[256];
   int num;

   ch->set_color( AT_IMMORT );

   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );

   if( arg1.empty(  ) || arg2.empty(  ) || argument.empty(  ) )
   {
      ch->print( "Syntax: installarea <current filename> <new filename> <Area name>\r\n" );
      return;
   }

   if( ( tarea = find_area( arg1 ) ) && tarea->flags.test( AFLAG_PROTOTYPE ) )
   {
      if( exists_file( arg2 ) )
      {
         ch->printf( "An area with filename %s already exists - choose another.\r\n", arg2.c_str(  ) );
         return;
      }

      DISPOSE( tarea->name );
      tarea->name = str_dup( argument.c_str(  ) );

      strlcpy( oldfilename, tarea->filename, 256 );
      DISPOSE( tarea->filename );
      tarea->filename = str_dup( arg2.c_str(  ) );

      /*
       * Fold area with install flag -- auto-removes prototype flags 
       */
      tarea->flags.reset( AFLAG_PROTOTYPE );
      ch->printf( "Saving and installing %s...\r\n", tarea->filename );
      tarea->install_date = current_time;
      tarea->fold( tarea->filename, true );

      /*
       * Fix up author if online 
       */
      list < descriptor_data * >::iterator ds;
      for( ds = dlist.begin(  ); ds != dlist.end(  ); ++ds )
      {
         descriptor_data *d = *ds;

         if( d->character && d->character->pcdata && d->character->pcdata->area == tarea )
         {
            /*
             * remove area from author 
             */
            d->character->pcdata->area = nullptr;
            /*
             * clear out author vnums  
             */
            d->character->pcdata->low_vnum = 0;
            d->character->pcdata->hi_vnum = 0;
            d->character->save(  );
         }
      }
      ++top_area;
      ch->print( "Writing area.lst...\r\n" );
      write_area_list(  );
      web_arealist(  );
      ch->print( "Resetting new area.\r\n" );
      num = tarea->nplayer;
      tarea->nplayer = 0;
      tarea->reset(  );
      tarea->nplayer = num;
      ch->print( "Removing author's building file.\r\n" );
      int bc = snprintf( buf, 256, "%s%s", BUILD_DIR, oldfilename );
      if( bc < 0 )
         bug( "%s: Output buffer error!", __func__ );
      unlink( buf );
      bc = snprintf( buf, 256, "%s%s.bak", BUILD_DIR, oldfilename );
      if( bc < 0 )
         bug( "%s: Output buffer error!", __func__ );
      unlink( buf );
      ch->print( "Done.\r\n" );
      return;
   }
   ch->printf( "No area with filename %s exists in the building directory.\r\n", arg1.c_str(  ) );
}

CMDF( do_astat )
{
   area_data *tarea;

   ch->set_color( AT_PLAIN );

   if( !( tarea = find_area( argument ) ) )
   {
      if( !argument.empty(  ) )
      {
         ch->print( "Area not found. Check 'zones' or 'vnums'.\r\n" );
         return;
      }
      else
         tarea = ch->in_room->area;
   }

   ch->printf( "\r\n&wName:     &W%s\r\n&wFilename: &W%-20s  &wPrototype: &W%s\r\n&wAuthor:   &W%s\r\n",
               tarea->name, tarea->filename, tarea->flags.test( AFLAG_PROTOTYPE ) ? "yes" : "no", tarea->author );
   ch->printf( "&wCreated on   : &W%s\r\n", c_time( tarea->creation_date, -1 ) );
   ch->printf( "&wInstalled on : &W%s\r\n", c_time( tarea->install_date, -1 ) );
   ch->printf( "&wLast reset on: &W%s\r\n", c_time( tarea->last_resettime, -1 ) );
   ch->printf( "&wVersion: &W%-3d &wAge: &W%-3d  &wCurrent number of players: &W%-3d\r\n", tarea->version, tarea->age, tarea->nplayer );
   ch->printf( "&wlow_vnum: &W%5d    &whi_vnum: &W%5d\r\n", tarea->low_vnum, tarea->hi_vnum );
   ch->printf( "&wSoft range: &W%d - %d    &wHard range: &W%d - %d\r\n", tarea->low_soft_range, tarea->hi_soft_range, tarea->low_hard_range, tarea->hi_hard_range );
   ch->printf( "&wArea flags: &W%s\r\n", bitset_string( tarea->flags, area_flags ) );

   ch->print( "&wTreasure Settings:\r\n" );
   ch->printf( "&wNothing: &W%-3hu &wGold:   &W%-3hu &wItem: &W%-3hu &wGem:   &W%-3hu\r\n", tarea->tg_nothing, tarea->tg_gold, tarea->tg_item, tarea->tg_gem );
   ch->printf( "&wScroll:  &W%-3hu &wPotion: &W%-3hu &wWand: &W%-3hu &wArmor: &W%-3hu\r\n", tarea->tg_scroll, tarea->tg_potion, tarea->tg_wand, tarea->tg_armor );

   if( tarea->continent )
      ch->printf( "&wContinent or Plane: &W%s\r\n", tarea->continent->name.c_str( ) );
   else
      ch->print( "&wContinent or Plane: &W<NOT SET>\r\n" );

   ch->printf( "&wCoordinates: &W%d %d\r\n", tarea->map_x, tarea->map_y );
   ch->printf( "&wWeather: X Coord: &W%-3d  &w Y Coord: &W%-3d\r\n", tarea->weatherx, tarea->weathery );

   ch->printf( "&wResetmsg: &W%s\r\n", tarea->resetmsg ? tarea->resetmsg : "(default)" ); /* Rennard */
   ch->printf( "&wReset frequency: &W%d &wminutes.\r\n", tarea->reset_frequency ? tarea->reset_frequency : 15 );
}

/* check other areas for a conflict while ignoring the current area */
bool check_for_area_conflicts( area_data * carea, int lo, int hi )
{
   list < area_data * >::iterator iarea;

   for( iarea = arealist.begin(  ); iarea != arealist.end(  ); ++iarea )
   {
      area_data *area = ( *iarea );

      if( area != carea && check_area_conflict( area, lo, hi ) )
         return true;
   }
   return false;
}

CMDF( do_aset )
{
   area_data *tarea;
   string arg1, arg2, arg3;
   int vnum, value;

   ch->set_color( AT_IMMORT );

   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );
   vnum = atoi( argument.c_str(  ) );

   if( arg1.empty(  ) || arg2.empty(  ) )
   {
      ch->print( "Usage: aset <area filename> <field> <value>\r\n" );
      ch->print( "\r\nField being one of:\r\n" );
      ch->print( "  low_vnum hi_vnum coords\r\n" );
      ch->print( "  name filename low_soft hi_soft low_hard hi_hard\r\n" );
      ch->print( "  author credits resetmsg resetfreq flags\r\n" );
      ch->print( "  weatherx weathery\r\n" );
      ch->print( "  nothing gold item gem\r\n" );
      ch->print( "  scroll potion wand armor\r\n" );
      return;
   }

   if( !( tarea = find_area( arg1 ) ) )
   {
      ch->print( "Area not found.\r\n" );
      return;
   }

   if( !str_cmp( arg2, "name" ) )
   {
      area_data *uarea;

      if( argument.empty(  ) )
      {
         ch->print( "You can't set an area's name to nothing.\r\n" );
         return;
      }

      if( ( uarea = find_area( argument ) ) != nullptr )
      {
         ch->print( "There is already an installed area with that name.\r\n" );
         return;
      }

      DISPOSE( tarea->name );
      tarea->name = str_dup( argument.c_str(  ) );
      ch->print( "Done.\r\n" );
      return;
   }

   if( !str_cmp( arg2, "filename" ) )
   {
      char filename[256];

      if( !is_valid_filename( ch, "", argument ) )
         return;

      strlcpy( filename, tarea->filename, 256 );
      DISPOSE( tarea->filename );
      tarea->filename = str_dup( argument.c_str(  ) );
      rename( filename, tarea->filename );
      write_area_list(  );
      ch->print( "Done.\r\n" );
      return;
   }

   if( !str_cmp( arg2, "continent" ) )
   {
      continent_data *continent;

      /*
       * Area continent editing - Samson 8-8-98 
       */
      if( argument.empty(  ) )
      {
         ch->print( "Set the area's continent.\r\n" );
         ch->print( "Usage: aset continent <name>\r\n" );
         return;
      }
      argument = one_argument( argument, arg2 );
      continent = find_continent_by_name( arg2 );
      if( !continent )
      {
         ch->printf( "Invalid area continent: %s does not exist.\r\n", arg2.c_str(  ) );
      }
      else
      {
         tarea->continent = continent;
         ch->printf( "Area continent set to %s.\r\n", arg2.c_str(  ) );
      }
      return;
   }

   if( !str_cmp( arg2, "low_vnum" ) )
   {
      if( check_for_area_conflicts( tarea, tarea->low_vnum, vnum ) )
      {
         ch->printf( "Setting %d for low_vnum would conflict with another area.\r\n", vnum );
         return;
      }
      if( tarea->hi_vnum < vnum )
      {
         ch->printf( "Vnum %d exceeds the hi_vnum of %d for this area.\r\n", vnum, tarea->hi_vnum );
         return;
      }
      tarea->low_vnum = vnum;
      ch->print( "Done.\r\n" );
      return;
   }

   if( !str_cmp( arg2, "hi_vnum" ) )
   {
      if( check_for_area_conflicts( tarea, tarea->hi_vnum, vnum ) )
      {
         ch->printf( "Setting %d for hi_vnum would conflict with another area.\r\n", vnum );
         return;
      }
      if( tarea->low_vnum > vnum )
      {
         ch->printf( "Cannot set %d for hi_vnum smaller than the low_vnum of %d.\r\n", vnum, tarea->low_vnum );
         return;
      }
      tarea->hi_vnum = vnum;
      ch->print( "Done.\r\n" );
      return;
   }

   if( !str_cmp( arg2, "coords" ) )
   {
      int x, y;

      argument = one_argument( argument, arg3 );

      if( arg3[0] == '\0' || argument[0] == '\0' )
      {
         ch->print( "You must specify X and Y coordinates for this.\r\n" );
         return;
      }

      x = atoi( arg3.c_str(  ) );
      y = atoi( argument.c_str(  ) );

      if( !is_valid_x( x ) )
      {
         ch->printf( "Valid X coordinates are from 0 to %d.\r\n", MAX_X - 1 );
         return;
      }

      if( !is_valid_y( y ) )
      {
         ch->printf( "Valid Y coordinates are from 0 to %d.\r\n", MAX_Y - 1 );
         return;
      }

      tarea->map_x = x;
      tarea->map_y = y;

      ch->print( "Area coordinates set.\r\n" );
      return;
   }

   if( !str_cmp( arg2, "weatherx" ) )
   {
      tarea->weatherx = vnum;
      ch->print( "Weather X Coordinate set.\r\n" );
      return;
   }

   if( !str_cmp( arg2, "weathery" ) )
   {
      tarea->weathery = vnum;
      ch->print( "Weather Y Coordinate set.\r\n" );
      return;
   }

   if( !str_cmp( arg2, "low_soft" ) )
   {
      if( vnum < 0 || vnum > MAX_LEVEL )
      {
         ch->print( "That is not an acceptable value.\r\n" );
         return;
      }

      tarea->low_soft_range = vnum;
      ch->print( "Done.\r\n" );
      web_arealist(  );
      return;
   }

   if( !str_cmp( arg2, "hi_soft" ) )
   {
      if( vnum < 0 || vnum > MAX_LEVEL )
      {
         ch->print( "That is not an acceptable value.\r\n" );
         return;
      }

      tarea->hi_soft_range = vnum;
      ch->print( "Done.\r\n" );
      web_arealist(  );
      return;
   }

   if( !str_cmp( arg2, "low_hard" ) )
   {
      if( vnum < 0 || vnum > MAX_LEVEL )
      {
         ch->print( "That is not an acceptable value.\r\n" );
         return;
      }

      tarea->low_hard_range = vnum;
      ch->print( "Done.\r\n" );
      web_arealist(  );
      return;
   }

   if( !str_cmp( arg2, "hi_hard" ) )
   {
      if( vnum < 0 || vnum > MAX_LEVEL )
      {
         ch->print( "That is not an acceptable value.\r\n" );
         return;
      }

      tarea->hi_hard_range = vnum;
      ch->print( "Done.\r\n" );
      web_arealist(  );
      return;
   }

   if( !str_cmp( arg2, "author" ) )
   {
      STRFREE( tarea->author );
      tarea->author = STRALLOC( argument.c_str(  ) );
      ch->print( "Done.\r\n" );
      web_arealist(  );
      return;
   }

   if( !str_cmp( arg2, "credits" ) )
   {
      STRFREE( tarea->credits );
      tarea->credits = STRALLOC( argument.c_str(  ) );
      ch->print( "Done.\r\n" );
      web_arealist(  );
      return;
   }

   if( !str_cmp( arg2, "resetmsg" ) )
   {
      DISPOSE( tarea->resetmsg );
      if( str_cmp( argument, "clear" ) )
         tarea->resetmsg = str_dup( argument.c_str(  ) );
      ch->print( "Done.\r\n" );
      return;
   }  /* Rennard */

   if( !str_cmp( arg2, "resetfreq" ) )
   {
      tarea->reset_frequency = vnum;
      ch->print( "Done.\r\n" );
      return;
   }

   if( !str_cmp( arg2, "flags" ) )
   {
      if( argument.empty(  ) )
      {
         ch->print( "Usage: aset <filename> flags <flag> [flag]...\r\n" );
         return;
      }
      while( !argument.empty(  ) )
      {
         argument = one_argument( argument, arg3 );
         value = get_areaflag( arg3 );
         if( value < 0 || value >= AFLAG_MAX )
            ch->printf( "Unknown flag: %s\r\n", arg3.c_str(  ) );
         else
            tarea->flags.flip( value );
      }
      return;
   }

   if( !str_cmp( arg2, "nothing" ) )
   {
      if( argument.empty(  ) || !is_number( argument ) )
      {
         ch->print( "Usage: aset <filename> nothing <percentage>\r\n" );
         return;
      }
      tarea->tg_nothing = stoi( argument );
      ch->printf( "Area chance to generate nothing set to %hu%%\r\n", tarea->tg_nothing );

      return;
   }

   if( !str_cmp( arg2, "gold" ) )
   {
      if( argument.empty(  ) || !is_number( argument ) )
      {
         ch->print( "Usage: aset <filename> gold <percentage>\r\n" );
         return;
      }
      tarea->tg_gold = stoi( argument );
      ch->printf( "Area chance to generate gold set to %hu%%\r\n", tarea->tg_gold );

      return;
   }

   if( !str_cmp( arg2, "item" ) )
   {
      if( argument.empty(  ) || !is_number( argument ) )
      {
         ch->print( "Usage: aset <filename> item <percentage>\r\n" );
         return;
      }
      tarea->tg_item = stoi( argument );
      ch->printf( "Area chance to generate item set to %hu%%\r\n", tarea->tg_item );

      return;
   }

   if( !str_cmp( arg2, "gem" ) )
   {
      if( argument.empty(  ) || !is_number( argument ) )
      {
         ch->print( "Usage: aset <filename> gem <percentage>\r\n" );
         return;
      }
      tarea->tg_gem = stoi( argument );
      ch->printf( "Area chance to generate gem set to %hu%%\r\n", tarea->tg_gem );

      return;
   }

   if( !str_cmp( arg2, "scroll" ) )
   {
      if( argument.empty(  ) || !is_number( argument ) )
      {
         ch->print( "Usage: aset <filename> scroll <percentage>\r\n" );
         return;
      }
      tarea->tg_scroll = stoi( argument );
      ch->printf( "Area chance to generate scroll set to %hu%%\r\n", tarea->tg_scroll );

      return;
   }

   if( !str_cmp( arg2, "potion" ) )
   {
      if( argument.empty(  ) || !is_number( argument ) )
      {
         ch->print( "Usage: aset <filename> potion <percentage>\r\n" );
         return;
      }
      tarea->tg_potion = stoi( argument );
      ch->printf( "Area chance to generate potion set to %hu%%\r\n", tarea->tg_potion );

      return;
   }

   if( !str_cmp( arg2, "wand" ) )
   {
      if( argument.empty(  ) || !is_number( argument ) )
      {
         ch->print( "Usage: aset <filename> wand <percentage>\r\n" );
         return;
      }
      tarea->tg_wand = stoi( argument );
      ch->printf( "Area chance to generate wand set to %hu%%\r\n", tarea->tg_wand );

      return;
   }

   if( !str_cmp( arg2, "armor" ) )
   {
      if( argument.empty(  ) || !is_number( argument ) )
      {
         ch->print( "Usage: aset <filename> armor <percentage>\r\n" );
         return;
      }
      tarea->tg_armor = stoi( argument );
      ch->printf( "Area chance to generate armor set to %hu%%\r\n", tarea->tg_armor );

      return;
   }

   do_aset( ch, "" );
}

/* Displays zone list. Will show proto, non-proto, or all. */
void show_vnums( char_data * ch, short proto )
{
   list < area_data * >::iterator iarea;
   int count = 0;

   ch->pagerf( "&W%-15.15s %-40.40s %5.5s\r\n\r\n", "Filename", "Area Name", "Vnums" );
   for( iarea = area_vsort.begin(  ); iarea != area_vsort.end(  ); ++iarea )
   {
      area_data *area = *iarea;

      if( proto == 0 && !area->flags.test( AFLAG_PROTOTYPE ) )
      {
         ch->pagerf( "&c%-15.15s %-40.40s V: %5d - %-5d\r\n", area->filename, area->name, area->low_vnum, area->hi_vnum );
         ++count;
      }
      else if( proto == 1 && area->flags.test( AFLAG_PROTOTYPE ) )
      {
         ch->pagerf( "&c%-15.15s %-40.40s V: %5d - %-5d &W[Proto]\r\n", area->filename, area->name, area->low_vnum, area->hi_vnum );
         ++count;
      }
      else if( proto == 2 )
      {
         ch->pagerf( "&c%-15.15s %-40.40s V: %5d - %-5d %s\r\n",
                     area->filename, area->name, area->low_vnum, area->hi_vnum, area->flags.test( AFLAG_PROTOTYPE ) ? "&W[Proto]" : "" );
         ++count;
      }
   }
   ch->pagerf( "&CAreas listed: %d\r\n", count );
   ch->pagerf( "Maximum allowed vnum is currently %d.&D\r\n", sysdata->maxvnum );
}

CMDF( do_zones )
{
   if( argument.empty(  ) )
   {
      show_vnums( ch, 0 );
      return;
   }
   if( !str_cmp( argument, "proto" ) )
   {
      show_vnums( ch, 1 );
      return;
   }
   if( !str_cmp( argument, "all" ) )
   {
      show_vnums( ch, 2 );
      return;
   }
   ch->print( "Usage: zones [proto/all]\r\n" );
}

/* Similar to checkvnum, but will list the freevnums -- Xerves 3-11-01 */
CMDF( do_freevnums )
{
   string arg1;
   int lohi[600]; /* Up to 300 areas, increase if you have more -- Xerves */

   argument = one_argument( argument, arg1 );

   if( arg1.empty(  ) )
   {
      ch->print( "Please specify the low end of the range to be searched.\r\n" );
      return;
   }

   if( argument.empty(  ) )
   {
      ch->print( "Please specify the high end of the range to be searched.\r\n" );
      return;
   }

   int low_range = atoi( arg1.c_str(  ) ), high_range = atoi( argument.c_str(  ) );

   if( low_range < 1 || low_range > sysdata->maxvnum )
   {
      ch->printf( "Invalid low range. Valid vnums are from 1 to %d.\r\n", sysdata->maxvnum );
      return;
   }

   if( high_range < 1 || high_range > sysdata->maxvnum )
   {
      ch->printf( "Invalid high range. Valid vnums are from 1 to %d.\r\n", sysdata->maxvnum );
      return;
   }

   if( high_range < low_range )
   {
      ch->print( "Low range must be below high range.\r\n" );
      return;
   }

   ch->set_color( AT_PLAIN );

   int x = 0;
   bool area_conflict = false;
   list < area_data * >::iterator iarea;
   for( iarea = arealist.begin(  ); iarea != arealist.end(  ); ++iarea )
   {
      area_data *area = *iarea;

      area_conflict = check_area_conflict( area, low_range, high_range );

      if( area_conflict )
      {
         lohi[x++] = area->low_vnum;
         lohi[x++] = area->hi_vnum;
         ch->printf( "&RArea Conflict: &g%-20.20s &wRange: &g%d - %d\r\n", area->filename, area->low_vnum, area->hi_vnum );
      }
   }
   int xfin = x;
   for( int y = low_range; y < high_range; y += 50 )
   {
      area_conflict = false;
      int z = y + 49;   /* y is min, z is max */
      for( x = 0; x < xfin; x += 2 )
      {
         int w = x + 1;

         if( y < lohi[x] && lohi[x] < z )
         {
            area_conflict = true;
            break;
         }
         if( y < lohi[w] && lohi[w] < z )
         {
            area_conflict = true;
            break;
         }
         if( y >= lohi[x] && y <= lohi[w] )
         {
            area_conflict = true;
            break;
         }
         if( z <= lohi[w] && z >= lohi[x] )
         {
            area_conflict = true;
            break;
         }
      }
      if( area_conflict == false )
         ch->printf( "&wOpen Range: &g%d - %d\r\n", y, z );
   }
}

/* Check to make sure range of vnums is free - Scryn 2/27/96 */
CMDF( do_check_vnums )
{
   string arg;

   argument = one_argument( argument, arg );

   if( arg.empty(  ) )
   {
      ch->print( "Please specify the low end of the range to be searched.\r\n" );
      return;
   }

   if( argument.empty(  ) )
   {
      ch->print( "Please specify the high end of the range to be searched.\r\n" );
      return;
   }

   int low_range = atoi( arg.c_str(  ) ), high_range = atoi( argument.c_str(  ) );

   if( low_range < 1 || low_range > sysdata->maxvnum )
   {
      ch->print( "Invalid argument for bottom of range.\r\n" );
      return;
   }

   if( high_range < 1 || high_range > sysdata->maxvnum )
   {
      ch->print( "Invalid argument for top of range.\r\n" );
      return;
   }

   if( high_range < low_range )
   {
      ch->print( "Bottom of range must be below top of range.\r\n" );
      return;
   }

   ch->set_color( AT_PLAIN );

   // FIXME: Can this use the area_conflict function?
   list < area_data * >::iterator iarea;
   for( iarea = arealist.begin(  ); iarea != arealist.end(  ); ++iarea )
   {
      area_data *area = *iarea;
      bool area_conflict = false;

      if( low_range < area->low_vnum && area->low_vnum < high_range )
         area_conflict = true;

      if( low_range < area->hi_vnum && area->hi_vnum < high_range )
         area_conflict = true;

      if( ( low_range >= area->low_vnum ) && ( low_range <= area->hi_vnum ) )
         area_conflict = true;

      if( ( high_range <= area->hi_vnum ) && ( high_range >= area->low_vnum ) )
         area_conflict = true;

      if( area_conflict )
      {
         ch->printf( "Conflict:%-15s| ", ( area->filename ? area->filename : "(invalid)" ) );
         ch->printf( "Vnums: %5d - %-5d\r\n", area->low_vnum, area->hi_vnum );
      }
   }
}

/* Shogar's code to hunt for exits/entrances to/from a zone, very nice - Samson 12-30-00 */
CMDF( do_aexit )
{
   area_data *tarea;
   list < mapexit_data * >::iterator imexit;
   list < continent_data * >::iterator c;

   if( argument.empty(  ) )
      tarea = ch->in_room->area;
   else
   {
      if( !( tarea = find_area( argument ) ) )
      {
         ch->print( "Area not found.\r\n" );
         return;
      }
   }

   list < room_index * >::iterator rindex;
   int trange = tarea->hi_vnum, lrange = tarea->low_vnum;
   for( rindex = tarea->rooms.begin(  ); rindex != tarea->rooms.end(  ); ++rindex )
   {
      room_index *room = *rindex;

      if( room->flags.test( ROOM_TELEPORT ) && ( room->tele_vnum < lrange || room->tele_vnum > trange ) )
      {
         ch->pagerf( "From: %-20.20s Room: %5d To: Room: %5d (Teleport)\r\n", tarea->filename, room->vnum, room->tele_vnum );
      }

      for( int i = 0; i < MAX_DIR + 2; ++i ) /* MAX_DIR+2 added to include ? exits.  Dwip 5/7/02 */
      {
         exit_data *pexit;
         if( !( pexit = room->get_exit( i ) ) )
            continue;

         if( IS_EXIT_FLAG( pexit, EX_OVERLAND ) )
         {
            ch->pagerf( "To: Overland %4dX %4dY From: %20.20s Room: %5d (%s)\r\n", pexit->map_x, pexit->map_y, tarea->filename, room->vnum, dir_name[i] );
            continue;
         }
         if( pexit->to_room->area != tarea )
         {
            ch->pagerf( "To: %-20.20s Room: %5d From: %-20.20s Room: %5d (%s)\r\n", pexit->to_room->area->filename, pexit->vnum, tarea->filename, room->vnum, dir_name[i] );
         }
      }
   }

   list < area_data * >::iterator iarea;
   for( iarea = arealist.begin(  ); iarea != arealist.end(  ); ++iarea )
   {
      area_data *area = *iarea;

      if( tarea == area )
         continue;

      trange = area->hi_vnum;
      lrange = area->low_vnum;
      for( rindex = area->rooms.begin(  ); rindex != area->rooms.end(  ); ++rindex )
      {
         room_index *room = *rindex;

         if( room->flags.test( ROOM_TELEPORT ) )
         {
            if( room->tele_vnum >= tarea->low_vnum && room->tele_vnum <= tarea->hi_vnum )
               ch->pagerf( "From: %-20.20s Room: %5d To: %-20.20s Room: %5d (Teleport)\r\n", area->filename, room->vnum, tarea->filename, room->tele_vnum );
         }

         for( int i = 0; i < MAX_DIR + 2; ++i )
         {
            exit_data *pexit;
            if( !( pexit = room->get_exit( i ) ) )
               continue;

            if( IS_EXIT_FLAG( pexit, EX_OVERLAND ) )
               continue;

            if( pexit->to_room->area == tarea )
            {
               ch->pagerf( "From: %-20.20s Room: %5d To: %-20.20s Room: %5d (%s)\r\n", area->filename, room->vnum, pexit->to_room->area->filename, pexit->vnum, dir_name[i] );
            }
         }
      }
   }

   for( c = continent_list.begin(  ); c != continent_list.end(  ); ++c )    
   {
      continent_data *continent = *c;

      for( imexit = continent->exits.begin(  ); imexit != continent->exits.end(  ); ++imexit )
      {
         mapexit_data *mexit = *imexit;

         if( mexit->vnum >= tarea->low_vnum && mexit->vnum <= tarea->hi_vnum )
            ch->pagerf( "From Continent %s: %4dX %4dY To: Room: %5d\r\n", continent->name.c_str( ), mexit->herex, mexit->herey, mexit->vnum );
      }
   }
}

/* Revised version of do_areas, orgininally written by Fireblade 4/27/97,
 * rewritten by Dwip to fix horrid argument bugs 4/14/02.  Happy 5 year
 * anniversary, do_areas! :)
 */
CMDF( do_areas )
{
   const char *print_string = "%-12s | %-36s | %4d - %-4d | %3d - %-3d \r\n";
   int lower_bound = 0, upper_bound = MAX_LEVEL + 1, num_args = 0, swap;
   /*
    * 0-2 = x arguments, 3 = old style 
    */
   string arg;

   argument = one_argument( argument, arg );

   if( arg.empty(  ) )
      num_args = 0;
   else
   {
      if( !is_number( arg ) )
      {
         if( !str_cmp( arg, "old" ) )
            num_args = 3;
         else
         {
            ch->print( "Area may only be followed by numbers or old.\r\n" );
            return;
         }
      }
      else
      {
         num_args = 1;

         upper_bound = atoi( arg.c_str(  ) );
         /*
          * Will need to swap this with ubound later 
          */

         argument = one_argument( argument, arg );

         if( !arg.empty(  ) )
         {
            if( !is_number( arg ) )
            {
               ch->print( "Area may only be followed by numbers or old.\r\n" );
               return;
            }
            num_args = 2;

            lower_bound = upper_bound;
            upper_bound = atoi( arg.c_str(  ) );
         }

         argument = one_argument( argument, arg );

         if( !arg.empty(  ) )
         {
            ch->print( "Only two level numbers allowed.\r\n" );
            return;
         }
      }

      if( lower_bound > upper_bound )
      {
         swap = lower_bound;
         lower_bound = upper_bound;
         upper_bound = swap;
      }
   }
   ch->set_pager_color( AT_PLAIN );
   ch->pager( "\r\n   Author    |             Area                     | Recommended |  Enforced\r\n" );
   ch->pager( "-------------+--------------------------------------+-------------+-----------\r\n" );

   list < area_data * >::iterator iarea;
   for( iarea = area_nsort.begin(  ); iarea != area_nsort.end(  ); ++iarea )
   {
      area_data *area = *iarea;

      // Hidden areas should not display on the list.
      if( area->flags.test( AFLAG_HIDDEN ) && !ch->is_immortal() )
         continue;

      switch ( num_args )
      {
         case 0:
            ch->pagerf( print_string, area->author, area->name, area->low_soft_range, area->hi_soft_range, area->low_hard_range, area->hi_hard_range );
            break;

         case 1:
            if( area->hi_soft_range >= upper_bound && area->low_soft_range <= upper_bound )
            {
               ch->pagerf( print_string, area->author, area->name, area->low_soft_range, area->hi_soft_range, area->low_hard_range, area->hi_hard_range );
            }
            break;

         case 2:
            if( area->hi_soft_range >= upper_bound && area->low_soft_range <= lower_bound )
            {
               ch->pagerf( print_string, area->author, area->name, area->low_soft_range, area->hi_soft_range, area->low_hard_range, area->hi_hard_range );
            }
            break;

         case 3:
            ch->pagerf( print_string, area->author, area->name, area->low_soft_range, area->hi_soft_range, area->low_hard_range, area->hi_hard_range );
            break;

         default:
            ch->pagerf( print_string, area->author, area->name, area->low_soft_range, area->hi_soft_range, area->low_hard_range, area->hi_hard_range );
            break;
      }
   }
}
