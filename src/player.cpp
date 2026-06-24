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
 *            Commands for personal player settings/statictics              *
 ****************************************************************************
 *                           Pet handling module                            *
 *                      Created by Samson of Alsherok                       *
 ****************************************************************************/

#include <filesystem>
#include <sstream>
#include "mud.h"
#include "area.h"
#include "calendar.h"
#include "clans.h"
#include "deity.h"
#include "descriptor.h"
#include "mobindex.h"
#include "mud_prog.h"
#include "objindex.h"
#include "raceclass.h"
#include "roomindex.h"
#include "skill_index.h"
#include "smaugaffect.h"

std::string default_fprompt( char_data * );
std::string default_prompt( char_data * );

extern int num_logins;
extern const std::array<unsigned char, 4> echo_off_str;

bool exists_player( std::string_view name )
{
   /*
    * Stands to reason that if there ain't a name to look at, they damn well don't exist! 
    */
   if( name.empty(  ) )
      return false;

   std::filesystem::path buf = std::format( "{}{}/{}", PLAYER_DIR, static_cast<char>( std::tolower( name.front() ) ), capitalize( name ) );

   if( std::filesystem::exists( buf ) )
      return true;

   else if( supermob->get_char_world( name ) != nullptr )
      return true;

   return false;
}

/* Rank buffer code */
std::string rankbuffer( char_data * ch )
{
   std::ostringstream rbuf;

   if( ch->is_immortal(  ) )
   {
      if( !ch->pcdata->rank.empty() )
         rbuf << "&Y" << ch->pcdata->rank;
      else
         rbuf << "&Y" << ch->pcdata->realm_name;
   }
   else
   {
      if( !ch->pcdata->rank.empty() )
         rbuf << "&B" << ch->pcdata->rank;
      else if( ch->pcdata->clan )
      {
         if( !ch->pcdata->clan->leadrank.empty(  ) && !ch->pcdata->clan->leader.empty(  ) && !str_cmp( ch->name, ch->pcdata->clan->leader ) )
            rbuf << "&B" << ch->pcdata->clan->leadrank;

         if( !ch->pcdata->clan->onerank.empty(  ) && !ch->pcdata->clan->number1.empty(  ) && !str_cmp( ch->name, ch->pcdata->clan->number1 ) )
            rbuf << "&B" << ch->pcdata->clan->onerank;

         if( !ch->pcdata->clan->tworank.empty(  ) && !ch->pcdata->clan->number2.empty(  ) && !str_cmp( ch->name, ch->pcdata->clan->number2 ) )
            rbuf << "&B" << ch->pcdata->clan->tworank;
      }
      else
         rbuf << "&B" << class_table[ch->Class]->who_name;
   }
   return rbuf.str(  );
}

const std::string how_good( int percent )
{
   std::string buf;

   if( percent < 0 )
      buf = "impossible - tell an immortal!";
   else if( percent == 0 )
      buf = "Not Learned";
   else if( percent <= 10 )
      buf = "Awful";
   else if( percent <= 20 )
      buf = "Terrible";
   else if( percent <= 30 )
      buf = "Bad";
   else if( percent <= 40 )
      buf = "Poor";
   else if( percent <= 55 )
      buf = "Average";
   else if( percent <= 60 )
      buf = "Tolerable";
   else if( percent <= 70 )
      buf = "Fair";
   else if( percent <= 80 )
      buf = "Good";
   else if( percent <= 85 )
      buf = "Very Good";
   else if( percent <= 90 )
      buf = "Excellent";
   else
      buf = "Superb";

   return buf;
}

bool check_pets( char_data * ch, mob_index * pet )
{
   for( auto* mob : ch->pets )
   {
      if( mob->pIndexData->vnum == pet->vnum )
         return true;
   }
   return false;
}

/* 
 * Edited by Tarl 12 May 2002 to accept a character name as argument.
 */
CMDF( do_petlist )
{
   if( !argument.empty(  ) )
   {
      char_data *victim;

      if( !( victim = ch->get_char_world( argument ) ) )
      {
         ch->print( "They aren't here.\r\n" );
         return;
      }

      if( victim->isnpc(  ) )
      {
         ch->print( "This doesn't work on NPC's.\r\n" );
         return;
      }
      ch->print_fmt( "Pets for {}:\r\n", victim->name );
      ch->print( "--------------------------------------------------------------------\r\n" );

      for( auto* pet : victim->pets )
      {
         if( !pet->in_room )
            continue;

         ch->print_fmt( "[{:5}] {:<28} [{:5}] {}\r\n", pet->pIndexData->vnum, pet->short_descr, pet->in_room->vnum, pet->in_room->name );
      }
   }
   else
   {
      ch->print( "Character           | Follower\r\n" );
      ch->print( "-----------------------------------------------------------------\r\n" );
      for( auto* vch : pclist )
      {
         for( auto* pet : vch->pets )
         {
            if( !pet->in_room )
               continue;

            ch->print_fmt( "[{:5}] {:<28} [{}] {}\r\n", pet->pIndexData->vnum, pet->short_descr, pet->in_room->vnum, pet->in_room->name );
         }
      }
   }
}

/* Smartsac toggle - Samson 6-6-99 */
CMDF( do_smartsac )
{
   if( ch->has_pcflag( PCFLAG_SMARTSAC ) )
   {
      ch->unset_pcflag( PCFLAG_SMARTSAC );
      ch->print( "Smartsac off.\r\n" );
   }
   else
   {
      ch->set_pcflag( PCFLAG_SMARTSAC );
      ch->print( "Smartsac on.\r\n" );
   }
}

/*
 * 'Split' originally by Gnort, God of Chaos.
 */
CMDF( do_split )
{
   if( argument.empty(  ) || !is_number( argument ) )
   {
      ch->print( "Split how much?\r\n" );
      return;
   }

   int amount = atoi( argument.c_str(  ) );

   if( amount < 0 )
   {
      ch->print( "Your group wouldn't like that.\r\n" );
      return;
   }

   if( amount == 0 )
   {
      ch->print( "You hand out zero coins, but no one notices.\r\n" );
      return;
   }

   if( ch->gold < amount )
   {
      ch->print( "You don't have that much gold.\r\n" );
      return;
   }

   int members = 0;
   for( auto* gch : ch->in_room->people )
   {
      if( is_same_group( gch, ch ) )
         ++members;
   }

   if( ch->has_pcflag( PCFLAG_AUTOGOLD ) && members < 2 )
      return;

   if( members < 2 )
   {
      ch->print( "Just keep it all.\r\n" );
      return;
   }

   int share = amount / members;
   int extra = amount % members;

   if( share == 0 )
   {
      ch->print( "Don't even bother, cheapskate.\r\n" );
      return;
   }

   ch->gold -= amount;
   ch->gold += share + extra;

   ch->print_fmt( "&[gold]You split {} gold coins. Your share is {} gold coins.\r\n", amount, share + extra );

   for( auto* gch2 : ch->in_room->people )
   {
      if( gch2 != ch && is_same_group( gch2, ch ) )
      {
         act_printf( AT_GOLD, ch, nullptr, gch2, TO_VICT, "$n splits {} gold coins. Your share is {} gold coins.", amount, share );
         gch2->gold += share;
      }
   }
}

CMDF( do_gold )
{
   ch->print_fmt( "&[gold]You have {} gold pieces.\r\n", ch->gold );
}

/* Spits back a word for a stat being rolled or viewed in score - Samson 12-20-00 */
const std::string attribtext( int attribute )
{
   std::string atext;

   if( attribute < 25 )
      atext = "Excellent";
   if( attribute < 17 )
      atext = "Good";
   if( attribute < 13 )
      atext = "Fair";
   if( attribute < 9 )
      atext = "Poor";
   if( attribute < 5 )
      atext = "Bad";

   return atext;
}

/* Return a string for weapon condition - Samson 3-01-02 */
const std::string condtxt( int current, int base )
{
   std::string text;

   current *= 100;
   base *= 100;

   if( current < base * 0.25 )
      text = " }R[Falling Apart!]&D";
   else if( current < base * 0.5 )
      text = " &R[In Need of Repair]&D";
   else if( current < base * 0.75 )
      text = " &Y[In Fair Condition]&D";
   else if( current < base )
      text = " &g[In Good Condition]&D";
   else
      text = " &G[In Perfect Condition]&D";

   return text;
}

void pc_data::save_zonedata( FILE * fp )
{
   /*
    * Save the list of zones PC has visited - Samson 7-11-00 
    */
   for( auto& zn : zone )
   {
      fprintf( fp, "Zone         %s~\n", zn.c_str(  ) );
   }
}

void pc_data::load_zonedata( FILE * fp )
{
   std::string zonename;
   bool found = false;

   fread_string( zonename, fp );

   for( auto* tarea : arealist )
   {
      if( !str_cmp( tarea->name, zonename ) )
      {
         found = true;
         break;
      }
   }

   if( !found )
      return;

   auto it = std::lower_bound( zone.begin(), zone.end(), zonename );
   zone.insert( it, zonename );
}

/* Functions for use with area visiting code - Samson 7-11-00  */
bool char_data::has_visited( area_data * area )
{
   if( isnpc(  ) )
      return true;

   for( auto& zn : pcdata->zone )
   {
      if( !str_cmp( area->name, zn ) )
         return true;
   }
   return false;
}

void char_data::update_visits( room_index * room )
{
   std::list<std::string>::iterator zl;

   if( isnpc(  ) || has_visited( room->area ) )
      return;

   std::string zonename = room->area->name;

   for( zl = pcdata->zone.begin(  ); zl != pcdata->zone.end(  ); ++zl )
   {
      std::string zn = *zl;

      if( zn >= zonename )
      {
         pcdata->zone.insert( zl, zonename );
         return;
      }
   }
   pcdata->zone.push_back( zonename );
}

void char_data::remove_visit( room_index * room )
{
   if( !has_visited( room->area ) || isnpc(  ) )
      return;

   pcdata->zone.remove( room->area->name );
}

/* Modified to display zones in alphabetical order by Tarl - 5th Feb 2001 */
/* Redone to use the sort methods in load_zonedata and update_visits by Samson 10-4-03 */
CMDF( do_visits )
{
   char_data *victim;
   int visits = 0;

   if( ch->isnpc(  ) )
   {
      ch->print( "NPCs cannot use this command.\r\n" );
      return;
   }

   if( !ch->is_immortal(  ) || argument.empty(  ) )
   {
      ch->pager( "You have visited the following areas:\r\n" );
      ch->pager( "-------------------------------------\r\n" );
      for( auto& zn : ch->pcdata->zone )
      {
         ch->pagerf( "%s\r\n", zn.c_str(  ) );
         ++visits;
      }
      ch->pager_fmt( "&YTotal areas visited: {}\r\n", visits );
      return;
   }

   if( !( victim = ch->get_char_world( argument ) ) )
   {
      ch->print( "No such person is online.\r\n" );
      return;
   }

   if( victim->isnpc(  ) )
   {
      ch->print( "That's an NPC, they don't have this data.\r\n" );
      return;
   }

   if( victim == ch )
   {
      do_visits( ch, "" );
      return;
   }
   ch->pager_fmt( "{} has visited the following areas:\r\n", victim->name );
   ch->pager( "-------------------------------------------\r\n" );

   for( auto& zn : victim->pcdata->zone )
   {
      ch->pagerf( "%s\r\n", zn.c_str(  ) );
      ++visits;
   }
   ch->pager_fmt( "&YTotal areas visited: {}\r\n", visits );
}

/* -Thoric
 * Display your current exp, level, and surrounding level exp requirements
 */
CMDF( do_level )
{
   int lowlvl, hilvl;

   if( ch->level == 1 )
      lowlvl = 1;
   else
      lowlvl = umax( 2, ch->level - 5 );
   hilvl = urange( ch->level, ch->level + 5, MAX_LEVEL );

   ch->print_fmt( "\r\n&[score]Experience required, levels {} to {}:\r\n______________________________________________\r\n\r\n", lowlvl, hilvl );
   std::string buf = std::format( " exp  (You have: {:11})", ch->exp );
   std::string buf2 = std::format( " exp  (To level: {:11})", exp_level( ch->level + 1 ) - ch->exp );
   for( int x = lowlvl; x <= hilvl; ++x )
      ch->print_fmt( " ({:2}) {:11}{}\r\n", x, exp_level( x ), ( x == ch->level ) ? buf : ( x == ch->level + 1 ) ? buf2 : " exp" );
   ch->print( "______________________________________________\r\n" );
}

/* 1997, Blodkai */
CMDF( do_remains )
{
   std::string buf;
   bool found = false;

   if( ch->isnpc(  ) )
      return;

   ch->set_color( AT_MAGIC );

   if( !ch->pcdata->deity )
   {
      ch->print( "You have no deity from which to seek such assistance...\r\n" );
      return;
   }

   if( ch->pcdata->favor < ch->level * 2 )
   {
      ch->print( "Your favor is insufficient for such assistance...\r\n" );
      return;
   }

   ch->pager_fmt( "{} appears in a vision, revealing that your remains... ", ch->pcdata->deity->name );

   buf = "the corpse of ";
   buf.append( ch->name );
   for( auto* obj : objlist )
   {
      if( obj->in_room && !str_cmp( buf, obj->short_descr ) && ( obj->pIndexData->vnum == OBJ_VNUM_CORPSE_PC ) )
      {
         found = true;
         ch->pager_fmt( "\r\n  - at {} will endure for {} ticks", obj->in_room->name, obj->timer );
      }
   }
   if( !found )
      ch->pager( " no longer exist.\r\n" );
   else
   {
      ch->pager( "\r\n" );
      ch->pcdata->favor -= ch->level * 2;
   }
}

/*-----------------------------------------------
Added 2-21-05 -- Cam 
This is a new format for the affected command by Zarius

Edited 2-27-05 to limit the output to various levels -- Cam
Levels 1-10 : Imbued, Spell Affects (spell name only)
Levels 11-20: Add Spell Affects (what is affected), Saves (yes/no)
Levels 21-40: Add Suscept, Resist, Spell Effects (modifier), Saves (modifier)
Levels 41+  : Add Immune, Absorb, Spell Effects (time), 
------------------------------------------------*/
CMDF( do_affected )
{
   int affnum = 0;
   skill_type *skill;

   if( ch->isnpc(  ) )
      return;

   // For a brief "at a glance" output of the affected command
   if( !str_cmp( argument, "by" ) )
   {
      ch->print( "&z-=&w[ &WAffects Quick-View &w]&z=-&D\r\n" );

      ch->print( "\r\n&zImbued with: \r\n" );
      ch->print_fmt( " &C{}\r\n\r\n", ch->has_aflags(  )? bitset_string( ch->get_aflags(  ), aff_flags ) : "nothing" );

      ch->print( "\r\n&zSpells: \r\n" );

      for( auto* paf : ch->affects )
      {
         if( ( skill = get_skilltype( paf->type ) ) != nullptr )
         {
            ++affnum;
            ch->print_fmt( "&W{}&z> &BSpell:&w {:<18}&D\r\n", affnum, skill->name );
         }
      }

      if( affnum == 0 )
         ch->print( "&wNo cantrip or skill affects you.\r\n\r\n" );
      return;
   }

   // For full output of affected command
   ch->print( "&z-=&w[ &WAffects Summary &w]&z=-&D\r\n" );

   ch->print( "\r\n&zImbued with:\r\n" );
   ch->print_fmt( "&C{}&D\r\n\r\n", ch->has_aflags(  )? bitset_string( ch->get_aflags(  ), aff_flags ) : "nothing" );

   if( ch->level > LEVEL_AVATAR * 0.40 )
   {
      if( ch->has_immunes(  ) )
      {
         ch->print( "&zYou are immune to:\r\n" );
         ch->print_fmt( "&C{}&D\r\n\r\n", bitset_string( ch->get_immunes(  ), ris_flags ) );
      }
      else
         ch->print( "&zYou are immune to: \r\n&CNothing&D\r\n\r\n" );

      if( ch->has_absorbs(  ) )
      {
         ch->print( "&zYou absorb: \r\n" );
         ch->print_fmt( "&C%s&D\r\n\r\n", bitset_string( ch->get_absorbs(  ), ris_flags ) );
      }
      else
         ch->print( "&zYou absorb: \r\n&CNothing&D\r\n\r\n" );
   }

   if( ch->level >= LEVEL_AVATAR * 0.20 )
   {
      if( ch->has_resists(  ) )
      {
         ch->print( "&zYou are resistant to:\r\n" );
         ch->print_fmt( "&C{}&D\r\n\r\n", bitset_string( ch->get_resists(  ), ris_flags ) );
      }
      else
         ch->print( "&zYou are resistant to: \r\n&CNothing&D\r\n\r\n" );

      if( ch->has_susceps(  ) )
      {
         ch->print( "&zYou are susceptible to:\r\n" );
         ch->print_fmt( "&C{}&D\r\n\r\n", bitset_string( ch->get_susceps(  ), ris_flags ) );
      }
      else
         ch->print( "&zYou are susceptible to: \r\n&CNothing&D\r\n\r\n" );
   }

   ch->print( "&z-=&w[ &WSpell Effects &w]&z=-&D\r\n\r\n" );
   for( auto* paf : ch->affects )
   {
      if( ( skill = get_skilltype( paf->type ) ) != nullptr )
      {
         std::string mod;
         ++affnum;

         if( paf->location == APPLY_AFFECT || paf->location == APPLY_EXT_AFFECT )
            mod = aff_flags[paf->modifier];
         else if( paf->location == APPLY_RESISTANT || paf->location == APPLY_IMMUNE || paf->location == APPLY_ABSORB || paf->location == APPLY_SUSCEPTIBLE )
            mod = ris_flags[paf->modifier];
         else
            mod = std::to_string( paf->modifier );

         if( ch->level > LEVEL_AVATAR * 0.40 )
         {
            ch->print_fmt( "&W{}&z> &BSpell:&w {:<18} &W- &PModifies&w {:<18} &Bby &Y{:<18}&B for&R {:<4}&B rounds.&D\r\n",
                        affnum, skill->name, a_types[paf->location], mod, paf->duration );
         }
         else if( ch->level > ( LEVEL_AVATAR * 0.20 ) && ch->level <= ( LEVEL_AVATAR * 0.40 ) )
         {
            ch->print_fmt( "&W{}&z> &BSpell:&w {:<18} &W- &PModifies&w {:<18}&B by &Y{:<18}&D\r\n", affnum, skill->name, a_types[paf->location], mod );
         }
         else if( ch->level > ( LEVEL_AVATAR * 0.10 ) && ch->level <= ( LEVEL_AVATAR * 0.20 ) )
         {
            ch->print_fmt( "&W{}&z> &BSpell:&w {:<18} &W- &PModifies&w {:<18}&D\r\n", affnum, skill->name, a_types[paf->location] );
         }
         else
            ch->print_fmt( "&W{}&z> &BSpell:&w {:<18}&D\r\n", affnum, skill->name );
      }
   }
   if( affnum == 0 )
      ch->print( "&wNo cantrip or skill affects you.\r\n" );

   ch->print_fmt( "\r\n&WTotal of &R{}&W affects.&D\r\n", affnum );

   if( ch->level > LEVEL_AVATAR * 0.10 )
      ch->print( "\r\n&z-=&w[ &WSaves vs Resistances &w]&z=-&D\r\n\r\n" );

   // This part limits the output to a yes\no for saves
   if( ch->level > ( LEVEL_AVATAR * 0.10 ) && ch->level < ( LEVEL_AVATAR * 0.20 ) )
   {
      if( ch->saving_spell_staff != 0 )
         ch->print( "&zSpell Effects, Staves    &W- &Gyes&D\r\n" );
      else
         ch->print( "&zSpell Effects, Staves    &W- &Gno&D\r\n" );

      if( ch->saving_para_petri != 0 )
         ch->print( "&zParalysis, Petrification &W- &Gyes&D\r\n" );
      else
         ch->print( "&zParalysis, Petrification &W- &Gno&D\r\n" );

      if( ch->saving_wand != 0 )
         ch->print( "&zWands                    &W- &Gyes&D\r\n" );
      else
         ch->print( "&zWands                    &W- &Gno&D\r\n" );

      if( ch->saving_poison_death != 0 )
         ch->print( "&zPoison, Death            &W- &Gyes&D\r\n" );
      else
         ch->print( "&zPoison, Death            &W- &Gno&D\r\n" );

      if( ch->saving_breath != 0 )
         ch->print( "&zBreath Weapons           &W- &Gyes&D\r\n" );
      else
         ch->print( "&zBreath Weapons           &W- &Gno&D\r\n" );
   }

   if( ch->level > LEVEL_AVATAR * 0.20 )
   {
      ch->print_fmt( "&zSpell Effects, Staves    &W- &G{}\r\n&D", ch->saving_spell_staff );
      ch->print_fmt( "&zParalysis, Petrification &W- &G{}\r\n&D", ch->saving_para_petri );
      ch->print_fmt( "&zWands                    &W- &G{}\r\n&D", ch->saving_wand );
      ch->print_fmt( "&zPoison, Death            &W- &G{}\r\n&D", ch->saving_poison_death );
      ch->print_fmt( "&zBreath Weapons           &W- &G{}\r\n&D", ch->saving_breath );
   }
}

CMDF( do_inventory )
{
   char_data *victim;

   if( argument.empty(  ) || !ch->is_immortal(  ) )
      victim = ch;
   else
   {
      if( !( victim = ch->get_char_world( argument ) ) )
      {
         ch->print_fmt( "There is nobody named {} online.\r\n", argument );
         return;
      }
   }

   if( victim != ch )
      ch->print_fmt( "&R{} is carrying:\r\n", victim->isnpc(  ) ? victim->short_descr : victim->name );
   else
      ch->print( "&RYou are carrying:\r\n" );
   show_list_to_char( ch, victim->carrying, true, true );
   ch->print( "&GItems with a &W*&G after them have other items stored inside.\r\n" );
}

bool is_valid_wear_loc( char_data *ch, int wear_loc )
{
   std::bitset<MAX_BPART> body_parts;

   if( ch->has_bparts() )
      body_parts = ch->get_bparts();
   else
      body_parts = race_table[ch->race]->body_parts;

   // Parts that have no conditions yet
   if( wear_loc == WEAR_NECK_1 || wear_loc == WEAR_NECK_2 || wear_loc == WEAR_BODY
    || wear_loc == WEAR_HEAD || wear_loc == WEAR_SHIELD || wear_loc == WEAR_ABOUT
    || wear_loc == WEAR_WAIST || wear_loc == WEAR_BACK || wear_loc == WEAR_FACE || wear_loc == WEAR_LODGE_RIB )
   {
      return true;
   }

   // Parts which are conditional
   if( wear_loc == WEAR_LIGHT || wear_loc == WEAR_HANDS || wear_loc == WEAR_WRIST_L
    || wear_loc == WEAR_WRIST_R || wear_loc == WEAR_WIELD || wear_loc == WEAR_HOLD
    || wear_loc == WEAR_DUAL_WIELD || wear_loc == WEAR_MISSILE_WIELD )
   {
      if( body_parts.test( PART_HANDS ) )
         return true;
   }

   if( wear_loc == WEAR_FINGER_L || wear_loc == WEAR_FINGER_R )
   {
      if( body_parts.test( PART_FINGERS ) )
         return true;
   }

   if( wear_loc == WEAR_LEGS || wear_loc == WEAR_LODGE_LEG )
   {
      if( body_parts.test( PART_LEGS ) )
         return true;
   }

   if( wear_loc == WEAR_FEET )
   {
      if( body_parts.test( PART_FEET ) )
         return true;
   }

   if( wear_loc == WEAR_ARMS || wear_loc == WEAR_LODGE_ARM )
   {
      if( body_parts.test( PART_ARMS ) )
         return true;
   }

   if( wear_loc == WEAR_EARS )
   {
      if( body_parts.test( PART_EAR ) )
         return true;
   }

   if( wear_loc == WEAR_EYES )
   {
      if( body_parts.test( PART_EYE ) )
         return true;
   }

   if( wear_loc == WEAR_HOOVES )
   {
      if( body_parts.test( PART_HOOVES ) )
         return true;
   }

   if( wear_loc == WEAR_TAIL )
   {
      if( body_parts.test( PART_TAIL ) || body_parts.test( PART_TAILATTACK ) )
         return true;
   }

   return false;
}

CMDF( do_equipment )
{
   char_data *victim;

   if( !ch )
      return;

   if( argument.empty(  ) || !ch->is_immortal(  ) )
      victim = ch;
   else
   {
      if( !( victim = ch->get_char_world( argument ) ) )
      {
         ch->print_fmt( "There is nobody named {} online.\r\n", argument );
         return;
      }
   }

   if( victim != ch )
      ch->print_fmt( "&R{} is using:\r\n", victim->isnpc(  ) ? victim->short_descr : victim->name );
   else
      ch->print( "&RYou are using:\r\n" );
   ch->set_color( AT_OBJECT );

   int iWear;
   for( iWear = 0; iWear < MAX_WEAR; ++iWear )
   {
      obj_data *obj2;
      int count = 0;

      if( iWear < ( MAX_WEAR - 3 ) )
      {
         if( victim->race > 0 && victim->race < MAX_PC_RACE )
         {
            if( !is_valid_wear_loc( victim, iWear ) )
               continue;

            if( victim->has_bparts() )
               ch->print( victim->bodypart_where_names[iWear] );
            else
               ch->print( race_table[victim->race]->bodypart_where_names[iWear] );
         }
         else
            ch->print( where_names[iWear] );
      }

      if( !( obj2 = victim->get_eq( iWear ) ) && iWear < ( MAX_WEAR - 3 ) )
      {
         ch->print( "&R<Nothing>&D\r\n" );
         continue;
      }

      for( auto* obj : victim->carrying )
      {
         if( obj->wear_loc == iWear )
         {
            ++count;
            if( iWear >= ( MAX_WEAR - 3 ) )
            {
               if( victim->race > 0 && victim->race < MAX_PC_RACE )
               {
                  if( !is_valid_wear_loc( victim, iWear ) )
                     continue;

                  if( victim->has_bparts() )
                     ch->print( victim->bodypart_where_names[iWear] );
                  else
                     ch->print( race_table[victim->race]->bodypart_where_names[iWear] );
               }
               else
                  ch->print( where_names[iWear] );
            }
            if( count > 1 )
               ch->print( "&C<&W LAYERED &C>&D         " );

            if( ch->can_see_obj( obj, false ) )
            {
               ch->print( obj->format_to_char( ch, true, true ) );

               if( obj->item_type == ITEM_ARMOR )
                  ch->print( condtxt( obj->value[0], obj->value[1] ) );
               if( obj->item_type == ITEM_WEAPON || obj->item_type == ITEM_MISSILE_WEAPON )
                  ch->print( condtxt( obj->value[6], obj->value[0] ) );
               if( obj->item_type == ITEM_PROJECTILE )
                  ch->print( condtxt( obj->value[5], obj->value[0] ) );

               if( !obj->contents.empty(  ) )
                  ch->print( " &W*" );
               ch->print( "\r\n" );
            }
            else
               ch->print( "Something\r\n" );
         }
      }
      if( count == 0 && iWear < ( MAX_WEAR - 3 ) )
         ch->print( "\r\n" );
   }
   ch->print( "&GItems with a &W*&G after them have other items stored inside.\r\n" );
}

CMDF( do_title )
{
   if( ch->isnpc(  ) )
      return;

   if( ch->has_pcflag( PCFLAG_NOTITLE ) )
   {
      ch->print( "The Gods prohibit you from changing your title.\r\n" );
      return;
   }

   if( argument.empty(  ) )
   {
      ch->print( "Change your title to what?\r\n" );
      return;
   }

   if( argument.find( '}' ) != std::string::npos )
   {
      ch->print( "New title is not acceptable, blinking colors are not allowed.\r\n" );
      return;
   }

   smash_tilde( argument );
   ch->set_title( argument );
   ch->print( "Ok.\r\n" );
}

/*
 * Set your personal description				-Thoric
 */
CMDF( do_description )
{
   if( ch->isnpc(  ) )
   {
      ch->print( "Monsters are too dumb to do that!\r\n" );
      return;
   }

   if( !ch->desc )
   {
      bug( "{}: no descriptor", __func__ );
      return;
   }

   if( ch->has_pcflag( PCFLAG_NODESC ) )
   {
      ch->print( "You cannot set your description.\r\n" );
      return;
   }

   switch ( ch->substate )
   {
      default:
         bug( "{}: {} illegal substate %d", __func__, ch->name, ch->substate );
         return;

      case SUB_RESTRICTED:
         ch->print( "You cannot use this command from within another command.\r\n" );
         return;

      case SUB_NONE:
         ch->substate = SUB_PERSONAL_DESC;
         ch->pcdata->dest_buf = ch;
         ch->set_editor_desc( std::format( "Your description ({})", ch->name ) );
         ch->start_editing( ch->chardesc );
         return;

      case SUB_PERSONAL_DESC:
         ch->chardesc = ch->copy_buffer( );
         ch->stop_editing(  );
         return;

      case SUB_EDIT_ABORT:
         ch->substate = SUB_NONE;
         ch->print( "Aborting description.\r\n" );
         return;
   }
}

/* Ripped off do_description for whois bio's -- Scryn*/
CMDF( do_bio )
{
   if( ch->isnpc(  ) )
   {
      ch->print( "Mobs cannot set a bio.\r\n" );
      return;
   }

   if( !ch->desc )
   {
      bug( "{}: no descriptor", __func__ );
      return;
   }

   if( ch->has_pcflag( PCFLAG_NOBIO ) )
   {
      ch->print( "The gods won't allow you to do that!\r\n" );
      return;
   }

   switch ( ch->substate )
   {
      default:
         bug( "{}: {} illegal substate {}", __func__, ch->name, ch->substate );
         return;

      case SUB_RESTRICTED:
         ch->print( "You cannot use this command from within another command.\r\n" );
         return;

      case SUB_NONE:
         ch->substate = SUB_PERSONAL_BIO;
         ch->pcdata->dest_buf = ch;
         ch->set_editor_desc( std::format( "Your bio ({}).", ch->name ) );
         ch->start_editing( ch->pcdata->bio );
         return;

      case SUB_PERSONAL_BIO:
         ch->pcdata->bio = ch->copy_buffer( );
         ch->stop_editing(  );
         return;

      case SUB_EDIT_ABORT:
         ch->substate = SUB_NONE;
         ch->print( "Aborting Bio.\r\n" );
         return;
   }
}

CMDF( do_report )
{
   if( ch->isnpc(  ) && ch->fighting )
      return;

   if( ch->has_aflag( AFF_POSSESS ) )
   {
      ch->print( "You can't do that in your current state of mind!\r\n" );
      return;
   }

   ch->print_fmt( "You report: {}/{} hp {}/{} mana {}/{} mv {} xp {} tnl.\r\n",
               ch->hit, ch->max_hit, ch->mana, ch->max_mana, ch->move, ch->max_move, ch->exp, ( exp_level( ch->level + 1 ) - ch->exp ) );

   act_printf( AT_REPORT, ch, nullptr, nullptr, TO_ROOM, "$n reports: {}/{} hp {}/{} mana {}/{} mv {} xp {} tnl.",
               ch->hit, ch->max_hit, ch->mana, ch->max_mana, ch->move, ch->max_move, ch->exp, ( exp_level( ch->level + 1 ) - ch->exp ) );
}

CMDF( do_fprompt )
{
   if( ch->isnpc(  ) )
   {
      ch->print( "NPC's can't change their prompt..\r\n" );
      return;
   }

   smash_tilde( argument );

   if( argument.empty(  ) || !str_cmp( argument, "display" ) )
   {
      ch->print( "Your current fighting prompt string:\r\n" );
      ch->print_fmt( "&W{}\r\n", ch->pcdata->fprompt );
      ch->print( "&wType 'help prompt' for information on changing your prompt.\r\n" );
      return;
   }

   ch->print( "&wReplacing old prompt of:\r\n" );
   ch->print_fmt( "&W{}\r\n", ch->pcdata->fprompt );
   ch->pcdata->fprompt.clear();

   if( argument.length(  ) > 128 )
      argument = argument.substr( 0, 128 );

   /*
    * Can add a list of pre-set prompts here if wanted.. perhaps
    * 'prompt 1' brings up a different, pre-set prompt 
    */
   if( !str_cmp( argument, "default" ) )
      ch->pcdata->fprompt = default_fprompt( ch );
   else
      ch->pcdata->fprompt = argument;
}

CMDF( do_prompt )
{
   if( ch->isnpc(  ) )
   {
      ch->print( "NPC's can't change their prompt..\r\n" );
      return;
   }

   smash_tilde( argument );

   if( argument.empty(  ) || !str_cmp( argument, "display" ) )
   {
      ch->print( "&wYour current prompt string:\r\n" );
      ch->print_fmt( "&W{}\r\n", ch->pcdata->prompt );
      ch->print( "&wType 'help prompt' for information on changing your prompt.\r\n" );
      return;
   }

   ch->print( "&wReplacing old prompt of:\r\n" );
   ch->print_fmt( "&W{}\r\n", ch->pcdata->prompt.empty() ? "(default prompt)" : ch->pcdata->prompt );
   ch->pcdata->prompt.clear();

   if( argument.length(  ) > 128 )
      argument = argument.substr( 0, 128 );

   /*
    * Can add a list of pre-set prompts here if wanted.. perhaps
    * 'prompt 1' brings up a different, pre-set prompt 
    */
   if( !str_cmp( argument, "default" ) )
      ch->pcdata->prompt = default_prompt( ch );
   else
      ch->pcdata->prompt = argument;
}

/* Alternate Self delete command provided by Waldemar Thiel (Swiv) */
/* Allows characters to delete themselves - Added 1-18-98 by Samson */
CMDF( do_delet )
{
   ch->print( "If you want to DELETE, spell it out.\r\n" );
}

CMDF( do_delete )
{
   if( ch->isnpc(  ) )
   {
      ch->print( "Yeah, right. Mobs can't delete themselves.\r\n" );
      return;
   }

   if( ch->fighting != nullptr )
   {
      ch->print( "Wait until the fight is over before deleting yourself.\r\n" );
      return;
   }

   /*
    * Reimbursement warning added to code by Samson 1-18-98 
    */
   ch->print( "&YRemember, this decision is IRREVERSABLE. There are no reimbursements!\r\n" );

   /*
    * Immortals warning added to code by Samson 1-18-98 
    */
   if( ch->is_immortal(  ) )
   {
      ch->print_fmt( "Consider this carefully {}, if you delete, you will not\r\nbe reinstated as an immortal!\r\n", ch->name );
      ch->print( "Any area data you have will also be lost if you proceed.\r\n" );
   }
   ch->print( "\r\n&RType your password if you wish to delete your character.\r\n" );
   ch->print( "[DELETE] Password: " );
   ch->desc->write_to_buffer( std::string_view{ reinterpret_cast<const char*>( echo_off_str.data() ), echo_off_str.size() } );
   ch->desc->connected = CON_DELETE;
}

/* New command for players to become pkillers - Samson 4-12-98 */
CMDF( do_deadly )
{
   if( ch->isnpc(  ) )
      return;

   if( ch->has_pcflag( PCFLAG_DEADLY ) )
   {
      ch->print( "You are already a deadly character!\r\n" );
      return;
   }

   if( argument.empty(  ) )
   {
      ch->print( "&RTo become a pkiller, you MUST type DEADLY YES to do so.\r\n" );
      ch->print( "Remember, this decision is IRREVERSEABLE, so consider it carefuly!\r\n" );
      return;
   }

   if( str_cmp( argument, "yes" ) )
   {
      ch->print( "&RTo become a pkiller, you MUST type DEADLY YES to do so.\r\n" );
      ch->print( "Remember, this decision is IRREVERSEABLE, so consider it carefuly!\r\n" );
      return;
   }

   ch->set_pcflag( PCFLAG_DEADLY );
   ch->print( "&YYou have joined the ranks of the deadly. The gods cease to protect you!\r\n" );
   log_printf( "{} has become a pkiller!", ch->name );
}

const std::string output_person( char_data * ch, char_data * player )
{
   std::string rank;
   std::ostringstream stats, clan_name, outbuf;

   rank = rankbuffer( player );
   outbuf << color_align( rank, 20, ALIGN_CENTER );

   stats << ch->color_str( AT_WHO3 ) << "[";

   if( player->has_pcflag( PCFLAG_AFK ) )
      stats << "AFK";
   else
      stats << "---";
   if( player->CAN_PKILL(  ) )
      stats << "PK]&d";
   else
      stats << "--]&d";

   stats << ch->color_str( AT_WHO3 );

   /*
    * Modified by Tarl 24 April 02 to display an invis level on the AFKMud interface. 
    */
   if( player->has_pcflag( PCFLAG_WIZINVIS ) )
   {
      std::string invis_str = std::format( " ({})", player->pcdata->wizinvis );
      stats << invis_str;
   }

   outbuf << stats.str(  );

   if( player->pcdata->clan )
      clan_name << " " << ch->color_str( AT_WHO5 ) << "[" << player->pcdata->clan->name << ch->color_str( AT_WHO5 ) << "]&d";
   else
      clan_name.clear(  );

   outbuf << " " << ch->color_str( AT_WHO4 ) << player->name << player->pcdata->title << clan_name.str(  );

   outbuf << std::endl;

   return outbuf.str(  );
}

/* Derived directly from the i3who code, which is a hybrid mix of Smaug, RM, and Dale who. */
int afk_who( char_data * ch )
{
   std::vector<char_data *> players;
   std::vector<char_data *> immortals;
   std::vector<char_data *>::iterator iplr;
   int pcount = 0;

   if( !ch )
   {
      bug( "{}: Called with no *ch!", __func__ );
      return 0;
   }

   std::string s1 = ch->color_str( AT_WHO );
   std::string s2 = ch->color_str( AT_WHO2 );
   std::string s3 = ch->color_str( AT_WHO3 );
   std::string s4 = ch->color_str( AT_WHO4 );
   std::string s5 = ch->color_str( AT_WHO5 );
   std::string s6 = ch->color_str( AT_WHO6 );
   std::string s7 = ch->color_str( AT_WHO7 );

   players.clear(  );
   immortals.clear(  );

   // Construct the two vectors.
   for( auto* d : dlist )
   {
      char_data *person = d->original ? d->original : d->character;

      if( person && d->connected >= CON_PLAYING )
      {
         if( !ch->can_see( person, true ) || is_ignoring( person, ch ) )
            continue;

         if( person->level >= LEVEL_IMMORTAL )
            immortals.push_back( person );
         else
            players.push_back( person );
      }
   }

   // Display any players who were visible to the person calling the command.
   if( !players.empty(  ) )
   {
      pcount += players.size(  );

      ch->pager_fmt( "\r\n{}--------------------------------=[ {}Players {}]=---------------------------------\r\n\r\n", s7, s6, s7 );

      for( auto* player : players )
      {
         ch->pager( output_person( ch, player ) );
      }
   }

   // Display any immortals who were visible to the person calling the command.
   if( !immortals.empty(  ) )
   {
      pcount += immortals.size(  );

      ch->pager_fmt( "\r\n{}-------------------------------=[ {}Immortals {}]=--------------------------------\r\n\r\n", s1, s6, s1 );

      for( auto* player : players )
      {
         ch->pager( output_person( ch, player ) );
      }
   }
   return pcount;
}

CMDF( do_who )
{
   int amount = 0, xx = 0, pcount = 0;

   std::string s1 = ch->color_str( AT_WHO );
   std::string s2 = ch->color_str( AT_WHO6 );
   std::string s3 = ch->color_str( AT_WHO2 );

   std::string outbuf = "";
   std::string buf = std::format( "{}-=[ {}{} {}]=-", s1, s2, sysdata->mud_name, s1 );
   std::string buf2 = std::format( "&R-=[ &W{} &R]=-", sysdata->mud_name );
   amount = 78 - color_strlen( buf2 ); /* Determine amount to put in front of line */

   if( amount < 1 )
      amount = 1;

   amount = amount / 2;

   for( xx = 0; xx < amount; ++xx )
      outbuf += " ";

   outbuf.append( buf );
   ch->pager_fmt( "{}\r\n", outbuf );

   pcount = afk_who( ch );

   ch->pager_fmt( "\r\n\r\n{}[{}{} Player{}{}] ", s3, s2, pcount, pcount == 1 ? "" : "s", s3 );

   ch->pager_fmt( "{}[{}Homepage: {}{}] [{}{} Max since reboot{}]\r\n", s3, s2, sysdata->http, s3, s2, sysdata->maxplayers, s3 );

   ch->pager_fmt( "{}[{}{} login{} since last reboot on {}{}]\r\n", s3, s2, num_logins, num_logins == 1 ? "" : "s", str_boot_time, s3 );
}

/*
 * Place any skill types you don't want them to be able to practice
 * normally in this list.  Separate each with a space.
 * (Uses a hasname check). -- Altrag
 */
void race_practice( char_data * ch, char_data * mob, int sn )
{
   short adept;
   int race = mob->race;

   if( ch->race != race )
   {
      act( AT_TELL, "$n tells you 'I cannot teach those of your race.'", mob, nullptr, ch, TO_VICT );
      return;
   }

   if( ( mob->level < ( skill_table[sn]->race_level[race] ) ) && skill_table[sn]->race_level[race] > 0 )
   {
      act( AT_TELL, "$n tells you 'You cannot learn that from me, you must find another...'", mob, nullptr, ch, TO_VICT );
      return;
   }

   if( ch->level < skill_table[sn]->race_level[race] )
   {
      act( AT_TELL, "$n tells you 'You're not ready to learn that yet...'", mob, nullptr, ch, TO_VICT );
      return;
   }

   if( hasname( "Tongue", skill_tname[skill_table[sn]->type] ) )
   {
      act( AT_TELL, "$n tells you 'I do not know how to teach that.'", mob, nullptr, ch, TO_VICT );
      return;
   }

   adept = ( short )( skill_table[sn]->race_adept[race] * 0.3 + ( ch->get_curr_int(  ) * 2 ) );

   if( ch->pcdata->learned[sn] >= adept )
   {
      act_printf( AT_TELL, mob, nullptr, ch, TO_VICT, "$n tells you, 'I've taught you everything I can about {}.'", skill_table[sn]->name );
      act( AT_TELL, "$n tells you, 'You'll have to practice it on your own now...'", mob, nullptr, ch, TO_VICT );
   }
   else
   {
      --ch->pcdata->practice;
      ch->pcdata->learned[sn] += ( 4 + int_app[ch->get_curr_int(  )].learn );
      act( AT_ACTION, "You practice $T.", ch, nullptr, skill_table[sn]->name.c_str(), TO_CHAR );
      act( AT_ACTION, "$n practices $T.", ch, nullptr, skill_table[sn]->name.c_str(), TO_ROOM );
      if( ch->pcdata->learned[sn] >= adept )
      {
         ch->pcdata->learned[sn] = adept;
         act( AT_TELL, "$n tells you. 'You'll have to practice it on your own now...'", mob, nullptr, ch, TO_VICT );
      }
   }
}

void display_practice( char_data * ch, char_data * mob, SKILL_INDEX * index, std::string_view header )
{
   SKILL_INDEX::iterator it, end;
   int cnt, sn, adept;
   skill_type *skill;

   std::string s1 = ch->color_str( AT_PRAC );
   std::string s2 = ch->color_str( AT_PRAC2 );
   std::string s3 = ch->color_str( AT_PRAC3 );
   std::string s4 = ch->color_str( AT_PRAC4 );

   it = index->begin(  );
   end = index->end(  );
   cnt = 0;

   for( ; it != end; ++it )
   {
      sn = it->second;

      // Invalid SN
      if( sn < 1 || sn >= MAX_SKILL )
         continue;

      // Null skill
      if( !( skill = skill_table[sn] ) )
         continue;

      // Get adept
      if( skill->type == SKILL_SKILL )
         adept = skill->skill_adept[ch->Class]; // skill->get_char_adept( ch, sn );
      else
         adept = skill->race_adept[ch->race];

      // Can't learn (yet)
      if( adept < 1 && !ch->is_immortal(  ) )
         continue;

      // If a teaching mob is present, skill displayed is based on the mob.
      if( mob )
      {
         if( mob->has_actflag( ACT_PRACTICE ) )
            if( skill_table[sn]->skill_level[mob->Class] > mob->level )
               continue;

         if( mob->has_actflag( ACT_TEACHER ) )
            if( skill_table[sn]->race_level[mob->race] > mob->level )
               continue;
      }
      else
      {
         if( ch->level < skill_table[sn]->skill_level[ch->Class] && ch->level < skill_table[sn]->race_level[ch->race] )
            continue;
      }

      // Guild only skills
      if( !ch->is_immortal(  ) && ( skill_table[sn]->guild != CLASS_NONE && ( !IS_GUILDED( ch ) ) ) )
         continue;

      // Secret teachers
      if( ch->pcdata->learned[sn] == 0 && SPELL_FLAG( skill_table[sn], SF_SECRETSKILL ) )
         continue;

      // Section Headers
      if( !cnt )
         ch->pager( std::vformat( std::string( header ), std::make_format_args( s4, s2, s4 ) ) );

      // Increment Count
      ++cnt;

      // Output Skill
      ch->pager_fmt( "{}{:22}{}{}({}{:11}{})", s2, skill->name,
                  ( ch->level < skill->skill_level[ch->Class] && ch->level < skill->race_level[ch->race] ) ? "&W*" : " ",
                  s1, s3, how_good( ch->pcdata->learned[sn] ), s1 );

      // Handle Columns
      if( ( cnt % 2 ) == 0 )
         ch->pager( "\r\n" );
   }

   // Add empty columns as necessary
   if( ( cnt % 2 ) )
      ch->pager( "\r\n" );
}

CMDF( do_practice )
{
   char_data *mob = nullptr;
   bool mobfound = false;

   if( ch->isnpc(  ) )
      return;

   for( auto* ich : ch->in_room->people )
   {
      if( ich->isnpc(  ) && ( ich->has_actflag( ACT_PRACTICE ) || ich->has_actflag( ACT_TEACHER ) ) )
      {
         mob = ich;
         mobfound = true;
         break;
      }
   }

   if( argument.empty(  ) )
   {
      std::string s1 = ch->color_str( AT_PRAC );
      std::string s4 = ch->color_str( AT_PRAC4 );

      ch->set_pager_color( AT_MAGIC );

      if( mobfound )
      {
         if( mob->has_actflag( ACT_PRACTICE ) )
         {
            if( mob->Class != ch->Class )
            {
               act( AT_TELL, "$n tells you 'I cannot teach those of your class.'", mob, nullptr, ch, TO_VICT );
               return;
            }
         }
         else
         {
            if( mob->race != ch->race )
            {
               act( AT_TELL, "$n tells you 'I cannot teach those of your race.'", mob, nullptr, ch, TO_VICT );
               return;
            }
         }
      }

      if( !mobfound )
         mob = nullptr;

      if( ch->is_immortal(  ) || ch->CAN_CAST(  ) )
      {
         display_practice( ch, mob, &skill_table__spell, "{}---------------------------------[ {}Spells  {}]-----------------------------------\r\n" );
      }
      display_practice( ch, mob, &skill_table__skill, "{}---------------------------------[ {}Skills  {}]-----------------------------------\r\n" );
      display_practice( ch, mob, &skill_table__racial, "{}---------------------------------[ {}Racials {}]-----------------------------------\r\n" );
      display_practice( ch, mob, &skill_table__combat, "{}---------------------------------[ {}Combat  {}]-----------------------------------\r\n" );
      display_practice( ch, mob, &skill_table__tongue, "{}---------------------------------[ {}Tongues {}]-----------------------------------\r\n" );
      display_practice( ch, mob, &skill_table__lore, "{}---------------------------------[ {}Lores   {}]-----------------------------------\r\n" );

      ch->pager( "&W*&D Indicates a skill you have not achieved the required level for yet.\r\n" );
      ch->pager_fmt( "{}You have {}{} {}practice sessions left.\r\n", s1, s4, ch->pcdata->practice, s1 );
      return;
   }
   else
   {
      if( !ch->IS_AWAKE(  ) )
      {
         ch->print( "In your dreams, or what?\r\n" );
         return;
      }

      if( !mobfound )
      {
         ch->print( "You can't do that here.\r\n" );
         return;
      }

      if( ch->pcdata->practice < 1 )
      {
         act( AT_TELL, "$n tells you 'You must earn some more practice sessions.'", mob, nullptr, ch, TO_VICT );
         return;
      }

      int sn = 0;
      if( ( sn = skill_lookup( argument ) ) == -1 )
      {
         act_printf( AT_TELL, mob, nullptr, ch, TO_VICT, "$n tells you 'I've never heard of {}. Are you sure you know what you want?'", argument );
         return;
      }

      if( mob->has_actflag( ACT_TEACHER ) )
      {
         race_practice( ch, mob, sn );
         return;
      }

      if( ch->Class != mob->Class )
      {
         act( AT_TELL, "$n tells you 'I cannot teach those of your Class.'", mob, nullptr, ch, TO_VICT );
         return;
      }

      if( skill_table[sn]->skill_level[mob->Class] > LEVEL_AVATAR )
      {
         act( AT_TELL, "$n tells you 'Only an immortal of your Class may learn that.'", mob, nullptr, ch, TO_VICT );
         return;
      }

      if( ( mob->level < ( skill_table[sn]->skill_level[mob->Class] ) ) && skill_table[sn]->skill_level[mob->Class] > 0 )
      {
         act( AT_TELL, "$n tells you 'You cannot learn that from me, you must find another...'", mob, nullptr, ch, TO_VICT );
         return;
      }

      if( ch->level < skill_table[sn]->skill_level[mob->Class] )
      {
         act( AT_TELL, "$n tells you 'You're not ready to learn that yet...'", mob, nullptr, ch, TO_VICT );
         return;
      }

      if( hasname( "Tongue", skill_tname[skill_table[sn]->type] ) )
      {
         act( AT_TELL, "$n tells you 'I do not know how to teach that.'", mob, nullptr, ch, TO_VICT );
         return;
      }

      /*
       * Skill requires a special teacher
       */
      if( !skill_table[sn]->teachers.empty() )
      {
         std::string buf = std::to_string( mob->pIndexData->vnum );
         if( !hasname( skill_table[sn]->teachers, buf ) )
         {
            act( AT_TELL, "$n tells you, 'You must find a specialist to learn that!'", mob, nullptr, ch, TO_VICT );
            return;
         }
      }

      /*
       * Guild checks - right now, can't practice guild skills - done on induct/outcast
       */
      /*
       * if( !ch->isnpc() && !IS_GUILDED(ch) && skill_table[sn]->guild != CLASS_NONE )
       * {
       * act( AT_TELL, "$n tells you 'Only guild members can use that..'", mob, nullptr, ch, TO_VICT );
       * return;
       * }
       * 
       * if( !ch->isnpc() && skill_table[sn]->guild != CLASS_NONE && ch->pcdata->clan->Class != skill_table[sn]->guild )
       * {
       * act( AT_TELL, "$n tells you 'That I can not teach to your guild.'", mob, nullptr, ch, TO_VICT );
       * return;
       * }
       * 
       * if( !ch->isnpc() && skill_table[sn]->guild != CLASS_NONE )
       * {
       * act( AT_TELL, "$n tells you 'That is only for members of guilds...'", mob, nullptr, ch, TO_VICT );
       * return;
       * }
       */

      short adept = ( short )( class_table[ch->Class]->skill_adept * 0.3 + ( ch->get_curr_int(  ) * 2 ) );

      if( ch->pcdata->learned[sn] >= adept )
      {
         act_printf( AT_TELL, mob, nullptr, ch, TO_VICT, "$n tells you, 'I've taught you everything I can about {}.'", skill_table[sn]->name );
         act( AT_TELL, "$n tells you, 'You'll have to practice it on your own now...'", mob, nullptr, ch, TO_VICT );
      }
      else
      {
         --ch->pcdata->practice;
         ch->pcdata->learned[sn] += ( 4 + int_app[ch->get_curr_int(  )].learn );
         act( AT_ACTION, "You practice $T.", ch, nullptr, skill_table[sn]->name.c_str(), TO_CHAR );
         act( AT_ACTION, "$n practices $T.", ch, nullptr, skill_table[sn]->name.c_str(), TO_ROOM );
         if( ch->pcdata->learned[sn] >= adept )
         {
            ch->pcdata->learned[sn] = adept;
            act( AT_TELL, "$n tells you. 'You'll have to practice it on your own now...'", mob, nullptr, ch, TO_VICT );
         }
      }
   }
}

CMDF( do_group )
{
   char_data *victim = nullptr;

   if( argument.empty(  ) )
   {
      char_data *leader = ch->leader ? ch->leader : ch;
      ch->print_fmt( "{}{}'s{} group:\r\n", ch->color_str( AT_SCORE3 ), leader->name, ch->color_str( AT_SCORE ) );

      for( auto* gch : charlist )
      {
         if( is_same_group( gch, ch ) )
         {
            ch->
               print_fmt
               ( "&[score3]{:<50}&[score] HP:&[score2]{:2.0f}&[score]% {:4}:&[score2]{:2.0f}&[score]% MV:&[score2]{:2.0f}&[score]%\r\n",
                 capitalize( gch->name ), ( ( float )gch->hit / gch->max_hit ) * 100 + 0.5, "MANA",
                 ( ( float )gch->mana / gch->max_mana ) * 100 + 0.5, ( ( float )gch->move / gch->max_move ) * 100 + 0.5 );
         }
      }
      return;
   }

   if( !str_cmp( argument, "disband" ) )
   {
      int count = 0;

      if( ch->leader || ch->master )
      {
         ch->print( "You cannot disband a group if you're following someone.\r\n" );
         return;
      }

      for( auto* gch : charlist )
      {
         if( is_same_group( ch, gch ) && ( ch != gch ) )
         {
            gch->leader = nullptr;
            if( ( gch->isnpc(  ) && !check_pets( ch, gch->pIndexData ) ) || !gch->isnpc(  ) )
               gch->master = nullptr;
            ++count;
            gch->print( "Your group is disbanded.\r\n" );
         }
      }
      if( count == 0 )
         ch->print( "You have no group members to disband.\r\n" );
      else
         ch->print( "You disband your group.\r\n" );
      return;
   }

   if( !str_cmp( argument, "all" ) )
   {
      int count = 0;

      for( auto* rch : ch->in_room->people )
      {
         if( ch != rch && !rch->isnpc(  ) && ch->can_see( rch, false ) && rch->master == ch
             && !ch->master && !ch->leader && !is_same_group( rch, ch ) && ch->IS_PKILL(  ) == rch->IS_PKILL(  ) )
         {
            rch->leader = ch;
            ++count;
         }
      }
      if( count == 0 )
         ch->print( "You have no eligible group members.\r\n" );
      else
      {
         act( AT_ACTION, "$n groups $s followers.", ch, nullptr, victim, TO_ROOM );
         ch->print( "You group your followers.\r\n" );
      }
      return;
   }

   if( !( victim = ch->get_char_room( argument ) ) )
   {
      ch->print( "They aren't here.\r\n" );
      return;
   }

   if( ch->master || ( ch->leader && ch->leader != ch ) )
   {
      ch->print( "But you are following someone else!\r\n" );
      return;
   }

   if( victim->master != ch && ch != victim )
   {
      act( AT_PLAIN, "$N isn't following you.", ch, nullptr, victim, TO_CHAR );
      return;
   }

   if( is_same_group( victim, ch ) && ch != victim )
   {
      victim->leader = nullptr;
      act( AT_ACTION, "$n removes $N from $s group.", ch, nullptr, victim, TO_NOTVICT );
      act( AT_ACTION, "$n removes you from $s group.", ch, nullptr, victim, TO_VICT );
      act( AT_ACTION, "You remove $N from your group.", ch, nullptr, victim, TO_CHAR );
      return;
   }

   victim->leader = ch;
   act( AT_ACTION, "$N joins $n's group.", ch, nullptr, victim, TO_NOTVICT );
   act( AT_ACTION, "You join $n's group.", ch, nullptr, victim, TO_VICT );
   act( AT_ACTION, "$N joins your group.", ch, nullptr, victim, TO_CHAR );
}

CMDF( do_attrib )
{
   std::string suf;
   int iLang;
   short day;

   if( ch->isnpc(  ) )
   {
      ch->print( "Huh?\r\n" );
      return;
   }

   day = ch->pcdata->day + 1;

   if( day > 4 && day < 20 )
      suf = "th";
   else if( day % 10 == 1 )
      suf = "st";
   else if( day % 10 == 2 )
      suf = "nd";
   else if( day % 10 == 3 )
      suf = "rd";
   else
      suf = "th";

   std::string s1 = ch->color_str( AT_SCORE );
   std::string s2 = ch->color_str( AT_SCORE2 );
   std::string s3 = ch->color_str( AT_SCORE3 );
   std::string s4 = ch->color_str( AT_SCORE4 );
   std::string s5 = ch->color_str( AT_SCORE5 );

   ch->printf( "%sYou are %s%d %syears old.\r\n", s2.c_str(), s3.c_str(), ch->get_age(  ), s2.c_str() );

   ch->printf( "%sYou are %s%d%s inches tall, and weigh %s%d%s lbs.\r\n", s2.c_str(), s3.c_str(), ch->height, s2.c_str(), s3.c_str(), ch->weight, s2.c_str() );

   if( time_info.day == ch->pcdata->day && time_info.month == ch->pcdata->month )
      ch->printf( "%sToday is your birthday!\r\n", s2.c_str() );
   else
      ch->printf( "%sYour birthday is: %sDay of %s, %d%s day in the Month of %s, in the year %d.\r\n",
                  s2.c_str(), s1.c_str(), day_name[ch->pcdata->day % sysdata->daysperweek], day, suf.c_str(), month_name[ch->pcdata->month], ch->pcdata->year );

   std::string time_played = std::format( "{}", ch->time_played( ) );
   ch->printf( "%sYou have played for %s%s %shours.\r\n", s2.c_str(), s3.c_str(), time_played.c_str(), s1.c_str() );

   if( ch->pcdata->deity )
      ch->printf( "%sYou have devoted to %s%s.\r\n", s2.c_str(), s3.c_str(), ch->pcdata->deity->name.c_str(  ) );

   ch->printf( "\r\n%sLanguages: ", s2.c_str() );

   for( iLang = 0; iLang < LANG_UNKNOWN; ++iLang )
   {
      if( knows_language( ch, iLang, ch ) > 0 )
      {
         if( iLang == ch->speaking )
            ch->print( s3 );
         ch->printf( " %s%s", lang_names[iLang], s1.c_str() );
      }
   }
   ch->print( "\r\n\r\n" );

   ch->printf( "%sLogin: %s%s\r\n", s2.c_str(), s3.c_str(), c_time( ch->pcdata->logon, ch->pcdata->timezone ).c_str() );

   std::string save_time = std::format( "{}", ch->pcdata->save_time );
   ch->printf( "%sSaved: %s%s\r\n", s2.c_str(), s3.c_str(), !save_time.empty() ? c_time( ch->pcdata->save_time, ch->pcdata->timezone ).c_str() : "no save this session" );
   ch->printf( "%sTime : %s%s\r", s2.c_str(), s3.c_str(), c_time( current_time, ch->pcdata->timezone ).c_str() );

   ch->printf( "\r\n%sMKills : %s%d\r\n", s2.c_str(), s3.c_str(), ch->pcdata->mkills );
   ch->printf( "%sMDeaths: %s%d\r\n\r\n", s2.c_str(), s3.c_str(), ch->pcdata->mdeaths );

   if( ch->pcdata->clan && ch->pcdata->clan->clan_type == CLAN_GUILD )
   {
      ch->printf( "%sGuild         : %s%s\r\n", s2.c_str(), s3.c_str(), ch->pcdata->clan->name.c_str(  ) );
      ch->printf( "%sGuild MDeaths : %s%-6d      %sGuild MKills: %s%d\r\n\r\n", s2.c_str(), s3.c_str(), ch->pcdata->clan->mdeaths, s2.c_str(), s3.c_str(), ch->pcdata->clan->mkills );
   }

   if( ch->CAN_PKILL(  ) )
   {
      ch->printf( "%sPKills        : %s%-6d      %sClan        : %s%s\r\n", s2.c_str(), s3.c_str(), ch->pcdata->pkills, s2.c_str(), s3.c_str(), ch->pcdata->clan ? ch->pcdata->clan->name.c_str(  ) : "None" );
      ch->printf( "%sIllegal PKills: %s%-6d      %sClan PKills : %s%d\r\n", s2.c_str(), s3.c_str(), ch->pcdata->illegal_pk, s2.c_str(), s3.c_str(), ( ch->pcdata->clan ? ch->pcdata->clan->pkills[0] : -1 ) );
      ch->printf( "%sPDeaths       : %s%-6d      %sClan PDeaths: %s%d\r\n", s2.c_str(), s3.c_str(), ch->pcdata->pdeaths, s2.c_str(), s3.c_str(), ( ch->pcdata->clan ? ch->pcdata->clan->pdeaths[0] : -1 ) );
   }
}

CMDF( do_score )
{
   std::string s1 = ch->color_str( AT_SCORE );
   std::string s2 = ch->color_str( AT_SCORE2 );
   std::string s3 = ch->color_str( AT_SCORE3 );
   std::string s4 = ch->color_str( AT_SCORE4 );
   std::string s5 = ch->color_str( AT_SCORE5 );

   ch->print_fmt( "{}Score for {}{}\r\n", s1, ch->name, ch->pcdata->title );
   ch->print_fmt( "{}--------------------------------------------------------------------------------\r\n", s5 );

   ch->print_fmt( "{}Level: {}{:<15} {}HitPoints: {}{:5}{}/{}{:5}      {}Pager    {}({}{}{}) {}{:4}\r\n",
                  s2, s3, ch->level, s2, s3, ch->hit, s1, s4, ch->max_hit, s2, s1, s3, ch->has_pcflag( PCFLAG_PAGERON ) ? "X" : " ", s1, s4, ch->pcdata->pagerlen );

   ch->print_fmt( "{}Race : {}{:<15.15} {}Mana     : {}{:5}{}/{}{:5}      {}Autoexit {}({}{}{})\r\n",
                  s2, s3, capitalize( npc_race[ch->race] ), s2, s3, ch->mana, s1, s4, ch->max_mana, s2, s1, s3, ch->has_pcflag( PCFLAG_AUTOEXIT ) ? "X" : " ", s1 );

   ch->print_fmt( "{}Class: {}{:<15.15} {}Movement : {}{:5}{}/{}{:5}      {}Autoloot {}({}{}{})\r\n",
                  s2, s3, capitalize( npc_class[ch->Class] ), s2, s3, ch->move, s1, s4, ch->max_move, s2, s1, s3, ch->has_pcflag( PCFLAG_AUTOLOOT ) ? "X" : " ", s1 );

   ch->print_fmt( "{}Align: {}{:<15} {}To Hit   : {}{}{:<9}       {}Autosac  {}({}{}{})\r\n",
                  s2, s3, ch->alignment, s2, s3, ch->GET_HITROLL(  ) > 0 ? "+" : "", ch->GET_HITROLL(  ), s2, s1, s3, ch->has_pcflag( PCFLAG_AUTOSAC ) ? "X" : " ", s1 );

   if( ch->level < 10 )
   {
      ch->print_fmt( "{}STR  : {}{:<15.15} {}To Dam   : {}{}{:<9}       {}Smartsac {}({}{}{})\r\n",
                     s2, s3, attribtext( ch->get_curr_str(  ) ), s2, s3, ch->GET_DAMROLL(  ) > 0 ? "+" : "",
                     ch->GET_DAMROLL(  ), s2, s1, s3, ch->has_pcflag( PCFLAG_SMARTSAC ) ? "X" : " ", s1 );

      ch->print_fmt( "{}INT  : {}{:<15.15} {}AC       : {}{}{}\r\n", s2, s3, attribtext( ch->get_curr_int(  ) ), s2, s3, ch->GET_AC(  ) > 0 ? "+" : "", ch->GET_AC(  ) );

      ch->print_fmt( "{}WIS  : {}{:<15.15} {}Wimpy    : {}{}\r\n", s2, s3, attribtext( ch->get_curr_wis(  ) ), s2, s3, ch->wimpy );

      ch->print_fmt( "{}DEX  : {}{:<15.15} {}Exp      : {}{}\r\n", s2, s3, attribtext( ch->get_curr_dex(  ) ), s2, s3, ch->exp );

      ch->print_fmt( "{}CON  : {}{:<15.15} {}Gold     : {}{}\r\n", s2, s3, attribtext( ch->get_curr_con(  ) ), s2, s3, ch->gold );

      ch->print_fmt( "{}CHA  : {}{:<15.15} {}Weight   : {}{}{}/{}{}\r\n",
                     s2, s3, attribtext( ch->get_curr_cha(  ) ), s2, s3, ch->carry_weight, s1, s4, ch->can_carry_w(  ) );

      ch->print_fmt( "{}LCK  : {}{:<15.15} {}Items    : {}{}{}/{}{}\r\n",
                     s2, s3, attribtext( ch->get_curr_lck(  ) ), s2, s3, ch->carry_number, s1, s4, ch->can_carry_n(  ) );
   }
   else
   {
      ch->print_fmt( "{}STR  : {}{:<2}{}/{}{:<12} {}To Dam   : {}{}{:<9}       {}Smartsac {}({}{}{})\r\n",
                     s2, s3, ch->get_curr_str(  ), s1, s4, ch->perm_str, s2, s3, ch->GET_DAMROLL(  ) > 0 ? "+" : " ",
                     ch->GET_DAMROLL(  ), s2, s1, s3, ch->has_pcflag( PCFLAG_SMARTSAC ) ? "X" : " ", s1 );

      ch->print_fmt( "{}INT  : {}{:<2}{}/{}{:<12} {}AC       : {}{}{}\r\n",
                     s2, s3, ch->get_curr_int(  ), s1, s4, ch->perm_int, s2, s3, ch->GET_AC(  ) > 0 ? "+" : "", ch->GET_AC(  ) );

      ch->print_fmt( "{}WIS  : {}{:<2}{}/{}{:<12} {}Wimpy    : {}{}\r\n", s2, s3, ch->get_curr_wis(  ), s1, s4, ch->perm_wis, s2, s3, ch->wimpy );

      ch->print_fmt( "{}DEX  : {}{:<2}{}/{}{:<12} {}Exp      : {}{}\r\n", s2, s3, ch->get_curr_dex(  ), s1, s4, ch->perm_dex, s2, s3, ch->exp );

      ch->print_fmt( "{}CON  : {}{:<2}{}/{}{:<12} {}Gold     : {}{}\r\n", s2, s3, ch->get_curr_con(  ), s1, s4, ch->perm_con, s2, s3, ch->gold );

      ch->print_fmt( "{}CHA  : {}{:<2}{}/{}{:<12} {}Weight   : {}{}{}/{}{}\r\n",
                     s2, s3, ch->get_curr_cha(  ), s1, s4, ch->perm_cha, s2, s3, ch->carry_weight, s1, s4, ch->can_carry_w(  ) );

      ch->print_fmt( "{}LCK  : {}{:<2}{}/{}{:<12} {}Items    : {}{}{}/{}{}\r\n",
                     s2, s3, ch->get_curr_lck(  ), s1, s4, ch->perm_lck, s2, s3, ch->carry_number, s1, s4, ch->can_carry_n(  ) );
   }

   ch->print_fmt( "{}Pracs: {}{:<15} {}Favor    : {}{}\r\n\r\n", s2, s3, ch->pcdata->practice, s2, s3, ch->pcdata->favor );

   ch->print_fmt( "{}You are {}.\r\n", s2, npc_position[ch->position] );

   if( ch->pcdata->condition[COND_DRUNK] > 10 )
      ch->print_fmt( "{}You are drunk.\r\n", s2 );

   if( ch->pcdata->condition[COND_THIRST] == 0 )
      ch->print_fmt( "{}You are extremely thirsty.\r\n", s2 );

   if( ch->pcdata->condition[COND_FULL] == 0 )
      ch->print_fmt( "{}You are starving.\r\n", s2 );

   ch->set_color( AT_SCORE2 );
   if( ch->position != POS_SLEEPING )
   {
      switch ( ch->mental_state / 10 )
      {
         default:
            ch->print( "You're completely messed up!\r\n" );
            break;
         case -10:
            ch->print( "You're barely conscious.\r\n" );
            break;
         case -9:
            ch->print( "You can barely keep your eyes open.\r\n" );
            break;
         case -8:
            ch->print( "You're extremely drowsy.\r\n" );
            break;
         case -7:
            ch->print( "You feel very unmotivated.\r\n" );
            break;
         case -6:
            ch->print( "You feel sedated.\r\n" );
            break;
         case -5:
            ch->print( "You feel sleepy.\r\n" );
            break;
         case -4:
            ch->print( "You feel tired.\r\n" );
            break;
         case -3:
            ch->print( "You could use a rest.\r\n" );
            break;
         case -2:
            ch->print( "You feel a little under the weather.\r\n" );
            break;
         case -1:
            ch->print( "You feel fine.\r\n" );
            break;
         case 0:
            ch->print( "You feel great.\r\n" );
            break;
         case 1:
            ch->print( "You feel energetic.\r\n" );
            break;
         case 2:
            ch->print( "Your mind is racing.\r\n" );
            break;
         case 3:
            ch->print( "You can't think straight.\r\n" );
            break;
         case 4:
            ch->print( "Your mind is going 100 miles an hour.\r\n" );
            break;
         case 5:
            ch->print( "You're high as a kite.\r\n" );
            break;
         case 6:
            ch->print( "Your mind and body are slipping apart.\r\n" );
            break;
         case 7:
            ch->print( "Reality is slipping away.\r\n" );
            break;
         case 8:
            ch->print( "You have no idea what is real, and what is not.\r\n" );
            break;
         case 9:
            ch->print( "You feel immortal.\r\n" );
            break;
         case 10:
            ch->print( "You are a Supreme Entity.\r\n" );
            break;
      }
   }
   else
   {
      if( ch->mental_state > 45 )
         ch->print( "Your sleep is filled with strange and vivid dreams.\r\n" );
      else if( ch->mental_state > 25 )
         ch->print( "Your sleep is uneasy.\r\n" );
      else if( ch->mental_state < -35 )
         ch->print( "You are deep in a much needed sleep.\r\n" );
      else if( ch->mental_state < -25 )
         ch->print( "You are in deep slumber.\r\n" );
   }

   if( ch->desc )
   {
      ch->print_fmt( "\r\n{}Terminal Support Detected: MCCP{}[{}{}{}] {}MSP{}[{}{}{}]\r\n",
                     s2, s1, s3, (ch->desc->can_compress ? "X" : " "), s1, s2, s1, s3, (ch->desc->msp_detected ? "X" : " "), s1 );

      ch->print_fmt( "{}Terminal Support In Use  : MCCP{}[{}{}{}] {}MSP{}[{}{}{}]\r\n",
                     s2, s1, s3, (ch->desc->can_compress ? "X" : " "), s1, s2, s1, s3, (ch->MSP_ON(  )? "X" : " "), s1 );
   }

   if( !ch->pcdata->bestowments.empty(  ) )
      ch->print_fmt( "\r\n{}You are bestowed with the command(s): {}{}.\r\n", s2, s3, ch->pcdata->bestowments );

   if( ch->is_immortal(  ) )
   {
      ch->print_fmt( "\r\n{}--------------------------------------------------------------------------------\r\n", s5 );

      ch->print_fmt( "{}IMMORTAL DATA: Wizinvis {}({}{}{}) {}Level {}({}{}{}) {}Realm: {}({}{}{})\r\n",
                     s2, s1, s3, ch->has_pcflag( PCFLAG_WIZINVIS ) ? "X" : " ", s1,
                     s2, s1, s3, ch->pcdata->wizinvis, s1,
                     s2, s1, s3, ch->pcdata->realm_name, s1 );

      ch->print_fmt( "{}Bamfin:  {}{}\r\n", s2, s1, ( !ch->pcdata->bamfin.empty() ) ? ch->pcdata->bamfin : "appears in a swirling mist." );
      ch->print_fmt( "{}Bamfout: {}{}\r\n", s2, s1, ( !ch->pcdata->bamfout.empty() ) ? ch->pcdata->bamfout : "leaves in a swirling mist." );

      /*
       * Area Loaded info - Scryn 8/11
       */
      if( ch->pcdata->area )
      {
         ch->print_fmt( "{}Vnums: {}{:10} - {:10}\r\n", s2, s1, ch->pcdata->area->low_vnum, ch->pcdata->area->hi_vnum );
      }
   }

   if( !ch->affects.empty(  ) )
   {
      skill_type *sktmp;

      ch->print_fmt( "\r\n{}--------------------------------------------------------------------------------\r\n", s5 );
      ch->print_fmt( "{}Affect Data:\r\n\r\n", s2 );

      for( auto* af : ch->affects )
      {
         if( !( sktmp = get_skilltype( af->type ) ) )
            continue;

         std::string buf = std::format( "{}'{}{}{}'", s1, s3, sktmp->name, s1 );

         ch->print_fmt( "{}{} : {:<20} {}({}{} hours{})\r\n", s2, skill_tname[sktmp->type], buf, s1, s4, af->duration / DUR_CONV, s1 );
      }
   }
}

obj_data *find_quill( char_data * ch )
{
   for( auto* quill : ch->carrying )
   {
      if( quill->item_type == ITEM_PEN && ch->can_see_obj( quill, false ) )
         return quill;
   }

   return nullptr;
}

/*
 * Journal command. Allows users to write notes to an object of type "journal".
 * Options are Write, Read and Size. Write and Read options require a numerical
 * argument. Option Size retrieves v0 or value0 from the object, which is indicative
 * of how many pages are in the journal.
 *
 * Forced a maximum limit of 50 pages to all journals, just in case someone slipped
 * with a value command and we ended up with an object that could store 500 pages.
 * This is added in journal write and journal size. Leart.
 */
CMDF( do_journal )
{
   std::string arg1, arg2, buf;
   extra_descr_data *ed;
   obj_data *quill = nullptr, *journal = nullptr;
   int pages;
   int anum = 0;

   if( ch->isnpc() )
      return;

   if( !ch->desc )
   {
      bug( "{}: no descriptor", __func__ );
      return;
   }

   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );

   switch ( ch->substate )
   {
      default:
         break;

      case SUB_JOURNAL_WRITE:
         if( ( journal = ch->get_eq( WEAR_HOLD ) ) == nullptr || journal->item_type != ITEM_JOURNAL )
         {
            bug( "{}: Player not holding journal. (Player: {})", __func__, ch->name );
            ch->stop_editing( );
            return;
         }
         ed = ( extra_descr_data * )ch->pcdata->dest_buf;
         ed->desc = ch->copy_buffer( );
         ch->stop_editing( );
         ch->substate = ch->tempnum;
         return;

      case SUB_EDIT_ABORT:
         ch->substate = ch->tempnum;
         ch->print( "Aborting journal entry.\r\n" );
         return;
   }

   if( arg1[0] == '\0' )
   {
      ch->print( "Syntax: Journal <command>\r\n\r\n" );
      ch->print( "Where command is one of:\r\n" );
      ch->print( "write read size\r\n" );
      return;
   }

   /* 
    * Write option. Allows user to enter the buffer, adding an extra_desc to the
    * journal object called "PageX" where X is the argument associated with the write command
    */
   if( !str_cmp( arg1, "write" ) )
   {
      if( ( journal = ch->get_eq( WEAR_HOLD ) ) == nullptr || journal->item_type != ITEM_JOURNAL )
      {
         ch->print( "You must be holding a journal in order to write in it.\r\n" );
         return;
      }

      if( arg2[0] == '\0' || !is_number( arg2 ) )
      {
         ch->print( "Syntax: Journal write <number>\r\n" );
         return;
      }

      quill = find_quill( ch );
      if( !quill )
      {
         ch->print( "You need a quill to write in your journal.\r\n" );
         return;
      }

      if( quill->value[0] < 1 )
      {
         ch->print( "Your quill is dry.\r\n" );
         return;
      }

      if( journal->value[0] < 1 )
      {
         ch->print( "There are no pages in this journal. Seek an immortal for assistance.\r\n" );
         return;
      }

      /* Force a max value of 50 */
      if( journal->value[0] > 50 )
      {
         journal->value[0] = 50;
         bug( "{}: Journal size greater than 50 pages! Resetting to 50 pages. (Player: {})", __func__, ch->name );
      }

      ch->set_color( AT_GREY );
      pages = journal->value[0];
      if( is_number( arg2 ) )
      {
         anum = atoi( arg2.c_str() );
      }

      if( pages < anum )
      {
         ch->print( "That page does not exist in this journal.\r\n" );
         return;
      }

      /* Making the edits turn out to be "page1" etc - just so people can't/don't type "look 1" */
      buf = std::format( "page {}", arg2 );

      ed = new extra_descr_data;
      ed->keyword = buf;
      ch->substate = SUB_JOURNAL_WRITE;
      ch->pcdata->dest_buf = ed;
      --quill->value[0];
      ch->set_editor_desc( "Writing a journal entry." );
      ch->start_editing( ed->desc );
      journal->value[1]++;

      return;
   }

   /* Size option, returns how many pages are in the journal */
   if( !str_cmp( arg1, "size" ) )
   {
      if( ( journal = ch->get_eq( WEAR_HOLD ) ) == nullptr || journal->item_type != ITEM_JOURNAL )
      {
         ch->print( "You must be holding a journal in order to check it's size.\r\n" );
         return;
      }

      if( journal->value[0] < 1 )
      {
         ch->print( "There are no pages in this journal. Seek an immortal for assistance.\r\n" );
      }
      else
      {
         if( journal->value[0] > 50 )
         {
            journal->value[0] = 50;
            bug( "{}: Journal size greater than 50 pages! Resetting to 50 pages. (Player: {})", __func__, ch->name );
         }
         ch->set_color( AT_GREY );
         ch->print_fmt( "There are {} pages in this journal.\r\n", journal->value[0] );
         return;
      }
   }

   /*
    * Read option. Players can read the desc on the journal by typing "look page1", but I thought about putting
    * in this option anyway.
    */
   if( !str_cmp( arg1, "read" ) )
   {
      if( arg2[0] == '\0' )
      {
         ch->print( "Syntax: Journal read <number>\r\n" );
         return;
      }

      if( !is_number( arg2 ) )
      {
         ch->print( "Syntax: Journal read <number>\r\n" );
         return;
      }

      if( is_number( arg2 ) )
      {
         anum = std::stoi( arg2 );
      }

      if( ( journal = ch->get_eq( WEAR_HOLD ) ) == nullptr || journal->item_type != ITEM_JOURNAL )
      {
         ch->print( "You must be holding a journal in order to read it.\r\n" );
         return;
      }

      if( journal->value[0] > 50 )
      {
         journal->value[0] = 50;
         bug( "{}: Journal size greater than 50 pages! Resetting to 50 pages. (Player: {})", __func__, ch->name );
      }

      ch->set_color( AT_GREY );
      pages = journal->value[0];
      if( pages < anum )
      {
         ch->print( "That page does not exist in this journal.\r\n" );
         return;
      }

      buf = std::format( "page {}", arg2 );

      if( ( ed = get_extra_descr( buf, journal ) ) == nullptr )
         ch->print( "That journal page is blank.\r\n" );
      else
         ch->print( ed->desc );
      return;
   }

   do_journal( ch, "" );
}
