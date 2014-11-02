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
 *                               Event Handlers                             *
 ****************************************************************************/

#include "mud.h"
#include "area.h"
#include "auction.h"
#include "descriptor.h"
#include "event.h"
#include "mud_prog.h"
#include "roomindex.h"
#if !defined(__CYGWIN__) && defined(SQL)
#include "sql.h"
#endif

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
CMDF( do_ageattack );
void talk_auction( const char *, ... );
void save_aucvault( char_data *, const string & );
void add_sale( const string &, const string &, const string &, const string &, int, bool );
void fly_skyship( char_data *, char_data * );
void purge_skyship( char_data *, char_data * );
void check_pfiles( time_t );
void check_boards(  );
void prune_dns(  );
void web_who(  );

int reboot_counter;

/* Replaces violence_update */
void ev_violence( void *data )
{
   char_data *ch = ( char_data * ) data;

   if( !ch )
   {
      bug( "%s: NULL ch pointer!", __FUNCTION__ );
      return;
   }
   char_data *victim = ch->who_fighting(  );

   if( !victim || !ch->in_room || !ch->name || ( ch->in_room != victim->in_room ) )
   {
      ch->stop_fighting( true );
      return;
   }

   if( ch->char_died(  ) )
   {
      ch->stop_fighting( true );
      return;
   }

   /*
    * Let the battle begin! 
    */
   if( ch->has_aflag( AFF_PARALYSIS ) )
   {
      add_event( 2, ev_violence, ch );
      return;
   }

   ch_ret retcode = rNONE;

   if( ch->in_room->flags.test( ROOM_SAFE ) )
   {
      log_printf( "%s: %s fighting %s in a SAFE room.", __FUNCTION__, ch->name, victim->name );
      ch->stop_fighting( true );
   }
   else if( ch->IS_AWAKE(  ) && ch->in_room == victim->in_room )
      retcode = multi_hit( ch, victim, TYPE_UNDEFINED );
   else
      ch->stop_fighting( false );

   if( ch->char_died(  ) )
      return;

   if( retcode == rCHAR_DIED || !( victim = ch->who_fighting(  ) ) )
      return;

   /*
    *  Mob triggers
    *  -- Added some victim death checks, because it IS possible.. -- Alty
    */
   rprog_rfight_trigger( ch );
   if( ch->char_died(  ) || victim->char_died(  ) )
      return;

   mprog_hitprcnt_trigger( ch, victim );
   if( ch->char_died(  ) || victim->char_died(  ) )
      return;

   mprog_fight_trigger( ch, victim );
   if( ch->char_died(  ) || victim->char_died(  ) )
      return;

   /*
    * NPC special attack flags - Thoric
    */
   int attacktype = -2, cnt = -2;
   if( ch->isnpc(  ) )
   {
      if( ch->has_attacks(  ) )
      {
         attacktype = -1;
         if( 30 + ( ch->level / 4 ) >= number_percent(  ) )
         {
            cnt = 0;
            for( ;; )
            {
               if( cnt++ > 10 )
               {
                  attacktype = -1;
                  break;
               }
               attacktype = number_range( 7, MAX_ATTACK_TYPE - 1 );
               if( ch->has_attack( attacktype ) )
                  break;
            }
            switch ( attacktype )
            {
               default:
                  break;
               case ATCK_BASH:
                  interpret( ch, "bash" );
                  retcode = global_retcode;
                  break;
               case ATCK_STUN:
                  interpret( ch, "stun" );
                  retcode = global_retcode;
                  break;
               case ATCK_GOUGE:
                  interpret( ch, "gouge" );
                  retcode = global_retcode;
                  break;
               case ATCK_AGE:
                  do_ageattack( ch, "" );
                  retcode = global_retcode;
                  break;
               case ATCK_DRAIN:
                  retcode = spell_energy_drain( skill_lookup( "energy drain" ), ch->level, ch, victim );
                  break;
               case ATCK_FIREBREATH:
                  retcode = spell_fire_breath( skill_lookup( "fire breath" ), ch->level, ch, victim );
                  break;
               case ATCK_FROSTBREATH:
                  retcode = spell_frost_breath( skill_lookup( "frost breath" ), ch->level, ch, victim );
                  break;
               case ATCK_ACIDBREATH:
                  retcode = spell_acid_breath( skill_lookup( "acid breath" ), ch->level, ch, victim );
                  break;
               case ATCK_LIGHTNBREATH:
                  retcode = spell_lightning_breath( skill_lookup( "lightning breath" ), ch->level, ch, victim );
                  break;
               case ATCK_GASBREATH:
                  retcode = spell_gas_breath( skill_lookup( "gas breath" ), ch->level, ch, victim );
                  break;
               case ATCK_SPIRALBLAST:
                  retcode = spell_spiral_blast( skill_lookup( "spiral blast" ), ch->level, ch, victim );
                  break;
               case ATCK_POISON:
                  retcode = spell_smaug( gsn_poison, ch->level, ch, victim );
                  break;
               case ATCK_NASTYPOISON:
                  retcode = spell_smaug( gsn_poison, ch->level, ch, victim );
                  break;
               case ATCK_GAZE:
                  break;
               case ATCK_BLINDNESS:
                  retcode = spell_smaug( skill_lookup( "blindness" ), ch->level, ch, victim );
                  break;
               case ATCK_CAUSESERIOUS:
                  retcode = spell_smaug( skill_lookup( "cause serious" ), ch->level, ch, victim );
                  break;
               case ATCK_EARTHQUAKE:
                  retcode = spell_smaug( skill_lookup( "earthquake" ), ch->level, ch, victim );
                  break;
               case ATCK_CAUSECRITICAL:
                  retcode = spell_smaug( skill_lookup( "cause critical" ), ch->level, ch, victim );
                  break;
               case ATCK_CURSE:
                  retcode = spell_smaug( skill_lookup( "curse" ), ch->level, ch, victim );
                  break;
               case ATCK_FLAMESTRIKE:
                  retcode = spell_smaug( skill_lookup( "flamestrike" ), ch->level, ch, victim );
                  break;
               case ATCK_HARM:
                  retcode = spell_smaug( skill_lookup( "harm" ), ch->level, ch, victim );
                  break;
               case ATCK_FIREBALL:
                  retcode = spell_smaug( skill_lookup( "fireball" ), ch->level, ch, victim );
                  break;
               case ATCK_COLORSPRAY:
                  retcode = spell_smaug( skill_lookup( "colour spray" ), ch->level, ch, victim );
                  break;
               case ATCK_WEAKEN:
                  retcode = spell_smaug( skill_lookup( "weaken" ), ch->level, ch, victim );
                  break;
            }
            if( attacktype != -1 && ( retcode == rCHAR_DIED || ch->char_died(  ) ) )
               return;
         }
      }

      /*
       * NPC special defense flags - Thoric
       */
      if( ch->has_defenses(  ) )
      {
         attacktype = -1;
         if( 50 + ( ch->level / 4 ) > number_percent(  ) )
         {
            cnt = 0;
            for( ;; )
            {
               if( cnt++ > 10 )
               {
                  attacktype = -1;
                  break;
               }
               attacktype = number_range( 2, MAX_DEFENSE_TYPE - 1 );
               if( ch->has_defense( attacktype ) )
                  break;
            }

            switch ( attacktype )
            {
               default:
                  break;
               case DFND_CURELIGHT:
                  act( AT_MAGIC, "$n mutters a few incantations...and looks a little better.", ch, NULL, NULL, TO_ROOM );
                  retcode = spell_smaug( skill_lookup( "cure light" ), ch->level, ch, ch );
                  break;
               case DFND_CURESERIOUS:
                  act( AT_MAGIC, "$n mutters a few incantations...and looks a bit better.", ch, NULL, NULL, TO_ROOM );
                  retcode = spell_smaug( skill_lookup( "cure serious" ), ch->level, ch, ch );
                  break;
               case DFND_CURECRITICAL:
                  act( AT_MAGIC, "$n mutters a few incantations...and looks healthier.", ch, NULL, NULL, TO_ROOM );
                  retcode = spell_smaug( skill_lookup( "cure critical" ), ch->level, ch, ch );
                  break;
               case DFND_HEAL:
                  act( AT_MAGIC, "$n mutters a few incantations...and looks much healthier.", ch, NULL, NULL, TO_ROOM );
                  retcode = spell_smaug( skill_lookup( "heal" ), ch->level, ch, ch );
                  break;
               case DFND_DISPELMAGIC:
                  if( !victim->affects.empty(  ) )
                  {
                     act( AT_MAGIC, "$n utters an incantation...", ch, NULL, NULL, TO_ROOM );
                     retcode = spell_dispel_magic( skill_lookup( "dispel magic" ), ch->level, ch, victim );
                  }
                  break;
               case DFND_DISPELEVIL:
                  act( AT_MAGIC, "$n utters an incantation...", ch, NULL, NULL, TO_ROOM );
                  retcode = spell_dispel_evil( skill_lookup( "dispel evil" ), ch->level, ch, victim );
                  break;
               case DFND_SANCTUARY:
                  if( !ch->has_aflag( AFF_SANCTUARY ) )
                  {
                     act( AT_MAGIC, "$n utters a few incantations...", ch, NULL, NULL, TO_ROOM );
                     retcode = spell_smaug( skill_lookup( "sanctuary" ), ch->level, ch, ch );
                  }
                  else
                     retcode = rNONE;
                  break;
            }
            if( attacktype != -1 && ( retcode == rCHAR_DIED || ch->char_died(  ) ) )
               return;
         }
      }
   }

   // Attack values reset - past here they mean nothing and I'm on a bughunt dammit!
   attacktype = -2;
   cnt = -2;
   retcode = -2;

   /*
    * Fun for the whole family!
    */
   list < char_data * >::iterator ich;
   for( ich = ch->in_room->people.begin(  ); ich != ch->in_room->people.end(  ); )
   {
      char_data *rch = *ich;
      ++ich;

      if( ch->in_room != rch->in_room )
         break;

      if( rch->IS_AWAKE(  ) && !rch->fighting )
      {
         /*
          * PC's auto-assist others in their group.
          */
         if( !ch->isnpc(  ) || ch->has_aflag( AFF_CHARM ) || ch->has_actflag( ACT_PET ) )
         {
            if( rch->isnpc(  ) && ( rch->has_aflag( AFF_CHARM ) || rch->is_pet(  ) ) )
            {
               multi_hit( rch, victim, TYPE_UNDEFINED );
               continue;
            }
            if( is_same_group( ch, rch ) && rch->has_pcflag( PCFLAG_AUTOASSIST ) )
            {
               multi_hit( rch, victim, TYPE_UNDEFINED );
               continue;
            }
         }

         /*
          * NPC's assist NPC's of same type or 12.5% chance regardless.
          */
         if( !rch->is_pet(  ) && !rch->has_aflag( AFF_CHARM ) && !rch->has_actflag( ACT_NOASSIST ) )
         {
            if( ch->char_died(  ) )
               break;
            if( rch->pIndexData == ch->pIndexData || number_bits( 3 ) == 0 )
            {
               list < char_data * >::iterator ich2;
               char_data *target = NULL;
               int number = 0;

               for( ich2 = ch->in_room->people.begin(  ); ich2 != ch->in_room->people.end(  ); ++ich2 )
               {
                  char_data *vch = *ich2;

                  if( rch->can_see( vch, false ) && is_same_group( vch, victim ) && number_range( 0, number ) == 0 )
                  {
                     if( vch->mount && vch->mount == rch )
                        target = NULL;
                     else
                     {
                        target = vch;
                        ++number;
                     }
                  }
               }
               if( target )
                  multi_hit( rch, target, TYPE_UNDEFINED );
            }
         }
      }
   }

   /*
    * If we are both still here lets get together and do it again some time :) 
    */
   if( ch && victim && victim->position != POS_DEAD && ch->position != POS_DEAD )
      add_event( 2, ev_violence, ch );
}

/* Replaces area_update */
void ev_area_reset( void *data )
{
   area_data *area;

   area = ( area_data * ) data;

   if( area->resetmsg && str_cmp( area->resetmsg, "" ) )
   {
      list < descriptor_data * >::iterator ds;

      for( ds = dlist.begin(  ); ds != dlist.end(  ); ++ds )
      {
         descriptor_data *d = *ds;
         char_data *ch = d->original ? d->original : d->character;

         if( !ch )
            continue;
         if( ch->IS_AWAKE(  ) && ch->in_room && ch->in_room->area == area )
            act( AT_RESET, "$t", ch, area->resetmsg, NULL, TO_CHAR );
      }
   }

   area->reset(  );
   area->last_resettime = current_time;
   add_event( number_range( ( area->reset_frequency * 60 ) / 2, 3 * ( area->reset_frequency * 60 ) / 2 ), ev_area_reset, area );
}

/* Replaces auction_update */
void ev_auction( void *data )
{
   room_index *aucvault;

   if( !auction->item )
      return;

   switch ( ++auction->going )   /* increase the going state */
   {
      default:
      case 1: /* going once */
      case 2: /* going twice */
      {
         talk_auction( "%s: going %s for %d.", auction->item->short_descr, ( ( auction->going == 1 ) ? "once" : "twice" ), auction->bet );

         add_event( sysdata->auctionseconds, ev_auction, NULL );
      }
         break;

      case 3: /* SOLD! */
         if( !auction->buyer && auction->bet )
         {
            bug( "%s: Auction code reached SOLD, with NULL buyer, but %d gold bid", __FUNCTION__, auction->bet );
            auction->bet = 0;
         }
         if( auction->bet > 0 && auction->buyer != auction->seller )
         {
            obj_data *obj = auction->item;

            aucvault = get_room_index( auction->seller->in_room->vnum + 1 );

            talk_auction( "%s sold to %s for %d gold.", auction->item->short_descr,
                          auction->buyer->isnpc(  )? auction->buyer->short_descr : auction->buyer->name, auction->bet );

            if( auction->item->buyer ) /* Set final buyer for item - Samson 6-23-99 */
            {
               STRFREE( auction->item->buyer );
               auction->item->buyer = STRALLOC( auction->buyer->name );
            }

            auction->item->bid = auction->bet;  /* Set final bid for item - Samson 6-23-99 */

            /*
             * Stop it from listing on house lists - Samson 11-1-99 
             */
            auction->item->extra_flags.test( ITEM_AUCTION );

            /*
             * Reset the 1 year timer on the item - Samson 11-1-99 
             */
            auction->item->day = time_info.day;
            auction->item->month = time_info.month;
            auction->item->year = time_info.year + 1;

            auction->item->to_room( aucvault, auction->seller );
            save_aucvault( auction->seller, auction->seller->short_descr );

            add_sale( auction->seller->short_descr, obj->seller, auction->buyer->name, obj->short_descr, obj->bid, false );

            auction->item = NULL;   /* reset item */
            obj = NULL;
         }
         else  /* not sold */
         {
            aucvault = get_room_index( auction->seller->in_room->vnum + 1 );

            talk_auction( "No bids received for %s - removed from auction.\r\n", auction->item->short_descr );
            talk_auction( "%s has been returned to the auction house.\r\n", auction->item->short_descr );

            auction->item->to_room( aucvault, auction->seller );
            save_aucvault( auction->seller, auction->seller->short_descr );
         }  /* else */
         auction->item = NULL;   /* clear auction */
   }  /* switch */
}

/* Replaces reboot_check from update.c */
void ev_reboot_count( void *data )
{
   if( reboot_counter == -5 )
   {
      echo_to_all( "&GReboot countdown cancelled.", ECHOTAR_ALL );
      return;
   }

   --reboot_counter;
   if( reboot_counter < 1 )
   {
      echo_to_all( "&YReboot countdown completed. System rebooting.", ECHOTAR_ALL );
      log_string( "Rebooted by countdown" );

      mud_down = true;
      return;
   }

   if( reboot_counter > 2 )
      echo_all_printf( ECHOTAR_ALL, "&YGame reboot in %d minutes.", reboot_counter );
   if( reboot_counter == 2 )
      echo_to_all( "&RGame reboot in 2 minutes.", ECHOTAR_ALL );
   if( reboot_counter == 1 )
      echo_to_all( "}RGame reboot in 1 minute!! Please find somewhere to log off.", ECHOTAR_ALL );

   add_event( 60, ev_reboot_count, NULL );
}

void ev_skyship( void *data )
{
   char_data *ch = ( char_data * ) data;

   if( !ch )
      return;

   if( ch->isnpc(  ) && ch->inflight )
   {
      fly_skyship( ch->my_rider, ch );
      add_event( 3, ev_skyship, ch );
   }

   if( !ch->isnpc(  ) && ch->inflight )
   {
      fly_skyship( ch, ch->my_skyship );
      add_event( 3, ev_skyship, ch );
   }

   /*
    * skyship idling Function
    */
   if( ch->isnpc(  ) && ch->my_rider && !ch->inflight )
   {
      ++ch->zzzzz;

      /*
       * First boredom warning   salt to taste 
       */
      if( ch->zzzzz == 20 )
         ch->print_room( "&CThe skyship pilot is growing restless. Perhaps you should decide where you want to go?\r\n" );

      /*
       * Second boredom warning   salt to taste 
       */
      if( ch->zzzzz == 40 )
         ch->print_room( "&CThe skyship pilot is making preparations to leave, you had better decide soon.\r\n" );

      /*
       * Third strike, yeeeer out!   salt to taste 
       */
      if( ch->zzzzz == 60 )
      {
         ch->print_room( "&CThe skyship pilot ascends to the winds without you.\r\n" );
         purge_skyship( ch->my_rider, ch );
         return;
      }
      add_event( 3, ev_skyship, ch );
   }
}

void ev_pfile_check( void *data )
{
   check_pfiles( 0 );
   add_event( 86400, ev_pfile_check, NULL );
}

void ev_board_check( void *data )
{
   check_boards(  );
   log_string( "Next board pruning in 24 hours." );
   add_event( 86400, ev_board_check, NULL );
}

void ev_dns_check( void *data )
{
   prune_dns(  );
   add_event( 86400, ev_dns_check, NULL );
}

void ev_webwho_refresh( void *data )
{
   web_who(  );
   add_event( sysdata->webwho, ev_webwho_refresh, NULL );
}

#if !defined(__CYGWIN__) && defined(SQL)
void ev_mysql_ping( void *data )
{
   mysql_ping( &myconn );
   add_event( 1800, ev_mysql_ping, NULL );
}
#endif
