/****************************************************************************
 *                   ^     +----- |  / ^     ^ |     | +-\                  *
 *                  / \    |      | /  |\   /| |     | |  \                 *
 *                 /   \   +---   |<   | \ / | |     | |  |                 *
 *                /-----\  |      | \  |  v  | |     | |  /                 *
 *               /       \ |      |  \ |     | +-----+ +-/                  *
 ****************************************************************************
 * AFKMud Copyright 1997-2014 by Roger Libiez (Samson),                     *
 * Levi Beckerson (Whir), Michael Ward (Tarl), Erik Wolfe (Dwip),           *
 * Cameron Carroll (Cam), Cyberfox, Karangi, Rathian, Raine,                *
 * Xorith, and Adjani.                                                      *
 * All Rights Reserved.                                                     *
 *                                                                          *
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
 *                      Calendar Handler/Seasonal Updates                   *
 ****************************************************************************/

#include <fstream>
#include "mud.h"
#include "calendar.h"
#include "pfiles.h"
#include "roomindex.h"

list < holiday_data * >daylist;

const int MAX_TZONE = 25;

extern time_t board_expire_time_t;

struct tzone_type
{
   const char *name; /* Name of the time zone */
   const char *zone; /* Cities or Zones in zone crossing */
   const int gmt_offset;   /* Difference in hours from Greenwich Mean Time */
   const int dst_offset;   /* Day Light Savings Time offset, Not used but left it in anyway */
};

struct tzone_type tzone_table[MAX_TZONE] = {
   {"GMT-12", "Eniwetok", -12, 0},
   {"GMT-11", "Samoa", -11, 0},
   {"GMT-10", "Hawaii", -10, 0},
   {"GMT-9", "Alaska", -9, 0},
   {"GMT-8", "Pacific US", -8, -7},
   {"GMT-7", "Mountain US", -7, -6},
   {"GMT-6", "Central US", -6, -5},
   {"GMT-5", "Eastern US", -5, -4},
   {"GMT-4", "Atlantic, Canada", -4, 0},
   {"GMT-3", "Brazilia, Buenos Aries", -3, 0},
   {"GMT-2", "Mid-Atlantic", -2, 0},
   {"GMT-1", "Cape Verdes", -1, 0},
   {"GMT", "Greenwich Mean Time, Greenwich", 0, 0},
   {"GMT+1", "Berlin, Rome", 1, 0},
   {"GMT+2", "Israel, Cairo", 2, 0},
   {"GMT+3", "Moscow, Kuwait", 3, 0},
   {"GMT+4", "Abu Dhabi, Muscat", 4, 0},
   {"GMT+5", "Islamabad, Karachi", 5, 0},
   {"GMT+6", "Almaty, Dhaka", 6, 0},
   {"GMT+7", "Bangkok, Jakarta", 7, 0},
   {"GMT+8", "Hong Kong, Beijing", 8, 0},
   {"GMT+9", "Tokyo, Osaka", 9, 0},
   {"GMT+10", "Sydney, Melbourne, Guam", 10, 0},
   {"GMT+11", "Magadan, Soloman Is.", 11, 0},
   {"GMT+12", "Fiji, Wellington, Auckland", 12, 0},
};

int tzone_lookup( const string & arg )
{
   int i;

   for( i = 0; i < MAX_TZONE; ++i )
   {
      if( !str_cmp( arg, tzone_table[i].name ) )
         return i;
   }

   for( i = 0; i < MAX_TZONE; ++i )
   {
      if( hasname( tzone_table[i].zone, arg ) )
         return i;
   }
   return -1;
}

CMDF( do_timezone )
{
   int i;

   if( ch->isnpc(  ) )
      return;

   if( argument.empty(  ) )
   {
      ch->printf( "%-6s %-30s (%s)\r\n", "Name", "City/Zone Crosses", "Time" );
      ch->print( "-------------------------------------------------------------------------\r\n" );
      for( i = 0; i < MAX_TZONE; ++i )
      {
         ch->printf( "%-6s %-30s (%s)\r\n", tzone_table[i].name, tzone_table[i].zone, c_time( current_time, i ) );
      }
      ch->print( "-------------------------------------------------------------------------\r\n" );
      return;
   }

   i = tzone_lookup( argument );

   if( i == -1 )
   {
      ch->print( "That time zone does not exists. Make sure to use the exact name.\r\n" );
      return;
   }

   ch->pcdata->timezone = i;
   ch->printf( "Your time zone is now %s %s (%s)\r\n", tzone_table[i].name, tzone_table[i].zone, c_time( current_time, i ) );
}

/* Ever so slightly modified version of "friendly_ctime" provided by Aurora.
 * Merged with the Timezone snippet by Ryan Jennings (Markanth) r-jenn@shaw.ca
 */
char *c_time( time_t curtime, int tz )
{
   static const char *day[] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
   static const char *month[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
   static char strtime[128];
   struct tm *ptime;
   char tzonename[50];

   if( curtime <= 0 )
      curtime = current_time;

   if( tz > -1 && tz < MAX_TZONE )
   {
      mudstrlcpy( tzonename, tzone_table[tz].zone, 50 );
#if defined(__CYGWIN__)
      curtime += ( time_t ) timezone;
#elif defined(__FreeBSD__) || defined(__OpenBSD__)
      /*
       * Hopefully this works 
       */
      ptime = localtime( &curtime );
      curtime += ptime->tm_gmtoff;
#else
      curtime += timezone; /* timezone external variable in time.h holds the 
                            * difference in seconds to GMT. */
#endif
      curtime += ( 60 * 60 * tzone_table[tz].gmt_offset );  /* Add the offset hours */
   }
   ptime = localtime( &curtime );
   if( tz < 0 || tz >= MAX_TZONE )
#if defined(__CYGWIN__) || defined(WIN32)
      mudstrlcpy( tzonename, tzname[ptime->tm_isdst], 50 );
#else
      mudstrlcpy( tzonename, ptime->tm_zone, 50 );
#endif

   snprintf( strtime, 128, "%3s %3s %d, %d %d:%02d:%02d %cM %s",
             day[ptime->tm_wday], month[ptime->tm_mon], ptime->tm_mday, ptime->tm_year + 1900,
             ptime->tm_hour == 0 ? 12 : ptime->tm_hour > 12 ? ptime->tm_hour - 12 : ptime->tm_hour, ptime->tm_min, ptime->tm_sec, ptime->tm_hour < 12 ? 'A' : 'P', tzonename );
   return strtime;
}

/* timeZone is not shown as it's a bit .. long.. but it is respected -- Xorith */
char *mini_c_time( time_t curtime, int tz )
{
   static char strtime[128];
   struct tm *ptime;
   char tzonename[50];

   if( curtime <= 0 )
      curtime = current_time;

   if( tz > -1 && tz < MAX_TZONE )
   {
      mudstrlcpy( tzonename, tzone_table[tz].zone, 50 );
#if defined(__CYGWIN__)
      curtime += ( time_t ) timezone;
#elif defined(__FreeBSD__) || defined(__OpenBSD__)
      /*
       * Hopefully this works 
       */
      ptime = localtime( &curtime );
      curtime += ptime->tm_gmtoff;
#else
      curtime += timezone; /* timezone external variable in time.h holds the
                            * difference in seconds to GMT. */
#endif
      curtime += ( 60 * 60 * tzone_table[tz].gmt_offset );  /* Add the offset hours */
   }
   ptime = localtime( &curtime );
   if( tz < 0 || tz >= MAX_TZONE )
#if defined(__CYGWIN__) || defined(WIN32)
      mudstrlcpy( tzonename, tzname[ptime->tm_isdst], 50 );
#else
      mudstrlcpy( tzonename, ptime->tm_zone, 50 );
#endif

   int year = ptime->tm_year - 100;
   if( year < 0 )
      year = 100 + year;   // Fix negative years for < Y2K

   snprintf( strtime, 128, "%02d/%02d/%02d %02d:%02d%c", ptime->tm_mon + 1, ptime->tm_mday, year,
             ptime->tm_hour == 0 ? 12 : ptime->tm_hour > 12 ? ptime->tm_hour - 12 : ptime->tm_hour, ptime->tm_min, ptime->tm_hour < 12 ? 'A' : 'P' );
   return strtime;
}

/* Time values modified to Alsherok calendar - Samson 5-6-99 */
const char *day_name[] = {
   "Agoras", "Archonis", "Ecclesias", "Talentas", "Pentes",
   "Kilinis", "Piloses", "Koses", "Demes", "Camillies", "Eluthes",
   "Aemiles", "Xenophes"
};

const char *month_name[] = {
   "Olympias", "Planting", "Agamemnus", "Athenas", "Pentes", "Sextes", "Harvest", "Octes",
   "Nones", "Deces", "Graecias", "Terminus"
};

const char *season_name[] = {
   "spring", "summer", "fall", "winter"
};

/* Calling function must insure tstr buffer is large enough.
 * Returns the address of the buffer passed, allowing things like
 * this printf example: 123456secs = 1day 10hrs 17mins 36secs
 *   time_t tsecs = 123456;
 *   char   buff[ MSL ];
 *
 *   printf( "Duration is %s\n", duration( tsecs, buff ) );
 */

const int DUR_SCMN = 60;
const int DUR_MNHR = 60;
const int DUR_HRDY = 24;
const int DUR_DYWK = 7;
#define DUR_ADDS( t )  ( (t) == 1 ? '\0' : 's' )

char *sec_to_hms( time_t loctime, char *tstr )
{
   time_t t_rem;
   int sc, mn, hr, dy, wk;
   int sflg = 0;
   char buff[MSL];

   if( loctime < 1 )
   {
      mudstrlcat( tstr, "no time at all", MSL );
      return ( tstr );
   }

   sc = loctime % DUR_SCMN;
   t_rem = loctime - sc;

   if( t_rem > 0 )
   {
      t_rem /= DUR_SCMN;
      mn = t_rem % DUR_MNHR;
      t_rem -= mn;

      if( t_rem > 0 )
      {
         t_rem /= DUR_MNHR;
         hr = t_rem % DUR_HRDY;
         t_rem -= hr;

         if( t_rem > 0 )
         {
            t_rem /= DUR_HRDY;
            dy = t_rem % DUR_DYWK;
            t_rem -= dy;

            if( t_rem > 0 )
            {
               wk = t_rem / DUR_DYWK;

               if( wk )
               {
                  sflg = 1;
                  snprintf( buff, MSL, "%d week%c", wk, DUR_ADDS( wk ) );
                  mudstrlcat( tstr, buff, MSL );
               }
            }
            if( dy )
            {
               if( sflg == 1 )
                  mudstrlcat( tstr, " ", MSL );
               sflg = 1;
               snprintf( buff, MSL, "%d day%c", dy, DUR_ADDS( dy ) );
               mudstrlcat( tstr, buff, MSL );
            }
         }
         if( hr )
         {
            if( sflg == 1 )
               mudstrlcat( tstr, " ", MSL );
            sflg = 1;
            snprintf( buff, MSL, "%d hour%c", hr, DUR_ADDS( hr ) );
            mudstrlcat( tstr, buff, MSL );
         }
      }
      if( mn )
      {
         if( sflg == 1 )
            mudstrlcat( tstr, " ", MSL );
         sflg = 1;
         snprintf( buff, MSL, "%d minute%c", mn, DUR_ADDS( mn ) );
         mudstrlcat( tstr, buff, MSL );
      }
   }
   if( sc )
   {
      if( sflg == 1 )
         mudstrlcat( tstr, " ", MSL );
      snprintf( buff, MSL, "%d second%c", sc, DUR_ADDS( sc ) );
      mudstrlcat( tstr, buff, MSL );
   }
   return ( tstr );
}

/* Reads the actual time file from disk - Samson 1-21-99 */
void fread_timedata( FILE * fp )
{
   for( ;; )
   {
      const char *word = ( feof( fp ) ? "End" : fread_word( fp ) );

      if( word[0] == '\0' )
      {
         bug( "%s: EOF encountered reading file!", __FUNCTION__ );
         word = "End";
      }

      switch ( UPPER( word[0] ) )
      {
         default:
            bug( "%s: no match: %s", __FUNCTION__, word );
            fread_to_eol( fp );
            break;

         case '*':
            fread_to_eol( fp );
            break;

         case 'B':
            KEY( "Boardtime", board_expire_time_t, fread_number( fp ) );
            break;

         case 'E':
            if( !str_cmp( word, "End" ) )
               return;
            break;

         case 'M':
            KEY( "Mhour", time_info.hour, fread_number( fp ) );
            KEY( "Mday", time_info.day, fread_number( fp ) );
            KEY( "Mmonth", time_info.month, fread_number( fp ) );
            KEY( "Myear", time_info.year, fread_number( fp ) );
            break;

         case 'P':
            KEY( "Purgetime", new_pfile_time_t, fread_number( fp ) );
            break;
      }
   }
}

/* Load time information from saved file - Samson 1-21-99 */
bool load_timedata( void )
{
   char filename[256];
   FILE *fp;
   bool found;

   found = false;
   snprintf( filename, 256, "%stime.dat", SYSTEM_DIR );

   if( ( fp = fopen( filename, "r" ) ) != NULL )
   {

      found = true;
      for( ;; )
      {
         char letter = '\0';
         char *word = NULL;

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
         if( !str_cmp( word, "TIME" ) )
         {
            fread_timedata( fp );
            break;
         }
         else if( !str_cmp( word, "END" ) )
            break;
         else
         {
            bug( "%s: bad section - %s.", __FUNCTION__, word );
            break;
         }
      }
      FCLOSE( fp );
   }
   return found;
}

/* Saves the current game world time to disk - Samson 1-21-99 */
void save_timedata( void )
{
   FILE *fp;
   char filename[256];

   snprintf( filename, 256, "%stime.dat", SYSTEM_DIR );

   if( ( fp = fopen( filename, "w" ) ) == NULL )
   {
      bug( "%s: fopen", __FUNCTION__ );
      perror( filename );
   }
   else
   {
      fprintf( fp, "%s", "#TIME\n" );
      fprintf( fp, "Mhour	%d\n", time_info.hour );
      fprintf( fp, "Mday	%d\n", time_info.day );
      fprintf( fp, "Mmonth	%d\n", time_info.month );
      fprintf( fp, "Myear	%d\n", time_info.year );
      fprintf( fp, "Purgetime %ld\n", ( long int )new_pfile_time_t );
      fprintf( fp, "Boardtime %ld\n", ( long int )board_expire_time_t );
      fprintf( fp, "%s", "End\n\n" );
      fprintf( fp, "%s", "#END\n" );
   }
   FCLOSE( fp );
}

CMDF( do_time )
{
   holiday_data *holiday;
   char buf[MSL];
   const char *suf;
   short day;

   if( !argument.empty(  ) && is_number( argument ) )
   {
      time_t intime = atol( argument.c_str(  ) );

      ch->printf( "&w%ld = &g%s\r\n", intime, mini_c_time( intime, ch->pcdata->timezone ) );
      return;
   }

   day = time_info.day + 1;

   if( day > 4 && day < 20 )
      suf = "th";
   else if( day % 10 == 1 )
      suf = "st";
   else if( day % 10 == 2 )
      suf = "nd";
   else if( day % 10 == 3 )
      suf = "rd";
   else
      suf = "th";

   ch->printf( "&YIt is %d o'clock %s, Day of %s, %d%s day in the Month of %s.\r\n"
               "It is the %s season, in the year %d.\r\n"
               "The mud started up at: %s\r\n"
               "The system time      : %s\r\n",
               ( time_info.hour % sysdata->hournoon == 0 ) ? sysdata->hournoon : time_info.hour % sysdata->hournoon,
               time_info.hour >= sysdata->hournoon ? "pm" : "am", day_name[( time_info.day ) % sysdata->daysperweek], day,
               suf, month_name[time_info.month], season_name[time_info.season], time_info.year, str_boot_time, c_time( current_time, -1 ) );

   ch->printf( "Your local time      : %s\r\n\r\n", c_time( current_time, ch->pcdata->timezone ) );
   holiday = get_holiday( time_info.month, day - 1 );

   if( holiday != NULL )
      ch->printf( "It's a holiday today: %s\r\n", holiday->get_name(  ).c_str(  ) );

   if( !ch->isnpc(  ) )
   {
      if( day == ch->pcdata->day + 1 && time_info.month == ch->pcdata->month )
         ch->print( "Today is your birthday!\r\n" );
   }

   if( ch->is_immortal(  ) && sysdata->CLEANPFILES == true )
   {
      long ptime, curtime;

      ptime = ( long int )( new_pfile_time_t );
      curtime = ( long int )( current_time );

      buf[0] = '\0';
      sec_to_hms( ptime - curtime, buf );
      ch->printf( "The next pfile cleanup is in %s.\r\n", buf );
   }

   if( ch->is_immortal(  ) )
   {
      long qtime, dtime;

      qtime = ( long int )( new_pfile_time_t );
      dtime = ( long int )( current_time );

      buf[0] = '\0';
      sec_to_hms( qtime - dtime, buf );
      ch->printf( "The next rare item update is in %s.\r\n", buf );
   }
}

void start_winter( void )
{
   map < int, room_index * >::iterator iroom;

   echo_to_all( "&cThe air takes on a chilling cold as winter sets in.", ECHOTAR_ALL );
   echo_to_all( "&cFreshwater bodies everywhere have frozen over.\r\n", ECHOTAR_ALL );

   winter_freeze = true;

   for( iroom = room_index_table.begin(); iroom != room_index_table.end(); ++iroom )
   {
      room_index *room = iroom->second;

      switch ( room->sector_type )
      {
         default:
            break;

         case SECT_WATER_SWIM:
         case SECT_WATER_NOSWIM:
            room->winter_sector = room->sector_type;
            room->sector_type = SECT_ICE;
            break;
      }
   }
}

void start_spring( void )
{
   map < int, room_index * >::iterator iroom;

   echo_to_all( "&cThe chill recedes from the air as spring begins to take hold.", ECHOTAR_ALL );
   echo_to_all( "&cFreshwater bodies everywhere have thawed out.\r\n", ECHOTAR_ALL );

   winter_freeze = false;

   for( iroom = room_index_table.begin(); iroom != room_index_table.end(); ++iroom )
   {
      room_index *room = iroom->second;

      if( room->sector_type == SECT_ICE && room->winter_sector != -1 )
      {
         room->sector_type = room->winter_sector;
         room->winter_sector = -1;
      }
   }
}

void start_summer( void )
{
   echo_to_all( "&YThe days grow longer and hotter as summer grips the world.\r\n", ECHOTAR_ALL );
}

void start_fall( void )
{
   echo_to_all( "&OThe leaves begin changing colors signaling the start of fall.\r\n", ECHOTAR_ALL );
}

void season_update( void )
{
   holiday_data *day;

   day = get_holiday( time_info.month, time_info.day );

   if( day != NULL )
   {
      if( time_info.day + 1 == day->get_day(  ) && time_info.hour == 0 )
         echo_all_printf( ECHOTAR_ALL, "&[immortal]%s", day->get_announce(  ).c_str(  ) );
   }

   if( time_info.season == SEASON_WINTER && winter_freeze == false )
   {
      map < int, room_index * >::iterator iroom;

      winter_freeze = true;

      for( iroom = room_index_table.begin(); iroom != room_index_table.end(); ++iroom )
      {
         room_index *room = iroom->second;

         switch ( room->sector_type )
         {
            default:
               break;

            case SECT_WATER_SWIM:
            case SECT_WATER_NOSWIM:
               room->winter_sector = room->sector_type;
               room->sector_type = SECT_ICE;
               break;
         }
      }
   }
}

/* PaB: Which season are we in? 
 * Notes: Simply figures out which season the current month falls into
 * and returns a proper value.
 */
void calc_season( void )
{
   /*
    * How far along in the year are we, measured in days? 
    * PaB: Am doing this in days to minimize roundoff impact 
    */
   int day = time_info.month * sysdata->dayspermonth + time_info.day;

   if( day < ( sysdata->daysperyear / 4 ) )
   {
      time_info.season = SEASON_SPRING;
      if( time_info.hour == 0 && day == 0 )
         start_spring(  );
   }
   else if( day < ( sysdata->daysperyear / 4 ) * 2 )
   {
      time_info.season = SEASON_SUMMER;
      if( time_info.hour == 0 && day == ( sysdata->daysperyear / 4 ) )
         start_summer(  );
   }
   else if( day < ( sysdata->daysperyear / 4 ) * 3 )
   {
      time_info.season = SEASON_FALL;
      if( time_info.hour == 0 && day == ( ( sysdata->daysperyear / 4 ) * 2 ) )
         start_fall(  );
   }
   else if( day < sysdata->daysperyear )
   {
      time_info.season = SEASON_WINTER;
      if( time_info.hour == 0 && day == ( ( sysdata->daysperyear / 4 ) * 3 ) )
         start_winter(  );
   }
   else
      time_info.season = SEASON_SPRING;

   season_update(  );   /* Maintain the season in case of reboot, check for holidays */
}

holiday_data::holiday_data(  )
{
   init_memory( &month, &day, sizeof( day ) );
}

holiday_data::~holiday_data(  )
{
   daylist.remove( this );
}

void free_holidays( void )
{
   list < holiday_data * >::iterator day;

   for( day = daylist.begin(  ); day != daylist.end(  ); )
   {
      holiday_data *holiday = *day;
      ++day;

      deleteptr( holiday );
   }
}

void check_holiday( char_data * ch )
{
   holiday_data *day;   /* To inform incoming players of holidays - Samson 6-3-99 */

   /*
    * Inform incoming player of any holidays - Samson 6-3-99 
    */
   day = get_holiday( time_info.month, time_info.day );

   if( day != NULL )
      ch->printf( "&Y%s\r\n", day->get_announce(  ).c_str(  ) );
}

holiday_data *get_holiday( short month, short day )
{
   list < holiday_data * >::iterator hld;

   for( hld = daylist.begin(  ); hld != daylist.end(  ); ++hld )
   {
      holiday_data *holiday = *hld;

      if( month + 1 == holiday->get_month(  ) && day + 1 == holiday->get_day(  ) )
         return holiday;
   }
   return NULL;
}

holiday_data *get_holiday( const string & name )
{
   list < holiday_data * >::iterator hld;

   for( hld = daylist.begin(  ); hld != daylist.end(  ); ++hld )
   {
      holiday_data *holiday = *hld;

      if( !str_cmp( name, holiday->get_name(  ) ) )
         return holiday;
   }
   return NULL;
}

CMDF( do_holidays )
{
   list < holiday_data * >::iterator day;

   ch->pager( "&RHoliday                &YMonth          &GDay\r\n" );
   ch->pager( "&g----------------------+----------------+---------------\r\n" );

   for( day = daylist.begin(  ); day != daylist.end(  ); ++day )
   {
      holiday_data *holiday = *day;

      ch->pagerf( "&G%-21s &g%-11s %-2d\r\n", holiday->get_name(  ).c_str(  ), month_name[holiday->get_month(  ) - 1], holiday->get_day(  ) );
   }
}

/* Load the holiday file */
void load_holidays( void )
{
   holiday_data *day;
   ifstream stream;

   daylist.clear(  );

   stream.open( HOLIDAY_FILE );
   if( !stream.is_open(  ) )
   {
      log_string( "No holiday file found." );
      return;
   }

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

      if( key == "#HOLIDAY" )
         day = new holiday_data;
      else if( key == "Name" )
         day->set_name( value );
      else if( key == "Announce" )
         day->set_announce( value );
      else if( key == "Month" )
         day->set_month( atoi( value.c_str(  ) ) );
      else if( key == "Day" )
         day->set_day( atoi( value.c_str(  ) ) );
      else if( key == "End" )
         daylist.push_back( day );
      else
         log_printf( "%s: Bad line in holiday file: %s %s", __FUNCTION__, key.c_str(  ), value.c_str(  ) );
   }
   while( !stream.eof(  ) );
   stream.close(  );
}

/* Save the holidays to disk - Samson 5-6-99 */
void save_holidays( void )
{
   ofstream stream;

   stream.open( HOLIDAY_FILE );
   if( !stream.is_open(  ) )
   {
      bug( "%s: fopen", __FUNCTION__ );
      perror( HOLIDAY_FILE );
   }
   else
   {
      list < holiday_data * >::iterator hday;
      for( hday = daylist.begin(  ); hday != daylist.end(  ); ++hday )
      {
         holiday_data *day = *hday;

         stream << "#HOLIDAY" << endl;
         stream << "Name     " << day->get_name(  ) << endl;
         stream << "Announce " << day->get_announce(  ) << endl;
         stream << "Month    " << day->get_month(  ) << endl;
         stream << "Day      " << day->get_day(  ) << endl;
         stream << "End" << endl << endl;
      }
      stream.close(  );
   }
}

/* Holiday OLC command - (c)Andrew Wilkie May-20-2005 */
CMDF( do_setholiday )
{
   string arg, arg2;
   holiday_data *day, *newday;
   int count = 0, x = 0, value = 0;

   argument = one_argument( argument, arg );
   argument = one_argument( argument, arg2 );
   if( arg.empty(  ) || arg2.empty(  ) || argument.empty(  ) )
   {
      ch->print( "Syntax: setholiday <name> <field> <argument>\r\n" );
      ch->print( "Field can be: day name create announce save delete\r\n" );
      return;
   }

   /*
    * Create a new holiday by name arg1 
    */
   if( !str_cmp( arg2, "create" ) )
   {
      list < holiday_data * >::iterator hld;
      for( hld = daylist.begin(  ); hld != daylist.end(  ); ++hld )
      {
         day = *hld;

         if( !str_cmp( arg, day->get_name(  ) ) )
         {
            ch->print( "A holiday with that name exists already!\r\n" );
            return;
         }
      }

      if( daylist.size(  ) >= sysdata->maxholiday )
      {
         ch->print( "There are already too many holidays!\r\n" );
         return;
      }

      newday = new holiday_data;
      newday->set_name( arg );
      newday->set_day( time_info.day );
      newday->set_month( time_info.month );
      newday->set_announce( "Today is the holiday of when some moron forgot to set the announcement for this one!" );
      daylist.push_back( newday );
      save_holidays(  );
      ch->print( "Holiday created.\r\n" );
      return;
   }

   /*
    * Now... let's find that holiday 
    */
   /*
    * Anything match? 
    */
   if( !( day = get_holiday( arg ) ) )
   {
      ch->print( "Which holiday was that?\r\n" );
      return;
   }

   /*
    * Set the day 
    */
   if( !str_cmp( arg2, "day" ) )
   {
      value = atoi( argument.c_str(  ) );
      if( argument.empty(  ) || !is_number( argument ) || value > sysdata->dayspermonth || value < 1 )
      {
         ch->printf( "You must specify a numeric value : %d - %d", 1, sysdata->dayspermonth );
         return;
      }

      /*
       * What day is it?... FRIDAY!.. oh... no... it's.. argument? 
       */
      day->set_day( value );
      save_holidays(  );
      ch->print( "Day changed\r\n" );
      return;
   }

   if( !str_cmp( arg2, "month" ) )
   {
      value = atoi( argument.c_str(  ) );
      /*
       * Go through the months and find argument
       */
      if( argument.empty(  ) || !is_number( argument ) || value > sysdata->monthsperyear || value < 1 )
      {
         ch->print( "You must specify a valid month number:\r\n" );

         /*
          * List all the months with a counter next to them 
          */
         count = 1;
         while( month_name[x] != '\0' && str_cmp( month_name[x], " " ) && x < sysdata->monthsperyear )
         {
            ch->printf( "&[red](&[white]%d&[red])&[yellow]%s\r\n", count, month_name[x] );
            ++x;
            ++count;
         }
         return;
      }

      /*
       * What's the month? 
       */
      day->set_month( value );
      save_holidays(  );
      ch->print( "Month changed\r\n" );
      return;
   }

   if( !str_cmp( arg2, "announce" ) )
   {
      if( argument.empty(  ) )
      {
         ch->print( "Set the annoucement to what?\r\n" );
         return;
      }

      /*
       * Set the announcement 
       */
      day->set_announce( argument );
      save_holidays(  );
      ch->print( "Announcement changed\r\n" );
      return;
   }

   /*
    * Change the name 
    */
   if( !str_cmp( arg2, "name" ) )
   {
      if( argument.empty(  ) )
      {
         ch->print( "Set the name to what?\r\n" );
         return;
      }

      day->set_name( argument );
      save_holidays(  );
      ch->print( "Name changed\r\n" );
      return;
   }

   if( !str_cmp( arg2, "delete" ) )
   {
      if( argument.empty(  ) || str_cmp( argument, "yes" ) )
      {
         ch->print( "If you are sure, use 'delete yes'\r\n" );
         return;
      }

      deleteptr( day );
      save_holidays(  );
      ch->print( "&RHoliday deleted\r\n" );
      return;
   }
   do_setholiday( ch, "" );
}
