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
 *                          Object Index Class Info                         *
 ****************************************************************************/

#pragma once

/*
 * Prototype for an object.
 */
class obj_index
{
 private:
   obj_index( const obj_index & o );
     obj_index & operator=( const obj_index & );

 public:
     obj_index(  );
    ~obj_index(  );

   void clean_obj(  );
   obj_data *create_object( int );
   int set_ego(  );
   void oprog_read_programs( FILE * );

   class area_data *area = nullptr;             // Area this objindex is associated with.
   std::list<affect_data *> affects;            // List of affects the object has.
   std::list<extra_descr_data *> extradesc;     // List of extra descriptions the object has.
   std::list<struct mud_prog_data *> mudprogs;  // Mudprogs
   std::bitset<MAX_PROG> progtypes;             // objprogs
   std::bitset<MAX_ITEM_FLAG> extra_flags;      // Extra flags for the object.
   std::bitset<MAX_WEAR_FLAG> wear_flags;       // Wear flags for the object.
   std::string name;                            // Keywords used to interact with this object.
   std::string short_descr;                     // The one line description of the object when seen in inventories or equipment lists.
   std::string objdesc;                         // Long description seen when the object is in a room.
   std::string action_desc;                     // Message displayed when someone interacts with the object. Not fully supported in code.
   std::string socket[3];                       // Name of rune/gem the item has in each socket - Samson 3-31-02
   int value[MAX_OBJ_VALUE];                    // Various values and flags based on the object type - Raised to 11 by Samson on 12-14-02
   int vnum = 0;                                // Vnum of the objindex. Must be unique.
   int cost = 0;                                // Base cost of the object when sold in shops.
   int ego = -2;                                // The ego level the object has. This is used to determine whether a player can use or keep this object long term. -2 triggers an autocalc function.
   int limit = 0;                               // Limit on how many of these are allowed to load - Samson 1-9-00
   short level = 0;                             // What level the object spawns at.
   short item_type = 0;                         // What type of object this is.
   short count = 1;                             // Support for object grouping.
   short weight = 0;                            // How much the object weighs in pounds.
   short layers = 0;                            // Bitvector value for which layers the object occupies.
};

extern std::map<int, obj_index *> obj_index_table;
obj_index *get_obj_index( int );
obj_index *make_object( int, int, const std::string &, area_data * );
