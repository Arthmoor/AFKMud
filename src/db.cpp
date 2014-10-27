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
 *                       Database management module                         *
 ****************************************************************************/

#include <dirent.h>
#include <sys/stat.h>
#include <sys/time.h>
#if !defined(WIN32)
 #include <dlfcn.h>   /* Required for libdl - Trax */
#else
 #include <windows.h>
 #define dlopen( libname, flags ) LoadLibrary( (libname) )
#endif
#if !defined(__CYGWIN__) && !defined(__FreeBSD__) && !defined(WIN32)
 #include <execinfo.h>
#endif
#include <cstdarg>
#include <cmath>
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
#if !defined(__CYGWIN__) && defined(SQL)
 #include "sql.h"
#endif

#if defined(WIN32)
void gettimeofday( struct timeval *, struct timezone * );
#endif

/* Change this alarm timer to whatever you think is appropriate.
 * 45 seconds allows plenty of time for Valgrind to run on an AMD 2800+ CPU.
 * Adjust if you are getting infinite loop warnings that are shutting down bootup.
 */
const int AREA_FILE_ALARM = 45;

system_data *sysdata;

/*
 * Structure used to build wizlist
 */
struct wizent
{
   wizent();
   ~wizent();

   char *name;
   char *http;
   short level;
};

list<wizent*> wizlist;

wizent::wizent()
{
   name = NULL;
   http = NULL;
   level = 0;
}

wizent::~wizent()
{
   DISPOSE( name );
   DISPOSE( http );
   wizlist.remove( this );
}

void init_supermob( void );

/*
 * Globals.
 */
time_info_data time_info;
extern char *alarm_section;
extern obj_data *extracted_obj_queue;
extern struct extracted_char_data *extracted_char_queue;

int weath_unit;   /* global weather param */
int rand_factor;
int climate_factor;
int neigh_factor;
int max_vector;
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
void load_shellcommands( );
#endif
void load_modules();
void web_arealist();
bool load_timedata( );
void load_shopkeepers( );
void load_auth_list( );  /* New Auth Code */
void save_auth_list( );
void build_wizinfo( );
void load_maps( ); /* Load in Overland Maps - Samson 8-1-99 */
void load_ships( );   /* Load ships - Samson 1-6-01 */
void load_world( );
void load_morphs( );
void load_skill_table( );
void remap_slot_numbers( );
void load_mxpobj_cmds( );
void load_quotes( );
void load_sales( );   /* Samson 6-24-99 for new auction system */
void load_aucvaults( );  /* Samson 6-20-99 for new auction system */
void load_corpses( );
void load_banlist( );
void update_timers( );
void update_calendar( );
void load_specfuns( );
void load_equipment_totals( bool );
void load_slays( );
void load_holidays( );
void load_bits( );
void load_liquids( );
void load_mixtures( );
void load_imm_host( );
void load_dns( );
void load_mudchannels( );
void to_channel( const char *, char *, int );
void load_runes( );
void load_clans( );
void load_socials( );
void load_commands( );
void load_deity( );
void load_boards( );
void load_projects( );
void load_watchlist( );
void assign_gsn_data( );
int mob_xp( char_data * );
void load_connhistory( );
void init_area_weather( );
void load_weatherdata( );
void sort_skill_table( );
void load_classes( );
void load_races( );
void load_herb_table( );
void load_tongues( );
void load_helps();

affect_data::affect_data()
{
   init_memory( &rismod, &type, sizeof( type ) );
}

void shutdown_mud( char *reason )
{
   FILE *fp;

   if( ( fp = fopen( SHUTDOWN_FILE, "a" ) ) != NULL )
   {
      fprintf( fp, "%s\n", reason );
      FCLOSE( fp );
   }
}

bool exists_file( string name )
{
   struct stat fst;

   /*
    * Stands to reason that if there ain't a name to look at, it damn well don't exist! 
    */
   if( name.empty() )
      return false;

   if( stat( name.c_str(), &fst ) != -1 )
      return true;
   else
      return false;
}

bool exists_file( const char *name )
{
   struct stat fst;

   /*
    * Stands to reason that if there ain't a name to look at, it damn well don't exist! 
    */
   if( !name || name[0] == '\0' || !str_cmp( name, "" ) )
      return false;

   if( stat( name, &fst ) != -1 )
      return true;
   else
      return false;
}

bool is_valid_filename( char_data *ch, const char *direct, const char *filename )
{
   char newfilename[256];
   struct stat fst;

   /* Length restrictions */
   if( !filename || filename[0] == '\0' || strlen( filename ) < 3 )
   {
      if( !filename || !str_cmp( filename, "" ) )
         ch->print( "Empty filename is not valid.\r\n" );
      else
         ch->printf( "%s: Filename is too short.\r\n", filename );
      return false;
   }

   /* Illegal characters */
   if( strstr( filename, ".." ) || strstr( filename, "/" ) || strstr( filename, "\\" ) )
   {
      ch->print( "A filename may not contain a '..', '/', or '\\' in it.\r\n" );
      return false;
   }

   /* If that filename is already being used lets not allow it now to be on the safe side */
   snprintf( newfilename, sizeof( newfilename ), "%s%s", direct, filename );
   if( stat( newfilename, &fst ) != -1 )
   {
      ch->printf( "%s is already an existing filename.\r\n", newfilename );
      return false;
   }

   /* If we got here assume its valid */
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
         bug( "%s: EOF encountered on read.", __FUNCTION__ );
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
float fread_float( FILE *fp )
{
   float number;
   bool sign, decimal;
   char c;
   double place = 0;

   do
   {
      if( feof( fp ) )
      {
         bug( "%s: EOF encountered on read.", __FUNCTION__ );
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
      bug( "%s: bad format. (%c)", __FUNCTION__, c );
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
            bug( "%s: EOF encountered on read.", __FUNCTION__ );
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
         bug( "%s: EOF encountered on read.", __FUNCTION__ );
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
      bug( "%s: bad format. (%c)", __FUNCTION__, c );
      if( fBootDb )
         exit( 1 );
      return 0;
   }

   while( isdigit( c ) )
   {
      if( feof( fp ) )
      {
         bug( "%s: EOF encountered on read.", __FUNCTION__ );
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
         bug( "%s: EOF encountered on read.", __FUNCTION__ );
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
      bug( "%s: bad format. (%c)", __FUNCTION__, c );
      if( fBootDb )
         exit( 1 );
      return 0;
   }

   while( isdigit( c ) )
   {
      if( feof( fp ) )
      {
         bug( "%s: EOF encountered on read.", __FUNCTION__ );
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
         bug( "%s: EOF encountered on read.", __FUNCTION__ );
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
      bug( "%s: bad format. (%c)", __FUNCTION__, c );
      if( fBootDb )
         exit( 1 );
      return 0;
   }

   while( isdigit( c ) )
   {
      if( feof( fp ) )
      {
         bug( "%s: EOF encountered on read.", __FUNCTION__ );
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
char *fread_flagstring( FILE * fp )
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
         bug( "%s: EOF encountered on read.", __FUNCTION__ );
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
         bug( "%s: string too long", __FUNCTION__ );
         *plast = '\0';
         return ( buf );
      }
      switch ( *plast = getc( fp ) )
      {
         default:
            ++plast;
            ++ln;
            break;

         case EOF:
            bug( "%s: EOF", __FUNCTION__ );
            if( fBootDb )
               exit( 1 );
            *plast = '\0';
            return ( buf );

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
            return ( buf );
      }
   }
}

/* Read a string from file and allocate it to the shared string hash */
char *fread_string( FILE * fp )
{
   char buf[MSL];

   mudstrlcpy( buf, fread_flagstring( fp ), MSL );

   if( !str_cmp( buf, "" ) )
      return NULL;
   return STRALLOC( buf );
}

/* Read a string from a file and assign it to a std::string */
void fread_string( string &newstring, FILE *fp )
{
   char buf[MSL];

   mudstrlcpy( buf, fread_flagstring( fp ), MSL );
   newstring = buf;
}

/* Read a string from file fp using str_dup (ie: no string hashing)
 * This will probably become obsolete after the std::string conversions are done.
 */
char *fread_string_nohash( FILE * fp )
{
   char buf[MSL];

   mudstrlcpy( buf, fread_flagstring( fp ), MSL );
   return str_dup( buf );
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
         bug( "%s: EOF encountered on read.", __FUNCTION__ );
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
   return;
}

/*
 * Read to end of line into static buffer - Thoric
 */
char *fread_line( FILE * fp )
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
         bug( "%s: EOF encountered on read.", __FUNCTION__ );
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
         bug( "%s: EOF encountered on read.", __FUNCTION__ );
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
         bug( "%s: line too long", __FUNCTION__ );
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

void fread_line( string &newstring, FILE *fp )
{
   char buf[MSL];

   mudstrlcpy( buf, fread_line( fp ), MSL );
   if( buf[strlen(buf)-1] == '\n' || buf[strlen(buf)-1] == '\r' )
      buf[strlen(buf)-1] = '\0';
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
         bug( "%s: EOF encountered on read.", __FUNCTION__ );
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
         bug( "%s: EOF encountered on read.", __FUNCTION__ );
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
   bug( "%s: word too long", __FUNCTION__ );
   *pword = '\0';
   return word;
}

/*
 * Add a string to the boot-up log - Thoric
 */
void boot_log( const char *str, ... )
{
   char buf[MSL];
   FILE *fp;
   va_list param;

   mudstrlcpy( buf, "[*****] BOOT: ", MSL );
   va_start( param, str );
   vsnprintf( buf + strlen( buf ), MSL, str, param );
   va_end( param );
   log_string( buf );

   if( ( fp = fopen( BOOTLOG_FILE, "a" ) ) != NULL )
   {
      fprintf( fp, "%s\n", buf );
      FCLOSE( fp );
   }
   return;
}

/* Build list of in_progress areas. Do not load areas.
 * define AREA_READ if you want it to build area names rather than reading
 * them out of the area files. -- Altrag */
/* The above info is obsolete - this will now simply load whatever is in the
 * BUILD_DIR and assume it to be a valid prototype zone. -- Samson 2-13-04
 */
void load_buildlist( void )
{
   DIR *dp;
   struct dirent *dentry;
   char buf[256];

   fBootDb = true;

   dp = opendir( BUILD_DIR );
   dentry = readdir( dp );
   while( dentry )
   {
      if( dentry->d_name[0] != '.' )
      {
         /*
          * Added by Tarl 3 Dec 02 because we are now using CVS 
          */
         if( str_cmp( dentry->d_name, "CVS" ) && !str_infix( ".are", dentry->d_name ) )
         {
            if( str_infix( ".bak", dentry->d_name ) )
            {
               snprintf( buf, 256, "%s%s", BUILD_DIR, dentry->d_name );
               mudstrlcpy( strArea, dentry->d_name, MIL );
               set_alarm( AREA_FILE_ALARM );
               alarm_section = "load_buildlist: read prototype area files";
               load_area_file( buf, true );
            }
         }
      }
      dentry = readdir( dp );
   }
   closedir( dp );
   fBootDb = false;
}

const int SYSFILEVER = 1;
/*
 * Save system info to data file
 */
void save_sysdata( void )
{
   FILE *fp;
   char filename[256];

   snprintf( filename, 256, "%ssysdata.dat", SYSTEM_DIR );

   if( !( fp = fopen( filename, "w" ) ) )
   {
      bug( "%s: fopen", __FUNCTION__ );
      perror( filename );
   }
   else
   {
      fprintf( fp, "%s", "#SYSTEM\n" );
      fprintf( fp, "Version        %d\n", SYSFILEVER );
      fprintf( fp, "MudName        %s~\n", sysdata->mud_name );
      fprintf( fp, "Password       %s~\n", sysdata->password );
      fprintf( fp, "Dbserver       %s~\n", sysdata->dbserver );
      fprintf( fp, "Dbname         %s~\n", sysdata->dbname );
      fprintf( fp, "Dbuser         %s~\n", sysdata->dbuser );
      fprintf( fp, "Dbpass         %s~\n", sysdata->dbpass );
      fprintf( fp, "Highplayers    %d\n", sysdata->alltimemax );
      fprintf( fp, "Highplayertime %s~\n", sysdata->time_of_max );
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
      fprintf( fp, "Admin_Email    %s~\n", sysdata->admin_email );
      fprintf( fp, "Newbie_purge   %d\n", sysdata->newbie_purge );
      fprintf( fp, "Regular_purge  %d\n", sysdata->regular_purge );
      fprintf( fp, "Autopurge      %d\n", sysdata->CLEANPFILES );
      fprintf( fp, "Testmode       %d\n", sysdata->TESTINGMODE );
      fprintf( fp, "Mapsize        %d\n", sysdata->mapsize );
      fprintf( fp, "Motd           %ld\n", ( time_t )sysdata->motd );
      fprintf( fp, "Imotd          %ld\n", ( time_t )sysdata->imotd );
      fprintf( fp, "Telnet         %s~\n", sysdata->telnet );
      fprintf( fp, "HTTP           %s~\n", sysdata->http );
      fprintf( fp, "Maxvnum        %d\n", sysdata->maxvnum );
      fprintf( fp, "Minguild       %d\n", sysdata->minguildlevel );
      fprintf( fp, "Maxcond        %d\n", sysdata->maxcondval );
      fprintf( fp, "Maxignore      %d\n", sysdata->maxign );
      fprintf( fp, "Maximpact      %d\n", sysdata->maximpact );
      fprintf( fp, "Maxholiday     %d\n", sysdata->maxholiday );
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
   return;
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
            bug( "%s: no match: %s", __FUNCTION__, word );
            fread_to_eol( fp );
            break;

         case '*':
            fread_to_eol( fp );
            break;

         case 'A':
            KEY( "Admin_Email", sysdata->admin_email, fread_string_nohash( fp ) );
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
            if( !str_cmp( word, "Dbserver" ) )
            {
               DISPOSE( sysdata->dbserver );
               sysdata->dbserver = fread_string_nohash( fp );
               break;
            }
            if( !str_cmp( word, "Dbname" ) )
            {
               DISPOSE( sysdata->dbname );
               sysdata->dbname = fread_string_nohash( fp );
               break;
            }
            if( !str_cmp( word, "Dbuser" ) )
            {
               DISPOSE( sysdata->dbuser );
               sysdata->dbuser = fread_string_nohash( fp );
               break;
            }
            if( !str_cmp( word, "Dbpass" ) )
            {
               DISPOSE( sysdata->dbpass );
               sysdata->dbpass = fread_string_nohash( fp );
               break;
            }
            break;

         case 'E':
            if( !str_cmp( word, "End" ) )
            {
               if( !sysdata->time_of_max )
                  sysdata->time_of_max = str_dup( "(not recorded)" );
               if( !sysdata->mud_name )
                  sysdata->mud_name = str_dup( "(Name Not Set)" );
               if( !sysdata->http )
                  sysdata->http = str_dup( "No page set" );
               if( !sysdata->telnet )
                  sysdata->telnet = str_dup( "Not set" );
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
            KEY( "Highplayertime", sysdata->time_of_max, fread_string_nohash( fp ) );
            KEY( "HTTP", sysdata->http, fread_string_nohash( fp ) );
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
            KEY( "MudName", sysdata->mud_name, fread_string_nohash( fp ) );
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
            KEY( "Password", sysdata->password, fread_string_nohash( fp ) );  /* Samson 2-8-01 */
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
            KEY( "Telnet", sysdata->telnet, fread_string_nohash( fp ) );
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
   char filename[256];
   FILE *fp;
   bool found;

   found = false;
   snprintf( filename, 256, "%ssysdata.dat", SYSTEM_DIR );

   if( ( fp = fopen( filename, "r" ) ) != NULL )
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
            bug( "%s: # not found.", __FUNCTION__ );
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
            bug( "%s: bad section: %s", __FUNCTION__, word );
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
   room_index *pRoomIndex;

   for( int iHash = 0; iHash < MAX_KEY_HASH; ++iHash )
   {
      for( pRoomIndex = room_index_hash[iHash]; pRoomIndex; pRoomIndex = pRoomIndex->next )
      {
         list<exit_data*>::iterator iexit;
         for( iexit = pRoomIndex->exits.begin(); iexit != pRoomIndex->exits.end(); )
         {
            exit_data *pexit = (*iexit);
            ++iexit;

            pexit->rvnum = pRoomIndex->vnum;
            if( pexit->vnum <= 0 || !( pexit->to_room = get_room_index( pexit->vnum ) ) )
            {
               if( fBootDb )
                  boot_log( "Fix_exits: room %d, exit %s leads to bad vnum (%d)",
                            pRoomIndex->vnum, dir_name[pexit->vdir], pexit->vnum );

               bug( "%s: Deleting %s exit in room %d", __FUNCTION__, dir_name[pexit->vdir], pRoomIndex->vnum );
               pRoomIndex->extract_exit( pexit );
            }
         }
      }
   }

   /*
    * Set all the rexit pointers - Thoric 
    */
   for( int iHash = 0; iHash < MAX_KEY_HASH; ++iHash )
   {
      for( pRoomIndex = room_index_hash[iHash]; pRoomIndex; pRoomIndex = pRoomIndex->next )
      {
         list<exit_data*>::iterator iexit;
         for( iexit = pRoomIndex->exits.begin(); iexit != pRoomIndex->exits.end(); ++iexit )
         {
            exit_data *pexit = (*iexit);

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
   return;
}

/*
 * wizlist builder! - Thoric
 */
void towizfile( const char *line )
{
   int filler, xx;
   char outline[MSL];
   FILE *wfp;

   outline[0] = '\0';

   if( line && line[0] != '\0' )
   {
      filler = ( 78 - strlen( line ) );
      if( filler < 1 )
         filler = 1;
      filler /= 2;
      for( xx = 0; xx < filler; ++xx )
         mudstrlcat( outline, " ", MSL );
      mudstrlcat( outline, line, MSL );
   }
   mudstrlcat( outline, "\r\n", MSL );
   wfp = fopen( WIZLIST_FILE, "a" );
   if( wfp )
   {
      fputs( outline, wfp );
      FCLOSE( wfp );
   }
}

void towebwiz( const char *line )
{
   char outline[MSL];
   FILE *wfp;

   outline[0] = '\0';

   mudstrlcat( outline, " ", MSL );
   mudstrlcat( outline, line, MSL );
   mudstrlcat( outline, "\r\n", MSL );
   wfp = fopen( WEBWIZ_FILE, "a" );
   if( wfp )
   {
      fputs( outline, wfp );
      FCLOSE( wfp );
   }
}

void add_to_wizlist( char *name, char *http, int level )
{
   wizent *wiz = new wizent;
   wiz->name = str_dup( name );
   if( http != NULL )
      wiz->http = str_dup( http );
   wiz->level = level;

   if( wizlist.empty() )
   {
      wizlist.push_back( wiz );
      return;
   }

   /*
    * insert sort, of sorts 
    */
   list<wizent*>::iterator tmp;
   for( tmp = wizlist.begin(); tmp != wizlist.end(); ++tmp )
   {
      wizent *wt = (*tmp);

      if( level > wt->level )
      {
         wizlist.insert( tmp, wiz );
         return;
      }
   }
   wizlist.push_back( wiz );
   return;
}

/*
 * Wizlist builder - Thoric
 */
void make_wizlist(  )
{
   DIR *dp;
   struct dirent *dentry;
   FILE *gfp;
   const char *word;
   char buf[256];

   wizlist.clear();

   dp = opendir( GOD_DIR );

   int ilevel = 0;
   dentry = readdir( dp );
   while( dentry )
   {
      if( dentry->d_name[0] != '.' )
      {
         /*
          * Added by Tarl 3 Dec 02 because we are now using CVS 
          */
         if( str_cmp( dentry->d_name, "CVS" ) )
         {
            snprintf( buf, 256, "%s%s", GOD_DIR, dentry->d_name );
            gfp = fopen( buf, "r" );
            if( gfp )
            {
               bitset<MAX_PCFLAG> iflags;
               iflags.reset(  );

               word = feof( gfp ) ? "End" : fread_word( gfp );
               ilevel = fread_number( gfp );
               fread_to_eol( gfp );
               word = feof( gfp ) ? "End" : fread_word( gfp );
               if( !str_cmp( word, "Pcflags" ) )
                  flag_set( gfp, iflags, pc_flags );
               FCLOSE( gfp );
               if( iflags.test( PCFLAG_RETIRED ) )
                  ilevel = MAX_LEVEL - 15;
               if( iflags.test( PCFLAG_GUEST ) )
                  ilevel = MAX_LEVEL - 16;
               add_to_wizlist( dentry->d_name, NULL, ilevel );
            }
         }
      }
      dentry = readdir( dp );
   }
   closedir( dp );

   unlink( WIZLIST_FILE );
   snprintf( buf, 256, "The Immortal Masters of %s", sysdata->mud_name );
   towizfile( buf );

   buf[0] = '\0';
   ilevel = 65535;
   list<wizent*>::iterator went;
   for( went = wizlist.begin(); went != wizlist.end(); ++went )
   {
      wizent *wiz = (*went);

      if( wiz->level < ilevel )
      {
         if( buf[0] )
         {
            towizfile( buf );
            buf[0] = '\0';
         }
         towizfile( "" );
         ilevel = wiz->level;
         switch ( ilevel )
         {
            case MAX_LEVEL - 0:
               towizfile( " Supreme Entity" );
               break;
            case MAX_LEVEL - 1:
               towizfile( " Realm Lords" );
               break;
            case MAX_LEVEL - 2:
               towizfile( " Eternals" );
               break;
            case MAX_LEVEL - 3:
               towizfile( " Ancients" );
               break;
            case MAX_LEVEL - 4:
               towizfile( " Astral Gods" );
               break;
            case MAX_LEVEL - 5:
               towizfile( " Elemental Gods" );
               break;
            case MAX_LEVEL - 6:
               towizfile( " Dream Gods" );
               break;
            case MAX_LEVEL - 7:
               towizfile( " Greater Gods" );
               break;
            case MAX_LEVEL - 8:
               towizfile( " Gods" );
               break;
            case MAX_LEVEL - 9:
               towizfile( " Demi Gods" );
               break;
            case MAX_LEVEL - 10:
               towizfile( " Deities" );
               break;
            case MAX_LEVEL - 11:
               towizfile( " Saviors" );
               break;
            case MAX_LEVEL - 12:
               towizfile( " Creators" );
               break;
            case MAX_LEVEL - 13:
               towizfile( " Acolytes" );
               break;
            case MAX_LEVEL - 14:
               towizfile( " Angels" );
               break;
            case MAX_LEVEL - 15:
               towizfile( " Retired" );
               break;
            case MAX_LEVEL - 16:
               towizfile( " Guests" );
               break;
            default:
               towizfile( " Servants" );
               break;
         }
      }
      if( strlen( buf ) + strlen( wiz->name ) > 76 )
      {
         towizfile( buf );
         buf[0] = '\0';
      }
      mudstrlcat( buf, " ", 256 );
      mudstrlcat( buf, wiz->name, 256 );
      if( strlen( buf ) > 70 )
      {
         towizfile( buf );
         buf[0] = '\0';
      }
   }
   if( buf[0] )
      towizfile( buf );

   for( went = wizlist.begin(); went != wizlist.end(); )
   {
      wizent *wiz = (*went);
      ++went;

      deleteptr( wiz );
   }
   wizlist.clear();
}

/*
 *	Makes a wizlist for showing on the Telnet Interface WWW Site -- KCAH
 */
void make_webwiz( void )
{
   DIR *dp;
   struct dirent *dentry;
   FILE *gfp;
   const char *word;
   char buf[MSL], http[MIL];

   wizlist.clear();

   dp = opendir( GOD_DIR );

   int ilevel = 0;
   dentry = readdir( dp );
   while( dentry )
   {
      if( dentry->d_name[0] != '.' )
      {
         /*
          * Added by Tarl 3 Dec 02 because we are now using CVS 
          */
         if( !str_cmp( dentry->d_name, "CVS" ) )
         {
            dentry = readdir( dp );
            continue;
         }
         if( strstr( dentry->d_name, "immlist" ) )
         {
            dentry = readdir( dp );
            continue;
         }

         snprintf( buf, 256, "%s%s", GOD_DIR, dentry->d_name );
         gfp = fopen( buf, "r" );
         if( gfp )
         {
            bitset<MAX_PCFLAG> iflags;
            iflags.reset(  );

            word = feof( gfp ) ? "End" : fread_word( gfp );
            ilevel = fread_number( gfp );
            fread_to_eol( gfp );
            word = feof( gfp ) ? "End" : fread_word( gfp );
            if( !str_cmp( word, "Pcflags" ) )
               flag_set( gfp, iflags, pc_flags );
            word = feof( gfp ) ? "End" : fread_word( gfp );
            if( !str_cmp( word, "Homepage" ) )
               mudstrlcpy( http, fread_flagstring( gfp ), MIL );
            else
               http[0] = '\0';
            FCLOSE( gfp );
            if( iflags.test( PCFLAG_RETIRED ) )
               ilevel = MAX_LEVEL - 15;
            if( iflags.test( PCFLAG_GUEST ) )
               ilevel = MAX_LEVEL - 16;
            add_to_wizlist( dentry->d_name, http, ilevel );
         }
      }
      dentry = readdir( dp );
   }
   closedir( dp );

   unlink( WEBWIZ_FILE );

   buf[0] = '\0';
   ilevel = 65535;

   list<wizent*>::iterator went;
   for( went = wizlist.begin(); went != wizlist.end(); ++went )
   {
      wizent *wiz = (*went);

      if( wiz->level < ilevel )
      {
         if( buf[0] )
         {
            towebwiz( buf );
            buf[0] = '\0';
         }
         towebwiz( "" );
         ilevel = wiz->level;

         switch ( ilevel )
         {
            case MAX_LEVEL - 0:
               towebwiz
                  ( "<p align=\"center\"><font size=\"+1\" color=\"#999999\" face=\"Comic Sans MS\">The Supreme Entity and Implementor</font></p>\n<p align=\"center\">" );
               break;
            case MAX_LEVEL - 1:
               towebwiz
                  ( "</p>\n<p align=\"center\"><font size=\"+1\" color=\"#999999\" face=\"Comic Sans MS\">The Realm Lords</font></p>\n<p align=\"center\">" );
               break;
            case MAX_LEVEL - 2:
               towebwiz
                  ( "</p>\n<p align=\"center\"><font size=\"+1\" color=\"#999999\" face=\"Comic Sans MS\">The Eternals</font></p>\n<p align=\"center\">" );
               break;
            case MAX_LEVEL - 3:
               towebwiz
                  ( "</p>\n<p align=\"center\"><font size=\"+1\" color=\"#999999\" face=\"Comic Sans MS\">The Ancients</font></p>\n<p align=\"center\">" );
               break;
            case MAX_LEVEL - 4:
               towebwiz
                  ( "</p>\n<p align=\"center\"><font size=\"+1\" color=\"#999999\" face=\"Comic Sans MS\">The Astral Gods</font></p>\n<p align=\"center\">" );
               break;
            case MAX_LEVEL - 5:
               towebwiz
                  ( "</p>\n<p align=\"center\"><font size=\"+1\" color=\"#999999\" face=\"Comic Sans MS\">The Elemental Gods</font></p>\n<p align=\"center\">" );
               break;
            case MAX_LEVEL - 6:
               towebwiz
                  ( "</p>\n<p align=\"center\"><font size=\"+1\" color=\"#999999\" face=\"Comic Sans MS\">The Dream Gods</font></p>\n<p align=\"center\">" );
               break;
            case MAX_LEVEL - 7:
               towebwiz
                  ( "</p>\n<p align=\"center\"><font size=\"+1\" color=\"#999999\" face=\"Comic Sans MS\">The Greater Gods</font></p>\n<p align=\"center\">" );
               break;
            case MAX_LEVEL - 8:
               towebwiz
                  ( "</p>\n<p align=\"center\"><font size=\"+1\" color=\"#999999\" face=\"Comic Sans MS\">The Gods</font></p>\n<p align=\"center\">" );
               break;
            case MAX_LEVEL - 9:
               towebwiz
                  ( "</p>\n<p align=\"center\"><font size=\"+1\" color=\"#999999\" face=\"Comic Sans MS\">The Demi Gods</font></p>\n<p align=\"center\">" );
               break;
            case MAX_LEVEL - 10:
               towebwiz
                  ( "</p>\n<p align=\"center\"><font size=\"+1\" color=\"#999999\" face=\"Comic Sans MS\">The Deities</font></p>\n<p align=\"center\">" );
               break;
            case MAX_LEVEL - 11:
               towebwiz
                  ( "</p>\n<p align=\"center\"><font size=\"+1\" color=\"#999999\" face=\"Comic Sans MS\">The Saviors</font></p>\n<p align=\"center\">" );
               break;
            case MAX_LEVEL - 12:
               towebwiz
                  ( "</p>\n<p align=\"center\"><font size=\"+1\" color=\"#999999\" face=\"Comic Sans MS\">The Creators</font></p>\n<p align=\"center\">" );
               break;
            case MAX_LEVEL - 13:
               towebwiz
                  ( "</p>\n<p align=\"center\"><font size=\"+1\" color=\"#999999\" face=\"Comic Sans MS\">The Acolytes</font></p>\n<p align=\"center\">" );
               break;
            case MAX_LEVEL - 14:
               towebwiz
                  ( "</p>\n<p align=\"center\"><font size=\"+1\" color=\"#999999\" face=\"Comic Sans MS\">The Angels</font></p>\n<p align=\"center\">" );
               break;
            case MAX_LEVEL - 15:
               towebwiz
                  ( "</p>\n<p align=\"center\"><font size=\"+1\" color=\"#999999\" face=\"Comic Sans MS\">Retired</font></p>\n<p align=\"center\">" );
               break;
            case MAX_LEVEL - 16:
               towebwiz
                  ( "</p>\n<p align=\"center\"><font size=\"+1\" color=\"#999999\" face=\"Comic Sans MS\">Guests</font></p>\n<p align=\"center\">" );
               break;
            default:
               towebwiz
                  ( "</p>\n<p align=\"center\"><font size=\"+1\" color=\"#999999\" face=\"Comic Sans MS\">Servants</font></p>\n<p align=\"center\">" );
               break;
         }
      }

      mudstrlcat( buf, "<font size=\"+1\" color=\"#FF0000\">", MSL );

      if( strlen( buf ) + strlen( wiz->name ) > 999 )
      {
         towebwiz( buf );
         buf[0] = '\0';
      }

      mudstrlcat( buf, " ", MSL );
      if( wiz->http && wiz->http[0] != '\0' )
      {
         char lbuf[MSL];

         snprintf( lbuf, MSL, "<a href=\"%s\" target=\"_blank\">%s</a>", wiz->http, wiz->name );
         mudstrlcat( buf, lbuf, MSL );
      }
      else
         mudstrlcat( buf, wiz->name, MSL );

      if( strlen( buf ) > 999 )
      {
         towebwiz( buf );
         buf[0] = '\0';
      }
      mudstrlcat( buf, "</font>\n", MSL );
   }

   if( buf[0] )
   {
      mudstrlcat( buf, "</p>\n", MSL );
      towebwiz( buf );
   }

   for( went = wizlist.begin(); went != wizlist.end(); )
   {
      wizent *wiz = (*went);
      ++went;

      deleteptr( wiz );
   }
   wizlist.clear();
}

CMDF( do_makewizlist )
{
   make_wizlist(  );
   make_webwiz(  );
   build_wizinfo(  );
}

system_data::system_data()
{
   init_memory( &dlHandle, &crashhandler, sizeof( crashhandler ) );
}

system_data::~system_data()
{
   DISPOSE( time_of_max );
   DISPOSE( mud_name );
   DISPOSE( admin_email );
   DISPOSE( password );
   DISPOSE( telnet );
   DISPOSE( http );
   DISPOSE( dbserver );
   DISPOSE( dbname );
   DISPOSE( dbuser );
   DISPOSE( dbpass );
}

/*
 * Big mama top level function.
 */
void boot_db( bool fCopyOver )
{
   short wear = 0;
   short x;

   fpArea = NULL;
   show_hash( 32 );
   unlink( BOOTLOG_FILE );
   boot_log( "---------------------[ Boot Log ]--------------------" );
   log_string( "Database bootup starting." );
   fBootDb = true;   /* Supposed to help with EOF bugs, so it got moved up */

   log_string( "Loading sysdata configuration..." );

   sysdata = new system_data;
   /*
    * default values
    */
   sysdata->playersonline = 0; // This one does not save
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
   sysdata->save_flags.reset();
   sysdata->save_flags.set();  /* This defaults to turning on every save_flag */
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
   sysdata->dbserver = str_dup( "localhost" );
   sysdata->dbname = str_dup( "afkdb" );
   sysdata->dbuser = str_dup( "dbuser" );
   sysdata->dbpass = str_dup( "dbpass" );

   if( !load_systemdata(  ) )
   {
      log_string( "Not found. Creating new configuration." );
      sysdata->alltimemax = 0;
      sysdata->mud_name = str_dup( "(Name not set)" );
      update_timers(  );
      update_calendar(  );
      save_sysdata(  );
   }

   log_string( "Initializing libdl support..." );
   /*
    * Open up a handle to the executable's symbol table for later use
    * when working with commands
    */
   sysdata->dlHandle = dlopen( NULL, RTLD_NOW );
   if( !sysdata->dlHandle )
   {
      log_printf( "%s: Error opening local system executable as handle, please check compile flags.", __FUNCTION__ );
      shutdown_mud( "libdl failure" );
      exit( 1 );
   }

   // Call up the module loader
   load_modules();

#if !defined(__CYGWIN__) && defined(SQL)
   log_string( "Initializing MySQL support..." );
   init_mysql();
   add_event( 1800, ev_mysql_ping, NULL );
#endif

   char lbuf[MSL];
   log_string( "Verifying existence of login greeting..." );
   snprintf( lbuf, MSL, "%sgreeting.dat", MOTD_DIR );
   if( !exists_file( lbuf ) )
   {
      bug( "%s: Login greeting not found!", __FUNCTION__ );
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
   load_helps();

   load_mudchannels(  );

   log_string( "Loading overland maps..." );
   load_maps(  );

   log_string( "Loading socials..." );
   load_socials(  );

   log_string( "Loading skill table..." );
   load_skill_table(  );
   sort_skill_table(  );
   remap_slot_numbers(  ); /* must be after the sort */

   gsn_first_spell = 0;
   gsn_first_skill = 0;
   gsn_first_combat = 0;
   gsn_first_tongue = 0;
   gsn_first_ability = 0;
   gsn_first_lore = 0;
   gsn_top_sn = top_sn;

   for( x = 0; x < top_sn; ++x )
   {
      if( !gsn_first_spell && skill_table[x]->type == SKILL_SPELL )
         gsn_first_spell = x;
      else if( !gsn_first_skill && skill_table[x]->type == SKILL_SKILL )
         gsn_first_skill = x;
      else if( !gsn_first_combat && skill_table[x]->type == SKILL_COMBAT )
         gsn_first_combat = x;
      else if( !gsn_first_tongue && skill_table[x]->type == SKILL_TONGUE )
         gsn_first_tongue = x;
      else if( !gsn_first_ability && skill_table[x]->type == SKILL_RACIAL )
         gsn_first_ability = x;
      else if( !gsn_first_lore && skill_table[x]->type == SKILL_LORE )
         gsn_first_lore = x;
   }

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
   log_string( "Loading quest bit tables" );
   load_bits(  );

   nummobsloaded = 0;
   numobjsloaded = 0;
   physicalobjects = 0;
   objlist.clear();
   charlist.clear();
   shoplist.clear();
   repairlist.clear();
   teleportlist.clear();
   room_act_list.clear();
   obj_act_list.clear();
   mob_act_list.clear();
   cur_qobjs = 0;
   cur_qchars = 0;
   extracted_obj_queue = NULL;
   extracted_char_queue = NULL;
   quitting_char = NULL;
   loading_char = NULL;
   saving_char = NULL;
   last_pkroom = 1;

   auction = new auction_data;
   auction->item = NULL;

   weath_unit = 10;
   rand_factor = 2;
   climate_factor = 1;
   neigh_factor = 3;
   max_vector = weath_unit * 3;

   for( wear = 0; wear < MAX_WEAR; ++wear )
      for( x = 0; x < MAX_LAYERS; ++x )
         save_equipment[wear][x] = NULL;

   /*
    * Init random number generator.
    */
   log_string( "Initializing random number generator" );
   srand( current_time );

   log_string( "Setting Astral Walk target room" );
   astral_target = number_range( 4350, 4449 );  /* Added by Samson for Astral Walk spell */

   /*
    * Set time and weather.
    */
   {
      long lhour, lday, lmonth;

      log_string( "Setting time and weather" );

      if( !load_timedata(  ) )   /* Loads time from stored file if true - Samson 1-21-99 */
      {
         boot_log( "Resetting mud time based on current system time." );
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

   /*
    * Read in all the area files.
    */
   {
      FILE *fpList;

      arealist.clear();
      area_nsort.clear();
      area_vsort.clear();
      log_string( "Reading in area files..." );
      if( !( fpList = fopen( AREA_LIST, "r" ) ) )
      {
         perror( AREA_LIST );
         shutdown_mud( "Boot_db: Unable to open area list" );
         exit( 1 );
      }

      for( ;; )
      {
         if( feof( fpList ) )
         {
            bug( "%s: EOF encountered reading area list - no $ found at end of file.", __FUNCTION__ );
            break;
         }
         mudstrlcpy( strArea, fread_word( fpList ), MIL );
         if( strArea[0] == '$' )
            break;

         set_alarm( AREA_FILE_ALARM );
         alarm_section = "boot_db: read area files";
         load_area_file( strArea, false );
         set_alarm( 0 );
      }
      FCLOSE( fpList );
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

   load_clans(  );
   load_deity(  );

   load_equipment_totals( fCopyOver ); /* Samson 10-16-98 - scans pfiles for rares */

   log_string( "Loading corpses..." );
   load_corpses(  );

   if( fCopyOver == true )
   {
      log_string( "Loading world state..." );
      load_world( );
   }

   fBootDb = false;

   log_string( "Making wizlist" );
   make_wizlist(  );
   make_webwiz(  );

   log_string( "Building wizinfo" );
   build_wizinfo(  );

   log_string( "Initializing area reset events..." );
   {
      list<area_data*>::iterator iarea;

      /*
       * Putting some random fuzz on this to scatter the times around more 
       */
      for( iarea = arealist.begin(); iarea != arealist.end(); ++iarea )
      {
         area_data *area = (*iarea);

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

   log_string( "Loading prototype area files..." );
   load_buildlist(  );

   log_string( "Fixing prototype zone exits..." );
   fix_exits(  );

   log_string( "Loading boards..." );
   load_boards(  );

   log_string( "Loading watches..." );
   load_watchlist(  );

   log_string( "Loading bans..." );
   load_banlist(  );

   log_string( "Loading auth namelist..." );
   load_auth_list(  );
   save_auth_list(  );

   log_string( "Loading Immortal Hosts..." );
   load_imm_host(  );

   log_string( "Loading Projects..." );
   load_projects(  );

   log_string( "Loading MXP object commands..." );
   load_mxpobj_cmds(  );

   load_quotes(  );

   /*
    * Morphs MUST be loaded after Class and race tables are set up --Shaddai 
    */
   log_string( "Loading Morphs..." );
   load_morphs(  );
   MOBtrigger = true;

   /*
    * Initialize area weather data 
    */
   load_weatherdata(  );
   init_area_weather(  );

   if( sysdata->webwho > 0 )
      add_event( sysdata->webwho, ev_webwho_refresh, NULL );
   web_arealist();

   /* Initialize pruning events */
   time_t ptime = new_pfile_time_t - current_time;
   if( ptime < 1 )
      ptime = 1;
   add_event( ptime, ev_pfile_check, NULL );

   add_event( 86400, ev_board_check, NULL );

   add_event( 1, ev_dns_check, NULL );

   log_string( "Database bootup completed." );
   return;
}

/* Removal of this function constitutes a license violation */
CMDF( do_basereport )
{
   ch->printf( "&RCodebase revision: %s %s - %s\r\n", CODENAME, CODEVERSION, COPYRIGHT );
   ch->print( "&YContributors: Samson, Dwip, Whir, Cyberfox, Karangi, Rathian, Cam, Raine, and Tarl.\r\n" );
   ch->print( "&BDevelopment site: www.alsherok.net\r\n" );
   ch->print( "&GThis function is included as a means to verify license compliance.\r\n" );
   ch->print( "Removal is a violation of your license.\r\n" );
   ch->print( "Copies of AFKMud beginning with 1.4 as of December 28, 2002 include this.\r\n" );
   ch->print( "Copies found to be running without this command will be subject to a violation report.\r\n" );
}

CMDF( do_memory )
{
   char arg[MIL];
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
   ch->printf( "&wMaxEver: &W%5d\t\t\t&wTopsn:   &W%5d(%5d)\r\n", sysdata->alltimemax, top_sn, MAX_SKILL );
   ch->printf( "&wMaxEver was recorded on:  &W%s\r\n\r\n", sysdata->time_of_max );

   if( !str_cmp( arg, "check" ) )
   {
      ch->print( check_hash( argument ) );
      return;
   }
   if( !str_cmp( arg, "showhigh" ) )
   {
      show_high_hash( atoi( argument ) );
      return;
   }
   if( argument[0] != '\0' )
      hash = atoi( argument );
   else
      hash = -1;
   if( !str_cmp( arg, "hash" ) )
   {
      ch->printf( "Hash statistics:\r\n%s", hash_stats(  ) );
      if( hash != -1 )
         hash_dump( hash );
   }
   return;
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
   ch->printf( "Uterly random number    : %d\r\n", rand() );
   ch->printf( "number_range 4350 - 4449: %d\r\n", number_range( 4350, 4449 ) );
   ch->printf( "number_percent          : %d\r\n", number_percent() );
   ch->printf( "number_door             : %d\r\n", number_door() );
   ch->printf( "number_bits 5           : %d\r\n", number_bits(5) );
   ch->printf( "3d35 ( 3 35 sided dice ): %d\r\n", dice( 3, 35 ) );
   return;
}

/*
 * Dump a text file to a player, a line at a time		-Thoric
 */
void show_file( char_data * ch, const char *filename )
{
   FILE *fp;
   char buf[MSL];
   int c, num = 0;

   if( ( fp = fopen( filename, "r" ) ) != NULL )
   {
      ch->pager( "\r\n" );
      while( !feof( fp ) )
      {
         while( ( buf[num] = fgetc( fp ) ) != EOF && buf[num] != '\n' && buf[num] != '\r' && num < ( MSL - 4 ) )
            ++num;

         c = fgetc( fp );
         if( ( c != '\n' && c != '\r' ) || c == buf[num] )
            ungetc( c, fp );

         buf[num++] = '\n';
         buf[num++] = '\r';
         buf[num] = '\0';
         ch->pager( buf );
         num = 0;
      }
      /*
       * Thanks to stu <sprice@ihug.co.nz> from the mailing list in pointing This out. 
       */
      FCLOSE( fp );
   }
}

/*
 * Append a string to a file.
 */
void append_file( char_data * ch, char *file, char *fmt, ... )
{
   FILE *fp;
   va_list arg;
   char str[MSL];

   va_start( arg, fmt );
   vsnprintf( str, MSL, fmt, arg );
   va_end( arg );

   if( ch->isnpc(  ) || !str || str[0] == '\0' )
      return;

   if( strlen( str ) < 1 || str[strlen( str ) - 1] != '\n' )
      mudstrlcat( str, "\n", MSL );

   if( !( fp = fopen( file, "a" ) ) )
   {
      perror( file );
      ch->print( "Could not open the file!\r\n" );
   }
   else
   {
      fprintf( fp, "[%5d] %s: %s\n", ch->in_room ? ch->in_room->vnum : 0, ch->name, str );
      FCLOSE( fp );
   }
   return;
}

/*
 * Append a string to a file.
 */
void append_to_file( char *file, char *fmt, ... )
{
   FILE *fp;
   va_list arg;
   char str[MSL];

   va_start( arg, fmt );
   vsnprintf( str, MSL, fmt, arg );
   va_end( arg );

   if( !str || str[0] == '\0' )
      return;

   if( strlen( str ) < 1 || str[strlen( str ) - 1] != '\n' )
      mudstrlcat( str, "\n", MSL );

   if( !( fp = fopen( file, "a" ) ) )
      perror( file );
   else
   {
      fprintf( fp, "%s\n", str );
      FCLOSE( fp );
   }
   return;
}

/* Reports a bug. */
/* Now includes backtrace data for that supernifty bug report! - Samson 10-11-03 */
void bug( const char *str, ... )
{
   char buf[MSL];
#if !defined(__CYGWIN__) && !defined(__FreeBSD__)
   void *array[20];
   size_t size, i;
   char **strings;
#endif

   if( fpArea != NULL )
   {
      int iLine;
      int iChar;

      if( fpArea == stdin )
         iLine = 0;
      else
      {
         iChar = ftell( fpArea );
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

   mudstrlcpy( buf, "[*****] BUG: ", MSL );
   {
      va_list param;

      va_start( param, str );
      vsnprintf( buf + strlen( buf ), MSL, str, param );
      va_end( param );
   }
   log_string_plus( LOG_DEBUG, LEVEL_IMMORTAL, buf );

#if !defined(__CYGWIN__) && !defined(__FreeBSD__) && !defined(WIN32)
   if( !fBootDb )
   {
      size = backtrace( array, 20 );
      strings = backtrace_symbols( array, size );

      log_printf_plus( LOG_DEBUG, LEVEL_IMMORTAL, "Obtained %d stack frames.", size );

      for( i = 0; i < size; ++i )
         log_string_plus( LOG_DEBUG, LEVEL_IMMORTAL, strings[i] );

      free( strings );
   }
#endif

   return;
}

/*
 * Writes a string to the log, extended version - Thoric
 */
void log_string_plus( short log_type, short level, const char *str )
{
   struct timeval last_time;
   time_t curtime;
   char *strtime;
   int offset;

   gettimeofday( &last_time, NULL );
   curtime = ( time_t ) last_time.tv_sec;

   strtime = c_time( curtime, -1 );
   fprintf( stderr, "%s :: %s\n", strtime, str );
   if( strncmp( str, "Log ", 4 ) == 0 )
      offset = 4;
   else
      offset = 0;
   switch ( log_type )
   {
      default:
         to_channel( str + offset, "Log", level );
         break;
      case LOG_BUILD:
         to_channel( str + offset, "Build", level );
         break;
      case LOG_COMM:
         to_channel( str + offset, "Comm", level );
         break;
      case LOG_WARN:
         to_channel( str + offset, "Warn", level );
         break;
      case LOG_INFO:
         to_channel( str + offset, "Info", level );
         break;
      case LOG_AUTH:
         to_channel( str + offset, "Auth", level );
         break;
      case LOG_DEBUG:
         to_channel( str + offset, "Bugs", level );
         break;
      case LOG_ALL:
         break;
   }
   return;
}

void log_printf_plus( short log_type, short level, const char *fmt, ... )
{
   char buf[MSL * 2];
   va_list args;

   va_start( args, fmt );
   vsnprintf( buf, MSL * 2, fmt, args );
   va_end( args );

   log_string_plus( log_type, level, buf );
}

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
         if( ( mob = get_mob_index( counter ) ) != NULL )
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
         if( ( room = get_room_index( counter ) ) != NULL )
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
         if( ( obj = get_obj_index( counter ) ) != NULL )
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

            list<affect_data*>::iterator paf;
            for( paf = obj->affects.begin(); paf != obj->affects.end(); ++paf )
            {
               affect_data *af = (*paf);

               if( af->location == APPLY_AFFECT )
                  fprintf( fp, "%s by %s\n", a_types[af->location], aff_flags[af->modifier] );
               else if( af->location == APPLY_WEAPONSPELL
                        || af->location == APPLY_WEARSPELL
                        || af->location == APPLY_REMOVESPELL
                        || af->location == APPLY_STRIPSN
                        || af->location == APPLY_RECURRINGSPELL || af->location == APPLY_EAT_SPELL )
                  fprintf( fp, "%s '%s'\n", a_types[af->location],
                           IS_VALID_SN( af->modifier ) ? skill_table[af->modifier]->name : "UNKNOWN" );
               else if( af->location == APPLY_RESISTANT
                        || af->location == APPLY_IMMUNE
                        || af->location == APPLY_SUSCEPTIBLE || af->location == APPLY_ABSORB )
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
   return;
}

/*
 * This function is here to aid in debugging.
 * If the last expression in a function is another function call,
 *   gcc likes to generate a JMP instead of a CALL.
 * This is called "tail chaining."
 * It hoses the debugger call stack for that call.
 * So I make this the last call in certain critical functions,
 *   where I really need the call stack to be right for debugging!
 *
 * If you don't understand this, then LEAVE IT ALONE.
 * Don't remove any calls to tail_chain anywhere.
 *
 * -- Furey
 */
void tail_chain( void )
{
   return;
}
