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
 *                          Main mud header file                            *
 ****************************************************************************/

#pragma once

#include <bitset>
#include <chrono>
#include <cstring> // Can't be rid of me until you've converted every last string in the code to std::string, or at least move it to any files still needing it for things like strdup.
#include <list>
#include <map>
#include <random>
#include <typeinfo>
#include <vector>

/* Used in basereport and world commands - don't remove these!
 * If you want to add your own, make a new set of defines and ADD the information.
 * Removing this is a violation of your license agreement.
 */
#define CODENAME "AFKMud"
#define CODEVERSION "3.0.0"
#define COPYRIGHT "Copyright The Alsherok Team 1997-2026. All rights reserved."

// Backward compatibility for snippets and such.
#define mudstrlcpy strlcpy
#define mudstrlcat strlcat
#define str_dup strdup

// String and memory management parameters. Will one day be a thing of the past once all the char[] arrays that use them are upgraded.
const int MSL = 8192; // MAX_STRING_LENGTH
const int MIL = 2048; // MAX_INPUT_LENGTH

// No idea what the purpose in doing this is, but BERR appears to be something Smaug made up and it's better defined this way.
const short BERR = 255;

// Not sure why Smaug did this to begin with but it's widely used as a return type everywhere.
typedef int ch_ret;

// Class declarations
class descriptor_data;
class area_data;
class mob_index;
class obj_index;
class room_index;
class char_data;
class pc_data;
class obj_data;

// Used in class headers - needs to be declared early
void init_memory( void *, void *, unsigned int );
void bug( const char *, ... ) __attribute__ ( ( format( printf, 1, 2 ) ) );

// Function types. Samson. Stop. Don't do it! NO! You have to keep these things you idiot!
typedef void DO_FUN( char_data * ch, std::string argument );
typedef ch_ret SPELL_FUN( int sn, int level, char_data * ch, void *vo );
typedef bool SPEC_FUN( char_data * ch );

/*
 * "Oh God Samson, this is so HACKISH!" "Yes my son, but this preserves dlsym, which is good."
 * "*The masses bow down in worship to their God....*"
 */
#define CMDF( name ) extern "C" void (name)( char_data *ch, std::string argument )
#define SPELLF( name ) extern "C" ch_ret (name)( int sn, int level, char_data *ch, void *vo )
#define SPECF( name ) extern "C" bool (name)( char_data *ch )

/*
 * Used in exactly 12 places, this is a multiplier on the duration of the spells and skills it affects.
 * It seems to have once been related to the number of pulses per second but does not appear bound to that now due to having been quadrupled.
 * It also has no need to be a define vs a proper constant value.
 */
const float DUR_CONV = 93.333333333333333333333333; /* Original value: 23.333333333333333333333333... - Quadrupled for time changes */

/*
 * Hidden tilde char generated with alt155 on the number pad.
 * The code blocks the use of these symbols by default, so this should be quite safe. Samson 3-14-04
 */
const char HIDDEN_TILDE = '\xa2';

// 32bit bitvector defines
const int BV00 = ( 1 << 0 );
const int BV01 = ( 1 << 1 );
const int BV02 = ( 1 << 2 );
const int BV03 = ( 1 << 3 );
const int BV04 = ( 1 << 4 );
const int BV05 = ( 1 << 5 );
const int BV06 = ( 1 << 6 );
const int BV07 = ( 1 << 7 );
const int BV08 = ( 1 << 8 );
const int BV09 = ( 1 << 9 );
const int BV10 = ( 1 << 10 );
const int BV11 = ( 1 << 11 );
const int BV12 = ( 1 << 12 );
const int BV13 = ( 1 << 13 );
const int BV14 = ( 1 << 14 );
const int BV15 = ( 1 << 15 );
const int BV16 = ( 1 << 16 );
const int BV17 = ( 1 << 17 );
const int BV18 = ( 1 << 18 );
const int BV19 = ( 1 << 19 );
const int BV20 = ( 1 << 20 );
const int BV21 = ( 1 << 21 );
const int BV22 = ( 1 << 22 );
const int BV23 = ( 1 << 23 );
const int BV24 = ( 1 << 24 );
const int BV25 = ( 1 << 25 );
const int BV26 = ( 1 << 26 );
const int BV27 = ( 1 << 27 );
const int BV28 = ( 1 << 28 );
const int BV29 = ( 1 << 29 );
const int BV30 = ( 1 << 30 );
const int BV31 = ( 1 << 31 );
// 32 USED! DO NOT ADD MORE! SB

/*
 * Command logging types.
 * DON'T FORGET TO ADD THESE TO BUILD.CPP !!!
 */
enum log_types
{
   LOG_NORMAL, LOG_ALWAYS, LOG_NEVER, LOG_BUILD, LOG_HIGH, LOG_COMM, LOG_WARN, LOG_INFO, LOG_AUTH, LOG_DEBUG, LOG_ALL
};

/*
 * Return types for move_char, damage, greet_trigger, etc, etc
 * Added by Thoric to get rid of bugs
 */
enum ret_types
{
   rNONE, rCHAR_DIED, rVICT_DIED, rSPELL_FAILED, rVICT_IMMUNE, rSTOP, rERROR = 255
};

// Echo types for echo_to_all
enum echo_types
{
   ECHOTAR_ALL, ECHOTAR_PC, ECHOTAR_PK, ECHOTAR_IMM
};

// These are skill_lookup return values for common skills and spells.
extern short gsn_style_evasive;
extern short gsn_style_defensive;
extern short gsn_style_standard;
extern short gsn_style_aggressive;
extern short gsn_style_berserk;

extern short gsn_backheel;          //Samson 5-31-00
extern short gsn_metallurgy;        // Samson 5-31-00
extern short gsn_scout;             // Samson 5-29-00
extern short gsn_scry;              // Samson 5-29-00
extern short gsn_tinker;            // Samson 4-25-00
extern short gsn_deathsong;         // Samson 4-25-00
extern short gsn_swim;              // Samson 4-24-00
extern short gsn_tenacity;          // Samson 4-24-00
extern short gsn_bargain;           // Samson 4-23-00
extern short gsn_bladesong;         // Samson 4-23-00
extern short gsn_elvensong;         // Samson 4-23-00
extern short gsn_reverie;           // Samson 4-23-00
extern short gsn_mining;            // Samson 4-17-00
extern short gsn_woodcall;          // Samson 4-17-00
extern short gsn_forage;            // Samson 3-26-00
extern short gsn_spy;               // Samson 6-20-99
extern short gsn_charge;            // Samson 6-07-99
extern short gsn_retreat;           // Samson 5-27-99
extern short gsn_detrap;
extern short gsn_backstab;
extern short gsn_circle;
extern short gsn_cook;
extern short gsn_assassinate;       // Samson
extern short gsn_feign;             // Samson 10-22-98
extern short gsn_quiv;              // Samson 6-02-99
extern short gsn_dodge;
extern short gsn_hide;
extern short gsn_peek;
extern short gsn_pick_lock;
extern short gsn_sneak;
extern short gsn_steal;
extern short gsn_gouge;
extern short gsn_track;
extern short gsn_search;
extern short gsn_dig;
extern short gsn_mount;
extern short gsn_bashdoor;
extern short gsn_berserk;
extern short gsn_hitall;

extern short gsn_disarm;
extern short gsn_enhanced_damage;
extern short gsn_kick;
extern short gsn_parry;
extern short gsn_rescue;
extern short gsn_dual_wield;
extern short gsn_grapple;
extern short gsn_cleave;

extern short gsn_aid;

// used to do specific lookups
extern short num_skills;
extern short num_sorted_skills;

// spells
extern short gsn_blindness;
extern short gsn_charm_person;
extern short gsn_aqua_breath;
extern short gsn_curse;
extern short gsn_invis;
extern short gsn_mass_invis;
extern short gsn_poison;
extern short gsn_sleep;
extern short gsn_silence;           // Samson 9-26-98
extern short gsn_paralyze;          // Samson 9-26-98
extern short gsn_fireball;          // for fireshield
extern short gsn_chill_touch;       // for iceshield
extern short gsn_lightning_bolt;    // for shockshield

// newer attack skills
extern short gsn_punch;
extern short gsn_bash;
extern short gsn_stun;
extern short gsn_bite;
extern short gsn_claw;
extern short gsn_sting;
extern short gsn_tail;

extern short gsn_poison_weapon;
extern short gsn_scribe;
extern short gsn_brew;
extern short gsn_climb;

// changed to new weapon types - Grimm
extern short gsn_pugilism;
extern short gsn_swords;
extern short gsn_daggers;
extern short gsn_whips;
extern short gsn_talonous_arms;
extern short gsn_maces_hammers;
extern short gsn_blowguns;
extern short gsn_slings;
extern short gsn_axes;
extern short gsn_spears;
extern short gsn_staves;
extern short gsn_archery;

extern short gsn_grip;
extern short gsn_slice;
extern short gsn_tumble;

// Language gsns. -- Altrag
extern short gsn_common;
extern short gsn_elven;
extern short gsn_dwarven;
extern short gsn_pixie;
extern short gsn_ogre;
extern short gsn_orcish;
extern short gsn_trollish;
extern short gsn_goblin;
extern short gsn_halfling;

extern short gsn_tan;
extern short gsn_dragon_ride;

 // Extra description data for a room or object.
struct extra_descr_data
{
   std::string keyword; // Keyword in look/examine
   std::string desc;    // What to see
};

#include "mudcfg.h" // Contains definitions specific to your mud - will not be covered by patches. Samson 3-14-04
#include "color.h"  // Custom color stuff
#include "olc.h"    // Oasis OLC code and global definitions

// Time and weather stuff.
enum sun_positions
{
   SUN_DARK, SUN_RISE, SUN_LIGHT, SUN_SET
};

enum sky_conditions
{
   SKY_CLOUDLESS, SKY_CLOUDY, SKY_RAINING, SKY_LIGHTNING
};

enum temp_conditions
{
   TEMP_COLD, TEMP_COOL, TEMP_NORMAL, TEMP_WARM, TEMP_HOT
};

enum precip_conditions
{
   PRECIP_ARID, PRECIP_DRY, PRECIP_NORMAL, PRECIP_DAMP, PRECIP_WET
};

enum wind_conditions
{
   WIND_STILL, WIND_CALM, WIND_NORMAL, WIND_BREEZY, WIND_WINDY
};

#define GET_TEMP_UNIT(weather)   ((weather->temp + 3*weath_unit - 1)/weath_unit)
#define GET_PRECIP_UNIT(weather) ((weather->precip + 3*weath_unit - 1)/weath_unit)
#define GET_WIND_UNIT(weather)   ((weather->wind + 3*weath_unit - 1)/weath_unit)

#define IS_RAINING(weather)      (GET_PRECIP_UNIT(weather)>PRECIP_NORMAL)
#define IS_WINDY(weather)        (GET_WIND_UNIT(weather)>WIND_NORMAL)
#define IS_CLOUDY(weather)       (GET_PRECIP_UNIT(weather)>1)
#define IS_TCOLD(weather)        (GET_TEMP_UNIT(weather)==TEMP_COLD)
#define IS_COOL(weather)         (GET_TEMP_UNIT(weather)==TEMP_COOL)
#define IS_HOT(weather)          (GET_TEMP_UNIT(weather)==TEMP_HOT)
#define IS_WARM(weather)         (GET_TEMP_UNIT(weather)==TEMP_WARM)
#define IS_SNOWING(weather)      (IS_RAINING(weather) && IS_COOL(weather))

struct time_info_data
{
   int hour;
   int day;
   int month;
   int year;
   int sunlight;
   int season;    // Samson 5-6-99
};

// short cut crash bug fix provided by gfinello@mail.karmanet.it
enum relation_type
{
   relMSET_ON, relOSET_ON
};

struct rel_data
{
   void *Actor;
   void *Subject;
   relation_type Type;
};

// Connected state for a descriptor.
enum connection_types
{
   CON_GET_NAME = -99, CON_GET_OLD_PASSWORD,
   CON_CONFIRM_NEW_NAME, CON_GET_NEW_PASSWORD, CON_CONFIRM_NEW_PASSWORD,
   CON_GET_PORT_PASSWORD, CON_GET_NEW_SEX, CON_PRESS_ENTER,
   CON_READ_MOTD, CON_COPYOVER_RECOVER, CON_PLOADED,
   CON_PLAYING = 0, CON_EDITING, CON_ROLL_STATS,
   CON_OEDIT, CON_MEDIT, CON_REDIT, /* Oasis OLC */
   CON_PRIZENAME, CON_CONFIRMPRIZENAME, CON_PRIZEKEY,
   CON_CONFIRMPRIZEKEY, CON_DELETE, CON_RAISE_STAT,
   CON_BOARD, CON_FORKED
};

// Character substates
enum char_substates
{
   SUB_NONE, SUB_PAUSE, SUB_PERSONAL_DESC, SUB_BAN_DESC, SUB_OBJ_LONG,
   SUB_OBJ_EXTRA, SUB_MOB_DESC, SUB_ROOM_DESC,
   SUB_ROOM_EXTRA, SUB_WRITING_NOTE, SUB_MPROG_EDIT,
   SUB_HELP_EDIT, SUB_SKILL_HELP_EDIT, SUB_PERSONAL_BIO, SUB_REPEATCMD,
   SUB_RESTRICTED, SUB_DEITYDESC, SUB_MORPH_DESC, SUB_MORPH_HELP,
   SUB_PROJ_DESC, SUB_SLAYCMSG, SUB_SLAYVMSG, SUB_SLAYRMSG, SUB_EDMOTD,
   SUB_ROOM_DESC_NITE, SUB_OVERLAND_DESC, SUB_BOARD_TO, SUB_BOARD_SUBJECT,
   SUB_BOARD_STICKY, SUB_BOARD_TEXT, SUB_BOARD_CONFIRM, SUB_BOARD_REDO_MENU,
   SUB_JOURNAL_WRITE, SUB_EDIT_ABORT,

   //timer types ONLY below this point
   SUB_TIMER_DO_ABORT = 128, SUB_TIMER_CANT_ABORT
};

// Attribute bonus structures.
struct str_app_type
{
   short tohit;
   short todam;
   short carry;
   short wield;
};

struct int_app_type
{
   short learn;
};

struct wis_app_type
{
   short practice;
};

struct dex_app_type
{
   short defensive;
};

struct con_app_type
{
   short hitp;
   short shock;
};

struct cha_app_type
{
   short charm;
};

struct lck_app_type
{
   short luck;
};

// TO types for act.
enum to_types
{
   TO_ROOM, TO_NOTVICT, TO_VICT, TO_CHAR, TO_CANSEE, TO_THIRD
};

/*
 * An affect.
 *
 * So limited... so few fields... should we add more?
 *
 * Son of a bitch! God damn piece of shit modifier value!
 * Yes its an incredible waste of space to use a bitset here for such a narrow set of conditions.
 * Don't blame us, blame the people who tried to shortcut things so badly. Samson 7-10-04
 */
class affect_data
{
 private:
   affect_data( const affect_data & a );
     affect_data & operator=( const affect_data & );

 public:
     affect_data(  );

   std::bitset<MAX_RIS_FLAG> rismod;
   int bit;
   int duration;
   int modifier;
   short location;
   short type;
};

// Autosave flags
enum autosave_flags
{
   SV_DEATH, SV_KILL, SV_PASSCHG, SV_DROP, SV_PUT, SV_GIVE, SV_AUTO,
   SV_ZAPDROP, SV_AUCTION, SV_GET, SV_RECEIVE, SV_IDLE, SV_FILL, SV_EMPTY, SV_MAX
};

/*
 * Flags for act_string -- Shaddai
 */
enum act_strings
{
   STRING_NONE, STRING_IMM
};

const int PT_WATER = 100;
const int PT_AIR = 200;
const int PT_EARTH = 300;
const int PT_FIRE = 400;

/*
 * Push/pull types for exits - Thoric
 * To differentiate between the current of a river, or a strong gust of wind
 */
enum dir_pulltypes
{
   PULL_UNDEFINED, PULL_VORTEX, PULL_VACUUM, PULL_SLIP, PULL_ICE, PULL_MYSTERIOUS,
   PULL_CURRENT = PT_WATER, PULL_WAVE, PULL_WHIRLPOOL, PULL_GEYSER,
   PULL_WIND = PT_AIR, PULL_STORM, PULL_COLDWIND, PULL_BREEZE,
   PULL_LANDSLIDE = PT_EARTH, PULL_SINKHOLE, PULL_QUICKSAND, PULL_EARTHQUAKE,
   PULL_LAVA = PT_FIRE, PULL_HOTAIR
};

// Conditions.
enum pc_conditions
{
   COND_DRUNK, COND_FULL, COND_THIRST, MAX_CONDS
};

// Styles.
enum combat_styles
{
   STYLE_BERSERK, STYLE_AGGRESSIVE, STYLE_FIGHTING, STYLE_DEFENSIVE, STYLE_EVASIVE
};

/*
 * Bits for pc_data->flags
 * DAMMIT! Don't forget to add these things to build.cpp!!
 */
enum pc_flags
{
   PCFLAG_NONE, PCFLAG_DEADLY, PCFLAG_UNAUTHED, PCFLAG_NORECALL, PCFLAG_NOINTRO, PCFLAG_GAG,
   PCFLAG_NOBIO, PCFLAG_NODESC, PCFLAG_NOSUMMON, PCFLAG_PAGERON, PCFLAG_NOTITLE,
   PCFLAG_GROUPWHO, PCFLAG_GROUPSPLIT, PCFLAG_HELPSTART,
   PCFLAG_AUTOFLAGS, PCFLAG_SECTORD, PCFLAG_ANAME, PCFLAG_NOBEEP, PCFLAG_PASSDOOR,
   PCFLAG_PRIVACY, PCFLAG_NOTELL, PCFLAG_CHECKBOARD, PCFLAG_NOQUOTE,
   PCFLAG_AUTOASSIST, PCFLAG_SHOVEDRAG, PCFLAG_AUTOEXIT, PCFLAG_AUTOLOOT,
   PCFLAG_AUTOSAC, PCFLAG_BLANK, PCFLAG_BRIEF, PCFLAG_AUTOMAP,
   PCFLAG_TELNET_GA, PCFLAG_HOLYLIGHT, PCFLAG_WIZINVIS, PCFLAG_ROOMVNUM, PCFLAG_SILENCE,
   PCFLAG_NO_EMOTE, PCFLAG_BOARDED, PCFLAG_NO_TELL, PCFLAG_LOG, PCFLAG_DENY, PCFLAG_FREEZE,
   PCFLAG_EXEMPT, PCFLAG_ONSHIP, PCFLAG_LITTERBUG, PCFLAG_ANSI, PCFLAG_MSP, PCFLAG_AUTOGOLD,
   PCFLAG_GHOST, PCFLAG_AFK, PCFLAG_NO_URL, PCFLAG_NO_EMAIL, PCFLAG_SMARTSAC,
   PCFLAG_IDLING, PCFLAG_ONMAP, PCFLAG_MAPEDIT, PCFLAG_GUILDSPLIT, PCFLAG_COMPASS,
   PCFLAG_RETIRED, PCFLAG_NO_BEEP, MAX_PCFLAG
};

/*
 * Required data files used by the server.
 *
 * AREA_LIST contains a list of areas to boot.
 * All files are read in completely at bootup.
 * Most output files (bug, idea, typo, shutdown) are append-only.
 */
#define AREA_CONVERT_DIR "../areaconvert/"   // Directory for manually converting areas.
#define MAP_DIR          "../maps/"          // Overland maps
#define PLAYER_DIR       "../player/"        // Player files
#define GOD_DIR          "../gods/"          // God Info Dir
#define BACKUP_DIR       "../backups"        // Backup folder for pfiles when the pfile pruning is active.
#define BUILD_DIR        "../building/"      // Online building save dir
#define SYSTEM_DIR       "../system/"        // Main system files
#define PROG_DIR         "../mudprogs/"      // External MUDProg files
#define CORPSE_DIR       "../corpses/"       // Player corpses
#define CLASS_DIR        "../classes/"       // Classes
#define RACE_DIR         "../races/"         // Races
#define DEITY_DIR        "../deity/"         // Deities
#define MOTD_DIR         "../motd/"          // Where the message of the day files are stored.
#define HOTBOOT_DIR      "../hotboot/"       // For storing objects across hotboots.
#define AUC_DIR          "../aucvaults/"     // Where auction sales data is stored.
#define BOARD_DIR        "../boards/"        // Board directory.
#define COLOR_DIR        "../color/"         // Custom color theme files.
#define WEB_DIR          "../web/"           // HTML static files.
#define EXE_FILE         "../src/afkmud"     // The binary file for the game. Used when executing a hotboot.

#define HOTBOOT_FILE     SYSTEM_DIR "copyover.dat"  // Stores descriptor information for hotboos.
#define MOB_FILE         "mobs.dat"                 // For storing mobs across hotboots.
#define AREA_LIST        "area.lst"                 // List of areas.
#define CLASS_LIST       "class.lst"                // List of classes.
#define RACE_LIST        "race.lst"                 // List of races.
#define SHUTDOWN_FILE    "shutdown.txt"             // For 'shutdown'. Logs the reason why it happened (hopefully).
#define IMM_HOST_FILE    SYSTEM_DIR "immortal.host" // For stopping hackers. Or something.
#define ANSITITLE_FILE   SYSTEM_DIR "mudtitle.ans"  // The ANSI screen shown to users logging in.
#define BOOTLOG_FILE     SYSTEM_DIR "boot.txt"      // Boot up error file.
#define PBUG_FILE        SYSTEM_DIR "pbugs.txt"     // For 'bug' command.
#define IDEA_FILE        SYSTEM_DIR "ideas.txt"     // For 'idea' command.
#define TYPO_FILE        SYSTEM_DIR "typos.txt"     // For 'typo' command.
#define FIXED_FILE       SYSTEM_DIR "fixed.txt"     // For 'fixed' command.
#define LOG_FILE         SYSTEM_DIR "log.txt"       // For talking in logged rooms.
#define MOBLOG_FILE      SYSTEM_DIR "moblog.txt"    // For mplog messages.
#define WIZLIST_FILE     SYSTEM_DIR "WIZLIST"       // Wizlist - Used with 'who' command, and 'wizlist' command.
#define SKILL_FILE       SYSTEM_DIR "skills.dat"    // Skill table. Holds all the data on every skill in the game.
#define HERB_FILE        SYSTEM_DIR "herbs.dat"     // Herb table.
#define TONGUE_FILE      SYSTEM_DIR "tongues.dat"   // Tongue tables.
#define SOCIAL_FILE      SYSTEM_DIR "socials.dat"   // Socials
#define COMMAND_FILE     SYSTEM_DIR "commands.dat"  // Commands
#define WEBWHO_FILE      WEB_DIR "WEBWHO"           // HTML Who output file.
#define WEBWIZ_FILE      WEB_DIR "WEBWIZ"           // HTML wizlist file.
#define AREALIST_FILE    WEB_DIR "AREALIST"         // HTML areas listing.
#define MOTD_FILE        MOTD_DIR "motd.dat"        // Message of the day. Seen by all players when logging in.
#define IMOTD_FILE       MOTD_DIR "imotd.dat"       // Message of the day for immortals when logging in.
#define SPEC_MOTD        MOTD_DIR "specmotd.dat"    // Special MOTD - cannot be ignored on login by anyone.
#define LOGIN_MSG       "login.msg"                 // List of login msgs,

// This damn thing is used in so many places it was about time to just move it here - Samson 10-4-03
#define KEY( literal, field, value ) \
if( !str_cmp( word, (literal) ) )    \
{                                    \
   (field) = (value);                \
   break;                            \
}

// This reads in a value and uses the class function to assign the value to the field - Samson 3-1-05
#define CLKEY( literal, value )   \
if( !str_cmp( word, (literal) ) ) \
{                                 \
   (value);                       \
   break;                         \
}

// This reads a string value into a C++ string variable using the tilde as a delimiter - Samson 10-3-04
#define STDSKEY( literal, field )      \
if( !strcasecmp( word, (literal) ) )   \
{                                      \
   (field).clear();                    \
   fread_string( (field), fp );        \
   break;                              \
}

// This reads a string value into a C++ string variable using line-feed as a delimiter - Samson 10-3-04
#define STDSLINE( literal, field )     \
if( !strcasecmp( word, (literal) ) )   \
{                                      \
   (field).clear();                    \
   fread_line( (field), fp );          \
   break;                              \
}

/*
 * Old-style Bit manipulation macros
 *
 * The bit passed is the actual value of the bit (Use the BV## defines)
 *
 * Take care *NOT* to use these with std::bitset types! You'll get unpredictable results!
 */
#define IS_SET(flag, bit)	((flag) & (bit))
#define SET_BIT(var, bit)	((var) |= (bit))
#define REMOVE_BIT(var, bit)	((var) &= ~(bit))
#define TOGGLE_BIT(var, bit)	((var) ^= (bit))

#define IS_SAVE_FLAG(bit)  sysdata->save_flags.test((bit))

#define IS_EXIT_FLAG(var, bit)     (var)->flags.test((bit))
#define SET_EXIT_FLAG(var, bit)    (var)->flags.set((bit))
#define REMOVE_EXIT_FLAG(var, bit) (var)->flags.reset((bit))

// Memory allocation macros.
#define CREATE(result, type, number)                                    \
do                                                                      \
{                                                                       \
   if (!((result) = (type *) calloc ((number), sizeof(type))))          \
   {                                                                    \
      perror("calloc failure");                                         \
      fprintf(stderr, "Calloc failure @ %s:%d\n", __FILE__, __LINE__ ); \
      abort();                                                          \
   }                                                                    \
} while(0)

#define RECREATE(result,type,number)                                    \
do                                                                      \
{                                                                       \
   if(!((result) = (type *)realloc((result), sizeof(type) * (number)))) \
   {                                                                    \
      log_printf( "Realloc failure @ %s:%d\n", __FILE__, __LINE__ );    \
      abort();                                                          \
   }                                                                    \
} while(0)

// WARNING! This macro should only be used on things explicitly handled by CREATE or RECREATE!
#define OLD_DISPOSE(point)                  \
do                                          \
{                                           \
   if( (point) )                            \
   {                                        \
      free( (point) );                      \
      (point) = nullptr;                    \
   }                                        \
} while(0)

#if defined(__FreeBSD__)
#define DISPOSE(point)                      \
do                                          \
{                                           \
   if( (point) )                            \
   {                                        \
      free( (point) );                      \
      (point) = nullptr;                    \
   }                                        \
} while(0)
#else
#define DISPOSE(point)                         \
do                                             \
{                                              \
   if( (point) )                               \
   {                                           \
      if( typeid((point)) == typeid(char*) || typeid((point)) == typeid(const char*) ) \
      {                                        \
         if( in_hash_table( (point) ) )        \
         {                                     \
            log_printf( "&RDISPOSE called on STRALLOC pointer: %s, line %d\n", __FILE__, __LINE__ ); \
            log_string( "Attempting to correct." ); \
            if( str_free( (point) ) == -1 )    \
               log_printf( "&RSTRFREEing bad pointer: %s, line %d\n", __FILE__, __LINE__ ); \
         }                                     \
         else                                  \
            free( (point) );                   \
      }                                        \
      else                                     \
         free( (point) );                      \
      (point) = nullptr;                       \
   }                                           \
   else                                        \
      (point) = nullptr;                       \
} while(0)
#endif

#define STRALLOC(point)		str_alloc((point), __func__, __FILE__, __LINE__)
#define QUICKLINK(point)	quick_link((point))
#if defined(__FreeBSD__)
#define STRFREE(point)                          \
do                                              \
{                                               \
   if((point))                                  \
   {                                            \
      if( str_free((point)) == -1 )             \
         bug( "&RSTRFREEing bad pointer: %s, line %d", __FILE__, __LINE__ ); \
      (point) = nullptr;                        \
   }                                            \
} while(0)
#else
#define STRFREE(point)                           \
do                                               \
{                                                \
   if((point))                                   \
   {                                             \
      if( !in_hash_table( (point) ) )            \
      {                                          \
         log_printf( "&RSTRFREE called on strdup pointer: %s, line %d\n", __FILE__, __LINE__ ); \
         log_string( "Attempting to correct." ); \
         free( (point) );                        \
      }                                          \
      else if( str_free( (point) ) == -1 )       \
         log_printf( "&RSTRFREEing bad pointer: %s, line %d\n", __FILE__, __LINE__ ); \
      (point) = nullptr;                         \
   }                                             \
   else                                          \
      (point) = nullptr;                         \
} while(0)
#endif

#define INDOOR_SECTOR(sect) ( (sect) == SECT_INDOORS ||     \
                              (sect) == SECT_UNDERWATER ||  \
                              (sect) == SECT_OCEANFLOOR ||  \
                              (sect) == SECT_UNDERGROUND )

#define IS_VALID_SN(sn)		( (sn) >=0 && (sn) < MAX_SKILL && skill_table[(sn)] && skill_table[(sn)]->name )
#define IS_VALID_HERB(sn)	( (sn) >=0 && (sn) < MAX_HERB	&& herb_table[(sn)] && herb_table[(sn)]->name )
#define IS_VALID_DISEASE(sn)	( (sn) >=0 && (sn) < MAX_DISEASE && disease_table[(sn)] && disease_table[(sn)]->name )

// Description macro.
#define PERS(ch, looker, from) ( (looker)->can_see( (ch), (from) ) ? \
      ( (ch)->isnpc() ? (ch)->short_descr : (ch)->name ) : "Someone" )

// These 2 macros are used for case changes on a particular character somewhere in a string.
#define LOWER( c )        ( (c) >= 'A' && (c) <= 'Z' ? (c) + 'a' - 'A' : (c) )
#define UPPER( c )        ( (c) >= 'a' && (c) <= 'z' ? (c) + 'A' - 'a' : (c) )

// Safe fclose macro adopted from DOTD Codebase.
#define FCLOSE(fp) fclose((fp)); (fp)=nullptr;

// These 3 functions replace the UMIN, UMAX, and URANGE macros. Must be declared here because the headers use them.
int umin( int, int );
int umax( int, int );
int urange( int, int, int );

/*
 * Character data is used in pretty much everything. May as well globally include it here.
 * This includes the pc_data information as well.
 */
#include "character.h"

// Object data is also used all over the place.
#include "object.h"

// object saving defines for fread/write_obj. -- Altrag
enum carry_types
{
   OS_CARRY, OS_CORPSE
};

/*
 * Damage types from the attack_table[]
 */
// modified for new weapon_types - Grimm
// Trimmed down to reduce duplicated types - Samson 1-9-00
enum damage_types
{
   DAM_HIT, DAM_SLASH, DAM_STAB, DAM_HACK, DAM_CRUSH, DAM_LASH,
   DAM_PIERCE, DAM_THRUST, DAM_MAX_TYPE
};

/*
 * Used to keep track of system settings and statistics - Thoric
 * Calculated values are derived when editing settings in do_cset - Samson
 */
class system_data
{
 private:
   system_data( const system_data & p );
     system_data & operator=( const system_data & );

 public:
     system_data(  );
    ~system_data(  );

   void *dlHandle;                               // libdl System Handle - Trax
   std::bitset<SV_MAX> save_flags;               // Toggles for saving conditions.
   std::string time_of_max;                      // Time of max ever.
   std::string mud_name;                         // Name of mud.
   std::string admin_email;                      // Email address for admin. Not set by default. - Samson 10-17-98
   std::string password;                         // Port access code. NOTE: THIS IS NOT ENCRYPTED!!!!
   std::string telnet;                           // Store telnet address for who/webwho.
   std::string http;                             // Store web address for who/webwho.
   std::string dbserver;                         // Database server address for SQL support, usually localhost.
   std::string dbname;                           // Database name for SQL support.
   std::string dbuser;                           // Database username for SQL support.
   std::string dbpass;                           // Database password for SQL support. NOTE: THIS IS NOT ENCRYPTED!!!!
   std::chrono::system_clock::time_point motd;   // Last time MOTD was edited.
   std::chrono::system_clock::time_point imotd;  // Last time IMOTD was edited.
   std::chrono::minutes save_frequency;          // How often to autosave someone. Default 20 minutes.
   size_t maxign;                                // Maximum number of ignores a player is allowed to have. Default 10.
   size_t maxholiday;                            // Maximum number of holidays allowed on the calendar. Default 30.
   int maxplayers;                               // Maximum players this boot. Updated automatically.
   int alltimemax;                               // Maximum players ever. Updated automatically.
   int auctionseconds;                           // Seconds between auction events. Default 15.
   int maxvnum;                                  // Defines the currently allowed highest vnum for mobs, objs, and rooms. Default 100000.
   int minguildlevel;                            // Defined, but apparently not actually used outside of being able to change it with the cset command. Default 10.
   int maxcondval;                               // Maximum value a condition on a player can have. Default 100.
   int maximpact;                                // Base level of resistance an object has to being damaged. Default 30.
   int initcond;                                 // The initial condition of a weapon when loaded or created. Default 12.
   int minego;                                   // The minimum ego value an object can have. Value is used to determine if a player can use it based on their own ego calculation. Default 25.
   int secpertick;                               // Number of seconds per game tick. Default 70.
   int pulsepersec;                              // How often the game runs its main loop. Used in pulse_sync in comm.cpp. Default 4. NOTE: Altering this value will have a wide ranging impact on how the MUD behaves. Be VERY careful with this. [Previously hardcoded as PULSE_PER_SECOND]
   int pulsetick;                                // Used in the timing of name authorizations and the main character and object updates in update.cpp. This is a calculated value: pulsetick = secpertick * pulsepersec.
   int pulseviolence;                            // Used in cooldowns for skill use in combat. This is a calculated value: 3 * pulsepersec.
   int pulsemobile;                              // Used for timing how often the mobile_update function runs in update.cpp. This is a calculated value: 4 * pulsepersec.
   int pulsecalendar;                            // Used to time how often the in-game world time updates, along with how often effects like hunger and thirst update, plus the global weather system. This is a calculated value: 4 * pulsetick. This timer does not update when nobody is logged on.
   int pulseenvironment;                         // Used to time how often major events on the overland maps take place. This is a calculated value: 15 * sysdata->pulsepersec. This timer does not update when nobody is logged on.
   int hoursperday;                              // Number of hours in an in-game day. Default 28.
   int daysperweek;                              // Number of days in an in-game week. Default 13.
   int dayspermonth;                             // Number of days in an in-game month. Default 26.
   int monthsperyear;                            // Number of months in an in-game year. Default 12.
   int daysperyear;                              // Number of days in an in-game year. This is a calculated value: dayspermonth * monthsperyear.
   int hoursunrise;                              // The hour when the sun rises. This is a calculated value: hoursunrise = hoursperday / 4.
   int hourdaybegin;                             // The hour when daylight begins. This is a calculated value: hourdaybegin = hoursunrise + 1.
   int hournoon;                                 // The hour when the sun is highest in the sky. This is a calculated value: hournoon = hoursperday / 2.
   int hoursunset;                               // The hour when the sun sets. This is a calculated value: hoursunset = ( ( hoursperday / 4 ) * 3 ).
   int hournightbegin;                           // The hour when nighttime begins. This is a calculated value: hournightbegin = hoursunset + 1.
   int hourmidnight;                             // The hour when midnight occurs. Equal to the hoursperday value.
   int rebootcount;                              // How many minutes to count down for a reboot. Default 5. - Samson 4-22-03
   int gameloopalarm;                            // Number of seconds before game_loop() triggers an alarm due to being hung up. Default 30. - Samson 1-24-05
   short webwho;                                 // Number of seconds between webwho refreshes, 0 for no refresh. Default 0. - Samson 5-13-06
   short read_all_mail;                          // Read all player mail. Default LEVEL_SUPREME. Defined in mudcfg.h.
   short read_mail_free;                         // Read mail for free. Default LEVEL_IMMORTAL. Defined in mudcfg.h.
   short write_mail_free;                        // Write mail for free. Default 2.
   short take_others_mail;                       // Take others mail. Default LEVEL_SUPREME. Defined in mudcfg.h.
   short build_level;                            // Level of build channel. Default LEVEL_DEMI. Defined in mudcfg.h.
   short level_modify_proto;                     // Level to modify prototype stuff. Default LEVEL_LESSER. Defined in mudcfg.h.
   short level_override_private;                 // Override private flag in rooms. Default LEVEL_GOD. Defined in mudcfg.h.
   short level_mset_player;                      // Level to mset a player. Default LEVEL_ASCENDANT. Defined in mudcfg.h.
   short level_getobjnotake;                     // Level at which an immortal can pick up objects without a take flag. Default LEVEL_GREATER. Defined in mudcfg.h.
   short level_forcepc;                          // The level at which an immortal can use the force command on players. Default LEVEL_ASCENDANT. Defined in mudcfg.h.
   short bestow_dif;                             // Max # of levels between trust and command level for a bestow to work. Default 5. --Blodkai
   short gouge_plr_vs_plr;                       // Gouge mod player vs. player. Bonus chance of being able to gouge another player in combat. Default 0.
   short gouge_nontank;                          // Gouge mod player != primary attacker. An additional bonus to gouging if the player is not the tank in a fight. Default 0.
   short stun_regular;                           // Base difficulty modifier to be able to stun an NPC in combat. Default 15.
   short stun_plr_vs_plr;                        // Stun mod player vs. player. Additional difficulty modifier for stunning another player in combat. Default 65.
   short dodge_mod;                              // Difficulty modifier for the chance to dodge an attack from another player. Divides the chances calculation by the value, thus making it more difficult. Default 2.
   short parry_mod;                              // Difficulty modifier for the chance to parry an attack from another player. Divides the chances calculation by the value, thus making it more difficult. Default 2.
   short tumble_mod;                             // Difficulty modifier for the chance to tumble away from an attack by another player. Divides the chances calculation by the value, thus making it more difficult. Default 4.
   short dam_plr_vs_plr;                         // Damage modifier in player vs. player combat. Value is a percentage. Default 100. [IE: 125% multiplies damage by 1.25, 25% multiplies it by 0.25 etc.]
   short dam_plr_vs_mob;                         // Damage modifier in player vs. mobile combat. Value is a percentage. Default 100. [IE: 125% multiplies damage by 1.25, 25% multiplies it by 0.25 etc.]
   short dam_mob_vs_plr;                         // Damage modifier in mobile vs. player combat. Value is a percentage. Default 100. [IE: 125% multiplies damage by 1.25, 25% multiplies it by 0.25 etc.]
   short dam_mob_vs_mob;                         // Damage modifier in mobile vs. mobile combat. Value is a percentage. Default 100. [IE: 125% multiplies damage by 1.25, 25% multiplies it by 0.25 etc.]
   short newbie_purge;                           // Number of days before newbies get purged during pfile cleanups. Default 30. A newbie is anyone under level 10. - Samson 12-27-98
   short regular_purge;                          // Number of days before regular players get purged during pfile cleanups. Default 90. - Samson 12-27-98
   short mapsize;                                // Laziness feature mostly. Changes the overland map visibility radius.
   short playersonline;                          // Number of players currently online. This starts at 0 and is tracked throughout the current booted session. Does not save.
   bool NO_NAME_RESOLVING;                       // Whether or not to do DNS resolution on a user's IP address. Default to no DNS resolution.
   bool DENY_NEW_PLAYERS;                        // New players cannot connect. Default to allowing new players. Does not save across reboots.
   bool WAIT_FOR_AUTH;                           // New players must have their names authorized. Default on.
   bool check_imm_host;                          // Do we check immortal's hosts? Default on.
   bool save_pets;                               // Do player's pets' save? Default on.
   bool WIZLOCK;                                 // Is the game wizlocked? Default off. - Samson 8-2-98
   bool IMPLOCK;                                 // Is the game implocked? Default off - Samson 8-2-98
   bool LOCKDOWN;                                // Is the game locked down? Default off. - Samson 8-23-98
   bool CLEANPFILES;                             // Should the mud clean up pfiles daily? Default off. - Samson 12-27-98
   bool TESTINGMODE;                             // Blocks file copies to main port when active. Default off. - Samson 1-31-99
   bool crashhandler;                            // Do we intercept SIGSEGV? Default off. - Samson 3-11-04
};

/*
 * Types of skill numbers.  Used to keep separate lists of sn's
 * Must be non-overlapping with spell/skill types,
 * but may be arbitrary beyond that.
 */
const int TYPE_UNDEFINED = -1;
const int TYPE_HIT = 1000;       /* allows for 1000 skills/spells */
const int TYPE_HERB = 2000;      /* allows for 1000 attack types  */
const int TYPE_PERSONAL = 3000;  /* allows for 1000 herb types    */
const int TYPE_RACIAL = 4000;    /* allows for 1000 personal types */
const int TYPE_DISEASE = 5000;   /* allows for 1000 racial types  */


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
   char *msg_off; /* Wear off message     */
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

// So we can have different configs for different ports -- Shaddai
extern int mud_port;

// Global constants.
extern const struct str_app_type str_app[26];
extern const struct int_app_type int_app[26];
extern const struct wis_app_type wis_app[26];
extern const struct dex_app_type dex_app[26];
extern const struct con_app_type con_app[26];
extern const struct cha_app_type cha_app[26];
extern const struct lck_app_type lck_app[26];

extern const char *attack_table[DAM_MAX_TYPE];
extern const char *attack_table_plural[DAM_MAX_TYPE];

extern const char **s_message_table[DAM_MAX_TYPE];
extern const char **p_message_table[DAM_MAX_TYPE];

extern const char *skill_tname[];
extern const char *dir_name[];
extern const char *short_dirname[];
extern const char *where_names[MAX_WHERE_NAME];
extern const short rev_dir[];
extern const int trap_door[];
extern const char *save_flag[];
extern const char *r_flags[];
extern const char *ex_flags[];
extern const char *w_flags[];
extern const char *o_flags[];
extern const char *aff_flags[];
extern const char *o_types[];
extern const char *a_types[];
extern const char *act_flags[];
extern const char *pc_flags[];
extern const char *trap_types[];
extern const char *trap_flags[];
extern const char *ris_flags[];
extern const char *trig_flags[];
extern const char *part_flags[];
extern const char *npc_race[];
extern const char *npc_class[];
extern const char *defense_flags[];
extern const char *attack_flags[];
extern const char *area_flags[];
extern const char *ex_pmisc[];
extern const char *ex_pwater[];
extern const char *ex_pair[];
extern const char *ex_pearth[];
extern const char *ex_pfire[];
extern const char *wear_locs[];
extern const char *lang_names[];
extern const char *temp_settings[]; /* FB */
extern const char *precip_settings[];
extern const char *wind_settings[];
extern const char *preciptemp_msg[6][6];
extern const char *windtemp_msg[6][6];
extern const char *precip_msg[];
extern const char *wind_msg[];
extern const char *const login_msg[];

/*
 * Global variables.
 * Stuff for area versions --Shaddai
 */
const int HAS_SPELL_INDEX = -1;
extern bool DONT_UPPER; // This is to tell if act uses uppercasestring or not --Shaddai
extern bool MOBtrigger;
extern bool MPSilent;
extern bool mud_down;
extern bool DONTSAVE;

extern const unsigned char echo_off_str[];
extern int top_area;
extern int top_mob_index;
extern int top_obj_index;
extern int top_room;
extern std::string str_boot_time;
extern char_data *timechar;
extern bool fBootDb;
extern char strArea[MIL];
extern int falling;

extern std::string target_name;
extern std::string ranged_target_name;
extern int numobjsloaded;
extern int nummobsloaded;
extern int physicalobjects;
extern int last_pkroom;
extern int num_descriptors;
extern system_data *sysdata;
extern int top_herb;
extern int top_disease;
extern ch_ret global_retcode;

extern char *title_table[MAX_CLASS][MAX_LEVEL + 1][SEX_MAX];
extern skill_type *skill_table[MAX_SKILL];
extern skill_type *herb_table[MAX_HERB];
extern skill_type *disease_table[MAX_DISEASE];
extern std::chrono::system_clock::time_point current_time;
extern bool fLogAll;
extern time_info_data time_info;

extern int top_ed;

/*
 * Our function prototypes.
 * One big lump ... this is every function in Merc.
 */

/* Actually.... it's not. If it were, then there would be ALOT more here.
 * As it stands, some of this didn't need to be globally aware, so there's even less.
 */

// Formatted log output - 3-07-02
void log_printf( const char *, ... ) __attribute__ ( ( format( printf, 1, 2 ) ) );

// act_comm.cpp
bool is_same_group( char_data *, char_data * );
int knows_language( char_data *, int, char_data * );
void act( short, const std::string &, char_data *, const void *, const void *, int );
void act_printf( short, char_data *, const void *, const void *, int, const char *, ... ) __attribute__ ( ( format( printf, 6, 7 ) ) );

// act_info.cpp
bool is_ignoring( char_data *, char_data * );

// act_move.cpp
exit_data *find_door( char_data *, const std::string &, bool );
ch_ret move_char( char_data *, exit_data *, int, int, bool );

// act_wiz.cpp
char_data *get_wizvictim( char_data *, const std::string &, bool );
void echo_to_all( const std::string &, short );
void echo_all_printf( short, const char *, ... ) __attribute__ ( ( format( printf, 2, 3 ) ) );

// build.cpp
int get_risflag( const std::string & );
int get_defenseflag( const std::string & );
int get_attackflag( const std::string & );
int get_npc_sex( const std::string & );
int get_npc_position( const std::string & );
int get_npc_class( const std::string & );
int get_npc_race( const std::string & );
int get_pc_race( const std::string & );
int get_pc_class( const std::string & );
int get_actflag( const std::string & );
int get_pcflag( const std::string & );
int get_langnum( const std::string & );
int get_langflag( const std::string & );
int get_rflag( const std::string & );
int get_exflag( const std::string & );
int get_sectypes( const std::string & );
int get_areaflag( const std::string & );
int get_partflag( const std::string & );
int get_magflag( const std::string & );
int get_otype( const std::string & );
int get_aflag( const std::string & );
int get_atype( const std::string & );
int get_oflag( const std::string & );
int get_wflag( const std::string & );
int get_saveflag( const std::string & );
int get_logflag( const std::string & );
int get_trapflag( const std::string & );
int get_flag( const std::string &, const char **, size_t );
int get_dir( const std::string & );
char *flag_string( int, const char *flagarray[] );

// calendar.cpp
const std::string c_time( std::chrono::system_clock::time_point, int );
const std::string mini_c_time( std::chrono::system_clock::time_point, int );

// comm.cpp
bool check_parse_name( const std::string &, bool );
void add_loginmsg( const char *, short, const char * );

// commands.cpp
int check_command_level( const std::string &, int );
void cmdf( char_data *, const char *, ... ) __attribute__ ( ( format( printf, 2, 3 ) ) );
void funcf( char_data *, DO_FUN *, const char *, ... ) __attribute__ ( ( format( printf, 3, 4 ) ) );
void interpret( char_data *, std::string );
void check_switches(  );
void check_switch( char_data * );

// db.cpp
extern std::mt19937 global_rng; // Mersenne Twister algorithm for random numbers. Produces better results than the rand() and srand() functions.
bool is_valid_filename( char_data *, const std::string &, const std::string & );
void shutdown_mud( const std::string & );
bool exists_file( const std::string & );
char fread_letter( FILE * );
char *fread_string( FILE * );
const char *fread_flagstring( FILE * );
char *fread_string_nohash( FILE * );
void fread_to_eol( FILE * );
const char *fread_line( FILE * );
char *fread_word( FILE * );
void fread_string( std::string &, FILE * );
void fread_line( std::string &, FILE * );
void log_printf_plus( short, short, const char *, ... ) __attribute__ ( ( format( printf, 3, 4 ) ) );
int number_percent( void );
int number_fuzzy( int );
int number_range( int, int );
int number_door( void );
int number_bits( int );
int dice( int, int );
void append_file( char_data *, const std::string &, const char *, ... ) __attribute__ ( ( format( printf, 3, 4 ) ) );
void append_to_file( const std::string &, const char *, ... ) __attribute__ ( ( format( printf, 2, 3 ) ) );
void log_string_plus( short, short, std::string_view );
void log_string( std::string_view );
void make_wizlist( void );

// descriptor.cpp
void show_file( char_data *, const std::string & );

// editor.cpp
bool hasname( std::string_view, std::string_view );
void removename( std::string &, const std::string & );
void addname( std::string &, const std::string & );
void stralloc_printf( char **, const char *, ... ) __attribute__ ( ( format( printf, 2, 3 ) ) );
void strdup_printf( char **, const char *, ... ) __attribute__ ( ( format( printf, 2, 3 ) ) );
void smash_tilde( char * );
void smash_tilde( std::string & );
void hide_tilde( std::string & );
const char *show_tilde( const char * );
const std::string show_tilde( const std::string & );
int str_cmp( std::string_view, std::string_view );
bool str_cmp( const char *, const char * );
bool str_prefix( std::string_view, std::string_view );
bool str_prefix( const char *, const char * );
bool str_infix( const char *, const char * );
bool str_infix( std::string_view, std::string_view );
bool str_suffix( const char *, const char * );
char *capitalize( const char * );
std::string capitalize( const std::string & );
char *strlower( const char * );
void strlower( std::string & );
char *strupper( const char * );
void strupper( std::string & );
const char *aoran( const std::string & );
void strip_tilde( std::string & );
void strip_lspace( std::string & );
void strip_tspace( std::string & );
void strip_spaces( std::string & );
const char *strip_cr( const char * );
std::string strip_cr( const std::string & );
const char *strip_crlf( const char * );
std::string strip_crlf( std::string_view );
void strip_whitespace( std::string & );
bool is_number( const std::string & );
bool is_number( const char * );
int number_argument( std::string_view, std::string & );
int number_argument( char *, char * );
const char *one_argument( const char *, char * );
char *one_argument( char *, char * );
std::string one_argument( const std::string &, std::string & );
std::string invert_string( const std::string & );
// const std::string add_percent( std::string );
const std::string escape_formatting( std::string );
void string_erase( std::string &, char );
void string_erase( std::string &, std::string_view );
void string_replace( std::string &, std::string_view, std::string_view );
std::vector<std::string> string_explode( std::string_view, char );
const char *print_array_string( const char *flagarray[], size_t );

// fight.cpp
ch_ret multi_hit( char_data *, char_data *, int );
ch_ret damage( char_data *, char_data *, double, int );

// handler.cpp
long exp_level( int );
bool nifty_is_name_prefix( std::string, std::string );
bool nifty_is_name_prefix( char *, char * );
bool is_name2_prefix( std::string_view, std::string );
bool is_name2_prefix( const char *, char * );
obj_data *get_obj_list( char_data *, const std::string &, std::list<obj_data *> );
ch_ret check_for_trap( char_data *, obj_data *, int );
ch_ret spring_trap( char_data *, obj_data * );
obj_data *find_obj( char_data *, std::string, bool );
bool ms_find_obj( char_data * );

// hashstr.cpp
char *str_alloc( const char *, const char *, const char *, int );
char *quick_link( char * );
int str_free( const char * );
void show_hash( int );
void hash_dump( int );
void show_high_hash( int );
bool in_hash_table( const char * );

// magic.cpp
bool process_spell_components( char_data *, int );
bool saves_poison_death( int, char_data * );
bool saves_para_petri( int, char_data * );
bool saves_breath( int, char_data * );
bool saves_spell_staff( int, char_data * );
ch_ret obj_cast_spell( int, int, char_data *, char_data *, obj_data * );
int dice_parse( char_data *, int, const std::string & );
skill_type *get_skilltype( int );

// mud_comm.cpp
const std::string mprog_type_to_name( int );

// mud_prog.cpp
void mprog_entry_trigger( char_data * );
void mprog_greet_trigger( char_data * );
void progbug( const std::string &, char_data * );
void progbugf( char_data *, const char *, ... ) __attribute__ ( ( format( printf, 2, 3 ) ) );
void rset_supermob( room_index * );
void release_supermob( void );

// overland.cpp
bool is_same_char_map( char_data *, char_data * );
bool is_same_obj_map( char_data *, obj_data * );
void fix_maps( char_data *, char_data * );
void enter_map( char_data *, exit_data *, int, int, const std::string & );
void leave_map( char_data *, char_data *, room_index * );
void collect_followers( char_data *, room_index *, room_index * );

// player.cpp
const std::string condtxt( int, int );
const std::string attribtext( int );
bool exists_player( const std::string & );

// save.cpp
void write_corpse( obj_data *, const std::string & );

// skills.cpp
void disarm( char_data *, char_data * );
int find_spell( char_data *, const std::string &, bool );
int find_skill( char_data *, const std::string &, bool );
int find_ability( char_data *, const std::string &, bool );
int find_combat( char_data *, const std::string &, bool );
int find_tongue( char_data *, const std::string &, bool );
int find_lore( char_data *, const std::string &, bool );
int skill_lookup( const std::string & );
int herb_lookup( const std::string & );
int slot_lookup( int );
CMDF( skill_notfound );

// tables.cpp
SPELL_FUN *spell_function( const std::string & );
DO_FUN *skill_function( const std::string & );
SPEC_FUN *m_spec_lookup( const std::string & );

// update.cpp
void gain_condition( char_data *, int, int );
void update_handler( void );
void weather_update( void );

// This used to be the old ext_flagstring converted to C++ and using strings so it can't overflow the temporary buffer.
template < size_t N > const char *bitset_string( std::bitset < N > bits, const char *flagarray[] )
{
   static std::string s;

   s.clear();

   for( size_t i = 0; i < bits.size (); ++i )
   {
      if( bits[i] )
      {
         s.append( flagarray[i] );
         s.append( 1, ' ' );
      }
   }
   strip_tspace(s); // get rid of final space

   return s.c_str();
}

// This template is used during file reading to set flags based on the string names.
// Loosely resembles Remcon's WEXTKEY macro from his LoP codebase.
template < size_t N > void flag_set( FILE * fp, std::bitset < N > &field, const char *flagarray[] )
{
   std::string flags, flag;

   flags = fread_flagstring( fp );
   while( flags[0] != '\0' )
   {
      flags = one_argument( flags, flag );
      int value = get_flag( flag, flagarray, N );

      // Casting N down to an int might not look good, but we can't check for -1 any other way.
      if( value < 0 || value >= ( int )N )
         bug( "%s: Invalid flag: %s", __func__, flag.c_str() );
      else
         field.set( value );
   }
}

// Just like the above, only doesn't read from a file. Just sets the flags from the specified string.
template < size_t N > void flag_string_set( std::string & original, std::bitset < N > &field, const char *flagarray[] )
{
   std::string flag;

   while( !original.empty(  ) )
   {
      original = one_argument( original, flag );
      int value = get_flag( flag, flagarray, N );

      // Casting N down to an int might not look good, but we can't check for -1 any other way.
      if( value < 0 || value >= ( int )N )
         bug( "%s: Invalid flag: %s", __func__, flag.c_str(  ) );
      else
         field.set( value );
   }
}

template < class N > extra_descr_data * get_extra_descr( const std::string & name, N * target )
{
   for( auto* ed : target->extradesc )
   {
      if( is_name2_prefix( name, ed->keyword ) )
      {
         if( !ed->desc.empty(  ) )
            return ed;
      }
   }
   return nullptr;
}

template < class N > extra_descr_data * set_extra_descr( N * target, const std::string & name )
{
   extra_descr_data *desc = nullptr;

   for( auto* ed : target->extradesc )
   {
      if( ed->keyword.find( name ) != std::string::npos )
      {
         desc = ed;
         break;
      }
   }

   if( !desc )
   {
      desc = new extra_descr_data;
      desc->keyword = name;
      target->extradesc.push_back( desc );
      ++top_ed;
   }
   return desc;
}

// Read a number from a file. Used to be several clones in db.cpp for each type. Now there is only one.
template <typename T> T fread_numeric( FILE *fp )
{
   static_assert( std::is_arithmetic<T>::value, "fread_numeric requires an arithmetic type (int, float, etc)." );

   int c;
   do
   {
      c = std::getc( fp );
      if( c == EOF )
         return 0;
   } while( std::isspace(c) );

   bool sign = false;
   if( c == '+' )
   {
      c = std::getc( fp );
   }
   else if( c == '-' )
   {
      sign = true;
      c = std::getc( fp );
   }

   double number = 0.0;

   // Parse integer part
   while( std::isdigit( c ) )
   {
      number = ( number * 10.0 ) + (c - '0' );
      c = std::getc( fp );
   }

   // Parse decimal part (if floating point type requested)
   if constexpr( std::is_floating_point<T>::value )
   {
      if( c == '.' )
      {
         double divisor = 10.0;
         c = std::getc( fp );
         while( std::isdigit( c ) )
         {
            number += (c - '0') / divisor;
            divisor *= 10.0;
            c = std::getc( fp );
         }
      }
   }

   if( sign )
      number = -number;

   // Handle flags (pipe operator) - note: only logical for integer flags
   if( c == '|' )
   {
      number += fread_numeric<double>( fp );
   }
   else if( c != EOF && c != ' ' )
   {
      std::ungetc( c, fp );
   }

   return static_cast<T>( number );
}

// Add any new types as needed here.
inline short fread_short( FILE *fp )  { return fread_numeric<short>( fp ); }
inline int   fread_number( FILE *fp ) { return fread_numeric<int>( fp ); }
inline long  fread_long( FILE *fp )   { return fread_numeric<long>( fp ); }
inline float fread_float( FILE *fp )  { return fread_numeric<float>(fp); }

// Thanks to David Haley for this little trick. A nifty little template that behaves like DISPOSE used to.
template < typename T > void deleteptr( T * &ptr )
{
   delete ptr;
   ptr = nullptr;
}
