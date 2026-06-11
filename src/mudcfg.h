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
 *                         MUD Specific Definitions                         *
 ****************************************************************************/

#pragma once

/*
 * These definitions can be safely changed without fear of being overwritten by future patches.
 * This does not guarantee however that compatibility will be maintained if you change
 * or remove too much of this stuff. Best to keep the names and only change the values they
 * represent if you want to spare yourself the hassle. Samson 3-14-2004
 */

/*
 * Game parameters.
 * Increase the max'es if you add more of something.
 * Adjust the pulse numbers to suit yourself.
 */
extern const char *SPELL_SILENT_MARKER;

constexpr int MAX_LAYERS = 8;  /* maximum clothing layers */
constexpr int MAX_NEST = 100;  /* maximum container nesting */
constexpr int MAX_REXITS = 20; /* Maximum exits allowed in 1 room */
constexpr int MAX_CLASS = 20;  /* Be careful with these two figures - alot depends on them being only available to PCs */
constexpr int MAX_RACE = 20;
constexpr int MAX_NPC_CLASS = 26;
constexpr int MAX_NPC_RACE = 162; /* Good God almighty! If a race your looking for isn't available, you have problems!!! */
constexpr int MAX_BEACONS = 10;
constexpr int MAX_SAYHISTORY = 25;
constexpr int MAX_TELLHISTORY = 25;
constexpr int MAX_OINVOKE_QUANTITY = 20;   // Limit on how many copies of an object can be loaded by an imm at one time.
constexpr int MAX_FIGHT = 8;
constexpr int MAX_SKILL = 500; /* Raised during 1.4a patch */
constexpr int MAX_HERB = 100;
constexpr int MAX_DISEASE = 20;
constexpr int MAX_RGRID_ROOMS = 30000;
constexpr int MAX_MSG = 18;

extern int MAX_PC_RACE;
extern int MAX_PC_CLASS;

constexpr int MAX_LEVEL = 115;     /* Raised from 65 by Teklord */
constexpr int MAX_PERSONAL = 5;    /* Maximum personal skills */
constexpr int MAX_WHERE_NAME = 35; /* See act_info.cpp for the text messages */

constexpr int LEVEL_SUPREME = MAX_LEVEL;
constexpr int LEVEL_ADMIN = ( MAX_LEVEL - 1 );
constexpr int LEVEL_KL = ( MAX_LEVEL - 2 );
constexpr int LEVEL_IMPLEMENTOR = ( MAX_LEVEL - 3 );
constexpr int LEVEL_SUB_IMPLEM = ( MAX_LEVEL - 4 );
constexpr int LEVEL_ASCENDANT = ( MAX_LEVEL - 5 );
constexpr int LEVEL_GREATER = ( MAX_LEVEL - 6 );
constexpr int LEVEL_GOD = ( MAX_LEVEL - 7 );
constexpr int LEVEL_LESSER = ( MAX_LEVEL - 8 );
constexpr int LEVEL_TRUEIMM = ( MAX_LEVEL - 9 );
constexpr int LEVEL_DEMI = ( MAX_LEVEL - 10 );
constexpr int LEVEL_SAVIOR = ( MAX_LEVEL - 11 );
constexpr int LEVEL_CREATOR = ( MAX_LEVEL - 12 );
constexpr int LEVEL_ACOLYTE = ( MAX_LEVEL - 13 );
constexpr int LEVEL_IMMORTAL = ( MAX_LEVEL - 14 );
constexpr int LEVEL_AVATAR = ( MAX_LEVEL - 15 );

constexpr int LEVEL_LOG = LEVEL_LESSER;

#ifdef MULTIPORT
/* Port definitions for various commands - Samson 8-22-98 */
/* Now only works if Multiport support is enabled at compile time - Samson 7-12-02 */
constexpr int CODEPORT = 9700;
constexpr int BUILDPORT = 9600;
constexpr int MAINPORT = 9500;
#endif

/*
 * Well known mob virtual numbers.
 * Defined in #MOBILES.
 */
/* Added animate dead mobs - Whir - 8/29/98 */
constexpr int MOB_VNUM_WOODCALL1 = 11001;
constexpr int MOB_VNUM_WOODCALL2 = 11002;
constexpr int MOB_VNUM_WOODCALL3 = 11003;
constexpr int MOB_VNUM_WOODCALL4 = 11004;
constexpr int MOB_VNUM_WOODCALL5 = 11005;
constexpr int MOB_VNUM_WOODCALL6 = 11006;
constexpr int MOB_VNUM_WARMOUNTTWO = 11007;      // Antipaladin warmount - Samson 4-2-00 */
constexpr int MOB_VNUM_WARMOUNTTHREE = 11008;    // Paladin flying warmount - Samson 4-2-00 */
constexpr int MOB_VNUM_WARMOUNTFOUR = 11009;     // Antipaladin flying warmount - Samson 4-2-00 */
constexpr int MOB_VNUM_GNOME_FLYER = 11010;      // Gnomish Flyer - for the Tinker skill
constexpr int MOB_VNUM_GATE = 11029;             // Gate spell servitor daemon - Samson 3-26-98 */
constexpr int MOB_VNUM_MAP_TRAVELER = 11030;     // Random traveler for Overland city sectors
constexpr int MOB_VNUM_MAP_MERCHANT = 11031;     // Random merchant for Overland city sectors
constexpr int MOB_VNUM_MAP_GYPSY = 11032;        // Random gypsy for Overland city sectors
constexpr int MOB_VNUM_MAP_BANDIT = 11033;       // Random bandit for Overland city sectors
constexpr int MOB_VNUM_MAP_FARMER = 11034;       // Random farmer for Overland field sectors
constexpr int MOB_VNUM_MAP_COW = 11035;          // Random cow for Overland field sectors
constexpr int MOB_VNUM_MAP_RABBIT = 11036;       // Random rabbit for Overland field sectors
constexpr int MOB_VNUM_MAP_BULL = 11037;         // Random bull for Overland field sectors
constexpr int MOB_VNUM_MAP_DEER = 11038;         // Random deer for Overland forest sectors
constexpr int MOB_VNUM_MAP_DRYAD = 11039;        // Random dryad for Overland forest sectors
constexpr int MOB_VNUM_MAP_TREANT = 11040;       // Random treant for Overland forest sectors
constexpr int MOB_VNUM_MAP_WURM = 11041;         // Random wurm for Overland forest sectors
constexpr int MOB_VNUM_MAP_DWARF = 11042;        // Random dwarf for Overland hill sectors
constexpr int MOB_VNUM_MAP_BADGER = 11043;       // Random rock badger for Overland hill sectors
constexpr int MOB_VNUM_MAP_CROW = 11044;         // Random crow for Overland hill sectors
constexpr int MOB_VNUM_MAP_DRAGON = 11045;       // Random dragon for Overland hill sectors
constexpr int MOB_VNUM_MAP_GOAT = 11046;         // Random goat for Overland mountain sectors
constexpr int MOB_VNUM_MAP_HOUND = 11047;        // Random wild hound for Overland mountain sectors
constexpr int MOB_VNUM_MAP_FIRBOLG = 11048;      // Random firbolg for Overland mountain sectors
constexpr int MOB_VNUM_MAP_BEETLE = 11049;       // Random scarab beetle for Overland desert sectors
constexpr int MOB_VNUM_MAP_NOMAD = 11050;        // Random nomad for Overland desert sectors
constexpr int MOB_VNUM_MAP_ELEMENTAL = 11051;    // Random sand elemental for Overland desert sectors
constexpr int MOB_VNUM_MAP_STIRGE = 11052;       // Random stirge for Overland swamp sectors
constexpr int MOB_VNUM_MAP_GOBLIN = 11053;       // Random goblin for Overland swamp sectors
constexpr int MOB_VNUM_MAP_ORANGUTAN = 11054;    // Random orangutan for Overland jungle sectors
constexpr int MOB_VNUM_MAP_PYTHON = 11055;       // Random python for Overland jungle sectors
constexpr int MOB_VNUM_MAP_LIZARD = 11056;       // Random monitor lizard for Overland jungle sectors
constexpr int MOB_VNUM_MAP_PANTHER = 11057;      // Random panther for Overland jungle sectors
constexpr int MOB_VNUM_MAP_CRAB = 11058;         // Random crab for Overland shore sectors
constexpr int MOB_VNUM_MAP_SEAGULL = 11059;      // Random seagull for Overland shore sectors
constexpr int MOB_VNUM_MAP_HYENA = 11060;        // Random hyena for Overland scrub sectors
constexpr int MOB_VNUM_MAP_MEERKAT = 11061;      // Random meerkat for Overland scrub sectors
constexpr int MOB_VNUM_MAP_ARMADILLO = 11062;    // Random armadillo for Overland scrub sectors
constexpr int MOB_VNUM_MAP_MANTICORE = 11063;    // Random manticore for Overland scrub sectors
constexpr int MOB_VNUM_SKYSHIP = 11072;
constexpr int MOB_VNUM_CITYGUARD = 11074;        // Replaced original cityguard - Samson 3-26-98 */
constexpr int MOB_VNUM_SUPERMOB = 11402;
constexpr int MOB_VNUM_ANIMATED_CORPSE = 11403;
constexpr int MOB_VNUM_ANIMATED_SKELETON = 11404;
constexpr int MOB_VNUM_ANIMATED_ZOMBIE = 11405;
constexpr int MOB_VNUM_ANIMATED_GHOUL = 11406;
constexpr int MOB_VNUM_ANIMATED_CRYPT_THING = 11407;
constexpr int MOB_VNUM_ANIMATED_MUMMY = 11408;
constexpr int MOB_VNUM_ANIMATED_GHOST = 11409;
constexpr int MOB_VNUM_ANIMATED_DEATH_KNIGHT = 11410;
constexpr int MOB_VNUM_ANIMATED_DRACOLICH = 11411;
constexpr int MOB_VNUM_CREEPINGDOOM = 11412;           // Creeping Doom mob - Samson 9-27-98 */
constexpr int MOB_DOPPLEGANGER = 11413;                // Doppleganger base mob - Samson 10-11-99 */
constexpr int MOB_VNUM_WARMOUNT = 11414;               // Paladin warmount - Samson 10-13-98 */

/* Deity avatars */

/*
 * Well known object virtual numbers.
 * Defined in #OBJECTS.
 */
constexpr int OBJ_VNUM_DUMMYOBJ = 11000;       // This one is used by resets to make sure they work right - Samson 2/25/05
constexpr int OBJ_VNUM_FIREPIT = 11001;        // Campfires turn into these when dead - Samson 8-10-98
constexpr int OBJ_VNUM_CAMPFIRE = 11002;       // Campfire that loads when a player camps - Samson 8-10-98
constexpr int OBJ_VNUM_BEDROLL = 11003;        // Bedroll for camping gear
constexpr int OBJ_VNUM_FIREWOOD = 11004;       // Firewood for camping gear
constexpr int OBJ_VNUM_OVFIRE = 11005;         // Overland environmental fire
constexpr int OBJ_VNUM_CAMPGEAR = 11006;       // Generic camping gear object
constexpr int OBJ_VNUM_FIRESEED = 11007;       // Fireseed object for spell_fireseed - Samson 10-13-98
constexpr int OBJ_VNUM_FORAGE = 11027;         // This is the start of a range of 10 vnums - Samson 5-1-04
                                           //  ( 11027 -> 11037 )
constexpr int OBJ_VNUM_FLAMETHROWER = 11039;   // Gnomish Flamethrower - for the Tinker skill
constexpr int OBJ_VNUM_LADDER = 11040;         // Gnomish Ladder - for the Tinker skill
constexpr int OBJ_VNUM_DIGGER = 11041;         // Gnomish Differ - for the Tinker skill
constexpr int OBJ_VNUM_GNOME_LOCKPICK = 11042; // Gnomish Lockpick - for the Tinker skill
constexpr int OBJ_VNUM_REBREATHER = 11043;     // Gnomish Rebreather - for the Tinker skill
constexpr int OBJ_VNUM_TREASURE = 11044;       // Base object used for generating random treasure in treasure.cpp
constexpr int OBJ_VNUM_RUNE = 11045;           // Base object used to generate runes in treasure.cpp for the socketing system
constexpr int OBJ_VNUM_MAPS = 11046;           // Object used for mapout - Samson 1/28/06
constexpr int OBJ_VNUM_ORE_BASE = 11299;       // This object does not actually exist, it's used by the blacksmithing code in treasure.cpp
constexpr int OBJ_VNUM_MINING = 11300;         // This is the start of a range of 5 vnums - Samson 5-1-04
                                           // 11300 -> 11304 [11305 and 11306 are special cases]
constexpr int OBJ_VNUM_TAN_JACKET = 11368;     // Block of objects used by the Tan skill in skills.cpp
constexpr int OBJ_VNUM_TAN_BOOTS = 11369;
constexpr int OBJ_VNUM_TAN_GLOVES = 11370;
constexpr int OBJ_VNUM_TAN_LEGGINGS = 11371;
constexpr int OBJ_VNUM_TAN_SLEEVES = 11372;
constexpr int OBJ_VNUM_TAN_HELMET = 11373;
constexpr int OBJ_VNUM_TAN_BAG = 11374;
constexpr int OBJ_VNUM_TAN_CLOAK = 11375;
constexpr int OBJ_VNUM_TAN_BELT = 11376;
constexpr int OBJ_VNUM_TAN_COLLAR = 11377;
constexpr int OBJ_VNUM_TAN_WATERSKIN = 11378;
constexpr int OBJ_VNUM_TAN_QUIVER = 11379;
constexpr int OBJ_VNUM_TAN_WHIP = 11380;
constexpr int OBJ_VNUM_TAN_SHIELD = 11381;

constexpr int OBJ_VNUM_PUDDLE = 11400;
constexpr int OBJ_VNUM_MONEY_ONE = 11401;
constexpr int OBJ_VNUM_MONEY_SOME = 11402;
constexpr int OBJ_VNUM_CORPSE_NPC = 11403;
constexpr int OBJ_VNUM_CORPSE_PC = 11404;
constexpr int OBJ_VNUM_SEVERED_HEAD = 11405;
constexpr int OBJ_VNUM_TORN_HEART = 11406;
constexpr int OBJ_VNUM_SLICED_ARM = 11407;
constexpr int OBJ_VNUM_SLICED_LEG = 11408;
constexpr int OBJ_VNUM_SPILLED_GUTS = 11409;
constexpr int OBJ_VNUM_BLOOD = 11410;
constexpr int OBJ_VNUM_BLOODSTAIN = 11411;
constexpr int OBJ_VNUM_SCRAPS = 11412;
constexpr int OBJ_VNUM_MUSHROOM = 11413;
constexpr int OBJ_VNUM_LIGHT_BALL = 11414;
constexpr int OBJ_VNUM_SPRING = 11415;
constexpr int OBJ_VNUM_SLICE = 11417;
constexpr int OBJ_VNUM_SHOPPING_BAG = 11418;

constexpr int OBJ_VNUM_FIRE = 11423;
constexpr int OBJ_VNUM_TRAP = 11424;
constexpr int OBJ_VNUM_PORTAL = 11425;
constexpr int OBJ_VNUM_BLACK_POWDER = 11426;
constexpr int OBJ_VNUM_SCROLL_SCRIBING = 11427;
constexpr int OBJ_VNUM_FLASK_BREWING = 11428;
constexpr int OBJ_VNUM_NOTE = 11429;

constexpr int OBJ_VNUM_WAND_CHARGING = 11432;

constexpr int OBJ_VNUM_BRAINS = 11435;
constexpr int OBJ_VNUM_HANDS = 11436;
constexpr int OBJ_VNUM_FOOT = 11437;
constexpr int OBJ_VNUM_FINGERS = 11438;
constexpr int OBJ_VNUM_EAR = 11439;
constexpr int OBJ_VNUM_EYE = 11440;
constexpr int OBJ_VNUM_TONGUE = 11441;
constexpr int OBJ_VNUM_EYESTALK = 11442;
constexpr int OBJ_VNUM_TENTACLE = 11443;
constexpr int OBJ_VNUM_FINS = 11444;
constexpr int OBJ_VNUM_WINGS = 11445;
constexpr int OBJ_VNUM_TAIL = 11446;
constexpr int OBJ_VNUM_SCALES = 11447;
constexpr int OBJ_VNUM_TUSKS = 11448;
constexpr int OBJ_VNUM_HORNS = 11449;
constexpr int OBJ_VNUM_CLAWS = 11450;

constexpr int OBJ_VNUM_SHELL = 11461;
constexpr int OBJ_VNUM_FEATHERS = 11462;
constexpr int OBJ_VNUM_FORELEG = 11463;
constexpr int OBJ_VNUM_PAWS = 11464;
constexpr int OBJ_VNUM_HOOVES = 11465;
constexpr int OBJ_VNUM_BEAK = 11466;
constexpr int OBJ_VNUM_SHARPSCALE = 11467;
constexpr int OBJ_VNUM_HAUNCHES = 11468;
constexpr int OBJ_VNUM_FANGS = 11469;

/* Academy eq */
constexpr int OBJ_VNUM_SCHOOL_BANNER = 11478;
constexpr int OBJ_VNUM_NEWBIE_GUIDE = 11479;

/*
 * Well known room virtual numbers.
 * Defined in #ROOMS.
 */
constexpr int ROOM_AUTH_START = 100;       /* Pregame Entry, auth system on */
constexpr int ROOM_NOAUTH_START = 102;     /* Pregame Entry, auth system off */
constexpr int ROOM_VNUM_LIMBO = 11401;
constexpr int ROOM_VNUM_POLY = 11402;
constexpr int ROOM_VNUM_RAREUPDATE = 11402;   /* Room where players get scanned for rare items - Samson 1-24-00 */
constexpr int ROOM_VNUM_HELL = 11405;   /* Vnum for Hell - Samson */
constexpr int ROOM_VNUM_CHAT = 11406;   /* Parlour of the Immortals */
constexpr int ROOM_VNUM_TEMPLE = 11407; /* Primary continent recall */
constexpr int ROOM_VNUM_ALTAR = 11407;  /* Primary continent death */
constexpr int ROOM_VNUM_DONATION = 11410;  /* First donation room - Samson 2-6-98 */
constexpr int ROOM_VNUM_REDEEM = 11411; /* Sindhae prize redemption start room - Samson 6-2-00 */
constexpr int ROOM_VNUM_ENDREDEEM = 11412; /* Sindhae prize redemption ending room - Samson 6-2-00 */

/* New continent and plane support - Samson 3-28-98
 * Name of continent or plane is followed by the recall and death zone.
 * Area continent flags for continent and plane system, revised format - Samson 8-8-98
 */
enum acon_types
{
   ACON_ONE, ACON_ASTRAL, ACON_IMMORTAL, ACON_MAX
};

// Playable Races
// If you add a new race to this table, make sure you update update_aris in character.cpp as well
// Also make sure the array in const.cpp is synced with this.
enum race_types
{
   RACE_HUMAN, RACE_HIGH_ELF, RACE_DWARF, RACE_HALFLING, RACE_PIXIE,
   RACE_HALF_OGRE, RACE_HALF_ORC, RACE_HALF_TROLL, RACE_HALF_ELF, RACE_GITH,
   RACE_MINOTAUR, RACE_DUERGAR, RACE_CENTAUR, RACE_IGUANADON,
   RACE_GNOME, RACE_DROW, RACE_WILD_ELF, RACE_INSECTOID, RACE_SAUGHIN, RACE_19
};

/* NPC Races */
constexpr int RACE_HALFBREED = 20;
constexpr int RACE_REPTILE = 21;
constexpr int RACE_SPECIAL = 22;
constexpr int RACE_LYCANTH = 23;
constexpr int RACE_DRAGON = 24;
constexpr int RACE_UNDEAD = 25;
constexpr int RACE_ORC = 26;
constexpr int RACE_INSECT = 27;
constexpr int RACE_ARACHNID = 28;
constexpr int RACE_DINOSAUR = 29;
constexpr int RACE_FISH = 30;
constexpr int RACE_BIRD = 31;
constexpr int RACE_GIANT = 32; /* generic giant more specials down ---V */
constexpr int RACE_PREDATOR = 33;
constexpr int RACE_PARASITE = 34;
constexpr int RACE_SLIME = 35;
constexpr int RACE_DEMON = 36;
constexpr int RACE_SNAKE = 37;
constexpr int RACE_HERBIV = 38;
constexpr int RACE_TREE = 39;
constexpr int RACE_VEGGIE = 40;
constexpr int RACE_ELEMENT = 41;
constexpr int RACE_PLANAR = 42;
constexpr int RACE_DEVIL = 43;
constexpr int RACE_GHOST = 44;
constexpr int RACE_GOBLIN = 45;
constexpr int RACE_TROLL = 46;
constexpr int RACE_VEGMAN = 47;
constexpr int RACE_MFLAYER = 48;
constexpr int RACE_PRIMATE = 49;
constexpr int RACE_ENFAN = 50;
constexpr int RACE_GOLEM = 51;
constexpr int RACE_SKEXIE = 52;
constexpr int RACE_TROGMAN = 53;
constexpr int RACE_PATRYN = 54;
constexpr int RACE_LABRAT = 55;
constexpr int RACE_SARTAN = 56;
constexpr int RACE_TYTAN = 57;
constexpr int RACE_SMURF = 58;
constexpr int RACE_ROO = 59;
constexpr int RACE_HORSE = 60;
constexpr int RACE_DRAAGDIM = 61;
constexpr int RACE_ASTRAL = 62;
constexpr int RACE_GOD = 63;

constexpr int RACE_GIANT_HILL = 64;
constexpr int RACE_GIANT_FROST = 65;
constexpr int RACE_GIANT_FIRE = 66;
constexpr int RACE_GIANT_CLOUD = 67;
constexpr int RACE_GIANT_STORM = 68;
constexpr int RACE_GIANT_STONE = 69;

constexpr int RACE_DRAGON_RED = 70;
constexpr int RACE_DRAGON_BLACK = 71;
constexpr int RACE_DRAGON_GREEN = 72;
constexpr int RACE_DRAGON_WHITE = 73;
constexpr int RACE_DRAGON_BLUE = 74;
constexpr int RACE_DRAGON_SILVER = 75;
constexpr int RACE_DRAGON_GOLD = 76;
constexpr int RACE_DRAGON_BRONZE = 77;
constexpr int RACE_DRAGON_COPPER = 78;
constexpr int RACE_DRAGON_BRASS = 79;

constexpr int RACE_VAMPIRE = 80;
constexpr int RACE_UNDEAD_VAMPIRE = 80;
constexpr int RACE_LICH = 81;
constexpr int RACE_UNDEAD_LICH = 81;
constexpr int RACE_WIGHT = 82;
constexpr int RACE_UNDEAD_WIGHT = 82;
constexpr int RACE_GHAST = 83;
constexpr int RACE_UNDEAD_GHAST = 83;
constexpr int RACE_SPECTRE = 84;
constexpr int RACE_UNDEAD_SPECTRE = 84;
constexpr int RACE_ZOMBIE = 85;
constexpr int RACE_UNDEAD_ZOMBIE = 85;
constexpr int RACE_SKELETON = 86;
constexpr int RACE_UNDEAD_SKELETON = 86;
constexpr int RACE_GHOUL = 87;
constexpr int RACE_UNDEAD_GHOUL = 87;

constexpr int RACE_HALF_GIANT = 88;
constexpr int RACE_DEEP_GNOME = 89;
constexpr int RACE_GNOLL = 90;

constexpr int RACE_GOLD_ELF = 91;
constexpr int RACE_GOLD_ELVEN = 91;
constexpr int RACE_SEA_ELF = 92;
constexpr int RACE_SEA_ELVEN = 92;

/* 10-20-96 Admiral */
constexpr int RACE_TIEFLING = 93;
constexpr int RACE_AASIMAR = 94;
constexpr int RACE_SOLAR = 95;
constexpr int RACE_PLANITAR = 96;
constexpr int RACE_UNDEAD_SHADOW = 97;
constexpr int RACE_GIANT_SKELETON = 98;
constexpr int RACE_NILBOG = 99;
constexpr int RACE_HOUSERS = 100;
constexpr int RACE_BAKU = 101;
constexpr int RACE_BEASTLORD = 102;
constexpr int RACE_DEVAS = 103;
constexpr int RACE_POLARIS = 104;
constexpr int RACE_DEMODAND = 105;
constexpr int RACE_TARASQUE = 106;
constexpr int RACE_DIETY = 107;
constexpr int RACE_DAEMON = 108;
constexpr int RACE_VAGABOND = 109;

/* Imported races from old Alsherok code - Samson */
constexpr int RACE_GARGOYLE = 110;
constexpr int RACE_BEAR = 111;
constexpr int RACE_BAT = 112;
constexpr int RACE_CAT = 113;
constexpr int RACE_DOG = 114;
constexpr int RACE_ANT = 115;
constexpr int RACE_APE = 116;
constexpr int RACE_BABOON = 117;
constexpr int RACE_BEE = 118;
constexpr int RACE_BEETLE = 119;
constexpr int RACE_BOAR = 120;
constexpr int RACE_BUGBEAR = 121;
constexpr int RACE_FERRET = 122;
constexpr int RACE_FLY = 123;
constexpr int RACE_GELATIN = 124;
constexpr int RACE_GORGON = 125;
constexpr int RACE_HARPY = 126;
constexpr int RACE_HOBGOBLIN = 127;
constexpr int RACE_KOBOLD = 128;
constexpr int RACE_LOCUST = 129;
constexpr int RACE_MOLD = 130;
constexpr int RACE_MULE = 131;
constexpr int RACE_NEANDERTHAL = 132;
constexpr int RACE_OOZE = 133;
constexpr int RACE_RAT = 134;
constexpr int RACE_RUSTMONSTER = 135;
constexpr int RACE_SHAPESHIFTER = 136;
constexpr int RACE_SHREW = 137;
constexpr int RACE_SHRIEKER = 138;
constexpr int RACE_STIRGE = 139;
constexpr int RACE_THOUL = 140;
constexpr int RACE_WOLF = 141;
constexpr int RACE_WORM = 142;
constexpr int RACE_BOVINE = 143;
constexpr int RACE_CANINE = 144;
constexpr int RACE_FELINE = 145;
constexpr int RACE_PORCINE = 146;
constexpr int RACE_MAMMAL = 147;
constexpr int RACE_RODENT = 148;
constexpr int RACE_AMPHIBIAN = 149;
constexpr int RACE_CRUSTACEAN = 150;
constexpr int RACE_SPIRIT = 151;
constexpr int RACE_MAGICAL = 152;
constexpr int RACE_ANIMAL = 153;
constexpr int RACE_HUMANOID = 154;
constexpr int RACE_MONSTER = 155;
constexpr int RACE_UNUSED1 = 156;
constexpr int RACE_UNUSED2 = 157;
constexpr int RACE_UNUSED3 = 158;
constexpr int RACE_UNUSED4 = 159;
constexpr int RACE_UNUSED5 = 160;
constexpr int RACE_UNUSED6 = 161;

constexpr int CLASS_NONE = -1; /* For skill/spells according to guild */

// Playable Classes
// If you add a new class to this, make sure you update update_aris in character.cpp as well.
// You also need to make sure the array in const.cpp is synced with this.
enum class_types
{
   CLASS_MAGE, CLASS_CLERIC, CLASS_ROGUE, CLASS_WARRIOR, CLASS_NECROMANCER,
   CLASS_DRUID, CLASS_RANGER, CLASS_MONK, CLASS_PC08, CLASS_PC09,
   CLASS_ANTIPALADIN, CLASS_PALADIN, CLASS_BARD, CLASS_PC13, CLASS_PC14, CLASS_PC15,
   CLASS_PC16, CLASS_PC17, CLASS_PC18, CLASS_PC19
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

#define VALID_LANGS ( LANG_COMMON | LANG_ELVEN | LANG_DWARVEN | LANG_PIXIE | LANG_OGRE | LANG_ORCISH | LANG_TROLLISH | LANG_GOBLIN | LANG_HALFLING | LANG_GITH | LANG_MINOTAUR | LANG_CENTAUR | LANG_GNOME | LANG_REPTILE | LANG_INSECTOID | LANG_SAHUAGIN )
