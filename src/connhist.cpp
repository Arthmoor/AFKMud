/****************************************************************************
 *                   ^     +----- |  / ^     ^ |     | +-\                  *
 *                  / \    |      | /  |\   /| |     | |  \                 *
 *                 /   \   +---   |<   | \ / | |     | |  |                 *
 *                /-----\  |      | \  |  v  | |     | |  /                 *
 *               /       \ |      |  \ |     | +-----+ +-/                  *
 ****************************************************************************
 * AFKMud Copyright 1997-2010 by Roger Libiez (Samson),                     *
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

#include <fstream>
#if defined(WIN32)
#include <unistd.h>
#endif
#include "mud.h"
#include "descriptor.h"
#include "connhist.h"

/* Globals */
list < conn_data * >connlist;

conn_data::conn_data(  )
{
   init_memory( &type, &invis_lvl, sizeof( invis_lvl ) );
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
   list < conn_data * >::iterator iconn;

   for( iconn = connlist.begin(  ); iconn != connlist.end(  ); )
   {
      conn_data *con = *iconn;
      ++iconn;

      deleteptr( con );
   }

   if( arg == 1 )
      unlink( CH_FILE );
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
      bug( "%s: Removed bugged conn entry!", __FUNCTION__ );
      return CHK_CONN_REMOVED;
   }
   return CHK_CONN_OK;
}

/* Loads the conn.hist file into memory */
void load_connhistory( void )
{
   ifstream stream;
   conn_data *conn;
   size_t conncount = 0;

   connlist.clear(  );

   stream.open( CH_FILE );
   if( !stream.is_open(  ) )
      return;

   do
   {
      string key, value;
      char buf[MIL];

      stream >> key;
      stream.getline( buf, MIL );
      value = buf;

      strip_lspace( key );
      strip_lspace( value );
      strip_tilde( value );

      if( key.empty(  ) )
         continue;

      if( key == "#CONN" )
         conn = new conn_data;
      else if( key == "User" )
         conn->user = value;
      else if( key == "When" )
         conn->when = value;
      else if( key == "Host" )
         conn->host = value;
      else if( key == "Level" )
         conn->level = atoi( value.c_str(  ) );
      else if( key == "Type" )
         conn->type = atoi( value.c_str(  ) );
      else if( key == "Invis" )
         conn->type = atoi( value.c_str(  ) );
      else if( key == "End" )
      {
         if( conncount < MAX_CONNHISTORY )
         {
            ++conncount;
            connlist.push_back( conn );
         }
         else
         {
            bug( "%s: more connection histories than MAX_CONNHISTORY %zd", __FUNCTION__, MAX_CONNHISTORY );
            stream.close(  );
            return;
         }
      }
      else
         bug( "%s: Bad line in connection history file: %s %s", __FUNCTION__, key.c_str(  ), value.c_str(  ) );
   }
   while( !stream.eof(  ) );
   stream.close(  );
}

/* Saves the conn.hist file */
void save_connhistory( void )
{
   ofstream stream;
   list < conn_data * >::iterator iconn;

   if( connlist.empty(  ) )
      return;

   stream.open( CH_FILE );
   if( !stream.is_open(  ) )
   {
      bug( "%s: Error opening '%s'", __FUNCTION__, CH_FILE );
      return;
   }

   for( iconn = connlist.begin(  ); iconn != connlist.end(  ); ++iconn )
   {
      conn_data *conn = *iconn;

      /*
       * Only save OK conn entries 
       */
      if( ( check_conn_entry( conn ) ) == CHK_CONN_OK )
      {
         stream << "#CONN" << endl;
         stream << "User   " << conn->user << endl;
         stream << "Host   " << conn->host << endl;
         stream << "When   " << conn->when << endl;
         stream << "Level  " << conn->level << endl;
         stream << "Type   " << conn->type << endl;
         stream << "Invis  " << conn->invis_lvl << endl;
         stream << "End" << endl << endl;
      }
   }
   stream.close(  );
}

/* NeoCode 0.08 Revamp of this! I now base it off of descriptor_data.
 * It will pull needed data from the descriptor. If there's no character
 * than it will use default information. -- X
 */
void update_connhistory( descriptor_data * d, int type )
{
   list < conn_data * >::iterator conn;
   conn_data *con;
   char_data *vch;
   struct tm *local;
   time_t t;
   char when[MIL];

   if( !d )
   {
      bug( "%s: NULL descriptor!", __FUNCTION__ );
      return;
   }

   vch = d->original ? d->original : d->character;
   if( !vch )
      return;

   if( vch->isnpc(  ) )
      return;

   /*
    * Count current histories, if more than the defined MAX, then remove the first one. -- X 
    */
   if( connlist.size(  ) >= MAX_CONNHISTORY )
   {
      con = *connlist.begin(  );
      connlist.erase( connlist.begin(  ) );
      deleteptr( con );
   }

   /*
    * Build our time string... 
    */
   t = time( NULL );
   local = localtime( &t );

   /*
    * Create our entry and fill the fields! 
    */
   con = new conn_data;
   con->user = vch->name ? vch->name : "NoName";
   snprintf( when, MIL, "%-2.2d/%-2.2d %-2.2d:%-2.2d", local->tm_mon + 1, local->tm_mday, local->tm_hour, local->tm_min );
   con->when = when;
   con->host = !d->host.empty(  )? d->host : "unknown";
   con->type = type;
   con->level = vch->level;
   con->invis_lvl = vch->has_pcflag( PCFLAG_WIZINVIS ) ? vch->pcdata->wizinvis : 0;
   connlist.push_back( con );
   save_connhistory(  );
}

/* The logins command */
/* Those who are equal or greather than the CH_LVL_ADMIN defined in connhist.h
 * can also prompt a complete purging of the histories and the conn.hist file. */
CMDF( do_logins )
{
   int conn_count = 0;
   list < conn_data * >::iterator iconn;
   char user[MSL];
   string typebuf;

   if( !argument.empty(  ) && ( !str_cmp( argument, "clear" ) && ch->level >= CH_LVL_ADMIN ) )
   {
      ch->print( "Clearing Connection history...\r\n" );
      free_connhistory( 1 );  /* Remember - Arg must = 1 to wipe the file on disk! -- X */
      return;
   }

   /*
    * Modify this line to fit your tastes 
    */
   ch->printf( "&c----[&WConnection History for %s&c]----&w\r\n", sysdata->mud_name.c_str(  ) );

   for( iconn = connlist.begin(  ); iconn != connlist.end(  ); ++iconn )
   {
      conn_data *conn = *iconn;

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
            snprintf( user, MSL, "(&cInvis &p%d&w) %s", conn->invis_lvl, conn->user.c_str(  ) );
         else
            mudstrlcpy( user, conn->user.c_str(  ), MSL );

         /*
          * The format for the history are these two lines below. First is for Immortals, second for players. 
          * If you know what you're doing, than you can modify the output here 
          */
         if( ch->is_immortal(  ) )
            ch->printf( "&c[&O%s&c] &w%s&g@&w%s&c%s&w\r\n", conn->when.c_str(  ), user, conn->host.c_str(  ), typebuf.c_str(  ) );
         else
            ch->printf( "&c[&O%s&c] &w%s&c%s&w\r\n", conn->when.c_str(  ), user, typebuf.c_str(  ) );
      }
   }

   if( !conn_count )
      ch->print( "&WNo Data.&w\r\n" );

   if( ch->level >= CH_LVL_ADMIN )
      ch->print( "\r\nAdmin: You may type LOGIN CLEAR to wipe the current history.\r\n" );
}
