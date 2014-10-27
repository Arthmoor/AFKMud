/****************************************************************************
 *                   ^     +----- |  / ^     ^ |     | +-\                  *
 *                  / \    |      | /  |\   /| |     | |  \                 *
 *                 /   \   +---   |<   | \ / | |     | |  |                 *
 *                /-----\  |      | \  |  v  | |     | |  /                 *
 *               /       \ |      |  \ |     | +-----+ +-/                  *
 ****************************************************************************
 * AFKMud Copyright 1997-2007 by Roger Libiez (Samson),                     *
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
 *                           Room Index Class Info                          *
 ****************************************************************************/

#ifndef __ROOMINDEX_H__
#define __ROOMINDEX_H__

/*
 * Reset definition.
 */
class reset_data
{
 private:
   reset_data( const reset_data& r );
   reset_data& operator=( const reset_data& );

 public:
   reset_data();

   list<reset_data*> resets; // Child resets associated with this reset
   char command;
   int arg1;
   int arg2;
   int arg3;
   short arg4; /* arg4 - arg6 used for overland coordinates */
   short arg5;
   short arg6;
   short arg7;
   short arg8;
   short arg9;
   short arg10;
   short arg11;
};

/*
 * Exit data.
 */
class exit_data
{
 private:
   exit_data( const exit_data& e );
   exit_data& operator=( const exit_data& );

 public:
   exit_data();
   ~exit_data();

   exit_data *rexit;         // Reverse exit pointer
   room_index *to_room;      // Pointer to destination room
   bitset<MAX_EXFLAG> flags; // door states & other flags
   char *keyword;            // Keywords for exit or door
   char *exitdesc;           // Description of exit
   int vnum;                 // Vnum of room exit leads to
   int rvnum;                // Vnum of room in opposite dir
   int key;                  // Key vnum
   short vdir;               // Physical "direction"
   short pull;               // pull of direction (current)
   short pulltype;           // type of pull (current, wind)
   short mx;                 // Coordinates to Overland Map - Samson 7-31-99
   short my;
};

/*
 * Room type.
 */
class room_index
{
 private:
   room_index( const room_index& r );
   room_index& operator=( const room_index& );

 public:
   room_index(  );
   ~room_index(  );

   void olc_add_affect( char_data *ch, bool indexaffect, char *argument );
   void olc_remove_affect( char_data *ch, bool indexaffect, char *argument );
   void clean_room(  );
   void randomize_exits( short );
   exit_data *make_exit( room_index *, short );
   void extract_exit( exit_data * );
   exit_data *get_exit( short );
   exit_data *get_exit_to( short, int );
   exit_data *get_exit_num( short );
   void echo( char * );
   void rprog_read_programs( FILE * );
   bool is_dark( char_data * );
   bool is_private(  );
   void room_affect( affect_data *, bool );
   reset_data *add_reset( char, int, int, int, short, short, short, short, short, short, short, short );
   void reset(  );
   void wipe_resets();
   void clean_resets();
   void renumber_put_resets(  );
   void load_reset( FILE *, bool );

   list<reset_data*> resets;   /* Things that get loaded in this room */
   list<char_data*> people;    /* People in the room  */
   list<obj_data*> objects;    /* Objects on the floor */
   list<affect_data*> indexaffects; // Permanent affects on the room set via the area file or OLC
   list<affect_data*> affects; /* Affects on the room */
   list<exit_data*> exits;     /* Exits from the room */
   list<extra_descr_data*> extradesc; /* Extra descriptions */
   list<struct mud_prog_data*> mudprogs; /* Mudprogs */
   list<struct mprog_act_list*> mpact; /* Mudprogs */
   room_index *next;
   area_data *area;
   reset_data *last_mob_reset;
   reset_data *last_obj_reset;
     bitset < ROOM_MAX > flags;
     bitset < MAX_PROG > progtypes; /* mudprogs */
   char *name;
   char *roomdesc;   /* So that it can now be more easily grep'd - Samson 10-16-03 */
   char *nitedesc;   /* added NiteDesc -- Dracones */
   int vnum;
   int tele_vnum;
   int mpactnum;  /* mudprogs */
   short baselight; /* Preset light amount in this room */
   short light;     /* Modified amount of light in the room */
   short sector_type;
   short winter_sector; /* Stores the original sector type for stuff that freezes in winter - Samson 7-19-00 */
   short tele_delay;
   short tunnel;  /* max people that will fit */
   unsigned short mpscriptpos;
};

extern room_index *room_index_hash[MAX_KEY_HASH];

room_index *get_room_index( int );
room_index *make_room( int, area_data * );

#define EXIT( x, door)  ( (x)->in_room->get_exit( door ) )
#define CAN_GO(x, door) ( EXIT((x),(door)) && (EXIT((x),(door))->to_room != NULL )  \
                          && !IS_EXIT_FLAG( EXIT((x), (door)), EX_CLOSED ) )

/*
 * Delayed teleport type.
 */
struct teleport_data
{
   room_index *room;
   short timer;
};

extern list<teleport_data*> teleportlist;
#endif
