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
 *                            Web Support Code                              *
 ****************************************************************************/

#include <cstdio>
#include "mud.h"
#include "area.h"
#include "roomindex.h"

#define WEB_ROOMS "../public_html/"

int web_colour( char type, char *string, bool &firsttag )
{
   char code[50];
   char *p = '\0';
   bool validcolor = false;

   switch ( type )
   {
      default:
         break;
      case '&':
         mudstrlcpy( code, "&amp;", 50 );
         break;
      case 'x':
         mudstrlcpy( code, "<span class=\"black\">", 50 );
         validcolor = true;
         break;
      case 'b':
         mudstrlcpy( code, "<span class=\"dblue\">", 50 );
         validcolor = true;
         break;
      case 'c':
         mudstrlcpy( code, "<span class=\"cyan\">", 50 );
         validcolor = true;
         break;
      case 'g':
         mudstrlcpy( code, "<span class=\"dgreen\">", 50 );
         validcolor = true;
         break;
      case 'r':
         mudstrlcpy( code, "<span class=\"dred\">", 50 );
         validcolor = true;
         break;
      case 'w':
         mudstrlcpy( code, "<span class=\"grey\">", 50 );
         validcolor = true;
         break;
      case 'y':
         mudstrlcpy( code, "<span class=\"yellow\">", 50 );
         validcolor = true;
         break;
      case 'Y':
         mudstrlcpy( code, "<span class=\"yellow\">", 50 );
         validcolor = true;
         break;
      case 'B':
         mudstrlcpy( code, "<span class=\"blue\">", 50 );
         validcolor = true;
         break;
      case 'C':
         mudstrlcpy( code, "<span class=\"lblue\">", 50 );
         validcolor = true;
         break;
      case 'G':
         mudstrlcpy( code, "<span class=\"green\">", 50 );
         validcolor = true;
         break;
      case 'R':
         mudstrlcpy( code, "<span class=\"red\">", 50 );
         validcolor = true;
         break;
      case 'W':
         mudstrlcpy( code, "<span class=\"white\">", 50 );
         validcolor = true;
         break;
      case 'z':
         mudstrlcpy( code, "<span class=\"dgrey\">", 50 );
         validcolor = true;
         break;
      case 'o':
         mudstrlcpy( code, "<span class=\"yellow\">", 50 );
         validcolor = true;
         break;
      case 'O':
         mudstrlcpy( code, "<span class=\"orange\">", 50 );
         validcolor = true;
         break;
      case 'p':
         mudstrlcpy( code, "<span class=\"purple\">", 50 );
         validcolor = true;
         break;
      case 'P':
         mudstrlcpy( code, "<span class=\"pink\">", 50 );
         validcolor = true;
         break;
      case '<':
         mudstrlcpy( code, "&lt;", 50 );
         break;
      case '>':
         mudstrlcpy( code, "&gt;", 50 );
         break;
      case '/':
         mudstrlcpy( code, "<br />", 50 );
         break;
      case '{':
         snprintf( code, 50, "%c", '{' );
         break;
      case '-':
         snprintf( code, 50, "%c", '~' );
         break;
   }

   if( !firsttag && validcolor )
   {
      char newcode[50];

      snprintf( newcode, 50, "</span>%s", code );
      mudstrlcpy( code, newcode, 50 );
   }

   if( firsttag && validcolor )
      firsttag = false;

   p = code;
   while( *p != '\0' )
   {
      *string = *p++;
      *++string = '\0';
   }

   return ( strlen( code ) );
}

void web_colourconv( char *buffer, const char *txt )
{
   const char *point;
   int skip = 0;
   bool firsttag = true;

   if( txt )
   {
      for( point = txt; *point; ++point )
      {
         if( *point == '&' )
         {
            ++point;
            skip = web_colour( *point, buffer, firsttag );
            while( skip-- > 0 )
               ++buffer;
            continue;
         }
         *buffer = *point;
         *++buffer = '\0';
      }
      *buffer = '\0';
   }
   if( !firsttag )
      mudstrlcat( buffer, "</span>", MSL );
   return;
}

/* Aurora's room-to-web toy - this could be quite fun to mess with */
void room_to_html( room_index * room, bool complete )
{
   FILE *fp = NULL;
   char filename[256];
   bool found = false;

   if( !room )
      return;

   snprintf( filename, 256, "%s%d.html", WEB_ROOMS, room->vnum );

   if( ( fp = fopen( filename, "w" ) ) != NULL )
   {
      char roomdesc[MSL];

      fprintf( fp, "%s", "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\">\n" );
      fprintf( fp, "<html>\n<head>\n<meta http-equiv=\"Content-Type\" content=\"text/html; charset=iso-8859-1\">\n" );
      fprintf( fp, "<title>%s: %s</title>\n</head>\n\n<body bgcolor=\"#000000\">\n", room->area->name, room->name );
      fprintf( fp, "%s", "<font face=\"Fixedsys\" size=\"3\">\n" );
      fprintf( fp, "<font color=\"#FF0000\">%s</font><br />\n", room->name );
      fprintf( fp, "%s", "<font color=\"#33FF33\">[Exits:" );

      list<exit_data*>::iterator iexit;
      for( iexit = room->exits.begin(); iexit != room->exits.end(); ++iexit )
      {
         exit_data *pexit = (*iexit);

         if( pexit->to_room ) /* Set any controls you want here, ie: not closed doors, etc */
         {
            found = true;
            fprintf( fp, " <a href=\"%d.html\">%s</a>", pexit->to_room->vnum, dir_name[pexit->vdir] );
         }
      }
      if( !found )
         fprintf( fp, "%s", " None.]</font><br />\n" );
      else
         fprintf( fp, "%s", "]</font><br />\n" );
      web_colourconv( roomdesc, room->roomdesc );
      fprintf( fp, "<font color=\"#999999\">%s</font><br />\n", roomdesc );

      if( complete )
      {
         list<obj_data*>::iterator iobj;
         for( iobj = room->objects.begin(); iobj != room->objects.end(); ++iobj )
         {
            obj_data *obj = (*iobj);
            if( obj->extra_flags.test( ITEM_AUCTION ) )
               continue;

            if( obj->objdesc && obj->objdesc[0] != '\0' )
               fprintf( fp, "<font color=\"#0000EE\">%s</font><br />\n", obj->objdesc );
         }

         list<char_data*>::iterator ich;
         for( ich = room->people.begin(); ich != room->people.end(); ++ich )
         {
            char_data *rch = (*ich);
            if( rch->isnpc(  ) )
               fprintf( fp, "<font color=\"#FF00FF\">%s</font><br />\n", rch->long_descr );
            else
            {
               char pctitle[MSL];

               web_colourconv( pctitle, rch->pcdata->title );
               fprintf( fp, "<font color=\"#FF00FF\">%s %s</font><br />\n", rch->name, pctitle );
            }
         }
      }
      fprintf( fp, "%s", "</font>\n</body>\n</html>\n" );
      FCLOSE( fp );
   }
   else
      bug( "%s: Error Opening room to html index stream!", __FUNCTION__ );

   return;
}

CMDF( do_webroom )
{
   list<room_index*>::iterator rindex;
   room_index *room;
   area_data *area;
   bool complete = false;

   if( !str_cmp( argument, "complete" ) )
      complete = true;

   /*
    * Limiting this to Bywater since I don't want the whole mud "webized" - silent return
    */
   if( !( area = find_area( "bywater.are" ) ) )
      return;

   for( rindex = area->rooms.begin(); rindex != area->rooms.end(); ++rindex )
   {
      room = *rindex;

      if( room->flags.test( ROOM_AUCTION ) || room->flags.test( ROOM_PET_SHOP ) )
         continue;

      room_to_html( room, complete );
   }
   ch->print( "Bywater rooms dumped to web.\r\n" );
   return;
}
