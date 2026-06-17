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
 *  The MUDprograms are heavily based on the original MOBprogram code that  *
 *                       was written by N'Atas-ha.                          *
 ****************************************************************************/

#include "mud.h"
#include "area.h"
#include "bits.h"
#include "deity.h"
#include "descriptor.h"
#include "mobindex.h"
#include "mud_prog.h"
#include "objindex.h"
#include "overland.h"
#include "polymorph.h"
#include "raceclass.h"
#include "roomindex.h"
#include "smaugaffect.h"

extern int top_affect;

void transfer_char( char_data *, char_data *, room_index * );
void raw_kill( char_data *, char_data * );
double damage_risa( char_data *, double, int );
void damage_obj( obj_data * );
int recall( char_data *, int );
int get_trigflag( std::string_view );
morph_data *get_morph( std::string_view );
morph_data *get_morph_vnum( int );
void start_hunting( char_data *, char_data * );
void start_hating( char_data *, char_data * );
void start_fearing( char_data *, char_data * );
void stop_hating( char_data * );
void stop_hunting( char_data * );
void stop_fearing( char_data * );
void add_to_auth( char_data * );
void advance_level( char_data * );
int race_bodyparts( char_data * );

bool can_use_mprog( char_data * ch )
{
   if( !ch->isnpc(  ) || ch->has_aflag( AFF_CHARM ) || ch->desc )
   {
      ch->print( "Huh?\r\n" );
      return false;
   }
   return true;
}

CMDF( do_mpmset )
{
   std::string arg1, arg2, arg3;
   char_data *victim;
   int value, minattr, maxattr;

   if( !can_use_mprog( ch ) )
      return;

   smash_tilde( argument );

   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );
   arg3 = argument;

   if( arg1.empty(  ) )
   {
      progbug( "MpMset: no args", ch );
      return;
   }

   if( !( victim = ch->get_char_room( arg1 ) ) )
   {
      progbug( "MpMset: no victim", ch );
      return;
   }

   if( victim->is_immortal(  ) )
   {
      ch->print( "You can't do that!\r\n" );
      return;
   }

   if( victim->has_actflag( ACT_PROTOTYPE ) )
   {
      progbug( "MpMset: victim is proto", ch );
      return;
   }

   if( victim->isnpc(  ) )
   {
      minattr = 1;
      maxattr = 25;
   }
   else
   {
      minattr = 3;
      maxattr = 18;
   }

   value = is_number( arg3 ) ? atoi( arg3.c_str(  ) ) : -1;
   if( atoi( arg3.c_str(  ) ) < -1 && value == -1 )
      value = atoi( arg3.c_str(  ) );

   if( !str_cmp( arg2, "str" ) )
   {
      if( value < minattr || value > maxattr )
      {
         progbug( "MpMset: Invalid str", ch );
         return;
      }
      victim->perm_str = value;
      return;
   }

   if( !str_cmp( arg2, "int" ) )
   {
      if( value < minattr || value > maxattr )
      {
         progbug( "MpMset: Invalid int", ch );
         return;
      }
      victim->perm_int = value;
      return;
   }

   if( !str_cmp( arg2, "wis" ) )
   {
      if( value < minattr || value > maxattr )
      {
         progbug( "MpMset: Invalid wis", ch );
         return;
      }
      victim->perm_wis = value;
      return;
   }

   if( !str_cmp( arg2, "dex" ) )
   {
      if( value < minattr || value > maxattr )
      {
         progbug( "MpMset: Invalid dex", ch );
         return;
      }
      victim->perm_dex = value;
      return;
   }

   if( !str_cmp( arg2, "con" ) )
   {
      if( value < minattr || value > maxattr )
      {
         progbug( "MpMset: Invalid con", ch );
         return;
      }
      victim->perm_con = value;
      return;
   }

   if( !str_cmp( arg2, "cha" ) )
   {
      if( value < minattr || value > maxattr )
      {
         progbug( "MpMset: Invalid cha", ch );
         return;
      }
      victim->perm_cha = value;
      return;
   }

   if( !str_cmp( arg2, "lck" ) )
   {
      if( value < minattr || value > maxattr )
      {
         progbug( "MpMset: Invalid lck", ch );
         return;
      }
      victim->perm_lck = value;
      return;
   }

   if( !str_cmp( arg2, "sav1" ) )
   {
      if( value < -30 || value > 30 )
      {
         progbug( "MpMset: Invalid sav1", ch );
         return;
      }
      victim->saving_poison_death = value;
      return;
   }

   if( !str_cmp( arg2, "sav2" ) )
   {
      if( value < -30 || value > 30 )
      {
         progbug( "MpMset: Invalid sav2", ch );
         return;
      }
      victim->saving_wand = value;
      return;
   }

   if( !str_cmp( arg2, "sav3" ) )
   {
      if( value < -30 || value > 30 )
      {
         progbug( "MpMset: Invalid sav3", ch );
         return;
      }
      victim->saving_para_petri = value;
      return;
   }

   if( !str_cmp( arg2, "sav4" ) )
   {
      if( value < -30 || value > 30 )
      {
         progbug( "MpMset: Invalid sav4", ch );
         return;
      }
      victim->saving_breath = value;
      return;
   }

   if( !str_cmp( arg2, "sav5" ) )
   {
      if( value < -30 || value > 30 )
      {
         progbug( "MpMset: Invalid sav5", ch );
         return;
      }
      victim->saving_spell_staff = value;
      return;
   }

   /*
    * Modified to allow mobs to set by name instead of number - Samson 7-30-98 
    */
   if( !str_cmp( arg2, "sex" ) || !str_cmp( arg2, "gender" ) )
   {
      switch ( arg3[0] )
      {
         case 'm':
         case 'M':
            value = SEX_MALE;
            break;
         case 'f':
         case 'F':
            value = SEX_FEMALE;
            break;
         case 'n':
         case 'N':
            value = SEX_NEUTRAL;
            break;
         case 'h':
         case 'H':
            value = SEX_HERMAPHRODYTE;
            break;
         default:
            progbug( "MpMset: Attempting to set invalid gender!", ch );
            return;
      }
      victim->sex = value;
      return;
   }

   /*
    * Modified to allow mudprogs to set PC Class/race during new creation process 
    * Samson - 7-30-98 
    */
   if( !str_cmp( arg2, "Class" ) )
   {
      value = get_npc_class( arg3 );
      if( value < 0 )
         value = atoi( arg3.c_str(  ) );

      if( !victim->isnpc(  ) )
      {
         if( value < 0 || value >= MAX_PC_CLASS )
         {
            progbug( "MpMset: Attempting to set invalid player Class!", ch );
            return;
         }
         victim->Class = value;

         std::string tbuf = std::format( "the {}", title_table[value][victim->level][victim->sex] );
         victim->set_title( tbuf );
         return;
      }

      if( victim->isnpc(  ) )
      {
         if( value < 0 || value >= MAX_NPC_CLASS )
         {
            progbug( "MpMset: Invalid npc Class", ch );
            return;
         }
         victim->Class = value;
         return;
      }
   }

   if( !str_cmp( arg2, "race" ) )
   {
      value = get_npc_race( arg3 );
      if( value < 0 )
         value = atoi( arg3.c_str(  ) );
      if( !victim->isnpc(  ) )
      {
         if( value < 0 || value >= MAX_RACE )
         {
            progbug( "MpMset: Attempting to set invalid player race!", ch );
            return;
         }
         victim->race = value;
         return;
      }
      if( victim->isnpc(  ) )
      {
         if( value < 0 || value >= MAX_NPC_RACE )
         {
            progbug( "MpMset: Invalid npc race", ch );
            return;
         }
         victim->race = value;
         return;
      }
   }

   if( !str_cmp( arg2, "armor" ) )
   {
      if( value < -300 || value > 300 )
      {
         progbug( "MpMset: Bad AC value", ch );
         return;
      }
      victim->armor = value;
      return;
   }

   if( !str_cmp( arg2, "level" ) )
   {
      if( !victim->isnpc(  ) )
      {
         progbug( "MpMset: can't set pc level", ch );
         return;
      }

      if( value < 0 || value > LEVEL_AVATAR + 5 )
      {
         progbug( "MpMset: Invalid npc level", ch );
         return;
      }
      victim->level = value;
      return;
   }

   if( !str_cmp( arg2, "numattacks" ) )
   {
      if( !victim->isnpc(  ) )
      {
         progbug( "MpMset: can't set pc numattacks", ch  );
         return;
      }

      if( value < 0 || value > 20 )
      {
         progbug( "MpMset: Invalid npc numattacks", ch );
         return;
      }
      victim->numattacks = ( float )( value );
      return;
   }

   if( !str_cmp( arg2, "gold" ) )
   {
      victim->gold = value;
      return;
   }

   if( !str_cmp( arg2, "hitroll" ) )
   {
      victim->hitroll = urange( 0, value, 85 );
      return;
   }

   if( !str_cmp( arg2, "damroll" ) )
   {
      victim->damroll = urange( 0, value, 65 );
      return;
   }

   if( !str_cmp( arg2, "hp" ) )
   {
      if( value < 1 || value > 32700 )
      {
         progbug( "MpMset: Invalid hp", ch );
         return;
      }
      victim->max_hit = value;
      return;
   }

   if( !str_cmp( arg2, "mana" ) )
   {
      if( value < 0 || value > 30000 )
      {
         progbug( "MpMset: Invalid mana", ch );
         return;
      }
      victim->max_mana = value;
      return;
   }

   if( !str_cmp( arg2, "move" ) )
   {
      if( value < 0 || value > 30000 )
      {
         progbug( "MpMset: Invalid move", ch );
         return;
      }
      victim->max_move = value;
      return;
   }

   if( !str_cmp( arg2, "practice" ) )
   {
      if( victim->isnpc(  ) )
      {
         progbug( "MpMset: can't set npc practice", ch );
         return;
      }
      if( value < 0 || value > 500 )
      {
         progbug( "MpMset: Invalid practice", ch );
         return;
      }
      victim->pcdata->practice = value;
      return;
   }

   if( !str_cmp( arg2, "align" ) )
   {
      if( value < -1000 || value > 1000 )
      {
         progbug( "MpMset: Invalid align", ch );
         return;
      }
      victim->alignment = value;
      return;
   }

   if( !str_cmp( arg2, "favor" ) )
   {
      if( victim->isnpc(  ) )
      {
         progbug( "MpMset: can't set npc favor", ch );
         return;
      }

      if( value < -2500 || value > 2500 )
      {
         progbug( "MpMset: Invalid pc favor", ch );
         return;
      }

      victim->pcdata->favor = value;
      return;
   }

   if( !str_cmp( arg2, "mentalstate" ) )
   {
      if( value < -100 || value > 100 )
      {
         progbug( "MpMset: Invalid mentalstate", ch );
         return;
      }
      victim->mental_state = value;
      return;
   }

   if( !str_cmp( arg2, "thirst" ) )
   {
      if( victim->isnpc(  ) )
      {
         progbug( "MpMset: can't set npc thirst", ch );
         return;
      }

      if( value < 0 || value > 100 )
      {
         progbug( "MpMset: Invalid pc thirst", ch );
         return;
      }

      victim->pcdata->condition[COND_THIRST] = value;
      return;
   }

   if( !str_cmp( arg2, "drunk" ) )
   {
      if( victim->isnpc(  ) )
      {
         progbug( "MpMset: can't set npc drunk", ch );
         return;
      }

      if( value < 0 || value > 100 )
      {
         progbug( "MpMset: Invalid pc drunk", ch );
         return;
      }

      victim->pcdata->condition[COND_DRUNK] = value;
      return;
   }

   if( !str_cmp( arg2, "full" ) )
   {
      if( victim->isnpc(  ) )
      {
         progbug( "MpMset: can't set npc full", ch );
         return;
      }

      if( value < 0 || value > 100 )
      {
         progbug( "MpMset: Invalid pc full", ch );
         return;
      }

      victim->pcdata->condition[COND_FULL] = value;
      return;
   }

   if( !str_cmp( arg2, "name" ) )
   {
      if( !victim->isnpc(  ) )
      {
         progbug( "MpMset: can't set pc name", ch );
         return;
      }

      victim->name = arg3;
      return;
   }

   if( !str_cmp( arg2, "deity" ) )
   {
      deity_data *deity;

      if( victim->isnpc(  ) )
      {
         progbug( "MpMset: can't set npc deity", ch );
         return;
      }

      if( arg3.empty(  ) )
      {
         victim->pcdata->deity_name.clear(  );
         victim->pcdata->deity = nullptr;
         return;
      }

      deity = get_deity( arg3 );
      if( !deity )
      {
         progbug( "MpMset: Invalid deity", ch );
         return;
      }
      victim->pcdata->deity_name = deity->name;
      victim->pcdata->deity = deity;
      return;
   }

   if( !str_cmp( arg2, "short" ) )
   {
      victim->short_descr = arg3;
      return;
   }

   if( !str_cmp( arg2, "long" ) )
   {
      victim->long_descr = std::format( "{}\r\n", arg3 );
      return;
   }

   if( !str_cmp( arg2, "title" ) )
   {
      if( victim->isnpc(  ) )
      {
         progbug( "MpMset: can't set npc title", ch );
         return;
      }

      victim->set_title( arg3 );
      return;
   }

   if( !str_cmp( arg2, "spec" ) || !str_cmp( arg2, "spec_fun" ) )
   {
      if( !victim->isnpc(  ) )
      {
         progbug( "MpMset: can't set pc spec", ch );
         return;
      }

      if( !str_cmp( arg3, "none" ) )
      {
         victim->spec_fun = nullptr;
         victim->spec_funname.clear(  );
         return;
      }

      if( !( victim->spec_fun = m_spec_lookup( arg3 ) ) )
      {
         progbug( "MpMset: Invalid spec", ch );
         return;
      }
      victim->spec_funname = arg3;
      return;
   }

   if( !str_cmp( arg2, "flags" ) )
   {
      if( !victim->isnpc(  ) )
      {
         progbug( "MpMset: can't set pc flags", ch );
         return;
      }

      if( argument.empty(  ) )
      {
         progbug( "MpMset: no flags", ch );
         return;
      }

      while( !argument.empty(  ) )
      {
         argument = one_argument( argument, arg3 );
         value = get_actflag( arg3 );
         if( value < 0 || value >= MAX_ACT_FLAG )
            progbugf( ch, "MpMset: Invalid flag: {}", arg3 );
         else
         {
            if( value == ACT_PROTOTYPE )
               progbug( "MpMset: can't set prototype flag", ch );
            else if( value == ACT_IS_NPC )
               progbug( "MpMset: can't remove npc flag", ch );
            else
               victim->toggle_actflag( value );
         }
      }
      return;
   }

   if( !str_cmp( arg2, "affected" ) )
   {
      if( !victim->isnpc(  ) )
      {
         progbug( "MpMset: can't modify pc affected", ch );
         return;
      }

      if( argument.empty(  ) )
      {
         progbug( "MpMset: no affected", ch );
         return;
      }

      while( !argument.empty(  ) )
      {
         argument = one_argument( argument, arg3 );
         value = get_aflag( arg3 );
         if( value < 0 || value >= MAX_AFFECTED_BY )
            progbugf( ch, "MpMset: Invalid affected: {}", arg3 );
         else
            victim->toggle_aflag( value );
      }
      return;
   }

   /*
    * save some more finger-leather for setting RIS stuff
    * Why there's can_modify checks here AND in the called function, I'll
    * never know, so I removed them. -- Alty
    */
   if( !str_cmp( arg2, "r" ) )
   {
      funcf( ch, do_mpmset, "{} resistant {}", arg1, arg3 );
      return;
   }
   if( !str_cmp( arg2, "i" ) )
   {
      funcf( ch, do_mpmset, "{} immune {}", arg1, arg3 );
      return;
   }
   if( !str_cmp( arg2, "s" ) )
   {
      funcf( ch, do_mpmset, "{} susceptible {}", arg1, arg3 );
      return;
   }
   if( !str_cmp( arg2, "ri" ) )
   {
      funcf( ch, do_mpmset, "{} resistant {}", arg1, arg3 );
      funcf( ch, do_mpmset, "{} immune {}", arg1, arg3 );
      return;
   }

   if( !str_cmp( arg2, "rs" ) )
   {
      funcf( ch, do_mpmset, "{} resistant {}", arg1, arg3 );
      funcf( ch, do_mpmset, "{} susceptible {}", arg1, arg3 );
      return;
   }
   if( !str_cmp( arg2, "is" ) )
   {
      funcf( ch, do_mpmset, "{} immune {}", arg1, arg3 );
      funcf( ch, do_mpmset, "{} susceptible {}", arg1, arg3 );
      return;
   }
   if( !str_cmp( arg2, "ris" ) )
   {
      funcf( ch, do_mpmset, "{} resistant {}", arg1, arg3 );
      funcf( ch, do_mpmset, "{} immune {}", arg1, arg3 );
      funcf( ch, do_mpmset, "{} susceptible {}", arg1, arg3 );
      return;
   }

   if( !str_cmp( arg2, "resistant" ) )
   {
      if( !victim->isnpc(  ) )
      {
         progbug( "MpMset: can't set pc resistant", ch );
         return;
      }

      if( argument.empty(  ) )
      {
         progbug( "MpMset: no resistant", ch );
         return;
      }

      while( !argument.empty(  ) )
      {
         argument = one_argument( argument, arg3 );
         value = get_risflag( arg3 );
         if( value < 0 || value >= MAX_RIS_FLAG )
            progbugf( ch, "MpMset: Invalid resistant: {}", arg3 );
         else
            victim->toggle_resist( value );
      }
      return;
   }

   if( !str_cmp( arg2, "immune" ) )
   {
      if( !victim->isnpc(  ) )
      {
         progbug( "MpMset: can't set pc immune", ch );
         return;
      }

      if( argument.empty(  ) )
      {
         progbug( "MpMset: no immune", ch );
         return;
      }

      while( !argument.empty(  ) )
      {
         argument = one_argument( argument, arg3 );
         value = get_risflag( arg3 );
         if( value < 0 || value >= MAX_RIS_FLAG )
            progbugf( ch, "MpMset: Invalid immune: {}", arg3 );
         else
            victim->toggle_immune( value );
      }
      return;
   }

   if( !str_cmp( arg2, "susceptible" ) )
   {
      if( !victim->isnpc(  ) )
      {
         progbug( "MpMset: can't set pc susceptible", ch );
         return;
      }

      if( argument.empty(  ) )
      {
         progbug( "MpMset: no susceptible", ch );
         return;
      }

      while( !argument.empty(  ) )
      {
         argument = one_argument( argument, arg3 );
         value = get_risflag( arg3 );
         if( value < 0 || value >= MAX_RIS_FLAG )
            progbugf( ch, "MpMset: Invalid susceptible: {}", arg3 );
         else
            victim->toggle_suscep( value );
      }
      return;
   }

   if( !str_cmp( arg2, "absorb" ) )
   {
      if( !victim->isnpc(  ) )
      {
         progbug( "MpMset: can't set pc absorb", ch );
         return;
      }

      if( argument.empty(  ) )
      {
         progbug( "MpMset: no absorb", ch );
         return;
      }

      while( !argument.empty(  ) )
      {
         argument = one_argument( argument, arg3 );
         value = get_risflag( arg3 );
         if( value < 0 || value >= MAX_RIS_FLAG )
            progbugf( ch, "MpMset: Invalid absorb: {}", arg3 );
         else
            victim->toggle_absorb( value );
      }
      return;
   }

   if( !str_cmp( arg2, "part" ) )
   {
      if( !victim->isnpc(  ) )
      {
         progbug( "MpMset: can't set pc part", ch );
         return;
      }

      if( argument.empty(  ) )
      {
         progbug( "MpMset: no part", ch );
         return;
      }

      while( !argument.empty(  ) )
      {
         argument = one_argument( argument, arg3 );
         value = get_partflag( arg3 );
         if( value < 0 || value >= MAX_BPART )
            progbugf( ch, "MpMset: Invalid part: {}", arg3 );
         else
            victim->toggle_bpart( value );
      }
      return;
   }

   if( !str_cmp( arg2, "attack" ) )
   {
      if( !victim->isnpc(  ) )
      {
         progbug( "MpMset: can't set pc attack", ch );
         return;
      }

      if( argument.empty(  ) )
      {
         progbug( "MpMset: no attack", ch );
         return;
      }

      while( !argument.empty(  ) )
      {
         argument = one_argument( argument, arg3 );
         value = get_attackflag( arg3 );
         if( value < 0 || value >= MAX_ATTACK_TYPE )
            progbugf( ch, "MpMset: Invalid attack: {}", arg3 );
         else
            victim->toggle_attack( value );
      }
      return;
   }

   if( !str_cmp( arg2, "defense" ) )
   {
      if( !victim->isnpc(  ) )
      {
         progbug( "MpMset: can't set pc defense", ch );
         return;
      }

      if( argument.empty(  ) )
      {
         progbug( "MpMset: no defense", ch );
         return;
      }

      while( !argument.empty(  ) )
      {
         argument = one_argument( argument, arg3 );
         value = get_defenseflag( arg3 );
         if( value < 0 || value >= MAX_DEFENSE_TYPE )
            progbugf( ch, "MpMset: Invalid defense: {}", arg3 );
         else
            victim->toggle_defense( value );
      }
      return;
   }

   if( !str_cmp( arg2, "pos" ) )
   {
      if( !victim->isnpc(  ) )
      {
         progbug( "MpMset: can't set pc pos", ch );
         return;
      }

      if( value < 0 || value > POS_STANDING )
      {
         progbug( "MpMset: Invalid pos", ch );
         return;
      }
      victim->position = value;
      return;
   }

   if( !str_cmp( arg2, "defpos" ) )
   {
      if( !victim->isnpc(  ) )
      {
         progbug( "MpMset: can't set pc defpos", ch );
         return;
      }

      if( value < 0 || value > POS_STANDING )
      {
         progbug( "MpMset: Invalid defpos", ch );
         return;
      }
      victim->defposition = value;
      return;
   }

   if( !str_cmp( arg2, "speaks" ) )
   {
      if( argument.empty(  ) )
      {
         progbug( "MpMset: no speaks", ch );
         return;
      }

      while( !argument.empty(  ) )
      {
         argument = one_argument( argument, arg3 );
         value = get_langnum( arg3 );
         if( value < 0 || value >= LANG_UNKNOWN )
            progbugf( ch, "MpMset: Invalid speaks: {}", arg3 );
         else if( !victim->isnpc(  ) )
         {
            if( !( value &= VALID_LANGS ) )
               progbugf( ch, "MpMset: Invalid player language: {}", arg3 );
            else
               victim->toggle_lang( value );
         }
         else
            victim->toggle_lang( value );
      }
      return;
   }

   if( !str_cmp( arg2, "speaking" ) )
   {
      if( !victim->isnpc(  ) )
      {
         progbug( "MpMset: can't set pc speaking", ch );
         return;
      }

      if( argument.empty(  ) )
      {
         progbug( "MpMset: no speaking", ch );
         return;
      }

      argument = one_argument( argument, arg3 );
      value = get_langnum( arg3 );
      if( value < 0 || value >= LANG_UNKNOWN )
         progbugf( ch, "MpMset: Invalid speaking: {}", arg3 );
      else
         victim->speaking = value;
      return;
   }
   progbugf( ch, "MpMset: Invalid field: {}", arg2 );
}

CMDF( do_mposet )
{
   std::string arg1, arg2, arg3;
   obj_data *obj;
   int value, tmp;

   if( !can_use_mprog( ch ) )
      return;

   smash_tilde( argument );

   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );
   arg3 = argument;

   if( arg1.empty(  ) )
   {
      progbug( "MpOset: no args", ch );
      return;
   }

   if( !( obj = ch->get_obj_here( arg1 ) ) )
   {
      progbug( "MpOset: no object", ch );
      return;
   }

   if( obj->extra_flags.test( ITEM_PROTOTYPE ) )
   {
      progbug( "MpOset: can't set prototype items", ch );
      return;
   }
   obj->separate(  );
   value = atoi( arg3.c_str(  ) );

   if( !str_cmp( arg2, "value0" ) || !str_cmp( arg2, "v0" ) )
   {
      obj->value[0] = value;
      return;
   }

   if( !str_cmp( arg2, "value1" ) || !str_cmp( arg2, "v1" ) )
   {
      obj->value[1] = value;
      return;
   }

   if( !str_cmp( arg2, "value2" ) || !str_cmp( arg2, "v2" ) )
   {
      obj->value[2] = value;
      return;
   }

   if( !str_cmp( arg2, "value3" ) || !str_cmp( arg2, "v3" ) )
   {
      obj->value[3] = value;
      return;
   }

   if( !str_cmp( arg2, "value4" ) || !str_cmp( arg2, "v4" ) )
   {
      obj->value[4] = value;
      return;
   }

   if( !str_cmp( arg2, "value5" ) || !str_cmp( arg2, "v5" ) )
   {
      if( obj->item_type == ITEM_CORPSE_PC )
      {
         progbug( "MpOset: Attempting to alter skeleton value for corpse", ch );
         return;
      }

      obj->value[5] = value;
      return;
   }

   if( !str_cmp( arg2, "value6" ) || !str_cmp( arg2, "v6" ) )
   {
      obj->value[6] = value;
      return;
   }

   if( !str_cmp( arg2, "value7" ) || !str_cmp( arg2, "v7" ) )
   {
      obj->value[7] = value;
      return;
   }

   if( !str_cmp( arg2, "value8" ) || !str_cmp( arg2, "v8" ) )
   {
      obj->value[8] = value;
      return;
   }

   if( !str_cmp( arg2, "value9" ) || !str_cmp( arg2, "v9" ) )
   {
      obj->value[9] = value;
      return;
   }

   if( !str_cmp( arg2, "value10" ) || !str_cmp( arg2, "v10" ) )
   {
      obj->value[10] = value;
      return;
   }

   if( !str_cmp( arg2, "type" ) )
   {
      if( argument.empty(  ) )
      {
         progbug( "MpOset: no type", ch );
         return;
      }

      value = get_otype( argument );
      if( value < 1 )
      {
         progbug( "MpOset: Invalid type", ch );
         return;
      }
      obj->item_type = value;
      return;
   }

   if( !str_cmp( arg2, "flags" ) )
   {
      if( argument.empty(  ) )
      {
         progbug( "MpOset: no flags", ch );
         return;
      }

      while( !argument.empty(  ) )
      {
         argument = one_argument( argument, arg3 );
         value = get_oflag( arg3 );
         if( value < 0 || value >= MAX_ITEM_FLAG )
            progbugf( ch, "MpOset: Invalid flag: {}", arg3 );
         else
         {
            if( value == ITEM_PROTOTYPE )
               progbug( "MpOset: can't set prototype flag", ch );
            else
               obj->extra_flags.flip( value );
         }
      }
      return;
   }

   if( !str_cmp( arg2, "wear" ) )
   {
      if( argument.empty(  ) )
      {
         progbug( "MpOset: no wear", ch );
         return;
      }

      while( !argument.empty(  ) )
      {
         argument = one_argument( argument, arg3 );
         value = get_wflag( arg3 );
         if( value < 0 || value >= MAX_WEAR_FLAG )
            progbugf( ch, "MpOset: Invalid wear: {}", arg3 );
         else
            obj->wear_flags.flip( value );
      }
      return;
   }

   if( !str_cmp( arg2, "level" ) )
   {
      obj->level = value;
      return;
   }

   if( !str_cmp( arg2, "weight" ) )
   {
      obj->weight = value;
      return;
   }

   if( !str_cmp( arg2, "cost" ) )
   {
      obj->cost = value;
      return;
   }

   if( !str_cmp( arg2, "timer" ) )
   {
      obj->timer = value;
      return;
   }

   if( !str_cmp( arg2, "name" ) )
   {
      STRFREE( obj->name );
      obj->name = STRALLOC( arg3.c_str(  ) );
      return;
   }

   if( !str_cmp( arg2, "short" ) )
   {
      STRFREE( obj->short_descr );
      obj->short_descr = STRALLOC( arg3.c_str(  ) );
      if( obj == supermob_obj )
      {
         supermob->short_descr = obj->short_descr;
      }

      /*
       * Feature added by Narn, Apr/96 
       * * If the item is not proto, add the word 'rename' to the keywords
       * * if it is not already there.
       */
      if( str_infix( "mprename", obj->name ) )
         stralloc_printf( &obj->name, "%s %s", obj->name, "mprename" );
      return;
   }

   if( !str_cmp( arg2, "long" ) )
   {
      STRFREE( obj->objdesc );
      obj->objdesc = STRALLOC( arg3.c_str(  ) );
      return;
   }

   if( !str_cmp( arg2, "actiondesc" ) )
   {
      if( strstr( arg3.c_str(  ), "%n" ) || strstr( arg3.c_str(  ), "%d" ) || strstr( arg3.c_str(  ), "%l" ) )
      {
         progbug( "MpOset: Illegal actiondesc", ch );
         return;
      }
      STRFREE( obj->action_desc );
      obj->action_desc = STRALLOC( arg3.c_str(  ) );
      return;
   }

   if( !str_cmp( arg2, "affect" ) )
   {
      affect_data *paf;
      short loc;

      argument = one_argument( argument, arg2 );
      if( arg2.empty(  ) || argument.empty(  ) )
      {
         progbug( "MpOset: Bad affect syntax", ch );
         return;
      }

      loc = get_atype( arg2 );
      if( loc < 1 )
      {
         progbug( "MpOset: Invalid affect field", ch );
         return;
      }

      if( ( loc >= APPLY_AFFECT && loc < APPLY_WEAPONSPELL ) || loc == APPLY_ABSORB )
      {
         bool found = false;

         argument = one_argument( argument, arg3 );
         if( loc == APPLY_AFFECT )
         {
            value = get_aflag( arg3 );

            if( value < 0 || value >= MAX_AFFECTED_BY )
               ch->printf( "Unknown flag: %s\r\n", arg3.c_str(  ) );
            found = true;
         }
         else
         {
            value = get_risflag( arg3 );

            if( value < 0 || value >= MAX_RIS_FLAG )
               ch->printf( "Unknown flag: %s\r\n", arg3.c_str(  ) );
            found = true;
         }
         if( !found )
            return;
      }
      else
      {
         argument = one_argument( argument, arg3 );
         value = atoi( arg3.c_str(  ) );
      }
      paf = new affect_data;
      paf->type = -1;
      paf->duration = -1;
      paf->location = loc;
      paf->modifier = value;
      paf->bit = 0;
      obj->affects.push_back( paf );
      ++top_affect;
      return;
   }

   if( !str_cmp( arg2, "rmaffect" ) )
   {
      short loc, count;

      if( argument.empty(  ) )
      {
         progbug( "MpOset: no rmaffect", ch );
         return;
      }

      loc = atoi( argument.c_str(  ) );
      if( loc < 1 )
      {
         progbug( "MpOset: Invalid rmaffect", ch );
         return;
      }

      count = 0;

      for( auto it = obj->affects.begin(); it != obj->affects.end(); )
      {
         affect_data *aff = *it;
         ++it;

         if( ++count == loc )
         {
            obj->affects.remove( aff );
            deleteptr( aff );
            --top_affect;
            return;
         }
      }
      progbug( "MpOset: rmaffect not found", ch );
      return;
   }

   /*
    * save some finger-leather
    */
   if( !str_cmp( arg2, "ris" ) )
   {
      funcf( ch, do_mposet, "{} affect resistant {}", arg1, arg3 );
      funcf( ch, do_mposet, "{} affect immune {}", arg1, arg3 );
      funcf( ch, do_mposet, "{} affect susceptible {}", arg1, arg3 );
      return;
   }

   if( !str_cmp( arg2, "r" ) )
   {
      funcf( ch, do_mpmset, "{} affect resistant {}", arg1, arg3 );
      return;
   }

   if( !str_cmp( arg2, "i" ) )
   {
      funcf( ch, do_mposet, "{} affect immune {}", arg1, arg3 );
      return;
   }

   if( !str_cmp( arg2, "s" ) )
   {
      funcf( ch, do_mposet, "{} affect susceptible {}", arg1, arg3 );
      return;
   }

   if( !str_cmp( arg2, "ri" ) )
   {
      funcf( ch, do_mposet, "{} affect resistant {}", arg1, arg3 );
      funcf( ch, do_mposet, "{} affect immune {}", arg1, arg3 );
      return;
   }

   if( !str_cmp( arg2, "rs" ) )
   {
      funcf( ch, do_mposet, "{} affect resistant {}", arg1, arg3 );
      funcf( ch, do_mposet, "{} affect susceptible {}", arg1, arg3 );
      return;
   }

   if( !str_cmp( arg2, "is" ) )
   {
      funcf( ch, do_mposet, "{} affect immune {}", arg1, arg3 );
      funcf( ch, do_mposet, "{} affect susceptible {}", arg1, arg3 );
      return;
   }

   /*
    * Make it easier to set special object values by name than number
    *                  -Thoric
    */
   tmp = -1;
   switch ( obj->item_type )
   {
      default:
         break;

      case ITEM_WEAPON:
         if( !str_cmp( arg2, "weapontype" ) )
         {
            value = -1;
            for( size_t x = 0; x < sizeof( attack_table ) / sizeof( attack_table[0] ); ++x )
               if( !str_cmp( arg3, attack_table[x] ) )
                  value = x;
            if( value < 0 )
            {
               progbug( "MpOset: Invalid weapon type", ch );
               return;
            }
            tmp = 3;
            break;
         }
         if( !str_cmp( arg2, "condition" ) )
            tmp = 0;
         break;

      case ITEM_ARMOR:
         if( !str_cmp( arg2, "condition" ) )
            tmp = 3;
         if( !str_cmp( arg2, "ac" ) )
            tmp = 1;
         break;

      case ITEM_SALVE:
         if( !str_cmp( arg2, "slevel" ) )
            tmp = 0;
         if( !str_cmp( arg2, "maxdoses" ) )
            tmp = 1;
         if( !str_cmp( arg2, "doses" ) )
            tmp = 2;
         if( !str_cmp( arg2, "delay" ) )
            tmp = 3;
         if( !str_cmp( arg2, "spell1" ) )
            tmp = 4;
         if( !str_cmp( arg2, "spell2" ) )
            tmp = 5;
         if( tmp >= 4 && tmp <= 5 )
            value = skill_lookup( arg3 );
         break;

      case ITEM_SCROLL:
      case ITEM_POTION:
      case ITEM_PILL:
         if( !str_cmp( arg2, "slevel" ) )
            tmp = 0;
         if( !str_cmp( arg2, "spell1" ) )
            tmp = 1;
         if( !str_cmp( arg2, "spell2" ) )
            tmp = 2;
         if( !str_cmp( arg2, "spell3" ) )
            tmp = 3;
         if( tmp >= 1 && tmp <= 3 )
            value = skill_lookup( arg3 );
         break;

      case ITEM_STAFF:
      case ITEM_WAND:
         if( !str_cmp( arg2, "slevel" ) )
            tmp = 0;
         if( !str_cmp( arg2, "spell" ) )
         {
            tmp = 3;
            value = skill_lookup( arg3 );
         }
         if( !str_cmp( arg2, "maxcharges" ) )
            tmp = 1;
         if( !str_cmp( arg2, "charges" ) )
            tmp = 2;
         break;

      case ITEM_CONTAINER:
         if( !str_cmp( arg2, "capacity" ) )
            tmp = 0;
         if( !str_cmp( arg2, "cflags" ) )
            tmp = 1;
         if( !str_cmp( arg2, "key" ) )
            tmp = 2;
         break;

      case ITEM_SWITCH:
      case ITEM_LEVER:
      case ITEM_PULLCHAIN:
      case ITEM_BUTTON:
         if( !str_cmp( arg2, "tflags" ) )
         {
            tmp = 0;
            value = get_trigflag( arg3 );
         }
         break;
   }
   if( tmp >= 0 && tmp <= 3 )
   {
      obj->value[tmp] = value;
      return;
   }

   progbugf( ch, "MpOset: Invalid field: {}", arg2 );
}

const std::string mprog_type_to_name( int type )
{
   switch ( type )
   {
      case IN_FILE_PROG:
         return "in_file_prog";
      case ACT_PROG:
         return "act_prog";
      case SPEECH_PROG:
         return "speech_prog";
      case SPEECH_AND_PROG:
         return "speech_and_prog";
      case RAND_PROG:
         return "rand_prog";
      case FIGHT_PROG:
         return "fight_prog";
      case HITPRCNT_PROG:
         return "hitprcnt_prog";
      case DEATH_PROG:
         return "death_prog";
      case ENTRY_PROG:
         return "entry_prog";
      case GREET_PROG:
         return "greet_prog";
      case ALL_GREET_PROG:
         return "all_greet_prog";
      case GIVE_PROG:
         return "give_prog";
      case BRIBE_PROG:
         return "bribe_prog";
      case HOUR_PROG:
         return "hour_prog";
      case TIME_PROG:
         return "time_prog";
      case MONTH_PROG:
         return "month_prog";
      case WEAR_PROG:
         return "wear_prog";
      case REMOVE_PROG:
         return "remove_prog";
      case SAC_PROG:
         return "sac_prog";
      case LOOK_PROG:
         return "look_prog";
      case EXA_PROG:
         return "exa_prog";
      case ZAP_PROG:
         return "zap_prog";
      case GET_PROG:
         return "get_prog";
      case DROP_PROG:
         return "drop_prog";
      case REPAIR_PROG:
         return "repair_prog";
      case DAMAGE_PROG:
         return "damage_prog";
      case PULL_PROG:
         return "pull_prog";
      case PUSH_PROG:
         return "push_prog";
      case SCRIPT_PROG:
         return "script_prog";
      case SLEEP_PROG:
         return "sleep_prog";
      case REST_PROG:
         return "rest_prog";
      case LEAVE_PROG:
         return "leave_prog";
      case USE_PROG:
         return "use_prog";
      case KEYWORD_PROG:
         return "keyword_prog";
      case SELL_PROG:
         return "sell_prog";
      case TELL_PROG:
         return "tell_prog";
      case TELL_AND_PROG:
         return "tell_and_prog";
      case CMD_PROG:
         return "command_prog";
      case EMOTE_PROG:
         return "emote_prog";
      case LOGIN_PROG:
         return "login_prog";
      case VOID_PROG:
         return "void_prog";
      case LOAD_PROG:
         return "load_prog";
      case GREET_IN_FIGHT_PROG:
         return "greet_in_fight_prog";
      default:
         return "ERROR_PROG";
   }
}

/* A trivial rehack of do_mstat.  This doesn't show all the data, but just
 * enough to identify the mob and give its basic condition.  It does however,
 * show the MUDprograms which are set.
 */
CMDF( do_mpstat )
{
   char_data *victim;
   short cnt = 0;

   if( argument.empty(  ) )
   {
      ch->print( "MProg stat whom?\r\n" );
      return;
   }

   if( !( victim = ch->get_char_world( argument ) ) )
   {
      ch->print( "They aren't here.\r\n" );
      return;
   }

   if( !victim->isnpc(  ) )
   {
      ch->print( "Only Mobiles can have MobPrograms!\r\n" );
      return;
   }

   if( victim->pIndexData->progtypes.none(  ) )
   {
      ch->print_fmt( "No programs on mobile: {} - #{}\r\n", victim->name, victim->pIndexData->vnum );
      return;
   }

   ch->print_fmt( "Name: {}.  Vnum: {}.\r\n", victim->name, victim->pIndexData->vnum );

   ch->print_fmt( "Short description: {}.\r\nLong  description: {}", victim->short_descr, !victim->long_descr.empty() ? victim->long_descr : "(none).\r\n" );

   ch->print_fmt( "Hp: {}/{}.  Mana: {}/{}.  Move: {}/{}. \r\n", victim->hit, victim->max_hit, victim->mana, victim->max_mana, victim->move, victim->max_move );

   ch->print_fmt( "Lv: {}.  Class: {}.  Align: {}.  AC: {}.  Gold: {}.  Exp: {}.\r\n",
               victim->level, victim->Class, victim->alignment, victim->GET_AC(  ), victim->gold, victim->exp );

   for( auto* prg : victim->pIndexData->mudprogs )
      ch->print_fmt( "{}{}>{} {}\r\n{}\r\n", ++cnt, ( prg->fileprog ? "(FILEPROG) " : "" ), mprog_type_to_name( prg->type ), prg->arglist, prg->comlist );
}

/* Opstat - Scryn 8/12*/
CMDF( do_opstat )
{
   obj_data *obj;
   short cnt = 0;

   if( argument.empty(  ) )
   {
      ch->print( "OProg stat what?\r\n" );
      return;
   }

   if( !( obj = ch->get_obj_world( argument ) ) )
   {
      ch->print( "You cannot find that.\r\n" );
      return;
   }

   if( obj->pIndexData->progtypes.none(  ) )
   {
      ch->printf( "No programs on object: %s - #%d\r\n", obj->short_descr, obj->pIndexData->vnum );
      return;
   }

   ch->printf( "Name: %s.  Vnum: %d.\r\n", obj->name, obj->pIndexData->vnum );
   ch->printf( "Short description: %s.\r\n", obj->short_descr );

   for( auto* mprog : obj->pIndexData->mudprogs )
      ch->printf( "%d >%s %s\r\n%s\r\n", ++cnt, mprog_type_to_name( mprog->type ).c_str(  ), mprog->arglist, mprog->comlist );
}

/* Rpstat - Scryn 8/12 */
CMDF( do_rpstat )
{
   room_index *location;
   short cnt = 0;

   if( argument.empty(  ) )
      location = ch->in_room;
   else
      location = ch->find_location( argument );

   if( !location )
   {
      bug( "%s: Bad room index: %s", __func__, argument.c_str(  ) );
      ch->print( "That room does not exist!\r\n" );
      return;
   }

   if( location->progtypes.none(  ) )
   {
      ch->printf( "No programs on room: %s - #%d\r\n", ch->in_room->name, ch->in_room->vnum );
      return;
   }

   ch->printf( "Name: %s.  Vnum: %d.\r\n", location->name, location->vnum );

   for( auto* mprog : location->mudprogs )
      ch->printf( "%d >%s %s\r\n%s\r\n", ++cnt, mprog_type_to_name( mprog->type ).c_str(  ), mprog->arglist, mprog->comlist );
}

/* Woowoo - Blodkai, November 1997 */
CMDF( do_mpasupress )
{
   std::string arg1;
   char_data *victim;
   int rnds;

   if( !can_use_mprog( ch ) )
      return;

   argument = one_argument( argument, arg1 );

   if( arg1.empty(  ) )
   {
      progbug( "Mpasupress:  invalid (nonexistent?) argument", ch );
      return;
   }

   if( argument.empty(  ) )
   {
      progbug( "Mpasupress:  invalid (nonexistent?) second argument", ch );
      return;
   }

   if( !( victim = ch->get_char_room( arg1 ) ) )
   {
      progbug( "Mpasupress: victim not present", ch );
      return;
   }

   rnds = atoi( argument.c_str(  ) );
   if( rnds < 0 || rnds > 32000 )
   {
      progbug( "Mpsupress: invalid rounds argument, non-numeric", ch );
      return;
   }
   victim->add_timer( TIMER_ASUPRESSED, rnds, nullptr, 0 );
}

/* lets the mobile kill any player or mobile without murder*/
CMDF( do_mpkill )
{
   char_data *victim;

   if( !ch )
   {
      bug( "%s: Nonexistent ch!", __func__ );
      return;
   }

   if( !can_use_mprog( ch ) )
      return;

   if( argument.empty(  ) )
   {
      progbug( "MpKill - no argument", ch );
      return;
   }

   if( !( victim = ch->get_char_room( argument ) ) )
   {
      progbugf( ch, "MpKill - Victim {} not in room", argument );
      return;
   }

   if( victim == ch )
   {
      progbug( "MpKill - Bad victim (self) to attack", ch );
      return;
   }

   if( ch->position > POS_SITTING && ch->position < POS_STANDING )
   {
      progbug( "MpKill - Already fighting", ch );
      return;
   }
   multi_hit( ch, victim, TYPE_UNDEFINED );
}

/* lets the mobile destroy an object in its inventory
   it can also destroy a worn object and it can destroy
   items using all.xxxxx or just plain all of them */
CMDF( do_mpjunk )
{
   if( !can_use_mprog( ch ) )
      return;

   if( argument.empty(  ) )
   {
      progbug( "Mpjunk - No argument", ch );
      return;
   }

   obj_data *obj;
   if( str_cmp( argument, "all" ) && str_prefix( "all.", argument ) )
   {
      if( ( obj = ch->get_obj_wear( argument ) ) != nullptr )
      {
         ch->unequip( obj );
         obj->extract(  );
         return;
      }
      if( !( obj = ch->get_obj_carry( argument ) ) )
         return;
      obj->extract(  );
   }
   else
   {
      std::list<obj_data *>::iterator iobj;
      for( iobj = ch->carrying.begin(  ); iobj != ch->carrying.end(  ); )
      {
         obj = *iobj;
         ++iobj;

         if( !str_cmp( argument, "all" ) || hasname( obj->name, argument.substr( 4, argument.length(  ) ) ) )
         {
            if( obj->wear_loc != WEAR_NONE )
               ch->unequip( obj );
            obj->extract(  );
         }
      }
   }
}

/* Prints the argument to all the rooms around the mobile */
CMDF( do_mpasound )
{
   if( !can_use_mprog( ch ) )
      return;

   if( argument.empty(  ) )
   {
      progbug( "Mpasound - No argument", ch );
      return;
   }

   std::bitset<MAX_ACT_FLAG> actflags = ch->get_actflags(  );
   ch->unset_actflag( ACT_SECRETIVE );
   /*
    * DONT_UPPER prevents argument[0] from being capitalized. --Shaddai
    */
   DONT_UPPER = true;
   room_index *was_in_room = ch->in_room;
   for( auto* pexit : was_in_room->exits )
   {
      if( pexit->to_room && pexit->to_room != was_in_room )
      {
         ch->from_room(  );
         if( !ch->to_room( pexit->to_room ) )
            log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __func__, __LINE__ );
         MOBtrigger = false;
         act( AT_ACTION, argument, ch, nullptr, nullptr, TO_ROOM );
      }
   }
   DONT_UPPER = false;  /* Always set it back to false */
   ch->set_actflags( actflags );
   ch->from_room(  );
   if( !ch->to_room( was_in_room ) )
      log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __func__, __LINE__ );
}

/* prints the message to all in the room other than the mob and victim */
CMDF( do_mpechoaround )
{
   char_data *victim;
   std::string arg;
   std::bitset<MAX_ACT_FLAG> actflags;

   if( !can_use_mprog( ch ) )
      return;

   if( argument.empty(  ) )
   {
      progbug( "Mpechoaround - No argument", ch );
      return;
   }

   argument = one_argument( argument, arg );

   if( !( victim = ch->get_char_room( arg ) ) )
   {
      progbugf( ch, "Mpechoaround - victim {} does not exist", arg );
      return;
   }

   actflags = ch->get_actflags(  );
   ch->unset_actflag( ACT_SECRETIVE );
   act( AT_ACTION, argument, ch, nullptr, victim, TO_NOTVICT );
   ch->set_actflags( actflags );
}

/* prints message only to victim */
CMDF( do_mpechoat )
{
   char_data *victim;
   std::string arg;
   std::bitset<MAX_ACT_FLAG> actflags;

   if( !can_use_mprog( ch ) )
      return;

   if( argument.empty(  ) )
   {
      progbug( "Mpechoat - No argument", ch );
      return;
   }

   argument = one_argument( argument, arg );
   if( !( victim = ch->get_char_room( arg ) ) )
   {
      progbugf( ch, "Mpechoat - victim {} does not exist", arg );
      return;
   }

   actflags = ch->get_actflags(  );
   ch->unset_actflag( ACT_SECRETIVE );

   DONT_UPPER = true;
   if( argument.empty(  ) )
      act( AT_ACTION, " ", ch, nullptr, victim, TO_VICT );
   else
      act( AT_ACTION, argument, ch, nullptr, victim, TO_VICT );

   DONT_UPPER = false;

   ch->set_actflags( actflags );
}

/* prints message to room at large. */
CMDF( do_mpecho )
{
   std::bitset<MAX_ACT_FLAG> actflags;

   if( !can_use_mprog( ch ) )
      return;

   actflags = ch->get_actflags(  );
   ch->unset_actflag( ACT_SECRETIVE );

   DONT_UPPER = true;
   if( argument.empty(  ) )
      act( AT_ACTION, " ", ch, nullptr, nullptr, TO_ROOM );
   else
      act( AT_ACTION, argument, ch, nullptr, nullptr, TO_ROOM );
   DONT_UPPER = false;
   ch->set_actflags( actflags );
}

/*
 * Lets the mobile load an item or mobile. All items are loaded into inventory.
 * You can specify a level with the load object portion as well. 
 */
CMDF( do_mpmload )
{
   mob_index *pMobIndex;
   char_data *victim;

   if( !can_use_mprog( ch ) )
      return;

   if( argument.empty(  ) || !is_number( argument ) )
   {
      progbugf( ch, "Mpmload - Bad vnum {} as argument", !argument.empty(  ) ? argument : "nullptr" );
      return;
   }

   if( !( pMobIndex = get_mob_index( atoi( argument.c_str(  ) ) ) ) )
   {
      progbugf( ch, "Mpmload - Bad mob vnum {}", argument );
      return;
   }

   victim = pMobIndex->create_mobile(  );
   if( !victim->to_room( ch->in_room ) )
      log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __func__, __LINE__ );
}

CMDF( do_mpoload )
{
   std::string arg1, arg2;
   obj_data *obj;
   int level, timer = 0;

   if( !can_use_mprog( ch ) )
      return;

   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );

   if( arg1.empty(  ) || !is_number( arg1 ) )
   {
      progbug( "Mpoload - Bad syntax", ch );
      return;
   }

   if( arg2.empty(  ) )
      level = ch->get_trust(  );
   else
   {
      /*
       * New feature from Alander.
       */
      if( !is_number( arg2 ) )
      {
         progbugf( ch, "Mpoload - Bad level syntax: {}", arg2 );
         return;
      }

      level = atoi( arg2.c_str(  ) );
      if( level < 0 || level > ch->get_trust(  ) )
      {
         progbugf( ch, "Mpoload - Bad level {}", level );
         return;
      }

      /*
       * New feature from Thoric.
       */
      timer = atoi( argument.c_str(  ) );
      if( timer < 0 )
      {
         progbugf( ch, "Mpoload - Bad timer {}", timer );
         return;
      }
   }

   if( !( obj = get_obj_index( atoi( arg1.c_str(  ) ) )->create_object( level ) ) )
   {
      log_printf( "create_object: %s:%s, line %d.", __FILE__, __func__, __LINE__ );
      progbugf( ch, "Mpoload - Bad vnum arg {}", arg1 );
      return;
   }

   obj->timer = timer;

   // Bugfix - objects & rooms use supermob, so he should ALWAYS drop stuff.
   if( obj->wear_flags.test( ITEM_TAKE ) && ch != supermob )
      obj->to_char( ch );
   else
      obj->to_room( ch->in_room, ch );
}

/* Just a hack of do_pardon from act_wiz.c -- Blodkai, 6/15/97 */
CMDF( do_mppardon )
{
   std::string arg1;
   char_data *victim;

   if( !can_use_mprog( ch ) )
      return;

   argument = one_argument( argument, arg1 );

   if( arg1.empty(  ) || argument.empty(  ) )
   {
      progbug( "Mppardon: missing argument", ch );
      return;
   }

   if( !( victim = ch->get_char_room( arg1 ) ) )
   {
      progbugf( ch, "Mppardon: offender {} not present", arg1 );
      return;
   }

   if( victim->isnpc(  ) )
   {
      progbugf( ch, "Mppardon:  trying to pardon NPC {}", victim->short_descr );
      return;
   }

   if( !str_cmp( argument, "litterbug" ) )
   {
      if( victim->has_pcflag( PCFLAG_LITTERBUG ) )
      {
         victim->unset_pcflag( PCFLAG_LITTERBUG );
         victim->print( "Your crime of littering has been pardoned.\r\n" );
      }
      return;
   }
   progbug( "Mppardon: Invalid argument", ch );
}

/* lets the mobile purge all objects and other npcs in the room,
   or purge a specified object or mob in the room.  It can purge
   itself, but this had best be the last command in the MUDprogram
   otherwise ugly stuff will happen */
CMDF( do_mppurge )
{
   if( !can_use_mprog( ch ) )
      return;

   if( argument.empty(  ) )
   {
      /*
       * 'purge' 
       */
      if( ch->has_actflag( ACT_ONMAP ) )
      {
         progbug( "mppurge: Room purge called from overland map", ch );
         return;
      }

      for( auto it = ch->in_room->people.begin(); it != ch->in_room->people.end(); )
      {
         char_data *victim = *it;
         ++it;

         if( victim->isnpc(  ) && victim != ch )
            victim->extract( true );
      }

      for( auto it = ch->in_room->objects.begin(); it != ch->in_room->objects.end(); )
      {
         obj_data *obj = *it;
         ++it;

         obj->extract(  );
      }
      return;
   }

   char_data *victim;
   obj_data *obj;
   if( !( victim = ch->get_char_room( argument ) ) )
   {
      if( ( obj = ch->get_obj_here( argument ) ) != nullptr )
      {
         obj->separate( );
         obj->extract( );
      }
      else
         progbug( "Mppurge - Bad argument", ch );
      return;
   }

   if( !victim->isnpc(  ) )
   {
      progbugf( ch, "Mppurge - Trying to purge a PC {}", victim->name );
      return;
   }

   if( victim == ch )
   {
      progbug( "Mppurge - Trying to purge oneself", ch );
      return;
   }

   if( victim->isnpc(  ) && victim->pIndexData->vnum == MOB_VNUM_SUPERMOB )
   {
      progbug( "Mppurge: trying to purge supermob", ch );
      return;
   }
   victim->extract( true );
}

/* Allow mobiles to go wizinvis with programs -- SB */
CMDF( do_mpinvis )
{
   short level;

   if( !can_use_mprog( ch ) )
      return;

   if( !argument.empty(  ) )
   {
      if( !is_number( argument ) )
      {
         progbug( "Mpinvis - Non numeric argument ", ch );
         return;
      }

      level = atoi( argument.c_str(  ) );
      if( level < 2 || level > LEVEL_IMMORTAL ) /* Updated hardcode level check - Samson */
      {
         progbugf( ch, "MPinvis - Invalid level {}", level );
         return;
      }
      ch->mobinvis = level;
      return;
   }

   if( ch->mobinvis < 2 )
      ch->mobinvis = ch->level;

   if( ch->has_actflag( ACT_MOBINVIS ) )
   {
      ch->unset_actflag( ACT_MOBINVIS );
      act( AT_IMMORT, "$n slowly fades into existence.", ch, nullptr, nullptr, TO_ROOM );
   }
   else
   {
      ch->set_actflag( ACT_MOBINVIS );
      act( AT_IMMORT, "$n slowly fades into thin air.", ch, nullptr, nullptr, TO_ROOM );
   }
}

/* lets the mobile goto any location it wishes that is not private */
/* Mounted chars follow their mobiles now - Blod, 11/97 */
CMDF( do_mpgoto )
{
   room_index *location;

   if( !can_use_mprog( ch ) )
      return;

   if( argument.empty(  ) )
   {
      progbug( "Mpgoto - No argument", ch );
      return;
   }

   if( !( location = ch->find_location( argument ) ) )
   {
      progbugf( ch, "Mpgoto - No such location: {}", argument );
      return;
   }

   if( ch->fighting )
      ch->stop_fighting( true );

   leave_map( ch, nullptr, location );
}

/* lets the mobile do a command at another location. Very useful */
CMDF( do_mpat )
{
   std::string arg;
   room_index *location, *original;

   if( !can_use_mprog( ch ) )
      return;

   argument = one_argument( argument, arg );

   if( arg.empty(  ) || argument.empty(  ) )
   {
      progbug( "Mpat - Bad argument", ch );
      return;
   }

   if( !( location = ch->find_location( arg ) ) )
   {
      progbugf( ch, "Mpat - No such location: {}", arg );
      return;
   }

   original = ch->in_room;
   ch->from_room(  );
   if( !ch->to_room( location ) )
      log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __func__, __LINE__ );
   interpret( ch, argument );

   if( !ch->char_died(  ) )
   {
      ch->from_room(  );
      if( !ch->to_room( original ) )
         log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __func__, __LINE__ );
   }
}

/* allow a mobile to advance a player's level... very dangerous */
CMDF( do_mpadvance )
{
   std::string arg;
   char_data *victim;
   int level, iLevel;

   if( !can_use_mprog( ch ) )
      return;


   if( argument.empty(  ) )
   {
      progbug( "Mpadvance - Bad syntax", ch );
      return;
   }

   argument = one_argument( argument, arg );

   if( !( victim = ch->get_char_room( arg ) ) )
   {
      progbugf( ch, "Mpadvance - Victim {} not there", arg );
      return;
   }

   if( victim->isnpc(  ) )
   {
      progbugf( ch, "Mpadvance - Victim {} is NPC", victim->name );
      return;
   }

   if( victim->level >= LEVEL_AVATAR )
      return;

   level = atoi( argument.c_str(  ) );

   if( victim->level > ch->level )
   {
      act( AT_TELL, "$n tells you, 'Sorry... you must seek someone more powerful than I.'", ch, nullptr, victim, TO_VICT );
      return;
   }

   switch ( level )
   {
      default:
         victim->print( "You feel more powerful!\r\n" );
         break;
   }

   for( iLevel = victim->level; iLevel < level; ++iLevel )
   {
      if( level < LEVEL_IMMORTAL )
         victim->print( "You raise a level!!  " );
      victim->level += 1;
      advance_level( victim );
   }

   /*
    * Modified by Samson 4-30-99 
    */
   victim->exp = exp_level( victim->level );
   victim->trust = 0;
}

/* lets the mobile transfer people.  the all argument transfers
   everyone in the current room to the specified location 
   the area argument transfers everyone in the current area to the
   specified location */
CMDF( do_mptransfer )
{
   std::string arg1;
   room_index *location;
   char_data *victim;
   std::list<char_data *>::iterator ich;

   if( !can_use_mprog( ch ) )
      return;

   argument = one_argument( argument, arg1 );

   if( arg1.empty(  ) )
   {
      progbug( "Mptransfer - Bad syntax", ch );
      return;
   }

   /*
    * Thanks to Grodyn for the optional location parameter.
    */
   if( argument.empty(  ) )
      location = ch->in_room;
   else
   {
      if( !( location = ch->find_location( argument ) ) )
      {
         progbugf( ch, "Mptransfer - no such location: to: {} from: {}", argument, ch->in_room->vnum );
         return;
      }
   }

   /*
    * Put in the variable nextinroom to make this work right. -Narn 
    */
   if( !str_cmp( arg1, "all" ) )
   {
      for( ich = ch->in_room->people.begin(  ); ich != ch->in_room->people.end(  ); )
      {
         victim = *ich;
         ++ich;

         if( ch == victim )
            continue;

         transfer_char( ch, victim, location );
      }
      return;
   }

   /*
    * This will only transfer PC's in the area not Mobs --Shaddai 
    */
   if( !str_cmp( arg1, "area" ) )
   {
      for( ich = pclist.begin(  ); ich != pclist.end(  ); ++ich )
      {
         victim = *ich;

         if( ch->in_room->area != victim->in_room->area || victim->level == 1 )  /* new auth */
            continue;
         transfer_char( ch, victim, location );
      }
      return;
   }

   if( !( victim = ch->get_char_world( arg1 ) ) )
   {
      progbugf( ch, "Mptransfer - No such person {}", arg1 );
      return;
   }

   /*
    * Alert, cheesy hack sighted on scanners sir! 
    */
   victim->tempnum = 3210;
   transfer_char( ch, victim, location );
   victim->tempnum = 0;
}

/*
 * mpbodybag for mobs to do cr's  --Shaddai
 */
CMDF( do_mpbodybag )
{
   std::string arg;
   char_data *victim;

   if( !can_use_mprog( ch ) )
      return;

   argument = one_argument( argument, arg );

   if( arg.empty(  ) )
   {
      progbug( "Mpbodybag - called w/o enough argument(s)", ch );
      return;
   }

   if( !( victim = ch->get_char_room( arg ) ) )
   {
      progbugf( ch, "Mpbodybag: victim {} not in room", arg );
      return;
   }

   if( victim->isnpc(  ) )
   {
      progbug( "Mpbodybag: bodybagging a npc corpse", ch );
      return;
   }

   std::string buf = std::format( "the corpse of {}", arg );

   for( auto* obj : objlist )
   {
      if( obj->in_room && !str_cmp( buf, obj->short_descr ) && ( obj->pIndexData->vnum == OBJ_VNUM_CORPSE_PC ) )
      {
         obj->from_room(  );
         obj = obj->to_char( ch );
         obj->timer = -1;
      }
   }

   /*
    * Maybe should just make the command logged... Shrug I am not sure
    * * --Shaddai
    */
   log_printf( "Mpbodybag: Grabbed %s", buf.c_str() );
}

/*
 * mpmorph and mpunmorph for morphing people with mobs. --Shaddai
 */
CMDF( do_mpmorph )
{
   char_data *victim;
   morph_data *morph;
   std::string arg1;

   if( !can_use_mprog( ch ) )
      return;

   argument = one_argument( argument, arg1 );

   if( arg1.empty(  ) || argument.empty(  ) )
   {
      progbug( "Mpmorph - called w/o enough argument(s)", ch );
      return;
   }

   if( !( victim = ch->get_char_room( arg1 ) ) )
   {
      progbugf( ch, "Mpmorph: victim {} not in room", arg1 );
      return;
   }

   if( !is_number( argument ) )
      morph = get_morph( argument );
   else
      morph = get_morph_vnum( atoi( argument.c_str(  ) ) );
   if( !morph )
   {
      progbugf( ch, "Mpmorph - unknown morph {}", argument );
      return;
   }

   if( victim->morph )
   {
      progbugf( ch, "Mpmorph - victim {} already morphed", victim->name );
      return;
   }
   do_morph_char( victim, morph );
}

CMDF( do_mpunmorph )
{
   char_data *victim;

   if( !can_use_mprog( ch ) )
      return;

   if( argument.empty(  ) )
   {
      progbug( "Mpmorph - called w/o an argument", ch );
      return;
   }

   if( !( victim = ch->get_char_room( argument ) ) )
   {
      progbugf( ch, "Mpunmorph: victim {} not in room", argument );
      return;
   }

   if( !victim->morph )
   {
      progbugf( ch, "Mpunmorph: victim {} not morphed", victim->name );
      return;
   }
   do_unmorph_char( victim );
}

CMDF( do_mpechozone )   /* Blod, late 97 */
{
   if( !can_use_mprog( ch ) )
      return;

   std::bitset<MAX_ACT_FLAG> actflags = ch->get_actflags(  );
   ch->unset_actflag( ACT_SECRETIVE );
   DONT_UPPER = true;
   for( auto it = pclist.begin(); it != pclist.end(); )
   {
      char_data *vch = *it;
      ++it;

      if( vch->in_room->area == ch->in_room->area && vch->IS_AWAKE(  ) )
      {
         if( argument.empty(  ) )
            act( AT_ACTION, " ", vch, nullptr, nullptr, TO_CHAR );
         else
            act( AT_ACTION, argument, vch, nullptr, nullptr, TO_CHAR );
      }
   }
   DONT_UPPER = false;
   ch->set_actflags( actflags );
}

/*
 *  Haus' toys follow:
 */

/*
 * syntax:  mppractice victim spell_name max%
 *
 */
CMDF( do_mp_practice )
{
   std::string arg1, arg2;
   char_data *victim;
   int sn, max, adept;
   char *skillname;

   if( !can_use_mprog( ch ) )
      return;

   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );

   if( arg1.empty(  ) || arg2.empty(  ) || argument.empty(  ) )
   {
      progbug( "Mppractice - Bad syntax", ch );
      return;
   }

   if( !( victim = ch->get_char_room( arg1 ) ) )
   {
      progbugf( ch, "Mppractice: Invalid student {} not in room", arg1 );
      return;
   }

   if( ( sn = skill_lookup( arg2 ) ) < 0 )
   {
      progbugf( ch, "Mppractice: Invalid spell/skill name {}", arg2 );
      return;
   }

   if( victim->isnpc(  ) )
   {
      progbug( "Mppractice: Can't train a mob", ch );
      return;
   }

   skillname = skill_table[sn]->name;

   max = atoi( argument.c_str(  ) );
   if( max < 0 || max > 100 )
   {
      progbugf( ch, "mp_practice: Invalid maxpercent: {}", max );
      return;
   }

   if( victim->level < skill_table[sn]->skill_level[victim->Class] )
   {
      act_printf( AT_TELL, ch, nullptr, victim, TO_VICT, "$n attempts to tutor you in {}, but it's beyond your comprehension.", skillname );
      return;
   }

   /*
    * adept is how high the player can learn it 
    * adept = class_table[ch->Class]->skill_adept; 
    */
   adept = victim->GET_ADEPT( sn );

   if( ( victim->pcdata->learned[sn] >= adept ) || ( victim->pcdata->learned[sn] >= max ) )
   {
      act_printf( AT_TELL, ch, nullptr, victim, TO_VICT, "$n shows some knowledge of {}, but yours is clearly superior.", skillname );
      return;
   }

   /*
    * past here, victim learns something 
    */
   act( AT_ACTION, "$N demonstrates $t to you. You feel more learned in this subject.", victim, skill_table[sn]->name, ch, TO_CHAR );

   victim->pcdata->learned[sn] = max;

   if( victim->pcdata->learned[sn] >= adept )
   {
      victim->pcdata->learned[sn] = adept;
      act( AT_TELL, "$n tells you, 'You have learned all I know on this subject...'", ch, nullptr, victim, TO_VICT );
   }
}

CMDF( do_mpstrew )
{
   std::string arg1;
   char_data *victim;
   room_index *pRoomIndex;
   int rvnum, vnum;

   if( !can_use_mprog( ch ) )
      return;

   argument = one_argument( argument, arg1 );

   if( arg1.empty(  ) )
   {
      progbug( "Mpstrew: invalid (nonexistent?) argument", ch );
      return;
   }

   if( !( victim = ch->get_char_room( arg1 ) ) )
   {
      progbugf( ch, "Mpstrew: victim {} not in room", arg1 );
      return;
   }

   if( argument.empty(  ) )
   {
      progbug( "Mpstrew: No command arguments", ch );
      return;
   }

   if( !str_cmp( argument, "coins" ) )
   {
      if( victim->gold < 1 )
         return;
      victim->gold = 0;
      return;
   }

   obj_data *obj_lose;
   std::list<obj_data *>::iterator iobj;
   if( !str_cmp( argument, "inventory" ) )
   {
      for( ;; )
      {
         rvnum = number_range( 1, sysdata->maxvnum );
         pRoomIndex = get_room_index( rvnum );
         if( pRoomIndex && !pRoomIndex->flags.test( ROOM_MAP ) )
            break;
      }

      for( iobj = victim->carrying.begin(  ); iobj != victim->carrying.end(  ); )
      {
         obj_lose = *iobj;
         ++iobj;

         obj_lose->from_char(  );
         obj_lose->to_room( pRoomIndex, nullptr );
      }
      return;
   }

   vnum = atoi( argument.c_str(  ) );

   for( iobj = victim->carrying.begin(  ); iobj != victim->carrying.end(  ); )
   {
      obj_lose = *iobj;
      ++iobj;

      if( obj_lose->pIndexData->vnum == vnum )
      {
         obj_lose->from_char(  );
         obj_lose->extract(  );
      }
   }
}

CMDF( do_mpscatter )
{
   char_data *victim;
   room_index *pRoomIndex;
   int schance;

   if( !can_use_mprog( ch ) )
      return;

   if( argument.empty(  ) )
   {
      progbug( "Mpscatter: invalid (nonexistent?) argument", ch );
      return;
   }

   if( !( victim = ch->get_char_room( argument ) ) )
   {
      progbugf( ch, "Mpscatter: victim {} not in room", argument );
      return;
   }

   if( victim->level == LEVEL_SUPREME )
   {
      progbugf( ch, "Mpscatter: victim {} level too high", victim->name );
      return;
   }

   schance = number_range( 1, 2 );

   if( schance == 1 )
   {
      continent_data *continent;
      int x, y;
      short sector;

      for( ;; )
      {
         continent = pick_random_continent( );
         x = number_range( 0, MAX_X - 1 );
         y = number_range( 0, MAX_Y - 1 );

         if( continent )
         {
            sector = continent->get_terrain( x, y );
            if( sector == -1 )
               continue;
            if( sect_show[sector].canpass )
               break;
         }
      }

      if( continent )
      {
         act( AT_MAGIC, "With the sweep of an arm, $n flings $N to the astral winds.", ch, nullptr, victim, TO_NOTVICT );
         act( AT_MAGIC, "With the sweep of an arm, $n flings you to the astral winds.", ch, nullptr, victim, TO_VICT );
         act( AT_MAGIC, "With the sweep of an arm, you fling $N to the astral winds.", ch, nullptr, victim, TO_CHAR );
         enter_map( victim, nullptr, x, y, continent->name );
         victim->position = POS_STANDING;
         act( AT_MAGIC, "$n is deposited in a heap by the astral winds.", victim, nullptr, nullptr, TO_ROOM );
      }
      else
         return;
   }
   else
   {
      for( ;; )
      {
         pRoomIndex = get_room_index( number_range( 0, sysdata->maxvnum ) );
         if( pRoomIndex )
            if( !pRoomIndex->flags.test( ROOM_PRIVATE ) && !pRoomIndex->flags.test( ROOM_SOLITARY ) && !pRoomIndex->flags.test( ROOM_PROTOTYPE ) )
               break;
      }
      if( victim->fighting )
         victim->stop_fighting( true );
      act( AT_MAGIC, "With the sweep of an arm, $n flings $N to the astral winds.", ch, nullptr, victim, TO_NOTVICT );
      act( AT_MAGIC, "With the sweep of an arm, $n flings you to the astral winds.", ch, nullptr, victim, TO_VICT );
      act( AT_MAGIC, "With the sweep of an arm, you fling $N to the astral winds.", ch, nullptr, victim, TO_CHAR );
      victim->from_room(  );
      if( !victim->to_room( pRoomIndex ) )
         log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __func__, __LINE__ );
      victim->position = POS_RESTING;
      act( AT_MAGIC, "$n is deposited in a heap by the astral winds.", victim, nullptr, nullptr, TO_ROOM );
      interpret( victim, "look" );
   }
}

/*
 * syntax: mpslay (character)
 */
CMDF( do_mp_slay )
{
   char_data *victim;

   if( !can_use_mprog( ch ) )
      return;

   if( argument.empty(  ) )
   {
      progbug( "Mpslay: invalid (nonexistent?) argument", ch );
      return;
   }

   if( !( victim = ch->get_char_room( argument ) ) )
   {
      progbugf( ch, "Mpslay: victim {} not in room", argument );
      return;
   }

   if( victim == ch )
   {
      progbug( "Mpslay: trying to slay self", ch );
      return;
   }

   if( victim->isnpc(  ) && victim->pIndexData->vnum == MOB_VNUM_SUPERMOB )
   {
      progbug( "Mpslay: trying to slay supermob", ch );
      return;
   }

   if( victim->level < LEVEL_SUPREME )
   {
      act( AT_IMMORT, "You slay $M in cold blood!", ch, nullptr, victim, TO_CHAR );
      act( AT_IMMORT, "$n slays you in cold blood!", ch, nullptr, victim, TO_VICT );
      act( AT_IMMORT, "$n slays $N in cold blood!", ch, nullptr, victim, TO_NOTVICT );
      raw_kill( ch, victim );
      ch->stop_fighting( false );
      stop_hating( ch );
      stop_fearing( ch );
      stop_hunting( ch );
   }
   else
   {
      act( AT_IMMORT, "You attempt to slay $M, but the All Mighty protects $M!", ch, nullptr, victim, TO_CHAR );
      act( AT_IMMORT, "$n attempts to slay you. The Almighty is snickering in the corner.", ch, nullptr, victim, TO_VICT );
      act( AT_IMMORT, "$n attempts to slay $N, but the All Mighty protects $M!", ch, nullptr, victim, TO_NOTVICT );
   }
}

/*
 * Inflict damage from a mudprogram
 *
 *  note: should be careful about using victim afterwards
 */
ch_ret simple_damage( char_data * ch, char_data * victim, double dam, int dt )
{
   short dameq;
   bool npcvict;
   obj_data *damobj;

   if( !ch )
   {
      bug( "%s: null ch!", __func__ );
      return rERROR;
   }
   if( !victim )
   {
      progbug( "simple_damage: null victim!", ch );
      return rVICT_DIED;
   }

   if( victim->position == POS_DEAD )
      return rVICT_DIED;

   npcvict = victim->isnpc(  );

   if( dam )
   {
      dam = damage_risa( victim, dam, dt );

      if( dam < 0 )
         dam = 0;
   }

   if( victim != ch )
   {
      /*
       * Damage modifiers.
       */
      if( victim->has_aflag( AFF_SANCTUARY ) )
         dam /= 2;

      if( victim->has_aflag( AFF_PROTECT ) && ch->IS_EVIL(  ) )
         dam -= ( int )( dam / 4 );

      if( dam < 0 )
         dam = 0;
   }

   /*
    * Check for EQ damage.... ;)
    */
   if( dam > 10 )
   {
      /*
       * get a random body eq part 
       */
      dameq = number_range( WEAR_LIGHT, WEAR_EYES );
      damobj = victim->get_eq( dameq );
      if( damobj )
      {
         if( dam > damobj->get_resistance(  ) )
            damage_obj( damobj );
      }
   }

   /*
    * Hurt the victim.
    * Inform the victim of his new state.
    */
   victim->hit -= ( short )dam;
   if( !victim->isnpc(  ) && victim->level >= LEVEL_IMMORTAL && victim->hit < 1 )
      victim->hit = 1;

   if( !npcvict && victim->get_trust(  ) >= LEVEL_IMMORTAL && ch->get_trust(  ) >= LEVEL_IMMORTAL && victim->hit < 1 )
      victim->hit = 1;
   victim->update_pos(  );

   switch ( victim->position )
   {
      case POS_MORTAL:
         act( AT_DYING, "$n is mortally wounded, and will die soon, if not aided.", victim, nullptr, nullptr, TO_ROOM );
         act( AT_DANGER, "You are mortally wounded, and will die soon, if not aided.", victim, nullptr, nullptr, TO_CHAR );
         break;

      case POS_INCAP:
         act( AT_DYING, "$n is incapacitated and will slowly die, if not aided.", victim, nullptr, nullptr, TO_ROOM );
         act( AT_DANGER, "You are incapacitated and will slowly die, if not aided.", victim, nullptr, nullptr, TO_CHAR );
         break;

      case POS_STUNNED:
         if( !victim->has_aflag( AFF_PARALYSIS ) )
         {
            act( AT_ACTION, "$n is stunned, but will probably recover.", victim, nullptr, nullptr, TO_ROOM );
            act( AT_HURT, "You are stunned, but will probably recover.", victim, nullptr, nullptr, TO_CHAR );
         }
         break;

      case POS_DEAD:
         act( AT_DEAD, "$n is DEAD!!", victim, 0, 0, TO_ROOM );
         act( AT_DEAD, "You have been KILLED!!\r\n", victim, 0, 0, TO_CHAR );
         break;

      default:
         if( dam > victim->max_hit / 4 )
            act( AT_HURT, "That really did HURT!", victim, 0, 0, TO_CHAR );
         if( victim->hit < victim->max_hit / 4 )
            act( AT_DANGER, "You wish that your wounds would stop BLEEDING so much!", victim, 0, 0, TO_CHAR );
         break;
   }

   /*
    * Payoff for killing things.
    */
   if( victim->position == POS_DEAD )
   {
      if( !npcvict )
      {
         log_printf_plus( LOG_INFO, LEVEL_IMMORTAL, "%s (%d) killed by %s at %d",
                          victim->name.c_str(), victim->level, ( ch->isnpc(  ) ? ch->short_descr.c_str() : ch->name.c_str() ), victim->in_room->vnum );

         /*
          * Dying penalty:
          * 1/2 way back to previous level.
          */
         if( victim->exp > exp_level( victim->level ) )
            victim->gain_exp( ( exp_level( victim->level ) - victim->exp ) / 2 );
      }
      raw_kill( ch, victim );
      victim = nullptr;
      return rVICT_DIED;
   }

   if( victim == ch )
      return rNONE;

   /*
    * Take care of link dead people.
    */
   if( !npcvict && !victim->desc )
   {
      if( number_range( 0, victim->wait ) == 0 )
      {
         recall( victim, -1 );
         return rNONE;
      }
   }

   /*
    * Wimp out?
    */
   if( npcvict && dam > 0 )
   {
      if( ( victim->has_actflag( ACT_WIMPY ) && number_bits( 1 ) == 0 && victim->hit < victim->max_hit / 2 )
          || ( victim->has_aflag( AFF_CHARM ) && victim->master && victim->master->in_room != victim->in_room ) )
      {
         start_fearing( victim, ch );
         stop_hunting( victim );
         interpret( victim, "flee" );
      }
   }

   if( !npcvict && victim->hit > 0 && victim->hit <= victim->wimpy && victim->wait == 0 )
      interpret( victim, "flee" );

   return rNONE;
}

/*
 * syntax: mpdamage (character) (#hps)
 */
CMDF( do_mp_damage )
{
   std::string arg1;
   char_data *victim;

   if( !can_use_mprog( ch ) )
      return;

   argument = one_argument( argument, arg1 );

   if( arg1.empty(  ) )
   {
      progbug( "Mpdamage: missing arg1", ch );
      return;
   }

   /*
    * Am I asking for trouble here or what?  But I need it. -- Blodkai 
    */
   if( !str_cmp( arg1, "all" ) )
   {
      for( auto it = ch->in_room->people.begin(); it != ch->in_room->people.end(); )
      {
         victim = *it;
         ++it;

         if( victim != ch && ch->can_see( victim, false ) ) /* Could go either way */
            funcf( ch, do_mp_damage, "'{}' {}", victim->name, argument );
      }
      return;
   }

   if( argument.empty(  ) )
   {
      progbug( "Missing argument", ch );
      return;
   }

   if( !( victim = ch->get_char_room( arg1 ) ) )
   {
      progbugf( ch, "{}: victim {} not in room", __func__, arg1 );
      return;
   }

   if( victim == ch )
   {
      progbug( "trying to damage self", ch );
      return;
   }

   double dam = atoi( argument.c_str(  ) );
   if( dam < 0 || dam > 32000 )
   {
      progbugf( ch, "Mpdamage: invalid (nonexistent?) argument {}", dam );
      return;
   }

   /*
    * this is kinda begging for trouble        
    * Note from Thoric to whoever put this in...
    * Wouldn't it be better to call damage(ch, ch, dam, dt)?
    * I hate redundant code
    */
   if( simple_damage( ch, victim, dam, TYPE_UNDEFINED ) == rVICT_DIED )
   {
      ch->stop_fighting( false );
      stop_hating( ch );
      stop_fearing( ch );
      stop_hunting( ch );
   }
}

CMDF( do_mp_log )
{
   if( !can_use_mprog( ch ) )
      return;

   if( argument.empty(  ) )
   {
      progbug( "Mp_log:  non-existent entry.", ch );
      return;
   }
   append_to_file( MOBLOG_FILE, "&p{}: &P{}:  &p{}", mini_c_time( current_time, -1 ), ch->short_descr, argument );
}

/*
 * syntax: mprestore (character) (#hps)                Gorog
 */
CMDF( do_mp_restore )
{
   std::string arg1;
   char_data *victim;
   int hp;

   if( !can_use_mprog( ch ) )
      return;

   argument = one_argument( argument, arg1 );

   if( arg1.empty(  ) )
   {
      progbug( "Mprestore: invalid arg1", ch );
      return;
   }

   if( argument.empty(  ) )
   {
      progbug( "Mprestore: invalid argument2", ch );
      return;
   }

   if( !( victim = ch->get_char_room( arg1 ) ) )
   {
      progbugf( ch, "Mprestore: victim {} not in room", arg1 );
      return;
   }

   hp = atoi( argument.c_str(  ) );
   if( hp < 0 || hp > 32000 )
   {
      progbugf( ch, "Mprestore: invalid hp amount {}", hp );
      return;
   }
   hp += victim->hit;
   victim->hit = ( hp > 32000 || hp < 0 || hp > victim->max_hit ) ? victim->max_hit : hp;
}

/*
 * Syntax mpfavor target number
 * Raise a player's favor in progs.
 */
CMDF( do_mpfavor )
{
   std::string arg1;
   char_data *victim;
   int favor;
   bool plus = false, minus = false;

   if( !can_use_mprog( ch ) )
      return;

   argument = one_argument( argument, arg1 );

   if( arg1.empty(  ) )
   {
      progbug( "Mpfavor: invalid argument1", ch );
      return;
   }

   if( argument.empty(  ) )
   {
      progbug( "Mpfavor: invalid argument2", ch );
      return;
   }

   if( !is_number( argument ) )
   {
      progbug( "Mpfavor: invalid argument2", ch );
      return;
   }

   if( !( victim = ch->get_char_room( arg1 ) ) )
   {
      progbugf( ch, "Mpfavor: victim {} not in room", arg1 );
      return;
   }

   if( argument[0] == '+' && argument.length(  ) > 1 )
      plus = true;
   if( argument[0] == '-' && argument.length(  ) > 1 )
      minus = true;

   favor = atoi( argument.c_str(  ) );
   if( plus )
      victim->pcdata->favor = urange( -2500, victim->pcdata->favor + favor, 2500 );
   else if( minus )
      victim->pcdata->favor = urange( -2500, victim->pcdata->favor - favor, 2500 );
   else
      victim->pcdata->favor = urange( -2500, favor, 2500 );
}

/*
 * Syntax mp_open_passage x y z
 *
 * opens a 1-way passage from room x to room y in direction z
 *
 *  won't mess with existing exits
 */
CMDF( do_mp_open_passage )
{
   std::string arg1, arg2;
   room_index *targetRoom, *fromRoom;
   int targetRoomVnum, fromRoomVnum, exit_num = 0;
   exit_data *pexit;

   if( !can_use_mprog( ch ) )
      return;

   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );

   if( arg1.empty(  ) )
   {
      progbug( "MpOpenPassage - Missing arg1.", ch );
      return;
   }

   if( arg2.empty(  ) )
   {
      progbug( "MpOpenPassage - Missing arg2.", ch );
      return;
   }

   if( argument.empty(  ) )
   {
      progbug( "MpOpenPassage - Missing arg3", ch );
      return;
   }

   if( !is_number( arg1 ) )
   {
      progbug( "MpOpenPassage - arg1 isn't a number.", ch );
      return;
   }

   fromRoomVnum = atoi( arg1.c_str(  ) );
   if( !( fromRoom = get_room_index( fromRoomVnum ) ) )
   {
      progbug( "MpOpenPassage - arg1 isn't an existing room.", ch );
      return;
   }

   if( !is_number( arg2 ) )
   {
      progbug( "MpOpenPassage - arg2 isn't a number.", ch );
      return;
   }

   targetRoomVnum = atoi( arg2.c_str(  ) );
   if( !( targetRoom = get_room_index( targetRoomVnum ) ) )
   {
      progbug( "MpOpenPassage - arg2 isn't an existing room.", ch );
      return;
   }

   if( !is_number( argument ) )
   {
      if( ( exit_num = get_dirnum( argument ) ) < 0 )
      {
         progbug( "MpOpenPassage - argument isn't a valid direction name.", ch );
         return;
      }
   }
   else if( is_number( argument ) )
      exit_num = atoi( argument.c_str(  ) );

   if( ( exit_num < 0 ) || ( exit_num > MAX_DIR ) )
   {
      progbugf( ch, "MpOpenPassage - argument isn't a valid direction use a number from 0 - {}.", MAX_DIR );
      return;
   }

   if( ( pexit = fromRoom->get_exit( exit_num ) ) != nullptr )
   {
      if( !IS_EXIT_FLAG( pexit, EX_PASSAGE ) )
         return;
      progbugf( ch, "MpOpenPassage - Exit {} already exists.", exit_num );
      return;
   }

   pexit = fromRoom->make_exit( targetRoom, exit_num );
   pexit->key = -1;
   pexit->flags.reset(  );
   SET_EXIT_FLAG( pexit, EX_PASSAGE );
}

/*
 * Syntax mp_fillin x
 * Simply closes the door
 */
CMDF( do_mp_fill_in )
{
   exit_data *pexit;

   if( !can_use_mprog( ch ) )
      return;

   if( !( pexit = find_door( ch, argument, true ) ) )
   {
      progbugf( ch, "MpFillIn - Exit {} does not exist", argument );
      return;
   }
   SET_EXIT_FLAG( pexit, EX_CLOSED );
}

/*
 * Syntax mp_close_passage x y 
 *
 * closes a passage in room x leading in direction y
 *
 * the exit must have EX_PASSAGE set
 */
CMDF( do_mp_close_passage )
{
   std::string arg1;
   room_index *fromRoom;
   int fromRoomVnum, exit_num = 0;
   exit_data *pexit;

   if( !can_use_mprog( ch ) )
      return;

   argument = one_argument( argument, arg1 );

   if( arg1.empty(  ) )
   {
      progbug( "MpClosePassage - Missing arg1.", ch );
      return;
   }

   if( argument.empty(  ) )
   {
      progbug( "MpClosePassage - Missing arg2.", ch );
      return;
   }

   if( !is_number( arg1 ) )
   {
      progbug( "MpClosePassage - arg1 isn't a number.", ch );
      return;
   }

   fromRoomVnum = atoi( arg1.c_str(  ) );
   if( !( fromRoom = get_room_index( fromRoomVnum ) ) )
   {
      progbug( "MpClosePassage - arg1 isn't an existing room.", ch );
      return;
   }

   if( !is_number( argument ) )
   {
      if( ( exit_num = get_dirnum( argument ) ) < 0 )
      {
         progbug( "MpOpenPassage - argument isn't a valid direction name.", ch );
         return;
      }
   }
   else if( is_number( argument ) )
      exit_num = atoi( argument.c_str(  ) );

   if( ( exit_num < 0 ) || ( exit_num > MAX_DIR ) )
   {
      progbugf( ch, "MpClosePassage - argument isn't a valid direction use a number from 0 - {}.", MAX_DIR );
      return;
   }

   if( !( pexit = fromRoom->get_exit( exit_num ) ) )
      return;  /* already closed, ignore...  so rand_progs close without spam */

   if( !IS_EXIT_FLAG( pexit, EX_PASSAGE ) )
   {
      progbug( "MpClosePassage - Exit not a passage.", ch );
      return;
   }
   fromRoom->extract_exit( pexit );
}

/*
 * Does nothing.  Used for scripts.
 */
/* don't get any funny ideas about making this take an arg and pause to count it off! */
CMDF( do_mpnothing )
{
   return;
}

/*
 * Sends a message to sleeping character. Should be fun with room sleep_progs
 */
CMDF( do_mpdream )
{
   std::string arg1;
   char_data *victim;

   if( !can_use_mprog( ch ) )
      return;

   argument = one_argument( argument, arg1 );

   if( !( victim = ch->get_char_world( arg1 ) ) )
   {
      progbugf( ch, "Mpdream: No such character {}", arg1 );
      return;
   }

   if( victim->position <= POS_SLEEPING )
      victim->printf( "%s\r\n", argument.c_str(  ) );
}

CMDF( do_mpdelay )
{
   std::string arg;
   char_data *victim;
   int delay;

   if( !can_use_mprog( ch ) )
      return;

   argument = one_argument( argument, arg );

   if( arg.empty(  ) )
   {
      progbug( "Mpdelay: no duration specified", ch );
      return;
   }

   if( !( victim = ch->get_char_room( arg ) ) )
   {
      progbugf( ch, "Mpdelay: target {} not in room", arg );
      return;
   }

   if( victim->is_immortal(  ) )
   {
      progbugf( ch, "Mpdelay: target {} is immortal", victim->name );
      return;
   }

   if( !is_number( argument ) )
   {
      progbug( "Mpdelay: invalid (nonexistant?) argument", ch );
      return;
   }

   delay = atoi( argument.c_str(  ) );
   if( delay < 1 || delay > 30 )
   {
      progbugf( ch, "Mpdelay:  argument {} out of range (1 to 30)", delay );
      return;
   }
   victim->WAIT_STATE( delay * sysdata->pulseviolence );
}

CMDF( do_mppeace )
{
   if( !can_use_mprog( ch ) )
      return;

   if( argument.empty(  ) || !str_cmp( argument, "all" ) )
   {
      for( auto* rch : ch->in_room->people )
      {
         if( rch->fighting )
         {
            rch->stop_fighting( true );
            interpret( rch, "sit" );
         }
         stop_hating( rch );
         stop_hunting( rch );
         stop_fearing( rch );
      }
      return;
   }

   char_data *victim;
   if( !( victim = ch->get_char_room( argument ) ) )
   {
      progbugf( ch, "Mppeace: target {} not in room", argument );
      return;
   }

   if( victim->fighting )
      victim->stop_fighting( true );
   stop_hating( ch );
   stop_hunting( ch );
   stop_fearing( ch );
   stop_hating( victim );
   stop_hunting( victim );
   stop_fearing( victim );
}

CMDF( do_mpsindhae )
{
   std::string arg;
   obj_index *pObjIndex, *prizeindex = nullptr;
   char_data *victim;

   if( !can_use_mprog( ch ) )
      return;

   argument = one_argument( argument, arg );

   if( !( victim = ch->get_char_room( arg ) ) )
   {
      progbug( "mpsindhae: No target for redemption!", ch );
      return;
   }

   if( !victim->in_room )
   {
      progbugf( ch, "mpsindhae: Redemption target {} in nullptr room! Transplanting to Limbo.", victim->name );
      if( !victim->to_room( get_room_index( ROOM_VNUM_LIMBO ) ) )
         log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __func__, __LINE__ );
      return;
   }

   if( victim->isnpc(  ) )
   {
      progbugf( ch, "mpsindhae: NPC {} triggered the program, bouncing his butt to Bywater!", victim->short_descr );
      victim->from_room(  );
      if( !victim->to_room( get_room_index( ROOM_VNUM_TEMPLE ) ) )
         log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __func__, __LINE__ );
      return;
   }

   int tokencount = 0, tokenstart = 5;

   if( !str_cmp( argument, "bronze" ) )
      tokenstart = 5;

   if( !str_cmp( argument, "silver" ) )
      tokenstart = 14;

   if( !str_cmp( argument, "gold" ) )
      tokenstart = 23;

   if( !str_cmp( argument, "platinum" ) )
      tokenstart = 32;

   if( victim->pcdata->qbits.find( tokenstart ) != victim->pcdata->qbits.end(  ) )
      ++tokencount;
   if( victim->pcdata->qbits.find( tokenstart + 1 ) != victim->pcdata->qbits.end(  ) )
      ++tokencount;
   if( victim->pcdata->qbits.find( tokenstart + 2 ) != victim->pcdata->qbits.end(  ) )
      ++tokencount;
   if( victim->pcdata->qbits.find( tokenstart + 3 ) != victim->pcdata->qbits.end(  ) )
      ++tokencount;
   if( victim->pcdata->qbits.find( tokenstart + 4 ) != victim->pcdata->qbits.end(  ) )
      ++tokencount;
   if( victim->pcdata->qbits.find( tokenstart + 5 ) != victim->pcdata->qbits.end(  ) )
      ++tokencount;
   if( victim->pcdata->qbits.find( tokenstart + 6 ) != victim->pcdata->qbits.end(  ) )
      ++tokencount;
   if( victim->pcdata->qbits.find( tokenstart + 7 ) != victim->pcdata->qbits.end(  ) )
      ++tokencount;
   if( victim->pcdata->qbits.find( tokenstart + 8 ) != victim->pcdata->qbits.end(  ) )
      ++tokencount;

   if( tokencount < 9 )
   {
      victim->from_room(  );
      if( !victim->to_room( get_room_index( ROOM_VNUM_REDEEM ) ) )
         log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __func__, __LINE__ );
      interpret( victim, "look" );
      victim->print_fmt( "&BYou have not killed all 9 {} creatures yet.\r\n", argument );
      victim->print( "You may not redeem your prize until you do.\r\n" );
      return;
   }

   std::string prizebuf = std::format(  "{}-", argument );
   const char *Class = npc_class[victim->Class];
   prizebuf.append( Class );

   bool found = false;
   std::map<int, obj_index *>::iterator mobj = obj_index_table.begin(  );
   for( mobj = obj_index_table.begin(); mobj != obj_index_table.end(); ++mobj )
   {
      pObjIndex = mobj->second;

      if( hasname( pObjIndex->name, prizebuf ) )
      {
         found = true;
         prizeindex = pObjIndex;
         break;
      }
   }

   if( !found || !prizeindex )
   {
      progbugf( ch, "Unable to resolve prize index for {}", prizebuf );
      victim->from_room(  );
      if( !victim->to_room( get_room_index( ROOM_VNUM_REDEEM ) ) )
         log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __func__, __LINE__ );
      interpret( victim, "look" );
      victim->print( "&RAn internal error occured, please ask an immortal for assistance.\r\n" );
      return;
   }

   for( auto* temp : objlist )
   {
      if( temp->pIndexData->vnum == prizeindex->vnum && !str_cmp( temp->owner, victim->name ) )
      {
         progbugf( ch, "mpsindhae: victim already has {} prize", argument );
         victim->from_room(  );
         if( !victim->to_room( get_room_index( ROOM_VNUM_REDEEM ) ) )
            log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __func__, __LINE__ );
         interpret( victim, "look" );
         victim->print_fmt( "&YYou already have a {} {} prize, you may not collect another yet.\r\n", argument, Class );
         return;
      }
   }

   obj_data *prize;
   if( !( prize = prizeindex->create_object( victim->level ) ) )
   {
      log_printf( "create_object: %s:%s, line %d.", __FILE__, __func__, __LINE__ );
      return;
   }
   prize = prize->to_char( victim );

   for( int x = tokenstart; x < tokenstart + 9; ++x )
      remove_qbit( victim, x );

   log_printf( "%s is beginning redemption for %s %s Sindhae prize.", victim->name.c_str(), argument.c_str(  ), Class );

   victim->print_fmt( "&[magic]{} appears from the mists of the void.\r\r\n\n", prize->short_descr );

   prize->extra_flags.set( ITEM_PERSONAL );
   STRFREE( prize->owner );
   prize->owner = STRALLOC( victim->name.c_str() );

   victim->print( "&GYou will now be asked to name your prize.\r\n" );
   victim->print( "When the command prompt appears, enter the name you want your prize to have.\r\n" );
   victim->print( "This will be the name other players will see when they look at you.\r\n" );
   victim->print( "As always, if you get stuck, type 'help' at the command prompt.\r\n\r\n" );
   victim->print_fmt( "&RYou are editing {}.\r\n", prize->short_descr );
   victim->desc->write_to_buffer( "[SINDHAE] Prizename: " );
   victim->desc->connected = CON_PRIZENAME;
   victim->pcdata->spare_ptr = prize;
}

/* Copy a PC, creating a doppleganger - Samson 10-11-99 */
char_data *make_doppleganger( char_data * ch )
{
   char_data *mob;
   mob_index *pMobIndex;

   if( !( pMobIndex = get_mob_index( MOB_DOPPLEGANGER ) ) )
   {
      bug( "%s: Doppleganger mob %d not found!", __func__, MOB_DOPPLEGANGER );
      return nullptr;
   }

   mob = new char_data;

   mob->pIndexData = pMobIndex;

   mob->name = std::format( "{} doppleganger", ch->name );
   mob->short_descr = ch->name;
   mob->long_descr = std::format( "{}{} is here before you.", ch->name, ch->pcdata->title );

   if( !ch->chardesc.empty() )
      mob->chardesc = ch->chardesc;
   else
      mob->chardesc = "Boring generic something.";
   mob->race = ch->race;
   mob->Class = ch->Class;
   mob->set_specfun(  );

   mob->mpscriptpos = 0;
   mob->level = ch->level;
   mob->set_actflags( pMobIndex->actflags );
   mob->set_actflag( ACT_AGGRESSIVE );
   mob->continent = ch->continent;
   mob->map_x = ch->map_x;
   mob->map_y = ch->map_y;

   if( mob->has_actflag( ACT_MOBINVIS ) )
      mob->mobinvis = mob->level;

   mob->set_aflags( ch->get_aflags(  ) );

   mob->set_aflag( AFF_DETECT_INVIS );
   mob->set_aflag( AFF_DETECT_HIDDEN );
   mob->set_aflag( AFF_TRUESIGHT );
   mob->set_aflag( AFF_INFRARED );

   mob->alignment = ch->alignment;
   mob->sex = ch->sex;
   mob->armor = ch->GET_AC(  );
   mob->max_hit = ch->max_hit;
   mob->hit = mob->max_hit;
   mob->gold = 0;
   mob->exp = 0;
   mob->position = POS_STANDING;
   mob->defposition = POS_STANDING;
   mob->barenumdie = ch->barenumdie;
   mob->baresizedie = ch->baresizedie;
   mob->mobthac0 = ch->mobthac0;
   mob->hitplus = ch->GET_HITROLL(  );
   mob->damplus = ch->GET_DAMROLL(  );
   mob->perm_str = ch->get_curr_str(  );
   mob->perm_wis = ch->get_curr_wis(  );
   mob->perm_int = ch->get_curr_int(  );
   mob->perm_dex = ch->get_curr_dex(  );
   mob->perm_con = ch->get_curr_con(  );
   mob->perm_cha = ch->get_curr_cha(  );
   mob->perm_lck = ch->get_curr_lck(  );
   mob->hitroll = ch->hitroll;
   mob->damroll = ch->damroll;
   mob->saving_poison_death = ch->saving_poison_death;
   mob->saving_wand = ch->saving_wand;
   mob->saving_para_petri = ch->saving_para_petri;
   mob->saving_breath = ch->saving_breath;
   mob->saving_spell_staff = ch->saving_spell_staff;
   mob->height = ch->height;
   mob->weight = ch->weight;
   mob->set_resists( ch->get_resists(  ) );
   mob->set_immunes( ch->get_immunes(  ) );
   mob->set_susceps( ch->get_susceps(  ) );
   mob->set_absorbs( ch->get_absorbs(  ) );
   mob->set_attacks( ch->get_attacks(  ) );
   mob->set_defenses( ch->get_defenses(  ) );
   mob->numattacks = ch->numattacks;
   mob->set_langs( ch->get_langs(  ) );
   mob->speaking = ch->speaking;
   if( ch->has_bparts() )
      mob->set_bparts( ch->get_bparts(  ) );
   else
      mob->set_bparts( race_table[ch->race]->body_parts );
   mob->set_noaflags( ch->get_noaflags(  ) );
   mob->set_noresists( ch->get_noresists(  ) );
   mob->set_noimmunes( ch->get_noimmunes(  ) );
   mob->set_nosusceps( ch->get_nosusceps(  ) );

   /*
    * Insert in list.
    */
   charlist.push_back( mob );
   ++pMobIndex->count;
   ++nummobsloaded;

   return ( mob );
}

/* Equip the doppleganger with everything the PC has - Samson 10-11-99 */
void equip_doppleganger( char_data * ch, char_data * mob )
{
   for( auto* obj : ch->carrying )
   {
      obj_data *newobj;

      if( obj->wear_loc != WEAR_NONE )
      {
         newobj = obj->clone(  );
         newobj->extra_flags.set( ITEM_DEATHROT );
         newobj->to_char( mob );
      }
   }
}

CMDF( do_mpdoppleganger )
{
   char_data *mob, *host;
   room_index *pRoomIndex;

   if( !can_use_mprog( ch ) )
      return;

   if( !( host = ch->get_char_room( argument ) ) )
   {
      progbug( "mpdoppleganger: Attempting to copy nullptr host!", ch );
      return;
   }

   if( host->isnpc(  ) )
   {
      progbugf( ch, "mpdoppleganger: Tried to copy NPC {}!", host->name );
      return;
   }

   if( host->in_room != ch->in_room )
   {
      progbugf( ch, "mpdoppleganger: Cannot copy host PC {}, not in same room!", host->name );
      return;
   }

   pRoomIndex = host->in_room;

   mob = make_doppleganger( host );

   if( !mob )
   {
      progbugf( ch, "mpdoppleganger: Failure to create doppleganer out of {}!", host->name );
      return;
   }
   if( !mob->to_room( pRoomIndex ) )
      log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __func__, __LINE__ );

   equip_doppleganger( host, mob );
}

/*
 * "Roll" players stats based on the character name		-Thoric
 */
/* Rewritten by Whir. Thanks to Vor/Casteele for help 2-1-98 */
/* Racial bonus calculations moved to this function and removed from comm.c - Samson 2-2-98 */
/* Updated to AD&D standards by Samson 9-5-98 */
/* Changed to use internal random number generator instead of OS dependent random() function - Samson 9-5-98 */
void name_stamp_stats( char_data * ch )
{
   ch->perm_str = 6 + dice( 2, 6 );
   ch->perm_dex = 6 + dice( 2, 6 );
   ch->perm_wis = 6 + dice( 2, 6 );
   ch->perm_int = 6 + dice( 2, 6 );
   ch->perm_con = 6 + dice( 2, 6 );
   ch->perm_cha = 6 + dice( 2, 6 );
   ch->perm_lck = 6 + dice( 2, 6 );

   ch->perm_str += race_table[ch->race]->str_plus;
   ch->perm_int += race_table[ch->race]->int_plus;
   ch->perm_wis += race_table[ch->race]->wis_plus;
   ch->perm_dex += race_table[ch->race]->dex_plus;
   ch->perm_con += race_table[ch->race]->con_plus;
   ch->perm_cha += race_table[ch->race]->cha_plus;
   ch->perm_lck += race_table[ch->race]->lck_plus;
}

/* Sets up newbie default values for new creation system - Samson 1-2-99 */
void setup_newbie( char_data * ch, bool NEWLOGIN )
{
   obj_data *obj;
   obj_index *objcheck;
   race_type *race;

   int iLang;

   ch->Class = CLASS_WARRIOR; /* Default for new PC - Samson 8-4-98 */
   ch->race = RACE_HUMAN;  /* Default for new PC - Samson 8-4-98 */
   ch->pcdata->clan = nullptr;

   race = race_table[ch->race];

   /*
    * Set to zero as default values - Samson 9-5-98
    */
   ch->set_aflags( race->affected );
   ch->set_attacks( race->attacks );
   ch->set_defenses( race->defenses );
   ch->saving_poison_death = 0;
   ch->saving_wand = 0;
   ch->saving_para_petri = 0;
   ch->saving_breath = 0;
   ch->saving_spell_staff = 0;

   ch->alignment = 0;   /* Oops, forgot to set this. Causes trouble for restarts :) */

   ch->height = ch->calculate_race_height(  );
   ch->weight = ch->calculate_race_weight(  );

   if( ( iLang = skill_lookup( "common" ) ) < 0 )
      bug( "%s: cannot find common language.", __func__ );
   else
      ch->pcdata->learned[iLang] = 100;

   name_stamp_stats( ch ); /* Initialize first stat roll for new PC - Samson */

   ch->level = 1;
   ch->exp = 0;

   /*
    * Set player birthday to current mud day, -17 years - Samson 10-25-99
    */
   ch->pcdata->day = time_info.day;
   ch->pcdata->month = time_info.month;
   ch->pcdata->year = time_info.year - 17;
   ch->pcdata->age = 17;
   ch->pcdata->age_bonus = 0;

   /*
    * Set recall point - Samson 5-13-01
    */
   ch->pcdata->one = ROOM_VNUM_TEMPLE;

   ch->max_hit = 10;
   ch->hit = ch->max_hit;

   ch->max_mana = 100;
   ch->mana = ch->max_mana;

   ch->max_move = 150;
   ch->move = ch->max_move;

   ch->set_title( "the Newbie" );

   /*
    * Added by Narn. Start new characters with autoexit and autgold already turned on. Very few people don't use those.
    */
   ch->set_pcflag( PCFLAG_AUTOGOLD );
   ch->set_pcflag( PCFLAG_AUTOEXIT );

   /*
    * Added by Brittany, Nov 24/96.  The object is the adventurer's guide to Alsherok, part of newbie.are.
    * Modified by Samson to use variable so object can be moved to a new zone if needed - 9-5-98
    */
   objcheck = get_obj_index( OBJ_VNUM_NEWBIE_GUIDE );
   if( objcheck != nullptr )
   {
      if( !( obj = get_obj_index( OBJ_VNUM_NEWBIE_GUIDE )->create_object( 1 ) ) )
         log_printf( "create_object: %s:%s, line %d.", __FILE__, __func__, __LINE__ );
      else
         obj->to_char( ch );
   }
   else
      bug( "%s: Newbie Guide object %d not found.", __func__, OBJ_VNUM_NEWBIE_GUIDE );

   objcheck = get_obj_index( OBJ_VNUM_SCHOOL_BANNER );
   if( objcheck != nullptr )
   {
      if( !( obj = get_obj_index( OBJ_VNUM_SCHOOL_BANNER )->create_object( 1 ) ) )
         log_printf( "create_object: %s:%s, line %d.", __FILE__, __func__, __LINE__ );
      else
      {
         obj->to_char( ch );
         ch->equip( obj, WEAR_LIGHT );
      }
   }
   else
      bug( "%s: Newbie light object %d not found.", __func__, OBJ_VNUM_SCHOOL_BANNER );

   if( !NEWLOGIN )
      return;

   if( !sysdata->WAIT_FOR_AUTH ) /* Altered by Samson 4-12-98 */
   {
      if( !ch->to_room( get_room_index( ROOM_NOAUTH_START ) ) )
         log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __func__, __LINE__ );
   }
   else
   {
      if( !ch->to_room( get_room_index( ROOM_AUTH_START ) ) )
         log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __func__, __LINE__ );
      ch->set_pcflag( PCFLAG_UNAUTHED );
      add_to_auth( ch );   /* new auth */
   }
   ch->music( "creation.mp3", 100, false );
   addname( ch->pcdata->chan_listen, "chat" );

   ch->set_color( AT_PLAIN );
}

/* Alignment setting for Class during creation - Samson 4-17-98 */
void class_create_check( char_data * ch )
{
   switch ( ch->Class )
   {
      default:   /* Any other Class not listed below */
         ch->alignment = 0;
         break;

      case CLASS_ANTIPALADIN:   /* Antipaladin */
         ch->alignment = -1000;
         break;

      case CLASS_PALADIN: /* Paladin */
         ch->alignment = 1000;
         break;

      case CLASS_RANGER:
         ch->alignment = 350;
         break;
   }
}

/* Strip PC & unlearn racial language when asking to restart creation - Samson 10-12-98 */
CMDF( do_mpredo )
{
   char_data *victim;

   if( !can_use_mprog( ch ) )
      return;

   if( !( victim = ch->get_char_world( argument ) ) )
   {
      progbug( "Mpredo - No such person", ch );
      return;
   }

   if( !victim->desc )
   {
      progbugf( ch, "Mpredo - Victim {} has no descriptor!", victim->name );
      return;
   }

   if( !victim->in_room )
   {
      progbugf( ch, "Mpredo - Victim {} in Limbo", victim->name );
      return;
   }

   if( victim->isnpc(  ) )
   {
      progbugf( ch, "Mpredo - Victim {} is an NPC", victim->short_descr );
      return;
   }

   log_printf( "%s is restarting creation from end room.\r\n", victim->name.c_str() );

   for( auto it = victim->carrying.begin(); it != victim->carrying.end(); )
   {
      obj_data *obj = *it;
      ++it;

      obj->extract(  );
   }

   for( int sn = 0; sn < num_skills; ++sn )
      victim->pcdata->learned[sn] = 0;

   setup_newbie( victim, false );
}

/* Final pre-entry setup for new characters - Samson 8-6-98 */
CMDF( do_mpraceset )
{
   char_data *victim;
   std::string arg1;
   race_type *race;
   class_type *Class;
   char *classname;
   obj_data *obj;
   int iLang;

   if( !can_use_mprog( ch ) )
      return;

   argument = one_argument( argument, arg1 );

   if( arg1.empty(  ) )
   {
      progbug( "Mpraceset - Bad syntax. No argument!", ch );
      return;
   }

   if( !( victim = ch->get_char_world( arg1 ) ) )
   {
      progbugf( ch, "Mpraceset - No such person {}", arg1 );
      return;
   }

   if( !victim->desc )
   {
      progbugf( ch, "Mpraceset - Victim {} has no descriptor!", victim->name );
      return;
   }

   if( !victim->in_room )
   {
      progbugf( ch, "Mpraceset - Victim {} in Limbo", victim->name );
      return;
   }

   if( victim->isnpc(  ) )
   {
      progbugf( ch, "Mpraceset - Victim {} is an NPC", victim->name );
      return;
   }

   race = race_table[victim->race];
   Class = class_table[victim->Class];
   classname = Class->who_name;

   victim->set_aflags( race->affected );
   victim->armor += race->ac_plus;
   victim->set_attacks( race->attacks );
   victim->set_defenses( race->defenses );
   victim->saving_poison_death = race->saving_poison_death;
   victim->saving_wand = race->saving_wand;
   victim->saving_para_petri = race->saving_para_petri;
   victim->saving_breath = race->saving_breath;
   victim->saving_spell_staff = race->saving_spell_staff;

   victim->height = victim->calculate_race_height(  );
   victim->weight = victim->calculate_race_weight(  );

   for( iLang = 0; iLang < LANG_UNKNOWN; ++iLang )
      if( race->language.test( iLang ) )
         break;

   if( iLang == LANG_UNKNOWN )
      progbug( "Mpraceset: invalid racial language.", ch );
   else
   {
      victim->set_lang( iLang );
      if( ( iLang = skill_lookup( lang_names[iLang] ) ) < 0 )
         progbug( "Mpraceset: cannot find racial language.", ch );
      else
         victim->pcdata->learned[iLang] = 100;
   }

   class_create_check( victim ); /* Checks Class for proper alignment on creation - Samson 4-17-98 */

   victim->max_hit = 15 + con_app[victim->get_curr_con(  )].hitp + number_range( Class->hp_min, Class->hp_max ) + race->hit;
   victim->hit = victim->max_hit;

   victim->max_mana = 100 + race->mana;
   victim->mana = victim->max_mana;

   std::string buf = std::format( "the {}", title_table[victim->Class][victim->level][victim->sex] );
   victim->set_title( buf );

   if( Class->weapon != -1 )
   {
      if( !( obj = get_obj_index( Class->weapon )->create_object( 1 ) ) )
      {
         log_printf( "create_object: %s:%s, line %d.", __FILE__, __func__, __LINE__ );
         bug( "Class weapon %d not found for Class %s.", Class->weapon, classname );
      }
      else
      {
         obj->to_char( victim );
         victim->equip( obj, WEAR_WIELD );
      }
   }

   if( Class->armor != -1 )
   {
      if( !( obj = get_obj_index( Class->armor )->create_object( 1 ) ) )
      {
         log_printf( "create_object: %s:%s, line %d.", __FILE__, __func__, __LINE__ );
         bug( "Class armor %d not found for Class %s.", Class->armor, classname );
      }
      else
      {
         obj->to_char( victim );
         victim->equip( obj, WEAR_BODY );
      }
   }

   if( Class->legwear != -1 )
   {
      if( !( obj = get_obj_index( Class->legwear )->create_object( 1 ) ) )
      {
         log_printf( "create_object: %s:%s, line %d.", __FILE__, __func__, __LINE__ );
         bug( "Class legwear %d not found for Class %s.", Class->legwear, classname );
      }
      else
      {
         obj->to_char( victim );
         victim->equip( obj, WEAR_LEGS );
      }
   }

   if( Class->headwear != -1 )
   {
      if( !( obj = get_obj_index( Class->headwear )->create_object( 1 ) ) )
      {
         log_printf( "create_object: %s:%s, line %d.", __FILE__, __func__, __LINE__ );
         bug( "Class headwear %d not found for Class %s.", Class->headwear, classname );
      }
      else
      {
         obj->to_char( victim );
         victim->equip( obj, WEAR_HEAD );
      }
   }

   if( Class->armwear != -1 )
   {
      if( !( obj = get_obj_index( Class->armwear )->create_object( 1 ) ) )
      {
         log_printf( "create_object: %s:%s, line %d.", __FILE__, __func__, __LINE__ );
         bug( "Class armwear %d not found for Class %s.", Class->armwear, classname );
      }
      else
      {
         obj->to_char( victim );
         victim->equip( obj, WEAR_ARMS );
      }
   }

   if( Class->footwear != -1 )
   {
      if( !( obj = get_obj_index( Class->footwear )->create_object( 1 ) ) )
      {
         log_printf( "create_object: %s:%s, line %d.", __FILE__, __func__, __LINE__ );
         bug( "Class footwear %d not found for Class %s.", Class->footwear, classname );
      }
      else
      {
         obj->to_char( victim );
         victim->equip( obj, WEAR_FEET );
      }
   }

   if( Class->shield != -1 )
   {
      if( !( obj = get_obj_index( Class->shield )->create_object( 1 ) ) )
      {
         log_printf( "create_object: %s:%s, line %d.", __FILE__, __func__, __LINE__ );
         bug( "Class shield %d not found for Class %s.", Class->shield, classname );
      }
      else
      {
         obj->to_char( victim );
         victim->equip( obj, WEAR_SHIELD );
      }
   }

   if( Class->held != -1 )
   {
      if( !( obj = get_obj_index( Class->held )->create_object( 1 ) ) )
      {
         log_printf( "create_object: %s:%s, line %d.", __FILE__, __func__, __LINE__ );
         bug( "Class held %d not found for Class %s.", Class->held, classname );
      }
      else
      {
         obj->to_char( victim );
         victim->equip( obj, WEAR_HOLD );
      }
   }
}

/* Stat rerolling function for new PC creation system - Samson 8-4-98 */
CMDF( do_mpstatreroll )
{
   char_data *victim;

   if( !can_use_mprog( ch ) )
      return;

   if( argument.empty(  ) )
   {
      progbug( "Mpstatreroll - Bad syntax. No argument!", ch );
      return;
   }

   if( !( victim = ch->get_char_world( argument ) ) )
   {
      progbugf( ch, "Mpstatreroll - No such person {}", argument );
      return;
   }

   if( !victim->desc )
   {
      progbugf( ch, "Mpstatreroll - Victim {} has no descriptor!", victim->name );
      return;
   }

   if( !victim->in_room )
   {
      progbugf( ch, "Mpstatreroll - Victim {} in Limbo", victim->name );
      return;
   }

   victim->desc->write_to_buffer( "You may roll as often as you like.\r\n" );

   name_stamp_stats( victim );

   victim->desc->buffer_printf( "\r\nStr: {}\r\n", attribtext( victim->perm_str ) );
   victim->desc->buffer_printf( "Int: {}\r\n", attribtext( victim->perm_int ) );
   victim->desc->buffer_printf( "Wis: {}\r\n", attribtext( victim->perm_wis ) );
   victim->desc->buffer_printf( "Dex: {}\r\n", attribtext( victim->perm_dex ) );
   victim->desc->buffer_printf( "Con: {}\r\n", attribtext( victim->perm_con ) );
   victim->desc->buffer_printf( "Cha: {}\r\n", attribtext( victim->perm_cha ) );
   victim->desc->buffer_printf( "Lck: {}\r\n", attribtext( victim->perm_lck ) );
   victim->desc->write_to_buffer( "\r\nKeep these stats? (Y/N)" );
   victim->desc->connected = CON_ROLL_STATS;
}

/* Copy of mptransfer with a do_look attached - Samson 4-14-98 */
CMDF( do_mptrlook )
{
   std::string arg1, arg2;
   room_index *location;
   char_data *victim;

   if( !can_use_mprog( ch ) )
      return;

   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );

   if( arg1.empty(  ) )
   {
      progbug( "Mptrlook - Bad syntax", ch );
      return;
   }

   /*
    * Put in the variable nextinroom to make this work right. -Narn 
    */
   if( !str_cmp( arg1, "all" ) )
   {
      std::list<char_data *>::iterator ich;

      for( ich = ch->in_room->people.begin(  ); ich != ch->in_room->people.end(  ); )
      {
         victim = *ich;
         ++ich;

         if( victim != ch && victim->level > 1 && ch->can_see( victim, true ) )
            funcf( ch, do_mptrlook, "{} {}", victim->name, arg2 );
      }
      return;
   }

   /*
    * Thanks to Grodyn for the optional location parameter.
    */
   if( arg2.empty(  ) )
      location = ch->in_room;
   else
   {
      if( !( location = ch->find_location( arg2 ) ) )
      {
         progbugf( ch, "Mptrlook - No such location: {}", arg2 );
         return;
      }

      if( location->is_private(  ) )
      {
         progbugf( ch, "Mptrlook - Private room: {}", location->vnum );
         return;
      }
   }

   if( !( victim = ch->get_char_world( arg1 ) ) )
   {
      progbugf( ch, "Mptrlook - No such person: {}", arg1 );
      return;
   }

   if( !victim->in_room )
   {
      progbugf( ch, "Mptrlook - Victim {} in Limbo", victim->name );
      return;
   }

   if( !victim->isnpc(  ) && victim->pcdata->release_date != std::chrono::system_clock::time_point{} )
      progbugf( ch, "Mptrlook - helled character ({})", victim->name );

   /*
    * If victim not in area's level range, do not transfer 
    */
   if( !victim->in_hard_range( location->area ) && !location->flags.test( ROOM_PROTOTYPE ) )
      return;

   if( victim->fighting )
      victim->stop_fighting( true );

   leave_map( victim, ch, location );
   act( AT_MAGIC, "A swirling vortex arrives, carrying $n!", victim, nullptr, nullptr, TO_ROOM );
}

/* New mob hate, hunt, and fear code courtesy Rjael of Saltwind MUD Installed by Samson 4-14-98 */
CMDF( do_mphate )
{
   std::string arg1, arg2;
   char_data *victim;
   int vnum = -1;

   if( !can_use_mprog( ch ) )
      return;

   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );

   if( arg1.empty(  ) )
   {
      progbug( "Mphate - Bad syntax, bad victim", ch );
      return;
   }

   if( !( victim = ch->get_char_world( arg1 ) ) )
   {
      progbugf( ch, "Mphate - No such person {}", arg1 );
      return;
   }
   else if( victim->isnpc(  ) )
   {
      char_data *master;

      if( victim->has_aflag( AFF_CHARM ) && ( master = victim->master ) )
      {
         if( !( victim = ch->get_char_world( master->name ) ) )
         {
            progbugf( ch, "Mphate - nullptr NPC Master for {}", victim->name );
            return;
         }
      }
      else
      {
         progbugf( ch, "Mphate - NPC victim: {}", victim->short_descr );
         return;
      }
   }

   if( arg2.empty(  ) )
   {
      progbug( "Mphate - bad syntax, no aggressor", ch );
      return;
   }
   else
   {
      if( is_number( arg2 ) )
      {
         vnum = atoi( arg2.c_str(  ) );
         if( vnum < 1 || vnum > sysdata->maxvnum )
         {
            progbugf( ch, "Mphate -- aggressor vnum {} out of range", vnum );
            return;
         }
      }
      else
      {
         progbug( "Mphate -- aggressor no vnum", ch );
         return;
      }
   }

   for( auto* mob : charlist )
   {
      if( !mob->isnpc(  ) || !mob->in_room || !mob->pIndexData->vnum )
         continue;

      if( vnum == mob->pIndexData->vnum )
         start_hating( mob, victim );
   }
}

CMDF( do_mphunt )
{
   std::string arg1, arg2;
   char_data *victim;
   int vnum = -1;

   if( !can_use_mprog( ch ) )
      return;

   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );

   if( arg1.empty(  ) )
   {
      progbug( "Mphunt - Bad syntax, bad victim", ch );
      return;
   }

   if( !( victim = ch->get_char_world( arg1 ) ) )
   {
      progbugf( ch, "Mphunt - No such person {}", arg1 );
      return;
   }
   else if( victim->isnpc(  ) )
   {
      char_data *master;

      if( victim->has_aflag( AFF_CHARM ) && ( master = victim->master ) )
      {
         if( !( victim = ch->get_char_world( master->name ) ) )
         {
            progbugf( ch, "Mphunt - nullptr NPC Master for {}", victim->name );
            return;
         }
      }
      else
      {
         progbugf( ch, "Mphunt - NPC victim: {}", victim->short_descr );
         return;
      }
   }

   if( arg2.empty(  ) )
   {
      progbug( "Mphunt - bad syntax, no aggressor", ch );
      return;
   }
   else
   {
      if( is_number( arg2 ) )
      {
         vnum = atoi( arg2.c_str(  ) );
         if( vnum < 1 || vnum > sysdata->maxvnum )
         {
            progbugf( ch, "Mphunt -- aggressor vnum {} out of range", vnum );
            return;
         }
      }
      else
      {
         progbug( "Mphunt -- aggressor no vnum", ch );
         return;
      }
   }

   for( auto* mob : charlist )
   {
      if( !mob->isnpc(  ) || !mob->in_room || !mob->pIndexData->vnum )
         continue;

      if( vnum == mob->pIndexData->vnum )
         start_hunting( mob, victim );
   }
}

CMDF( do_mpfear )
{
   std::string arg1, arg2;
   char_data *victim;
   int vnum = -1;

   if( !can_use_mprog( ch ) )
      return;

   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );

   if( arg1.empty(  ) )
   {
      progbug( "Mpfear - Bad syntax, bad victim", ch );
      return;
   }

   if( !( victim = ch->get_char_world( arg1 ) ) )
   {
      progbugf( ch, "Mpfear - No such person {}", arg1 );
      return;
   }
   else if( victim->isnpc(  ) )
   {
      char_data *master;

      if( victim->has_aflag( AFF_CHARM ) && ( master = victim->master ) )
      {
         if( !( victim = ch->get_char_world( master->name ) ) )
         {
            progbugf( ch, "Mpfear - nullptr NPC Master for {}", victim->name );
            return;
         }
      }
      else
      {
         progbugf( ch, "Mpfear - NPC victim: {}", victim->short_descr );
         return;
      }
   }

   if( arg2.empty(  ) )
   {
      progbug( "Mpfear - bad syntax, no aggressor", ch );
      return;
   }
   else
   {
      if( is_number( arg2 ) )
      {
         vnum = atoi( arg2.c_str(  ) );
         if( vnum < 1 || vnum > sysdata->maxvnum )
         {
            progbugf( ch, "Mpfear -- aggressor vnum {} out of range", vnum );
            return;
         }
      }
      else
      {
         progbug( "Mpfear -- aggressor no vnum", ch );
         return;
      }
   }

   for( auto* mob : charlist )
   {
      if( !mob->isnpc(  ) || !mob->in_room || !mob->pIndexData->vnum )
         continue;

      if( vnum == mob->pIndexData->vnum )
         start_fearing( mob, victim );
   }
}
