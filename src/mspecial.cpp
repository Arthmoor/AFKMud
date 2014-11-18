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
 *                   "Special procedure" module for Mobs                    *
 ****************************************************************************/

/******************************************************
            Desolation of the Dragon MUD II
      (C) 1997, 1998  Jesse DeFer and Heath Leach
 http://dotd.mudservices.com  dotd@dotd.mudservices.com 
 ******************************************************/

/* Any spec_fun added here needs to be added to specfuns.dat as well.
 * If you don't know what that means, ask Samson to take care of it.
 */

#if !defined(WIN32)
#include <dlfcn.h>
#else
#include <windows.h>
#define dlsym( handle, name ) ( (void*)GetProcAddress( (HINSTANCE) (handle), (name) ) )
#define dlerror() GetLastError()
#endif
#include "mud.h"
#include "area.h"
#include "fight.h"
#include "mspecial.h"
#include "objindex.h"
#include "roomindex.h"

SPELLF( spell_smaug );
SPELLF( spell_cure_blindness );
SPELLF( spell_cure_poison );
SPELLF( spell_remove_curse );
void start_hunting( char_data *, char_data * );
void start_hating( char_data *, char_data * );
void set_fighting( char_data *, char_data * );
int IsUndead( char_data * );
void wear_obj( char_data *, obj_data *, bool, int );
void damage_obj( obj_data * );

list < string > speclist;

void free_specfuns( void )
{
   speclist.clear(  );
}

/* Simple load function - no OLC support for now.
 * This is probably something you DONT want builders playing with.
 */
void load_specfuns( void )
{
   FILE *fp;
   char filename[256];
   string sfun;

   speclist.clear(  );

   snprintf( filename, 256, "%sspecfuns.dat", SYSTEM_DIR );
   if( !( fp = fopen( filename, "r" ) ) )
   {
      bug( "%s: FATAL - cannot load specfuns.dat, exiting.", __func__ );
      perror( filename );
      exit( 1 );
   }
   else
   {
      for( ;; )
      {
         if( feof( fp ) )
         {
            bug( "%s: Premature end of file!", __func__ );
            FCLOSE( fp );
            return;
         }
         sfun.clear(  );
         fread_string( sfun, fp );
         if( sfun == "$" )
            break;
         speclist.push_back( sfun );
      }
      FCLOSE( fp );
   }
}

/* Simple validation function to be sure a function can be used on mobs */
bool validate_spec_fun( const string & name )
{
   list < string >::iterator spec;

   for( spec = speclist.begin(  ); spec != speclist.end(  ); ++spec )
   {
      string specfun = *spec;

      if( !str_cmp( specfun, name ) )
         return true;
   }
   return false;
}

/*
 * Given a name, return the appropriate spec_fun.
 */
SPEC_FUN *m_spec_lookup( const string & name )
{
   void *funHandle;
#if !defined(WIN32)
   const char *error;
#else
   DWORD error;
#endif

   funHandle = dlsym( sysdata->dlHandle, name.c_str(  ) );
   if( ( error = dlerror(  ) ) )
   {
      bug( "%s: %s", __func__, error );
      return NULL;
   }
   return ( SPEC_FUN * ) funHandle;
}

char_data *spec_find_victim( char_data * ch, bool combat )
{
   list < char_data * >::iterator ich;
   char_data *chosen = NULL;
   int count = 0;

   if( !combat )
   {
      for( ich = ch->in_room->people.begin(  ); ich != ch->in_room->people.end(  ); ++ich )
      {
         char_data *victim = *ich;

         if( victim != ch && ch->can_see( victim, false ) && number_bits( 1 ) == 0 )
            return victim;
      }
      return NULL;
   }

   for( ich = ch->in_room->people.begin(  ); ich != ch->in_room->people.end(  ); ++ich )
   {
      char_data *victim = *ich;

      if( victim->who_fighting(  ) != ch )
         continue;

      if( !number_range( 0, count ) )
         chosen = victim, ++count;
   }
   return chosen;
}

/* if a spell casting mob is hating someone... try and summon them */
bool summon_if_hating( char_data * ch )
{
   int sn;

   /*
    * Gee, if we have no summon spell for some reason, they can't very well cast it. 
    */
   if( ( sn = skill_lookup( "summon" ) ) == -1 )
      return false;

   if( ch->fighting || ch->fearing || !ch->hating || ch->in_room->flags.test( ROOM_SAFE ) )
      return false;

   /*
    * if player is close enough to hunt... don't summon 
    */
   if( ch->hunting )
      return false;

   /*
    * If the mob isn't of sufficient level to cast summon, then why let it? 
    */
   if( ch->level < skill_table[sn]->skill_level[ch->Class] )
      return false;

   char name[MIL];
   one_argument( ch->hating->name, name );

   /*
    * make sure the char exists - works even if player quits 
    */
   char_data *victim;
   bool found = false;
   list < char_data * >::iterator ich;
   for( ich = charlist.begin(  ); ich != charlist.end(  ); ++ich )
   {
      victim = *ich;

      if( !str_cmp( ch->hating->name, victim->name ) )
      {
         found = true;
         break;
      }
   }

   if( !found )
      return false;
   if( ch->in_room == victim->in_room )
      return false;

   /*
    * Modified so Mobs can't drag you back from the ends of the earth - Samson 12-25-98 
    */
   if( ch->in_room->area != victim->in_room->area )
      return false;

   if( ch->position < POS_STANDING )
      return false;

   if( !victim->isnpc(  ) )
      cmdf( ch, "cast summon 0.%s", name );
   else
      cmdf( ch, "cast summon %s", name );
   return true;
}

/*
 * Core procedure for dragons.
 */
bool dragon( char_data * ch, const string & spellname )
{
   if( ch->position != POS_FIGHTING && ch->position != POS_EVASIVE && ch->position != POS_DEFENSIVE && ch->position != POS_AGGRESSIVE && ch->position != POS_BERSERK )
      return false;

   char_data *victim = NULL;
   if( !( victim = spec_find_victim( ch, true ) ) )
      return false;

   int sn;
   if( ( sn = skill_lookup( spellname ) ) < 0 )
      return false;
   ( *skill_table[sn]->spell_fun ) ( sn, ch->level, ch, victim );
   return true;
}

/*
 * Special procedures for mobiles.
 */
SPECF( spec_breath_any )
{
   if( ch->position != POS_FIGHTING && ch->position != POS_EVASIVE && ch->position != POS_DEFENSIVE && ch->position != POS_AGGRESSIVE && ch->position != POS_BERSERK )
      return false;

   switch ( number_bits( 3 ) )
   {
      default:
      case 0:
         return spec_breath_fire( ch );
      case 1:
      case 2:
         return spec_breath_lightning( ch );
      case 3:
         return spec_breath_gas( ch );
      case 4:
         return spec_breath_acid( ch );
      case 5:
      case 6:
      case 7:
         return spec_breath_frost( ch );
   }
   return false;
}

SPECF( spec_breath_acid )
{
   return dragon( ch, "acid breath" );
}

SPECF( spec_breath_fire )
{
   return dragon( ch, "fire breath" );
}

SPECF( spec_breath_frost )
{
   return dragon( ch, "frost breath" );
}

SPECF( spec_breath_gas )
{
   if( ch->position != POS_FIGHTING && ch->position != POS_EVASIVE && ch->position != POS_DEFENSIVE && ch->position != POS_AGGRESSIVE && ch->position != POS_BERSERK )
      return false;

   int sn;
   if( ( sn = skill_lookup( "gas breath" ) ) < 0 )
      return false;
   ( *skill_table[sn]->spell_fun ) ( sn, ch->level, ch, NULL );
   return true;
}

SPECF( spec_breath_lightning )
{
   return dragon( ch, "lightning breath" );
}

/*
** New Healer Procedure, Modified by Tarl/Lemming
*/
SPECF( spec_cast_adept )
{
   char_data *victim = NULL;
   int percent;

   if( !ch->IS_AWAKE(  ) || ch->fighting )
      return false;

   if( !( victim = spec_find_victim( ch, false ) ) )
      return false;

   /*
    * Wastes too many CPU cycles to let the healer stuff work on them 
    */
   if( victim->isnpc(  ) )
      return false;

   if( ch->position < POS_AGGRESSIVE || ch->position > POS_MOUNTED )
      return false;

   switch ( number_bits( 4 ) )
   {
      case 0:
         act( AT_MAGIC, "$n utters the word 'ciroht'.", ch, NULL, NULL, TO_ROOM );
         spell_smaug( skill_lookup( "armor" ), ch->level, ch, victim );
         return true;

      case 1:
         act( AT_MAGIC, "$n utters the word 'sunimod'.", ch, NULL, NULL, TO_ROOM );
         spell_smaug( skill_lookup( "bless" ), ch->level, ch, victim );
         return true;

      case 2:
         act( AT_MAGIC, "$n utters the word 'suah'.", ch, NULL, NULL, TO_ROOM );
         spell_cure_blindness( skill_lookup( "cure blindness" ), ch->level, ch, victim );
         return true;

      case 3:
         act( AT_MAGIC, "$n utters the word 'nyrcs'.", ch, NULL, NULL, TO_ROOM );
         spell_cure_poison( skill_lookup( "cure poison" ), ch->level, ch, victim );
         return true;

      case 4:
         act( AT_MAGIC, "$n utters the word 'gartla'.", ch, NULL, NULL, TO_ROOM );
         spell_smaug( skill_lookup( "refresh" ), ch->level, ch, victim );
         return true;

      case 5:
         act( AT_MAGIC, "$n utters the word 'gorog'.", ch, NULL, NULL, TO_ROOM );
         spell_remove_curse( skill_lookup( "remove curse" ), ch->level, ch, victim );
         return true;

      default:
         if( victim->max_hit > 0 )
            percent = ( 100 * victim->hit ) / victim->max_hit;
         else
            percent = -1;

         if( percent >= 90 )
         {
            act( AT_MAGIC, "$n utters the word 'nran'.", ch, NULL, NULL, TO_ROOM );
            spell_smaug( skill_lookup( "cure light" ), ch->level, ch, victim );
         }
         else if( percent >= 75 )
         {
            act( AT_MAGIC, "$n utters the word 'naimad'.", ch, NULL, NULL, TO_ROOM );
            spell_smaug( skill_lookup( "cure serious" ), ch->level, ch, victim );
         }
         else if( percent >= 60 )
         {
            act( AT_MAGIC, "$n utters the word 'piwd'.", ch, NULL, NULL, TO_ROOM );
            spell_smaug( skill_lookup( "cure critical" ), ch->level, ch, victim );
         }
         else
         {
            act( AT_MAGIC, "$n utters the word 'nosmas'.", ch, NULL, NULL, TO_ROOM );
            spell_smaug( skill_lookup( "heal" ), ch->level, ch, victim );
         }
         return true;
   }
}

SPECF( spec_cast_cleric )
{
   char_data *victim = NULL;
   string spell;
   int sn;

   if( !ch->IS_AWAKE(  ) )
      return false;

   summon_if_hating( ch );

   if( ch->position < POS_AGGRESSIVE || ch->position > POS_MOUNTED )
      return false;

   victim = spec_find_victim( ch, true );

   if( !victim || victim == ch )
      return false;

   switch ( number_bits( 5 ) )
   {
      case 0:
         spell = "cause light";
         break;
      case 1:
         spell = "curse";
         break;
      case 2:
         spell = "spiritual hammer";
         break;
      case 3:
         spell = "dispel evil";
         break;
      case 4:
         spell = "cause serious";
         break;
      case 5:
         spell = "blindness";
         break;
      case 6:
         spell = "hold person";
         break;
      case 7:
         spell = "dispel magic";
         break;
      case 8:
         spell = "cause critical";
         break;
      case 9:
         spell = "silence";
         break;
      case 10:
         spell = "harm";
         break;
      case 11:
         spell = "flamestrike";
         break;
      case 12:
         spell = "earthquake";
         break;
      case 13:
         spell = "spectral furor";
         break;
      case 14:
         spell = "holy word";
         break;
      case 15:
         spell = "spiritual wrath";
         break;
      default:
         spell = "cause light";
         break;
   }

   if( ( sn = skill_lookup( spell ) ) < 0 )
      return false;

   if( ch->level < skill_table[sn]->skill_level[ch->Class] )
      return false;

   cmdf( ch, "cast %s %s", skill_table[sn]->name, victim->name );
   return true;
}

SPECF( spec_cast_mage )
{
   char_data *victim = NULL;
   string spell;
   int sn;

   if( !ch->IS_AWAKE(  ) )
      return false;

   summon_if_hating( ch );

   if( ch->position < POS_AGGRESSIVE || ch->position > POS_MOUNTED )
      return false;

   victim = spec_find_victim( ch, true );

   if( !victim || victim == ch )
      return false;

   switch ( number_bits( 5 ) )
   {
      case 0:
         spell = "magic missile";
         break;
      case 1:
         spell = "burning hands";
         break;
      case 2:
         spell = "shocking grasp";
         break;
      case 3:
         spell = "sleep";
         break;
      case 4:
         spell = "weaken";
         break;
      case 5:
         spell = "acid blast";
         break;
      case 6:
         spell = "dispel magic";
         break;
      case 7:
         spell = "magnetic thrust";
         break;
      case 8:
         spell = "blindness";
         break;
      case 9:
         spell = "web";
         break;
      case 10:
         spell = "colour spray";
         break;
      case 11:
         spell = "sulfrous spray";
         break;
      case 12:
         spell = "caustic fount";
         break;
      case 13:
         spell = "lightning bolt";
         break;
      case 14:
         spell = "fireball";
         break;
      case 15:
         spell = "acetum primus";
         break;
      case 16:
         spell = "cone of cold";
         break;
      case 17:
         spell = "quantum spike";
         break;
      case 18:
         spell = "scorching surge";
         break;
      case 19:
         spell = "meteor swarm";
         break;
      case 20:
         spell = "spiral blast";
         break;
      default:
         spell = "magic missile";
         break;
   }

   if( ( sn = skill_lookup( spell ) ) < 0 )
      return false;

   if( ch->level < skill_table[sn]->skill_level[ch->Class] )
      return false;

   cmdf( ch, "cast %s %s", skill_table[sn]->name, victim->name );
   return true;
}

SPECF( spec_cast_undead )
{
   char_data *victim = NULL;
   string spell;
   int sn;

   if( !ch->IS_AWAKE(  ) )
      return false;

   summon_if_hating( ch );

   if( ch->position < POS_AGGRESSIVE || ch->position > POS_MOUNTED )
      return false;

   victim = spec_find_victim( ch, true );

   if( !victim || victim == ch )
      return false;

   switch ( number_bits( 5 ) )
   {
      case 0:
         spell = "chill touch";
         break;
      case 2:
         spell = "sleep";
         break;
      case 3:
         spell = "weaken";
         break;
      case 4:
         spell = "black hand";
         break;
      case 5:
         spell = "black fist";
         break;
      case 6:
         spell = "dispel magic";
         break;
      case 7:
         spell = "fatigue";
         break;
      case 8:
         spell = "lethargy";
         break;
      case 9:
         spell = "necromantic touch";
         break;
      case 10:
         spell = "withering hand";
         break;
      case 11:
         spell = "death chant";
         break;
      case 12:
         spell = "paralyze";
         break;
      case 13:
         spell = "black lightning";
         break;
      case 14:
         spell = "harm";
         break;
      case 15:
         spell = "death aura";
         break;
      case 16:
         spell = "death spell";
         break;
      case 17:
         spell = "vampiric touch";
         break;
      default:
         spell = "chill touch";
         break;
   }

   if( ( sn = skill_lookup( spell ) ) < 0 )
      return false;

   if( ch->level < skill_table[sn]->skill_level[ch->Class] )
      return false;

   cmdf( ch, "cast %s %s", skill_table[sn]->name, victim->name );
   return true;
}

SPECF( spec_guard )
{
   if( !ch->IS_AWAKE(  ) || ch->fighting )
      return false;

   int max_evil = 300;
   char_data *ech = NULL;

   list < char_data * >::iterator ich;
   for( ich = ch->in_room->people.begin(  ); ich != ch->in_room->people.end(  ); ++ich )
   {
      char_data *victim = *ich;

      if( victim->fighting && victim->who_fighting(  ) != ch && victim->alignment < max_evil )
      {
         max_evil = victim->alignment;
         ech = victim;
      }
   }

   if( ech )
   {
      act( AT_YELL, "$n screams 'PROTECT THE INNOCENT!!  BANZAI!!  SPOON!!", ch, NULL, NULL, TO_ROOM );
      multi_hit( ch, ech, TYPE_UNDEFINED );
      return true;
   }
   return false;
}

SPECF( spec_janitor )
{
   if( !ch->IS_AWAKE(  ) )
      return false;

   list < obj_data * >::iterator iobj;
   for( iobj = ch->in_room->objects.begin(  ); iobj != ch->in_room->objects.end(  ); )
   {
      obj_data *trash = *iobj;
      ++iobj;

      if( !trash->wear_flags.test( ITEM_TAKE ) || trash->extra_flags.test( ITEM_BURIED ) )
         continue;

      if( trash->extra_flags.test( ITEM_PROTOTYPE ) && !ch->has_actflag( ACT_PROTOTYPE ) )
         continue;

      if( ch->has_actflag( ACT_ONMAP ) )
      {
         if( !is_same_obj_map( ch, trash ) )
            continue;
      }

      if( trash->item_type == ITEM_DRINK_CON || trash->item_type == ITEM_TRASH
          || trash->cost < 10 || ( trash->pIndexData->vnum == OBJ_VNUM_SHOPPING_BAG && trash->contents.empty(  ) ) )
      {
         act( AT_ACTION, "$n picks up some trash.", ch, NULL, NULL, TO_ROOM );
         trash->from_room(  );
         trash->to_char( ch );
         return true;
      }
   }
   return false;
}

/* For area conversion compatibility - DON'T REMOVE THIS */
SPECF( spec_poison )
{
   return spec_snake( ch );
}

SPECF( spec_snake )
{
   char_data *victim;

   if( ch->position < POS_AGGRESSIVE || ch->position > POS_MOUNTED )
      return false;

   if( !( victim = ch->who_fighting(  ) ) )
      return false;

   if( number_percent(  ) > ch->level )
      return false;

   act( AT_HIT, "You bite $N!", ch, NULL, victim, TO_CHAR );
   act( AT_ACTION, "$n bites $N!", ch, NULL, victim, TO_NOTVICT );
   act( AT_POISON, "$n bites you!", ch, NULL, victim, TO_VICT );
   spell_smaug( gsn_poison, ch->level, ch, victim );
   return true;
}

SPECF( spec_thief )
{
   if( ch->position != POS_STANDING )
      return false;

   list < char_data * >::iterator ich;
   for( ich = ch->in_room->people.begin(  ); ich != ch->in_room->people.end(  ); ++ich )
   {
      char_data *victim = *ich;

      if( victim->isnpc(  ) || victim->is_immortal(  ) || number_bits( 2 ) != 0 || !ch->can_see( victim, false ) )
         continue;

      if( victim->IS_AWAKE(  ) && number_range( 0, ch->level ) == 0 )
      {
         act( AT_ACTION, "You discover $n's hands in your sack of gold!", ch, NULL, victim, TO_VICT );
         act( AT_ACTION, "$N discovers $n's hands in $S sack of gold!", ch, NULL, victim, TO_NOTVICT );
         return true;
      }
      else
      {
         int maxgold = ch->level * ch->level * 1000;
         int gold = victim->gold * number_range( 1, URANGE( 2, ch->level / 4, 10 ) ) / 100;
         ch->gold += 9 * gold / 10;
         victim->gold -= gold;
         if( ch->gold > maxgold )
            ch->gold = maxgold / 2;
         return true;
      }
   }
   return false;
}

void submit( char_data * ch, char_data * t )
{
   switch ( number_range( 1, 8 ) )
   {
      case 1:
         cmdf( ch, "bow %s", t->name );
         break;
      case 2:
         cmdf( ch, "smile %s", t->name );
         break;
      case 3:
         cmdf( ch, "wink %s", t->name );
         break;
      case 4:
         cmdf( ch, "wave %s", t->name );
         break;
      default:
         act( AT_PLAIN, "$n nods $s head at you", ch, 0, t, TO_VICT );
         act( AT_PLAIN, "$n nods $s head at $N", ch, 0, t, TO_NOTVICT );
         break;
   }
}

void sayhello( char_data * ch, char_data * t )
{
   if( ch->IS_EVIL(  ) )
   {
      switch ( number_range( 1, 10 ) )
      {
         default:
         case 1:
            interpret( ch, "say Hey doofus, go get a life!" );
            break;
         case 2:
            if( t->sex == SEX_FEMALE )
               interpret( ch, "say Get lost ...... witch" );
            else
               interpret( ch, "say Are you talking to me, punk?" );
            break;
         case 3:
            interpret( ch, "say May the road you travel be cursed!" );
            break;
         case 4:
            if( t->sex == SEX_FEMALE )
               cmdf( ch, "say Make way! Make way for me, %s!", t->name );
            break;
         case 5:
            interpret( ch, "say May the evil godling Ixzuul grin evily at you." );
            break;
         case 6:
            interpret( ch, "say Not you again..." );
            break;
         case 7:
            interpret( ch, "say You are always welcome here, great one, now go clean the stables." );
            break;
         case 8:
            interpret( ch, "say Ya know, those smugglers are men after my own heart..." );
            break;
         case 9:
            if( time_info.hour > sysdata->hoursunrise && time_info.hour < sysdata->hournoon )
               cmdf( ch, "say It's morning, %s, do you know where your brains are?", t->name );
            else if( time_info.hour >= sysdata->hournoon && time_info.hour < sysdata->hoursunset )
               cmdf( ch, "say It's afternoon, %s, do you know where your parents are?", t->name );
            else if( time_info.hour >= sysdata->hoursunset && time_info.hour <= sysdata->hourmidnight )
               cmdf( ch, "say It's evening, %s, do you know where your kids are?", t->name );
            else
               cmdf( ch, "say Up for a midnight stroll, %s?\n", t->name );
            break;
         case 10:
         {
            char buf2[80];

            if( time_info.hour < sysdata->hoursunrise )
               mudstrlcpy( buf2, "evening", 80 );
            else if( time_info.hour < sysdata->hournoon )
               mudstrlcpy( buf2, "morning", 80 );
            else if( time_info.hour < sysdata->hoursunset )
               mudstrlcpy( buf2, "afternoon", 80 );
            else
               mudstrlcpy( buf2, "evening", 80 );

            if( IS_CLOUDY( ch->in_room->area->weather ) )
               cmdf( ch, "say Nice %s to go for a walk, %s, I hate it.", buf2, t->name );
            else if( IS_RAINING( ch->in_room->area->weather ) )
               cmdf( ch, "say I hope %s's rain never clears up.. don't you %s?", buf2, t->name );
            else if( IS_SNOWING( ch->in_room->area->weather ) )
               cmdf( ch, "say What a wonderful miserable %s, %s!", buf2, t->name );
            else
               cmdf( ch, "say Such a terrible %s, don't you think?", buf2 );
            break;
         }
      }
   }
   else
   {
      switch ( number_range( 1, 10 ) )
      {
         default:
         case 1:
            interpret( ch, "say Greetings, adventurer" );
            break;
         case 2:
            if( t->sex == SEX_FEMALE )
               interpret( ch, "say Good day, milady" );
            else
               interpret( ch, "say Good day, lord" );
            break;
         case 3:
            if( t->sex == SEX_FEMALE )
               interpret( ch, "say Pleasant Journey, Mistress" );
            else
               interpret( ch, "say Pleasant Journey, Master" );
            break;
         case 4:
            if( t->sex == SEX_FEMALE )
               cmdf( ch, "say Make way! Make way for the lady %s!", t->name );
            else
               cmdf( ch, "say Make way! Make way for the lord %s!", t->name );
            break;
         case 5:
            interpret( ch, "say May the prophet smile upon you" );
            break;
         case 6:
            interpret( ch, "say It is a pleasure to see you again." );
            break;
         case 7:
            interpret( ch, "say You are always welcome here, great one" );
            break;
         case 8:
            interpret( ch, "say My lord bids you greetings" );
            break;
         case 9:
            if( time_info.hour > sysdata->hoursunrise && time_info.hour < sysdata->hournoon )
               cmdf( ch, "say Good morning, %s", t->name );
            else if( time_info.hour >= sysdata->hournoon && time_info.hour < sysdata->hoursunset )
               cmdf( ch, "say Good afternoon, %s", t->name );
            else if( time_info.hour >= sysdata->hoursunset && time_info.hour <= sysdata->hourmidnight )
               cmdf( ch, "say Good evening, %s", t->name );
            else
               cmdf( ch, "say Up for a midnight stroll, %s?", t->name );
            break;
         case 10:
         {
            char buf2[80];

            if( time_info.hour < sysdata->hoursunrise )
               mudstrlcpy( buf2, "evening", 80 );
            else if( time_info.hour < sysdata->hournoon )
               mudstrlcpy( buf2, "morning", 80 );
            else if( time_info.hour < sysdata->hoursunset )
               mudstrlcpy( buf2, "afternoon", 80 );
            else
               mudstrlcpy( buf2, "evening", 80 );
            if( IS_CLOUDY( ch->in_room->area->weather ) )
               cmdf( ch, "say Nice %s to go for a walk, %s.", buf2, t->name );
            else if( IS_RAINING( ch->in_room->area->weather ) )
               cmdf( ch, "say I hope %s's rain clears up.. don't you %s?", buf2, t->name );
            else if( IS_SNOWING( ch->in_room->area->weather ) )
               cmdf( ch, "say How can you be out on such a miserable %s, %s!", buf2, t->name );
            else
               cmdf( ch, "say Such a pleasant %s, don't you think?", buf2 );
            break;
         }
      }
   }
}

void greet_people( char_data * ch )
{
   if( ch->has_actflag( ACT_GREET ) )
   {
      list < char_data * >::iterator ich;
      for( ich = ch->in_room->people.begin(  ); ich != ch->in_room->people.end(  ); ++ich )
      {
         char_data *tch = *ich;

         if( !tch->isnpc(  ) && ch->can_see( tch, false ) && number_range( 1, 8 ) == 1 )
         {
            if( tch->level > ch->level )
            {
               submit( ch, tch );
               sayhello( ch, tch );
               break;
            }
         }
      }
   }
}

bool callforhelp( char_data * ch, SPEC_FUN * spec )
{
   list < char_data * >::iterator ich;
   short count = 0;

   for( ich = charlist.begin(  ); ich != charlist.end(  ) && count <= 2; ++ich )
   {
      char_data *vch = *ich;

      if( ch != vch && !vch->hunting && spec == vch->spec_fun )
      {
         start_hating( vch, ch->who_fighting(  ) );
         start_hunting( vch, ch->who_fighting(  ) );
         ++count;
      }
   }
   if( count > 0 )
      return true;
   return false;
}

char_data *race_align_hatee( char_data * ch )
{
   list < char_data * >::iterator ich;

   for( ich = ch->in_room->people.begin(  ); ich != ch->in_room->people.end(  ); ++ich )
   {
      char_data *vch = *ich;

      if( ch->can_see( vch, false )
          && ( ( vch->IS_EVIL(  ) && ch->IS_GOOD(  ) ) || ( vch->IS_GOOD(  ) && ch->IS_EVIL(  ) )
               || ( IsUndead( vch ) && !IsUndead( ch ) ) || ( !IsUndead( vch ) && IsUndead( ch ) ) ) )
         return vch;
   }
   return NULL;
}

SPECF( spec_GenericCityguard )
{
   char_data *hatee, *fighting;

   if( !ch->IS_AWAKE(  ) )
      return false;

   fighting = ch->who_fighting(  );

   if( fighting && fighting->spec_fun == ch->spec_fun )
   {
      ch->stop_fighting( true );
      interpret( ch, "say Pardon me, I didn't mean to attack you!" );
      return true;
   }

   if( fighting )
   {
      if( number_bits( 2 ) > 2 )
         interpret( ch, "yell To me, my fellows!  I need thy aid!" );
      if( !callforhelp( ch, ch->spec_fun ) )
         return true;
   }

   if( ( hatee = race_align_hatee( ch ) ) != NULL )
   {
      interpret( ch, "say Die!" );
      set_fighting( ch, hatee );
      multi_hit( ch, hatee, TYPE_UNDEFINED );
      return true;
   }
   greet_people( ch );
   return false;
}

SPECF( spec_GenericCitizen )
{
   if( !ch->IS_AWAKE(  ) )
      return false;

   if( ch->who_fighting(  ) )
   {
      if( !callforhelp( ch, ch->spec_fun ) )
      {
         interpret( ch, "say Alas, I am alone!" );
         return true;
      }
   }
   greet_people( ch );
   return true;
}

SPECF( spec_fido )
{
   if( !ch->IS_AWAKE(  ) )
      return false;

   list < obj_data * >::iterator iobj;
   for( iobj = ch->in_room->objects.begin(  ); iobj != ch->in_room->objects.end(  ); )
   {
      obj_data *corpse = *iobj;
      ++iobj;

      if( corpse->item_type != ITEM_CORPSE_NPC )
         continue;

      if( ch->has_actflag( ACT_ONMAP ) )
      {
         if( !is_same_obj_map( ch, corpse ) )
            continue;
      }
      act( AT_ACTION, "$n savagely devours a corpse.", ch, NULL, NULL, TO_ROOM );

      list < obj_data * >::iterator iobj2;
      for( iobj2 = corpse->contents.begin(  ); iobj2 != corpse->contents.end(  ); )
      {
         obj_data *obj = *iobj2;
         ++iobj2;

         obj->from_obj(  );
         obj->to_room( ch->in_room, ch );
      }
      corpse->extract(  );
      return true;
   }
   return false;
}

bool StandUp( char_data * ch )
{
   if( ch->wait )
      return false;

   if( ch->position <= POS_STUNNED || ch->position >= POS_BERSERK )
      return false;

   if( ch->hit > ( ch->max_hit / 2 ) )
      act( AT_PLAIN, "$n quickly stands up.", ch, NULL, NULL, TO_ROOM );
   else if( ch->hit > ( ch->max_hit / 6 ) )
      act( AT_PLAIN, "$n slowly stands up.", ch, NULL, NULL, TO_ROOM );
   else
      act( AT_PLAIN, "$n gets to $s feet very slowly.", ch, NULL, NULL, TO_ROOM );

   if( ch->who_fighting(  ) )
      ch->position = POS_FIGHTING;
   else
      ch->position = POS_STANDING;

   return true;
}

void MakeNiftyAttack( char_data * ch )
{
   char_data *fighting;
   int num;

   if( ch->wait )
      return;

   if( ch->position != POS_FIGHTING || !( fighting = ch->who_fighting(  ) ) )
      return;

   num = number_range( 1, 4 );

   if( num <= 2 )
      cmdf( ch, "bash %s", fighting->name );
   else if( num == 3 )
   {
      if( ch->get_eq( WEAR_WIELD ) && fighting->get_eq( WEAR_WIELD ) )
         cmdf( ch, "disarm %s", fighting->name );
      else
         cmdf( ch, "kick %s", fighting->name );
   }
   else
      cmdf( ch, "kick %s", fighting->name );
}

bool FighterMove( char_data * ch )
{
   char_data *mfriend, *foe;

   if( ch->wait )
      return false;

   if( !( foe = ch->who_fighting(  ) ) )
      return false;

   if( !( mfriend = foe->who_fighting(  ) ) )
      return false;

   if( mfriend->race == ch->race && mfriend->hit < ch->hit )
      cmdf( ch, "rescue %s", mfriend->name );
   else
      MakeNiftyAttack( ch );

   return true;
}

bool FindABetterWeapon( char_data * ch )
{
   return false;
}

SPECF( spec_warrior )
{
   if( !ch->IS_AWAKE(  ) )
      return false;

   if( ch->who_fighting(  ) )
   {
      if( StandUp( ch ) )
         return true;

      if( FighterMove( ch ) )
         return true;

      if( FindABetterWeapon( ch ) )
         return true;
   }
   return false;
}

SPECF( spec_RustMonster )
{
   if( !ch->IS_AWAKE(  ) )
      return false;

   list < obj_data * >::iterator iobj;
   for( iobj = ch->in_room->objects.begin(  ); iobj != ch->in_room->objects.end(  ); )
   {
      obj_data *eat = *iobj;
      ++iobj;

      if( !eat->wear_flags.test( ITEM_TAKE ) || eat->extra_flags.test( ITEM_BURIED ) || eat->item_type == ITEM_CORPSE_PC )
         continue;

      if( ch->has_actflag( ACT_ONMAP ) )
      {
         if( !is_same_obj_map( ch, eat ) )
            continue;
      }

      act( AT_ACTION, "$n picks up $p and swallows it.", ch, eat, NULL, TO_ROOM );

      if( eat->item_type == ITEM_CORPSE_NPC )
      {
         act( AT_ACTION, "$n savagely devours a corpse.", ch, NULL, NULL, TO_ROOM );

         list < obj_data * >::iterator iobj2;
         for( iobj2 = eat->contents.begin(  ); iobj2 != eat->contents.end(  ); )
         {
            obj_data *obj = *iobj2;
            ++iobj2;

            obj->from_obj(  );
            obj->to_room( ch->in_room, ch );
         }
      }
      eat->separate(  );
      eat->from_room(  );
      eat->extract(  );
      return true;
   }
   return false;
}

/*****
 * A wanderer (nomad) that arm's itself and discards what it doesnt.. Kinda fun
 * to watch. I wrote this for an area that repops alot of armor on the ground
 * Adds a little spice to the area. Basically a modified janitor. Started off as
 * an easy project. Thanks to Mark Zagorski <mwz0615@ksu.edu> for pointing out I 
 * had a reduntancy if I had two mobs that in rooms that only led to each other
 * they would throw the piece of armor back and forth. He also suggested 
 * damaging the armor when thrown. [Imorted from Smaug 1.8]
 *****/
SPECF( spec_wanderer )
{
   obj_data *trash;
   room_index *was_in_room;
   exit_data *pexit = NULL;
   int door, schance = 50;
   bool found = false;  /* Valid direction */
   bool thrown = false; /* Whether to be thrown or not */
   bool noexit = true;  /* Assume there is no valid exits */

   if( !ch->IS_AWAKE(  ) )
      return false;

   was_in_room = ch->in_room;
   if( ch->in_room->exits.size(  ) > 0 )
   {
      pexit = ( *ch->in_room->exits.begin(  ) );
      noexit = false;
   }

   if( schance > number_percent(  ) )
   {
      /****
       * Look for objects on the ground and pick it up
       ****/
      list < obj_data * >::iterator iobj;
      for( iobj = ch->in_room->objects.begin(  ); iobj != ch->in_room->objects.end(  ); )
      {
         trash = *iobj;
         ++iobj;

         if( !trash->wear_flags.test( ITEM_TAKE ) || trash->extra_flags.test( ITEM_BURIED ) )
            continue;

         if( trash->item_type == ITEM_WEAPON || trash->item_type == ITEM_ARMOR || trash->item_type == ITEM_LIGHT )
         {
            trash->separate(  ); // So there is no 'sword <6>' gets only one object off ground
            act( AT_ACTION, "$n leans over and gets $p.", ch, trash, NULL, TO_ROOM );
            trash->from_room(  );
            trash->to_char( ch );

            /*****
             * If object is too high a level throw it away.
             *****/
            if( ch->level < trash->level )
            {
               act( AT_ACTION, "$n tries to use $p, but is too inexperienced.", ch, trash, NULL, TO_ROOM );
               thrown = true;
            }

            /*****
             * Wear the object if it is not to be thrown. The FALSE is passed
             * so that the mob wont remove a piece of armor already there
             * if it is not worn it is assumed that they can't use it or 
             * they already are wearing something.
             *****/
            if( !thrown )
               wear_obj( ch, trash, false, -1 );

            /*****
             * Look for an object in the inventory that is not being worn
             * then throw it away...
             *****/
            found = false;
            if( !thrown )
            {
               list < obj_data * >::iterator iobj2;
               for( iobj2 = ch->carrying.begin(  ); iobj2 != ch->carrying.end(  ); ++iobj2 )
               {
                  obj_data *obj2 = *iobj2;
                  if( obj2->wear_loc == WEAR_NONE )
                  {
                     interpret( ch, "say Hmm, I can't use this." );
                     trash = obj2;
                     thrown = true;
                  }
               }
            }
         }

         /*****
          * Ugly bit of code..
          * Checks if the object is to be thrown & there is a valid exit, 
          * randomly pick a direction to throw it, and check to make sure no other
          * spec_wanderer mobs are in that room.
          *****/
         if( thrown && !noexit )
         {
            while( !found && !noexit )
            {
               door = number_door(  );

               if( ( pexit = ch->in_room->get_exit( door ) ) != NULL && pexit->to_room && !IS_EXIT_FLAG( pexit, EX_CLOSED ) && !pexit->to_room->flags.test( ROOM_NODROP ) )
               {
                  list < char_data * >::iterator ich;
                  for( ich = pexit->to_room->people.begin(  ); ich != pexit->to_room->people.end(  ); ++ich )
                  {
                     char_data *vch = *ich;
                     if( vch->spec_fun == spec_wanderer )
                     {
                        noexit = true;
                        return false;
                     }
                  }
                  found = true;
               }
            }

            if( !noexit && thrown )
            {
               damage_obj( trash );
               if( !trash->extracted(  ) )
               {
                  trash->separate(  );
                  act( AT_ACTION, "$n growls and throws $p $T.", ch, trash, dir_name[pexit->vdir], TO_ROOM );
                  trash->from_char(  );
                  trash->to_room( pexit->to_room, ch );
                  ch->from_room(  );
                  if( !ch->to_room( pexit->to_room ) )
                     log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __func__, __LINE__ );
                  act( AT_CYAN, "$p thrown by $n lands in the room.", ch, trash, ch, TO_ROOM );
                  ch->from_room(  );
                  if( !ch->to_room( was_in_room ) )
                     log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __func__, __LINE__ );
               }
               else
               {
                  interpret( ch, "say This thing is junk!" );
                  act( AT_ACTION, "$n growls and breaks $p.", ch, trash, NULL, TO_ROOM );
               }
               return true;
            }
            return true;
         }
      }  /* get next obj */
      return false;  /* No objects :< */
   }
   return false;
}

bool cast_ranger( char_data * ch )
{
   char_data *victim = NULL;
   string spell;
   int sn;

   if( !ch->IS_AWAKE(  ) )
      return false;

   if( ch->position < POS_AGGRESSIVE || ch->position > POS_MOUNTED )
      return false;

   victim = spec_find_victim( ch, true );

   if( !victim || victim == ch )
      return false;

   switch ( number_bits( 3 ) )
   {
      case 0:
         spell = "faerie fire";
         break;
      case 1:
         spell = "entangle";
         break;
      case 2:
         spell = "cause light";
         break;
      case 3:
         spell = "faerie fog";
         break;
      case 4:
         spell = "cause serious";
         break;
      case 5:
         spell = "earthquake";
         break;
      case 6:
         spell = "poison";
         break;
      case 7:
         spell = "cause critical";
         break;
      default:
         spell = "cause light";
         break;
   }

   if( ( sn = skill_lookup( spell ) ) < 0 )
      return false;

   if( ch->level < skill_table[sn]->skill_level[ch->Class] )
      return false;

   cmdf( ch, "cast %s %s", skill_table[sn]->name, victim->name );
   return true;
}

SPECF( spec_ranger )
{
   if( !ch->IS_AWAKE(  ) )
      return false;

   if( ch->who_fighting(  ) )
   {
      switch ( number_range( 1, 2 ) )
      {
         default:
         case 1:
         {
            if( StandUp( ch ) )
               return true;

            if( FighterMove( ch ) )
               return true;

            if( FindABetterWeapon( ch ) )
               return true;
         }
         case 2:
            if( cast_ranger( ch ) )
               return true;
      }
   }
   return false;
}

bool cast_paladin( char_data * ch )
{
   char_data *victim = NULL;
   string spell;
   int sn;

   if( !ch->IS_AWAKE(  ) )
      return false;

   if( ch->position < POS_AGGRESSIVE || ch->position > POS_MOUNTED )
      return false;

   victim = spec_find_victim( ch, true );

   if( !victim || victim == ch )
      return false;

   switch ( number_bits( 3 ) )
   {
      case 0:
         spell = "spiritual hammer";
         break;
      case 1:
         spell = "dispel evil";
         break;
      default:
         spell = "spiritual hammer";
         break;
   }

   if( ( sn = skill_lookup( spell ) ) < 0 )
      return false;

   if( ch->level < skill_table[sn]->skill_level[ch->Class] )
      return false;

   cmdf( ch, "cast %s %s", skill_table[sn]->name, victim->name );
   return true;
}

SPECF( spec_paladin )
{
   if( !ch->IS_AWAKE(  ) )
      return false;

   if( ch->who_fighting(  ) )
   {
      switch ( number_range( 1, 3 ) )
      {
         default:
         case 1:
         case 2:
         {
            if( StandUp( ch ) )
               return true;

            if( FighterMove( ch ) )
               return true;

            if( FindABetterWeapon( ch ) )
               return true;
         }
         case 3:
            if( cast_paladin( ch ) )
               return true;
      }
   }
   return false;
}

SPECF( spec_druid )
{
   char_data *victim = NULL;
   string spell;
   int sn;

   if( !ch->IS_AWAKE(  ) )
      return false;

   if( ch->position < POS_AGGRESSIVE || ch->position > POS_MOUNTED )
      return false;

   victim = spec_find_victim( ch, true );

   if( !victim || victim == ch )
      return false;

   switch ( number_bits( 5 ) )
   {
      case 0:
         spell = "cause light";
         break;
      case 1:
         spell = "faerie fire";
         break;
      case 2:
         spell = "cause serious";
         break;
      case 3:
         spell = "entangle";
         break;
      case 4:
         spell = "gust of wind";
         break;
      case 5:
         spell = "earthquake";
         break;
      case 6:
         spell = "harm";
         break;
      case 7:
         spell = "cause critical";
         break;
      case 8:
         spell = "dispel magic";
         break;
      case 9:
         spell = "flamestrike";
         break;
      case 10:
         spell = "call lightning";
         break;
      case 11:
         spell = "firestorm";
         break;
      default:
         spell = "cause light";
         break;
   }

   if( ( sn = skill_lookup( spell ) ) < 0 )
      return false;

   if( ch->level < skill_table[sn]->skill_level[ch->Class] )
      return false;

   cmdf( ch, "cast %s %s", skill_table[sn]->name, victim->name );
   return true;
}

bool cast_antipaladin( char_data * ch )
{
   char_data *victim = NULL;
   string spell;
   int sn;

   if( !ch->IS_AWAKE(  ) )
      return false;

   if( ch->position < POS_AGGRESSIVE || ch->position > POS_MOUNTED )
      return false;

   victim = spec_find_victim( ch, true );

   if( !victim || victim == ch )
      return false;

   switch ( number_bits( 3 ) )
   {
      case 0:
         spell = "curse";
         break;
      case 1:
         spell = "chill touch";
         break;
      case 2:
         spell = "black hand";
         break;
      case 3:
         spell = "withering hand";
         break;
      default:
         spell = "curse";
         break;
   }

   if( ( sn = skill_lookup( spell ) ) < 0 )
      return false;

   if( ch->level < skill_table[sn]->skill_level[ch->Class] )
      return false;

   cmdf( ch, "cast %s %s", skill_table[sn]->name, victim->name );
   return true;
}

SPECF( spec_antipaladin )
{
   if( !ch->IS_AWAKE(  ) )
      return false;

   if( ch->who_fighting(  ) )
   {
      switch ( number_range( 1, 3 ) )
      {
         default:
         case 1:
         {
            if( StandUp( ch ) )
               return true;

            if( FighterMove( ch ) )
               return true;

            if( FindABetterWeapon( ch ) )
               return true;
         }
         case 2:
         case 3:
            if( cast_antipaladin( ch ) )
               return true;
      }
   }
   return false;
}

SPECF( spec_bard )
{
   char_data *victim = NULL;
   string spell;
   int sn;

   if( !ch->IS_AWAKE(  ) )
      return false;

   if( ch->position < POS_AGGRESSIVE || ch->position > POS_MOUNTED )
      return false;

   victim = spec_find_victim( ch, true );

   if( !victim || victim == ch )
      return false;

   switch ( number_bits( 3 ) )
   {
      case 0:
         spell = "magic missile";
         break;
      case 1:
         spell = "shocking grasp";
         break;
      case 2:
         spell = "burning hands";
         break;
      case 3:
         spell = "despair";
         break;
      case 4:
         spell = "acid blast";
         break;
      case 5:
         spell = "sonic resonance";
         break;
      case 6:
         spell = "blindness";
         break;
      case 7:
         spell = "colour spray";
         break;
      case 8:
         spell = "lightning bolt";
         break;
      case 9:
         spell = "cacophony";
         break;
      case 10:
         spell = "disintegrate";
         break;
      default:
         spell = "magic missile";
         break;
   }

   if( ( sn = skill_lookup( spell ) ) < 0 )
      return false;

   if( ch->level < skill_table[sn]->skill_level[ch->Class] )
      return false;

   cmdf( ch, "cast %s %s", skill_table[sn]->name, victim->name );
   return true;
}
