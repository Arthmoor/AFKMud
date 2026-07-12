/****************************************************************************
 *                   ^     +----- |  / ^     ^ |     | +-\                  *
 *                  / \    |      | /  |\   /| |     | |  \                 *
 *                 /   \   +---   |<   | \ / | |     | |  |                 *
 *                /-----\  |      | \  |  v  | |     | |  /                 *
 *               /       \ |      |  \ |     | +-----+ +-/                  *
 ****************************************************************************
 * AFKMud Copyright 1997-2026 by Roger Libiez (Samson),                     *
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
 *                       Xorith's Connection History                        *
 ****************************************************************************/

/* ConnHistory Feature
 *
 * Based loosely on Samson's Channel History functions. (basic idea)
 * Written by: Xorith on 5/7/03, last updated: 9/20/03
 *
 * Stores connection data in an array so that it can be reviewed later.
 *
 */
#include <filesystem>
#include <fstream>
#include "mud.h"
#include "descriptor.h"
#include "connhist.h"

/* Globals */
std::list<conn_data *> connlist;

conn_data::conn_data(  )
{
}

/* Removes a single conn entry */
conn_data::~conn_data(  )
{
   connlist.remove( this );
}

/* Frees all the histories from memory.
 * If Arg == 1, then it also deletes the conn.hist file
 */
void free_connhistory( int arg )
{
   for( auto it = connlist.begin(  ); it != connlist.end(  ); )
   {
      conn_data *con = *it;
      ++it;

      deleteptr( con );
   }

   if( arg == 1 )
      std::filesystem::remove( CH_FILE );
}

/* Checks an entry for validity. Removes an invalid entry. */
/* This could possibly be useless, as the code that handles updating the file
 * already does most of this. Why not be extra-safe though? */
int check_conn_entry( conn_data * conn )
{
   if( !conn )
      return CHK_CONN_REMOVED;

   if( conn->user.empty(  ) || conn->host.empty(  ) || conn->when.empty(  ) )
   {
      deleteptr( conn );
      bug( "{}: Removed bugged conn entry!", __func__ );
      return CHK_CONN_REMOVED;
   }
   return CHK_CONN_OK;
}

/* Loads the conn.hist file into memory */
void load_connhistory( void )
{
   std::ifstream stream( std::filesystem::path{CH_FILE} );
   if( !stream.is_open(  ) )
   {
      bug( "{}: Cannot open {} for reading: {}", __func__, CH_FILE, std::strerror(errno) );
      return;
   }

   connlist.clear(  );
   size_t conncount = 0;
   conn_data *conn = nullptr;
   std::string key;
   while( stream >> key )
   {
      if( key == "#CONN" )
         conn = new conn_data;
      else if( key == "End" )
      {
         if( conncount < MAX_CONNHISTORY )
         {
            ++conncount;
            connlist.push_back( conn );
         }
         else
         {
            bug( "{}: more connection histories than MAX_CONNHISTORY {}", __func__, MAX_CONNHISTORY );
            stream.close(  );
            return;
         }
      }
      else if( key == "User" )   { conn->user = fread_line( stream, '\n' ); }
      else if( key == "When" )   { conn->when = fread_line( stream, '\n' ); }
      else if( key == "Host" )   { conn->host = fread_line( stream, '\n' ); }
      else if( key == "Level" )  { stream >> conn->level; }
      else if( key == "Type" )   { stream >> conn->type; }
      else if( key == "Invis" )  { stream >> conn->invis_lvl; }
      else
      {
         bug( "{}: Bad key in connection history file: {}", __func__, key );
         fread_to_eol( stream );
      }
   }
   stream.close(  );
}

/* Saves the conn.hist file */
void save_connhistory( void )
{
   if( connlist.empty(  ) )
      return;

   std::ofstream stream( std::filesystem::path{CH_FILE} );
   if( !stream.is_open(  ) )
   {
      bug( "{}: Cannot open {} for writing: {}", __func__, CH_FILE, std::strerror(errno) );
      return;
   }

   for( auto* conn : connlist )
   {
      /*
       * Only save OK conn entries 
       */
      if( ( check_conn_entry( conn ) ) == CHK_CONN_OK )
      {
         stream << "#CONN\n";
         stream << std::format( "User       {}\n", conn->user );
         stream << std::format( "Host       {}\n", conn->host );
         stream << std::format( "When       {}\n", conn->when );
         stream << std::format( "Level      {}\n", conn->level );
         stream << std::format( "Type       {}\n", conn->type );
         stream << std::format( "Invis      {}\n", conn->invis_lvl );
         stream << "End\n\n";
      }
   }
   stream.close(  );
   if( stream.fail() )
      bug( "{}: Error occurred after closing {}: ", __func__, CH_FILE, std::strerror(errno) );
}

/*
 * NeoCode 0.08 Revamp of this! I now base it off of descriptor_data.
 * It will pull needed data from the descriptor. If there's no character
 * than it will use default information. -- X
 */
void update_connhistory( descriptor_data * d, int type )
{
   conn_data *con;
   char_data *vch;

   if( !d )
   {
      bug( "{}: nullptr descriptor!", __func__ );
      return;
   }

   vch = d->original ? d->original : d->character;
   if( !vch || vch->isnpc() )
      return;

   if( connlist.size() >= MAX_CONNHISTORY )
   {
      con = *connlist.begin();
      connlist.erase( connlist.begin() );
      deleteptr( con );
   }

   auto local_now = std::chrono::zoned_time{ std::chrono::current_zone(), current_time };

   con = new conn_data;
   con->user = !vch->name.empty() ? vch->name : "NoName";

   con->when = std::format( "{:%m/%d %H:%M}", local_now );

   con->host = !d->hostname.empty() ? d->hostname : d->ipaddress;
   con->type = type;
   con->level = vch->level;
   con->invis_lvl = vch->has_pcflag( PCFLAG_WIZINVIS ) ? vch->pcdata->wizinvis : 0;

   connlist.push_back( con );
   save_connhistory();
}

/* The logins command */
/* Those who are equal or greater than the CH_LVL_ADMIN defined in connhist.h
 * can also prompt a complete purging of the histories and the conn.hist file. */
CMDF( do_logins )
{
   int conn_count = 0;
   std::string user, typebuf;

   if( !argument.empty(  ) && ( !str_cmp( argument, "clear" ) && ch->level >= CH_LVL_ADMIN ) )
   {
      ch->print( "Clearing Connection history...\r\n" );
      free_connhistory( 1 );  /* Remember - Arg must = 1 to wipe the file on disk! -- X */
      return;
   }

   /*
    * Modify this line to fit your tastes 
    */
   ch->print_fmt( "&c----[&WConnection History for {}&c]----&w\r\n", sysdata->mud_name );

   for( auto* conn : connlist )
   {
      if( ( check_conn_entry( conn ) ) != CHK_CONN_OK )
         continue;

      if( conn->invis_lvl <= ch->level )
      {
         ++conn_count;
         switch ( conn->type )
         {
            case CONNTYPE_LOGIN:
               typebuf = " has logged in.";
               break;

            case CONNTYPE_QUIT:
               typebuf = " has logged out.";
               break;

            case CONNTYPE_IDLE:
               typebuf = " has been logged out due to inactivity.";
               break;

            case CONNTYPE_LINKDEAD:
               typebuf = " has lost their link.";
               break;

            case CONNTYPE_NEWPLYR:
               typebuf = " has logged in for the first time!";
               break;

            case CONNTYPE_RECONN:
               typebuf = " has reconnected.";
               break;

            default:
               typebuf = ".";
               break;
         }

         /*
          * If a player is wizinvis, and an immortal can see them, then tell that immortal
          * * what invis level they were. Note: change color for the Invis tag here.
          */
         if( conn->invis_lvl > 0 && ch->is_immortal(  ) )
            user = std::format( "(&cInvis &p{}&w) {}", conn->invis_lvl, conn->user );
         else
            user = conn->user;

         /*
          * The format for the history are these two lines below. First is for Immortals, second for players. 
          * If you know what you're doing, than you can modify the output here 
          */
         if( ch->is_immortal(  ) )
            ch->print_fmt( "&c[&O{}&c] &w{}&g@&w{}&c{}&w\r\n", conn->when, user, conn->host, typebuf );
         else
            ch->print_fmt( "&c[&O{}&c] &w{}&c{}&w\r\n", conn->when, user, typebuf );
      }
   }

   if( !conn_count )
      ch->print( "&WNo Data.&w\r\n" );

   if( ch->level >= CH_LVL_ADMIN )
      ch->print( "\r\nAdmin: You may type LOGIN CLEAR to wipe the current history.\r\n" );
}
