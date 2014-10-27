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
 *                             Object Class Info                            *
 ****************************************************************************/

#ifndef __OBJECT_H__
#define __OBJECT_H__

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
   const string oshort(  );
   const string format_to_char( char_data *, bool, int );
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
   const string item_type_name(  );
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
   const string myobj(  );

   /*
    * External refs in other files 
    */
   void armorgen(  );
   void weapongen(  );

     list < obj_data * >contents;   /* Objects this object contains */
     list < affect_data * >affects;
     list < extra_descr_data * >extradesc;
     list < struct mprog_act_list *>mpact;   /* Mudprogs */
   obj_data *in_obj;
   obj_index *pIndexData;
   room_index *in_room;
   char_data *carried_by;
     bitset < MAX_ITEM_FLAG > extra_flags;
     bitset < MAX_WEAR_FLAG > wear_flags;
   char *name;
   char *short_descr;
   char *objdesc;
   char *action_desc;
   char *owner;   /* Who owns this item? Used with personal flag for Sindhae prizes. */
   char *seller;  /* Who put the item up for auction? */
   char *buyer;   /* Who made the final bid on the item? */
   char *socket[3];  /* Name of rune/gem the item has in each socket - Samson 3-31-02 */
   int value[11]; /* Raised to 11 by Samson on 12-14-02 */
   int bid; /* What was the amount of the final bid? */
   int mpactnum;  /* mudprogs */
   int ego;
   int room_vnum; /* Track it's room vnum for hotbooting and such */
   int cost;
   short wear_loc;
   short weight;
   short level;
   short timer;
   short count;   /* support for object grouping */
   short mx;   /* Object coordinates on overland maps - Samson 8-21-99 */
   short my;
   short map;  /* Which map is it on? - Samson 8-21-99 */
   short day;  /* What day of the week was it offered or sold? */
   short month;   /* What month? */
   short year; /* What year? */
   short item_type;
   unsigned short mpscriptpos;
};

obj_data *get_objtype( char_data *, short );
void obj_identify_output( char_data *, obj_data * );
void show_list_to_char( char_data *, list < obj_data * >, bool, bool );
void fwrite_obj( char_data *, list < obj_data * >, clan_data *, FILE *, int, bool );
void fread_obj( char_data *, FILE *, short );
obj_data *group_obj( obj_data * obj, obj_data * obj2 );

extern list < obj_data * >objlist;
extern obj_data *save_equipment[MAX_WEAR][MAX_LAYERS];
extern obj_data *mob_save_equipment[MAX_WEAR][MAX_LAYERS];

#endif
