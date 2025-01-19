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
 *                      Room editing module (redit.c) v1.0                *
 *                                                                        *
\**************************************************************************/

#include <cstdarg>
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
void redit_parse( descriptor_data *, string & );
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
         room = get_room_index( atoi( argument.c_str(  ) ) );
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
    * Make sure the room isnt already being edited 
    */
   list < descriptor_data * >::iterator ds;
   descriptor_data *d;
   for( ds = dlist.begin(  ); ds != dlist.end(  ); ++ds )
   {
      d = *ds;

      if( d->connected == CON_REDIT )
         if( d->olc && OLC_VNUM( d ) == room->vnum )
         {
            ch->printf( "That room is currently being edited by %s.\r\n", d->character->name );
            return;
         }
   }
   if( !can_rmodify( ch, room ) )
      return;

   d = ch->desc;
   d->olc = new olc_data;
   OLC_VNUM( d ) = room->vnum;
   OLC_CHANGE( d ) = false;
   d->character->pcdata->dest_buf = room;
   d->connected = CON_REDIT;
   redit_disp_menu( d );

   act( AT_ACTION, "$n starts using OLC.", ch, nullptr, nullptr, TO_ROOM );
}

CMDF( do_rcopy )
{
   string arg1;

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

   int rvnum = atoi( arg1.c_str(  ) );
   int cvnum = atoi( argument.c_str(  ) );

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
   copy->name = QUICKLINK( orig->name );
   copy->roomdesc = strdup( orig->roomdesc );
   copy->nitedesc = strdup( orig->nitedesc );
   copy->flags = orig->flags;
   copy->sector_type = orig->sector_type;
   copy->baselight = orig->baselight;
   copy->light = orig->light;
   copy->tele_vnum = orig->tele_vnum;
   copy->tele_delay = orig->tele_delay;
   copy->tunnel = orig->tunnel;

   list < extra_descr_data * >::iterator ced;
   for( ced = orig->extradesc.begin(  ); ced != orig->extradesc.end(  ); ++ced )
   {
      extra_descr_data *edesc = *ced;

      extra_descr_data *ed = new extra_descr_data;
      ed->keyword = edesc->keyword;
      if( !edesc->desc.empty(  ) )
         ed->desc = edesc->desc;
      copy->extradesc.push_back( ed );
      ++top_ed;
   }

   list < exit_data * >::iterator cxit;
   for( cxit = orig->exits.begin(  ); cxit != orig->exits.end(  ); ++cxit )
   {
      exit_data *pexit = *cxit;

      exit_data *xit = copy->make_exit( get_room_index( pexit->rvnum ), pexit->vdir );
      xit->keyword = QUICKLINK( pexit->keyword );
      if( pexit->exitdesc && pexit->exitdesc[0] != '\0' )
         xit->exitdesc = QUICKLINK( pexit->exitdesc );
      xit->key = pexit->key;
      xit->flags = pexit->flags;
   }

   room_index_table.insert( map < int, room_index * >::value_type( cvnum, copy ) );
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
void olc_log( descriptor_data * d, const char *format, ... ) __attribute__ ( ( format( printf, 2, 3 ) ) );

void olc_log( descriptor_data * d, const char *format, ... )
{
   char logline[MSL];
   va_list args;

   if( !d )
   {
      bug( "%s: called with null descriptor", __func__ );
      return;
   }

   room_index *room = ( room_index * ) d->character->pcdata->dest_buf;
   obj_data *obj = ( obj_data * ) d->character->pcdata->dest_buf;
   char_data *victim = ( char_data * ) d->character->pcdata->dest_buf;

   va_start( args, format );
   vsnprintf( logline, MSL, format, args );
   va_end( args );

   if( d->connected == CON_REDIT )
      log_printf_plus( LOG_BUILD, sysdata->build_level, "OLCLog: %s ROOM(%d): ", d->character->name, room->vnum );

   else if( d->connected == CON_OEDIT )
      log_printf_plus( LOG_BUILD, sysdata->build_level, "OLCLog: %s OBJ(%d): ", d->character->name, obj->pIndexData->vnum );

   else if( d->connected == CON_MEDIT )
   {
      if( victim->isnpc(  ) )
         log_printf_plus( LOG_BUILD, sysdata->build_level, "OLCLog: %s MOB(%d): ", d->character->name, victim->pIndexData->vnum );
      else
         log_printf_plus( LOG_BUILD, sysdata->build_level, "OLCLog: %s PLR(%s): ", d->character->name, victim->name );
   }
   else
      bug( "%s: called with a bad connected state", __func__ );
}

/**************************************************************************
  Menu functions 
 **************************************************************************/

/*
 * Nice fancy redone Extra Description stuff :)
 */
void redit_disp_extradesc_prompt_menu( descriptor_data * d )
{
   list < extra_descr_data * >::iterator ed;
   room_index *room = ( room_index * ) d->character->pcdata->dest_buf;
   int counter = 0;

   for( ed = room->extradesc.begin(  ); ed != room->extradesc.end(  ); ++ed )
   {
      extra_descr_data *edesc = *ed;

      d->character->printf( "&g%2d&w) %-40.40s\r\n", ++counter, edesc->keyword.c_str(  ) );
   }
   d->character->print( "\r\nWhich extra description do you want to edit? " );
}

void redit_disp_extradesc_menu( descriptor_data * d )
{
   room_index *room = ( room_index * ) d->character->pcdata->dest_buf;
   int count = 0;

   d->write_to_buffer( "50\x1B[;H\x1B[2J" );
   if( !room->extradesc.empty(  ) )
   {
      list < extra_descr_data * >::iterator ed;

      for( ed = room->extradesc.begin(  ); ed != room->extradesc.end(  ); ++ed )
      {
         extra_descr_data *edesc = *ed;

         d->character->printf( "&g%2d&w) Keyword: &O%s\r\n", ++count, edesc->keyword.c_str(  ) );
      }
      d->character->print( "\r\n" );
   }

   d->character->print( "&gA&w) Add a new description\r\n" );
   d->character->print( "&gR&w) Remove a description\r\n" );
   d->character->print( "&gQ&w) Quit\r\n" );
   d->character->print( "\r\nEnter choice: " );

   OLC_MODE( d ) = REDIT_EXTRADESC_MENU;
}

/* For exits */
void redit_disp_exit_menu( descriptor_data * d )
{
   room_index *room = ( room_index * ) d->character->pcdata->dest_buf;
   list < exit_data * >::iterator iexit;
   int cnt = 0;

   OLC_MODE( d ) = REDIT_EXIT_MENU;
   d->write_to_buffer( "50\x1B[;H\x1B[2J" );
   for( iexit = room->exits.begin(  ); iexit != room->exits.end(  ); ++iexit )
   {
      exit_data *pexit = *iexit;

      d->character->printf( "&g%2d&w) %-10.10s to %-5d. Key: %d Keywords: %s Flags: %s\r\n",
                            ++cnt, dir_name[pexit->vdir], pexit->to_room ? pexit->to_room->vnum : 0,
                            pexit->key, ( pexit->keyword && pexit->keyword[0] != '\0' ) ? pexit->keyword : "(none)", bitset_string( pexit->flags, ex_flags ) );
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
   char flags[MSL];
   exit_data *pexit = ( exit_data * ) d->character->pcdata->spare_ptr;

   flags[0] = '\0';
   for( int i = 0; i < MAX_EXFLAG; ++i )
   {
      if( IS_EXIT_FLAG( pexit, i ) )
      {
         strlcat( flags, ex_flags[i], MSL );
         strlcat( flags, " ", MSL );
      }
   }

   OLC_MODE( d ) = REDIT_EXIT_EDIT;
   d->write_to_buffer( "50\x1B[;H\x1B[2J" );
   d->character->printf( "&g1&w) Direction  : &c%s\r\n", dir_name[pexit->vdir] );
   d->character->printf( "&g2&w) To Vnum    : &c%d\r\n", pexit->to_room ? pexit->to_room->vnum : -1 );
   d->character->printf( "&g3&w) Key        : &c%d\r\n", pexit->key );
   d->character->printf( "&g4&w) Keyword    : &c%s\r\n", ( pexit->keyword && pexit->keyword[0] != '\0' ) ? pexit->keyword : "(none)" );
   d->character->printf( "&g5&w) Flags      : &c%s\r\n", flags[0] != '\0' ? flags : "(none)" );
   d->character->printf( "&g6&w) Description: &c%s\r\n", ( pexit->exitdesc && pexit->exitdesc[0] != '\0' ) ? pexit->exitdesc : "(none)" );
   d->character->print( "&gQ&w) Quit\r\n" );
   d->character->print( "\r\nEnter choice: " );
}

void redit_disp_exit_dirs( descriptor_data * d )
{
   d->write_to_buffer( "50\x1B[;H\x1B[2J" );
   for( int i = 0; i <= DIR_SOMEWHERE; ++i )
      d->character->printf( "&g%2d&w) %s\r\n", i, dir_name[i] );

   d->character->print( "\r\nChoose a direction: " );
}

/* For exit flags */
void redit_disp_exit_flag_menu( descriptor_data * d )
{
   exit_data *pexit = ( exit_data * ) d->character->pcdata->spare_ptr;
   char buf1[MSL];
   int i;

   d->write_to_buffer( "50\x1B[;H\x1B[2J" );
   for( i = 0; i < MAX_EXFLAG; ++i )
   {
      if( i == EX_PORTAL )
         continue;
      d->character->printf( "&g%2d&w) %-20.20s\r\n", i + 1, ex_flags[i] );
   }
   buf1[0] = '\0';
   for( i = 0; i < MAX_EXFLAG; ++i )
      if( IS_EXIT_FLAG( pexit, i ) )
      {
         strlcat( buf1, ex_flags[i], MSL );
         strlcat( buf1, " ", MSL );
      }

   d->character->printf( "\r\nExit flags: &c%s&w\r\nEnter room flags, 0 to quit: ", buf1 );
   OLC_MODE( d ) = REDIT_EXIT_FLAGS;
}

/* For room flags */
void redit_disp_flag_menu( descriptor_data * d )
{
   room_index *room = ( room_index * ) d->character->pcdata->dest_buf;
   int columns = 0;

   d->write_to_buffer( "50\x1B[;H\x1B[2J" );
   for( int counter = 0; counter < ROOM_MAX; ++counter )
   {
      d->character->printf( "&g%2d&w) %-20.20s ", counter + 1, r_flags[counter] );

      if( !( ++columns % 2 ) )
         d->character->print( "\r\n" );
   }
   d->character->printf( "\r\nRoom flags: &c%s&w\r\nEnter room flags, 0 to quit : ", bitset_string( room->flags, r_flags ) );
   OLC_MODE( d ) = REDIT_FLAGS;
}

/* for sector type */
void redit_disp_sector_menu( descriptor_data * d )
{
   int columns = 0;

   d->write_to_buffer( "50\x1B[;H\x1B[2J" );
   for( int counter = 0; counter < SECT_MAX; ++counter )
   {
      d->character->printf( "&g%2d&w) %-20.20s ", counter, sect_types[counter] );

      if( !( ++columns % 2 ) )
         d->character->print( "\r\n" );
   }
   d->character->print( "\r\nEnter sector type : " );
   OLC_MODE( d ) = REDIT_SECTOR;
}

/* the main menu */
void redit_disp_menu( descriptor_data * d )
{
   room_index *room = ( room_index * ) d->character->pcdata->dest_buf;

   d->write_to_buffer( "50\x1B[;H\x1B[2J" );
   d->character->printf( "&w-- Room number : [&c%d&w]      Room area: [&c%-30.30s&w]\r\n"
                         "&g1&w) Room Name   : &O%s\r\n"
                         "&g2&w) Description :\r\n&O%s"
                         "&g3&w) Night Desc  :\r\n&O%s"
                         "&g4&w) Room flags  : &c%s\r\n"
                         "&g5&w) Sector type : &c%s\r\n"
                         "&g6&w) Tunnel      : &c%d\r\n"
                         "&g7&w) TeleDelay   : &c%d\r\n"
                         "&g8&w) TeleVnum    : &c%d\r\n"
                         "&g9&w) Base Light  : &c%d\r\n"
                         "&gA&w) Exit menu\r\n"
                         "&gB&w) Extra descriptions menu\r\n"
                         "&gQ&w) Quit\r\n"
                         "Enter choice : ",
                         OLC_NUM( d ), room->area ? room->area->name : "None????",
                         room->name, room->roomdesc,
                         room->nitedesc ? room->nitedesc : "None Set\r\n", bitset_string( room->flags, r_flags ),
                         sect_types[room->sector_type], room->tunnel, room->tele_delay, room->tele_vnum, room->light );

   OLC_MODE( d ) = REDIT_MAIN_MENU;
}

extra_descr_data *redit_find_extradesc( room_index * room, int number )
{
   int count = 0;
   list < extra_descr_data * >::iterator ed;

   for( ed = room->extradesc.begin(  ); ed != room->extradesc.end(  ); ++ed )
   {
      extra_descr_data *edesc = *ed;

      if( ++count == number )
         return edesc;
   }
   return nullptr;
}

CMDF( do_redit_reset )
{
   room_index *room = ( room_index * ) ch->pcdata->dest_buf;
   extra_descr_data *ed = ( extra_descr_data * ) ch->pcdata->spare_ptr;

   switch ( ch->substate )
   {
      default:
         bug( "%s: Invalid substate!", __func__ );
         return;

      case SUB_ROOM_DESC:
         if( !ch->pcdata->dest_buf )
         {
            /*
             * If theres no dest_buf, theres no object, so stick em back as playing 
             */
            ch->print( "Fatal error, report to Samson.\r\n" );
            bug( "%s: sub_obj_extra: nullptr ch->pcdata->dest_buf", __func__ );
            ch->substate = SUB_NONE;
            ch->desc->connected = CON_PLAYING;
            return;
         }
         DISPOSE( room->roomdesc );
         room->roomdesc = ch->copy_buffer( false );
         ch->stop_editing(  );
         ch->pcdata->dest_buf = room;
         ch->desc->connected = CON_REDIT;
         ch->substate = SUB_NONE;

         olc_log( ch->desc, "Edited room description" );
         redit_disp_menu( ch->desc );
         return;

      case SUB_ROOM_DESC_NITE:
         if( !ch->pcdata->dest_buf )
         {
            /*
             * If theres no dest_buf, theres no object, so stick em back as playing 
             */
            ch->print( "Fatal error, report to Samson.\r\n" );
            bug( "%s: sub_obj_extra: nullptr ch->pcdata->dest_buf", __func__ );
            ch->substate = SUB_NONE;
            ch->desc->connected = CON_PLAYING;
            return;
         }
         DISPOSE( room->nitedesc );
         room->nitedesc = ch->copy_buffer( false );
         ch->stop_editing(  );
         ch->pcdata->dest_buf = room;
         ch->desc->connected = CON_REDIT;
         ch->substate = SUB_NONE;

         olc_log( ch->desc, "Edited room night description" );
         redit_disp_menu( ch->desc );
         return;

      case SUB_ROOM_EXTRA:
         ed->desc = ch->copy_buffer(  );
         ch->stop_editing(  );
         ch->pcdata->dest_buf = room;
         ch->pcdata->spare_ptr = ed;
         ch->substate = SUB_NONE;
         ch->desc->connected = CON_REDIT;
         oedit_disp_extra_choice( ch->desc );
         OLC_MODE( ch->desc ) = REDIT_EXTRADESC_CHOICE;
         olc_log( ch->desc, "Edit description for exdesc %s", ed->keyword.c_str(  ) );
         return;
   }
}

/**************************************************************************
  The main loop
 **************************************************************************/

void redit_parse( descriptor_data * d, string & arg )
{
   room_index *room = ( room_index * ) d->character->pcdata->dest_buf;
   room_index *tmp;
   exit_data *pexit = ( exit_data * ) d->character->pcdata->spare_ptr;
   extra_descr_data *ed = ( extra_descr_data * ) d->character->pcdata->spare_ptr;
   string arg1;
   int number = 0;

   switch ( OLC_MODE( d ) )
   {
      case REDIT_MAIN_MENU:
         switch ( arg[0] )
         {
            case 'q':
            case 'Q':
               cleanup_olc( d );
               return;

            case '1':
               d->character->print( "Enter room name:-\r\n| " );
               OLC_MODE( d ) = REDIT_NAME;
               break;

            case '2':
               OLC_MODE( d ) = REDIT_DESC;
               d->character->substate = SUB_ROOM_DESC;
               d->character->last_cmd = do_redit_reset;

               d->character->print( "Enter room description:-\r\n" );
               if( !room->roomdesc )
                  room->roomdesc = strdup( "" );
               d->character->editor_desc_printf( "Room description for vnum %d", room->vnum );
               d->character->start_editing( room->roomdesc );
               break;

            case '3':
               OLC_MODE( d ) = REDIT_NDESC;
               d->character->substate = SUB_ROOM_DESC_NITE;
               d->character->last_cmd = do_redit_reset;

               d->character->print( "Enter room night description:-\r\n" );
               if( !room->nitedesc )
                  room->nitedesc = strdup( "" );
               d->character->editor_desc_printf( "Night description for vnum %d", room->vnum );
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
               OLC_MODE( d ) = REDIT_TUNNEL;
               break;

            case '7':
               d->character->print( "How long before people are teleported out? " );
               OLC_MODE( d ) = REDIT_TELEDELAY;
               break;

            case '8':
               d->character->print( "Where are they teleported to? " );
               OLC_MODE( d ) = REDIT_TELEVNUM;
               break;

            case '9':
               d->character->print( "What is the base lighting factor? " );
               OLC_MODE( d ) = REDIT_LIGHT;
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
         STRFREE( room->name );
         room->name = STRALLOC( arg.c_str(  ) );
         olc_log( d, "Changed name to %s", room->name );
         break;

      case REDIT_DESC:
         /*
          * we will NEVER get here 
          */
         bug( "%s: Reached REDIT_DESC case", __func__ );
         break;

      case REDIT_NDESC:
         /*
          * we will NEVER get here 
          */
         bug( "%s: Reached REDIT_NDESC case", __func__ );
         break;

      case REDIT_FLAGS:
         if( is_number( arg ) )
         {
            number = atoi( arg.c_str(  ) );
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
               olc_log( d, "%s the room flag %s", room->flags.test( number ) ? "Added" : "Removed", r_flags[number] );
            }
         }
         else
         {
            while( !arg.empty(  ) )
            {
               arg = one_argument( arg, arg1 );
               number = get_rflag( arg1 );
               if( number > 0 )
               {
                  room->flags.flip( number );
                  olc_log( d, "%s the room flag %s", room->flags.test( number ) ? "Added" : "Removed", r_flags[number] );
               }
            }
         }
         redit_disp_flag_menu( d );
         return;

      case REDIT_SECTOR:
         number = atoi( arg.c_str(  ) );
         if( number < 0 || number >= SECT_MAX )
         {
            d->character->print( "Invalid choice!" );
            redit_disp_sector_menu( d );
            return;
         }
         else
            room->sector_type = number;
         olc_log( d, "Changed sector to %s", sect_types[number] );
         break;

      case REDIT_TUNNEL:
         number = atoi( arg.c_str(  ) );
         room->tunnel = URANGE( 0, number, 1000 );
         olc_log( d, "Changed tunnel amount to %d", room->tunnel );
         break;

      case REDIT_TELEDELAY:
         number = atoi( arg.c_str(  ) );
         room->tele_delay = number;
         olc_log( d, "Changed teleportation delay to %d", room->tele_delay );
         break;

      case REDIT_TELEVNUM:
         number = atoi( arg.c_str(  ) );
         room->tele_vnum = URANGE( 1, number, sysdata->maxvnum );
         olc_log( d, "Changed teleportation vnum to %d", room->tele_vnum );
         break;

      case REDIT_LIGHT:
         number = atoi( arg.c_str(  ) );
         room->baselight = URANGE( -32000, number, 32000 );
         olc_log( d, "Changed base lighting factor %d", room->baselight );
         break;

      case REDIT_EXIT_MENU:
         switch ( UPPER( arg[0] ) )
         {
            default:
               if( is_number( arg ) )
               {
                  number = atoi( arg.c_str(  ) );
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
               OLC_MODE( d ) = REDIT_EXIT_ADD;
               redit_disp_exit_dirs( d );
               return;
            case 'R':
               OLC_MODE( d ) = REDIT_EXIT_DELETE;
               d->character->print( "Delete which exit? " );
               return;
            case 'Q':
               d->character->pcdata->spare_ptr = nullptr;
               break;
         }
         break;

      case REDIT_EXIT_EDIT:
         switch ( UPPER( arg[0] ) )
         {
            default:
            case 'Q':
               d->character->pcdata->spare_ptr = nullptr;
               redit_disp_exit_menu( d );
               return;
            case '1':
               OLC_MODE( d ) = REDIT_EXIT_DIR;
               redit_disp_exit_dirs( d );
               return;
            case '2':
               OLC_MODE( d ) = REDIT_EXIT_VNUM;
               d->character->print( "Which room does this exit go to? " );
               return;
            case '3':
               OLC_MODE( d ) = REDIT_EXIT_KEY;
               d->character->print( "What is the vnum of the key to this exit? " );
               return;
            case '4':
               OLC_MODE( d ) = REDIT_EXIT_KEYWORD;
               d->character->print( "What is the keyword to this exit? " );
               return;
            case '5':
               OLC_MODE( d ) = REDIT_EXIT_FLAGS;
               redit_disp_exit_flag_menu( d );
               return;
            case '6':
               OLC_MODE( d ) = REDIT_EXIT_DESC;
               d->character->print( "Description:\r\n] " );
               return;
         }
         redit_disp_exit_edit( d );
         return;

      case REDIT_EXIT_DESC:
         if( !arg.empty(  ) )
            stralloc_printf( &pexit->exitdesc, "%s\r\n", arg.c_str(  ) );

         olc_log( d, "Changed %s description to %s", dir_name[pexit->vdir], !arg.empty(  )? arg.c_str(  ) : "none" );
         redit_disp_exit_edit( d );
         return;

      case REDIT_EXIT_ADD:
         if( is_number( arg ) )
         {
            number = atoi( arg.c_str(  ) );
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
         OLC_MODE( d ) = REDIT_EXIT_ADD_VNUM;
         d->character->print( "Which room does this exit go to? " );
         return;

      case REDIT_EXIT_DIR:
         if( is_number( arg ) )
         {
            exit_data *xit;

            number = atoi( arg.c_str(  ) );
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
         OLC_MODE( d ) = REDIT_EXIT_EDIT;
         redit_disp_exit_edit( d );
         return;

      case REDIT_EXIT_ADD_VNUM:
         number = atoi( arg.c_str(  ) );
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

         olc_log( d, "Added %s exit to %d", dir_name[pexit->vdir], pexit->vnum );

         OLC_MODE( d ) = REDIT_EXIT_EDIT;
         redit_disp_exit_edit( d );
         return;

      case REDIT_EXIT_DELETE:
         if( !is_number( arg ) )
         {
            d->character->print( "Exit must be specified in a number.\r\n" );
            redit_disp_exit_menu( d );
         }
         number = atoi( arg.c_str(  ) );
         pexit = room->get_exit_num( number );
         if( !pexit )
         {
            d->character->print( "That exit does not exist.\r\n" );
            redit_disp_exit_menu( d );
         }
         olc_log( d, "Removed %s exit", dir_name[pexit->vdir] );
         room->extract_exit( pexit );
         redit_disp_exit_menu( d );
         return;

      case REDIT_EXIT_VNUM:
         number = atoi( arg.c_str(  ) );
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

         olc_log( d, "%s exit vnum changed to %d", dir_name[pexit->vdir], pexit->to_room->vnum );
         redit_disp_exit_edit( d );
         return;

      case REDIT_EXIT_KEYWORD:
         STRFREE( pexit->keyword );
         pexit->keyword = STRALLOC( arg.c_str(  ) );
         olc_log( d, "Changed %s keyword to %s", dir_name[pexit->vdir], pexit->keyword );
         redit_disp_exit_edit( d );
         return;

      case REDIT_EXIT_KEY:
         number = atoi( arg.c_str(  ) );
         if( number < -1 || number > sysdata->maxvnum )
            d->character->print( "Invalid vnum, try again: " );
         else
         {
            pexit->key = number;
            redit_disp_exit_edit( d );
         }
         olc_log( d, "%s key vnum is now %d", dir_name[pexit->vdir], pexit->key );
         return;

      case REDIT_EXIT_FLAGS:
         number = atoi( arg.c_str(  ) );
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
         olc_log( d, "%s %s to %s exit", IS_EXIT_FLAG( pexit, number ) ? "Added" : "Removed", ex_flags[number], dir_name[pexit->vdir] );
         redit_disp_exit_flag_menu( d );
         return;

      case REDIT_EXTRADESC_DELETE:
         if( !( ed = redit_find_extradesc( room, atoi( arg.c_str(  ) ) ) ) )
         {
            d->character->print( "Not found, try again: " );
            return;
         }
         olc_log( d, "Deleted exdesc %s", ed->keyword.c_str(  ) );
         room->extradesc.remove( ed );
         deleteptr( ed );
         --top_ed;
         redit_disp_extradesc_menu( d );
         return;

      case REDIT_EXTRADESC_CHOICE:
         switch ( UPPER( arg[0] ) )
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
               OLC_MODE( d ) = REDIT_EXTRADESC_KEY;
               d->character->print( "Keywords, seperated by spaces: " );
               return;

            case '2':
               OLC_MODE( d ) = REDIT_EXTRADESC_DESCRIPTION;
               d->character->substate = SUB_ROOM_EXTRA;
               d->character->last_cmd = do_redit_reset;

               d->character->print( "Enter new extradesc description: \r\n" );
               d->character->editor_desc_printf( "Extra description for room %d", room->vnum );
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
         olc_log( d, "Changed exkey %s to %s", ed->keyword.c_str(  ), arg.c_str(  ) );
         ed->keyword = arg;
         oedit_disp_extra_choice( d );
         OLC_MODE( d ) = REDIT_EXTRADESC_CHOICE;
         return;

      case REDIT_EXTRADESC_MENU:
         switch ( UPPER( arg[0] ) )
         {
            case 'Q':
               break;

            case 'A':
               ed = new extra_descr_data;
               room->extradesc.push_back( ed );
               ++top_ed;
               d->character->pcdata->spare_ptr = ed;
               olc_log( d, "Added new exdesc" );

               oedit_disp_extra_choice( d );
               OLC_MODE( d ) = REDIT_EXTRADESC_CHOICE;
               return;

            case 'R':
               OLC_MODE( d ) = REDIT_EXTRADESC_DELETE;
               d->character->print( "Delete which extra description? " );
               return;

            default:
               if( is_number( arg ) )
               {
                  if( !( ed = redit_find_extradesc( room, atoi( arg.c_str(  ) ) ) ) )
                  {
                     d->character->print( "Not found, try again: " );
                     return;
                  }
                  d->character->pcdata->spare_ptr = ed;
                  oedit_disp_extra_choice( d );
                  OLC_MODE( d ) = REDIT_EXTRADESC_CHOICE;
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
         bug( "%s: Reached default case", __func__ );
         break;
   }
   /*
    * Log the changes, so we can keep track of those sneaky bastards 
    * Don't log on the flags cause it does that above 
    */
   /*
    * if ( OLC_MODE(d) != REDIT_FLAGS )
    * olc_log( d, arg ); 
    */

   /*
    * If we get this far, something has be changed.
    */
   OLC_CHANGE( d ) = true;
   redit_disp_menu( d );
}
