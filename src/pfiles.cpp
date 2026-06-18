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
 *                          Pfile Pruning Module                            *
 ****************************************************************************/

#include <filesystem>
#include <fstream>
#include "mud.h"
#include "clans.h"
#include "deity.h"
#include "objindex.h"
#include "pfiles.h"
#include "realms.h"

int num_quotes;   /* for quotes */

void prune_sales(  );
void remove_from_auth( std::string_view );
void rare_update(  );
void save_timedata(  );
void adjust_pfile( const std::string & );
bool check_parse_name( std::string, bool );

/* Globals */
std::chrono::system_clock::time_point new_pfile_time_t;
int num_pfiles; /* Count up number of pfiles */
int deleted = 0;
int pexempt = 0;

struct quote_data
{
   quote_data(  );
   ~quote_data(  );

   std::string quote;
   int number;
};

std::list<quote_data *> quotelist;

quote_data::quote_data(  )
{
   number = 0;
}

quote_data::~quote_data(  )
{
   quotelist.remove( this );
}

void free_quotes( void )
{
   for( auto it = quotelist.begin(  ); it != quotelist.end(  ); )
   {
      quote_data *quote = *it;
      ++it;

      deleteptr( quote );
   }
}

quote_data *get_quote( int q )
{
   for( auto* quote : quotelist )
   {
      if( quote->number == q )
         return quote;
   }
   return nullptr;
}

void save_quotes( void )
{
   std::ofstream stream;
   int q = 0;

   std::filesystem::path filename = std::format( "{}{}", SYSTEM_DIR, QUOTE_FILE );
   stream.open( filename );
   if( !stream.is_open(  ) )
   {
      perror( filename.c_str() );
      bug( "%s: Unable to open quote file for writing!", __func__ );
      return;
   }

   for( auto* quote : quotelist )
   {
      stream << "Quote: " << quote->quote << '~' << std::endl;
      quote->number = ++q;
   }
   stream.close(  );
   num_quotes = q;
}

/** Function: load_quotes
  * Descr   : Determines how many (if any) quote files are located within
  *           QUOTE_DIR, for later use by "do_quotes".
  * Returns : (void)
  * Syntax  : (none)
  * Written : v1.0 12/97
  * Author  : Gary McNickle <gary@dharvest.com>
  *
  * Completely rewritten by Samson so it can be OLC'able. 10-15-03
  */
void load_quotes( void )
{
   quote_data *quote;
   std::ifstream stream;

   quotelist.clear(  );
   num_quotes = 0;

   std::filesystem::path filename = std::format( "{}{}", SYSTEM_DIR, QUOTE_FILE );
   stream.open( filename );
   if( !stream.is_open(  ) )
   {
      perror( filename.c_str() );
      return;
   }

   do
   {
      std::string key, value;
      char buf[MIL];

      stream >> key;

      strip_lspace( key );
      strip_lspace( value );

      if( key == "#QUOTE" )
      {
         stream >> key;
         stream.getline( buf, MIL, '~' );
         value = buf;

         quote = new quote_data;

         quote->quote = value;
         quote->number = ++num_quotes;
         quotelist.push_back( quote );
      }
      else if( key == "Quote:" )
      {
         stream.getline( buf, MIL, '\xa2' );
         value = buf;

         quote = new quote_data;

         quote->quote = value;
         quote->number = ++num_quotes;
         quotelist.push_back( quote );
      }
   }
   while( !stream.eof(  ) );
   stream.close(  );
}

std::string add_linebreak( const std::string & str )
{
   std::string newstr = str;

   if( newstr.empty(  ) )
      return "";

   string_replace( newstr, "~", "\r\n" );
   return newstr;
}

/** Function: do_quotes
  * Descr   : Outputs an ascii file "quote.#" to the player. The number (#)
  *           is determined at random, based on how many quote files are
  *           stored in the QUOTE_DIR directory.
  * Returns : (void)
  * Syntax  : (none)
  * Written : v1.0 12/97
  * Author  : Gary McNickle <gary@dharvest.com>
  *
  * Completely rewritten by Samson so it can be OLC'able. 10-15-03
  */
CMDF( do_quoteset )
{
   quote_data *quote;
   std::string arg;
   int q;

   if( argument.empty(  ) )
   {
      ch->print( "Usage: quoteset add <quote>\r\n" );
      ch->print( "Usage: quoteset remove <quote#>\r\n" );
      ch->print( "Usage: quoteset list\r\n" );
      ch->print( "\r\nTo add a line break, insert a tilde (~) to signify it.\r\n" );
      return;
   }

   if( !str_cmp( argument, "list" ) )
   {
      std::list<quote_data *>::iterator qt;

      if( num_quotes == 0 )
      {
         ch->print( "There are no quotes to list.\r\n" );
         return;
      }
      for( qt = quotelist.begin(  ); qt != quotelist.end(  ); ++qt )
      {
         quote = *qt;

         ch->pagerf( "&cQuote #%d:\r\n &W%s&D\r\n\r\n", quote->number, quote->quote.c_str(  ) );
      }
      return;
   }

   argument = one_argument( argument, arg );

   if( !str_cmp( arg, "add" ) )
   {
      if( argument.empty(  ) )
      {
         do_quoteset( ch, "" );
         return;
      }
      argument = add_linebreak( argument );
      quote = new quote_data;
      quote->quote = argument;
      quotelist.push_back( quote );
      save_quotes(  );
      ch->printf( "Quote #%d has been added to the list.\r\n", quote->number );
      return;
   }

   if( str_cmp( arg, "remove" ) || !is_number( argument ) )
   {
      do_quoteset( ch, "" );
      return;
   }

   q = atoi( argument.c_str(  ) );
   if( !( quote = get_quote( q ) ) )
   {
      ch->print( "No quote by that number exists!\r\n" );
      return;
   }
   deleteptr( quote );
   save_quotes(  );
   ch->printf( "Quote #%d has been removed from the list.\r\n", q );
}

void quotes( char_data * ch )
{
   quote_data *quote;
   int q;

   if( num_quotes == 0 )
      return;

   /*
    * Lamers! How can you not like quotes! 
    */
   if( ch->has_pcflag( PCFLAG_NOQUOTE ) )
      return;

   q = number_range( 1, num_quotes );
   quote = get_quote( q );
   if( !quote )
   {
      bug( "%s: Missing quote #%d ?!?", __func__, q );
      return;
   }
   ch->printf( "&W\r\n%s&d\r\n", quote->quote.c_str(  ) );
}

CMDF( do_quotes )
{
   quotes( ch );
}

CMDF( do_pcrename )
{
   char_data *victim;
   std::string arg1;

   argument = one_argument( argument, arg1 );
   smash_tilde( argument );

   if( ch->isnpc(  ) )
      return;

   if( arg1.empty(  ) || argument.empty(  ) )
   {
      ch->print( "Syntax: rename <victim> <new name>\r\n" );
      return;
   }

   if( !check_parse_name( argument, true ) )
   {
      ch->print( "Illegal name.\r\n" );
      return;
   }

   /*
    * Just a security precaution so you don't rename someone you don't mean too --Shaddai 
    */
   if( !( victim = ch->get_char_room( arg1 ) ) )
   {
      ch->print( "That person is not in the room.\r\n" );
      return;
   }
   if( victim->isnpc(  ) )
   {
      ch->print( "You can't rename NPC's.\r\n" );
      return;
   }

   if( ch->get_trust(  ) < victim->get_trust(  ) )
   {
      ch->print( "I don't think they would like that!\r\n" );
      return;
   }
   std::filesystem::path newname = std::format( "{}{}/{}", PLAYER_DIR, static_cast<char>( std::tolower( argument.front() ) ), capitalize( argument ) );
   std::filesystem::path oldname = std::format( "{}{}/{}", PLAYER_DIR, static_cast<char>( std::tolower( victim->pcdata->filename[0] ) ), capitalize( victim->pcdata->filename ) );

   if( std::filesystem::exists( newname ) )
   {
      ch->print( "That name already exists.\r\n" );
      return;
   }

   /*
    * Have to remove the old god entry in the directories 
    */
   if( victim->is_immortal(  ) )
   {
      std::filesystem::path godname = std::format( "{}{}", GOD_DIR, capitalize( victim->pcdata->filename ) );
      std::filesystem::remove( godname );
   }

   /*
    * Remember to change the names of the areas 
    */
   if( victim->pcdata->area )
   {
      std::filesystem::path filename = std::format( "{}{}.are", BUILD_DIR, victim->name );
      std::filesystem::path newfilename = std::format( "{}{}.are", BUILD_DIR, capitalize( argument ) );
      std::filesystem::rename( filename, newfilename );

      filename = std::format( "{}{}.are.bak", BUILD_DIR, victim->name );
      newfilename = std::format( "{}{}.are.bak", BUILD_DIR, capitalize( argument ) );
      std::filesystem::rename( filename, newfilename );
   }

   /*
    * If they're in a clan/guild, remove them from the roster for it 
    */
   if( victim->pcdata->clan )
      remove_roster( victim->pcdata->clan, victim->name );

   victim->name = capitalize( argument );
   victim->pcdata->filename = capitalize( argument );
   if( !std::filesystem::remove( oldname ) )
   {
      log_printf( "Error: Couldn't delete file {} in do_rename.", oldname.string() );
      ch->print( "Couldn't delete the old file!\r\n" );
   }

   /*
    * Time to save to force the affects to take place 
    */
   if( victim->pcdata->clan )
   {
      add_roster( victim->pcdata->clan, victim->name, victim->Class, victim->level, victim->pcdata->mkills, victim->pcdata->mdeaths );
      save_clan( victim->pcdata->clan );
   }
   victim->save(  );

   /*
    * Now lets update the wizlist 
    */
   if( victim->is_immortal(  ) )
      make_wizlist(  );
   ch->print( "Character was renamed.\r\n" );
}

void search_pfiles( char_data* ch, const std::string & dirname, const std::string & filename, int cvnum )
{
   std::filesystem::path fname = std::filesystem::path( dirname ) / filename;
   std::ifstream is( fname, std::ios::in );

   if( !is.is_open() )
   {
      perror( fname.string().c_str() );
      return;
   }

   // Using a simple loop to process the stream
   std::string word;
   while( is >> word )
   {
      if( word == "#OBJECT" )
      {
         int vnum = 0, counter = 1;
         bool done = false;

         while( !done && ( is >> word ) )
         {
            switch( toupper( word[0] ) )
            {
               case 'C':
                  if( word == "Count" )
                     is >> counter;
                  break;
               case 'E':
                  if( word == "End" )
                     done = true;
                  break;
               case 'N':
                  if( word == "Nest" )
                  {
                     int n;
                     is >> n;
                  }
                  break;
               case 'O':
                  if( word == "Ovnum" )
                  {
                     is >> vnum;
                     if( !get_obj_index( vnum ) )
                     {
                        bug( "%s: Bad obj vnum: %d", __func__, vnum );
                        adjust_pfile( filename );
                     }
                     else if( vnum == cvnum )
                     {
                        ch->pagerf( "Player %s: Counted %d of Vnum %d.\r\n",  filename.empty() ? "<NO PFILE?!?>" : filename.c_str(), counter, cvnum );
                     }
                  }
                  break;
               default:
                  is.ignore( std::numeric_limits<std::streamsize>::max(), '\n' );
                  break;
            }
         }
      }
      else if( word == "#End" || word == "End" )
         break;
   }
}

/* Scans the pfiles to count the number of copies of a vnum being stored - Samson 1-3-99 */
void check_stored_objects( char_data * ch, int cvnum )
{
   for( char c = 'a'; c <= 'z'; ++c )
   {
      std::filesystem::path directory_path = std::format ("{}{}", PLAYER_DIR, c );

      if( !std::filesystem::exists( directory_path ) || !std::filesystem::is_directory( directory_path ) )
         continue;

      for( const auto& entry : std::filesystem::directory_iterator( directory_path ) )
      {
         // Skip entries starting with '.'
         if( entry.path().filename().string().starts_with( '.' ) )
            continue;

         search_pfiles( ch, directory_path, entry.path().filename(), cvnum );
      }
   }
}

void delete_pfile( const std::filesystem::path & path, std::string_view name, std::string_view clan_name, int days )
{
   if( std::filesystem::remove( path ) )
   {
      log_printf( "Player {} was deleted. Exceeded time limit of {} days.", name, days );
      remove_from_auth( name.data() );
      if( auto* pclan = get_clan( clan_name.data() ) )
      {
         remove_roster( pclan, name.data() );
      }
      ++deleted;
   }
}

void fread_pfile( std::ifstream & is, time_t tdiff, const std::filesystem::path & filepath, bool count )
{
   std::string name, clan, realm, deity;
   short level = 0;
   std::bitset<MAX_PCFLAG> pact;

   pact.reset();

   std::string word;
   char delim = '~';

   auto read_line = [&]( char delimiter = '\n' ) -> std::string
   {
      std::string line;
      std::getline( is, line, delimiter );
      strip_spaces( line );

      return line;
   };

   while( is >> word && word != "End" )
   {
      if( word == "*" )
      {
         is.ignore( std::numeric_limits<std::streamsize>::max(), '\n' );
         continue;
      }

      if( word == "Clan" )
         clan = read_line( delim );
      else if( word == "Deity" )
         deity = read_line( delim );
      else if( word == "ImmRealm" )
         realm = read_line( delim );
      else if( word == "Name" )
         name = read_line( delim );
      else if( word == "PCFlags" )
      {
         std::string flags = read_line( delim );
         flag_string_set( flags, pact, pc_flags );
      }
      else if( word == "Status" )
      {
         is >> level;
         is.ignore( 256, '\n' );
      }
      else if( word == "Version" )
      {
         is >> word;
      }
   }

   if( !count && !pact.test( PCFLAG_EXEMPT ) )
   {
      if( ( level < 10 && tdiff > sysdata->newbie_purge ) || ( level < LEVEL_IMMORTAL && tdiff > sysdata->regular_purge ) )
      {
         delete_pfile( filepath, name, clan, ( level < 10 ? sysdata->newbie_purge : sysdata->regular_purge ) );
         return;
      }
   }

   if( pact.test( PCFLAG_EXEMPT ) || level >= LEVEL_IMMORTAL )
      ++pexempt;

   if( auto* guild = get_clan( clan.c_str() ) )
      ++guild->members;
   if( auto* rl = get_realm( realm.c_str() ) )
      ++rl->members;
   if( auto* god = get_deity( deity.c_str() ) )
      ++god->worshippers;
}

void read_pfile( const std::filesystem::path & dirname, const std::string & filename, bool count )
{
   // Prevent directory traversal
   if( filename.find_first_of( "/\\." ) != std::string::npos )
   {
      return;
   }

   std::filesystem::path full_path = dirname / filename;

   if( !std::filesystem::exists( full_path ) )
      return;

   auto ftime = std::filesystem::last_write_time( full_path );
   auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>( ftime - std::filesystem::file_time_type::clock::now() + current_time );
   time_t tdiff = ( std::chrono::system_clock::to_time_t( current_time ) - std::chrono::system_clock::to_time_t( sctp ) ) / 86400;

   std::ifstream is( full_path );
   if( !is.is_open() )
      return;

   std::string line;
   while( is >> line )
   {
      if( line == "#PLAYER" )
      {
         fread_pfile( is, tdiff, full_path, count );
      }
   }
}

void pfile_scan( bool count )
{
   deleted = 0;
   pexempt = 0;

   if( !count )
   {
      // Reset all clans
      for( auto* clan : clanlist )
         clan->members = 0;

      // Reset all realms
      for( auto* realm : realmlist )
         realm->members = 0;

      // Reset all deities
      for( auto* deity : deitylist )
         deity->worshippers = 0;
   }

   int cou = 0;
   for( char c = 'a'; c <= 'z'; ++c )
   {
      std::filesystem::path directory_path = std::format ("{}{}", PLAYER_DIR, c );

      if( !std::filesystem::exists( directory_path ) || !std::filesystem::is_directory( directory_path ) )
         continue;

      for( const auto& entry : std::filesystem::directory_iterator( directory_path ) )
      {
         // Skip entries starting with '.'
         if( entry.path().filename().string().starts_with( '.' ) )
            continue;

         if( !count )
            read_pfile( directory_path, entry.path().filename(), count );
         ++cou;
      }
   }

   if( !count )
      log_string( "Pfile cleanup completed." );
   else
      log_string( "Pfile count completed." );

   log_printf( "Total pfiles scanned: {}", cou );
   log_printf( "Total exempted pfiles: {}", pexempt );

   if( !count )
   {
      log_printf( "Total pfiles deleted: {}", deleted );
      log_printf( "Total pfiles remaining: {}", cou - deleted );
      num_pfiles = cou - deleted;
   }
   else
      num_pfiles = cou;

   if( !count )
   {
      for( auto* clan : clanlist )
         save_clan( clan );

      for( auto* realm : realmlist )
         save_realm( realm );

      for( auto* deity : deitylist )
         save_deity( deity );

      verify_clans(  );
      verify_realms( );
      prune_sales(  );
   }
}

bool is_safe_target( const std::filesystem::path & target_dir )
{
   std::error_code ec;
   std::filesystem::path base = std::filesystem::weakly_canonical( PLAYER_DIR, ec );
   std::filesystem::path target = std::filesystem::weakly_canonical( target_dir, ec );

   if( ec )
      return false;

   auto [mismatch_base, mismatch_target] = std::mismatch( base.begin(), base.end(), target.begin() );

   return mismatch_base == base.end();
}

// This should enforce using a specific filename found in BACKUP_DIR so that the admin has to input that to the do_pfiles() command.
bool restore_pfiles( std::string_view a_file )
{
   std::filesystem::path target_dir = PLAYER_DIR;
   std::filesystem::path archive_filename = a_file;

   // Construct the path and ensure it's inside BACKUP_DIR
   std::filesystem::path archive_path = std::filesystem::path( BACKUP_DIR ) / archive_filename;

   // Ensure the file exists and is indeed a .bak file
   if( !std::filesystem::exists( archive_path ) || archive_path.extension() != ".bak" )
   {
      return false;
   }

   // Ensure the target is the authorized PLAYER_DIR
   if( !is_safe_target( target_dir ) )
   {
      return false;
   }

   std::ifstream archive( archive_path, std::ios::binary );
   if( !archive.is_open() )
      return false;

   std::string line;
   std::ofstream current_file;

   while( std::getline( archive, line ) )
   {
      if( line.starts_with( "FILE_START:" ) )
      {
         std::filesystem::path rel_path = line.substr( 11 );
         std::filesystem::path full_path = target_dir / rel_path;

         std::filesystem::create_directories( full_path.parent_path() );
         current_file.open( full_path, std::ios::binary );
      }
      else if( line == "FILE_END" )
      {
         current_file.close();
      }
      else if( current_file.is_open() )
      {
         current_file << line << "\n";
      }
   }
   return true;
}

// Old backups will accumulate daily. So let's clean those up.
void cleanup_old_backups( int days_to_keep )
{
   std::filesystem::path backup_dir = BACKUP_DIR;

   for( const auto& entry : std::filesystem::directory_iterator( backup_dir ) )
   {
      auto ftime = std::filesystem::last_write_time( entry );
      auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>( ftime - std::filesystem::file_time_type::clock::now() + current_time );

      if( current_time - sctp > std::chrono::hours( 24 * days_to_keep ) )
      {
         std::filesystem::remove( entry );
      }
   }
}

// Generate a backup file whenever the pfiles are being pruned.
void create_backup_archive( void )
{
   std::filesystem::path player_dir = PLAYER_DIR;
   std::filesystem::path backup_dir = BACKUP_DIR;

   std::string timestamp = std::format( "{:%Y%m%d_%H%M%S}", std::chrono::floor<std::chrono::seconds>( current_time ) );

   std::filesystem::create_directories( backup_dir );
   std::filesystem::path archive_path = backup_dir / std::format( "pfiles_{}.bak", timestamp );

   std::ofstream archive( archive_path, std::ios::binary );
   if( !archive.is_open() )
      return;

   for( const auto& entry : std::filesystem::recursive_directory_iterator( player_dir ) )
   {
      if( entry.is_regular_file() )
      {
         std::filesystem::path rel_path = std::filesystem::relative( entry.path(), player_dir );
         std::ifstream source( entry.path(), std::ios::binary );

         if( source.is_open() )
         {
            archive << "FILE_START:" << rel_path.string() << "\n";
            archive << source.rdbuf();
            archive << "\nFILE_END\n";
         }
      }
   }
}

CMDF( do_pfiles )
{
   if( ch->isnpc(  ) )
   {
      ch->print( "Mobs cannot use this command!\r\n" );
      return;
   }

   if( argument.empty(  ) )
   {
      log_printf( "Manual pfile cleanup started by {}.", ch->name );

      /*
       * Makes a backup copy of existing pfiles just in case - Samson
       */
      // Clears out any backup archives that are older than 30 days. Which should be more than enough time.
      cleanup_old_backups( 30 ); // FIXME: Make this a sysdata setting once testing is complete.

      // Generate the archive for this operation.
      create_backup_archive( );

      pfile_scan( false );
      rare_update(  );
      return;
   }

   if( !str_cmp( argument, "settime" ) )
   {
      new_pfile_time_t = current_time + std::chrono::hours( 24 );
      save_timedata(  );
      ch->print( "New cleanup time set for 24 hrs from now.\r\n" );
      return;
   }

   if( !str_cmp( argument, "count" ) )
   {
      log_printf( "Pfile count started by {}.", ch->name );
      pfile_scan( true );
      return;
   }

   std::string arg1 = one_argument( argument, arg1 );
   if( !str_cmp( arg1, "restore" ) )
   {
      std::string arg2 = one_argument( argument, arg2 );

      if( !sysdata->LOCKDOWN )
      {
         ch->print( "Pfile restoration can only take place if the MUD is in lockdown.\r\nThis is to protect against active users overwriting the restored files.\r\n" );
         return;
      }

      if( arg2.empty() )
      {
         ch->print( "Syntax: pfiles restore <archive name>\r\n" );
         return;
      }

      if( restore_pfiles( arg2 ) )
      {
         ch->print_fmt( "Successfully restored from {}.\r\n", arg2 );
      }
      else
      {
         ch->print_fmt( "Error: Could not restore. Check if the filename is correct and exists in {}.\r\n", BACKUP_DIR );
      }
      return;
   }

   ch->print( "Invalid argument.\r\n" );
}

void check_pfiles( time_t reset )
{
   /*
    * This only counts them up on reboot if the cleanup isn't needed - Samson 1-2-00 
    */
   if( reset == 255 && new_pfile_time_t > current_time )
   {
      reset = 0;  /* Call me paranoid, but it might be meaningful later on */
      log_string( "Counting pfiles....." );
      pfile_scan( true );
      return;
   }

   if( current_time >= new_pfile_time_t )
   {
      if( sysdata->CLEANPFILES == true )
      {
         /*
          * Makes a backup copy of existing pfiles just in case - Samson 
          */
         // Clears out any backup archives that are older than 30 days. Which should be more than enough time.
         cleanup_old_backups( 30 ); // FIXME: Make this a sysdata setting once testing is complete.

         // Generate the archive for this operation.
         create_backup_archive( );

         new_pfile_time_t = current_time + std::chrono::hours( 24 );
         save_timedata(  );
         log_string( "Automated pfile cleanup beginning...." );
         pfile_scan( false );
         if( reset == 0 )
            rare_update(  );
      }
      else
      {
         new_pfile_time_t = current_time + std::chrono::hours( 24 );
         save_timedata(  );
         log_string( "Counting pfiles....." );
         pfile_scan( true );
         if( reset == 0 )
            rare_update(  );
      }
   }
}
