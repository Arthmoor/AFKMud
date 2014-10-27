/****************************************************************************
 *                   ^     +----- |  / ^     ^ |     | +-\                  *
 *                  / \    |      | /  |\   /| |     | |  \                 *
 *                 /   \   +---   |<   | \ / | |     | |  |                 *
 *                /-----\  |      | \  |  v  | |     | |  /                 *
 *               /       \ |      |  \ |     | +-----+ +-/                  *
 ****************************************************************************
 * AFKMud Copyright 1997-2012 by Roger Libiez (Samson),                     *
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
 *                       Rune/Gem socketing module                          *
 *                 Inspired by the system used in Diablo 2                  *
 *             Also contains the random treasure creation code              *
 ****************************************************************************/

#ifndef __TREASURE_H__
#define __TREASURE_H__

#define RUNE_FILE SYSTEM_DIR "runes.dat"
#define RUNEWORD_FILE SYSTEM_DIR "runeword.dat"
#define WTYPE_FILE SYSTEM_DIR "weapontypes.dat"
#define ATYPE_FILE SYSTEM_DIR "armortypes.dat"
#define MAT_FILE SYSTEM_DIR "materials.dat"

enum rune_rarities
{
   RUNE_COMMON, RUNE_RARE, RUNE_ULTRARARE
};

/* Materials for Armor Generator and Weapon Generator */
struct armorgenM
{
   short material;   /* Type of material */
   const char *name; /* Descriptive name */
   float weight;  /* Modification to weight */
   short ac;   /* Modification to armor class */
   short wd;   /* Modification to weapon damage */
   float cost; /* Modification to item value or cost */
   short minlevel;   /* Minimum mob level before this material will drop */
   short maxlevel;   /* Maximum mob level this material will drop for */
};

/* Armor types for Armor Generator */
struct armorgenT
{
   short type; /* Armor type */
   const char *name; /* Descriptive name */
   float weight;  /* Base weight */
   short ac;   /* Base armor class */
   float cost; /* Base value or cost */
   short minlevel;   /* Minimum mob level before this armor type will drop */
   short maxlevel;   /* Maximum mob level this armor type will drop for */
   string flags;   /* Default flag set */
};

/* Weapon types for Weapon Generator */
struct weaponT
{
   short type; /* Weapon type */
   const char *name; /* Descriptive name */
   short wd;   /* Base damage */
   float weight;  /* Base weight */
   float cost; /* Base cost/value */
   short skill;   /* Skill type */
   short damage;  /* Damage type */
   string flags;   /* Default flag set */
};

struct weapontable
{
   weapontable(  );

   const char *name; /* Descriptive name */
   char *flags;   /* Default flag set */
   float weight;  /* Base weight */
   float cost; /* Base cost/value */
   short type; /* Weapon type */
   short basedam; /* Base damage */
   short skill;   /* Skill type */
   short damtype; /* Damage type */
};

class rune_data
{
 private:
   rune_data( const rune_data & r );
     rune_data & operator=( const rune_data & );

 public:
     rune_data(  );
    ~rune_data(  );

   void set_name( const string & name )
   {
      _name = name;
   }
   string get_name(  )
   {
      return _name;
   }
   const char *get_cname(  )
   {
      return _name.c_str(  );
   }

   void set_rarity( short rarity )
   {
      _rarity = rarity;
   }
   short get_rarity(  )
   {
      return _rarity;
   }

 private:
   string _name;
   short _rarity; /* Common, Rare, Ultrarare */

 public:
   int stat1[2];  /* The stat to modify goes in the first spot, modifier value in the second. */
   int stat2[2];  /* Stat1 is for weapons, Stat2 is for armors */
};

class runeword_data
{
 private:
   runeword_data( const runeword_data & r );
     runeword_data & operator=( const runeword_data & );

 public:
     runeword_data(  );
    ~runeword_data(  );

   void set_name( const string & name )
   {
      _name = name;
   }
   string get_name(  )
   {
      return _name;
   }
   const char *get_cname(  )
   {
      return _name.c_str(  );
   }

   void set_rune1( const string & rune1 )
   {
      _rune1 = rune1;
   }
   string get_rune1(  )
   {
      return _rune1;
   }
   const char *get_crune1(  )
   {
      return _rune1.c_str(  );
   }

   void set_rune2( const string & rune2 )
   {
      _rune2 = rune2;
   }
   string get_rune2(  )
   {
      return _rune2;
   }
   const char *get_crune2(  )
   {
      return _rune2.c_str(  );
   }

   void set_rune3( const string & rune3 )
   {
      _rune3 = rune3;
   }
   string get_rune3(  )
   {
      return _rune3;
   }
   const char *get_crune3(  )
   {
      return _rune3.c_str(  );
   }

   void set_type( short type )
   {
      _type = type;
   }
   short get_type(  )
   {
      return _type;
   }

 private:
   string _name;  /* The runeword name */
   string _rune1; /* 1st required rune */
   string _rune2; /* 2nd required rune */
   string _rune3; /* 3rd required rune - NULL if not required */
   short _type;   /* Weapon(1) or Armor(0) ? */

 public:
   int stat1[2];  /* Affects the runeword transfers to the item */
   int stat2[2];
   int stat3[2];
   int stat4[2];
};

extern const struct weaponT weapon_type[];
extern const struct armorgenT armor_type[];
extern const struct armorgenM materials[];
extern const char *weapon_quality[];

// Refer to the tables in treasure.c to see what these affect.
const int TMAT_MAX = 14;
const int TATP_MAX = 17;
const int TWTP_MAX = 14;
const int TQUAL_MAX = 5;
#endif
