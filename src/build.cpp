/****************************************************************************
 *                   ^     +----- |  / ^     ^ |     | +-\                  *
 *                  / \    |      | /  |\   /| |     | |  \                 *
 *                 /   \   +---   |<   | \ / | |     | |  |                 *
 *                /-----\  |      | \  |  v  | |     | |  /                 *
 *               /       \ |      |  \ |     | +-----+ +-/                  *
 ****************************************************************************
 * AFKMud Copyright 1997-2020 by Roger Libiez (Samson),                     *
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
 *                   Online Building and Editing Module                     *
 ****************************************************************************/

#include "mud.h"
#include "area.h"
#include "clans.h"
#include "deity.h"
#include "mobindex.h"
#include "mud_prog.h"
#include "objindex.h"
#include "overland.h"
#include "raceclass.h"
#include "realms.h"
#include "roomindex.h"
#include "treasure.h"

extern int top_affect;
list < rel_data * >relationlist;

void build_wizinfo( void );   /* For mset realm option - Samson 6-6-99 */
CMDF( do_rstat );
CMDF( do_mstat );
CMDF( do_ostat );
bool validate_spec_fun( const string & );
int mob_xp( char_data * );
char *sprint_reset( reset_data *, short & );
void assign_area( char_data * );
bool check_area_conflicts( int, int );

/*
 * Exit Pull/push types
 * (water, air, earth, fire)
 */
const char *ex_pmisc[] = { "undefined", "vortex", "vacuum", "slip", "ice", "mysterious" };

const char *ex_pwater[] = { "current", "wave", "whirlpool", "geyser" };

const char *ex_pair[] = { "wind", "storm", "coldwind", "breeze" };

const char *ex_pearth[] = { "landslide", "sinkhole", "quicksand", "earthquake" };

const char *ex_pfire[] = { "lava", "hotair" };

/* Stuff that isn't from stock Smaug */

const char *npc_sex[SEX_MAX] = {
   "neuter", "male", "female", "hermaphrodite"
};

const char *npc_position[POS_MAX] = {
   "dead", "mortal", "incapacitated", "stunned", "sleeping",
   "resting", "sitting", "berserk", "aggressive", "fighting", "defensive",
   "evasive", "standing", "mounted", "shove", "drag"
};

const char *styles[] = {
   "berserk", "aggressive", "standard", "defensive", "evasive"
};

const char *campgear[GEAR_MAX] = {
   "none", "bedroll", "misc", "firewood"
};

const char *ores[ORE_MAX] = {
   "unknown", "iron", "gold", "silver", "adamantine", "mithril", "blackmite",
   "titanium", "steel", "bronze", "dwarven", "elven"
};

const char *container_flags[] = {
   "closeable", "pickproof", "closed", "locked", "eatkey", "r1", "r2", "r3",
   "r4", "r5", "r6", "r7", "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15",
   "r16", "r17", "r18", "r19", "r20", "r21", "r22", "r23", "r24", "r25", "r26",
   "r27"
};

const char *weapon_skills[WEP_MAX] = {
   "Barehand", "Sword", "Dagger", "Whip", "Talon",
   "Mace", "Archery", "Blowgun", "Sling", "Axe", "Spear",
   "Staff"
};

const char *projectiles[PROJ_MAX] = {
   "Bolt", "Arrow", "Dart", "Stone"
};

/* Area continent table for continent/plane system */
const char *continents[] = {
   "one", "astral", "immortal"
};

const char *log_flag[] = {
   "normal", "always", "never", "build", "high", "comm", "warn", "info", "auth", "debug", "all"
};

const char *furniture_flags[] = {
   "sit_on", "sit_in", "sit_at",
   "stand_on", "stand_in", "stand_at",
   "sleep_on", "sleep_in", "sleep_at",
   "rest_on", "rest_in", "rest_at"
};

/* End of non-stock stuff */

const char *save_flag[] = {
   "death", "kill", "passwd", "drop", "put", "give", "auto", "zap",
   "auction", "get", "receive", "idle", "fill", "empty"
};

const char *ex_flags[] = {
   "isdoor", "closed", "locked", "secret", "swim", "pickproof", "fly", "climb",
   "dig", "eatkey", "nopassdoor", "hidden", "passage", "portal", "overland", "arrowslit",
   "can_climb", "can_enter", "can_leave", "auto", "noflee", "searchable",
   "bashed", "bashproof", "nomob", "window", "can_look", "isbolt", "bolted",
   "fortified", "heavy", "medium", "light", "crumbling", "destroyed"
};

const char *r_flags[] = {
   "dark", "death", "nomob", "indoors", "safe", "nocamp", "nosummon",
   "nomagic", "tunnel", "private", "silence", "nosupplicate", "arena", "nomissile",
   "norecall", "noportal", "noastral", "nodrop", "clanstoreroom", "teleport",
   "teleshowdesc", "nofloor", "solitary", "petshop", "donation", "nodropall",
   "logspeech", "proto", "noteleport", "noscry", "cave", "cavern", "nobeacon",
   "auction", "map", "forge", "guildinn", "isolated", "watchtower",
   "noquit", "telenofly", "_track_", "noyell", "nowhere", "notrack"
};

const char *o_flags[] = {
   "glow", "hum", "metal", "mineral", "organic", "invis", "magic", "nodrop", "bless",
   "antigood", "antievil", "antineutral", "anticleric", "antimage",
   "antirogue", "antiwarrior", "inventory", "noremove", "twohand", "evil",
   "donated", "clanobject", "clancorpse", "antibard", "hidden",
   "antidruid", "poisoned", "covering", "deathrot", "buried", "proto",
   "nolocate", "groundrot", "antimonk", "loyal", "brittle", "resistant",
   "immune", "antimen", "antiwomen", "antineuter", "antiherma", "antisun", "antiranger",
   "antipaladin", "antinecro", "antiapal", "onlycleric", "onlymage", "onlyrogue",
   "onlywarrior", "onlybard", "onlydruid", "onlymonk", "onlyranger", "onlypaladin",
   "onlynecro", "onlyapal", "auction", "onmap", "personal", "lodged", "sindhae",
   "mustmount", "noauction", "thrown", "permanent", "deathdrop", "nofill"
};

const char *w_flags[] = {
   "take", "finger", "neck", "body", "head", "legs", "feet", "hands", "arms",
   "shield", "about", "waist", "wrist", "wield", "hold", "dual", "ears", "eyes",
   "missile", "back", "face", "ankle", "hooves", "tail",
   "lodge_rib", "lodge_arm", "lodge_leg"
};

/* Area Flags for continent and plane system - Samson 3-28-98 */
const char *area_flags[] = {
   "nopkill", "nocamp", "noastral", "noportal", "norecall", "nosummon", "noscry",
   "noteleport", "arena", "nobeacon", "noquit", "prototype", "silence", "nomagic",
   "hidden", "nowhere"
};

const char *o_types[] = {
   "NONE", "light", "scroll", "wand", "staff", "weapon", "scabbard", "puddle",
   "treasure", "armor", "potion", "clothing", "furniture", "trash", "journal",
   "container", "UNUSED5", "drinkcon", "key", "food", "money", "pen", "boat",
   "corpse", "corpse_pc", "fountain", "pill", "blood", "bloodstain",
   "scraps", "pipe", "herbcon", "herb", "incense", "fire", "book", "switch",
   "lever", "pullchain", "button", "UNUSED", "rune", "runepouch", "match", "trap",
   "map", "portal", "paper", "tinder", "lockpick", "spike", "disease", "oil",
   "fuel", "piece", "tree", "missileweapon", "projectile", "quiver", "shovel",
   "salve", "cook", "keyring", "odor", "campgear", "drinkmix", "instrument", "ore"
};

const char *a_types[] = {
   "NONE", "strength", "intelligence", "wisdom", "dexterity", "constitution", "charisma",
   "luck", "level", "age", "height", "weight", "mana", "hit", "move",
   "gold", "experience", "armor", "hitroll", "damroll", "save_poison", "save_rod",
   "save_para", "save_breath", "save_spell", "sex", "affected", "resistant",
   "immune", "susceptible", "weaponspell", "class", "backstab", "pick", "track",
   "steal", "sneak", "hide", "palm", "detrap", "dodge", "spellfail", "scan", "gouge",
   "search", "mount", "disarm", "kick", "parry", "bash", "stun", "punch", "climb",
   "grip", "scribe", "brew", "wearspell", "removespell", "UNUSED", "mentalstate",
   "stripsn", "remove", "dig", "full", "thirst", "drunk", "hitregen",
   "manaregen", "moveregen", "antimagic", "roomflag", "sectortype",
   "roomlight", "televnum", "teledelay", "cook", "recurringspell", "race", "hit-n-dam",
   "save_all", "eat_spell", "race_slayer", "align_slayer", "contagious",
   "ext_affect", "odor", "peek", "absorb", "attacks", "extragold", "allstats"
};

const char *aff_flags[] = {
   "NONE", "blind", "invisible", "detect_evil", "detect_invis", "detect_magic",
   "detect_hidden", "hold", "sanctuary", "faerie_fire", "infrared", "curse",
   "grapple", "poison", "protect", "paralysis", "sneak", "hide", "sleep",
   "charm", "flying", "acidmist", "floating", "truesight", "detect_traps",
   "scrying", "fireshield", "shockshield", "venomshield", "iceshield", "wizardeye",
   "berserk", "aqua_breath", "recurringspell", "contagious", "bladebarrier",
   "silence", "UNUSED", "UNUSED", "UNUSED", "UNUSED",
   "growth", "tree_travel", "UNUSED", "UNUSED", "UNUSED",
   "passdoor", "quiv", "UNUSED", "haste", "slow", "elvensong", "bladesong",
   "reverie", "tenacity", "deathsong", "possess", "notrack", "enlighten", "treetalk",
   "spamguard", "bash"
};

const char *act_flags[] = {
   "npc", "sentinel", "scavenger", "innkeeper", "banker", "aggressive", "stayarea",
   "wimpy", "pet", "nosteal", "practice", "immortal", "deadly", "polyself",
   "meta_aggr", "guardian", "boarded", "nowander", "mountable", "mounted",
   "scholar", "secretive", "hardhat", "mobinvis", "noassist", "illusion",
   "pacifist", "noattack", "annoying", "auction", "proto", "mage", "warrior", "cleric",
   "rogue", "druid", "monk", "paladin", "ranger", "necromancer", "antipaladin",
   "huge", "greet", "teacher", "onmap", "smith", "guildauc", "guildbank", "guildvendor",
   "guildrepair", "guildforge", "idmob", "guildidmob", "stopscript"
};

const char *pc_flags[] = {
   "NONE", "deadly", "unauthed", "norecall", "nointro", "gag", "nobio", "nodesc",
   "nosummon", "pager", "notitled", "groupwho", "groupsplit", "nstart", "flags",
   "sector", "aname", "nobeep", "passdoor", "privacy", "notell", "checkboards",
   "noquote", "autoassist", "shovedrag", "autoexits", "autoloot", "autosac",
   "blank", "brief", "automap", "telnet_ga", "holylight", "wizinvis", "roomvnum",
   "silence", "noemote", "boarded", "no_tell", "log", "deny", "freeze", "exempt",
   "onship", "litterbug", "ansi", "msp", "autogold", "ghost", "afk", "nourl",
   "noemail", "smartsac", "idle", "onmap", "mapedit", "guildsplit", "compass",
   "retired", "no_beep"
};

const char *trap_types[] = {
   "Generic", "Poison Gas", "Poison Dart", "Poison Needle", "Poison Dagger",
   "Poison Arrow", "Blinding Gas", "Sleep Gas", "Flame", "Explosion",
   "Acid Spray", "Electric Shock", "Blade", "Sex Change"
};

const char *trap_flags[] = {
   "room", "obj", "enter", "leave", "open", "close", "get", "put", "pick",
   "unlock", "north", "south", "east", "west", "up", "down", "examine",
   "r2", "r3", "r4", "r5", "r6", "r7", "r8",
   "r9", "r10", "r11", "r12", "r13", "r14", "r15"
};

const char *wear_locs[] = {
   "light", "finger1", "finger2", "neck1", "neck2", "body", "head", "legs",
   "feet", "hands", "arms", "shield", "about", "waist", "wrist1", "wrist2",
   "wield", "hold", "dual_wield", "ears", "eyes", "missile_wield", "back",
   "face", "ankle1", "ankle2", "hooves", "tail"
};

const char *ris_flags[] = {
   "NONE", "fire", "cold", "electricity", "energy", "blunt", "pierce", "slash", "acid",
   "poison", "drain", "sleep", "charm", "hold", "nonmagic", "plus1", "plus2",
   "plus3", "plus4", "plus5", "plus6", "magic", "paralysis", "good", "evil", "hack",
   "lash"
};

const char *trig_flags[] = {
   "up", "unlock", "lock", "north", "south", "east", "west", "d_up", "d_down",
   "door", "container", "open", "close", "passage", "oload", "mload", "teleport",
   "teleportall", "teleportplus", "death", "cast", "showdesc", "rand4",
   "rand6", "autoreturn", "r26", "r27", "r28", "r29", "r30", "r31"
};

const char *part_flags[] = {
   "head", "arms", "legs", "heart", "brains", "guts", "hands", "feet", "fingers",
   "ear", "eye", "long_tongue", "eyestalks", "tentacles", "fins", "wings",
   "tail", "scales", "claws", "fangs", "horns", "tusks", "tailattack",
   "sharpscales", "beak", "haunches", "hooves", "paws", "forelegs", "feathers", "shell",
   "ankles"
};

const char *attack_flags[] = {
   "bite", "claws", "tail", "sting", "punch", "kick", "trip", "bash", "stun",
   "gouge", "backstab", "age", "drain", "firebreath", "frostbreath",
   "acidbreath", "lightnbreath", "gasbreath", "poison", "nastypoison", "gaze",
   "blindness", "causeserious", "earthquake", "causecritical", "curse",
   "flamestrike", "harm", "fireball", "colorspray", "weaken", "spiralblast"
};

const char *defense_flags[] = {
   "parry", "dodge", "heal", "curelight", "cureserious", "curecritical",
   "dispelmagic", "dispelevil", "sanctuary", "fireshield", "shockshield", "shield",
   "bless", "stoneskin", "teleport", "r3", "r4", "r5", "r6", "disarm", "r7", "grip",
   "truesight"
};

const char *lang_names[] = {
   "common", "elvish", "dwarven", "pixie", "ogre",
   "orcish", "trollese", "rodent", "insectoid",
   "mammal", "reptile", "dragon", "spiritual",
   "magical", "goblin", "god", "ancient",
   "halfling", "clan", "gith", "minotaur",
   "centaur", "gnomish", "sahuagin"
};

/*
 * Note: I put them all in one big set of flags since almost all of these
 * can be shared between mobs, objs and rooms for the exception of
 * bribe and hitprcnt, which will probably only be used on mobs.
 * ie: drop -- for an object, it would be triggered when that object is
 * dropped; -- for a room, it would be triggered when anything is dropped
 *          -- for a mob, it would be triggered when anything is dropped
 *
 * Something to consider: some of these triggers can be grouped together,
 * and differentiated by different arguments... for example:
 *  hour and time, rand and randiw, speech and speechiw
 * 
 */
const char *mprog_flags[] = {
   "act", "speech", "rand", "fight", "death", "hitprcnt", "entry", "greet",
   "allgreet", "give", "bribe", "hour", "time", "wear", "remove", "sac",
   "look", "exa", "zap", "get", "drop", "damage", "repair", "randiw",
   "speechiw", "pull", "push", "sleep", "rest", "leave", "script", "use",
   "speechand", "month", "keyword", "sell", "tell", "telland", "command",
   "emote", "login", "void", "load", "greetinfight"
};

/* Bamfin parsing code by Altrag, installed by Samson 12-10-97
   Allows use of $n where the player's name would be */
string bamf_print( const string & fmt, char_data * ch )
{
   string bamf = fmt;

   string_replace( bamf, "$n", ch->name );
   string_replace( bamf, "$N", ch->name );

   return bamf;
}

/*
 * Relations created to fix a crash bug with oset on and rset on
 * code by: gfinello@mail.karmanet.it
 */
void RelCreate( relation_type tp, void *actor, void *subject )
{
   list < rel_data * >::iterator tmp;
   rel_data *rel;

   if( tp < relMSET_ON || tp > relOSET_ON )
   {
      bug( "%s: invalid type (%d)", __func__, tp );
      return;
   }
   if( !actor )
   {
      bug( "%s: nullptr actor", __func__ );
      return;
   }
   if( !subject )
   {
      bug( "%s: nullptr subject", __func__ );
      return;
   }

   for( tmp = relationlist.begin(  ); tmp != relationlist.end(  ); ++tmp )
   {
      rel = *tmp;

      if( rel->Type == tp && rel->Actor == actor && rel->Subject == subject )
      {
         bug( "%s: duplicated relation", __func__ );
         return;
      }
   }
   rel = new rel_data;
   rel->Type = tp;
   rel->Actor = actor;
   rel->Subject = subject;
   relationlist.push_back( rel );
}

/*
 * Relations created to fix a crash bug with oset on and rset on
 * code by: gfinello@mail.karmanet.it
 */
void RelDestroy( relation_type tp, void *actor, void *subject )
{
   if( tp < relMSET_ON || tp > relOSET_ON )
   {
      bug( "%s: invalid type (%d)", __func__, tp );
      return;
   }
   if( !actor )
   {
      bug( "%s: nullptr actor", __func__ );
      return;
   }
   if( !subject )
   {
      bug( "%s: nullptr subject", __func__ );
      return;
   }

   list < rel_data * >::iterator rq;
   for( rq = relationlist.begin(  ); rq != relationlist.end(  ); ++rq )
   {
      rel_data *rel = *rq;

      if( rel->Type == tp && rel->Actor == actor && rel->Subject == subject )
      {
         relationlist.remove( rel );
         deleteptr( rel );
         return;
      }
   }
}

char *flag_string( int bitvector, const char *flagarray[] )
{
   static char buf[MSL];
   int x;

   buf[0] = '\0';
   for( x = 0; x < 32; ++x )
      if( IS_SET( bitvector, 1 << x ) )
      {
         mudstrlcat( buf, flagarray[x], MSL );
         /*
          * don't catenate a blank if the last char is blank  --Gorog 
          */
         if( buf[0] != '\0' && ' ' != buf[strlen( buf ) - 1] )
            mudstrlcat( buf, " ", MSL );
      }

   if( ( x = strlen( buf ) ) > 0 )
      buf[--x] = '\0';

   return buf;
}

bool can_rmodify( char_data * ch, room_index * room )
{
   int vnum = room->vnum;
   area_data *pArea;

   if( ch->isnpc(  ) )
      return false;

   if( ch->get_trust(  ) >= sysdata->level_modify_proto )
      return true;

   if( !room->flags.test( ROOM_PROTOTYPE ) )
   {
      ch->print( "You cannot modify this room.\r\n" );
      return false;
   }

   if( !( pArea = ch->pcdata->area ) )
   {
      ch->print( "You must have an assigned area to modify this room.\r\n" );
      return false;
   }

   if( vnum >= pArea->low_vnum && vnum <= pArea->hi_vnum )
      return true;

   ch->print( "That room is not in your allocated range.\r\n" );
   return false;
}

bool can_omodify( char_data * ch, obj_data * obj )
{
   int vnum = obj->pIndexData->vnum;
   area_data *pArea;

   if( ch->isnpc(  ) )
      return false;

   if( ch->get_trust(  ) >= sysdata->level_modify_proto )
      return true;

   if( !obj->extra_flags.test( ITEM_PROTOTYPE ) )
   {
      ch->print( "You cannot modify this object.\r\n" );
      return false;
   }

   if( !( pArea = ch->pcdata->area ) )
   {
      ch->print( "You must have an assigned area to modify this object.\r\n" );
      return false;
   }

   if( vnum >= pArea->low_vnum && vnum <= pArea->hi_vnum )
      return true;

   ch->print( "That object is not in your allocated range.\r\n" );
   return false;
}

bool can_mmodify( char_data * ch, char_data * mob )
{
   int vnum;
   area_data *pArea;

   if( mob == ch )
      return true;

   if( !mob->isnpc(  ) )
   {
      if( ch->get_trust(  ) >= sysdata->level_modify_proto && ch->get_trust(  ) > mob->get_trust(  ) )
         return true;
      else
         ch->print( "You can't do that.\r\n" );
      return false;
   }

   vnum = mob->pIndexData->vnum;

   if( ch->isnpc(  ) )
      return false;

   if( ch->get_trust(  ) >= sysdata->level_modify_proto )
      return true;

   if( !mob->has_actflag( ACT_PROTOTYPE ) )
   {
      ch->print( "You cannot modify this mobile.\r\n" );
      return false;
   }

   if( !( pArea = ch->pcdata->area ) )
   {
      ch->print( "You must have an assigned area to modify this mobile.\r\n" );
      return false;
   }

   if( vnum >= pArea->low_vnum && vnum <= pArea->hi_vnum )
      return true;

   ch->print( "That mobile is not in your allocated range.\r\n" );
   return false;
}

int get_saveflag( const string & name )
{
   for( size_t x = 0; x < sizeof( save_flag ) / sizeof( save_flag[0] ); ++x )
      if( !str_cmp( name, save_flag[x] ) )
         return x;
   return -1;
}

int get_logflag( const string & flag )
{
   for( int x = 0; x <= LOG_ALL; ++x )
      if( !str_cmp( flag, log_flag[x] ) )
         return x;
   return -1;
}

int get_npc_sex( const string & sex )
{
   for( int x = 0; x < SEX_MAX; ++x )
      if( !str_cmp( sex, npc_sex[x] ) )
         return x;
   return -1;
}

int get_style( const string & style )
{
   for( int x = 0; x <= STYLE_EVASIVE; ++x )
      if( !str_cmp( style, styles[x] ) )
         return x;
   return -1;
}

int get_npc_position( const string & position )
{
   for( int x = 0; x < POS_MAX; ++x )
      if( !str_cmp( position, npc_position[x] ) )
         return x;
   return -1;
}

int get_sectypes( const string & sector )
{
   for( int x = 0; x < SECT_MAX; ++x )
      if( !str_cmp( sector, sect_types[x] ) )
         return x;
   return -1;
}

int get_pc_class( const string & Class )
{
   for( int x = 0; x < MAX_PC_CLASS; ++x )
      if( !str_cmp( Class, class_table[x]->who_name ) )
         return x;
   return -1;
}

int get_npc_class( const string & Class )
{
   for( int x = 0; x < MAX_NPC_CLASS; ++x )
      if( !str_cmp( Class, npc_class[x] ) )
         return x;
   return -1;
}

int get_continent( const string & continent )
{
   for( int x = 0; x < ACON_MAX; ++x )
      if( !str_cmp( continent, continents[x] ) )
         return x;
   return -1;
}

int get_pc_race( const string & type )
{
   for( int i = 0; i < MAX_PC_RACE; ++i )
      if( !str_cmp( type, race_table[i]->race_name ) )
         return i;
   return -1;
}

int get_otype( const string & type )
{
   for( size_t x = 0; x < ( sizeof( o_types ) / sizeof( o_types[0] ) ); ++x )
      if( !str_cmp( type, o_types[x] ) )
         return x;
   return -1;
}

int get_aflag( const string & flag )
{
   for( size_t x = 0; x < ( sizeof( aff_flags ) / sizeof( aff_flags[0] ) ); ++x )
      if( !str_cmp( flag, aff_flags[x] ) )
         return x;
   return -1;
}

int get_traptype( const string & flag )
{
   for( size_t x = 0; x < ( sizeof( trap_types ) / sizeof( trap_types[0] ) ); ++x )
      if( !str_cmp( flag, trap_types[x] ) )
         return x;
   return -1;
}

int get_trapflag( const string & flag )
{
   for( size_t x = 0; x < ( sizeof( trap_flags ) / sizeof( trap_flags[0] ) ); ++x )
      if( !str_cmp( flag, trap_flags[x] ) )
         return x;
   return -1;
}

int get_atype( const string & type )
{
   for( size_t x = 0; x < ( sizeof( a_types ) / sizeof( a_types[0] ) ); ++x )
      if( !str_cmp( type, a_types[x] ) )
         return x;
   return -1;
}

int get_npc_race( const string & type )
{
   for( int x = 0; x < MAX_NPC_RACE; ++x )
      if( !str_cmp( type, npc_race[x] ) )
         return x;
   return -1;
}

int get_wearloc( const string & type )
{
   for( size_t x = 0; x < ( sizeof( wear_locs ) / sizeof( wear_locs[0] ) ); ++x )
      if( !str_cmp( type, wear_locs[x] ) )
         return x;
   return -1;
}

int get_exflag( const string & flag )
{
   for( size_t x = 0; x < ( sizeof( ex_flags ) / sizeof( ex_flags[0] ) ); ++x )
      if( !str_cmp( flag, ex_flags[x] ) )
         return x;
   return -1;
}

int get_pulltype( const string & type )
{
   size_t x;

   if( !str_cmp( type, "none" ) || !str_cmp( type, "clear" ) )
      return 0;

   for( x = 0; x < ( sizeof( ex_pmisc ) / sizeof( ex_pmisc[0] ) ); ++x )
      if( !str_cmp( type, ex_pmisc[x] ) )
         return x;

   for( x = 0; x < ( sizeof( ex_pwater ) / sizeof( ex_pwater[0] ) ); ++x )
      if( !str_cmp( type, ex_pwater[x] ) )
         return x + PT_WATER;

   for( x = 0; x < ( sizeof( ex_pair ) / sizeof( ex_pair[0] ) ); ++x )
      if( !str_cmp( type, ex_pair[x] ) )
         return x + PT_AIR;

   for( x = 0; x < ( sizeof( ex_pearth ) / sizeof( ex_pearth[0] ) ); ++x )
      if( !str_cmp( type, ex_pearth[x] ) )
         return x + PT_EARTH;

   for( x = 0; x < ( sizeof( ex_pfire ) / sizeof( ex_pfire[0] ) ); ++x )
      if( !str_cmp( type, ex_pfire[x] ) )
         return x + PT_FIRE;
   return -1;
}

// This is part of a rather slick way to set flags on things during file I/O
int get_flag( const string & flag, const char *flagarray[], size_t max )
{
   if( flag.empty(  ) || !flagarray )
      return -1;

   for( size_t x = 0; x < max; ++x )
      if( !str_cmp( flag, flagarray[x] ) )
         return x;
   return -1;
}

int get_rflag( const string & flag )
{
   for( size_t x = 0; x < ( sizeof( r_flags ) / sizeof( r_flags[0] ) ); ++x )
      if( !str_cmp( flag, r_flags[x] ) )
         return x;
   return -1;
}

int get_mpflag( const string & flag )
{
   for( size_t x = 0; x < ( sizeof( mprog_flags ) / sizeof( mprog_flags[0] ) ); ++x )
      if( !str_cmp( flag, mprog_flags[x] ) )
         return x;
   return -1;
}

int get_oflag( const string & flag )
{
   for( size_t x = 0; x < ( sizeof( o_flags ) / sizeof( o_flags[0] ) ); ++x )
      if( !str_cmp( flag, o_flags[x] ) )
         return x;
   return -1;
}

int get_areaflag( const string & flag )
{
   for( size_t x = 0; x < ( sizeof( area_flags ) / sizeof( area_flags[0] ) ); ++x )
      if( !str_cmp( flag, area_flags[x] ) )
         return x;
   return -1;
}

int get_wflag( const string & flag )
{
   for( size_t x = 0; x < ( sizeof( w_flags ) / sizeof( w_flags[0] ) ); ++x )
      if( !str_cmp( flag, w_flags[x] ) )
         return x;
   return -1;
}

int get_actflag( const string & flag )
{
   for( size_t x = 0; x < ( sizeof( act_flags ) / sizeof( act_flags[0] ) ); ++x )
      if( !str_cmp( flag, act_flags[x] ) )
         return x;
   return -1;
}

int get_pcflag( const string & flag )
{
   for( size_t x = 0; x < ( sizeof( pc_flags ) / sizeof( pc_flags[0] ) ); ++x )
      if( !str_cmp( flag, pc_flags[x] ) )
         return x;
   return -1;
}

int get_risflag( const string & flag )
{
   for( size_t x = 0; x < ( sizeof( ris_flags ) / sizeof( ris_flags[0] ) ); ++x )
      if( !str_cmp( flag, ris_flags[x] ) )
         return x;
   return -1;
}

int get_trigflag( const string & flag )
{
   for( size_t x = 0; x < ( sizeof( trig_flags ) / sizeof( trig_flags[0] ) ); ++x )
      if( !str_cmp( flag, trig_flags[x] ) )
         return x;
   return -1;
}

int get_partflag( const string & flag )
{
   for( size_t x = 0; x < ( sizeof( part_flags ) / sizeof( part_flags[0] ) ); ++x )
      if( !str_cmp( flag, part_flags[x] ) )
         return x;
   return -1;
}

int get_attackflag( const string & flag )
{
   for( size_t x = 0; x < ( sizeof( attack_flags ) / sizeof( attack_flags[0] ) ); ++x )
      if( !str_cmp( flag, attack_flags[x] ) )
         return x;
   return -1;
}

int get_defenseflag( const string & flag )
{
   for( size_t x = 0; x < ( sizeof( defense_flags ) / sizeof( defense_flags[0] ) ); ++x )
      if( !str_cmp( flag, defense_flags[x] ) )
         return x;
   return -1;
}

int get_containerflag( const string & flag )
{
   for( size_t x = 0; x < ( sizeof( container_flags ) / sizeof( container_flags[0] ) ); ++x )
      if( !str_cmp( flag, container_flags[x] ) )
         return x;
   return -1;
}

int get_furnitureflag( const string & flag )
{
   for( size_t x = 0; x < ( sizeof( furniture_flags ) / sizeof( furniture_flags[0] ) ); ++x )
      if( !str_cmp( flag, furniture_flags[x] ) )
         return x;
   return -1;
}

int get_langnum( const string & flag )
{
   for( size_t x = 0; x < ( sizeof( lang_names ) / sizeof( lang_names[0] ) ); ++x )
      if( !str_cmp( flag, lang_names[x] ) )
         return x;
   return -1;
}

void goto_char( char_data * ch, char_data * wch )
{
   room_index *location;

   ch->set_color( AT_IMMORT );
   location = wch->in_room;

   if( is_ignoring( wch, ch ) )
   {
      ch->print( "No such location.\r\n" );
      return;
   }

   if( location->is_private(  ) )
   {
      if( ch->level < sysdata->level_override_private )
      {
         ch->print( "That room is private right now.\r\n" );
         return;
      }
      else
         ch->print( "Overriding private flag!\r\n" );
   }

   if( location->flags.test( ROOM_ISOLATED ) && ch->level < LEVEL_SUPREME )
   {
      ch->print( "Go away! That room has been sealed for privacy!\r\n" );
      return;
   }

   if( ch->fighting )
      ch->stop_fighting( true );

   /*
    * Bamfout processing by Altrag, installed by Samson 12-10-97 
    */
   if( ch->pcdata && ch->pcdata->bamfout && ch->pcdata->bamfout[0] != '\0' )
      act( AT_IMMORT, "$T", ch, nullptr, bamf_print( ch->pcdata->bamfout, ch ).c_str(  ), TO_CANSEE );
   else
      act( AT_IMMORT, "$n vanishes suddenly into thin air.", ch, nullptr, nullptr, TO_CANSEE );

   leave_map( ch, wch, location );

   /*
    * Bamfin processing by Altrag, installed by Samson 12-10-97 
    */
   if( ch->pcdata && ch->pcdata->bamfin && ch->pcdata->bamfin[0] != '\0' )
      act( AT_IMMORT, "$T", ch, nullptr, bamf_print( ch->pcdata->bamfin, ch ).c_str(  ), TO_CANSEE );
   else
      act( AT_IMMORT, "$n appears suddenly out of thin air.", ch, nullptr, nullptr, TO_CANSEE );
}

void goto_obj( char_data * ch, obj_data * obj, const string & argument )
{
   room_index *location;

   ch->set_color( AT_IMMORT );
   location = obj->in_room;

   if( !location && obj->carried_by )
      location = obj->carried_by->in_room;
   else  /* It's in a container, this becomes too much hassle to recursively locate */
   {
      ch->printf( "%s is inside a container. Try locating that container first.\r\n", argument.c_str(  ) );
      return;
   }

   if( !location )
   {
      bug( "%s: Object in nullptr room!", __func__ );
      return;
   }

   if( location->is_private(  ) )
   {
      if( ch->level < sysdata->level_override_private )
      {
         ch->print( "That room is private right now.\r\n" );
         return;
      }
      else
         ch->print( "Overriding private flag!\r\n" );
   }

   if( location->flags.test( ROOM_ISOLATED ) && ch->level < LEVEL_SUPREME )
   {
      ch->print( "Go away! That room has been sealed for privacy!\r\n" );
      return;
   }

   if( ch->fighting )
      ch->stop_fighting( true );

   /*
    * Bamfout processing by Altrag, installed by Samson 12-10-97 
    */
   if( ch->pcdata && ch->pcdata->bamfout && ch->pcdata->bamfout[0] != '\0' )
      act( AT_IMMORT, "$T", ch, nullptr, bamf_print( ch->pcdata->bamfout, ch ).c_str(  ), TO_CANSEE );
   else
      act( AT_IMMORT, "$n vanishes suddenly into thin air.", ch, nullptr, nullptr, TO_CANSEE );

   leave_map( ch, nullptr, location );

   /*
    * Bamfin processing by Altrag, installed by Samson 12-10-97 
    */
   if( ch->pcdata && ch->pcdata->bamfin && ch->pcdata->bamfin[0] != '\0' )
      act( AT_IMMORT, "$T", ch, nullptr, bamf_print( ch->pcdata->bamfin, ch ).c_str(  ), TO_CANSEE );
   else
      act( AT_IMMORT, "$n appears suddenly out of thin air.", ch, nullptr, nullptr, TO_CANSEE );
}

CMDF( do_goto )
{
   string arg;
   room_index *location;
   char_data *wch;
   obj_data *obj;
   area_data *pArea;
   int vnum;

   argument = one_argument( argument, arg );

   if( arg.empty(  ) )
   {
      ch->print( "Goto where?\r\n" );
      return;
   }

   /*
    * Begin Overland Map additions 
    */
   if( !str_cmp( arg, "map" ) )
   {
      string arg1, arg2;
      int x, y, map = -1;

      argument = one_argument( argument, arg1 );
      argument = one_argument( argument, arg2 );

      if( arg1.empty(  ) )
      {
         ch->print( "Goto which map??\r\n" );
         return;
      }

      if( !str_cmp( arg1, "one" ) )
         map = ACON_ONE;

      if( map == -1 )
      {
         ch->printf( "There isn't a map for '%s'.\r\n", arg1.c_str(  ) );
         return;
      }

      if( arg2.empty(  ) && argument.empty(  ) )
      {
         enter_map( ch, nullptr, 499, 499, map );
         return;
      }

      if( arg2.empty(  ) || argument.empty(  ) )
      {
         ch->print( "Usage: goto map <mapname> <X> <Y>\r\n" );
         return;
      }

      x = atoi( arg2.c_str(  ) );
      y = atoi( argument.c_str(  ) );

      if( x < 0 || x >= MAX_X )
      {
         ch->printf( "Valid x coordinates are 0 to %d.\r\n", MAX_X - 1 );
         return;
      }

      if( y < 0 || y >= MAX_Y )
      {
         ch->printf( "Valid y coordinates are 0 to %d.\r\n", MAX_Y - 1 );
         return;
      }

      enter_map( ch, nullptr, x, y, map );
      return;
   }
   /*
    * End of Overland Map additions 
    */

   if( !is_number( arg ) )
   {
      if( ( wch = ch->get_char_world( arg ) ) != nullptr && wch->in_room != nullptr )
      {
         goto_char( ch, wch );
         return;
      }

      if( ( obj = ch->get_obj_world( arg ) ) != nullptr )
      {
         goto_obj( ch, obj, arg );
         return;
      }
   }

   if( !( location = ch->find_location( arg ) ) )
   {
      vnum = atoi( arg.c_str(  ) );
      if( vnum < 0 || get_room_index( vnum ) )
      {
         ch->print( "You cannot find that...\r\n" );
         return;
      }

      if( ch->get_trust(  ) < LEVEL_CREATOR || vnum < 1 || ch->isnpc(  ) || !ch->pcdata->area )
      {
         ch->print( "No such location.\r\n" );
         return;
      }

      if( ch->get_trust(  ) < sysdata->level_modify_proto )
      {
         if( !ch->pcdata || !( pArea = ch->pcdata->area ) )
         {
            ch->print( "You must have an assigned area to create rooms.\r\n" );
            return;
         }
         if( vnum < pArea->low_vnum || vnum > pArea->hi_vnum )
         {
            ch->print( "That room is not within your assigned range.\r\n" );
            return;
         }
      }

      if( vnum < 1 || vnum > sysdata->maxvnum )
      {
         ch->printf( "Invalid vnum. Allowable range is 1 to %d\r\n", sysdata->maxvnum );
         return;
      }

      location = make_room( vnum, ch->pcdata->area );
      if( !location )
      {
         bug( "%s: failed", __func__ );
         return;
      }
      ch->print( "&WWaving your hand, you form order from swirling chaos,\r\nand step into a new reality...\r\n" );
   }

   if( location->is_private(  ) )
   {
      if( ch->level < sysdata->level_override_private )
      {
         ch->print( "That room is private right now.\r\n" );
         return;
      }
      else
         ch->print( "Overriding private flag!\r\n" );
   }

   if( location->flags.test( ROOM_ISOLATED ) && ch->level < LEVEL_SUPREME )
   {
      ch->print( "Go away! That room has been sealed for privacy!\r\n" );
      return;
   }

   if( ch->fighting )
      ch->stop_fighting( true );

   /*
    * Bamfout processing by Altrag, installed by Samson 12-10-97 
    */
   if( ch->pcdata && ch->pcdata->bamfout && ch->pcdata->bamfout[0] != '\0' )
      act( AT_IMMORT, "$T", ch, nullptr, bamf_print( ch->pcdata->bamfout, ch ).c_str(  ), TO_CANSEE );
   else
      act( AT_IMMORT, "$n vanishes suddenly into thin air.", ch, nullptr, nullptr, TO_CANSEE );

   /*
    * It's assumed that if you've come this far, it's a room vnum you entered 
    */
   leave_map( ch, nullptr, location );

   if( ch->pcdata && ch->pcdata->bamfin && ch->pcdata->bamfin[0] != '\0' )
      act( AT_IMMORT, "$T", ch, nullptr, bamf_print( ch->pcdata->bamfin, ch ).c_str(  ), TO_CANSEE );
   else
      act( AT_IMMORT, "$n appears suddenly out of thin air.", ch, nullptr, nullptr, TO_CANSEE );
}

CMDF( do_mset )
{
   string arg1, arg2, arg3;
   int num, size, plus, value, minattr, maxattr;
   char char1, char2;
   char_data *victim, *tmpmob;
   bool lockvic;
   string origarg = argument;

   ch->set_color( AT_PLAIN );

   if( ch->isnpc(  ) )
   {
      ch->print( "Mob's can't mset\r\n" );
      return;
   }

   if( !ch->desc )
   {
      ch->print( "You have no descriptor\r\n" );
      return;
   }

   switch ( ch->substate )
   {
      default:
         break;

      case SUB_MOB_DESC:
         if( !ch->pcdata->dest_buf )
         {
            ch->print( "Fatal error: report to Samson.\r\n" );
            bug( "%s: sub_mob_desc: nullptr ch->pcdata->dest_buf", __func__ );
            ch->substate = SUB_NONE;
            return;
         }
         victim = ( char_data * ) ch->pcdata->dest_buf;

         if( victim->char_died(  ) )
         {
            ch->print( "Your victim died!\r\n" );
            ch->stop_editing(  );
            return;
         }
         STRFREE( victim->chardesc );
         victim->chardesc = ch->copy_buffer( true );
         if( victim->has_actflag( ACT_PROTOTYPE ) )
         {
            STRFREE( victim->pIndexData->chardesc );
            victim->pIndexData->chardesc = QUICKLINK( victim->chardesc );
         }
         tmpmob = ( char_data * ) ch->pcdata->spare_ptr;
         ch->stop_editing(  );
         ch->pcdata->dest_buf = tmpmob;
         ch->substate = ch->tempnum;
         return;

      case SUB_EDIT_ABORT:
         ch->substate = SUB_NONE;
         ch->print( "Aborting description.\r\n" );
         return;
   }

   victim = nullptr;
   lockvic = false;
   smash_tilde( argument );

   if( ch->substate == SUB_REPEATCMD )
   {
      victim = ( char_data * ) ch->pcdata->dest_buf;

      if( !victim )
      {
         ch->print( "Your victim died!\r\n" );
         argument = "done";
      }
      if( argument.empty(  ) || !str_cmp( argument, " " ) || !str_cmp( argument, "stat" ) )
      {
         if( victim )
         {
            if( !victim->isnpc(  ) )
               do_mstat( ch, victim->name );
            else
               funcf( ch, do_mstat, "%d", victim->pIndexData->vnum );
         }
         else
            ch->print( "No victim selected.  Type '?' for help.\r\n" );
         return;
      }

      if( !str_cmp( argument, "done" ) || !str_cmp( argument, "off" ) )
      {
         if( ch->pcdata->dest_buf )
            RelDestroy( relMSET_ON, ch, ch->pcdata->dest_buf );
         ch->print( "Mset mode off.\r\n" );
         ch->substate = SUB_NONE;
         ch->pcdata->dest_buf = nullptr;
         STRFREE( ch->pcdata->subprompt );
         return;
      }
   }

   if( victim )
   {
      lockvic = true;
      arg1 = victim->name;
      argument = one_argument( argument, arg2 );
      arg3 = argument;
   }
   else
   {
      lockvic = false;
      argument = one_argument( argument, arg1 );
      argument = one_argument( argument, arg2 );
      arg3 = argument;
   }

   if( !str_cmp( arg1, "on" ) )
   {
      ch->print( "Syntax: mset <victim|vnum> on.\r\n" );
      return;
   }

   if( arg1.empty(  ) || ( arg2.empty(  ) && ch->substate != SUB_REPEATCMD ) || !str_cmp( arg1, "?" ) || !str_cmp( arg1, "help" ) )
   {
      if( ch->substate == SUB_REPEATCMD )
      {
         if( victim )
            ch->print( "Syntax: <field> <value>\r\n" );
         else
            ch->print( "Syntax: <victim> <field> <value>\r\n" );
      }

      /*
       * Output reformatted by Whir - 8/27/98 
       */
      ch->print( "Syntax: mset <victim> <field> <value>\r\n\r\n" );
      ch->print( "Field being one of:\r\n" );
      ch->print( " [Naming]\r\n" );
      ch->print( "   |name       |short      |long       |title      |description\r\n" );
      ch->print( " [Stats]\r\n" );
      ch->print( "   |height     |weight     |sex        |align      |race\r\n" );
      ch->print( "   |gold       |hp         |mana       |move       |pracs\r\n" );
      ch->print( "   |Class      |level      |pos        |defpos     |part\r\n" );
      ch->print( "   |speaking   |speaks     |resist (r) |immune (i) |suscept (s)\r\n" );
      ch->print( "   |absorb	|agemod	|		|		|\r\n" );
      ch->print( " [Combat stats]\r\n" );
      ch->print( "   |thac0      |numattacks |armor      |affected   |attack\r\n" );
      ch->print( "   |defense    |           |           |           |\r\n" );
      ch->print( " [Groups]\r\n" );
      ch->print( "   |clan       |favor      |deity\r\n" );
      ch->print( " [Misc]\r\n" );
      ch->print( "   |thirst     |drunk      |hunger     |flags\r\n\r\n" );
      ch->print( "  see BODYPARTS, RIS, LANGAUGES, and SAVINGTHROWS for help\r\n\r\n" );
      ch->print( "For editing index/prototype mobiles:\r\n" );
      ch->print( "   |hitnumdie  |hitsizedie |hitplus (hit points)\r\n" );
      ch->print( "   |damnumdie  |damsizedie |damplus (damage roll)\r\n" );
      if( IS_ADMIN_REALM(ch) )
      {
         ch->print( "\r\nTo toggle pkill flag: pkill\r\n" );
         ch->print( "------------------------------------------------------------\r\n" );
         ch->print( "   realm\r\n" );
      }
      return;
   }

   if( !victim && ch->get_trust(  ) < LEVEL_GOD )
   {
      if( !( victim = ch->get_char_room( arg1 ) ) )
      {
         ch->print( "Where are they again?\r\n" );
         return;
      }
   }
   else if( !victim )
   {
      if( !( victim = ch->get_char_world( arg1 ) ) )
      {
         ch->printf( "Sorry, %s doesn't seem to exist in this universe.\r\n", arg1.c_str(  ) );
         return;
      }
   }

   if( ch->get_trust(  ) < victim->get_trust(  ) && !victim->isnpc(  ) )
   {
      ch->print( "Not this time chummer!\r\n" );
      ch->pcdata->dest_buf = nullptr;
      return;
   }

   if( !can_mmodify( ch, victim ) )
      return;

   if( lockvic )
      ch->pcdata->dest_buf = victim;

   if( victim->isnpc(  ) )
   {
      minattr = 1;
      maxattr = 25;
   }
   else
   {
      minattr = 3;
      maxattr = 18;
   }

   if( !str_cmp( arg2, "on" ) )
   {
      ch->CHECK_SUBRESTRICTED(  );
      ch->printf( "Mset mode on. (Editing %s).\r\n", victim->name );
      ch->substate = SUB_REPEATCMD;
      ch->pcdata->dest_buf = victim;
      if( victim->isnpc(  ) )
         stralloc_printf( &ch->pcdata->subprompt, "<&CMset &W#%d&w> %%i", victim->pIndexData->vnum );
      else
         stralloc_printf( &ch->pcdata->subprompt, "<&CMset &W%s&w> %%i", victim->name );
      RelCreate( relMSET_ON, ch, victim );
      return;
   }
   value = is_number( arg3 ) ? atoi( arg3.c_str(  ) ) : -1;

   if( atoi( arg3.c_str(  ) ) < -1 && value == -1 )
      value = atoi( arg3.c_str(  ) );

   if( !str_cmp( arg2, "str" ) )
   {
      if( value < minattr || value > maxattr )
      {
         ch->printf( "Strength range is %d to %d.\r\n", minattr, maxattr );
         return;
      }
      victim->perm_str = value;
      if( victim->has_actflag( ACT_PROTOTYPE ) )
         victim->pIndexData->perm_str = value;
      ch->print( "Strength set.\r\n" );
      return;
   }

   if( !str_cmp( arg2, "int" ) )
   {
      if( value < minattr || value > maxattr )
      {
         ch->printf( "Intelligence range is %d to %d.\r\n", minattr, maxattr );
         return;
      }
      victim->perm_int = value;
      if( victim->has_actflag( ACT_PROTOTYPE ) )
         victim->pIndexData->perm_int = value;
      ch->print( "Intelligence set.\r\n" );
      return;
   }

   if( !str_cmp( arg2, "wis" ) )
   {
      if( value < minattr || value > maxattr )
      {
         ch->printf( "Wisdom range is %d to %d.\r\n", minattr, maxattr );
         return;
      }
      victim->perm_wis = value;
      if( victim->has_actflag( ACT_PROTOTYPE ) )
         victim->pIndexData->perm_wis = value;
      ch->print( "Wisdom set.\r\n" );
      return;
   }

   if( !str_cmp( arg2, "dex" ) )
   {
      if( value < minattr || value > maxattr )
      {
         ch->printf( "Dexterity range is %d to %d.\r\n", minattr, maxattr );
         return;
      }
      victim->perm_dex = value;
      if( victim->has_actflag( ACT_PROTOTYPE ) )
         victim->pIndexData->perm_dex = value;
      ch->print( "Dexterity set.\r\n" );
      return;
   }

   if( !str_cmp( arg2, "con" ) )
   {
      if( value < minattr || value > maxattr )
      {
         ch->printf( "Constitution range is %d to %d.\r\n", minattr, maxattr );
         return;
      }
      victim->perm_con = value;
      if( victim->has_actflag( ACT_PROTOTYPE ) )
         victim->pIndexData->perm_con = value;
      ch->print( "Constitution set.\r\n" );
      return;
   }

   if( !str_cmp( arg2, "cha" ) )
   {
      if( value < minattr || value > maxattr )
      {
         ch->printf( "Charisma range is %d to %d.\r\n", minattr, maxattr );
         return;
      }
      victim->perm_cha = value;
      if( victim->has_actflag( ACT_PROTOTYPE ) )
         victim->pIndexData->perm_cha = value;
      ch->print( "Charisma set.\r\n" );
      return;
   }

   if( !str_cmp( arg2, "lck" ) )
   {
      if( value < minattr || value > maxattr )
      {
         ch->printf( "Luck range is %d to %d.\r\n", minattr, maxattr );
         return;
      }
      victim->perm_lck = value;
      if( victim->has_actflag( ACT_PROTOTYPE ) )
         victim->pIndexData->perm_lck = value;
      ch->print( "Luck set.\r\n" );
      return;
   }

   if( !str_cmp( arg2, "height" ) )
   {
      if( value < 0 || value > 500 )
      {
         ch->print( "Height range is 0 to 500.\r\n" );
         return;
      }

      if( value == 0 )
         value = victim->calculate_race_height(  );

      victim->height = value;
      if( victim->has_actflag( ACT_PROTOTYPE ) )
         victim->pIndexData->height = value;
      ch->print( "Height set.\r\n" );
      return;
   }

   if( !str_cmp( arg2, "weight" ) )
   {
      if( value < 0 || value > 30000 )
      {
         ch->print( "Weight range is 0 to 30000.\r\n" );
         return;
      }

      if( value == 0 )
         value = victim->calculate_race_weight(  );

      victim->weight = value;
      if( victim->has_actflag( ACT_PROTOTYPE ) )
         victim->pIndexData->weight = value;
      ch->print( "Weight set.\r\n" );
      return;
   }

   /*
    * Altered to allow sex changes by name instead of number - Samson 8-3-98 
    */
   if( !str_cmp( arg2, "sex" ) || !str_cmp( arg2, "gender" ) )
   {
      value = get_npc_sex( arg3 );

      if( value < 0 || value >= SEX_MAX )
      {
         ch->print( "Invalid gender.\r\n" );
         return;
      }
      victim->sex = value;
      if( victim->has_actflag( ACT_PROTOTYPE ) )
         victim->pIndexData->sex = value;
      ch->print( "Gender set.\r\n" );
      return;
   }

   /*
    * Altered to allow Class changes by name instead of number - Samson 8-2-98 
    */
   if( !str_cmp( arg2, "class" ) )
   {
      value = get_npc_class( arg3 );

      if( !victim->isnpc(  ) && ( value < 0 || value >= MAX_CLASS ) )
      {
         ch->printf( "%s is not a valid player class.\r\n", arg3.c_str(  ) );
         return;
      }
      if( victim->isnpc(  ) && ( value < 0 || value >= MAX_NPC_CLASS ) )
      {
         ch->printf( "%s is not a valid NPC class.\r\n", arg3.c_str(  ) );
         return;
      }
      victim->Class = value;
      if( victim->has_actflag( ACT_PROTOTYPE ) )
         victim->pIndexData->Class = value;
      ch->print( "Class set.\r\n" );
      return;
   }

   if( !str_cmp( arg2, "race" ) )
   {
      if( victim->isnpc(  ) )
         value = get_npc_race( arg3 );
      else
         value = get_pc_race( arg3 );

      if( !victim->isnpc(  ) && ( value < 0 || value >= MAX_PC_RACE ) )
      {
         ch->printf( "%s is not a valid player race.\r\n", arg3.c_str(  ) );
         return;
      }
      if( victim->isnpc(  ) && ( value < 0 || value >= MAX_NPC_RACE ) )
      {
         ch->printf( "%s is not a valid NPC race.\r\n", arg3.c_str(  ) );
         return;
      }

      // Change in race necessitates keeping height/weight values sane for that race. -- Samson 11/04/2007
      victim->race = value;
      victim->calculate_race_height(  );
      victim->calculate_race_weight(  );

      if( victim->has_actflag( ACT_PROTOTYPE ) )
         victim->pIndexData->race = value;
      ch->print( "Race set.\r\n" );
      return;
   }

   if( !str_cmp( arg2, "armor" ) || !str_cmp( arg2, "ac" ) )
   {
      if( value < -300 || value > 300 )
      {
         ch->print( "AC range is -300 to 300.\r\n" );
         return;
      }
      victim->armor = value;
      if( victim->has_actflag( ACT_PROTOTYPE ) )
         victim->pIndexData->ac = value;
      ch->print( "AC set.\r\n" );
      return;
   }

   if( !str_cmp( arg2, "gold" ) )
   {
      if( value < 0 )
         value = -1;
      victim->gold = value;
      if( victim->has_actflag( ACT_PROTOTYPE ) )
         victim->pIndexData->gold = value;
      ch->print( "Gold set.\r\n" );
      return;
   }

   if( !str_cmp( arg2, "exp" ) )
   {
      if( !victim->isnpc(  ) && !ch->is_imp(  ) )
      {
         ch->print( "Not on PC's.\r\n" );
         return;
      }

      if( !str_cmp( arg3, "x" ) || value < 0 )
         value = -1;

      if( value == -1 )
         victim->exp = mob_xp( victim );
      else
         victim->exp = value;

      if( victim->has_actflag( ACT_PROTOTYPE ) )
         victim->pIndexData->exp = value;
      ch->print( "Experience value set.\r\n" );
      return;
   }

   if( !str_cmp( arg2, "hp" ) )
   {
      if( value < 1 || value > 32700 )
      {
         ch->print( "Hp range is 1 to 32,700 hit points.\r\n" );
         return;
      }
      victim->max_hit = value;
      ch->print( "HP set.\r\n" );
      return;
   }

   if( !str_cmp( arg2, "mana" ) )
   {
      if( value < 0 || value > 32700 )
      {
         ch->print( "Mana range is 0 to 32,700 mana points.\r\n" );
         return;
      }
      victim->max_mana = value;
      if( victim->has_actflag( ACT_PROTOTYPE ) )
         victim->pIndexData->max_mana = value;
      ch->print( "Mana set.\r\n" );
      return;
   }

   if( !str_cmp( arg2, "move" ) )
   {
      if( value < 0 || value > 32700 )
      {
         ch->print( "Move range is 0 to 32,700 move points.\r\n" );
         return;
      }
      victim->max_move = value;
      if( victim->has_actflag( ACT_PROTOTYPE ) )
         victim->pIndexData->max_move = value;
      ch->print( "Movement set.\r\n" );
      return;
   }

   if( !str_cmp( arg2, "practice" ) )
   {
      if( victim->isnpc(  ) )
      {
         ch->print( "Not on NPC's.\r\n" );
         return;
      }

      if( value < 0 || value > 500 )
      {
         ch->print( "Practice range is 0 to 500 sessions.\r\n" );
         return;
      }
      victim->pcdata->practice = value;
      ch->print( "Practices set.\r\n" );
      return;
   }

   if( !str_cmp( arg2, "align" ) )
   {
      if( value < -1000 || value > 1000 )
      {
         ch->print( "Alignment range is -1000 to 1000.\r\n" );
         return;
      }
      victim->alignment = value;
      if( victim->has_actflag( ACT_PROTOTYPE ) )
         victim->pIndexData->alignment = value;
      ch->print( "Alignment set.\r\n" );
      return;
   }

   if( !str_cmp( arg2, "rank" ) )
   {
      if( ch->level < LEVEL_ASCENDANT )
      {
         ch->print( "You can't change your rank.\r\n" );
         return;
      }
      if( victim->isnpc(  ) )
      {
         ch->print( "Not on NPC's.\r\n" );
         return;
      }
      if( victim->level < LEVEL_ASCENDANT && !ch->is_imp(  ) )
      {
         ch->print( "Rank is intended for high level immortals ONLY.\r\n" );
         return;
      }
      smash_tilde( argument );
      STRFREE( victim->pcdata->rank );
      if( !argument.empty(  ) && str_cmp( argument, "none" ) )
         victim->pcdata->rank = STRALLOC( argument.c_str(  ) );
      ch->print( "Rank set.\r\n" );
      return;
   }

   if( !str_cmp( arg2, "favor" ) )
   {
      if( victim->isnpc(  ) )
      {
         ch->print( "Not on NPC's.\r\n" );
         return;
      }

      if( value < -2500 || value > 2500 )
      {
         ch->print( "Range is from -2500 to 2500.\r\n" );
         return;
      }

      victim->pcdata->favor = value;
      ch->print( "Favor set.\r\n" );
      return;
   }

   if( !str_cmp( arg2, "mentalstate" ) )
   {
      if( value < -100 || value > 100 )
      {
         ch->print( "Value must be in range -100 to +100.\r\n" );
         return;
      }
      victim->mental_state = value;
      ch->print( "Mental state set.\r\n" );
      return;
   }

   if( !str_cmp( arg2, "thirst" ) )
   {
      if( victim->isnpc(  ) )
      {
         ch->print( "Not on NPC's.\r\n" );
         return;
      }

      if( value < -1 || value > 100 )
      {
         ch->print( "Thirst range is -1 to 100.\r\n" );
         return;
      }
      victim->pcdata->condition[COND_THIRST] = value;
      ch->print( "Thirst set.\r\n" );
      return;
   }

   if( !str_cmp( arg2, "drunk" ) )
   {
      if( victim->isnpc(  ) )
      {
         ch->print( "Not on NPC's.\r\n" );
         return;
      }

      if( value < 0 || value > 100 )
      {
         ch->print( "Drunk range is 0 to 100.\r\n" );
         return;
      }
      victim->pcdata->condition[COND_DRUNK] = value;
      ch->print( "Drunk set.\r\n" );
      return;
   }

   if( !str_cmp( arg2, "agemod" ) )
   {
      if( victim->isnpc(  ) )
      {
         ch->print( "Not on NPC's.\r\n" );
         return;
      }
      victim->pcdata->age_bonus = value;
      ch->print( "Agemod set.\r\n" );
      return;
   }

   if( !str_cmp( arg2, "hunger" ) )
   {
      if( victim->isnpc(  ) )
      {
         ch->print( "Not on NPC's.\r\n" );
         return;
      }

      if( value < -1 || value > 100 )
      {
         ch->print( "Hunger range is -1 to 100.\r\n" );
         return;
      }
      victim->pcdata->condition[COND_FULL] = value;
      ch->print( "Hunger set.\r\n" );
      return;
   }

   if( !str_cmp( arg2, "realm" ) )
   {
      realm_data *realm;

      if( !IS_ADMIN_REALM(ch) )
      {
         ch->print( "You can't do that.\r\n" );
         return;
      }

      if( victim->isnpc(  ) )
      {
         ch->print( "Not on NPC's.\r\n" );
         return;
      }

      if( !victim->is_immortal(  ) )
      {
         ch->print( "This can only be done for immortals.\r\n" );
         return;
      }

      if( arg3.empty(  ) )
      {
         /*
          * Crash bug fix, oops guess I should have caught this one :)
          * * But it was early in the morning :P --Shaddai 
          */
         if( victim->pcdata->realm == nullptr )
            return;

         remove_realm_roster( victim->pcdata->realm, victim->name );
         --victim->pcdata->realm->members;
         if( victim->pcdata->realm->members < 0 )
            victim->pcdata->realm->members = 0;
         save_realm( victim->pcdata->realm );

         victim->pcdata->realm_name.clear(  );
         victim->pcdata->realm = nullptr;
         victim->save(  );
         build_wizinfo(  );
         ch->print( "Realm removed.\r\n" );
         return;
      }

      if( !( realm = get_realm( arg3 ) ) )
      {
         ch->print( "No such realm.\r\n" );
         return;
      }

      if( victim->pcdata->realm != nullptr )
      {
         remove_realm_roster( victim->pcdata->realm, victim->name );
         --victim->pcdata->realm->members;
         if( victim->pcdata->realm->members < 0 )
            victim->pcdata->realm->members = 0;
         save_realm( victim->pcdata->realm );
      }
      victim->pcdata->realm_name = realm->name;
      victim->pcdata->realm = realm;

      add_realm_roster( victim->pcdata->realm, victim->name );
      ++victim->pcdata->realm->members;
      save_realm( victim->pcdata->realm );

      victim->save(  );
      build_wizinfo(  );
      ch->print( "Realm set.\r\n" );
      return;
   }

   if( !str_cmp( arg2, "clan" ) || !str_cmp( arg2, "guild" ) )
   {
      clan_data *clan;

      if( !ch->is_imp(  ) )
      {
         ch->print( "You can't do that.\r\n" );
         return;
      }

      if( victim->isnpc(  ) )
      {
         ch->print( "Not on NPC's.\r\n" );
         return;
      }

      if( arg3.empty(  ) )
      {
         /*
          * Crash bug fix, oops guess I should have caught this one :)
          * * But it was early in the morning :P --Shaddai 
          */
         if( victim->pcdata->clan == nullptr )
            return;

         /*
          * Added a check on immortals so immortals don't take up
          * * any membership space. --Shaddai
          */
         if( !victim->is_immortal(  ) )
         {
            remove_roster( victim->pcdata->clan, victim->name );
            --victim->pcdata->clan->members;
            if( victim->pcdata->clan->members < 0 )
               victim->pcdata->clan->members = 0;
            save_clan( victim->pcdata->clan );
         }
         victim->pcdata->clan_name.clear(  );
         victim->pcdata->clan = nullptr;
         victim->save(  );
         ch->print( "Clan removed.\r\n" );
         return;
      }

      if( !( clan = get_clan( arg3 ) ) )
      {
         ch->print( "No such clan.\r\n" );
         return;
      }

      if( victim->pcdata->clan != nullptr && !victim->is_immortal(  ) )
      {
         remove_roster( victim->pcdata->clan, victim->name );
         --victim->pcdata->clan->members;
         if( victim->pcdata->clan->members < 0 )
            victim->pcdata->clan->members = 0;
         save_clan( victim->pcdata->clan );
      }
      victim->pcdata->clan_name = clan->name;
      victim->pcdata->clan = clan;
      if( !victim->is_immortal(  ) )
      {
         add_roster( victim->pcdata->clan, victim->name, victim->Class, victim->level, victim->pcdata->mkills, victim->pcdata->mdeaths );
         ++victim->pcdata->clan->members;
         save_clan( victim->pcdata->clan );
      }
      victim->save(  );
      ch->print( "Clan set.\r\n" );
      return;
   }

   if( !str_cmp( arg2, "deity" ) )
   {
      deity_data *deity;

      if( victim->isnpc(  ) )
      {
         ch->print( "Not on NPC's.\r\n" );
         return;
      }

      if( arg3.empty(  ) )
      {
         if( victim->pcdata->deity )
         {
            --victim->pcdata->deity->worshippers;
            if( victim->pcdata->deity->worshippers < 0 )
               victim->pcdata->deity->worshippers = 0;
            save_deity( victim->pcdata->deity );
         }
         victim->pcdata->deity_name.clear(  );
         victim->pcdata->deity = nullptr;
         ch->print( "Deity removed.\r\n" );
         return;
      }

      if( !( deity = get_deity( arg3 ) ) )
      {
         ch->print( "No such deity.\r\n" );
         return;
      }

      if( victim->pcdata->deity )
      {
         --victim->pcdata->deity->worshippers;
         if( victim->pcdata->deity->worshippers < 0 )
            victim->pcdata->deity->worshippers = 0;
         save_deity( victim->pcdata->deity );
      }
      victim->pcdata->deity_name = deity->name;
      victim->pcdata->deity = deity;
      ++deity->worshippers;
      save_deity( deity );
      ch->print( "Deity set.\r\n" );
      return;
   }

   if( !str_cmp( arg2, "short" ) )
   {
      STRFREE( victim->short_descr );
      victim->short_descr = STRALLOC( arg3.c_str(  ) );
      if( victim->has_actflag( ACT_PROTOTYPE ) )
      {
         STRFREE( victim->pIndexData->short_descr );
         victim->pIndexData->short_descr = QUICKLINK( victim->short_descr );
      }
      ch->print( "Short description set.\r\n" );
      return;
   }

   if( !str_cmp( arg2, "long" ) )
   {
      stralloc_printf( &victim->long_descr, "%s\r\n", arg3.c_str(  ) );
      if( victim->has_actflag( ACT_PROTOTYPE ) )
      {
         STRFREE( victim->pIndexData->long_descr );
         victim->pIndexData->long_descr = QUICKLINK( victim->long_descr );
      }
      ch->print( "Long description set.\r\n" );
      return;
   }

   if( !str_cmp( arg2, "description" ) || !str_cmp( arg2, "desc" ) )
   {
      if( !arg3.empty(  ) )
      {
         STRFREE( victim->chardesc );
         victim->chardesc = STRALLOC( arg3.c_str(  ) );
         if( victim->has_actflag( ACT_PROTOTYPE ) )
         {
            STRFREE( victim->pIndexData->chardesc );
            victim->pIndexData->chardesc = QUICKLINK( victim->chardesc );
         }
         ch->print( "Detailed description set.\r\n" );
         return;
      }
      ch->CHECK_SUBRESTRICTED(  );
      if( ch->substate == SUB_REPEATCMD )
         ch->tempnum = SUB_REPEATCMD;
      else
         ch->tempnum = SUB_NONE;

      if( lockvic )
         ch->pcdata->spare_ptr = victim;
      else
         ch->pcdata->spare_ptr = nullptr;

      ch->substate = SUB_MOB_DESC;
      ch->pcdata->dest_buf = victim;
      if( !victim->chardesc || victim->chardesc[0] == '\0' )
         victim->chardesc = STRALLOC( "" );
      if( victim->isnpc(  ) )
         ch->editor_desc_printf( "Description of mob, vnum %d (%s).", victim->pIndexData->vnum, victim->name );
      else
         ch->editor_desc_printf( "Description of player %s.", capitalize( victim->name ) );
      ch->start_editing( victim->chardesc );
      return;
   }

   if( !str_cmp( arg2, "title" ) )
   {
      if( victim->isnpc(  ) )
      {
         ch->print( "Not on NPC's.\r\n" );
         return;
      }
      victim->set_title( arg3 );
      ch->print( "Title set.\r\n" );
      return;
   }

   if( !str_cmp( arg2, "flags" ) )
   {
      bool protoflag = false;

      if( !victim->isnpc(  ) && ch->get_trust(  ) < LEVEL_GREATER )
      {
         ch->print( "You can only modify a mobile's flags.\r\n" );
         return;
      }

      if( argument.empty(  ) )
      {
         ch->print( "Usage: mset <victim> flags <flag> [flag] ...\r\n" );
         return;
      }

      if( victim->has_actflag( ACT_PROTOTYPE ) )
         protoflag = true;

      while( !argument.empty(  ) )
      {
         argument = one_argument( argument, arg3 );

         if( victim->isnpc(  ) )
         {
            value = get_actflag( arg3 );

            if( value < 0 || value >= MAX_ACT_FLAG )
               ch->printf( "Unknown flag: %s\r\n", arg3.c_str(  ) );
            else if( value == ACT_PROTOTYPE && ch->level < sysdata->level_modify_proto )
               ch->print( "You cannot change the prototype flag.\r\n" );
            else if( value == ACT_IS_NPC )
               ch->print( "If the NPC flag could be changed, it would cause many problems.\r\n" );
            else
               victim->toggle_actflag( value );
         }
         else
         {
            value = get_pcflag( arg3 );

            if( value < 0 || value >= MAX_PCFLAG )
               ch->printf( "Unknown flag: %s\r\n", arg3.c_str(  ) );
            else
               victim->toggle_pcflag( value );
         }
      }
      ch->print( "Flags set.\r\n" );
      if( victim->has_actflag( ACT_PROTOTYPE ) || ( value == ACT_PROTOTYPE && protoflag ) )
         victim->pIndexData->actflags = victim->get_actflags(  );
      return;
   }

   if( !str_cmp( arg2, "affected" ) )
   {
      if( !victim->isnpc(  ) && !ch->is_imp(  ) )
      {
         ch->print( "You can only modify a mobile's flags.\r\n" );
         return;
      }

      if( argument.empty(  ) )
      {
         ch->print( "Usage: mset <victim> affected <flag> [flag]...\r\n" );
         return;
      }

      while( !argument.empty(  ) )
      {
         argument = one_argument( argument, arg3 );
         value = get_aflag( arg3 );
         if( value < 0 || value >= MAX_AFFECTED_BY )
            ch->printf( "Unknown flag: %s\r\n", arg3.c_str(  ) );
         else
            victim->toggle_aflag( value );
      }
      if( victim->has_actflag( ACT_PROTOTYPE ) )
         victim->pIndexData->affected_by = victim->get_aflags(  );
      ch->print( "Affects set.\r\n" );
      return;
   }

   /*
    * save some more finger-leather for setting RIS stuff
    */
   if( !str_cmp( arg2, "r" ) )
   {
      if( !victim->isnpc(  ) && !ch->is_imp(  ) )
      {
         ch->print( "You can only modify a mobile's ris.\r\n" );
         return;
      }
      funcf( ch, do_mset, "%s resistant %s", arg1.c_str(  ), arg3.c_str(  ) );
      return;
   }
   if( !str_cmp( arg2, "i" ) )
   {
      if( !victim->isnpc(  ) && !ch->is_imp(  ) )
      {
         ch->print( "You can only modify a mobile's ris.\r\n" );
         return;
      }
      funcf( ch, do_mset, "%s immune %s", arg1.c_str(  ), arg3.c_str(  ) );
      return;
   }
   if( !str_cmp( arg2, "s" ) )
   {
      if( !victim->isnpc(  ) && !ch->is_imp(  ) )
      {
         ch->print( "You can only modify a mobile's ris.\r\n" );
         return;
      }
      funcf( ch, do_mset, "%s susceptible %s", arg1.c_str(  ), arg3.c_str(  ) );
      return;
   }
   if( !str_cmp( arg2, "ri" ) )
   {
      if( !victim->isnpc(  ) && !ch->is_imp(  ) )
      {
         ch->print( "You can only modify a mobile's ris.\r\n" );
         return;
      }
      funcf( ch, do_mset, "%s resistant %s", arg1.c_str(  ), arg3.c_str(  ) );
      funcf( ch, do_mset, "%s immune %s", arg1.c_str(  ), arg3.c_str(  ) );
      return;
   }
   if( !str_cmp( arg2, "rs" ) )
   {
      if( !victim->isnpc(  ) && !ch->is_imp(  ) )
      {
         ch->print( "You can only modify a mobile's ris.\r\n" );
         return;
      }
      funcf( ch, do_mset, "%s resistant %s", arg1.c_str(  ), arg3.c_str(  ) );
      funcf( ch, do_mset, "%s susceptible %s", arg1.c_str(  ), arg3.c_str(  ) );
      return;
   }
   if( !str_cmp( arg2, "is" ) )
   {
      if( !victim->isnpc(  ) && !ch->is_imp(  ) )
      {
         ch->print( "You can only modify a mobile's ris.\r\n" );
         return;
      }
      funcf( ch, do_mset, "%s immune %s", arg1.c_str(  ), arg3.c_str(  ) );
      funcf( ch, do_mset, "%s susceptible %s", arg1.c_str(  ), arg3.c_str(  ) );
      return;
   }
   if( !str_cmp( arg2, "ris" ) )
   {
      if( !victim->isnpc(  ) && !ch->is_imp(  ) )
      {
         ch->print( "You can only modify a mobile's ris.\r\n" );
         return;
      }
      funcf( ch, do_mset, "%s resistant %s", arg1.c_str(  ), arg3.c_str(  ) );
      funcf( ch, do_mset, "%s immune %s", arg1.c_str(  ), arg3.c_str(  ) );
      funcf( ch, do_mset, "%s susceptible %s", arg1.c_str(  ), arg3.c_str(  ) );
      return;
   }

   if( !str_cmp( arg2, "resistant" ) )
   {
      if( !victim->isnpc(  ) && !ch->is_imp(  ) )
      {
         ch->print( "You can only modify a mobile's resistancies.\r\n" );
         return;
      }

      if( argument.empty(  ) )
      {
         ch->print( "Usage: mset <victim> resistant <flag> [flag] ...\r\n" );
         return;
      }

      while( !argument.empty(  ) )
      {
         argument = one_argument( argument, arg3 );
         value = get_risflag( arg3 );
         if( value < 0 || value >= MAX_RIS_FLAG )
            ch->printf( "Unknown flag: %s\r\n", arg3.c_str(  ) );
         else
            victim->toggle_resist( value );
      }
      if( victim->has_actflag( ACT_PROTOTYPE ) )
         victim->pIndexData->resistant = victim->get_resists(  );
      ch->print( "Resistances set.\r\n" );
      return;
   }

   if( !str_cmp( arg2, "immune" ) )
   {
      if( !victim->isnpc(  ) && !ch->is_imp(  ) )
      {
         ch->print( "You can only modify a mobile's immunities.\r\n" );
         return;
      }

      if( argument.empty(  ) )
      {
         ch->print( "Usage: mset <victim> immune <flag> [flag] ...\r\n" );
         return;
      }

      while( !argument.empty(  ) )
      {
         argument = one_argument( argument, arg3 );
         value = get_risflag( arg3 );
         if( value < 0 || value >= MAX_RIS_FLAG )
            ch->printf( "Unknown flag: %s\r\n", arg3.c_str(  ) );
         else
            victim->toggle_immune( value );
      }
      if( victim->has_actflag( ACT_PROTOTYPE ) )
         victim->pIndexData->immune = victim->get_immunes(  );
      ch->print( "Immunities set.\r\n" );
      return;
   }

   if( !str_cmp( arg2, "susceptible" ) )
   {
      if( !victim->isnpc(  ) && !ch->is_imp(  ) )
      {
         ch->print( "You can only modify a mobile's susceptibilities.\r\n" );
         return;
      }

      if( argument.empty(  ) )
      {
         ch->print( "Usage: mset <victim> susceptible <flag> [flag] ...\r\n" );
         return;
      }

      while( !argument.empty(  ) )
      {
         argument = one_argument( argument, arg3 );
         value = get_risflag( arg3 );
         if( value < 0 || value >= MAX_RIS_FLAG )
            ch->printf( "Unknown flag: %s\r\n", arg3.c_str(  ) );
         else
            victim->toggle_suscep( value );
      }
      if( victim->has_actflag( ACT_PROTOTYPE ) )
         victim->pIndexData->susceptible = victim->get_susceps(  );
      ch->print( "Susceptibilities set.\r\n" );
      return;
   }

   if( !str_cmp( arg2, "absorb" ) )
   {
      if( !victim->isnpc(  ) && !ch->is_imp(  ) )
      {
         ch->print( "You can only modify a mobile's absorbs.\r\n" );
         return;
      }

      if( argument.empty(  ) )
      {
         ch->print( "Usage: mset <victim> absorb <flag> [flag] ...\r\n" );
         return;
      }

      while( !argument.empty(  ) )
      {
         argument = one_argument( argument, arg3 );
         value = get_risflag( arg3 );
         if( value < 0 || value >= MAX_RIS_FLAG )
            ch->printf( "Unknown flag: %s\r\n", arg3.c_str(  ) );
         else
            victim->toggle_absorb( value );
      }
      if( victim->has_actflag( ACT_PROTOTYPE ) )
         victim->pIndexData->absorb = victim->get_absorbs(  );
      ch->print( "Absorbs set.\r\n" );
      return;
   }

   if( !str_cmp( arg2, "part" ) )
   {
      if( !victim->isnpc(  ) && !ch->is_imp(  ) )
      {
         ch->print( "You can only modify a mobile's parts.\r\n" );
         return;
      }

      if( argument.empty(  ) )
      {
         ch->print( "Usage: mset <victim> part <flag> [flag] ...\r\n" );
         return;
      }

      while( !argument.empty(  ) )
      {
         argument = one_argument( argument, arg3 );
         value = get_partflag( arg3 );
         if( value < 0 || value >= MAX_BPART )
            ch->printf( "Unknown flag: %s\r\n", arg3.c_str(  ) );
         else
            victim->toggle_bpart( value );
      }
      if( victim->has_actflag( ACT_PROTOTYPE ) )
         victim->pIndexData->body_parts = victim->get_bparts(  );
      ch->print( "Body parts set.\r\n" );
      return;
   }

   if( !str_cmp( arg2, "pkill" ) )
   {
      if( victim->isnpc(  ) )
      {
         ch->print( "Player Characters only.\r\n" );
         return;
      }

      if( victim->has_pcflag( PCFLAG_DEADLY ) )
      {
         victim->unset_pcflag( PCFLAG_DEADLY );
         victim->print( "You are now a NON-PKILL player.\r\n" );
         if( ch != victim )
            ch->print( "That player is now non-pkill.\r\n" );
      }
      else
      {
         victim->set_pcflag( PCFLAG_DEADLY );
         victim->print( "You are now a PKILL player.\r\n" );
         if( ch != victim )
            ch->print( "That player is now pkill.\r\n" );
      }

      if( victim->pcdata->clan && !victim->is_immortal(  ) )
      {
         if( victim->speaking == LANG_CLAN )
            victim->speaking = LANG_COMMON;
         victim->unset_lang( LANG_CLAN );
         --victim->pcdata->clan->members;
         if( victim->pcdata->clan->members < 0 )
            victim->pcdata->clan->members = 0;
         if( !str_cmp( victim->name, victim->pcdata->clan->leader ) )
            victim->pcdata->clan->leader.clear(  );
         if( !str_cmp( victim->name, victim->pcdata->clan->number1 ) )
            victim->pcdata->clan->number1.clear(  );
         if( !str_cmp( victim->name, victim->pcdata->clan->number2 ) )
            victim->pcdata->clan->number2.clear(  );
         save_clan( victim->pcdata->clan );
         victim->pcdata->clan_name.clear(  );
         victim->pcdata->clan = nullptr;
      }
      victim->save(  );
      return;
   }

   if( !str_cmp( arg2, "speaks" ) )
   {
      if( argument.empty(  ) )
      {
         ch->print( "Usage: mset <victim> speaks <language> [language] ...\r\n" );
         return;
      }

      while( !argument.empty(  ) )
      {
         argument = one_argument( argument, arg3 );
         value = get_langnum( arg3 );
         if( value < 0 || value >= LANG_UNKNOWN )
            ch->printf( "Unknown language: %s\r\n", arg3.c_str(  ) );
         else if( !victim->isnpc(  ) )
         {
            if( !( value &= VALID_LANGS ) )
               ch->printf( "Players may not know %s.\r\n", arg3.c_str(  ) );
            else
               victim->toggle_lang( value );
         }
         else
            victim->toggle_lang( value );
      }
      if( victim->has_actflag( ACT_PROTOTYPE ) )
         victim->pIndexData->speaks = victim->get_langs(  );
      ch->print( "Speaks languages set.\r\n" );
      return;
   }

   if( !str_cmp( arg2, "speaking" ) )
   {
      if( argument.empty(  ) )
      {
         ch->print( "Usage: mset <victim> speaking <language>\r\n" );
         return;
      }

      argument = one_argument( argument, arg3 );
      value = get_langnum( arg3 );
      if( value < 0 || value >= LANG_UNKNOWN )
         ch->printf( "Unknown language: %s\r\n", arg3.c_str(  ) );
      else
         victim->speaking = value;
      if( victim->has_actflag( ACT_PROTOTYPE ) )
         victim->pIndexData->speaking = victim->speaking;
      ch->print( "Speaking language set.\r\n" );
      return;
   }

   if( !str_cmp( arg2, "style" ) )
   {
      value = get_style( arg3 );

      if( value < 0 || value > STYLE_EVASIVE )
      {
         ch->print( "Invalid style.\r\n" );
         return;
      }
      victim->style = value;
      ch->print( "Fighting style set.\r\n" );
      return;
   }

   if( !victim->isnpc(  ) )
   {
      ch->printf( "Cannot change %s on PC's.\r\n", arg2.c_str(  ) );
      return;
   }

   if( !str_cmp( arg2, "level" ) )
   {
      if( value < 0 || value > LEVEL_AVATAR + 10 )
      {
         ch->printf( "Level range is 0 to %d.\r\n", LEVEL_AVATAR + 10 );
         return;
      }
      victim->level = value;
      if( victim->has_actflag( ACT_PROTOTYPE ) )
         victim->pIndexData->level = value;
      ch->print( "Level set.\r\n" );
      return;
   }

   if( !str_cmp( arg2, "numattacks" ) )
   {
      if( value < 0 || value > 10 )
      {
         ch->print( "Attacks range is 0 to 10.\r\n" );
         return;
      }
      victim->numattacks = ( float )( value );
      if( victim->has_actflag( ACT_PROTOTYPE ) )
         victim->pIndexData->numattacks = ( float )( value );
      ch->print( "Numattacks set.\r\n" );
      return;
   }

   if( !str_cmp( arg2, "thac0" ) || !str_cmp( arg2, "thaco" ) )
   {
      if( !str_cmp( arg3, "x" ) || value > 20 )
         value = 21;

      victim->mobthac0 = value;
      if( victim->has_actflag( ACT_PROTOTYPE ) )
         victim->pIndexData->mobthac0 = value;
      ch->print( "Thac0 set.\r\n" );
      return;
   }

   if( !str_cmp( arg2, "name" ) )
   {
      if( arg3.empty(  ) )
      {
         ch->print( "Cannot set empty keywords!\r\n" );
         return;
      }

      STRFREE( victim->name );
      victim->name = STRALLOC( arg3.c_str(  ) );
      if( victim->has_actflag( ACT_PROTOTYPE ) )
      {
         STRFREE( victim->pIndexData->player_name );
         victim->pIndexData->player_name = QUICKLINK( victim->name );
      }
      ch->print( "Keywords set.\r\n" );
      return;
   }

   if( !str_cmp( arg2, "spec" ) || !str_cmp( arg2, "spec_fun" ) )
   {
      SPEC_FUN *specfun;

      if( !str_cmp( arg3, "none" ) )
      {
         victim->spec_fun = nullptr;
         victim->spec_funname.clear(  );
         ch->print( "Special function removed.\r\n" );
         if( victim->has_actflag( ACT_PROTOTYPE ) )
         {
            victim->pIndexData->spec_fun = nullptr;
            victim->pIndexData->spec_funname.clear(  );
         }
         return;
      }

      if( !( specfun = m_spec_lookup( arg3 ) ) )
      {
         ch->print( "No such function.\r\n" );
         return;
      }

      if( !validate_spec_fun( arg3 ) )
      {
         ch->printf( "%s is not a valid spec_fun for mobiles.\r\n", arg3.c_str(  ) );
         return;
      }

      victim->spec_fun = specfun;
      victim->spec_funname = arg3;
      if( victim->has_actflag( ACT_PROTOTYPE ) )
      {
         victim->pIndexData->spec_fun = victim->spec_fun;
         victim->pIndexData->spec_funname = arg3;
      }
      ch->print( "Special function set.\r\n" );
      return;
   }

   if( !str_cmp( arg2, "attack" ) )
   {
      if( argument.empty(  ) )
      {
         ch->print( "Usage: mset <victim> attack <flag> [flag]...\r\n" );
         return;
      }

      while( !argument.empty(  ) )
      {
         argument = one_argument( argument, arg3 );
         value = get_attackflag( arg3 );
         if( value < 0 || value >= MAX_ATTACK_TYPE )
            ch->printf( "Unknown flag: %s\r\n", arg3.c_str(  ) );
         else
            victim->toggle_attack( value );
      }
      if( victim->has_actflag( ACT_PROTOTYPE ) )
         victim->pIndexData->attacks = victim->get_attacks(  );
      ch->print( "Attacks set.\r\n" );
      return;
   }

   if( !str_cmp( arg2, "defense" ) )
   {
      if( argument.empty(  ) )
      {
         ch->print( "Usage: mset <victim> defense <flag> [flag]...\r\n" );
         return;
      }

      while( !argument.empty(  ) )
      {
         argument = one_argument( argument, arg3 );
         value = get_defenseflag( arg3 );
         if( value < 0 || value >= MAX_DEFENSE_TYPE )
            ch->printf( "Unknown flag: %s\r\n", arg3.c_str(  ) );
         else
            victim->toggle_defense( value );
      }
      if( victim->has_actflag( ACT_PROTOTYPE ) )
         victim->pIndexData->defenses = victim->get_defenses(  );
      ch->print( "Defenses set.\r\n" );
      return;
   }

   if( !str_cmp( arg2, "pos" ) )
   {
      value = get_npc_position( arg3 );

      if( value < 0 || value > POS_STANDING )
      {
         ch->print( "Invalid position.\r\n" );
         return;
      }
      victim->position = value;
      if( victim->has_actflag( ACT_PROTOTYPE ) )
         victim->pIndexData->position = victim->position;
      ch->print( "Position set.\r\n" );
      return;
   }

   if( !str_cmp( arg2, "defpos" ) )
   {
      value = get_npc_position( arg3 );

      if( value < 0 || value > POS_STANDING )
      {
         ch->print( "Invalid position.\r\n" );
         return;
      }
      victim->defposition = value;
      if( victim->has_actflag( ACT_PROTOTYPE ) )
         victim->pIndexData->defposition = victim->defposition;
      ch->print( "Default position set.\r\n" );
      return;
   }

   /*
    * save some finger-leather
    */
   if( !str_cmp( arg2, "hitdie" ) )
   {
      sscanf( arg3.c_str(  ), "%d %c %d %c %d", &num, &char1, &size, &char2, &plus );
      funcf( ch, do_mset, "%s hitnumdie %d", arg1.c_str(  ), num );
      funcf( ch, do_mset, "%s hitsizedie %d", arg1.c_str(  ), size );
      funcf( ch, do_mset, "%s hitplus %d", arg1.c_str(  ), plus );
      return;
   }

   /*
    * save some more finger-leather
    */
   if( !str_cmp( arg2, "damdie" ) )
   {
      sscanf( arg3.c_str(  ), "%d %c %d %c %d", &num, &char1, &size, &char2, &plus );
      funcf( ch, do_mset, "%s damnumdie %d", arg1.c_str(  ), num );
      funcf( ch, do_mset, "%s damsizedie %d", arg1.c_str(  ), size );
      funcf( ch, do_mset, "%s damplus %d", arg1.c_str(  ), plus );
      return;
   }

   if( !str_cmp( arg2, "hitnumdie" ) )
   {
      if( value < 0 || value > 32700 )
      {
         ch->print( "Number of hitpoint dice range is 0 to 32700.\r\n" );
         return;
      }
      if( victim->has_actflag( ACT_PROTOTYPE ) )
         victim->pIndexData->hitnodice = value;
      ch->print( "HP dice number set.\r\n" );
      return;
   }

   if( !str_cmp( arg2, "hitsizedie" ) )
   {
      if( value < 0 || value > 32700 )
      {
         ch->print( "Hitpoint dice size range is 0 to 32700.\r\n" );
         return;
      }
      if( victim->has_actflag( ACT_PROTOTYPE ) )
         victim->pIndexData->hitsizedice = value;
      ch->print( "HP dice size set.\r\n" );
      return;
   }

   if( !str_cmp( arg2, "hitplus" ) )
   {
      if( value < 0 || value > 32700 )
      {
         ch->print( "Hitpoint bonus range is 0 to 32700.\r\n" );
         return;
      }
      if( victim->has_actflag( ACT_PROTOTYPE ) )
         victim->pIndexData->hitplus = value;
      ch->print( "HP bonus set.\r\n" );
      return;
   }

   if( !str_cmp( arg2, "damnumdie" ) )
   {
      if( value < 0 || value > 100 )
      {
         ch->print( "Number of damage dice range is 0 to 100.\r\n" );
         return;
      }
      if( victim->has_actflag( ACT_PROTOTYPE ) )
         victim->pIndexData->damnodice = value;
      ch->print( "Damage dice number set.\r\n" );
      return;
   }

   if( !str_cmp( arg2, "damsizedie" ) )
   {
      if( value < 0 || value > 100 )
      {
         ch->print( "Damage dice size range is 0 to 100.\r\n" );
         return;
      }
      if( victim->has_actflag( ACT_PROTOTYPE ) )
         victim->pIndexData->damsizedice = value;
      ch->print( "Damage dice size set.\r\n" );
      return;
   }

   if( !str_cmp( arg2, "damplus" ) )
   {
      if( value < 0 || value > 1000 )
      {
         ch->print( "Damage bonus range is 0 to 1000.\r\n" );
         return;
      }

      if( victim->has_actflag( ACT_PROTOTYPE ) )
         victim->pIndexData->damplus = value;
      ch->print( "Damage bonus set.\r\n" );
      return;
   }

   /*
    * Generate usage message.
    */
   if( ch->substate == SUB_REPEATCMD )
   {
      ch->substate = SUB_RESTRICTED;
      interpret( ch, origarg );
      ch->substate = SUB_REPEATCMD;
      ch->last_cmd = do_mset;
   }
   else
      do_mset( ch, "" );
}

CMDF( do_oset )
{
   string arg1, arg2, arg3;
   obj_data *obj, *tmpobj;
   extra_descr_data *ed;
   bool lockobj;
   string origarg = argument;

   int value, tmp;

   ch->set_color( AT_PLAIN );

   if( ch->isnpc(  ) )
   {
      ch->print( "Mob's can't oset\r\n" );
      return;
   }

   if( !ch->desc )
   {
      ch->print( "You have no descriptor\r\n" );
      return;
   }

   switch ( ch->substate )
   {
      default:
         break;

      case SUB_OBJ_EXTRA:
         if( !ch->pcdata->dest_buf )
         {
            ch->print( "Fatal error: report to Samson.\r\n" );
            bug( "%s: sub_obj_extra: nullptr ch->pcdata->dest_buf", __func__ );
            ch->substate = SUB_NONE;
            return;
         }

         /*
          * hopefully the object didn't get extracted...
          * if you're REALLY paranoid, you could always go through
          * the object and index-object lists, searching through the
          * extra_descr lists for a matching pointer...
          */
         ed = ( extra_descr_data * ) ch->pcdata->dest_buf;
         ed->desc = ch->copy_buffer(  );
         tmpobj = ( obj_data * ) ch->pcdata->spare_ptr;
         ch->stop_editing(  );
         ch->pcdata->dest_buf = tmpobj;
         ch->substate = ch->tempnum;
         return;

      case SUB_OBJ_LONG:
         if( !ch->pcdata->dest_buf )
         {
            ch->print( "Fatal error: report to Samson.\r\n" );
            bug( "%s: sub_obj_long: nullptr ch->pcdata->dest_buf", __func__ );
            ch->substate = SUB_NONE;
            return;
         }
         obj = ( obj_data * ) ch->pcdata->dest_buf;
         if( obj && obj->extracted(  ) )
         {
            ch->print( "Your object was extracted!\r\n" );
            ch->stop_editing(  );
            return;
         }
         STRFREE( obj->objdesc );
         obj->objdesc = ch->copy_buffer( true );
         if( obj->extra_flags.test( ITEM_PROTOTYPE ) )
         {
            if( can_omodify( ch, obj ) )
            {
               STRFREE( obj->pIndexData->objdesc );
               obj->pIndexData->objdesc = QUICKLINK( obj->objdesc );
            }
         }
         tmpobj = ( obj_data * ) ch->pcdata->spare_ptr;
         ch->stop_editing(  );
         ch->substate = ch->tempnum;
         ch->pcdata->dest_buf = tmpobj;
         return;

      case SUB_EDIT_ABORT:
         ch->substate = SUB_NONE;
         ch->print( "Aborting description.\r\n" );
         return;
   }

   obj = nullptr;
   smash_tilde( argument );

   if( ch->substate == SUB_REPEATCMD )
   {
      obj = ( obj_data * ) ch->pcdata->dest_buf;
      if( !obj )
      {
         ch->print( "Your object was extracted!\r\n" );
         argument = "done";
      }
      if( argument.empty(  ) || !str_cmp( argument, " " ) || !str_cmp( argument, "stat" ) )
      {
         if( obj )
            funcf( ch, do_ostat, "%d", obj->pIndexData->vnum );
         else
            ch->print( "No object selected.  Type '?' for help.\r\n" );
         return;
      }
      if( !str_cmp( argument, "done" ) || !str_cmp( argument, "off" ) )
      {
         if( ch->pcdata->dest_buf )
            RelDestroy( relOSET_ON, ch, ch->pcdata->dest_buf );
         ch->print( "Oset mode off.\r\n" );
         ch->substate = SUB_NONE;
         ch->pcdata->dest_buf = nullptr;
         STRFREE( ch->pcdata->subprompt );
         return;
      }
   }

   if( obj )
   {
      lockobj = true;
      arg1 = obj->name;
      argument = one_argument( argument, arg2 );
      arg3 = argument;
   }
   else
   {
      lockobj = false;
      argument = one_argument( argument, arg1 );
      argument = one_argument( argument, arg2 );
      arg3 = argument;
   }

   if( !str_cmp( arg1, "on" ) )
   {
      ch->print( "Syntax: oset <object|vnum> on.\r\n" );
      return;
   }

   if( arg1.empty(  ) || arg2.empty(  ) || !str_cmp( arg1, "?" ) || !str_cmp( arg1, "help" ) )
   {
      if( ch->substate == SUB_REPEATCMD )
      {
         if( obj )
            ch->print( "Syntax: <field>  <value>\r\n\r\n" );
         else
            ch->print( "Syntax: <object> <field>  <value>\r\n\r\n" );
      }
      else
         ch->print( "Syntax: oset <object> <field>  <value>\r\n\r\n" );
      ch->print( "Field being one of:\r\n" );
      ch->print( "  flags wear level weight cost rent limit timer\r\n" );
      ch->print( "  name short long ed rmed actiondesc\r\n" );
      ch->print( "  type value0 value1 value2 value3 value4 value5 value6 value7\r\n" );
      ch->print( "  affect rmaffect layers\r\n" );
      ch->print( "For weapons:             For armor:\r\n" );
      ch->print( "  weapontype condition     ac condition\r\n" );
      ch->print( "For scrolls, potions and pills:\r\n" );
      ch->print( "  slevel spell1 spell2 spell3\r\n" );
      ch->print( "For wands and staves:\r\n" );
      ch->print( "  slevel spell maxcharges charges\r\n" );
      ch->print( "For containers:          For levers and switches:\r\n" );
      ch->print( "  cflags key capacity      tflags\r\n" );
      return;
   }

   if( !obj && ch->get_trust(  ) < LEVEL_GOD )
   {
      if( !( obj = ch->get_obj_here( arg1 ) ) )
      {
         ch->print( "You can't find that here.\r\n" );
         return;
      }
   }
   else if( !obj )
   {
      if( !( obj = ch->get_obj_world( arg1 ) ) )
      {
         ch->print( "There is nothing like that in all the lands.\r\n" );
         return;
      }
   }
   if( lockobj )
      ch->pcdata->dest_buf = obj;

   obj->separate(  );
   value = atoi( arg3.c_str(  ) );

   if( !can_omodify( ch, obj ) )
      return;

   if( !str_cmp( arg2, "on" ) )
   {
      ch->CHECK_SUBRESTRICTED(  );
      ch->printf( "Oset mode on. (Editing '%s' vnum %d).\r\n", obj->name, obj->pIndexData->vnum );
      ch->substate = SUB_REPEATCMD;
      ch->pcdata->dest_buf = obj;
      stralloc_printf( &ch->pcdata->subprompt, "<&COset &W#%d&w> %%i", obj->pIndexData->vnum );
      RelCreate( relOSET_ON, ch, obj );
      return;
   }

   if( !str_cmp( arg2, "owner" ) )
   {
      if( !ch->is_imp(  ) )
      {
         do_oset( ch, "" );
         return;
      }

      STRFREE( obj->owner );
      obj->owner = STRALLOC( arg3.c_str(  ) );
      ch->print( "Object owner set.\r\n" );
      return;
   }

   if( !str_cmp( arg2, "name" ) )
   {
      bool proto = false;

      if( arg3.empty(  ) )
      {
         ch->print( "Cannot set empty keywords!\r\n" );
         return;
      }

      if( obj->extra_flags.test( ITEM_PROTOTYPE ) )
         proto = true;
      STRFREE( obj->name );
      obj->name = STRALLOC( arg3.c_str(  ) );
      if( proto )
      {
         STRFREE( obj->pIndexData->name );
         obj->pIndexData->name = QUICKLINK( obj->name );
      }
      ch->print( "Object keywords set.\r\n" );
      return;
   }

   if( !str_cmp( arg2, "short" ) )
   {
      STRFREE( obj->short_descr );
      obj->short_descr = STRALLOC( arg3.c_str(  ) );

      if( obj->extra_flags.test( ITEM_PROTOTYPE ) )
      {
         STRFREE( obj->pIndexData->short_descr );
         obj->pIndexData->short_descr = QUICKLINK( obj->short_descr );
      }
      else
         /*
          * Feature added by Narn, Apr/96 
          * * If the item is not proto, add the word 'rename' to the keywords
          * * if it is not already there.
          */
      {
         if( str_infix( "rename", obj->name ) )
            stralloc_printf( &obj->name, "%s %s", obj->name, "rename" );
      }
      ch->print( "Object short description set.\r\n" );
      return;
   }

   if( !str_cmp( arg2, "long" ) )
   {
      if( !arg3.empty(  ) )
      {
         STRFREE( obj->objdesc );
         obj->objdesc = STRALLOC( arg3.c_str(  ) );
         if( obj->extra_flags.test( ITEM_PROTOTYPE ) )
         {
            STRFREE( obj->pIndexData->objdesc );
            obj->pIndexData->objdesc = QUICKLINK( obj->objdesc );
            return;
         }
         ch->print( "Object long description set.\r\n" );
         return;
      }
      ch->CHECK_SUBRESTRICTED(  );
      if( ch->substate == SUB_REPEATCMD )
         ch->tempnum = SUB_REPEATCMD;
      else
         ch->tempnum = SUB_NONE;
      if( lockobj )
         ch->pcdata->spare_ptr = obj;
      else
         ch->pcdata->spare_ptr = nullptr;
      ch->substate = SUB_OBJ_LONG;
      ch->pcdata->dest_buf = obj;
      if( !obj->objdesc || obj->objdesc[0] == '\0' )
         obj->objdesc = STRALLOC( "" );
      ch->editor_desc_printf( "Object long desc, vnum %d (%s).", obj->pIndexData->vnum, obj->short_descr );
      ch->start_editing( obj->objdesc );
      return;
   }

   if( !str_cmp( arg2, "value0" ) || !str_cmp( arg2, "v0" ) )
   {
      obj->value[0] = value;
      if( obj->extra_flags.test( ITEM_PROTOTYPE ) )
         obj->pIndexData->value[0] = value;
      ch->print( "Object value set.\r\n" );
      return;
   }

   if( !str_cmp( arg2, "value1" ) || !str_cmp( arg2, "v1" ) )
   {
      obj->value[1] = value;
      if( obj->extra_flags.test( ITEM_PROTOTYPE ) )
         obj->pIndexData->value[1] = value;
      ch->print( "Object value set.\r\n" );
      return;
   }

   if( !str_cmp( arg2, "value2" ) || !str_cmp( arg2, "v2" ) )
   {
      obj->value[2] = value;
      if( obj->extra_flags.test( ITEM_PROTOTYPE ) )
      {
         obj->pIndexData->value[2] = value;
         if( obj->item_type == ITEM_WEAPON && value != 0 )
            obj->value[2] = obj->pIndexData->value[1] * obj->pIndexData->value[2];
      }
      ch->print( "Object value set.\r\n" );
      return;
   }

   if( !str_cmp( arg2, "value3" ) || !str_cmp( arg2, "v3" ) )
   {
      if( obj->item_type == ITEM_ARMOR && ( value < 0 || value >= TATP_MAX ) )
      {
         ch->printf( "Value is out of range for armor type. Range is 0 to %d\r\n", TATP_MAX );
         return;
      }

      obj->value[3] = value;

      if( obj->extra_flags.test( ITEM_PROTOTYPE ) )
         obj->pIndexData->value[3] = value;

      /*
       * Automatic armor stat generation, v3 and v4 must both be non-zero 
       */
      if( obj->item_type == ITEM_ARMOR && obj->value[3] > 0 && obj->value[4] > 0 )
      {
         ch->print( "Updating generated armor settings.\r\n" );
         obj->armorgen(  );
      }
      ch->print( "Object value set.\r\n" );
      return;
   }

   if( !str_cmp( arg2, "value4" ) || !str_cmp( arg2, "v4" ) )
   {
      if( obj->item_type == ITEM_ARMOR && ( value < 0 || value >= TMAT_MAX ) )
      {
         ch->printf( "Value is out of range for material type. Range is 0 to %d\r\n", TMAT_MAX );
         return;
      }

      obj->value[4] = value;

      if( obj->extra_flags.test( ITEM_PROTOTYPE ) )
         obj->pIndexData->value[4] = value;

      /*
       * Automatic armor stat generation, v3 and v4 must both be non-zero 
       */
      if( obj->item_type == ITEM_ARMOR && obj->value[3] > 0 && obj->value[4] > 0 )
      {
         ch->print( "Updating generated armor settings.\r\n" );
         obj->armorgen(  );
      }
      ch->print( "Object value set.\r\n" );
      return;
   }

   if( !str_cmp( arg2, "value5" ) || !str_cmp( arg2, "v5" ) )
   {
      if( obj->item_type == ITEM_CORPSE_PC && !ch->is_imp(  ) )
      {
         ch->print( "Cannot alter the skeleton value.\r\n" );
         return;
      }
      obj->value[5] = value;
      if( obj->extra_flags.test( ITEM_PROTOTYPE ) )
         obj->pIndexData->value[5] = value;
      ch->print( "Object value set.\r\n" );
      return;
   }

   if( !str_cmp( arg2, "value6" ) || !str_cmp( arg2, "v6" ) )
   {
      obj->value[6] = value;
      if( obj->extra_flags.test( ITEM_PROTOTYPE ) )
         obj->pIndexData->value[6] = value;
      ch->print( "Object value set.\r\n" );
      return;
   }

   if( !str_cmp( arg2, "value7" ) || !str_cmp( arg2, "v7" ) )
   {
      obj->value[7] = value;
      if( obj->extra_flags.test( ITEM_PROTOTYPE ) )
         obj->pIndexData->value[7] = value;
      ch->print( "Object value set.\r\n" );
      return;
   }

   if( !str_cmp( arg2, "value8" ) || !str_cmp( arg2, "v8" ) )
   {
      if( obj->item_type == ITEM_WEAPON && ( value < 0 || value >= TWTP_MAX ) )
      {
         ch->printf( "Value is out of range for weapon type. Range is 0 to %d\r\n", TWTP_MAX );
         return;
      }

      obj->value[8] = value;
      if( obj->extra_flags.test( ITEM_PROTOTYPE ) )
         obj->pIndexData->value[8] = value;

      if( obj->item_type == ITEM_WEAPON && obj->value[8] > 0 && obj->value[9] > 0 && obj->value[10] > 0 )
      {
         ch->print( "Updating generated weapon settings.\r\n" );
         obj->weapongen(  );
      }
      ch->print( "Object value set.\r\n" );
      return;
   }

   if( !str_cmp( arg2, "value9" ) || !str_cmp( arg2, "v9" ) )
   {
      if( obj->item_type == ITEM_WEAPON && ( value < 0 || value >= TMAT_MAX ) )
      {
         ch->printf( "Value is out of range for material type. Range is 0 to %d\r\n", TMAT_MAX );
         return;
      }

      obj->value[9] = value;
      if( obj->extra_flags.test( ITEM_PROTOTYPE ) )
         obj->pIndexData->value[9] = value;

      if( obj->item_type == ITEM_WEAPON && obj->value[8] > 0 && obj->value[9] > 0 && obj->value[10] > 0 )
      {
         ch->print( "Updating generated weapon settings.\r\n" );
         obj->weapongen(  );
      }
      ch->print( "Object value set.\r\n" );
      return;
   }

   if( !str_cmp( arg2, "value10" ) || !str_cmp( arg2, "v10" ) )
   {
      if( obj->item_type == ITEM_WEAPON && ( value < 0 || value >= TQUAL_MAX ) )
      {
         ch->printf( "Value is out of range for quality type. Range is 0 to %d\r\n", TQUAL_MAX );
         return;
      }

      obj->value[10] = value;
      if( obj->extra_flags.test( ITEM_PROTOTYPE ) )
         obj->pIndexData->value[10] = value;

      if( obj->item_type == ITEM_WEAPON && obj->value[8] > 0 && obj->value[9] > 0 && obj->value[10] > 0 )
      {
         ch->print( "Updating generated weapon settings.\r\n" );
         obj->weapongen(  );
      }
      ch->print( "Object value set.\r\n" );
      return;
   }

   if( !str_cmp( arg2, "type" ) )
   {
      if( argument.empty(  ) )
      {
         ch->print( "Usage: oset <object> type <type>\r\n" );
         return;
      }
      value = get_otype( argument );
      if( value < 1 )
      {
         ch->printf( "Unknown type: %s\r\n", arg3.c_str(  ) );
         return;
      }
      obj->item_type = ( short )value;
      if( obj->extra_flags.test( ITEM_PROTOTYPE ) )
         obj->pIndexData->item_type = obj->item_type;
      ch->printf( "Object type set to %s.\r\n", arg3.c_str(  ) );
      return;
   }

   if( !str_cmp( arg2, "flags" ) )
   {
      if( argument.empty(  ) )
      {
         ch->print( "Usage: oset <object> flags <flag> [flag]...\r\n" );
         return;
      }

      while( !argument.empty(  ) )
      {
         argument = one_argument( argument, arg3 );
         value = get_oflag( arg3 );
         if( value < 0 || value >= MAX_ITEM_FLAG )
            ch->printf( "Unknown flag: %s\r\n", arg3.c_str(  ) );
         else
         {
            if( value == ITEM_PROTOTYPE && ch->get_trust(  ) < sysdata->level_modify_proto )
               ch->print( "You cannot change the prototype flag.\r\n" );
            else
            {
               obj->extra_flags.flip( value );
               if( value == ITEM_PROTOTYPE )
                  obj->pIndexData->extra_flags = obj->extra_flags;
            }
         }
      }
      if( obj->extra_flags.test( ITEM_PROTOTYPE ) )
         obj->pIndexData->extra_flags = obj->extra_flags;
      ch->print( "Object extra flags set.\r\n" );
      return;
   }

   if( !str_cmp( arg2, "wear" ) )
   {
      if( argument.empty(  ) )
      {
         ch->print( "Usage: oset <object> wear <flag> [flag]...\r\n" );
         return;
      }

      while( !argument.empty(  ) )
      {
         argument = one_argument( argument, arg3 );
         value = get_wflag( arg3 );
         if( value < 0 || value >= MAX_WEAR_FLAG )
            ch->printf( "Unknown flag: %s\r\n", arg3.c_str(  ) );
         else
            obj->wear_flags.flip( value );
      }
      if( obj->extra_flags.test( ITEM_PROTOTYPE ) )
         obj->pIndexData->wear_flags = obj->wear_flags;
      ch->print( "Object wear flags set.\r\n" );
      return;
   }

   if( !str_cmp( arg2, "level" ) )
   {
      obj->level = value;
      ch->print( "Object level set. Note: This is not permanently changed.\r\n" );
      return;
   }

   if( !str_cmp( arg2, "weight" ) )
   {
      obj->weight = value;
      if( obj->extra_flags.test( ITEM_PROTOTYPE ) )
         obj->pIndexData->weight = value;
      ch->print( "Object weight set.\r\n" );
      return;
   }

   if( !str_cmp( arg2, "cost" ) )
   {
      obj->cost = value;
      if( obj->extra_flags.test( ITEM_PROTOTYPE ) )
         obj->pIndexData->cost = value;
      ch->print( "Object cost set.\r\n" );
      return;
   }

   if( !str_cmp( arg2, "ego" ) )
   {
      if( obj->extra_flags.test( ITEM_PROTOTYPE ) )
      {
         obj->pIndexData->ego = value;
         if( value == -2 )
            obj->ego = obj->pIndexData->set_ego(  );
         ch->print( "&GObject ego set.\r\n" );
         if( obj->ego == -2 )
            ch->print( "&YWARNING: This object exceeds allowable ego specs.\r\n" );
      }
      else
      {
         obj->ego = value;
         if( value == -2 )
            obj->ego = obj->pIndexData->set_ego(  );
         ch->print( "&GObject ego set.\r\n" );
         if( obj->ego == -2 )
            ch->print( "&YWARNING: This object exceeds allowable ego specs.\r\n" );
      }
      return;
   }

   if( !str_cmp( arg2, "limit" ) )
   {
      if( obj->extra_flags.test( ITEM_PROTOTYPE ) )
      {
         obj->pIndexData->limit = value;
         ch->print( "Object limit set.\r\n" );
      }
      else
         ch->print( "Item must have prototype flag to set this value.\r\n" );
      return;
   }

   if( !str_cmp( arg2, "layers" ) )
   {
      if( obj->extra_flags.test( ITEM_PROTOTYPE ) )
      {
         obj->pIndexData->layers = value;
         ch->print( "Object layers set.\r\n" );
      }
      else
         ch->print( "Item must have prototype flag to set this value.\r\n" );
      return;
   }

   if( !str_cmp( arg2, "timer" ) )
   {
      obj->timer = value;
      ch->print( "Object timer set.\r\n" );
      return;
   }

   if( !str_cmp( arg2, "prizeowner" ) || !str_cmp( arg2, "owner" ) )
   {
      STRFREE( obj->owner );
      obj->owner = STRALLOC( arg3.c_str(  ) );
      ch->print( "Object prize ownership changed.\r\n" );
      return;
   }

   if( !str_cmp( arg2, "actiondesc" ) )
   {
      if( strstr( arg3.c_str(  ), "%n" ) || strstr( arg3.c_str(  ), "%d" ) || strstr( arg3.c_str(  ), "%l" ) )
      {
         ch->print( "Illegal characters!\r\n" );
         return;
      }
      STRFREE( obj->action_desc );
      obj->action_desc = STRALLOC( arg3.c_str(  ) );
      if( obj->extra_flags.test( ITEM_PROTOTYPE ) )
      {
         STRFREE( obj->pIndexData->action_desc );
         obj->pIndexData->action_desc = QUICKLINK( obj->action_desc );
      }
      ch->print( "Object action description set.\r\n" );
      return;
   }

   /*
    * Crash fix and name support by Shaddai 
    */
   if( !str_cmp( arg2, "effect" ) )
   {
      affect_data *paf;
      bitset < MAX_RIS_FLAG > risabit;
      short loc;
      bool found = false;

      risabit.reset(  );

      argument = one_argument( argument, arg2 );
      if( arg2.empty(  ) || argument.empty(  ) )
      {
         ch->print( "Usage: oset <object> effect <field> <value>\r\n" );
         return;
      }
      loc = get_atype( arg2 );
      if( loc < 1 )
      {
         ch->printf( "Unknown field: %s\r\n", arg2.c_str(  ) );
         return;
      }
      if( loc == APPLY_AFFECT )
      {
         argument = one_argument( argument, arg3 );
         if( loc == APPLY_AFFECT )
         {
            value = get_aflag( arg3 );

            if( value < 0 || value >= MAX_AFFECTED_BY )
               ch->printf( "Unknown affect: %s\r\n", arg3.c_str(  ) );
            else
               found = true;
         }
      }
      else if( loc == APPLY_RESISTANT || loc == APPLY_IMMUNE || loc == APPLY_SUSCEPTIBLE || loc == APPLY_ABSORB )
      {
         string flag;

         while( !arg3.empty(  ) )
         {
            arg3 = one_argument( arg3, flag );
            value = get_risflag( flag );

            if( value < 0 || value >= MAX_RIS_FLAG )
               ch->printf( "Unknown flag: %s\r\n", flag.c_str(  ) );
            else
            {
               risabit.set( value );
               found = true;
            }
         }
      }
      else if( loc == APPLY_WEAPONSPELL
               || loc == APPLY_WEARSPELL || loc == APPLY_REMOVESPELL || loc == APPLY_STRIPSN || loc == APPLY_RECURRINGSPELL || loc == APPLY_EAT_SPELL )
      {
         argument = one_argument( argument, arg3 );
         value = skill_lookup( arg3 );

         if( !IS_VALID_SN( value ) )
            ch->printf( "Invalid spell: %s", arg3.c_str(  ) );
         else
            found = true;
      }
      else
      {
         value = atoi( arg3.c_str(  ) );
         found = true;
      }
      if( !found )
         return;

      paf = new affect_data;
      paf->type = -1;
      paf->duration = -1;
      paf->location = loc;
      paf->modifier = value;
      paf->rismod = risabit;
      paf->bit = 0;
      if( obj->extra_flags.test( ITEM_PROTOTYPE ) )
      {
         if( loc != APPLY_WEARSPELL && loc != APPLY_REMOVESPELL && loc != APPLY_STRIPSN && loc != APPLY_WEAPONSPELL )
         {
            list < char_data * >::iterator ich;
            list < obj_data * >::iterator iobj;

            for( ich = charlist.begin(  ); ich != charlist.end(  ); ++ich )
            {
               char_data *vch = *ich;

               for( iobj = vch->carrying.begin(  ); iobj != vch->carrying.end(  ); ++iobj )
               {
                  obj_data *eq = *iobj;

                  if( eq->pIndexData == obj->pIndexData && eq->wear_loc != WEAR_NONE )
                     vch->affect_modify( paf, true );
               }
            }
         }
         obj->pIndexData->affects.push_back( paf );
      }
      else
         obj->affects.push_back( paf );
      ++top_affect;
      ch->print( "Object affect added.\r\n" );
      return;
   }

   if( !str_cmp( arg2, "rmaffect" ) )
   {
      list < affect_data * >::iterator paf;
      short loc, count;

      if( argument.empty(  ) )
      {
         ch->print( "Usage: oset <object> rmaffect <affect#>\r\n" );
         return;
      }
      loc = atoi( argument.c_str(  ) );
      if( loc < 1 )
      {
         ch->print( "Invalid number.\r\n" );
         return;
      }

      count = 0;

      if( obj->extra_flags.test( ITEM_PROTOTYPE ) )
      {
         obj_index *pObjIndex;

         pObjIndex = obj->pIndexData;
         for( paf = pObjIndex->affects.begin(  ); paf != pObjIndex->affects.end(  ); )
         {
            affect_data *aff = *paf;
            ++paf;

            if( ++count == loc )
            {
               pObjIndex->affects.remove( aff );
               deleteptr( aff );
               ch->print( "Object affect removed.\r\n" );
               --top_affect;
               return;
            }
         }
         ch->print( "Object affect not found.\r\n" );
         return;
      }

      if( obj->extra_flags.test( ITEM_PROTOTYPE ) )
      {
         obj_index *pObjIndex;

         pObjIndex = obj->pIndexData;
         for( paf = pObjIndex->affects.begin(  ); paf != pObjIndex->affects.end(  ); )
         {
            affect_data *aff = *paf;
            ++paf;

            if( ++count == loc )
            {
               pObjIndex->affects.remove( aff );

               list < char_data * >::iterator ich;
               list < obj_data * >::iterator iobj;

               for( ich = charlist.begin(  ); ich != charlist.end(  ); ++ich )
               {
                  char_data *vch = *ich;

                  for( iobj = vch->carrying.begin(  ); iobj != vch->carrying.end(  ); ++iobj )
                  {
                     obj_data *eq = *iobj;

                     if( eq->pIndexData == obj->pIndexData && eq->wear_loc != WEAR_NONE )
                        vch->affect_modify( aff, false );
                  }
               }
               deleteptr( aff );
               ch->print( "Object affect removed.\r\n" );
               --top_affect;
               return;
            }
         }
         ch->print( "Object affect not found.\r\n" );
         return;
      }
      else
      {
         for( paf = obj->affects.begin(  ); paf != obj->affects.end(  ); )
         {
            affect_data *aff = *paf;
            ++paf;

            if( ++count == loc )
            {
               obj->affects.remove( aff );
               deleteptr( aff );
               ch->print( "Object affect removed.\r\n" );
               --top_affect;
               return;
            }
         }
         ch->print( "Object affect not found.\r\n" );
         return;
      }
   }

   if( !str_cmp( arg2, "ed" ) )
   {
      if( arg3.empty(  ) )
      {
         ch->print( "Syntax: oset <object> ed <keywords>\r\n" );
         return;
      }
      ch->CHECK_SUBRESTRICTED(  );
      if( obj->timer )
      {
         ch->print( "It's not safe to edit an extra description on an object with a timer.\r\nTurn it off first.\r\n" );
         return;
      }
      if( obj->item_type == ITEM_PAPER )
      {
         ch->print( "You can not add an extra description to a note paper at the moment.\r\n" );
         return;
      }
      if( obj->extra_flags.test( ITEM_PROTOTYPE ) )
         ed = set_extra_descr( obj->pIndexData, arg3 );
      else
         ed = set_extra_descr( obj, arg3 );
      if( ch->substate == SUB_REPEATCMD )
         ch->tempnum = SUB_REPEATCMD;
      else
         ch->tempnum = SUB_NONE;
      if( lockobj )
         ch->pcdata->spare_ptr = obj;
      else
         ch->pcdata->spare_ptr = nullptr;
      ch->substate = SUB_OBJ_EXTRA;
      ch->pcdata->dest_buf = ed;
      ch->editor_desc_printf( "Extra description '%s' on object vnum %d (%s).", arg3.c_str(  ), obj->pIndexData->vnum, obj->short_descr );
      ch->start_editing( ed->desc );
      return;
   }

   if( !str_cmp( arg2, "rmed" ) )
   {
      if( arg3.empty(  ) )
      {
         ch->print( "Syntax: oset <object> rmed <keywords>\r\n" );
         return;
      }
      if( obj->extra_flags.test( ITEM_PROTOTYPE ) )
      {
         if( ( ed = get_extra_descr( arg3, obj->pIndexData ) ) )
         {
            obj->pIndexData->extradesc.remove( ed );
            deleteptr( ed );
            ch->print( "Deleted.\r\n" );
         }
         else
            ch->print( "Not found.\r\n" );
         return;
      }
      if( ( ed = get_extra_descr( arg3, obj ) ) )
      {
         obj->extradesc.remove( ed );
         ch->print( "Deleted.\r\n" );
      }
      else
         ch->print( "Not found.\r\n" );
      return;
   }

   /*
    * save some finger-leather
    */
   if( !str_cmp( arg2, "ris" ) )
   {
      funcf( ch, do_oset, "%s affect resistant %s", arg1.c_str(  ), arg3.c_str(  ) );
      funcf( ch, do_oset, "%s affect immune %s", arg1.c_str(  ), arg3.c_str(  ) );
      funcf( ch, do_oset, "%s affect susceptible %s", arg1.c_str(  ), arg3.c_str(  ) );
      return;
   }

   if( !str_cmp( arg2, "r" ) )
   {
      funcf( ch, do_oset, "%s affect resistant %s", arg1.c_str(  ), arg3.c_str(  ) );
      return;
   }

   if( !str_cmp( arg2, "i" ) )
   {
      funcf( ch, do_oset, "%s affect immune %s", arg1.c_str(  ), arg3.c_str(  ) );
      return;
   }
   if( !str_cmp( arg2, "s" ) )
   {
      funcf( ch, do_oset, "%s affect susceptible %s", arg1.c_str(  ), arg3.c_str(  ) );
      return;
   }

   if( !str_cmp( arg2, "ri" ) )
   {
      funcf( ch, do_oset, "%s affect resistant %s", arg1.c_str(  ), arg3.c_str(  ) );
      funcf( ch, do_oset, "%s affect immune %s", arg1.c_str(  ), arg3.c_str(  ) );
      return;
   }

   if( !str_cmp( arg2, "rs" ) )
   {
      funcf( ch, do_oset, "%s affect resistant %s", arg1.c_str(  ), arg3.c_str(  ) );
      funcf( ch, do_oset, "%s affect susceptible %s", arg1.c_str(  ), arg3.c_str(  ) );
      return;
   }

   if( !str_cmp( arg2, "is" ) )
   {
      funcf( ch, do_oset, "%s affect immune %s", arg1.c_str(  ), arg3.c_str(  ) );
      funcf( ch, do_oset, "%s affect susceptible %s", arg1.c_str(  ), arg3.c_str(  ) );
      return;
   }

   /*
    * Make it easier to set special object values by name than number
    * - Thoric
    */
   tmp = -1;
   switch ( obj->item_type )
   {
      default:
         break;

      case ITEM_TRAP:
         if( !str_cmp( arg2, "charges" ) )
            tmp = 0;
         if( !str_cmp( arg2, "level" ) )
            tmp = 2;
         if( !str_cmp( arg2, "mindamage" ) )
            tmp = 4;
         if( !str_cmp( arg2, "maxdamage" ) )
            tmp = 5;

         if( !str_cmp( arg2, "type" ) )
         {
            value = -1;
            for( size_t x = 0; x < sizeof( trap_types ) / sizeof( trap_types[0] ); ++x )
               if( !str_cmp( arg3, trap_types[x] ) )
                  value = x;
            if( value < 0 )
            {
               ch->print( "Unknown trap type.\r\n" );
               return;
            }
            tmp = 1;
            break;
         }

         if( !str_cmp( arg2, "flags" ) )
         {
            int tmpval = 0;

            tmp = 3;
            argument = arg3;

            if( is_number( argument ) )
            {
               int bv = atoi( argument.c_str() );

               if( bv < 0 || bv > BV31 )
               {
                  ch->printf( "Invalid bitvector value %d. Edit cancelled.\r\n", bv );
                  return;
               }
               tmpval = bv;
            }
            else
            {
               while( !argument.empty(  ) )
               {
                  argument = one_argument( argument, arg3 );
                  value = get_trapflag( arg3 );
                  if( value < 0 || value > 31 )
                     ch->printf( "Invalid trap flag %s\r\n", arg3.c_str(  ) );
                  else
                     tmpval += ( 1 << value );
               }
            }
            value = tmpval;
         }

      case ITEM_CAMPGEAR:
         if( !str_cmp( arg2, "type" ) )
         {
            value = -1;
            for( size_t x = 0; x < sizeof( campgear ) / sizeof( campgear[0] ); ++x )
               if( !str_cmp( arg3, campgear[x] ) )
                  value = x;
            if( value < 0 )
            {
               ch->print( "Unknown camping gear type.\r\n" );
               return;
            }
            tmp = 0;
            break;
         }

      case ITEM_ORE:
         if( !str_cmp( arg2, "type" ) )
         {
            value = -1;
            for( size_t x = 0; x < sizeof( ores ) / sizeof( ores[0] ); ++x )
               if( !str_cmp( arg3, ores[x] ) )
                  value = x;
            if( value < 0 )
            {
               ch->print( "Unknown ore type.\r\n" );
               return;
            }
            tmp = 0;
            break;
         }

      case ITEM_FURNITURE:
         if( !str_cmp( arg2, "flags" ) )
         {
            int tmpval = 0;

            tmp = 2;
            argument = arg3;

            if( is_number( argument ) )
            {
               int bv = atoi( argument.c_str() );

               if( bv < 0 || bv > BV31 )
               {
                  ch->printf( "Invalid bitvector value %d. Edit cancelled.\r\n", bv );
                  return;
               }
               tmpval = bv;
            }
            else
            {
               while( !argument.empty(  ) )
               {
                  argument = one_argument( argument, arg3 );
                  value = get_furnitureflag( arg3 );
                  if( value < 0 || value > 31 )
                     ch->printf( "Invalid furniture flag %s\r\n", arg3.c_str(  ) );
                  else
                     tmpval += ( 1 << value );
               }
            }
            value = tmpval;
         }

      case ITEM_PROJECTILE:
         if( !str_cmp( arg2, "projectiletype" ) )
         {
            value = -1;
            for( size_t x = 0; x < sizeof( projectiles ) / sizeof( projectiles[0] ); ++x )
               if( !str_cmp( arg3, projectiles[x] ) )
                  value = x;
            if( value < 0 )
            {
               ch->print( "Unknown projectile type.\r\n" );
               return;
            }
            tmp = 4;
            break;
         }

         if( !str_cmp( arg2, "damtype" ) )
         {
            value = -1;
            for( size_t x = 0; x < sizeof( attack_table ) / sizeof( attack_table[0] ); ++x )
               if( !str_cmp( arg3, attack_table[x] ) )
                  value = x;
            if( value < 0 )
            {
               ch->print( "Unknown damage type.\r\n" );
               return;
            }
            tmp = 3;
            break;
         }
         break;

      case ITEM_WEAPON:
         if( !str_cmp( arg2, "weapontype" ) )
         {
            value = -1;
            for( size_t x = 0; x < sizeof( weapon_skills ) / sizeof( weapon_skills[0] ); ++x )
               if( !str_cmp( arg3, weapon_skills[x] ) )
                  value = x;
            if( value < 0 )
            {
               ch->print( "Unknown weapon type.\r\n" );
               return;
            }
            tmp = 4;
            break;
         }

         if( !str_cmp( arg2, "damtype" ) )
         {
            value = -1;
            for( size_t x = 0; x < sizeof( attack_table ) / sizeof( attack_table[0] ); ++x )
               if( !str_cmp( arg3, attack_table[x] ) )
                  value = x;
            if( value < 0 )
            {
               ch->print( "Unknown damage type.\r\n" );
               return;
            }
            tmp = 3;
            break;
         }

         if( !str_cmp( arg2, "condition" ) )
            tmp = 0;
         if( !str_cmp( arg2, "damage" ) )
            tmp = 6;
         if( !str_cmp( arg2, "sockets" ) )
            tmp = 7;
         break;

      case ITEM_ARMOR:
         if( !str_cmp( arg2, "condition" ) )
            tmp = 0;
         if( !str_cmp( arg2, "ac" ) )
            tmp = 1;
         if( !str_cmp( arg2, "sockets" ) )
            tmp = 2;
         break;

      case ITEM_SALVE:
         if( !str_cmp( arg2, "slevel" ) )
            tmp = 0;
         if( !str_cmp( arg2, "maxdoses" ) )
            tmp = 1;
         if( !str_cmp( arg2, "doses" ) )
            tmp = 2;
         if( !str_cmp( arg2, "delay" ) )
            tmp = 3;
         if( !str_cmp( arg2, "spell1" ) )
            tmp = 4;
         if( !str_cmp( arg2, "spell2" ) )
            tmp = 5;
         if( tmp >= 4 && tmp <= 5 )
            value = skill_lookup( arg3 );
         break;

      case ITEM_SCROLL:
      case ITEM_POTION:
      case ITEM_PILL:
         if( !str_cmp( arg2, "slevel" ) )
            tmp = 0;
         if( !str_cmp( arg2, "spell1" ) )
            tmp = 1;
         if( !str_cmp( arg2, "spell2" ) )
            tmp = 2;
         if( !str_cmp( arg2, "spell3" ) )
            tmp = 3;
         if( tmp >= 1 && tmp <= 3 )
            value = skill_lookup( arg3 );
         break;

      case ITEM_STAFF:
      case ITEM_WAND:
         if( !str_cmp( arg2, "slevel" ) )
            tmp = 0;
         if( !str_cmp( arg2, "spell" ) )
         {
            tmp = 3;
            value = skill_lookup( arg3 );
         }
         if( !str_cmp( arg2, "maxcharges" ) )
            tmp = 1;
         if( !str_cmp( arg2, "charges" ) )
            tmp = 2;
         break;

      case ITEM_CONTAINER:
         if( !str_cmp( arg2, "capacity" ) )
            tmp = 0;

         if( !str_cmp( arg2, "flags" ) )
         {
            int tmpval = 0;

            tmp = 1;
            argument = arg3;

            if( is_number( argument ) )
            {
               int bv = atoi( argument.c_str() );

               if( bv < 0 || bv > BV31 )
               {
                  ch->printf( "Invalid bitvector value %d. Edit cancelled.\r\n", bv );
                  return;
               }
               tmpval = bv;
            }
            else
            {
               while( !argument.empty(  ) )
               {
                  argument = one_argument( argument, arg3 );
                  value = get_containerflag( arg3 );
                  if( value < 0 || value > 31 )
                     ch->printf( "Invalid container flag %s\r\n", arg3.c_str(  ) );
                  else
                     tmpval += ( 1 << value );
               }
            }
            value = tmpval;
         }

         if( !str_cmp( arg2, "key" ) )
            tmp = 2;
         break;

      case ITEM_SWITCH:
      case ITEM_LEVER:
      case ITEM_PULLCHAIN:
      case ITEM_BUTTON:
         if( !str_cmp( arg2, "flags" ) )
         {
            int tmpval = 0;

            tmp = 0;
            argument = arg3;

            if( is_number( argument ) )
            {
               int bv = atoi( argument.c_str() );

               if( bv < 0 || bv > BV31 )
               {
                  ch->printf( "Invalid bitvector value %d. Edit cancelled.\r\n", bv );
                  return;
               }
               tmpval = bv;
            }
            else
            {
               while( !argument.empty(  ) )
               {
                  argument = one_argument( argument, arg3 );
                  value = get_trigflag( arg3 );
                  if( value < 0 || value > 31 )
                     ch->printf( "Invalid trigger flag %s\r\n", arg3.c_str(  ) );
                  else
                     tmpval += ( 1 << value );
               }
            }
            value = tmpval;
         }
         break;
   }

   if( tmp >= 0 && tmp <= 10 )
   {
      obj->value[tmp] = value;
      if( obj->extra_flags.test( ITEM_PROTOTYPE ) )
         obj->pIndexData->value[tmp] = value;
      ch->print( "Object value set.\r\n" );
      return;
   }

   /*
    * Generate usage message.
    */
   if( ch->substate == SUB_REPEATCMD )
   {
      ch->substate = SUB_RESTRICTED;
      interpret( ch, origarg );
      ch->substate = SUB_REPEATCMD;
      ch->last_cmd = do_oset;
   }
   else
      do_oset( ch, "" );
}

/*
 * Returns value 0 - 9 based on directional text.
 */
int get_dir( const string & txt )
{
   int edir;
   char c1, c2;

   if( !str_cmp( txt, "north" ) )
      return DIR_NORTH;
   if( !str_cmp( txt, "south" ) )
      return DIR_SOUTH;
   if( !str_cmp( txt, "east" ) )
      return DIR_EAST;
   if( !str_cmp( txt, "west" ) )
      return DIR_WEST;
   if( !str_cmp( txt, "up" ) )
      return DIR_UP;
   if( !str_cmp( txt, "down" ) )
      return DIR_DOWN;
   if( !str_cmp( txt, "northeast" ) )
      return DIR_NORTHEAST;
   if( !str_cmp( txt, "northwest" ) )
      return DIR_NORTHWEST;
   if( !str_cmp( txt, "southeast" ) )
      return DIR_SOUTHEAST;
   if( !str_cmp( txt, "southwest" ) )
      return DIR_SOUTHWEST;
   if( !str_cmp( txt, "somewhere" ) )
      return DIR_SOMEWHERE;

   c1 = txt[0];
   if( c1 == '\0' )
      return 0;
   c2 = txt[1];
   edir = 0;
   switch ( c1 )
   {
      case 'n':
         switch ( c2 )
         {
            default:
               edir = DIR_NORTH;
               break;   /* north */

            case 'e':
               edir = DIR_NORTHEAST;
               break;   /* ne   */

            case 'w':
               edir = DIR_NORTHWEST;
               break;   /* nw   */
         }
         break;

      case '0':
         edir = DIR_NORTH;
         break;   /* north */

      case 'e':
      case '1':
         if( c2 == '0' )
            edir = DIR_SOMEWHERE;
         else
            edir = DIR_EAST;
         break;   /* east  */

      case 's':
         switch ( c2 )
         {
            default:
               edir = DIR_SOUTH;
               break;   /* south */

            case 'e':
               edir = DIR_SOUTHEAST;
               break;   /* se   */

            case 'w':
               edir = DIR_SOUTHWEST;
               break;   /* sw   */
         }
         break;

      case '2':
         edir = DIR_SOUTH;
         break;   /* south */

      case 'w':
      case '3':
         edir = DIR_WEST;
         break;   /* west  */

      case 'u':
      case '4':
         edir = DIR_UP;
         break;   /* up    */

      case 'd':
      case '5':
         edir = DIR_DOWN;
         break;   /* down  */

      case '6':
         edir = DIR_NORTHEAST;
         break;   /* ne   */

      case '7':
         edir = DIR_NORTHWEST;
         break;   /* nw   */

      case '8':
         edir = DIR_SOUTHEAST;
         break;   /* se   */

      case '9':
         edir = DIR_SOUTHWEST;
         break;   /* sw   */

      default:
      case '?':
         edir = DIR_SOMEWHERE;
         break;   /* somewhere */
   }
   return edir;
}

CMDF( do_redit )
{
   string arg, arg2, arg3;
   room_index *location, *tmp;
   extra_descr_data *ed;
   exit_data *xit, *texit;
   int value = 0, edir = 0, ekey, evnum;
   string origarg = argument;

   ch->set_color( AT_PLAIN );
   if( !ch->desc )
   {
      ch->print( "You have no descriptor.\r\n" );
      return;
   }

   if( ch->isnpc(  ) )
   {
      ch->print( "NPCs can't redit\r\n" );
      return;
   }

   switch ( ch->substate )
   {
      default:
         break;

      case SUB_ROOM_DESC:
         location = ( room_index * ) ch->pcdata->dest_buf;
         if( !location )
         {
            bug( "%s: sub_room_desc: nullptr ch->pcdata->dest_buf", __func__ );
            location = ch->in_room;
         }
         DISPOSE( location->roomdesc );
         location->roomdesc = ch->copy_buffer( false );
         ch->stop_editing(  );
         ch->substate = ch->tempnum;
         return;

      case SUB_ROOM_DESC_NITE:  /* NiteDesc by Dracones */
         location = ( room_index * ) ch->pcdata->dest_buf;
         if( !location )
         {
            bug( "%s: sub_room_desc_nite: nullptr ch->pcdata->dest_buf", __func__ );
            location = ch->in_room;
         }
         DISPOSE( location->nitedesc );
         location->nitedesc = ch->copy_buffer( false );
         ch->stop_editing(  );
         ch->substate = ch->tempnum;
         return;

      case SUB_ROOM_EXTRA:
         if( !( ed = ( extra_descr_data * ) ch->pcdata->dest_buf ) )
         {
            bug( "%s: sub_room_extra: nullptr ch->pcdata->dest_buf", __func__ );
            ch->stop_editing(  );
            return;
         }
         ed->desc = ch->copy_buffer(  );
         ch->stop_editing(  );
         ch->substate = ch->tempnum;
         return;

      case SUB_EDIT_ABORT:
         ch->substate = SUB_NONE;
         ch->print( "Aborting description.\r\n" );
         return;
   }

   location = ch->in_room;

   smash_tilde( argument );
   argument = one_argument( argument, arg );

   if( ch->substate == SUB_REPEATCMD )
   {
      if( arg.empty(  ) )
      {
         do_rstat( ch, "" );
         return;
      }
      if( !str_cmp( arg, "done" ) || !str_cmp( arg, "off" ) )
      {
         ch->print( "Redit mode off.\r\n" );
         STRFREE( ch->pcdata->subprompt );
         ch->substate = SUB_NONE;
         return;
      }
   }

   if( arg.empty(  ) || !str_cmp( arg, "?" ) || !str_cmp( arg, "help" ) )
   {
      if( ch->substate == SUB_REPEATCMD )
         ch->print( "Syntax: <field> value\r\n\r\n" );
      else
         ch->print( "Syntax: redit <field> value\r\n\r\n" );
      ch->print( "Field being one of:\r\n" );
      ch->print( "  name desc nitedesc ed rmed\r\n" );
      ch->print( "  affect permaffect rmaffect rmpermaffect\r\n" );
      ch->print( "  exit bexit exdesc exflags exname exkey excoord\r\n" );
      ch->print( "  flags sector teledelay televnum tunnel light\r\n" );
      ch->print( "  rlist pulltype pull push\r\n" );
      return;
   }

   if( !can_rmodify( ch, location ) )
      return;

   if( !str_cmp( arg, "on" ) )
   {
      ch->CHECK_SUBRESTRICTED(  );
      ch->print( "Redit mode on.\r\n" );
      ch->substate = SUB_REPEATCMD;
      STRFREE( ch->pcdata->subprompt );
      ch->pcdata->subprompt = STRALLOC( "<&CRedit &W#%r&w> %i" );
      return;
   }

   if( !str_cmp( arg, "name" ) )
   {
      if( argument.empty(  ) )
      {
         ch->print( "Set the room name. A very brief single line room description.\r\n" );
         ch->print( "Usage: redit name <Room summary>\r\n" );
         return;
      }
      STRFREE( location->name );
      location->name = STRALLOC( argument.c_str(  ) );
      ch->print( "Room name set.\r\n" );
      return;
   }

   if( !str_cmp( arg, "desc" ) )
   {
      if( ch->substate == SUB_REPEATCMD )
         ch->tempnum = SUB_REPEATCMD;
      else
         ch->tempnum = SUB_NONE;
      ch->substate = SUB_ROOM_DESC;
      ch->pcdata->dest_buf = location;
      if( !location->roomdesc || location->roomdesc[0] == '\0' )
         location->roomdesc = str_dup( "" );
      ch->editor_desc_printf( "Description of room vnum %d (%s).", location->vnum, location->name );
      ch->start_editing( location->roomdesc );
      return;
   }

   /*
    * nitedesc editing by Dracones 
    */
   if( !str_cmp( arg, "nitedesc" ) )
   {
      if( ch->substate == SUB_REPEATCMD )
         ch->tempnum = SUB_REPEATCMD;
      else
         ch->tempnum = SUB_NONE;
      ch->substate = SUB_ROOM_DESC_NITE;
      ch->pcdata->dest_buf = location;
      if( !location->nitedesc || location->nitedesc[0] == '\0' )
         location->nitedesc = str_dup( "" );
      ch->editor_desc_printf( "Night description of room vnum %d (%s).", location->vnum, location->name );
      ch->start_editing( location->nitedesc );
      return;
   }

   // Imported from Smaug 1.8
   if( !str_cmp( arg, "max_weight" ) || !str_cmp( arg, "maxweight" ) )
   {
      if( argument.empty(  ) )
      {
         ch->print( "Set the maximum weight this room can hold.\r\n" );
         ch->print( "Usage: redit max_weight <value>\r\n" );
         return;
      }
      location->max_weight = URANGE( 0, atoi( argument.c_str(  ) ), 100000 );
      ch->print( "Max weight value set.\r\n" );
      return;
   }

   if( !str_cmp( arg, "tunnel" ) )
   {
      if( argument.empty(  ) )
      {
         ch->print( "Set the maximum characters allowed in the room at one time. (0 = unlimited).\r\n" );
         ch->print( "Usage: redit tunnel <value>\r\n" );
         return;
      }
      location->tunnel = URANGE( 0, atoi( argument.c_str(  ) ), 1000 );
      ch->print( "Tunnel value set.\r\n" );
      return;
   }

   if( !str_cmp( arg, "light" ) )
   {
      if( argument.empty(  ) )
      {
         ch->print( "Set the brightness of light in this room.\r\n" );
         ch->print( "Negative values can indicate magical darkness.\r\n" );
         ch->print( "0 is 'normal' darkness.\r\n" );
         ch->print( "Values from 1 and up indicate artificial, natural, or magical light sources.\r\n" );
         ch->print( "Usage: redit light <value>\r\n" );
         return;
      }
      location->baselight = URANGE( -32000, atoi( argument.c_str(  ) ), 32000 );
      ch->print( "Light value set.\r\n" );
      return;
   }

   // Offloaded the logic to room->olc_add_affect() in roomindex.cpp
   if( !str_cmp( arg, "permaffect" ) )
   {
      location->olc_add_affect( ch, true, argument );
      return;
   }

   if( !str_cmp( arg, "affect" ) )
   {
      location->olc_add_affect( ch, false, argument );
      return;
   }

   // Offloaded the logic to room->olc_remove_affect() in roomindex.cpp
   if( !str_cmp( arg, "rmpermaffect" ) )
   {
      location->olc_remove_affect( ch, true, argument );
      return;
   }

   if( !str_cmp( arg, "rmaffect" ) )
   {
      location->olc_remove_affect( ch, false, argument );
      return;
   }

   if( !str_cmp( arg, "ed" ) )
   {
      if( argument.empty(  ) )
      {
         ch->print( "Create an extra description.\r\n" );
         ch->print( "You must supply keyword(s).\r\n" );
         return;
      }
      ch->CHECK_SUBRESTRICTED(  );
      ed = set_extra_descr( location, argument );
      if( ch->substate == SUB_REPEATCMD )
         ch->tempnum = SUB_REPEATCMD;
      else
         ch->tempnum = SUB_NONE;
      ch->substate = SUB_ROOM_EXTRA;
      ch->pcdata->dest_buf = ed;
      ch->editor_desc_printf( "Extra description '%s' on room %d (%s).", argument.c_str(  ), location->vnum, location->name );
      ch->start_editing( ed->desc );
      return;
   }

   if( !str_cmp( arg, "rmed" ) )
   {
      if( argument.empty(  ) )
      {
         ch->print( "Remove an extra description.\r\n" );
         ch->print( "You must supply keyword(s).\r\n" );
         return;
      }
      if( ( ed = get_extra_descr( argument, location ) ) )
      {
         location->extradesc.remove( ed );
         deleteptr( ed );
         ch->print( "Extra description deleted.\r\n" );
      }
      else
         ch->print( "Extra description not found.\r\n" );
      return;
   }

   if( !str_cmp( arg, "rlist" ) )
   {
      list < reset_data * >::iterator rst;
      char *rbuf;

      if( location->resets.empty(  ) )
      {
         ch->print( "This room has no resets to list.\r\n" );
         return;
      }

      short num = 0;
      for( rst = location->resets.begin(  ); rst != location->resets.end(  ); ++rst )
      {
         reset_data *pReset = *rst;

         ++num;
         if( !( rbuf = sprint_reset( pReset, num ) ) )
            continue;
         ch->print( rbuf );
      }
      return;
   }

   if( !str_cmp( arg, "flags" ) )
   {
      if( argument.empty(  ) )
      {
         ch->print( "Toggle the room flags.\r\n" );
         ch->print( "Usage: redit flags <flag> [flag]...\r\n" );
         return;
      }

      while( !argument.empty(  ) )
      {
         argument = one_argument( argument, arg2 );
         value = get_rflag( arg2 );
         if( value < 0 || value >= ROOM_MAX )
            ch->printf( "Unknown flag: %s\r\n", arg2.c_str(  ) );
         else
         {
            if( value == ROOM_PROTOTYPE && ch->level < sysdata->level_modify_proto )
               ch->print( "You cannot change the prototype flag.\r\n" );
            else
               location->flags.flip( value );
         }
      }
      ch->print( "Room flags set.\r\n" );
      return;
   }

   if( !str_cmp( arg, "teledelay" ) )
   {
      if( argument.empty(  ) )
      {
         ch->print( "Set the delay of the teleport. (0 = off).\r\n" );
         ch->print( "Usage: redit teledelay <value>\r\n" );
         return;
      }
      location->tele_delay = atoi( argument.c_str(  ) );
      ch->print( "Teledelay set.\r\n" );
      return;
   }

   if( !str_cmp( arg, "televnum" ) )
   {
      room_index *temp = nullptr;
      int tvnum;

      if( argument.empty(  ) )
      {
         ch->print( "Set the vnum of the room to teleport to.\r\n" );
         ch->print( "Usage: redit televnum <vnum>\r\n" );
         return;
      }
      tvnum = atoi( argument.c_str(  ) );
      if( tvnum < 1 || tvnum > sysdata->maxvnum )
      {
         ch->printf( "Invalid vnum. Allowable range is 1 to %d\r\n", sysdata->maxvnum );
         return;
      }
      if( !( temp = get_room_index( tvnum ) ) )
      {
         ch->print( "Target vnum does not exist yet.\r\n" );
         return;
      }
      location->tele_vnum = tvnum;
      ch->print( "Televnum set.\r\n" );
      return;
   }

   /*
    * Sector editing now handled via name instead of numerical value - Samson 
    */
   if( !str_cmp( arg, "sector" ) )
   {
      if( argument.empty(  ) )
      {
         ch->print( "Set the sector type.\r\n" );
         ch->print( "Usage: redit sector <name>\r\n" );
         return;
      }
      argument = one_argument( argument, arg2 );
      value = get_sectypes( arg2 );
      if( value < 0 || value >= SECT_MAX )
      {
         location->sector_type = 1;
         ch->print( "Invalid sector type, set to city by default.\r\n" );
      }
      else
      {
         location->sector_type = value;
         ch->printf( "Sector type set to %s.\r\n", arg2.c_str(  ) );
      }
      return;
   }

   if( !str_cmp( arg, "exkey" ) )
   {
      argument = one_argument( argument, arg2 );
      argument = one_argument( argument, arg3 );
      if( arg2.empty(  ) || arg3.empty(  ) )
      {
         ch->print( "Usage: redit exkey <dir> <key vnum>\r\n" );
         return;
      }
      if( arg2[0] == '#' )
      {
         edir = atoi( arg2.substr( 1, arg2.length(  ) ).c_str(  ) );
         xit = location->get_exit_num( edir );
      }
      else
      {
         edir = get_dir( arg2 );
         xit = location->get_exit( edir );
      }
      value = atoi( arg3.c_str(  ) );
      if( !xit )
      {
         ch->print( "No exit in that direction. Use 'redit exit ...' first.\r\n" );
         return;
      }
      xit->key = value;
      ch->print( "Exit key vnum set.\r\n" );
      return;
   }

   if( !str_cmp( arg, "excoord" ) )
   {
      int x, y;

      argument = one_argument( argument, arg2 );
      argument = one_argument( argument, arg3 );
      if( arg2.empty(  ) || arg3.empty(  ) || argument.empty(  ) )
      {
         ch->print( "Usage: redit excoord <dir> <X> <Y>\r\n" );
         return;
      }
      if( arg2[0] == '#' )
      {
         edir = atoi( arg2.substr( 1, arg2.length(  ) ).c_str(  ) );
         xit = location->get_exit_num( edir );
      }
      else
      {
         edir = get_dir( arg2 );
         xit = location->get_exit( edir );
      }

      x = atoi( arg3.c_str(  ) );
      y = atoi( argument.c_str(  ) );

      if( x < 0 || x >= MAX_X )
      {
         ch->printf( "Valid X coordinates are 0 to %d.\r\n", MAX_X - 1 );
         return;
      }

      if( y < 0 || y >= MAX_Y )
      {
         ch->printf( "Valid Y coordinates are 0 to %d.\r\n", MAX_Y - 1 );
         return;
      }

      if( !xit )
      {
         ch->print( "No exit in that direction.  Use 'redit exit ...' first.\r\n" );
         return;
      }
      xit->mx = x;
      xit->my = y;
      ch->print( "Exit coordinates set.\r\n" );
      return;
   }

   if( !str_cmp( arg, "exname" ) )
   {
      argument = one_argument( argument, arg2 );
      if( arg2.empty(  ) )
      {
         ch->print( "Change or clear exit keywords.\r\n" );
         ch->print( "Usage: redit exname <dir> [keywords]\r\n" );
         return;
      }
      if( arg2[0] == '#' )
      {
         edir = atoi( arg2.substr( 1, arg2.length(  ) ).c_str(  ) );
         xit = location->get_exit_num( edir );
      }
      else
      {
         edir = get_dir( arg2 );
         xit = location->get_exit( edir );
      }
      if( !xit )
      {
         ch->print( "No exit in that direction.  Use 'redit exit ...' first.\r\n" );
         return;
      }
      STRFREE( xit->keyword );
      xit->keyword = STRALLOC( argument.c_str(  ) );
      ch->print( "Exit keywords set.\r\n" );
      return;
   }

   if( !str_cmp( arg, "exflags" ) )
   {
      if( argument.empty(  ) )
      {
         ch->print( "Toggle or display exit flags.\r\n" );
         ch->print( "Usage: redit exflags <dir> <flag> [flag]...\r\n" );
         return;
      }

      argument = one_argument( argument, arg2 );
      if( arg2[0] == '#' )
      {
         edir = atoi( arg2.substr( 1, arg2.length(  ) ).c_str(  ) );
         xit = location->get_exit_num( edir );
      }
      else
      {
         edir = get_dir( arg2 );
         xit = location->get_exit( edir );
      }
      if( !xit )
      {
         ch->print( "No exit in that direction.  Use 'redit exit ...' first.\r\n" );
         return;
      }
      if( argument.empty(  ) )
      {
         ch->printf( "Flags for exit direction: %d  Keywords: %s  Key: %d\r\n[ %s ]", xit->vdir, xit->keyword, xit->key, bitset_string( xit->flags, ex_flags ) );
         return;
      }
      while( !argument.empty(  ) )
      {
         argument = one_argument( argument, arg2 );
         value = get_exflag( arg2 );
         if( value < 0 || value >= MAX_EXFLAG )
            ch->printf( "Unknown flag: %s\r\n", arg2.c_str(  ) );
         else
            xit->flags.flip( value );
      }
      ch->print( "Exit flags set.\r\n" );
      return;
   }

   if( !str_cmp( arg, "exit" ) )
   {
      bool addexit, numnotdir;

      argument = one_argument( argument, arg2 );
      argument = one_argument( argument, arg3 );
      if( arg2.empty(  ) )
      {
         ch->print( "Create, change or remove an exit.\r\n" );
         ch->print( "Usage: redit exit <dir> [room] [key] [keyword] [flags]\r\n" );
         return;
      }

      // Pick a direction. Variable edir assumes this value once set.
      addexit = numnotdir = false;
      switch ( arg2[0] )
      {
         default:
            edir = get_dir( arg2 );
            break;
         case '+':
            edir = get_dir( arg2.substr( 1, arg2.length(  ) ) );
            addexit = true;
            break;
         case '#':
            edir = atoi( arg2.substr( 1, arg2.length(  ) ).c_str(  ) );
            numnotdir = true;
            break;
      }

      // Pick a target room number for the exit. Set to 0 if not found.
      if( arg3.empty(  ) )
         evnum = 0;
      else
         evnum = atoi( arg3.c_str(  ) );

      if( numnotdir )
      {
         if( ( xit = location->get_exit_num( edir ) ) != nullptr )
            edir = xit->vdir;
      }
      else
         xit = location->get_exit( edir );

      // If the evnum value is 0, delete this exit and ignore all other arguments to the command.
      if( !evnum )
      {
         if( xit )
         {
            location->extract_exit( xit );
            ch->print( "Exit removed.\r\n" );
            return;
         }
         ch->print( "No exit in that direction.\r\n" );
         return;
      }

      // Validate the target room vnum is within allowable maximums.
      if( evnum < 1 || evnum > ( sysdata->maxvnum - 1 ) )
      {
         ch->print( "Invalid room number.\r\n" );
         return;
      }

      // Check for existing target room....
      if( !( tmp = get_room_index( evnum ) ) )
      {
         // If outside the person's vnum range, bail out. FIXME: Check for people who can edit globally.
         if( evnum < ch->pcdata->low_vnum || evnum > ch->pcdata->hi_vnum )
         {
            ch->printf( "Room #%d does not exist.\r\n", evnum );
            return;
         }

         // Create the target room if the vnum did not exist yet.
         tmp = make_room( evnum, ch->pcdata->area );
         if( !tmp )
         {
            bug( "%s: make_room failed", __func__ );
            return;
         }
      }

      // Actually add or change the exit affected.
      if( addexit || !xit )
      {
         if( numnotdir )
         {
            ch->print( "Cannot add an exit by number, sorry.\r\n" );
            return;
         }
         if( addexit && xit && location->get_exit_to( edir, tmp->vnum ) )
         {
            ch->print( "There is already an exit in that direction leading to that location.\r\n" );
            return;
         }
         xit = location->make_exit( tmp, edir );
         xit->key = -1;
         xit->flags.reset(  );
         act( AT_IMMORT, "$n reveals a hidden passage!", ch, nullptr, nullptr, TO_ROOM );
      }
      else
         act( AT_IMMORT, "Something is different...", ch, nullptr, nullptr, TO_ROOM );

      // A sanity check to make sure it got sent to the proper place.
      if( xit->to_room != tmp )
      {
         xit->to_room = tmp;
         xit->vnum = evnum;
         texit = xit->to_room->get_exit_to( rev_dir[edir], location->vnum );
         if( texit )
         {
            texit->rexit = xit;
            xit->rexit = texit;
         }
      }

      // Set the vnum of the key required to unlock this exit.
      argument = one_argument( argument, arg3 );
      if( !arg3.empty(  ) )
      {
         ekey = atoi( arg3.c_str(  ) );
         if( ekey != 0 || arg3[0] == '0' )
            xit->key = ekey;
      }

      // Set a keyword on this exit. "door", "gate", etc. Only accepts *ONE* keyword.
      argument = one_argument( argument, arg3 );
      if( !arg3.empty(  ) )
      {
         STRFREE( xit->keyword );
         xit->keyword = STRALLOC( arg3.c_str(  ) );
      }

      // And finally set any flags which have been specified.
      if( !argument.empty(  ) )
      {
         while( !argument.empty(  ) )
         {
            argument = one_argument( argument, arg3 );
            value = get_exflag( arg3 );
            if( value < 0 || value >= MAX_EXFLAG )
               ch->printf( "Unknown exit flag: %s\r\n", arg3.c_str(  ) );
            else
               SET_EXIT_FLAG( xit, value );
         }
      }

      // WOO! Finally done. Inform the user.
      ch->print( "New exit added.\r\n" );
      return;
   }

   /*
    * Twisted and evil, but works            -Thoric
    * Makes an exit, and the reverse in one shot.
    */
   if( !str_cmp( arg, "bexit" ) )
   {
      exit_data *bxit, *rxit;
      room_index *tmploc;
      int vnum, exnum;
      char rvnum[MIL];
      bool numnotdir;

      argument = one_argument( argument, arg2 );
      argument = one_argument( argument, arg3 );
      if( arg2.empty(  ) )
      {
         ch->print( "Create, change or remove a two-way exit.\r\n" );
         ch->print( "Usage: redit bexit <dir> [room] [key] [keyword] [flags]\r\n" );
         return;
      }

      numnotdir = false;
      switch ( arg2[0] )
      {
         default:
            edir = get_dir( arg2 );
            break;
         case '#':
            numnotdir = true;
            edir = atoi( arg2.substr( 1, arg2.length(  ) ).c_str(  ) );
            break;
         case '+':
            edir = get_dir( arg2.substr( 1, arg2.length(  ) ) );
            break;
      }

      tmploc = location;
      exnum = edir;
      if( numnotdir )
      {
         if( ( bxit = tmploc->get_exit_num( edir ) ) != nullptr )
            edir = bxit->vdir;
      }
      else
         bxit = tmploc->get_exit( edir );

      rxit = nullptr;
      vnum = 0;
      rvnum[0] = '\0';
      if( bxit )
      {
         vnum = bxit->vnum;
         if( !arg3.empty(  ) )
            snprintf( rvnum, MIL, "%d", tmploc->vnum );
         if( bxit->to_room )
            rxit = bxit->to_room->get_exit( rev_dir[edir] );
         else
            rxit = nullptr;
      }

      funcf( ch, do_redit, "exit %s %s %s", arg2.c_str(  ), arg3.c_str(  ), argument.c_str(  ) );
      if( numnotdir )
         bxit = tmploc->get_exit_num( exnum );
      else
         bxit = tmploc->get_exit( edir );
      if( !rxit && bxit )
      {
         vnum = bxit->vnum;
         if( !arg3.empty(  ) )
            snprintf( rvnum, MIL, "%d", tmploc->vnum );
         if( bxit->to_room )
            rxit = bxit->to_room->get_exit( rev_dir[edir] );
         else
            rxit = nullptr;
      }
      if( vnum )
         cmdf( ch, "at %d redit exit %d %s %s", vnum, rev_dir[edir], rvnum, argument.c_str(  ) );
      return;
   }

   if( !str_cmp( arg, "pulltype" ) || !str_cmp( arg, "pushtype" ) )
   {
      int pt;

      argument = one_argument( argument, arg2 );
      if( arg2.empty(  ) )
      {
         ch->printf( "Set the %s between this room, and the destination room.\r\n", arg.c_str(  ) );
         ch->printf( "Usage: redit %s <dir> <type>\r\n", arg.c_str(  ) );
         return;
      }
      if( arg2[0] == '#' )
      {
         edir = atoi( arg2.substr( 1, arg2.length(  ) ).c_str(  ) );
         xit = location->get_exit_num( edir );
      }
      else
      {
         edir = get_dir( arg2 );
         xit = location->get_exit( edir );
      }
      if( xit )
      {
         if( ( pt = get_pulltype( argument ) ) == -1 )
            ch->printf( "Unknown pulltype: %s.  (See help PULLTYPES)\r\n", argument.c_str(  ) );
         else
         {
            xit->pulltype = pt;
            ch->print( "Done.\r\n" );
            return;
         }
      }
      ch->print( "No exit in that direction.  Use 'redit exit ...' first.\r\n" );
      return;
   }

   if( !str_cmp( arg, "pull" ) )
   {
      argument = one_argument( argument, arg2 );
      if( arg2.empty(  ) )
      {
         ch->print( "Set the 'pull' between this room, and the destination room.\r\n" );
         ch->print( "Usage: redit pull <dir> <force (0 to 100)>\r\n" );
         return;
      }
      if( arg2[0] == '#' )
      {
         edir = atoi( arg2.substr( 1, arg2.length(  ) ).c_str(  ) );
         xit = location->get_exit_num( edir );
      }
      else
      {
         edir = get_dir( arg2 );
         xit = location->get_exit( edir );
      }
      if( xit )
      {
         xit->pull = URANGE( -100, atoi( argument.c_str(  ) ), 100 );
         ch->print( "Done.\r\n" );
         return;
      }
      ch->print( "No exit in that direction.  Use 'redit exit ...' first.\r\n" );
      return;
   }

   if( !str_cmp( arg, "push" ) )
   {
      argument = one_argument( argument, arg2 );
      if( arg2.empty(  ) )
      {
         ch->print( "Set the 'push' away from the destination room in the opposite direction.\r\n" );
         ch->print( "Usage: redit push <dir> <force (0 to 100)>\r\n" );
         return;
      }
      if( arg2[0] == '#' )
      {
         edir = atoi( arg2.substr( 1, arg2.length(  ) ).c_str(  ) );
         xit = location->get_exit_num( edir );
      }
      else
      {
         edir = get_dir( arg2 );
         xit = location->get_exit( edir );
      }
      if( xit )
      {
         xit->pull = URANGE( -100, -( atoi( argument.c_str(  ) ) ), 100 );
         ch->print( "Done.\r\n" );
         return;
      }
      ch->print( "No exit in that direction.  Use 'redit exit ...' first.\r\n" );
      return;
   }

   if( !str_cmp( arg, "exdesc" ) )
   {
      argument = one_argument( argument, arg2 );
      if( arg2.empty(  ) )
      {
         ch->print( "Create or clear a description for an exit.\r\n" );
         ch->print( "Usage: redit exdesc <dir> [description]\r\n" );
         return;
      }
      if( arg2[0] == '#' )
      {
         edir = atoi( arg2.substr( 1, arg2.length(  ) ).c_str(  ) );
         xit = location->get_exit_num( edir );
      }
      else
      {
         edir = get_dir( arg2 );
         xit = location->get_exit( edir );
      }
      if( xit )
      {
         STRFREE( xit->exitdesc );
         if( !argument.empty(  ) )
            stralloc_printf( &xit->exitdesc, "%s\r\n", argument.c_str(  ) );
         ch->print( "Exit description set.\r\n" );
         return;
      }
      ch->print( "No exit in that direction.  Use 'redit exit ...' first.\r\n" );
      return;
   }

   /*
    * Generate usage message.
    */
   if( ch->substate == SUB_REPEATCMD )
   {
      ch->substate = SUB_RESTRICTED;
      interpret( ch, origarg );
      ch->substate = SUB_REPEATCMD;
      ch->last_cmd = do_redit;
   }
   else
      do_redit( ch, "" );
}

/* rdig command by Dracones - From Smaug 1.8 */
CMDF( do_rdig )
{
   room_index *location, *ch_location;
   area_data *pArea;
   exit_data *xit;
   int vnum, edir;

   ch->set_color( AT_PLAIN );

   if( ch->isnpc(  ) )
      return;

   if( !ch->pcdata->area )
   {
      ch->print( "You have no area file with which to work.\r\n" );
      return;
   }

   ch_location = ch->in_room;

   if( argument.empty(  ) )
   {
      ch->print( "Dig out a new room or dig into an existing room.\r\n" );
      ch->print( "Usage: rdig <dir>\r\n" );
      return;
   }

   edir = get_dir( argument );
   xit = ch_location->get_exit( edir );

   if( !xit )
   {
      pArea = ch->pcdata->area;

      vnum = pArea->low_vnum;

      while( vnum <= pArea->hi_vnum && get_room_index( vnum ) != nullptr )
         ++vnum;

      if( vnum > pArea->hi_vnum )
      {
         ch->print( "No empty upper rooms could be found.\r\n" );
         return;
      }

      ch->printf( "Digging out room %d to the %s.\r\n", vnum, argument.c_str(  ) );

      location = make_room( vnum, pArea );
      if( !location )
      {
         bug( "%s: make_room failed", __func__ );
         return;
      }
      location->area = ch->pcdata->area;

      funcf( ch, do_redit, "bexit %s %d", argument.c_str(  ), vnum );
   }
   else
   {
      vnum = xit->vnum;
      location = get_room_index( vnum );
      ch->printf( "Digging into room %d to the %s.\r\n", vnum, argument.c_str(  ) );
   }

   stralloc_printf( &location->name, "%s", ch_location->name );
   strdup_printf( &location->roomdesc, "%s", ch_location->roomdesc );
   location->sector_type = ch_location->sector_type;
   location->flags = ch_location->flags;

   /*
    * Below here you may add anything else you wish to be
    * copied into the rdug rooms.
    */

   /*
    * Move while rdigging -- Dracones 
    */
   cmdf( ch, "goto %d", vnum );
}

/* rgrid command by Dracones - From Smaug 1.8 */
CMDF( do_rgrid )
{
   string arg, arg2;
   room_index *location, *ch_location, *tmp;
   area_data *pArea;
   int vnum, maxnum, x, y, z;
   int room_count;
   int room_hold[MAX_RGRID_ROOMS];

   if( ch->isnpc(  ) )
   {
      ch->print( "NPCs cannot use the rgrid command.\r\n" );
      return;
   }

   ch_location = ch->in_room;
   pArea = ch->pcdata->area;

   argument = one_argument( argument, arg );
   argument = one_argument( argument, arg2 );

   if( arg.empty(  ) )
   {
      ch->print( "Create a block of rooms.\r\n" );
      ch->print( "Usage: rgrid <x> <y> <z>\r\n" );
      return;
   }
   x = atoi( arg.c_str(  ) );
   y = atoi( arg2.c_str(  ) );
   z = atoi( argument.c_str(  ) );

   if( x < 1 || y < 1 )
   {
      ch->print( "You must specify an x and y of at least 1.\r\n" );
      return;
   }
   if( z < 1 )
      z = 1;

   maxnum = x * y * z;

   ch->printf( "Attempting to create a block of %d rooms, %d x %d x %d.\r\n", maxnum, x, y, z );

   if( maxnum > MAX_RGRID_ROOMS )
   {
      ch->printf( "The maximum number of rooms this mud can create at once is %d.\r\n", MAX_RGRID_ROOMS );
      ch->print( "Please try to create a smaller block of rooms.\r\n" );
      return;
   }

   room_count = 0;
   ch->print( "Checking for available rooms...\r\n" );

   if( pArea->low_vnum + maxnum > pArea->hi_vnum )
   {
      ch->print( "You don't even have that many rooms assigned to you.\r\n" );
      return;
   }

   for( vnum = pArea->low_vnum; vnum <= pArea->hi_vnum; ++vnum )
   {
      if( get_room_index( vnum ) == nullptr )
         ++room_count;

      if( room_count >= maxnum )
         break;
   }

   if( room_count < maxnum )
   {
      ch->print( "There aren't enough free rooms in your assigned range!\r\n" );
      return;
   }

   ch->print( "Enough free rooms were found, creating the rooms...\r\n" );

   room_count = 0;
   vnum = pArea->low_vnum;

   while( room_count < maxnum )
   {
      if( !get_room_index( vnum ) )
      {
         room_hold[room_count++] = vnum;

         location = make_room( vnum, pArea );
         if( !location )
         {
            bug( "%s: make_room failed", __func__ );
            return;
         }

         location->area = ch->pcdata->area;

         stralloc_printf( &location->name, "%s", ch_location->name );
         strdup_printf( &location->roomdesc, "%s", ch_location->roomdesc );
         location->sector_type = ch_location->sector_type;
         location->flags = ch_location->flags;
      }
      ++vnum;
   }

   ch->print( "Rooms created, linking the exits...\r\n" );

   for( room_count = 1; room_count <= maxnum; ++room_count )
   {
      vnum = room_hold[room_count - 1];

      // Check to see if we can make N exits
      if( room_count % x )
      {
         location = get_room_index( vnum );
         tmp = get_room_index( room_hold[room_count] );

         location->make_exit( tmp, DIR_NORTH );
      }

      // Check to see if we can make S exits
      if( ( room_count - 1 ) % x )
      {
         location = get_room_index( vnum );
         tmp = get_room_index( room_hold[room_count - 2] );

         location->make_exit( tmp, DIR_SOUTH );
      }

      // Check to see if we can make E exits
      if( ( room_count - 1 ) % ( x * y ) < x * y - x )
      {
         location = get_room_index( vnum );
         tmp = get_room_index( room_hold[room_count + x - 1] );

         location->make_exit( tmp, DIR_EAST );
      }

      // Check to see if we can make W exits
      if( ( room_count - 1 ) % ( x * y ) >= x )
      {
         location = get_room_index( vnum );
         tmp = get_room_index( room_hold[room_count - x - 1] );

         location->make_exit( tmp, DIR_WEST );
      }

      // Check to see if we can make D exits
      if( room_count > x * y )
      {
         location = get_room_index( vnum );
         tmp = get_room_index( room_hold[room_count - x * y - 1] );

         location->make_exit( tmp, DIR_DOWN );
      }

      // Check to see if we can make U exits
      if( room_count <= maxnum - ( x * y ) )
      {
         location = get_room_index( vnum );
         tmp = get_room_index( room_hold[room_count + x * y - 1] );

         location->make_exit( tmp, DIR_UP );
      }
   }
}

CMDF( do_ocreate )
{
   string arg, arg2;
   obj_index *pObjIndex;
   area_data *pArea;
   obj_data *obj;
   int vnum, cvnum;

   if( ch->isnpc(  ) )
   {
      ch->print( "Mobiles cannot create.\r\n" );
      return;
   }
   argument = one_argument( argument, arg );

   vnum = is_number( arg ) ? atoi( arg.c_str(  ) ) : -1;

   if( vnum == -1 || argument.empty(  ) )
   {
      ch->print( "Usage:  ocreate <vnum> [copy vnum] <item name>\r\n" );
      return;
   }

   if( vnum < 1 || vnum > sysdata->maxvnum )
   {
      ch->printf( "Invalid vnum. Allowable range is 1 to %d\r\n", sysdata->maxvnum );
      return;
   }

   one_argument( argument, arg2 );
   cvnum = atoi( arg2.c_str(  ) );
   if( cvnum != 0 )
      argument = one_argument( argument, arg2 );
   if( cvnum < 1 )
      cvnum = 0;

   if( get_obj_index( vnum ) )
   {
      ch->print( "An object with that number already exists.\r\n" );
      return;
   }

   if( ch->isnpc(  ) )
      return;

   if( ch->get_trust(  ) < LEVEL_LESSER )
   {
      if( !ch->pcdata || !( pArea = ch->pcdata->area ) )
      {
         ch->print( "You must have an assigned area to create objects.\r\n" );
         return;
      }
      if( vnum < pArea->low_vnum || vnum > pArea->hi_vnum )
      {
         ch->print( "That number is not in your allocated range.\r\n" );
         return;
      }
   }
   else
   {
      if( !( pArea = ch->pcdata->area ) )
         pArea = ch->in_room->area;
   }

   pObjIndex = make_object( vnum, cvnum, argument, pArea );
   if( !pObjIndex )
   {
      ch->print( "Error.\r\n" );
      bug( "%s: make_object failed.", __func__ );
      return;
   }
   if( !( obj = pObjIndex->create_object( ch->level ) ) )
   {
      log_printf( "create_object: %s:%s, line %d.", __FILE__, __func__, __LINE__ );
      return;
   }
   obj->to_char( ch );

   act( AT_IMMORT, "$n makes arcane gestures, and opens $s hands to reveal $p!", ch, obj, nullptr, TO_ROOM );
   ch->printf( "&YYou make arcane gestures, and open your hands to reveal %s!\r\nObjVnum:  &W%d   &YKeywords:  &W%s\r\n",
               pObjIndex->short_descr, pObjIndex->vnum, pObjIndex->name );
}

CMDF( do_mcreate )
{
   string arg, arg2;
   mob_index *pMobIndex;
   area_data *pArea;
   char_data *mob;
   int vnum, cvnum;

   if( ch->isnpc(  ) )
   {
      ch->print( "Mobiles cannot create.\r\n" );
      return;
   }

   argument = one_argument( argument, arg );

   vnum = is_number( arg ) ? atoi( arg.c_str(  ) ) : -1;

   if( vnum == -1 || argument.empty(  ) )
   {
      ch->print( "Usage: mcreate <vnum> [cvnum] <mobile name>\r\n" );
      return;
   }

   if( vnum < 1 || vnum > sysdata->maxvnum )
   {
      ch->printf( "Invalid vnum. Allowable range is 1 to %d\r\n", sysdata->maxvnum );
      return;
   }

   one_argument( argument, arg2 );
   cvnum = atoi( arg2.c_str(  ) );
   if( cvnum != 0 )
      argument = one_argument( argument, arg2 );
   if( cvnum < 1 )
      cvnum = 0;

   if( get_mob_index( vnum ) )
   {
      ch->print( "A mobile with that number already exists.\r\n" );
      return;
   }

   if( ch->isnpc(  ) )
      return;

   if( ch->get_trust(  ) < LEVEL_LESSER )
   {
      if( !ch->pcdata || !( pArea = ch->pcdata->area ) )
      {
         ch->print( "You must have an assigned area to create mobiles.\r\n" );
         return;
      }
      if( vnum < pArea->low_vnum || vnum > pArea->hi_vnum )
      {
         ch->print( "That number is not in your allocated range.\r\n" );
         return;
      }
   }
   else
   {
      if( !( pArea = ch->pcdata->area ) )
         pArea = ch->in_room->area;
   }

   if( !( pMobIndex = make_mobile( vnum, cvnum, argument, pArea ) ) )
   {
      ch->print( "Error.\r\n" );
      bug( "%s: make_mobile failed.", __func__ );
      return;
   }
   mob = pMobIndex->create_mobile(  );
   if( !mob->to_room( ch->in_room ) )
      log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __func__, __LINE__ );

   /*
    * If you create one on the map, make sure it gets placed properly - Samson 8-21-99 
    */
   fix_maps( ch, mob );

   act( AT_IMMORT, "$n waves $s arms about, and $N appears at $s command!", ch, nullptr, mob, TO_ROOM );
   ch->printf( "&YYou wave your arms about, and %s appears at your command!\r\nMobVnum:  &W%d   &YKeywords:  &W%s\r\n",
               pMobIndex->short_descr, pMobIndex->vnum, pMobIndex->player_name );
}

CMDF( do_rlist )
{
   room_index *room;
   string arg1;
   int lrange, trange, vnum;

   ch->set_pager_color( AT_PLAIN );

   argument = one_argument( argument, arg1 );

   lrange = ( is_number( arg1 ) ? atoi( arg1.c_str(  ) ) : 1 );
   trange = ( is_number( argument ) ? atoi( argument.c_str(  ) ) : 2 );

   for( vnum = lrange; vnum <= trange; ++vnum )
   {
      if( !( room = get_room_index( vnum ) ) )
         continue;
      ch->pagerf( "%5d) %s\r\n", vnum, room->name );
   }
}

CMDF( do_olist )
{
   obj_index *obj;
   string arg1;
   int lrange, trange, vnum;

   ch->set_pager_color( AT_PLAIN );

   argument = one_argument( argument, arg1 );

   lrange = ( is_number( arg1 ) ? atoi( arg1.c_str(  ) ) : 1 );
   trange = ( is_number( argument ) ? atoi( argument.c_str(  ) ) : 2 );

   for( vnum = lrange; vnum <= trange; ++vnum )
   {
      if( !( obj = get_obj_index( vnum ) ) )
         continue;
      ch->pagerf( "%5d) %-20s (%s)\r\n", vnum, obj->name, obj->short_descr );
   }
}

CMDF( do_mlist )
{
   mob_index *mob;
   string arg1;
   int lrange, trange, vnum;

   ch->set_pager_color( AT_PLAIN );

   argument = one_argument( argument, arg1 );

   lrange = ( is_number( arg1 ) ? atoi( arg1.c_str(  ) ) : 1 );
   trange = ( is_number( argument ) ? atoi( argument.c_str(  ) ) : 2 );

   for( vnum = lrange; vnum <= trange; ++vnum )
   {
      if( !( mob = get_mob_index( vnum ) ) )
         continue;
      ch->pagerf( "%5d) %-20s '%s'\r\n", vnum, mob->player_name, mob->short_descr );
   }
}

/* Consolidated list command - Samson 3-21-98 */
/* Command online renamed to 'show' via cedit - Samson 9-8-98 */
CMDF( do_vlist )
{
   string arg, arg2;
   area_data *tarea = nullptr;

   argument = one_argument( argument, arg );
   argument = one_argument( argument, arg2 );

   if( arg.empty(  ) )
   {
      ch->print( "Syntax:\r\n" );
      if( ch->level >= LEVEL_DEMI )
      {
         ch->print( "show mob <lowvnum> <hivnum>\r\n" );
         ch->print( "OR: show mob <area filename>\r\n" );
      }
      if( ch->level >= LEVEL_SAVIOR )
      {
         ch->print( "\r\nshow obj <lowvnum> <hivnum>\r\n" );
         ch->print( "OR: show obj <area filename>\r\n" );
         ch->print( "\r\nshow room <lowvnum> <hivnum>\r\n" );
         ch->print( "OR: show room <area filename>\r\n" );
      }
      return;
   }

   if( !is_number( arg2 ) )
   {
      if( !( tarea = find_area( arg2 ) ) )
      {
         if( !arg2.empty(  ) )
         {
            ch->print( "Area not found.\r\n" );
            return;
         }
         else
            tarea = ch->in_room->area;
      }
   }

   if( ( !str_cmp( arg, "mob" ) || !str_cmp( arg, "m" ) ) && ch->level >= LEVEL_DEMI )
   {
      if( !arg2.empty(  ) && is_number( arg2 ) )
      {
         if( argument.empty(  ) )
         {
            do_vlist( ch, "" );
            return;
         }
         funcf( ch, do_mlist, "%s %s", arg2.c_str(  ), argument.c_str(  ) );
         return;
      }
      funcf( ch, do_mlist, "%d %d", tarea->low_vnum, tarea->hi_vnum );
      return;
   }

   if( ( !str_cmp( arg, "obj" ) || !str_cmp( arg, "o" ) ) && ch->level >= LEVEL_SAVIOR )
   {
      if( !arg2.empty(  ) && is_number( arg2 ) )
      {
         if( argument.empty(  ) )
         {
            do_vlist( ch, "" );
            return;
         }
         funcf( ch, do_olist, "%s %s", arg2.c_str(  ), argument.c_str(  ) );
         return;
      }
      funcf( ch, do_olist, "%d %d", tarea->low_vnum, tarea->hi_vnum );
      return;
   }

   if( !str_cmp( arg, "room" ) || !str_cmp( arg, "r" ) )
   {
      if( !arg2.empty(  ) && is_number( arg2 ) )
      {
         if( argument.empty(  ) )
         {
            do_vlist( ch, "" );
            return;
         }
         funcf( ch, do_rlist, "%s %s", arg2.c_str(  ), argument.c_str(  ) );
         return;
      }
      funcf( ch, do_rlist, "%d %d", tarea->low_vnum, tarea->hi_vnum );
      return;
   }
   /*
    * echo syntax 
    */
   do_vlist( ch, "" );
}

void mpedit( char_data * ch, mud_prog_data * mprg, int mptype, const string & argument )
{
   if( mptype != -1 )
   {
      mprg->type = mptype;
      STRFREE( mprg->arglist );
      if( !argument.empty(  ) )
         mprg->arglist = STRALLOC( argument.c_str(  ) );
   }
   ch->substate = SUB_MPROG_EDIT;
   ch->pcdata->dest_buf = mprg;

   ch->editor_desc_printf( "Program '%s %s'.", mprog_type_to_name( mprg->type ).c_str(  ), mprg->arglist );
   ch->start_editing( mprg->comlist );
}

/*
 * Mobprogram editing - cumbersome - Thoric
 */
CMDF( do_mpedit )
{
   string arg1, arg2, arg3, arg4;
   char_data *victim;
   mud_prog_data *mprog;
   list < mud_prog_data * >::iterator mprg;
   int value, mptype = -1, cnt;

   ch->set_color( AT_PLAIN );

   if( ch->isnpc(  ) )
   {
      ch->print( "Mob's can't mpedit\r\n" );
      return;
   }

   if( !ch->desc )
   {
      ch->print( "You have no descriptor\r\n" );
      return;
   }

   switch ( ch->substate )
   {
      default:
         break;
      case SUB_RESTRICTED:
         ch->print( "You can't use this command from within another command.\r\n" );
         return;
      case SUB_MPROG_EDIT:
         if( !ch->pcdata->dest_buf )
         {
            ch->print( "Fatal error: report to Samson.\r\n" );
            bug( "%s: sub_mprog_edit: nullptr ch->pcdata->dest_buf", __func__ );
            ch->substate = SUB_NONE;
            return;
         }
         mprog = ( mud_prog_data * ) ch->pcdata->dest_buf;
         STRFREE( mprog->comlist );
         mprog->comlist = ch->copy_buffer( true );
         ch->stop_editing(  );
         return;

      case SUB_EDIT_ABORT:
         ch->substate = SUB_NONE;
         ch->print( "Aborting mobprog.\r\n" );
         return;
   }
   smash_tilde( argument );
   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );
   argument = one_argument( argument, arg3 );
   value = atoi( arg3.c_str(  ) );
   if( arg1.empty(  ) || arg2.empty(  ) )
   {
      ch->print( "Syntax: mpedit <victim> <command> [number] <program> <value>\r\n\r\n" );
      ch->print( "Command being one of:\r\n" );
      ch->print( "  add delete insert edit list\r\n" );
      ch->print( "Program being one of:\r\n" );
      ch->print( print_array_string( mprog_flags, ( sizeof( mprog_flags ) / sizeof( mprog_flags[ 0 ] ) ) ) );
      return;
   }

   if( ch->get_trust(  ) < LEVEL_GOD )
   {
      if( !( victim = ch->get_char_room( arg1 ) ) )
      {
         ch->print( "They aren't here.\r\n" );
         return;
      }
   }
   else
   {
      if( !( victim = ch->get_char_world( arg1 ) ) )
      {
         ch->print( "No one like that in all the realms.\r\n" );
         return;
      }
   }

   if( ch->get_trust(  ) < victim->level || !victim->isnpc(  ) )
   {
      ch->print( "You can't do that!\r\n" );
      return;
   }

   if( !can_mmodify( ch, victim ) )
      return;

   if( !victim->has_actflag( ACT_PROTOTYPE ) )
   {
      ch->print( "A mobile must have a prototype flag to be mpset.\r\n" );
      return;
   }

   ch->set_color( AT_GREEN );

   if( !str_cmp( arg2, "add" ) )
   {
      mptype = get_mpflag( arg3 );
      if( mptype == -1 )
      {
         ch->print( "Unknown program type.\r\n" );
         return;
      }
      mprog = new mud_prog_data;
      victim->pIndexData->progtypes.set( mptype );
      mpedit( ch, mprog, mptype, argument );
      victim->pIndexData->mudprogs.push_back( mprog );
      return;
   }

   if( victim->pIndexData->mudprogs.empty(  ) )
   {
      ch->print( "That mobile has no mob programs.\r\n" );
      return;
   }

   if( !str_cmp( arg2, "list" ) )
   {
      cnt = 0;

      if( value < 1 )
      {
         for( mprg = victim->pIndexData->mudprogs.begin(  ); mprg != victim->pIndexData->mudprogs.end(  ); ++mprg )
         {
            mprog = *mprg;

            ch->printf( "%d>%s %s\r\n%s\r\n", ++cnt, mprog_type_to_name( mprog->type ).c_str(  ), mprog->arglist, ( str_cmp( "full", arg3 ) ? mprog->comlist : "" ) );
         }
         return;
      }
      for( mprg = victim->pIndexData->mudprogs.begin(  ); mprg != victim->pIndexData->mudprogs.end(  ); ++mprg )
      {
         mprog = *mprg;

         if( ++cnt == value )
         {
            ch->printf( "%d>%s %s\r\n%s\r\n", cnt, mprog_type_to_name( mprog->type ).c_str(  ), mprog->arglist, mprog->comlist );
            return;
         }
      }
      ch->print( "Program not found.\r\n" );
      return;
   }

   if( !str_cmp( arg2, "edit" ) )
   {
      argument = one_argument( argument, arg4 );
      if( !arg4.empty(  ) )
      {
         mptype = get_mpflag( arg4 );
         if( mptype == -1 )
         {
            ch->print( "Unknown program type.\r\n" );
            return;
         }
      }
      else
         mptype = -1;
      if( value < 1 )
      {
         ch->print( "Program not found.\r\n" );
         return;
      }
      cnt = 0;
      for( mprg = victim->pIndexData->mudprogs.begin(  ); mprg != victim->pIndexData->mudprogs.end(  ); ++mprg )
      {
         mprog = *mprg;

         if( ++cnt == value )
         {
            mpedit( ch, mprog, mptype, argument );
            victim->pIndexData->progtypes.reset(  );
            for( mprg = victim->pIndexData->mudprogs.begin(  ); mprg != victim->pIndexData->mudprogs.end(  ); ++mprg )
            {
               mprog = *mprg;
               victim->pIndexData->progtypes.set( mprog->type );
            }
            return;
         }
      }
      ch->print( "Program not found.\r\n" );
      return;
   }

   if( !str_cmp( arg2, "delete" ) )
   {
      int num;
      bool found;

      argument = one_argument( argument, arg4 );
      if( value < 1 )
      {
         ch->print( "Program not found.\r\n" );
         return;
      }
      cnt = 0;
      found = false;
      for( mprg = victim->pIndexData->mudprogs.begin(  ); mprg != victim->pIndexData->mudprogs.end(  ); ++mprg )
      {
         mprog = *mprg;

         if( ++cnt == value )
         {
            mptype = mprog->type;
            found = true;
            break;
         }
      }
      if( !found )
      {
         ch->print( "Program not found.\r\n" );
         return;
      }
      cnt = num = 0;
      for( mprg = victim->pIndexData->mudprogs.begin(  ); mprg != victim->pIndexData->mudprogs.end(  ); ++mprg )
      {
         mprog = *mprg;

         if( mprog->type == mptype )
            ++num;
      }
      for( mprg = victim->pIndexData->mudprogs.begin(  ); mprg != victim->pIndexData->mudprogs.end(  ); )
      {
         mprog = *mprg;
         ++mprg;

         if( ++cnt == ( value - 1 ) )
         {
            victim->pIndexData->mudprogs.remove( mprog );
            deleteptr( mprog );
            if( num <= 1 )
               victim->pIndexData->progtypes.reset( mptype );
            ch->print( "Program removed.\r\n" );
            return;
         }
      }
      ch->print( "Program not found.\r\n" );
      return;
   }

   if( !str_cmp( arg2, "insert" ) )
   {
      argument = one_argument( argument, arg4 );
      mptype = get_mpflag( arg4 );
      if( mptype == -1 )
      {
         ch->print( "Unknown program type.\r\n" );
         return;
      }
      if( value < 1 )
      {
         ch->print( "Program not found.\r\n" );
         return;
      }
      if( value == 1 )
      {
         mprog = new mud_prog_data;
         victim->pIndexData->progtypes.set( mptype );
         mpedit( ch, mprog, mptype, argument );
         victim->pIndexData->mudprogs.push_back( mprog );
         return;
      }
      cnt = 1;
      for( mprg = victim->pIndexData->mudprogs.begin(  ); mprg != victim->pIndexData->mudprogs.end(  ); ++mprg )
      {
         if( ++cnt == value )
         {
            mprog = new mud_prog_data;
            victim->pIndexData->progtypes.set( mptype );
            mpedit( ch, mprog, mptype, argument );
            victim->pIndexData->mudprogs.insert( mprg, mprog );
            return;
         }
      }
      ch->print( "Program not found.\r\n" );
      return;
   }
   do_mpedit( ch, "" );
}

CMDF( do_opedit )
{
   string arg1, arg2, arg3, arg4;
   obj_data *obj;
   mud_prog_data *mprog;
   list < mud_prog_data * >::iterator mprg;
   int value, mptype = -1, cnt;

   ch->set_color( AT_PLAIN );

   if( ch->isnpc(  ) )
   {
      ch->print( "Mob's can't opedit\r\n" );
      return;
   }

   if( !ch->desc )
   {
      ch->print( "You have no descriptor\r\n" );
      return;
   }

   switch ( ch->substate )
   {
      default:
         break;
      case SUB_RESTRICTED:
         ch->print( "You can't use this command from within another command.\r\n" );
         return;
      case SUB_MPROG_EDIT:
         if( !ch->pcdata->dest_buf )
         {
            ch->print( "Fatal error: report to Samson.\r\n" );
            bug( "%s: sub_oprog_edit: nullptr ch->pcdata->dest_buf", __func__ );
            ch->substate = SUB_NONE;
            return;
         }
         mprog = ( mud_prog_data * ) ch->pcdata->dest_buf;
         STRFREE( mprog->comlist );
         mprog->comlist = ch->copy_buffer( true );
         ch->stop_editing(  );
         return;

      case SUB_EDIT_ABORT:
         ch->substate = SUB_NONE;
         ch->print( "Aborting objprog.\r\n" );
         return;
   }

   smash_tilde( argument );
   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );
   argument = one_argument( argument, arg3 );
   value = atoi( arg3.c_str(  ) );

   if( arg1.empty(  ) || arg2.empty(  ) )
   {
      ch->print( "Syntax: opedit <object> <command> [number] <program> <value>\r\n\r\n" );
      ch->print( "Command being one of:\r\n" );
      ch->print( "  add delete insert edit list\r\n" );
      ch->print( "Program being one of:\r\n" );
      ch->print( print_array_string( mprog_flags, ( sizeof( mprog_flags ) / sizeof( mprog_flags[ 0 ] ) ) ) );
      ch->print( "Object should be in your inventory to edit.\r\n" );
      return;
   }

   if( ch->get_trust(  ) < LEVEL_GOD )
   {
      if( !( obj = ch->get_obj_carry( arg1 ) ) )
      {
         ch->print( "You aren't carrying that.\r\n" );
         return;
      }
   }
   else
   {
      if( !( obj = ch->get_obj_world( arg1 ) ) )
      {
         ch->print( "Nothing like that in all the realms.\r\n" );
         return;
      }
   }

   if( !can_omodify( ch, obj ) )
      return;

   if( !obj->extra_flags.test( ITEM_PROTOTYPE ) )
   {
      ch->print( "An object must have a prototype flag to be opset.\r\n" );
      return;
   }

   ch->set_color( AT_GREEN );

   if( !str_cmp( arg2, "add" ) )
   {
      mptype = get_mpflag( arg3 );
      if( mptype == -1 )
      {
         ch->print( "Unknown program type.\r\n" );
         return;
      }
      mprog = new mud_prog_data;

      obj->pIndexData->progtypes.set( mptype );
      mpedit( ch, mprog, mptype, argument );
      obj->pIndexData->mudprogs.push_back( mprog );
      return;
   }

   if( obj->pIndexData->mudprogs.empty(  ) )
   {
      ch->print( "That object has no obj programs.\r\n" );
      return;
   }

   if( !str_cmp( arg2, "list" ) )
   {
      cnt = 0;
      for( mprg = obj->pIndexData->mudprogs.begin(  ); mprg != obj->pIndexData->mudprogs.end(  ); ++mprg )
      {
         mprog = *mprg;

         ch->printf( "%d>%s %s\r\n%s\r\n", ++cnt, mprog_type_to_name( mprog->type ).c_str(  ), mprog->arglist, mprog->comlist );
      }
      return;
   }

   if( !str_cmp( arg2, "edit" ) )
   {
      argument = one_argument( argument, arg4 );
      if( !arg4.empty(  ) )
      {
         mptype = get_mpflag( arg4 );
         if( mptype == -1 )
         {
            ch->print( "Unknown program type.\r\n" );
            return;
         }
      }
      else
         mptype = -1;
      if( value < 1 )
      {
         ch->print( "Program not found.\r\n" );
         return;
      }
      cnt = 0;
      for( mprg = obj->pIndexData->mudprogs.begin(  ); mprg != obj->pIndexData->mudprogs.end(  ); ++mprg )
      {
         mprog = *mprg;
         if( ++cnt == value )
         {
            mpedit( ch, mprog, mptype, argument );
            obj->pIndexData->progtypes.reset(  );
            for( mprg = obj->pIndexData->mudprogs.begin(  ); mprg != obj->pIndexData->mudprogs.end(  ); ++mprg )
            {
               mprog = *mprg;
               obj->pIndexData->progtypes.set( mprog->type );
            }
            return;
         }
      }
      ch->print( "Program not found.\r\n" );
      return;
   }

   if( !str_cmp( arg2, "delete" ) )
   {
      int num;
      bool found;

      argument = one_argument( argument, arg4 );
      if( value < 1 )
      {
         ch->print( "Program not found.\r\n" );
         return;
      }
      cnt = 0;
      found = false;
      for( mprg = obj->pIndexData->mudprogs.begin(  ); mprg != obj->pIndexData->mudprogs.end(  ); ++mprg )
      {
         mprog = *mprg;

         if( ++cnt == value )
         {
            mptype = mprog->type;
            found = true;
            break;
         }
      }
      if( !found )
      {
         ch->print( "Program not found.\r\n" );
         return;
      }
      cnt = num = 0;
      for( mprg = obj->pIndexData->mudprogs.begin(  ); mprg != obj->pIndexData->mudprogs.end(  ); ++mprg )
      {
         mprog = *mprg;

         if( mprog->type == mptype )
            ++num;
      }
      for( mprg = obj->pIndexData->mudprogs.begin(  ); mprg != obj->pIndexData->mudprogs.end(  ); )
      {
         mprog = *mprg;
         ++mprg;

         if( ++cnt == ( value - 1 ) )
         {
            obj->pIndexData->mudprogs.remove( mprog );
            deleteptr( mprog );
            if( num <= 1 )
               obj->pIndexData->progtypes.reset( mptype );
            ch->print( "Program removed.\r\n" );
            return;
         }
      }
      ch->print( "Program not found.\r\n" );
      return;
   }

   if( !str_cmp( arg2, "insert" ) )
   {
      argument = one_argument( argument, arg4 );
      mptype = get_mpflag( arg4 );
      if( mptype == -1 )
      {
         ch->print( "Unknown program type.\r\n" );
         return;
      }
      if( value < 1 )
      {
         ch->print( "Program not found.\r\n" );
         return;
      }
      if( value == 1 )
      {
         mprog = new mud_prog_data;
         obj->pIndexData->progtypes.set( mptype );
         mpedit( ch, mprog, mptype, argument );
         obj->pIndexData->mudprogs.push_back( mprog );
         return;
      }
      cnt = 1;
      for( mprg = obj->pIndexData->mudprogs.begin(  ); mprg != obj->pIndexData->mudprogs.end(  ); ++mprg )
      {
         if( ++cnt == value )
         {
            mprog = new mud_prog_data;
            obj->pIndexData->progtypes.set( mptype );
            mpedit( ch, mprog, mptype, argument );
            obj->pIndexData->mudprogs.insert( mprg, mprog );
            return;
         }
      }
      ch->print( "Program not found.\r\n" );
      return;
   }
   do_opedit( ch, "" );
}

CMDF( do_rpedit )
{
   string arg1, arg2, arg3;
   mud_prog_data *mprog;
   list < mud_prog_data * >::iterator mprg;
   int value, mptype = -1, cnt;

   ch->set_color( AT_PLAIN );

   if( ch->isnpc(  ) )
   {
      ch->print( "Mob's can't rpedit\r\n" );
      return;
   }

   if( !ch->desc )
   {
      ch->print( "You have no descriptor\r\n" );
      return;
   }

   switch ( ch->substate )
   {
      default:
         break;
      case SUB_RESTRICTED:
         ch->print( "You can't use this command from within another command.\r\n" );
         return;
      case SUB_MPROG_EDIT:
         if( !ch->pcdata->dest_buf )
         {
            ch->print( "Fatal error: report to Samson.\r\n" );
            bug( "%s: sub_oprog_edit: nullptr ch->pcdata->dest_buf", __func__ );
            ch->substate = SUB_NONE;
            return;
         }
         mprog = ( mud_prog_data * ) ch->pcdata->dest_buf;
         STRFREE( mprog->comlist );
         mprog->comlist = ch->copy_buffer( true );
         ch->stop_editing(  );
         return;

      case SUB_EDIT_ABORT:
         ch->substate = SUB_NONE;
         ch->print( "Aborting roomprog.\r\n" );
         return;
   }

   smash_tilde( argument );
   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );
   value = atoi( arg2.c_str(  ) );
   /*
    * argument = one_argument( argument, arg3 ); 
    */

   if( arg1.empty(  ) )
   {
      ch->print( "Syntax: rpedit <command> [number] <program> <value>\r\n\r\n" );
      ch->print( "Command being one of:\r\n" );
      ch->print( "  add delete insert edit list\r\n" );
      ch->print( "Program being one of:\r\n" );
      ch->print( print_array_string( mprog_flags, ( sizeof( mprog_flags ) / sizeof( mprog_flags[ 0 ] ) ) ) );
      ch->print( "You should be standing in the room you wish to edit.\r\n" );
      return;
   }

   if( !can_rmodify( ch, ch->in_room ) )
      return;

   ch->set_color( AT_GREEN );

   if( !str_cmp( arg1, "add" ) )
   {
      mptype = get_mpflag( arg2 );
      if( mptype == -1 )
      {
         ch->print( "Unknown program type.\r\n" );
         return;
      }
      mprog = new mud_prog_data;
      ch->in_room->progtypes.set( mptype );
      mpedit( ch, mprog, mptype, argument );
      ch->in_room->mudprogs.push_back( mprog );
      return;
   }

   if( ch->in_room->mudprogs.empty(  ) )
   {
      ch->print( "This room has no room programs.\r\n" );
      return;
   }

   if( !str_cmp( arg1, "list" ) )
   {
      cnt = 0;
      for( mprg = ch->in_room->mudprogs.begin(  ); mprg != ch->in_room->mudprogs.end(  ); ++mprg )
      {
         mprog = *mprg;

         ch->printf( "%d>%s %s\r\n%s\r\n", ++cnt, mprog_type_to_name( mprog->type ).c_str(  ), mprog->arglist, mprog->comlist );
      }
      return;
   }

   if( !str_cmp( arg1, "edit" ) )
   {
      argument = one_argument( argument, arg3 );
      if( !arg3.empty(  ) )
      {
         mptype = get_mpflag( arg3 );
         if( mptype == -1 )
         {
            ch->print( "Unknown program type.\r\n" );
            return;
         }
      }
      else
         mptype = -1;
      if( value < 1 )
      {
         ch->print( "Program not found.\r\n" );
         return;
      }
      cnt = 0;
      for( mprg = ch->in_room->mudprogs.begin(  ); mprg != ch->in_room->mudprogs.end(  ); ++mprg )
      {
         mprog = *mprg;

         if( ++cnt == value )
         {
            mpedit( ch, mprog, mptype, argument );
            ch->in_room->progtypes.reset(  );
            for( mprg = ch->in_room->mudprogs.begin(  ); mprg != ch->in_room->mudprogs.end(  ); ++mprg )
            {
               mprog = *mprg;

               ch->in_room->progtypes.set( mprog->type );
            }
            return;
         }
      }
      ch->print( "Program not found.\r\n" );
      return;
   }

   if( !str_cmp( arg1, "delete" ) )
   {
      int num;
      bool found;

      argument = one_argument( argument, arg3 );
      if( value < 1 )
      {
         ch->print( "Program not found.\r\n" );
         return;
      }
      cnt = 0;
      found = false;
      for( mprg = ch->in_room->mudprogs.begin(  ); mprg != ch->in_room->mudprogs.end(  ); ++mprg )
      {
         mprog = *mprg;
         if( ++cnt == value )
         {
            mptype = mprog->type;
            found = true;
            break;
         }
      }
      if( !found )
      {
         ch->print( "Program not found.\r\n" );
         return;
      }
      cnt = num = 0;
      for( mprg = ch->in_room->mudprogs.begin(  ); mprg != ch->in_room->mudprogs.end(  ); ++mprg )
      {
         mprog = *mprg;

         if( mprog->type == mptype )
            ++num;
      }
      for( mprg = ch->in_room->mudprogs.begin(  ); mprg != ch->in_room->mudprogs.end(  ); ++mprg )
      {
         mprog = *mprg;

         if( ++cnt == value )
         {
            ch->in_room->mudprogs.remove( mprog );
            deleteptr( mprog );
            if( num <= 1 )
               ch->in_room->progtypes.reset( mptype );
            ch->print( "Program removed.\r\n" );
            return;
         }
      }
      return;
   }

   if( !str_cmp( arg2, "insert" ) )
   {
      argument = one_argument( argument, arg3 );
      mptype = get_mpflag( arg2 );
      if( mptype == -1 )
      {
         ch->print( "Unknown program type.\r\n" );
         return;
      }
      if( value < 1 )
      {
         ch->print( "Program not found.\r\n" );
         return;
      }
      if( value == 1 )
      {
         mprog = new mud_prog_data;
         ch->in_room->progtypes.set( mptype );
         mpedit( ch, mprog, mptype, argument );
         ch->in_room->mudprogs.push_back( mprog );
         return;
      }
      cnt = 1;
      for( mprg = ch->in_room->mudprogs.begin(  ); mprg != ch->in_room->mudprogs.end(  ); ++mprg )
      {
         if( ++cnt == value )
         {
            mprog = new mud_prog_data;
            ch->in_room->progtypes.set( mptype );
            mpedit( ch, mprog, mptype, argument );
            ch->in_room->mudprogs.insert( mprg, mprog );
            return;
         }
      }
      ch->print( "Program not found.\r\n" );
      return;
   }
   do_rpedit( ch, "" );
}

/*  
 *  Mobile and Object Program Copying 
 *  Last modified Feb. 24 1999
 *  Mystaric
 */
void mpcopy( mud_prog_data * source, mud_prog_data * destination )
{
   destination->type = source->type;
   destination->triggered = source->triggered;
   destination->resetdelay = source->resetdelay;
   destination->arglist = QUICKLINK( source->arglist );
   destination->comlist = QUICKLINK( source->comlist );
}

CMDF( do_opcopy )
{
   string sobj, prog, num, dobj;
   obj_data *source = nullptr, *destination = nullptr;
   int value = -1, optype = -1, cnt = 0;
   bool COPY = false;

   if( ch->isnpc(  ) )
   {
      ch->print( "Mob's can't opcopy\r\n" );
      return;
   }

   if( !ch->desc )
   {
      ch->print( "You have no descriptor\r\n" );
      return;
   }

   smash_tilde( argument );
   argument = one_argument( argument, sobj );
   argument = one_argument( argument, prog );

   if( sobj.empty(  ) || prog.empty(  ) )
   {
      ch->print( "Syntax: opcopy <source object> <program> [number] <destination object>\r\n" );
      ch->print( "        opcopy <source object> all <destination object>\r\n" );
      ch->print( "        opcopy <source object> all <destination object> <program>\r\n\r\n" );
      ch->print( "Program being one of:\r\n" );
      ch->print( "  act speech rand wear remove sac zap get\r\n" );
      ch->print( "  drop damage repair greet exa use\r\n" );
      ch->print( "  pull push (for levers,pullchains,buttons)\r\n\r\n" );
      ch->print( "Object should be in your inventory to edit.\r\n" );
      return;
   }

   if( !str_cmp( prog, "all" ) )
   {
      argument = one_argument( argument, dobj );
      argument = one_argument( argument, prog );
      optype = get_mpflag( prog );
      COPY = true;
   }
   else
   {
      argument = one_argument( argument, num );
      argument = one_argument( argument, dobj );
      value = atoi( num.c_str(  ) );
   }

   if( ch->get_trust(  ) < LEVEL_GOD )
   {
      if( !( source = ch->get_obj_carry( sobj ) ) )
      {
         ch->print( "You aren't carrying source object.\r\n" );
         return;
      }

      if( !( destination = ch->get_obj_carry( dobj ) ) )
      {
         ch->print( "You aren't carrying destination object.\r\n" );
         return;
      }
   }
   else
   {
      if( !( source = ch->get_obj_world( sobj ) ) )
      {
         ch->print( "Can't find source object in all the realms.\r\n" );
         return;
      }

      if( !( destination = ch->get_obj_world( dobj ) ) )
      {
         ch->print( "Can't find destination object in all the realms.\r\n" );
         return;
      }
   }

   if( source == destination )
   {
      ch->print( "Source and destination objects cannot be the same\r\n" );
      return;
   }

   if( !can_omodify( ch, destination ) )
   {
      ch->print( "You cannot modify destination object.\r\n" );
      return;
   }

   if( !destination->extra_flags.test( ITEM_PROTOTYPE ) )
   {
      ch->print( "Destination object must have prototype flag.\r\n" );
      return;
   }

   ch->set_color( AT_GREEN );

   if( source->pIndexData->mudprogs.empty(  ) )
   {
      ch->print( "Source object has no programs.\r\n" );
      return;
   }

   list < mud_prog_data * >::iterator sp;
   if( COPY )
   {
      for( sp = source->pIndexData->mudprogs.begin(  ); sp != source->pIndexData->mudprogs.end(  ); ++sp )
      {
         mud_prog_data *source_oprg = *sp;

         if( optype == source_oprg->type || optype == -1 )
         {
            mud_prog_data *dest_oprg = new mud_prog_data;
            mpcopy( source_oprg, dest_oprg );
            destination->pIndexData->mudprogs.push_back( dest_oprg );
            destination->pIndexData->progtypes.set( dest_oprg->type );
            ++cnt;
         }
      }

      if( cnt == 0 )
      {
         ch->print( "No such program in source object\r\n" );
         return;
      }
      ch->printf( "%d programs successfully copied from %s to %s.\r\n", cnt, sobj.c_str(  ), dobj.c_str(  ) );
      return;
   }

   if( value < 1 )
   {
      ch->print( "No such program in source object.\r\n" );
      return;
   }

   optype = get_mpflag( prog );

   for( sp = source->pIndexData->mudprogs.begin(  ); sp != source->pIndexData->mudprogs.end(  ); ++sp )
   {
      mud_prog_data *source_oprg = *sp;

      if( ++cnt == value && source_oprg->type == optype )
      {
         mud_prog_data *dest_oprg = new mud_prog_data;

         mpcopy( source_oprg, dest_oprg );
         destination->pIndexData->mudprogs.push_back( dest_oprg );
         destination->pIndexData->progtypes.set( dest_oprg->type );
         ch->printf( "%s program %d from %s successfully copied to %s.\r\n", prog.c_str(  ), value, sobj.c_str(  ), dobj.c_str(  ) );
         return;
      }
   }
   ch->print( "No such program in source object.\r\n" );
}

CMDF( do_mpcopy )
{
   string smob, prog, num, dmob;
   char_data *source = nullptr, *destination = nullptr;
   list < mud_prog_data * >::iterator sp;
   int value = -1, mptype = -1, cnt = 0;
   bool COPY = false;

   ch->set_color( AT_PLAIN );

   if( ch->isnpc(  ) )
   {
      ch->print( "Mob's can't opcopy\r\n" );
      return;
   }

   if( !ch->desc )
   {
      ch->print( "You have no descriptor\r\n" );
      return;
   }

   smash_tilde( argument );
   argument = one_argument( argument, smob );
   argument = one_argument( argument, prog );

   if( smob.empty(  ) || prog.empty(  ) )
   {
      ch->print( "Syntax: mpcopy <source mobile> <program> [number] <destination mobile>\r\n" );
      ch->print( "        mpcopy <source mobile> all <destination mobile>\r\n" );
      ch->print( "        mpcopy <source mobile> all <destination mobile> <program>\r\n\r\n" );
      ch->print( "Program being one of:\r\n" );
      ch->print( "  act speech rand fight hitprcnt greet allgreet\r\n" );
      ch->print( "  entry give bribe death time hour script\r\n" );
      return;
   }

   if( !str_cmp( prog, "all" ) )
   {
      argument = one_argument( argument, dmob );
      argument = one_argument( argument, prog );
      mptype = get_mpflag( prog );
      COPY = true;
   }
   else
   {
      argument = one_argument( argument, num );
      argument = one_argument( argument, dmob );
      value = atoi( num.c_str(  ) );
   }

   if( ch->get_trust(  ) < LEVEL_GOD )
   {
      if( !( source = ch->get_char_room( smob ) ) )
      {
         ch->print( "Source mobile is not present.\r\n" );
         return;
      }

      if( !( destination = ch->get_char_room( dmob ) ) )
      {
         ch->print( "Destination mobile is not present.\r\n" );
         return;
      }
   }
   else
   {
      if( !( source = ch->get_char_world( smob ) ) )
      {
         ch->print( "Can't find source mobile\r\n" );
         return;
      }

      if( !( destination = ch->get_char_world( dmob ) ) )
      {
         ch->print( "Can't find destination mobile\r\n" );
         return;
      }
   }
   if( source == destination )
   {
      ch->print( "Source and destination mobiles cannot be the same\r\n" );
      return;
   }

   if( ch->get_trust(  ) < source->level || !source->isnpc(  ) || ch->get_trust(  ) < destination->level || !destination->isnpc(  ) )
   {
      ch->print( "You can't do that!\r\n" );
      return;
   }

   if( !can_mmodify( ch, destination ) )
   {
      ch->print( "You cannot modify destination mobile.\r\n" );
      return;
   }

   if( !destination->has_actflag( ACT_PROTOTYPE ) )
   {
      ch->print( "Destination mobile must have a prototype flag to mpcopy.\r\n" );
      return;
   }

   ch->set_color( AT_GREEN );

   if( source->pIndexData->mudprogs.empty(  ) )
   {
      ch->print( "Source mobile has no programs.\r\n" );
      return;
   }

   if( COPY )
   {
      for( sp = source->pIndexData->mudprogs.begin(  ); sp != source->pIndexData->mudprogs.end(  ); ++sp )
      {
         mud_prog_data *source_mprg = *sp;

         if( mptype == source_mprg->type || mptype == -1 )
         {
            mud_prog_data *dest_mprg = new mud_prog_data;

            mpcopy( source_mprg, dest_mprg );
            destination->pIndexData->mudprogs.push_back( dest_mprg );
            destination->pIndexData->progtypes.set( dest_mprg->type );
            ++cnt;
         }
      }
      if( cnt == 0 )
      {
         ch->printf( "No such program in source mobile\r\n" );
         return;
      }
      ch->printf( "%d programs successfully copied from %s to %s.\r\n", cnt, smob.c_str(  ), dmob.c_str(  ) );
      return;
   }

   if( value < 1 )
   {
      ch->print( "No such program in source mobile.\r\n" );
      return;
   }

   mptype = get_mpflag( prog );

   for( sp = source->pIndexData->mudprogs.begin(  ); sp != source->pIndexData->mudprogs.end(  ); ++sp )
   {
      mud_prog_data *source_mprg = *sp;

      if( ++cnt == value && source_mprg->type == mptype )
      {
         mud_prog_data *dest_mprg = new mud_prog_data;

         mpcopy( source_mprg, dest_mprg );
         destination->pIndexData->mudprogs.push_back( dest_mprg );
         destination->pIndexData->progtypes.set( dest_mprg->type );
         ch->printf( "%s program %d from %s successfully copied to %s.\r\n", prog.c_str(  ), value, smob.c_str(  ), dmob.c_str(  ) );
         return;
      }
   }
   ch->print( "No such program in source mobile.\r\n" );
}

CMDF( do_rpcopy )
{
   string sroom, prog, num, droom;
   room_index *source = nullptr, *destination = nullptr;
   list < mud_prog_data * >::iterator sp;
   int value = -1, mptype = -1, cnt = 0;
   bool COPY = false;

   ch->set_color( AT_PLAIN );

   if( ch->isnpc(  ) )
   {
      ch->print( "Mob's can't rpcopy.\r\n" );
      return;
   }

   if( !ch->desc )
   {
      ch->print( "You have no descriptor!\r\n" );
      return;
   }

   smash_tilde( argument );
   argument = one_argument( argument, sroom );
   argument = one_argument( argument, prog );

   if( !is_number( sroom ) || prog.empty(  ) )
   {
      ch->print( "Syntax: rpcopy <source rvnum> <program> [number] <destination rvnum>\r\n" );
      ch->print( "  rpcopy <source rvnum> all <destination rvnum>\r\n" );
      ch->print( "  rpcopy <source rvnum> all <destination rvnum> <program>\r\n\r\n" );
      ch->print( "Program being one of:\r\n" );
      ch->print( "  act speech rand fight hitprcnt greet allgreet\r\n" );
      ch->print( "  entry give bribe death time hour script\r\n" );
      return;
   }

   if( !str_cmp( prog, "all" ) )
   {
      argument = one_argument( argument, droom );
      argument = one_argument( argument, prog );
      mptype = get_mpflag( prog );
      COPY = true;
   }
   else
   {
      argument = one_argument( argument, num );
      argument = one_argument( argument, droom );
      value = atoi( num.c_str(  ) );
   }

   if( !( source = get_room_index( atoi( sroom.c_str(  ) ) ) ) )
   {
      ch->print( "Source room does not exist.\r\n" );
      return;
   }

   if( !is_number( droom ) || !( destination = get_room_index( atoi( droom.c_str(  ) ) ) ) )
   {
      ch->print( "Destination room does not exist.\r\n" );
      return;
   }

   if( source == destination )
   {
      ch->print( "Source and destination rooms cannot be the same.\r\n" );
      return;
   }

   if( !can_rmodify( ch, destination ) )
   {
      ch->print( "You cannot modify destination room.\r\n" );
      return;
   }

   ch->set_color( AT_GREEN );

   if( source->mudprogs.empty(  ) )
   {
      ch->print( "Source room has no programs.\r\n" );
      return;
   }

   if( COPY )
   {
      for( sp = source->mudprogs.begin(  ); sp != source->mudprogs.end(  ); ++sp )
      {
         mud_prog_data *source_rprg = *sp;

         if( mptype == source_rprg->type || mptype == -1 )
         {
            mud_prog_data *dest_rprg = new mud_prog_data;

            destination->mudprogs.push_back( dest_rprg );
            mpcopy( source_rprg, dest_rprg );
            destination->progtypes.set( dest_rprg->type );
            ++cnt;
         }
      }

      if( cnt == 0 )
      {
         ch->print( "No such program in source room.\r\n" );
         return;
      }
      ch->printf( "%d programs successfully copied from %s to %s.\r\n", cnt, sroom.c_str(  ), droom.c_str(  ) );
      return;
   }

   if( value < 1 )
   {
      ch->print( "No such program in source room.\r\n" );
      return;
   }

   mptype = get_mpflag( prog );

   for( sp = source->mudprogs.begin(  ); sp != source->mudprogs.end(  ); ++sp )
   {
      mud_prog_data *source_rprg = *sp;

      if( ++cnt == value && source_rprg->type == mptype )
      {
         mud_prog_data *dest_rprg = new mud_prog_data;

         mpcopy( source_rprg, dest_rprg );
         destination->mudprogs.push_back( dest_rprg );
         destination->progtypes.set( dest_rprg->type );
         ch->printf( "%s program %d from %s successfully copied to %s.\r\n", prog.c_str(  ), value, sroom.c_str(  ), droom.c_str(  ) );
         return;
      }
   }
   ch->print( "No such program in source room.\r\n" );
}

CMDF( do_makerooms )
{
   room_index *location;
   area_data *pArea;
   int vnum, x, room_count;

   pArea = ch->pcdata->area;

   if( !pArea )
   {
      ch->print( "You must have an area assigned to do this.\r\n" );
      return;
   }

   if( argument.empty(  ) )
   {
      ch->print( "Create a block of rooms.\r\n" );
      ch->print( "Usage: makerooms <# of rooms>\r\n" );
      return;
   }
   x = atoi( argument.c_str(  ) );

   ch->printf( "Attempting to create a block of %d rooms.\r\n", x );

   if( x > 1000 )
   {
      ch->printf( "The maximum number of rooms this mud can create at once is 1000.\r\n" );
      return;
   }

   room_count = 0;

   if( pArea->low_vnum + x > pArea->hi_vnum )
   {
      ch->print( "You don't even have that many rooms assigned to you.\r\n" );
      return;
   }

   for( vnum = pArea->low_vnum; vnum <= pArea->hi_vnum; ++vnum )
   {
      if( !get_room_index( vnum ) )
         ++room_count;

      if( room_count >= x )
         break;
   }

   if( room_count < x )
   {
      ch->print( "There aren't enough free rooms in your assigned range!\r\n" );
      return;
   }

   ch->print( "Creating the rooms...\r\n" );

   room_count = 0;
   vnum = pArea->low_vnum;

   while( room_count < x )
   {
      if( !get_room_index( vnum ) )
      {
         ++room_count;

         if( !( location = make_room( vnum, ch->pcdata->area ) ) )
         {
            bug( "%s: failed", __func__ );
            return;
         }
      }
      ++vnum;
   }
   ch->printf( "%d rooms created.\r\n", room_count );
}

/* Consolidated *assign function. 
 * Assigns room/obj/mob ranges and initializes new zone - Samson 2-12-99 
 */
CMDF( do_vassign )
{
   string arg1, arg2, arg3;
   int lo, hi;
   char_data *victim, *mob;
   room_index *room;
   mob_index *pMobIndex;
   obj_index *pObjIndex;
   obj_data *obj;
   area_data *tarea;
   char filename[256];

   ch->set_color( AT_IMMORT );

#ifdef MULTIPORT
   if( mud_port == MAINPORT )
   {
      ch->print( "Vassign is disabled on this port.\r\n" );
      return;
   }
#endif

   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );
   argument = one_argument( argument, arg3 );
   lo = atoi( arg2.c_str(  ) );
   hi = atoi( arg3.c_str(  ) );

   if( arg1.empty(  ) || lo < 0 || hi < 0 )
   {
      ch->print( "Syntax: vassign <who> <low> <high>\r\n" );
      return;
   }

   if( !( victim = ch->get_char_world( arg1 ) ) )
   {
      ch->print( "They don't seem to be around.\r\n" );
      return;
   }

   if( victim->isnpc(  ) || victim->get_trust(  ) < LEVEL_CREATOR )
   {
      ch->print( "They wouldn't know what to do with a vnum range.\r\n" );
      return;
   }

   if( lo == 0 && hi == 0 )
   {
      victim->pcdata->area = nullptr;
      victim->pcdata->low_vnum = 0;
      victim->pcdata->hi_vnum = 0;
      victim->printf( "%s has removed your vnum range.\r\n", ch->name );
      victim->save(  );
      return;
   }

   if( lo >= sysdata->maxvnum || hi >= sysdata->maxvnum )
   {
      ch->printf( "Cannot assign this range, maximum allowable vnum is currently %d.\r\n", sysdata->maxvnum );
      return;
   }

   if( lo == 0 && hi != 0 )
   {
      ch->print( "Unacceptable vnum range, low vnum cannot be 0 when hi vnum is not.\r\n" );
      return;
   }

   if( lo > hi )
   {
      ch->print( "Unacceptable vnum range, low vnum must be smaller than high vnum.\r\n" );
      return;
   }

   if( victim->pcdata->area || victim->pcdata->low_vnum || victim->pcdata->hi_vnum )
   {
      ch->print( "You cannot assign them a range, they already have one!\r\n" );
      return;
   }

   if( check_area_conflicts( lo, hi ) )
   {
      ch->print( "That vnum range conflicts with another area.\r\n" );
      return;
   }

   victim->pcdata->low_vnum = lo;
   victim->pcdata->hi_vnum = hi;
   assign_area( victim );
   ch->print( "Done.\r\n" );
   assign_area( victim );  /* Put back by Thoric on 02/07/96 */
   victim->printf( "&Y%s has assigned you the vnum range %d - %d.\r\n", ch->name, lo, hi );

   if( !victim->pcdata->area )
   {
      bug( "%s: assign_area failed", __func__ );
      return;
   }

   tarea = victim->pcdata->area;

   /*
    * Initialize first and last rooms in range 
    */
   if( !( room = make_room( lo, tarea ) ) )
   {
      bug( "%s: failed to initialize first room.", __func__ );
      return;
   }

   if( !( room = make_room( hi, tarea ) ) )
   {
      bug( "%s: failed to initialize last room.", __func__ );
      return;
   }

   /*
    * Initialize first mob in range 
    */
   if( !( pMobIndex = make_mobile( lo, 0, "first mob", tarea ) ) )
   {
      bug( "%s: make_mobile failed to initialize first mob.", __func__ );
      return;
   }
   mob = pMobIndex->create_mobile(  );
   if( !mob->to_room( room ) )
      log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __func__, __LINE__ );

   /*
    * Initialize last mob in range 
    */
   if( !( pMobIndex = make_mobile( hi, 0, "last mob", tarea ) ) )
   {
      bug( "%s: make_mobile failed to initialize last mob.", __func__ );
      return;
   }
   mob = pMobIndex->create_mobile(  );
   if( !mob->to_room( room ) )
      log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __func__, __LINE__ );

   /*
    * Initialize first obj in range 
    */
   if( !( pObjIndex = make_object( lo, 0, "first obj", tarea ) ) )
   {
      bug( "%s: make_object failed to initialize first obj.", __func__ );
      return;
   }
   if( !( obj = pObjIndex->create_object( 1 ) ) )
   {
      log_printf( "create_object: %s:%s, line %d.", __FILE__, __func__, __LINE__ );
      return;
   }
   obj->to_room( room, nullptr );

   /*
    * Initialize last obj in range 
    */
   if( !( pObjIndex = make_object( hi, 0, "last obj", tarea ) ) )
   {
      bug( "%s: make_object failed to initialize last obj.", __func__ );
      return;
   }
   if( !( obj = pObjIndex->create_object( 1 ) ) )
   {
      log_printf( "create_object: %s:%s, line %d.", __FILE__, __func__, __LINE__ );
      return;
   }
   obj->to_room( room, nullptr );

   /*
    * Save character and newly created zone 
    */
   victim->save(  );

   tarea->creation_date = current_time;
   snprintf( filename, 256, "%s%s", BUILD_DIR, tarea->filename );
   tarea->fold( filename, false );

   tarea->flags.set( AFLAG_PROTOTYPE );
   ch->set_color( AT_IMMORT );
   ch->printf( "Vnum range set for %s and initialized.\r\n", victim->name );
}
