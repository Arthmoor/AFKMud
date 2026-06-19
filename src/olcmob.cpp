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

std::string specmenu[100];

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
    * Make sure the mob isn't already being edited
    */
   std::list<descriptor_data *>::iterator ds;
   descriptor_data *d;
   for( ds = dlist.begin(  ); ds != dlist.end(  ); ++ds )
   {
      d = *ds;

      if( d->connected == CON_MEDIT )
         if( d->olc && d->olc->number == victim->pIndexData->vnum )
         {
            ch->print_fmt( "That mob is currently being edited by {}.\r\n", d->character->name );
            return;
         }
   }
   if( !can_mmodify( ch, victim ) )
      return;

   d = ch->desc;
   d->olc = new olc_data;
   d->olc->number = victim->pIndexData->vnum;
   /*
    * medit_setup( d, d->olc->number );
    */
   d->character->pcdata->dest_buf = victim;
   d->connected = CON_MEDIT;
   medit_disp_menu( d );

   act( AT_ACTION, "$n starts using OLC.", ch, nullptr, nullptr, TO_ROOM );
}

CMDF( do_mcopy )
{
   area_data *pArea;
   std::string arg1;
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
 * Display positions (sitting, standing etc), same for pos and defpos
 */
void medit_disp_positions( descriptor_data * d )
{
   d->write_to_buffer( "50\x1B[;H\x1B[2J" );
   for( int i = 0; i < POS_MAX; ++i )
      d->character->print_fmt( "&g{:2}&w) {}\r\n", i, capitalize( npc_position[i] ) );
   d->character->print( "Enter position number : " );
}

/*
 * Display mobile sexes, this is hard coded cause it just works that way :)
 */
void medit_disp_sex( descriptor_data * d )
{
   d->write_to_buffer( "50\x1B[;H\x1B[2J" );
   for( int i = 0; i < SEX_MAX; ++i )
      d->character->print_fmt( "&g{:2}&w) {}\r\n", i, capitalize( npc_sex[i] ) );
   d->character->print( "\r\nEnter gender number : " );
}

void spec_menu( void )
{
   int j = 0;

   specmenu[0] = "None";

   for( auto& spec : speclist )
   {
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
      switch ( d->olc->mode )
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
      switch ( d->olc->mode )
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

   if( d->olc->mode == OEDIT_AFFECT_MODIFIER )
   {
      char buf[MSL];

      buf[0] = '\0';

      for( i = 0; i < 32; ++i )
         if( IS_SET( d->character->tempnum, 1 << i ) )
         {
            strlcat( buf, " ", MSL );
            strlcat( buf, aff_flags[i], MSL );
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
   ch->print_fmt( "&w-- Mob Number:  [&c{}&w]\r\n", mob->pIndexData->vnum );
   ch->print_fmt( "&g1&w) Sex: &O{}          &g2&w) Name: &O{}\r\n", npc_sex[mob->sex], mob->name );
   ch->print_fmt( "&g3&w) Shortdesc: &O{}\r\n", mob->short_descr[0] == '\0' ? "(none set)" : mob->short_descr );
   ch->print_fmt( "&g4&w) Longdesc:\r\n&O{}\r\n", mob->long_descr.empty() ? "(none set)" : mob->long_descr );
   ch->print_fmt( "&g5&w) Description:\r\n&O{:<74.74}\r\n\r\n", !mob->chardesc.empty() ? mob->chardesc : "(none set)" );
   ch->print_fmt( "&g6&w) Class: [&c{:<11.11}&w], &g7&w) Race:   [&c{:<11.11}&w]\r\n", npc_class[mob->Class], npc_race[mob->race] );
   ch->print_fmt( "&g8&w) Level:       [&c{:5}&w], &g9&w) Alignment:    [&c{:5}&w]\r\n\r\n", mob->level, mob->alignment );

   ch->print_fmt( " &w) Calc Thac0:      [&c{:5}&w]\r\n", calc_thac0( mob, nullptr, 0 ) );
   ch->print_fmt( "&gA&w) Real Thac0:  [&c{:5}&w]\r\n\r\n", mob->mobthac0 );

   ch->print_fmt( " &w) Calc Experience: [&c{:10}&w]\r\n", mob->exp );
   ch->print_fmt( "&gB&w) Real Experience: [&c{:10}&w]\r\n\r\n", mob->pIndexData->exp );
   ch->print_fmt( "&gC&w) DamNumDice:  [&c{:5}&w], &gD&w) DamSizeDice:  [&c{:5}&w], &gE&w) DamPlus:  [&c{:5}&w]\r\n",
               mob->pIndexData->damnodice, mob->pIndexData->damsizedice, mob->pIndexData->damplus );
   ch->print_fmt( "&gF&w) HitDice:  [&c{}d{}+{}&w]\r\n", mob->pIndexData->hitnodice, mob->pIndexData->hitsizedice, mob->pIndexData->hitplus );
   ch->print_fmt( "&gG&w) Gold:     [&c{:8}&w], &gH&w) Spec: &O{:<22.22}\r\n", mob->gold, !mob->spec_funname.empty(  )? mob->spec_funname : "None" );
   ch->print_fmt( "&gI&w) Resistant   : &O{}\r\n", bitset_string( mob->get_resists(  ), ris_flags ) );
   ch->print_fmt( "&gJ&w) Immune      : &O{}\r\n", bitset_string( mob->get_immunes(  ), ris_flags ) );
   ch->print_fmt( "&gK&w) Susceptible : &O{}\r\n", bitset_string( mob->get_susceps(  ), ris_flags ) );
   ch->print_fmt( "&gL&w) Absorb      : &O{}\r\n", bitset_string( mob->get_absorbs(  ), ris_flags ) );
   ch->print_fmt( "&gM&w) Position    : &O{}\r\n", npc_position[mob->position] );
   ch->print_fmt( "&gN&w) Default Pos : &O{}\r\n", npc_position[mob->defposition] );
   ch->print_fmt( "&gO&w) Attacks     : &c{}\r\n", bitset_string( mob->get_attacks(  ), attack_flags ) );
   ch->print_fmt( "&gP&w) Defenses    : &c{}\r\n", bitset_string( mob->get_defenses(  ), defense_flags ) );
   ch->print_fmt( "&gR&w) Body Parts  : &c{}\r\n", bitset_string( mob->get_bparts(  ), part_flags ) );
   ch->print_fmt( "&gS&w) Act Flags   : &c{}\r\n", bitset_string( mob->get_actflags(  ), act_flags ) );
   ch->print_fmt( "&gT&w) Affected    : &c{}\r\n", bitset_string( mob->get_aflags(  ), aff_flags ) );
   ch->print( "&gQ&w) Quit\r\n" );
   ch->print( "Enter choice : " );

   d->olc->mode = MEDIT_NPC_MAIN_MENU;
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
            bug( "{}: sub_mob_desc: nullptr ch->pcdata->dest_buf", __func__ );
            cleanup_olc( ch->desc );
            ch->substate = SUB_NONE;
            return;
         }
         victim->chardesc = ch->copy_buffer( );
         if( victim->has_actflag( ACT_PROTOTYPE ) )
         {
            STRFREE( victim->pIndexData->chardesc );
            victim->pIndexData->chardesc = STRALLOC( victim->chardesc.c_str() );
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

void medit_parse( descriptor_data * d, std::string & arg )
{
   char_data *victim = ( char_data * ) d->character->pcdata->dest_buf;
   int number = 0;
   std::string arg1;

   switch ( d->olc->mode )
   {
      case MEDIT_NPC_MAIN_MENU:
         switch ( to_upper( arg.front() ) )
         {
            case 'Q':
               cleanup_olc( d );
               return;
            case '1':
               d->olc->mode = MEDIT_SEX;
               medit_disp_sex( d );
               return;
            case '2':
               d->olc->mode = MEDIT_NAME;
               d->character->print( "\r\nEnter name: " );
               return;
            case '3':
               d->olc->mode = MEDIT_S_DESC;
               d->character->print( "\r\nEnter short description: " );
               return;
            case '4':
               d->olc->mode = MEDIT_L_DESC;
               d->character->print( "\r\nEnter long description: " );
               return;
            case '5':
               d->olc->mode = MEDIT_D_DESC;
               d->character->substate = SUB_MOB_DESC;
               d->character->last_cmd = do_medit_reset;

               d->character->print( "Enter new mob description:\r\n" );
               d->character->editor_desc_printf( "Mob description for vnum %d", victim->pIndexData->vnum );
               d->character->start_editing( victim->chardesc );
               return;
            case '6':
               d->olc->mode = MEDIT_CLASS;
               medit_disp_classes( d );
               return;
            case '7':
               d->olc->mode = MEDIT_RACE;
               medit_disp_races( d );
               return;
            case '8':
               d->olc->mode = MEDIT_LEVEL;
               d->character->print( "\r\nEnter level: " );
               return;
            case '9':
               d->olc->mode = MEDIT_ALIGNMENT;
               d->character->print( "\r\nEnter alignment: " );
               return;
            case 'A':
               d->olc->mode = MEDIT_THACO;
               d->character->print( "\r\nUse 21 to have the mud autocalculate Thac0\r\nEnter Thac0: " );
               return;
            case 'B':
               d->olc->mode = MEDIT_EXP;
               d->character->print( "\r\nUse -1 to have the mud autocalculate Exp\r\nEnter Exp: " );
               return;
            case 'C':
               d->olc->mode = MEDIT_DAMNUMDIE;
               d->character->print( "\r\nEnter number of damage dice: " );
               return;
            case 'D':
               d->olc->mode = MEDIT_DAMSIZEDIE;
               d->character->print( "\r\nEnter size of damage dice: " );
               return;
            case 'E':
               d->olc->mode = MEDIT_DAMPLUS;
               d->character->print( "\r\nEnter amount to add to damage: " );
               return;
            case 'F':
               d->olc->mode = MEDIT_HITPLUS;
               d->character->print( "\r\nEnter amount to add to hitpoints: " );
               return;
            case 'G':
               d->olc->mode = MEDIT_GOLD;
               d->character->print( "\r\nEnter amount of gold mobile carries: " );
               return;
            case 'H':
               d->olc->mode = MEDIT_SPEC;
               medit_disp_spec( d );
               return;
            case 'I':
               d->olc->mode = MEDIT_RESISTANT;
               medit_disp_ris( d );
               return;
            case 'J':
               d->olc->mode = MEDIT_IMMUNE;
               medit_disp_ris( d );
               return;
            case 'K':
               d->olc->mode = MEDIT_SUSCEPTIBLE;
               medit_disp_ris( d );
               return;
            case 'L':
               d->olc->mode = MEDIT_ABSORB;
               medit_disp_ris( d );
               return;
            case 'M':
               d->olc->mode = MEDIT_POS;
               medit_disp_positions( d );
               return;
            case 'N':
               d->olc->mode = MEDIT_DEFPOS;
               medit_disp_positions( d );
               return;
            case 'O':
               d->olc->mode = MEDIT_ATTACK;
               medit_disp_attack_menu( d );
               return;
            case 'P':
               d->olc->mode = MEDIT_DEFENSE;
               medit_disp_defense_menu( d );
               return;
            case 'R':
               d->olc->mode = MEDIT_PARTS;
               medit_disp_parts( d );
               return;
            case 'S':
               d->olc->mode = MEDIT_NPC_FLAGS;
               medit_disp_mob_flags( d );
               return;
            case 'T':
               d->olc->mode = MEDIT_AFF_FLAGS;
               medit_disp_aff_flags( d );
               return;
            default:
               medit_disp_menu( d );
               return;
         }
         break;

      case MEDIT_NAME:
         victim->name = arg;
         if( victim->has_actflag( ACT_PROTOTYPE ) )
         {
            STRFREE( victim->pIndexData->player_name );
            victim->pIndexData->player_name = STRALLOC( victim->name.c_str() );
         }
         olc_log( d, "Changed name to %s", arg.c_str(  ) );
         break;

      case MEDIT_S_DESC:
         victim->short_descr = arg;
         if( victim->has_actflag( ACT_PROTOTYPE ) )
         {
            STRFREE( victim->pIndexData->short_descr );
            victim->pIndexData->short_descr = STRALLOC( victim->short_descr.c_str() );
         }
         olc_log( d, "Changed short desc to %s", arg.c_str(  ) );
         break;

      case MEDIT_L_DESC:
         victim->long_descr = std::format( "{}\r\n", arg );
         if( victim->has_actflag( ACT_PROTOTYPE ) )
         {
            victim->pIndexData->long_descr = STRALLOC( victim->long_descr.c_str() );
         }
         olc_log( d, "Changed long desc to %s", arg.c_str(  ) );
         break;

      case MEDIT_D_DESC:
         /*
          * . We should never get here .
          */
         cleanup_olc( d );
         bug( "{}: OLC: medit_parse(): Reached D_DESC case!", __func__ );
         break;

      case MEDIT_NPC_FLAGS:
         /*
          * REDONE, again, then again 
          */
         if( is_number( arg ) )
            if( std::stoi( arg ) == 0 )
               break;

         while( !arg.empty(  ) )
         {
            arg = one_argument( arg, arg1 );

            if( is_number( arg1 ) )
            {
               number = std::stoi( arg1 );
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
            number = std::stoi( arg );
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
         victim->max_hit = urange( 1, std::stoi( arg ), 32700 );
         olc_log( d, "Changed hitpoints to %d", victim->max_hit );
         break;

      case MEDIT_MANA:
         victim->max_mana = urange( 1, std::stoi( arg ), 30000 );
         if( victim->has_actflag( ACT_PROTOTYPE ) )
            victim->pIndexData->max_mana = victim->max_mana;
         olc_log( d, "Changed mana to %d", victim->max_mana );
         break;

      case MEDIT_MOVE:
         victim->max_move = urange( 1, std::stoi( arg ), 30000 );
         if( victim->has_actflag( ACT_PROTOTYPE ) )
            victim->pIndexData->max_move = victim->max_move;
         olc_log( d, "Changed moves to %d", victim->max_move );
         break;

      case MEDIT_SEX:
         victim->sex = urange( 0, std::stoi( arg ), SEX_MAX - 1 );
         if( victim->has_actflag( ACT_PROTOTYPE ) )
            victim->pIndexData->sex = victim->sex;
         olc_log( d, "Changed sex to %s", victim->sex == SEX_MALE ? "Male" : victim->sex == SEX_FEMALE ? "Female" : victim->sex == SEX_NEUTRAL ? "Neutral" : "Hermaphrodite" );
         break;

      case MEDIT_HITROLL:
         victim->hitroll = urange( 0, std::stoi( arg ), 85 );
         if( victim->has_actflag( ACT_PROTOTYPE ) )
            victim->pIndexData->hitroll = victim->hitroll;
         olc_log( d, "Changed hitroll to %d", victim->hitroll );
         break;

      case MEDIT_DAMROLL:
         victim->damroll = urange( 0, std::stoi( arg ), 65 );
         if( victim->has_actflag( ACT_PROTOTYPE ) )
            victim->pIndexData->damroll = victim->damroll;
         olc_log( d, "Changed damroll to %d", victim->damroll );
         break;

      case MEDIT_DAMNUMDIE:
         if( victim->has_actflag( ACT_PROTOTYPE ) )
            victim->pIndexData->damnodice = urange( 0, std::stoi( arg ), 100 );
         olc_log( d, "Changed damnumdie to %d", victim->pIndexData->damnodice );
         break;

      case MEDIT_DAMSIZEDIE:
         if( victim->has_actflag( ACT_PROTOTYPE ) )
            victim->pIndexData->damsizedice = urange( 0, std::stoi( arg ), 100 );
         olc_log( d, "Changed damsizedie to %d", victim->pIndexData->damsizedice );
         break;

      case MEDIT_DAMPLUS:
         if( victim->has_actflag( ACT_PROTOTYPE ) )
            victim->pIndexData->damplus = urange( 0, std::stoi( arg ), 1000 );
         olc_log( d, "Changed damplus to %d", victim->pIndexData->damplus );
         break;

      case MEDIT_HITPLUS:
         if( victim->has_actflag( ACT_PROTOTYPE ) )
            victim->pIndexData->hitplus = urange( 0, std::stoi( arg ), 32767 );
         olc_log( d, "Changed hitplus to %d", victim->pIndexData->hitplus );
         break;

      case MEDIT_AC:
         victim->armor = urange( -300, std::stoi( arg ), 300 );
         if( victim->has_actflag( ACT_PROTOTYPE ) )
            victim->pIndexData->ac = victim->armor;
         olc_log( d, "Changed armor to %d", victim->armor );
         break;

      case MEDIT_GOLD:
         victim->gold = urange( -1, std::stoi( arg ), 2000000000 );
         if( victim->has_actflag( ACT_PROTOTYPE ) )
            victim->pIndexData->gold = victim->gold;
         olc_log( d, "Changed gold to %d", victim->gold );
         break;

      case MEDIT_POS:
         victim->position = urange( 0, std::stoi( arg ), POS_STANDING );
         if( victim->has_actflag( ACT_PROTOTYPE ) )
            victim->pIndexData->position = victim->position;
         olc_log( d, "Changed position to %d", victim->position );
         break;

      case MEDIT_DEFPOS:
         victim->defposition = urange( 0, std::stoi( arg ), POS_STANDING );
         if( victim->has_actflag( ACT_PROTOTYPE ) )
            victim->pIndexData->defposition = victim->defposition;
         olc_log( d, "Changed default position to %d", victim->defposition );
         break;

      case MEDIT_MENTALSTATE:
         victim->mental_state = urange( -100, std::stoi( arg ), 100 );
         olc_log( d, "Changed mental state to %d", victim->mental_state );
         break;

      case MEDIT_CLASS:
         number = std::stoi( arg );
         if( victim->isnpc(  ) )
         {
            victim->Class = urange( 0, number, MAX_NPC_CLASS - 1 );
            if( victim->has_actflag( ACT_PROTOTYPE ) )
               victim->pIndexData->Class = victim->Class;
            break;
         }
         victim->Class = urange( 0, number, MAX_CLASS );
         olc_log( d, "Changed Class to %s", npc_class[victim->Class] );
         break;

      case MEDIT_RACE:
         number = std::stoi( arg );
         if( victim->isnpc(  ) )
         {
            victim->race = urange( 0, number, MAX_NPC_RACE - 1 );
            if( victim->has_actflag( ACT_PROTOTYPE ) )
               victim->pIndexData->race = victim->race;
            break;
         }
         victim->race = urange( 0, number, MAX_RACE - 1 );
         olc_log( d, "Changed race to %s", npc_race[victim->race] );
         break;

      case MEDIT_PARTS:
         number = std::stoi( arg );
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
            number = std::stoi( arg );
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
            number = std::stoi( arg );
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
         victim->level = urange( 1, std::stoi( arg ), LEVEL_IMMORTAL + 5 );
         if( victim->has_actflag( ACT_PROTOTYPE ) )
            victim->pIndexData->level = victim->level;
         olc_log( d, "Changed level to %d", victim->level );
         break;

      case MEDIT_ALIGNMENT:
         victim->alignment = urange( -1000, std::stoi( arg ), 1000 );
         if( victim->has_actflag( ACT_PROTOTYPE ) )
            victim->pIndexData->alignment = victim->alignment;
         olc_log( d, "Changed alignment to %d", victim->alignment );
         break;

      case MEDIT_EXP:
         victim->exp = std::stoi( arg );
         if( victim->has_actflag( ACT_PROTOTYPE ) )
            victim->pIndexData->exp = victim->exp;
         if( victim->exp == -1 )
            victim->exp = mob_xp( victim );
         olc_log( d, "Changed exp to %d", victim->exp );
         break;

      case MEDIT_THACO:
         victim->mobthac0 = std::stoi( arg );
         if( victim->has_actflag( ACT_PROTOTYPE ) )
            victim->pIndexData->mobthac0 = victim->mobthac0;
         olc_log( d, "Changed thac0 to %d", victim->mobthac0 );
         break;

      case MEDIT_RESISTANT:
         if( is_number( arg ) )
         {
            number = std::stoi( arg );
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
            number = std::stoi( arg );
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
            number = std::stoi( arg );
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
            number = std::stoi( arg );
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
         number = std::stoi( arg );
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
         bug( "{}: OLC: medit_parse(): Reached default case!", __func__ );
         cleanup_olc( d );
         return;
   }

/*. END OF CASE 
   If we get here, we have probably changed something, and now want to
   return to main menu.  Use d->olc->changed as a 'has changed' flag .*/

   d->olc->changed = true;
   medit_disp_menu( d );
}

/*. End of medit_parse() .*/
