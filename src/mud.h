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
 *                          Main mud header file                            *
 ****************************************************************************/

#ifndef __MUD_H__
#define __MUD_H__

#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <vector>
#include <list>
#include <map>
#include <bitset>
#include <typeinfo>

using namespace std;

/* Used in basereport and world commands - don't remove these!
 * If you want to add your own, make a new set of defines and ADD the information.
 * Removing this is a violation of your license agreement.
 */
#define CODENAME "AFKMud"
#define CODEVERSION "2.2.3"
#define COPYRIGHT "Copyright The Alsherok Team 1997-2020. All rights reserved."

/*
 * String and memory management parameters.
 */
const int MSL = 8192;   /* MAX_STRING_LENGTH */
const int MIL = 2048;   /* MAX_INPUT_LENGTH */

/*
 * Short scalar types.
 */
#if !defined(BERR)
#define BERR 255
#endif

typedef int ch_ret;

/*
 * Class declarations
 */
class descriptor_data;
class area_data;
class mob_index;
class obj_index;
class room_index;
class char_data;
class pc_data;
class obj_data;

/* Used in class headers - needs to be declared early */
void init_memory( void *start, void *end, unsigned int size );
void bug( const char *, ... ) __attribute__ ( ( format( printf, 1, 2 ) ) );

/*
 * Function types. Samson. Stop. Don't do it! NO! You have to keep these things you idiot!
 */
typedef void DO_FUN( char_data * ch, string argument );
typedef ch_ret SPELL_FUN( int sn, int level, char_data * ch, void *vo );
typedef bool SPEC_FUN( char_data * ch );

/* "Oh God Samson, this is so HACKISH!" "Yes my son, but this preserves dlsym, which is good."
 * "*The masses bow down in worship to their God....*"
 */
#define CMDF( name ) extern "C" void (name)( char_data *ch, string argument )
#define SPELLF( name ) extern "C" ch_ret (name)( int sn, int level, char_data *ch, void *vo )
#define SPECF( name ) extern "C" bool (name)( char_data *ch )

#define DUR_CONV	93.333333333333333333333333   /* Original value: 23.3333... - Quadrupled for time changes */

/* Hidden tilde char generated with alt155 on the number pad.
 * The code blocks the use of these symbols by default, so this should be quite safe. Samson 3-14-04
 */
#define HIDDEN_TILDE	'\xa2'

/* 32bit bitvector defines */
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
/* 32 USED! DO NOT ADD MORE! SB */

/*
 * Command logging types.
 */
// DON'T FORGET TO ADD THESE TO BUILD.C !!!
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

/* Echo types for echo_to_all */
enum echo_types
{
   ECHOTAR_ALL, ECHOTAR_PC, ECHOTAR_PK, ECHOTAR_IMM
};

/* Global Skill Numbers */
#define ASSIGN_GSN(gsn, skill) \
do                             \
{                              \
   if( ( (gsn) = skill_lookup( (skill) ) ) == -1 ) \
      log_printf( "ASSIGN_GSN: Skill %s not found.\n", (skill) ); \
} while(0)

/*
 * These are skill_lookup return values for common skills and spells.
 */
extern short gsn_style_evasive;
extern short gsn_style_defensive;
extern short gsn_style_standard;
extern short gsn_style_aggressive;
extern short gsn_style_berserk;

extern short gsn_backheel; /* Samson 5-31-00 */
extern short gsn_metallurgy;  /* Samson 5-31-00 */
extern short gsn_scout; /* Samson 5-29-00 */
extern short gsn_scry;  /* Samson 5-29-00 */
extern short gsn_tinker;   /* Samson 4-25-00 */
extern short gsn_deathsong;   /* Samson 4-25-00 */
extern short gsn_swim;  /* Samson 4-24-00 */
extern short gsn_tenacity; /* Samson 4-24-00 */
extern short gsn_bargain;  /* Samson 4-23-00 */
extern short gsn_bladesong;   /* Samson 4-23-00 */
extern short gsn_elvensong;   /* Samson 4-23-00 */
extern short gsn_reverie;  /* Samson 4-23-00 */
extern short gsn_mining;   /* Samson 4-17-00 */
extern short gsn_woodcall; /* Samson 4-17-00 */
extern short gsn_forage;   /* Samson 3-26-00 */
extern short gsn_spy;   /* Samson 6-20-99 */
extern short gsn_charge;   /* Samson 6-07-99 */
extern short gsn_retreat;  /* Samson 5-27-99 */
extern short gsn_detrap;
extern short gsn_backstab;
extern short gsn_circle;
extern short gsn_cook;
extern short gsn_assassinate; /* Samson */
extern short gsn_feign; /* Samson 10-22-98 */
extern short gsn_quiv;  /* Samson 6-02-99 */
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

/* used to do specific lookups */
extern short num_skills;
extern short num_sorted_skills;

/* spells */
extern short gsn_blindness;
extern short gsn_charm_person;
extern short gsn_aqua_breath;
extern short gsn_curse;
extern short gsn_invis;
extern short gsn_mass_invis;
extern short gsn_poison;
extern short gsn_sleep;
extern short gsn_silence;  /* Samson 9-26-98 */
extern short gsn_paralyze; /* Samson 9-26-98 */
extern short gsn_fireball; /* for fireshield  */
extern short gsn_chill_touch; /* for iceshield   */
extern short gsn_lightning_bolt; /* for shockshield */

/* newer attack skills */
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

/* changed to new weapon types - Grimm */
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

/* Language gsns. -- Altrag */
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

/*
 * Extra description data for a room or object.
 */
struct extra_descr_data
{
   string keyword;   /* Keyword in look/examine */
   string desc;   /* What to see */
};

#include "mudcfg.h"  /* Contains definitions specific to your mud - will not be covered by patches. Samson 3-14-04 */
#include "color.h"   /* Custom color stuff */
#include "olc.h"  /* Oasis OLC code and global definitions */

/*
 * Time and weather stuff.
 */
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
   int season; /* Samson 5-6-99 */
};

/* short cut crash bug fix provided by gfinello@mail.karmanet.it*/
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

/*
 * Connected state for a channel.
 */
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

/*
 * Character substates
 */
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
   /*
    * timer types ONLY below this point 
    */
   SUB_TIMER_DO_ABORT = 128, SUB_TIMER_CANT_ABORT
};

/*
 * Attribute bonus structures.
 */
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

/*
 * TO types for act.
 */
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

     bitset < MAX_RIS_FLAG > rismod;
   int bit;
   int duration;
   int modifier;
   short location;
   short type;
};

/*
 * Autosave flags
 */
enum autosave_flags
{
   SV_DEATH, SV_KILL, SV_PASSCHG, SV_DROP, SV_PUT, SV_GIVE, SV_AUTO,
   SV_ZAPDROP, SV_AUCTION, SV_GET, SV_RECEIVE, SV_IDLE, SV_FILL, SV_EMPTY, SV_MAX
};

/*
 * Flags for act_string -- Shaddai
 */
/* There was really no justifiable reason to make this a BV guys. What the hell? */
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

/*
 * Conditions.
 */
enum pc_conditions
{
   COND_DRUNK, COND_FULL, COND_THIRST, MAX_CONDS
};

/*
 * Styles.
 */
enum combat_styles
{
   STYLE_BERSERK, STYLE_AGGRESSIVE, STYLE_FIGHTING, STYLE_DEFENSIVE, STYLE_EVASIVE
};

/* Bits for pc_data->flags */
/* DAMMIT! Don't forget to add these things to build.cpp!! */
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
#define AREA_CONVERT_DIR "../areaconvert/"   /* Directory for manually converting areas */
#define MAP_DIR          "../maps/" /* Overland maps */
#define PLAYER_DIR       "../player/"  /* Player files      */
#define GOD_DIR          "../gods/" /* God Info Dir      */
#define BUILD_DIR        "../building/"   /* Online building save dir */
#define SYSTEM_DIR       "../system/"  /* Main system files */
#define PROG_DIR         "../mudprogs/"   /* MUDProg files     */
#define CORPSE_DIR       "../corpses/" /* Corpses        */
#define CLASS_DIR        "../classes/" /* Classes        */
#define RACE_DIR         "../races/"   /* Races */
#define DEITY_DIR        "../deity/"   /* Deity data dir      */
#define MOTD_DIR         "../motd/"
#define HOTBOOT_DIR      "../hotboot/" /* For storing objects across hotboots */
#define AUC_DIR          "../aucvaults/"
#define BOARD_DIR        "../boards/"  /* Board directory */
#define COLOR_DIR        "../color/"
#define WEB_DIR          "../web/"  /* HTML static files */

#define EXE_FILE "../src/afkmud"
#define HOTBOOT_FILE    SYSTEM_DIR "copyover.dat"  /* for hotboots */
#define MOB_FILE        "mobs.dat"  /* For storing mobs across hotboots */
#define AREA_LIST		"area.lst"  /* List of areas     */
#define CLASS_LIST	"class.lst" /* List of classes   */
#define RACE_LIST		"race.lst"  /* List of races     */
#define SHUTDOWN_FILE	"shutdown.txt" /* For 'shutdown' */
#define IMM_HOST_FILE   SYSTEM_DIR "immortal.host" /* For stoping hackers */
#define ANSITITLE_FILE	SYSTEM_DIR "mudtitle.ans"
#define BOOTLOG_FILE	SYSTEM_DIR "boot.txt"   /* Boot up error file  */
#define PBUG_FILE		SYSTEM_DIR "pbugs.txt"  /* For 'bug' command   */
#define IDEA_FILE		SYSTEM_DIR "ideas.txt"  /* For 'idea'       */
#define TYPO_FILE		SYSTEM_DIR "typos.txt"  /* For 'typo'       */
#define FIXED_FILE	SYSTEM_DIR "fixed.txt"  /* For 'fixed' command */
#define LOG_FILE		SYSTEM_DIR "log.txt" /* For talking in logged rooms */
#define MOBLOG_FILE	SYSTEM_DIR "moblog.txt" /* For mplog messages  */
#define WIZLIST_FILE	SYSTEM_DIR "WIZLIST" /* Wizlist       */
#define SKILL_FILE	SYSTEM_DIR "skills.dat" /* Skill table         */
#define HERB_FILE		SYSTEM_DIR "herbs.dat"  /* Herb table       */
#define NAMEGEN_FILE	SYSTEM_DIR "namegen.txt"   /* Used for the name generator */
#define TONGUE_FILE	SYSTEM_DIR "tongues.dat"   /* Tongue tables    */
#define SOCIAL_FILE	SYSTEM_DIR "socials.dat"   /* Socials       */
#define COMMAND_FILE	SYSTEM_DIR "commands.dat"  /* Commands      */
#define WEBWHO_FILE	WEB_DIR "WEBWHO"  // HTML Who output file
#define WEBWIZ_FILE     WEB_DIR "WEBWIZ"  // HTML wizlist file
#define AREALIST_FILE   WEB_DIR "AREALIST"   // HTML areas listing
#define MOTD_FILE       MOTD_DIR "motd.dat"
#define IMOTD_FILE      MOTD_DIR "imotd.dat"
#define SPEC_MOTD       MOTD_DIR "specmotd.dat" /* Special MOTD - cannot be ignored on login */
#define LOGIN_MSG       "login.msg"               /* List of login msgs      */

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

// This reads a string value into a C++ string variable using the tilde as a delimeter - Samson 10-3-04
#define STDSKEY( literal, field )      \
if( !strcasecmp( word, (literal) ) )   \
{                                      \
   (field).clear();                    \
   fread_string( (field), fp );        \
   break;                              \
}

// This reads a string value into a C++ string variable using line-feed as a delimeter - Samson 10-3-04
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
 * Take care *NOT* to use these with bitset types! You'll get unpredictable results!
 */
#define IS_SET(flag, bit)	((flag) & (bit))
#define SET_BIT(var, bit)	((var) |= (bit))
#define REMOVE_BIT(var, bit)	((var) &= ~(bit))
#define TOGGLE_BIT(var, bit)	((var) ^= (bit))

#define IS_SAVE_FLAG(bit)  sysdata->save_flags.test((bit))

#define IS_EXIT_FLAG(var, bit)     (var)->flags.test((bit))
#define SET_EXIT_FLAG(var, bit)    (var)->flags.set((bit))
#define REMOVE_EXIT_FLAG(var, bit) (var)->flags.reset((bit))

/*
 * Memory allocation macros.
 */
#define CREATE(result, type, number)                                    \
do                                                                      \
{                                                                       \
   if (!((result) = (type *) calloc ((number), sizeof(type))))          \
   {                                                                    \
      perror("malloc failure");                                         \
      fprintf(stderr, "Malloc failure @ %s:%d\n", __FILE__, __LINE__ ); \
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
            delete[] (point);                  \
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
         log_printf( "&RSTRFREE called on str_dup pointer: %s, line %d\n", __FILE__, __LINE__ ); \
         log_string( "Attempting to correct." ); \
         free( (point) );                        \
      }                                          \
      else if( str_free((point)) == -1 )         \
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

/*
 * Description macros.
 */
#define PERS(ch, looker, from) ( (looker)->can_see( (ch), (from) ) ? \
      ( (ch)->isnpc() ? (ch)->short_descr : (ch)->name ) : "Someone" )

#define log_string(txt)		( log_string_plus( LOG_NORMAL, LEVEL_LOG, (txt) ) )

// Utility macros.
int umin( int, int );
int umax( int, int );
int urange( int, int, int );

#define UMIN( a, b )      ( umin( (a), (b) ) )
#define UMAX( a, b )      ( umax( (a), (b) ) )
#define URANGE(a, b, c )  ( urange( (a), (b), (c) ) )
#define LOWER( c )        ( (c) >= 'A' && (c) <= 'Z' ? (c) + 'a' - 'A' : (c) )
#define UPPER( c )        ( (c) >= 'a' && (c) <= 'z' ? (c) + 'A' - 'a' : (c) )

/* Safe fclose macro adopted from DOTD Codebase */
#define FCLOSE(fp) fclose((fp)); (fp)=nullptr;

/* Character data is used in pretty much everything. May as well globally include it here.
 * This includes the pc_data information as well.
 */
#include "character.h"

/* Object data is also used all over the place. */
#include "object.h"

/* object saving defines for fread/write_obj. -- Altrag */
enum carry_types
{
   OS_CARRY, OS_CORPSE
};

/*
 * Damage types from the attack_table[]
 */
/* modified for new weapon_types - Grimm */
/* Trimmed down to reduce duplicated types - Samson 1-9-00 */
enum damage_types
{
   DAM_HIT, DAM_SLASH, DAM_STAB, DAM_HACK, DAM_CRUSH, DAM_LASH,
   DAM_PIERCE, DAM_THRUST, DAM_MAX_TYPE
};

// Login Messages
class lmsg_data
{
 private:
   lmsg_data( const lmsg_data & p );
     lmsg_data & operator=( const lmsg_data & );

 public:
     lmsg_data(  );
    ~lmsg_data(  );

   char *name;
   char *text;
   short type;
};

/*
 * Used to keep track of system settings and statistics - Thoric
 */
class system_data
{
 private:
   system_data( const system_data & p );
     system_data & operator=( const system_data & );

 public:
     system_data(  );
    ~system_data(  );

   void *dlHandle;   // libdl System Handle - Trax
     bitset < SV_MAX > save_flags;  // Toggles for saving conditions
   string time_of_max;  // Time of max ever
   string mud_name;  // Name of mud
   string admin_email;  // Email address for admin - Samson 10-17-98
   string password;  // Port access code
   string telnet; // Store telnet address for who/webwho
   string http;   // Store web address for who/webwho
   string dbserver;  // Database server address for SQL support, usually localhost
   string dbname; // Database name for SQL support
   string dbuser; // Database username for SQL support
   string dbpass; // Database password for SQL support
   time_t motd;   // Last time MOTD was edited
   time_t imotd;  // Last time IMOTD was edited
   size_t maxign;
   size_t maxholiday;
   int maxplayers;   // Maximum players this boot
   int alltimemax;   // Maximum players ever
   int auctionseconds;  // Seconds between auction events
   int maxvnum;
   int minguildlevel;
   int maxcondval;
   int maximpact;
   int initcond;
   int minego;
   int secpertick;
   int pulsepersec;
   int pulsetick;
   int pulseviolence;
   int pulsemobile;
   int pulsecalendar;
   int pulseenvironment;
   int pulseskyship;
   int hoursperday;
   int daysperweek;
   int dayspermonth;
   int monthsperyear;
   int daysperyear;
   int hoursunrise;
   int hourdaybegin;
   int hournoon;
   int hoursunset;
   int hournightbegin;
   int hourmidnight;
   int rebootcount;  /* How many minutes to count down for a reboot - Samson 4-22-03 */
   int gameloopalarm;   /* Number of seconds before game_loop() triggers an alarm due to being hung up - Samson 1-24-05 */
   short webwho;  // Number of seconds between webwho refreshes, 0 for no refresh - Samson 5-13-06
   short read_all_mail; /* Read all player mail(was 54) */
   short read_mail_free;   /* Read mail for free (was 51) */
   short write_mail_free;  /* Write mail for free(was 51) */
   short take_others_mail; /* Take others mail (was 54)   */
   short build_level;   /* Level of build channel LEVEL_BUILD */
   short level_modify_proto;  /* Level to modify prototype stuff LEVEL_LESSER */
   short level_override_private; /* override private flag */
   short level_mset_player;   /* Level to mset a player */
   short bash_plr_vs_plr;  /* Bash mod player vs. player */
   short bash_nontank;  /* Bash mod basher != primary attacker */
   short gouge_plr_vs_plr; /* Gouge mod player vs. player */
   short gouge_nontank; /* Gouge mod player != primary attacker */
   short stun_plr_vs_plr;  /* Stun mod player vs. player */
   short stun_regular;  /* Stun difficult */
   short dodge_mod;  /* Divide dodge chance by */
   short parry_mod;  /* Divide parry chance by */
   short tumble_mod; /* Divide tumble chance by */
   short dam_plr_vs_plr;   /* Damage mod player vs. player */
   short dam_plr_vs_mob;   /* Damage mod player vs. mobile */
   short dam_mob_vs_plr;   /* Damage mod mobile vs. player */
   short dam_mob_vs_mob;   /* Damage mod mobile vs. mobile */
   short level_getobjnotake;  /* Get objects without take flag */
   short level_forcepc; /* The level at which you can use force on players. */
   short bestow_dif; /* Max # of levels between trust and command level for a bestow to work --Blodkai */
   short save_frequency;   /* How often to autosave someone */
   short newbie_purge;  /* Level to auto-purge newbies at - Samson 12-27-98 */
   short regular_purge; /* Level to purge normal players at - Samson 12-27-98 */
   short mapsize; /* Laziness feature mostly. Changes the overland map visibility radius */
   short playersonline;
   bool NO_NAME_RESOLVING; /* Hostnames are not resolved  */
   bool DENY_NEW_PLAYERS;  /* New players cannot connect  */
   bool WAIT_FOR_AUTH;  /* New players must be auth'ed */
   bool check_imm_host; /* Do we check immortal's hosts? */
   bool save_pets;   /* Do pets save? */
   bool WIZLOCK;  /* Is the game wizlocked? - Samson 8-2-98 */
   bool IMPLOCK;  /* Is the game implocked? - Samson 8-2-98 */
   bool LOCKDOWN; /* Is the game locked down? - Samson 8-23-98 */
   bool CLEANPFILES; /* Should the mud clean up pfiles daily? - Samson 12-27-98 */
   bool TESTINGMODE; /* Blocks file copies to main port when active - Samson 1-31-99 */
   bool crashhandler;   /* Do we intercept SIGSEGV - Samson 3-11-04 */
};

/*
 * Types of skill numbers.  Used to keep separate lists of sn's
 * Must be non-overlapping with spell/skill types,
 * but may be arbitrary beyond that.
 */
const int TYPE_UNDEFINED = -1;
const int TYPE_HIT = 1000; /* allows for 1000 skills/spells */
const int TYPE_HERB = 2000;   /* allows for 1000 attack types  */
const int TYPE_PERSONAL = 3000;  /* allows for 1000 herb types    */
const int TYPE_RACIAL = 4000; /* allows for 1000 personal types */
const int TYPE_DISEASE = 5000;   /* allows for 1000 racial types  */

/*
 *  Target types.
 */
enum target_types
{
   TAR_IGNORE, TAR_CHAR_OFFENSIVE, TAR_CHAR_DEFENSIVE, TAR_CHAR_SELF, TAR_OBJ_INV
};

enum skill_types
{
   SKILL_UNKNOWN, SKILL_SPELL, SKILL_SKILL, SKILL_COMBAT, SKILL_TONGUE,
   SKILL_HERB, SKILL_RACIAL, SKILL_DISEASE, SKILL_LORE
};

/*
 * Skill/Spell flags
 */
enum skill_spell_flags
{
   SF_WATER, SF_EARTH, SF_AIR, SF_ASTRAL, SF_AREA, SF_DISTANT,
   SF_REVERSE, SF_NOSELF, SF_NOCHARGE, SF_ACCUMULATIVE, SF_RECASTABLE,
   SF_NOSCRIBE, SF_NOBREW, SF_GROUPSPELL, SF_OBJECT, SF_CHARACTER,
   SF_SECRETSKILL, SF_PKSENSITIVE, SF_STOPONFAIL, SF_NOFIGHT,
   SF_NODISPEL, SF_RANDOMTARGET, SF_NOMOUNT, SF_NOMOB, MAX_SF_FLAG
};

/*
 * Skills include spells as a particular case.
 */
class skill_type
{
 private:
   skill_type( const skill_type & s );
     skill_type & operator=( const skill_type & );

 public:
     skill_type(  );
    ~skill_type(  );

     list < class smaug_affect * >affects;   /* Spell affects, if any */
     bitset < MAX_SF_FLAG > flags;  /* Flags */
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

/*
 * So we can have different configs for different ports -- Shaddai
 */
extern int mud_port;

/*
 * Global constants.
 */
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
 */
/* 
 * Stuff for area versions --Shaddai
 */
const int HAS_SPELL_INDEX = -1;
/* This is to tell if act uses uppercasestring or not --Shaddai */
extern bool DONT_UPPER;
extern bool MOBtrigger;
extern bool MPSilent;
extern bool mud_down;
extern bool DONTSAVE;

extern const unsigned char echo_off_str[];
extern int top_area;
extern int top_mob_index;
extern int top_obj_index;
extern int top_room;
extern char str_boot_time[];
extern char_data *timechar;
extern bool fBootDb;
extern char strArea[MIL];
extern int falling;

extern string target_name;
extern string ranged_target_name;
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
extern time_t current_time;
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

/* Formatted log output - 3-07-02 */
void log_printf( const char *, ... ) __attribute__ ( ( format( printf, 1, 2 ) ) );

/* act_comm.c */
bool is_same_group( char_data *, char_data * );
int knows_language( char_data *, int, char_data * );
void act( short, const string &, char_data *, const void *, const void *, int );
void act_printf( short, char_data *, const void *, const void *, int, const char *, ... ) __attribute__ ( ( format( printf, 6, 7 ) ) );

/* act_info.c */
bool is_ignoring( char_data *, char_data * );

/* act_move.c */
exit_data *find_door( char_data *, const string &, bool );
ch_ret move_char( char_data *, exit_data *, int, int, bool );

/* act_wiz.c */
char_data *get_wizvictim( char_data *, const string &, bool );
void echo_to_all( const string &, short );
void echo_all_printf( short, const char *, ... ) __attribute__ ( ( format( printf, 2, 3 ) ) );

/* build.c */
int get_risflag( const string & );
int get_defenseflag( const string & );
int get_attackflag( const string & );
int get_npc_sex( const string & );
int get_npc_position( const string & );
int get_npc_class( const string & );
int get_npc_race( const string & );
int get_pc_race( const string & );
int get_pc_class( const string & );
int get_actflag( const string & );
int get_pcflag( const string & );
int get_langnum( const string & );
int get_langflag( const string & );
int get_rflag( const string & );
int get_exflag( const string & );
int get_sectypes( const string & );
int get_areaflag( const string & );
int get_partflag( const string & );
int get_magflag( const string & );
int get_otype( const string & );
int get_aflag( const string & );
int get_atype( const string & );
int get_oflag( const string & );
int get_wflag( const string & );
int get_saveflag( const string & );
int get_logflag( const string & );
int get_trapflag( const string & );
int get_flag( const string &, const char **, size_t );
int get_dir( const string & );
char *flag_string( int, const char *flagarray[] );

/* calendar.c */
char *c_time( time_t, int );

/* comm.c */
bool check_parse_name( const string &, bool );
void add_loginmsg( const char *, short, const char * );

/* commands.c */
int check_command_level( const string &, int );
void cmdf( char_data *, const char *, ... ) __attribute__ ( ( format( printf, 2, 3 ) ) );
void funcf( char_data *, DO_FUN *, const char *, ... ) __attribute__ ( ( format( printf, 3, 4 ) ) );
void interpret( char_data *, string );
void check_switches(  );
void check_switch( char_data * );

/* db.c */
bool is_valid_filename( char_data *, const string &, const string & );
void shutdown_mud( const string & );
bool exists_file( const string & );
char fread_letter( FILE * );
int fread_number( FILE * );
short fread_short( FILE * );
long fread_long( FILE * );
float fread_float( FILE * );
char *fread_string( FILE * );
const char *fread_flagstring( FILE * );
char *fread_string_nohash( FILE * );
void fread_to_eol( FILE * );
const char *fread_line( FILE * );
char *fread_word( FILE * );
void fread_string( string &, FILE * );
void fread_line( string &, FILE * );
void log_printf_plus( short, short, const char *, ... ) __attribute__ ( ( format( printf, 3, 4 ) ) );
int number_percent( void );
int number_fuzzy( int );
int number_range( int, int );
int number_door( void );
int number_bits( int );
int dice( int, int );
void show_file( char_data *, const string & );
void append_file( char_data *, const string &, const char *, ... ) __attribute__ ( ( format( printf, 3, 4 ) ) );
void append_to_file( const string &, const char *, ... ) __attribute__ ( ( format( printf, 2, 3 ) ) );
void log_string_plus( short, short, const string & );
void make_wizlist( void );

/* editor.cpp */
bool hasname( const string &, const string & );
void removename( string &, const string & );
void addname( string &, const string & );
char *str_dup( const char * );
void stralloc_printf( char **, const char *, ... ) __attribute__ ( ( format( printf, 2, 3 ) ) );
void strdup_printf( char **, const char *, ... ) __attribute__ ( ( format( printf, 2, 3 ) ) );
size_t mudstrlcat( char * __restrict dst, const char * __restrict src, size_t dsize );
size_t mudstrlcpy( char * __restrict dst, const char * __restrict src, size_t dsize );
void smash_tilde( char * );
void smash_tilde( string & );
void hide_tilde( string & );
const char *show_tilde( const char * );
const string show_tilde( const string & );
int str_cmp( const string &, const string & );
bool str_cmp( const char *, const char * );
bool str_prefix( const string &, const string & );
bool str_prefix( const char *, const char * );
bool str_infix( const char *, const char * );
bool str_infix( const string &, const string & );
bool str_suffix( const char *, const char * );
char *capitalize( const char * );
string capitalize( const string & );
char *strlower( const char * );
void strlower( string & );
char *strupper( const char * );
void strupper( string & );
const char *aoran( const string & );
void strip_tilde( string & );
void strip_lspace( string & );
void strip_tspace( string & );
void strip_spaces( string & );
const char *strip_cr( const char * );
string strip_cr( const string & );
const char *strip_crlf( const char * );
void strip_whitespace( string & str );
bool is_number( const string & );
bool is_number( const char * );
int number_argument( const string &, string & );
int number_argument( char *, char * );
const char *one_argument( const char *, char * );
char *one_argument( char *, char * );
string one_argument( const string &, string & );
string invert_string( const string & );
const string add_percent( const string & );
void string_erase( string & src, char find );
void string_erase( string &, const string & );
void string_replace( string &, const string &, const string & );
vector < string > string_explode( const string &, char );
const char *print_array_string( const char *flagarray[], size_t arraySize );

/* fight.c */
ch_ret multi_hit( char_data *, char_data *, int );
ch_ret damage( char_data *, char_data *, double, int );

/* handler.c */
long exp_level( int );
bool nifty_is_name_prefix( string, string );
bool nifty_is_name_prefix( char *, char * );
bool is_name2_prefix( const string &, string );
bool is_name2_prefix( const char *, char * );
obj_data *get_obj_list( char_data *, const string &, list < obj_data * > );
ch_ret check_for_trap( char_data *, obj_data *, int );
ch_ret spring_trap( char_data *, obj_data * );
obj_data *find_obj( char_data *, string, bool );
bool ms_find_obj( char_data * );

/* hashstr.c */
char *str_alloc( const char *, const char *, const char *, int );
char *quick_link( char * );
int str_free( const char * );
void show_hash( int );
char *hash_stats( void );
char *check_hash( const char * );
void hash_dump( int );
void show_high_hash( int );
bool in_hash_table( const char * );

/* magic.c */
bool process_spell_components( char_data *, int );
bool saves_poison_death( int, char_data * );
bool saves_para_petri( int, char_data * );
bool saves_breath( int, char_data * );
bool saves_spell_staff( int, char_data * );
ch_ret obj_cast_spell( int, int, char_data *, char_data *, obj_data * );
int dice_parse( char_data *, int, const string & );
skill_type *get_skilltype( int );

/* mud_comm.c */
const string mprog_type_to_name( int );

/* mud_prog.c */
void mprog_entry_trigger( char_data * );
void mprog_greet_trigger( char_data * );
void progbug( const string &, char_data * );
void progbugf( char_data *, const char *, ... ) __attribute__ ( ( format( printf, 2, 3 ) ) );
void rset_supermob( room_index * );
void release_supermob( void );

/* overland.c */
bool is_same_char_map( char_data *, char_data * );
bool is_same_obj_map( char_data *, obj_data * );
void fix_maps( char_data *, char_data * );
void enter_map( char_data *, exit_data *, int, int, int );
void leave_map( char_data *, char_data *, room_index * );
void collect_followers( char_data *, room_index *, room_index * );

/* player.c */
const string condtxt( int, int );
const string attribtext( int );
bool exists_player( const string & );

/* save.c */
void write_corpse( obj_data *, const string & );

/* skills.c */
void disarm( char_data *, char_data * );
int find_spell( char_data *, const string &, bool );
int find_skill( char_data *, const string &, bool );
int find_ability( char_data *, const string &, bool );
int find_combat( char_data *, const string &, bool );
int find_tongue( char_data *, const string &, bool );
int find_lore( char_data *, const string &, bool );
int skill_lookup( const string & );
int herb_lookup( const string & );
int slot_lookup( int );
CMDF( skill_notfound );

/* tables.c */
SPELL_FUN *spell_function( const string & );
DO_FUN *skill_function( const string & );
SPEC_FUN *m_spec_lookup( const string & );

/* update.c */
void gain_condition( char_data *, int, int );
void update_handler( void );
void weather_update( void );

// This used to be the old ext_flagstring converted to C++ and using strings so it can't overflow the temporary buffer.
template < size_t N > const char *bitset_string( bitset < N > bits, const char *flagarray[] )
{
   static string s;

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

// This temlate is used during file reading to set flags based on the string names.
// Loosely resembled Remcon's WEXTKEY macro from his LoP codebase.
template < size_t N > void flag_set( FILE * fp, bitset < N > &field, const char *flagarray[] )
{
   string flags, flag;

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
template < size_t N > void flag_string_set( string & original, bitset < N > &field, const char *flagarray[] )
{
   string flag;

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

template < class N > extra_descr_data * get_extra_descr( const string & name, N * target )
{
   list < extra_descr_data * >::iterator ied;

   for( ied = target->extradesc.begin(  ); ied != target->extradesc.end(  ); ++ied )
   {
      extra_descr_data *ed = *ied;

      if( is_name2_prefix( name, ed->keyword ) )
      {
         if( !ed->desc.empty(  ) )
            return ed;
      }
   }
   return nullptr;
}

template < class N > extra_descr_data * set_extra_descr( N * target, const string & name )
{
   extra_descr_data *desc = nullptr;
   list < extra_descr_data * >::iterator ied;

   for( ied = target->extradesc.begin(  ); ied != target->extradesc.end(  ); ++ied )
   {
      extra_descr_data *ed = *ied;

      if( ed->keyword.find( name ) != string::npos )
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

// Thanks to David Haley for this little trick. A nifty little template that behaves like DISPOSE used to.
template < typename T > void deleteptr( T * &ptr )
{
   delete ptr;
   ptr = nullptr;
}
#endif
