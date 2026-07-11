/****************************************************************************
 *                   ^     +----- |  / ^     ^ |     | +-\                  *
 *                  / \    |      | /  |\   /| |     | |  \                 *
 *                 /   \   +---   |<   | \ / | |     | |  |                 *
 *                /-----\  |      | \  |  v  | |     | |  /                 *
 *               /       \ |      |  \ |     | +-----+ +-/                  *
 ****************************************************************************
 * AFKMud Copyright 1997-2026 by Roger Libiez (Samson),                     *
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
 *                     SmaugFUSS Zone Conversion Support                    *
 ****************************************************************************/

// Converts SmaugFUSS version 1+ areas into AFKMud format.

#include <filesystem>
#include <fstream>
#include "mud.h"
#include "area.h"
#include "areaconvert.h"
#include "mobindex.h"
#include "mud_prog.h"
#include "mudprog_loader.h"
#include "objindex.h"
#include "roomindex.h"
#include "shops.h"

extern int top_prog;
extern int top_affect;
extern int top_shop;
extern int top_repair;

bool check_area_conflict( area_data *, int, int );
void fread_afk_exit( std::ifstream &, room_index * );
extra_descr_data *fread_afk_exdesc( std::ifstream & );

/*
 * There should be 34 entries here to match the size of the sector_types list in olc.h.
 * They should be listed in the same order they appear in sec_flags in build.c for SmaugFUSS.
 * Don't fill in the unused slots unless SmaugFUSS plops a new value into one of them. [Last checked: June 2026]
 */
const char *fuss_sec_flags[] = {
   "inside", "city", "field", "forest", "hills", "mountain", "water_swim",           // 6
   "water_noswim", "underwater", "air", "desert", "dunno", "oceanfloor",             // 12
   "underground", "lava", "swamp", "UNUSED", "UNUSED", "UNUSED", "UNUSED", "UNUSED", // 20
   "UNUSED", "UNUSED", "UNUSED", "UNUSED", "UNUSED", "UNUSED", "UNUSED", "UNUSED",   // 28
   "UNUSED", "UNUSED", "UNUSED", "UNUSED", "UNUSED", "UNUSED"                        // 34
};

// This table needs to stay in sync with SmaugFUSS to remain effective. [Last checked: June 2026]
const char *fuss_npc_race[] = {
   // Playable Races
   "human", "elf", "dwarf", "halfling", "pixie", "half-ogre", "half-orc",          // 6
   "half-troll", "half-elf", "gith", "minotaur", "UNUSED", "UNUSED",               // 12
   "lizardman", "gnome", "drow", "UNUSED", "UNUSED", "UNUSED", "UNUSED",           // 19

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

// This table needs to stay in sync with SmaugFUSS to remain effective. [Last checked: June 2026]
const char *fuss_npc_class[] = {
   // Playable Classes
   "mage", "cleric", "thief", "warrior", "UNUSED", "druid", "ranger",
   "UNUSED", "paladin", "UNUSED", "UNUSED", "pc11", "pc12", "pc13",
   "pc14", "pc15", "pc16", "pc17", "pc18", "pc19",

   // NPCs only
   "baker", "butcher", "blacksmith", "mayor", "king", "queen"
};

// Samson, next time you get the bright idea to shorten a flag name, don't do it, ok? [Last checked: June 2026]
const char *fuss_act_flags[] = {
   "npc", "sentinel", "scavenger", "innkeeper", "banker", "aggressive", "stayarea",
   "wimpy", "pet", "nosteal", "practice", "immortal", "deadly", "polyself",
   "meta_aggr", "guardian", "boarded", "nowander", "mountable", "mounted",
   "scholar", "secretive", "hardhat", "mobinvis", "noassist", "illusion",
   "pacifist", "noattack", "annoying", "auction", "prototype", "mage", "warrior", "cleric",
   "rogue", "druid", "monk", "paladin", "ranger", "necromancer", "antipaladin",
   "huge", "greet", "teacher", "onmap", "smith", "guildauc", "guildbank", "guildvendor",
   "guildrepair", "guildforge", "idmob", "guildidmob", "stopscript"
};

// Samson, next time you get the bright idea to shorten a flag name, don't do it, ok? [Last checked: June 2026]
const char *fuss_o_flags[] = {
   "glow", "hum", "metal", "mineral", "organic", "invis", "magic", "nodrop", "bless",
   "antigood", "antievil", "antineutral", "anticleric", "antimage",
   "antithief", "antiwarrior", "inventory", "noremove", "twohand", "evil",
   "donated", "clanobject", "clancorpse", "antibard", "hidden",
   "antidruid", "poisoned", "covering", "deathrot", "buried", "prototype",
   "nolocate", "groundrot", "antimonk", "loyal", "brittle", "resistant",
   "immune", "antimen", "antiwomen", "antineuter", "antiherma", "antisun", "antiranger",
   "antipaladin", "antinecro", "antiapal", "onlycleric", "onlymage", "onlyrogue",
   "onlywarrior", "onlybard", "onlydruid", "onlymonk", "onlyranger", "onlypaladin",
   "onlynecro", "onlyapal", "auction", "onmap", "personal", "lodged", "sindhae",
   "mustmount", "noauction", "thrown", "permanent", "deathdrop", "nofill"
};

// Samson, next time you get the bright idea to shorten a flag name, don't do it, ok? [Last checked: June 2026]
const char *fuss_r_flags[] = {
   "dark", "death", "nomob", "indoors", "safe", "nocamp", "nosummon",
   "nomagic", "tunnel", "private", "silence", "nosupplicate", "arena", "nomissile",
   "norecall", "noportal", "noastral", "nodrop", "clanstoreroom", "teleport",
   "teleshowdesc", "nofloor", "solitary", "petshop", "donation", "nodropall",
   "logspeech", "prototype", "noteleport", "noscry", "cave", "cavern", "nobeacon",
   "auction", "map", "forge", "guildinn", "isolated", "watchtower",
   "noquit", "telenofly", "_track_", "noyell", "nowhere", "notrack"
};

int get_fuss_sectypes( std::string_view sector )
{
   for( int x = 0; x < SECT_MAX; ++x )
      if( !str_cmp( sector, fuss_sec_flags[x] ) )
         return x;
   return -1;
}

int get_fuss_npc_race( std::string_view type )
{
   for( int x = 0; x < MAX_NPC_RACE; ++x )
      if( !str_cmp( type, fuss_npc_race[x] ) )
         return x;
   return -1;
}

int get_fuss_npc_class( std::string_view type )
{
   for( int x = 0; x < MAX_NPC_CLASS; ++x )
      if( !str_cmp( type, fuss_npc_class[x] ) )
         return x;
   return -1;
}

affect_data *fread_fuss_affect( std::ifstream & stream, std::string_view word )
{
   int pafmod;

   affect_data *paf = new affect_data;
   paf->location = APPLY_NONE;
   paf->type = -1;
   paf->duration = -1;
   paf->bit = 0;
   paf->modifier = 0;
   paf->rismod.reset(  );

   if( !str_cmp( word, "Affect" ) )
   {
      stream >> paf->type;
   }
   else
   {
      int sn;

      sn = skill_lookup( fread_word( stream ) );
      if( sn < 0 )
         bug( "{}: unknown skill.", __func__ );
      else
         paf->type = sn;
   }

   stream >> paf->duration;
   stream >> pafmod;
   stream >> paf->location;
   fread_bitvector( stream ); // Bit conversions don't take for this.

   if( paf->location == APPLY_WEAPONSPELL
       || paf->location == APPLY_WEARSPELL
       || paf->location == APPLY_STRIPSN || paf->location == APPLY_REMOVESPELL || paf->location == APPLY_RECURRINGSPELL )
      paf->modifier = slot_lookup( pafmod );
   else
      paf->modifier = pafmod;

   ++top_affect;
   return paf;
}

void fread_fuss_room( std::ifstream & stream, area_data * tarea )
{
   room_index *pRoomIndex = nullptr;
   bool oldroom = false;

   std::string key;
   while( stream >> key )
   {
      if( key == "Vnum" )
      {
         bool tmpBootDb = fBootDb;
         fBootDb = false;

         int vnum;
         stream >> vnum;

         for( auto* area : arealist )
         {
            if( !str_cmp( area->filename, tarea->filename ) )
               continue;

            bool area_conflict = check_area_conflict( area, vnum, vnum );

            if( area_conflict )
            {
               log_printf( "ERROR: {} has vnum conflict with {}!", tarea->filename, ( !area->filename.empty() ? area->filename : "(invalid)" ) );
               log_printf( "{} occupies vnums   : {:<6} - {:<6}", ( !area->filename.empty() ? area->filename : "(invalid)" ), area->low_vnum, area->hi_vnum );
               log_printf( "{} wants to use vnum: {:<6}", tarea->filename, vnum );
               log_string( "This is a fatal error. Program terminated." );
               std::exit( EXIT_FAILURE );
            }
         }

         if( get_room_index( vnum ) )
         {
            if( tmpBootDb )
            {
               fBootDb = tmpBootDb;
               bug( "{}: vnum {} duplicated.", __func__, vnum );

               // Try to recover, read to end of duplicated room and then bail out
               for( ;; )
               {
                  stream >> key;
                  if( key == "#ENDROOM" || stream.eof() )
                     return;
               }
            }
            else
            {
               pRoomIndex = get_room_index( vnum );
               log_printf_plus( LOG_BUILD, sysdata->build_level, "Cleaning room: {}", vnum );
               pRoomIndex->clean_room(  );
               oldroom = true;
            }
         }
         else
         {
            pRoomIndex = new room_index;
            pRoomIndex->clean_room(  );
         }
         pRoomIndex->vnum = vnum;
         pRoomIndex->area = tarea;
         fBootDb = tmpBootDb;

         if( fBootDb )
         {
            if( !tarea->low_vnum )
               tarea->low_vnum = vnum;
            if( vnum > tarea->hi_vnum )
               tarea->hi_vnum = vnum;
         }

         if( !pRoomIndex->resets.empty(  ) )
         {
            if( fBootDb )
            {
               int count = 0;

               bug( "{}: WARNING: resets already exist for this room.", __func__ );
               for( auto* rtmp : pRoomIndex->resets )
               {
                  ++count;
                  if( !rtmp->resets.empty(  ) )
                     count += rtmp->resets.size(  );
               }
            }
            else
            {
               /*
                * Clean out the old resets
                */
               log_printf_plus( LOG_BUILD, sysdata->build_level, "Cleaning resets: {}", tarea->name );
               pRoomIndex->clean_resets(  );
            }
         }
      }
      else if( key == "Name" )
         pRoomIndex->name = fread_line( stream );
      else if( key == "Sector" )
      {
         std::string sec_type = fread_line( stream );
         int sector = get_fuss_sectypes( sec_type );

         if( sector < 0 || sector >= SECT_MAX )
         {
            bug( "{}: Room #{} has bad sector type: {}", __func__, pRoomIndex->vnum, sec_type );
            sector = 1;
         }

         pRoomIndex->sector_type = sector;
         pRoomIndex->winter_sector = -1;
      }
      else if( key == "Flags" )
         flag_set( stream, pRoomIndex->flags, fuss_r_flags );
      else if( key == "Stats" )
         stream >> pRoomIndex->tele_delay >> pRoomIndex->tele_vnum >> pRoomIndex->tunnel >> pRoomIndex->max_weight;
      else if( key == "Desc" )
         pRoomIndex->roomdesc = fread_line( stream );
      else if( key == "#EXIT" ) // Format of this section is identical to AFKMud. Let's cheat :)
         fread_afk_exit( stream, pRoomIndex );
      else if( key == "Reset" ) // Format of this section is identical to AFKMud. Let's cheat :)
         pRoomIndex->load_reset( stream, false );
      else if( key == "Affect" || key == "AffectData" )
      {
         affect_data *af = fread_fuss_affect( stream, key );

         if( af )
            pRoomIndex->permaffects.push_back( af );
      }
      else if( key == "#EXDESC" ) // Format of this section is identical to AFKMud. Let's cheat :)
      {
         extra_descr_data *ed = fread_afk_exdesc( stream );
         if( ed )
            pRoomIndex->extradesc.push_back( ed );
      }
      else if( key == "#MUDPROG" ) // Format of this section is identical to AFKMud. Let's cheat :)
      {
         mud_prog_data *mprg = new mud_prog_data;
         fread_afk_mudprog( stream, mprg, pRoomIndex );
         pRoomIndex->mudprogs.push_back( mprg );
         ++top_prog;
      }
      else if( key == "#ENDROOM" )
      {
         if( !oldroom )
         {
            room_index_table.insert( std::map<int, room_index *>::value_type( pRoomIndex->vnum, pRoomIndex ) );
            tarea->rooms.push_back( pRoomIndex );
            ++top_room;
         }
         return;
      }
      else
      {
         bug( "{}: Bad section '{}' in FUSS rooms - skipping.", __func__, key );
         fread_to_eol( stream );
      }
   }
}

void fread_fuss_object( std::ifstream & stream, area_data * tarea )
{
   obj_index *pObjIndex = nullptr;
   bool oldobj = false;

   std::string key;
   while( stream >> key )
   {
      if( key == "Vnum" )
      {
         bool tmpBootDb = fBootDb;
         fBootDb = false;

         int vnum;
         stream >> vnum;

         for( auto* area : arealist )
         {
            if( !str_cmp( area->filename, tarea->filename ) )
               continue;

            bool area_conflict = check_area_conflict( area, vnum, vnum );

            if( area_conflict )
            {
               log_printf( "ERROR: {} has vnum conflict with {}!", tarea->filename, ( !area->filename.empty() ? area->filename : "(invalid)" ) );
               log_printf( "{} occupies vnums   : {:<6} - {:<6}", ( !area->filename.empty() ? area->filename : "(invalid)" ), area->low_vnum, area->hi_vnum );
               log_printf( "{} wants to use vnum: {:<6}", tarea->filename, vnum );
               log_string( "This is a fatal error. Program terminated." );
               std::exit( EXIT_FAILURE );
            }
         }

         if( get_obj_index( vnum ) )
         {
            if( tmpBootDb )
            {
               fBootDb = tmpBootDb;
               bug( "{}: vnum {} duplicated.", __func__, vnum );

               // Try to recover, read to end of duplicated object and then bail out
               for( ;; )
               {
                  stream >> key;
                  if( key == "#ENDOBJECT" || stream.eof() )
                     return;
               }
            }
            else
            {
               pObjIndex = get_obj_index( vnum );
               log_printf_plus( LOG_BUILD, sysdata->build_level, "Cleaning object: {}", vnum );
               pObjIndex->clean_obj(  );
               oldobj = true;
            }
         }
         else
         {
            pObjIndex = new obj_index;
            pObjIndex->clean_obj(  );
         }
         pObjIndex->vnum = vnum;
         pObjIndex->area = tarea;
         fBootDb = tmpBootDb;

         if( fBootDb )
         {
            if( !tarea->low_vnum )
               tarea->low_vnum = vnum;
            if( vnum > tarea->hi_vnum )
               tarea->hi_vnum = vnum;
         }
      }
      else if( key == "Keywords" )
         pObjIndex->name = fread_line( stream );
      else if( key == "Type" )
      {
         std::string o_type = fread_line( stream );
         int value = get_otype( o_type );

         if( value < 0 )
         {
            bug( "{}: vnum {}: Object has invalid type! Defaulting to trash.", __func__, pObjIndex->vnum );
            value = get_otype( "trash" );
         }
         pObjIndex->item_type = value;
      }
      else if( key == "Short" )
         pObjIndex->short_descr = fread_line( stream );
      else if( key == "Long" )
         pObjIndex->objdesc = fread_line( stream );
      else if( key == "Action" )
         pObjIndex->action_desc = fread_line( stream );
      else if( key == "Flags" )
         flag_set( stream, pObjIndex->extra_flags, fuss_o_flags );
      else if( key == "WFlags" )
         flag_set( stream, pObjIndex->wear_flags, w_flags );
      else if( key == "Values" )
         stream >> pObjIndex->value[0] >> pObjIndex->value[1] >> pObjIndex->value[2] >> pObjIndex->value[3] >> pObjIndex->value[4] >> pObjIndex->value[5];
      else if( key == "Stats" )
      {
         stream >> pObjIndex->weight;
         pObjIndex->weight = umax( 1, pObjIndex->weight );

         stream >> pObjIndex->cost >> pObjIndex->ego >> pObjIndex->level >> pObjIndex->layers;

         pObjIndex->socket[0] = "None";
         pObjIndex->socket[1] = "None";
         pObjIndex->socket[2] = "None";

         if( pObjIndex->ego >= sysdata->minego )
         {
            log_printf( "Item {} gaining new rare item limit of 1", pObjIndex->vnum );
            pObjIndex->limit = 1;   /* Sets new limit since stock zones won't have one */
         }
         else
            pObjIndex->limit = 9999;   /* Default value, this should more than insure that the shit loads */

         pObjIndex->ego = -2;
      }
      else if( key == "Affect" || key == "AffectData" )
      {
         affect_data *af = fread_fuss_affect( stream, key );

         if( af )
            pObjIndex->affects.push_back( af );
      }
      else if( key == "Spells" )
      {
         switch( pObjIndex->item_type )
         {
            default:
               break;

            case ITEM_PILL:
            case ITEM_POTION:
            case ITEM_SCROLL:
               pObjIndex->value[1] = skill_lookup( fread_word( stream ) );
               pObjIndex->value[2] = skill_lookup( fread_word( stream ) );
               pObjIndex->value[3] = skill_lookup( fread_word( stream ) );
               break;

            case ITEM_STAFF:
            case ITEM_WAND:
               pObjIndex->value[3] = skill_lookup( fread_word( stream ) );
               break;

            case ITEM_SALVE:
               pObjIndex->value[4] = skill_lookup( fread_word( stream ) );
               pObjIndex->value[5] = skill_lookup( fread_word( stream ) );
               break;
         }
      }
      else if( key == "#EXDESC" ) // Format of this section is identical to AFKMud. Let's cheat :)
      {
         extra_descr_data *ed = fread_afk_exdesc( stream );
         if( ed )
            pObjIndex->extradesc.push_back( ed );
      }
      else if( key == "#MUDPROG" ) // Format of this section is identical to AFKMud. Let's cheat :)
      {
         mud_prog_data *mprg = new mud_prog_data;
         fread_afk_mudprog( stream, mprg, pObjIndex );
         pObjIndex->mudprogs.push_back( mprg );
         ++top_prog;
      }
      else if( key == "#ENDOBJECT" )
      {
         if( !oldobj )
         {
            obj_index_table.insert( std::map<int, obj_index *>::value_type( pObjIndex->vnum, pObjIndex ) );
            tarea->objects.push_back( pObjIndex );
            ++top_obj_index;
         }
         return;
      }
      else
      {
         bug( "{}: Bad section '{}' in FUSS objects - skipping.", __func__, key );
         fread_to_eol( stream );
      }
   }
}

void fread_fuss_mobile( std::ifstream & stream, area_data * tarea )
{
   mob_index *pMobIndex = nullptr;
   bool oldmob = false;

   std::string key;
   while( stream >> key )
   {
      if( key == "Vnum" )
      {
         bool tmpBootDb = fBootDb;
         fBootDb = false;

         int vnum;
         stream >> vnum;

         for( auto* area : arealist )
         {
            if( !str_cmp( area->filename, tarea->filename ) )
               continue;

            bool area_conflict = check_area_conflict( area, vnum, vnum );

            if( area_conflict )
            {
               log_printf( "ERROR: {} has vnum conflict with {}!", tarea->filename, ( !area->filename.empty() ? area->filename : "(invalid)" ) );
               log_printf( "{} occupies vnums   : {:<6} - {:<6}", ( !area->filename.empty() ? area->filename : "(invalid)" ), area->low_vnum, area->hi_vnum );
               log_printf( "{} wants to use vnum: {:<6}", tarea->filename, vnum );
               log_string( "This is a fatal error. Program terminated." );
               std::exit( EXIT_FAILURE );
            }
         }

         if( get_mob_index( vnum ) )
         {
            if( tmpBootDb )
            {
               fBootDb = tmpBootDb;
               bug( "{}: vnum {} duplicated.", __func__, vnum );

               // Try to recover, read to end of duplicated mobile and then bail out.
               for( ;; )
               {
                  stream >> key;
                  if( key == "#ENDMOBILE" || stream.eof() )
                     return;
               }
            }
            else
            {
               pMobIndex = get_mob_index( vnum );
               log_printf_plus( LOG_BUILD, sysdata->build_level, "Cleaning mobile: {}", vnum );
               pMobIndex->clean_mob(  );
               oldmob = true;
            }
         }
         else
         {
            pMobIndex = new mob_index;
            pMobIndex->clean_mob(  );
         }
         pMobIndex->vnum = vnum;
         pMobIndex->area = tarea;
         fBootDb = tmpBootDb;

         if( fBootDb )
         {
            if( !tarea->low_vnum )
               tarea->low_vnum = vnum;
            if( vnum > tarea->hi_vnum )
               tarea->hi_vnum = vnum;
         }
      }
      else if( key == "Keywords" )
         pMobIndex->player_name = fread_line( stream );
      else if( key == "Short" )
         pMobIndex->short_descr = fread_line( stream );
      else if( key == "Long" )
         pMobIndex->long_descr = fread_line( stream );
      else if( key == "Desc" )
         pMobIndex->chardesc = fread_line( stream );
      else if( key == "Race" )
      {
         std::string race_word = fread_line( stream );
         short race = get_fuss_npc_race( race_word );

         if( race < 0 || race >= MAX_NPC_RACE )
         {
            bug( "{}: vnum {}: Mob has invalid race: {} Defaulting to monster.", __func__, pMobIndex->vnum, race_word );
            race = get_npc_race( "monster" );
         }

         pMobIndex->race = race;
      }
      else if( key == "Class" )
      {
         std::string class_word = fread_line( stream );
         short Class = get_fuss_npc_class( class_word );

         if( Class < 0 || Class >= MAX_NPC_CLASS )
         {
            bug( "{}: vnum {}: Mob has invalid class: {}. Defaulting to warrior.", __func__, pMobIndex->vnum, class_word );
            Class = get_npc_class( "warrior" );
         }

         pMobIndex->Class = Class;
      }
      else if( key == "Position" )
      {
         std::string pos_word = fread_line( stream );
         short position = get_npc_position( pos_word );

         if( position < 0 || position >= POS_MAX )
         {
            bug( "{}: vnum {}: Mobile in invalid position: {}. Defaulting to standing.", __func__, pMobIndex->vnum, pos_word );
            position = POS_STANDING;
         }
         pMobIndex->position = position;
      }
      else if( key == "DefPos" )
      {
         std::string pos_word = fread_line( stream );
         short position = get_npc_position( pos_word );

         if( position < 0 || position >= POS_MAX )
         {
            bug( "{}: vnum {}: Mobile in invalid default position: {}. Defaulting to standing.", __func__, pMobIndex->vnum, pos_word );
            position = POS_STANDING;
         }
         pMobIndex->defposition = position;
      }
      else if( key == "Specfun" )
      {
         std::string temp = fread_line( stream );

         if( !pMobIndex )
            bug( "{}: Specfun: Invalid mob vnum!", __func__ );
         else if( !( pMobIndex->spec_fun = m_spec_lookup( temp ) ) )
         {
            bug( "{}: Specfun: vnum {}, no spec_fun called {}.", __func__, pMobIndex->vnum, temp );
            pMobIndex->spec_funname.clear(  );
         }
         else
            pMobIndex->spec_funname = temp;
      }
      else if( key == "Gender" )
      {
         std::string gender = fread_line( stream );
         short sex = get_npc_sex( gender );

         if( sex < 0 || sex > SEX_FEMALE )
         {
            bug( "{}: vnum {}: Mobile has invalid gender: {}. Defaulting to neuter.", __func__, pMobIndex->vnum, gender );
            sex = SEX_NEUTRAL;
         }
         pMobIndex->sex = sex;
      }
      else if( key == "Actflags" )
         flag_set( stream, pMobIndex->actflags, fuss_act_flags );
      else if( key == "Affected" )
         flag_set( stream, pMobIndex->affected_by, aff_flags );
      else if( key == "Stats1" )
      {
         stream >> pMobIndex->alignment >> pMobIndex->level >> pMobIndex->mobthac0 >> pMobIndex->ac >> pMobIndex->gold;
         fread_to_eol( stream );

         pMobIndex->exp = -1; // Converts to AFKMud auto-calc exp
      }
      else if( key == "Stats2" )
      {
         int dummyval;
         stream >> dummyval >> dummyval;

         // Converts to AFKMUd standard ld8 formula.
         pMobIndex->hitnodice = pMobIndex->level;
         pMobIndex->hitsizedice = 8;

         stream >> pMobIndex->hitplus;
      }
      else if( key == "Stats3" )
         stream >> pMobIndex->damnodice >> pMobIndex->damsizedice >> pMobIndex->damplus;
      else if( key == "Stats4" )
         stream >> pMobIndex->height >> pMobIndex->weight >> pMobIndex->numattacks >> pMobIndex->hitroll >> pMobIndex->damroll;
      else if( key == "Attribs" )
         stream >> pMobIndex->perm_str >> pMobIndex->perm_int >> pMobIndex->perm_wis >> pMobIndex->perm_dex >> pMobIndex->perm_con >> pMobIndex->perm_cha >> pMobIndex->perm_lck;
      else if( key == "Saves" ) // Unused - this is autocalced in mobindex.cpp
         fread_to_eol( stream );
      else if( key == "Speaks" )
      {
         flag_set( stream, pMobIndex->speaks, lang_names );

         if( pMobIndex->speaks.none(  ) )
            pMobIndex->speaks.set( LANG_COMMON );
      }
      else if( key == "Speaking" )
      {
         std::string speaking, flag;
         int value;

         speaking = fread_line( stream );

         speaking = one_argument( speaking, flag );
         value = get_langnum( flag );
         if( value < 0 || value >= LANG_UNKNOWN )
            bug( "Unknown speaking language: {}", flag );
         else
            pMobIndex->speaking = value;

         if( !pMobIndex->speaking )
            pMobIndex->speaking = LANG_COMMON;
      }
      else if( key == "Bodyparts" )
         flag_set( stream, pMobIndex->body_parts, part_flags );
      else if( key == "Resist" )
         flag_set( stream, pMobIndex->resistant, ris_flags );
      else if( key == "Immune" )
         flag_set( stream, pMobIndex->immune, ris_flags );
      else if( key == "Suscept" )
         flag_set( stream, pMobIndex->susceptible, ris_flags );
      else if( key == "Attacks" )
         flag_set( stream, pMobIndex->attacks, attack_flags );
      else if( key == "Defenses" )
         flag_set( stream, pMobIndex->defenses, defense_flags );
      else if( key == "ShopData" )
      {
         shop_data *pShop = new shop_data;

         pShop->keeper = pMobIndex->vnum;
         for( int iTrade = 0; iTrade < MAX_TRADE; ++iTrade )
            stream >> pShop->buy_type[iTrade];
         stream >> pShop->profit_buy >> pShop->profit_sell;

         pShop->profit_buy = urange( pShop->profit_sell + 5, pShop->profit_buy, 1000 );
         pShop->profit_sell = urange( 0, pShop->profit_sell, pShop->profit_buy - 5 );

         stream >> pShop->open_hour;
         stream >> pShop->close_hour;

         pMobIndex->pShop = pShop;
         shoplist.push_back( pShop );
         ++top_shop;
      }
      else if( key == "RepairData" )
      {
         repair_data *rShop = new repair_data;

         rShop->keeper = pMobIndex->vnum;
         for( int iFix = 0; iFix < MAX_FIX; ++iFix )
            stream >> rShop->fix_type[iFix];

         stream >> rShop->profit_fix >> rShop->shop_type >> rShop->open_hour >> rShop->close_hour;

         pMobIndex->rShop = rShop;
         repairlist.push_back( rShop );
         ++top_repair;
      }
      else if( key == "#MUDPROG" ) // We can cheat here since the prog formats are identical :)
      {
         mud_prog_data *mprg = new mud_prog_data;
         fread_afk_mudprog( stream, mprg, pMobIndex );
         pMobIndex->mudprogs.push_back( mprg );
         ++top_prog;
      }
      else if( key == "#ENDMOBILE" )
      {
         if( !oldmob )
         {
            mob_index_table.insert( std::map<int, mob_index *>::value_type( pMobIndex->vnum, pMobIndex ) );
            tarea->mobs.push_back( pMobIndex );
            ++top_mob_index;
         }
         return;
      }
      else
      {
         bug( "{}: Bad section '{}' in FUSS mobiles - skipping.", __func__, key );
         fread_to_eol( stream );
      }
   }
}

void fread_fuss_areadata( std::ifstream & stream, area_data * tarea )
{
   std::string key;
   while( stream >> key )
   {
      if( key == "Version" )
         stream >> tarea->version;
      else if( key == "Name" )
         tarea->name = fread_line( stream );
      else if( key == "Author" )
         tarea->author = fread_line( stream );
      else if( key == "Credits" )
         tarea->credits = fread_line( stream );
      else if( key == "WeatherX" )
         stream >> tarea->weatherx;
      else if( key == "WeatherY" )
         stream >> tarea->weathery;
      else if( key == "Ranges" )
         stream >> tarea->low_soft_range >> tarea->hi_soft_range >> tarea->low_hard_range >> tarea->hi_hard_range;
      else if( key == "SpellLimit" ) // Spelllimit field is unused by AFKMud - skip it.
         fread_to_eol( stream );
      else if( key == "Economy" ) // Economy field is unused by AFKMud - skip it.
         fread_to_eol( stream );
      else if( key == "ResetMsg" )
         tarea->resetmsg = fread_line( stream );
      else if( key == "ResetFreq" )
         stream >> tarea->reset_frequency;
      else if( key == "Flags" )
         flag_set( stream, tarea->flags, area_flags );
      else if( key == "#ENDAREADATA" )
      {
         tarea->age = tarea->reset_frequency;
         return;
      }
      else
      {
         bug( "{}: Bad section '{}' in FUSS area header - skipping.", __func__, key );
         fread_to_eol( stream );
      }
   }
}

area_data *fread_smaugfuss_area( std::ifstream & stream )
{
   area_data *tarea = nullptr;

   std::string key;
   while( stream >> key )
   {
      if( key == "#AREADATA" )
      {
         tarea = create_area(  );
         tarea->filename = strArea;
         fread_fuss_areadata( stream, tarea );
      }
      else if( key == "#MOBILE" )
         fread_fuss_mobile( stream, tarea );
      else if( key == "#OBJECT" )
         fread_fuss_object( stream, tarea );
      else if( key == "#ROOM" )
         fread_fuss_room( stream, tarea );
      else if( key == "#ENDAREA" )
         break;
      else
      {
         bug( "{}: Bad section header: {}", __func__, key );
         fread_to_eol( stream );
      }
   }
   return tarea;
}
