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
 *                          MSSP Plaintext Module                           *
 ****************************************************************************/

/******************************************************************
* Program writen by:                                              *
*  Greg (Keberus Maou'San) Mosley                                 *
*  Co-Owner/Coder SW: TGA                                         *
*  www.t-n-k-games.com                                            *
*                                                                 *
* Description:                                                    *
*  This program will allow admin to view and set their MSSP       *
*  variables in game, and allows thier game to respond to a MSSP  *
*  Server with the MSSP-Plaintext protocol                        *
*******************************************************************
* What it does:                                                   *
*  Allows admin to set/view MSSP variables and transmits the MSSP *
*  information to anyone who does an MSSP-REQUEST at the login    *
*  screen                                                         *
*******************************************************************
* Special Thanks:                                                 *
*  A special thanks to Scandum for coming up with the MSSP        *
*  protocol, Cratylus for the MSSP-Plaintext idea, and Elanthis   *
*  for the GNUC_FORMAT idea ( which I like to use now ).          *
******************************************************************/

/*TERMS OF USE
         I only really have 2 terms...
 1. Give credit where it is due, keep the above header in your code 
    (you don't have to give me credit in mud) and if someone asks 
	don't lie and say you did it.
 2. If you have any comments or questions feel free to email me
    at keberus@gmail.com

  Thats All....
 */
#include <filesystem>
#include <fstream>
#include "mud.h"
#include "descriptor.h"
#include "mssp.h"

struct msspinfo *mssp_info;

void free_mssp_info( void )
{
   deleteptr( mssp_info );
}

msspinfo::msspinfo()
{
   this->hostname = "localhost";
   this->ip = "127.0.0.1";
   this->ipv6 = "::1";
   this->contact = "admin@example.com";
   this->language = "English";
   this->location = "USA";
   this->family = "DikuMUD";
   this->genre = "Fantasy";
   this->subgenre = "Medieval Fantasy";
   this->gamePlay = "Adventure, Hack and Slash";
   this->gameSystem = "D&D";
   this->status = "Closed";
   this->created = 2009;
   this->ansi = true;
   this->mccp = true;
   this->msp = true;
}

void save_mssp_info( void )
{
   std::ofstream stream( std::filesystem::path{MSSP_FILE} );

   if( !stream.is_open( ) )
   {
      bug( "%s: Cannot open MSSP file.", __func__ );
      return;
   }

   // For writing string values if they are actually set.
   auto write_field = [&stream](const std::string& label, const std::string& value) {
      if( !value.empty() )
      {
         stream << label << value << std::endl;
      }
   };

   // For writing numerical values if they're > 0.
   auto write_field_int = [&stream](const std::string& label, int value) {
      if( value > 0 )
      {
         stream << label << value << std::endl;
      }
   };

   stream << "#MSSP_INFO" << std::endl;

   write_field( "Hostname          ", mssp_info->hostname );
   write_field( "IP                ", mssp_info->ip );
   write_field( "IPv6              ", mssp_info->ipv6 );
   write_field( "Contact           ", mssp_info->contact );
   write_field( "Icon              ", mssp_info->icon );
   write_field( "Language          ", mssp_info->language );
   write_field( "Location          ", mssp_info->location );
   write_field( "Family            ", mssp_info->family );
   write_field( "Genre             ", mssp_info->genre );
   write_field( "GamePlay          ", mssp_info->gamePlay );
   write_field( "GameSystem        ", mssp_info->gameSystem );
   write_field( "Status            ", mssp_info->status );
   write_field( "SubGenre          ", mssp_info->subgenre );
   write_field( "Intermud          ", mssp_info->intermud );

   if( mssp_info->ssl >= 1024 )
      stream << "SSL               " << mssp_info->ssl;

   write_field_int( "CrawlDelay        ", mssp_info->crawldelay );
   write_field_int( "MinAge            ", mssp_info->minAge );
   write_field_int( "Ansi              ", mssp_info->ansi );
   write_field_int( "Created           ", mssp_info->created );
   write_field_int( "MCCP              ", mssp_info->mccp );
   write_field_int( "MCP               ", mssp_info->mcp );
   write_field_int( "MSP               ", mssp_info->msp );
   write_field_int( "MXP               ", mssp_info->mxp );
   write_field_int( "Vt100             ", mssp_info->vt100 );
   write_field_int( "Xterm256          ", mssp_info->xterm256 );
   write_field_int( "Pay2Play          ", mssp_info->pay2play );
   write_field_int( "Pay4Perks         ", mssp_info->pay4perks );
   write_field_int( "HiringBuilders    ", mssp_info->hiringBuilders );
   write_field_int( "HiringCoders      ", mssp_info->hiringCoders );

   stream << "End" << std::endl << std::endl;
}

/*
 * Load the MSSP data file
 */
void load_mssp_data( void )
{
   std::ifstream stream;

   stream.open( std::filesystem::path( MSSP_FILE ) );
   if( !stream.is_open(  ) )
   {
      log_string( "No MSSP data file found. Generating default data." );
      mssp_info = new msspinfo;
      save_mssp_info();
      return;
   }

   do
   {
      std::string key, value;
      char buf[MSL];

      stream >> key;
      strip_lspace( key );

      if( key.empty(  ) )
         continue;

      if( key == "#MSSP_INFO" )
         mssp_info = new msspinfo;
      else if( key == "Ansi" )
      {
         stream.getline( buf, MSL );
         value = buf;
         strip_lspace( value );

         mssp_info->ansi = std::stoi( value );
      }
      else if( key == "Contact" )
      {
         stream.getline( buf, MSL );
         value = buf;
         strip_lspace( value );

         mssp_info->contact = value;
      }
      else if( key == "CrawlDelay" )
      {
         stream.getline( buf, MSL );
         value = buf;
         strip_lspace( value );

         mssp_info->crawldelay = std::stoi( value );
      }
      else if( key == "Created" )
      {
         stream.getline( buf, MSL );
         value = buf;
         strip_lspace( value );

         mssp_info->created = std::stoi( value );
      }
      else if( key == "Family" )
      {
         stream.getline( buf, MSL );
         value = buf;
         strip_lspace( value );

         mssp_info->family = value;
      }
      else if( key == "Genre" )
      {
         stream.getline( buf, MSL );
         value = buf;
         strip_lspace( value );

         mssp_info->genre = value;
      }
      else if( key == "GamePlay" )
      {
         stream.getline( buf, MSL );
         value = buf;
         strip_lspace( value );

         mssp_info->gamePlay = value;
      }
      else if( key == "GameSystem" )
      {
         stream.getline( buf, MSL );
         value = buf;
         strip_lspace( value );

         mssp_info->gameSystem = value;
      }
      else if( key == "Intermud" )
      {
         stream.getline( buf, MSL );
         value = buf;
         strip_lspace( value );

         mssp_info->intermud = value;
      }
      else if( key == "Hostname" )
      {
         stream.getline( buf, MSL );
         value = buf;
         strip_lspace( value );

         mssp_info->hostname = value;
      }
      else if( key == "HiringBuilders" )
      {
         stream.getline( buf, MSL );
         value = buf;
         strip_lspace( value );

         mssp_info->hiringBuilders = std::stoi( value );
      }
      else if( key == "HiringCoders" )
      {
         stream.getline( buf, MSL );
         value = buf;
         strip_lspace( value );

         mssp_info->hiringCoders = std::stoi( value );
      }
      else if( key == "Icon" )
      {
         stream.getline( buf, MSL );
         value = buf;
         strip_lspace( value );

         mssp_info->icon = value;
      }
      else if( key == "Language" )
      {
         stream.getline( buf, MSL );
         value = buf;
         strip_lspace( value );

         mssp_info->language = value;
      }
      else if( key == "Location" )
      {
         stream.getline( buf, MSL );
         value = buf;
         strip_lspace( value );

         mssp_info->location = value;
      }
      else if( key == "IP" )
      {
         stream.getline( buf, MSL );
         value = buf;
         strip_lspace( value );

         mssp_info->ip = value;
      }
      else if( key == "IPv6" )
      {
         stream.getline( buf, MSL );
         value = buf;
         strip_lspace( value );

         mssp_info->ip = value;
      }
      else if( key == "MCCP" )
      {
         stream.getline( buf, MSL );
         value = buf;
         strip_lspace( value );

         mssp_info->mccp = std::stoi( value );
      }
      else if( key == "MCP" )
      {
         stream.getline( buf, MSL );
         value = buf;
         strip_lspace( value );

         mssp_info->mcp = std::stoi( value );
      }
      else if( key == "MinAge" )
      {
         stream.getline( buf, MSL );
         value = buf;
         strip_lspace( value );

         mssp_info->minAge = std::stoi( value );
      }
      else if( key == "MSP" )
      {
         stream.getline( buf, MSL );
         value = buf;
         strip_lspace( value );

         mssp_info->msp = std::stoi( value );
      }
      else if( key == "MXP" )
      {
         stream.getline( buf, MSL );
         value = buf;
         strip_lspace( value );

         mssp_info->mxp = std::stoi( value );
      }
      else if( key == "Pay2Play" )
      {
         stream.getline( buf, MSL );
         value = buf;
         strip_lspace( value );

         mssp_info->pay2play = std::stoi( value );
      }
      else if( key == "Pay4Perks" )
      {
         stream.getline( buf, MSL );
         value = buf;
         strip_lspace( value );

         mssp_info->pay4perks = std::stoi( value );
      }
      else if( key == "SSL" )
      {
         stream.getline( buf, MSL );
         value = buf;
         strip_lspace( value );

         mssp_info->ssl = std::stoi( value );
      }
      else if( key == "Status" )
      {
         stream.getline( buf, MSL );
         value = buf;
         strip_lspace( value );

         mssp_info->status = value;
      }
      else if( key == "SubGenre" )
      {
         stream.getline( buf, MSL );
         value = buf;
         strip_lspace( value );

         mssp_info->subgenre = value;
      }
      else if( key == "Vt100" )
      {
         stream.getline( buf, MSL );
         value = buf;
         strip_lspace( value );

         mssp_info->vt100 = std::stoi( value );
      }
      else if( key == "Xterm256" )
      {
         stream.getline( buf, MSL );
         value = buf;
         strip_lspace( value );

         mssp_info->xterm256 = std::stoi( value );
      }
      else if( key == "End" )
         break;
      else
      {
         stream.getline( buf, MSL );
         value = buf;
         strip_lspace( value );
         log_printf( "Bad line in MSSP data file: {} {}", key, value );
      }
   }
   while( !stream.eof(  ) );
   stream.close(  );
}

std::string MSSP_YN( int value )
{
   return( value == 0 ? "No" : "Yes" );
}

void show_mssp( char_data * ch )
{
   if( !ch )
   {
      bug( "%s: nullptr ch", __func__ );
      return;
   }

   ch->print_fmt( "&zHostname          &W{}\r\n", mssp_info->hostname );
   ch->print_fmt( "&zIP                &W{}\r\n", mssp_info->ip );
   ch->print_fmt( "&zIPv6              &W{}\r\n", mssp_info->ip );
   ch->print_fmt( "&zContact           &W{}\r\n", mssp_info->contact );
   ch->print_fmt( "&zIcon              &W{}\r\n", mssp_info->icon );
   ch->print_fmt( "&zLanguage          &W{}\r\n", mssp_info->language );
   ch->print_fmt( "&zLocation          &W{}\r\n", mssp_info->location );
   ch->print_fmt( "&zWebsite           &W{}\r\n", show_tilde( sysdata->http ) );
   ch->print_fmt( "&zFamily            &W{}\r\n", mssp_info->family );
   ch->print_fmt( "&zGenre             &W{}\r\n", mssp_info->genre );
   ch->print_fmt( "&zGamePlay          &W{}\r\n", mssp_info->gamePlay );
   ch->print_fmt( "&zGameSystem        &W" );
   ch->desc->buffer_printf( "{}\r\n", mssp_info->gameSystem ); // Because D&D is a valid option and color tags mess this up.
   ch->print_fmt( "&zIntermud          &W{}\r\n", mssp_info->intermud );
   ch->print_fmt( "&zStatus            &W{}\r\n", mssp_info->status );
   ch->print_fmt( "&zSubGenre          &W{}\r\n", mssp_info->subgenre );
   ch->print_fmt( "&zCreated           &W{}\r\n", mssp_info->created );
   ch->print_fmt( "&zMinAge            &W{}\r\n", mssp_info->minAge );
   ch->print_fmt( "&zAnsi              &W{}\r\n", MSSP_YN( mssp_info->ansi ) );
   ch->print_fmt( "&zMCCP              &W{}\r\n", MSSP_YN( mssp_info->mccp ) );
   ch->print_fmt( "&zMCP               &W{}\r\n", MSSP_YN( mssp_info->mcp ) );
   ch->print_fmt( "&zMSP               &W{}\r\n", MSSP_YN( mssp_info->msp ) );
   ch->print_fmt( "&zSSL               &W{}\r\n", MSSP_YN( mssp_info->ssl ) );
   ch->print_fmt( "&zMXP               &W{}\r\n", MSSP_YN( mssp_info->mxp ) );
   ch->print_fmt( "&zVt100             &W{}\r\n", MSSP_YN( mssp_info->vt100 ) );
   ch->print_fmt( "&zXterm256          &W{}\r\n", MSSP_YN( mssp_info->xterm256 ) );
   ch->print_fmt( "&zPay2Play          &W{}\r\n", MSSP_YN( mssp_info->pay2play ) );
   ch->print_fmt( "&zPay4Perks         &W{}\r\n", MSSP_YN( mssp_info->pay4perks ) );
   ch->print_fmt( "&zHiringBuilders    &W{}\r\n", MSSP_YN( mssp_info->hiringBuilders ) );
   ch->print_fmt( "&zHiringCoders      &W{}\r\n", MSSP_YN( mssp_info->hiringCoders ) );
}

CMDF( do_setmssp )
{
   std::string arg1;
   std::string *strptr = nullptr;
   bool *ynptr = nullptr;

   if( argument.empty() )
   {
      ch->print( "Syntax: setmssp show\r\n" );
      ch->print( "Syntax: setmssp <field> [value]\r\n" );
      ch->print( "Field being one of:\r\n" );
      ch->print( "hostname       ip                ipv6               contact          icon\r\n" );
      ch->print( "language       location          crawldelay\r\n" );
      ch->print( "website        family            genre              gameplay         game_system\r\n" );
      ch->print( "status         subgenre          intermud           created          min_age\r\n" );
      ch->print( "worlds         ansi              mccp               mcp              msp\r\n" );
      ch->print( "ssl            mxp               vt100              xterm256\r\n" );
      ch->print( "pay2play       pay4perks         hiring_builders    hiring_coders\r\n" );

      return;
   }

   argument = one_argument( argument, arg1 );

   if( !str_cmp( arg1, "show" ) ) // Here you go Conner :)
   {
      show_mssp( ch );
      return;
   }

   if( !str_cmp( arg1, "hostname" ) )
      strptr = &mssp_info->hostname;
   else if( !str_cmp( arg1, "ip" ) )
      strptr = &mssp_info->ip;
   else if( !str_cmp( arg1, "ipv6" ) )
      strptr = &mssp_info->ipv6;
   else if( !str_cmp( arg1, "contact" ) )
      strptr = &mssp_info->contact;
   else if( !str_cmp( arg1, "icon" ) )
      strptr = &mssp_info->icon;
   else if( !str_cmp( arg1, "language" ) )
      strptr = &mssp_info->language;
   else if( !str_cmp( arg1, "location" ) )
      strptr = &mssp_info->location;
   else if( !str_cmp( arg1, "family" ) )
      strptr = &mssp_info->family;
   else if( !str_cmp( arg1, "genre" ) )
      strptr = &mssp_info->genre;
   else if( !str_cmp( arg1, "gameplay" ) )
      strptr = &mssp_info->gamePlay;
   else if( !str_cmp( arg1, "game_system" ) )
      strptr = &mssp_info->gameSystem;
   else if( !str_cmp( arg1, "intermud" ) )
      strptr = &mssp_info->intermud;
   else if( !str_cmp( arg1, "status" ) )
      strptr = &mssp_info->status;
   else if( !str_cmp( arg1, "subgenre" ) )
      strptr = &mssp_info->subgenre;

   if( strptr != nullptr )
   {
      *strptr = argument;
      ch->print_fmt( "MSSP value, {} has been changed to: {}\r\n", arg1, argument );
      save_mssp_info(  );
      return;
   }
   if( !str_cmp( arg1, "ansi" ) )
      ynptr = &mssp_info->ansi;
   else if( !str_cmp( arg1, "mccp" ) )
      ynptr = &mssp_info->mccp;
   else if( !str_cmp( arg1, "msp" ) )
      ynptr = &mssp_info->msp;
/* Uncomment these only if you have added support to the codebase.
   else if( !str_cmp( arg1, "mcp" ) )
      ynptr = &mssp_info->mcp;
   else if( !str_cmp( arg1, "mxp" ) )
      ynptr = &mssp_info->mxp;
   else if( !str_cmp( arg1, "vt100" ) )
      ynptr = &mssp_info->vt100;
*/
   else if( !str_cmp( arg1, "xterm256" ) )
      ynptr = &mssp_info->xterm256;
   else if( !str_cmp( arg1, "pay2play" ) )
      ynptr = &mssp_info->pay2play;
   else if( !str_cmp( arg1, "pay4perks" ) )
      ynptr = &mssp_info->pay4perks;
   else if( !str_cmp( arg1, "hiring_builders" ) )
      ynptr = &mssp_info->hiringBuilders;
   else if( !str_cmp( arg1, "hiring_coders" ) )
      ynptr = &mssp_info->hiringCoders;

   if( ynptr != nullptr )
   {
      bool newvalue = false;

      if( str_cmp( argument, "yes" ) && str_cmp( argument, "no" ) )
      {
         ch->print_fmt( "You must specify 'yes' or 'no' for the {} value!\r\n", arg1 );
         return;
      }
      newvalue = !str_cmp( argument, "yes" ) ? true : false;
      *ynptr = newvalue;
      ch->printf( "MSSP value, %s has been changed to: %s\r\n", arg1.c_str(), argument.c_str() );
      save_mssp_info(  );
      return;
   }

   if( !str_cmp( arg1, "created" ) )
   {
      int value;

      value = atoi( argument.c_str() );

      if( !is_number( argument ) || ( value < MSSP_MINCREATED ) || ( value > MSSP_MAXCREATED ) )
      {
         ch->print_fmt( "The value for created must be between {} and {}.%d\r\n", MSSP_MINCREATED, MSSP_MAXCREATED );
         return;
      }
      mssp_info->created = value;
      ch->print_fmt( "MSSP value, {} has been changed to: {}\r\n", arg1, argument );
      save_mssp_info(  );
      return;
   }
   else if( !str_cmp( arg1, "min_age" ) )
   {
      int value = std::stoi( argument );

      if( !is_number( argument ) || ( value < MSSP_MINAGE ) || ( value > MSSP_MAXAGE ) )
      {
         ch->print_fmt( "The value for min_age must be between {} and {}.\r\n", MSSP_MINAGE, MSSP_MAXAGE );
         return;
      }
      mssp_info->minAge = value;
      ch->print_fmt( "MSSP value, {} has been changed to: {}\r\n", arg1, argument );
      save_mssp_info(  );
      return;
   }
   else if( !str_cmp( arg1, "ssl" ) )
   {
      int value = std::stoi( argument );

      if( !is_number( argument ) || ( value < 1024 ) || ( value > 65535 ) )
      {
         ch->print( "The value for ssl must be between 1024 and 65535.\r\n" );
         return;
      }
      mssp_info->ssl = value;
      ch->print_fmt( "MSSP value, {} has been changed to: {}\r\n", arg1, argument );
      save_mssp_info(  );
      return;
   }
   else if( !str_cmp( arg1, "crawldelay" ) )
   {
      int value = std::stoi( argument );

      if( !is_number( argument ) || value < -1 )
      {
         ch->print( "The value for crawldelay must be a numeric value >= 1, or -1 to use the protocol default.\r\n" );
         return;
      }
      mssp_info->crawldelay = value;
      ch->print_fmt( "MSSP value, {} has been changed to: {}\r\n", arg1, argument );
      save_mssp_info(  );
      return;
   }
   else
      do_setmssp( ch, "" );
}

void write_to_descriptor_printf( descriptor_data * desc, std::string_view fmt, auto&&... args )
{
   std::string buf;

   try
   {
      buf = std::vformat( fmt, std::make_format_args( args... ) );
   }
   catch( const std::exception & e )
   {
      // In case someone bodged a call to this that isn't formatted right.
      bug( "%s: Formatting error: %s", __func__, e.what() );
      return;
   }

   desc->write( buf );
}

void mssp_reply( descriptor_data * d, const char *var, std::string_view fmt, auto&&... args )
{
   std::string buf;

   if( !d )
   {
      bug( "%s: nullptr d", __func__ );
      return;
   }

   if( !var || var[0] == '\0' )
   {
      bug( "%s: nullptr var", __func__ );
      return;
   }

   try
   {
      buf = std::vformat( fmt, std::make_format_args( args... ) );
   }
   catch( const std::exception & e )
   {
      // In case someone bodged a call to this that isn't formatted right.
      bug( "%s: Formatting error: %s", __func__, e.what() );
      return;
   }

   write_to_descriptor_printf( d, "{}\t{}\r\n", var, buf );
}

short player_count( void )
{
   short count = 0;

   for( auto* d : dlist )
   {
      if( d->connected >= CON_PLAYING )
         ++count;
   }
   return count;
}

#if defined(SQL)
 int db_help_count();
#endif
extern std::chrono::system_clock::time_point mud_start_time;
extern int mud_port;
extern int top_prog;
extern int top_help;
extern int top_reset;
extern int top_exit;

void send_mssp_data( descriptor_data * d )
{
   if( !d )
   {
      bug( "%s: nullptr d", __func__ );
      return;
   }

   d->write( "\r\nMSSP-REPLY-START\r\n" );

   mssp_reply( d, "NAME", "{}", sysdata->mud_name );
   mssp_reply( d, "WEBSITE", "{}", show_tilde( sysdata->http ) );
   mssp_reply( d, "HOSTNAME", "{}", mssp_info->hostname );
   mssp_reply( d, "IP", "{}", mssp_info->ip );

   if( !mssp_info->ipv6.empty() )
      mssp_reply( d, "IPV6", "{}", mssp_info->ipv6 );

   mssp_reply( d, "PORT", "{}", mud_port );
   mssp_reply( d, "CRAWL DELAY", "{}", mssp_info->crawldelay );
   mssp_reply( d, "CONTACT", "{}", mssp_info->contact );
   mssp_reply( d, "CREATED", "{}", mssp_info->created );
   mssp_reply( d, "CODEBASE", "{} {}", CODENAME, CODEVERSION );
   mssp_reply( d, "PLAYERS", "{}", player_count( ) );
   mssp_reply( d, "UPTIME", "{}", mud_start_time );

   if( !mssp_info->icon.empty() )
      mssp_reply( d, "ICON", "{}", mssp_info->icon );

   mssp_reply( d, "LANGUAGE", "{}", mssp_info->language );
   mssp_reply( d, "LOCATION", "{}", mssp_info->location );

   if( mssp_info->minAge > 0 )
      mssp_reply( d, "MINIMUM AGE", "{}", mssp_info->minAge );

   mssp_reply( d, "FAMILY", "{}", mssp_info->family );
   mssp_reply( d, "GENRE", "{}", mssp_info->genre );
   mssp_reply( d, "SUBGENRE", "{}", mssp_info->subgenre );
   mssp_reply( d, "GAMEPLAY", "{}", mssp_info->gamePlay );
   mssp_reply( d, "GAMESYSTEM", "{}", mssp_info->gameSystem );

   mssp_reply( d, "INTERMUD", "{}", mssp_info->intermud );

   mssp_reply( d, "STATUS", "{}", mssp_info->status );
   mssp_reply( d, "AREAS", "{}", top_area );
   mssp_reply( d, "ROOMS", "{}", top_room );
   mssp_reply( d, "MOBILES", "{}", top_mob_index );
   mssp_reply( d, "OBJECTS", "{}", top_obj_index );
#if defined(SQL)
   mssp_reply( d, "HELPFILES", "{}", db_help_count() );
#else
   mssp_reply( d, "HELPFILES", "{}", top_help );
#endif
   mssp_reply( d, "LEVELS", "{}", LEVEL_AVATAR );
   mssp_reply( d, "RACES", "{}", MAX_PC_RACE );
   mssp_reply( d, "CLASSES", "{}", MAX_PC_CLASS );
   mssp_reply( d, "SKILLS", "{}", num_skills );
   mssp_reply( d, "ANSI", "{}", mssp_info->ansi );
   mssp_reply( d, "MCCP", "{}", mssp_info->mccp );
   mssp_reply( d, "MCP", "{}", 0 ); // Hardcoded 0 response because this codebase doesn't support MCP.
   mssp_reply( d, "MSP", "{}", mssp_info->msp );

   if( mssp_info->ssl > 1024 )
      mssp_reply( d, "SSL", "{}", mssp_info->ssl );

   mssp_reply( d, "MXP", "{}", 0 ); // Hardcoded 0 response because this codebase doesn't support MXP. Or VT100 either.
   mssp_reply( d, "VT100", "{}", 0 );
   mssp_reply( d, "XTERM 256 COLORS", "{}", mssp_info->xterm256 );
   mssp_reply( d, "PAY TO PLAY", "{}", mssp_info->pay2play );
   mssp_reply( d, "PAY FOR PERKS", "{}", mssp_info->pay4perks );
   mssp_reply( d, "HIRING BUILDERS", "{}", mssp_info->hiringBuilders );
   mssp_reply( d, "HIRING CODERS", "{}", mssp_info->hiringCoders );

   d->write( "MSSP-REPLY-END\r\n" );
}
