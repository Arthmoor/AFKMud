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
 *            Commands for personal player settings/statictics              *
 ****************************************************************************
 *                           Pet handling module                            *
 *                      Created by Samson of Alsherok                       *
 ****************************************************************************/

#include <sstream>
#include <sys/stat.h>
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

char *default_fprompt( char_data * );
char *default_prompt( char_data * );

extern int num_logins;

bool exists_player( const string & name )
{
   struct stat fst;
   char buf[256];

   /*
    * Stands to reason that if there ain't a name to look at, they damn well don't exist! 
    */
   if( name.empty(  ) )
      return false;

   snprintf( buf, 256, "%s%c/%s", PLAYER_DIR, tolower( name[0] ), capitalize( name ).c_str(  ) );

   if( stat( buf, &fst ) != -1 )
      return true;

   else if( supermob->get_char_world( name ) != nullptr )
      return true;

   return false;
}

/* Rank buffer code */
string rankbuffer( char_data * ch )
{
   ostringstream rbuf;

   if( ch->is_immortal(  ) )
   {
      if( ch->pcdata->rank && ch->pcdata->rank[0] != '\0' )
         rbuf << "&Y" << ch->pcdata->rank;
      else
         rbuf << "&Y" << ch->pcdata->realm_name;
   }
   else
   {
      if( ch->pcdata->rank && ch->pcdata->rank[0] != '\0' )
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

const string how_good( int percent )
{
   string buf;

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
   list < char_data * >::iterator ich;

   for( ich = ch->pets.begin(  ); ich != ch->pets.end(  ); ++ich )
   {
      char_data *mob = *ich;

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
      ch->printf( "Pets for %s:\r\n", victim->name );
      ch->print( "--------------------------------------------------------------------\r\n" );

      list < char_data * >::iterator ipet;
      for( ipet = victim->pets.begin(  ); ipet != victim->pets.end(  ); ++ipet )
      {
         char_data *pet = *ipet;

         if( !pet->in_room )
            continue;

         ch->printf( "[%5d] %-28s [%5d] %s\r\n", pet->pIndexData->vnum, pet->short_descr, pet->in_room->vnum, pet->in_room->name );
      }
   }
   else
   {
      ch->print( "Character           | Follower\r\n" );
      ch->print( "-----------------------------------------------------------------\r\n" );
      list < char_data * >::iterator ich;
      for( ich = pclist.begin(  ); ich != pclist.end(  ); ++ich )
      {
         char_data *vch = *ich;

         list < char_data * >::iterator ipet;

         for( ipet = vch->pets.begin(  ); ipet != vch->pets.end(  ); ++ipet )
         {
            char_data *pet = ( *ipet );

            if( !pet->in_room )
               continue;

            ch->printf( "[%5d] %-28s [%5d] %s\r\n", pet->pIndexData->vnum, pet->short_descr, pet->in_room->vnum, pet->in_room->name );
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
   list < char_data * >::iterator ich;
   for( ich = ch->in_room->people.begin(  ); ich != ch->in_room->people.end(  ); ++ich )
   {
      char_data *gch = *ich;

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

   ch->printf( "&[gold]You split %d gold coins. Your share is %d gold coins.\r\n", amount, share + extra );

   for( ich = ch->in_room->people.begin(  ); ich != ch->in_room->people.end(  ); ++ich )
   {
      char_data *gch = *ich;

      if( gch != ch && is_same_group( gch, ch ) )
      {
         act_printf( AT_GOLD, ch, nullptr, gch, TO_VICT, "$n splits %d gold coins. Your share is %d gold coins.", amount, share );
         gch->gold += share;
      }
   }
}

CMDF( do_gold )
{
   ch->printf( "&[gold]You have %d gold pieces.\r\n", ch->gold );
}

/* Spits back a word for a stat being rolled or viewed in score - Samson 12-20-00 */
const string attribtext( int attribute )
{
   string atext;

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
const string condtxt( int current, int base )
{
   string text;

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
   list < string >::iterator zl;

   /*
    * Save the list of zones PC has visited - Samson 7-11-00 
    */
   for( zl = zone.begin(  ); zl != zone.end(  ); ++zl )
   {
      string zn = *zl;

      fprintf( fp, "Zone         %s~\n", zn.c_str(  ) );
   }
}

void pc_data::load_zonedata( FILE * fp )
{
   string zonename;
   bool found = false;

   fread_string( zonename, fp );

   list < area_data * >::iterator iarea;
   for( iarea = arealist.begin(  ); iarea != arealist.end(  ); ++iarea )
   {
      area_data *tarea = *iarea;

      if( !str_cmp( tarea->name, zonename ) )
      {
         found = true;
         break;
      }
   }

   if( !found )
      return;

   list < string >::iterator zl;
   for( zl = zone.begin(  ); zl != zone.end(  ); ++zl )
   {
      string zn = *zl;

      if( zn >= zonename )
      {
         zone.insert( zl, zonename );
         return;
      }
   }
   zone.push_back( zonename );
}

/* Functions for use with area visiting code - Samson 7-11-00  */
bool char_data::has_visited( area_data * area )
{
   list < string >::iterator zl;

   if( isnpc(  ) )
      return true;

   for( zl = pcdata->zone.begin(  ); zl != pcdata->zone.end(  ); ++zl )
   {
      string zn = *zl;

      if( !str_cmp( area->name, zn ) )
         return true;
   }
   return false;
}

void char_data::update_visits( room_index * room )
{
   list < string >::iterator zl;

   if( isnpc(  ) || has_visited( room->area ) )
      return;

   string zonename = room->area->name;

   for( zl = pcdata->zone.begin(  ); zl != pcdata->zone.end(  ); ++zl )
   {
      string zn = *zl;

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
   list < string >::iterator zl;
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
      for( zl = ch->pcdata->zone.begin(  ); zl != ch->pcdata->zone.end(  ); ++zl )
      {
         string zn = *zl;

         ch->pagerf( "%s\r\n", zn.c_str(  ) );
         ++visits;
      }
      ch->pagerf( "&YTotal areas visited: %d\r\n", visits );
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
   ch->pagerf( "%s has visited the following areas:\r\n", victim->name );
   ch->pager( "-------------------------------------------\r\n" );
   for( zl = victim->pcdata->zone.begin(  ); zl != victim->pcdata->zone.end(  ); ++zl )
   {
      string zn = *zl;

      ch->pagerf( "%s\r\n", zn.c_str(  ) );
      ++visits;
   }
   ch->pagerf( "&YTotal areas visited: %d\r\n", visits );
}

/* -Thoric
 * Display your current exp, level, and surrounding level exp requirements
 */
CMDF( do_level )
{
   char buf[MSL], buf2[MSL];
   int lowlvl, hilvl;

   if( ch->level == 1 )
      lowlvl = 1;
   else
      lowlvl = UMAX( 2, ch->level - 5 );
   hilvl = URANGE( ch->level, ch->level + 5, MAX_LEVEL );

   ch->printf( "\r\n&[score]Experience required, levels %d to %d:\r\n______________________________________________\r\n\r\n", lowlvl, hilvl );
   snprintf( buf, MSL, " exp  (You have: %11d)", ch->exp );
   snprintf( buf2, MSL, " exp  (To level: %11ld)", exp_level( ch->level + 1 ) - ch->exp );
   for( int x = lowlvl; x <= hilvl; ++x )
      ch->printf( " (%2d) %11ld%s\r\n", x, exp_level( x ), ( x == ch->level ) ? buf : ( x == ch->level + 1 ) ? buf2 : " exp" );
   ch->print( "______________________________________________\r\n" );
}

/* 1997, Blodkai */
CMDF( do_remains )
{
   string buf;
   list < obj_data * >::iterator iobj;
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

   ch->pagerf( "%s appears in a vision, revealing that your remains... ", ch->pcdata->deity->name.c_str(  ) );

   buf = "the corpse of ";
   buf.append( ch->name );
   for( iobj = objlist.begin(  ); iobj != objlist.end(  ); ++iobj )
   {
      obj_data *obj = *iobj;

      if( obj->in_room && !str_cmp( buf, obj->short_descr ) && ( obj->pIndexData->vnum == OBJ_VNUM_CORPSE_PC ) )
      {
         found = true;
         ch->pagerf( "\r\n  - at %s will endure for %d ticks", obj->in_room->name, obj->timer );
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
   list < affect_data * >::iterator af;
   skill_type *skill;

   if( ch->isnpc(  ) )
      return;

   // For a brief "at a glance" output of the affected command
   if( !str_cmp( argument, "by" ) )
   {
      ch->print( "&z-=&w[ &WAffects Quick-View &w]&z=-&D\r\n" );

      ch->print( "\r\n&zImbued with: \r\n" );
      ch->printf( " &C%s\r\n\r\n", ch->has_aflags(  )? bitset_string( ch->get_aflags(  ), aff_flags ) : "nothing" );

      ch->print( "\r\n&zSpells: \r\n" );

      for( af = ch->affects.begin(  ); af != ch->affects.end(  ); ++af )
      {
         affect_data *paf = *af;

         if( ( skill = get_skilltype( paf->type ) ) != nullptr )
         {
            ++affnum;
            ch->printf( "&W%d&z> &BSpell:&w %-18s&D\r\n", affnum, skill->name );
         }
      }

      if( affnum == 0 )
         ch->print( "&wNo cantrip or skill affects you.\r\n\r\n" );
      return;
   }

   // For full output of affected command
   ch->print( "&z-=&w[ &WAffects Summary &w]&z=-&D\r\n" );

   ch->print( "\r\n&zImbued with:\r\n" );
   ch->printf( "&C%s&D\r\n\r\n", ch->has_aflags(  )? bitset_string( ch->get_aflags(  ), aff_flags ) : "nothing" );

   if( ch->level > LEVEL_AVATAR * 0.40 )
   {
      if( ch->has_immunes(  ) )
      {
         ch->print( "&zYou are immune to:\r\n" );
         ch->printf( "&C%s&D\r\n\r\n", bitset_string( ch->get_immunes(  ), ris_flags ) );
      }
      else
         ch->print( "&zYou are immune to: \r\n&CNothing&D\r\n\r\n" );

      if( ch->has_absorbs(  ) )
      {
         ch->print( "&zYou absorb: \r\n" );
         ch->printf( "&C%s&D\r\n\r\n", bitset_string( ch->get_absorbs(  ), ris_flags ) );
      }
      else
         ch->print( "&zYou absorb: \r\n&CNothing&D\r\n\r\n" );
   }

   if( ch->level >= LEVEL_AVATAR * 0.20 )
   {
      if( ch->has_resists(  ) )
      {
         ch->print( "&zYou are resistant to:\r\n" );
         ch->printf( "&C%s&D\r\n\r\n", bitset_string( ch->get_resists(  ), ris_flags ) );
      }
      else
         ch->print( "&zYou are resistant to: \r\n&CNothing&D\r\n\r\n" );

      if( ch->has_susceps(  ) )
      {
         ch->print( "&zYou are susceptible to:\r\n" );
         ch->printf( "&C%s&D\r\n\r\n", bitset_string( ch->get_susceps(  ), ris_flags ) );
      }
      else
         ch->print( "&zYou are susceptible to: \r\n&CNothing&D\r\n\r\n" );
   }

   ch->print( "&z-=&w[ &WSpell Effects &w]&z=-&D\r\n\r\n" );
   for( af = ch->affects.begin(  ); af != ch->affects.end(  ); ++af )
   {
      affect_data *paf = ( *af );

      if( ( skill = get_skilltype( paf->type ) ) != nullptr )
      {
         char mod[MIL];
         ++affnum;

         if( paf->location == APPLY_AFFECT || paf->location == APPLY_EXT_AFFECT )
            mudstrlcpy( mod, aff_flags[paf->modifier], MIL );
         else if( paf->location == APPLY_RESISTANT || paf->location == APPLY_IMMUNE || paf->location == APPLY_ABSORB || paf->location == APPLY_SUSCEPTIBLE )
            mudstrlcpy( mod, ris_flags[paf->modifier], MIL );
         else
            snprintf( mod, MIL, "%d", paf->modifier );

         if( ch->level > LEVEL_AVATAR * 0.40 )
         {
            ch->printf( "&W%d&z> &BSpell:&w %-18s &W- &PModifies&w %-18s &Bby &Y%-18s&B for&R %-4d&B rounds.&D\r\n",
                        affnum, skill->name, a_types[paf->location], mod, paf->duration );
         }
         else if( ch->level > ( LEVEL_AVATAR * 0.20 ) && ch->level <= ( LEVEL_AVATAR * 0.40 ) )
         {
            ch->printf( "&W%d&z> &BSpell:&w %-18s &W- &PModifies&w %-18s&B by &Y%-18s&D\r\n", affnum, skill->name, a_types[paf->location], mod );
         }
         else if( ch->level > ( LEVEL_AVATAR * 0.10 ) && ch->level <= ( LEVEL_AVATAR * 0.20 ) )
         {
            ch->printf( "&W%d&z> &BSpell:&w %-18s &W- &PModifies&w %-18s&D\r\n", affnum, skill->name, a_types[paf->location] );
         }
         else
            ch->printf( "&W%d&z> &BSpell:&w %-18s&D\r\n", affnum, skill->name );
      }
   }
   if( affnum == 0 )
      ch->print( "&wNo cantrip or skill affects you.\r\n" );

   ch->printf( "\r\n&WTotal of &R%d&W affects.&D\r\n", affnum );

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
      ch->printf( "&zSpell Effects, Staves    &W- &G%d\r\n&D", ch->saving_spell_staff );
      ch->printf( "&zParalysis, Petrification &W- &G%d\r\n&D", ch->saving_para_petri );
      ch->printf( "&zWands                    &W- &G%d\r\n&D", ch->saving_wand );
      ch->printf( "&zPoison, Death            &W- &G%d\r\n&D", ch->saving_poison_death );
      ch->printf( "&zBreath Weapons           &W- &G%d\r\n&D", ch->saving_breath );
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
         ch->printf( "There is nobody named %s online.\r\n", argument.c_str(  ) );
         return;
      }
   }

   if( victim != ch )
      ch->printf( "&R%s is carrying:\r\n", victim->isnpc(  )? victim->short_descr : victim->name );
   else
      ch->print( "&RYou are carrying:\r\n" );
   show_list_to_char( ch, victim->carrying, true, true );
   ch->print( "&GItems with a &W*&G after them have other items stored inside.\r\n" );
}

bool is_valid_wear_loc( char_data *ch, int wear_loc )
{
   bitset<MAX_BPART> body_parts;

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
         ch->printf( "There is nobody named %s online.\r\n", argument.c_str(  ) );
         return;
      }
   }

   if( victim != ch )
      ch->printf( "&R%s is using:\r\n", victim->isnpc(  )? victim->short_descr : victim->name );
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

      list < obj_data * >::iterator iobj;
      for( iobj = victim->carrying.begin(  ); iobj != victim->carrying.end(  ); ++iobj )
      {
         obj_data *obj = *iobj;
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

   if( argument.find( '}' ) != string::npos )
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
      bug( "%s: no descriptor", __func__ );
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
         bug( "%s: %s illegal substate %d", __func__, ch->name, ch->substate );
         return;

      case SUB_RESTRICTED:
         ch->print( "You cannot use this command from within another command.\r\n" );
         return;

      case SUB_NONE:
         ch->substate = SUB_PERSONAL_DESC;
         ch->pcdata->dest_buf = ch;
         if( !ch->chardesc || ch->chardesc[0] == '\0' )
            ch->chardesc = STRALLOC( "" );
         ch->editor_desc_printf( "Your description (%s)", ch->name );
         ch->start_editing( ch->chardesc );
         return;

      case SUB_PERSONAL_DESC:
         STRFREE( ch->chardesc );
         ch->chardesc = ch->copy_buffer( true );
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
      bug( "%s: no descriptor", __func__ );
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
         bug( "%s: %s illegal substate %d", __func__, ch->name, ch->substate );
         return;

      case SUB_RESTRICTED:
         ch->print( "You cannot use this command from within another command.\r\n" );
         return;

      case SUB_NONE:
         ch->substate = SUB_PERSONAL_BIO;
         ch->pcdata->dest_buf = ch;
         if( !ch->pcdata->bio || ch->pcdata->bio[0] == '\0' )
            ch->pcdata->bio = str_dup( "" );
         ch->editor_desc_printf( "Your bio (%s).", ch->name );
         ch->start_editing( ch->pcdata->bio );
         return;

      case SUB_PERSONAL_BIO:
         DISPOSE( ch->pcdata->bio );
         ch->pcdata->bio = ch->copy_buffer( false );
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

   ch->printf( "You report: %d/%d hp %d/%d mana %d/%d mv %d xp %ld tnl.\r\n",
               ch->hit, ch->max_hit, ch->mana, ch->max_mana, ch->move, ch->max_move, ch->exp, ( exp_level( ch->level + 1 ) - ch->exp ) );

   act_printf( AT_REPORT, ch, nullptr, nullptr, TO_ROOM, "$n reports: %d/%d hp %d/%d mana %d/%d mv %d xp %ld tnl.",
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
      ch->printf( "&W%s\r\n", ch->pcdata->fprompt );
      ch->print( "&wType 'help prompt' for information on changing your prompt.\r\n" );
      return;
   }

   ch->print( "&wReplacing old prompt of:\r\n" );
   ch->printf( "&W%s\r\n", ch->pcdata->fprompt );
   STRFREE( ch->pcdata->fprompt );

   if( argument.length(  ) > 128 )
      argument = argument.substr( 0, 128 );

   /*
    * Can add a list of pre-set prompts here if wanted.. perhaps
    * 'prompt 1' brings up a different, pre-set prompt 
    */
   if( !str_cmp( argument, "default" ) )
      ch->pcdata->fprompt = STRALLOC( default_fprompt( ch ) );
   else
      ch->pcdata->fprompt = STRALLOC( argument.c_str(  ) );
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
      ch->printf( "&W%s\r\n", ch->pcdata->prompt );
      ch->print( "&wType 'help prompt' for information on changing your prompt.\r\n" );
      return;
   }

   ch->print( "&wReplacing old prompt of:\r\n" );
   ch->printf( "&W%s\r\n", !str_cmp( ch->pcdata->prompt, "" ) ? "(default prompt)" : ch->pcdata->prompt );
   STRFREE( ch->pcdata->prompt );

   if( argument.length(  ) > 128 )
      argument = argument.substr( 0, 128 );

   /*
    * Can add a list of pre-set prompts here if wanted.. perhaps
    * 'prompt 1' brings up a different, pre-set prompt 
    */
   if( !str_cmp( argument, "default" ) )
      ch->pcdata->prompt = STRALLOC( default_prompt( ch ) );
   else
      ch->pcdata->prompt = STRALLOC( argument.c_str(  ) );
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
      ch->printf( "Consider this carefuly %s, if you delete, you will not\r\nbe reinstated as an immortal!\r\n", ch->name );
      ch->print( "Any area data you have will also be lost if you proceed.\r\n" );
   }
   ch->print( "\r\n&RType your password if you wish to delete your character.\r\n" );
   ch->print( "[DELETE] Password: " );
   ch->desc->write_to_buffer( (const char*)echo_off_str );
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
   log_printf( "%s has become a pkiller!", ch->name );
}

const string output_person( char_data * ch, char_data * player )
{
   string rank;
   ostringstream stats, clan_name, outbuf;

   rank = rankbuffer( player );
   outbuf << color_align( rank.c_str(  ), 20, ALIGN_CENTER );

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
      char invis_str[50];

      snprintf( invis_str, 50, " (%d)", player->pcdata->wizinvis );
      stats << invis_str;
   }

   outbuf << stats.str(  );

   if( player->pcdata->clan )
      clan_name << " " << ch->color_str( AT_WHO5 ) << "[" << player->pcdata->clan->name << ch->color_str( AT_WHO5 ) << "]&d";
   else
      clan_name.clear(  );

   outbuf << " " << ch->color_str( AT_WHO4 ) << player->name << player->pcdata->title << clan_name.str(  );

   outbuf << endl;

   return outbuf.str(  );
}

/* Derived directly from the i3who code, which is a hybrid mix of Smaug, RM, and Dale who. */
int afk_who( char_data * ch )
{
   list < descriptor_data * >::iterator ds;
   vector < char_data * >players;
   vector < char_data * >immortals;
   vector < char_data * >::iterator iplr;
   char s1[16], s2[16], s3[16], s4[16], s5[16], s6[16], s7[16];
   int pcount = 0;

   if( !ch )
   {
      bug( "%s: Called with no *ch!", __func__ );
      return 0;
   }

   snprintf( s1, 16, "%s", ch->color_str( AT_WHO ) );
   snprintf( s2, 16, "%s", ch->color_str( AT_WHO2 ) );
   snprintf( s3, 16, "%s", ch->color_str( AT_WHO3 ) );
   snprintf( s4, 16, "%s", ch->color_str( AT_WHO4 ) );
   snprintf( s5, 16, "%s", ch->color_str( AT_WHO5 ) );
   snprintf( s6, 16, "%s", ch->color_str( AT_WHO6 ) );
   snprintf( s7, 16, "%s", ch->color_str( AT_WHO7 ) );

   players.clear(  );
   immortals.clear(  );

   // Construct the two vectors.
   for( ds = dlist.begin(  ); ds != dlist.end(  ); ++ds )
   {
      descriptor_data *d = *ds;
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

      ch->pagerf( "\r\n%s--------------------------------=[ %sPlayers %s]=---------------------------------\r\n\r\n", s7, s6, s7 );

      for( iplr = players.begin(  ); iplr != players.end(  ); ++iplr )
      {
         char_data *player = *iplr;

         ch->pager( output_person( ch, player ) );
      }
   }

   // Display any immortals who were visible to the person calling the command.
   if( !immortals.empty(  ) )
   {
      pcount += immortals.size(  );

      ch->pagerf( "\r\n%s-------------------------------=[ %sImmortals %s]=--------------------------------\r\n\r\n", s1, s6, s1 );

      for( iplr = immortals.begin(  ); iplr != immortals.end(  ); ++iplr )
      {
         char_data *player = *iplr;

         ch->pager( output_person( ch, player ) );
      }
   }
   return pcount;
}

CMDF( do_who )
{
   char buf[MSL], buf2[MSL], outbuf[MSL];
   char s1[16], s2[16], s3[16];
   int amount = 0, xx = 0, pcount = 0;

   snprintf( s1, 16, "%s", ch->color_str( AT_WHO ) );
   snprintf( s2, 16, "%s", ch->color_str( AT_WHO6 ) );
   snprintf( s3, 16, "%s", ch->color_str( AT_WHO2 ) );

   outbuf[0] = '\0';
   snprintf( buf, MSL, "%s-=[ %s%s %s]=-", s1, s2, sysdata->mud_name.c_str(  ), s1 );
   snprintf( buf2, MSL, "&R-=[ &W%s &R]=-", sysdata->mud_name.c_str(  ) );
   amount = 78 - color_strlen( buf2 ); /* Determine amount to put in front of line */

   if( amount < 1 )
      amount = 1;

   amount = amount / 2;

   for( xx = 0; xx < amount; ++xx )
      mudstrlcat( outbuf, " ", MSL );

   mudstrlcat( outbuf, buf, MSL );
   ch->pagerf( "%s\r\n", outbuf );

   pcount = afk_who( ch );

   ch->pagerf( "\r\n\r\n%s[%s%d Player%s%s] ", s3, s2, pcount, pcount == 1 ? "" : "s", s3 );

   ch->pagerf( "%s[%sHomepage: %s%s] [%s%d Max since reboot%s]\r\n", s3, s2, sysdata->http.c_str(  ), s3, s2, sysdata->maxplayers, s3 );

   ch->pagerf( "%s[%s%d login%s since last reboot on %s%s]\r\n", s3, s2, num_logins, num_logins == 1 ? "" : "s", str_boot_time, s3 );
}

/*
 * Place any skill types you don't want them to be able to practice
 * normally in this list.  Separate each with a space.
 * (Uses a hasname check). -- Altrag
 */
#define CANT_PRAC "Tongue"
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

   if( hasname( CANT_PRAC, skill_tname[skill_table[sn]->type] ) )
   {
      act( AT_TELL, "$n tells you 'I do not know how to teach that.'", mob, nullptr, ch, TO_VICT );
      return;
   }

   adept = ( short )( skill_table[sn]->race_adept[race] * 0.3 + ( ch->get_curr_int(  ) * 2 ) );

   if( ch->pcdata->learned[sn] >= adept )
   {
      act_printf( AT_TELL, mob, nullptr, ch, TO_VICT, "$n tells you, 'I've taught you everything I can about %s.'", skill_table[sn]->name );
      act( AT_TELL, "$n tells you, 'You'll have to practice it on your own now...'", mob, nullptr, ch, TO_VICT );
   }
   else
   {
      --ch->pcdata->practice;
      ch->pcdata->learned[sn] += ( 4 + int_app[ch->get_curr_int(  )].learn );
      act( AT_ACTION, "You practice $T.", ch, nullptr, skill_table[sn]->name, TO_CHAR );
      act( AT_ACTION, "$n practices $T.", ch, nullptr, skill_table[sn]->name, TO_ROOM );
      if( ch->pcdata->learned[sn] >= adept )
      {
         ch->pcdata->learned[sn] = adept;
         act( AT_TELL, "$n tells you. 'You'll have to practice it on your own now...'", mob, nullptr, ch, TO_VICT );
      }
   }
}

void display_practice( char_data * ch, char_data * mob, SKILL_INDEX * index, const char *header )
{
   SKILL_INDEX::iterator it, end;
   int cnt, sn, adept;
   skill_type *skill;
   char s1[16], s2[16], s3[16], s4[16];

   snprintf( s1, 16, "%s", ch->color_str( AT_PRAC ) );
   snprintf( s2, 16, "%s", ch->color_str( AT_PRAC2 ) );
   snprintf( s3, 16, "%s", ch->color_str( AT_PRAC3 ) );
   snprintf( s4, 16, "%s", ch->color_str( AT_PRAC4 ) );

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
         ch->pagerf( header, s4, s2, s4 );

      // Increment Count
      ++cnt;

      // Output Skill
      ch->pagerf( "%s%22s%s%s(%s%11s%s)", s2, skill->name,
                  ( ch->level < skill->skill_level[ch->Class] && ch->level < skill->race_level[ch->race] ) ? "&W*" : " ",
                  s1, s3, how_good( ch->pcdata->learned[sn] ).c_str(  ), s1 );

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
   char buf[MSL];
   char_data *mob = nullptr;
   list < char_data * >::iterator ich;
   bool mobfound = false;

   if( ch->isnpc(  ) )
      return;

   for( ich = ch->in_room->people.begin(  ); ich != ch->in_room->people.end(  ); ++ich )
   {
      mob = *ich;
      if( mob->isnpc(  ) && ( mob->has_actflag( ACT_PRACTICE ) || mob->has_actflag( ACT_TEACHER ) ) )
      {
         mobfound = true;
         break;
      }
   }

   if( argument.empty(  ) )
   {
      char s1[16], s4[16];

      snprintf( s1, 16, "%s", ch->color_str( AT_PRAC ) );
      snprintf( s4, 16, "%s", ch->color_str( AT_PRAC4 ) );

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
         display_practice( ch, mob, &skill_table__spell, "%s---------------------------------[ %sSpells  %s]-----------------------------------\r\n" );
      }
      display_practice( ch, mob, &skill_table__skill, "%s---------------------------------[ %sSkills  %s]-----------------------------------\r\n" );
      display_practice( ch, mob, &skill_table__racial, "%s---------------------------------[ %sRacials %s]-----------------------------------\r\n" );
      display_practice( ch, mob, &skill_table__combat, "%s---------------------------------[ %sCombat  %s]-----------------------------------\r\n" );
      display_practice( ch, mob, &skill_table__tongue, "%s---------------------------------[ %sTongues %s]-----------------------------------\r\n" );
      display_practice( ch, mob, &skill_table__lore, "%s---------------------------------[ %sLores   %s]-----------------------------------\r\n" );

      ch->pager( "&W*&D Indicates a skill you have not acheived the required level for yet.\r\n" );
      ch->pagerf( "%sYou have %s%d %spractice sessions left.\r\n", s1, s4, ch->pcdata->practice, s1 );
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
         act_printf( AT_TELL, mob, nullptr, ch, TO_VICT, "$n tells you 'I've never heard of %s. Are you sure you know what you want?'", argument.c_str(  ) );
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

      if( hasname( CANT_PRAC, skill_tname[skill_table[sn]->type] ) )
      {
         act( AT_TELL, "$n tells you 'I do not know how to teach that.'", mob, nullptr, ch, TO_VICT );
         return;
      }

      /*
       * Skill requires a special teacher
       */
      if( skill_table[sn]->teachers && skill_table[sn]->teachers[0] != '\0' )
      {
         snprintf( buf, MSL, "%d", mob->pIndexData->vnum );
         if( !hasname( skill_table[sn]->teachers, buf ) )
         {
            act( AT_TELL, "$n tells you, 'You must find a specialist to learn that!'", mob, nullptr, ch, TO_VICT );
            return;
         }
      }

      /*
       * Guild checks - right now, cant practice guild skills - done on 
       * induct/outcast
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
         act_printf( AT_TELL, mob, nullptr, ch, TO_VICT, "$n tells you, 'I've taught you everything I can about %s.'", skill_table[sn]->name );
         act( AT_TELL, "$n tells you, 'You'll have to practice it on your own now...'", mob, nullptr, ch, TO_VICT );
      }
      else
      {
         --ch->pcdata->practice;
         ch->pcdata->learned[sn] += ( 4 + int_app[ch->get_curr_int(  )].learn );
         act( AT_ACTION, "You practice $T.", ch, nullptr, skill_table[sn]->name, TO_CHAR );
         act( AT_ACTION, "$n practices $T.", ch, nullptr, skill_table[sn]->name, TO_ROOM );
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
      char_data *leader;
      list < char_data * >::iterator ich;

      leader = ch->leader ? ch->leader : ch;
      ch->printf( "%s%s's%s group:\r\n", ch->color_str( AT_SCORE3 ), leader->name, ch->color_str( AT_SCORE ) );

      for( ich = charlist.begin(  ); ich != charlist.end(  ); ++ich )
      {
         char_data *gch = *ich;

         if( is_same_group( gch, ch ) )
         {
            ch->
               printf
               ( "&[score3]%-50s&[score] HP:&[score2]%2.0f&[score]%% %4s:&[score2]%2.0f&[score]%% MV:&[score2]%2.0f&[score]%%\r\n",
                 capitalize( gch->name ), ( ( float )gch->hit / gch->max_hit ) * 100 + 0.5, "MANA",
                 ( ( float )gch->mana / gch->max_mana ) * 100 + 0.5, ( ( float )gch->move / gch->max_move ) * 100 + 0.5 );
         }
      }
      return;
   }

   if( !str_cmp( argument, "disband" ) )
   {
      list < char_data * >::iterator ich;
      int count = 0;

      if( ch->leader || ch->master )
      {
         ch->print( "You cannot disband a group if you're following someone.\r\n" );
         return;
      }

      for( ich = charlist.begin(  ); ich != charlist.end(  ); ++ich )
      {
         char_data *gch = *ich;

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
      list < char_data * >::iterator ich;
      int count = 0;

      for( ich = ch->in_room->people.begin(  ); ich != ch->in_room->people.end(  ); ++ich )
      {
         char_data *rch = *ich;

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
   const char *suf;
   char s1[16], s2[16], s3[16], s4[16], s5[16];
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

   snprintf( s1, 16, "%s", ch->color_str( AT_SCORE ) );
   snprintf( s2, 16, "%s", ch->color_str( AT_SCORE2 ) );
   snprintf( s3, 16, "%s", ch->color_str( AT_SCORE3 ) );
   snprintf( s4, 16, "%s", ch->color_str( AT_SCORE4 ) );
   snprintf( s5, 16, "%s", ch->color_str( AT_SCORE5 ) );

   ch->printf( "%sYou are %s%d %syears old.\r\n", s2, s3, ch->get_age(  ), s2 );

   ch->printf( "%sYou are %s%d%s inches tall, and weigh %s%d%s lbs.\r\n", s2, s3, ch->height, s2, s3, ch->weight, s2 );

   if( time_info.day == ch->pcdata->day && time_info.month == ch->pcdata->month )
      ch->printf( "%sToday is your birthday!\r\n", s2 );
   else
      ch->printf( "%sYour birthday is: %sDay of %s, %d%s day in the Month of %s, in the year %d.\r\n",
                  s2, s1, day_name[ch->pcdata->day % sysdata->daysperweek], day, suf, month_name[ch->pcdata->month], ch->pcdata->year );

   ch->printf( "%sYou have played for %s%ld %shours.\r\n", s2, s3, ch->time_played( ), s1 );

   if( ch->pcdata->deity )
      ch->printf( "%sYou have devoted to %s%s.\r\n", s2, s3, ch->pcdata->deity->name.c_str(  ) );

   ch->printf( "\r\n%sLanguages: ", s2 );

   for( iLang = 0; iLang < LANG_UNKNOWN; ++iLang )
   {
      if( knows_language( ch, iLang, ch ) > 0 )
      {
         if( iLang == ch->speaking )
            ch->print( s3 );
         ch->printf( " %s%s", lang_names[iLang], s1 );
      }
   }
   ch->print( "\r\n\r\n" );

   ch->printf( "%sLogin: %s%s\r\n", s2, s3, c_time( ch->pcdata->logon, ch->pcdata->timezone ) );
   ch->printf( "%sSaved: %s%s\r\n", s2, s3, ch->pcdata->save_time ? c_time( ch->pcdata->save_time, ch->pcdata->timezone ) : "no save this session" );
   ch->printf( "%sTime : %s%s\r", s2, s3, c_time( current_time, ch->pcdata->timezone ) );

   ch->printf( "\r\n%sMKills : %s%d\r\n", s2, s3, ch->pcdata->mkills );
   ch->printf( "%sMDeaths: %s%d\r\n\r\n", s2, s3, ch->pcdata->mdeaths );

   if( ch->pcdata->clan && ch->pcdata->clan->clan_type == CLAN_GUILD )
   {
      ch->printf( "%sGuild         : %s%s\r\n", s2, s3, ch->pcdata->clan->name.c_str(  ) );
      ch->printf( "%sGuild MDeaths : %s%-6d      %sGuild MKills: %s%d\r\n\r\n", s2, s3, ch->pcdata->clan->mdeaths, s2, s3, ch->pcdata->clan->mkills );
   }

   if( ch->CAN_PKILL(  ) )
   {
      ch->printf( "%sPKills        : %s%-6d      %sClan        : %s%s\r\n", s2, s3, ch->pcdata->pkills, s2, s3, ch->pcdata->clan ? ch->pcdata->clan->name.c_str(  ) : "None" );
      ch->printf( "%sIllegal PKills: %s%-6d      %sClan PKills : %s%d\r\n", s2, s3, ch->pcdata->illegal_pk, s2, s3, ( ch->pcdata->clan ? ch->pcdata->clan->pkills[0] : -1 ) );
      ch->printf( "%sPDeaths       : %s%-6d      %sClan PDeaths: %s%d\r\n", s2, s3, ch->pcdata->pdeaths, s2, s3, ( ch->pcdata->clan ? ch->pcdata->clan->pdeaths[0] : -1 ) );
   }
}

CMDF( do_score )
{
   char s1[16], s2[16], s3[16], s4[16], s5[16];

   snprintf( s1, 16, "%s", ch->color_str( AT_SCORE ) );
   snprintf( s2, 16, "%s", ch->color_str( AT_SCORE2 ) );
   snprintf( s3, 16, "%s", ch->color_str( AT_SCORE3 ) );
   snprintf( s4, 16, "%s", ch->color_str( AT_SCORE4 ) );
   snprintf( s5, 16, "%s", ch->color_str( AT_SCORE5 ) );

   ch->printf( "%sScore for %s%s\r\n", s1, ch->name, ch->pcdata->title );
   ch->printf( "%s--------------------------------------------------------------------------------\r\n", s5 );

   ch->printf( "%sLevel: %s%-15d %sHitPoints: %s%5d%s/%s%5d      %sPager    %s(%s%s%s) %s%4d\r\n",
               s2, s3, ch->level, s2, s3, ch->hit, s1, s4, ch->max_hit, s2, s1, s3, ch->has_pcflag( PCFLAG_PAGERON ) ? "X" : " ", s1, s4, ch->pcdata->pagerlen );

   ch->printf( "%sRace : %s%-15.15s %sMana     : %s%5d%s/%s%5d      %sAutoexit %s(%s%s%s)\r\n",
               s2, s3, capitalize( npc_race[ch->race] ), s2, s3, ch->mana, s1, s4, ch->max_mana, s2, s1, s3, ch->has_pcflag( PCFLAG_AUTOEXIT ) ? "X" : " ", s1 );

   ch->printf( "%sClass: %s%-15.15s %sMovement : %s%5d%s/%s%5d      %sAutoloot %s(%s%s%s)\r\n",
               s2, s3, capitalize( npc_class[ch->Class] ), s2, s3, ch->move, s1, s4, ch->max_move, s2, s1, s3, ch->has_pcflag( PCFLAG_AUTOLOOT ) ? "X" : " ", s1 );

   ch->printf( "%sAlign: %s%-15d %sTo Hit   : %s%s%-9d       %sAutosac  %s(%s%s%s)\r\n",
               s2, s3, ch->alignment, s2, s3, ch->GET_HITROLL(  ) > 0 ? "+" : "", ch->GET_HITROLL(  ), s2, s1, s3, ch->has_pcflag( PCFLAG_AUTOSAC ) ? "X" : " ", s1 );

   if( ch->level < 10 )
   {
      ch->printf( "%sSTR  : %s%-15.15s %sTo Dam   : %s%s%-9d       %sSmartsac %s(%s%s%s)\r\n",
                  s2, s3, attribtext( ch->get_curr_str(  ) ).c_str(  ), s2, s3, ch->GET_DAMROLL(  ) > 0 ? "+" : "",
                  ch->GET_DAMROLL(  ), s2, s1, s3, ch->has_pcflag( PCFLAG_SMARTSAC ) ? "X" : " ", s1 );

      ch->printf( "%sINT  : %s%-15.15s %sAC       : %s%s%d\r\n", s2, s3, attribtext( ch->get_curr_int(  ) ).c_str(  ), s2, s3, ch->GET_AC(  ) > 0 ? "+" : "", ch->GET_AC(  ) );

      ch->printf( "%sWIS  : %s%-15.15s %sWimpy    : %s%d\r\n", s2, s3, attribtext( ch->get_curr_wis(  ) ).c_str(  ), s2, s3, ch->wimpy );

      ch->printf( "%sDEX  : %s%-15.15s %sExp      : %s%d\r\n", s2, s3, attribtext( ch->get_curr_dex(  ) ).c_str(  ), s2, s3, ch->exp );

      ch->printf( "%sCON  : %s%-15.15s %sGold     : %s%d\r\n", s2, s3, attribtext( ch->get_curr_con(  ) ).c_str(  ), s2, s3, ch->gold );

      ch->printf( "%sCHA  : %s%-15.15s %sWeight   : %s%d%s/%s%d\r\n",
                  s2, s3, attribtext( ch->get_curr_cha(  ) ).c_str(  ), s2, s3, ch->carry_weight, s1, s4, ch->can_carry_w(  ) );

      ch->printf( "%sLCK  : %s%-15.15s %sItems    : %s%d%s/%s%d\r\n",
                  s2, s3, attribtext( ch->get_curr_lck(  ) ).c_str(  ), s2, s3, ch->carry_number, s1, s4, ch->can_carry_n(  ) );
   }
   else
   {
      ch->printf( "%sSTR  : %s%-2d%s/%s%-12d %sTo Dam   : %s%s%-9d       %sSmartsac %s(%s%s%s)\r\n",
                  s2, s3, ch->get_curr_str(  ), s1, s4, ch->perm_str, s2, s3, ch->GET_DAMROLL(  ) > 0 ? "+" : " ",
                  ch->GET_DAMROLL(  ), s2, s1, s3, ch->has_pcflag( PCFLAG_SMARTSAC ) ? "X" : " ", s1 );

      ch->printf( "%sINT  : %s%-2d%s/%s%-12d %sAC       : %s%s%d\r\n",
                  s2, s3, ch->get_curr_int(  ), s1, s4, ch->perm_int, s2, s3, ch->GET_AC(  ) > 0 ? "+" : "", ch->GET_AC(  ) );

      ch->printf( "%sWIS  : %s%-2d%s/%s%-12d %sWimpy    : %s%d\r\n", s2, s3, ch->get_curr_wis(  ), s1, s4, ch->perm_wis, s2, s3, ch->wimpy );

      ch->printf( "%sDEX  : %s%-2d%s/%s%-12d %sExp      : %s%d\r\n", s2, s3, ch->get_curr_dex(  ), s1, s4, ch->perm_dex, s2, s3, ch->exp );

      ch->printf( "%sCON  : %s%-2d%s/%s%-12d %sGold     : %s%d\r\n", s2, s3, ch->get_curr_con(  ), s1, s4, ch->perm_con, s2, s3, ch->gold );

      ch->printf( "%sCHA  : %s%-2d%s/%s%-12d %sWeight   : %s%d%s/%s%d\r\n",
                  s2, s3, ch->get_curr_cha(  ), s1, s4, ch->perm_cha, s2, s3, ch->carry_weight, s1, s4, ch->can_carry_w(  ) );

      ch->printf( "%sLCK  : %s%-2d%s/%s%-12d %sItems    : %s%d%s/%s%d\r\n",
                  s2, s3, ch->get_curr_lck(  ), s1, s4, ch->perm_lck, s2, s3, ch->carry_number, s1, s4, ch->can_carry_n(  ) );
   }

   ch->printf( "%sPracs: %s%-15d %sFavor    : %s%d\r\n\r\n", s2, s3, ch->pcdata->practice, s2, s3, ch->pcdata->favor );

   ch->printf( "%sYou are %s.\r\n", s2, npc_position[ch->position] );

   if( ch->pcdata->condition[COND_DRUNK] > 10 )
      ch->printf( "%sYou are drunk.\r\n", s2 );

   if( ch->pcdata->condition[COND_THIRST] == 0 )
      ch->printf( "%sYou are extremely thirsty.\r\n", s2 );

   if( ch->pcdata->condition[COND_FULL] == 0 )
      ch->printf( "%sYou are starving.\r\n", s2 );

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
      ch->printf( "\r\n%sTerminal Support Detected: MCCP%s[%s%s%s] %sMSP%s[%s%s%s]\r\n",
                  s2, s1, s3, (ch->desc->can_compress ? "X" : " "), s1, s2, s1, s3, (ch->desc->msp_detected ? "X" : " "), s1 );

      ch->printf( "%sTerminal Support In Use  : MCCP%s[%s%s%s] %sMSP%s[%s%s%s]\r\n",
                  s2, s1, s3, (ch->desc->can_compress ? "X" : " "), s1, s2, s1, s3, (ch->MSP_ON(  )? "X" : " "), s1 );
   }

   if( !ch->pcdata->bestowments.empty(  ) )
      ch->printf( "\r\n%sYou are bestowed with the command(s): %s%s.\r\n", s2, s3, ch->pcdata->bestowments.c_str(  ) );

   if( ch->is_immortal(  ) )
   {
      ch->printf( "\r\n%s--------------------------------------------------------------------------------\r\n", s5 );

      ch->printf( "%sIMMORTAL DATA: Wizinvis %s(%s%s%s) %sLevel %s(%s%d%s) %sRealm: %s(%s%s%s)\r\n",
                  s2, s1, s3, ch->has_pcflag( PCFLAG_WIZINVIS ) ? "X" : " ", s1,
                  s2, s1, s3, ch->pcdata->wizinvis, s1,
                  s2, s1, s3, ch->pcdata->realm_name.c_str(), s1 );

      ch->printf( "%sBamfin:  %s%s\r\n", s2, s1, ( ch->pcdata->bamfin && ch->pcdata->bamfin[0] != '\0' ) ? ch->pcdata->bamfin : "appears in a swirling mist." );
      ch->printf( "%sBamfout: %s%s\r\n", s2, s1, ( ch->pcdata->bamfout && ch->pcdata->bamfout[0] != '\0' ) ? ch->pcdata->bamfout : "leaves in a swirling mist." );

      /*
       * Area Loaded info - Scryn 8/11
       */
      if( ch->pcdata->area )
      {
         ch->printf( "%sVnums: %s%-5.5d - %-5.5d\r\n", s2, s1, ch->pcdata->area->low_vnum, ch->pcdata->area->hi_vnum );
      }
   }

   if( !ch->affects.empty(  ) )
   {
      skill_type *sktmp;
      list < affect_data * >::iterator paf;
      char buf[MSL];

      ch->printf( "\r\n%s--------------------------------------------------------------------------------\r\n", s5 );
      ch->printf( "%sAffect Data:\r\n\r\n", s2 );

      for( paf = ch->affects.begin(  ); paf != ch->affects.end(  ); ++paf )
      {
         affect_data *af = *paf;

         if( !( sktmp = get_skilltype( af->type ) ) )
            continue;

         snprintf( buf, MSL, "%s'%s%s%s'", s1, s3, sktmp->name, s1 );

         ch->printf( "%s%s : %-20s %s(%s%d hours%s)\r\n", s2, skill_tname[sktmp->type], buf, s1, s4, af->duration / ( int )DUR_CONV, s1 );
      }
   }
}

obj_data *find_quill( char_data * ch )
{
   list < obj_data * >::iterator iobj;

   for( iobj = ch->carrying.begin(  ); iobj != ch->carrying.end(  ); ++iobj )
   {
      obj_data *quill = *iobj;

      if( quill->item_type == ITEM_PEN && ch->can_see_obj( quill, false ) )
         return quill;
   }

   return nullptr;
}

/*
 * Journal command. Allows users to write notes to an object of type "journal".
 * Options are Write, Read and Size. Write and Read options require a numerical
 * argument. Option Size retrives v0 or value0 from the object, which is indicitive
 * of how many pages are in the journal.
 *
 * Forced a maximum limit of 50 pages to all journals, just incase someone slipped
 * with a value command and we ended up with an object that could store 500 pages.
 * This is added in journal write and journal size. Leart.
 */
CMDF( do_journal )
{
   string arg1, arg2;
   char buf[MSL];
   extra_descr_data *ed;
   obj_data *quill = nullptr, *journal = nullptr;
   int pages;
   int anum = 0;

   if( ch->isnpc() )
      return;

   if( !ch->desc )
   {
      bug( "%s: no descriptor", __func__ );
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
            bug( "%s: Player not holding journal. (Player: %s)", __func__, ch->name );
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
         bug( "%s: Journal size greater than 50 pages! Resetting to 50 pages. (Player: %s)", __func__, ch->name );
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
      snprintf( buf, MSL, "page %s", arg2.c_str() );

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
            bug( "%s: Journal size greater than 50 pages! Resetting to 50 pages. (Player: %s)", __func__, ch->name );
         }
         ch->set_color( AT_GREY );
         ch->printf( "There are %d pages in this journal.\r\n", journal->value[0] );
         return;
      }
   }

   /* Read option. Players can read the desc on the journal by typing "look page1", but I thought about putting
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
         anum = atoi( arg2.c_str() );
      }

      if( ( journal = ch->get_eq( WEAR_HOLD ) ) == nullptr || journal->item_type != ITEM_JOURNAL )
      {
         ch->print( "You must be holding a journal in order to read it.\r\n" );
         return;
      }

      if( journal->value[0] > 50 )
      {
         journal->value[0] = 50;
         bug( "%s: Journal size greater than 50 pages! Resetting to 50 pages. (Player: %s)", __func__, ch->name );
      }

      ch->set_color( AT_GREY );
      pages = journal->value[0];
      if( pages < anum )
      {
         ch->print( "That page does not exist in this journal.\r\n" );
         return;
      }

      snprintf( buf, MSL, "page %s", arg2.c_str() );

      if( ( ed = get_extra_descr( buf, journal ) ) == nullptr )
         ch->print( "That journal page is blank.\r\n" );
      else
         ch->print( ed->desc.c_str() );
      return;
   }

   do_journal( ch, "" );
}
