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
 *                          Race and Class Header                           *
 ****************************************************************************/

#pragma once

// Race dedicated stuff.
class race_type
{
 private:
   race_type( const race_type & r );
     race_type & operator=( const race_type & );

 public:
     race_type(  );
    ~race_type(  );

   std::vector<std::string> bodypart_where_names;  // Body part wear messages.
   std::bitset<MAX_AFFECTED_BY> affected;          // Default affect bitvectors.
   std::bitset<MAX_ATTACK_TYPE> attacks;
   std::bitset<MAX_DEFENSE_TYPE> defenses;
   std::bitset<MAX_BPART> body_parts;              // Bodyparts this race has.
   std::bitset<LANG_UNKNOWN> language;             // Default racial language - can have multiples
   std::bitset<MAX_RIS_FLAG> resist;               // Bugfix: Samson 5-7-99
   std::bitset<MAX_RIS_FLAG> suscept;              // Bugfix: Samson 5-7-99
   std::bitset<MAX_CLASS> allowed_classes;         // Flags for allowed classes.
   std::string race_name;                          // Race name.
   short str_plus = 0;                             // Str bonus/penalty
   short dex_plus = 0;                             // Dex      "
   short wis_plus = 0;                             // Wis      "
   short int_plus = 0;                             // Int      "
   short con_plus = 0;                             // Con      "
   short cha_plus = 0;                             // Cha      "
   short lck_plus = 0;                             // Lck      "
   short hit = 0;
   short mana = 0;
   short ac_plus = 0;
   short alignment = 0;
   short minalign = 0;
   short maxalign = 0;
   short exp_multiplier = 0;
   short height = 0;
   short weight = 0;
   short hunger_mod = 0;
   short thirst_mod = 0;
   short saving_poison_death = 0;
   short saving_wand = 0;
   short saving_para_petri = 0;
   short saving_breath = 0;
   short saving_spell_staff = 0;
   short mana_regen = 0;
   short hp_regen = 0;
};

// Per-class stuff.
class class_type
{
 private:
   class_type( const class_type & c );
     class_type & operator=( const class_type & );

 public:
     class_type(  );
    ~class_type(  );

   std::bitset<MAX_AFFECTED_BY> affected; // Default affect bitvectors.
   std::bitset<MAX_RIS_FLAG> resist;
   std::bitset<MAX_RIS_FLAG> suscept;
   std::string who_name;      // Name for 'who'.
   float thac0_gain = 0.0;    // Thac0 amount gained per level - Dwip 5-11-01
   int weapon = 0;            // Vnum of Weapon given at creation.
   int armor = 0;             // Vnum of Body Armor given at creation - Samson
   int legwear = 0;           // Vnum of Legwear given at creation - Samson 1-3-99
   int headwear = 0;          // Vnum of Headwear given at creation - Samson 1-3-99
   int armwear = 0;           // Vnum of Armwear given at creation - Samson 1-3-99
   int footwear = 0;          // Vnum of Footwear given at creation - Samson 1-3-99
   int shield = 0;            // Vnum of Shield given at creation - Samson 1-3-99
   int held = 0;              // Vnum of held item given at creation - Samson 1-3-99
   int base_thac0 = 0;        // Thac0 for level 1 - Dwip 5-11-01
   short hp_min = 0;          // Min hp gained on leveling.
   short hp_max = 0;          // Max hp gained on leveling.
   short skill_adept = 0;     // Maximum skill level.
   short attr_prime = 0;      // Prime attribute.
   bool fMana = false;        // Class gains mana on level?
};

extern race_type *race_table[MAX_RACE];
extern class_type *class_table[MAX_CLASS];
