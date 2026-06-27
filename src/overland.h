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
 *                         Overland ANSI Map Module                         *
 *                      Created by Samson of Alsherok                       *
 ****************************************************************************/

#pragma once

constexpr int MAX_X = 1000;
constexpr int MAX_Y = 1000;

inline constexpr std::string_view CONT_LIST = "../maps/continent.lst"; // Continent list file.

class mapexit_data
{
 private:
   mapexit_data( const mapexit_data & m );
     mapexit_data & operator=( const mapexit_data & );

 public:
     mapexit_data(  );
    ~mapexit_data(  );

   std::string tomap;      // Target map name if the exit goes to another continent.
   int vnum = 0;           // Target vnum if it goes to a regular zone.
   short herex = 0;        // Coordinates the entrance is at. This should never hold negative values.
   short herey = 0;
   short therex = 0;       // Coordinates the entrance goes to. Value of -1 means it goes to a standard room.
   short therey = 0;
   short prevsector = 0;   // Previous sector type to restore with when an exit is deleted.
};

class landmark_data
{
 private:
   landmark_data( const landmark_data & l );
     landmark_data & operator=( const landmark_data & );

 public:
     landmark_data(  );
    ~landmark_data(  );

   std::string description;  // Description of the landmark.
   int distance = 0;         // Distance the landmark is visible from.
   short map_x = 0;          // X coordinate of landmark.
   short map_y = 0;          // Y coordinate of landmark.
   bool Isdesc = false;      // If true is room desc. If not is landmark.
};

class landing_data
{
 private:
   landing_data( const landing_data & l );
     landing_data & operator=( const landing_data & );

 public:
     landing_data(  );
    ~landing_data(  );

   std::string area;    // Name of the area the landing site serves.
   int cost = 50000;    // How much the skyship operator charges.
   short map_x = 0;     // X/Y coordinates on the overland where this landing site is placed.
   short map_y = 0;
};

class continent_data
{
 private:
   continent_data( const continent_data & c );
     continent_data & operator=( const continent_data & );

 public:
     continent_data(  );
    ~continent_data(  );

   void save( );                                      // Saves continent data after editing.
   void fread_mapexit( std::ifstream & );
   void fread_landmark( std::ifstream & );
   void fread_landing_site( std::ifstream & );
   void free_exits( );
   void free_landmarks( );
   void free_landing_sites( );
   void add_landing_site( short, short );
   void delete_landing_site( landing_data * );
   void delete_landmark( landmark_data * );
   void add_landmark( short, short );
   void load_png_file( );                                // Function to load the map data from the .png file.
   void save_png_file( );                                // Function to write the map data to the .png file.
   void putterr( short, short, short );                  // Populate one coordinate with the proper code value.
   short get_terrain( short, short );                    // Get the sector type for the space the actor is standing on.
   landmark_data *check_landmark( short, short );        // Check to see if the actor is close to a landmark.
   mapexit_data *check_mapexit( short, short );          // Check to see if the actor is on the coordinates of an exit.
   landing_data *check_landing_site( short, short );     // Check to see if the actor is at a landing site.
   void add_mapexit( std::string_view, short, short, short, short, int );
   void modify_mapexit( mapexit_data *, std::string_view, short, short, short, short, int );
   void delete_mapexit( mapexit_data * );
   int floodfill( short, short, short, char );        // Used for large scale terrain changing in OLC.
   int unfloodfill( void );

   class area_data *area = nullptr;               // The area associated with this map. This is set during area load based on the areafile string, in the validate_overland_data function in overland.cpp
   std::list<class mapexit_data *> exits;         // List of exists for this map.
   std::list<class landmark_data *> landmarks;    // List of landmarks for this map.
   std::list<class landing_data *> landing_sites; // List of landing sites for this map.
   std::string name;                              // The name of the continent. Used for lookups and to associate the continent with an area file.
   std::string mapfile;                           // .png file where the map data will be loaded from.
   std::string areafile;                          // The area file associated with this map where all of its mobs, objs, and special rooms will go.
   std::string filename;                          // The map's filename. Used during online editing.
   unsigned char grid[MAX_X][MAX_Y];              // Grid of sector types. Not stored in the continent's data file. This is loaded indirectly through the .png file.
   int vnum = -1;                                 // VNUM for the master room that controls this map.
   bool nogrid = false;                           // If this is set to true, the continent will have no map grid. This allows continents to be treated as planes. For places like the Astral Plane etc.
};

extern const char *sect_types[];
extern const struct sect_color_type sect_show[];

struct sect_color_type
{
   short sector;        // Terrain sector.
   const char *color;   // Color to display as.
   const char *symbol;  // Symbol you see for the sector.
   const char *desc;    // Description of sector type.
   bool canpass;        // Impassable terrain.
   int move;            // Movement loss.
   short graph1;        // Color numbers for graphic conversion.
   short graph2;
   short graph3;
};

extern std::list<continent_data *> continent_list;

continent_data *find_continent_by_name( std::string_view );
continent_data *find_continent_by_room( room_index * );
continent_data *find_continent_by_room_vnum( int );
continent_data *pick_random_continent( void );
ch_ret process_exit( char_data *, short, short, int, bool );
double distance( short, short, short, short );
double calc_angle( short, short, short, short, double * );
bool is_valid_x( short );
bool is_valid_y( short );
bool valid_coordinates( short, short );
