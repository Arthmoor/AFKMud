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

#ifndef __SMAUG_AFFECT_H__
#define __SMAUG_AFFECT_H__

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

#define SPELL_FLAG(skill, flag) ( (skill)->flags.test(flag) )

#define SPELL_DAMAGE(skill)	( ((skill)->info      ) & 7 )
#define SPELL_ACTION(skill)	( ((skill)->info >>  3) & 7 )
#define SPELL_CLASS(skill)	( ((skill)->info >>  6) & 7 )
#define SPELL_POWER(skill)	( ((skill)->info >>  9) & 3 )
#define SPELL_SAVE(skill)	( ((skill)->info >> 11) & 7 )
#define SET_SDAM(skill, val)	( (skill)->info =  ((skill)->info & SDAM_MASK) + ((val) & 7) )
#define SET_SACT(skill, val)	( (skill)->info =  ((skill)->info & SACT_MASK) + (((val) & 7) << 3) )
#define SET_SCLA(skill, val)	( (skill)->info =  ((skill)->info & SCLA_MASK) + (((val) & 7) << 6) )
#define SET_SPOW(skill, val)	( (skill)->info =  ((skill)->info & SPOW_MASK) + (((val) & 3) << 9) )
#define SET_SSAV(skill, val)	( (skill)->info =  ((skill)->info & SSAV_MASK) + (((val) & 7) << 11) )

/* RIS by gsn lookups. -- Altrag.
   Will need to add some || stuff for spells that need a special GSN. */

#define IS_FIRE(dt)		( IS_VALID_SN(dt) && SPELL_DAMAGE(skill_table[(dt)]) == SD_FIRE )
#define IS_COLD(dt)		( IS_VALID_SN(dt) && SPELL_DAMAGE(skill_table[(dt)]) == SD_COLD )
#define IS_ACID(dt)		( IS_VALID_SN(dt) && SPELL_DAMAGE(skill_table[(dt)]) == SD_ACID )
#define IS_ELECTRICITY(dt)	( IS_VALID_SN(dt) && SPELL_DAMAGE(skill_table[(dt)]) == SD_ELECTRICITY )
#define IS_ENERGY(dt)		( IS_VALID_SN(dt) && SPELL_DAMAGE(skill_table[(dt)]) == SD_ENERGY )
#define IS_DRAIN(dt)		( IS_VALID_SN(dt) && SPELL_DAMAGE(skill_table[(dt)]) == SD_DRAIN )
#define IS_POISON(dt)		( IS_VALID_SN(dt) && SPELL_DAMAGE(skill_table[(dt)]) == SD_POISON )

enum save_types
{
   SS_NONE, SS_POISON_DEATH, SS_ROD_WANDS, SS_PARA_PETRI, SS_BREATH, SS_SPELL_STAFF
};

const int ALL_BITS = INT_MAX;
#define SDAM_MASK		ALL_BITS & ~(BV00 | BV01 | BV02)
#define SACT_MASK		ALL_BITS & ~(BV03 | BV04 | BV05)
#define SCLA_MASK		ALL_BITS & ~(BV06 | BV07 | BV08)
#define SPOW_MASK		ALL_BITS & ~(BV09 | BV10)
#define SSAV_MASK		ALL_BITS & ~(BV11 | BV12 | BV13)

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

extern list<smaug_affect*> saflist;
#endif
