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
 *                           Smaug Affect Header                            *
 ****************************************************************************/

#pragma once

#include <climits>

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
