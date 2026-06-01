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
 *                      Calendar Handler/Seasonal Updates                   *
 ****************************************************************************/

#include <fstream>
#include <format>
#include <filesystem>
#include "mud.h"
#include "calendar.h"
#include "pfiles.h"
#include "roomindex.h"

list < holiday_data * >daylist;

const int MAX_TZONE = 25;

extern std::chrono::system_clock::time_point board_expire_time_t;

struct tzone_type
{
   std::string_view display_label; // e.g., "Pacific US"
   std::string_view iana_id;       // e.g., "America/Los_Angeles"
};

inline constexpr std::array<tzone_type, 25> tzone_table = {{
   {"Eniwetok",             "Pacific/Kwajalein"},
   {"Samoa",                "Pacific/Samoa"},
   {"Hawaii",               "Pacific/Honolulu"},
   {"Alaska",               "America/Anchorage"},
   {"Pacific US",           "America/Los_Angeles"},
   {"Mountain US",          "America/Denver"},
   {"Central US",           "America/Chicago"},
   {"Eastern US",           "America/New_York"},
   {"Atlantic Canada",      "America/Halifax"},
   {"Brazil/Argentina",     "America/Sao_Paulo"},
   {"Mid-Atlantic",         "Atlantic/South_Georgia"},
   {"Cape Verde",           "Atlantic/Cape_Verde"},
   {"Greenwich Mean Time",  "Etc/GMT"},
   {"Berlin, Rome",         "Europe/Berlin"},
   {"Israel, Cairo",        "Africa/Cairo"},
   {"Moscow, Kuwait",       "Europe/Moscow"},
   {"Abu Dhabi, Muscat",    "Asia/Dubai"},
   {"Islamabad, Karachi",   "Asia/Karachi"},
   {"Almaty, Dhaka",        "Asia/Almaty"},
   {"Bangkok, Jakarta",     "Asia/Bangkok"},
   {"Hong Kong, Beijing",   "Asia/Shanghai"},
   {"Tokyo, Osaka",         "Asia/Tokyo"},
   {"Sydney, Melbourne",    "Australia/Sydney"},
   {"Magadan, Solomon Is.", "Asia/Magadan"},
   {"Fiji, Auckland",       "Pacific/Auckland"}
}};

int tzone_lookup( const std::string & arg )
{
   for( int i = 0; i < static_cast<int>( tzone_table.size() ); ++i )
   {
      if( !str_cmp( arg, tzone_table[i].display_label.data() ) )
         return i;
   }

   for( int i = 0; i < static_cast<int>( tzone_table.size() ); ++i )
   {
      if( hasname( tzone_table[i].iana_id.data(), arg ) )
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
      ch->printf( "%-6s %-30s (%s)\r\n", "Name", "IANA Timezone Name", "Time" );
      ch->print( "-------------------------------------------------------------------------\r\n" );
      for( i = 0; i < static_cast<int>( tzone_table.size() ); ++i )
      {
         ch->printf( "%-6s %-30s (%s)\r\n", tzone_table[i].display_label.data(), tzone_table[i].iana_id.data(), c_time( current_time, i ).c_str() );
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
   ch->printf( "Your time zone is now %s %s (%s)\r\n", tzone_table[i].display_label.data(), tzone_table[i].iana_id.data(), c_time( current_time, i ).c_str() );
}

/*
 * Ever so slightly modified version of "friendly_ctime" provided by Aurora.
 * Merged with the Timezone snippet by Ryan Jennings (Markanth) r-jenn@shaw.ca
 */
// Returns a localized time string based on an index from tzone_table.
std::string c_time( std::chrono::system_clock::time_point curtime, int tz )
{
   // Select the time zone string.
   auto zone_id = ( tz >= 0 && tz < static_cast<int>( tzone_table.size() ) ) ? tzone_table[tz].iana_id : "GMT";

   // Create a zoned_time object using the specified zone identifier
   auto zt = std::chrono::zoned_time{ zone_id, curtime };

   // Format: Sun Jan 01, 2026 12:00:00 PM UTC
   // %I is 12-hour clock, %p is AM/PM, %Z is zone name.
   return std::format( "{:%a %b %d, %Y %I:%M:%S %p} {}", zt, zone_id );
}

// Returns a compact localized time string.
std::string mini_c_time( std::chrono::system_clock::time_point curtime, int tz )
{
   // Select the time zone string.
   auto zone_id = ( tz >= 0 && tz < static_cast<int>( tzone_table.size() ) ) ? tzone_table[tz].iana_id : "GMT";

   // Create a zoned_time object using the specified zone identifier
   auto zt = std::chrono::zoned_time{ zone_id, curtime };

   // Format: 05/30/26 12:00PM
   // %m/%d/%y for date, %I:%M%p for 12hr time.
   return std::format( "{:%a %b %d, %Y %I:%M:%S %p} %Z", zt );
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

/* Reads the actual time file from disk - Samson 1-21-99 */
void fread_timedata( FILE * fp )
{
   for( ;; )
   {
      const char *word = ( feof( fp ) ? "End" : fread_word( fp ) );

      if( word[0] == '\0' )
      {
         bug( "%s: EOF encountered reading file!", __func__ );
         word = "End";
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

         case 'B':
            if( !str_cmp( word, "Boardtime" ) )
            {
               time_t loaded_time = fread_long( fp );
               board_expire_time_t = std::chrono::system_clock::from_time_t( loaded_time );
            }
            break;

         case 'E':
            if( !str_cmp( word, "End" ) )
               return;
            break;

         case 'M':
            KEY( "Mhour", time_info.hour, fread_long( fp ) );
            KEY( "Mday", time_info.day, fread_long( fp ) );
            KEY( "Mmonth", time_info.month, fread_long( fp ) );
            KEY( "Myear", time_info.year, fread_long( fp ) );
            break;

         case 'P':
            if( !str_cmp( word, "Purgetime") )
            {
               time_t loaded_time = fread_long( fp );
               new_pfile_time_t = std::chrono::system_clock::from_time_t( loaded_time );
               break;
            }
            break;
      }
   }
}

/* Load time information from saved file - Samson 1-21-99 */
bool load_timedata( void )
{
   std::filesystem::path filename;
   FILE *fp;
   bool found;

   found = false;
   filename = std::format( "{}time.dat", SYSTEM_DIR );

   if( ( fp = fopen( filename.c_str(), "r" ) ) != nullptr )
   {
      found = true;

      for( ;; )
      {
         char letter = '\0';
         char *word = nullptr;

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
         if( !str_cmp( word, "TIME" ) )
         {
            fread_timedata( fp );
            break;
         }
         else if( !str_cmp( word, "END" ) )
            break;
         else
         {
            bug( "%s: bad section - %s.", __func__, word );
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
   std::filesystem::path filename;

   filename = std::format( "{}time.dat", SYSTEM_DIR );

   if( ( fp = fopen( filename.c_str(), "w" ) ) == nullptr )
   {
      bug( "%s: fopen", __func__ );
      perror( filename.c_str() );
   }
   else
   {
      auto purgetime = std::chrono::system_clock::to_time_t( new_pfile_time_t );
      auto boardtime = std::chrono::system_clock::to_time_t( board_expire_time_t );

      fprintf( fp, "%s", "#TIME\n" );
      fprintf( fp, "Mhour	%d\n", time_info.hour );
      fprintf( fp, "Mday	%d\n", time_info.day );
      fprintf( fp, "Mmonth	%d\n", time_info.month );
      fprintf( fp, "Myear	%d\n", time_info.year );
      fprintf( fp, "Purgetime %ld\n", purgetime );
      fprintf( fp, "Boardtime %ld\n", boardtime );
      fprintf( fp, "%s", "End\n\n" );
      fprintf( fp, "%s", "#END\n" );
   }
   FCLOSE( fp );
}

CMDF( do_time )
{
   holiday_data *holiday;
   const char *suf;
   short day;

   if( !argument.empty(  ) && is_number( argument ) )
   {
      time_t intime = atol( argument.c_str(  ) );

      std::chrono::system_clock::time_point str_time = std::chrono::system_clock::from_time_t( intime );

      ch->printf( "&w%ld = &g%s\r\n", intime, mini_c_time( str_time, ch->pcdata->timezone ).c_str() );
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
               suf, month_name[time_info.month], season_name[time_info.season], time_info.year, str_boot_time.c_str(), c_time( current_time, -1 ).c_str() );

   ch->printf( "Your local time      : %s\r\n\r\n", c_time( current_time, ch->pcdata->timezone ).c_str() );
   holiday = get_holiday( time_info.month, day - 1 );

   if( holiday != nullptr )
      ch->printf( "It's a holiday today: %s\r\n", holiday->get_name(  ).c_str(  ) );

   if( !ch->isnpc(  ) )
   {
      if( day == ch->pcdata->day + 1 && time_info.month == ch->pcdata->month )
         ch->print( "Today is your birthday!\r\n" );
   }

   if( ch->is_immortal(  ) && sysdata->CLEANPFILES == true )
   {
      std::string buf = std::format( "Next Pfile cleanup: {:%Y-%m-%d %H:%M:%S}\r\n", new_pfile_time_t );

      ch->print( buf );
   }

   if( ch->is_immortal(  ) )
   {
      std::string buf = std::format( "Next rare items cleanup: {:%Y-%m-%d %H:%M:%S}\r\n", new_pfile_time_t );

      ch->print( buf );
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

   if( day != nullptr )
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

   if( day != nullptr )
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
   return nullptr;
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
   return nullptr;
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
   holiday_data *day = nullptr;
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
         log_printf( "%s: Bad line in holiday file: %s %s", __func__, key.c_str(  ), value.c_str(  ) );
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
      bug( "%s: fopen", __func__ );
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
         while( month_name[x][0] != '\0' && str_cmp( month_name[x], " " ) && x < sysdata->monthsperyear )
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
