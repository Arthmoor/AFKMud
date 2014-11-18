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
 *                   Character saving and loading module                    *
 ****************************************************************************/

#include <dirent.h>
#include <sys/stat.h>
#include "mud.h"
#include "bits.h"
#include "boards.h"
#include "channels.h"
#include "clans.h"
#include "deity.h"
#include "descriptor.h"
#include "finger.h"
#include "mobindex.h"
#include "objindex.h"
#include "raceclass.h"
#include "realms.h"
#include "roomindex.h"

extern FILE *fpArea;

/*
 * Externals
 */
void reset_colors( char_data * );
board_data *get_board( char_data *, const string & );
#ifdef IMC
void imc_initchar( char_data * );
bool imc_loadchar( char_data *, FILE *, const char * );
void imc_savechar( char_data *, FILE * );
#endif
void fwrite_morph_data( char_data *, FILE * );
void fread_morph_data( char_data *, FILE * );
void fread_variable( char_data *, FILE * );
void fwrite_variables( char_data *, FILE * );
char *default_fprompt( char_data * );
char *default_prompt( char_data * );
void bind_follower( char_data *, char_data *, int, int );
void assign_area( char_data * );
affect_data *fread_afk_affect( FILE * );
bool is_valid_wear_loc( char_data *, int );

/*
 * Increment with every major format change.
 */
const int SAVEVERSION = 23;
/* Updated to version 4 after addition of alias code - Samson 3-23-98 */
/* Updated to version 5 after installation of color code - Samson */
/* Updated to version 6 for rare item tracking support - Samson */
/* DOTD pfiles saved as version 7 */
/* Updated to version 8 for text based data saving - Samson */
/* Updated to version 9 for new exp tables - Samson 4-30-99 */
/* Updated to version 10 after weapon code updates - Samson 1-15-00 */
/* Updated to version 11 for mv + 50 boost - Samson 4-25-00 */
/* Updated to version 12 for mana recalcs - Samson 1-19-01 */
/* Updated to version 13 to force activation of MSP/MXP for old players - Samson 8-21-01 */
/* Updated to version 14 to force activation of MXP Prompt line - Samson 2-27-02 */
/* Updated to version 15 for new exp system - Samson 12-15-02 */
/* Updated to 16 to award stat gains for old characters - Samson 12-16-02 */
/* Updated to 17 for yet another try at an xp system that doesn't suck - Samson 12-22-02 */
/* Updated to 18 for the reorganized format - Samson 5-16-04 */
/* 19 skipped */
/* Updated to 20: Starting version for official support of AFKMud 2.0 pfiles */
/* Updated to 21 because Samson was stupid and acted hastily before finalizing the bitset conversions 7-8-04 */
/* Updated to 22 for sha256 password conversion */
// Updated to 23 - Site data in the save requires the tilde now which pfiles won't have yet.

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
            mob_save_equipment[x][y] = NULL;
         else
            save_equipment[x][y] = NULL;
      }
   }

   list < obj_data * >::iterator iobj;
   for( iobj = carrying.begin(  ); iobj != carrying.end(  ); ++iobj )
   {
      obj_data *obj = *iobj;

      if( obj->wear_loc > -1 && obj->wear_loc < MAX_WEAR )
      {
         if( char_ego(  ) >= obj->ego )
         {
            for( int x = 0; x < MAX_LAYERS; ++x )
            {
               if( x == MAX_LAYERS )
               {
                  bug( "%s: %s had on more than %d layers of clothing in one location (%d): %s", __func__, name, MAX_LAYERS, obj->wear_loc, obj->name );
                  break;
               }

               if( isnpc(  ) )
               {
                  if( !mob_save_equipment[obj->wear_loc][x] )
                  {
                     mob_save_equipment[obj->wear_loc][x] = obj;
                     break;
                  }
               }
               else
               {
                  if( !save_equipment[obj->wear_loc][x] )
                  {
                     save_equipment[obj->wear_loc][x] = obj;
                     break;
                  }
               }
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
            if( mob_save_equipment[x][y] != NULL )
            {
               if( quitting_char != this )
                  equip( mob_save_equipment[x][y], x );
               mob_save_equipment[x][y] = NULL;
            }
            else
               break;
         }
         else
         {
            if( save_equipment[x][y] != NULL )
            {
               if( quitting_char != this )
                  equip( save_equipment[x][y], x );
               save_equipment[x][y] = NULL;
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
void fwrite_char( char_data * ch, FILE * fp )
{
   short pos;

   if( ch->isnpc(  ) )
   {
      bug( "%s: NPC save called!", __func__ );
      return;
   }

   fprintf( fp, "%s", "#PLAYER\n" );
   fprintf( fp, "Version      %d\n", SAVEVERSION );
   fprintf( fp, "Name         %s~\n", ch->name );
   fprintf( fp, "Password     %s~\n", ch->pcdata->pwd );
   if( ch->chardesc && ch->chardesc[0] != '\0' )
      fprintf( fp, "Description  %s~\n", strip_cr( ch->chardesc ) );
   fprintf( fp, "Sex          %s~\n", npc_sex[ch->sex] );
   fprintf( fp, "Race         %s~\n", npc_race[ch->race] );
   fprintf( fp, "Class        %s~\n", npc_class[ch->Class] );
   if( ch->pcdata->title && ch->pcdata->title[0] != '\0' )
      fprintf( fp, "Title        %s~\n", ch->pcdata->title );
   if( ch->pcdata->rank && ch->pcdata->rank[0] != '\0' )
      fprintf( fp, "Rank         %s~\n", ch->pcdata->rank );
   if( !ch->pcdata->bestowments.empty(  ) )
      fprintf( fp, "Bestowments  %s~\n", ch->pcdata->bestowments.c_str(  ) );
   if( !ch->pcdata->homepage.empty(  ) )
      fprintf( fp, "Homepage     %s~\n", ch->pcdata->homepage.c_str(  ) );
   if( !ch->pcdata->email.empty(  ) )  /* Samson 4-19-98 */
      fprintf( fp, "Email        %s~\n", ch->pcdata->email.c_str(  ) );
   fprintf( fp, "Site         %s~\n", ch->pcdata->lasthost.c_str(  ) );
   if( ch->pcdata->bio && ch->pcdata->bio[0] != '\0' )
      fprintf( fp, "Bio          %s~\n", strip_cr( ch->pcdata->bio ) );
   if( !ch->pcdata->authed_by.empty(  ) )
      fprintf( fp, "AuthedBy     %s~\n", ch->pcdata->authed_by.c_str(  ) );
   if( ch->pcdata->prompt && ch->pcdata->prompt[0] != '\0' )
      fprintf( fp, "Prompt       %s~\n", ch->pcdata->prompt );
   if( ch->pcdata->fprompt && ch->pcdata->fprompt[0] != '\0' )
      fprintf( fp, "FPrompt      %s~\n", ch->pcdata->fprompt );
   if( !ch->pcdata->deity_name.empty(  ) )
      fprintf( fp, "Deity        %s~\n", ch->pcdata->deity_name.c_str(  ) );
   if( !ch->pcdata->clan_name.empty(  ) )
      fprintf( fp, "Clan         %s~\n", ch->pcdata->clan_name.c_str(  ) );
   if( ch->has_pcflags(  ) )
      fprintf( fp, "PCFlags      %s~\n", bitset_string( ch->get_pcflags(  ), pc_flags ) );
   if( !ch->pcdata->chan_listen.empty(  ) )
      fprintf( fp, "Channels     %s~\n", ch->pcdata->chan_listen.c_str(  ) );
   if( ch->has_langs(  ) )
      fprintf( fp, "Speaks       %s~\n", bitset_string( ch->get_langs(  ), lang_names ) );
   if( ch->speaking )
      fprintf( fp, "Speaking     %s~\n", lang_names[ch->speaking] );
   if( ch->pcdata->release_date )
      fprintf( fp, "Helled       %ld %s~\n", ch->pcdata->release_date, ch->pcdata->helled_by );
   fprintf( fp, "Status       %d %d %d %d %d %d %d\n", ch->level, ch->gold, ch->exp, ch->height, ch->weight, ch->spellfail, ch->mental_state );
   fprintf( fp, "Status2      %d %d %d %d %d %d %d %d\n", ch->style, ch->pcdata->practice, ch->alignment, ch->pcdata->favor, ch->hitroll, ch->damroll, ch->armor, ch->wimpy );
   fprintf( fp, "Configs      %d %d %d %d %d %d 0 %d\n", ch->pcdata->pagerlen, -1, -1, -1, ch->pcdata->timezone, ch->wait,
            ( ch->in_room == get_room_index( ROOM_VNUM_LIMBO ) && ch->was_in_room ) ? ch->was_in_room->vnum : ch->in_room->vnum );

   /*
    * MOTD times - Samson 12-31-00 
    */
   fprintf( fp, "Motd         %ld %ld\n", ch->pcdata->motd, ch->pcdata->imotd );

   fprintf( fp, "Age          %d %d %d %d %ld\n",
            ch->pcdata->age_bonus, ch->pcdata->day, ch->pcdata->month, ch->pcdata->year, ch->pcdata->played + ( current_time - ch->pcdata->logon ) );

   fprintf( fp, "HpManaMove   %d %d %d %d %d %d\n", ch->hit, ch->max_hit, ch->mana, ch->max_mana, ch->move, ch->max_move );
   fprintf( fp, "Regens       %d %d %d\n", ch->hit_regen, ch->mana_regen, ch->move_regen );

   pos = ch->position;
   if( pos > POS_SITTING && pos < POS_STANDING )
      pos = POS_STANDING;
   fprintf( fp, "Position     %s~\n", npc_position[pos] );
   /*
    * Overland Map - Samson 7-31-99 
    */
   fprintf( fp, "Coordinates  %d %d %d\n", ch->mx, ch->my, ch->wmap );
   fprintf( fp, "SavingThrows %d %d %d %d %d\n", ch->saving_poison_death, ch->saving_wand, ch->saving_para_petri, ch->saving_breath, ch->saving_spell_staff );
   fprintf( fp, "RentData     %d %d 0 0 %d\n", ch->pcdata->balance, ch->pcdata->daysidle, ch->pcdata->camp );
   /*
    * Recall code update to recall to last inn rented at - Samson 12-20-00 
    */
   fprintf( fp, "RentRooms    %d\n", ch->pcdata->one );

   if( ch->has_bparts( )  )
      fprintf( fp, "BodyParts    %s~\n", bitset_string( ch->get_bparts(  ), part_flags ) );
   if( ch->has_aflags(  ) )
      fprintf( fp, "AffectFlags  %s~\n", bitset_string( ch->get_aflags(  ), aff_flags ) );
   if( ch->has_noaflags(  ) )
      fprintf( fp, "NoAffectedBy %s~\n", bitset_string( ch->get_noaflags(  ), aff_flags ) );
   if( ch->has_resists(  ) )
      fprintf( fp, "Resistant    %s~\n", bitset_string( ch->get_resists(  ), ris_flags ) );
   if( ch->has_noresists(  ) )
      fprintf( fp, "Nores        %s~\n", bitset_string( ch->get_noresists(  ), ris_flags ) );
   if( ch->has_susceps(  ) )
      fprintf( fp, "Susceptible  %s~\n", bitset_string( ch->get_susceps(  ), ris_flags ) );
   if( ch->has_nosusceps(  ) )
      fprintf( fp, "Nosusc       %s~\n", bitset_string( ch->get_nosusceps(  ), ris_flags ) );
   if( ch->has_immunes(  ) )
      fprintf( fp, "Immune       %s~\n", bitset_string( ch->get_immunes(  ), ris_flags ) );
   if( ch->has_noimmunes(  ) )
      fprintf( fp, "Noimm        %s~\n", bitset_string( ch->get_noimmunes(  ), ris_flags ) );
   if( ch->has_absorbs(  ) )
      fprintf( fp, "Absorb       %s~\n", bitset_string( ch->get_absorbs(  ), ris_flags ) );

   fprintf( fp, "KillInfo     %d %d %d %d %d\n", ch->pcdata->pkills, ch->pcdata->pdeaths, ch->pcdata->mkills, ch->pcdata->mdeaths, ch->pcdata->illegal_pk );

   if( ch->get_timer( TIMER_PKILLED ) && ( ch->get_timer( TIMER_PKILLED ) > 0 ) )
      fprintf( fp, "PTimer       %d\n", ch->get_timer( TIMER_PKILLED ) );

   fprintf( fp, "AttrPerm     %d %d %d %d %d %d %d\n", ch->perm_str, ch->perm_int, ch->perm_wis, ch->perm_dex, ch->perm_con, ch->perm_cha, ch->perm_lck );

   fprintf( fp, "AttrMod      %d %d %d %d %d %d %d\n", ch->mod_str, ch->mod_int, ch->mod_wis, ch->mod_dex, ch->mod_con, ch->mod_cha, ch->mod_lck );

   fprintf( fp, "Condition    %d %d %d\n", ch->pcdata->condition[0], ch->pcdata->condition[1], ch->pcdata->condition[2] );

   if( ch->is_immortal(  ) )
   {
      if( ch->pcdata->realm && !ch->pcdata->realm_name.empty() )
         fprintf( fp, "ImmRealm     %s~\n", ch->pcdata->realm_name.c_str() );
      if( ch->pcdata->bamfin && ch->pcdata->bamfin[0] != '\0' )
         fprintf( fp, "Bamfin       %s~\n", ch->pcdata->bamfin );
      if( ch->pcdata->bamfout && ch->pcdata->bamfout[0] != '\0' )
         fprintf( fp, "Bamfout      %s~\n", ch->pcdata->bamfout );
      fprintf( fp, "ImmData      %d %ld %d %d %d\n",
               ch->trust, ch->pcdata->restore_time, ch->pcdata->wizinvis, ch->pcdata->low_vnum, ch->pcdata->hi_vnum );
   }

   for( int sn = 1; sn < num_skills; ++sn )
   {
      if( skill_table[sn]->name && ch->pcdata->learned[sn] > 0 )
      {
         switch ( skill_table[sn]->type )
         {
            default:
               fprintf( fp, "Skill        %d '%s'\n", ch->pcdata->learned[sn], skill_table[sn]->name );
               break;

            case SKILL_SPELL:
               fprintf( fp, "Spell        %d '%s'\n", ch->pcdata->learned[sn], skill_table[sn]->name );
               break;

            case SKILL_COMBAT:
               fprintf( fp, "Combat       %d '%s'\n", ch->pcdata->learned[sn], skill_table[sn]->name );
               break;

            case SKILL_TONGUE:
               fprintf( fp, "Tongue       %d '%s'\n", ch->pcdata->learned[sn], skill_table[sn]->name );
               break;

            case SKILL_RACIAL:
               fprintf( fp, "Ability      %d '%s'\n", ch->pcdata->learned[sn], skill_table[sn]->name );
               break;

            case SKILL_LORE:
               fprintf( fp, "Lore         %d '%s'\n", ch->pcdata->learned[sn], skill_table[sn]->name );
         }
      }
   }

   list < affect_data * >::iterator paf;
   for( paf = ch->affects.begin(  ); paf != ch->affects.end(  ); ++paf )
   {
      affect_data *af = *paf;
      skill_type *skill = NULL;

      if( af->type >= 0 && !( skill = get_skilltype( af->type ) ) )
         continue;

      if( af->type >= 0 && af->type < TYPE_PERSONAL )
         fprintf( fp, "AffectData   '%s' %3d %3d %3d %d\n", skill->name, af->duration, af->modifier, af->location, af->bit );
      else
      {
         if( af->location == APPLY_AFFECT )
            fprintf( fp, "Affect %s '%s' %d %d %d\n", a_types[af->location], aff_flags[af->modifier], af->type, af->duration, af->bit );
         else if( af->location == APPLY_WEAPONSPELL
                  || af->location == APPLY_WEARSPELL
                  || af->location == APPLY_REMOVESPELL || af->location == APPLY_STRIPSN || af->location == APPLY_RECURRINGSPELL || af->location == APPLY_EAT_SPELL )
            fprintf( fp, "Affect %s '%s' %d %d %d\n", a_types[af->location],
                     IS_VALID_SN( af->modifier ) ? skill_table[af->modifier]->name : "UNKNOWN", af->type, af->duration, af->bit );
         else if( af->location == APPLY_RESISTANT || af->location == APPLY_IMMUNE || af->location == APPLY_SUSCEPTIBLE || af->location == APPLY_ABSORB )
            fprintf( fp, "Affect %s %s~ %d %d %d\n", a_types[af->location], bitset_string( af->rismod, ris_flags ), af->type, af->duration, af->bit );
         else
            fprintf( fp, "Affect %s %d %d %d %d\n", a_types[af->location], af->modifier, af->type, af->duration, af->bit );
      }
   }

   /*
    * Save color values - Samson 9-29-98 
    */
   fprintf( fp, "MaxColors    %d\n", MAX_COLORS );
   fprintf( fp, "%s", "Colors      " );
   for( int x = 0; x < MAX_COLORS; ++x )
      fprintf( fp, " %d", ch->pcdata->colors[x] );
   fprintf( fp, "%s", "\n" );

   /*
    * Save recall beacons - Samson 2-7-99 
    */
   fprintf( fp, "MaxBeacons   %d\n", MAX_BEACONS );
   fprintf( fp, "%s", "Beacons     " );
   for( int x = 0; x < MAX_BEACONS; ++x )
      fprintf( fp, " %d", ch->pcdata->beacon[x] );
   fprintf( fp, "%s", "\n" );

   map < string, string >::iterator pal;
   for( pal = ch->pcdata->alias_map.begin(  ); pal != ch->pcdata->alias_map.end(  ); ++pal )
   {
      if( pal->second.empty(  ) )
         continue;
      fprintf( fp, "Alias        %s~ %s~\n", pal->first.c_str(  ), pal->second.c_str(  ) );
   }

   if( !ch->pcdata->boarddata.empty(  ) )
   {
      list < board_chardata * >::iterator bd;

      for( bd = ch->pcdata->boarddata.begin(  ); bd != ch->pcdata->boarddata.end(  ); )
      {
         board_chardata *chbd = *bd;
         ++bd;

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
         fprintf( fp, "Board_Data   %s~ %ld %d\n", chbd->board_name.c_str(  ), chbd->last_read, chbd->alert );
      }
   }

   ch->pcdata->save_zonedata( fp );
   ch->pcdata->save_ignores( fp );

   if( !ch->pcdata->qbits.empty(  ) )
   {
      map < int, string >::iterator bit;

      for( bit = ch->pcdata->qbits.begin(  ); bit != ch->pcdata->qbits.end(  ); ++bit )
         fprintf( fp, "Qbit         %d %s~\n", bit->first, bit->second.c_str(  ) );
   }
#ifdef IMC
   imc_savechar( ch, fp );
#endif
   fprintf( fp, "%s", "End\n\n" );
}

/*
 * Write an object list, with contents.
 */
void fwrite_obj( char_data * ch, list < obj_data * >source, clan_data * clan, FILE * fp, int iNest, bool hotboot )
{
   if( iNest >= MAX_NEST )
   {
      bug( "%s: iNest hit MAX_NEST %d", __func__, iNest );
      return;
   }

   list < obj_data * >::iterator iobj;
   for( iobj = source.begin(  ); iobj != source.end(  ); ++iobj )
   {
      obj_data *obj = *iobj;

      if( !obj )
      {
         bug( "%s: NULL obj", __func__ );
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
      fprintf( fp, "%s", "#OBJECT\n" );
      fprintf( fp, "Version      %d\n", SAVEVERSION );
      if( iNest )
         fprintf( fp, "Nest         %d\n", iNest );
      if( obj->count > 1 )
         fprintf( fp, "Count        %d\n", obj->count );
      if( obj->name && obj->pIndexData->name && str_cmp( obj->name, obj->pIndexData->name ) )
         fprintf( fp, "Name         %s~\n", obj->name );
      if( obj->short_descr && obj->pIndexData->short_descr && str_cmp( obj->short_descr, obj->pIndexData->short_descr ) )
         fprintf( fp, "ShortDescr   %s~\n", obj->short_descr );
      if( obj->objdesc && obj->pIndexData->objdesc && str_cmp( obj->objdesc, obj->pIndexData->objdesc ) )
         fprintf( fp, "Description  %s~\n", obj->objdesc );
      if( obj->action_desc && obj->pIndexData->action_desc && str_cmp( obj->action_desc, obj->pIndexData->action_desc ) )
         fprintf( fp, "ActionDesc   %s~\n", obj->action_desc );
      fprintf( fp, "Ovnum        %d\n", obj->pIndexData->vnum );
      fprintf( fp, "Ego          %d\n", obj->ego );
      if( hotboot && obj->in_room )
      {
         fprintf( fp, "Room         %d\n", obj->in_room->vnum );
         fprintf( fp, "Rvnum        %d\n", obj->room_vnum );
      }
      if( obj->extra_flags != obj->pIndexData->extra_flags )
         fprintf( fp, "ExtraFlags   %s~\n", bitset_string( obj->extra_flags, o_flags ) );
      if( obj->wear_flags != obj->pIndexData->wear_flags )
         fprintf( fp, "WearFlags    %s~\n", bitset_string( obj->wear_flags, w_flags ) );

      short wear, wear_loc = -1, x;
      if( ch )
      {
         for( wear = 0; wear < MAX_WEAR; ++wear )
         {
            for( x = 0; x < MAX_LAYERS; ++x )
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
         fprintf( fp, "WearLoc      %d\n", wear_loc );
      if( obj->item_type != obj->pIndexData->item_type )
         fprintf( fp, "ItemType     %d\n", obj->item_type );
      if( obj->weight != obj->pIndexData->weight )
         fprintf( fp, "Weight       %d\n", obj->weight );
      if( obj->level )
         fprintf( fp, "Level        %d\n", obj->level );
      if( obj->timer )
         fprintf( fp, "Timer        %d\n", obj->timer );
      if( obj->cost != obj->pIndexData->cost )
         fprintf( fp, "Cost         %d\n", obj->cost );
      if( obj->seller != NULL )
         fprintf( fp, "Seller       %s~\n", obj->seller );
      if( obj->buyer != NULL )
         fprintf( fp, "Buyer        %s~\n", obj->buyer );
      if( obj->bid != 0 )
         fprintf( fp, "Bid          %d\n", obj->bid );
      if( obj->owner != NULL )
         fprintf( fp, "Owner        %s~\n", obj->owner );
      fprintf( fp, "Oday         %d\n", obj->day );
      fprintf( fp, "Omonth       %d\n", obj->month );
      fprintf( fp, "Oyear        %d\n", obj->year );
      fprintf( fp, "Coords       %d %d %d\n", obj->mx, obj->my, obj->wmap );
      fprintf( fp, "%s", "Values      " );
      for( x = 0; x < MAX_OBJ_VALUE; ++x )
         fprintf( fp, " %d", obj->value[x] );
      fprintf( fp, "%s", "\n" );
      fprintf( fp, "Sockets      %s %s %s\n", obj->socket[0] ? obj->socket[0] : "None", obj->socket[1] ? obj->socket[1] : "None", obj->socket[2] ? obj->socket[2] : "None" );

      switch ( obj->item_type )
      {
         default:
            break;

         case ITEM_PILL:  /* was down there with staff and wand, wrongly - Scryn */
         case ITEM_POTION:
         case ITEM_SCROLL:
            if( IS_VALID_SN( obj->value[1] ) )
               fprintf( fp, "Spell 1      '%s'\n", skill_table[obj->value[1]]->name );
            if( IS_VALID_SN( obj->value[2] ) )
               fprintf( fp, "Spell 2      '%s'\n", skill_table[obj->value[2]]->name );
            if( IS_VALID_SN( obj->value[3] ) )
               fprintf( fp, "Spell 3      '%s'\n", skill_table[obj->value[3]]->name );
            break;

         case ITEM_STAFF:
         case ITEM_WAND:
            if( IS_VALID_SN( obj->value[3] ) )
               fprintf( fp, "Spell 3      '%s'\n", skill_table[obj->value[3]]->name );
            break;

         case ITEM_SALVE:
            if( IS_VALID_SN( obj->value[4] ) )
               fprintf( fp, "Spell 4      '%s'\n", skill_table[obj->value[4]]->name );
            if( IS_VALID_SN( obj->value[5] ) )
               fprintf( fp, "Spell 5      '%s'\n", skill_table[obj->value[5]]->name );
            break;
      }

      list < affect_data * >::iterator paf;
      for( paf = obj->affects.begin(  ); paf != obj->affects.end(  ); ++paf )
      {
         affect_data *af = *paf;

         /*
          * Save extra object affects - Thoric
          */
         if( af->type < 0 || af->type >= num_skills )
         {
            fprintf( fp, "Affect       %d %d %d %d %d\n",
                     af->type, af->duration,
                     ( ( af->location == APPLY_WEAPONSPELL
                         || af->location == APPLY_WEARSPELL
                         || af->location == APPLY_REMOVESPELL
                         || af->location == APPLY_STRIPSN
                         || af->location == APPLY_RECURRINGSPELL ) && IS_VALID_SN( af->modifier ) ) ? skill_table[af->modifier]->slot : af->modifier, af->location, af->bit );
         }
         else
            fprintf( fp, "AffectData   '%s' %d %d %d %d\n",
                     skill_table[af->type]->name, af->duration,
                     ( ( af->location == APPLY_WEAPONSPELL
                         || af->location == APPLY_WEARSPELL
                         || af->location == APPLY_REMOVESPELL
                         || af->location == APPLY_STRIPSN
                         || af->location == APPLY_RECURRINGSPELL ) && IS_VALID_SN( af->modifier ) ) ? skill_table[af->modifier]->slot : af->modifier, af->location, af->bit );
      }

      list < extra_descr_data * >::iterator ed;
      for( ed = obj->extradesc.begin(  ); ed != obj->extradesc.end(  ); ++ed )
      {
         extra_descr_data *desc = *ed;

         if( !desc->desc.empty(  ) )
            fprintf( fp, "ExtraDescr   %s~ %s~\n", desc->keyword.c_str(  ), desc->desc.c_str(  ) );
         else
            fprintf( fp, "ExtraDescr   %s~ ~\n", desc->keyword.c_str(  ) );
      }

      fprintf( fp, "%s", "End\n\n" );

      if( !obj->contents.empty(  ) )
         fwrite_obj( ch, obj->contents, clan, fp, iNest + 1, hotboot );
   }
}

/*
 * This will write one mobile structure pointed to be fp --Shaddai
 *   Edited by Tarl 5 May 2002 to allow pets to save items.
*/
void fwrite_mobile( char_data * mob, FILE * fp, bool shopmob )
{
   if( !mob->isnpc(  ) || !fp )
      return;

   fprintf( fp, "%s", shopmob ? "#SHOP\n" : "#MOBILE\n" );
   fprintf( fp, "Vnum    %d\n", mob->pIndexData->vnum );
   fprintf( fp, "Level   %d\n", mob->level );
   fprintf( fp, "Gold	 %d\n", mob->gold );
   if( mob->in_room )
      fprintf( fp, "Room      %d\n", mob->in_room->vnum );
   else
      fprintf( fp, "Room      %d\n", ROOM_VNUM_ALTAR );
   fprintf( fp, "Coordinates  %d %d %d\n", mob->mx, mob->my, mob->wmap );
   if( mob->name && mob->pIndexData->player_name && str_cmp( mob->name, mob->pIndexData->player_name ) )
      fprintf( fp, "Name     %s~\n", mob->name );
   if( mob->short_descr && mob->pIndexData->short_descr && str_cmp( mob->short_descr, mob->pIndexData->short_descr ) )
      fprintf( fp, "Short	%s~\n", mob->short_descr );
   if( mob->long_descr && mob->pIndexData->long_descr && str_cmp( mob->long_descr, mob->pIndexData->long_descr ) )
      fprintf( fp, "Long	%s~\n", mob->long_descr );
   if( mob->chardesc && mob->pIndexData->chardesc && str_cmp( mob->chardesc, mob->pIndexData->chardesc ) )
      fprintf( fp, "Description %s~\n", mob->chardesc );
   fprintf( fp, "Position        %d\n", mob->position );
   if( mob->has_actflag( ACT_MOUNTED ) )
      mob->unset_actflag( ACT_MOUNTED );
   if( mob->has_actflags(  ) )
      fprintf( fp, "ActFlags     %s~\n", bitset_string( mob->get_actflags(  ), act_flags ) );
   if( mob->has_aflags(  ) )
      fprintf( fp, "AffectedBy   %s~\n", bitset_string( mob->get_aflags(  ), aff_flags ) );

   list < affect_data * >::iterator paf;
   for( paf = mob->affects.begin(  ); paf != mob->affects.end(  ); ++paf )
   {
      affect_data *af = *paf;
      skill_type *skill = NULL;

      if( af->type >= 0 && !( skill = get_skilltype( af->type ) ) )
         continue;

      if( af->type >= 0 && af->type < TYPE_PERSONAL )
         fprintf( fp, "AffectData   '%s' %3d %3d %3d %d\n", skill->name, af->duration, af->modifier, af->location, af->bit );
      else
         fprintf( fp, "Affect       %3d %3d %3d %3d %d\n", af->type, af->duration, af->modifier, af->location, af->bit );
   }
   fprintf( fp, "HpManaMove   %d %d %d %d %d %d\n", mob->hit, mob->max_hit, mob->mana, mob->max_mana, mob->move, mob->max_move );
   fprintf( fp, "Exp          %d\n", mob->exp );
   if( !mob->carrying.empty(  ) )
   {
      list < obj_data * >::iterator iobj;

      for( iobj = mob->carrying.begin(  ); iobj != mob->carrying.end(  ); )
      {
         obj_data *obj = *iobj;
         ++iobj;

         if( obj->ego >= sysdata->minego )
            obj->extract(  );
      }
   }

   if( shopmob )
      fprintf( fp, "%s", "EndMobile\n\n" );

   mob->de_equip(  );
   if( !mob->carrying.empty(  ) )
      fwrite_obj( mob, mob->carrying, NULL, fp, 0, ( shopmob ? false : mob->master->pcdata->hotboot ) );
   mob->re_equip(  );

   if( !shopmob )
      fprintf( fp, "%s", "EndMobile\n\n" );
}

/*
 * Save a character and inventory.
 * Would be cool to save NPC's too for quest purposes, some of the infrastructure is provided.
 */
void char_data::save(  )
{
   char strsave[256], strback[256];
   FILE *fp;

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
   snprintf( strsave, 256, "%s%c/%s", PLAYER_DIR, tolower( pcdata->filename[0] ), capitalize( pcdata->filename ) );

   /*
    * Save immortal stats, level & vnums for wizlist     -Thoric
    * and do_vnums command
    *
    * Also save the player flags so we the wizlist builder can see
    * who is a guest and who is retired.
    */
   if( is_immortal(  ) )
   {
      snprintf( strback, 256, "%s%s", GOD_DIR, capitalize( pcdata->filename ) );

      if( !( fp = fopen( strback, "w" ) ) )
      {
         perror( strback );
         bug( "%s: fopen", __func__ );
      }
      else
      {
         fprintf( fp, "Level        %d\n", level );
         fprintf( fp, "Pcflags      %s~\n", bitset_string( pcdata->flags, pc_flags ) );
         if( !pcdata->homepage.empty(  ) )
            fprintf( fp, "Homepage     %s~\n", pcdata->homepage.c_str(  ) );
         if( pcdata->realm && !pcdata->realm_name.empty() )
            fprintf( fp, "ImmRealm     %s~\n", pcdata->realm_name.c_str() );
         if( pcdata->low_vnum && pcdata->hi_vnum )
            fprintf( fp, "VnumRange    %d %d\n", pcdata->low_vnum, pcdata->hi_vnum );
         if( !pcdata->email.empty(  ) )
            fprintf( fp, "Email        %s~\n", pcdata->email.c_str(  ) );
         fprintf( fp, "%s", "End\n" );
         FCLOSE( fp );
      }
   }

   if( !( fp = fopen( strsave, "w" ) ) )
   {
      bug( "%s: fopen", __func__ );
      perror( strsave );
   }
   else
   {
      fwrite_char( this, fp );
      if( morph )
         fwrite_morph_data( this, fp );
      if( !carrying.empty(  ) )
         fwrite_obj( this, carrying, NULL, fp, 0, pcdata->hotboot );

      if( sysdata->save_pets && !pets.empty(  ) )
      {
         list < char_data * >::iterator ipet;

         for( ipet = pets.begin(  ); ipet != pets.end(  ); ++ipet )
         {
            char_data *pet = *ipet;

            fwrite_mobile( pet, fp, false );
         }
      }
      fwrite_variables( this, fp );
      pcdata->fwrite_comments( fp );
      fprintf( fp, "%s", "#END\n" );
      FCLOSE( fp );
   }

   ClassSpecificStuff(  ); /* Brought over from DOTD code - Samson 4-6-99 */

   re_equip(  );

   quitting_char = NULL;
   saving_char = NULL;
}

short find_old_age( char_data * ch )
{
   short age;

   if( ch->isnpc(  ) )
      return -1;

   age = ch->pcdata->played / 86400;   /* Calculate realtime number of days played */

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
void fread_char( char_data * ch, FILE * fp, bool preload, bool copyover )
{
   const char *line;
   int x1, x2, x3, x4, x5, x6, x7, x8;
   int max_colors = 0;  /* Color code */
   int max_beacons = 0; /* Beacon spell */

   file_ver = 0;

   do
   {
      const char *word = ( feof( fp ) ? "End" : fread_word( fp ) );

      if( word[0] == '\0' )
      {
         log_printf( "%s: EOF encountered reading file!", __func__ );
         word = "End";
      }

      switch ( UPPER( word[0] ) )
      {
         default:
            log_printf( "%s: no match: %s", __func__, word );
            fread_to_eol( fp );
            break;

         case '*':
            fread_to_eol( fp );
            break;

         case 'A':
            if( !str_cmp( word, "Ability" ) )
            {
               int value = fread_number( fp );
               char *ability = fread_word( fp );
               int sn = find_ability( NULL, ability, false );

               if( sn < 0 )
                  log_printf( "%s: unknown ability: %s", __func__, ability );
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
               break;
            }

            if( !str_cmp( word, "Absorb" ) )
            {
               ch->set_file_absorbs( fp );
               break;
            }

            if( !str_cmp( word, "AffectFlags" ) )
            {
               ch->set_file_aflags( fp );
               break;
            }

            if( !str_cmp( word, "Age" ) )
            {
               time_t xx5 = 0;
               line = fread_line( fp );
               x1 = x2 = x3 = x4 = 0;
               sscanf( line, "%d %d %d %d %ld", &x1, &x2, &x3, &x4, &xx5 );
               ch->pcdata->age_bonus = x1;
               ch->pcdata->day = x2;
               ch->pcdata->month = x3;
               ch->pcdata->year = x4;
               ch->pcdata->played = xx5;
               break;
            }

            if( !str_cmp( word, "Affect" ) || !str_cmp( word, "AffectData" ) )
            {
               affect_data *paf;

               if( !str_cmp( word, "AffData" ) )
                  paf = fread_afk_affect( fp );
               else
               {
                  paf = new affect_data;
                  paf->type = -1;
                  paf->duration = -1;
                  paf->bit = 0;
                  paf->modifier = 0;
                  paf->rismod.reset(  );

                  int sn;
                  char *sname = fread_word( fp );

                  if( ( sn = skill_lookup( sname ) ) < 0 )
                  {
                     if( ( sn = herb_lookup( sname ) ) < 0 )
                        log_printf( "%s: unknown skill.", __func__ );
                     else
                        sn += TYPE_HERB;
                  }
                  paf->type = sn;
                  paf->duration = fread_number( fp );
                  paf->modifier = fread_number( fp );
                  paf->location = fread_number( fp );
                  if( paf->location == APPLY_WEAPONSPELL
                      || paf->location == APPLY_WEARSPELL || paf->location == APPLY_REMOVESPELL || paf->location == APPLY_STRIPSN || paf->location == APPLY_RECURRINGSPELL )
                     paf->modifier = slot_lookup( paf->modifier );
                  paf->bit = fread_number( fp );
                  if( paf->bit >= MAX_AFFECTED_BY )
                  {
                     deleteptr( paf );
                     break;
                  }
               }
               ch->affects.push_back( paf );
               break;
            }

            if( !str_cmp( word, "AttrMod" ) )
            {
               line = fread_line( fp );
               x1 = x2 = x3 = x4 = x5 = x6 = x7 = 13;
               sscanf( line, "%d %d %d %d %d %d %d", &x1, &x2, &x3, &x4, &x5, &x6, &x7 );
               ch->mod_str = x1;
               ch->mod_int = x2;
               ch->mod_wis = x3;
               ch->mod_dex = x4;
               ch->mod_con = x5;
               ch->mod_cha = x6;
               ch->mod_lck = x7;
               if( !x7 )
                  ch->mod_lck = 0;
               break;
            }

            if( !str_cmp( word, "Alias" ) )
            {
               string alias, cmd;

               fread_string( alias, fp );
               fread_string( cmd, fp );
               ch->pcdata->alias_map[alias] = cmd;
               break;
            }

            if( !str_cmp( word, "AttrPerm" ) )
            {
               line = fread_line( fp );
               x1 = x2 = x3 = x4 = x5 = x6 = x7 = 0;
               sscanf( line, "%d %d %d %d %d %d %d", &x1, &x2, &x3, &x4, &x5, &x6, &x7 );
               ch->perm_str = x1;
               ch->perm_int = x2;
               ch->perm_wis = x3;
               ch->perm_dex = x4;
               ch->perm_con = x5;
               ch->perm_cha = x6;
               ch->perm_lck = x7;
               if( x7 == 0 )
                  ch->perm_lck = 13;
               break;
            }
            STDSKEY( "AuthedBy", ch->pcdata->authed_by );
            break;

         case 'B':
            KEY( "Bamfin", ch->pcdata->bamfin, fread_string_nohash( fp ) );
            KEY( "Bamfout", ch->pcdata->bamfout, fread_string_nohash( fp ) );

            /*
             * Load beacons - Samson 9-29-98 
             */
            if( !str_cmp( word, "Beacons" ) )
            {
               for( int x = 0; x < max_beacons; ++x )
                  ch->pcdata->beacon[x] = fread_number( fp );
               break;
            }

            STDSKEY( "Bestowments", ch->pcdata->bestowments );
            KEY( "Bio", ch->pcdata->bio, fread_string_nohash( fp ) );

            if( !str_cmp( word, "Board_Data" ) )
            {
               board_chardata *pboard;
               board_data *board;

               word = fread_flagstring( fp );
               if( !( board = get_board( NULL, word ) ) )
               {
                  log_printf( "Player %s has board %s which apparently doesn't exist?", ch->name, word );
                  ch->printf( "Warning: the board %s no longer exsists.\r\n", word );
                  fread_to_eol( fp );
                  break;
               }
               pboard = new board_chardata;
               pboard->board_name = board->name;
               pboard->last_read = fread_long( fp );
               pboard->alert = fread_number( fp );
               ch->pcdata->boarddata.push_back( pboard );
               break;
            }

            if( !str_cmp( word, "BodyParts" ) )
            {
               ch->set_file_bparts( fp );
               break;
            }
            break;

         case 'C':
            STDSKEY( "Channels", ch->pcdata->chan_listen );
            STDSKEY( "Clan", ch->pcdata->clan_name );

            if( !str_cmp( word, "Class" ) )
            {
               int Class = get_npc_class( fread_flagstring( fp ) );

               if( Class < 0 || Class >= MAX_NPC_CLASS )
               {
                  bug( "%s: Player %s has invalid Class! Defaulting to warrior.", __func__, ch->name );
                  Class = CLASS_WARRIOR;
               }
               ch->Class = Class;
               break;
            }

            if( !str_cmp( word, "Combat" ) )
            {
               int value = fread_number( fp );
               char *combat = fread_word( fp );
               int sn = find_combat( NULL, combat, false );

               if( sn < 0 )
                  log_printf( "%s: unknown combat skill: %s", __func__, combat );
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
               break;
            }

            if( !str_cmp( word, "Condition" ) )
            {
               line = fread_line( fp );
               sscanf( line, "%d %d %d", &x1, &x2, &x3 );
               ch->pcdata->condition[0] = x1;
               ch->pcdata->condition[1] = x2;
               ch->pcdata->condition[2] = x3;
               break;
            }

            /*
             * Load color values - Samson 9-29-98 
             */
            if( !str_cmp( word, "Colors" ) )
            {
               for( int x = 0; x < max_colors; ++x )
                  ch->pcdata->colors[x] = fread_number( fp );
               break;
            }

            if( !str_cmp( word, "Configs" ) )
            {
               line = fread_line( fp );
               x1 = x2 = x3 = x4 = x5 = x6 = x7 = x8 = 0;
               sscanf( line, "%d %d %d %d %d %d %d %d", &x1, &x2, &x3, &x4, &x5, &x6, &x7, &x8 );
               ch->pcdata->pagerlen = x1;
               ch->pcdata->timezone = x5;
               ch->wait = x6;

               room_index *temp = get_room_index( x8 );

               if( !temp )
                  temp = get_room_index( ROOM_VNUM_TEMPLE );

               if( !temp )
                  temp = get_room_index( ROOM_VNUM_LIMBO );

               /*
                * Um, yeah. If this happens you're shit out of luck! 
                */
               if( !temp )
               {
                  bug( "%s", "FATAL: No valid fallback rooms. Program terminating!" );
                  exit( 1 );
               }
               /*
                * And you're going to crash if the above check failed, because you're an idiot if you remove this Vnum 
                */
               if( temp->flags.test( ROOM_ISOLATED ) )
                  ch->in_room = get_room_index( ROOM_VNUM_TEMPLE );
               else
                  ch->in_room = temp;
               break;
            }

            if( !str_cmp( word, "Coordinates" ) )
            {
               ch->mx = fread_short( fp );
               ch->my = fread_short( fp );
               ch->wmap = fread_short( fp );

               if( !ch->has_pcflag( PCFLAG_ONMAP ) )
               {
                  ch->mx = -1;
                  ch->my = -1;
                  ch->wmap = -1;
               }
               break;
            }
            break;

         case 'D':
            STDSKEY( "Deity", ch->pcdata->deity_name );
            KEY( "Description", ch->chardesc, fread_string( fp ) );
            break;

            /*
             * 'E' was moved to after 'S' 
             */
         case 'F':
            if( !str_cmp( word, "FPrompt" ) )
            {
               STRFREE( ch->pcdata->fprompt );
               ch->pcdata->fprompt = fread_string( fp );
               break;
            }
            break;

         case 'H':
            if( !str_cmp( word, "Helled" ) )
            {
               ch->pcdata->release_date = fread_long( fp );
               ch->pcdata->helled_by = fread_string( fp );
               break;
            }
            STDSKEY( "Homepage", ch->pcdata->homepage );

            if( !str_cmp( word, "HpManaMove" ) )
            {
               line = fread_line( fp );
               x1 = x2 = x3 = x4 = x5 = x6 = 0;
               sscanf( line, "%d %d %d %d %d %d", &x1, &x2, &x3, &x4, &x5, &x6 );
               ch->hit = x1;
               ch->max_hit = x2;
               ch->mana = x3;
               ch->max_mana = x4;
               ch->move = x5;
               ch->max_move = x6;
               break;
            }
            break;

         case 'I':
            if( !str_cmp( word, "ImmData" ) )
            {
               line = fread_line( fp );
               time_t xx2 = 0;
               x1 = x3 = x4 = x5 = 0;
               sscanf( line, "%d %ld %d %d %d", &x1, &xx2, &x3, &x4, &x5 );
               ch->trust = x1;
               ch->pcdata->restore_time = xx2;
               ch->pcdata->wizinvis = x3;
               ch->pcdata->low_vnum = x4;
               ch->pcdata->hi_vnum = x5;
               break;
            }
            STDSKEY( "ImmRealm", ch->pcdata->realm_name );

            if( !str_cmp( word, "Immune" ) )
            {
               ch->set_file_immunes( fp );
               break;
            }

            if( !str_cmp( word, "Ignored" ) )
            {
               ch->pcdata->load_ignores( fp );
               break;
            }
#ifdef IMC
            imc_loadchar( ch, fp, word );
#endif
            break;

         case 'K':
            if( !str_cmp( word, "KillInfo" ) )
            {
               line = fread_line( fp );
               x1 = x2 = x3 = x4 = x5 = 0;
               sscanf( line, "%d %d %d %d %d", &x1, &x2, &x3, &x4, &x5 );
               ch->pcdata->pkills = x1;
               ch->pcdata->pdeaths = x2;
               ch->pcdata->mkills = x3;
               ch->pcdata->mdeaths = x4;
               ch->pcdata->illegal_pk = x5;
               break;
            }
            break;

         case 'L':
            if( !str_cmp( word, "Lore" ) )
            {
               int value = fread_number( fp );
               char *lore = fread_word( fp );
               int sn = find_lore( NULL, lore, false );

               if( sn < 0 )
                  log_printf( "%s: unknown lore: %s", __func__, lore );
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
               break;
            }
            break;

         case 'M':
            KEY( "MaxBeacons", max_beacons, fread_number( fp ) );
            KEY( "MaxColors", max_colors, fread_number( fp ) );
            if( !str_cmp( word, "Motd" ) )
            {
               line = fread_line( fp );
               time_t xx1 = 0, xx2 = 0;
               sscanf( line, "%ld %ld", &xx1, &xx2 );
               ch->pcdata->motd = xx1;
               ch->pcdata->imotd = xx2;
               break;
            }
            break;

         case 'N':
            KEY( "Name", ch->name, fread_string( fp ) );
            if( !str_cmp( word, "NoAffectedBy" ) )
            {
               ch->set_file_noaflags( fp );
               break;
            }

            if( !str_cmp( word, "Nores" ) )
            {
               ch->set_file_noresists( fp );
               break;
            }

            if( !str_cmp( word, "Nosusc" ) )
            {
               ch->set_file_nosusceps( fp );
               break;
            }

            if( !str_cmp( word, "Noimm" ) )
            {
               ch->set_file_noimmunes( fp );
               break;
            }
            break;

         case 'P':
            KEY( "Password", ch->pcdata->pwd, fread_string_nohash( fp ) );

            if( !str_cmp( word, "PCFlags" ) )
            {
               ch->set_file_pcflags( fp );
               break;
            }

            if( !str_cmp( word, "Position" ) )
            {
               int position;

               position = get_npc_position( fread_flagstring( fp ) );

               if( position < 0 || position >= POS_MAX )
               {
                  bug( "%s: Player %s has invalid position! Defaulting to standing.", __func__, ch->name );
                  position = POS_STANDING;
               }
               ch->position = position;
               break;
            }

            if( !str_cmp( word, "Prompt" ) )
            {
               STRFREE( ch->pcdata->prompt );
               ch->pcdata->prompt = fread_string( fp );
               break;
            }

            if( !str_cmp( word, "PTimer" ) )
            {
               ch->add_timer( TIMER_PKILLED, fread_number( fp ), NULL, 0 );
               break;
            }
            break;

         case 'Q':
            if( !str_cmp( word, "Qbit" ) )
            {
               map < int, string >::iterator bit;
               string desc;

               int number = fread_number( fp );
               fread_string( desc, fp );
               if( ( bit = qbits.find( number ) ) != qbits.end(  ) )
               {
                  if( bit->second.empty(  ) )
                     ch->pcdata->qbits[number] = desc;
                  else
                     ch->pcdata->qbits[number] = qbits[number];
               }
               else
                  ch->pcdata->qbits[number] = desc;
               break;
            }
            break;

         case 'R':
            if( !str_cmp( word, "Race" ) )
            {
               int race;

               race = get_npc_race( fread_flagstring( fp ) );

               if( race < 0 || race >= MAX_NPC_RACE )
               {
                  bug( "%s: Player %s has invalid race! Defaulting to human.", __func__, ch->name );
                  race = RACE_HUMAN;
               }
               ch->race = race;
               break;
            }
            KEY( "Rank", ch->pcdata->rank, fread_string( fp ) );

            if( !str_cmp( word, "RentData" ) )
            {
               line = fread_line( fp );
               x1 = x2 = x3 = x4 = x5 = 0;
               sscanf( line, "%d %d %d %d %d", &x1, &x2, &x3, &x4, &x5 );
               ch->pcdata->balance = x1;
               ch->pcdata->daysidle = x2;
               ch->pcdata->camp = x5;
               break;
            }

            if( !str_cmp( word, "RentRooms" ) )
            {
               line = fread_line( fp );
               x1 = ROOM_VNUM_TEMPLE;
               sscanf( line, "%d", &x1 );
               ch->pcdata->one = x1;
               break;
            }

            if( !str_cmp( word, "Regens" ) )
            {
               line = fread_line( fp );
               x1 = x2 = x3 = 0;
               sscanf( line, "%d %d %d", &x1, &x2, &x3 );
               ch->hit_regen = x1;
               ch->mana_regen = x2;
               ch->move_regen = x3;
               break;
            }

            if( !str_cmp( word, "Resistant" ) )
            {
               ch->set_file_resists( fp );
               break;
            }
            break;

         case 'S':
            if( !str_cmp( word, "Sex" ) )
            {
               int sex;

               sex = get_npc_sex( fread_flagstring( fp ) );

               if( sex < 0 || sex >= SEX_MAX )
               {
                  bug( "%s: Player %s has invalid sex! Defaulting to Neuter.", __func__, ch->name );
                  sex = SEX_NEUTRAL;
               }
               ch->sex = sex;
               break;
            }

            if( !str_cmp( word, "SavingThrows" ) )
            {
               line = fread_line( fp );
               x1 = x2 = x3 = x4 = x5 = 0;
               sscanf( line, "%d %d %d %d %d", &x1, &x2, &x3, &x4, &x5 );
               ch->saving_poison_death = x1;
               ch->saving_wand = x2;
               ch->saving_para_petri = x3;
               ch->saving_breath = x4;
               ch->saving_spell_staff = x5;
               break;
            }

            if( !str_cmp( word, "Site" ) )
            {
               if( !copyover && !preload )
               {
                  if( file_ver < 23 )
                     ch->pcdata->prevhost = fread_word( fp );
                  else
                     fread_string( ch->pcdata->prevhost, fp );
                  ch->printf( "Last connected from: %s\r\n", ch->pcdata->prevhost.c_str() );
               }
               else
                  fread_to_eol( fp );
               break;
            }

            if( !str_cmp( word, "Skill" ) )
            {
               int value = fread_number( fp );
               char *skill = fread_word( fp );
               int sn = find_skill( NULL, skill, false );

               if( sn < 0 )
                  log_printf( "%s: unknown skill: %s", __func__, skill );
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
               break;
            }

            if( !str_cmp( word, "Speaks" ) )
            {
               ch->set_file_langs( fp );
               break;
            }

            if( !str_cmp( word, "Speaking" ) )
            {
               const char *speaking = fread_flagstring( fp );
               int value;

               value = get_langnum( speaking );
               if( value < 0 || value >= LANG_UNKNOWN )
                  bug( "Unknown language: %s", speaking );
               else
                  ch->speaking = value;
               break;
            }

            if( !str_cmp( word, "Spell" ) )
            {
               int value = fread_number( fp );
               char *spell = fread_word( fp );
               int sn = find_spell( NULL, spell, false );

               if( sn < 0 )
                  log_printf( "%s: unknown spell: %s", __func__, spell );
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
               break;
            }

            if( !str_cmp( word, "Status" ) )
            {
               line = fread_line( fp );
               x1 = x2 = x3 = x4 = x5 = x6 = x7 = 0;
               sscanf( line, "%d %d %d %d %d %d %d", &x1, &x2, &x3, &x4, &x5, &x6, &x7 );
               ch->level = x1;
               ch->gold = x2;
               ch->exp = x3;
               ch->height = x4;
               ch->weight = x5;
               ch->spellfail = x6;
               ch->mental_state = x7;
               if( preload )
                  return;
               else
                  break;
            }

            if( !str_cmp( word, "Status2" ) )
            {
               line = fread_line( fp );
               x1 = x2 = x3 = x4 = x5 = x6 = x7 = x8 = 0;
               sscanf( line, "%d %d %d %d %d %d %d %d", &x1, &x2, &x3, &x4, &x5, &x6, &x7, &x8 );
               ch->style = x1;
               ch->pcdata->practice = x2;
               ch->alignment = x3;
               ch->pcdata->favor = x4;
               ch->hitroll = x5;
               ch->damroll = x6;
               ch->armor = x7;
               ch->wimpy = x8;
               break;
            }

            if( !str_cmp( word, "Susceptible" ) )
            {
               ch->set_file_susceps( fp );
               break;
            }
            break;

         case 'E':
            if( !str_cmp( word, "End" ) )
            {
               char buf[MSL];

               if( preload )
                  return;

               /*
                * Let no character be trusted higher than one below maxlevel -- Narn 
                */
               ch->trust = UMIN( ch->trust, MAX_LEVEL - 1 );

               if( ch->pcdata->played < 0 )
                  ch->pcdata->played = 0;

               if( !ch->pcdata->chan_listen.empty(  ) )
               {
                  mud_channel *channel = NULL;
                  string channels = ch->pcdata->chan_listen;
                  string arg;

                  while( !channels.empty(  ) )
                  {
                     channels = one_argument( channels, arg );

                     if( !( channel = find_channel( arg ) ) )
                        removename( ch->pcdata->chan_listen, arg );
                  }
               }
               /*
                * Provide at least the one channel 
                */
               else
                  ch->pcdata->chan_listen = "chat";

               ch->pcdata->editor = NULL;

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
                  ch->pcdata->day = sysdata->dayspermonth - 1; /* Cathes the bad day values */

               if( !ch->pcdata->deity_name.empty(  ) && !( ch->pcdata->deity = get_deity( ch->pcdata->deity_name ) ) )
               {
                  snprintf( buf, MSL, "&R\r\nYour deity, %s, has met its demise!\r\n", ch->pcdata->deity_name.c_str() );
                  add_loginmsg( ch->name, 18, buf );
                  ch->pcdata->deity_name.clear(  );
               }

               if( ch->pcdata->deity_name.empty(  ) )
                  ch->pcdata->favor = 0;

               if( !ch->pcdata->clan_name.empty(  ) && !( ch->pcdata->clan = get_clan( ch->pcdata->clan_name ) ) )
               {
                  snprintf( buf, MSL, "&R\r\nWarning: The organization %s no longer exists, and therefore you no longer\r\nbelong to that organization.\r\n",
                     ch->pcdata->clan_name.c_str() );
                  add_loginmsg( ch->name, 18, buf );
                  ch->pcdata->clan_name.clear(  );
               }

               if( ch->pcdata->clan )
                  update_roster( ch );

               if( !ch->pcdata->realm_name.empty(  ) && !( ch->pcdata->realm = get_realm( ch->pcdata->realm_name ) ) )
               {
                  snprintf( buf, MSL, "&Y\r\nWarning: The realm %s no longer exists, and therefore you no longer\r\nbelong to a realm.\r\n",
                     ch->pcdata->realm_name.c_str() );
                  add_loginmsg( ch->name, 18, buf );
                  ch->pcdata->realm_name.clear(  );
               }

               return;
            }
            STDSKEY( "Email", ch->pcdata->email );
            break;

         case 'T':
            if( !str_cmp( word, "Tongue" ) )
            {
               int value = fread_number( fp );
               char *tongue = fread_word( fp );
               int sn = find_tongue( NULL, tongue, false );

               if( sn < 0 )
                  log_printf( "%s: unknown tongue: %s", __func__, tongue );
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
               break;
            }

            if( !str_cmp( word, "Title" ) )
            {
               ch->pcdata->title = fread_string( fp );
               if( ch->pcdata->title != NULL && ( isalpha( ch->pcdata->title[0] ) || isdigit( ch->pcdata->title[0] ) ) )
                  stralloc_printf( &ch->pcdata->title, " %s", ch->pcdata->title );
               break;
            }
            break;

         case 'V':
            if( !str_cmp( word, "Version" ) )
            {
               file_ver = fread_number( fp );
               ch->pcdata->version = file_ver;
               break;
            }
            break;

         case 'Z':
            if( !str_cmp( word, "Zone" ) )
            {
               ch->pcdata->load_zonedata( fp );
               break;
            }
            break;
      }
   }
   while( !feof( fp ) );
}

void fread_obj( char_data * ch, FILE * fp, short os_type )
{
   obj_data *obj;
   int iNest, obj_file_ver;
   bool fNest, fVnum;
   room_index *room = NULL;

   if( ch )
   {
      room = ch->in_room;
      if( ch->tempnum == -9999 )
         file_ver = 0;
   }

   /*
    * Jesus Christ, how the hell did Smaug get away without versioning the object format for so long!!! 
    */
   obj_file_ver = 0;

   obj = new obj_data;
   obj->count = 1;
   obj->wear_loc = -1;
   obj->weight = 1;
   obj->wmap = -1;
   obj->mx = -1;
   obj->my = -1;

   fNest = true;  /* Requiring a Nest 0 is a waste */
   fVnum = false; // We can't assume this - what if Vnum isn't written to the file? Crashy crashy is what. - Pulled from Smaug 1.8
   iNest = 0;

   for( ;; )
   {
      const char *word = ( feof( fp ) ? "End" : fread_word( fp ) );

      if( word[0] == '\0' )
      {
         log_printf( "%s: EOF encountered reading file!", __func__ );
         word = "End";
      }

      switch ( UPPER( word[0] ) )
      {
         default:
            bug( "%s: no match: %s", __func__, word );
            fread_to_eol( fp );
            deleteptr( obj );
            return;

         case '*':
            fread_to_eol( fp );
            break;

         case 'A':
            KEY( "ActionDesc", obj->action_desc, fread_string( fp ) );
            if( !str_cmp( word, "Affect" ) || !str_cmp( word, "AffectData" ) )
            {
               affect_data *paf;
               int pafmod;

               paf = new affect_data;
               if( !str_cmp( word, "Affect" ) )
                  paf->type = fread_number( fp );
               else
               {
                  int sn;

                  sn = skill_lookup( fread_word( fp ) );
                  if( sn < 0 )
                     bug( "%s: Vnum %d - unknown skill: %s", __func__, obj->pIndexData->vnum, word );
                  else
                     paf->type = sn;
               }
               paf->duration = fread_number( fp );
               pafmod = fread_number( fp );
               paf->location = fread_number( fp );
               paf->bit = fread_number( fp );
               if( paf->location == APPLY_WEAPONSPELL
                   || paf->location == APPLY_WEARSPELL || paf->location == APPLY_STRIPSN || paf->location == APPLY_REMOVESPELL || paf->location == APPLY_RECURRINGSPELL )
                  paf->modifier = slot_lookup( pafmod );
               else
                  paf->modifier = pafmod;
               obj->affects.push_back( paf );
               break;
            }
            break;

         case 'B':
            KEY( "Bid", obj->bid, fread_number( fp ) );  /* Samson 6-20-99 */
            KEY( "Buyer", obj->buyer, fread_string( fp ) ); /* Samson 6-20-99 */
            break;

         case 'C':
            if( !str_cmp( word, "Coords" ) )
            {
               obj->mx = fread_short( fp );
               obj->my = fread_short( fp );
               obj->wmap = fread_short( fp );
               break;
            }
            KEY( "Cost", obj->cost, fread_number( fp ) );
            KEY( "Count", obj->count, fread_number( fp ) );
            break;

         case 'D':
            KEY( "Description", obj->objdesc, fread_string( fp ) );
            break;

         case 'E':
            KEY( "Ego", obj->ego, fread_number( fp ) );  /* Samson 5-8-99 */
            if( !str_cmp( word, "ExtraFlags" ) )
            {
               flag_set( fp, obj->extra_flags, o_flags );
               break;
            }

            if( !str_cmp( word, "ExtraDescr" ) )
            {
               extra_descr_data *ed = new extra_descr_data;

               fread_string( ed->keyword, fp );
               fread_string( ed->desc, fp );
               obj->extradesc.push_back( ed );
               break;
            }

            if( !str_cmp( word, "End" ) )
            {
               if( !fNest || !fVnum )
               {
                  if( obj->name )
                     bug( "%s: %s incomplete object. obj_file_ver=%d", __func__, obj->name, obj_file_ver );
                  else
                     bug( "%s: incomplete object. obj_file_ver=%d", __func__, obj_file_ver );
                  deleteptr( obj );
                  return;
               }
               else
               {
                  short wear_loc = obj->wear_loc;

                  if( !obj->name && obj->pIndexData->name != NULL )
                     obj->name = QUICKLINK( obj->pIndexData->name );
                  if( !obj->objdesc && obj->pIndexData->objdesc != NULL )
                     obj->objdesc = QUICKLINK( obj->pIndexData->objdesc );
                  if( !obj->short_descr && obj->pIndexData->short_descr != NULL )
                     obj->short_descr = QUICKLINK( obj->pIndexData->short_descr );
                  if( !obj->action_desc && obj->pIndexData->action_desc != NULL )
                     obj->action_desc = QUICKLINK( obj->pIndexData->action_desc );
                  if( obj->extra_flags.test( ITEM_PERSONAL ) && !obj->owner && ch )
                     obj->owner = QUICKLINK( ch->name );
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
                  if( !is_valid_wear_loc( ch, wear_loc ) )
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
                        bug( "%s: Corpse without room", __func__ );
                        room = get_room_index( ROOM_VNUM_LIMBO );
                     }

                     /*
                      * Give the corpse a timer if there isn't one 
                      */
                     if( obj->timer < 1 )
                        obj->timer = 80;
                     obj->to_room( room, NULL );
                  }
                  else if( iNest == 0 || rgObjNest[iNest] == NULL )
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
                        bug( "%s: nest layer missing %d", __func__, iNest - 1 );
                  }
                  if( fNest )
                     rgObjNest[iNest] = obj;
                  return;
               }
            }
            break;

         case 'I':
            KEY( "ItemType", obj->item_type, fread_number( fp ) );
            break;

         case 'L':
            KEY( "Level", obj->level, fread_number( fp ) );
            break;

         case 'N':
            KEY( "Name", obj->name, fread_string( fp ) );

            if( !str_cmp( word, "Nest" ) )
            {
               iNest = fread_number( fp );
               if( iNest < 0 || iNest >= MAX_NEST )
               {
                  bug( "%s: bad nest %d.", __func__, iNest );
                  iNest = 0;
                  fNest = false;
               }
               break;
            }
            break;

         case 'O':
            KEY( "Oday", obj->day, fread_number( fp ) );
            KEY( "Omonth", obj->month, fread_number( fp ) );
            KEY( "Oyear", obj->year, fread_number( fp ) );
            KEY( "Owner", obj->owner, fread_string( fp ) );
            if( !str_cmp( word, "Ovnum" ) )
            {
               int vnum;

               vnum = fread_number( fp );
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
               break;
            }
            break;

         case 'R':
            KEY( "Rent", obj->ego, fread_number( fp ) ); /* Samson 5-8-99 - OLD FIELD */
            KEY( "Room", room, get_room_index( fread_number( fp ) ) );
            KEY( "Rvnum", obj->room_vnum, fread_number( fp ) );   /* hotboot tracker */
            break;

         case 'S':
            KEY( "Seller", obj->seller, fread_string( fp ) );  /* Samson 6-20-99 */
            KEY( "ShortDescr", obj->short_descr, fread_string( fp ) );

            if( !str_cmp( word, "Spell" ) )
            {
               int iValue, sn;

               iValue = fread_number( fp );
               sn = skill_lookup( fread_word( fp ) );
               if( iValue < 0 || iValue > 10 )
                  bug( "%s: bad iValue %d.", __func__, iValue );
               /*
                * Bug fixed here to change corrupted spell values to -1 to stop spamming logs - Samson 7-5-03 
                */
               else if( sn < 0 )
               {
                  bug( "%s: Vnum %d - unknown skill: %s", __func__, obj->pIndexData->vnum, word );
                  obj->value[iValue] = -1;
               }
               else
                  obj->value[iValue] = sn;
               break;
            }

            if( !str_cmp( word, "Sockets" ) )
            {
               obj->socket[0] = STRALLOC( fread_word( fp ) );
               obj->socket[1] = STRALLOC( fread_word( fp ) );
               obj->socket[2] = STRALLOC( fread_word( fp ) );
               break;
            }
            break;

         case 'T':
            KEY( "Timer", obj->timer, fread_number( fp ) );
            break;

         case 'V':
            KEY( "Version", obj_file_ver, fread_number( fp ) );
            if( !str_cmp( word, "Values" ) )
            {
               int x1, x2, x3, x4, x5, x6, x7, x8, x9, x10, x11;
               const char *ln = fread_line( fp );

               x1 = x2 = x3 = x4 = x5 = x6 = x7 = x8 = 0;
               sscanf( ln, "%d %d %d %d %d %d %d %d %d %d %d", &x1, &x2, &x3, &x4, &x5, &x6, &x7, &x8, &x9, &x10, &x11 );

               if( x7 == 0 && ( obj->item_type == ITEM_WEAPON || obj->item_type == ITEM_MISSILE_WEAPON ) )
                  x7 = x1;

               if( x6 == 0 && obj->item_type == ITEM_PROJECTILE )
                  x6 = x1;

               obj->value[0] = x1;
               obj->value[1] = x2;
               obj->value[2] = x3;
               obj->value[3] = x4;
               obj->value[4] = x5;
               obj->value[5] = x6;
               obj->value[6] = x7;
               obj->value[7] = x8;
               obj->value[8] = x9;
               obj->value[9] = x10;
               obj->value[10] = x11;

               // Note to future self looking at this is what's possibly 2025: Don't try to fix this again, ok? It works properly as listed now that hotboots have been accounted for.
               // It will still log from other sources though, so hey, if something OTHER that the hotboot recovery is triggering it, investigate that cause it may not be right!
               if( file_ver < 10 && fVnum == true && os_type != OS_CORPSE && ch->tempnum != -9999 )
               {
                  log_printf( "%s: != OS_CORPSE case encountered. file_ver=%d", __func__, file_ver );
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
               break;
            }

            if( !str_cmp( word, "Vnum" ) )
            {
               int vnum;

               vnum = fread_number( fp );
               if( !( obj->pIndexData = get_obj_index( vnum ) ) )
                  fVnum = false;
               else
               {
                  fVnum = true;
                  obj->cost = obj->pIndexData->cost;
                  obj->ego = obj->pIndexData->ego; /* Samson 5-8-99 */
                  obj->weight = obj->pIndexData->weight;
                  obj->item_type = obj->pIndexData->item_type;
                  obj->wear_flags = obj->pIndexData->wear_flags;
                  obj->extra_flags = obj->pIndexData->extra_flags;
               }
               break;
            }
            break;

         case 'W':
            if( !str_cmp( word, "WearFlags" ) )
            {
               flag_set( fp, obj->wear_flags, w_flags );
               break;
            }
            KEY( "WearLoc", obj->wear_loc, fread_number( fp ) );
            KEY( "Weight", obj->weight, fread_number( fp ) );
            break;
      }
   }
}

/*
 * This will read one mobile structure pointer to by fp --Shaddai
 *   Edited by Tarl 5 May 2002 to allow pets to load equipment.
 */
char_data *fread_mobile( FILE * fp, bool shopmob )
{
   char_data *mob = NULL;
   const char *word;
   int inroom = 0;
   room_index *pRoomIndex = NULL;
   mob_index *pMobIndex = NULL;

   if( !shopmob )
      word = feof( fp ) ? "EndMobile" : fread_word( fp );
   else
      word = feof( fp ) ? "EndVendor" : fread_word( fp );

   if( !str_cmp( word, "Vnum" ) )
   {
      int vnum = fread_number( fp );
      if( !( pMobIndex = get_mob_index( vnum ) ) )
      {
         for( ;; )
         {
            if( !shopmob )
               word = feof( fp ) ? "EndMobile" : fread_word( fp );
            else
               word = feof( fp ) ? "EndVendor" : fread_word( fp );
            /*
             * So we don't get so many bug messages when something messes up
             * * --Shaddai
             */
            if( !str_cmp( word, "EndMobile" ) || !str_cmp( word, "EndVendor" ) )
               break;
         }
         bug( "%s: No index data for vnum %d", __func__, vnum );
         return NULL;
      }
      mob = pMobIndex->create_mobile(  );
   }
   else
   {
      for( ;; )
      {
         if( !shopmob )
            word = feof( fp ) ? "EndMobile" : fread_word( fp );
         else
            word = feof( fp ) ? "EndVendor" : fread_word( fp );
         /*
          * So we don't get so many bug messages when something messes up
          * * --Shaddai
          */
         if( !str_cmp( word, "EndMobile" ) || !str_cmp( word, "EndVendor" ) )
            break;
      }
      mob->extract( true );
      bug( "%s: Vnum not found", __func__ );
      return NULL;
   }

   for( ;; )
   {
      if( !shopmob )
         word = feof( fp ) ? "EndMobile" : fread_word( fp );
      else
         word = feof( fp ) ? "EndVendor" : fread_word( fp );

      switch ( UPPER( word[0] ) )
      {
         default:
            bug( "%s: no match: %s", __func__, word );
            fread_to_eol( fp );
            break;

         case '*':
            fread_to_eol( fp );
            break;

         case 'A':
            if( !str_cmp( word, "ActFlags" ) )
            {
               mob->set_file_actflags( fp );
               break;
            }

            if( !str_cmp( word, "Affect" ) || !str_cmp( word, "AffectData" ) )
            {
               affect_data *paf;

               paf = new affect_data;
               if( !str_cmp( word, "Affect" ) )
                  paf->type = fread_number( fp );
               else
               {
                  int sn;
                  char *sname = fread_word( fp );

                  if( ( sn = skill_lookup( sname ) ) < 0 )
                  {
                     if( ( sn = herb_lookup( sname ) ) < 0 )
                        bug( "%s: unknown skill.", __func__ );
                     else
                        sn += TYPE_HERB;
                  }
                  paf->type = sn;
               }

               paf->duration = fread_number( fp );
               paf->modifier = fread_number( fp );
               paf->location = fread_number( fp );
               if( paf->location == APPLY_WEAPONSPELL
                   || paf->location == APPLY_WEARSPELL || paf->location == APPLY_REMOVESPELL || paf->location == APPLY_STRIPSN || paf->location == APPLY_RECURRINGSPELL )
                  paf->modifier = slot_lookup( paf->modifier );
               paf->bit = fread_number( fp );
               mob->affects.push_back( paf );
               break;
            }

            if( !str_cmp( word, "AffectedBy" ) )
            {
               mob->set_file_aflags( fp );
               break;
            }
            break;

         case 'C':
            if( !str_cmp( word, "Coordinates" ) )
            {
               mob->mx = fread_short( fp );
               mob->my = fread_short( fp );
               mob->wmap = fread_short( fp );
               break;
            }
            break;

         case 'D':
            if( !str_cmp( word, "Description" ) )
            {
               STRFREE( mob->chardesc );
               mob->chardesc = fread_string( fp );
               break;
            }
            break;

         case 'E':
            if( !str_cmp( word, "EndMobile" ) || !str_cmp( word, "EndVendor" ) )
            {
               if( inroom == 0 )
                  inroom = ROOM_VNUM_TEMPLE;
               pRoomIndex = get_room_index( inroom );
               if( !pRoomIndex )
                  pRoomIndex = get_room_index( ROOM_VNUM_LIMBO );
               if( !mob->to_room( pRoomIndex ) )
                  log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __func__, __LINE__ );

               for( int i = 0; i < MAX_WEAR; ++i )
                  for( int x = 0; x < MAX_LAYERS; ++x )
                     if( mob_save_equipment[i][x] )
                     {
                        mob->equip( mob_save_equipment[i][x], i );
                        mob_save_equipment[i][x] = NULL;
                     }
               return mob;
            }

            if( !str_cmp( word, "Exp" ) )
            {
               mob->exp = fread_number( fp );
               break;
            }
            break;

         case 'G':
            KEY( "Gold", mob->gold, fread_number( fp ) );
            break;

         case 'H':
            if( !str_cmp( word, "HpManaMove" ) )
            {
               mob->hit = fread_number( fp );
               mob->max_hit = fread_number( fp );
               mob->mana = fread_number( fp );
               mob->max_mana = fread_number( fp );
               mob->move = fread_number( fp );
               mob->max_move = fread_number( fp );
               break;
            }
            break;

         case 'L':
            KEY( "Level", mob->level, fread_number( fp ) );
            if( !str_cmp( word, "Long" ) )
            {
               STRFREE( mob->long_descr );
               mob->long_descr = fread_string( fp );
               break;
            }
            break;

         case 'N':
            if( !str_cmp( word, "Name" ) )
            {
               STRFREE( mob->name );
               mob->name = fread_string( fp );
               break;
            }
            break;

         case 'P':
            KEY( "Position", mob->position, fread_number( fp ) );
            break;

         case 'R':
            KEY( "Room", inroom, fread_number( fp ) );
            break;

         case 'S':
            if( !str_cmp( word, "Short" ) )
            {
               STRFREE( mob->short_descr );
               mob->short_descr = fread_string( fp );
               break;
            }
            break;
      }
      if( !str_cmp( word, "#OBJECT" ) )
      {  /* Objects      */
         fread_obj( mob, fp, OS_CARRY );
      }
   }
}

/*
 * Load a char and inventory into a new ch structure.
 */
bool load_char_obj( descriptor_data * d, const string & name, bool preload, bool copyover )
{
   char strsave[256];
   FILE *fp;
   bool found = false;
   struct stat fst;
   int i, x;

   if( !d )
   {
      bug( "%s: NULL d! This should not have happened!", __func__ );
      return false;
   }

   char_data *ch = new char_data;

   for( x = 0; x < MAX_WEAR; ++x )
   {
      for( i = 0; i < MAX_LAYERS; ++i )
      {
         save_equipment[x][i] = NULL;
         mob_save_equipment[x][i] = NULL;
      }
   }
   loading_char = ch;

   ch->pcdata = new pc_data;

   d->character = ch;
   ch->desc = d;
   ch->pcdata->filename = STRALLOC( name.c_str(  ) );
   if( !d->host.empty() )
      ch->pcdata->lasthost = d->host;
   ch->style = STYLE_FIGHTING;
   ch->mental_state = -10;
   ch->pcdata->prompt = STRALLOC( default_prompt( ch ) );
   ch->pcdata->fprompt = STRALLOC( default_fprompt( ch ) );
   ch->pcdata->version = 0;
   ch->set_langs( 0 );
   ch->set_lang( LANG_COMMON );
   ch->speaking = LANG_COMMON;
   ch->abits.clear(  );
   ch->pcdata->qbits.clear(  );
#ifdef IMC
   imc_initchar( ch );
#endif

   /*
    * Setup color values in case player has none set - Samson 
    */
   reset_colors( ch );

   snprintf( strsave, 256, "%s%c/%s", PLAYER_DIR, tolower( name[0] ), capitalize( name ).c_str(  ) );
   if( stat( strsave, &fst ) != -1 && d->connected != CON_PLOADED )
   {
      if( preload )
         log_printf_plus( LOG_COMM, LEVEL_KL, "Preloading player data for: %s", ch->pcdata->filename );
      else
         log_printf_plus( LOG_COMM, LEVEL_KL, "Loading player data for %s (%dK)", ch->pcdata->filename, ( int )fst.st_size / 1024 );
   }
   /*
    * else no player file 
    */

   if( ( fp = fopen( strsave, "r" ) ) != NULL )
   {
      for( int iNest = 0; iNest < MAX_NEST; ++iNest )
         rgObjNest[iNest] = NULL;

      found = true;
      /*
       * Cheat so that bug will show line #'s -- Altrag 
       */
      fpArea = fp;
      mudstrlcpy( strArea, strsave, MIL );
      for( ;; )
      {
         char letter;
         char *word;

         letter = fread_letter( fp );
         if( letter == '*' )
         {
            fread_to_eol( fp );
            continue;
         }

         if( letter != '#' )
         {
            bug( "%s: # not found. %s", __func__, name.c_str(  ) );
            break;
         }

         word = fread_word( fp );
         if( !str_cmp( word, "PLAYER" ) )
         {
            fread_char( ch, fp, preload, copyover );
            if( preload )
               break;
         }
         else if( !str_cmp( word, "OBJECT" ) )  /* Objects  */
            fread_obj( ch, fp, OS_CARRY );
         else if( !str_cmp( word, "MorphData" ) )  /* Morphs */
            fread_morph_data( ch, fp );
         else if( !str_cmp( word, "COMMENT2" ) )
            ch->pcdata->fread_comment( fp ); /* Comments */
         else if( !str_cmp( word, "COMMENT" ) )
            ch->pcdata->fread_old_comment( fp );   /* Older Comments */
         else if( !str_cmp( word, "MOBILE" ) )
         {
            char_data *mob = NULL;
            mob = fread_mobile( fp, false );
            if( mob )
            {
               bind_follower( mob, ch, -1, -2 );
               if( mob->in_room && ch->in_room )
               {
                  mob->from_room(  );
                  if( !mob->to_room( ch->in_room ) )
                     log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __func__, __LINE__ );
               }
            }
         }
         else if( !str_cmp( word, "VARIABLE" ) )   // Quest Flags
            fread_variable( ch, fp );
         else if( !str_cmp( word, "END" ) )  /* Done */
            break;
         else
         {
            bug( "%s: bad section: %s", __func__, word );
            break;
         }
      }
      FCLOSE( fp );
      fpArea = NULL;
      mudstrlcpy( strArea, "$", MIL );
   }

   if( !found )
   {
      if( d )
      {
         if( d->msp_detected )
            ch->set_pcflag( PCFLAG_MSP );
      }
      ch->name = STRALLOC( name.c_str(  ) );
   }
   else
   {
      if( !ch->name )
         ch->name = STRALLOC( name.c_str(  ) );

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
                  save_equipment[i][x] = NULL;
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
   loading_char = NULL;
   return found;
}

/* Rewritten corpse saver - Samson */
void write_corpse( obj_data * corpse, const string & name )
{
   FILE *fp;
   char filename[256];

   snprintf( filename, 256, "%s%s", CORPSE_DIR, capitalize( name ).c_str(  ) );

   // No no no no no! Timer of 0 means it should be GONE GONE GONE!
   if( corpse->timer < 1 )
   {
      unlink( filename );
      return;
   }

   if( !( fp = fopen( filename, "w" ) ) )
   {
      bug( "%s: Error opening corpse file for write: %s", __func__, filename );
      return;
   }

   fprintf( fp, "%s", "#CORPSE\n" );
   fprintf( fp, "Version      %d\n", SAVEVERSION );
   if( corpse->count > 1 )
      fprintf( fp, "Count        %d\n", corpse->count );
   if( str_cmp( corpse->name, corpse->pIndexData->name ) )
      fprintf( fp, "Name         %s~\n", corpse->name );
   if( str_cmp( corpse->short_descr, corpse->pIndexData->short_descr ) )
      fprintf( fp, "ShortDescr   %s~\n", corpse->short_descr );
   if( str_cmp( corpse->objdesc, corpse->pIndexData->objdesc ) )
      fprintf( fp, "Description  %s~\n", corpse->objdesc );
   fprintf( fp, "Ovnum        %d\n", corpse->pIndexData->vnum );
   if( corpse->in_room )
   {
      fprintf( fp, "Room         %d\n", corpse->in_room->vnum );
      fprintf( fp, "Rvnum        %d\n", corpse->room_vnum );
   }
   if( corpse->weight != corpse->pIndexData->weight )
      fprintf( fp, "Weight       %d\n", corpse->weight );
   if( corpse->level )
      fprintf( fp, "Level        %d\n", corpse->level );
   if( corpse->timer )
      fprintf( fp, "Timer        %d\n", corpse->timer );
   if( corpse->cost != corpse->pIndexData->cost )
      fprintf( fp, "Cost         %d\n", corpse->cost );
   fprintf( fp, "Coords       %d %d %d\n", corpse->mx, corpse->my, corpse->wmap );
   fprintf( fp, "Values      " );
   for( int x = 0; x < MAX_OBJ_VALUE; ++x )
      fprintf( fp, " %d", corpse->value[x] );
   fprintf( fp, "%s", "\n" );
   fprintf( fp, "%s", "End\n\n" );

   if( !corpse->contents.empty(  ) )
      fwrite_obj( NULL, corpse->contents, NULL, fp, 1, false );

   fprintf( fp, "%s", "#END\n\n" );
   FCLOSE( fp );
}

void load_corpses( void )
{
   DIR *dp;
   struct dirent *de;

   if( !( dp = opendir( CORPSE_DIR ) ) )
   {
      bug( "%s: can't open %s", __func__, CORPSE_DIR );
      perror( CORPSE_DIR );
      return;
   }

   falling = 1;   /* Arbitrary, must be >0 though. */
   while( ( de = readdir( dp ) ) != NULL )
   {
      if( de->d_name[0] != '.' )
      {
         if( !str_cmp( de->d_name, "CVS" ) )
            continue;
         snprintf( strArea, MIL, "%s%s", CORPSE_DIR, de->d_name );
         fprintf( stderr, "Corpse -> %s\n", strArea );
         if( !( fpArea = fopen( strArea, "r" ) ) )
         {
            perror( strArea );
            continue;
         }
         for( ;; )
         {
            char letter;
            char *word;

            letter = fread_letter( fpArea );
            if( letter == '*' )
            {
               fread_to_eol( fpArea );
               continue;
            }
            if( letter != '#' )
            {
               bug( "%s: # not found.", __func__ );
               break;
            }
            word = fread_word( fpArea );
            if( !str_cmp( word, "CORPSE" ) )
               fread_obj( NULL, fpArea, OS_CORPSE );
            else if( !str_cmp( word, "OBJECT" ) )
               fread_obj( NULL, fpArea, OS_CARRY );
            else if( !str_cmp( word, "END" ) )
               break;
            else
            {
               bug( "%s: bad section: %s", __func__, word );
               break;
            }
         }
         FCLOSE( fpArea );
      }
   }
   mudstrlcpy( strArea, "$", MIL );
   closedir( dp );
   falling = 0;
}

CMDF( do_save )
{
   if( ch->isnpc(  ) )
      return;

   ch->WAIT_STATE( 2 ); /* For big muds with save-happy players, like RoD */
   ch->update_aris(  ); /* update char affects and RIS */
   ch->save(  );
   saving_char = NULL;
   ch->print( "Saved...\r\n" );
}
