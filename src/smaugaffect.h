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
 *                              Skills Header                               *
 *                           Smaug Affect Header                            *
 ****************************************************************************/

#pragma once

#include <climits>

inline constexpr std::string_view SKILL_FILE       = "../system/skills.dat";    // Skill table. Holds all the data on every skill in the game.
inline constexpr std::string_view HERB_FILE        = "../system/herbs.dat";     // Herb table.

/*
 * Types of skill numbers.  Used to keep separate lists of sn's
 * Must be non-overlapping with spell/skill types,
 * but may be arbitrary beyond that.
 */
constexpr int TYPE_UNDEFINED = -1;
constexpr int TYPE_HIT = 1000;       /* allows for 1000 skills/spells */
constexpr int TYPE_HERB = 2000;      /* allows for 1000 attack types  */
constexpr int TYPE_PERSONAL = 3000;  /* allows for 1000 herb types    */
constexpr int TYPE_RACIAL = 4000;    /* allows for 1000 personal types */
constexpr int TYPE_DISEASE = 5000;   /* allows for 1000 racial types  */

// Target types.
enum target_types
{
   TAR_IGNORE, TAR_CHAR_OFFENSIVE, TAR_CHAR_DEFENSIVE, TAR_CHAR_SELF, TAR_OBJ_INV
};

enum skill_types
{
   SKILL_UNKNOWN, SKILL_SPELL, SKILL_SKILL, SKILL_COMBAT, SKILL_TONGUE,
   SKILL_HERB, SKILL_RACIAL, SKILL_DISEASE, SKILL_LORE
};

// * Skill/Spell flags
enum skill_spell_flags
{
   SF_WATER, SF_EARTH, SF_AIR, SF_ASTRAL, SF_AREA, SF_DISTANT,
   SF_REVERSE, SF_NOSELF, SF_NOCHARGE, SF_ACCUMULATIVE, SF_RECASTABLE,
   SF_NOSCRIBE, SF_NOBREW, SF_GROUPSPELL, SF_OBJECT, SF_CHARACTER,
   SF_SECRETSKILL, SF_PKSENSITIVE, SF_STOPONFAIL, SF_NOFIGHT,
   SF_NODISPEL, SF_RANDOMTARGET, SF_NOMOUNT, SF_NOMOB, MAX_SF_FLAG
};

// Skills include spells as a particular case.
class skill_type
{
private:
   skill_type( const skill_type & s );
   skill_type & operator=( const skill_type & );

public:
   skill_type(  );
   ~skill_type(  );

   std::list<class smaug_affect *> affects;  // Spell affects, if any.
   std::bitset<MAX_SF_FLAG> flags;           // Flags
   SPELL_FUN *spell_fun = nullptr;           // Spell pointer (for spells)
   DO_FUN *skill_fun = nullptr;              // Skill pointer (for skills)
   std::string name;                         // Name of skill.
   std::string author;                       // Skill's author.
   std::string spell_fun_name;               // Spell function name - Trax
   std::string skill_fun_name;               // Skill function name - Trax
   std::string noun_damage;                  // Damage message.
   std::string msg_off;                      // Wear off message.
   std::string hit_char;                     // Success message to caster.
   std::string hit_vict;                     // Success message to victim.
   std::string hit_room;                     // Success message to room.
   std::string hit_dest;                     // Success message to dest room.
   std::string miss_char;                    // Failure message to caster.
   std::string miss_vict;                    // Failure message to victim.
   std::string miss_room;                    // Failure message to room.
   std::string die_char;                     // Victim death msg to caster.
   std::string die_vict;                     // Victim death msg to victim.
   std::string die_room;                     // Victim death msg to room.
   std::string imm_char;                     // Victim immune msg to caster.
   std::string imm_vict;                     // Victim immune msg to victim.
   std::string imm_room;                     // Victim immune msg to room.
   std::string dice;                         // Dice roll.
   std::string components;                   // Spell components, if any.
   std::string teachers;                     // Skill requires a special teacher.
   std::string helptext;                     // Help description for dynamic system.
   int info = 0;                             // Spell action/class/etc.
   int ego = -2;                             // Adjusted ego value used in object creation, accounts for SMAUG_AFF's.
   int value = 0;                            // Misc value.
   int spell_sector = 0;                     // Sector Spell works in.
   short skill_level[MAX_CLASS]{0};          // Level needed by class.
   short skill_adept[MAX_CLASS]{0};          // Max attainable % in this skill.
   short race_level[MAX_RACE]{0};            // Racial abilities: level.
   short race_adept[MAX_RACE]{0};            // Racial abilities: adept.
   short target = 0;                         // Legal targets.
   short minimum_position = POS_STANDING;    // Position for caster / user.
   short slot = 0;                           // Slot for #OBJECT loading.
   short min_mana = 0;                       // Minimum mana used.
   short beats = 0;                          // Rounds required to use skill.
   short guild = -1;                         // Which guild the skill belongs to.
   short min_level = 0;                      // Minimum level to be able to cast.
   short type = 0;                           // Spell/Skill/Weapon/Tongue.
   short range = 0;                          // Range of spell (rooms).
   short saves = 0;                          // What saving spell applies.
   short difficulty = 0;                     // Difficulty of casting/learning.
   short participants = 0;                   // # of required participants.
};

extern skill_type *skill_table[MAX_SKILL];
extern skill_type *herb_table[MAX_HERB];
extern skill_type *disease_table[MAX_DISEASE];
skill_type *get_skilltype( int );

/*
 * A SMAUG spell
 */
class smaug_affect
{
 private:
   smaug_affect( const smaug_affect& s );
   smaug_affect& operator=( const smaug_affect& );

 public:
   smaug_affect();
   ~smaug_affect();

   std::string duration;
   std::string modifier;
   int bit = 0;
   short location = 0;
};

constexpr int ALL_BITS = INT_MAX;
constexpr int SDAM_MASK = ALL_BITS & ~(BV00 | BV01 | BV02);
constexpr int SACT_MASK = ALL_BITS & ~(BV03 | BV04 | BV05);
constexpr int SCLA_MASK = ALL_BITS & ~(BV06 | BV07 | BV08);
constexpr int SPOW_MASK = ALL_BITS & ~(BV09 | BV10);
constexpr int SSAV_MASK = ALL_BITS & ~(BV11 | BV12 | BV13);

enum save_types
{
   SS_NONE, SS_POISON_DEATH, SS_ROD_WANDS, SS_PARA_PETRI, SS_BREATH, SS_SPELL_STAFF
};

enum spell_dam_types
{
   SD_NONE, SD_FIRE, SD_COLD, SD_ELECTRICITY, SD_ENERGY, SD_ACID, SD_POISON, SD_DRAIN
};

enum spell_act_types
{
   SA_NONE, SA_CREATE, SA_DESTROY, SA_RESIST, SA_SUSCEPT, SA_DIVINATE, SA_OBSCURE, SA_CHANGE
};

enum spell_power_types
{
   SP_NONE, SP_MINOR, SP_GREATER, SP_MAJOR
};

enum spell_class_types
{
   SC_NONE, SC_LUNAR, SC_SOLAR, SC_TRAVEL, SC_SUMMON, SC_LIFE, SC_DEATH, SC_ILLUSION
};

enum spell_save_effects
{
   SE_NONE, SE_NEGATE, SE_EIGHTHDAM, SE_QUARTERDAM, SE_HALFDAM, SE_3QTRDAM, SE_REFLECT, SE_ABSORB
};

extern std::list<smaug_affect*> saflist;
bool SPELL_FLAG( const skill_type *, int );
int SPELL_DAMAGE( const skill_type * );
int SPELL_ACTION( const skill_type * );
int SPELL_CLASS( const skill_type * );
int SPELL_POWER( const skill_type * );
int SPELL_SAVE( const skill_type * );

/*
 * RIS by gsn lookups. -- Altrag.
 * Will need to add some || stuff for spells that need a special GSN.
 */
bool IS_FIRE( int );
bool IS_COLD( int );
bool IS_ACID( int );
bool IS_ELECTRICITY( int );
bool IS_ENERGY( int );
bool IS_DRAIN( int );
bool IS_POISON( int );
