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
 *                     Stock Zone reader and convertor                      *
 ****************************************************************************/

/* Converts stock Smaug version 0 and version 1 areas into AFKMud format - Samson 12-21-01 */
/* Converts SmaugWiz version 1000 files into AFKMud format - Samson 4-24-03 */

#include <sys/stat.h>
#include "mud.h"
#include "area.h"
#include "mobindex.h"
#include "objindex.h"
#include "roomindex.h"
#include "shops.h"

/* Extended bitvector matierial is now kept only for legacy purposes to convert old areas. */
typedef struct extended_bitvector EXT_BV;

/*
 * Defines for extended bitvectors
 */
#ifndef INTBITS
const int INTBITS = 32;
#endif
const int XBM = 31;  /* extended bitmask   ( INTBITS - 1 )  */
const int RSV = 5;   /* right-shift value  ( sqrt(XBM+1) )  */
const int XBI = 4;   /* integers in an extended bitvector   */
const int MAX_BITS = XBI * INTBITS;

#define xIS_SET(var, bit) ((var).bits[(bit) >> RSV] & 1 << ((bit) & XBM))

/*
 * Structure for extended bitvectors -- Thoric
 */
struct extended_bitvector
{
   unsigned int bits[XBI]; /* Needs to be unsigned to compile in Redhat 6 - Samson */
};

EXT_BV fread_bitvector( FILE * );
char *ext_flag_string( EXT_BV *, char *const flagarray[] );

extern int top_affect;
extern int top_exit;
extern int top_reset;
extern int top_shop;
extern int top_repair;
extern FILE *fpArea;

bool area_failed;
int dotdcheck;

const char *stock_act[] = {
   "npc", "sentinel", "scavenger", "r1", "r2", "aggressive", "stayarea",
   "wimpy", "pet", "train", "practice", "immortal", "deadly", "polyself",
   "meta_aggr", "guardian", "running", "nowander", "mountable", "mounted",
   "scholar", "secretive", "hardhat", "mobinvis", "noassist", "autonomous",
   "pacifist", "noattack", "annoying", "statshield", "proto", "r14"
};

const char *stock_ex_flags[] = {
   "isdoor", "closed", "locked", "secret", "swim", "pickproof", "fly", "climb",
   "dig", "eatkey", "nopassdoor", "hidden", "passage", "portal", "r1", "r2",
   "can_climb", "can_enter", "can_leave", "auto", "noflee", "searchable",
   "bashed", "bashproof", "nomob", "window", "can_look", "isbolt", "bolted"
};

const char *stock_aff[] = {
   "blind", "invisible", "detect_evil", "detect_invis", "detect_magic",
   "detect_hidden", "hold", "sanctuary", "faerie_fire", "infrared", "curse",
   "_flaming", "poison", "protect", "_paralysis", "sneak", "hide", "sleep",
   "charm", "flying", "pass_door", "floating", "truesight", "detect_traps",
   "scrying", "fireshield", "shockshield", "r1", "iceshield", "possess",
   "berserk", "aqua_breath", "recurringspell", "contagious", "acidmist",
   "venomshield"
};

const char *stock_race[] = {
   "human", "high-elf", "dwarf", "halfling", "pixie", "vampire", "half-ogre",
   "half-orc", "half-troll", "half-elf", "gith", "drow", "sea-elf",
   "iguanadon", "gnome", "r5", "r6", "r7", "r8", "troll",
   "ant", "ape", "baboon", "bat", "bear", "bee",
   "beetle", "boar", "bugbear", "cat", "dog", "dragon", "ferret", "fly",
   "gargoyle", "gelatin", "ghoul", "gnoll", "gnome", "goblin", "golem",
   "gorgon", "harpy", "hobgoblin", "kobold", "lizardman", "locust",
   "lycanthrope", "minotaur", "mold", "mule", "neanderthal", "ooze", "orc",
   "rat", "rustmonster", "shadow", "shapeshifter", "shrew", "shrieker",
   "skeleton", "slime", "snake", "spider", "stirge", "thoul", "troglodyte",
   "undead", "wight", "wolf", "worm", "zombie", "bovine", "canine", "feline",
   "porcine", "mammal", "rodent", "avis", "reptile", "amphibian", "fish",
   "crustacean", "insect", "spirit", "magical", "horse", "animal", "humanoid",
   "monster", "god"
};

const char *stock_class[] = {
   "mage", "cleric", "rogue", "warrior", "vampire", "druid", "ranger",
   "augurer", "paladin", "nephandi", "savage", "pirate", "pc12", "pc13",
   "pc14", "pc15", "pc16", "pc17", "pc18", "pc19",
   "baker", "butcher", "blacksmith", "mayor", "king", "queen"
};

const char *stock_pos[] = {
   "dead", "mortal", "incapacitated", "stunned", "sleeping", "berserk", "resting",
   "aggressive", "sitting", "fighting", "defensive", "evasive", "standing", "mounted",
   "shove", "drag"
};

const char *stock_oflags[] = {
   "glow", "hum", "dark", "loyal", "evil", "invis", "magic", "nodrop", "bless",
   "antigood", "antievil", "antineutral", "noremove", "inventory",
   "antimage", "antirogue", "antiwarrior", "anticleric", "organic", "metal",
   "donation", "clanobject", "clancorpse", "antivampire(UNUSED)", "antidruid",
   "hidden", "poisoned", "covering", "deathrot", "buried", "proto",
   "nolocate", "groundrot", "lootable"
};

const char *stock_wflags[] = {
   "take", "finger", "neck", "body", "head", "legs", "feet", "hands", "arms",
   "shield", "about", "waist", "wrist", "wield", "hold", "dual", "ears", "eyes",
   "missile", "back", "face", "ankle", "r4", "r5", "r6",
   "r7", "r8", "r9", "r10", "r11", "r12", "r13"
};

const char *stock_rflags[] = {
   "dark", "death", "nomob", "indoors", "lawful", "neutral", "chaotic",
   "nomagic", "tunnel", "private", "safe", "solitary", "petshop", "norecall",
   "donation", "nodropall", "silence", "logspeech", "nodrop", "clanstoreroom",
   "nosummon", "noastral", "teleport", "teleshowdesc", "nofloor",
   "nosupplicate", "arena", "nomissile", "r4", "r5", "proto", "dnd"
};

const char *stock_area_flags[] = {
   "nopkill", "freekill", "noteleport", "spelllimit", "r4", "r5", "r6", "r7", "r8",
   "r9", "r10", "r11", "r12", "r13", "r14", "r15", "r16", "r17",
   "r18", "r19", "r20", "r21", "r22", "r23", "r24",
   "r25", "r26", "r27", "r28", "r29", "r30", "r31"
};

const char *stock_lang_names[] = {
   "common", "elvish", "dwarven", "pixie", "ogre", "orcish", "trollese", "rodent", "insectoid",
   "mammal", "reptile", "dragon", "spiritual", "magical", "goblin", "god", "ancient",
   "halfling", "clan", "gith", "r20", "r21", "r22", "r23", "r24",
   "r25", "r26", "r27", "r28", "r29", "r30", "r31"
};

const char *stock_attack_flags[] = {
   "bite", "claws", "tail", "sting", "punch", "kick", "trip", "bash", "stun",
   "gouge", "backstab", "feed", "drain", "firebreath", "frostbreath",
   "acidbreath", "lightnbreath", "gasbreath", "poison", "nastypoison", "gaze",
   "blindness", "causeserious", "earthquake", "causecritical", "curse",
   "flamestrike", "harm", "fireball", "colorspray", "weaken", "r1"
};

const char *stock_defense_flags[] = {
   "parry", "dodge", "heal", "curelight", "cureserious", "curecritical",
   "dispelmagic", "dispelevil", "sanctuary", "fireshield", "shockshield",
   "shield", "bless", "stoneskin", "teleport", "monsum1", "monsum2", "monsum3",
   "monsum4", "disarm", "iceshield", "grip", "truesight", "r4", "r5", "r6", "r7",
   "r8", "r9", "r10", "r11", "r12", "acidmist", "venomshield"
};

const char *stock_o_types[] = {
   "none", "light", "scroll", "wand", "staff", "weapon", "_fireweapon", "_missile",
   "treasure", "armor", "potion", "_worn", "furniture", "trash", "_oldtrap",
   "container", "_note", "drinkcon", "key", "food", "money", "pen", "boat",
   "corpse", "corpse_pc", "fountain", "pill", "blood", "bloodstain",
   "scraps", "pipe", "herbcon", "herb", "incense", "fire", "book", "switch",
   "lever", "pullchain", "button", "dial", "rune", "runepouch", "match", "trap",
   "map", "portal", "paper", "tinder", "lockpick", "spike", "disease", "oil",
   "fuel", "_empty1", "_empty2", "missileweapon", "projectile", "quiver", "shovel",
   "salve", "cook", "keyring", "odor", "chance"
};

const char *stock_sect[] = {
   "indoors", "city", "field", "forest", "hills", "mountain", "water_swim",
   "water_noswim", "underwater", "air", "desert", "dunno", "oceanfloor",
   "underground", "lava", "swamp", "r1", "r2", "r3", "r4", "r5", "r6", "r7",
   "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15", "r16"
};

/* Extended bitvector matierial is now kept only for legacy purposes to convert old areas. */

/*
 * Read an extended bitvector from a file. - Thoric
 */
EXT_BV fread_bitvector( FILE * fp )
{
   EXT_BV ret;
   int c, x = 0;
   int num = 0;

   memset( &ret, '\0', sizeof( ret ) );
   for( ;; )
   {
      num = fread_number( fp );
      if( x < XBI )
         ret.bits[x] = num;
      ++x;
      if( ( c = getc( fp ) ) != '&' )
      {
         ungetc( c, fp );
         break;
      }
   }
   return ret;
}

char *ext_flag_string( EXT_BV * bitvector, const char *flagarray[] )
{
   static char buf[MSL];
   int x;

   buf[0] = '\0';
   for( x = 0; x < MAX_BITS; ++x )
      if( xIS_SET( *bitvector, x ) )
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

bool check_area_conflict( area_data * area, int low_range, int hi_range )
{
   if( low_range < area->low_vnum && area->low_vnum < hi_range )
      return true;

   if( low_range < area->hi_vnum && area->hi_vnum < hi_range )
      return true;

   if( ( low_range >= area->low_vnum ) && ( low_range <= area->hi_vnum ) )
      return true;

   if( ( hi_range <= area->hi_vnum ) && ( hi_range >= area->low_vnum ) )
      return true;

   return false;
}

// Runs the entire list, easier to call in places that have to check them all
bool check_area_conflicts( int lo, int hi )
{
   list < area_data * >::iterator iarea;

   for( iarea = arealist.begin(  ); iarea != arealist.end(  ); ++iarea )
   {
      area_data *area = *iarea;

      if( check_area_conflict( area, lo, hi ) )
         return true;
   }
   return false;
}

void load_stmobiles( area_data * tarea, FILE * fp, bool manual )
{
   mob_index *pMobIndex;
   const char *ln;
   int x1, x2, x3, x4, x5, x6, x7;

   if( !tarea )
   {
      bug( "%s: no #AREA seen yet.", __func__ );
      if( fBootDb )
      {
         shutdown_mud( "No #AREA" );
         exit( 1 );
      }
      else
         return;
   }

   for( ;; )
   {
      char letter;
      bool oldmob, tmpBootDb;

      letter = fread_letter( fp );
      if( letter != '#' )
      {
         bug( "%s: # not found.", __func__ );
         if( fBootDb )
         {
            shutdown_mud( "# not found" );
            exit( 1 );
         }
         else
            return;
      }

      int vnum = fread_number( fp );
      if( vnum == 0 )
         break;

      tmpBootDb = fBootDb;
      fBootDb = false;

      list < area_data * >::iterator iarea;
      for( iarea = arealist.begin(  ); iarea != arealist.end(  ); ++iarea )
      {
         area_data *area = *iarea;

         if( !str_cmp( area->filename, tarea->filename ) )
            continue;

         bool area_conflict = check_area_conflict( area, vnum, vnum );

         if( area_conflict )
         {
            log_printf( "ERROR: %s has vnum conflict with %s!", tarea->filename, ( area->filename ? area->filename : "(invalid)" ) );
            log_printf( "%s occupies vnums   : %-6d - %-6d", ( area->filename ? area->filename : "(invalid)" ), area->low_vnum, area->hi_vnum );
            log_printf( "%s wants to use vnum: %-6d", tarea->filename, vnum );
            if( !manual )
            {
               log_string( "This is a fatal error. Program terminated." );
               exit( 1 );
            }
            else
            {
               area_failed = true;
               log_string( "This is a fatal error. Conversion terminated." );
               deleteptr( tarea );
               return;
            }
         }
      }

      if( get_mob_index( vnum ) )
      {
         if( tmpBootDb )
         {
            bug( "%s: vnum %d duplicated.", __func__, vnum );
            if( manual )
            {
               area_failed = true;
               log_string( "This is a fatal error. Conversion terminated." );
               deleteptr( tarea );
               return;
            }
            else
            {
               shutdown_mud( "duplicate vnum" );
               exit( 1 );
            }
         }
         else
         {
            pMobIndex = get_mob_index( vnum );
            log_printf_plus( LOG_BUILD, sysdata->build_level, "Cleaning mobile: %d", vnum );
            pMobIndex->clean_mob(  );
            oldmob = true;
         }
      }
      else
      {
         oldmob = false;
         pMobIndex = new mob_index;
      }
      fBootDb = tmpBootDb;

      pMobIndex->vnum = vnum;
      if( fBootDb )
      {
         if( !tarea->low_vnum )
            tarea->low_vnum = vnum;
         if( vnum > tarea->hi_vnum )
            tarea->hi_vnum = vnum;
      }
      pMobIndex->area = tarea;
      pMobIndex->player_name = fread_string( fp );
      pMobIndex->short_descr = fread_string( fp );
      pMobIndex->long_descr = fread_string( fp );

      const char *desc = fread_flagstring( fp );
      if( desc && desc[0] != '\0' && str_cmp( desc, "(null)" ) )
      {
         pMobIndex->chardesc = STRALLOC( desc );
         pMobIndex->chardesc[0] = UPPER( pMobIndex->chardesc[0] );
      }

      if( pMobIndex->long_descr != nullptr )
         pMobIndex->long_descr[0] = UPPER( pMobIndex->long_descr[0] );
      {
         char *sact, *saff;
         char flag[MIL];
         EXT_BV temp;
         int value;

         temp = fread_bitvector( fp );
         sact = ext_flag_string( &temp, stock_act );

         while( sact[0] != '\0' )
         {
            sact = one_argument( sact, flag );
            value = get_actflag( flag );
            if( value < 0 || value >= MAX_ACT_FLAG )
               bug( "Unsupported act_flag dropped: %s", flag );
            else
               pMobIndex->actflags.set( value );
         }
         pMobIndex->actflags.set( ACT_IS_NPC );

         temp = fread_bitvector( fp );
         saff = ext_flag_string( &temp, stock_aff );

         while( saff[0] != '\0' )
         {
            saff = one_argument( saff, flag );
            value = get_aflag( flag );
            if( value < 0 || value >= MAX_AFFECTED_BY )
               bug( "Unsupported aff_flag dropped: %s", flag );
            else
               pMobIndex->affected_by.set( value );
         }
      }

      pMobIndex->pShop = nullptr;
      pMobIndex->rShop = nullptr;
      pMobIndex->alignment = fread_number( fp );
      letter = fread_letter( fp );
      pMobIndex->level = fread_number( fp );

      fread_number( fp );
      pMobIndex->mobthac0 = 21;  /* Autovonvert to the autocomputation value */
      pMobIndex->ac = fread_number( fp );
      pMobIndex->hitnodice = fread_number( fp );
      /*
       * 'd'      
       */ fread_letter( fp );
      pMobIndex->hitsizedice = fread_number( fp );
      /*
       * '+'      
       */ fread_letter( fp );
      pMobIndex->hitplus = fread_number( fp );
      pMobIndex->damnodice = fread_number( fp );
      /*
       * 'd'      
       */ fread_letter( fp );
      pMobIndex->damsizedice = fread_number( fp );
      /*
       * '+'      
       */ fread_letter( fp );
      pMobIndex->damplus = fread_number( fp );

      ln = fread_line( fp );
      x1 = x2 = 0;
      sscanf( ln, "%d %d", &x1, &x2 );
      pMobIndex->gold = x1;
      pMobIndex->exp = -1; /* Convert mob to use autocalc exp */

      {
         const char *spos;
         int pos = fread_number( fp );

         if( pos < 100 )
         {
            switch ( pos )
            {
               default:
               case 0:
               case 1:
               case 2:
               case 3:
               case 4:
                  break;
               case 5:
                  pos = 6;
                  break;
               case 6:
                  pos = 8;
                  break;
               case 7:
                  pos = 9;
                  break;
               case 8:
                  pos = 12;
                  break;
               case 9:
                  pos = 13;
                  break;
               case 10:
                  pos = 14;
                  break;
               case 11:
                  pos = 15;
                  break;
            }
         }
         else
            pos -= 100;

         spos = stock_pos[pos];
         pMobIndex->position = get_npc_position( spos );
      }

      {
         const char *sdefpos;
         int defpos = fread_number( fp );

         if( defpos < 100 )
         {
            switch ( defpos )
            {
               default:
               case 0:
               case 1:
               case 2:
               case 3:
               case 4:
                  break;
               case 5:
                  defpos = 6;
                  break;
               case 6:
                  defpos = 8;
                  break;
               case 7:
                  defpos = 9;
                  break;
               case 8:
                  defpos = 12;
                  break;
               case 9:
                  defpos = 13;
                  break;
               case 10:
                  defpos = 14;
                  break;
               case 11:
                  defpos = 15;
                  break;
            }
         }
         else
         {
            defpos -= 100;
         }
         sdefpos = stock_pos[defpos];
         pMobIndex->defposition = get_npc_position( sdefpos );
      }
      /*
       * Back to meaningful values.
       */
      pMobIndex->sex = fread_number( fp );

      if( letter != 'S' && letter != 'C' && letter != 'D' && letter != 'Z' )
      {
         bug( "%s: vnum %d: letter '%c' not S, C, Z, or D.", __func__, vnum, letter );
         shutdown_mud( "bad mob data" );
         exit( 1 );
      }

      if( letter == 'C' || letter == 'D' || letter == 'Z' ) /* Realms complex mob  -Thoric */
      {
         pMobIndex->perm_str = fread_number( fp );
         pMobIndex->perm_int = fread_number( fp );
         pMobIndex->perm_wis = fread_number( fp );
         pMobIndex->perm_dex = fread_number( fp );
         pMobIndex->perm_con = fread_number( fp );
         pMobIndex->perm_cha = fread_number( fp );
         pMobIndex->perm_lck = fread_number( fp );
         pMobIndex->saving_poison_death = fread_number( fp );
         pMobIndex->saving_wand = fread_number( fp );
         pMobIndex->saving_para_petri = fread_number( fp );
         pMobIndex->saving_breath = fread_number( fp );
         pMobIndex->saving_spell_staff = fread_number( fp );

         if( tarea->version < 1000 )   /* Standard Smaug version 0 or 1 */
         {
            ln = fread_line( fp );
            x1 = x2 = x3 = x4 = x5 = x6 = x7 = 0;
            sscanf( ln, "%d %d %d %d %d %d %d", &x1, &x2, &x3, &x4, &x5, &x6, &x7 );

            {
               const char *srace, *sclass;

               if( x1 >= 0 && x1 < 90 )
                  srace = stock_race[x1];
               else
                  srace = "human";

               pMobIndex->race = get_npc_race( srace );

               if( pMobIndex->race < 0 || pMobIndex->race >= MAX_NPC_RACE )
               {
                  bug( "%s: vnum %d: Mob has invalid race! Defaulting to monster.", __func__, vnum );
                  pMobIndex->race = get_npc_race( "monster" );
               }

               if( x2 >= 0 && x2 < 25 )
                  sclass = stock_class[x2];
               else
                  sclass = "warrior";

               pMobIndex->Class = get_npc_class( sclass );

               if( pMobIndex->Class < 0 || pMobIndex->Class >= MAX_NPC_CLASS )
               {
                  bug( "%s: vnum %d: Mob has invalid Class! Defaulting to warrior.", __func__, vnum );
                  pMobIndex->Class = get_npc_class( "warrior" );
               }
            }
            pMobIndex->height = x3;
            pMobIndex->weight = x4;

            {
               char *speaks = nullptr, *speaking = nullptr;
               char flag[MIL];
               int value;

               speaks = flag_string( x5, stock_lang_names );

               while( speaks[0] != '\0' )
               {
                  speaks = one_argument( speaks, flag );
                  value = get_langnum( flag );
                  if( value == -1 )
                     bug( "Unsupported speaks flag dropped: %s", flag );
                  else
                     pMobIndex->speaks.set( value );
               }

               speaking = flag_string( x6, stock_lang_names );

               while( speaking[0] != '\0' )
               {
                  speaking = one_argument( speaking, flag );
                  value = get_langnum( flag );
                  if( value == -1 )
                     bug( "Unsupported speaking flag dropped: %s", flag );
                  else
                     pMobIndex->speaking = value;
               }
            }
            pMobIndex->numattacks = ( float )x7;
         }  /* End of standard Smaug zone */
         else  /* A SmaugWiz zone */
         {
            string speaking, flag;
            int value;

            ln = fread_line( fp );
            x1 = x2 = x3 = x4 = x5 = 0;
            sscanf( ln, "%d %d %d %d %d", &x1, &x2, &x3, &x4, &x5 );

            {
               const char *srace, *sclass;

               if( x1 >= 0 && x1 < 90 )
                  srace = stock_race[x1];
               else
                  srace = "human";

               pMobIndex->race = get_npc_race( srace );

               if( pMobIndex->race < 0 || pMobIndex->race >= MAX_NPC_RACE )
               {
                  bug( "%s: vnum %d: Mob has invalid race: %s. Defaulting to monster.", __func__, vnum, srace );
                  pMobIndex->race = get_npc_race( "monster" );
               }

               if( x2 >= 0 && x2 < 25 )
                  sclass = stock_class[x2];
               else
                  sclass = "warrior";

               pMobIndex->Class = get_npc_class( sclass );

               if( pMobIndex->Class < 0 || pMobIndex->Class >= MAX_NPC_CLASS )
               {
                  bug( "%s: vnum %d: Mob has invalid class: %s. Defaulting to warrior.", __func__, vnum, sclass );
                  pMobIndex->Class = get_npc_class( "warrior" );
               }
            }
            pMobIndex->height = x3;
            pMobIndex->weight = x4;
            pMobIndex->numattacks = ( float )x5;

            flag_set( fp, pMobIndex->speaks, lang_names );

            speaking = fread_flagstring( fp );

            while( !speaking.empty() )
            {
               speaking = one_argument( speaking, flag );
               value = get_langnum( flag );
               if( value == -1 )
                  bug( "Unknown speaking language: %s", flag.c_str() );
               else
                  pMobIndex->speaking = value;
            }
         }  /* End of SmaugWiz zone */

         if( pMobIndex->speaks.none(  ) )
            pMobIndex->speaks.set( LANG_COMMON );
         if( !pMobIndex->speaking )
            pMobIndex->speaking = LANG_COMMON;

         if( pMobIndex->race <= MAX_RACE )   /* Convert the mob to use randatreasure according to race */
            pMobIndex->gold = -1;
         else
            pMobIndex->gold = 0;

         pMobIndex->hitroll = fread_number( fp );
         pMobIndex->damroll = fread_number( fp );
         pMobIndex->body_parts = fread_number( fp );
         pMobIndex->resistant = fread_number( fp );
         pMobIndex->immune = fread_number( fp );
         pMobIndex->susceptible = fread_number( fp );

         {
            char *attacks, *defenses;
            char flag[MIL];
            EXT_BV temp;
            int value;

            temp = fread_bitvector( fp );
            attacks = ext_flag_string( &temp, stock_attack_flags );

            while( attacks[0] != '\0' )
            {
               attacks = one_argument( attacks, flag );
               value = get_attackflag( flag );
               if( value < 0 || value >= MAX_ATTACK_TYPE )
                  bug( "Unsupported attack flag dropped: %s", flag );
               else
                  pMobIndex->attacks.set( value );
            }

            temp = fread_bitvector( fp );
            defenses = ext_flag_string( &temp, stock_defense_flags );

            while( defenses[0] != '\0' )
            {
               defenses = one_argument( defenses, flag );
               value = get_defenseflag( flag );
               if( value < 0 || value >= MAX_DEFENSE_TYPE )
                  bug( "Unsupported defense flag dropped: %s", flag );
               else
                  pMobIndex->defenses.set( value );
            }
         }
         if( letter == 'Z' )
         {
            fread_number( fp );
            fread_number( fp );
            fread_number( fp );
            fread_number( fp );
            fread_number( fp );
            fread_number( fp );
            fread_number( fp );
            fread_number( fp );
         }
         if( letter == 'D' && dotdcheck )
         {
            fread_number( fp );
            fread_number( fp );
            pMobIndex->absorb = fread_number( fp );
         }
      }
      else
      {
         pMobIndex->perm_str = 13;
         pMobIndex->perm_dex = 13;
         pMobIndex->perm_int = 13;
         pMobIndex->perm_wis = 13;
         pMobIndex->perm_cha = 13;
         pMobIndex->perm_con = 13;
         pMobIndex->perm_lck = 13;
         pMobIndex->race = RACE_HUMAN;
         pMobIndex->Class = CLASS_WARRIOR;
         pMobIndex->numattacks = 0;
         pMobIndex->body_parts.reset(  );
         pMobIndex->resistant.reset(  );
         pMobIndex->immune.reset(  );
         pMobIndex->susceptible.reset(  );
         pMobIndex->absorb.reset(  );
         pMobIndex->attacks.reset(  );
         pMobIndex->defenses.reset(  );
      }

      for( ;; )
      {
         letter = fread_letter( fp );

         if( letter == 'S' || letter == '$' )
            break;

         if( letter == '#' )
         {
            ungetc( letter, fp );
            break;
         }

         if( letter == 'T' )
            fread_to_eol( fp );

         else if( letter == '>' )
         {
            ungetc( letter, fp );
            pMobIndex->mprog_read_programs( fp );
         }
         else
         {
            bug( "%s: vnum %d has unknown field '%c' after defense values", __func__, vnum, letter );
            shutdown_mud( "Invalid mob field data" );
            exit( 1 );
         }
      }

      if( !oldmob )
      {
         mob_index_table.insert( map < int, mob_index * >::value_type( vnum, pMobIndex ) );
         tarea->mobs.push_back( pMobIndex );
         ++top_mob_index;
      }
   }
}

void load_stobjects( area_data * tarea, FILE * fp, bool manual )
{
   obj_index *pObjIndex;
   char letter;
   const char *ln;
   int x1, x2, x3, x4, x5, x6;

   if( !tarea )
   {
      bug( "%s: no #AREA seen yet.", __func__ );
      if( fBootDb )
      {
         shutdown_mud( "No #AREA" );
         exit( 1 );
      }
      else
         return;
   }

   for( ;; )
   {
      bool tmpBootDb, oldobj;

      letter = fread_letter( fp );
      if( letter != '#' )
      {
         bug( "%s: # not found.", __func__ );
         if( fBootDb )
         {
            shutdown_mud( "# not found" );
            exit( 1 );
         }
         else
            return;
      }

      int vnum = fread_number( fp );
      if( vnum == 0 )
         break;

      tmpBootDb = fBootDb;
      fBootDb = false;

      list < area_data * >::iterator iarea;
      for( iarea = arealist.begin(  ); iarea != arealist.end(  ); ++iarea )
      {
         area_data *area = *iarea;

         if( !str_cmp( area->filename, tarea->filename ) )
            continue;

         bool area_conflict = check_area_conflict( area, vnum, vnum );

         if( area_conflict )
         {
            log_printf( "ERROR: %s has vnum conflict with %s!", tarea->filename, ( area->filename ? area->filename : "(invalid)" ) );
            log_printf( "%s occupies vnums   : %-6d - %-6d", ( area->filename ? area->filename : "(invalid)" ), area->low_vnum, area->hi_vnum );
            log_printf( "%s wants to use vnum: %-6d", tarea->filename, vnum );
            if( !manual )
            {
               log_string( "This is a fatal error. Program terminated." );
               exit( 1 );
            }
            else
            {
               area_failed = true;
               log_string( "This is a fatal error. Conversion terminated." );
               deleteptr( tarea );
               return;
            }
         }
      }

      if( get_obj_index( vnum ) )
      {
         if( tmpBootDb )
         {
            bug( "%s: vnum %d duplicated.", __func__, vnum );
            if( manual )
            {
               area_failed = true;
               log_string( "This is a fatal error. Conversion terminated." );
               deleteptr( tarea );
               return;
            }
            else
            {
               shutdown_mud( "duplicate vnum" );
               exit( 1 );
            }
         }
         else
         {
            pObjIndex = get_obj_index( vnum );
            log_printf_plus( LOG_BUILD, sysdata->build_level, "Cleaning object: %d", vnum );
            pObjIndex->clean_obj(  );
            oldobj = true;
         }
      }
      else
      {
         oldobj = false;
         pObjIndex = new obj_index;
      }
      fBootDb = tmpBootDb;

      pObjIndex->vnum = vnum;
      if( fBootDb )
      {
         if( !tarea->low_vnum )
            tarea->low_vnum = vnum;
         if( vnum > tarea->hi_vnum )
            tarea->hi_vnum = vnum;
      }

      pObjIndex->area = tarea;
      pObjIndex->name = fread_string( fp );
      pObjIndex->short_descr = fread_string( fp );
      pObjIndex->objdesc = fread_string( fp );

      const char *desc = fread_flagstring( fp );
      if( desc && desc[0] != '\0' && str_cmp( desc, "(null)" ) )
         pObjIndex->action_desc = STRALLOC( desc );

      if( pObjIndex->objdesc != nullptr )
         pObjIndex->objdesc[0] = UPPER( pObjIndex->objdesc[0] );
      {
         const char *sotype;
         char *eflags, *wflags;
         char flag[MIL];
         EXT_BV temp;
         int value;

         sotype = stock_o_types[fread_number( fp )];
         pObjIndex->item_type = get_otype( sotype );

         temp = fread_bitvector( fp );
         eflags = ext_flag_string( &temp, stock_oflags );

         while( eflags[0] != '\0' )
         {
            eflags = one_argument( eflags, flag );
            value = get_oflag( flag );
            if( value < 0 || value >= MAX_ITEM_FLAG )
               bug( "Unsupported flag dropped: %s", flag );
            else
               pObjIndex->extra_flags.set( value );
         }

         ln = fread_line( fp );
         x1 = x2 = 0;
         sscanf( ln, "%d %d", &x1, &x2 );

         wflags = flag_string( x1, stock_wflags );

         while( wflags[0] != '\0' )
         {
            wflags = one_argument( wflags, flag );
            value = get_wflag( flag );
            if( value < 0 || value >= MAX_WEAR_FLAG )
               bug( "Unsupported wear flag dropped: %s", flag );
            else
               pObjIndex->wear_flags.set( value );
         }
         pObjIndex->layers = x2;
      }

      if( tarea->version < 1000 )
      {
         ln = fread_line( fp );
         x1 = x2 = x3 = x4 = x5 = x6 = 0;
         sscanf( ln, "%d %d %d %d %d %d", &x1, &x2, &x3, &x4, &x5, &x6 );
         pObjIndex->value[0] = x1;
         pObjIndex->value[1] = x2;
         pObjIndex->value[2] = x3;
         pObjIndex->value[3] = x4;
         pObjIndex->value[4] = x5;
         pObjIndex->value[5] = x6;

         pObjIndex->weight = fread_number( fp );
         pObjIndex->weight = UMAX( 1, pObjIndex->weight );
         pObjIndex->cost = fread_number( fp );
         pObjIndex->ego = fread_number( fp );

         if( pObjIndex->ego >= sysdata->minego )
         {
            log_printf( "Item %d gaining new rare item limit of 1", pObjIndex->vnum );
            pObjIndex->limit = 1;   /* Sets new limit since stock zones won't have one */
         }
         else
            pObjIndex->limit = 9999;   /* Default value, this should more than insure that the shit loads */

         pObjIndex->ego = -2;
      }
      else
      {
         ln = fread_line( fp );
         x1 = x2 = x3 = x4 = x5 = x6 = 0;
         sscanf( ln, "%d %d %d %d %d %d", &x1, &x2, &x3, &x4, &x5, &x6 );
         pObjIndex->value[0] = x1;
         pObjIndex->value[1] = x2;
         pObjIndex->value[2] = x3;
         pObjIndex->value[3] = x4;
         pObjIndex->value[4] = x5;
         pObjIndex->value[5] = x6;
      }

      if( tarea->version == 1 || tarea->version == 1000 )
      {
         switch ( pObjIndex->item_type )
         {
            default:
               break;

            case ITEM_PILL:
            case ITEM_POTION:
            case ITEM_SCROLL:
               pObjIndex->value[1] = skill_lookup( fread_word( fp ) );
               pObjIndex->value[2] = skill_lookup( fread_word( fp ) );
               pObjIndex->value[3] = skill_lookup( fread_word( fp ) );
               break;

            case ITEM_STAFF:
            case ITEM_WAND:
               pObjIndex->value[3] = skill_lookup( fread_word( fp ) );
               break;

            case ITEM_SALVE:
               pObjIndex->value[4] = skill_lookup( fread_word( fp ) );
               pObjIndex->value[5] = skill_lookup( fread_word( fp ) );
               break;
         }
      }

      if( tarea->version == 1000 )
      {
         while( !isdigit( letter = fread_letter( fp ) ) )
            fread_to_eol( fp );
         ungetc( letter, fp );

         pObjIndex->weight = fread_number( fp );
         pObjIndex->weight = UMAX( 1, pObjIndex->weight );
         pObjIndex->cost = fread_number( fp );
         pObjIndex->ego = fread_number( fp );

         if( pObjIndex->ego >= sysdata->minego )
         {
            log_printf( "Item %d gaining new rare item limit of 1", pObjIndex->vnum );
            pObjIndex->limit = 1;   /* Sets new limit since stock zones won't have one */
         }
         else
            pObjIndex->limit = 9999;   /* Default value, this should more than insure that the shit loads */

         pObjIndex->ego = -2;
      }

      for( ;; )
      {
         letter = fread_letter( fp );

         if( letter == 'S' )
            break;

         if( letter == 'A' )
         {
            affect_data *paf;
            char *risa = nullptr;
            char flag[MIL];
            int value;

            paf = new affect_data;
            paf->type = -1;
            paf->duration = -1;
            paf->bit = 0;
            paf->modifier = 0;
            paf->rismod.reset(  );

            paf->location = fread_number( fp );

            if( paf->location == APPLY_WEAPONSPELL
                || paf->location == APPLY_WEARSPELL
                || paf->location == APPLY_REMOVESPELL || paf->location == APPLY_STRIPSN || paf->location == APPLY_RECURRINGSPELL || paf->location == APPLY_EAT_SPELL )
               paf->modifier = slot_lookup( fread_number( fp ) );
            else if( paf->location == APPLY_RESISTANT || paf->location == APPLY_IMMUNE || paf->location == APPLY_SUSCEPTIBLE || paf->location == APPLY_ABSORB )
            {
               value = fread_number( fp );
               risa = flag_string( value, ris_flags );

               while( risa[0] != '\0' )
               {
                  risa = one_argument( risa, flag );
                  value = get_risflag( flag );
                  if( value < 0 || value >= MAX_RIS_FLAG )
                     bug( "%s: Unsupportable value for RISA flag: %s", __func__, flag );
                  else
                     paf->rismod.set( value );
               }
            }
            else
               paf->modifier = fread_number( fp );
            paf->bit = 0;
            pObjIndex->affects.push_back( paf );
            ++top_affect;
         }

         else if( letter == 'D' )
         {
            fread_number( fp );
            fread_number( fp );
         }

         else if( letter == 'E' )
         {
            extra_descr_data *ed = new extra_descr_data;

            fread_string( ed->keyword, fp );
            fread_string( ed->desc, fp );
            pObjIndex->extradesc.push_back( ed );
            ++top_ed;
         }
         else if( letter == '>' )
         {
            ungetc( letter, fp );
            pObjIndex->oprog_read_programs( fp );
         }

         else
         {
            ungetc( letter, fp );
            break;
         }
      }

      /*
       * Translate spell "slot numbers" to internal "skill numbers."
       */
      if( tarea->version == 0 )
      {
         switch ( pObjIndex->item_type )
         {
            default:
               break;

            case ITEM_PILL:
            case ITEM_POTION:
            case ITEM_SCROLL:
               pObjIndex->value[1] = slot_lookup( pObjIndex->value[1] );
               pObjIndex->value[2] = slot_lookup( pObjIndex->value[2] );
               pObjIndex->value[3] = slot_lookup( pObjIndex->value[3] );
               break;

            case ITEM_STAFF:
            case ITEM_WAND:
               pObjIndex->value[3] = slot_lookup( pObjIndex->value[3] );
               break;

            case ITEM_SALVE:
               pObjIndex->value[4] = slot_lookup( pObjIndex->value[4] );
               pObjIndex->value[5] = slot_lookup( pObjIndex->value[5] );
               break;
         }
      }

      /*
       * Set stuff the object won't have in stock 
       */
      pObjIndex->socket[0] = STRALLOC( "None" );
      pObjIndex->socket[1] = STRALLOC( "None" );
      pObjIndex->socket[2] = STRALLOC( "None" );

      if( !oldobj )
      {
         obj_index_table.insert( map < int, obj_index * >::value_type( pObjIndex->vnum, pObjIndex ) );
         tarea->objects.push_back( pObjIndex );
         ++top_obj_index;
      }
   }
}

void load_strooms( area_data * tarea, FILE * fp, bool manual )
{
   room_index *pRoomIndex;
   const char *ln;
   int count = 0;

   if( !tarea )
   {
      bug( "%s: no #AREA seen yet.", __func__ );
      shutdown_mud( "No #AREA" );
      exit( 1 );
   }

   for( ;; )
   {
      char letter;
      int door;
      bool tmpBootDb;
      bool oldroom;
      int x1, x2, x3, x4, x5, x6, x7, x8, x9;

      letter = fread_letter( fp );
      if( letter != '#' )
      {
         bug( "%s: # not found.", __func__ );
         if( fBootDb )
         {
            shutdown_mud( "# not found" );
            exit( 1 );
         }
         else
            return;
      }

      int vnum = fread_number( fp );
      if( vnum == 0 )
         break;

      tmpBootDb = fBootDb;
      fBootDb = false;

      list < area_data * >::iterator iarea;
      for( iarea = arealist.begin(  ); iarea != arealist.end(  ); ++iarea )
      {
         area_data *area = *iarea;

         if( !str_cmp( area->filename, tarea->filename ) )
            continue;

         bool area_conflict = check_area_conflict( area, vnum, vnum );

         if( area_conflict )
         {
            log_printf( "ERROR: %s has vnum conflict with %s!", tarea->filename, ( area->filename ? area->filename : "(invalid)" ) );
            log_printf( "%s occupies vnums   : %-6d - %-6d", ( area->filename ? area->filename : "(invalid)" ), area->low_vnum, area->hi_vnum );
            log_printf( "%s wants to use vnum: %-6d", tarea->filename, vnum );
            if( !manual )
            {
               log_string( "This is a fatal error. Program terminated." );
               exit( 1 );
            }
            else
            {
               area_failed = true;
               log_string( "This is a fatal error. Conversion terminated." );
               deleteptr( tarea );
               return;
            }
         }
      }

      if( get_room_index( vnum ) != nullptr )
      {
         if( tmpBootDb )
         {
            bug( "%s: vnum %d duplicated.", __func__, vnum );
            if( manual )
            {
               area_failed = true;
               log_string( "This is a fatal error. Conversion terminated." );
               deleteptr( tarea );
               return;
            }
            else
            {
               shutdown_mud( "duplicate vnum" );
               exit( 1 );
            }
         }
         else
         {
            pRoomIndex = get_room_index( vnum );
            log_printf_plus( LOG_BUILD, sysdata->build_level, "Cleaning room: %d", vnum );
            pRoomIndex->clean_room(  );
            oldroom = true;
         }
      }
      else
      {
         oldroom = false;
         pRoomIndex = new room_index;
      }

      fBootDb = tmpBootDb;
      pRoomIndex->area = tarea;
      pRoomIndex->vnum = vnum;

      if( fBootDb )
      {
         if( !tarea->low_vnum )
            tarea->low_vnum = vnum;
         if( vnum > tarea->hi_vnum )
            tarea->hi_vnum = vnum;
      }
      pRoomIndex->name = fread_string( fp );

      const char *desc = fread_flagstring( fp );
      if( desc && desc[0] != '\0' && str_cmp( desc, "(null)" ) )
         pRoomIndex->roomdesc = str_dup( desc );

      /*
       * Area number         fread_number( fp ); 
       */
      ln = fread_line( fp );
      x1 = x2 = x3 = x4 = x5 = x6 = x7 = x8 = x9 = 0;
      sscanf( ln, "%d %d %d %d %d %d %d %d %d", &x1, &x2, &x3, &x4, &x5, &x6, &x7, &x8, &x9 );

      {
         char *roomflags;
         const char *sect;
         char flag[MIL];
         int value;

         roomflags = flag_string( x2, stock_rflags );

         while( roomflags[0] != '\0' )
         {
            roomflags = one_argument( roomflags, flag );
            value = get_rflag( flag );
            if( value < 0 || value >= ROOM_MAX )
               bug( "Unsupported room flag dropped: %s", flag );
            else
               pRoomIndex->flags.set( value );
         }

         sect = stock_sect[x3];
         pRoomIndex->sector_type = get_sectypes( sect );
         pRoomIndex->winter_sector = -1;
      }
      pRoomIndex->tele_delay = x4;
      pRoomIndex->tele_vnum = x5;
      pRoomIndex->tunnel = x6;
      pRoomIndex->baselight = 0;
      pRoomIndex->light = 0;

      if( pRoomIndex->sector_type < 0 || pRoomIndex->sector_type >= SECT_MAX )
      {
         bug( "%s: vnum %d has unsupported sector_type %d.", __func__, vnum, pRoomIndex->sector_type );
         pRoomIndex->sector_type = 1;
      }

      for( ;; )
      {
         letter = fread_letter( fp );

         if( letter == 'S' )
            break;

         // Smaug resets, applied reset fix. We can cheat some here since we wrote that :)
         if( letter == 'R' && ( tarea->version == 0 || tarea->version == 1 ) )
            pRoomIndex->load_reset( fp, false );

         else if( letter == 'R' && tarea->version == 1000 ) /* SmaugWiz resets */
         {
            exit_data *pexit;
            char letter2;
            int extra, arg1, arg2, arg3, arg4;

            letter2 = fread_letter( fp );
            extra = fread_number( fp );
            arg1 = fread_number( fp );
            arg2 = fread_number( fp );
            arg3 = fread_number( fp );
            arg4 = ( letter2 == 'G' || letter2 == 'R' ) ? 0 : fread_number( fp );
            fread_to_eol( fp );

            ++count;

            /*
             * Validate parameters.
             * We're calling the index functions for the side effect.
             */
            switch ( letter2 )
            {
               default:
                  bug( "%s: SmaugWiz - bad command '%c'.", __func__, letter2 );
                  if( fBootDb )
                     boot_log( "%s: %s (%d) bad command '%c'.", __func__, tarea->filename, count, letter2 );
                  return;

               case 'M':
                  if( get_mob_index( arg2 ) == nullptr && fBootDb )
                     boot_log( "%s: SmaugWiz - %s (%d) 'M': mobile %d doesn't exist.", __func__, tarea->filename, count, arg2 );
                  break;

               case 'O':
                  if( get_obj_index( arg2 ) == nullptr && fBootDb )
                     boot_log( "%s: SmaugWiz - %s (%d) '%c': object %d doesn't exist.", __func__, tarea->filename, count, letter2, arg2 );
                  break;

               case 'P':
                  if( get_obj_index( arg2 ) == nullptr && fBootDb )
                     boot_log( "%s: SmaugWiz - %s (%d) '%c': object %d doesn't exist.", __func__, tarea->filename, count, letter2, arg2 );
                  if( arg4 > 0 )
                  {
                     if( get_obj_index( arg4 ) == nullptr && fBootDb )
                        boot_log( "%s: SmaugWiz - %s (%d) 'P': destination object %d doesn't exist.", __func__, tarea->filename, count, arg4 );
                  }
                  break;

               case 'G':
               case 'E':
                  if( get_obj_index( arg2 ) == nullptr && fBootDb )
                     boot_log( "%s: SmaugWiz - %s (%d) '%c': object %d doesn't exist.", __func__, tarea->filename, count, letter2, arg2 );
                  break;

               case 'T':
                  break;

               case 'H':
                  if( arg1 > 0 )
                     if( get_obj_index( arg2 ) == nullptr && fBootDb )
                        boot_log( "%s: SmaugWiz - %s (%d) 'H': object %d doesn't exist.", __func__, tarea->filename, count, arg2 );
                  break;

               case 'D':
                  if( arg3 < 0 || arg3 > MAX_DIR + 1 || !( pexit = pRoomIndex->get_exit( arg3 ) ) || !IS_EXIT_FLAG( pexit, EX_ISDOOR ) )
                  {
                     bug( "%s: SmaugWiz - 'D': exit %d not door.", __func__, arg3 );
                     log_printf( "Reset: %c %d %d %d %d %d", letter2, extra, arg1, arg2, arg3, arg4 );
                     if( fBootDb )
                        boot_log( "%s: SmaugWiz - %s (%d) 'D': exit %d not door.", __func__, tarea->filename, count, arg3 );
                  }
                  if( arg4 < 0 || arg4 > 2 )
                  {
                     bug( "%s: 'D': bad 'locks': %d.", __func__, arg4 );
                     if( fBootDb )
                        boot_log( "%s: SmaugWiz - %s (%d) 'D': bad 'locks': %d.", __func__, tarea->filename, count, arg4 );
                  }
                  break;

               case 'R':
                  if( arg3 < 0 || arg3 > 10 )
                  {
                     bug( "%s: 'R': bad exit %d.", __func__, arg3 );
                     if( fBootDb )
                        boot_log( "%s: SmaugWiz - %s (%d) 'R': bad exit %d.", __func__, tarea->filename, count, arg3 );
                     break;
                  }
                  break;
            }
            /*
             * Don't bother asking why arg1 isn't passed, SmaugWiz had some purpose for it, but it remains a mystery 
             */
            if( letter2 == 'P' )
               pRoomIndex->add_reset( letter2, extra, arg2, arg3, arg4, -1, -1, -1, 100, -2, -2, -2 );
            else
               pRoomIndex->add_reset( letter2, arg2, arg3, arg4, -1, -1, -1, 100, -2, -2, -2, -2 );

         }  /* End SmaugWiz resets */

         else if( letter == 'D' )
         {
            exit_data *pexit;
            int locks;

            door = fread_number( fp );
            if( door < 0 || door > DIR_SOMEWHERE )
            {
               bug( "%s: vnum %d has bad door number %d.", __func__, vnum, door );
               if( fBootDb )
                  exit( 1 );
            }
            else
            {
               pexit = pRoomIndex->make_exit( nullptr, door );
               pexit->exitdesc = fread_string( fp );
               pexit->keyword = fread_string( fp );
               pexit->flags.reset(  );
               ln = fread_line( fp );
               x1 = x2 = x3 = x4 = x5 = x6 = 0;
               sscanf( ln, "%d %d %d %d %d %d", &x1, &x2, &x3, &x4, &x5, &x6 );

               locks = x1;
               pexit->key = x2;
               pexit->vnum = x3;
               pexit->vdir = door;
               pexit->mx = -1;
               pexit->my = -1;
               pexit->pulltype = x5;
               pexit->pull = x6;

               switch ( locks )
               {
                  case 1:
                     SET_EXIT_FLAG( pexit, EX_ISDOOR );
                     break;
                  case 2:
                     SET_EXIT_FLAG( pexit, EX_ISDOOR );
                     SET_EXIT_FLAG( pexit, EX_PICKPROOF );
                     break;
                  default:
                  {
                     char *oldexits = nullptr;
                     char flag[MIL];
                     int value;

                     oldexits = flag_string( locks, stock_ex_flags );

                     while( oldexits[0] != '\0' )
                     {
                        oldexits = one_argument( oldexits, flag );
                        value = get_exflag( flag );
                        if( value < 0 || value >= MAX_EXFLAG )
                           bug( "Unsupported exit flag dropped: %s", flag );
                        else
                           SET_EXIT_FLAG( pexit, value );
                     }
                  }
               }
            }
         }
         else if( letter == 'E' )
         {
            extra_descr_data *ed = new extra_descr_data;

            fread_string( ed->keyword, fp );
            fread_string( ed->desc, fp );
            pRoomIndex->extradesc.push_back( ed );
            ++top_ed;
         }
         else if( letter == 'A' )   // This section was added in SmaugFUSS 1.8
         {
            affect_data *paf;
            char *risa = nullptr;
            char flag[MIL];
            int value;

            paf = new affect_data;
            paf->type = -1;
            paf->duration = -1;
            paf->bit = 0;
            paf->modifier = 0;
            paf->rismod.reset(  );

            paf->location = fread_number( fp );

            if( paf->location == APPLY_WEAPONSPELL
                || paf->location == APPLY_WEARSPELL
                || paf->location == APPLY_REMOVESPELL || paf->location == APPLY_STRIPSN || paf->location == APPLY_RECURRINGSPELL || paf->location == APPLY_EAT_SPELL )
               paf->modifier = slot_lookup( fread_number( fp ) );
            else if( paf->location == APPLY_RESISTANT || paf->location == APPLY_IMMUNE || paf->location == APPLY_SUSCEPTIBLE || paf->location == APPLY_ABSORB )
            {
               value = fread_number( fp );
               risa = flag_string( value, ris_flags );

               while( risa[0] != '\0' )
               {
                  risa = one_argument( risa, flag );
                  value = get_risflag( flag );
                  if( value < 0 || value >= MAX_RIS_FLAG )
                     bug( "%s: Unsupportable value for RISA flag: %s", __func__, flag );
                  else
                     paf->rismod.set( value );
               }
            }
            else
               paf->modifier = fread_number( fp );
            paf->bit = 0;
            pRoomIndex->permaffects.push_back( paf );
            ++top_affect;
         }
         else if( letter == 'M' )   /* maps */
            fread_to_eol( fp );  /* Skip, AFKMud doesn't have these */
         else if( letter == '>' )
         {
            ungetc( letter, fp );
            pRoomIndex->rprog_read_programs( fp );
         }
         else
         {
            bug( "%s: vnum %d has flag '%c' not 'RDES'.", __func__, vnum, letter );
            shutdown_mud( "Room flag not RDES" );
            exit( 1 );
         }
      }

      if( !oldroom )
      {
         room_index_table.insert( map < int, room_index * >::value_type( pRoomIndex->vnum, pRoomIndex ) );
         tarea->rooms.push_back( pRoomIndex );
         ++top_room;
      }
   }
}

void load_stresets( area_data * tarea, FILE * fp )
{
   room_index *pRoomIndex = nullptr;
   bool not01 = false;
   int count = 0;

   if( !tarea )
   {
      bug( "%s: no #AREA seen yet.", __func__ );
      if( fBootDb )
      {
         shutdown_mud( "No #AREA" );
         exit( 1 );
      }
      else
         return;
   }

   if( tarea->rooms.empty(  ) )
   {
      bug( "%s: No #ROOMS section found. Cannot load resets.", __func__ );
      if( fBootDb )
      {
         shutdown_mud( "No #ROOMS" );
         exit( 1 );
      }
      else
         return;
   }

   for( ;; )
   {
      exit_data *pexit;
      char letter;
      int extra, arg1, arg2, arg3 = 0;
      short arg4, arg5, arg6, arg7 = 0;

      if( ( letter = fread_letter( fp ) ) == 'S' )
         break;

      if( letter == '*' )
      {
         fread_to_eol( fp );
         continue;
      }

      extra = fread_number( fp );
      if( letter == 'M' || letter == 'O' )
         extra = 0;
      arg1 = fread_number( fp );
      arg2 = fread_number( fp );
      if( letter != 'G' && letter != 'R' )
         arg3 = fread_number( fp );

      arg4 = arg5 = arg6 = -1;  // Converted resets have no overland coordinates
      fread_to_eol( fp );
      ++count;

      // Converted resets are assumed to fire off 100% of the time
      switch ( letter )
      {
         default:
         case 'M':
         case 'O':
            arg7 = 100;
            break;

         case 'E':
         case 'P':
         case 'T':
         case 'D':
            arg4 = 100;
            break;

         case 'G':
         case 'H':
         case 'R':
            arg3 = 100;
            break;
      }

      /*
       * Validate parameters.
       * We're calling the index functions for the side effect.
       */
      switch ( letter )
      {
         default:
            bug( "%s: bad command '%c'.", __func__, letter );
            if( fBootDb )
               boot_log( "%s: %s (%d) bad command '%c'.", __func__, tarea->filename, count, letter );
            return;

         case 'M':
            if( get_mob_index( arg1 ) == nullptr && fBootDb )
               boot_log( "%s: %s (%d) 'M': mobile %d doesn't exist.", __func__, tarea->filename, count, arg1 );

            if( ( pRoomIndex = get_room_index( arg3 ) ) == nullptr && fBootDb )
               boot_log( "%s: %s (%d) 'M': room %d doesn't exist.", __func__, tarea->filename, count, arg3 );
            else
               pRoomIndex->add_reset( letter, arg1, arg2, arg3, arg4, arg5, arg6, arg7, -2, -2, -2, -2 );
            break;

         case 'O':
            if( get_obj_index( arg1 ) == nullptr && fBootDb )
               boot_log( "%s: %s (%d) '%c': object %d doesn't exist.", __func__, tarea->filename, count, letter, arg1 );

            if( ( pRoomIndex = get_room_index( arg3 ) ) == nullptr && fBootDb )
               boot_log( "%s: %s (%d) '%c': room %d doesn't exist.", __func__, tarea->filename, count, letter, arg3 );
            else
            {
               if( !pRoomIndex )
                  bug( "%s: Unable to add object reset - room not found.", __func__ );
               else
                  pRoomIndex->add_reset( letter, arg1, arg2, arg3, arg4, arg5, arg6, arg7, -2, -2, -2, -2 );
            }
            break;

         case 'P':
            if( get_obj_index( arg1 ) == nullptr && fBootDb )
               boot_log( "%s: %s (%d) '%c': object %d doesn't exist.", __func__, tarea->filename, count, letter, arg1 );
            if( arg3 > 0 )
            {
               if( get_obj_index( arg3 ) == nullptr && fBootDb )
                  boot_log( "%s: %s (%d) 'P': destination object %d doesn't exist.", __func__, tarea->filename, count, arg3 );
               if( extra > 1 )
                  not01 = true;
            }
            if( !pRoomIndex )
               bug( "%s: Unable to add put reset - room not found.", __func__ );
            else
            {
               if( arg3 == 0 )
                  arg3 = OBJ_VNUM_DUMMYOBJ;  // This may look stupid, but for some reason it works.
               pRoomIndex->add_reset( letter, extra, arg1, arg2, arg3, arg4, -2, -2, -2, -2, -2, -2 );
            }
            break;

         case 'G':
         case 'E':
            if( get_obj_index( arg1 ) == nullptr && fBootDb )
               boot_log( "%s: %s (%d) '%c': object %d doesn't exist.", __func__, tarea->filename, count, letter, arg1 );
            if( !pRoomIndex )
               bug( "%s: Unable to add give/equip reset - room not found.", __func__ );
            else
               pRoomIndex->add_reset( letter, arg1, arg2, arg3, arg4, -2, -2, -2, -2, -2, -2, -2 );
            break;

         case 'T':
            if( IS_SET( extra, TRAP_OBJ ) )
               bug( "%s: Unable to add legacy object trap reset. Must be converted manually.", __func__ );
            else
            {
               if( !( pRoomIndex = get_room_index( arg3 ) ) )
                  bug( "%s: Unable to add trap reset - room not found.", __func__ );
               else
                  pRoomIndex->add_reset( letter, extra, arg1, arg2, arg3, arg4, -2, -2, -2, -2, -2, -2 );
            }
            break;

         case 'H':
            bug( "%s: Unable to convert legacy hide reset. Must be converted manually.", __func__ );
            break;

         case 'D':
            if( !( pRoomIndex = get_room_index( arg1 ) ) )
            {
               bug( "%s: 'D': room %d doesn't exist.", __func__, arg1 );
               log_printf( "Reset: %c %d %d %d %d", letter, extra, arg1, arg2, arg3 );
               if( fBootDb )
                  boot_log( "%s: %s (%d) 'D': room %d doesn't exist.", __func__, tarea->filename, count, arg1 );
               break;
            }

            if( arg2 < 0 || arg2 > MAX_DIR + 1 || !( pexit = pRoomIndex->get_exit( arg2 ) ) || !IS_EXIT_FLAG( pexit, EX_ISDOOR ) )
            {
               bug( "%s: 'D': exit %d not door.", __func__, arg2 );
               log_printf( "Reset: %c %d %d %d %d", letter, extra, arg1, arg2, arg3 );
               if( fBootDb )
                  boot_log( "%s: %s (%d) 'D': exit %d not door.", __func__, tarea->filename, count, arg2 );
            }

            if( arg3 < 0 || arg3 > 2 )
            {
               bug( "%s: 'D': bad 'locks': %d.", __func__, arg3 );
               if( fBootDb )
                  boot_log( "%s: %s (%d) 'D': bad 'locks': %d.", __func__, tarea->filename, count, arg3 );
            }
            pRoomIndex->add_reset( letter, arg1, arg2, arg3, arg4, -2, -2, -2, -2, -2, -2, -2 );
            break;

         case 'R':
            if( !( pRoomIndex = get_room_index( arg1 ) ) && fBootDb )
               boot_log( "%s: %s (%d) 'R': room %d doesn't exist.", __func__, tarea->filename, count, arg1 );
            else
               pRoomIndex->add_reset( letter, arg1, arg2, arg3, -2, -2, -2, -2, -2, -2, -2, -2 );
            if( arg2 < 0 || arg2 > 10 )
            {
               bug( "%s: 'R': bad exit %d.", __func__, arg2 );
               if( fBootDb )
                  boot_log( "%s: %s (%d) 'R': bad exit %d.", __func__, tarea->filename, count, arg2 );
               break;
            }
            break;
      }
   }
   if( !not01 )
   {
      list < room_index * >::iterator iroom;
      for( iroom = tarea->rooms.begin(  ); iroom != tarea->rooms.end(  ); ++iroom )
      {
         pRoomIndex = *iroom;

         pRoomIndex->renumber_put_resets(  );
      }
   }
}

void load_stshops( FILE * fp )
{
   shop_data *pShop;

   for( ;; )
   {
      mob_index *pMobIndex;
      int iTrade;

      pShop = new shop_data;
      pShop->keeper = fread_number( fp );
      if( pShop->keeper == 0 )
      {
         deleteptr( pShop );
         break;
      }
      for( iTrade = 0; iTrade < MAX_TRADE; ++iTrade )
         pShop->buy_type[iTrade] = fread_number( fp );
      pShop->profit_buy = fread_number( fp );
      pShop->profit_sell = fread_number( fp );
      pShop->profit_buy = URANGE( pShop->profit_sell + 5, pShop->profit_buy, 1000 );
      pShop->profit_sell = URANGE( 0, pShop->profit_sell, pShop->profit_buy - 5 );
      pShop->open_hour = fread_number( fp );
      pShop->close_hour = fread_number( fp );
      fread_to_eol( fp );
      pMobIndex = get_mob_index( pShop->keeper );
      pMobIndex->pShop = pShop;
      shoplist.push_back( pShop );
      ++top_shop;
   }
}

void load_strepairs( FILE * fp )
{
   repair_data *rShop;

   for( ;; )
   {
      mob_index *pMobIndex;
      int iFix;

      rShop = new repair_data;
      rShop->keeper = fread_number( fp );
      if( rShop->keeper == 0 )
      {
         deleteptr( rShop );
         break;
      }
      for( iFix = 0; iFix < MAX_FIX; ++iFix )
         rShop->fix_type[iFix] = fread_number( fp );
      rShop->profit_fix = fread_number( fp );
      rShop->shop_type = fread_number( fp );
      rShop->open_hour = fread_number( fp );
      rShop->close_hour = fread_number( fp );
      fread_to_eol( fp );
      pMobIndex = get_mob_index( rShop->keeper );
      pMobIndex->rShop = rShop;
      repairlist.push_back( rShop );
      ++top_repair;
   }
}

void load_stock_area_file( const string & filename, bool manual )
{
   area_data *tarea = nullptr;
   char *word;
   struct stat fst;
   time_t umod = 0;

   if( manual )
   {
      char fname[256];

      snprintf( fname, 256, "%s%s", AREA_CONVERT_DIR, filename.c_str(  ) );
      if( !( fpArea = fopen( fname, "r" ) ) )
      {
         perror( fname );
         bug( "%s: Error locating area file for conversion. Not present in conversion directory.", __func__ );
         return;
      }
      if( stat( fname, &fst ) != -1 )
         umod = fst.st_mtime;
   }
   else if( !( fpArea = fopen( filename.c_str(  ), "r" ) ) )
   {
      perror( filename.c_str(  ) );
      bug( "%s: error loading file (can't open) %s", __func__, filename.c_str(  ) );
      return;
   }

   if( umod == 0 && stat( filename.c_str(  ), &fst ) != -1 )
      umod = fst.st_mtime;

   if( fread_letter( fpArea ) != '#' )
   {
      if( fBootDb )
      {
         bug( "%s: No # found at start of area file.", __func__ );
         exit( 1 );
      }
      else
      {
         log_printf( "%s: No # found at start of area file %s", __func__, filename.c_str(  ) );
         return;
      }
   }
   word = fread_word( fpArea );
   if( !str_cmp( word, "AREA" ) )
   {
      tarea = create_area(  );
      tarea->name = fread_string_nohash( fpArea );
      tarea->author = STRALLOC( "unknown" );
      tarea->filename = str_dup( strArea );
      tarea->version = 0;
   }
   else if( !str_cmp( word, "VERSION" ) )
   {
      int temp = fread_number( fpArea );

      if( temp >= 1000 )
      {
         word = fread_word( fpArea );
         if( !str_cmp( word, "#AREA" ) )
         {
            tarea = create_area(  );
            tarea->name = fread_string_nohash( fpArea );
            tarea->author = STRALLOC( "unknown" );
            tarea->filename = str_dup( strArea );
            tarea->version = temp;
         }
         else
         {
            area_failed = true;
            FCLOSE( fpArea );
            log_printf( "%s: Invalid header at start of area file %s", __func__, filename.c_str(  ) );
            return;
         }
      }
   }
   else
   {
      area_failed = true;
      FCLOSE( fpArea );
      log_printf( "%s: Invalid header at start of area file %s", __func__, filename.c_str(  ) );
      return;
   }
   dotdcheck = 0;

   for( ;; )
   {
      if( manual && area_failed )
      {
         FCLOSE( fpArea );
         return;
      }

      if( fread_letter( fpArea ) != '#' )
      {
         log_printf( "%s: # not found. %s", __func__, tarea->filename );
         return;
      }

      word = fread_word( fpArea );

      if( word[0] == '$' )
         break;

      // Skip the helps as we no longer support them imbedded in area files
      else if( !str_cmp( word, "HELPS" ) )
      {
         const char *key, *text;

         fread_number( fpArea );
         key = fread_flagstring( fpArea );
         if( key[0] == '$' )
            continue;
         text = fread_flagstring( fpArea );
         if( text[0] == '\0' )
            continue;
      }
      else if( !str_cmp( word, "AUTHOR" ) )
      {
         STRFREE( tarea->author );
         tarea->author = fread_string( fpArea );
      }
      else if( !str_cmp( word, "FLAGS" ) )
      {
         const char *ln;
         char *aflags;
         char flag[MIL];
         int x1, x2, value;

         ln = fread_line( fpArea );
         x1 = x2 = 0;
         sscanf( ln, "%d %d", &x1, &x2 );

         aflags = flag_string( x1, stock_area_flags );

         while( aflags[0] != '\0' )
         {
            aflags = one_argument( aflags, flag );
            value = get_areaflag( flag );
            if( value < 0 || value >= AFLAG_MAX )
               bug( "Unsupported area flag dropped: %s", flag );
            else
               tarea->flags.set( value );
         }
         tarea->reset_frequency = x2;
      }
      else if( !str_cmp( word, "RANGES" ) )
      {
         int x1, x2, x3, x4;
         const char *ln;

         for( ;; )
         {
            ln = fread_line( fpArea );

            if( ln[0] == '$' )
               break;

            x1 = x2 = x3 = x4 = 0;
            sscanf( ln, "%d %d %d %d", &x1, &x2, &x3, &x4 );

            tarea->low_soft_range = x1;
            tarea->hi_soft_range = x2;
            tarea->low_hard_range = x3;
            tarea->hi_hard_range = x4;
         }
      }
      else if( !str_cmp( word, "ECONOMY" ) )
      {
         /*
          * Not that these values are used anymore, but hey. 
          */
         fread_number( fpArea );
         fread_number( fpArea );
      }
      else if( !str_cmp( word, "RESETMSG" ) )
      {
         DISPOSE( tarea->resetmsg );
         tarea->resetmsg = fread_string_nohash( fpArea );
      }
      else if( !str_cmp( word, "MOBILES" ) )
         load_stmobiles( tarea, fpArea, manual );
      else if( !str_cmp( word, "OBJECTS" ) )
         load_stobjects( tarea, fpArea, manual );
      else if( !str_cmp( word, "RESETS" ) )
         load_stresets( tarea, fpArea );
      else if( !str_cmp( word, "ROOMS" ) )
         load_strooms( tarea, fpArea, manual );
      else if( !str_cmp( word, "SHOPS" ) )
         load_stshops( fpArea );
      else if( !str_cmp( word, "REPAIRS" ) )
         load_strepairs( fpArea );
      else if( !str_cmp( word, "SPECIALS" ) )
      {
         bool done = false;

         for( ;; )
         {
            mob_index *pMobIndex;
            char *temp;
            char letter;

            switch ( letter = fread_letter( fpArea ) )
            {
               default:
                  bug( "%s: letter '%c' not *MORS.", __func__, letter );
                  exit( 1 );

               case 'S':
                  done = true;
                  break;

               case '*':
               case 'O':
               case 'R':
                  break;

               case 'M':
                  pMobIndex = get_mob_index( fread_number( fpArea ) );
                  temp = fread_word( fpArea );
                  if( !pMobIndex )
                  {
                     bug( "%s: 'M': Invalid mob vnum!", __func__ );
                     break;
                  }
                  if( !( pMobIndex->spec_fun = m_spec_lookup( temp ) ) )
                  {
                     bug( "%s: 'M': vnum %d, no spec_fun called %s.", __func__, pMobIndex->vnum, temp );
                     pMobIndex->spec_funname.clear(  );
                  }
                  else
                     pMobIndex->spec_funname = temp;
                  break;
            }
            if( done )
               break;
            fread_to_eol( fpArea );
         }
      }
      else if( !str_cmp( word, "CLIMATE" ) )
      {
         const char *ln;
         int x1, x2, x3, x4;

         if( dotdcheck > 0 && dotdcheck < 4 )
         {
            bug( "DOTDII area encountered with invalid header format, check value %d", dotdcheck );
            shutdown_mud( "Invalid DOTDII area" );
            exit( 1 );
         }

         ln = fread_line( fpArea );
         x1 = x2 = x3 = x4 = 0;
         sscanf( ln, "%d %d %d %d", &x1, &x2, &x3, &x4 );

         tarea->weather->climate_temp = x1;
         tarea->weather->climate_precip = x2;
         tarea->weather->climate_wind = x3;
      }
      else if( !str_cmp( word, "NEIGHBOR" ) )
      {
         neighbor_data *anew;

         anew = new neighbor_data;
         anew->address = nullptr;
         anew->name = fread_string( fpArea );
         tarea->weather->neighborlist.push_back( anew );
      }
      else if( !str_cmp( word, "VERSION" ) )
      {
         int area_version = fread_number( fpArea );

         if( ( area_version < 0 || area_version > 1 ) && area_version != 1000 )
         {
            area_failed = true;
            bug( "%s: Version %d in %s is non-stock area format. Unable to process.", __func__, area_version, filename.c_str(  ) );
            if( !manual )
            {
               shutdown_mud( "Non-standard area format" );
               exit( 1 );
            }
            deleteptr( tarea );
            --top_area;
            return;
         }
         tarea->version = area_version;
      }
      else if( !str_cmp( word, "SPELLLIMIT" ) )
         fread_number( fpArea ); /* Skip, AFKMud doesn't have this */
      else if( !str_cmp( word, "DERIVATIVES" ) )   /* Chronicles tag, safe to skip */
         fread_to_eol( fpArea );
      else if( !str_cmp( word, "NAMEFORMAT" ) ) /* Chronicles tag, safe to skip */
         fread_to_eol( fpArea );
      else if( !str_cmp( word, "DESCFORMAT" ) ) /* Chronicles tag, safe to skip */
         fread_to_eol( fpArea );
      else if( !str_cmp( word, "PLANE" ) )   /* DOTD 2.3.6 tag, safe to skip */
      {
         ++dotdcheck;
         fread_number( fpArea );
      }
      else if( !str_cmp( word, "CURRENCY" ) )   /* DOTD 2.3.6 tag, safe to skip */
      {
         ++dotdcheck;
         fread_to_eol( fpArea );
      }
      else if( !str_cmp( word, "HIGHECONOMY" ) )   /* DOTD 2.3.6 tag, safe to skip */
      {
         ++dotdcheck;
         fread_to_eol( fpArea );
      }
      else if( !str_cmp( word, "LOWECONOMY" ) ) /* DOTD 2.3.6 tag, safe to skip */
      {
         ++dotdcheck;
         fread_to_eol( fpArea );
      }
      else
      {
         bug( "%s: %s: bad section name.", __func__, tarea->filename );
         if( fBootDb )
         {
            shutdown_mud( "Corrupted area file" );
            exit( 1 );
         }
         else
         {
            FCLOSE( fpArea );
            return;
         }
      }
   }
   FCLOSE( fpArea );
   if( tarea )
   {
      tarea->sort_name(  );
      tarea->sort_vnums(  );
      if( tarea->version == 0 && !dotdcheck )
         log_printf( "%-20s: Converted Smaug 1.02a Zone  Vnums: %5d - %-5d", tarea->filename, tarea->low_vnum, tarea->hi_vnum );
      else if( tarea->version == 1 && !dotdcheck )
         log_printf( "%-20s: Converted Smaug 1.4a Zone  Vnums: %5d - %-5d", tarea->filename, tarea->low_vnum, tarea->hi_vnum );
      else if( dotdcheck )
         log_printf( "%-20s: Converted DOTDII 2.3.6 Zone  Vnums: %5d - %-5d", tarea->filename, tarea->low_vnum, tarea->hi_vnum );
      else
         log_printf( "%-20s: Converted SmaugWiz Zone  Vnums: %5d - %-5d", tarea->filename, tarea->low_vnum, tarea->hi_vnum );

      if( tarea->low_vnum < 0 || tarea->hi_vnum < 0 )
         bug( "%-20s: Bad Vnum Range", tarea->filename );
      if( !tarea->author )
         tarea->author = STRALLOC( "Unknown" );
      if( !tarea->creation_date )
         tarea->creation_date = umod;
      if( !tarea->install_date )
         tarea->install_date = umod;
   }
   else
      log_printf( "(%s)", filename.c_str(  ) );
}

/* Use of the forceload argument with this command isn't recommended
 * unless you KNOW for sure that what you're doing will be safe.
 * Trying to force something that is broken *WILL* result in a crashed mud.
 * The argument was added as a laziness feature for the installation of new
 * zones built on a builders' port and transferred to the main port either via
 * the shell code or some other means. IT IS NOT MEANT TO DO ANYTHING ELSE!
 * Omitting it from the helpfile was deliberate.
 * You've been warned. Broken muds are not our fault.
 */
CMDF( do_areaconvert )
{
   area_data *tarea = nullptr;
   int tmp;
   bool manual;
   string arg;

   argument = one_argument( argument, arg );

   if( arg.empty(  ) )
   {
      if( ch )
         ch->print( "Convert what zone?\r\n" );
      else
         bug( "%s: Attempt made to convert with no filename.", __func__ );
      return;
   }

   area_failed = false;

   mudstrlcpy( strArea, arg.c_str(  ), MIL );

   if( ch )
   {
      fBootDb = true;
      manual = true;
      ch->print( "&RReading in area file...\r\n" );
   }
   else
      manual = false;
   if( !argument.empty(  ) && !str_cmp( argument, "forceload" ) )
      load_area_file( strArea, false );
   else
      load_stock_area_file( arg, manual );
   if( ch )
      fBootDb = false;

   if( ch )
   {
      if( area_failed )
      {
         ch->print( "&YArea conversion failed! See your logs for why.\r\n" );
         return;
      }

      if( ( tarea = find_area( arg ) ) )
      {
         ch->print( "&YLinking exits...\r\n" );
         tarea->fix_exits(  );
         if( !tarea->rooms.empty(  ) )
         {
            tmp = tarea->nplayer;
            tarea->nplayer = 0;
            ch->print( "&YResetting area...\r\n" );
            tarea->reset(  );
            tarea->nplayer = tmp;
         }
         if( tarea->version < 2 || tarea->version == 1000 )
         {
            ch->print( "&GWriting area in AFKMud format...\r\n" );
            tarea->fold( tarea->filename, false );
            ch->print( "&YArea conversion complete.\r\n" );
         }
         else
            ch->printf( "&GForced load on %s complete.\r\n", tarea->filename );
         write_area_list(  );
      }
   }
}
