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
 *	                     Overland ANSI Map Module                         *
 *                      Created by Samson of Alsherok                       *
 ****************************************************************************/

#include <fstream>
#include <gd.h>
#include <cmath>
#include "mud.h"
#include "area.h"
#include "mobindex.h"
#include "mud_prog.h"
#include "overland.h"
#include "roomindex.h"
#include "ships.h"
#include "skyship.h"

bool survey_environment( char_data * );
void load_landing_sites(  );
landing_data *check_landing_site( short, short, short );

list < landmark_data * >landmarklist;
list < mapexit_data * >mapexitlist;

/* Gee, I got bored and added a bunch of new comments to things. Interested parties know what to look for. 
 * Reorganized the placement of many things so that they're grouped together better.
 */

unsigned char map_sector[MAP_MAX][MAX_X][MAX_Y];   /* Initializes the sector array */

const char *map_filenames[] = {
   "one.png"
};

const char *map_names[] = {
   "One"
};

const char *map_name[] = {
   "one"
};

/* The names of varous sector types. Used in the OLC code in addition to
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

/* The symbol table. 
 * First entry is the sector type. 
 * Second entry is the color used to display it.
 * Third entry is the symbol used to represent it.
 * Fourth entry is the description of the terrain.
 * Fifth entry determines weather or not the terrain can be walked on.
 * Sixth entry is the amount of movement used up travelling on the terrain.
 * Last 3 entries are the RGB values for each type of terrain used to generate the graphic files.
 */
const struct sect_color_type sect_show[] = {
/*   Sector Type		Color	Symbol Description	Passable?	Move  R  G  B	*/

   {SECT_INDOORS, "&x", " ", "indoors", false, 1, 0, 0, 0},
   {SECT_CITY, "&Y", ":", "city", true, 1, 255, 128, 64},
   {SECT_FIELD, "&G", "+", "field", true, 1, 141, 215, 1},
   {SECT_FOREST, "&g", "+", "forest", true, 2, 0, 108, 47},
   {SECT_HILLS, "&O", "^", "hills", true, 3, 140, 102, 54},
   {SECT_MOUNTAIN, "&w", "^", "mountain", true, 5, 152, 152, 152},
   {SECT_WATER_SWIM, "&C", "~", "shallow water", true, 2, 89, 242, 251},
   {SECT_WATER_NOSWIM, "&B", "~", "deep water", true, 2, 67, 114, 251},
   {SECT_AIR, "&x", "?", "air", false, 1, 0, 0, 0},
   {SECT_UNDERWATER, "&x", "?", "underwater", false, 5, 0, 0, 0},
   {SECT_DESERT, "&Y", "~", "desert", true, 3, 241, 228, 145},
   {SECT_RIVER, "&B", "~", "river", true, 3, 0, 0, 255},
   {SECT_OCEANFLOOR, "&x", "?", "ocean floor", false, 4, 0, 0, 0},
   {SECT_UNDERGROUND, "&x", "?", "underground", false, 3, 0, 0, 0},
   {SECT_JUNGLE, "&g", "*", "jungle", true, 2, 70, 149, 52},
   {SECT_SWAMP, "&g", "~", "swamp", true, 3, 218, 176, 56},
   {SECT_TUNDRA, "&C", "-", "tundra", true, 2, 54, 255, 255},
   {SECT_ICE, "&W", "=", "ice", true, 3, 133, 177, 252},
   {SECT_OCEAN, "&b", "~", "ocean", false, 1, 0, 0, 128},
   {SECT_LAVA, "&R", ":", "lava", false, 2, 245, 37, 29},
   {SECT_SHORE, "&Y", ".", "shoreline", true, 3, 255, 255, 0},
   {SECT_TREE, "&g", "^", "impass forest", false, 10, 0, 64, 0},
   {SECT_STONE, "&W", "^", "impas mountain", false, 10, 128, 128, 128},
   {SECT_QUICKSAND, "&g", "%", "quicksand", false, 10, 128, 128, 0},
   {SECT_WALL, "&P", "I", "wall", false, 10, 255, 0, 255},
   {SECT_GLACIER, "&W", "=", "glacier", false, 10, 141, 207, 244},
   {SECT_EXIT, "&W", "#", "exit", true, 1, 255, 255, 255},
   {SECT_TRAIL, "&O", ":", "trail", true, 1, 128, 64, 0},
   {SECT_BLANDS, "&r", ".", "blasted lands", true, 2, 128, 0, 0},
   {SECT_GRASSLAND, "&G", ".", "grassland", true, 1, 83, 202, 2},
   {SECT_SCRUB, "&g", ".", "scrub", true, 2, 123, 197, 112},
   {SECT_BARREN, "&O", ".", "barren", true, 2, 192, 192, 192},
   {SECT_BRIDGE, "&P", ":", "bridge", true, 1, 255, 0, 128},
   {SECT_ROAD, "&Y", ":", "road", true, 1, 215, 107, 0},
   {SECT_LANDING, "&R", "#", "landing", true, 1, 255, 0, 0}
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

/* The array of predefined mobs that the check_random_mobs function uses.
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
    11030, 11031, 11032, 11033, -1,
    -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1},
   /*
    * Mobs for SECT_FIELD 
    */
   {-1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1,
    11034, 11035, 11036, 11037, -1,
    -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1},
   /*
    * Mobs for SECT_FOREST 
    */
   {-1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1,
    11038, 11039, 11040, 11041, -1,
    -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1},
   /*
    * Mobs for SECT_HILLS 
    */
   {-1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1,
    11042, 11043, 11044, 11045, -1,
    -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1},
   /*
    * Mobs for SECT_MOUNTAIN 
    */
   {-1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1,
    11046, 11047, 4900, 11048, -1,
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
    11049, 11050, 4654, 11051, -1,
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
    11054, 11055, 11056, 11057, -1,
    -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1},
   /*
    * Mobs for SECT_SWAMP 
    */
   {-1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1,
    11052, 11053, 25111, 5393, -1,
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
    11058, 11059, -1, -1, -1,
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
    11060, 11061, 11062, 11063, -1,
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

/* Simply changes the sector type for the specified coordinates */
void putterr( short map, short x, short y, short terr )
{
   map_sector[map][x][y] = terr;
}

/* Alrighty - this checks where the PC is currently standing to see what kind of terrain the space is.
 * Returns -1 if something is not kosher with where they're standing.
 * Called from several places below, so leave this right here.
 */
short get_terrain( short map, short x, short y )
{
   short terrain;

   if( map == -1 )
      return -1;

   if( x < 0 || x >= MAX_X || y < 0 || y >= MAX_Y )
      return -1;

   terrain = map_sector[map][x][y];

   return terrain;
}

/* Used with the survey command to calculate distance to the landmark.
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
bool is_same_char_map( char_data * ch, char_data * victim )
{
   if( victim->cmap != ch->cmap || victim->mx != ch->mx || victim->my != ch->my )
      return false;
   return true;
}

bool is_same_obj_map( char_data * ch, obj_data * obj )
{
   if( !obj->extra_flags.test( ITEM_ONMAP ) )
   {
      if( ch->has_pcflag( PCFLAG_ONMAP ) )
         return false;
      if( ch->has_actflag( ACT_ONMAP ) )
         return false;
      return true;
   }

   if( ch->cmap != obj->cmap || ch->mx != obj->mx || ch->my != obj->my )
      return false;
   return true;
}

/* Will set the vics map the same as the characters map
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
      bug( "%s: NULL victim!", __FUNCTION__ );
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
   victim->cmap = ch->cmap;
   victim->mx = ch->mx;
   victim->my = ch->my;
}

/* Overland landmark stuff starts here */
landmark_data::landmark_data(  )
{
   init_memory( &distance, &Isdesc, sizeof( Isdesc ) );
}

landmark_data::~landmark_data(  )
{
   landmarklist.remove( this );
}

void load_landmarks( void )
{
   landmark_data *landmark;
   ifstream stream;

   landmarklist.clear(  );

   stream.open( LANDMARK_FILE );
   if( !stream.is_open(  ) )
      return;

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

      if( key == "#LANDMARK" )
         landmark = new landmark_data;
      else if( key == "Coordinates" )
      {
         string arg;

         value = one_argument( value, arg );
         landmark->cmap = atoi( arg.c_str(  ) );

         value = one_argument( value, arg );
         landmark->mx = atoi( arg.c_str(  ) );

         value = one_argument( value, arg );
         landmark->my = atoi( arg.c_str(  ) );

         landmark->distance = atoi( value.c_str(  ) );
      }
      else if( key == "Description" )
         landmark->description = value;
      else if( key == "Isdesc" )
         landmark->Isdesc = atoi( value.c_str(  ) );
      else if( key == "End" )
         landmarklist.push_back( landmark );
      else
         log_printf( "%s: Bad line in landmark data file: %s %s", __FUNCTION__, key.c_str(  ), value.c_str(  ) );
   }
   while( !stream.eof(  ) );
   stream.close(  );
}

void save_landmarks( void )
{
   ofstream stream;

   stream.open( LANDMARK_FILE );
   if( !stream.is_open(  ) )
   {
      bug( "%s: fopen", __FUNCTION__ );
      perror( LANDMARK_FILE );
   }
   else
   {
      list < landmark_data * >::iterator imark;

      for( imark = landmarklist.begin(  ); imark != landmarklist.end(  ); ++imark )
      {
         landmark_data *landmark = *imark;

         stream << "#LANDMARK" << endl;
         stream << "Coordinates " << landmark->cmap << " " << landmark->mx << " " << landmark->my << " " << landmark->distance << endl;
         stream << "Description " << landmark->description << endl;
         stream << "Isdesc      " << landmark->Isdesc << endl;
         stream << "End" << endl << endl;
      }
      stream.close(  );
   }
}

landmark_data *check_landmark( short cmap, short x, short y )
{
   list < landmark_data * >::iterator imark;

   for( imark = landmarklist.begin(  ); imark != landmarklist.end(  ); ++imark )
   {
      landmark_data *landmark = *imark;

      if( landmark->cmap == cmap )
      {
         if( landmark->mx == x && landmark->my == y )
            return landmark;
      }
   }
   return NULL;
}

void add_landmark( short cmap, short x, short y )
{
   landmark_data *landmark;

   landmark = new landmark_data;
   landmark->cmap = cmap;
   landmark->mx = x;
   landmark->my = y;
   landmarklist.push_back( landmark );

   save_landmarks(  );
}

void delete_landmark( landmark_data * landmark )
{
   if( !landmark )
   {
      bug( "%s: Trying to delete NULL landmark!", __FUNCTION__ );
      return;
   }

   deleteptr( landmark );

   if( !mud_down )
      save_landmarks(  );
}

void free_landmarks( void )
{
   list < landmark_data * >::iterator land;

   for( land = landmarklist.begin(  ); land != landmarklist.end(  ); )
   {
      landmark_data *landmark = *land;
      ++land;

      delete_landmark( landmark );
   }
}

/* Landmark survey module - idea snarfed from Medievia and adapted to Smaug by Samson - 8-19-00 */
CMDF( do_survey )
{
   list < landmark_data * >::iterator imark;
   double dist, angle;
   int dir = -1, iMes = 0;
   bool found = false, env = false;

   if( !ch )
      return;

   for( imark = landmarklist.begin(  ); imark != landmarklist.end(  ); ++imark )
   {
      landmark_data *landmark = *imark;

      /*
       * No point in bothering if its not even on this map 
       */
      if( ch->cmap != landmark->cmap )
         continue;

      if( landmark->Isdesc )
         continue;

      dist = distance( ch->mx, ch->my, landmark->mx, landmark->my );

      /*
       * Save the math if it's too far away anyway 
       */
      if( dist <= landmark->distance )
      {
         found = true;

         angle = calc_angle( ch->mx, ch->my, landmark->mx, landmark->my, &dist );

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
            ch->printf( "Right here nearby, %s.\r\n", !landmark->description.empty(  )? landmark->description.c_str(  ) : "BUG! Please report!" );
         else
            ch->printf( "To the %s, %s, %s.\r\n", dir_name[dir], landmark_distances[iMes],
                        !landmark->description.empty(  )? landmark->description.c_str(  ) : "<BUG! Inform the Immortals>" );

         if( ch->is_immortal(  ) )
         {
            ch->printf( "Distance to landmark: %d\r\n", ( int )dist );
            ch->printf( "Landmark coordinates: %dX %dY\r\n", landmark->mx, landmark->my );
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
   list < landmark_data * >::iterator imark;

   if( landmarklist.empty(  ) )
   {
      ch->print( "No landmarks defined.\r\n" );
      return;
   }

   ch->pager( "Continent | Coordinates | Distance | Description\r\n" );
   ch->pager( "-----------------------------------------------------------\r\n" );

   for( imark = landmarklist.begin(  ); imark != landmarklist.end(  ); ++imark )
   {
      landmark_data *landmark = *imark;

      ch->pagerf( "%-10s  %-4dX %-4dY   %-4d       %s\r\n", map_names[landmark->cmap], landmark->mx, landmark->my, landmark->distance, landmark->description.c_str(  ) );
   }
}

/* OLC command to add/delete/edit landmark information */
CMDF( do_setmark )
{
   landmark_data *landmark = NULL;
   string arg;

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

   switch ( ch->substate )
   {
      default:
         break;

      case SUB_RESTRICTED:
         ch->print( "You cannot do this while in another command.\r\n" );
         return;

      case SUB_OVERLAND_DESC:
         landmark = ( landmark_data * ) ch->pcdata->dest_buf;
         if( !landmark )
            bug( "%s: setmark desc: sub_overland_desc: NULL ch->pcdata->dest_buf", __FUNCTION__ );
         landmark->description = ch->copy_buffer(  );
         ch->stop_editing(  );
         ch->substate = ch->tempnum;
         save_landmarks(  );
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

   landmark = check_landmark( ch->cmap, ch->mx, ch->my );

   if( !str_cmp( arg, "add" ) )
   {
      if( landmark )
      {
         ch->print( "There's already a landmark at this location.\r\n" );
         return;
      }
      add_landmark( ch->cmap, ch->mx, ch->my );
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
      delete_landmark( landmark );
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
      save_landmarks(  );

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

      value = atoi( argument.c_str(  ) );

      if( value < 1 )
      {
         ch->print( "Distance must be at least 1.\r\n" );
         return;
      }
      landmark->distance = value;
      save_landmarks(  );
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
mapexit_data::mapexit_data(  )
{
   init_memory( &vnum, &prevsector, sizeof( prevsector ) );
}

mapexit_data::~mapexit_data(  )
{
   mapexitlist.remove( this );
}

void load_mapexits( void )
{
   mapexit_data *mexit;
   ifstream stream;

   mapexitlist.clear(  );

   stream.open( ENTRANCE_FILE );
   if( !stream.is_open(  ) )
      return;

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

      if( key == "#ENTRANCE" )
      {
         mexit = new mapexit_data;
         mexit->prevsector = SECT_OCEAN;  // Default for legacy exits
      }
      else if( key == "ToMap" )
         mexit->tomap = atoi( value.c_str(  ) );
      else if( key == "OnMap" )
         mexit->onmap = atoi( value.c_str(  ) );
      else if( key == "Here" )
      {
         string arg;

         value = one_argument( value, arg );
         mexit->herex = atoi( arg.c_str(  ) );

         mexit->herey = atoi( value.c_str(  ) );
      }
      else if( key == "There" )
      {
         string arg;

         value = one_argument( value, arg );
         mexit->therex = atoi( arg.c_str(  ) );

         mexit->therey = atoi( value.c_str(  ) );
      }
      else if( key == "Vnum" )
         mexit->vnum = atoi( value.c_str(  ) );
      else if( key == "Prevsector" )
         mexit->prevsector = atoi( value.c_str(  ) );
      else if( key == "Area" )
         mexit->area = value;
      else if( key == "End" )
         mapexitlist.push_back( mexit );
      else
         log_printf( "%s: Bad line in overland exists file: %s %s", __FUNCTION__, key.c_str(  ), value.c_str(  ) );
   }
   while( !stream.eof(  ) );
   stream.close(  );
}

void save_mapexits( void )
{
   ofstream stream;

   stream.open( ENTRANCE_FILE );
   if( !stream.is_open(  ) )
   {
      bug( "%s: fopen", __FUNCTION__ );
      perror( ENTRANCE_FILE );
   }
   else
   {
      list < mapexit_data * >::iterator iexit;

      for( iexit = mapexitlist.begin(  ); iexit != mapexitlist.end(  ); ++iexit )
      {
         mapexit_data *mexit = *iexit;

         stream << "#ENTRANCE" << endl;
         stream << "ToMap      " << mexit->tomap << endl;
         stream << "OnMap      " << mexit->onmap << endl;
         stream << "Here       " << mexit->herex << " " << mexit->herey << endl;
         stream << "There      " << mexit->therex << " " << mexit->therey << endl;
         stream << "Vnum       " << mexit->vnum << endl;
         stream << "Prevsector " << mexit->prevsector << endl;
         if( !mexit->area.empty(  ) )
            stream << "Area       " << mexit->area << endl;
         stream << "End" << endl << endl;
      }
      stream.close(  );
   }
}

mapexit_data *check_mapexit( short map, short x, short y )
{
   list < mapexit_data * >::iterator iexit;

   for( iexit = mapexitlist.begin(  ); iexit != mapexitlist.end(  ); ++iexit )
   {
      mapexit_data *mexit = *iexit;

      if( mexit->onmap == map )
      {
         if( mexit->herex == x && mexit->herey == y )
            return mexit;
      }
   }
   return NULL;
}

void modify_mapexit( mapexit_data * mexit, short tomap, short onmap, short hereX, short hereY, short thereX, short thereY, int vnum, const string & area )
{
   if( !mexit )
   {
      bug( "%s: NULL exit being modified!", __FUNCTION__ );
      return;
   }

   mexit->tomap = tomap;
   mexit->onmap = onmap;
   mexit->herex = hereX;
   mexit->herey = hereY;
   mexit->therex = thereX;
   mexit->therey = thereY;
   mexit->vnum = vnum;
   if( !area.empty(  ) )
      mexit->area = area;

   save_mapexits(  );
}

void add_mapexit( short tomap, short onmap, short hereX, short hereY, short thereX, short thereY, int vnum )
{
   mapexit_data *mexit;

   mexit = new mapexit_data;
   mexit->tomap = tomap;
   mexit->onmap = onmap;
   mexit->herex = hereX;
   mexit->herey = hereY;
   mexit->therex = thereX;
   mexit->therey = thereY;
   mexit->vnum = vnum;
   mexit->prevsector = get_terrain( onmap, hereX, hereY );
   mapexitlist.push_back( mexit );

   save_mapexits(  );
}

void delete_mapexit( mapexit_data * mexit )
{
   if( !mexit )
   {
      bug( "%s: Trying to delete NULL exit!", __FUNCTION__ );
      return;
   }

   deleteptr( mexit );

   if( !mud_down )
      save_mapexits(  );
}

void free_mapexits( void )
{
   list < mapexit_data * >::iterator en;

   for( en = mapexitlist.begin(  ); en != mapexitlist.end(  ); )
   {
      mapexit_data *mexit = *en;
      ++en;

      delete_mapexit( mexit );
   }
}

/* OLC command to add/delete/edit overland exit information */
CMDF( do_setexit )
{
   string arg;
   room_index *location;
   mapexit_data *mexit = NULL;
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
      ch->print( "Usage: setexit <vnum>\r\n" );
      ch->print( "Usage: setexit <area>\r\n" );
      ch->print( "Usage: setexit map <mapname> <X-coord> <Y-coord>\r\n" );
      return;
   }

   mexit = check_mapexit( ch->cmap, ch->mx, ch->my );

   if( !str_cmp( arg, "create" ) )
   {
      if( mexit )
      {
         ch->print( "An exit already exists at these coordinates.\r\n" );
         return;
      }

      add_mapexit( ch->cmap, ch->cmap, ch->mx, ch->my, ch->mx, ch->my, -1 );
      putterr( ch->cmap, ch->mx, ch->my, SECT_EXIT );
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
      putterr( ch->cmap, ch->mx, ch->my, mexit->prevsector );
      delete_mapexit( mexit );
      ch->print( "Exit deleted.\r\n" );
      return;
   }

   if( !str_cmp( arg, "map" ) )
   {
      string arg2, arg3;
      short x, y, map = -1;

      if( ch->cmap == -1 )
      {
         bug( "%s: %s is not on a valid map!", __FUNCTION__, ch->name );
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

      if( !str_cmp( arg2, "one" ) )
         map = ACON_ONE;

      if( map == -1 )
      {
         ch->printf( "There isn't a map for '%s'.\r\n", arg2.c_str(  ) );
         return;
      }

      x = atoi( arg3.c_str(  ) );
      y = atoi( argument.c_str(  ) );

      if( x < 0 || x >= MAX_X )
      {
         ch->printf( "Valid x coordinates are 0 to %d.\r\n", MAX_X - 1 );
         return;
      }

      if( y < 0 || y >= MAX_Y )
      {
         ch->printf( "Valid y coordinates are 0 to %d.\r\n", MAX_Y - 1 );
         return;
      }

      modify_mapexit( mexit, map, ch->cmap, ch->mx, ch->my, x, y, -1, NULL );
      putterr( ch->cmap, ch->mx, ch->my, SECT_EXIT );
      ch->printf( "Exit set to map of %s, at %dX, %dY.\r\n", arg2.c_str(  ), x, y );
      return;
   }

   if( !str_cmp( arg, "area" ) )
   {
      if( argument.empty(  ) )
      {
         do_setexit( ch, "" );
         return;
      }

      modify_mapexit( mexit, mexit->onmap, ch->cmap, ch->mx, ch->my, mexit->therex, mexit->therey, mexit->vnum, argument );
      ch->printf( "Exit identified for area: %s\r\n", argument.c_str(  ) );
      return;
   }

   vnum = atoi( arg.c_str(  ) );

   if( !( location = get_room_index( vnum ) ) )
   {
      ch->print( "No such room exists.\r\n" );
      return;
   }

   modify_mapexit( mexit, -1, ch->cmap, ch->mx, ch->my, -1, -1, vnum, "" );
   putterr( ch->cmap, ch->mx, ch->my, SECT_EXIT );
   ch->printf( "Exit set to room %d.\r\n", vnum );
}

/* Overland exit stuff ends here */

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

bool load_oldmapfile( const char *mapfile, short mapnumber )
{
   FILE *fp;
   char filename[256];
   short graph1, graph2, graph3, x, y, z;
   short terr = SECT_OCEAN;

   log_printf( "Attempting RAW file conversion of %s...", map_names[mapnumber] );

   snprintf( filename, 256, "%s%s", MAP_DIR, mapfile );

   if( !( fp = fopen( filename, "r" ) ) )
   {
      log_string( "Conversion failed!" );
      return false;
   }

   for( y = 0; y < MAX_Y; ++y )
   {
      for( x = 0; x < MAX_X; ++x )
      {
         graph1 = getc( fp );
         graph2 = getc( fp );
         graph3 = getc( fp );

         for( z = 0; z < SECT_MAX; ++z )
         {
            if( sect_show[z].graph1 == graph1 && sect_show[z].graph2 == graph2 && sect_show[z].graph3 == graph3 )
            {
               terr = z;
               break;
            }
            terr = SECT_OCEAN;
         }
         putterr( mapnumber, x, y, terr );
      }
   }
   FCLOSE( fp );

   log_string( "Conversion successful." );
   return true;
}

/* As it implies, loads the map from the graphic file */
void load_map_png( const char *mapfile, short mapnumber )
{
   FILE *jpgin;
   char filename[256];
   gdImagePtr im;

   log_printf( "Loading continent of %s.....", map_names[mapnumber] );

   snprintf( filename, 256, "%s%s", MAP_DIR, mapfile );

   if( !( jpgin = fopen( filename, "r" ) ) )
   {
      char oldfile[256];

      snprintf( oldfile, 256, "%s%s.raw", MAP_DIR, map_name[mapnumber] );
      if( !load_oldmapfile( oldfile, mapnumber ) )
      {
         bug( "%s: Missing graphical map file %s for continent!", __FUNCTION__, mapfile );
         shutdown_mud( "Missing map file" );
         exit( 1 );
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
         putterr( mapnumber, x, y, terr );
      }
   }
   FCLOSE( jpgin );
   gdImageDestroy( im );
}

/* Called from db.c - loads up the map files at bootup */
void load_maps( void )
{
   short x, y;

   log_string( "Initializing map grid array...." );
   for( short map = 0; map < MAP_MAX; ++map )
   {
      for( x = 0; x < MAX_X; ++x )
      {
         for( y = 0; y < MAX_Y; ++y )
         {
            putterr( map, x, y, SECT_OCEAN );
         }
      }
   }

   /*
    * My my Samson, aren't you getting slick.... 
    */
   for( x = 0; x < MAP_MAX; ++x )
      load_map_png( map_filenames[x], x );

   log_string( "Loading overland map exits...." );
   load_mapexits(  );

   log_string( "Loading overland landmarks...." );
   load_landmarks(  );

   log_string( "Loading landing sites...." );
   load_landing_sites(  );
}

/* The guts of the map display code. Streamlined to only change color codes when it needs to.
 * If you are also using my newly updated Custom Color code you'll get even better performance
 * out of it since that code replaced the stock Smaug color tag convertor, which shaved a few
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
 */
void new_map_to_char( char_data * ch, short startx, short starty, short endx, short endy, int radius )
{
   string secbuf;

   if( startx < 0 )
      startx = 0;

   if( starty < 0 )
      starty = 0;

   if( endx >= MAX_X )
      endx = MAX_X - 1;

   if( endy >= MAX_Y )
      endy = MAX_Y - 1;

   short lastsector = -1;
   secbuf.append( "\r\n" );

   for( short y = starty; y < endy + 1; ++y )
   {
      for( short x = startx; x < endx + 1; ++x )
      {
         if( distance( ch->mx, ch->my, x, y ) > radius )
         {
            if( !ch->has_pcflag( PCFLAG_HOLYLIGHT ) && !ch->in_room->flags.test( ROOM_WATCHTOWER ) && !ch->inflight )
            {
               secbuf += " ";
               lastsector = -1;
               continue;
            }
         }

         short sector = get_terrain( ch->cmap, x, y );
         bool other = false, npc = false, object = false, group = false, aship = false;
         list < char_data * >::iterator ich;
         for( ich = ch->in_room->people.begin(  ); ich != ch->in_room->people.end(  ); ++ich )
         {
            char_data *rch = *ich;

            if( x == rch->mx && y == rch->my && rch != ch && ( rch->mx != ch->mx || rch->my != ch->my ) )
            {
               if( rch->has_pcflag( PCFLAG_WIZINVIS ) && rch->pcdata->wizinvis > ch->level )
                  other = false;
               else
               {
                  other = true;
                  if( rch->isnpc(  ) )
                     npc = true;
                  lastsector = -1;
               }
            }

            if( is_same_group( ch, rch ) && ch != rch )
            {
               if( x == ch->mx && y == ch->my && is_same_char_map( ch, rch ) )
               {
                  group = true;
                  lastsector = -1;
               }
            }
         }

         list < obj_data * >::iterator iobj;
         for( iobj = ch->in_room->objects.begin(  ); iobj != ch->in_room->objects.end(  ); ++iobj )
         {
            obj_data *obj = *iobj;

            if( x == obj->mx && y == obj->my && !is_same_obj_map( ch, obj ) )
            {
               object = true;
               lastsector = -1;
            }
         }

         list < ship_data * >::iterator sh;
         for( sh = shiplist.begin(  ); sh != shiplist.end(  ); ++sh )
         {
            ship_data *ship = *sh;

            if( x == ship->mx && y == ship->my && ship->room == ch->in_room->vnum )
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

         if( x == ch->mx && y == ch->my && !aship )
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
               secbuf.append( sect_show[sector].symbol );
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

/* This function determines the size of the display to show to a character.
 * For immortals, it also shows any details present in the sector, like resets and landmarks.
 */
void display_map( char_data * ch )
{
   landmark_data *landmark = NULL;
   landing_data *landing = NULL;
   short startx, starty, endx, endy, sector;
   int mod = sysdata->mapsize;

   if( ch->cmap == -1 )
   {
      bug( "%s: Player %s on invalid map! Moving them to %s.", __FUNCTION__, ch->name, map_names[MAP_ONE] );
      ch->printf( "&RYou were found on an invalid map and have been moved to %s.\r\n", map_names[MAP_ONE] );
      enter_map( ch, NULL, 499, 500, ACON_ONE );
      return;
   }

   sector = get_terrain( ch->cmap, ch->mx, ch->my );

   if( ch->has_pcflag( PCFLAG_HOLYLIGHT ) || ch->in_room->flags.test( ROOM_WATCHTOWER ) || ch->inflight )
   {
      startx = ch->mx - 37;
      endx = ch->mx + 37;
      starty = ch->my - 14;
      endy = ch->my + 14;
   }
   else
   {
      obj_data *light;

      light = ch->get_eq( WEAR_LIGHT );

      if( time_info.hour == sysdata->hoursunrise || time_info.hour == sysdata->hoursunset )
         mod = 4;

      if( ( ch->in_room->area->weather->precip + 3 * weath_unit - 1 ) / weath_unit > 1 && mod != 1 )
         mod -= 1;

      if( time_info.hour > sysdata->hoursunset || time_info.hour < sysdata->hoursunrise )
         mod = 2;

      if( light != NULL )
      {
         if( light->item_type == ITEM_LIGHT && ( time_info.hour > sysdata->hoursunset || time_info.hour < sysdata->hoursunrise ) )
            mod += 1;
      }

      if( ch->has_aflag( AFF_WIZARDEYE ) )
         mod = sysdata->mapsize * 2;

      startx = ( UMAX( ( short )( ch->mx - ( mod * 1.5 ) ), ch->mx - 37 ) );
      starty = ( UMAX( ch->my - mod, ch->my - 14 ) );
      endx = ( UMIN( ( short )( ch->mx + ( mod * 1.5 ) ), ch->mx + 37 ) );
      endy = ( UMIN( ch->my + mod, ch->my + 14 ) );
   }

   if( ch->has_pcflag( PCFLAG_MAPEDIT ) && sector != SECT_EXIT )
   {
      putterr( ch->cmap, ch->mx, ch->my, ch->pcdata->secedit );
      sector = ch->pcdata->secedit;
   }

   new_map_to_char( ch, startx, starty, endx, endy, mod );

   if( !ch->inflight && !ch->in_room->flags.test( ROOM_WATCHTOWER ) )
   {
      ch->printf( "&GTravelling on the continent of %s.\r\n", map_names[ch->cmap] );
      landmark = check_landmark( ch->cmap, ch->mx, ch->my );

      if( landmark && landmark->Isdesc )
         ch->printf( "&G%s\r\n", !landmark->description.empty(  )? landmark->description.c_str(  ) : "" );
      else
         ch->printf( "&G%s\r\n", impass_message[sector] );
   }
   else if( ch->in_room->flags.test( ROOM_WATCHTOWER ) )
   {
      ch->printf( "&YYou are overlooking the continent of %s.\r\n", map_names[ch->cmap] );
      ch->print( "The view from this watchtower is amazing!\r\n" );
   }
   else
      ch->printf( "&GRiding a skyship over the continent of %s.\r\n", map_names[ch->cmap] );

   if( ch->is_immortal(  ) )
   {
      ch->printf( "&GSector type: %s. Coordinates: %dX, %dY\r\n", sect_types[sector], ch->mx, ch->my );

      landing = check_landing_site( ch->cmap, ch->mx, ch->my );

      if( landing )
         ch->printf( "&CLanding site for %s.\r\n", !landing->area.empty(  )? landing->area.c_str(  ) : "<NOT SET>" );

      if( landmark && !landmark->Isdesc )
      {
         ch->printf( "&BLandmark present: %s\r\n", !landmark->description.empty(  )? landmark->description.c_str(  ) : "<NO DESCRIPTION>" );
         ch->printf( "&BVisibility distance: %d.\r\n", landmark->distance );
      }

      if( ch->has_pcflag( PCFLAG_MAPEDIT ) )
         ch->printf( "&YYou are currently creating %s sectors.&z\r\n", sect_types[ch->pcdata->secedit] );
   }
}

/* Called in update.c modification for wandering mobiles - Samson 7-29-00
 * For mobs entering overland from normal zones:
 * If the sector of the origin room matches the terrain type on the map,
 *    it becomes the mob's native terrain.
 * If the origin room sector does NOT match, then the mob will assign itself the
 *    first terrain type it can move onto that is different from what it entered on.
 * This allows you to load mobs in a loading room, and have them stay on roads or
 *    trails to act as patrols. And also compensates for when a map road or trail
 *    enters a zone and becomes forest 
 */
bool map_wander( char_data * ch, short map, short x, short y, short sector )
{
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
   if( get_terrain( map, x, y ) == ch->sector || get_terrain( map, x, y ) == SECT_CITY || get_terrain( map, x, y ) == SECT_TRAIL )
      return true;

   /*
    * Sector -2 is used to tell the code this mob came to the overland from an internal
    * * zone, but the terrain didn't match the originating room's sector type. It'll assign 
    * * the first differing terrain upon moving, provided it isn't a SECT_ROAD. From then on 
    * * it will only wander in that type of terrain - Samson 7-29-00
    */
   if( ch->sector == -2 && get_terrain( map, x, y ) != sector && get_terrain( map, x, y ) != SECT_ROAD && sect_show[get_terrain( map, x, y )].canpass )
   {
      ch->sector = get_terrain( map, x, y );
      return true;
   }

   return false;
}

/* This function does a random check, currently set to 6%, to see if it should load a mobile
 * at the sector the PC just walked into. I have no doubt a cleaner, more efficient way can
 * be found to accomplish this, but until such time as divine inspiration hits me ( or some kind
 * soul shows me the light ) this is how its done. [WOO! Yes Geni, I think that's how its done :P]
 * 
 * Rewritten by Geni to use tables and way too many mobs.
 */
void check_random_mobs( char_data * ch )
{
   mob_index *imob = NULL;
   char_data *mob = NULL;
   int vnum = -1;
   int terrain = get_terrain( ch->cmap, ch->mx, ch->my );

   /*
    * This could ( and did ) get VERY messy
    * * VERY quickly if wandering mobs trigger more of themselves
    */
   if( ch->isnpc(  ) )
      return;

   if( number_percent(  ) < 6 )
      vnum = random_mobs[terrain][UMIN( 25, number_range( 0, ch->level / 2 ) )];

   if( vnum == -1 )
      return;
   if( !( imob = get_mob_index( vnum ) ) )
   {
      log_printf( "%s: Missing mob for vnum %d", __FUNCTION__, vnum );
      return;
   }

   mob = imob->create_mobile(  );
   if( !mob->to_room( ch->in_room ) )
      log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
   mob->set_actflag( ACT_ONMAP );
   mob->sector = terrain;
   mob->cmap = ch->cmap;
   mob->mx = ch->mx;
   mob->my = ch->my;
   mob->timer = 400;
   /*
    * This should be long enough. If not, increase. Mob will be extracted when it expires.
    * * And trust me, this is a necessary measure unless you LIKE having your memory flooded
    * * by random overland mobs.
    */
}

/* An overland hack of the scan command - the OLD scan command, not the one in stock Smaug
 * where you have to specify a direction to scan in. This is only called from within do_scan
 * and probably won't be of much use to you unless you want to modify your scan command to
 * do like ours. And hey, if you do, I'd be happy to send you our do_scan - separately.
 */
void map_scan( char_data * ch )
{
   if( !ch )
      return;

   obj_data *light = ch->get_eq( WEAR_LIGHT );

   int mod = sysdata->mapsize;
   if( time_info.hour == sysdata->hoursunrise || time_info.hour == sysdata->hoursunset )
      mod = 4;

   if( ( ch->in_room->area->weather->precip + 3 * weath_unit - 1 ) / weath_unit > 1 && mod != 1 )
      mod -= 1;

   if( time_info.hour > sysdata->hoursunset || time_info.hour < sysdata->hoursunrise )
      mod = 1;

   if( light != NULL )
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

   bool found = false;
   list < char_data * >::iterator ich;
   for( ich = ch->in_room->people.begin(  ); ich != ch->in_room->people.end(  ); ++ich )
   {
      char_data *gch = *ich;

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

      double dist = distance( ch->mx, ch->my, gch->mx, gch->my );

      /*
       * Save the math if they're too far away anyway 
       */
      if( dist <= mod )
      {
         double angle = calc_angle( ch->mx, ch->my, gch->mx, gch->my, &dist );
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
            ch->printf( "&[skill]Here with you: %s.\r\n", gch->name );
         else
            ch->printf( "&[skill]To the %s, %s, %s.\r\n", dir_name[dir], landmark_distances[iMes], gch->name );
      }
   }
   if( !found )
      ch->print( "Your survey of the area turns up nobody.\r\n" );
}

/* Note: For various reasons, this isn't designed to pull PC followers along with you */
void collect_followers( char_data * ch, room_index * from, room_index * to )
{
   if( !ch )
   {
      bug( "%s: NULL master!", __FUNCTION__ );
      return;
   }

   if( !from )
   {
      bug( "%s: %s NULL source room!", __FUNCTION__, ch->name );
      return;
   }

   if( !to )
   {
      bug( "%s: %s NULL target room!", __FUNCTION__, ch->name );
      return;
   }

   list < char_data * >::iterator ich;
   for( ich = from->people.begin(  ); ich != from->people.end(  ); )
   {
      char_data *fch = *ich;
      ++ich;

      if( fch != ch && fch->master == ch && ( fch->position == POS_STANDING || fch->position == POS_MOUNTED ) )
      {
         if( !fch->isnpc(  ) )
            continue;

         fch->from_room(  );
         if( !fch->to_room( to ) )
            log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
         fix_maps( ch, fch );
      }
   }
}

/* The guts of movement on the overland. Checks all sorts of nice things. Makes DAMN
 * sure you can actually go where you just tried to. Breaks easily if messed with wrong too.
 */
ch_ret process_exit( char_data * ch, short cmap, short x, short y, int dir, bool running )
{
   list < ship_data * >::iterator sh;

   /*
    * Cheap ass hack for now - better than nothing though 
    */
   for( sh = shiplist.begin(  ); sh != shiplist.end(  ); ++sh )
   {
      ship_data *ship = *sh;

      if( ship->cmap == cmap && ship->mx == x && ship->my == y )
      {
         if( !str_cmp( ch->name, ship->owner ) )
         {
            ch->set_pcflag( PCFLAG_ONSHIP );
            ch->on_ship = ship;
            ch->mx = ship->mx;
            ch->my = ship->my;
            interpret( ch, "look" );
            ch->printf( "You board %s.\r\n", ship->name.c_str(  ) );
            return rSTOP;
         }
         else
         {
            ch->printf( "The crew abord %s blocks you from boarding!\r\n", ship->name.c_str(  ) );
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
   list < obj_data * >::iterator iobj;
   for( iobj = ch->carrying.begin(  ); iobj != ch->carrying.end(  ); ++iobj )
   {
      obj_data *obj = *iobj;

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
   int sector = get_terrain( cmap, x, y );
   short fx = ch->mx, fy = ch->my, fmap = ch->cmap;
   if( sector == SECT_EXIT )
   {
      mapexit_data *mexit;
      room_index *toroom = NULL;

      mexit = check_mapexit( cmap, x, y );

      if( mexit != NULL && !ch->has_pcflag( PCFLAG_MAPEDIT ) )
      {
         if( mexit->tomap != -1 )   /* Means exit goes to another map */
         {
            enter_map( ch, NULL, mexit->therex, mexit->therey, mexit->tomap );

            list < char_data * >::iterator ich;
            size_t chars = from_room->people.size(  );
            size_t count = 0;
            for( ich = from_room->people.begin(  ); ich != from_room->people.end(  ), ( count < chars ); )
            {
               char_data *fch = *ich;
               ++ich;
               ++count;

               if( fch != ch  /* loop room bug fix here by Thoric */
                   && fch->master == ch && ( fch->position == POS_STANDING || fch->position == POS_MOUNTED ) && fch->mx == fx && fch->my == fy && fch->cmap == fmap )
               {
                  if( !fch->isnpc(  ) )
                  {
                     act( AT_ACTION, "You follow $N.", fch, NULL, ch, TO_CHAR );
                     process_exit( fch, fch->cmap, x, y, dir, running );
                  }
                  else
                     enter_map( fch, NULL, mexit->therex, mexit->therey, mexit->tomap );
               }
            }
            return rSTOP;
         }

         if( !( toroom = get_room_index( mexit->vnum ) ) )
         {
            if( !ch->isnpc(  ) )
            {
               bug( "%s: Target vnum %d for map exit does not exist!", __FUNCTION__, mexit->vnum );
               ch->print( "Ooops. Something bad happened. Contact the immortals ASAP.\r\n" );
            }
            return rSTOP;
         }

         if( ch->isnpc(  ) )
         {
            list < exit_data * >::iterator ex;
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

         leave_map( ch, NULL, toroom );

         list < char_data * >::iterator ich;
         size_t chars = from_room->people.size(  );
         size_t count = 0;
         for( ich = from_room->people.begin(  ); ich != from_room->people.end(  ), ( count < chars ); )
         {
            char_data *fch = *ich;
            ++ich;
            ++count;

            if( fch != ch  /* loop room bug fix here by Thoric */
                && fch->master == ch && ( fch->position == POS_STANDING || fch->position == POS_MOUNTED ) && fch->mx == fx && fch->my == fy && fch->cmap == fmap )
            {
               if( !fch->isnpc(  ) )
               {
                  act( AT_ACTION, "You follow $N.", fch, NULL, ch, TO_CHAR );
                  process_exit( fch, fch->cmap, x, y, dir, running );
               }
               else
                  leave_map( fch, ch, toroom );
            }
         }
         return rSTOP;
      }

      if( mexit != NULL && ch->has_pcflag( PCFLAG_MAPEDIT ) )
      {
         delete_mapexit( mexit );
         putterr( ch->cmap, x, y, ch->pcdata->secedit );
         ch->print( "&RMap exit deleted.\r\n" );
      }
   }

   if( !sect_show[sector].canpass && !ch->is_immortal(  ) )
   {
      ch->printf( "%s\r\n", impass_message[sector] );
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
         act_printf( AT_ACTION, ch, NULL, ch->mount, TO_NOTVICT, "$n %s %s upon $N.", txt, dir_name[dir] );
      else
         act_printf( AT_ACTION, ch, NULL, dir_name[dir], TO_ROOM, "$n %s $T.", txt );
   }

   if( !ch->is_immortal(  ) ) /* Imms don't get charged movement */
   {
      if( ch->mount )
      {
         ch->mount->move -= move;
         ch->mount->mx = x;
         ch->mount->my = y;
      }
      else
         ch->move -= move;
   }

   ch->mx = x;
   ch->my = y;

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
   const char *dtxt = rev_exit( dir );

   if( !running )
   {
      if( ch->mount )
         act_printf( AT_ACTION, ch, NULL, ch->mount, TO_ROOM, "$n %s from %s upon $N.", txt, dtxt );
      else
         act_printf( AT_ACTION, ch, NULL, NULL, TO_ROOM, "$n %s from %s.", txt, dtxt );
   }

   list < char_data * >::iterator ich;
   size_t chars = from_room->people.size(  );
   size_t count = 0;
   for( ich = from_room->people.begin(  ); ich != from_room->people.end(  ), ( count < chars ); )
   {
      char_data *fch = *ich;
      ++ich;
      ++count;

      if( fch != ch  /* loop room bug fix here by Thoric */
          && fch->master == ch && ( fch->position == POS_STANDING || fch->position == POS_MOUNTED ) && fch->mx == fx && fch->my == fy )
      {
         if( !fch->isnpc(  ) )
         {
            if( !running )
               act( AT_ACTION, "You follow $N.", fch, NULL, ch, TO_CHAR );
            process_exit( fch, fch->cmap, x, y, dir, running );
         }
         else
         {
            fch->mx = x;
            fch->my = y;
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

/* Fairly self-explanitory. It finds out what continent an area is on and drops the PC on the appropriate map for it */
room_index *find_continent( char_data * ch, room_index * maproom )
{
   room_index *location = NULL;

   if( maproom->area->continent == ACON_ONE )
   {
      location = get_room_index( OVERLAND_ONE );
      ch->cmap = MAP_ONE;
   }

   return location;
}

/* How one gets from a normal zone onto the overland, via an exit flagged as EX_OVERLAND */
void enter_map( char_data * ch, exit_data * pexit, int x, int y, int continent )
{
   room_index *maproom = NULL, *original;

   if( continent < 0 )  /* -1 means you came in from a regular area exit */
      maproom = find_continent( ch, ch->in_room );

   else  /* Means you are either an immortal using the goto command, or a mortal who teleported */
   {
      switch ( continent )
      {
         case ACON_ONE:
            maproom = get_room_index( OVERLAND_ONE );
            ch->cmap = MAP_ONE;
            break;
         default:
            bug( "%s: Invalid target map specified: %d", __FUNCTION__, continent );
            return;
      }
   }

   /*
    * Hopefully this hack works 
    */
   if( pexit && pexit->to_room->flags.test( ROOM_WATCHTOWER ) )
      maproom = pexit->to_room;

   if( !maproom )
   {
      bug( "%s: Overland map room is missing!", __FUNCTION__ );
      ch->print( "Woops. Something is majorly wrong here - inform the immortals.\r\n" );
      return;
   }

   if( !ch->isnpc(  ) )
      ch->set_pcflag( PCFLAG_ONMAP );
   else
   {
      ch->set_actflag( ACT_ONMAP );
      if( ch->sector == -1 && get_terrain( ch->cmap, x, y ) == ch->in_room->sector_type )
         ch->sector = get_terrain( ch->cmap, x, y );
      else
         ch->sector = -2;
   }

   ch->mx = x;
   ch->my = y;

   original = ch->in_room;
   ch->from_room(  );
   if( !ch->to_room( maproom ) )
      log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
   collect_followers( ch, original, ch->in_room );
   interpret( ch, "look" );

   /*
    * Turn on the overland music 
    */
   if( !maproom->flags.test( ROOM_WATCHTOWER ) )
      ch->music( "wilderness.mid", 100, false );
}

/* How one gets off the overland into a regular zone via those nifty white # symbols :) 
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

   ch->mx = -1;
   ch->my = -1;
   ch->cmap = -1;

   if( target != NULL )
   {
      room_index *from = ch->in_room;
      ch->from_room(  );
      if( !ch->to_room( target ) )
         log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
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
   string arg;
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

   x = atoi( arg.c_str(  ) );
   y = atoi( argument.c_str(  ) );

   if( x < 0 || x >= MAX_X )
   {
      ch->printf( "Valid x coordinates are 0 to %d.\r\n", MAX_X - 1 );
      return;
   }

   if( y < 0 || y >= MAX_Y )
   {
      ch->printf( "Valid y coordinates are 0 to %d.\r\n", MAX_Y - 1 );
      return;
   }

   ch->mx = x;
   ch->my = y;

   if( ch->mount )
   {
      ch->mount->mx = x;
      ch->mount->my = y;
   }

   if( ch->on_ship && ch->has_pcflag( PCFLAG_ONSHIP ) )
   {
      ch->on_ship->mx = x;
      ch->on_ship->my = y;
   }

   interpret( ch, "look" );
}

/* Online OLC map editing stuff starts here */

/* Stuff for the floodfill and undo functions */
struct undotype
{
   struct undotype *next;
   short cmap;
   short xcoord;
   short ycoord;
   short prevterr;
};

struct undotype *undohead = 0x0; /* for undo buffer */
struct undotype *undocurr = 0x0; /* most recent undo data */

/* This baby floodfills an entire region of sectors, starting from where the PC is standing,
 * and continuing until it hits something other than what it was told to fill with. 
 * Aborts if you attempt to fill with the same sector your standing on.
 * Floodfill code courtesy of Vor - don't ask for his address, I'm not allowed to give it to you!
 */
int floodfill( short cmap, short xcoord, short ycoord, short fill, char terr )
{
   struct undotype *undonew;

   /*
    * Abort if trying to flood same terr type 
    */
   if( terr == fill )
      return ( 1 );

   undonew = new undotype;

   undonew->cmap = cmap;
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

   putterr( cmap, xcoord, ycoord, fill );

   if( get_terrain( cmap, xcoord + 1, ycoord ) == terr )
      floodfill( cmap, xcoord + 1, ycoord, fill, terr );
   if( get_terrain( cmap, xcoord, ycoord + 1 ) == terr )
      floodfill( cmap, xcoord, ycoord + 1, fill, terr );
   if( get_terrain( cmap, xcoord - 1, ycoord ) == terr )
      floodfill( cmap, xcoord - 1, ycoord, fill, terr );
   if( get_terrain( cmap, xcoord, ycoord - 1 ) == terr )
      floodfill( cmap, xcoord, ycoord - 1, fill, terr );

   return ( 0 );
}

/* call this to undo any floodfills buffered, changes the undo buffer 
to hold the undone terr type, so doing an undo twice actually 
redoes the floodfill :) */
int unfloodfill( void )
{
   char terr;  /* terr to undo */

   if( undohead == 0x0 )
      return ( 0 );

   undocurr = undohead;
   do
   {
      terr = get_terrain( undocurr->cmap, undocurr->xcoord, undocurr->ycoord );
      putterr( undocurr->cmap, undocurr->xcoord, undocurr->ycoord, undocurr->prevterr );
      undocurr->prevterr = terr;
      undocurr = undocurr->next;
   }
   while( undocurr->next != 0x0 );

   /*
    * you'll prolly want to add some error checking here.. :P 
    */
   return ( 0 );
}

/* call this any time you want to clear the undo buffer, such as 
between successful floodfills (otherwise, an undo will undo ALL floodfills done this session :P) */
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

/* Used to reload a graphic file for the map you are currently standing on.
 * Take great care not to hose your file, results would be unpredictable at best.
 */
void reload_map( char_data * ch )
{
   if( ch->cmap < 0 || ch->cmap >= MAP_MAX )
   {
      bug( "%s: Trying to reload invalid map!", __FUNCTION__ );
      return;
   }

   ch->pagerf( "&GReinitializing map grid for %s....\r\n", map_names[ch->cmap] );

   for( short x = 0; x < MAX_X; ++x )
   {
      for( short y = 0; y < MAX_Y; ++y )
         putterr( ch->cmap, x, y, SECT_OCEAN );
   }
   load_map_png( map_filenames[ch->cmap], ch->cmap );
}

/* As it implies, this saves the map you are currently standing on to disk.
 * Output is in graphic format, making it easily edited with most paint programs.
 * Could also be used as a method for filtering bad color data out of your source
 * image if it had any at loadup. This code should only be called from the mapedit
 * command. Using it in any other way could break something.
 */
// Thanks Davion for this :), PNG format = HUGE time-saver :)
void save_map_png( const char *mapfile, short mapnumber )
{
   gdImagePtr im;
   FILE *PngOut;
   char graphicname[256];
   short x, y, terr;
   int image[SECT_MAX];

   im = gdImageCreate( MAX_X, MAX_Y );

   for( x = 0; x < SECT_MAX; ++x )
      image[x] = gdImageColorAllocate( im, sect_show[x].graph1, sect_show[x].graph2, sect_show[x].graph3 );

   for( y = 0; y < MAX_Y; ++y )
   {
      for( x = 0; x < MAX_X; ++x )
      {
         terr = get_terrain( mapnumber, x, y );
         if( terr == -1 )
            terr = SECT_OCEAN;

         gdImageLine( im, x, y, x, y, image[terr] );
      }
   }

   mapfile = strlower( mapfile );   /* Forces filename into lowercase */
   snprintf( graphicname, 256, "%s%s.png", MAP_DIR, mapfile );

   if( ( PngOut = fopen( graphicname, "w" ) ) == NULL )
   {
      bug( "%s: fopen", __FUNCTION__ );
      perror( graphicname );
   }

   /*
    * Output the same image in JPEG format, using the default JPEG quality setting. 
    */
   gdImagePng( im, PngOut );  //, -1 );

   /*
    * Close the files. 
    */
   FCLOSE( PngOut );

   /*
    * Destroy the image in memory. 
    */
   gdImageDestroy( im );
}

/* And here we have the OLC command itself. Fairly simplistic really. And more or less useless
 * for anything but really small edits now ( graphic file editing is vastly superior ).
 * Used to be the only means for editing maps before the 12-6-00 release of this code.
 * Caution is warranted if using the floodfill option - trying to floodfill a section that
 * is too large will overflow the memory in short order and cause a rather uncool crash.
 */
CMDF( do_mapedit )
{
   string arg1;
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

   if( !ch->has_pcflag( PCFLAG_ONMAP ) )
   {
      ch->print( "This command can only be used from an overland map.\r\n" );
      return;
   }

   argument = one_argument( argument, arg1 );

   if( arg1.empty(  ) )
   {
      if( ch->has_pcflag( PCFLAG_MAPEDIT ) )
      {
         ch->unset_pcflag( PCFLAG_MAPEDIT );
         ch->print( "&GMap editing mode is now OFF.\r\n" );
         return;
      }

      ch->set_pcflag( PCFLAG_MAPEDIT );
      ch->print( "&RMap editing mode is now ON.\r\n" );
      ch->printf( "&YYou are currently creating %s sectors.&z\r\n", sect_types[ch->pcdata->secedit] );
      return;
   }

   if( !str_cmp( arg1, "help" ) )
   {
      ch->print( "Usage: mapedit sector <sectortype>\r\n" );
      ch->print( "Usage: mapedit save <mapname>\r\n" );
      ch->print( "Usage: mapedit fill <sectortype>\r\n" );
      ch->print( "Usage: mapedit undo\r\n" );
      ch->print( "Usage: mapedit reload\r\n\r\n" );
      return;
   }

   if( !str_cmp( arg1, "reload" ) )
   {
      if( str_cmp( argument, "confirm" ) )
      {
         ch->print( "This is a dangerous command if used improperly.\r\n" );
         ch->print( "Are you sure about this? Confirm by typing: mapedit reload confirm\r\n" );
         return;
      }
      reload_map( ch );
      return;
   }

   if( !str_cmp( arg1, "fill" ) )
   {
      int flood, fill;
      short standingon = get_terrain( ch->cmap, ch->mx, ch->my );

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
      fill = floodfill( ch->cmap, ch->mx, ch->my, flood, standingon );

      if( fill == 0 )
      {
         display_map( ch );
         ch->printf( "&RFooodfill with %s sectors successful.\r\n", argument.c_str(  ) );
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
      int undo = unfloodfill(  );

      if( undo == 0 )
      {
         display_map( ch );
         ch->print( "&RUndo successful.\r\n" );
         return;
      }
      ch->print( "Unknown error during undo.\r\n" );
      return;
   }

   if( !str_cmp( arg1, "save" ) )
   {
      int map = -1;

      if( argument.empty(  ) )
      {
         const char *mapname;

         if( ch->cmap == -1 )
         {
            bug( "%s: %s is not on a valid map!", __FUNCTION__, ch->name );
            ch->print( "Can't do that - your on an invalid map.\r\n" );
            return;
         }
         mapname = map_name[ch->cmap];

         ch->printf( "Saving map of %s....\r\n", mapname );
         save_map_png( mapname, ch->cmap );
         return;
      }

      if( !str_cmp( argument, "one" ) )
         map = MAP_ONE;

      if( map == -1 )
      {
         ch->printf( "There isn't a map for '%s'.\r\n", arg1.c_str(  ) );
         return;
      }

      ch->printf( "Saving map of %s....", argument.c_str(  ) );
      save_map_png( argument.c_str(  ), map );
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
      ch->printf( "&YYou are now creating %s sectors.\r\n", argument.c_str(  ) );
      return;
   }

   ch->print( "Usage: mapedit sector <sectortype>\r\n" );
   ch->print( "Usage: mapedit save <mapname>\r\n" );
   ch->print( "Usage: mapedit fill <sectortype>\r\n" );
   ch->print( "Usage: mapedit undo\r\n" );
   ch->print( "Usage: mapedit reload\r\n\r\n" );
}

/* Online OLC map editing stuff ends here */
