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
#include <sstream>
#include "mud.h"
#include "area.h"
#include "clans.h"
#include "descriptor.h"
#include "roomindex.h"

#define WEB_ROOMS "../public_html/"

char *rankbuffer( char_data * );
extern int num_logins;

int web_colour( char type, char *string, bool & firsttag )
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
      mudstrlcat( buffer, "</span>", 64000 );
}

void web_who(  )
{
   FILE *webwho = NULL;
   list < descriptor_data * >::iterator ds;
   ostringstream webbuf, buf;
   string rank, outbuf, stats, clan_name;
   int pcount = 0, amount, xx = 0, yy = 0;

   if( !( webwho = fopen( WEBWHO_FILE, "w" ) ) )
   {
      bug( "%s: Unable to open webwho file for writing!", __FUNCTION__ );
      return;
   }

   buf << "&R-=[ &WPlayers on " << sysdata->mud_name << "&R]=-&d";
   webbuf << color_align( buf.str(  ).c_str(  ), 80, ALIGN_CENTER ) << "\n";

   buf.clear(  );
   buf << "&Y-=[&d &Wtelnet://" << sysdata->telnet << ":" << mud_port << "&d &Y]=-&d";
   amount = 78 - color_strlen( buf.str(  ).c_str(  ) );  /* Determine amount to put in front of line */

   if( amount < 1 )
      amount = 1;

   amount = amount / 2;

   for( xx = 0; xx < amount; ++xx )
      webbuf << " ";

   webbuf << buf << "\n";

   xx = 0;
   for( ds = dlist.begin(  ); ds != dlist.end(  ); ++ds )
   {
      descriptor_data *d = *ds;
      char_data *person = d->original ? d->original : d->character;

      if( !d )
      {
         bug( "%s: NULL DESCRIPTOR in list!", __FUNCTION__ );
         continue;
      }

      if( person && d->connected >= CON_PLAYING )
      {
         if( person->level >= LEVEL_IMMORTAL )
            continue;

         if( xx == 0 )
            webbuf << "\n&B--------------------------------=[&d &WPlayers&d &B]=---------------------------------&d\n\n";

         ++pcount;

         rank = rankbuffer( person );
         outbuf = color_align( rank.c_str(  ), 20, ALIGN_CENTER );

         webbuf << outbuf;

         stats = "&z[";
         if( person->has_pcflag( PCFLAG_AFK ) )
            stats += "AFK";
         else
            stats += "---";
         if( person->CAN_PKILL(  ) )
            stats += "PK]&d";
         else
            stats += "--]&d";
         stats += "&G";

         if( person->pcdata->clan )
            clan_name = " &c[" + person->pcdata->clan->name + "&c]&d";
         else
            clan_name.clear(  );

         if( !person->pcdata->homepage.empty(  ) )
            webbuf << stats << " <a href=\"" << person->pcdata->homepage << "\" target=\"_blank\">" << person->name << "</a>" << person->pcdata->title << clan_name << "&d\n";
         else
            webbuf << stats << " &G" << person->name << person->pcdata->title << clan_name << "&d\n";
         ++xx;
      }
   }

   yy = 0;
   for( ds = dlist.begin(  ); ds != dlist.end(  ); ++ds )
   {
      descriptor_data *d = *ds;
      char_data *person = d->original ? d->original : d->character;

      if( !d )
      {
         bug( "%s: NULL DESCRIPTOR in list!", __FUNCTION__ );
         continue;
      }

      if( person && d->connected >= CON_PLAYING )
      {
         if( person->level < LEVEL_IMMORTAL )
            continue;

         if( person->has_pcflag( PCFLAG_WIZINVIS ) )
            continue;

         if( yy == 0 )
            webbuf << "\n&R-------------------------------=[&d &WImmortals&d &R]=--------------------------------&d\n\n";

         ++pcount;

         rank = rankbuffer( person );
         outbuf = color_align( rank.c_str(  ), 20, ALIGN_CENTER );

         webbuf << outbuf;

         stats = "&z[";
         if( person->has_pcflag( PCFLAG_AFK ) )
            stats += "AFK";
         else
            stats += "---";

         if( person->CAN_PKILL(  ) )
            stats += "PK]&d";
         else
            stats = "--]&d";
         stats += "&G";

         if( person->pcdata->clan )
            clan_name = " &c[" + person->pcdata->clan->name + "&c]&d";
         else
            clan_name.clear(  );

         if( !person->pcdata->homepage.empty(  ) )
            webbuf << stats << " <a href=\"" << person->pcdata->homepage << "\" target=\"_blank\">" << person->name << "</a>" << person->pcdata->title << clan_name << "&d\n";
         else
            webbuf << stats << " &G" << person->name << person->pcdata->title << clan_name << "&d\n";
         ++yy;
      }
   }

   char col_buf[64000];

   webbuf << "\n&Y[&d&W" << pcount << " Player" << ( pcount == 1 ? "" : "s" ) << "&d&Y] ";
   webbuf << "[&d&WHomepage: <a href=\"" << sysdata->http << "\" target=\"_blank\">" << sysdata->
      http << "</a>&d&Y] [&d&W" << sysdata->maxplayers << " Max Since Reboot&d&Y]&d\n";
   webbuf << "&Y[&d&W" << num_logins << " login" << ( num_logins == 1 ? "" : "s" ) << " since last reboot on " << str_boot_time << "&d&Y]&d\n";
   web_colourconv( col_buf, webbuf.str(  ).c_str(  ) );
   fprintf( webwho, "%s", col_buf );
   FCLOSE( webwho );
}

void web_arealist(  )
{
   const char *print_string =
      "<tr><td><font color=\"red\">%s   </font></td><td><font color=\"yellow\">%s</font></td><td><font color=\"green\">%d - %d   </font></td><td><font color=\"blue\">%d - %d</font></td></tr>\n";
   list < area_data * >::iterator pArea;
   FILE *fp;

   if( !( fp = fopen( AREALIST_FILE, "w" ) ) )
   {
      bug( "%s: Unable to open arealist file for writing!", __FUNCTION__ );
      return;
   }

   fprintf( fp,
            "<table><tr><td><font color=\"red\">Author   </font></td><td><font color=\"yellow\">Area</font></td><td><font color=\"green\">Recommened   </font></td><td><font color=\"blue\">Enforced</font></td></tr>\n" );

   for( pArea = area_nsort.begin(  ); pArea != area_nsort.end(  ); ++pArea )
   {
      area_data *area = *pArea;

      if( !area->flags.test( AFLAG_PROTOTYPE ) )
         fprintf( fp, print_string, area->author, area->name, area->low_soft_range, area->hi_soft_range, area->low_hard_range, area->hi_hard_range );
   }
   fprintf( fp, "%s", "</table>\n" );
   FCLOSE( fp );
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

      list < exit_data * >::iterator iexit;
      for( iexit = room->exits.begin(  ); iexit != room->exits.end(  ); ++iexit )
      {
         exit_data *pexit = *iexit;

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
         list < obj_data * >::iterator iobj;
         for( iobj = room->objects.begin(  ); iobj != room->objects.end(  ); ++iobj )
         {
            obj_data *obj = *iobj;
            if( obj->extra_flags.test( ITEM_AUCTION ) )
               continue;

            if( obj->objdesc && obj->objdesc[0] != '\0' )
               fprintf( fp, "<font color=\"#0000EE\">%s</font><br />\n", obj->objdesc );
         }

         list < char_data * >::iterator ich;
         for( ich = room->people.begin(  ); ich != room->people.end(  ); ++ich )
         {
            char_data *rch = *ich;
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
}

CMDF( do_webroom )
{
   list < room_index * >::iterator rindex;
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

   for( rindex = area->rooms.begin(  ); rindex != area->rooms.end(  ); ++rindex )
   {
      room = *rindex;

      if( room->flags.test( ROOM_AUCTION ) || room->flags.test( ROOM_PET_SHOP ) )
         continue;

      room_to_html( room, complete );
   }
   ch->print( "Bywater rooms dumped to web.\r\n" );
}
