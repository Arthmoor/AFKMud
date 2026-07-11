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
 *                          Mobile Index Class Info                         *
 ****************************************************************************/

#pragma once

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
   void mprog_read_programs( std::ifstream & );

   class area_data *area = nullptr;             // Area this mob is associated with.
   SPEC_FUN *spec_fun = nullptr;                // Pointer to the special function this mob uses if one was specified.
   struct shop_data *pShop = nullptr;           // Data for a shop being run by this mob.
   struct repair_data *rShop = nullptr;         // Data for a repair shop being run by this mob.
   std::list<struct mud_prog_data *> mudprogs;  // List of Mudprogs this mob has.
   std::bitset<MAX_PROG> progtypes;
   std::bitset<MAX_ACT_FLAG> actflags;
   std::bitset<MAX_AFFECTED_BY> affected_by;    // What affect types the mobs has on it.
   std::bitset<MAX_ATTACK_TYPE> attacks;        // What types of attacks the mob has.
   std::bitset<MAX_DEFENSE_TYPE> defenses;      // What defenses the mob has.
   std::bitset<MAX_BPART> body_parts;           // What body parts the mob has.
   std::bitset<MAX_RIS_FLAG> resistant;         // What resistances the mob has.
   std::bitset<MAX_RIS_FLAG> immune;            // What immunities the mob has.
   std::bitset<MAX_RIS_FLAG> susceptible;       // What susceptibilities the mob has.
   std::bitset<MAX_RIS_FLAG> absorb;            // What the mob is able to absorb - Samson 3-16-00
   std::bitset<LANG_UNKNOWN> speaks;            // What languages the mob is capable of speaking.
   std::string player_name;                     // A list of keywords used to interact with this mob.
   std::string short_descr;                     // The visible name of the mob.
   std::string long_descr;                      // Description added after the name when seen in a room.
   std::string chardesc;                        // The detailed description when looked at by a player.
   std::string spec_funname;                    // Name of a special function if it has one.
   float numattacks = 1.0;                      // Number of attacks the mob has.
   int speaking = LANG_COMMON;                  // What language the mob is currently speaking. Don't bitset this - it should only be a single language at a time.
   int vnum = 0;                                // The mob's vnum.
   int gold = 0;                                // How much gold it has.
   int exp = -1;                                // How much experience the mob is worth when killed. -1 tells it to use the autocalc system.
   short count = 0;                             // Number of instances of this mob currently active.
   short killed = 0;                            // Number of times this mob has been killed.
   short sex = SEX_NEUTRAL;                     // Gender of the mob.
   short level = 1;                             // Level of the mob.
   short alignment = 0;                         // The mob's alignment.
   short mobthac0 = 21;                         // The mob's thac0 rating.
   short ac = 0;                                // The mob's armor class rating.
   short hitnodice = 0;                         // The number of hit dice the mob has.
   short hitsizedice = 0;                       // The size of each die used for hitnodice.
   short hitplus = 0;                           // Bonus applied to the roll of XdY where X is hitnodice and Y is hitsizedice.
   short damnodice = 0;                         // The number of barehanded damage dice the mob has.
   short damsizedice = 0;                       // The size of each barehanded damage die.
   short damplus = 0;                           // Bonus applied to the roll of XdY where X is the damnodice and Y is damsizedice.
   short max_move = 150;                        // Maximum movement points the mob has - Samson 7-14-00
   short max_mana = 150;                        // Maximum mana the mob has - Samson 7-14-00
   short position = POS_STANDING;               // The position the mob is in.
   short defposition = POS_STANDING;            // What position the mob will default to.
   short height = 0;                            // How tall is it in inches? 0 triggers an autocalc.
   short weight = 0;                            // How heavy is it in pounds? 0 triggers an autocalc.
   short race = RACE_HUMAN;                     // What race the mob is.
   short Class = CLASS_WARRIOR;                 // What class the mob is.
   short hitroll;                               // D&D thing I don't remember.
   short damroll;                               // D&D thing I don't remember.
   short perm_str = 13;                         // The mob's permanent strength stat.
   short perm_int = 13;                         // The mob's permanent intelligence stat.
   short perm_wis = 13;                         // The mob's permanent wisdom stat.
   short perm_dex = 13;                         // The mob's permanent dexterity stat.
   short perm_con = 13;                         // The mob's permanent constitution stat.
   short perm_cha = 13;                         // The mob's permanent charisma stat.
   short perm_lck = 13;                         // The mob's permanent luck stat.
   short saving_poison_death = 0;               // Saving throw vs poison and death.
   short saving_wand = 0;                       // Saving throw vs wands.
   short saving_para_petri = 0;                 // Saving throw vs petrification.
   short saving_breath = 0;                     // Saving throw vs breath attacks.
   short saving_spell_staff = 0;                // Saving throw vs spells and staves.
};

extern std::map<int, mob_index *> mob_index_table;
mob_index *get_mob_index( int );
mob_index *make_mobile( int, int, const std::string &, area_data * );
int interpolate( int, int, int );
