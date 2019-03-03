/****************************************************************************
 *                   ^     +----- |  / ^     ^ |     | +-\                  *
 *                  / \    |      | /  |\   /| |     | |  \                 *
 *                 /   \   +---   |<   | \ / | |     | |  |                 *
 *                /-----\  |      | \  |  v  | |     | |  /                 *
 *               /       \ |      |  \ |     | +-----+ +-/                  *
 ****************************************************************************
 * AFKMud Copyright 1997-2019 by Roger Libiez (Samson),                     *
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
 *                           Mud constants module                           *
 ****************************************************************************/

#include "mud.h"
#include "area.h"

/* Global Skill Numbers */
// Combat skills: Changed to reflect new weapon types - Grimm
// Consolidated styles in with these - Samson 2/23/06
short gsn_pugilism;
short gsn_swords;
short gsn_daggers;
short gsn_whips;
short gsn_talonous_arms;
short gsn_maces_hammers;
short gsn_crossbows;
short gsn_bows;
short gsn_blowguns;
short gsn_slings;
short gsn_axes;
short gsn_spears;
short gsn_staves;
short gsn_archery;
short gsn_style_evasive;
short gsn_style_defensive;
short gsn_style_standard;
short gsn_style_aggressive;
short gsn_style_berserk;

/* monk */
short gsn_feign;
short gsn_quiv;

/* rogue */
short gsn_assassinate;
short gsn_detrap;
short gsn_backstab;
short gsn_circle;
short gsn_dodge;
short gsn_hide;
short gsn_peek;
short gsn_pick_lock;
short gsn_sneak;
short gsn_steal;
short gsn_gouge;
short gsn_poison_weapon;
short gsn_spy;

/* rogue & warrior */
short gsn_disarm;
short gsn_enhanced_damage;
short gsn_kick;
short gsn_parry;
short gsn_rescue;
short gsn_dual_wield;
short gsn_punch;
short gsn_bash;
short gsn_stun;
short gsn_bashdoor;
short gsn_grip;
short gsn_berserk;
short gsn_hitall;
short gsn_tumble;
short gsn_grapple;
short gsn_cleave;
short gsn_retreat;   /* Samson 5-27-99 */

/* other   */
short gsn_aid;
short gsn_track;
short gsn_search;
short gsn_dig;
short gsn_mount;
short gsn_bite;
short gsn_claw;
short gsn_sting;
short gsn_tail;
short gsn_scribe;
short gsn_brew;
short gsn_climb;
short gsn_cook;
short gsn_slice;
short gsn_charge;

/* Racials */
short gsn_forage; /* Samson 3-26-00 */
short gsn_woodcall;  /* Samson 4-17-00 */
short gsn_mining; /* Samson 4-17-00 */
short gsn_bladesong; /* Samson 4-23-00 */
short gsn_elvensong; /* Samson 4-23-00 */
short gsn_reverie;   /* Samson 4-23-00 */
short gsn_bargain;   /* Samson 4-23-00 */
short gsn_tenacity;  /* Samson 4-24-00 */
short gsn_swim;   /* Samson 4-24-00 */
short gsn_deathsong; /* Samson 4-25-00 */
short gsn_tinker; /* Samson 4-25-00 */
short gsn_scout;  /* Samson 5-29-00 */
short gsn_metallurgy;   /* Samson 5-31-00 */
short gsn_backheel;  /* Samson 5-31-00 */

/* spells */
short gsn_aqua_breath;
short gsn_blindness;
short gsn_charm_person;
short gsn_curse;
short gsn_invis;
short gsn_mass_invis;
short gsn_poison;
short gsn_sleep;
short gsn_fireball;
short gsn_chill_touch;
short gsn_lightning_bolt;
short gsn_paralyze;  /* Samson 9-26-98 */
short gsn_silence;   /* Samson 9-26-98 */
short gsn_scry;   /* Samson 5-29-00 */

/* languages */
short gsn_common;
short gsn_elven;
short gsn_dwarven;
short gsn_pixie;
short gsn_ogre;
short gsn_orcish;
short gsn_trollish;
short gsn_goblin;
short gsn_halfling;

// The total number of skills.
// Note that the range [0; num_sorted_skills[ is
// the only range that can be b-searched.
//
// The range [num_sorted_skills; num_skills[ is for
// skills added during the game; we cannot resort the
// skills due to there being direct indexing into the
// skill array. So, we have this additional linear
// range for the skills added at runtime.
short num_skills;
// The number of sorted skills. (see above)
short num_sorted_skills;

/* For styles?  Trying to rebuild from some kind of accident here - Blod */
short gsn_tan;
short gsn_dragon_ride;

void assign_gsn_data( void )
{
   ASSIGN_GSN( gsn_style_evasive, "evasive style" );
   ASSIGN_GSN( gsn_style_defensive, "defensive style" );
   ASSIGN_GSN( gsn_style_standard, "standard style" );
   ASSIGN_GSN( gsn_style_aggressive, "aggressive style" );
   ASSIGN_GSN( gsn_style_berserk, "berserk style" );

   ASSIGN_GSN( gsn_tan, "tan" );
   ASSIGN_GSN( gsn_dragon_ride, "dragon ride" );
   ASSIGN_GSN( gsn_retreat, "retreat" );
   ASSIGN_GSN( gsn_charge, "bind" );
   ASSIGN_GSN( gsn_spy, "spy" );
   ASSIGN_GSN( gsn_forage, "forage" );
   ASSIGN_GSN( gsn_woodcall, "woodcall" );
   ASSIGN_GSN( gsn_mining, "mining" );
   ASSIGN_GSN( gsn_bladesong, "bladesong" );
   ASSIGN_GSN( gsn_elvensong, "elvensong" );
   ASSIGN_GSN( gsn_reverie, "reverie" );
   ASSIGN_GSN( gsn_bargain, "bargain" );
   ASSIGN_GSN( gsn_tenacity, "tenacity" );
   ASSIGN_GSN( gsn_swim, "swim" );
   ASSIGN_GSN( gsn_deathsong, "deathsong" );
   ASSIGN_GSN( gsn_tinker, "tinker" );
   ASSIGN_GSN( gsn_scout, "scout" );
   ASSIGN_GSN( gsn_scry, "scry" );
   ASSIGN_GSN( gsn_metallurgy, "metallurgy" );
   ASSIGN_GSN( gsn_backheel, "backheel" );

   /*
    * new gsn assigns for the new weapon skills - Grimm 
    */
   ASSIGN_GSN( gsn_pugilism, "pugilism" );
   ASSIGN_GSN( gsn_swords, "swords" );
   ASSIGN_GSN( gsn_daggers, "daggers" );
   ASSIGN_GSN( gsn_whips, "whips" );
   ASSIGN_GSN( gsn_talonous_arms, "talonous arms" );
   ASSIGN_GSN( gsn_maces_hammers, "maces and hammers" );
   ASSIGN_GSN( gsn_blowguns, "blowguns" );
   ASSIGN_GSN( gsn_slings, "slings" );
   ASSIGN_GSN( gsn_axes, "axes" );
   ASSIGN_GSN( gsn_spears, "spears" );
   ASSIGN_GSN( gsn_staves, "staves" );
   ASSIGN_GSN( gsn_archery, "archery" );

   ASSIGN_GSN( gsn_assassinate, "assassinate" );
   ASSIGN_GSN( gsn_feign, "feign death" );
   ASSIGN_GSN( gsn_quiv, "quivering palm" );
   ASSIGN_GSN( gsn_detrap, "detrap" );
   ASSIGN_GSN( gsn_backstab, "backstab" );
   ASSIGN_GSN( gsn_circle, "circle" );
   ASSIGN_GSN( gsn_tumble, "tumble" );
   ASSIGN_GSN( gsn_dodge, "dodge" );
   ASSIGN_GSN( gsn_hide, "hide" );
   ASSIGN_GSN( gsn_peek, "peek" );
   ASSIGN_GSN( gsn_pick_lock, "pick lock" );
   ASSIGN_GSN( gsn_sneak, "sneak" );
   ASSIGN_GSN( gsn_steal, "steal" );
   ASSIGN_GSN( gsn_gouge, "gouge" );
   ASSIGN_GSN( gsn_poison_weapon, "poison weapon" );
   ASSIGN_GSN( gsn_disarm, "disarm" );
   ASSIGN_GSN( gsn_enhanced_damage, "enhanced damage" );
   ASSIGN_GSN( gsn_kick, "kick" );
   ASSIGN_GSN( gsn_parry, "parry" );
   ASSIGN_GSN( gsn_rescue, "rescue" );
   ASSIGN_GSN( gsn_dual_wield, "dual wield" );
   ASSIGN_GSN( gsn_punch, "punch" );
   ASSIGN_GSN( gsn_bash, "bash" );
   ASSIGN_GSN( gsn_stun, "stun" );
   ASSIGN_GSN( gsn_bashdoor, "doorbash" );
   ASSIGN_GSN( gsn_grip, "grip" );
   ASSIGN_GSN( gsn_berserk, "berserk" );
   ASSIGN_GSN( gsn_hitall, "hitall" );
   ASSIGN_GSN( gsn_aid, "aid" );
   ASSIGN_GSN( gsn_track, "track" );
   ASSIGN_GSN( gsn_search, "search" );
   ASSIGN_GSN( gsn_dig, "dig" );
   ASSIGN_GSN( gsn_mount, "mount" );
   ASSIGN_GSN( gsn_bite, "bite" );
   ASSIGN_GSN( gsn_claw, "claw" );
   ASSIGN_GSN( gsn_sting, "sting" );
   ASSIGN_GSN( gsn_tail, "tail" );
   ASSIGN_GSN( gsn_scribe, "scribe" );
   ASSIGN_GSN( gsn_brew, "brew" );
   ASSIGN_GSN( gsn_climb, "climb" );
   ASSIGN_GSN( gsn_cook, "cook" );
   ASSIGN_GSN( gsn_slice, "slice" );
   ASSIGN_GSN( gsn_grapple, "grapple" );
   ASSIGN_GSN( gsn_cleave, "cleave" );
   ASSIGN_GSN( gsn_fireball, "fireball" );
   ASSIGN_GSN( gsn_chill_touch, "chill touch" );
   ASSIGN_GSN( gsn_lightning_bolt, "lightning bolt" );
   ASSIGN_GSN( gsn_aqua_breath, "aqua breath" );
   ASSIGN_GSN( gsn_blindness, "blindness" );
   ASSIGN_GSN( gsn_charm_person, "charm person" );
   ASSIGN_GSN( gsn_curse, "curse" );
   ASSIGN_GSN( gsn_invis, "invis" );
   ASSIGN_GSN( gsn_mass_invis, "mass invis" );
   ASSIGN_GSN( gsn_poison, "poison" );
   ASSIGN_GSN( gsn_sleep, "sleep" );
   ASSIGN_GSN( gsn_paralyze, "paralyze" );   /* Samson 9-26-98 */
   ASSIGN_GSN( gsn_silence, "silence" );  /* Samson 9-26-98 */
   ASSIGN_GSN( gsn_common, "common" );
   ASSIGN_GSN( gsn_elven, "elven" );
   ASSIGN_GSN( gsn_dwarven, "dwarven" );
   ASSIGN_GSN( gsn_pixie, "pixie" );
   ASSIGN_GSN( gsn_ogre, "ogre" );
   ASSIGN_GSN( gsn_orcish, "orcish" );
   ASSIGN_GSN( gsn_trollish, "trollese" );
   ASSIGN_GSN( gsn_goblin, "goblin" );
   ASSIGN_GSN( gsn_halfling, "halfling" );
}

/*
 * Race table.
 */
const char *npc_race[MAX_NPC_RACE] = {
   // Playable Races - MAX_RACE in mudcfg.h must be raised before more can be added after r9.
   // The race_types enum also needs to be syncd with this.
   "human", "high-elf", "dwarf", "halfling", "pixie", "half-ogre", "half-orc",     // 6 (starts from 0)
   "half-troll", "half-elf", "gith", "minotaur", "duergar", "centaur",             // 12
   "iguanadon", "gnome", "drow", "wild-elf", "insectoid", "sahuagin", "r9",        // 19

   // NPCs only
   "halfbreed", "reptile", "Mysterion", "lycanthrope", "dragon", "undead",         // 25
   "orc", "insect", "spider", "dinosaur", "fish", "avis", "Giant",                 // 32
   "Carnivorous", "Parasitic", "slime", "Demonic", "snake", "Herbivorous", "Tree", // 39
   "Vegan", "Elemental", "Planar", "Diabolic", "ghost", "goblin", "troll",         // 46
   "Vegman", "Mindflayer", "Primate", "Enfan", "golem", "Aarakocra", "troglodyte", // 53
   "Patryn", "Labrynthian", "Sartan", "Titan", "Smurf", "Kangaroo", "horse",       // 60
   "Ratperson", "Astralion", "god", "Hill Giant", "Frost Giant", "Fire Giant",     // 66
   "Cloud Giant", "Storm Giant", "Stone Giant", "Red Dragon", "Black Dragon",      // 71
   "Green Dragon", "White Dragon", "Blue Dragon", "Silver Dragon", "Gold Dragon",  // 76
   "Bronze Dragon", "Copper Dragon", "Brass Dragon", "Vampire", "Lich", "wight",   // 82
   "Ghast", "Spectre", "zombie", "skeleton", "ghoul", "Half Giant", "Deep Gnome",  // 89
   "gnoll", "Sylvan Elf", "Sea-Elf", "Tiefling", "Aasimar", "Solar", "Planitar",   // 96
   "shadow", "Giant Skeleton", "Nilbog", "Houser", "Baku", "Beast Lord", "Deva",   // 103
   "Polaris", "Demodand", "Tarasque", "Diety", "Daemon", "Vagabond",               // 109
   "gargoyle", "bear", "bat", "cat", "dog", "ant", "ape", "baboon",                // 117
   "bee", "beetle", "boar", "bugbear", "ferret", "fly", "gelatin", "gorgon",       // 125
   "harpy", "hobgoblin", "kobold", "locust", "mold", "mule",                       // 131
   "neanderthal", "ooze", "rat", "rustmonster", "shapeshifter", "shrew",           // 137
   "shrieker", "stirge", "thoul", "wolf", "worm", "bovine", "canine",              // 144
   "feline", "porcine", "mammal", "rodent", "amphibian", "crustacean",             // 150
   "spirit", "magical", "animal", "humanoid", "monster", "???", "???", "???",      // 158
   "???", "???", "???"                                                             // 161
};

/*
 * Class table.
 */
const char *npc_class[MAX_NPC_CLASS] = {
   // Playable classes - MAX_CLASS in mudcfg.h must be raised before more can be added after pc19.
   // The class_types enum also needs to be synced with this.
   "Mage", "Cleric", "Rogue", "Warrior", "Necromancer", "Druid", "Ranger",
   "Monk", "pc8", "pc9", "Antipaladin", "Paladin", "Bard", "pc13",
   "pc14", "pc15", "pc16", "pc17", "pc18", "pc19",

   // NPCs only
   "baker", "butcher", "blacksmith", "mayor", "king", "queen"
};

/*
 * Attribute bonus tables.
 */
/* Strength bonuses altered to reflect 3rd Edition AD&D rules */
const struct str_app_type str_app[26] = {
   {-5, -5, 0, 0},   /* 0  */
   {-5, -4, 10, 1},  /* 1  */
   {-4, -2, 20, 2},
   {-4, -1, 30, 3},  /* 3  */
   {-2, -1, 40, 4},
   {-2, -1, 50, 5},  /* 5  */
   {-1, 0, 60, 6},
   {-1, 0, 70, 7},
   {0, 0, 80, 8},
   {0, 0, 90, 9},
   {0, 0, 100, 10},  /* 10  */
   {0, 0, 115, 11},
   {0, 0, 130, 12},
   {0, 0, 150, 13},  /* 13  */
   {0, 1, 175, 14},
   {1, 1, 200, 15},  /* 15  */
   {1, 2, 230, 16},
   {2, 3, 260, 22},
   {2, 4, 300, 25},  /* 18  */
   {3, 5, 350, 30},
   {3, 6, 400, 35},  /* 20  */
   {4, 7, 460, 40},
   {5, 7, 520, 45},
   {6, 8, 600, 50},
   {8, 10, 700, 55},
   {10, 12, 800, 60} /* 25   */
};

/* Intelligence tables converted to 3rd Edition AD&D rules - determines skill % you learn when practicing.
 * Base increase is 4, then add modifier for total learned per practice.
 */
const struct int_app_type int_app[26] = {
   {-5}, /*  0 */
   {-5}, /*  1 */
   {-4},
   {-4}, /*  3 */
   {-3},
   {-3}, /*  5 */
   {-2},
   {-2},
   {-1},
   {-1},
   {0},  /* 10 */
   {0},
   {1},
   {1},
   {2},
   {2},  /* 15 */
   {3},
   {3},
   {4},  /* 18 */
   {4},
   {5},  /* 20 */
   {5},
   {6},
   {6},
   {7},
   {7}   /* 25 */
};

/* Wisdom tables changed to 3rd Edition AD&D rules - determines number of practices per level gained.
 * Base increase is 4, then add the modifer for total gained per level.
 * Also affect the amount of mana gained during regeneration.
 */
const struct wis_app_type wis_app[26] = {
   {-5}, /*  0 */
   {-5}, /*  1 */
   {-4},
   {-4}, /*  3 */
   {-3},
   {-3}, /*  5 */
   {-2},
   {-2},
   {-1},
   {-1},
   {0},  /* 10 */
   {0},
   {1},
   {1},
   {2},
   {2},  /* 15 */
   {3},
   {3},
   {4},  /* 18 */
   {4},
   {5},  /* 20 */
   {5},
   {6},
   {6},
   {7},
   {7}   /* 25 */
};

/* Deterity table: Not converted. Bonuses here are adequate.
 * Adds the modifier to armor Class.
 */
const struct dex_app_type dex_app[26] = {
   {60}, /* 0 */
   {50}, /* 1 */
   {50},
   {40},
   {30},
   {20}, /* 5 */
   {10},
   {0},
   {0},
   {0},
   {0},  /* 10 */
   {0},
   {0},
   {0},
   {0},
   {-10},   /* 15 */
   {-15},
   {-20},
   {-30},
   {-40},
   {-50},   /* 20 */
   {-60},
   {-75},
   {-90},
   {-105},
   {-120}   /* 25 */
};

/* Constitution tables: Converted to 3rd Edition AD&D rules.
 * Left side bonus adds to hit points gained at level up, and regeneration rates during rest.
 * Unsure what Smaug had in mind for the right side, it's not used anywhere yet.
 */
const struct con_app_type con_app[26] = {
   {-5, 20},   /*  0 */
   {-5, 25},   /*  1 */
   {-4, 30},
   {-4, 35},   /*  3 */
   {-3, 40},
   {-3, 45},   /*  5 */
   {-2, 50},
   {-2, 55},
   {-1, 60},
   {-1, 65},
   {0, 70}, /* 10 */
   {0, 75},
   {1, 80},
   {1, 85},
   {2, 88},
   {2, 90}, /* 15 */
   {3, 95},
   {3, 97},
   {4, 99}, /* 18 */
   {4, 99},
   {5, 99}, /* 20 */
   {5, 99},
   {6, 99},
   {6, 99},
   {7, 99},
   {7, 99}  /* 25 */
};

/* Charisma tables: Unsure what Smaug had in mind for this. */
const struct cha_app_type cha_app[26] = {
   {-60},   /* 0 */
   {-50},   /* 1 */
   {-50},
   {-40},
   {-30},
   {-20},   /* 5 */
   {-10},
   {-5},
   {-1},
   {0},
   {0},  /* 10 */
   {0},
   {0},
   {0},
   {1},
   {5},  /* 15 */
   {10},
   {20},
   {30},
   {40},
   {50}, /* 20 */
   {60},
   {70},
   {80},
   {90},
   {99}  /* 25 */
};

/* Have to fix this up - not exactly sure how it works (Scryn) */
const struct lck_app_type lck_app[26] = {
   {60}, /* 0 */
   {50}, /* 1 */
   {50},
   {40},
   {30},
   {20}, /* 5 */
   {10},
   {0},
   {0},
   {0},
   {0},  /* 10 */
   {0},
   {0},
   {0},
   {0},
   {-10},   /* 15 */
   {-15},
   {-20},
   {-30},
   {-40},
   {-50},   /* 20 */
   {-60},
   {-75},
   {-90},
   {-105},
   {-120}   /* 25 */
};

/* removed "pea" and added chop, spear, smash - Grimm */
/* Removed duplication in damage types - Samson 1-9-00 */
const char *attack_table[DAM_MAX_TYPE] = {
   "hit", "slash", "stab", "hack", "crush", "lash", "pierce",
   "thrust"
};

const char *attack_table_plural[DAM_MAX_TYPE] = {
   "hits", "slashes", "stabs", "hacks", "crushes", "lashes", "pierces",
   "thrusts"
};

const char *s_blade_messages[24] = {
   "miss", "barely scratch", "scratch", "nick", "cut", "hit", "tear",
   "rip", "gash", "lacerate", "hack", "maul", "rend", "decimate",
   "_mangle_", "_devastate_", "_cleave_", "_butcher_", "DISEMBOWEL",
   "DISFIGURE", "GUT", "EVISCERATE", "* SLAUGHTER *", "*** ANNIHILATE ***"
};

const char *p_blade_messages[24] = {
   "misses", "barely scratches", "scratches", "nicks", "cuts", "hits",
   "tears", "rips", "gashes", "lacerates", "hacks", "mauls", "rends",
   "decimates", "_mangles_", "_devastates_", "_cleaves_", "_butchers_",
   "DISEMBOWELS", "DISFIGURES", "GUTS", "EVISCERATES", "* SLAUGHTERS *",
   "*** ANNIHILATES ***"
};

const char *s_blunt_messages[24] = {
   "miss", "barely scuff", "scuff", "pelt", "bruise", "strike", "thrash",
   "batter", "flog", "pummel", "smash", "maul", "bludgeon", "decimate",
   "_shatter_", "_devastate_", "_maim_", "_cripple_", "MUTILATE", "DISFIGURE",
   "MASSACRE", "PULVERIZE", "* OBLITERATE *", "*** ANNIHILATE ***"
};

const char *p_blunt_messages[24] = {
   "misses", "barely scuffs", "scuffs", "pelts", "bruises", "strikes",
   "thrashes", "batters", "flogs", "pummels", "smashes", "mauls",
   "bludgeons", "decimates", "_shatters_", "_devastates_", "_maims_",
   "_cripples_", "MUTILATES", "DISFIGURES", "MASSACRES", "PULVERIZES",
   "* OBLITERATES *", "*** ANNIHILATES ***"
};

const char *s_generic_messages[24] = {
   "miss", "brush", "scratch", "graze", "nick", "jolt", "wound",
   "injure", "hit", "jar", "thrash", "maul", "decimate", "_traumatize_",
   "_devastate_", "_maim_", "_demolish_", "MUTILATE", "MASSACRE",
   "PULVERIZE", "DESTROY", "* OBLITERATE *", "*** ANNIHILATE ***",
   "**** SMITE ****"
};

const char *p_generic_messages[24] = {
   "misses", "brushes", "scratches", "grazes", "nicks", "jolts", "wounds",
   "injures", "hits", "jars", "thrashes", "mauls", "decimates", "_traumatizes_",
   "_devastates_", "_maims_", "_demolishes_", "MUTILATES", "MASSACRES",
   "PULVERIZES", "DESTROYS", "* OBLITERATES *", "*** ANNIHILATES ***",
   "**** SMITES ****"
};

const char **s_message_table[DAM_MAX_TYPE] = {
   s_generic_messages,  /* hit */
   s_blade_messages, /* slash */
   s_blade_messages, /* stab */
   s_blade_messages, /* hack */
   s_blunt_messages, /* crush */
   s_blunt_messages, /* lash */
   s_blade_messages, /* pierce */
   s_blade_messages, /* thrust */
};

const char **p_message_table[DAM_MAX_TYPE] = {
   p_generic_messages,  /* hit */
   p_blade_messages, /* slash */
   p_blade_messages, /* stab */
   p_blade_messages, /* hack */
   p_blunt_messages, /* crush */
   p_blunt_messages, /* lash */
   p_blade_messages, /* pierce */
   p_blade_messages, /* thrust */
};

/* Weather constants - FB */
const char *temp_settings[MAX_CLIMATE] = {
   "cold",
   "cool",
   "normal",
   "warm",
   "hot",
};

const char *precip_settings[MAX_CLIMATE] = {
   "arid",
   "dry",
   "normal",
   "damp",
   "wet",
};

const char *wind_settings[MAX_CLIMATE] = {
   "still",
   "calm",
   "normal",
   "breezy",
   "windy",
};

const char *preciptemp_msg[6][6] = {
   /*
    * precip = 0 
    */
   {
    "Frigid temperatures settle over the land",
    "It is bitterly cold",
    "The weather is crisp and dry",
    "A comfortable warmth sets in",
    "A dry heat warms the land",
    "Seething heat bakes the land"},
   /*
    * precip = 1 
    */
   {
    "A few flurries drift from the high clouds",
    "Frozen drops of rain fall from the sky",
    "An occasional raindrop falls to the ground",
    "Mild drops of rain seep from the clouds",
    "It is very warm, and the sky is overcast",
    "High humidity intensifies the seering heat"},
   /*
    * precip = 2 
    */
   {
    "A brief snow squall dusts the earth",
    "A light flurry dusts the ground",
    "Light snow drifts down from the heavens",
    "A light drizzle mars an otherwise perfect day",
    "A few drops of rain fall to the warm ground",
    "A light rain falls through the sweltering sky"},
   /*
    * precip = 3 
    */
   {
    "Snowfall covers the frigid earth",
    "Light snow falls to the ground",
    "A brief shower moistens the crisp air",
    "A pleasant rain falls from the heavens",
    "The warm air is heavy with rain",
    "A refreshing shower eases the oppresive heat"},
   /*
    * precip = 4 
    */
   {
    "Sleet falls in sheets through the frosty air",
    "Snow falls quickly, piling upon the cold earth",
    "Rain pelts the ground on this crisp day",
    "Rain drums the ground rythmically",
    "A warm rain drums the ground loudly",
    "Tropical rain showers pelt the seering ground"},
   /*
    * precip = 5 
    */
   {
    "A downpour of frozen rain covers the land in ice",
    "A blizzard blankets everything in pristine white",
    "Torrents of rain fall from a cool sky",
    "A drenching downpour obscures the temperate day",
    "Warm rain pours from the sky",
    "A torrent of rain soaks the heated earth"}
};

const char *windtemp_msg[6][6] = {
   /*
    * wind = 0 
    */
   {
    "The frigid air is completely still",
    "A cold temperature hangs over the area",
    "The crisp air is eerily calm",
    "The warm air is still",
    "No wind makes the day uncomfortably warm",
    "The stagnant heat is sweltering"},
   /*
    * wind = 1 
    */
   {
    "A light breeze makes the frigid air seem colder",
    "A stirring of the air intensifies the cold",
    "A touch of wind makes the day cool",
    "It is a temperate day, with a slight breeze",
    "It is very warm, the air stirs slightly",
    "A faint breeze stirs the feverish air"},
   /*
    * wind = 2 
    */
   {
    "A breeze gives the frigid air bite",
    "A breeze swirls the cold air",
    "A lively breeze cools the area",
    "It is a temperate day, with a pleasant breeze",
    "Very warm breezes buffet the area",
    "A breeze ciculates the sweltering air"},
   /*
    * wind = 3 
    */
   {
    "Stiff gusts add cold to the frigid air",
    "The cold air is agitated by gusts of wind",
    "Wind blows in from the north, cooling the area",
    "Gusty winds mix the temperate air",
    "Brief gusts of wind punctuate the warm day",
    "Wind attempts to cut the sweltering heat"},
   /*
    * wind = 4 
    */
   {
    "The frigid air whirls in gusts of wind",
    "A strong, cold wind blows in from the north",
    "Strong wind makes the cool air nip",
    "It is a pleasant day, with gusty winds",
    "Warm, gusty winds move through the area",
    "Blustering winds punctuate the seering heat"},
   /*
    * wind = 5 
    */
   {
    "A frigid gale sets bones shivering",
    "Howling gusts of wind cut the cold air",
    "An angry wind whips the air into a frenzy",
    "Fierce winds tear through the tepid air",
    "Gale-like winds whip up the warm air",
    "Monsoon winds tear the feverish air"}
};

const char *precip_msg[3] = {
   "there is not a cloud in the sky",
   "pristine white clouds are in the sky",
   "thick, grey clouds mask the sun"
};

const char *wind_msg[6] = {
   "there is not a breath of wind in the air",
   "a slight breeze stirs the air",
   "a breeze wafts through the area",
   "brief gusts of wind punctuate the air",
   "angry gusts of wind blow",
   "howling winds whip the air into a frenzy"
};
