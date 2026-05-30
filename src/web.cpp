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
 *                            Web Support Code                              *
 ****************************************************************************/

#include <filesystem>
#include <format>
#include <fstream>
#include <memory>
#include <sstream>
#include "mud.h"
#include "area.h"
#include "clans.h"
#include "descriptor.h"
#include "roomindex.h"

#define WEB_ROOMS "../public_html/"

namespace fs = std::filesystem;

/*
 * Structure used to build wizlist
 */
class wizentweb
{
public:
   wizentweb( ) : level( 0 ) {}

   string name;
   string http;
   short level;
};

std::list<std::unique_ptr<wizentweb>> wizlistweb;

void free_wizlist_web_data()
{
   wizlistweb.clear();
}

string rankbuffer( char_data * );
extern int num_logins;

int web_colour( char type, char *string, bool & firsttag )
{
   char code[50];
   char *p = nullptr;
   bool validcolor = false;

   switch ( type )
   {
      default:
         break;
      case '&':
         strlcpy( code, "&amp;", 50 );
         break;
      case 'x':
         strlcpy( code, "<span class=\"black\">", 50 );
         validcolor = true;
         break;
      case 'b':
         strlcpy( code, "<span class=\"dblue\">", 50 );
         validcolor = true;
         break;
      case 'c':
         strlcpy( code, "<span class=\"cyan\">", 50 );
         validcolor = true;
         break;
      case 'g':
         strlcpy( code, "<span class=\"dgreen\">", 50 );
         validcolor = true;
         break;
      case 'r':
         strlcpy( code, "<span class=\"dred\">", 50 );
         validcolor = true;
         break;
      case 'w':
         strlcpy( code, "<span class=\"grey\">", 50 );
         validcolor = true;
         break;
      case 'y':
         strlcpy( code, "<span class=\"yellow\">", 50 );
         validcolor = true;
         break;
      case 'Y':
         strlcpy( code, "<span class=\"yellow\">", 50 );
         validcolor = true;
         break;
      case 'B':
         strlcpy( code, "<span class=\"blue\">", 50 );
         validcolor = true;
         break;
      case 'C':
         strlcpy( code, "<span class=\"lblue\">", 50 );
         validcolor = true;
         break;
      case 'G':
         strlcpy( code, "<span class=\"green\">", 50 );
         validcolor = true;
         break;
      case 'R':
         strlcpy( code, "<span class=\"red\">", 50 );
         validcolor = true;
         break;
      case 'W':
         strlcpy( code, "<span class=\"white\">", 50 );
         validcolor = true;
         break;
      case 'z':
         strlcpy( code, "<span class=\"dgrey\">", 50 );
         validcolor = true;
         break;
      case 'o':
         strlcpy( code, "<span class=\"yellow\">", 50 );
         validcolor = true;
         break;
      case 'O':
         strlcpy( code, "<span class=\"orange\">", 50 );
         validcolor = true;
         break;
      case 'p':
         strlcpy( code, "<span class=\"purple\">", 50 );
         validcolor = true;
         break;
      case 'P':
         strlcpy( code, "<span class=\"pink\">", 50 );
         validcolor = true;
         break;
      case '<':
         strlcpy( code, "&lt;", 50 );
         break;
      case '>':
         strlcpy( code, "&gt;", 50 );
         break;
      case '/':
         strlcpy( code, "<br />", 50 );
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
      char newcode[70];

      snprintf( newcode, 70, "</span>%s", code );
      strlcpy( code, newcode, 50 );
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
      strlcat( buffer, "</span>", 64000 );
}

void web_who(  )
{
   FILE *webwho = nullptr;
   list < descriptor_data * >::iterator ds;
   ostringstream webbuf, buf1, buf2;
   string rank, outbuf, stats, clan_name;
   int pcount = 0, amount, xx = 0, yy = 0;

   if( !( webwho = fopen( WEBWHO_FILE, "w" ) ) )
   {
      bug( "%s: Unable to open webwho file for writing!", __func__ );
      return;
   }

   buf1 << "&R-=[ &WPlayers on " << sysdata->mud_name << "&R]=-&d";
   webbuf << color_align( buf1.str(  ), 80, ALIGN_CENTER ) << "\n";

   buf2 << "&Y-=[&d &Wtelnet://" << sysdata->telnet << ":" << mud_port << "&d &Y]=-&d";
   amount = 78 - color_strlen( buf2.str(  ) );  /* Determine amount to put in front of line */

   if( amount < 1 )
      amount = 1;

   amount = amount / 2;

   for( xx = 0; xx < amount; ++xx )
      webbuf << " ";

   webbuf << buf2.str() << "\n";

   xx = 0;
   for( ds = dlist.begin(  ); ds != dlist.end(  ); ++ds )
   {
      descriptor_data *d = *ds;

      if( !d )
      {
         bug( "%s: nullptr DESCRIPTOR in list!", __func__ );
         continue;
      }

      char_data *person = d->original ? d->original : d->character;
      if( person && d->connected >= CON_PLAYING )
      {
         if( person->level >= LEVEL_IMMORTAL )
            continue;

         if( xx == 0 )
            webbuf << "\n&B--------------------------------=[&d &WPlayers&d &B]=---------------------------------&d\n\n";

         ++pcount;

         rank = rankbuffer( person );
         outbuf = color_align( rank, 20, ALIGN_CENTER );

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

      if( !d )
      {
         bug( "%s: nullptr DESCRIPTOR in list!", __func__ );
         continue;
      }

      char_data *person = d->original ? d->original : d->character;
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
         outbuf = color_align( rank, 20, ALIGN_CENTER );

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
   webbuf << "[&d&WHomepage: <a href=\"" << sysdata->http << "\">" << sysdata->http << "</a>&d&Y] [&d&W" << sysdata->maxplayers << " Max Since Reboot&d&Y]&d\n";
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
      bug( "%s: Unable to open arealist file for writing!", __func__ );
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

/*
 * Wizlist builder! - Thoric [Web version]
 */
void add_to_webwizlist( const string & name, const string & http, int level )
{
   auto wiz = std::make_unique<wizentweb>();

   wiz->name = name;
   if( !http.empty(  ) )
      wiz->http = http;
   wiz->level = level;

   wizlistweb.push_back( std::move( wiz ) );
}

std::string get_formatted_web_title( int level, bool is_first )
{
   int offset = MAX_LEVEL - level;
   std::string title;

   if( offset < 0 )
      offset = 17;

   if( !is_first )
   {
      title = "</div>\n<br />";
   }

   title += "<div style=\"text-align:center; font-weight:bold\">";

   switch( offset )
   {
      case 0:  title += "The Supreme Entity and Implementor"; break;
      case 1:  title += "The Realm Lords"; break;
      case 2:  title += "The Eternals"; break;
      case 3:  title += "The Ancients"; break;
      case 4:  title += "The Astral Gods"; break;
      case 5:  title += "The Elemental Gods"; break;
      case 6:  title += "The Dream Gods"; break;
      case 7:  title += "The Greater Gods"; break;
      case 8:  title += "The Gods"; break;
      case 9:  title += "The Demi Gods"; break;
      case 10: title += "The Deities"; break;
      case 11: title += "The Saviors"; break;
      case 12: title += "The Creators"; break;
      case 13: title += "The Acolytes"; break;
      case 14: title += "The Angels"; break;
      case 15: title += "Retired"; break;
      case 16: title += "Guests"; break;
      default: title += "Servants"; break;
   }

   title += "</div>\n<br /><div style=\"text-align:center\">";
   return title;
}

void make_webwiz( )
{
   // Always start with an empty list.
   wizlistweb.clear();

   // Walk the file list in GOD_DIR.
   for( const auto& entry : fs::directory_iterator( GOD_DIR ) )
   {
      // An actual file entry and not another folder.
      if( entry.is_regular_file( ) )
      {
         std::ifstream file( entry.path( ) );
         std::string word, http;
         int ilevel = 0;

         http.clear();
         while( file >> word )
         {
            if( word == "Level" )
            {
               file >> ilevel;
            }
            if( word == "Homepage" )
            {
               file >> http;
            }
         }
         add_to_webwizlist( entry.path( ).filename( ).string( ), http, ilevel );
      }
   }

   // Sort the list by level.
   wizlistweb.sort( []( const std::unique_ptr<wizentweb>& a, const std::unique_ptr<wizentweb>& b ) { return a->level > b->level; } );

   // Open WEBWIZ_FILE file for writing.
   std::ofstream out( WEBWIZ_FILE, std::ios::trunc );

   // Center the top banner with the MUD's name.
   out << std::format( "{:^78}\n", std::format( "The Immortal Masters of {}", sysdata->mud_name ) );

   int current_level = -1;
   std::string line_buffer;
   bool is_first = true;

   // Add each of the entries to the output file.
   for( const auto& wiz : wizlistweb )
   {
      if( wiz->level != current_level )
      {
         if( !line_buffer.empty() )
            out << std::format( "{:^78}\n ", line_buffer );

         out << std::format( "\n{:^78}\n ", get_formatted_web_title( wiz->level, is_first ) );
         line_buffer.clear();
         current_level = wiz->level;
         is_first = false;
      }

      std::string entry = wiz->http.empty()
         ? std::format( "<span style=\"font-size:14px;\">{}</span>", wiz->name )
         : std::format( "<a href=\"{}\" target=\"_blank\">{}</a>", wiz->http, wiz->name );

      if( line_buffer.length() + wiz->name.length() > 70 )
      {
         out << std::format( "{:^78}\n", line_buffer );
         line_buffer.clear();
      }

      line_buffer += ( line_buffer.empty() ? "" : " " ) + entry;
   }

   if( !line_buffer.empty() )
      out << std::format( "{:^78}\n", line_buffer );

   out << "</div>\n";

   // File stream will close automatically when function goes out of scope.
}

/* Aurora's room-to-web toy - this could be quite fun to mess with */
void room_to_html( room_index * room, bool complete )
{
   FILE *fp = nullptr;
   char filename[256];
   bool found = false;

   if( !room )
      return;

   snprintf( filename, 256, "%s%d.html", WEB_ROOMS, room->vnum );

   if( ( fp = fopen( filename, "w" ) ) != nullptr )
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
      bug( "%s: Error Opening room to html index stream!", __func__ );
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
