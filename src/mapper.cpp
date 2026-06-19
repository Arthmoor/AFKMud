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
 *                           Room Mapper Module                             *
 ****************************************************************************/

/**************************************************************************
*  Original Diku Mud copyright (C) 1990, 1991 by Sebastian Hammer,        *
*  Michael Seifert, Hans Henrik St{rfeldt, Tom Madsen, and Katja Nyboe.   *
*                                                                         *
*  Merc Diku Mud improvments copyright (C) 1992, 1993 by Michael          *
*  Chastain, Michael Quan, and Mitchell Tse.                              *
*                                                                         *
*  In order to use any part of this Merc Diku Mud, you must comply with   *
*  both the original Diku license in 'license.doc' as well the Merc       *
*  license in 'license.txt'.  In particular, you may not remove either of *
*  these copyright notices.                                               *
*                                                                         *
*  Dystopia Mud improvements copyright (C) 2000, 2001 by Brian Graversen  *
*                                                                         *
*  Much time and thought has gone into this software and you are          *
*  benefitting.  We hope that you share your changes too.  What goes      *
*  around, comes around.                                                  *
***************************************************************************
*  Converted for AFKMud 1.64 by Zarius (jeff@mindcloud.com)               *
*  If you like the snippet let me know                                    *
***************************************************************************/
/**************************************************************************
 * 	                       Version History                              *
 **************************************************************************
 *  (v1.0) - Converted Automapper to AFKMud 1.64 and added additional     *
 *           directions and removed room desc code into a sep func        *
 **************************************************************************/

/*
   TODO:
   -----

   1. Add a way of displaying up and down directions effectively
 */
#include <algorithm>
#include "mud.h"
#include "descriptor.h"
#include "mapper.h"
#include "overland.h"   // For sect_show
#include "roomindex.h"

bool check_blind( char_data * );

/* The map itself */
map_type dmap[MAPX + 1][MAPY + 1];

// Defines the boundary of the map box.
bool BOUNDARY( int x, int y )
{
   if( x < 0 || y < 0 || x > MAPX || y > MAPY )
      return true;
   return false;
}

/* Take care of some repetitive code for later */
void get_exit_dir( int dir, int & x, int & y, int xorig, int yorig )
{
   /*
    * Get the next coord based on direction 
    */
   switch ( dir )
   {
      case DIR_NORTH:  /* North */
         x = xorig;
         y = yorig - 1;
         break;

      case DIR_EAST:   /* East */
         x = xorig + 1;
         y = yorig;
         break;

      case DIR_SOUTH:  /* South */
         x = xorig;
         y = yorig + 1;
         break;

      case DIR_WEST:   /* West */
         x = xorig - 1;
         y = yorig;
         break;

      case DIR_UP:  /* UP */
         break;

      case DIR_DOWN:   /* DOWN */
         break;

      case DIR_NORTHEAST: /* NE */
         x = xorig + 1;
         y = yorig - 1;
         break;

      case DIR_NORTHWEST: /* NW */
         x = xorig - 1;
         y = yorig - 1;
         break;

      case DIR_SOUTHEAST: /* SE */
         x = xorig + 1;
         y = yorig + 1;
         break;

      case DIR_SOUTHWEST: /* SW */
         x = xorig - 1;
         y = yorig + 1;
         break;

      default:
         x = -1;
         y = -1;
         break;
   }
}

/* Clear one map coord */
void clear_coord( int x, int y )
{
   dmap[x][y].tegn = ' ';
   dmap[x][y].vnum = 0;
   dmap[x][y].depth = 0;
   dmap[x][y].sector = -1;
//   xCLEAR_BITS( dmap[x][y].info );
   dmap[x][y].can_see = true;
}

/* Clear all exits for one room */
void clear_room( int x, int y )
{
   int dir, exitx, exity;

   /*
    * Cycle through the four directions 
    */
   for( dir = 0; dir < 4; ++dir )
   {
      /*
       * Find next coord in this direction 
       */
      get_exit_dir( dir, exitx, exity, x, y );

      /*
       * If coord is valid, clear it 
       */
      if( !BOUNDARY( exitx, exity ) )
         clear_coord( exitx, exity );
   }
}

std::string get_exits( char_data* ch )
{
   if( !ch || !check_blind( ch ) )
      return "";

   std::string result = "[Exits:";
   bool found = false;

   for( auto* pexit : ch->in_room->exits )
   {
      bool is_immort = ch->is_immortal();

      // Mortal visibility logic
      bool is_hidden = IS_EXIT_FLAG( pexit, EX_SECRET ) ||
         IS_EXIT_FLAG( pexit, EX_HIDDEN ) ||
         IS_EXIT_FLAG( pexit, EX_FORTIFIED ) ||
         IS_EXIT_FLAG( pexit, EX_HEAVY ) ||
         IS_EXIT_FLAG( pexit, EX_MEDIUM ) ||
         IS_EXIT_FLAG( pexit, EX_LIGHT ) ||
         IS_EXIT_FLAG( pexit, EX_CRUMBLING ) ||
         ( IS_EXIT_FLAG( pexit, EX_WINDOW ) && !IS_EXIT_FLAG( pexit, EX_ISDOOR ) );

      if( !pexit->to_room )
         continue;

      if( !is_immort && is_hidden )
         continue;

      found = true;
      result += std::format( " {}", capitalize( dir_name[pexit->vdir] ) );

      /*
       * Immortals see all exits, even secret ones
       */
      if( is_immort )
      {
         if( IS_EXIT_FLAG( pexit, EX_OVERLAND ) ) result += "->(Overland)";
         if( IS_EXIT_FLAG( pexit, EX_CLOSED ) )   result += "->(Closed)";
         if( IS_EXIT_FLAG( pexit, EX_DIG ) )      result += "->(Dig)";
         if( IS_EXIT_FLAG( pexit, EX_WINDOW ) )   result += "->(Window)";
         if( IS_EXIT_FLAG( pexit, EX_HIDDEN ) )   result += "->(Hidden)";
         if( pexit->to_room->flags.test( ROOM_DEATH ) ) result += "->(Deathtrap)";
      }
      else
      {
         if( IS_EXIT_FLAG( pexit, EX_CLOSED ) ) result += "->(Closed)";
         if( ch->has_aflag( AFF_DETECTTRAPS ) && pexit->to_room->flags.test( ROOM_DEATH ) )
            result += "->(Deathtrap)";
      }
   }

   result += found ? "]\r\n" : " none]\r\n";

   return std::format( "{}{}", ch->color_str( AT_EXITS ), result );
}

/* This function is recursive, ie it calls itself */
void map_exits( char_data* ch, room_index* pRoom, int x, int y, int depth )
{
   static constexpr std::array<char, 10> map_chars = {'|', '-', '|', '-', 'U', 'D', '/', '\\', '\\', '/'};
   exit_data *pExit;
   int exitx = 0, exity = 0;
   int roomx = 0, roomy = 0;

   switch( pRoom->sector_type )
   {
      case SECT_INDOORS:
         dmap[x][y].tegn = 'O';
         dmap[x][y].sector = -1;
         break;
      default:
         dmap[x][y].sector = pRoom->sector_type;
         break;
   }

   if( pRoom->flags.test( ROOM_MAP ) )
      dmap[x][y].sector = SECT_EXIT;

   dmap[x][y].vnum = pRoom->vnum;
   dmap[x][y].depth = depth;
   //   dmap[x][y].info = pRoom->room_flags;
   dmap[x][y].can_see = pRoom->is_dark( ch );

   if( depth > MAXDEPTH || pRoom->flags.test( ROOM_MAP ) )
      return;

   for( int door = 0; door < 10; ++door )
   {
      /*
       * Skip if there is no exit in this direction
       */
      if( !( pExit = pRoom->get_exit( door ) ) )
         continue;

      if( door == 4 || door == 5 )
         continue;

      pExit = pRoom->get_exit( door );
      if( !pExit || !pExit->to_room )
         continue;

      /*
       * Get the coords for the next exit and room in this direction
       */
      get_exit_dir( door, exitx, exity, x, y );
      get_exit_dir( door, roomx, roomy, exitx, exity );

      /*
       * Skip if coords fall outside map
       */
      if( BOUNDARY( exitx, exity ) || BOUNDARY( roomx, roomy ) )
         continue;

      /*
       * Skip if there is no room beyond this exit
       */
      if( !pExit->to_room )
         continue;

      /*
       * Ensure there are no clashes with previously defined rooms
       */
      if( dmap[roomx][roomy].vnum != 0 && dmap[roomx][roomy].vnum != pExit->to_room->vnum )
      {
         /*
          * Use the new room if the depth is higher
          */
         if( dmap[roomx][roomy].depth <= depth )
            continue;

         /*
          * It is so clear the old room
          */
         clear_room( roomx, roomy );
      }

      /*
       * No exits at MAXDEPTH
       */
      if( depth == MAXDEPTH )
         continue;

      /*
       * No need for exits that are already mapped
       */
      if( dmap[exitx][exity].depth > 0 )
         continue;

      /*
       * Fill in exit
       */
      dmap[exitx][exity].depth = depth;
      dmap[exitx][exity].vnum = pExit->to_room->vnum;
      //      dmap[exitx][exity].info = pExit->exit_info;
      dmap[exitx][exity].tegn = map_chars[door];
      dmap[exitx][exity].sector = -1;

      /*
       * More to do? If so we recurse
       */
      if( depth < MAXDEPTH && ( dmap[roomx][roomy].vnum == pExit->to_room->vnum || dmap[roomx][roomy].vnum == 0 ) )
      {
         map_exits(ch, pExit->to_room, roomx, roomy, depth + 1);
      }
   }
}

/* Reformat room descriptions to exclude undesirable characters */
void reformat_desc( std::string & desc )
{
   // Replace all returns/newlines/tabs with spaces.
   std::replace_if( desc.begin(), desc.end(), []( char c )
   {
      return c == '\r' || c == '\n' || c == '\t';
   }, ' ' );

   /*
    * Two or more consecutive spaces?
    */
   auto new_end = std::unique( desc.begin(), desc.end(), []( char a, char b )
   {
      return a == ' ' && b == ' ';
   } );

   desc.erase( new_end, desc.end() );
}

std::string whatColor( std::string_view str, std::string_view pos )
{
   std::string col;

   std::string_view segment = str.substr( 0, str.find (pos ) );

   size_t i = 0;
   while( i < segment.length() )
   {
      if( segment[i] == '&' || segment[i] == '{' || segment[i] == '}' )
      {
         if(i + 1 < segment.length() )
         {
            col = std::string( segment.substr( i, 2 ) );
         }
      }
      ++i;
   }

   return col;
}

size_t get_line( std::string_view desc, size_t max_len )
{
   if( desc.length() <= max_len )
      return 0;

   size_t visual_len = 0;
   size_t byte_idx = 0;

   /*
    * Calculate end point in string without color.
    */
   while( byte_idx < desc.length() && visual_len <= max_len )
   {
      if( desc[byte_idx] == '&' || desc[byte_idx] == '{' || desc[byte_idx] == '}' )
      {
         size_t consumed = 0;

         // We call colorcode with nullptr for descriptor because we
         // only care about the length consumed, not the actual ANSI translation.
         colorcode( desc.substr( byte_idx ), nullptr, consumed );

         if( consumed > 0 )
            byte_idx += consumed; // Tokens (like color codes) have a visual length of 0 so we don't add to byte_idx.
         else
         {
            // Not a valid color token, treat as normal char.
            ++visual_len;
            ++byte_idx;
         }
      }
      else
      {
         // No conversion, just count
         ++visual_len;
         ++byte_idx;
      }
   }

   if( byte_idx >= desc.length() )
      return 0;

   // Find the nearest space.
   size_t break_idx = desc.find_last_of( ' ', byte_idx );

   // There could be a problem if there are no spaces on the line.
   return ( break_idx == std::string_view::npos ) ? byte_idx : break_idx + 1;
}

std::string get_next_line_for_map( std::string_view & remaining, size_t width, char_data * ch, std::string_view original_text, bool & alldesc )
{
   int len = get_line( remaining.data(), width );
   std::string_view line = ( len > 0 ) ? remaining.substr( 0, len ) : remaining;

   std::string color = whatColor( original_text, line );
   std::string result = ( color.empty() ? ch->color_str( AT_RMDESC ) : std::string( color ) );
   result += line;

   if( len <= 0 )
      alldesc = true;
   else remaining.remove_prefix( len );

   return result;
}

/* Display the map to the player */
void show_map( char_data* ch, std::string_view text )
{
   if( !ch )
      return;

   std::string output;
   std::string_view remaining_text = ( !text.empty() ? text : "" );
   bool alldesc = remaining_text.empty();

   if( ch->has_pcflag( PCFLAG_AUTOEXIT ) )
   {
      output += std::format( "{}{}\r\n", ch->color_str( AT_EXITS ), get_exits( ch ) );
   }

   output += std::format( "&z+-----------+&w {}\r\n", ( alldesc ? "" : get_next_line_for_map( remaining_text, 63, ch, text, alldesc ) ) );

   for( int y = 0; y <= MAPY; ++y )
   {
      std::string row = "&z|&D";

      for( int x = 0; x <= MAPX; ++x )
      {
         auto& cell = dmap[x][y];

         if( cell.sector < 0 )
         {
            switch ( cell.tegn )
            {
               case '-': case '|': case '\\': case '/': row += std::format( "&O{}&d", cell.tegn ); break;
               case '@': row += std::format( "&R{}&d", cell.tegn ); break;
               case 'O': row += std::format( "&w{}&d", cell.tegn ); break;
               default:  row += cell.tegn; break;
            }
         }
         else
         {
            row += std::format( "{}{}&d", sect_show[cell.sector].color, sect_show[cell.sector].symbol );
         }
      }

      row += "&z|&D ";

      if( !alldesc )
      {
         row += get_next_line_for_map( remaining_text, 63, ch, text, alldesc );
      }

      output += row + "\r\n";
   }

   output += "&z+-----------+&D ";

   if( !alldesc )
      output += get_next_line_for_map( remaining_text, 63, ch, text, alldesc );
   output += "\r\n";

   while( !alldesc )
   {
      output += get_next_line_for_map( remaining_text, 78, ch, text, alldesc ) + "\r\n";
   }

   ch->print( output );
}

/* Clear, generate and display the map */
void draw_map( char_data* ch, std::string_view desc )
{
   if( !ch || !ch->in_room )
      return;

   std::string formatted_desc{ desc };
   reformat_desc( formatted_desc );

   for( int y = 0; y <= MAPY; ++y )
   {
      for( int x = 0; x <= MAPX; ++x )
      {
         clear_coord(x, y);
      }
   }

   int x = MAPX / 2;
   int y = MAPY / 2;

   dmap[x][y].vnum = ch->in_room->vnum;
   dmap[x][y].depth = 0;

   map_exits( ch, ch->in_room, x, y, 0 );

   dmap[x][y].tegn = '@';
   dmap[x][y].sector = -1;

   show_map( ch, formatted_desc );
}
