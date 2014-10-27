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

#if defined(WIN32)
#include <unistd.h>
#endif
#include "mud.h"
#include "descriptor.h"
#include "connhist.h"

/* Globals */
list<conn_data*> connlist;

conn_data::conn_data()
{
   init_memory( &user, &invis_lvl, sizeof( invis_lvl ) );
}

/* Removes a single conn entry */
conn_data::~conn_data()
{
   STRFREE( user );
   DISPOSE( when );
   STRFREE( host );
   connlist.remove( this );
}

/* Checks an entry for validity. Removes an invalid entry. */
/* This could possibly be useless, as the code that handles updating the file
 * already does most of this. Why not be extra-safe though? */
int check_conn_entry( conn_data * conn )
{
   if( !conn )
      return CHK_CONN_REMOVED;

   if( !conn->user || !conn->host || !conn->when )
   {
      deleteptr( conn );
      bug( "%s: Removed bugged conn entry!", __FUNCTION__ );
      return CHK_CONN_REMOVED;
   }
   return CHK_CONN_OK;
}

void fread_connhist( conn_data * conn, FILE * fp )
{
   for( ;; )
   {
      const char *word = feof( fp ) ? "End" : fread_word( fp );

      switch ( UPPER( word[0] ) )
      {
         default:
            bug( "%s: no match: %s", __FUNCTION__, word );
            fread_to_eol( fp );
            break;

         case '*':
            fread_to_eol( fp );
            break;

         case 'E':
            if( !str_cmp( word, "End" ) )
               return;
            break;

         case 'H':
            KEY( "Host", conn->host, fread_string( fp ) );
            break;

         case 'I':
            KEY( "Invis", conn->invis_lvl, fread_number( fp ) );
            break;

         case 'L':
            KEY( "Level", conn->level, fread_number( fp ) );
            break;

         case 'T':
            KEY( "Type", conn->type, fread_number( fp ) );
            break;

         case 'U':
            KEY( "User", conn->user, fread_string( fp ) );
            break;

         case 'W':
            KEY( "When", conn->when, fread_string_nohash( fp ) );
            break;
      }
   }
}

/* Loads the conn.hist file into memory */
void load_connhistory( void )
{
   conn_data *conn;
   FILE *fp;
   int conncount;

   connlist.clear();

   if( ( fp = fopen( CH_FILE, "r" ) ) != NULL )
   {
      conncount = 0;
      for( ;; )
      {
         char letter;
         char *word;

         letter = fread_letter( fp );
         if( letter == '*' )
         {
            fread_to_eol( fp );
            continue;
         }

         if( letter != '#' )
         {
            bug( "%s: # not found.", __FUNCTION__ );
            break;
         }

         word = fread_word( fp );
         if( !str_cmp( word, "CONN" ) )
         {
            if( conncount >= MAX_CONNHISTORY )
            {
               bug( "%s: more connection histories than MAX_CONNHISTORY %d", __FUNCTION__, MAX_CONNHISTORY );
               FCLOSE( fp );
               return;
            }
            conn = new conn_data;
            fread_connhist( conn, fp );
            ++conncount;
            connlist.push_back( conn );
            continue;
         }
         else if( !str_cmp( word, "END" ) )
            break;
         else
         {
            bug( "%s: bad section: %s", __FUNCTION__, word );
            continue;
         }
      }
      FCLOSE( fp );
   }
   return;
}

/* Saves the conn.hist file */
void save_connhistory( void )
{
   FILE *tempfile;
   list<conn_data*>::iterator iconn;

   if( connlist.empty() )
      return;

   if( !( tempfile = fopen( CH_FILE, "w" ) ) )
   {
      bug( "%s: Error opening '%s'", __FUNCTION__, CH_FILE );
      return;
   }

   for( iconn = connlist.begin(); iconn != connlist.end(); ++iconn )
   {
      conn_data *conn = (*iconn);

      /*
       * Only save OK conn entries 
       */
      if( ( check_conn_entry( conn ) ) == CHK_CONN_OK )
      {
         fprintf( tempfile, "%s", "#CONN\n" );
         fprintf( tempfile, "User   %s~\n", conn->user );
         fprintf( tempfile, "When   %s~\n", conn->when );
         fprintf( tempfile, "Host   %s~\n", conn->host );
         fprintf( tempfile, "Level  %d\n", conn->level );
         fprintf( tempfile, "Type   %d\n", conn->type );
         fprintf( tempfile, "Invis  %d\n", conn->invis_lvl );
         fprintf( tempfile, "%s", "End\n\n" );
      }
   }
   fprintf( tempfile, "%s", "#END\n" );
   FCLOSE( tempfile );
   return;
}

/* Frees all the histories from memory.
 * If Arg == 1, then it also deletes the conn.hist file
 */
void free_connhistory( int arg )
{
   list<conn_data*>::iterator iconn;

   for( iconn = connlist.begin(); iconn != connlist.end(); )
   {
      conn_data *con = (*iconn);
      ++iconn;

      deleteptr( con );
   }

   if( arg == 1 )
      unlink( CH_FILE );

   return;
}

/* NeoCode 0.08 Revamp of this! I now base it off of descriptor_data.
 * It will pull needed data from the descriptor. If there's no character
 * than it will use default information. -- X
 */
void update_connhistory( descriptor_data * d, int type )
{
   list<conn_data*>::iterator conn;
   conn_data *con;
   char_data *vch;
   struct tm *local;
   time_t t;

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
   if( (int)connlist.size() >= MAX_CONNHISTORY )
   {
      con = *connlist.begin();
      connlist.erase( connlist.begin() );
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
   con->user = vch->name ? STRALLOC( vch->name ) : STRALLOC( "NoName" );
   strdup_printf( &con->when,
                  "%-2.2d/%-2.2d %-2.2d:%-2.2d", local->tm_mon + 1, local->tm_mday, local->tm_hour, local->tm_min );
   con->host = d->host ? STRALLOC( d->host ) : STRALLOC( "unknown" );
   con->type = type;
   con->level = vch->level;
   con->invis_lvl = vch->has_pcflag( PCFLAG_WIZINVIS ) ? vch->pcdata->wizinvis : 0;
   connlist.push_back( con );
   save_connhistory(  );
   return;
}

/* The logins command */
/* Those who are equal or greather than the CH_LVL_ADMIN defined in connhist.h
 * can also prompt a complete purging of the histories and the conn.hist file. */
CMDF( do_logins )
{
   int conn_count = 0;
   list<conn_data*>::iterator iconn;
   char user[MSL], typebuf[MSL];

   if( ( argument ) && ( !str_cmp( argument, "clear" ) && ch->level >= CH_LVL_ADMIN ) )
   {
      ch->print( "Clearing Connection history...\r\n" );
      free_connhistory( 1 );  /* Remember - Arg must = 1 to wipe the file on disk! -- X */
      return;
   }

   /*
    * Modify this line to fit your tastes 
    */
   ch->printf( "&c----[&WConnection History for %s&c]----&w\r\n", sysdata->mud_name );

   for( iconn = connlist.begin(); iconn != connlist.end(); ++iconn )
   {
      conn_data *conn = (*iconn);

      if( ( check_conn_entry( conn ) ) != CHK_CONN_OK )
         continue;

      if( conn->invis_lvl <= ch->level )
      {
         ++conn_count;
         switch ( conn->type )
         {
            case CONNTYPE_LOGIN:
               mudstrlcpy( typebuf, " has logged in.", MSL );
               break;
            case CONNTYPE_QUIT:
               mudstrlcpy( typebuf, " has logged out.", MSL );
               break;
            case CONNTYPE_IDLE:
               mudstrlcpy( typebuf, " has been logged out due to inactivity.", MSL );
               break;
            case CONNTYPE_LINKDEAD:
               mudstrlcpy( typebuf, " has lost their link.", MSL );
               break;
            case CONNTYPE_NEWPLYR:
               mudstrlcpy( typebuf, " has logged in for the first time!", MSL );
               break;
            case CONNTYPE_RECONN:
               mudstrlcpy( typebuf, " has reconnected.", MSL );
               break;
            default:
               mudstrlcpy( typebuf, ".", MSL );
               break;
         }

         /*
          * If a player is wizinvis, and an immortal can see them, then tell that immortal
          * * what invis level they were. Note: change color for the Invis tag here.
          */
         if( conn->invis_lvl > 0 && ch->is_immortal(  ) )
            snprintf( user, MSL, "(&cInvis &p%d&w) %s", conn->invis_lvl, conn->user );
         else
            mudstrlcpy( user, conn->user, MSL );

         /*
          * The format for the history are these two lines below. First is for Immortals, second for players. 
          * If you know what you're doing, than you can modify the output here 
          */
         if( ch->is_immortal(  ) )
            ch->printf( "&c[&O%s&c] &w%s&g@&w%s&c%s&w\r\n", conn->when, user, conn->host, typebuf );
         else
            ch->printf( "&c[&O%s&c] &w%s&c%s&w\r\n", conn->when, user, typebuf );
      }
   }

   if( !conn_count )
      ch->print( "&WNo Data.&w\r\n" );

   if( ch->level >= CH_LVL_ADMIN )
      ch->print( "\r\nAdmin: You may type LOGIN CLEAR to wipe the current history.\r\n" );
   return;
}
