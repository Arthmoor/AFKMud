/****************************************************************************
 *                   ^     +----- |  / ^     ^ |     | +-\                  *
 *                  / \    |      | /  |\   /| |     | |  \                 *
 *                 /   \   +---   |<   | \ / | |     | |  |                 *
 *                /-----\  |      | \  |  v  | |     | |  /                 *
 *               /       \ |      |  \ |     | +-----+ +-/                  *
 ****************************************************************************
 * AFKMud Copyright 1997-2012 by Roger Libiez (Samson),                     *
 * Levi Beckerson (Whir), Michael Ward (Tarl), Erik Wolfe (Dwip),           *
 * Cameron Carroll (Cam), Cyberfox, Karangi, Rathian, Raine,                *
 * Xorith, and Adjani.                                                      *
 * All Rights Reserved.                                                     *
 *                                                                          *
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
 *                             Archery Module                               *
 ****************************************************************************/

/*
Bowfire Code v1.0 (c)1997-99 Feudal Realms 

This is my bow and arrow code that I wrote based off of a thrown weapon code
that I had from long ago (if you wrote it let me know so I can give you
credit for that part, I do not have it anymore), it's a little more complex 
than I had originally wanted, but well, it works.  There are a couple things 
that are involved which if you don't want to use, remove them, that simple.  
One of them are the use of the "lodged" wearbits.  The code is designed to 
lodge an arrow in a victim, not just do damage to them once, and there are three
places it can lodge, etc.  Included are all of the pieces of code for quivers,
arrows, drawing arrows, dislodging, etc.  Use whatever of this code that you
want, if you have a credits page, add me on there, and please drop me an email
at mustang@roscoe.mudservices.com so I know its out there somewhere being used.

Any bugs that people find, if you email me, I will fix, unless it's something
from a modification that you made, and if that's the case, I will probably
help you figure out what's up with it if I can.  My code is not stock, and I
tried to add in everything that people might need to add in this feature.

Thanks,
Tch

===============================================================================
Features in v1.0

- Bowfire from adjacent rooms at targets
- Arrows lodge in various body parts (leg, arm, and chest)
- Quiver and arrow new item types
- Shoulder wearbit used for quivers (if you want it)
- Dislodging arrows does damage
- OLC support for arrows and quivers

===============================================================================

Add in a bow weapon type with the other ones on the list.(if you don't know 
how to do this, grep/search for sword and add in bow in the respective places)
		
=============================================================================== 

Bowfire code ported for Smaug 1.4a by Samson.
Combined with portions of the Smaug 1.4a archery code.
Additional portions by Samson.
*/

#include "mud.h"
#include "objindex.h"
#include "roomindex.h"

int weapon_prof_bonus_check( char_data *, obj_data *, int & );
SPELLF( spell_attack );
int calc_thac0( char_data *, char_data *, int );
double ris_damage( char_data *, double, int );
bool is_safe( char_data *, char_data * );
bool check_illegal_pk( char_data *, char_data * );
void check_attacker( char_data *, char_data * );
void wear_obj( char_data *, obj_data *, bool, int );
bool can_use_skill( char_data *, int, int );

obj_data *find_quiver( char_data * ch )
{
   list < obj_data * >::iterator iobj;

   for( iobj = ch->carrying.begin(  ); iobj != ch->carrying.end(  ); ++iobj )
   {
      obj_data *obj = *iobj;

      if( ch->can_see_obj( obj, false ) )
      {
         if( obj->item_type == ITEM_QUIVER && !IS_SET( obj->value[1], CONT_CLOSED ) )
            return obj;
      }
   }
   return NULL;
}

obj_data *find_projectile( char_data * ch, obj_data * quiver )
{
   list < obj_data * >::iterator iobj;

   for( iobj = quiver->contents.begin(  ); iobj != quiver->contents.end(  ); ++iobj )
   {
      obj_data *obj = *iobj;

      if( ch->can_see_obj( obj, false ) )
      {
         if( obj->item_type == ITEM_PROJECTILE )
            return obj;
      }
   }
   return NULL;
}

/* Bowfire code -- used to draw an arrow from a quiver */
CMDF( do_draw )
{
   obj_data *bow, *arrow, *quiver;
   int hand_count = 0;

   if( !( bow = ch->get_eq( WEAR_MISSILE_WIELD ) ) )
   {
      ch->print( "You are not wielding a missile weapon!\r\n" );
      return;
   }

   if( !( quiver = find_quiver( ch ) ) )
   {
      ch->print( "You aren't wearing a quiver where you can get to it!\r\n" );
      return;
   }

   if( ch->get_eq( WEAR_LIGHT ) != NULL )
      ++hand_count;
   if( ch->get_eq( WEAR_SHIELD ) != NULL )
      ++hand_count;
   if( ch->get_eq( WEAR_HOLD ) != NULL )
      ++hand_count;
   if( ch->get_eq( WEAR_WIELD ) != NULL )
      ++hand_count;
   if( hand_count > 1 )
   {
      ch->print( "You need a free hand to draw with.\r\n" );
      return;
   }

   if( ch->get_eq( WEAR_HOLD ) != NULL )
   {
      ch->print( "Your hand is not empty!\r\n" );
      return;
   }

   if( !( arrow = find_projectile( ch, quiver ) ) )
   {
      ch->print( "Your quiver is empty!!\r\n" );
      return;
   }

   /*
    * Forgot about this little important bit 
    */
   arrow->separate(  );

   /*
    * OOPS - had these backwards, this is now correct :) 
    */
   if( bow->value[5] != arrow->value[4] )
   {
      ch->print( "You drew the wrong projectile type for this weapon!\r\n" );
      arrow->from_obj(  );
      arrow->to_char( ch );
      return;
   }

   ch->WAIT_STATE( sysdata->pulseviolence );
   act_printf( AT_ACTION, ch, quiver, NULL, TO_ROOM, "$n draws %s from $p.", arrow->short_descr );
   act_printf( AT_ACTION, ch, quiver, NULL, TO_CHAR, "You draw %s from $p.", arrow->short_descr );
   arrow->from_obj(  );
   arrow->to_char( ch );

   /*
    * Changed the last arg here because for some whacked reason it won't accept ITEM_HOLD 
    * Just make sure all your projectiles have the 'hold' wearflag on them 
    */
   wear_obj( ch, arrow, true, -1 );
}

/* Bowfire code -- Used to dislodge an arrow already lodged */
CMDF( do_dislodge )
{
   obj_data *arrow = NULL;
   double dam = 0;

   if( argument.empty(  ) )   /* empty */
   {
      ch->print( "Dislodge what?\r\n" );
      return;
   }

   if( ch->get_eq( WEAR_LODGE_RIB ) != NULL )
   {
      arrow = ch->get_eq( WEAR_LODGE_RIB );
      act( AT_CARNAGE, "With a wrenching pull, you dislodge $p from your chest.", ch, arrow, NULL, TO_CHAR );
      act( AT_CARNAGE, "$N winces in pain as $e dislodges $p from $s chest.", ch, arrow, NULL, TO_ROOM );
      ch->unequip( arrow );
      arrow->wear_flags.reset( ITEM_LODGE_RIB );
      arrow->extra_flags.reset( ITEM_LODGED );
      dam = number_range( ( 3 * arrow->value[1] ), ( 3 * arrow->value[2] ) );
      damage( ch, ch, dam, TYPE_UNDEFINED );
   }
   else if( ch->get_eq( WEAR_LODGE_ARM ) != NULL )
   {
      arrow = ch->get_eq( WEAR_LODGE_ARM );
      act( AT_CARNAGE, "With a tug you dislodge $p from your arm.", ch, arrow, NULL, TO_CHAR );
      act( AT_CARNAGE, "$N winces in pain as $e dislodges $p from $s arm.", ch, arrow, NULL, TO_ROOM );
      ch->unequip( arrow );
      arrow->wear_flags.reset( ITEM_LODGE_ARM );
      arrow->extra_flags.reset( ITEM_LODGED );
      dam = number_range( ( 3 * arrow->value[1] ), ( 2 * arrow->value[2] ) );
      damage( ch, ch, dam, TYPE_UNDEFINED );
   }
   else if( ch->get_eq( WEAR_LODGE_LEG ) != NULL )
   {
      arrow = ch->get_eq( WEAR_LODGE_LEG );
      act( AT_CARNAGE, "With a tug you dislodge $p from your leg.", ch, arrow, NULL, TO_CHAR );
      act( AT_CARNAGE, "$N winces in pain as $e dislodges $p from $s leg.", ch, arrow, NULL, TO_ROOM );
      ch->unequip( arrow );
      arrow->wear_flags.reset( ITEM_LODGE_LEG );
      arrow->extra_flags.reset( ITEM_LODGED );
      dam = number_range( ( 2 * arrow->value[1] ), ( 2 * arrow->value[2] ) );
      damage( ch, ch, dam, TYPE_UNDEFINED );
   }
   else
      ch->print( "You have nothing lodged in your body.\r\n" );
}

/*
 * Hit one guy with a projectile.
 * Handles use of missile weapons (wield = missile weapon)
 * or thrown items/weapons
 */
ch_ret projectile_hit( char_data * ch, char_data * victim, obj_data * wield, obj_data * projectile, short dist )
{
   int vic_ac, thac0, plusris, diceroll, prof_bonus, prof_gsn = -1, proj_bonus, dt, pchance;
   double dam = 0;
   ch_ret retcode;

   if( !projectile )
      return rNONE;

   if( projectile->item_type == ITEM_PROJECTILE || projectile->item_type == ITEM_WEAPON )
   {
      dt = TYPE_HIT + projectile->value[3];
      if( wield )
         proj_bonus = number_range( wield->value[1], wield->value[2] );
      else
         proj_bonus = 0;
   }
   else
   {
      dt = TYPE_UNDEFINED;
      proj_bonus = 0;
   }

   /*
    * Can't beat a dead char!
    */
   if( victim->position == POS_DEAD || victim->char_died(  ) )
   {
      projectile->extract(  );
      return rVICT_DIED;
   }

   if( wield )
      prof_bonus = weapon_prof_bonus_check( ch, wield, prof_gsn );
   else
      prof_bonus = 0;

   if( dt == TYPE_UNDEFINED )
   {
      dt = TYPE_HIT;
      if( wield && wield->item_type == ITEM_MISSILE_WEAPON )
         dt += wield->value[3];
   }

   thac0 = calc_thac0( ch, victim, dist );

   vic_ac = UMAX( -19, ( int )( victim->GET_AC(  ) / 10 ) );

   /*
    * if you can't see what's coming... 
    */
   if( !victim->can_see_obj( projectile, false ) )
      vic_ac += 1;
   if( !ch->can_see( victim, false ) )
      vic_ac -= 4;

   /*
    * Weapon proficiency bonus 
    */
   vic_ac += prof_bonus;

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

      /*
       * Do something with the projectile 
       */
      if( number_percent(  ) < 50 )
         projectile->extract(  );
      else
      {
         if( projectile->carried_by )
            projectile->from_char(  );
         projectile->to_room( victim->in_room, victim );
      }
      damage( ch, victim, 0, dt );
      return rNONE;
   }

   /*
    * Hit.
    * Calc damage.
    */

   pchance = number_range( 1, 10 );

   switch ( pchance )
   {
      case 1:
      case 2:
      case 3: /* Hit in the arm */
         dam = number_range( projectile->value[1], projectile->value[2] ) + proj_bonus;
         break;

      case 4:
      case 5:
      case 6: /* Hit in the leg */
         dam = number_range( ( 2 * projectile->value[1] ), ( 2 * projectile->value[2] ) ) + proj_bonus;
         break;

      default:
      case 7:
      case 8:
      case 9:
      case 10:   /* Hit in the chest */
         dam = number_range( ( 3 * projectile->value[1] ), ( 3 * projectile->value[2] ) ) + proj_bonus;
         break;
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

   if( !ch->isnpc(  ) && ch->pcdata->learned[gsn_enhanced_damage] > 0 && number_percent(  ) < ch->pcdata->learned[gsn_enhanced_damage] )
      dam += ( int )( dam * ch->LEARNED( gsn_enhanced_damage ) / 120 );
   else
      ch->learn_from_failure( gsn_enhanced_damage );

   if( !victim->IS_AWAKE(  ) )
      dam *= 2;

   if( dam <= 0 )
      dam = 1;

   plusris = 0;

   if( projectile->extra_flags.test( ITEM_MAGIC ) )
      dam = ris_damage( victim, dam, RIS_MAGIC );
   else
      dam = ris_damage( victim, dam, RIS_NONMAGIC );

   /*
    * Handle PLUS1 - PLUS6 ris bits vs. weapon hitroll   -Thoric
    */
   if( wield )
      plusris = wield->hitroll(  );

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
       * find high ris 
       *//*
       * FIXME: Absorb handling needs to be included
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
            act( AT_HIT, skill->imm_char, ch, NULL, victim, TO_CHAR );
            found = true;
         }
         if( skill->imm_vict && skill->imm_vict[0] != '\0' )
         {
            act( AT_HITME, skill->imm_vict, ch, NULL, victim, TO_VICT );
            found = true;
         }
         if( skill->imm_room && skill->imm_room[0] != '\0' )
         {
            act( AT_ACTION, skill->imm_room, ch, NULL, victim, TO_NOTVICT );
            found = true;
         }
         if( found )
         {
            if( number_percent(  ) < 50 )
               projectile->extract(  );
            else
            {
               if( projectile->carried_by )
                  projectile->from_char(  );
               projectile->to_room( victim->in_room, victim );
            }
            return rNONE;
         }
      }
      dam = 0;
   }
   if( ( retcode = damage( ch, victim, dam, dt ) ) != rNONE )
   {
      if( projectile->value[5] == PROJ_STONE )
         projectile->extract(  );
      else
      {
         if( victim->char_died(  ) )
         {
            projectile->extract(  );
            return rVICT_DIED;
         }
         projectile->from_char(  );
         projectile->to_char( victim );
         projectile->extra_flags.set( ITEM_LODGED );

         switch ( pchance )
         {
            case 1:
            case 2:
            case 3: /* Hit in the arm */
               projectile->wear_flags.set( ITEM_LODGE_ARM );
               wear_obj( victim, projectile, true, get_wflag( "lodge_arm" ) );
               break;

            case 4:
            case 5:
            case 6: /* Hit in the leg */
               projectile->wear_flags.set( ITEM_LODGE_LEG );
               wear_obj( victim, projectile, true, get_wflag( "lodge_leg" ) );
               break;

            default:
            case 7:
            case 8:
            case 9:
            case 10:   /* Hit in the chest */
               projectile->wear_flags.set( ITEM_LODGE_RIB );
               wear_obj( victim, projectile, true, get_wflag( "lodge_rib" ) );
               break;
         }
      }
      return retcode;
   }
   if( ch->char_died(  ) )
   {
      projectile->extract(  );
      return rCHAR_DIED;
   }
   if( victim->char_died(  ) )
   {
      projectile->extract(  );
      return rVICT_DIED;
   }

   retcode = rNONE;
   if( dam == 0 )
   {
      if( number_percent(  ) < 50 )
         projectile->extract(  );
      else
      {
         if( projectile->carried_by )
            projectile->from_char(  );
         projectile->to_room( victim->in_room, victim );
      }
      return retcode;
   }

   /*
    * weapon spells -Thoric 
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
      {
         projectile->extract(  );
         return retcode;
      }

      for( paf = wield->affects.begin(  ); paf != wield->affects.end(  ); ++paf )
      {
         affect_data *af = *paf;

         if( af->location == APPLY_WEAPONSPELL && IS_VALID_SN( af->modifier ) && skill_table[af->modifier]->spell_fun )
            retcode = ( *skill_table[af->modifier]->spell_fun ) ( af->modifier, 7, ch, victim );
      }
      if( retcode != rNONE || ch->char_died(  ) || victim->char_died(  ) )
      {
         projectile->extract(  );
         return retcode;
      }
   }
   projectile->extract(  );
   return retcode;
}

/*
 * Perform the actual attack on a victim			-Thoric
 */
ch_ret ranged_got_target( char_data * ch, char_data * victim, obj_data * weapon, obj_data * projectile, short dist, short dt, const char *stxt, short color )
{
   /*
    * added wtype for check to determine skill used for ranged attacks - Grimm 
    */
   short wtype = 0;

   if( ch->in_room->flags.test( ROOM_SAFE ) )
   {
      /*
       * safe room, bubye projectile 
       */
      if( projectile )
      {
         ch->printf( "Your %s is blasted from existance by a godly presense.", projectile->myobj(  ).c_str(  ) );
         act( color, "A godly presence smites $p!", ch, projectile, NULL, TO_ROOM );
         projectile->extract(  );
      }
      else
      {
         ch->printf( "Your %s is blasted from existance by a godly presense.", stxt );
         act( color, "A godly presence smites $t!", ch, aoran( stxt ), NULL, TO_ROOM );
      }
      return rNONE;
   }

/*
   if( victim->has_actflag( ACT_SENTINEL ) && ch->in_room != victim->in_room )
   {
	*
	 * letsee, if they're high enough.. attack back with fireballs
	 * long distance or maybe some minions... go herne! heh..
	 *
	 * For now, just always miss if not in same room  -Thoric
	 *

      if( projectile )
      {
           * check dam type of projectile to determine skill to use - Grimm *
	   switch( projectile->value[4] )
	   {
		case PROJ_BOLT:
		case PROJ_ARROW:
               ch->learn_from_failure( gsn_archery );
		   break;
		 
		case PROJ_DART:
               ch->learn_from_failure( gsn_blowguns );
		   break;

		case PROJ_STONE:
               ch->learn_from_failure( gsn_slings );
		   break;
	   }
            
          * 50% chance of projectile getting lost *
         if( number_percent() < 50 )
            projectile->extract();
         else
         {
            if( projectile->in_obj )
               projectile->from_obj();
            if( projectile->carried_by )
               projectile->from_char();
            projectile->to_room( victim->in_room, victim );
         }
      }
	return damage( ch, victim, 0, dt );
   }
*/

   /*
    * check type of projectile to determine value of wtype 
    * * wtype points to same "short" as the skill assigned to that
    * * range by the code and as such the proper skill will be used. 
    * * Grimm 
    */
   switch ( projectile->value[4] )
   {
      default:
      case PROJ_BOLT:
      case PROJ_ARROW:
         wtype = gsn_archery;
         break;

      case PROJ_DART:
         wtype = gsn_blowguns;
         break;

      case PROJ_STONE:
         wtype = gsn_slings;
         break;
   }

   if( number_percent(  ) > 50 || ( projectile && weapon && can_use_skill( ch, number_percent(  ), wtype ) ) )
   {
      if( victim->isnpc(  ) ) /* This way the poor sap can hunt its attacker */
         victim->unset_actflag( ACT_SENTINEL );
      if( projectile )
         global_retcode = projectile_hit( ch, victim, weapon, projectile, dist );
      else
         global_retcode = spell_attack( dt, ch->level, ch, victim );
   }
   else
   {
      switch ( projectile->value[4] )
      {
         default:
         case PROJ_BOLT:
         case PROJ_ARROW:
            ch->learn_from_failure( gsn_archery );
            break;

         case PROJ_DART:
            ch->learn_from_failure( gsn_blowguns );
            break;

         case PROJ_STONE:
            ch->learn_from_failure( gsn_slings );
            break;
      }
      global_retcode = damage( ch, victim, 0, dt );

      if( projectile )
      {
         /*
          * 50% chance of getting lost 
          */
         if( number_percent(  ) < 50 )
            projectile->extract(  );
         else
         {
            if( projectile->carried_by )
               projectile->from_char(  );
            projectile->to_room( victim->in_room, victim );
         }
      }
   }
   return global_retcode;
}

/*
 * Basically the same guts as do_scan() from above (please keep them in
 * sync) used to find the victim we're firing at.	-Thoric
 */
char_data *scan_for_vic( char_data * ch, exit_data * pexit, const string & name )
{
   char_data *victim;
   room_index *was_in_room;
   short dist, dir;
   short max_dist = 8;

   if( ch->has_aflag( AFF_BLIND ) || !pexit )
      return NULL;

   was_in_room = ch->in_room;

   if( ch->level < 50 )
      --max_dist;
   if( ch->level < 40 )
      --max_dist;
   if( ch->level < 30 )
      --max_dist;

   for( dist = 1; dist <= max_dist; )
   {
      if( IS_EXIT_FLAG( pexit, EX_CLOSED ) )
         break;

      if( pexit->to_room->is_private(  ) && ch->level < sysdata->level_override_private )
         break;

      ch->from_room(  );
      if( !ch->to_room( pexit->to_room ) )
         log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );

      if( ( victim = ch->get_char_room( name ) ) != NULL )
      {
         ch->from_room(  );
         if( !ch->to_room( was_in_room ) )
            log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
         return victim;
      }

      switch ( ch->in_room->sector_type )
      {
         default:
            ++dist;
            break;
         case SECT_AIR:
            if( number_percent(  ) < 80 )
               ++dist;
            break;
         case SECT_INDOORS:
         case SECT_FIELD:
         case SECT_UNDERGROUND:
            ++dist;
            break;
         case SECT_FOREST:
         case SECT_CITY:
         case SECT_DESERT:
         case SECT_HILLS:
            dist += 2;
            break;
         case SECT_WATER_SWIM:
         case SECT_WATER_NOSWIM:
         case SECT_RIVER:
            dist += 3;
            break;
         case SECT_MOUNTAIN:
         case SECT_UNDERWATER:
         case SECT_OCEANFLOOR:
            dist += 4;
            break;
      }

      if( dist >= max_dist )
         break;

      dir = pexit->vdir;
      if( !( pexit = ch->in_room->get_exit( dir ) ) )
         break;
   }

   ch->from_room(  );
   if( !ch->to_room( was_in_room ) )
      log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );

   return NULL;
}

/*
 * Generic use ranged attack function			-Thoric & Tricops
 */
ch_ret ranged_attack( char_data * ch, string argument, obj_data * weapon, obj_data * projectile, short dt, short range )
{
   string arg, arg1, temp;

   if( !argument.empty(  ) && argument[0] == '\'' )
   {
      one_argument( argument, temp );
      argument = temp;
   }

   argument = one_argument( argument, arg );
   argument = one_argument( argument, arg1 );

   if( arg.empty(  ) )
   {
      ch->print( "Where?  At who?\r\n" );
      return rNONE;
   }

   /*
    * get an exit or a victim 
    */
   short dir = -1;
   exit_data *pexit;
   char_data *victim = NULL;
   if( !( pexit = find_door( ch, arg, true ) ) )
   {
      if( !( victim = ch->get_char_room( arg ) ) )
      {
         ch->print( "Aim in what direction?\r\n" );
         return rNONE;
      }
      else
      {
         if( ch->who_fighting(  ) == victim )
         {
            ch->print( "They are too close to release that type of attack!\r\n" );
            return rNONE;
         }
      }
   }
   else
      dir = pexit->vdir;

   /*
    * check for ranged attacks from private rooms, etc 
    */
   if( !victim )
   {
      if( ch->in_room->flags.test( ROOM_PRIVATE ) || ch->in_room->flags.test( ROOM_SOLITARY ) )
      {
         ch->print( "You cannot perform a ranged attack from a private room.\r\n" );
         return rNONE;
      }
      if( ch->in_room->tunnel > 0 )
      {
         if( ( int )ch->in_room->people.size(  ) >= ch->in_room->tunnel )
         {
            ch->print( "This room is too cramped to perform such an attack.\r\n" );
            return rNONE;
         }
      }
   }

   skill_type *skill = NULL;
   if( IS_VALID_SN( dt ) )
      skill = skill_table[dt];

   if( pexit && !pexit->to_room )
   {
      ch->print( "Are you expecting to fire through a wall!?\r\n" );
      return rNONE;
   }

   /*
    * Check for obstruction 
    */
   if( pexit && IS_EXIT_FLAG( pexit, EX_CLOSED ) )
   {
      if( IS_EXIT_FLAG( pexit, EX_SECRET ) || IS_EXIT_FLAG( pexit, EX_DIG ) )
         ch->print( "Are you expecting to fire through a wall!?\r\n" );
      else
         ch->print( "Are you expecting to fire through a door!?\r\n" );
      return rNONE;
   }

   /*
    * Keeps em from firing through a wall but can still fire through an arrow slit or window, Marcus 
    */
   if( pexit )
   {
      if( ( IS_EXIT_FLAG( pexit, EX_FORTIFIED ) || IS_EXIT_FLAG( pexit, EX_HEAVY )
            || IS_EXIT_FLAG( pexit, EX_MEDIUM ) || IS_EXIT_FLAG( pexit, EX_LIGHT ) || IS_EXIT_FLAG( pexit, EX_CRUMBLING ) )
          && !IS_EXIT_FLAG( pexit, EX_WINDOW ) && !IS_EXIT_FLAG( pexit, EX_ASLIT ) )
      {
         ch->print( "Are you expecting to fire through a wall!?\r\n" );
         return rNONE;
      }
   }

   char_data *vch = NULL;
   if( pexit && !arg1.empty(  ) )
   {
      if( !( vch = scan_for_vic( ch, pexit, arg1 ) ) )
      {
         ch->print( "You cannot see your target.\r\n" );
         return rNONE;
      }

      /*
       * don't allow attacks on mobs that are in a no-missile room --Shaddai 
       */
      if( vch->in_room->flags.test( ROOM_NOMISSILE ) )
      {
         ch->print( "You can't get a clean shot off.\r\n" );
         return rNONE;
      }

      /*
       * can't properly target someone heavily in battle 
       */
      if( vch->num_fighting > MAX_FIGHT )
      {
         ch->print( "There is too much activity there for you to get a clear shot.\r\n" );
         return rNONE;
      }
   }

   if( vch )
   {
      if( !vch->CAN_PKILL(  ) || !ch->CAN_PKILL(  ) )
      {
         ch->print( "You can't do that!\r\n" );
         return rNONE;
      }
      if( vch && is_safe( ch, vch ) )
         return rNONE;
   }

   room_index *was_in_room = ch->in_room;
   const char *stxt = "burst of energy";
   if( projectile )
   {
      projectile->separate(  );
      if( pexit )
      {
         if( weapon )
         {
            act( AT_GREY, "You fire $p $T.", ch, projectile, dir_name[dir], TO_CHAR );
            act( AT_GREY, "$n fires $p $T.", ch, projectile, dir_name[dir], TO_ROOM );
         }
         else
         {
            act( AT_GREY, "You throw $p $T.", ch, projectile, dir_name[dir], TO_CHAR );
            act( AT_GREY, "$n throw $p $T.", ch, projectile, dir_name[dir], TO_ROOM );
         }
      }
      else
      {
         if( weapon )
         {
            act( AT_GREY, "You fire $p at $N.", ch, projectile, victim, TO_CHAR );
            act( AT_GREY, "$n fires $p at $N.", ch, projectile, victim, TO_NOTVICT );
            act( AT_GREY, "$n fires $p at you!", ch, projectile, victim, TO_VICT );
         }
         else
         {
            act( AT_GREY, "You throw $p at $N.", ch, projectile, victim, TO_CHAR );
            act( AT_GREY, "$n throws $p at $N.", ch, projectile, victim, TO_NOTVICT );
            act( AT_GREY, "$n throws $p at you!", ch, projectile, victim, TO_VICT );
         }
      }
   }
   else if( skill )
   {
      if( skill->noun_damage && skill->noun_damage[0] != '\0' )
         stxt = skill->noun_damage;
      else
         stxt = skill->name;
      /*
       * a plain "spell" flying around seems boring 
       */
      if( !str_cmp( stxt, "spell" ) )
         stxt = "magical burst of energy";
      if( skill->type == SKILL_SPELL )
      {
         if( pexit )
         {
            act( AT_MAGIC, "You release $t $T.", ch, aoran( stxt ), dir_name[dir], TO_CHAR );
            act( AT_MAGIC, "$n releases $s $t $T.", ch, stxt, dir_name[dir], TO_ROOM );
         }
         else
         {
            act( AT_MAGIC, "You release $t at $N.", ch, aoran( stxt ), victim, TO_CHAR );
            act( AT_MAGIC, "$n releases $s $t at $N.", ch, stxt, victim, TO_NOTVICT );
            act( AT_MAGIC, "$n releases $s $t at you!", ch, stxt, victim, TO_VICT );
         }
      }
   }
   else
   {
      bug( "%s: no projectile, no skill dt %d", __FUNCTION__, dt );
      return rNONE;
   }

   /*
    * victim in same room 
    */
   if( victim )
   {
      check_illegal_pk( ch, victim );
      check_attacker( ch, victim );
      return ranged_got_target( ch, victim, weapon, projectile, 0, dt, stxt, AT_MAGIC );
   }

   /*
    * assign scanned victim 
    */
   victim = vch;

   /*
    * reverse direction text from move_char 
    */
   const char *dtxt = rev_exit( pexit->vdir );
   int dist = 0;

   while( dist <= range )
   {
      ch->from_room(  );
      if( !ch->to_room( pexit->to_room ) )
         log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );

      if( IS_EXIT_FLAG( pexit, EX_CLOSED ) )
      {
         /*
          * whadoyahknow, the door's closed 
          */
         if( projectile )
            ch->printf( "&wYou see your %s pierce a door in the distance to the %s.", projectile->myobj(  ).c_str(  ), dir_name[dir] );
         else
            ch->printf( "&wYou see your %s hit a door in the distance to the %s.", stxt, dir_name[dir] );
         if( projectile )
            act_printf( AT_GREY, ch, projectile, NULL, TO_ROOM, "$p flies in from %s and implants itself solidly in the %sern door.", dtxt, dir_name[dir] );
         else
            act_printf( AT_GREY, ch, NULL, NULL, TO_ROOM, "%s flies in from %s and implants itself solidly in the %sern door.", aoran( stxt ), dtxt, dir_name[dir] );
         break;
      }

      /*
       * no victim? pick a random one 
       */
      if( !victim )
      {
         list < char_data * >::iterator ich;
         for( ich = ch->in_room->people.begin(  ); ich != ch->in_room->people.end(  ); ++ich )
         {
            vch = *ich;

            if( ( ( ch->isnpc(  ) && !vch->isnpc(  ) ) || ( !ch->isnpc(  ) && vch->isnpc(  ) ) ) && number_bits( 1 ) == 0 )
            {
               victim = vch;
               break;
            }
         }
         if( victim && is_safe( ch, victim ) )
         {
            ch->from_room(  );
            if( !ch->to_room( was_in_room ) )
               log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
            return rNONE;
         }
      }

      /*
       * In the same room as our victim? 
       */
      if( victim && ch->in_room == victim->in_room )
      {
         if( projectile )
            act( AT_GREY, "$p flies in from $T.", ch, projectile, dtxt, TO_ROOM );
         else
            act( AT_GREY, "$t flies in from $T.", ch, aoran( stxt ), dtxt, TO_ROOM );

         /*
          * get back before the action starts 
          */
         ch->from_room(  );
         if( !ch->to_room( was_in_room ) )
            log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );

         check_illegal_pk( ch, victim );
         check_attacker( ch, victim );
         return ranged_got_target( ch, victim, weapon, projectile, dist, dt, stxt, AT_GREY );
      }

      if( dist == range )
      {
         if( projectile )
         {
            act( AT_GREY, "Your $t falls harmlessly to the ground to the $T.", ch, projectile->myobj(  ).c_str(  ), dir_name[dir], TO_CHAR );
            act( AT_GREY, "$p flies in from $T and falls harmlessly to the ground here.", ch, projectile, dtxt, TO_ROOM );
            if( projectile->in_obj )
               projectile->from_obj(  );
            if( projectile->carried_by )
               projectile->from_char(  );
            projectile->to_room( ch->in_room, ch );
         }
         else
         {
            act( AT_MAGIC, "Your $t fizzles out harmlessly to the $T.", ch, stxt, dir_name[dir], TO_CHAR );
            act( AT_MAGIC, "$t flies in from $T and fizzles out harmlessly.", ch, aoran( stxt ), dtxt, TO_ROOM );
         }
         break;
      }

      if( !( pexit = ch->in_room->get_exit( dir ) ) )
      {
         if( projectile )
         {
            act( AT_GREY, "Your $t hits a wall and bounces harmlessly to the ground to the $T.", ch, projectile->myobj(  ).c_str(  ), dir_name[dir], TO_CHAR );
            act( AT_GREY, "$p strikes the $Tsern wall and falls harmlessly to the ground.", ch, projectile, dir_name[dir], TO_ROOM );
            if( projectile->in_obj )
               projectile->from_obj(  );
            if( projectile->carried_by )
               projectile->from_char(  );
            projectile->to_room( ch->in_room, ch );
         }
         else
         {
            act( AT_MAGIC, "Your $t harmlessly hits a wall to the $T.", ch, stxt, dir_name[dir], TO_CHAR );
            act( AT_MAGIC, "$t strikes the $Tsern wall and falls harmlessly to the ground.", ch, aoran( stxt ), dir_name[dir], TO_ROOM );
         }
         break;
      }
      if( projectile )
         act( AT_GREY, "$p flies in from $T.", ch, projectile, dtxt, TO_ROOM );
      else
         act( AT_MAGIC, "$t flies in from $T.", ch, aoran( stxt ), dtxt, TO_ROOM );
      ++dist;
   }

   ch->from_room(  );
   if( !ch->to_room( was_in_room ) )
      log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );

   if( projectile->carried_by == ch )
      projectile->extract(  );
   return rNONE;
}

/* Bowfire code -- actual firing function */
CMDF( do_fire )
{
   string arg;
   char_data *victim = NULL;
   obj_data *arrow, *bow;
   short max_dist;

   if( !( bow = ch->get_eq( WEAR_MISSILE_WIELD ) ) )
   {
      ch->print( "But you are not wielding a missile weapon!!\r\n" );
      return;
   }

   one_argument( argument, arg );
   if( arg.empty(  ) && ch->fighting == NULL )
   {
      ch->print( "Fire at whom or what?\r\n" );
      return;
   }

   if( !str_cmp( arg, "none" ) || !str_cmp( arg, "self" ) || victim == ch )
   {
      ch->print( "How exactly did you plan on firing at yourself?\r\n" );
      return;
   }

   if( !( arrow = ch->get_eq( WEAR_HOLD ) ) )
   {
      ch->print( "You are not holding a projectile!\r\n" );
      return;
   }

   if( arrow->item_type != ITEM_PROJECTILE )
   {
      ch->print( "You are not holding a projectile!\r\n" );
      return;
   }

   /*
    * modify maximum distance based on bow-type and ch's Class/str/etc 
    */
   max_dist = URANGE( 1, bow->value[4], 10 );

   if( bow->value[5] != arrow->value[4] )
   {
      string msg = "You have nothing to fire...\r\n";

      switch ( bow->value[5] )
      {
         case PROJ_BOLT:
            msg = "You have no bolts...\r\n";
            break;

         default:
         case PROJ_ARROW:
            msg = "You have no arrows...\r\n";
            break;

         case PROJ_DART:
            msg = "You have no darts...\r\n";
            break;

         case PROJ_STONE:
            msg = "You have no slingstones...\r\n";
            break;
      }
      ch->print( msg );
      return;
   }

   /*
    * Add wait state to fire for pkill, etc... 
    */
   ch->WAIT_STATE( 6 );

   /*
    * handle the ranged attack 
    */
   ranged_attack( ch, argument, bow, arrow, TYPE_HIT + arrow->value[3], max_dist );
}

/*
 * Attempt to fire at a victim.
 * Returns false if no attempt was made
 */
bool mob_fire( char_data * ch, const string & name )
{
   obj_data *arrow, *bow;
   short max_dist;

   if( ch->in_room->flags.test( ROOM_SAFE ) )
      return false;

   if( !( bow = ch->get_eq( WEAR_MISSILE_WIELD ) ) )
      return false;

   if( !( arrow = ch->get_eq( WEAR_HOLD ) ) )
      return false;

   if( arrow->item_type != ITEM_PROJECTILE )
      return false;

   if( bow->value[4] != arrow->value[5] )
      return false;

   /*
    * modify maximum distance based on bow-type and ch's Class/str/etc 
    */
   max_dist = URANGE( 1, bow->value[4], 10 );
   ranged_attack( ch, name, bow, arrow, TYPE_HIT + arrow->value[3], max_dist );
   return true;
}
