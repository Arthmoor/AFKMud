/****************************************************************************
 *                   ^     +----- |  / ^     ^ |     | +-\                  *
 *                  / \    |      | /  |\   /| |     | |  \                 *
 *                 /   \   +---   |<   | \ / | |     | |  |                 *
 *                /-----\  |      | \  |  v  | |     | |  /                 *
 *               /       \ |      |  \ |     | +-----+ +-/                  *
 ****************************************************************************
 * AFKMud Copyright 1997-2020 by Roger Libiez (Samson),                     *
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
 *                         Battle & Death module                            *
 ****************************************************************************/

#include "mud.h"
#include "area.h"
#include "clans.h"
#include "deity.h"
#include "descriptor.h"
#include "event.h"
#include "fight.h"
#include "mobindex.h"
#include "mud_prog.h"
#include "objindex.h"
#include "raceclass.h"
#include "roomindex.h"
#include "smaugaffect.h"

extern char lastplayercmd[MIL * 2];
obj_data *used_weapon;  /* Used to figure out which weapon later */
bool dual_flip = false;
bool alreadyUsedSkill = false;

SPELLF( spell_smaug );
SPELLF( spell_energy_drain );
SPELLF( spell_fire_breath );
SPELLF( spell_frost_breath );
SPELLF( spell_acid_breath );
SPELLF( spell_lightning_breath );
SPELLF( spell_gas_breath );
SPELLF( spell_spiral_blast );
SPELLF( spell_dispel_magic );
SPELLF( spell_dispel_evil );
int recall( char_data *, int );
obj_data *create_money( int );
void generate_treasure( char_data *, obj_data * );
void do_unmorph_char( char_data * );
bool check_parry( char_data *, char_data * );
bool check_dodge( char_data *, char_data * );
bool check_tumble( char_data *, char_data * );
void trip( char_data *, char_data * );
room_index *check_room( char_data *, room_index * );
void unbind_follower( char_data *, char_data * );
int IsUndead( char_data * );
int IsDragon( char_data * );
int IsGiantish( char_data * );
void stop_hunting( char_data * );
void start_hunting( char_data *, char_data * );
void start_hating( char_data *, char_data * );
void start_fearing( char_data *, char_data * );

/*
 * Check to see if the player is in an "Arena".
 */
bool in_arena( char_data * ch )
{
   if( !ch )   /* Um..... Could THIS be why ? */
   {
      bug( "%s: nullptr CH!!! Wedgy, you better spill the beans!", __func__ );
      return false;
   }

   if( !ch->in_room )
   {
      bug( "%s: %s in nullptr room. Only The Wedgy knows how though.", __func__, ch->name );
      log_string( "Going to attempt to move them to Limbo to prevent a crash." );
      if( !ch->to_room( get_room_index( ROOM_VNUM_LIMBO ) ) )
         log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __func__, __LINE__ );
      return false;
   }

   if( ch->in_room->flags.test( ROOM_ARENA ) )
      return true;
   if( ch->in_room->area->flags.test( AFLAG_ARENA ) )
      return true;
   if( !str_cmp( ch->in_room->area->filename, "arena.are" ) )
      return true;

   return false;
}

void make_blood( char_data * ch )
{
   obj_data *obj;

   if( !( obj = get_obj_index( OBJ_VNUM_BLOOD )->create_object( 1 ) ) )
   {
      log_printf( "create_object: %s:%s, line %d.", __FILE__, __func__, __LINE__ );
      return;
   }
   obj->timer = number_range( 2, 4 );
   obj->value[1] = number_range( 3, UMIN( 5, ch->level ) );
   obj->to_room( ch->in_room, ch );
}

/*
 * Make a corpse out of a character.
 */
void make_corpse( char_data * ch, char_data * killer )
{
   const char *name;
   obj_data *corpse;

   if( ch->isnpc(  ) )
   {
      name = ch->short_descr;
      if( !( corpse = get_obj_index( OBJ_VNUM_CORPSE_NPC )->create_object( ch->level ) ) )
      {
         log_printf( "create_object: %s:%s, line %d.", __FILE__, __func__, __LINE__ );
         return;
      }

      corpse->timer = 8;
      corpse->value[3] = 100; /* So the slice skill will work */

      /*
       * Just use the 4th value - cost is creating odd errors on compile - Samson 
       * Used by do_slice and spell_animate_dead 
       */
      corpse->value[4] = ch->pIndexData->vnum;

      if( ch->gold > 0 )
      {
         create_money( ch->gold )->to_obj( corpse );
         ch->gold = 0;
      }
      /*
       * Access random treasure generator, only if the killer was a player 
       */
      else if( ch->gold < 0 && !killer->isnpc(  ) )
         generate_treasure( killer, corpse );

      /*
       * Cannot use these! They are used.
       * corpse->value[0] = ch->pIndexData->vnum;
       * corpse->value[1] = ch->max_hit;
       * Using corpse cost to cheat, since corpses not sellable 
       */
      corpse->cost = ( -(ch->pIndexData->vnum) );
      corpse->value[2] = corpse->timer;
   }
   else
   {
      name = ch->name;
      if( !( corpse = get_obj_index( OBJ_VNUM_CORPSE_PC )->create_object( ch->level ) ) )
      {
         log_printf( "create_object: %s:%s, line %d.", __FILE__, __func__, __LINE__ );
         return;
      }

      if( in_arena( ch ) )
         corpse->timer = 0;
      else
         corpse->timer = 45;  /* This will provide a slight window to resurrect at full health - Samson */

      if( ch->gold > 0 )
      {
         create_money( ch->gold )->to_obj( corpse );
         ch->gold = 0;
      }
      corpse->weight = ch->weight;

      /*
       * Removed special handling for PK corpses because it was messing up my evil plot - Samson 
       */
      corpse->value[2] = corpse->timer / 8;
      corpse->value[3] = 5;   /* Decay stage - 5 is best. */
      corpse->value[4] = ch->level;
      corpse->value[5] = 0;   /* Still flesh rotting */
   }

   if( ch->CAN_PKILL(  ) && killer->CAN_PKILL(  ) && ch != killer )
      stralloc_printf( &corpse->action_desc, "%s", killer->name );

   /*
    * Added corpse name - make locate easier, other skills 
    */
   stralloc_printf( &corpse->name, "corpse %s", name );
   stralloc_printf( &corpse->short_descr, corpse->short_descr, name );
   stralloc_printf( &corpse->objdesc, corpse->objdesc, name );

   /*
    * Used in spell_animate_dead to check for dragons, currently. -- Tarl 29 July 2002 
    */
   corpse->value[6] = ch->race;

   obj_data *obj;
   list < obj_data * >::iterator iobj;
   for( iobj = ch->carrying.begin(  ); iobj != ch->carrying.end(  ); )
   {
      obj = ( *iobj );
      ++iobj;

      /*
       * don't put perm player eq in the corpse
       */
      if( !ch->isnpc() && obj->extra_flags.test( ITEM_PERMANENT ) )
         continue;

      obj->from_char(  );
      if( obj->extra_flags.test( ITEM_INVENTORY ) || obj->extra_flags.test( ITEM_DEATHROT ) )
         obj->extract(  );
      else if( obj->extra_flags.test( ITEM_DEATHDROP ) )
      {
         obj->to_room( ch->in_room, ch );
         oprog_drop_trigger( ch, obj ); /* mudprogs */
      }
      else
         obj->to_obj( corpse );
   }
   corpse->to_room( ch->in_room, ch );
}

/* 
 * New alignment shift computation ported from Sillymud code.
 * Samson 3-13-98
 */
int align_compute( char_data * ch, char_data * victim )
{
   int change, align;

   if( ch->isnpc(  ) )
      return ch->alignment;

   align = ch->alignment;

   if( ch->IS_GOOD(  ) && victim->IS_GOOD(  ) )
      change = ( victim->alignment / 200 ) * ( UMAX( 1, ( victim->level - ch->level ) ) );

   else if( ch->IS_EVIL(  ) && victim->IS_GOOD(  ) )
      change = ( victim->alignment / 30 ) * ( UMAX( 1, ( victim->level - ch->level ) ) );

   else if( victim->IS_EVIL(  ) && ch->IS_GOOD(  ) )
      change = ( victim->alignment / 30 ) * ( UMAX( 1, ( victim->level - ch->level ) ) );

   else if( ch->IS_EVIL(  ) && victim->IS_EVIL(  ) )
      change = ( ( victim->alignment / 200 ) + 1 ) * ( UMAX( 1, ( victim->level - ch->level ) ) );

   else
      change = ( victim->alignment / 200 ) * ( UMAX( 1, ( victim->level - ch->level ) ) );

   if( change == 0 )
   {
      if( victim->alignment > 0 )
         change = 1;
      else if( victim->alignment < 0 )
         change = -1;
   }

   align -= change;

   align = UMAX( align, -1000 );
   align = UMIN( align, 1000 );

   return align;
}

/* Alignment monitor for Paladin, Antipaladin and Druid classes - Samson 4-17-98
   Code provided by Sten */
void class_monitor( char_data * ch )
{
   if( ch->Class == CLASS_RANGER && ch->alignment < -350 )
   {
      ch->print( "You are no longer worthy of your powers as a ranger.....\r\n" );
      ch->print( "A strange feeling comes over you as you become a mere warrior!\r\n" );
      ch->Class = CLASS_WARRIOR;
   }

   if( ch->Class == CLASS_ANTIPALADIN && ch->alignment > -350 )
   {
      ch->print( "You are no longer worthy of your powers as an antipaladin.....\r\n" );
      ch->print( "A strange feeling comes over you as you become a mere warrior!\r\n" );
      ch->Class = CLASS_WARRIOR;
   }

   if( ch->Class == CLASS_PALADIN && ch->alignment < 350 )
   {
      ch->print( "You are no longer worthy of your powers as a paladin.....\r\n" );
      ch->print( "A strange feeling comes over you as you become a mere warrior!\r\n" );
      ch->Class = CLASS_WARRIOR;
   }

   if( ch->Class == CLASS_DRUID && ( ch->alignment < -349 || ch->alignment > 349 ) )
   {
      ch->print( "You are no longer worthy of your powers as a druid.....\r\n" );
      ch->print( "A strange feeling comes over you as you become a mere cleric!\r\n" );
      ch->Class = CLASS_CLERIC;
   }

   if( ch->Class == CLASS_RANGER && ( ch->alignment < -249 && ch->alignment >= -350 ) )
      ch->printf( "You are straying from your cause against evil %s!", ch->name );

   if( ch->Class == CLASS_ANTIPALADIN && ( ch->alignment > -449 && ch->alignment <= -351 ) )
      ch->printf( "You are straying from your evil ways %s!", ch->name );

   if( ch->Class == CLASS_PALADIN && ( ch->alignment < 449 && ch->alignment >= 350 ) )
      ch->printf( "You are straying from your rightious ways %s!", ch->name );

   if( ch->Class == CLASS_DRUID && ( ch->alignment < -249 || ch->alignment > 249 ) )
      ch->printf( "You are straying from the balanced path %s!", ch->name );
}

char_data *char_data::who_fighting(  )
{
   if( !fighting )
      return nullptr;
   return fighting->who;
}

/* Aging attack for mobs - Samson 3-28-00 */
CMDF( do_ageattack )
{
   char_data *victim;
   int agechance;

   if( !ch->isnpc(  ) )
   {
      ch->print( "Huh?\r\n" );
      return;
   }

   if( ch->has_aflag( AFF_CHARM ) )
   {
      ch->print( "You can't concentrate enough for that.\r\n" );
      return;
   }

   if( !( victim = ch->who_fighting(  ) ) )
   {
      ch->print( "You aren't fighting anyone.\r\n" );
      return;
   }

   if( victim->isnpc(  ) )
      return;

   agechance = number_range( 1, 100 );

   if( agechance < 92 )
      return;

   victim->printf( "%s touches you, and you feel yourself aging!\r\n", ch->short_descr );
   victim->pcdata->age_bonus += 1;
}

/*
 * Check to see if player's attacks are (still?) suppressed
 *
 */
bool is_attack_supressed( char_data * ch )
{
   timer_data *chtimer;

   if( ch->isnpc(  ) )
      return false;

   if( ch->has_aflag( AFF_GRAPPLE ) )
      return true;

   if( !( chtimer = ch->get_timerptr( TIMER_ASUPRESSED ) ) )
      return false;

   /*
    * perma-supression -- bard? (can be reset at end of fight, or spell, etc) 
    */
   if( chtimer->value == -1 )
      return true;

   /*
    * this is for timed supressions 
    */
   if( chtimer->count >= 1 )
      return true;

   return false;
}

/*
 * Check to see if weapon is poisoned.
 */
bool is_wielding_poisoned( char_data * ch )
{
   obj_data *obj;

   if( !used_weapon )
      return false;

   if( ( obj = ch->get_eq( WEAR_WIELD ) ) != nullptr && used_weapon == obj && obj->extra_flags.test( ITEM_POISONED ) )
      return true;
   if( ( obj = ch->get_eq( WEAR_DUAL_WIELD ) ) != nullptr && used_weapon == obj && obj->extra_flags.test( ITEM_POISONED ) )
      return true;

   return false;
}

CMDF( do_gfighting )
{
   char_data *victim;
   list < char_data * >::iterator ich;
   string arg1, arg2;
   bool pmobs = false, phating = false, phunting = false;
   int low = 1, high = MAX_LEVEL, count = 0;

   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );
   if( !arg1.empty(  ) )
   {
      if( arg2.empty(  ) )
      {
         ch->pager( "\r\n&wSyntax:  gfighting | gfighting <low> <high> | gfighting <low> <high> mobs\r\n" );
         return;
      }
      low = atoi( arg1.c_str(  ) );
      high = atoi( arg2.c_str(  ) );
   }

   if( low < 1 || high < low || low > high || high > MAX_LEVEL )
   {
      ch->pager( "&wInvalid level range.\r\n" );
      return;
   }

   if( !str_cmp( argument, "mobs" ) )
      pmobs = true;
   else if( !str_cmp( argument, "hating" ) )
      phating = true;
   else if( !str_cmp( argument, "hunting" ) )
      phunting = true;

   ch->pagerf( "\r\n&cGlobal %s conflict:\r\n", pmobs ? "mob" : "character" );
   if( !pmobs && !phating && !phunting )
   {
      list < descriptor_data * >::iterator ds;

      for( ds = dlist.begin(  ); ds != dlist.end(  ); ++ds )
      {
         descriptor_data *d = *ds;

         if( ( d->connected == CON_PLAYING || d->connected == CON_EDITING )
             && ( victim = d->character ) != nullptr && !victim->isnpc(  ) && victim->in_room
             && ch->can_see( victim, false ) && victim->fighting && victim->level >= low && victim->level <= high )
         {
            ch->pagerf( "&w%-12.12s &C|%2d &wvs &C%2d| &w%-16.16s [%5d]  &c%-20.20s [%5d]\r\n",
                        victim->name, victim->level, victim->fighting->who->level,
                        victim->fighting->who->isnpc(  )? victim->fighting->who->short_descr : victim->fighting->who->name,
                        victim->fighting->who->isnpc(  )? victim->fighting->who->pIndexData->vnum : 0,
                        victim->in_room->area->name, victim->in_room == nullptr ? 0 : victim->in_room->vnum );
            ++count;
         }
      }
   }
   else if( !phating && !phunting )
   {
      for( ich = charlist.begin(  ); ich != charlist.end(  ); ++ich )
      {
         victim = *ich;

         if( victim->isnpc(  ) && victim->in_room && ch->can_see( victim, false ) && victim->fighting && victim->level >= low && victim->level <= high )
         {
            ch->pagerf( "&w%-12.12s &C|%2d &wvs &C%2d| &w%-16.16s [%5d]  &c%-20.20s [%5d]\r\n",
                        victim->name, victim->level, victim->fighting->who->level,
                        victim->fighting->who->isnpc(  )? victim->fighting->who->short_descr : victim->fighting->who->name,
                        victim->fighting->who->isnpc(  )? victim->fighting->who->pIndexData->vnum : 0,
                        victim->in_room->area->name, victim->in_room == nullptr ? 0 : victim->in_room->vnum );
            ++count;
         }
      }
   }
   else if( !phunting && phating )
   {
      for( ich = charlist.begin(  ); ich != charlist.end(  ); ++ich )
      {
         victim = *ich;

         if( victim->isnpc(  ) && victim->in_room && ch->can_see( victim, false ) && victim->hating && victim->level >= low && victim->level <= high )
         {
            ch->pagerf( "&w%-12.12s &C|%2d &wvs &C%2d| &w%-16.16s [%5d]  &c%-20.20s [%5d]\r\n",
                        victim->name, victim->level, victim->hating->who->level, victim->hating->who->isnpc(  )?
                        victim->hating->who->short_descr : victim->hating->who->name, victim->hating->who->isnpc(  )?
                        victim->hating->who->pIndexData->vnum : 0, victim->in_room->area->name, victim->in_room == nullptr ? 0 : victim->in_room->vnum );
            ++count;
         }
      }
   }
   else if( phunting )
   {
      for( ich = charlist.begin(  ); ich != charlist.end(  ); ++ich )
      {
         victim = *ich;

         if( victim->isnpc(  ) && victim->in_room && ch->can_see( victim, false ) && victim->hunting && victim->level >= low && victim->level <= high )
         {
            ch->pagerf( "&w%-12.12s &C|%2d &wvs &C%2d| &w%-16.16s [%5d]  &c%-20.20s [%5d]\r\n",
                        victim->name, victim->level, victim->hunting->who->level, victim->hunting->who->isnpc(  )?
                        victim->hunting->who->short_descr : victim->hunting->who->name,
                        victim->hunting->who->isnpc(  )? victim->hunting->who->pIndexData->vnum : 0,
                        victim->in_room->area->name, victim->in_room == nullptr ? 0 : victim->in_room->vnum );
            ++count;
         }
      }
   }
   ch->pagerf( "&c%d %s conflicts located.\r\n", count, pmobs ? "mob" : "character" );
}

/*
 * Weapon types, haus
 */
int weapon_prof_bonus_check( char_data * ch, obj_data * wield, int &gsn_ptr )
{
   int bonus = 0;

   gsn_ptr = gsn_pugilism; /* Change back to -1 if this fails horribly */

   if( !ch->isnpc(  ) && wield )
   {
      switch ( wield->value[4] )
      {
            /*
             * Restructured weapon system - Samson 11-20-99 
             */
         default:
            gsn_ptr = -1;
            break;
         case WEP_BAREHAND:
            gsn_ptr = gsn_pugilism;
            break;
         case WEP_SWORD:
            gsn_ptr = gsn_swords;
            break;
         case WEP_DAGGER:
            gsn_ptr = gsn_daggers;
            break;
         case WEP_WHIP:
            gsn_ptr = gsn_whips;
            break;
         case WEP_TALON:
            gsn_ptr = gsn_talonous_arms;
            break;
         case WEP_MACE:
            gsn_ptr = gsn_maces_hammers;
            break;
         case WEP_ARCHERY:
            gsn_ptr = gsn_archery;
            break;
         case WEP_BLOWGUN:
            gsn_ptr = gsn_blowguns;
            break;
         case WEP_SLING:
            gsn_ptr = gsn_slings;
            break;
         case WEP_AXE:
            gsn_ptr = gsn_axes;
            break;
         case WEP_SPEAR:
            gsn_ptr = gsn_spears;
            break;
         case WEP_STAFF:
            gsn_ptr = gsn_staves;
            break;
      }
      if( gsn_ptr != -1 )
         bonus = ( ( ch->LEARNED( gsn_ptr ) - 50 ) / 10 );
   }
   return bonus;
}

/*
 * Calculate to hit armor Class 0 versus armor.
 * Rewritten by Dwip, 5-11-01
 */
int calc_thac0( char_data * ch, char_data * victim, int dist )
{
   int base_thac0, tenacity_adj;
   float thac0, thac0_gain;

   if( ch->isnpc(  ) && ( ch->mobthac0 < 21 || !class_table[ch->Class] ) )
      thac0 = ch->mobthac0;
   else
   {
      base_thac0 = class_table[ch->Class]->base_thac0;   /* This is thac0_00 right now */
      thac0_gain = class_table[ch->Class]->thac0_gain;
      /*
       * thac0_gain is a new thing replacing thac0_32 in the classfiles.
       * *  It needs to be calced and set for all classes, but shouldn't be overly hard. 
       */
      thac0 = ( base_thac0 - ( ch->level * thac0_gain ) ) - ch->GET_HITROLL(  ) + ( dist * 2 );
   }

   /*
    * Tenacity skill affect for Dwarves - Samson 4-24-00 
    */
   tenacity_adj = 0;
   if( dist == 0 && victim )
   {
      if( ch->fighting && ch->fighting->who == victim && ch->has_aflag( AFF_TENACITY ) )
         tenacity_adj -= 4;
   }
   thac0 -= tenacity_adj;

   return ( int )thac0;
}

/*
 * Calculate damage based on resistances, immunities and suceptibilities
 *					-Thoric
 */
double ris_damage( char_data * ch, double dam, int ris )
{
   short modifier;   /* FIND ME */

   modifier = 10;
   if( ch->has_immune( ris ) && !ch->has_noimmune( ris ) )
      modifier -= 10;
   if( ch->has_resist( ris ) && !ch->has_noresist( ris ) )
      modifier -= 2;
   if( ch->has_suscep( ris ) && !ch->has_nosuscep( ris ) )
   {
      if( ch->isnpc(  ) && ch->has_immune( ris ) )
         modifier += 0;
      else
         modifier += 2;
   }
   if( modifier <= 0 )
      return -1;
   if( modifier == 10 )
      return dam;
   return ( dam * modifier ) / 10;
}

bool check_illegal_pk( char_data * ch, char_data * victim )
{
   if( !victim->isnpc(  ) && !ch->isnpc(  ) )
   {
      if( ( !victim->has_pcflag( PCFLAG_DEADLY ) || !ch->has_pcflag( PCFLAG_DEADLY ) )
          && !in_arena( ch ) && ch != victim && !( ch->is_immortal(  ) && victim->is_immortal(  ) ) )
      {
         log_printf( "&p%s on %s%s in &W***&rILLEGAL PKILL&W*** &pattempt at %d",
                     ( lastplayercmd ), ( victim->isnpc(  )? victim->short_descr : victim->name ), ( victim->isnpc(  )? victim->name : "" ), victim->in_room->vnum );
         last_pkroom = victim->in_room->vnum;
         return true;
      }
   }
   return false;
}

bool is_safe( char_data * ch, char_data * victim )
{
   if( victim->char_died(  ) || ch->char_died(  ) )
      return true;

   /*
    * Thx Josh! 
    */
   if( ch->who_fighting(  ) == ch )
      return false;

   if( !victim )  /*Gonna find this is_safe crash bug -Blod */
   {
      bug( "%s: %s opponent does not exist!", __func__, ch->name );
      return true;
   }

   if( !victim->in_room )
   {
      bug( "%s: %s has no physical location!", __func__, victim->name );
      return true;
   }

   if( victim->in_room->flags.test( ROOM_SAFE ) )
   {
      ch->print( "&[magic]A magical force prevents you from attacking.\r\n" );
      return true;
   }

   if( ch->has_actflag( ACT_IMMORTAL ) )
   {
      ch->print( "&[magic]You are protected by the Gods, and therefore cannot engage in combat.\r\n" );
      return true;
   }

   if( victim->has_actflag( ACT_IMMORTAL ) )
   {
      ch->printf( "&[magic]%s is protected by the Gods and cannot be killed!\r\n", capitalize( victim->short_descr ) );
      return true;
   }

   if( ch->has_actflag( ACT_PACIFIST ) )  /* Fireblade */
   {
      ch->print( "&[magic]You are a pacifist and will not fight.\r\n" );
      return true;
   }

   if( victim->has_actflag( ACT_PACIFIST ) ) /* Gorog */
   {
      ch->printf( "&[magic]%s is a pacifist and will not fight.\r\n", capitalize( victim->short_descr ) );
      return true;
   }

   if( !ch->isnpc(  ) && ch->level >= LEVEL_IMMORTAL )
      return false;

   if( !ch->isnpc(  ) && !victim->isnpc(  ) && ch != victim && victim->in_room->area->flags.test( AFLAG_NOPKILL ) )
   {
      ch->print( "&[immortal]The gods have forbidden player killing in this area.\r\n" );
      return true;
   }

   if( victim->is_pet(  ) )
   {
      if( check_illegal_pk( ch, victim->master ) )
      {
         ch->printf( "You may not engage %s's followers in combat.\r\n", victim->master->name );
         return true;
      }

      /*
       * If check added to stop slaughtering pets by accident. (ie: burning hands spell on a pony ;) 
       * Added by Tarl, 18 Mar 02 
       */
      if( !str_cmp( ch->name, victim->master->name ) )
      {
         ch->print( "You may not engage your followers in combat.\r\n" );
         return true;
      }
   }

   // Prevent one person's pets from attacking another person's pets.
   if( ch->is_pet(  ) && victim->is_pet(  ) )
   {
      if( check_illegal_pk( ch->master, victim->master ) )
      {
         ch->master->printf( "&RYou cannot have your followers engage %s's followers in combat.\r\n", victim->master->name );
         return true;
      }
   }

   if( ch->isnpc(  ) || victim->isnpc(  ) )
      return false;

   if( check_illegal_pk( ch, victim ) )
   {
      ch->print( "You cannot attack that person.\r\n" );
      return true;
   }

   if( victim->get_timer( TIMER_PKILLED ) > 0 )
   {
      ch->print( "&GThat character has died within the last 5 minutes.\r\n" );
      return true;
   }

   if( ch->get_timer( TIMER_PKILLED ) > 0 )
   {
      ch->print( "&GYou have been killed within the last 5 minutes.\r\n" );
      return true;
   }
   return false;
}

void align_zap( char_data * ch )
{
   list < obj_data * >::iterator iobj;

   for( iobj = ch->carrying.begin(  ); iobj != ch->carrying.end(  ); )
   {
      obj_data *obj = *iobj;
      ++iobj;

      if( obj->wear_loc == WEAR_NONE )
         continue;

      if( ( obj->extra_flags.test( ITEM_ANTI_EVIL ) && ch->IS_EVIL(  ) )
          || ( obj->extra_flags.test( ITEM_ANTI_GOOD ) && ch->IS_GOOD(  ) ) || ( obj->extra_flags.test( ITEM_ANTI_NEUTRAL ) && ch->IS_NEUTRAL(  ) ) )
      {
         act( AT_MAGIC, "You are zapped by $p.", ch, obj, nullptr, TO_CHAR );
         act( AT_MAGIC, "$n is zapped by $p.", ch, obj, nullptr, TO_ROOM );
         obj->from_char(  );
         if( in_arena( ch ) )
            obj = obj->to_char( ch );
         else
            obj = obj->to_room( ch->in_room, ch );
         oprog_zap_trigger( ch, obj ); /* mudprogs */
         if( ch->char_died(  ) )
            return;
      }
   }
}

/* New (or old depending on point of view :P) exp calculations.
 * Reformulated by Samson on 3-12-98. Lets hope this crap works better
 * than what came stock! Thanks to Sillymud for the ratio formula.
 */
double RatioExp( char_data * gch, char_data * victim, double xp )
{
   double ratio, fexp, chlevel = gch->level, viclevel = victim->level, tempexp = victim->exp;

   ratio = viclevel / chlevel;

   if( ratio < 1.0 )
      fexp = ratio * tempexp;
   else
      fexp = xp;

   return fexp;
}

int xp_compute( char_data * gch, char_data * victim )
{
   int xp, gchlev = gch->level;
//   int xp_ratio;
   xp = victim->exp;

   if( gch->level > 35 )
      xp = ( int )RatioExp( gch, victim, xp );

   /*
    * Supposed to prevent polies from abusing something, not sure what tho. 
    */
   if( gch->isnpc(  ) )
   {
      xp *= 3;
      xp /= 4;
   }

   xp = number_range( ( xp * 3 ) >> 2, ( xp * 5 ) >> 2 );

   /*
    * semi-intelligent experienced player vs. novice player xp gain
    * "bell curve"ish xp mod by Thoric
    * based on time played vs. level
    *
    * We're shutting this off for now to see what affect it has on things - Samson 2-27-06
    if( !gch->isnpc() && gchlev > 5 )
    {
    xp_ratio = ( int )gch->pcdata->played / gchlev;
    if( xp_ratio > 20000 )  // 5/4
    xp = ( xp * 5 ) >> 2;
    else if( xp_ratio > 16000 )   // 3/4
    xp = ( xp * 3 ) >> 2;
    else if( xp_ratio > 10000 )   // 1/2
    xp >>= 1;
    else if( xp_ratio > 5000 ) // 1/4
    xp >>= 2;
    else if( xp_ratio > 3500 ) // 1/8
    xp >>= 3;
    else if( xp_ratio > 2000 ) // 1/16
    xp >>= 4;
    } */

   /*
    * New EXP cap for PK kills 
    */
   if( !victim->isnpc(  ) )
   {
      int diff = victim->level - gch->level;

      if( diff == 0 )
         diff = 1;

      if( diff < 0 )
         diff = 0;

      xp = diff * 2000; /* 2000 exp per level above the killer */
   }

   /*
    * Level based experience gain cap.  Cannot get more experience for
    * a kill than the amount for your current experience level   -Thoric
    */
   return URANGE( 0, xp, exp_level( gchlev ) );
}

/* Figures up a mob's exp value based on some obscure D&D formula.
   Not sure exactly where or how it was derived, but it worked well
   enough for Sillymud, so why not here? :P Samson - 5-15-98 */
/* This has been modified to calculate based on what the mob_xp function tells it to use - Samson 5-18-01 */
int calculate_mob_exp( char_data * mob, int exp_flags )
{
   int base;
   int phit;
   int sab;

   switch ( mob->level )
   {

      case 0:
         base = 10;
         phit = 1;
         sab = 4;
         break;

      case 1:
         base = 20;
         phit = 2;
         sab = 8;
         break;

      case 2:
         base = 35;
         phit = 3;
         sab = 15;
         break;

      case 3:
         base = 60;
         phit = 4;
         sab = 25;
         break;

      case 4:
         base = 90;
         phit = 5;
         sab = 40;
         break;

      case 5:
         base = 150;
         phit = 6;
         sab = 75;
         break;

      case 6:
         base = 225;
         phit = 8;
         sab = 125;
         break;

      case 7:
         base = 375;
         phit = 10;
         sab = 175;
         break;

      case 8:
         base = 600;
         phit = 12;
         sab = 300;
         break;

      case 9:
         base = 900;
         phit = 14;
         sab = 450;
         break;

      case 10:
         base = 1100;
         phit = 15;
         sab = 575;
         break;

      case 11:
         base = 1300;
         phit = 16;
         sab = 700;
         break;

      case 12:
         base = 1550;
         phit = 17;
         sab = 825;
         break;

      case 13:
         base = 1800;
         phit = 18;
         sab = 950;
         break;

      case 14:
         base = 2100;
         phit = 19;
         sab = 1100;
         break;

      case 15:
         base = 2400;
         phit = 20;
         sab = 1250;
         break;

      case 16:
         base = 2700;
         phit = 23;
         sab = 1400;
         break;

      case 17:
         base = 3000;
         phit = 25;
         sab = 1550;
         break;

      case 18:
         base = 4000;
         phit = 28;
         sab = 2000;
         break;

      case 19:
         base = 5000;
         phit = 30;
         sab = 2500;
         break;

      case 20:
         base = 6000;
         phit = 33;
         sab = 3000;
         break;

      case 21:
         base = 7000;
         phit = 35;
         sab = 3500;
         break;

      case 22:
         base = 8000;
         phit = 38;
         sab = 4000;
         break;

      case 23:
         base = 9000;
         phit = 40;
         sab = 4500;
         break;

      case 24:
         base = 11000;
         phit = 45;
         sab = 5500;
         break;

      case 25:
         base = 13000;
         phit = 50;
         sab = 6500;
         break;

      case 26:
         base = 15000;
         phit = 55;
         sab = 7500;
         break;

      case 27:
         base = 17000;
         phit = 60;
         sab = 8500;
         break;

      case 28:
         base = 19000;
         phit = 65;
         sab = 9500;
         break;

      case 29:
         base = 21000;
         phit = 70;
         sab = 10500;
         break;

      case 30:
      case 31:
      case 32:
      case 33:
      case 34:
         base = 24000;
         phit = 80;
         sab = 12000;
         break;


      case 35:
      case 36:
      case 37:
      case 38:
      case 39:
         base = 27000;
         phit = 90;
         sab = 13500;
         break;

      case 40:
      case 41:
      case 42:
      case 43:
      case 44:
         base = 30000;
         phit = 100;
         sab = 15000;
         break;

      case 45:
      case 46:
      case 47:
      case 48:
      case 49:
         base = 33000;
         phit = 110;
         sab = 16500;
         break;

      case 50:
      case 51:
      case 52:
      case 53:
      case 54:
         base = 36000;
         phit = 120;
         sab = 18000;
         break;

      case 55:
      case 56:
      case 57:
      case 58:
      case 59:
         base = 39000;
         phit = 130;
         sab = 19500;
         break;

      case 60:
      case 61:
      case 62:
      case 63:
      case 64:
         base = 43000;
         phit = 150;
         sab = 21500;
         break;

      case 65:
      case 66:
      case 67:
      case 68:
      case 69:
         base = 47000;
         phit = 170;
         sab = 23500;
         break;

      case 70:
      case 71:
      case 72:
      case 73:
      case 74:
         base = 51000;
         phit = 190;
         sab = 25500;
         break;

      case 75:
      case 76:
      case 77:
      case 78:
      case 79:
         base = 55000;
         phit = 210;
         sab = 27500;
         break;

      case 80:
      case 81:
      case 82:
      case 83:
      case 84:
         base = 59000;
         phit = 230;
         sab = 29500;
         break;

      case 85:
      case 86:
      case 87:
      case 88:
      case 89:
         base = 63000;
         phit = 250;
         sab = 31500;
         break;

      case 90:
      case 91:
      case 92:
      case 93:
      case 94:
         base = 68000;
         phit = 280;
         sab = 34000;
         break;

      case 95:
      case 96:
      case 97:
      case 98:
      case 99:
         base = 73000;
         phit = 310;
         sab = 36500;
         break;

      case 100:
         base = 78000;
         phit = 350;
         sab = 39000;
         break;

      default:
         base = 85000;
         phit = 400;
         sab = 42500;
         break;
   }

   return ( base + ( phit * mob->max_hit ) + ( sab * exp_flags ) );
}

/* The new and improved mob xp_flag formula. Data used is according to information from Dwip.
 * Sue him if this isn't working right :)
 * This is gonna be one big, ugly set of ifchecks.
 * Samson 5-18-01
 */
int mob_xp( char_data * mob )
{
   int flags = 0;

   if( mob->armor < 0 )
      ++flags;

   if( mob->numattacks > 4 )
      flags += 2;

   if( mob->has_actflag( ACT_AGGRESSIVE ) || mob->has_actflag( ACT_META_AGGR ) )
      ++flags;

   if( mob->has_actflag( ACT_WIMPY ) )
      flags -= 2;

   if( mob->has_aflag( AFF_DETECT_INVIS ) || mob->has_aflag( AFF_DETECT_HIDDEN ) || mob->has_aflag( AFF_TRUESIGHT ) )
      ++flags;

   if( mob->has_aflag( AFF_SANCTUARY ) )
      ++flags;

   if( mob->has_aflag( AFF_SNEAK ) || mob->has_aflag( AFF_HIDE ) || mob->has_aflag( AFF_INVISIBLE ) )
      ++flags;

   if( mob->has_aflag( AFF_FIRESHIELD ) )
      ++flags;

   if( mob->has_aflag( AFF_SHOCKSHIELD ) )
      ++flags;

   if( mob->has_aflag( AFF_ICESHIELD ) )
      ++flags;

   if( mob->has_aflag( AFF_VENOMSHIELD ) )
      flags += 2;

   if( mob->has_aflag( AFF_ACIDMIST ) )
      flags += 2;

   if( mob->has_aflag( AFF_BLADEBARRIER ) )
      flags += 2;

   if( mob->has_resist( RIS_FIRE ) || mob->has_resist( RIS_COLD ) || mob->has_resist( RIS_ELECTRICITY ) || mob->has_resist( RIS_ENERGY ) || mob->has_resist( RIS_ACID ) )
      ++flags;

   if( mob->has_resist( RIS_BLUNT ) || mob->has_resist( RIS_SLASH ) || mob->has_resist( RIS_PIERCE ) || mob->has_resist( RIS_HACK ) || mob->has_resist( RIS_LASH ) )
      ++flags;

   if( mob->has_resist( RIS_SLEEP ) || mob->has_resist( RIS_CHARM ) || mob->has_resist( RIS_HOLD ) || mob->has_resist( RIS_POISON ) || mob->has_resist( RIS_PARALYSIS ) )
      ++flags;

   if( mob->has_resist( RIS_PLUS1 ) || mob->has_resist( RIS_PLUS2 ) || mob->has_resist( RIS_PLUS3 )
       || mob->has_resist( RIS_PLUS4 ) || mob->has_resist( RIS_PLUS5 ) || mob->has_resist( RIS_PLUS6 ) )
      ++flags;

   if( mob->has_resist( RIS_MAGIC ) )
      flags += 2;

   if( mob->has_resist( RIS_NONMAGIC ) )
      ++flags;

   if( mob->has_immune( RIS_FIRE ) || mob->has_immune( RIS_COLD ) || mob->has_immune( RIS_ELECTRICITY ) || mob->has_immune( RIS_ENERGY ) || mob->has_immune( RIS_ACID ) )
      flags += 2;

   if( mob->has_immune( RIS_BLUNT ) || mob->has_immune( RIS_SLASH ) || mob->has_immune( RIS_PIERCE ) || mob->has_immune( RIS_HACK ) || mob->has_immune( RIS_LASH ) )
      flags += 2;

   if( mob->has_immune( RIS_SLEEP ) || mob->has_immune( RIS_CHARM ) || mob->has_immune( RIS_HOLD ) || mob->has_immune( RIS_POISON ) || mob->has_immune( RIS_PARALYSIS ) )
      flags += 2;

   if( mob->has_immune( RIS_PLUS1 ) || mob->has_immune( RIS_PLUS2 ) || mob->has_immune( RIS_PLUS3 )
       || mob->has_immune( RIS_PLUS4 ) || mob->has_immune( RIS_PLUS5 ) || mob->has_immune( RIS_PLUS6 ) )
      flags += 2;

   if( mob->has_immune( RIS_MAGIC ) )
      flags += 2;

   if( mob->has_immune( RIS_NONMAGIC ) )
      ++flags;

   if( mob->has_suscep( RIS_FIRE ) || mob->has_suscep( RIS_COLD ) || mob->has_suscep( RIS_ELECTRICITY ) || mob->has_suscep( RIS_ENERGY ) || mob->has_suscep( RIS_ACID ) )
      flags -= 2;

   if( mob->has_suscep( RIS_BLUNT ) || mob->has_suscep( RIS_SLASH ) || mob->has_suscep( RIS_PIERCE ) || mob->has_suscep( RIS_HACK ) || mob->has_suscep( RIS_LASH ) )
      flags -= 3;

   if( mob->has_suscep( RIS_SLEEP ) || mob->has_suscep( RIS_CHARM ) || mob->has_suscep( RIS_HOLD ) || mob->has_suscep( RIS_POISON ) || mob->has_suscep( RIS_PARALYSIS ) )
      flags -= 3;

   if( mob->has_suscep( RIS_PLUS1 ) || mob->has_suscep( RIS_PLUS2 ) || mob->has_suscep( RIS_PLUS3 )
       || mob->has_suscep( RIS_PLUS4 ) || mob->has_suscep( RIS_PLUS5 ) || mob->has_suscep( RIS_PLUS6 ) )
      flags -= 2;

   if( mob->has_suscep( RIS_MAGIC ) )
      flags -= 3;

   if( mob->has_suscep( RIS_NONMAGIC ) )
      flags -= 2;

   if( mob->has_absorb( RIS_FIRE ) || mob->has_absorb( RIS_COLD ) || mob->has_absorb( RIS_ELECTRICITY ) || mob->has_absorb( RIS_ENERGY ) || mob->has_absorb( RIS_ACID ) )
      flags += 3;

   if( mob->has_absorb( RIS_BLUNT ) || mob->has_absorb( RIS_SLASH ) || mob->has_absorb( RIS_PIERCE ) || mob->has_absorb( RIS_HACK ) || mob->has_absorb( RIS_LASH ) )
      flags += 3;

/*
   if( mob->has_absorb( RIS_SLEEP ) || mob->has_absorb( RIS_CHARM ) || mob->has_absorb( RIS_HOLD )
    || mob->has_absorb( RIS_POISON ) || mob->has_absorb( RIS_PARALYSIS ) )
	flags += 3;
*/

   if( mob->has_absorb( RIS_PLUS1 ) || mob->has_absorb( RIS_PLUS2 ) || mob->has_absorb( RIS_PLUS3 )
       || mob->has_absorb( RIS_PLUS4 ) || mob->has_absorb( RIS_PLUS5 ) || mob->has_absorb( RIS_PLUS6 ) )
      flags += 3;

   if( mob->has_absorb( RIS_MAGIC ) )
      flags += 3;

   if( mob->has_absorb( RIS_NONMAGIC ) )
      flags += 2;

   if( mob->has_attack( ATCK_BASH ) )
      ++flags;

   if( mob->has_attack( ATCK_STUN ) )
      ++flags;

   if( mob->has_attack( ATCK_BACKSTAB ) )
      flags += 2;

   if( mob->has_attack( ATCK_FIREBREATH ) || mob->has_attack( ATCK_FROSTBREATH )
       || mob->has_attack( ATCK_ACIDBREATH ) || mob->has_attack( ATCK_LIGHTNBREATH ) || mob->has_attack( ATCK_GASBREATH ) )
      flags += 2;

   if( mob->has_attack( ATCK_EARTHQUAKE ) || mob->has_attack( ATCK_FIREBALL ) || mob->has_attack( ATCK_COLORSPRAY ) )
      flags += 2;

   if( mob->has_attack( ATCK_SPIRALBLAST ) )
      flags += 2;

   if( mob->has_attack( ATCK_AGE ) )
      flags += 2;

   if( mob->has_attack( ATCK_BITE ) || mob->has_attack( ATCK_CLAWS ) || mob->has_attack( ATCK_TAIL )
       || mob->has_attack( ATCK_STING ) || mob->has_attack( ATCK_PUNCH ) || mob->has_attack( ATCK_KICK )
       || mob->has_attack( ATCK_TRIP ) || mob->has_attack( ATCK_GOUGE ) || mob->has_attack( ATCK_DRAIN )
       || mob->has_attack( ATCK_POISON ) || mob->has_attack( ATCK_NASTYPOISON ) || mob->has_attack( ATCK_GAZE )
       || mob->has_attack( ATCK_BLINDNESS ) || mob->has_attack( ATCK_CAUSESERIOUS ) || mob->has_attack( ATCK_CAUSECRITICAL )
       || mob->has_attack( ATCK_CURSE ) || mob->has_attack( ATCK_FLAMESTRIKE ) || mob->has_attack( ATCK_HARM ) || mob->has_attack( ATCK_WEAKEN ) )
      ++flags;

   if( mob->has_defense( DFND_PARRY ) )
      ++flags;

   if( mob->has_defense( DFND_DODGE ) )
      ++flags;

   if( mob->has_defense( DFND_DISPELEVIL ) )
      ++flags;

   if( mob->has_defense( DFND_DISPELMAGIC ) )
      ++flags;

   if( mob->has_defense( DFND_DISARM ) )
      ++flags;

   if( mob->has_defense( DFND_SANCTUARY ) )
      ++flags;

   if( mob->has_defense( DFND_HEAL ) || mob->has_defense( DFND_CURELIGHT ) || mob->has_defense( DFND_CURESERIOUS )
       || mob->has_defense( DFND_CURECRITICAL ) || mob->has_defense( DFND_SHIELD ) || mob->has_defense( DFND_BLESS )
       || mob->has_defense( DFND_STONESKIN ) || mob->has_defense( DFND_TELEPORT ) || mob->has_defense( DFND_GRIP ) || mob->has_defense( DFND_TRUESIGHT ) )
      ++flags;

   // Flags: Floor of 0, or it goes negative and the player LOSES xp. Ceiling of 10 or it gets overpowered.
   return calculate_mob_exp( mob, urange( 0, flags, 10 ) );
}

void group_gain( char_data * ch, char_data * victim )
{
   /*
    * Monsters don't get kill xp's or alignment changes ( exception: charmies )
    * Dying of mortal wounds or poison doesn't give xp to anyone!
    */
   if( victim == ch )
      return;

   /*
    * We hope this works of course 
    */
   if( ch->isnpc(  ) )
   {
      if( ch->leader )
      {
         if( !ch->leader->isnpc(  ) && ch->leader == ch->master && ch->has_aflag( AFF_CHARM ) && ch->in_room == ch->leader->in_room )
            ch = ch->master;
      }
   }

   /*
    * See above. If this is STILL an NPC after that, then yes, we need to bail out now. 
    */
   if( ch->isnpc(  ) )
      return;

   int members = 0, max_level = 0;
   list < char_data * >::iterator ich;
   for( ich = ch->in_room->people.begin(  ); ich != ch->in_room->people.end(  ); ++ich )
   {
      char_data *gch = ( *ich );

      if( !is_same_group( gch, ch ) )
         continue;

      if( ch->has_pcflag( PCFLAG_ONMAP ) || ch->has_actflag( ACT_ONMAP ) )
      {
         if( gch->has_pcflag( PCFLAG_ONMAP ) || gch->has_actflag( ACT_ONMAP ) )
         {
            if( !is_same_char_map( ch, gch ) )
               continue;
         }
      }

      /*
       * Count members only if they're PCs so charmies don't dillute the kill 
       */
      if( !gch->isnpc(  ) )
      {
         ++members;
         max_level = UMAX( max_level, gch->level );
      }
   }
   if( members == 0 )
   {
      bug( "%s: members.", __func__ );
      members = 1;
   }

   char_data *lch = ch->leader ? ch->leader : ch;
   for( ich = ch->in_room->people.begin(  ); ich != ch->in_room->people.end(  ); )
   {
      char_data *gch = ( *ich );
      ++ich;
      int xpmod = 1;

      if( !is_same_group( gch, ch ) )
         continue;

      if( ch->has_pcflag( PCFLAG_ONMAP ) || ch->has_actflag( ACT_ONMAP ) )
      {
         if( gch->has_pcflag( PCFLAG_ONMAP ) || gch->has_actflag( ACT_ONMAP ) )
         {
            if( !is_same_char_map( ch, gch ) )
               continue;
         }
      }

      if( gch->level - lch->level > 20 )
         xpmod = 5;

      if( gch->level - lch->level < -20 )
         xpmod = 5;

      int xp = ( xp_compute( gch, victim ) / members ) / xpmod;

      gch->alignment = align_compute( gch, victim );
      class_monitor( gch );   /* Alignment monitoring added - Samson 4-17-98 */
      gch->printf( "%sYou receive %d experience points.\r\n", gch->color_str( AT_PLAIN ), xp );
      gch->gain_exp( xp ); /* group gain */
      align_zap( gch );
   }
}

/*
 * Revamped by Thoric to be more realistic
 * Added code to produce different messages based on weapon type - FB
 * Added better bug message so you can track down the bad dt's -Shaddai
 */
void dam_message( char_data * ch, char_data * victim, double dam, unsigned int dt, obj_data * obj )
{
   char buf1[256], buf2[256], buf3[256];
   const char *vs;
   const char *vp;
   const char *attack;
   char punct;
   double dampc, d_index;
   class skill_type *skill = nullptr;
   bool gcflag = false;
   bool gvflag = false;
   int w_index;
   room_index *was_in_room;

   if( !dam )
      dampc = 0;
   else
      dampc = ( ( dam * 1000 ) / victim->max_hit ) + ( 50 - ( ( victim->hit * 50 ) / victim->max_hit ) );

   if( ch->in_room != victim->in_room )
   {
      was_in_room = ch->in_room;
      ch->from_room(  );
      if( !ch->to_room( victim->in_room ) )
         log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __func__, __LINE__ );
   }
   else
      was_in_room = nullptr;

   /*
    * Get the weapon index 
    */
   if( dt > 0 && dt < ( unsigned int )num_skills )
      w_index = 0;
   else if( dt >= ( unsigned int )TYPE_HIT && dt < ( unsigned int )TYPE_HIT + sizeof( attack_table ) / sizeof( attack_table[0] ) )
      w_index = dt - TYPE_HIT;
   else
   {
      bug( "%s: bad dt %ud from %s in %d.", __func__, dt, ch->name, ch->in_room->vnum );
      dt = TYPE_HIT;
      w_index = 0;
   }

   /*
    * get the damage index 
    */
   if( dam == 0 )
      d_index = 0;
   else if( dampc < 0 )
      d_index = 1;
   else if( dampc <= 100 )
      d_index = 1 + dampc / 10;
   else if( dampc <= 200 )
      d_index = 11 + ( dampc - 100 ) / 20;
   else if( dampc <= 900 )
      d_index = 16 + ( dampc - 200 ) / 100;
   else
      d_index = 23;

   /*
    * Lookup the damage message 
    */
   vs = s_message_table[w_index][( int )d_index];
   vp = p_message_table[w_index][( int )d_index];

   punct = ( dampc <= 30 ) ? '.' : '!';

   if( dam == 0 && ( ch->has_pcflag( PCFLAG_GAG ) ) )
      gcflag = true;

   if( dam == 0 && ( victim->has_pcflag( PCFLAG_GAG ) ) )
      gvflag = true;

   if( dt < ( unsigned int )num_skills )
      skill = skill_table[dt];
   if( dt == ( unsigned int )TYPE_HIT )
   {
      snprintf( buf1, 256, "$n %s $N%c", vp, punct );
      snprintf( buf2, 256, "You %s $N%c", vs, punct );
      snprintf( buf3, 256, "$n %s you%c", vp, punct );
   }
   else if( dt > ( unsigned int )TYPE_HIT && is_wielding_poisoned( ch ) )
   {
      if( dt < ( unsigned int )TYPE_HIT + sizeof( attack_table ) / sizeof( attack_table[0] ) )
         attack = attack_table[dt - TYPE_HIT];
      else
      {
         bug( "%s: bad dt %ud from %s in %d.", __func__, dt, ch->name, ch->in_room->vnum );
         dt = TYPE_HIT;
         attack = attack_table[0];
      }
      snprintf( buf1, 256, "$n's poisoned %s %s $N%c", attack, vp, punct );
      snprintf( buf2, 256, "Your poisoned %s %s $N%c", attack, vp, punct );
      snprintf( buf3, 256, "$n's poisoned %s %s you%c", attack, vp, punct );
   }
   else
   {
      if( skill )
      {
         attack = skill->noun_damage;
         if( dam == 0 )
         {
            bool found = false;

            if( skill->miss_char && skill->miss_char[0] != '\0' )
            {
               act( AT_HIT, skill->miss_char, ch, nullptr, victim, TO_CHAR );
               found = true;
            }
            if( skill->miss_vict && skill->miss_vict[0] != '\0' )
            {
               act( AT_HITME, skill->miss_vict, ch, nullptr, victim, TO_VICT );
               found = true;
            }
            if( skill->miss_room && skill->miss_room[0] != '\0' )
            {
               if( str_cmp( skill->miss_room, "supress" ) )
                  act( AT_ACTION, skill->miss_room, ch, nullptr, victim, TO_NOTVICT );
               found = true;
            }
            if( found ) /* miss message already sent */
            {
               if( was_in_room )
               {
                  ch->from_room(  );
                  if( !ch->to_room( was_in_room ) )
                     log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __func__, __LINE__ );
               }
               return;
            }
         }
         else
         {
            if( skill->hit_char && skill->hit_char[0] != '\0' )
               act( AT_HIT, skill->hit_char, ch, nullptr, victim, TO_CHAR );
            if( skill->hit_vict && skill->hit_vict[0] != '\0' )
               act( AT_HITME, skill->hit_vict, ch, nullptr, victim, TO_VICT );
            if( skill->hit_room && skill->hit_room[0] != '\0' )
               act( AT_ACTION, skill->hit_room, ch, nullptr, victim, TO_NOTVICT );
         }
      }
      else if( dt >= ( unsigned int )TYPE_HIT && dt < ( unsigned int )TYPE_HIT + sizeof( attack_table ) / sizeof( attack_table[0] ) )
      {
         if( obj )
            attack = obj->short_descr;
         else
            attack = attack_table[dt - TYPE_HIT];
      }
      else
      {
         bug( "%s: bad dt %ud from %s in %d.", __func__, dt, ch->name, ch->in_room->vnum );
         attack = attack_table[0];
      }
      snprintf( buf1, 256, "$n's %s %s $N%c", attack, vp, punct );
      snprintf( buf2, 256, "Your %s %s $N%c", attack, vp, punct );
      snprintf( buf3, 256, "$n's %s %s you%c", attack, vp, punct );
   }

   act( AT_ACTION, buf1, ch, nullptr, victim, TO_NOTVICT );
   if( !gcflag )
      act( AT_HIT, buf2, ch, nullptr, victim, TO_CHAR );
   if( !gvflag )
      act( AT_HITME, buf3, ch, nullptr, victim, TO_VICT );

   if( was_in_room )
   {
      ch->from_room(  );
      if( !ch->to_room( was_in_room ) )
         log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __func__, __LINE__ );
   }
}

/*
 * See if an attack justifies a ATTACKER flag.
 */
void check_attacker( char_data * ch, char_data * victim )
{
   /*
    * Made some changes to this function Apr 6/96 to reduce the prolifiration
    * of attacker flags in the realms. -Narn
    */
   /*
    * NPC's are fair game.
    */
   if( victim->isnpc(  ) )
      return;

   /*
    * deadly char check 
    */
   if( !ch->isnpc(  ) && !victim->isnpc(  ) && ch->CAN_PKILL(  ) && victim->CAN_PKILL(  ) )
      return;

   /*
    * Charm-o-rama.
    */
   if( ch->has_aflag( AFF_CHARM ) )
   {
      if( !ch->master )
      {
         bug( "%s: %s bad AFF_CHARM", __func__, ch->isnpc(  )? ch->short_descr : ch->name );
         ch->affect_strip( gsn_charm_person );
         ch->unset_aflag( AFF_CHARM );
         return;
      }
      return;
   }

   /*
    * NPC's are cool of course (as long as not charmed).
    * Hitting yourself is cool too (bleeding).
    * So is being immortal (Alander's idea).
    */
   if( ch->isnpc(  ) || ch == victim || ch->level >= LEVEL_IMMORTAL )
      return;

   return;
}

/*
 * Start fights.
 */
void set_fighting( char_data * ch, char_data * victim )
{
   fighting_data *fight;

   if( ch->fighting )
   {
      bug( "%s: %s -> %s (already fighting %s)", __func__, ch->name, victim->name, ch->fighting->who->name );
      return;
   }

   if( ch->has_aflag( AFF_SLEEP ) )
      ch->affect_strip( gsn_sleep );

   /*
    * Limit attackers -Thoric 
    */
   if( victim->num_fighting > MAX_FIGHT )
   {
      ch->print( "There are too many people fighting for you to join in.\r\n" );
      return;
   }

   fight = new fighting_data;
   fight->who = victim;
   fight->timeskilled = 0;
   ch->num_fighting = 1;
   ch->fighting = fight;

   if( ch->isnpc(  ) )
      ch->position = POS_FIGHTING;
   else
      switch ( ch->style )
      {
         case ( STYLE_EVASIVE ):
            ch->position = POS_EVASIVE;
            break;

         case ( STYLE_DEFENSIVE ):
            ch->position = POS_DEFENSIVE;
            break;

         case ( STYLE_AGGRESSIVE ):
            ch->position = POS_AGGRESSIVE;
            break;

         case ( STYLE_BERSERK ):
            ch->position = POS_BERSERK;
            break;

         default:
            ch->position = POS_FIGHTING;
      }
   ++victim->num_fighting;
   if( victim->switched && victim->switched->has_aflag( AFF_POSSESS ) )
   {
      victim->switched->print( "You are disturbed!\r\n" );
      interpret( victim->switched, "return" );
   }
   add_event( 2, ev_violence, ch );
   return;
}

/*
 * Set position of a victim.
 */
void char_data::update_pos(  )
{
   if( hit > 0 )
   {
      if( position <= POS_STUNNED )
         position = POS_STANDING;
      if( has_aflag( AFF_PARALYSIS ) )
         position = POS_STUNNED;
      return;
   }

   if( isnpc(  ) || hit <= -11 )
   {
      if( mount )
      {
         act( AT_ACTION, "$n falls from $N.", this, nullptr, mount, TO_ROOM );
         mount->unset_actflag( ACT_MOUNTED );
         mount = nullptr;
      }
      position = POS_DEAD;
      return;
   }

   if( hit <= -6 )
      position = POS_MORTAL;
   else if( hit <= -3 )
      position = POS_INCAP;
   else
      position = POS_STUNNED;

   if( position > POS_STUNNED && has_aflag( AFF_PARALYSIS ) )
      position = POS_STUNNED;

   if( mount )
   {
      act( AT_ACTION, "$n falls unconscious from $N.", this, nullptr, mount, TO_ROOM );
      mount->unset_actflag( ACT_MOUNTED );
      mount = nullptr;
   }
   return;
}

/*
 * See if an attack justifies a KILLER flag.
 *
 * Actually since the killer flag no longer exists, this is just tallying kill totals now - Samson
 */
void check_killer( char_data * ch, char_data * victim )
{
   if( victim->isnpc(  ) )
   {
      if( !ch->isnpc(  ) )
      {
         int level_ratio;

         /*
          * Fix for crashes when killing mobs of level 0
          * * by Joe Fabiano -rinthos@yahoo.com
          * * on 03-16-03.
          */
         if( victim->level < 1 )
            level_ratio = URANGE( 1, ch->level, MAX_LEVEL );
         else
            level_ratio = URANGE( 1, ch->level / victim->level, MAX_LEVEL );
         if( ch->pcdata->clan )
            ++ch->pcdata->clan->mkills;
         ++ch->pcdata->mkills;
         if( ch->pcdata->deity )
         {
            if( victim->race == ch->pcdata->deity->npcrace[0] )
               ch->adjust_favor( 3, level_ratio );
            else if( victim->race == ch->pcdata->deity->npcrace[1] )
               ch->adjust_favor( 22, level_ratio );
            else if( victim->race == ch->pcdata->deity->npcrace[2] )
               ch->adjust_favor( 23, level_ratio );
            else if( victim->race == ch->pcdata->deity->npcfoe[0] )
               ch->adjust_favor( 17, level_ratio );
            else if( victim->race == ch->pcdata->deity->npcfoe[1] )
               ch->adjust_favor( 24, level_ratio );
            else if( victim->race == ch->pcdata->deity->npcfoe[2] )
               ch->adjust_favor( 25, level_ratio );
            else
               ch->adjust_favor( 2, level_ratio );
         }
      }
      return;
   }

   if( ch == victim || ch->level >= LEVEL_IMMORTAL )
      return;

   if( in_arena( ch ) )
   {
      if( !ch->isnpc(  ) && !victim->isnpc(  ) )
      {
         ++ch->pcdata->pkills;
         ++victim->pcdata->pdeaths;
      }
      return;
   }

   /*
    * clan checks             -Thoric 
    */
   if( ch->has_pcflag( PCFLAG_DEADLY ) && victim->has_pcflag( PCFLAG_DEADLY ) )
   {
      /*
       * not of same clan? 
       */
      if( !ch->pcdata->clan || !victim->pcdata->clan || ch->pcdata->clan != victim->pcdata->clan )
      {
         if( ch->pcdata->clan )
         {
            if( victim->level < ( LEVEL_AVATAR * 0.1 ) )
               ++ch->pcdata->clan->pkills[0];
            else if( victim->level < ( LEVEL_AVATAR * 0.2 ) )
               ++ch->pcdata->clan->pkills[1];
            else if( victim->level < ( LEVEL_AVATAR * 0.3 ) )
               ++ch->pcdata->clan->pkills[2];
            else if( victim->level < ( LEVEL_AVATAR * 0.4 ) )
               ++ch->pcdata->clan->pkills[3];
            else if( victim->level < ( LEVEL_AVATAR * 0.5 ) )
               ++ch->pcdata->clan->pkills[4];
            else if( victim->level < ( LEVEL_AVATAR * 0.6 ) )
               ++ch->pcdata->clan->pkills[5];
            else if( victim->level < ( LEVEL_AVATAR * 0.7 ) )
               ++ch->pcdata->clan->pkills[6];
            else if( victim->level < ( LEVEL_AVATAR * 0.8 ) )
               ++ch->pcdata->clan->pkills[7];
            else if( victim->level < ( LEVEL_AVATAR * 0.9 ) )
               ++ch->pcdata->clan->pkills[8];
            else
               ++ch->pcdata->clan->pkills[9];
         }
         ++ch->pcdata->pkills;
         ch->hit = ch->max_hit;
         ch->mana = ch->max_mana;
         ch->move = ch->max_move;
         victim->update_pos(  );
         if( victim != ch )
         {
            act( AT_MAGIC, "Bolts of blue energy rise from the corpse, seeping into $n.", ch, victim->name, nullptr, TO_ROOM );
            act( AT_MAGIC, "Bolts of blue energy rise from the corpse, seeping into you.", ch, victim->name, nullptr, TO_CHAR );
         }
         if( victim->pcdata->clan )
         {
            if( ch->level < ( LEVEL_AVATAR * 0.1 ) )
               ++victim->pcdata->clan->pdeaths[0];
            else if( ch->level < ( LEVEL_AVATAR * 0.2 ) )
               ++victim->pcdata->clan->pdeaths[1];
            else if( ch->level < ( LEVEL_AVATAR * 0.3 ) )
               ++victim->pcdata->clan->pdeaths[2];
            else if( ch->level < ( LEVEL_AVATAR * 0.4 ) )
               ++victim->pcdata->clan->pdeaths[3];
            else if( ch->level < ( LEVEL_AVATAR * 0.5 ) )
               ++victim->pcdata->clan->pdeaths[4];
            else if( ch->level < ( LEVEL_AVATAR * 0.6 ) )
               ++victim->pcdata->clan->pdeaths[5];
            else if( ch->level < ( LEVEL_AVATAR * 0.7 ) )
               ++victim->pcdata->clan->pdeaths[6];
            else if( ch->level < ( LEVEL_AVATAR * 0.8 ) )
               ++victim->pcdata->clan->pdeaths[7];
            else if( ch->level < ( LEVEL_AVATAR * 0.9 ) )
               ++victim->pcdata->clan->pdeaths[8];
            else
               ++victim->pcdata->clan->pdeaths[9];
         }
         ++victim->pcdata->pdeaths;
         victim->adjust_favor( 11, 1 );
         ch->adjust_favor( 2, 1 );
         victim->add_timer( TIMER_PKILLED, 115, nullptr, 0 );
         victim->WAIT_STATE( 3 * sysdata->pulseviolence );
         return;
      }
   }

   /*
    * Charm-o-rama.
    */
   if( ch->has_aflag( AFF_CHARM ) )
   {
      if( !ch->master )
      {
         bug( "%s: %s bad AFF_CHARM", __func__, ch->isnpc(  )? ch->short_descr : ch->name );
         ch->affect_strip( gsn_charm_person );
         ch->unset_aflag( AFF_CHARM );
         return;
      }

      /*
       * stop_follower( ch ); 
       */
      if( ch->master )
         check_killer( ch->master, victim );
      return;
   }

   if( ch->isnpc(  ) )
   {
      if( !victim->isnpc(  ) )
      {
         int level_ratio;

         if( victim->pcdata->clan )
            ++victim->pcdata->clan->mdeaths;
         ++victim->pcdata->mdeaths;
         level_ratio = URANGE( 1, ch->level / victim->level, LEVEL_AVATAR );
         if( victim->pcdata->deity )
         {
            if( ch->race == victim->pcdata->deity->npcrace[0] )
               victim->adjust_favor( 12, level_ratio );
            else if( ch->race == victim->pcdata->deity->npcrace[1] )
               victim->adjust_favor( 26, level_ratio );
            else if( ch->race == victim->pcdata->deity->npcrace[2] )
               victim->adjust_favor( 27, level_ratio );
            else if( ch->race == victim->pcdata->deity->npcfoe[0] )
               victim->adjust_favor( 15, level_ratio );
            else if( ch->race == victim->pcdata->deity->npcfoe[1] )
               victim->adjust_favor( 28, level_ratio );
            else if( ch->race == victim->pcdata->deity->npcfoe[2] )
               victim->adjust_favor( 29, level_ratio );
            else
               victim->adjust_favor( 11, level_ratio );
         }
      }
      return;
   }

   if( !ch->isnpc(  ) )
   {
      if( ch->pcdata->clan )
         ++ch->pcdata->clan->illegal_pk;
      ++ch->pcdata->illegal_pk;
   }
   if( !victim->isnpc(  ) )
   {
      if( victim->pcdata->clan )
      {
         if( ch->level < ( LEVEL_AVATAR * 0.1 ) )
            ++victim->pcdata->clan->pdeaths[0];
         else if( ch->level < ( LEVEL_AVATAR * 0.2 ) )
            ++victim->pcdata->clan->pdeaths[1];
         else if( ch->level < ( LEVEL_AVATAR * 0.3 ) )
            ++victim->pcdata->clan->pdeaths[2];
         else if( ch->level < ( LEVEL_AVATAR * 0.4 ) )
            ++victim->pcdata->clan->pdeaths[3];
         else if( ch->level < ( LEVEL_AVATAR * 0.5 ) )
            ++victim->pcdata->clan->pdeaths[4];
         else if( ch->level < ( LEVEL_AVATAR * 0.6 ) )
            ++victim->pcdata->clan->pdeaths[5];
         else if( ch->level < ( LEVEL_AVATAR * 0.7 ) )
            ++victim->pcdata->clan->pdeaths[6];
         else if( ch->level < ( LEVEL_AVATAR * 0.8 ) )
            ++victim->pcdata->clan->pdeaths[7];
         else if( ch->level < ( LEVEL_AVATAR * 0.9 ) )
            ++victim->pcdata->clan->pdeaths[8];
         else
            ++victim->pcdata->clan->pdeaths[9];
      }
      ++victim->pcdata->pdeaths;
   }
   ch->save(  );
   return;
}

/*
 * just verify that a corpse looting is legal
 */
bool legal_loot( char_data * ch, char_data * victim )
{
   /*
    * anyone can loot mobs 
    */
   if( victim->isnpc(  ) )
      return true;

   /*
    * non-charmed mobs can loot anything 
    */
   if( ch->isnpc(  ) && !ch->master )
      return true;

   /*
    * members of different clans can loot too! -Thoric 
    */
   if( ch->has_pcflag( PCFLAG_DEADLY ) && victim->has_pcflag( PCFLAG_DEADLY ) )
      return true;

   return false;
}

void free_fight( char_data * ch )
{
   if( ch->fighting )
   {
      if( !ch->fighting->who->char_died(  ) )
         --ch->fighting->who->num_fighting;
      deleteptr( ch->fighting );
   }
   ch->fighting = nullptr;
   if( ch->mount )
      ch->position = POS_MOUNTED;
   else
      ch->position = POS_STANDING;
   /*
    * Berserk wears off after combat. -- Altrag 
    */
   if( ch->has_aflag( AFF_BERSERK ) )
   {
      ch->affect_strip( gsn_berserk );
      ch->set_color( AT_WEAROFF );
      ch->print( skill_table[gsn_berserk]->msg_off );
      ch->print( "\r\n" );
   }
   return;
}

void strip_grapple( char_data * ch )
{
   if( ch->isnpc() )
      return;

   if( ch->has_aflag( AFF_GRAPPLE ) )
   {
      ch->affect_strip( gsn_grapple );
      ch->set_color( AT_WEAROFF );
      ch->printf( "%s\r\n", skill_table[gsn_grapple]->msg_off );
      if( ch->get_timer( TIMER_PKILLED ) > 0 )
         ch->remove_timer( TIMER_ASUPRESSED );
   }

   if( ch->who_fighting() )
   {
      char_data *vch = ch->who_fighting();

      if( ch->fighting->who->who_fighting( ) == ch && vch->has_aflag( AFF_GRAPPLE ) )
      {
         vch->affect_strip( gsn_grapple );
         vch->set_color( AT_WEAROFF );
         vch->printf( "%s\r\n", skill_table[gsn_grapple]->msg_off );

         if( vch->get_timer( TIMER_PKILLED ) > 0 )
            vch->remove_timer( TIMER_ASUPRESSED );
      }
   }
   return;
}

/*
 * Stop fights.
 */
void char_data::stop_fighting( bool fBoth )
{
   cancel_event( ev_violence, this );
   strip_grapple( this );
   free_fight( this );
   update_pos(  );

   if( !fBoth )   /* major short cut here by Thoric */
      return;

   list < char_data * >::iterator ich;
   for( ich = charlist.begin(  ); ich != charlist.end(  ); ++ich )
   {
      char_data *fch = ( *ich );

      if( fch->who_fighting(  ) == this )
      {
         cancel_event( ev_violence, fch );
         free_fight( fch );
         fch->update_pos(  );
      }
   }
   return;
}

/* Vnums for the various bodyparts */
const int part_vnums[] = {
   OBJ_VNUM_SEVERED_HEAD, /* Head */
   OBJ_VNUM_SLICED_ARM, /* arms */
   OBJ_VNUM_SLICED_LEG, /* legs */
   OBJ_VNUM_TORN_HEART, /* heart */
   OBJ_VNUM_BRAINS,     /* brains */
   OBJ_VNUM_SPILLED_GUTS,  /* guts */
   OBJ_VNUM_HANDS,      /* hands */
   OBJ_VNUM_FOOT,       /* feet */
   OBJ_VNUM_FINGERS,    /* fingers */
   OBJ_VNUM_EAR,        /* ear */
   OBJ_VNUM_EYE,        /* eye */
   OBJ_VNUM_TONGUE,     /* long_tongue */
   OBJ_VNUM_EYESTALK,   /* eyestalks */
   OBJ_VNUM_TENTACLE,   /* tentacles */
   OBJ_VNUM_FINS,       /* fins */
   OBJ_VNUM_WINGS,      /* wings */
   OBJ_VNUM_TAIL,       /* tail */
   OBJ_VNUM_SCALES,     /* scales */
   OBJ_VNUM_CLAWS,      /* claws */
   OBJ_VNUM_FANGS,      /* fangs */
   OBJ_VNUM_HORNS,      /* horns */
   OBJ_VNUM_TUSKS,      /* tusks */
   OBJ_VNUM_TAIL,       /* tailattack */
   OBJ_VNUM_SHARPSCALE, /* sharpscales */
   OBJ_VNUM_BEAK,       /* beak */
   OBJ_VNUM_HAUNCHES,   /* haunches */
   OBJ_VNUM_HOOVES,     /* hooves */
   OBJ_VNUM_PAWS,       /* paws */
   OBJ_VNUM_FORELEG,    /* forelegs */
   OBJ_VNUM_FEATHERS,   /* feathers */
   OBJ_VNUM_SHELL,      /* shell */
   0  /* r2 */
};

/* Messages for flinging off the various bodyparts */
const char *part_messages[] = {
   "$n's severed head plops from its neck.",
   "$n's arm is sliced from $s dead body.",
   "$n's leg is sliced from $s dead body.",
   "$n's heart is torn from $s chest.",
   "$n's brains spill grotesquely from $s head.",
   "$n's guts spill grotesquely from $s torso.",
   "$n's hand is sliced from $s dead body.",
   "$n's foot is sliced from $s dead body.",
   "A finger is sliced from $n's dead body.",
   "$n's ear is sliced from $s dead body.",
   "$n's eye is gouged from its socket.",
   "$n's tongue is torn from $s mouth.",
   "An eyestalk is sliced from $n's dead body.",
   "A tentacle is severed from $n's dead body.",
   "A fin is sliced from $n's dead body.",
   "A wing is severed from $n's dead body.",
   "$n's tail is sliced from $s dead body.",
   "A scale falls from the body of $n.",
   "A claw is torn from $n's dead body.",
   "$n's fangs are torn from $s mouth.",
   "A horn is wrenched from the body of $n.",
   "$n's tusk is torn from $s dead body.",
   "$n's tail is sliced from $s dead body.",
   "A ridged scale falls from the body of $n.",
   "$n's beak is sliced from $s dead body.",
   "$n's haunches are sliced from $s dead body.",
   "A hoof is sliced from $n's dead body.",
   "A paw is sliced from $n's dead body.",
   "$n's foreleg is sliced from $s dead body.",
   "Some feathers fall from $n's dead body.",
   "$n's shell remains.",
   "r2 message."
};

/*
 * Improved Death_cry contributed by Diavolo.
 * Additional improvement by Thoric (and removal of turds... sheesh!)  
 * Support for additional bodyparts by Fireblade
 */
void death_cry( char_data * ch )
{
   const char *msg;
   int vnum, i;

   if( !ch )
   {
      bug( "%s: null ch!", __func__ );
      return;
   }

   vnum = 0;
   msg = nullptr;

   switch ( number_range( 0, 5 ) )
   {
      default:
         msg = "You hear $n's death cry.";
         break;

      case 0:
         msg = "$n screams furiously as $e falls to the ground in a heap!";
         break;

      case 1:
         msg = "$n hits the ground ... DEAD.";
         break;

      case 2:
         msg = "$n catches $s guts in $s hands as they pour through $s fatal wound!";
         break;

      case 3:
         msg = "$n splatters blood on your armor.";
         break;

      case 4:
         msg = "$n gasps $s last breath and blood spurts out of $s mouth and ears.";
         break;

      case 5:
      // TODO: Right now, this selects the first body part it finds, which makes multiple bodyparts on NPCs irrelevant.
      //       Make this randomly select one of the listed bodyparts. I'm sure this was the original intention. - Parsival
         if( ch->has_bparts() )
         {
            for( i = 0; i < MAX_BPART; ++i )
            {
               if( ch->has_bpart( i ) )
               {
                  msg = part_messages[i];
                  vnum = part_vnums[i];
                  break;
               }
            }
         }
         else
         {
	        if( !ch->isnpc() ) // Crash bug fix when NPCs have no bodyparts (and it's possible!). race_table[] only applies to PC races. - Parsival 2017-1228
	        {    
               for( i = 0; i < MAX_BPART; ++i )
               {
                  if( race_table[ch->race]->body_parts.test( i ) )
                  {
                     msg = part_messages[i];
                     vnum = part_vnums[i];
                     break;
                  }
               }
	        }
         }

         if( !msg )
            msg = "You hear $n's death cry.";
         break;
   }

   act( AT_CARNAGE, msg, ch, nullptr, nullptr, TO_ROOM );

   if( vnum )
   {
      obj_data *obj;
      char *name;

      if( !get_obj_index( vnum ) )
      {
         bug( "%s: invalid vnum %d", __func__, vnum );
         return;
      }

      name = ch->isnpc(  )? ch->short_descr : ch->name;
      if( !( obj = get_obj_index( vnum )->create_object( 1 ) ) )
      {
         log_printf( "create_object: %s:%s, line %d.", __FILE__, __func__, __LINE__ );
         return;
      }
      obj->timer = number_range( 4, 7 );
      if( ch->has_aflag( AFF_POISON ) )
         obj->value[3] = 10;

      stralloc_printf( &obj->short_descr, obj->short_descr, name );
      stralloc_printf( &obj->objdesc, obj->objdesc, name );
      obj->to_room( ch->in_room, ch );
   }
}

void raw_kill( char_data * ch, char_data * victim )
{
   if( !victim )
   {
      bug( "%s: null victim! CH: %s", __func__, ch->name );
      return;
   }

   /*
    * backup in case hp goes below 1 
    */
   if( !victim->isnpc(  ) && victim->level == 1 )
   {
      bug( "%s: killing level 1", __func__ );
      return;
   }

   victim->stop_fighting( true );

   if( in_arena( victim ) )
   {
      room_index *location = nullptr;

      log_printf_plus( LOG_INFO, LEVEL_IMMORTAL, "%s bested %s in the arena.", ch->name, victim->name );
      ch->printf( "You bested %s in arena combat!\r\n", victim->name );
      victim->printf( "%s bested you in arena combat!\r\n", ch->name );
      victim->hit = 1;
      victim->position = POS_RESTING;

      location = check_room( victim, victim->in_room );  /* added check to see what continent PC is on - Samson 3-29-98 */

      if( !location )
         location = get_room_index( ROOM_VNUM_LIMBO );

      victim->from_room(  );
      if( !victim->to_room( location ) )
         log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __func__, __LINE__ );

      if( victim->has_aflag( AFF_PARALYSIS ) )
         victim->unset_aflag( AFF_PARALYSIS );

      return;
   }

   /*
    * Take care of morphed characters 
    */
   if( victim->morph )
   {
      do_unmorph_char( victim );
      raw_kill( ch, victim );
      return;
   }

   /*
    * Deal with switched imms 
    */
   if( victim->desc && victim->desc->original )
   {
      char_data *temp = victim;

      interpret( victim, "return" );
      raw_kill( ch, temp );
      return;
   }

   mprog_death_trigger( ch, victim );
   if( victim->char_died(  ) )
      return;

   rprog_death_trigger( victim );
   if( victim->char_died(  ) )
      return;

   death_cry( victim );
   make_corpse( victim, ch );
   if( victim->in_room->sector_type == SECT_OCEANFLOOR
       || victim->in_room->sector_type == SECT_UNDERWATER
       || victim->in_room->sector_type == SECT_WATER_SWIM || victim->in_room->sector_type == SECT_WATER_NOSWIM || victim->in_room->sector_type == SECT_RIVER )
      act( AT_BLOOD, "$n's blood slowly clouds the surrounding water.", victim, nullptr, nullptr, TO_ROOM );
   else if( victim->in_room->sector_type == SECT_AIR )
      act( AT_BLOOD, "$n's blood sprays wildly through the air.", victim, nullptr, nullptr, TO_ROOM );
   else
      make_blood( victim );

   if( victim->isnpc(  ) )
   {
      ++victim->pIndexData->killed;
      victim->extract( true );
      victim = nullptr;
      return;
   }

   victim->set_color( AT_DIEMSG );
   if( victim->pcdata->mdeaths + victim->pcdata->pdeaths < 3 )
      interpret( victim, "help new_death" );
   else
      interpret( victim, "help _DIEMSG_" );

   victim->extract( false );
   if( !victim )
   {
      bug( "%s: extract_char destroyed pc char", __func__ );
      return;
   }

   list < affect_data * >::iterator paf;
   for( paf = victim->affects.begin(  ); paf != victim->affects.end(  ); )
   {
      affect_data *aff = ( *paf );
      ++paf;

      victim->affect_remove( aff );
   }
   victim->set_aflags( race_table[victim->race]->affected );
   victim->set_resists( 0 );
   victim->set_susceps( 0 );
   victim->set_immunes( 0 );
   victim->set_absorbs( 0 );
   victim->carry_weight = 0;
   victim->armor = 100;
   victim->armor += race_table[victim->race]->ac_plus;
   victim->set_attacks( race_table[victim->race]->attacks );
   victim->set_defenses( race_table[victim->race]->defenses );
   victim->mod_str = 0;
   victim->mod_dex = 0;
   victim->mod_wis = 0;
   victim->mod_int = 0;
   victim->mod_con = 0;
   victim->mod_cha = 0;
   victim->mod_lck = 0;
   victim->damroll = 0;
   victim->hitroll = 0;
   victim->mental_state = -10;
   victim->alignment = URANGE( -1000, victim->alignment, 1000 );
   victim->saving_poison_death = race_table[victim->race]->saving_poison_death;
   victim->saving_wand = race_table[victim->race]->saving_wand;
   victim->saving_para_petri = race_table[victim->race]->saving_para_petri;
   victim->saving_breath = race_table[victim->race]->saving_breath;
   victim->saving_spell_staff = race_table[victim->race]->saving_spell_staff;
   victim->position = POS_RESTING;
   victim->hit = UMAX( 1, victim->hit );
   victim->mana = UMAX( 1, victim->mana );
   victim->move = UMAX( 1, victim->move );
   if( victim->pcdata->condition[COND_FULL] != -1 )
      victim->pcdata->condition[COND_FULL] = sysdata->maxcondval / 2;
   if( victim->pcdata->condition[COND_THIRST] != -1 )
      victim->pcdata->condition[COND_THIRST] = sysdata->maxcondval / 2;

   if( IS_SAVE_FLAG( SV_DEATH ) )
      victim->save(  );
}

/*
 * Damage an object. - Thoric
 * Affect player's AC if necessary.
 * Make object into scraps if necessary.
 * Send message about damaged object.
 */
void damage_obj( obj_data * obj )
{
   char_data *ch;

   ch = obj->carried_by;

   obj->separate(  );
   if( !ch->IS_PKILL(  ) || ( ch->IS_PKILL(  ) && !ch->has_pcflag( PCFLAG_GAG ) ) )
      act( AT_OBJECT, "($p gets damaged)", ch, obj, nullptr, TO_CHAR );
   else if( obj->in_room && ( ch = ( *obj->in_room->people.begin(  ) ) ) != nullptr )
   {
      act( AT_OBJECT, "($p gets damaged)", ch, obj, nullptr, TO_ROOM );
      act( AT_OBJECT, "($p gets damaged)", ch, obj, nullptr, TO_CHAR );
      ch = nullptr;
   }

   if( obj->item_type != ITEM_LIGHT )
      oprog_damage_trigger( ch, obj );
   else if( !in_arena( ch ) )
      oprog_damage_trigger( ch, obj );

   if( obj->extracted(  ) )
      return;

   switch ( obj->item_type )
   {
      default:
         obj->make_scraps(  );
         break;

      case ITEM_CONTAINER:
      case ITEM_KEYRING:
      case ITEM_QUIVER:
         if( --obj->value[3] <= 0 )
         {
            if( !in_arena( ch ) )
               obj->make_scraps(  );
            else
               obj->value[3] = 1;
         }
         break;

      case ITEM_LIGHT:
         if( --obj->value[0] <= 0 )
         {
            if( !in_arena( ch ) )
               obj->make_scraps(  );
            else
               obj->value[0] = 1;
         }
         break;

      case ITEM_ARMOR:
         if( ch && obj->value[0] >= 1 )
            ch->armor += obj->apply_ac( obj->wear_loc );
         if( --obj->value[0] <= 0 )
         {
            if( !ch->IS_PKILL(  ) && !in_arena( ch ) )
               obj->make_scraps(  );
            else
            {
               obj->value[0] = 1;
               ch->armor -= obj->apply_ac( obj->wear_loc );
            }
         }
         else if( ch && obj->value[0] >= 1 )
            ch->armor -= obj->apply_ac( obj->wear_loc );
         break;

      case ITEM_WEAPON:
      case ITEM_MISSILE_WEAPON:
         if( --obj->value[6] <= 0 )
         {
            if( !ch->IS_PKILL(  ) && !in_arena( ch ) )
               obj->make_scraps(  );
            else
               obj->value[6] = 1;
         }
         break;

      case ITEM_PROJECTILE:
         if( --obj->value[5] <= 0 )
         {
            if( !ch->IS_PKILL(  ) && !in_arena( ch ) )
               obj->make_scraps(  );
            else
               obj->value[5] = 1;
         }
         break;
   }
   if( ch != nullptr )
      ch->save(  );  /* Stop scrap duping - Samson 1-2-00 */
}

/* Added checks for the 3 new dam types, and removed DAM_PEA - Grimm
 * Removed excess duplication, added hack and lash RISA types - Samson 1-9-00
 * This function also affects simple_damage in mud_comm.c - Samson 4-11-04
 */
double damage_risa( char_data * victim, double dam, int dt )
{
   if( IS_FIRE( dt ) )
      dam = ris_damage( victim, dam, RIS_FIRE );
   else if( IS_COLD( dt ) )
      dam = ris_damage( victim, dam, RIS_COLD );
   else if( IS_ACID( dt ) )
      dam = ris_damage( victim, dam, RIS_ACID );
   else if( IS_ELECTRICITY( dt ) )
      dam = ris_damage( victim, dam, RIS_ELECTRICITY );
   else if( IS_ENERGY( dt ) )
      dam = ris_damage( victim, dam, RIS_ENERGY );
   else if( IS_DRAIN( dt ) )
      dam = ris_damage( victim, dam, RIS_DRAIN );
   else if( dt == gsn_poison || IS_POISON( dt ) )
      dam = ris_damage( victim, dam, RIS_POISON );
   else if( dt == ( TYPE_HIT + DAM_CRUSH ) )
      dam = ris_damage( victim, dam, RIS_BLUNT );
   else if( dt == ( TYPE_HIT + DAM_STAB ) || dt == ( TYPE_HIT + DAM_PIERCE ) || dt == ( TYPE_HIT + DAM_THRUST ) )
      dam = ris_damage( victim, dam, RIS_PIERCE );
   else if( dt == ( TYPE_HIT + DAM_SLASH ) )
      dam = ris_damage( victim, dam, RIS_SLASH );
   else if( dt == ( TYPE_HIT + DAM_HACK ) )
      dam = ris_damage( victim, dam, RIS_HACK );
   else if( dt == ( TYPE_HIT + DAM_LASH ) )
      dam = ris_damage( victim, dam, RIS_LASH );

   return dam;
}

/*
 * Inflict damage from a hit. This is one damn big function.
 */
ch_ret damage( char_data * ch, char_data * victim, double dam, int dt )
{
   short dameq, maxdam, dampmod;
   bool npcvict, loot;
   obj_data *damobj;
   ch_ret retcode;
   int init_gold, new_gold, gold_diff;

   retcode = rNONE;

   if( !ch )
   {
      bug( "%s: null ch!", __func__ );
      return rERROR;
   }

   if( !victim )
   {
      bug( "%s: null victim!", __func__ );
      return rVICT_DIED;
   }

   if( victim->position == POS_DEAD )
      return rVICT_DIED;

   npcvict = victim->isnpc(  );

   /*
    * Check Align types for RIS - Heath ;)
    */
   if( ch->IS_GOOD(  ) && victim->IS_EVIL(  ) )
      dam = ris_damage( victim, dam, RIS_GOOD );
   else if( ch->IS_EVIL(  ) && victim->IS_GOOD(  ) )
      dam = ris_damage( victim, dam, RIS_EVIL );

   /*
    * Check damage types for RIS          -Thoric
    */
   if( dam && dt != TYPE_UNDEFINED )
   {
      dam = damage_risa( victim, dam, dt );

      if( dam == -1 )
      {
         if( dt >= 0 && dt < num_skills )
         {
            bool found = false;
            skill_type *skill = skill_table[dt];

            if( skill->imm_char && skill->imm_char[0] != '\0' )
            {
               act( AT_HIT, skill->imm_char, ch, nullptr, victim, TO_CHAR );
               found = true;
            }
            if( skill->imm_vict && skill->imm_vict[0] != '\0' )
            {
               act( AT_HITME, skill->imm_vict, ch, nullptr, victim, TO_VICT );
               found = true;
            }
            if( skill->imm_room && skill->imm_room[0] != '\0' )
            {
               act( AT_ACTION, skill->imm_room, ch, nullptr, victim, TO_NOTVICT );
               found = true;
            }
            if( found )
               return rNONE;
         }
         dam = 0;
      }
   }

   /*
    * Precautionary step mainly to prevent people in Hell from finding
    * a way out. --Shaddai
    */
   if( victim->in_room->flags.test( ROOM_SAFE ) )
      dam = 0;

   if( dam && npcvict && ch != victim )
   {
      if( !victim->has_actflag( ACT_SENTINEL ) )
      {
         if( victim->hunting )
         {
            if( victim->hunting->who != ch )
            {
               STRFREE( victim->hunting->name );
               victim->hunting->name = QUICKLINK( ch->name );
               victim->hunting->who = ch;
            }
         }
         else if( !victim->has_actflag( ACT_PACIFIST ) ) /* Gorog */
            start_hunting( victim, ch );
      }

      if( victim->hating )
      {
         if( victim->hating->who != ch )
         {
            STRFREE( victim->hating->name );
            victim->hating->name = QUICKLINK( ch->name );
            victim->hating->who = ch;
         }
      }
      else if( !victim->has_actflag( ACT_PACIFIST ) ) /* Gorog */
         start_hating( victim, ch );
   }

   /*
    * Stop up any residual loopholes.
    */
   maxdam = ch->level * 30;
   if( dt == gsn_backstab )
      maxdam = ch->level * 80;
   if( dam > maxdam )
   {
      bug( "%s: %d more than %d points!", __func__, ( int )dam, maxdam );
      log_printf( "** %s (lvl %d) -> %s **", ch->name, ch->level, victim->name );
      dam = maxdam;
   }

   if( victim != ch )
   {
      /*
       * Certain attacks are forbidden.
       * Most other attacks are returned.
       */
      if( victim == supermob ) /* Stop any damage */
         return rNONE;

      if( is_safe( ch, victim ) )
         return rNONE;

      check_attacker( ch, victim );

      if( victim->position > POS_STUNNED )
      {
         if( !victim->fighting && victim->in_room == ch->in_room )
            set_fighting( victim, ch );

         /*
          * vwas: victim->position = POS_FIGHTING; 
          */
         if( victim->isnpc(  ) && victim->fighting )
            victim->position = POS_FIGHTING;
         else if( victim->fighting )
         {
            switch ( victim->style )
            {
               case ( STYLE_EVASIVE ):
                  victim->position = POS_EVASIVE;
                  break;

               case ( STYLE_DEFENSIVE ):
                  victim->position = POS_DEFENSIVE;
                  break;

               case ( STYLE_AGGRESSIVE ):
                  victim->position = POS_AGGRESSIVE;
                  break;

               case ( STYLE_BERSERK ):
                  victim->position = POS_BERSERK;
                  break;

               default:
                  victim->position = POS_FIGHTING;
            }
         }
      }

      if( victim->position > POS_STUNNED )
      {
         if( !ch->fighting && victim->in_room == ch->in_room )
            set_fighting( ch, victim );

         /*
          * If victim is charmed, ch might attack victim's master.
          */
         if( ch->isnpc(  ) && npcvict && victim->has_aflag( AFF_CHARM ) && victim->master && victim->master->in_room == ch->in_room && number_bits( 3 ) == 0 )
         {
            ch->stop_fighting( false );
            retcode = multi_hit( ch, victim->master, TYPE_UNDEFINED );
            return retcode;
         }
      }

      /*
       * More charm stuff.
       */
      if( victim->master == ch )
         unbind_follower( victim, ch );

      /*
       * Inviso attacks ... not.
       */
      if( ch->has_aflag( AFF_INVISIBLE ) )
      {
         ch->affect_strip( gsn_invis );
         ch->affect_strip( gsn_mass_invis );
         ch->unset_aflag( AFF_INVISIBLE );
         act( AT_MAGIC, "$n fades into existence.", ch, nullptr, nullptr, TO_ROOM );
      }

      /*
       * Take away Hide 
       */
      if( ch->is_affected( gsn_hide ) || ch->has_aflag( AFF_HIDE ) )
      {
         ch->affect_strip( gsn_hide );
         ch->unset_aflag( AFF_HIDE );
      }

      /*
       * Take away Sneak 
       */
      if( ch->is_affected( gsn_sneak ) || ch->has_aflag( AFF_SNEAK ) )
      {
         ch->affect_strip( gsn_sneak );
         ch->unset_aflag( AFF_SNEAK );
      }

      /*
       * Damage modifiers.
       */
      if( victim->has_aflag( AFF_SANCTUARY ) )
         dam /= 2;

      if( victim->has_aflag( AFF_PROTECT ) && ch->IS_EVIL(  ) )
         dam -= ( int )( dam / 4 );

      if( dam < 0 )
         dam = 0;

      /*
       * Check for disarm, trip, parry, and dodge.
       */
      if( dt >= TYPE_HIT && ch->in_room == victim->in_room )
      {
         if( ch->has_defense( DFND_DISARM ) && ch->level > 9 && number_percent(  ) < ch->level / 3 )
            disarm( ch, victim );

         if( ch->has_attack( ATCK_TRIP ) && ch->level > 5 && number_percent(  ) < ch->level / 2 )
            trip( ch, victim );

         if( check_parry( ch, victim ) )
            return rNONE;

         if( check_dodge( ch, victim ) )
            return rNONE;

         if( check_tumble( ch, victim ) )
            return rNONE;
      }

      /*
       * Check control panel settings and modify damage
       */
      if( ch->isnpc(  ) )
      {
         if( npcvict )
            dampmod = sysdata->dam_mob_vs_mob;
         else
            dampmod = sysdata->dam_mob_vs_plr;
      }
      else
      {
         if( npcvict )
            dampmod = sysdata->dam_plr_vs_mob;
         else
            dampmod = sysdata->dam_plr_vs_plr;
      }
      if( dampmod > 0 )
         dam = ( dam * dampmod ) / 100;
   }

   /*
    * Code to handle equipment getting damaged, and also support  -Thoric
    * bonuses/penalties for having or not having equipment where hit
    */
   if( dam > 10 && dt != TYPE_UNDEFINED )
   {
      /*
       * get a random body eq part 
       */
      dameq = number_range( WEAR_LIGHT, WEAR_TAIL );
      damobj = victim->get_eq( dameq );
      if( damobj )
      {
         if( dam > damobj->get_resistance(  ) && number_bits( 1 ) == 0 )
            damage_obj( damobj );
         dam -= 5;   /* add a bonus for having something to block the blow */
      }
      else
         dam += 5;   /* add penalty for bare skin! */
   }

   if( ch != victim )
      dam_message( ch, victim, dam, dt, nullptr );

   /*
    * Hurt the victim.
    * Inform the victim of his new state.
    */
   victim->hit -= ( int )dam;

   if( !victim->isnpc(  ) && victim->level >= LEVEL_IMMORTAL && victim->hit < 1 )
      victim->hit = 1;

   /*
    * Make sure newbies dont die 
    */
   if( !victim->isnpc(  ) && victim->level == 1 && victim->hit < 1 )
      victim->hit = 1;

   if( dam > 0 && dt > TYPE_HIT && !victim->has_aflag( AFF_POISON ) && is_wielding_poisoned( ch )
       && !victim->has_immune( RIS_POISON ) && !saves_poison_death( ch->level, victim ) )
   {
      affect_data af;

      af.type = gsn_poison;
      af.duration = 20;
      af.location = APPLY_STR;
      af.modifier = -2;
      af.bit = AFF_POISON;
      victim->affect_join( &af );
      victim->mental_state = URANGE( 20, victim->mental_state + 2, 100 );
   }

   if( !npcvict && victim->level >= LEVEL_IMMORTAL && ch->level >= LEVEL_IMMORTAL && victim->hit < 1 )
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
         if( dt >= 0 && dt < num_skills )
         {
            skill_type *skill = skill_table[dt];

            if( skill->die_char && skill->die_char[0] != '\0' )
               act( AT_DEAD, skill->die_char, ch, nullptr, victim, TO_CHAR );
            if( skill->die_vict && skill->die_vict[0] != '\0' )
               act( AT_DEAD, skill->die_vict, ch, nullptr, victim, TO_VICT );
            if( skill->die_room && skill->die_room[0] != '\0' )
               act( AT_DEAD, skill->die_room, ch, nullptr, victim, TO_NOTVICT );
         }
         act( AT_DEAD, "$n is DEAD!!", victim, 0, 0, TO_ROOM );
         act( AT_DEAD, "You have been KILLED!!\r\n", victim, 0, 0, TO_CHAR );
         break;

      default:
         if( dam > victim->max_hit / 4 )
         {
            act( AT_HURT, "That really did HURT!", victim, 0, 0, TO_CHAR );
            if( number_bits( 3 ) == 0 )
               victim->worsen_mental_state( 1 );   /* Bug fix - Samson 9-24-98 */
         }
         if( victim->hit < victim->max_hit / 4 )

         {
            act( AT_DANGER, "You wish that your wounds would stop BLEEDING so much!", victim, 0, 0, TO_CHAR );
            if( number_bits( 2 ) == 0 )
               victim->worsen_mental_state( 1 );   /* Bug fix - Samson 9-24-98 */
         }
         break;
   }

   /*
    * Payoff for killing things.
    */
   if( victim->position == POS_DEAD )
   {
      if( !in_arena( victim ) )
         group_gain( ch, victim );

      if( !npcvict )
      {
         if( !victim->desc )
            add_loginmsg( victim->name, 17, ( ch->isnpc() ? ch->short_descr : ch->name ) );

         log_printf_plus( LOG_INFO, LEVEL_IMMORTAL, "%s (%d) killed by %s at %d",
                          victim->name, victim->level, ( ch->isnpc(  )? ch->short_descr : ch->name ), victim->in_room->vnum );

         if( !ch->isnpc(  ) && !ch->is_immortal(  ) && ch->pcdata->clan && ch->pcdata->clan->clan_type != CLAN_GUILD && victim != ch )
         {
            if( victim->pcdata->clan && victim->pcdata->clan->name == ch->pcdata->clan->name )
               ;
            else
            {
               char filename[256];

               snprintf( filename, 256, "%s%s.record", CLAN_DIR, ch->pcdata->clan->name.c_str(  ) );
               append_to_file( filename, "&P(%2d) %-12s &wvs &P(%2d) %s &P%s ... &w%s",
                               ch->level, ch->name, victim->level, !victim->CAN_PKILL(  )? "&W<Peaceful>" :
                               victim->pcdata->clan ? victim->pcdata->clan->badge.c_str(  ) : "&P(&WUnclanned&P)", victim->name, ch->in_room->area->name );
            }
         }

         if( !victim->isnpc() && !victim->is_immortal() && victim->pcdata->clan
             && victim->pcdata->clan->clan_type == CLAN_CLAN && ch != victim && !ch->isnpc() )
         {
            char filename[256];

            snprintf( filename, 256, "%s%s.defeats", CLAN_DIR, victim->pcdata->clan->name.c_str() );

            if( ch->pcdata && ch->pcdata->clan && ch->pcdata->clan->name == victim->pcdata->clan->name )
               ;
            else
               append_to_file( filename, "&P(%2d) %-12s &wdefeated by &P(%2d) %s &P%s ... &w%s",
                     victim->level, victim->name, ch->level, !ch->CAN_PKILL( ) ? "&W<Peaceful>" :
                     ch->pcdata->clan ? ch->pcdata->clan->badge.c_str() : "&P(&WUnclanned&P)", ch->name, victim->in_room->area->name );
         }

         /*
          * Dying penalty:
          * 1/4 way back to previous level.
          */
         if( !in_arena( victim ) )
         {
            if( victim->exp > exp_level( victim->level ) )
               victim->gain_exp( ( exp_level( victim->level ) - victim->exp ) / 4 );
         }

      }

      check_killer( ch, victim );

      if( !ch->isnpc(  ) && ch->pcdata->clan )
         update_roster( ch );
      if( !victim->isnpc(  ) && victim->pcdata->clan )
         update_roster( victim );

      if( ch->in_room == victim->in_room )
         loot = legal_loot( ch, victim );
      else
         loot = false;

      raw_kill( ch, victim );
      victim = nullptr;

      if( !ch->isnpc(  ) && loot )
      {
         /*
          * Autogold by Scryn 8/12 
          */
         if( ch->has_pcflag( PCFLAG_AUTOGOLD ) || ch->has_pcflag( PCFLAG_AUTOLOOT ) )
         {
            init_gold = ch->gold;
            interpret( ch, "get coins corpse" );
            new_gold = ch->gold;
            gold_diff = ( new_gold - init_gold );
            if( ch->has_pcflag( PCFLAG_GUILDSPLIT ) && ch->pcdata->clan && ch->pcdata->clan->bank )
            {
               int split = 0;

               if( gold_diff > 0 )
               {
                  float xx = ( float )( ch->pcdata->clan->tithe ) / 100;
                  float pct = ( float )( gold_diff ) * xx;
                  split = ( int )( gold_diff - pct );

                  ch->pcdata->clan->balance += split;
                  save_clan( ch->pcdata->clan );
                  ch->printf( "The Wedgy comes to collect %d gold on behalf of your %s.\r\n", split, ch->pcdata->clan->clan_type == CLAN_GUILD ? "guild" : "clan" );
                  gold_diff -= split;
                  ch->gold -= split;
               }
            }
            if( gold_diff > 0 && ch->has_pcflag( PCFLAG_GROUPSPLIT ) )
               cmdf( ch, "split %d", gold_diff );
         }
         if( ch->has_pcflag( PCFLAG_AUTOLOOT ) && victim != ch )  /* prevent nasty obj problems -- Blodkai */
            interpret( ch, "get all corpse" );

         if( ch->has_pcflag( PCFLAG_SMARTSAC ) )
         {
            obj_data *corpse;

            if( ( corpse = ch->get_obj_here( "corpse" ) ) != nullptr )
            {
               if( corpse->contents.empty(  ) )
                  interpret( ch, "sacrifice corpse" );
               else
                  interpret( ch, "look in corpse" );
            }
         }
         else if( ch->has_pcflag( PCFLAG_AUTOSAC ) && !ch->has_pcflag( PCFLAG_SMARTSAC ) )
            interpret( ch, "sacrifice corpse" );
      }

      if( IS_SAVE_FLAG( SV_KILL ) )
         ch->save(  );
      return rVICT_DIED;
   }

   if( victim == ch )
      return rNONE;

   /*
    * Take care of link dead people.
    */
   if( !npcvict && !victim->desc && !victim->has_pcflag( PCFLAG_NORECALL ) )
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
 * Hit one guy once.
 */
ch_ret one_hit( char_data * ch, char_data * victim, int dt )
{
   obj_data *wield;
   double vic_ac, dam;
   int thac0, plusris, diceroll, attacktype, cnt, prof_bonus, prof_gsn = -1;
   ch_ret retcode = rNONE;

   /*
    * Can't beat a dead char!
    * Guard against weird room-leavings.
    */
   if( victim->position == POS_DEAD || ch->in_room != victim->in_room )
      return rVICT_DIED;

   used_weapon = nullptr;
   /*
    * Figure out the weapon doing the damage       -Thoric
    */
   if( ( wield = ch->get_eq( WEAR_DUAL_WIELD ) ) != nullptr )
   {
      if( dual_flip == true )
         dual_flip = false;
      else
      {
         dual_flip = true;
         wield = ch->get_eq( WEAR_WIELD );
      }
   }
   else
      wield = ch->get_eq( WEAR_WIELD );

   used_weapon = wield;

   if( wield )
      prof_bonus = weapon_prof_bonus_check( ch, wield, prof_gsn );
   else
      prof_bonus = 0;

   /*
    * Lets hope this works, the called function here defaults out to gsn_pugilism, and since
    * every Class can learn it, this should theoretically make it advance with use now 
    */
   prof_bonus = weapon_prof_bonus_check( ch, wield, prof_gsn );

   if( prof_bonus < 0 && ch->level <= 5 ) // Give the lowbies a break!
      prof_bonus = 0;

   if( !( alreadyUsedSkill ) )
   {
      if( ch->fighting && dt == TYPE_UNDEFINED && ch->isnpc(  ) && ch->has_attacks(  ) )
      {
         cnt = 0;
         for( ;; )
         {
            attacktype = number_range( 0, 6 );
            if( ch->has_attack( attacktype ) )
               break;
            if( cnt++ > 16 )
            {
               attacktype = -1;
               break;
            }
         }
         if( attacktype == ATCK_BACKSTAB )
            attacktype = -1;
         if( wield && number_percent(  ) > 25 )
            attacktype = -1;
         if( !wield && number_percent(  ) > 50 )
            attacktype = -1;
         switch ( attacktype )
         {
            default:
               break;
            case ATCK_BITE:
               interpret( ch, "bite" );
               retcode = global_retcode;
               break;
            case ATCK_CLAWS:
               interpret( ch, "claw" );
               retcode = global_retcode;
               break;
            case ATCK_TAIL:
               interpret( ch, "tail" );
               retcode = global_retcode;
               break;
            case ATCK_STING:
               interpret( ch, "sting" );
               retcode = global_retcode;
               break;
            case ATCK_PUNCH:
               interpret( ch, "punch" );
               retcode = global_retcode;
               break;
            case ATCK_KICK:
               interpret( ch, "kick" );
               retcode = global_retcode;
               break;
            case ATCK_TRIP:
               attacktype = 0;
               break;
         }
         alreadyUsedSkill = true;
         if( attacktype >= 0 )
            return retcode;
      }
   }

   if( dt == TYPE_UNDEFINED )
   {
      dt = TYPE_HIT;
      if( wield && wield->item_type == ITEM_WEAPON )
         dt += wield->value[3];
   }

   /*
    * Go grab the thac0 value 
    */
   thac0 = calc_thac0( ch, victim, 0 );

   /*
    * Get the victim's armor Class 
    */
   vic_ac = umax( -19, ( victim->GET_AC(  ) / 10 ) );   /* Use -19 or AC / 10 */

   /*
    * if you can't see what's coming... 
    */
   if( wield && !victim->can_see_obj( wield, false ) )
      vic_ac += 1;
   if( !ch->can_see( victim, false ) )
      vic_ac -= 4;

   /*
    * "learning" between combatients.  Takes the intelligence difference,
    * and multiplies by the times killed to make up a learning bonus
    * given to whoever is more intelligent            -Thoric
    */
   if( ch->fighting && ch->fighting->who == victim )
   {
      short times = ch->fighting->timeskilled;

      if( times )
      {
         short intdiff = ch->get_curr_int(  ) - victim->get_curr_int(  );

         if( intdiff != 0 )
            vic_ac += ( intdiff * times ) / 10;
      }
   }

   /*
    * Weapon proficiency bonus 
    */
   vic_ac += prof_bonus;

   /*
    * Faerie Fire Fix - Tarl 10 Dec 02 
    */
   if( victim->has_aflag( AFF_FAERIE_FIRE ) )
      vic_ac = vic_ac + 20;

   /*
    * The following section allows fighting style to modify AC. Added by Tarl 26 Mar 02 
    */
   if( victim->position == POS_BERSERK )
      vic_ac = 0.8 * vic_ac;
   else if( victim->position == POS_AGGRESSIVE )
      vic_ac = 0.85 * vic_ac;
   else if( victim->position == POS_DEFENSIVE )
      vic_ac = 1.1 * vic_ac;
   else if( victim->position == POS_EVASIVE )
      vic_ac = 1.2 * vic_ac;

   /*
    * The moment of excitement!
    */
   while( ( diceroll = number_bits( 5 ) ) >= 20 )
      ;

   if( diceroll == 0 || ( diceroll != 19 && diceroll < thac0 - vic_ac ) )
   {
      /*
       * Miss. 
       */
      if( prof_gsn != -1 )
         ch->learn_from_failure( prof_gsn );
      damage( ch, victim, 0, dt );

      // Evasive style should try to advance if the victim avoids damage, and is using the evasive style.
      if( !victim->isnpc(  ) && victim->style == STYLE_EVASIVE )
         victim->learn_from_failure( gsn_style_evasive );
      return rNONE;
   }

   /*
    * Hit.
    * Calc damage.
    */
   if( !wield )   /* dice formula fixed by Thoric */
      dam = dice( ch->barenumdie, ch->baresizedie );
   else
   {
      dam = number_range( wield->value[1], wield->value[2] );
      if( ch->Class == CLASS_MONK ) /* Monks get extra damage - Samson 5-31-99 */
         dam += ( ch->level / 10 );
   }

   /*
    * Bonuses.
    */
   dam += ch->GET_DAMROLL(  );

   if( prof_bonus )
      dam += prof_bonus / 4;

   /*
    * Calculate Damage Modifiers from Victim's Fighting Style
    */
   if( victim->position == POS_BERSERK )
      dam = 1.2 * dam;
   else if( victim->position == POS_AGGRESSIVE )
      dam = 1.1 * dam;
   else if( victim->position == POS_DEFENSIVE )
      dam = .85 * dam;
   else if( victim->position == POS_EVASIVE )
      dam = .8 * dam;

   /*
    * Calculate Damage Modifiers from Attacker's Fighting Style
    */
   if( ch->position == POS_BERSERK )
      dam = 1.2 * dam;
   else if( ch->position == POS_AGGRESSIVE )
      dam = 1.1 * dam;
   else if( ch->position == POS_DEFENSIVE )
      dam = .85 * dam;
   else if( ch->position == POS_EVASIVE )
      dam = .8 * dam;

   if( !ch->isnpc(  ) && ch->pcdata->learned[gsn_enhanced_damage] > 0 && number_percent(  ) < ch->pcdata->learned[gsn_enhanced_damage] )
   {
      dam += ( int )( dam * ch->LEARNED( gsn_enhanced_damage ) / 120 );
   }
   else
      ch->learn_from_failure( gsn_enhanced_damage );

   if( !victim->IS_AWAKE(  ) )
      dam *= 2;
   if( dt == gsn_backstab )
      dam *= ( 1 + ( ch->level / 4 ) );
   if( dt == gsn_circle )
      dam *= ( 1 + ( ch->level / 8 ) );

   dam = UMAX( ( int )dam, 1 );

   plusris = 0;

   if( wield )
   {
      if( wield->extra_flags.test( ITEM_MAGIC ) )
         dam = ris_damage( victim, dam, RIS_MAGIC );
      else
         dam = ris_damage( victim, dam, RIS_NONMAGIC );

      /*
       * Handle PLUS1 - PLUS6 ris bits vs. weapon hitroll   -Thoric
       */
      plusris = wield->hitroll(  );
   }
   else if( ch->Class == CLASS_MONK )
   {
      dam = ris_damage( victim, dam, RIS_NONMAGIC );

      if( ch->level <= MAX_LEVEL )
         plusris = 3;

      if( ch->level < 80 )
         plusris = 2;

      if( ch->level < 50 )
         plusris = 1;

      if( ch->level < 30 )
         plusris = 0;
   }
   else
      dam = ris_damage( victim, dam, RIS_NONMAGIC );

   /*
    * check for RIS_PLUSx - Thoric 
    */
   if( dam )
   {
      int x, res, imm, sus, mod;

      if( plusris )
         plusris = RIS_PLUS1 << UMIN( plusris, 7 );

      /*
       * initialize values to handle a zero plusris 
       */
      imm = res = -1;
      sus = 1;

      /*
       * find high ris // FIXME: The absorbs need to get thrown in here at some point.
       */
      for( x = RIS_PLUS1; x <= RIS_PLUS6; ++x )
      {
         if( victim->has_immune( x ) )
            imm = x;
         if( victim->has_resist( x ) )
            res = x;
         if( victim->has_suscep( x ) )
            sus = x;
      }
      mod = 10;
      if( imm >= plusris )
         mod -= 10;
      if( res >= plusris )
         mod -= 2;
      if( sus <= plusris )
         mod += 2;

      /*
       * check if immune 
       */
      if( mod <= 0 )
         dam = -1;
      if( mod != 10 )
         dam = ( dam * mod ) / 10;
   }

   /*
    * immune to damage 
    */
   if( dam == -1 )
   {
      if( dt >= 0 && dt < num_skills )
      {
         skill_type *skill = skill_table[dt];
         bool found = false;

         if( skill->imm_char && skill->imm_char[0] != '\0' )
         {
            act( AT_HIT, skill->imm_char, ch, nullptr, victim, TO_CHAR );
            found = true;
         }
         if( skill->imm_vict && skill->imm_vict[0] != '\0' )
         {
            act( AT_HITME, skill->imm_vict, ch, nullptr, victim, TO_VICT );
            found = true;
         }
         if( skill->imm_room && skill->imm_room[0] != '\0' )
         {
            act( AT_ACTION, skill->imm_room, ch, nullptr, victim, TO_NOTVICT );
            found = true;
         }
         if( found )
            return rNONE;
      }
      dam = 0;
   }
   if( wield )
   {
      list < affect_data * >::iterator paf;

      for( paf = wield->affects.begin(  ); paf != wield->affects.end(  ); ++paf )
      {
         affect_data *af = *paf;

         if( af->location == APPLY_RACE_SLAYER )
         {
            if( ( af->modifier == victim->race )
                || ( ( af->modifier == RACE_UNDEAD ) && IsUndead( victim ) )
                || ( ( af->modifier == RACE_DRAGON ) && IsDragon( victim ) ) || ( ( af->modifier == RACE_GIANT ) && IsGiantish( victim ) ) )
               dam *= 2;
         }
         if( af->location == APPLY_ALIGN_SLAYER )
            switch ( af->modifier )
            {
               case 0:
                  if( victim->IS_EVIL(  ) )
                     dam *= 2;
                  break;
               case 1:
                  if( victim->IS_NEUTRAL(  ) )
                     dam *= 2;
                  break;
               case 2:
                  if( victim->IS_GOOD(  ) )
                     dam *= 2;
                  break;
               default:
                  break;
            }
      }
   }

   if( ( retcode = damage( ch, victim, dam, dt ) ) != rNONE )
      return retcode;
   if( ch->char_died(  ) )
      return rCHAR_DIED;
   if( victim->char_died(  ) )
      return rVICT_DIED;

   retcode = rNONE;
   if( dam == 0 )
      return retcode;

   /*
    * Weapon spell support - Thoric
    * Each successful hit casts a spell
    */
   if( wield && !victim->has_immune( RIS_MAGIC ) && !victim->in_room->flags.test( ROOM_NO_MAGIC ) )
   {
      list < affect_data * >::iterator paf;

      for( paf = wield->pIndexData->affects.begin(  ); paf != wield->pIndexData->affects.end(  ); ++paf )
      {
         affect_data *af = *paf;

         if( af->location == APPLY_WEAPONSPELL && IS_VALID_SN( af->modifier ) && skill_table[af->modifier]->spell_fun )
            retcode = ( *skill_table[af->modifier]->spell_fun ) ( af->modifier, 7, ch, victim );
      }
      if( retcode != rNONE || ch->char_died(  ) || victim->char_died(  ) )
         return retcode;

      for( paf = wield->affects.begin(  ); paf != wield->affects.end(  ); ++paf )
      {
         affect_data *af = *paf;

         if( af->location == APPLY_WEAPONSPELL && IS_VALID_SN( af->modifier ) && skill_table[af->modifier]->spell_fun )
            retcode = ( *skill_table[af->modifier]->spell_fun ) ( af->modifier, 7, ch, victim );
      }
      if( retcode != rNONE || ch->char_died(  ) || victim->char_died(  ) )
         return retcode;
   }

   /*
    * magic shields that retaliate - Thoric
    * Redone in dale fashion   -Heath, 1-18-98
    */

   if( victim->has_aflag( AFF_BLADEBARRIER ) && !ch->has_aflag( AFF_BLADEBARRIER ) )
      retcode = spell_smaug( skill_lookup( "blades" ), victim->level, victim, ch );
   if( retcode != rNONE || ch->char_died(  ) || victim->char_died(  ) )
      return retcode;

   if( victim->has_aflag( AFF_FIRESHIELD ) && !ch->has_aflag( AFF_FIRESHIELD ) )
      retcode = spell_smaug( skill_lookup( "flare" ), victim->level, victim, ch );
   if( retcode != rNONE || ch->char_died(  ) || victim->char_died(  ) )
      return retcode;

   if( victim->has_aflag( AFF_ICESHIELD ) && !ch->has_aflag( AFF_ICESHIELD ) )
      retcode = spell_smaug( skill_lookup( "iceshard" ), victim->level, victim, ch );
   if( retcode != rNONE || ch->char_died(  ) || victim->char_died(  ) )
      return retcode;

   if( victim->has_aflag( AFF_SHOCKSHIELD ) && !ch->has_aflag( AFF_SHOCKSHIELD ) )
      retcode = spell_smaug( skill_lookup( "torrent" ), victim->level, victim, ch );
   if( retcode != rNONE || ch->char_died(  ) || victim->char_died(  ) )
      return retcode;

   if( victim->has_aflag( AFF_ACIDMIST ) && !ch->has_aflag( AFF_ACIDMIST ) )
      retcode = spell_smaug( skill_lookup( "acidshot" ), victim->level, victim, ch );
   if( retcode != rNONE || ch->char_died(  ) || victim->char_died(  ) )
      return retcode;

   if( victim->has_aflag( AFF_VENOMSHIELD ) && !ch->has_aflag( AFF_VENOMSHIELD ) )
      retcode = spell_smaug( skill_lookup( "venomshot" ), victim->level, victim, ch );
   if( retcode != rNONE || ch->char_died(  ) || victim->char_died(  ) )
      return retcode;

   return retcode;
}

/*
 * Do one group of attacks.
 */
ch_ret multi_hit( char_data * ch, char_data * victim, int dt )
{
   float x = 0.0;
   int hchance;
   ch_ret retcode;

   /*
    * add timer to pkillers 
    */
   if( ch->CAN_PKILL(  ) && victim->CAN_PKILL(  ) )
   {
      ch->add_timer( TIMER_RECENTFIGHT, 11, nullptr, 0 );
      victim->add_timer( TIMER_RECENTFIGHT, 11, nullptr, 0 );
   }

   if( ch->isnpc(  ) )
      alreadyUsedSkill = false;

   if( is_attack_supressed( ch ) )
      return rNONE;

   if( ch->has_actflag( ACT_NOATTACK ) )
      return rNONE;

   if( ( retcode = one_hit( ch, victim, dt ) ) != rNONE )
      return retcode;

   if( ch->who_fighting(  ) != victim || dt == gsn_backstab || dt == gsn_circle )
      return rNONE;

   /*
    * Very high chance of hitting compared to chance of going berserk 
    * 40% or higher is always hit.. don't learn anything here though. 
    * -- Altrag 
    */
   hchance = ch->isnpc(  )? 100 : ( ch->LEARNED( gsn_berserk ) * 5 / 2 );
   if( ch->has_aflag( AFF_BERSERK ) && number_percent(  ) < hchance )
      if( ( retcode = one_hit( ch, victim, dt ) ) != rNONE || ch->who_fighting(  ) != victim )
         return retcode;

   x = ch->numattacks;

   if( ch->has_aflag( AFF_HASTE ) )
      x *= 2;

   if( ch->has_aflag( AFF_SLOW ) )
      x /= 2;

   x -= 1.0;

   while( x > 0.9999 )
   {
      /*
       * Moved up here by Tarl, 14 April 02 
       */
      if( ch->get_eq( WEAR_DUAL_WIELD ) )
      {
         hchance = ch->isnpc(  )? ch->level : ch->pcdata->learned[gsn_dual_wield];
         if( number_percent(  ) < hchance )
         {
            if( !ch->get_eq( WEAR_WIELD ) )
            {
               bug( "%s: !WEAR_WIELD in multi_hit in fight.c: %s", __func__, ch->name );
               return rNONE;
            }
            /*
             * dual_flip = true;  
             */
            retcode = one_hit( ch, victim, dt );
            if( retcode != rNONE || ch->who_fighting(  ) != victim )
               return retcode;
         }
         else
            ch->learn_from_failure( gsn_dual_wield );
      }
      else
      {
         retcode = one_hit( ch, victim, dt );
         if( retcode != rNONE || ch->who_fighting(  ) != victim )
            return retcode;
      }
      x -= 1.0;
   }

   if( x > 0.01 )
      if( number_percent(  ) > ( 100 * x ) )
      {
         retcode = one_hit( ch, victim, dt );
         if( retcode != rNONE || ch->who_fighting(  ) != victim )
            return retcode;
      }
   retcode = rNONE;

   hchance = ch->isnpc(  ) ? ( ch->level / 2 ) : 0;
   if( number_percent(  ) < hchance )
      retcode = one_hit( ch, victim, dt );

   return retcode;
}

CMDF( do_assist )
{
   char_data *victim, *bob;

   if( argument.empty(  ) )
   {
      ch->print( "Assist whom?\r\n" );
      return;
   }

   if( !( bob = ch->get_char_room( argument ) ) )
   {
      ch->print( "They aren't here.\r\n" );
      return;
   }

   if( !( victim = bob->who_fighting(  ) ) )
   {
      ch->print( "They aren't fighting anyone!\r\n" );
      return;
   }

   if( victim == ch )
   {
      ch->print( "You hit yourself. Ouch!\r\n" );
      return;
   }

   if( !victim->isnpc(  ) && !in_arena( victim ) )
   {
      if( !ch->CAN_PKILL(  ) )
      {
         ch->print( "You are not a pkiller!\r\n" );
         return;
      }
      if( !victim->CAN_PKILL(  ) )
      {
         ch->print( "You cannot engage them in combat. They are not a pkiller.\r\n" );
         return;
      }
   }

   if( is_safe( ch, victim ) )
      return;

   if( ch->has_aflag( AFF_CHARM ) && ch->master == victim )
   {
      act( AT_PLAIN, "$N is your beloved master.", ch, nullptr, victim, TO_CHAR );
      return;
   }

   if( ch->position > POS_SITTING && ch->position < POS_STANDING )
   {
      ch->print( "You do the best you can!\r\n" );
      return;
   }

   ch->WAIT_STATE( 1 * sysdata->pulseviolence );
   check_attacker( ch, victim );
   multi_hit( ch, victim, TYPE_UNDEFINED );
}

CMDF( do_kill )
{
   char_data *victim;

   if( argument.empty(  ) )
   {
      ch->print( "Kill whom?\r\n" );
      return;
   }

   if( !( victim = ch->get_char_room( argument ) ) )
   {
      ch->print( "They aren't here.\r\n" );
      return;
   }

   if( victim->isnpc(  ) && victim->morph && !in_arena( victim ) )
   {
      ch->print( "This creature appears strange to you. Look upon it more closely before attempting to kill it." );
      return;
   }

   if( !victim->isnpc(  ) && !in_arena( victim ) )
   {
      if( !ch->CAN_PKILL(  ) )
      {
         ch->print( "You are not a pkiller!\r\n" );
         return;
      }
      if( !victim->CAN_PKILL(  ) )
      {
         ch->print( "You cannot engage them in combat. They are not a pkiller.\r\n" );
         return;
      }
   }

   if( victim == ch )
   {
      ch->print( "You hit yourself. Ouch!\r\n" );
      multi_hit( ch, ch, TYPE_UNDEFINED );
      return;
   }

   if( is_safe( ch, victim ) )
      return;

   if( ch->has_aflag( AFF_CHARM ) && ch->master == victim )
   {
      act( AT_PLAIN, "$N is your beloved master.", ch, nullptr, victim, TO_CHAR );
      return;
   }
   if( ch->position > POS_SITTING && ch->position < POS_STANDING )
   {
      ch->print( "You do the best you can!\r\n" );
      return;
   }
   ch->WAIT_STATE( sysdata->pulseviolence );
   check_attacker( ch, victim );
   multi_hit( ch, victim, TYPE_UNDEFINED );
}

CMDF( do_flee )
{
   room_index *was_in, *now_in;
   double los;
   int attempt, oldmap = ch->wmap, oldx = ch->mx, oldy = ch->my;
   short door;
   exit_data *pexit;

   if( !ch->who_fighting(  ) )
   {
      if( ch->position > POS_SITTING && ch->position < POS_STANDING )
      {
         if( ch->mount )
            ch->position = POS_MOUNTED;
         else
            ch->position = POS_STANDING;
      }
      ch->print( "You aren't fighting anyone.\r\n" );
      return;
   }

   if( ch->has_aflag( AFF_BERSERK ) )
   {
      ch->print( "Flee while berserking?  You aren't thinking very clearly...\r\n" );
      return;
   }

   if( ch->has_aflag( AFF_GRAPPLE ) )
   {
      ch->print( "You're too wrapped up to flee!\r\n" );
      return;
   }

   if( ch->move <= 0 )
   {
      ch->print( "You're too exhausted to flee from combat!\r\n" );
      return;
   }

   /*
    * No fleeing while more aggressive than standard or hurt. - Haus 
    */
   if( !ch->isnpc(  ) && ch->position < POS_FIGHTING )
   {
      ch->print( "You can't flee in an aggressive stance...\r\n" );
      return;
   }

   if( ch->isnpc(  ) && ch->position <= POS_SLEEPING )
      return;

   was_in = ch->in_room;
   for( attempt = 0; attempt < 8; ++attempt )
   {
      door = number_door(  );
      if( !( pexit = was_in->get_exit( door ) )
          || !pexit->to_room || IS_EXIT_FLAG( pexit, EX_NOFLEE )
          || pexit->to_room->flags.test( ROOM_DEATH )
          || ( IS_EXIT_FLAG( pexit, EX_CLOSED ) && !ch->has_aflag( AFF_PASS_DOOR ) ) || ( ch->isnpc(  ) && pexit->to_room->flags.test( ROOM_NO_MOB ) ) )
         continue;

      if( ( IS_EXIT_FLAG( pexit, EX_FORTIFIED ) || IS_EXIT_FLAG( pexit, EX_HEAVY )
            || IS_EXIT_FLAG( pexit, EX_MEDIUM ) || IS_EXIT_FLAG( pexit, EX_LIGHT ) || IS_EXIT_FLAG( pexit, EX_CRUMBLING ) ) )
         continue;

      if( ch->mount && ch->mount->fighting )
         ch->mount->stop_fighting( true );

      move_char( ch, pexit, 0, door, false );

      if( ch->has_pcflag( PCFLAG_ONMAP ) || ch->has_actflag( ACT_ONMAP ) )
      {
         now_in = ch->in_room;
         if( ch->wmap == oldmap && ch->mx == oldx && ch->my == oldy )
            continue;
      }
      else
      {
         if( ( now_in = ch->in_room ) == was_in )
            continue;
      }
      ch->in_room = was_in;
      act( AT_FLEE, "$n flees head over heels!", ch, nullptr, nullptr, TO_ROOM );
      ch->in_room = now_in;
      act( AT_FLEE, "$n glances around for signs of pursuit.", ch, nullptr, nullptr, TO_ROOM );
      if( !ch->isnpc(  ) )
      {
         char_data *wf = ch->who_fighting(  );
         int fchance = 0;
         los = ( exp_level( ch->level + 1 ) - exp_level( ch->level ) ) * 0.03;

         if( wf )
            fchance = ( ( wf->get_curr_dex(  ) + wf->get_curr_str(  ) + wf->level ) - ( ch->get_curr_dex(  ) + ch->get_curr_str(  ) + ch->level ) );

         if( ( number_percent(  ) + fchance ) < ch->pcdata->learned[gsn_retreat] )
         {
            ch->in_room = was_in;
            act( AT_FLEE, "You skillfuly retreat from combat.", ch, nullptr, nullptr, TO_CHAR );
            ch->in_room = now_in;
            act( AT_FLEE, "$n skillfully retreats from combat.", ch, nullptr, nullptr, TO_ROOM );
         }
         else
         {
            if( ch->level < LEVEL_AVATAR )
               act_printf( AT_FLEE, ch, nullptr, nullptr, TO_CHAR, "You flee head over heels from combat, losing %.0f experience.", los );
            else
               act( AT_FLEE, "You flee head over heels from combat!", ch, nullptr, nullptr, TO_CHAR );
            ch->gain_exp( 0 - los );
            if( ch->level >= skill_table[gsn_retreat]->skill_level[ch->Class] )
               ch->learn_from_failure( gsn_retreat );
         }

         if( wf && ch->pcdata->deity )
         {
            int level_ratio = URANGE( 1, wf->level / ch->level, LEVEL_AVATAR );

            if( wf && wf->race == ch->pcdata->deity->npcrace[0] )
               ch->adjust_favor( 1, level_ratio );
            else if( wf && wf->race == ch->pcdata->deity->npcrace[1] )
               ch->adjust_favor( 18, level_ratio );
            else if( wf && wf->race == ch->pcdata->deity->npcrace[2] )
               ch->adjust_favor( 19, level_ratio );
            else if( wf && wf->race == ch->pcdata->deity->npcfoe[0] )
               ch->adjust_favor( 16, level_ratio );
            else if( wf && wf->race == ch->pcdata->deity->npcfoe[1] )
               ch->adjust_favor( 20, level_ratio );
            else if( wf && wf->race == ch->pcdata->deity->npcfoe[2] )
               ch->adjust_favor( 21, level_ratio );

            else
               ch->adjust_favor( 0, level_ratio );
         }
      }
      ch->stop_fighting( true );
      return;
   }
   los = ( exp_level( ch->level + 1 ) - exp_level( ch->level ) ) * 0.01;
   if( ch->level < LEVEL_AVATAR )
      act_printf( AT_FLEE, ch, nullptr, nullptr, TO_CHAR, "You attempt to flee from combat, losing %.0f experience.\r\n", los );
   else
      act( AT_FLEE, "You attempt to flee from combat, but can't escape!", ch, nullptr, nullptr, TO_CHAR );
   ch->gain_exp( 0 - los );
   if( ch->level >= skill_table[gsn_retreat]->skill_level[ch->Class] )
      ch->learn_from_failure( gsn_retreat );
}
