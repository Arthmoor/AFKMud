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
 *                             Player ships                                 *
 *                   Brought over from Lands of Altanos                     *
 ***************************************************************************/

#pragma once

inline constexpr std::string_view SHIP_FILE = "../system/ships.dat";

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

   std::string name;                            // Name of the ship.
   std::string owner;                           // Who owns the ship.
   std::bitset<MAX_SHIP_FLAG> flags;            // Flags to set on the ship.
   class continent_data *continent = nullptr;   // Which continent the ship is on.
   int fuel = 0;                                // How much fuel the ship currently has.
   int max_fuel = 0;                            // How much fuel the ship can hold.
   int hull = 0;                                // Current strength of the hull. If it reaches zero, it sinks.
   int max_hull = 0;                            // Maximum hull strength.
   int type = 0;                                // What type of ship this is.
   int vnum = 0;                                // Ship's unique identifier.
   int room = 0;                                // What room the ship is currently in.
   short map_x = -1;                            // X/Y coordinates on the overland.
   short map_y = -1;
};

enum ship_types
{
   SHIP_NONE, SHIP_SKIFF, SHIP_COASTER, SHIP_CARAVEL, SHIP_GALLEON, SHIP_WARSHIP, SHIP_MAX
};

extern std::list<ship_data *> shiplist;
