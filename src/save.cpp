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
 *                   Character Saving and Loading Module                    *
 ****************************************************************************/

#include <filesystem>
#include <fstream>
#include "mud.h"
#include "area.h"
#include "bits.h"
#include "boards.h"
#include "clans.h"
#include "deity.h"
#include "descriptor.h"
#include "mobindex.h"
#include "objindex.h"
#include "overland.h"
#include "raceclass.h"
#include "realms.h"
#include "roomindex.h"
#include "smaugaffect.h"

/*
 * Externals
 */
void reset_colors( char_data * );
board_data *get_board( char_data *, std::string_view );
void fwrite_morph_data( char_data *, std::ofstream & );
void fread_morph_data( char_data *, std::ifstream & );
void fread_variable( char_data *, std::ifstream & );
void fwrite_variables( const char_data *, std::ofstream & );
std::string default_fprompt( char_data * );
std::string default_prompt( char_data * );
void bind_follower( char_data *, char_data *, int, int );
void assign_area( char_data * );
void fwrite_afk_affect( std::ofstream &, affect_data * );
affect_data *fread_afk_affect( std::ifstream & );
bool is_valid_wear_loc( char_data *, int );
void restore_map_buffer( char_data * );
std::string convert_old_timezone( int );

class mud_channel;
mud_channel *find_channel( std::string_view );

/*
 * Increment with every major format change.
 */
constexpr int SAVEVERSION = 25;
// Updated to version 4 after addition of alias code. - Samson 3-23-98
// Updated to version 5 after installation of color code. - Samson
// Updated to version 6 for rare item tracking support. - Samson
// DOTD pfiles saved as version 7.
// Updated to version 8 for text based data saving. - Samson
// Updated to version 9 for new exp tables. - Samson 4-30-99
// Updated to version 10 after weapon code updates. - Samson 1-15-00
// Updated to version 11 for mv + 50 boost. - Samson 4-25-00
// Updated to version 12 for mana recalcs. - Samson 1-19-01
// Updated to version 13 to force activation of MSP/MXP for old players. - Samson 8-21-01
// Updated to version 14 to force activation of MXP Prompt line. - Samson 2-27-02
// Updated to version 15 for new exp system. - Samson 12-15-02
// Updated to 16 to award stat gains for old characters. - Samson 12-16-02
// Updated to 17 for yet another try at an xp system that doesn't suck. - Samson 12-22-02
// Updated to 18 for the reorganized format. - Samson 5-16-04
// 19 skipped. Nobody remembers why.
// Updated to 20: Starting version for official support of AFKMud 2.0 pfiles.
// Updated to 21 because Samson was stupid and acted hastily before finalizing the std::bitset conversions. 7-8-04
// Updated to 22 for sha256 password conversion.
// Updated to 23 - Site data in the save requires the tilde now which pfiles won't have yet.
// Updated to 24 - Map coordinates now only store the X and Y. Which map the player is on is determined by the room they're in.
// Updated to 25 to change timezone handling. -- Samson 6/28/2026.

/*
 * Array to keep track of equipment temporarily. - Thoric
 */
obj_data *save_equipment[MAX_WEAR][MAX_LAYERS];
obj_data *mob_save_equipment[MAX_WEAR][MAX_LAYERS];
char_data *quitting_char, *loading_char, *saving_char;

int file_ver = SAVEVERSION;

/*
 * Array of containers read for proper re-nesting of objects.
 */
static obj_data *rgObjNest[MAX_NEST];

/*
 * Un-equip character before saving to ensure proper	-Thoric
 * stats are saved in case of changes to or removal of EQ
 */
void char_data::de_equip(  )
{
   for( int x = 0; x < MAX_WEAR; ++x )
   {
      for( int y = 0; y < MAX_LAYERS; ++y )
      {
         if( isnpc(  ) )
            mob_save_equipment[x][y] = nullptr;
         else
            save_equipment[x][y] = nullptr;
      }
   }

   for( auto* obj : carrying )
   {
      if( obj->wear_loc > -1 && obj->wear_loc < MAX_WEAR )
      {
         if( char_ego(  ) >= obj->ego )
         {
            bool found_slot = false;
            for( int x = 0; x < MAX_LAYERS; ++x )
            {
               if( isnpc(  ) )
               {
                  if( !mob_save_equipment[obj->wear_loc][x] )
                  {
                     mob_save_equipment[obj->wear_loc][x] = obj;
                     found_slot = true;
                     break;
                  }
               }
               else
               {
                  if( !save_equipment[obj->wear_loc][x] )
                  {
                     save_equipment[obj->wear_loc][x] = obj;
                     found_slot = true;
                     break;
                  }
               }
            }

            if( !found_slot )
            {
               bug( "{}: {} had on more than {} layers of clothing in one location ({}): {}", __func__, name, MAX_LAYERS, obj->wear_loc, obj->name );
            }
         }
         unequip( obj );
      }
   }
}

/*
 * Re-equip character - Thoric
 */
void char_data::re_equip(  )
{
   for( int x = 0; x < MAX_WEAR; ++x )
   {
      for( int y = 0; y < MAX_LAYERS; ++y )
      {
         if( isnpc(  ) )
         {
            if( mob_save_equipment[x][y] != nullptr )
            {
               if( quitting_char != this )
                  equip( mob_save_equipment[x][y], x );
               mob_save_equipment[x][y] = nullptr;
            }
            else
               break;
         }
         else
         {
            if( save_equipment[x][y] != nullptr )
            {
               if( quitting_char != this )
                  equip( save_equipment[x][y], x );
               save_equipment[x][y] = nullptr;
            }
            else
               break;
         }
      }
   }
}

/*
 * Write the char.
 */
void fwrite_char( char_data * ch, std::ofstream & stream )
{
   if( ch->isnpc(  ) )
   {
      bug( "{}: NPC save called!", __func__ );
      return;
   }

   stream << "#PLAYER\n";
   stream << std::format( "Version      {}\n", SAVEVERSION );
   stream << std::format( "Name         {}~\n", ch->name );
   stream << std::format( "Password     {}~\n", ch->pcdata->pwd );
   stream << std::format( "Site         {}~\n", ch->pcdata->lasthost );
   stream << std::format( "Status       {} {} {} {} {} {} {}\n", ch->level, ch->gold, ch->exp, ch->height, ch->weight, ch->spellfail, ch->mental_state );
   if( !ch->pcdata->email.empty(  ) )  /* Samson 4-19-98 */
      stream << std::format( "Email        {}~\n", ch->pcdata->email );
   if( !ch->pcdata->homepage.empty(  ) )
      stream << std::format( "Homepage     {}~\n", ch->pcdata->homepage );
   if( !ch->pcdata->bio.empty() )
      stream << std::format( "Bio          {}~\n", strip_cr( ch->pcdata->bio ) );
   if( !ch->chardesc.empty() )
      stream << std::format( "Description  {}~\n", strip_cr( ch->chardesc ) );
   if( !ch->pcdata->timezone_name.empty() )
      stream << std::format( "Timezone     {}~\n", ch->pcdata->timezone_name );
   if( !ch->pcdata->map_buffer.empty() )
      stream << std::format( "OLCMapBuffer {}~\n", ch->pcdata->map_buffer );
   stream << std::format( "Sex          {}~\n", npc_sex[ch->sex] );
   stream << std::format( "Race         {}~\n", npc_race[ch->race] );
   stream << std::format( "Class        {}~\n", npc_class[ch->Class] );
   if( !ch->pcdata->title.empty() )
      stream << std::format( "Title        {}~\n", ch->pcdata->title );
   if( !ch->pcdata->rank.empty() )
      stream << std::format( "Rank         {}~\n", ch->pcdata->rank );
   if( !ch->pcdata->bestowments.empty(  ) )
      stream << std::format( "Bestowments  {}~\n", ch->pcdata->bestowments );
   if( !ch->pcdata->authed_by.empty(  ) )
      stream << std::format( "AuthedBy     {}~\n", ch->pcdata->authed_by );
   if( !ch->pcdata->prompt.empty() )
      stream << std::format( "Prompt       {}~\n", ch->pcdata->prompt );
   if( !ch->pcdata->fprompt.empty() )
      stream << std::format( "FPrompt      {}~\n", ch->pcdata->fprompt );
   if( !ch->pcdata->deity_name.empty(  ) )
      stream << std::format( "Deity        {}~\n", ch->pcdata->deity_name );
   if( !ch->pcdata->clan_name.empty(  ) )
      stream << std::format( "Clan         {}~\n", ch->pcdata->clan_name );
   if( ch->has_pcflags(  ) )
      stream << std::format( "PCFlags      {}~\n", bitset_string( ch->get_pcflags(  ), pc_flags ) );
   if( !ch->pcdata->chan_listen.empty(  ) )
      stream << std::format( "Channels     {}~\n", ch->pcdata->chan_listen );
   if( ch->has_langs(  ) )
      stream << std::format( "Speaks       {}~\n", bitset_string( ch->get_langs(  ), lang_names ) );
   if( ch->speaking )
      stream << std::format( "Speaking     {}~\n", lang_names[ch->speaking] );
   if( ch->pcdata->release_date > std::chrono::system_clock::time_point{} )
   {
      auto release_date = std::chrono::system_clock::to_time_t( ch->pcdata->release_date );
      stream << std::format( "Helled       {} {}~\n", release_date, ch->pcdata->helled_by );
   }
   stream << std::format( "Status2      {} {} {} {} {} {} {} {}\n", ch->style, ch->pcdata->practice, ch->alignment, ch->pcdata->favor, ch->hitroll, ch->damroll, ch->armor, ch->wimpy );
   stream << std::format( "Configs      {} {}\n", ch->pcdata->pagerlen, ch->wait );

   /*
    * MOTD times - Samson 12-31-00 
    */
   auto motd = std::chrono::system_clock::to_time_t( ch->pcdata->motd );
   auto imotd = std::chrono::system_clock::to_time_t( ch->pcdata->imotd );

   stream << std::format( "Motd         {} {}\n", motd, imotd );

   auto elapsed_since_logon = std::chrono::duration_cast<std::chrono::hours>( current_time - ch->pcdata->logon );
   auto total_played = ch->pcdata->played + elapsed_since_logon;

   stream << std::format( "Age          {} {} {} {} {}\n",
            ch->pcdata->age_bonus, ch->pcdata->day, ch->pcdata->month, ch->pcdata->year, total_played.count() );

   stream << std::format( "HpManaMove   {} {} {} {} {} {}\n", ch->hit, ch->max_hit, ch->mana, ch->max_mana, ch->move, ch->max_move );
   stream << std::format( "Regens       {} {} {}\n", ch->hit_regen, ch->mana_regen, ch->move_regen );

   short pos = ch->position;
   if( pos > POS_SITTING && pos < POS_STANDING )
      pos = POS_STANDING;
   stream << std::format( "Position     {}~\n", npc_position[pos] );

   // What room vnum are we in? Need this to be able to handle continent assignment on load.
   stream << std::format( "Room         {}\n", ( ch->in_room == get_room_index( ROOM_VNUM_LIMBO ) && ch->was_in_room ) ? ch->was_in_room->vnum : ch->in_room->vnum );

   /*
    * Overland Map - Samson 7-31-99 
    */
   stream << std::format( "Coordinates  {} {}\n", ch->map_x, ch->map_y );

   stream << std::format( "SavingThrows {} {} {} {} {}\n", ch->saving_poison_death, ch->saving_wand, ch->saving_para_petri, ch->saving_breath, ch->saving_spell_staff );
   stream << std::format( "RentData     {} {} 0 0 {}\n", ch->pcdata->balance, ch->pcdata->daysidle, ch->pcdata->camp );

   /*
    * Recall code update to recall to last inn rented at - Samson 12-20-00 
    */
   stream << std::format( "RentRooms    {}\n", ch->pcdata->one );

   if( ch->has_bparts( )  )
      stream << std::format( "BodyParts    {}~\n", bitset_string( ch->get_bparts(  ), part_flags ) );
   if( ch->has_aflags(  ) )
      stream << std::format( "AffectFlags  {}~\n", bitset_string( ch->get_aflags(  ), aff_flags ) );
   if( ch->has_noaflags(  ) )
      stream << std::format( "NoAffectedBy {}~\n", bitset_string( ch->get_noaflags(  ), aff_flags ) );
   if( ch->has_resists(  ) )
      stream << std::format( "Resistant    {}~\n", bitset_string( ch->get_resists(  ), ris_flags ) );
   if( ch->has_noresists(  ) )
      stream << std::format( "Nores        {}~\n", bitset_string( ch->get_noresists(  ), ris_flags ) );
   if( ch->has_susceps(  ) )
      stream << std::format( "Susceptible  {}~\n", bitset_string( ch->get_susceps(  ), ris_flags ) );
   if( ch->has_nosusceps(  ) )
      stream << std::format( "Nosusc       {}~\n", bitset_string( ch->get_nosusceps(  ), ris_flags ) );
   if( ch->has_immunes(  ) )
      stream << std::format( "Immune       {}~\n", bitset_string( ch->get_immunes(  ), ris_flags ) );
   if( ch->has_noimmunes(  ) )
      stream << std::format( "Noimm        {}~\n", bitset_string( ch->get_noimmunes(  ), ris_flags ) );
   if( ch->has_absorbs(  ) )
      stream << std::format( "Absorb       {}~\n", bitset_string( ch->get_absorbs(  ), ris_flags ) );

   stream << std::format( "KillInfo     {} {} {} {} {}\n", ch->pcdata->pkills, ch->pcdata->pdeaths, ch->pcdata->mkills, ch->pcdata->mdeaths, ch->pcdata->illegal_pk );

   if( ch->get_timer( TIMER_PKILLED ) && ( ch->get_timer( TIMER_PKILLED ) > 0 ) )
      stream << std::format( "PTimer       {}\n", ch->get_timer( TIMER_PKILLED ) );

   stream << std::format( "AttrPerm     {} {} {} {} {} {} {}\n", ch->perm_str, ch->perm_int, ch->perm_wis, ch->perm_dex, ch->perm_con, ch->perm_cha, ch->perm_lck );

   stream << std::format( "AttrMod      {} {} {} {} {} {} {}\n", ch->mod_str, ch->mod_int, ch->mod_wis, ch->mod_dex, ch->mod_con, ch->mod_cha, ch->mod_lck );

   stream << std::format( "Condition    {} {} {}\n", ch->pcdata->condition[0], ch->pcdata->condition[1], ch->pcdata->condition[2] );

   if( ch->is_immortal(  ) )
   {
      if( ch->pcdata->realm && !ch->pcdata->realm_name.empty() )
         stream << std::format( "ImmRealm     {}~\n", ch->pcdata->realm_name );
      if( !ch->pcdata->bamfin.empty() )
         stream << std::format( "Bamfin       {}~\n", ch->pcdata->bamfin );
      if( !ch->pcdata->bamfout.empty() )
         stream << std::format( "Bamfout      {}~\n", ch->pcdata->bamfout );

      auto restore_time = std::chrono::system_clock::to_time_t( ch->pcdata->restore_time );
      stream << std::format( "ImmData      {} {} {} {} {}\n",
               ch->trust, restore_time, ch->pcdata->wizinvis, ch->pcdata->low_vnum, ch->pcdata->hi_vnum );
   }

   for( int sn = 1; sn < num_skills; ++sn )
   {
      if( !skill_table[sn]->name.empty() && ch->pcdata->learned[sn] > 0 )
      {
         switch ( skill_table[sn]->type )
         {
            default:
               stream << std::format( "Skill        {} '{}'\n", ch->pcdata->learned[sn], skill_table[sn]->name );
               break;

            case SKILL_SPELL:
               stream << std::format( "Spell        {} '{}'\n", ch->pcdata->learned[sn], skill_table[sn]->name );
               break;

            case SKILL_COMBAT:
               stream << std::format( "Combat       {} '{}'\n", ch->pcdata->learned[sn], skill_table[sn]->name );
               break;

            case SKILL_TONGUE:
               stream << std::format( "Tongue       {} '{}'\n", ch->pcdata->learned[sn], skill_table[sn]->name );
               break;

            case SKILL_RACIAL:
               stream << std::format( "Ability      {} '{}'\n", ch->pcdata->learned[sn], skill_table[sn]->name );
               break;

            case SKILL_LORE:
               stream << std::format( "Lore         {} '{}'\n", ch->pcdata->learned[sn], skill_table[sn]->name );
         }
      }
   }

   for( auto* af : ch->affects )
   {
      skill_type *skill = nullptr;

      if( af->type >= 0 && !( skill = get_skilltype( af->type ) ) )
         continue;

      if( af->type >= 0 && af->type < TYPE_PERSONAL )
         stream << std::format( "AffectData   '{}' {:3} {:3} {:3} {}\n", skill->name, af->duration, af->modifier, af->location, af->bit );
      else
         fwrite_afk_affect( stream, af );
   }

   /*
    * Save color values - Samson 9-29-98 
    */
   stream << std::format( "MaxColors    {}\n", MAX_COLORS );
   stream << "Colors      ";
   for( int x = 0; x < MAX_COLORS; ++x )
      stream << std::format( " {}", ch->pcdata->colors[x] );
   stream << "\n";

   /*
    * Save recall beacons - Samson 2-7-99 
    */
   stream << std::format( "MaxBeacons   {}\n", MAX_BEACONS );
   stream << "Beacons     ";
   for( int x = 0; x < MAX_BEACONS; ++x )
      stream << std::format( " {}", ch->pcdata->beacon[x] );
   stream << "\n";

   std::map<std::string, std::string >::iterator pal;
   for( pal = ch->pcdata->alias_map.begin(  ); pal != ch->pcdata->alias_map.end(  ); ++pal )
   {
      if( pal->second.empty(  ) )
         continue;
      stream << std::format( "Alias        {}~ {}~\n", pal->first, pal->second );
   }

   if( !ch->pcdata->boarddata.empty(  ) )
   {
      for( auto it = ch->pcdata->boarddata.begin(  ); it != ch->pcdata->boarddata.end(  ); )
      {
         board_chardata *chbd = *it;
         ++it;

         /*
          * Ugh.. is it worth saving that extra board_chardata field on pcdata? 
          * No, Xorith, it wasn't. So I changed it - Samson 10-15-03 
          */
         if( chbd->board_name.empty(  ) )
         {
            ch->pcdata->boarddata.remove( chbd );
            deleteptr( chbd );
            continue;
         }
         auto last_read = std::chrono::system_clock::to_time_t( chbd->last_read );
         stream << std::format( "Board_Data   {}~ {} {}\n", chbd->board_name, last_read, chbd->alert );
      }
   }

   /*
    * Save the list of zones PC has visited - Samson 7-11-00
    */
   for( auto& zn : ch->pcdata->zone )
      stream << std::format( "Zone         {}~\n", zn );

   for( const auto& name : ch->pcdata->ignore )
      stream << std::format( "Ignored      {}~\n", name );

   if( !ch->pcdata->qbits.empty(  ) )
   {
      std::map<int, std::string>::iterator bit;

      for( bit = ch->pcdata->qbits.begin(  ); bit != ch->pcdata->qbits.end(  ); ++bit )
         stream << std::format( "Qbit         {} {}~\n", bit->first, bit->second );
   }
   stream << "End\n\n";
}

/*
 * Write an object list, with contents.
 */
void fwrite_obj( char_data * ch, std::list<obj_data *> source, clan_data * clan, std::ofstream & stream, int iNest, bool hotboot )
{
   if( iNest >= MAX_NEST )
   {
      bug( "{}: iNest hit MAX_NEST {}", __func__, iNest );
      return;
   }

   for( auto* obj : source )
   {
      if( !obj )
      {
         bug( "{}: nullptr obj", __func__ );
         continue;
      }

      /*
       * Castrate storage characters.
       * Catch deleted objects         -Thoric
       * Do NOT save prototype items!  -Thoric
       * But bypass this if it's a hotboot because you want it to stay on the ground. - Samson
       */
      if( !hotboot )
      {
         if( ( obj->item_type == ITEM_KEY && !obj->extra_flags.test( ITEM_CLANOBJECT ) ) || obj->extracted(  ) || obj->extra_flags.test( ITEM_PROTOTYPE ) || obj->ego == -1 )
            return;
      }

      /*
       * Catch rare items going into auction houses - Samson 6-23-99 
       */
      if( ch )
      {
         if( ( ch->has_actflag( ACT_AUCTION ) ) && obj->ego >= sysdata->minego )
            return;
      }

      /*
       * Catch rare items going into clan storerooms - Samson 2-3-01 
       */
      if( clan && ch && ch->in_room )
      {
         if( obj->ego >= sysdata->minego && ch->in_room->vnum == clan->storeroom )
            return;
      }

      /*
       * DO NOT save corpses lying on the ground as a hotboot item, they already saved elsewhere! - Samson 
       */
      if( hotboot && obj->item_type == ITEM_CORPSE_PC )
         return;

      /*
       * No longer using OS_CORPSE during object writes - Samson 
       */
      stream << "#OBJECT\n";
      stream << std::format( "Version      {}\n", SAVEVERSION );
      if( iNest )
         stream << std::format( "Nest         {}\n", iNest );
      if( obj->count > 1 )
         stream << std::format( "Count        {}\n", obj->count );
      if( !obj->name.empty() && !obj->pIndexData->name.empty() && str_cmp( obj->name, obj->pIndexData->name ) )
         stream << std::format( "Name         {}~\n", obj->name );
      if( !obj->short_descr.empty() && !obj->pIndexData->short_descr.empty() && str_cmp( obj->short_descr, obj->pIndexData->short_descr ) )
         stream << std::format( "ShortDescr   {}~\n", obj->short_descr );
      if( !obj->objdesc.empty() && !obj->pIndexData->objdesc.empty() && str_cmp( obj->objdesc, obj->pIndexData->objdesc ) )
         stream << std::format( "Description  {}~\n", obj->objdesc );
      if( !obj->action_desc.empty() && !obj->pIndexData->action_desc.empty() && str_cmp( obj->action_desc, obj->pIndexData->action_desc ) )
         stream << std::format( "ActionDesc   {}~\n", obj->action_desc );
      stream << std::format( "Ovnum        {}\n", obj->pIndexData->vnum );
      stream << std::format( "Ego          {}\n", obj->ego );
      if( hotboot && obj->in_room )
      {
         stream << std::format( "Room         {}\n", obj->in_room->vnum );
         stream << std::format( "Rvnum        {}\n", obj->room_vnum );
      }
      if( obj->extra_flags != obj->pIndexData->extra_flags )
         stream << std::format( "ExtraFlags   {}~\n", bitset_string( obj->extra_flags, o_flags ) );
      if( obj->wear_flags != obj->pIndexData->wear_flags )
         stream << std::format( "WearFlags    {}~\n", bitset_string( obj->wear_flags, w_flags ) );

      short wear_loc = -1;
      if( ch )
      {
         for( short wear = 0; wear < MAX_WEAR; ++wear )
         {
            for( short x = 0; x < MAX_LAYERS; ++x )
            {
               if( ch->isnpc(  ) )
               {
                  if( obj == mob_save_equipment[wear][x] )
                  {
                     wear_loc = wear;
                     break;
                  }
                  else if( !mob_save_equipment[wear][x] )
                     break;
               }
               else
               {
                  if( obj == save_equipment[wear][x] )
                  {
                     wear_loc = wear;
                     break;
                  }
                  else if( !save_equipment[wear][x] )
                     break;
               }
            }
         }
      }
      if( wear_loc != -1 )
         stream << std::format( "WearLoc      {}\n", wear_loc );
      if( obj->item_type != obj->pIndexData->item_type )
         stream << std::format( "ItemType     {}\n", obj->item_type );
      if( obj->weight != obj->pIndexData->weight )
         stream << std::format( "Weight       {}\n", obj->weight );
      if( obj->level )
         stream << std::format( "Level        {}\n", obj->level );
      if( obj->timer )
         stream << std::format( "Timer        {}\n", obj->timer );
      if( obj->cost != obj->pIndexData->cost )
         stream << std::format( "Cost         {}\n", obj->cost );
      if( !obj->seller.empty() )
         stream << std::format( "Seller       {}~\n", obj->seller );
      if( !obj->buyer.empty() )
         stream << std::format( "Buyer        {}~\n", obj->buyer );
      if( obj->bid != 0 )
         stream << std::format( "Bid          {}\n", obj->bid );
      if( !obj->owner.empty() )
         stream << std::format( "Owner        {}~\n", obj->owner );
      stream << std::format( "Oday         {}\n", obj->day );
      stream << std::format( "Omonth       {}\n", obj->month );
      stream << std::format( "Oyear        {}\n", obj->year );
      stream << std::format( "Coordinates  {} {}\n", obj->map_x, obj->map_y );
      stream << "Values      ";
      for( short x = 0; x < MAX_OBJ_VALUE; ++x )
         stream << std::format( " {}", obj->value[x] );
      stream << "\n";
      stream << std::format( "Sockets      {} {} {}\n", !obj->socket[0].empty() ? obj->socket[0] : "None", !obj->socket[1].empty() ? obj->socket[1] : "None", obj->socket[2].empty() ? obj->socket[2] : "None" );

      switch ( obj->item_type )
      {
         default:
            break;

         case ITEM_PILL:  /* was down there with staff and wand, wrongly - Scryn */
         case ITEM_POTION:
         case ITEM_SCROLL:
            if( IS_VALID_SN( obj->value[1] ) )
               stream << std::format( "Spell 1      '{}'\n", skill_table[obj->value[1]]->name );
            if( IS_VALID_SN( obj->value[2] ) )
               stream << std::format( "Spell 2      '{}'\n", skill_table[obj->value[2]]->name );
            if( IS_VALID_SN( obj->value[3] ) )
               stream << std::format( "Spell 3      '{}'\n", skill_table[obj->value[3]]->name );
            break;

         case ITEM_STAFF:
         case ITEM_WAND:
            if( IS_VALID_SN( obj->value[3] ) )
               stream << std::format( "Spell 3      '{}'\n", skill_table[obj->value[3]]->name );
            break;

         case ITEM_SALVE:
            if( IS_VALID_SN( obj->value[4] ) )
               stream << std::format( "Spell 4      '{}'\n", skill_table[obj->value[4]]->name );
            if( IS_VALID_SN( obj->value[5] ) )
               stream << std::format( "Spell 5      '{}'\n", skill_table[obj->value[5]]->name );
            break;
      }

      for( auto* af : obj->affects )
      {
         /*
          * Save extra object affects - Thoric
          */
         if( af->type < 0 || af->type >= num_skills )
         {
            stream << std::format( "Affect       {} {} {} {} {}\n",
                     af->type, af->duration,
                     ( ( af->location == APPLY_WEAPONSPELL
                         || af->location == APPLY_WEARSPELL
                         || af->location == APPLY_REMOVESPELL
                         || af->location == APPLY_STRIPSN
                         || af->location == APPLY_RECURRINGSPELL ) && IS_VALID_SN( af->modifier ) ) ? skill_table[af->modifier]->slot : af->modifier, af->location, af->bit );
         }
         else
            stream << std::format( "AffectData   '{}' {} {} {} {}\n",
                     skill_table[af->type]->name, af->duration,
                     ( ( af->location == APPLY_WEAPONSPELL
                         || af->location == APPLY_WEARSPELL
                         || af->location == APPLY_REMOVESPELL
                         || af->location == APPLY_STRIPSN
                         || af->location == APPLY_RECURRINGSPELL ) && IS_VALID_SN( af->modifier ) ) ? skill_table[af->modifier]->slot : af->modifier, af->location, af->bit );
      }

      for( auto* desc : obj->extradesc )
      {
         if( !desc->desc.empty(  ) )
            stream << std::format( "ExtraDescr   {}~ {}~\n", desc->keyword, desc->desc );
         else
            stream << std::format( "ExtraDescr   {}~ ~\n", desc->keyword );
      }

      stream << "End\n\n";

      if( !obj->contents.empty(  ) )
         fwrite_obj( ch, obj->contents, clan, stream, iNest + 1, hotboot );
   }
}

/*
 * This will write one mobile structure pointed to be fp --Shaddai
 *   Edited by Tarl 5 May 2002 to allow pets to save items.
*/
void fwrite_mobile( char_data * mob, std::ofstream & stream, bool shopmob )
{
   if( !mob->isnpc() )
      return;

   stream << std::format( "{}", shopmob ? "#SHOP\n" : "#MOBILE\n" );
   stream << std::format( "Vnum    {}\n", mob->pIndexData->vnum );
   stream << std::format( "Version {}\n", SAVEVERSION );
   stream << std::format( "Level   {}\n", mob->level );
   stream << std::format( "Gold	  {}\n", mob->gold );

   if( mob->hotboot )
   {
      stream << std::format( "Resetvnum {}\n", mob->resetvnum );
      stream << std::format( "Resetnum  {}\n", mob->resetnum );
   }

   if( mob->in_room )
   {
      if( mob->hotboot && mob->has_actflag( ACT_SENTINEL ) )
      {
         /*
          * Sentinel mobs get stamped with a "home room" when they are created
          * by create_mobile(), so we need to save them in their home room regardless
          * of where they are right now, so they will go to their home room when they
          * enter the game from a reboot or copyover -- Scion
          */
         stream << std::format( "Room  {}\n", mob->home_vnum );
      }
      else
         stream << std::format( "Room  {}\n", mob->in_room->vnum );
   }
   else
      stream << std::format( "Room           {}\n", ROOM_VNUM_LIMBO );
   if( mob->hotboot && mob->continent )
      stream << std::format( "Continent      {}\n", mob->continent->name );
   stream << std::format( "Coordinates    {} {}\n", mob->map_x, mob->map_y );
   if( !mob->name.empty() && !mob->pIndexData->player_name.empty() && str_cmp( mob->name, mob->pIndexData->player_name ) )
      stream << std::format( "Name           {}~\n", mob->name );
   if( !mob->short_descr.empty() && !mob->pIndexData->short_descr.empty() && str_cmp( mob->short_descr, mob->pIndexData->short_descr ) )
      stream << std::format( "Short          {}~\n", mob->short_descr );
   if( !mob->long_descr.empty() && !mob->pIndexData->long_descr.empty() && str_cmp( mob->long_descr, mob->pIndexData->long_descr ) )
      stream << std::format( "Long           {}~\n", mob->long_descr );
   if( !mob->chardesc.empty() && !mob->pIndexData->chardesc.empty() && str_cmp( mob->chardesc, mob->pIndexData->chardesc ) )
      stream << std::format( "Description    {}~\n", mob->chardesc );
   stream << std::format( "HpManaMove     {} {} {} {} {} {}\n", mob->hit, mob->max_hit, mob->mana, mob->max_mana, mob->move, mob->max_move );
   stream << std::format( "Position       {}\n", mob->position );
   if( !mob->hotboot )
   {
      if( mob->has_actflag( ACT_MOUNTED ) )
         mob->unset_actflag( ACT_MOUNTED );
   }
   if( mob->has_actflags(  ) )
      stream << std::format( "ActFlags       {}~\n", bitset_string( mob->get_actflags(  ), act_flags ) );
   if( mob->has_aflags(  ) )
      stream << std::format( "AffectedBy     {}~\n", bitset_string( mob->get_aflags(  ), aff_flags ) );

   for( auto* af : mob->affects )
   {
      skill_type *skill = nullptr;

      if( af->type >= 0 && !( skill = get_skilltype( af->type ) ) )
         continue;

      if( af->type >= 0 && af->type < TYPE_PERSONAL )
      stream << std::format( "AffectData     '{}' {:3} {:3} {:3} {}\n", skill->name, af->duration, af->modifier, af->location, af->bit );
      else
         fwrite_afk_affect( stream, af );
   }

   if( mob->hotboot )
      stream << "\n";
   else
      stream << std::format( "Exp          {}\n", mob->exp );

   if( !mob->hotboot && !mob->carrying.empty(  ) )
   {
      for( auto it = mob->carrying.begin(  ); it != mob->carrying.end(  ); )
      {
         obj_data *obj = *it;
         ++it;

         if( obj->ego >= sysdata->minego )
            obj->extract(  );
      }
   }

   if( shopmob )
      stream << "EndVendor\n\n";

   mob->de_equip(  );

   if( !mob->carrying.empty(  ) )
      fwrite_obj( mob, mob->carrying, nullptr, stream, 0, ( shopmob ? false : mob->hotboot ) );
   mob->re_equip(  );

   if( !shopmob )
      stream << "EndMobile\n\n";
}

/*
 * Save a character and inventory.
 * Would be cool to save NPC's too for quest purposes, some of the infrastructure is provided.
 */
void char_data::save(  )
{
   if( isnpc(  ) )
      return;

   saving_char = this;

   if( desc && desc->original )
   {
      desc->original->save(  );
      return;
   }

   de_equip(  );

   pcdata->save_time = current_time;

   /*
    * Save immortal stats, level & vnums for wizlist     -Thoric
    * and do_vnums command
    *
    * Also save the player flags so we the wizlist builder can see
    * who is a guest and who is retired.
    */
   if( is_immortal(  ) )
   {
      std::filesystem::path strback = std::format( "{}{}", GOD_DIR, capitalize( pcdata->filename ) );
      std::ofstream stream( strback );
      if( !stream.is_open() )
      {
         bug( "{}: Cannot open {} for writing: {}", __func__, strback.string(), std::strerror(errno) );
      }
      else
      {
         stream << std::format( "Level        {}\n", level );
         stream << std::format( "Pcflags      {}~\n", bitset_string( pcdata->flags, pc_flags ) );
         if( !pcdata->homepage.empty(  ) )
            stream << std::format( "Homepage     {}~\n", pcdata->homepage );
         if( pcdata->realm && !pcdata->realm_name.empty() )
            stream << std::format( "ImmRealm     {}~\n", pcdata->realm_name );
         if( pcdata->low_vnum && pcdata->hi_vnum )
            stream << std::format( "VnumRange    {} {}\n", pcdata->low_vnum, pcdata->hi_vnum );
         if( !pcdata->email.empty(  ) )
            stream << std::format( "Email        {}~\n", pcdata->email );
         stream << "End\n";
         stream.close();
      }
   }

   std::filesystem::path strsave = std::format( "{}{}/{}", PLAYER_DIR, static_cast<char>( std::tolower( name[0] ) ), capitalize( pcdata->filename ) );
   std::ofstream stream( strsave );
   if( !stream.is_open() )
   {
      bug( "{}: Cannot open {} for writing: {}", __func__, strsave.string(), std::strerror(errno) );
   }
   else
   {
      fwrite_char( this, stream );
      if( morph )
         fwrite_morph_data( this, stream );
      if( !carrying.empty(  ) )
         fwrite_obj( this, carrying, nullptr, stream, 0, pcdata->hotboot );

      if( sysdata->save_pets && !pets.empty(  ) )
      {
         for( auto* pet : pets )
         {
            pet->hotboot = pet->master->pcdata->hotboot;
            fwrite_mobile( pet, stream, false );
         }
      }
      fwrite_variables( this, stream );
      pcdata->fwrite_comments( stream );
      stream << "#END\n";
      stream.close();
      if( stream.fail() )
         bug( "{}: Error occurred after closing {}: ", __func__, strsave.string(), std::strerror(errno) );
   }

   ClassSpecificStuff(  ); /* Brought over from DOTD code - Samson 4-6-99 */

   re_equip(  );

   quitting_char = nullptr;
   saving_char = nullptr;
}

short find_old_age( char_data * ch )
{
   short age;

   if( ch->isnpc(  ) )
      return -1;

   auto played = std::chrono::duration_cast<std::chrono::seconds>( ch->pcdata->played ).count();
   age = played / 86400;   /* Calculate realtime number of days played */

   age = age / 7; /* Calculates rough estimate on number of mud years played */

   age += 17;  /* Add 17 years, new characters begin at 17. */

   ch->pcdata->day = ( number_range( 1, sysdata->dayspermonth ) - 1 );  /* Assign random day of birth */
   ch->pcdata->month = ( number_range( 1, sysdata->monthsperyear ) - 1 );  /* Assign random month of birth */
   ch->pcdata->year = time_info.year - age;  /* Assign birth year based on calculations above */

   return age;
}

/*
 * Read in a char.
 */
void fread_char( char_data * ch, std::ifstream & stream, bool preload, bool copyover )
{
   int max_colors = 0;  /* Color code */
   int max_beacons = 0; /* Beacon spell */
   file_ver = 0;

   std::string key;
   while( stream >> key )
   {
      if( key == "Version" )
         stream >> file_ver;
      else if( key == "Name" )
         ch->name = fread_line( stream );
      else if( key == "Password" )
         ch->pcdata->pwd = fread_line( stream );
      else if( key == "Site" )
      {
         if( !copyover && !preload )
         {
            ch->pcdata->prevhost = fread_line( stream );
            ch->print_fmt( "Last connected from: {}\r\n", ch->pcdata->prevhost );
         }
         else
            fread_to_eol( stream );
      }
      else if( key == "Email" )
         ch->pcdata->email = fread_line( stream );
      else if( key == "Homepage" )
         ch->pcdata->homepage = fread_line( stream );
      else if( key == "Bio" )
         ch->pcdata->bio = fread_line( stream );
      else if( key == "Description" )
         ch->chardesc = fread_line( stream );
      else if( key == "Timezone" )
         ch->pcdata->timezone_name = fread_line( stream );
      else if( key == "OLCMapBuffer" )
         fread_string( ch->pcdata->map_buffer, stream );
      else if( key == "Sex" )
      {
         int sex = get_npc_sex( fread_line( stream ) );

         if( sex < 0 || sex >= SEX_MAX )
         {
            bug( "{}: Player {} has invalid sex! Defaulting to Neuter.", __func__, ch->name );
            sex = SEX_NEUTRAL;
         }
         ch->sex = sex;
      }
      else if( key == "Race" )
      {
         int race = get_npc_race( fread_line( stream ) );

         if( race < 0 || race >= MAX_NPC_RACE )
         {
            bug( "{}: Player {} has invalid race! Defaulting to human.", __func__, ch->name );
            race = RACE_HUMAN;
         }
         ch->race = race;
      }
      else if( key == "Class" )
      {
         int Class = get_npc_class( fread_line( stream ) );

         if( Class < 0 || Class >= MAX_NPC_CLASS )
         {
            bug( "{}: Player {} has invalid Class! Defaulting to warrior.", __func__, ch->name );
            Class = CLASS_WARRIOR;
         }
         ch->Class = Class;
      }
      else if( key == "Title" )
         ch->pcdata->title = fread_line( stream );
      else if( key == "Rank" )
         ch->pcdata->rank = fread_line( stream );
      else if( key == "Bestowments" )
         ch->pcdata->bestowments = fread_line( stream );
      else if( key == "AuthedBy" )
         ch->pcdata->authed_by = fread_line( stream );
      else if( key == "Prompt" )
         ch->pcdata->prompt = fread_line( stream );
      else if( key == "FPrompt" )
         ch->pcdata->fprompt = fread_line( stream );
      else if( key == "Deity" )
         ch->pcdata->deity_name = fread_line( stream );
      else if( key == "Clan" )
         ch->pcdata->clan_name = fread_line( stream );
      else if( key == "PCFlags" )
         ch->set_pcflags_file( stream );
      else if( key == "Channels" )
         ch->pcdata->chan_listen = fread_line( stream );
      else if( key =="Speaks" )
         ch->set_langs_file( stream );
      else if( key == "Speaking" )
      {
         std::string speaking = fread_line( stream );
         int value;

         value = get_langnum( speaking );
         if( value < 0 || value >= LANG_UNKNOWN )
            bug( "Unknown language: {}", speaking );
         else
            ch->speaking = value;
      }
      else if( key == "Helled" )
      {
         time_t loaded_time;
         stream >> loaded_time;

         ch->pcdata->release_date = std::chrono::system_clock::from_time_t( loaded_time );

         fread_string( ch->pcdata->helled_by, stream );
      }
      else if( key == "Status" )
      {
         stream >> ch->level >> ch->gold >> ch->exp >> ch->height >> ch->weight >> ch->spellfail >> ch->mental_state;

         if( preload )
            return;
      }
      else if( key == "Status2" )
         stream >> ch->style >> ch->pcdata->practice >> ch->alignment >> ch->pcdata->favor >> ch->hitroll >> ch->damroll >> ch->armor >> ch->wimpy;
      else if( key == "Configs" )
      {
         int old_timezone = -1;

         if( file_ver < 24 )
         {
            int dummyval, vnum = 0;
            stream >> ch->pcdata->pagerlen >> dummyval >> dummyval >> dummyval >> old_timezone >> ch->wait >> dummyval >> vnum;

            room_index *temp = get_room_index( vnum );

            if( !temp )
               temp = get_room_index( ROOM_VNUM_TEMPLE );

            if( !temp )
               temp = get_room_index( ROOM_VNUM_LIMBO );

            /*
             * Um, yeah. If this happens you're shit out of luck!
             */
            if( !temp )
            {
               bug( "{}", "FATAL: No valid fallback rooms. Program terminating!" );
               std::exit( EXIT_FAILURE );
            }

            /*
             * And you're going to crash if the above check failed, because you're an idiot if you remove this Vnum
             */
            if( temp->flags.test( ROOM_ISOLATED ) )
               ch->in_room = get_room_index( ROOM_VNUM_TEMPLE );
            else
               ch->in_room = temp;

            ch->pcdata->timezone_name = convert_old_timezone( old_timezone );
         }
         else if( file_ver == 24 )
         {
            stream >> ch->pcdata->pagerlen >> old_timezone >> ch->wait;

            ch->pcdata->timezone_name = convert_old_timezone( old_timezone );
         }
         else
         {
            stream >> ch->pcdata->pagerlen >> ch->wait;
         }
      }
      else if( key == "Motd" )
      {
         time_t motd, imotd;
         stream >> motd >> imotd;

         ch->pcdata->motd = std::chrono::system_clock::from_time_t( motd );
         ch->pcdata->imotd = std::chrono::system_clock::from_time_t( imotd );
      }
      else if( key == "Age" )
      {
         time_t played;

         stream >> ch->pcdata->age_bonus >> ch->pcdata->day >> ch->pcdata->month >> ch->pcdata->year >> played;

         ch->pcdata->played = std::chrono::hours{played};
      }
      else if( key == "HpManaMove" )
         stream >> ch->hit >> ch->max_hit >> ch->mana >> ch->max_mana >> ch->move >> ch->max_move;
      else if( key == "Regens" )
         stream >> ch->hit_regen >> ch->mana_regen >> ch->move_regen;
      else if( key == "Position" )
      {
         int position = get_npc_position( fread_line( stream ) );

         if( position < 0 || position >= POS_MAX )
         {
            bug( "{}: Player {} has invalid position! Defaulting to standing.", __func__, ch->name );
            position = POS_STANDING;
         }
         ch->position = position;
      }
      else if( key == "Room" )
      {
         // Section added in version 24.
         int rnum;
         stream >> rnum;

         room_index *temp = get_room_index( rnum );

         if( !temp )
            temp = get_room_index( ROOM_VNUM_TEMPLE );

         if( !temp )
            temp = get_room_index( ROOM_VNUM_LIMBO );

         /*
          * Um, yeah. If this happens you're shit out of luck! The game would have crashed anyway.
          */
         if( !temp )
         {
            bug( "{}", "FATAL: No valid fallback rooms. Program terminating!" );
            std::exit( EXIT_FAILURE );
         }

         /*
          * And you're going to crash if the above check failed, because you're an idiot if you remove this Vnum
          */
         if( temp->flags.test( ROOM_ISOLATED ) )
            ch->in_room = get_room_index( ROOM_VNUM_TEMPLE );
         else
            ch->in_room = temp;
      }
      else if( key == "Coordinates" )
      {
         stream >> ch->map_x >> ch->map_y;

         if( file_ver < 24 )
            fread_to_eol( stream );

         if( !ch->has_pcflag( PCFLAG_ONMAP ) )
         {
            ch->map_x = -1;
            ch->map_y = -1;
            ch->continent = nullptr;
         }
      }
      else if( key == "SavingThrows" )
         stream >> ch->saving_poison_death >> ch->saving_wand >> ch->saving_para_petri >> ch->saving_breath >> ch->saving_spell_staff;
      else if( key == "RentData" )
      {
         int dummyval;

         stream >> ch->pcdata->balance >> ch->pcdata->daysidle >> dummyval >> dummyval >> ch->pcdata->camp;
      }
      else if( key == "RentRooms" )
         stream >> ch->pcdata->one;
      else if( key == "BodyParts" )
         ch->set_bparts_file( stream );
      else if( key == "AffectFlags" )
         ch->set_aflags_file( stream );
      else if( key == "NoAffectedBy" )
         ch->set_noaflags_file( stream );
      else if( key == "Resistant" )
         ch->set_resists_file( stream );
      else if( key == "Nores" )
         ch->set_noresists_file( stream );
      else if( key == "Susceptible" )
         ch->set_susceps_file( stream );
      else if( key == "Nosusc" )
         ch->set_nosusceps_file( stream );
      else if( key == "Immune" )
         ch->set_immunes_file( stream );
      else if( key == "Noimm" )
         ch->set_noimmunes_file( stream );
      else if( key == "Absorb" )
         ch->set_absorbs_file( stream );
      else if( key == "KillInfo" )
         stream >> ch->pcdata->pkills >> ch->pcdata->pdeaths >> ch->pcdata->mkills >> ch->pcdata->mdeaths >> ch->pcdata->illegal_pk;
      else if( key == "PTimer" )
      {
         int ptimer;
         stream >> ptimer;

         ch->add_timer( TIMER_PKILLED, ptimer, nullptr, 0 );
      }
      else if( key == "AttrPerm" )
      {
         stream >> ch->perm_str >> ch->perm_int >> ch->perm_wis >> ch->perm_dex >> ch->perm_con >> ch->perm_cha >> ch->perm_lck;

         if( ch->perm_lck == 0 )
            ch->perm_lck = 13;
      }
      else if( key == "AttrMod" )
         stream >> ch->mod_str >> ch->mod_int >> ch->mod_wis >> ch->mod_dex >> ch->mod_con >> ch->mod_cha >> ch->mod_lck;
      else if( key == "Condition" )
         stream >> ch->pcdata->condition[0] >> ch->pcdata->condition[1] >> ch->pcdata->condition[2];
      else if( key == "ImmRealm" )
         ch->pcdata->realm_name = fread_line( stream );
      else if( key == "Bamfin" )
         ch->pcdata->bamfin = fread_line( stream );
      else if( key == "Bamfout" )
         ch->pcdata->bamfout = fread_line( stream );
      else if( key == "ImmData" )
      {
         time_t restore_time;

         stream >> ch->trust >> restore_time >> ch->pcdata->wizinvis >> ch->pcdata->low_vnum >> ch->pcdata->hi_vnum;

         ch->pcdata->restore_time = std::chrono::system_clock::from_time_t( restore_time );
      }
      else if( key == "Skill" )
      {
         int value;
         stream >> value;

         std::string skill = fread_word( stream );
         int sn = find_skill( nullptr, skill, false );

         if( sn < 0 )
            log_printf( "{}: unknown skill: {}", __func__, skill );
         else
         {
            ch->pcdata->learned[sn] = value;
            /*
             * Take care of people who have stuff they shouldn't
             * * Assumes Class and level were loaded before. -- Altrag
             * * Assumes practices are loaded first too now. -- Altrag
             */
            if( ch->level < LEVEL_IMMORTAL )
            {
               if( skill_table[sn]->skill_level[ch->Class] >= LEVEL_IMMORTAL )
               {
                  ch->pcdata->learned[sn] = 0;
                  ++ch->pcdata->practice;
               }
            }
         }
      }
      else if( key == "Spell" )
      {
         int value;
         stream >> value;

         std::string spell = fread_word( stream );
         int sn = find_spell( nullptr, spell, false );

         if( sn < 0 )
            log_printf( "{}: unknown spell: {}", __func__, spell );
         else
         {
            ch->pcdata->learned[sn] = value;
            if( ch->level < LEVEL_IMMORTAL )
            {
               if( skill_table[sn]->skill_level[ch->Class] >= LEVEL_IMMORTAL )
               {
                  ch->pcdata->learned[sn] = 0;
                  ++ch->pcdata->practice;
               }
            }
         }
      }
      else if( key == "Combat" )
      {
         int value;
         stream >> value;

         std::string combat = fread_word( stream );
         int sn = find_combat( nullptr, combat, false );

         if( sn < 0 )
            log_printf( "{}: unknown combat skill: {}", __func__, combat );
         else
         {
            ch->pcdata->learned[sn] = value;
            if( ch->level < LEVEL_IMMORTAL )
            {
               if( skill_table[sn]->skill_level[ch->Class] >= LEVEL_IMMORTAL )
               {
                  ch->pcdata->learned[sn] = 0;
                  ++ch->pcdata->practice;
               }
            }
         }
      }
      else if( key == "Tongue" )
      {
         int value;
         stream >> value;

         std::string tongue = fread_word( stream );
         int sn = find_tongue( nullptr, tongue, false );

         if( sn < 0 )
            log_printf( "{}: unknown tongue: {}", __func__, tongue );
         else
         {
            ch->pcdata->learned[sn] = value;
            if( ch->level < LEVEL_IMMORTAL )
            {
               if( skill_table[sn]->skill_level[ch->Class] >= LEVEL_IMMORTAL )
               {
                  ch->pcdata->learned[sn] = 0;
                  ++ch->pcdata->practice;
               }
            }
         }
      }
      else if( key == "Ability" )
      {
         int value;
         stream >> value;

         std::string ability = fread_word( stream );
         int sn = find_ability( nullptr, ability, false );

         if( sn < 0 )
            log_printf( "{}: unknown ability: {}", __func__, ability );
         else
         {
            ch->pcdata->learned[sn] = value;
            if( ch->level < LEVEL_IMMORTAL )
            {
               if( skill_table[sn]->race_level[ch->race] >= LEVEL_IMMORTAL )
               {
                  ch->pcdata->learned[sn] = 0;
                  ++ch->pcdata->practice;
               }
            }
         }
      }
      else if( key == "Lore" )
      {
         int value;
         stream >> value;

         std::string lore = fread_word( stream );
         int sn = find_lore( nullptr, lore, false );

         if( sn < 0 )
            log_printf( "{}: unknown lore: {}", __func__, lore );
         else
         {
            ch->pcdata->learned[sn] = value;
            if( ch->level < LEVEL_IMMORTAL )
            {
               if( skill_table[sn]->skill_level[ch->Class] >= LEVEL_IMMORTAL )
               {
                  ch->pcdata->learned[sn] = 0;
                  ++ch->pcdata->practice;
               }
            }
         }
      }
      else if( key == "Affect" || key == "AffectData" || key == "AffData" )
      {
         affect_data *paf = nullptr;

         if( key == "AffData" )
            paf = fread_afk_affect( stream );
         else
         {
            int sn;
            std::string sname = fread_word( stream );
            paf = new affect_data;

            if( ( sn = skill_lookup( sname ) ) < 0 )
            {
               if( ( sn = herb_lookup( sname ) ) < 0 )
                  log_printf( "{}: unknown skill.", __func__ );
               else
                  sn += TYPE_HERB;
            }
            paf->type = sn;
            stream >> paf->duration;
            stream >> paf->modifier;
            stream >> paf->location;
            if( paf->location == APPLY_WEAPONSPELL
               || paf->location == APPLY_WEARSPELL || paf->location == APPLY_REMOVESPELL || paf->location == APPLY_STRIPSN || paf->location == APPLY_RECURRINGSPELL )
               paf->modifier = slot_lookup( paf->modifier );
            stream >> paf->bit;
            if( paf->bit >= MAX_AFFECTED_BY )
            {
               deleteptr( paf );
            }
         }
         if( paf )
            ch->affects.push_back( paf );
      }
      else if( key == "MaxColors" )
         stream >> max_colors;
      else if( key == "Colors" )
      {
         /*
          * Load color values - Samson 9-29-98
          */
         for( int x = 0; x < max_colors; ++x )
            stream >> ch->pcdata->colors[x];
      }
      else if( key == "MaxBeacons" )
         stream >> max_beacons;
      else if( key == "Beacons" )
      {
         /*
          * Load beacons - Samson 9-29-98
          */
         for( int x = 0; x < max_beacons; ++x )
            stream >> ch->pcdata->beacon[x];
      }
      else if( key == "Alias" )
      {
         std::string alias, cmd;

         fread_string( alias, stream );
         fread_string( cmd, stream );
         ch->pcdata->alias_map[alias] = cmd;
      }
      else if( key == "Board_Data" )
      {
         const board_data *board = nullptr;

         std::string word = fread_word( stream );
         if( !( board = get_board( nullptr, word ) ) )
         {
            log_printf( "Player {} has board {} which apparently doesn't exist?", ch->name, word );
            ch->print_fmt( "Warning: the board {} no longer exists.\r\n", word );
            fread_to_eol( stream );
         }

         if( board )
         {
            board_chardata *pboard = new board_chardata;
            pboard->board_name = board->name;

            time_t last_read;
            stream >> last_read;
            pboard->last_read = std::chrono::system_clock::from_time_t( last_read );

            stream >> pboard->alert;
            ch->pcdata->boarddata.push_back( pboard );
         }
      }
      else if( key == "Zone" )
      {
         bool found = false;
         std::string zonename = fread_line( stream );

         for( const auto* tarea : arealist )
         {
            if( !str_cmp( tarea->name, zonename ) )
            {
               found = true;
               break;
            }
         }

         if( found )
         {
            auto it = std::lower_bound( ch->pcdata->zone.begin(), ch->pcdata->zone.end(), zonename );
            ch->pcdata->zone.insert( it, zonename );
         }
      }
      else if( key == "Ignored" )
      {
         /*
          * Get the name
          */
         std::string temp = fread_line( stream );
         std::filesystem::path fname = std::format( "{}{}/{}", PLAYER_DIR, static_cast<char>( std::tolower( temp[0] ) ), capitalize( temp ) );

         /*
          * If there isn't a pfile for the name then don't add it to the list
          */
         if( std::filesystem::exists( fname ) )
         {
            /*
            * Add the name unless the limit has been reached
            */
            if( ch->pcdata->ignore.size(  ) >= sysdata->maxign )
               bug( "{}: too many ignored names", __func__ );
            else
               ch->pcdata->ignore.push_back( temp );
         }
      }
      else if( key == "Qbit" )
      {
         std::map<int, std::string>::iterator bit;
         std::string desc;

         int number;
         stream >> number;
         fread_string( desc, stream );

         if( ( bit = qbits.find( number ) ) != qbits.end(  ) )
         {
            if( bit->second.empty(  ) )
               ch->pcdata->qbits[number] = desc;
            else
               ch->pcdata->qbits[number] = qbits[number];
         }
         else
            ch->pcdata->qbits[number] = desc;
      }
      else if( key == "End" )
      {
         if( preload )
            return;

         std::string buf;

         /*
          * Let no character be trusted higher than one below maxlevel -- Narn
          */
         ch->trust = umin( ch->trust, MAX_LEVEL - 1 );

         if( ch->pcdata->played < std::chrono::hours::zero() )
            ch->pcdata->played = std::chrono::hours::zero();

         if( !ch->pcdata->chan_listen.empty(  ) )
         {
            std::string channels = ch->pcdata->chan_listen;
            std::string arg;

            while( !channels.empty(  ) )
            {
               channels = one_argument( channels, arg );

               if( !find_channel( arg ) )
                  removename( ch->pcdata->chan_listen, arg );
            }
         }
         /*
          * Provide at least the one channel
          */
         else
            ch->pcdata->chan_listen = "chat";

         ch->pcdata->editor = nullptr;

         /*
          * no good for newbies at all
          */
         if( !ch->is_immortal(  ) && ( !ch->speaking || ch->speaking < 0 ) )
            ch->speaking = LANG_COMMON;
         if( ch->is_immortal(  ) )
            ch->set_lang( -1 );

         // No height or weight? BAD!
         if( ch->height == 0 )
            ch->height = ch->calculate_race_height(  );

         if( ch->weight == 0 )
            ch->weight = ch->calculate_race_weight(  );

         ch->unset_pcflag( PCFLAG_MAPEDIT ); /* In case they saved while editing */

         if( ch->pcdata->year == 0 )
            ch->pcdata->age = find_old_age( ch );
         else
            ch->pcdata->age = ch->calculate_age(  );

         if( ch->pcdata->month > sysdata->monthsperyear - 1 )
            ch->pcdata->month = sysdata->monthsperyear - 1; /* Catches the bad month values */

         if( ch->pcdata->day > sysdata->dayspermonth - 1 )
            ch->pcdata->day = sysdata->dayspermonth - 1; /* Catches the bad day values */

         if( !ch->pcdata->deity_name.empty(  ) && !( ch->pcdata->deity = get_deity( ch->pcdata->deity_name ) ) )
         {
            buf = std::format( "&R\r\nYour deity, {}, has met its demise!\r\n", ch->pcdata->deity_name );
            add_login_message( ch->name, 18, buf );
            ch->pcdata->deity_name.clear(  );
         }

         if( ch->pcdata->deity_name.empty(  ) )
            ch->pcdata->favor = 0;

         if( !ch->pcdata->clan_name.empty(  ) && !( ch->pcdata->clan = get_clan( ch->pcdata->clan_name ) ) )
         {
            buf = std::format( "&R\r\nWarning: The organization {} no longer exists, and therefore you no longer belong to that organization.\r\n", ch->pcdata->clan_name );
            add_login_message( ch->name, 18, buf );
            ch->pcdata->clan_name.clear(  );
         }

         if( ch->pcdata->clan )
            update_roster( ch );

         if( !ch->pcdata->realm_name.empty(  ) && !( ch->pcdata->realm = get_realm( ch->pcdata->realm_name ) ) )
         {
            buf = std::format( "&Y\r\nWarning: The realm {} no longer exists, and therefore you no longer belong to a realm.\r\n", ch->pcdata->realm_name );
            add_login_message( ch->name, 18, buf );
            ch->pcdata->realm_name.clear(  );
         }

         if( !ch->pcdata->map_buffer.empty() )
            restore_map_buffer( ch );

         if( ch->has_pcflag( PCFLAG_ONMAP ) )
            ch->continent = find_continent_by_room_vnum( ch->in_room->vnum );
         return;
      }
      else
      {
         bug( "{}: Bad section '{}' - skipping.", __func__, key );
         fread_to_eol( stream );
      }
   }
   bug( "{}: Fell through to bottom. Possible corrupted file. KEY: {}", __func__, key );
}

void fread_obj( char_data * ch, std::ifstream & stream, short os_type )
{
   obj_data *obj;
   int iNest, obj_file_ver = 0;
   bool fNest, fVnum;
   room_index *room = nullptr;

   if( ch )
   {
      room = ch->in_room;
      if( ch->tempnum == -9999 )
         file_ver = 0;
   }

   obj = new obj_data;
   obj->count = 1;
   obj->wear_loc = -1;
   obj->weight = 1;
   obj->continent = nullptr;
   obj->map_x = -1;
   obj->map_y = -1;

   fNest = true;  /* Requiring a Nest 0 is a waste */
   fVnum = false; // We can't assume this - what if Vnum isn't written to the file? Crashy crashy is what. - Pulled from Smaug 1.8
   iNest = 0;

   std::string key;
   std::string temp;
   while( stream >> key )
   {
      if( key == "Version" )
         stream >> obj_file_ver;
      else if( key == "Nest" )
      {
         stream >> iNest;

         if( iNest < 0 || iNest >= MAX_NEST )
         {
            bug( "{}: bad nest {}.", __func__, iNest );
            iNest = 0;
            fNest = false;
         }
      }
      else if( key == "Count" )
         stream >> obj->count;
      else if( key == "Name" )
         obj->name = fread_line( stream );
      else if( key == "ShortDescr" )
         obj->short_descr = fread_line( stream );
      else if( key == "Description" )
         obj->objdesc = fread_line( stream );
      else if( key == "ActionDesc" )
         obj->action_desc = fread_line( stream );
      else if( key == "Ovnum" )
      {
         int vnum;
         stream >> vnum;

         if( !( obj->pIndexData = get_obj_index( vnum ) ) )
            fVnum = false;
         else
         {
            fVnum = true;
            obj->cost = obj->pIndexData->cost;
            obj->ego = obj->pIndexData->ego;
            obj->weight = obj->pIndexData->weight;
            obj->item_type = obj->pIndexData->item_type;
            obj->wear_flags = obj->pIndexData->wear_flags;
            obj->extra_flags = obj->pIndexData->extra_flags;
         }
      }
      else if( key == "Ego" ) // Samson 5-8-99
         stream >> obj->ego;
      else if( key == "Room" )
      {
         int vnum;
         stream >> vnum;

         room = get_room_index( vnum );
      }
      else if( key == "Rvnum" ) // hotboot tracker
         stream >> obj->room_vnum;
      else if( key == "ExtraFlags" )
      {
         temp = fread_line( stream );

         flag_string_set( temp, obj->extra_flags, o_flags );
      }
      else if( key == "WearFlags" )
      {
         temp = fread_line( stream );

         flag_string_set( temp, obj->wear_flags, w_flags );
      }
      else if( key == "WearLoc" )
         stream >> obj->wear_loc;
      else if( key == "ItemType" )
         stream >> obj->item_type;
      else if( key == "Weight" )
         stream >> obj->weight;
      else if( key == "Level" )
         stream >> obj->level;
      else if( key == "Timer" )
         stream >> obj->timer;
      else if( key == "Cost" )
         stream >> obj->cost;
      else if( key == "Seller" )
         obj->seller = fread_line( stream ); // Samson 6-20-99
      else if( key == "Buyer" )
         obj->buyer = fread_line( stream ); // Samson 6-20-99
      else if( key == "Bid" )
         stream >> obj->bid; // Samson 6-20-99
      else if( key == "Owner" )
         obj->owner = fread_line( stream );
      else if( key == "Oday" )
         stream >> obj->day;
      else if( key == "Omonth" )
         stream >> obj->month;
      else if( key == "Oyear" )
         stream >> obj->year;
      else if( key == "Coords" || key == "Coordinates" )
      {
         stream >> obj->map_x;
         stream >> obj->map_y;

         if( obj_file_ver < 24 )
            fread_to_eol( stream );
      }
      else if( key == "Values" )
      {
         std::string line = fread_line( stream, '\n' );
         std::stringstream ss(line);

         for( int x = 0; x < MAX_OBJ_VALUE; ++x )
         {
            if( !( ss >> obj->value[x] ) )
               obj->value[x] = 0;
         }

         if( obj->value[6] == 0 && ( obj->item_type == ITEM_WEAPON || obj->item_type == ITEM_MISSILE_WEAPON ) )
            obj->value[6] = obj->value[0];

         if( obj->value[5] == 0 && obj->item_type == ITEM_PROJECTILE )
            obj->value[5] = obj->value[0];

         // Note to future self looking at this in what's possibly 2025: Don't try to fix this again, ok? It works properly as listed now that hotboots have been accounted for.
         // It will still log from other sources though, so hey, if something OTHER that the hotboot recovery is triggering it, investigate that cause it may not be right!
         if( file_ver < 10 && fVnum == true && os_type != OS_CORPSE && ch->tempnum != -9999 )
         {
            log_printf( "{}: != OS_CORPSE case encountered. file_ver={}", __func__, file_ver );
            obj->value[0] = obj->pIndexData->value[0];
            obj->value[1] = obj->pIndexData->value[1];
            obj->value[2] = obj->pIndexData->value[2];
            obj->value[3] = obj->pIndexData->value[3];
            obj->value[4] = obj->pIndexData->value[4];
            obj->value[5] = obj->pIndexData->value[5];

            if( obj->item_type == ITEM_WEAPON )
            {
               obj->value[2] = obj->pIndexData->value[1] * obj->pIndexData->value[2];
            }
         }
      }
      else if( key == "Sockets" )
      {
         obj->socket[0] = fread_word( stream );
         obj->socket[1] = fread_word( stream );
         obj->socket[2] = fread_word( stream );
      }
      else if( key == "Spell" )
      {
         int iValue, sn;

         stream >> iValue;
         std::string word = fread_word( stream );
         sn = skill_lookup( word );
         if( iValue < 0 || iValue > 10 )
            bug( "{}: bad iValue {}.", __func__, iValue );
         /*
          * Bug fixed here to change corrupted spell values to -1 to stop spamming logs - Samson 7-5-03
          */
         else if( sn < 0 )
         {
            bug( "{}: Vnum {} - unknown skill: {}", __func__, obj->pIndexData->vnum, word );
            obj->value[iValue] = -1;
         }
         else
            obj->value[iValue] = sn;
      }
      else if( key == "Affect" || key == "AffectData" )
      {
         affect_data *paf;
         int pafmod;

         paf = new affect_data;
         if( key == "Affect" )
            stream >> paf->type;
         else
         {
            std::string word = fread_word( stream );
            int sn = skill_lookup( word );
            if( sn < 0 )
               bug( "{}: Vnum {} - unknown skill: {}", __func__, obj->pIndexData->vnum, word );
            else
               paf->type = sn;
         }
         stream >> paf->duration;
         stream >> pafmod;
         stream >> paf->location;
         stream >> paf->bit;
         if( paf->location == APPLY_WEAPONSPELL
            || paf->location == APPLY_WEARSPELL || paf->location == APPLY_STRIPSN || paf->location == APPLY_REMOVESPELL || paf->location == APPLY_RECURRINGSPELL )
            paf->modifier = slot_lookup( pafmod );
         else
            paf->modifier = pafmod;
         obj->affects.push_back( paf );
      }
      else if( key == "ExtraDescr" )
      {
         extra_descr_data *ed = new extra_descr_data;

         fread_string( ed->keyword, stream );
         fread_string( ed->desc, stream );
         obj->extradesc.push_back( ed );
      }
      else if( key == "Rent" )
         stream >> obj->ego; // Samson 5-8-99 - OLD FIELD
      else if( key == "End" )
      {
         if( !fNest || !fVnum )
         {
            if( !obj->name.empty() )
               bug( "{}: {} incomplete object. obj_file_ver={}", __func__, obj->name, obj_file_ver );
            else
               bug( "{}: incomplete object. obj_file_ver={}", __func__, obj_file_ver );
            deleteptr( obj );
            return;
         }
         else
         {
            short wear_loc = obj->wear_loc;

            if( obj->name.empty() && !obj->pIndexData->name.empty() )
               obj->name = obj->pIndexData->name;
            if( obj->short_descr.empty() && !obj->pIndexData->short_descr.empty() )
               obj->short_descr = obj->pIndexData->short_descr;
            if( obj->objdesc.empty() && !obj->pIndexData->objdesc.empty() )
               obj->objdesc = obj->pIndexData->objdesc;
            if( obj->action_desc.empty() && !obj->pIndexData->action_desc.empty() )
               obj->action_desc = obj->pIndexData->action_desc;
            if( obj->extra_flags.test( ITEM_PERSONAL ) && obj->owner.empty() && ch )
               obj->owner = ch->name;
            if( obj->ego > 90 )
               obj->ego = obj->pIndexData->set_ego(  );
            objlist.push_back( obj );

            /*
             * Don't fix it if it matches the vnum for random treasure
             */
            if( obj->item_type == ITEM_WEAPON && obj->pIndexData->vnum != OBJ_VNUM_TREASURE )
               obj->value[4] = obj->pIndexData->value[4];

            // This is a hack. Some items which have been idling in clan storage are corrupt.
            if( obj->item_type != ITEM_WEAPON && obj->item_type != ITEM_MISSILE_WEAPON )
               obj->value[8] = obj->value[9] = obj->value[10] = 0;

            /*
             * Altered count method for rare items - Samson 11-5-98
             */
            obj->pIndexData->count += obj->count;

            if( obj->ego >= sysdata->minego )
               obj->pIndexData->count -= obj->count;

            if( fNest )
               rgObjNest[iNest] = obj;
            numobjsloaded += obj->count;
            ++physicalobjects;
            if( file_ver > 1 || wear_loc < -1 || wear_loc >= MAX_WEAR )
               obj->wear_loc = -1;

            // Fix equipment on invalid body parts
            if( ch && ch->race > 0 && ch->race < MAX_PC_RACE && !is_valid_wear_loc( ch, wear_loc ) )
            {
               wear_loc = -1;
               obj->wear_loc = -1;
            }

            /*
             * Corpse saving. -- Altrag
             */
            if( os_type == OS_CORPSE )
            {
               if( !room )
               {
                  bug( "{}: Corpse without room", __func__ );
                  room = get_room_index( ROOM_VNUM_LIMBO );
               }

               /*
                * Give the corpse a timer if there isn't one
                */
               if( obj->timer < 1 )
                  obj->timer = 80;
               obj->to_room( room, nullptr );
               if( obj->extra_flags.test( ITEM_ONMAP ) )
                  obj->continent = find_continent_by_room_vnum( obj->in_room->vnum );
            }
            else if( iNest == 0 || rgObjNest[iNest] == nullptr )
            {
               int slot = -1;
               bool reslot = false;

               if( file_ver > 1 && wear_loc > -1 && wear_loc < MAX_WEAR )
               {
                  for( int x = 0; x < MAX_LAYERS; ++x )
                     if( ch->isnpc(  ) )
                     {
                        if( !mob_save_equipment[wear_loc][x] )
                        {
                           mob_save_equipment[wear_loc][x] = obj;
                           slot = x;
                           reslot = true;
                           break;
                        }
                     }
                     else
                     {
                        if( !save_equipment[wear_loc][x] )
                        {
                           save_equipment[wear_loc][x] = obj;
                           slot = x;
                           reslot = true;
                           break;
                        }
                     }
               }
               obj->to_char( ch );
               if( reslot && slot != -1 )
               {
                  if( ch->isnpc(  ) )
                     mob_save_equipment[wear_loc][slot] = obj;
                  else
                     save_equipment[wear_loc][slot] = obj;
               }
            }
            else
            {
               if( rgObjNest[iNest - 1] )
               {
                  rgObjNest[iNest - 1]->separate(  );
                  obj = obj->to_obj( rgObjNest[iNest - 1] );
               }
               else
                  bug( "{}: nest layer missing {}", __func__, iNest - 1 );
            }
            if( fNest )
               rgObjNest[iNest] = obj;
            return;
         }
      }
      else
      {
         bug( "{}: Bad section '{}' - skipping.", __func__, key );
         fread_to_eol( stream );
      }
   } // End while loop
   bug( "{}: Fell through to bottom. Possible corrupted file.", __func__ );
}

/*
 * This will read one mobile structure pointer to by fp --Shaddai
 *   Edited by Tarl 5 May 2002 to allow pets to load equipment.
 */
char_data *fread_mobile( std::ifstream & stream, bool shopmob, bool hotboot )
{
   char_data *mob = nullptr;
   std::string word, key;
   int inroom = 0;
   room_index *pRoomIndex = nullptr;
   mob_index *pMobIndex = nullptr;
   int mob_file_ver = 0;

   stream >> key;
   if( stream.eof() )
   {
      bug( "{}: Reached EOF on file stream.", __func__ );
      return nullptr;
   }

   if( key == "Vnum" )
   {
      int vnum;
      stream >> vnum;

      if( !( pMobIndex = get_mob_index( vnum ) ) )
      {
         while( stream >> key )
         {
            /*
             * So we don't get so many bug messages when something messes up
             * * --Shaddai
             */
            if( key != "EndMobile" && key != "EndVendor" )
               fread_to_eol( stream );
            else
            {
               bug( "{}: No index data for vnum {}", __func__, vnum );
               return nullptr;
            }
         }
      }
      else
         mob = pMobIndex->create_mobile(  );
   }
   else
   {
      while( stream >> key )
      {
         /*
          * So we don't get so many bug messages when something messes up
          * * --Shaddai
          */
         if( key != "EndMobile" && key != "EndVendor" )
            fread_to_eol( stream );
         else
         {
            bug( "{}: Vnum not found", __func__ );
            return nullptr;
         }
      }
   }

   // Safety catch:
   if( !mob )
      return nullptr;

   while( stream >> key )
   {
      if( key == "Version" )
         stream >> mob_file_ver;
      else if( key == "Level" )
         stream >> mob->level;
      else if( key == "Gold" )
         stream >> mob->gold;
      else if( key == "Resetvnum" )
         stream >> mob->resetvnum;
      else if( key == "Resetnum" )
         stream >> mob->resetnum;
      else if( key == "Room" )
         stream >> inroom;
      else if( key == "Continent" )
      {
         std::string temp;

         fread_string( temp, stream );
         continent_data *continent = find_continent_by_name( temp );

         if( continent )
            mob->continent = continent;
      }
      else if( key == "Coordinates" )
      {
         stream >> mob->map_x;
         stream >> mob->map_y;

         if( mob_file_ver < 24 )
            fread_to_eol( stream );
      }
      else if( key == "Name" )
         mob->name = fread_line( stream );
      else if( key == "Short" )
         mob->short_descr = fread_line( stream );
      else if( key == "Long" )
         mob->long_descr = fread_line( stream );
      else if( key == "Description" )
         mob->chardesc = fread_line( stream );
      else if( key == "HpManaMove" )
         stream >> mob->hit >> mob->max_hit >> mob->mana >> mob->max_mana >> mob->move >> mob->max_move;
      else if( key == "Position" )
         stream >> mob->position;
      else if( key == "ActFlags" )
         mob->set_actflags_file( stream );
      else if( key == "AffectedBy" )
         mob->set_aflags_file( stream );
      else if( key == "Affect" || key == "AffectData" )
      {
         affect_data *paf;

         paf = new affect_data;
         if( !str_cmp( word, "Affect" ) )
            stream >> paf->type;
         else
         {
            int sn;
            std::string sname = fread_word( stream );

            if( ( sn = skill_lookup( sname ) ) < 0 )
            {
               if( ( sn = herb_lookup( sname ) ) < 0 )
                  bug( "{}: unknown skill.", __func__ );
               else
                  sn += TYPE_HERB;
            }
            paf->type = sn;
         }

         stream >> paf->duration;
         stream >> paf->modifier;
         stream >> paf->location;
         if( paf->location == APPLY_WEAPONSPELL
            || paf->location == APPLY_WEARSPELL || paf->location == APPLY_REMOVESPELL || paf->location == APPLY_STRIPSN || paf->location == APPLY_RECURRINGSPELL )
            paf->modifier = slot_lookup( paf->modifier );
         stream >> paf->bit;
         mob->affects.push_back( paf );
      }
      else if( key == "Exp" )
         stream >> mob->exp;
      else if( key == "EndMobile" || key == "EndVendor" )
      {
         if( inroom == 0 )
            inroom = ROOM_VNUM_TEMPLE;
         pRoomIndex = get_room_index( inroom );
         if( !pRoomIndex )
            pRoomIndex = get_room_index( ROOM_VNUM_LIMBO );
         mob->tempnum = -9998;   /* Yet another hackish fix! */
         if( !mob->to_room( pRoomIndex ) )
            log_printf( "char_to_room: {}:{}, line {}.", __FILE__, __func__, __LINE__ );

         for( int i = 0; i < MAX_WEAR; ++i )
         {
            for( int x = 0; x < MAX_LAYERS; ++x )
            {
               if( mob_save_equipment[i][x] )
               {
                  mob->equip( mob_save_equipment[i][x], i );
                  mob_save_equipment[i][x] = nullptr;
               }
            }
         }
         return mob;
      }
      else if( key == "#OBJECT" )
      {  /* Objects      */
         mob->tempnum = -9999;   /* Hackish, yes. Works though doesn't it? */
         fread_obj( mob, stream, OS_CARRY );
      }
      else
      {
         bug( "{}: Bad section '{}' - skipping.", __func__, key );
         fread_to_eol( stream );
      }
   }
   bug( "{}: Fell through to bottom. Possible corrupted file.", __func__ );
   return mob;
}

/*
 * Load a char and inventory into a new ch structure.
 */
bool load_char_obj( descriptor_data * d, std::string_view name, bool preload, bool copyover )
{
   bool found = false;
   int i, x;

   if( !d )
   {
      bug( "{}: nullptr d! This should not have happened!", __func__ );
      return false;
   }

   char_data *ch = new char_data;

   for( x = 0; x < MAX_WEAR; ++x )
   {
      for( i = 0; i < MAX_LAYERS; ++i )
      {
         save_equipment[x][i] = nullptr;
         mob_save_equipment[x][i] = nullptr;
      }
   }
   loading_char = ch;

   ch->pcdata = new pc_data;

   d->character = ch;
   ch->desc = d;
   ch->pcdata->filename = capitalize( name );
   if( !d->hostname.empty() )
      ch->pcdata->lasthost = d->hostname;
   else
      ch->pcdata->lasthost = d->ipaddress;
   ch->style = STYLE_FIGHTING;
   ch->mental_state = -10;
   ch->pcdata->prompt = default_prompt( ch );
   ch->pcdata->fprompt = default_fprompt( ch );
   ch->pcdata->version = 0;
   ch->set_langs( 0 );
   ch->set_lang( LANG_COMMON );
   ch->speaking = LANG_COMMON;
   ch->abits.clear(  );
   ch->pcdata->qbits.clear(  );

   /*
    * Setup color values in case player has none set - Samson 
    */
   reset_colors( ch );

   std::filesystem::path strsave = std::format( "{}{}/{}", PLAYER_DIR, static_cast<char>( std::tolower( name.front() ) ), capitalize( name ) );
   if( std::filesystem::exists( strsave ) && d->connected != CON_PLOADED )
   {
      if( preload )
         log_printf_plus( LOG_COMM, LEVEL_KL, "Preloading player data for: {}", ch->pcdata->filename );
      else
         log_printf_plus( LOG_COMM, LEVEL_KL, "Loading player data for {} ({}K)", ch->pcdata->filename, static_cast<long>( std::filesystem::file_size( strsave ) / 1024 ) );
   }
   /*
    * else no player file 
    */

   std::ifstream stream( strsave );
   if( stream.is_open() )
   {
      for( int iNest = 0; iNest < MAX_NEST; ++iNest )
         rgObjNest[iNest] = nullptr;

      found = true;
      /*
       * Cheat so that bug will show line #'s -- Altrag 
       */
      strArea = strsave;
      std::string key;
      while( stream >> key )
      {
         if( key == "#PLAYER" )
         {
            fread_char( ch, stream, preload, copyover );
            if( preload )
               break;
         }
         else if( key == "#OBJECT" )  /* Objects  */
            fread_obj( ch, stream, OS_CARRY );
         else if( key == "MorphData" )  /* Morphs */
            fread_morph_data( ch, stream );
         else if( key == "#COMMENT2" )
            ch->pcdata->fread_comment( stream ); /* Comments */
         else if( key == "#COMMENT" )
            ch->pcdata->fread_old_comment( stream );   /* Older Comments */
         else if( key == "#MOBILE" )
         {
            char_data *mob = nullptr;
            mob = fread_mobile( stream, false, false );
            if( mob )
            {
               bind_follower( mob, ch, -1, -2 );
               if( mob->in_room && ch->in_room )
               {
                  mob->from_room(  );
                  if( !mob->to_room( ch->in_room ) )
                     log_printf( "char_to_room: {}:{}, line {}.", __FILE__, __func__, __LINE__ );
               }
            }
         }
         else if( key == "#VARIABLE" )   // Quest Flags
            fread_variable( ch, stream );
         else if( key == "#END" )  /* Done */
            break;
         else
         {
            bug( "{}: bad section: {}", __func__, key );
            fread_to_eol( stream );
         }
      }
      stream.close();
      strArea = "$";
   }

   if( !found )
   {
      if( d->msp_detected )
         ch->set_pcflag( PCFLAG_MSP );
      ch->name = name;
   }
   else
   {
      if( ch->name.empty() )
         ch->name = name;

      if( ch->is_immortal(  ) )
      {
         if( ch->pcdata->wizinvis < 2 )
            ch->pcdata->wizinvis = ch->level;
         assign_area( ch );
      }
      if( file_ver > 1 )
      {
         for( i = 0; i < MAX_WEAR; ++i )
            for( x = 0; x < MAX_LAYERS; ++x )
               if( save_equipment[i][x] )
               {
                  ch->equip( save_equipment[i][x], i );
                  save_equipment[i][x] = nullptr;
               }
               else
                  break;
      }
      ch->ClassSpecificStuff(  );   /* Brought over from DOTD code - Samson 4-6-99 */
   }

   /*
    * Rebuild affected_by and RIS to catch errors - FB 
    */
   ch->update_aris(  );
   loading_char = nullptr;
   return found;
}

/* Rewritten corpse saver - Samson */
void write_corpse( obj_data * corpse, std::string_view name )
{
   std::filesystem::path filename = std::format( "{}{}", CORPSE_DIR, capitalize( name ) );

   // No no no no no! Timer of 0 means it should be GONE GONE GONE!
   if( corpse->timer < 1 )
   {
      std::filesystem::remove( filename );
      return;
   }

   std::ofstream stream( filename );
   if( !stream.is_open() )
   {
      bug( "{}: Error opening corpse file for write: {}", __func__, filename.string() );
      return;
   }

   stream << "#CORPSE\n";
   stream << std::format( "Version      {}\n", SAVEVERSION );
   if( corpse->count > 1 )
      stream << std::format( "Count        {}\n", corpse->count );
   if( str_cmp( corpse->name, corpse->pIndexData->name ) )
      stream << std::format( "Name         {}~\n", corpse->name );
   if( str_cmp( corpse->short_descr, corpse->pIndexData->short_descr ) )
      stream << std::format( "ShortDescr   {}~\n", corpse->short_descr );
   if( str_cmp( corpse->objdesc, corpse->pIndexData->objdesc ) )
      stream << std::format( "Description  {}~\n", corpse->objdesc );
   stream << std::format( "Ovnum        {}\n", corpse->pIndexData->vnum );
   if( corpse->in_room )
   {
      stream << std::format( "Room         {}\n", corpse->in_room->vnum );
      stream << std::format( "Rvnum        {}\n", corpse->room_vnum );
   }
   if( corpse->weight != corpse->pIndexData->weight )
      stream << std::format( "Weight       {}\n", corpse->weight );
   if( corpse->level )
      stream << std::format( "Level        {}\n", corpse->level );
   if( corpse->timer )
      stream << std::format( "Timer        {}\n", corpse->timer );
   if( corpse->cost != corpse->pIndexData->cost )
      stream << std::format( "Cost         {}\n", corpse->cost );
   stream << std::format( "Coordinates  {} {}\n", corpse->map_x, corpse->map_y );
   stream << "Values      ";
   for( int x = 0; x < MAX_OBJ_VALUE; ++x )
      stream << std::format( " {}", corpse->value[x] );
   stream << "\n";
   stream << "End\n\n";

   if( !corpse->contents.empty(  ) )
      fwrite_obj( nullptr, corpse->contents, nullptr, stream, 1, false );

   stream << "#END\n\n";
   stream.close();
}

void load_corpses( void )
{
   falling = 1;   /* Arbitrary, must be >0 though. */
   for( const auto& entry : std::filesystem::directory_iterator( CORPSE_DIR ) )
   {
      const auto& path = entry.path();
      const std::string filename = path.filename().string();

      if( filename.empty() || filename[0] == '.' )
         continue;

      strArea = std::format( "{}{}", CORPSE_DIR, filename );
      std::ifstream stream( std::filesystem::path{strArea} );
      if( !stream.is_open() )
      {
         bug( "{}: Unable to open corpse file {} for reading.", __func__, strArea );
         continue;
      }

      std::string key;
      while( stream >> key )
      {
         if( key == "#CORPSE" )
            fread_obj( nullptr, stream, OS_CORPSE );
         else if( key == "#OBJECT" )
            fread_obj( nullptr, stream, OS_CARRY );
         else if( key == "#END" )
            break;
         else
         {
            bug( "{}: Bad section '{}' in {} - skipping.", __func__, key, strArea );
            fread_to_eol( stream );
         }
      }
      stream.close();
   }
   strArea = "$";
   falling = 0;
}

CMDF( do_save )
{
   if( ch->isnpc(  ) )
      return;

   ch->WAIT_STATE( 2 ); /* For big muds with save-happy players, like RoD */
   ch->update_aris(  ); /* update char affects and RIS */
   ch->save(  );
   saving_char = nullptr;
   ch->print( "Saved...\r\n" );
}
