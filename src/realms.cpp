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

/*
 * Save a realm's data to its data file.
 */
constexpr int REALM_VERSION = 1;
void save_realm( realm_data * realm )
{
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
   std::ofstream stream( std::filesystem::path{filename} );
   if( !stream.is_open() )
   {
      bug( "{}: Cannot open {} for writing: {}", __func__, filename.string(), std::strerror(errno) );
      return;
   }

   stream << "#REALM\n";
   stream << std::format( "Version      {}\n", REALM_VERSION );
   stream << std::format( "Name         {}~\n", realm->name );
   stream << std::format( "Filename     {}~\n", realm->filename );
   stream << std::format( "Description  {}~\n", realm->realmdesc );
   stream << std::format( "Leader       {}~\n", realm->leader );
   stream << std::format( "Leadrank     {}~\n", realm->leadrank );
   stream << std::format( "Badge        {}~\n", realm->badge );
   stream << std::format( "Type         {}~\n", realm_type_names[realm->type] );
   stream << std::format( "Members      {}\n", realm->members );
   stream << std::format( "Board        {}\n", realm->board );
   stream << "End\n\n";

   for( auto* member : realm->memberlist )
   {
      auto joined = std::chrono::system_clock::to_time_t( member->joined );

      stream << "#ROSTER\n";
      stream << std::format( "Name      {}~\n", member->name );
      stream << std::format( "Joined    {}\n", joined );
      stream << "End\n\n";
   }

   stream << "#END\n";
   stream.close();
   if( stream.fail() )
      bug( "{}: Error occurred after closing {}: ", __func__, filename.string(), std::strerror(errno) );
}

/*
 * Read in actual realm data.
 */
void fread_realm( std::ifstream & stream, realm_data * realm )
{
   int file_ver = 0;

   std::string key;
   while( stream >> key )
   {
      if( key[0] == '*' )
         fread_to_eol( stream );
      else if( key == "Version" )
         stream >> file_ver;
      else if( key == "Name" )
         realm->name = fread_line( stream );
      else if( key == "Filename" )
         realm->filename = fread_line( stream );
      else if( key == "Description" )
         realm->realmdesc = fread_line( stream );
      else if( key == "Leader" )
         realm->leader = fread_line( stream );
      else if( key == "Leadrank" )
         realm->leadrank = fread_line( stream );
      else if( key == "Badge" )
         realm->badge = fread_line( stream );
      else if( key == "Type" )
      {
         std::string temp = fread_line( stream );
         int value = get_realm_type_name( temp );

         if( value < 0 || value > MAX_REALM )
         {
            bug( "{}: Invalid realm type: {}. Setting to None.", __func__, temp );
            value = 0;
         }
         realm->type = value;
      }
      else if( key == "Members" )
         stream >> realm->members;
      else if( key == "Board" )
         stream >> realm->board;
      else if( key == "End" )
      {
         return;
      }
      else
         bug( "{}: Bad section '{}' - skipping.", __func__, key );
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
   std::filesystem::path filename = std::format( "{}{}", REALM_DIR, realmfile );
   std::ifstream stream( std::filesystem::path{filename} );
   if( !stream.is_open() )
   {
      bug( "{}: Cannot open {} for reading: {}", __func__, filename.string(), std::strerror(errno) );
      return false;
   }

   realm_data *realm = new realm_data;
   clean_realm( realm );  /* Default settings so we don't get weird ass stuff */

   std::string key;
   while( stream >> key )
   {
      if( key == "#REALM" )
         fread_realm( stream, realm );
      else if( key == "#ROSTER" )
      {
         realm_roster_data *roster = new realm_roster_data;

         while( stream >> key )
         {
            if( key == "Name" )
               roster->name = fread_line( stream );
            else if( key == "Joined" )
            {
               time_t loaded_time;
               stream >> loaded_time;

               roster->joined = std::chrono::system_clock::from_time_t( loaded_time );
            }
            else if( key == "End" )
            {
               realm->memberlist.push_back( roster );
               break;
            }
         }
      }
      else if( key == "#END" )
      {
         realmlist.push_back( realm );
         break;
      }
      else
      {
          bug( "{}: Bad section '{}' in {} - skipping.", __func__, key, filename.string() );
      }
   }
   stream.close();
   return true;
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

   log_string( "Loading Realms..." );

   std::filesystem::path realmlistfile = std::format( "{}{}", REALM_DIR, REALM_LIST );
   std::ifstream stream( std::filesystem::path{realmlistfile} );
   if( !stream.is_open(  ) )
   {
      bug( "{}: Cannot open {} for reading: {}", __func__, realmlistfile.string(), std::strerror(errno) );
      std::exit( EXIT_FAILURE );
   }

   for( ;; )
   {
      std::string filename = fread_line( stream, '\n' );

      if( filename[0] == '$' )
         break;

      log_string( filename );

      if( !load_realm_file( filename ) )
         bug( "{}: Cannot load realm file: {} - {}", __func__, filename, std::strerror(errno) );
   }
   stream.close();

   verify_realms(  ); /* Check against pfiles to see if realms should still exist */
   log_string( "Done: Realms." );
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
