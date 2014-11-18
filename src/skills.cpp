/****************************************************************************
 *                   ^     +----- |  / ^     ^ |     | +-\                  *
 *                  / \    |      | /  |\   /| |     | |  \                 *
 *                 /   \   +---   |<   | \ / | |     | |  |                 *
 *                /-----\  |      | \  |  v  | |     | |  /                 *
 *               /       \ |      |  \ |     | +-----+ +-/                  *
 ****************************************************************************
 * AFKMud Copyright 1997-2015 by Roger Libiez (Samson),                     *
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
 *                         Player skills module                             *
 ****************************************************************************/

#if defined(__CYGWIN__) || defined(WIN32)
#include <sys/time.h>
#endif
#include <algorithm>
#include "mud.h"
#include "mudcfg.h"
#include "fight.h"
#include "mobindex.h"
#include "objindex.h"
#include "overland.h"
#include "polymorph.h"
#include "raceclass.h"
#include "roomindex.h"
#include "skill_index.h"
#include "smaugaffect.h"

ch_ret ranged_attack( char_data *, string, obj_data *, obj_data *, short, short );
bool check_pos( char_data *, short );
SPELLF( spell_smaug );
SPELLF( spell_notfound );
SPELLF( spell_null );
int ris_save( char_data *, int, int );
bool validate_spec_fun( const string & );
bool is_safe( char_data *, char_data * );
bool check_illegal_pk( char_data *, char_data * );
bool legal_loot( char_data *, char_data * );
void set_fighting( char_data *, char_data * );
void failed_casting( struct skill_type *, char_data *, char_data *, obj_data * );
void start_timer( struct timeval * );
time_t end_timer( struct timeval * );
void check_mount_objs( char_data *, bool );
int get_door( const string & );
void check_killer( char_data *, char_data * );
void raw_kill( char_data *, char_data * );
void death_cry( char_data * );
void group_gain( char_data *, char_data * );
ch_ret one_hit( char_data *, char_data *, int );
ch_ret check_room_for_traps( char_data *, int );
int IsGiant( char_data * );
int IsDragon( char_data * );
void bind_follower( char_data *, char_data *, int, int );
void save_classes(  );
void save_races(  );
void remove_bexit_flag( exit_data *, int );

/* global variables */
int top_herb;
int top_disease;
skill_type *skill_table[MAX_SKILL];
skill_type *herb_table[MAX_HERB];
skill_type *disease_table[MAX_DISEASE];
extern FILE *fpArea;

SKILL_INDEX skill_table__index;
SKILL_INDEX skill_table__spell;
SKILL_INDEX skill_table__skill;
SKILL_INDEX skill_table__racial;
SKILL_INDEX skill_table__combat;
SKILL_INDEX skill_table__tongue;
SKILL_INDEX skill_table__lore;

const char *old_ris_flags[] = {
   "fire", "cold", "electricity", "energy", "blunt", "pierce", "slash", "acid",
   "poison", "drain", "sleep", "charm", "hold", "nonmagic", "plus1", "plus2",
   "plus3", "plus4", "plus5", "plus6", "magic", "paralysis", "good", "evil", "hack",
   "lash"
};

const char *skill_tname[] = { "unknown", "Spell", "Skill", "Combat", "Tongue", "Herb", "Racial", "Disease", "Lore" };

const char *att_kick_kill_ch[] = {
   "Your kick caves $N's chest in, which kills $M.",
   "Your kick destroys $N's arm and caves in one side of $S rib cage.",
   "Your kick smashes through $N's leg and into $S pelvis, killing $M.",
   "Your kick shatters $N's skull.",
   "Your kick at $N's snout shatters $S jaw, killing $M.",
   "You kick $N in the rump with such force that $E keels over dead.",
   "You kick $N in the belly, mangling several ribs and killing $M instantly.",
   "$N's scales cave in as your mighty kick kills $N.",
   "Your kick rips bark asunder and leaves fly everywhere, killing the $N.",
   "Bits of $N are sent flying as you kick him to pieces.",
   "You punt $N across the room, $E lands in a heap of broken flesh.",
   "You kick $N in the groin, $E dies screaming an octave higher.",
   "",   /* GHOST */
   "Feathers fly about as you blast $N to pieces with your kick.",
   "Your kick splits $N to pieces, rotting flesh flies everywhere.",
   "Your kick topples $N over, killing it.",
   "Your foot shatters cartilage, sending bits of $N everywhere.",
   "You launch a mighty kick at $N's gills, killing it.",
   "Your kick at $N sends $M to the grave.",
   "."
};

const char *att_kick_kill_vic[] = {
   "$n crushes you beneath $s foot, killing you.",
   "$n destroys your arm and half your ribs.  You die.",
   "$n neatly splits your groin in two, you collapse and die instantly.",
   "$n splits your head in two, killing you instantly.",
   "$n forces your jaw into the lower part of your brain.",
   "$n kicks you from behind, snapping your spine and killing you.",
   "$n kicks your stomach and you into the great land beyond!!",
   "Your scales are no defense against $n's mighty kick.",
   "$n rips you apart with a massive kick, you die in a flutter of leaves.",
   "You are torn to little pieces as $n splits you with $s kick.",
   "$n's kick sends you flying, you die before you land.",
   "Puny little $n manages to land a killing blow to your groin, OUCH!",
   "",   /* GHOST */
   "Your feathers fly about as $n pulverizes you with a massive kick.",
   "$n's kick rips your rotten body into shreds, and your various pieces die.",
   "$n kicks you so hard, you fall over and die.",
   "$n shatters your exoskeleton, you die.",
   "$n kicks you in the gills!  You cannot breath..... you die!.",
   "$n sends you to the grave with a mighty kick.",
   "."
};

const char *att_kick_kill_room[] = {
   "$n strikes $N in chest, shattering the ribs beneath it.",
   "$n kicks $N in the side, destroying $S arm and ribs.",
   "$n nails $N in the groin, the pain killing $M.",
   "$n shatters $N's head, reducing $M to a twitching heap!",
   "$n blasts $N in the snout, destroying bones and causing death.",
   "$n kills $N with a massive kick to the rear.",
   "$n sends $N to the grave with a massive blow to the stomach!",
   "$n ignores $N's scales and kills $M with a mighty kick.",
   "$n sends bark and leaves flying as $e splits $N in two.",
   "$n blasts $N to pieces with a ferocious kick.",
   "$n sends $N flying, $E lands with a LOUD THUD, making no other noise.",
   "$N falls to the ground and dies clutching $S crotch due to $n's kick.",
   "",   /* GHOST */
   "$N disappears into a cloud of feathers as $n kicks $M to death.",
   "$n blasts $N's rotten body into pieces with a powerful kick.",
   "$n kicks $N so hard, it falls over and dies.",
   "$n blasts $N's exoskeleton to little fragments.",
   "$n kicks $N in the gills, killing it.",
   "$n sends $N to the grave with a mighty kick.",
   "."
};

const char *att_kick_miss_ch[] = {
   "$N steps back, and your kick misses $M.",
   "$N deftly blocks your kick with $S forearm.",
   "$N dodges, and you miss your kick at $S legs.",
   "$N ducks, and your foot flies a mile high.",
   "$N steps back and grins evilly as your foot flys by $S face.",
   "$N laughs at your feeble attempt to kick $M from behind.",
   "Your kick at $N's belly makes it laugh.",
   "$N chuckles as your kick bounces off $S tough scales.",
   "You kick $N in the side, denting your foot.",
   "Your sloppy kick is easily avoided by $N.",
   "You misjudge $N's height and kick well above $S head.",
   "You stub your toe against $N's shin as you try to kick $M.",
   "Your kick passes through $N!!", /* Ghost */
   "$N nimbly flitters away from your kick.",
   "$N sidesteps your kick and sneers at you.",
   "Your kick bounces off $N's leathery hide.",
   "Your kick bounces off $N's tough exoskeleton.",
   "$N deflects your kick with a fin.",
   "$N avoids your paltry attempt at a kick.",
   "."
};

const char *att_kick_miss_vic[] = {
   "$n misses you with $s clumsy kick at your chest.",
   "You block $n's feeble kick with your arm.",
   "You dodge $n's feeble leg sweep.",
   "You duck under $n's lame kick.",
   "You step back and grin as $n misses your face with a kick.",
   "$n attempts a feeble kick from behind, which you neatly avoid.",
   "You laugh at $n's feeble attempt to kick you in the stomach.",
   "$n kicks you, but your scales are much too tough for that wimp.",
   "You laugh as $n dents $s foot on your bark.",
   "You easily avoid a sloppy kick from $n.",
   "$n's kick parts your hair but does little else.",
   "$n's light kick to your shin bearly gets your attention.",
   "$n passes through you with $s puny kick.",
   "You nimbly flitter away from $n's kick.",
   "You sneer as you sidestep $n's kick.",
   "$n's kick bounces off your tough hide.",
   "$n tries to kick you, but your too tough.",
   "$n tried to kick you, but you deflected it with a fin.",
   "You avoid $n's feeble attempt to kick you.",
   "."
};

const char *att_kick_miss_room[] = {
   "$n misses $N with a clumsy kick.",
   "$N blocks $n's kick with $S arm.",
   "$N easily dodges $n's feeble leg sweep.",
   "$N easily ducks under $n's lame kick.",
   "$N steps back and grins evilly at $n's feeble kick to $S face misses.",
   "$n launches a kick at $N's behind, but fails miserably.",
   "$N laughs at $n's attempt to kick $M in the stomach.",
   "$n tries to kick $N, but $s foot bounces off of $N's scales.",
   "$n hurts his foot trying to kick $N.",
   "$N avoids a lame kick launched by $n.",
   "$n misses a kick at $N due to $S small size.",
   "$n misses a kick at $N's groin, stubbing $s toe in the process.",
   "$n's foot goes right through $N!!!!",
   "$N flitters away from $n's kick.",
   "$N sneers at $n while sidestepping $s kick.",
   "$N's tough hide deflects $n's kick.",
   "$n hurts $s foot on $N's tough exterior.",
   "$n tries to kick $N, but is thwarted by a fin.",
   "$N avoids $n's feeble kick.",
   "."
};

const char *att_kick_hit_ch[] = {
   "Your kick crashes into $N's chest.",
   "Your kick hits $N in the side.",
   "You hit $N in the thigh with a hefty sweep.",
   "You hit $N in the face, sending $M reeling.",
   "You plant your foot firmly in $N's snout, smashing it to one side.",
   "You nail $N from behind, sending him reeling.",
   "You kick $N in the stomach, winding $M.",
   "You find a soft spot in $N's scales and launch a solid kick there.",
   "Your kick hits $N, sending small branches and leaves everywhere.",
   "Your kick contacts with $N, dislodging little pieces of $M.",
   "Your kick hits $N right in the stomach, $N is rendered breathless.",
   "You stomp on $N's foot. After all, thats about all you can do to a giant.",
   "",   /* GHOST */
   "Your kick  sends $N reeling through the air.",
   "You kick $N and feel rotten bones crunch from the blow.",
   "You smash $N with a hefty roundhouse kick.",
   "You kick $N, cracking it's exoskeleton.",
   "Your mighty kick rearranges $N's scales.",
   "You leap off the ground and crash into $N with a powerful kick.",
   "."
};

const char *att_kick_hit_vic[] = {
   "$n's kick crashes into your chest.",
   "$n's kick hits you in your side.",
   "$n's sweep catches you in the side and you almost stumble.",
   "$n hits you in the face, gee, what pretty colors...",
   "$n kicks you in the snout, smashing it up against your face.",
   "$n blasts you in the rear, ouch!",
   "Your breath rushes from you as $n kicks you in the stomach.",
   "$n finds a soft spot on your scales and kicks you, ouch!",
   "$n kicks you hard, sending leaves flying everywhere!",
   "$n kicks you in the side, dislodging small parts of you.",
   "You suddenly see $n's foot in your chest.",
   "$n lands a kick hard on your foot making you jump around in pain.",
   "",   /* GHOST */
   "$n kicks you, and you go reeling through the air.",
   "$n kicks you and your bones crumble.",
   "$n hits you in the flank with a hefty roundhouse kick.",
   "$n ruins some of your scales with a well placed kick.",
   "$n leaps off of the grand and crashes into you with $s kick.",
   "."
};

const char *att_kick_hit_room[] = {
   "$n hits $N with a mighty kick to $S chest.",
   "$n whacks $N in the side with a sound kick.",
   "$n almost sweeps $N off of $S feet with a well placed leg sweep.",
   "$N's eyes roll as $n plants a foot in $S face.",
   "$N's snout is smashed as $n relocates it with $s foot.",
   "$n hits $N with an impressive kick from behind.",
   "$N gasps as $n kick $N in the stomach.",
   "$n finds a soft spot in $N's scales and launches a solid kick there.",
   "$n kicks $N.  Leaves fly everywhere!!",
   "$n hits $N with a mighty kick, $N loses parts of $Mself.",
   "$n kicks $N in the stomach, $N is rendered breathless.",
   "$n kicks $N in the foot, $N hops around in pain.",
   "",   /* GHOST */
   "$n sends $N reeling through the air with a mighty kick.",
   "$n kicks $N causing parts of $N to cave in!",
   "$n kicks $N in the side with a hefty roundhouse kick.",
   "$n kicks $N, cracking exo-skelelton.",
   "$n kicks $N hard, sending scales flying!",
   "$n leaps up and nails $N with a mighty kick.",
   "."
};

const char *spell_flag[] = {
   "water", "earth", "air", "astral", "area", "distant", "reverse",
   "noself", "nobind", "accumulative", "recastable", "noscribe",
   "nobrew", "group", "object", "character", "secretskill", "pksensitive",
   "stoponfail", "nofight", "nodispel", "randomtarget", "nomob"
};

const char *spell_saves[] = { "none", "Poison/Death", "Wands", "Paralysis/Petrification", "Breath", "Spells" };

const char *spell_save_effect[] = { "none", "Negation", "1/8 Damage", "1/4 Damage", "Half Damage", "3/4 Damage", "Reflection", "Absorbtion" };

const char *spell_damage[] = { "none", "fire", "cold", "electricity", "energy", "acid", "poison", "drain" };

const char *spell_action[] = { "none", "create", "destroy", "resist", "suscept", "divinate", "obscure", "change" };

const char *spell_power[] = { "none", "minor", "greater", "major" };

const char *spell_class[] = { "none", "lunar", "solar", "travel", "summon", "life", "death", "illusion" };

const char *target_type[] = { "ignore", "offensive", "defensive", "self", "objinv" };

smaug_affect::smaug_affect(  )
{
   init_memory( &duration, &location, sizeof( location ) );
}

smaug_affect::~smaug_affect(  )
{
   DISPOSE( duration );
   DISPOSE( modifier );
}

skill_type::skill_type(  )
{
   init_memory( &flags, &participants, sizeof( participants ) );
   affects.clear(  );
}

skill_type::~skill_type(  )
{
   list < smaug_affect * >::iterator aff;

   if( !affects.empty(  ) )
   {
      for( aff = affects.begin(  ); aff != affects.end(  ); )
      {
         smaug_affect *af = *aff;
         ++aff;

         affects.remove( af );
         deleteptr( af );
      }
   }
   DISPOSE( name );
   STRFREE( author );
   DISPOSE( noun_damage );
   DISPOSE( msg_off );
   DISPOSE( hit_char );
   DISPOSE( hit_vict );
   DISPOSE( hit_room );
   DISPOSE( hit_dest );
   DISPOSE( miss_char );
   DISPOSE( miss_vict );
   DISPOSE( miss_room );
   DISPOSE( die_char );
   DISPOSE( die_vict );
   DISPOSE( die_room );
   DISPOSE( imm_char );
   DISPOSE( imm_vict );
   DISPOSE( imm_room );
   DISPOSE( dice );
   DISPOSE( components );
   DISPOSE( teachers );
   DISPOSE( spell_fun_name );
   DISPOSE( skill_fun_name );
   DISPOSE( helptext );
}

int get_skillflag( const string & flag )
{
   for( size_t x = 0; x < ( sizeof( spell_flag ) / sizeof( spell_flag[0] ) ); ++x )
      if( !str_cmp( flag, spell_flag[x] ) )
         return x;
   return -1;
}

// String Sort
// Used to sort strings by string value rather than binary value.
string_sort::string_sort( void )
{
}

bool string_sort::operator(  ) ( const string & left, const string & right )
{
   return ( left.compare( right ) < 0 );
}

// Used to find skills using their prefix
find__skill_prefix::find__skill_prefix( char_data * p_actor, const string & p_value )
{
   actor = p_actor;
   value = p_value;
}

bool find__skill_prefix::operator(  ) ( pair < string, int >compare )
{
   string key;
   int sn;

   key = compare.first;
   sn = compare.second;

   if( actor && !actor->isnpc(  ) && ( ( !skill_table[sn] || actor->pcdata->learned[sn] == 0 ) ) )
      return false;

   return ( key.find( value ) == 0 );
}

// Used to find skills using the exact name.
find__skill_exact::find__skill_exact( char_data * p_actor, const string & p_value )
{
   actor = p_actor;
   value = p_value;
}

bool find__skill_exact::operator(  ) ( pair < string, int >compare )
{
   string key;
   int sn;

   key = compare.first;
   sn = compare.second;

   if( actor && !actor->isnpc(  ) && ( ( !skill_table[sn] || actor->pcdata->learned[sn] == 0 ) ) )
      return false;

   return ( key == value );
}

// Skill Search Functions
int search_skill_prefix( SKILL_INDEX index, const string & key )
{
   SKILL_INDEX::iterator fnd, it, end;

   it = index.begin(  );
   end = index.end(  );

   if( ( fnd = find_if( it, end, find__skill_prefix( NULL, key ) ) ) == end )
      return -1;
   return fnd->second;
}

int search_skill_prefix( SKILL_INDEX index, const string & key, char_data * actor )
{
   SKILL_INDEX::iterator fnd, it, end;

   // Safety
   if( actor == NULL || actor->isnpc(  ) )
      return -1;

   it = index.begin(  );
   end = index.end(  );

   if( ( fnd = find_if( it, end, find__skill_prefix( actor, key ) ) ) == end )
      return -1;
   return fnd->second;
}

int search_skill_exact( SKILL_INDEX index, const string & key )
{
   SKILL_INDEX::iterator fnd, it, end;

   it = index.begin(  );
   end = index.end(  );

   if( ( fnd = index.find( key ) ) == end )
      return -1;
   return fnd->second;
}

int search_skill_exact( SKILL_INDEX index, const string & key, char_data * actor )
{
   SKILL_INDEX::iterator fnd, it, end;

   // Safety
   if( actor == NULL || actor->isnpc(  ) )
      return -1;

   it = index.begin(  );
   end = index.end(  );

   if( ( fnd = find_if( it, end, find__skill_exact( actor, key ) ) ) == end )
      return -1;
   return fnd->second;
}

int search_skill( SKILL_INDEX index, const string & key )
{
   int sn;

   if( ( sn = search_skill_exact( index, key ) ) != -1 )
      return sn;
   return search_skill_prefix( index, key );
}

int search_skill( SKILL_INDEX index, const string & key, char_data * actor )
{
   int sn;

   if( ( sn = search_skill_exact( index, key, actor ) ) != -1 )
      return sn;
   return search_skill_prefix( index, key, actor );
}

int find_spell( char_data * ch, const string & name, bool know )
{
   if( !ch || ch->isnpc(  ) || !know )
      return search_skill( skill_table__spell, name );
   else
      return search_skill( skill_table__spell, name, ch );
}

int find_skill( char_data * ch, const string & name, bool know )
{
   if( !ch || ch->isnpc(  ) || !know )
      return search_skill( skill_table__skill, name );
   else
      return search_skill( skill_table__skill, name, ch );
}

int find_combat( char_data * ch, const string & name, bool know )
{
   if( !ch || ch->isnpc(  ) || !know )
      return search_skill( skill_table__combat, name );
   else
      return search_skill( skill_table__combat, name, ch );
}

int find_ability( char_data * ch, const string & name, bool know )
{
   if( !ch || ch->isnpc(  ) || !know )
      return search_skill( skill_table__racial, name );
   else
      return search_skill( skill_table__racial, name, ch );
}

int find_tongue( char_data * ch, const string & name, bool know )
{
   if( !ch || ch->isnpc(  ) || !know )
      return search_skill( skill_table__tongue, name );
   else
      return search_skill( skill_table__tongue, name, ch );
}

int find_lore( char_data * ch, const string & name, bool know )
{
   if( !ch || ch->isnpc(  ) || !know )
      return search_skill( skill_table__lore, name );
   else
      return search_skill( skill_table__lore, name, ch );
}

/*
 * Lookup a skill by name, only stopping at skills the player has.
 */
int ch_slookup( char_data * ch, const string & name )
{
   int sn;

   sn = skill_lookup( name );
   // does this skill even exist?
   if( sn == -1 )
      return -1;

   // Make sure that:
   // (a) ch knows this skill
   // and, (b) ch's level is high enough
   if( ch->isnpc(  ) )
      return sn;
   if( ch->pcdata->learned[sn] > 0 && ( ch->level >= skill_table[sn]->skill_level[ch->Class] || ch->level >= skill_table[sn]->race_level[ch->race] ) )
   {
      return sn;
   }
   else
   {
      // nope... ch doesn't have this skill.
      return -1;
   }
}

/*
 * Lookup a skill by name.
 *
 * First tries to find an exact match. Then looks for a prefix match.
 * Rehauled by dchaley 2007-06-22.
 *
 * Overhauled and uber-simplified by Justice on 8/13/07
 * This is the main gateway into searching skills and is the only method that should be called
 * to do so aside from the find_* functions above.
 * If you want more fine-grained lookups this is the place to put them.
 */
int skill_lookup( const string & name )
{
   int sn;

   if( ( sn = search_skill_exact( skill_table__index, name ) ) != -1 )
      return sn;
   if( ( sn = search_skill_prefix( skill_table__index, name ) ) != -1 )
      return sn;

   return -1;
}

/*
 * Return a skilltype pointer based on sn			-Thoric
 * Returns NULL if bad, unused or personal sn.
 */
skill_type *get_skilltype( int sn )
{
   if( sn >= TYPE_PERSONAL )
      return NULL;
   if( sn >= TYPE_HERB )
      return IS_VALID_HERB( sn - TYPE_HERB ) ? herb_table[sn - TYPE_HERB] : NULL;
   if( sn >= TYPE_HIT )
      return NULL;
   return IS_VALID_SN( sn ) ? skill_table[sn] : NULL;
}

/*
 * Lookup an herb by name.
 */
int herb_lookup( const string & name )
{
   for( int sn = 0; sn < top_herb; ++sn )
   {
      if( !herb_table[sn] || !herb_table[sn]->name )
         return -1;
      if( LOWER( name[0] ) == LOWER( herb_table[sn]->name[0] ) && !str_prefix( name, herb_table[sn]->name ) )
         return sn;
   }
   return -1;
}

/*
 * Lookup a skill by slot number.
 * Used for object loading.
 */
int slot_lookup( int slot )
{
   int sn;

   if( slot <= 0 )
      return -1;

   for( sn = 0; sn < num_skills; ++sn )
      if( slot == skill_table[sn]->slot )
         return sn;

   if( fBootDb )
      bug( "%s: bad slot %d.", __func__, slot );

   return -1;
}

CMDF( do_slotlookup )
{
   int sn;

   if( ( sn = slot_lookup( atoi( argument.c_str(  ) ) ) ) == -1 )
   {
      ch->printf( "%s is not a valid slot number.\r\n", argument.c_str(  ) );
      return;
   }
   ch->printf( "Slot %s belongs to skill/spell '%s'\r\n", argument.c_str(  ), skill_table[sn]->name );
}

/*
 * Remap slot numbers to sn values
 */
void remap_slot_numbers( void )
{
   skill_type *skill;
   list < smaug_affect * >::iterator aff;

   log_string( "Remapping slots to sns..." );

   for( int sn = 0; sn < num_skills; ++sn )
   {
      if( ( skill = skill_table[sn] ) != NULL )
      {
         for( aff = skill->affects.begin(  ); aff != skill->affects.end(  ); ++aff )
         {
            smaug_affect *af = *aff;

            if( af->location == APPLY_WEAPONSPELL
                || af->location == APPLY_WEARSPELL || af->location == APPLY_REMOVESPELL || af->location == APPLY_STRIPSN || af->location == APPLY_RECURRINGSPELL )
            {
               strdup_printf( &af->modifier, "%d", slot_lookup( atoi( af->modifier ) ) );
            }
         }
      }
   }
}

/*
 * Function used by qsort to sort skills; sorts by name, not case sensitive.
 */
int skill_comp( skill_type ** sk1, skill_type ** sk2 )
{
   skill_type *skill1 = ( *sk1 );
   skill_type *skill2 = ( *sk2 );

   if( !skill1 && skill2 )
      return 1;
   if( skill1 && !skill2 )
      return -1;
   if( !skill1 && !skill2 )
      return 0;
   // Sort without regard to case.
   return strcasecmp( skill1->name, skill2->name );
}

void update_skill_index( skill_type * skill, int sn )
{
   string buf = skill->name;

   // to LowerCase
   strlower( buf );

   skill_table__index[buf] = sn;
   switch ( skill->type )
   {
      case SKILL_SPELL:
         skill_table__spell[buf] = sn;
         break;
      case SKILL_SKILL:
         skill_table__skill[buf] = sn;
         break;
      case SKILL_RACIAL:
         skill_table__racial[buf] = sn;
         break;
      case SKILL_COMBAT:
         skill_table__combat[buf] = sn;
         break;
      case SKILL_TONGUE:
         skill_table__tongue[buf] = sn;
         break;
      case SKILL_LORE:
         skill_table__lore[buf] = sn;
         break;
   }
}

/*
 * Sort the skill table with qsort
 */
void sort_skill_table(  )
{
   log_string( "Sorting skill table..." );
   // Jury is still out on whether or not we care if this is sorted: qsort( &skill_table[1], num_skills - 1, sizeof( skill_type * ), ( int ( * )( const void *, const void * ) )skill_comp );

   // Populate index
   skill_type *cur;
   for( int sn = 1; sn < num_skills; ++sn )
   {
      if( !( cur = skill_table[sn] ) || !cur->name )
         continue;
      update_skill_index( cur, sn );
   }
}

int get_skill( const string & skilltype )
{
   if( !str_cmp( skilltype, "Racial" ) )
      return SKILL_RACIAL;
   if( !str_cmp( skilltype, "Spell" ) )
      return SKILL_SPELL;
   if( !str_cmp( skilltype, "Skill" ) )
      return SKILL_SKILL;
   if( !str_cmp( skilltype, "Combat" ) )
      return SKILL_COMBAT;
   if( !str_cmp( skilltype, "Tongue" ) )
      return SKILL_TONGUE;
   if( !str_cmp( skilltype, "Herb" ) )
      return SKILL_HERB;
   if( !str_cmp( skilltype, "Lore" ) )
      return SKILL_LORE;
   return SKILL_UNKNOWN;
}

/*
 * Write skill data to a file
 */
void fwrite_skill( FILE * fpout, skill_type * skill )
{
   fprintf( fpout, "Name         %s~\n", skill->name );
   fprintf( fpout, "Type         %s\n", skill_tname[skill->type] );
   fprintf( fpout, "Info         %d\n", skill->info );
   if( skill->author && skill->author[0] != '\0' )
      fprintf( fpout, "Author       %s~\n", skill->author );
   if( skill->flags.any(  ) )
      fprintf( fpout, "Flags        %s~\n", bitset_string( skill->flags, spell_flag ) );
   if( skill->target )
      fprintf( fpout, "Target       %d\n", skill->target );
   if( skill->minimum_position )
      fprintf( fpout, "Minpos       %s~\n", npc_position[skill->minimum_position] );
   if( skill->saves )
      fprintf( fpout, "Saves        %d\n", skill->saves );
   if( skill->slot )
      fprintf( fpout, "Slot         %d\n", skill->slot );
   if( skill->min_mana )
      fprintf( fpout, "Mana         %d\n", skill->min_mana );
   if( skill->beats )
      fprintf( fpout, "Rounds       %d\n", skill->beats );
   if( skill->range )
      fprintf( fpout, "Range        %d\n", skill->range );
   if( skill->guild != -1 )
      fprintf( fpout, "Guild        %d\n", skill->guild );
   if( skill->ego )
      fprintf( fpout, "Ego          %d\n", skill->ego );
   if( skill->skill_fun )
      fprintf( fpout, "Code         %s\n", skill->skill_fun_name );
   else if( skill->spell_fun )
      fprintf( fpout, "Code         %s\n", skill->spell_fun_name );
   fprintf( fpout, "Dammsg       %s~\n", skill->noun_damage );
   if( skill->msg_off && skill->msg_off[0] != '\0' )
      fprintf( fpout, "Wearoff      %s~\n", skill->msg_off );

   if( skill->hit_char && skill->hit_char[0] != '\0' )
      fprintf( fpout, "Hitchar      %s~\n", skill->hit_char );
   if( skill->hit_vict && skill->hit_vict[0] != '\0' )
      fprintf( fpout, "Hitvict      %s~\n", skill->hit_vict );
   if( skill->hit_room && skill->hit_room[0] != '\0' )
      fprintf( fpout, "Hitroom      %s~\n", skill->hit_room );
   if( skill->hit_dest && skill->hit_dest[0] != '\0' )
      fprintf( fpout, "Hitdest      %s~\n", skill->hit_dest );

   if( skill->miss_char && skill->miss_char[0] != '\0' )
      fprintf( fpout, "Misschar     %s~\n", skill->miss_char );
   if( skill->miss_vict && skill->miss_vict[0] != '\0' )
      fprintf( fpout, "Missvict     %s~\n", skill->miss_vict );
   if( skill->miss_room && skill->miss_room[0] != '\0' )
      fprintf( fpout, "Missroom     %s~\n", skill->miss_room );

   if( skill->die_char && skill->die_char[0] != '\0' )
      fprintf( fpout, "Diechar      %s~\n", skill->die_char );
   if( skill->die_vict && skill->die_vict[0] != '\0' )
      fprintf( fpout, "Dievict      %s~\n", skill->die_vict );
   if( skill->die_room && skill->die_room[0] != '\0' )
      fprintf( fpout, "Dieroom      %s~\n", skill->die_room );

   if( skill->imm_char && skill->imm_char[0] != '\0' )
      fprintf( fpout, "Immchar      %s~\n", skill->imm_char );
   if( skill->imm_vict && skill->imm_vict[0] != '\0' )
      fprintf( fpout, "Immvict      %s~\n", skill->imm_vict );
   if( skill->imm_room && skill->imm_room[0] != '\0' )
      fprintf( fpout, "Immroom      %s~\n", skill->imm_room );

   if( skill->dice && skill->dice[0] != '\0' )
      fprintf( fpout, "Dice         %s~\n", skill->dice );
   if( skill->value )
      fprintf( fpout, "Value        %d\n", skill->value );
   if( skill->difficulty )
      fprintf( fpout, "Difficulty   %d\n", skill->difficulty );
   if( skill->participants )
      fprintf( fpout, "Participants %d\n", skill->participants );
   if( skill->components && skill->components[0] != '\0' )
      fprintf( fpout, "Components   %s~\n", skill->components );
   if( skill->teachers && skill->teachers[0] != '\0' )
      fprintf( fpout, "Teachers     %s~\n", skill->teachers );

   int modifier;
   list < smaug_affect * >::iterator aff;
   for( aff = skill->affects.begin(  ); aff != skill->affects.end(  ); ++aff )
   {
      smaug_affect *af = *aff;

      if( af->location == APPLY_AFFECT )
         af->location = APPLY_EXT_AFFECT;
      fprintf( fpout, "Affect       '%s' %d ", af->duration, af->location );
      modifier = atoi( af->modifier );
      if( ( af->location == APPLY_WEAPONSPELL
            || af->location == APPLY_WEARSPELL
            || af->location == APPLY_REMOVESPELL || af->location == APPLY_STRIPSN || af->location == APPLY_RECURRINGSPELL ) && IS_VALID_SN( modifier ) )
         fprintf( fpout, "'%d' ", skill_table[modifier]->slot );
      else
         fprintf( fpout, "'%s' ", af->modifier );
      fprintf( fpout, "%d\n", af->bit );
   }

   if( skill->type != SKILL_HERB )
   {
      int y, min = 1000;

      for( y = 0; y < MAX_PC_CLASS; ++y )
         if( skill->skill_level[y] < min )
            min = skill->skill_level[y];

      fprintf( fpout, "Minlevel     %d\n", min );

      min = 1000;
      for( y = 0; y < MAX_PC_RACE; ++y )
         if( skill->race_level[y] < min )
            min = skill->race_level[y];
   }
   if( skill->helptext && skill->helptext[0] != '\0' )
      fprintf( fpout, "Helptext     %s~\n", strip_cr( skill->helptext ) );
   fprintf( fpout, "%s", "End\n\n" );
}

/*
 * Save the skill table to disk
 */
const int SKILLVERSION = 5;
/* Updated to 1 for position text - Samson 4-26-00 */
/* Updated to 2 for bitset flags - Samson 7-10-04 */
/* Updated to 3 for AFF_NONE insertion - Samson 7-27-04 */
/* Updated to 4 for bug corrections - Samson 1-28-06 */
// Updated to 5 because Samson got stupid. Again. *ugh* - Samson 8/11/07
void save_skill_table( void )
{
   int x;
   FILE *fpout;

   if( !( fpout = fopen( SKILL_FILE, "w" ) ) )
   {
      perror( SKILL_FILE );
      bug( "%s: Cannot open skills.dat for writing", __func__ );
      return;
   }

   fprintf( fpout, "#VERSION %d\n", SKILLVERSION );

   for( x = 0; x < num_skills; ++x )
   {
      if( !skill_table[x]->name || skill_table[x]->name[0] == '\0' )
         break;
      fprintf( fpout, "%s", "#SKILL\n" );
      fwrite_skill( fpout, skill_table[x] );
   }
   fprintf( fpout, "%s", "#END\n" );
   FCLOSE( fpout );
}

/*
 * Save the herb table to disk
 */
/* Uses the same format as skills, therefore the skill version applies */
void save_herb_table(  )
{
   int x;
   FILE *fpout;

   if( !( fpout = fopen( HERB_FILE, "w" ) ) )
   {
      perror( HERB_FILE );
      bug( "%s: Cannot open herbs.dat for writing", __func__ );
      return;
   }

   fprintf( fpout, "#VERSION %d\n", SKILLVERSION );

   for( x = 0; x < top_herb; ++x )
   {
      if( !herb_table[x]->name || herb_table[x]->name[0] == '\0' )
         break;
      fprintf( fpout, "%s", "#HERB\n" );
      fwrite_skill( fpout, herb_table[x] );
   }
   fprintf( fpout, "%s", "#END\n" );
   FCLOSE( fpout );
}

skill_type *fread_skill( FILE * fp, int version )
{
   skill_type *skill = new skill_type;

   for( int x = 0; x < MAX_CLASS; ++x )
   {
      skill->skill_level[x] = LEVEL_IMMORTAL;
      skill->skill_adept[x] = 95;
   }

   for( int x = 0; x < MAX_RACE; ++x )
   {
      skill->race_level[x] = LEVEL_IMMORTAL;
      skill->race_adept[x] = 95;
   }
   skill->guild = -1;
   skill->ego = -2;

   for( ;; )
   {
      const char *word = ( feof( fp ) ? "End" : fread_word( fp ) );

      if( word[0] == '\0' )
      {
         bug( "%s: EOF encountered reading file!", __func__ );
         word = "End";
      }

      switch ( UPPER( word[0] ) )
      {
         default:
            bug( "%s: no match: %s", __func__, word );
            fread_to_eol( fp );
            break;

         case '*':
            fread_to_eol( fp );
            break;

         case 'A':
            KEY( "Author", skill->author, fread_string( fp ) );
            if( !str_cmp( word, "Affect" ) )
            {
               smaug_affect *aff;
               char mod[MIL];

               aff = new smaug_affect;
               aff->duration = str_dup( fread_word( fp ) );
               aff->location = fread_number( fp );
               mudstrlcpy( mod, fread_word( fp ), MIL );

               // Conversion needed because Samson was stupid and didn't think. Again. *sigh*
               if( version < 5 )
               {
                  if( aff->location == APPLY_INT )
                     aff->location = APPLY_DEX;
                  else if( aff->location == APPLY_WIS )
                     aff->location = APPLY_INT;
                  else if( aff->location == APPLY_DEX )
                     aff->location = APPLY_WIS;
                  else if( aff->location == APPLY_CHA )
                     aff->location = APPLY_SEX;
                  else if( aff->location == APPLY_LCK )
                     aff->location = APPLY_CLASS;
                  else if( aff->location == APPLY_SEX )
                     aff->location = APPLY_CHA;
                  else if( aff->location == APPLY_CLASS )
                     aff->location = APPLY_LCK;
               }

               if( version < 2 )
               {
                  if( version < 1 )
                  {
                     if( aff->location == APPLY_AFFECT || aff->location == APPLY_EXT_AFFECT )
                     {
                        int mvalue = atoi( mod );

                        mudstrlcpy( mod, aff_flags[mvalue], MIL );
                     }
                     if( aff->location == APPLY_RESISTANT || aff->location == APPLY_IMMUNE || aff->location == APPLY_ABSORB || aff->location == APPLY_SUSCEPTIBLE )
                     {
                        int mvalue = atoi( mod );

                        mudstrlcpy( mod, flag_string( mvalue, old_ris_flags ), MIL );
                     }
                  }
                  else
                  {
                     if( ( aff->location == APPLY_RESISTANT && is_number( mod ) )
                         || ( aff->location == APPLY_IMMUNE && is_number( mod ) )
                         || ( aff->location == APPLY_ABSORB && is_number( mod ) ) || ( aff->location == APPLY_SUSCEPTIBLE && is_number( mod ) ) )
                     {
                        int mvalue = atoi( mod );

                        mudstrlcpy( mod, flag_string( mvalue, old_ris_flags ), MIL );
                     }
                  }
               }

               if( aff->location == APPLY_AFFECT )
                  aff->location = APPLY_EXT_AFFECT;
               aff->modifier = str_dup( mod );
               aff->bit = fread_number( fp );
               if( version < 3 && aff->bit > -1 )
                  ++aff->bit;
               skill->affects.push_back( aff );
               break;
            }
            break;

         case 'C':
            if( !str_cmp( word, "Class" ) )
            {
               int Class = fread_number( fp );

               skill->skill_level[Class] = fread_number( fp );
               skill->skill_adept[Class] = fread_number( fp );
               break;
            }
            if( !str_cmp( word, "Code" ) )
            {
               SPELL_FUN *spellfun = NULL;
               DO_FUN *dofun = NULL;
               char *w = fread_word( fp );

               if( validate_spec_fun( w ) )
               {
                  bug( "%s: ERROR: Trying to assign spec_fun to skill/spell %s", __func__, w );
                  skill->skill_fun = skill_notfound;
                  skill->spell_fun = spell_notfound;
               }
               else if( !str_prefix( "do_", w ) && ( dofun = skill_function( w ) ) != skill_notfound )
               {
                  skill->skill_fun = dofun;
                  skill->spell_fun = NULL;
                  skill->skill_fun_name = str_dup( w );
               }
               else if( str_prefix( "do_", w ) && ( spellfun = spell_function( w ) ) != spell_notfound )
               {
                  skill->spell_fun = spellfun;
                  skill->skill_fun = NULL;
                  skill->spell_fun_name = str_dup( w );
               }
               else
               {
                  bug( "%s: unknown skill/spell %s", __func__, w );
                  skill->spell_fun = spell_null;
               }
               break;
            }
            KEY( "Components", skill->components, fread_string_nohash( fp ) );
            break;

         case 'D':
            KEY( "Dammsg", skill->noun_damage, fread_string_nohash( fp ) );
            KEY( "Dice", skill->dice, fread_string_nohash( fp ) );
            KEY( "Diechar", skill->die_char, fread_string_nohash( fp ) );
            KEY( "Dieroom", skill->die_room, fread_string_nohash( fp ) );
            KEY( "Dievict", skill->die_vict, fread_string_nohash( fp ) );
            KEY( "Difficulty", skill->difficulty, fread_number( fp ) );
            break;

         case 'E':
            KEY( "Ego", skill->ego, fread_number( fp ) );
            if( !str_cmp( word, "End" ) )
            {
               if( skill->saves != 0 && SPELL_SAVE( skill ) == SE_NONE )
               {
                  bug( "%s: %s: Has saving throw (%d) with no saving effect.", __func__, skill->name, skill->saves );
                  SET_SSAV( skill, SE_NEGATE );
               }
               if( !skill->author )
                  skill->author = STRALLOC( "Smaug" );
               if( skill->ego > 90 )
                  skill->ego /= 1000;
               return skill;
            }
            break;

         case 'F':
            if( !str_cmp( word, "Flags" ) )
            {
               if( version < 4 )
                  skill->flags = fread_number( fp );
               else
                  flag_set( fp, skill->flags, spell_flag );
               break;
            }
            break;

         case 'G':
            KEY( "Guild", skill->guild, fread_number( fp ) );
            break;

         case 'H':
            KEY( "Helptext", skill->helptext, fread_string_nohash( fp ) );
            KEY( "Hitchar", skill->hit_char, fread_string_nohash( fp ) );
            KEY( "Hitdest", skill->hit_dest, fread_string_nohash( fp ) );
            KEY( "Hitroom", skill->hit_room, fread_string_nohash( fp ) );
            KEY( "Hitvict", skill->hit_vict, fread_string_nohash( fp ) );
            break;

         case 'I':
            KEY( "Immchar", skill->imm_char, fread_string_nohash( fp ) );
            KEY( "Immroom", skill->imm_room, fread_string_nohash( fp ) );
            KEY( "Immvict", skill->imm_vict, fread_string_nohash( fp ) );
            KEY( "Info", skill->info, fread_number( fp ) );
            break;

         case 'M':
            KEY( "Mana", skill->min_mana, fread_number( fp ) );
            if( !str_cmp( word, "Minlevel" ) )
            {
               fread_to_eol( fp );
               break;
            }
            if( !str_cmp( word, "Minpos" ) )
            {
               if( version < 1 )
               {
                  skill->minimum_position = fread_number( fp );
                  if( skill->minimum_position < 100 )
                  {
                     switch ( skill->minimum_position )
                     {
                        default:
                        case 0:
                        case 1:
                        case 2:
                        case 3:
                        case 4:
                           break;
                        case 5:
                           skill->minimum_position = 6;
                           break;
                        case 6:
                           skill->minimum_position = 8;
                           break;
                        case 7:
                           skill->minimum_position = 9;
                           break;
                        case 8:
                           skill->minimum_position = 12;
                           break;
                        case 9:
                           skill->minimum_position = 13;
                           break;
                        case 10:
                           skill->minimum_position = 14;
                           break;
                        case 11:
                           skill->minimum_position = 15;
                           break;
                     }
                  }
                  else
                     skill->minimum_position -= 100;
                  break;
               }
               else
               {
                  int position;

                  position = get_npc_position( fread_flagstring( fp ) );

                  if( position < 0 || position >= POS_MAX )
                  {
                     bug( "%s: Skill %s has invalid position! Defaulting to standing.", __func__, skill->name );
                     position = POS_STANDING;
                  }
                  skill->minimum_position = position;
                  break;
               }
            }
            KEY( "Misschar", skill->miss_char, fread_string_nohash( fp ) );
            KEY( "Missroom", skill->miss_room, fread_string_nohash( fp ) );
            KEY( "Missvict", skill->miss_vict, fread_string_nohash( fp ) );
            break;

         case 'N':
            KEY( "Name", skill->name, fread_string_nohash( fp ) );
            break;

         case 'P':
            KEY( "Participants", skill->participants, fread_number( fp ) );
            break;

         case 'R':
            KEY( "Range", skill->range, fread_number( fp ) );
            KEY( "Rounds", skill->beats, fread_number( fp ) );
            KEY( "Rent", skill->ego, fread_number( fp ) );
            if( !str_cmp( word, "Race" ) )
            {
               int race = fread_number( fp );

               skill->race_level[race] = fread_number( fp );
               skill->race_adept[race] = fread_number( fp );
               break;
            }
            break;

         case 'S':
            KEY( "Slot", skill->slot, fread_number( fp ) );
            KEY( "Saves", skill->saves, fread_number( fp ) );
            break;

         case 'T':
            KEY( "Target", skill->target, fread_number( fp ) );
            KEY( "Teachers", skill->teachers, fread_string_nohash( fp ) );
            KEY( "Type", skill->type, get_skill( fread_word( fp ) ) );
            break;

         case 'V':
            KEY( "Value", skill->value, fread_number( fp ) );
            break;

         case 'W':
            KEY( "Wearoff", skill->msg_off, fread_string_nohash( fp ) );
            break;
      }
   }
}

void load_skill_table( void )
{
   FILE *fp;
   int version = 0;

   if( ( fp = fopen( SKILL_FILE, "r" ) ) != NULL )
   {
      fpArea = fp;
      num_skills = 0;
      for( ;; )
      {
         char letter;
         char *word;

         letter = fread_letter( fp );
         if( letter == '*' )
         {
            fread_to_eol( fp );
            continue;
         }

         if( letter != '#' )
         {
            bug( "%s: # not found.", __func__ );
            break;
         }

         word = fread_word( fp );
         if( !str_cmp( word, "VERSION" ) )
         {
            version = fread_number( fp );
            continue;
         }
         if( !str_cmp( word, "SKILL" ) )
         {
            if( num_skills >= MAX_SKILL )
            {
               bug( "%s: more skills than MAX_SKILL %d", __func__, MAX_SKILL );
               FCLOSE( fp );
               fpArea = NULL;
               return;
            }
            skill_table[num_skills++] = fread_skill( fp, version );
            continue;
         }
         else if( !str_cmp( word, "END" ) )
            break;
         else
         {
            bug( "%s: bad section: %s", __func__, word );
            continue;
         }
      }
      FCLOSE( fp );
      fpArea = NULL;
   }
   else
   {
      perror( SKILL_FILE );
      bug( "%s: Cannot open skills.dat", __func__ );
      exit( 1 );
   }
}

void load_herb_table(  )
{
   FILE *fp;
   int version = 0;

   if( ( fp = fopen( HERB_FILE, "r" ) ) != NULL )
   {
      top_herb = 0;
      for( ;; )
      {
         char letter;
         char *word;

         letter = fread_letter( fp );
         if( letter == '*' )
         {
            fread_to_eol( fp );
            continue;
         }

         if( letter != '#' )
         {
            bug( "%s: # not found.", __func__ );
            break;
         }

         word = fread_word( fp );
         if( !str_cmp( word, "VERSION" ) )
         {
            version = fread_number( fp );
            continue;
         }
         if( !str_cmp( word, "HERB" ) )
         {
            if( top_herb >= MAX_HERB )
            {
               bug( "%s: more herbs than MAX_HERB %d", __func__, MAX_HERB );
               FCLOSE( fp );
               return;
            }
            herb_table[top_herb++] = fread_skill( fp, version );
            if( herb_table[top_herb - 1]->slot == 0 )
               herb_table[top_herb - 1]->slot = top_herb - 1;
            continue;
         }
         else if( !str_cmp( word, "END" ) )
            break;
         else
         {
            bug( "%s: bad section: %s", __func__, word );
            continue;
         }
      }
      FCLOSE( fp );
   }
   else
   {
      bug( "%s: Cannot open herbs.dat", __func__ );
      exit( 1 );
   }
}

void free_skills( void )
{
   int hash = 0;

   for( hash = 0; hash < num_skills; ++hash )
      deleteptr( skill_table[hash] );

   for( hash = 0; hash < top_herb; ++hash )
      deleteptr( herb_table[hash] );

   for( hash = 0; hash < top_disease; ++hash )
      deleteptr( disease_table[hash] );
}

/*
 * Dummy function - Unused parameter should be left alone, as-is.
 */
void skill_notfound( char_data * ch, string argument )
{
   ch->print( "Huh?\r\n" );
}

/*
 * New skill computation, based on character's current learned value
 * as well as character's intelligence + wisdom. Samson 3-13-98.
 *
 * Modified 5-28-99 to conform with Crystal Shard advancement code,
 * int + wis factor removed from consideration. Difficulty rating removed
 * from consideration - Samson
 */
void char_data::learn_racials( int sn )
{
   int adept, gain, sklvl;

   sklvl = skill_table[sn]->race_level[race];
   if( sklvl == 0 )
      sklvl = level;

   if( isnpc(  ) || pcdata->learned[sn] == 0 || level < skill_table[sn]->race_level[race] )
      return;

   if( number_percent(  ) > pcdata->learned[sn] / 2 )
   {
      adept = skill_table[sn]->race_adept[race];

      if( pcdata->learned[sn] < adept )
      {
         pcdata->learned[sn] += 1;

         if( !skill_table[sn]->skill_fun && str_cmp( skill_table[sn]->spell_fun_name, "spell_null" ) )
            print( "You learn from your mistake.\r\n" );

         if( pcdata->learned[sn] == adept )  /* fully learned! */
         {
            gain = sklvl * 1000;
            if( Class == CLASS_MAGE )
               gain = gain * 2;
            printf( "&WYou are now an adept of %s! You gain %d bonus experience!\r\n", skill_table[sn]->name, gain );
            gain_exp( gain );
         }
      }
   }
}

void char_data::learn_from_failure( int sn )
{
   int adept, gain, lchance, sklvl;

   lchance = number_range( 1, 2 );

   if( lchance == 2 )   /* Time to cut into the speed of advancement a bit */
      return;

   if( skill_table[sn]->type == SKILL_RACIAL )
   {
      learn_racials( sn );
      return;
   }

   /*
    * Edited by Tarl, 7th June 2003 to make weapon skills increase 1 in 20 hits or so 
    * Hijacked by Samson 2/23/06 to include fighting styles under the new combat skill group
    */
   if( skill_table[sn]->type == SKILL_COMBAT )
   {
      lchance = number_range( 1, 10 );
      if( lchance != 5 )
         return;
   }

   sklvl = skill_table[sn]->skill_level[Class];
   if( sklvl == 0 )
      sklvl = 1;

   if( isnpc(  ) || pcdata->learned[sn] == 0 || level < skill_table[sn]->skill_level[Class] )
      return;

   if( number_percent(  ) > pcdata->learned[sn] / 2 )
   {
      adept = GET_ADEPT( sn );
      if( pcdata->learned[sn] < adept )
      {
         if( skill_table[sn]->type == SKILL_COMBAT )
            printf( "&RYou have improved in %s!&D\r\n", skill_table[sn]->name );

         pcdata->learned[sn] += 1;

         if( !skill_table[sn]->skill_fun && str_cmp( skill_table[sn]->spell_fun_name, "spell_null" ) )
            print( "You learn from your mistake.\r\n" );

         if( pcdata->learned[sn] == adept )  /* fully learned! */
         {
            gain = sklvl * 1000;

            if( Class == CLASS_MAGE )
               gain = gain * 2;
            printf( "&WYou are now an adept of %s! You gain %d bonus experience!\r\n", skill_table[sn]->name, gain );
            gain_exp( gain );
         }
      }
   }
}

/* New command to view a player's skills - Samson 4-13-98 */
CMDF( do_viewskills )
{
   char_data *victim;
   int sn, col;

   if( argument.empty(  ) )
   {
      ch->print( "&zSyntax: skills <player>.\r\n" );
      return;
   }

   if( !( victim = ch->get_char_world( argument ) ) )
   {
      ch->print( "No such person in the game.\r\n" );
      return;
   }

   col = 0;

   if( !victim->isnpc(  ) )
   {
      ch->set_color( AT_SKILL );
      for( sn = 0; sn < num_skills && skill_table[sn] && skill_table[sn]->name; ++sn )
      {
         if( skill_table[sn]->name == NULL )
            break;
         if( victim->pcdata->learned[sn] == 0 )
            continue;

         ch->printf( "%20s %3d%% ", skill_table[sn]->name, victim->pcdata->learned[sn] );

         if( ++col % 3 == 0 )
            ch->print( "\r\n" );
      }
   }
}

int get_ssave( const string & name )
{
   for( size_t x = 0; x < sizeof( spell_saves ) / sizeof( spell_saves[0] ); ++x )
      if( !str_cmp( name, spell_saves[x] ) )
         return x;
   return -1;
}

int get_starget( const string & name )
{
   for( size_t x = 0; x < sizeof( target_type ) / sizeof( target_type[0] ); ++x )
      if( !str_cmp( name, target_type[x] ) )
         return x;
   return -1;
}

int get_sflag( const string & name )
{
   for( size_t x = 0; x < sizeof( spell_flag ) / sizeof( spell_flag[0] ); ++x )
      if( !str_cmp( name, spell_flag[x] ) )
         return x;
   return -1;
}

int get_sdamage( const string & name )
{
   for( size_t x = 0; x < sizeof( spell_damage ) / sizeof( spell_damage[0] ); ++x )
      if( !str_cmp( name, spell_damage[x] ) )
         return x;
   return -1;
}

int get_saction( const string & name )
{
   for( size_t x = 0; x < sizeof( spell_action ) / sizeof( spell_action[0] ); ++x )
      if( !str_cmp( name, spell_action[x] ) )
         return x;
   return -1;
}

int get_ssave_effect( const string & name )
{
   for( size_t x = 0; x < sizeof( spell_save_effect ) / sizeof( spell_save_effect[0] ); ++x )
      if( !str_cmp( name, spell_save_effect[x] ) )
         return x;
   return -1;
}

int get_spower( const string & name )
{
   for( size_t x = 0; x < sizeof( spell_power ) / sizeof( spell_power[0] ); ++x )
      if( !str_cmp( name, spell_power[x] ) )
         return x;
   return -1;
}

int get_sclass( const string & name )
{
   for( size_t x = 0; x < sizeof( spell_class ) / sizeof( spell_class[0] ); ++x )
      if( !str_cmp( name, spell_class[x] ) )
         return x;
   return -1;
}

int skill_number( const string & argument )
{
   int sn;

   if( ( sn = skill_lookup( argument ) ) >= 0 )
      return sn;
   return -1;
}

bool get_skill_help( char_data * ch, const string & argument )
{
   skill_type *skill = NULL;
   char buf[MSL], target[MSL];
   int sn;

   if( ( sn = skill_number( argument ) ) >= 0 )
      skill = skill_table[sn];

   // Not a skill/spell, drop back to regular help
   if( sn < 0 || !skill || skill->type == SKILL_HERB )
      return false;

   target[0] = '\0';
   switch ( skill->target )
   {
      default:
         break;

      case TAR_CHAR_OFFENSIVE:
         mudstrlcpy( target, "<victim>", MSL );
         break;

      case TAR_CHAR_SELF:
         mudstrlcpy( target, "<self>", MSL );
         break;

      case TAR_OBJ_INV:
         mudstrlcpy( target, "<object>", MSL );
         break;
   }

   snprintf( buf, MSL, "cast '%s'", skill->name );
   ch->printf( "Usage        : %s %s\r\n", skill->type == SKILL_SPELL ? buf : skill->name, target );

   if( skill->affects.empty(  ) )
      ch->print( "Duration     : Instant\r\n" );
   else
   {
      list < smaug_affect * >::iterator paf;
      bool found = false;

      for( paf = skill->affects.begin(  ); paf != skill->affects.end(  ); ++paf )
      {
         smaug_affect *af = *paf;

         // Make sure duration isn't null, and is not 0
         if( af->duration && af->duration[0] != '\0' && af->duration[0] != '0' )
         {
            if( !found )
            {
               ch->print( "Duration     :\r\n" );
               found = true;
            }
            ch->printf( "   Affect    : '%s' for '%s' rounds.\r\n", aff_flags[af->bit], af->duration );
         }
      }
   }

   ch->printf( "%-5s Level  : ", skill->type == SKILL_RACIAL ? "Race" : "Class" );

   bool firstpass = true;
   if( skill->type != SKILL_RACIAL )
   {
      for( int iClass = 0; iClass < MAX_PC_CLASS; ++iClass )
      {
         if( skill->skill_level[iClass] > LEVEL_AVATAR )
            continue;

         if( firstpass )
         {
            snprintf( buf, MSL, "%s ", class_table[iClass]->who_name );
            firstpass = false;
         }
         else
            snprintf( buf, MSL, ", %s ", class_table[iClass]->who_name );

         snprintf( buf + strlen( buf ), MSL - strlen( buf ), "%d", skill->skill_level[iClass] );
         ch->print( buf );
      }
   }
   else
   {
      for( int iRace = 0; iRace < MAX_PC_RACE; ++iRace )
      {
         if( skill->race_level[iRace] > LEVEL_AVATAR )
            continue;

         if( firstpass )
         {
            snprintf( buf, MSL, "%s ", race_table[iRace]->race_name );
            firstpass = false;
         }
         else
            snprintf( buf, MSL, ", %s ", race_table[iRace]->race_name );

         snprintf( buf + strlen( buf ), MSL - strlen( buf ), "%d", skill->race_level[iRace] );
         ch->print( buf );
      }
   }
   ch->print( "\r\n" );

   if( skill->dice )
      ch->printf( "Damage       : %s\r\n", skill->dice );

   if( skill->type == SKILL_SPELL )
   {
      if( skill->saves > 0 )
         ch->printf( "Save         : vs %s for %s\r\n", spell_saves[( int )skill->saves], spell_save_effect[SPELL_SAVE( skill )] );
      ch->printf( "Minimum cost : %d mana\r\n", skill->min_mana );
   }

   if( skill->helptext )
      ch->printf( "\r\n%s\r\n", skill->helptext );
   return true;
}

bool is_legal_kill( char_data * ch, char_data * vch )
{
   if( ch->isnpc(  ) || vch->isnpc(  ) )
      return true;
   if( !ch->IS_PKILL(  ) || !vch->IS_PKILL(  ) )
      return false;
   if( ch->pcdata->clan && ch->pcdata->clan == vch->pcdata->clan )
      return false;
   return true;
}

/*  New check to see if you can use skills to support morphs --Shaddai */
bool can_use_skill( char_data * ch, int percent, int gsn )
{
   bool check = false;

   if( ch->isnpc(  ) && percent < 85 )
      check = true;
   else if( !ch->isnpc(  ) && percent < ch->LEARNED( gsn ) )
      check = true;
   else if( ch->morph && ch->morph->morph && ch->morph->morph->skills &&
            ch->morph->morph->skills[0] != '\0' && hasname( ch->morph->morph->skills, skill_table[gsn]->name ) && percent < 85 )
      check = true;

   if( ch->morph && ch->morph->morph && ch->morph->morph->no_skills &&
       ch->morph->morph->no_skills[0] != '\0' && hasname( ch->morph->morph->no_skills, skill_table[gsn]->name ) )
      check = false;

   return check;
}

bool check_ability( char_data * ch, const string & command, const string & argument )
{
   int sn, mana;
   struct timeval time_used;

   /*
    * bsearch for the ability
    */
   sn = find_ability( ch, command, true );

   if( sn == -1 )
      return false;

   // some additional checks
   if( !( skill_table[sn]->skill_fun || skill_table[sn]->spell_fun != spell_null ) || !can_use_skill( ch, 0, sn ) )
   {
      return false;
   }

   if( !check_pos( ch, skill_table[sn]->minimum_position ) )
      return true;

   if( ch->isnpc(  ) && ( ch->has_aflag( AFF_CHARM ) || ch->has_aflag( AFF_POSSESS ) ) )
   {
      ch->print( "For some reason, you seem unable to perform that...\r\n" );
      act( AT_GREY, "$n wanders around aimlessly.", ch, NULL, NULL, TO_ROOM );
      return true;
   }

   /*
    * check if mana is required 
    */
   if( skill_table[sn]->min_mana )
   {
      mana = ch->isnpc(  )? 0 : UMAX( skill_table[sn]->min_mana, 100 / ( 2 + ch->level - skill_table[sn]->race_level[ch->race] ) );

      if( !ch->isnpc(  ) && ch->mana < mana )
      {
         ch->print( "You don't have enough mana.\r\n" );
         return true;
      }
   }
   else
      mana = 0;

   /*
    * Is this a real do-fun, or a really a spell?
    */
   if( !skill_table[sn]->skill_fun )
   {
      ch_ret retcode = rNONE;
      void *vo = NULL;
      char_data *victim = NULL;
      obj_data *obj = NULL;

      target_name.clear(  );

      switch ( skill_table[sn]->target )
      {
         default:
            bug( "%s: bad target for sn %d.", __func__, sn );
            ch->print( "Something went wrong...\r\n" );
            return true;

         case TAR_IGNORE:
            vo = NULL;
            if( argument.empty(  ) )
            {
               if( ( victim = ch->who_fighting(  ) ) != NULL )
                  target_name = victim->name;
            }
            else
               target_name = argument;
            break;

         case TAR_CHAR_OFFENSIVE:
            if( argument.empty(  ) && !( victim = ch->who_fighting(  ) ) )
            {
               ch->printf( "Confusion overcomes you as your '%s' has no target.\r\n", skill_table[sn]->name );
               return true;
            }
            else if( !argument.empty(  ) && !( victim = ch->get_char_room( argument ) ) )
            {
               ch->print( "They aren't here.\r\n" );
               return true;
            }

            if( is_safe( ch, victim ) )
               return true;

            if( ch == victim && SPELL_FLAG( skill_table[sn], SF_NOSELF ) )
            {
               ch->print( "You can't target yourself!\r\n" );
               return true;
            }

            if( !ch->isnpc(  ) )
            {
               if( !victim->isnpc(  ) )
               {
                  if( ch->get_timer( TIMER_PKILLED ) > 0 )
                  {
                     ch->print( "You have been killed in the last 5 minutes.\r\n" );
                     return true;
                  }

                  if( victim->get_timer( TIMER_PKILLED ) > 0 )
                  {
                     ch->print( "This player has been killed in the last 5 minutes.\r\n" );
                     return true;
                  }

                  if( victim != ch )
                     ch->print( "You really shouldn't do this to another player...\r\n" );
               }

               if( ch->has_aflag( AFF_CHARM ) && ch->master == victim )
               {
                  ch->print( "You can't do that on your own follower.\r\n" );
                  return true;
               }
            }

            if( check_illegal_pk( ch, victim ) )
            {
               ch->print( "You can't do that to another player!\r\n" );
               return true;
            }
            vo = ( void * )victim;
            break;

         case TAR_CHAR_DEFENSIVE:
            if( !argument.empty(  ) && !( victim = ch->get_char_room( argument ) ) )
            {
               ch->print( "They aren't here.\r\n" );
               return true;
            }
            if( !victim )
               victim = ch;

            if( ch == victim && SPELL_FLAG( skill_table[sn], SF_NOSELF ) )
            {
               ch->print( "You can't target yourself!\r\n" );
               return true;
            }
            vo = ( void * )victim;
            break;

         case TAR_CHAR_SELF:
            victim = ch;
            vo = ( void * )ch;
            break;

         case TAR_OBJ_INV:
            if( !( obj = ch->get_obj_carry( argument ) ) )
            {
               ch->print( "You can't find that.\r\n" );
               return true;
            }
            vo = ( void * )obj;
            break;
      }

      /*
       * waitstate 
       */
      ch->WAIT_STATE( skill_table[sn]->beats );
      /*
       * check for failure 
       */
      if( ( number_percent(  ) + skill_table[sn]->difficulty * 5 ) > ( ch->isnpc(  )? 75 : ch->LEARNED( sn ) ) )
      {
         failed_casting( skill_table[sn], ch, victim, obj );
         ch->learn_from_failure( sn );
         if( mana )
            ch->mana -= mana / 2;
         return true;
      }
      if( mana )
         ch->mana -= mana;

      start_timer( &time_used );
      retcode = ( *skill_table[sn]->spell_fun ) ( sn, ch->level, ch, vo );
      end_timer( &time_used );

      if( retcode == rCHAR_DIED || retcode == rERROR )
         return true;

      if( ch->char_died(  ) )
         return true;

      if( retcode == rSPELL_FAILED )
      {
         ch->learn_from_failure( sn );
         retcode = rNONE;
      }

      if( skill_table[sn]->target == TAR_CHAR_OFFENSIVE && victim != ch && !victim->char_died(  ) )
      {
         list < char_data * >::iterator ich;

         for( ich = ch->in_room->people.begin(  ); ich != ch->in_room->people.end(  ); )
         {
            char_data *vch = *ich;
            ++ich;

            if( victim == vch && !victim->fighting && victim->master != ch )
            {
               retcode = multi_hit( victim, ch, TYPE_UNDEFINED );
               break;
            }
         }
      }
      return true;
   }

   if( mana )
      ch->mana -= mana;

   ch->prev_cmd = ch->last_cmd;  /* haus, for automapping */
   ch->last_cmd = skill_table[sn]->skill_fun;
   start_timer( &time_used );
   ( *skill_table[sn]->skill_fun ) ( ch, argument );
   end_timer( &time_used );

   return true;
}

/*
 * Perform a binary search on a section of the skill table
 * Each different section of the skill table is sorted alphabetically
 * Only match skills player knows - Thoric
 */
bool check_skill( char_data * ch, const string & command, const string & argument )
{
   int sn, mana;
   struct timeval time_used;

   /*
    * bsearch for the skill
    */
   sn = find_skill( ch, command, true );

   if( sn == -1 )
      return false;

   // some additional checks
   if( !( skill_table[sn]->skill_fun || skill_table[sn]->spell_fun != spell_null ) || !can_use_skill( ch, 0, sn ) )
      return false;

   if( !check_pos( ch, skill_table[sn]->minimum_position ) )
      return true;

   if( ch->isnpc(  ) && ( ch->has_aflag( AFF_CHARM ) || ch->has_aflag( AFF_POSSESS ) ) )
   {
      ch->print( "For some reason, you seem unable to perform that...\r\n" );
      act( AT_GREY, "$n wanders around aimlessly.", ch, NULL, NULL, TO_ROOM );
      return true;
   }

   /*
    * check if mana is required 
    */
   if( skill_table[sn]->min_mana )
   {
      mana = ch->isnpc(  )? 0 : UMAX( skill_table[sn]->min_mana, 100 / ( 2 + ch->level - skill_table[sn]->skill_level[ch->Class] ) );

      if( !ch->isnpc(  ) && ch->mana < mana )
      {
         ch->print( "You don't have enough mana.\r\n" );
         return true;
      }
   }
   else
      mana = 0;

   /*
    * Is this a real do-fun, or a really a spell?
    */
   if( !skill_table[sn]->skill_fun )
   {
      ch_ret retcode = rNONE;
      void *vo = NULL;
      char_data *victim = NULL;
      obj_data *obj = NULL;

      target_name.clear(  );

      switch ( skill_table[sn]->target )
      {
         default:
            bug( "%s: bad target for sn %d.", __func__, sn );
            ch->print( "Something went wrong...\r\n" );
            return true;

         case TAR_IGNORE:
            vo = NULL;
            if( argument.empty(  ) )
            {
               if( ( victim = ch->who_fighting(  ) ) != NULL )
                  target_name = victim->name;
            }
            else
               target_name = argument;
            break;

         case TAR_CHAR_OFFENSIVE:
            if( argument.empty(  ) && !( victim = ch->who_fighting(  ) ) )
            {
               ch->printf( "Confusion overcomes you as your '%s' has no target.\r\n", skill_table[sn]->name );
               return true;
            }
            else if( !argument.empty(  ) && !( victim = ch->get_char_room( argument ) ) )
            {
               ch->print( "They aren't here.\r\n" );
               return true;
            }

            if( is_safe( ch, victim ) )
               return true;

            if( ch == victim && SPELL_FLAG( skill_table[sn], SF_NOSELF ) )
            {
               ch->print( "You can't target yourself!\r\n" );
               return true;
            }

            if( !ch->isnpc(  ) )
            {
               if( !victim->isnpc(  ) )
               {
                  if( ch->get_timer( TIMER_PKILLED ) > 0 )
                  {
                     ch->print( "You have been killed in the last 5 minutes.\r\n" );
                     return true;
                  }

                  if( victim->get_timer( TIMER_PKILLED ) > 0 )
                  {
                     ch->print( "This player has been killed in the last 5 minutes.\r\n" );
                     return true;
                  }

                  if( victim != ch )
                     ch->print( "You really shouldn't do this to another player...\r\n" );
               }

               if( ch->has_aflag( AFF_CHARM ) && ch->master == victim )
               {
                  ch->print( "You can't do that on your own follower.\r\n" );
                  return true;
               }
            }

            if( check_illegal_pk( ch, victim ) )
            {
               ch->print( "You can't do that to another player!\r\n" );
               return true;
            }
            vo = ( void * )victim;
            break;

         case TAR_CHAR_DEFENSIVE:
            if( !argument.empty(  ) && !( victim = ch->get_char_room( argument ) ) )
            {
               ch->print( "They aren't here.\r\n" );
               return true;
            }
            if( !victim )
               victim = ch;

            if( ch == victim && SPELL_FLAG( skill_table[sn], SF_NOSELF ) )
            {
               ch->print( "You can't target yourself!\r\n" );
               return true;
            }
            vo = ( void * )victim;
            break;

         case TAR_CHAR_SELF:
            victim = ch;
            vo = ( void * )ch;
            break;

         case TAR_OBJ_INV:
            if( !( obj = ch->get_obj_carry( argument ) ) )
            {
               ch->print( "You can't find that.\r\n" );
               return true;
            }
            vo = ( void * )obj;
            break;
      }

      /*
       * waitstate 
       */
      ch->WAIT_STATE( skill_table[sn]->beats );
      /*
       * check for failure 
       */
      if( ( number_percent(  ) + skill_table[sn]->difficulty * 5 ) > ( ch->isnpc(  )? 75 : ch->LEARNED( sn ) ) )
      {
         failed_casting( skill_table[sn], ch, victim, obj );
         ch->learn_from_failure( sn );
         if( mana )
            ch->mana -= mana / 2;
         return true;
      }
      if( mana )
         ch->mana -= mana;

      start_timer( &time_used );
      retcode = ( *skill_table[sn]->spell_fun ) ( sn, ch->level, ch, vo );
      end_timer( &time_used );

      if( retcode == rCHAR_DIED || retcode == rERROR )
         return true;

      if( ch->char_died(  ) )
         return true;

      if( retcode == rSPELL_FAILED )
      {
         ch->learn_from_failure( sn );
         retcode = rNONE;
      }

      if( skill_table[sn]->target == TAR_CHAR_OFFENSIVE && victim != ch && !victim->char_died(  ) )
      {
         list < char_data * >::iterator ich;

         for( ich = ch->in_room->people.begin(  ); ich != ch->in_room->people.end(  ); )
         {
            char_data *vch = *ich;
            ++ich;

            if( victim == vch && !victim->fighting && victim->master != ch )
            {
               retcode = multi_hit( victim, ch, TYPE_UNDEFINED );
               break;
            }
         }
      }
      return true;
   }

   if( mana )
      ch->mana -= mana;

   ch->prev_cmd = ch->last_cmd;  /* haus, for automapping */
   ch->last_cmd = skill_table[sn]->skill_fun;
   start_timer( &time_used );
   ( *skill_table[sn]->skill_fun ) ( ch, argument );
   end_timer( &time_used );

   return true;
}

/* A buncha different stuff added/subtracted/rearranged in do_slist to remove 
 * annoying, but harmless bugs, as well as provide better functionality - Adjani 12-05-2002 
 */
CMDF( do_slist )
{
   int sn, i, lFound;
   string arg1, arg2;
   char skn[MIL];
   int lowlev, hilev;
   short lasttype = SKILL_SPELL;
   int cl = ch->Class;

   if( ch->isnpc(  ) )
      return;

   lowlev = 1;
   hilev = LEVEL_AVATAR;

   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );

   if( !arg1.empty(  ) && !is_number( arg1 ) )
   {
      cl = get_npc_class( arg1 );
      if( cl < 0 || cl > MAX_CLASS )
      {
         ch->printf( "%s isn't a valid class!\r\n", arg1.c_str(  ) );
         return;
      }

      if( !arg2.empty(  ) )
      {
         if( is_number( arg2 ) )
         {
            lowlev = atoi( arg2.c_str(  ) );
            if( lowlev < 1 || lowlev > LEVEL_AVATAR )
            {
               ch->printf( "%d is out of range. Only valid between 1 and %d\r\n", lowlev, LEVEL_AVATAR );
               return;
            }
         }
      }

      if( !argument.empty(  ) )
      {
         if( is_number( argument ) )
         {
            hilev = atoi( argument.c_str(  ) );
            if( hilev < 1 || hilev > LEVEL_AVATAR )
            {
               ch->printf( "%d is out of range. Only valid between 1 and %d\r\n", hilev, LEVEL_AVATAR );
               return;
            }
         }
      }
   }
   else if( !arg1.empty(  ) && is_number( arg1 ) )
   {
      lowlev = atoi( arg1.c_str(  ) );
      if( lowlev < 1 || lowlev > LEVEL_AVATAR )
      {
         ch->printf( "%d is out of range. Only valid between 1 and %d\r\n", lowlev, LEVEL_AVATAR );
         return;
      }

      if( !arg2.empty(  ) && is_number( arg2 ) )
      {
         hilev = atoi( arg2.c_str(  ) );
         if( hilev < 1 || hilev > LEVEL_AVATAR )
         {
            ch->printf( "%d is out of range. Only valid between 1 and %d\r\n", hilev, LEVEL_AVATAR );
            return;
         }
      }
   }

   if( lowlev > hilev )
      lowlev = hilev;

   ch->set_pager_color( AT_MAGIC );
   ch->pagerf( "%s Spell & Skill List\r\n", class_table[cl]->who_name );
   ch->pager( "--------------------------------------\r\n" );

   for( i = lowlev; i <= hilev; ++i )
   {
      lFound = 0;
      mudstrlcpy( skn, "Spell", MIL );
      for( sn = 0; sn < num_skills; ++sn )
      {
         if( !skill_table[sn]->name )
            break;

         if( skill_table[sn]->type != lasttype )
         {
            lasttype = skill_table[sn]->type;
            mudstrlcpy( skn, skill_tname[lasttype], MIL );
         }

         if( ch->pcdata->learned[sn] <= 0 && SPELL_FLAG( skill_table[sn], SF_SECRETSKILL ) )
            continue;

         if( i == skill_table[sn]->skill_level[cl] || i == skill_table[sn]->race_level[ch->race] )
         {
            int xx = cl;

            if( skill_table[sn]->type == SKILL_RACIAL )
               xx = ch->race;

            if( !lFound )
            {
               lFound = 1;
               ch->pagerf( "Level %d\r\n", i );
            }

            ch->pagerf( "%7s: %20.20s \t Current: %-3d Max: %-3d  MinPos: %s \r\n",
                        skn, skill_table[sn]->name, ch->pcdata->learned[sn], skill_table[sn]->skill_adept[xx], npc_position[skill_table[sn]->minimum_position] );
         }
      }
   }
}

/*
 * Lookup a skills information
 * High god command
 */
CMDF( do_slookup )
{
   char buf[MSL];
   int sn;
   int iClass, iRace;
   skill_type *skill = NULL;

   if( argument.empty(  ) )
   {
      ch->print( "Slookup what?\r\n" "  slookup all\r\n  slookup null\r\n  slookup smaug\r\n" "  slookup herbs\r\n  slookup tongues\r\n" );
      return;
   }

   if( !str_cmp( argument, "all" ) )
   {
      for( sn = 0; sn < num_skills && skill_table[sn] && skill_table[sn]->name; ++sn )
         ch->pagerf( "Sn: %4d Slot: %4d Skill/spell: '%-20s' Damtype: %s\r\n",
                     sn, skill_table[sn]->slot, skill_table[sn]->name, spell_damage[SPELL_DAMAGE( skill_table[sn] )] );
   }
   else if( !str_cmp( argument, "null" ) )
   {
      int num = 0;

      for( sn = 0; sn < num_skills && skill_table[sn] && skill_table[sn]->name; ++sn )
         if( ( skill_table[sn]->skill_fun == skill_notfound || skill_table[sn]->skill_fun == NULL )
             && ( skill_table[sn]->spell_fun == spell_notfound || skill_table[sn]->spell_fun == spell_null ) && skill_table[sn]->type != SKILL_TONGUE )
         {
            ch->pagerf( "Sn: %3d Slot: %3d Name: '%-24s' Damtype: %s\r\n", sn, skill_table[sn]->slot, skill_table[sn]->name, spell_damage[SPELL_DAMAGE( skill_table[sn] )] );
            ++num;
         }
      ch->pagerf( "%d matches found.\r\n", num );
   }
   else if( !str_cmp( argument, "tongues" ) )
   {
      int num = 0;

      for( sn = 0; sn < num_skills && skill_table[sn] && skill_table[sn]->name; ++sn )
         if( skill_table[sn]->type == SKILL_TONGUE )
         {
            ch->pagerf( "Sn: %3d Slot: %3d Name: '%-24s'\r\n", sn, skill_table[sn]->slot, skill_table[sn]->name );
            ++num;
         }
      ch->pagerf( "%d matches found.\r\n", num );
   }
   else if( !str_cmp( argument, "smaug" ) )
   {
      int num = 0;

      for( sn = 0; sn < num_skills && skill_table[sn] && skill_table[sn]->name; ++sn )
         if( skill_table[sn]->spell_fun == spell_smaug )
         {
            ch->pagerf( "Sn: %3d Slot: %3d Name: '%-24s' Damtype: %s\r\n", sn, skill_table[sn]->slot, skill_table[sn]->name, spell_damage[SPELL_DAMAGE( skill_table[sn] )] );
            ++num;
         }
      ch->pagerf( "%d matches found.\r\n", num );
   }
   else if( !str_cmp( argument, "herbs" ) )
   {
      for( sn = 0; sn < top_herb && herb_table[sn] && herb_table[sn]->name; ++sn )
         ch->pagerf( "%d) %s\r\n", sn, herb_table[sn]->name );
   }
   else
   {
      if( argument[0] == 'h' && is_number( argument.substr( 1, argument.length(  ) ) ) )
      {
         sn = atoi( argument.substr( 1, argument.length(  ) ).c_str(  ) );
         if( !IS_VALID_HERB( sn ) )
         {
            ch->print( "Invalid herb.\r\n" );
            return;
         }
         skill = herb_table[sn];
      }
      else if( is_number( argument ) )
      {
         sn = atoi( argument.c_str(  ) );
         if( !( skill = get_skilltype( sn ) ) )
         {
            ch->print( "Invalid sn.\r\n" );
            return;
         }
         sn %= 1000;
      }
      else if( ( sn = skill_lookup( argument ) ) >= 0 )
         skill = skill_table[sn];
      else if( ( sn = herb_lookup( argument ) ) >= 0 )
         skill = herb_table[sn];
      else
      {
         ch->print( "No such skill, spell, proficiency or tongue.\r\n" );
         return;
      }
      if( !skill )
      {
         ch->print( "Not created yet.\r\n" );
         return;
      }

      ch->printf( "Sn: %4d Slot: %4d %s: '%-20s'\r\n", sn, skill->slot, skill_tname[skill->type], skill->name );
      if( skill->author )
         ch->printf( "Author: %s\r\n", skill->author );
      if( skill->info )
         ch->printf( "DamType: %s  ActType: %s   ClassType: %s   PowerType: %s\r\n",
                     spell_damage[SPELL_DAMAGE( skill )], spell_action[SPELL_ACTION( skill )], spell_class[SPELL_CLASS( skill )], spell_power[SPELL_POWER( skill )] );
      if( skill->flags.any(  ) )
         ch->printf( "Flags: %s\r\n", bitset_string( skill->flags, spell_flag ) );
      ch->printf( "Saves: %s  SaveEffect: %s\r\n", spell_saves[( int )skill->saves], spell_save_effect[SPELL_SAVE( skill )] );

      if( skill->difficulty != '\0' )
         ch->printf( "Difficulty: %d\r\n", ( int )skill->difficulty );

      ch->printf( "Type: %s  Target: %s  Minpos: %s  Mana: %d  Beats: %d  Range: %d\r\n",
                  skill_tname[skill->type], target_type[URANGE( TAR_IGNORE, skill->target, TAR_OBJ_INV )],
                  npc_position[skill->minimum_position], skill->min_mana, skill->beats, skill->range );

      ch->printf( "Guild: %d  Value: %d  Info: %d\r\n", skill->guild, skill->value, skill->info );

      ch->printf( "Ego: %d  Code: %s\r\n", skill->ego, skill->skill_fun ? skill->skill_fun_name : skill->spell_fun_name );

      ch->printf( "Dammsg: %s\r\nWearoff: %s\n", skill->noun_damage, skill->msg_off ? skill->msg_off : "(none set)" );

      if( skill->dice && skill->dice[0] != '\0' )
         ch->printf( "Dice: %s\r\n", skill->dice );

      if( skill->teachers && skill->teachers[0] != '\0' )
         ch->printf( "Teachers: %s\r\n", skill->teachers );

      if( skill->components && skill->components[0] != '\0' )
         ch->printf( "Components: %s\r\n", skill->components );

      if( skill->participants )
         ch->printf( "Participants: %d\r\n", ( int )skill->participants );

      int cnt = 0;
      list < smaug_affect * >::iterator aff;
      for( aff = skill->affects.begin(  ); aff != skill->affects.end(  ); )
      {
         smaug_affect *af = ( *aff );

         if( aff == skill->affects.begin(  ) )
            ch->print( "\r\n" );

         snprintf( buf, MSL, "Affect %d", ++cnt );
         if( af->location )
         {
            mudstrlcat( buf, " modifies ", MSL );
            mudstrlcat( buf, a_types[af->location % REVERSE_APPLY], MSL );
            mudstrlcat( buf, " by '", MSL );
            mudstrlcat( buf, af->modifier, MSL );
            if( af->bit != -1 )
               mudstrlcat( buf, "' and", MSL );
            else
               mudstrlcat( buf, "'", MSL );
         }
         if( af->bit != -1 )
         {
            mudstrlcat( buf, " applies ", MSL );
            mudstrlcat( buf, aff_flags[af->bit], MSL );
         }
         if( af->duration[0] != '\0' && af->duration[0] != '0' )
         {
            mudstrlcat( buf, " for '", MSL );
            mudstrlcat( buf, af->duration, MSL );
            mudstrlcat( buf, "' rounds", MSL );
         }
         if( af->location >= REVERSE_APPLY )
            mudstrlcat( buf, " (affects caster only)", MSL );
         mudstrlcat( buf, "\r\n", MSL );
         ch->print( buf );
         if( ++aff == skill->affects.end(  ) )
            ch->print( "\r\n" );
      }
      if( skill->hit_char && skill->hit_char[0] != '\0' )
         ch->printf( "Hitchar   : %s\r\n", skill->hit_char );

      if( skill->hit_vict && skill->hit_vict[0] != '\0' )
         ch->printf( "Hitvict   : %s\r\n", skill->hit_vict );

      if( skill->hit_room && skill->hit_room[0] != '\0' )
         ch->printf( "Hitroom   : %s\r\n", skill->hit_room );

      if( skill->hit_dest && skill->hit_dest[0] != '\0' )
         ch->printf( "Hitdest   : %s\r\n", skill->hit_dest );

      if( skill->miss_char && skill->miss_char[0] != '\0' )
         ch->printf( "Misschar  : %s\r\n", skill->miss_char );

      if( skill->miss_vict && skill->miss_vict[0] != '\0' )
         ch->printf( "Missvict  : %s\r\n", skill->miss_vict );

      if( skill->miss_room && skill->miss_room[0] != '\0' )
         ch->printf( "Missroom  : %s\r\n", skill->miss_room );

      if( skill->die_char && skill->die_char[0] != '\0' )
         ch->printf( "Diechar   : %s\r\n", skill->die_char );

      if( skill->die_vict && skill->die_vict[0] != '\0' )
         ch->printf( "Dievict   : %s\r\n", skill->die_vict );

      if( skill->die_room && skill->die_room[0] != '\0' )
         ch->printf( "Dieroom   : %s\r\n", skill->die_room );

      if( skill->imm_char && skill->imm_char[0] != '\0' )
         ch->printf( "Immchar   : %s\r\n", skill->imm_char );

      if( skill->imm_vict && skill->imm_vict[0] != '\0' )
         ch->printf( "Immvict   : %s\r\n", skill->imm_vict );

      if( skill->imm_room && skill->imm_room[0] != '\0' )
         ch->printf( "Immroom   : %s\r\n", skill->imm_room );

      if( skill->type != SKILL_HERB )
      {
         if( skill->type != SKILL_RACIAL )
         {
            ch->print( "--------------------------[CLASS USE]--------------------------\r\n" );
            for( iClass = 0; iClass < MAX_PC_CLASS; ++iClass )
            {
               mudstrlcpy( buf, class_table[iClass]->who_name, MSL );
               snprintf( buf + 3, MSL - 3, " ) lvl: %3d max: %2d%%", skill->skill_level[iClass], skill->skill_adept[iClass] );
               if( iClass % 3 == 2 )
                  mudstrlcat( buf, "\r\n", MSL );
               else
                  mudstrlcat( buf, "  ", MSL );
               ch->print( buf );
            }
         }
         else
         {
            ch->print( "\r\n--------------------------[RACE USE]--------------------------\r\n" );
            for( iRace = 0; iRace < MAX_PC_RACE; ++iRace )
            {
               snprintf( buf, MSL, "%8.8s ) lvl: %3d max: %2d%%", race_table[iRace]->race_name, skill->race_level[iRace], skill->race_adept[iRace] );
               if( !str_cmp( race_table[iRace]->race_name, "unused" ) )
                  mudstrlcpy( buf, "                           ", MSL );
               if( ( iRace > 0 ) && ( iRace % 2 == 1 ) )
                  mudstrlcat( buf, "\r\n", MSL );
               else
                  mudstrlcat( buf, "  ", MSL );
               ch->print( buf );
            }
         }
      }
      ch->print( "\r\n" );
   }
}

/*
 * Set a skill's attributes or what skills a player has.
 * High god command, with support for creating skills/spells/herbs/etc
 */
CMDF( do_sset )
{
   string arg1, arg2;
   char_data *victim;
   int value, sn, i;
   bool fAll;

   smash_tilde( argument );
   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );

   if( arg1.empty(  ) || arg2.empty(  ) || argument.empty(  ) )
   {
      ch->print( "Syntax: sset <victim> <skill> <value>\r\n" );
      ch->print( "or:     sset <victim> all     <value>\r\n" );
      if( ch->is_imp(  ) )
      {
         ch->print( "or:     sset save skill table\r\n" );
         ch->print( "or:     sset save herb table\r\n" );
         ch->print( "or:     sset create skill 'new skill'\r\n" );
         ch->print( "or:     sset create herb 'new herb'\r\n" );
         ch->print( "or:     sset create ability 'new ability'\r\n" );
         ch->print( "or:     sset create lore 'new lore'\r\n" );
      }
      if( ch->level > LEVEL_GREATER )
      {
         ch->print( "or:     sset <sn>     <field> <value>\r\n" );
         ch->print( "\r\nField being one of:\r\n" );
         ch->print( "  name code target minpos slot mana beats dammsg wearoff guild minlevel\r\n" );
         ch->print( "  type damtype acttype classtype powertype seffect flags dice value difficulty\r\n" );
         ch->print( "  affect rmaffect level adept hit miss die imm (char/vict/room)\r\n" );
         ch->print( "  components teachers racelevel raceadept ego\r\n" );
         ch->print( "Affect having the fields: <location> <modfifier> [duration] [bitvector]\r\n" );
         ch->print( "(See AFFECTTYPES for location, and AFFECTED_BY for bitvector)\r\n" );
      }
      ch->print( "Skill being any skill or spell.\r\n" );
      return;
   }

   if( ch->is_imp(  ) && !str_cmp( arg1, "save" ) && !str_cmp( argument, "table" ) )
   {
      if( !str_cmp( arg2, "skill" ) )
      {
         ch->print( "Saving skill table...\r\n" );
         save_skill_table(  );
         save_classes(  );
         save_races(  );
         return;
      }
      if( !str_cmp( arg2, "herb" ) )
      {
         ch->print( "Saving herb table...\r\n" );
         save_herb_table(  );
         return;
      }
   }
   if( ch->is_imp(  ) && !str_cmp( arg1, "create" ) && ( !str_cmp( arg2, "skill" ) || !str_cmp( arg2, "herb" ) || !str_cmp( arg2, "ability" ) ) )
   {
      struct skill_type *skill;
      short type = SKILL_UNKNOWN;

      if( !str_cmp( arg2, "herb" ) )
      {
         type = SKILL_HERB;
         if( top_herb >= MAX_HERB )
         {
            ch->printf( "The current top herb is %d, which is the maximum. "
                        "To add more herbs,\r\nMAX_HERB will have to be raised in mudcfg.h, and the mud recompiled.\r\n", top_herb );
            return;
         }
      }
      else if( num_skills >= MAX_SKILL )
      {
         ch->printf( "The current top sn is %d, which is the maximum. "
                     "To add more skills,\r\nMAX_SKILL will have to be raised in mudcfg.h, and the mud recompiled.\r\n", num_skills );
         return;
      }
      skill = new skill_type;
      skill->slot = 0;
      if( type == SKILL_HERB )
      {
         int max, x;

         herb_table[top_herb++] = skill;
         for( max = x = 0; x < top_herb - 1; ++x )
            if( herb_table[x] && herb_table[x]->slot > max )
               max = herb_table[x]->slot;
         skill->slot = max + 1;
      }
      else
         skill_table[num_skills++] = skill;
      skill->min_mana = 0;
      skill->name = str_dup( argument.c_str(  ) );
      skill->spell_fun = spell_smaug;
      skill->guild = -1;
      skill->type = type;
      skill->author = QUICKLINK( ch->name );
      if( !str_cmp( arg2, "ability" ) )
         skill->type = SKILL_RACIAL;
      if( !str_cmp( arg2, "lore" ) )
         skill->type = SKILL_LORE;

      for( i = 0; i < MAX_PC_CLASS; ++i )
      {
         skill->skill_level[i] = LEVEL_IMMORTAL;
         skill->skill_adept[i] = 95;
      }
      for( i = 0; i < MAX_PC_RACE; ++i )
      {
         skill->race_level[i] = LEVEL_IMMORTAL;
         skill->race_adept[i] = 95;
      }

      ch->print( "Done.\r\n" );
      return;
   }

   if( arg1[0] == 'h' )
      sn = atoi( arg1.substr( 1, arg1.length(  ) ).c_str(  ) );
   else
      sn = atoi( arg1.c_str(  ) );
   if( ch->get_trust(  ) > LEVEL_GREATER && sn >= 0 )
   {
      skill_type *skill;

      if( arg1[0] == 'h' )
      {
         if( sn >= top_herb )
         {
            ch->print( "Herb number out of range.\r\n" );
            return;
         }
         skill = herb_table[sn];
      }
      else
      {
         if( !( skill = get_skilltype( sn ) ) )
         {
            ch->print( "Skill number out of range.\r\n" );
            return;
         }
         sn %= 1000;
      }

      if( !str_cmp( arg2, "difficulty" ) )
      {
         skill->difficulty = atoi( argument.c_str(  ) );
         ch->print( "Ok.\r\n" );
         return;
      }
      if( !str_cmp( arg2, "ego" ) )
      {
         skill->ego = atoi( argument.c_str(  ) );
         ch->print( "Ok.\r\n" );
         return;
      }
      if( !str_cmp( arg2, "participants" ) )
      {
         skill->participants = atoi( argument.c_str(  ) );
         ch->print( "Ok.\r\n" );
         return;
      }
      if( !str_cmp( arg2, "damtype" ) )
      {
         int x = get_sdamage( argument );

         if( x == -1 )
            ch->print( "Not a spell damage type.\r\n" );
         else
         {
            SET_SDAM( skill, x );
            ch->print( "Ok.\r\n" );
         }
         return;
      }
      if( !str_cmp( arg2, "acttype" ) )
      {
         int x = get_saction( argument );

         if( x == -1 )
            ch->print( "Not a spell action type.\r\n" );
         else
         {
            SET_SACT( skill, x );
            ch->print( "Ok.\r\n" );
         }
         return;
      }
      if( !str_cmp( arg2, "classtype" ) )
      {
         int x = get_sclass( argument );

         if( x == -1 )
            ch->print( "Not a spell Class type.\r\n" );
         else
         {
            SET_SCLA( skill, x );
            ch->print( "Ok.\r\n" );
         }
         return;
      }
      if( !str_cmp( arg2, "powertype" ) )
      {
         int x = get_spower( argument );

         if( x == -1 )
            ch->print( "Not a spell power type.\r\n" );
         else
         {
            SET_SPOW( skill, x );
            ch->print( "Ok.\r\n" );
         }
         return;
      }
      if( !str_cmp( arg2, "seffect" ) )
      {
         int x = get_ssave_effect( argument );

         if( x == -1 )
            ch->print( "Not a spell save effect type.\r\n" );
         else
         {
            SET_SSAV( skill, x );
            ch->print( "Ok.\r\n" );
         }
         return;
      }
      if( !str_cmp( arg2, "flags" ) )
      {
         string arg3;
         int x;

         while( !argument.empty(  ) )
         {
            argument = one_argument( argument, arg3 );
            x = get_sflag( arg3 );
            if( x < 0 || x >= MAX_SF_FLAG )
               ch->printf( "Unknown flag: %s\r\n", arg3.c_str(  ) );
            else
               skill->flags.flip( x );
         }
         ch->print( "Ok.\r\n" );
         return;
      }
      if( !str_cmp( arg2, "saves" ) )
      {
         int x = get_ssave( argument );

         if( x == -1 )
            ch->print( "Not a saving type.\r\n" );
         else
         {
            skill->saves = x;
            ch->print( "Ok.\r\n" );
         }
         return;
      }

      if( !str_cmp( arg2, "code" ) )
      {
         DO_FUN *skillfun = NULL;
         SPELL_FUN *spellfun = NULL;

         if( validate_spec_fun( argument ) )
         {
            ch->print( "Cannot use a spec_fun for skills or spells.\r\n" );
            return;
         }
         else if( !str_prefix( "do_", argument ) && ( skillfun = skill_function( argument ) ) != skill_notfound )
         {
            skill->skill_fun = skillfun;
            skill->spell_fun = NULL;
            DISPOSE( skill->skill_fun_name );
            skill->skill_fun_name = str_dup( argument.c_str(  ) );
         }
         else if( ( spellfun = spell_function( argument ) ) != spell_notfound )
         {
            skill->spell_fun = spellfun;
            skill->skill_fun = NULL;
            DISPOSE( skill->skill_fun_name );
            skill->spell_fun_name = str_dup( argument.c_str(  ) );
         }
         else
         {
            ch->print( "Not a spell or skill.\r\n" );
            return;
         }
         ch->print( "Ok.\r\n" );
         return;
      }

      if( !str_cmp( arg2, "target" ) )
      {
         int x = get_starget( argument );

         if( x == -1 )
            ch->print( "Not a valid target type.\r\n" );
         else
         {
            skill->target = x;
            ch->print( "Ok.\r\n" );
         }
         return;
      }
      if( !str_cmp( arg2, "minpos" ) )
      {
         int pos;

         pos = get_npc_position( argument );

         if( pos < 0 || pos > POS_MAX )
         {
            ch->print( "Invalid position.\r\n" );
            return;
         }

         skill->minimum_position = pos;
         ch->print( "Skill minposition set.\r\n" );
         return;
      }
      if( !str_cmp( arg2, "minlevel" ) )
      {
         skill->min_level = URANGE( 1, atoi( argument.c_str(  ) ), MAX_LEVEL );
         ch->print( "Ok.\r\n" );
         return;
      }
      if( !str_cmp( arg2, "slot" ) )
      {
         skill->slot = URANGE( 0, atoi( argument.c_str(  ) ), 30000 );
         ch->print( "Ok.\r\n" );
         return;
      }
      if( !str_cmp( arg2, "mana" ) )
      {
         skill->min_mana = URANGE( 0, atoi( argument.c_str(  ) ), 2000 );
         ch->print( "Ok.\r\n" );
         return;
      }
      if( !str_cmp( arg2, "beats" ) )
      {
         skill->beats = URANGE( 0, atoi( argument.c_str(  ) ), 120 );
         ch->print( "Ok.\r\n" );
         return;
      }
      if( !str_cmp( arg2, "range" ) )
      {
         skill->range = URANGE( 0, atoi( argument.c_str(  ) ), 20 );
         ch->print( "Ok.\r\n" );
         return;
      }
      if( !str_cmp( arg2, "guild" ) )
      {
         skill->guild = atoi( argument.c_str(  ) );
         ch->print( "Ok.\r\n" );
         return;
      }
      if( !str_cmp( arg2, "value" ) )
      {
         skill->value = atoi( argument.c_str(  ) );
         ch->print( "Ok.\r\n" );
         return;
      }
      if( !str_cmp( arg2, "type" ) )
      {
         skill->type = get_skill( argument );
         ch->print( "Ok.\r\n" );
         return;
      }
      if( !str_cmp( arg2, "rmaffect" ) )
      {
         list < smaug_affect * >::iterator aff;
         int num = atoi( argument.c_str(  ) );
         int cnt = 0;

         if( skill->affects.empty(  ) )
         {
            ch->print( "This spell has no special affects to remove.\r\n" );
            return;
         }
         for( aff = skill->affects.begin(  ); aff != skill->affects.end(  ); ++aff )
         {
            smaug_affect *af = *aff;

            if( ++cnt == num )
            {
               skill->affects.remove( af );
               deleteptr( af );
               ch->print( "Removed.\r\n" );
               return;
            }
         }
         ch->print( "Not found.\r\n" );
         return;
      }
      /*
       * affect <location> <modifier> <duration> <bitvector>
       */
      if( !str_cmp( arg2, "affect" ) )
      {
         string location, modifier, duration;
         int loc, bit, tmpbit;
         smaug_affect *aff;

         argument = one_argument( argument, location );
         argument = one_argument( argument, modifier );
         argument = one_argument( argument, duration );

         if( location[0] == '!' )
            loc = get_atype( location.substr( 1, location.length(  ) ) ) + REVERSE_APPLY;
         else
            loc = get_atype( location );
         if( ( loc % REVERSE_APPLY ) < 0 || ( loc % REVERSE_APPLY ) >= MAX_APPLY_TYPE )
         {
            ch->print( "Unknown affect location.  See AFFECTTYPES.\r\n" );
            return;
         }
         bit = -1;
         if( !argument.empty(  ) )
         {
            if( ( tmpbit = get_aflag( argument ) ) == -1 )
               ch->printf( "Unknown bitvector: %s.  See AFFECTED_BY\r\n", argument.c_str(  ) );
            else
               bit = tmpbit;
         }
         aff = new smaug_affect;
         if( !str_cmp( duration, "0" ) )
            duration.clear(  );
         if( !str_cmp( modifier, "0" ) )
            modifier.clear(  );
         aff->duration = str_dup( duration.c_str(  ) );
         aff->location = loc;
         if( loc == APPLY_AFFECT || loc == APPLY_EXT_AFFECT )
         {
            int modval = get_aflag( modifier );

            /*
             * Sanitize the flag input for the modifier if needed -- Samson 
             */
            if( modval < 0 )
               modval = 0;
            /*
             * Spells/skills affect people, yes? People can only have EXT_BV affects, yes? 
             */
            if( loc == APPLY_AFFECT )
               aff->location = APPLY_EXT_AFFECT;
            modifier = aff_flags[modval];
         }
         if( loc == APPLY_RESISTANT || loc == APPLY_IMMUNE || loc == APPLY_ABSORB || loc == APPLY_SUSCEPTIBLE )
         {
            int modval = get_risflag( modifier );

            /*
             * Sanitize the flag input for the modifier if needed -- Samson 
             */
            if( modval < 0 )
               modval = 0;
            modifier = ris_flags[modval];
         }
         aff->modifier = str_dup( modifier.c_str(  ) );
         aff->bit = bit;
         skill->affects.push_back( aff );
         ch->print( "Ok.\r\n" );
         return;
      }

      if( !str_cmp( arg2, "level" ) )
      {
         string arg3;
         int Class;

         argument = one_argument( argument, arg3 );
         Class = get_pc_class( arg3 );

         if( Class >= MAX_PC_CLASS || Class < 0 )
            ch->printf( "%s is not a valid Class.\r\n", arg3.c_str(  ) );
         else
            skill->skill_level[Class] = URANGE( 0, atoi( argument.c_str(  ) ), MAX_LEVEL );
         return;
      }

      if( !str_cmp( arg2, "racelevel" ) )
      {
         string arg3;
         int race;

         argument = one_argument( argument, arg3 );
         race = get_pc_race( arg3 );

         if( race >= MAX_PC_RACE || race < 0 )
            ch->printf( "%s is not a valid race.\r\n", arg3.c_str(  ) );
         else
            skill->race_level[race] = URANGE( 0, atoi( argument.c_str(  ) ), MAX_LEVEL );
         return;
      }

      if( !str_cmp( arg2, "adept" ) )
      {
         string arg3;
         int Class;

         argument = one_argument( argument, arg3 );
         Class = get_pc_class( arg3 );

         if( Class >= MAX_PC_CLASS || Class < 0 )
            ch->printf( "%s is not a valid Class.\r\n", arg3.c_str(  ) );
         else
            skill->skill_adept[Class] = URANGE( 0, atoi( argument.c_str(  ) ), 100 );
         return;
      }

      if( !str_cmp( arg2, "raceadept" ) )
      {
         string arg3;
         int race;

         argument = one_argument( argument, arg3 );
         race = get_pc_race( arg3 );

         if( race >= MAX_PC_RACE || race < 0 )
            ch->printf( "%s is not a valid race.\r\n", arg3.c_str(  ) );
         else
            skill->race_adept[race] = URANGE( 0, atoi( argument.c_str(  ) ), 100 );
         return;
      }

      if( !str_cmp( arg2, "name" ) )
      {
         DISPOSE( skill->name );
         skill->name = str_dup( argument.c_str(  ) );
         ch->print( "Ok.\r\n" );
         return;
      }
      if( !str_cmp( arg2, "author" ) )
      {
         if( !ch->is_imp(  ) )
         {
            ch->print( "Sorry, only admins can change this.\r\n" );
            return;
         }
         STRFREE( skill->author );
         skill->author = STRALLOC( argument.c_str(  ) );
         ch->print( "Ok.\r\n" );
         return;
      }
      if( !str_cmp( arg2, "dammsg" ) )
      {
         DISPOSE( skill->noun_damage );
         if( str_cmp( argument, "clear" ) )
            skill->noun_damage = str_dup( argument.c_str(  ) );
         ch->print( "Ok.\r\n" );
         return;
      }
      if( !str_cmp( arg2, "wearoff" ) )
      {
         DISPOSE( skill->msg_off );
         if( str_cmp( argument, "clear" ) )
            skill->msg_off = str_dup( argument.c_str(  ) );
         ch->print( "Ok.\r\n" );
         return;
      }
      if( !str_cmp( arg2, "hitchar" ) )
      {
         DISPOSE( skill->hit_char );
         if( str_cmp( argument, "clear" ) )
            skill->hit_char = str_dup( argument.c_str(  ) );
         ch->print( "Ok.\r\n" );
         return;
      }
      if( !str_cmp( arg2, "hitvict" ) )
      {
         DISPOSE( skill->hit_vict );
         if( str_cmp( argument, "clear" ) )
            skill->hit_vict = str_dup( argument.c_str(  ) );
         ch->print( "Ok.\r\n" );
         return;
      }
      if( !str_cmp( arg2, "hitroom" ) )
      {
         DISPOSE( skill->hit_room );
         if( str_cmp( argument, "clear" ) )
            skill->hit_room = str_dup( argument.c_str(  ) );
         ch->print( "Ok.\r\n" );
         return;
      }
      if( !str_cmp( arg2, "hitdest" ) )
      {
         DISPOSE( skill->hit_dest );
         if( str_cmp( argument, "clear" ) )
            skill->hit_dest = str_dup( argument.c_str(  ) );
         ch->print( "Ok.\r\n" );
         return;
      }
      if( !str_cmp( arg2, "misschar" ) )
      {
         DISPOSE( skill->miss_char );
         if( str_cmp( argument, "clear" ) )
            skill->miss_char = str_dup( argument.c_str(  ) );
         ch->print( "Ok.\r\n" );
         return;
      }
      if( !str_cmp( arg2, "missvict" ) )
      {
         DISPOSE( skill->miss_vict );
         if( str_cmp( argument, "clear" ) )
            skill->miss_vict = str_dup( argument.c_str(  ) );
         ch->print( "Ok.\r\n" );
         return;
      }
      if( !str_cmp( arg2, "missroom" ) )
      {
         DISPOSE( skill->miss_room );
         if( str_cmp( argument, "clear" ) )
            skill->miss_room = str_dup( argument.c_str(  ) );
         ch->print( "Ok.\r\n" );
         return;
      }
      if( !str_cmp( arg2, "diechar" ) )
      {
         DISPOSE( skill->die_char );
         if( str_cmp( argument, "clear" ) )
            skill->die_char = str_dup( argument.c_str(  ) );
         ch->print( "Ok.\r\n" );
         return;
      }
      if( !str_cmp( arg2, "dievict" ) )
      {
         DISPOSE( skill->die_vict );
         if( str_cmp( argument, "clear" ) )
            skill->die_vict = str_dup( argument.c_str(  ) );
         ch->print( "Ok.\r\n" );
         return;
      }
      if( !str_cmp( arg2, "dieroom" ) )
      {
         DISPOSE( skill->die_room );
         if( str_cmp( argument, "clear" ) )
            skill->die_room = str_dup( argument.c_str(  ) );
         ch->print( "Ok.\r\n" );
         return;
      }
      if( !str_cmp( arg2, "immchar" ) )
      {
         DISPOSE( skill->imm_char );
         if( str_cmp( argument, "clear" ) )
            skill->imm_char = str_dup( argument.c_str(  ) );
         ch->print( "Ok.\r\n" );
         return;
      }
      if( !str_cmp( arg2, "immvict" ) )
      {
         DISPOSE( skill->imm_vict );
         if( str_cmp( argument, "clear" ) )
            skill->imm_vict = str_dup( argument.c_str(  ) );
         ch->print( "Ok.\r\n" );
         return;
      }
      if( !str_cmp( arg2, "immroom" ) )
      {
         DISPOSE( skill->imm_room );
         if( str_cmp( argument, "clear" ) )
            skill->imm_room = str_dup( argument.c_str(  ) );
         ch->print( "Ok.\r\n" );
         return;
      }
      if( !str_cmp( arg2, "dice" ) )
      {
         DISPOSE( skill->dice );
         if( str_cmp( argument, "clear" ) )
            skill->dice = str_dup( argument.c_str(  ) );
         ch->print( "Ok.\r\n" );
         return;
      }
      if( !str_cmp( arg2, "components" ) )
      {
         DISPOSE( skill->components );
         if( str_cmp( argument, "clear" ) )
            skill->components = str_dup( argument.c_str(  ) );
         ch->print( "Ok.\r\n" );
         return;
      }
      if( !str_cmp( arg2, "teachers" ) )
      {
         DISPOSE( skill->teachers );
         if( str_cmp( argument, "clear" ) )
            skill->teachers = str_dup( argument.c_str(  ) );
         ch->print( "Ok.\r\n" );
         return;
      }
      do_sset( ch, "" );
      return;
   }

   if( !( victim = ch->get_char_world( arg1 ) ) )
   {
      if( ( sn = skill_lookup( arg1 ) ) >= 0 )
         funcf( ch, do_sset, "%d %s %s", sn, arg2.c_str(  ), argument.c_str(  ) );
      else
         ch->print( "They aren't here.\r\n" );
      return;
   }

   if( victim->isnpc(  ) )
   {
      ch->print( "Not on NPC's.\r\n" );
      return;
   }

   fAll = !str_cmp( arg2, "all" );
   sn = 0;
   if( !fAll && ( sn = skill_lookup( arg2 ) ) < 0 )
   {
      ch->print( "No such skill or spell.\r\n" );
      return;
   }

   /*
    * Snarf the value.
    */
   if( !is_number( argument ) )
   {
      ch->print( "Value must be numeric.\r\n" );
      return;
   }

   value = atoi( argument.c_str(  ) );
   if( value < 0 || value > 100 )
   {
      ch->print( "Value range is 0 to 100.\r\n" );
      return;
   }

   if( fAll )
   {
      for( sn = 0; sn < num_skills; ++sn )
      {
         /*
          * Fix by Narn to prevent ssetting skills the player shouldn't have. 
          */
         if( victim->level >= skill_table[sn]->skill_level[victim->Class] || victim->level >= skill_table[sn]->race_level[victim->race] )
         {
            // Bugfix by Sadiq - Modified slightly by Samson. No need to call GET_ADEPT more than once each time this loop runs.
            int adept = victim->GET_ADEPT( sn );

            if( value > adept && !victim->is_immortal() )
               victim->pcdata->learned[sn] = adept;
            else
               victim->pcdata->learned[sn] = value;
         }
      }
   }
   else
      victim->pcdata->learned[sn] = value;
}

CMDF( do_grapple )
{
   char_data *victim;
   affect_data af;
   string arg;
   short percent;

   if( !ch->IS_PKILL( ) && !ch->is_immortal() )
      return;

   if( ch->isnpc() && ch->has_aflag( AFF_CHARM ) )
   {
      ch->print( "You can't do that right now.\r\n" );
      return;
   }

   if( ch->mount )
   {
      ch->print( "You can't get close enough while mounted.\r\n" );
      return;
   }

   if( ( victim = ch->who_fighting( ) ) == NULL )
   {
      one_argument( argument, arg );
      if( arg[0] == '\0' )
      {
         act( AT_ACTION, "You move in a circle looking for someone to grapple.", ch, NULL, NULL, TO_CHAR );
         act( AT_ACTION, "$n moves in a circle looking for someone to grapple.", ch, NULL, NULL, TO_ROOM );
         return;
      }

      if( ( victim = ch->get_char_room( arg ) ) == NULL )
      {
         ch->print( "They aren't here.\r\n" );
         return;
      }

      if( victim == ch )
      {
         ch->print( "How can you sneak up on yourself?\r\n" );
         return;
      }
   }

   if( victim->isnpc() )
   {
      ch->print( "Stick to wrestling players.\r\n" );
      return;
   }

   if( victim->has_aflag( AFF_GRAPPLE ) )
      return;

   if( is_safe( ch, victim ) )
   {
      ch->print( "A magical force prevents you from attacking.\r\n" );
      return;
   }

   if( ch->who_fighting( ) && ch->who_fighting( ) != victim )
   {
      ch->print( "You're fighting someone else!\r\n" );
      return;
   }

   if( victim->who_fighting( ) && victim->who_fighting( ) != ch )
   {
      ch->print( "You can't get close enough.\r\n" );
      return;
   }

   percent = ch->LEARNED( gsn_grapple );
   ch->WAIT_STATE( skill_table[gsn_grapple]->beats );

   if( !ch->chance( percent ) )
   {
      ch->print( "You lost your balance.\r\n" );
      act( AT_ACTION, "$n tries to grapple you but can't get close enough.", ch, NULL, victim, TO_VICT );
      ch->learn_from_failure( gsn_grapple );
      check_illegal_pk( ch, victim );
      return;
   }

   af.type = gsn_grapple;
   af.duration = 2;
   af.location = APPLY_DEX;
   af.modifier = -2;
   af.bit = AFF_GRAPPLE;
   victim->affect_to_char( &af );

   af.type = gsn_grapple;
   af.duration = 2;
   af.location = APPLY_DEX;
   af.modifier = -2;
   af.bit = AFF_GRAPPLE;
   ch->affect_to_char( &af );

   ch->printf( "You manage to grab hold of %s!\r\n", capitalize( victim->name ) );
   act( AT_ACTION, "$n grabs hold of you!", ch, NULL, victim, TO_VICT );
   act( AT_ACTION, "$n begins grappling with $N!", ch, NULL, victim, TO_NOTVICT );
   check_illegal_pk( ch, victim );

   if( !ch->fighting && victim->in_room == ch->in_room )
      set_fighting( ch, victim );
   if( !victim->fighting && ch->in_room == victim->in_room )
      set_fighting( victim, ch );
   return;
}

CMDF( do_gouge )
{
   char_data *victim;
   affect_data af;
   short dam;
   int schance;

   if( ch->isnpc(  ) && ch->has_aflag( AFF_CHARM ) )
   {
      ch->print( "You can't concentrate enough for that.\r\n" );
      return;
   }

   if( !can_use_skill( ch, 0, gsn_gouge ) )
   {
      ch->print( "You do not yet know of this skill.\r\n" );
      return;
   }

   if( ch->mount )
   {
      ch->print( "You can't get close enough while mounted.\r\n" );
      return;
   }

   if( !( victim = ch->who_fighting(  ) ) )
   {
      ch->print( "You aren't fighting anyone.\r\n" );
      return;
   }

   schance = ( ( victim->get_curr_dex(  ) - ch->get_curr_dex(  ) ) * 10 ) + 10;
   if( !ch->isnpc(  ) && !victim->isnpc(  ) )
      schance += sysdata->gouge_plr_vs_plr;
   if( victim->fighting && victim->fighting->who != ch )
      schance += sysdata->gouge_nontank;
   if( can_use_skill( ch, ( number_percent(  ) + schance ), gsn_gouge ) )
   {
      dam = number_range( 5, ch->level );
      global_retcode = damage( ch, victim, dam, gsn_gouge );
      if( global_retcode == rNONE )
      {
         if( !victim->has_aflag( AFF_BLIND ) )
         {
            af.type = gsn_blindness;
            af.location = APPLY_HITROLL;
            af.modifier = -6;
            if( !victim->isnpc(  ) && !ch->isnpc(  ) )
               af.duration = ( ch->level + 10 ) / victim->get_curr_con(  );
            else
               af.duration = 3 + ( ch->level / 15 );
            af.bit = AFF_BLIND;
            victim->affect_to_char( &af );
            act( AT_SKILL, "You can't see a thing!", victim, NULL, NULL, TO_CHAR );
         }
         ch->WAIT_STATE( sysdata->pulseviolence );
         if( !ch->isnpc(  ) && !victim->isnpc(  ) )
         {
            if( number_bits( 1 ) == 0 )
            {
               ch->printf( "%s looks momentarily dazed.\r\n", victim->name );
               victim->print( "You are momentarily dazed ...\r\n" );
               victim->WAIT_STATE( sysdata->pulseviolence );
            }
         }
         else
            victim->WAIT_STATE( sysdata->pulseviolence );
      }
      else if( global_retcode == rVICT_DIED )
         act( AT_BLOOD, "Your fingers plunge into your victim's brain, causing immediate death!", ch, NULL, NULL, TO_CHAR );
   }
   else
   {
      ch->WAIT_STATE( skill_table[gsn_gouge]->beats );
      global_retcode = damage( ch, victim, 0, gsn_gouge );
      ch->learn_from_failure( gsn_gouge );
   }
}

CMDF( do_detrap )
{
   string arg;
   obj_data *trap, *obj = NULL;
   list < obj_data * >::iterator iobj;
   int percent;
   bool found = false;

   switch ( ch->substate )
   {
      default:
         if( ch->isnpc(  ) && ch->has_aflag( AFF_CHARM ) )
         {
            ch->print( "You can't concentrate enough for that.\r\n" );
            return;
         }
         argument = one_argument( argument, arg );
         if( !can_use_skill( ch, 0, gsn_detrap ) )
         {
            ch->print( "You do not yet know of this skill.\r\n" );
            return;
         }
         if( arg.empty(  ) )
         {
            ch->print( "Detrap what?\r\n" );
            return;
         }
         if( ms_find_obj( ch ) )
            return;
         found = false;
         if( ch->mount )
         {
            ch->print( "You can't do that while mounted.\r\n" );
            return;
         }
         if( ch->in_room->objects.empty(  ) )
         {
            ch->print( "You can't find that here.\r\n" );
            return;
         }
         for( iobj = ch->in_room->objects.begin(  ); iobj != ch->in_room->objects.end(  ); ++iobj )
         {
            obj = *iobj;

            if( ch->can_see_obj( obj, false ) && hasname( obj->name, arg ) )
            {
               found = true;
               break;
            }
         }
         if( !found )
         {
            ch->print( "You can't find that here.\r\n" );
            return;
         }
         act( AT_ACTION, "You carefully begin your attempt to remove a trap from $p...", ch, obj, NULL, TO_CHAR );
         act( AT_ACTION, "$n carefully attempts to remove a trap from $p...", ch, obj, NULL, TO_ROOM );
         ch->alloc_ptr = str_dup( obj->name );
         ch->add_timer( TIMER_DO_FUN, 3, do_detrap, 1 );
/*	    ch->WAIT_STATE( skill_table[gsn_detrap]->beats ); */
         return;

      case 1:
         if( !ch->alloc_ptr )
         {
            ch->print( "Your detrapping was interrupted!\r\n" );
            bug( "%s: ch->alloc_ptr NULL!", __func__ );
            return;
         }
         arg = ch->alloc_ptr;
         DISPOSE( ch->alloc_ptr );
         ch->substate = SUB_NONE;
         break;

      case SUB_TIMER_DO_ABORT:
         DISPOSE( ch->alloc_ptr );
         ch->substate = SUB_NONE;
         ch->print( "You carefully stop what you were doing.\r\n" );
         return;
   }

   if( ch->in_room->objects.empty(  ) )
   {
      ch->print( "You can't find that here.\r\n" );
      return;
   }
   for( iobj = ch->in_room->objects.begin(  ); iobj != ch->in_room->objects.end(  ); ++iobj )
   {
      obj = *iobj;

      if( ch->can_see_obj( obj, false ) && hasname( obj->name, arg ) )
      {
         found = true;
         break;
      }
   }
   if( !found )
   {
      ch->print( "You can't find that here.\r\n" );
      return;
   }
   if( !( trap = obj->get_trap(  ) ) )
   {
      ch->print( "You find no trap on that.\r\n" );
      return;
   }

   percent = number_percent(  ) - ( ch->level / 15 ) - ( ch->get_curr_lck(  ) - 16 );

   obj->separate(  );
   if( !can_use_skill( ch, percent, gsn_detrap ) )
   {
      ch->print( "Ooops!\r\n" );
      spring_trap( ch, trap );
      ch->learn_from_failure( gsn_detrap );
      return;
   }
   trap->extract(  );
   ch->print( "You successfully remove a trap.\r\n" );
}

CMDF( do_dig )
{
   string arg;
   exit_data *pexit;

   switch ( ch->substate )
   {
      default:
         if( ch->isnpc(  ) && ch->has_aflag( AFF_CHARM ) )
         {
            ch->print( "You can't concentrate enough for that.\r\n" );
            return;
         }
         if( ch->mount )
         {
            ch->print( "You can't do that while mounted.\r\n" );
            return;
         }
         one_argument( argument, arg );
         if( !arg.empty(  ) )
         {
            if( ( pexit = find_door( ch, arg, true ) ) == NULL && get_dir( arg ) == -1 )
            {
               ch->print( "What direction is that?\r\n" );
               return;
            }
            if( pexit )
            {
               if( !IS_EXIT_FLAG( pexit, EX_DIG ) && !IS_EXIT_FLAG( pexit, EX_CLOSED ) )
               {
                  ch->print( "There is no need to dig out that exit.\r\n" );
                  return;
               }
            }
         }
         else
         {
            int sector;

            if( ch->has_pcflag( PCFLAG_ONMAP ) || ch->has_actflag( ACT_ONMAP ) )
               sector = map_sector[ch->wmap][ch->mx][ch->my];
            else
               sector = ch->in_room->sector_type;

            switch ( sector )
            {
               default:
                  break;
               case SECT_CITY:
               case SECT_ROAD:
                  ch->print( "The road is too hard to dig through.\r\n" );
                  return;
               case SECT_BRIDGE:
                  ch->print( "You cannot dig on a bridge.\r\n" );
                  return;
               case SECT_INDOORS:
                  ch->print( "The floor is too hard to dig through.\r\n" );
                  return;
               case SECT_WATER_SWIM:
               case SECT_WATER_NOSWIM:
               case SECT_UNDERWATER:
                  ch->print( "You cannot dig here.\r\n" );
                  return;
               case SECT_AIR:
                  ch->print( "What?  In the air?!\r\n" );
                  return;
               case SECT_LAVA:
                  ch->print( "Are you trying to barbecue yourself?\r\n" );
                  return;
            }
         }
         ch->add_timer( TIMER_DO_FUN, UMIN( skill_table[gsn_dig]->beats / 10, 3 ), do_dig, 1 );
         ch->alloc_ptr = str_dup( arg.c_str(  ) );
         ch->print( "You begin digging...\r\n" );
         act( AT_PLAIN, "$n begins digging...", ch, NULL, NULL, TO_ROOM );
         return;

      case 1:
         if( !ch->alloc_ptr )
         {
            ch->print( "Your digging was interrupted!\r\n" );
            act( AT_PLAIN, "$n's digging was interrupted!", ch, NULL, NULL, TO_ROOM );
            bug( "%s: alloc_ptr NULL", __func__ );
            return;
         }
         arg = ch->alloc_ptr;
         DISPOSE( ch->alloc_ptr );
         break;

      case SUB_TIMER_DO_ABORT:
         DISPOSE( ch->alloc_ptr );
         ch->substate = SUB_NONE;
         ch->print( "You stop digging...\r\n" );
         act( AT_PLAIN, "$n stops digging...", ch, NULL, NULL, TO_ROOM );
         return;
   }

   ch->substate = SUB_NONE;

   /*
    * not having a shovel makes it harder to succeed 
    */
   bool shovel = false;
   list < obj_data * >::iterator iobj;
   for( iobj = ch->carrying.begin(  ); iobj != ch->carrying.end(  ); ++iobj )
   {
      obj_data *obj = *iobj;
      if( obj->item_type == ITEM_SHOVEL )
      {
         shovel = true;
         break;
      }
   }

   /*
    * dig out an EX_DIG exit... 
    */
   if( !arg.empty(  ) )
   {
      if( ( pexit = find_door( ch, arg, true ) ) != NULL && IS_EXIT_FLAG( pexit, EX_DIG ) && IS_EXIT_FLAG( pexit, EX_CLOSED ) )
      {
         /*
          * 4 times harder to dig open a passage without a shovel 
          */
         if( can_use_skill( ch, ( number_percent(  ) * ( shovel ? 1 : 4 ) ), gsn_dig ) )
         {
            if( !IS_EXIT_FLAG( pexit, EX_ISDOOR ) )
            {
               remove_bexit_flag( pexit, EX_CLOSED );
               ch->print( "You dig open a passageway!\r\n" );
               act( AT_PLAIN, "$n digs open a passageway!", ch, NULL, NULL, TO_ROOM );
            }
            else
            {
               REMOVE_EXIT_FLAG( pexit, EX_DIG );
               ch->print( "You uncover a doorway!\r\n" );
               act( AT_PLAIN, "$n uncovers a doorway!", ch, NULL, NULL, TO_ROOM );
            }
         }
      }
      ch->learn_from_failure( gsn_dig );
      ch->print( "Your dig did not discover any exit...\r\n" );
      act( AT_PLAIN, "$n's dig did not discover any exit...", ch, NULL, NULL, TO_ROOM );
      return;
   }

   bool found = false;
   obj_data *obj = NULL;
   for( iobj = ch->in_room->objects.begin(  ); iobj != ch->in_room->objects.end(  ); ++iobj )
   {
      obj = *iobj;
      /*
       * twice as hard to find something without a shovel 
       */
      if( obj->extra_flags.test( ITEM_BURIED ) && ( can_use_skill( ch, ( number_percent(  ) * ( shovel ? 1 : 2 ) ), gsn_dig ) ) )
      {
         found = true;
         break;
      }
   }

   if( !found )
   {
      ch->print( "Your dig uncovered nothing.\r\n" );
      act( AT_PLAIN, "$n's dig uncovered nothing.", ch, NULL, NULL, TO_ROOM );
      ch->learn_from_failure( gsn_dig );
      return;
   }

   obj->separate(  );
   obj->extra_flags.reset( ITEM_BURIED );
   act( AT_SKILL, "Your dig uncovered $p!", ch, obj, NULL, TO_CHAR );
   act( AT_SKILL, "$n's dig uncovered $p!", ch, obj, NULL, TO_ROOM );
   if( obj->item_type == ITEM_CORPSE_PC || obj->item_type == ITEM_CORPSE_NPC )
      ch->adjust_favor( 14, 1 );
}

CMDF( do_search )
{
   string arg;
   obj_data *container;

   int door = -1;
   switch ( ch->substate )
   {
      default:
         if( ch->isnpc(  ) && ch->has_aflag( AFF_CHARM ) )
         {
            ch->print( "You can't concentrate enough for that.\r\n" );
            return;
         }
         if( ch->mount )
         {
            ch->print( "You can't do that while mounted.\r\n" );
            return;
         }
         argument = one_argument( argument, arg );
         if( !arg.empty(  ) && ( door = get_door( arg ) ) == -1 )
         {
            if( !( container = ch->get_obj_here( arg ) ) )
            {
               ch->print( "You can't find that here.\r\n" );
               return;
            }
            if( container->item_type != ITEM_CONTAINER )
            {
               ch->print( "You can't search in that!\r\n" );
               return;
            }
            if( IS_SET( container->value[1], CONT_CLOSED ) )
            {
               ch->print( "It is closed.\r\n" );
               return;
            }
         }
         ch->add_timer( TIMER_DO_FUN, UMIN( skill_table[gsn_search]->beats / 10, 3 ), do_search, 1 );
         ch->print( "You begin your search...\r\n" );
         act( AT_MAGIC, "$n begins searching the room.....", ch, NULL, NULL, TO_ROOM );
         ch->alloc_ptr = str_dup( arg.c_str(  ) );
         return;

      case 1:
         if( !ch->alloc_ptr )
         {
            ch->print( "Your search was interrupted!\r\n" );
            bug( "%s: alloc_ptr NULL", __func__ );
            return;
         }
         arg = ch->alloc_ptr;
         DISPOSE( ch->alloc_ptr );
         break;

      case SUB_TIMER_DO_ABORT:
         DISPOSE( ch->alloc_ptr );
         ch->substate = SUB_NONE;
         ch->print( "You stop your search...\r\n" );
         return;
   }

   bool found = false;
   ch->substate = SUB_NONE;
   list < obj_data * >startlist;
   if( arg.empty(  ) )
   {
      startlist = ch->in_room->objects;
      found = true;
   }
   else
   {
      if( ( door = get_door( arg ) ) != -1 )
         found = true;
      else
      {
         if( !( container = ch->get_obj_here( arg ) ) )
         {
            ch->print( "You can't find that here.\r\n" );
            return;
         }
         startlist = container->contents;
         found = true;
      }
   }

   if( ( !found && door == -1 ) || ch->isnpc(  ) )
   {
      ch->print( "You find nothing.\r\n" );
      act( AT_MAGIC, "$n found nothing in $s search.", ch, NULL, NULL, TO_ROOM );
      ch->learn_from_failure( gsn_search );
      return;
   }

   int percent = number_percent(  ) + number_percent(  ) - ( ch->level / 10 );

   if( door != -1 )
   {
      exit_data *pexit;

      if( ( pexit = ch->in_room->get_exit( door ) ) != NULL && IS_EXIT_FLAG( pexit, EX_SECRET )
          && IS_EXIT_FLAG( pexit, EX_xSEARCHABLE ) && can_use_skill( ch, percent, gsn_search ) )
      {
         act( AT_SKILL, "Your search reveals the $d!", ch, NULL, pexit->keyword, TO_CHAR );
         act( AT_SKILL, "$n finds the $d!", ch, NULL, pexit->keyword, TO_ROOM );
         REMOVE_EXIT_FLAG( pexit, EX_SECRET );
         return;
      }
   }
   else
   {
      list < obj_data * >::iterator iobj;
      for( iobj = startlist.begin(  ); iobj != startlist.end(  ); ++iobj )
      {
         obj_data *obj = *iobj;
         if( obj->extra_flags.test( ITEM_HIDDEN ) && can_use_skill( ch, percent, gsn_search ) )
         {
            obj->separate(  );
            obj->extra_flags.reset( ITEM_HIDDEN );
            act( AT_SKILL, "Your search reveals $p!", ch, obj, NULL, TO_CHAR );
            act( AT_SKILL, "$n finds $p!", ch, obj, NULL, TO_ROOM );
            return;
         }
      }
   }
   ch->print( "You find nothing.\r\n" );
   act( AT_MAGIC, "$n found nothing in $s search.", ch, NULL, NULL, TO_ROOM );
   ch->learn_from_failure( gsn_search );
}

CMDF( do_steal )
{
   string arg1, arg2;
   char_data *victim, *mst;
   obj_data *obj;
   int percent;

   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );

   if( ch->mount )
   {
      ch->print( "You can't do that while mounted.\r\n" );
      return;
   }

   if( arg1.empty(  ) || arg2.empty(  ) )
   {
      ch->print( "Steal what from whom?\r\n" );
      return;
   }

   if( ms_find_obj( ch ) )
      return;

   if( !( victim = ch->get_char_room( arg2 ) ) )
   {
      ch->print( "They aren't here.\r\n" );
      return;
   }

   if( victim == ch )
   {
      ch->print( "That's pointless.\r\n" );
      return;
   }

   if( ch->in_room->flags.test( ROOM_SAFE ) )
   {
      ch->print( "&[magic]A magical force interrupts you.\r\n" );
      return;
   }

   if( !victim->isnpc(  ) )
   {
      if( !ch->CAN_PKILL(  ) || !victim->CAN_PKILL(  ) )
      {
         ch->print( "&[immortal]The gods forbid stealing from non-PK players.\r\n" );
         return;
      }
   }

   if( victim->isnpc() && victim->has_actflag( ACT_PACIFIST ) ) /* Gorog */
   {
      ch->print( "They are a pacifist - Shame on you!\r\n" );
      return;
   }

   ch->WAIT_STATE( skill_table[gsn_steal]->beats );
   percent = number_percent(  ) + ( victim->IS_AWAKE(  )? 10 : -50 ) - ( ch->get_curr_lck(  ) - 15 ) + ( victim->get_curr_lck(  ) - 13 );

   /*
    * Changed the level check, made it 10 levels instead of five and made the 
    * victim not attack in the case of a too high level difference.  This is 
    * to allow mobprogs where the mob steals eq without having to put level 
    * checks into the progs.  Also gave the mobs a 10% chance of failure.
    */
   if( ch->level + 10 < victim->level )
   {
      ch->print( "You really don't want to try that!\r\n" );
      return;
   }

   if( victim->position == POS_FIGHTING || !can_use_skill( ch, percent, gsn_steal ) )
   {
      /*
       * Failure.
       */
      ch->print( "Oops...\r\n" );
      act( AT_ACTION, "$n tried to steal from you!\r\n", ch, NULL, victim, TO_VICT );
      act( AT_ACTION, "$n tried to steal from $N.\r\n", ch, NULL, victim, TO_NOTVICT );

      cmdf( victim, "yell %s is a bloody thief!", ch->name );

      ch->learn_from_failure( gsn_steal );
      if( !ch->isnpc(  ) )
      {
         if( legal_loot( ch, victim ) )
            global_retcode = multi_hit( victim, ch, TYPE_UNDEFINED );
         else
         {
            if( ch->isnpc(  ) )
            {
               if( !( mst = ch->master ) )
                  return;
            }
            else
               mst = ch;
            if( mst->isnpc(  ) )
               return;
         }
      }
      return;
   }

   if( !str_cmp( arg1, "coin" ) || !str_cmp( arg1, "coins" ) || !str_cmp( arg1, "gold" ) )
   {
      int amount;

      amount = ( int )( victim->gold * number_range( 1, 10 ) / 100 );
      if( amount <= 0 )
      {
         ch->print( "You couldn't get any gold.\r\n" );
         ch->learn_from_failure( gsn_steal );
         return;
      }
      ch->gold += amount;
      victim->gold -= amount;
      ch->printf( "Aha!  You got %d gold coins.\r\n", amount );
      return;
   }

   if( !( obj = victim->get_obj_carry( arg1 ) ) )
   {
      ch->print( "You can't seem to find it.\r\n" );
      ch->learn_from_failure( gsn_steal );
      return;
   }

   if( obj->extra_flags.test( ITEM_LOYAL ) )
   {
      ch->print( "The item refuses to part with its owner!\r\n" );
      return;
   }

   if( !ch->can_drop_obj( obj ) || obj->extra_flags.test( ITEM_INVENTORY ) || obj->extra_flags.test( ITEM_PROTOTYPE ) || obj->ego > ch->char_ego(  ) )
   {
      ch->print( "You can't manage to pry it away.\r\n" );
      ch->learn_from_failure( gsn_steal );
      return;
   }

   if( ch->carry_number + ( obj->get_number(  ) / obj->count ) > ch->can_carry_n(  ) )
   {
      ch->print( "You have your hands full.\r\n" );
      ch->learn_from_failure( gsn_steal );
      return;
   }

   if( ch->carry_weight + ( obj->get_weight(  ) / obj->count ) > ch->can_carry_w(  ) )
   {
      ch->print( "You can't carry that much weight.\r\n" );
      ch->learn_from_failure( gsn_steal );
      return;
   }

   obj->separate(  );
   obj->from_char(  );
   obj->to_char( ch );
   ch->print( "Ok.\r\n" );
   ch->adjust_favor( 9, 1 );
}

/* Heavily modified with Shard stuff, including the critical pierce - Samson 5-22-99 */
CMDF( do_backstab )
{
   string arg;
   char_data *victim;
   obj_data *obj;
   affect_data af;
   int percent, base;
   int awake_saw = 50, to_saw = 0, sleep_paralysis = 70, awake_paralysis = 90;
   bool no_para = false;

   if( ch->isnpc(  ) && ch->has_aflag( AFF_CHARM ) )
   {
      ch->print( "You can't do that right now.\r\n" );
      return;
   }

   /*
    * Someone is gonna die if I come back later and find this removed again!!! 
    */
   if( ch->Class != CLASS_ROGUE && !ch->is_immortal(  ) )
   {
      ch->print( "What do you think you are? A Rogue???\r\n" );
      return;
   }

   one_argument( argument, arg );

   if( ch->mount )
   {
      ch->print( "You can't get close enough while mounted.\r\n" );
      return;
   }

   if( arg.empty(  ) )
   {
      ch->print( "Backstab whom?\r\n" );
      return;
   }

   if( !( victim = ch->get_char_room( arg ) ) )
   {
      ch->print( "They aren't here.\r\n" );
      return;
   }

   if( victim == ch )
   {
      ch->print( "How can you sneak up on yourself?\r\n" );
      return;
   }

   if( is_safe( ch, victim ) )
      return;

   if( victim->hit < victim->max_hit )
   {
      ch->printf( "%s is hurt and suspicious, you'll never get close enough.\r\n", victim->short_descr );
      return;
   }

   /*
    * Added stabbing weapon. -Narn 
    */
   if( !( obj = ch->get_eq( WEAR_WIELD ) ) )
   {
      ch->print( "You need to wield a piercing or stabbing weapon.\r\n" );
      return;
   }

   if( obj->value[4] != WEP_DAGGER )
   {
      if( ( obj->value[4] == WEP_SWORD && obj->value[3] != DAM_PIERCE ) || obj->value[4] != WEP_SWORD )
      {
         ch->print( "You need to wield a piercing or stabbing weapon.\r\n" );
         return;
      }
   }

   if( victim->fighting )
      base = 0;
   else
      base = 4;

/* ==4/3/95== Gives the backstab a chance to paralysis it's victim.
 * This chance is increased and is mainly dependent upon
 * the fact that the thief is sneaking.  There is also a
 * chance that the mob will not even notice thief has missed
 * the backstab.  A sleeping mob has better chance of being
 * paralysised and has no chance for missing backstab.
 * Chance to paralysis and not be seen is modified by thieves
 * dexterity - Open [Former Shard coder]
 */

   af.type = gsn_paralyze;
   af.duration = -1;
   af.modifier = 0;
   af.location = APPLY_NONE;
   af.bit = AFF_PARALYSIS;

   if( ch->has_aflag( AFF_SNEAK ) )
      to_saw += number_range( 1, 100 );
   if( ch->get_curr_dex(  ) > victim->get_curr_dex(  ) )
      to_saw += ch->get_curr_dex(  );

   percent = number_percent(  ) - ( ch->get_curr_lck(  ) - 14 ) + ( victim->get_curr_lck(  ) - 13 );

   if( check_illegal_pk( ch, victim ) )
   {
      ch->print( "You cannot do that to another player!\r\n" );
      return;
   }

   ch->WAIT_STATE( skill_table[gsn_backstab]->beats );

   if( ( ch->isnpc(  ) || percent > ch->pcdata->learned[gsn_backstab] ) && victim->IS_AWAKE(  ) )
   {
      /*
       * Failed backstab 
       */
      if( awake_saw > to_saw )
      {
         /*
          * Victim saw attempt at backstab 
          */
         act( AT_SKILL, "$n nearly slices off $s finger trying to backstab $N!", ch, NULL, victim, TO_NOTVICT );
         act( AT_SKILL, "$n nearly slices off $s finger trying to backstab you!", ch, NULL, victim, TO_VICT );
         act( AT_SKILL, "You nearly slice off your finger trying to backstab $N!", ch, NULL, victim, TO_CHAR );
         global_retcode = damage( ch, victim, 0, gsn_backstab );
      }
      else
      {
         /*
          * victim did not see failed attempt 
          */
         act( AT_SKILL, "$N didn't seem to notice your futile attempt to backstab!", ch, NULL, victim, TO_CHAR );
         act( AT_SKILL, "$N didn't seem to notice $n's failed backstab attempt!", ch, NULL, victim, TO_NOTVICT );
      }
      ch->learn_from_failure( gsn_backstab );

   }
   else
   {  /* Successful Backstab */
      if( !victim->has_aflag( AFF_PARALYSIS ) )
      {
         if( victim->has_immune( RIS_HOLD ) )
            no_para = true;
         if( victim->has_resist( RIS_HOLD ) )
         {
            if( saves_spell_staff( ch->level, victim ) )
               no_para = true;
         }
      }

      if( victim->IS_AWAKE(  ) )
      {
         if( to_saw > awake_paralysis && !no_para )
         {  /* Para victim */
            act( AT_SKILL, "$N is frozen by a critical pierce to the spine!", ch, NULL, victim, TO_CHAR );
            act( AT_SKILL, "$n got you in the spine! You are paralyzed!!", ch, NULL, victim, TO_VICT );
            act( AT_SKILL, "$n paralyzed $N with a critical pierce to the spine!", ch, NULL, victim, TO_NOTVICT );
            victim->affect_to_char( &af );
         }

         act( AT_SKILL, "$n sneaks up behind $N, stabbing $M in the back!", ch, NULL, victim, TO_NOTVICT );
         act( AT_SKILL, "$n sneaks up behind you, stabbing you in the back!", ch, NULL, victim, TO_VICT );
         act( AT_SKILL, "You sneak up behind $N, stabbing $M in the back!", ch, NULL, victim, TO_CHAR );
         ch->hitroll += base;
         global_retcode = multi_hit( ch, victim, gsn_backstab );
         ch->hitroll -= base;
         ch->adjust_favor( 10, 1 );
      }
      else
      {
         if( to_saw > sleep_paralysis && !no_para )
         {  /* Para victim */
            act( AT_SKILL, "$N is frozen by a critical pierce to the spine!", ch, NULL, victim, TO_NOTVICT );
            act( AT_SKILL, "$n got you in the spine! You are paralyzed!!", ch, NULL, victim, TO_CHAR );
            victim->affect_to_char( &af );
         }

         act( AT_SKILL, "$n sneaks up behind $N, stabbing $M in the back!", ch, NULL, victim, TO_NOTVICT );
         act( AT_SKILL, "$n sneaks up behind you, stabbing you in the back!", ch, NULL, victim, TO_VICT );
         act( AT_SKILL, "You sneak up behind $N, stabbing $M in the back!", ch, NULL, victim, TO_CHAR );
         ch->hitroll += base;
         global_retcode = multi_hit( ch, victim, gsn_backstab );
         ch->hitroll -= base;
         ch->adjust_favor( 10, 1 );
      }
   }
}

CMDF( do_rescue )
{
   char_data *victim, *fch;
   int percent;

   if( ch->isnpc(  ) && ch->has_aflag( AFF_CHARM ) )
   {
      ch->print( "You can't concentrate enough for that.\r\n" );
      return;
   }

   if( ch->has_aflag( AFF_BERSERK ) )
   {
      ch->print( "You aren't thinking clearly...\r\n" );
      return;
   }

   if( argument.empty(  ) )
   {
      ch->print( "Rescue whom?\r\n" );
      return;
   }

   if( !( victim = ch->get_char_room( argument ) ) )
   {
      ch->print( "They aren't here.\r\n" );
      return;
   }

   if( victim == ch )
   {
      ch->print( "How about fleeing instead?\r\n" );
      return;
   }

   if( ch->mount )
   {
      ch->print( "You can't do that while mounted.\r\n" );
      return;
   }

   if( !ch->isnpc(  ) && victim->isnpc(  ) )
   {
      ch->print( "They don't need your help!\r\n" );
      return;
   }

   if( !ch->fighting )
   {
      ch->print( "Too late...\r\n" );
      return;
   }

   if( !( fch = victim->who_fighting(  ) ) )
   {
      ch->print( "They are not fighting right now.\r\n" );
      return;
   }

   if( victim->who_fighting(  ) == ch )
   {
      ch->print( "Just running away would be better...\r\n" );
      return;
   }

   if( victim->has_aflag( AFF_BERSERK ) )
   {
      ch->print( "Stepping in front of a berserker would not be an intelligent decision.\r\n" );
      return;
   }

   percent = number_percent(  ) - ( ch->get_curr_lck(  ) - 14 ) - ( victim->get_curr_lck(  ) - 16 );

   ch->WAIT_STATE( skill_table[gsn_rescue]->beats );
   if( !can_use_skill( ch, percent, gsn_rescue ) )
   {
      ch->print( "You fail the rescue.\r\n" );
      act( AT_SKILL, "$n tries to rescue you!", ch, NULL, victim, TO_VICT );
      act( AT_SKILL, "$n tries to rescue $N!", ch, NULL, victim, TO_NOTVICT );
      ch->learn_from_failure( gsn_rescue );
      return;
   }

   act( AT_SKILL, "You rescue $N!", ch, NULL, victim, TO_CHAR );
   act( AT_SKILL, "$n rescues you!", ch, NULL, victim, TO_VICT );
   act( AT_SKILL, "$n moves in front of $N!", ch, NULL, victim, TO_NOTVICT );

   ch->adjust_favor( 8, 1 );
   fch->stop_fighting( false );
   victim->stop_fighting( false );
   if( ch->fighting )
      ch->stop_fighting( false );

   set_fighting( ch, fch );
   set_fighting( fch, ch );
}

void kick_messages( char_data * ch, char_data * victim, int dam, ch_ret rcode )
{
   int i;

   switch ( victim->race )
   {
      case RACE_HUMAN:
      case RACE_HIGH_ELF:
      case RACE_DWARF:
      case RACE_DROW:
      case RACE_ORC:
      case RACE_LYCANTH:
      case RACE_TROLL:
      case RACE_DEMON:
      case RACE_DEVIL:
      case RACE_MFLAYER:
      case RACE_ASTRAL:
      case RACE_PATRYN:
      case RACE_SARTAN:
      case RACE_DRAAGDIM:
      case RACE_GOLEM:
      case RACE_TROGMAN:
      case RACE_IGUANADON:
      case RACE_HALF_ELF:
      case RACE_HALF_OGRE:
      case RACE_HALF_ORC:
      case RACE_HALF_GIANT:
         i = number_range( 0, 3 );
         break;
      case RACE_PREDATOR:
      case RACE_HERBIV:
      case RACE_LABRAT:
         i = number_range( 4, 6 );
         break;
      case RACE_REPTILE:
      case RACE_DRAGON:
      case RACE_DRAGON_RED:
      case RACE_DRAGON_BLACK:
      case RACE_DRAGON_GREEN:
      case RACE_DRAGON_WHITE:
      case RACE_DRAGON_BLUE:
      case RACE_DRAGON_SILVER:
      case RACE_DRAGON_GOLD:
      case RACE_DRAGON_BRONZE:
      case RACE_DRAGON_COPPER:
      case RACE_DRAGON_BRASS:
         i = number_range( 4, 7 );
         break;
      case RACE_TREE:
         i = 8;
         break;
      case RACE_PARASITE:
      case RACE_SLIME:
      case RACE_VEGGIE:
      case RACE_VEGMAN:
         i = 9;
         break;
      case RACE_ROO:
      case RACE_GNOME:
      case RACE_HALFLING:
      case RACE_GOBLIN:
      case RACE_SMURF:
      case RACE_ENFAN:
         i = 10;
         break;
      case RACE_GIANT:
      case RACE_GIANT_HILL:
      case RACE_GIANT_FROST:
      case RACE_GIANT_FIRE:
      case RACE_GIANT_CLOUD:
      case RACE_GIANT_STORM:
      case RACE_GIANT_STONE:
      case RACE_TYTAN:
      case RACE_GOD:
         i = 11;
         break;
      case RACE_GHOST:
         i = 12;
         break;
      case RACE_BIRD:
      case RACE_SKEXIE:
         i = 13;
         break;
      case RACE_UNDEAD:
      case RACE_UNDEAD_VAMPIRE:
      case RACE_UNDEAD_LICH:
      case RACE_UNDEAD_WIGHT:
      case RACE_UNDEAD_GHAST:
      case RACE_UNDEAD_SPECTRE:
      case RACE_UNDEAD_ZOMBIE:
      case RACE_UNDEAD_SKELETON:
      case RACE_UNDEAD_GHOUL:
         i = 14;
         break;
      case RACE_DINOSAUR:
         i = 15;
         break;
      case RACE_INSECT:
      case RACE_ARACHNID:
         i = 16;
         break;
      case RACE_FISH:
         i = 17;
         break;
      default:
         i = 1;
   }

   if( !dam || rcode == rVICT_IMMUNE )
   {
      act( AT_GREY, att_kick_miss_ch[i], ch, NULL, victim, TO_CHAR );
      act( AT_GREY, att_kick_miss_vic[i], ch, NULL, victim, TO_VICT );
      act( AT_GREY, att_kick_miss_room[i], ch, NULL, victim, TO_NOTVICT );
   }
   else if( rcode == rVICT_DIED )
   {
      act( AT_GREY, att_kick_kill_ch[i], ch, NULL, victim, TO_CHAR );
      /*
       * act(AT_GREY, att_kick_kill_vic[i], ch, NULL, victim, TO_VICT); 
       */
      act( AT_GREY, att_kick_kill_room[i], ch, NULL, victim, TO_NOTVICT );
   }
   else
   {
      act( AT_GREY, att_kick_hit_ch[i], ch, NULL, victim, TO_CHAR );
      act( AT_GREY, att_kick_hit_vic[i], ch, NULL, victim, TO_VICT );
      act( AT_GREY, att_kick_hit_room[i], ch, NULL, victim, TO_NOTVICT );
   }
}

CMDF( do_kick )
{
   char_data *victim;

   if( ch->isnpc(  ) && ch->has_aflag( AFF_CHARM ) )
   {
      ch->print( "You can't concentrate enough for that.\r\n" );
      return;
   }

   if( !ch->isnpc(  ) && ch->level < skill_table[gsn_kick]->skill_level[ch->Class] && ch->level < skill_table[gsn_kick]->race_level[ch->race] )
   {
      ch->print( "You better leave the martial arts to fighters.\r\n" );
      return;
   }

   if( !( victim = ch->who_fighting(  ) ) )
      if( !( victim = ch->get_char_room( argument ) ) )
      {
         ch->print( "They aren't here.\r\n" );
         return;
      }

   if( !is_legal_kill( ch, victim ) || is_safe( ch, victim ) )
   {
      ch->print( "You can't do that!\r\n" );
      return;
   }

   ch->WAIT_STATE( skill_table[gsn_kick]->beats );
   if( ( ch->isnpc(  ) || number_percent(  ) < ch->pcdata->learned[gsn_kick] ) && victim->race != RACE_GHOST )
   {
      global_retcode = damage( ch, victim, ch->Class == CLASS_MONK ? ch->level : ( ch->level / 2 ), gsn_kick );
      kick_messages( ch, victim, 1, global_retcode );
   }
   else
   {
      ch->learn_from_failure( gsn_kick );
      global_retcode = damage( ch, victim, 0, gsn_kick );
      kick_messages( ch, victim, 0, global_retcode );
   }
}

CMDF( do_bite )
{
   char_data *victim;

   if( ch->isnpc(  ) && ch->has_aflag( AFF_CHARM ) )
   {
      ch->print( "You can't concentrate enough for that.\r\n" );
      return;
   }

   if( !ch->isnpc(  ) && ch->level < skill_table[gsn_bite]->skill_level[ch->Class] )
   {
      ch->print( "That isn't quite one of your natural skills.\r\n" );
      return;
   }

   if( !( victim = ch->who_fighting(  ) ) )
   {
      ch->print( "You aren't fighting anyone.\r\n" );
      return;
   }

   ch->WAIT_STATE( skill_table[gsn_bite]->beats );
   if( can_use_skill( ch, number_percent(  ), gsn_bite ) )
      global_retcode = damage( ch, victim, number_range( 1, ch->level ), gsn_bite );
   else
   {
      ch->learn_from_failure( gsn_bite );
      global_retcode = damage( ch, victim, 0, gsn_bite );
   }
}

CMDF( do_claw )
{
   char_data *victim;

   if( !ch->isnpc(  ) && ch->level < skill_table[gsn_claw]->skill_level[ch->Class] )
   {
      ch->print( "That isn't quite one of your natural skills.\r\n" );
      return;
   }

   if( !( victim = ch->who_fighting(  ) ) )
   {
      ch->print( "You aren't fighting anyone.\r\n" );
      return;
   }

   ch->WAIT_STATE( skill_table[gsn_claw]->beats );
   if( can_use_skill( ch, number_percent(  ), gsn_claw ) )
      global_retcode = damage( ch, victim, number_range( 1, ch->level ), gsn_claw );
   else
   {
      ch->learn_from_failure( gsn_claw );
      global_retcode = damage( ch, victim, 0, gsn_claw );
   }
}

CMDF( do_sting )
{
   char_data *victim;

   if( ch->isnpc(  ) && ch->has_aflag( AFF_CHARM ) )
   {
      ch->print( "You can't concentrate enough for that.\r\n" );
      return;
   }

   if( !ch->isnpc(  ) && ch->level < skill_table[gsn_sting]->skill_level[ch->Class] )
   {
      ch->print( "That isn't quite one of your natural skills.\r\n" );
      return;
   }

   if( !( victim = ch->who_fighting(  ) ) )
   {
      ch->print( "You aren't fighting anyone.\r\n" );
      return;
   }

   ch->WAIT_STATE( skill_table[gsn_sting]->beats );
   if( can_use_skill( ch, number_percent(  ), gsn_sting ) )
      global_retcode = damage( ch, victim, number_range( 1, ch->level ), gsn_sting );
   else
   {
      ch->learn_from_failure( gsn_sting );
      global_retcode = damage( ch, victim, 0, gsn_sting );
   }
}

CMDF( do_tail )
{
   char_data *victim;

   if( ch->isnpc(  ) && ch->has_aflag( AFF_CHARM ) )
   {
      ch->print( "You can't concentrate enough for that.\r\n" );
      return;
   }

   if( !ch->isnpc(  ) && ch->level < skill_table[gsn_tail]->skill_level[ch->Class] )
   {
      ch->print( "That isn't quite one of your natural skills.\r\n" );
      return;
   }

   if( !( victim = ch->who_fighting(  ) ) )
   {
      ch->print( "You aren't fighting anyone.\r\n" );
      return;
   }

   ch->WAIT_STATE( skill_table[gsn_tail]->beats );
   if( can_use_skill( ch, number_percent(  ), gsn_tail ) )
      global_retcode = damage( ch, victim, number_range( 1, ch->level ), gsn_tail );
   else
   {
      ch->learn_from_failure( gsn_tail );
      global_retcode = damage( ch, victim, 0, gsn_tail );
   }
}

CMDF( do_bash )
{
   affect_data af;
   char_data *victim;
   int schance;

   if( ch->isnpc(  ) && ch->has_aflag( AFF_CHARM ) )
   {
      ch->print( "You can't concentrate enough for that.\r\n" );
      return;
   }

   if( !ch->isnpc(  ) && ch->level < skill_table[gsn_bash]->skill_level[ch->Class] )
   {
      ch->print( "You better leave the martial arts to fighters.\r\n" );
      return;
   }

   if( !( victim = ch->who_fighting(  ) ) )
      if( !( victim = ch->get_char_room( argument ) ) )
      {
         ch->print( "They aren't here.\r\n" );
         return;
      }

   if( victim->has_actflag( ACT_HUGE ) || victim->has_aflag( AFF_GROWTH ) )
      if( !IsGiant( ch ) && !ch->has_aflag( AFF_GROWTH ) )
      {
         ch->printf( "%s is MUCH too large to bash!\r\n", PERS( victim, ch, false ) );
         return;
      }

   schance = ( ( victim->get_curr_dex(  ) + victim->get_curr_str(  ) + victim->level ) - ( ch->get_curr_dex(  ) + ch->get_curr_str(  ) + ch->level ) );
   if( victim->fighting && victim->fighting->who != ch )
      schance += 25;
   ch->WAIT_STATE( skill_table[gsn_bash]->beats );
   if( ch->isnpc(  ) || ( number_percent(  ) + schance ) < ch->pcdata->learned[gsn_bash] )
   {
      ch->WAIT_STATE( 2 * sysdata->pulseviolence );
      victim->WAIT_STATE( 2 * sysdata->pulseviolence );
      victim->position = POS_SITTING;
      act( AT_SKILL, "$N smashes into you, and knocks you to the ground!", victim, NULL, ch, TO_CHAR );
      act( AT_SKILL, "You smash into $N, and knock $M to the ground!", ch, NULL, victim, TO_CHAR );
      act( AT_SKILL, "$n smashes into $N, and knocks $M to the ground!", ch, NULL, victim, TO_NOTVICT );
      global_retcode = damage( ch, victim, number_range( 1, 2 ), gsn_bash );
      victim->position = POS_SITTING;

      /*
       * A cheap hack to force the issue of not being able to cast spells 
       */
      af.type = gsn_bash;
      af.duration = 4;  /* 4 battle rounds, hopefully */
      af.modifier = 0;
      af.location = APPLY_NONE;
      af.bit = AFF_BASH;
      victim->affect_to_char( &af );
   }
   else
   {
      ch->WAIT_STATE( 2 * sysdata->pulseviolence );
      ch->learn_from_failure( gsn_bash );
      ch->position = POS_SITTING;
      act( AT_SKILL, "$N tries to smash into you, but falls to the ground!", victim, NULL, ch, TO_CHAR );
      act( AT_SKILL, "You smash into $N, and bounce off $M to the ground!", ch, NULL, victim, TO_CHAR );
      act( AT_SKILL, "$n smashes into $N, and bounces off $M to the ground!", ch, NULL, victim, TO_NOTVICT );
      global_retcode = damage( ch, victim, 0, gsn_bash );
   }
}

CMDF( do_stun )
{
   char_data *victim;
   affect_data af;
   int schance;
   bool fail;

   if( ch->isnpc(  ) && ch->has_aflag( AFF_CHARM ) )
   {
      ch->print( "You can't concentrate enough for that.\r\n" );
      return;
   }

   if( !ch->isnpc(  ) && ch->level < skill_table[gsn_stun]->skill_level[ch->Class] )
   {
      ch->print( "You better leave the martial arts to fighters.\r\n" );
      return;
   }

   if( !( victim = ch->who_fighting(  ) ) )
   {
      ch->print( "You aren't fighting anyone.\r\n" );
      return;
   }

   if( !ch->isnpc(  ) && ch->move < ch->max_move / 10 )
   {
      ch->print( "&[skill]You are far too tired to do that.\r\n" );
      return;  /* missing return fixed March 11/96 */
   }

   ch->WAIT_STATE( skill_table[gsn_stun]->beats );
   fail = false;
   schance = ris_save( victim, ch->level, RIS_PARALYSIS );
   if( schance == 1000 )
      fail = true;
   else
      fail = saves_para_petri( schance, victim );

   schance = ( ( ( victim->get_curr_dex(  ) + victim->get_curr_str(  ) ) - ( ch->get_curr_dex(  ) + ch->get_curr_str(  ) ) ) * 10 ) + 10;
   /*
    * harder for player to stun another player 
    */
   if( !ch->isnpc(  ) && !victim->isnpc(  ) )
      schance += sysdata->stun_plr_vs_plr;
   else
      schance += sysdata->stun_regular;
   if( !fail && can_use_skill( ch, ( number_percent(  ) + schance ), gsn_stun ) )
   {
      /*
       * DO *NOT* CHANGE!    -Thoric    
       */
      if( !ch->isnpc(  ) )
         ch->move -= ch->max_move / 10;
      ch->WAIT_STATE( 2 * sysdata->pulseviolence );
      victim->WAIT_STATE( sysdata->pulseviolence );
      act( AT_SKILL, "$N smashes into you, leaving you stunned!", victim, NULL, ch, TO_CHAR );
      act( AT_SKILL, "You smash into $N, leaving $M stunned!", ch, NULL, victim, TO_CHAR );
      act( AT_SKILL, "$n smashes into $N, leaving $M stunned!", ch, NULL, victim, TO_NOTVICT );
      if( !victim->has_aflag( AFF_PARALYSIS ) )
      {
         af.type = gsn_stun;
         af.location = APPLY_AC;
         af.modifier = 20;
         af.duration = 3;
         af.bit = AFF_PARALYSIS;
         victim->affect_to_char( &af );
         victim->update_pos(  );
      }
   }
   else
   {
      ch->WAIT_STATE( 2 * sysdata->pulseviolence );
      if( !ch->isnpc(  ) )
         ch->move -= ch->max_move / 15;
      ch->learn_from_failure( gsn_stun );
      act( AT_SKILL, "$n charges at you screaming, but you dodge out of the way.", ch, NULL, victim, TO_VICT );
      act( AT_SKILL, "You try to stun $N, but $E dodges out of the way.", ch, NULL, victim, TO_CHAR );
      act( AT_SKILL, "$n charges screaming at $N, but keeps going right on past.", ch, NULL, victim, TO_NOTVICT );
   }
}

bool check_grip( char_data * ch, char_data * victim )
{
   int schance;

   if( !victim->IS_AWAKE(  ) )
      return false;

   if( !victim->has_defense( DFND_GRIP ) )
      return false;

   if( victim->isnpc(  ) )
      schance = UMIN( 60, 2 * victim->level );
   else
      schance = ( int )( victim->LEARNED( gsn_grip ) / 2 );

   /*
    * Consider luck as a factor 
    */
   schance += ( 2 * ( victim->get_curr_lck(  ) - 13 ) );

   if( number_percent(  ) >= schance + victim->level - ch->level )
   {
      victim->learn_from_failure( gsn_grip );
      return false;
   }
   act( AT_SKILL, "You evade $n's attempt to disarm you.", ch, NULL, victim, TO_VICT );
   act( AT_SKILL, "$N holds $S weapon strongly, and is not disarmed.", ch, NULL, victim, TO_CHAR );
   return true;
}

/*
 * Disarm a creature.
 * Caller must check for successful attack.
 * Check for loyalty flag (weapon disarms to inventory) for pkillers -Blodkai
 */
void disarm( char_data * ch, char_data * victim )
{
   obj_data *obj, *tmpobj;

   if( !( obj = victim->get_eq( WEAR_WIELD ) ) )
      return;

   if( ( tmpobj = victim->get_eq( WEAR_DUAL_WIELD ) ) != NULL && number_bits( 1 ) == 0 )
      obj = tmpobj;

   if( !ch->get_eq( WEAR_WIELD ) && number_bits( 1 ) == 0 )
   {
      ch->learn_from_failure( gsn_disarm );
      return;
   }

   if( ch->isnpc(  ) && !ch->can_see_obj( obj, false ) && number_bits( 1 ) == 0 )
   {
      ch->learn_from_failure( gsn_disarm );
      return;
   }

   if( check_grip( ch, victim ) )
   {
      ch->learn_from_failure( gsn_disarm );
      return;
   }

   act( AT_SKILL, "$n DISARMS you!", ch, NULL, victim, TO_VICT );
   act( AT_SKILL, "You disarm $N!", ch, NULL, victim, TO_CHAR );
   act( AT_SKILL, "$n disarms $N!", ch, NULL, victim, TO_NOTVICT );

   if( obj == victim->get_eq( WEAR_WIELD ) && ( tmpobj = victim->get_eq( WEAR_DUAL_WIELD ) ) != NULL )
      tmpobj->wear_loc = WEAR_WIELD;

   obj->from_char(  );
   if( victim->isnpc(  ) || ( obj->extra_flags.test( ITEM_LOYAL ) && victim->IS_PKILL(  ) && !ch->isnpc(  ) ) )
      obj->to_char( victim );
   else
      obj->to_room( victim->in_room, victim );
}

CMDF( do_disarm )
{
   char_data *victim;
   obj_data *obj;
   int percent;

   if( ch->isnpc(  ) && ch->has_aflag( AFF_CHARM ) )
   {
      ch->print( "You can't concentrate enough for that.\r\n" );
      return;
   }

   if( !ch->isnpc(  ) && ch->level < skill_table[gsn_disarm]->skill_level[ch->Class] )
   {
      ch->print( "You don't know how to disarm opponents.\r\n" );
      return;
   }

   if( !ch->get_eq( WEAR_WIELD ) && ch->Class != CLASS_MONK )
   {
      ch->print( "You must wield a weapon to disarm.\r\n" );
      return;
   }

   if( !( victim = ch->who_fighting(  ) ) )
   {
      ch->print( "You aren't fighting anyone.\r\n" );
      return;
   }

   if( !( obj = victim->get_eq( WEAR_WIELD ) ) )
   {
      ch->print( "Your opponent is not wielding a weapon.\r\n" );
      return;
   }

   ch->WAIT_STATE( skill_table[gsn_disarm]->beats );
   percent = number_percent(  ) + victim->level - ch->level - ( ch->get_curr_lck(  ) - 15 ) + ( victim->get_curr_lck(  ) - 15 );
   if( !ch->can_see_obj( obj, false ) )
      percent += 10;
   if( can_use_skill( ch, ( percent * 3 / 2 ), gsn_disarm ) )
      disarm( ch, victim );
   else
   {
      ch->print( "You failed.\r\n" );
      ch->learn_from_failure( gsn_disarm );
   }
}

/*
 * Trip a creature.
 * Caller must check for successful attack.
 */
void trip( char_data * ch, char_data * victim )
{
   if( victim->has_aflag( AFF_FLYING ) || victim->has_aflag( AFF_FLOATING ) )
      return;

   if( victim->mount )
   {
      if( victim->mount->has_aflag( AFF_FLYING ) || victim->mount->has_aflag( AFF_FLOATING ) )
         return;
      act( AT_SKILL, "$n trips your mount and you fall off!", ch, NULL, victim, TO_VICT );
      act( AT_SKILL, "You trip $N's mount and $N falls off!", ch, NULL, victim, TO_CHAR );
      act( AT_SKILL, "$n trips $N's mount and $N falls off!", ch, NULL, victim, TO_NOTVICT );
      victim->mount->unset_actflag( ACT_MOUNTED );
      victim->mount = NULL;
      ch->WAIT_STATE( 2 * sysdata->pulseviolence );
      victim->WAIT_STATE( 2 * sysdata->pulseviolence );
      victim->position = POS_RESTING;
      return;
   }
   if( victim->wait == 0 )
   {
      act( AT_SKILL, "$n trips you and you go down!", ch, NULL, victim, TO_VICT );
      act( AT_SKILL, "You trip $N and $N goes down!", ch, NULL, victim, TO_CHAR );
      act( AT_SKILL, "$n trips $N and $N goes down!", ch, NULL, victim, TO_NOTVICT );

      ch->WAIT_STATE( 2 * sysdata->pulseviolence );
      victim->WAIT_STATE( 2 * sysdata->pulseviolence );
      victim->position = POS_RESTING;
   }
}

/* Shargate, May 2002 */
CMDF( do_cleave )
{
   char_data *victim;
   obj_data *obj;
   int dam = 0;
   short percent;

   if( ch->isnpc() && ch->has_aflag( AFF_CHARM ) )
   {
      ch->print( "A clear mind is required to use that skill.\r\n" );
      return;
   }

   if( !ch->isnpc() && ch->level < skill_table[gsn_cleave]->skill_level[ch->Class] )
   {
      ch->print( "You can't seem to summon the strength.\r\n" );
      return;
   }

   if( ( obj = ch->get_eq( WEAR_WIELD ) ) == NULL || ( obj->value[3] != 1 && obj->value[3] != 3 && obj->value[3] != 5 ) )
   {
      ch->print( "You need a slashing weapon.\r\n" );
      return;
   }

   if( ( victim = ch->who_fighting( ) ) == NULL )
   {
      ch->print( "You aren't fighting anyone.\r\n" );
      return;
   }

   percent = number_percent();
   if( can_use_skill( ch, percent, gsn_cleave ) )
   {
      ch->WAIT_STATE( skill_table[gsn_cleave]->beats );
      if( percent <= 20 )
      {
         ch->set_color( AT_WHITE );
         ch->print( "You deal a devastating blow!\r\n" );
         dam = ( number_range( 11, 22 ) * ch->get_curr_str( ) ) + 30;
         victim->WAIT_STATE( 2 );
      }
      else
      {
         dam = ( number_range( 9, 18 ) * ch->get_curr_str( ) ) + 30;
      }

      global_retcode = damage( ch, victim, dam, gsn_cleave );
   }
   else
   {
      ch->learn_from_failure( gsn_cleave );
      global_retcode = damage( ch, victim, 0, gsn_cleave );
   }
}

CMDF( do_pick )
{
   string arg;

   if( ch->isnpc(  ) && ch->has_aflag( AFF_CHARM ) )
   {
      ch->print( "You can't concentrate enough for that.\r\n" );
      return;
   }

   one_argument( argument, arg );

   if( arg.empty(  ) )
   {
      ch->print( "Pick what?\r\n" );
      return;
   }

   if( ms_find_obj( ch ) )
      return;

   if( ch->mount )
   {
      ch->print( "You can't do that while mounted.\r\n" );
      return;
   }

   ch->WAIT_STATE( skill_table[gsn_pick_lock]->beats );

   /*
    * look for guards 
    */
   list < char_data * >::iterator ich;
   for( ich = ch->in_room->people.begin(  ); ich != ch->in_room->people.end(  ); ++ich )
   {
      char_data *gch = *ich;

      if( gch->isnpc(  ) && gch->IS_AWAKE(  ) && ch->level + 5 < gch->level )
      {
         act( AT_PLAIN, "$N is standing too close to the lock.", ch, NULL, gch, TO_CHAR );
         return;
      }
   }

   if( !can_use_skill( ch, number_percent(  ), gsn_pick_lock ) )
   {
      ch->print( "You failed.\r\n" );
      ch->learn_from_failure( gsn_pick_lock );
      return;
   }

   exit_data *pexit;
   if( ( pexit = find_door( ch, arg, true ) ) != NULL )
   {
      /*
       * 'pick door' 
       */
      if( !IS_EXIT_FLAG( pexit, EX_CLOSED ) )
      {
         ch->print( "It's not closed.\r\n" );
         return;
      }
      if( pexit->key < 0 )
      {
         ch->print( "It can't be picked.\r\n" );
         return;
      }
      if( !IS_EXIT_FLAG( pexit, EX_LOCKED ) )
      {
         ch->print( "It's already unlocked.\r\n" );
         return;
      }
      if( IS_EXIT_FLAG( pexit, EX_PICKPROOF ) )
      {
         ch->print( "You failed.\r\n" );
         ch->learn_from_failure( gsn_pick_lock );
         check_room_for_traps( ch, TRAP_PICK | trap_door[pexit->vdir] );
         return;
      }

      REMOVE_EXIT_FLAG( pexit, EX_LOCKED );
      ch->print( "*Click*\r\n" );
      act( AT_ACTION, "$n picks the $d.", ch, NULL, pexit->keyword, TO_ROOM );
      ch->adjust_favor( 9, 1 );
      /*
       * pick the other side 
       */
      exit_data *pexit_rev;
      if( ( pexit_rev = pexit->rexit ) != NULL && pexit_rev->to_room == ch->in_room )
      {
         REMOVE_EXIT_FLAG( pexit_rev, EX_LOCKED );
      }
      check_room_for_traps( ch, TRAP_PICK | trap_door[pexit->vdir] );
      return;
   }

   obj_data *obj;
   if( ( obj = ch->get_obj_here( arg ) ) != NULL )
   {
      /*
       * 'pick object' 
       */
      if( obj->item_type != ITEM_CONTAINER )
      {
         ch->print( "That's not a container.\r\n" );
         return;
      }
      if( !IS_SET( obj->value[1], CONT_CLOSED ) )
      {
         ch->print( "It's not closed.\r\n" );
         return;
      }
      if( obj->value[2] < 0 )
      {
         ch->print( "It can't be unlocked.\r\n" );
         return;
      }
      if( !IS_SET( obj->value[1], CONT_LOCKED ) )
      {
         ch->print( "It's already unlocked.\r\n" );
         return;
      }
      if( IS_SET( obj->value[1], CONT_PICKPROOF ) )
      {
         ch->print( "You failed.\r\n" );
         ch->learn_from_failure( gsn_pick_lock );
         check_for_trap( ch, obj, TRAP_PICK );
         return;
      }

      obj->separate(  );
      REMOVE_BIT( obj->value[1], CONT_LOCKED );
      ch->print( "*Click*\r\n" );
      act( AT_ACTION, "$n picks $p.", ch, obj, NULL, TO_ROOM );
      ch->adjust_favor( 9, 1 );
      check_for_trap( ch, obj, TRAP_PICK );
      return;
   }
   ch->printf( "You see no %s here.\r\n", arg.c_str(  ) );
}

/*
 * Contributed by Alander.
 */
CMDF( do_visible )
{
   ch->affect_strip( gsn_invis );
   ch->affect_strip( gsn_mass_invis );
   ch->affect_strip( gsn_sneak );
   ch->unset_aflag( AFF_HIDE );
   ch->unset_aflag( AFF_INVISIBLE );
   ch->unset_aflag( AFF_SNEAK );

   /*
    * Remove immortal wizinvis - Samson 11-28-98 
    */
   if( ch->has_pcflag( PCFLAG_WIZINVIS ) )
   {
      ch->unset_pcflag( PCFLAG_WIZINVIS );
      act( AT_IMMORT, "$n slowly fades into existence.", ch, NULL, NULL, TO_ROOM );
      ch->print( "&[immortal]You slowly fade back into existence.\r\n" );
   }
   ch->print( "Ok.\r\n" );
}

CMDF( do_aid )
{
   char_data *victim;
   int percent;

   if( ch->isnpc(  ) && ch->has_aflag( AFF_CHARM ) )
   {
      ch->print( "You can't concentrate enough for that.\r\n" );
      return;
   }

   if( argument.empty(  ) )
   {
      ch->print( "Aid whom?\r\n" );
      return;
   }

   if( !( victim = ch->get_char_room( argument ) ) )
   {
      ch->print( "They aren't here.\r\n" );
      return;
   }

   if( ch->mount )
   {
      ch->print( "You can't do that while mounted.\r\n" );
      return;
   }

   if( victim == ch )
   {
      ch->print( "Aid yourself?\r\n" );
      return;
   }

   if( victim->position > POS_STUNNED )
   {
      act( AT_PLAIN, "$N doesn't need your help.", ch, NULL, victim, TO_CHAR );
      return;
   }

   if( victim->hit <= -6 )
   {
      act( AT_PLAIN, "$N's condition is beyond your aiding ability.", ch, NULL, victim, TO_CHAR );
      return;
   }

   percent = number_percent(  ) - ( ch->get_curr_lck(  ) - 13 );
   ch->WAIT_STATE( skill_table[gsn_aid]->beats );
   if( !can_use_skill( ch, percent, gsn_aid ) )
   {
      ch->print( "You fail.\r\n" );
      ch->learn_from_failure( gsn_aid );
      return;
   }

   act( AT_SKILL, "You aid $N!", ch, NULL, victim, TO_CHAR );
   act( AT_SKILL, "$n aids $N!", ch, NULL, victim, TO_NOTVICT );
   ch->adjust_favor( 8, 1 );
   if( victim->hit < 1 )
      victim->hit = 1;

   victim->update_pos(  );
   act( AT_SKILL, "$n aids you!", ch, NULL, victim, TO_VICT );
}

int mount_ego_check( char_data * ch, char_data * horse )
{
   int ride_ego, drag_ego, align, check;

   if( ch->isnpc(  ) )
      return -5;

   if( IsDragon( horse ) )
   {
      drag_ego = horse->level * 2;
      if( horse->has_actflag( ACT_AGGRESSIVE ) || horse->has_actflag( ACT_META_AGGR ) )
         drag_ego += horse->level;

      ride_ego = ch->pcdata->learned[gsn_mount] / 10 + ch->level / 2;

      if( ch->is_affected( gsn_dragon_ride ) )
         ride_ego += ( ( ch->get_curr_int(  ) + ch->get_curr_wis(  ) ) / 2 );
      align = ch->alignment - horse->alignment;
      if( align < 0 )
         align = -align;
      align /= 100;
      align -= 5;
      drag_ego += align;
      if( horse->hit > 0 )
         drag_ego -= horse->max_hit / horse->hit;
      else
         drag_ego = 0;
      if( ch->hit )
         ride_ego -= ch->max_hit / ch->hit;
      else
         ride_ego = 0;

      check = drag_ego + number_range( 1, 10 ) - ( ride_ego + number_range( 1, 10 ) );
   }
   else
   {
      drag_ego = horse->level;

      if( drag_ego > 15 )
         drag_ego *= 2;

      ride_ego = ch->pcdata->learned[gsn_mount] / 10 + ch->level;

      if( ch->is_affected( gsn_dragon_ride ) )
         ride_ego += ( ch->get_curr_int(  ) + ch->get_curr_wis(  ) );
      check = drag_ego + number_range( 1, 5 ) - ( ride_ego + number_range( 1, 10 ) );
      if( horse->is_pet(  ) && horse->master == ch )
         check = -1;
   }
   return check;
}

CMDF( do_mount )
{
   char_data *victim;
   short learned;
   int check;

   if( !ch->isnpc(  ) && ch->level < skill_table[gsn_mount]->skill_level[ch->Class] )
   {
      ch->print( "I don't think that would be a good idea...\r\n" );
      return;
   }

   if( ch->mount )
   {
      ch->print( "You're already mounted!\r\n" );
      return;
   }

   if( !( victim = ch->get_char_room( argument ) ) )
   {
      ch->print( "You can't find that here.\r\n" );
      return;
   }

   if( !victim->has_actflag( ACT_MOUNTABLE ) )
   {
      ch->print( "You can't mount that!\r\n" );
      return;
   }

   if( victim->has_actflag( ACT_MOUNTED ) )
   {
      ch->print( "That mount already has a rider.\r\n" );
      return;
   }

   if( victim->position < POS_STANDING )
   {
      ch->print( "Your mount must be standing.\r\n" );
      return;
   }

   if( victim->position == POS_FIGHTING || victim->fighting )
   {
      ch->print( "Your mount is moving around too much.\r\n" );
      return;
   }

   if( victim->my_rider && victim->my_rider != ch )   /* prevents dragon theft */
   {
      ch->print( "This is someone else's dragon.\r\n" );
      return;
   }

   ch->WAIT_STATE( skill_table[gsn_mount]->beats );

   check = mount_ego_check( ch, victim );
   if( check > 5 )
   {
      act( AT_SKILL, "$N snarls and attacks!", ch, NULL, victim, TO_CHAR );
      act( AT_SKILL, "As $n tries to mount $N, $N attacks $n!", ch, NULL, victim, TO_NOTVICT );
      cmdf( victim, "kill %s", ch->name );
      return;
   }
   else if( check > -1 )
   {
      act( AT_SKILL, "$N moves out of the way and you fall on your butt.", ch, NULL, victim, TO_CHAR );
      act( AT_SKILL, "as $n tries to mount $N, $N moves out of the way", ch, NULL, victim, TO_NOTVICT );
      ch->position = POS_SITTING;
      return;
   }

   if( !ch->isnpc(  ) )
      learned = ch->pcdata->learned[gsn_mount];
   else
      learned = 0;

   if( ch->is_affected( gsn_dragon_ride ) )
      learned += ch->level;

   if( ch->isnpc(  ) || number_percent(  ) < learned )
   {
      victim->set_actflag( ACT_MOUNTED );
      ch->mount = victim;
      act( AT_SKILL, "You mount $N.", ch, NULL, victim, TO_CHAR );
      act( AT_SKILL, "$n skillfully mounts $N.", ch, NULL, victim, TO_NOTVICT );
      act( AT_SKILL, "$n mounts you.", ch, NULL, victim, TO_VICT );
      ch->position = POS_MOUNTED;
   }
   else
   {
      act( AT_SKILL, "You unsuccessfully try to mount $N.", ch, NULL, victim, TO_CHAR );
      act( AT_SKILL, "$n unsuccessfully attempts to mount $N.", ch, NULL, victim, TO_NOTVICT );
      act( AT_SKILL, "$n tries to mount you.", ch, NULL, victim, TO_VICT );
      ch->learn_from_failure( gsn_mount );
   }
}

CMDF( do_dismount )
{
   char_data *victim;
   bool fell = false;

   if( !( victim = ch->mount ) )
   {
      ch->print( "You're not mounted.\r\n" );
      return;
   }

   ch->WAIT_STATE( skill_table[gsn_mount]->beats );
   if( ch->isnpc(  ) || number_percent(  ) < ch->pcdata->learned[gsn_mount] )
   {
      act( AT_SKILL, "You dismount $N.", ch, NULL, victim, TO_CHAR );
      act( AT_SKILL, "$n skillfully dismounts $N.", ch, NULL, victim, TO_NOTVICT );
      act( AT_SKILL, "$n dismounts you.  Whew!", ch, NULL, victim, TO_VICT );
      victim->unset_actflag( ACT_MOUNTED );
      ch->mount = NULL;
      ch->position = POS_STANDING;
   }
   else
   {
      act( AT_SKILL, "You fall off while dismounting $N.  Ouch!", ch, NULL, victim, TO_CHAR );
      act( AT_SKILL, "$n falls off of $N while dismounting.", ch, NULL, victim, TO_NOTVICT );
      act( AT_SKILL, "$n falls off your back.", ch, NULL, victim, TO_VICT );
      ch->learn_from_failure( gsn_mount );
      victim->unset_actflag( ACT_MOUNTED );
      ch->mount = NULL;
      ch->position = POS_SITTING;
      fell = true;
      global_retcode = damage( ch, ch, 1, TYPE_UNDEFINED );
   }
   check_mount_objs( ch, fell ); /* Check for ITEM_MUSTMOUNT stuff */
}

/*
 * Check for parry.
 */
bool check_parry( char_data * ch, char_data * victim )
{
   int chances;

   if( !victim->IS_AWAKE(  ) )
      return false;

   if( !victim->has_defense( DFND_PARRY ) )
      return false;

   if( victim->isnpc(  ) )
   {
      /*
       * Tuan was here.  :) 
       */
      chances = UMIN( 60, 2 * victim->level );
   }
   else
   {
      if( victim->get_eq( WEAR_WIELD ) )
         return false;
      chances = ( int )( victim->LEARNED( gsn_parry ) / sysdata->parry_mod );
   }

   /*
    * Put in the call to chance() to allow penalties for misaligned clannies. 
    */
   if( chances != 0 && victim->morph )
      chances += victim->morph->parry;

   if( !victim->chance( chances + victim->level - ch->level ) )
   {
      victim->learn_from_failure( gsn_parry );
      return false;
   }

   /*
    * Modified by Tarl 24 April 02 to reduce combat spam with GAG flag. 
    *
    * BAD TARL! You forgot yur isnpc checks here :) 
    * Macro cleanup made that unnecessary on 9-18-03 - Samson 
    */
   if( !victim->has_pcflag( PCFLAG_GAG ) )
      act( AT_SKILL, "You parry $n's attack.", ch, NULL, victim, TO_VICT );

   if( !ch->isnpc(  ) && !ch->has_pcflag( PCFLAG_GAG ) )
      act( AT_SKILL, "$N parries your attack.", ch, NULL, victim, TO_CHAR );

   act( AT_SKILL, "$N parries $n's attack.", ch, NULL, victim, TO_NOTVICT );

   return true;
}

/*
 * Check for dodge.
 */
bool check_dodge( char_data * ch, char_data * victim )
{
   int chances;

   if( !victim->IS_AWAKE(  ) )
      return false;

   if( !victim->has_defense( DFND_DODGE ) )
      return false;

   if( victim->isnpc(  ) )
      chances = UMIN( 60, 2 * victim->level );
   else
      chances = ( int )( victim->LEARNED( gsn_dodge ) / sysdata->dodge_mod );

   if( chances != 0 && victim->morph != NULL )
      chances += victim->morph->dodge;

   /*
    * Consider luck as a factor 
    */
   if( !victim->chance( chances + victim->level - ch->level ) )
   {
      victim->learn_from_failure( gsn_dodge );
      return false;
   }
   /*
    * Modified by Tarl 24 April 02 to reduce combat spam with GAG flag. 
    * And yes, I forgot the NPC checks here at first, too. :P 
    * And the macro cleanup made this one moot as well - Samson 
    */
   if( !victim->has_pcflag( PCFLAG_GAG ) )
      act( AT_SKILL, "You dodge $n's attack.", ch, NULL, victim, TO_VICT );

   if( !ch->has_pcflag( PCFLAG_GAG ) )
      act( AT_SKILL, "$N dodges your attack.", ch, NULL, victim, TO_CHAR );

   act( AT_SKILL, "$N dodges $n's attack.", ch, NULL, victim, TO_NOTVICT );

   return true;
}

bool check_tumble( char_data * ch, char_data * victim )
{
   int chances;

   if( ( victim->Class != CLASS_ROGUE && victim->Class != CLASS_MONK ) || !victim->IS_AWAKE(  ) )
      return false;
   if( !victim->isnpc(  ) && !victim->pcdata->learned[gsn_tumble] > 0 )
      return false;
   if( victim->isnpc(  ) )
      chances = UMIN( 60, 2 * victim->level );
   else
      chances = ( int )( victim->LEARNED( gsn_tumble ) / sysdata->tumble_mod + ( victim->get_curr_dex(  ) - 13 ) );
   if( chances != 0 && victim->morph )
      chances += victim->morph->tumble;
   if( !victim->chance( chances + victim->level - ch->level ) )
   {
      victim->learn_from_failure( gsn_tumble );
      return false;
   }
   act( AT_SKILL, "You tumble away from $n's attack.", ch, NULL, victim, TO_VICT );
   act( AT_SKILL, "$N tumbles away from your attack.", ch, NULL, victim, TO_CHAR );
   act( AT_SKILL, "$N tumbles away from $n's attack.", ch, NULL, victim, TO_NOTVICT );
   return true;
}

CMDF( do_poison_weapon )
{
   if( !ch->isnpc(  ) && ch->level < skill_table[gsn_poison_weapon]->skill_level[ch->Class] )
   {
      ch->print( "What do you think you are, a thief?\r\n" );
      return;
   }

   if( argument.empty(  ) )
   {
      ch->print( "What are you trying to poison?\r\n" );
      return;
   }
   if( ch->fighting )
   {
      ch->print( "While you're fighting?  Nice try.\r\n" );
      return;
   }
   if( ms_find_obj( ch ) )
      return;

   obj_data *obj;
   if( !( obj = ch->get_obj_carry( argument ) ) )
   {
      ch->print( "You do not have that weapon.\r\n" );
      return;
   }
   if( obj->item_type != ITEM_WEAPON )
   {
      ch->print( "That item is not a weapon.\r\n" );
      return;
   }
   if( obj->extra_flags.test( ITEM_POISONED ) )
   {
      ch->print( "That weapon is already poisoned.\r\n" );
      return;
   }
   if( obj->extra_flags.test( ITEM_CLANOBJECT ) )
   {
      ch->print( "It doesn't appear to be fashioned of a poisonable material.\r\n" );
      return;
   }

   /*
    * Now we have a valid weapon...check to see if we have the powder. 
    */
   obj_data *pobj = NULL, *wobj = NULL;
   list < obj_data * >::iterator iobj;
   bool pfound = false;
   for( iobj = ch->carrying.begin(  ); iobj != ch->carrying.end(  ); ++iobj )
   {
      pobj = *iobj;
      if( pobj->pIndexData->vnum == OBJ_VNUM_BLACK_POWDER )
      {
         pfound = true;
         break;
      }
   }
   if( !pfound )
   {
      ch->print( "You do not have the black poison powder.\r\n" );
      return;
   }

   /*
    * Okay, we have the powder...do we have water? 
    */
   bool wfound = false;
   for( iobj = ch->carrying.begin(  ); iobj != ch->carrying.end(  ); ++iobj )
   {
      wobj = *iobj;
      if( wobj->item_type == ITEM_DRINK_CON && wobj->value[1] > 0 && wobj->value[2] == 0 )
      {
         wfound = true;
         break;
      }
   }
   if( !wfound )
   {
      ch->print( "You have no water to mix with the powder.\r\n" );
      return;
   }

   /*
    * Great, we have the ingredients...but is the thief smart enough? 
    * Modified by Tarl 24 April 02 Lowered the wisdom requirement from 16 to 14. 
    */
   if( !ch->isnpc(  ) && ch->get_curr_wis(  ) < 14 )
   {
      ch->print( "You can't quite remember what to do...\r\n" );
      return;
   }

   /*
    * And does the thief have steady enough hands? 
    */
   if( !ch->isnpc(  ) && ( ( ch->get_curr_dex(  ) < 16 ) || ch->pcdata->condition[COND_DRUNK] > 0 ) )
   {
      ch->print( "Your hands aren't steady enough to properly mix the poison.\r\n" );
      return;
   }
   ch->WAIT_STATE( skill_table[gsn_poison_weapon]->beats );

   int percent = ( number_percent(  ) - ( ch->get_curr_lck(  ) - 14 ) );

   /*
    * Check the skill percentage 
    */
   pobj->separate(  );
   wobj->separate(  );
   if( !can_use_skill( ch, percent, gsn_poison_weapon ) )
   {
      ch->print( "&RYou failed and spill some on yourself.  Ouch!&w\r\n" );
      damage( ch, ch, ch->level, gsn_poison_weapon );
      act( AT_RED, "$n spills the poison all over!", ch, NULL, NULL, TO_ROOM );
      pobj->extract(  );
      wobj->extract(  );
      ch->learn_from_failure( gsn_poison_weapon );
      return;
   }
   obj->separate(  );

   /*
    * Well, I'm tired of waiting.  Are you? 
    */
   act( AT_RED, "You mix $p in $P, creating a deadly poison!", ch, pobj, wobj, TO_CHAR );
   act( AT_RED, "$n mixes $p in $P, creating a deadly poison!", ch, pobj, wobj, TO_ROOM );
   act( AT_GREEN, "You pour the poison over $p, which glistens wickedly!", ch, obj, NULL, TO_CHAR );
   act( AT_GREEN, "$n pours the poison over $p, which glistens wickedly!", ch, obj, NULL, TO_ROOM );
   obj->extra_flags.set( ITEM_POISONED );
   obj->cost *= 2;
   /*
    * Set an object timer.  Don't want proliferation of poisoned weapons 
    */
   obj->timer = UMIN( obj->level, ch->level );

   if( obj->extra_flags.test( ITEM_BLESS ) )
      obj->timer *= 2;

   if( obj->extra_flags.test( ITEM_MAGIC ) )
      obj->timer *= 2;

   /*
    * WHAT?  All of that, just for that one bit?  How lame. ;) 
    */
   act( AT_BLUE, "The remainder of the poison eats through $p.", ch, wobj, NULL, TO_CHAR );
   act( AT_BLUE, "The remainder of the poison eats through $p.", ch, wobj, NULL, TO_ROOM );
   pobj->extract(  );
   wobj->extract(  );
}

CMDF( do_scribe )
{
   obj_data *scroll;
   int sn, mana;

   if( ch->isnpc(  ) )
      return;

   if( !ch->isnpc(  ) && ch->level < skill_table[gsn_scribe]->skill_level[ch->Class] )
   {
      ch->print( "A skill such as this requires more magical ability than that of your Class.\r\n" );
      return;
   }

   if( argument.empty(  ) )
   {
      ch->print( "Scribe what?\r\n" );
      return;
   }

   if( ms_find_obj( ch ) )
      return;

   if( ( sn = find_spell( ch, argument, true ) ) < 0 )
   {
      ch->print( "You have not learned that spell.\r\n" );
      return;
   }

   if( !str_cmp( argument, "word of recall" ) )
      sn = find_spell( ch, "recall", false );

   if( skill_table[sn]->spell_fun == spell_null )
   {
      ch->print( "That's not a spell!\r\n" );
      return;
   }

   if( SPELL_FLAG( skill_table[sn], SF_NOSCRIBE ) )
   {
      ch->print( "You cannot scribe that spell.\r\n" );
      return;
   }

   mana = ch->isnpc(  )? 0 : UMAX( skill_table[sn]->min_mana, 100 / ( 2 + ch->level - skill_table[sn]->skill_level[ch->Class] ) );

   mana *= 5;

   if( !ch->isnpc(  ) && ch->mana < mana )
   {
      ch->print( "You don't have enough mana.\r\n" );
      return;
   }

   if( !( scroll = ch->get_eq( WEAR_HOLD ) ) )
   {
      ch->print( "You must be holding a blank scroll to scribe it.\r\n" );
      return;
   }

   if( scroll->pIndexData->vnum != OBJ_VNUM_SCROLL_SCRIBING )
   {
      ch->print( "You must be holding a blank scroll to scribe it.\r\n" );
      return;
   }

   if( ( scroll->value[1] != -1 ) && ( scroll->pIndexData->vnum == OBJ_VNUM_SCROLL_SCRIBING ) )
   {
      ch->print( "That scroll has already been inscribed.\r\n" );
      return;
   }

   if( !process_spell_components( ch, sn ) )
   {
      ch->learn_from_failure( gsn_scribe );
      ch->mana -= ( mana / 2 );
      return;
   }

   if( !can_use_skill( ch, number_percent(  ), gsn_scribe ) )
   {
      ch->print( "&[magic]You failed.\r\n" );
      ch->learn_from_failure( gsn_scribe );
      ch->mana -= ( mana / 2 );
      return;
   }

   scroll->value[1] = sn;
   scroll->value[0] = ch->level;
   stralloc_printf( &scroll->short_descr, "%s scroll", skill_table[sn]->name );
   stralloc_printf( &scroll->objdesc, "A glowing scroll inscribed '%s' lies in the dust.", skill_table[sn]->name );
   stralloc_printf( &scroll->name, "scroll scribing %s", skill_table[sn]->name );

   act( AT_MAGIC, "$n magically scribes $p.", ch, scroll, NULL, TO_ROOM );
   act( AT_MAGIC, "You magically scribe $p.", ch, scroll, NULL, TO_CHAR );

   ch->WAIT_STATE( skill_table[gsn_scribe]->beats );

   ch->mana -= mana;
}

CMDF( do_brew )
{
   if( ch->isnpc(  ) )
      return;

   if( !ch->isnpc(  ) && ch->level < skill_table[gsn_brew]->skill_level[ch->Class] )
   {
      ch->print( "A skill such as this requires more magical ability than that of your class.\r\n" );
      return;
   }

   if( argument.empty(  ) )
   {
      ch->print( "Brew what?\r\n" );
      return;
   }

   if( ms_find_obj( ch ) )
      return;

   int sn;
   if( ( sn = find_spell( ch, argument, true ) ) < 0 )
   {
      ch->print( "You have not learned that spell.\r\n" );
      return;
   }

   if( !str_cmp( argument, "word of recall" ) )
      sn = find_spell( ch, "recall", false );

   if( skill_table[sn]->spell_fun == spell_null )
   {
      ch->print( "That's not a spell!\r\n" );
      return;
   }

   if( SPELL_FLAG( skill_table[sn], SF_NOBREW ) )
   {
      ch->print( "You cannot brew that spell.\r\n" );
      return;
   }

   int mana = ch->isnpc(  )? 0 : UMAX( skill_table[sn]->min_mana,
                                       100 / ( 2 + ch->level - skill_table[sn]->skill_level[ch->Class] ) );

   mana *= 4;

   if( !ch->isnpc(  ) && ch->mana < mana )
   {
      ch->print( "You don't have enough mana.\r\n" );
      return;
   }

   bool found = false;
   list < obj_data * >::iterator iobj;
   for( iobj = ch->in_room->objects.begin(  ); iobj != ch->in_room->objects.end(  ); ++iobj )
   {
      obj_data *fire = *iobj;
      if( fire->item_type == ITEM_FIRE )
      {
         found = true;
         break;
      }
   }

   if( !found )
   {
      ch->print( "There must be a fire in the room to brew a potion.\r\n" );
      return;
   }

   obj_data *potion;
   if( !( potion = ch->get_eq( WEAR_HOLD ) ) )
   {
      ch->print( "You must be holding an empty flask to brew a potion.\r\n" );
      return;
   }

   if( potion->pIndexData->vnum != OBJ_VNUM_FLASK_BREWING )
   {
      ch->print( "You must be holding an empty flask to brew a potion.\r\n" );
      return;
   }

   if( ( potion->value[1] != -1 ) && ( potion->pIndexData->vnum == OBJ_VNUM_FLASK_BREWING ) )
   {
      ch->print( "That's not an empty flask.\r\n" );
      return;
   }

   if( !process_spell_components( ch, sn ) )
   {
      ch->learn_from_failure( gsn_brew );
      ch->mana -= ( mana / 2 );
      return;
   }

   if( !can_use_skill( ch, number_percent(  ), gsn_brew ) )
   {
      ch->set_color( AT_MAGIC );
      ch->print( "&[magic]You failed.\r\n" );
      ch->learn_from_failure( gsn_brew );
      ch->mana -= ( mana / 2 );
      return;
   }

   potion->value[1] = sn;
   potion->value[0] = ch->level;
   stralloc_printf( &potion->short_descr, "%s potion", skill_table[sn]->name );
   stralloc_printf( &potion->objdesc, "A strange potion labelled '%s' sizzles in a glass flask.", skill_table[sn]->name );
   stralloc_printf( &potion->name, "flask potion %s", skill_table[sn]->name );

   act( AT_MAGIC, "$n brews up $p.", ch, potion, NULL, TO_ROOM );
   act( AT_MAGIC, "You brew up $p.", ch, potion, NULL, TO_CHAR );

   ch->WAIT_STATE( skill_table[gsn_brew]->beats );

   ch->mana -= mana;
}

CMDF( do_circle )
{
   char_data *victim;
   obj_data *obj;
   int percent;

   if( ch->isnpc(  ) && ch->has_aflag( AFF_CHARM ) )
   {
      ch->print( "You can't concentrate enough for that.\r\n" );
      return;
   }

   if( ch->mount )
   {
      ch->print( "You can't circle while mounted.\r\n" );
      return;
   }

   if( argument.empty(  ) )
   {
      ch->print( "Circle around whom?\r\n" );
      return;
   }

   if( !( victim = ch->get_char_room( argument ) ) )
   {
      ch->print( "They aren't here.\r\n" );
      return;
   }

   if( victim == ch )
   {
      ch->print( "How can you sneak up on yourself?\r\n" );
      return;
   }

   if( is_safe( ch, victim ) )
      return;

   if( !( obj = ch->get_eq( WEAR_WIELD ) ) )
   {
      ch->print( "You need to wield a piercing or stabbing weapon.\r\n" );
      return;
   }

   if( obj->value[4] != WEP_DAGGER )
   {
      if( ( obj->value[4] == WEP_SWORD && obj->value[3] != DAM_PIERCE ) || obj->value[4] != WEP_SWORD )
      {
         ch->print( "You need to wield a piercing or stabbing weapon.\r\n" );
         return;
      }
   }

   if( !ch->fighting )
   {
      ch->print( "You can't circle when you aren't fighting.\r\n" );
      return;
   }

   if( !victim->fighting )
   {
      ch->print( "You can't circle around a person who is not fighting.\r\n" );
      return;
   }

   if( victim->num_fighting < 2 )
   {
      act( AT_PLAIN, "You can't circle around them without a distraction.", ch, NULL, victim, TO_CHAR );
      return;
   }

   percent = number_percent(  ) - ( ch->get_curr_lck(  ) - 16 ) + ( victim->get_curr_lck(  ) - 13 );

   if( check_illegal_pk( ch, victim ) )
   {
      ch->print( "You can't do that to another player!\r\n" );
      return;
   }

   ch->WAIT_STATE( skill_table[gsn_circle]->beats );
   if( can_use_skill( ch, percent, gsn_circle ) )
   {
      ch->WAIT_STATE( 2 * sysdata->pulseviolence );
      act( AT_SKILL, "$n sneaks up behind $N, stabbing $M in the back!", ch, NULL, victim, TO_NOTVICT );
      act( AT_SKILL, "$n sneaks up behind you, stabbing you in the back!", ch, NULL, victim, TO_VICT );
      act( AT_SKILL, "You sneak up behind $N, stabbing $M in the back!", ch, NULL, victim, TO_CHAR );
      global_retcode = multi_hit( ch, victim, gsn_circle );
      ch->adjust_favor( 10, 1 );
   }
   else
   {
      ch->learn_from_failure( gsn_circle );
      ch->WAIT_STATE( 2 * sysdata->pulseviolence );
      act( AT_SKILL, "$n nearly slices off $s finger trying to backstab $N!", ch, NULL, victim, TO_NOTVICT );
      act( AT_SKILL, "$n nearly slices off $s finger trying to backstab you!", ch, NULL, victim, TO_VICT );
      act( AT_SKILL, "You nearly slice off your finger trying to backstab $N!", ch, NULL, victim, TO_CHAR );
      global_retcode = damage( ch, victim, 0, gsn_circle );
   }
}

/* Berserk and HitAll. -- Altrag */
CMDF( do_berserk )
{
   short percent;
   affect_data af;

   if( !ch->fighting )
   {
      ch->print( "But you aren't fighting!\r\n" );
      return;
   }

   if( ch->has_aflag( AFF_BERSERK ) )
   {
      ch->print( "Your rage is already at its peak!\r\n" );
      return;
   }

   percent = ch->LEARNED( gsn_berserk );
   ch->WAIT_STATE( skill_table[gsn_berserk]->beats );
   if( !ch->chance( percent ) )
   {
      ch->print( "You couldn't build up enough rage.\r\n" );
      ch->learn_from_failure( gsn_berserk );
      return;
   }
   af.type = gsn_berserk;
   /*
    * Hmmm.. 10-20 combat rounds at level 50.. good enough for most mobs,
    * and if not they can always go berserk again.. shrug.. maybe even
    * too high. -- Altrag 
    */
   af.duration = number_range( ch->level / 5, ch->level * 2 / 5 );
   /*
    * Hmm.. you get stronger when yer really enraged.. mind over matter
    * type thing.. 
    */
   af.location = APPLY_STR;
   af.modifier = 1;
   af.bit = AFF_BERSERK;
   ch->affect_to_char( &af );
   ch->print( "You start to lose control..\r\n" );
}

CMDF( do_hitall )
{
   short nvict = 0, nhit = 0, percent;

   if( ch->in_room->flags.test( ROOM_SAFE ) )
   {
      ch->print( "&BA godly force prevents you.\r\n" );
      return;
   }

   if( ch->in_room->people.empty(  ) )
   {
      ch->print( "There's no one else here!\r\n" );
      return;
   }

   percent = ch->LEARNED( gsn_hitall );
   list < char_data * >::iterator ich;
   for( ich = ch->in_room->people.begin(  ); ich != ch->in_room->people.end(  ); )
   {
      char_data *vch = *ich;
      ++ich;

      if( is_same_group( ch, vch ) || !is_legal_kill( ch, vch ) || !ch->can_see( vch, false ) || is_safe( ch, vch ) )
         continue;
      if( ++nvict > ch->level / 5 )
         break;
      if( ch->chance( percent ) )
      {
         ++nhit;
         global_retcode = one_hit( ch, vch, gsn_hitall );
      }
      else
         global_retcode = damage( ch, vch, 0, gsn_hitall );
      /*
       * Fireshield, etc. could kill ch too.. :>.. -- Altrag 
       */
      if( global_retcode == rCHAR_DIED || ch->char_died(  ) )
         return;
   }
   if( !nvict )
   {
      ch->print( "There's no one else here!\r\n" );
      return;
   }
   ch->move = UMAX( 0, ch->move - nvict * 3 + nhit );
   if( !nhit )
      ch->learn_from_failure( gsn_hitall );
}

static const char *dir_desc[] = {
   "to the north",
   "to the east",
   "to the south",
   "to the west",
   "upwards",
   "downwards",
   "to the northeast",
   "to the northwest",
   "to the southeast",
   "to the southwest",
   "through the portal"
};

static const char *rng_desc[] = {
   "right here",
   "immediately",
   "nearby",
   "a ways",
   "a good ways",
   "far",
   "far off",
   "very far",
   "very far off",
   "in the distance"
};

static void scanroom( char_data * ch, room_index * room, int dir, int maxdist, int dist )
{
   list < char_data * >::iterator ich;
   for( ich = room->people.begin(  ); ich != room->people.end(  ); ++ich )
   {
      char_data *tch = *ich;

      if( ch->can_see( tch, false ) && !is_ignoring( tch, ch ) )
         ch->printf( "%-30s : %s %s\r\n", tch->isnpc(  )? tch->short_descr : tch->name, rng_desc[dist], dist == 0 ? "" : dir_desc[dir] );
   }

   list < exit_data * >::iterator ex;
   exit_data *pexit = NULL;
   for( ex = room->exits.begin(  ); ex != room->exits.end(  ); ++ex )
   {
      exit_data *iexit = *ex;

      if( iexit->vdir == dir )
      {
         pexit = iexit;
         break;
      }
   }

   if( !pexit || pexit->vdir != dir || pexit->vdir == DIR_SOMEWHERE || maxdist - 1 == 0
       || IS_EXIT_FLAG( pexit, EX_CLOSED ) || IS_EXIT_FLAG( pexit, EX_DIG ) || IS_EXIT_FLAG( pexit, EX_FORTIFIED )
       || IS_EXIT_FLAG( pexit, EX_HEAVY ) || IS_EXIT_FLAG( pexit, EX_MEDIUM ) || IS_EXIT_FLAG( pexit, EX_LIGHT )
       || IS_EXIT_FLAG( pexit, EX_CRUMBLING ) || IS_EXIT_FLAG( pexit, EX_OVERLAND ) )
      return;

   scanroom( ch, pexit->to_room, dir, maxdist - 1, dist + 1 );
}

void map_scan( char_data * ch );

/* Scan no longer accepts a direction argument */
CMDF( do_scan )
{
   int maxdist = ch->level / 10;
   maxdist = URANGE( 1, maxdist, 9 );

   if( ch->has_pcflag( PCFLAG_ONMAP ) || ch->has_actflag( ACT_ONMAP ) )
   {
      map_scan( ch );
      return;
   }

   scanroom( ch, ch->in_room, -1, 1, 0 );

   list < exit_data * >::iterator iexit;
   for( iexit = ch->in_room->exits.begin(  ); iexit != ch->in_room->exits.end(  ); ++iexit )
   {
      exit_data *pexit = *iexit;

      if( IS_EXIT_FLAG( pexit, EX_DIG ) || IS_EXIT_FLAG( pexit, EX_CLOSED )
          || IS_EXIT_FLAG( pexit, EX_FORTIFIED ) || IS_EXIT_FLAG( pexit, EX_HEAVY )
          || IS_EXIT_FLAG( pexit, EX_MEDIUM ) || IS_EXIT_FLAG( pexit, EX_LIGHT ) || IS_EXIT_FLAG( pexit, EX_CRUMBLING ) || IS_EXIT_FLAG( pexit, EX_OVERLAND ) )
         continue;

      if( pexit->vdir == DIR_SOMEWHERE && !ch->is_immortal(  ) )
         continue;

      scanroom( ch, pexit->to_room, pexit->vdir, maxdist, 1 );
   }
}

CMDF( do_slice )
{
   obj_data *corpse, *obj, *slice;
   mob_index *pMobIndex;

   if( !ch->isnpc(  ) && !ch->is_immortal(  ) && ch->level < skill_table[gsn_slice]->skill_level[ch->Class] )
   {
      ch->print( "You are not learned in this skill.\r\n" );
      return;
   }

   if( argument.empty(  ) )
   {
      ch->print( "From what do you wish to slice meat?\r\n" );
      return;
   }

   if( !( obj = ch->get_eq( WEAR_WIELD ) ) || ( obj->value[3] != DAM_SLASH && obj->value[3] != DAM_HACK && obj->value[3] != DAM_PIERCE && obj->value[3] != DAM_STAB ) )
   {
      ch->print( "You need to wield a sharp weapon.\r\n" );
      return;
   }

   if( !( corpse = ch->get_obj_here( argument ) ) )
   {
      ch->print( "You can't find that here.\r\n" );
      return;
   }

   if( corpse->item_type != ITEM_CORPSE_NPC || corpse->timer < 5 || corpse->value[3] < 75 )
   {
      ch->print( "That is not a suitable source of meat.\r\n" );
      return;
   }

   if( !( pMobIndex = get_mob_index( corpse->value[4] ) ) )
   {
      ch->print( "Error - report to immortals\r\n" );
      bug( "%s: Can not find mob for value[4] of corpse", __func__ );
      return;
   }

   if( !can_use_skill( ch, number_percent(  ), gsn_slice ) && !ch->is_immortal(  ) )
   {
      ch->print( "You fail to slice the meat properly.\r\n" );
      ch->learn_from_failure( gsn_slice );   /* Just in case they die :> */
      if( number_percent(  ) + ( ch->get_curr_dex(  ) - 13 ) < 10 )
      {
         act( AT_BLOOD, "You cut yourself!", ch, NULL, NULL, TO_CHAR );
         damage( ch, ch, ch->level, gsn_slice );
      }
      return;
   }

   if( !( slice = get_obj_index( OBJ_VNUM_SLICE )->create_object( 1 ) ) )
   {
      ch->print( "Error - report to immortals\r\n" );
      log_printf( "create_object: %s:%s, line %d.", __FILE__, __func__, __LINE__ );
      return;
   }

   stralloc_printf( &slice->name, "meat fresh slice %s", pMobIndex->player_name );
   stralloc_printf( &slice->short_descr, "a slice of raw meat from %s", pMobIndex->short_descr );
   stralloc_printf( &slice->objdesc, "A slice of raw meat from %s lies on the ground.", pMobIndex->short_descr );

   act( AT_BLOOD, "$n cuts a slice of meat from $p.", ch, corpse, NULL, TO_ROOM );
   act( AT_BLOOD, "You cut a slice of meat from $p.", ch, corpse, NULL, TO_CHAR );

   slice->to_char( ch );
   corpse->value[3] -= 25;
}

/*
 *  Fighting Styles - haus
 */
CMDF( do_style )
{
   if( ch->isnpc(  ) )
      return;

   if( argument.empty(  ) )
   {
      ch->printf( "&wAdopt which fighting style?  (current:  %s&w)\r\n",
                  ch->style == STYLE_BERSERK ? "&Rberserk" :
                  ch->style == STYLE_AGGRESSIVE ? "&Raggressive" : ch->style == STYLE_DEFENSIVE ? "&Ydefensive" : ch->style == STYLE_EVASIVE ? "&Yevasive" : "standard" );
      return;
   }

   if( !str_prefix( argument, "evasive" ) )
   {
      if( ch->level < skill_table[gsn_style_evasive]->skill_level[ch->Class] )
      {
         ch->print( "You have not yet learned enough to fight evasively.\r\n" );
         return;
      }
      ch->style = STYLE_EVASIVE;
      ch->print( "You adopt an evasive fighting style.\r\n" );
      return;
   }
   else if( !str_prefix( argument, "defensive" ) )
   {
      if( ch->level < skill_table[gsn_style_defensive]->skill_level[ch->Class] )
      {
         ch->print( "You have not yet learned enough to fight defensively.\r\n" );
         return;
      }
      ch->WAIT_STATE( skill_table[gsn_style_defensive]->beats );
      if( number_percent(  ) < ch->LEARNED( gsn_style_defensive ) )
      {
         /*
          * success 
          */
         if( ch->fighting )
         {
            ch->position = POS_DEFENSIVE;
            if( ch->IS_PKILL(  ) )
               act( AT_ACTION, "$n moves into a defensive posture.", ch, NULL, NULL, TO_ROOM );
         }
         ch->style = STYLE_DEFENSIVE;
         ch->print( "You adopt a defensive fighting style.\r\n" );
         return;
      }
      else
      {
         /*
          * failure 
          */
         ch->print( "You nearly trip in a lame attempt to adopt a defensive fighting style.\r\n" );
         ch->learn_from_failure( gsn_style_defensive );
         return;
      }
   }
   else if( !str_prefix( argument, "standard" ) )
   {
      if( ch->level < skill_table[gsn_style_standard]->skill_level[ch->Class] )
      {
         ch->print( "You have not yet learned enough to fight in the standard style.\r\n" );
         return;
      }
      ch->WAIT_STATE( skill_table[gsn_style_standard]->beats );
      if( number_percent(  ) < ch->LEARNED( gsn_style_standard ) )
      {
         /*
          * success 
          */
         if( ch->fighting )
         {
            ch->position = POS_FIGHTING;
            if( ch->IS_PKILL(  ) )
               act( AT_ACTION, "$n switches to a standard fighting style.", ch, NULL, NULL, TO_ROOM );
         }
         ch->style = STYLE_FIGHTING;
         ch->print( "You adopt a standard fighting style.\r\n" );
         return;
      }
      else
      {
         /*
          * failure 
          */
         ch->print( "You nearly trip in a lame attempt to adopt a standard fighting style.\r\n" );
         ch->learn_from_failure( gsn_style_standard );
         return;
      }
   }
   else if( !str_prefix( argument, "aggressive" ) )
   {
      if( ch->level < skill_table[gsn_style_aggressive]->skill_level[ch->Class] )
      {
         ch->print( "You have not yet learned enough to fight aggressively.\r\n" );
         return;
      }
      ch->WAIT_STATE( skill_table[gsn_style_aggressive]->beats );
      if( number_percent(  ) < ch->LEARNED( gsn_style_aggressive ) )
      {
         /*
          * success 
          */
         if( ch->fighting )
         {
            ch->position = POS_AGGRESSIVE;
            if( ch->IS_PKILL(  ) )
               act( AT_ACTION, "$n assumes an aggressive stance.", ch, NULL, NULL, TO_ROOM );
         }
         ch->style = STYLE_AGGRESSIVE;
         ch->print( "You adopt an aggressive fighting style.\r\n" );
         return;
      }
      else
      {
         /*
          * failure 
          */
         ch->print( "You nearly trip in a lame attempt to adopt an aggressive fighting style.\r\n" );
         ch->learn_from_failure( gsn_style_aggressive );
         return;
      }
   }
   else if( !str_prefix( argument, "berserk" ) )
   {
      if( ch->level < skill_table[gsn_style_berserk]->skill_level[ch->Class] )
      {
         ch->print( "You have not yet learned enough to fight as a berserker.\r\n" );
         return;
      }
      ch->WAIT_STATE( skill_table[gsn_style_berserk]->beats );
      if( number_percent(  ) < ch->LEARNED( gsn_style_berserk ) )
      {
         /*
          * success 
          */
         if( ch->fighting )
         {
            ch->position = POS_BERSERK;
            if( ch->IS_PKILL(  ) )
               act( AT_ACTION, "$n enters a wildly aggressive style.", ch, NULL, NULL, TO_ROOM );
         }
         ch->style = STYLE_BERSERK;
         ch->print( "You adopt a berserk fighting style.\r\n" );
         return;
      }
      else
      {
         /*
          * failure 
          */
         ch->print( "You nearly trip in a lame attempt to adopt a berserk fighting style.\r\n" );
         ch->learn_from_failure( gsn_style_berserk );
         return;
      }
   }
   ch->print( "Adopt which fighting style?\r\n" );
}

/*
 * Cook was coded by Blackmane and heavily modified by Shaddai
 */
CMDF( do_cook )
{
   if( ch->isnpc(  ) || ch->level < skill_table[gsn_cook]->skill_level[ch->Class] )
   {
      ch->print( "That skill is beyond your understanding.\r\n" );
      return;
   }

   if( argument.empty(  ) )
   {
      ch->print( "Cook what?\r\n" );
      return;
   }

   if( ms_find_obj( ch ) )
      return;

   obj_data *food;
   if( !( food = ch->get_obj_carry( argument ) ) )
   {
      ch->print( "You do not have that item.\r\n" );
      return;
   }
   if( food->item_type != ITEM_COOK )
   {
      ch->print( "How can you cook that?\r\n" );
      return;
   }
   if( food->value[2] > 2 )
   {
      ch->print( "That is already burnt to a crisp.\r\n" );
      return;
   }

   bool found = false;
   list < obj_data * >::iterator iobj;
   for( iobj = ch->in_room->objects.begin(  ); iobj != ch->in_room->objects.end(  ); ++iobj )
   {
      obj_data *fire = *iobj;
      if( fire->item_type == ITEM_FIRE )
      {
         found = true;
         break;
      }
   }
   if( !found )
   {
      ch->print( "There is no fire here!\r\n" );
      return;
   }

   food->separate(  );  /* Yeah, so you don't end up burning all your meat */

   if( number_percent(  ) > ch->LEARNED( gsn_cook ) )
   {
      food->timer = food->timer / 2;
      food->value[0] = 0;
      food->value[2] = 3;
      act( AT_MAGIC, "$p catches on fire burning it to a crisp!\r\n", ch, food, NULL, TO_CHAR );
      act( AT_MAGIC, "$n catches $p on fire burning it to a crisp.", ch, food, NULL, TO_ROOM );
      stralloc_printf( &food->short_descr, "a burnt %s", food->pIndexData->name );
      stralloc_printf( &food->objdesc, "A burnt %s.", food->pIndexData->name );
      ch->learn_from_failure( gsn_cook );
      return;
   }

   if( number_percent(  ) > 85 )
   {
      food->timer = food->timer * 3;
      food->value[2] += 2;
      act( AT_MAGIC, "$n overcooks $p.", ch, food, NULL, TO_ROOM );
      act( AT_MAGIC, "You overcook $p.", ch, food, NULL, TO_CHAR );
      stralloc_printf( &food->short_descr, "an overcooked %s", food->pIndexData->name );
      stralloc_printf( &food->objdesc, "An overcooked %s.", food->pIndexData->name );
   }
   else
   {
      food->timer = food->timer * 4;
      food->value[0] *= 2;
      act( AT_MAGIC, "$n roasts $p.", ch, food, NULL, TO_ROOM );
      act( AT_MAGIC, "You roast $p.", ch, food, NULL, TO_CHAR );
      stralloc_printf( &food->short_descr, "a roasted %s", food->pIndexData->name );
      stralloc_printf( &food->objdesc, "A roasted %s.", food->pIndexData->name );
      ++food->value[2];
   }
}

/* Everything from here down has been added by The AFKMud Team */

/* Centaur backheel skill, clone of kick, but needs to be separate for GSN reasons */
CMDF( do_backheel )
{
   char_data *victim;

   if( ch->isnpc(  ) && ch->has_aflag( AFF_CHARM ) )
   {
      ch->print( "You can't concentrate enough for that.\r\n" );
      return;
   }

   if( !ch->isnpc(  ) && ch->level < skill_table[gsn_backheel]->race_level[ch->race] )
   {
      ch->print( "You better leave the martial arts to fighters.\r\n" );
      return;
   }

   if( !( victim = ch->who_fighting(  ) ) )
   {
      if( !( victim = ch->get_char_room( argument ) ) )
      {
         ch->print( "They aren't here.\r\n" );
         return;
      }
   }

   if( !is_legal_kill( ch, victim ) || is_safe( ch, victim ) )
   {
      ch->print( "You can't do that!\r\n" );
      return;
   }

   ch->WAIT_STATE( skill_table[gsn_backheel]->beats );
   if( ( ch->isnpc(  ) || number_percent(  ) < ch->pcdata->learned[gsn_backheel] ) && victim->race != RACE_GHOST )
   {
      global_retcode = damage( ch, victim, ch->Class == CLASS_MONK ? ch->level : ( ch->level / 2 ), gsn_backheel );
      kick_messages( ch, victim, 1, global_retcode );
   }
   else
   {
      ch->learn_from_failure( gsn_backheel );
      global_retcode = damage( ch, victim, 0, gsn_backheel );
      kick_messages( ch, victim, 0, global_retcode );
   }
}

CMDF( do_tinker )
{
   obj_index *pobj = NULL;
   obj_data *obj = NULL;
   mob_index *pmob = NULL;
   char_data *mob = NULL;
   int cost = 10000;

   ch->set_color( AT_SKILL );

   if( argument.empty(  ) )
   {
      ch->print( "What do you wish to construct?\r\n" );
      return;
   }

   if( !str_cmp( argument, "flamethrower" ) )
      pobj = get_obj_index( OBJ_VNUM_FLAMETHROWER );

   if( !str_cmp( argument, "ladder" ) )
      pobj = get_obj_index( OBJ_VNUM_LADDER );

   if( !str_cmp( argument, "digger" ) )
      pobj = get_obj_index( OBJ_VNUM_DIGGER );

   if( !str_cmp( argument, "lockpick" ) )
      pobj = get_obj_index( OBJ_VNUM_GNOME_LOCKPICK );

   if( !str_cmp( argument, "breather" ) )
      pobj = get_obj_index( OBJ_VNUM_REBREATHER );

   if( !str_cmp( argument, "flyer" ) )
   {
      pmob = get_mob_index( MOB_VNUM_GNOME_FLYER );
      cost = 20000;
   }

   if( !pobj && !pmob )
   {
      ch->printf( "You cannot construct a %s.\r\n", argument.c_str(  ) );
      return;
   }

   if( ch->gold < cost )
   {
      ch->print( "You don't have enough gold for the parts!\r\n" );
      return;
   }

   ch->gold -= cost;

   ch->WAIT_STATE( skill_table[gsn_tinker]->beats );
   if( number_percent(  ) > ch->pcdata->learned[gsn_tinker] )
   {
      if( ch->pcdata->learned[gsn_tinker] > 0 )
      {
         ch->print( "You fiddle around for awhile, but only cost yourself money.\r\n" );
         ch->learn_from_failure( gsn_tinker );
      }
      return;
   }

   if( pobj )
   {
      obj = pobj->create_object( 1 );
      obj = obj->to_char( ch );

      ch->printf( "You tinker around awhile and construct %s!\r\n", obj->short_descr );
      return;
   }
   if( pmob )
   {
      mob = pmob->create_mobile(  );
      if( !mob->to_room( ch->in_room ) )
         log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __func__, __LINE__ );

      ch->printf( "You tinker around awhile and construct a %s!\r\n", mob->short_descr );
      return;
   }
   bug( "%s: Somehow reached the end of the function!!!", __func__ );
   ch->print( "Oops. Something mighty odd just happened. The imms have been informed.\r\n" );
   ch->print( "Reimbursing the gold you lost...\r\n" );
   ch->gold += cost;
}

CMDF( do_deathsong )
{
   affect_data af;

   ch->set_color( AT_SKILL );

   if( ch->has_aflag( AFF_DEATHSONG ) )
   {
      ch->print( "You can only use the song once per day.\r\n" );
      return;
   }

   ch->WAIT_STATE( skill_table[gsn_deathsong]->beats );

   af.type = gsn_deathsong;
   af.duration = 2616;  /* One day long */
   af.modifier = 0;
   af.location = APPLY_NONE;
   af.bit = AFF_DEATHSONG;
   ch->affect_to_char( &af );

   if( number_percent(  ) < ch->pcdata->learned[gsn_deathsong] )
   {
      af.type = gsn_deathsong;
      af.duration = ch->level;
      af.modifier = 20;
      af.location = APPLY_PARRY;
      af.bit = AFF_DEATHSONG;
      ch->affect_to_char( &af );

      af.type = gsn_deathsong;
      af.duration = ch->level;
      af.modifier = ch->level / 50;
      af.location = APPLY_DAMROLL;
      af.bit = AFF_DEATHSONG;
      ch->affect_to_char( &af );

      ch->print( "Your song increases your combat abilities.\r\n" );
   }
   else
   {
      if( ch->pcdata->learned[gsn_deathsong] > 0 )
      {
         ch->print( "Something distracts you and you fumble the words.\r\n" );
         ch->learn_from_failure( gsn_deathsong );
      }
   }
}

CMDF( do_tenacity )
{
   affect_data af;

   ch->set_color( AT_SKILL );

   if( ch->has_aflag( AFF_TENACITY ) )
   {
      ch->print( "You are already as tenacious as can be!\r\n" );
      return;
   }

   ch->WAIT_STATE( skill_table[gsn_tenacity]->beats );

   if( number_percent(  ) < ch->pcdata->learned[gsn_tenacity] )
   {
      af.type = gsn_tenacity;
      af.duration = ch->level;
      af.modifier = 2;
      af.location = APPLY_HITROLL;
      af.bit = AFF_TENACITY;
      ch->affect_to_char( &af );

      ch->print( "You psych yourself up into a tenacious frenzy.\r\n" );
   }
   else
   {
      if( ch->pcdata->learned[gsn_tenacity] > 0 )
      {
         ch->print( "You just can't seem to psych yourself up for some reason.\r\n" );
         ch->learn_from_failure( gsn_tenacity );
      }
   }
}

CMDF( do_reverie )
{
   affect_data af;

   ch->set_color( AT_SKILL );

   if( ch->has_aflag( AFF_REVERIE ) )
   {
      ch->print( "You may only use reverie once per day.\r\n" );
      return;
   }

   af.type = gsn_reverie;
   af.duration = 2616;  /* One day long */
   af.modifier = 0;
   af.location = APPLY_NONE;
   af.bit = AFF_REVERIE;
   ch->affect_to_char( &af );

   if( number_percent(  ) < ch->pcdata->learned[gsn_reverie] )
   {
      af.type = gsn_reverie;
      af.duration = ( int )( 2 * DUR_CONV );
      af.modifier = 80;
      af.location = APPLY_HIT_REGEN;
      af.bit = AFF_REVERIE;
      ch->affect_to_char( &af );

      af.type = gsn_reverie;
      af.duration = ( int )( 2 * DUR_CONV );
      af.modifier = 80;
      af.location = APPLY_MANA_REGEN;
      af.bit = AFF_REVERIE;
      ch->affect_to_char( &af );

      ch->print( "Your song speeds up your regeneration.\r\n" );
   }
   else
   {
      if( ch->pcdata->learned[gsn_reverie] > 0 )
      {
         ch->print( "Something distracts you, causing you to fumble the words.\r\n" );
         ch->learn_from_failure( gsn_reverie );
      }
   }
}

CMDF( do_bladesong )
{
   affect_data af;

   ch->set_color( AT_MAGIC );

   if( ch->has_aflag( AFF_BLADESONG ) )
   {
      ch->print( "You may only use bladesong once per day.\r\n" );
      return;
   }

   af.type = gsn_bladesong;
   af.duration = 2616;  /* One day long */
   af.modifier = 0;
   af.location = APPLY_NONE;
   af.bit = AFF_BLADESONG;
   ch->affect_to_char( &af );

   if( number_percent(  ) < ch->pcdata->learned[gsn_bladesong] )
   {
      af.type = gsn_bladesong;
      af.duration = ( ch->level / 2 );
      af.modifier = 2;
      af.location = APPLY_HITROLL;
      af.bit = AFF_BLADESONG;
      ch->affect_to_char( &af );

      af.type = gsn_bladesong;
      af.duration = ( ch->level / 2 );
      af.modifier = 2;
      af.location = APPLY_DAMROLL;
      af.bit = AFF_BLADESONG;
      ch->affect_to_char( &af );

      af.type = gsn_bladesong;
      af.duration = ( ch->level / 2 );
      af.modifier = 20;
      af.location = APPLY_DODGE;
      af.bit = AFF_BLADESONG;
      ch->affect_to_char( &af );

      ch->print( "Your song inspires you to heightened ability in combat.\r\n" );
   }
   else
   {
      if( ch->pcdata->learned[gsn_bladesong] > 0 )
      {
         ch->print( "Something distracts you, causing you to fumble the words.\r\n" );
         ch->learn_from_failure( gsn_bladesong );
      }
   }
}

CMDF( do_elvensong )
{
   affect_data af;

   ch->set_color( AT_MAGIC );

   if( ch->has_aflag( AFF_ELVENSONG ) )
   {
      ch->print( "You may only use Elven song once per day.\r\n" );
      return;
   }

   af.type = gsn_elvensong;
   af.duration = 2616;  /* One day long */
   af.modifier = 0;
   af.location = APPLY_NONE;
   af.bit = AFF_ELVENSONG;
   ch->affect_to_char( &af );

   if( number_percent(  ) < ch->pcdata->learned[gsn_elvensong] )
   {
      ch->print( "Your soothing song regenerates your health and energy.\r\n" );

      ch->hit += 40;
      ch->mana += 40;

      if( ch->hit >= ch->max_hit )
         ch->hit = ch->max_hit;
      if( ch->mana >= ch->max_mana )
         ch->mana = ch->max_mana;
   }
   else
   {
      if( ch->pcdata->learned[gsn_elvensong] > 0 )
      {
         ch->print( "Something distracts you, causing you to fumble the words.\r\n" );
         ch->learn_from_failure( gsn_elvensong );
      }
   }
}

/* Assassinate skill, added by Samson on unknown date. Code courtesy of unknown author from Smaug mailing list. */
CMDF( do_assassinate )
{
   char_data *victim;
   obj_data *obj;
   short percent;

   /*
    * Someone is gonna die if I come back later and find this removed again!!! 
    */
   if( ch->Class != CLASS_ROGUE && !ch->is_immortal(  ) )
   {
      ch->print( "What do you think you are? A Rogue???\r\n" );
      return;
   }

   if( ch->isnpc(  ) && ch->has_aflag( AFF_CHARM ) )
   {
      ch->print( "You can't do that right now.\r\n" );
      return;
   }

   if( argument.empty(  ) )
   {
      ch->print( "Who do you want to assassinate?\r\n" );
      return;
   }

   if( !( victim = ch->get_char_room( argument ) ) )
   {
      ch->print( "They aren't here.\r\n" );
      return;
   }

   if( victim->is_immortal(  ) && victim->level > ch->level )
   {
      ch->print( "I don't think so...\r\n" );
      return;
   }

   if( ch->mount )
   {
      ch->print( "You can't get close enough while mounted.\r\n" );
      return;
   }

   if( victim == ch )
   {
      ch->print( "How can you sneak up on yourself?\r\n" );
      return;
   }

   if( is_safe( ch, victim ) )
      return;

   /*
    * Added stabbing weapon. -Narn 
    */
   if( !( obj = ch->get_eq( WEAR_WIELD ) ) )
   {
      ch->print( "You need to wield a piercing or stabbing weapon.\r\n" );
      return;
   }

   if( obj->value[4] != WEP_DAGGER )
   {
      if( ( obj->value[4] == WEP_SWORD && obj->value[3] != DAM_PIERCE ) || obj->value[4] != WEP_SWORD )
      {
         ch->print( "You need to wield a piercing or stabbing weapon.\r\n" );
         return;
      }
   }

   if( victim->fighting )
   {
      ch->print( "You can't assassinate someone who is in combat.\r\n" );
      return;
   }

   /*
    * Can assassinate a char even if it's hurt as long as it's sleeping. -Tsunami 
    */
   if( victim->hit < victim->max_hit && victim->IS_AWAKE(  ) )
   {
      act( AT_PLAIN, "$N is hurt and suspicious ... you can't sneak up.", ch, NULL, victim, TO_CHAR );
      return;
   }

   ch->WAIT_STATE( skill_table[gsn_assassinate]->beats );
   percent = number_percent(  ) + UMAX( 0, ( victim->level - ch->level ) * 2 );
   if( ch->isnpc(  ) || percent < ch->pcdata->learned[gsn_assassinate] )
   {
      act( AT_ACTION, "You slip quietly up behind $N, plunging your", ch, NULL, victim, TO_CHAR );
      act( AT_ACTION, "$p deep into $s back!", ch, obj, victim, TO_CHAR );
      act( AT_ACTION, "Piercing a vital organ, $N falls to the ground, dead.", ch, NULL, victim, TO_CHAR );
      act( AT_ACTION, "$n slips behind you, and before you can react, plunges", ch, NULL, victim, TO_VICT );
      act( AT_ACTION, "$s $p deep into your back!", ch, obj, victim, TO_VICT );
      act( AT_ACTION, "You slip quietly off to your death as $e pierces a vital organ.....", ch, NULL, victim, TO_VICT );
      act( AT_ACTION, "$n slips quietly up behind $N, plunging $s", ch, NULL, victim, TO_NOTVICT );
      act( AT_ACTION, "$p deep into $S back!", ch, obj, victim, TO_NOTVICT );
      act( AT_ACTION, "$N falls to the ground, dead, after $n pierces a vital organ.....", ch, NULL, victim, TO_NOTVICT );
      check_killer( ch, victim );
      group_gain( ch, victim );
      raw_kill( ch, victim );
   }
   else
   {
      act( AT_MAGIC, "You fail to assassinate $N.", ch, NULL, victim, TO_CHAR );
      act( AT_MAGIC, "$n tried to assassinate you, but luckily $e failed...", ch, NULL, victim, TO_VICT );
      act( AT_MAGIC, "$n tries to assassinate $N but fails.", ch, NULL, victim, TO_NOTVICT );
      ch->learn_from_failure( gsn_assassinate );
      global_retcode = one_hit( ch, victim, TYPE_UNDEFINED );
   }
}

/* Adapted from Dalemud by Sten */
/* Installed by Samson 10-22-98 - Monk skill */
CMDF( do_feign )
{
   char_data *victim;

   if( ch->isnpc(  ) && ch->has_aflag( AFF_CHARM ) )
   {
      ch->print( "You can't concentrate enough for that.\r\n" );
      return;
   }

   if( !ch->isnpc(  ) && ch->level < skill_table[gsn_feign]->skill_level[ch->Class] )
   {
      ch->print( "You better leave the martial arts to fighters.\r\n" );
      return;
   }

   if( ch->mount )
   {
      ch->print( "What?! and fall off your mount?!\r\n" );
      return;
   }

   if( !( victim = ch->who_fighting(  ) ) )
   {
      ch->print( "You aren't fighting anyone.\r\n" );
      return;
   }

   ch->print( "You try to fake your own demise\r\n" );
   death_cry( ch );

   act( AT_SKILL, "$n is dead! R.I.P.", ch, NULL, victim, TO_NOTVICT );

   ch->WAIT_STATE( skill_table[gsn_feign]->beats );
   if( can_use_skill( ch, number_percent(  ), gsn_feign ) )
   {
      victim->stop_fighting( true );
      ch->position = POS_SLEEPING;
      ch->set_aflag( AFF_HIDE );
      ch->WAIT_STATE( 2 * sysdata->pulseviolence );
   }
   else
   {
      ch->position = POS_SLEEPING;
      ch->WAIT_STATE( 3 * sysdata->pulseviolence );
      ch->learn_from_failure( gsn_feign );
   }
}

CMDF( do_forage )
{
   obj_index *herb;
   obj_data *obj;
   int vnum = OBJ_VNUM_FORAGE, sector;
   short range;

   if( ch->isnpc(  ) )
   {
      ch->print( "Mobs cannot use this skill.\r\n" );
      return;
   }

   if( ( !ch->IS_OUTSIDE(  ) || INDOOR_SECTOR( ch->in_room->sector_type ) ) && !ch->has_pcflag( PCFLAG_ONMAP ) )
   {
      ch->print( "You must be outdoors to use this skill.\r\n" );
      return;
   }

   if( ch->move < 10 )
   {
      ch->print( "You do not have enough movement left to forage.\r\n" );
      return;
   }

   if( ch->has_pcflag( PCFLAG_ONMAP ) )
      sector = map_sector[ch->wmap][ch->mx][ch->my];
   else
      sector = ch->in_room->sector_type;

   switch ( sector )
   {
      case SECT_UNDERWATER:
      case SECT_OCEANFLOOR:
         ch->print( "You can't spend that kind of time underwater!\r\n" );
         return;

      case SECT_RIVER:
         ch->print( "The river's current is too strong to stay in one spot!\r\n" );
         return;

      case SECT_WATER_SWIM:
      case SECT_WATER_NOSWIM:
      case SECT_OCEAN:
         ch->print( "The water is too deep to see anything here!\r\n" );
         return;

      case SECT_AIR:
         ch->print( "Yeah, sure, forage in thin air???\r\n" );
         return;

      case SECT_CITY:
         ch->print( "This spot is far too well traveled to find anything useful.\r\n" );
         return;

      case SECT_ICE:
         ch->print( "Nothing but ice here buddy.\r\n" );
         return;

      case SECT_LAVA:
         ch->print( "What? You want to barbecue yourself?\r\n" );
         return;

      default:
         break;
   }

   range = number_range( 0, 10 );
   vnum += range;

   herb = get_obj_index( vnum );

   if( herb == NULL )
   {
      bug( "%s: Cannot locate item for vnum %d", __func__, vnum );
      ch->print( "Oops. Slight bug here. The immortals have been notified.\r\n" );
      return;
   }

   ch->WAIT_STATE( skill_table[gsn_forage]->beats );
   ch->move -= 10;
   if( ch->move < 1 )
      ch->move = 0;

   if( number_percent(  ) < ch->pcdata->learned[gsn_forage] )
   {
      obj = herb->create_object( 1 );
      obj = obj->to_char( ch );
      ch->print( "After an intense search of the area, your efforts have\r\n" );
      ch->printf( "yielded you %s!\r\n", obj->short_descr );
      return;
   }
   else
   {
      ch->print( "Your search of the area reveals nothing useful.\r\n" );
      ch->learn_from_failure( gsn_forage );
   }
}

/* The mob vnum definition here should always be the first in the series.
 * Keep them grouped or you'll have unpredictable results.
 */
CMDF( do_woodcall )
{
   mob_index *call;
   char_data *mob;
   int vnum = MOB_VNUM_WOODCALL1;
   int sector;
   short range;

   if( ch->isnpc(  ) )
   {
      ch->print( "Mobs cannot use this skill.\r\n" );
      return;
   }

   if( ( !ch->IS_OUTSIDE(  ) || INDOOR_SECTOR( ch->in_room->sector_type ) ) && !ch->has_pcflag( PCFLAG_ONMAP ) )
   {
      ch->print( "You must be outdoors to use this skill.\r\n" );
      return;
   }

   if( ch->move < 30 )
   {
      ch->print( "You do not have enough movement left to call forth the animal.\r\n" );
      return;
   }

   if( ch->has_pcflag( PCFLAG_ONMAP ) )
      sector = map_sector[ch->wmap][ch->mx][ch->my];
   else
      sector = ch->in_room->sector_type;

   switch ( sector )
   {
      case SECT_UNDERWATER:
      case SECT_OCEANFLOOR:
         ch->print( "You can't call out underwater!\r\n" );
         return;

      case SECT_RIVER:
         ch->print( "The river is too swift for that!\r\n" );
         return;

      case SECT_WATER_SWIM:
      case SECT_WATER_NOSWIM:
      case SECT_OCEAN:
         ch->print( "The water is too deep to call anything here!\r\n" );
         return;

      case SECT_AIR:
         ch->print( "Yeah, sure, in thin air???\r\n" );
         return;

      case SECT_CITY:
         ch->print( "This spot is far too well traveled to attract anything.\r\n" );
         return;

      case SECT_ICE:
         ch->print( "Nothing but ice here buddy.\r\n" );
         return;

      case SECT_LAVA:
         ch->print( "What? You want to barbecue yourself?\r\n" );
         return;

      default:
         break;
   }

   range = number_range( 0, 5 );
   vnum += range;

   if( !( call = get_mob_index( vnum ) ) )
   {
      bug( "%s: Cannot locate mob for vnum %d", __func__, vnum );
      ch->print( "Oops. Slight bug here. The immortals have been notified.\r\n" );
      return;
   }

   if( !ch->can_charm(  ) )
   {
      ch->print( "You already have too many followers to support!\r\n" );
      return;
   }

   ch->WAIT_STATE( skill_table[gsn_woodcall]->beats );
   ch->move -= 30;
   if( ch->move < 1 )
      ch->move = 0;

   if( number_percent(  ) < ch->pcdata->learned[gsn_woodcall] )
   {
      mob = call->create_mobile(  );
      if( !mob->to_room( ch->in_room ) )
         log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __func__, __LINE__ );
      ch->printf( "&[skill]Your calls attract %s to your side!\r\n", mob->short_descr );
      bind_follower( mob, ch, gsn_woodcall, ch->level * 10 );
      return;
   }
   else
   {
      ch->print( "Your calls fall on deaf ears, nothing comes forth.\r\n" );
      ch->learn_from_failure( gsn_woodcall );
   }
}

CMDF( do_mining )
{
   obj_index *ore;
   obj_data *obj;
   int vnum = OBJ_VNUM_MINING;
   short range;

   if( ch->isnpc(  ) )
   {
      ch->print( "Mobs cannot use this skill.\r\n" );
      return;
   }

   if( !ch->has_pcflag( PCFLAG_ONMAP ) && ch->in_room->sector_type != SECT_MOUNTAIN && ch->in_room->sector_type != SECT_UNDERGROUND )
   {
      ch->print( "You must be in the mountains, or underground to do mining.\r\n" );
      return;
   }

   if( ch->has_pcflag( PCFLAG_ONMAP ) && map_sector[ch->wmap][ch->mx][ch->my] != SECT_MOUNTAIN )
   {
      ch->print( "You must be in the mountains to do mining.\r\n" );
      return;
   }

   if( ch->move < 40 )
   {
      ch->print( "You do not have enough movement left to mine.\r\n" );
      return;
   }

   range = number_range( 1, 50 );

   if( range < 14 )
      vnum = OBJ_VNUM_MINING;
   if( range > 13 && range < 27 )
      vnum = OBJ_VNUM_MINING + 1;
   if( range > 26 && range < 41 )
      vnum = OBJ_VNUM_MINING + 2;
   if( range > 40 && range < 47 )
      vnum = OBJ_VNUM_MINING + 3;
   if( range > 46 )
      vnum = OBJ_VNUM_MINING + 4;

   ore = get_obj_index( vnum );

   if( ore == NULL )
   {
      bug( "%s: Cannot locate item for vnum %d", __func__, vnum );
      ch->print( "Oops. Slight bug here. The immortals have been notified.\r\n" );
      return;
   }

   ch->WAIT_STATE( skill_table[gsn_mining]->beats );
   ch->move -= 40;
   if( ch->move < 1 )
      ch->move = 0;

   if( number_percent(  ) < ch->pcdata->learned[gsn_mining] )
   {
      obj = ore->create_object( 1 );
      obj = obj->to_char( ch );
      ch->printf( "After some intense mining, you unearth %s!\r\n", obj->short_descr );
      return;
   }
   else
   {
      ch->print( "Your search of the area reveals nothing useful.\r\n" );
      ch->learn_from_failure( gsn_mining );
   }
}

CMDF( do_quiv )
{
   char_data *victim;
   affect_data af;

   if( ch->isnpc(  ) )
   {
      ch->print( "Mobs can't use the fabled quivering palm.\r\n" );
      return;
   }

   if( ch->Class != CLASS_MONK && !ch->is_immortal(  ) )
   {
      ch->print( "Yeah, I bet you thought you were a monk.\r\n" );
      return;
   }

   if( ch->level < skill_table[gsn_quiv]->skill_level[ch->Class] )
   {
      ch->print( "You are not yet powerful enough to try that......\r\n" );
      return;
   }

   if( ch->has_aflag( AFF_QUIV ) )
   {
      ch->print( "You may only use this attack once per day.\r\n" );
      return;
   }

   if( ch->mount )
   {
      ch->print( "Your mount prevents you from getting close enough.\r\n" );
      return;
   }

   if( !( victim = ch->get_char_room( argument ) ) )
   {
      ch->print( "Who do you wish to use the fabled quivering palm on?\r\n" );
      return;
   }

   if( ch == victim )
   {
      ch->print( "Use quivering palm on yourself? That's ludicrous.\r\n" );
      return;
   }

   if( is_safe( ch, victim ) )
      return;

   if( ch->level < victim->level && number_percent(  ) > 15 )
   {
      act( AT_SKILL, "$N's experience prevents your attempt.", ch, NULL, victim, TO_CHAR );
      return;
   }

   if( ch->max_hit * 2 < victim->max_hit && number_percent(  ) > 10 )
   {
      act( AT_PLAIN, "$N's might prevents your attempt.", ch, NULL, victim, TO_CHAR );
      return;
   }

   ch->print( "You begin to work on the vibrations.\r\n" );

   af.type = gsn_quiv;
   af.duration = 2616;  /* One day long */
   af.modifier = 0;
   af.location = APPLY_NONE;
   af.bit = AFF_QUIV;

   if( number_percent(  ) < ch->pcdata->learned[gsn_quiv] )
   {
      act( AT_SKILL, "Your hand vibrates intensly as it strikes $N dead!", ch, NULL, victim, TO_CHAR );
      act( AT_SKILL, "The last thing you see before dying is $n's blurred palm!", ch, NULL, victim, TO_VICT );
      act( AT_SKILL, "$n's palm blurs from sight, and suddenly $N drops dead!", ch, NULL, victim, TO_NOTVICT );
      group_gain( ch, victim );
      raw_kill( ch, victim );
      ch->affect_to_char( &af );
   }
   else
   {
      ch->print( "Your vibrations fade ineffectively.\r\n" );
      ch->learn_from_failure( gsn_quiv );
   }
}

/* Charge code written by Sadiq - April 28, 1998    *
 * e-mail to MudPrince@aol.com                      */
CMDF( do_charge )
{
   obj_data *wand;
   int sn, mana, charge;

   if( ch->isnpc(  ) )
      return;

   if( ch->level < skill_table[gsn_charge]->skill_level[ch->Class] )
   {
      ch->print( "A skill such as this is presently beyond your comprehension.\r\n" );
      return;
   }

   if( argument.empty(  ) )
   {
      ch->print( "Bind what?\r\n" );
      return;
   }

   if( ms_find_obj( ch ) )
      return;

   if( ( sn = find_spell( ch, argument, true ) ) < 0 )
   {
      ch->print( "There is no such spell.\r\n" );
      return;
   }

   if( !str_cmp( argument, "word of recall" ) )
      sn = find_spell( ch, "recall", false );

   if( skill_table[sn]->type != SKILL_SPELL )
   {
      ch->print( "That's not a spell!\r\n" );
      return;
   }

   if( ch->level < skill_table[sn]->skill_level[ch->Class] )
   {
      ch->print( "You have not yet learned that spell.\r\n" );
      return;
   }

   if( SPELL_FLAG( skill_table[sn], SF_NOCHARGE ) )
   {
      ch->print( "You cannot bind that spell.\r\n" );
      return;
   }

   mana = UMAX( skill_table[sn]->min_mana, 100 / ( 2 + ch->level - skill_table[sn]->skill_level[ch->Class] ) );

   mana *= 3;

   if( ch->mana < mana )
   {
      ch->print( "You don't have enough mana.\r\n" );
      return;
   }

   if( !( wand = ch->get_eq( WEAR_HOLD ) ) )
   {
      ch->print( "You must be holding a suitable wand to bind it.\r\n" );
      return;
   }

   if( wand->pIndexData->vnum != OBJ_VNUM_WAND_CHARGING )
   {
      ch->print( "You must be holding a suitable wand to bind it.\r\n" );
      return;
   }

   if( wand->value[3] != -1 && wand->pIndexData->vnum == OBJ_VNUM_WAND_CHARGING )
   {
      ch->print( "That wand has already been bound.\r\n" );
      return;
   }

   if( !process_spell_components( ch, sn ) )
   {
      ch->print( "The spell fizzles and dies due to a lack of the proper components.\r\n" );
      ch->mana -= ( mana / 2 );
      return;
   }

   ch->WAIT_STATE( skill_table[gsn_charge]->beats );
   if( !ch->isnpc(  ) && number_percent(  ) > ch->pcdata->learned[gsn_charge] )
   {
      ch->print( "&[magic]Your spell fails and the distortions in the magic destroy the wand!\r\n" );
      ch->learn_from_failure( gsn_charge );
      wand->extract(  );
      ch->mana -= ( mana / 2 );
      return;
   }

   charge = ch->get_curr_int(  ) + dice( 1, 5 );

   wand->level = ch->level;
   wand->value[0] = ch->level / 2;
   wand->value[1] = charge;
   wand->value[2] = charge;
   wand->value[3] = sn;
   stralloc_printf( &wand->short_descr, "wand of %s", skill_table[sn]->name );
   stralloc_printf( &wand->objdesc, "A polished wand of '%s' has been left here.", skill_table[sn]->name );
   stralloc_printf( &wand->name, "wand %s", skill_table[sn]->name );
   act( AT_MAGIC, "$n magically charges $p.", ch, wand, NULL, TO_ROOM );
   act( AT_MAGIC, "You magically charge $p.", ch, wand, NULL, TO_CHAR );

   ch->mana -= mana;
}

CMDF( do_tan )
{
   obj_data *corpse = NULL, *hide = NULL;
   string itemname, itemtype, hidetype;
   int percent = 0, acapply = 0, acbonus = 0, lev = 0;

   if( ch->isnpc(  ) && !ch->has_actflag( ACT_POLYSELF ) )
      return;

   if( ch->mount )
   {
      ch->print( "Not from this mount you cannot!\r\n" );
      return;
   }

   if( !ch->isnpc(  ) && ch->level < skill_table[gsn_tan]->skill_level[ch->Class] )
   {
      ch->print( "What do you think you are, A tanner?\r\n" );
      return;
   }

   argument = one_argument( argument, itemname );
   argument = one_argument( argument, itemtype );

   if( itemname.empty(  ) )
   {
      ch->print( "You may make the following items out of a corpse:\r\n" );
      ch->print( "Jacket, shield, boots, gloves, leggings, sleeves, helmet, bag, belt, cloak, whip,\r\nquiver, waterskin, and collar\r\n" );
      return;
   }

   if( itemtype.empty(  ) )
   {
      ch->print( "I see that, but what do you wanna make?\r\n" );
      return;
   }

   if( !( corpse = ch->get_obj_here( itemname ) ) )
   {
      ch->print( "Where did that carcass go?\r\n" );
      return;
   }

   if( corpse->item_type != ITEM_CORPSE_PC && corpse->item_type != ITEM_CORPSE_NPC )
   {
      ch->print( "That is not a corpse, you cannot tan it.\r\n" );
      return;
   }

   if( corpse->value[5] == 1 )
   {
      ch->print( "There isn't any flesh left on that corpse to tan anything with!\r\n" );
      return;
   }

   corpse->separate(  );   /* No need to destroy ALL corpses of this type */
   percent = number_percent(  );

   if( percent > ch->LEARNED( gsn_tan ) )
   {
      act( AT_PLAIN, "You hack at $p but manage to only destroy the hide.", ch, corpse, NULL, TO_CHAR );
      act( AT_PLAIN, "$n tries to skins $p for its hide, but destroys it.", ch, corpse, NULL, TO_ROOM );
      ch->learn_from_failure( gsn_tan );

      /*
       * Tanning won't destroy what the corpse was carrying - Samson 11-20-99 
       */
      if( corpse->carried_by )
         corpse->empty( NULL, corpse->carried_by->in_room );
      else if( corpse->in_room )
         corpse->empty( NULL, corpse->in_room );

      corpse->extract(  );
      return;
   }

   lev = corpse->level; /* Why not? The corpse creation code already provided us with this. */

   acbonus += lev / 10;
   acapply += lev / 10;

   if( ch->Class == CLASS_RANGER )
      acbonus += 1;

   acbonus = URANGE( 0, acbonus, 20 );
   acapply = URANGE( 0, acapply, 20 );

   if( !str_cmp( itemtype, "shield" ) )
   {
      if( !( hide = get_obj_index( OBJ_VNUM_TAN_SHIELD )->create_object( 1 ) ) )
      {
         ch->print( "Ooops. Bug. The immortals have been notified.\r\n" );
         log_printf( "create_object: %s:%s, line %d.", __FILE__, __func__, __LINE__ );
         return;
      }
      ++acapply;
      ++acbonus;
      hidetype = "A shield";
   }
   else if( !str_cmp( itemtype, "jacket" ) )
   {
      if( !( hide = get_obj_index( OBJ_VNUM_TAN_JACKET )->create_object( 1 ) ) )
      {
         ch->print( "Ooops. Bug. The immortals have been notified.\r\n" );
         log_printf( "create_object: %s:%s, line %d.", __FILE__, __func__, __LINE__ );
         return;
      }
      acapply += 5;
      acbonus += 2;
      hidetype = "A jacket";
   }
   else if( !str_cmp( itemtype, "boots" ) )
   {
      if( !( hide = get_obj_index( OBJ_VNUM_TAN_BOOTS )->create_object( 1 ) ) )
      {
         ch->print( "Ooops. Bug. The immortals have been notified.\r\n" );
         log_printf( "create_object: %s:%s, line %d.", __FILE__, __func__, __LINE__ );
         return;
      }
      --acapply;
      if( acapply < 0 )
         acapply = 0;
      --acbonus;
      if( acbonus < 0 )
         acbonus = 0;
      hidetype = "Boots";
   }
   else if( !str_cmp( itemtype, "gloves" ) )
   {
      if( !( hide = get_obj_index( OBJ_VNUM_TAN_GLOVES )->create_object( 1 ) ) )
      {
         ch->print( "Ooops. Bug. The immortals have been notified.\r\n" );
         log_printf( "create_object: %s:%s, line %d.", __FILE__, __func__, __LINE__ );
         return;
      }
      --acapply;
      if( acapply < 0 )
         acapply = 0;
      --acbonus;
      if( acbonus < 0 )
         acbonus = 0;
      hidetype = "Gloves";
   }
   else if( !str_cmp( itemtype, "leggings" ) )
   {
      if( !( hide = get_obj_index( OBJ_VNUM_TAN_LEGGINGS )->create_object( 1 ) ) )
      {
         ch->print( "Ooops. Bug. The immortals have been notified.\r\n" );
         log_printf( "create_object: %s:%s, line %d.", __FILE__, __func__, __LINE__ );
         return;
      }
      ++acapply;
      ++acbonus;
      hidetype = "Leggings";
   }
   else if( !str_cmp( itemtype, "sleeves" ) )
   {
      if( !( hide = get_obj_index( OBJ_VNUM_TAN_SLEEVES )->create_object( 1 ) ) )
      {
         ch->print( "Ooops. Bug. The immortals have been notified.\r\n" );
         log_printf( "create_object: %s:%s, line %d.", __FILE__, __func__, __LINE__ );
         return;
      }
      ++acapply;
      ++acbonus;
      hidetype = "Sleeves";
   }
   else if( !str_cmp( itemtype, "helmet" ) )
   {
      if( !( hide = get_obj_index( OBJ_VNUM_TAN_HELMET )->create_object( 1 ) ) )
      {
         ch->print( "Ooops. Bug. The immortals have been notified.\r\n" );
         log_printf( "create_object: %s:%s, line %d.", __FILE__, __func__, __LINE__ );
         return;
      }
      --acapply;
      if( acapply < 0 )
         acapply = 0;
      --acbonus;
      if( acbonus < 0 )
         acbonus = 0;
      hidetype = "A helmet";
   }
   else if( !str_cmp( itemtype, "bag" ) )
   {
      if( !( hide = get_obj_index( OBJ_VNUM_TAN_BAG )->create_object( 1 ) ) )
      {
         ch->print( "Ooops. Bug. The immortals have been notified.\r\n" );
         log_printf( "create_object: %s:%s, line %d.", __FILE__, __func__, __LINE__ );
         return;
      }
      hidetype = "A bag";
   }
   else if( !str_cmp( itemtype, "belt" ) )
   {
      if( !( hide = get_obj_index( OBJ_VNUM_TAN_BELT )->create_object( 1 ) ) )
      {
         ch->print( "Ooops. Bug. The immortals have been notified.\r\n" );
         log_printf( "create_object: %s:%s, line %d.", __FILE__, __func__, __LINE__ );
         return;
      }
      --acapply;
      if( acapply < 0 )
         acapply = 0;
      --acbonus;
      if( acbonus < 0 )
         acbonus = 0;
      hidetype = "A belt";
   }
   else if( !str_cmp( itemtype, "cloak" ) )
   {
      if( !( hide = get_obj_index( OBJ_VNUM_TAN_CLOAK )->create_object( 1 ) ) )
      {
         ch->print( "Ooops. Bug. The immortals have been notified.\r\n" );
         log_printf( "create_object: %s:%s, line %d.", __FILE__, __func__, __LINE__ );
         return;
      }
      --acapply;
      if( acapply < 0 )
         acapply = 0;
      --acbonus;
      if( acbonus < 0 )
         acbonus = 0;
      hidetype = "A cloak";
   }
   else if( !str_cmp( itemtype, "quiver" ) )
   {
      if( !( hide = get_obj_index( OBJ_VNUM_TAN_QUIVER )->create_object( 1 ) ) )
      {
         ch->print( "Ooops. Bug. The immortals have been notified.\r\n" );
         log_printf( "create_object: %s:%s, line %d.", __FILE__, __func__, __LINE__ );
         return;
      }
      hidetype = "A quiver";
   }
   else if( !str_cmp( itemtype, "waterskin" ) )
   {
      if( !( hide = get_obj_index( OBJ_VNUM_TAN_WATERSKIN )->create_object( 1 ) ) )
      {
         ch->print( "Ooops. Bug. The immortals have been notified.\r\n" );
         log_printf( "create_object: %s:%s, line %d.", __FILE__, __func__, __LINE__ );
         return;
      }
      hidetype = "A waterskin";
   }
   else if( !str_cmp( itemtype, "collar" ) )
   {
      if( !( hide = get_obj_index( OBJ_VNUM_TAN_COLLAR )->create_object( 1 ) ) )
      {
         ch->print( "Ooops. Bug. The immortals have been notified.\r\n" );
         log_printf( "create_object: %s:%s, line %d.", __FILE__, __func__, __LINE__ );
         return;
      }
      --acapply;
      if( acapply < 0 )
         acapply = 0;
      --acbonus;
      if( acbonus < 0 )
         acbonus = 0;
      hidetype = "A collar";
   }
   else if( !str_cmp( itemtype, "whip" ) )
   {
      if( !( hide = get_obj_index( OBJ_VNUM_TAN_WHIP )->create_object( 1 ) ) )
      {
         ch->print( "Ooops. Bug. The immortals have been notified.\r\n" );
         log_printf( "create_object: %s:%s, line %d.", __FILE__, __func__, __LINE__ );
         return;
      }
      ++acapply;
      hidetype = "A whip";
   }
   else
   {
      ch->print( "Illegal type of equipment!\r\n" );
      return;
   }

   if( !hide )
   {
      bug( "%s: Tan objects missing.", __func__ );
      ch->print( "You messed up the hide and it's useless.\r\n" );
      return;
   }
   hide->to_char( ch );

   stralloc_printf( &hide->name, "%s", hidetype.c_str(  ) );
   stralloc_printf( &hide->short_descr, "%s made from the hide of %s", hidetype.c_str(  ), corpse->short_descr + 14 );
   stralloc_printf( &hide->objdesc, "%s made from the hide of %s lies here.", hidetype.c_str(  ), corpse->short_descr + 14 );

   if( hide->item_type == ITEM_ARMOR )
   {
      hide->value[0] = acapply;
      hide->value[1] = acapply;
   }
   else if( hide->item_type == ITEM_WEAPON )
   {
      hide->value[0] = acapply;
      hide->value[1] = ( acapply / 10 ) + 1;
      hide->value[2] = acapply * 2;
   }
   else if( hide->item_type == ITEM_CONTAINER )
      hide->value[0] = acapply + 25;
   else if( hide->item_type == ITEM_DRINK_CON )
      hide->value[0] = acapply + 10;

   act_printf( AT_PLAIN, ch, corpse, NULL, TO_CHAR, "You carve %s from $p.", hide->name );
   act_printf( AT_PLAIN, ch, corpse, NULL, TO_ROOM, "$n carves %s from $p.", hide->name );

   /*
    * Tanning won't destroy what the corpse was carrying - Samson 11-20-99 
    */
   if( corpse->carried_by )
      corpse->empty( NULL, corpse->carried_by->in_room );
   else if( corpse->in_room )
      corpse->empty( NULL, corpse->in_room );

   corpse->extract(  );
}

CMDF( do_throw )
{
   if( argument.empty(  ) )
   {
      ch->print( "Throw at whom or what?\r\n" );
      return;
   }

   obj_data *obj;
   if( !( obj = ch->get_eq( WEAR_HOLD ) ) )
   {
      ch->print( "You hold nothing in your hand.\r\n" );
      return;
   }

   if( obj->item_type != ITEM_THROWN )
   {
      ch->print( "You can throw only a throwing item.\r\n" );
      return;
   }

   ch->WAIT_STATE( 6 );

   /*
    * handle the ranged attack
    */
   string name = obj->name;
   ranged_attack( ch, argument, NULL, obj, TYPE_HIT + obj->value[3], ( short )( ( ch->perm_str / 4 ) * .5 ) );

   obj_data *item = NULL;
   list < obj_data * >::iterator iobj;
   for( iobj = ch->carrying.begin(  ); iobj != ch->carrying.end(  ); ++iobj )
   {
      obj = *iobj;

      if( ( obj->item_type == ITEM_SCABBARD ) && ch->can_see_obj( obj, true ) )
      {
         item = get_obj_list( ch, name, obj->contents );
         if( item )
            break;
      }
      if( obj->item_type == ITEM_THROWN && ch->can_see_obj( obj, true ) )
      {
         item = obj;
         break;
      }
   }

   if( !item )
      ch->print( "You are out of thrown weapons\r\n" );
   else
   {
      ch->printf( "You draw %s\r\n", item->short_descr );
      if( item->in_obj )
      {
         item->from_obj(  );
         item->to_char( ch );
      }
      if( ch->get_eq( WEAR_HOLD ) != item )
         ch->equip( item, WEAR_HOLD );
   }
}
