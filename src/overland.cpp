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
 *	                       Overland ANSI Map Module                          *
 *                      Created by Samson of Alsherok                       *
 ****************************************************************************/

#include <gd.h>
#include <cmath>
#include <filesystem>
#include <fstream>
#include "mud.h"
#include "area.h"
#include "mobindex.h"
#include "mud_prog.h"
#include "overland.h"
#include "roomindex.h"
#include "ships.h"
#include "weather.h"

std::list<continent_data *> continent_list;

void set_alarm( long );
bool survey_environment( char_data * );
void web_arealist(  );

extern const char *alarm_section;

mapexit_data::mapexit_data(  )
{
}

mapexit_data::~mapexit_data(  )
{
}

landmark_data::landmark_data(  )
{
}

landmark_data::~landmark_data(  )
{
}

void continent_data::free_exits( void )
{
   for( auto it = this->exits.begin(); it != this->exits.end(); )
   {
      mapexit_data *mexit = *it;
      ++it;

      deleteptr( mexit );
   }
}

void continent_data::free_landmarks( void )
{
   for( auto it = this->landmarks.begin(); it != this->landmarks.end(); )
   {
      landmark_data *landmark = *it;
      ++it;

      deleteptr( landmark );
   }
}

void continent_data::free_landing_sites( void )
{
   for( auto it = this->landing_sites.begin(); it != this->landing_sites.end(); )
   {
      landing_data *landing = *it;
      ++it;

      deleteptr( landing );
   }
}

continent_data::continent_data(  )
{
   // Seed all newly created continents with ocean sectors before populating them with actual data.
   for( short x = 0; x < MAX_X; ++x )
   {
      for( short y = 0; y < MAX_Y; ++y )
         putterr( x, y, SECT_OCEAN );
   }
}

continent_data::~continent_data(  )
{
   this->free_exits();
   this->free_landmarks();
   this->free_landing_sites();

   exits.clear();
   landmarks.clear();
   landing_sites.clear();
}

void free_continents( void )
{
   for( auto it = continent_list.begin(); it != continent_list.end(); )
   {
      continent_data *continent = *it;
      ++it;

      deleteptr( continent );
   }
   continent_list.clear();
}

bool pixel_colour( gdImagePtr im, int pixel, short red, short green, short blue )
{
   if( gdImageGreen( im, pixel ) == green && gdImageRed( im, pixel ) == red && gdImageBlue( im, pixel ) == blue )
      return true;
   return false;
}

short get_sector_colour( gdImagePtr im, int pixel )
{
   for( int i = 0; i < SECT_MAX; ++i )
   {
      if( pixel_colour( im, pixel, sect_show[i].graph1, sect_show[i].graph2, sect_show[i].graph3 ) )
         return sect_show[i].sector;
   }
   return SECT_OCEAN;
}

void continent_data::load_png_file( void )
{
   FILE *jpgin;
   gdImagePtr im;

   log_printf( "Loading png file for {}...", this->name );

   std::filesystem::path file_name = std::format( "{}{}", MAP_DIR, this->mapfile );

   if( !( jpgin = fopen( file_name.c_str(), "rb" ) ) )
   {
      bug( "{} -> {}:{}: Missing graphical map file '{}' for continent '{}'!", __func__, __FILE__, __LINE__, this->mapfile, this->name );
      if( fBootDb )
      {
         shutdown_mud( "Missing map file" );
         std::exit( EXIT_FAILURE );
      }
      else
         return;
   }

   im = gdImageCreateFromPng( jpgin );

   for( short y = 0; y < gdImageSY( im ); ++y )
   {
      for( short x = 0; x < gdImageSX( im ); ++x )
      {
         int pixel = gdImageGetPixel( im, x, y );
         short terr = get_sector_colour( im, pixel );
         if( valid_coordinates( x, y ) )
            this->putterr( x, y, terr );
         else
         {
            bug( "{} -> {}:{}: Attempting to add sector data out of bounds for '{}' ({} {}). Graphic file is too large.", __func__, __FILE__, __LINE__, this->name, x, y );
            if( fBootDb )
               std::exit( EXIT_FAILURE );
         }
      }
   }
   fclose( jpgin );
   jpgin = nullptr;
   gdImageDestroy( im );
}

/*
 * As it implies, this saves the map you are currently standing on to disk.
 * Output is in graphic format, making it easily edited with most paint programs.
 * Could also be used as a method for filtering bad color data out of your source
 * image if it had any at loadup. This code should only be called from the mapedit
 * command. Using it in any other way could break something.
 */
// Thanks Davion for this :), PNG format = HUGE time-saver :)
void continent_data::save_png_file( )
{
   gdImagePtr im;
   FILE *PngOut;
   short x, y, terr;
   int image[SECT_MAX];

   im = gdImageCreate( MAX_X, MAX_Y );

   for( x = 0; x < SECT_MAX; ++x )
      image[x] = gdImageColorAllocate( im, sect_show[x].graph1, sect_show[x].graph2, sect_show[x].graph3 );

   for( y = 0; y < MAX_Y; ++y )
   {
      for( x = 0; x < MAX_X; ++x )
      {
         terr = this->get_terrain( x, y );
         if( terr == -1 )
            terr = SECT_OCEAN;

         gdImageLine( im, x, y, x, y, image[terr] );
      }
   }

   std::filesystem::path graphicname = std::format( "{}{}", MAP_DIR, this->mapfile );

   if( ( PngOut = fopen( graphicname.c_str(), "wb" ) ) == nullptr )
   {
      bug( "{} -> {}:{}: Unable to open {} for writing.", __func__, __FILE__, __LINE__, graphicname.string() );
      return;
   }

   /*
    * Output the same image in PNG format. 
    */
   gdImagePng( im, PngOut );

   /*
    * Close the file. 
    */
   fclose( PngOut );
   PngOut = nullptr;

   /*
    * Destroy the image in memory. 
    */
   gdImageDestroy( im );
}

void write_continent_list( void )
{
   std::ofstream stream( std::filesystem::path{CONT_LIST} );
   if( !stream.is_open(  ) )
   {
      bug( "{}: Cannot open {} for writing: {}", __func__, CONT_LIST, std::strerror(errno) );
      return;
   }

   for( auto* continent : continent_list )
      stream << std::format( "{}\n", continent->filename );

   stream << "$\n";
   stream.close();
   if( stream.fail() )
      bug( "{}: Error occurred after closing {}: ", __func__, CONT_LIST, std::strerror(errno) );
}

void continent_data::fread_landing_site( std::ifstream & stream )
{
   landing_data *landing_site = new landing_data;

   std::string key;
   while( stream >> key )
   {
      if( key == "Coordinates" )
      {
         std::string ln;
         std::getline( stream, ln );
         std::istringstream( ln ) >> landing_site->map_x >> landing_site->map_y;
      }
      else if( key == "Area" )
         landing_site->area = fread_line( stream, '\n' );
      else if( key == "Cost" )
         stream >> landing_site->cost;
      else if( key == "End" )
      {
         this->landing_sites.push_back( landing_site );
         return;
      }
      else
      {
         bug( "{}: {} - Bad line reading landing sites: {}", __func__, this->name, key );
         fread_to_eol( stream );
      }
   }

   bug( "{}: Filestream reached premature EOF reading landing sites for continent {} - FATAL ERROR: Aborting file read.", __func__, this->name );
   shutdown_mud( "Corrupt continent file." );
   std::exit( EXIT_FAILURE );
}

void continent_data::fread_landmark( std::ifstream & stream )
{
   landmark_data *landmark = new landmark_data;

   std::string key;
   while( stream >> key )
   {
      if( key == "Coordinates" )
      {
         std::string ln;
         std::getline( stream, ln );
         std::istringstream( ln ) >> landmark->map_x >> landmark->map_y >> landmark->distance;
      }
      else if( key == "Description" )
         landmark->description = fread_line( stream, '\n' );
      else if( key == "Isdesc" )
         stream >> landmark->Isdesc;
      else if( key == "End" )
      {
         this->landmarks.push_back( landmark );
         return;
      }
      else
      {
         bug( "{}: {} - Bad line reading landmarks: {}", __func__, this->name, key );
         fread_to_eol( stream );
      }
   }

   bug( "{} -> {}:{}: Filestream reached premature EOF reading landmarks for continent {} - FATAL ERROR: Aborting file read.", __func__,  __FILE__, __LINE__, this->name );
   shutdown_mud( "Corrupt continent file." );
   std::exit( EXIT_FAILURE );
}

void continent_data::fread_mapexit( std::ifstream & stream )
{
   mapexit_data *mexit = new mapexit_data;

   std::string key, ln;
   while( stream >> key )
   {
      if( key == "ToMap" )
         stream >> mexit->tomap;
      else if( key == "Here" )
      {
         std::getline( stream, ln );
         std::istringstream( ln ) >> mexit->herex >> mexit->herey;
      }
      else if( key == "There" )
      {
         std::getline( stream, ln );
         std::istringstream( ln ) >> mexit->therex >> mexit->therey;
      }
      else if( key == "Vnum" )
         stream >> mexit->vnum;
      else if( key == "Prevsector" )
         stream >> mexit->prevsector;
      else if( key == "End" )
      {
         this->exits.push_back( mexit );
         return;
      }
      else
      {
         bug( "{}: {} - Bad line reading map exits: {}", __func__, this->name, key );
         fread_to_eol( stream );
      }
   }

   bug( "{} -> {}:{}: Filestream reached premature EOF reading map exits for continent {} - FATAL ERROR: Aborting file read.", __func__, __FILE__, __LINE__, this->name );
   shutdown_mud( "Corrupt continent file." );
   std::exit( EXIT_FAILURE );
}

void load_continent( std::string_view continent_file )
{
   std::filesystem::path filename = std::format( "{}{}", MAP_DIR, continent_file );
   std::ifstream stream( filename );
   if( !stream.is_open() )
   {
      bug( "{}: Cannot open {} for reading: {}", __func__, filename.string(), std::strerror(errno) );
      return;
   }

   int file_ver = 0;
   continent_data *continent = nullptr;
   std::string key;
   while( stream >> key )
   {
      if( key == "#CONTINENT" )
         continent = new continent_data;
      else if( key == "Version" )
         stream >> file_ver;
      else if( key == "Name" )
         continent->name = fread_line( stream, '\n' );
      else if( key == "Mapfile" )
         continent->mapfile = fread_line( stream, '\n' );
      else if( key == "Areafile" )
         continent->areafile = fread_line( stream, '\n' );
      else if( key == "NoGrid" )
         stream >> continent->nogrid;
      else if( key == "Vnum" )
         stream >> continent->vnum;
      else if( key == "#ENTRANCE" )
         continent->fread_mapexit( stream );
      else if( key == "#LANDMARK" )
         continent->fread_landmark( stream );
      else if( key == "#LANDING_SITE" )
         continent->fread_landing_site( stream );
      else if( key == "#END" )
      {
         continent->filename = continent_file;

         if( file_ver == 0 )
            ; // Shut the hell up GCC!

         // Only loads a png file if the continent should have one. Otherwise the game is only tracking it as a plane instead of an overland map.
         if( !continent->nogrid )
            continent->load_png_file( );

         continent_list.push_back( continent );
      }
      else
      {
         log_printf( "{}: Bad line in continent file: {}", __func__, key );
         fread_to_eol( stream );
      }
   }
   stream.close(  );
}

void load_continents( const int AREA_FILE_ALARM )
{
   std::ifstream stream( std::filesystem::path{CONT_LIST} );
   if( !stream.is_open(  ) )
   {
      bug( "{}: Cannot open {} for reading: {}", __func__, CONT_LIST, std::strerror(errno) );
      shutdown_mud( "Boot_db: Unable to open continent list." );
      std::exit( EXIT_FAILURE );
   }

   continent_list.clear( );

   for( ;; )
   {
      std::string filename = fread_line( stream, '\n' );

      if( filename.empty() || filename[0] == '$' )
         break;

      set_alarm( AREA_FILE_ALARM );
      alarm_section = "boot_db: read continent files";
      load_continent( filename );
      set_alarm( 0 );
   }
   stream.close();
}

// This is called in db.cpp during startup after the area files have been loaded.
void validate_overland_data( void )
{
   int error_count = 0;

   for( auto* continent : continent_list )
   {
      if( continent->nogrid == false )
      {
         if( !get_room_index( continent->vnum ) )
         {
            bug( "{} -> {}:{}: Continent {} had an invalid room vnum assigned: {}", __func__, __FILE__, __LINE__, continent->name, continent->vnum );
            error_count++;
         }

         area_data *area = find_area( continent->areafile );

         if( !area )
         {
            bug( "{} -> {}:{}: Continent {} had an invalid area filename assigned: {}", __func__, __FILE__, __LINE__, continent->name, continent->areafile );
            error_count++;
         }
         else
            continent->area = area;
      }
   }

   if( error_count == 0 )
      log_string( "Overland map data validated with no errors." );
   else
   {
      log_string( "Overland map data has invalid settings that need to be corrected. See the above bug messages." );
      std::exit( EXIT_FAILURE );
   }
}

// 1: Initial version.
constexpr int CONTINENT_FILE_VERSION = 1;

void continent_data::save( )
{
   log_printf_plus( LOG_BUILD, LEVEL_GREATER, "Saving continent data for {}...", this->filename );

   std::filesystem::path fname = std::format( "{}{}", MAP_DIR, this->filename );
   std::ofstream stream( fname );
   if( !stream.is_open(  ) )
   {
      bug( "{}: Cannot open {} for writing: {}", __func__, fname.string(), std::strerror(errno) );
      return;
   }

   stream << "#CONTINENT\n";
   stream << std::format( "Version   {}\n", CONTINENT_FILE_VERSION );
   stream << std::format( "Name      {}\n", this->name );

   if( this->nogrid == true )
      stream << std::format( "NoGrid    {}\n", this->nogrid );
   else
   {
      stream << std::format( "Vnum      {}\n", this->vnum );
      stream << std::format( "Mapfile   {}\n", this->mapfile );
   }

   stream << std::format( "Areafile  {}\n\n", this->areafile );

   if( this->nogrid == false )
   {
      for( auto* mexit : this->exits )
      {
         stream << "#ENTRANCE\n";
         stream << std::format( "ToMap      {}\n", mexit->tomap );
         stream << std::format( "Vnum       {}\n", mexit->vnum );
         stream << std::format( "Here       {} {}\n", mexit->herex, mexit->herey );
         stream << std::format( "There      {} {}\n", mexit->therex, mexit->therey );
         stream << std::format( "Prevsector {}\n", mexit->prevsector );
         stream << "End\n\n";
      }

      for( auto* landmark : this->landmarks )
      {
         stream << "#LANDMARK\n";
         stream << std::format( "Coordinates {} {} {}\n", landmark->map_x, landmark->map_y, landmark->distance );
         stream << std::format( "Description {}\n", landmark->description );
         stream << std::format( "Isdesc      {}\n", landmark->Isdesc );
         stream << "End\n\n";
      }

      for( auto* landing : this->landing_sites )
      {
         stream << "#LANDING_SITE\n";
         stream << std::format( "Coordinates {} {}\n", landing->map_x, landing->map_y );
         stream << std::format( "Area        {}\n", landing->area );
         stream << std::format( "Cost        {}\n", landing->cost );
         stream << "End\n\n";
      }      
   }

   stream << "#END\n";
   stream.close();
   if( stream.fail() )
   {
      bug( "{}: Error occurred after closing {}: ", __func__, fname.string(), std::strerror(errno) );
      return;
   }
   log_printf_plus( LOG_BUILD, LEVEL_GREATER, "Data for {} saved.", this->filename );
}

/*
 * The names of various sector types. Used in the OLC code in addition to
 * the display_map function and probably some other places I've long
 * since forgotten by now.
 */
const char *sect_types[] = {
   "indoors", "city", "field", "forest", "hills", "mountain", "water_swim",
   "water_noswim", "air", "underwater", "desert", "river", "oceanfloor",
   "underground", "jungle", "swamp", "tundra", "ice", "ocean", "lava",
   "shore", "tree", "stone", "quicksand", "wall", "glacier", "exit",
   "trail", "blands", "grassland", "scrub", "barren", "bridge", "road",
   "landing", "\n"
};

/* Note - this message array is used to broadcast both the in sector messages,
 * as well as the messages sent to PCs when they can't move into the sector.
 */
const char *impass_message[SECT_MAX] = {
   "You must locate the proper entrance to go in there.",
   "You are travelling along a smooth stretch of road.",
   "Rich farmland stretches out before you.",
   "Thick forest vegetation covers the ground all around.",
   "Gentle rolling hills stretch out all around.",
   "The rugged terrain of the mountains makes movement slow.",
   "The waters lap at your feet.",
   "The deep waters lap at your feet.",
   "Air", "Underwater",
   "The hot, dry desert sands seem to go on forever.",
   "The river churns and burbles beneath you.",
   "Oceanfloor", "Underground",
   "The jungle is extremely thick and humid.",
   "The swamps seem to surround everything.",
   "The frozen wastes seem to stretch on forever.",
   "The ice barely provides a stable footing.",
   "The rough seas would rip any boat to pieces!",
   "That's lava! You'd be burnt to a crisp!!!",
   "The soft sand makes for difficult walking.",
   "The forest becomes too thick to pass through that direction.",
   "The mountains are far too steep to keep going that way.",
   "That's quicksand! You'd be dragged under!",
   "The walls are far too high to scale.",
   "The glacier ahead is far too vast to safely cross.",
   "An exit to somewhere new.....",
   "You are walking along a dusty trail.",
   "All around you the land has been scorched to ashes.",
   "Tall grass ripples in the wind.",
   "Scrub land stretches out as far as the eye can see.",
   "The land around you is dry and barren.",
   "A sturdy span of bridge passes over the water.",
   "You are travelling along a smooth stretch of road.",
   "The area here has been smoothed over and designated for skyship landings."
};

/*
 * The symbol table. 
 * First entry is the sector type. 
 * Second entry is the color used to display it.
 * Third entry is the symbol used to represent it.
 * Fourth entry is the description of the terrain.
 * Fifth entry determines weather or not the terrain can be walked on.
 * Sixth entry is the amount of movement used up travelling on the terrain.
 * Last 3 entries are the RGB values for each type of terrain used to generate the graphic files.
 */
const struct sect_color_type sect_show[] = {
/* Sector Type          Color	   Symbol   Description       Passable?   Move  R     G     B	*/

   {SECT_INDOORS,       "&x",    " ",     "indoors",        false,      1,    0,    0,    0},
   {SECT_CITY,          "&Y",    ":",     "city",           true,       1,    255,  128,  64},
   {SECT_FIELD,         "&G",    "+",     "field",          true,       1,    141,  215,  1},
   {SECT_FOREST,        "&g",    "+",     "forest",         true,       2,    0,    108,  47},
   {SECT_HILLS,         "&O",    "^",     "hills",          true,       3,    140,  102,  54},
   {SECT_MOUNTAIN,      "&w",    "^",     "mountain",       true,       5,    152,  152,  152},
   {SECT_WATER_SWIM,    "&C",    "~",     "shallow water",  true,       2,    89,   242,  251},
   {SECT_WATER_NOSWIM,  "&B",    "~",     "deep water",     true,       2,    67,   114,  251},
   {SECT_AIR,           "&x",    "?",     "air",            false,      1,    1,    1,    1},
   {SECT_UNDERWATER,    "&x",    "?",     "underwater",     false,      5,    2,    2,    2},
   {SECT_DESERT,        "&Y",    "~",     "desert",         true,       3,    241,  228,  145},
   {SECT_RIVER,         "&B",    "~",     "river",          true,       3,    0,    0,    255},
   {SECT_OCEANFLOOR,    "&x",    "?",     "ocean floor",    false,      4,    3,    3,    3},
   {SECT_UNDERGROUND,   "&x",    "?",     "underground",    false,      3,    4,    4,    4},
   {SECT_JUNGLE,        "&g",    "*",     "jungle",         true,       2,    70,   149,  52},
   {SECT_SWAMP,         "&g",    "~",     "swamp",          true,       3,    218,  176,  56},
   {SECT_TUNDRA,        "&C",    "-",     "tundra",         true,       2,    54,   255,  255},
   {SECT_ICE,           "&W",    "=",     "ice",            true,       3,    133,  177,  252},
   {SECT_OCEAN,         "&b",    "~",     "ocean",          false,      1,    0,    0,    128},
   {SECT_LAVA,          "&R",    ":",     "lava",           false,      2,    245,  37,   29},
   {SECT_SHORE,         "&Y",    ".",     "shoreline",      true,       3,    255,  255,  0},
   {SECT_TREE,          "&g",    "^",     "impass forest",  false,      10,   0,    64,   0},
   {SECT_STONE,         "&W",    "^",     "impass mountain", false,     10,   128,  128,  128},
   {SECT_QUICKSAND,     "&g",    "%",     "quicksand",      false,      10,   128,  128,  0},
   {SECT_WALL,          "&P",    "I",     "wall",           false,      10,   255,  0,    255},
   {SECT_GLACIER,       "&W",    "=",     "glacier",        false,      10,   141,  207,  244},
   {SECT_EXIT,          "&W",    "#",     "exit",           true,       1,    255,  255,  255},
   {SECT_TRAIL,         "&O",    ":",     "trail",          true,       1,    128,  64,   0},
   {SECT_BLANDS,        "&r",    ".",     "blasted lands",  true,       2,    128,  0,    0},
   {SECT_GRASSLAND,     "&G",    ".",     "grassland",      true,       1,    83,   202,  2},
   {SECT_SCRUB,         "&g",    ".",     "scrub",          true,       2,    123,  197,  112},
   {SECT_BARREN,        "&O",    ".",     "barren",         true,       2,    192,  192,  192},
   {SECT_BRIDGE,        "&P",    ":",     "bridge",         true,       1,    255,  0,    128},
   {SECT_ROAD,          "&Y",    ":",     "road",           true,       1,    215,  107,  0},
   {SECT_LANDING,       "&R",    "#",     "landing",        true,       1,    255,  0,    0}
};

/* The distance messages for the survey command */
const char *landmark_distances[] = {
   "hundreds of miles away in the distance",
   "far off in the skyline",
   "many miles away at great distance",
   "far off many miles away",
   "tens of miles away in the distance",
   "far off in the distance",
   "several miles away",
   "off in the distance",
   "not far from here",
   "in the near vicinity",
   "in the immediate area"
};

/*
 * The array of predefined mobs that the check_random_mobs function uses.
 * Thanks to Geni for supplying this method :)
 */
int const random_mobs[SECT_MAX][25] = {
   /*
    * Mobs for SECT_INDOORS 
    */
   {-1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1},
   /*
    * Mobs for SECT_CITY 
    */
   {-1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1,
    MOB_VNUM_MAP_TRAVELER, MOB_VNUM_MAP_MERCHANT, MOB_VNUM_MAP_GYPSY, MOB_VNUM_MAP_BANDIT, -1,
    -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1},
   /*
    * Mobs for SECT_FIELD 
    */
   {-1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1,
    MOB_VNUM_MAP_FARMER, MOB_VNUM_MAP_COW, MOB_VNUM_MAP_RABBIT, MOB_VNUM_MAP_BULL, -1,
    -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1},
   /*
    * Mobs for SECT_FOREST 
    */
   {-1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1,
    MOB_VNUM_MAP_DEER, MOB_VNUM_MAP_DRYAD, MOB_VNUM_MAP_TREANT, MOB_VNUM_MAP_WURM, -1,
    -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1},
   /*
    * Mobs for SECT_HILLS 
    */
   {-1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1,
    MOB_VNUM_MAP_DWARF, MOB_VNUM_MAP_BADGER, MOB_VNUM_MAP_CROW, MOB_VNUM_MAP_DRAGON, -1,
    -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1},
   /*
    * Mobs for SECT_MOUNTAIN 
    */
   {-1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1,
    MOB_VNUM_MAP_GOAT, MOB_VNUM_MAP_HOUND, MOB_VNUM_MAP_FIRBOLG, -1, -1,
    -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1},
   /*
    * Mobs for SECT_WATER_SWIM 
    */
   {-1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1},
   /*
    * Mobs for SECT_WATER_NOSWIM 
    */
   {-1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1},
   /*
    * Mobs for SECT_AIR 
    */
   {-1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1},
   /*
    * Mobs for SECT_UNDERWATER 
    */
   {-1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1},
   /*
    * Mobs for SECT_DESERT 
    */
   {-1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1,
    MOB_VNUM_MAP_BEETLE, MOB_VNUM_MAP_NOMAD, MOB_VNUM_MAP_ELEMENTAL, -1, -1,
    -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1},
   /*
    * Mobs for SECT_RIVER 
    */
   {-1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1},
   /*
    * Mobs for SECT_OCEANFLOOR 
    */
   {-1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1},
   /*
    * Mobs for SECT_UNDERGROUND 
    */
   {-1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1},
   /*
    * Mobs for SECT_JUNGLE 
    */
   {-1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1,
    MOB_VNUM_MAP_ORANGUTAN, MOB_VNUM_MAP_PYTHON, MOB_VNUM_MAP_LIZARD, MOB_VNUM_MAP_PANTHER, -1,
    -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1},
   /*
    * Mobs for SECT_SWAMP 
    */
   {-1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1,
    MOB_VNUM_MAP_STIRGE, MOB_VNUM_MAP_GOBLIN, -1, -1, -1,
    -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1},
   /*
    * Mobs for SECT_TUNDRA 
    */
   {-1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1},
   /*
    * Mobs for SECT_ICE 
    */
   {-1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1},
   /*
    * Mobs for SECT_OCEAN 
    */
   {-1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1},
   /*
    * Mobs for SECT_LAVA 
    */
   {-1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1},
   /*
    * Mobs for SECT_SHORE 
    */
   {-1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1,
    MOB_VNUM_MAP_CRAB, MOB_VNUM_MAP_SEAGULL, -1, -1, -1,
    -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1},
   /*
    * Mobs for SECT_TREE 
    */
   {-1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1},
   /*
    * Mobs for SECT_STONE 
    */
   {-1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1},
   /*
    * Mobs for SECT_QUICKSAND 
    */
   {-1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1},
   /*
    * Mobs for SECT_WALL 
    */
   {-1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1},
   /*
    * Mobs for SECT_GLACIER 
    */
   {-1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1},
   /*
    * Mobs for SECT_EXIT 
    */
   {-1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1},
   /*
    * Mobs for SECT_TRAIL 
    */
   {-1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1},
   /*
    * Mobs for SECT_BLANDS 
    */
   {-1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1},
   /*
    * Mobs for SECT_GRASSLAND 
    */
   {-1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1},
   /*
    * Mobs for SECT_SCRUB 
    */
   {-1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1,
    MOB_VNUM_MAP_HYENA, MOB_VNUM_MAP_MEERKAT, MOB_VNUM_MAP_ARMADILLO, MOB_VNUM_MAP_MANTICORE, -1,
    -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1},
   /*
    * Mobs for SECT_BARREN 
    */
   {-1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1},
   /*
    * Mobs for SECT_BRIDGE 
    */
   {-1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1},
   /*
    * Mobs for SECT_ROAD 
    */
   {-1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1},
   /*
    * Mobs for SECT_LANDING 
    */
   {-1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1}
};

// Pick a continent at random. For whatever silly reasons one might want to do that.
continent_data *pick_random_continent( void )
{
   std::map<int, continent_data *> choices;
   int num_conts = 0, pick;

   for( auto* continent : continent_list )
   {
      choices[num_conts] = continent;
      num_conts++;
   }

   if( num_conts == 0 )
      return nullptr;

   pick = number_range( 0, num_conts );

   return choices[pick];
}

// Find an existing continent by its name. Used during boot, and to find targets for exits.
continent_data *find_continent_by_name( std::string_view name )
{

   for( auto* continent : continent_list )
   {
      if( !str_cmp( name, continent->name ) )
         return continent;
   }

   return nullptr;
}

// Used in OLC when creating a map and associating a png file with it.
continent_data *find_continent_by_pngfile( std::string_view name )
{
   for( auto* continent : continent_list )
   {
      if( !str_cmp( name, continent->mapfile ) )
         return continent;
   }

   return nullptr;
}

// Find a map based on what continent a room is part of, via it's area.
continent_data *find_continent_by_room( const room_index * room )
{
   std::list<area_data *>::iterator a;
   std::list<continent_data *>::iterator c;
   const area_data *area;
   bool foundarea = false;

   for( a = arealist.begin(  ); a != arealist.end(  ); ++a )
   {
      area = *a;

      if( room->area == area )
      {
         foundarea = true;
         break;
      }
   }

   if( !foundarea )
      return nullptr;

   for( c = continent_list.begin(  ); c != continent_list.end(  ); ++c )
   {
      continent_data *continent = *c;

      if( area->continent == continent )
         return continent;
   }

   return nullptr;
}

// Finds the map the player is on by its room vnum. The room vnum is specified in the continent file.
continent_data *find_continent_by_room_vnum( int rvnum )
{
   for( auto* continent : continent_list )
   {
      if( continent->vnum == rvnum )
         return continent;
   }

   return nullptr;
}

/* Simply changes the sector type for the specified coordinates */
void continent_data::putterr( short x, short y, short terr )
{
   if( valid_coordinates( x, y ) )
      this->grid[x][y] = terr;
   else
      bug( "{} -> {}:{}: Attempting to edit sector out of bounds for '{}' ({} {})", __func__, __FILE__, __LINE__, this->name, x, y );
}

/*
 * Alrighty - this checks where the PC is currently standing to see what kind of terrain the space is.
 * Returns -1 if something is not kosher with where they're standing.
 * Called from several places below, so leave this right here.
 */
short continent_data::get_terrain( short x, short y )
{
   if( !valid_coordinates( x, y ) )
      return -1;

   return this->grid[x][y];
}

CMDF( do_continents )
{
   ch->pager( "Continent      | #Exits | #Landmarks | #Landing Sites\r\n" );
   ch->pager( "-----------------------------------------------------\r\n" );

   for( auto* continent : continent_list )
      ch->pager_fmt( "{:<15}  {:<4}     {:<4}         {:<4}\r\n", continent->name, continent->exits.size(), continent->landmarks.size(), continent->landing_sites.size() );
}

/*
 * Used with the survey command to calculate distance to the landmark.
 * Used by do_scan to see if the target is close enough to justify showing it.
 * Used by the display_map code to create a more "circular" display.
 *
 * Other possible uses: 
 * Summon spell - restrict the distance someone can summon another person from.
 */
double distance( short chX, short chY, short lmX, short lmY )
{
   double xchange, ychange;
   double zdistance;

   xchange = ( chX - lmX );
   xchange *= xchange;
   /*
    * To make the display more circular - Matarael 
    */
   xchange *= ( 5.120000 / 10.780000 );   /* The font ratio. */
   ychange = ( chY - lmY );
   ychange *= ychange;

   zdistance = sqrt( ( xchange + ychange ) );
   return ( zdistance );
}

/* Used by the survey command to determine the directional message to send */
double calc_angle( short chX, short chY, short lmX, short lmY, double *ipDistan )
{
   int iNx1 = 0, iNy1 = 0, iNx2, iNy2, iNx3, iNy3;
   double dDist1, dDist2;
   double dTandeg, dDeg, iFinal;
   iNx2 = lmX - chX;
   iNy2 = lmY - chY;
   iNx3 = 0;
   iNy3 = iNy2;

   *ipDistan = distance( iNx1, iNy1, iNx2, iNy2 );

   if( iNx2 == 0 && iNy2 == 0 )
      return ( -1 );
   if( iNx2 == 0 && iNy2 > 0 )
      return ( 180 );
   if( iNx2 == 0 && iNy2 < 0 )
      return ( 0 );
   if( iNy2 == 0 && iNx2 > 0 )
      return ( 90 );
   if( iNy2 == 0 && iNx2 < 0 )
      return ( 270 );

   /*
    * ADJACENT 
    */
   dDist1 = distance( iNx1, iNy1, iNx3, iNy3 );

   /*
    * OPPOSSITE 
    */
   dDist2 = distance( iNx3, iNy3, iNx2, iNy2 );

   dTandeg = dDist2 / dDist1;
   dDeg = atan( dTandeg );

   iFinal = ( dDeg * 180 ) / 3.14159265358979323846;  /* Pi for the math impared :P */

   if( iNx2 > 0 && iNy2 > 0 )
      return ( ( 90 + ( 90 - iFinal ) ) );

   if( iNx2 > 0 && iNy2 < 0 )
      return ( iFinal );

   if( iNx2 < 0 && iNy2 > 0 )
      return ( ( 180 + iFinal ) );

   if( iNx2 < 0 && iNy2 < 0 )
      return ( ( 270 + ( 90 - iFinal ) ) );

   return ( -1 );
}

/* Will return true or false if ch and victim are in the same map room
 * Used in a LOT of places for checks
 */
bool is_same_char_map( const char_data * ch, const char_data * victim )
{
   if( victim->continent != ch->continent || victim->map_x != ch->map_x || victim->map_y != ch->map_y )
      return false;
   return true;
}

bool is_same_obj_map( char_data * ch, obj_data * obj )
{
   // If it's being carried, treat it as a match.
   if( obj->carried_by )
      return true;

   // Similarly, if it's in another object, treat it as a match.
   if( obj->in_obj )
      return true;

   if( !obj->extra_flags.test( ITEM_ONMAP ) )
   {
      if( ch->has_pcflag( PCFLAG_ONMAP ) )
         return false;
      if( ch->has_actflag( ACT_ONMAP ) )
         return false;
      return true;
   }

   if( ch->continent != obj->continent || ch->map_x != obj->map_x || ch->map_y != obj->map_y )
      return false;
   return true;
}

bool is_valid_x( short x )
{
   if( x < 0 || x >= MAX_X )
      return false;
   return true;
}

bool is_valid_y( short y )
{
   if( y < 0 || y >= MAX_Y )
      return false;
   return true;
}

// Routine to check if the X and Y values from input are valid.
bool valid_coordinates( short x, short y )
{
   if( is_valid_x( x ) && is_valid_y( y ) )
      return true;

   if( !is_valid_x( x ) )
      return false;
   if( !is_valid_y( y ) )
      return false;
 
   // No idea how you'd ever get here, but ok.
   return true;
}

/*
 * Will set the vics map the same as the characters map
 * I got tired of coding the same stuff... over.. and over...
 *
 * Used in summon, gate, goto
 *
 * Fraktyl
 */
/* Sets victim to whatever conditions ch is under */
void fix_maps( char_data * ch, char_data * victim )
{
   /*
    * Null ch is an acceptable condition, don't do anything. 
    */
   if( !ch )
      return;

   /*
    * This would be bad though, bug out. 
    */
   if( !victim )
   {
      bug( "{} -> {}:{}: nullptr victim!", __func__, __FILE__, __LINE__ );
      return;
   }

   /*
    * Fix Act/Plr flags first 
    */
   if( ch->has_pcflag( PCFLAG_ONMAP ) || ch->has_actflag( ACT_ONMAP ) )
   {
      if( victim->isnpc(  ) )
         victim->set_actflag( ACT_ONMAP );
      else
         victim->set_pcflag( PCFLAG_ONMAP );
   }
   else
   {
      if( victim->isnpc(  ) )
         victim->unset_actflag( ACT_ONMAP );
      else
         victim->unset_pcflag( PCFLAG_ONMAP );
   }

   /*
    * Either way, the map will be the same 
    */
   victim->continent = ch->continent;
   victim->map_x = ch->map_x;
   victim->map_y = ch->map_y;
}

/* Overland landmark stuff starts here */
landmark_data *continent_data::check_landmark( short x, short y )
{
   for( auto* landmark : this->landmarks )
   {
      if( landmark->map_x == x && landmark->map_y == y )
         return landmark;
   }
   return nullptr;
}

void continent_data::add_landmark( short x, short y )
{
   landmark_data *landmark;

   landmark = new landmark_data;
   landmark->map_x = x;
   landmark->map_y = y;
   this->landmarks.push_back( landmark );
}

void continent_data::delete_landmark( landmark_data * landmark )
{
   if( !landmark )
   {
      bug( "{} -> {}:{}: Trying to delete nullptr landmark!", __func__, __FILE__, __LINE__ );
      return;
   }

   this->landmarks.remove( landmark );
   deleteptr( landmark );
}

/* Landmark survey module - idea snarfed from Medievia and adapted to Smaug by Samson - 8-19-00 */
CMDF( do_survey )
{
   double dist, angle;
   int dir = -1, iMes = 0;
   bool found = false, env = false;

   if( !ch )
      return;

   if( !ch->continent )
   {
      ch->print( "You cannot survey from a regular area.\r\n" );
      return;
   }

   for( auto* landmark : ch->continent->landmarks )
   {
      if( landmark->Isdesc )
         continue;

      dist = distance( ch->map_x, ch->map_y, landmark->map_x, landmark->map_y );

      /*
       * Save the math if it's too far away anyway 
       */
      if( dist <= landmark->distance )
      {
         found = true;

         angle = calc_angle( ch->map_x, ch->map_y, landmark->map_x, landmark->map_y, &dist );

         if( angle == -1 )
            dir = -1;
         else if( angle >= 360 )
            dir = DIR_NORTH;
         else if( angle >= 315 )
            dir = DIR_NORTHWEST;
         else if( angle >= 270 )
            dir = DIR_WEST;
         else if( angle >= 225 )
            dir = DIR_SOUTHWEST;
         else if( angle >= 180 )
            dir = DIR_SOUTH;
         else if( angle >= 135 )
            dir = DIR_SOUTHEAST;
         else if( angle >= 90 )
            dir = DIR_EAST;
         else if( angle >= 45 )
            dir = DIR_NORTHEAST;
         else if( angle >= 0 )
            dir = DIR_NORTH;

         if( dist > 200 )
            iMes = 0;
         else if( dist > 150 )
            iMes = 1;
         else if( dist > 100 )
            iMes = 2;
         else if( dist > 75 )
            iMes = 3;
         else if( dist > 50 )
            iMes = 4;
         else if( dist > 25 )
            iMes = 5;
         else if( dist > 15 )
            iMes = 6;
         else if( dist > 10 )
            iMes = 7;
         else if( dist > 5 )
            iMes = 8;
         else if( dist > 1 )
            iMes = 9;
         else
            iMes = 10;

         if( dir == -1 )
            ch->print_fmt( "Right here nearby, {}.\r\n", !landmark->description.empty(  ) ? landmark->description : "BUG! Please report!" );
         else
            ch->print_fmt( "To the {}, {}, {}.\r\n", dir_name[dir], landmark_distances[iMes],
                        !landmark->description.empty(  ) ? landmark->description : "<BUG! Inform the Immortals>" );

         if( ch->is_immortal(  ) )
         {
            ch->print_fmt( "Distance to landmark: {}\r\n", dist );
            ch->print_fmt( "Landmark coordinates: {}X {}Y\r\n", landmark->map_x, landmark->map_y );
         }
      }
   }
   env = survey_environment( ch );

   if( !found && !env )
      ch->print( "Your survey of the area yields nothing special.\r\n" );
}

/* Support command to list all landmarks currently loaded */
CMDF( do_landmarks )
{
   std::list<continent_data *>::iterator cont;
   std::list<landmark_data *>::iterator imark;

   ch->pager( "Continent | Coordinates | Distance | Description\r\n" );
   ch->pager( "-----------------------------------------------------------\r\n" );

   for( cont = continent_list.begin(  ); cont != continent_list.end(  ); ++cont )
   {
      continent_data *continent = *cont;

      if( continent->landmarks.empty(  ) && continent->nogrid == false )
      {
         ch->pager_fmt( "{}: No landmarks defined.\r\n", continent->name );
         continue;
      }

      for( imark = continent->landmarks.begin(  ); imark != continent->landmarks.end(  ); ++imark )
      {
         landmark_data *landmark = *imark;

         ch->pager_fmt( "{:<10}  {:<4}X {:<4}Y   {:<4}       {}\r\n", continent->name, landmark->map_x, landmark->map_y, landmark->distance, landmark->description );
      }
   }
}

/* OLC command to add/delete/edit landmark information */
CMDF( do_setmark )
{
   landmark_data *landmark = nullptr;
   std::string arg;

#ifdef MULTIPORT
   if( mud_port == MAINPORT )
   {
      ch->print( "This command is not available on this port.\r\n" );
      return;
   }
#endif

   if( ch->isnpc(  ) )
   {
      ch->print( "Sorry, NPCs can't edit the overland maps.\r\n" );
      return;
   }

   if( !ch->desc )
   {
      ch->print( "You have no descriptor.\r\n" );
      return;
   }

   switch( ch->substate )
   {
      default:
         break;

      case SUB_RESTRICTED:
         ch->print( "You cannot do this while in another command.\r\n" );
         return;

      case SUB_OVERLAND_DESC:
         landmark = ( landmark_data * ) ch->pcdata->dest_buf;
         if( !landmark )
            bug( "{} -> {}:{}: setmark desc: sub_overland_desc: nullptr ch->pcdata->dest_buf", __func__, __FILE__, __LINE__ );
         landmark->description = ch->copy_buffer( );
         ch->stop_editing(  );
         ch->substate = ch->tempnum;
         ch->continent->save(  );
         ch->print( "Description set.\r\n" );
         return;

      case SUB_EDIT_ABORT:
         ch->substate = SUB_NONE;
         ch->print( "Aborting description.\r\n" );
         return;
   }

   argument = one_argument( argument, arg );

   if( arg.empty(  ) || !str_cmp( arg, "help" ) )
   {
      ch->print( "Usage: setmark add\r\n" );
      ch->print( "Usage: setmark delete\r\n" );
      ch->print( "Usage: setmark distance <value>\r\n" );
      ch->print( "Usage: setmark desc\r\n" );
      ch->print( "Usage: setmark isdesc\r\n" );
      return;
   }

   landmark = ch->continent->check_landmark( ch->map_x, ch->map_y );

   if( !str_cmp( arg, "add" ) )
   {
      if( landmark )
      {
         ch->print( "There's already a landmark at this location.\r\n" );
         return;
      }
      ch->continent->add_landmark( ch->map_x, ch->map_y );
      ch->continent->save(  );
      ch->print( "Landmark added.\r\n" );
      return;
   }

   if( !str_cmp( arg, "delete" ) )
   {
      if( !landmark )
      {
         ch->print( "There is no landmark here.\r\n" );
         return;
      }
      ch->continent->delete_landmark( landmark );
      ch->continent->save(  );
      ch->print( "Landmark deleted.\r\n" );
      return;
   }

   if( !landmark )
   {
      ch->print( "There is no landmark here.\r\n" );
      return;
   }

   if( !str_cmp( arg, "isdesc" ) )
   {
      landmark->Isdesc = !landmark->Isdesc;
      ch->continent->save(  );

      if( landmark->Isdesc )
         ch->print( "Landmark is now a room description.\r\n" );
      else
         ch->print( "Room description is now a landmark.\r\n" );
      return;
   }

   if( !str_cmp( arg, "distance" ) )
   {
      int value;

      if( !is_number( argument ) )
      {
         ch->print( "Distance must be a numeric amount.\r\n" );
         return;
      }

      value = std::stoi( argument );

      if( value < 1 )
      {
         ch->print( "Distance must be at least 1.\r\n" );
         return;
      }
      landmark->distance = value;
      ch->continent->save(  );
      ch->print( "Visibility distance set.\r\n" );
      return;
   }

   if( !str_cmp( arg, "desc" ) || !str_cmp( arg, "description" ) )
   {
      if( ch->substate == SUB_REPEATCMD )
         ch->tempnum = SUB_REPEATCMD;
      else
         ch->tempnum = SUB_NONE;
      ch->substate = SUB_OVERLAND_DESC;
      ch->pcdata->dest_buf = landmark;
      if( landmark->description.empty(  ) )
         landmark->description.clear(  );
      ch->set_editor_desc( "An Overland landmark description." );
      ch->start_editing( landmark->description );
      return;
   }

   do_setmark( ch, "" );
}

/* Overland landmark stuff ends here */

/* Overland exit stuff starts here */
mapexit_data *continent_data::check_mapexit( short x, short y )
{
   for( auto* mexit : this->exits )
   {
      if( mexit->herex == x && mexit->herey == y )
         return mexit;
   }
   return nullptr;
}

void continent_data::modify_mapexit( mapexit_data * mexit, std::string_view tomap, short hereX, short hereY, short thereX, short thereY, int mvnum )
{
   if( !mexit )
   {
      bug( "{} -> {}:{}: nullptr exit being modified!", __func__, __FILE__, __LINE__ );
      return;
   }

   mexit->tomap = tomap;
   mexit->herex = hereX;
   mexit->herey = hereY;
   mexit->therex = thereX;
   mexit->therey = thereY;
   mexit->vnum = mvnum;
}

void continent_data::add_mapexit( std::string_view tomap, short hereX, short hereY, short thereX, short thereY, int mvnum )
{
   mapexit_data *mexit;

   mexit = new mapexit_data;
   mexit->tomap = tomap;
   mexit->herex = hereX;
   mexit->herey = hereY;
   mexit->therex = thereX;
   mexit->therey = thereY;
   mexit->vnum = mvnum;
   mexit->prevsector = this->get_terrain( hereX, hereY );
   this->exits.push_back( mexit );
}

void continent_data::delete_mapexit( mapexit_data * mexit )
{
   if( !mexit )
   {
      bug( "{} -> {}:{}: Trying to delete nullptr exit!", __func__, __FILE__, __LINE__ );
      return;
   }

   this->exits.remove( mexit );
   deleteptr( mexit );
}

/* OLC command to add/delete/edit overland exit information */
CMDF( do_setexit )
{
   std::string arg;
   room_index *location;
   mapexit_data *mexit = nullptr;
   int vnum;

#ifdef MULTIPORT
   if( mud_port == MAINPORT )
   {
      ch->print( "This command is not available on this port.\r\n" );
      return;
   }
#endif

   if( ch->isnpc(  ) )
   {
      ch->print( "Sorry, NPCs can't edit the overland maps.\r\n" );
      return;
   }

   if( !ch->has_pcflag( PCFLAG_ONMAP ) )
   {
      ch->print( "This command can only be used from an overland map.\r\n" );
      return;
   }

   argument = one_argument( argument, arg );

   if( arg.empty(  ) || !str_cmp( arg, "help" ) )
   {
      ch->print( "Usage: setexit create\r\n" );
      ch->print( "Usage: setexit delete\r\n" );
      ch->print( "Usage: setexit vnum <vnum>\r\n" );
      ch->print( "Usage: setexit <mapname> <X-coord> <Y-coord>\r\n" );
      return;
   }

   mexit = ch->continent->check_mapexit( ch->map_x, ch->map_y );

   if( !str_cmp( arg, "create" ) )
   {
      if( mexit )
      {
         ch->print( "An exit already exists at these coordinates.\r\n" );
         return;
      }

      ch->continent->add_mapexit( "-1", ch->map_x, ch->map_y, ch->map_x, ch->map_y, -1 );
      ch->continent->putterr( ch->map_x, ch->map_y, SECT_EXIT );

      ch->continent->save( );
      ch->continent->save_png_file( );

      ch->print( "New exit created.\r\n" );
      return;
   }

   if( !mexit )
   {
      ch->print( "No exit exists at these coordinates.\r\n" );
      return;
   }

   if( !str_cmp( arg, "delete" ) )
   {
      ch->continent->putterr( ch->map_x, ch->map_y, mexit->prevsector );
      ch->continent->delete_mapexit( mexit );

      ch->continent->save( );
      ch->continent->save_png_file( );

      ch->print( "Exit deleted.\r\n" );
      return;
   }

   if( !str_cmp( arg, "map" ) )
   {
      continent_data *newmap;
      std::string arg2, arg3;
      short x, y = -1;

      if( !ch->continent )
      {
         bug( "{} -> {}:{}: {} is not on a valid map!", __func__, __FILE__, __LINE__, ch->name );
         ch->print( "Can't do that - your on an invalid map.\r\n" );
         return;
      }

      argument = one_argument( argument, arg2 );
      argument = one_argument( argument, arg3 );

      if( arg2.empty(  ) )
      {
         ch->print( "Make an exit to what map??\r\n" );
         return;
      }

      newmap = find_continent_by_name( arg2 );

      if( !newmap )
      {
         ch->print_fmt( "There isn't a map for '{}'.\r\n", arg2 );
         return;
      }

      x = std::stoi( arg3 );
      y = std::stoi( argument );

      if( !is_valid_x( x ) )
      {
         ch->print_fmt( "Valid x coordinates are 0 to {}.\r\n", MAX_X - 1 );
         return;
      }

      if( !is_valid_y( y ) )
      {
         ch->print_fmt( "Valid y coordinates are 0 to {}.\r\n", MAX_Y - 1 );
         return;
      }

      ch->continent->modify_mapexit( mexit, newmap->name, ch->map_x, ch->map_y, x, y, -1 );
      ch->continent->putterr( ch->map_x, ch->map_y, SECT_EXIT );

      ch->continent->save( );
      ch->continent->save_png_file( );

      ch->print_fmt( "Exit set to map of {}, at {}X, {}Y.\r\n", arg2, x, y );

      return;
   }

   if( !str_cmp( arg, "vnum" ) )
   {
      vnum = std::stoi( argument );

      if( !( location = get_room_index( vnum ) ) )
      {
         ch->print( "No such room exists.\r\n" );
         return;
      }

      ch->continent->modify_mapexit( mexit, "-1", ch->map_x, ch->map_y, -1, -1, vnum );
      ch->continent->putterr( ch->map_x, ch->map_y, SECT_EXIT );

      ch->continent->save( );
      ch->continent->save_png_file( );

      ch->print_fmt( "Exit set to room {}.\r\n", vnum );
      return;
   }
}
/* Overland exit stuff ends here */

/*
 * The guts of the map display code. Streamlined to only change color codes when it needs to.
 * If you are also using my newly updated Custom Color code you'll get even better performance
 * out of it since that code replaced the stock Smaug color tag converter, which shaved a few
 * microseconds off the time it takes to display a full immortal map :)
 * Don't believe me? Check this out:
 * 
 * 3 immortals in full immortal view spamming 15 movement commands:
 * Unoptimized code: Timing took 0.123937 seconds.
 * Optimized code  : Timing took 0.031564 seconds.
 *
 * 3 mortals with wizardeye spell on spamming 15 movement commands:
 * Unoptimized code: Timing took 0.064086 seconds.
 * Optimized code  : Timing took 0.009459 seconds.
 *
 * Results: The code is at the very least 4x faster than it was before being optimized.
 *
 * Problem? It crashes the damn game with MSL set to 4096, so I raised it to 8192.
 *
 * Proper solution: Converted the buffer to std::string and now, in theory, it could be as large as the memory on your server.
 *
 * And now we've finally done something to improve performance even further and reduce the number of heap allocations reprocessing the output string.
 * Samson 6/11/2026.
 *
 * TODO: Future plans - Generate a version of this that would work with GMCP enabled clients like Mudlet and use Unicode symbols in it for the terrain.
 */
void new_map_to_char( char_data * ch, short startx, short starty, short endx, short endy, int radius )
{
   if( !ch || !ch->in_room )
      return;

   startx = urange( 0, startx, MAX_X - 1 );
   starty = urange( 0, starty, MAX_Y - 1 );
   endx = urange( 0, endx, MAX_X - 1 );
   endy = urange( 0, endy, MAX_Y - 1 );

   // Pre-allocate buffer to prevent reallocations on the heap. They slow things down by quite a bit.
   std::string secbuf;
   secbuf.reserve( ( static_cast<std::size_t>( endx - startx + 1 ) * static_cast<std::size_t>( endy - starty + 1 ) ) * 16 );
   secbuf.append( "\r\n" );

   // Pre-cache entities at specific coordinates for O(1) retrieval during drawing. This provides even more performance in dealing with other people near you.
   struct MapEntity {
      std::vector<char_data*> people;
      std::vector<obj_data*> objects;
      std::vector<ship_data*> ships;
   };

   // Allocate the grid dynamically based on the current viewport size to save memory.
   short width = ( endx - startx + 1 );
   short height = ( endy - starty + 1 );
   std::vector<MapEntity> grid( width * height );

   auto get_grid_index = [startx, starty, width](short x, short y) -> int {
      return (x - startx) + (y - starty) * width;
   };

   // Populate grid for people in the room.
   for( auto* rch : ch->in_room->people ) {
      if( rch->map_x >= startx && rch->map_x <= endx && rch->map_y >= starty && rch->map_y <= endy ) {
         grid[get_grid_index(rch->map_x, rch->map_y)].people.push_back(rch);
      }
   }

   // Populate grid for objects in the room.
   for( auto* obj : ch->in_room->objects ) {
      if( obj->map_x >= startx && obj->map_x <= endx && obj->map_y >= starty && obj->map_y <= endy ) {
         grid[get_grid_index(obj->map_x, obj->map_y)].objects.push_back(obj);
      }
   }

   // Populate grid for ships.
   for( auto* ship : shiplist ) {
      if( ship->room == ch->in_room->vnum && ship->map_x >= startx && ship->map_x <= endx && ship->map_y >= starty && ship->map_y <= endy ) {
         grid[get_grid_index(ship->map_x, ship->map_y)].ships.push_back(ship);
      }
   }

   short lastsector = -1;

   for( short y = starty; y <= endy; ++y )
   {
      for( short x = startx; x <= endx; ++x )
      {
         if( distance( ch->map_x, ch->map_y, x, y ) > radius )
         {
            if( !ch->has_pcflag( PCFLAG_HOLYLIGHT ) && !ch->in_room->flags.test( ROOM_WATCHTOWER ) && !ch->inflight )
            {
               secbuf += " ";
               lastsector = -1;
               continue;
            }
         }

         short sector = ch->continent->get_terrain( x, y );
         bool other = false, npc = false, object = false, group = false, aship = false;

         const auto& tile_entities = grid[get_grid_index(x, y)];

         // Process pre-cached people
         for( auto* rch : tile_entities.people )
         {
            if( x == rch->map_x && y == rch->map_y && rch != ch && ( rch->map_x != ch->map_x || rch->map_y != ch->map_y ) )
            {
               if( rch->has_pcflag( PCFLAG_WIZINVIS ) && rch->pcdata->wizinvis > ch->level )
                  other = false;
               else
               {
                  other = true;
                  if( rch->isnpc() )
                     npc = true;
                  lastsector = -1;
               }
            }

            if( is_same_group( ch, rch ) && ch != rch )
            {
               if( x == ch->map_x && y == ch->map_y && is_same_char_map( ch, rch ) )
               {
                  group = true;
                  lastsector = -1;
               }
            }
         }

         // Process pre-cached objects.
         for( auto* obj : tile_entities.objects )
         {
            // Nolocate flags should block the $. Useful for road signs and such.
            if( x == obj->map_x && y == obj->map_y && !is_same_obj_map( ch, obj ) && !obj->extra_flags.test( ITEM_NOLOCATE ) )
            {
               object = true;
               lastsector = -1;
            }
         }

         // Process pre-cached ships
         for( const auto* ship : tile_entities.ships )
         {
            if( x == ship->map_x && y == ship->map_y )
            {
               aship = true;
               lastsector = -1;
            }
         }

         if( object && !other && !aship )
            secbuf.append( "&Y$" );

         if( other && !aship )
         {
            if( npc )
               secbuf.append( "&B@" );
            else
               secbuf.append( "&P@" );
         }

         if( aship )
            secbuf.append( "&R4" );

         if( x == ch->map_x && y == ch->map_y && !aship )
         {
            if( group )
               secbuf.append( "&Y@" );
            else
               secbuf.append( "&R@" );
            other = true;
            lastsector = -1;
         }

         if( !other && !object && !aship )
         {
            if( lastsector == sector )
            {
               secbuf.append( sect_show[sector].symbol );
            }
            else
            {
               lastsector = sector;
               secbuf.append( sect_show[sector].color );
               secbuf.append( sect_show[sector].symbol );
            }
         }
      }
      secbuf.append( "\r\n" );
   }
   ch->print( secbuf );
}

/*
 * This function determines the size of the display to show to a character.
 * For immortals, it also shows any details present in the sector, like resets and landmarks.
 */
void display_map( char_data * ch )
{
   landmark_data *landmark = nullptr;
   landing_data *landing = nullptr;
   short startx, starty, endx, endy, sector;
   int mod = sysdata->mapsize;

   if( !ch->continent )
   {
      bug( "{} -> {}:{}: Player {} on invalid map! Moving them to Bywater recall room.", __func__, __FILE__, __LINE__, ch->name );
      ch->print( "&RYou were found on an invalid map and have been moved to the recall room in Bywater.\r\n" );
      room_index *toroom = get_room_index( ROOM_VNUM_ALTAR );
      leave_map( ch, nullptr, toroom );
      return;
   }

   sector = ch->continent->get_terrain( ch->map_x, ch->map_y );
   WeatherCell *cell = getWeatherCell( ch->continent->area );

   if( ch->has_pcflag( PCFLAG_HOLYLIGHT ) || ch->in_room->flags.test( ROOM_WATCHTOWER ) || ch->inflight )
   {
      startx = ch->map_x - 37;
      endx = ch->map_x + 37;
      starty = ch->map_y - 14;
      endy = ch->map_y + 14;
   }
   else
   {
      const obj_data *light = ch->get_eq( WEAR_LIGHT );

      if( time_info.hour == sysdata->hoursunrise || time_info.hour == sysdata->hoursunset )
         mod = 4;

      if( getCloudCover( cell ) > 0 )
      {
         if( isExtremelyCloudy( getCloudCover( cell ) ) || isModeratelyCloudy( getCloudCover( cell ) ) )
            mod -= 2;
         else if( isPartlyCloudy( getCloudCover( cell ) ) || isCloudy( getCloudCover( cell ) ) )
            mod -= 1;
      }

      if( time_info.hour > sysdata->hoursunset || time_info.hour < sysdata->hoursunrise )
         mod = 2;

      if( light != nullptr )
      {
         if( light->item_type == ITEM_LIGHT && ( time_info.hour > sysdata->hoursunset || time_info.hour < sysdata->hoursunrise ) )
            mod += 1;
      }

      if( ch->has_aflag( AFF_WIZARDEYE ) )
         mod = sysdata->mapsize * 2;

      startx = ( umax( ( short )( ch->map_x - ( mod * 1.5 ) ), ch->map_x - 37 ) );
      starty = ( umax( ch->map_y - mod, ch->map_y - 14 ) );
      endx = ( umin( ( short )( ch->map_x + ( mod * 1.5 ) ), ch->map_x + 37 ) );
      endy = ( umin( ch->map_y + mod, ch->map_y + 14 ) );
   }

   if( ch->has_pcflag( PCFLAG_MAPEDIT ) && sector != SECT_EXIT )
   {
      ch->continent->putterr( ch->map_x, ch->map_y, ch->pcdata->secedit );
      sector = ch->pcdata->secedit;
   }

   new_map_to_char( ch, startx, starty, endx, endy, mod );

   if( !ch->inflight && !ch->in_room->flags.test( ROOM_WATCHTOWER ) )
   {
      ch->print_fmt( "&GTraveling on the continent of {}.\r\n", ch->continent->name );
      landmark = ch->continent->check_landmark( ch->map_x, ch->map_y );

      if( landmark && landmark->Isdesc )
         ch->print_fmt( "&G{}\r\n", !landmark->description.empty(  ) ? landmark->description : "" );
      else
         ch->print_fmt( "&G{}\r\n", impass_message[sector] );
   }
   else if( ch->in_room->flags.test( ROOM_WATCHTOWER ) )
   {
      ch->print_fmt( "&YYou are overlooking the continent of {}.\r\n", ch->continent->name );
      ch->print( "The view from this watchtower is amazing!\r\n" );
   }
   else
      ch->print_fmt( "&GRiding a skyship over the continent of {}.\r\n", ch->continent->name );

   if( ch->is_immortal(  ) )
   {
      ch->print_fmt( "&GSector type: {}. Coordinates: {}X, {}Y\r\n", sect_types[sector], ch->map_x, ch->map_y );

      landing = ch->continent->check_landing_site( ch->map_x, ch->map_y );

      if( landing )
         ch->print_fmt( "&CLanding site for {}.\r\n", !landing->area.empty(  ) ? landing->area : "<NOT SET>" );

      if( landmark && !landmark->Isdesc )
      {
         ch->print_fmt( "&BLandmark present: {}\r\n", !landmark->description.empty(  ) ? landmark->description : "<NO DESCRIPTION>" );
         ch->print_fmt( "&BVisibility distance: {}.\r\n", landmark->distance );
      }

      if( ch->has_pcflag( PCFLAG_MAPEDIT ) )
         ch->print_fmt( "&YYou are currently creating {} sectors.&z\r\n", sect_types[ch->pcdata->secedit] );
   }
}

/*
 * Called in update.cpp modification for wandering mobiles - Samson 7-29-00
 *
 * For mobs entering overland from normal zones:
 *
 * If the sector of the origin room matches the terrain type on the map, it becomes the mob's native terrain.
 * If the origin room sector does NOT match, then the mob will assign itself the first terrain type it can move onto that is different from what it entered on.
 * This allows you to load mobs in a loading room, and have them stay on roads or trails to act as patrols.
 * It also compensates for when a map road or trail enters a zone and becomes forest.
 */
bool map_wander( char_data * ch, short x, short y, short sector )
{
   short terrain = ch->continent->get_terrain( x, y );

   /*
    * Obviously sentinel mobs have no need to move :P 
    */
   if( ch->has_actflag( ACT_SENTINEL ) )
      return false;

   /*
    * Allows the mob to move onto it's native terrain as well as roads or trails - 
    * * EG: a mob loads in a forest, but a road slices it in half, mob can cross the
    * * road to reach more forest on the other side. Won't cross SECT_ROAD though,
    * * we use this to keep SECT_CITY and SECT_TRAIL mobs from leaving the roads
    * * near their origin sites - Samson 7-29-00
    */
   if( terrain == ch->sector || terrain == SECT_CITY || terrain == SECT_TRAIL )
      return true;

   /*
    * Sector -2 is used to tell the code this mob came to the overland from an internal
    * * zone, but the terrain didn't match the originating room's sector type. It'll assign 
    * * the first differing terrain upon moving, provided it isn't a SECT_ROAD. From then on 
    * * it will only wander in that type of terrain - Samson 7-29-00
    */
   if( ch->sector == -2 && terrain != sector && terrain != SECT_ROAD && sect_show[terrain].canpass )
   {
      ch->sector = terrain;
      return true;
   }

   return false;
}

/*
 * This function does a random check, currently set to 6%, to see if it should load a mobile
 * at the sector the PC just walked into. I have no doubt a cleaner, more efficient way can
 * be found to accomplish this, but until such time as divine inspiration hits me ( or some kind
 * soul shows me the light ) this is how its done. [WOO! Yes Geni, I think that's how its done :P]
 * 
 * Rewritten by Geni to use tables and way too many mobs.
 */
void check_random_mobs( char_data * ch )
{
   mob_index *imob = nullptr;
   char_data *mob = nullptr;
   int vnum = -1;
   short terrain = ch->continent->get_terrain( ch->map_x, ch->map_y );

   // This could ( and did ) get VERY messy VERY quickly if wandering mobs trigger more of themselves
   if( ch->isnpc(  ) )
      return;

   /*
    * Temp fix to protect newbies in the academy from overland mobs 
    */
   if( !str_cmp( ch->continent->name, "One" ) && distance( ch->map_x, ch->map_y, 911, 932 ) <= 30 )
      return;

   if( number_percent(  ) < 6 )
      vnum = random_mobs[terrain][umin( SECT_MAX - 1, number_range( 0, ch->level / 2 ) )];

   if( vnum == -1 )
      return;

   if( !( imob = get_mob_index( vnum ) ) )
   {
      log_printf( "{}: Missing mob for vnum {}", __func__, vnum );
      return;
   }

   mob = imob->create_mobile(  );
   if( !mob->to_room( ch->in_room ) )
      log_printf( "char_to_room: {}:{}, line {}.", __FILE__, __func__, __LINE__ );
   mob->set_actflag( ACT_ONMAP );
   mob->sector = terrain;
   mob->continent = ch->continent;
   mob->map_x = ch->map_x;
   mob->map_y = ch->map_y;
   mob->timer = 400;

   /*
    * 400 should be long enough. If not, increase. Mob will be extracted when it expires.
    * * And trust me, this is a necessary measure unless you LIKE having your memory flooded
    * * by random overland mobs.
    */
}

/*
 * An overland hack of the scan command - the OLD scan command, not the one in stock Smaug
 * where you have to specify a direction to scan in. This is only called from within do_scan
 * and probably won't be of much use to you unless you want to modify your scan command to
 * do like ours. And hey, if you do, I'd be happy to send you our do_scan - separately.
 */
void map_scan( char_data * ch )
{
   int mod = sysdata->mapsize;
   bool found = false;

   if( !ch )
      return;

   const obj_data *light = ch->get_eq( WEAR_LIGHT );
   const WeatherCell *cell = getWeatherCell( ch->continent->area );

   if( time_info.hour == sysdata->hoursunrise || time_info.hour == sysdata->hoursunset )
      mod = 4;

   if( getCloudCover( cell ) > 0 )
   {
      if( isExtremelyCloudy( getCloudCover( cell ) ) || isModeratelyCloudy( getCloudCover( cell ) ) )
         mod -= 2;
      else if( isPartlyCloudy( getCloudCover( cell ) ) || isCloudy( getCloudCover( cell ) ) )
         mod -= 1;
   }

   if( time_info.hour > sysdata->hoursunset || time_info.hour < sysdata->hoursunrise )
      mod = 1;

   if( light != nullptr )
   {
      if( light->item_type == ITEM_LIGHT && ( time_info.hour > sysdata->hoursunset || time_info.hour < sysdata->hoursunrise ) )
         mod += 1;
   }

   if( ch->has_aflag( AFF_WIZARDEYE ) )
      mod = sysdata->mapsize * 2;

   /*
    * Freshen the map with up to the nanosecond display :) 
    */
   interpret( ch, "look" );

   for( auto* gch : ch->in_room->people )
   {
      /*
       * No need in scanning for yourself. 
       */
      if( ch == gch )
         continue;

      /*
       * Don't reveal invisible imms 
       */
      if( gch->has_pcflag( PCFLAG_WIZINVIS ) && gch->pcdata->wizinvis > ch->level )
         continue;

      double dist = distance( ch->map_x, ch->map_y, gch->map_x, gch->map_y );

      /*
       * Save the math if they're too far away anyway 
       */
      if( dist <= mod )
      {
         double angle = calc_angle( ch->map_x, ch->map_y, gch->map_x, gch->map_y, &dist );
         int dir = -1, iMes = 0;
         found = true;

         if( angle == -1 )
            dir = -1;
         else if( angle >= 360 )
            dir = DIR_NORTH;
         else if( angle >= 315 )
            dir = DIR_NORTHWEST;
         else if( angle >= 270 )
            dir = DIR_WEST;
         else if( angle >= 225 )
            dir = DIR_SOUTHWEST;
         else if( angle >= 180 )
            dir = DIR_SOUTH;
         else if( angle >= 135 )
            dir = DIR_SOUTHEAST;
         else if( angle >= 90 )
            dir = DIR_EAST;
         else if( angle >= 45 )
            dir = DIR_NORTHEAST;
         else if( angle >= 0 )
            dir = DIR_NORTH;

         if( dist > 200 )
            iMes = 0;
         else if( dist > 150 )
            iMes = 1;
         else if( dist > 100 )
            iMes = 2;
         else if( dist > 75 )
            iMes = 3;
         else if( dist > 50 )
            iMes = 4;
         else if( dist > 25 )
            iMes = 5;
         else if( dist > 15 )
            iMes = 6;
         else if( dist > 10 )
            iMes = 7;
         else if( dist > 5 )
            iMes = 8;
         else if( dist > 1 )
            iMes = 9;
         else
            iMes = 10;

         if( dir == -1 )
            ch->print_fmt( "&[skill]Here with you: {}.\r\n", gch->name );
         else
            ch->print_fmt( "&[skill]To the {}, {}, {}.\r\n", dir_name[dir], landmark_distances[iMes], gch->name );
      }
   }
   if( !found )
      ch->print( "Your survey of the area turns up nobody.\r\n" );
}

// Note: For various reasons, this isn't designed to pull PC followers along with you
void collect_followers( char_data * ch, room_index * from, room_index * to )
{
   if( !ch )
   {
      bug( "{} -> {}:{}: nullptr master!", __func__, __FILE__, __LINE__ );
      return;
   }

   if( !from )
   {
      bug( "{} -> {}:{}: {} nullptr source room!", __func__, __FILE__, __LINE__, ch->name );
      return;
   }

   if( !to )
   {
      bug( "{} -> {}:{}: {} nullptr target room!", __func__, __FILE__, __LINE__, ch->name );
      return;
   }

   for( auto it = from->people.begin(); it != from->people.end(); )
   {
      char_data *fch = *it;
      ++it;

      if( fch != ch && fch->master == ch && ( fch->position == POS_STANDING || fch->position == POS_MOUNTED ) )
      {
         if( !fch->isnpc(  ) )
            continue;

         fch->from_room(  );
         if( !fch->to_room( to ) )
            log_printf( "char_to_room: {}:{}, line {}.", __FILE__, __func__, __LINE__ );
         fix_maps( ch, fch );
      }
   }
}

/*
 * The guts of movement on the overland. Checks all sorts of nice things. Makes DAMN
 * sure you can actually go where you just tried to. Breaks easily if messed with wrong too.
 */
ch_ret process_exit( char_data * ch, short x, short y, int dir, bool running )
{
   // Cheap ass hack for now - better than nothing though
   for( auto* ship : shiplist )
   {
      if( ship->continent == ch->continent && ship->map_x == x && ship->map_y == y )
      {
         if( !str_cmp( ch->name, ship->owner ) )
         {
            ch->set_pcflag( PCFLAG_ONSHIP );
            ch->on_ship = ship;
            ch->map_x = ship->map_x;
            ch->map_y = ship->map_y;
            interpret( ch, "look" );
            ch->print_fmt( "You board {}.\r\n", ship->name );
            return rSTOP;
         }
         else
         {
            ch->print_fmt( "The crew aboard {} blocks you from boarding!\r\n", ship->name );
            return rSTOP;
         }
      }
   }

   bool drunk = false;
   if( !ch->isnpc(  ) )
   {
      if( ch->IS_DRUNK( 2 ) && ( ch->position != POS_SHOVE ) && ( ch->position != POS_DRAG ) )
         drunk = true;
   }

   bool boat = false;
   for( const auto* obj : ch->carrying )
   {
      if( obj->item_type == ITEM_BOAT )
      {
         boat = true;
         break;
      }
   }

   if( ch->has_aflag( AFF_FLYING ) || ch->has_aflag( AFF_FLOATING ) )
      boat = true;   /* Cheap hack, but I think it'll work for this purpose */

   if( ch->is_immortal(  ) )
      boat = true;   /* Cheap hack, but hey, imms need a break on this one :P */

   room_index *from_room = ch->in_room;
   const continent_data *from_continent = ch->continent;
   int sector = ch->continent->get_terrain( x, y );
   short fx = ch->map_x, fy = ch->map_y;

   if( sector == SECT_EXIT )
   {
      mapexit_data *mexit;
      room_index *toroom = nullptr;

      mexit = ch->continent->check_mapexit( x, y );

      if( mexit != nullptr && !ch->has_pcflag( PCFLAG_MAPEDIT ) )
      {
         if( str_cmp( mexit->tomap, "-1" ) )   /* Means exit goes to another map */
         {
            enter_map( ch, nullptr, mexit->therex, mexit->therey, mexit->tomap );

            size_t chars = from_room->people.size(  );
            size_t count = 0;
            for( auto ich = from_room->people.begin(  ); ich != from_room->people.end(  ) && ( count < chars ); )
            {
               char_data *fch = *ich;
               ++ich;
               ++count;

               if( fch != ch  /* loop room bug fix here by Thoric */
                   && fch->master == ch && ( fch->position == POS_STANDING || fch->position == POS_MOUNTED ) && fch->map_x == fx && fch->map_y == fy && fch->continent == from_continent )
               {
                  if( !fch->isnpc(  ) )
                  {
                     act( AT_ACTION, "You follow $N.", fch, nullptr, ch, TO_CHAR );
                     process_exit( fch, x, y, dir, running );
                  }
                  else
                     enter_map( fch, nullptr, mexit->therex, mexit->therey, mexit->tomap );
               }
            }
            return rSTOP;
         }

         if( !( toroom = get_room_index( mexit->vnum ) ) )
         {
            if( !ch->isnpc(  ) )
            {
               bug( "{} -> {}:{}: Target vnum {} for map exit does not exist!", __func__, __FILE__, __LINE__, mexit->vnum );
               ch->print( "Ooops. Something bad happened. Contact the immortals ASAP.\r\n" );
            }
            return rSTOP;
         }

         if( ch->isnpc(  ) )
         {
            std::list<exit_data *>::iterator ex;
            exit_data *pexit;
            bool found = false;

            for( ex = toroom->exits.begin(  ); ex != toroom->exits.end(  ); ++ex )
            {
               pexit = *ex;

               if( IS_EXIT_FLAG( pexit, EX_OVERLAND ) )
               {
                  found = true;
                  break;
               }
            }

            if( found )
            {
               if( IS_EXIT_FLAG( pexit, EX_NOMOB ) )
               {
                  ch->print( "Mobs cannot go there.\r\n" );
                  return rSTOP;
               }
            }
         }

         if( ( toroom->sector_type == SECT_WATER_NOSWIM || toroom->sector_type == SECT_OCEAN ) && !boat )
         {
            ch->print( "The water is too deep to cross without a boat or spell!\r\n" );
            return rSTOP;
         }

         if( toroom->sector_type == SECT_RIVER && !boat )
         {
            ch->print( "The river is too swift to cross without a boat or spell!\r\n" );
            return rSTOP;
         }

         if( toroom->sector_type == SECT_AIR && !ch->has_aflag( AFF_FLYING ) )
         {
            ch->print( "You'd need to be able to fly to go there!\r\n" );
            return rSTOP;
         }

         leave_map( ch, nullptr, toroom );

         size_t chars = from_room->people.size(  );
         size_t count = 0;
         for( auto ich = from_room->people.begin(  ); ich != from_room->people.end(  ) && ( count < chars ); )
         {
            char_data *fch = *ich;
            ++ich;
            ++count;

            if( fch != ch  /* loop room bug fix here by Thoric */
                && fch->master == ch && ( fch->position == POS_STANDING || fch->position == POS_MOUNTED ) && fch->map_x == fx && fch->map_y == fy && fch->continent == from_continent )
            {
               if( !fch->isnpc(  ) )
               {
                  act( AT_ACTION, "You follow $N.", fch, nullptr, ch, TO_CHAR );
                  process_exit( fch, x, y, dir, running );
               }
               else
                  leave_map( fch, ch, toroom );
            }
         }
         return rSTOP;
      }

      if( mexit != nullptr && ch->has_pcflag( PCFLAG_MAPEDIT ) )
      {
         ch->continent->delete_mapexit( mexit );
         ch->continent->putterr( x, y, ch->pcdata->secedit );

         ch->print( "&RMap exit deleted.\r\n" );
      }
   }

   if( !sect_show[sector].canpass && !ch->is_immortal(  ) )
   {
      ch->print_fmt( "{}\r\n", impass_message[sector] );
      return rSTOP;
   }

   if( sector == SECT_RIVER && !boat )
   {
      ch->print( "The river is too swift to cross without a boat or spell!\r\n" );
      return rSTOP;
   }

   if( sector == SECT_WATER_NOSWIM && !boat )
   {
      ch->print( "The water is too deep to navigate without a boat or spell!\r\n" );
      return rSTOP;
   }

   switch ( dir )
   {
      default:
         ch->print( "Alas, you cannot go that way...\r\n" );
         return rSTOP;

      case DIR_NORTH:
         if( y == -1 )
         {
            ch->print( "You cannot go any further north!\r\n" );
            return rSTOP;
         }
         break;

      case DIR_EAST:
         if( x == MAX_X )
         {
            ch->print( "You cannot go any further east!\r\n" );
            return rSTOP;
         }
         break;

      case DIR_SOUTH:
         if( y == MAX_Y )
         {
            ch->print( "You cannot go any further south!\r\n" );
            return rSTOP;
         }
         break;

      case DIR_WEST:
         if( x == -1 )
         {
            ch->print( "You cannot go any further west!\r\n" );
            return rSTOP;
         }
         break;

      case DIR_NORTHEAST:
         if( x == MAX_X || y == -1 )
         {
            ch->print( "You cannot go any further northeast!\r\n" );
            return rSTOP;
         }
         break;

      case DIR_NORTHWEST:
         if( x == -1 || y == -1 )
         {
            ch->print( "You cannot go any further northwest!\r\n" );
            return rSTOP;
         }
         break;

      case DIR_SOUTHEAST:
         if( x == MAX_X || y == MAX_Y )
         {
            ch->print( "You cannot go any further southeast!\r\n" );
            return rSTOP;
         }
         break;

      case DIR_SOUTHWEST:
         if( x == -1 || y == MAX_Y )
         {
            ch->print( "You cannot go any further southwest!\r\n" );
            return rSTOP;
         }
         break;
   }

   int move = 0;
   if( ch->mount && !ch->mount->IS_FLOATING(  ) )
      move = sect_show[sector].move;
   else if( !ch->IS_FLOATING(  ) )
      move = sect_show[sector].move;
   else
      move = 1;

   if( ch->mount && ch->mount->move < move )
   {
      ch->print( "Your mount is too exhausted.\r\n" );
      return rSTOP;
   }

   if( ch->move < move )
   {
      ch->print( "You are too exhausted.\r\n" );
      return rSTOP;
   }

   const char *txt;
   if( ch->mount )
   {
      if( ch->mount->has_aflag( AFF_FLOATING ) )
         txt = "floats";
      else if( ch->mount->has_aflag( AFF_FLYING ) )
         txt = "flies";
      else
         txt = "rides";
   }
   else if( ch->has_aflag( AFF_FLOATING ) )
   {
      if( drunk )
         txt = "floats unsteadily";
      else
         txt = "floats";
   }
   else if( ch->has_aflag( AFF_FLYING ) )
   {
      if( drunk )
         txt = "flies shakily";
      else
         txt = "flies";
   }
   else if( ch->position == POS_SHOVE )
      txt = "is shoved";
   else if( ch->position == POS_DRAG )
      txt = "is dragged";
   else
   {
      if( drunk )
         txt = "stumbles drunkenly";
      else
         txt = "leaves";
   }

   if( !running )
   {
      if( ch->mount )
         act_printf( AT_ACTION, ch, nullptr, ch->mount, TO_NOTVICT, "$n {} {} upon $N.", txt, dir_name[dir] );
      else
         act_printf( AT_ACTION, ch, nullptr, dir_name[dir], TO_ROOM, "$n {} $T.", txt );
   }

   if( !ch->is_immortal(  ) ) /* Imms don't get charged movement */
   {
      if( ch->mount )
      {
         ch->mount->move -= move;
         ch->mount->map_x = x;
         ch->mount->map_y = y;
      }
      else
         ch->move -= move;
   }

   ch->map_x = x;
   ch->map_y = y;

   /*
    * Don't make your imms suffer - editing with movement delays is a bitch 
    */
   if( !ch->is_immortal(  ) )
      ch->WAIT_STATE( move );

   if( ch->mount )
   {
      if( ch->mount->has_aflag( AFF_FLOATING ) )
         txt = "floats in";
      else if( ch->mount->has_aflag( AFF_FLYING ) )
         txt = "flies in";
      else
         txt = "rides in";
   }
   else
   {
      if( ch->has_aflag( AFF_FLOATING ) )
      {
         if( drunk )
            txt = "floats in unsteadily";
         else
            txt = "floats in";
      }
      else if( ch->has_aflag( AFF_FLYING ) )
      {
         if( drunk )
            txt = "flies in shakily";
         else
            txt = "flies in";
      }
      else if( ch->position == POS_SHOVE )
         txt = "is shoved in";
      else if( ch->position == POS_DRAG )
         txt = "is dragged in";
      else
      {
         if( drunk )
            txt = "stumbles drunkenly in";
         else
            txt = "arrives";
      }
   }

   std::string dtxt = rev_exit( dir );

   if( !running )
   {
      if( ch->mount )
         act_printf( AT_ACTION, ch, nullptr, ch->mount, TO_ROOM, "$n {} from {} upon $N.", txt, dtxt );
      else
         act_printf( AT_ACTION, ch, nullptr, nullptr, TO_ROOM, "$n {} from {}.", txt, dtxt );
   }

   size_t chars = from_room->people.size(  );
   size_t count = 0;
   for( auto ich = from_room->people.begin(  ); ich != from_room->people.end(  ) && ( count < chars ); )
   {
      char_data *fch = *ich;
      ++ich;
      ++count;

      if( fch != ch  /* loop room bug fix here by Thoric */
          && fch->master == ch && ( fch->position == POS_STANDING || fch->position == POS_MOUNTED ) && fch->map_x == fx && fch->map_y == fy )
      {
         if( !fch->isnpc(  ) )
         {
            if( !running )
               act( AT_ACTION, "You follow $N.", fch, nullptr, ch, TO_CHAR );
            process_exit( fch, x, y, dir, running );
         }
         else
         {
            fch->map_x = x;
            fch->map_y = y;
         }
      }
   }

   check_random_mobs( ch );

   if( !running )
      interpret( ch, "look" );

   ch_ret retcode = rNONE;
   mprog_entry_trigger( ch );
   if( ch->char_died(  ) )
      return retcode;

   rprog_enter_trigger( ch );
   if( ch->char_died(  ) )
      return retcode;

   mprog_greet_trigger( ch );
   if( ch->char_died(  ) )
      return retcode;

   oprog_greet_trigger( ch );
   if( ch->char_died(  ) )
      return retcode;

   return retcode;
}

/*
 * How one gets from a normal zone onto the overland, via an exit flagged as EX_OVERLAND
 * A cheap hack has been employed for the continent overhaul to use "-1" as a string argument
 * if you're coming from a regular room exit.
 */
void enter_map( char_data * ch, exit_data * pexit, int x, int y, std::string_view tomap )
{
   room_index *maproom = nullptr, *original;
   continent_data *continent = nullptr;

   if( !str_cmp( tomap, "-1" ) )  /* -1 means you came in from a regular area exit */
   {
      continent = find_continent_by_room( ch->in_room );

      if( !continent )
      {
         bug( "{} -> {}:{}: Cannot find area to enter an overland map from room {}.", __func__, __FILE__, __LINE__, ch->in_room->vnum );
         return;
      }

      maproom = get_room_index( continent->vnum );
   }
   else  /* Means you are either an immortal using the goto command, or a mortal who teleported */
   {
      continent = find_continent_by_name( tomap );

      if( continent )
         maproom = get_room_index( continent->vnum );
      else
      {
         bug( "{} -> {}:{}: Invalid target map specified: {}", __func__, __FILE__, __LINE__, tomap );
         return;
      }
   }

   if( !valid_coordinates( x, y ) )
   {
      ch->print_fmt( "Coordinates {} {} are not valid.\r\n", x, y );
      return;
   }

   /*
    * Hopefully this hack works 
    */
   if( pexit && pexit->to_room->flags.test( ROOM_WATCHTOWER ) )
      maproom = pexit->to_room;

   if( !maproom )
   {
      bug( "{} -> {}:{}: Overland map room is missing!", __func__, __FILE__, __LINE__ );
      ch->print( "Woops. Something is majorly wrong here - inform the immortals.\r\n" );
      return;
   }

   if( !ch->isnpc(  ) )
      ch->set_pcflag( PCFLAG_ONMAP );
   else
   {
      ch->set_actflag( ACT_ONMAP );
      if( ch->sector == -1 && continent->get_terrain( x, y ) == ch->in_room->sector_type )
         ch->sector = continent->get_terrain( x, y );
      else
         ch->sector = -2;
   }

   ch->continent = continent;
   ch->map_x = x;
   ch->map_y = y;

   original = ch->in_room;
   ch->from_room(  );
   if( !ch->to_room( maproom ) )
      log_printf( "char_to_room: {}:{}, line {}.", __FILE__, __func__, __LINE__ );
   collect_followers( ch, original, ch->in_room );
   interpret( ch, "look" );

   /*
    * Turn on the overland music 
    */
   if( !maproom->flags.test( ROOM_WATCHTOWER ) )
      ch->music( "wilderness.mp3", 100, false );
}

/*
 * How one gets off the overland into a regular zone via those nifty white # symbols :) 
 * Of course, it's also how you leave the overland by any means, but hey.
 * This can also be used to simply move a person from a normal room to another normal room.
 */
void leave_map( char_data * ch, char_data * victim, room_index * target )
{
   if( !ch->isnpc(  ) )
   {
      ch->unset_pcflag( PCFLAG_ONMAP );
      ch->unset_pcflag( PCFLAG_MAPEDIT ); /* Just in case they were editing */
   }
   else
      ch->unset_actflag( ACT_ONMAP );

   ch->map_x = -1;
   ch->map_y = -1;
   ch->continent = nullptr;

   if( target != nullptr )
   {
      room_index *from = ch->in_room;
      ch->from_room(  );
      if( !ch->to_room( target ) )
         log_printf( "char_to_room: {}:{}, line {}.", __FILE__, __func__, __LINE__ );
      fix_maps( victim, ch );
      collect_followers( ch, from, target );

      /*
       * Alert, cheesy hack sighted on scanners sir! 
       */
      if( ch->tempnum != 3210 )
         interpret( ch, "look" );

      /*
       * Turn off the overland music if it's still playing 
       */
      if( !ch->has_pcflag( PCFLAG_ONMAP ) )
         ch->reset_music(  );
   }
}

/* Imm command to jump to a different set of coordinates on the same map */
CMDF( do_coords )
{
   std::string arg;
   int x, y;

   if( ch->isnpc(  ) )
   {
      ch->print( "NPCs cannot use this command.\r\n" );
      return;
   }

   if( !ch->has_pcflag( PCFLAG_ONMAP ) )
   {
      ch->print( "This command can only be used from the overland maps.\r\n" );
      return;
   }

   argument = one_argument( argument, arg );

   if( arg.empty(  ) || argument.empty(  ) )
   {
      ch->print( "Usage: coords <x> <y>\r\n" );
      return;
   }

   x = std::stoi( arg );
   y = std::stoi( argument );

   if( !is_valid_x( x ) )
   {
      ch->print_fmt( "Valid x coordinates are 0 to {}.\r\n", MAX_X - 1 );
      return;
   }

   if( !is_valid_y( y ) )
   {
      ch->print_fmt( "Valid y coordinates are 0 to {}.\r\n", MAX_Y - 1 );
      return;
   }

   ch->map_x = x;
   ch->map_y = y;

   if( ch->mount )
   {
      ch->mount->map_x = x;
      ch->mount->map_y = y;
   }

   if( ch->on_ship && ch->has_pcflag( PCFLAG_ONSHIP ) )
   {
      ch->on_ship->map_x = x;
      ch->on_ship->map_y = y;
   }

   interpret( ch, "look" );
}

/* Online OLC map editing stuff starts here */

/* Stuff for the floodfill and undo functions */
struct undotype
{
   struct undotype *next;
   short xcoord;
   short ycoord;
   short prevterr;
};

struct undotype *undohead = 0x0; /* for undo buffer */
struct undotype *undocurr = 0x0; /* most recent undo data */

/*
 * This baby floodfills an entire region of sectors, starting from where the PC is standing,
 * and continuing until it hits something other than what it was told to fill with. 
 * Aborts if you attempt to fill with the same sector your standing on.
 * Floodfill code courtesy of Vor - don't ask for his address, I'm not allowed to give it to you!
 */
int continent_data::floodfill( short xcoord, short ycoord, short fill, char terr )
{
   struct undotype *undonew;

   /*
    * Abort if trying to flood same terr type 
    */
   if( terr == fill )
      return ( 1 );

   undonew = new undotype;

   undonew->xcoord = xcoord;
   undonew->ycoord = ycoord;
   undonew->prevterr = terr;
   undonew->next = 0x0;

   if( undohead == 0x0 )
      undohead = undocurr = undonew;
   else
   {
      undocurr->next = undonew;
      undocurr = undonew;
   };

   this->putterr( xcoord, ycoord, fill );

   if( this->get_terrain( xcoord + 1, ycoord ) == terr )
      this->floodfill( xcoord + 1, ycoord, fill, terr );
   if( this->get_terrain( xcoord, ycoord + 1 ) == terr )
      this->floodfill( xcoord, ycoord + 1, fill, terr );
   if( this->get_terrain( xcoord - 1, ycoord ) == terr )
      this->floodfill( xcoord - 1, ycoord, fill, terr );
   if( this->get_terrain( xcoord, ycoord - 1 ) == terr )
      this->floodfill( xcoord, ycoord - 1, fill, terr );

   return ( 0 );
}

/*
 * call this to undo any floodfills buffered, changes the undo buffer 
 * to hold the undone terr type, so doing an undo twice actually 
 * redoes the floodfill :)
 */
int continent_data::unfloodfill( void )
{
   if( undohead == 0x0 )
      return ( 0 );

   undocurr = undohead;
   do
   {
      char terr = this->get_terrain( undocurr->xcoord, undocurr->ycoord );
      this->putterr( undocurr->xcoord, undocurr->ycoord, undocurr->prevterr );
      undocurr->prevterr = terr;
      undocurr = undocurr->next;
   }
   while( undocurr->next != 0x0 );

   /*
    * you'll prolly want to add some error checking here.. :P 
    */
   return ( 0 );
}

/*
 * call this any time you want to clear the undo buffer, such as 
 * between successful floodfills (otherwise, an undo will undo ALL floodfills done this session :P)
 */
int purgeundo( void )
{
   while( undohead != 0x0 )
   {
      undocurr = undohead;
      undohead = undocurr->next;
      deleteptr( undocurr );
   }

   /*
    * you'll prolly want to add some error checking here.. :P 
    */
   return ( 0 );
}

/*
 * Used to reload a graphic file for the map you are currently standing on.
 * Take great care not to hose your file, results would be unpredictable at best.
 */
void reload_map( char_data * ch )
{
   if( !ch->continent )
   {
      ch->print( "This can only be called from an overland map.\r\n" );
      return;
   }

   ch->print_fmt( "&GReinitializing map grid for {}...\r\n", ch->continent->name );

   for( short x = 0; x < MAX_X; ++x )
   {
      for( short y = 0; y < MAX_Y; ++y )
         ch->continent->putterr( x, y, SECT_OCEAN );
   }

   ch->continent->load_png_file( );
   ch->print_fmt( "Map for {} has been reinitialized from .png file.\r\n", ch->continent->name );
}

CMDF( do_mapcreate )
{
#ifdef MULTIPORT
   if( mud_port == MAINPORT )
   {
      ch->print( "This command is not available on this port.\r\n" );
      return;
   }
#endif

   if( ch->isnpc(  ) )
   {
      ch->print( "Sorry, NPCs can't create overland maps.\r\n" );
      return;
   }

   if( argument.empty() || !str_cmp( argument, "help" ) )
   {
      ch->print( "&YUsage: mapcreate <name>\r\n" );
      ch->print( "'Name' should be a one word name to represent the map.\r\n" );
      ch->print( "An area file matching the name of the new map must exist, with at least one valid room vnum in it.\r\n" );
      ch->print( "Use the 'mapedit' command to set other values.&D\r\n" );
      return;
   }

   continent_data *continent = find_continent_by_name( argument );
   if( continent )
   {
      ch->print_fmt( "&YA map named '{}' already exists.&D\r\n", argument );
      return;
   }

   area_data *pArea = nullptr;
   std::filesystem::path area_file = std::format( "{}.are", argument );
   if( !( pArea = find_area( argument ) ) )
   {
      ch->print_fmt( "&YNo area file named '{}' exists. You must create this before setting up the map.&D\r\n", area_file.string() );
      return;
   }

   continent = new continent_data;

   continent->name = argument;
   continent->area = pArea;
   continent->areafile = area_file;

   std::filesystem::path cont_file = std::format( "{}.cont", argument );
   continent->filename = cont_file;

   continent->vnum = -1;
   continent->nogrid = true;

   continent->save( );
   continent_list.push_back( continent );
   write_continent_list( );

   ch->print_fmt( "&YMap '{}' created. Associated area file '{}'. Filename '{}'.&D\r\n", argument, area_file.string(), cont_file.string() );
}

/*
 * And here we have the OLC command itself. Fairly simplistic really. And more or less useless
 * for anything but really small edits now ( graphic file editing is vastly superior ).
 * Used to be the only means for editing maps before the 12-6-00 release of this code.
 * Caution is warranted if using the floodfill option - trying to floodfill a section that
 * is too large will overflow the memory in short order and cause a rather uncool crash.
 */
CMDF( do_mapedit )
{
   std::string arg1;
   int value;

#ifdef MULTIPORT
   if( mud_port == MAINPORT )
   {
      ch->print( "This command is not available on this port.\r\n" );
      return;
   }
#endif

   if( ch->isnpc(  ) )
   {
      ch->print( "Sorry, NPCs can't edit the overland maps.\r\n" );
      return;
   }

   argument = one_argument( argument, arg1 );

   if( !str_cmp( arg1, "help" ) )
   {
      ch->print( "Usage: mapedit save <mapname>\r\n" );
      ch->print( "Usage: mapedit vnum <mapname> <room_number>\r\n" );
      ch->print( "Usage: mapedit png <mapname> <png_filename>\r\n" );
      ch->print( "Usage: mapedit delete <mapname>\r\n\r\n" );

      ch->print( "The following options require you to be on the map you intend to edit:\r\n\r\n" );

      ch->print( "Usage: mapedit sector <sectortype>\r\n" );
      ch->print( "Usage: mapedit fill <sectortype>\r\n" );
      ch->print( "Usage: mapedit undo\r\n" );
      ch->print( "Usage: mapedit reload\r\n" );
      return;
   }

   if( !str_cmp( arg1, "save" ) )
   {
      continent_data *continent = find_continent_by_name( argument );

      if( !continent )
      {
         ch->print_fmt( "There is no such map as '{}'.\r\n", argument );
         return;
      }

      ch->print_fmt( "Saving map of {}...\r\n", continent->name );
      continent->save( );

      if( continent->nogrid == false )
         continent->save_png_file( );
      return;
   }

   if( !str_cmp( arg1, "delete" ) )
   {
      continent_data *continent;
      std::string arg2;

      argument = one_argument( argument, arg2 );
      
      if( str_cmp( arg2, "confirm" ) )
      {
         continent = find_continent_by_name( arg2 );

         if( !continent )
         {
            ch->print_fmt( "There is no continent named {}.\r\n", continent->name );
            return;
         }

         ch->print( "&RSTOP!!!\r\n\r\n" );
         ch->print( "&YThe operation you are requesting is highly disruptive and could lead to a game crash!\r\n" );
         ch->print_fmt( "You are requesting the deletion of continent data for: {}\r\n", continent->name );
         ch->print( "The following files will be permanently removed from the game:\r\n\r\n" );
         ch->print_fmt( "Continent file: {}\r\n", continent->filename );
         ch->print_fmt( "The associated area file: {}\r\n", continent->areafile );

         if( continent->nogrid == false )
         {
            ch->print_fmt( "The associated .png file: {}\r\n", continent->mapfile );
            ch->print_fmt( "{} entrance points stored in the continent data.\r\n", continent->exits.size() );
            ch->print_fmt( "{} landmarks stored in the continent file.\r\n", continent->landmarks.size() );
            ch->print_fmt( "{} skyship landing sites in the continent file.\r\n", continent->landing_sites.size() );
         }

         ch->print( "If you need to make backups of any of the above, please do so before committing to this operation.\r\n" );
         ch->print_fmt( "To confirm your intent, you must enter the following command: &Rmapedit delete confirm {}\r\n", continent->name );
         ch->print( "&YOnce this is done, the files will be removed. The in-game data will not be fully affected until the next reboot.&D\r\n" );
         return;
      }

      continent = find_continent_by_name( argument );

      if( !continent )
      {
         ch->print_fmt( "&YWhat are you even doing? There is no continent called {}.&D\r\n", argument );
         return;
      }

      if( ch->continent == continent )
      {
         ch->print( "&YYou need to get off this map before you can delete it.&D\r\n" );
         return;
      }

      ch->print_fmt( "&RHold on to your butts! Deletion of '{}' has begun!&D\r\n", continent->name );

      std::filesystem::path file_name = std::format( "{}{}", MAP_DIR, continent->filename );
      std::filesystem::path area_file = continent->areafile;

      // Ordinarily not a good idea to destroy an area file like this but the file should only ever be used for the map's purposes.
      std::filesystem::remove( area_file );
      write_area_list();
      web_arealist();

      std::filesystem::remove( file_name );
      if( continent->nogrid == false )
      {
         std::filesystem::path map_file = std::format( "{}{}", MAP_DIR, continent->mapfile );
         std::filesystem::remove( map_file );
      }

      /*
       * This is where it gets dicey cause we're gonna remove the continent data from the list.
       * We're not going to delete the data from memory so that those currently playing will have a chance
       * to get off of the map and into a regular room (or onto a different map). When the MUD reboots
       * the data files won't be loaded from storage and anyone logging in who was last seen here will just get
       * sent to a safe room.
       */
      continent_list.remove( continent );

      // Write out the list with the deleted one removed. If things crash, then at least it won't try to reload that on boot.
      write_continent_list();

      // If we're still here, cool. Hopefully everything worked out?
      ch->print_fmt( "&YWell. You're seeing this message. Continent {} has been deleted from the game.&D\r\n", argument );
      return;
   }

   if( !str_cmp( arg1, "vnum" ) )
   {
      room_index *room;
      area_data *area;
      std::string arg2;
      int vnum;

      argument = one_argument( argument, arg2 );
      continent_data *continent = find_continent_by_name( arg2 );

      if( !continent )
      {
         ch->print_fmt( "There is no such map as '{}'.\r\n", arg2 );
         return;
      }

      vnum = std::stoi( argument );

      if( !( room = get_room_index( vnum ) ) )
      {
         ch->print_fmt( "Room vnum {} does not exist.\r\n", vnum );
         return;
      }

      if( !( area = find_area( continent->areafile ) ) )
      {
         ch->print_fmt( "The area associated with this continent seems to be missing? Looking for: {}.\r\n", continent->areafile );
         return;
      }

      if( vnum < area->low_vnum || vnum > area->hi_vnum )
      {
         ch->print_fmt( "{} is not within the vnum assignment for {}. Valid range is: {} to {}.\r\n", vnum, continent->areafile, area->low_vnum, area->hi_vnum );
         return;
      }

      continent->vnum = vnum;
      continent->save( );

      ch->print_fmt( "{} has been assigned vnum {}.\r\n", continent->name, vnum );
      return;
   }

   if( !str_cmp( arg1, "png" ) )
   {
      std::string arg2;

      argument = one_argument( argument, arg2 );
      continent_data *continent = find_continent_by_name( arg2 );

      if( !continent )
      {
         ch->print_fmt( "There is no such map as '{}'.\r\n", arg2 );
         return;
      }

      if( continent->nogrid == false )
      {
         ch->print_fmt( "{} already has an assigned png file: {}. This cannot be changed once set.\r\n", continent->name, continent->mapfile );
         return;
      }

      if( argument.empty() )
      {
         ch->print( "No png filename was specified.\r\n" );
         return;
      }

      if( !str_cmp( argument, ".png" ) )
      {
         ch->print( "The filename must be more than just the .png extension.\r\n" );
         return;
      }

      std::filesystem::path file_name = std::format( "{}{}", MAP_DIR, argument );

      if( std::filesystem::exists( file_name ) )
      {
         continent_data *con_check = find_continent_by_pngfile( argument );

         if( con_check )
         {
            ch->print_fmt( "{} is already using {} for its map.\r\n", con_check->name, argument );
            return;
         }

         ch->print_fmt( "Assigning file {} to {}.\r\n", argument, continent->name );

         for( short x = 0; x < MAX_X; ++x )
         {
            for( short y = 0; y < MAX_Y; ++y )
               continent->putterr( x, y, SECT_OCEAN );
         }

         continent->mapfile = argument;
         continent->load_png_file( );
      }
      else
      {
         ch->print_fmt( "&GInitializing new map grid for {}...\r\n", continent->name );

         for( short x = 0; x < MAX_X; ++x )
         {
            for( short y = 0; y < MAX_Y; ++y )
               continent->putterr( x, y, SECT_OCEAN );
         }

         continent->mapfile = argument;
         continent->save_png_file( );
      }

      continent->nogrid = false;

      ch->print_fmt( "Saving map of {}...\r\n", continent->name );
      continent->save( );

      ch->print_fmt( "Map file {} has been assigned to {}.\r\n", argument, continent->name );
      return;
   }

   if( !ch->has_pcflag( PCFLAG_ONMAP ) )
   {
      ch->print( "That option can only be used from an overland map.\r\n" );
      return;
   }

   if( arg1.empty(  ) )
   {
      if( ch->has_pcflag( PCFLAG_MAPEDIT ) )
      {
         ch->unset_pcflag( PCFLAG_MAPEDIT );
         ch->print( "&GMap editing mode is now OFF. Don't forget to save your work!\r\n" );
         return;
      }

      ch->set_pcflag( PCFLAG_MAPEDIT );
      ch->print( "&RMap editing mode is now ON.\r\n" );
      ch->print_fmt( "&YYou are currently creating {} sectors.&z\r\n", sect_types[ch->pcdata->secedit] );
      return;
   }

   if( !str_cmp( arg1, "reload" ) )
   {
      if( str_cmp( argument, "confirm" ) )
      {
         ch->print( "This is a dangerous command if used improperly.\r\n" );
         ch->print_fmt( "You would be reloading the map for {}\r\n", ch->continent->name );
         ch->print( "Are you sure about this? Confirm by typing: mapedit reload confirm\r\n" );
         return;
      }
      reload_map( ch );
      return;
   }

   if( !str_cmp( arg1, "fill" ) )
   {
      int flood, fill;
      short standingon = ch->continent->get_terrain( ch->map_x, ch->map_y );

      if( argument.empty(  ) )
      {
         ch->print( "Floodfill with what???\r\n" );
         return;
      }

      if( standingon == -1 )
      {
         ch->print( "Unable to process floodfill. Your coordinates are invalid.\r\n" );
         return;
      }

      flood = get_sectypes( argument );
      if( flood < 0 || flood >= SECT_MAX )
      {
         ch->print( "Invalid sector type.\r\n" );
         return;
      }

      purgeundo(  );
      fill = ch->continent->floodfill( ch->map_x, ch->map_y, flood, standingon );

      if( fill == 0 )
      {
         display_map( ch );
         ch->print_fmt( "&RFooodfill with {} sectors successful.\r\n", argument );
         return;
      }

      if( fill == 1 )
      {
         ch->print( "Cannot floodfill identical terrain type!\r\n" );
         return;
      }
      ch->print( "Unknown error during floodfill.\r\n" );
      return;
   }

   if( !str_cmp( arg1, "undo" ) )
   {
      int undo = ch->continent->unfloodfill(  );

      if( undo == 0 )
      {
         display_map( ch );
         ch->print( "&RUndo successful.\r\n" );
         return;
      }
      ch->print( "Unknown error during undo.\r\n" );
      return;
   }

   if( !str_cmp( arg1, "sector" ) )
   {
      value = get_sectypes( argument );
      if( value < 0 || value > SECT_MAX )
      {
         ch->print( "Invalid sector type.\r\n" );
         return;
      }

      if( !str_cmp( argument, "exit" ) )
      {
         ch->print( "You cannot place exits this way.\r\n" );
         ch->print( "Please use the setexit command for this.\r\n" );
         return;
      }

      ch->pcdata->secedit = value;
      ch->print_fmt( "&YYou are now creating {} sectors.\r\n", argument );
      return;
   }

   ch->print( "Usage: mapedit sector <sectortype>\r\n" );
   ch->print( "Usage: mapedit save <mapname>\r\n" );
   ch->print( "Usage: mapedit fill <sectortype>\r\n" );
   ch->print( "Usage: mapedit undo\r\n" );
   ch->print( "Usage: mapedit reload\r\n\r\n" );
}

/* Online OLC map editing stuff ends here */
