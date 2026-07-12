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
 *                      Calendar Handler/Seasonal Updates                   *
 ****************************************************************************/

#include <filesystem>
#include <fstream>
#include "mud.h"
#include "calendar.h"
// #include "pfiles.h"
#include "roomindex.h"

std::list<holiday_data *> daylist;

extern std::chrono::system_clock::time_point board_expire_time_t;
extern std::chrono::system_clock::time_point new_pfile_time_t;

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
   {"Greenwich Mean Time",  "Etc/UTC"},
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

std::string convert_old_timezone( int t_zone )
{
   // Invalid timezones convert to server local time.
   if( t_zone < 0 || t_zone > 24 )
      return std::string( std::chrono::current_zone()->name() );

   // Otherwise the player had a valid one on them, return that.
   return std::string( tzone_table[t_zone].iana_id );
}

const std::chrono::time_zone* get_tz( const std::string & iana_name )
{
   try
   {
      return std::chrono::locate_zone( iana_name );
   }
   catch( const std::runtime_error & )
   {
      return std::chrono::current_zone(); // Timezone was invalid, fall back to server local time.
   }
}

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
   if( ch->isnpc() )
      return;

   const auto& db = std::chrono::get_tzdb();

   // Show the user's current time if no argument is provided.
   if( argument.empty() )
   {
      ch->print( "Syntax: timezone <list>     - Shows a list of all available timezones.\r\n" );
      ch->print( "Syntax: timezone <timezone> - Set your timezone using the IANA name in the list.\r\n\r\n" );

      ch->print_fmt( "Your timezone name: {}\r\n", ch->pcdata->timezone_name );
      ch->print_fmt( "Your current time : {}\r\n", c_time( current_time, ch->pcdata->timezone_name ) );
      return;
   }

   // List all available zones.
   if( !str_cmp( argument, "list" ) )
   {
      ch->pager_fmt("{:<30} {}\r\n", "IANA Timezone Name", "Current Time");
      ch->pager("----------------------------------------------------------\r\n");

      for( const auto& zone : db.zones )
      {
         // Using zoned_time to format the time automatically for that zone.
         std::chrono::zoned_time zt{zone.name(), current_time};
         ch->pager_fmt( "{:<30} {:%a %b %d, %Y %I:%M:%S %p %Z}\r\n", zone.name(), zt );
      }
      return;
   }

   // Lookup a specific zone to set.
   const std::chrono::time_zone* zone = get_tz( argument );

   if( !zone )
   {
      ch->print( "That time zone does not exist. Please use a valid IANA name (e.g., 'America/New_York').\r\n" );
      return;
   }

   // Update player data.
   ch->pcdata->timezone_name = zone->name();

   ch->print_fmt( "Your time zone is now set to {}.\r\n", zone->name() );
   ch->print_fmt( "Current time: {:%a %b %d, %Y %I:%M:%S %p %Z}\r\n", std::chrono::zoned_time{zone, current_time} );
   ch->save();
}

/*
 * Ever so slightly modified version of "friendly_ctime" provided by Aurora.
 * Merged with the Timezone snippet by Ryan Jennings (Markanth) r-jenn@shaw.ca
 */
// Returns a localized time string based on an index from tzone_table.
const std::string c_time( std::chrono::system_clock::time_point curtime, const std::string & iana_id )
{
   const std::chrono::time_zone* zone = nullptr;

   if( !iana_id.empty() )
   {
      try
      {
         // Find the timezone in the database.
         zone = std::chrono::locate_zone( iana_id );
      }
      catch( const std::runtime_error & )
      {
         // Fallback to server timezone if the zone is invalid.
         zone = std::chrono::current_zone();
      }
   }
   else // Empty string means use the server timezone.
      zone = std::chrono::current_zone();

   // Convert to the target time zone.
//   std::chrono::zoned_time zt{zone, curtime};
   std::chrono::zoned_time zt{zone, std::chrono::floor<std::chrono::seconds>(curtime)};

   // Format: Sun Jan 01, 2026 12:00:00 PM UTC
   // %I is 12-hour clock, %p is AM/PM, %Z is zone name.
   return std::format( "{:%a %b %d, %Y %I:%M:%S %p %Z}", zt );
}

const std::string mini_c_time( std::chrono::system_clock::time_point curtime, const std::string & iana_id )
{
   const std::chrono::time_zone* zone = nullptr;

   if( !iana_id.empty() )
   {
      try
      {
         // Find the timezone in the database.
         zone = std::chrono::locate_zone( iana_id );
      }
      catch( const std::runtime_error & )
      {
         // Fallback to server timezone if the zone is invalid.
         zone = std::chrono::current_zone();
      }
   }
   else
      zone = std::chrono::current_zone();

   // Convert to the target time zone.
//   std::chrono::zoned_time zt{zone, curtime};
   std::chrono::zoned_time zt{zone, std::chrono::floor<std::chrono::seconds>(curtime)};

   // Format: Sun Jan 01, 2026 12:00:00 PM UTC
   // %I is 12-hour clock, %p is AM/PM, %Z is zone name.
   return std::format( "{:%a %b %d, %Y %I:%M%p %Z}", zt );
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

/* Load time information from saved file - Samson 1-21-99 */
bool load_timedata( void )
{
   bool found = false;

   std::ifstream stream( std::filesystem::path{TIME_FILE} );
   if( !stream.is_open() )
   {
      bug( "{}: Cannot open {} for reading: {}", __func__, TIME_FILE, std::strerror(errno) );
      return found;
   }

   std::string key;
   while( stream >> key )
   {
      if( key == "#TIME" )
         ; // Do nothing, the #TIME marker has nothing to process.
      else if( key == "Mhour" )
         stream >> time_info.hour;
      else if( key == "Mday" )
         stream >> time_info.day;
      else if( key == "Mmonth" )
         stream >> time_info.month;
      else if( key == "Myear" )
         stream >>  time_info.year;
      else if( key == "Purgetime" )
      {
         time_t loaded_time;
         stream >> loaded_time;

         new_pfile_time_t = std::chrono::system_clock::from_time_t( loaded_time );
      }
      else if( key == "Boardtime" )
      {
         time_t loaded_time;
         stream >> loaded_time;

         board_expire_time_t = std::chrono::system_clock::from_time_t( loaded_time );
      }
      else if( key == "End" || key == "#END" )
      {
         found = true;
         break;
      }
      else
         bug( "{}: Bad section - {} in timedata.", __func__, key );
   }
   stream.close();
   return found;
}

/* Saves the current game world time to disk - Samson 1-21-99 */
void save_timedata( void )
{
   std::ofstream stream( std::filesystem::path{TIME_FILE} );
   if( !stream.is_open() )
   {
      bug( "{}: Cannot open {} for writing: {}", __func__, TIME_FILE, std::strerror(errno) );
      return;
   }

   auto purgetime = std::chrono::system_clock::to_time_t( new_pfile_time_t );
   auto boardtime = std::chrono::system_clock::to_time_t( board_expire_time_t );

   stream << "#TIME\n";
   stream << std::format( "Mhour    {}\n", time_info.hour );
   stream << std::format( "Mday     {}\n", time_info.day );
   stream << std::format( "Mmonth   {}\n", time_info.month );
   stream << std::format( "Myear    {}\n", time_info.year );
   stream << std::format( "Purgetime {}\n", purgetime );
   stream << std::format( "Boardtime {}\n", boardtime );
   stream << "#END\n";
   stream.close();
   if( stream.fail() )
      bug( "{}: Error occurred after closing {}: ", __func__, TIME_FILE, std::strerror(errno) );
}

CMDF( do_time )
{
   holiday_data *holiday;
   std::string suf;
   short day;

   if( !argument.empty(  ) && is_number( argument ) )
   {
      time_t intime = std::stol( argument );

      std::chrono::system_clock::time_point str_time = std::chrono::system_clock::from_time_t( intime );

      ch->print_fmt( "&w{} = &g{}\r\n", intime, c_time( str_time, ch->pcdata->timezone_name ) );
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

   ch->print_fmt( "&YIt is {} o'clock {}, Day of {}, {}{} day in the Month of {}.\r\n"
               "It is the {} season, in the year {}.\r\n"
               "The mud started up at: {}\r\n"
               "The system time      : {}\r\n",
               ( time_info.hour % sysdata->hournoon == 0 ) ? sysdata->hournoon : time_info.hour % sysdata->hournoon,
               time_info.hour >= sysdata->hournoon ? "pm" : "am", day_name[( time_info.day ) % sysdata->daysperweek], day,
               suf, month_name[time_info.month], season_name[time_info.season], time_info.year, str_boot_time, c_time( current_time, "" ) );

   ch->print_fmt( "Your local time      : {}\r\n\r\n", c_time( current_time, ch->pcdata->timezone_name ) );
   holiday = get_holiday( time_info.month, day - 1 );

   if( holiday != nullptr )
      ch->print_fmt( "It's a holiday today: {}\r\n", holiday->get_name(  ) );

   if( !ch->isnpc(  ) )
   {
      if( day == ch->pcdata->day + 1 && time_info.month == ch->pcdata->month )
         ch->print( "Today is your birthday!\r\n" );
   }

   if( ch->is_immortal(  ) && sysdata->CLEANPFILES == true )
   {
      std::string buf = std::format( "Next Pfile cleanup: {}\r\n", c_time( new_pfile_time_t, ch->pcdata->timezone_name ) );

      ch->print( buf );
   }

   if( ch->is_immortal(  ) )
   {
      std::string buf = std::format( "Next rare items cleanup: {}\r\n", c_time( new_pfile_time_t, ch->pcdata->timezone_name ) );

      ch->print( buf );
   }
}

void start_winter( void )
{
   std::map<int, room_index *>::iterator iroom;

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
   std::map<int, room_index *>::iterator iroom;

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
         echo_all_printf( ECHOTAR_ALL, "&[immortal]{}", day->get_announce(  ) );
   }

   if( time_info.season == SEASON_WINTER && winter_freeze == false )
   {
      std::map<int, room_index *>::iterator iroom;

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
}

holiday_data::~holiday_data(  )
{
   daylist.remove( this );
}

void free_holidays( void )
{
   for( auto it = daylist.begin(  ); it != daylist.end(  ); )
   {
      holiday_data *holiday = *it;
      ++it;

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
      ch->print_fmt( "&Y{}\r\n", day->get_announce(  ) );
}

holiday_data *get_holiday( short month, short day )
{
   for( auto* holiday : daylist )
   {
      if( month + 1 == holiday->get_month(  ) && day + 1 == holiday->get_day(  ) )
         return holiday;
   }
   return nullptr;
}

holiday_data *get_holiday( std::string_view name )
{
   for( auto* holiday : daylist )
   {
      if( !str_cmp( name, holiday->get_name(  ) ) )
         return holiday;
   }
   return nullptr;
}

CMDF( do_holidays )
{
   ch->pager( "&RHoliday                &YMonth          &GDay\r\n" );
   ch->pager( "&g----------------------+----------------+---------------\r\n" );

   for( auto* holiday : daylist )
   {
      ch->pager_fmt( "&G{:<21} &g{:<11} {:<2}\r\n", holiday->get_name(  ), month_name[holiday->get_month(  ) - 1], holiday->get_day(  ) );
   }
}

/* Load the holiday file */
void load_holidays( void )
{
   std::ifstream stream( std::filesystem::path{HOLIDAY_FILE} );
   if( !stream.is_open() )
   {
      bug( "{}: Cannot open {} for reading: {}", __func__, HOLIDAY_FILE, std::strerror(errno) );
      return;
   }

   daylist.clear(  );
   holiday_data *day = nullptr;
   std::string key;
   while( stream >> key )
   {
      if( key == "#HOLIDAY" )
         day = new holiday_data;
      else if( key == "Name" )
         day->set_name( fread_line( stream ) );
      else if( key == "Announce" )
         day->set_announce( fread_line( stream ) );
      else if( key == "Month" )
      {
         int value;
         stream >> value;

         day->set_month( value );
      }
      else if( key == "Day" )
      {
         int value;
         stream >> value;

         day->set_day( value );
      }
      else if( key == "End" )
         daylist.push_back( day );
      else
         bug( "{}: Bad line in holiday file: {}", __func__, key );
   }
   stream.close(  );
}

/* Save the holidays to disk - Samson 5-6-99 */
void save_holidays( void )
{
   std::ofstream stream( std::filesystem::path{HOLIDAY_FILE} );
   if( !stream.is_open() )
   {
      bug( "{}: Cannot open {} for writing: {}", __func__, HOLIDAY_FILE, std::strerror(errno) );
      return;
   }

   for( auto* day : daylist )
   {
      stream << "#HOLIDAY\n";
      stream << std::format( "Name     {}~\n", day->get_name(  ) );
      stream << std::format( "Announce {}~\n", day->get_announce(  ) );
      stream << std::format( "Month    {}\n",  day->get_month(  ) );
      stream << std::format( "Day      {}\n",  day->get_day(  ) );
      stream << "End\n\n";
   }
   stream.close(  );
   if( stream.fail() )
      bug( "{}: Error occurred after closing {}: ", __func__, HOLIDAY_FILE, std::strerror(errno) );
}

/* Holiday OLC command - (c)Andrew Wilkie May-20-2005 */
CMDF( do_setholiday )
{
   std::string arg, arg2;
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
      for( auto* day : daylist )
      {
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

      holiday_data *newday = new holiday_data;
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
   holiday_data *day;
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
      value = std::stoi( argument );
      if( argument.empty(  ) || !is_number( argument ) || value > sysdata->dayspermonth || value < 1 )
      {
         ch->print_fmt( "You must specify a numeric value : {} - {}", 1, sysdata->dayspermonth );
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
      value = std::stoi( argument );
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
            ch->print_fmt( "&[red](&[white]{}&[red])&[yellow]{}\r\n", count, month_name[x] );
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
