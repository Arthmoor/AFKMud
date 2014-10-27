/****************************************************************************
 *                   ^     +----- |  / ^     ^ |     | +-\                  *
 *                  / \    |      | /  |\   /| |     | |  \                 *
 *                 /   \   +---   |<   | \ / | |     | |  |                 *
 *                /-----\  |      | \  |  v  | |     | |  /                 *
 *               /       \ |      |  \ |     | +-----+ +-/                  *
 ****************************************************************************
 * AFKMud Copyright 1997-2008 by Roger Libiez (Samson),                     *
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
 *	                     Overland ANSI Map Module                         *
 *                      Created by Samson of Alsherok                       *
 ****************************************************************************/

#ifndef __OVERLAND_H__
#define __OVERLAND_H__

const int OVERLAND_ONE = 50000;

const int MAX_X = 1000;
const int MAX_Y = 1000;

/* Change these filenames to match yours */
#define ENTRANCE_FILE "entrances.dat"
#define LANDMARK_FILE "landmarks.dat"

enum map_types
{
   MAP_ONE, MAP_MAX
};

// Single continent, starts at 0,0 in the NW corner. 1000x1000 default size.
extern unsigned char map_sector[MAP_MAX][MAX_X][MAX_Y];
extern char *const map_names[];
extern char *const map_name[];
extern char *const continents[];
extern char *const sect_types[];
extern const struct sect_color_type sect_show[];

class landmark_data
{
 private:
   landmark_data( const landmark_data& l );
   landmark_data& operator=( const landmark_data& );

 public:
   landmark_data();
   ~landmark_data();

   char *description;   /* Description of the landmark */
   int distance;  /* Distance the landmark is visible from */
   short map;  /* Map the landmark is on */
   short mx;   /* X coordinate of landmark */
   short my;   /* Y coordinate of landmark */
   bool Isdesc;   /* If true is room desc. If not is landmark */
};

class mapexit_data
{
 private:
   mapexit_data( const mapexit_data& m );
   mapexit_data& operator=( const mapexit_data& );

 public:
   mapexit_data();
   ~mapexit_data();

   char *area; /* Area name */
   int vnum;   /* Target vnum if it goes to a regular zone */
   short herex;   /* Coordinates the entrance is at */
   short herey;
   short therex;  /* Coordinates the entrance goes to, if any */
   short therey;
   short tomap;   /* Map it goes to, if any */
   short onmap;   /* Which map it's on */
   short prevsector; /* Previous sector type to restore with when an exit is deleted */
};

struct sect_color_type
{
   short sector;  /* Terrain sector */
   char *color;   /* Color to display as */
   char *symbol;  /* Symbol you see for the sector */
   char *desc;    /* Description of sector type */
   bool canpass;  /* Impassable terrain */
   int move;      /* Movement loss */
   short graph1;  /* Color numbers for graphic conversion */
   short graph2;
   short graph3;
};

extern list<mapexit_data*> mapexitlist;

short get_terrain( short, short, short );
mapexit_data *check_mapexit( short, short, short );
void delete_mapexit( mapexit_data * );
void putterr( short, short, short, short );
ch_ret process_exit( char_data *, short, short, short, int, bool );
double distance( short, short, short, short );
double calc_angle( short, short, short, short, double * );
#endif
