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
 * Original SMAUG 1.8b written by Thoric (Derek Snider) with Altrag,        *
 * Blodkai, Haus, Narn, Scryn, Swordbearer, Tricops, Gorog, Rennard,        *
 * Grishnakh, Fireblade, Edmond, Conran, and Nivek.                         *
 *                                                                          *
 * Original MERC 2.1 code by Hatchet, Furey, and Kahn.                      *
 *                                                                          *
 * Original DikuMUD code by: Hans Staerfeldt, Katja Nyboe, Tom Madsen,      *
 * Michael Seifert, and Sebastian Hammer.                                   *
 ****************************************************************************
 *                  Noplex's Immhost Verification Module                    *
 ****************************************************************************/

/*******************************************************
		Crimson Blade Codebase
	Copyright 2000-2002 Noplex (John Bellone)
	      http://www.crimsonblade.org
		admin@crimsonblade.org
		Coders: Noplex, Krowe
		 Based on Smaug 1.4a
*******************************************************/

/*
======================
Advanced Immortal Host
======================
By Noplex with help from Senir and Samson
*/

#include "mud.h"
#include "descriptor.h"
#include "imm_host.h"

list < immortal_host * >hostlist;

immortal_host_log::immortal_host_log(  )
{
}

immortal_host_log::~immortal_host_log(  )
{
}

immortal_host::immortal_host(  )
{
   loglist.clear(  );
}

immortal_host::~immortal_host(  )
{
   list < immortal_host_log * >::iterator hlog;

   for( hlog = loglist.begin(  ); hlog != loglist.end(  ); )
   {
      immortal_host_log *ilog = *hlog;
      ++hlog;

      loglist.remove( ilog );
      deleteptr( ilog );
   }
   hostlist.remove( this );
}

void free_immhosts()
{
   list < immortal_host * >::iterator ihost;

   for( ihost = hostlist.begin(  ); ihost != hostlist.end(  ); )
   {
      immortal_host *host = *ihost;
      ++ihost;

      hostlist.remove( host );
      deleteptr( host );
   }
}

immortal_host_log *fread_imm_host_log( FILE * fp )
{
   immortal_host_log *hlog = new immortal_host_log;

   for( ;; )
   {
      const char *word = ( feof( fp ) ? "LEnd" : fread_word( fp ) );

      if( word[0] == '\0' )
      {
         bug( "%s: EOF encountered reading file!", __func__ );
         word = "LEnd";
      }

      switch ( UPPER( word[0] ) )
      {
         default:
            bug( "%s: no match: %s", __func__, word );
            fread_to_eol( fp );
            break;

         case '*':
            fread_to_eol( fp );
            break;

         case 'L':
            if( !str_cmp( word, "LEnd" ) )
            {
               if( !hlog->date.empty(  ) && !hlog->host.empty(  ) )
                  return hlog;

               deleteptr( hlog );
               return NULL;
            }
            STDSKEY( "Log_Date", hlog->date );
            STDSKEY( "Log_Host", hlog->host );
            break;
      }
   }
}

immortal_host *fread_imm_host( FILE * fp )
{
   immortal_host *host = new immortal_host;
   immortal_host_log *nlog = NULL;
   short dnum = 0;

   for( ;; )
   {
      const char *word = ( feof( fp ) ? "ZEnd" : fread_word( fp ) );

      if( word[0] == '\0' )
      {
         bug( "%s: EOF encountered reading file!", __func__ );
         word = "ZEnd";
      }

      switch ( UPPER( word[0] ) )
      {
         default:
            bug( "%s: no match: %s", __func__, word );
            fread_to_eol( fp );
            break;

         case '*':
            fread_to_eol( fp );
            break;

         case 'D':
            if( !str_cmp( word, "Domain_host" ) )
            {
               if( dnum >= MAX_DOMAIN )
                  bug( "%s: more saved domains than MAX_DOMAIN", __func__ );
               else
               {
                  dnum++;
                  fread_string( host->domain[dnum], fp );
               }
               break;
            }
            break;

         case 'L':
            if( !str_cmp( word, "LOG" ) )
            {
               if( !( nlog = fread_imm_host_log( fp ) ) )
                  bug( "%s: incomplete log returned", __func__ );
               else
                  host->loglist.push_back( nlog );

               break;
            }
            break;

         case 'N':
            STDSKEY( "Name", host->name );
            break;

         case 'Z':
            if( !str_cmp( word, "ZEnd" ) )
            {
               if( host->name.empty(  ) || host->domain[0].empty(  ) )
               {
                  deleteptr( host );
                  return NULL;
               }
               return host;
            }
            break;
      }
   }
}

void load_imm_host( void )
{
   FILE *fp;

   hostlist.clear(  );

   if( !( fp = fopen( IMM_HOST_FILE, "r" ) ) )
   {
      bug( "%s: could not open immhost file for reading", __func__ );
      return;
   }

   for( ;; )
   {
      char letter = fread_letter( fp );
      char *word;

      if( letter == '*' )
      {
         fread_to_eol( fp );
         continue;
      }

      if( letter != '#' )
      {
         bug( "%s: # not found", __func__ );
         break;
      }

      word = fread_word( fp );

      if( !str_cmp( word, "IMMORTAL" ) )
      {
         immortal_host *host = NULL;

         if( !( host = fread_imm_host( fp ) ) )
         {
            bug( "%s: incomplete immhost", __func__ );
            continue;
         }
         hostlist.push_back( host );
         continue;
      }
      else if( !str_cmp( word, "END" ) )
         break;
      else
      {
         bug( "%s: unknown section %s", __func__, word );
         continue;
      }
   }
   FCLOSE( fp );
}

void save_imm_host( void )
{
   FILE *fp;
   list < immortal_host * >::iterator ihost;

   if( !( fp = fopen( IMM_HOST_FILE, "w" ) ) )
   {
      bug( "%s: could not open immhost file for writing", __func__ );
      return;
   }

   for( ihost = hostlist.begin(  ); ihost != hostlist.end(  ); ++ihost )
   {
      immortal_host *host = *ihost;
      short dnum = 0;

      fprintf( fp, "%s", "\n#IMMORTAL\n" );
      fprintf( fp, "Name        %s~\n", host->name.c_str(  ) );

      for( dnum = 0; dnum < MAX_DOMAIN && !host->domain[dnum].empty(  ); ++dnum )
         fprintf( fp, "Domain_Host %s~\n", host->domain[dnum].c_str(  ) );

      list < immortal_host_log * >::iterator ilog;
      for( ilog = host->loglist.begin(  ); ilog != host->loglist.end(  ); ++ilog )
      {
         immortal_host_log *nlog = *ilog;

         fprintf( fp, "%s", "LOG\n" );
         fprintf( fp, "Log_Host    %s~\n", nlog->host.c_str(  ) );
         fprintf( fp, "Log_Date    %s~\n", nlog->date.c_str(  ) );
         fprintf( fp, "%s", "LEnd\n" );
      }
      fprintf( fp, "%s", "ZEnd\n" );
   }
   fprintf( fp, "%s", "#END\n" );
   FCLOSE( fp );
}

bool check_immortal_domain( char_data * ch, const string & lhost )
{
   list < immortal_host * >::iterator ihost;
   immortal_host *host = NULL;
   bool found = false;

   for( ihost = hostlist.begin(  ); ihost != hostlist.end(  ); ++ihost )
   {
      host = *ihost;

      if( !str_cmp( host->name, ch->name ) )
      {
         found = true;
         break;
      }
   }

   /*
    * no immortal host or no domains 
    */
   if( !found || host->domain[0].empty(  ) )
      return true;

   /*
    * check if the domain is valid 
    */
   for( short x = 0; x < MAX_DOMAIN && !host->domain[x].empty(  ); ++x )
   {
      bool suffix = false, prefix = false;
      char chost[50];
      short s = 0, t = 0;

      if( host->domain[x][0] == '*' )
      {
         prefix = true;
         t = 1;
      }

      while( host->domain[x][t] != '\0' )
         chost[s++] = host->domain[x][t++];

      chost[s] = '\0';

      if( chost[strlen( chost ) - 1] == '*' )
      {
         chost[strlen( chost ) - 1] = '\0';
         suffix = true;
      }

      if( ( prefix && suffix && !str_infix( ch->desc->host, chost ) )
          || ( prefix && !str_suffix( chost, ch->desc->host.c_str(  ) ) ) || ( suffix && !str_prefix( chost, ch->desc->host ) ) || ( !str_cmp( chost, ch->desc->host ) ) )
      {
         log_printf( "&C&GImmotal_Host: %s's host authorized.", ch->name );
         return true;
      }
   }

   /*
    * denied attempts now get logged 
    */
   log_printf( "&C&RImmortal_Host: %s's host denied. This hacking attempt has been logged.", ch->name );

   immortal_host_log *nlog = new immortal_host_log;
   nlog->host = lhost;
   nlog->date = c_time( current_time, -1 );
   host->loglist.push_back( nlog );

   save_imm_host(  );

   ch->print( "You have been caught attempting to hack an immortal's character and have been logged.\r\n" );

   return false;
}

CMDF( do_immhost )
{
   immortal_host *host = NULL;
   list < immortal_host * >::iterator ihost;
   list < immortal_host_log * >::iterator ilog;
   string arg, arg2;
   short x = 0;

   if( ch->isnpc(  ) || !ch->is_immortal(  ) )
   {
      ch->print( "Huh?\r\n" );
      return;
   }

   argument = one_argument( argument, arg );

   if( arg.empty(  ) )
   {
      ch->print( "&C&RSyntax: &Gimmhost list\r\n"
                 "&RSyntax: &Gimmhost add <&rcharacter&G>\r\n"
                 "&RSyntax: &Gimmhost remove <&rcharacter&G>\r\n"
                 "&RSyntax: &Gimmhost viewlogs <&rcharacter&G>\r\n"
                 "&RSyntax: &Gimmhost removelog <&rcharacter&G> <&rlog number&G>\r\n"
                 "&RSyntax: &Gimmhost viewdomains <&rcharacter&G>\r\n"
                 "&RSyntax: &Gimmhost createdomain <&rcharacter&G> <&rhost&G>\r\n" "&RSyntax: &Gimmhost removedomain <&rcharacter&G> <&rdomain number&G>\r\n" );
      return;
   }

   if( !str_cmp( arg, "list" ) )
   {
      if( hostlist.empty(  ) )
      {
         ch->print( "No immortals are currently protected at this time.\r\n" );
         return;
      }

      ch->pager( "&C&R[&GName&R]     [&GDomains&R]  [&GLogged Attempts&R]\r\n" );

      for( ihost = hostlist.begin(  ); ihost != hostlist.end(  ); ++ihost, ++x )
      {
         host = *ihost;
         short dnum = 0;

         while( dnum < MAX_DOMAIN && !host->domain[dnum].empty(  ) )
            ++dnum;

         ch->pagerf( "&C&G%-10s %-10d %d\r\n", host->name.c_str(  ), dnum, host->loglist.size(  ) );
      }
      ch->pagerf( "&C&R%d immortals are being protected.&g\r\n", x );
      return;
   }

   argument = one_argument( argument, arg2 );

   if( arg2.empty(  ) )
   {
      ch->print( "Which character would you like to use?\r\n" );
      return;
   }

   if( !str_cmp( arg, "add" ) )
   {
      host = new immortal_host;

      smash_tilde( arg2 );
      host->name = capitalize( arg2 );
      hostlist.push_back( host );
      save_imm_host(  );
      ch->print( "Immortal host added.\r\n" );
      return;
   }

   bool found = false;
   for( ihost = hostlist.begin(  ); ihost != hostlist.end(  ); ++ihost )
   {
      host = *ihost;
      if( !str_cmp( host->name, arg2 ) )
      {
         found = true;
         break;
      }
   }

   if( !found )
   {
      ch->print( "There is no immortal host with that name.\r\n" );
      return;
   }

   if( !str_cmp( arg, "remove" ) )
   {
      deleteptr( host );
      save_imm_host(  );
      ch->print( "Immortal host removed.\r\n" );
      return;
   }

   if( !str_cmp( arg, "viewlogs" ) )
   {
      if( host->loglist.empty(  ) )
      {
         ch->print( "There are no logs for this immortal host.\r\n" );
         return;
      }

      ch->pagerf( "&C&RImmortal:&W %s\r\n", host->name.c_str(  ) );
      ch->pager( "&R[&GNum&R]  [&GLogged Host&R]     [&GDate&R]\r\n" );

      for( ilog = host->loglist.begin(  ); ilog != host->loglist.end(  ); ++ilog )
      {
         immortal_host_log *hlog = *ilog;
         ch->pagerf( "&C&G%-6d %-17s %s\r\n", ++x, hlog->host.c_str(  ), hlog->date.c_str(  ) );
      }
      ch->pagerf( "&C&R%d logged hacking attempts.&g\r\n", x );
      return;
   }

   if( !str_cmp( arg, "removelog" ) )
   {
      immortal_host_log *hlog = NULL;

      if( argument.empty(  ) || !is_number( argument ) )
      {
         ch->print( "Syntax: immhost removelog <character> <log number>\r\n" );
         return;
      }

      found = false;
      for( ilog = host->loglist.begin(  ); ilog != host->loglist.end(  ); ++ilog )
      {
         hlog = *ilog;
         if( ++x == atoi( argument.c_str(  ) ) )
         {
            found = true;
            break;
         }
      }

      if( !found )
      {
         ch->print( "That immortal host doesn't have a log with that number.\r\n" );
         return;
      }
      host->loglist.remove( hlog );
      deleteptr( hlog );
      save_imm_host(  );
      ch->print( "Log removed.\r\n" );
      return;
   }

   if( !str_cmp( arg, "viewdomains" ) )
   {
      ch->pager( "&C&R[&GNum&R]  [&GHost&R]\r\n" );

      for( x = 0; x < MAX_DOMAIN && !host->domain[x].empty(  ); ++x )
         ch->pagerf( "&C&G%-5d  %s\r\n", x + 1, host->domain[x].c_str(  ) );

      ch->pagerf( "&C&R%d immortal domains.&g\r\n", x );
      return;
   }

   if( !str_cmp( arg, "createdomain" ) )
   {
      if( argument.empty(  ) )
      {
         ch->print( "Syntax: immhost createdomain <character> <host>\r\n" );
         return;
      }

      smash_tilde( argument );

      for( x = 0; x < MAX_DOMAIN && !host->domain[x].empty(  ); ++x )
      {
         if( !str_cmp( argument, host->domain[x] ) )
         {
            ch->print( "That immortal host already has an entry like that.\r\n" );
            return;
         }
      }

      if( x == MAX_DOMAIN )
      {
         ch->pagerf( "This immortal host has the maximum allowed, %d domains.\r\n", MAX_DOMAIN );
         return;
      }

      host->domain[x] = argument;

      save_imm_host(  );
      ch->print( "Done.\r\n" );
      return;
   }

   if( !str_cmp( arg, "removedomain" ) )
   {
      if( argument.empty(  ) || !is_number( argument ) )
      {
         ch->print( "Syntax: immhost removedomain <character> <domain number>\r\n" );
         return;
      }

      x = URANGE( 1, atoi( argument.c_str(  ) ), MAX_DOMAIN );
      --x;

      if( host->domain[x].empty(  ) )
      {
         ch->print( "That immortal host doesn't have a domain with that number.\r\n" );
         return;
      }

      host->domain[x].clear(  );

      save_imm_host(  );
      ch->print( "Domain removed.\r\n" );
      return;
   }
   do_immhost( ch, "" );
}
