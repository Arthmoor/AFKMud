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
 *                              Skills Header                               *
 *                           Smaug Affect Header                            *
 ****************************************************************************/

#pragma once

#include <climits>

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

   std::list<class smaug_affect *> affects;   /* Spell affects, if any */
   std::bitset<MAX_SF_FLAG> flags;  /* Flags */
   SPELL_FUN *spell_fun;   /* Spell pointer (for spells) */
   DO_FUN *skill_fun;   /* Skill pointer (for skills) */
   char *name; /* Name of skill */
   char *spell_fun_name;   /* Spell function name - Trax */
   char *skill_fun_name;   /* Skill function name - Trax */
   char *noun_damage;   /* Damage message    */
   char *msg_off;    /* Wear off message     */
   char *hit_char;   /* Success message to caster  */
   char *hit_vict;   /* Success message to victim  */
   char *hit_room;   /* Success message to room */
   char *hit_dest;   /* Success message to dest room */
   char *miss_char;  /* Failure message to caster  */
   char *miss_vict;  /* Failure message to victim  */
   char *miss_room;  /* Failure message to room */
   char *die_char;   /* Victim death msg to caster */
   char *die_vict;   /* Victim death msg to victim */
   char *die_room;   /* Victim death msg to room   */
   char *imm_char;   /* Victim immune msg to caster   */
   char *imm_vict;   /* Victim immune msg to victim   */
   char *imm_room;   /* Victim immune msg to room  */
   char *dice; /* Dice roll         */
   char *author;  /* Skill's author */
   char *components; /* Spell components, if any   */
   char *teachers;   /* Skill requires a special teacher */
   char *helptext;   /* Help description for dynamic system */
   int info;   /* Spell action/class/etc  */
   int ego; /* Adjusted ego value used in object creation, accounts for SMAUG_AFF's */
   int value;  /* Misc value        */
   int spell_sector; /* Sector Spell work    */
   short skill_level[MAX_CLASS]; /* Level needed by class */
   short skill_adept[MAX_CLASS]; /* Max attainable % in this skill */
   short race_level[MAX_RACE];   /* Racial abilities: level */
   short race_adept[MAX_RACE];   /* Racial abilities: adept */
   short target;  /* Legal targets     */
   short minimum_position; /* Position for caster / user */
   short slot; /* Slot for #OBJECT loading   */
   short min_mana;   /* Minimum mana used    */
   short beats;   /* Rounds required to use skill */
   short guild;   /* Which guild the skill belongs to */
   short min_level;  /* Minimum level to be able to cast */
   short type; /* Spell/Skill/Weapon/Tongue  */
   short range;   /* Range of spell (rooms)  */
   short saves;   /* What saving spell applies  */
   short difficulty; /* Difficulty of casting/learning */
   short participants;  /* # of required participants */
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

   char *duration;
   char *modifier;
   int bit;
   short location;
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
