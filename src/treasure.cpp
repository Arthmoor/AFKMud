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
 *                       Rune/Gem socketing module                          *
 *                 Inspired by the system used in Diablo 2                  *
 *             Also contains the random treasure creation code              *
 ****************************************************************************/

#include <fstream>
#include <sstream>
#include "mud.h"
#include "area.h"
#include "clans.h"
#include "mobindex.h"
#include "objindex.h"
#include "roomindex.h"
#include "treasure.h"

int ncommon, nrare, nurare;
extern int top_affect;

list<rune_data*> runelist;
list<runeword_data*> rwordlist;
vector<weapontable*> w_table;

/* AGH! *BONK BONK BONK* Why didn't any of us think of THIS before!? So much NICER!
 * This table is also used in generating weapons.
 * If you edit this table, adjust TMAT_MAX in treasure.h by the number of entries you add/remove.
 */
const struct armorgenM materials[] = {

    // Material   Material Name   Weight Mod   AC Mod   WD Mod   Cost Mod   Minlevel   Maxlevel

   {0, "Not Defined", 0, 0, 0, 0, 0, 0},
   {1, "Iron ", 1.25, 0, 0, 0.9, 1, 20},
   {2, "Steel ", 1, 0, 0, 1, 5, 50}, /* Steel is the baseline value */
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
   {13, "", 1, 0, 0, 1, 1, 100} // Generic non-descript material, should always be the last one in the table
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
char *const weapon_quality[] = {
   "Not Defined", "Average", "Good", "Superb", "Legendary"
};

char *const rarity[] = {
   "Common", "Rare", "Ultrarare"
};

char *const gems1[12] = {
   "Banded Agate", "Eye Agate", "Moss Agate", "Azurite", "Blue Quartz", "Hematite",
   "Lapus Lazuli", "Malachite", "Obsidian", "Rhodochrosite", "Tiger Eye", "Freshwater Pearl"
};

char *const gems2[16] = {
   "Bloodstone", "Carnelian", "Chalcedony", "Chrysoprase", "Citrine", "Iolite", "Jasper",
   "Moonstone", "Peridot", "Quartz", "Sadonyx", "Rose Quartz", "Smokey Quartz", "Star Rose Quartz",
   "Zircon", "Black Zircon"
};

char *const gems3[16] = {
   "Amber", "Amethyst", "Chrysoberyl", "Coral", "Red Garnet", "Brown-Green Garnet", "Jade", "Jet",
   "White Pearl", "Golden Pearl", "Pink Pearl", "Silver Pearl", "Red Spinel", "Red-Brown Spinel",
   "Deep Green Spinel", "Tourmaline"
};

char *const gems4[6] = {
   "Alexandrite", "Aquamarine", "Violet Garnet", "Black Pearl", "Deep Blue Spinel", "Golden Yellow Topaz"
};

char *const gems5[7] = {
   "Emerald", "White Opal", "Black Opal", "Fire Opal", "Blue Sapphire", "Tomb Jade", "Water Opal"
};

char *const gems6[11] = {
   "Bright Green Emerald", "Blue-White Diamond", "Pink Diamond", "Brown Diamond", "Blue Diamond",
   "Jacinth", "Black Sapphire", "Ruby", "Star Ruby", "Blue Star Sapphire", "Black Star Sapphire"
};

weapontable::weapontable()
{
   init_memory( &flags, &damage, sizeof( damage ) );
}

void save_weapontable()
{
   ofstream stream;

   stream.open( WTYPE_FILE );
   if( !stream.is_open() )
   {
      log_string( "Couldn't write to weapontypes file." );
      return;
   }

   for( int x = 0; x < TWTP_MAX; ++x )
   {
      stream << weapon_type[x].type << " " << weapon_type[x].name << " " << weapon_type[x].wd << " " << weapon_type[x].weight << " "
         << weapon_type[x].cost << " " << weapon_type[x].skill << " " << weapon_type[x].damage << " " << weapon_type[x].flags << endl;
   }
   stream.close();
}

void load_weapontable()
{
   ifstream stream;
   weapontable *wt;

   w_table.clear();

   stream.open( WTYPE_FILE );
   if( !stream.is_open() )
   {
      log_string( "Couldn't read from weapontypes file." );
      return;
   }

   do
   {
      string key, value;
      char buf[MSL];

      stream >> key;
      strip_lspace( key );

      stream.getline( buf, MSL );
      value = buf;
      strip_lspace( value );

      if( key == "#WTYPE" )
         wt = new weapontable;
      else if( key == "Type" )
         wt->type = atoi( value.c_str() );
      else if( key == "Name" )
         wt->name = STRALLOC( value.c_str() );
      else if( key == "WD" )
         wt->wd = atoi( value.c_str() );
      else if( key == "Weight" )
         wt->weight = atoi( value.c_str() );
      else if( key == "Cost" )
         wt->cost = atoi( value.c_str() );
      else if( key == "Skill" )
         wt->skill = atoi( value.c_str() );
      else if( key == "Damage" )
         wt->damage = atoi( value.c_str() );
      else if( key == "Flags" )
         wt->flags = STRALLOC( value.c_str() );
      else if( key == "End" )
         w_table.push_back( wt );
   }
   while( !stream.eof() );
   stream.close();
}

CMDF( do_wtsave )
{
   save_weapontable();
   ch->print( "Done.\r\n" );
}

CMDF( do_wtload )
{
   load_weapontable();
   for( size_t x = 0; x < w_table.size(); ++x )
   {
      weapontable *w = w_table[x];

      ch->printf( "Type %d, Name %s, WD %d, Weight %f, Cost %f, Skill %d, Damage %d, Flags %s\r\n",
         w->type, w->name, w->wd, w->weight, w->cost, w->skill, w->damage, w->flags );
   }
}

rune_data::rune_data()
{
   init_memory( &stat1, &stat2, sizeof( stat2 ) );
}

rune_data::~rune_data()
{
   runelist.remove( this );
}

runeword_data::runeword_data()
{
   init_memory( &_type, &stat4, sizeof( stat4 ) );
}

runeword_data::~runeword_data()
{
   rwordlist.remove( this );
}

void free_runedata( void )
{
   list<rune_data*>::iterator rn;
   for( rn = runelist.begin(); rn != runelist.end(); )
   {
      rune_data *rune = (*rn);
      ++rn;

      deleteptr( rune );
   }

   list<runeword_data*>::iterator rw;
   for( rw = rwordlist.begin(); rw != rwordlist.end(); )
   {
      runeword_data *rword = (*rw);
      ++rw;

      deleteptr( rword );
   }
   return;
}

short get_rarity( const char *name )
{
   for( unsigned int x = 0; x < sizeof( rarity ) / sizeof( rarity[0] ); ++x )
      if( !str_cmp( name, rarity[x] ) )
         return x;
   return -1;
}

runeword_data *pick_runeword( )
{
   list<runeword_data*>::iterator irword;
   int wordpick = number_range( 1, rwordlist.size() );
   int counter = 1;

   for( irword = rwordlist.begin(); irword != rwordlist.end(); ++irword, ++counter )
   {
      runeword_data *rword = (*irword);

      if( counter == wordpick )
         return rword;
   }
   return NULL;
}

void load_runewords( void )
{
   ifstream stream;
   runeword_data *rword;

   rwordlist.clear();

   log_string( "Loading runewords..." );

   stream.open( RUNEWORD_FILE );
   if( !stream.is_open() )
   {
      log_string( "No runeword file found." );
      return;
   }

   do
   {
      string key, value;
      char buf[MIL];

      stream >> key;
      strip_lspace( key );

      if( key == "#RWORD" )
         rword = new runeword_data;
      else if( key == "Name" )
      {
         stream.getline( buf, MIL );
         value = buf;
         strip_lspace( value );
         rword->set_name( value );
      }
      else if( key == "Type" )
      {
         stream >> value;
         strip_lspace( value );
         rword->set_type( atoi( value.c_str() ) );
      }
      else if( key == "Rune1" )
      {
         stream >> value;
         strip_lspace( value );
         rword->set_rune1( value );
      }
      else if( key == "Rune2" )
      {
         stream >> value;
         strip_lspace( value );
         rword->set_rune2( value );
      }
      else if( key == "Rune3" )
      {
         stream >> value;
         strip_lspace( value );
         rword->set_rune3( value );
      }
      else if( key == "Stat1" )
      {
         stream >> value;
         strip_lspace( value );
         rword->stat1[0] = atoi( value.c_str() );

         stream >> value;
         strip_lspace( value );
         rword->stat1[1] = atoi( value.c_str() );
      }
      else if( key == "Stat2" )
      {
         stream >> value;
         strip_lspace( value );
         rword->stat2[0] = atoi( value.c_str() );

         stream >> value;
         strip_lspace( value );
         rword->stat2[1] = atoi( value.c_str() );
      }
      else if( key == "Stat3" )
      {
         stream >> value;
         strip_lspace( value );
         rword->stat3[0] = atoi( value.c_str() );

         stream >> value;
         strip_lspace( value );
         rword->stat3[1] = atoi( value.c_str() );
      }
      else if( key == "Stat4" )
      {
         stream >> value;
         strip_lspace( value );
         rword->stat4[0] = atoi( value.c_str() );

         stream >> value;
         strip_lspace( value );
         rword->stat4[1] = atoi( value.c_str() );
      }
      else if( key == "End" )
      {
         bool found = false;
         list<runeword_data*>::iterator rw;

         for( rw = rwordlist.begin(); rw != rwordlist.end(); ++rw )
         {
            runeword_data *rwd = (*rw);

            if( rwd->get_name() >= rword->get_name() )
            {
               found = true;
               rwordlist.insert( rw, rword );
               break;
            }
         }
         if( !found )
            rwordlist.push_back( rword );
      }
   }
   while( !stream.eof(  ) );
   stream.close(  );
   return;
}

void load_runes( void )
{
   ifstream stream;
   rune_data *rune;

   runelist.clear();

   log_string( "Loading runes..." );

   stream.open( RUNE_FILE );
   if( !stream.is_open() )
   {
      log_string( "No rune file found." );
      return;
   }

   do
   {
      string key, value;

      stream >> key;
      strip_lspace( key );

      if( key == "#RUNE" )
         rune = new rune_data;
      else if( key == "Name" )
      {
         stream >> value;
         strip_lspace( value );
         strip_tilde( value );
         rune->set_name( value );
      }
      else if( key == "Rarity" )
      {
         stream >> value;
         strip_lspace( value );
         rune->set_rarity( atoi( value.c_str() ) );
      }
      else if( key == "Stat1" )
      {
         stream >> value;
         strip_lspace( value );
         rune->stat1[0] = atoi( value.c_str() );

         stream >> value;
         strip_lspace( value );
         rune->stat1[1] = atoi( value.c_str() );
      }
      else if( key == "Stat2" )
      {
         stream >> value;
         strip_lspace( value );
         rune->stat2[0] = atoi( value.c_str() );

         stream >> value;
         strip_lspace( value );
         rune->stat2[1] = atoi( value.c_str() );
      }
      else if( key == "End" )
      {
         bool found = false;
         list<rune_data*>::iterator rn;

         for( rn = runelist.begin(); rn != runelist.end(); ++rn )
         {
            rune_data *r = (*rn);
            if( r->get_name() >= rune->get_name() )
            {
               found = true;
               runelist.insert( rn, rune );
               break;
            }
         }

         if( !found )
            runelist.push_back( rune );

         switch ( rune->get_rarity() )
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
   }
   while( !stream.eof(  ) );
   stream.close(  );
   load_runewords(  );
}

void save_runes( void )
{
   ofstream stream;

   stream.open( RUNE_FILE );
   if( !stream.is_open() )
   {
      log_string( "Couldn't write to rune file." );
      return;
   }

   list<rune_data*>::iterator irune;
   for( irune = runelist.begin(); irune != runelist.end(); ++irune )
   {
      rune_data *rune = (*irune);

      stream << "#RUNE" << endl;
      stream << "Name     " << rune->get_name() << endl;
      stream << "Rarity   " << rune->get_rarity() << endl;
      stream << "Stat1    " << rune->stat1[0] << " " << rune->stat1[1] << endl;
      stream << "Stat2    " << rune->stat2[0] << " " << rune->stat2[1] << endl;
      stream << "End" << endl << endl;
   }
   stream.close();
}

rune_data *check_rune( string name )
{
   list<rune_data*>::iterator irune;

   for( irune = runelist.begin(); irune != runelist.end(); ++irune )
   {
      rune_data *rune = (*irune);

      if( scomp( rune->get_name(), name ) )
         return rune;
   }
   return NULL;
}

CMDF( do_makerune )
{
   vector<string> arg = vector_argument( argument, 1 );
   rune_data *rune = NULL;

   if( ch->isnpc(  ) )
   {
      ch->print( "NPCs can't use this command.\r\n" );
      return;
   }

   if( arg.size() < 1 )
   {
      ch->print( "Syntax: makerune <name>\r\n" );
      return;
   }

   if( ( rune = check_rune( arg[0] ) ) != NULL )
   {
      ch->printf( "A rune called %s already exists. Choose another name.\r\n", arg[0].c_str() );
      return;
   }

   rune = new rune_data;
   rune->set_name( arg[0] );
   rune->set_rarity( RUNE_COMMON );

   bool found = false;
   list<rune_data*>::iterator rn;
   for( rn = runelist.begin(); rn != runelist.end(); ++rn )
   {
      rune_data *r = (*rn);

      if( r->get_name() >= rune->get_name() )
      {
         found = true;
         runelist.insert( rn, rune );
         break;
      }
   }
   if( !found )
      runelist.push_back( rune );

   ch->printf( "New rune %s has been created.\r\n", arg[0].c_str() );
   save_runes(  );
   return;
}

CMDF( do_destroyrune )
{
   vector<string> arg = vector_argument( argument, 1 );
   rune_data *rune = NULL;

   if( ch->isnpc(  ) )
   {
      ch->print( "NPCs can't use this command.\r\n" );
      return;
   }

   if( arg.size() < 1 )
   {
      ch->print( "Syntax: destroyrune <name>\r\n" );
      return;
   }

   if( !( rune = check_rune( arg[0] ) ) )
   {
      ch->printf( "No rune called %s exists.\r\n", arg[0].c_str() );
      return;
   }

   deleteptr( rune );
   save_runes(  );

   ch->printf( "Rune %s has been destroyed.\r\n", arg[0].c_str() );
   return;
}

CMDF( do_setrune )
{
   vector<string> arg = vector_argument( argument, -1 );
   rune_data *rune;

   if( ch->isnpc(  ) )
   {
      ch->print( "NPCs can't use this command.\r\n" );
      return;
   }

   if( arg.size() < 3 )
   {
      ch->print( "Syntax: setrune <rune_name> <field> <value> [second value]\r\n" );
      ch->print( "Field can be one of the following:\r\n" );
      ch->print( "name rarity stat1 stat2\r\n" );
      return;
   }

   if( !( rune = check_rune( arg[0] ) ) )
   {
      ch->printf( "No rune named %s exists.\r\n", arg[0].c_str() );
      return;
   }

   if( scomp( arg[1], "name" ) )
   {
      rune_data *newrune;

      if( ( newrune = check_rune( arg[2] ) ) != NULL )
      {
         ch->printf( "A rune named %s already exists. Choose a new name.\r\n", arg[2].c_str() );
         return;
      }
      rune->set_name( arg[2] );
      save_runes(  );
      ch->printf( "Rune %s has been renamed as %s\r\n", arg[0].c_str(), arg[2].c_str() );
      return;
   }

   if( scomp( arg[1], "rarity" ) )
   {
      short value = get_rarity( arg[2].c_str() );

      if( value < 0 || value > RUNE_ULTRARARE )
      {
         ch->printf( "%s is an invalid rarity.\r\n", arg[2].c_str() );
         return;
      }
      rune->set_rarity( value );
      save_runes(  );
      ch->printf( "%s rune is now %s rarity.\r\n", rune->get_cname(), rarity[value] );
      return;
   }

   if( scomp( arg[1], "stat1" ) )
   {
      int value = get_atype( arg[2].c_str() );

      if( value < 0 )
      {
         ch->printf( "%s is an invalid stat to apply.\r\n", arg[2].c_str() );
         return;
      }

      if( arg.size() < 4 )
      {
         do_setrune( ch, "" );
         return;
      }

      if( value == APPLY_AFFECT )
      {
         int val2 = get_aflag( arg[3].c_str() );

         if( val2 < 0 || val2 >= MAX_AFFECTED_BY )
         {
            ch->printf( "%s is an invalid affect.\r\n", argument );
            return;
         }
         rune->stat1[0] = value;
         rune->stat1[1] = val2;
         save_runes(  );
         ch->printf( "%s rune now confers: %s %s\r\n", rune->get_cname(), arg[2].c_str(), arg[3].c_str() );
         return;
      }

      if( value == APPLY_RESISTANT || value == APPLY_IMMUNE || value == APPLY_SUSCEPTIBLE || value == APPLY_ABSORB )
      {
         int val2 = get_risflag( arg[3].c_str() );

         if( val2 < 0 || val2 >= MAX_RIS_FLAG )
         {
            ch->printf( "%s is an invalid RISA flag.\r\n", arg[3].c_str() );
            return;
         }
         rune->stat1[0] = value;
         rune->stat1[1] = val2;
         save_runes(  );
         ch->printf( "%s rune now confers: %s %s\r\n", rune->get_cname(), arg[2].c_str(), arg[3].c_str() );
         return;
      }

      if( value == APPLY_WEAPONSPELL || value == APPLY_REMOVESPELL || value == APPLY_WEARSPELL )
      {
         int val2 = skill_lookup( arg[3].c_str() );

         if( !IS_VALID_SN( val2 ) )
         {
            ch->printf( "Invalid skill/spell: %s", arg[2].c_str() );
            return;
         }
         rune->stat1[0] = value;
         rune->stat1[1] = skill_table[val2]->slot;
         save_runes(  );
         ch->printf( "%s rune now confers: %s %s\r\n", rune->get_cname(), arg[2].c_str(), arg[3].c_str() );
         return;
      }

      if( !is_number( arg[3].c_str() ) )
      {
         ch->print( "Apply modifier must be numerical.\r\n" );
         return;
      }
      rune->stat1[0] = value;
      rune->stat1[1] = atoi( arg[3].c_str() );
      save_runes(  );
      ch->printf( "%s rune now confers: %s %s\r\n", rune->get_cname(), arg[2].c_str(), arg[3].c_str() );
      return;
   }

   if( scomp( arg[1], "stat2" ) )
   {
      int value = get_atype( arg[2].c_str() );

      if( value < 0 )
      {
         ch->printf( "%s is an invalid stat to apply.\r\n", arg[2].c_str() );
         return;
      }

      if( arg.size() < 4 )
      {
         do_setrune( ch, "" );
         return;
      }

      if( value == APPLY_AFFECT )
      {
         int val2 = get_aflag( arg[3].c_str() );

         if( val2 < 0 || val2 >= MAX_AFFECTED_BY )
         {
            ch->printf( "%s is an invalid affect.\r\n", argument );
            return;
         }
         rune->stat2[0] = value;
         rune->stat2[1] = val2;
         save_runes(  );
         ch->printf( "%s rune now confers: %s %s\r\n", rune->get_cname(), arg[2].c_str(), arg[3].c_str() );
         return;
      }

      if( value == APPLY_RESISTANT || value == APPLY_IMMUNE || value == APPLY_SUSCEPTIBLE || value == APPLY_ABSORB )
      {
         int val2 = get_risflag( arg[3].c_str() );

         if( val2 < 0 || val2 >= MAX_RIS_FLAG )
         {
            ch->printf( "%s is an invalid RISA flag.\r\n", arg[3].c_str() );
            return;
         }
         rune->stat2[0] = value;
         rune->stat2[1] = val2;
         save_runes(  );
         ch->printf( "%s rune now confers: %s %s\r\n", rune->get_cname(), arg[2].c_str(), arg[3].c_str() );
         return;
      }

      if( value == APPLY_WEAPONSPELL || value == APPLY_REMOVESPELL || value == APPLY_WEARSPELL )
      {
         int val2 = skill_lookup( arg[3].c_str() );

         if( !IS_VALID_SN( val2 ) )
         {
            ch->printf( "Invalid skill/spell: %s", arg[2].c_str() );
            return;
         }
         rune->stat2[0] = value;
         rune->stat2[1] = skill_table[val2]->slot;
         save_runes(  );
         ch->printf( "%s rune now confers: %s %s\r\n", rune->get_cname(), arg[2].c_str(), arg[3].c_str() );
         return;
      }

      if( !is_number( arg[3].c_str() ) )
      {
         ch->print( "Apply modifier must be numerical.\r\n" );
         return;
      }
      rune->stat2[0] = value;
      rune->stat2[1] = atoi( arg[3].c_str() );
      save_runes(  );
      ch->printf( "%s rune now confers: %s %s\r\n", rune->get_cname(), arg[2].c_str(), arg[3].c_str() );
      return;
   }
   return;
}

/* Ok Tarl, you've convinced me this is needed :) */
CMDF( do_loadrune )
{
   rune_data *rune;
   obj_data *obj;

   if( !argument || argument[0] == '\0' )
   {
      ch->print( "Load which rune? Use showrunes to display the list.\r\n" );
      return;
   }

   if( !( rune = check_rune( argument ) ) )
   {
      ch->printf( "%s does not exist.\r\n", argument );
      return;
   }

   if( !( obj = get_obj_index( OBJ_VNUM_RUNE )->create_object( ch->level ) ) )
   {
      log_printf( "create_object: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
      ch->print( "&RGeneric rune item is MISSING! Report to Samson.\r\n" );
      return;
   }
   stralloc_printf( &obj->name, "%s rune", rune->get_cname() );
   stralloc_printf( &obj->short_descr, "%s Rune", rune->get_cname() );
   stralloc_printf( &obj->objdesc, "A magical %s Rune lies here pulsating.", rune->get_cname() );
   obj->value[0] = rune->stat1[0];
   obj->value[1] = rune->stat1[1];
   obj->value[2] = rune->stat2[0];
   obj->value[3] = rune->stat2[1];
   obj->to_char( ch );
   ch->printf( "You now have a %s Rune.\r\n", rune->get_cname() );
   return;
}

/* Edited by Tarl 2 April 02 for alphabetical display */
/* Modified again by Samson for the same purpose only done differently :) */
CMDF( do_showrunes )
{
   int total = 0;

   if( runelist.empty() )
   {
      ch->print( "There are no runes created yet.\r\n" );
      return;
   }

   ch->pager( "Currently created runes:\r\n\r\n" );
   ch->pagerf( "%-6.6s %-10.10s %-15.15s %-6.6s %-15.15s %-6.6s\r\n", "Rune", "Rarity", "Stat1", "Mod1", "Stat2", "Mod2" );

   if( !argument || argument[0] == '\0' )
   {
      list<rune_data*>::iterator irune;
      for( irune = runelist.begin(); irune != runelist.end(); ++irune )
      {
         rune_data *rune = (*irune);

         ch->pagerf( "%-6.6s %-10.10s %-15.15s %-6d %-15.15s %-6d\r\n", rune->get_cname(), rarity[rune->get_rarity()],
                     a_types[rune->stat1[0]], rune->stat1[1], a_types[rune->stat2[0]], rune->stat2[1] );
         ++total;
      }
      ch->pagerf( "%d total runes displayed.\r\n", total );
   }
   else
   {
      list<rune_data*>::iterator irune;
      for( irune = runelist.begin(); irune != runelist.end(); ++irune )
      {
         rune_data *rune = (*irune);

         if( !str_prefix( argument, rune->get_cname() ) )
         {
            ch->pagerf( "%-6.6s %-10.10s %-15.15s %-6d %-15.15s %-6d\r\n", rune->get_cname(), rarity[rune->get_rarity()],
                        a_types[rune->stat1[0]], rune->stat1[1], a_types[rune->stat2[0]], rune->stat2[1] );
            ++total;
         }
      }
      ch->pagerf( "%d total runes displayed.\r\n", total );
   }
   return;
}

CMDF( do_runewords )
{
   if( rwordlist.empty() )
   {
      ch->print( "There are no runewords created yet.\r\n" );
      return;
   }

   ch->pager( "Currently created runewords:\r\n\r\n" );
   ch->pagerf( "%-13.13s %-6.6s %-12.12s %-6.6s %-12.12s %-6.6s %-12.12s %-6.6s %-12.12s %-6.6s\r\n",
               "Word", "Type", "Stat1", "Mod1", "Stat2", "Mod2", "Stat3", "Mod3", "Stat4", "Mod4" );

   int total = 0;
   if( !argument || argument[0] == '\0' )
   {
      list<runeword_data*>::iterator irword;
      for( irword = rwordlist.begin(); irword != rwordlist.end(); ++irword )
      {
         runeword_data *rword = (*irword);

         ch->pagerf( "%-17.17s %-6.6s %-12.12s %-6d %-12.12s %-6d %-12.12s %-6d %-12.12s %-6d\r\n",
                     rword->get_cname(), rword->get_type() == 0 ? "Armor" : "Weapon",
                     a_types[rword->stat1[0]], rword->stat1[1], a_types[rword->stat2[0]], rword->stat2[1],
                     a_types[rword->stat3[0]], rword->stat3[1], a_types[rword->stat4[0]], rword->stat4[1] );
         ++total;
      }
      ch->pagerf( "%d total runewords displayed.\r\n", total );
   }
   else
   {
      list<runeword_data*>::iterator irword;

      for( irword = rwordlist.begin(); irword != rwordlist.end(); ++irword )
      {
         runeword_data *rword = (*irword);

         if( !str_prefix( argument, rword->get_cname() ) )
         {
            ch->pagerf( "%-10.10s %-6.6s %-12.12s %-6d %-12.12s %-6d %-12.12s %-6d %-12.12s %-6d\r\n",
                        rword->get_cname(), rword->get_type() == 0 ? "Armor" : "Weapon",
                        a_types[rword->stat1[0]], rword->stat1[1], a_types[rword->stat2[0]], rword->stat2[1],
                        a_types[rword->stat3[0]], rword->stat3[1], a_types[rword->stat4[0]], rword->stat4[1] );
            ++total;
         }
      }
      ch->pagerf( "%d total runewords displayed.\r\n", total );
   }
   return;
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
   rune_data *rune = NULL;
   list<rune_data*>::iterator rn;
   for( rn = runelist.begin(); rn != runelist.end(); ++rn )
   {
      rune = (*rn);

      switch ( rune->get_rarity() )
      {
         default:
            break;
         case RUNE_COMMON:
            ccount += 1;
            if( ccount == pick && rare == rune->get_rarity() )
               found = true;
            break;
         case RUNE_RARE:
            rcount += 1;
            if( rcount == pick && rare == rune->get_rarity() )
               found = true;
            break;
         case RUNE_ULTRARARE:
            ucount += 1;
            if( ucount == pick && rare == rune->get_rarity() )
               found = true;
            break;
      }
      if( found )
         break;
   }

   if( !found )
   {
      bug( "%s: Rune data not found? Something bad happened!", __FUNCTION__ );
      return NULL;
   }

   obj_data *newrune;
   if( !( newrune = get_obj_index( OBJ_VNUM_RUNE )->create_object( level ) ) )
   {
      log_printf( "create_object: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
      return NULL;
   }

   stralloc_printf( &newrune->name, "%s rune", rune->get_cname() );
   stralloc_printf( &newrune->short_descr, "%s Rune", rune->get_cname() );
   stralloc_printf( &newrune->objdesc, "A magical %s Rune lies here pulsating.", rune->get_cname() );
   newrune->value[0] = rune->stat1[0];
   newrune->value[1] = rune->stat1[1];
   newrune->value[2] = rune->stat2[0];
   newrune->value[3] = rune->stat2[1];

   return newrune;
}

obj_data *generate_gem( short level )
{
   obj_data *gem;
   char *gname;
   int cost;
   short gemname, gemtable = number_range( 1, 100 );

   if( !( gem = get_obj_index( OBJ_VNUM_TREASURE )->create_object( level ) ) )
   {
      log_printf( "create_object: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
      return NULL;
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
   stralloc_printf( &gem->name, "gem %s", gname );
   stralloc_printf( &gem->short_descr, "%s", gname );
   stralloc_printf( &gem->objdesc, "A chunk of %s lies here gleaming.", gname );
   gem->cost = cost;
   return gem;
}

void obj_data::weapongen(  )
{
   affect_data *paf;
   list<affect_data*>::iterator paff;
   char *eflags = NULL;
   char flag[MIL];
   int v8, v9, v10, ovalue;
   bool protoflag = false;

   if( item_type != ITEM_WEAPON )
   {
      bug( "%s: Improperly set item passed: %s", __FUNCTION__, name );
      return;
   }

   if( value[8] >= TWTP_MAX )
   {
      bug( "%s: Improper weapon type passed for %s", __FUNCTION__, name );
      value[8] = 0;
      return;
   }

   if( value[9] >= TMAT_MAX )
   {
      bug( "%s: Improper material passed for %s", __FUNCTION__, name );
      value[9] = 0;
      return;
   }

   if( value[10] >= TQUAL_MAX )
   {
      bug( "%s: Improper quality passed for %s", __FUNCTION__, name );
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
   value[1] = UMAX( 1, v10 + materials[v9].wd );
   value[2] = UMAX( 1, v10 * ( weapon_type[v8].wd + materials[v9].wd ) );
   value[3] = weapon_type[v8].damage;
   value[4] = weapon_type[v8].skill;
   cost = ( int )( weapon_type[v8].cost * materials[v9].cost );

   eflags = weapon_type[v8].flags;

   while( eflags[0] != '\0' )
   {
      eflags = one_argument( eflags, flag );
      ovalue = get_oflag( flag );
      if( ovalue < 0 || ovalue >= MAX_ITEM_FLAG )
         bug( "%s: Unknown object extraflag: %s", __FUNCTION__, flag );
      else
         extra_flags.set( ovalue );
   }

   for( paff = affects.begin(); paff != affects.end(); )
   {
      affect_data *aff = (*paff);
      ++paff;

      affects.remove( aff );
      deleteptr( aff );
      --top_affect;
      continue;
   }

   if( protoflag )
   {
      for( paff = pIndexData->affects.begin(); paff != pIndexData->affects.end(); )
      {
         affect_data *aff = (*paff);
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
      stralloc_printf( &name, "socketed %s%s", materials[v9].name, weapon_type[v8].name );
      stralloc_printf( &short_descr, "Socketed %s%s", materials[v9].name, weapon_type[v8].name );
      stralloc_printf( &objdesc, "A socketed %s%s lies here on the ground.",
         materials[v9].name, weapon_type[v8].name );
   }
   else
   {
      stralloc_printf( &name, "%s%s", materials[v9].name, weapon_type[v8].name );
      stralloc_printf( &short_descr, "%s%s", materials[v9].name, weapon_type[v8].name );
      stralloc_printf( &objdesc, "A %s%s lies here on the ground.", materials[v9].name, weapon_type[v8].name );
   }
}

void obj_data::armorgen(  )
{
   affect_data *paf;
   list<affect_data*>::iterator paff;
   char *eflags = NULL;
   char flag[MIL];
   int v3, v4, ovalue;
   bool protoflag = false;

   if( item_type != ITEM_ARMOR )
   {
      bug( "%s: Improperly set item passed: %s", __FUNCTION__, name );
      return;
   }

   if( value[3] >= TATP_MAX )
   {
      bug( "%s: Improper armor type passed for %s", __FUNCTION__, name );
      value[3] = 0;
      return;
   }

   if( value[4] >= TMAT_MAX )
   {
      bug( "%s: Improper material passed for %s", __FUNCTION__, name );
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

   while( eflags[0] != '\0' )
   {
      eflags = one_argument( eflags, flag );
      ovalue = get_oflag( flag );
      if( ovalue < 0 || ovalue >= MAX_ITEM_FLAG )
         bug( "%s: Unknown object extraflag: %s", __FUNCTION__, flag );
      else
         extra_flags.set( ovalue );
   }

   for( paff = affects.begin(); paff != affects.end(); )
   {
      affect_data *aff = (*paff);
      ++paff;

      affects.remove( aff );
      deleteptr( aff );
      --top_affect;
      continue;
   }

   if( protoflag )
   {
      for( paff = pIndexData->affects.begin(); paff != pIndexData->affects.end(); )
      {
         affect_data *aff = (*paff);
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

   if( value[2] < 2 )
      ego = 0;
   else
      ego = sysdata->minego - 1;

   if( value[2] > 0 )
   {
      extra_flags.set( ITEM_MAGIC );
      stralloc_printf( &name, "socketed %s%s", materials[v4].name, armor_type[v3].name );
      if( v3 > 12 )
      {
         stralloc_printf( &short_descr, "Socketed %s%s", materials[v4].name, armor_type[v3].name );
         stralloc_printf( &objdesc, "A socketed %s%s lies here in a heap.",
            materials[v4].name, armor_type[v3].name );
      }
      else
      {
         stralloc_printf( &short_descr, "Socketed %s%s Chestpiece", materials[v4].name, armor_type[v3].name );
         stralloc_printf( &objdesc, "A socketed %s%s chestpiece lies here in a heap.",
            materials[v4].name, armor_type[v3].name );
      }
   }
   else
   {
      stralloc_printf( &name, "%s%s", materials[v4].name, armor_type[v3].name );
      if( v3 > 12 )
      {
         stralloc_printf( &short_descr, "%s%s", materials[v4].name, armor_type[v3].name );
         stralloc_printf( &objdesc, "A %s%s lies here in a heap.", materials[v4].name, armor_type[v3].name );
      }
      else
      {
         stralloc_printf( &short_descr, "%s%s Chestpiece", materials[v4].name, armor_type[v3].name );
         stralloc_printf( &objdesc, "A %s%s chestpiece lies here in a heap.", materials[v4].name, armor_type[v3].name );
      }
   }
   return;
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
   log_printf( "Notice: %s failed to choose. Setting generic.", __FUNCTION__ );
   return( TMAT_MAX - 1 );
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
   log_printf( "Notice: %s failed to choose. Setting hide armor.", __FUNCTION__ );
   return( 3 );
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

void make_scroll( obj_data *newitem )
{
   runeword_data *runeword = NULL;
   char *name = "Empty", *desc = "Empty";
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
            runeword = pick_runeword();
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
            runeword = pick_runeword();
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
   stralloc_printf( &newitem->name, "%s", name );
   stralloc_printf( &newitem->short_descr, "%s", desc );
   stralloc_printf( &newitem->objdesc, "%s", "A parchment scroll lies here on the ground." );
   if( runeword )
   {
      extra_descr_data *ed = new extra_descr_data;
      string typedesc;

      ed->keyword = "parchment scroll";
      if( runeword->get_type() == 0 )
         typedesc = "\"Weapon\".\n";
      else
         typedesc = "\"Armor\".\n";

      ed->desc = "The scrawlings on this parchment are almost indecipherable. All you can make out are \"";
      ed->desc += runeword->get_rune1();
      ed->desc += runeword->get_rune2();
      if( !runeword->get_rune3().empty() )
         ed->desc += runeword->get_rune3();
      ed->desc += "\" and ";
      ed->desc += typedesc;

      newitem->extradesc.push_back( ed );
   }
}

void make_potion( obj_data *newitem )
{
   char *name = "Empty", *desc = "Empty";
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
   stralloc_printf( &newitem->name, "%s", name );
   stralloc_printf( &newitem->short_descr, "%s", desc );
   stralloc_printf( &newitem->objdesc, "%s", "A glass potion flask lies here on the ground." );
}

void make_wand( obj_data *newitem )
{
   char *name = "Empty", *desc = "Empty";
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
   stralloc_printf( &newitem->name, "%s", name );
   stralloc_printf( &newitem->short_descr, "%s", desc );
   stralloc_printf( &newitem->objdesc, "%s", "A glowing wand lies here on the ground." );
}

void make_armor( obj_data *newitem )
{
   newitem->item_type = ITEM_ARMOR;

   if( newitem->value[2] < 0 )
      newitem->value[2] = num_sockets( newitem->level );     // Determine the number of sockets to put on the armor.
   if( newitem->value[3] < 0 )
      newitem->value[3] = choose_armor( newitem->level );    // Pick out an armor type.
   if( newitem->value[4] < 0 )
      newitem->value[4] = choose_material( newitem->level ); // Pick out a material for this armor.

   if( newitem->value[3] < 5 )
      newitem->value[4] = TMAT_MAX - 1; // Sets the generic material value if an organic armor is created.

   if( newitem->value[3] > 12 )
      newitem->wear_flags.set( ITEM_WEAR_SHIELD );
   else
      newitem->wear_flags.set( ITEM_WEAR_BODY );

   newitem->armorgen(  );
}

void make_weapon( obj_data *newitem )
{
   newitem->item_type = ITEM_WEAPON;

   if( newitem->value[7] < 0 )
      newitem->value[7] = num_sockets( newitem->level ); // Determine the number of sockets to put on the weapon.
   if( newitem->value[8] < 0 )
      newitem->value[8] = number_range( 1, TWTP_MAX - 1 ); // Pick out a weapon type.
   if( newitem->value[9] < 0 )
      newitem->value[9] = choose_material( newitem->level ); // Pick out a material for this weapon.
   if( newitem->value[10] < 0 )
      newitem->value[10] = choose_quality( newitem->level ); // Set the quality of this weapon.

   newitem->weapongen(  );
}

obj_data *generate_item( area_data * area, short level )
{
   obj_data *newitem = NULL;
   short pick = number_range( 1, 100 );

   if( !( newitem = get_obj_index( OBJ_VNUM_TREASURE )->create_object( level ) ) )
   {
      log_printf( "create_object: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
      return NULL;
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
      bug( "%s: zero or negative money %d.", __FUNCTION__, amount );
      amount = 1;
   }

   if( amount == 1 )
   {
      if( !( obj = get_obj_index( OBJ_VNUM_MONEY_ONE )->create_object( 1 ) ) )
      {
         log_printf( "create_object: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
         return NULL;
      }
   }
   else
   {
      if( !( obj = get_obj_index( OBJ_VNUM_MONEY_SOME )->create_object( 1 ) ) )
      {
         log_printf( "create_object: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
         return NULL;
      }
      stralloc_printf( &obj->short_descr, obj->short_descr, amount );
      obj->value[0] = amount;
   }
   return obj;
}

int make_gold( short level, char_data *ch )
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
obj_data *generate_random( reset_data *pReset, char_data *mob )
{
   obj_data *newobj = NULL;
   short picker = pReset->arg1;
   short level = pReset->arg2;

   if( pReset->command == 'W' )
   {
      picker = pReset->arg2;
      level = pReset->arg3;
   }

   if( picker == 0 )
      picker = number_range( 1, 8 );

   switch( picker )
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
         log_printf( "Generated %d gold", gold );
      return;
   }
   else if( tchance <= area->tg_item )
   {
      obj_data *item = generate_item( area, level );
      if( !item )
      {
         bug( "%s: Item object failed to create!", __FUNCTION__ );
         return;
      }
      item->to_obj( corpse );
      if( !str_cmp( corpse->name, "corpse random" ) )
         log_printf( "Generated %s", item->short_descr );
      return;
   }
   else if( tchance <= area->tg_gem )
   {
      for( int x = 0; x < ( ( level / 25 ) + 1 ); ++x )
      {
         obj_data *item = generate_gem( level );
         if( !item )
         {
            bug( "%s: Gem object failed to create!", __FUNCTION__ );
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
         bug( "%s: Rune object failed to create!", __FUNCTION__ );
         return;
      }
      item->to_obj( corpse );
      if( !str_cmp( corpse->name, "corpse random" ) )
         log_printf( "Generated %s", item->short_descr );
      return;
   }
}

/* Command used to test random treasure drops */
CMDF( do_rttest )
{
   obj_data *corpse;
   char arg[MIL];
   int mlvl, times, x;

   if( !ch->is_imp(  ) )
   {
      ch->print( "Huh?\r\n" );
      return;
   }

   argument = one_argument( argument, arg );
   if( !arg || arg[0] == '\0' || !argument || argument[0] == '\0' )
   {
      ch->print( "Usage: rttest <mob level> <times>\r\n" );
      return;
   }
   if( !is_number( arg ) && !is_number( argument ) )
   {
      ch->print( "Numerical arguments only.\r\n" );
      return;
   }

   mlvl = atoi( arg );
   times = atoi( argument );

   if( times < 1 )
   {
      ch->print( "Um, yeah. Come on man!\r\n" );
      return;
   }
   if( !( corpse = get_obj_index( OBJ_VNUM_CORPSE_NPC )->create_object( mlvl ) ) )
   {
      log_printf( "create_object: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
      return;
   }
   stralloc_printf( &corpse->name, "%s", "corpse random" );
   stralloc_printf( &corpse->short_descr, corpse->short_descr, "some random thing" );
   stralloc_printf( &corpse->objdesc, corpse->objdesc, "some random thing" );
   corpse->to_room( ch->in_room, ch );

   for( x = 0; x < times; ++x )
      generate_treasure( ch, corpse );
}

void rword_descrips( char_data * ch, obj_data * item, runeword_data * rword )
{
   ch->printf( "&YAs you attach the rune, your %s glows radiantly and becomes %s!\r\n", item->short_descr, rword->get_cname() );
   stralloc_printf( &item->name, "%s %s", item->name, rword->get_cname() );
   stralloc_printf( &item->short_descr, "%s", rword->get_cname() );
   stralloc_printf( &item->objdesc, "%s lies here on the ground.", rword->get_cname() );
   return;
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
       || paf->location == APPLY_REMOVESPELL || paf->location == APPLY_STRIPSN
       || paf->location == APPLY_RECURRINGSPELL || paf->location == APPLY_EAT_SPELL )
      paf->modifier = slot_lookup( v2 );
   else
      paf->modifier = v2;
   item->affects.push_back( paf );
   ++top_affect;
   return;
}

void check_runewords( char_data * ch, obj_data * item )
{
   list<runeword_data*>::iterator irword;

   // Runewords must contain at least 2 runes, so if these first 2 checks fail, bail out. 
   if( !item->socket[0] || !str_cmp( item->socket[0], "None" ) )
      return;

   if( !item->socket[1] || !str_cmp( item->socket[1], "None" ) )
      return;

   // Only body armors get runewords
   if( item->item_type == ITEM_ARMOR && item->wear_flags.test( ITEM_WEAR_BODY ) )
   {
      for( irword = rwordlist.begin(); irword != rwordlist.end(); ++irword )
      {
         runeword_data *rword = (*irword);

         if( rword->get_type() == 1 )
            continue;

         if( rword->get_rune3().empty() )
         {
            if( scomp( rword->get_rune1(), item->socket[0] ) && scomp( rword->get_rune2(), item->socket[1] ) )
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

         if( !item->socket[2] || !str_cmp( item->socket[2], "None" ) )
            continue;

         if( scomp( rword->get_rune1(), item->socket[0] ) && scomp( rword->get_rune2(), item->socket[1] )
             && scomp( rword->get_rune3(), item->socket[2] ) )
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
   for( irword = rwordlist.begin(); irword != rwordlist.end(); ++irword )
   {
      runeword_data *rword = (*irword);

      if( rword->get_type() == 0 )
         continue;

      if( rword->get_rune3().empty() )
      {
         if( scomp( rword->get_rune1(), item->socket[0] ) && scomp( rword->get_rune2(), item->socket[1] ) )
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

      if( !item->socket[2] || !str_cmp( item->socket[2], "None" ) )
         continue;

      if( scomp( rword->get_rune1(), item->socket[0] ) && scomp( rword->get_rune2(), item->socket[1] )
          && scomp( rword->get_rune3(), item->socket[2] ) )
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
   return;
}

void add_rune_affect( char_data * ch, obj_data * item, obj_data * rune )
{
   affect_data *paf;

   paf = new affect_data;
   paf->type = -1;
   paf->duration = -1;
   paf->bit = 0;
   if( item->item_type == ITEM_WEAPON || item->item_type == ITEM_MISSILE_WEAPON )
      paf->location = rune->value[0];
   else
      paf->location = rune->value[2];
   if( paf->location == APPLY_WEAPONSPELL || paf->location == APPLY_WEARSPELL
       || paf->location == APPLY_REMOVESPELL || paf->location == APPLY_STRIPSN
       || paf->location == APPLY_RECURRINGSPELL || paf->location == APPLY_EAT_SPELL )
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
   return;
}

CMDF( do_socket )
{
   char arg[MIL];
   obj_data *rune, *item;

   if( ch->isnpc(  ) )
      return;

   if( !argument || argument[0] == '\0' )
   {
      ch->print( "Usage: socket <rune> <item>\r\n\r\n" );
      ch->print( "Where <rune> is the name of the rune you wish to use.\r\n" );
      ch->print( "Where <item> is the weapon or armor you wish to socket the rune into.\r\n" );
      return;
   }

   argument = one_argument( argument, arg );

   if( !arg || arg[0] == '\0' )
   {
      do_socket( ch, "" );
      return;
   }

   if( !argument || argument[0] == '\0' )
   {
      do_socket( ch, "" );
      return;
   }

   if( !( rune = ch->get_obj_carry( arg ) ) )
   {
      ch->printf( "You do not have a %s rune in your inventory!\r\n", arg );
      return;
   }

   if( !( item = ch->get_obj_carry( argument ) ) )
   {
      ch->printf( "You do not have a %s in your inventory!\r\n", argument );
      return;
   }

   if( rune->item_type != ITEM_RUNE )
   {
      ch->printf( "%s is not a rune and cannot be used like that!\r\n", rune->short_descr );
      return;
   }

   item->separate(  );

   if( item->item_type == ITEM_WEAPON || item->item_type == ITEM_MISSILE_WEAPON )
   {
      if( item->value[7] < 1 )
      {
         ch->printf( "%s does not have any free sockets left.\r\n", item->short_descr );
         return;
      }

      if( !item->socket[0] || !str_cmp( item->socket[0], "None" ) )
      {
         stralloc_printf( &item->socket[0], "%s", capitalize( arg ) );
         item->value[7] -= 1;
         ch->printf( "%s glows brightly as the %s rune is inserted and now feels more powerful!\r\n",
                     item->short_descr, capitalize( arg ) );
         add_rune_affect( ch, item, rune );
         return;
      }

      if( !item->socket[1] || !str_cmp( item->socket[1], "None" ) )
      {
         stralloc_printf( &item->socket[1], "%s", capitalize( arg ) );
         item->value[7] -= 1;
         ch->printf( "%s glows brightly as the %s rune is inserted and now feels more powerful!\r\n",
                     item->short_descr, capitalize( arg ) );
         add_rune_affect( ch, item, rune );
         return;
      }

      if( !item->socket[2] || !str_cmp( item->socket[2], "None" ) )
      {
         stralloc_printf( &item->socket[2], "%s", capitalize( arg ) );
         item->value[7] -= 1;
         ch->printf( "%s glows brightly as the %s rune is inserted and now feels more powerful!\r\n",
                     item->short_descr, capitalize( arg ) );
         add_rune_affect( ch, item, rune );
         return;
      }
      bug( "%s: (%s) %s has open sockets, but all sockets are filled?!?", __FUNCTION__, ch->name, item->short_descr );
      ch->print( "Ooops. Something bad happened. Contact the immortals for assitance.\r\n" );
      return;
   }

   if( item->item_type == ITEM_ARMOR )
   {
      if( item->value[2] < 1 )
      {
         ch->printf( "%s does not have any free sockets left.\r\n", item->short_descr );
         return;
      }

      if( !item->socket[0] || !str_cmp( item->socket[0], "None" ) )
      {
         stralloc_printf( &item->socket[0], "%s", capitalize( arg ) );
         item->value[2] -= 1;
         ch->printf( "%s glows brightly as the %s rune is inserted and now feels more powerful!\r\n",
                     item->short_descr, capitalize( arg ) );
         add_rune_affect( ch, item, rune );
         return;
      }

      if( !item->socket[1] || !str_cmp( item->socket[1], "None" ) )
      {
         stralloc_printf( &item->socket[1], "%s", capitalize( arg ) );
         item->value[2] -= 1;
         ch->printf( "%s glows brightly as the %s rune is inserted and now feels more powerful!\r\n",
                     item->short_descr, capitalize( arg ) );
         add_rune_affect( ch, item, rune );
         return;
      }

      if( !item->socket[2] || !str_cmp( item->socket[2], "None" ) )
      {
         stralloc_printf( &item->socket[2], "%s", capitalize( arg ) );
         item->value[2] -= 1;
         ch->printf( "%s glows brightly as the %s rune is inserted and now feels more powerful!\r\n",
                     item->short_descr, capitalize( arg ) );
         add_rune_affect( ch, item, rune );
         return;
      }
      bug( "%s: (%s) %s has open sockets, but all sockets are filled?!?", __FUNCTION__, ch->name, item->short_descr );
      ch->print( "Ooops. Something bad happened. Contact the immortals for assitance.\r\n" );
      return;
   }
   ch->printf( "%s cannot be socketed. Only weapons, body armors, and shields are valid.\r\n", item->short_descr );
   return;
}

int get_ore( char *ore )
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

/* Written by Samson - 6/2/00
   Rewritten by Dwip - 12/12/02 (Happy Birthday, me!)
   Re-rewritten by Tarl 13/12/02 (Happy belated Birthday, Dwip ;)
   Forge command stuff.  Eliminates the need for forgemob. 
   Utilizes the new armorgen code, and greatly expands the types
   of things makable with forge.
*/
CMDF( do_forge )
{
   /*
    * Check to see what sort of flunky the smith is 
    */
   list<char_data*>::iterator ich;
   char_data *smith = NULL;
   bool msmith = false, gsmith = false;
   for( ich = ch->in_room->people.begin(); ich != ch->in_room->people.end(); ++ich )
   {
      smith = (*ich);

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
   char arg[MIL], item_type[MIL], arg3[MIL];
   argument = one_argument( argument, arg );
   argument = one_argument( argument, item_type );
   argument = one_argument( argument, arg3 );

   /*
    * Make sure we got all the args in there 
    */
   if( arg[0] == '\0' || item_type[0] == '\0' || arg3[0] == '\0' )
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
      ch->printf( "%s isn't a valid ore type.\r\n", arg );
      return;
   }

   /*
    * Oh, Dwip.... you thought that get_ore had no purpose? Guess again..... 
    */
   int ore_vnum = 0, material = 0, armor = 0, weapon = 0;
   int base_vnum = 11299;  /* All ore vnums must be one after this one */
   switch ( ore_type )
   {
      default:
         bug( "%s: Bad ore value: %d", __FUNCTION__, ore_type );
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
   list<obj_data*>::iterator iobj;
   for( iobj = ch->carrying.begin(); iobj != ch->carrying.end(); ++iobj )
   {
      obj_data *oreobj = (*iobj);
      if( oreobj->pIndexData->vnum == ore_vnum )
         orecount += oreobj->count;
   }

   if( orecount < 1 )
   {
      ch->printf( "You have no %s ore to forge an item with!\r\n", arg );
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
      ch->printf( "%s is not a valid item type to forge.\r\n", item_type );
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
      ch->printf( "%s is not a valid item type to forge.\r\n", arg3 );
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
         act_printf( AT_TELL, smith, NULL, ch, TO_VICT,
                     "$n tells you 'It will cost %d gold to forge this, but you cannot afford it!", cost );
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
            list<clan_data*>::iterator cl;
            for( cl = clanlist.begin(); cl != clanlist.end(); ++cl )
            {
               clan_data *clan = (*cl);

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
   for( iobj = ch->carrying.begin(); iobj != ch->carrying.end(); )
   {
      obj_data *item = (*iobj);
      ++iobj;

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
      log_printf( "create_object: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
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
      ch->printf( "%s forges you %s, at a cost of %d gold.\r\n", smith->short_descr, item->short_descr, cost );
   else
      ch->printf( "You've forged yourself %s!\r\n", aoran( item->short_descr ) );
   return;
}
