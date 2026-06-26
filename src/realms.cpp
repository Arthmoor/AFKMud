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
 *                         Immortal Realms Module                           *
 ****************************************************************************/

#include <filesystem>
#include <fstream>
#include <sstream>
#include "mud.h"
#include "realms.h"

std::list<realm_data *> realmlist;

const char *realm_type_names[MAX_REALM] = {
   "None", "Quest", "PR", "QA", "Builder", "Coder", "Admin", "Owner"
};

int get_realm_type_name( std::string_view flag )
{
   for( size_t x = 0; x < ( sizeof( realm_type_names ) / sizeof( realm_type_names[0] ) ); ++x )
      if( !str_cmp( flag, realm_type_names[x] ) )
         return x;
   return -1;
}

realm_roster_data::realm_roster_data(  )
{
}

realm_roster_data::~realm_roster_data(  )
{
}

void add_realm_roster( realm_data * realm, std::string_view name )
{
   realm_roster_data *roster;

   roster = new realm_roster_data;
   roster->name = name;
   roster->joined = current_time;
   realm->memberlist.push_back( roster );
}

void remove_realm_roster( realm_data * realm, std::string_view name )
{
   for( auto* member : realm->memberlist )
   {
      if( !str_cmp( name, member->name ) )
      {
         realm->memberlist.remove( member );
         deleteptr( member );
         return;
      }
   }
}

/* For use during realm removal and memory cleanup */
void remove_all_realm_rosters( realm_data * realm )
{
   for( auto it = realm->memberlist.begin(  ); it != realm->memberlist.end(  ); )
   {
      realm_roster_data *roster = *it;
      ++it;

      realm->memberlist.remove( roster );
      deleteptr( roster );
   }
}

realm_data::realm_data(  )
{
}

realm_data::~realm_data(  )
{
   remove_all_realm_rosters( this );
   memberlist.clear(  );
   realmlist.remove( this );
}

/*
 * Get pointer to realm structure from realm name.
 */
realm_data *get_realm( std::string_view name )
{
   for( auto* realm : realmlist )
   {
      if( !str_cmp( name, realm->name ) )
         return realm;
   }
   return nullptr;
}

void free_realms( void )
{
   for( auto it = realmlist.begin(  ); it != realmlist.end(  ); )
   {
      realm_data *realm = *it;
      ++it;

      deleteptr( realm );
   }
}

bool IS_REALM_LEADER( char_data * ch )
{
   if( !ch->isnpc() && ch->pcdata->realm && !str_cmp( ch->name, ch->pcdata->realm->leader ) )
      return true;
   return false;
}

bool IS_ADMIN_REALM( char_data * ch )
{
   if( !ch->isnpc() && ch->pcdata->realm && ( ch->pcdata->realm->type == REALM_ADMIN || ch->pcdata->realm->type == REALM_OWNER ) )
      return true;
   return false;
}

bool IS_OWNER_REALM( char_data * ch )
{
   if( !ch->isnpc() && ch->pcdata->realm && ch->pcdata->realm->type == REALM_OWNER )
      return true;
   return false;
}

void delete_realm( char_data * ch, realm_data * realm )
{
   std::string realmname = realm->name;
   std::filesystem::path filename = realm->filename;

   for( auto* vch : pclist )
   {
      if( !vch->pcdata->realm )
         continue;

      if( vch->pcdata->realm == realm )
      {
         vch->pcdata->realm_name.clear(  );
         vch->pcdata->realm = nullptr;
         vch->print_fmt( "The realm known as &W{}&D has been destroyed by the gods!\r\n", realm->name );
      }
   }

   realmlist.remove( realm );
   deleteptr( realm );

   if( !ch )
   {
      if( std::filesystem::remove( filename ) )
         log_printf( "Realm data for {} destroyed - no members left.", realmname );
      return;
   }

   if( std::filesystem::remove( filename ) )
   {
      ch->print_fmt( "&RRealm data for {} has been destroyed.\r\n", realmname );
      log_printf( "Realm data for {} has been destroyed by {}.", realmname.c_str(  ), ch->name.c_str() );
   }
}

void write_realm_list( void )
{
   std::ofstream stream;

   std::filesystem::path filename = std::format( "{}{}", REALM_DIR, REALM_LIST );
   stream.open( filename );
   if( !stream.is_open(  ) )
   {
      bug( "{}: Cannot open {} for writing: {}", __func__, filename.string(), std::strerror(errno) );
      return;
   }

   for( auto* realm : realmlist )
      stream << std::format( "{}\n", realm->filename );

   stream << "$\n";
   stream.close();

   if( stream.fail() )
      bug( "{}: Error occurred after closing {}: ", __func__, filename.string(), std::strerror(errno) );
}

void fwrite_realm_memberlist( FILE * fp, realm_data * realm )
{
   for( auto* member : realm->memberlist )
   {
      auto joined = std::chrono::system_clock::to_time_t( member->joined );

      fprintf( fp, "%s", "#ROSTER\n" );
      fprintf( fp, "Name      %s~\n", member->name.c_str(  ) );
      fprintf( fp, "Joined    %ld\n", joined );
      fprintf( fp, "%s", "End\n\n" );
   }
}

void fread_realm_memberlist( realm_data * realm, FILE * fp )
{
   realm_roster_data *roster;

   roster = new realm_roster_data;

   for( ;; )
   {
      std::string word = feof( fp ) ? "End" : fread_word( fp );

      switch ( to_upper( word[0] ) )
      {
         default:
            bug( "{}: no match: {}", __func__, word );
            fread_to_eol( fp );
            break;

         case '*':
            fread_to_eol( fp );
            break;

         case 'E':
            if( !str_cmp( word, "End" ) )
            {
               realm->memberlist.push_back( roster );
               return;
            }
            break;

         case 'J':
            if( !str_cmp( word, "Joined" ) )
            {
               time_t loaded_time = fread_long( fp );
               roster->joined = std::chrono::system_clock::from_time_t( loaded_time );
            }
            break;

         case 'N':
            STDSKEY( "Name", roster->name );
            break;
      }
   }
}

/*
 * Save a realm's data to its data file
 */
constexpr int REALM_VERSION = 1;
void save_realm( realm_data * realm )
{
   FILE *fp;

   if( !realm )
   {
      bug( "{}: null realm pointer!", __func__ );
      return;
   }

   if( realm->filename.empty(  ) )
   {
      bug( "{}: {} has no filename", __func__, realm->name );
      return;
   }

   std::filesystem::path filename = std::format( "{}{}", REALM_DIR, realm->filename );

   if( !( fp = fopen( filename.c_str(), "w" ) ) )
   {
      bug( "{}: Cannot open {} for writing.", __func__, filename.string() );
      perror( filename.c_str() );
   }
   else
   {
      fprintf( fp, "%s", "#REALM\n" );
      fprintf( fp, "Version      %d\n", REALM_VERSION );
      fprintf( fp, "Name         %s~\n", realm->name.c_str(  ) );
      fprintf( fp, "Filename     %s~\n", realm->filename.c_str(  ) );
      fprintf( fp, "Description  %s~\n", realm->realmdesc.c_str(  ) );
      fprintf( fp, "Leader       %s~\n", realm->leader.c_str(  ) );
      fprintf( fp, "Leadrank     %s~\n", realm->leadrank.c_str(  ) );
      fprintf( fp, "Badge        %s~\n", realm->badge.c_str(  ) );
      fprintf( fp, "Type         %s~\n", realm_type_names[realm->type] );
      fprintf( fp, "Members      %d\n", realm->members );
      fprintf( fp, "Board        %d\n", realm->board );
      fprintf( fp, "%s", "End\n\n" );
      fwrite_realm_memberlist( fp, realm );
      fprintf( fp, "%s", "#END\n" );
   }
   FCLOSE( fp );
}

/*
 * Read in actual realm data.
 */
void fread_realm( realm_data * realm, FILE * fp )
{
   int file_ver = 0;

   for( ;; )
   {
      std::string word = feof( fp ) ? "End" : fread_word( fp );

      switch ( to_upper( word[0] ) )
      {
         default:
            bug( "{}: no match: {}", __func__, word );
            fread_to_eol( fp );
            break;

         case '*':
            fread_to_eol( fp );
            break;

         case 'B':
            STDSKEY( "Badge", realm->badge );
            KEY( "Board", realm->board, fread_number( fp ) );
            break;

         case 'D':
            STDSKEY( "Description", realm->realmdesc );
            break;

         case 'E':
            if( !str_cmp( word, "End" ) )
            {
               if( file_ver == 0 )
                  ; // Do Nothing. This is just to shut the compiler up about file_ver not yet being used.

               return;
            }
            break;

         case 'F':
            STDSKEY( "Filename", realm->filename );
            break;

         case 'L':
            STDSKEY( "Leader", realm->leader );
            STDSKEY( "Leadrank", realm->leadrank );
            break;

         case 'M':
            KEY( "Members", realm->members, fread_short( fp ) );
            break;

         case 'N':
            STDSKEY( "Name", realm->name );
            break;

         case 'T':
            if( !str_cmp( word, "Type" ) )
            {
               const char *temp = fread_flagstring( fp );
               int value = get_realm_type_name( temp );

               if( value < 0 || value > MAX_REALM )
               {
                  bug( "{}: Invalid realm type: {}. Setting to None.", __func__, temp );
                  value = 0;
               }
               realm->type = value;
            }
            break;

         case 'V':
            KEY( "Version", file_ver, fread_number( fp ) );
            break;
      }
   }
}

void clean_realm( realm_data * realm )
{
   realm->memberlist.clear(  );
   realm->members = 1;
   realm->type = REALM_NONE;
}

/*
 * Load a realm file
 */
bool load_realm_file( std::string_view realmfile )
{
   realm_data *realm;
   FILE *fp;

   realm = new realm_data;

   clean_realm( realm );  /* Default settings so we don't get weird ass stuff */

   bool found = false;
   std::filesystem::path filename = std::format( "{}{}", REALM_DIR, realmfile );

   if( ( fp = fopen( filename.c_str(), "r" ) ) != nullptr )
   {
      found = true;
      for( ;; )
      {
         char letter = fread_letter( fp );
         if( letter == '*' )
         {
            fread_to_eol( fp );
            continue;
         }

         if( letter != '#' )
         {
            bug( "{}: # not found.", __func__ );
            break;
         }

         std::string word = fread_word( fp );
         if( !str_cmp( word, "REALM" ) )
            fread_realm( realm, fp );
         else if( !str_cmp( word, "ROSTER" ) )
            fread_realm_memberlist( realm, fp );
         else if( !str_cmp( word, "END" ) )
            break;
         else
         {
            bug( "{}: bad section: {}.", __func__, word );
            break;
         }
      }
      FCLOSE( fp );
   }

   if( found )
      realmlist.push_back( realm );
   else
      deleteptr( realm );

   return found;
}

void verify_realms( void )
{
   log_string( "Cleaning up realm data..." );
   for( auto it = realmlist.begin(); it != realmlist.end(); )
   {
      realm_data *realm = *it;
      ++it;

      log_printf( "Checking data for {}...", realm->name );

      if( realm->leader.empty(  ) )
      {
         delete_realm( nullptr, realm );
         continue;
      }

      if( realm->members < 1 )
      {
         delete_realm( nullptr, realm );
         continue;
      }

      if( !exists_player( realm->leader ) )
      {
         delete_realm( nullptr, realm );
         continue;
      }

      for( auto it2 = realm->memberlist.begin(); it2 != realm->memberlist.end(); )
      {
         realm_roster_data *roster = *it2;
         ++it2;

         if( !exists_player( roster->name ) )
         {
            log_printf( "{} removed from roster. Player no longer exists.", roster->name );
            remove_realm_roster( realm, roster->name );
         }
      }
      save_realm( realm );
   }
   write_realm_list(  );
}

/*
 * Load in all the realm files.
 */
void load_realms( void )
{
   realmlist.clear(  );

   log_string( "Loading realms..." );

   std::filesystem::path realmlistfile = std::format( "{}{}", REALM_DIR, REALM_LIST );
   std::ifstream stream( std::filesystem::path{realmlistfile} );
   if( !stream.is_open(  ) )
   {
      bug( "{}: Cannot open {} for reading: {}", __func__, realmlistfile.string(), std::strerror(errno) );
      std::exit( EXIT_FAILURE );
   }

   for( ;; )
   {
      std::string filename;
      std::getline( stream, filename, '\n' );

      strip_whitespace( filename );

      if( filename[0] == '$' )
         break;

      log_string( filename );

      if( !load_realm_file( filename ) )
         bug( "{}: Cannot load realm file: {} - {}", __func__, filename, std::strerror(errno) );
   }
   stream.close();

   verify_realms(  ); /* Check against pfiles to see if realms should still exist */
   log_string( "Done realms." );
}

CMDF( do_setrealm )
{
   std::string arg1, arg2;
   realm_data *realm;

   if( ch->isnpc(  ) || !ch->is_immortal(  ) )
   {
      ch->print( "Huh?\r\n" );
      return;
   }

   // is_imp check is here because realms won't exist right away so we need a way in :P
   if( !IS_REALM_LEADER(ch) && !IS_ADMIN_REALM(ch) && !ch->is_imp(  ) )
   {
      ch->print( "Only realm leaders, and members of the admin realm can use this command.\r\n" );
      return;
   }

   ch->set_color( AT_PLAIN );
   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );
   if( arg1.empty(  ) || arg2.empty(  ) )
   {
      ch->print( "Usage: setrealm <field> <value>\r\n" );
      ch->print( "\r\nField being one of:\r\n" );
      ch->print( " leadrank board name desc badge\r\n" );

      if( IS_ADMIN_REALM(ch) || ch->is_imp(  ) )
      {
         ch->print( "\r\nAdminstrative Usage: setrealm <realm> <field> <value>\r\n" );
         ch->print( "\r\nAdminstrative Field being one of:\r\n" );
         ch->print( " type filename leader members delete\r\n" );
      }
      return;
   }

   if( !IS_ADMIN_REALM(ch) && !ch->is_imp(  ) )
   {
      realm = ch->pcdata->realm;

      if( !str_cmp( arg1, "leadrank" ) )
      {
         realm->leadrank = argument;
         ch->print( "Done.\r\n" );
         save_realm( realm );
         return;
      }

      if( !str_cmp( arg1, "badge" ) )
      {
         realm->badge = argument;
         ch->print( "Done.\r\n" );
         save_realm( realm );
         return;
      }

      if( !str_cmp( arg1, "name" ) )
      {
         realm_data *urealm = nullptr;

         if( argument.empty(  ) )
         {
            ch->print( "You can't name a realm nothing.\r\n" );
            return;
         }
         if( ( urealm = get_realm( argument ) ) )
         {
            ch->print( "There is already another realm with that name.\r\n" );
            return;
         }
         realm->name = argument;
         ch->print( "Done.\r\n" );
         save_realm( realm );
         return;
      }

      if( !str_cmp( arg1, "desc" ) )
      {
         realm->realmdesc = argument;
         ch->print( "Done.\r\n" );
         save_realm( realm );
         return;
      }

      do_setrealm( ch, "" );
      return;
   }

   // Admins should end up down here.
   realm = get_realm( arg1 );
   if( !realm )
   {
      ch->print( "No such realm.\r\n" );
      return;
   }

   // Yes, I know, it's redundant as hell.

   if( !str_cmp( arg2, "leader" ) )
   {
      realm->leader = argument;
      ch->print( "Done.\r\n" );
      save_realm( realm );
      return;
   }

   if( !str_cmp( arg2, "leadrank" ) )
   {
      realm->leadrank = argument;
      ch->print( "Done.\r\n" );
      save_realm( realm );
      return;
   }

   if( !str_cmp( arg2, "badge" ) )
   {
      realm->badge = argument;
      ch->print( "Done.\r\n" );
      save_realm( realm );
      return;
   }

   if( !str_cmp( arg2, "name" ) )
   {
      realm_data *urealm = nullptr;

      if( argument.empty(  ) )
      {
         ch->print( "You can't name a realm nothing.\r\n" );
         return;
      }
      if( ( urealm = get_realm( argument ) ) )
      {
         ch->print( "There is already another realm with that name.\r\n" );
         return;
      }
      realm->name = argument;
      ch->print( "Done.\r\n" );
      save_realm( realm );
      return;
   }

   if( !str_cmp( arg2, "desc" ) )
   {
      realm->realmdesc = argument;
      ch->print( "Done.\r\n" );
      save_realm( realm );
      return;
   }

   if( !str_cmp( arg2, "board" ) )
   {
      realm->board = atoi( argument.c_str(  ) );
      ch->print( "Done.\r\n" );
      save_realm( realm );
      return;
   }

   if( !str_cmp( arg2, "type" ) )
   {
      int value = get_realm_type_name( argument );

      if( value < 0 || value >= MAX_REALM )
      {
         ch->print_fmt( "Unknown realm type: {}\r\n", argument );
         return;
      }
      realm->type = value;
      ch->print( "Done.\r\n" );
      save_realm( realm );
      return;
   }

   if( !str_cmp( arg2, "members" ) )
   {
      realm->members = umin( 0, std::stoi( argument ) );
      ch->print( "Done.\r\n" );
      save_realm( realm );
      return;
   }

   if( !str_cmp( arg2, "filename" ) )
   {
      if( !is_valid_filename( ch, REALM_DIR, argument ) )
         return;

      std::filesystem::path filename = std::format( "{}{}", REALM_DIR, realm->filename );
      if( std::filesystem::remove( filename ) )
         ch->print( "Old realm file deleted.\r\n" );

      realm->filename = argument;
      ch->print( "Done.\r\n" );
      save_realm( realm );
      write_realm_list(  );
      return;
   }

   if( !str_cmp( arg2, "delete" ) )
   {
      if( str_cmp( argument, "confirmed" ) )
      {
         ch->print_fmt( "Are you sure? Type &Ysetrealm \"{}\" delete confirmed&D to delete this realm.\r\n", realm->name );
         return;
      }
      delete_realm( ch, realm );
      write_realm_list(  );
      return;
   }

   do_setrealm( ch, "" );
}

CMDF( do_showrealm )
{
   realm_data *realm;

   ch->set_color( AT_PLAIN );

   if( ch->isnpc(  ) )
   {
      ch->print( "Huh?\r\n" );
      return;
   }

   if( argument.empty(  ) )
   {
      ch->print( "Usage: showrealm <realm>\r\n" );
      return;
   }

   realm = get_realm( argument );
   if( !realm )
   {
      ch->print( "No such realm.\r\n" );
      return;
   }

   ch->print_fmt( "\r\n&wRealm    : &W{}\t\t&wBadge: &W{}\t\t&wFilename : &W{}\t\t&wType      : &W{}\r\n",
      realm->name, !realm->badge.empty(  ) ? realm->badge : "(not set)", realm->filename, realm_type_names[realm->type] );
   ch->print_fmt( "&wDesc     : &W{}\r\n", realm->realmdesc );
   ch->print_fmt( "&wLeader   : &W{:<19.19}\t&wRank: &W{}\r\n", realm->leader, realm->leadrank );
   ch->print_fmt( "&wMembers  : &W{:<6}    &wBoard    : &W{:<6}\r\n", realm->members, realm->board );
}

CMDF( do_makerealm )
{
   std::string arg, arg2;
   realm_data *realm;
   realm_roster_data *member;
   char_data *victim;

   // is_imp check is here because realms won't exist right away so we need a way in :P
   if( !IS_ADMIN_REALM(ch) && !ch->is_imp(  ) )
   {
      ch->print( "Only members of the admin realm can use this command.\r\n" );
      return;
   }

   ch->set_color( AT_IMMORT );

   argument = one_argument( argument, arg );
   argument = one_argument( argument, arg2 );

   if( arg.empty(  ) || arg2.empty(  ) || argument.empty(  ) )
   {
      ch->print( "Usage: makerealm <filename> <realm name> <realm leader>\r\n" );
      ch->print( "Filename must be a SINGLE word, no punctuation marks.\r\n" );
      ch->print( "If the Realm name has spaces, enclose it inside quote marks.\r\n" );
      ch->print( "The realm leader must be present online to form a new realm.\r\n" );
      return;
   }

   if( !( victim = ch->get_char_world( argument ) ) )
   {
      ch->print( "That player is not online.\r\n" );
      return;
   }

   if( !victim->is_immortal(  ) )
   {
      ch->print( "A realm leader must be an immortal!\r\n" );
      return;
   }

   realm = get_realm( arg2 );
   if( realm )
   {
      ch->print( "There is already a realm with that name.\r\n" );
      return;
   }

   if( !is_valid_filename( ch, REALM_DIR, arg ) )
      return;

   strlower( arg );
   std::string filename = std::format( "{}{}", REALM_DIR, arg );

   realm = new realm_data;
   clean_realm( realm );

   realm->name = arg2;
   realm->filename = filename;
   realm->leader = victim->name;

   member = new realm_roster_data;
   member->name = victim->name;
   member->joined = current_time;
   realm->memberlist.push_back( member );
   realmlist.push_back( realm );

   victim->pcdata->realm = realm;
   victim->pcdata->realm_name = realm->name;

   victim->save(  );
   save_realm( victim->pcdata->realm );
   write_realm_list(  );

   ch->print_fmt( "Realm {} created with leader {}.\r\n", realm->name, realm->leader );
}

CMDF( do_realmroster )
{
   realm_data *realm;
   std::string arg, arg2;
   int total = 0;

   if( !IS_REALM_LEADER(ch) && !IS_ADMIN_REALM(ch) && !ch->is_imp(  ) )
   {
      ch->print( "Huh?\r\n" );
      return;
   }

   if( ch->isnpc(  ) )
   {
      ch->print( "NPCs can't use this command.\r\n" );
      return;
   }

   if( argument.empty(  ) )
   {
      ch->print( "Usage: realmroster <realmname>\r\n" );
      ch->print( "Usage: realmroster <realmname> remove <name>\r\n" );
      return;
   }

   argument = one_argument( argument, arg );
   if( !( realm = get_realm( arg ) ) )
   {
      ch->print_fmt( "No such realm known as {}\r\n", arg );
      return;
   }

   if( argument.empty(  ) )
   {
      ch->print_fmt( "Membership roster for {}\r\n\r\n", realm->name );
      ch->print_fmt( "{:<15.15}  {}\r\n", "Name", "Joined on" );
      ch->print( "---------------------------------------\r\n" );
      for( auto* member : realm->memberlist )
      {
         ch->print_fmt( "{:<15.15}  {}\r\n", member->name, c_time( member->joined, ch->pcdata->timezone ) );
         ++total;
      }
      ch->print_fmt( "\r\nThere are {} member{} in {}\r\n", total, total == 1 ? "" : "s", realm->name );
      return;
   }

   argument = one_argument( argument, arg2 );
   if( !str_cmp( arg2, "remove" ) )
   {
      if( argument.empty(  ) )
      {
         ch->print( "Remove who from the roster?\r\n" );
         return;
      }
      remove_realm_roster( realm, argument );
      save_realm( realm );
      ch->print_fmt( "{} has been removed from the roster for {}\r\n", argument, realm->name );
      return;
   }
   do_realmroster( ch, "" );
}

CMDF( do_realms )
{
   int count = 0;

   ch->print( "\r\n&RRealm         Type          Leader        Description:\r\n_________________________________________________________________________\r\n\r\n" );
   for( auto* realm : realmlist )
   {
      ch->print_fmt( "&w{:<13} {:<13} {:<13} {:<13}\r\n", realm->name, realm_type_names[realm->type], realm->leader, realm->realmdesc );
      ++count;
   }

   if( !count )
      ch->print( "&RThere are no Realms currently formed.\r\n" );
   else
      ch->print( "\r\n&R_________________________________________________________________________\r\n" );
   return;
}
