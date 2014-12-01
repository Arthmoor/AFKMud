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
 *                          Regular update module                           *
 ****************************************************************************/

#include <sys/time.h>
#include "mud.h"
#include "area.h"
#include "clans.h"
#include "deity.h"
#include "descriptor.h"
#include "mobindex.h"
#include "mud_prog.h"
#include "new_auth.h"
#include "objindex.h"
#include "polymorph.h"
#include "raceclass.h"
#include "roomindex.h"
#include "variables.h"

/*
 * Global Variables
 */
char_data *timechar;
extern char lastplayercmd[MIL * 2];
static bool update_month_trigger;
extern bool bootlock;

int IsRideable( char_data * );
void unbind_follower( char_data *, char_data * );
void save_timedata(  );
void calc_season(  );   /* Samson - See calendar.c */
void room_act_update(  );
void obj_act_update(  );
void mpsleep_update(  );
bool is_fearing( char_data *, char_data * );
void raw_kill( char_data *, char_data * );
bool is_hating( char_data *, char_data * );
void check_attacker( char_data *, char_data * );
int get_terrain( short, short, short );
bool map_wander( char_data *, short, short, short, short );
void clean_auctions(  );
void set_supermob( obj_data * );
bool check_social( char_data *, const string &, const string & );
void auth_update(  );
void environment_update(  );
bool will_fall( char_data *, int );
void make_corpse( char_data *, char_data * );
void hunt_vic( char_data * );
void clean_char_queue(  );
void clean_obj_queue(  );
ch_ret pullcheck( char_data *, int );
void teleport( char_data *, int, int );
void found_prey( char_data *, char_data * );
#if defined(WIN32)
void gettimeofday( struct timeval *, struct timezone * );
#endif

const char *corpse_descs[] = {
   "A skeleton of %s lies here in a pile.",
   "The corpse of %s is in the last stages of decay.",
   "The corpse of %s is crawling with vermin.",
   "The corpse of %s fills the air with a foul stench.",
   "The corpse of %s is buzzing with flies.",
   "The corpse of %s lies here."
};

/*
 * Regeneration stuff.
 */
/* When age < 15 return the value p0 */
/* When age in 15..29 calculate the line between p1 & p2 */
/* When age in 30..44 calculate the line between p2 & p3 */
/* When age in 45..59 calculate the line between p3 & p4 */
/* When age in 60..79 calculate the line between p4 & p5 */
/* When age >= 80 return the value p6 */
int graf( int age, int p0, int p1, int p2, int p3, int p4, int p5, int p6 )
{
   if( age < 15 )
      return p0;  // < 15
   else if( age <= 29 )
      return ( p1 + ( ( ( age - 15 ) * ( p2 - p1 ) ) / 15 ) );   /* 15..29 */
   else if( age <= 44 )
      return ( p2 + ( ( ( age - 30 ) * ( p3 - p2 ) ) / 15 ) );   /* 30..44 */
   else if( age <= 59 )
      return ( p3 + ( ( ( age - 45 ) * ( p4 - p3 ) ) / 15 ) );   /* 45..59 */
   else if( age <= 79 )
      return ( p4 + ( ( ( age - 60 ) * ( p5 - p4 ) ) / 20 ) );   /* 60..79 */
   else
      return p6;  // >= 80
}

/* manapoint gain pr. game hour */
int mana_gain( char_data * ch )
{
   int gain = 0;

   if( ch->isnpc(  ) )
      gain = 8;
   else
      gain = graf( ch->get_age(  ), 3, 9, 12, 16, 20, 16, 2 );

   switch ( ch->position )
   {
      default:
         break;
      case POS_SLEEPING:
         gain += gain;
         break;
      case POS_RESTING:
         gain += ( gain / 2 );
         break;
      case POS_SITTING:
         gain += ( gain / 4 );
         break;
   }

   gain += ch->mana_regen;
   gain += gain;
   gain += wis_app[ch->get_curr_wis(  )].practice * 2;

   if( ch->has_aflag( AFF_POISON ) )
      gain = gain / 4;

   if( !ch->isnpc(  ) )
      if( ch->pcdata->condition[COND_FULL] == 0 || ch->pcdata->condition[COND_THIRST] == 0 )
         gain = gain / 4;

   if( !ch->isnpc(  ) )
   {
      if( ch->pcdata->condition[COND_DRUNK] > 10 )
         gain += ( gain / 2 );
      else if( ch->pcdata->condition[COND_DRUNK] > 0 )
         gain += ( gain / 4 );
   }

   if( ch->Class != CLASS_MAGE && ch->Class != CLASS_NECROMANCER && ch->Class != CLASS_CLERIC && ch->Class != CLASS_DRUID )
      gain -= 2;

   return gain;
}

int hit_gain( char_data * ch )
{
   int gain = 0, dam = 0;

   if( ch->isnpc(  ) )
      gain = 8;
   else
   {
      if( ch->position > POS_SITTING && ch->position < POS_STANDING )
         gain = 0;
      else
         gain = graf( ch->get_age(  ), 17, 20, 23, 26, 23, 18, 12 );
   }

   switch ( ch->position )
   {
      default:
         break;
      case POS_SLEEPING:
         gain += 15;
         break;
      case POS_RESTING:
         gain += 10;
         break;
      case POS_SITTING:
         gain += 5;
         break;
   }

   gain += con_app[ch->get_curr_con(  )].hitp / 2;

   if( ch->has_aflag( AFF_POISON ) )
   {
      gain = 0;
      dam = number_range( 10, 32 );
      if( ch->race == RACE_HALFLING )
         dam = number_range( 1, 20 );
      damage( ch, ch, dam, gsn_poison );
      return 0;
   }

   gain += ch->hit_regen;

   if( !ch->isnpc(  ) )
   {
      if( ch->pcdata->condition[COND_FULL] == 0 || ch->pcdata->condition[COND_THIRST] == 0 )
         gain = gain / 8;
      if( ch->pcdata->condition[COND_DRUNK] > 10 )
         gain += ( gain / 2 );
      else if( ch->pcdata->condition[COND_DRUNK] > 0 )
         gain += ( gain / 4 );
   }

   if( ch->Class != CLASS_WARRIOR && ch->Class != CLASS_PALADIN && ch->Class != CLASS_ANTIPALADIN && ch->Class != CLASS_RANGER )
   {
      gain -= 2;
      if( gain < 0 && !ch->fighting )
         damage( ch, ch, gain * -1, skill_lookup( "unknown" ) );
   }
   return gain;
}

int move_gain( char_data * ch )
/* move gain pr. game hour */
{
   int gain = 0;

   if( ch->isnpc(  ) )
   {
      gain = 22;
      if( IsRideable( ch ) )
         gain += ( gain / 2 );
      /*
       * Neat and fast 
       */
   }
   else
   {
      if( ch->position < POS_BERSERK || ch->position > POS_EVASIVE )
         gain = graf( ch->get_age(  ), 15, 21, 25, 28, 20, 10, 3 );
      else
         gain = 0;
   }

   switch ( ch->position )
   {
      default:
         break;
      case POS_SLEEPING:
         gain += ( gain / 4 );
         break;
      case POS_RESTING:
         gain += ( gain / 3 );
         break;
      case POS_SITTING:
         gain += ( gain / 16 );
         break;
   }

   gain += ch->move_regen;

   if( ch->has_aflag( AFF_POISON ) )
      gain = gain / 8;

   if( !ch->isnpc(  ) )
      if( ch->pcdata->condition[COND_FULL] == 0 || ch->pcdata->condition[COND_THIRST] == 0 )
         gain = gain / 4;

   if( ch->Class != CLASS_ROGUE && ch->Class != CLASS_BARD && ch->Class != CLASS_MONK )
      gain -= 2;

   return gain;
}

void gain_condition( char_data * ch, int iCond, int value )
{
   int condition;
   ch_ret retcode = rNONE;

   if( value == 0 || ch->isnpc(  ) || ch->is_immortal(  ) )
      return;

   /*
    * Ghosts are immune to all these 
    */
   if( ch->has_pcflag( PCFLAG_GHOST ) )
      return;

   condition = ch->pcdata->condition[iCond];

   /*
    * Immune to hunger/thirst 
    */
   if( condition == -1 )
      return;

   ch->pcdata->condition[iCond] = URANGE( 0, condition + value, sysdata->maxcondval );

   if( ch->pcdata->condition[iCond] == 0 )
   {
      switch ( iCond )
      {
         case COND_FULL:
            ch->print( "&[hungry]You are STARVING!\r\n" );
            act( AT_HUNGRY, "$n is starved half to death!", ch, nullptr, nullptr, TO_ROOM );
            if( number_bits( 1 ) == 0 )
               ch->worsen_mental_state( 1 );
            break;

         case COND_THIRST:
            ch->print( "&[thirsty]You are DYING of THIRST!\r\n" );
            act( AT_THIRSTY, "$n is dying of thirst!", ch, nullptr, nullptr, TO_ROOM );
            if( number_bits( 1 ) == 0 )
               ch->worsen_mental_state( 1 );
            break;

         case COND_DRUNK:
            if( condition != 0 )
               ch->print( "&[sober]You are sober.\r\n" );
            retcode = rNONE;
            break;

         default:
            bug( "%s: invalid condition type %d", __func__, iCond );
            retcode = rNONE;
            break;
      }
   }

   if( retcode != rNONE )
      return;

   if( ch->pcdata->condition[iCond] == 1 )
   {
      switch ( iCond )
      {
         default:
            break;

         case COND_FULL:
            ch->print( "&[hungry]You are really hungry.\r\n" );
            act( AT_HUNGRY, "You can hear $n's stomach growling.", ch, nullptr, nullptr, TO_ROOM );
            if( number_bits( 1 ) == 0 )
               ch->worsen_mental_state( 1 );
            break;

         case COND_THIRST:
            ch->print( "&[thirsty]You are really thirsty.\r\n" );
            act( AT_THIRSTY, "$n looks a little parched.", ch, nullptr, nullptr, TO_ROOM );
            if( number_bits( 1 ) == 0 )
               ch->worsen_mental_state( 1 );
            break;

         case COND_DRUNK:
            if( condition != 0 )
               ch->print( "&[sober]You are feeling a little less light headed.\r\n" );
            break;
      }
   }

   if( ch->pcdata->condition[iCond] == 2 )
   {
      switch ( iCond )
      {
         default:
            break;

         case COND_FULL:
            ch->print( "&[hungry]You are hungry.\r\n" );
            break;

         case COND_THIRST:
            ch->print( "&[thirsty]You are thirsty.\r\n" );
            break;
      }
   }

   if( ch->pcdata->condition[iCond] == 3 )
   {
      switch ( iCond )
      {
         default:
            break;

         case COND_FULL:
            ch->print( "&[hungry]You are a mite peckish.\r\n" );
            break;

         case COND_THIRST:
            ch->print( "&[thirsty]You could use a sip of something refreshing.\r\n" );
            break;
      }
   }
}

/*
 * drunk randoms	- Tricops
 * (Made part of mobile_update	-Thoric)
 */
void drunk_randoms( char_data * ch )
{
   short drunk, position;

   if( ch->isnpc(  ) || ch->pcdata->condition[COND_DRUNK] <= 0 )
      return;

   if( number_percent(  ) < 30 )
      return;

   drunk = ch->pcdata->condition[COND_DRUNK];
   position = ch->position;
   ch->position = POS_STANDING;

   if( number_percent(  ) < ( 2 * drunk / 20 ) )
      check_social( ch, "burp", "" );
   else if( number_percent(  ) < ( 2 * drunk / 20 ) )
      check_social( ch, "hiccup", "" );
   else if( number_percent(  ) < ( 2 * drunk / 20 ) )
      check_social( ch, "drool", "" );
   else if( number_percent(  ) < ( 2 * drunk / 20 ) )
      check_social( ch, "fart", "" );
   else if( drunk > ( 10 + ( ch->get_curr_con(  ) / 5 ) ) && number_percent(  ) < ( 2 * drunk / 18 ) )
   {
      list < char_data * >::iterator ich;
      char_data *rvch = nullptr;

      for( ich = ch->in_room->people.begin(  ); ich != ch->in_room->people.end(  ); ++ich )
         if( number_percent(  ) < 10 )
            rvch = *ich;
      if( rvch )
         cmdf( ch, "puke %s", rvch->name );
      else
         interpret( ch, "puke" );
   }
   ch->position = position;
}

/*
 * Random hallucinations for those suffering from an overly high mentalstate
 * (Hats off to Albert Hoffman's "problem child")	-Thoric
 */
void hallucinations( char_data * ch )
{
   if( ch->mental_state >= 30 && number_bits( 5 - ( ch->mental_state >= 50 ) - ( ch->mental_state >= 75 ) ) == 0 )
   {
      const char *t;

      switch ( number_range( 1, UMIN( 21, ( ch->mental_state + 5 ) / 5 ) ) )
      {
         default:
         case 1:
            t = "You feel very restless... you can't sit still.\r\n";
            break;
         case 2:
            t = "You're tingling all over.\r\n";
            break;
         case 3:
            t = "Your skin is crawling.\r\n";
            break;
         case 4:
            t = "You suddenly feel that something is terribly wrong.\r\n";
            break;
         case 5:
            t = "Those damn little fairies keep laughing at you!\r\n";
            break;
         case 6:
            t = "You can hear your mother crying...\r\n";
            break;
         case 7:
            t = "Have you been here before, or not?  You're not sure...\r\n";
            break;
         case 8:
            t = "Painful childhood memories flash through your mind.\r\n";
            break;
         case 9:
            t = "You hear someone call your name in the distance...\r\n";
            break;
         case 10:
            t = "Your head is pulsating... you can't think straight.\r\n";
            break;
         case 11:
            t = "The ground... seems to be squirming...\r\n";
            break;
         case 12:
            t = "You're not quite sure what is real anymore.\r\n";
            break;
         case 13:
            t = "It's all a dream... or is it?\r\n";
            break;
         case 14:
            t = "You hear your grandchildren praying for you to watch over them.\r\n";
            break;
         case 15:
            t = "They're coming to get you... coming to take you away...\r\n";
            break;
         case 16:
            t = "You begin to feel all powerful!\r\n";
            break;
         case 17:
            t = "You're light as air... the heavens are yours for the taking.\r\n";
            break;
         case 18:
            t = "Your whole life flashes by... and your future...\r\n";
            break;
         case 19:
            t = "You are everywhere and everything... you know all and are all!\r\n";
            break;
         case 20:
            t = "You feel immortal!\r\n";
            break;
         case 21:
            t = "Ahh... the power of a Supreme Entity... what to do...\r\n";
            break;
      }
      ch->print( t );
   }
}

void affect_update( char_data * ch )
{
   list < affect_data * >::iterator paf;
   skill_type *skill;

   for( paf = ch->affects.begin(  ); paf != ch->affects.end(  ); )
   {
      affect_data *aff = *paf;
      ++paf;

      if( aff->duration > 0 )
         --aff->duration;
      else if( aff->duration < 0 )
         ;
      else
      {
         skill = get_skilltype( aff->type );
         if( aff->type > 0 && skill && skill->msg_off )
            ch->printf( "&[wearoff]%s\r\n", skill->msg_off );
         ch->affect_remove( aff );
      }
   }
}

/*
 * Mob autonomous action.
 * This function takes 25% to 35% of ALL Mud cpu time.
 */
void mobile_update( void )
{
   exit_data *pexit;
   int door;

   ch_ret retcode = rNONE;

   /*
    * Examine all mobs.
    */
   list < char_data * >::iterator ich;
   for( ich = charlist.begin(  ); ich != charlist.end(  ); )
   {
      char_data *ch = *ich;
      ++ich;

      if( ch->char_died(  ) )
         continue;

      if( !ch->in_room )
      {
         log_printf( "%s: ch in nullptr room - attempting limbo transfer", __func__ );
         if( !ch->to_room( get_room_index( ROOM_VNUM_LIMBO ) ) )
            log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __func__, __LINE__ );
         continue;
      }

      /*
       * NPCs belonging to prototype areas should be doing nothing 
       */
      if( ch->isnpc(  ) && ch->pIndexData->area->flags.test( AFLAG_PROTOTYPE ) )
         continue;

      /*
       * Moved here to remove spell_update() which was quite useless. 
       */
      affect_update( ch );
      if( ch->char_died(  ) )
         continue;

      if( !ch->isnpc(  ) )
      {
         if( !ch->has_pcflag( PCFLAG_GHOST ) )
         {
            drunk_randoms( ch );
            hallucinations( ch );
         }
         continue;
      }

      /*
       * Everything below this point should be happening only to NPCs.
       *
       * Removed || ch->has_aflag( AFF_CHARM ) from the line below, so that pets with progs would be nice.
       * * Tarl 5 May 2002
       */
      if( ch->has_aflag( AFF_PARALYSIS ) )
         continue;

      /*
       * Clean up 'animated corpses' that are not charmed' - Scryn 
       * Also clean up woodland call mobs and summoned warmounts etc - Samson 5-3-00 
       */
      switch ( ch->pIndexData->vnum )
      {
         default:
            break;

         case MOB_VNUM_ANIMATED_CORPSE:
         case MOB_VNUM_ANIMATED_SKELETON:
         case MOB_VNUM_ANIMATED_ZOMBIE:
         case MOB_VNUM_ANIMATED_GHOUL:
         case MOB_VNUM_ANIMATED_CRYPT_THING:
         case MOB_VNUM_ANIMATED_MUMMY:
         case MOB_VNUM_ANIMATED_GHOST:
         case MOB_VNUM_ANIMATED_DEATH_KNIGHT:
         case MOB_VNUM_ANIMATED_DRACOLICH:
            if( !ch->has_aflag( AFF_CHARM ) )
            {
               if( !ch->in_room->people.empty(  ) )
                  act( AT_MAGIC, "$n returns to the dust from whence $e came.", ch, nullptr, nullptr, TO_ROOM );
               if( ch->isnpc(  ) )  /* Guard against purging switched? */
                  ch->extract( true );
               continue;
            }

         case MOB_VNUM_WOODCALL1:
         case MOB_VNUM_WOODCALL2:
         case MOB_VNUM_WOODCALL3:
         case MOB_VNUM_WOODCALL4:
         case MOB_VNUM_WOODCALL5:
         case MOB_VNUM_WOODCALL6:
            if( !ch->has_aflag( AFF_CHARM ) )
            {
               if( !ch->in_room->people.empty(  ) )
                  act( AT_MAGIC, "$n dashes back into the brush.", ch, nullptr, nullptr, TO_ROOM );
               if( ch->isnpc(  ) )
                  ch->extract( true );
               continue;
            }

         case MOB_VNUM_WARMOUNT:
         case MOB_VNUM_WARMOUNTTWO:
         case MOB_VNUM_WARMOUNTTHREE:
         case MOB_VNUM_WARMOUNTFOUR:
            if( !ch->has_aflag( AFF_CHARM ) )
            {
               if( ch->master && ch->master->mount == ch )
               {
                  ch->master->position = POS_SITTING;
                  ch->master->printf( "%s bucks you off, and then gallops away into the wilds!\r\n", ch->short_descr );
               }
               else
               {
                  if( !ch->in_room->people.empty(  ) )
                     act( AT_MAGIC, "$n suddenly bolts and gallops away.", ch, nullptr, nullptr, TO_ROOM );
               }
               if( ch->isnpc(  ) )
                  ch->extract( true );
               continue;
            }

         case MOB_VNUM_GATE:
            if( !ch->has_aflag( AFF_CHARM ) )
            {
               if( !ch->in_room->people.empty(  ) )
                  act( AT_MAGIC, "The magic binding $n to this plane dissipates and $e vanishes.", ch, nullptr, nullptr, TO_ROOM );
               if( ch->isnpc(  ) )
                  ch->extract( true );
               continue;
            }
      }

      if( ch->timer > 0 )
      {
         if( --ch->timer == 0 )
         {
            /*
             * Don't litter the overland with skyship corpses. It's tacky. Samson 4-11-04 
             */
            if( ch->pIndexData->vnum != MOB_VNUM_SKYSHIP )
               make_corpse( ch, ch );
            ch->extract( true );
            continue;
         }
      }

      // If this doesn't work, then... uh... find someplace new to put it?
      will_fall( ch, 0 );

      if( ch->char_died(  ) )
         continue;

      if( ch->has_actflag( ACT_PET ) && !ch->has_aflag( AFF_CHARM ) && ch->master )
         unbind_follower( ch, ch->master );

      if( !ch->has_actflag( ACT_SENTINEL ) && !ch->fighting && ch->hunting )
      {
         ch->WAIT_STATE( 2 * sysdata->pulseviolence );
         hunt_vic( ch );
         continue;
      }

      /*
       * Examine call for special procedure 
       */
      if( ch->spec_fun )
      {
         if( ( *ch->spec_fun ) ( ch ) )
            continue;
         if( ch->char_died(  ) )
            continue;
      }

      /*
       * Check for mudprogram script on mob 
       */
      if( HAS_PROG( ch->pIndexData, SCRIPT_PROG ) && !ch->has_actflag( ACT_STOP_SCRIPT ) )
      {
         mprog_script_trigger( ch );
         continue;
      }

      /*
       * That's all for sleeping / busy monster 
       */
      if( ch->position != POS_STANDING )
         continue;

      if( ch->has_actflag( ACT_MOUNTED ) )
      {
         if( ch->has_actflag( ACT_AGGRESSIVE ) || ch->has_actflag( ACT_META_AGGR ) )
            interpret( ch, "emote makes threatening gestures." );
         continue;
      }

      if( !ch->in_room->area )
      {
         log_printf( "Room %d for mob %s is not associated with an area?", ch->in_room->vnum, ch->name );
         if( ch->was_in_room )
            log_printf( "Was in room %d", ch->was_in_room->vnum );
         ch->extract( true );
         continue;
      }

      /*
       * MOBprogram random trigger 
       */
      if( ch->in_room->area->nplayer > 0 )
      {
         mprog_random_trigger( ch );
         if( ch->char_died(  ) )
            continue;
         if( ch->position < POS_STANDING )
            continue;
      }

      /*
       * MOBprogram hour trigger: do something for an hour 
       */
      mprog_hour_trigger( ch );
      if( ch->char_died(  ) )
         continue;

      rprog_hour_trigger( ch );
      if( ch->char_died(  ) )
         continue;

      if( ch->position < POS_STANDING )
         continue;

      /*
       * Scavenge 
       */
      if( ch->has_actflag( ACT_SCAVENGER ) && !ch->in_room->objects.empty(  ) && number_bits( 2 ) == 0 )
      {
         obj_data *obj_best = nullptr;
         int max = 1;

         list < obj_data * >::iterator iobj;
         for( iobj = ch->in_room->objects.begin(  ); iobj != ch->in_room->objects.end(  ); ++iobj )
         {
            obj_data *obj = *iobj;

            if( obj->extra_flags.test( ITEM_PROTOTYPE ) && !ch->has_actflag( ACT_PROTOTYPE ) )
               continue;
            if( obj->wear_flags.test( ITEM_TAKE ) && obj->cost > max && !obj->extra_flags.test( ITEM_BURIED ) && !obj->extra_flags.test( ITEM_HIDDEN ) )
            {
               obj_best = obj;
               max = obj->cost;
            }
         }

         if( obj_best )
         {
            obj_best->from_room(  );
            obj_best->to_char( ch );
            act( AT_ACTION, "$n gets $p.", ch, obj_best, nullptr, TO_ROOM );
         }
      }

      /*
       * Map wanderers - Samson 7-29-00 
       */
      if( ch->has_actflag( ACT_ONMAP ) )
      {
         short sector = get_terrain( ch->wmap, ch->mx, ch->my );
         short wmap = ch->wmap;
         short x = ch->mx;
         short y = ch->my;
         short dir = number_bits( 5 );

         if( dir < DIR_SOMEWHERE && dir != DIR_UP && dir != DIR_DOWN )
         {
            switch ( dir )
            {
               default:
                  break;
               case DIR_NORTH:
                  if( map_wander( ch, wmap, x, y - 1, sector ) )
                     move_char( ch, nullptr, 0, DIR_NORTH, false );
                  break;
               case DIR_NORTHEAST:
                  if( map_wander( ch, wmap, x + 1, y - 1, sector ) )
                     move_char( ch, nullptr, 0, DIR_NORTHEAST, false );
                  break;
               case DIR_EAST:
                  if( map_wander( ch, wmap, x + 1, y, sector ) )
                     move_char( ch, nullptr, 0, DIR_EAST, false );
                  break;
               case DIR_SOUTHEAST:
                  if( map_wander( ch, wmap, x + 1, y + 1, sector ) )
                     move_char( ch, nullptr, 0, DIR_SOUTHEAST, false );
                  break;
               case DIR_SOUTH:
                  if( map_wander( ch, wmap, x, y + 1, sector ) )
                     move_char( ch, nullptr, 0, DIR_SOUTH, false );
                  break;
               case DIR_SOUTHWEST:
                  if( map_wander( ch, wmap, x - 1, y + 1, sector ) )
                     move_char( ch, nullptr, 0, DIR_SOUTHWEST, false );
                  break;
               case DIR_WEST:
                  if( map_wander( ch, wmap, x - 1, y, sector ) )
                     move_char( ch, nullptr, 0, DIR_WEST, false );
                  break;
               case DIR_NORTHWEST:
                  if( map_wander( ch, wmap, x - 1, y - 1, sector ) )
                     move_char( ch, nullptr, 0, DIR_NORTHWEST, false );
                  break;
            }
         }
         if( ch->char_died(  ) )
            continue;
      }

      /*
       * Wander 
       * Update hunt_vic also if any changes are made here 
       */
      if( !ch->has_actflag( ACT_SENTINEL )
          && !ch->has_actflag( ACT_PROTOTYPE ) && ( door = number_bits( 5 ) ) <= 9 && ( pexit = ch->in_room->get_exit( door ) ) != nullptr && pexit->to_room
          /*
           * && !IS_EXIT_FLAG( pexit, EX_CLOSED ) - Test to see if mobs will open doors like this. 
           */
          && !IS_EXIT_FLAG( pexit, EX_WINDOW ) && !IS_EXIT_FLAG( pexit, EX_NOMOB )
          /*
           * Keep em from wandering through my walls, Marcus 
           */
          && !IS_EXIT_FLAG( pexit, EX_FORTIFIED )
          && !IS_EXIT_FLAG( pexit, EX_HEAVY )
          && !IS_EXIT_FLAG( pexit, EX_MEDIUM )
          && !IS_EXIT_FLAG( pexit, EX_LIGHT )
          && !IS_EXIT_FLAG( pexit, EX_CRUMBLING )
          && !pexit->to_room->flags.test( ROOM_NO_MOB )
          && !pexit->to_room->flags.test( ROOM_DEATH ) && ( !ch->has_actflag( ACT_STAY_AREA ) || pexit->to_room->area == ch->in_room->area ) )
      {
         if( pexit->to_room->sector_type == SECT_WATER_NOSWIM && !ch->has_aflag( AFF_AQUA_BREATH ) )
            continue;

         if( pexit->to_room->sector_type == SECT_RIVER && !ch->has_aflag( AFF_AQUA_BREATH ) )
            continue;

         // Is it closed? If the mob doesn't have passdoor, OR the exit is passdoor-proof, have them try and open it first.
         if( IS_EXIT_FLAG( pexit, EX_CLOSED ) && ( !ch->has_aflag( AFF_PASS_DOOR ) || IS_EXIT_FLAG( pexit, EX_NOPASSDOOR ) ) )
            cmdf( ch, "open %s", pexit->keyword );

         // Is it STILL closed? Is it marked no passdoor? Bail out.
         if( IS_EXIT_FLAG( pexit, EX_CLOSED ) && IS_EXIT_FLAG( pexit, EX_NOPASSDOOR ) )
            continue;

         // Yes, I know, this is probably not very efficient, but... if the mob does not have passdoor and it's still closed, then we're done here.
         if( IS_EXIT_FLAG( pexit, EX_CLOSED ) && !ch->has_aflag( AFF_PASS_DOOR ) )
            continue;

         retcode = move_char( ch, pexit, 0, pexit->vdir, false );

         /*
          * If ch changes position due to it's or some other mob's movement via MOBProgs, continue - Kahn 
          */
         if( ch->char_died(  ) )
            continue;

         if( retcode != rNONE || ch->has_actflag( ACT_SENTINEL ) || ch->position < POS_STANDING )
            continue;
      }

      /*
       * Flee 
       */
      if( ch->hit < ch->max_hit / 2
          && ( door = number_bits( 4 ) ) <= 9
          && ( pexit = ch->in_room->get_exit( door ) ) != nullptr
          && pexit->to_room && !IS_EXIT_FLAG( pexit, EX_CLOSED ) && !IS_EXIT_FLAG( pexit, EX_NOMOB ) && !IS_EXIT_FLAG( pexit, EX_WINDOW )
          /*
           * Keep em from wandering through my walls, Marcus 
           */
          && !IS_EXIT_FLAG( pexit, EX_FORTIFIED )
          && !IS_EXIT_FLAG( pexit, EX_HEAVY )
          && !IS_EXIT_FLAG( pexit, EX_MEDIUM )
          && !IS_EXIT_FLAG( pexit, EX_LIGHT )
          && !IS_EXIT_FLAG( pexit, EX_CRUMBLING ) && !pexit->to_room->flags.test( ROOM_NO_MOB ) && !pexit->to_room->flags.test( ROOM_DEATH ) )
      {
         if( pexit->to_room->sector_type == SECT_WATER_NOSWIM && !ch->has_aflag( AFF_AQUA_BREATH ) )
            continue;

         if( pexit->to_room->sector_type == SECT_RIVER && !ch->has_aflag( AFF_AQUA_BREATH ) )
            continue;

         bool found = false;
         list < char_data * >::iterator ich2;
         for( ich2 = ch->in_room->people.begin(  ); ich2 != ch->in_room->people.end(  ); ++ich2 )
         {
            char_data *rch = *ich2;

            if( is_fearing( ch, rch ) )
            {
               switch ( number_bits( 2 ) )
               {
                  default:
                  case 0:
                     cmdf( ch, "yell Get away from me, %s!", rch->name );
                     break;
                  case 1:
                     cmdf( ch, "yell Leave me be, %s!", rch->name );
                     break;
                  case 2:
                     cmdf( ch, "yell %s is trying to kill me!  Help!", rch->name );
                     break;
                  case 3:
                     cmdf( ch, "yell Someone save me from %s!", rch->name );
                     break;
               }
               found = true;
               break;
            }
         }
         if( found )
            retcode = move_char( ch, pexit, 0, pexit->vdir, false );
      }
   }
}

/* Anything that should be updating based on time should go here - like hunger/thirst for one */
void char_calendar_update( void )
{
   list < char_data * >::iterator ich;

   for( ich = pclist.begin(  ); ich != pclist.end(  ); )
   {
      char_data *ch = *ich;
      ++ich;

      if( ch->char_died(  ) )
         continue;

      if( !ch->is_immortal(  ) )
      {
         gain_condition( ch, COND_DRUNK, -1 );

         /*
          * Newbies won't starve now - Samson 10-2-98 
          */
         if( ch->in_room && ch->level > 3 )
            gain_condition( ch, COND_FULL, -1 + race_table[ch->race]->hunger_mod );

         /*
          * Newbies won't dehydrate now - Samson 10-2-98 
          */
         if( ch->in_room && ch->level > 3 )
         {
            int sector;

            if( ch->has_pcflag( PCFLAG_ONMAP ) )
               sector = get_terrain( ch->wmap, ch->mx, ch->my );
            else
               sector = ch->in_room->sector_type;

            switch ( sector )
            {
               default:
                  gain_condition( ch, COND_THIRST, -1 + race_table[ch->race]->thirst_mod );
                  break;
               case SECT_DESERT:
                  gain_condition( ch, COND_THIRST, -3 + race_table[ch->race]->thirst_mod );
                  break;
               case SECT_UNDERWATER:
               case SECT_OCEANFLOOR:
                  if( number_bits( 1 ) == 0 )
                     gain_condition( ch, COND_THIRST, -1 + race_table[ch->race]->thirst_mod );
                  break;
            }
         }
      }
   }
}

/*
 * Update all chars, including mobs.
 * This function is performance sensitive.
 */
void char_update( void )
{
   list < char_data * >::iterator ich;
   char_data *ch_save = nullptr;
   short save_count = 0;

   for( ich = charlist.begin(  ); ich != charlist.end(  ); )
   {
      char_data *ch = *ich;
      ++ich;

      if( ch->char_died(  ) )
         continue;

      /*
       *  Do a room_prog rand check right off the bat
       *  if ch disappears (rprog might wax npc's), continue
       */
      if( !ch->isnpc(  ) )
         rprog_random_trigger( ch );
      if( ch->char_died(  ) )
         continue;

      if( ch->isnpc(  ) )
         mprog_time_trigger( ch );
      if( ch->char_died(  ) )
         continue;

      rprog_time_trigger( ch );
      if( ch->char_died(  ) )
         continue;

      if( ch->isnpc(  ) && update_month_trigger == true )
         mprog_month_trigger( ch );
      if( ch->char_died(  ) )
         continue;

      if( ch->isnpc(  ) && update_month_trigger == true )
         rprog_month_trigger( ch );
      if( ch->char_died(  ) )
         continue;

      /*
       * See if player should be auto-saved.
       */
      if( !ch->isnpc(  ) && ( !ch->desc || ch->desc->connected == CON_PLAYING ) && current_time - ch->pcdata->save_time > ( sysdata->save_frequency * 60 ) )
         ch_save = ch;
      else
         ch_save = nullptr;

      if( ch->position >= POS_STUNNED )
      {
         if( ch->hit < ch->max_hit )
         {
            ch->hit += hit_gain( ch );

            if( ch->hit > ch->max_hit )
               ch->hit = ch->max_hit;
         }

         if( ch->mana < ch->max_mana )
         {
            ch->mana += mana_gain( ch );

            if( ch->mana > ch->max_mana )
               ch->mana = ch->max_mana;
         }

         if( ch->move < ch->max_move )
         {
            ch->move += move_gain( ch );

            if( ch->move > ch->max_move )
               ch->move = ch->max_move;
         }
      }

      if( ch->position < POS_STUNNED && ch->hit <= -10 )
         raw_kill( ch, ch );

      if( ch->position == POS_STUNNED )
         ch->update_pos(  );

      /* Expire variables */
      if( !ch->variables.empty() )
      {
         list < variable_data * >::iterator ivar;

         for( ivar = ch->variables.begin(); ivar != ch->variables.end(); )
         {
            variable_data *vd = *ivar;
            ++ivar;

            if( vd->timer > 0 && --vd->timer == 0 )
            {
               ch->variables.remove( vd );
               deleteptr( vd );
            }
         }
      }

      /*
       * Morph timer expires 
       */
      if( ch->morph )
      {
         if( ch->morph->timer > 0 )
         {
            --ch->morph->timer;
            if( ch->morph->timer == 0 )
               do_unmorph_char( ch );
         }
      }

      if( !ch->isnpc(  ) && !ch->is_immortal(  ) )
      {
         obj_data *obj;

         if( ( obj = ch->get_eq( WEAR_LIGHT ) ) != nullptr && obj->item_type == ITEM_LIGHT && obj->value[2] > 0 )
         {
            if( --obj->value[2] == 0 && ch->in_room )
            {
               ch->in_room->light -= obj->count;
               if( ch->in_room->light < 0 )
                  ch->in_room->light = 0;
               act( AT_ACTION, "$p goes out.", ch, obj, nullptr, TO_ROOM );
               act( AT_ACTION, "$p goes out.", ch, obj, nullptr, TO_CHAR );
               obj->extract(  );
            }
         }

         if( ++ch->timer >= 12 )
         {
            if( ch->timer == 12 && ch->in_room )
            {
               if( ch->fighting )
                  ch->stop_fighting( true );
               act( AT_ACTION, "$n enters a state of suspended animation.", ch, nullptr, nullptr, TO_ROOM );
               ch->print( "You have entered a state of suspended animation.\r\n" );
               ch->set_pcflag( PCFLAG_IDLING ); /* Samson 5-8-99 */
               if( IS_SAVE_FLAG( SV_IDLE ) )
                  ch->save(  );
            }
         }

         if( ch->timer > 24 )
            interpret( ch, "quit auto" );
         else if( ch == ch_save && IS_SAVE_FLAG( SV_AUTO ) && ++save_count < 10 )
            /*
             * save max of 15 per tick 
             */
         {
            ch->save(  );
            ch->print( "You have been AutoSaved.\r\n" );
         }

         if( ch->pcdata->condition[COND_DRUNK] > 8 )
            ch->worsen_mental_state( ch->pcdata->condition[COND_DRUNK] / 8 );
         if( ch->pcdata->condition[COND_FULL] > 1 )
         {
            switch ( ch->position )
            {
               default:
                  break;
               case POS_SLEEPING:
                  ch->better_mental_state( 4 );
                  break;
               case POS_RESTING:
                  ch->better_mental_state( 3 );
                  break;
               case POS_SITTING:
               case POS_MOUNTED:
                  ch->better_mental_state( 2 );
                  break;
               case POS_STANDING:
                  ch->better_mental_state( 1 );
                  break;
               case POS_FIGHTING:
               case POS_EVASIVE:
               case POS_DEFENSIVE:
               case POS_AGGRESSIVE:
               case POS_BERSERK:
                  if( number_bits( 2 ) == 0 )
                     ch->better_mental_state( 1 );
                  break;
            }
         }

         if( ch->pcdata->condition[COND_THIRST] > 1 )
         {
            switch ( ch->position )
            {
               default:
                  break;
               case POS_SLEEPING:
                  ch->better_mental_state( 5 );
                  break;
               case POS_RESTING:
                  ch->better_mental_state( 3 );
                  break;
               case POS_SITTING:
               case POS_MOUNTED:
                  ch->better_mental_state( 2 );
                  break;
               case POS_STANDING:
                  ch->better_mental_state( 1 );
                  break;
               case POS_FIGHTING:
               case POS_EVASIVE:
               case POS_DEFENSIVE:
               case POS_AGGRESSIVE:
               case POS_BERSERK:
                  if( number_bits( 2 ) == 0 )
                     ch->better_mental_state( 1 );
                  break;
            }
         }
      }

      if( !ch->isnpc(  ) && !ch->is_immortal(  ) && ch->pcdata->release_date > 0 && ch->pcdata->release_date <= current_time )
      {
         room_index *location;

         if( ch->pcdata->clan )
            location = get_room_index( ch->pcdata->clan->recall );
         else if( ch->pcdata->deity )
            location = get_room_index( ch->pcdata->deity->recallroom );
         else
            location = get_room_index( ROOM_VNUM_TEMPLE );
         if( !location )
            location = get_room_index( ROOM_VNUM_LIMBO );
         if( !location )
            location = ch->in_room;
         MOBtrigger = false;
         ch->from_room(  );
         if( !ch->to_room( location ) )
            log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __func__, __LINE__ );
         ch->print( "The gods have released you from hell as your sentence is up!\r\n" );
         interpret( ch, "look" );
         STRFREE( ch->pcdata->helled_by );
         ch->pcdata->helled_by = nullptr;
         ch->pcdata->release_date = 0;
         ch->save(  );
      }

      if( !ch->isnpc(  ) )
         ch->calculate_age(  );

      if( !ch->char_died(  ) )
      {
         obj_data *arrow = nullptr;
         int dam = 0;

         if( ( arrow = ch->get_eq( WEAR_LODGE_RIB ) ) != nullptr )
         {
            dam = number_range( ( 2 * arrow->value[1] ), ( 2 * arrow->value[2] ) );
            act( AT_CARNAGE, "$n suffers damage from $p stuck in $s rib.", ch, arrow, nullptr, TO_ROOM );
            act( AT_CARNAGE, "You suffer damage from $p stuck in your rib.", ch, arrow, nullptr, TO_CHAR );
            damage( ch, ch, dam, TYPE_UNDEFINED );
         }
         if( ch->char_died(  ) )
            continue;

         if( ( arrow = ch->get_eq( WEAR_LODGE_LEG ) ) != nullptr )
         {
            dam = number_range( arrow->value[1], arrow->value[2] );
            act( AT_CARNAGE, "$n suffers damage from $p stuck in $s leg.", ch, arrow, nullptr, TO_ROOM );
            act( AT_CARNAGE, "You suffer damage from $p stuck in your leg.", ch, arrow, nullptr, TO_CHAR );
            damage( ch, ch, dam, TYPE_UNDEFINED );
         }
         if( ch->char_died(  ) )
            continue;

         if( ( arrow = ch->get_eq( WEAR_LODGE_ARM ) ) != nullptr )
         {
            dam = number_range( arrow->value[1], arrow->value[2] );
            act( AT_CARNAGE, "$n suffers damage from $p stuck in $s arm.", ch, arrow, nullptr, TO_ROOM );
            act( AT_CARNAGE, "You suffer damage from $p stuck in your arm.", ch, arrow, nullptr, TO_CHAR );
            damage( ch, ch, dam, TYPE_UNDEFINED );
         }
         if( ch->char_died(  ) )
            continue;
      }

      if( !ch->char_died(  ) )
      {
         /*
          * Careful with the damages here,
          *   MUST NOT refer to ch after damage taken, without checking
          *   return code and/or char_died as it may be lethal damage.
          */
         if( ch->has_aflag( AFF_POISON ) )
         {
            act( AT_POISON, "$n shivers and suffers.", ch, nullptr, nullptr, TO_ROOM );
            act( AT_POISON, "You shiver and suffer.", ch, nullptr, nullptr, TO_CHAR );
            ch->mental_state = URANGE( 20, ch->mental_state + ( ch->isnpc(  )? 2 : 4 ), 100 );
            damage( ch, ch, 30, gsn_poison );
         }
         else if( ch->position == POS_INCAP )
            damage( ch, ch, 2, TYPE_UNDEFINED );
         else if( ch->position == POS_MORTAL )
            damage( ch, ch, 4, TYPE_UNDEFINED );
         if( ch->char_died(  ) )
            continue;

         /*
          * Recurring spell affect
          */
         if( ch->has_aflag( AFF_RECURRINGSPELL ) )
         {
            list < affect_data * >::iterator paf;
            skill_type *skill;
            bool found = false, died = false;

            for( paf = ch->affects.begin(  ); paf != ch->affects.end(  ); )
            {
               affect_data *aff = *paf;
               ++paf;

               if( aff->location == APPLY_RECURRINGSPELL )
               {
                  found = true;
                  if( IS_VALID_SN( aff->modifier ) && ( skill = skill_table[aff->modifier] ) != nullptr && skill->type == SKILL_SPELL )
                  {
                     if( ( *skill->spell_fun ) ( aff->modifier, ch->level, ch, ch ) == rCHAR_DIED || ch->char_died(  ) )
                     {
                        died = true;
                        break;
                     }
                  }
               }
            }
            if( died )
               continue;
            if( !found )
               ch->unset_aflag( AFF_RECURRINGSPELL );
         }

         if( ch->mental_state >= 30 )
         {
            switch ( ( ch->mental_state + 5 ) / 10 )
            {
               default:
               case 3:
                  ch->print( "You feel feverish.\r\n" );
                  act( AT_ACTION, "$n looks kind of out of it.", ch, nullptr, nullptr, TO_ROOM );
                  break;
               case 4:
                  ch->print( "You do not feel well at all.\r\n" );
                  act( AT_ACTION, "$n doesn't look too good.", ch, nullptr, nullptr, TO_ROOM );
                  break;
               case 5:
                  ch->print( "You need help!\r\n" );
                  act( AT_ACTION, "$n looks like $e could use your help.", ch, nullptr, nullptr, TO_ROOM );
                  break;
               case 6:
                  ch->print( "Seekest thou a cleric.\r\n" );
                  act( AT_ACTION, "Someone should fetch a healer for $n.", ch, nullptr, nullptr, TO_ROOM );
                  break;
               case 7:
                  ch->print( "You feel reality slipping away...\r\n" );
                  act( AT_ACTION, "$n doesn't appear to be aware of what's going on.", ch, nullptr, nullptr, TO_ROOM );
                  break;
               case 8:
                  ch->print( "You begin to understand... everything.\r\n" );
                  act( AT_ACTION, "$n starts ranting like a madman!", ch, nullptr, nullptr, TO_ROOM );
                  break;
               case 9:
                  ch->print( "You are ONE with the universe.\r\n" );
                  act( AT_ACTION, "$n is ranting on about 'the answer', 'ONE' and other mumbo-jumbo...", ch, nullptr, nullptr, TO_ROOM );
                  break;
               case 10:
                  ch->print( "You feel the end is near.\r\n" );
                  act( AT_ACTION, "$n is muttering and ranting in tongues...", ch, nullptr, nullptr, TO_ROOM );
                  break;
            }
         }

         if( ch->mental_state <= -30 )
         {
            switch ( ( abs( ch->mental_state ) + 5 ) / 10 )
            {
               case 10:
                  if( ch->position > POS_SLEEPING )
                  {
                     if( ( ch->position == POS_STANDING || ch->position < POS_BERSERK ) && number_percent(  ) + 10 < abs( ch->mental_state ) )
                        interpret( ch, "sleep" );
                     else
                        ch->print( "You're barely conscious.\r\n" );
                  }
                  break;
               case 9:
                  if( ch->position > POS_SLEEPING )
                  {
                     if( ( ch->position == POS_STANDING || ch->position < POS_BERSERK ) && ( number_percent(  ) + 20 ) < abs( ch->mental_state ) )
                        interpret( ch, "sleep" );
                     else
                        ch->print( "You can barely keep your eyes open.\r\n" );
                  }
                  break;
               case 8:
                  if( ch->position > POS_SLEEPING )
                  {
                     if( ch->position < POS_SITTING && ( number_percent(  ) + 30 ) < abs( ch->mental_state ) )
                        interpret( ch, "sleep" );
                     else
                        ch->print( "You're extremely drowsy.\r\n" );
                  }
                  break;
               case 7:
                  if( ch->position > POS_RESTING )
                     ch->print( "You feel very unmotivated.\r\n" );
                  break;
               case 6:
                  if( ch->position > POS_RESTING )
                     ch->print( "You feel sedated.\r\n" );
                  break;
               case 5:
                  if( ch->position > POS_RESTING )
                     ch->print( "You feel sleepy.\r\n" );
                  break;
               case 4:
                  if( ch->position > POS_RESTING )
                     ch->print( "You feel tired.\r\n" );
                  break;
               default:
               case 3:
                  if( ch->position > POS_RESTING )
                     ch->print( "You could use a rest.\r\n" );
                  break;
            }
         }

         if( ch->timer > 24 )
            interpret( ch, "quit auto" );
         else if( ch == ch_save && IS_SAVE_FLAG( SV_AUTO ) && ++save_count < 10 )   /* save max of 10 per tick */
            ch->save(  );
      }
   }
}

/*
 * Update all objs.
 * This function is performance sensitive.
 */
void obj_update( void )
{
   list < obj_data * >::iterator iobj;

   for( iobj = objlist.begin(  ); iobj != objlist.end(  ); )
   {
      obj_data *obj = *iobj;
      ++iobj;

      // Due to nature of std::list, DO NOT remove this check - objects are not deallocated until this loop is done!
      if( obj->extracted(  ) )
         continue;

      const char *message = "RESET FOR NEW OBJECT";
      short AT_TEMP = -1;

      if( obj->carried_by )
      {
         oprog_random_trigger( obj );
         if( update_month_trigger == true )
            oprog_month_trigger( obj );
      }
      else if( obj->in_room && obj->in_room->area->nplayer > 0 )
      {
         oprog_random_trigger( obj );
         if( update_month_trigger == true )
            oprog_month_trigger( obj );
      }

      if( obj->item_type == ITEM_LIGHT )
      {
         char_data *tch = nullptr;

         if( ( tch = obj->carried_by ) )
         {
            if( !tch->isnpc() /* && ( tch->level < LEVEL_IMMORTAL ) */
               && ( ( obj == tch->get_eq( WEAR_LIGHT ) )
               || ( IS_SET( obj->value[3], PIPE_LIT ) ) )
               && ( obj->value[2] > 0 ) )
            {

               if( --obj->value[2] == 0 && tch->in_room )
               {
                  tch->in_room->light -= obj->count;
                  if( tch->in_room->light < 0 )
                     tch->in_room->light = 0;

                  act( AT_ACTION, "$p goes out.", tch, obj, nullptr, TO_ROOM );
                  act( AT_ACTION, "$p goes out.", tch, obj, nullptr, TO_CHAR );

                  obj->extract( );
                  continue;
               }
            }
         }
         else if( obj->in_room )
         {
			   if( IS_SET( obj->value[3], PIPE_LIT ) && ( obj->value[2] > 0 ) )
            {
               if ( --obj->value[2] == 0 )
               {
                  obj->in_room->light -= obj->count;
                  if( obj->in_room->light < 0 )
                     obj->in_room->light = 0;

                  if( !obj->in_room->people.empty() )
                  {
                     act( AT_ACTION, "$p goes out.", ( *obj->in_room->people.begin(  ) ), obj, nullptr, TO_ROOM );
                     act( AT_ACTION, "$p goes out.", ( *obj->in_room->people.begin(  ) ), obj, nullptr, TO_CHAR );
                  }

                  obj->extract( );
                  continue;
               }
            }
         }
      }

      if( obj->item_type == ITEM_PIPE )
      {
         if( IS_SET( obj->value[3], PIPE_LIT ) )
         {
            if( --obj->value[1] <= 0 )
            {
               obj->value[1] = 0;
               REMOVE_BIT( obj->value[3], PIPE_LIT );
            }
            else if( IS_SET( obj->value[3], PIPE_HOT ) )
               REMOVE_BIT( obj->value[3], PIPE_HOT );
            else
            {
               if( IS_SET( obj->value[3], PIPE_GOINGOUT ) )
               {
                  REMOVE_BIT( obj->value[3], PIPE_LIT );
                  REMOVE_BIT( obj->value[3], PIPE_GOINGOUT );
               }
               else
                  SET_BIT( obj->value[3], PIPE_GOINGOUT );
            }
            if( !IS_SET( obj->value[3], PIPE_LIT ) )
               SET_BIT( obj->value[3], PIPE_FULLOFASH );
         }
         else
            REMOVE_BIT( obj->value[3], PIPE_HOT );
      }

      /*
       * Separated these because the stock method had a bug, and it wasn't working with the skeletons - Samson 
       */
      if( obj->item_type == ITEM_CORPSE_PC && obj->value[5] == 0 )
      {
         char name[MSL];
         char *bufptr;
         int frac = obj->timer / 8;
         bufptr = one_argument( obj->short_descr, name );
         bufptr = one_argument( bufptr, name );
         bufptr = one_argument( bufptr, name );

         obj->separate(  );

         frac = URANGE( 1, frac, 5 );

         obj->value[3] = frac;   /* Advances decay stage for resurrection spell */
         stralloc_printf( &obj->objdesc, corpse_descs[frac], bufptr );

         /*
          * Why not update the description? 
          */
         write_corpse( obj, bufptr );
      }

      /*
       * Make sure skeletons get saved as their timers decrease 
       */
      if( obj->item_type == ITEM_CORPSE_PC && obj->value[5] == 1 )
         write_corpse( obj, obj->short_descr + 14 );

      if( obj->item_type == ITEM_CORPSE_NPC && obj->timer <= 5 )
      {
         char name[MSL];
         char *bufptr;
         bufptr = one_argument( obj->short_descr, name );
         bufptr = one_argument( bufptr, name );
         bufptr = one_argument( bufptr, name );

         obj->separate(  );
         stralloc_printf( &obj->objdesc, corpse_descs[obj->timer], bufptr );
      }

      /*
       * don't let inventory decay
       */
      if( obj->extra_flags.test( ITEM_INVENTORY ) )
         continue;

      /*
       * groundrot items only decay on the ground 
       */
      if( obj->extra_flags.test( ITEM_GROUNDROT ) && !obj->in_room )
         continue;

      if( ( obj->timer <= 0 || --obj->timer > 0 ) )
         continue;

      /*
       * if we get this far, object's timer has expired. 
       */
      AT_TEMP = AT_PLAIN;
      switch ( obj->item_type )
      {
         default:
            message = "$p mysteriously vanishes.";
            AT_TEMP = AT_PLAIN;
            break;

         case ITEM_CONTAINER:
            message = "$p falls apart, tattered from age.";
            AT_TEMP = AT_OBJECT;
            break;

         case ITEM_PORTAL:
            message = "$p unravels and winks from existence.";
            obj->remove_portal(  );
            obj->item_type = ITEM_TRASH;  /* so extract_obj  */
            AT_TEMP = AT_MAGIC;  /* doesn't remove_portal */
            break;

         case ITEM_FOUNTAIN:
         case ITEM_PUDDLE:
            message = "$p dries up.";
            AT_TEMP = AT_BLUE;
            break;

         case ITEM_CORPSE_PC:
            if( obj->value[5] == 0 )
               message = "The flesh from $p decays away leaving just a skeleton behind.";
            else
               message = "$p decays into dust and blows away.";
            AT_TEMP = AT_OBJECT;
            break;

         case ITEM_CORPSE_NPC:
            message = "$p decays into dust and blows away.";
            AT_TEMP = AT_OBJECT;
            break;

         case ITEM_COOK:
         case ITEM_FOOD:
            message = "$p is devoured by a swarm of maggots.";
            AT_TEMP = AT_HUNGRY;
            break;

         case ITEM_BLOOD:
            message = "$p slowly seeps into the ground.";
            AT_TEMP = AT_BLOOD;
            break;

         case ITEM_BLOODSTAIN:
            message = "$p dries up into flakes and blows away.";
            AT_TEMP = AT_ORANGE;
            break;

         case ITEM_SCRAPS:
            message = "$p crumble and decay into nothing.";
            AT_TEMP = AT_OBJECT;
            break;

         case ITEM_FIRE:
            message = "$p burns out.";
            AT_TEMP = AT_FIRE;
            break;

         case ITEM_TRASH:
            message = "$p mysteriously vanishes.";
            AT_TEMP = AT_PLAIN;
            if( obj->in_room )
            {
               if( obj->pIndexData->vnum == OBJ_VNUM_FIREPIT )
               {
                  message = "$p scatters away in the breeze.";
                  AT_TEMP = AT_OBJECT;
               }
            }
      }

      if( obj->carried_by )
         act( AT_TEMP, message, obj->carried_by, obj, nullptr, TO_CHAR );

      else if( obj->in_room && !obj->in_room->people.empty(  ) && !obj->extra_flags.test( ITEM_BURIED ) )
      {
         act( AT_TEMP, message, ( *obj->in_room->people.begin(  ) ), obj, nullptr, TO_ROOM );
         act( AT_TEMP, message, ( *obj->in_room->people.begin(  ) ), obj, nullptr, TO_CHAR );
      }

      /*
       * Player skeletons surrender any objects on them to the room when the corpse decays away. - Samson 8-13-98 
       */
      if( obj->item_type == ITEM_CORPSE_PC )
      {
         if( obj->value[5] == 0 )
         {
            char name[MSL], name2[MSL];
            char *bufptr;

            bufptr = one_argument( obj->short_descr, name );
            bufptr = one_argument( bufptr, name );
            bufptr = one_argument( bufptr, name );
            mudstrlcpy( name2, bufptr, MSL );   /* Dunno why, but it's corrupting if I don't do this - Samson */

            obj->timer = 250; /* Corpse is now a skeleton */
            obj->value[3] = 0;
            obj->value[5] = 1;

            stralloc_printf( &obj->name, "corpse skeleton %s", name2 );
            stralloc_printf( &obj->short_descr, "A skeleton of %s", name2 );
            stralloc_printf( &obj->objdesc, corpse_descs[0], name2 );
            write_corpse( obj, obj->short_descr + 14 );
         }
         else
         {
            if( obj->carried_by )
               obj->empty( nullptr, obj->carried_by->in_room );
            else if( obj->in_room )
               obj->empty( nullptr, obj->in_room );

            write_corpse( obj, obj->short_descr + 14 );
         }
      }

      if( obj->item_type == ITEM_FIRE )
      {
         obj_data *firepit;

         if( !( firepit = get_obj_index( OBJ_VNUM_FIREPIT )->create_object( 1 ) ) )
            log_printf( "create_object: %s:%s, line %d.", __FILE__, __func__, __LINE__ );
         else
         {
            firepit->wmap = obj->wmap;
            firepit->mx = obj->mx;
            firepit->my = obj->my;
            set_supermob( obj );
            firepit->to_room( obj->in_room, supermob );
            release_supermob(  );
            firepit->timer = 40;
         }
      }
      if( obj->timer < 1 )
      {
         // Dump contents from PC carried containers onto ground before extraction - Samson 2/4/05
         if( obj->carried_by && obj->carried_by->in_room && !obj->carried_by->isnpc(  ) )
            obj->empty( nullptr, obj->carried_by->in_room );
         obj->extract(  );
      }
   }
}

/*
 * Function to check important stuff happening to a player
 * This function should take about 5% of mud cpu time
 ( Figure should be far less now that it only checks pclist - Samson )
 */
void char_check( void )
{
   list < char_data * >::iterator ich;
   static int pulse = 0;

   pulse = ( pulse + 1 ) % 100;

   for( ich = pclist.begin(  ); ich != pclist.end(  ); )
   {
      char_data *ch = *ich;
      ++ich;

      if( ch->char_died(  ) )
         continue;

      list < timer_data * >::iterator chtimer;
      for( chtimer = ch->timers.begin(  ); chtimer != ch->timers.end(  ); )
      {
         timer_data *timer = *chtimer;
         ++chtimer;

         if( ch->fighting && timer->type == TIMER_DO_FUN )
         {
            int tempsub;

            tempsub = ch->substate;
            ch->substate = SUB_TIMER_DO_ABORT;
            ( timer->do_fun ) ( ch, "" );
            if( ch->char_died( ) )
               break;
            ch->substate = tempsub;
            ch->extract_timer( timer );
            continue;
         }

         if( --timer->count <= 0 )
         {
            if( timer->type == TIMER_ASUPRESSED )
            {
               if( timer->value == -1 )
               {
                  timer->count = 1000;
                  continue;
               }
            }

            if( timer->type == TIMER_DO_FUN )
            {
               int tempsub;

               tempsub = ch->substate;
               ch->substate = timer->value;
               ( timer->do_fun ) ( ch, "" );
               if( ch->char_died(  ) )
                  break;
               ch->substate = tempsub;
               if( timer->count > 0 )
                  continue;
            }
            ch->extract_timer( timer );
         }
      }

      if( ch->char_died(  ) )
         continue;

      /*
       * check for exits moving players around 
       */
      if( pullcheck( ch, pulse ) == rCHAR_DIED || ch->char_died(  ) )
         continue;

      if( ch->mount && ch->in_room != ch->mount->in_room )
      {
         ch->mount->unset_actflag( ACT_MOUNTED );
         ch->mount = nullptr;
         ch->position = POS_STANDING;
         ch->print( "No longer upon your mount, you fall to the ground...\r\nOUCH!\r\n" );
      }

      if( ( ch->in_room && ch->in_room->sector_type == SECT_UNDERWATER ) || ( ch->in_room && ch->in_room->sector_type == SECT_OCEANFLOOR ) )
      {
         /*
          * Immortal timer test message added by Samson, works only in room 1450 
          */
         if( ch->is_immortal(  ) && ch->in_room->vnum == 1450 )
            ch->print( "You're underwater, or on the oceanfloor." );

         if( !ch->has_aflag( AFF_AQUA_BREATH ) )
         {
            /*
             * Immortal timer test message added by Samson, works only in room 1450 
             */
            if( ch->is_immortal(  ) && ch->in_room->vnum == 1450 )
               ch->print( "No aqua breath, you'd be drowning this fast if mortal." );
            if( !ch->is_immortal(  ) )
            {
               int dam;

               /*
                * Changed level of damage at Brittany's request. -- Narn 
                */
               dam = number_range( ch->max_hit / 100, ch->max_hit / 50 );
               dam = UMAX( 1, dam );
               if( number_bits( 3 ) == 0 )
                  ch->print( "You cough and choke as you try to breathe water!\r\n" );
               damage( ch, ch, dam, TYPE_UNDEFINED );
            }
         }
      }

      if( ch->char_died(  ) )
         continue;

      if( ch->in_room && ( ch->in_room->sector_type == SECT_RIVER ) )
      {
         /*
          * Immortal timer test message added by Samson, works only in room 1450 
          */
         if( ch->is_immortal(  ) && ch->in_room->vnum == 1450 )
            ch->print( "You're in a river, which hopefully will be flowing soon.\r\n" );

         if( !ch->has_aflag( AFF_FLYING ) && !ch->has_aflag( AFF_AQUA_BREATH ) && !ch->has_aflag( AFF_FLOATING ) )
         {
            bool boat = false;
            list < obj_data * >::iterator iobj;
            for( iobj = ch->carrying.begin(  ); iobj != ch->carrying.end(  ); ++iobj )
            {
               obj_data *obj = *iobj;
               if( obj->item_type == ITEM_BOAT )
               {
                  boat = true;
                  break;
               }
            }

            if( !boat )
            {
               /*
                * Immortal timer test message added by Samson, works only in room 1450 
                */
               if( ch->is_immortal(  ) && ch->in_room->vnum == 1450 )
                  ch->print( "If mortal, now you'd be drowning this fast.\r\n" );

               if( !ch->is_immortal(  ) )
               {
                  int mov, dam;

                  if( ch->move > 0 )
                  {
                     mov = number_range( ch->max_move / 20, ch->max_move / 5 );
                     mov = UMAX( 1, mov );

                     if( ch->move - mov < 0 )
                        ch->move = 0;
                     else
                        ch->move -= mov;

                     if( number_bits( 3 ) == 0 )
                        ch->print( "You struggle to remain afloat in the current.\r\n" );
                  }
                  else
                  {
                     dam = number_range( ch->max_hit / 20, ch->max_hit / 5 );
                     dam = UMAX( 1, dam );

                     if( number_bits( 3 ) == 0 )
                        ch->print( "Struggling with exhaustion, you choke on a mouthful of water.\r\n" );
                     damage( ch, ch, dam, TYPE_UNDEFINED );
                  }
               }
            }
         }
      }

      if( ch->in_room && ch->in_room->sector_type == SECT_WATER_SWIM )
      {
         /*
          * Immortal timer test message added by Samson, works only in room 1450 
          */
         if( ch->is_immortal(  ) && ch->in_room->vnum == 1450 )
            ch->print( "You're in shallow water.\r\n" );

         if( !ch->has_aflag( AFF_FLYING ) && !ch->has_aflag( AFF_FLOATING ) && !ch->has_aflag( AFF_AQUA_BREATH ) && !ch->mount )
         {
            bool boat = false;
            list < obj_data * >::iterator iobj;

            for( iobj = ch->carrying.begin(  ); iobj != ch->carrying.end(  ); ++iobj )
            {
               obj_data *obj = *iobj;

               if( obj->item_type == ITEM_BOAT )
               {
                  boat = true;
                  break;
               }
            }

            if( !boat )
            {
               /*
                * Immortal timer test message added by Samson, works only in room 1450 
                */
               if( ch->is_immortal(  ) && ch->in_room->vnum == 1450 )
                  ch->print( "One of these days I suppose swim skill would be nice.\r\n" );

               if( ch->level < LEVEL_IMMORTAL )
               {
                  int mov, dam;

                  if( ch->move > 0 )
                  {
                     mov = number_range( ch->max_move / 20, ch->max_move / 5 );
                     mov = UMAX( 1, mov );

                     if( !ch->isnpc(  ) && number_percent(  ) < ch->pcdata->learned[gsn_swim] )
                        ;
                     else
                     {
                        if( ch->move - mov < 0 )
                           ch->move = 0;
                        else
                           ch->move -= mov;

                        if( number_bits( 3 ) == 0 )
                        {
                           ch->print( "You struggle to remain afloat.\r\n" );
                           if( !ch->isnpc(  ) && ch->pcdata->learned[gsn_swim] > 0 )
                              ch->learn_from_failure( gsn_swim );
                        }
                     }
                  }
                  else
                  {
                     dam = number_range( ch->max_hit / 20, ch->max_hit / 5 );
                     dam = UMAX( 1, dam );

                     if( number_bits( 3 ) == 0 )
                        ch->print( "Struggling with exhaustion, you choke on a mouthful of water.\r\n" );
                     damage( ch, ch, dam, TYPE_UNDEFINED );
                     if( !ch->isnpc(  ) && ch->pcdata->learned[gsn_swim] > 0 )
                        ch->learn_from_failure( gsn_swim );
                  }
               }
            }
         }
      }

      if( ch->char_died(  ) )
         continue;

      if( ch->in_room && ch->in_room->sector_type == SECT_WATER_NOSWIM )
      {
         /*
          * Immortal timer test message added by Samson, works only in room 1450 
          */
         if( ch->is_immortal(  ) && ch->in_room->vnum == 1450 )
            ch->print( "You're in deep water.\r\n" );

         if( !ch->has_aflag( AFF_FLYING ) && !ch->has_aflag( AFF_FLOATING ) && !ch->has_aflag( AFF_AQUA_BREATH ) && !ch->mount )
         {
            bool boat = false;
            list < obj_data * >::iterator iobj;
            for( iobj = ch->carrying.begin(  ); iobj != ch->carrying.end(  ); ++iobj )
            {
               obj_data *obj = *iobj;

               if( obj->item_type == ITEM_BOAT )
               {
                  boat = true;
                  break;
               }
            }

            if( !boat )
            {
               /*
                * Immortal timer test message added by Samson, works only in room 1450 
                */
               if( ch->is_immortal(  ) && ch->in_room->vnum == 1450 )
                  ch->print( "You'd be drowning this fast if mortal.\r\n" );

               if( !ch->is_immortal(  ) )
               {
                  int mov, dam;

                  if( ch->move > 0 )
                  {
                     mov = number_range( ch->max_move / 20, ch->max_move / 5 );
                     mov = UMAX( 1, mov );

                     if( ch->move - mov < 0 )
                        ch->move = 0;
                     else
                        ch->move -= mov;

                     if( number_bits( 3 ) == 0 )
                        ch->print( "You struggle to remain afloat in the deep water.\r\n" );
                  }
                  else
                  {
                     dam = number_range( ch->max_hit / 20, ch->max_hit / 5 );
                     dam = UMAX( 1, dam );

                     if( number_bits( 3 ) == 0 )
                        ch->print( "Struggling with exhaustion, you choke on a mouthful of water.\r\n" );
                     damage( ch, ch, dam, TYPE_UNDEFINED );
                  }
               }
            }
         }
      }
   }
}

/*
 * Aggress.
 *
 * for each descriptor
 *     for each mob in room
 *         aggress on some random PC
 *
 * This function should take 5% to 10% of ALL mud cpu time.
 * Unfortunately, checking on each PC move is too tricky,
 *   because we don't the mob to just attack the first PC
 *   who leads the party into the room.
 *
 */
void aggr_update( void )
{
   list < descriptor_data * >::iterator ds;

   /*
    * Just check descriptors here for vics to aggressive mobs
    * We can check for linkdead vics in char_check -Thoric
    */
   for( ds = dlist.begin(  ); ds != dlist.end(  ); )
   {
      char_data *wch = nullptr;
      descriptor_data *d = *ds;
      ++ds;

      if( ( d->connected != CON_PLAYING && d->connected != CON_EDITING ) || !( wch = d->character ) )
         continue;

      if( wch->char_died(  ) || wch->isnpc(  ) || wch->level >= LEVEL_IMMORTAL || !wch->in_room || wch->has_pcflag( PCFLAG_IDLING ) )
         /*
          * Protect Idle/Linkdead players - Samson 5-8-99 
          */
         continue;

      list < char_data * >::iterator ich;
      for( ich = wch->in_room->people.begin(  ); ich != wch->in_room->people.end(  ); )
      {
         char_data *victim = nullptr;
         int count = 0;
         char_data *ch = *ich;
         ++ich;

         if( !ch->isnpc(  ) || ch->fighting || ch->has_aflag( AFF_CHARM ) || !ch->IS_AWAKE(  )
             || ( ch->has_actflag( ACT_WIMPY ) && wch->IS_AWAKE(  ) ) || !ch->can_see( wch, false ) )
            continue;

         if( is_hating( ch, wch ) )
         {
            found_prey( ch, wch );
            continue;
         }

         if( ( !ch->has_actflag( ACT_AGGRESSIVE ) && !ch->has_actflag( ACT_META_AGGR ) ) || ch->has_actflag( ACT_MOUNTED ) || ch->in_room->flags.test( ROOM_SAFE ) )
            continue;

         /*
          * Ok we have a 'wch' player character and a 'ch' npc aggressor.
          * Now make the aggressor fight a RANDOM pc victim in the room,
          *   giving each 'vch' an equal chance of selection.
          *
          * Depending on flags set, the mob may attack another mob
          */
         list < char_data * >::iterator ich2;
         for( ich2 = wch->in_room->people.begin(  ); ich2 != wch->in_room->people.end(  ); )
         {
            char_data *vch = *ich2;
            ++ich2;

            if( ( !vch->isnpc(  ) || ch->has_actflag( ACT_META_AGGR ) || vch->has_actflag( ACT_ANNOYING ) )
                && vch->level < LEVEL_IMMORTAL && ( !ch->has_actflag( ACT_WIMPY ) || !vch->IS_AWAKE(  ) ) && ch->can_see( vch, false ) )
            {
               if( number_range( 0, count ) == 0 )
                  victim = vch;
               ++count;
            }
         }

         if( !victim )
         {
            bug( "%s: null victim. Aggro: %s", __func__, ch->name );
            log_printf( "Breaking %s loop and transferring aggressor to Limbo.", __func__ );
            ch->from_room(  );
            if( !ch->to_room( get_room_index( ROOM_VNUM_LIMBO ) ) )
               log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __func__, __LINE__ );
            break;
         }

         /*
          * Skyships are immune to attack 
          */
         if( victim->has_pcflag( PCFLAG_BOARDED ) )
            break;

         /*
          * backstabbing mobs (Thoric) 
          */
         if( ch->has_attack( ATCK_BACKSTAB ) )
         {
            obj_data *obj;

            if( !ch->mount && ( obj = ch->get_eq( WEAR_WIELD ) ) != nullptr && ( obj->value[4] == WEP_DAGGER ) && !victim->fighting && victim->hit >= victim->max_hit )
            {
               check_attacker( ch, victim );
               ch->WAIT_STATE( skill_table[gsn_backstab]->beats );
               if( !victim->IS_AWAKE(  ) || number_percent(  ) + 5 < ch->level )
               {
                  global_retcode = multi_hit( ch, victim, gsn_backstab );
                  continue;
               }
               else
               {
                  global_retcode = damage( ch, victim, 0, gsn_backstab );
                  continue;
               }
            }
         }
         global_retcode = multi_hit( ch, victim, TYPE_UNDEFINED );
      }
   }
}

void mob_act_update(  )
{
   list < char_data * >::iterator ach;

   /*
    * check mobprog act queue
    */
   for( ach = mob_act_list.begin(  ); ach != mob_act_list.end(  ); )
   {
      char_data *pch = *ach;
      ++ach;

      if( !pch->char_died(  ) && pch->mpactnum > 0 )
      {
         list < mprog_act_list * >::iterator mal;
         for( mal = pch->mpact.begin(  ); mal != pch->mpact.end(  ); )
         {
            mprog_act_list *tmp_act = *mal;
            ++mal;

            if( tmp_act->obj && tmp_act->obj->extracted(  ) )
               tmp_act->obj = nullptr;
            if( tmp_act->ch && !tmp_act->ch->char_died(  ) )
               mprog_wordlist_check( tmp_act->buf, pch, tmp_act->ch, tmp_act->obj, tmp_act->victim, tmp_act->target, ACT_PROG );
            pch->mpact.remove( tmp_act );
            deleteptr( tmp_act );
         }
         pch->mpactnum = 0;
      }
      mob_act_list.remove( pch );
   }
}

void tele_update( void )
{
   list < teleport_data * >::iterator tele;

   if( teleportlist.empty(  ) )
      return;

   for( tele = teleportlist.begin(  ); tele != teleportlist.end(  ); )
   {
      teleport_data *tport = *tele;
      ++tele;

      if( --tport->timer <= 0 )
      {
         if( !tport->room->people.empty(  ) )
         {
            if( tport->room->flags.test( ROOM_TELESHOWDESC ) )
               teleport( ( *tport->room->people.begin(  ) ), tport->room->tele_vnum, TELE_SHOWDESC | TELE_TRANSALL );
            else
               teleport( ( *tport->room->people.begin(  ) ), tport->room->tele_vnum, TELE_TRANSALL );
         }
         teleportlist.remove( tport );
         deleteptr( tport );
      }
   }
}

/*
 * Function to update weather vectors according to climate
 * settings, random effects, and neighboring areas.
 * Last modified: July 18, 1997
 * - Fireblade
 */
void adjust_vectors( weather_data * weather )
{
   list < neighbor_data * >::iterator lneigh;
   double dT = 0, dP = 0, dW = 0;

   if( !weather )
   {
      bug( "%s: nullptr weather data.", __func__ );
      return;
   }

   /*
    * Add in random effects 
    */
   dT += number_range( -rand_factor, rand_factor );
   dP += number_range( -rand_factor, rand_factor );
   dW += number_range( -rand_factor, rand_factor );

   /*
    * Add in climate effects
    */
   dT += climate_factor * ( ( ( weather->climate_temp - 2 ) * weath_unit ) - ( weather->temp ) ) / weath_unit;
   dP += climate_factor * ( ( ( weather->climate_precip - 2 ) * weath_unit ) - ( weather->precip ) ) / weath_unit;
   dW += climate_factor * ( ( ( weather->climate_wind - 2 ) * weath_unit ) - ( weather->wind ) ) / weath_unit;

   /*
    * Add in effects from neighboring areas 
    */
   for( lneigh = weather->neighborlist.begin(  ); lneigh != weather->neighborlist.end(  ); )
   {
      neighbor_data *neigh = *lneigh;
      ++lneigh;

      /*
       * see if we have the area cache'd already
       */
      if( !neigh->address )
      {
         /*
          * try and find address for area
          */
         neigh->address = get_area( neigh->name );

         /*
          * if couldn't find area ditch the neigh
          */
         if( !neigh->address )
         {
            weather->neighborlist.remove( neigh );
            deleteptr( neigh );
            continue;
         }
      }
      dT += ( neigh->address->weather->temp - weather->temp ) / neigh_factor;
      dP += ( neigh->address->weather->precip - weather->precip ) / neigh_factor;
      dW += ( neigh->address->weather->wind - weather->wind ) / neigh_factor;
   }

   /*
    * now apply the effects to the vectors 
    */
   weather->temp_vector += ( int )dT;
   weather->precip_vector += ( int )dP;
   weather->wind_vector += ( int )dW;

   /*
    * Make sure they are within the right range 
    */
   weather->temp_vector = URANGE( -max_vector, weather->temp_vector, max_vector );
   weather->precip_vector = URANGE( -max_vector, weather->precip_vector, max_vector );
   weather->wind_vector = URANGE( -max_vector, weather->wind_vector, max_vector );
}

/*
 * get weather echo messages according to area weather...
 * stores echo message in weath_data.... must be called before
 * the vectors are adjusted
 * Last Modified: August 10, 1997
 * Fireblade
 */
void get_weather_echo( weather_data * weath )
{
   /*
    * set echo to be nothing 
    */
   weath->echo.clear(  );
   weath->echo_color = AT_GREY;

   /*
    * get the random number 
    */
   int n = number_bits( 2 );

   /*
    * variables for convenience 
    */
   int temp = weath->temp;
   int precip = weath->precip;
//   int wind = weath->wind; <- This one is flagged as unused

   int dT = weath->temp_vector;
   int dP = weath->precip_vector;
//   int dW = weath->wind_vector; <- This one is flagged as unused

   int tindex = ( temp + 3 * weath_unit - 1 ) / weath_unit;
   int pindex = ( precip + 3 * weath_unit - 1 ) / weath_unit;
//   int windex = ( wind + 3 * weath_unit - 1 ) / weath_unit; <- This one is flagged as unused

   /*
    * get the echo string... mainly based on precip 
    */
   switch ( pindex )
   {
      case 0:
         if( precip - dP > -2 * weath_unit )
         {
            const char *echo_strings[4] = {
               "The clouds disappear.\r\n",
               "The clouds disappear.\r\n",
               "The sky begins to break through the clouds.\r\n",
               "The clouds are slowly evaporating.\r\n"
            };

            weath->echo = echo_strings[n];
            weath->echo_color = AT_WHITE;
         }
         break;

      case 1:
         if( precip - dP <= -2 * weath_unit )
         {
            const char *echo_strings[4] = {
               "The sky is getting cloudy.\r\n",
               "The sky is getting cloudy.\r\n",
               "Light clouds cast a haze over the sky.\r\n",
               "Billows of clouds spread through the sky.\r\n"
            };
            weath->echo = echo_strings[n];
            weath->echo_color = AT_GREY;
         }
         break;

      case 2:
         if( precip - dP > 0 )
         {
            if( tindex > 1 )
            {
               const char *echo_strings[4] = {
                  "The rain stops.\r\n",
                  "The rain stops.\r\n",
                  "The rainstorm tapers off.\r\n",
                  "The rain's intensity breaks.\r\n"
               };
               weath->echo = echo_strings[n];
               weath->echo_color = AT_CYAN;
            }
            else
            {
               const char *echo_strings[4] = {
                  "The snow stops.\r\n",
                  "The snow stops.\r\n",
                  "The snow showers taper off.\r\n",
                  "The snow flakes disappear from the sky.\r\n"
               };
               weath->echo = echo_strings[n];
               weath->echo_color = AT_WHITE;
            }
         }
         break;

      case 3:
         if( precip - dP <= 0 )
         {
            if( tindex > 1 )
            {
               const char *echo_strings[4] = {
                  "It starts to rain.\r\n",
                  "It starts to rain.\r\n",
                  "A droplet of rain falls upon you.\r\n",
                  "The rain begins to patter.\r\n"
               };
               weath->echo = echo_strings[n];
               weath->echo_color = AT_CYAN;
            }
            else
            {
               const char *echo_strings[4] = {
                  "It starts to snow.\r\n",
                  "It starts to snow.\r\n",
                  "Crystal flakes begin to fall from the sky.\r\n",
                  "Snow flakes drift down from the clouds.\r\n"
               };
               weath->echo = echo_strings[n];
               weath->echo_color = AT_WHITE;
            }
         }
         else if( tindex < 2 && temp - dT > -weath_unit )
         {
            const char *echo_strings[4] = {
               "The temperature drops and the rain becomes a light snow.\r\n",
               "The temperature drops and the rain becomes a light snow.\r\n",
               "Flurries form as the rain freezes.\r\n",
               "Large snow flakes begin to fall with the rain.\r\n"
            };
            weath->echo = echo_strings[n];
            weath->echo_color = AT_WHITE;
         }
         else if( tindex > 1 && temp - dT <= -weath_unit )
         {
            const char *echo_strings[4] = {
               "The snow flurries are gradually replaced by pockets of rain.\r\n",
               "The snow flurries are gradually replaced by pockets of rain.\r\n",
               "The falling snow turns to a cold drizzle.\r\n",
               "The snow turns to rain as the air warms.\r\n"
            };
            weath->echo = echo_strings[n];
            weath->echo_color = AT_CYAN;
         }
         break;

      case 4:
         if( precip - dP > 2 * weath_unit )
         {
            if( tindex > 1 )
            {
               const char *echo_strings[4] = {
                  "The lightning has stopped.\r\n",
                  "The lightning has stopped.\r\n",
                  "The sky settles, and the thunder surrenders.\r\n",
                  "The lightning bursts fade as the storm weakens.\r\n"
               };
               weath->echo = echo_strings[n];
               weath->echo_color = AT_GREY;
            }
         }
         else if( tindex < 2 && temp - dT > -weath_unit )
         {
            const char *echo_strings[4] = {
               "The cold rain turns to snow.\r\n",
               "The cold rain turns to snow.\r\n",
               "Snow flakes begin to fall amidst the rain.\r\n",
               "The driving rain begins to freeze.\r\n"
            };
            weath->echo = echo_strings[n];
            weath->echo_color = AT_WHITE;
         }
         else if( tindex > 1 && temp - dT <= -weath_unit )
         {
            const char *echo_strings[4] = {
               "The snow becomes a freezing rain.\r\n",
               "The snow becomes a freezing rain.\r\n",
               "A cold rain beats down on you as the snow begins to melt.\r\n",
               "The snow is slowly replaced by a heavy rain.\r\n"
            };
            weath->echo = echo_strings[n];
            weath->echo_color = AT_CYAN;
         }
         break;

      case 5:
         if( precip - dP <= 2 * weath_unit )
         {
            if( tindex > 1 )
            {
               const char *echo_strings[4] = {
                  "Lightning flashes in the sky.\r\n",
                  "Lightning flashes in the sky.\r\n",
                  "A flash of lightning splits the sky.\r\n",
                  "The sky flashes, and the ground trembles with thunder.\r\n"
               };
               weath->echo = echo_strings[n];
               weath->echo_color = AT_YELLOW;
            }
         }
         else if( tindex > 1 && temp - dT <= -weath_unit )
         {
            const char *echo_strings[4] = {
               "The sky rumbles with thunder as the snow changes to rain.\r\n",
               "The sky rumbles with thunder as the snow changes to rain.\r\n",
               "The falling snow turns to freezing rain amidst flashes of lightning.\r\n",
               "The falling snow begins to melt as thunder crashes overhead.\r\n"
            };
            weath->echo = echo_strings[n];
            weath->echo_color = AT_WHITE;
         }
         else if( tindex < 2 && temp - dT > -weath_unit )
         {
            const char *echo_strings[4] = {
               "The lightning stops as the rainstorm becomes a blinding blizzard.\r\n",
               "The lightning stops as the rainstorm becomes a blinding blizzard.\r\n",
               "The thunder dies off as the pounding rain turns to heavy snow.\r\n",
               "The cold rain turns to snow and the lightning stops.\r\n"
            };
            weath->echo = echo_strings[n];
            weath->echo_color = AT_CYAN;
         }
         break;

      default:
         bug( "%s: invalid precip index", __func__ );
         weath->precip = 0;
         break;
   }
}

/*
 * get echo messages according to time changes...
 * some echoes depend upon the weather so an echo must be
 * found for each area
 * Last Modified: August 10, 1997
 * Fireblade
 */
void get_time_echo( weather_data * weath )
{
   int n = number_bits( 2 );
   int pindex = ( weath->precip + 3 * weath_unit - 1 ) / weath_unit;
   weath->echo.clear(  );
   weath->echo_color = AT_GREY;

   if( time_info.hour == sysdata->hourdaybegin )
   {
      const char *echo_strings[4] = {
         "The day has begun.\r\n",
         "The day has begun.\r\n",
         "The sky slowly begins to glow.\r\n",
         "The sun slowly embarks upon a new day.\r\n"
      };
      time_info.sunlight = SUN_RISE;
      weath->echo = echo_strings[n];
      weath->echo_color = AT_YELLOW;
   }

   if( time_info.hour == sysdata->hoursunrise )
   {
      const char *echo_strings[4] = {
         "The sun rises in the east.\r\n",
         "The sun rises in the east.\r\n",
         "The hazy sun rises over the horizon.\r\n",
         "Day breaks as the sun lifts into the sky.\r\n"
      };
      time_info.sunlight = SUN_LIGHT;
      weath->echo = echo_strings[n];
      weath->echo_color = AT_ORANGE;
   }

   if( time_info.hour == sysdata->hournoon )
   {
      if( pindex > 0 )
      {
         weath->echo = "It's noon.\r\n";
      }
      else
      {
         const char *echo_strings[2] = {
            "The intensity of the sun heralds the noon hour.\r\n",
            "The sun's bright rays beat down upon your shoulders.\r\n"
         };
         weath->echo = echo_strings[n % 2];
      }
      time_info.sunlight = SUN_LIGHT;
      weath->echo_color = AT_WHITE;
   }

   if( time_info.hour == sysdata->hoursunset )
   {
      const char *echo_strings[4] = {
         "The sun slowly disappears in the west.\r\n",
         "The reddish sun sets past the horizon.\r\n",
         "The sky turns a reddish orange as the sun ends its journey.\r\n",
         "The sun's radiance dims as it sinks in the sky.\r\n"
      };
      time_info.sunlight = SUN_SET;
      weath->echo = echo_strings[n];
      weath->echo_color = AT_RED;
   }

   if( time_info.hour == sysdata->hournightbegin )
   {
      if( pindex > 0 )
      {
         const char *echo_strings[2] = {
            "The night begins.\r\n",
            "Twilight descends around you.\r\n"
         };
         weath->echo = echo_strings[n % 2];
      }
      else
      {
         const char *echo_strings[2] = {
            "The moon's gentle glow diffuses through the night sky.\r\n",
            "The night sky gleams with glittering starlight.\r\n"
         };
         weath->echo = echo_strings[n % 2];
      }
      time_info.sunlight = SUN_DARK;
      weath->echo_color = AT_DBLUE;
   }
}

/*
 * function updates weather for each area
 * Last Modified: July 31, 1997
 * Fireblade
 */
void weather_update(  )
{
   list < area_data * >::iterator ar;
   int limit = 3 * weath_unit;

   for( ar = arealist.begin(  ); ar != arealist.end(  ); ++ar )
   {
      area_data *area = *ar;

      /*
       * Apply vectors to fields 
       */
      area->weather->temp += area->weather->temp_vector;
      area->weather->precip += area->weather->precip_vector;
      area->weather->wind += area->weather->wind_vector;

      /*
       * Make sure they are within the proper range 
       */
      area->weather->temp = URANGE( -limit, area->weather->temp, limit );
      area->weather->precip = URANGE( -limit, area->weather->precip, limit );
      area->weather->wind = URANGE( -limit, area->weather->wind, limit );

      /*
       * get an appropriate echo for the area 
       */
      get_weather_echo( area->weather );
   }

   for( ar = arealist.begin(  ); ar != arealist.end(  ); ++ar )
   {
      area_data *area = *ar;

      adjust_vectors( area->weather );
   }

   /*
    * display the echo strings to the appropriate players 
    */
   list < descriptor_data * >::iterator ds;
   for( ds = dlist.begin(  ); ds != dlist.end(  ); ++ds )
   {
      descriptor_data *d = *ds;

      if( d->connected == CON_PLAYING && d->character->IS_OUTSIDE(  ) && !INDOOR_SECTOR( d->character->in_room->sector_type ) && d->character->IS_AWAKE(  ) )
         /*
          * Changed to use INDOOR_SECTOR macro - Samson 9-27-98 
          */
      {
         weather_data *weath = d->character->in_room->area->weather;

         if( weath->echo.empty(  ) )
            continue;
         d->character->set_color( weath->echo_color );
         d->character->print( weath->echo );
      }
   }
}

/*
 * update the time
 */
void time_update( void )
{
   ++time_info.hour;

   if( time_info.hour == 1 )
      update_month_trigger = false;

   if( time_info.hour == sysdata->hourdaybegin || time_info.hour == sysdata->hoursunrise
       || time_info.hour == sysdata->hournoon || time_info.hour == sysdata->hoursunset || time_info.hour == sysdata->hournightbegin )
   {
      list < area_data * >::iterator ar;

      for( ar = arealist.begin(  ); ar != arealist.end(  ); ++ar )
      {
         area_data *area = *ar;

         get_time_echo( area->weather );
      }

      list < descriptor_data * >::iterator ds;
      for( ds = dlist.begin(  ); ds != dlist.end(  ); ++ds )
      {
         descriptor_data *d = *ds;

         if( d->connected == CON_PLAYING && d->character->IS_OUTSIDE(  ) && !INDOOR_SECTOR( d->character->in_room->sector_type ) && d->character->IS_AWAKE(  ) )
         {
            weather_data *weath = d->character->in_room->area->weather;

            if( weath->echo.empty(  ) )
               continue;
            d->character->set_color( weath->echo_color );
            d->character->print( weath->echo );
         }
      }
   }

   if( time_info.hour == sysdata->hourmidnight )
   {
      time_info.hour = 0;
      ++time_info.day;
      /*
       * Sweep old crap from auction houses on daily basis - Samson 11-1-99 
       */
      clean_auctions(  );
   }

   if( time_info.day >= sysdata->dayspermonth )
   {
      time_info.day = 0;
      ++time_info.month;
      update_month_trigger = true;
   }

   if( time_info.month >= sysdata->monthsperyear )
   {
      time_info.month = 0;
      ++time_info.year;
   }
   calc_season(  );  /* Samson 5-6-99 */
   /*
    * Save game world time - Samson 1-21-99 
    */
   save_timedata(  );
}

void subtract_times( struct timeval *endtime, struct timeval *starttime )
{
   endtime->tv_sec -= starttime->tv_sec;
   endtime->tv_usec -= starttime->tv_usec;
   while( endtime->tv_usec < 0 )
   {
      endtime->tv_usec += 1000000;
      --endtime->tv_sec;
   }
}

/*
 * Handle all kinds of updates.
 * Called once per pulse from game loop.
 * Random times to defeat tick-timing clients and players.
 */
void update_handler( void )
{
   static int pulse_environment;
   static int pulse_mobile;
   static int pulse_violence;
   static int pulse_point;
   static int pulse_second;
   static int pulse_time;
   struct timeval sttime;
   struct timeval entime;

   if( timechar )
   {
      timechar->set_color( AT_PLAIN );
      timechar->print( "Starting update timer.\r\n" );
      gettimeofday( &sttime, nullptr );
   }

   if( --pulse_mobile <= 0 )
   {
      pulse_mobile = sysdata->pulsemobile;
      mobile_update(  );
   }

   // Overland environmental stuff. Does not advance when no players are on.
   if( sysdata->playersonline > 0 )
   {
      if( --pulse_environment <= 0 )
      {
         pulse_environment = sysdata->pulseenvironment;
         environment_update(  );
      }
   }

   /*
    * If this damn thing hadn't been used as a base value in about 50 other places..... 
    */
   if( --pulse_violence <= 0 )
      pulse_violence = sysdata->pulseviolence;

   // Time. Does not pass when no players are on.
   if( sysdata->playersonline > 0 )
   {
      if( --pulse_time <= 0 )
      {
         pulse_time = sysdata->pulsecalendar;

         time_update(  );
         weather_update(  );
         char_calendar_update(  );
      }
   }

   if( --pulse_point <= 0 )
   {
      pulse_point = number_range( ( int )( sysdata->pulsetick * 0.65 ), ( int )( sysdata->pulsetick * 1.35 ) );

      auth_update(  );  /* Gorog */
      char_update(  );
      obj_update(  );
   }

   if( --pulse_second <= 0 )
   {
      pulse_second = sysdata->pulsepersec;
      char_check(  );
   }

   mpsleep_update(  );  /* Check for sleeping mud progs  -rkb */
   tele_update(  );
   aggr_update(  );
   mob_act_update(  );
   obj_act_update(  );
   room_act_update(  );

   clean_obj_queue(  ); /* dispose of extracted objects */
   clean_char_queue(  );   /* dispose of dead mobs/quitting chars */

   if( timechar )
   {
      gettimeofday( &entime, nullptr );
      timechar->set_color( AT_PLAIN );
      timechar->print( "Update timing complete.\r\n" );
      subtract_times( &entime, &sttime );
      timechar->printf( "Timing took %ld.%ld seconds.\r\n", entime.tv_sec, entime.tv_usec );
      timechar = nullptr;
   }
}
