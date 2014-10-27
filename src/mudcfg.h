/****************************************************************************
 *                   ^     +----- |  / ^     ^ |     | +-\                  *
 *                  / \    |      | /  |\   /| |     | |  \                 *
 *                 /   \   +---   |<   | \ / | |     | |  |                 *
 *                /-----\  |      | \  |  v  | |     | |  /                 *
 *               /       \ |      |  \ |     | +-----+ +-/                  *
 ****************************************************************************
 * AFKMud Copyright 1997-2008 by Roger Libiez (Samson),                     *
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
 *                         MUD Specific Definitions                         *
 ****************************************************************************/

#ifndef __MUDCFG_H__
#define __MUDCFG_H__

/* These definitions can be safely changed without fear of being overwritten by future patches.
 * This does not guarantee however that compatibility will be maintained if you change
 * or remove too much of this stuff. Best to keep the names and only change the values they
 * represent if you want to spare yourself the hassle. Samson 3-14-2004
 */

/*
 * Game parameters.
 * Increase the max'es if you add more of something.
 * Adjust the pulse numbers to suit yourself.
 */
extern const char* SPELL_SILENT_MARKER;

const int MAX_LAYERS        = 8;   /* maximum clothing layers */
const int MAX_NEST          = 100; /* maximum container nesting */
const int MAX_REXITS        = 20;  /* Maximum exits allowed in 1 room */
const int MAX_CLASS         = 20;  /* Be careful with these two figures - alot depends on them being only available to PCs */
const int MAX_RACE          = 20;
const int MAX_NPC_CLASS     = 26;
const int MAX_NPC_RACE      = 162; /* Good God almighty! If a race your looking for isn't available, you have problems!!! */
const int MAX_BEACONS       = 10;
const int MAX_SAYHISTORY    = 25;
const int MAX_TELLHISTORY   = 25;
const int MAX_FIGHT   = 8;
const int MAX_SKILL   = 500;   /* Raised during 1.4a patch */
const int MAX_HERB    = 100;
const int MAX_DISEASE = 20;

extern int MAX_PC_RACE;
extern int MAX_PC_CLASS;

const int MAX_LEVEL         = 115; /* Raised from 65 by Teklord */
const int MAX_PERSONAL      = 5;   /* Maximum personal skills */
const int MAX_WHERE_NAME    = 29;  /* See act_info.c for the text messages */

const int LEVEL_SUPREME     = MAX_LEVEL;
const int LEVEL_ADMIN       = (MAX_LEVEL - 1);
const int LEVEL_KL          = (MAX_LEVEL - 2);
const int LEVEL_IMPLEMENTOR = (MAX_LEVEL - 3);
const int LEVEL_SUB_IMPLEM  = (MAX_LEVEL - 4);
const int LEVEL_ASCENDANT   = (MAX_LEVEL - 5);
const int LEVEL_GREATER     = (MAX_LEVEL - 6);
const int LEVEL_GOD         = (MAX_LEVEL - 7);
const int LEVEL_LESSER      = (MAX_LEVEL - 8);
const int LEVEL_TRUEIMM     = (MAX_LEVEL - 9);
const int LEVEL_DEMI        = (MAX_LEVEL - 10);
const int LEVEL_SAVIOR      = (MAX_LEVEL - 11);
const int LEVEL_CREATOR     = (MAX_LEVEL - 12);
const int LEVEL_ACOLYTE     = (MAX_LEVEL - 13);
const int LEVEL_IMMORTAL    = (MAX_LEVEL - 14);
const int LEVEL_AVATAR      = (MAX_LEVEL - 15);

const int LEVEL_LOG         = LEVEL_LESSER;

/* Realm Globals */
/* --- Xorith -- */
const int REALM_IMP          = 5;
const int REALM_HEAD_BUILDER = 4;
const int REALM_HEAD_CODER   = 3;
const int REALM_CODE         = 2;
const int REALM_BUILD        = 1;

#ifdef MULTIPORT
/* Port definitions for various commands - Samson 8-22-98 */
/* Now only works if Multiport support is enabled at compile time - Samson 7-12-02 */
const int CODEPORT  = 9700;
const int BUILDPORT = 9600;
const int MAINPORT  = 9500;
#endif

/*
 * Well known mob virtual numbers.
 * Defined in #MOBILES.
 */
/* Added animate dead mobs - Whir - 8/29/98 */
const int MOB_VNUM_SUPERMOB                = 11402;
const int MOB_VNUM_ANIMATED_CORPSE         = 11403;
const int MOB_VNUM_ANIMATED_SKELETON       = 11404;
const int MOB_VNUM_ANIMATED_ZOMBIE         = 11405;
const int MOB_VNUM_ANIMATED_GHOUL          = 11406;
const int MOB_VNUM_ANIMATED_CRYPT_THING    = 11407;
const int MOB_VNUM_ANIMATED_MUMMY          = 11408;
const int MOB_VNUM_ANIMATED_GHOST          = 11409;
const int MOB_VNUM_ANIMATED_DEATH_KNIGHT   = 11410;
const int MOB_VNUM_ANIMATED_DRACOLICH      = 11411;
const int MOB_VNUM_CITYGUARD               = 11074; /* Replaced original cityguard - Samson 3-26-98 */
const int MOB_VNUM_GATE                    = 11029; /* Gate spell servitor daemon - Samson 3-26-98 */
const int MOB_VNUM_CREEPINGDOOM            = 11412; /* Creeping Doom mob - Samson 9-27-98 */
const int MOB_DOPPLEGANGER                 = 11413; /* Doppleganger base mob - Samson 10-11-99 */
const int MOB_VNUM_WARMOUNT                = 11414; /* Paladin warmount - Samson 10-13-98 */
const int MOB_VNUM_WARMOUNTTWO             = 11007; /* Antipaladin warmount - Samson 4-2-00 */
const int MOB_VNUM_WARMOUNTTHREE           = 11008; /* Paladin flying warmount - Samson 4-2-00 */
const int MOB_VNUM_WARMOUNTFOUR            = 11009; /* Antipaladin flying warmount - Samson 4-2-00 */
const int MOB_VNUM_SKYSHIP                 = 11072;
const int MOB_VNUM_WOODCALL1               = 11001;
const int MOB_VNUM_WOODCALL2               = 11002;
const int MOB_VNUM_WOODCALL3               = 11003;
const int MOB_VNUM_WOODCALL4               = 11004;
const int MOB_VNUM_WOODCALL5               = 11005;
const int MOB_VNUM_WOODCALL6               = 11006;

/* Deity avatars */

/*
 * Well known object virtual numbers.
 * Defined in #OBJECTS.
 */
const int OBJ_VNUM_DUMMYOBJ          = 11000; /* This one is used by resets to make sure they work right - Samson 2/25/05 */
const int OBJ_VNUM_FIREPIT           = 11001; /* Campfires turn into these when dead - Samson 8-10-98 */
const int OBJ_VNUM_CAMPFIRE          = 11002; /* Campfire that loads when a player camps - Samson 8-10-98 */
const int OBJ_VNUM_OVFIRE            = 11005; /* Overland environmental fire */
const int OBJ_VNUM_FIRESEED          = 11007; /* Fireseed object for spell_fireseed - Samson 10-13-98 */
const int OBJ_VNUM_FORAGE            = 11027; /* This is the start of a range of 10 vnums - Samson 5-1-04 */
const int OBJ_VNUM_MAPS              = 11046; /* Object used for mapout - Samson 1/28/06 */
const int OBJ_VNUM_MINING            = 11300; /* This is the start of a range of 5 vnums - Samson 5-1-04 */
const int OBJ_VNUM_MONEY_ONE         = 11401;
const int OBJ_VNUM_MONEY_SOME        = 11402;
const int OBJ_VNUM_CORPSE_NPC        = 11403;
const int OBJ_VNUM_CORPSE_PC         = 11404;
const int OBJ_VNUM_TREASURE          = 11044;
const int OBJ_VNUM_RUNE              = 11045;
const int OBJ_VNUM_SEVERED_HEAD      = 11405;
const int OBJ_VNUM_TORN_HEART        = 11406;
const int OBJ_VNUM_SLICED_ARM        = 11407;
const int OBJ_VNUM_SLICED_LEG        = 11408;
const int OBJ_VNUM_SPILLED_GUTS      = 11409;
const int OBJ_VNUM_BRAINS            = 11435;
const int OBJ_VNUM_HANDS             = 11436;
const int OBJ_VNUM_FOOT              = 11437;
const int OBJ_VNUM_FINGERS           = 11438;
const int OBJ_VNUM_EAR               = 11439;
const int OBJ_VNUM_EYE               = 11440;
const int OBJ_VNUM_TONGUE            = 11441;
const int OBJ_VNUM_EYESTALK          = 11442;
const int OBJ_VNUM_TENTACLE          = 11443;
const int OBJ_VNUM_FINS              = 11444;
const int OBJ_VNUM_WINGS             = 11445;
const int OBJ_VNUM_TAIL              = 11446;
const int OBJ_VNUM_SCALES            = 11447;
const int OBJ_VNUM_TUSKS             = 11448;
const int OBJ_VNUM_HORNS             = 11449;
const int OBJ_VNUM_CLAWS             = 11450;
const int OBJ_VNUM_FEATHERS          = 11462;
const int OBJ_VNUM_FORELEG           = 11463;
const int OBJ_VNUM_PAWS              = 11464;
const int OBJ_VNUM_HOOVES            = 11465;
const int OBJ_VNUM_BEAK              = 11466;
const int OBJ_VNUM_SHARPSCALE        = 11467;
const int OBJ_VNUM_HAUNCHES          = 11468;
const int OBJ_VNUM_FANGS             = 11469;
const int OBJ_VNUM_BLOOD             = 11410;
const int OBJ_VNUM_BLOODSTAIN        = 11411;
const int OBJ_VNUM_SCRAPS            = 11412;
const int OBJ_VNUM_MUSHROOM          = 11413;
const int OBJ_VNUM_LIGHT_BALL        = 11414;
const int OBJ_VNUM_SPRING            = 11415;
const int OBJ_VNUM_SLICE             = 11417;
const int OBJ_VNUM_SHOPPING_BAG      = 11418;
const int OBJ_VNUM_FIRE              = 11423;
const int OBJ_VNUM_TRAP              = 11424;
const int OBJ_VNUM_PORTAL            = 11425;
const int OBJ_VNUM_BLACK_POWDER      = 11426;
const int OBJ_VNUM_SCROLL_SCRIBING   = 11427;
const int OBJ_VNUM_FLASK_BREWING     = 11428;
const int OBJ_VNUM_NOTE              = 11429;
const int OBJ_VNUM_WAND_CHARGING     = 11432;
const int OBJ_VNUM_TAN_JACKET        = 11368;
const int OBJ_VNUM_TAN_BOOTS         = 11369;
const int OBJ_VNUM_TAN_GLOVES        = 11370;
const int OBJ_VNUM_TAN_LEGGINGS      = 11371;
const int OBJ_VNUM_TAN_SLEEVES       = 11372;
const int OBJ_VNUM_TAN_HELMET        = 11373;
const int OBJ_VNUM_TAN_BAG           = 11374;
const int OBJ_VNUM_TAN_CLOAK         = 11375;
const int OBJ_VNUM_TAN_BELT          = 11376;
const int OBJ_VNUM_TAN_COLLAR        = 11377;
const int OBJ_VNUM_TAN_WATERSKIN     = 11378;
const int OBJ_VNUM_TAN_QUIVER        = 11379;
const int OBJ_VNUM_TAN_WHIP          = 11380;
const int OBJ_VNUM_TAN_SHIELD        = 11381;

/* Academy eq */
const int OBJ_VNUM_SCHOOL_BANNER     = 11478;
const int OBJ_VNUM_NEWBIE_GUIDE      = 11479;

/*
 * Well known room virtual numbers.
 * Defined in #ROOMS.
 */
const int ROOM_VNUM_LIMBO            = 11401;
const int ROOM_VNUM_POLY             = 11402;
const int ROOM_VNUM_CHAT             = 11406; /* Parlour of the Immortals */
const int ROOM_NOAUTH_START          = 102;   /* Pregame Entry, auth system off */
const int ROOM_AUTH_START	       = 100;   /* Pregame Entry, auth system on */
const int ROOM_VNUM_TEMPLE           = 11407; /* Primary continent recall */
const int ROOM_VNUM_ALTAR            = 11407; /* Primary continent death */
const int ROOM_VNUM_ASTRAL_DEATH     = 11407; /* Astral Plane, death sends you to Primary continent */
const int ROOM_VNUM_HELL             = 11405; /* Vnum for Hell - Samson */
const int ROOM_VNUM_DONATION         = 11410; /* First donation room - Samson 2-6-98 */
const int ROOM_VNUM_REDEEM           = 11411; /* Sindhae prize redemption start room - Samson 6-2-00 */
const int ROOM_VNUM_ENDREDEEM        = 11412; /* Sindhae prize redemption ending room - Samson 6-2-00 */
const int ROOM_VNUM_RAREUPDATE       = 11402; /* Room where players get scanned for rare items - Samson 1-24-00 */
const int ROOM_VNUM_TEMPSHOP         = 11402; /* Shopkeeper mobs get loaded here at bootup - Samson 7-4-04 */

/* New continent and plane support - Samson 3-28-98
 * Name of continent or plane is followed by the recall and death zone.
 * Area continent flags for continent and plane system, revised format - Samson 8-8-98
 */
enum acon_types
{
   ACON_ONE, ACON_ASTRAL, ACON_IMMORTAL, ACON_MAX
};

/* the races */
/* If you add a new race to this table, make sure you update update_aris in character.c as well */
enum race_types
{
   RACE_HUMAN, RACE_HIGH_ELF, RACE_DWARF, RACE_HALFLING, RACE_PIXIE,
   RACE_HALF_OGRE, RACE_HALF_ORC, RACE_HALF_TROLL, RACE_HALF_ELF, RACE_GITH,
   RACE_MINOTAUR, RACE_DUERGAR, RACE_CENTAUR, RACE_IGUANADON,
   RACE_GNOME, RACE_DROW, RACE_WILD_ELF, RACE_INSECTOID, RACE_SAUGHIN, RACE_19
};

/* NPC Races */
const int RACE_HALFBREED = 20;
const int RACE_REPTILE  =21;
const int RACE_SPECIAL  =22;
const int RACE_LYCANTH  =23;
const int RACE_DRAGON   =24;
const int RACE_UNDEAD   =25;
const int RACE_ORC      =26;
const int RACE_INSECT   =27;
const int RACE_ARACHNID =28;
const int RACE_DINOSAUR =29;
const int RACE_FISH     =30;
const int RACE_BIRD     =31;
const int RACE_GIANT    =32;   /* generic giant more specials down ---V */
const int RACE_PREDATOR =33;
const int RACE_PARASITE =34;
const int RACE_SLIME    =35;
const int RACE_DEMON    =36;
const int RACE_SNAKE    =37;
const int RACE_HERBIV   =38;
const int RACE_TREE     =39;
const int RACE_VEGGIE   =40;
const int RACE_ELEMENT  =41;
const int RACE_PLANAR   =42;
const int RACE_DEVIL    =43;
const int RACE_GHOST    =44;
const int RACE_GOBLIN   =45;
const int RACE_TROLL    =46;
const int RACE_VEGMAN   =47;
const int RACE_MFLAYER  =48;
const int RACE_PRIMATE  =49;
const int RACE_ENFAN    =50;
const int RACE_GOLEM    =51;
const int RACE_SKEXIE   =52;
const int RACE_TROGMAN  =53;
const int RACE_PATRYN   =54;
const int RACE_LABRAT   =55;
const int RACE_SARTAN   =56;
const int RACE_TYTAN    =57;
const int RACE_SMURF    =58;
const int RACE_ROO      =59;
const int RACE_HORSE    =60;
const int RACE_DRAAGDIM =61;
const int RACE_ASTRAL   =62;
const int RACE_GOD      =63;

const int RACE_GIANT_HILL  = 64;
const int RACE_GIANT_FROST = 65;
const int RACE_GIANT_FIRE  = 66;
const int RACE_GIANT_CLOUD = 67;
const int RACE_GIANT_STORM = 68;
const int RACE_GIANT_STONE = 69;

const int RACE_DRAGON_RED    =70;
const int RACE_DRAGON_BLACK  =71;
const int RACE_DRAGON_GREEN  =72;
const int RACE_DRAGON_WHITE  =73;
const int RACE_DRAGON_BLUE   =74;
const int RACE_DRAGON_SILVER =75;
const int RACE_DRAGON_GOLD   =76;
const int RACE_DRAGON_BRONZE =77;
const int RACE_DRAGON_COPPER =78;
const int RACE_DRAGON_BRASS  =79;

const int RACE_VAMPIRE		=80;
const int RACE_UNDEAD_VAMPIRE	=80;
const int RACE_LICH           = 81;
const int RACE_UNDEAD_LICH	=81;
const int RACE_WIGHT		=82;
const int RACE_UNDEAD_WIGHT	=82;
const int RACE_GHAST		=83;
const int RACE_UNDEAD_GHAST	=83;
const int RACE_SPECTRE		=84;
const int RACE_UNDEAD_SPECTRE	=84;
const int RACE_ZOMBIE		=85;
const int RACE_UNDEAD_ZOMBIE	=85;
const int RACE_SKELETON		=86;
const int RACE_UNDEAD_SKELETON =86;
const int RACE_GHOUL		=87;
const int RACE_UNDEAD_GHOUL	=87;

const int RACE_HALF_GIANT   =88;
const int RACE_DEEP_GNOME =89;
const int RACE_GNOLL	=90;

const int RACE_GOLD_ELF	=91;
const int RACE_GOLD_ELVEN =91;
const int RACE_SEA_ELF	=92;
const int RACE_SEA_ELVEN =92;

/* 10-20-96 Admiral */
const int RACE_TIEFLING  = 93;
const int RACE_AASIMAR   = 94;
const int RACE_SOLAR     = 95;
const int RACE_PLANITAR  = 96;
const int RACE_UNDEAD_SHADOW = 97;
const int RACE_GIANT_SKELETON= 98;
const int RACE_NILBOG        = 99;
const int RACE_HOUSERS       = 100;
const int RACE_BAKU          = 101;
const int RACE_BEASTLORD     = 102;
const int RACE_DEVAS         = 103;
const int RACE_POLARIS       = 104;
const int RACE_DEMODAND      = 105;
const int RACE_TARASQUE      = 106;
const int RACE_DIETY         = 107;
const int RACE_DAEMON        = 108;
const int RACE_VAGABOND      = 109;

/* Imported races from old Alsherok code - Samson */
const int RACE_GARGOYLE      = 110;
const int RACE_BEAR          = 111;
const int RACE_BAT           = 112;
const int RACE_CAT           = 113;
const int RACE_DOG           = 114;
const int RACE_ANT           = 115;
const int RACE_APE           = 116;
const int RACE_BABOON        = 117;
const int RACE_BEE           = 118;
const int RACE_BEETLE        = 119;
const int RACE_BOAR          = 120;
const int RACE_BUGBEAR       = 121;
const int RACE_FERRET        = 122;
const int RACE_FLY           = 123;
const int RACE_GELATIN       = 124;
const int RACE_GORGON        = 125;
const int RACE_HARPY         = 126;
const int RACE_HOBGOBLIN     = 127;
const int RACE_KOBOLD        = 128;
const int RACE_LOCUST        = 129;
const int RACE_MOLD          = 130;
const int RACE_MULE          = 131;
const int RACE_NEANDERTHAL   = 132;
const int RACE_OOZE          = 133;
const int RACE_RAT           = 134;
const int RACE_RUSTMONSTER   = 135;
const int RACE_SHAPESHIFTER  = 136;
const int RACE_SHREW         = 137;
const int RACE_SHRIEKER      = 138;
const int RACE_STIRGE        = 139;
const int RACE_THOUL         = 140;
const int RACE_WOLF          = 141;
const int RACE_WORM          = 142;
const int RACE_BOVINE        = 143;
const int RACE_CANINE        = 144;
const int RACE_FELINE        = 145;
const int RACE_PORCINE       = 146;
const int RACE_MAMMAL        = 147;
const int RACE_RODENT        = 148;
const int RACE_AMPHIBIAN     = 149;
const int RACE_CRUSTACEAN    = 150;
const int RACE_SPIRIT        = 151;
const int RACE_MAGICAL       = 152;
const int RACE_ANIMAL        = 153;
const int RACE_HUMANOID      = 154;
const int RACE_MONSTER       = 155;
const int RACE_UNUSED1       = 156;
const int RACE_UNUSED2       = 157;
const int RACE_UNUSED3       = 158;
const int RACE_UNUSED4       = 159;
const int RACE_UNUSED5       = 160;
const int RACE_UNUSED6       = 161;

const int CLASS_NONE	   = -1; /* For skill/spells according to guild */

/* If you add a new class to this, make sure you update update_aris in character.c as well */
enum class_types
{
   CLASS_MAGE, CLASS_CLERIC, CLASS_ROGUE, CLASS_WARRIOR, CLASS_NECROMANCER,
   CLASS_DRUID, CLASS_RANGER, CLASS_MONK, CLASS_AVAILABLE, CLASS_AVAILABLE2,
   CLASS_ANTIPALADIN, CLASS_PALADIN, CLASS_BARD
};

/*
 * Languages -- Altrag
 */
enum lang_array
{
   LANG_COMMON, LANG_ELVEN, LANG_DWARVEN, LANG_PIXIE,
   LANG_OGRE, LANG_ORCISH, LANG_TROLLISH, LANG_RODENT,
   LANG_INSECTOID, LANG_MAMMAL, LANG_REPTILE,
   LANG_DRAGON, LANG_SPIRITUAL, LANG_MAGICAL,
   LANG_GOBLIN, LANG_GOD, LANG_ANCIENT, LANG_HALFLING,
   LANG_CLAN, LANG_GITH, LANG_MINOTAUR, LANG_CENTAUR,
   LANG_GNOME, LANG_SAHUAGIN, LANG_UNKNOWN
};

#define VALID_LANGS    ( LANG_COMMON | LANG_ELVEN | LANG_DWARVEN | LANG_PIXIE  \
		       | LANG_OGRE | LANG_ORCISH | LANG_TROLLISH | LANG_GOBLIN \
		       | LANG_HALFLING | LANG_GITH | LANG_MINOTAUR | LANG_CENTAUR | LANG_GNOME \
			 | LANG_REPTILE | LANG_INSECTOID | LANG_SAHUAGIN )
#endif
