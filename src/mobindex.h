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
 *                          Mobile Index Class Info                         *
 ****************************************************************************/

#ifndef __MOBINDEX_H__
#define __MOBINDEX_H__

/*
 * Prototype for a mob.
 * This is the in-memory version of #MOBILES.
 */
class mob_index
{
 private:
   mob_index( const mob_index & m );
     mob_index & operator=( const mob_index & );

 public:
     mob_index(  );
    ~mob_index(  );

   void clean_mob(  );
   char_data *create_mobile(  );
   void mprog_read_programs( FILE * fp );

   string spec_funname;
     list < struct mud_prog_data *>mudprogs; /* Mudprogs */
   area_data *area;
   SPEC_FUN *spec_fun;
   struct shop_data *pShop;
   struct repair_data *rShop;
     bitset < MAX_PROG > progtypes;
     bitset < MAX_ACT_FLAG > actflags;
     bitset < MAX_AFFECTED_BY > affected_by;
     bitset < MAX_ATTACK_TYPE > attacks;
     bitset < MAX_DEFENSE_TYPE > defenses;
     bitset < MAX_BPART > body_parts;
     bitset < MAX_RIS_FLAG > resistant;
     bitset < MAX_RIS_FLAG > immune;
     bitset < MAX_RIS_FLAG > susceptible;
     bitset < MAX_RIS_FLAG > absorb;   /* Samson 3-16-00 */
     bitset < LANG_UNKNOWN > speaks;
   char *player_name;
   char *short_descr;
   char *long_descr;
   char *chardesc;
   float numattacks;
   int speaking;  /* Don't bitset this - it should only be a single language at a time */
   int vnum;
   int gold;
   int exp;
   short count;
   short killed;
   short sex;
   short level;
   short alignment;
   short mobthac0;
   short ac;
   short hitnodice;
   short hitsizedice;
   short hitplus;
   short damnodice;
   short damsizedice;
   short damplus;
   short max_move;   /* Samson 7-14-00 */
   short max_mana;   /* Samson 7-14-00 */
   short position;
   short defposition;
   short height;
   short weight;
   short race;
   short Class;
   short hitroll;
   short damroll;
   short perm_str;
   short perm_int;
   short perm_wis;
   short perm_dex;
   short perm_con;
   short perm_cha;
   short perm_lck;
   short saving_poison_death;
   short saving_wand;
   short saving_para_petri;
   short saving_breath;
   short saving_spell_staff;
};

extern map < int, mob_index * >mob_index_table;
mob_index *get_mob_index( int );
mob_index *make_mobile( int, int, const string &, area_data * );
int interpolate( int, int, int );
#endif
