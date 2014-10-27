/****************************************************************************
 *                   ^     +----- |  / ^     ^ |     | +-\                  *
 *                  / \    |      | /  |\   /| |     | |  \                 *
 *                 /   \   +---   |<   | \ / | |     | |  |                 *
 *                /-----\  |      | \  |  v  | |     | |  /                 *
 *               /       \ |      |  \ |     | +-----+ +-/                  *
 ****************************************************************************
 * AFKMud Copyright 1997-2007 by Roger Libiez (Samson),                     *
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
 * Note: Most of the stuff in here would go in act_obj.c, but act_obj was   *
 *                               getting big.                               *
 ****************************************************************************/

#include "mud.h"
#include "area.h"
#include "liquids.h"
#include "mud_prog.h"
#include "mobindex.h"
#include "objindex.h"
#include "roomindex.h"

liquid_data *get_liq_vnum( int );
ch_ret check_room_for_traps( char_data *, int );
void teleport( char_data *, int, int );
void raw_kill( char_data *, char_data * );

extern int top_exit;

/* generate an action description message */
void actiondesc( char_data * ch, obj_data * obj )
{
   liquid_data *liq = NULL;
   char charbuf[MSL], roombuf[MSL];
   char *srcptr = obj->action_desc;
   char *charptr = charbuf;
   char *roomptr = roombuf;
   const char *ichar = "You";
   const char *iroom = "Someone";

   while( *srcptr != '\0' )
   {
      if( *srcptr == '$' )
      {
         ++srcptr;
         switch ( *srcptr )
         {
            case 'e':
               ichar = "you";
               iroom = "$e";
               break;

            case 'm':
               ichar = "you";
               iroom = "$m";
               break;

            case 'n':
               ichar = "you";
               iroom = "$n";
               break;

            case 's':
               ichar = "your";
               iroom = "$s";
               break;

               /*
                * case 'q':
                * iroom = "s";
                * break;
                */

            default:
               --srcptr;
               *charptr++ = *srcptr;
               *roomptr++ = *srcptr;
               break;
         }
      }
      else if( *srcptr == '%' && *++srcptr == 's' )
      {
         ichar = "You";
         iroom = "$n";
      }
      else
      {
         *charptr++ = *srcptr;
         *roomptr++ = *srcptr;
         ++srcptr;
         continue;
      }

      while( ( *charptr = *ichar ) != '\0' )
      {
         ++charptr;
         ++ichar;
      }

      while( ( *roomptr = *iroom ) != '\0' )
      {
         ++roomptr;
         ++iroom;
      }
      ++srcptr;
   }
   *charptr = '\0';
   *roomptr = '\0';

   switch ( obj->item_type )
   {
      case ITEM_BLOOD:
      case ITEM_FOUNTAIN:
         act( AT_ACTION, charbuf, ch, obj, ch, TO_CHAR );
         act( AT_ACTION, roombuf, ch, obj, ch, TO_ROOM );
         return;

      case ITEM_DRINK_CON:
         if( ( liq = get_liq_vnum( obj->value[2] ) ) == NULL )
            bug( "%s: bad liquid number %d.", __FUNCTION__, obj->value[2] );

         act( AT_ACTION, charbuf, ch, obj, liq->shortdesc, TO_CHAR );
         act( AT_ACTION, roombuf, ch, obj, liq->shortdesc, TO_ROOM );
         return;

      case ITEM_PIPE:
         return;

      case ITEM_ARMOR:
      case ITEM_WEAPON:
      case ITEM_LIGHT:
         return;

      case ITEM_COOK:
      case ITEM_FOOD:
      case ITEM_PILL:
         act( AT_ACTION, charbuf, ch, obj, ch, TO_CHAR );
         act( AT_ACTION, roombuf, ch, obj, ch, TO_ROOM );
         return;

      default:
         return;
   }
}

CMDF( do_eat )
{
   char buf[MSL];
   obj_data *obj;
   ch_ret retcode;
   int foodcond;
   bool hgflag = true;
   bool immH = false;   /* Immune to hunger */
   bool immT = false;   /* Immune to thirst */

   if( argument[0] == '\0' )
   {
      ch->print( "Eat what?\r\n" );
      return;
   }

   if( ch->isnpc(  ) || ch->pcdata->condition[COND_FULL] > 5 )
      if( ms_find_obj( ch ) )
         return;

   if( !( obj = find_obj( ch, argument, true ) ) )
      return;

   if( !ch->is_immortal(  ) )
   {
      if( obj->item_type != ITEM_FOOD && obj->item_type != ITEM_PILL && obj->item_type != ITEM_COOK )
      {
         act( AT_ACTION, "$n starts to nibble on $p... ($e must really be hungry)", ch, obj, NULL, TO_ROOM );
         act( AT_ACTION, "You try to nibble on $p...", ch, obj, NULL, TO_CHAR );
         return;
      }

      if( !ch->isnpc(  ) && ch->pcdata->condition[COND_FULL] > sysdata->maxcondval - 8 )
      {
         ch->print( "You are too full to eat more.\r\n" );
         return;
      }
   }

   if( !ch->isnpc(  ) && ch->pcdata->condition[COND_FULL] == -1 )
      immH = true;

   if( !ch->isnpc(  ) && ch->pcdata->condition[COND_THIRST] == -1 )
      immT = true;

   if( !ch->IS_PKILL(  ) || ( ch->IS_PKILL(  ) && !ch->has_pcflag( PCFLAG_HIGHGAG ) ) )
      hgflag = false;

   /*
    * required due to object grouping 
    */
   obj->separate(  );
   if( obj->in_obj )
   {
      if( !hgflag )
         act( AT_PLAIN, "You take $p from $P.", ch, obj, obj->in_obj, TO_CHAR );
      act( AT_PLAIN, "$n takes $p from $P.", ch, obj, obj->in_obj, TO_ROOM );
   }
   if( ch->fighting && number_percent(  ) > ( ch->get_curr_dex(  ) * 2 + 47 ) )
   {
      snprintf( buf, MSL, "%s",
                ( ch->in_room->sector_type == SECT_UNDERWATER ||
                  ch->in_room->sector_type == SECT_WATER_SWIM ||
                  ch->in_room->sector_type == SECT_WATER_NOSWIM ||
                  ch->in_room->sector_type == SECT_RIVER ) ? "dissolves in the water" :
                ( ch->in_room->sector_type == SECT_AIR ||
                  ch->in_room->flags.test( ROOM_NOFLOOR ) ) ? "falls far below" : "is trampled underfoot" );
      act( AT_MAGIC, "$n drops $p, and it $T.", ch, obj, buf, TO_ROOM );
      if( !hgflag )
         act( AT_MAGIC, "Oops, $p slips from your hand and $T!", ch, obj, buf, TO_CHAR );
   }
   else
   {
      if( !oprog_use_trigger( ch, obj, NULL, NULL ) )
      {
         if( !obj->action_desc || obj->action_desc[0] == '\0' )
         {
            act( AT_ACTION, "$n eats $p.", ch, obj, NULL, TO_ROOM );
            if( !hgflag )
               act( AT_ACTION, "You eat $p.", ch, obj, NULL, TO_CHAR );
            ch->sound( "eat.wav", 100, false );
         }
         else
            actiondesc( ch, obj );
      }
      switch ( obj->item_type )
      {
         default:
            break;

         case ITEM_COOK:
         case ITEM_FOOD:
            ch->WAIT_STATE( sysdata->pulsepersec / 3 );
            if( obj->timer > 0 && obj->value[1] > 0 )
               foodcond = ( obj->timer * 10 ) / obj->value[1];
            else
               foodcond = 10;

            if( !ch->isnpc(  ) )
            {
               int condition;

               condition = ch->pcdata->condition[COND_FULL];
               gain_condition( ch, COND_FULL, ( obj->value[0] * foodcond ) / 10 );
               if( condition <= 1 && ch->pcdata->condition[COND_FULL] > 1 )
                  ch->print( "You are no longer hungry.\r\n" );
               else if( ch->pcdata->condition[COND_FULL] > sysdata->maxcondval * 0.8 )
                  ch->print( "You are full.\r\n" );
            }

            if( obj->value[3] != 0 || ( foodcond < 4 && number_range( 0, foodcond + 1 ) == 0 )
                || ( obj->item_type == ITEM_COOK && obj->value[2] == 0 ) )
            {
               /*
                * The food was poisoned! 
                */
               affect_data af;

               /*
                * Half Trolls are not affected by bad food 
                */
               if( ch->race == RACE_HALF_TROLL )
                  ch->printf( "%s was poisoned, but had no affect on you.\r\n", obj->short_descr );

               else
               {
                  if( obj->value[3] != 0 )
                  {
                     act( AT_POISON, "$n chokes and gags.", ch, NULL, NULL, TO_ROOM );
                     act( AT_POISON, "You choke and gag.", ch, NULL, NULL, TO_CHAR );
                     ch->mental_state = URANGE( 20, ch->mental_state + 5, 100 );
                  }
                  else
                  {
                     act( AT_POISON, "$n gags on $p.", ch, obj, NULL, TO_ROOM );
                     act( AT_POISON, "You gag on $p.", ch, obj, NULL, TO_CHAR );
                     ch->mental_state = URANGE( 15, ch->mental_state + 5, 100 );
                  }

                  af.type = gsn_poison;
                  af.duration = 2 * obj->value[0] * ( obj->value[3] > 0 ? obj->value[3] : 1 );
                  af.location = APPLY_NONE;
                  af.modifier = 0;
                  af.bit = AFF_POISON;
                  ch->affect_join( &af );
               }
            }
            break;

         case ITEM_PILL:
            if( ch->who_fighting(  ) && ch->IS_PKILL(  ) )
               ch->WAIT_STATE( sysdata->pulsepersec / 4 );
            else
               ch->WAIT_STATE( sysdata->pulsepersec / 3 );
            /*
             * allow pills to fill you, if so desired 
             */
            if( !ch->isnpc(  ) && obj->value[4] )
            {
               int condition;

               condition = ch->pcdata->condition[COND_FULL];
               gain_condition( ch, COND_FULL, obj->value[4] );
               if( condition <= 1 && ch->pcdata->condition[COND_FULL] > 1 )
                  ch->print( "You are no longer hungry.\r\n" );
               else if( ch->pcdata->condition[COND_FULL] > sysdata->maxcondval - 8 )
                  ch->print( "You are full.\r\n" );
            }
            retcode = obj_cast_spell( obj->value[1], obj->value[0], ch, ch, NULL );
            if( retcode == rNONE )
               retcode = obj_cast_spell( obj->value[2], obj->value[0], ch, ch, NULL );
            if( retcode == rNONE )
               retcode = obj_cast_spell( obj->value[3], obj->value[0], ch, ch, NULL );
            break;
      }
   }
   obj->extract(  );

   /*
    * Reset immunity for those who have it 
    */
   if( !ch->isnpc(  ) )
   {
      if( immH )
         ch->pcdata->condition[COND_FULL] = -1;
      if( immT )
         ch->pcdata->condition[COND_THIRST] = -1;
   }
   return;
}

CMDF( do_quaff )
{
   obj_data *obj;
   ch_ret retcode;
   bool hgflag = true;

   if( argument[0] == '\0' || !str_cmp( argument, "" ) )
   {
      ch->print( "Quaff what?\r\n" );
      return;
   }

   if( !( obj = find_obj( ch, argument, true ) ) )
      return;

   if( ch->isnpc(  ) && ch->has_aflag( AFF_CHARM ) )
      return;

   if( obj->item_type != ITEM_POTION )
   {
      if( obj->item_type == ITEM_DRINK_CON )
         cmdf( ch, "drink %s", obj->name );
      else
      {
         act( AT_ACTION, "$n lifts $p up to $s mouth and tries to drink from it...", ch, obj, NULL, TO_ROOM );
         act( AT_ACTION, "You bring $p up to your mouth and try to drink from it...", ch, obj, NULL, TO_CHAR );
      }
      return;
   }

   /*
    * Empty container check               -Shaddai
    */
   if( obj->value[1] == -1 && obj->value[2] == -1 && obj->value[3] == -1 )
   {
      ch->print( "You suck in nothing but air.\r\n" );
      return;
   }

   if( !ch->IS_PKILL(  ) || ( ch->IS_PKILL(  ) && !ch->has_pcflag( PCFLAG_HIGHGAG ) ) )
      hgflag = false;

   obj->separate(  );
   if( obj->in_obj )
   {
      if( !ch->CAN_PKILL(  ) )
      {
         act( AT_PLAIN, "You take $p from $P.", ch, obj, obj->in_obj, TO_CHAR );
         act( AT_PLAIN, "$n takes $p from $P.", ch, obj, obj->in_obj, TO_ROOM );
      }
   }

   /*
    * If fighting, chance of dropping potion       -Thoric
    */
   if( ch->fighting && number_percent(  ) > ( ch->get_curr_dex(  ) * 2 + 48 ) )
   {
      act( AT_MAGIC, "$n fumbles $p and shatters it into fragments.", ch, obj, NULL, TO_ROOM );
      if( !hgflag )
         act( AT_MAGIC, "Oops... $p is knocked from your hand and shatters!", ch, obj, NULL, TO_CHAR );
   }
   else
   {
      if( !oprog_use_trigger( ch, obj, NULL, NULL ) )
      {
         if( !ch->CAN_PKILL(  ) || !obj->in_obj )
         {
            act( AT_ACTION, "$n quaffs $p.", ch, obj, NULL, TO_ROOM );
            if( !hgflag )
               act( AT_ACTION, "You quaff $p.", ch, obj, NULL, TO_CHAR );
         }
         else if( obj->in_obj )
         {
            act( AT_ACTION, "$n quaffs $p from $P.", ch, obj, obj->in_obj, TO_ROOM );
            if( !hgflag )
               act( AT_ACTION, "You quaff $p from $P.", ch, obj, obj->in_obj, TO_CHAR );
         }
      }

      if( ch->who_fighting(  ) && ch->IS_PKILL(  ) )
         ch->WAIT_STATE( sysdata->pulsepersec / 5 );
      else
         ch->WAIT_STATE( sysdata->pulsepersec / 3 );

      retcode = obj_cast_spell( obj->value[1], obj->value[0], ch, ch, NULL );
      if( retcode == rNONE )
         retcode = obj_cast_spell( obj->value[2], obj->value[0], ch, ch, NULL );
      if( retcode == rNONE )
         retcode = obj_cast_spell( obj->value[3], obj->value[0], ch, ch, NULL );
   }
   obj->extract(  );
   return;
}

CMDF( do_recite )
{
   char arg1[MIL];
   char_data *victim;
   obj_data *scroll, *obj;
   ch_ret retcode;

   argument = one_argument( argument, arg1 );

   if( !arg1 || arg1[0] == '\0' )
   {
      ch->print( "Recite what?\r\n" );
      return;
   }

   if( ms_find_obj( ch ) )
      return;

   if( !( scroll = ch->get_obj_carry( arg1 ) ) )
   {
      ch->print( "You do not have that scroll.\r\n" );
      return;
   }

   if( scroll->item_type != ITEM_SCROLL )
   {
      act( AT_ACTION, "$n holds up $p as if to recite something from it...", ch, scroll, NULL, TO_ROOM );
      act( AT_ACTION, "You hold up $p and stand there with your mouth open.  (Now what?)", ch, scroll, NULL, TO_CHAR );
      return;
   }

   if( ch->isnpc(  ) && ( scroll->pIndexData->vnum == OBJ_VNUM_SCROLL_SCRIBING ) )
   {
      ch->print( "As a mob, this dialect is foreign to you.\r\n" );
      return;
   }

   if( ( scroll->pIndexData->vnum == OBJ_VNUM_SCROLL_SCRIBING ) && ( ch->level + 10 < scroll->value[0] ) )
   {
      ch->print( "This scroll is too complex for you to understand.\r\n" );
      return;
   }

   obj = NULL;
   if( !argument || argument[0] == '\0' )
      victim = ch;
   else
   {
      if( !( victim = ch->get_char_room( argument ) ) && !( obj = ch->get_obj_here( argument ) ) )
      {
         ch->print( "You can't find it.\r\n" );
         return;
      }
   }

   scroll->separate(  );
   act( AT_MAGIC, "$n recites $p.", ch, scroll, NULL, TO_ROOM );
   act( AT_MAGIC, "You recite $p.", ch, scroll, NULL, TO_CHAR );

   if( victim != ch )
      ch->WAIT_STATE( 2 * sysdata->pulseviolence );
   else
      ch->WAIT_STATE( sysdata->pulsepersec / 2 );

   retcode = obj_cast_spell( scroll->value[1], scroll->value[0], ch, victim, obj );
   if( retcode == rNONE )
      retcode = obj_cast_spell( scroll->value[2], scroll->value[0], ch, victim, obj );
   if( retcode == rNONE )
      retcode = obj_cast_spell( scroll->value[3], scroll->value[0], ch, victim, obj );

   scroll->extract(  );
   return;
}

/*
 * Function to handle the state changing of a triggerobject (lever)  -Thoric
 */
void pullorpush( char_data * ch, obj_data * obj, bool pull )
{
   bool isup;
   room_index *room, *to_room;

   if( IS_SET( obj->value[0], TRIG_UP ) )
      isup = true;
   else
      isup = false;
   switch ( obj->item_type )
   {
      default:
         ch->printf( "You can't %s that!\r\n", pull ? "pull" : "push" );
         return;
      case ITEM_SWITCH:
      case ITEM_LEVER:
      case ITEM_PULLCHAIN:
         if( ( !pull && isup ) || ( pull && !isup ) )
         {
            ch->printf( "It is already %s.\r\n", isup ? "up" : "down" );
            return;
         }
         break;
      case ITEM_BUTTON:
         if( ( !pull && isup ) || ( pull && !isup ) )
         {
            ch->printf( "It is already %s.\r\n", isup ? "in" : "out" );
            return;
         }
         break;
   }

   if( ( pull ) && HAS_PROG( obj->pIndexData, PULL_PROG ) )
   {
      if( !IS_SET( obj->value[0], TRIG_AUTORETURN ) )
         REMOVE_BIT( obj->value[0], TRIG_UP );
      oprog_pull_trigger( ch, obj );
      return;
   }

   if( ( !pull ) && HAS_PROG( obj->pIndexData, PUSH_PROG ) )
   {
      if( !IS_SET( obj->value[0], TRIG_AUTORETURN ) )
         SET_BIT( obj->value[0], TRIG_UP );
      oprog_push_trigger( ch, obj );
      return;
   }

   if( !oprog_use_trigger( ch, obj, NULL, NULL ) )
   {
      act_printf( AT_ACTION, ch, obj, NULL, TO_ROOM, "$n %s $p.", pull ? "pulls" : "pushes" );
      act_printf( AT_ACTION, ch, obj, NULL, TO_CHAR, "You %s $p.", pull ? "pull" : "push" );
   }

   if( !IS_SET( obj->value[0], TRIG_AUTORETURN ) )
   {
      if( pull )
         REMOVE_BIT( obj->value[0], TRIG_UP );
      else
         SET_BIT( obj->value[0], TRIG_UP );
   }

   if( IS_SET( obj->value[0], TRIG_TELEPORT )
       || IS_SET( obj->value[0], TRIG_TELEPORTALL ) || IS_SET( obj->value[0], TRIG_TELEPORTPLUS ) )
   {
      int flags;

      if( !( room = get_room_index( obj->value[1] ) ) )
      {
         bug( "%s: obj points to invalid room %d", __FUNCTION__, obj->value[1] );
         return;
      }
      flags = 0;
      if( IS_SET( obj->value[0], TRIG_SHOWROOMDESC ) )
         SET_BIT( flags, TELE_SHOWDESC );
      if( IS_SET( obj->value[0], TRIG_TELEPORTALL ) )
         SET_BIT( flags, TELE_TRANSALL );
      if( IS_SET( obj->value[0], TRIG_TELEPORTPLUS ) )
         SET_BIT( flags, TELE_TRANSALLPLUS );

      teleport( ch, obj->value[1], flags );
      return;
   }

   if( IS_SET( obj->value[0], TRIG_RAND4 ) || IS_SET( obj->value[0], TRIG_RAND6 ) )
   {
      int maxd;

      if( !( room = get_room_index( obj->value[1] ) ) )
      {
         bug( "%s: obj points to invalid room %d", __FUNCTION__, obj->value[1] );
         return;
      }

      if( IS_SET( obj->value[0], TRIG_RAND4 ) )
         maxd = 3;
      else
         maxd = 5;

      room->randomize_exits( maxd );
      list<char_data*>::iterator ich;
      for( ich = room->people.begin(); ich != room->people.end(); ++ich )
      {
         char_data *rch = (*ich);

         rch->print( "You hear a loud rumbling sound.\r\n" );
         rch->print( "Something seems different...\r\n" );
      }
   }

   /* Death support added by Remcon */
   if( IS_SET( obj->value[0], TRIG_DEATH ) )
   {
      /* Should we really send a message to the room? */
      act( AT_DEAD, "$n falls prey to a terrible death!", ch, NULL, NULL, TO_ROOM );
      act( AT_DEAD, "Oopsie... you're dead!\r\n", ch, NULL, NULL, TO_CHAR );
      log_printf( "%s hit a DEATH TRIGGER in room %d!", ch->name, ch->in_room->vnum );

      /* Personaly I figured if we wanted it to be a full DT we could just have it send them into a DT. */
      raw_kill( ch, ch );
      return;
   }

   /* Object loading added by Remcon */
   if( IS_SET( obj->value[0], TRIG_OLOAD ) )
   {
      obj_index *pObjIndex;
      obj_data *tobj;

      /* value[1] for the obj vnum */
      if( !( pObjIndex = get_obj_index( obj->value[1] ) ) )
      {
         bug( "%s: obj points to invalid object vnum %d", __FUNCTION__, obj->value[1] );
         return;
      }
      /* Set room to NULL before the check */
      room = NULL;
      /* value[2] for the room vnum to put the object in if there is one, 0 for giving it to char or current room */
      if( obj->value[2] > 0 && !( room = get_room_index( obj->value[2] ) ) )
      {
         bug( "%s: obj points to invalid room vnum %d", __FUNCTION__, obj->value[2] );
         return;
      }
      /* Uses value[3] for level */
      if( !( tobj = pObjIndex->create_object( URANGE( 0, obj->value[3], MAX_LEVEL ) ) ) )
      {
         bug( "%s: obj couldn't create obj vnum %d at level %d", __FUNCTION__, obj->value[1], obj->value[3] );
         return;
      }
      if( room )
         tobj->to_room( room, ch );
      else
      {
         if( obj->wear_flags.test( ITEM_TAKE ) )
            tobj->to_char( ch );
         else
            tobj->to_room( ch->in_room, ch );
      }
      return;
   }

   /* Mob loading added by Remcon */
   if( IS_SET( obj->value[0], TRIG_MLOAD ) )
   {
      mob_index *pMobIndex;
      char_data *mob;

      /* value[1] for the obj vnum */
      if( !( pMobIndex = get_mob_index( obj->value[1] ) ) )
      {
         bug( "%s: obj points to invalid mob vnum %d", __FUNCTION__, obj->value[1] );
         return;
      }
      /* Set room to current room before the check */
      room = ch->in_room;
      /* value[2] for the room vnum to put the object in if there is one, 0 for giving it to char or current room */
      if( obj->value[2] > 0 && !( room = get_room_index( obj->value[2] ) ) )
      {
         bug( "%s: obj points to invalid room vnum %d", __FUNCTION__, obj->value[2] );
         return;
      }
      if( !( mob = pMobIndex->create_mobile() ) )
      {
         bug( "%s: obj couldnt create_mobile vnum %d", __FUNCTION__, obj->value[1] );
         return;
      }
      mob->to_room( room );
      return;
   }

   /* Spell casting support added by Remcon */
   if( IS_SET( obj->value[0], TRIG_CAST ) )
   {
      if( obj->value[1] <= 0 || !IS_VALID_SN( obj->value[1] ) )
      {
         bug( "%s: obj points to invalid sn [%d]", __FUNCTION__, obj->value[1] );
         return;
      }
      obj_cast_spell( obj->value[1], URANGE( 1, ( obj->value[2] > 0 ) ? obj->value[2] : ch->level, MAX_LEVEL ), ch, ch, NULL );
      return;
   }

   /* Container support added by Remcon */
   if( IS_SET( obj->value[0], TRIG_CONTAINER ) )
   {
      obj_data *container = NULL;
      list<obj_data*>::iterator iobj;
      bool found = false;

      room = get_room_index( obj->value[1] );
      if( !room )
         room = obj->in_room;
      if( !room )
      {
         bug( "%s: obj points to invalid room %d", __FUNCTION__, obj->value[1] );
         return;
      }

      for( iobj = ch->in_room->objects.begin(); iobj != ch->in_room->objects.end(); ++iobj )
      {
         container = *iobj;

         if( container->pIndexData->vnum == obj->value[2] )
         {
            found = true;
            break;
         }
      }
      if( !found )
      {
         bug( "%s: obj points to a container [%d] thats not where it should be?", __FUNCTION__, obj->value[2] );
         return;
      }
      if( container->item_type != ITEM_CONTAINER )
      {
         bug( "%s: obj points to object [%d], but it isn't a container.", __FUNCTION__, obj->value[2] );
         return;
      }
      /* Could toss in some messages. Limit how it is handled etc... I'll leave that to each one to do */
      /* Started to use TRIG_OPEN, TRIG_CLOSE, TRIG_LOCK, and TRIG_UNLOCK like TRIG_DOOR does. */
      /* It limits it alot, but it wouldn't allow for an EATKEY change */
      if( IS_SET( obj->value[3], CONT_CLOSEABLE ) )
         TOGGLE_BIT( container->value[1], CONT_CLOSEABLE );
      if( IS_SET( obj->value[3], CONT_PICKPROOF ) )
         TOGGLE_BIT( container->value[1], CONT_PICKPROOF );
      if( IS_SET( obj->value[3], CONT_CLOSED ) )
         TOGGLE_BIT( container->value[1], CONT_CLOSED );
      if( IS_SET( obj->value[3], CONT_LOCKED ) )
         TOGGLE_BIT( container->value[1], CONT_LOCKED );
      if( IS_SET( obj->value[3], CONT_EATKEY ) )
         TOGGLE_BIT( container->value[1], CONT_EATKEY );
      return;
   }

   if( IS_SET( obj->value[0], TRIG_DOOR ) )
   {
      int edir;
      char *txt;

      if( !( room = get_room_index( obj->value[1] ) ) )
         room = obj->in_room;
      if( !room )
      {
         bug( "%s: obj points to invalid room %d", __FUNCTION__, obj->value[1] );
         return;
      }
      if( IS_SET( obj->value[0], TRIG_D_NORTH ) )
      {
         edir = DIR_NORTH;
         txt = "to the north";
      }
      else if( IS_SET( obj->value[0], TRIG_D_SOUTH ) )
      {
         edir = DIR_SOUTH;
         txt = "to the south";
      }
      else if( IS_SET( obj->value[0], TRIG_D_EAST ) )
      {
         edir = DIR_EAST;
         txt = "to the east";
      }
      else if( IS_SET( obj->value[0], TRIG_D_WEST ) )
      {
         edir = DIR_WEST;
         txt = "to the west";
      }
      else if( IS_SET( obj->value[0], TRIG_D_UP ) )
      {
         edir = DIR_UP;
         txt = "from above";
      }
      else if( IS_SET( obj->value[0], TRIG_D_DOWN ) )
      {
         edir = DIR_DOWN;
         txt = "from below";
      }
      else
      {
         bug( "%s: door: no direction flag set.", __FUNCTION__ );
         return;
      }
      exit_data *pexit = room->get_exit( edir );
      exit_data *pexit_rev;
      if( !pexit )
      {
         if( !IS_SET( obj->value[0], TRIG_PASSAGE ) )
         {
            bug( "%s: obj points to non-exit %d", __FUNCTION__, obj->value[1] );
            return;
         }
         if( !( to_room = get_room_index( obj->value[2] ) ) )
         {
            bug( "%s: dest points to invalid room %d", __FUNCTION__, obj->value[2] );
            return;
         }
         pexit = room->make_exit( to_room, edir );
         pexit->key = -1;
         pexit->flags.reset(  );
         ++top_exit;
         act( AT_PLAIN, "A passage opens!", ch, NULL, NULL, TO_CHAR );
         act( AT_PLAIN, "A passage opens!", ch, NULL, NULL, TO_ROOM );
         return;
      }
      if( IS_SET( obj->value[0], TRIG_UNLOCK ) && IS_EXIT_FLAG( pexit, EX_LOCKED ) )
      {
         REMOVE_EXIT_FLAG( pexit, EX_LOCKED );
         act( AT_PLAIN, "You hear a faint click $T.", ch, NULL, txt, TO_CHAR );
         act( AT_PLAIN, "You hear a faint click $T.", ch, NULL, txt, TO_ROOM );
         if( ( pexit_rev = pexit->rexit ) != NULL && pexit_rev->to_room == ch->in_room )
            REMOVE_EXIT_FLAG( pexit_rev, EX_LOCKED );
         return;
      }
      if( IS_SET( obj->value[0], TRIG_LOCK ) && !IS_EXIT_FLAG( pexit, EX_LOCKED ) )
      {
         SET_EXIT_FLAG( pexit, EX_LOCKED );
         act( AT_PLAIN, "You hear a faint click $T.", ch, NULL, txt, TO_CHAR );
         act( AT_PLAIN, "You hear a faint click $T.", ch, NULL, txt, TO_ROOM );
         if( ( pexit_rev = pexit->rexit ) != NULL && pexit_rev->to_room == ch->in_room )
            SET_EXIT_FLAG( pexit_rev, EX_LOCKED );
         return;
      }
      if( IS_SET( obj->value[0], TRIG_OPEN ) && IS_EXIT_FLAG( pexit, EX_CLOSED ) )
      {
         list<char_data*>::iterator ich;

         REMOVE_EXIT_FLAG( pexit, EX_CLOSED );
         for( ich = room->people.begin(); ich != room->people.end(); ++ich )
         {
            char_data *rch = (*ich);
            act( AT_ACTION, "The $d opens.", rch, NULL, pexit->keyword, TO_CHAR );
         }
         if( ( pexit_rev = pexit->rexit ) != NULL && pexit_rev->to_room == ch->in_room )
         {
            REMOVE_EXIT_FLAG( pexit_rev, EX_CLOSED );
            /*
             * bug here pointed out by Nick Gammon 
             */
            for( ich = pexit->to_room->people.begin(); ich != pexit->to_room->people.end(); ++ich )
            {
               char_data *rch = (*ich);
               act( AT_ACTION, "The $d opens.", rch, NULL, pexit_rev->keyword, TO_CHAR );
            }
         }
         check_room_for_traps( ch, trap_door[edir] );
         return;
      }
      if( IS_SET( obj->value[0], TRIG_CLOSE ) && !IS_EXIT_FLAG( pexit, EX_CLOSED ) )
      {
         list<char_data*>::iterator ich;

         SET_EXIT_FLAG( pexit, EX_CLOSED );
         for( ich = room->people.begin(); ich != room->people.end(); ++ich )
         {
            char_data *rch = (*ich);
            act( AT_ACTION, "The $d closes.", rch, NULL, pexit->keyword, TO_CHAR );
         }
         if( ( pexit_rev = pexit->rexit ) != NULL && pexit_rev->to_room == ch->in_room )
         {
            SET_EXIT_FLAG( pexit_rev, EX_CLOSED );
            /*
             * bug here pointed out by Nick Gammon 
             */
            for( ich = pexit->to_room->people.begin(); ich != pexit->to_room->people.end(); ++ich )
            {
               char_data *rch = (*ich);
               act( AT_ACTION, "The $d closes.", rch, NULL, pexit_rev->keyword, TO_CHAR );
            }
         }
         check_room_for_traps( ch, trap_door[edir] );
         return;
      }
   }
}

CMDF( do_pull )
{
   obj_data *obj;

   if( !argument || argument[0] == '\0' )
   {
      ch->print( "Pull what?\r\n" );
      return;
   }

   if( ms_find_obj( ch ) )
      return;

   if( !( obj = ch->get_obj_here( argument ) ) )
   {
      act( AT_PLAIN, "I see no $T here.", ch, NULL, argument, TO_CHAR );
      return;
   }
   pullorpush( ch, obj, true );
}

CMDF( do_push )
{
   obj_data *obj;

   if( !argument || argument[0] == '\0' )
   {
      ch->print( "Push what?\r\n" );
      return;
   }

   if( ms_find_obj( ch ) )
      return;

   if( !( obj = ch->get_obj_here( argument ) ) )
   {
      act( AT_PLAIN, "I see no $T here.", ch, NULL, argument, TO_CHAR );
      return;
   }
   pullorpush( ch, obj, false );
}

CMDF( do_rap )
{
   exit_data *pexit;

   if( !argument || argument[0] == '\0' )
   {
      ch->print( "Rap on what?\r\n" );
      return;
   }
   if( ch->fighting )
   {
      ch->print( "You have better things to do with your hands right now.\r\n" );
      return;
   }
   if( ( pexit = find_door( ch, argument, false ) ) != NULL )
   {
      room_index *to_room;
      exit_data *pexit_rev;
      char *keyword;

      if( !IS_EXIT_FLAG( pexit, EX_CLOSED ) )
      {
         ch->print( "Why knock? It's open.\r\n" );
         return;
      }
      if( IS_EXIT_FLAG( pexit, EX_SECRET ) )
         keyword = "wall";
      else
         keyword = pexit->keyword;
      act( AT_ACTION, "You rap loudly on the $d.", ch, NULL, keyword, TO_CHAR );
      act( AT_ACTION, "$n raps loudly on the $d.", ch, NULL, keyword, TO_ROOM );
      if( ( to_room = pexit->to_room ) != NULL && ( pexit_rev = pexit->rexit ) != NULL && pexit_rev->to_room == ch->in_room )
      {
         list<char_data*>::iterator ich;

         for( ich = to_room->people.begin(); ich != to_room->people.end(); ++ich )
         {
            char_data *rch = (*ich);
            act( AT_ACTION, "Someone raps loudly from the other side of the $d.", rch, NULL, pexit_rev->keyword, TO_CHAR );
         }
      }
   }
   else
   {
      act( AT_ACTION, "You make knocking motions through the air.", ch, NULL, NULL, TO_CHAR );
      act( AT_ACTION, "$n makes knocking motions through the air.", ch, NULL, NULL, TO_ROOM );
   }
   return;
}

/* pipe commands (light, tamp, smoke) by Thoric */
CMDF( do_tamp )
{
   obj_data *cpipe;

   if( !argument || argument[0] == '\0' )
   {
      ch->print( "Tamp what?\r\n" );
      return;
   }

   if( ms_find_obj( ch ) )
      return;

   if( !( cpipe = ch->get_obj_carry( argument ) ) )
   {
      ch->print( "You aren't carrying that.\r\n" );
      return;
   }

   if( cpipe->item_type != ITEM_PIPE )
   {
      ch->print( "You can't tamp that.\r\n" );
      return;
   }

   if( !IS_SET( cpipe->value[3], PIPE_TAMPED ) )
   {
      act( AT_ACTION, "You gently tamp $p.", ch, cpipe, NULL, TO_CHAR );
      act( AT_ACTION, "$n gently tamps $p.", ch, cpipe, NULL, TO_ROOM );
      SET_BIT( cpipe->value[3], PIPE_TAMPED );
      return;
   }
   ch->print( "It doesn't need tamping.\r\n" );
}

CMDF( do_smoke )
{
   obj_data *cpipe;

   if( !argument || argument[0] == '\0' )
   {
      ch->print( "Smoke what?\r\n" );
      return;
   }

   if( ms_find_obj( ch ) )
      return;

   if( !( cpipe = ch->get_obj_carry( argument ) ) )
   {
      ch->print( "You aren't carrying that.\r\n" );
      return;
   }
   if( cpipe->item_type != ITEM_PIPE )
   {
      act( AT_ACTION, "You try to smoke $p... but it doesn't seem to work.", ch, cpipe, NULL, TO_CHAR );
      act( AT_ACTION, "$n tries to smoke $p... (I wonder what $e's been putting in $s pipe?)", ch, cpipe, NULL, TO_ROOM );
      return;
   }
   if( !IS_SET( cpipe->value[3], PIPE_LIT ) )
   {
      act( AT_ACTION, "You try to smoke $p, but it's not lit.", ch, cpipe, NULL, TO_CHAR );
      act( AT_ACTION, "$n tries to smoke $p, but it's not lit.", ch, cpipe, NULL, TO_ROOM );
      return;
   }
   if( cpipe->value[1] > 0 )
   {
      if( !oprog_use_trigger( ch, cpipe, NULL, NULL ) )
      {
         act( AT_ACTION, "You draw thoughtfully from $p.", ch, cpipe, NULL, TO_CHAR );
         act( AT_ACTION, "$n draws thoughtfully from $p.", ch, cpipe, NULL, TO_ROOM );
      }

      if( IS_VALID_HERB( cpipe->value[2] ) && cpipe->value[2] < top_herb )
      {
         int sn = cpipe->value[2] + TYPE_HERB;
         skill_type *skill = get_skilltype( sn );

         ch->WAIT_STATE( skill->beats );
         if( skill->spell_fun )
            obj_cast_spell( sn, UMIN( skill->min_level, ch->level ), ch, ch, NULL );
         if( cpipe->extracted(  ) )
            return;
      }
      else
         bug( "%s: bad herb type %d", __FUNCTION__, cpipe->value[2] );

      SET_BIT( cpipe->value[3], PIPE_HOT );
      if( --cpipe->value[1] < 1 )
      {
         REMOVE_BIT( cpipe->value[3], PIPE_LIT );
         SET_BIT( cpipe->value[3], PIPE_DIRTY );
         SET_BIT( cpipe->value[3], PIPE_FULLOFASH );
      }
   }
}

CMDF( do_light )
{
   obj_data *cpipe;

   if( !argument || argument[0] == '\0' )
   {
      ch->print( "Light what?\r\n" );
      return;
   }

   if( ms_find_obj( ch ) )
      return;

   if( !( cpipe = ch->get_obj_carry( argument ) ) )
   {
      ch->print( "You aren't carrying that.\r\n" );
      return;
   }
   if( cpipe->item_type != ITEM_PIPE )
   {
      ch->print( "You can't light that.\r\n" );
      return;
   }
   if( !IS_SET( cpipe->value[3], PIPE_LIT ) )
   {
      if( cpipe->value[1] < 1 )
      {
         act( AT_ACTION, "You try to light $p, but it's empty.", ch, cpipe, NULL, TO_CHAR );
         act( AT_ACTION, "$n tries to light $p, but it's empty.", ch, cpipe, NULL, TO_ROOM );
         return;
      }
      act( AT_ACTION, "You carefully light $p.", ch, cpipe, NULL, TO_CHAR );
      act( AT_ACTION, "$n carefully lights $p.", ch, cpipe, NULL, TO_ROOM );
      SET_BIT( cpipe->value[3], PIPE_LIT );
      return;
   }
   ch->print( "It's already lit.\r\n" );
}

/*
 * Apply a salve/ointment					-Thoric
 * Support for applying to others.  Pkill concerns dealt with elsewhere.
 */
CMDF( do_apply )
{
   char arg1[MIL];
   char_data *victim;
   obj_data *salve, *obj;
   ch_ret retcode;

   argument = one_argument( argument, arg1 );
   if( !arg1 || arg1[0] == '\0' )
   {
      ch->print( "Apply what?\r\n" );
      return;
   }
   if( ch->fighting )
   {
      ch->print( "You're too busy fighting ...\r\n" );
      return;
   }
   if( ms_find_obj( ch ) )
      return;
   if( !( salve = ch->get_obj_carry( arg1 ) ) )
   {
      ch->print( "You do not have that.\r\n" );
      return;
   }

   obj = NULL;
   if( !argument || argument[0] == '\0' )
      victim = ch;
   else
   {
      if( !( victim = ch->get_char_room( argument ) ) && !( obj = ch->get_obj_here( argument ) ) )
      {
         ch->print( "Apply it to what or who?\r\n" );
         return;
      }
   }

   /*
    * apply salve to another object 
    */
   if( obj )
   {
      act( AT_ACTION, "You rub $p on $o, but nothing happens. (not implemented)", ch, salve, obj, TO_CHAR );
      return;
   }

   if( victim->fighting )
   {
      ch->print( "Wouldn't work very well while they're fighting ...\r\n" );
      return;
   }

   if( salve->item_type != ITEM_SALVE )
   {
      if( victim == ch )
      {
         act( AT_ACTION, "$n starts to rub $p on $mself...", ch, salve, NULL, TO_ROOM );
         act( AT_ACTION, "You try to rub $p on yourself...", ch, salve, NULL, TO_CHAR );
      }
      else
      {
         act( AT_ACTION, "$n starts to rub $p on $N...", ch, salve, victim, TO_NOTVICT );
         act( AT_ACTION, "$n starts to rub $p on you...", ch, salve, victim, TO_VICT );
         act( AT_ACTION, "You try to rub $p on $N...", ch, salve, victim, TO_CHAR );
      }
      return;
   }
   salve->separate(  );
   --salve->value[1];

   if( !oprog_use_trigger( ch, salve, NULL, NULL ) )
   {
      if( !salve->action_desc || salve->action_desc[0] == '\0' )
      {
         if( salve->value[1] < 1 )
         {
            if( victim != ch )
            {
               act( AT_ACTION, "$n rubs the last of $p onto $N.", ch, salve, victim, TO_NOTVICT );
               act( AT_ACTION, "$n rubs the last of $p onto you.", ch, salve, victim, TO_VICT );
               act( AT_ACTION, "You rub the last of $p onto $N.", ch, salve, victim, TO_CHAR );
            }
            else
            {
               act( AT_ACTION, "You rub the last of $p onto yourself.", ch, salve, NULL, TO_CHAR );
               act( AT_ACTION, "$n rubs the last of $p onto $mself.", ch, salve, NULL, TO_ROOM );
            }
         }
         else
         {
            if( victim != ch )
            {
               act( AT_ACTION, "$n rubs $p onto $N.", ch, salve, victim, TO_NOTVICT );
               act( AT_ACTION, "$n rubs $p onto you.", ch, salve, victim, TO_VICT );
               act( AT_ACTION, "You rub $p onto $N.", ch, salve, victim, TO_CHAR );
            }
            else
            {
               act( AT_ACTION, "You rub $p onto yourself.", ch, salve, NULL, TO_CHAR );
               act( AT_ACTION, "$n rubs $p onto $mself.", ch, salve, NULL, TO_ROOM );
            }
         }
      }
      else
         actiondesc( ch, salve );
   }

   ch->WAIT_STATE( salve->value[3] );
   retcode = obj_cast_spell( salve->value[4], salve->value[0], ch, victim, NULL );
   if( retcode == rNONE )
      retcode = obj_cast_spell( salve->value[5], salve->value[0], ch, victim, NULL );
   if( retcode == rCHAR_DIED )
   {
      bug( "%s: char died", __FUNCTION__ );
      return;
   }

   if( !salve->extracted(  ) && salve->value[1] <= 0 )
      salve->extract(  );
   return;
}

/* The Northwind's Rune system */
CMDF( do_invoke )
{
   obj_data *obj;
   room_index *location;
   char_data *opponent;

   location = NULL;

   if( !argument || argument[0] == '\0' )
   {
      ch->print( "Invoke what?\r\n" );
      return;
   }

   if( ms_find_obj( ch ) )
      return;

   if( !( obj = ch->get_obj_carry( argument ) ) )
   {
      ch->print( "You do not have that item.\r\n" );
      return;
   }

   if( obj->value[2] < 1 )
   {
      ch->print( "&RIt is out of charges!\r\n" );
      return;
   }

   if( ch->in_room->flags.test( ROOM_NO_RECALL ) || ch->in_room->area->flags.test( AFLAG_NORECALL ) )
   {
      ch->print( "&RAncient magiks prevent the rune from functioning here.\r\n" );
      return;
   }

   if( obj->value[1] > 0 )
   {
      location = get_room_index( obj->value[1] );

      if( ( opponent = ch->who_fighting(  ) ) != NULL )
      {
         ch->print( "&RYou cannot recall during a fight!\r\n" );
         return;
      }

      if( !location )
      {
         ch->print( "There is a problem with your rune. Contact the immortals.\r\n" );
         bug( "%s: Rune with NULL room!", __FUNCTION__ );
         return;
      }

      act( AT_MAGIC, "$n disappears in a swirl of smoke.", ch, NULL, NULL, TO_ROOM );
      ch->print( "&[magic]You invoke the rune and are instantly transported away!\r\n" );

      leave_map( ch, NULL, location );

      obj->value[2] -= 1;
      if( obj->value[2] == 0 )
      {
         STRFREE( obj->short_descr );
         obj->short_descr = STRALLOC( "A depleted recall rune" );
         ch->print( "The rune hums softly and is now depleted of power.\r\n" );
      }
      act( AT_ACTION, "$n appears in the room.", ch, NULL, NULL, TO_ROOM );
      return;
   }
   else
   {
      ch->print( "&RHow are you going to recall with THAT?!?\r\n" );
      return;
   }
}

CMDF( do_mark )
{
   obj_data *obj;
   char arg[MIL];

   argument = one_argument( argument, arg );

   if( !arg || arg[0] == '\0' )
   {
      ch->print( "Mark what?\r\n" );
      return;
   }

   if( ms_find_obj( ch ) )
      return;

   if( !( obj = ch->get_obj_carry( arg ) ) )
   {
      ch->print( "You do not have that item.\r\n" );
      return;
   }

   if( obj->item_type != ITEM_RUNE )
   {
      ch->print( "&RHow can you mark something that's not a rune?\r\n" );
      return;
   }
   else
   {
      if( obj->value[1] > 0 )
      {
         ch->print( "&RThat rune is already marked!\r\n" );
         return;
      }

      if( !argument || argument[0] == '\0' )
      {
         ch->print( "Ok, but how do you want to identify this rune?\r\n" );
         return;
      }

      if( ch->in_room->flags.test( ROOM_NO_RECALL ) || ch->in_room->area->flags.test( AFLAG_NORECALL ) )
      {
         ch->print( "&RAncient magiks prevent the rune from taking the mark here.\r\n" );
         return;
      }

      if( ch->has_pcflag( PCFLAG_ONMAP ) || ch->has_actflag( ACT_ONMAP ) )
      {
         ch->print( "You cannot mark runes on the overland.\r\n" );
         return;
      }

      obj->value[1] = ch->in_room->vnum;
      obj->value[2] = 5;
      stralloc_printf( &obj->name, "%s rune", argument );
      stralloc_printf( &obj->short_descr, "A recall rune to %s", capitalize( argument ) );
      STRFREE( obj->objdesc );
      obj->objdesc = STRALLOC( "A small magical rune with a mark inscribed on it lies here." );
      ch->print( "&BYou mark the rune with your location.\r\n" );
      return;
   }
}

/* Connect pieces of an ITEM -- Originally from ACK! Modified for Smaug by Zarius 5/19/2000 */
CMDF( do_connect )
{
   obj_data *first_ob, *second_ob, *new_ob;
   char arg1[MSL];

   argument = one_argument( argument, arg1 );

   if( !arg1 || arg1[0] == '\0' || !argument || argument[0] == '\0' )
   {
      ch->print( "Syntax: Connect <firstobj> <secondobj>.\r\n" );
      return;
   }

   if( !( first_ob = ch->get_obj_carry( arg1 ) ) )
   {
      ch->print( "You must be holding both parts!!\r\n" );
      return;
   }

   if( !( second_ob = ch->get_obj_carry( argument ) ) )
   {
      ch->print( "You must be holding both parts!!\r\n" );
      return;
   }

   if( first_ob->item_type != ITEM_PIECE || second_ob->item_type != ITEM_PIECE )
   {
      ch->print( "Both items must be pieces of another item!\r\n" );
      return;
   }

   first_ob->separate(  );
   second_ob->separate(  );

   /*
    * check to see if the pieces connect 
    */
   if( first_ob->value[0] == second_ob->pIndexData->vnum && second_ob->value[0] == first_ob->pIndexData->vnum )
   {
      /*
       * good connection  
       */
      if( !( new_ob = get_obj_index( first_ob->value[2] )->create_object( ch->level ) ) )
      {
         log_printf( "create_object: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
         return;
      }
      first_ob->extract(  );
      second_ob->extract(  );
      new_ob->to_char( ch );
      act( AT_ACTION, "$n jiggles some pieces together...\r\n...suddenly they snap in place, creating $p!", ch, new_ob, NULL,
           TO_ROOM );
      act( AT_ACTION, "You jiggle the pieces together...\r\n...suddenly they snap into place, creating $p!", ch, new_ob,
           NULL, TO_CHAR );
   }
   else
   {
      /*
       * bad connection 
       */
      act( AT_ACTION, "$n jiggles some pieces together, but can't seem to make them connect.", ch, NULL, NULL, TO_ROOM );
      act( AT_ACTION, "You try to fit them together every which way, but they just don't want to fit together.",
           ch, NULL, NULL, TO_CHAR );
      return;
   }
   return;
}

/* Junk command installed by Samson 1-13-98 */
/* Code courtesy of Stu, from the mailing list. Allows player to destroy an item in their inventory */
CMDF( do_junk )
{
   obj_data *obj;

   if( !argument || argument[0] == '\0' )
   {
      ch->print( "Junk what?\r\n" );
      return;
   }

   if( !( obj = ch->get_obj_carry( argument ) ) )
   {
      ch->print( "You are not carrying that!\r\n" );
      return;
   }

   if( !ch->can_drop_obj( obj ) && !ch->is_immortal(  ) )
   {
      ch->print( "You cannot junk that, it's cursed!\r\n" );
      return;
   }
   obj->separate(  );
   obj->from_char(  );
   obj->extract(  );
   act( AT_ACTION, "$n junks $p.", ch, obj, NULL, TO_ROOM );
   act( AT_ACTION, "You junk $p.", ch, obj, NULL, TO_CHAR );
   return;
}

/* Donate command installed by Samson 2-6-98 
   Coded by unknown author. Players can donate items for others to use. */
/* Slight bug corrected, objects weren't being seperated from each other - Whir 3-25-98 */
CMDF( do_donate )
{
   obj_data *obj;

   if( !argument || argument[0] == '\0' )
   {
      ch->print( "Donate what?\r\n" );
      return;
   }

   if( ch->position > POS_SITTING && ch->position < POS_STANDING )
   {
      ch->print( "You cannot donate while fighting!\r\n" );
      return;
   }

   if( !( obj = ch->get_obj_carry( argument ) ) )
   {
      ch->print( "You do not have that!\r\n" );
      return;
   }
   else
   {
      if( !ch->can_drop_obj( obj ) && !ch->is_immortal(  ) )
      {
         ch->print( "You cannot donate that, it's cursed!\r\n" );
         return;
      }

      if( ( obj->item_type == ITEM_CORPSE_NPC ) || ( obj->item_type == ITEM_CORPSE_PC ) )
      {
         ch->print( "You cannot donate corpses!\r\n" );
         return;
      }

      if( obj->timer > 0 )
      {
         ch->print( "You cannot donate that.\r\n" );
         return;
      }

      if( obj->extra_flags.test( ITEM_PERSONAL ) )
      {
         ch->print( "You cannot donate personal items.\r\n" );
         return;
      }
      obj->separate(  );
      obj->extra_flags.set( ITEM_DONATION );
      obj->from_char(  );
      act( AT_ACTION, "You donate $p, how generous of you!", ch, obj, NULL, TO_CHAR );
      obj->to_room( get_room_index( ROOM_VNUM_DONATION ), NULL );
      ch->save(  );
      return;
   }
}
