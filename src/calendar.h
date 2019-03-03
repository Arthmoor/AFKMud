/****************************************************************************
 *                   ^     +----- |  / ^     ^ |     | +-\                  *
 *                  / \    |      | /  |\   /| |     | |  \                 *
 *                 /   \   +---   |<   | \ / | |     | |  |                 *
 *                /-----\  |      | \  |  v  | |     | |  /                 *
 *               /       \ |      |  \ |     | +-----+ +-/                  *
 ****************************************************************************
 * AFKMud Copyright 1997-2019 by Roger Libiez (Samson),                     *
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

#ifndef __CALENDAR_H__
#define __CALENDAR_H__

/* Well, ok, so it didn't turn out the way I wanted, but that's life - Samson */
/* Ever write a comment like the one above this one and completely forget what it means? */
/* Portions of this data courtesy of Yrth mud */

/* PaB: Seasons */
/* Notes: Each season will be arbitrarily set at 1/4 of the year.
 */
enum seasons
{
   SEASON_SPRING, SEASON_SUMMER, SEASON_FALL, SEASON_WINTER, SEASON_MAX
};

/* Hunger/Thirst modifiers */
const int WINTER_HUNGER = 1;
const int SUMMER_THIRST = 1;
const int SUMMER_THIRST_DESERT = 2;

/* Holiday chart */
#define HOLIDAY_FILE SYSTEM_DIR "holidays.dat"

extern const char *day_name[];
extern const char *month_name[];
extern const char *season_name[];
extern bool winter_freeze;

class holiday_data
{
 private:
   holiday_data( const holiday_data & n );
     holiday_data & operator=( const holiday_data & );

 public:
     holiday_data(  );
    ~holiday_data(  );

   void set_name( const string & newname )
   {
      name = newname;
   }
   string get_name(  )
   {
      return name;
   }

   void set_announce( const string & text )
   {
      announce = text;
   }
   string get_announce(  )
   {
      return announce;
   }

   void set_month( short mn )
   {
      month = mn;
   }
   short get_month(  )
   {
      return month;
   }

   void set_day( short dy )
   {
      day = dy;
   }
   short get_day(  )
   {
      return day;
   }

 private:
   string name;   /* Name of the holiday */
   string announce;  /* Message to announce the holiday with */
   short month;   /* Month the holiday falls in */
   short day;  /* Day the holiday falls on */
};

void check_holiday( char_data * );
holiday_data *get_holiday( short, short );
#endif
