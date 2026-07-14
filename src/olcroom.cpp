/****************************************************************************
 *                   ^     +----- |  / ^     ^ |     | +-\                  *
 *                  / \    |      | /  |\   /| |     | |  \                 *
 *                 /   \   +---   |<   | \ / | |     | |  |                 *
 *                /-----\  |      | \  |  v  | |     | |  /                 *
 *               /       \ |      |  \ |     | +-----+ +-/                  *
 ****************************************************************************
 * AFKMud Copyright 1997-2026 by Roger Libiez (Samson),                     *
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
 *                      Room editing module (redit.c) v1.0                *
 *                                                                        *
\**************************************************************************/

#include "mud.h"
#include "area.h"
#include "descriptor.h"
#include "mobindex.h"
#include "objindex.h"
#include "overland.h"
#include "roomindex.h"

/* externs */
void redit_disp_extradesc_menu( descriptor_data * );
void redit_disp_exit_menu( descriptor_data * );
void redit_disp_exit_flag_menu( descriptor_data * );
void redit_disp_flag_menu( descriptor_data * );
void redit_disp_sector_menu( descriptor_data * );
void redit_disp_menu( descriptor_data * );
void redit_parse( descriptor_data *, std::string & );
void cleanup_olc( descriptor_data * );
bool can_rmodify( char_data *, room_index * );
void oedit_disp_extra_choice( descriptor_data * );

CMDF( do_oredit )
{
   room_index *room;

   if( ch->isnpc(  ) || !ch->desc )
   {
      ch->print( "I don't think so...\r\n" );
      return;
   }

   if( argument.empty(  ) )
      room = ch->in_room;
   else
   {
      if( is_number( argument ) )
         room = get_room_index( std::stoi( argument ) );
      else
      {
         ch->print( "Vnum must be specified in numbers!\r\n" );
         return;
      }
   }

   if( !room )
   {
      ch->print( "That room does not exist!\r\n" );
      return;
   }

   /*
    * Make sure the room isn't already being edited
    */
   std::list<descriptor_data *>::iterator ds;
   descriptor_data *d;
   for( ds = dlist.begin(  ); ds != dlist.end(  ); ++ds )
   {
      d = *ds;

      if( d->connected == CON_REDIT )
         if( d->olc && d->olc->number == room->vnum )
         {
            ch->print_fmt( "That room is currently being edited by {}.\r\n", d->character->name );
            return;
         }
   }
   if( !can_rmodify( ch, room ) )
      return;

   d = ch->desc;
   d->olc = new olc_data;
   d->olc->number = room->vnum;
   d->olc->changed = false;
   d->character->pcdata->dest_buf = room;
   d->connected = CON_REDIT;
   redit_disp_menu( d );

   act( AT_ACTION, "$n starts using OLC.", ch, nullptr, nullptr, TO_ROOM );
}

CMDF( do_rcopy )
{
   std::string arg1;

   argument = one_argument( argument, arg1 );
   if( arg1.empty(  ) || argument.empty(  ) )
   {
      ch->print( "Usage: rcopy <original> <new>\r\n" );
      return;
   }

   if( !is_number( arg1 ) || !is_number( argument ) )
   {
      ch->print( "Values must be numeric.\r\n" );
      return;
   }

   int rvnum = std::stoi( arg1 );
   int cvnum = std::stoi( argument );

   if( ch->get_trust(  ) < LEVEL_GREATER )
   {
      area_data *pArea;

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

   if( get_room_index( cvnum ) )
   {
      ch->print( "Target vnum already exists.\r\n" );
      return;
   }

   room_index *orig;
   if( !( orig = get_room_index( rvnum ) ) )
   {
      ch->print( "Source vnum doesn't exist.\r\n" );
      return;
   }

   room_index *copy = new room_index;
   copy->exits.clear(  );
   copy->extradesc.clear(  );
   copy->area = ( ch->pcdata->area ) ? ch->pcdata->area : orig->area;
   copy->vnum = cvnum;
   copy->name = orig->name;
   copy->roomdesc = orig->roomdesc;
   copy->nitedesc = orig->nitedesc;
   copy->flags = orig->flags;
   copy->sector_type = orig->sector_type;
   copy->baselight = orig->baselight;
   copy->light = orig->light;
   copy->tele_vnum = orig->tele_vnum;
   copy->tele_delay = orig->tele_delay;
   copy->tunnel = orig->tunnel;

   for( auto* edesc : orig->extradesc )
   {
      extra_descr_data *ed = new extra_descr_data;
      ed->keyword = edesc->keyword;
      if( !edesc->desc.empty(  ) )
         ed->desc = edesc->desc;
      copy->extradesc.push_back( ed );
      ++top_ed;
   }

   for( auto* pexit : orig->exits )
   {
      exit_data *xit = copy->make_exit( get_room_index( pexit->rvnum ), pexit->vdir );
      xit->keyword = pexit->keyword;
      if( !pexit->exitdesc.empty() )
         xit->exitdesc = pexit->exitdesc;
      xit->key = pexit->key;
      xit->flags = pexit->flags;
   }

   room_index_table.insert( std::map<int, room_index *>::value_type( cvnum, copy ) );
   copy->area->rooms.push_back( copy );
   ++top_room;
   ch->print( "Room copied.\r\n" );
}

bool is_inolc( descriptor_data * d )
{
   /*
    * safeties, not that its necessary really... 
    */
   if( !d || !d->character )
      return false;

   if( d->character->isnpc(  ) )
      return false;

   /*
    * objs 
    */
   if( d->connected == CON_OEDIT )
      return true;

   /*
    * mobs 
    */
   if( d->connected == CON_MEDIT )
      return true;

   /*
    * rooms 
    */
   if( d->connected == CON_REDIT )
      return true;

   return false;
}

/*
 * Log all changes to catch those sneaky bastards =)
 */
void process_olc_log( char_data * ch, std::string_view message )
{
   if( !ch )
   {
      bug( "{}: called with null ch", __func__ );
      return;
   }

   if( ch->isnpc() )
   {
      bug( "{}: called by NPC!", __func__ );
      return;
   }

   room_index *room = static_cast<room_index *>( ch->pcdata->dest_buf );
   obj_data *obj = static_cast<obj_data *>( ch->pcdata->dest_buf );
   char_data *victim = static_cast<char_data *>( ch->pcdata->dest_buf );

   if( ch->desc->connected == CON_REDIT )
      log_printf_plus( LOG_BUILD, sysdata->build_level, "OLCLog: {} ROOM({}): {}", ch->name, room->vnum, message );

   else if( ch->desc->connected == CON_OEDIT )
      log_printf_plus( LOG_BUILD, sysdata->build_level, "OLCLog: {} OBJ({}): {}", ch->name, obj->pIndexData->vnum, message );

   else if( ch->desc->connected == CON_MEDIT )
   {
      if( victim->isnpc(  ) )
         log_printf_plus( LOG_BUILD, sysdata->build_level, "OLCLog: {} MOB({}): {}", ch->name, victim->pIndexData->vnum, message );
      else
         log_printf_plus( LOG_BUILD, sysdata->build_level, "OLCLog: {} PLR({}): {}", ch->name, victim->name, message );
   }
   else
      bug( "{}: called with a bad connected state", __func__ );
}

/**************************************************************************
  Menu functions 
 **************************************************************************/

/*
 * Nice fancy redone Extra Description stuff :)
 */
void redit_disp_extradesc_prompt_menu( descriptor_data * d )
{
   const room_index *room = static_cast<room_index *>( d->character->pcdata->dest_buf );
   int counter = 0;

   for( auto* edesc : room->extradesc )
      d->character->print_fmt( "&g{:2}&w) {:<40.40}\r\n", ++counter, edesc->keyword );

   d->character->print( "\r\nWhich extra description do you want to edit? " );
}

void redit_disp_extradesc_menu( descriptor_data * d )
{
   room_index *room = static_cast<room_index *>( d->character->pcdata->dest_buf );

   d->write_to_buffer( "50\x1B[;H\x1B[2J" );
   if( !room->extradesc.empty(  ) )
   {
      int count = 0;
      for( auto* edesc : room->extradesc )
         d->character->print_fmt( "&g{}&w) Keyword: &O{}\r\n", ++count, edesc->keyword );

      d->character->print( "\r\n" );
   }

   d->character->print( "&gA&w) Add a new description\r\n" );
   d->character->print( "&gR&w) Remove a description\r\n" );
   d->character->print( "&gQ&w) Quit\r\n" );
   d->character->print( "\r\nEnter choice: " );

   d->olc->mode = REDIT_EXTRADESC_MENU;
}

/* For exits */
void redit_disp_exit_menu( descriptor_data * d )
{
   room_index *room = static_cast<room_index *>( d->character->pcdata->dest_buf );
   int cnt = 0;

   d->olc->mode = REDIT_EXIT_MENU;
   d->write_to_buffer( "50\x1B[;H\x1B[2J" );
   for( auto* pexit : room->exits )
   {
      d->character->print_fmt( "&g{:2}&w) {:<10.10} to {:<5}. Key: {} Keywords: {} Flags: {}\r\n",
                            ++cnt, dir_name[pexit->vdir], pexit->to_room ? pexit->to_room->vnum : 0,
                            pexit->key, !pexit->keyword.empty() ? pexit->keyword : "(none)", bitset_string( pexit->flags, ex_flags ) );
   }

   if( !room->exits.empty(  ) )
      d->character->print( "\r\n" );
   d->character->print( "&gA&w) Add a new exit\r\n" );
   d->character->print( "&gR&w) Remove an exit\r\n" );
   d->character->print( "&gQ&w) Quit\r\n" );

   d->character->print( "\r\nEnter choice: " );
}

void redit_disp_exit_edit( descriptor_data * d )
{
   std::string flags;
   exit_data *pexit = static_cast<exit_data *>( d->character->pcdata->spare_ptr );

   for( int i = 0; i < MAX_EXFLAG; ++i )
   {
      if( IS_EXIT_FLAG( pexit, i ) )
      {
         flags.append( ex_flags[i] );
         flags.append( " " );
      }
   }

   d->olc->mode = REDIT_EXIT_EDIT;
   d->write_to_buffer( "50\x1B[;H\x1B[2J" );
   d->character->print_fmt( "&g1&w) Direction  : &c{}\r\n", dir_name[pexit->vdir] );
   d->character->print_fmt( "&g2&w) To Vnum    : &c{}\r\n", pexit->to_room ? pexit->to_room->vnum : -1 );
   d->character->print_fmt( "&g3&w) Key        : &c{}\r\n", pexit->key );
   d->character->print_fmt( "&g4&w) Keyword    : &c{}\r\n", !pexit->keyword.empty() ? pexit->keyword : "(none)" );
   d->character->print_fmt( "&g5&w) Flags      : &c{}\r\n", !flags.empty() ? flags : "(none)" );
   d->character->print_fmt( "&g6&w) Description: &c{}\r\n", !pexit->exitdesc.empty() ? pexit->exitdesc : "(none)" );
   d->character->print( "&gQ&w) Quit\r\n" );
   d->character->print( "\r\nEnter choice: " );
}

void redit_disp_exit_dirs( descriptor_data * d )
{
   d->write_to_buffer( "50\x1B[;H\x1B[2J" );
   for( int i = 0; i <= DIR_SOMEWHERE; ++i )
      d->character->print_fmt( "&g{:2}&w) {}\r\n", i, dir_name[i] );

   d->character->print( "\r\nChoose a direction: " );
}

/* For exit flags */
void redit_disp_exit_flag_menu( descriptor_data * d )
{
   exit_data *pexit = static_cast<exit_data *>( d->character->pcdata->spare_ptr );
   std::string buf1;
   int i;

   d->write_to_buffer( "50\x1B[;H\x1B[2J" );
   for( i = 0; i < MAX_EXFLAG; ++i )
   {
      if( i == EX_PORTAL )
         continue;
      d->character->print_fmt( "&g{}&w) {:<20.20}\r\n", i + 1, ex_flags[i] );
   }

   for( i = 0; i < MAX_EXFLAG; ++i )
   {
      if( IS_EXIT_FLAG( pexit, i ) )
      {
         buf1.append( ex_flags[i] );
         buf1.append( " " );
      }
   }

   d->character->print_fmt( "\r\nExit flags: &c{}&w\r\nEnter room flags, 0 to quit: ", buf1 );
   d->olc->mode = REDIT_EXIT_FLAGS;
}

/* For room flags */
void redit_disp_flag_menu( descriptor_data * d )
{
   const room_index *room = static_cast<room_index *>( d->character->pcdata->dest_buf );
   int columns = 0;

   d->write_to_buffer( "50\x1B[;H\x1B[2J" );
   for( int counter = 0; counter < ROOM_MAX; ++counter )
   {
      d->character->print_fmt( "&g{}&w) {:<20.20} ", counter + 1, r_flags[counter] );

      if( !( ++columns % 2 ) )
         d->character->print( "\r\n" );
   }
   d->character->print_fmt( "\r\nRoom flags: &c{}&w\r\nEnter room flags, 0 to quit : ", bitset_string( room->flags, r_flags ) );
   d->olc->mode = REDIT_FLAGS;
}

/* for sector type */
void redit_disp_sector_menu( descriptor_data * d )
{
   int columns = 0;

   d->write_to_buffer( "50\x1B[;H\x1B[2J" );
   for( int counter = 0; counter < SECT_MAX; ++counter )
   {
      d->character->print_fmt( "&g{}&w) {:<20.20} ", counter, sect_types[counter] );

      if( !( ++columns % 2 ) )
         d->character->print( "\r\n" );
   }
   d->character->print( "\r\nEnter sector type : " );
   d->olc->mode = REDIT_SECTOR;
}

/* the main menu */
void redit_disp_menu( descriptor_data * d )
{
   room_index *room = static_cast<room_index *>( d->character->pcdata->dest_buf );

   d->write_to_buffer( "50\x1B[;H\x1B[2J" );
   d->character->print_fmt( "&w-- Room number : [&c{}&w]      Room area: [&c{:<30.30}&w]\r\n"
                         "&g1&w) Room Name   : &O{}\r\n"
                         "&g2&w) Description :\r\n&O{}"
                         "&g3&w) Night Desc  :\r\n&O{}"
                         "&g4&w) Room flags  : &c{}\r\n"
                         "&g5&w) Sector type : &c{}\r\n"
                         "&g6&w) Tunnel      : &c{}\r\n"
                         "&g7&w) TeleDelay   : &c{}\r\n"
                         "&g8&w) TeleVnum    : &c{}\r\n"
                         "&g9&w) Base Light  : &c{}\r\n"
                         "&gA&w) Exit menu\r\n"
                         "&gB&w) Extra descriptions menu\r\n"
                         "&gQ&w) Quit\r\n"
                         "Enter choice : ",
                         d->olc->number, room->area ? room->area->name : "None????",
                         room->name, room->roomdesc,
                         !room->nitedesc.empty() ? room->nitedesc : "None Set\r\n", bitset_string( room->flags, r_flags ),
                         sect_types[room->sector_type], room->tunnel, room->tele_delay, room->tele_vnum, room->light );

   d->olc->mode = REDIT_MAIN_MENU;
}

extra_descr_data *redit_find_extradesc( const room_index * room, int number )
{
   int count = 0;

   for( auto* edesc : room->extradesc )
   {
      if( ++count == number )
         return edesc;
   }
   return nullptr;
}

CMDF( do_redit_reset )
{
   room_index *room = static_cast<room_index *>( ch->pcdata->dest_buf );
   extra_descr_data *ed = ( extra_descr_data * ) ch->pcdata->spare_ptr;

   switch ( ch->substate )
   {
      default:
         bug( "{}: Invalid substate!", __func__ );
         return;

      case SUB_ROOM_DESC:
         if( !ch->pcdata->dest_buf )
         {
            /*
             * If there's no dest_buf, there's no object, so stick em back as playing
             */
            ch->print( "Fatal error, report to Samson.\r\n" );
            bug( "{}: sub_obj_extra: nullptr ch->pcdata->dest_buf", __func__ );
            ch->substate = SUB_NONE;
            ch->desc->connected = CON_PLAYING;
            return;
         }
         room->roomdesc = ch->copy_buffer( );
         ch->stop_editing(  );
         ch->pcdata->dest_buf = room;
         ch->desc->connected = CON_REDIT;
         ch->substate = SUB_NONE;

         olc_log( ch, "Edited room description: {}", room->vnum );
         redit_disp_menu( ch->desc );
         return;

      case SUB_ROOM_DESC_NITE:
         if( !ch->pcdata->dest_buf )
         {
            /*
             * If theres no dest_buf, theres no object, so stick em back as playing 
             */
            ch->print( "Fatal error, report to Samson.\r\n" );
            bug( "{}: sub_obj_extra: nullptr ch->pcdata->dest_buf", __func__ );
            ch->substate = SUB_NONE;
            ch->desc->connected = CON_PLAYING;
            return;
         }
         room->nitedesc = ch->copy_buffer( );
         ch->stop_editing(  );
         ch->pcdata->dest_buf = room;
         ch->desc->connected = CON_REDIT;
         ch->substate = SUB_NONE;

         olc_log( ch, "Edited room night description: {}", room->vnum );
         redit_disp_menu( ch->desc );
         return;

      case SUB_ROOM_EXTRA:
         ed->desc = ch->copy_buffer( );
         ch->stop_editing(  );
         ch->pcdata->dest_buf = room;
         ch->pcdata->spare_ptr = ed;
         ch->substate = SUB_NONE;
         ch->desc->connected = CON_REDIT;
         oedit_disp_extra_choice( ch->desc );
         ch->desc->olc->mode = REDIT_EXTRADESC_CHOICE;
         olc_log( ch, "Edit description for exdesc {}", ed->keyword );
         return;
   }
}

/**************************************************************************
  The main loop
 **************************************************************************/

void redit_parse( descriptor_data * d, std::string & arg )
{
   room_index *room = static_cast<room_index *>( d->character->pcdata->dest_buf );
   room_index *tmp;
   exit_data *pexit = static_cast<exit_data *>( d->character->pcdata->spare_ptr );
   extra_descr_data *ed = static_cast<extra_descr_data *>( d->character->pcdata->spare_ptr );
   int number = 0;

   switch ( d->olc->mode )
   {
      case REDIT_MAIN_MENU:
         switch ( arg.front() )
         {
            case 'q':
            case 'Q':
               cleanup_olc( d );
               return;

            case '1':
               d->character->print( "Enter room name:-\r\n| " );
               d->olc->mode = REDIT_NAME;
               break;

            case '2':
               d->olc->mode = REDIT_DESC;
               d->character->substate = SUB_ROOM_DESC;
               d->character->last_cmd = do_redit_reset;

               d->character->print( "Enter room description:-\r\n" );
               d->character->set_editor_desc( std::format( "Room description for vnum {}", room->vnum ) );
               d->character->start_editing( room->roomdesc );
               break;

            case '3':
               d->olc->mode = REDIT_NDESC;
               d->character->substate = SUB_ROOM_DESC_NITE;
               d->character->last_cmd = do_redit_reset;

               d->character->print( "Enter room night description:-\r\n" );
               d->character->set_editor_desc( std::format( "Night description for vnum {}", room->vnum ) );
               d->character->start_editing( room->nitedesc );
               break;

            case '4':
               redit_disp_flag_menu( d );
               break;

            case '5':
               redit_disp_sector_menu( d );
               break;

            case '6':
               d->character->print( "How many people can fit in the room? " );
               d->olc->mode = REDIT_TUNNEL;
               break;

            case '7':
               d->character->print( "How long before people are teleported out? " );
               d->olc->mode = REDIT_TELEDELAY;
               break;

            case '8':
               d->character->print( "Where are they teleported to? " );
               d->olc->mode = REDIT_TELEVNUM;
               break;

            case '9':
               d->character->print( "What is the base lighting factor? " );
               d->olc->mode = REDIT_LIGHT;
               break;

            case 'A':
               redit_disp_exit_menu( d );
               break;

            case 'b':
            case 'B':
               redit_disp_extradesc_menu( d );
               break;

            default:
               d->character->print( "Invalid choice!" );
               redit_disp_menu( d );
               break;
         }
         return;

      case REDIT_NAME:
         room->name = arg;
         olc_log( d->character, "Changed name to {}", room->name );
         break;

      case REDIT_DESC:
         /*
          * we will NEVER get here 
          */
         bug( "{}: Reached REDIT_DESC case", __func__ );
         break;

      case REDIT_NDESC:
         /*
          * we will NEVER get here 
          */
         bug( "{}: Reached REDIT_NDESC case", __func__ );
         break;

      case REDIT_FLAGS:
         if( is_number( arg ) )
         {
            number = std::stoi( arg );
            if( number == 0 )
               break;
            else if( number < 0 || number >= ROOM_MAX )
            {
               d->character->print( "Invalid flag, try again: " );
               return;
            }
            else
            {
               number -= 1;   /* Offset for 0 */
               room->flags.flip( number );
               olc_log( d->character, "{} the room flag {}", room->flags.test( number ) ? "Added" : "Removed", r_flags[number] );
            }
         }
         else
         {
            std::string arg1;

            while( !arg.empty(  ) )
            {
               arg = one_argument( arg, arg1 );
               number = get_rflag( arg1 );
               if( number > 0 )
               {
                  room->flags.flip( number );
                  olc_log( d->character, "{} the room flag {}", room->flags.test( number ) ? "Added" : "Removed", r_flags[number] );
               }
            }
         }
         redit_disp_flag_menu( d );
         return;

      case REDIT_SECTOR:
         number = std::stoi( arg );
         if( number < 0 || number >= SECT_MAX )
         {
            d->character->print( "Invalid choice!" );
            redit_disp_sector_menu( d );
            return;
         }
         else
            room->sector_type = number;
         olc_log( d->character, "Changed sector to {}", sect_types[number] );
         break;

      case REDIT_TUNNEL:
         number = std::stoi( arg );
         room->tunnel = urange( 0, number, 1000 );
         olc_log( d->character, "Changed tunnel amount to {}", room->tunnel );
         break;

      case REDIT_TELEDELAY:
         number = std::stoi( arg );
         room->tele_delay = number;
         olc_log( d->character, "Changed teleportation delay to {}", room->tele_delay );
         break;

      case REDIT_TELEVNUM:
         number = std::stoi( arg );
         room->tele_vnum = urange( 1, number, sysdata->maxvnum );
         olc_log( d->character, "Changed teleportation vnum to {}", room->tele_vnum );
         break;

      case REDIT_LIGHT:
         number = std::stoi( arg );
         room->baselight = urange( -32000, number, 32000 );
         olc_log( d->character, "Changed base lighting factor {}", room->baselight );
         break;

      case REDIT_EXIT_MENU:
         switch ( to_upper( arg.front() ) )
         {
            default:
               if( is_number( arg ) )
               {
                  number = std::stoi( arg );
                  pexit = room->get_exit_num( number );
                  if( pexit )
                  {
                     d->character->pcdata->spare_ptr = pexit;
                     redit_disp_exit_edit( d );
                     return;
                  }
               }
               redit_disp_exit_menu( d );
               return;
            case 'A':
               d->olc->mode = REDIT_EXIT_ADD;
               redit_disp_exit_dirs( d );
               return;
            case 'R':
               d->olc->mode = REDIT_EXIT_DELETE;
               d->character->print( "Delete which exit? " );
               return;
            case 'Q':
               d->character->pcdata->spare_ptr = nullptr;
               break;
         }
         break;

      case REDIT_EXIT_EDIT:
         switch ( to_upper( arg.front() ) )
         {
            default:
            case 'Q':
               d->character->pcdata->spare_ptr = nullptr;
               redit_disp_exit_menu( d );
               return;
            case '1':
               d->olc->mode = REDIT_EXIT_DIR;
               redit_disp_exit_dirs( d );
               return;
            case '2':
               d->olc->mode = REDIT_EXIT_VNUM;
               d->character->print( "Which room does this exit go to? " );
               return;
            case '3':
               d->olc->mode = REDIT_EXIT_KEY;
               d->character->print( "What is the vnum of the key to this exit? " );
               return;
            case '4':
               d->olc->mode = REDIT_EXIT_KEYWORD;
               d->character->print( "What is the keyword to this exit? " );
               return;
            case '5':
               d->olc->mode = REDIT_EXIT_FLAGS;
               redit_disp_exit_flag_menu( d );
               return;
            case '6':
               d->olc->mode = REDIT_EXIT_DESC;
               d->character->print( "Description:\r\n] " );
               return;
         }
         redit_disp_exit_edit( d );
         return;

      case REDIT_EXIT_DESC:
         if( !arg.empty(  ) )
            pexit->exitdesc = std::format( "{}\r\n", arg );

         olc_log( d->character, "Changed {} description to {}", dir_name[pexit->vdir], !arg.empty(  ) ? arg : "none" );
         redit_disp_exit_edit( d );
         return;

      case REDIT_EXIT_ADD:
         if( is_number( arg ) )
         {
            number = std::stoi( arg );
            if( number < DIR_NORTH || number > DIR_SOMEWHERE )
            {
               d->character->print( "Invalid direction, try again: " );
               return;
            }
            pexit = room->get_exit( number );
            if( pexit )
            {
               d->character->print( "An exit in that direction already exists, try again: " );
               return;
            }
            d->character->tempnum = number;
         }
         else
         {
            number = get_dir( arg );
            pexit = room->get_exit( number );
            if( pexit )
            {
               d->character->print( "An exit in that direction already exists, try again: " );
               return;
            }
            d->character->tempnum = number;
         }
         d->olc->mode = REDIT_EXIT_ADD_VNUM;
         d->character->print( "Which room does this exit go to? " );
         return;

      case REDIT_EXIT_DIR:
         if( is_number( arg ) )
         {
            exit_data *xit;

            number = std::stoi( arg );
            if( number < DIR_NORTH || number > DIR_SOMEWHERE )
            {
               d->character->print( "Invalid direction, try again: " );
               return;
            }
            xit = room->get_exit( number );
            if( xit && xit != pexit )
            {
               d->character->print( "An exit in that direction already exists, try again: " );
               return;
            }
            pexit->vdir = number;
         }
         else
         {
            exit_data *xit;

            number = get_dir( arg );
            if( number < DIR_NORTH || number > DIR_SOMEWHERE )
            {
               d->character->print( "Invalid direction, try again: " );
               return;
            }
            xit = room->get_exit( number );
            if( xit && xit != pexit )
            {
               d->character->print( "An exit in that direction already exists, try again: " );
               return;
            }
            pexit->vdir = number;
         }
         d->olc->mode = REDIT_EXIT_EDIT;
         redit_disp_exit_edit( d );
         return;

      case REDIT_EXIT_ADD_VNUM:
         number = std::stoi( arg );
         if( !( tmp = get_room_index( number ) ) )
         {
            d->character->print( "Non-existant room, try again: " );
            return;
         }
         pexit = room->make_exit( tmp, d->character->tempnum );
         pexit->key = -1;
         pexit->flags.reset(  );
         act( AT_IMMORT, "$n reveals a hidden passage!", d->character, nullptr, nullptr, TO_ROOM );
         d->character->pcdata->spare_ptr = pexit;

         olc_log( d->character, "Added {} exit to {}", dir_name[pexit->vdir], pexit->vnum );

         d->olc->mode = REDIT_EXIT_EDIT;
         redit_disp_exit_edit( d );
         return;

      case REDIT_EXIT_DELETE:
         if( !is_number( arg ) )
         {
            d->character->print( "Exit must be specified in a number.\r\n" );
            redit_disp_exit_menu( d );
         }
         number = std::stoi( arg );
         pexit = room->get_exit_num( number );
         if( !pexit )
         {
            d->character->print( "That exit does not exist.\r\n" );
            redit_disp_exit_menu( d );
         }
         olc_log( d->character, "Removed {} exit", dir_name[pexit->vdir] );
         room->extract_exit( pexit );
         redit_disp_exit_menu( d );
         return;

      case REDIT_EXIT_VNUM:
         number = std::stoi( arg );
         if( number < 0 || number > sysdata->maxvnum )
         {
            d->character->print( "Invalid room number, try again : " );
            return;
         }
         if( !get_room_index( number ) )
         {
            d->character->print( "That room does not exist, try again: " );
            return;
         }
         /*
          * pexit->vnum = number;
          */
         pexit->to_room = get_room_index( number );

         olc_log( d->character, "{} exit vnum changed to {}", dir_name[pexit->vdir], pexit->to_room->vnum );
         redit_disp_exit_edit( d );
         return;

      case REDIT_EXIT_KEYWORD:
         pexit->keyword = arg;
         olc_log( d->character, "Changed {} keyword to {}", dir_name[pexit->vdir], pexit->keyword );
         redit_disp_exit_edit( d );
         return;

      case REDIT_EXIT_KEY:
         number = std::stoi( arg );
         if( number < -1 || number > sysdata->maxvnum )
            d->character->print( "Invalid vnum, try again: " );
         else
         {
            pexit->key = number;
            redit_disp_exit_edit( d );
         }
         olc_log( d->character, "{} key vnum is now {}", dir_name[pexit->vdir], pexit->key );
         return;

      case REDIT_EXIT_FLAGS:
         number = std::stoi( arg );
         if( number == 0 )
         {
            redit_disp_exit_edit( d );
            return;
         }

         if( ( number < 0 ) || ( number >= MAX_EXFLAG ) || ( ( number - 1 ) == EX_PORTAL ) )
         {
            d->character->print( "That's not a valid choice!\r\n" );
            redit_disp_exit_flag_menu( d );
         }
         number -= 1;
         pexit->flags.flip( number );
         olc_log( d->character, "{} {} to {} exit", IS_EXIT_FLAG( pexit, number ) ? "Added" : "Removed", ex_flags[number], dir_name[pexit->vdir] );
         redit_disp_exit_flag_menu( d );
         return;

      case REDIT_EXTRADESC_DELETE:
         if( !( ed = redit_find_extradesc( room, std::stoi( arg ) ) ) )
         {
            d->character->print( "Not found, try again: " );
            return;
         }
         olc_log( d->character, "Deleted exdesc {}", ed->keyword );
         room->extradesc.remove( ed );
         deleteptr( ed );
         --top_ed;
         redit_disp_extradesc_menu( d );
         return;

      case REDIT_EXTRADESC_CHOICE:
         switch ( to_upper( arg.front() ) )
         {
            default:
            case 'Q':
               if( ed->keyword.empty(  ) || ed->desc.empty(  ) )
               {
                  d->character->print( "No keyword and/or description, junking..." );
                  room->extradesc.remove( ed );
                  deleteptr( ed );
                  --top_ed;
               }
               d->character->pcdata->spare_ptr = nullptr;
               redit_disp_extradesc_menu( d );
               return;

            case '1':
               d->olc->mode = REDIT_EXTRADESC_KEY;
               d->character->print( "Keywords, seperated by spaces: " );
               return;

            case '2':
               d->olc->mode = REDIT_EXTRADESC_DESCRIPTION;
               d->character->substate = SUB_ROOM_EXTRA;
               d->character->last_cmd = do_redit_reset;

               d->character->print( "Enter new extradesc description: \r\n" );
               d->character->set_editor_desc( std::format( "Extra description for room {}", room->vnum ) );
               d->character->start_editing( ed->desc );
               return;
         }
         break;

      case REDIT_EXTRADESC_KEY:
         /*
          * if ( SetRExtra( room, arg ) )
          * {
          * d->character->print( "An extradesc with that keyword already exists.\r\n" );
          * redit_disp_extradesc_menu(d);
          * return;
          * } 
          */
         olc_log( d->character, "Changed exkey {} to {}", ed->keyword, arg );
         ed->keyword = arg;
         oedit_disp_extra_choice( d );
         d->olc->mode = REDIT_EXTRADESC_CHOICE;
         return;

      case REDIT_EXTRADESC_MENU:
         switch ( to_upper( arg.front() ) )
         {
            case 'Q':
               break;

            case 'A':
               ed = new extra_descr_data;
               room->extradesc.push_back( ed );
               ++top_ed;
               d->character->pcdata->spare_ptr = ed;
               olc_log( d->character, "Added new exdesc" );

               oedit_disp_extra_choice( d );
               d->olc->mode = REDIT_EXTRADESC_CHOICE;
               return;

            case 'R':
               d->olc->mode = REDIT_EXTRADESC_DELETE;
               d->character->print( "Delete which extra description? " );
               return;

            default:
               if( is_number( arg ) )
               {
                  if( !( ed = redit_find_extradesc( room, std::stoi( arg ) ) ) )
                  {
                     d->character->print( "Not found, try again: " );
                     return;
                  }
                  d->character->pcdata->spare_ptr = ed;
                  oedit_disp_extra_choice( d );
                  d->olc->mode = REDIT_EXTRADESC_CHOICE;
               }
               else
                  redit_disp_extradesc_menu( d );
               return;
         }
         break;

      default:
         /*
          * we should never get here 
          */
         bug( "{}: Reached default case", __func__ );
         break;
   }
   /*
    * Log the changes, so we can keep track of those sneaky bastards 
    * Don't log on the flags cause it does that above 
    */
   /*
    * if ( d->olc->mode != REDIT_FLAGS )
    * olc_log( d->character, arg );
    */

   /*
    * If we get this far, something has be changed.
    */
   d->olc->changed = true;
   redit_disp_menu( d );
}
