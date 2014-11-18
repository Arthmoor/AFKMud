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
*  Downloaded from http://www.mindcloud.com                               *
*  If you like the snippet let me know                                    *
***************************************************************************/
/**************************************************************************
 * 	                       Version History                              *
 **************************************************************************
 *  (v1.0) - Converted Automapper to AFKMud 1.64 and added additional     *
 *           directions and removed room desc code into a sep func        *
 **************************************************************************/

/*
   TO DO
   -----

   1. Add a way of displaying up and down directions effectively
 */

#include <cstring>
#include "mud.h"
#include "descriptor.h"
#include "mapper.h"
#include "overland.h"   // For sect_show
#include "roomindex.h"

bool check_blind( char_data * );

/* The map itself */
map_type dmap[MAPX + 1][MAPY + 1];

/* Take care of some repetitive code for later */
void get_exit_dir( int dir, int &x, int &y, int xorig, int yorig )
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

char *get_exits( char_data * ch )
{
   static char buf[MSL];

   buf[0] = '\0';

   if( !check_blind( ch ) )
      return buf;

   ch->set_color( AT_EXITS );

   mudstrlcpy( buf, "[Exits:", MSL );

   bool found = false;
   list < exit_data * >::iterator iexit;
   for( iexit = ch->in_room->exits.begin(  ); iexit != ch->in_room->exits.end(  ); ++iexit )
   {
      exit_data *pexit = *iexit;

      if( ch->is_immortal(  ) )
         /*
          * Immortals see all exits, even secret ones 
          */
      {
         if( pexit->to_room )
         {
            found = true;
            mudstrlcat( buf, " ", MSL );

            mudstrlcat( buf, capitalize( dir_name[pexit->vdir] ), MSL );

            if( IS_EXIT_FLAG( pexit, EX_OVERLAND ) )
               mudstrlcat( buf, "->(Overland)", MSL );

            /*
             * New code added to display closed, or otherwise invisible exits to immortals 
             * Installed by Samson 1-25-98 
             */
            if( IS_EXIT_FLAG( pexit, EX_CLOSED ) )
               mudstrlcat( buf, "->(Closed)", MSL );
            if( IS_EXIT_FLAG( pexit, EX_DIG ) )
               mudstrlcat( buf, "->(Dig)", MSL );
            if( IS_EXIT_FLAG( pexit, EX_WINDOW ) )
               mudstrlcat( buf, "->(Window)", MSL );
            if( IS_EXIT_FLAG( pexit, EX_HIDDEN ) )
               mudstrlcat( buf, "->(Hidden)", MSL );
            if( pexit->to_room->flags.test( ROOM_DEATH ) )
               mudstrlcat( buf, "->(Deathtrap)", MSL );
         }
      }
      else
      {
         if( pexit->to_room && !IS_EXIT_FLAG( pexit, EX_SECRET ) && ( !IS_EXIT_FLAG( pexit, EX_WINDOW ) || IS_EXIT_FLAG( pexit, EX_ISDOOR ) ) && !IS_EXIT_FLAG( pexit, EX_HIDDEN ) && !IS_EXIT_FLAG( pexit, EX_FORTIFIED ) /* Checks for walls, Marcus */
             && !IS_EXIT_FLAG( pexit, EX_HEAVY ) && !IS_EXIT_FLAG( pexit, EX_MEDIUM ) && !IS_EXIT_FLAG( pexit, EX_LIGHT ) && !IS_EXIT_FLAG( pexit, EX_CRUMBLING ) )
         {
            found = true;
            mudstrlcat( buf, " ", MSL );

            mudstrlcat( buf, capitalize( dir_name[pexit->vdir] ), MSL );

            if( IS_EXIT_FLAG( pexit, EX_CLOSED ) )
               mudstrlcat( buf, "->(Closed)", MSL );
            if( ch->has_aflag( AFF_DETECTTRAPS ) && pexit->to_room->flags.test( ROOM_DEATH ) )
               mudstrlcat( buf, "->(Deathtrap)", MSL );
         }
      }
   }

   if( !found )
      mudstrlcat( buf, " none]", MSL );
   else
      mudstrlcat( buf, "]", MSL );
   mudstrlcat( buf, "\r\n", MSL );
   return buf;
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

/* This function is recursive, ie it calls itself */
void map_exits( char_data * ch, room_index * pRoom, int x, int y, int depth )
{
   static char map_chars[11] = "|-|-UD/\\\\/";
   int door;
   int exitx = 0, exity = 0;
   int roomx = 0, roomy = 0;
   exit_data *pExit;

   /*
    * Setup this coord as a room - Change any symbols that can't be displayed here 
    */
   switch ( pRoom->sector_type )
   {
      case SECT_INDOORS:
         dmap[x][y].tegn = 'O';
         dmap[x][y].sector = -1;
         break;

      default:
         dmap[x][y].sector = pRoom->sector_type;
         break;
   }

   /*
    * An exit out to the overland should display as such 
    */
   if( pRoom->flags.test( ROOM_MAP ) )
      dmap[x][y].sector = SECT_EXIT;

   dmap[x][y].vnum = pRoom->vnum;
   dmap[x][y].depth = depth;
//   dmap[x][y].info = pRoom->room_flags;
   dmap[x][y].can_see = pRoom->is_dark( ch );

   /*
    * Limit recursion 
    */
   if( depth > MAXDEPTH )
      return;

   /*
    * No exits for overland sectors 
    */
   if( pRoom->flags.test( ROOM_MAP ) )
      return;

   /*
    * This room is done, deal with it's exits 
    */
   for( door = 0; door < 10; ++door )
   {
      /*
       * Skip if there is no exit in this direction 
       */
      if( !( pExit = pRoom->get_exit( door ) ) )
         continue;

      /*
       * Skip up and down until I can figure out a good way to display it 
       */
      if( door == 4 || door == 5 )
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
      if( ( dmap[roomx][roomy].vnum != 0 ) && ( dmap[roomx][roomy].vnum != pExit->to_room->vnum ) )
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
      if( depth < MAXDEPTH && !pRoom->flags.test( ROOM_MAP ) && ( ( dmap[roomx][roomy].vnum == pExit->to_room->vnum ) || ( dmap[roomx][roomy].vnum == 0 ) ) )
      {
         /*
          * Depth increases by one each time 
          */
         map_exits( ch, pExit->to_room, roomx, roomy, depth + 1 );
      }
   }
}

/* Reformat room descriptions to exclude undesirable characters */
void reformat_desc( char *desc )
{
   /*
    * Index variables to keep track of array/pointer elements 
    */
   size_t i = 0;
   int j = 0;
   char buf[MSL], *p;

   buf[0] = '\0';

   if( !desc )
      return;

   /*
    * Replace all "\n" and "\r" with spaces 
    */
   for( i = 0; i <= strlen( desc ); ++i )
   {
      if( ( desc[i] == '\r' ) || ( desc[i] == '\n' ) )
         desc[i] = ' ';
   }

   /*
    * Remove multiple spaces 
    */
   for( p = desc; *p != '\0'; ++p )
   {
      buf[j] = *p;
      ++j;

      /*
       * Two or more consecutive spaces? 
       */
      if( ( *p == ' ' ) && ( *( p + 1 ) == ' ' ) )
      {
         do
         {
            ++p;
         }
         while( *( p + 1 ) == ' ' );
      }
   }

   buf[j] = '\0';

   /*
    * Copy to desc 
    */
   mudstrlcpy( desc, buf, MSL );
}

int get_line( const char *desc, size_t max_len )
{
   size_t i, j = 0;

   /*
    * Return if it's short enough for one line 
    */
   if( strlen( desc ) <= max_len )
      return 0;

   /*
    * Calculate end point in string without color 
    */
   for( i = 0; i <= strlen( desc ); ++i )
   {
      char dst[20];
      int vislen;

      switch ( desc[i] )
      {
         case '&':  /* NORMAL, Foreground colour */
         case '{':  /* BACKGROUND colour */
         case '}':  /* BLINK Foreground colour */
            *dst = '\0';
            vislen = 0;
            i += colorcode( &desc[i], dst, nullptr, 20, &vislen ); /* Skip input token */
            j += vislen; /* Count output token length */
            break;   /* this was missing - if you have issues, remove it */

         default:   /* No conversion, just count */
            ++j;
            break;
      }

      if( j > max_len )
         break;
   }

   /*
    * End point is now in i, find the nearest space 
    */
   for( j = i; j > 0; --j )
   {
      if( desc[j] == ' ' )
         break;
   }

   /*
    * There could be a problem if there are no spaces on the line 
    */
   return j + 1;
}

char *whatColor( const char *str, const char *pos )
{
   static char col[3];

   col[0] = '\0';
   while( str != pos )
   {
      if( *str == '&' || *str == '{' || *str == '}' )
      {
         col[0] = *str;

         ++str;
         if( !str )
         {
            col[1] = '\0';
            break;
         }
         col[1] = *str;
      }
      ++str;
   }
   col[2] = '\0';
   return col;
}

/* Display the map to the player */
void show_map( char_data * ch, char *text )
{
   char buf[MSL * 2];
   int x, y, pos;
   char *p;
   bool alldesc = false;   /* Has desc been fully displayed? */

   if( !text )
      alldesc = true;

   pos = 0;
   p = text;
   buf[0] = '\0';

   /*
    * Show exits 
    */
   if( ch->has_pcflag( PCFLAG_AUTOEXIT ) )
      snprintf( buf, MSL * 2, "%s%s", ch->color_str( AT_EXITS ), get_exits( ch ) );
   else
      mudstrlcpy( buf, "", MSL * 2 );

   /*
    * Top of map frame 
    */
   mudstrlcat( buf, "&z+-----------+&w ", MSL * 2 );
   if( !alldesc )
   {
      pos = get_line( p, 63 );
      if( pos > 0 )
      {
         mudstrlcat( buf, ch->color_str( AT_RMDESC ), MSL * 2 );
         strncat( buf, p, pos );
         p += pos;
      }
      else
      {
         mudstrlcat( buf, ch->color_str( AT_RMDESC ), MSL * 2 );
         mudstrlcat( buf, p, MSL * 2 );
         alldesc = true;
      }
   }
   mudstrlcat( buf, "\r\n", MSL * 2 );

   /*
    * Write out the main map area with text 
    */
   for( y = 0; y <= MAPY; ++y )
   {
      mudstrlcat( buf, "&z|&D", MSL * 2 );

      for( x = 0; x <= MAPX; ++x )
      {
         int sect = dmap[x][y].sector;

         if( sect < 0 )
         {
            switch ( dmap[x][y].tegn )
            {
               case '-':
               case '|':
               case '\\':
               case '/':
                  snprintf( buf + strlen( buf ), ( MSL * 2 ) - strlen( buf ), "&O%c&d", dmap[x][y].tegn );
                  break;

               case '@':  // Character is standing here
                  snprintf( buf + strlen( buf ), ( MSL * 2 ) - strlen( buf ), "&R%c&d", dmap[x][y].tegn );
                  break;

               case 'O':  // Indoors
                  snprintf( buf + strlen( buf ), ( MSL * 2 ) - strlen( buf ), "&w%c&d", dmap[x][y].tegn );
                  break;

               default:   // Empty space
                  snprintf( buf + strlen( buf ), ( MSL * 2 ) - strlen( buf ), "%c", dmap[x][y].tegn );
                  break;
            }
         }
         else
         {
            snprintf( buf + strlen( buf ), ( MSL * 2 ) - strlen( buf ), "%s%s&d", sect_show[sect].color, sect_show[sect].symbol );
         }
      }
      mudstrlcat( buf, "&z|&D ", MSL * 2 );

      /*
       * Add the text, if necessary 
       */
      if( !alldesc )
      {
         pos = get_line( p, 63 );
         char col[10], c[3];

         strcpy( c, whatColor( text, p ) );
         if( c[0] == '\0' )
            mudstrlcpy( col, ch->color_str( AT_RMDESC ), 10 );
         else
            snprintf( col, 10, "%s", c );

         if( pos > 0 )
         {
            mudstrlcat( buf, col, MSL * 2 );
            strncat( buf, p, pos );
            p += pos;
         }
         else
         {
            mudstrlcat( buf, col, MSL * 2 );
            mudstrlcat( buf, p, MSL * 2 );
            alldesc = true;
         }
      }
      mudstrlcat( buf, "\r\n", MSL * 2 );
   }

   /*
    * Finish off map area 
    */
   mudstrlcat( buf, "&z+-----------+&D ", MSL * 2 );
   if( !alldesc )
   {
      char col[10], c[3];
      pos = get_line( p, 63 );

      strcpy( c, whatColor( text, p ) );
      if( c[0] == '\0' )
         mudstrlcpy( col, ch->color_str( AT_RMDESC ), 10 );
      else
         snprintf( col, 10, "%s", c );

      if( pos > 0 )
      {
         mudstrlcat( buf, col, MSL * 2 );
         strncat( buf, p, pos );
         p += pos;
         mudstrlcat( buf, "\r\n", MSL * 2 );
      }
      else
      {
         mudstrlcat( buf, col, MSL * 2 );
         mudstrlcat( buf, p, MSL * 2 );
         alldesc = true;
      }
   }

   /*
    * Deal with any leftover text 
    */
   if( !alldesc )
   {
      char col[10], c[3];

      do
      {
         /*
          * Note the number - no map to detract from width 
          */
         pos = get_line( p, 78 );

         strcpy( c, whatColor( text, p ) );
         if( c[0] == '\0' )
            mudstrlcpy( col, ch->color_str( AT_RMDESC ), 10 );
         else
            snprintf( col, 10, "%s", c );

         if( pos > 0 )
         {
            mudstrlcat( buf, col, MSL * 2 );
            strncat( buf, p, pos );
            p += pos;
            mudstrlcat( buf, "\r\n", MSL * 2 );
         }
         else
         {
            mudstrlcat( buf, col, MSL * 2 );
            mudstrlcat( buf, p, MSL * 2 );
            alldesc = true;
         }
      }
      while( !alldesc );
   }
   mudstrlcat( buf, "&D\r\n", MSL * 2 );
   ch->print( buf );
}

/* Clear, generate and display the map */
void draw_map( char_data * ch, const char *desc )
{
   int x, y;
   static char buf[MSL];

   mudstrlcpy( buf, desc, MSL );
   /*
    * Remove undesirable characters 
    */
   reformat_desc( buf );

   /*
    * Clear map 
    */
   for( y = 0; y <= MAPY; ++y )
   {
      for( x = 0; x <= MAPX; ++x )
      {
         clear_coord( x, y );
      }
   }

   /*
    * Start with players pos at centre of map 
    */
   x = MAPX / 2;
   y = MAPY / 2;

   dmap[x][y].vnum = ch->in_room->vnum;
   dmap[x][y].depth = 0;

   /*
    * Generate the map 
    */
   map_exits( ch, ch->in_room, x, y, 0 );

   /*
    * Current position should be a "X" 
    */
   dmap[x][y].tegn = '@';
   dmap[x][y].sector = -1;

   /*
    * Send the map 
    */
   show_map( ch, buf );
}
