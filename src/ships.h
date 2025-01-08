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
 *                             Player ships                                 *
 *                   Brought over from Lands of Altanos                     *
 ***************************************************************************/

#ifndef __SHIPS_H__
#define __SHIPS_H__

enum ship_flag_settings
{
   SHIP_ANCHORED, SHIP_ONMAP, SHIP_AIRSHIP, MAX_SHIP_FLAG
};

class ship_data
{
 private:
   ship_data( const ship_data & s );
     ship_data & operator=( const ship_data & );

 public:
     ship_data(  );
    ~ship_data(  );

   string name;
   string owner;
     bitset < MAX_SHIP_FLAG > flags;
   int fuel;
   int max_fuel;
   int hull;
   int max_hull;
   int type;
   int vnum;
   int room;
   short mx;
   short my;
   short wmap;
};

enum ship_types
{
   SHIP_NONE, SHIP_SKIFF, SHIP_COASTER, SHIP_CARAVEL, SHIP_GALLEON, SHIP_WARSHIP, SHIP_MAX
};

extern list < ship_data * >shiplist;

#define SHIP_FILE SYSTEM_DIR "ships.dat"  /* For ships */
#endif
