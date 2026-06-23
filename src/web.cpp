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
#include <fstream>
#include <memory>
#include <sstream>
#include <unordered_map>
#include "mud.h"
#include "area.h"
#include "clans.h"
#include "descriptor.h"
#include "roomindex.h"
#include "web.h"

std::string rankbuffer( char_data * );
extern int num_logins;

std::list<std::unique_ptr<wizentweb>> wizlistweb;

void free_wizlist_web_data()
{
   wizlistweb.clear();
}

std::string web_colourconv( std::string_view txt )
{
   std::string result;
   result.reserve( txt.size() * 2 );
   bool in_span = false;

   static const std::unordered_map<char, std::string_view> color_map = {
      {'x', "black"}, {'b', "dblue"},  {'c', "cyan"},   {'g', "dgreen"}, {'r', "dred"},
      {'w', "grey"},  {'y', "yellow"}, {'Y', "yellow"}, {'B', "blue"},   {'C', "lblue"},
      {'G', "green"}, {'R', "red"},    {'W', "white"},  {'z', "dgrey"},  {'o', "yellow"},
      {'O', "orange"},{'p', "purple"}, {'P', "pink"}
   };

   for( size_t i = 0; i < txt.size(); ++i )
   {
      if( txt[i] == '&' && ( i + 1 < txt.size() ) )
      {
         char code = txt[++i];

         if( code == '&' )
            result += "&amp;";
         else if( code == '<' )
            result += "&lt;";
         else if( code == '>' )
            result += "&gt;";
         else if( code == '/' )
            result += "<br />";
         else if( code == '{' )
            result += '{';
         else if( code == '-' )
            result += '~';

         else if( auto it = color_map.find( code ); it != color_map.end() )
         {
            if( in_span )
            {
               result += "</span>";
            }
            result += "<span class=\"";
            result += it->second;
            result += "\">";
            in_span = true;
         }
      }
      else
      {
         result += txt[i];
      }
   }

   if( in_span )
   {
      result += "</span>";
   }

   return result;
}

void web_who(  )
{
   FILE *webwho = nullptr;
   std::list<descriptor_data *>::iterator ds;
   std::ostringstream webbuf, buf1, buf2;
   std::string rank, outbuf, stats, clan_name;
   int pcount = 0, amount, xx = 0, yy = 0;

   std::filesystem::path filename = std::format( "{}", WEBWHO_FILE );
   if( !( webwho = fopen( filename.c_str(), "w" ) ) )
   {
      bug( "{}: Unable to open webwho file for writing!", __func__ );
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
         bug( "{}: nullptr DESCRIPTOR in list!", __func__ );
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
         bug( "{}: nullptr DESCRIPTOR in list!", __func__ );
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

   webbuf << "\n&Y[&d&W" << pcount << " Player" << ( pcount == 1 ? "" : "s" ) << "&d&Y] ";
   webbuf << "[&d&WHomepage: <a href=\"" << sysdata->http << "\">" << sysdata->http << "</a>&d&Y] [&d&W" << sysdata->maxplayers << " Max Since Reboot&d&Y]&d\n";
   webbuf << "&Y[&d&W" << num_logins << " login" << ( num_logins == 1 ? "" : "s" ) << " since last reboot on " << str_boot_time << "&d&Y]&d\n";

   std::string final_output = web_colourconv( webbuf.str() );
   fprintf( webwho, "%s", final_output.c_str() );
   FCLOSE( webwho );
}

void web_arealist(  )
{
   FILE *fp;
   const char *print_string =
      "<tr><td><font color=\"red\">%s   </font></td><td><font color=\"yellow\">%s</font></td><td><font color=\"green\">%d - %d   </font></td><td><font color=\"blue\">%d - %d</font></td></tr>\n";

   if( !( fp = fopen( AREALIST_FILE.data(), "w" ) ) )
   {
      bug( "{}: Unable to open arealist file for writing!", __func__ );
      return;
   }

   fprintf( fp,
            "<table><tr><td><font color=\"red\">Author   </font></td><td><font color=\"yellow\">Area</font></td><td><font color=\"green\">Recommened   </font></td><td><font color=\"blue\">Enforced</font></td></tr>\n" );

   for( auto* area : area_nsort )
   {
      if( !area->flags.test( AFLAG_PROTOTYPE ) )
         fprintf( fp, print_string, area->author, area->name, area->low_soft_range, area->hi_soft_range, area->low_hard_range, area->hi_hard_range );
   }
   fprintf( fp, "%s", "</table>\n" );
   FCLOSE( fp );
}

/*
 * Wizlist builder! - Thoric [Web version]
 */
void add_to_webwizlist( std::string_view name, std::string_view http, int level )
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
   for( const auto& entry : std::filesystem::directory_iterator( GOD_DIR ) )
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
   std::ofstream out( std::filesystem::path{WEBWIZ_FILE}, std::ios::trunc );

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
   bool found = false;

   if( !room )
      return;

   std::filesystem::path filename = std::format( "{}{}.html", WEB_ROOMS, room->vnum );

   if( ( fp = fopen( filename.c_str(), "w" ) ) != nullptr )
   {
      fprintf( fp, "%s", "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\">\n" );
      fprintf( fp, "<html>\n<head>\n<meta http-equiv=\"Content-Type\" content=\"text/html; charset=iso-8859-1\">\n" );
      fprintf( fp, "<title>%s: %s</title>\n</head>\n\n<body bgcolor=\"#000000\">\n", room->area->name.c_str(), room->name.c_str() );
      fprintf( fp, "%s", "<font face=\"Fixedsys\" size=\"3\">\n" );
      fprintf( fp, "<font color=\"#FF0000\">%s</font><br />\n", room->name.c_str() );
      fprintf( fp, "%s", "<font color=\"#33FF33\">[Exits:" );

      for( auto* pexit : room->exits )
      {
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
      std::string room_desc = web_colourconv( room->roomdesc );
      fprintf( fp, "<font color=\"#999999\">%s</font><br />\n", room_desc.c_str() );

      if( complete )
      {
         for( auto* obj : room->objects )
         {
            if( obj->extra_flags.test( ITEM_AUCTION ) )
               continue;

            if( !obj->objdesc.empty() )
               fprintf( fp, "<font color=\"#0000EE\">%s</font><br />\n", obj->objdesc.c_str() );
         }

         for( auto* rch : room->people )
         {
            if( rch->isnpc(  ) )
               fprintf( fp, "<font color=\"#FF00FF\">%s</font><br />\n", rch->long_descr.c_str() );
            else
            {
               std::string pctitle = web_colourconv( rch->pcdata->title );
               fprintf( fp, "<font color=\"#FF00FF\">%s %s</font><br />\n", rch->name.c_str(), pctitle.c_str() );
            }
         }
      }
      fprintf( fp, "%s", "</font>\n</body>\n</html>\n" );
      FCLOSE( fp );
   }
   else
      bug( "{}: Error Opening room to html index stream!", __func__ );
}

CMDF( do_webroom )
{
   area_data *area;
   bool complete = false;

   if( !str_cmp( argument, "complete" ) )
      complete = true;

   // Limiting this to Bywater since I don't want the whole mud "webized" - silent return
   if( !( area = find_area( "bywater.are" ) ) )
      return;

   for( auto* room : area->rooms )
   {
      if( room->flags.test( ROOM_AUCTION ) || room->flags.test( ROOM_PET_SHOP ) )
         continue;

      room_to_html( room, complete );
   }
   ch->print( "Bywater rooms dumped to web.\r\n" );
}
