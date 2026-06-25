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
 *                             Object Class Info                            *
 ****************************************************************************/

#pragma once

constexpr int MAX_OBJ_VALUE = 11; // This should always be one more than you actually have.

/*
 * One object.
 */
class obj_data
{
 private:
   obj_data( const obj_data & o );
     obj_data & operator=( const obj_data & );

 public:
     obj_data(  );
    ~obj_data(  );

   /*
    * Internal refs in object.c 
    */
   void fall( bool );
   short get_resistance(  );
   const std::string oshort(  );
   const std::string format_to_char( char_data *, bool, int );
   obj_data *to_char( char_data * );
   void from_char(  );
   int apply_ac( int );
   void from_room(  );
   obj_data *to_room( room_index *, char_data * );
   obj_data *to_obj( obj_data * );
   void from_obj(  );
   void extract(  );
   int get_number(  );
   int get_weight(  );
   int get_real_weight(  );
   const std::string item_type_name(  );
   bool is_trapped(  );
   obj_data *get_trap(  );
   bool extracted(  );
   obj_data *clone(  );
   void split( int );
   void separate(  );
   bool empty( obj_data *, room_index * );
   void remove_portal(  );
   char_data *who_carrying(  );
   bool in_magic_container(  );
   void make_scraps(  );
   int hitroll(  );
   const std::string myobj(  );

   /*
    * External refs in other files 
    */
   void armorgen(  );
   void weapongen(  );

   class obj_data *in_obj = nullptr;            // Pointer to the object which this object may be contained in.
   class obj_index *pIndexData = nullptr;       // Pointer to the object's master index data.
   class room_index *in_room = nullptr;         // Pointer to the room the object may be sitting in.
   class char_data *carried_by = nullptr;       // Pointer to the character who may be carrying this object.
   class continent_data *continent = nullptr;   // Pointer to which continent the object is on for the overland system - Samson 8-21-99
   std::list<obj_data *> contents;              // Objects this object contains.
   std::list<affect_data *> affects;            // List of affects this object has.
   std::list<extra_descr_data *> extradesc;     // List of extra descriptions the object has.
   std::list<class mprog_act_list *> mpact;     // Mudprogs
   std::bitset<MAX_ITEM_FLAG> extra_flags;      // Extra flags for the object.
   std::bitset<MAX_WEAR_FLAG> wear_flags;       // Wear flags for the object.
   std::string name;                            // Keywords used to interact with this object.
   std::string short_descr;                     // The one line description of the object when seen in inventories or equipment lists.
   std::string objdesc;                         // Long description seen when the object is in a room.
   std::string action_desc;                     // Message displayed when someone interacts with the object. Not fully supported in code.
   std::string owner;                           // Who owns this item? Used with personal flag for Sindhae prizes.
   std::string seller;                          // Who put the item up for auction?
   std::string buyer;                           // Who made the final bid on the item?
   std::string socket[3];                       // Name of rune/gem the item has in each socket - Samson 3-31-02
   int value[MAX_OBJ_VALUE];                    // Various values and flags based on the object type - Raised to 11 by Samson on 12-14-02
   int bid = 0;                                 // What was the amount of the final bid on auction?
   int mpactnum = 0;                            // Mudprogs
   int ego = -2;                                // The ego level the object has. This is used to determine whether a player can use or keep this object long term. -2 triggers an autocalc function.
   int room_vnum = 0;                           // Track it's room vnum for hotbooting and such.
   int cost = 0;                                // Base cost of the object when sold in shops.
   short wear_loc = -1;                         // What part of the body the object is intended to be used on.
   short weight = 0;                            // How much the object weighs in pounds.
   short level = 0;                             // What level the object spawns at.
   short timer = 0;                             // Countdown timer before the object despawns on its own.
   short count = 1;                             // Support for object grouping.
   short map_x = -1;                            // Object X/Y coordinates on overland maps - Samson 8-21-99
   short map_y = -1;
   short day = 0;                               // What day of the week was it offered or sold?
   short month = 0;                             // What month?
   short year = 0;                              // What year?
   short item_type = 0;                         // What type of object this is.
   unsigned short mpscriptpos = 0;              // Something to do with Mudprogs.
};

obj_data *get_objtype( char_data *, short );
void obj_identify_output( char_data *, obj_data * );
void show_list_to_char( char_data *, std::list<obj_data *>, bool, bool );
void fwrite_obj( char_data *, std::list<obj_data *>, clan_data *, FILE *, int, bool );
void fread_obj( char_data *, FILE *, short );
obj_data *group_obj( obj_data * obj, obj_data * obj2 );

extern std::list<obj_data *> objlist;
extern obj_data *save_equipment[MAX_WEAR][MAX_LAYERS];
extern obj_data *mob_save_equipment[MAX_WEAR][MAX_LAYERS];
