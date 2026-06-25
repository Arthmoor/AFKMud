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
 *                              Area Class Info                             *
 ****************************************************************************/

#pragma once

/* Area flags - Narn Mar/96 */
/* Don't forget to update build.cpp!!! */
enum area_flags
{
   AFLAG_NOPKILL, AFLAG_NOCAMP, AFLAG_NOASTRAL, AFLAG_NOPORTAL, AFLAG_NORECALL,
   AFLAG_NOSUMMON, AFLAG_NOSCRY, AFLAG_NOTELEPORT, AFLAG_ARENA, AFLAG_NOBEACON,
   AFLAG_NOQUIT, AFLAG_PROTOTYPE, AFLAG_SILENCE, AFLAG_NOMAGIC, AFLAG_HIDDEN,
   AFLAG_NOWHERE, AFLAG_MAX
};

/*
 * Area definition.
 */
class area_data
{
 private:
   area_data( const area_data & a );
     area_data & operator=( const area_data & );

 public:
     area_data(  );
    ~area_data(  );

   void fix_exits(  );
   void sort_name(  );
   void sort_vnums(  );
   void reset(  );
   void fold( const std::string &, bool );
   void wipe_resets(  );

   class continent_data *continent = nullptr; // Continent data structure this area is associated with.
   std::list<room_index *> rooms;         // The list of room indexes for this area.
   std::list<mob_index *> mobs;           // The list of mob indexes for this area.
   std::list<obj_index *> objects;        // The list of object indexes for this area.
   std::bitset<AFLAG_MAX> flags;          // The flags for this area.
   std::chrono::system_clock::time_point creation_date;   // Timestamp for when this area was first created. Samson 1/20/07
   std::chrono::system_clock::time_point install_date;    // Timestamp for when this area was "live" installed. Samson 1/20/07
   std::chrono::system_clock::time_point last_resettime;  // Tracking for when the area was last reset. Debugging tool. Samson 3-6-04
   std::string name;                      // The name of the area.
   std::string filename;                  // The area's filename on disk.
   std::string author;                    // The person who wrote the area. - Scryn
   std::string credits;                   // Additional credits for the area.
   std::string resetmsg;                  // Message shown to players when an area resets.     /* Rennard */
   int low_vnum = 0;                      // First Vnum for this area.
   int hi_vnum = 0;                       // Last Vnum for this area.
   int low_soft_range = 0;                // Recommended minimum level for this area.
   int hi_soft_range = MAX_LEVEL;         // Maximum recommended level for this area.
   int low_hard_range = 0;                // Enforced minimum level for this area. Players who do not meet this cannot enter.
   int hi_hard_range = MAX_LEVEL;         // Enforced maximum level for this area. Players who exceed this cannot enter.
   short weatherx = 0;                    // X Coordinate for this area's weather data.
   short weathery = 0;                    // Y Coordinate for this area's weather data.
   short age = 15;                        // Crude timer that counts up how long it's been since the last reset. This does not save to the .are file.
   short nplayer = 0;                     // Number of players currently in the area. This does not save to the .are file.
   short reset_frequency = 15;            // Time in minutes before this area will reset.
   short map_x = 0;                       // Coordinates of a zone on the overland, for recall/death purposes - Samson 12-25-00
   short map_y = 0;
   unsigned short version = 0;            // Replaces the file_ver method of tracking - Samson 12-23-02
   unsigned short tg_nothing = 20;        // TG Values are for area-specific random treasure chances - Samson 11-25-04
   unsigned short tg_gold = 74;
   unsigned short tg_item = 85;
   unsigned short tg_gem = 93;
   unsigned short tg_scroll = 20;         // These are for specific chances of a particular item type - Samson 11-25-04
   unsigned short tg_potion = 50;
   unsigned short tg_wand = 60;
   unsigned short tg_armor = 75;          // Weapons come after armors and go up to 100%
};

area_data *create_area(  );
void write_area_list(  );
area_data *get_area( std::string_view ); /* FB */
area_data *find_area( std::string_view );
void load_area_file( const std::string &, bool );

extern std::list<area_data *> arealist;
extern std::list<area_data *> area_nsort;
extern std::list<area_data *> area_vsort;
extern int weath_unit;
extern int rand_factor;
extern int climate_factor;
extern int neigh_factor;
extern int max_vector;

void write_to_boot_log( std::string_view );

inline void boot_log( std::string_view fmt, auto&&... args )
{
   std::string buf;

   try
   {
      buf = std::vformat( fmt, std::make_format_args( args... ) );
   }
   catch( const std::exception & e )
   {
      // In case someone bodged a call to this that isn't formatted right.
      bug( "{}: Boot Log formatting error: {}", __func__, e.what() );
      return;
   }

   write_to_boot_log( buf );
}
