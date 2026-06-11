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
#include <format>
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
   init_memory( &this->created, &this->playerGuilds, sizeof( this->playerGuilds ) );

   this->hostname = "localhost";
   this->ip = "127.0.0.1";
   this->contact = "admin@example.com";
   this->language = "English";
   this->location = "United States";
   this->family = "DikuMUD";
   this->genre = "Fantasy";
   this->gamePlay = "Hack and Slash";
   this->gameSystem = "D&D";
   this->status = "Closed";
   this->subgenre = "Medieval Fantasy";
   this->created = 2009;
   this->worlds = 1;
   this->ansi = true;
   this->mccp = true;
   this->msp = true;
   this->playerClans = true;
   this->playerCrafting = true;
   this->playerGuilds = true;
   this->equipmentSystem = "Both";
   this->playerKilling = "Restricted";
   this->trainingSystem = "Both";
   this->worldOriginality = "All Stock";
}

void save_mssp_info( void )
{
   std::ofstream stream;

   stream.open( std::filesystem::path( MSSP_FILE ) );
   if( !stream.is_open(  ) )
   {
      bug( "%s: Cannot open MSSP file.", __func__ );
      return;
   }

   stream << "#MSSP_INFO" << std::endl;
   stream << "Hostname          " << mssp_info->hostname << std::endl;
   stream << "IP                " << mssp_info->ip << std::endl;
   stream << "Contact           " << mssp_info->contact << std::endl;
   stream << "Icon              " << mssp_info->icon << std::endl;
   stream << "Language          " << mssp_info->language << std::endl;
   stream << "Location          " << mssp_info->location << std::endl;
   stream << "Family            " << mssp_info->family << std::endl;
   stream << "Genre             " << mssp_info->genre << std::endl;
   stream << "GamePlay          " << mssp_info->gamePlay << std::endl;
   stream << "GameSystem        " << mssp_info->gameSystem << std::endl;
   stream << "Intermud          " << mssp_info->intermud << std::endl;
   stream << "Status            " << mssp_info->status << std::endl;
   stream << "SubGenre          " << mssp_info->subgenre << std::endl;
   stream << "Created           " << mssp_info->created << std::endl;
   stream << "MinAge            " << mssp_info->minAge << std::endl;
   stream << "Worlds            " << mssp_info->worlds << std::endl;
   stream << "Ansi              " << mssp_info->ansi << std::endl;
   stream << "MCCP              " << mssp_info->mccp << std::endl;
   stream << "MCP               " << mssp_info->mcp << std::endl;
   stream << "MSP               " << mssp_info->msp << std::endl;
   stream << "SSL               " << mssp_info->ssl << std::endl;
   stream << "MXP               " << mssp_info->mxp << std::endl;
   stream << "Pueblo            " << mssp_info->pueblo << std::endl;
   stream << "Vt100             " << mssp_info->vt100 << std::endl;
   stream << "Xterm256          " << mssp_info->xterm256 << std::endl;
   stream << "Pay2Play          " << mssp_info->pay2play << std::endl;
   stream << "Pay4Perks         " << mssp_info->pay4perks << std::endl;
   stream << "HiringBuilders    " << mssp_info->hiringBuilders << std::endl;
   stream << "HiringCoders      " << mssp_info->hiringCoders << std::endl;
   stream << "AdultMaterial     " << mssp_info->adultMaterial << std::endl;
   stream << "Multiclassing     " << mssp_info->multiclassing << std::endl;
   stream << "NewbieFriendly    " << mssp_info->newbieFriendly << std::endl;
   stream << "PlayerCities      " << mssp_info->playerCities << std::endl;
   stream << "PlayerClans       " << mssp_info->playerClans << std::endl;
   stream << "PlayerCrafting    " << mssp_info->playerCrafting << std::endl;
   stream << "PlayerGuilds      " << mssp_info->playerGuilds << std::endl;
   stream << "EquipmentSystem   " << mssp_info->equipmentSystem << std::endl;
   stream << "Multiplaying      " << mssp_info->multiplaying << std::endl;
   stream << "PlayerKilling     " << mssp_info->playerKilling << std::endl;
   stream << "QuestSystem       " << mssp_info->questSystem << std::endl;
   stream << "RolePlaying       " << mssp_info->roleplaying << std::endl;
   stream << "TrainingSystem    " << mssp_info->trainingSystem << std::endl;
   stream << "WorldOriginality  " << mssp_info->worldOriginality << std::endl;
   stream << "End" << std::endl << std::endl;

   stream.close(  );
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
      log_printf( "No MSSP data file found. Generating default data." );
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
      else if( key == "AdultMaterial" )
      {
         stream.getline( buf, MSL );
         value = buf;
         strip_lspace( value );

         mssp_info->adultMaterial = atoi( value.c_str() );
      }
      else if( key == "Ansi" )
      {
         stream.getline( buf, MSL );
         value = buf;
         strip_lspace( value );

         mssp_info->ansi = atoi( value.c_str() );
      }
      else if( key == "Contact" )
      {
         stream.getline( buf, MSL );
         value = buf;
         strip_lspace( value );

         mssp_info->contact = value;
      }
      else if( key == "Created" )
      {
         stream.getline( buf, MSL );
         value = buf;
         strip_lspace( value );

         mssp_info->created = atoi( value.c_str() );
      }
      else if( key == "EquipmentSystem" )
      {
         stream.getline( buf, MSL );
         value = buf;
         strip_lspace( value );

         mssp_info->equipmentSystem = value;
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

         mssp_info->hiringBuilders = atoi( value.c_str() );
      }
      else if( key == "HiringCoders" )
      {
         stream.getline( buf, MSL );
         value = buf;
         strip_lspace( value );

         mssp_info->hiringCoders = atoi( value.c_str() );
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
      else if( key == "MCCP" )
      {
         stream.getline( buf, MSL );
         value = buf;
         strip_lspace( value );

         mssp_info->mccp = atoi( value.c_str() );
      }
      else if( key == "MCP" )
      {
         stream.getline( buf, MSL );
         value = buf;
         strip_lspace( value );

         mssp_info->mcp = atoi( value.c_str() );
      }
      else if( key == "MinAge" )
      {
         stream.getline( buf, MSL );
         value = buf;
         strip_lspace( value );

         mssp_info->minAge = atoi( value.c_str() );
      }
      else if( key == "MSP" )
      {
         stream.getline( buf, MSL );
         value = buf;
         strip_lspace( value );

         mssp_info->msp = atoi( value.c_str() );
      }
      else if( key == "Multiclassing" )
      {
         stream.getline( buf, MSL );
         value = buf;
         strip_lspace( value );

         mssp_info->multiclassing = atoi( value.c_str() );
      }
      else if( key == "Multiplaying" )
      {
         stream.getline( buf, MSL );
         value = buf;
         strip_lspace( value );

         mssp_info->multiplaying = atoi( value.c_str() );
      }
      else if( key == "MXP" )
      {
         stream.getline( buf, MSL );
         value = buf;
         strip_lspace( value );

         mssp_info->mxp = atoi( value.c_str() );
      }
      else if( key == "NewbieFriendly" )
      {
         stream.getline( buf, MSL );
         value = buf;
         strip_lspace( value );

         mssp_info->newbieFriendly = atoi( value.c_str() );
      }
      else if( key == "Pay2Play" )
      {
         stream.getline( buf, MSL );
         value = buf;
         strip_lspace( value );

         mssp_info->pay2play = atoi( value.c_str() );
      }
      else if( key == "Pay4Perks" )
      {
         stream.getline( buf, MSL );
         value = buf;
         strip_lspace( value );

         mssp_info->pay4perks = atoi( value.c_str() );
      }
      else if( key == "PlayerCities" )
      {
         stream.getline( buf, MSL );
         value = buf;
         strip_lspace( value );

         mssp_info->playerCities = atoi( value.c_str() );
      }
      else if( key == "PlayerClans" )
      {
         stream.getline( buf, MSL );
         value = buf;
         strip_lspace( value );

         mssp_info->playerClans = atoi( value.c_str() );
      }
      else if( key == "PlayerCrafting" )
      {
         stream.getline( buf, MSL );
         value = buf;
         strip_lspace( value );

         mssp_info->playerCrafting = atoi( value.c_str() );
      }
      else if( key == "PlayerGuilds" )
      {
         stream.getline( buf, MSL );
         value = buf;
         strip_lspace( value );

         mssp_info->playerGuilds = atoi( value.c_str() );
      }
      else if( key == "PlayerKilling" )
      {
         stream.getline( buf, MSL );
         value = buf;
         strip_lspace( value );

         mssp_info->playerKilling = value;
      }
      else if( key == "Pueblo" )
      {
         stream.getline( buf, MSL );
         value = buf;
         strip_lspace( value );

         mssp_info->pueblo = atoi( value.c_str() );
      }
      else if( key == "QuestSystem" )
      {
         stream.getline( buf, MSL );
         value = buf;
         strip_lspace( value );

         mssp_info->questSystem = value;
      }
      else if( key == "RolePlaying" )
      {
         stream.getline( buf, MSL );
         value = buf;
         strip_lspace( value );

         mssp_info->roleplaying = value;
      }
      else if( key == "SSL" )
      {
         stream.getline( buf, MSL );
         value = buf;
         strip_lspace( value );

         mssp_info->ssl = atoi( value.c_str() );
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
      else if( key == "TrainingSystem" )
      {
         stream.getline( buf, MSL );
         value = buf;
         strip_lspace( value );

         mssp_info->trainingSystem = value;
      }
      else if( key == "Vt100" )
      {
         stream.getline( buf, MSL );
         value = buf;
         strip_lspace( value );

         mssp_info->vt100 = atoi( value.c_str() );
      }
      else if( key == "WorldOriginality" )
      {
         stream.getline( buf, MSL );
         value = buf;
         strip_lspace( value );

         mssp_info->worldOriginality = value;
      }
      else if( key == "Worlds" )
      {
         stream.getline( buf, MSL );
         value = buf;
         strip_lspace( value );

         mssp_info->worlds = atoi( value.c_str() );
      }
      else if( key == "Xterm256" )
      {
         stream.getline( buf, MSL );
         value = buf;
         strip_lspace( value );

         mssp_info->xterm256 = atoi( value.c_str() );
      }
      else if( key == "End" )
         break;
      else
      {
         stream.getline( buf, MSL );
         value = buf;
         strip_lspace( value );
         log_printf( "Bad line in MSSP data file: %s %s", key.c_str(  ), value.c_str(  ) );
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
   ch->print_fmt( "&zWorlds            &W{}\r\n", mssp_info->worlds );
   ch->print_fmt( "&zAnsi              &W{}\r\n", MSSP_YN( mssp_info->ansi ) );
   ch->print_fmt( "&zMCCP              &W{}\r\n", MSSP_YN( mssp_info->mccp ) );
   ch->print_fmt( "&zMCP               &W{}\r\n", MSSP_YN( mssp_info->mcp ) );
   ch->print_fmt( "&zMSP               &W{}\r\n", MSSP_YN( mssp_info->msp ) );
   ch->print_fmt( "&zSSL               &W{}\r\n", MSSP_YN( mssp_info->ssl ) );
   ch->print_fmt( "&zMXP               &W{}\r\n", MSSP_YN( mssp_info->mxp ) );
   ch->print_fmt( "&zPueblo            &W{}\r\n", MSSP_YN( mssp_info->pueblo ) );
   ch->print_fmt( "&zVt100             &W{}\r\n", MSSP_YN( mssp_info->vt100 ) );
   ch->print_fmt( "&zXterm256          &W{}\r\n", MSSP_YN( mssp_info->xterm256 ) );
   ch->print_fmt( "&zPay2Play          &W{}\r\n", MSSP_YN( mssp_info->pay2play ) );
   ch->print_fmt( "&zPay4Perks         &W{}\r\n", MSSP_YN( mssp_info->pay4perks ) );
   ch->print_fmt( "&zHiringBuilders    &W{}\r\n", MSSP_YN( mssp_info->hiringBuilders ) );
   ch->print_fmt( "&zHiringCoders      &W{}\r\n", MSSP_YN( mssp_info->hiringCoders ) );
   ch->print_fmt( "&zAdultMaterial     &W{}\r\n", MSSP_YN( mssp_info->adultMaterial ) );
   ch->print_fmt( "&zMulticlassing     &W{}\r\n", MSSP_YN( mssp_info->multiclassing ) );
   ch->print_fmt( "&zNewbieFriendly    &W{}\r\n", MSSP_YN( mssp_info->newbieFriendly ) );
   ch->print_fmt( "&zPlayerCities      &W{}\r\n", MSSP_YN( mssp_info->playerCities ) );
   ch->print_fmt( "&zPlayerClans       &W{}\r\n", MSSP_YN( mssp_info->playerClans ) );
   ch->print_fmt( "&zPlayerCrafting    &W{}\r\n", MSSP_YN( mssp_info->playerCrafting ) );
   ch->print_fmt( "&zPlayerGuilds      &W{}\r\n", MSSP_YN( mssp_info->playerGuilds ) );
   ch->print_fmt( "&zEquipmentSystem   &W{}\r\n", mssp_info->equipmentSystem );
   ch->print_fmt( "&zMultiplaying      &W{}\r\n", mssp_info->multiplaying );
   ch->print_fmt( "&zPlayerKilling     &W{}\r\n", mssp_info->playerKilling );
   ch->print_fmt( "&zQuestSystem       &W{}\r\n", mssp_info->questSystem );
   ch->print_fmt( "&zRolePlaying       &W{}\r\n", mssp_info->roleplaying );
   ch->print_fmt( "&zTrainingSystem    &W{}\r\n", mssp_info->trainingSystem );
   ch->print_fmt( "&zWorldOriginality  &W{}\r\n", mssp_info->worldOriginality );
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
      ch->print( "hostname       ip                contact            icon             lanuage          location\r\n" );
      ch->print( "website        family            genre              gameplay         game_system\r\n" );
      ch->print( "status         subgenre          intermud           created          min_age\r\n" );
      ch->print( "worlds         ansi              mccp               mcp              msp\r\n" );
      ch->print( "ssl            mxp               pueblo             vt100            xterm256\r\n" );
      ch->print( "pay2play       pay4perks         hiring_builders    hiring_coders    adult_material\r\n" );
      ch->print( "multiclassing  newbie_friendly   player_cities      player_clans     player_crafting\r\n" );
      ch->print( "player_guilds  equipment_system  multiplaying       player_killing   quest_system\r\n" );
      ch->print( "roleplaying    training_system   world_originality\r\n" );

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
      ch->printf( "MSSP value, %s has been changed to: %s\r\n", arg1.c_str(), argument.c_str() );
      save_mssp_info(  );
      return;
   }
   if( !str_cmp( arg1, "ansi" ) )
      ynptr = &mssp_info->ansi;
   else if( !str_cmp( arg1, "mccp" ) )
      ynptr = &mssp_info->mccp;
   else if( !str_cmp( arg1, "mcp" ) )
      ynptr = &mssp_info->mcp;
   else if( !str_cmp( arg1, "msp" ) )
      ynptr = &mssp_info->msp;
   else if( !str_cmp( arg1, "ssl" ) )
      ynptr = &mssp_info->ssl;
   else if( !str_cmp( arg1, "mxp" ) )
      ynptr = &mssp_info->mxp;
   else if( !str_cmp( arg1, "pueblo" ) )
      ynptr = &mssp_info->pueblo;
   else if( !str_cmp( arg1, "vt100" ) )
      ynptr = &mssp_info->vt100;
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
   else if( !str_cmp( arg1, "adult_material" ) )
      ynptr = &mssp_info->adultMaterial;
   else if( !str_cmp( arg1, "multiclassing" ) )
      ynptr = &mssp_info->multiclassing;
   else if( !str_cmp( arg1, "newbie_friendly" ) )
      ynptr = &mssp_info->newbieFriendly;
   else if( !str_cmp( arg1, "player_cities" ) )
      ynptr = &mssp_info->playerCities;
   else if( !str_cmp( arg1, "player_clans" ) )
      ynptr = &mssp_info->playerClans;
   else if( !str_cmp( arg1, "player_crafting" ) )
      ynptr = &mssp_info->playerCrafting;
   else if( !str_cmp( arg1, "player_guilds" ) )
      ynptr = &mssp_info->playerGuilds;

   if( ynptr != nullptr )
   {
      bool newvalue = false;

      if( str_cmp( argument, "yes" ) && str_cmp( argument, "no" ) )
      {
         ch->printf( "You must specify 'yes' or 'no' for the %s value!\r\n", arg1.c_str() );
         return;
      }
      newvalue = !str_cmp( argument, "yes" ) ? true : false;
      *ynptr = newvalue;
      ch->printf( "MSSP value, %s has been changed to: %s\r\n", arg1.c_str(), argument.c_str() );
      save_mssp_info(  );
      return;
   }

   if( !str_cmp( arg1, "worlds" ) )
   {
      int value = atoi( argument.c_str() );

      if( !is_number( argument ) || ( value < 0 ) || ( value > MSSP_MAXVAL ) )
      {
         ch->printf( "The value for %s must be between 0 and %d\r\n", arg1.c_str(), MSSP_MAXVAL );
         return;
      }
      mssp_info->worlds = value;

      ch->printf( "MSSP value, %s has been changed to: %s\r\n", arg1.c_str(), argument.c_str() );
      save_mssp_info(  );
      return;
   }
   else if( !str_cmp( arg1, "created" ) )
   {
      int value;

      value = atoi( argument.c_str() );

      if( !is_number( argument ) || ( value < MSSP_MINCREATED ) || ( value > MSSP_MAXCREATED ) )
      {
         ch->printf( "The value for created must be between %d and %d\r\n", MSSP_MINCREATED, MSSP_MAXCREATED );
         return;
      }
      mssp_info->created = value;
      ch->printf( "MSSP value, %s has been changed to: %s\r\n", arg1.c_str(), argument.c_str() );
      save_mssp_info(  );
      return;
   }
   else if( !str_cmp( arg1, "multiplaying" ) || !str_cmp( arg1, "player_killing" ) ) 
   {
      if( str_cmp( argument, "None" ) && str_cmp( argument, "Restricted" ) && str_cmp( argument, "Full" ) ) 
      {
         ch->printf( "Valid choices for %s are: None, Restricted or Full\r\n", arg1.c_str() );
         return; 
      }
      if( !str_cmp( arg1, "multiplaying" ) )
         mssp_info->multiplaying = argument;
      else
         mssp_info->playerKilling = argument;

      ch->printf( "MSSP value, %s has been changed to: %s\r\n", arg1.c_str(), argument.c_str() );
      save_mssp_info(  );
      return;
   }
   else if( !str_cmp( arg1, "training_system" ) || !str_cmp( arg1, "equipment_system" ) )
   {
      if( str_cmp( argument, "None" ) && str_cmp( argument, "Level" ) && str_cmp( argument, "Skill" ) && str_cmp( argument, "Both" ) )
      {
         ch->printf( "Valid choices for %s are: None, Level, Skill or Both\r\n", arg1.c_str() );
         return;
      }
      if( !str_cmp( arg1, "training_system" ) )
         mssp_info->trainingSystem = argument;
      else
         mssp_info->equipmentSystem = argument;

      ch->printf( "MSSP value, %s has been changed to: %s\r\n", arg1.c_str(), argument.c_str() );
      save_mssp_info(  );
      return;
   }
   else if( !str_cmp( arg1, "quest_system" ) )
   {
      if( str_cmp( argument, "None" ) && str_cmp( argument, "Immortal Run" ) && str_cmp( argument, "Automated" ) && str_cmp( argument, "Integrated" ) )
      {
         ch->printf( "Valid choices for %s are: None, Immortal Run, Automated or Integrated\r\n", arg1.c_str() );
         return;
      }
      mssp_info->questSystem = argument;
      ch->printf( "MSSP value, %s has been changed to: %s\r\n", arg1.c_str(), argument.c_str() );
      save_mssp_info(  );
      return;
   }
   else if( !str_cmp( arg1, "roleplaying" ) )
   {
      if( str_cmp( argument, "None" ) && str_cmp( argument, "Accepted" ) && str_cmp( argument, "Encouraged" ) && str_cmp( argument, "Enforced" ) )
      {
         ch->printf( "Valid choices for %s are: None, Accepted, Encouraged or Enforced\r\n", arg1.c_str() );
         return;
      }
      mssp_info->roleplaying = argument;
      ch->printf( "MSSP value, %s has been changed to: %s\r\n", arg1.c_str(), argument.c_str() );
      save_mssp_info(  );
      return;
   }
   else if( !str_cmp( arg1, "world_originality" ) )
   {
      if( str_cmp( argument, "All Stock" ) && str_cmp( argument, "Mostly Stock" ) && str_cmp( argument, "Mostly Original" ) && str_cmp( argument, "All Original" ) )
      {
         ch->printf( "Valid choices for %s are: All Stock, Mostly Stock, Mostly Original or All Original\r\n", arg1.c_str() );
         return;
      }
      mssp_info->worldOriginality = argument;
      ch->printf( "MSSP value, %s has been changed to: %s\r\n", arg1.c_str(), argument.c_str() );
      save_mssp_info(  );
      return;
   }
   else if( !str_cmp( arg1, "min_age" ) )
   {
      int value;

      value = atoi( argument.c_str() );

      if( !is_number( argument ) || ( value < MSSP_MINAGE ) || ( value > MSSP_MAXAGE ) )
      {
         ch->printf( "The value for min_age must be between %d and %d\r\n", MSSP_MINAGE, MSSP_MAXAGE );
         return;
      }
      mssp_info->minAge = value;
      ch->printf( "MSSP value, %s has been changed to: %s\r\n", arg1.c_str(), argument.c_str() );
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

   mssp_reply( d, "HOSTNAME", "{}", mssp_info->hostname );
   mssp_reply( d, "IP", "{}", mssp_info->ip );
   mssp_reply( d, "PORT", "{}", mud_port );
   mssp_reply( d, "UPTIME", "{}", mud_start_time );
   mssp_reply( d, "PLAYERS", "{}", player_count( ) );
   mssp_reply( d, "CODEBASE", "{} {}", CODENAME, CODEVERSION );
   mssp_reply( d, "CONTACT", "{}", mssp_info->contact );
   mssp_reply( d, "CREATED", "{}", mssp_info->created );
   mssp_reply( d, "ICON", "{}", mssp_info->icon );
   mssp_reply( d, "LANGUAGE", "{}", mssp_info->language );
   mssp_reply( d, "LOCATION", "{}", mssp_info->location );
   mssp_reply( d, "MINIMUM AGE", "{}", mssp_info->minAge );
   mssp_reply( d, "NAME", "{}", sysdata->mud_name );
   mssp_reply( d, "WEBSITE", "{}", show_tilde( sysdata->http ) );
   mssp_reply( d, "FAMILY", "{}", mssp_info->family );
   mssp_reply( d, "GENRE", "{}", mssp_info->genre );
   mssp_reply( d, "GAMEPLAY", "{}", mssp_info->gamePlay );
   mssp_reply( d, "GAMESYSTEM", "{}", mssp_info->gameSystem );
   mssp_reply( d, "INTERMUD", "{}", mssp_info->intermud );
   mssp_reply( d, "STATUS", "{}", mssp_info->status );
   mssp_reply( d, "SUBGENRE", "{}", mssp_info->subgenre );
   mssp_reply( d, "WORLDS", "{}", mssp_info->worlds );
   mssp_reply( d, "AREAS", "{}", top_area );
   mssp_reply( d, "ROOMS", "{}", top_room );
   mssp_reply( d, "RESETS", "{}", top_reset );
   mssp_reply( d, "EXITS", "{}", top_exit );
   mssp_reply( d, "MOBILES", "{}", top_mob_index );
   mssp_reply( d, "OBJECTS", "{}", top_obj_index );
   mssp_reply( d, "MUDPROGS", "{}", top_prog );
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
   mssp_reply( d, "MCP", "{}", mssp_info->mcp );
   mssp_reply( d, "MSP", "{}", mssp_info->msp );
   mssp_reply( d, "SSL", "{}", mssp_info->ssl );
   mssp_reply( d, "MXP", "{}", mssp_info->mxp );
   mssp_reply( d, "PUEBLO", "{}", mssp_info->pueblo );
   mssp_reply( d, "VT100", "{}", mssp_info->vt100 );
   mssp_reply( d, "XTERM 256 COLORS", "{}", mssp_info->xterm256 );
   mssp_reply( d, "PAY TO PLAY", "{}", mssp_info->pay2play );
   mssp_reply( d, "PAY FOR PERKS", "{}", mssp_info->pay4perks );
   mssp_reply( d, "HIRING BUILDERS", "{}", mssp_info->hiringBuilders );
   mssp_reply( d, "HIRING CODERS", "{}", mssp_info->hiringCoders );
   mssp_reply( d, "ADULT MATERIAL", "{}", mssp_info->adultMaterial );
   mssp_reply( d, "MULTICLASSING", "{}", mssp_info->multiclassing );
   mssp_reply( d, "NEWBIE FRIENDLY", "{}", mssp_info->newbieFriendly );
   mssp_reply( d, "PLAYER CITIES", "{}", mssp_info->playerCities );
   mssp_reply( d, "PLAYER CLANSS", "{}", mssp_info->playerClans );
   mssp_reply( d, "PLAYER CRAFTING", "{}", mssp_info->playerCrafting );
   mssp_reply( d, "PLAYER GUILDS", "{}", mssp_info->playerGuilds );
   mssp_reply( d, "EQUIPMENT SYSTEM", "{}", mssp_info->equipmentSystem );
   mssp_reply( d, "MULTIPLAYING", "{}", mssp_info->multiplaying );
   mssp_reply( d, "PLAYERKILLING", "{}", mssp_info->playerKilling );
   mssp_reply( d, "QUEST SYSTEM", "{}", mssp_info->questSystem );
   mssp_reply( d, "ROLEPLAYING", "{}", mssp_info->roleplaying );
   mssp_reply( d, "TRAINING SYSTEM", "{}", mssp_info->trainingSystem );
   mssp_reply( d, "WORLD ORIGINALITY", "{}", mssp_info->worldOriginality );

   d->write( "MSSP-REPLY-END\r\n" );
}
