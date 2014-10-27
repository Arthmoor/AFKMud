/****************************************************************************
 *                   ^     +----- |  / ^     ^ |     | +-\                  *
 *                  / \    |      | /  |\   /| |     | |  \                 *
 *                 /   \   +---   |<   | \ / | |     | |  |                 *
 *                /-----\  |      | \  |  v  | |     | |  /                 *
 *               /       \ |      |  \ |     | +-----+ +-/                  *
 ****************************************************************************
 * AFKMud Copyright 1997-2010 by Roger Libiez (Samson),                     *
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
 *                          Object Index Class Info                         *
 ****************************************************************************/

#ifndef __OBJINDEX_H__
#define __OBJINDEX_H__

const int MAX_OBJ_VALUE = 11; // This should always be one more than you actually have

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

     list < affect_data * >affects;
     list < extra_descr_data * >extradesc;
     list < struct mud_prog_data *>mudprogs; /* Mudprogs */
   obj_index *next;
   area_data *area;
     bitset < MAX_PROG > progtypes; /* objprogs */
     bitset < MAX_ITEM_FLAG > extra_flags;
     bitset < MAX_WEAR_FLAG > wear_flags;
   char *name;
   char *short_descr;
   char *objdesc;
   char *action_desc;
   char *socket[3];  /* Name of rune/gem the item has in each socket - Samson 3-31-02 */
   int value[MAX_OBJ_VALUE];
   int vnum;
   int cost;
   int ego;
   int limit;  /* Limit on how many of these are allowed to load - Samson 1-9-00 */
   short level;
   short item_type;
   short count;
   short weight;
   short layers;
};

extern map < int, obj_index * >obj_index_table;
obj_index *get_obj_index( int );
obj_index *make_object( int, int, const string &, area_data * );
#endif
