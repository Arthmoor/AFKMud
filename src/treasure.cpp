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
 *                       Rune/Gem Socketing Module                          *
 *                 Inspired by the system used in Diablo 2                  *
 *             Also contains the random treasure creation code              *
 ****************************************************************************/

#include <filesystem>
#include <fstream>
#include "mud.h"
#include "area.h"
#include "clans.h"
#include "mobindex.h"
#include "objindex.h"
#include "roomindex.h"
#include "smaugaffect.h"
#include "treasure.h"

int ncommon, nrare, nurare;
extern int top_affect;

std::list<rune_data *> runelist;
std::list<runeword_data *> rwordlist;
std::vector<weapontable *> w_table;

/*
 * AGH! *BONK BONK BONK* Why didn't any of us think of THIS before!? So much NICER!
 * This table is also used in generating weapons.
 * If you edit this table, adjust TMAT_MAX in treasure.h by the number of entries you add/remove.
 */
const struct armorgenM materials[] = {

   // Material   Material Name   Weight Mod   AC Mod   WD Mod   Cost Mod   Minlevel   Maxlevel

   {0, "Not Defined", 0, 0, 0, 0, 0, 0},
   {1, "Iron ", 1.25, 0, 0, 0.9, 1, 20},
   {2, "Steel ", 1, 0, 0, 1, 5, 50},   /* Steel is the baseline value */
   {3, "Bronze ", 1, -1, -1, 1.1, 1, 25},
   {4, "Dwarven Steel ", 1, 0, 1, 1.4, 10, 50},
   {5, "Silver ", 1, -2, -2, 2, 15, 80},
   {6, "Gold ", 2, -4, -4, 4, 20, 100},
   {7, "Elven Steel ", 0.5, 0, 0, 5, 10, 50},
   {8, "Mithril ", 0.75, 2, 1, 5, 20, 80},
   {9, "Titanium ", 0.25, 2, 1, 5, 30, 90},
   {10, "Adamantine ", 0.5, 4, 2, 7, 40, 100},
   {11, "Blackmite ", 1.2, 5, 3, 7, 60, 100},
   {12, "Stone ", 3, 3, -1, 2, 1, 20},
   {13, "", 1, 0, 0, 1, 1, 100}  // Generic non-descript material, should always be the last one in the table
};

/* If you edit this table, adjust TATP_MAX in treasure.h by the number of entries you add/remove. */
const struct armorgenT armor_type[] = {

   // Type    Name   Base Weight   Base AC   Base Cost   Minlevel   Maxlevel   Flags

   {0, "Not Defined", 0, 0, 0, 0, 0, ""},
   {1, "Padded", 2, 1, 200, 1, 10, "organic"},
   {2, "Leather", 3, 2, 500, 5, 30, "antimage antinecro antimonk organic"},
   {3, "Hide", 5, 4, 500, 20, 100, "antimage antinecro antimonk organic"},
   {4, "Studded Leather", 5, 3, 700, 10, 70, "antimage antinecro antimonk organic"},
   {5, "Chainmail", 8, 5, 750, 15, 100, "antidruid antimage antinecro antimonk metal"},
   {6, "Splintmail", 13, 6, 800, 10, 60, "antidruid antimage antinecro antimonk antibard anticleric antirogue metal"},
   {7, "Ringmail", 10, 3, 1000, 1, 15, "antidruid antimage antinecro antimonk metal"},
   {8, "Scalemail", 13, 4, 1200, 5, 20, "antidruid antimage antinecro antimonk metal"},
   {9, "Banded Mail", 12, 6, 2000, 10, 60, "antidruid antimage antinecro antimonk antibard anticleric antirogue metal"},
   {10, "Platemail", 14, 8, 6000, 20, 100, "antidruid antimage antinecro antimonk antibard anticleric antirogue metal"},
   {11, "Field Plate", 10, 9, 20000, 30, 100, "antidruid antimage antinecro antimonk antibard anticleric antirogue metal"},
   {12, "Full Plate", 12, 10, 40000, 40, 100, "antidruid antimage antinecro antimonk antibard anticleric antirogue metal"},
   {13, "Buckler", 3, 3, 200, 1, 100, ""},
   {14, "Small Shield", 5, 7, 500, 5, 100, "antidruid antimage antinecro antimonk"},
   {15, "Medium Shield", 5, 10, 1000, 15, 100, "antidruid antimage antinecro antimonk antibard antirogue"},
   {16, "Body Shield", 10, 15, 1500, 20, 100, "antidruid antimage antinecro antimonk antibard antirogue"}
};

/* If you edit this table, adjust TWTP_MAX in treasure.h by the number of entries you add/remove. */
const struct weaponT weapon_type[] = {

   // Type   Name   Base damage   Weight   Cost   Skill   Damage Type   Flags

   {0, "Not Defined", 0, 0, 0, 0, 0, "brittle"},
   {1, "Dagger", 4, 4, 200, WEP_DAGGER, DAM_PIERCE, "anticleric antimonk metal"},
   {2, "Claw", 5, 4, 300, WEP_TALON, DAM_SLASH, "anticleric antimonk metal"},
   {3, "Shortsword", 6, 6, 500, WEP_SWORD, DAM_PIERCE, "anticleric antimonk antimage antinecro antidruid metal"},
   {4, "Longsword", 8, 10, 800, WEP_SWORD, DAM_SLASH, "anticleric antimonk antimage antinecro antidruid metal"},
   {5, "Claymore", 10, 20, 1600, WEP_SWORD, DAM_SLASH,
    "anticleric antimonk antimage antinecro antidruid antibard antirogue twohand metal"},
   {6, "Mace", 8, 12, 700, WEP_MACE, DAM_CRUSH, "antimonk antimage antinecro antirogue metal"},
   {7, "Maul", 10, 24, 1500, WEP_MACE, DAM_CRUSH, "antimonk antimage antinecro antirogue twohand metal"},
   {8, "Staff", 7, 8, 600, WEP_STAFF, DAM_CRUSH, "twohand"},
   {9, "Axe", 9, 14, 800, WEP_AXE, DAM_HACK, "anticleric antimonk antimage antinecro antidruid antirogue metal"},
   {10, "War Axe", 11, 26, 1600, WEP_AXE, DAM_HACK,
    "anticleric antimonk antimage antinecro antidruid antirogue twohand metal"},
   {11, "Spear", 7, 10, 600, WEP_SPEAR, DAM_THRUST, "antimage anticleric antinecro twohand"},
   {12, "Pike", 9, 15, 900, WEP_SPEAR, DAM_THRUST, "anticleric antimonk antimage antinecro antidruid antirogue twohand"},
   {13, "Whip", 5, 2, 150, WEP_WHIP, DAM_LASH, "antimonk organic"}
};

/* Adjust TQUAL_MAX in treasure.h when editing this table */
const char *weapon_quality[] = {
   "Not Defined", "Average", "Good", "Superb", "Legendary"
};

const char *rarity[] = {
   "Common", "Rare", "Ultrarare"
};

const char *gems1[12] = {
   "Banded Agate", "Eye Agate", "Moss Agate", "Azurite", "Blue Quartz", "Hematite",
   "Lapus Lazuli", "Malachite", "Obsidian", "Rhodochrosite", "Tiger Eye", "Freshwater Pearl"
};

const char *gems2[16] = {
   "Bloodstone", "Carnelian", "Chalcedony", "Chrysoprase", "Citrine", "Iolite", "Jasper",
   "Moonstone", "Peridot", "Quartz", "Sadonyx", "Rose Quartz", "Smokey Quartz", "Star Rose Quartz",
   "Zircon", "Black Zircon"
};

const char *gems3[16] = {
   "Amber", "Amethyst", "Chrysoberyl", "Coral", "Red Garnet", "Brown-Green Garnet", "Jade", "Jet",
   "White Pearl", "Golden Pearl", "Pink Pearl", "Silver Pearl", "Red Spinel", "Red-Brown Spinel",
   "Deep Green Spinel", "Tourmaline"
};

const char *gems4[6] = {
   "Alexandrite", "Aquamarine", "Violet Garnet", "Black Pearl", "Deep Blue Spinel", "Golden Yellow Topaz"
};

const char *gems5[7] = {
   "Emerald", "White Opal", "Black Opal", "Fire Opal", "Blue Sapphire", "Tomb Jade", "Water Opal"
};

const char *gems6[11] = {
   "Bright Green Emerald", "Blue-White Diamond", "Pink Diamond", "Brown Diamond", "Blue Diamond",
   "Jacinth", "Black Sapphire", "Ruby", "Star Ruby", "Blue Star Sapphire", "Black Star Sapphire"
};

weapontable::weapontable(  )
{
}

weapontable::~weapontable(  )
{
}

void free_weapon_table( void )
{
   for( auto it = w_table.begin(); it != w_table.end(); )
   {
      weapontable *wt = *it;
      ++it;

      deleteptr( wt );
   }
}

void save_weapontable(  )
{
   std::ofstream stream( std::filesystem::path{WTYPE_FILE} );
   if( !stream.is_open() )
   {
      bug( "{}: Cannot open {} for writing: {}", __func__, WTYPE_FILE, std::strerror(errno) );
      return;
   }

   for( auto* wt : w_table )
   {
      stream << "#WTYPE\n";
      stream << std::format( "Name    {}\n", wt->name );
      stream << std::format( "Type    {}\n", wt->type );
      stream << std::format( "BaseDam {}\n", wt->basedam );
      stream << std::format( "Weight  {}\n", wt->weight );
      stream << std::format( "Cost    {}\n", wt->cost );
      stream << std::format( "Skill   {}\n", wt->skill );
      stream << std::format( "DamType {}\n", wt->damtype );
      stream << std::format( "Flags   {}\n", wt->flags );
      stream << "End\n\n";
   }
   stream.close(  );
   if( stream.fail() )
      bug( "{}: Error occurred after closing {}: ", __func__, WTYPE_FILE, std::strerror(errno) );
}

void load_weapontable(  )
{
   std::ifstream stream( std::filesystem::path{WTYPE_FILE} );
   if( !stream.is_open() )
   {
      bug( "{}: Cannot open {} for reading: {}", __func__, WTYPE_FILE, std::strerror(errno) );
      return;
   }

   weapontable *wt = nullptr;
   w_table.clear(  );

   std::string key;
   while( stream >> key )
   {
      if( key == "#WTYPE" )
         wt = new weapontable;
      else if( key == "Name" )
         wt->name = fread_line( stream, '\n' );
      else if( key == "Type" )
         stream >> wt->type;
      else if( key == "BaseDam" )
         stream >> wt->basedam;
      else if( key == "Weight" )
         stream >> wt->weight;
      else if( key == "Cost" )
         stream >> wt->cost;
      else if( key == "Skill" )
         stream >> wt->skill;
      else if( key == "DamType" )
         stream >> wt->damtype;
      else if( key == "Flags" )
         wt->flags = fread_line( stream, '\n' );
      else if( key == "End" )
         w_table.push_back( wt );
      else
         bug( "{}: Bad section '{}' in {} - skipping.", __func__, key, WTYPE_FILE );
   }
   stream.close(  );
}

CMDF( do_wtsave )
{
   save_weapontable(  );
   ch->print( "Done.\r\n" );
}

CMDF( do_wtload )
{
   load_weapontable(  );

   for( auto* wt : w_table )
   {
      ch->print_fmt( "Name {}, Type {}, BaseDam {}, Weight {}, Cost {}, Skill {}, DamType {}, Flags: {}\r\n",
                  wt->name, wt->type, wt->basedam, wt->weight, wt->cost, wt->skill, wt->damtype, wt->flags );
   }
}

rune_data::rune_data(  )
{
}

rune_data::~rune_data(  )
{
   runelist.remove( this );
}

runeword_data::runeword_data(  )
{
}

runeword_data::~runeword_data(  )
{
   rwordlist.remove( this );
}

void free_runedata( void )
{
   for( auto it = runelist.begin(  ); it != runelist.end(  ); )
   {
      rune_data *rune = *it;
      ++it;

      deleteptr( rune );
   }

   for( auto it = rwordlist.begin(  ); it != rwordlist.end(  ); )
   {
      runeword_data *rword = *it;
      ++it;

      deleteptr( rword );
   }
}

short get_rarity( std::string_view name )
{
   for( unsigned int x = 0; x < sizeof( rarity ) / sizeof( rarity[0] ); ++x )
      if( !str_cmp( name, rarity[x] ) )
         return x;
   return -1;
}

runeword_data *pick_runeword(  )
{
   int wordpick = number_range( 1, rwordlist.size(  ) );
   int counter = 1;

   for( auto* rword : rwordlist )
   {
      if( counter == wordpick )
         return rword;
   }
   return nullptr;
}

void load_runewords( void )
{
   log_string( "Loading runewords..." );

   std::ifstream stream( std::filesystem::path{RUNEWORD_FILE} );
   if( !stream.is_open( ) )
   {
      bug( "{}: Cannot open {} for reading: {}", __func__, RUNEWORD_FILE, std::strerror(errno) );
      return;
   }

   rwordlist.clear(  );
   runeword_data *rword = nullptr;

   std::string key;
   while( stream >> key )
   {
      if( key == "#RWORD" )
         rword = new runeword_data;
      else if( key == "Name" )
         rword->set_name( fread_line( stream, '\n' ) );
      else if( key == "Type" )
      {
         int value;

         stream >> value;
         rword->set_type( value );
      }
      else if( key == "Rune1" )
         rword->set_rune1( fread_line( stream, '\n' ) );
      else if( key == "Rune2" )
         rword->set_rune2( fread_line( stream, '\n' ) );
      else if( key == "Rune3" )
         rword->set_rune3( fread_line( stream, '\n' ) );
      else if( key == "Stat1" )
         stream >> rword->stat1[0] >> rword->stat1[1];
      else if( key == "Stat2" )
         stream >> rword->stat2[0] >> rword->stat2[1];
      else if( key == "Stat3" )
         stream >> rword->stat3[0] >> rword->stat3[1];
      else if( key == "Stat4" )
         stream >> rword->stat4[0] >> rword->stat4[1];
      else if( key == "End" )
      {
         bool found = false;
         std::list<runeword_data *>::iterator rw;

         for( rw = rwordlist.begin(  ); rw != rwordlist.end(  ); ++rw )
         {
            runeword_data *rwd = *rw;

            if( rwd->get_name(  ) >= rword->get_name(  ) )
            {
               found = true;
               rwordlist.insert( rw, rword );
               break;
            }
         }
         if( !found )
            rwordlist.push_back( rword );
      }
      else
         log_printf( "{}: Bad line in runewords file: {}", __func__, key );
   }
   stream.close(  );
}

void load_runes( void )
{
   log_string( "Loading runes..." );

   std::ifstream stream( std::filesystem::path{RUNE_FILE} );
   if( !stream.is_open( ) )
   {
      bug( "{}: Cannot open {} for reading: {}", __func__, RUNE_FILE, std::strerror(errno) );
      return;
   }

   runelist.clear();

   rune_data *rune = nullptr;
   std::string key;
   while( stream >> key )
   {
      if( key == "#RUNE" )
         rune = new rune_data;
      else if( key == "Name" )
         rune->set_name( fread_line( stream, '\n' ) );
      else if( key == "Rarity" )
      {
         int value;

         stream >> value;
         rune->set_rarity( value );
      }
      else if( key == "Stat1" )
         stream >> rune->stat1[0] >> rune->stat1[1];
      else if( key == "Stat2" )
         stream >> rune->stat2[0] >> rune->stat2[1];
      else if( key == "End" )
      {
         bool found = false;
         std::list<rune_data *>::iterator rn;

         for( rn = runelist.begin(  ); rn != runelist.end(  ); ++rn )
         {
            rune_data *r = *rn;

            if( r->get_name(  ) >= rune->get_name(  ) )
            {
               found = true;
               runelist.insert( rn, rune );
               break;
            }
         }

         if( !found )
            runelist.push_back( rune );

         switch ( rune->get_rarity(  ) )
         {
            case RUNE_COMMON:
               ncommon += 1;
               break;
            case RUNE_RARE:
               nrare += 1;
               break;
            case RUNE_ULTRARARE:
               nurare += 1;
               break;
            default:
               break;
         }
      }
      else
         log_printf( "{}: Bad line in runes file: {}", __func__, key );
   }
   stream.close(  );

   load_runewords(  );
}

void save_runes( void )
{
   std::ofstream stream( std::filesystem::path{RUNE_FILE} );
   if( !stream.is_open() )
   {
      bug( "{}: Cannot open {} for writing: {}", __func__, RUNE_FILE, std::strerror(errno) );
      return;
   }

   for( auto* rune : runelist )
   {
      stream << "#RUNE\n";
      stream << std::format( "Name     {}\n", rune->get_name(  ) );
      stream << std::format( "Rarity   {}\n", rune->get_rarity(  ) );
      stream << std::format( "Stat1    {} {}\n", rune->stat1[0], rune->stat1[1] );
      stream << std::format( "Stat2    {} {}\n", rune->stat2[0], rune->stat2[1] );
      stream << "End\n\n";
   }
   stream.close(  );
   if( stream.fail() )
      bug( "{}: Error occurred after closing {}: ", __func__, RUNE_FILE, std::strerror(errno) );
}

rune_data *check_rune( std::string_view name )
{
   for( auto* rune : runelist )
   {
      if( !str_cmp( rune->get_name(  ), name ) )
         return rune;
   }
   return nullptr;
}

CMDF( do_makerune )
{
   rune_data *rune = nullptr;

   if( ch->isnpc(  ) )
   {
      ch->print( "NPCs can't use this command.\r\n" );
      return;
   }

   if( argument.empty(  ) )
   {
      ch->print( "Syntax: makerune <name>\r\n" );
      return;
   }

   if( ( rune = check_rune( argument ) ) != nullptr )
   {
      ch->print_fmt( "A rune called {} already exists. Choose another name.\r\n", argument );
      return;
   }

   rune = new rune_data;
   rune->set_name( argument );

   bool found = false;
   std::list<rune_data *>::iterator rn;
   for( rn = runelist.begin(  ); rn != runelist.end(  ); ++rn )
   {
      rune_data *r = *rn;

      if( r->get_name(  ) >= rune->get_name(  ) )
      {
         found = true;
         runelist.insert( rn, rune );
         break;
      }
   }
   if( !found )
      runelist.push_back( rune );

   ch->print_fmt( "New rune {} has been created.\r\n", argument );
   save_runes(  );
}

CMDF( do_destroyrune )
{
   rune_data *rune = nullptr;

   if( ch->isnpc(  ) )
   {
      ch->print( "NPCs can't use this command.\r\n" );
      return;
   }

   if( argument.empty(  ) )
   {
      ch->print( "Syntax: destroyrune <name>\r\n" );
      return;
   }

   if( !( rune = check_rune( argument ) ) )
   {
      ch->print_fmt( "No rune called {} exists.\r\n", argument );
      return;
   }

   deleteptr( rune );
   save_runes(  );

   ch->print_fmt( "Rune {} has been destroyed.\r\n", argument );
}

CMDF( do_setrune )
{
   std::string arg, arg2, arg3;
   rune_data *rune;

   if( ch->isnpc(  ) )
   {
      ch->print( "NPCs can't use this command.\r\n" );
      return;
   }

   argument = one_argument( argument, arg );
   argument = one_argument( argument, arg2 );
   argument = one_argument( argument, arg3 );
   if( arg.empty(  ) || arg2.empty(  ) || arg3.empty(  ) )
   {
      ch->print( "Syntax: setrune <rune_name> <field> <value> [second value]\r\n" );
      ch->print( "Field can be one of the following:\r\n" );
      ch->print( "name rarity stat1 stat2\r\n" );
      return;
   }

   if( !( rune = check_rune( arg ) ) )
   {
      ch->print_fmt( "No rune named {} exists.\r\n", arg );
      return;
   }

   if( !str_cmp( arg2, "name" ) )
   {
      rune_data *newrune;

      if( ( newrune = check_rune( arg3 ) ) != nullptr )
      {
         ch->print_fmt( "A rune named {} already exists. Choose a new name.\r\n", arg3 );
         return;
      }
      rune->set_name( arg3 );
      save_runes(  );
      ch->print_fmt( "Rune {} has been renamed as {}\r\n", arg, arg3 );
      return;
   }

   if( !str_cmp( arg2, "rarity" ) )
   {
      short value = get_rarity( arg3 );

      if( value < 0 || value > RUNE_ULTRARARE )
      {
         ch->print_fmt( "{} is an invalid rarity.\r\n", arg3 );
         return;
      }
      rune->set_rarity( value );
      save_runes(  );
      ch->print_fmt( "{} rune is now {} rarity.\r\n", rune->get_name(  ), rarity[value] );
      return;
   }

   if( !str_cmp( arg2, "stat1" ) )
   {
      int value = get_atype( arg3 );

      if( value < 0 )
      {
         ch->print_fmt( "{} is an invalid stat to apply.\r\n", arg3 );
         return;
      }

      if( argument.empty(  ) )
      {
         do_setrune( ch, "" );
         return;
      }

      if( value == APPLY_AFFECT )
      {
         int val2 = get_aflag( argument );

         if( val2 < 0 || val2 >= MAX_AFFECTED_BY )
         {
            ch->print_fmt( "{} is an invalid affect.\r\n", argument );
            return;
         }
         rune->stat1[0] = value;
         rune->stat1[1] = val2;
         save_runes(  );
         ch->print_fmt( "{} rune now confers: {} {}\r\n", rune->get_name(  ), arg3, argument );
         return;
      }

      if( value == APPLY_RESISTANT || value == APPLY_IMMUNE || value == APPLY_SUSCEPTIBLE || value == APPLY_ABSORB )
      {
         int val2 = get_risflag( argument );

         if( val2 < 0 || val2 >= MAX_RIS_FLAG )
         {
            ch->print_fmt( "{} is an invalid RISA flag.\r\n", argument );
            return;
         }
         rune->stat1[0] = value;
         rune->stat1[1] = val2;
         save_runes(  );
         ch->print_fmt( "{} rune now confers: {} {}\r\n", rune->get_name(  ), arg3, argument );
         return;
      }

      if( value == APPLY_WEAPONSPELL || value == APPLY_REMOVESPELL || value == APPLY_WEARSPELL )
      {
         int val2 = skill_lookup( argument );

         if( !IS_VALID_SN( val2 ) )
         {
            ch->print_fmt( "Invalid skill/spell: {}", argument );
            return;
         }
         rune->stat1[0] = value;
         rune->stat1[1] = skill_table[val2]->slot;
         save_runes(  );
         ch->print_fmt( "{} rune now confers: {} {}\r\n", rune->get_name(  ), arg3, argument );
         return;
      }

      if( !is_number( argument ) )
      {
         ch->print( "Apply modifier must be numerical.\r\n" );
         return;
      }
      rune->stat1[0] = value;
      rune->stat1[1] = std::stoi( argument );
      save_runes(  );
      ch->print_fmt( "{} rune now confers: {} {}\r\n", rune->get_name(  ), arg3, argument );
      return;
   }

   if( !str_cmp( arg2, "stat2" ) )
   {
      int value = get_atype( arg3 );

      if( value < 0 )
      {
         ch->print_fmt( "{} is an invalid stat to apply.\r\n", arg3 );
         return;
      }

      if( argument.empty(  ) )
      {
         do_setrune( ch, "" );
         return;
      }

      if( value == APPLY_AFFECT )
      {
         int val2 = get_aflag( argument );

         if( val2 < 0 || val2 >= MAX_AFFECTED_BY )
         {
            ch->print_fmt( "{} is an invalid affect.\r\n", argument );
            return;
         }
         rune->stat2[0] = value;
         rune->stat2[1] = val2;
         save_runes(  );
         ch->print_fmt( "{} rune now confers: {} {}\r\n", rune->get_name(  ), arg3, argument );
         return;
      }

      if( value == APPLY_RESISTANT || value == APPLY_IMMUNE || value == APPLY_SUSCEPTIBLE || value == APPLY_ABSORB )
      {
         int val2 = get_risflag( argument );

         if( val2 < 0 || val2 >= MAX_RIS_FLAG )
         {
            ch->print_fmt( "{} is an invalid RISA flag.\r\n", argument );
            return;
         }
         rune->stat2[0] = value;
         rune->stat2[1] = val2;
         save_runes(  );
         ch->print_fmt( "{} rune now confers: {} {}\r\n", rune->get_name(  ), arg3, argument );
         return;
      }

      if( value == APPLY_WEAPONSPELL || value == APPLY_REMOVESPELL || value == APPLY_WEARSPELL )
      {
         int val2 = skill_lookup( argument );

         if( !IS_VALID_SN( val2 ) )
         {
            ch->print_fmt( "Invalid skill/spell: {}", argument );
            return;
         }
         rune->stat2[0] = value;
         rune->stat2[1] = skill_table[val2]->slot;
         save_runes(  );
         ch->print_fmt( "{} rune now confers: {} {}\r\n", rune->get_name(  ), arg3, argument );
         return;
      }

      if( !is_number( argument ) )
      {
         ch->print( "Apply modifier must be numerical.\r\n" );
         return;
      }
      rune->stat2[0] = value;
      rune->stat2[1] = std::stoi( argument );
      save_runes(  );
      ch->print_fmt( "{} rune now confers: {} {}\r\n", rune->get_name(  ), arg3, argument );
      return;
   }
}

/* Ok Tarl, you've convinced me this is needed :) */
CMDF( do_loadrune )
{
   rune_data *rune;
   obj_data *obj;

   if( argument.empty(  ) )
   {
      ch->print( "Load which rune? Use showrunes to display the list.\r\n" );
      return;
   }

   if( !( rune = check_rune( argument ) ) )
   {
      ch->print_fmt( "{} does not exist.\r\n", argument );
      return;
   }

   if( !( obj = get_obj_index( OBJ_VNUM_RUNE )->create_object( ch->level ) ) )
   {
      log_printf( "create_object: {}:{}, line {}.", __FILE__, __func__, __LINE__ );
      ch->print( "&RGeneric rune item is MISSING! Report to Samson.\r\n" );
      return;
   }
   obj->name = std::format( "{} rune", rune->get_name(  ) );
   obj->short_descr = std::format( "{} Rune", rune->get_name(  ) );
   obj->objdesc = std::format( "A magical {} Rune lies here pulsating.", rune->get_name(  ) );
   obj->value[0] = rune->stat1[0];
   obj->value[1] = rune->stat1[1];
   obj->value[2] = rune->stat2[0];
   obj->value[3] = rune->stat2[1];
   obj->to_char( ch );
   ch->print_fmt( "You now have a {} Rune.\r\n", rune->get_name(  ) );
}

/* Edited by Tarl 2 April 02 for alphabetical display */
/* Modified again by Samson for the same purpose only done differently :) */
CMDF( do_showrunes )
{
   int total = 0;

   if( runelist.empty(  ) )
   {
      ch->print( "There are no runes created yet.\r\n" );
      return;
   }

   ch->pager( "Currently created runes:\r\n\r\n" );
   ch->pager( " Rune    Rarity        Stat1       Mod1      Stat2       Mod2\r\n" );
   ch->pager( "------+----------+---------------+------+--------------+------\r\n" );

   if( argument.empty(  ) )
   {
      for( auto* rune : runelist )
      {
         ch->pager_fmt( "{:<6.6} {:<10.10} {:<15.15} {:<6} {:<15.15} {}\r\n", rune->get_name(  ), rarity[rune->get_rarity(  )],
                     a_types[rune->stat1[0]], rune->stat1[1], a_types[rune->stat2[0]], rune->stat2[1] );
         ++total;
      }
      ch->pager_fmt( "{} total runes displayed.\r\n", total );
   }
   else
   {
      for( auto* rune : runelist )
      {
         if( !str_prefix( argument, rune->get_name(  ) ) )
         {
            ch->pager_fmt( "{:<6.6} {:<10.10} {:<15.15} {:<6} {:<15.15} {:<6}\r\n", rune->get_name(  ), rarity[rune->get_rarity(  )],
                        a_types[rune->stat1[0]], rune->stat1[1], a_types[rune->stat2[0]], rune->stat2[1] );
            ++total;
         }
      }
      ch->pager_fmt( "{} total runes displayed.\r\n", total );
   }
}

CMDF( do_runewords )
{
   int total = 0;

   if( rwordlist.empty(  ) )
   {
      ch->print( "There are no runewords created yet.\r\n" );
      return;
   }

   ch->pager( "Currently created runewords:\r\n\r\n" );
   ch->pager_fmt( "{:<13.13} {:<6.6} {:<12.12} {:<6.6} {:<12.12} {:<6.6} {:<12.12} {:<6.6} {:<12.12} {:<6.6}\r\n",
               "Word", "Type", "Stat1", "Mod1", "Stat2", "Mod2", "Stat3", "Mod3", "Stat4", "Mod4" );

   if( argument.empty(  ) )
   {
      for( auto* rword : rwordlist )
      {
         ch->pager_fmt( "{:<17.17} {:<6.6} {:<12.12} {:<6} {:<12.12} {:<6} {:<12.12} {:<6} {:<12.12} {:<6}\r\n",
                     rword->get_name(  ), rword->get_type(  ) == 0 ? "Armor" : "Weapon",
                     a_types[rword->stat1[0]], rword->stat1[1], a_types[rword->stat2[0]], rword->stat2[1],
                     a_types[rword->stat3[0]], rword->stat3[1], a_types[rword->stat4[0]], rword->stat4[1] );
         ++total;
      }
      ch->pager_fmt( "{} total runewords displayed.\r\n", total );
   }
   else
   {
      for( auto* rword : rwordlist )
      {
         if( !str_prefix( argument, rword->get_name(  ) ) )
         {
            ch->pager_fmt( "{:<10.10} {:<6.6} {:<12.12} {:<6} {:<12.12} {:<6} {:<12.12} {:<6} {:<12.12} {:<6}\r\n",
                        rword->get_name(  ), rword->get_type(  ) == 0 ? "Armor" : "Weapon",
                        a_types[rword->stat1[0]], rword->stat1[1], a_types[rword->stat2[0]], rword->stat2[1],
                        a_types[rword->stat3[0]], rword->stat3[1], a_types[rword->stat4[0]], rword->stat4[1] );
            ++total;
         }
      }
      ch->pager_fmt( "{} total runewords displayed.\r\n", total );
   }
}

obj_data *generate_rune( short level )
{
   short wrune = 0, cap = 0, rare = 0, pick = 0, ccount = 0, rcount = 0, ucount = 0;

   if( level < ( LEVEL_AVATAR * 0.20 ) )
      cap = 80;
   else if( level < ( LEVEL_AVATAR * 0.40 ) )
      cap = 98;
   else
      cap = 100;

   wrune = number_range( 1, cap );

   if( wrune <= 88 )
      rare = RUNE_COMMON;
   else if( wrune <= 98 )
      rare = RUNE_RARE;
   else
      rare = RUNE_ULTRARARE;

   switch ( rare )
   {
      case RUNE_COMMON:
         pick = number_range( 1, ncommon );
         break;
      case RUNE_RARE:
         pick = number_range( 1, nrare );
         break;
      case RUNE_ULTRARARE:
         pick = number_range( 1, nurare );
         break;
      default:
         pick = 1;
         break;
   }

   bool found = false;
   rune_data *rune = nullptr;
   std::list<rune_data *>::iterator rn;
   for( rn = runelist.begin(  ); rn != runelist.end(  ); ++rn )
   {
      rune = *rn;

      switch ( rune->get_rarity(  ) )
      {
         default:
            break;
         case RUNE_COMMON:
            ccount += 1;
            if( ccount == pick && rare == rune->get_rarity(  ) )
               found = true;
            break;
         case RUNE_RARE:
            rcount += 1;
            if( rcount == pick && rare == rune->get_rarity(  ) )
               found = true;
            break;
         case RUNE_ULTRARARE:
            ucount += 1;
            if( ucount == pick && rare == rune->get_rarity(  ) )
               found = true;
            break;
      }
      if( found )
         break;
   }

   if( !found )
      return nullptr;

   obj_data *newrune;
   if( !( newrune = get_obj_index( OBJ_VNUM_RUNE )->create_object( level ) ) )
   {
      log_printf( "create_object: {}:{}, line {}.", __FILE__, __func__, __LINE__ );
      return nullptr;
   }

   newrune->name = std::format( "{} rune", rune->get_name(  ) );
   newrune->short_descr = std::format( "{} Rune", rune->get_name(  ) );
   newrune->objdesc = std::format( "A magical {} Rune lies here pulsating.", rune->get_name(  ) );
   newrune->value[0] = rune->stat1[0];
   newrune->value[1] = rune->stat1[1];
   newrune->value[2] = rune->stat2[0];
   newrune->value[3] = rune->stat2[1];

   return newrune;
}

obj_data *generate_gem( short level )
{
   obj_data *gem;
   std::string gname;
   int cost;
   short gemname, gemtable = number_range( 1, 100 );

   if( !( gem = get_obj_index( OBJ_VNUM_TREASURE )->create_object( level ) ) )
   {
      log_printf( "create_object: {}:{}, line {}.", __FILE__, __func__, __LINE__ );
      return nullptr;
   }

   if( gemtable <= 25 )
   {
      gemname = number_range( 0, 11 );
      gname = gems1[gemname];
      cost = 10;
   }
   else if( gemtable <= 50 )
   {
      gemname = number_range( 0, 15 );
      gname = gems2[gemname];
      cost = 50;
   }
   else if( gemtable <= 70 )
   {
      gemname = number_range( 0, 15 );
      gname = gems3[gemname];
      cost = 100;
   }
   else if( gemtable <= 90 )
   {
      gemname = number_range( 0, 5 );
      gname = gems4[gemname];
      cost = 500;
   }
   else if( gemtable <= 99 )
   {
      gemname = number_range( 0, 6 );
      gname = gems5[gemname];
      cost = 1000;
   }
   else
   {
      gemname = number_range( 0, 10 );
      gname = gems6[gemname];
      cost = 5000;
   }
   gem->item_type = ITEM_TREASURE;
   gem->name = std::format( "gem {}", gname );
   gem->short_descr = gname;
   gem->objdesc = std::format( "A chunk of {} lies here gleaming.", gname );
   gem->cost = cost;
   return gem;
}

void obj_data::weapongen(  )
{
   affect_data *paf;
   std::string eflags, flag;
   int v8, v9, v10, ovalue;
   bool protoflag = false;

   if( item_type != ITEM_WEAPON )
   {
      bug( "{}: Improperly set item passed: {}", __func__, name );
      return;
   }

   if( value[8] >= TWTP_MAX )
   {
      bug( "{}: Improper weapon type passed for {}", __func__, name );
      value[8] = 0;
      return;
   }

   if( value[9] >= TMAT_MAX )
   {
      bug( "{}: Improper material passed for {}", __func__, name );
      value[9] = 0;
      return;
   }

   if( value[10] >= TQUAL_MAX )
   {
      bug( "{}: Improper quality passed for {}", __func__, name );
      value[10] = 0;
      return;
   }

   if( extra_flags.test( ITEM_PROTOTYPE ) )
      protoflag = true;

   v8 = value[8];
   v9 = value[9];
   v10 = value[10];

   weight = ( int )( weapon_type[v8].weight * materials[v9].weight );
   value[0] = value[6] = sysdata->initcond;
   value[1] = umax( 1, v10 + materials[v9].wd );
   value[2] = umax( 1, v10 * ( weapon_type[v8].wd + materials[v9].wd ) );
   value[3] = weapon_type[v8].damage;
   value[4] = weapon_type[v8].skill;
   cost = ( int )( weapon_type[v8].cost * materials[v9].cost );

   eflags = weapon_type[v8].flags;

   while( !eflags.empty(  ) )
   {
      eflags = one_argument( eflags, flag );
      ovalue = get_oflag( flag );
      if( ovalue < 0 || ovalue >= MAX_ITEM_FLAG )
         bug( "{}: Unknown object extraflag: {}", __func__, flag );
      else
         extra_flags.set( ovalue );
   }

   std::list<affect_data *>::iterator paff;
   for( paff = affects.begin(  ); paff != affects.end(  ); )
   {
      affect_data *aff = *paff;
      ++paff;

      affects.remove( aff );
      deleteptr( aff );
      --top_affect;
      continue;
   }

   if( protoflag )
   {
      for( paff = pIndexData->affects.begin(  ); paff != pIndexData->affects.end(  ); )
      {
         affect_data *aff = *paff;
         ++paff;

         pIndexData->affects.remove( aff );
         deleteptr( aff );
         continue;
      }
   }

   wear_flags.set( ITEM_WIELD );
   wear_flags.set( ITEM_TAKE );

   /*
    * And now to adjust the index for the new settings, if needed. 
    */
   if( protoflag )
   {
      pIndexData->weight = weight;
      pIndexData->value[0] = value[0];
      pIndexData->value[1] = value[1];
      pIndexData->value[2] = value[2];
      pIndexData->value[3] = value[3];
      pIndexData->value[4] = value[4];
      pIndexData->cost = cost;
      pIndexData->extra_flags = extra_flags;
      pIndexData->wear_flags = wear_flags;
      extra_flags.set( ITEM_PROTOTYPE );
   }

   if( v9 == 11 ) /* Blackmite save vs spell/stave affect */
   {
      extra_flags.set( ITEM_MAGIC );

      paf = new affect_data;
      paf->type = -1;
      paf->duration = -1;
      paf->location = APPLY_SAVING_SPELL; /* Save vs Spell */
      paf->modifier = -2;
      paf->bit = 0;
      if( protoflag )
         pIndexData->affects.push_back( paf );
      else
      {
         affects.push_back( paf );
         ++top_affect;
      }
   }

   if( value[7] < 2 )
      ego = 0;
   else
      ego = sysdata->minego - 1;

   if( value[7] > 0 )
   {
      extra_flags.set( ITEM_MAGIC );
      name = std::format( "socketed {}{}", materials[v9].name, weapon_type[v8].name );
      short_descr = std::format( "Socketed {}{}", materials[v9].name, weapon_type[v8].name );
      objdesc = std::format( "A socketed {}{} lies here on the ground.", materials[v9].name, weapon_type[v8].name );
   }
   else
   {
      name = std::format( "{}{}", materials[v9].name, weapon_type[v8].name );
      short_descr = std::format( "{}{}", materials[v9].name, weapon_type[v8].name );
      objdesc = std::format( "A {}{} lies here on the ground.", materials[v9].name, weapon_type[v8].name );
   }
}

void obj_data::armorgen(  )
{
   std::string eflags, flag;
   int v3, v4, ovalue;
   bool protoflag = false;

   if( item_type != ITEM_ARMOR )
   {
      bug( "{}: Improperly set item passed: {}", __func__, name );
      return;
   }

   if( value[3] >= TATP_MAX )
   {
      bug( "{}: Improper armor type passed for {}", __func__, name );
      value[3] = 0;
      return;
   }

   if( value[4] >= TMAT_MAX )
   {
      bug( "{}: Improper material passed for {}", __func__, name );
      value[4] = 0;
      return;
   }

   if( extra_flags.test( ITEM_PROTOTYPE ) )
      protoflag = true;

   /*
    * Nice ugly block of class-anti removals and of course metal/organic 
    */
   extra_flags.reset( ITEM_ANTI_CLERIC );
   extra_flags.reset( ITEM_ANTI_MAGE );
   extra_flags.reset( ITEM_ANTI_ROGUE );
   extra_flags.reset( ITEM_ANTI_WARRIOR );
   extra_flags.reset( ITEM_ANTI_BARD );
   extra_flags.reset( ITEM_ANTI_DRUID );
   extra_flags.reset( ITEM_ANTI_MONK );
   extra_flags.reset( ITEM_ANTI_RANGER );
   extra_flags.reset( ITEM_ANTI_PALADIN );
   extra_flags.reset( ITEM_ANTI_NECRO );
   extra_flags.reset( ITEM_ANTI_APAL );
   extra_flags.reset( ITEM_METAL );
   extra_flags.reset( ITEM_ORGANIC );

   wear_flags.set( ITEM_TAKE );

   v3 = value[3];
   v4 = value[4];

   weight = ( int )( armor_type[v3].weight * materials[v4].weight );
   value[0] = value[1] = armor_type[v3].ac + materials[v4].ac;
   cost = ( int )( armor_type[v3].cost * materials[v4].cost );

   eflags = armor_type[v3].flags;

   while( !eflags.empty(  ) )
   {
      eflags = one_argument( eflags, flag );
      ovalue = get_oflag( flag );
      if( ovalue < 0 || ovalue >= MAX_ITEM_FLAG )
         bug( "{}: Unknown object extraflag: {}", __func__, flag );
      else
         extra_flags.set( ovalue );
   }

   std::list<affect_data *>::iterator paff;
   for( paff = affects.begin(  ); paff != affects.end(  ); )
   {
      affect_data *aff = *paff;
      ++paff;

      affects.remove( aff );
      deleteptr( aff );
      --top_affect;
      continue;
   }

   if( protoflag )
   {
      for( paff = pIndexData->affects.begin(  ); paff != pIndexData->affects.end(  ); )
      {
         affect_data *aff = *paff;
         ++paff;

         pIndexData->affects.remove( aff );
         deleteptr( aff );
         continue;
      }
   }

   /*
    * And now to adjust the index for the new settings, if needed. 
    */
   if( protoflag )
   {
      pIndexData->weight = weight;
      pIndexData->value[0] = value[0];
      pIndexData->value[1] = value[1];
      pIndexData->cost = cost;
      pIndexData->extra_flags = extra_flags;
      pIndexData->wear_flags = wear_flags;
      extra_flags.set( ITEM_PROTOTYPE );
   }

   if( v4 == 11 ) /* Blackmite save vs spell/stave affect */
   {
      extra_flags.set( ITEM_MAGIC );

      affect_data *paf = new affect_data;
      paf->type = -1;
      paf->duration = -1;
      paf->location = APPLY_SAVING_SPELL; /* Save vs Spell */
      paf->modifier = -2;
      paf->bit = 0;
      if( protoflag )
         pIndexData->affects.push_back( paf );
      else
      {
         affects.push_back( paf );
         ++top_affect;
      }
   }

   if( value[2] < 2 )
      ego = 0;
   else
      ego = sysdata->minego - 1;

   if( value[2] > 0 )
   {
      extra_flags.set( ITEM_MAGIC );
      name = std::format( "socketed {}{}", materials[v4].name, armor_type[v3].name );
      if( v3 > 12 )
      {
         short_descr = std::format( "Socketed {}{}", materials[v4].name, armor_type[v3].name );
         objdesc = std::format( "A socketed {}{} lies here in a heap.", materials[v4].name, armor_type[v3].name );
      }
      else
      {
         short_descr = std::format( "Socketed {}{} Chestpiece", materials[v4].name, armor_type[v3].name );
         objdesc = std::format( "A socketed {}{} chestpiece lies here in a heap.", materials[v4].name, armor_type[v3].name );
      }
   }
   else
   {
      name = std::format( "{}{}", materials[v4].name, armor_type[v3].name );
      if( v3 > 12 )
      {
         short_descr = std::format( "{}{}", materials[v4].name, armor_type[v3].name );
         objdesc = std::format( "A {}{} lies here in a heap.", materials[v4].name, armor_type[v3].name );
      }
      else
      {
         short_descr = std::format( "{}{} Chestpiece", materials[v4].name, armor_type[v3].name );
         objdesc = std::format( "A {}{} chestpiece lies here in a heap.", materials[v4].name, armor_type[v3].name );
      }
   }
}

// This determines the number of sockets to put on a weapon or a body armor
short num_sockets( short level )
{
   short value = 0;

   if( level < ( LEVEL_AVATAR * 0.20 ) )
      value = number_range( 0, 1 );
   else if( level >= ( LEVEL_AVATAR * 0.21 ) && level <= ( LEVEL_AVATAR * 0.50 ) )
      value = number_range( 0, 2 );
   else if( level >= ( LEVEL_AVATAR * 0.51 ) && level <= ( LEVEL_AVATAR * 0.70 ) )
      value = number_range( 1, 2 );
   else if( level >= ( LEVEL_AVATAR * 0.71 ) && level <= ( LEVEL_AVATAR * 0.90 ) )
      value = number_range( 1, 3 );
   else
      value = number_range( 2, 3 );

   return value;
}

// This determines what material an armor or weapon is made of
short choose_material( short level )
{
   short mval;

   /*
    * Hey look, if it takes more than 50000 iterations to find something.... 
    */
   for( int x = 0; x < 50000; ++x )
   {
      mval = number_range( 1, TMAT_MAX - 1 );
      if( materials[mval].minlevel <= level && materials[mval].maxlevel >= level )
         return mval;
   }
   log_printf( "Notice: {} failed to choose. Setting generic.", __func__ );
   return ( TMAT_MAX - 1 );
}

// This determines what type of armor is generated
short choose_armor( short level )
{
   short mval;

   /*
    * Hey look, if it takes more than 50000 iterations to find something.... 
    */
   for( int x = 0; x < 50000; ++x )
   {
      mval = number_range( 1, TATP_MAX - 1 );
      if( armor_type[mval].minlevel <= level && armor_type[mval].maxlevel >= level )
         return mval;
   }
   log_printf( "Notice: {} failed to choose. Setting hide armor.", __func__ );
   return ( 3 );
}

// This determines the quality of an item being generated
short choose_quality( short level )
{
   short value = 0;

   if( level <= ( LEVEL_AVATAR * 0.10 ) )
      value = 1;
   else if( level >= ( LEVEL_AVATAR * 0.11 ) && level <= ( LEVEL_AVATAR * 0.20 ) )
      value = number_range( 1, 2 );
   else if( level >= ( LEVEL_AVATAR * 0.21 ) && level <= ( LEVEL_AVATAR * 0.50 ) )
      value = 2;
   else if( level >= ( LEVEL_AVATAR * 0.51 ) && level <= ( LEVEL_AVATAR * 0.60 ) )
      value = number_range( 2, 3 );
   else if( level >= ( LEVEL_AVATAR * 0.61 ) && level <= ( LEVEL_AVATAR * 0.90 ) )
      value = 3;
   else
      value = number_range( 3, 4 );

   return value;
}

void make_scroll( obj_data * newitem )
{
   runeword_data *runeword = nullptr;
   std::string name = "Empty", desc = "Empty";
   short value = 0, pick2 = 0;

   // Curse you Rabbit! Look what you've done!
   if( newitem->level <= ( LEVEL_AVATAR * 0.25 ) )
   {
      pick2 = number_range( 1, 8 );

      switch ( pick2 )
      {
         case 1:
            value = skill_lookup( "cure light" );
            name = "scroll cure light";
            desc = "Scroll of Cure Light Wounds";
            break;
         case 2:
            value = skill_lookup( "recall" );
            name = "scroll recall";
            desc = "Scroll of Recall";
            break;
         case 3:
            value = skill_lookup( "identify" );
            name = "scroll identify";
            desc = "Scroll of Identify";
            break;
         case 4:
            value = skill_lookup( "bless" );
            name = "scroll bless";
            desc = "Scroll of Bless";
            break;
         case 5:
            value = skill_lookup( "armor" );
            name = "scroll armor";
            desc = "Scroll of Armor";
            break;
         case 6:
            value = skill_lookup( "refresh" );
            name = "scroll refresh";
            desc = "Scroll of Refresh";
            break;
         case 7:
            value = skill_lookup( "magic missile" );
            name = "scroll magic missile";
            desc = "Scroll of Magic Missile";
            break;
         case 8:
            value = skill_lookup( "burning hands" );
            name = "scroll burning hands";
            desc = "Scroll of Burning Hands";
            break;
         default:
            value = -1;
            name = "scroll blank";
            desc = "Blank Scroll";
            break;
      }
   }
   else if( newitem->level <= ( LEVEL_AVATAR * 0.75 ) )
   {
      pick2 = number_range( 1, 8 );

      switch ( pick2 )
      {
         case 1:
            value = skill_lookup( "cure serious" );
            name = "scroll cure serious";
            desc = "Scroll of Cure Serious Wounds";
            break;
         case 2:
            value = skill_lookup( "invisibility" );
            name = "scroll invisibility";
            desc = "Scroll of Invisibility";
            break;
         case 3:
            value = skill_lookup( "fly" );
            name = "scroll fly";
            desc = "Scroll of Fly";
            break;
         case 4:
            value = skill_lookup( "minor invocation" );
            name = "scroll minor invocation";
            desc = "Scroll of Minor Invocation";
            break;
         case 5:
            value = skill_lookup( "magnetic thrust" );
            name = "scroll magnetic thrust";
            desc = "Scroll of Magnetic Thrust";
            break;
         case 6:
            value = skill_lookup( "colour spray" );
            name = "scroll colour spray";
            desc = "Scroll of Colour Spray";
            break;
         case 7:
            value = -1;
            name = "parchment scroll";
            desc = "Tattered parchment";
            runeword = pick_runeword(  );
            break;
         case 8:
            value = skill_lookup( "stoneskin" );
            name = "scroll stoneskin";
            desc = "Scroll of Stoneskin";
            break;
         default:
            value = -1;
            name = "scroll blank";
            desc = "Blank Scroll";
            break;
      }
   }
   else if( newitem->level <= LEVEL_AVATAR )
   {
      pick2 = number_range( 1, 8 );

      switch ( pick2 )
      {
         case 1:
            value = skill_lookup( "haste" );
            name = "scroll haste";
            desc = "Scroll of Haste";
            break;
         case 2:
            value = skill_lookup( "heal" );
            name = "scroll heal";
            desc = "Scroll of Heal";
            break;
         case 3:
            value = skill_lookup( "rejuvenate" );
            name = "scroll rejuvenate";
            desc = "Scroll of Rejuvenate";
            break;
         case 4:
            value = skill_lookup( "astral walk" );
            name = "scroll astral walk";
            desc = "Scroll of Astral Walk";
            break;
         case 5:
            value = skill_lookup( "indignation" );
            name = "scroll indignation";
            desc = "Scroll of Indignation";
            break;
         case 6:
            value = skill_lookup( "lightning bolt" );
            name = "scroll lightning bolt";
            desc = "Scroll of Lightning Bolt";
            break;
         case 7:
            value = skill_lookup( "fireball" );
            name = "scroll fireball";
            desc = "Scroll of Fireball";
            break;
         case 8:
            value = -1;
            name = "parchment scroll";
            desc = "Tattered parchment";
            runeword = pick_runeword(  );
            break;
         default:
            value = -1;
            name = "scroll blank";
            desc = "Blank Scroll";
            break;
      }
   }

   newitem->item_type = ITEM_SCROLL;
   newitem->value[0] = newitem->level;
   newitem->value[1] = value;
   newitem->value[2] = -1;
   newitem->value[3] = -1;
   newitem->cost = 200;
   newitem->ego = 0;
   newitem->name = name;
   newitem->short_descr = desc;
   newitem->objdesc = "A parchment scroll lies here on the ground.";
   if( runeword )
   {
      extra_descr_data *ed = new extra_descr_data;
      std::string typedesc;

      ed->keyword = "parchment scroll";
      if( runeword->get_type(  ) == 0 )
         typedesc = "\"Weapon\".\n";
      else
         typedesc = "\"Armor\".\n";

      ed->desc = "The scrawlings on this parchment are almost indecipherable. All you can make out are \"";
      ed->desc += runeword->get_rune1(  );
      ed->desc += runeword->get_rune2(  );
      if( !runeword->get_rune3(  ).empty(  ) )
         ed->desc += runeword->get_rune3(  );
      ed->desc += "\" and ";
      ed->desc += typedesc;

      newitem->extradesc.push_back( ed );
   }
}

void make_potion( obj_data * newitem )
{
   std::string name = "Empty", desc = "Empty";
   short value = 0, pick2 = 0;

   // Curse you Rabbit! Look what you've done!
   if( newitem->level <= ( LEVEL_AVATAR * 0.25 ) )
   {
      pick2 = number_range( 1, 8 );

      switch ( pick2 )
      {
         case 1:
            value = skill_lookup( "cure light" );
            name = "potion cure light";
            desc = "Potion of Cure Light Wounds";
            break;
         case 2:
            value = skill_lookup( "bless" );
            name = "potion bless";
            desc = "Potion of Bless";
            break;
         case 3:
            value = skill_lookup( "armor" );
            name = "potion armor";
            desc = "Potion of Armor";
            break;
         case 4:
            value = skill_lookup( "cure blindness" );
            name = "potion cure blind";
            desc = "Potion of Cure Blindness";
            break;
         case 5:
            value = skill_lookup( "cure poison" );
            name = "potion cure poison";
            desc = "Potion of Cure Poison";
            break;
         case 6:
            value = skill_lookup( "aqua breath" );
            name = "potion aqua breath";
            desc = "Potion of Aqua Breath";
            break;
         case 7:
            value = skill_lookup( "refresh" );
            name = "potion refresh";
            desc = "Potion of Refresh";
            break;
         case 8:
            value = skill_lookup( "detect evil" );
            name = "potion detect evil";
            desc = "Potion of Detect Evil";
            break;
         default:
            value = -1;
            name = "flask emtpy";
            desc = "An empty flask";
            break;
      }
   }
   else if( newitem->level <= ( LEVEL_AVATAR * 0.75 ) )
   {
      pick2 = number_range( 1, 8 );

      switch ( pick2 )
      {
         case 1:
            value = skill_lookup( "cure serious" );
            name = "potion cure serious";
            desc = "Potion of Cure Serious Wounds";
            break;
         case 2:
            value = skill_lookup( "invisibility" );
            name = "potion invisibility";
            desc = "Potion of Invisibility";
            break;
         case 3:
            value = skill_lookup( "kindred strength" );
            name = "potion kindred strength";
            desc = "Potion of Kindred Strength";
            break;
         case 4:
            value = skill_lookup( "fly" );
            name = "potion fly";
            desc = "Potion of Fly";
            break;
         case 5:
            value = skill_lookup( "cure poison" );
            name = "potion cure poison";
            desc = "Potion of Cure Poison";
            break;
         case 6:
            value = skill_lookup( "minor invocation" );
            name = "potion minor invocation";
            desc = "Potion of Minor Invocation";
            break;
         case 7:
            value = skill_lookup( "cure blindness" );
            name = "potion cure blind";
            desc = "Potion of Cure Blindness";
            break;
         case 8:
            value = skill_lookup( "refresh" );
            name = "potion refresh";
            desc = "Potion of Refresh";
            break;
         default:
            value = -1;
            name = "flask emtpy";
            desc = "An empty flask";
            break;
      }
   }
   else if( newitem->level <= LEVEL_AVATAR )
   {
      pick2 = number_range( 1, 6 );

      switch ( pick2 )
      {
         case 1:
            value = skill_lookup( "heal" );
            name = "potion heal";
            desc = "Potion of Heal";
            break;
         case 2:
            value = skill_lookup( "haste" );
            name = "potion haste";
            desc = "Potion of Haste";
            break;
         case 3:
            value = skill_lookup( "sanctuary" );
            name = "potion sanctuary";
            desc = "Potion of Sanctuary";
            break;
         case 4:
            value = skill_lookup( "rejuvenate" );
            name = "potion rejuvenate";
            desc = "Potion of Rejuvenate";
            break;
         case 5:
            value = skill_lookup( "indignation" );
            name = "potion indignation";
            desc = "Potion of Indignation";
            break;
         case 6:
            value = skill_lookup( "restore mana" );
            name = "potion restore mana";
            desc = "Potion of Restore Mana";
            break;
         default:
            value = -1;
            name = "flask emtpy";
            desc = "An empty flask";
            break;
      }
   }

   newitem->item_type = ITEM_POTION;
   newitem->value[0] = newitem->level;
   newitem->value[1] = value;
   newitem->value[2] = -1;
   newitem->value[3] = -1;
   newitem->cost = 200;
   newitem->ego = 0;
   newitem->name = name;
   newitem->short_descr = desc;
   newitem->objdesc = "A glass potion flask lies here on the ground.";
}

void make_wand( obj_data * newitem )
{
   std::string name = "Empty", desc = "Empty";
   short value = 0, pick2 = 0;

   // Curse you Rabbit! Look what you've done!
   if( newitem->level <= ( LEVEL_AVATAR * 0.25 ) )
   {
      pick2 = number_range( 1, 4 );

      switch ( pick2 )
      {
         case 1:
            value = skill_lookup( "magic missile" );
            name = "wand magic missile";
            desc = "Wand of Magic Missile";
            break;
         case 2:
            value = skill_lookup( "burning hands" );
            name = "wand burning hands";
            desc = "Wand of Burning Hands";
            break;
         case 3:
            value = skill_lookup( "shield" );
            name = "wand shield";
            desc = "Wand of Shield";
            break;
         case 4:
            value = skill_lookup( "cure light" );
            name = "wand cure light";
            desc = "Wand of Cure Light Wounds";
            break;
         default:
            value = -1;
            name = "wand blank";
            desc = "A blank spell wand";
            break;
      }
   }
   else if( newitem->level <= ( LEVEL_AVATAR * 0.75 ) )
   {
      pick2 = number_range( 1, 4 );

      switch ( pick2 )
      {
         case 1:
            value = skill_lookup( "magnetic thrust" );
            name = "wand magnetic thrust";
            desc = "Wand of Magnetic Thrust";
            break;
         case 2:
            value = skill_lookup( "colour spray" );
            name = "wand colour spray";
            desc = "Wand of Colour Spray";
            break;
         case 3:
            value = skill_lookup( "cure serious" );
            name = "wand cure serious";
            desc = "Wand of Cure Serious Wounds";
            break;
         case 4:
            value = skill_lookup( "stoneskin" );
            name = "wand stoneskin";
            desc = "Wand of Stoneskin";
            break;
         default:
            value = -1;
            name = "wand blank";
            desc = "A blank spell wand";
            break;
      }
   }
   else if( newitem->level <= LEVEL_AVATAR )
   {
      pick2 = number_range( 1, 4 );

      switch ( pick2 )
      {
         case 1:
            value = skill_lookup( "lightning bolt" );
            name = "wand lightning bolt";
            desc = "Wand of Lightning Bolt";
            break;
         case 2:
            value = skill_lookup( "fireball" );
            name = "wand fireball";
            desc = "Wand of Fireball";
            break;
         case 3:
            value = skill_lookup( "indignation" );
            name = "wand indignation";
            desc = "Wand of Indignation";
            break;
         case 4:
            value = skill_lookup( "quantum spike" );
            name = "wand quantum spike";
            desc = "Wand of Quantum Spike";
            break;
         default:
            value = -1;
            name = "wand blank";
            desc = "A blank spell wand";
            break;
      }
   }
   newitem->item_type = ITEM_WAND;
   newitem->wear_flags.set( ITEM_HOLD );
   newitem->value[0] = newitem->level;
   newitem->value[1] = dice( 2, 10 );
   newitem->value[2] = newitem->value[1];
   newitem->value[3] = value;
   newitem->cost = 200;
   newitem->ego = 0;
   newitem->name = name;
   newitem->short_descr = desc;
   newitem->objdesc = "A glowing wand lies here on the ground.";
}

void make_armor( obj_data * newitem )
{
   newitem->item_type = ITEM_ARMOR;

   if( newitem->value[2] < 0 )
      newitem->value[2] = num_sockets( newitem->level ); // Determine the number of sockets to put on the armor.
   if( newitem->value[3] < 0 )
      newitem->value[3] = choose_armor( newitem->level );   // Pick out an armor type.
   if( newitem->value[4] < 0 )
      newitem->value[4] = choose_material( newitem->level );   // Pick out a material for this armor.

   if( newitem->value[3] < 5 )
      newitem->value[4] = TMAT_MAX - 1;   // Sets the generic material value if an organic armor is created.

   if( newitem->value[3] > 12 )
      newitem->wear_flags.set( ITEM_WEAR_SHIELD );
   else
      newitem->wear_flags.set( ITEM_WEAR_BODY );

   newitem->armorgen(  );
}

void make_weapon( obj_data * newitem )
{
   newitem->item_type = ITEM_WEAPON;

   if( newitem->value[7] < 0 )
      newitem->value[7] = num_sockets( newitem->level ); // Determine the number of sockets to put on the weapon.
   if( newitem->value[8] < 0 )
      newitem->value[8] = number_range( 1, TWTP_MAX - 1 );  // Pick out a weapon type.
   if( newitem->value[9] < 0 )
      newitem->value[9] = choose_material( newitem->level );   // Pick out a material for this weapon.
   if( newitem->value[10] < 0 )
      newitem->value[10] = choose_quality( newitem->level );   // Set the quality of this weapon.

   newitem->weapongen(  );
}

obj_data *generate_item( area_data * area, short level )
{
   obj_data *newitem = nullptr;
   short pick = number_range( 1, 100 );

   if( !( newitem = get_obj_index( OBJ_VNUM_TREASURE )->create_object( level ) ) )
   {
      log_printf( "create_object: {}:{}, line {}.", __FILE__, __func__, __LINE__ );
      return nullptr;
   }

   // Make a random scroll
   if( pick <= area->tg_scroll )
      make_scroll( newitem );

   // Make a random potion 
   else if( pick <= area->tg_potion )
      make_potion( newitem );

   // Make a random wand 
   else if( pick <= area->tg_wand )
      make_wand( newitem );

   // Make a random armor 
   else if( pick <= area->tg_armor )
   {
      newitem->value[2] = -1;
      newitem->value[3] = -1;
      newitem->value[4] = -1;
      make_armor( newitem );
   }

   // Make a random weapon
   else
   {
      newitem->value[7] = -1;
      newitem->value[8] = -1;
      newitem->value[9] = -1;
      newitem->value[10] = -1;
      make_weapon( newitem );
   }
   return newitem;
}

/*
 * make some coinage
 */
obj_data *create_money( int amount )
{
   obj_data *obj;

   if( amount <= 0 )
   {
      bug( "{}: zero or negative money {}.", __func__, amount );
      amount = 1;
   }

   if( amount == 1 )
   {
      if( !( obj = get_obj_index( OBJ_VNUM_MONEY_ONE )->create_object( 1 ) ) )
      {
         log_printf( "create_object: {}:{}, line {}.", __FILE__, __func__, __LINE__ );
         return nullptr;
      }
   }
   else
   {
      if( !( obj = get_obj_index( OBJ_VNUM_MONEY_SOME )->create_object( 1 ) ) )
      {
         log_printf( "create_object: {}:{}, line {}.", __FILE__, __func__, __LINE__ );
         return nullptr;
      }
      obj->short_descr = std::vformat( obj->short_descr, std::make_format_args( amount ) );
      obj->value[0] = amount;
   }
   return obj;
}

int make_gold( short level, char_data * ch )
{
   int x, gold, luck = 13;

   if( ch )
      luck = ch->get_curr_lck(  );

   if( level <= ( LEVEL_AVATAR * 0.10 ) )
      x = 30;
   else if( level <= ( LEVEL_AVATAR * 0.20 ) )
      x = 40;
   else if( level <= ( LEVEL_AVATAR * 0.30 ) )
      x = 60;
   else if( level <= ( LEVEL_AVATAR * 0.40 ) )
      x = 90;
   else if( level <= ( LEVEL_AVATAR * 0.50 ) )
      x = 150;
   else if( level <= ( LEVEL_AVATAR * 0.60 ) )
      x = 170;
   else if( level <= ( LEVEL_AVATAR * 0.70 ) )
      x = 200;
   else if( level <= ( LEVEL_AVATAR * 0.80 ) )
      x = 300;
   else if( level <= ( LEVEL_AVATAR * 0.90 ) )
      x = 400;
   else
      x = 500;

   gold = ( dice( level, x ) + ( dice( level, x / 10 ) + dice( luck, x / 3 ) ) );
   return gold;
}

// A slightly butchered way for resets to pick out random junk too
obj_data *generate_random( reset_data * pReset, char_data * mob )
{
   obj_data *newobj = nullptr;
   short picker = pReset->arg1;
   short level = pReset->arg2;

   if( pReset->command == 'W' )
   {
      picker = pReset->arg2;
      level = pReset->arg3;
   }

   if( picker == 0 )
      picker = number_range( 1, 8 );

   switch ( picker )
   {
      default:
      case 1: // Random gold
      {
         int gold = make_gold( level, mob );
         newobj = create_money( gold );
         break;
      }

      case 2: // Random number of gems
         for( int x = 0; x < ( ( level / 25 ) + 1 ); ++x )
            newobj = generate_gem( level );
         break;

      case 3: // Random scroll
         newobj = get_obj_index( OBJ_VNUM_TREASURE )->create_object( level );
         make_scroll( newobj );
         break;

      case 4: // Random potion
         newobj = get_obj_index( OBJ_VNUM_TREASURE )->create_object( level );
         make_potion( newobj );
         break;

      case 5: // Random wand
         newobj = get_obj_index( OBJ_VNUM_TREASURE )->create_object( level );
         make_wand( newobj );
         break;

      case 6: // Random armor
         newobj = get_obj_index( OBJ_VNUM_TREASURE )->create_object( level );
         if( pReset->command == 'W' )
         {
            newobj->value[2] = pReset->arg7;
            newobj->value[3] = pReset->arg4;
            newobj->value[4] = pReset->arg5;
         }
         else
         {
            newobj->value[2] = pReset->arg6;
            newobj->value[3] = pReset->arg3;
            newobj->value[4] = pReset->arg4;
         }
         make_armor( newobj );
         break;

      case 7: // Random weapon
         newobj = get_obj_index( OBJ_VNUM_TREASURE )->create_object( level );
         if( pReset->command == 'W' )
         {
            newobj->value[7] = pReset->arg7;
            newobj->value[8] = pReset->arg4;
            newobj->value[9] = pReset->arg5;
            newobj->value[10] = pReset->arg6;
         }
         else
         {
            newobj->value[7] = pReset->arg6;
            newobj->value[8] = pReset->arg3;
            newobj->value[9] = pReset->arg4;
            newobj->value[10] = pReset->arg5;
         }
         make_weapon( newobj );
         break;

      case 8: // Random rune
         newobj = generate_rune( level );
         break;
   }
   return newobj;
}

void generate_treasure( char_data * ch, obj_data * corpse )
{
   int tchance;
   short level = corpse->level;
   area_data *area = ch->in_room->area;

   /*
    * Rolling for the initial check to see if we should be generating anything at all 
    */
   tchance = number_range( 1, 100 );

   /*
    * 1-20% chance of zilch 
    */
   if( tchance <= area->tg_nothing )
   {
      if( !str_cmp( corpse->name, "corpse random" ) )
         log_string( "Generated nothing" );
      return;
   }

   /*
    * 21-74% of generating gold 
    */
   else if( tchance <= area->tg_gold )
   {
      int gold = make_gold( level, ch );

      gold = gold + ( gold * ( ch->pcdata->exgold / 100 ) );
      create_money( gold )->to_obj( corpse );
      if( !str_cmp( corpse->name, "corpse random" ) )
         log_printf( "Generated {} gold", gold );
      return;
   }
   else if( tchance <= area->tg_item )
   {
      obj_data *item = generate_item( area, level );
      if( !item )
      {
         bug( "{}: Item object failed to create!", __func__ );
         return;
      }
      item->to_obj( corpse );
      if( !str_cmp( corpse->name, "corpse random" ) )
         log_printf( "Generated {}", item->short_descr );
      return;
   }
   else if( tchance <= area->tg_gem )
   {
      for( int x = 0; x < ( ( level / 25 ) + 1 ); ++x )
      {
         obj_data *item = generate_gem( level );
         if( !item )
         {
            bug( "{}: Gem object failed to create!", __func__ );
            return;
         }
         item->to_obj( corpse );
      }
      if( !str_cmp( corpse->name, "corpse random" ) )
         log_string( "Generated gems" );
      return;
   }
   else
   {
      obj_data *item = generate_rune( level );
      if( !item )
      {
         bug( "{}: Rune object failed to create!", __func__ );
         return;
      }
      item->to_obj( corpse );
      if( !str_cmp( corpse->name, "corpse random" ) )
         log_printf( "Generated {}", item->short_descr );
      return;
   }
}

/* Command used to test random treasure drops */
CMDF( do_rttest )
{
   obj_data *corpse;
   std::string arg;
   int mlvl, times, x;

   if( !ch->is_imp(  ) )
   {
      ch->print( "Huh?\r\n" );
      return;
   }

   argument = one_argument( argument, arg );
   if( arg.empty(  ) || argument.empty(  ) )
   {
      ch->print( "Usage: rttest <mob level> <times>\r\n" );
      return;
   }
   if( !is_number( arg ) && !is_number( argument ) )
   {
      ch->print( "Numerical arguments only.\r\n" );
      return;
   }

   mlvl = std::stoi( arg );
   times = std::stoi( argument );

   if( times < 1 )
   {
      ch->print( "Um, yeah. Come on man!\r\n" );
      return;
   }
   if( !( corpse = get_obj_index( OBJ_VNUM_CORPSE_NPC )->create_object( mlvl ) ) )
   {
      log_printf( "create_object: {}:{}, line {}.", __FILE__, __func__, __LINE__ );
      return;
   }
   corpse->name = "corpse random";
   corpse->short_descr = std::vformat( corpse->short_descr, std::make_format_args( "some random thing" ) );
   corpse->objdesc = std::vformat( corpse->objdesc, std::make_format_args( "some random thing" ) );
   corpse->to_room( ch->in_room, ch );

   for( x = 0; x < times; ++x )
      generate_treasure( ch, corpse );
}

void rword_descrips( char_data * ch, obj_data * item, runeword_data * rword )
{
   ch->print_fmt( "&YAs you attach the rune, your {} glows radiantly and becomes {}!\r\n", item->short_descr, rword->get_name(  ) );
   item->name = std::format( "{} {}", item->name, rword->get_name(  ) );
   item->short_descr = rword->get_name(  );
   item->objdesc = std::format( "{} lies here on the ground.", rword->get_name(  ) );
}

void add_rword_affect( obj_data * item, int v1, int v2 )
{
   affect_data *paf;

   if( v1 == 0 )
      return;

   paf = new affect_data;
   paf->type = -1;
   paf->duration = -1;
   paf->location = v1;
   paf->bit = 0;
   if( paf->location == APPLY_WEAPONSPELL || paf->location == APPLY_WEARSPELL
       || paf->location == APPLY_REMOVESPELL || paf->location == APPLY_STRIPSN || paf->location == APPLY_RECURRINGSPELL || paf->location == APPLY_EAT_SPELL )
      paf->modifier = slot_lookup( v2 );
   else
      paf->modifier = v2;
   item->affects.push_back( paf );
   ++top_affect;
}

void check_runewords( char_data * ch, obj_data * item )
{
   std::list<runeword_data *>::iterator irword;

   // Runewords must contain at least 2 runes, so if these first 2 checks fail, bail out. 
   if( item->socket[0].empty() || !str_cmp( item->socket[0], "None" ) )
      return;

   if( item->socket[1].empty() || !str_cmp( item->socket[1], "None" ) )
      return;

   // Only body armors get runewords
   if( item->item_type == ITEM_ARMOR && item->wear_flags.test( ITEM_WEAR_BODY ) )
   {
      for( irword = rwordlist.begin(  ); irword != rwordlist.end(  ); ++irword )
      {
         runeword_data *rword = *irword;

         if( rword->get_type(  ) == 1 )
            continue;

         if( rword->get_rune3(  ).empty(  ) )
         {
            if( !str_cmp( rword->get_rune1(  ), item->socket[0] ) && !str_cmp( rword->get_rune2(  ), item->socket[1] ) )
            {
               add_rword_affect( item, rword->stat1[0], rword->stat1[1] );
               add_rword_affect( item, rword->stat2[0], rword->stat2[1] );
               add_rword_affect( item, rword->stat3[0], rword->stat3[1] );
               add_rword_affect( item, rword->stat4[0], rword->stat4[1] );
               item->value[2] = 0;
               rword_descrips( ch, item, rword );
               return;
            }
            continue;
         }

         if( item->socket[2].empty() || !str_cmp( item->socket[2], "None" ) )
            continue;

         if( !str_cmp( rword->get_rune1(  ), item->socket[0] ) && !str_cmp( rword->get_rune2(  ), item->socket[1] ) && !str_cmp( rword->get_rune3(  ), item->socket[2] ) )
         {
            add_rword_affect( item, rword->stat1[0], rword->stat1[1] );
            add_rword_affect( item, rword->stat2[0], rword->stat2[1] );
            add_rword_affect( item, rword->stat3[0], rword->stat3[1] );
            add_rword_affect( item, rword->stat4[0], rword->stat4[1] );
            item->value[2] = 0;
            rword_descrips( ch, item, rword );
            return;
         }
      }
   }

   // If we fall through to here, it's assumed to be a weapon
   for( irword = rwordlist.begin(  ); irword != rwordlist.end(  ); ++irword )
   {
      runeword_data *rword = *irword;

      if( rword->get_type(  ) == 0 )
         continue;

      if( rword->get_rune3(  ).empty(  ) )
      {
         if( !str_cmp( rword->get_rune1(  ), item->socket[0] ) && !str_cmp( rword->get_rune2(  ), item->socket[1] ) )
         {
            add_rword_affect( item, rword->stat1[0], rword->stat1[1] );
            add_rword_affect( item, rword->stat2[0], rword->stat2[1] );
            add_rword_affect( item, rword->stat3[0], rword->stat3[1] );
            add_rword_affect( item, rword->stat4[0], rword->stat4[1] );
            item->value[7] = 0;
            rword_descrips( ch, item, rword );
            return;
         }
         continue;
      }

      if( item->socket[2].empty() || !str_cmp( item->socket[2], "None" ) )
         continue;

      if( !str_cmp( rword->get_rune1(  ), item->socket[0] ) && !str_cmp( rword->get_rune2(  ), item->socket[1] ) && !str_cmp( rword->get_rune3(  ), item->socket[2] ) )
      {
         add_rword_affect( item, rword->stat1[0], rword->stat1[1] );
         add_rword_affect( item, rword->stat2[0], rword->stat2[1] );
         add_rword_affect( item, rword->stat3[0], rword->stat3[1] );
         add_rword_affect( item, rword->stat4[0], rword->stat4[1] );
         item->value[7] = 0;
         rword_descrips( ch, item, rword );
         return;
      }
   }
}

void add_rune_affect( char_data * ch, obj_data * item, obj_data * rune )
{
   affect_data *paf = new affect_data;
   paf->type = -1;
   paf->duration = -1;
   paf->bit = 0;

   if( item->item_type == ITEM_WEAPON || item->item_type == ITEM_MISSILE_WEAPON )
      paf->location = rune->value[0];
   else
      paf->location = rune->value[2];
   if( paf->location == APPLY_WEAPONSPELL || paf->location == APPLY_WEARSPELL
       || paf->location == APPLY_REMOVESPELL || paf->location == APPLY_STRIPSN || paf->location == APPLY_RECURRINGSPELL || paf->location == APPLY_EAT_SPELL )
   {
      if( item->item_type == ITEM_WEAPON || item->item_type == ITEM_MISSILE_WEAPON )
         paf->modifier = slot_lookup( rune->value[1] );
      else
         paf->modifier = slot_lookup( rune->value[3] );
   }
   else
   {
      if( item->item_type == ITEM_WEAPON || item->item_type == ITEM_MISSILE_WEAPON )
         paf->modifier = rune->value[1];
      else
         paf->modifier = rune->value[3];
   }

   item->affects.push_back( paf );
   rune->separate(  );
   rune->from_char(  );
   rune->extract(  );
   ++top_affect;
   check_runewords( ch, item );
}

CMDF( do_socket )
{
   std::string arg;
   obj_data *rune, *item;

   if( ch->isnpc(  ) )
      return;

   argument = one_argument( argument, arg );
   if( arg.empty(  ) || argument.empty(  ) )
   {
      ch->print( "Usage: socket <rune> <item>\r\n\r\n" );
      ch->print( "Where <rune> is the name of the rune you wish to use.\r\n" );
      ch->print( "Where <item> is the weapon or armor you wish to socket the rune into.\r\n" );
      return;
   }

   if( !( rune = ch->get_obj_carry( arg ) ) )
   {
      ch->print_fmt( "You do not have a {} rune in your inventory!\r\n", arg );
      return;
   }

   if( !( item = ch->get_obj_carry( argument ) ) )
   {
      ch->print_fmt( "You do not have a {} in your inventory!\r\n", argument );
      return;
   }

   if( rune->item_type != ITEM_RUNE )
   {
      ch->print_fmt( "{} is not a rune and cannot be used like that!\r\n", rune->short_descr );
      return;
   }

   item->separate(  );

   if( item->item_type == ITEM_WEAPON || item->item_type == ITEM_MISSILE_WEAPON )
   {
      if( item->value[7] < 1 )
      {
         ch->print_fmt( "{} does not have any free sockets left.\r\n", item->short_descr );
         return;
      }

      if( item->socket[0].empty() || !str_cmp( item->socket[0], "None" ) )
      {
         item->socket[0] = rune->short_descr;
         item->value[7] -= 1;
         ch->print_fmt( "{} glows brightly as the {} rune is inserted and now feels more powerful!\r\n", item->short_descr, rune->short_descr );
         add_rune_affect( ch, item, rune );
         return;
      }

      if( item->socket[1].empty() || !str_cmp( item->socket[1], "None" ) )
      {
         item->socket[1] = rune->short_descr;
         item->value[7] -= 1;
         ch->print_fmt( "{} glows brightly as the {} rune is inserted and now feels more powerful!\r\n", item->short_descr, rune->short_descr );
         add_rune_affect( ch, item, rune );
         return;
      }

      if( item->socket[2].empty() || !str_cmp( item->socket[2], "None" ) )
      {
         item->socket[2] = rune->short_descr;
         item->value[7] -= 1;
         ch->print_fmt( "{} glows brightly as the {} rune is inserted and now feels more powerful!\r\n", item->short_descr, rune->short_descr );
         add_rune_affect( ch, item, rune );
         return;
      }
      bug( "{}: ({}) {} has open sockets, but all sockets are filled?!?", __func__, ch->name, item->short_descr );
      ch->print( "Ooops. Something bad happened. Contact the immortals for assistance.\r\n" );
      return;
   }

   if( item->item_type == ITEM_ARMOR )
   {
      if( item->value[2] < 1 )
      {
         ch->print_fmt( "{} does not have any free sockets left.\r\n", item->short_descr );
         return;
      }

      if( item->socket[0].empty() || !str_cmp( item->socket[0], "None" ) )
      {
         item->socket[0] = rune->short_descr;
         item->value[2] -= 1;
         ch->print_fmt( "{} glows brightly as the {} rune is inserted and now feels more powerful!\r\n", item->short_descr, rune->short_descr );
         add_rune_affect( ch, item, rune );
         return;
      }

      if( item->socket[1].empty() || !str_cmp( item->socket[1], "None" ) )
      {
         item->socket[1] = rune->short_descr;
         item->value[2] -= 1;
         ch->print_fmt( "{} glows brightly as the {} rune is inserted and now feels more powerful!\r\n", item->short_descr, rune->short_descr );
         add_rune_affect( ch, item, rune );
         return;
      }

      if( item->socket[2].empty() || !str_cmp( item->socket[2], "None" ) )
      {
         item->socket[2] = rune->short_descr;
         item->value[2] -= 1;
         ch->print_fmt( "{} glows brightly as the {} rune is inserted and now feels more powerful!\r\n", item->short_descr, rune->short_descr );
         add_rune_affect( ch, item, rune );
         return;
      }
      bug( "{}: ({}) {} has open sockets, but all sockets are filled?!?", __func__, ch->name, item->short_descr );
      ch->print( "Ooops. Something bad happened. Contact the immortals for assistance.\r\n" );
      return;
   }
   ch->print_fmt( "{} cannot be socketed. Only weapons, body armors, and shields are valid.\r\n", item->short_descr );
}

int get_ore( std::string_view ore )
{
   if( !str_cmp( ore, "iron" ) )
      return ORE_IRON;

   if( !str_cmp( ore, "gold" ) )
      return ORE_GOLD;

   if( !str_cmp( ore, "silver" ) )
      return ORE_SILVER;

   if( !str_cmp( ore, "adamantite" ) )
      return ORE_ADAM;

   if( !str_cmp( ore, "mithril" ) )
      return ORE_MITH;

   if( !str_cmp( ore, "blackmite" ) )
      return ORE_BLACK;

   if( !str_cmp( ore, "titanium" ) )
      return ORE_TITANIUM;

   if( !str_cmp( ore, "steel" ) )
      return ORE_STEEL;

   if( !str_cmp( ore, "bronze" ) )
      return ORE_BRONZE;

   if( !str_cmp( ore, "dwarven" ) )
      return ORE_DWARVEN;

   if( !str_cmp( ore, "elven" ) )
      return ORE_ELVEN;

   return -1;
}

/*
 * Written by Samson - 6/2/00
 * Rewritten by Dwip - 12/12/02 (Happy Birthday, me!)
 * Re-rewritten by Tarl 13/12/02 (Happy belated Birthday, Dwip ;)
 * Forge command stuff.  Eliminates the need for forgemob.
 * Utilizes the new armorgen code, and greatly expands the types
 *  of things makable with forge.
 */
CMDF( do_forge )
{
   /*
    * Check to see what sort of flunky the smith is 
    */
   std::list<char_data *>::iterator ich;
   char_data *smith = nullptr;
   bool msmith = false, gsmith = false;

   for( ich = ch->in_room->people.begin(  ); ich != ch->in_room->people.end(  ); ++ich )
   {
      smith = *ich;

      if( smith->has_actflag( ACT_SMITH ) )
      {
         msmith = true; /* We have a mob flagged as a smith */
         break;
      }
      if( smith->has_actflag( ACT_GUILDFORGE ) )
      {
         gsmith = true; /* We have a mob flagged as a guildforge */
         break;
      }
   }

   /*
    * Check for required stuff for PC forging 
    */
   if( msmith == false && gsmith == false )
   {
      /*
       * Check to see we're dealing with a PC, here. 
       */
      if( ch->isnpc(  ) )
      {
         ch->print( "Sorry, NPCs cannot forge items.\r\n" );
         return;
      }

      /*
       * Does the PC have the metallurgy skill? 
       */
      if( ch->level < skill_table[gsn_metallurgy]->race_level[ch->race] )
      {
         ch->print( "Better leave the metallurgy to the experienced smiths.\r\n" );
         return;
      }

      /*
       * Does the PC have actual training in the metallurgy skill? 
       */
      if( ch->pcdata->learned[gsn_metallurgy] < 1 )
      {
         ch->print( "Perhaps you should seek training before attempting this on your own.\r\n" );
         return;
      }

      /*
       * And let's make sure they're in a forge room, too. 
       */
      if( !ch->in_room->flags.test( ROOM_FORGE ) )
      {
         ch->print( "But you are not in a forge!\r\n" );
         return;
      }
   }

   /*
    * Finally, the argument funness. 
    */
   std::string arg, item_type, arg3;
   argument = one_argument( argument, arg );
   argument = one_argument( argument, item_type );
   argument = one_argument( argument, arg3 );

   /*
    * Make sure we got all the args in there 
    */
   if( arg.empty(  ) || item_type.empty(  ) || arg3.empty(  ) )
   {
      ch->print( "Usage: forge <ore type> <item type> <item>\r\n\r\n" );
      ch->print( "Ore type may be one of the following:\r\n" );
      ch->print( "Bronze, Iron, Steel, Silver, Gold, Adamantine, Mithril, Blackmite*, or Titanium*.\r\n\r\n" );
      ch->print( "Item Type may be one of the following types:\r\n" );
      ch->print( "Boots, Leggings, Cuirass, Sleeves, Gauntlets, Helmet, Shield, Weapon.\r\n\r\n" );
      ch->print( "Item may be one of the following if armor: \r\n" );
      ch->print( "Chain, Splint, Ring, Scale, Banded, Plate, Fieldplate, Fullplate, Buckler**,\r\n" );
      ch->print( "Small**, Medium**, Body**, Longsword***, Dagger***, Mace***, Axe***, Claw***\r\n" );
      ch->print( "Shortsword***, Claymore***, Maul***, Staff***, WarAxe***, Spear***, Pike***\r\n\r\n" );
      ch->print( "*Blackmite and Titanium ores can only be worked by Dwarves.\r\n" );
      ch->print( "**For use with Item Type shield only.\r\n" );
      ch->print( "***For use with Item Type weapon only.\r\n" );
      return;
   }

   int ore_type = -1;
   ore_type = get_ore( arg );
   if( ore_type == -1 )
   {
      ch->print_fmt( "{} isn't a valid ore type.\r\n", arg );
      return;
   }

   /*
    * Oh, Dwip.... you thought that get_ore had no purpose? Guess again..... 
    */
   int ore_vnum = 0, material = 0, armor = 0, weapon = 0;
   int base_vnum = OBJ_VNUM_ORE_BASE;  /* All ore vnums must be one after this one */
   switch ( ore_type )
   {
      default:
         bug( "{}: Bad ore value: {}", __func__, ore_type );
         break;

      case ORE_IRON:
         ore_vnum = base_vnum + 1;
         material = 1;  /* this will be value4 of the armorgen */
         break;

      case ORE_SILVER:
         ore_vnum = base_vnum + 3;
         material = 5;
         break;

      case ORE_GOLD:
         ore_vnum = base_vnum + 2;
         material = 6;
         break;

      case ORE_ADAM:
         ore_vnum = base_vnum + 4;
         material = 10;
         break;

      case ORE_MITH:
         ore_vnum = base_vnum + 5;
         material = 8;
         break;

      case ORE_BLACK:
         ore_vnum = base_vnum + 6;
         material = 11;
         break;

      case ORE_TITANIUM:
         ore_vnum = base_vnum + 7;
         material = 9;
         break;

      case ORE_STEEL:
         ore_vnum = base_vnum + 8;
         material = 2;
         break;

      case ORE_BRONZE:
         ore_vnum = base_vnum + 9;
         material = 3;
         break;

      case ORE_DWARVEN:
         ore_vnum = base_vnum + 10;
         material = 12;
         break;

      case ORE_ELVEN:
         ore_vnum = base_vnum + 11;
         material = 13;
         break;
   }

   /*
    * Check to see if forger can work the material in question. 
    */
   if( ore_type == ORE_BLACK || ore_type == ORE_TITANIUM || ore_type == ORE_DWARVEN )
   {
      if( msmith || gsmith )
      {
         if( smith->race != RACE_DWARF )
         {
            interpret( smith, "say I lack the skills to work with that ore. You will have to find someone else." );
            return;
         }
      }
      else
      {
         if( ch->race != RACE_DWARF )
         {
            ch->print( "You lack the skills to work that ore.\r\n" );
            return;
         }
      }
   }

   /*
    * See how much of the specified ore the PC has 
    */
   int orecount = 0, consume = 0;
   for( auto* oreobj : ch->carrying )
   {
      if( oreobj->pIndexData->vnum == ore_vnum )
         orecount += oreobj->count;
   }

   if( orecount < 1 )
   {
      ch->print_fmt( "You have no {} ore to forge an item with!\r\n", arg );
      return;
   }

   /*
    * And now we play with the second argument. 
    */
   int location = 0;
   if( !str_cmp( item_type, "boots" ) )
   {
      if( orecount < 2 )
      {
         ch->print( "You need at least 2 chunks of ore to create boots.\r\n" );
         return;
      }
      consume = 2;
      location = 1;
   }

   if( !str_cmp( item_type, "leggings" ) )
   {
      if( orecount < 3 )
      {
         ch->print( "You need at least 3 chunks of ore to create leggings.\r\n" );
         return;
      }
      consume = 3;
      location = 2;
   }

   if( !str_cmp( item_type, "cuirass" ) )
   {
      if( orecount < 4 )
      {
         ch->print( "You need at least 4 chunks of ore to create a cuirass.\r\n" );
         return;
      }
      consume = 4;
      location = 3;
   }

   if( !str_cmp( item_type, "gauntlets" ) )
   {
      if( orecount < 2 )
      {
         ch->print( "You need at least 2 chunks of ore to create gauntlets.\r\n" );
         return;
      }
      consume = 2;
      location = 4;
   }

   if( !str_cmp( item_type, "sleeves" ) )
   {
      if( orecount < 2 )
      {
         ch->print( "You need at least 2 chunks of ore to create sleeves.\r\n" );
         return;
      }
      consume = 2;
      location = 5;
   }

   if( !str_cmp( item_type, "helmet" ) )
   {
      if( orecount < 3 )
      {
         ch->print( "You need at least 3 chunks of ore to create a helmet.\r\n" );
         return;
      }
      consume = 3;
      location = 6;
   }

   if( !str_cmp( item_type, "shield" ) )
   {
      if( orecount < 3 )
      {
         ch->print( "You need at least 3 chunks of ore to create a shield.\r\n" );
         return;
      }
      consume = 3;
      location = 7;
   }

   if( !str_cmp( item_type, "weapon" ) )
   {
      if( orecount < 2 )
      {
         ch->print( "You need at least 2 chunks of ore to create a weapon.\r\n" );
         return;
      }
      consume = 2;
      location = 8;
   }

   if( consume == 0 )
   {
      ch->print_fmt( "{} is not a valid item type to forge.\r\n", item_type );
      return;
   }

   /*
    * Now to play with argument 3 a bit. 
    */
   if( !str_cmp( arg3, "chain" ) )
   {
      if( location == 7 || location == 8 )
      {
         ch->print( "Armor cannot be of types weapon or shield.\r\n" );
         return;
      }
      if( location == 1 )
      {
         ch->print( "You can't make that out of chainmail...\r\n" );
         return;
      }
      armor = 5;
   }

   if( !str_cmp( arg3, "splint" ) )
   {
      if( location == 7 || location == 8 )
      {
         ch->print( "Armor cannot be of types weapon or shield.\r\n" );
         return;
      }
      if( location == 1 || location == 4 || location == 6 )
      {
         ch->print( "You can't make that out of splintmail...\r\n" );
         return;
      }
      armor = 6;
   }

   if( !str_cmp( arg3, "ring" ) )
   {
      if( location == 7 || location == 8 )
      {
         ch->print( "Armor cannot be of types weapon or shield.\r\n" );
         return;
      }
      if( location == 1 || location == 6 )
      {
         ch->print( "You can't make that out of ringmail...\r\n" );
         return;
      }
      armor = 7;
   }

   if( !str_cmp( arg3, "scale" ) )
   {
      if( location == 7 || location == 8 )
      {
         ch->print( "Armor cannot be of types weapon or shield.\r\n" );
         return;
      }

      if( location == 1 || location == 6 )
      {
         ch->print( "You can't make that out of scalemail...\r\n" );
         return;
      }
      armor = 8;
   }

   if( !str_cmp( arg3, "banded" ) )
   {
      if( location == 7 || location == 8 )
      {
         ch->print( "Armor cannot be of types weapon or shield.\r\n" );
         return;
      }
      if( location == 1 || location == 6 )
      {
         ch->print( "You can't make that out of bandedmail...\r\n" );
         return;
      }
      armor = 9;
   }

   if( !str_cmp( arg3, "plate" ) )
   {
      if( location == 7 || location == 8 )
      {
         ch->print( "Armor cannot be of types weapon or shield.\r\n" );
         return;
      }
      armor = 10;
   }

   if( !str_cmp( arg3, "fieldplate" ) )
   {
      if( location == 7 || location == 8 )
      {
         ch->print( "Armor cannot be of types weapon or shield.\r\n" );
         return;
      }
      armor = 11;
   }

   if( !str_cmp( arg3, "fullplate" ) )
   {
      if( location == 7 || location == 8 )
      {
         ch->print( "Armor cannot be of types weapon or shield.\r\n" );
         return;
      }
      armor = 12;
   }

   if( !str_cmp( arg3, "buckler" ) )
   {
      if( location != 7 )
      {
         ch->print( "Bucklers must be of type shield.\r\n" );
         return;
      }
      armor = 13;
   }

   if( !str_cmp( arg3, "small" ) )
   {
      if( location != 7 )
      {
         ch->print( "Small shields must be of type shield.\r\n" );
         return;
      }
      armor = 14;
   }

   if( !str_cmp( arg3, "medium" ) )
   {
      if( location != 7 )
      {
         ch->print( "Medium shields must be of type shield.\r\n" );
         return;
      }
      armor = 15;
   }

   if( !str_cmp( arg3, "body" ) )
   {
      if( location != 7 )
      {
         ch->print( "Body shields must be of type shield.\r\n" );
         return;
      }
      armor = 16;
   }

   if( !str_cmp( arg3, "longsword" ) )
   {
      if( location != 8 )
      {
         ch->print( "Longswords must be of type weapon.\r\n" );
         return;
      }
      weapon = 4;
   }

   if( !str_cmp( arg3, "claw" ) )
   {
      if( location != 8 )
      {
         ch->print( "Claws must be of type weapon.\r\n" );
         return;
      }
      weapon = 2;
   }

   if( !str_cmp( arg3, "dagger" ) )
   {
      if( location != 8 )
      {
         ch->print( "Daggers must be of type weapon.\r\n" );
         return;
      }
      weapon = 1;
   }

   if( !str_cmp( arg3, "mace" ) )
   {
      if( location != 8 )
      {
         ch->print( "Maces must be of type weapon.\r\n" );
         return;
      }
      weapon = 6;
   }

   if( !str_cmp( arg3, "axe" ) )
   {
      if( location != 8 )
      {
         ch->print( "Axes must be of type weapon.\r\n" );
         return;
      }
      weapon = 9;
   }

   if( !str_cmp( arg3, "shortsword" ) )
   {
      if( location != 8 )
      {
         ch->print( "Shortswords must be of type weapon.\r\n" );
         return;
      }
      weapon = 3;
   }

   if( !str_cmp( arg3, "claymore" ) )
   {
      if( location != 8 )
      {
         ch->print( "Claymores must be of type weapon.\r\n" );
         return;
      }
      weapon = 5;
   }

   if( !str_cmp( arg3, "maul" ) )
   {
      if( location != 8 )
      {
         ch->print( "Mauls must be of type weapon.\r\n" );
         return;
      }
      weapon = 7;
   }

   if( !str_cmp( arg3, "staff" ) )
   {
      if( location != 8 )
      {
         ch->print( "Staves must be of type weapon.\r\n" );
         return;
      }
      weapon = 8;
   }

   if( !str_cmp( arg3, "waraxe" ) )
   {
      if( location != 8 )
      {
         ch->print( "War Axes must be of type weapon.\r\n" );
         return;
      }
      weapon = 10;
   }

   if( !str_cmp( arg3, "spear" ) )
   {
      if( location != 8 )
      {
         ch->print( "Spears must be of type weapon.\r\n" );
         return;
      }
      weapon = 11;
   }

   if( !str_cmp( arg3, "pike" ) )
   {
      if( location != 8 )
      {
         ch->print( "Pikes must be of type weapon.\r\n" );
         return;
      }
      weapon = 12;
   }

   if( armor == 0 && weapon == 0 )
   {
      ch->print_fmt( "{} is not a valid item type to forge.\r\n", arg3 );
      return;
   }

   /*
    * Check to see if the item can be made and charge $$$ 
    */
   int cost = 0;
   if( msmith || gsmith )
   {
      if( location == 8 )
         cost = ( int )( 1.3 * ( weapon_type[armor].cost * materials[material].cost ) );
      else
         cost = ( int )( 1.3 * ( armor_type[armor].cost * materials[material].cost ) );

      if( ch->gold < cost )
      {
         act_printf( AT_TELL, smith, nullptr, ch, TO_VICT, "$n tells you 'It will cost {} gold to forge this, but you cannot afford it!", cost );
         return;
      }
      else
         ch->gold -= cost;

      if( gsmith )
      {
         /*
          * Guild forge mobs 
          */
         if( !ch->pcdata->clan )
         {
            for( auto* clan : clanlist )
            {
               if( clan->forge == smith->pIndexData->vnum )
               {
                  clan->balance += ( int )( 0.2 * cost );
                  save_clan( clan );
                  break;
               }
            }
         }
      }
   }

   /*
    * Needs rewriting to go through inventory less, but couldn't use the previous
    * code since there could be multiple groups of ore. 
    */
   /*
    * Had to be modified and such. Wasn't doing anything as a while statement - Samson 
    */
   for( auto it = ch->carrying.begin(  ); it != ch->carrying.end(  ); )
   {
      obj_data *item = *it;
      ++it;

      if( item->pIndexData->vnum == ore_vnum && consume > 0 )
      {
         if( item->count - consume <= 0 )
         {
            consume -= item->count;
            item->extract(  );
         }
         else
         {
            item->count -= consume;
            consume = 0;
         }
      }
   }

   /*
    * If PC, check vs skill 
    */
   if( msmith == false && gsmith == false )
   {
      if( number_percent(  ) > ch->pcdata->learned[gsn_metallurgy] )
      {
         ch->print( "Your efforts result in nothing more than a molten pile of goo.\r\n" );
         ch->learn_from_failure( gsn_metallurgy );
         return;
      }
   }

   obj_data *item;
   if( !( item = get_obj_index( OBJ_VNUM_TREASURE )->create_object( 50 ) ) )
   {
      log_printf( "create_object: {}:{}, line {}.", __FILE__, __func__, __LINE__ );
      ch->print( "Ooops. Something happened while forging the item. Inform the immortals.\r\n" );
      return;
   }
   item->to_char( ch );

   if( location == 8 )
   {
      item->item_type = ITEM_WEAPON;
      item->value[7] = 0;
      item->value[8] = weapon;
      item->value[9] = material;
      if( msmith == false && gsmith == false )
         item->value[10] = choose_quality( ch->pcdata->learned[gsn_metallurgy] );
      else
         item->value[10] = choose_quality( smith->level );

      item->weapongen(  );

      if( item->value[10] == 4 )
         item->ego = sysdata->minego - 1;
   }
   else
   {
      item->item_type = ITEM_ARMOR;
      item->value[2] = 0;
      item->value[3] = armor;
      item->value[4] = material;

      item->armorgen(  );
   }

   /*
    * And finally - set the damn wear flag! 
    */
   switch ( location )
   {
      default:
         break;

      case 1:
         item->wear_flags.set( ITEM_WEAR_FEET );
         break;

      case 2:
         item->wear_flags.set( ITEM_WEAR_LEGS );
         break;

      case 3:
         item->wear_flags.set( ITEM_WEAR_BODY );
         break;

      case 4:
         item->wear_flags.set( ITEM_WEAR_HANDS );
         break;

      case 5:
         item->wear_flags.set( ITEM_WEAR_ARMS );
         break;

      case 6:
         item->wear_flags.set( ITEM_WEAR_HEAD );
         break;

      case 7:
         item->wear_flags.set( ITEM_WEAR_SHIELD );
         break;

      case 8:
         item->wear_flags.set( ITEM_WIELD );
         break;
   }
   if( msmith || gsmith )
      ch->print_fmt( "{} forges you {}, at a cost of {} gold.\r\n", smith->short_descr, item->short_descr, cost );
   else
      ch->print_fmt( "You've forged yourself {}!\r\n", aoran( item->short_descr ) );
}
