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
 *                       Database management module                         *
 ****************************************************************************/

// For backtrace info in logging functions.
#if __has_include(<cxxabi.h>)
#include <cxxabi.h>
#define HAVE_CXXABI 1
#endif

#include <dlfcn.h> // For libdl - Trax
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <random>
#include <stacktrace>
#include <unordered_map>
#include "mud.h"
#include "area.h"
#include "event.h"

#if defined(SQL)
 #include "sql.h"
#endif

/*
 * This will seed the random number generator once during startup. I guess it's magic code :P
 * Uses the Mersenne Twister algorithm. - Samson 6/1/2026.
 */
std::mt19937 global_rng( std::random_device{}() );

/*
 * Change this alarm timer to whatever you think is appropriate.
 * Adjust if you are getting infinite loop warnings that are shutting down bootup.
 */
constexpr int AREA_FILE_ALARM = 45;

system_data *sysdata;

/*
 * Structure used to build wizlist. No destructor needed as it uses std::unique_ptr
 */
class wizent
{
 public:
   wizent( ) : level( 0 ) {}

   std::string name;
   short level;
};

std::list<std::unique_ptr<wizent>> wizlist;

void free_wizlist_data()
{
   wizlist.clear();
}

void init_supermob( void );

/*
 * Globals.
 */
time_info_data time_info;
extern const char *alarm_section;
extern struct extracted_obj_data *extracted_obj_queue;
extern struct extracted_char_data *extracted_char_queue;

int cur_qobjs;
int cur_qchars;
int nummobsloaded;
int numobjsloaded;
int physicalobjects;
int last_pkroom;

/*
 * Locals.
 */
int top_affect;
int top_ed;
int top_exit;
int top_mob_index;
int top_obj_index;
int top_prog;
int top_reset;
int top_room;
int top_shop;
int top_repair;

/*
 * Semi-locals.
 */
bool fBootDb;
std::ifstream fpArea;
std::string strArea;

extern int astral_target;

/*
 * External booting function
 */
void set_alarm( long );
#ifdef MULTIPORT
void load_shellcommands(  );
#endif
void web_arealist(  );
bool load_timedata(  );
void load_shopkeepers(  );
void load_auth_list(  );   /* New Auth Code */
void save_auth_list(  );
void build_wizinfo(  );
void load_ships(  ); /* Load ships - Samson 1-6-01 */
void load_world(  );
void load_morphs(  );
void load_skill_table(  );
void remap_slot_numbers(  );
void load_quotes(  );
void load_sales(  ); /* Samson 6-24-99 for new auction system */
void load_aucvaults(  );   /* Samson 6-20-99 for new auction system */
void load_corpses(  );
void load_banlist(  );
void update_timers(  );
void update_calendar(  );
void load_specfuns(  );
void load_equipment_totals( bool );
void load_slays(  );
void load_holidays(  );
void load_bits(  );
void load_liquids(  );
void load_mixtures(  );
void load_imm_host(  );
void load_dns(  );
void load_mudchannels(  );
void to_channel( std::string_view, std::string_view, int );
void load_runes(  );
void load_clans(  );
void load_realms(  );
void load_socials(  );
void load_commands(  );
void load_mssp_data( );
void load_deity(  );
void load_boards(  );
void load_projects(  );
void assign_gsn_data(  );
void load_connhistory(  );
void populate_skill_indexes(  );
void load_classes(  );
void load_races(  );
void load_herb_table(  );
void load_tongues(  );
void load_helps(  );
void load_login_messages(  );
void init_chess(  );
void load_continents( const int );
void validate_overland_data(  );
void make_webwiz(  );
void init_auction( );
void load_name_generator( );
void load_reserved_names( );
bool load_weathermap( );
void InitializeWeatherMap( );
void fix_exits( );

affect_data::affect_data(  )
{
}

void close_libdl( void )
{
   dlclose( sysdata->dlHandle );
}

void reopen_libdl( void )
{
   sysdata->dlHandle = dlopen( nullptr, RTLD_LAZY );
}

void shutdown_mud( std::string_view reason )
{
   std::ofstream stream( std::filesystem::path( SHUTDOWN_FILE ), std::ios::app );
   if( !stream.is_open() )
   {
      bug( "{}: Cannot open {} for writing: {}", __func__, SHUTDOWN_FILE, std::strerror(errno) );
      return;
   }

   stream << reason << "\n";
   stream.close();
   if( stream.fail() )
      bug( "{}: Error occurred after closing {}: ", __func__, SHUTDOWN_FILE, std::strerror(errno) );
}

bool truncate_file( std::string_view filename )
{
   std::ofstream stream( std::filesystem::path( filename ), std::ios::trunc );

   if( !stream.is_open() )
      return false;

   stream.close();
   return true;
}

void append( std::string_view file, std::string content )
{
   std::ofstream stream( std::filesystem::path( file ), std::ios::app );
   if( !stream.is_open() )
   {
      bug( "{}: Cannot open {} for writing: {}", __func__, file, std::strerror(errno) );
      return;
   }

   if( content.back() != '\n' )
      content += '\n';

   stream << content;
   stream.close();
   if( stream.fail() )
      bug( "{}: Error occurred after closing {}: ", __func__, file, std::strerror(errno) );
}

bool exists_file( std::string_view name )
{
   /*
    * Stands to reason that if there ain't a name to look at, it damn well don't exist! 
    */
   if( name.empty(  ) )
      return false;

   std::filesystem::path filename = name;
   if( std::filesystem::exists( filename ) )
      return true;

   return false;
}

bool has_illegal_file_chars( std::string_view filename, bool check_spaces )
{
   /*
    * Illegal characters
    */
   if( filename.contains( "." ) || filename.contains( "/" ) || filename.contains( "\\" ) )
      return true;

   if( check_spaces && filename.contains( " " ) )
      return true;

   return false;
}

bool is_valid_filename( char_data * ch, std::string_view direct, std::string_view filename )
{
   /*
    * Length restrictions 
    */
   if( filename.empty(  ) || filename.length(  ) < 3 )
   {
      if( filename.empty(  ) )
         ch->print( "Empty filename is not valid.\r\n" );
      else
         ch->print_fmt( "{}: Filename is too short.\r\n", filename );
      return false;
   }

   if( has_illegal_file_chars( filename, true ) )
   {
      ch->print( "A filename may not contain a '.', '/', space, or '\\' in it.\r\n" );
      return false;
   }

   /*
    * If that filename is already being used lets not allow it now to be on the safe side 
    */
   std::filesystem::path newfilename = std::format( "{}{}", direct, filename );
   if( std::filesystem::exists( newfilename ) )
   {
      ch->print_fmt( "{} is already an existing filename.\r\n", newfilename.string() );
      return false;
   }

   /*
    * If we got here assume its valid 
    */
   return true;
}

/*
 * Added lots of EOF checks, as most of the file crashes are based on them.
 * If an area file encounters EOF, the fread_* functions will shutdown the
 * MUD, as all area files should be read in in full or bad things will
 * happen during the game.  Any files loaded in without fBootDb which
 * encounter EOF will return what they have read so far. These files
 * should include player files, and in-progress areas that are not loaded
 * upon bootup.
 * -- Altrag
 */
/*
 * Read a letter from a file.
 */
char fread_letter( std::ifstream & stream )
{
   char c;

   while( stream.get(c) )
   {
      if( !std::isspace( static_cast<unsigned char>( c ) ) )
      {
         return c;
      }
   }

   bug( "{}: EOF encountered on read.", __func__ );

   if( fBootDb )
   {
      shutdown_mud( "Corrupt file somewhere." );
      std::exit( EXIT_FAILURE );
   }

   return '\0';
}

static std::string internal_fread_flagstring( std::ifstream & stream )
{
   char c;

   // Skip leading whitespace
   while( stream.get( c ) && std::isspace( static_cast<unsigned char>( c ) ) );

   if( stream.eof() )
   {
      bug( "{}: EOF encountered on read.", __func__ );
      if( fBootDb )
      {
         shutdown_mud( "Corrupt file somewhere." );
         std::exit( EXIT_FAILURE );
      }
      return "";
   }

   if( c == '~' )
      return "";

   std::string result;
   result.reserve( 256 );
   result.push_back( c );

   // Read until '~' or EOF
   while( stream.get( c ) )
   {
      if( c == '~' )
         return result;

      if( c == '\n' )
      {
         result.push_back( '\n' );
         result.push_back( '\r' );
      }
      else if( c != '\r' )
      {
         result.push_back( c );
      }
   }

   bug( "{}: EOF encountered before ~", __func__ );
   return result;
}

// Read a string from a file and assign it to a std::string.
void fread_string( std::string & newstring, std::ifstream & stream )
{
   newstring = internal_fread_flagstring( stream );
}

/*
 * When called for a file comment, will ignore up to MSL characters, or it hits a newline.
 * The chances of this overflowing the MSL buffer size is extremely remote because someone would have to have written a book to fill that much space.
 * Even though this isn't the "right" way to do it, it avoided a massive amount of inclusion bloat.
 */
void fread_to_eol( std::ifstream & stream )
{
   stream.ignore( static_cast<std::streamsize>(MSL), '\n' );
}

// Read one line into a std::string, with delimiter check. Usually a '~' but can be overridden at the call site.
std::string fread_line( std::ifstream & stream, char delimiter )
{
   std::string line;

   if( std::getline( stream, line, delimiter ) )
      strip_whitespace( line ); // Once you have the line, it's best to strip it of any potential whitespace characters to eliminate the possibility of a bloat loop later on when the value is written back to disk.

   return line;
}

/*
 * Read one word from a file. Can be wrapped in '' or "".
 */
std::string fread_word( std::ifstream & stream )
{
   char c;

   // Skip leading whitespace.
   while( stream.get(c) && std::isspace( static_cast<unsigned char>(c) ) );

   if( stream.eof() )
   {
      bug( "{}: EOF encountered on read.", __func__ );
      if( fBootDb )
      {
         shutdown_mud( "Corrupt file." );
         std::exit( EXIT_FAILURE );
      }
      return {};
   }

   std::string word;
   word.reserve(64);

   if( c == '\'' || c == '"' )
   {
      const char delimiter = c;

      while( stream.get(c) && c != delimiter )
      {
         word.push_back(c);
      }

      if( stream.eof() )
      {
         bug( "{}: EOF encountered inside quoted string.", __func__ );
         if( fBootDb )
            std::exit( EXIT_FAILURE );
      }
      return word;
   }

   word.push_back(c);
   while( stream.get(c) )
   {
      if( std::isspace( static_cast<unsigned char>(c) ) )
      {
         stream.unget();
         break;
      }
      word.push_back(c);
   }
   return word;
}

/*
 * Add a string to the boot-up log - Thoric
 */
void write_to_boot_log( std::string_view text )
{
   log_printf( "[*****] BOOT: {}", text );

   std::ofstream stream;
   stream.open( std::filesystem::path( BOOTLOG_FILE ), std::ios::app );
   if( !stream.is_open() )
   {
      bug( "{}: Cannot open {} for writing: {}", __func__, BOOTLOG_FILE, std::strerror(errno) );
      return;
   }

   stream << text << "\n";
   stream.close();
   if( stream.fail() )
      bug( "{}: Error occurred after closing {}: ", __func__, BOOTLOG_FILE, std::strerror(errno) );
}

/*
 * Build list of in progress areas. Do not load areas.
 * define AREA_READ if you want it to build area names rather than reading
 * them out of the area files. -- Altrag
 */
/* The above info is obsolete - this will now simply load whatever is in the
 * BUILD_DIR and assume it to be a valid prototype zone. -- Samson 2-13-04
 */
void load_buildlist( void )
{
   if( !std::filesystem::exists( BUILD_DIR ) || !std::filesystem::is_directory( BUILD_DIR ) )
   {
      // This should be treated as fatal.
      bug( "{}: Builder directory is missing!", __func__ );
      std::exit( EXIT_FAILURE );
   }

   for( const auto& entry : std::filesystem::directory_iterator( BUILD_DIR ) )
   {
      std::string filename = entry.path().filename().string();

      if( filename.empty() || filename[0] == '.' )
         continue;
      if( filename.find( ".are" ) == std::string::npos )
         continue;
      if( filename.find( ".bak" ) != std::string::npos )
         continue;

      std::filesystem::path full_path = std::filesystem::path( BUILD_DIR ) / filename;

      strArea = filename;
      set_alarm( AREA_FILE_ALARM );
      alarm_section = "load_buildlist: read prototype area files";

      load_area_file( full_path.string(), true );
   }
}

constexpr int SYSFILEVER = 1;
/*
 * Save system info to data file
 */
void save_sysdata( void )
{
   std::filesystem::path filename = std::format( "{}sysdata.dat", SYSTEM_DIR );
   std::ofstream stream( filename );

   if( !stream.is_open() )
   {
      bug( "{}: Cannot open {} for writing: {}", __func__, filename.string(), std::strerror(errno) );
      return;
   }
   else
   {
      auto motd = std::chrono::system_clock::to_time_t( sysdata->motd );
      auto imotd = std::chrono::system_clock::to_time_t( sysdata->imotd );

      stream << "#SYSTEM\n";
      stream << std::format( "Version        {}\n", SYSFILEVER );
      stream << std::format( "Mudname        {}~\n", sysdata->mud_name );

      if( !sysdata->password.empty() )
         stream << std::format( "Password       {}~\n", sysdata->password );

      stream << std::format( "Dbserver       {}~\n", sysdata->dbserver );
      stream << std::format( "Dbname         {}~\n", sysdata->dbname );
      stream << std::format( "Dbuser         {}~\n", sysdata->dbuser );
      stream << std::format( "Dbpass         {}~\n", sysdata->dbpass );
      stream << std::format( "Highplayers    {}\n", sysdata->alltimemax );
      stream << std::format( "Highplayertime {}~\n", sysdata->time_of_max );
      stream << std::format( "CheckImmHost   {}\n", sysdata->check_imm_host );
      stream << std::format( "Nameresolving  {}\n", sysdata->NO_NAME_RESOLVING );
      stream << std::format( "Waitforauth    {}\n", sysdata->WAIT_FOR_AUTH );
      stream << std::format( "Readallmail    {}\n", sysdata->read_all_mail );
      stream << std::format( "Readmailfree   {}\n", sysdata->read_mail_free );
      stream << std::format( "Writemailfree  {}\n", sysdata->write_mail_free );
      stream << std::format( "Takeothersmail {}\n", sysdata->take_others_mail );
      stream << std::format( "Getnotake      {}\n", sysdata->level_getobjnotake );
      stream << std::format( "Build          {}\n", sysdata->build_level );
      stream << std::format( "Protoflag      {}\n", sysdata->level_modify_proto );
      stream << std::format( "Overridepriv   {}\n", sysdata->level_override_private );
      stream << std::format( "Msetplayer     {}\n", sysdata->level_mset_player );
      stream << std::format( "Stunplrvsplr   {}\n", sysdata->stun_plr_vs_plr );
      stream << std::format( "Stunregular    {}\n", sysdata->stun_regular );
      stream << std::format( "Gougepvp       {}\n", sysdata->gouge_plr_vs_plr );
      stream << std::format( "Gougenontank   {}\n", sysdata->gouge_nontank );
      stream << std::format( "Dodgemod       {}\n", sysdata->dodge_mod );
      stream << std::format( "Parrymod       {}\n", sysdata->parry_mod );
      stream << std::format( "Tumblemod      {}\n", sysdata->tumble_mod );
      stream << std::format( "Damplrvsplr    {}\n", sysdata->dam_plr_vs_plr );
      stream << std::format( "Damplrvsmob    {}\n", sysdata->dam_plr_vs_mob );
      stream << std::format( "Dammobvsplr    {}\n", sysdata->dam_mob_vs_plr );
      stream << std::format( "Dammobvsmob    {}\n", sysdata->dam_mob_vs_mob );
      stream << std::format( "Forcepc        {}\n", sysdata->level_forcepc );
      stream << std::format( "Saveflags      {}~\n", bitset_string( sysdata->save_flags, save_flag ) );
      stream << std::format( "Savefreq       {}\n", sysdata->save_frequency.count() );
      stream << std::format( "Bestowdif      {}\n", sysdata->bestow_dif) ;
      stream << std::format( "PetSave        {}\n", sysdata->save_pets );
      stream << std::format( "Wizlock        {}\n", sysdata->WIZLOCK );
      stream << std::format( "Implock        {}\n", sysdata->IMPLOCK );
      stream << std::format( "Lockdown       {}\n", sysdata->LOCKDOWN );
      stream << std::format( "Admin_Email    {}~\n", sysdata->admin_email );
      stream << std::format( "Newbie_purge   {}\n", sysdata->newbie_purge );
      stream << std::format( "Regular_purge  {}\n", sysdata->regular_purge );
      stream << std::format( "Autopurge      {}\n", sysdata->CLEANPFILES );
      stream << std::format( "Testmode       {}\n", sysdata->TESTINGMODE );
      stream << std::format( "Mapsize        {}\n", sysdata->mapsize );
      stream << std::format( "Motd           {}\n", motd );
      stream << std::format( "Imotd          {}\n", imotd );
      stream << std::format( "Telnet         {}~\n", sysdata->telnet );
      stream << std::format( "HTTP           {}~\n", sysdata->http );
      stream << std::format( "Maxvnum        {}\n", sysdata->maxvnum );
      stream << std::format( "Minguild       {}\n", sysdata->minguildlevel );
      stream << std::format( "Maxcond        {}\n", sysdata->maxcondval );
      stream << std::format( "Maxignore      {}\n", sysdata->maxign );
      stream << std::format( "Maximpact      {}\n", sysdata->maximpact );
      stream << std::format( "Maxholiday     {}\n", sysdata->maxholiday );
      stream << std::format( "Initcond       {}\n", sysdata->initcond );
      stream << std::format( "Secpertick     {}\n", sysdata->secpertick );
      stream << std::format( "Pulsepersec    {}\n", sysdata->pulsepersec );
      stream << std::format( "Hoursperday    {}\n", sysdata->hoursperday );
      stream << std::format( "Daysperweek    {}\n", sysdata->daysperweek );
      stream << std::format( "Dayspermonth   {}\n", sysdata->dayspermonth );
      stream << std::format( "Monthsperyear  {}\n", sysdata->monthsperyear );
      stream << std::format( "Minego         {}\n", sysdata->minego );
      stream << std::format( "Rebootcount    {}\n", sysdata->rebootcount );
      stream << std::format( "Auctionseconds {}\n", sysdata->auctionseconds );
      stream << std::format( "Gameloopalarm  {}\n", sysdata->gameloopalarm );
      stream << std::format( "Webwho         {}\n", sysdata->webwho );
      stream << "\n#END\n";
   }
   stream.close();
   if( stream.fail() )
      bug( "{}: Error occurred after closing {}: ", __func__, filename.string(), std::strerror(errno) );
}

/*
 * Load the sysdata file
 */
bool load_systemdata( void )
{
   int file_ver = 0;

   std::filesystem::path filename = std::format( "{}sysdata.dat", SYSTEM_DIR );
   std::ifstream stream( filename );

   if( !stream.is_open() )
      return false;

   static const std::unordered_map<std::string, std::function<void()>> loaders = {
      { "Version",        [&](){ stream >> file_ver; } },
      { "Mudname",        [&](){ sysdata->mud_name = fread_line( stream ); } },
      { "Password",       [&](){ sysdata->password = fread_line( stream ); } },
      { "Dbserver",       [&](){ sysdata->dbserver = fread_line( stream ); } },
      { "Dbname",         [&](){ sysdata->dbname = fread_line( stream ); } },
      { "Dbuser",         [&](){ sysdata->dbuser = fread_line( stream ); } },
      { "Dbpass",         [&](){ sysdata->dbpass = fread_line( stream ); } },
      { "Highplayers",    [&](){ stream >> sysdata->alltimemax; } },
      { "Highplayertime", [&](){ sysdata->time_of_max = fread_line( stream ); } },
      { "CheckImmHost",   [&](){ stream >> sysdata->check_imm_host; } },
      { "Nameresolving",  [&](){ stream >> sysdata->NO_NAME_RESOLVING; } },
      { "Waitforauth",    [&](){ stream >> sysdata->WAIT_FOR_AUTH; } },
      { "Readallmail",    [&](){ stream >> sysdata->read_all_mail; } },
      { "Readmailfree",   [&](){ stream >> sysdata->read_mail_free; } },
      { "Writemailfree",  [&](){ stream >> sysdata->write_mail_free; } },
      { "Takeothersmail", [&](){ stream >> sysdata->take_others_mail; } },
      { "Getnotake",      [&](){ stream >> sysdata->level_getobjnotake; } },
      { "Build",          [&](){ stream >> sysdata->build_level; } },
      { "Protoflag",      [&](){ stream >> sysdata->level_modify_proto; } },
      { "Overridepriv",   [&](){ stream >> sysdata->level_override_private; } },
      { "Msetplayer",     [&](){ stream >> sysdata->level_mset_player; } },
      { "Stunplrvsplr",   [&](){ stream >> sysdata->stun_plr_vs_plr; } },
      { "Stunregular",    [&](){ stream >> sysdata->stun_regular; } },
      { "Gougepvp",       [&](){ stream >> sysdata->gouge_plr_vs_plr; } },
      { "Gougenontank",   [&](){ stream >> sysdata->gouge_nontank; } },
      { "Dodgemod",       [&](){ stream >> sysdata->dodge_mod; } },
      { "Parrymod",       [&](){ stream >> sysdata->parry_mod; } },
      { "Tumblemod",      [&](){ stream >> sysdata->tumble_mod; } },
      { "Damplrvsplr",    [&](){ stream >> sysdata->dam_plr_vs_plr; } },
      { "Damplrvsmob",    [&](){ stream >> sysdata->dam_plr_vs_mob; } },
      { "Dammobvsplr",    [&](){ stream >> sysdata->dam_mob_vs_plr; } },
      { "Dammobvsmob",    [&](){ stream >> sysdata->dam_mob_vs_mob; } },
      { "Forcepc",        [&](){ stream >> sysdata->level_forcepc; } },
      { "Bestowdif",      [&](){ stream >> sysdata->bestow_dif; } },
      { "PetSave",        [&](){ stream >> sysdata->save_pets; } },
      { "Wizlock",        [&](){ stream >> sysdata->WIZLOCK; } },
      { "Implock",        [&](){ stream >> sysdata->IMPLOCK; } },
      { "Lockdown",       [&](){ stream >> sysdata->LOCKDOWN; } },
      { "Admin_Email",    [&](){ sysdata->admin_email = fread_line( stream ); } },
      { "Newbie_purge",   [&](){ stream >> sysdata->newbie_purge; } },
      { "Regular_purge",  [&](){ stream >> sysdata->regular_purge; } },
      { "Autopurge",      [&](){ stream >> sysdata->CLEANPFILES; } },
      { "Testmode",       [&](){ stream >> sysdata->TESTINGMODE; } },
      { "Mapsize",        [&](){ stream >> sysdata->mapsize; } },
      { "Telnet",         [&](){ sysdata->telnet = fread_line( stream ); } },
      { "HTTP",           [&](){ sysdata->http = fread_line( stream ); } },
      { "Maxvnum",        [&](){ stream >> sysdata->maxvnum; } },
      { "Minguild",       [&](){ stream >> sysdata->minguildlevel; } },
      { "Maxcond",        [&](){ stream >> sysdata->maxcondval; } },
      { "Maxignore",      [&](){ stream >> sysdata->maxign; } },
      { "Maximpact",      [&](){ stream >> sysdata->maximpact; } },
      { "Maxholiday",     [&](){ stream >> sysdata->maxholiday; } },
      { "Initcond",       [&](){ stream >> sysdata->initcond; } },
      { "Secpertick",     [&](){ stream >> sysdata->secpertick; } },
      { "Pulsepersec",    [&](){ stream >> sysdata->pulsepersec; } },
      { "Hoursperday",    [&](){ stream >> sysdata->hoursperday; } },
      { "Daysperweek",    [&](){ stream >> sysdata->daysperweek; } },
      { "Dayspermonth",   [&](){ stream >> sysdata->dayspermonth; } },
      { "Monthsperyear",  [&](){ stream >> sysdata->monthsperyear; } },
      { "Minego",         [&](){ stream >> sysdata->minego; } },
      { "Rebootcount",    [&](){ stream >> sysdata->rebootcount; } },
      { "Auctionseconds", [&](){ stream >> sysdata->auctionseconds; } },
      { "Gameloopalarm",  [&](){ stream >> sysdata->gameloopalarm; } },
      { "Webwho",         [&](){ stream >> sysdata->webwho; } }
   };

   std::string key;
   while( stream >> key )
   {
      if( key == "#SYSTEM" )
         continue;

      if( key == "#END" || key == "End" )
         break;

      if( key == "Motd" )
      {
         std::time_t t;
         stream >> t;
         sysdata->motd = std::chrono::system_clock::from_time_t(t);
      }
      else if( key == "Imotd" )
      {
         std::time_t t;
         stream >> t;
         sysdata->imotd = std::chrono::system_clock::from_time_t(t);
      }
      else if( key == "Savefreq" )
      {
         int f;
         stream >> f;
         sysdata->save_frequency = std::chrono::minutes(f);
      }
      else if( key == "Saveflags" )
      {
         if( file_ver < 1 )
            stream >> sysdata->save_flags;
         else
         {
            std::string flags = fread_line( stream );

            flag_string_set( flags, sysdata->save_flags, save_flag );
         }
      }
      else if( loaders.contains( key ) )
      {
         loaders.at( key )();
      }
      else
      {
         bug( "{}: Invalid key in sysdata file: {}", __func__, key );
         continue;
      }
   }
   stream.close();

   update_timers(  );
   update_calendar(  );

   return true;
}

/*
 * Wizlist builder - Thoric
 */
void add_to_wizlist( std::string_view name, int level )
{
   auto wiz = std::make_unique<wizent>();

   wiz->name = name;
   wiz->level = level;

   wizlist.push_back( std::move( wiz ) );
}

std::string get_title( int level )
{
   int offset = MAX_LEVEL - level;

   switch( offset )
   {
      case 0:  return "Supreme Entity";
      case 1:  return "Realm Lords";
      case 2:  return "Eternals";
      case 3:  return "Ancients";
      case 4:  return "Astral Gods";
      case 5:  return "Elemental Gods";
      case 6:  return "Dream Gods";
      case 7:  return "Greater Gods";
      case 8:  return "Gods";
      case 9:  return "Demi Gods";
      case 10: return "Deities";
      case 11: return "Saviors";
      case 12: return "Creators";
      case 13: return "Acolytes";
      case 14: return "Angels";
      case 15: return "Retired";
      case 16: return "Guests";
      default: return "Servants";
   }
}

void make_wizlist( )
{
   // Always start with an empty list.
   wizlist.clear();

   // Walk the file list in GOD_DIR.
   for( const auto& entry : std::filesystem::directory_iterator( GOD_DIR ) )
   {
      // An actual file entry and not another folder.
      if( entry.is_regular_file( ) )
      {
         std::ifstream file( entry.path( ) );
         std::string word;
         int ilevel = 0;

         while( file >> word )
         {
            if( word == "Level" )
            {
               file >> ilevel;
               break;
            }
         }
         add_to_wizlist( entry.path( ).filename( ).string( ), ilevel );
      }
   }

   // Sort the list by level.
   wizlist.sort( []( const std::unique_ptr<wizent>& a, const std::unique_ptr<wizent>& b ) { return a->level > b->level; } );

   // Open WIZLIST_FILE file for writing.
   std::ofstream stream( std::filesystem::path( WIZLIST_FILE ), std::ios::trunc );

   // Center the top banner with the MUD's name.
   stream << std::format( "{:^78}\n", std::format( "The Immortal Masters of {}", sysdata->mud_name ) );

   int current_level = -1;
   std::string line_buffer;

   // Add each of the entries to the output file.
   for( const auto& wiz : wizlist )
   {
      if( wiz->level != current_level )
      {
         if( !line_buffer.empty() )
            stream << std::format( "{:^78}\n ", line_buffer );

         stream << std::format( "\n{:^78}\n ", get_title( wiz->level ) );
         line_buffer.clear();
         current_level = wiz->level;
      }

      if( line_buffer.length() + wiz->name.length() > 70 )
      {
         stream << std::format( "{:^78}\n", line_buffer );
         line_buffer.clear();
      }
      line_buffer += ( line_buffer.empty() ? "" : " " ) + wiz->name;
   }

   if( !line_buffer.empty() )
      stream << std::format( "{:^78}\n", line_buffer );

   stream.close();
}

CMDF( do_makewizlist )
{
   make_wizlist(  );
   make_webwiz(  );
   build_wizinfo(  );
}

system_data::system_data(  )
{
}

system_data::~system_data(  )
{
}

/*
 * Big mama top level function.
 */
void boot_db( bool fCopyOver )
{
   short wear = 0;
   short x;

   std::filesystem::remove( BOOTLOG_FILE );
   boot_log( "{}", "---------------------[ Boot Log: Start ]--------------------" );
   log_string( "Database bootup starting." );
   fBootDb = true;   /* Supposed to help with EOF bugs, so it got moved up */

   // This is purely informational with the global definition at the top of the file. It initializes before getting this far.
   // Uses the Mersenne Twister algorithm. - Samson 6/1/2026.
   log_string( "Initializing random number generator." );

   log_string( "Loading sysdata configuration..." );

   // See class system_data in mud.h for details on each setting.
   sysdata = new system_data;

   sysdata->save_frequency = std::chrono::minutes( 20 );
   sysdata->save_flags.set(  );  // This defaults to turning on every save_flag
   sysdata->motd = current_time;
   sysdata->imotd = current_time;
   sysdata->dbserver = "localhost";
   sysdata->dbname = "afkdb";
   sysdata->dbuser = "dbuser";
   sysdata->dbpass = "dbpass";

   if( !load_systemdata(  ) )
   {
      log_string( "Not found. Creating new configuration." );
      sysdata->mud_name = "(Name not set)";
      update_timers(  );
      update_calendar(  );
      save_sysdata(  );
   }
   log_string( "Done: System Data." );

   log_string( "Initializing libdl support..." );
   /*
    * Open up a handle to the executable's symbol table for later use
    * when working with commands
    */
   sysdata->dlHandle = dlopen( nullptr, RTLD_NOW );
   if( !sysdata->dlHandle )
   {
      log_printf( "{}: Error opening local system executable as handle, please check compile flags.", __func__ );
      shutdown_mud( "libdl failure" );
      std::exit( EXIT_FAILURE );
   }
   log_string( "Done: libdl." );

#if defined(SQL)
   log_string( "Initializing MySQL support..." );
   init_mysql(  );
   add_event( 1800, ev_mysql_ping, nullptr );
   log_string( "Done: SQL." );
#endif

   log_string( "Verifying existence of login greeting..." );
   std::filesystem::path lbuf = GREETING_FILE;
   if( !std::filesystem::exists( lbuf ) )
   {
      bug( "{}: Login greeting not found!", __func__ );
      shutdown_mud( "Missing login greeting!" );
      std::exit( EXIT_FAILURE );
   }
   else
      log_string( "Login greeting located." );

   log_string( "Loading commands..." );
   load_commands(  );

#ifdef MULTIPORT
   log_string( "Loading shell commands..." );
   load_shellcommands(  );
#endif

   log_string( "Loading spec_funs..." );
   load_specfuns(  );

   log_string( "Loading helps..." );
   load_helps(  );

   load_mudchannels(  );

   log_string( "Loading socials..." );
   load_socials(  );

   log_string( "Loading Skill Table..." );
   load_skill_table(  );
   populate_skill_indexes(  );
   remap_slot_numbers(  ); /* must be after the sort */
   num_sorted_skills = num_skills;
   log_string( "Done: Skill Table." );

   log_string( "Loading classes" );
   load_classes(  );

   log_string( "Loading races" );
   load_races(  );

   log_string( "Loading Connection History" );
   load_connhistory(  );

   /*
    * Noplex's liquid system code 
    */
   log_string( "Loading liquid table" );
   load_liquids(  );

   log_string( "Loading mixture table" );
   load_mixtures(  );

   log_string( "Loading Herb Table..." );
   load_herb_table(  );
   log_string( "Done: Herb Table." );

   log_string( "Loading tongues..." );
   load_tongues(  );

   log_string( "Loading boards..." );
   load_boards(  );

   /*
    * abit/qbit code 
    */
   log_string( "Loading quest bit tables..." );
   load_bits(  );

   log_string( "Initializing globals." );
   nummobsloaded = 0;
   numobjsloaded = 0;
   physicalobjects = 0;
   cur_qobjs = 0;
   cur_qchars = 0;
   extracted_obj_queue = nullptr;
   extracted_char_queue = nullptr;
   quitting_char = nullptr;
   loading_char = nullptr;
   saving_char = nullptr;
   last_pkroom = 1;

   log_string( "Initializing global auction item." );
   init_auction();

   for( wear = 0; wear < MAX_WEAR; ++wear )
      for( x = 0; x < MAX_LAYERS; ++x )
         save_equipment[wear][x] = nullptr;

   /*
    * Set time and weather.
    */
   {
      log_string( "Setting time and weather." );

      if( !load_timedata(  ) )   /* Loads time from stored file if true - Samson 1-21-99 */
      {
         boot_log( "{}", "Resetting mud time based on current system time." );

         auto duration = current_time.time_since_epoch();
         long long total_seconds = std::chrono::duration_cast<std::chrono::seconds>(duration).count();

         const long long MUD_EPOCH_OFFSET = 650336715;
         long long tick_duration = sysdata->pulsetick / sysdata->pulsepersec;
         long long lhour = ( total_seconds - MUD_EPOCH_OFFSET ) / tick_duration;

         time_info.hour = lhour % sysdata->hoursperday;
         long long lday = lhour / sysdata->hoursperday;
         time_info.day = lday % sysdata->dayspermonth;
         long long lmonth = lday / sysdata->dayspermonth;
         time_info.month = lmonth % sysdata->monthsperyear;
         time_info.year = lmonth / sysdata->monthsperyear;
      }

      if( time_info.hour < sysdata->hoursunrise )
         time_info.sunlight = SUN_DARK;
      else if( time_info.hour < sysdata->hourdaybegin )
         time_info.sunlight = SUN_RISE;
      else if( time_info.hour < sysdata->hoursunset )
         time_info.sunlight = SUN_LIGHT;
      else if( time_info.hour < sysdata->hournightbegin )
         time_info.sunlight = SUN_SET;
      else
         time_info.sunlight = SUN_DARK;
   }

   if( !load_weathermap(  ) )
   {
      log_string( "Initializing new weather map." );
      InitializeWeatherMap(  );
   }
   else
      log_string( "Weather map data loaded." );

   log_string( "Loading holiday chart..." ); /* Samson 5-13-99 */
   load_holidays(  );

   /*
    * Assign gsn's for skills which need them.
    */
   log_string( "Assigning gsn's" );
   assign_gsn_data(  );

   log_string( "Loading DNS cache..." );  /* Samson 1-30-02 */
   load_dns(  );

   log_string( "Loading slay table..." ); /* Online slay table - Samson 8-3-98 */
   load_slays(  );

   log_string( "Loading overland map data..." );
   load_continents( AREA_FILE_ALARM );
   log_string( "...done loading overland data." );

   log_string( "Loading reserved names list." );
   load_reserved_names( );

   log_string( "Loading namegen file..." );
   load_name_generator( );

   /*
    * Read in all the area files.
    */
   {
      arealist.clear(  );
      area_nsort.clear(  );
      area_vsort.clear(  );
      log_string( "Reading in area files..." );
      std::ifstream stream( std::filesystem::path{AREA_LIST} );
      if( !stream.is_open(  ) )
      {
         log_string( "Cannot open area.lst file.");
         shutdown_mud( "Boot_db: Unable to open area list." );
         std::exit( EXIT_FAILURE );
      }

      // Why were these never initialized before?!?
      top_area = top_prog = top_room = top_exit = top_reset = top_mob_index = top_obj_index = 0;

      for( ;; )
      {
         if( stream.eof() )
         {
            bug( "{}: EOF encountered reading area list - no $ found at end of file.", __func__ );
            break;
         }

         strArea = fread_line( stream, '\n' );
         if( strArea[0] == '$' )
            break;

         set_alarm( AREA_FILE_ALARM );
         alarm_section = "boot_db: read area files";
         load_area_file( strArea, false );
         set_alarm( 0 );
      }
      stream.close();
      log_string( "...done reading in area files." );
   }

   strArea = "NO FILE";

   log_string( "Validating overland data with areas..." );
   validate_overland_data(  );

   log_string( "Setting Astral Walk target room." );
   if( !find_area( "astral.are" ) )
   {
      log_string( "Astral Plane zone not found. Spell will be disabled." );
      astral_target = -1;
   }
   else
   {
      astral_target = number_range( 4350, 4449 );  /* Added by Samson for Astral Walk spell. Chooses a random target room. */
      log_printf( "Astral Walk target room for this boot is: {}", astral_target );
   }

   log_string( "Loading ships..." );
   load_ships(  );

   load_runes(  );

   /*
    *   initialize supermob.
    *    must be done before reset_area!
    */
   init_supermob(  );

   /*
    * Fix up exits.
    * Declare db booting over.
    * Reset all areas once.
    * Load up the notes file.
    */
   log_string( "Fixing exits..." );
   fix_exits(  );

   log_string( "Loading prototype area files..." );
   load_buildlist(  );

   strArea = "NO FILE";

   log_string( "Fixing prototype zone exits..." );
   fix_exits(  );

   load_clans(  );
   load_realms( );
   load_deity(  );

   load_equipment_totals( fCopyOver ); /* Samson 10-16-98 - scans pfiles for rares */

   log_string( "Loading corpses..." );
   load_corpses(  );

   if( fCopyOver == true )
   {
      log_string( "Loading world state..." );
      load_world(  );
   }

   fBootDb = false;

   log_string( "Making wizlist" );
   make_wizlist(  );
   make_webwiz(  );

   log_string( "Building wizinfo" );
   build_wizinfo(  );

   log_string( "Loading MSSP Data..." );
   load_mssp_data( );

   log_string( "Initializing area reset events..." );
   {
      /*
       * Putting some random fuzz on this to scatter the times around more 
       */
      for( auto* area : arealist )
      {
         area->reset(  );
         area->last_resettime = current_time;
         add_event( number_range( ( area->reset_frequency * 60 ) / 2, 3 * ( area->reset_frequency * 60 ) / 2 ), ev_area_reset, area );
      }
   }

   log_string( "Loading auction sales list..." );  /* Samson 6-24-99 */
   load_sales(  );
   log_string( "Loading auction houses..." );   /* Samson 6-20-99 */
   load_aucvaults(  );

   /*
    * Clan/Guild shopkeepers - Samson 7-16-00 
    */
   log_string( "Loading clan/guild shops..." );
   load_shopkeepers(  );

   log_string( "Loading bans..." );
   load_banlist(  );

   log_string( "Loading auth namelist..." );
   load_auth_list(  );
   save_auth_list(  );

   log_string( "Loading Immortal Hosts..." );
   load_imm_host(  );

   log_string( "Loading Projects..." );
   load_projects(  );

   load_quotes(  );

   log_string( "Loading login messages..." );
   load_login_messages( );

   /*
    * Morphs MUST be loaded after Class and race tables are set up --Shaddai 
    */
   log_string( "Loading Morphs..." );
   load_morphs(  );

   MPSilent = false;
   MOBtrigger = true;

   /*
    * Initialize chess board stuff 
    */
   init_chess(  );

   if( sysdata->webwho > 0 )
      add_event( sysdata->webwho, ev_webwho_refresh, nullptr );
   web_arealist(  );

   /*
    * Initialize pruning events 
    */
   add_event( 86400, ev_pfile_check, nullptr );
   add_event( 86400, ev_board_check, nullptr );
   add_event( 1, ev_dns_check, nullptr );

   log_string( "Database bootup completed." );
   boot_log( "{}", "---------------------[ Boot Log: End ]--------------------" );
}

/* Removal of this function constitutes a license violation */
CMDF( do_basereport )
{
   ch->print_fmt( "&RCodebase revision: {} {} - {}\r\n", CODENAME, CODEVERSION, COPYRIGHT );
   ch->print( "&YContributors: Samson, Dwip, Whir, Cyberfox, Karangi, Rathian, Cam, Raine, and Tarl.\r\n" );
   ch->print( "&BDevelopment site: smaugmuds.afkmods.com\r\n" );
   ch->print( "&GThis function is included as a means to verify license compliance.\r\n" );
   ch->print( "Removal is a violation of your license.\r\n" );
   ch->print( "Copies of AFKMud beginning with 1.4 as of December 28, 2002 include this.\r\n" );
   ch->print( "Copies found to be running without this command will be subject to a violation report.\r\n" );
}

CMDF( do_memory )
{
   ch->print( "\r\n&wSystem Memory\r\n" );
   ch->print_fmt( "&wAffects: &W{:5}\t\t\t&wAreas:   &W{:5}\r\n", top_affect, top_area );
   ch->print_fmt( "&wExtDes:  &W{:5}\t\t\t&wExits:   &W{:5}\r\n", top_ed, top_exit );
   ch->print_fmt( "&wResets:  &W{:5}\r\n", top_reset );
   ch->print_fmt( "&wIdxMobs: &W{:5}\t\t\t&wMobiles: &W{:5}\r\n", top_mob_index, nummobsloaded );
   ch->print_fmt( "&wIdxObjs: &W{:5}\t\t\t&wObjs:    &W{:5}({})\r\n", top_obj_index, numobjsloaded, physicalobjects );
   ch->print_fmt( "&wRooms:   &W{:5}\r\n", top_room );
   ch->print_fmt( "&wShops:   &W{:5}\t\t\t&wRepShps: &W{:5}\r\n", top_shop, top_repair );
   ch->print_fmt( "&wCurOq's: &W{:5}\t\t\t&wCurCq's: &W{:5}\r\n", cur_qobjs, cur_qchars );
   ch->print_fmt( "&wPeople : &W{:5}\t\t\t&wMaxplrs: &W{:5}\r\n", num_descriptors, sysdata->maxplayers );
   ch->print_fmt( "&wPlayers: &W{:5}\r\n", sysdata->playersonline );
   ch->print_fmt( "&wMaxEver: &W{:5}\t\t\t&wTopsn:   &W{:5}({:5})\r\n", sysdata->alltimemax, num_skills, MAX_SKILL );
   ch->print_fmt( "&wMaxEver was recorded on:  &W{}\r\n\r\n", sysdata->time_of_max );
#if defined(SQL)
   ch->print_fmt( "&wMySQL Connection Active:  &W{}\r\n\r\n", ( db && db->ping() ) ? "YES" : ( db ? db->get_error() : "NO" ) );
#endif
}

/* Dummy code added to block number_fuzzy from messing things up - Samson 3-28-98 */
int number_fuzzy( int number )
{
   return number;
}

/*
 * Generate a random number.
 * Ooops was (number_mm() % to) + from which doesn't work -Shaddai
 * Changed to use rand() directly, since the system random is just fine. - Samson
 * Many years later, updated to use the new RNG system. - Samson 5/31/2026
 */
int number_range( int from, int to )
{
   if( from > to )
      std::swap( from, to );
   if( from == to )
      return from;

   static std::uniform_int_distribution<int> dist;
   using param_t = std::uniform_int_distribution<int>::param_type;
   return dist( global_rng, param_t( from, to ) );
}

/*
 * Generate a percentile roll.
 * number_mm() % 100 only does 0-99, changed to do 1-100 -Shaddai
 * Changed to use rand() directly, since the system random is just fine. - Samson
 * Many years later, updated to use the new RNG system. - Samson 5/31/2026
 */
int number_percent( void )
{
   static std::uniform_int_distribution<int> dist( 1, 100 );
   return dist( global_rng );
}

/*
 * Generate a random door.
 */
int number_door( void )
{
   // Replaces: rand() & (16 - 1) > 9
   // This is effectively a 0-9 range
   return number_range( 0, 9 );
}

int number_bits( int width )
{
   // Generates a value in range [0, 2^width - 1]
   return number_range( 0, (1 << width) - 1 );
}

/*
 * Roll some dice.   -Thoric
 */
int dice( int number, int size )
{
   if( size <= 0 )
      return 0;

   if( size == 1 )
      return number;

   int sum = 0;
   for( int i = 0; i < number; ++i )
      sum += number_range( 1, size );
   return sum;
}

CMDF( do_randtest )
{
   ch->print_fmt( "Utterly random number   : {}\r\n", global_rng( ) );
   ch->print_fmt( "number_range 4350 - 4449: {}\r\n", number_range( 4350, 4449 ) );
   ch->print_fmt( "number_percent          : {}\r\n", number_percent(  ) );
   ch->print_fmt( "number_door             : {}\r\n", number_door(  ) );
   ch->print_fmt( "number_bits 5           : {}\r\n", number_bits( 5 ) );
   ch->print_fmt( "3d35 ( 3 35 sided dice ): {}\r\n", dice( 3, 35 ) );
}

// Command so imms can view the imotd - Samson 1-20-01
CMDF( do_imotd )
{
   if( exists_file( IMOTD_FILE ) )
      show_file( ch, IMOTD_FILE );
}

// Command so people can call up the MOTDs - Samson 1-20-01
CMDF( do_motd )
{
   if( exists_file( MOTD_FILE ) )
      show_file( ch, MOTD_FILE );
}

// Saves MOTDs to disk - Samson 12-31-00
void save_motd( std::string_view name, std::string_view str )
{
   std::ofstream stream( std::filesystem::path{name} );

   if( !stream )
   {
      bug( "{}: Cannot open {} for writing: {}", __func__, name, std::strerror(errno) );
      return;
   }

   stream << str;
   stream.close();

   if( stream.fail() )
      bug( "{}: Error occurred after closing {}: ", __func__, name, std::strerror(errno) );
}

void load_motd( char_data * ch, std::string_view name )
{
   std::ifstream stream( std::filesystem::path{name} );

   if( !stream )
   {
      bug( "{}: Cannot open {} for reading: {}", __func__, name, std::strerror(errno) );
      return;
   }

   std::stringstream buffer;
   buffer << stream.rdbuf();
   ch->pcdata->motd_buf = buffer.str();

   stream.close();
}

// Handles editing the MOTDs on the server, independent of helpfiles now - Samson 12-31-00
CMDF( do_motdedit )
{
   std::string arg1;

   if( ch->isnpc(  ) )
   {
      ch->print( "Huh?\r\n" );
      return;
   }

   switch ( ch->substate )
   {
      default:
         break;

      case SUB_RESTRICTED:
         ch->print( "You cannot do this while in another command.\r\n" );
         return;

      case SUB_EDMOTD:
         ch->pcdata->motd_buf = ch->copy_buffer( );
         ch->stop_editing(  );
         ch->substate = ch->tempnum;
         return;

      case SUB_EDIT_ABORT:
         ch->substate = SUB_NONE;
         ch->print( "Aborting MOTD message.\r\n" );
         return;
   }

   argument = one_argument( argument, arg1 );

   if( arg1.empty(  ) )
   {
      ch->print( "Usage: motdedit <field>\r\n" );
      ch->print( "Usage: motdedit save <field>\r\n\r\n" );
      ch->print( "Field can be one of:\r\n" );
      ch->print( "motd imotd\r\n" );
      return;
   }

   if( !str_cmp( arg1, "save" ) )
   {
      if( ch->pcdata->motd_buf.empty() )
      {
         ch->print( "Nothing to save.\r\n" );
         return;
      }
      if( !str_cmp( argument, "motd" ) )
      {
         save_motd( MOTD_FILE, ch->pcdata->motd_buf );
         ch->print( "MOTD Message updated.\r\n" );
         ch->pcdata->motd_buf.clear();
         sysdata->motd = current_time;
         save_sysdata(  );
         return;
      }
      if( !str_cmp( argument, "imotd" ) )
      {
         save_motd( IMOTD_FILE, ch->pcdata->motd_buf );
         ch->print( "IMOTD Message updated.\r\n" );
         ch->pcdata->motd_buf.clear();
         sysdata->imotd = current_time;
         save_sysdata(  );
         return;
      }
      do_motdedit( ch, "" );
      return;
   }

   if( !str_cmp( arg1, "motd" ) || !str_cmp( arg1, "imotd" ) )
   {
      if( !str_cmp( arg1, "motd" ) )
         load_motd( ch, MOTD_FILE );
      else
         load_motd( ch, IMOTD_FILE );
      if( ch->substate == SUB_REPEATCMD )
         ch->tempnum = SUB_REPEATCMD;
      else
         ch->tempnum = SUB_NONE;
      ch->substate = SUB_EDMOTD;
      ch->pcdata->dest_buf = ch;
      ch->pcdata->motd_buf.clear();
      ch->start_editing( ch->pcdata->motd_buf );
      ch->set_editor_desc( "An MOTD." );
      return;
   }
   do_motdedit( ch, "" );
}

/*
 * Believe it or not, this little gem originated as a code example in a Google search.
 * I've modified what Google AI provided to cut the filename down to the actual source code file.
 * Output of the results gets sent to the standard game log. - Samson 1/11/2025
 *
 * Note: This requires compiling with C++23 support. Currently C++23 is available with GCC 13 and later.
 * If your OS does not have the appropriate library then bug() will still work, you just won't get the traces.
 */
#if defined(HAVE_CXXABI)
std::string demangle( const std::string & name )
{
   int status = -1;

   std::unique_ptr<char, void(*)(void*)> res
   {
      abi::__cxa_demangle( name.c_str(), nullptr, nullptr, &status ), std::free
   };
   return ( status == 0 ) ? res.get() : name;
}

void generate_backtrace( void )
{
   std::stacktrace trace = std::stacktrace::current();
   std::ostringstream lines;

   lines << std::endl << "Obtained " << trace.size() << " stack frames:" << std::endl << std::endl;

   for( const auto& frame : trace )
   {
      std::string::size_type pos = frame.source_file().find_last_of( "/", frame.source_file().length() );
      std::string file_name;

      if( pos != std::string::npos )
         file_name = frame.source_file().substr( pos + 1 );
      else
         file_name = frame.source_file();

      std::string func_name = demangle( frame.description() );
      lines << frame.description() << " -> " << file_name << ":" << frame.source_line() << std::endl;
   }
   log_string( lines.str( ) );
}
#endif

/*
 * Reports a bug.
 *
 * In the function call for this, you can specify the following:
 *
 * __func__ The function name where "bug" was called from.    Type = *char, uses %s formatting.
 * __FILE__ The source code file where "bug" was called from. Type = *char, uses %s formatting.
 * __LINE__ The line of the file where "bug" was called from. Type = int,   uses %d formatting.
 *
 * Example:
 *
 * bug( "{} -> {}:{}: Backtrace test.", __func__, __FILE__, __LINE__ );
 *
 * It's helpful to include the file and line tags since for some odd reason the backtrace code won't include the function that called "bug".
 */
void process_bug( std::string_view text )
{
   log_printf_plus( LOG_DEBUG, LEVEL_IMMORTAL, "[*****] BUG: {}", text );

   if( fpArea.is_open() )
   {
      std::streampos current_pos = fpArea.tellg();

      fpArea.clear();
      fpArea.seekg( 0, std::ios::beg );

      int iLine = 1;
      char c;
      while( fpArea.tellg() < current_pos && fpArea.get(c) )
      {
         if( c == '\n' )
            iLine++;
      }

      fpArea.clear();
      fpArea.seekg( current_pos );

      log_printf( "[*****] FILE: {} LINE: {}", strArea, iLine );
   }

#if defined(HAVE_CXXABI)
   if( !fBootDb )
   {
      generate_backtrace();
   }
#endif
}

/*
 * Writes a string to the log, extended version - Thoric
 */
void log_string_plus( short log_type, short level, std::string_view str )
{
   std::cerr << std::format( "{} :: {}\n", c_time( current_time, "" ), str );

   std::string_view newstr = str;
   if( newstr.starts_with( "Log " ) )
   {
      newstr.remove_prefix( 4 );
   }

   switch ( log_type )
   {
      default:
         to_channel( newstr, "Log", level );
         break;
      case LOG_BUILD:
         to_channel( newstr, "Build", level );
         break;
      case LOG_COMM:
         to_channel( newstr, "Comm", level );
         break;
      case LOG_WARN:
         to_channel( newstr, "Warn", level );
         break;
      case LOG_INFO:
         to_channel( newstr, "Info", level );
         break;
      case LOG_AUTH:
         to_channel( newstr, "Auth", level );
         break;
      case LOG_DEBUG:
         to_channel( newstr, "Bugs", level );
         break;
      case LOG_ALL:
         break;
   }
}

void log_string( std::string_view txt )
{
   log_string_plus( LOG_NORMAL, LEVEL_LOG, txt );
}
