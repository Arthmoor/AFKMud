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
 *                       Database management module                         *
 ****************************************************************************/

// For libdl - Trax
#if !defined(WIN32)
#include <dlfcn.h>
#else
#include <windows.h>
#define dlopen( libname, flags ) LoadLibrary( (libname) )
#endif

// For backtrace info in logging functions.
#if !defined(__CYGWIN__) && !defined(__FreeBSD__) && !defined(WIN32)
#include <cxxabi.h>
#endif

#include <chrono>
#include <cmath>
#include <cstdarg>
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <stacktrace>
#include "mud.h"
#include "area.h"
#include "auction.h"
#include "bits.h"
#include "connhist.h"
#include "event.h"
#include "mobindex.h"
#include "mud_prog.h"
#include "objindex.h"
#include "overland.h"
#include "pfiles.h"
#include "roomindex.h"
#include "shops.h"
#include "weather.h"

#if !defined(__CYGWIN__) && defined(SQL)
 #include "sql.h"
#endif

namespace fs = std::filesystem;

/*
 * Change this alarm timer to whatever you think is appropriate.
 * Adjust if you are getting infinite loop warnings that are shutting down bootup.
 */
const int AREA_FILE_ALARM = 45;

system_data *sysdata;

/*
 * Structure used to build wizlist. No destructor needed as it uses std::unique_ptr
 */
class wizent
{
 public:
   wizent( ) : level( 0 ) {}

   string name;
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
extern obj_data *extracted_obj_queue;
extern struct extracted_char_data *extracted_char_queue;

int cur_qobjs;
int cur_qchars;
int nummobsloaded;
int numobjsloaded;
int physicalobjects;
int last_pkroom;

auction_data *auction;  /* auctions */

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
FILE *fpArea;
char strArea[MIL];

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
void to_channel( const string &, const string &, int );
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
int mob_xp( char_data * );
void load_connhistory(  );
void sort_skill_table(  );
void load_classes(  );
void load_races(  );
void load_herb_table(  );
void load_tongues(  );
void load_helps(  );
void load_loginmsg(  );
void init_chess(  );
void load_continents( const int );
void validate_overland_data(  );
void make_webwiz(  );
std::string check_hash( const char * );
std::string hash_stats( );

affect_data::affect_data(  )
{
   init_memory( &rismod, &type, sizeof( type ) );
}

void shutdown_mud( const string & reason )
{
   FILE *fp;

   if( ( fp = fopen( SHUTDOWN_FILE, "a" ) ) != nullptr )
   {
      fprintf( fp, "%s\n", reason.c_str(  ) );
      FCLOSE( fp );
   }
}

// FIXME: Remove this when all uses are using std::filesystem instead.
bool exists_file( const string & name )
{
   fs::path filename = name;

   /*
    * Stands to reason that if there ain't a name to look at, it damn well don't exist! 
    */
   if( name.empty(  ) )
      return false;

   if( fs::exists( filename ) )
      return true;
   else
      return false;
}

bool is_valid_filename( char_data * ch, const string & direct, const string & filename )
{
   fs::path newfilename;

   /*
    * Length restrictions 
    */
   if( filename.empty(  ) || filename.length(  ) < 3 )
   {
      if( filename.empty(  ) )
         ch->print( "Empty filename is not valid.\r\n" );
      else
         ch->printf( "%s: Filename is too short.\r\n", filename.c_str(  ) );
      return false;
   }

   /*
    * Illegal characters 
    */
   if( strstr( filename.c_str(  ), ".." ) || strstr( filename.c_str(  ), "/" ) || strstr( filename.c_str(  ), "\\" ) )
   {
      ch->print( "A filename may not contain a '..', '/', or '\\' in it.\r\n" );
      return false;
   }

   /*
    * If that filename is already being used lets not allow it now to be on the safe side 
    */
   newfilename = std::format( "{}{}", direct, filename );
   if( fs::exists( newfilename ) )
   {
      ch->printf( "%s is already an existing filename.\r\n", newfilename.c_str() );
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
 * encounter EOF will return what they have read so far.   These files
 * should include player files, and in-progress areas that are not loaded
 * upon bootup.
 * -- Altrag
 */
/*
 * Read a letter from a file.
 */
char fread_letter( FILE * fp )
{
   char c;

   do
   {
      if( feof( fp ) )
      {
         bug( "%s: EOF encountered on read.", __func__ );
         if( fBootDb )
         {
            shutdown_mud( "Corrupt file somewhere." );
            exit( 1 );
         }
         return '\0';
      }
      c = getc( fp );
   }
   while( isspace( c ) );

   return c;
}

/*
 * Read a float number from a file. Turn the result into a float value.
 */
float fread_float( FILE * fp )
{
   float number;
   bool sign, decimal;
   char c;
   double place = 0;

   do
   {
      if( feof( fp ) )
      {
         bug( "%s: EOF encountered on read.", __func__ );
         if( fBootDb )
         {
            shutdown_mud( "Corrupt file somewhere." );
            exit( 1 );
         }
         return 0;
      }
      c = getc( fp );
   }
   while( isspace( c ) );

   number = 0;

   sign = false;
   decimal = false;

   if( c == '+' )
      c = getc( fp );
   else if( c == '-' )
   {
      sign = true;
      c = getc( fp );
   }

   if( !isdigit( c ) )
   {
      bug( "%s: bad format. (%c)", __func__, c );
      if( fBootDb )
         exit( 1 );
      return 0;
   }

   while( 1 )
   {
      if( c == '.' || isdigit( c ) )
      {
         if( c == '.' )
         {
            decimal = true;
            c = getc( fp );
         }

         if( feof( fp ) )
         {
            bug( "%s: EOF encountered on read.", __func__ );
            if( fBootDb )
               exit( 1 );
            return number;
         }
         if( !decimal )
            number = number * 10 + c - '0';
         else
         {
            ++place;
            number += pow( 10, ( -1 * place ) ) * ( c - '0' );
         }
         c = getc( fp );
      }
      else
         break;
   }

   if( sign )
      number = 0 - number;

   if( c == '|' )
      number += fread_float( fp );
   else if( c != ' ' )
      ungetc( c, fp );

   return number;
}

/*
 * Read a number from a file. Convert to long integer.
 */
long fread_long( FILE * fp )
{
   long number;
   bool sign;
   char c;

   do
   {
      if( feof( fp ) )
      {
         bug( "%s: EOF encountered on read.", __func__ );
         if( fBootDb )
         {
            shutdown_mud( "Corrupt file somewhere." );
            exit( 1 );
         }
         return 0;
      }
      c = getc( fp );
   }
   while( isspace( c ) );

   number = 0;

   sign = false;
   if( c == '+' )
      c = getc( fp );
   else if( c == '-' )
   {
      sign = true;
      c = getc( fp );
   }

   if( !isdigit( c ) )
   {
      bug( "%s: bad format. (%c)", __func__, c );
      if( fBootDb )
         exit( 1 );
      return 0;
   }

   while( isdigit( c ) )
   {
      if( feof( fp ) )
      {
         bug( "%s: EOF encountered on read.", __func__ );
         if( fBootDb )
            exit( 1 );
         return number;
      }
      number = number * 10 + c - '0';
      c = getc( fp );
   }

   if( sign )
      number = 0 - number;

   if( c == '|' )
      number += fread_long( fp );
   else if( c != ' ' )
      ungetc( c, fp );

   return number;
}

/*
 * Read a number from a file. Convert to short integer.
 */
short fread_short( FILE * fp )
{
   short number;
   bool sign;
   char c;

   do
   {
      if( feof( fp ) )
      {
         bug( "%s: EOF encountered on read.", __func__ );
         if( fBootDb )
         {
            shutdown_mud( "Corrupt file somewhere." );
            exit( 1 );
         }
         return 0;
      }
      c = getc( fp );
   }
   while( isspace( c ) );

   number = 0;

   sign = false;
   if( c == '+' )
      c = getc( fp );
   else if( c == '-' )
   {
      sign = true;
      c = getc( fp );
   }

   if( !isdigit( c ) )
   {
      bug( "%s: bad format. (%c)", __func__, c );
      if( fBootDb )
         exit( 1 );
      return 0;
   }

   while( isdigit( c ) )
   {
      if( feof( fp ) )
      {
         bug( "%s: EOF encountered on read.", __func__ );
         if( fBootDb )
            exit( 1 );
         return number;
      }
      number = number * 10 + c - '0';
      c = getc( fp );
   }

   if( sign )
      number = 0 - number;

   if( c == '|' )
      number += fread_short( fp );
   else if( c != ' ' )
      ungetc( c, fp );

   return number;
}

/*
 * Read a number from a file.
 */
int fread_number( FILE * fp )
{
   int number;
   bool sign;
   char c;

   do
   {
      if( feof( fp ) )
      {
         bug( "%s: EOF encountered on read.", __func__ );
         if( fBootDb )
         {
            shutdown_mud( "Corrupt file somewhere." );
            exit( 1 );
         }
         return 0;
      }
      c = getc( fp );
   }
   while( isspace( c ) );

   number = 0;

   sign = false;
   if( c == '+' )
      c = getc( fp );
   else if( c == '-' )
   {
      sign = true;
      c = getc( fp );
   }

   if( !isdigit( c ) )
   {
      bug( "%s: bad format. (%c)", __func__, c );
      if( fBootDb )
         exit( 1 );
      return 0;
   }

   while( isdigit( c ) )
   {
      if( feof( fp ) )
      {
         bug( "%s: EOF encountered on read.", __func__ );
         if( fBootDb )
            exit( 1 );
         return number;
      }
      number = number * 10 + c - '0';
      c = getc( fp );
   }

   if( sign )
      number = 0 - number;

   if( c == '|' )
      number += fread_number( fp );
   else if( c != ' ' )
      ungetc( c, fp );

   return number;
}

/*
 * Read a string of text based flags from file fp. Ending in ~
 */
const char *fread_flagstring( FILE * fp )
{
   static char buf[MSL];
   char *plast;
   char c;
   int ln;

   plast = buf;
   buf[0] = '\0';
   ln = 0;

   /*
    * Skip blanks.
    * Read first char.
    */
   do
   {
      if( feof( fp ) )
      {
         bug( "%s: EOF encountered on read.", __func__ );
         if( fBootDb )
         {
            shutdown_mud( "Corrupt file somewhere." );
            exit( 1 );
         }
         return "";
      }
      c = getc( fp );
   }
   while( isspace( c ) );

   if( ( *plast++ = c ) == '~' )
      return "";

   for( ;; )
   {
      if( ln >= ( MSL - 1 ) )
      {
         bug( "%s: string too long", __func__ );
         *plast = '\0';
         return buf;
      }
      switch ( *plast = getc( fp ) )
      {
         default:
            ++plast;
            ++ln;
            break;

         case EOF:
            bug( "%s: EOF", __func__ );
            if( fBootDb )
               exit( 1 );
            *plast = '\0';
            return buf;

         case '\n':
            ++plast;
            ++ln;
            *plast++ = '\r';
            ++ln;
            break;

         case '\r':
            break;

         case '~':
            *plast = '\0';
            return buf;
      }
   }
}

/* Read a string from file and allocate it to the shared string hash */
char *fread_string( FILE * fp )
{
   char buf[MSL];

   strlcpy( buf, fread_flagstring( fp ), MSL );

   if( !str_cmp( buf, "" ) )
      return nullptr;
   return STRALLOC( buf );
}

/* Read a string from a file and assign it to a std::string */
void fread_string( string & newstring, FILE * fp )
{
   char buf[MSL];

   strlcpy( buf, fread_flagstring( fp ), MSL );
   newstring = buf;
}

/* Read a string from file fp using strdup (ie: no string hashing)
 * This will probably become obsolete after the std::string conversions are done.
 */
char *fread_string_nohash( FILE * fp )
{
   char buf[MSL];

   strlcpy( buf, fread_flagstring( fp ), MSL );
   return strdup( buf );
}

/*
 * Read to end of line (for comments).
 */
void fread_to_eol( FILE * fp )
{
   char c;

   do
   {
      if( feof( fp ) )
      {
         bug( "%s: EOF encountered on read.", __func__ );
         if( fBootDb )
         {
            shutdown_mud( "Corrupt file somewhere." );
            exit( 1 );
         }
         return;
      }
      c = getc( fp );
   }
   while( c != '\n' && c != '\r' );

   do
   {
      c = getc( fp );
   }
   while( c == '\n' || c == '\r' );

   ungetc( c, fp );
}

/*
 * Read to end of line into static buffer - Thoric
 */
const char *fread_line( FILE * fp )
{
   static char line[MSL];
   char *pline;
   char c;
   int ln;

   pline = line;
   line[0] = '\0';
   ln = 0;

   /*
    * Skip blanks.
    * Read first char.
    */
   do
   {
      if( feof( fp ) )
      {
         bug( "%s: EOF encountered on read.", __func__ );
         if( fBootDb )
         {
            shutdown_mud( "Corrupt file somewhere." );
            exit( 1 );
         }
         return "";
      }
      c = getc( fp );
   }
   while( isspace( c ) );

   ungetc( c, fp );

   do
   {
      if( feof( fp ) )
      {
         bug( "%s: EOF encountered on read.", __func__ );
         if( fBootDb )
            exit( 1 );
         *pline = '\0';
         return line;
      }
      c = getc( fp );
      *pline++ = c;
      ++ln;
      if( ln >= ( MSL - 1 ) )
      {
         bug( "%s: line too long", __func__ );
         break;
      }
   }
   while( c != '\n' && c != '\r' );

   do
      c = getc( fp );
   while( c == '\n' || c == '\r' );

   ungetc( c, fp );
   --pline;
   *pline = '\0';
   return line;
}

void fread_line( string & newstring, FILE * fp )
{
   char buf[MSL];

   strlcpy( buf, fread_line( fp ), MSL );
   if( buf[strlen( buf ) - 1] == '\n' || buf[strlen( buf ) - 1] == '\r' )
      buf[strlen( buf ) - 1] = '\0';
   newstring = buf;
}

/*
 * Read one word (into static buffer).
 */
char *fread_word( FILE * fp )
{
   static char word[MIL];
   char *pword;
   char cEnd;

   do
   {
      if( feof( fp ) )
      {
         bug( "%s: EOF encountered on read.", __func__ );
         if( fBootDb )
         {
            shutdown_mud( "Corrupt file somewhere." );
            exit( 1 );
         }
         word[0] = '\0';
         return word;
      }
      cEnd = getc( fp );
   }
   while( isspace( cEnd ) );

   if( cEnd == '\'' || cEnd == '"' )
      pword = word;
   else
   {
      word[0] = cEnd;
      pword = word + 1;
      cEnd = ' ';
   }

   for( ; pword < word + MIL; ++pword )
   {
      if( feof( fp ) )
      {
         bug( "%s: EOF encountered on read.", __func__ );
         if( fBootDb )
            exit( 1 );
         *pword = '\0';
         return word;
      }
      *pword = getc( fp );
      if( cEnd == ' ' ? isspace( *pword ) : *pword == cEnd )
      {
         if( cEnd == ' ' )
            ungetc( *pword, fp );
         *pword = '\0';
         return word;
      }
   }
   bug( "%s: word too long", __func__ );
   *--pword = '\0';
   return word;
}

// FIXME: Tagging this for upgrade to std::format. Many places call this. Follow example from character.cpp
/*
 * Add a string to the boot-up log - Thoric
 */
void boot_log( const char *str, ... )
{
   char buf[MSL];
   FILE *fp;
   va_list param;

   va_start( param, str );
   vsnprintf( buf, MSL, str, param );
   va_end( param );

   log_printf( "[*****] BOOT: %s", buf );

   if( ( fp = fopen( BOOTLOG_FILE, "a" ) ) != nullptr )
   {
      fprintf( fp, "%s\n", buf );
      FCLOSE( fp );
   }
}

/* Build list of in progress areas. Do not load areas.
 * define AREA_READ if you want it to build area names rather than reading
 * them out of the area files. -- Altrag */
/* The above info is obsolete - this will now simply load whatever is in the
 * BUILD_DIR and assume it to be a valid prototype zone. -- Samson 2-13-04
 */
void load_buildlist( void )
{
   if( !fs::exists( BUILD_DIR ) || !fs::is_directory( BUILD_DIR ) )
   {
      // This should be treated as fatal.
      bug( "%s: Builder directory is missing!", __func__ );
      exit( 1 );
   }

   for( const auto& entry : fs::directory_iterator( BUILD_DIR ) )
   {
      std::string filename = entry.path().filename().string();

      if( filename.empty() || filename[0] == '.' )
         continue;
      if( filename.find( ".are" ) == std::string::npos )
         continue;
      if( filename.find( ".bak" ) != std::string::npos )
         continue;

      fs::path full_path = fs::path( BUILD_DIR ) / filename;

      strlcpy( strArea, filename.c_str(), MIL );
      set_alarm( AREA_FILE_ALARM );
      alarm_section = "load_buildlist: read prototype area files";

      load_area_file( full_path.string().c_str(), true );
   }
}

const int SYSFILEVER = 1;
/*
 * Save system info to data file
 */
void save_sysdata( void )
{
   FILE *fp;
   fs::path filename;

   filename = std::format( "{}sysdata.dat", SYSTEM_DIR );

   if( !( fp = fopen( filename.c_str(), "w" ) ) )
   {
      bug( "%s: fopen", __func__ );
      perror( filename.c_str() );
   }
   else
   {
      fprintf( fp, "%s", "#SYSTEM\n" );
      fprintf( fp, "Version        %d\n", SYSFILEVER );
      fprintf( fp, "MudName        %s~\n", sysdata->mud_name.c_str(  ) );
      fprintf( fp, "Password       %s~\n", sysdata->password.c_str(  ) );
      fprintf( fp, "Dbserver       %s~\n", sysdata->dbserver.c_str(  ) );
      fprintf( fp, "Dbname         %s~\n", sysdata->dbname.c_str(  ) );
      fprintf( fp, "Dbuser         %s~\n", sysdata->dbuser.c_str(  ) );
      fprintf( fp, "Dbpass         %s~\n", sysdata->dbpass.c_str(  ) );
      fprintf( fp, "Highplayers    %d\n", sysdata->alltimemax );
      fprintf( fp, "Highplayertime %s~\n", sysdata->time_of_max.c_str(  ) );
      fprintf( fp, "CheckImmHost   %d\n", sysdata->check_imm_host );
      fprintf( fp, "Nameresolving  %d\n", sysdata->NO_NAME_RESOLVING );
      fprintf( fp, "Waitforauth    %d\n", sysdata->WAIT_FOR_AUTH );
      fprintf( fp, "Readallmail    %d\n", sysdata->read_all_mail );
      fprintf( fp, "Readmailfree   %d\n", sysdata->read_mail_free );
      fprintf( fp, "Writemailfree  %d\n", sysdata->write_mail_free );
      fprintf( fp, "Takeothersmail %d\n", sysdata->take_others_mail );
      fprintf( fp, "Getnotake      %d\n", sysdata->level_getobjnotake );
      fprintf( fp, "Build          %d\n", sysdata->build_level );
      fprintf( fp, "Protoflag      %d\n", sysdata->level_modify_proto );
      fprintf( fp, "Overridepriv   %d\n", sysdata->level_override_private );
      fprintf( fp, "Msetplayer     %d\n", sysdata->level_mset_player );
      fprintf( fp, "Stunplrvsplr   %d\n", sysdata->stun_plr_vs_plr );
      fprintf( fp, "Stunregular    %d\n", sysdata->stun_regular );
      fprintf( fp, "Gougepvp       %d\n", sysdata->gouge_plr_vs_plr );
      fprintf( fp, "Gougenontank   %d\n", sysdata->gouge_nontank );
      fprintf( fp, "Bashpvp        %d\n", sysdata->bash_plr_vs_plr );
      fprintf( fp, "Bashnontank    %d\n", sysdata->bash_nontank );
      fprintf( fp, "Dodgemod       %d\n", sysdata->dodge_mod );
      fprintf( fp, "Parrymod       %d\n", sysdata->parry_mod );
      fprintf( fp, "Tumblemod      %d\n", sysdata->tumble_mod );
      fprintf( fp, "Damplrvsplr    %d\n", sysdata->dam_plr_vs_plr );
      fprintf( fp, "Damplrvsmob    %d\n", sysdata->dam_plr_vs_mob );
      fprintf( fp, "Dammobvsplr    %d\n", sysdata->dam_mob_vs_plr );
      fprintf( fp, "Dammobvsmob    %d\n", sysdata->dam_mob_vs_mob );
      fprintf( fp, "Forcepc        %d\n", sysdata->level_forcepc );
      fprintf( fp, "Saveflags      %s~\n", bitset_string( sysdata->save_flags, save_flag ) );
      fprintf( fp, "Savefreq       %d\n", sysdata->save_frequency );
      fprintf( fp, "Bestowdif      %d\n", sysdata->bestow_dif );
      fprintf( fp, "PetSave        %d\n", sysdata->save_pets );
      fprintf( fp, "Wizlock        %d\n", sysdata->WIZLOCK );
      fprintf( fp, "Implock        %d\n", sysdata->IMPLOCK );
      fprintf( fp, "Lockdown       %d\n", sysdata->LOCKDOWN );
      fprintf( fp, "Admin_Email    %s~\n", sysdata->admin_email.c_str(  ) );
      fprintf( fp, "Newbie_purge   %d\n", sysdata->newbie_purge );
      fprintf( fp, "Regular_purge  %d\n", sysdata->regular_purge );
      fprintf( fp, "Autopurge      %d\n", sysdata->CLEANPFILES );
      fprintf( fp, "Testmode       %d\n", sysdata->TESTINGMODE );
      fprintf( fp, "Mapsize        %d\n", sysdata->mapsize );
      fprintf( fp, "Motd           %ld\n", sysdata->motd );
      fprintf( fp, "Imotd          %ld\n", sysdata->imotd );
      fprintf( fp, "Telnet         %s~\n", sysdata->telnet.c_str(  ) );
      fprintf( fp, "HTTP           %s~\n", sysdata->http.c_str(  ) );
      fprintf( fp, "Maxvnum        %d\n", sysdata->maxvnum );
      fprintf( fp, "Minguild       %d\n", sysdata->minguildlevel );
      fprintf( fp, "Maxcond        %d\n", sysdata->maxcondval );
      fprintf( fp, "Maxignore      %zu\n", sysdata->maxign );
      fprintf( fp, "Maximpact      %d\n", sysdata->maximpact );
      fprintf( fp, "Maxholiday     %zu\n", sysdata->maxholiday );
      fprintf( fp, "Initcond       %d\n", sysdata->initcond );
      fprintf( fp, "Secpertick     %d\n", sysdata->secpertick );
      fprintf( fp, "Pulsepersec    %d\n", sysdata->pulsepersec );
      fprintf( fp, "Hoursperday    %d\n", sysdata->hoursperday );
      fprintf( fp, "Daysperweek    %d\n", sysdata->daysperweek );
      fprintf( fp, "Dayspermonth   %d\n", sysdata->dayspermonth );
      fprintf( fp, "Monthsperyear  %d\n", sysdata->monthsperyear );
      fprintf( fp, "Minego         %d\n", sysdata->minego );
      fprintf( fp, "Rebootcount    %d\n", sysdata->rebootcount );
      fprintf( fp, "Auctionseconds %d\n", sysdata->auctionseconds );
      fprintf( fp, "Crashhandler   %d\n", sysdata->crashhandler );
      fprintf( fp, "Gameloopalarm  %d\n", sysdata->gameloopalarm );
      fprintf( fp, "Webwho         %d\n", sysdata->webwho );
      fprintf( fp, "%s", "End\n\n" );
      fprintf( fp, "%s", "#END\n" );
   }
   FCLOSE( fp );
}

void fread_sysdata( FILE * fp )
{
   int file_ver = 0;

   for( ;; )
   {
      const char *word = feof( fp ) ? "End" : fread_word( fp );

      switch ( UPPER( word[0] ) )
      {
         default:
            bug( "%s: no match: %s", __func__, word );
            fread_to_eol( fp );
            break;

         case '*':
            fread_to_eol( fp );
            break;

         case 'A':
            STDSKEY( "Admin_Email", sysdata->admin_email );
            KEY( "Auctionseconds", sysdata->auctionseconds, fread_number( fp ) );
            KEY( "Autopurge", sysdata->CLEANPFILES, fread_number( fp ) );
            break;

         case 'B':
            KEY( "Bashpvp", sysdata->bash_plr_vs_plr, fread_number( fp ) );
            KEY( "Bashnontank", sysdata->bash_nontank, fread_number( fp ) );
            KEY( "Bestowdif", sysdata->bestow_dif, fread_number( fp ) );
            KEY( "Build", sysdata->build_level, fread_number( fp ) );
            break;

         case 'C':
            KEY( "CheckImmHost", sysdata->check_imm_host, fread_number( fp ) );
            KEY( "Crashhandler", sysdata->crashhandler, fread_number( fp ) );
            break;

         case 'D':
            KEY( "Damplrvsplr", sysdata->dam_plr_vs_plr, fread_number( fp ) );
            KEY( "Damplrvsmob", sysdata->dam_plr_vs_mob, fread_number( fp ) );
            KEY( "Dammobvsplr", sysdata->dam_mob_vs_plr, fread_number( fp ) );
            KEY( "Dammobvsmob", sysdata->dam_mob_vs_mob, fread_number( fp ) );
            KEY( "Dodgemod", sysdata->dodge_mod, fread_number( fp ) );
            KEY( "Daysperweek", sysdata->daysperweek, fread_number( fp ) );
            KEY( "Dayspermonth", sysdata->dayspermonth, fread_number( fp ) );
            STDSKEY( "Dbserver", sysdata->dbserver );
            STDSKEY( "Dbname", sysdata->dbname );
            STDSKEY( "Dbuser", sysdata->dbuser );
            STDSKEY( "Dbpass", sysdata->dbpass );
            break;

         case 'E':
            if( !str_cmp( word, "End" ) )
            {
               if( sysdata->time_of_max.empty(  ) )
                  sysdata->time_of_max = "(not recorded)";
               if( sysdata->mud_name.empty(  ) )
                  sysdata->mud_name = "(Name Not Set)";
               if( sysdata->http.empty(  ) )
                  sysdata->http = "No page set";
               if( sysdata->telnet.empty(  ) )
                  sysdata->telnet = "Not set";
               return;
            }
            break;

         case 'F':
            KEY( "Forcepc", sysdata->level_forcepc, fread_number( fp ) );
            break;

         case 'G':
            KEY( "Gameloopalarm", sysdata->gameloopalarm, fread_number( fp ) );
            KEY( "Getnotake", sysdata->level_getobjnotake, fread_number( fp ) );
            KEY( "Gougepvp", sysdata->gouge_plr_vs_plr, fread_number( fp ) );
            KEY( "Gougenontank", sysdata->gouge_nontank, fread_number( fp ) );
            break;

         case 'H':
            KEY( "Highplayers", sysdata->alltimemax, fread_number( fp ) );
            STDSKEY( "Highplayertime", sysdata->time_of_max );
            STDSKEY( "HTTP", sysdata->http );
            KEY( "Hoursperday", sysdata->hoursperday, fread_number( fp ) );
            break;

         case 'I':
            KEY( "Imotd", sysdata->imotd, fread_number( fp ) );
            KEY( "Implock", sysdata->IMPLOCK, fread_number( fp ) );
            KEY( "Initcond", sysdata->initcond, fread_number( fp ) );
            break;

         case 'L':
            KEY( "Lockdown", sysdata->LOCKDOWN, fread_number( fp ) );
            break;

         case 'M':
            KEY( "Mapsize", sysdata->mapsize, fread_number( fp ) );
            KEY( "Motd", sysdata->motd, fread_number( fp ) );
            KEY( "Msetplayer", sysdata->level_mset_player, fread_number( fp ) );
            STDSKEY( "MudName", sysdata->mud_name );
            KEY( "Maxvnum", sysdata->maxvnum, fread_number( fp ) );
            KEY( "Minguild", sysdata->minguildlevel, fread_number( fp ) );
            KEY( "Maxcond", sysdata->maxcondval, fread_number( fp ) );
            KEY( "Maxignore", sysdata->maxign, fread_number( fp ) );
            KEY( "Maximpact", sysdata->maximpact, fread_number( fp ) );
            KEY( "Maxholiday", sysdata->maxholiday, fread_number( fp ) );
            KEY( "Monthsperyear", sysdata->monthsperyear, fread_number( fp ) );
            KEY( "Minego", sysdata->minego, fread_number( fp ) );
            break;

         case 'N':
            KEY( "Nameresolving", sysdata->NO_NAME_RESOLVING, fread_number( fp ) );
            KEY( "Newbie_purge", sysdata->newbie_purge, fread_number( fp ) );
            break;

         case 'O':
            KEY( "Overridepriv", sysdata->level_override_private, fread_number( fp ) );
            break;

         case 'P':
            KEY( "Parrymod", sysdata->parry_mod, fread_number( fp ) );
            STDSKEY( "Password", sysdata->password ); /* Samson 2-8-01 */
            KEY( "PetSave", sysdata->save_pets, fread_number( fp ) );
            KEY( "Protoflag", sysdata->level_modify_proto, fread_number( fp ) );
            KEY( "Pulsepersec", sysdata->pulsepersec, fread_number( fp ) );
            break;

         case 'R':
            KEY( "Readallmail", sysdata->read_all_mail, fread_number( fp ) );
            KEY( "Readmailfree", sysdata->read_mail_free, fread_number( fp ) );
            KEY( "Rebootcount", sysdata->rebootcount, fread_number( fp ) );
            KEY( "Regular_purge", sysdata->regular_purge, fread_number( fp ) );
            break;

         case 'S':
            KEY( "Stunplrvsplr", sysdata->stun_plr_vs_plr, fread_number( fp ) );
            KEY( "Stunregular", sysdata->stun_regular, fread_number( fp ) );
            if( !str_cmp( word, "Saveflags" ) )
            {
               if( file_ver < 1 )
                  sysdata->save_flags = fread_number( fp );
               else
                  flag_set( fp, sysdata->save_flags, save_flag );
               break;
            }
            KEY( "Savefreq", sysdata->save_frequency, fread_number( fp ) );
            KEY( "Secpertick", sysdata->secpertick, fread_number( fp ) );
            break;

         case 'T':
            KEY( "Takeothersmail", sysdata->take_others_mail, fread_number( fp ) );
            STDSKEY( "Telnet", sysdata->telnet );
            KEY( "Testmode", sysdata->TESTINGMODE, fread_number( fp ) );
            KEY( "Tumblemod", sysdata->tumble_mod, fread_number( fp ) );
            break;

         case 'V':
            KEY( "Version", file_ver, fread_number( fp ) );
            break;

         case 'W':
            KEY( "Waitforauth", sysdata->WAIT_FOR_AUTH, fread_number( fp ) );
            KEY( "Wizlock", sysdata->WIZLOCK, fread_number( fp ) );
            KEY( "Writemailfree", sysdata->write_mail_free, fread_number( fp ) );
            KEY( "Webwho", sysdata->webwho, fread_number( fp ) );
            break;
      }
   }
}

/*
 * Load the sysdata file
 */
bool load_systemdata( void )
{
   fs::path filename;
   FILE *fp;
   bool found;

   found = false;
   filename = std::format( "{}sysdata.dat", SYSTEM_DIR );

   if( ( fp = fopen( filename.c_str(), "r" ) ) != nullptr )
   {
      found = true;
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
            bug( "%s: # not found.", __func__ );
            break;
         }

         word = fread_word( fp );
         if( !str_cmp( word, "SYSTEM" ) )
         {
            fread_sysdata( fp );
            break;
         }
         else if( !str_cmp( word, "END" ) )
            break;
         else
         {
            bug( "%s: bad section: %s", __func__, word );
            break;
         }
      }
      FCLOSE( fp );
      update_timers(  );
      update_calendar(  );
   }
   return found;
}

/*
 * Translate all room exits from virtual to real.
 * Has to be done after all rooms are read in.
 * Check for bad reverse exits.
 */
void fix_exits( void )
{
   map < int, room_index * >::iterator iroom;
   room_index *pRoomIndex;

   for( iroom = room_index_table.begin(); iroom != room_index_table.end(); ++iroom )
   {
      pRoomIndex = iroom->second;

      list < exit_data * >::iterator iexit;
      for( iexit = pRoomIndex->exits.begin(  ); iexit != pRoomIndex->exits.end(  ); )
      {
         exit_data *pexit = *iexit;
         ++iexit;

         pexit->rvnum = pRoomIndex->vnum;
         if( pexit->vnum <= 0 || !( pexit->to_room = get_room_index( pexit->vnum ) ) )
         {
            if( fBootDb )
               boot_log( "%s: room %d, exit %s leads to bad vnum (%d)", __func__, pRoomIndex->vnum, dir_name[pexit->vdir], pexit->vnum );

            bug( "%s: Deleting %s exit in room %d", __func__, dir_name[pexit->vdir], pRoomIndex->vnum );
            pRoomIndex->extract_exit( pexit );
         }
      }
   }

   /*
    * Set all the rexit pointers - Thoric 
    */
   for( iroom = room_index_table.begin(); iroom != room_index_table.end(); ++iroom )
   {
      pRoomIndex = iroom->second;

      list < exit_data * >::iterator iexit;
      for( iexit = pRoomIndex->exits.begin(  ); iexit != pRoomIndex->exits.end(  ); ++iexit )
      {
         exit_data *pexit = *iexit;

         if( pexit->to_room && !pexit->rexit )
         {
            exit_data *rv_exit = pexit->to_room->get_exit_to( rev_dir[pexit->vdir], pRoomIndex->vnum );
            if( rv_exit )
            {
               pexit->rexit = rv_exit;
               rv_exit->rexit = pexit;
            }
         }
      }
   }
}

/*
 * Wizlist builder - Thoric
 */
void add_to_wizlist( const string & name, int level )
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
   for( const auto& entry : fs::directory_iterator( GOD_DIR ) )
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
   std::ofstream out( WIZLIST_FILE, std::ios::trunc );

   // Center the top banner with the MUD's name.
   out << std::format( "{:^78}\n", std::format( "The Immortal Masters of {}", sysdata->mud_name ) );

   int current_level = -1;
   std::string line_buffer;

   // Add each of the entries to the output file.
   for( const auto& wiz : wizlist )
   {
      if( wiz->level != current_level )
      {
         if( !line_buffer.empty() )
            out << std::format( "{:^78}\n ", line_buffer );

         out << std::format( "\n{:^78}\n ", get_title( wiz->level ) );
         line_buffer.clear();
         current_level = wiz->level;
      }

      if( line_buffer.length() + wiz->name.length() > 70 )
      {
         out << std::format( "{:^78}\n", line_buffer );
         line_buffer.clear();
      }
      line_buffer += ( line_buffer.empty() ? "" : " " ) + wiz->name;
   }

   if( !line_buffer.empty() )
      out << std::format( "{:^78}\n", line_buffer );

   // File stream will close automatically when function goes out of scope.
}

CMDF( do_makewizlist )
{
   make_wizlist(  );
   make_webwiz(  );
   build_wizinfo(  );
}

system_data::system_data(  )
{
   init_memory( &this->motd, &this->crashhandler, sizeof( this->crashhandler ) );
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

   fpArea = nullptr;
   fs::remove( BOOTLOG_FILE );
   boot_log( "%s", "---------------------[ Boot Log: Start ]--------------------" );
   log_string( "Database bootup starting." );
   fBootDb = true;   /* Supposed to help with EOF bugs, so it got moved up */

   log_string( "Loading sysdata configuration..." );

   sysdata = new system_data;
   /*
    * default values
    */
   sysdata->playersonline = 0;   // This one does not save
   sysdata->NO_NAME_RESOLVING = true;
   sysdata->WAIT_FOR_AUTH = true;
   sysdata->read_all_mail = LEVEL_DEMI;
   sysdata->read_mail_free = LEVEL_IMMORTAL;
   sysdata->write_mail_free = LEVEL_IMMORTAL;
   sysdata->take_others_mail = LEVEL_DEMI;
   sysdata->build_level = LEVEL_DEMI;
   sysdata->level_modify_proto = LEVEL_LESSER;
   sysdata->level_override_private = LEVEL_GREATER;
   sysdata->level_mset_player = LEVEL_LESSER;
   sysdata->stun_plr_vs_plr = 65;
   sysdata->stun_regular = 15;
   sysdata->dodge_mod = 2;
   sysdata->parry_mod = 2;
   sysdata->tumble_mod = 4;
   sysdata->dam_plr_vs_plr = 100;
   sysdata->dam_plr_vs_mob = 100;
   sysdata->dam_mob_vs_plr = 100;
   sysdata->dam_mob_vs_mob = 100;
   sysdata->level_getobjnotake = LEVEL_GREATER;
   sysdata->save_frequency = 20; /* minutes */
   sysdata->bestow_dif = 5;
   sysdata->check_imm_host = 1;
   sysdata->save_pets = 1;
   sysdata->save_flags.reset(  );
   sysdata->save_flags.set(  );  /* This defaults to turning on every save_flag */
   sysdata->motd = current_time;
   sysdata->imotd = current_time;
   sysdata->mapsize = 7;
   sysdata->maxvnum = 60000;
   sysdata->minguildlevel = 10;
   sysdata->maxcondval = 100;
   sysdata->maxign = 6;
   sysdata->maximpact = 30;
   sysdata->maxholiday = 30;
   sysdata->initcond = 12;
   sysdata->minego = 25;
   sysdata->secpertick = 70;
   sysdata->pulsepersec = 4;
   sysdata->hoursperday = 28;
   sysdata->daysperweek = 13;
   sysdata->dayspermonth = 26;
   sysdata->monthsperyear = 12;
   sysdata->rebootcount = 5;
   sysdata->auctionseconds = 15;
   sysdata->gameloopalarm = 30;
   sysdata->webwho = 0;
   sysdata->dbserver = "localhost";
   sysdata->dbname = "afkdb";
   sysdata->dbuser = "dbuser";
   sysdata->dbpass = "dbpass";

   if( !load_systemdata(  ) )
   {
      log_string( "Not found. Creating new configuration." );
      sysdata->alltimemax = 0;
      sysdata->mud_name = "(Name not set)";
      update_timers(  );
      update_calendar(  );
      save_sysdata(  );
   }

   log_string( "Initializing libdl support..." );
   /*
    * Open up a handle to the executable's symbol table for later use
    * when working with commands
    */
   sysdata->dlHandle = dlopen( nullptr, RTLD_NOW );
   if( !sysdata->dlHandle )
   {
      log_printf( "%s: Error opening local system executable as handle, please check compile flags.", __func__ );
      shutdown_mud( "libdl failure" );
      exit( 1 );
   }

#if !defined(__CYGWIN__) && defined(SQL)
   log_string( "Initializing MySQL support..." );
   init_mysql(  );
   add_event( 1800, ev_mysql_ping, nullptr );
#endif

   log_string( "Verifying existence of login greeting..." );
   fs::path lbuf = std::format( "{}greeting.dat", MOTD_DIR );
   if( !fs::exists( lbuf ) )
   {
      bug( "%s: Login greeting not found!", __func__ );
      shutdown_mud( "Missing login greeting" );
      exit( 1 );
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

   log_string( "Loading skill table..." );
   load_skill_table(  );
   sort_skill_table(  );
   remap_slot_numbers(  ); /* must be after the sort */
   num_sorted_skills = num_skills;

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

   log_string( "Loading herb table" );
   load_herb_table(  );

   log_string( "Loading tongues" );
   load_tongues(  );

   /*
    * abit/qbit code 
    */
   log_string( "Loading quest bit tables..." );
   load_bits(  );

   log_string( "Initalizing global lists" );
   nummobsloaded = 0;
   numobjsloaded = 0;
   physicalobjects = 0;
   objlist.clear(  );
   charlist.clear(  );
   shoplist.clear(  );
   repairlist.clear(  );
   teleportlist.clear(  );
   room_act_list.clear(  );
   obj_act_list.clear(  );
   mob_act_list.clear(  );
   cur_qobjs = 0;
   cur_qchars = 0;
   extracted_obj_queue = nullptr;
   extracted_char_queue = nullptr;
   quitting_char = nullptr;
   loading_char = nullptr;
   saving_char = nullptr;
   last_pkroom = 1;

   auction = new auction_data;
   auction->item = nullptr;

   for( wear = 0; wear < MAX_WEAR; ++wear )
      for( x = 0; x < MAX_LAYERS; ++x )
         save_equipment[wear][x] = nullptr;

   /*
    * Init random number generator.
    */
   log_string( "Initializing random number generator" );
   srand( current_time );

   /*
    * Set time and weather.
    */
   {
      long lhour, lday, lmonth;

      log_string( "Setting time and weather" );

      if( !load_timedata(  ) )   /* Loads time from stored file if true - Samson 1-21-99 */
      {
         boot_log( "%s", "Resetting mud time based on current system time." );
         lhour = ( current_time - 650336715 ) / ( sysdata->pulsetick / sysdata->pulsepersec );
         time_info.hour = lhour % sysdata->hoursperday;
         lday = lhour / sysdata->hoursperday;
         time_info.day = lday % sysdata->dayspermonth;
         lmonth = lday / sysdata->dayspermonth;
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

   /*
    * Read in all the area files.
    */
   {
      FILE *fpList;

      arealist.clear(  );
      area_nsort.clear(  );
      area_vsort.clear(  );
      log_string( "Reading in area files..." );
      if( !( fpList = fopen( AREA_LIST, "r" ) ) )
      {
         perror( AREA_LIST );
         shutdown_mud( "Boot_db: Unable to open area list" );
         exit( 1 );
      }

      // Why were these never initialized before?!?
      top_area = top_prog = top_room = top_exit = top_reset = top_mob_index = top_obj_index = 0;

      for( ;; )
      {
         if( feof( fpList ) )
         {
            bug( "%s: EOF encountered reading area list - no $ found at end of file.", __func__ );
            break;
         }
         strlcpy( strArea, fread_word( fpList ), MIL );
         if( strArea[0] == '$' )
            break;

         set_alarm( AREA_FILE_ALARM );
         alarm_section = "boot_db: read area files";
         load_area_file( strArea, false );
         set_alarm( 0 );
      }
      FCLOSE( fpList );
      log_string( "...done reading in area files." );
   }

   strlcpy( strArea, "NO FILE", MIL );

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
      log_printf( "Astral Walk target room for this boot is: %d", astral_target );
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

   strlcpy( strArea, "NO FILE", MIL );

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
      list < area_data * >::iterator iarea;

      /*
       * Putting some random fuzz on this to scatter the times around more 
       */
      for( iarea = arealist.begin(  ); iarea != arealist.end(  ); ++iarea )
      {
         area_data *area = *iarea;

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

   log_string( "Loading boards..." );
   load_boards(  );

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
   load_loginmsg( );

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
   time_t ptime = new_pfile_time_t - current_time;
   if( ptime < 1 )
      ptime = 1;
   add_event( ptime, ev_pfile_check, nullptr );

   add_event( 86400, ev_board_check, nullptr );

   add_event( 1, ev_dns_check, nullptr );

   log_string( "Database bootup completed." );
   boot_log( "%s", "---------------------[ Boot Log: End ]--------------------" );
}

/* Removal of this function constitutes a license violation */
CMDF( do_basereport )
{
   ch->printf( "&RCodebase revision: %s %s - %s\r\n", CODENAME, CODEVERSION, COPYRIGHT );
   ch->print( "&YContributors: Samson, Dwip, Whir, Cyberfox, Karangi, Rathian, Cam, Raine, and Tarl.\r\n" );
   ch->print( "&BDevelopment site: smaugmuds.afkmods.com\r\n" );
   ch->print( "&GThis function is included as a means to verify license compliance.\r\n" );
   ch->print( "Removal is a violation of your license.\r\n" );
   ch->print( "Copies of AFKMud beginning with 1.4 as of December 28, 2002 include this.\r\n" );
   ch->print( "Copies found to be running without this command will be subject to a violation report.\r\n" );
}

CMDF( do_memory )
{
   string arg;
   int hash;

   argument = one_argument( argument, arg );
   ch->print( "\r\n&wSystem Memory [arguments - hash, check, showhigh]\r\n" );
   ch->printf( "&wAffects: &W%5d\t\t\t&wAreas:   &W%5d\r\n", top_affect, top_area );
   ch->printf( "&wExtDes:  &W%5d\t\t\t&wExits:   &W%5d\r\n", top_ed, top_exit );
   ch->printf( "&wResets:  &W%5d\r\n", top_reset );
   ch->printf( "&wIdxMobs: &W%5d\t\t\t&wMobiles: &W%5d\r\n", top_mob_index, nummobsloaded );
   ch->printf( "&wIdxObjs: &W%5d\t\t\t&wObjs:    &W%5d(%d)\r\n", top_obj_index, numobjsloaded, physicalobjects );
   ch->printf( "&wRooms:   &W%5d\r\n", top_room );
   ch->printf( "&wShops:   &W%5d\t\t\t&wRepShps: &W%5d\r\n", top_shop, top_repair );
   ch->printf( "&wCurOq's: &W%5d\t\t\t&wCurCq's: &W%5d\r\n", cur_qobjs, cur_qchars );
   ch->printf( "&wPeople : &W%5d\t\t\t&wMaxplrs: &W%5d\r\n", num_descriptors, sysdata->maxplayers );
   ch->printf( "&wPlayers: &W%5d\r\n", sysdata->playersonline );
   ch->printf( "&wMaxEver: &W%5d\t\t\t&wTopsn:   &W%5d(%5d)\r\n", sysdata->alltimemax, num_skills, MAX_SKILL );
   ch->printf( "&wMaxEver was recorded on:  &W%s\r\n\r\n", sysdata->time_of_max.c_str(  ) );
#if !defined(__CYGWIN__) && defined(SQL)
   ch->printf( "&wMySQL Connection Active:  &W%s\r\n\r\n", ( db && db->ping() ) ? "YES" : ( db ? db->get_error().c_str() : "NO" ) );
#endif

   if( !str_cmp( arg, "check" ) )
   {
      ch->print( check_hash( argument.c_str(  ) ).c_str() );
      return;
   }
   if( !str_cmp( arg, "showhigh" ) )
   {
      show_high_hash( atoi( argument.c_str(  ) ) );
      return;
   }
   if( !argument.empty(  ) )
      hash = atoi( argument.c_str(  ) );
   else
      hash = -1;
   if( !str_cmp( arg, "hash" ) )
   {
      ch->printf( "Hash statistics:\r\n%s", hash_stats(  ).c_str() );
      if( hash != -1 )
         hash_dump( hash );
   }
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
 */
int number_range( int from, int to )
{
   if( ( to - from ) < 1 )
      return from;
   return ( ( rand(  ) % ( to - from + 1 ) ) + from );
}

/*
 * Generate a percentile roll.
 * number_mm() % 100 only does 0-99, changed to do 1-100 -Shaddai
 * Changed to use rand() directly, since the system random is just fine. - Samson
 */
int number_percent( void )
{
   return ( rand(  ) % 100 ) + 1;
}

/*
 * Generate a random door.
 */
int number_door( void )
{
   int door;

   while( ( door = rand(  ) & ( 16 - 1 ) ) > 9 )
      ;

   return door;
}

int number_bits( int width )
{
   return rand(  ) & ( ( 1 << width ) - 1 );
}

/*
 * Roll some dice.						-Thoric
 */
int dice( int number, int size )
{
   if( size == 0 )
      return 0;
   if( size == 1 )
      return number;

   int idice, sum;
   for( idice = 0, sum = 0; idice < number; ++idice )
      sum += number_range( 1, size );

   return sum;
}

CMDF( do_randtest )
{
   ch->printf( "Uterly random number    : %d\r\n", rand(  ) );
   ch->printf( "number_range 4350 - 4449: %d\r\n", number_range( 4350, 4449 ) );
   ch->printf( "number_percent          : %d\r\n", number_percent(  ) );
   ch->printf( "number_door             : %d\r\n", number_door(  ) );
   ch->printf( "number_bits 5           : %d\r\n", number_bits( 5 ) );
   ch->printf( "3d35 ( 3 35 sided dice ): %d\r\n", dice( 3, 35 ) );
}

/*
 * Dump a text file to a player, a line at a time		-Thoric
 */
void show_file( char_data * ch, const std::string & filename )
{
   std::ifstream fp{ filename };

   if( !fp.is_open() )
      return;

   ch->pager("\r\n");

   std::string line;
   while( std::getline( fp, line ) )
   {
      line += "\r\n";
      ch->pager( line );
   }
}

/*
 * Append a string to a file.
 */
// FIXME: Tagging this for upgrade to std::format. Many places call this. Follow example from character.cpp
void append_file( char_data * ch, const string & file, const char *fmt, ... )
{
   FILE *fp;
   va_list arg;
   char str[MSL];

   va_start( arg, fmt );
   vsnprintf( str, MSL, fmt, arg );
   va_end( arg );

   if( ch->isnpc(  ) || str[0] == '\0' )
      return;

   if( strlen( str ) < 1 || str[strlen( str ) - 1] != '\n' )
      strlcat( str, "\n", MSL );

   if( !( fp = fopen( file.c_str(  ), "a" ) ) )
   {
      perror( file.c_str(  ) );
      ch->print( "Could not open the file!\r\n" );
   }
   else
   {
      fprintf( fp, "[%5d] %s: %s\n", ch->in_room ? ch->in_room->vnum : 0, ch->name, str );
      FCLOSE( fp );
   }
}

/*
 * Append a string to a file.
 */
// FIXME: Tagging this for upgrade to std::format. Many places call this. Follow example from character.cpp
void append_to_file( const string & file, const char *fmt, ... )
{
   FILE *fp;
   va_list arg;
   char str[MSL];

   va_start( arg, fmt );
   vsnprintf( str, MSL, fmt, arg );
   va_end( arg );

   if( str[0] == '\0' )
      return;

   if( strlen( str ) < 1 || str[strlen( str ) - 1] != '\n' )
      strlcat( str, "\n", MSL );

   if( !( fp = fopen( file.c_str(  ), "a" ) ) )
      perror( file.c_str(  ) );
   else
   {
      fprintf( fp, "%s\n", str );
      FCLOSE( fp );
   }
}

/*
 * Believe it or not, this little gem originated as a code example in a Google search.
 * I've modified what Google AI provided to cut the filename down to the actual source code file.
 * Output of the results gets sent to the standard game log. - Samson 1/11/2025
 *
 * Note: This requires compiling with C++23 support. Currently C++23 is available with GCC 13 and later.
 */
#if !defined(__CYGWIN__) && !defined(__FreeBSD__) && !defined(WIN32)
std::string demangle( const std::string& name )
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
   ostringstream lines;

   lines << endl << "Obtained " << trace.size() << " stack frames:" << endl << endl;

   for( const auto& frame : trace )
   {
      string::size_type pos = frame.source_file().find_last_of( "/", frame.source_file().length() );
      string file_name;

      if( pos != string::npos )
         file_name = frame.source_file().substr( pos + 1 );
      else
         file_name = frame.source_file();

      std::string func_name = demangle( frame.description() );
      lines << frame.description() << " -> " << file_name << ":" << frame.source_line() << endl;
   }
   log_string( lines.str( ) );
}
#endif

/* Reports a bug.
 *
 * In the function call for this, you can specify the following:
 *
 * __func__ The function name where "bug" was called from.    Type = *char, uses %s formatting.
 * __FILE__ The source code file where "bug" was called from. Type = *char, uses %s formatting.
 * __LINE__ The line of the file where "bug" was called from. Type = int,   uses %d formatting.
 *
 * Example:
 *
 * bug( "%s -> %s:%d: Backtrace test.", __func__, __FILE__, __LINE__ );
 *
 * Due to the formatting attributes, at least __func__ is required.
 * It's helpful to include the file and line tags since for some odd reason the backtrace code won't include the function that called "bug".
 */
// FIXME: Tagging this for upgrade to std::format. Many places call this. Follow example from character.cpp
void bug( const char *str, ... )
{
   char buf[MSL];

   va_list param;

   va_start( param, str );
   vsnprintf( buf, MSL, str, param );
   va_end( param );

   log_printf_plus( LOG_DEBUG, LEVEL_IMMORTAL, "[*****] BUG: %s", buf );

   if( fpArea != nullptr )
   {
      int iLine;

      if( fpArea == stdin )
         iLine = 0;
      else
      {
         int iChar = ftell( fpArea );
         fseek( fpArea, 0, 0 );
         for( iLine = 0; ftell( fpArea ) < iChar; ++iLine )
         {
            while( getc( fpArea ) != '\n' )
               ;
         }
         fseek( fpArea, iChar, 0 );
      }
      log_printf( "[*****] FILE: %s LINE: %d", strArea, iLine );
   }

#if !defined(__CYGWIN__) && !defined(__FreeBSD__) && !defined(WIN32)
   if( !fBootDb )
   {
      generate_backtrace();
   }
#endif
}

/*
 * Writes a string to the log, extended version - Thoric
 */
void log_string_plus( short log_type, short level, const string & str )
{
   auto now = std::chrono::system_clock::now();
   auto seconds_only = std::chrono::floor<std::chrono::seconds>(now);
   auto local_time = std::chrono::zoned_time{ std::chrono::current_zone(), seconds_only };

   std::string timestamp = std::format( "{0:%F %T}", local_time );
   std::cerr << std::format( "{} :: {}\n", timestamp, str );

   std::string newstr = str;
   if( newstr.starts_with( "Log " ) )
   {
      newstr = newstr.substr( 4 );
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

// FIXME: Tagging this for upgrade to std::format. Many places call this. Follow example from character.cpp
void log_printf_plus( short log_type, short level, const char *fmt, ... )
{
   char buf[MSL * 2];
   va_list args;

   va_start( args, fmt );
   vsnprintf( buf, MSL * 2, fmt, args );
   va_end( args );

   log_string_plus( log_type, level, buf );
}

// FIXME: Tagging this for upgrade to std::format. Many places call this. Follow example from character.cpp
void log_printf( const char *fmt, ... )
{
   char buf[MSL * 2];
   va_list args;

   va_start( args, fmt );
   vsnprintf( buf, MSL * 2, fmt, args );
   va_end( args );

   log_string_plus( LOG_NORMAL, LEVEL_LOG, buf );
}

/*  Dump command...This command creates a text file with the stats of every  *
 *  mob, or object in the mud, depending on the argument given.              *
 *  Obviously, this will tend to create HUGE files, so it is recommended     *
 *  that it be only given to VERY high level imms, and preferably those      *
 *  with shell access, so that they may clean it out, when they are done     *
 *  with it.
 */
CMDF( do_dump )
{
   if( ch->isnpc(  ) )
      return;

   if( !ch->is_imp(  ) )
   {
      ch->print( "Sorry, only an Implementor may use this command!\r\n" );
      return;
   }

   if( !str_cmp( argument, "mobs" ) )
   {
      FILE *fp = fopen( "../mobdata.txt", "w" );
      ch->print( "Writing to file...\r\n" );

      for( int counter = 0; counter < sysdata->maxvnum; ++counter )
      {
         mob_index *mob;

         if( ( mob = get_mob_index( counter ) ) != nullptr )
         {
            fprintf( fp, "VNUM:  %d\n", mob->vnum );
            fprintf( fp, "S_DESC:  %s\n", mob->short_descr );
            fprintf( fp, "LEVEL:  %d\n", mob->level );
            fprintf( fp, "HITROLL:  %d\n", mob->hitroll );
            fprintf( fp, "DAMROLL:  %d\n", mob->damroll );
            fprintf( fp, "HITDIE:  %dd%d+%d\n", mob->hitnodice, mob->hitsizedice, mob->hitplus );
            fprintf( fp, "DAMDIE:  %dd%d+%d\n", mob->damnodice, mob->damsizedice, mob->damplus );
            fprintf( fp, "ACT FLAGS:  %s\n", bitset_string( mob->actflags, act_flags ) );
            fprintf( fp, "AFFECTED_BY:  %s\n", bitset_string( mob->affected_by, aff_flags ) );
            fprintf( fp, "RESISTS:  %s\n", bitset_string( mob->resistant, ris_flags ) );
            fprintf( fp, "SUSCEPTS:  %s\n", bitset_string( mob->susceptible, ris_flags ) );
            fprintf( fp, "IMMUNE:  %s\n", bitset_string( mob->immune, ris_flags ) );
            fprintf( fp, "ABSORB:  %s\n", bitset_string( mob->absorb, ris_flags ) );
            fprintf( fp, "ATTACKS:  %s\n", bitset_string( mob->attacks, attack_flags ) );
            fprintf( fp, "DEFENSES:  %s\n\n\n", bitset_string( mob->defenses, defense_flags ) );
         }
      }
      FCLOSE( fp );
      ch->print( "Done.\r\n" );
      return;
   }

   if( !str_cmp( argument, "rooms" ) )
   {
      FILE *fp = fopen( "../roomdata.txt", "w" );
      ch->print( "Writing to file...\r\n" );

      for( int counter = 0; counter < sysdata->maxvnum; ++counter )
      {
         room_index *room;

         if( ( room = get_room_index( counter ) ) != nullptr )
         {
            fprintf( fp, "VNUM:  %d\n", room->vnum );
            fprintf( fp, "NAME:  %s\n", room->name );
            fprintf( fp, "SECTOR:  %s\n", sect_types[room->sector_type] );
            fprintf( fp, "FLAGS:  %s\n\n\n", bitset_string( room->flags, r_flags ) );
         }
      }
      FCLOSE( fp );
      ch->print( "Done.\r\n" );
      return;
   }

   if( !str_cmp( argument, "objects" ) )
   {
      FILE *fp = fopen( "../objdata.txt", "w" );
      ch->print( "Writing objects to file...\r\n" );

      for( int counter = 0; counter < sysdata->maxvnum; ++counter )
      {
         obj_index *obj;

         if( ( obj = get_obj_index( counter ) ) != nullptr )
         {
            fprintf( fp, "VNUM: %d\n", obj->vnum );
            fprintf( fp, "KEYWORDS: %s\n", obj->name );
            fprintf( fp, "TYPE: %s\n", o_types[obj->item_type] );
            fprintf( fp, "SHORT DESC: %s\n", obj->short_descr );
            fprintf( fp, "FLAGS: %s\n", bitset_string( obj->extra_flags, o_flags ) );
            fprintf( fp, "WEARFLAGS: %s\n", bitset_string( obj->wear_flags, w_flags ) );
            fprintf( fp, "WEIGHT: %d\n", obj->weight );
            fprintf( fp, "COST: %d\n", obj->cost );
            fprintf( fp, "EGO: %d\n", obj->ego );
            fprintf( fp, "ZONE: %s\n", obj->area->name );
            fprintf( fp, "%s", "AFFECTS:\n" );

            list < affect_data * >::iterator paf;
            for( paf = obj->affects.begin(  ); paf != obj->affects.end(  ); ++paf )
            {
               affect_data *af = ( *paf );

               if( af->location == APPLY_AFFECT )
                  fprintf( fp, "%s by %s\n", a_types[af->location], aff_flags[af->modifier] );
               else if( af->location == APPLY_WEAPONSPELL
                        || af->location == APPLY_WEARSPELL
                        || af->location == APPLY_REMOVESPELL || af->location == APPLY_STRIPSN || af->location == APPLY_RECURRINGSPELL || af->location == APPLY_EAT_SPELL )
                  fprintf( fp, "%s '%s'\n", a_types[af->location], IS_VALID_SN( af->modifier ) ? skill_table[af->modifier]->name : "UNKNOWN" );
               else if( af->location == APPLY_RESISTANT || af->location == APPLY_IMMUNE || af->location == APPLY_SUSCEPTIBLE || af->location == APPLY_ABSORB )
                  fprintf( fp, "%s %s\n", a_types[af->location], bitset_string( af->rismod, ris_flags ) );
               else
                  fprintf( fp, "%s %d\n", a_types[af->location], af->modifier );
            }

            if( obj->layers > 0 )
               fprintf( fp, "Layerable - Wear layer: %d\n", obj->layers );

            for( int x = 0; x < MAX_OBJ_VALUE; ++x )
               fprintf( fp, "VAL%d: %d\n", x, obj->value[x] );
         }
      }
      FCLOSE( fp );
      ch->print( "Done.\r\n" );
      return;
   }
   ch->print( "Syntax: dump <mobs/objects>\r\n" );
}
