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
 *                  Noplex's Immhost Verification Module                    *
 ****************************************************************************/

/*******************************************************
		Crimson Blade Codebase
	Copyright 2000-2002 Noplex (John Bellone)
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

#include <filesystem>
#include <fstream>
#include "mud.h"
#include "descriptor.h"
#include "imm_host.h"

std::list<immortal_host *> hostlist;

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
   for( auto it = loglist.begin(); it != loglist.end(); )
   {
      immortal_host_log *ilog = *it;
      ++it;

      loglist.remove( ilog );
      deleteptr( ilog );
   }
   hostlist.remove( this );
}

void free_immhosts()
{
   for( auto it = hostlist.begin(); it != hostlist.end(); )
   {
      immortal_host *host = *it;
      ++it;

      hostlist.remove( host );
      deleteptr( host );
   }
}

immortal_host_log *fread_imm_host_log( std::ifstream & stream )
{
   immortal_host_log *hlog = new immortal_host_log;

   std::string key;
   while( stream >> key )
   {
      if( key == "Log_Host" )
         hlog->host = fread_line( stream );
      else if( key == "Log_Date" )
         hlog->date = fread_line( stream );
      else if( key =="Lend" )
      {
         if( !hlog->date.empty(  ) && !hlog->host.empty(  ) )
            return hlog;

         deleteptr( hlog );
         return nullptr;
      }
   }
   bug( "{}: Fell through to bottom. Corrupted file.", __func__ );
   std::exit( EXIT_FAILURE );
}

void fread_imm_host( immortal_host * host, std::ifstream & stream )
{
   short dnum = 0;
   std::string key;
   while( stream >> key )
   {
      if( key == "Name" )
         host->name = fread_line( stream );
      else if( key =="Domain_Host" )
      {
         if( dnum >= MAX_DOMAIN )
            bug( "{}: more saved domains than MAX_DOMAIN", __func__ );
         else
         {
            dnum++;
            host->domain[dnum] = fread_line( stream );
         }
      }
      else if( key == "#LOG" )
      {
         immortal_host_log *nlog;

         if( !( nlog = fread_imm_host_log( stream ) ) )
            bug( "{}: incomplete log returned", __func__ );
         else
            host->loglist.push_back( nlog );
      }
      else if( key == "ZEnd" )
      {
         if( host->name.empty(  ) || host->domain[0].empty(  ) )
         {
            bug( "{}: Bad entry in host data.", __func__ );
            deleteptr( host );
            return;
         }
         hostlist.push_back( host );
         return;
      }

   }
}

void load_imm_host( void )
{
   std::ifstream stream( std::filesystem::path{IMM_HOST_FILE} );
   if( !stream.is_open() )
   {
      bug( "{}: Cannot open {} for reading: {}", __func__, IMM_HOST_FILE, std::strerror(errno) );
      return;
   }

   hostlist.clear(  );

   immortal_host *host = nullptr;

   std::string key;
   while( stream >> key )
   {
      if( key ==  "#IMMORTAL" )
      {
         host = new immortal_host;

         fread_imm_host( host, stream );
      }
      else if( key == "#END" )
         break;
      else
      {
         bug( "{}: Bad section '{}' in {} - skipping.", __func__, key, IMM_HOST_FILE );
         fread_to_eol( stream );
      }
   }
   stream.close();
}

void save_imm_host( void )
{
   std::ofstream stream( std::filesystem::path{IMM_HOST_FILE} );
   if( !stream.is_open() )
   {
      bug( "{}: Cannot open {} for writing: {}", __func__, IMM_HOST_FILE, std::strerror(errno) );
      return;
   }

   for( auto* host : hostlist )
   {
      stream << "#IMMORTAL\n";
      stream << std::format( "Name        {}~\n", host->name );

      for( short dnum = 0; dnum < MAX_DOMAIN && !host->domain[dnum].empty(); ++dnum )
         stream << std::format( "Domain_Host {}~\n", host->domain[dnum] );

      for( auto* nlog : host->loglist )
      {
         stream << "#LOG\n";
         stream << std::format( "Log_Host    {}~\n", nlog->host );
         stream << std::format( "Log_Date    {}~\n", nlog->date );
         stream << "LEnd\n";
      }
      stream << "ZEnd\n\n";
   }
   stream << "#END\n";
   if( stream.fail() )
      bug( "{}: Error occurred after closing {}: ", __func__, IMM_HOST_FILE, std::strerror(errno) );
}

bool check_immortal_domain( char_data * ch, std::string_view lhost )
{
   std::list<immortal_host *>::iterator ihost;
   immortal_host *host = nullptr;
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

      if( ( prefix && suffix && !str_infix( ch->desc->ipaddress, chost ) )
          || ( prefix && !str_suffix( chost, ch->desc->ipaddress ) ) || ( suffix && !str_prefix( chost, ch->desc->ipaddress ) ) || ( !str_cmp( chost, ch->desc->ipaddress ) ) )
      {
         log_printf( "&C&GImmotal_Host: {}'s host authorized.", ch->name );
         return true;
      }
   }

   /*
    * denied attempts now get logged 
    */
   log_printf( "&C&RImmortal_Host: {}'s host denied. This hacking attempt has been logged.", ch->name );

   immortal_host_log *nlog = new immortal_host_log;
   nlog->host = lhost;
   nlog->date = c_time( current_time, "" );
   host->loglist.push_back( nlog );

   save_imm_host(  );

   ch->print( "You have been caught attempting to hack an immortal's character and have been logged.\r\n" );

   return false;
}

CMDF( do_immhost )
{
   immortal_host *host = nullptr;
   std::list<immortal_host *>::iterator ihost;
   std::list<immortal_host_log *>::iterator ilog;
   std::string arg, arg2;
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

         ch->pager_fmt( "&C&G{:<10} {:<10} {}\r\n", host->name, dnum, host->loglist.size(  ) );
      }
      ch->pager_fmt( "&C&R{} immortals are being protected.&g\r\n", x );
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

      ch->pager_fmt( "&C&RImmortal:&W {}\r\n", host->name );
      ch->pager( "&R[&GNum&R]  [&GLogged Host&R]     [&GDate&R]\r\n" );

      for( ilog = host->loglist.begin(  ); ilog != host->loglist.end(  ); ++ilog )
      {
         immortal_host_log *hlog = *ilog;
         ch->pager_fmt( "&C&G{:<6} {:<17} {}\r\n", ++x, hlog->host, hlog->date );
      }
      ch->pager_fmt( "&C&R{} logged hacking attempts.&g\r\n", x );
      return;
   }

   if( !str_cmp( arg, "removelog" ) )
   {
      immortal_host_log *hlog = nullptr;

      if( argument.empty(  ) || !is_number( argument ) )
      {
         ch->print( "Syntax: immhost removelog <character> <log number>\r\n" );
         return;
      }

      found = false;
      for( ilog = host->loglist.begin(  ); ilog != host->loglist.end(  ); ++ilog )
      {
         hlog = *ilog;
         if( ++x == std::stoi( argument ) )
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
         ch->pager_fmt( "&C&G{:<5}  {}\r\n", x + 1, host->domain[x] );

      ch->pager_fmt( "&C&R{} immortal domains.&g\r\n", x );
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
         ch->pager_fmt( "This immortal host has the maximum allowed, {} domains.\r\n", MAX_DOMAIN );
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

      x = urange( 1, std::stoi( argument ), MAX_DOMAIN );
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
