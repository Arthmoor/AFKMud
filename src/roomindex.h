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
 *                           Room Index Class Info                          *
 ****************************************************************************/

#pragma once

/*
 * Reset definition.
 */
class reset_data
{
 private:
   reset_data( const reset_data & r );
     reset_data & operator=( const reset_data & );

 public:
     reset_data(  );

   std::list<reset_data *> resets;   // Child resets associated with this reset
   obj_data *resetobj;
   char command;
   // Attention at the keyboard: Don't go setting these back to shorts. Charlana.are will drain your soul!
   // Cause, ya know, this shit loads vnums all over the place and you'll break stuff. Badly.
   int arg1 = 0;
   int arg2 = 0;
   int arg3 = 0;

   // Values from here onward added by AFKMud.
   int arg4 = 0;
   int arg5 = 0;
   int arg6 = 0;
   int arg7 = 0;
   int arg8 = 0;
   int arg9 = 0;
   int arg10 = 0;
   int arg11 = 0;
   bool sreset = false; // Purpose of this bool is unclear.
};

/*
 * Exit data.
 */
class exit_data
{
 private:
   exit_data( const exit_data & e );
     exit_data & operator=( const exit_data & );

 public:
     exit_data(  );
    ~exit_data(  );

   exit_data *rexit = nullptr;      // Reverse exit pointer.
   room_index *to_room = nullptr;   // Pointer to destination room.
   std::bitset<MAX_EXFLAG> flags;   // door states & other flags.
   std::string keyword;             // Keywords for exit or door.
   std::string exitdesc;            // Description of exit.
   int vnum = 0;                    // Vnum of room exit leads to.
   int rvnum = 0;                   // Vnum of room in opposite dir.
   int key = -1;                    // Key vnum.
   short vdir = 0;                  // Physical "direction".
   short pull = 0;                  // pull of direction (current).
   short pulltype = 0;              // type of pull (current, wind).
   short map_x = -1;                // Coordinates to Overland Map - Samson 7-31-99
   short map_y = -1;
};

/*
 * Room type.
 */
class room_index
{
 private:
   room_index( const room_index & r );
     room_index & operator=( const room_index & );

 public:
     room_index(  );
    ~room_index(  );

   void olc_add_affect( char_data *, bool, std::string & );
   void olc_remove_affect( char_data *, bool, std::string_view );
   void clean_room(  );
   void randomize_exits( short );
   exit_data *make_exit( room_index *, short );
   void extract_exit( exit_data * );
   exit_data *get_exit( short );
   exit_data *get_exit_to( short, int );
   exit_data *get_exit_num( short );
   void echo( std::string_view );
   void rprog_read_programs( FILE * );
   bool is_dark( char_data * );
   bool is_private(  );
   void room_affect( affect_data *, bool );
   reset_data *add_reset( char, int, int, int, int, int, int, int, int, int, int, int );
   void reset(  );
   void wipe_coord_resets( short, short );
   void wipe_resets(  );
   void clean_resets(  );
   void renumber_put_resets(  );
   void load_reset( FILE *, bool );

   area_data *area = nullptr;                   // Area this room belongs to.
   reset_data *last_mob_reset = nullptr;
   reset_data *last_obj_reset = nullptr;
   std::list<reset_data *> resets;              // Things that get loaded in this room.
   std::list<char_data *> people;               // People in the room.
   std::list<obj_data *> objects;               // Objects on the floor.
   std::list<affect_data *> permaffects;        // Permanent affects on the room set via the area file or OLC.
   std::list<affect_data *> affects;            // Affects on the room.
   std::list<exit_data *> exits;                // Exits from the room.
   std::list<extra_descr_data *> extradesc;     // Extra descriptions.
   std::list<struct mud_prog_data *> mudprogs;  // Mudprogs
   std::list<mprog_act_list *> mpact;           // Mudprogs
   std::bitset<ROOM_MAX> flags;                 // Flags on the room.
   std::bitset<MAX_PROG> progtypes;             // Mudprogs
   std::string name;                            // The room's name.
   std::string roomdesc;                        // Detailed description of the room. Pointer name changed so that it can now be more easily grep'd - Samson 10-16-03
   std::string nitedesc;                        // Detailed description of this room at night. Added NiteDesc -- Dracones
   int vnum = 0;                                // The room's unique Vnum.
   int tele_vnum = 0;                           // Where players get teleported to once the delay timer runs out.
   int weight = 0;                              // Current amount of weight present in the room. - Taken from Smaug 1.8
   int max_weight = 100000;                     // Limit for how much weight the room can hold.  - Taken from Smaug 1.8
   int mpactnum = 0;                            // Mudprogs
   unsigned short mpscriptpos;                  // Mudprogs
   short baselight = 0;                         // Preset light amount in this room.
   short light = 0;                             // Modified amount of light in the room.
   short sector_type = 0;                       // What type of terrain this room is set to.
   short winter_sector = -1;                    // Stores the original sector type for stuff that freezes in winter - Samson 7-19-00
   short tele_delay = 0;                        // Delay timer before players are teleported from the room.
   short tunnel = 0;                            // Max people that will fit in the room.

};

extern std::map<int, room_index *> room_index_table;

room_index *get_room_index( int );
room_index *make_room( int, area_data * );
int get_dirnum( std::string_view );
std::string rev_exit( short );

/*
 * Delayed teleport type.
 */
struct teleport_data
{
   room_index *room = nullptr;
   short timer = 0;
};

extern std::list<teleport_data *> teleportlist;

template <typename T>
bool CAN_GO( const T* x, int door )
{
   if( !x || !x->in_room )
      return false;

   auto exit = x->in_room->get_exit( door );

   return exit != nullptr && exit->to_room != nullptr && !IS_EXIT_FLAG( exit, EX_CLOSED );
}
