/****************************************************************************
 *                   ^     +----- |  / ^     ^ |     | +-\                  *
 *                  / \    |      | /  |\   /| |     | |  \                 *
 *                 /   \   +---   |<   | \ / | |     | |  |                 *
 *                /-----\  |      | \  |  v  | |     | |  /                 *
 *               /       \ |      |  \ |     | +-----+ +-/                  *
 ****************************************************************************
 * AFKMud Copyright 1997-2020 by Roger Libiez (Samson),                     *
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
*  This program will allow admin to view and set thier MSSP       *
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

#include <cstdio>
#include <cstdarg>
#include <cstring>
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
   ofstream stream;

   stream.open( MSSP_FILE );
   if( !stream.is_open(  ) )
   {
      bug( "%s: fopen", __func__ );
      perror( MSSP_FILE );
   }
   else
   {
      stream << "#MSSP_INFO" << endl;
      stream << "Hostname          " << mssp_info->hostname << endl;
      stream << "IP                " << mssp_info->ip << endl;
      stream << "Contact           " << mssp_info->contact << endl;
      stream << "Icon              " << mssp_info->icon << endl;
      stream << "Language          " << mssp_info->language << endl;
      stream << "Location          " << mssp_info->location << endl;
      stream << "Family            " << mssp_info->family << endl;
      stream << "Genre             " << mssp_info->genre << endl;
      stream << "GamePlay          " << mssp_info->gamePlay << endl;
      stream << "GameSystem        " << mssp_info->gameSystem << endl;
      stream << "Status            " << mssp_info->status << endl;
      stream << "SubGenre          " << mssp_info->subgenre << endl;
      stream << "Created           " << mssp_info->created << endl;
      stream << "MinAge            " << mssp_info->minAge << endl;
      stream << "Worlds            " << mssp_info->worlds << endl;
      stream << "Ansi              " << mssp_info->ansi << endl;
      stream << "MCCP              " << mssp_info->mccp << endl;
      stream << "MCP               " << mssp_info->mcp << endl;
      stream << "MSP               " << mssp_info->msp << endl;
      stream << "SSL               " << mssp_info->ssl << endl;
      stream << "MXP               " << mssp_info->mxp << endl;
      stream << "Pueblo            " << mssp_info->pueblo << endl;
      stream << "Vt100             " << mssp_info->vt100 << endl;
      stream << "Xterm256          " << mssp_info->xterm256 << endl;
      stream << "Pay2Play          " << mssp_info->pay2play << endl;
      stream << "Pay4Perks         " << mssp_info->pay4perks << endl;
      stream << "HiringBuilders    " << mssp_info->hiringBuilders << endl;
      stream << "HiringCoders      " << mssp_info->hiringCoders << endl;
      stream << "AdultMaterial     " << mssp_info->adultMaterial << endl;
      stream << "Multiclassing     " << mssp_info->multiclassing << endl;
      stream << "NewbieFriendly    " << mssp_info->newbieFriendly << endl;
      stream << "PlayerCities      " << mssp_info->playerCities << endl;
      stream << "PlayerClans       " << mssp_info->playerClans << endl;
      stream << "PlayerCrafting    " << mssp_info->playerCrafting << endl;
      stream << "PlayerGuilds      " << mssp_info->playerGuilds << endl;
      stream << "EquipmentSystem   " << mssp_info->equipmentSystem << endl;
      stream << "Multiplaying      " << mssp_info->multiplaying << endl;
      stream << "PlayerKilling     " << mssp_info->playerKilling << endl;
      stream << "QuestSystem       " << mssp_info->questSystem << endl;
      stream << "RolePlaying       " << mssp_info->roleplaying << endl;
      stream << "TrainingSystem    " << mssp_info->trainingSystem << endl;
      stream << "WorldOriginality  " << mssp_info->worldOriginality << endl;
      stream << "End" << endl << endl;

      stream.close(  );
   }
}

/*
 * Load the MSSP data file
 */
void load_mssp_data( void )
{
   ifstream stream;

   stream.open( MSSP_FILE );
   if( !stream.is_open(  ) )
   {
      log_printf( "No MSSP data file found. Generating default data." );
      mssp_info = new msspinfo;
      save_mssp_info();
      return;
   }

   do
   {
      string key, value;
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

#define MSSP_YN( value )  ( (value) == 0 ? "No" : "Yes" )

void show_mssp( char_data * ch )
{
   if( !ch )
   {
      bug( "%s: nullptr ch", __func__ );
      return;
   }

   ch->printf( "&zHostname          &W%s\r\n", mssp_info->hostname.c_str() );
   ch->printf( "&zIP                &W%s\r\n", mssp_info->ip.c_str() );
   ch->printf( "&zContact           &W%s\r\n", mssp_info->contact.c_str() );
   ch->printf( "&zIcon              &W%s\r\n", mssp_info->icon.c_str() );
   ch->printf( "&zLanguage          &W%s\r\n", mssp_info->language.c_str() );
   ch->printf( "&zLocation          &W%s\r\n", mssp_info->location.c_str() );
   ch->printf( "&zWebsite           &W%s\r\n", show_tilde( sysdata->http ).c_str() );
   ch->printf( "&zFamily            &W%s\r\n", mssp_info->family.c_str() );
   ch->printf( "&zGenre             &W%s\r\n", mssp_info->genre.c_str() );
   ch->printf( "&zGamePlay          &W%s\r\n", mssp_info->gamePlay.c_str() );
   ch->printf( "&zGameSystem        &W" );
   ch->desc->buffer_printf( "%s\r\n", mssp_info->gameSystem.c_str() ); // Because D&D is a valid option and color tags mess this up.
   ch->printf( "&zStatus            &W%s\r\n", mssp_info->status.c_str() );
   ch->printf( "&zSubGenre          &W%s\r\n", mssp_info->subgenre.c_str() );
   ch->printf( "&zCreated           &W%d\r\n", mssp_info->created );
   ch->printf( "&zMinAge            &W%d\r\n", mssp_info->minAge );
   ch->printf( "&zWorlds            &W%d\r\n", mssp_info->worlds );
   ch->printf( "&zAnsi              &W%s\r\n", MSSP_YN( mssp_info->ansi ) );
   ch->printf( "&zMCCP              &W%s\r\n", MSSP_YN( mssp_info->mccp ) );
   ch->printf( "&zMCP               &W%s\r\n", MSSP_YN( mssp_info->mcp ) );
   ch->printf( "&zMSP               &W%s\r\n", MSSP_YN( mssp_info->msp ) );
   ch->printf( "&zSSL               &W%s\r\n", MSSP_YN( mssp_info->ssl ) );
   ch->printf( "&zMXP               &W%s\r\n", MSSP_YN( mssp_info->mxp ) );
   ch->printf( "&zPueblo            &W%s\r\n", MSSP_YN( mssp_info->pueblo ) );
   ch->printf( "&zVt100             &W%s\r\n", MSSP_YN( mssp_info->vt100 ) );
   ch->printf( "&zXterm256          &W%s\r\n", MSSP_YN( mssp_info->xterm256 ) );
   ch->printf( "&zPay2Play          &W%s\r\n", MSSP_YN( mssp_info->pay2play ) );
   ch->printf( "&zPay4Perks         &W%s\r\n", MSSP_YN( mssp_info->pay4perks ) );
   ch->printf( "&zHiringBuilders    &W%s\r\n", MSSP_YN( mssp_info->hiringBuilders ) );
   ch->printf( "&zHiringCoders      &W%s\r\n", MSSP_YN( mssp_info->hiringCoders ) );
   ch->printf( "&zAdultMaterial     &W%s\r\n", MSSP_YN( mssp_info->adultMaterial ) );
   ch->printf( "&zMulticlassing     &W%s\r\n", MSSP_YN( mssp_info->multiclassing ) );
   ch->printf( "&zNewbieFriendly    &W%s\r\n", MSSP_YN( mssp_info->newbieFriendly ) );
   ch->printf( "&zPlayerCities      &W%s\r\n", MSSP_YN( mssp_info->playerCities ) );
   ch->printf( "&zPlayerClans       &W%s\r\n", MSSP_YN( mssp_info->playerClans ) );
   ch->printf( "&zPlayerCrafting    &W%s\r\n", MSSP_YN( mssp_info->playerCrafting ) );
   ch->printf( "&zPlayerGuilds      &W%s\r\n", MSSP_YN( mssp_info->playerGuilds ) );
   ch->printf( "&zEquipmentSystem   &W%s\r\n", mssp_info->equipmentSystem.c_str() );
   ch->printf( "&zMultiplaying      &W%s\r\n", mssp_info->multiplaying.c_str() );
   ch->printf( "&zPlayerKilling     &W%s\r\n", mssp_info->playerKilling.c_str() );
   ch->printf( "&zQuestSystem       &W%s\r\n", mssp_info->questSystem.c_str() );
   ch->printf( "&zRolePlaying       &W%s\r\n", mssp_info->roleplaying.c_str() );
   ch->printf( "&zTrainingSystem    &W%s\r\n", mssp_info->trainingSystem.c_str() );
   ch->printf( "&zWorldOriginality  &W%s\r\n", mssp_info->worldOriginality.c_str() );
}

CMDF( do_setmssp )
{
   string arg1;
   string *strptr = nullptr;
   bool *ynptr = nullptr;

   if( argument.empty() )
   {
      ch->print( "Syntax: setmssp show\r\n" );
      ch->print( "Syntax: setmssp <field> [value]\r\n" );
      ch->print( "Field being one of:\r\n" );
      ch->print( "hostname       ip                contact            icon             lanuage          location\r\n" );
      ch->print( "website        family            genre              gameplay         game_system\r\n" );
      ch->print( "status         subgenre          created            min_age\r\n" );
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

void write_to_descriptor_printf( descriptor_data * desc, const char *fmt, ... ) __attribute__ ( ( format( printf, 2, 3 ) ) );
void write_to_descriptor_printf( descriptor_data * desc, const char *fmt, ... )
{
    char buf[MSL * 2];

    va_list args;

    va_start( args, fmt );
    vsprintf( buf, fmt, args );
    va_end( args );

    desc->write( buf );
}

void mssp_reply( descriptor_data * d, const char *var, const char *fmt, ... ) __attribute__ ( ( format( printf, 3, 4 ) ) );
void mssp_reply( descriptor_data * d, const char *var, const char *fmt, ... )
{
   char buf[MSL];
   va_list args;

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

   va_start( args, fmt );
   vsprintf( buf, fmt, args );
   va_end( args );

   write_to_descriptor_printf( d, "%s\t%s\r\n", var, buf );
}

short player_count( void )
{
   list < descriptor_data * >::iterator ds;
   short count = 0;

   for( ds = dlist.begin(  ); ds != dlist.end(  ); ++ds )
   {
      descriptor_data *d = *ds;

      if( d->connected >= CON_PLAYING )
         ++count;
   }
   return count;
}

#if !defined(__CYGWIN__) && defined(SQL)
 int db_help_count();
#endif
extern time_t mud_start_time;
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

   mssp_reply( d, "HOSTNAME", "%s", mssp_info->hostname.c_str() );
   mssp_reply( d, "IP", "%s", mssp_info->ip.c_str() );
   mssp_reply( d, "PORT", "%d", mud_port );
   mssp_reply( d, "UPTIME", "%d", (int)mud_start_time );
   mssp_reply( d, "PLAYERS", "%d", player_count( ) );
   mssp_reply( d, "CODEBASE", "%s %s", CODENAME, CODEVERSION );
   mssp_reply( d, "CONTACT", "%s", mssp_info->contact.c_str() );
   mssp_reply( d, "CREATED", "%d", mssp_info->created );
   mssp_reply( d, "ICON", "%s", mssp_info->icon.c_str() );
   mssp_reply( d, "LANGUAGE", "%s", mssp_info->language.c_str() );
   mssp_reply( d, "LOCATION", "%s", mssp_info->location.c_str() );
   mssp_reply( d, "MINIMUM AGE", "%d", mssp_info->minAge );
   mssp_reply( d, "NAME", "%s", sysdata->mud_name.c_str() );
   mssp_reply( d, "WEBSITE", "%s", show_tilde( sysdata->http ).c_str(  ) );
   mssp_reply( d, "FAMILY", "%s", mssp_info->family.c_str(  ) );
   mssp_reply( d, "GENRE", "%s", mssp_info->genre.c_str(  ) );
   mssp_reply( d, "GAMEPLAY", "%s", mssp_info->gamePlay.c_str(  ) );
   mssp_reply( d, "GAMESYSTEM", "%s", mssp_info->gameSystem.c_str(  ) );
   mssp_reply( d, "STATUS", "%s", mssp_info->status.c_str(  ) );
   mssp_reply( d, "SUBGENRE", "%s", mssp_info->subgenre.c_str(  ) );
   mssp_reply( d, "WORLDS", "%d", mssp_info->worlds );
   mssp_reply( d, "AREAS", "%d", top_area );
   mssp_reply( d, "ROOMS", "%d", top_room );
   mssp_reply( d, "RESETS", "%d", top_reset );
   mssp_reply( d, "EXITS", "%d", top_exit );
   mssp_reply( d, "MOBILES", "%d", top_mob_index );
   mssp_reply( d, "OBJECTS", "%d", top_obj_index );
   mssp_reply( d, "MUDPROGS", "%d", top_prog );
#if !defined(__CYGWIN__) && defined(SQL)
   mssp_reply( d, "HELPFILES", "%d", db_help_count() );
#else
   mssp_reply( d, "HELPFILES", "%d", top_help );
#endif
   mssp_reply( d, "LEVELS", "%d", LEVEL_AVATAR );
   mssp_reply( d, "RACES", "%d", MAX_PC_RACE );
   mssp_reply( d, "CLASSES", "%d", MAX_PC_CLASS );
   mssp_reply( d, "SKILLS", "%d", num_skills );
   mssp_reply( d, "ANSI", "%d", mssp_info->ansi );
   mssp_reply( d, "MCCP", "%d", mssp_info->mccp );
   mssp_reply( d, "MCP", "%d", mssp_info->mcp );
   mssp_reply( d, "MSP", "%d", mssp_info->msp );
   mssp_reply( d, "SSL", "%d", mssp_info->ssl );
   mssp_reply( d, "MXP", "%d", mssp_info->mxp );
   mssp_reply( d, "PUEBLO", "%d", mssp_info->pueblo );
   mssp_reply( d, "VT100", "%d", mssp_info->vt100 );
   mssp_reply( d, "XTERM 256 COLORS", "%d", mssp_info->xterm256 );
   mssp_reply( d, "PAY TO PLAY", "%d", mssp_info->pay2play );
   mssp_reply( d, "PAY FOR PERKS", "%d", mssp_info->pay4perks );
   mssp_reply( d, "HIRING BUILDERS", "%d", mssp_info->hiringBuilders );
   mssp_reply( d, "HIRING CODERS", "%d", mssp_info->hiringCoders );
   mssp_reply( d, "ADULT MATERIAL", "%d", mssp_info->adultMaterial );
   mssp_reply( d, "MULTICLASSING", "%d", mssp_info->multiclassing );
   mssp_reply( d, "NEWBIE FRIENDLY", "%d", mssp_info->newbieFriendly );
   mssp_reply( d, "PLAYER CITIES", "%d", mssp_info->playerCities );
   mssp_reply( d, "PLAYER CLANSS", "%d", mssp_info->playerClans );
   mssp_reply( d, "PLAYER CRAFTING", "%d", mssp_info->playerCrafting );
   mssp_reply( d, "PLAYER GUILDS", "%d", mssp_info->playerGuilds );
   mssp_reply( d, "EQUIPMENT SYSTEM", "%s", mssp_info->equipmentSystem.c_str(  ) );
   mssp_reply( d, "MULTIPLAYING", "%s", mssp_info->multiplaying.c_str(  ) );
   mssp_reply( d, "PLAYERKILLING", "%s", mssp_info->playerKilling.c_str(  ) );
   mssp_reply( d, "QUEST SYSTEM", "%s", mssp_info->questSystem.c_str(  ) );
   mssp_reply( d, "ROLEPLAYING", "%s", mssp_info->roleplaying.c_str(  ) );
   mssp_reply( d, "TRAINING SYSTEM", "%s", mssp_info->trainingSystem.c_str(  ) );
   mssp_reply( d, "WORLD ORIGINALITY", "%s", mssp_info->worldOriginality.c_str(  ) );

   d->write( "MSSP-REPLY-END\r\n" );
}
