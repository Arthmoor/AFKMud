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
 *                     SmaugFUSS Zone Conversion Support                    *
 ****************************************************************************/

// Converts SmaugFUSS version 1+ areas into AFKMud format.

#include <sys/stat.h>
#include "mud.h"
#include "area.h"
#include "areaconvert.h"
#include "mobindex.h"
#include "mud_prog.h"
#include "objindex.h"
#include "roomindex.h"
#include "shops.h"

extern int top_prog;
extern int top_affect;
extern int top_shop;
extern int top_repair;

bool check_area_conflict( area_data *, int, int );
void fread_afk_exit( FILE *, room_index * );
extra_descr_data *fread_afk_exdesc( FILE * );

/*
 * There should be 34 entries here to match the size of the sector_types list in olc.h.
 * They should be listed in the same order they appear in sec_flags in build.c for SmaugFUSS.
 * Don't fill in the unused slots unless SmaugFUSS plops a new value into one of them. [Last checked: January 2025]
 */
const char *fuss_sec_flags[] = {
   "inside", "city", "field", "forest", "hills", "mountain", "water_swim",           // 6
   "water_noswim", "underwater", "air", "desert", "dunno", "oceanfloor",             // 12
   "underground", "lava", "swamp", "UNUSED", "UNUSED", "UNUSED", "UNUSED", "UNUSED", // 20
   "UNUSED", "UNUSED", "UNUSED", "UNUSED", "UNUSED", "UNUSED", "UNUSED", "UNUSED",   // 28
   "UNUSED", "UNUSED", "UNUSED", "UNUSED", "UNUSED", "UNUSED"                        // 34
};

// This table needs to stay in sync with SmaugFUSS to remain effective.
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

// This table needs to stay in sync with SmaugFUSS to remain effective.
const char *fuss_npc_class[] = {
   // Playable Classes
   "mage", "cleric", "thief", "warrior", "UNUSED", "druid", "ranger",
   "UNUSED", "paladin", "UNUSED", "UNUSED", "pc11", "pc12", "pc13",
   "pc14", "pc15", "pc16", "pc17", "pc18", "pc19",

   // NPCs only
   "baker", "butcher", "blacksmith", "mayor", "king", "queen"
};

// Samson, next time you get the bright idea to shorten a flag name, don't do it, ok?
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

// Samson, next time you get the bright idea to shorten a flag name, don't do it, ok?
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

// Samson, next time you get the bright idea to shorten a flag name, don't do it, ok?
const char *fuss_r_flags[] = {
   "dark", "death", "nomob", "indoors", "safe", "nocamp", "nosummon",
   "nomagic", "tunnel", "private", "silence", "nosupplicate", "arena", "nomissile",
   "norecall", "noportal", "noastral", "nodrop", "clanstoreroom", "teleport",
   "teleshowdesc", "nofloor", "solitary", "petshop", "donation", "nodropall",
   "logspeech", "prototype", "noteleport", "noscry", "cave", "cavern", "nobeacon",
   "auction", "map", "forge", "guildinn", "isolated", "watchtower",
   "noquit", "telenofly", "_track_", "noyell", "nowhere", "notrack"
};

int get_fuss_sectypes( const string & sector )
{
   for( int x = 0; x < SECT_MAX; ++x )
      if( !str_cmp( sector, fuss_sec_flags[x] ) )
         return x;
   return -1;
}

int get_fuss_npc_race( const string & type )
{
   for( int x = 0; x < MAX_NPC_RACE; ++x )
      if( !str_cmp( type, fuss_npc_race[x] ) )
         return x;
   return -1;
}

int get_fuss_npc_class( const string & type )
{
   for( int x = 0; x < MAX_NPC_CLASS; ++x )
      if( !str_cmp( type, fuss_npc_class[x] ) )
         return x;
   return -1;
}

affect_data *fread_fuss_affect( FILE * fp, const char *word )
{
   int pafmod;

   affect_data *paf = new affect_data;
   paf->location = APPLY_NONE;
   paf->type = -1;
   paf->duration = -1;
   paf->bit = 0;
   paf->modifier = 0;
   paf->rismod.reset(  );

   if( !strcmp( word, "Affect" ) )
   {
      paf->type = fread_number( fp );
   }
   else
   {
      int sn;

      sn = skill_lookup( fread_word( fp ) );
      if( sn < 0 )
         bug( "%s: unknown skill.", __func__ );
      else
         paf->type = sn;
   }

   paf->duration = fread_number( fp );
   pafmod = fread_number( fp );
   paf->location = fread_number( fp );
   fread_bitvector( fp ); // Bit conversions don't take for this.

   if( paf->location == APPLY_WEAPONSPELL
       || paf->location == APPLY_WEARSPELL
       || paf->location == APPLY_STRIPSN || paf->location == APPLY_REMOVESPELL || paf->location == APPLY_RECURRINGSPELL )
      paf->modifier = slot_lookup( pafmod );
   else
      paf->modifier = pafmod;

   ++top_affect;
   return paf;
}

void fread_fuss_room( FILE * fp, area_data * tarea )
{
   room_index *pRoomIndex = nullptr;
   bool oldroom = false;

   for( ;; )
   {
      const char *word = ( feof( fp ) ? "#ENDROOM" : fread_word( fp ) );

      if( word[0] == '\0' )
      {
         log_printf( "%s: EOF encountered reading file!", __func__ );
         word = "#ENDROOM";
      }

      switch ( word[0] )
      {
         default:
            bug( "%s: no match: %s", __func__, word );
            fread_to_eol( fp );
            break;

         case '#':
            if( !str_cmp( word, "#ENDROOM" ) )
            {
               if( !oldroom )
               {
                  room_index_table.insert( map < int, room_index * >::value_type( pRoomIndex->vnum, pRoomIndex ) );
                  tarea->rooms.push_back( pRoomIndex );
                  ++top_room;
               }
               return;
            }

            // Format of this section is identical to AFKMud. Let's cheat :)
            if( !str_cmp( word, "#EXIT" ) )
            {
               fread_afk_exit( fp, pRoomIndex );
               break;
            }

            // Format of this section is identical to AFKMud. Let's cheat :)
            if( !str_cmp( word, "#EXDESC" ) )
            {
               extra_descr_data *ed = fread_afk_exdesc( fp );
               if( ed )
                  pRoomIndex->extradesc.push_back( ed );
               break;
            }

            // Format of this section is identical to AFKMud. Let's cheat :)
            if( !str_cmp( word, "#MUDPROG" ) )
            {
               mud_prog_data *mprg = new mud_prog_data;
               fread_afk_mudprog( fp, mprg, pRoomIndex );
               pRoomIndex->mudprogs.push_back( mprg );
               ++top_prog;
               break;
            }
            break;

         case 'A':
            if( !str_cmp( word, "Affect" ) || !str_cmp( word, "AffectData" ) )
            {
               affect_data *af = fread_fuss_affect( fp, word );

               if( af )
                  pRoomIndex->permaffects.push_back( af );
               break;
            }
            break;

         case 'D':
            KEY( "Desc", pRoomIndex->roomdesc, fread_string_nohash( fp ) );
            break;

         case 'F':
            if( !str_cmp( word, "Flags" ) )
            {
               flag_set( fp, pRoomIndex->flags, fuss_r_flags );
               break;
            }
            break;

         case 'N':
            KEY( "Name", pRoomIndex->name, fread_string( fp ) );
            break;

         // Format of this section is identical to AFKMud. Let's cheat :)
         case 'R':
            CLKEY( "Reset", pRoomIndex->load_reset( fp, false ) );
            break;

         case 'S':
            if( !str_cmp( word, "Sector" ) )
            {
               const char *sec_type = fread_flagstring( fp );
               int sector = get_fuss_sectypes( sec_type );

               if( sector < 0 || sector >= SECT_MAX )
               {
                  bug( "%s: Room #%d has bad sector type: %s", __func__, pRoomIndex->vnum, sec_type );
                  sector = 1;
               }

               pRoomIndex->sector_type = sector;
               pRoomIndex->winter_sector = -1;
               break;
            }

            if( !str_cmp( word, "Stats" ) )
            {
               const char *ln = fread_line( fp );
               int x1, x2, x3, x4;

               x1 = x2 = x3 = x4 = 0;
               sscanf( ln, "%d %d %d %d", &x1, &x2, &x3, &x4 );

               pRoomIndex->tele_delay = x1;
               pRoomIndex->tele_vnum = x2;
               pRoomIndex->tunnel = x3;
               pRoomIndex->max_weight = x4;

               break;
            }
            break;

         case 'V':
            if( !str_cmp( word, "Vnum" ) )
            {
               bool tmpBootDb = fBootDb;
               fBootDb = false;

               int vnum = fread_number( fp );

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
                     log_string( "This is a fatal error. Program terminated." );
                     exit( 1 );
                  }
               }

               if( get_room_index( vnum ) )
               {
                  if( tmpBootDb )
                  {
                     fBootDb = tmpBootDb;
                     bug( "%s: vnum %d duplicated.", __func__, vnum );

                     // Try to recover, read to end of duplicated room and then bail out
                     for( ;; )
                     {
                        word = feof( fp ) ? "#ENDROOM" : fread_word( fp );

                        if( !str_cmp( word, "#ENDROOM" ) )
                           return;
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
                     list < reset_data * >::iterator rst;

                     bug( "%s: WARNING: resets already exist for this room.", __func__ );
                     for( rst = pRoomIndex->resets.begin(  ); rst != pRoomIndex->resets.end(  ); ++rst )
                     {
                        reset_data *rtmp = *rst;

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
                     log_printf_plus( LOG_BUILD, sysdata->build_level, "Cleaning resets: %s", tarea->name );
                     pRoomIndex->clean_resets(  );
                  }
               }
               break;
            }
            break;
      }
   }
}

void fread_fuss_object( FILE * fp, area_data * tarea )
{
   obj_index *pObjIndex = nullptr;
   bool oldobj = false;

   for( ;; )
   {
      const char *word = ( feof( fp ) ? "#ENDOBJECT" : fread_word( fp ) );

      if( word[0] == '\0' )
      {
         log_printf( "%s: EOF encountered reading file!", __func__ );
         word = "#ENDOBJECT";
      }

      switch ( word[0] )
      {
         default:
            bug( "%s: no match: %s", __func__, word );
            fread_to_eol( fp );
            break;

         case '#':
            if( !str_cmp( word, "#ENDOBJECT" ) )
            {
               if( !oldobj )
               {
                  obj_index_table.insert( map < int, obj_index * >::value_type( pObjIndex->vnum, pObjIndex ) );
                  tarea->objects.push_back( pObjIndex );
                  ++top_obj_index;
               }
               return;
            }

            // Format of this section is identical to AFKMud. Let's cheat :)
            if( !str_cmp( word, "#EXDESC" ) )
            {
               extra_descr_data *ed = fread_afk_exdesc( fp );
               if( ed )
                  pObjIndex->extradesc.push_back( ed );
               break;
            }

            // Format of this section is identical to AFKMud. Let's cheat :)
            if( !str_cmp( word, "#MUDPROG" ) )
            {
               mud_prog_data *mprg = new mud_prog_data;
               fread_afk_mudprog( fp, mprg, pObjIndex );
               pObjIndex->mudprogs.push_back( mprg );
               ++top_prog;
               break;
            }
            break;

         case 'A':
            if( !str_cmp( word, "Action" ) )
            {
               const char *desc = fread_flagstring( fp );

               if( desc && desc[0] != '\0' && str_cmp( desc, "(null)" ) )
                  pObjIndex->action_desc = STRALLOC( desc );
               break;
            }

            if( !str_cmp( word, "Affect" ) || !str_cmp( word, "AffectData" ) )
            {
               affect_data *af = fread_fuss_affect( fp, word );

               if( af )
                  pObjIndex->affects.push_back( af );
               break;
            }
            break;

         case 'F':
            if( !str_cmp( word, "Flags" ) )
            {
               flag_set( fp, pObjIndex->extra_flags, fuss_o_flags );
               break;
            }
            break;

         case 'K':
            KEY( "Keywords", pObjIndex->name, fread_string( fp ) );
            break;

         case 'L':
            if( !str_cmp( word, "Long" ) )
            {
               const char *desc = fread_flagstring( fp );

               if( desc && desc[0] != '\0' && str_cmp( desc, "(null)" ) )
                  pObjIndex->objdesc = STRALLOC( desc );
               break;
            }
            break;

         case 'S':
            KEY( "Short", pObjIndex->short_descr, fread_string( fp ) );

            if( !str_cmp( word, "Spells" ) )
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
               break;
            }

            if( !str_cmp( word, "Stats" ) )
            {
               const char *ln = fread_line( fp );
               int x1, x2, x3, x4, x5;

               x1 = x2 = x3 = x5 = 0;
               x4 = 9999;
               sscanf( ln, "%d %d %d %d %d", &x1, &x2, &x3, &x4, &x5 );

               pObjIndex->weight = x1;
               pObjIndex->weight = UMAX( 1, pObjIndex->weight );
               pObjIndex->cost = x2;
               pObjIndex->ego = x3;
               pObjIndex->level = x4;
               pObjIndex->layers = x5;

               pObjIndex->socket[0] = STRALLOC( "None" );
               pObjIndex->socket[1] = STRALLOC( "None" );
               pObjIndex->socket[2] = STRALLOC( "None" );

               if( pObjIndex->ego >= sysdata->minego )
               {
                  log_printf( "Item %d gaining new rare item limit of 1", pObjIndex->vnum );
                  pObjIndex->limit = 1;   /* Sets new limit since stock zones won't have one */
               }
               else
                  pObjIndex->limit = 9999;   /* Default value, this should more than insure that the shit loads */

               pObjIndex->ego = -2;

               break;
            }
            break;

         case 'T':
            if( !str_cmp( word, "Type" ) )
            {
               int value = get_otype( fread_flagstring( fp ) );

               if( value < 0 )
               {
                  bug( "%s: vnum %d: Object has invalid type! Defaulting to trash.", __func__, pObjIndex->vnum );
                  value = get_otype( "trash" );
               }
               pObjIndex->item_type = value;
               break;
            }
            break;

         case 'V':
            if( !str_cmp( word, "Values" ) )
            {
               const char *ln = fread_line( fp );
               int x1, x2, x3, x4, x5, x6;
               x1 = x2 = x3 = x4 = x5 = x6 = 0;

               sscanf( ln, "%d %d %d %d %d %d", &x1, &x2, &x3, &x4, &x5, &x6 );

               pObjIndex->value[0] = x1;
               pObjIndex->value[1] = x2;
               pObjIndex->value[2] = x3;
               pObjIndex->value[3] = x4;
               pObjIndex->value[4] = x5;
               pObjIndex->value[5] = x6;

               break;
            }

            if( !str_cmp( word, "Vnum" ) )
            {
               bool tmpBootDb = fBootDb;
               fBootDb = false;

               int vnum = fread_number( fp );

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
                     log_string( "This is a fatal error. Program terminated." );
                     exit( 1 );
                  }
               }

               if( get_obj_index( vnum ) )
               {
                  if( tmpBootDb )
                  {
                     fBootDb = tmpBootDb;
                     bug( "%s: vnum %d duplicated.", __func__, vnum );

                     // Try to recover, read to end of duplicated object and then bail out
                     for( ;; )
                     {
                        word = feof( fp ) ? "#ENDOBJECT" : fread_word( fp );

                        if( !str_cmp( word, "#ENDOBJECT" ) )
                           return;
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
               break;
            }
            break;

         case 'W':
            if( !str_cmp( word, "WFlags" ) )
            {
               flag_set( fp, pObjIndex->wear_flags, w_flags );
               break;
            }
            break;
      }
   }
}

void fread_fuss_mobile( FILE * fp, area_data * tarea )
{
   mob_index *pMobIndex = nullptr;
   bool oldmob = false;

   for( ;; )
   {
      const char *word = ( feof( fp ) ? "#ENDMOBILE" : fread_word( fp ) );

      if( word[0] == '\0' )
      {
         log_printf( "%s: EOF encountered reading file!", __func__ );
         word = "#ENDMOBILE";
      }

      switch ( word[0] )
      {
         default:
            log_printf( "%s: no match: %s", __func__, word );
            fread_to_eol( fp );
            break;

         case '#':
            // We can cheat here since the prog formats are identical :)
            if( !str_cmp( word, "#MUDPROG" ) )
            {
               mud_prog_data *mprg = new mud_prog_data;
               fread_afk_mudprog( fp, mprg, pMobIndex );
               pMobIndex->mudprogs.push_back( mprg );
               ++top_prog;
               break;
            }

            if( !str_cmp( word, "#ENDMOBILE" ) )
            {
               if( !oldmob )
               {
                  mob_index_table.insert( map < int, mob_index * >::value_type( pMobIndex->vnum, pMobIndex ) );
                  tarea->mobs.push_back( pMobIndex );
                  ++top_mob_index;
               }
               return;
            }
            break;

         case 'A':
            if( !str_cmp( word, "Actflags" ) )
            {
               flag_set( fp, pMobIndex->actflags, fuss_act_flags );
               break;
            }

            if( !str_cmp( word, "Affected" ) )
            {
               flag_set( fp, pMobIndex->affected_by, aff_flags );
               break;
            }

            if( !str_cmp( word, "Attacks" ) )
            {
               flag_set( fp, pMobIndex->attacks, attack_flags );
               break;
            }

            if( !str_cmp( word, "Attribs" ) )
            {
               const char *ln = fread_line( fp );
               int x1, x2, x3, x4, x5, x6, x7;

               x1 = x2 = x3 = x4 = x5 = x6 = x7 = 0;
               sscanf( ln, "%d %d %d %d %d %d %d", &x1, &x2, &x3, &x4, &x5, &x6, &x7 );

               pMobIndex->perm_str = x1;
               pMobIndex->perm_int = x2;
               pMobIndex->perm_wis = x3;
               pMobIndex->perm_dex = x4;
               pMobIndex->perm_con = x5;
               pMobIndex->perm_cha = x6;
               pMobIndex->perm_lck = x7;

               break;
            }
            break;

         case 'B':
            if( !str_cmp( word, "Bodyparts" ) )
            {
               flag_set( fp, pMobIndex->body_parts, part_flags );
               break;
            }
            break;

         case 'C':
            if( !str_cmp( word, "Class" ) )
            {
               const char *class_word = fread_flagstring( fp );
               short Class = get_fuss_npc_class( class_word );

               if( Class < 0 || Class >= MAX_NPC_CLASS )
               {
                  bug( "%s: vnum %d: Mob has invalid class: %s. Defaulting to warrior.", __func__, pMobIndex->vnum, class_word );
                  Class = get_npc_class( "warrior" );
               }

               pMobIndex->Class = Class;
               break;
            }
            break;

         case 'D':
            if( !str_cmp( word, "Defenses" ) )
            {
               flag_set( fp, pMobIndex->defenses, defense_flags );
               break;
            }

            if( !str_cmp( word, "DefPos" ) )
            {
               short position = get_npc_position( fread_flagstring( fp ) );

               if( position < 0 || position >= POS_MAX )
               {
                  bug( "%s: vnum %d: Mobile in invalid default position! Defaulting to standing.", __func__, pMobIndex->vnum );
                  position = POS_STANDING;
               }
               pMobIndex->defposition = position;
               break;
            }

            if( !str_cmp( word, "Desc" ) )
            {
               const char *desc = fread_flagstring( fp );

               if( desc && desc[0] != '\0' && str_cmp( desc, "(null)" ) )
                  pMobIndex->chardesc = STRALLOC( desc );
               break;
            }
            break;

         case 'G':
            if( !str_cmp( word, "Gender" ) )
            {
               short sex = get_npc_sex( fread_flagstring( fp ) );

               if( sex < 0 || sex > SEX_FEMALE )
               {
                  bug( "%s: vnum %d: Mobile has invalid sex! Defaulting to neuter.", __func__, pMobIndex->vnum );
                  sex = SEX_NEUTRAL;
               }
               pMobIndex->sex = sex;
               break;
            }
            break;

         case 'I':
            if( !str_cmp( word, "Immune" ) )
            {
               flag_set( fp, pMobIndex->immune, ris_flags );
               break;
            }
            break;

         case 'K':
            KEY( "Keywords", pMobIndex->player_name, fread_string( fp ) );
            break;

         case 'L':
            KEY( "Long", pMobIndex->long_descr, fread_string( fp ) );
            break;

         case 'P':
            if( !str_cmp( word, "Position" ) )
            {
               short position = get_npc_position( fread_flagstring( fp ) );

               if( position < 0 || position >= POS_MAX )
               {
                  bug( "%s: vnum %d: Mobile in invalid position! Defaulting to standing.", __func__, pMobIndex->vnum );
                  position = POS_STANDING;
               }
               pMobIndex->position = position;
               break;
            }
            break;

         case 'R':
            if( !str_cmp( word, "Race" ) )
            {
               const char *race_word = fread_flagstring( fp );
               short race = get_fuss_npc_race( race_word );

               if( race < 0 || race >= MAX_NPC_RACE )
               {
                  bug( "%s: vnum %d: Mob has invalid race: %s Defaulting to monster.", __func__, pMobIndex->vnum, race_word );
                  race = get_npc_race( "monster" );
               }

               pMobIndex->race = race;
               break;
            }

            if( !str_cmp( word, "RepairData" ) )
            {
               int iFix;
               repair_data *rShop = new repair_data;

               rShop->keeper = pMobIndex->vnum;
               for( iFix = 0; iFix < MAX_FIX; ++iFix )
                  rShop->fix_type[iFix] = fread_number( fp );
               rShop->profit_fix = fread_number( fp );
               rShop->shop_type = fread_number( fp );
               rShop->open_hour = fread_number( fp );
               rShop->close_hour = fread_number( fp );

               pMobIndex->rShop = rShop;
               repairlist.push_back( rShop );
               ++top_repair;

               break;
            }

            if( !str_cmp( word, "Resist" ) )
            {
               flag_set( fp, pMobIndex->resistant, ris_flags );
               break;
            }
            break;

         case 'S':
            // Unused - this is autocalced in mobindex.cpp
            if( !str_cmp( word, "Saves" ) )
            {
               fread_line( fp );
               break;
            }

            KEY( "Short", pMobIndex->short_descr, fread_string( fp ) );

            if( !str_cmp( word, "ShopData" ) )
            {
               int iTrade;
               shop_data *pShop = new shop_data;

               pShop->keeper = pMobIndex->vnum;
               for( iTrade = 0; iTrade < MAX_TRADE; ++iTrade )
                  pShop->buy_type[iTrade] = fread_number( fp );
               pShop->profit_buy = fread_number( fp );
               pShop->profit_sell = fread_number( fp );
               pShop->profit_buy = URANGE( pShop->profit_sell + 5, pShop->profit_buy, 1000 );
               pShop->profit_sell = URANGE( 0, pShop->profit_sell, pShop->profit_buy - 5 );
               pShop->open_hour = fread_number( fp );
               pShop->close_hour = fread_number( fp );

               pMobIndex->pShop = pShop;
               shoplist.push_back( pShop );
               ++top_shop;

               break;
            }

            if( !str_cmp( word, "Speaks" ) )
            {
               flag_set( fp, pMobIndex->speaks, lang_names );

               if( pMobIndex->speaks.none(  ) )
                  pMobIndex->speaks.set( LANG_COMMON );
               break;
            }

            if( !str_cmp( word, "Speaking" ) )
            {
               string speaking, flag;
               int value;

               speaking = fread_flagstring( fp );

               speaking = one_argument( speaking, flag );
               value = get_langnum( flag );
               if( value < 0 || value >= LANG_UNKNOWN )
                  bug( "Unknown speaking language: %s", flag.c_str() );
               else
                  pMobIndex->speaking = value;

               if( !pMobIndex->speaking )
                  pMobIndex->speaking = LANG_COMMON;
               break;
            }

            if( !str_cmp( word, "Specfun" ) )
            {
               const char *temp = fread_flagstring( fp );

               if( !pMobIndex )
               {
                  bug( "%s: Specfun: Invalid mob vnum!", __func__ );
                  break;
               }
               if( !( pMobIndex->spec_fun = m_spec_lookup( temp ) ) )
               {
                  bug( "%s: Specfun: vnum %d, no spec_fun called %s.", __func__, pMobIndex->vnum, temp );
                  pMobIndex->spec_funname.clear(  );
               }
               else
                  pMobIndex->spec_funname = temp;
               break;
            }

            if( !str_cmp( word, "Stats1" ) )
            {
               const char *ln = fread_line( fp );
               int x1, x2, x3, x4, x5, x6;

               x1 = x2 = x3 = x4 = x5 = x6 = 0;
               sscanf( ln, "%d %d %d %d %d %d", &x1, &x2, &x3, &x4, &x5, &x6 );

               pMobIndex->alignment = x1;
               pMobIndex->level = x2;
               pMobIndex->mobthac0 = x3;
               pMobIndex->ac = x4;
               pMobIndex->gold = x5;
               pMobIndex->exp = -1; // Converts to AFKMud auto-calc exp

               break;
            }

            if( !str_cmp( word, "Stats2" ) )
            {
               const char *ln = fread_line( fp );
               int x1, x2, x3;
               x1 = x2 = x3 = 0;
               sscanf( ln, "%d %d %d", &x1, &x2, &x3 );

               pMobIndex->hitnodice = pMobIndex->level;
               pMobIndex->hitsizedice = 8;
               pMobIndex->hitplus = x3;

               break;
            }

            if( !str_cmp( word, "Stats3" ) )
            {
               const char *ln = fread_line( fp );
               int x1, x2, x3;
               x1 = x2 = x3 = 0;
               sscanf( ln, "%d %d %d", &x1, &x2, &x3 );

               pMobIndex->damnodice = x1;
               pMobIndex->damsizedice = x2;
               pMobIndex->damplus = x3;

               break;
            }

            if( !str_cmp( word, "Stats4" ) )
            {
               const char *ln = fread_line( fp );
               int x1, x2, x3, x4, x5;

               x1 = x2 = x3 = x4 = x5 = 0;
               sscanf( ln, "%d %d %d %d %d", &x1, &x2, &x3, &x4, &x5 );

               pMobIndex->height = x1;
               pMobIndex->weight = x2;
               pMobIndex->numattacks = x3;
               pMobIndex->hitroll = x4;
               pMobIndex->damroll = x5;

               break;
            }

            if( !str_cmp( word, "Suscept" ) )
            {
               flag_set( fp, pMobIndex->susceptible, ris_flags );
               break;
            }
            break;

         case 'V':
            if( !str_cmp( word, "Vnum" ) )
            {
               bool tmpBootDb = fBootDb;
               fBootDb = false;

               int vnum = fread_number( fp );

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
                     log_string( "This is a fatal error. Program terminated." );
                     exit( 1 );
                  }
               }

               if( get_mob_index( vnum ) )
               {
                  if( tmpBootDb )
                  {
                     fBootDb = tmpBootDb;
                     bug( "%s: vnum %d duplicated.", __func__, vnum );

                     // Try to recover, read to end of duplicated mobile and then bail out
                     for( ;; )
                     {
                        word = feof( fp ) ? "#ENDMOBILE" : fread_word( fp );

                        if( !str_cmp( word, "#ENDMOBILE" ) )
                           return;
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
               break;
            }
            break;
      }
   }
}

void fread_fuss_areadata( FILE * fp, area_data * tarea )
{
   for( ;; )
   {
      const char *word = ( feof( fp ) ? "#ENDAREADATA" : fread_word( fp ) );

      if( word[0] == '\0' )
      {
         log_printf( "%s: EOF encountered reading file!", __func__ );
         word = "#ENDAREADATA";
      }

      switch ( word[0] )
      {
         default:
            log_printf( "%s: no match: %s", __func__, word );
            fread_to_eol( fp );
            break;

         case '#':
            if( !str_cmp( word, "#ENDAREADATA" ) )
            {
               tarea->age = tarea->reset_frequency;
               return;
            }
            break;

         case 'A':
            KEY( "Author", tarea->author, fread_string( fp ) );
            break;

         case 'C':
            KEY( "Credits", tarea->credits, fread_string( fp ) );
            break;

         case 'E':
            // Economy field is unused by AFKMud - skip it.
            if( !str_cmp( word, "Economy" ) )
            {
               fread_number( fp );
               fread_number( fp );
               break;
            }
            break;

         case 'F':
            if( !str_cmp( word, "Flags" ) )
            {
               flag_set( fp, tarea->flags, area_flags );
               break;
            }
            break;

         case 'N':
            KEY( "Name", tarea->name, fread_string_nohash( fp ) );
            break;

         case 'R':
            if( !str_cmp( word, "Ranges" ) )
            {
               int x1, x2, x3, x4;
               const char *ln = fread_line( fp );

               x1 = x2 = x3 = x4 = 0;
               sscanf( ln, "%d %d %d %d", &x1, &x2, &x3, &x4 );

               tarea->low_soft_range = x1;
               tarea->hi_soft_range = x2;
               tarea->low_hard_range = x3;
               tarea->hi_hard_range = x4;

               break;
            }
            KEY( "ResetMsg", tarea->resetmsg, fread_string_nohash( fp ) );
            KEY( "ResetFreq", tarea->reset_frequency, fread_short( fp ) );
            break;

         case 'S':
            // Spelllimit field is unused by AFKMud - skip it.
            if( !str_cmp( word, "Spelllimit" ) )
            {
               fread_number( fp );
               break;
            }
            break;

         case 'V':
            KEY( "Version", tarea->version, fread_number( fp ) );
            break;

         case 'W':
            KEY( "WeatherX", tarea->weatherx, fread_number( fp ) );
            KEY( "WeatherY", tarea->weathery, fread_number( fp ) );
            break;
      }
   }
}

area_data *fread_smaugfuss_area( FILE * fp )
{
   area_data *tarea = nullptr;

   for( ;; )
   {
      char letter;
      const char *word;

      letter = fread_letter( fp );
      if( letter == '*' )
      {
         fread_to_eol( fp );
         continue;
      }

      if( letter != '#' )
      {
         bug( "%s: # not found. Invalid format.", __func__ );
         if( fBootDb )
            exit( 1 );
         break;
      }

      word = ( feof( fp ) ? "ENDAREA" : fread_word( fp ) );

      if( word[0] == '\0' )
      {
         bug( "%s: EOF encountered reading file!", __func__ );
         word = "ENDAREA";
      }

      if( !str_cmp( word, "AREADATA" ) )
      {
         tarea = create_area(  );
         tarea->filename = strdup( strArea );
         fread_fuss_areadata( fp, tarea );
      }
      else if( !str_cmp( word, "MOBILE" ) )
         fread_fuss_mobile( fp, tarea );
      else if( !str_cmp( word, "OBJECT" ) )
         fread_fuss_object( fp, tarea );
      else if( !str_cmp( word, "ROOM" ) )
         fread_fuss_room( fp, tarea );
      else if( !str_cmp( word, "ENDAREA" ) )
         break;
      else
      {
         bug( "%s: Bad section header: %s", __func__, word );
         fread_to_eol( fp );
      }
   }
   return tarea;
}
