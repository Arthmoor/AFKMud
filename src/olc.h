/****************************************************************************
 *                   ^     +----- |  / ^     ^ |     | +-\                  *
 *                  / \    |      | /  |\   /| |     | |  \                 *
 *                 /   \   +---   |<   | \ / | |     | |  |                 *
 *                /-----\  |      | \  |  v  | |     | |  /                 *
 *               /       \ |      |  \ |     | +-----+ +-/                  *
 ****************************************************************************
 * AFKMud Copyright 1997-2009 by Roger Libiez (Samson),                     *
 * Levi Beckerson (Whir), Michael Ward (Tarl), Erik Wolfe (Dwip),           *
 * Cameron Carroll (Cam), Cyberfox, Karangi, Rathian, Raine,                *
 * Xorith, and Adjani.                                                      *
 * All Rights Reserved.                                                     *
 * Registered with the United States Copyright Office: TX 5-877-286         *
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
 *                          Oasis OLC Module                                *
 ****************************************************************************/

/****************************************************************************
 * ResortMUD 4.0 Beta by Ntanel, Garinan, Badastaz, Josh, Digifuzz, Senir,  *
 * Kratas, Scion, Shogar and Tagith.  Special thanks to Thoric, Nivek,      *
 * Altrag, Arlorn, Justice, Samson, Dace, HyperEye and Yakkov.              *
 ****************************************************************************
 * Copyright (C) 1996 - 2001 Haslage Net Electronics: MudWorld              *
 * of Lorain, Ohio - ALL RIGHTS RESERVED                                    *
 * The text and pictures of this publication, or any part thereof, may not  *
 * be reproduced or transmitted in any form or by any means, electronic or  *
 * mechanical, includes photocopying, recording, storage in a information   *
 * retrieval system, or otherwise, without the prior written or e-mail      *
 * consent from the publisher.                                              *
 ****************************************************************************
 * GREETING must mention ResortMUD programmers and the help file named      *
 * CREDITS must remain completely intact as listed in the SMAUG license.    *
 ****************************************************************************/

/**************************************************************************\
 *     OasisOLC II for Smaug 1.40 written by Evan Cortens(Tagith)         *
 *   Based on OasisOLC for CircleMUD3.0bpl9 written by Harvey Gilpin      *
 **************************************************************************
 *                         Defines, structs, etc.. v1.0                   *
\**************************************************************************/

#ifndef __OLC_H__
#define __OLC_H__

enum weapon_types
{
   WEP_BAREHAND, WEP_SWORD, WEP_DAGGER, WEP_WHIP, WEP_TALON, WEP_MACE,
   WEP_ARCHERY, WEP_BLOWGUN, WEP_SLING, WEP_AXE, WEP_SPEAR, WEP_STAFF,
   WEP_MAX
};

enum projectile_types
{
   PROJ_BOLT, PROJ_ARROW, PROJ_DART, PROJ_STONE, PROJ_MAX
};

enum campgear_types
{
   GEAR_NONE, GEAR_BED, GEAR_MISC, GEAR_FIRE, GEAR_MAX
};

enum ore_types
{
   ORE_NONE, ORE_IRON, ORE_GOLD, ORE_SILVER, ORE_ADAM, ORE_MITH, ORE_BLACK,
   ORE_TITANIUM, ORE_STEEL, ORE_BRONZE, ORE_DWARVEN, ORE_ELVEN, ORE_MAX
};

enum gender_types
{
   SEX_NEUTRAL, SEX_MALE, SEX_FEMALE, SEX_HERMAPHRODYTE, SEX_MAX
};

enum positions
{
   POS_DEAD, POS_MORTAL, POS_INCAP, POS_STUNNED, POS_SLEEPING,
   POS_RESTING, POS_SITTING, POS_BERSERK, POS_AGGRESSIVE, POS_FIGHTING,
   POS_DEFENSIVE, POS_EVASIVE, POS_STANDING, POS_MOUNTED,
   POS_SHOVE, POS_DRAG, POS_MAX
};

/*
 * Directions.
 * Used in #ROOMS.
 */
enum dir_types
{
   DIR_NORTH, DIR_EAST, DIR_SOUTH, DIR_WEST, DIR_UP, DIR_DOWN,
   DIR_NORTHEAST, DIR_NORTHWEST, DIR_SOUTHEAST, DIR_SOUTHWEST, DIR_SOMEWHERE
};

const int MAX_DIR = DIR_SOUTHWEST;  /* max for normal walking */
const int DIR_PORTAL = DIR_SOMEWHERE;  /* portal direction    */

/*
 * ACT bits for mobs.
 * Used in #MOBILES.
 */
/* Don't forget to add the flag to build.c!!! */
/* current # of flags: 54 */
enum mob_flags
{
   ACT_IS_NPC, ACT_SENTINEL, ACT_SCAVENGER, ACT_INNKEEPER, ACT_BANKER,
   ACT_AGGRESSIVE, ACT_STAY_AREA, ACT_WIMPY, ACT_PET, ACT_AUTONOMOUS,
   ACT_PRACTICE, ACT_IMMORTAL, ACT_DEADLY, ACT_POLYSELF, ACT_META_AGGR,
   ACT_GUARDIAN, ACT_BOARDED, ACT_NOWANDER, ACT_MOUNTABLE, ACT_MOUNTED,
   ACT_SCHOLAR, ACT_SECRETIVE, ACT_HARDHAT, ACT_MOBINVIS, ACT_NOASSIST,
   ACT_ILLUSION, ACT_PACIFIST, ACT_NOATTACK, ACT_ANNOYING, ACT_AUCTION,
   ACT_PROTOTYPE,
   ACT_MAGE, ACT_WARRIOR, ACT_CLERIC, ACT_ROGUE, ACT_DRUID, ACT_MONK,
   ACT_PALADIN, ACT_RANGER, ACT_NECROMANCER, ACT_ANTIPALADIN, ACT_HUGE,
   ACT_GREET, ACT_TEACHER, ACT_ONMAP, ACT_SMITH, ACT_GUILDAUC, ACT_GUILDBANK,
   ACT_GUILDVENDOR, ACT_GUILDREPAIR, ACT_GUILDFORGE, ACT_IDMOB,
   ACT_GUILDIDMOB, MAX_ACT_FLAG
};

/*
 * Bits for 'affected_by'.
 * Used in #MOBILES.
 *
 * hold and flaming are yet uncoded
 */
/* Current # of bits: 60 */
/* Don't forget to add the flag to build.c, and the affect_bit_name function in handler.c */
enum affected_by_types
{
   AFF_NONE, AFF_BLIND, AFF_INVISIBLE, AFF_DETECT_EVIL, AFF_DETECT_INVIS,
   AFF_DETECT_MAGIC, AFF_DETECT_HIDDEN, AFF_HOLD, AFF_SANCTUARY,
   AFF_FAERIE_FIRE, AFF_INFRARED, AFF_CURSE, AFF_SPY, AFF_POISON,
   AFF_PROTECT, AFF_PARALYSIS, AFF_SNEAK, AFF_HIDE, AFF_SLEEP, AFF_CHARM,
   AFF_FLYING, AFF_ACIDMIST, AFF_FLOATING, AFF_TRUESIGHT, AFF_DETECTTRAPS,
   AFF_SCRYING, AFF_FIRESHIELD, AFF_SHOCKSHIELD, AFF_VENOMSHIELD, AFF_ICESHIELD,
   AFF_WIZARDEYE, AFF_BERSERK, AFF_AQUA_BREATH, AFF_RECURRINGSPELL,
   AFF_CONTAGIOUS, AFF_BLADEBARRIER, AFF_SILENCE, AFF_ANIMAL_INVIS,
   AFF_HEAT_STUFF, AFF_LIFE_PROT, AFF_DRAGON_RIDE, AFF_GROWTH, AFF_TREE_TRAVEL,
   AFF_TRAVELLING, AFF_TELEPATHY, AFF_ETHEREAL, AFF_PASS_DOOR, AFF_QUIV,
   AFF_FLAMING, AFF_HASTE, AFF_SLOW, AFF_ELVENSONG, AFF_BLADESONG,
   AFF_REVERIE, AFF_TENACITY, AFF_DEATHSONG, AFF_POSSESS, AFF_NOTRACK, AFF_ENLIGHTEN,
   AFF_TREETALK, AFF_SPAMGUARD, AFF_BASH, MAX_AFFECTED_BY
};

/*
 * Item types.
 * Used in #OBJECTS.
 */
/* Don't forget to add the flag to build.c!!! */
/* also don't forget to add new item types to the find_oftype function in afk.c */
/* Current # of types: 68 */
enum item_types
{
   ITEM_NONE, ITEM_LIGHT, ITEM_SCROLL, ITEM_WAND, ITEM_STAFF, ITEM_WEAPON,
   ITEM_SCABBARD, ITEM_unused2, ITEM_TREASURE, ITEM_ARMOR, ITEM_POTION,
   ITEM_CLOTHING, ITEM_FURNITURE, ITEM_TRASH, ITEM_unused4, ITEM_CONTAINER,
   ITEM_unused5, ITEM_DRINK_CON, ITEM_KEY, ITEM_FOOD, ITEM_MONEY, ITEM_PEN,
   ITEM_BOAT, ITEM_CORPSE_NPC, ITEM_CORPSE_PC, ITEM_FOUNTAIN, ITEM_PILL,
   ITEM_BLOOD, ITEM_BLOODSTAIN, ITEM_SCRAPS, ITEM_PIPE, ITEM_HERB_CON,
   ITEM_HERB, ITEM_INCENSE, ITEM_FIRE, ITEM_BOOK, ITEM_SWITCH, ITEM_LEVER,
   ITEM_PULLCHAIN, ITEM_BUTTON, ITEM_DIAL, ITEM_RUNE, ITEM_RUNEPOUCH,
   ITEM_MATCH, ITEM_TRAP, ITEM_MAP, ITEM_PORTAL, ITEM_PAPER,
   ITEM_TINDER, ITEM_LOCKPICK, ITEM_SPIKE, ITEM_DISEASE, ITEM_OIL, ITEM_FUEL,
   ITEM_PIECE, ITEM_TREE, ITEM_MISSILE_WEAPON, ITEM_PROJECTILE, ITEM_QUIVER,
   ITEM_SHOVEL, ITEM_SALVE, ITEM_COOK, ITEM_KEYRING, ITEM_ODOR, ITEM_CAMPGEAR,
   ITEM_DRINK_MIX, ITEM_INSTRUMENT, ITEM_ORE, MAX_ITEM_TYPE
};

/*
 * Extra flags.
 * Used in #OBJECTS. Rearranged for better compatibility with Shard areas - Samson
 */
/* Don't forget to add the flag to o_flags in build.c and handler.c!!! */
/* Current # of flags: 66 */
enum item_extra_flags
{
   ITEM_GLOW, ITEM_HUM, ITEM_METAL, ITEM_MINERAL, ITEM_ORGANIC, ITEM_INVIS, ITEM_MAGIC,
   ITEM_NODROP, ITEM_BLESS, ITEM_ANTI_GOOD, ITEM_ANTI_EVIL, ITEM_ANTI_NEUTRAL,
   ITEM_ANTI_CLERIC, ITEM_ANTI_MAGE, ITEM_ANTI_ROGUE, ITEM_ANTI_WARRIOR,
   ITEM_INVENTORY, ITEM_NOREMOVE, ITEM_TWOHAND, ITEM_EVIL, ITEM_DONATION,
   ITEM_CLANOBJECT, ITEM_CLANCORPSE, ITEM_ANTI_BARD, ITEM_HIDDEN,
   ITEM_ANTI_DRUID, ITEM_POISONED, ITEM_COVERING, ITEM_DEATHROT, ITEM_BURIED,
   ITEM_PROTOTYPE, ITEM_NOLOCATE, ITEM_GROUNDROT, ITEM_ANTI_MONK, ITEM_LOYAL,
   ITEM_BRITTLE, ITEM_RESISTANT, ITEM_IMMUNE, ITEM_ANTI_MEN, ITEM_ANTI_WOMEN,
   ITEM_ANTI_NEUTER, ITEM_ANTI_HERMA, ITEM_ANTI_SUN, ITEM_ANTI_RANGER, ITEM_ANTI_PALADIN,
   ITEM_ANTI_NECRO, ITEM_ANTI_APAL, ITEM_ONLY_CLERIC, ITEM_ONLY_MAGE,
   ITEM_ONLY_ROGUE, ITEM_ONLY_WARRIOR, ITEM_ONLY_BARD, ITEM_ONLY_DRUID,
   ITEM_ONLY_MONK, ITEM_ONLY_RANGER, ITEM_ONLY_PALADIN, ITEM_ONLY_NECRO,
   ITEM_ONLY_APAL, ITEM_AUCTION, ITEM_ONMAP, ITEM_PERSONAL, ITEM_LODGED,
   ITEM_SINDHAE, ITEM_MUSTMOUNT, ITEM_NOAUCTION, ITEM_THROWN, MAX_ITEM_FLAG
};

/*
 * Wear flags.
 * Used in #OBJECTS.
 */
enum item_wear_flags
{
   ITEM_TAKE, ITEM_WEAR_FINGER, ITEM_WEAR_NECK, ITEM_WEAR_BODY, ITEM_WEAR_HEAD,
   ITEM_WEAR_LEGS, ITEM_WEAR_FEET, ITEM_WEAR_HANDS, ITEM_WEAR_ARMS, ITEM_WEAR_SHIELD,
   ITEM_WEAR_ABOUT, ITEM_WEAR_WAIST, ITEM_WEAR_WRIST, ITEM_WIELD, ITEM_HOLD,
   ITEM_DUAL_WIELD, ITEM_WEAR_EARS, ITEM_WEAR_EYES, ITEM_MISSILE_WIELD,
   ITEM_WEAR_BACK, ITEM_WEAR_FACE, ITEM_WEAR_ANKLE, ITEM_LODGE_RIB, ITEM_LODGE_ARM,
   ITEM_LODGE_LEG, MAX_WEAR_FLAG
};

/*
 * Equpiment wear locations.
 * Used in #RESETS.
 */
enum wear_locations
{
   WEAR_NONE = -1, WEAR_LIGHT = 0, WEAR_FINGER_L, WEAR_FINGER_R, WEAR_NECK_1,
   WEAR_NECK_2, WEAR_BODY, WEAR_HEAD, WEAR_LEGS, WEAR_FEET, WEAR_HANDS,
   WEAR_ARMS, WEAR_SHIELD, WEAR_ABOUT, WEAR_WAIST, WEAR_WRIST_L, WEAR_WRIST_R,
   WEAR_WIELD, WEAR_HOLD, WEAR_DUAL_WIELD, WEAR_EARS, WEAR_EYES,
   WEAR_MISSILE_WIELD, WEAR_BACK, WEAR_FACE, WEAR_ANKLE_L, WEAR_ANKLE_R,
   WEAR_LODGE_RIB, WEAR_LODGE_ARM, WEAR_LODGE_LEG, MAX_WEAR
};

/*
 * Apply types (for affects).
 * Used in #OBJECTS.
 */
enum apply_types
{
   APPLY_NONE, APPLY_STR, APPLY_INT, APPLY_WIS, APPLY_DEX, APPLY_CON,   // 5
   APPLY_CHA, APPLY_LCK, APPLY_LEVEL, APPLY_AGE, APPLY_HEIGHT, APPLY_WEIGHT,  // 11
   APPLY_MANA, APPLY_HIT, APPLY_MOVE, APPLY_GOLD, APPLY_EXP, APPLY_AC,  // 17
   APPLY_HITROLL, APPLY_DAMROLL, APPLY_SAVING_POISON, APPLY_SAVING_ROD, // 21
   APPLY_SAVING_PARA, APPLY_SAVING_BREATH, APPLY_SAVING_SPELL, APPLY_SEX,  // 25
   APPLY_AFFECT, APPLY_RESISTANT, APPLY_IMMUNE, APPLY_SUSCEPTIBLE,   // 29
   APPLY_WEAPONSPELL, APPLY_CLASS, APPLY_BACKSTAB, APPLY_PICK, APPLY_TRACK,   // 34
   APPLY_STEAL, APPLY_SNEAK, APPLY_HIDE, APPLY_PALM, APPLY_DETRAP, APPLY_DODGE,  // 40
   APPLY_SF, APPLY_SCAN, APPLY_GOUGE, APPLY_SEARCH, APPLY_MOUNT, APPLY_DISARM,   // 46
   APPLY_KICK, APPLY_PARRY, APPLY_BASH, APPLY_STUN, APPLY_PUNCH, APPLY_CLIMB, // 52
   APPLY_GRIP, APPLY_SCRIBE, APPLY_BREW, APPLY_WEARSPELL, APPLY_REMOVESPELL,  // 57
   APPLY_unused, APPLY_MENTALSTATE, APPLY_STRIPSN, APPLY_REMOVE, APPLY_DIG,   // 62
   APPLY_FULL, APPLY_THIRST, APPLY_DRUNK, APPLY_HIT_REGEN,  // 66
   APPLY_MANA_REGEN, APPLY_MOVE_REGEN, APPLY_ANTIMAGIC,  // 69
   APPLY_ROOMFLAG, APPLY_SECTORTYPE, APPLY_ROOMLIGHT, APPLY_TELEVNUM,   // 73
   APPLY_TELEDELAY, APPLY_COOK, APPLY_RECURRINGSPELL, APPLY_RACE, APPLY_HITNDAM, // 78
   APPLY_SAVING_ALL, APPLY_EAT_SPELL, APPLY_RACE_SLAYER, APPLY_ALIGN_SLAYER,  // 82
   APPLY_CONTAGIOUS, APPLY_EXT_AFFECT, APPLY_ODOR, APPLY_PEEK, APPLY_ABSORB,  // 87
   APPLY_ATTACKS, APPLY_EXTRAGOLD, APPLY_ALLSTATS, MAX_APPLY_TYPE // 91 ( counting MAX_APPLY_TYPE )
};

const int REVERSE_APPLY = 1000;

/*
 * Room flags.           Holy cow!  Talked about stripped away..
 * Used in #ROOMS.       Those merc guys know how to strip code down.
 *			 Lets put it all back... ;)
 */
/* Roomflags converted to Extended BV - Samson 8-11-98 */
/* NOTE: If this list grows to 65, raise BFS_MARK in track.c - Samson */
/* Don't forget to add the flag to build.c!!! */
/* Current # of flags: 41 */
enum room_flags
{
   ROOM_DARK, ROOM_DEATH, ROOM_NO_MOB, ROOM_INDOORS, ROOM_SAFE, ROOM_NOCAMP,
   ROOM_NO_SUMMON, ROOM_NO_MAGIC, ROOM_TUNNEL, ROOM_PRIVATE, ROOM_SILENCE,
   ROOM_NOSUPPLICATE, ROOM_ARENA, ROOM_NOMISSILE, ROOM_NO_RECALL, ROOM_NO_PORTAL,
   ROOM_NO_ASTRAL, ROOM_NODROP, ROOM_CLANSTOREROOM, ROOM_TELEPORT, ROOM_TELESHOWDESC,
   ROOM_NOFLOOR, ROOM_SOLITARY, ROOM_PET_SHOP, ROOM_DONATION, ROOM_NODROPALL,
   ROOM_LOGSPEECH, ROOM_PROTOTYPE, ROOM_NOTELEPORT, ROOM_NOSCRY, ROOM_CAVE,
   ROOM_CAVERN, ROOM_NOBEACON, ROOM_AUCTION, ROOM_MAP, ROOM_FORGE, ROOM_GUILDINN,
   ROOM_ISOLATED, ROOM_WATCHTOWER, ROOM_NOQUIT, ROOM_TELENOFLY, ROOM_TRACK, ROOM_MAX
};

/*
 * Exit flags.			EX_RES# are reserved for use by the
 * Used in #ROOMS.		SMAUG development team ( No RES flags left, sorry. - Samson )
 */
/* Converted to Extended BV - Samson 7-23-00 */
/* Don't forget to add the flag to build.c!!! */
/* Current # of flags: 35 */
enum exit_flags
{
   EX_ISDOOR, EX_CLOSED, EX_LOCKED, EX_SECRET, EX_SWIM, EX_PICKPROOF, EX_FLY,
   EX_CLIMB, EX_DIG, EX_EATKEY, EX_NOPASSDOOR, EX_HIDDEN, EX_PASSAGE, EX_PORTAL,
   EX_OVERLAND, EX_ASLIT, EX_xCLIMB, EX_xENTER, EX_xLEAVE, EX_xAUTO, EX_NOFLEE,
   EX_xSEARCHABLE, EX_BASHED, EX_BASHPROOF, EX_NOMOB, EX_WINDOW, EX_xLOOK,
   EX_ISBOLT, EX_BOLTED, EX_FORTIFIED, EX_HEAVY, EX_MEDIUM, EX_LIGHT, EX_CRUMBLING,
   EX_DESTROYED, MAX_EXFLAG
};

/*
 * Sector types.
 * Used in #ROOMS and on Overland maps.
 */
/* Current number of types: 34 */
enum sector_types
{
   SECT_INDOORS, SECT_CITY, SECT_FIELD, SECT_FOREST, SECT_HILLS, SECT_MOUNTAIN,
   SECT_WATER_SWIM, SECT_WATER_NOSWIM, SECT_AIR, SECT_UNDERWATER, SECT_DESERT,
   SECT_RIVER, SECT_OCEANFLOOR, SECT_UNDERGROUND, SECT_JUNGLE, SECT_SWAMP,
   SECT_TUNDRA, SECT_ICE, SECT_OCEAN, SECT_LAVA, SECT_SHORE, SECT_TREE, SECT_STONE,
   SECT_QUICKSAND, SECT_WALL, SECT_GLACIER, SECT_EXIT, SECT_TRAIL, SECT_BLANDS,
   SECT_GRASSLAND, SECT_SCRUB, SECT_BARREN, SECT_BRIDGE, SECT_ROAD, SECT_LANDING, SECT_MAX
};

/*
 * Resistant Immune Susceptible flags
 * Now also supporting absorb flags - Samson 3-16-00 
 */
enum risa_flags
{
   RIS_NONE, RIS_FIRE, RIS_COLD, RIS_ELECTRICITY, RIS_ENERGY, RIS_BLUNT, RIS_PIERCE, RIS_SLASH,
   RIS_ACID, RIS_POISON, RIS_DRAIN, RIS_SLEEP, RIS_CHARM, RIS_HOLD, RIS_NONMAGIC,
   RIS_PLUS1, RIS_PLUS2, RIS_PLUS3, RIS_PLUS4, RIS_PLUS5, RIS_PLUS6, RIS_MAGIC,
   RIS_PARALYSIS, RIS_GOOD, RIS_EVIL, RIS_HACK, RIS_LASH, MAX_RIS_FLAG
};

/* 
 * Attack types
 */
enum attack_types
{
   ATCK_BITE, ATCK_CLAWS, ATCK_TAIL, ATCK_STING, ATCK_PUNCH, ATCK_KICK,
   ATCK_TRIP, ATCK_BASH, ATCK_STUN, ATCK_GOUGE, ATCK_BACKSTAB, ATCK_AGE,
   ATCK_DRAIN, ATCK_FIREBREATH, ATCK_FROSTBREATH, ATCK_ACIDBREATH,
   ATCK_LIGHTNBREATH, ATCK_GASBREATH, ATCK_POISON, ATCK_NASTYPOISON, ATCK_GAZE,
   ATCK_BLINDNESS, ATCK_CAUSESERIOUS, ATCK_EARTHQUAKE, ATCK_CAUSECRITICAL,
   ATCK_CURSE, ATCK_FLAMESTRIKE, ATCK_HARM, ATCK_FIREBALL, ATCK_COLORSPRAY,
   ATCK_WEAKEN, ATCK_SPIRALBLAST, MAX_ATTACK_TYPE
};

/* Removed redundant defenses that were covered in affectflags - Samson 5-12-99 */
/*
 * Defense types
 */
enum defense_types
{
   DFND_PARRY, DFND_DODGE, DFND_HEAL, DFND_CURELIGHT, DFND_CURESERIOUS,
   DFND_CURECRITICAL, DFND_DISPELMAGIC, DFND_DISPELEVIL, DFND_SANCTUARY,
   DFND_UNUSED1, DFND_UNUSED2, DFND_SHIELD, DFND_BLESS, DFND_STONESKIN,
   DFND_TELEPORT, DFND_UNUSED3, DFND_UNUSED4, DFND_UNUSED5, DFND_UNUSED6,
   DFND_DISARM, DFND_UNUSED7, DFND_GRIP, DFND_TRUESIGHT, MAX_DEFENSE_TYPE
};

enum prog_types
{
   ACT_PROG, SPEECH_PROG, RAND_PROG, FIGHT_PROG, DEATH_PROG, HITPRCNT_PROG,
   ENTRY_PROG, GREET_PROG, ALL_GREET_PROG, GIVE_PROG, BRIBE_PROG, HOUR_PROG,
   TIME_PROG, WEAR_PROG, REMOVE_PROG, SAC_PROG, LOOK_PROG, EXA_PROG, ZAP_PROG,
   GET_PROG, DROP_PROG, DAMAGE_PROG, REPAIR_PROG, RANDIW_PROG, SPEECHIW_PROG,
   PULL_PROG, PUSH_PROG, SLEEP_PROG, REST_PROG, LEAVE_PROG, SCRIPT_PROG,
   USE_PROG, SPEECH_AND_PROG, MONTH_PROG, KEYWORD_PROG, SELL_PROG, TELL_PROG,
   TELL_AND_PROG, CMD_PROG, MAX_PROG
};

/*
 * Body parts
 */
enum body_parts
{
   PART_HEAD, PART_ARMS, PART_LEGS, PART_HEART, PART_BRAINS, PART_GUTS,
   PART_HANDS, PART_FEET, PART_FINGERS, PART_EAR, PART_EYE, PART_LONG_TONGUE,
   PART_EYESTALKS, PART_TENTACLES, PART_FINS, PART_WINGS, PART_TAIL, PART_SCALES,
   /*
    * for combat 
    */
   PART_CLAWS, PART_FANGS, PART_HORNS, PART_TUSKS, PART_TAILATTACK, PART_SHARPSCALES, PART_BEAK,
   /*
    * Normal stuff again 
    */
   PART_HAUNCH, PART_HOOVES, PART_PAWS, PART_FORELEGS, PART_FEATHERS, MAX_BPART
};

enum traps
{
   TRAP_TYPE_POISON_GAS = 1, TRAP_TYPE_POISON_DART, TRAP_TYPE_POISON_NEEDLE,
   TRAP_TYPE_POISON_DAGGER, TRAP_TYPE_POISON_ARROW, TRAP_TYPE_BLINDNESS_GAS,
   TRAP_TYPE_SLEEPING_GAS, TRAP_TYPE_FLAME, TRAP_TYPE_EXPLOSION,
   TRAP_TYPE_ACID_SPRAY, TRAP_TYPE_ELECTRIC_SHOCK, TRAP_TYPE_BLADE,
   TRAP_TYPE_SEX_CHANGE, MAX_TRAPTYPE
};

/*
 * Values for containers (value[1]).
 * Used in #OBJECTS.
 */
const int CONT_CLOSEABLE = BV00;
const int CONT_PICKPROOF = BV01;
const int CONT_CLOSED = BV02;
const int CONT_LOCKED = BV03;
const int CONT_EATKEY = BV04;

const int MAX_CONT_FLAG = 5;  /* This needs to be equal to the number of container flags for the OLC menu editor */

/* Lever/dial/switch/button/pullchain flags */
const int TRIG_UP = BV00;
const int TRIG_UNLOCK = BV01;
const int TRIG_LOCK = BV02;
const int TRIG_D_NORTH = BV03;
const int TRIG_D_SOUTH = BV04;
const int TRIG_D_EAST = BV05;
const int TRIG_D_WEST = BV06;
const int TRIG_D_UP = BV07;
const int TRIG_D_DOWN = BV08;
const int TRIG_DOOR = BV09;
const int TRIG_CONTAINER = BV10;
const int TRIG_OPEN = BV11;
const int TRIG_CLOSE = BV12;
const int TRIG_PASSAGE = BV13;
const int TRIG_OLOAD = BV14;
const int TRIG_MLOAD = BV15;
const int TRIG_TELEPORT = BV16;
const int TRIG_TELEPORTALL = BV17;
const int TRIG_TELEPORTPLUS = BV18;
const int TRIG_DEATH = BV19;
const int TRIG_CAST = BV20;
const int TRIG_FAKEBLADE = BV21;
const int TRIG_RAND4 = BV22;
const int TRIG_RAND6 = BV23;
const int TRIG_TRAPDOOR = BV24;
const int TRIG_ANOTHEROOM = BV25;
const int TRIG_USEDIAL = BV26;
const int TRIG_ABSOLUTEVNUM = BV27;
const int TRIG_SHOWROOMDESC = BV28;
const int TRIG_AUTORETURN = BV29;

const int MAX_TRIGFLAG = 30;  /* Make equal to the number of trigger flags for OLC menu editor */

const int TELE_SHOWDESC = BV00;
const int TELE_TRANSALL = BV01;
const int TELE_TRANSALLPLUS = BV02;

/*
 * Sitting/Standing/Sleeping/Sitting on/in/at Objects - Xerves
 * Used for furniture (value[2]) in the #OBJECTS Section
 */
const int SIT_ON = BV00;
const int SIT_IN = BV01;
const int SIT_AT = BV02;

const int STAND_ON = BV03;
const int STAND_IN = BV04;
const int STAND_AT = BV05;

const int SLEEP_ON = BV06;
const int SLEEP_IN = BV07;
const int SLEEP_AT = BV08;

const int REST_ON = BV09;
const int REST_IN = BV10;
const int REST_AT = BV11;

const int MAX_FURNFLAG = 12;

/*
 * Pipe flags
 */
const int PIPE_TAMPED = BV01;
const int PIPE_LIT = BV02;
const int PIPE_HOT = BV03;
const int PIPE_DIRTY = BV04;
const int PIPE_FILTHY = BV05;
const int PIPE_GOINGOUT = BV06;
const int PIPE_BURNT = BV07;
const int PIPE_FULLOFASH = BV08;

const int TRAP_ROOM = BV00;
const int TRAP_OBJ = BV01;
const int TRAP_ENTER_ROOM = BV02;
const int TRAP_LEAVE_ROOM = BV03;
const int TRAP_OPEN = BV04;
const int TRAP_CLOSE = BV05;
const int TRAP_GET = BV06;
const int TRAP_PUT = BV07;
const int TRAP_PICK = BV08;
const int TRAP_UNLOCK = BV09;
const int TRAP_N = BV10;
const int TRAP_S = BV11;
const int TRAP_E = BV12;
const int TRAP_W = BV13;
const int TRAP_U = BV14;
const int TRAP_D = BV15;
const int TRAP_EXAMINE = BV16;
const int TRAP_NE = BV17;
const int TRAP_NW = BV18;
const int TRAP_SE = BV19;
const int TRAP_SW = BV20;
const int TRAPFLAG_MAX = 20;

extern const char *log_flag[];   /* Used in cedit display and command saving */
extern const char *mag_flags[];  /* Used during bootup */
extern const char *npc_sex[SEX_MAX];
extern const char *styles[];  // Combat styles
extern const char *npc_position[POS_MAX];
extern const char *weapon_skills[WEP_MAX];   /* Used in spell_identify */
extern const char *projectiles[PROJ_MAX]; /* For archery weapons */
extern const char *container_flags[];  /* Tagith */
extern const char *furniture_flags[];  /* Zarius */
extern const char *campgear[GEAR_MAX]; /* For OLC menus */
extern const char *ores[ORE_MAX];   /* For OLC menus */

/*. OLC structs .*/

class olc_data
{
 private:
   olc_data( const olc_data & o );
     olc_data & operator=( const olc_data & );

 public:
     olc_data(  );

   mob_index *mob;
   room_index *room;
   obj_data *obj;
   area_data *area;
   struct shop_data *shop;
   extra_descr_data *extradesc;
   struct affect_data *paf;
   struct exit_data *xit;
   int mode;
   int zone_num;
   int number;
   int value;
   bool changed;
};

/*. Descriptor access macros .*/
#define OLC_MODE(d) 	((d)->olc->mode)  /* Parse input mode  */
#define OLC_NUM(d) 	((d)->olc->number)   /* Room/Obj VNUM  */
#define OLC_VNUM(d)	OLC_NUM(d)
#define OLC_VAL(d) 	((d)->olc->value) /* Scratch variable  */
#define OLC_OBJ(d)	(obj)
#define OLC_DESC(d) 	((d)->olc->desc)  /* Extra description */
#define OLC_AFF(d)	((d)->olc->paf)   /* Affect data       */
#define OLC_CHANGE(d)	((d)->olc->changed)  /* Changed flag      */
#define OLC_EXIT(d)     ((d)->olc->xit)   /* An Exit     */

#ifdef OLD_CIRCLE_STYLE
# define OLC_ROOM(d)    ((d)->olc->room)  /* Room structure       */
# define OLC_OBJ(d)     ((d)->olc->obj)   /* Object structure     */
# define OLC_MOB(d)     ((d)->olc->mob)   /* Mob structure        */
# define OLC_SHOP(d)    ((d)->olc->shop)  /* Shop structure       */
# define OLC_EXIT(d)	(OLC_ROOM(d)->dir_option[OLC_VAL(d)])
#endif

/*. Add/Remove save list types	.*/
const int OLC_SAVE_ROOM = 0;
const int OLC_SAVE_OBJ = 1;
const int OLC_SAVE_ZONE = 2;
const int OLC_SAVE_MOB = 3;
const int OLC_SAVE_SHOP = 4;

/* Submodes of OEDIT connectedness */
const int OEDIT_MAIN_MENU = 1;
const int OEDIT_EDIT_NAMELIST = 2;
const int OEDIT_SHORTDESC = 3;
const int OEDIT_LONGDESC = 4;
const int OEDIT_ACTDESC = 5;
const int OEDIT_TYPE = 6;
const int OEDIT_EXTRAS = 7;
const int OEDIT_WEAR = 8;
const int OEDIT_WEIGHT = 9;
const int OEDIT_COST = 10;
const int OEDIT_COSTPERDAY = 11;
const int OEDIT_TIMER = 12;
const int OEDIT_VALUE_0 = 13;
const int OEDIT_VALUE_1 = 14;
const int OEDIT_VALUE_2 = 15;
const int OEDIT_VALUE_3 = 16;
const int OEDIT_VALUE_4 = 17;
const int OEDIT_VALUE_5 = 18;
const int OEDIT_EXTRADESC_KEY = 19;
const int OEDIT_TRAPFLAGS = 20;

const int OEDIT_EXTRADESC_DESCRIPTION = 22;
const int OEDIT_EXTRADESC_MENU = 23;
const int OEDIT_LEVEL = 24;
const int OEDIT_LAYERS = 25;
const int OEDIT_AFFECT_MENU = 26;
const int OEDIT_AFFECT_LOCATION = 27;
const int OEDIT_AFFECT_MODIFIER = 28;
const int OEDIT_AFFECT_REMOVE = 29;
const int OEDIT_AFFECT_RIS = 30;
const int OEDIT_EXTRADESC_CHOICE = 31;
const int OEDIT_EXTRADESC_DELETE = 32;
const int OEDIT_VALUE_6 = 33;
const int OEDIT_VALUE_7 = 34;
const int OEDIT_VALUE_8 = 35;
const int OEDIT_VALUE_9 = 36;
const int OEDIT_VALUE_10 = 37;

/* Submodes of REDIT connectedness */
const int REDIT_MAIN_MENU = 1;
const int REDIT_NAME = 2;
const int REDIT_DESC = 3;
const int REDIT_FLAGS = 4;
const int REDIT_SECTOR = 5;
const int REDIT_EXIT_MENU = 6;

const int REDIT_EXIT_DIR = 8;
const int REDIT_EXIT_VNUM = 9;
const int REDIT_EXIT_DESC = 10;
const int REDIT_EXIT_KEYWORD = 11;
const int REDIT_EXIT_KEY = 12;
const int REDIT_EXIT_FLAGS = 13;
const int REDIT_EXTRADESC_MENU = 14;
const int REDIT_EXTRADESC_KEY = 15;
const int REDIT_EXTRADESC_DESCRIPTION = 16;
const int REDIT_TUNNEL = 17;
const int REDIT_TELEDELAY = 18;
const int REDIT_TELEVNUM = 19;
const int REDIT_EXIT_EDIT = 20;
const int REDIT_EXIT_ADD = 21;
const int REDIT_EXIT_DELETE = 22;
const int REDIT_EXIT_ADD_VNUM = 23;
const int REDIT_EXTRADESC_DELETE = 24;
const int REDIT_EXTRADESC_CHOICE = 25;
const int REDIT_NDESC = 26;
const int REDIT_LIGHT = 27;

/*. Submodes of MEDIT connectedness 	.*/
const int MEDIT_NPC_MAIN_MENU = 0;
const int MEDIT_NAME = 1;
const int MEDIT_S_DESC = 2;
const int MEDIT_L_DESC = 3;
const int MEDIT_D_DESC = 4;
const int MEDIT_NPC_FLAGS = 5;
const int MEDIT_AFF_FLAGS = 6;

const int MEDIT_SEX = 8;
const int MEDIT_HITROLL = 9;
const int MEDIT_DAMROLL = 10;
const int MEDIT_DAMNUMDIE = 11;
const int MEDIT_DAMSIZEDIE = 12;
const int MEDIT_DAMPLUS = 13;
const int MEDIT_HITPLUS = 14;
const int MEDIT_AC = 15;
const int MEDIT_GOLD = 16;
const int MEDIT_POS = 17;
const int MEDIT_DEFPOS = 18;
const int MEDIT_ATTACK = 19;
const int MEDIT_DEFENSE = 20;
const int MEDIT_LEVEL = 21;
const int MEDIT_ALIGNMENT = 22;
const int MEDIT_THACO = 23;
const int MEDIT_EXP = 24;
const int MEDIT_SPEC = 25;
const int MEDIT_RESISTANT = 26;
const int MEDIT_IMMUNE = 27;
const int MEDIT_SUSCEPTIBLE = 28;
const int MEDIT_ABSORB = 29;
const int MEDIT_MENTALSTATE = 30;

const int MEDIT_PARTS = 32;
const int MEDIT_HITPOINT = 33;
const int MEDIT_MANA = 34;
const int MEDIT_MOVE = 35;
const int MEDIT_CLASS = 36;
const int MEDIT_RACE = 37;
#endif
