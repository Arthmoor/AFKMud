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
 *                          Oasis OLC Module                                *
 ****************************************************************************/

/****************************************************************************
 * ResortMUD 4.0 Beta by Ntanel, Garinan, Badastaz, Josh, Digifuzz, Senir,  *
 * Kratas, Scion, Shogar and Tagith.  Special thanks to Thoric, Nivek,      *
 * Altrag, Arlorn, Justice, Samson, Dace, HyperEye and Yakkov.              *
 ****************************************************************************
 * Copyright (C) 1996 - 2001 Haslage Net Electronics: MudWorld              *
 * of Lorain, Ohio - ALL RIGHTS RESERVED                                    *
 * The text and pictures of this publication, or any part thereof, may not  *
 * be reproduced or transmitted in any form or by any means, electronic or  *
 * mechanical, includes photocopying, recording, storage in a information   *
 * retrieval system, or otherwise, without the prior written or e-mail      *
 * consent from the publisher.                                              *
 ****************************************************************************
 * GREETING must mention ResortMUD programmers and the help file named      *
 * CREDITS must remain completely intact as listed in the SMAUG license.    *
 ****************************************************************************/

/**************************************************************************\
 *                                                                        *
 *     OasisOLC II for Smaug 1.40 written by Evan Cortens(Tagith)         *
 *                                                                        *
 *   Based on OasisOLC for CircleMUD3.0bpl9 written by Harvey Gilpin      *
 *                                                                        *
 **************************************************************************
 *                                                                        *
 *               Mobile/Player editing module (medit.c) v1.0              *
 *                                                                        *
\**************************************************************************/

#include "mud.h"
#include "area.h"
#include "descriptor.h"
#include "mobindex.h"
#include "mspecial.h"
#include "roomindex.h"

/* Global Variables */
int SPEC_MAX;

string specmenu[100];

/* Function prototypes */
void medit_disp_menu( descriptor_data * );
int calc_thac0( char_data *, char_data *, int );
void cleanup_olc( descriptor_data * );
void olc_log( descriptor_data *, const char *, ... ) __attribute__ ( ( format( printf, 2, 3 ) ) );
int mob_xp( char_data * );
bool can_mmodify( char_data *, char_data * );

CMDF( do_omedit )
{
   char_data *victim;

   if( ch->isnpc(  ) )
   {
      ch->print( "I don't think so...\r\n" );
      return;
   }

   if( argument.empty(  ) )
   {
      ch->print( "OEdit what?\r\n" );
      return;
   }

   if( !( victim = ch->get_char_world( argument ) ) )
   {
      ch->print( "Nothing like that in hell, earth, or heaven.\r\n" );
      return;
   }

   if( !victim->isnpc(  ) )
   {
      ch->print( "PC editing is not allowed through the menu system.\r\n" );
      return;
   }

   if( ch->get_trust(  ) < sysdata->level_modify_proto )
   {
      ch->print( "Huh?\r\n" );
      return;
   }

   /*
    * Make sure the mob isnt already being edited 
    */
   list < descriptor_data * >::iterator ds;
   descriptor_data *d;
   for( ds = dlist.begin(  ); ds != dlist.end(  ); ++ds )
   {
      d = *ds;

      if( d->connected == CON_MEDIT )
         if( d->olc && OLC_VNUM( d ) == victim->pIndexData->vnum )
         {
            ch->printf( "That mob is currently being edited by %s.\r\n", d->character->name );
            return;
         }
   }
   if( !can_mmodify( ch, victim ) )
      return;

   d = ch->desc;
   d->olc = new olc_data;
   OLC_VNUM( d ) = victim->pIndexData->vnum;
   /*
    * medit_setup( d, OLC_VNUM(d) ); 
    */
   d->character->pcdata->dest_buf = victim;
   d->connected = CON_MEDIT;
   OLC_CHANGE( d ) = false;
   medit_disp_menu( d );

   act( AT_ACTION, "$n starts using OLC.", ch, nullptr, nullptr, TO_ROOM );
}

CMDF( do_mcopy )
{
   area_data *pArea;
   string arg1;
   int ovnum, cvnum;
   mob_index *orig;

   argument = one_argument( argument, arg1 );
   if( arg1.empty(  ) || argument.empty(  ) )
   {
      ch->print( "Usage: mcopy <original> <new>\r\n" );
      return;
   }

   if( !is_number( arg1 ) || !is_number( argument ) )
   {
      ch->print( "Values must be numeric.\r\n" );
      return;
   }

   ovnum = atoi( arg1.c_str(  ) );
   cvnum = atoi( argument.c_str(  ) );

   if( ch->get_trust(  ) < LEVEL_GREATER )
   {
      if( !ch->pcdata || !( pArea = ch->pcdata->area ) )
      {
         ch->print( "You must have an assigned area to copy objects.\r\n" );
         return;
      }
      if( cvnum < pArea->low_vnum || cvnum > pArea->hi_vnum )
      {
         ch->print( "That number is not in your allocated range.\r\n" );
         return;
      }
   }
   else
   {
      if( !( pArea = ch->pcdata->area ) )
         pArea = ch->in_room->area;
   }

   if( get_mob_index( cvnum ) )
   {
      ch->print( "Target vnum already exists.\r\n" );
      return;
   }

   if( !( orig = get_mob_index( ovnum ) ) )
   {
      ch->print( "Source vnum doesn't exist.\r\n" );
      return;
   }
   make_mobile( cvnum, ovnum, orig->player_name, pArea );
   ch->print( "Mobile copied.\r\n" );
}

/**************************************************************************
 Menu Displaying Functions
 **************************************************************************/

/*
 * Display poistions (sitting, standing etc), same for pos and defpos
 */
void medit_disp_positions( descriptor_data * d )
{
   d->write_to_buffer( "50\x1B[;H\x1B[2J" );
   for( int i = 0; i < POS_MAX; ++i )
      d->character->printf( "&g%2d&w) %s\r\n", i, capitalize( npc_position[i] ) );
   d->character->print( "Enter position number : " );
}

/*
 * Display mobile sexes, this is hard coded cause it just works that way :)
 */
void medit_disp_sex( descriptor_data * d )
{
   d->write_to_buffer( "50\x1B[;H\x1B[2J" );
   for( int i = 0; i < SEX_MAX; ++i )
      d->character->printf( "&g%2d&w) %s\r\n", i, capitalize( npc_sex[i] ) );
   d->character->print( "\r\nEnter gender number : " );
}

void spec_menu( void )
{
   list < string >::iterator specfun;
   int j = 0;

   specmenu[0] = "None";

   for( specfun = speclist.begin(  ); specfun != speclist.end(  ); ++specfun )
   {
      string spec = *specfun;

      ++j;
      specmenu[j] = spec;
   }
   SPEC_MAX = j + 1;
}

void medit_disp_spec( descriptor_data * d )
{
   char_data *ch = d->character;
   int col = 0;

   /*
    * Initialize this baby - hopefully it works?? 
    */
   spec_menu(  );

   d->write_to_buffer( "50\x1B[;H\x1B[2J" );
   for( int counter = 0; counter < SPEC_MAX; ++counter )
   {
      ch->printf( "&g%2d&w) %-30.30s ", counter, specmenu[counter].c_str(  ) );
      if( ++col % 2 == 0 )
         ch->print( "\r\n" );
   }
   ch->print( "\r\nEnter number of special: " );
}

/*
 * Used for both mob affected_by and object affect bitvectors
 */
void medit_disp_ris( descriptor_data * d )
{
   char_data *victim = ( char_data * ) d->character->pcdata->dest_buf;

   d->write_to_buffer( "50\x1B[;H\x1B[2J" );

   for( int counter = 0; counter <= MAX_RIS_FLAG; ++counter )
      d->character->printf( "&g%2d&w) %-20.20s\r\n", counter + 1, ris_flags[counter] );

   if( d->connected == CON_OEDIT )
   {
      switch ( OLC_MODE( d ) )
      {
         default:
            break;

         case OEDIT_AFFECT_MODIFIER:
            d->character->printf( "\r\nCurrent flags: &c%s&w\r\n", flag_string( d->character->tempnum, ris_flags ) );
            break;
      }
   }
   else if( d->connected == CON_MEDIT )
   {
      switch ( OLC_MODE( d ) )
      {
         default:
            break;
         case MEDIT_RESISTANT:
            d->character->printf( "\r\nCurrent flags: &c%s&w\r\n", bitset_string( victim->get_resists(  ), ris_flags ) );
            break;
         case MEDIT_IMMUNE:
            d->character->printf( "\r\nCurrent flags: &c%s&w\r\n", bitset_string( victim->get_immunes(  ), ris_flags ) );
            break;
         case MEDIT_SUSCEPTIBLE:
            d->character->printf( "\r\nCurrent flags: &c%s&w\r\n", bitset_string( victim->get_susceps(  ), ris_flags ) );
            break;
            // FIX: Editing Absorb flags did not show current flags 
            // Zarius 5/19/2003 
         case MEDIT_ABSORB:
            d->character->printf( "\r\nCurrent flags: &c%s&w\r\n", bitset_string( victim->get_absorbs(  ), ris_flags ) );
            break;
      }
   }
   d->character->print( "Enter flag (0 to quit): " );
}

/*
 * Mobile attacks
 */
void medit_disp_attack_menu( descriptor_data * d )
{
   char_data *victim = ( char_data * ) d->character->pcdata->dest_buf;

   d->write_to_buffer( "50\x1B[;H\x1B[2J" );
   for( int i = 0; i < MAX_ATTACK_TYPE; ++i )
      d->character->printf( "&g%2d&w) %-20.20s\r\n", i + 1, attack_flags[i] );

   d->character->printf( "Current flags: &c%s&w\r\nEnter attack flag (0 to exit): ", bitset_string( victim->get_attacks(  ), attack_flags ) );
}

/*
 * Display menu of NPC defense flags
 */
void medit_disp_defense_menu( descriptor_data * d )
{
   char_data *victim = ( char_data * ) d->character->pcdata->dest_buf;

   d->write_to_buffer( "50\x1B[;H\x1B[2J" );
   for( int i = 0; i < MAX_DEFENSE_TYPE; ++i )
      d->character->printf( "&g%2d&w) %-20.20s\r\n", i + 1, defense_flags[i] );

   d->character->printf( "Current flags: &c%s&w\r\nEnter defense flag (0 to exit): ", bitset_string( victim->get_defenses(  ), defense_flags ) );
}

/*-------------------------------------------------------------------*/
/*. Display mob-flags menu .*/

void medit_disp_mob_flags( descriptor_data * d )
{
   char_data *victim = ( char_data * ) d->character->pcdata->dest_buf;
   int columns = 0;

   d->write_to_buffer( "50\x1B[;H\x1B[2J" );
   for( int i = 0; i < MAX_ACT_FLAG; ++i )
   {
      d->character->printf( "&g%2d&w) %-20.20s  ", i + 1, act_flags[i] );
      if( !( ++columns % 2 ) )
         d->character->print( "\r\n" );
   }
   d->character->printf( "\r\nCurrent flags : &c%s&w\r\nEnter mob flags (0 to quit) : ", bitset_string( victim->get_actflags(  ), act_flags ) );
}

/*-------------------------------------------------------------------*/
/*. Display aff-flags menu .*/

void medit_disp_aff_flags( descriptor_data * d )
{
   char_data *victim = ( char_data * ) d->character->pcdata->dest_buf;
   int i, columns = 0;

   d->write_to_buffer( "50\x1B[;H\x1B[2J" );
   for( i = 0; i < MAX_AFFECTED_BY; ++i )
   {
      d->character->printf( "&g%2d&w) %-20.20s  ", i + 1, aff_flags[i] );
      if( !( ++columns % 2 ) )
         d->character->print( "\r\n" );
   }

   if( OLC_MODE( d ) == OEDIT_AFFECT_MODIFIER )
   {
      char buf[MSL];

      buf[0] = '\0';

      for( i = 0; i < 32; ++i )
         if( IS_SET( d->character->tempnum, 1 << i ) )
         {
            mudstrlcat( buf, " ", MSL );
            mudstrlcat( buf, aff_flags[i], MSL );
         }
      d->character->printf( "\r\nCurrent flags   : &c%s&w\r\n", buf );
   }
   else
      d->character->printf( "\r\nCurrent flags   : &c%s&w\r\n", bitset_string( victim->get_aflags(  ), aff_flags ) );
   d->character->print( "Enter affected flags (0 to quit) : " );
}

void medit_disp_parts( descriptor_data * d )
{
   char_data *victim = ( char_data * ) d->character->pcdata->dest_buf;
   int columns = 0;

   d->write_to_buffer( "50\x1B[;H\x1B[2J" );
   for( int count = 0; count < MAX_BPART; ++count )
   {
      d->character->printf( "&g%2d&w) %-20.20s    ", count + 1, part_flags[count] );

      if( ++columns % 2 == 0 )
         d->character->print( "\r\n" );
   }
   d->character->printf( "\r\nCurrent flags: %s\r\nEnter flag or 0 to exit: ", bitset_string( victim->get_bparts(  ), part_flags ) );
}

void medit_disp_classes( descriptor_data * d )
{
   int columns = 0;

   d->write_to_buffer( "50\x1B[;H\x1B[2J" );
   for( int iClass = 0; iClass < MAX_NPC_CLASS; ++iClass )
   {
      d->character->printf( "&g%2d&w) %-20.20s     ", iClass, npc_class[iClass] );
      if( ++columns % 2 == 0 )
         d->character->print( "\r\n" );
   }
   d->character->print( "\r\nEnter Class: " );
}

void medit_disp_races( descriptor_data * d )
{
   int columns = 0;

   d->write_to_buffer( "50\x1B[;H\x1B[2J" );
   for( int iRace = 0; iRace < MAX_NPC_RACE; ++iRace )
   {
      d->character->printf( "&g%2d&w) %-20.20s  ", iRace, npc_race[iRace] );
      if( ++columns % 3 == 0 )
         d->character->print( "\r\n" );
   }
   d->character->print( "\r\nEnter race: " );
}

/*
 * Display main menu for NPCs
 */
void medit_disp_menu( descriptor_data * d )
{
   char_data *ch = d->character;
   char_data *mob = ( char_data * ) d->character->pcdata->dest_buf;

   d->write_to_buffer( "50\x1B[;H\x1B[2J" );
   ch->printf( "&w-- Mob Number:  [&c%d&w]\r\n", mob->pIndexData->vnum );
   ch->printf( "&g1&w) Sex: &O%s          &g2&w) Name: &O%s\r\n", npc_sex[mob->sex], mob->name );
   ch->printf( "&g3&w) Shortdesc: &O%s\r\n", mob->short_descr[0] == '\0' ? "(none set)" : mob->short_descr );
   ch->printf( "&g4&w) Longdesc:\r\n&O%s\r\n", mob->long_descr[0] == '\0' ? "(none set)" : mob->long_descr );
   ch->printf( "&g5&w) Description:\r\n&O%-74.74s\r\n\r\n", mob->chardesc ? mob->chardesc : "(none set)" );
   ch->printf( "&g6&w) Class: [&c%-11.11s&w], &g7&w) Race:   [&c%-11.11s&w]\r\n", npc_class[mob->Class], npc_race[mob->race] );
   ch->printf( "&g8&w) Level:       [&c%5d&w], &g9&w) Alignment:    [&c%5d&w]\r\n\r\n", mob->level, mob->alignment );

   ch->printf( " &w) Calc Thac0:      [&c%5d&w]\r\n", calc_thac0( mob, nullptr, 0 ) );
   ch->printf( "&gA&w) Real Thac0:  [&c%5d&w]\r\n\r\n", mob->mobthac0 );

   ch->printf( " &w) Calc Experience: [&c%10d&w]\r\n", mob->exp );
   ch->printf( "&gB&w) Real Experience: [&c%10d&w]\r\n\r\n", mob->pIndexData->exp );
   ch->printf( "&gC&w) DamNumDice:  [&c%5d&w], &gD&w) DamSizeDice:  [&c%5d&w], &gE&w) DamPlus:  [&c%5d&w]\r\n",
               mob->pIndexData->damnodice, mob->pIndexData->damsizedice, mob->pIndexData->damplus );
   ch->printf( "&gF&w) HitDice:  [&c%dd%d+%d&w]\r\n", mob->pIndexData->hitnodice, mob->pIndexData->hitsizedice, mob->pIndexData->hitplus );
   ch->printf( "&gG&w) Gold:     [&c%8d&w], &gH&w) Spec: &O%-22.22s\r\n", mob->gold, !mob->spec_funname.empty(  )? mob->spec_funname.c_str(  ) : "None" );
   ch->printf( "&gI&w) Resistant   : &O%s\r\n", bitset_string( mob->get_resists(  ), ris_flags ) );
   ch->printf( "&gJ&w) Immune      : &O%s\r\n", bitset_string( mob->get_immunes(  ), ris_flags ) );
   ch->printf( "&gK&w) Susceptible : &O%s\r\n", bitset_string( mob->get_susceps(  ), ris_flags ) );
   ch->printf( "&gL&w) Absorb      : &O%s\r\n", bitset_string( mob->get_absorbs(  ), ris_flags ) );
   ch->printf( "&gM&w) Position    : &O%s\r\n", npc_position[mob->position] );
   ch->printf( "&gN&w) Default Pos : &O%s\r\n", npc_position[mob->defposition] );
   ch->printf( "&gO&w) Attacks     : &c%s\r\n", bitset_string( mob->get_attacks(  ), attack_flags ) );
   ch->printf( "&gP&w) Defenses    : &c%s\r\n", bitset_string( mob->get_defenses(  ), defense_flags ) );
   ch->printf( "&gR&w) Body Parts  : &c%s\r\n", bitset_string( mob->get_bparts(  ), part_flags ) );
   ch->printf( "&gS&w) Act Flags   : &c%s\r\n", bitset_string( mob->get_actflags(  ), act_flags ) );
   ch->printf( "&gT&w) Affected    : &c%s\r\n", bitset_string( mob->get_aflags(  ), aff_flags ) );
   ch->printf( "&gQ&w) Quit\r\n" );
   ch->print( "Enter choice : " );

   OLC_MODE( d ) = MEDIT_NPC_MAIN_MENU;
}

/*
 * Bogus command for resetting stuff
 */
CMDF( do_medit_reset )
{
   char_data *victim = ( char_data * ) ch->pcdata->dest_buf;

   switch ( ch->substate )
   {
      default:
         return;

      case SUB_MOB_DESC:
         if( !ch->pcdata->dest_buf )
         {
            ch->print( "Fatal error, report to Samson.\r\n" );
            bug( "%s: sub_mob_desc: nullptr ch->pcdata->dest_buf", __func__ );
            cleanup_olc( ch->desc );
            ch->substate = SUB_NONE;
            return;
         }
         STRFREE( victim->chardesc );
         victim->chardesc = ch->copy_buffer( true );
         if( victim->has_actflag( ACT_PROTOTYPE ) )
         {
            STRFREE( victim->pIndexData->chardesc );
            victim->pIndexData->chardesc = QUICKLINK( victim->chardesc );
         }
         ch->stop_editing(  );
         ch->pcdata->dest_buf = victim;
         ch->substate = SUB_NONE;
         ch->desc->connected = CON_MEDIT;
         medit_disp_menu( ch->desc );
         return;
   }
}

/**************************************************************************
  The GARGANTAUN event handler
 **************************************************************************/

void medit_parse( descriptor_data * d, string & arg )
{
   char_data *victim = ( char_data * ) d->character->pcdata->dest_buf;
   int number = 0;
   string arg1;

   switch ( OLC_MODE( d ) )
   {
      case MEDIT_NPC_MAIN_MENU:
         switch ( UPPER( arg[0] ) )
         {
            case 'Q':
               cleanup_olc( d );
               return;
            case '1':
               OLC_MODE( d ) = MEDIT_SEX;
               medit_disp_sex( d );
               return;
            case '2':
               OLC_MODE( d ) = MEDIT_NAME;
               d->character->print( "\r\nEnter name: " );
               return;
            case '3':
               OLC_MODE( d ) = MEDIT_S_DESC;
               d->character->print( "\r\nEnter short description: " );
               return;
            case '4':
               OLC_MODE( d ) = MEDIT_L_DESC;
               d->character->print( "\r\nEnter long description: " );
               return;
            case '5':
               OLC_MODE( d ) = MEDIT_D_DESC;
               d->character->substate = SUB_MOB_DESC;
               d->character->last_cmd = do_medit_reset;

               d->character->print( "Enter new mob description:\r\n" );
               if( !victim->chardesc )
                  victim->chardesc = STRALLOC( "" );
               d->character->editor_desc_printf( "Mob description for vnum %d", victim->pIndexData->vnum );
               d->character->start_editing( victim->chardesc );
               return;
            case '6':
               OLC_MODE( d ) = MEDIT_CLASS;
               medit_disp_classes( d );
               return;
            case '7':
               OLC_MODE( d ) = MEDIT_RACE;
               medit_disp_races( d );
               return;
            case '8':
               OLC_MODE( d ) = MEDIT_LEVEL;
               d->character->print( "\r\nEnter level: " );
               return;
            case '9':
               OLC_MODE( d ) = MEDIT_ALIGNMENT;
               d->character->print( "\r\nEnter alignment: " );
               return;
            case 'A':
               OLC_MODE( d ) = MEDIT_THACO;
               d->character->print( "\r\nUse 21 to have the mud autocalculate Thac0\r\nEnter Thac0: " );
               return;
            case 'B':
               OLC_MODE( d ) = MEDIT_EXP;
               d->character->print( "\r\nUse -1 to have the mud autocalculate Exp\r\nEnter Exp: " );
               return;
            case 'C':
               OLC_MODE( d ) = MEDIT_DAMNUMDIE;
               d->character->print( "\r\nEnter number of damage dice: " );
               return;
            case 'D':
               OLC_MODE( d ) = MEDIT_DAMSIZEDIE;
               d->character->print( "\r\nEnter size of damage dice: " );
               return;
            case 'E':
               OLC_MODE( d ) = MEDIT_DAMPLUS;
               d->character->print( "\r\nEnter amount to add to damage: " );
               return;
            case 'F':
               OLC_MODE( d ) = MEDIT_HITPLUS;
               d->character->print( "\r\nEnter amount to add to hitpoints: " );
               return;
            case 'G':
               OLC_MODE( d ) = MEDIT_GOLD;
               d->character->print( "\r\nEnter amount of gold mobile carries: " );
               return;
            case 'H':
               OLC_MODE( d ) = MEDIT_SPEC;
               medit_disp_spec( d );
               return;
            case 'I':
               OLC_MODE( d ) = MEDIT_RESISTANT;
               medit_disp_ris( d );
               return;
            case 'J':
               OLC_MODE( d ) = MEDIT_IMMUNE;
               medit_disp_ris( d );
               return;
            case 'K':
               OLC_MODE( d ) = MEDIT_SUSCEPTIBLE;
               medit_disp_ris( d );
               return;
            case 'L':
               OLC_MODE( d ) = MEDIT_ABSORB;
               medit_disp_ris( d );
               return;
            case 'M':
               OLC_MODE( d ) = MEDIT_POS;
               medit_disp_positions( d );
               return;
            case 'N':
               OLC_MODE( d ) = MEDIT_DEFPOS;
               medit_disp_positions( d );
               return;
            case 'O':
               OLC_MODE( d ) = MEDIT_ATTACK;
               medit_disp_attack_menu( d );
               return;
            case 'P':
               OLC_MODE( d ) = MEDIT_DEFENSE;
               medit_disp_defense_menu( d );
               return;
            case 'R':
               OLC_MODE( d ) = MEDIT_PARTS;
               medit_disp_parts( d );
               return;
            case 'S':
               OLC_MODE( d ) = MEDIT_NPC_FLAGS;
               medit_disp_mob_flags( d );
               return;
            case 'T':
               OLC_MODE( d ) = MEDIT_AFF_FLAGS;
               medit_disp_aff_flags( d );
               return;
            default:
               medit_disp_menu( d );
               return;
         }
         break;

      case MEDIT_NAME:
         STRFREE( victim->name );
         victim->name = STRALLOC( arg.c_str(  ) );
         if( victim->has_actflag( ACT_PROTOTYPE ) )
         {
            STRFREE( victim->pIndexData->player_name );
            victim->pIndexData->player_name = QUICKLINK( victim->name );
         }
         olc_log( d, "Changed name to %s", arg.c_str(  ) );
         break;

      case MEDIT_S_DESC:
         STRFREE( victim->short_descr );
         victim->short_descr = STRALLOC( arg.c_str(  ) );
         if( victim->has_actflag( ACT_PROTOTYPE ) )
         {
            STRFREE( victim->pIndexData->short_descr );
            victim->pIndexData->short_descr = QUICKLINK( victim->short_descr );
         }
         olc_log( d, "Changed short desc to %s", arg.c_str(  ) );
         break;

      case MEDIT_L_DESC:
         stralloc_printf( &victim->long_descr, "%s\r\n", arg.c_str(  ) );
         if( victim->has_actflag( ACT_PROTOTYPE ) )
         {
            STRFREE( victim->pIndexData->long_descr );
            victim->pIndexData->long_descr = QUICKLINK( victim->long_descr );
         }
         olc_log( d, "Changed long desc to %s", arg.c_str(  ) );
         break;

      case MEDIT_D_DESC:
         /*
          * . We should never get here .
          */
         cleanup_olc( d );
         bug( "%s: OLC: medit_parse(): Reached D_DESC case!", __func__ );
         break;

      case MEDIT_NPC_FLAGS:
         /*
          * REDONE, again, then again 
          */
         if( is_number( arg ) )
            if( atoi( arg.c_str(  ) ) == 0 )
               break;

         while( !arg.empty(  ) )
         {
            arg = one_argument( arg, arg1 );

            if( is_number( arg1 ) )
            {
               number = atoi( arg1.c_str(  ) );
               number -= 1;

               if( number < 0 || number >= MAX_ACT_FLAG )
               {
                  d->character->print( "Invalid flag, try again: " );
                  return;
               }
            }
            else
            {
               number = get_actflag( arg1 );
               if( number < 0 )
               {
                  d->character->print( "Invalid flag, try again: " );
                  return;
               }
            }
            if( victim->isnpc(  ) && number == ACT_PROTOTYPE && d->character->get_trust(  ) < LEVEL_GREATER && !hasname( d->character->pcdata->bestowments, "protoflag" ) )
               d->character->print( "You don't have permission to change the prototype flag.\r\n" );
            else if( victim->isnpc(  ) && number == ACT_IS_NPC )
               d->character->print( "It isn't possible to change that flag.\r\n" );
            else
               victim->unset_actflag( number );
            if( victim->has_actflag( ACT_PROTOTYPE ) )
               victim->pIndexData->actflags = victim->get_actflags(  );
         }
         medit_disp_mob_flags( d );
         return;

      case MEDIT_AFF_FLAGS:
         if( is_number( arg ) )
         {
            number = atoi( arg.c_str(  ) );
            if( number == 0 )
               break;
            if( number > 0 && number < MAX_AFFECTED_BY )
            {
               number -= 1;
               victim->toggle_aflag( number );
               olc_log( d, "%s the affect %s", victim->has_aflag( number ) ? "Added" : "Removed", aff_flags[number] );
            }
         }
         else
         {
            while( !arg.empty(  ) )
            {
               arg = one_argument( arg, arg1 );
               number = get_actflag( arg1 );
               if( number > 0 )
               {
                  victim->toggle_aflag( number );
                  olc_log( d, "%s the affect %s", victim->has_aflag( number ) ? "Added" : "Removed", aff_flags[number] );
               }
            }
         }
         if( victim->has_actflag( ACT_PROTOTYPE ) )
            victim->pIndexData->affected_by = victim->get_aflags(  );
         medit_disp_aff_flags( d );
         return;

/*-------------------------------------------------------------------*/
/*. Numerical responses .*/
      case MEDIT_HITPOINT:
         victim->max_hit = URANGE( 1, atoi( arg.c_str(  ) ), 32700 );
         olc_log( d, "Changed hitpoints to %d", victim->max_hit );
         break;

      case MEDIT_MANA:
         victim->max_mana = URANGE( 1, atoi( arg.c_str(  ) ), 30000 );
         if( victim->has_actflag( ACT_PROTOTYPE ) )
            victim->pIndexData->max_mana = victim->max_mana;
         olc_log( d, "Changed mana to %d", victim->max_mana );
         break;

      case MEDIT_MOVE:
         victim->max_move = URANGE( 1, atoi( arg.c_str(  ) ), 30000 );
         if( victim->has_actflag( ACT_PROTOTYPE ) )
            victim->pIndexData->max_move = victim->max_move;
         olc_log( d, "Changed moves to %d", victim->max_move );
         break;

      case MEDIT_SEX:
         victim->sex = URANGE( 0, atoi( arg.c_str(  ) ), SEX_MAX - 1 );
         if( victim->has_actflag( ACT_PROTOTYPE ) )
            victim->pIndexData->sex = victim->sex;
         olc_log( d, "Changed sex to %s", victim->sex == SEX_MALE ? "Male" : victim->sex == SEX_FEMALE ? "Female" : victim->sex == SEX_NEUTRAL ? "Neutral" : "Hermaphrodite" );
         break;

      case MEDIT_HITROLL:
         victim->hitroll = URANGE( 0, atoi( arg.c_str(  ) ), 85 );
         if( victim->has_actflag( ACT_PROTOTYPE ) )
            victim->pIndexData->hitroll = victim->hitroll;
         olc_log( d, "Changed hitroll to %d", victim->hitroll );
         break;

      case MEDIT_DAMROLL:
         victim->damroll = URANGE( 0, atoi( arg.c_str(  ) ), 65 );
         if( victim->has_actflag( ACT_PROTOTYPE ) )
            victim->pIndexData->damroll = victim->damroll;
         olc_log( d, "Changed damroll to %d", victim->damroll );
         break;

      case MEDIT_DAMNUMDIE:
         if( victim->has_actflag( ACT_PROTOTYPE ) )
            victim->pIndexData->damnodice = URANGE( 0, atoi( arg.c_str(  ) ), 100 );
         olc_log( d, "Changed damnumdie to %d", victim->pIndexData->damnodice );
         break;

      case MEDIT_DAMSIZEDIE:
         if( victim->has_actflag( ACT_PROTOTYPE ) )
            victim->pIndexData->damsizedice = URANGE( 0, atoi( arg.c_str(  ) ), 100 );
         olc_log( d, "Changed damsizedie to %d", victim->pIndexData->damsizedice );
         break;

      case MEDIT_DAMPLUS:
         if( victim->has_actflag( ACT_PROTOTYPE ) )
            victim->pIndexData->damplus = URANGE( 0, atoi( arg.c_str(  ) ), 1000 );
         olc_log( d, "Changed damplus to %d", victim->pIndexData->damplus );
         break;

      case MEDIT_HITPLUS:
         if( victim->has_actflag( ACT_PROTOTYPE ) )
            victim->pIndexData->hitplus = URANGE( 0, atoi( arg.c_str(  ) ), 32767 );
         olc_log( d, "Changed hitplus to %d", victim->pIndexData->hitplus );
         break;

      case MEDIT_AC:
         victim->armor = URANGE( -300, atoi( arg.c_str(  ) ), 300 );
         if( victim->has_actflag( ACT_PROTOTYPE ) )
            victim->pIndexData->ac = victim->armor;
         olc_log( d, "Changed armor to %d", victim->armor );
         break;

      case MEDIT_GOLD:
         victim->gold = URANGE( -1, atoi( arg.c_str(  ) ), 2000000000 );
         if( victim->has_actflag( ACT_PROTOTYPE ) )
            victim->pIndexData->gold = victim->gold;
         olc_log( d, "Changed gold to %d", victim->gold );
         break;

      case MEDIT_POS:
         victim->position = URANGE( 0, atoi( arg.c_str(  ) ), POS_STANDING );
         if( victim->has_actflag( ACT_PROTOTYPE ) )
            victim->pIndexData->position = victim->position;
         olc_log( d, "Changed position to %d", victim->position );
         break;

      case MEDIT_DEFPOS:
         victim->defposition = URANGE( 0, atoi( arg.c_str(  ) ), POS_STANDING );
         if( victim->has_actflag( ACT_PROTOTYPE ) )
            victim->pIndexData->defposition = victim->defposition;
         olc_log( d, "Changed default position to %d", victim->defposition );
         break;

      case MEDIT_MENTALSTATE:
         victim->mental_state = URANGE( -100, atoi( arg.c_str(  ) ), 100 );
         olc_log( d, "Changed mental state to %d", victim->mental_state );
         break;

      case MEDIT_CLASS:
         number = atoi( arg.c_str(  ) );
         if( victim->isnpc(  ) )
         {
            victim->Class = URANGE( 0, number, MAX_NPC_CLASS - 1 );
            if( victim->has_actflag( ACT_PROTOTYPE ) )
               victim->pIndexData->Class = victim->Class;
            break;
         }
         victim->Class = URANGE( 0, number, MAX_CLASS );
         olc_log( d, "Changed Class to %s", npc_class[victim->Class] );
         break;

      case MEDIT_RACE:
         number = atoi( arg.c_str(  ) );
         if( victim->isnpc(  ) )
         {
            victim->race = URANGE( 0, number, MAX_NPC_RACE - 1 );
            if( victim->has_actflag( ACT_PROTOTYPE ) )
               victim->pIndexData->race = victim->race;
            break;
         }
         victim->race = URANGE( 0, number, MAX_RACE - 1 );
         olc_log( d, "Changed race to %s", npc_race[victim->race] );
         break;

      case MEDIT_PARTS:
         number = atoi( arg.c_str(  ) );
         if( number < 0 || number >= MAX_BPART )
         {
            d->character->print( "Invalid part, try again: " );
            return;
         }
         else
         {
            if( number == 0 )
               break;
            else
            {
               number -= 1;
               victim->toggle_bpart( number );
            }
            if( victim->has_actflag( ACT_PROTOTYPE ) )
               victim->pIndexData->body_parts = victim->get_bparts(  );
         }
         olc_log( d, "%s the body part %s", victim->has_bpart( number ) ? "Added" : "Removed", part_flags[number] );
         medit_disp_parts( d );
         return;

      case MEDIT_ATTACK:
         if( is_number( arg ) )
         {
            number = atoi( arg.c_str(  ) );
            if( number == 0 )
               break;

            number -= 1;   /* offset */
            if( number < 0 || number >= MAX_ATTACK_TYPE )
            {
               d->character->print( "Invalid flag, try again: " );
               return;
            }
            else
               victim->toggle_attack( number );
         }
         else
         {
            while( !arg.empty(  ) )
            {
               arg = one_argument( arg, arg1 );
               number = get_attackflag( arg1 );
               if( number < 0 || number >= MAX_ATTACK_TYPE )
               {
                  d->character->print( "Invalid flag, try again: " );
                  return;
               }
               victim->toggle_attack( number );
            }
         }
         if( victim->has_actflag( ACT_PROTOTYPE ) )
            victim->pIndexData->attacks = victim->get_attacks(  );
         medit_disp_attack_menu( d );
         olc_log( d, "%s the attack %s", victim->has_attack( number ) ? "Added" : "Removed", attack_flags[number] );
         return;

      case MEDIT_DEFENSE:
         if( is_number( arg ) )
         {
            number = atoi( arg.c_str(  ) );
            if( number == 0 )
               break;

            number -= 1;   /* offset */
            if( number < 0 || number >= MAX_DEFENSE_TYPE )
            {
               d->character->print( "Invalid flag, try again: " );
               return;
            }
            else
               victim->toggle_defense( number );
         }
         else
         {
            while( !arg.empty(  ) )
            {
               arg = one_argument( arg, arg1 );
               number = get_defenseflag( arg1 );
               if( number < 0 || number >= MAX_DEFENSE_TYPE )
               {
                  d->character->print( "Invalid flag, try again: " );
                  return;
               }
               victim->toggle_defense( number );
            }
         }
         if( victim->has_actflag( ACT_PROTOTYPE ) )
            victim->pIndexData->defenses = victim->get_defenses(  );
         medit_disp_defense_menu( d );
         olc_log( d, "%s the attack %s", victim->has_defense( number ) ? "Added" : "Removed", defense_flags[number] );
         return;

      case MEDIT_LEVEL:
         victim->level = URANGE( 1, atoi( arg.c_str(  ) ), LEVEL_IMMORTAL + 5 );
         if( victim->has_actflag( ACT_PROTOTYPE ) )
            victim->pIndexData->level = victim->level;
         olc_log( d, "Changed level to %d", victim->level );
         break;

      case MEDIT_ALIGNMENT:
         victim->alignment = URANGE( -1000, atoi( arg.c_str(  ) ), 1000 );
         if( victim->has_actflag( ACT_PROTOTYPE ) )
            victim->pIndexData->alignment = victim->alignment;
         olc_log( d, "Changed alignment to %d", victim->alignment );
         break;

      case MEDIT_EXP:
         victim->exp = atoi( arg.c_str(  ) );
         if( victim->has_actflag( ACT_PROTOTYPE ) )
            victim->pIndexData->exp = victim->exp;
         if( victim->exp == -1 )
            victim->exp = mob_xp( victim );
         olc_log( d, "Changed exp to %d", victim->exp );
         break;

      case MEDIT_THACO:
         victim->mobthac0 = atoi( arg.c_str(  ) );
         if( victim->has_actflag( ACT_PROTOTYPE ) )
            victim->pIndexData->mobthac0 = victim->mobthac0;
         olc_log( d, "Changed thac0 to %d", victim->mobthac0 );
         break;

      case MEDIT_RESISTANT:
         if( is_number( arg ) )
         {
            number = atoi( arg.c_str(  ) );
            if( number == 0 )
               break;

            number -= 1;   /* offset */
            if( number < 0 || number >= MAX_RIS_FLAG )
            {
               d->character->print( "Invalid flag, try again: " );
               return;
            }
            victim->toggle_resist( number );
         }
         else
         {
            while( !arg.empty(  ) )
            {
               arg = one_argument( arg, arg1 );
               number = get_risflag( arg1 );
               if( number < 0 )
               {
                  d->character->print( "Invalid flag, try again: " );
                  return;
               }
               victim->toggle_resist( number );
            }
         }
         if( victim->has_actflag( ACT_PROTOTYPE ) )
            victim->pIndexData->resistant = victim->get_resists(  );
         medit_disp_ris( d );
         olc_log( d, "%s the resistant %s", victim->has_resist( number ) ? "Added" : "Removed", ris_flags[number] );
         return;

      case MEDIT_IMMUNE:
         if( is_number( arg ) )
         {
            number = atoi( arg.c_str(  ) );
            if( number == 0 )
               break;

            number -= 1;
            if( number < 0 || number >= MAX_RIS_FLAG )
            {
               d->character->print( "Invalid flag, try again: " );
               return;
            }
            victim->toggle_immune( number );
         }
         else
         {
            while( !arg.empty(  ) )
            {
               arg = one_argument( arg, arg1 );
               number = get_risflag( arg1 );
               if( number < 0 )
               {
                  d->character->print( "Invalid flag, try again: " );
                  return;
               }
               victim->toggle_immune( number );
            }
         }
         if( victim->has_actflag( ACT_PROTOTYPE ) )
            victim->pIndexData->immune = victim->get_immunes(  );

         medit_disp_ris( d );
         olc_log( d, "%s the immune %s", victim->has_immune( number ) ? "Added" : "Removed", ris_flags[number] );
         return;

      case MEDIT_SUSCEPTIBLE:
         if( is_number( arg ) )
         {
            number = atoi( arg.c_str(  ) );
            if( number == 0 )
               break;

            number -= 1;
            if( number < 0 || number >= MAX_RIS_FLAG )
            {
               d->character->print( "Invalid flag, try again: " );
               return;
            }
            victim->toggle_suscep( number );
         }
         else
         {
            while( !arg.empty(  ) )
            {
               arg = one_argument( arg, arg1 );
               number = get_risflag( arg1 );
               if( number < 0 )
               {
                  d->character->print( "Invalid flag, try again: " );
                  return;
               }
               victim->toggle_suscep( number );
            }
         }
         if( victim->has_actflag( ACT_PROTOTYPE ) )
            victim->pIndexData->susceptible = victim->get_susceps(  );
         medit_disp_ris( d );
         olc_log( d, "%s the suscept %s", victim->has_suscep( number ) ? "Added" : "Removed", ris_flags[number] );
         return;

      case MEDIT_ABSORB:
         if( is_number( arg ) )
         {
            number = atoi( arg.c_str(  ) );
            if( number == 0 )
               break;

            number -= 1;   /* offset */
            if( number < 0 || number >= MAX_RIS_FLAG )
            {
               d->character->print( "Invalid flag, try again: " );
               return;
            }
            victim->toggle_absorb( number );
         }
         else
         {
            while( !arg.empty(  ) )
            {
               arg = one_argument( arg, arg1 );
               number = get_risflag( arg1 );
               if( number < 0 )
               {
                  d->character->print( "Invalid flag, try again: " );
                  return;
               }
               victim->toggle_absorb( number );
            }
         }
         if( victim->has_actflag( ACT_PROTOTYPE ) )
            victim->pIndexData->absorb = victim->get_absorbs(  );
         medit_disp_ris( d );
         olc_log( d, "%s the absorb %s", victim->has_absorb( number ) ? "Added" : "Removed", ris_flags[number] );
         return;

      case MEDIT_SPEC:
         number = atoi( arg.c_str(  ) );
         /*
          * FIX: Selecting 0 crashed the mud when editing spec procs --Zarius 5/19/2003 
          */
         if( number == 0 )
            break;
         if( number < 1 || number >= SPEC_MAX )
         {
            victim->spec_fun = nullptr;
            victim->spec_funname.clear(  );
         }
         else
         {
            victim->spec_fun = m_spec_lookup( specmenu[number] );
            victim->spec_funname = specmenu[number];
         }

         if( victim->has_actflag( ACT_PROTOTYPE ) )
         {
            victim->pIndexData->spec_fun = victim->spec_fun;
            victim->pIndexData->spec_funname = victim->spec_funname;
         }
         olc_log( d, "Changes spec_func to %s", !victim->spec_funname.empty(  )? victim->spec_funname.c_str(  ) : "None" );
         break;

      default:
         /*
          * . We should never get here .
          */
         bug( "%s: OLC: medit_parse(): Reached default case!", __func__ );
         cleanup_olc( d );
         return;
   }

/*. END OF CASE 
   If we get here, we have probably changed something, and now want to
   return to main menu.  Use OLC_CHANGE as a 'has changed' flag .*/

   OLC_CHANGE( d ) = true;
   medit_disp_menu( d );
}

/*. End of medit_parse() .*/
