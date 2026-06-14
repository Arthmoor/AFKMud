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
 *                          Shaddai's Polymorph                             *
 ****************************************************************************/

#pragma once

inline constexpr std::string_view MORPH_FILE = "morph.dat";   /* For morph data */

/*
 * Structure for a morph -- Shaddai
 */
/*
 *  Morph structs.
 */

constexpr int ONLY_PKILL = 1;
constexpr int ONLY_PEACEFULL = 2;

class morph_data
{
 private:
   morph_data( const morph_data & m );
     morph_data & operator=( const morph_data & );

 public:
     morph_data(  );
    ~morph_data(  );

   std::bitset<MAX_AFFECTED_BY> affected_by;    // New affected_by added.
   std::bitset<MAX_AFFECTED_BY> no_affected_by; // Prevents affects from being added.
   std::bitset<MAX_CLASS> Class;                // Classes not allowed to use this.
   std::bitset<MAX_RACE>race;                   // Races not allowed to use this.
   std::bitset<MAX_RIS_FLAG> no_immune;         // Prevents Immunities.
   std::bitset<MAX_RIS_FLAG> no_resistant;      // Prevents resistances.
   std::bitset<MAX_RIS_FLAG> no_suscept;        // Prevents Susceptibilities.
   std::bitset<MAX_RIS_FLAG> immune;            // Immunities added.
   std::bitset<MAX_RIS_FLAG> resistant;         // Resistances added.
   std::bitset<MAX_RIS_FLAG> suscept;           // Suscepts added.
   std::bitset<MAX_RIS_FLAG> absorb;            // Absorbs added - Samson 3-16-00
   char *damroll = nullptr;                     // Bonus to damage. This is a char* because it can be written as dice formulas to pass to dice_parse().
   char *deity = nullptr;                       // Players worshiping this deity cannot use this morph.
   char *description = nullptr;
   char *help = nullptr;                        // What player sees for info on morph.
   char *hit = nullptr;                         // Hitpoints added. This is a char* because it can be written as dice formulas to pass to dice_parse().
   char *hitroll = nullptr;                     // Bonus to hit. This is a char* because it can be written as dice formulas to pass to dice_parse().
   char *key_words = nullptr;                   // Keywords added to your name.
   char *long_desc = nullptr;                   // New long_desc for player.
   char *mana = nullptr;                        // Mana added. This is a char* because it can be written as dice formulas to pass to dice_parse().
   char *morph_other = nullptr;                 // What others see when you morph.
   char *morph_self = nullptr;                  // What you see when you morph.
   char *move = nullptr;                        // Movement points added. This is a char* because it can be written as dice formulas to pass to dice_parse().
   char *name = nullptr;                        // Name used to polymorph into this.
   char *short_desc = nullptr;                  // New short desc for player.
   char *no_skills = nullptr;                   // Prevented Skills.
   char *skills = nullptr;
   char *unmorph_other = nullptr;               // What others see when you unmorph.
   char *unmorph_self = nullptr;                // What you see when you unmorph.
   int obj[3]{0};                               // Object needed to morph you.
   int defpos = POS_STANDING;                   // Default position.
   int timer = -1;                              // Timer for how long it lasts.
   int used = 0;                                // How many times has this morph been used.
   int vnum = 0;                                // Unique identifier.
   short ac = 0;
   short cha = 0;                               // Amount CHA gained/lost.
   short con = 0;                               // Amount of CON gained/lost.
   short dayfrom = -1;                          // Starting Day you can morph into this.
   short dayto = -1;                            // Ending Day you can morph into this.
   short dex = 0;                               // Amount of DEX gaines/lost.
   short dodge = 0;                             // Percent of dodge added IE 1 = 1%
   short favourused = 0;                        // Amount of favour to morph.
   short hpused = 0;                            // Amount of HP used to morph.
   short inte = 0;                              // Amount of INT gained/lost.
   short lck = 0;                               // Amount of LCK gained/lost.
   short level = 0;                             // Minimum level to use this morph.
   short manaused = 0;                          // Amount of mana used to morph.
   short moveused = 0;                          // Amount of movement points used to morph.
   short parry = 0;                             // Percent of parry added IE 1 = 1%
   short pkill = 0;                             // Pkill Only, Peaceful Only or Both
   short saving_breath = 0;                     // Below are saving adjusted.
   short saving_para_petri = 0;
   short saving_poison_death = 0;
   short saving_spell_staff = 0;
   short saving_wand = 0;
   short sex = -1;                              // The sex that can morph into this.
   short str = 0;                               // Amount of STR gained/lost.
   short timefrom = -1;                         // Hour starting you can morph.
   short timeto = -1;                           // Hour ending that you can morph.
   short tumble = 0;                            // Percent of tumble added IE 1 = 1%
   short wis = 0;                               // Amount of WIS gained/lost.
   bool objuse[3]{0};                           // Objects needed to morph.
   bool no_cast = false;                        // Can you cast a spell to morph into it?
   bool cast_allowed = false;                   // Can you cast spells whilst morphed into this?
};

// Scaled down version of original morph so if morph gets changed stats don't get messed up.
class char_morph
{
 private:
   char_morph( const char_morph & m );
     char_morph & operator=( const char_morph & );

 public:
   char_morph(  );
   ~char_morph(  );

   morph_data *morph = nullptr;
   std::bitset<MAX_AFFECTED_BY> affected_by;    // New affected_by added/
   std::bitset<MAX_AFFECTED_BY> no_affected_by; // Prevents affects from being added.
   std::bitset<MAX_RIS_FLAG> no_immune;         // Prevents Immunities.
   std::bitset<MAX_RIS_FLAG> no_resistant;      // Prevents resistances.
   std::bitset<MAX_RIS_FLAG> no_suscept;        // Prevents Susceptibilities.
   std::bitset<MAX_RIS_FLAG> resistant;         // Resistances added.
   std::bitset<MAX_RIS_FLAG> suscept;           // Suscepts added.
   std::bitset<MAX_RIS_FLAG> immune;            // Immunities added.
   std::bitset<MAX_RIS_FLAG> absorb;            // Absorbs added.
   int timer = -1;                              // Timer for how long it lasts.
   short ac;
   short cha;
   short con;
   short damroll;
   short dex;
   short dodge;
   short hit;
   short hitroll;
   short inte;
   short lck;
   short mana;
   short move;
   short parry;
   short saving_breath;
   short saving_para_petri;
   short saving_poison_death;
   short saving_spell_staff;
   short saving_wand;
   short str;
   short tumble;
   short wis;
   bool cast_allowed;   /* Casting allowed whilst morphed */
};

int do_morph_char( char_data *, morph_data * );
void do_unmorph_char( char_data * );
const std::string MORPHPERS( char_data *, char_data *, bool );
