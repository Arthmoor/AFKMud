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
 *                           Area Support Functions                         *
 ****************************************************************************/

#include <filesystem>
#include <fstream>
#include "mud.h"
#include "area.h"
#include "calendar.h"
#include "mobindex.h"
#include "mud_prog.h"
#include "mudprog_loader.h"
#include "objindex.h"
#include "overland.h"
#include "roomindex.h"
#include "shops.h"
#include "smaugaffect.h"

int top_area;

extern int top_prog;
extern int top_affect;
extern int top_shop;
extern int top_repair;
extern std::ifstream fpArea;

std::list < area_data * >arealist;
std::list < area_data * >area_nsort;
std::list < area_data * >area_vsort;

int recall( char_data *, int );
void save_sysdata(  );
bool check_area_conflict( const area_data *, int, int );
void web_arealist(  );
area_data *fread_smaugfuss_area( std::ifstream & );
bool load_oldafk_area( std::ifstream &, area_data *, int );
CMDF( do_areaconvert );

area_data::area_data(  )
{
}

area_data::~area_data(  )
{
   for( auto it = charlist.begin(); it != charlist.end(); )
   {
      char_data *ech = *it;
      ++it;

      if( ech->fighting )
         ech->stop_fighting( true );

      if( ech->isnpc(  ) )
      {
         /*
          * if mob is in area, or part of area. 
          */
         if( urange( low_vnum, ech->pIndexData->vnum, hi_vnum ) == ech->pIndexData->vnum || ( ech->in_room && ech->in_room->area == this ) )
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

   for( auto it = objlist.begin(); it != objlist.end(); )
   {
      obj_data *eobj = *it;
      ++it;

      /*
       * if obj is in area, or part of area. 
       */
      if( urange( low_vnum, eobj->pIndexData->vnum, hi_vnum ) == eobj->pIndexData->vnum || ( eobj->in_room && eobj->in_room->area == this ) )
         eobj->extract(  );
   }

   wipe_resets(  );

   for( auto it = rooms.begin(); it != rooms.end(); )
   {
      room_index *room = *it;
      ++it;

      deleteptr( room );
   }
   rooms.clear(  );

   for( auto it = mobs.begin(); it != mobs.end(); )
   {
      mob_index *mob = *it;
      ++it;

      deleteptr( mob );
   }
   mobs.clear(  );

   for( auto it = objects.begin(); it != objects.end(); )
   {
      obj_index *obj = *it;
      ++it;

      deleteptr( obj );
   }
   objects.clear(  );

   arealist.remove( this );
   area_nsort.remove( this );
   area_vsort.remove( this );
}

void area_data::fix_exits(  )
{
   room_index *pRoomIndex;
   std::list < room_index * >::iterator rindex;
   std::list < exit_data * >::iterator iexit;

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
   for( auto iarea = area_nsort.begin(); iarea != area_nsort.end(); ++iarea )
   {
      const area_data* area = *iarea;

      if( this->name < area->name )
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
   for( auto iarea = area_vsort.begin(); iarea != area_vsort.end(); ++iarea )
   {
      const area_data *area = *iarea;

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
   if( rooms.empty(  ) )
      return;

   for( auto* room : rooms )
      room->reset(  );
}

void area_data::wipe_resets(  )
{
   if( !mud_down )
   {
      for( auto* room : rooms )
         room->wipe_resets(  );
   }
}

area_data *create_area( void )
{
   area_data *pArea;

   pArea = new area_data;

   arealist.push_back( pArea );
   ++top_area;

   return pArea;
}

void validate_treasure_settings( area_data * area )
{
   if( area->tg_nothing > area->tg_gold && area->tg_gold != 0 )
   {
      log_printf( "{}: Nothing setting is larger than gold setting.", area->filename );
      if( area->tg_nothing > 100 )
      {
         log_printf( "{}: Nothing setting exceeds 100%, correcting.", area->filename );
         area->tg_nothing = 100;
      }
   }

   if( area->tg_gold > area->tg_item && area->tg_item != 0 )
   {
      log_printf( "{}: Gold setting is larger than item setting.", area->filename );
      if( area->tg_gold > 100 )
      {
         log_printf( "{}: Gold setting exceeds 100%, correcting.", area->filename );
         area->tg_gold = 100;
      }
   }

   if( area->tg_item > area->tg_gem && area->tg_gem != 0 )
   {
      log_printf( "{}: Item setting is larger than gem setting.", area->filename );
      if( area->tg_item > 100 )
      {
         log_printf( "{}: Item setting exceeds 100%, correcting.", area->filename );
         area->tg_item = 100;
      }
   }

   if( area->tg_gem > 100 )
   {
      log_printf( "{}: Gem setting exceeds 100%, correcting.", area->filename );
      area->tg_gem = 100;
   }

   if( area->tg_scroll > area->tg_potion && area->tg_potion != 0 )
   {
      log_printf( "{}: Scroll setting is larger than potion setting.", area->filename );
      if( area->tg_scroll > 100 )
      {
         log_printf( "{}: Scroll setting exceeds 100%, correcting.", area->filename );
         area->tg_scroll = 100;
      }
   }

   if( area->tg_potion > area->tg_wand && area->tg_wand != 0 )
   {
      log_printf( "{}: Potion setting is larger than wand setting.", area->filename );
      if( area->tg_wand > 100 )
      {
         log_printf( "{}: Wand setting exceeds 100%, correcting.", area->filename );
         area->tg_wand = 100;
      }
   }

   if( area->tg_wand > area->tg_armor && area->tg_armor != 0 )
   {
      log_printf( "{}: Wand setting is larger than armor setting.", area->filename );
      if( area->tg_wand > 100 )
      {
         log_printf( "{}: Wand setting exceeds 100%, correcting.", area->filename );
         area->tg_wand = 100;
      }
   }

   if( area->tg_armor > 100 )
   {
      log_printf( "{}: Armor setting exceeds 100%, correcting.", area->filename );
      area->tg_armor = 100;
   }
}

void fread_afk_exit( std::ifstream & stream, room_index * pRoomIndex )
{
   exit_data *pexit = nullptr;

   std::string key;
   while( stream >> key )
   {
      if( key == "Direction" )
      {
         int door = get_dir( fread_line( stream ) );

         if( door < 0 || door > DIR_SOMEWHERE )
         {
            bug( "{}: vnum {} has bad door number {}.", __func__, pRoomIndex->vnum, door );
            if( fBootDb )
               return;
         }
         pexit = pRoomIndex->make_exit( nullptr, door );
      }
      else if( key == "ToRoom" )
         stream >> pexit->vnum;
      else if( key == "Key" )
         stream >> pexit->key;
      else if( key == "ToCoords" )
         stream >> pexit->map_x >> pexit->map_y;
      else if( key == "Pull" )
         stream >> pexit->pulltype >> pexit->pull;
      else if( key == "Desc" )
         pexit->exitdesc = fread_line( stream ) ;
      else if( key == "Keywords" )
         pexit->keyword = fread_line( stream );
      else if( key == "Flags" )
         flag_set( stream, pexit->flags, ex_flags );
      else if( key == "#ENDEXIT" )
         return;
      else
      {
         bug( "{}: Bad section '{}' in exits - skipping.", __func__, key );
         fread_to_eol( stream );
      }
   }
   // Reach this point, you fell through somehow. The data is no longer valid.
   bug( "{}: Reached fallout point! Exit data invalid.", __func__ );
   if( pexit )
      pRoomIndex->extract_exit( pexit );
}

affect_data *fread_afk_affect( std::ifstream & stream )
{
   bool setaff = true;

   affect_data *paf = new affect_data;
   paf->location = APPLY_NONE;
   paf->type = -1;
   paf->duration = -1;
   paf->bit = 0;
   paf->modifier = 0;
   paf->rismod.reset(  );

   std::string loc = fread_word( stream );
   int value = get_atype( loc );
   if( value < 0 || value >= MAX_APPLY_TYPE )
   {
      bug( "{}: Invalid apply type: {}", __func__, loc );
      setaff = false;
   }
   paf->location = value;

   if( paf->location == APPLY_WEAPONSPELL
      || paf->location == APPLY_WEARSPELL
      || paf->location == APPLY_REMOVESPELL || paf->location == APPLY_STRIPSN || paf->location == APPLY_RECURRINGSPELL || paf->location == APPLY_EAT_SPELL )
      paf->modifier = skill_lookup( fread_word( stream ) );
   else if( paf->location == APPLY_AFFECT )
   {
      std::string aff = fread_word( stream );
      value = get_aflag( aff );
      if( value < 0 || value >= MAX_AFFECTED_BY )
      {
         bug( "{}: Unsupportable value for affect flag: {}", __func__, aff );
         setaff = false;
      }
      else
         paf->modifier = value;
   }
   else if( paf->location == APPLY_RESISTANT || paf->location == APPLY_IMMUNE || paf->location == APPLY_SUSCEPTIBLE || paf->location == APPLY_ABSORB )
      flag_set( stream, paf->rismod, ris_flags );
   else
      stream >> paf->modifier;

   stream >> paf->type;
   stream >> paf->duration;
   stream >> paf->bit;

   if( !setaff )
      deleteptr( paf );
   else
      ++top_affect;
   return paf;
}

extra_descr_data *fread_afk_exdesc( std::ifstream & stream )
{
   extra_descr_data *ed = new extra_descr_data;

   std::string key;
   while( stream >> key )
   {
      if( key == "ExDescKey" )
         ed->keyword = fread_line( stream );
      else if( key == "ExDesc" )
         ed->desc = fread_line( stream );
      else if( key == "#ENDEXDESC" )
      {
         if( ed->keyword.empty(  ) )
         {
            bug( "{}: Missing ExDesc keyword. Returning nullptr.", __func__ );
            deleteptr( ed );
            return nullptr;
         }
         return ed;
      }
      else
      {
         bug( "{}: Bad section '{}' in exdesc - skipping.", __func__, key );
         fread_to_eol( stream );
      }
   }
   // Reach this point, you fell through somehow. The data is no longer valid.
   bug( "{}: Reached fallout point! ExtraDesc data invalid.", __func__ );
   deleteptr( ed );
   return nullptr;
}

void fread_afk_areadata( std::ifstream & stream, area_data * tarea )
{
   std::string key;
   while( stream >> key )
   {
      if( key == "Version" )
         stream >> tarea->version;
      else if( key == "Name" )
         tarea->name = fread_line( stream );
      else if( key == "Author" )
         tarea->author = fread_line( stream );
      else if( key == "Credits" )
         tarea->credits = fread_line( stream );
      else if( key == "Climate" || key == "Neighbor" ) // These values are no longer used with the new weather code.
         fread_to_eol( stream );
      else if( key == "Vnums" )
      {
         stream >> tarea->low_vnum >> tarea->hi_vnum;

         /*
          * Protection against forgetting to raise the MaxVnum value before adding a new zone that would exceed it.
          * Potentially dangerous if some blockhead makes insanely high vnums and then installs the area.
          */
         if( tarea->hi_vnum >= sysdata->maxvnum )
         {
            sysdata->maxvnum = tarea->hi_vnum + 1;
            log_printf( "MaxVnum value raised to {} to accomadate new zone.", sysdata->maxvnum );
            save_sysdata(  );
         }
      }
      else if( key == "Continent" )
      {
         continent_data *continent = nullptr;
         std::string value = fread_line( stream );

         if( !( continent = find_continent_by_name( value ) ) )
         {
            bug( "{}: Invalid area continent '{}' - Needs to be manually corrected.", __func__, value );
            tarea->continent = nullptr;
         }
         else
         {
            tarea->continent = continent;
         }
      }
      else if( key == "Coordinates" )
      {
         short x, y;

         stream >> x >> y;

         if( !is_valid_x( x ) )
         {
            bug( "{}: Area has bad x coord - setting X to 0", __func__ );
            x = 0;
         }

         if( !is_valid_y( y ) )
         {
            bug( "{}: Area has bad y coord - setting Y to 0", __func__ );
            y = 0;
         }

         tarea->map_x = x;
         tarea->map_y = y;
      }
      else if( key == "Dates" )
      {
         time_t cdate;
         stream >> cdate;
         tarea->creation_date = std::chrono::system_clock::from_time_t( cdate );

         time_t idate;
         stream >> idate;
         tarea->install_date = std::chrono::system_clock::from_time_t( idate );
      }
      else if( key == "Ranges" )
         stream >> tarea->low_soft_range >> tarea->hi_soft_range >> tarea->low_hard_range >> tarea->hi_hard_range;
      else if( key == "ResetMsg" )
         tarea->resetmsg = fread_line( stream );
      else if( key == "ResetFreq" )
         stream >> tarea->reset_frequency;
      else if( key == "Flags" )
         flag_set( stream, tarea->flags, area_flags );
      else if( key == "Treasure" )
      {
         stream >> tarea->tg_nothing >> tarea->tg_gold >> tarea->tg_item >> tarea->tg_gem >> tarea->tg_scroll >> tarea->tg_potion >> tarea->tg_wand >> tarea->tg_armor;

         validate_treasure_settings( tarea );
      }
      else if( key == "WeatherCoords" )
         stream >> tarea->weatherx >> tarea->weathery;
      else if( key == "#ENDAREADATA" )
      {
         tarea->age = tarea->reset_frequency;
         return;
      }
      else
      {
         bug( "{}: Bad section '{}' in header data - skipping.", __func__, key );
         fread_to_eol( stream );
      }
   }
}

void fread_afk_mobile( std::ifstream & stream, area_data * tarea )
{
   mob_index *pMobIndex = nullptr;
   bool oldmob = false;

   std::string key;
   while( stream >> key )
   {
      if( key == "Vnum" )
      {
         bool tmpBootDb = fBootDb;
         fBootDb = false;

         int vnum;
         stream >> vnum;

         if( get_mob_index( vnum ) )
         {
            if( tmpBootDb )
            {
               fBootDb = tmpBootDb;
               bug( "{}: vnum {} duplicated.", __func__, vnum );

               // Try to recover, read to end of duplicated mobile and then bail out
               for( ;; )
               {
                  stream >> key;
                  if( key == "#ENDMOBILE" || stream.eof() )
                     return;
               }
            }
            else
            {
               pMobIndex = get_mob_index( vnum );
               log_printf_plus( LOG_BUILD, sysdata->build_level, "Cleaning mobile: {}", vnum );
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
      }
      else if( key == "Keywords" )
         pMobIndex->player_name = fread_line( stream );
      else if( key == "Race" )
      {
         std::string race_text = fread_line( stream );
         short race = get_npc_race( race_text );

         if( race < 0 || race >= MAX_NPC_RACE )
         {
            bug( "{}: vnum {}: Mob has invalid race: {}. Defaulting to monster.", __func__, pMobIndex->vnum, race_text );
            race = get_npc_race( "monster" );
         }

         pMobIndex->race = race;
      }
      else if( key == "Class" )
      {
         std::string class_text = fread_line( stream );
         short Class = get_npc_class( class_text );

         if( Class < 0 || Class >= MAX_NPC_CLASS )
         {
            bug( "{}: vnum {}: Mob has invalid class: {}. Defaulting to warrior.", __func__, pMobIndex->vnum, class_text );
            Class = get_npc_class( "warrior" );
         }

         pMobIndex->Class = Class;
      }
      else if( key == "Gender" )
      {
         std::string gender = fread_line( stream );
         short sex = get_npc_sex( gender );

         if( sex < 0 || sex >= SEX_MAX )
         {
            bug( "{}: vnum {}: Mobile has invalid gender: {}. Defaulting to neuter.", __func__, pMobIndex->vnum, gender );
            sex = SEX_NEUTRAL;
         }
         pMobIndex->sex = sex;
      }
      else if( key == "Position" )
      {
         std::string pos_text = fread_line( stream );
         short position = get_npc_position( pos_text );

         if( position < 0 || position >= POS_MAX )
         {
            bug( "{}: vnum {}: Mobile in invalid position: {}. Defaulting to standing.", __func__, pMobIndex->vnum, pos_text );
            position = POS_STANDING;
         }
         pMobIndex->position = position;
      }
      else if( key == "DefPos" )
      {
         std::string npc_pos = fread_line( stream );
         short position = get_npc_position( npc_pos );

         if( position < 0 || position >= POS_MAX )
         {
            bug( "{}: vnum {}: Mobile in invalid default position: {}. Defaulting to standing.", __func__, pMobIndex->vnum, npc_pos );
            position = POS_STANDING;
         }
         pMobIndex->defposition = position;
      }
      else if( key == "Specfun" )
      {
         std::string temp = fread_line( stream );

         if( !pMobIndex )
            bug( "{}: Specfun: Invalid mob vnum!", __func__ );
         else if( !( pMobIndex->spec_fun = m_spec_lookup( temp ) ) )
         {
            bug( "{}: Specfun: vnum {}, no spec_fun called {}.", __func__, pMobIndex->vnum, temp );
            pMobIndex->spec_funname.clear(  );
         }
         else
            pMobIndex->spec_funname = temp;
      }
      else if( key == "Short" )
         pMobIndex->short_descr = fread_line( stream );
      else if( key == "Long" )
         pMobIndex->long_descr = fread_line( stream );
      else if( key == "Desc" )
      {
         pMobIndex->chardesc = fread_line( stream );

         if( !pMobIndex->chardesc.empty() )
         {
            if( str_prefix( "namegen", pMobIndex->chardesc ) )
               pMobIndex->chardesc[0] = to_upper( pMobIndex->chardesc[0] );
         }
      }
      else if( key == "Nattacks" )
         stream >> pMobIndex->numattacks;
      else if( key == "Stats1" )
      {
         stream >> pMobIndex->alignment >> pMobIndex->gold >> pMobIndex->height >> pMobIndex->weight >> pMobIndex->max_move >> pMobIndex->max_mana;

         if( pMobIndex->max_move < 1 )
            pMobIndex->max_move = 150;

         if( pMobIndex->max_mana < 1 )
            pMobIndex->max_mana = 100;
      }
      else if( key == "Stats2" )
      {
         stream >> pMobIndex->level >> pMobIndex->mobthac0 >> pMobIndex->ac >> pMobIndex->hitplus >> pMobIndex->damnodice >> pMobIndex->damsizedice >> pMobIndex->damplus;

         pMobIndex->hitnodice = pMobIndex->level;
         pMobIndex->hitsizedice = 8;
      }
      else if( key == "Speaks" )
      {
         flag_set( stream, pMobIndex->speaks, lang_names );

         if( pMobIndex->speaks.none(  ) )
            pMobIndex->speaks.set( LANG_COMMON );
      }
      else if( key == "Speaking" )
      {
         std::string speaking = fread_line( stream );
         int value = get_langnum( speaking );

         if( value < 0 || value >= LANG_UNKNOWN )
            bug( "Unknown speaking language: {}", speaking );
         else
            pMobIndex->speaking = value;

         if( !pMobIndex->speaking )
            pMobIndex->speaking = LANG_COMMON;
      }
      else if( key == "Actflags" )
         flag_set( stream, pMobIndex->actflags, act_flags );
      else if( key == "Affected" )
         flag_set( stream, pMobIndex->affected_by, aff_flags );
      else if( key == "Bodyparts" )
         flag_set( stream, pMobIndex->body_parts, part_flags );
      else if( key == "Resist" )
         flag_set( stream, pMobIndex->resistant, ris_flags );
      else if( key == "Immune" )
         flag_set( stream, pMobIndex->immune, ris_flags );
      else if( key == "Suscept" )
         flag_set( stream, pMobIndex->susceptible, ris_flags );
      else if( key == "Absorb" )
         flag_set( stream, pMobIndex->absorb, ris_flags );
      else if( key == "Attacks" )
         flag_set( stream, pMobIndex->attacks, attack_flags );
      else if( key == "Defenses" )
         flag_set( stream, pMobIndex->defenses, defense_flags );
      else if( key == "ShopData" )
      {
         int iTrade;
         shop_data *pShop = new shop_data;

         pShop->keeper = pMobIndex->vnum;
         for( iTrade = 0; iTrade < MAX_TRADE; ++iTrade )
            stream >> pShop->buy_type[iTrade];

         stream >> pShop->profit_buy >> pShop->profit_sell;
         pShop->profit_buy = urange( pShop->profit_sell + 5, pShop->profit_buy, 1000 );
         pShop->profit_sell = urange( 0, pShop->profit_sell, pShop->profit_buy - 5 );

         stream >> pShop->open_hour >> pShop->close_hour;

         pMobIndex->pShop = pShop;
         shoplist.push_back( pShop );
         ++top_shop;
      }
      else if( key == "RepairData" )
      {
         int iFix;
         repair_data *rShop = new repair_data;

         rShop->keeper = pMobIndex->vnum;
         for( iFix = 0; iFix < MAX_FIX; ++iFix )
            stream >> rShop->fix_type[iFix];

         stream >> rShop->profit_fix >> rShop->shop_type >> rShop->open_hour >> rShop->close_hour;

         pMobIndex->rShop = rShop;
         repairlist.push_back( rShop );
         ++top_repair;
      }
      else if( key == "#MUDPROG" )
      {
         mud_prog_data *mprg = new mud_prog_data;
         fread_afk_mudprog( stream, mprg, pMobIndex );
         pMobIndex->mudprogs.push_back( mprg );
         ++top_prog;
      }
      else if( key == "#ENDMOBILE" )
      {
         if( !oldmob )
         {
            mob_index_table.insert( std::map<int, mob_index *>::value_type( pMobIndex->vnum, pMobIndex ) );
            tarea->mobs.push_back( pMobIndex );
            ++top_mob_index;
         }
         return;
      }
      else
      {
         bug( "{}: Bad section '{}' in mobiles - skipping.", __func__, key );
         fread_to_eol( stream );
      }
   }
}

void fread_afk_object( std::ifstream & stream, area_data * tarea )
{
   obj_index *pObjIndex = nullptr;
   bool oldobj = false;

   std::string key;
   while( stream >> key )
   {
      if( key == "Vnum" )
      {
         bool tmpBootDb = fBootDb;
         fBootDb = false;
         int vnum;
         stream >> vnum;

         if( get_obj_index( vnum ) )
         {
            if( tmpBootDb )
            {
               fBootDb = tmpBootDb;
               bug( "{}: vnum {} duplicated.", __func__, vnum );

               // Try to recover, read to end of duplicated object and then bail out
               for( ;; )
               {
                  stream >> key;
                  if( key == "#ENDOBJECT" || stream.eof() )
                     return;
               }
            }
            else
            {
               pObjIndex = get_obj_index( vnum );
               log_printf_plus( LOG_BUILD, sysdata->build_level, "Cleaning object: {}", vnum );
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
      }
      else if( key == "Keywords" )
         pObjIndex->name = fread_line( stream );
      else if( key == "Type" )
      {
         std::string o_type = fread_line( stream );
         int value = get_otype( o_type );

         if( value < 0 )
         {
            bug( "{}: vnum {}: Object has invalid type: {}. Defaulting to trash.", __func__, pObjIndex->vnum, o_type );
            value = get_otype( "trash" );
         }
         pObjIndex->item_type = value;
      }
      else if( key == "Short" )
         pObjIndex->short_descr = fread_line( stream );
      else if( key == "Long" )
         pObjIndex->objdesc = fread_line( stream );
      else if( key == "Action" )
         pObjIndex->action_desc = fread_line( stream );
      else if( key == "Flags" )
         flag_set( stream, pObjIndex->extra_flags, o_flags );
      else if( key == "WFlags" )
         flag_set( stream, pObjIndex->wear_flags, w_flags );
      else if( key == "Values" )
      {
         int x1, x2, x3, x4, x5, x6, x7, x8, x9, x10, x11;
         stream >> x1 >> x2 >> x3 >> x4 >> x5 >> x6 >> x7 >> x8 >> x9 >> x10 >> x11;

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
      }
      else if( key == "Stats" )
      {
         std::string ln;
         std::getline( stream, ln );

         int x1 = 0, x2 = 0, x3 = 0, x4 = 0, x5 = 0;
         std::string s1 = "None", s2 = "None", s3 = "None";
         std::istringstream( ln ) >> x1 >> x2 >> x3 >> x4 >> x5 >> s1 >> s2 >> s3;

         pObjIndex->weight = x1;
         pObjIndex->weight = umax( 1, pObjIndex->weight );
         pObjIndex->cost = x2;
         pObjIndex->ego = x3;
         pObjIndex->limit = x4;
         pObjIndex->layers = x5;

         pObjIndex->socket[0] = s1;
         pObjIndex->socket[1] = s2;
         pObjIndex->socket[2] = s3;

         if( pObjIndex->ego > 90 )
            pObjIndex->ego = -2;
      }
      else if( key == "AffData" )
      {
         affect_data *af = fread_afk_affect( stream );

         if( af )
            pObjIndex->affects.push_back( af );
      }
      else if( key == "Spells" )
      {
         switch( pObjIndex->item_type )
         {
            default:
               break;

            case ITEM_PILL:
            case ITEM_POTION:
            case ITEM_SCROLL:
               pObjIndex->value[1] = skill_lookup( fread_word( stream ) );
               pObjIndex->value[2] = skill_lookup( fread_word( stream ) );
               pObjIndex->value[3] = skill_lookup( fread_word( stream ) );
               break;

            case ITEM_STAFF:
            case ITEM_WAND:
               pObjIndex->value[3] = skill_lookup( fread_word( stream ) );
               break;

            case ITEM_SALVE:
               pObjIndex->value[4] = skill_lookup( fread_word( stream ) );
               pObjIndex->value[5] = skill_lookup( fread_word( stream ) );
               break;
         }
      }
      else if( key == "#EXDESC" )
      {
         extra_descr_data *ed = fread_afk_exdesc( stream );
         if( ed )
         {
            pObjIndex->extradesc.push_back( ed );
            ++top_ed;
         }
      }
      else if( key == "#MUDPROG" )
      {
         mud_prog_data *mprg = new mud_prog_data;
         fread_afk_mudprog( stream, mprg, pObjIndex );
         pObjIndex->mudprogs.push_back( mprg );
         ++top_prog;
      }
      else if( key == "#ENDOBJECT" )
      {
         if( !oldobj )
         {
            obj_index_table.insert( std::map<int, obj_index *>::value_type( pObjIndex->vnum, pObjIndex ) );
            tarea->objects.push_back( pObjIndex );
            ++top_obj_index;
         }
         return;
      }
      else
      {
         bug( "{}: Bad section '{}' in objects - skipping.", __func__, key );
         fread_to_eol( stream );
      }
   }
}

void fread_afk_room( std::ifstream & stream, area_data * tarea )
{
   room_index *pRoomIndex = nullptr;
   bool oldroom = false;

   std::string key;
   while( stream >> key )
   {
      if( key == "Vnum" )
      {
         bool tmpBootDb = fBootDb;
         fBootDb = false;

         int vnum;
         stream >> vnum;

         if( get_room_index( vnum ) )
         {
            if( tmpBootDb )
            {
               fBootDb = tmpBootDb;
               bug( "{}: vnum {} duplicated.", __func__, vnum );

               // Try to recover, read to end of duplicated room and then bail out
               for( ;; )
               {
                  stream >> key;
                  if( key == "#ENDROOM" || stream.eof() )
                     return;
               }
            }
            else
            {
               pRoomIndex = get_room_index( vnum );
               log_printf_plus( LOG_BUILD, sysdata->build_level, "Cleaning room: {}", vnum );
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

               bug( "{}: WARNING: resets already exist for this room.", __func__ );
               for( auto* rtmp : pRoomIndex->resets )
               {
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
               log_printf_plus( LOG_BUILD, sysdata->build_level, "Cleaning resets: {}", tarea->name );
               pRoomIndex->clean_resets(  );
            }
         }
      }
      else if( key == "Name" )
         pRoomIndex->name = fread_line( stream );
      else if( key == "Sector" )
      {
         std::string sec_type = fread_line( stream );
         int sector = get_sectypes( sec_type );

         if( sector < 0 || sector >= SECT_MAX )
         {
            bug( "{}: Room #{} has bad sector type: {}.", __func__, pRoomIndex->vnum, sec_type );
            sector = 1;
         }

         pRoomIndex->sector_type = sector;
         pRoomIndex->winter_sector = -1;
      }
      else if( key == "Flags" )
         flag_set( stream, pRoomIndex->flags, r_flags );
      else if( key == "Stats" )
      {
         int x1, x2, x3, x4, x5;
         stream >> x1 >> x2 >> x3 >> x4 >> x5;

         pRoomIndex->tele_delay = x1;
         pRoomIndex->tele_vnum = x2;
         pRoomIndex->tunnel = x3;
         pRoomIndex->baselight = x4;
         pRoomIndex->light = x4;

         if( x5 == 0 )
            x5 = 100000;
         pRoomIndex->max_weight = x5;  // Imported from Smaug 1.8
      }
      else if( key == "Desc" )
         pRoomIndex->roomdesc = fread_line( stream );
      else if( key == "Nightdesc" )
         pRoomIndex->nitedesc = fread_line( stream );
      else if( key == "AffData" )
      {
         affect_data *af = fread_afk_affect( stream );

         if( af )
            pRoomIndex->permaffects.push_back( af );
      }
      else if( key == "#EXIT" )
         fread_afk_exit( stream, pRoomIndex );
      else if( key == "#EXDESC" )
      {
         extra_descr_data *ed = fread_afk_exdesc( stream );
         if( ed )
            pRoomIndex->extradesc.push_back( ed );
      }
      else if( key == "#MUDPROG" )
      {
         mud_prog_data *mprg = new mud_prog_data;
         fread_afk_mudprog( stream, mprg, pRoomIndex );
         pRoomIndex->mudprogs.push_back( mprg );
         ++top_prog;
      }
      else if( key == "Reset" )
         pRoomIndex->load_reset( stream, true );
      else if( key == "#ENDROOM" )
      {
         if( !oldroom )
         {
            room_index_table.insert( std::map<int, room_index *>::value_type( pRoomIndex->vnum, pRoomIndex ) );
            tarea->rooms.push_back( pRoomIndex );
            ++top_room;
         }
         return;
      }
      else
      {
         bug( "{}: Bad section '{}' in rooms - skipping.", __func__, key );
         fread_to_eol( stream );
      }
   }
}

// AFKMud 2.0 Area file format. Liberal use of KEY macro support. Far more flexible.
area_data *fread_afk_area( std::ifstream & stream )
{
   area_data *tarea = nullptr;

   std::string key;
   while( stream >> key )
   {
      if( key == "#AREADATA" )
      {
         tarea = create_area(  );
         tarea->filename = strArea;
         fread_afk_areadata( stream, tarea );
      }
      else if( key == "#MOBILE" )
         fread_afk_mobile( stream, tarea );
      else if( key == "#OBJECT" )
         fread_afk_object( stream, tarea );
      else if( key == "#ROOM" )
         fread_afk_room( stream, tarea );
      else if( key == "#ENDAREA" )
         break;
      else
      {
         bug( "{}: Bad section header: {}", __func__, key );
         fread_to_eol( stream );
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
   log_printf( "{:<20}: Version {:<3} Vnums: {:5} - {:<5}", tarea->filename, tarea->version, tarea->low_vnum, tarea->hi_vnum );
   if( tarea->low_vnum < 0 || tarea->hi_vnum < 0 )
      log_printf( "{:<20}: Bad Vnum Range", tarea->filename );
   if( tarea->author.empty() )
      tarea->author = "AFKMud";
}

void load_area_file( const std::string & filename, bool isproto )
{
   area_data *tarea = nullptr;

   fpArea.open( filename );
   if( !fpArea.is_open() )
   {
      bug( "{}: Cannot open {} for reading: {}", __func__, filename, std::strerror(errno) );
      return;
   }

   if( fread_letter( fpArea ) != '#' )
   {
      if( fBootDb )
      {
         bug( "{}: No # found at start of area file.", __func__ );
         std::exit( EXIT_FAILURE );
      }
      else
      {
         bug( "{}: No # found at start of area file.", __func__ );
         fpArea.close();
         return;
      }
   }

   std::string word = fread_word( fpArea );

   // New AFKMud area format support -- Samson 10/28/06
   if( word == "AFKAREA" )
   {
      tarea = fread_afk_area( fpArea );
      fpArea.close();

      if( tarea )
         process_sorting( tarea, isproto );
      return;
   }

   // Support conversions from other bases if encountered. -- Samson 12/31/06
   log_printf( "Area format conversion: {}", filename );

   if( word == "FUSSAREA" )
   {
      log_string( "SmaugFUSS 1.7+ area encountered. Passing to SmaugFUSS area parser." );

      tarea = fread_smaugfuss_area( fpArea );
      fpArea.close();

      if( tarea )
         process_sorting( tarea, isproto );
      return;
   }

   // Found #AREA, let the process begin!
   if( word == "AREA" )
   {
      tarea = create_area();

      tarea->filename = filename;
      tarea->name = fread_line( fpArea );
   }
   else
   {
      // If you found #VERSION first though, it's a SmaugWiz zone or it's invalid.
      if( word == "VERSION" )
      {
         int temp;
         fpArea >> temp;
         if( temp >= 1000 )
         {
            fpArea.close();
            do_areaconvert( nullptr, filename );
            return;
         }

         bug( "{}: Invalid header at start of area file: {}", __func__, word );
         if( fBootDb )
            std::exit( EXIT_FAILURE );
         else
         {
            fpArea.close();
            return;
         }
      }

      bug( "{}: Invalid header at start of area file: {}", __func__, word );
      if( fBootDb )
         std::exit( EXIT_FAILURE );
      else
      {
         fpArea.close();
         return;
      }
   }

   for( ;; )
   {
      if( !fpArea.is_open() )  /* Should only happen if a stock conversion takes place */
         return;

      if( fread_letter( fpArea ) != '#' )
      {
         bug( "{}: # not found {}", __func__, tarea->filename );
         std::exit( EXIT_FAILURE );
      }

      word = fread_word( fpArea );

      if( word[0] == '$' )
         break;

      else if( word == "VERSION" )
      {
         int aversion;
         fpArea >> aversion;

         if( aversion <= 3 )
         {
            log_string( "Smaug 1.02a, 1.4a, or 1.8b area encountered. Attempting to pass to Area Converter." );
            fpArea.close();
            deleteptr( tarea );
            --top_area;
            do_areaconvert( nullptr, filename );
            return;
         }

         if( aversion == 1000 )
         {
            log_string( "SmaugWiz 2.02 area encountered. Attempting to pass to Area Converter." );
            fpArea.close();
            deleteptr( tarea );
            do_areaconvert( nullptr, filename );
            return;
         }

         // AFKMud 1.x files only went up to version 20. Anything higher is invalid.
         // This is gonna become a problem for old areas if RoD ever releases another version of SMAUG and they bump their area version past 3.
         if( aversion > 3 && aversion <= 20 )
         {
            bool oldafk_fail = load_oldafk_area( fpArea, tarea, aversion );

            if( !oldafk_fail )
               break;
            else
            {
               bug( "{}: Bad conversion from AFKMud 1.x format. Cannot continue.", __func__ );
               if( fBootDb )
                  std::exit( EXIT_FAILURE );
               else
               {
                  fpArea.close();
               }
            }
            return;
         }

         bug( "{}: {}:{} - Bad version header. Format conversion failed.", __func__, word, aversion );
         if( fBootDb )
            std::exit( EXIT_FAILURE );
         else
         {
            fpArea.close();
            return;
         }
      }
      else
      {
         bug( "{}: {} bad section name {}", __func__, tarea->filename, word );
         if( fBootDb )
            std::exit( EXIT_FAILURE );
         else
         {
            fpArea.close();
            return;
         }
      }
   }
   fpArea.close();

   if( tarea )
      process_sorting( tarea, isproto );
   else
      log_printf( "({})", filename );
}

void fwrite_afk_affect( std::ofstream & stream, affect_data * af )
{
   if( af->location == APPLY_AFFECT )
      stream << std::format( "AffData {} '{}' {} {} {}\n", a_types[af->location], aff_flags[af->modifier], af->type, af->duration, af->bit );
   else if( af->location == APPLY_WEAPONSPELL || af->location == APPLY_WEARSPELL || af->location == APPLY_REMOVESPELL || af->location == APPLY_STRIPSN
         || af->location == APPLY_RECURRINGSPELL || af->location == APPLY_EAT_SPELL )
      stream << std::format( "AffData {} '{}' {} {} {}\n", a_types[af->location],
               IS_VALID_SN( af->modifier ) ? skill_table[af->modifier]->name : "UNKNOWN", af->type, af->duration, af->bit );
   else if( af->location == APPLY_RESISTANT || af->location == APPLY_IMMUNE || af->location == APPLY_SUSCEPTIBLE || af->location == APPLY_ABSORB )
      stream << std::format( "AffData {} {}~ {} {} {}\n", a_types[af->location], bitset_string( af->rismod, ris_flags ), af->type, af->duration, af->bit );
   else
      stream << std::format( "AffData {} {} {} {} {}\n", a_types[af->location], af->modifier, af->type, af->duration, af->bit );
}

void fwrite_afk_exdesc( std::ofstream & stream, extra_descr_data * desc )
{
   stream << "#EXDESC\n";
   stream << std::format( "ExDescKey    {}~\n", desc->keyword );
   if( !desc->desc.empty(  ) )
      stream << std::format( "ExDesc       {}~\n", strip_cr( desc->desc ) );
   stream << "#ENDEXDESC\n\n";
}

void fwrite_afk_exit( std::ofstream & stream, exit_data * pexit )
{
   stream << "#EXIT\n";
   stream << std::format( "Direction {}~\n", strip_cr( dir_name[pexit->vdir] ) );
   stream << std::format( "ToRoom    {}\n", pexit->vnum );
   if( pexit->key > 0 )
      stream << std::format( "Key       {}\n", pexit->key );
   if( IS_EXIT_FLAG( pexit, EX_OVERLAND ) && pexit->map_x != -1 && pexit->map_y != -1 )
      stream << std::format( "ToCoords  {} {}\n", pexit->map_x, pexit->map_y );
   if( pexit->pull )
      stream << std::format( "Pull      {} {}\n", pexit->pulltype, pexit->pull );
   if( !pexit->exitdesc.empty() )
      stream << std::format( "Desc      {}~\n", strip_cr( pexit->exitdesc ) );
   if( !pexit->keyword.empty() )
      stream << std::format( "Keywords  {}~\n", strip_cr( pexit->keyword ) );
   if( pexit->flags.any(  ) )
      stream << std::format( "Flags     {}~\n", bitset_string( pexit->flags, ex_flags ) );
   stream << "#ENDEXIT\n\n";
}

// Write a prog
bool mprog_write_prog( std::ofstream & stream, mud_prog_data * mprog )
{
   if( !mprog->arglist.empty() )
   {
      stream << "#MUDPROG\n";
      stream << std::format( "Progtype  {}~\n", mprog_type_to_name( mprog->type ) );
      stream << std::format( "Arglist   {}~\n", mprog->arglist );

      if( !mprog->comlist.empty() && !mprog->fileprog )
         stream << std::format( "Comlist   {}~\n", strip_cr( mprog->comlist ) );

      stream << "#ENDPROG\n\n";
      return true;
   }
   return false;
}

void save_reset_level( std::ofstream & stream, const std::list<reset_data *> & source, const int level )
{
   int spaces = level * 2;

   for( auto* pReset : source )
   {
      switch ( to_upper( pReset->command ) ) /* extra arg1 arg2 arg3 */
      {
         default:
            bug( "{}: Invalid reset type {}", __func__, to_upper( pReset->command ) );
            break;

         case '*':
            break;

         case 'Z':  // RT room object - no sub-resets
            stream << std::format( "{:>{}}Reset {} {} {} {} {} {} {} {} {} {} {} {}\n", "", spaces, to_upper( pReset->command ),
                     pReset->arg1, pReset->arg2, pReset->arg3, pReset->arg4, pReset->arg5, pReset->arg6,
                     pReset->arg7, pReset->arg8, pReset->arg9, pReset->arg10, pReset->arg11 );
            break;

         case 'M':
         case 'O':
         case 'Y':  // RT give - has no sub-resets
            stream << std::format( "{:>{}}Reset {} {} {} {} {} {} {} {}\n", "", spaces, to_upper( pReset->command ),
                     pReset->arg1, pReset->arg2, pReset->arg3, pReset->arg4, pReset->arg5, pReset->arg6, pReset->arg7 );
            break;

         case 'W':  // RT put - has no sub-resets
            stream << std::format( "{:>{}}Reset {} {} {} {} {} {} {} {} {} {}\n", "", spaces, to_upper( pReset->command ),
                     pReset->arg1, pReset->arg2, pReset->arg3, pReset->arg4, pReset->arg5, pReset->arg6, pReset->arg7, pReset->arg8, pReset->arg9 );
            break;

         case 'P':
         case 'E':
            if( pReset->command == 'E' )
               stream << std::format( "{:>{}}Reset {} {} {} {} {}\n", "", spaces, to_upper( pReset->command ), pReset->arg1, pReset->arg2, pReset->arg3, pReset->arg4 );
            else
               stream << std::format( "{:>{}}Reset {} {} {} {} {} {}\n", "", spaces, to_upper( pReset->command ), pReset->arg1, pReset->arg2, pReset->arg3, pReset->arg4, pReset->arg5 );
            break;

         case 'G':
         case 'R':
            stream << std::format( "{:>{}}Reset {} {} {} {}\n", "", spaces, to_upper( pReset->command ), pReset->arg1, pReset->arg2, pReset->arg3 );
            break;

         case 'X':  // RT equipped - has no sub-resets
            stream << std::format( "{:>{}}Reset {} {} {} {} {} {} {} {} {}\n", "", spaces, to_upper( pReset->command ),
                     pReset->arg1, pReset->arg2, pReset->arg3, pReset->arg4, pReset->arg5, pReset->arg6, pReset->arg7, pReset->arg8 );
            break;

         case 'T':
         case 'H':
         case 'D':
            stream << std::format( "{:>{}}Reset {} {} {} {} {} {}\n", "", spaces, to_upper( pReset->command ), pReset->arg1, pReset->arg2, pReset->arg3, pReset->arg4, pReset->arg5 );
            break;
      }  /* end of switch on command */

      /*
       * recurse to save nested resets 
       */
      if( !pReset->resets.empty(  ) )
         save_reset_level( stream, pReset->resets, level + 1 );
   }  /* end of looping through resets */
}  /* end of save_reset_level */

// Write out the top header for the file.
void fwrite_area_header( area_data * area, std::ofstream & stream )
{
   stream << "#AREADATA\n";
   stream << std::format( "Version         {}\n", area->version );
   stream << std::format( "Name            {}~\n", area->name );
   stream << std::format( "Author          {}~\n", area->author );
   if( !area->credits.empty() )
      stream << std::format( "Credits         {}~\n", area->credits );
   stream << std::format( "Vnums           {} {}\n", area->low_vnum, area->hi_vnum );
   if( area->continent )
      stream << std::format( "Continent       {}~\n", area->continent->name );
   stream << std::format( "Coordinates     {} {}\n", area->map_x, area->map_y );

   auto cdate = std::chrono::system_clock::to_time_t( area->creation_date );
   auto idate = std::chrono::system_clock::to_time_t( area->install_date );

   stream << std::format( "Dates           {} {}\n", cdate, idate );
   stream << std::format( "Ranges          {} {} {} {}\n", area->low_soft_range, area->hi_soft_range, area->low_hard_range, area->hi_hard_range );
   if( !area->resetmsg.empty() ) /* Rennard */
      stream << std::format( "ResetMsg        {}~\n", area->resetmsg );
   if( area->reset_frequency )
      stream << std::format( "ResetFreq       {}\n", area->reset_frequency );
   if( area->flags.any(  ) )
      stream << std::format( "Flags           {}~\n", bitset_string( area->flags, area_flags ) );
   stream << std::format( "Treasure        {} {} {} {} {} {} {} {}\n",
            area->tg_nothing, area->tg_gold, area->tg_item, area->tg_gem, area->tg_scroll, area->tg_potion, area->tg_wand, area->tg_armor );
   stream << std::format( "WeatherCoords   {} {}\n\n", area->weatherx, area->weathery );

   stream << "#ENDAREADATA\n\n";
}

// Write out an individual mob
void fwrite_afk_mobile( std::ofstream & stream, mob_index * pMobIndex, bool install )
{
   shop_data *pShop = nullptr;
   repair_data *pRepair = nullptr;

   if( install )
      pMobIndex->actflags.reset( ACT_PROTOTYPE );

   stream << "#MOBILE\n";
   stream << std::format( "Vnum      {}\n", pMobIndex->vnum );
   stream << std::format( "Keywords  {}~\n", pMobIndex->player_name );
   stream << std::format( "Race      {}~\n", npc_race[pMobIndex->race] );
   stream << std::format( "Class     {}~\n", npc_class[pMobIndex->Class] );
   stream << std::format( "Gender    {}~\n", npc_sex[pMobIndex->sex] );
   stream << std::format( "Position  {}~\n", npc_position[pMobIndex->position] );
   stream << std::format( "DefPos    {}~\n", npc_position[pMobIndex->defposition] );
   if( pMobIndex->spec_fun && !pMobIndex->spec_funname.empty(  ) )
      stream << std::format( "Specfun   {}~\n", pMobIndex->spec_funname );
   stream << std::format( "Short     {}~\n", pMobIndex->short_descr );
   if( !pMobIndex->long_descr.empty() )
      stream << std::format( "Long      {}~\n", strip_cr( pMobIndex->long_descr ) );
   if( !pMobIndex->chardesc.empty() )
      stream << std::format( "Desc      {}~\n", strip_cr( pMobIndex->chardesc ) );
   stream << std::format( "Nattacks  {:.6f}\n", pMobIndex->numattacks );
   stream << std::format( "Stats1    {} {} {} {} {} {}\n", pMobIndex->alignment, pMobIndex->gold, pMobIndex->height, pMobIndex->weight, pMobIndex->max_move, pMobIndex->max_mana );
   stream << std::format( "Stats2    {} {} {} {} {} {} {}\n",
            pMobIndex->level, pMobIndex->mobthac0, pMobIndex->ac, pMobIndex->hitplus, pMobIndex->damnodice, pMobIndex->damsizedice, pMobIndex->damplus );
   if( pMobIndex->speaks.any(  ) )
      stream << std::format( "Speaks    {}~\n", bitset_string( pMobIndex->speaks, lang_names ) );
   stream << std::format( "Speaking  {}~\n", lang_names[pMobIndex->speaking] );
   if( pMobIndex->actflags.any(  ) )
      stream << std::format( "Actflags  {}~\n", bitset_string( pMobIndex->actflags, act_flags ) );
   if( pMobIndex->affected_by.any(  ) )
      stream << std::format( "Affected  {}~\n", bitset_string( pMobIndex->affected_by, aff_flags ) );
   if( pMobIndex->body_parts.any(  ) )
      stream << std::format( "Bodyparts {}~\n", bitset_string( pMobIndex->body_parts, part_flags ) );
   if( pMobIndex->resistant.any(  ) )
      stream << std::format( "Resist    {}~\n", bitset_string( pMobIndex->resistant, ris_flags ) );
   if( pMobIndex->immune.any(  ) )
      stream << std::format( "Immune    {}~\n", bitset_string( pMobIndex->immune, ris_flags ) );
   if( pMobIndex->susceptible.any(  ) )
      stream << std::format( "Suscept   {}~\n", bitset_string( pMobIndex->susceptible, ris_flags ) );
   if( pMobIndex->absorb.any(  ) )
      stream << std::format( "Absorb    {}~\n", bitset_string( pMobIndex->absorb, ris_flags ) );
   if( pMobIndex->attacks.any(  ) )
      stream << std::format( "Attacks   {}~\n", bitset_string( pMobIndex->attacks, attack_flags ) );
   if( pMobIndex->defenses.any(  ) )
      stream << std::format( "Defenses  {}~\n", bitset_string( pMobIndex->defenses, defense_flags ) );

   // Mob has a shop? Add that data to the mob index.
   if( ( pShop = pMobIndex->pShop ) != nullptr )
   {
      stream << std::format( "ShopData   {} {} {} {} {} {} {} {} {}\n",
               pShop->buy_type[0], pShop->buy_type[1], pShop->buy_type[2], pShop->buy_type[3], pShop->buy_type[4],
               pShop->profit_buy, pShop->profit_sell, pShop->open_hour, pShop->close_hour );
   }

   // Mob is a repair shop? Add that data to the mob index.
   if( ( pRepair = pMobIndex->rShop ) != nullptr )
   {
      stream << std::format( "RepairData {} {} {} {} {} {} {}\n",
               pRepair->fix_type[0], pRepair->fix_type[1], pRepair->fix_type[2], pRepair->profit_fix, pRepair->shop_type, pRepair->open_hour, pRepair->close_hour );
   }

   for( auto* mp : pMobIndex->mudprogs )
      mprog_write_prog( stream, mp );

   stream << "#ENDMOBILE\n\n";
}

// Write out an individual obj
void fwrite_afk_object( std::ofstream & stream, obj_index * pObjIndex, bool install )
{
   if( install )
      pObjIndex->extra_flags.reset( ITEM_PROTOTYPE );

   stream << "#OBJECT\n";
   stream << std::format( "Vnum      {}\n", pObjIndex->vnum );
   stream << std::format( "Keywords  {}~\n", pObjIndex->name );
   stream << std::format( "Type      {}~\n", o_types[pObjIndex->item_type] );
   stream << std::format( "Short     {}~\n", pObjIndex->short_descr );
   if( !pObjIndex->objdesc.empty() )
      stream << std::format( "Long      {}~\n", strip_cr( pObjIndex->objdesc ) );
   if( !pObjIndex->action_desc.empty() )
      stream << std::format( "Action    {}~\n", pObjIndex->action_desc );
   if( pObjIndex->extra_flags.any(  ) )
      stream << std::format( "Flags     {}~\n", bitset_string( pObjIndex->extra_flags, o_flags ) );
   if( pObjIndex->wear_flags.any(  ) )
      stream << std::format( "WFlags    {}~\n", bitset_string( pObjIndex->wear_flags, w_flags ) );

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
   stream << std::format( "Values    {} {} {} {} {} {} {} {} {} {} {}\n", val0, val1, val2, val3, val4, val5, val6, val7, val8, val9, val10 );

   stream << std::format( "Stats     {} {} {} {} {} {} {} {}\n",
            pObjIndex->weight, pObjIndex->cost, pObjIndex->ego,
            pObjIndex->limit, pObjIndex->layers,
            !pObjIndex->socket[0].empty() ? pObjIndex->socket[0] : "None", !pObjIndex->socket[1].empty() ? pObjIndex->socket[1] : "None", pObjIndex->socket[2].empty() ? pObjIndex->socket[2] : "None" );

   for( auto* af : pObjIndex->affects )
      fwrite_afk_affect( stream, af );

   switch ( pObjIndex->item_type )
   {
      default:
         break;

      case ITEM_PILL:
      case ITEM_POTION:
      case ITEM_SCROLL:
         stream << std::format( "Spells       '{}' '{}' '{}'\n",
                  IS_VALID_SN( pObjIndex->value[1] ) ? skill_table[pObjIndex->value[1]]->name : "NONE",
                  IS_VALID_SN( pObjIndex->value[2] ) ? skill_table[pObjIndex->value[2]]->name : "NONE",
                  IS_VALID_SN( pObjIndex->value[3] ) ? skill_table[pObjIndex->value[3]]->name : "NONE" );
         break;

      case ITEM_STAFF:
      case ITEM_WAND:
         stream << std::format( "Spells       '{}'\n", IS_VALID_SN( pObjIndex->value[3] ) ? skill_table[pObjIndex->value[3]]->name : "NONE" );
         break;

      case ITEM_SALVE:
         stream << std::format( "Spells       '{}' '{}'\n",
                  IS_VALID_SN( pObjIndex->value[4] ) ? skill_table[pObjIndex->value[4]]->name : "NONE",
                  IS_VALID_SN( pObjIndex->value[5] ) ? skill_table[pObjIndex->value[5]]->name : "NONE" );
         break;
   }

   for( auto* desc : pObjIndex->extradesc )
      fwrite_afk_exdesc( stream, desc );

   for( auto* mp : pObjIndex->mudprogs )
      mprog_write_prog( stream, mp );

   stream << "#ENDOBJECT\n\n";
}

// Write out an individual room
void fwrite_afk_room( std::ofstream & stream, room_index * room, bool install )
{
   if( install )
   {
      // remove prototype flag from room
      room->flags.reset( ROOM_PROTOTYPE );

      // purge room of (prototyped) mobiles
      for( auto it = room->people.begin(  ); it != room->people.end(  ); )
      {
         char_data *victim = *it;
         ++it;

         if( victim->isnpc(  ) && victim->has_actflag( ACT_PROTOTYPE ) )
            victim->extract( true );
      }

      // purge room of (prototyped) objects
      for( auto it = room->objects.begin(  ); it != room->objects.end(  ); )
      {
         obj_data *obj = *it;
         ++it;

         if( obj->extra_flags.test( ITEM_PROTOTYPE ) )
            obj->extract(  );
      }
   }

   stream << "#ROOM\n";

   /*
    * Take off track flags before saving 
    */
   if( room->flags.test( ROOM_TRACK ) )
      room->flags.reset( ROOM_TRACK );

   stream << std::format( "Vnum      {}\n", room->vnum );
   stream << std::format( "Name      {}~\n", strip_cr( room->name ) );

   /*
    * Retain the ORIGINAL sector type to the area file - Samson 7-19-00 
    */
   if( time_info.season == SEASON_WINTER && room->winter_sector != -1 )
      room->sector_type = room->winter_sector;
   stream << std::format( "Sector    {}~\n", strip_cr( sect_types[room->sector_type] ) );

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

   stream << std::format( "Flags     {}~\n", bitset_string( room->flags, r_flags ) );
   stream << std::format( "Stats     {} {} {} {} {}\n", room->tele_delay, room->tele_vnum, room->tunnel, room->baselight, room->max_weight );

   if( !room->roomdesc.empty() )
      stream << std::format( "Desc      {}~\n", strip_cr( room->roomdesc ) );

   /*
    * write NiteDesc's -- Dracones 
    */
   if( !room->nitedesc.empty() )
      stream << std::format( "Nightdesc {}~\n", strip_cr( room->nitedesc ) );

   // Save the list of room index affects.
   for( auto* af : room->permaffects )
      fwrite_afk_affect( stream, af );

   // Save the list of room exits.
   for( auto* pexit : room->exits )
   {
      if( IS_EXIT_FLAG( pexit, EX_PORTAL ) ) // Don't fold portals
         continue;

      fwrite_afk_exit( stream, pexit );
   }

   // Save the list of extra descriptions.
   for( auto* desc : room->extradesc )
      fwrite_afk_exdesc( stream, desc );

   // Save the list of mudprogs.
   for( auto* mp : room->mudprogs )
      mprog_write_prog( stream, mp );

   // Recursive function that saves the nested resets.
   save_reset_level( stream, room->resets, 0 );

   stream << "#ENDROOM\n\n";
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
 * Removing existing ones will require updated version handling unless you like dirty logs.
 *
 * Version 1: Initial construction of new format -- Samson 10/28/06
 */
constexpr int AREA_VERSION_WRITE = 1;

void area_data::fold( const std::string & fname, bool install )
{
   log_printf_plus( LOG_BUILD, LEVEL_GREATER, "Saving {}...", this->filename );

   std::filesystem::path buf = std::format( "{}.bak", fname );
   std::filesystem::rename( fname, buf );
   std::ofstream stream( this->filename );
   if( !stream.is_open() )
   {
      bug( "{}: Cannot open {} for writing: {}", __func__, this->filename, std::strerror(errno) );
      return;
   }

   this->version = AREA_VERSION_WRITE;

   stream << "#AFKAREA\n";

   fwrite_area_header( this, stream );

   for( auto* pMobIndex : mobs )
      fwrite_afk_mobile( stream, pMobIndex, install );

   for( auto* pObjIndex : objects )
      fwrite_afk_object( stream, pObjIndex, install );

   for( auto* pRoomIndex : rooms )
      fwrite_afk_room( stream, pRoomIndex, install );

   stream << "#ENDAREA\n";
   stream.close();
   if( stream.fail() )
      bug( "{}: Error occurred after closing {}: ", __func__, this->filename, std::strerror(errno) );
}

void close_all_areas( void )
{
   for( auto it = arealist.begin(); it != arealist.end(); )
   {
      area_data *pArea = *it;
      ++it;

      deleteptr( pArea );
   }
}

CMDF( do_savearea )
{
   area_data *tarea;
   std::filesystem::path fname;

   if( ch->isnpc(  ) || ch->get_trust(  ) < LEVEL_CREATOR || !ch->pcdata || ( argument[0] == '\0' && !ch->pcdata->area ) )
   {
      ch->print( "&[immortal]You don't have an assigned area to save.\r\n" );
      return;
   }

   if( argument.empty(  ) )
      tarea = ch->pcdata->area;
   else if( ch->is_imp(  ) && !str_cmp( argument, "all" ) )
   {
      std::list<area_data *>::iterator iarea;

      for( iarea = arealist.begin(  ); iarea != arealist.end(  ); ++iarea )
      {
         tarea = *iarea;

         if( !tarea->flags.test( AFLAG_PROTOTYPE ) )
            continue;
         fname = std::format( "{}{}", BUILD_DIR, tarea->filename );
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
      ch->print_fmt( "&[immortal]Cannot savearea {}, use foldarea instead.\r\n", tarea->filename );
      return;
   }
   fname = std::format( "{}{}", BUILD_DIR, tarea->filename );
   ch->print( "&[immortal]Saving area...\r\n" );
   tarea->fold( fname, false );
   ch->print( "&[immortal]Done.\r\n" );
}

CMDF( do_foldarea )
{
   ch->set_color( AT_IMMORT );

   if( argument.empty(  ) )
   {
      ch->print( "Fold what?\r\n" );
      return;
   }

   if( !str_cmp( argument, "all" ) )
   {
      for( auto* tarea : arealist )
      {
         if( !tarea->flags.test( AFLAG_PROTOTYPE ) )
            tarea->fold( tarea->filename, false );
      }
      ch->print( "&[immortal]Folding completed.\r\n" );
      return;
   }

   area_data *tarea;
   if( !( tarea = find_area( argument ) ) )
   {
      ch->print( "No such area exists.\r\n" );
      return;
   }

   if( tarea->flags.test( AFLAG_PROTOTYPE ) )
   {
      ch->print_fmt( "Cannot fold {}, use savearea instead.\r\n", tarea->filename );
      return;
   }
   ch->print( "Folding area...\r\n" );
   tarea->fold( tarea->filename, false );
   ch->print( "&[immortal]Done.\r\n" );
}

void write_area_list( void )
{
   std::ofstream stream;

   stream.open( std::filesystem::path( AREA_LIST ) );
   if( !stream )
   {
      bug( "{}: Cannot open {} for writing: {}", __func__, AREA_LIST, std::strerror(errno) );
      return;
   }

   for( auto* area : arealist )
   {
      if( !area->flags.test( AFLAG_PROTOTYPE ) )
         stream << area->filename << "\n";
   }
   stream << "$\n";
   stream.close();
   if( stream.fail() )
      bug( "{}: Error occurred after closing {}: ", __func__, AREA_LIST, std::strerror(errno) );
}

/*
 * Returns area with name matching input string
 * Last Modified : July 21, 1997
 * Fireblade
 */
area_data *get_area( std::string_view name )
{
   if( name.empty(  ) )
      return nullptr;

   for( auto* area : arealist )
   {
      if( hasname( area->name, name ) )
         return area;
   }
   return nullptr;
}

/* Locate an area by its filename first, then fall back to other means if not found */
area_data *find_area( std::string_view filename )
{
   for( auto* area : arealist )
   {
      if( !str_cmp( area->filename, filename ) )
         return area;
   }
   return get_area( filename );
}

CMDF( do_adelete )
{
   area_data *tarea;
   std::string arg;
   std::filesystem::path filename;

   if( argument.empty(  ) )
   {
      ch->print( "Usage: adelete <areafilename>\r\n" );
      return;
   }

   argument = one_argument( argument, arg );

   if( !( tarea = find_area( arg ) ) )
   {
      ch->print_fmt( "No such area as {}\r\n", arg );
      return;
   }

   if( argument.empty(  ) || str_cmp( argument, "yes" ) )
   {
      ch->print( "&RThis action must be confirmed before executing. It is not reversible.\r\n" );
      ch->print_fmt( "&RTo delete this area, type: &Wadelete {} yes&D", arg );
      return;
   }
   if( tarea->flags.test( AFLAG_PROTOTYPE ) )
      filename = std::format( "{}{}", BUILD_DIR, tarea->filename );
   else
      filename = tarea->filename;
   deleteptr( tarea );
   std::filesystem::remove( filename );
   write_area_list(  );
   web_arealist(  );
   ch->print_fmt( "&W{}&R has been destroyed.&D\r\n", arg );
}

void assign_area( char_data * ch )
{
   std::string taf;
   area_data *tarea;

   if( ch->isnpc(  ) )
      return;

   if( ch->get_trust(  ) > LEVEL_IMMORTAL && ch->pcdata->low_vnum && ch->pcdata->hi_vnum )
   {
      tarea = ch->pcdata->area;
      taf = std::format( "{}.are", capitalize( ch->name ) );
      if( !tarea )
         tarea = find_area( taf );
      if( !tarea )
      {
         log_printf_plus( LOG_BUILD, ch->level, "Creating area entry for {}", ch->name );

         tarea = create_area(  );
         tarea->name = std::format( "[PROTO] {}'s area in progress", ch->name );
         tarea->filename = taf;
         tarea->author = ch->name;
         tarea->sort_name(  );
         tarea->sort_vnums(  );
      }
      else
         log_printf_plus( LOG_BUILD, ch->level, "Updating area entry for {}", ch->name );

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
      ch->print_fmt( "The area '{}' does not exsist. Please use the 'zones' command for a list.\r\n", argument );
      return;
   }

   if( !tarea->flags.test( AFLAG_PROTOTYPE ) && ch->get_trust(  ) < sysdata->level_modify_proto )
   {
      ch->print_fmt( "The area '{}' is not a proto area, and you're not authorized to work on non-proto areas.\r\n", tarea->name );
      return;
   }

   ch->pcdata->area = tarea;
   ch->print_fmt( "Assigning you: {}\r\n", tarea->name );
   log_printf( "Assigning {} to {}.", tarea->name, ch->name );
}

/*
 * A complicated to use command as it currently exists.		-Thoric
 * Once area->author and area->name are cleaned up... it will be easier
 */
CMDF( do_installarea )
{
   area_data *tarea;
   std::string arg1, arg2;
   std::filesystem::path oldfilename, buf;
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
         ch->print_fmt( "An area with filename {} already exists - choose another.\r\n", arg2 );
         return;
      }

      tarea->name = argument;

      oldfilename = tarea->filename;
      tarea->filename = arg2;

      /*
       * Fold area with install flag -- auto-removes prototype flags 
       */
      tarea->flags.reset( AFLAG_PROTOTYPE );
      ch->print_fmt( "Saving and installing {}...\r\n", tarea->filename );
      tarea->install_date = current_time;
      tarea->fold( tarea->filename, true );

      /*
       * Fix up author if online 
       */
      for( auto* author : pclist )
      {
         if( author->pcdata->area == tarea )
         {
            /*
             * remove area from author 
             */
            author->pcdata->area = nullptr;
            /*
             * clear out author vnums  
             */
            author->pcdata->low_vnum = 0;
            author->pcdata->hi_vnum = 0;
            author->save(  );
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
      buf = std::format( "{}{}", BUILD_DIR, oldfilename.string() );
      std::filesystem::remove( buf );
      buf = std::format( "{}{}.bak", BUILD_DIR, oldfilename.string() );
      std::filesystem::remove( buf );
      ch->print( "Done.\r\n" );
      return;
   }
   ch->print_fmt( "No area with filename {} exists in the building directory.\r\n", arg1 );
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

   ch->print_fmt( "\r\n&wName:     &W{}\r\n&wFilename: &W{:<20}  &wPrototype: &W{}\r\n&wAuthor:   &W{}\r\n",
               tarea->name, tarea->filename, tarea->flags.test( AFLAG_PROTOTYPE ) ? "yes" : "no", tarea->author );
   if( !tarea->credits.empty() )
      ch->print_fmt( "&wAdditional Credits: : &W{}\r\n", tarea->credits );
   ch->print_fmt( "&wCreated on   : &W{}\r\n", c_time( tarea->creation_date, ch->pcdata->timezone_name ) );
   ch->print_fmt( "&wInstalled on : &W{}\r\n", c_time( tarea->install_date, ch->pcdata->timezone_name ) );
   ch->print_fmt( "&wLast reset on: &W{}\r\n", c_time( tarea->last_resettime, ch->pcdata->timezone_name ) );
   ch->print_fmt( "&wVersion: &W{:<3} &wAge: &W{:<3}  &wCurrent number of players: &W{:<3}\r\n", tarea->version, tarea->age, tarea->nplayer );
   ch->print_fmt( "&wlow_vnum: &W{:5}    &whi_vnum: &W{:5}\r\n", tarea->low_vnum, tarea->hi_vnum );
   ch->print_fmt( "&wSoft range: &W{} - {}    &wHard range: &W{} - {}\r\n", tarea->low_soft_range, tarea->hi_soft_range, tarea->low_hard_range, tarea->hi_hard_range );
   ch->print_fmt( "&wArea flags: &W{}\r\n", bitset_string( tarea->flags, area_flags ) );

   ch->print( "&wTreasure Settings:\r\n" );
   ch->print_fmt( "&wNothing: &W{:<3} &wGold:   &W{:<3} &wItem: &W{:<3} &wGem:   &W{:<3}\r\n", tarea->tg_nothing, tarea->tg_gold, tarea->tg_item, tarea->tg_gem );
   ch->print_fmt( "&wScroll:  &W{:<3} &wPotion: &W{:<3} &wWand: &W{:<3} &wArmor: &W{:<3}\r\n", tarea->tg_scroll, tarea->tg_potion, tarea->tg_wand, tarea->tg_armor );

   if( tarea->continent )
      ch->print_fmt( "&wContinent or Plane: &W{}\r\n", tarea->continent->name );
   else
      ch->print( "&wContinent or Plane: &W<NOT SET>\r\n" );

   ch->print_fmt( "&wCoordinates: &W{} {}\r\n", tarea->map_x, tarea->map_y );
   ch->print_fmt( "&wWeather: X Coord: &W{:<3}  &w Y Coord: &W{:<3}\r\n", tarea->weatherx, tarea->weathery );

   ch->print_fmt( "&wResetmsg: &W{}\r\n", !tarea->resetmsg.empty() ? tarea->resetmsg : "(default)" ); /* Rennard */
   ch->print_fmt( "&wReset frequency: &W{} &wminutes.\r\n", tarea->reset_frequency ? tarea->reset_frequency : 15 );
}

/* check other areas for a conflict while ignoring the current area */
bool check_for_area_conflicts( const area_data * carea, int lo, int hi )
{
   for( const auto* area: arealist )
   {
      if( area != carea && check_area_conflict( area, lo, hi ) )
         return true;
   }
   return false;
}

CMDF( do_aset )
{
   area_data *tarea;
   std::string arg1, arg2, arg3;
   int vnum, value;

   ch->set_color( AT_IMMORT );

   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );
   vnum = std::stoi( argument );

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

      tarea->name = argument;
      ch->print( "Done.\r\n" );
      return;
   }

   if( !str_cmp( arg2, "filename" ) )
   {
      if( !is_valid_filename( ch, "", argument ) )
         return;

      std::filesystem::path filename = tarea->filename;
      tarea->filename = argument;
      std::filesystem::rename( filename, tarea->filename );
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
         ch->print_fmt( "Invalid area continent: {} does not exist.\r\n", arg2 );
      }
      else
      {
         tarea->continent = continent;
         ch->print_fmt( "Area continent set to {}.\r\n", arg2 );
      }
      return;
   }

   if( !str_cmp( arg2, "low_vnum" ) )
   {
      if( check_for_area_conflicts( tarea, tarea->low_vnum, vnum ) )
      {
         ch->print_fmt( "Setting {} for low_vnum would conflict with another area.\r\n", vnum );
         return;
      }
      if( tarea->hi_vnum < vnum )
      {
         ch->print_fmt( "Vnum {} exceeds the hi_vnum of {} for this area.\r\n", vnum, tarea->hi_vnum );
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
         ch->print_fmt( "Setting {} for hi_vnum would conflict with another area.\r\n", vnum );
         return;
      }
      if( tarea->low_vnum > vnum )
      {
         ch->print_fmt( "Cannot set {} for hi_vnum smaller than the low_vnum of {}.\r\n", vnum, tarea->low_vnum );
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

      x = std::stoi( arg3 );
      y = std::stoi( argument );

      if( !is_valid_x( x ) )
      {
         ch->print_fmt( "Valid X coordinates are from 0 to {}.\r\n", MAX_X - 1 );
         return;
      }

      if( !is_valid_y( y ) )
      {
         ch->print_fmt( "Valid Y coordinates are from 0 to {}.\r\n", MAX_Y - 1 );
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
      tarea->author = argument;
      ch->print( "Done.\r\n" );
      web_arealist(  );
      return;
   }

   if( !str_cmp( arg2, "credits" ) )
   {
      tarea->credits = argument;
      ch->print( "Done.\r\n" );
      web_arealist(  );
      return;
   }

   if( !str_cmp( arg2, "resetmsg" ) )
   {
      if( str_cmp( argument, "clear" ) )
         tarea->resetmsg = argument;
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
            ch->print_fmt( "Unknown flag: {}\r\n", arg3 );
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
      tarea->tg_nothing = std::stoi( argument );
      ch->print_fmt( "Area chance to generate nothing set to {}%\r\n", tarea->tg_nothing );

      return;
   }

   if( !str_cmp( arg2, "gold" ) )
   {
      if( argument.empty(  ) || !is_number( argument ) )
      {
         ch->print( "Usage: aset <filename> gold <percentage>\r\n" );
         return;
      }
      tarea->tg_gold = std::stoi( argument );
      ch->print_fmt( "Area chance to generate gold set to {}%\r\n", tarea->tg_gold );

      return;
   }

   if( !str_cmp( arg2, "item" ) )
   {
      if( argument.empty(  ) || !is_number( argument ) )
      {
         ch->print( "Usage: aset <filename> item <percentage>\r\n" );
         return;
      }
      tarea->tg_item = std::stoi( argument );
      ch->print_fmt( "Area chance to generate item set to {}%\r\n", tarea->tg_item );

      return;
   }

   if( !str_cmp( arg2, "gem" ) )
   {
      if( argument.empty(  ) || !is_number( argument ) )
      {
         ch->print( "Usage: aset <filename> gem <percentage>\r\n" );
         return;
      }
      tarea->tg_gem = std::stoi( argument );
      ch->print_fmt( "Area chance to generate gem set to {}%\r\n", tarea->tg_gem );

      return;
   }

   if( !str_cmp( arg2, "scroll" ) )
   {
      if( argument.empty(  ) || !is_number( argument ) )
      {
         ch->print( "Usage: aset <filename> scroll <percentage>\r\n" );
         return;
      }
      tarea->tg_scroll = std::stoi( argument );
      ch->print_fmt( "Area chance to generate scroll set to {}%\r\n", tarea->tg_scroll );

      return;
   }

   if( !str_cmp( arg2, "potion" ) )
   {
      if( argument.empty(  ) || !is_number( argument ) )
      {
         ch->print( "Usage: aset <filename> potion <percentage>\r\n" );
         return;
      }
      tarea->tg_potion = std::stoi( argument );
      ch->print_fmt( "Area chance to generate potion set to {}%\r\n", tarea->tg_potion );

      return;
   }

   if( !str_cmp( arg2, "wand" ) )
   {
      if( argument.empty(  ) || !is_number( argument ) )
      {
         ch->print( "Usage: aset <filename> wand <percentage>\r\n" );
         return;
      }
      tarea->tg_wand = std::stoi( argument );
      ch->print_fmt( "Area chance to generate wand set to {}%\r\n", tarea->tg_wand );

      return;
   }

   if( !str_cmp( arg2, "armor" ) )
   {
      if( argument.empty(  ) || !is_number( argument ) )
      {
         ch->print( "Usage: aset <filename> armor <percentage>\r\n" );
         return;
      }
      tarea->tg_armor = std::stoi( argument );
      ch->print_fmt( "Area chance to generate armor set to {}%\r\n", tarea->tg_armor );

      return;
   }

   do_aset( ch, "" );
}

/* Displays zone list. Will show proto, non-proto, or all. */
void show_vnums( char_data * ch, short proto )
{
   int count = 0;

   ch->pager_fmt( "&W{:<15.15} {:<40.40} {:5.5}\r\n\r\n", "Filename", "Area Name", "Vnums" );
   for( auto* area : area_vsort )
   {
      if( proto == 0 && !area->flags.test( AFLAG_PROTOTYPE ) )
      {
         ch->pager_fmt( "&c{:<15.15} {:<40.40} V: {:5} - {:<5}\r\n", area->filename, area->name, area->low_vnum, area->hi_vnum );
         ++count;
      }
      else if( proto == 1 && area->flags.test( AFLAG_PROTOTYPE ) )
      {
         ch->pager_fmt( "&c{:<15.15} {:<40.40} V: {:5} - {:<5} &W[Proto]\r\n", area->filename, area->name, area->low_vnum, area->hi_vnum );
         ++count;
      }
      else if( proto == 2 )
      {
         ch->pager_fmt( "&c{:<15.15} {:<40.40} V: {:5} - {:<5} {}\r\n",
                     area->filename, area->name, area->low_vnum, area->hi_vnum, area->flags.test( AFLAG_PROTOTYPE ) ? "&W[Proto]" : "" );
         ++count;
      }
   }
   ch->pager_fmt( "&CAreas listed: {}\r\n", count );
   ch->pager_fmt( "Maximum allowed vnum is currently {}.&D\r\n", sysdata->maxvnum );
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
   std::string arg1;
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

   int low_range = std::stoi( arg1 ), high_range = std::stoi( argument );

   if( low_range < 1 || low_range > sysdata->maxvnum )
   {
      ch->print_fmt( "Invalid low range. Valid vnums are from 1 to {}.\r\n", sysdata->maxvnum );
      return;
   }

   if( high_range < 1 || high_range > sysdata->maxvnum )
   {
      ch->print_fmt( "Invalid high range. Valid vnums are from 1 to {}.\r\n", sysdata->maxvnum );
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
   for( auto* area : arealist )
   {
      area_conflict = check_area_conflict( area, low_range, high_range );

      if( area_conflict )
      {
         lohi[x++] = area->low_vnum;
         lohi[x++] = area->hi_vnum;
         ch->print_fmt( "&RArea Conflict: &g{:<20.20} &wRange: &g{} - {}\r\n", area->filename, area->low_vnum, area->hi_vnum );
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
         ch->print_fmt( "&wOpen Range: &g{} - {}\r\n", y, z );
   }
}

/* Check to make sure range of vnums is free - Scryn 2/27/96 */
CMDF( do_check_vnums )
{
   std::string arg;

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

   int low_range = std::stoi( arg ), high_range = std::stoi( argument );

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
   for( auto* area : arealist )
   {
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
         ch->print_fmt( "Conflict:{:<15}| ", ( !area->filename.empty() ? area->filename : "(invalid)" ) );
         ch->print_fmt( "Vnums: {:5} - {:<5}\r\n", area->low_vnum, area->hi_vnum );
      }
   }
}

/* Shogar's code to hunt for exits/entrances to/from a zone, very nice - Samson 12-30-00 */
CMDF( do_aexit )
{
   area_data *tarea;

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

   int trange = tarea->hi_vnum, lrange = tarea->low_vnum;
   for( auto* room : tarea->rooms )
   {
      if( room->flags.test( ROOM_TELEPORT ) && ( room->tele_vnum < lrange || room->tele_vnum > trange ) )
      {
         ch->pager_fmt( "From: {:<20.20} Room: {:5} To: Room: {:5} (Teleport)\r\n", tarea->filename, room->vnum, room->tele_vnum );
      }

      for( int i = 0; i < MAX_DIR + 2; ++i ) /* MAX_DIR+2 added to include ? exits.  Dwip 5/7/02 */
      {
         exit_data *pexit;
         if( !( pexit = room->get_exit( i ) ) )
            continue;

         if( IS_EXIT_FLAG( pexit, EX_OVERLAND ) )
         {
            ch->pager_fmt( "To: Overland {:4}X {:4}Y From: {:20.20} Room: {:5} ({})\r\n", pexit->map_x, pexit->map_y, tarea->filename, room->vnum, dir_name[i] );
            continue;
         }
         if( pexit->to_room->area != tarea )
         {
            ch->pager_fmt( "To: {:<20.20} Room: {:5} From: {:<20.20} Room: {:5} ({})\r\n", pexit->to_room->area->filename, pexit->vnum, tarea->filename, room->vnum, dir_name[i] );
         }
      }
   }

   for( auto* area : arealist )
   {
      if( tarea == area )
         continue;

      trange = area->hi_vnum;
      lrange = area->low_vnum;
      for( auto* room2 : area->rooms )
      {
         if( room2->flags.test( ROOM_TELEPORT ) )
         {
            if( room2->tele_vnum >= tarea->low_vnum && room2->tele_vnum <= tarea->hi_vnum )
               ch->pager_fmt( "From: {:<20.20} Room: {:5} To: {:<20.20} Room: {:5} (Teleport)\r\n", area->filename, room2->vnum, tarea->filename, room2->tele_vnum );
         }

         for( int i = 0; i < MAX_DIR + 2; ++i )
         {
            exit_data *pexit;
            if( !( pexit = room2->get_exit( i ) ) )
               continue;

            if( IS_EXIT_FLAG( pexit, EX_OVERLAND ) )
               continue;

            if( pexit->to_room->area == tarea )
            {
               ch->pager_fmt( "From: {:<20.20} Room: {:5} To: {:<20.20} Room: {:5} ({})\r\n", area->filename, room2->vnum, pexit->to_room->area->filename, pexit->vnum, dir_name[i] );
            }
         }
      }
   }

   for( auto* continent : continent_list )
   {
      for( auto* mexit : continent->exits )
      {
         if( mexit->vnum >= tarea->low_vnum && mexit->vnum <= tarea->hi_vnum )
            ch->pager_fmt( "From Continent {}: {:4}X {:4}Y To: Room: {:5}\r\n", continent->name, mexit->herex, mexit->herey, mexit->vnum );
      }
   }
}

/*
 * Revised version of do_areas, originally written by Fireblade 4/27/97,
 * Rewritten by Dwip to fix horrid argument bugs 4/14/02.  Happy 5 year
 * anniversary, do_areas! :)
 */
CMDF( do_areas )
{
   int lower_bound = 0, upper_bound = MAX_LEVEL + 1, num_args = 0, swap;
   /*
    * 0-2 = x arguments, 3 = old style 
    */
   std::string arg;

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

         upper_bound = std::stoi( arg );
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
            upper_bound = std::stoi( arg );
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

   std::string print_string = "{:<12} | {:<36} | {:4} - {:<4} | {:3} - {:<3} \r\n";
   for( auto* area : area_nsort )
   {
      // Hidden areas should not display on the list.
      if( area->flags.test( AFLAG_HIDDEN ) && !ch->is_immortal() )
         continue;

      switch ( num_args )
      {
         case 0:
            ch->pager( std::vformat( print_string, std::make_format_args( area->author, area->name, area->low_soft_range, area->hi_soft_range, area->low_hard_range, area->hi_hard_range ) ) );
            break;

         case 1:
            if( area->hi_soft_range >= upper_bound && area->low_soft_range <= upper_bound )
            {
               ch->pager( std::vformat( print_string, std::make_format_args( area->author, area->name, area->low_soft_range, area->hi_soft_range, area->low_hard_range, area->hi_hard_range ) ) );
            }
            break;

         case 2:
            if( area->hi_soft_range >= upper_bound && area->low_soft_range <= lower_bound )
            {
               ch->pager( std::vformat( print_string, std::make_format_args( area->author, area->name, area->low_soft_range, area->hi_soft_range, area->low_hard_range, area->hi_hard_range ) ) );
            }
            break;

         case 3:
            ch->pager( std::vformat( print_string, std::make_format_args( area->author, area->name, area->low_soft_range, area->hi_soft_range, area->low_hard_range, area->hi_hard_range ) ) );
            break;

         default:
            ch->pager( std::vformat( print_string, std::make_format_args( area->author, area->name, area->low_soft_range, area->hi_soft_range, area->low_hard_range, area->hi_hard_range ) ) );
            break;
      }
   }
}
