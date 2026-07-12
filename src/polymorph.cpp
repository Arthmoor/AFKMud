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
 *                          Shaddai's Polymorph                             *
 ****************************************************************************/

#include <filesystem>
#include <fstream>
#include "mud.h"
#include "deity.h"
#include "objindex.h"
#include "polymorph.h"
#include "raceclass.h"

std::list<morph_data *> morphlist;
int morph_vnum = 0;

/*
 * Local functions
 */
void copy_morph( morph_data *, morph_data * );
void unmorph_all( morph_data * );
CMDF( do_morphstat );

char_morph::char_morph(  )
{
}

char_morph::~char_morph(  )
{
}

morph_data::morph_data(  )
{
}

/*
 * Function that releases all the memory for a morph struct.  Careful not
 * to use the memory afterwards as it doesn't exist.
 * --Shaddai
 */
morph_data::~morph_data(  )
{
   unmorph_all( this );
}

void free_morphs( void )
{
   for( auto it = morphlist.begin(  ); it != morphlist.end(  ); )
   {
      morph_data *poly = *it;
      ++it;

      deleteptr( poly );
   }
   morphlist.clear( );
}

/*
 * Given the Morph's name, returns the pointer to the morph structure.
 * --Shaddai
 */
morph_data *get_morph( std::string_view arg )
{
   if( arg.empty(  ) )
      return nullptr;

   for( auto* morph : morphlist )
   {
      if( !str_cmp( morph->name, arg ) )
         return morph;
   }
   return nullptr;
}

/*
 * Given the Morph's vnum, returns the pointer to the morph structure.
 * --Shaddai
 */
morph_data *get_morph_vnum( int vnum )
{
   if( vnum < 1 )
      return nullptr;

   for( auto* morph : morphlist )
   {
      if( morph->vnum == vnum )
         return morph;
   }
   return nullptr;
}

/*
 * Checks to see if the char meets all the requirements to morph into
 * the provided morph.  Doesn't Look at NPC's Class or race as they
 * are different from pc's, but still checks level and if they can
 * cast it or not.  --Shaddai
 */
bool can_morph( char_data * ch, morph_data * morph, bool is_cast )
{
   if( morph == nullptr )
      return false;
   if( ch->is_immortal(  ) || ch->isnpc(  ) )   /* Let immortals morph to anything Also NPC can do any morph  */
      return true;
   if( morph->no_cast && is_cast )
      return false;
   if( ch->level < morph->level )
      return false;
   if( morph->pkill == ONLY_PKILL && !ch->IS_PKILL(  ) )
      return false;
   if( morph->pkill == ONLY_PEACEFULL && ch->IS_PKILL(  ) )
      return false;
   if( morph->sex != -1 && morph->sex != ch->sex )
      return false;
   if( morph->Class.any(  ) && !morph->Class.test( ch->Class ) )
      return false;
   if( morph->race.any(  ) && !morph->race.test( ch->race ) )
      return false;
   if( !morph->deity.empty() && ( !ch->pcdata->deity || !get_deity( morph->deity ) ) )
      return false;
   if( morph->timeto != -1 && morph->timefrom != -1 )
   {
      int tmp, i;
      bool found = false;

      /*
       * i is a sanity check, just in case things go haywire so it doesn't
       * * loop forever here. -Shaddai
       */
      for( i = 0, tmp = morph->timefrom; i <= sysdata->hoursperday && tmp != morph->timeto; ++i )
      {
         if( tmp == time_info.hour )
         {
            found = true;
            break;
         }
         if( tmp == sysdata->hourmidnight - 1 )
            tmp = 0;
         else
            ++tmp;
      }
      if( !found )
         return false;
   }
   if( morph->dayfrom != -1 && morph->dayto != -1 && ( morph->dayto < ( time_info.day + 1 ) || morph->dayfrom > ( time_info.day + 1 ) ) )
      return false;
   return true;
}

/*
 * Find a morph you can use -- Shaddai
 */
morph_data *find_morph( char_data * ch, std::string_view target, bool is_cast )
{
   if( target.empty(  ) )
      return nullptr;

   for( auto* morph : morphlist )
   {
      if( str_cmp( morph->name, target ) )
         continue;
      if( can_morph( ch, morph, is_cast ) )
         return morph;
   }
   return nullptr;
}

void add_morph_effects( char_data * ch )
{
   ch->get_aflags(  ) |= ch->morph->affected_by;
   ch->get_immunes(  ) |= ch->morph->immune;
   ch->get_resists(  ) |= ch->morph->resistant;
   ch->get_susceps(  ) |= ch->morph->suscept;
   ch->get_absorbs(  ) |= ch->morph->absorb;
   /*
   * Right now only morphs have no_ things --Shaddai
   */
   ch->get_noaflags(  ) |= ch->morph->no_affected_by;
   ch->get_noimmunes(  ) |= ch->morph->no_immune;
   ch->get_noresists(  ) |= ch->morph->no_resistant;
   ch->get_nosusceps(  ) |= ch->morph->no_suscept;
}

const std::string MORPHNAME( char_data * ch )
{
   if( ch->morph && ch->morph->morph && !ch->morph->morph->short_desc.empty() )
      return ch->morph->morph->short_desc;

   return( ch->isnpc() ? ch->short_descr : ch->name );
}

const std::string MORPHPERS( char_data * ch, char_data * looker, bool from )
{
   if( looker->can_see( ch, from ) )
      return ch->morph->morph->short_desc;
   return "Someone";
}

// 0: Initial version.
// 1: Flags written as text.
constexpr int MORPHFILEVER = 1;

/*
 * Write one morph structure to a file. It doesn't print the variable to file
 * if it hasn't been set why waste disk-space :)  --Shaddai
 */
void fwrite_morph( std::ofstream & stream, morph_data * morph )
{
   stream << std::format( "Morph           {}\n", morph->name );
   if( morph->obj[0] != 0 || morph->obj[1] != 0 || morph->obj[2] != 0 )
      stream << std::format( "Objs            {} {} {}\n", morph->obj[0], morph->obj[1], morph->obj[2] );
   if( morph->objuse[0] != 0 || morph->objuse[1] != 0 || morph->objuse[2] != 0 )
      stream << std::format( "Objuse          {} {} {}\n", morph->objuse[0], morph->objuse[1], morph->objuse[2] );
   if( morph->vnum != 0 )
      stream << std::format( "Vnum            {}\n", morph->vnum );
   if( !morph->damroll.empty() )
      stream << std::format( "Damroll         {}~\n", morph->damroll );
   if( morph->defpos != POS_STANDING )
      stream << std::format( "Defpos          {}\n", morph->defpos );
   if( !morph->description.empty() )
      stream << std::format( "Description     {}~\n", morph->description );
   if( !morph->help.empty() )
      stream << std::format( "Help            {}~\n", morph->help );
   if( !morph->hit.empty() )
      stream << std::format( "Hit             {}~\n", morph->hit );
   if( !morph->hitroll.empty() )
      stream << std::format( "Hitroll         {}~\n", morph->hitroll );
   if( !morph->key_words.empty() )
      stream << std::format( "Keywords        {}~\n", morph->key_words );
   if( !morph->long_desc.empty() )
      stream << std::format( "Longdesc        {}~\n", morph->long_desc );
   if( !morph->mana.empty() )
      stream << std::format( "Mana            {}~\n", morph->mana );
   if( !morph->morph_other.empty() )
      stream << std::format( "MorphOther      {}~\n", morph->morph_other );
   if( !morph->morph_self.empty() )
      stream << std::format( "MorphSelf       {}~\n", morph->morph_self );
   if( !morph->move.empty() )
      stream << std::format( "Move            {}~\n", morph->move );
   if( !morph->no_skills.empty() )
      stream << std::format( "NoSkills        {}~\n", morph->no_skills );
   if( !morph->short_desc.empty() )
      stream << std::format( "ShortDesc       {}~\n", morph->short_desc );
   if( !morph->skills.empty() )
      stream << std::format( "Skills          {}~\n", morph->skills );
   if( !morph->unmorph_other.empty() )
      stream << std::format( "UnmorphOther    {}~\n", morph->unmorph_other );
   if( !morph->unmorph_self.empty() )
      stream << std::format( "UnmorphSelf     {}~\n", morph->unmorph_self );
   if( morph->affected_by.any(  ) )
      stream << std::format( "Affected        {}~\n", bitset_string( morph->affected_by, aff_flags ) );
   if( morph->no_affected_by.any(  ) )
      stream << std::format( "NoAffected      {}~\n", bitset_string( morph->no_affected_by, aff_flags ) );
   if( morph->Class.any(  ) )
      stream << std::format( "Class           {}~\n", bitset_string( morph->Class, npc_class ) );
   if( morph->race.any(  ) )
      stream << std::format( "Race            {}~\n", bitset_string( morph->race, npc_race ) );
   if( morph->resistant.any(  ) )
      stream << std::format( "Resistant       {}~\n", bitset_string( morph->resistant, ris_flags ) );
   if( morph->suscept.any(  ) )
      stream << std::format( "Suscept         {}~\n", bitset_string( morph->suscept, ris_flags ) );
   if( morph->immune.any(  ) )
      stream << std::format( "Immune          {}~\n", bitset_string( morph->immune, ris_flags ) );
   if( morph->absorb.any(  ) )
      stream << std::format( "Absorb          {}~\n", bitset_string( morph->absorb, ris_flags ) );
   if( morph->no_immune.any(  ) )
      stream << std::format( "NoImmune        {}~\n", bitset_string( morph->no_immune, ris_flags ) );
   if( morph->no_resistant.any(  ) )
      stream << std::format( "NoResistant     {}~\n", bitset_string( morph->no_resistant, ris_flags ) );
   if( morph->no_suscept.any(  ) )
      stream << std::format( "NoSuscept       {}~\n", bitset_string( morph->no_suscept, ris_flags ) );
   if( morph->timer > 0 )
      stream << std::format( "Timer           {}\n", morph->timer );
   if( morph->used != 0 )
      stream << std::format( "Used            {}\n", morph->used );
   if( morph->sex != -1 )
      stream << std::format( "Sex             {}\n", morph->sex );
   if( morph->pkill != 0 )
      stream << std::format( "Pkill           {}\n", morph->pkill );
   if( morph->timefrom != -1 )
      stream << std::format( "TimeFrom        {}\n", morph->timefrom );
   if( morph->timeto != -1 )
      stream << std::format( "TimeTo          {}\n", morph->timeto );
   if( morph->dayfrom != -1 )
      stream << std::format( "DayFrom         {}\n", morph->dayfrom );
   if( morph->dayto != -1 )
      stream << std::format( "DayTo           {}\n", morph->dayto );
   if( morph->manaused != 0 )
      stream << std::format( "ManaUsed        {}\n", morph->manaused );
   if( morph->moveused != 0 )
      stream << std::format( "MoveUsed        {}\n", morph->moveused );
   if( morph->hpused != 0 )
      stream << std::format( "HpUsed          {}\n", morph->hpused );
   if( morph->favourused != 0 )
      stream << std::format( "FavourUsed      {}\n", morph->favourused );
   if( morph->ac != 0 )
      stream << std::format( "Armor           {}\n", morph->ac );
   if( morph->cha != 0 )
      stream << std::format( "Charisma        {}\n", morph->cha );
   if( morph->con != 0 )
      stream << std::format( "Constitution    {}\n", morph->con );
   if( morph->dex != 0 )
      stream << std::format( "Dexterity       {}\n", morph->dex );
   if( morph->dodge != 0 )
      stream << std::format( "Dodge           {}\n", morph->dodge );
   if( morph->inte != 0 )
      stream << std::format( "Intelligence    {}\n", morph->inte );
   if( morph->lck != 0 )
      stream << std::format( "Luck            {}\n", morph->lck );
   if( morph->level != 0 )
      stream << std::format( "Level           {}\n", morph->level );
   if( morph->parry != 0 )
      stream << std::format( "Parry           {}\n", morph->parry );
   if( morph->saving_breath != 0 )
      stream << std::format( "SaveBreath      {}\n", morph->saving_breath );
   if( morph->saving_para_petri != 0 )
      stream << std::format( "SavePara        {}\n", morph->saving_para_petri );
   if( morph->saving_poison_death != 0 )
      stream << std::format( "SavePoison      {}\n", morph->saving_poison_death );
   if( morph->saving_spell_staff != 0 )
      stream << std::format( "SaveSpell       {}\n", morph->saving_spell_staff );
   if( morph->saving_wand != 0 )
      stream << std::format( "SaveWand        {}\n", morph->saving_wand );
   if( morph->str != 0 )
      stream << std::format( "Strength        {}\n", morph->str );
   if( morph->tumble != 0 )
      stream << std::format( "Tumble          {}\n", morph->tumble );
   if( morph->wis != 0 )
      stream << std::format( "Wisdom          {}\n", morph->wis );
   if( morph->no_cast )
      stream << std::format( "NoCast          {}\n", morph->no_cast );
   if( morph->cast_allowed )
      stream << std::format( "CastAllowed     {}\n", morph->cast_allowed );
   stream << "End\n\n";
}

/*
 * This function saves the morph data.  Should be put in on reboot or shutdown
 * to make use of the sort algorithm. --Shaddai
 */
void save_morphs( void )
{
   std::filesystem::path filename = std::format( "{}{}", SYSTEM_DIR, MORPH_FILE );
   std::ofstream stream( filename );
   if( !stream.is_open(  ) )
   {
      bug( "{}: Cannot open {} for writing: {}", __func__, filename.string(), std::strerror(errno) );
      return;
   }

   stream << std::format( "#VERSION        {}\n", MORPHFILEVER );

   for( auto* morph : morphlist )
      fwrite_morph( stream, morph );

   stream << "#END\n";
   stream.close();
   if( stream.fail() )
      bug( "{}: Error occurred after closing {}: ", __func__, filename.string(), std::strerror(errno) );
}

/*
 * Read in one morph structure
 */
void fread_morph( std::ifstream & stream, int file_ver )
{
   std::string arg, temp;
   int i;

   std::string key = fread_word( stream );
   if( key.empty() || key == "End" )
   {
      bug( "{}: EOF encountered reading morph file!", __func__ );
      return;
   }

   morph_data *morph = new morph_data;
   morph->name = key;

   while( stream >> key )
   {
      switch( to_upper( key[0] ) )
      {
         default:
            bug( "{}: Bad section '{}' in morph file - skipping to next morph.", __func__, key );
            if( morph )
               deleteptr( morph );
         return;

         case 'E':
            if( key == "End" )
            {
               morphlist.push_back( morph );
               return;
            }
            break;

         case 'A':
            if( key == "Absorb" )
            {
               if( file_ver < 1 )
                  stream >> morph->absorb;
               else
               {
                  temp = fread_line( stream );
                  flag_string_set( temp, morph->absorb, ris_flags );
               }
               break;
            }

            if( key == "Armor" )
            {
               stream >> morph->ac;
               break;
            }

            if( key == "Affected" )
            {
               if( file_ver < 1 )
                  stream >> morph->affected_by;
               else
               {
                  temp = fread_line( stream );
                  flag_string_set( temp, morph->affected_by, aff_flags );
               }
               break;
            }
            break;

         case 'C':
            if( key == "Charisma" )
            {
               stream >> morph->cha;
               break;
            }

            if( key == "Class" )
            {
               arg = fread_line( stream );
               while( !arg.empty() )
               {
                  arg = one_argument( arg, temp );
                  for( i = 0; i < MAX_PC_CLASS; ++i )
                     if( !str_cmp( temp, class_table[i]->who_name ) )
                     {
                        morph->Class.set( i );
                        break;
                     }
               }
               break;
            }

            if( key == "Constitution" )
            {
               stream >> morph->con;
               break;
            }

            if( key == "CastAllowed" )
            {
               stream >> morph->cast_allowed;
               break;
            }
            break;

         case 'D':
            if( key == "Damroll" )
            {
               morph->damroll = fread_line( stream );
               break;
            }

            if( key == "DayFrom" )
            {
               stream >> morph->dayfrom;
               break;
            }

            if( key == "DayTo" )
            {
               stream >> morph->dayto;
               break;
            }

            if( key == "Defpos" )
            {
               stream >> morph->defpos;
               break;
            }

            if( key == "Description" )
            {
               morph->description = fread_line( stream );
               break;
            }

            if( key == "Dexterity" )
            {
               stream >> morph->dex;
               break;
            }

            if( key == "Dodge" )
            {
               stream >> morph->dodge;
               break;
            }
            break;

         case 'F':
            if( key == "FavourUsed" )
            {
               stream >> morph->favourused;
            }
            break;

         case 'H':
            if( key == "Help" )
            {
               morph->help = fread_line( stream );
               break;
            }

            if( key == "Hit" )
            {
               morph->hit = fread_line( stream );
               break;
            }

            if( key == "Hitroll" )
            {
               morph->hitroll = fread_line( stream );
               break;
            }

            if( key == "HpUsed" )
            {
               stream >> morph->hpused;
               break;
            }
            break;

         case 'I':
            if( key == "Intelligence" )
            {
               stream >> morph->inte;
               break;
            }

            if( key == "Immune" )
            {
               if( file_ver < 1 )
                  stream >> morph->immune;
               else
               {
                  temp = fread_line( stream );
                  flag_string_set( temp, morph->immune, ris_flags );
               }
               break;
            }
            break;

         case 'K':
            if( key == "Keywords" )
            {
               morph->key_words = fread_line( stream );
               break;
            }
            break;

         case 'L':
            if( key == "Level" )
            {
               stream >> morph->level;
               break;
            }

            if( key == "Longdesc" )
            {
               morph->long_desc = fread_line( stream );
               break;
            }

            if( key == "Luck" )
            {
               stream >> morph->lck;
               break;
            }
            break;

         case 'M':
            if( key == "Mana" )
            {
               morph->mana = fread_line( stream );
               break;
            }

            if( key == "ManaUsed" )
            {
               stream >> morph->manaused;
               break;
            }

            if( key == "MorphOther" )
            {
               morph->morph_other = fread_line( stream );
               break;
            }

            if( key == "MorphSelf" )
            {
               morph->morph_self = fread_line( stream );
               break;
            }

            if( key == "Move" ) /* EEK! This was set wrong! Caught by Matteo 2303 */
            {
               morph->move = fread_line( stream );
               break;
            }

            if( key == "MoveUsed" )
            {
               stream >> morph->moveused;
               break;
            }
            break;

         case 'N':
            if( key == "NoSkills" )
            {
               morph->no_skills = fread_line( stream );
               break;
            }

            if( key == "NoAffected" )
            {
               if( file_ver < 1 )
                  stream >> morph->no_affected_by;
               else
               {
                  temp = fread_line( stream );
                  flag_string_set( temp, morph->no_affected_by, aff_flags );
               }
               break;
            }

            if( key == "NoResistant" )
            {
               if( file_ver < 1 )
                  stream >> morph->no_resistant;
               else
               {
                  temp = fread_line( stream );
                  flag_string_set( temp, morph->no_resistant, ris_flags );
               }
               break;
            }

            if( key == "NoSuscept" )
            {
               if( file_ver < 1 )
                  stream >> morph->no_suscept;
               else
               {
                  temp = fread_line( stream );
                  flag_string_set( temp, morph->no_suscept, ris_flags );
               }
               break;
            }

            if( key == "NoImmune" )
            {
               if( file_ver < 1 )
                  stream >> morph->no_immune;
               else
               {
                  temp = fread_line( stream );
                  flag_string_set( temp, morph->no_immune, ris_flags );
               }
               break;
            }

            if( key == "NoCast" )
            {
               stream >> morph->no_cast;
               break;
            }
            break;

            case 'O':
               if( key == "Objs" )
               {
                  stream >> morph->obj[0] >> morph->obj[1] >> morph->obj[2];
                  break;
               }

               if( key == "Objuse" )
               {
                  stream >> morph->objuse[0] >> morph->objuse[1] >> morph->objuse[2];
                  break;
               }
               break;

            case 'P':
               if( key == "Parry" )
               {
                  stream >> morph->parry;
                  break;
               }

               if( key == "Pkill" )
               {
                  stream >> morph->pkill;
                  break;
               }
               break;

            case 'R':
               if( key == "Race" )
               {
                  arg = fread_line( stream );
                  while( !arg.empty() )
                  {
                     arg = one_argument( arg, temp );
                     for( i = 0; i < MAX_PC_RACE; ++i )
                        if( !str_cmp( temp, race_table[i]->race_name ) )
                        {
                           morph->race.set( i );
                           break;
                        }
                  }
                  break;
               }

               if( key == "Resistant" )
               {
                  if( file_ver < 1 )
                     stream >> morph->resistant;
                  else
                  {
                     temp = fread_line( stream );
                     flag_string_set( temp, morph->resistant, ris_flags );
                  }
                  break;
               }
               break;

            case 'S':
               if( key == "SaveBreath" )
               {
                  stream >> morph->saving_breath;
                  break;
               }

               if( key == "SavePara" )
               {
                  stream >> morph->saving_para_petri;
                  break;
               }

               if( key == "SavePoison" )
               {
                  stream >> morph->saving_poison_death;
                  break;
               }

               if( key == "SaveSpell" )
               {
                  stream >> morph->saving_spell_staff;
                  break;
               }

               if( key == "SaveWand" )
               {
                  stream >> morph->saving_wand;
                  break;
               }

               if( key == "Sex" )
               {
                  stream >> morph->sex;
                  break;
               }

               if( key == "ShortDesc" )
               {
                  morph->short_desc = fread_line( stream );
                  break;
               }

               if( key == "Skills" )
               {
                  morph->skills = fread_line( stream );
                  break;
               }

               if( key == "Strength" )
               {
                  stream >> morph->str;
                  break;
               }

               if( key == "Suscept" )
               {
                  if( file_ver < 1 )
                     stream >> morph->suscept;
                  else
                  {
                     temp = fread_line( stream );
                     flag_string_set( temp, morph->suscept, ris_flags );
                  }
                  break;
               }
               break;

               case 'T':
                  if( key == "Timer" )
                  {
                     stream >> morph->timer;
                     break;
                  }

                  if( key == "TimeFrom" )
                  {
                     stream >> morph->timefrom;
                     break;
                  }

                  if( key == "TimeTo" )
                  {
                     stream >> morph->timeto;
                     break;
                  }

                  if( key == "Tumble" )
                  {
                     stream >> morph->tumble;
                     break;
                  }
                  break;

               case 'U':
                  if( key == "UnmorphOther" )
                  {
                     morph->unmorph_other = fread_line( stream );
                     break;
                  }

                  if( key == "UnmorphSelf" )
                  {
                     morph->unmorph_self = fread_line( stream );
                     break;
                  }

                  if( key == "Used" )
                  {
                     stream >> morph->used;
                     break;
                  }
                  break;

               case 'V':
                  if( key == "Vnum" )
                  {
                     stream >> morph->vnum;
                     break;
                  }
                  break;

               case 'W':
                  if( key == "Wisdom" )
                  {
                     stream >> morph->wis;
                     break;
                  }
                  break;
      }
   }
}

void setup_morph_vnum( void )
{
   int vnum = morph_vnum;

   for( auto* morph : morphlist )
   {
      if( morph->vnum > vnum )
         vnum = morph->vnum;
   }
   if( vnum < 1000 )
      vnum = 1000;
   else
      ++vnum;

   for( auto* imorph : morphlist )
   {
      if( imorph->vnum == 0 )
      {
         imorph->vnum = vnum++;
      }
   }
   morph_vnum = vnum;
}

/*
 *  This function loads in the morph data.  Note that this function MUST be
 *  used AFTER the race and Class tables have been read in and setup.
 *  --Shaddai
 */
void load_morphs( void )
{
   std::filesystem::path filename = std::format( "{}{}", SYSTEM_DIR, MORPH_FILE );
   std::ifstream stream( filename );
   if( !stream.is_open(  ) )
   {
      bug( "{}: Cannot open {} for reading: {}", __func__, filename.string(), std::strerror(errno) );
      std::exit( EXIT_FAILURE );
   }

   int file_ver = 0;
   std::string key;
   while( stream >> key )
   {
      if( key == "#VERSION" )
         stream >> file_ver;
      else if( key == "#END" )
         break;
      else if( key == "Morph" )
         fread_morph( stream, file_ver );
      else
      {
         bug( "{}: Bad section '{}' in {} - skipping.", __func__, key, filename.string() );
         fread_to_eol( stream );
      }
   }
   stream.close();
   setup_morph_vnum(  );
   log_string( "Done: Morphs." );
}

/*
 *  Command used to set all the morphing information.
 *  Must use the morphset save command, to write the commands to file.
 *  Hp/Mana/Move/Hitroll/Damroll can be set using variables such
 *  as 1d2+10.  No boundary checks are in place yet on those, so care must
 *  be taken when using these.  --Shaddai
 */
CMDF( do_morphset )
{
   std::string arg1, arg2, arg3, buf;
   std::string origarg = argument;
   int value;
   morph_data *morph = nullptr;

   if( ch->isnpc(  ) )
   {
      ch->print( "Mob's can't morphset\r\n" );
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
      case SUB_MORPH_DESC:
         if( !ch->pcdata->dest_buf )
         {
            ch->print( "Fatal error: report to Samson.\r\n" );
            bug( "{}: sub_morph_desc: nullptr ch->pcdata->dest_buf", __func__ );
            ch->substate = SUB_NONE;
            return;
         }
         morph = ( morph_data * ) ch->pcdata->dest_buf;
         morph->description = ch->copy_buffer( );
         ch->stop_editing(  );
         ch->substate = ch->tempnum;
         if( ch->substate == SUB_REPEATCMD )
            ch->pcdata->dest_buf = morph;
         return;

      case SUB_MORPH_HELP:
         if( !ch->pcdata->dest_buf )
         {
            ch->print( "Fatal error: report to Samson.\r\n" );
            bug( "{}: sub_morph_help: nullptr ch->pcdata->dest_buf", __func__ );
            ch->substate = SUB_NONE;
            return;
         }
         morph = ( morph_data * ) ch->pcdata->dest_buf;
         morph->help = ch->copy_buffer( );
         ch->stop_editing(  );
         ch->substate = ch->tempnum;
         if( ch->substate == SUB_REPEATCMD )
            ch->pcdata->dest_buf = morph;
         return;

      case SUB_EDIT_ABORT:
         ch->substate = SUB_NONE;
         ch->print( "Aborting description.\r\n" );
         return;
   }
   morph = nullptr;
   smash_tilde( argument );

   if( ch->substate == SUB_REPEATCMD )
   {
      morph = ( morph_data * ) ch->pcdata->dest_buf;

      if( !morph )
      {
         ch->print( "Someone deleted your morph!\r\n" );
         argument = "done";
      }

      if( argument.empty(  ) )
      {
         do_morphstat( ch, morph->name );
         return;
      }

      if( !str_cmp( argument, "stat" ) )
      {
         funcf( ch, do_morphstat, "{} help", morph->name );
         return;
      }

      if( !str_cmp( argument, "done" ) || !str_cmp( argument, "off" ) )
      {
         ch->print( "Morphset mode off.\r\n" );
         ch->substate = SUB_NONE;
         ch->pcdata->dest_buf = nullptr;
         ch->pcdata->subprompt.clear();
         return;
      }
   }

   if( morph )
   {
      arg1 = morph->name;
      argument = one_argument( argument, arg2 );
      arg3 = argument;
   }
   else
   {
      argument = one_argument( argument, arg1 );
      argument = one_argument( argument, arg2 );
      arg3 = argument;
   }

   if( !str_cmp( arg1, "on" ) )
   {
      ch->print( "Syntax: morphset <morph> on.\r\n" );
      return;
   }
   value = is_number( arg3 ) ? std::stoi( arg3 ) : -1;

   if( std::stoi( arg3 ) < -1 && value == -1 )
      value = std::stoi( arg3 );

   if( ch->substate != SUB_REPEATCMD && !arg1.empty(  ) && !str_cmp( arg1, "save" ) )
   {
      save_morphs(  );
      ch->print( "Morph data saved.\r\n" );
      return;
   }

   if( arg1.empty(  ) || ( arg2.empty(  ) && ch->substate != SUB_REPEATCMD ) || !str_cmp( arg1, "?" ) )
   {
      if( ch->substate == SUB_REPEATCMD )
      {
         if( morph )
            ch->print( "Syntax: <field>  <value>\r\n" );
         else
            ch->print( "Syntax: <morph> <field>  <value>\r\n" );
      }
      else
         ch->print( "Syntax: morphset <morph> <field>  <value>\r\n" );
      ch->print( "Syntax: morphset save\r\n\r\n" );
      ch->print( "&cField being one of:\r\n" );
      ch->print( "&c-------------------------------------------------\r\n" );
      ch->print( "  ac, affected, cha, Class, con, damroll, dayto,\r\n" );
      ch->print( "  dayfrom, deity, description, defpos, dex, dodge,\r\n" );
      ch->print( "  favourused, help, hitroll, hp, hpused, immune,\r\n" );
      ch->print( "  int, str, keyword, lck, level, long, mana, manaused,\r\n" );
      ch->print( "  morphother, morphself, move, moveused, name, noaffected,\r\n" );
      ch->print( "  nocast, castallow, noimmune, noresistant, noskill, nosusceptible,\r\n" );
      ch->print( "  obj1, obj2, obj3, objuse1, objuse2, objuse3, parry,\r\n" );
      ch->print( "  pkill, race, resistant, sav1, sav2, sav3, sav4, sav5,\r\n" );
      ch->print( "  sex, short, skills, susceptible, timefrom, timer, timeto,\r\n" );
      ch->print( "  tumble, unmorphother, unmorphself, wis.\r\n" );
      ch->print( "&c-------------------------------------------------\r\n" );
      return;
   }

   if( !morph )
   {
      if( !is_number( arg1 ) )
         morph = get_morph( arg1 );
      else
         morph = get_morph_vnum( std::stoi( arg1 ) );

      if( morph == nullptr )
      {
         ch->print( "That morph does not exist.\r\n" );
         return;
      }
   }

   if( !str_cmp( arg2, "on" ) )
   {
      ch->CHECK_SUBRESTRICTED(  );
      ch->print_fmt( "Morphset mode on. (Editing {}).\r\n", morph->name );
      ch->substate = SUB_REPEATCMD;
      ch->pcdata->dest_buf = morph;
      ch->pcdata->subprompt = std::format( "<&CMorphset &W{}&w> %i", morph->name );
      return;
   }

   if( !str_cmp( arg2, "str" ) )
   {
      if( value < -10 || value > 10 )
      {
         ch->print( "Strength must be a value from -10 to 10.\r\n" );
         return;
      }
      morph->str = value;
   }
   else if( !str_cmp( arg2, "int" ) )
   {
      if( value < -10 || value > 10 )
      {
         ch->print( "Intelligence must be a value from -10 to 10.\r\n" );
         return;
      }
      morph->inte = value;
   }
   else if( !str_cmp( arg2, "wis" ) )
   {
      if( value < -10 || value > 10 )
      {
         ch->print( "Wisdom must be a value from -10 to 10.\r\n" );
         return;
      }
      morph->wis = value;
   }
   else if( !str_cmp( arg2, "defpos" ) )
   {
      if( value < 0 || value > POS_STANDING )
      {
         ch->print_fmt( "Position range is 0 to {}.\r\n", (int)POS_STANDING );
         return;
      }
      morph->defpos = value;
   }
   else if( !str_cmp( arg2, "dex" ) )
   {
      if( value < -10 || value > 10 )
      {
         ch->print( "Dexterity must be a value from -10 to 10.\r\n" );
         return;
      }
      morph->dex = value;
   }
   else if( !str_cmp( arg2, "con" ) )
   {
      if( value < -10 || value > 10 )
      {
         ch->print( "Constitution must be a value from -10 to 10.\r\n" );
         return;
      }
      morph->con = value;
   }
   else if( !str_cmp( arg2, "cha" ) )
   {
      if( value < -10 || value > 10 )
      {
         ch->print( "Charisma must be a value from -10 to 10.\r\n" );
         return;
      }
      morph->cha = value;
   }
   else if( !str_cmp( arg2, "lck" ) )
   {
      if( value < -10 || value > 10 )
      {
         ch->print( "Luck must be a value from -10 to 10.\r\n" );
         return;
      }
      morph->lck = value;
   }
   else if( !str_cmp( arg2, "sex" ) )
   {
      value = get_npc_sex( arg3 );

      if( value < 0 || value >= SEX_MAX )
      {
         ch->print( "Invalid sex.\r\n" );
         return;
      }
      morph->sex = value;
   }
   else if( !str_cmp( arg2, "pkill" ) )
   {
      if( !str_cmp( arg3, "pkill" ) )
         morph->pkill = ONLY_PKILL;
      else if( !str_cmp( arg3, "peace" ) )
         morph->pkill = ONLY_PEACEFULL;
      else if( !str_cmp( arg3, "none" ) )
         morph->pkill = 0;
      else
      {
         ch->print( "Usuage: morphset <morph> pkill [pkill|peace|none]\r\n" );
         return;
      }
   }
   else if( !str_cmp( arg2, "manaused" ) )
   {
      if( value < 0 || value > 2000 )
      {
         ch->print( "Mana used is a value from 0 to 2000.\r\n" );
         return;
      }
      morph->manaused = value;
   }
   else if( !str_cmp( arg2, "moveused" ) )
   {
      if( value < 0 || value > 2000 )
      {
         ch->print( "Move used is a value from 0 to 2000.\r\n" );
         return;
      }
      morph->moveused = value;
   }
   else if( !str_cmp( arg2, "hpused" ) )
   {
      if( value < 0 || value > 2000 )
      {
         ch->print( "Hp used is a value from 0 to 2000.\r\n" );
         return;
      }
      morph->hpused = value;
   }
   else if( !str_cmp( arg2, "favourused" ) )
   {
      if( value < 0 || value > 2000 )
      {
         ch->print( "Favour used is a value from 0 to 2000.\r\n" );
         return;
      }
      morph->favourused = value;
   }
   else if( !str_cmp( arg2, "timeto" ) )
   {
      if( value < 0 || value > sysdata->hoursperday - 1 )
      {
         ch->print_fmt( "Timeto is a value from 0 to {}.\r\n", sysdata->hoursperday - 1 );
         return;
      }
      morph->timeto = value;
   }
   else if( !str_cmp( arg2, "timefrom" ) )
   {
      if( value < 0 || value > sysdata->hoursperday - 1 )
      {
         ch->print_fmt( "Timefrom is a value from 0 to {}.\r\n", sysdata->hoursperday - 1 );
         return;
      }
      morph->timefrom = value;
   }
   else if( !str_cmp( arg2, "dayto" ) )
   {
      if( value < 0 || value > sysdata->dayspermonth - 1 )
      {
         ch->print_fmt( "Dayto is a value from 0 to {}.\r\n", sysdata->dayspermonth - 1 );
         return;
      }
      morph->dayto = value;
   }
   else if( !str_cmp( arg2, "dayfrom" ) )
   {
      if( value < 0 || value > sysdata->dayspermonth - 1 )
      {
         ch->print_fmt( "Dayfrom is a value from 0 to {}.\r\n", sysdata->dayspermonth - 1 );
         return;
      }
      morph->dayfrom = value;
   }
   else if( !str_cmp( arg2, "sav1" ) || !str_cmp( arg2, "savepoison" ) )
   {
      if( value < -30 || value > 30 )
      {
         ch->print( "Saving throw range is -30 to 30.\r\n" );
         return;
      }
      morph->saving_poison_death = value;
   }
   else if( !str_cmp( arg2, "sav2" ) || !str_cmp( arg2, "savewand" ) )
   {
      if( value < -30 || value > 30 )
      {
         ch->print( "Saving throw range is -30 to 30.\r\n" );
         return;
      }
      morph->saving_wand = value;
   }
   else if( !str_cmp( arg2, "sav3" ) || !str_cmp( arg2, "savepara" ) )
   {
      if( value < -30 || value > 30 )
      {
         ch->print( "Saving throw range is -30 to 30.\r\n" );
         return;
      }
      morph->saving_para_petri = value;
   }
   else if( !str_cmp( arg2, "sav4" ) || !str_cmp( arg2, "savebreath" ) )
   {
      if( value < -30 || value > 30 )
      {
         ch->print( "Saving throw range is -30 to 30.\r\n" );
         return;
      }
      morph->saving_breath = value;
   }
   else if( !str_cmp( arg2, "sav5" ) || !str_cmp( arg2, "savestaff" ) )
   {
      if( value < -30 || value > 30 )
      {
         ch->print( "Saving throw range is -30 to 30.\r\n" );
         return;
      }
      morph->saving_spell_staff = value;
   }
   else if( !str_cmp( arg2, "timer" ) )
   {
      if( value < -1 || value == 0 )
      {
         ch->print( "Timer must be -1 (None) or greater than 0.\r\n" );
         return;
      }
      morph->timer = value;
   }
   else if( !str_cmp( arg2, "hp" ) )
   {
      argument = one_argument( argument, arg3 );
      if( str_cmp( arg3, "0" ) )
         morph->hit = arg3;
   }
   else if( !str_cmp( arg2, "mana" ) )
   {
      argument = one_argument( argument, arg3 );
      if( str_cmp( arg3, "0" ) )
         morph->mana = arg3;
   }
   else if( !str_cmp( arg2, "move" ) )
   {
      argument = one_argument( argument, arg3 );
      if( str_cmp( arg3, "0" ) )
         morph->move = arg3;
   }
   else if( !str_cmp( arg2, "ac" ) )
   {
      if( value > 500 || value < -500 )
      {
         ch->print( "Ac range is -500 to 500.\r\n" );
         return;
      }
      morph->ac = value;
   }
   else if( !str_cmp( arg2, "hitroll" ) )
   {
      argument = one_argument( argument, arg3 );
      if( str_cmp( arg3, "0" ) )
         morph->hitroll = arg3;
   }
   else if( !str_cmp( arg2, "damroll" ) )
   {
      argument = one_argument( argument, arg3 );
      if( str_cmp( arg3, "0" ) )
         morph->damroll = arg3;
   }
   else if( !str_cmp( arg2, "dodge" ) )
   {
      if( value > 100 || value < -100 )
      {
         ch->print( "Dodge range is -100 to 100.\r\n" );
         return;
      }
      morph->dodge = value;
   }
   else if( !str_prefix( "obj", arg2 ) )
   {
      int oindex;
      std::string temp;

      if( arg2.length(  ) <= 3 )
      {
         ch->print( "Obj 1, 2, or 3.\r\n" );
         return;
      }
      temp = arg2.substr( 3, arg3.length(  ) );
      oindex = std::stoi( temp );
      if( oindex > 3 || oindex < 1 )
      {
         ch->print( "Obj 1, 2, or 3.\r\n" );
         return;
      }
      if( !( get_obj_index( value ) ) )
      {
         ch->print( "No such vnum.\r\n" );
         return;
      }
      morph->obj[oindex - 1] = value;
   }
   else if( !str_cmp( arg2, "parry" ) )
   {
      if( value > 100 || value < -100 )
      {
         ch->print( "Parry range is -100 to 100.\r\n" );
         return;
      }
      morph->parry = value;
   }
   else if( !str_cmp( arg2, "tumble" ) )
   {
      if( value > 100 || value < -100 )
      {
         ch->print( "Tumble range is -100 to 100.\r\n" );
         return;
      }
      morph->tumble = value;
   }
   else if( !str_cmp( arg2, "level" ) )
   {
      if( value < 0 || value > LEVEL_AVATAR )
      {
         ch->print_fmt( "Level range is 0 to {}.\r\n", LEVEL_AVATAR );
         return;
      }
      morph->level = value;
   }
   else if( !str_prefix( arg2, "objuse" ) )
   {
      int oindex;
      std::string temp;

      if( arg2.length(  ) <= 6 )
      {
         ch->print( "Objuse 1, 2 or 3?\r\n" );
         return;
      }
      temp = arg2.substr( 6, arg2.length(  ) );
      oindex = std::stoi( temp );
      if( oindex > 3 || oindex < 1 )
      {
         ch->print( "Objuse 1, 2, or 3?\r\n" );
         return;
      }
      if( value )
         morph->objuse[oindex - 1] = true;
      else
         morph->objuse[oindex - 1] = false;
   }
   else if( !str_cmp( arg2, "nocast" ) )
   {
      if( value )
         morph->no_cast = true;
      else
         morph->no_cast = false;
   }
   else if( !str_cmp( arg2, "castallow" ) )
   {
      if( !str_cmp( arg3, "true" ) )
         morph->cast_allowed = true;
      else
         morph->cast_allowed = false;
   }
   else if( !str_cmp( arg2, "resistant" ) || !str_cmp( arg2, "r" ) )
   {
      if( argument.empty(  ) )
      {
         ch->print( "Usage: morphset <morph> resistant <flag> [flag]...\r\n" );
         return;
      }

      while( !argument.empty(  ) )
      {
         argument = one_argument( argument, arg3 );
         value = get_risflag( arg3 );
         if( value < 0 || value >= MAX_RIS_FLAG )
            ch->print_fmt( "Unknown flag: {}\r\n", arg3 );
         else
            morph->resistant.flip( value );
      }
   }
   else if( !str_cmp( arg2, "susceptible" ) || !str_cmp( arg2, "s" ) )
   {
      if( argument.empty(  ) )
      {
         ch->print( "Usage: morphset <morph> susceptible <flag> [flag]...\r\n" );
         return;
      }

      while( !argument.empty(  ) )
      {
         argument = one_argument( argument, arg3 );
         value = get_risflag( arg3 );
         if( value < 0 || value >= MAX_RIS_FLAG )
            ch->print_fmt( "Unknown flag: {}\r\n", arg3 );
         else
            morph->suscept.flip( value );
      }
   }
   else if( !str_cmp( arg2, "immune" ) || !str_cmp( arg2, "i" ) )
   {
      if( argument.empty(  ) )
      {
         ch->print( "Usage: morphset <morph> immune <flag> [flag]...\r\n" );
         return;
      }

      while( !argument.empty(  ) )
      {
         argument = one_argument( argument, arg3 );
         value = get_risflag( arg3 );
         if( value < 0 || value >= MAX_RIS_FLAG )
            ch->print_fmt( "Unknown flag: {}\r\n", arg3 );
         else
            morph->immune.flip( value );
      }
   }
   else if( !str_cmp( arg2, "absorb" ) || !str_cmp( arg2, "a" ) )
   {
      if( argument.empty(  ) )
      {
         ch->print( "Usage: morphset <morph> absorb <flag> [flag]...\r\n" );
         return;
      }

      while( !argument.empty(  ) )
      {
         argument = one_argument( argument, arg3 );
         value = get_risflag( arg3 );
         if( value < 0 || value >= MAX_RIS_FLAG )
            ch->print_fmt( "Unknown flag: {}\r\n", arg3 );
         else
            morph->absorb.flip( value );
      }
   }
   else if( !str_cmp( arg2, "noresistant" ) || !str_cmp( arg2, "nr" ) )
   {
      if( argument.empty(  ) )
      {
         ch->print( "Usage: morphset <morph> noresistant <flag> [flag]...\r\n" );
         return;
      }

      while( !argument.empty(  ) )
      {
         argument = one_argument( argument, arg3 );
         value = get_risflag( arg3 );
         if( value < 0 || value >= MAX_RIS_FLAG )
            ch->print_fmt( "Unknown flag: {}\r\n", arg3 );
         else
            morph->no_resistant.flip( value );
      }
   }
   else if( !str_cmp( arg2, "nosusceptible" ) || !str_cmp( arg2, "ns" ) )
   {
      if( argument.empty(  ) )
      {
         ch->print( "Usage: morphset <morph> nosusceptible <flag> [flag]...\r\n" );
         return;
      }

      while( !argument.empty(  ) )
      {
         argument = one_argument( argument, arg3 );
         value = get_risflag( arg3 );
         if( value < 0 || value >= MAX_RIS_FLAG )
            ch->print_fmt( "Unknown flag: {}\r\n", arg3 );
         else
            morph->no_suscept.flip( value );
      }
   }
   else if( !str_cmp( arg2, "noimmune" ) || !str_cmp( arg2, "ni" ) )
   {
      if( argument.empty(  ) )
      {
         ch->print( "Usage: morphset <morph> noimmune <flag> [flag]...\r\n" );
         return;
      }

      while( !argument.empty(  ) )
      {
         argument = one_argument( argument, arg3 );
         value = get_risflag( arg3 );
         if( value < 0 || value >= MAX_RIS_FLAG )
            ch->print_fmt( "Unknown flag: {}\r\n", arg3 );
         else
            morph->no_immune.flip( value );
      }
   }
   else if( !str_cmp( arg2, "affected" ) || !str_cmp( arg2, "aff" ) )
   {
      if( argument.empty(  ) )
      {
         ch->print( "Usage: morphset <morph> affected <flag> [flag]...\r\n" );
         return;
      }

      while( !argument.empty(  ) )
      {
         argument = one_argument( argument, arg3 );
         value = get_aflag( arg3 );
         if( value < 0 || value >= MAX_AFFECTED_BY )
            ch->print_fmt( "Unknown flag: {}\r\n", arg3 );
         else
            morph->affected_by.flip( value );
      }
   }
   else if( !str_cmp( arg2, "noaffected" ) || !str_cmp( arg2, "naff" ) )
   {
      if( argument.empty(  ) )
      {
         ch->print( "Usage: morphset <morph> noaffected <flag> [flag]...\r\n" );
         return;
      }

      while( !argument.empty(  ) )
      {
         argument = one_argument( argument, arg3 );
         value = get_aflag( arg3 );
         if( value < 0 || value >= MAX_AFFECTED_BY )
            ch->print_fmt( "Unknown flag: {}\r\n", arg3 );
         else
            morph->no_affected_by.flip( value );
      }
   }
   else if( !str_cmp( arg2, "short" ) )
   {
      morph->short_desc = arg3;
   }
   else if( !str_cmp( arg2, "morphother" ) )
   {
      morph->morph_other = arg3;
   }
   else if( !str_cmp( arg2, "morphself" ) )
   {
      morph->morph_self = arg3;
   }
   else if( !str_cmp( arg2, "unmorphother" ) )
   {
      morph->unmorph_other = arg3;
   }
   else if( !str_cmp( arg2, "unmorphself" ) )
   {
      morph->unmorph_self = arg3;
   }
   else if( !str_cmp( arg2, "keyword" ) )
   {
      morph->key_words = arg3;
   }
   else if( !str_cmp( arg2, "long" ) )
      morph->long_desc = std::format( "{}\r\n", arg3 );
   else if( !str_cmp( arg2, "description" ) || !str_cmp( arg2, "desc" ) )
   {
      if( !arg3.empty(  ) )
      {
         morph->description = arg3;
      }
      ch->CHECK_SUBRESTRICTED(  );
      if( ch->substate == SUB_REPEATCMD )
         ch->tempnum = SUB_REPEATCMD;
      else
         ch->tempnum = SUB_NONE;
      ch->substate = SUB_MORPH_DESC;
      ch->pcdata->dest_buf = morph;
      ch->set_editor_desc( "A morph description." );
      ch->start_editing( morph->description );
      return;
   }
   else if( !str_cmp( arg2, "name" ) )
   {
      morph->name = arg3;
   }
   else if( !str_cmp( arg2, "help" ) )
   {
      if( !arg3.empty(  ) )
      {
         morph->help = arg3;
      }
      ch->CHECK_SUBRESTRICTED(  );
      if( ch->substate == SUB_REPEATCMD )
         ch->tempnum = SUB_REPEATCMD;
      else
         ch->tempnum = SUB_NONE;
      ch->substate = SUB_MORPH_HELP;
      ch->pcdata->dest_buf = morph;
      ch->set_editor_desc( "A morph help file." );
      ch->start_editing( morph->help );
      return;
   }
   else if( !str_cmp( arg2, "skills" ) )
   {
      if( arg3.empty(  ) || !str_cmp( arg3, "none" ) )
         return;

      if( morph->skills.empty() )
         buf = arg3;
      else
         buf = std::format( "{} {}", morph->skills, arg3 );
      morph->skills = buf;
   }
   else if( !str_cmp( arg2, "noskills" ) )
   {
      if( arg3.empty(  ) || !str_cmp( arg3, "none" ) )
         return;

      if( morph->no_skills.empty() )
         buf = arg3;
      else
         buf = std::format( "{} {}", morph->no_skills, arg3 );
      morph->no_skills = buf;
   }
   else if( !str_cmp( arg2, "Class" ) )
   {
      value = get_pc_class( arg3 );

      if( value < 0 || value >= MAX_PC_CLASS )
      {
         ch->print_fmt( "Unknown PC Class: {}", arg3 );
         return;
      }
      morph->Class.flip( value );
   }
   else if( !str_cmp( arg2, "race" ) )
   {
      value = get_pc_race( arg3 );

      if( value < 0 || value >= MAX_PC_RACE )
      {
         ch->print_fmt( "Unknown PC race: {}", arg3 );
         return;
      }
      morph->race.flip( value );
   }
   else if( ch->substate == SUB_REPEATCMD )
   {
      ch->substate = SUB_RESTRICTED;
      interpret( ch, origarg );
      ch->substate = SUB_REPEATCMD;
      ch->last_cmd = do_morphset;
   }
   else
   {
      do_morphset( ch, "" );
      return;
   }
   ch->print( "Done.\r\n" );
}

/*
 *  Shows morph info on a given morph.
 *  To see the description and help file, must use morphstat <morph> help
 *  Shaddai
 */
CMDF( do_morphstat )
{
   morph_data *morph;
   std::string arg;
   int count = 1;

   ch->set_pager_color( AT_CYAN );

   argument = one_argument( argument, arg );
   if( arg.empty(  ) )
   {
      ch->print( "Morphstat what?\r\n" );
      return;
   }

   if( ch->isnpc(  ) )
   {
      ch->print( "Mob's can't morphstat\r\n" );
      return;
   }

   if( !str_cmp( arg, "list" ) )
   {
      if( morphlist.empty(  ) )
      {
         ch->print( "No morph's currently exist.\r\n" );
         return;
      }

      for( auto* poly : morphlist )
      {
         ch->pager_fmt( "&c[&C{}&c]   Name:  &C{:<13}    &cVnum:  &C{:4}  &cUsed:  &C{:3}\r\n", count, poly->name, poly->vnum, poly->used );
         ++count;
      }
      return;
   }
   if( !is_number( arg ) )
      morph = get_morph( arg );
   else
      morph = get_morph_vnum( std::stoi( arg ) );

   if( morph == nullptr )
   {
      ch->print( "No such morph exists.\r\n" );
      return;
   }

   if( argument.empty(  ) )
   {
      ch->pager_fmt( "  &cMorph Name: &C{:<20}  Vnum: {:4}\r\n", morph->name, morph->vnum );
      ch->pager( "&B[----------------------------------------------------------------------------]\r\n" );
      ch->pager( "                           &BMorph Restrictions\r\n" );
      ch->pager( "&B[----------------------------------------------------------------------------]\r\n" );
      ch->pager_fmt( "  &cClasses Allowed   : &w{}\r\n", bitset_string( morph->Class, npc_class ) );
      ch->pager_fmt( "  &cRaces Not Allowed: &w{}\r\n", bitset_string( morph->race, npc_race ) );
      ch->pager_fmt( "  &cSex:  &C{}   &cPkill:   &C{}   &cTime From:   &C{}   &cTime To:    &C{}\r\n",
                  morph->sex == SEX_MALE ? "male" :
                  morph->sex == SEX_FEMALE ? "female" : "neutral",
                  morph->pkill == ONLY_PKILL ? "YES" : morph->pkill == ONLY_PEACEFULL ? "NO" : "n/a", morph->timefrom, morph->timeto );
      ch->pager_fmt( "  &cDay From:  &C{}  &cDay To:  &C{}\r\n", morph->dayfrom, morph->dayto );
      ch->pager_fmt( "  &cLevel:  &C{}       &cMorph Via Spell   : &C{}\r\n", morph->level, ( morph->no_cast ) ? "NO" : "yes" );
      ch->pager_fmt( " &cCasting Allowed   : &C{}\r\n", ( morph->cast_allowed ? "Yes" : "No" ) );
      ch->pager_fmt( "  &cUSAGES:  Mana:  &C{}  &cMove:  &C{}  &cHp:  &C{}  &cFavour:  &C{}\r\n", morph->manaused, morph->moveused, morph->hpused, morph->favourused );
      ch->pager_fmt( "  &cObj1: &C{}  &cObjuse1: &C{}   &cObj2: &C{}  &cObjuse2: &C{}   &cObj3: &C{}  &cObjuse3: &c{}\r\n",
                  morph->obj[0], ( morph->objuse[0] ? "YES" : "no" ), morph->obj[1], ( morph->objuse[1] ? "YES" : "no" ), morph->obj[2], ( morph->objuse[2] ? "YES" : "no" ) );
      ch->pager_fmt( "  &cTimer: &w{}\r\n", morph->timer );
      ch->pager( "&B[----------------------------------------------------------------------------]\r\n" );
      ch->pager( "                       &BEnhancements to the Player\r\n" );
      ch->pager( "&B[----------------------------------------------------------------------------]\r\n" );
      ch->
         pager_fmt
         ( "  &cStr: &C{:2}&c )( Int: &C{:2}&c )( Wis: &C{:2}&c )( Dex: &C{:2}&c )( Con: &C{:2}&c )( Cha: &C{:2}&c )( Lck: &C{:2}&c )\r\n",
           morph->str, morph->inte, morph->wis, morph->dex, morph->con, morph->cha, morph->lck );
      ch->pager_fmt( "  &cSave versus: &w{} {} {} {} {}       &cDodge: &w{}  &cParry: &w{}  &cTumble: &w{}\r\n",
                  morph->saving_poison_death, morph->saving_wand, morph->saving_para_petri, morph->saving_breath,
                  morph->saving_spell_staff, morph->dodge, morph->parry, morph->tumble );
      ch->pager_fmt( "  &cHps     : &w{}    &cMana   : &w{}    &cMove      : &w{}\r\n", morph->hit, morph->mana, morph->move );
      ch->pager_fmt( "  &cDamroll : &w{}    &cHitroll: &w{}    &cAC     : &w{}\r\n", morph->damroll, morph->hitroll, morph->ac );
      ch->pager( "&B[----------------------------------------------------------------------------]\r\n" );
      ch->pager( "                          &BAffects to the Player\r\n" );
      ch->pager( "&B[----------------------------------------------------------------------------]\r\n" );
      ch->pager_fmt( "  &cAffected by: &C{}\r\n", bitset_string( morph->affected_by, aff_flags ) );
      ch->pager_fmt( "  &cImmune     : &w{}\r\n", bitset_string( morph->immune, ris_flags ) );
      ch->pager_fmt( "  &cSusceptible: &w{}\r\n", bitset_string( morph->suscept, ris_flags ) );
      ch->pager_fmt( "  &cResistant  : &w{}\r\n", bitset_string( morph->resistant, ris_flags ) );
      ch->pager_fmt( "  &cAbsorb     : &w{}\r\n", bitset_string( morph->absorb, ris_flags ) );
      ch->pager_fmt( "  &cSkills     : &w{}\r\n", morph->skills );
      ch->pager( "&B[----------------------------------------------------------------------------]\r\n" );
      ch->pager( "                     &BPrevented affects to the Player\r\n" );
      ch->pager( "&B[----------------------------------------------------------------------------]\r\n" );
      ch->pager_fmt( "  &cNot affected by: &C{}\r\n", bitset_string( morph->no_affected_by, aff_flags ) );
      ch->pager_fmt( "  &cNot Immune     : &w{}\r\n", bitset_string( morph->no_immune, ris_flags ) );
      ch->pager_fmt( "  &cNot Susceptible: &w{}\r\n", bitset_string( morph->no_suscept, ris_flags ) );
      ch->pager_fmt( "  &cNot Resistant  : &w{}\r\n", bitset_string( morph->no_resistant, ris_flags ) );
      ch->pager_fmt( "  &cNot Skills     : &w{}\r\n", morph->no_skills );
      ch->pager( "&B[----------------------------------------------------------------------------]\r\n" );
      ch->pager( "\r\n" );
   }
   else if( !str_cmp( argument, "help" ) || !str_cmp( argument, "desc" ) )
   {
      ch->pager_fmt( "  &cMorph Name  : &C{:<20}\r\n", morph->name );
      ch->pager_fmt( "  &cDefault Pos : &w{}\r\n", morph->defpos );
      ch->pager_fmt( "  &cKeywords    : &w{}\r\n", morph->key_words );
      ch->pager_fmt( "  &cShortdesc   : &w{}\r\n", !morph->short_desc.empty() ? "(none set)" : morph->short_desc );
      ch->pager_fmt( "  &cLongdesc    : &w{}", morph->long_desc.empty() ? "(none set)\r\n" : morph->long_desc );
      ch->pager_fmt( "  &cMorphself   : &w{}\r\n", morph->morph_self );
      ch->pager_fmt( "  &cMorphother  : &w{}\r\n", morph->morph_other );
      ch->pager_fmt( "  &cUnMorphself : &w{}\r\n", morph->unmorph_self );
      ch->pager_fmt( "  &cUnMorphother: &w{}\r\n", morph->unmorph_other );
      ch->pager( "&B[----------------------------------------------------------------------------]\r\n" );
      ch->pager_fmt( "                                  &cHelp:\r\n&w{}\r\n", morph->help );
      ch->pager( "&B[----------------------------------------------------------------------------]\r\n" );
      ch->pager_fmt( "                               &cDescription:\r\n&w{}\r\n", morph->description );
      ch->pager( "&B[----------------------------------------------------------------------------]\r\n" );
   }
   else
   {
      ch->print( "Syntax: morphstat <morph>\r\n" );
      ch->print( "Syntax: morphstat <morph> <help/desc>\r\n" );
   }
}

/*
 * This function sends the morph/unmorph message to the people in the room.
 * -- Shaddai
 */
void send_morph_message( char_data * ch, morph_data * morph, bool is_morph )
{
   if( morph == nullptr )
      return;
   if( is_morph )
   {
      act( AT_MORPH, morph->morph_other, ch, nullptr, nullptr, TO_ROOM );
      act( AT_MORPH, morph->morph_self, ch, nullptr, nullptr, TO_CHAR );
   }
   else
   {
      act( AT_MORPH, morph->unmorph_other, ch, nullptr, nullptr, TO_ROOM );
      act( AT_MORPH, morph->unmorph_self, ch, nullptr, nullptr, TO_CHAR );
   }
}

/*
 * Create new player morph, a scaled down version of original morph
 * so if morph gets changed stats don't get messed up.
 */
char_morph *make_char_morph( morph_data * morph )
{
   if( morph == nullptr )
      return nullptr;

   char_morph *ch_morph = new char_morph;

   ch_morph->morph = morph;
   ch_morph->ac = morph->ac;
   ch_morph->str = morph->str;
   ch_morph->inte = morph->inte;
   ch_morph->wis = morph->wis;
   ch_morph->dex = morph->dex;
   ch_morph->cha = morph->cha;
   ch_morph->lck = morph->lck;
   ch_morph->saving_breath = morph->saving_breath;
   ch_morph->saving_para_petri = morph->saving_para_petri;
   ch_morph->saving_poison_death = morph->saving_poison_death;
   ch_morph->saving_spell_staff = morph->saving_spell_staff;
   ch_morph->saving_wand = morph->saving_wand;
   ch_morph->timer = morph->timer;
   ch_morph->affected_by = morph->affected_by;
   ch_morph->immune = morph->immune;
   ch_morph->absorb = morph->absorb;
   ch_morph->resistant = morph->resistant;
   ch_morph->suscept = morph->suscept;
   ch_morph->no_affected_by = morph->no_affected_by;
   ch_morph->no_immune = morph->no_immune;
   ch_morph->no_resistant = morph->no_resistant;
   ch_morph->no_suscept = morph->no_suscept;
   ch_morph->cast_allowed = morph->cast_allowed;

   return ch_morph;
}

/*
 * Workhorse of the polymorph code, this turns the player into the proper
 * morph.  Doesn't check if you can morph or not so must use can_morph before
 * this is called.  That is so we can let people morph into things without
 * checking. Also, if anything is changed here, make sure it gets changed
 * in do_unmorph otherwise you will be giving your player stats for free.
 * This also does not send the message to people when you morph good to
 * use in save functions.
 * --Shaddai
 */
void do_morph( char_data * ch, morph_data * morph )
{
   char_morph *ch_morph;

   if( !morph )
      return;

   if( ch->morph )
      do_unmorph_char( ch );

   ch_morph = make_char_morph( morph );
   ch->armor += morph->ac;
   ch->mod_str += morph->str;
   ch->mod_int += morph->inte;
   ch->mod_wis += morph->wis;
   ch->mod_dex += morph->dex;
   ch->mod_cha += morph->cha;
   ch->mod_lck += morph->lck;
   ch->mod_con += morph->con; /* Was missing. Caught by Matteo2303 */
   ch->saving_breath += morph->saving_breath;
   ch->saving_para_petri += morph->saving_para_petri;
   ch->saving_poison_death += morph->saving_poison_death;
   ch->saving_spell_staff += morph->saving_spell_staff;
   ch->saving_wand += morph->saving_wand;

   ch_morph->hitroll = !morph->hitroll.empty() ? dice_parse( ch, morph->level, morph->hitroll ) : 0;
   ch->hitroll += ch_morph->hitroll;

   ch_morph->damroll = !morph->damroll.empty() ? dice_parse( ch, morph->level, morph->damroll ) : 0;
   ch->damroll += ch_morph->damroll;

   ch_morph->hit = !morph->hit.empty() ? dice_parse( ch, morph->level, morph->hit ) : 0;
   if( ( ch->hit + ch_morph->hit ) > 32700 )
      ch_morph->hit = ( 32700 - ch->hit );
   ch->hit += ch_morph->hit;

   ch_morph->move = !morph->move.empty() ? dice_parse( ch, morph->level, morph->move ) : 0;
   if( ( ch->move + ch_morph->move ) > 32700 )
      ch_morph->move = ( 32700 - ch->move );
   ch->move += ch_morph->move;

   ch_morph->mana = !morph->mana.empty() ? dice_parse( ch, morph->level, morph->mana ) : 0;
   if( ( ch->mana + ch_morph->mana ) > 32700 )
      ch_morph->mana = ( 32700 - ch->mana );
   ch->mana += ch_morph->mana;

   ch->get_aflags(  ) |= morph->affected_by;
   ch->get_immunes(  ) |= morph->immune;
   ch->get_resists(  ) |= morph->resistant;
   ch->get_susceps(  ) |= morph->suscept;
   ch->get_absorbs(  ) |= morph->absorb;
   ch->get_aflags(  ) &= ~( morph->no_affected_by );
   ch->get_immunes(  ) &= ~( morph->no_immune );
   ch->get_resists(  ) &= ~( morph->no_resistant );
   ch->get_susceps(  ) &= ~( morph->no_suscept );
   ch->morph = ch_morph;
   ++morph->used;
}

/*
 * These functions wrap the sending of morph stuff, with morphing the code.
 * In most cases this is what should be called in the code when using spells,
 * obj morphing, etc... --Shaddai
 */
int do_morph_char( char_data * ch, morph_data * morph )
{
   bool canmorph = true;
   obj_data *obj, *tmpobj;

   if( ch->morph )
      canmorph = false;

   if( morph->obj[0] )
   {
      if( !( obj = ch->get_obj_vnum( morph->obj[0] ) ) )
         canmorph = false;
      else if( morph->objuse[0] )
      {
         act( AT_OBJECT, "$p disappears in a whisp of smoke!", obj->carried_by, obj, nullptr, TO_CHAR );
         if( obj == obj->carried_by->get_eq( WEAR_WIELD ) && ( tmpobj = obj->carried_by->get_eq( WEAR_DUAL_WIELD ) ) != nullptr )
            tmpobj->wear_loc = WEAR_WIELD;
         obj->separate(  );
         obj->extract(  );
      }
   }

   if( morph->obj[1] )
   {
      if( !( obj = ch->get_obj_vnum( morph->obj[1] ) ) )
         canmorph = false;
      else if( morph->objuse[1] )
      {
         act( AT_OBJECT, "$p disappears in a whisp of smoke!", obj->carried_by, obj, nullptr, TO_CHAR );
         if( obj == obj->carried_by->get_eq( WEAR_WIELD ) && ( tmpobj = obj->carried_by->get_eq( WEAR_DUAL_WIELD ) ) != nullptr )
            tmpobj->wear_loc = WEAR_WIELD;
         obj->separate(  );
         obj->extract(  );
      }
   }

   if( morph->obj[2] )
   {
      if( !( obj = ch->get_obj_vnum( morph->obj[2] ) ) )
         canmorph = false;
      else if( morph->objuse[2] )
      {
         act( AT_OBJECT, "$p disappears in a whisp of smoke!", obj->carried_by, obj, nullptr, TO_CHAR );
         if( obj == obj->carried_by->get_eq( WEAR_WIELD ) && ( tmpobj = obj->carried_by->get_eq( WEAR_DUAL_WIELD ) ) != nullptr )
            tmpobj->wear_loc = WEAR_WIELD;
         obj->separate(  );
         obj->extract(  );
      }
   }

   if( morph->hpused )
   {
      if( ch->hit < morph->hpused )
         canmorph = false;
      else
         ch->hit -= morph->hpused;
   }

   if( morph->moveused )
   {
      if( ch->move < morph->moveused )
         canmorph = false;
      else
         ch->move -= morph->moveused;
   }

   if( morph->manaused )
   {
      if( ch->mana < morph->manaused )
         canmorph = false;
      else
         ch->mana -= morph->manaused;
   }

   if( morph->favourused )
   {
      if( ch->isnpc(  ) || !ch->pcdata->deity || ch->pcdata->favor < morph->favourused )
         canmorph = false;
      else
      {
         ch->pcdata->favor -= morph->favourused;
         ch->adjust_favor( -1, 1 );
      }
   }

   if( !canmorph )
   {
      ch->print( "You begin to transform, but something goes wrong.\r\n" );
      return false;
   }
   send_morph_message( ch, morph, true );
   do_morph( ch, morph );
   return true;
}

/*
 * This makes sure to take all the affects given to the player by the morph
 * away.  Several things to keep in mind.  If you add something here make
 * sure you add it to do_morph as well (Unless you want to charge your players
 * for morphing ;) ).  Also make sure that their pfile saves with the morph
 * affects off, as the morph may not exist when they log back in.  This
 * function also does not send the message to people when you morph so it is
 * good to use in save functions and other places you don't want people to
 * see the stuff.
 * --Shaddai
 */
void do_unmorph( char_data * ch )
{
   char_morph *morph;

   if( !( morph = ch->morph ) )
      return;

   ch->armor -= morph->ac;
   ch->mod_str -= morph->str;
   ch->mod_int -= morph->inte;
   ch->mod_wis -= morph->wis;
   ch->mod_dex -= morph->dex;
   ch->mod_cha -= morph->cha;
   ch->mod_lck -= morph->lck;
   ch->mod_con -= morph->con; /* Was missing. Caught by Matteo2303 */
   ch->saving_breath -= morph->saving_breath;
   ch->saving_para_petri -= morph->saving_para_petri;
   ch->saving_poison_death -= morph->saving_poison_death;
   ch->saving_spell_staff -= morph->saving_spell_staff;
   ch->saving_wand -= morph->saving_wand;
   ch->hitroll -= morph->hitroll;
   ch->damroll -= morph->damroll;
   ch->hit -= morph->hit;
   ch->move -= morph->move;
   ch->mana -= morph->mana;
   /*
    * Added by Tarl 21 Mar 02 to fix polymorph massive mana bug 
    */
   if( ch->mana > ch->max_mana )
      ch->mana = ch->max_mana;
   ch->get_aflags(  ) &= ~( morph->affected_by );
   ch->get_immunes(  ) &= ~( morph->immune );
   ch->get_resists(  ) &= ~( morph->resistant );
   ch->get_susceps(  ) &= ~( morph->suscept );
   ch->get_absorbs(  ) &= ~( morph->absorb );
   deleteptr( ch->morph );
   ch->update_aris(  );
}

void do_unmorph_char( char_data * ch )
{
   morph_data *temp;

   if( !ch->morph )
      return;

   temp = ch->morph->morph;
   do_unmorph( ch );
   send_morph_message( ch, temp, false );
}

/* Morph revert command ( God only knows why the Smaugers left this out ) - Samson 6-14-99 */
CMDF( do_revert )
{
   if( !ch->morph )
   {
      ch->print( "But you aren't polymorphed?!?\r\n" );
      return;
   }
   do_unmorph_char( ch );
}

/*
 * This function copies one morph structure to another
 */
void copy_morph( morph_data * morph, morph_data * temp )
{
   morph->damroll = temp->damroll;
   morph->description = temp->description;
   morph->help = temp->help;
   morph->hit = temp->hit;
   morph->hitroll = temp->hitroll;
   morph->key_words = temp->key_words;
   morph->long_desc = temp->long_desc;
   morph->mana = temp->mana;
   morph->morph_other = temp->morph_other;
   morph->morph_self = temp->morph_self;
   morph->move = temp->move;
   morph->name = temp->name;
   morph->short_desc = temp->short_desc;
   morph->skills = temp->skills;
   morph->no_skills = temp->no_skills;
   morph->unmorph_other = temp->unmorph_other;
   morph->unmorph_self = temp->unmorph_self;
   morph->affected_by = temp->affected_by;
   morph->Class = temp->Class;
   morph->sex = temp->sex;
   morph->timefrom = temp->timefrom;
   morph->timeto = temp->timeto;
   morph->timefrom = temp->timefrom;
   morph->dayfrom = temp->dayfrom;
   morph->dayto = temp->dayto;
   morph->pkill = temp->pkill;
   morph->manaused = temp->manaused;
   morph->moveused = temp->moveused;
   morph->hpused = temp->hpused;
   morph->favourused = temp->favourused;
   morph->immune = temp->immune;
   morph->absorb = temp->absorb;
   morph->no_affected_by = temp->no_affected_by;
   morph->no_immune = temp->no_immune;
   morph->no_resistant = temp->no_resistant;
   morph->no_suscept = temp->no_suscept;
   morph->obj[0] = temp->obj[0];
   morph->obj[1] = temp->obj[1];
   morph->obj[2] = temp->obj[2];
   morph->objuse[0] = temp->objuse[0];
   morph->objuse[1] = temp->objuse[1];
   morph->objuse[2] = temp->objuse[2];
   morph->race = temp->race;
   morph->resistant = temp->resistant;
   morph->suscept = temp->suscept;
   morph->ac = temp->ac;
   morph->defpos = temp->defpos;
   morph->dex = temp->dex;
   morph->dodge = temp->dodge;
   morph->cha = temp->cha;
   morph->con = temp->con;
   morph->inte = temp->inte;
   morph->lck = temp->lck;
   morph->level = temp->level;
   morph->parry = temp->parry;
   morph->saving_breath = temp->saving_breath;
   morph->saving_para_petri = temp->saving_para_petri;
   morph->saving_poison_death = temp->saving_poison_death;
   morph->saving_spell_staff = temp->saving_spell_staff;
   morph->saving_wand = temp->saving_wand;
   morph->str = temp->str;
   morph->tumble = temp->tumble;
   morph->wis = temp->wis;
   morph->no_cast = temp->no_cast;
   morph->cast_allowed = temp->cast_allowed;
   morph->timer = temp->timer;
}

/*
 * Player command to create a new morph
 */
CMDF( do_morphcreate )
{
   morph_data *morph, *temp = nullptr;
   std::string arg1;

   argument = one_argument( argument, arg1 );

   if( arg1.empty(  ) )
   {
      ch->print( "Usage: morphcreate <name>\r\n" );
      ch->print( "Usage: morphcreate <name/vnum> copy\r\n" );
      return;
   }

   if( !argument.empty(  ) && !str_cmp( argument, "copy" ) )
   {
      if( is_number( arg1 ) )
      {
         if( !( temp = get_morph_vnum( std::stoi( arg1 ) ) ) )
         {
            ch->print_fmt( "No such morph vnum {} exists.\r\n", std::stoi( arg1 ) );
            return;
         }
      }
      else if( !( temp = get_morph( arg1 ) ) )
      {
         ch->print_fmt( "No such morph {} exists.\r\n", arg1 );
         return;
      }
   }
   smash_tilde( arg1 );
   morph = new morph_data;

   if( !argument.empty(  ) && !str_cmp( argument, "copy" ) && temp )
      copy_morph( morph, temp );
   else
      morph->name = arg1;
   if( morph->short_desc.empty() )
      morph->short_desc = arg1;
   morph->vnum = morph_vnum;
   ++morph_vnum;
   morphlist.push_back( morph );
   ch->print_fmt( "Morph {} created with vnum {}.\r\n", morph->name, morph->vnum );
}

void unmorph_all( morph_data * morph )
{
   for( auto* vch : pclist )
   {
      if( vch->morph == nullptr || vch->morph->morph == nullptr || vch->morph->morph != morph )
         continue;
      do_unmorph_char( vch );
   }
}

/*
 * Player function to delete a morph. --Shaddai
 * FIXME: Need to check all players and force them to unmorph first
 */
CMDF( do_morphdestroy )
{
   morph_data *morph;

   if( argument.empty(  ) )
   {
      ch->print( "Destroy which morph?\r\n" );
      return;
   }
   if( is_number( argument ) )
      morph = get_morph_vnum( std::stoi( argument ) );
   else
      morph = get_morph( argument );

   if( !morph )
   {
      ch->print_fmt( "Unknown morph {}.\r\n", argument );
      return;
   }
   deleteptr( morph );
   ch->print( "Morph deleted.\r\n" );
}

// This data is written to the player's pfile when they save while morphed.
void fwrite_morph_data( char_data * ch, std::ofstream & stream )
{
   if( ch->morph == nullptr )
      return;

   char_morph *morph = ch->morph;

   stream << "#MorphData\n";
   /*
    * Only Print Out what is necessary 
    */
   if( morph->morph != nullptr )
   {
      stream << std::format( "Vnum {}\n", morph->morph->vnum );
      stream << std::format( "Name {}~\n", morph->morph->name );
   }
   if( morph->affected_by.any(  ) )
      stream << std::format( "Affect          {}~\n", bitset_string( morph->affected_by, aff_flags ) );
   if( morph->no_affected_by.any(  ) )
      stream << std::format( "NoAffect        {}~\n", bitset_string( morph->no_affected_by, aff_flags ) );
   if( morph->immune.any(  ) )
      stream << std::format( "Immune          {}~\n", bitset_string( morph->immune, ris_flags ) );
   if( morph->resistant.any(  ) )
      stream << std::format( "Resistant       {}~\n", bitset_string( morph->resistant, ris_flags ) );
   if( morph->suscept.any(  ) )
      stream << std::format( "Suscept         {}~\n", bitset_string( morph->suscept, ris_flags ) );
   if( morph->absorb.any(  ) )
      stream << std::format( "Absorb          {}~\n", bitset_string( morph->absorb, ris_flags ) );
   if( morph->no_immune.any(  ) )
      stream << std::format( "NoImmune        {}~\n", bitset_string( morph->no_immune, ris_flags ) );
   if( morph->no_resistant.any(  ) )
      stream << std::format( "NoResistant     {}~\n", bitset_string( morph->no_resistant, ris_flags ) );
   if( morph->no_suscept.any(  ) )
      stream << std::format( "NoSuscept       {}~\n", bitset_string( morph->no_suscept, ris_flags ) );
   if( morph->ac != 0 )
      stream << std::format( "Armor           {}\n", morph->ac );
   if( morph->cha != 0 )
      stream << std::format( "Charisma        {}\n", morph->cha );
   if( morph->con != 0 )
      stream << std::format( "Constitution    {}\n", morph->con );
   if( morph->damroll != 0 )
      stream << std::format( "Damroll         {}\n", morph->damroll );
   if( morph->dex != 0 )
      stream << std::format( "Dexterity       {}\n", morph->dex );
   if( morph->dodge != 0 )
      stream << std::format( "Dodge           {}\n", morph->dodge );
   if( morph->hit != 0 )
      stream << std::format( "Hit             {}\n", morph->hit );
   if( morph->hitroll != 0 )
      stream << std::format( "Hitroll         {}\n", morph->hitroll );
   if( morph->inte != 0 )
      stream << std::format( "Intelligence    {}\n", morph->inte );
   if( morph->lck != 0 )
      stream << std::format( "Luck            {}\n", morph->lck );
   if( morph->mana != 0 )
      stream << std::format( "Mana            {}\n", morph->mana );
   if( morph->move != 0 )
      stream << std::format( "Move            {}\n", morph->move );
   if( morph->parry != 0 )
      stream << std::format( "Parry           {}\n", morph->parry );
   if( morph->saving_breath != 0 )
      stream << std::format( "Save1           {}\n", morph->saving_breath );
   if( morph->saving_para_petri != 0 )
      stream << std::format( "Save2           {}\n", morph->saving_para_petri );
   if( morph->saving_poison_death != 0 )
      stream << std::format( "Save3           {}\n", morph->saving_poison_death );
   if( morph->saving_spell_staff != 0 )
      stream << std::format( "Save4           {}\n", morph->saving_spell_staff );
   if( morph->saving_wand != 0 )
      stream << std::format( "Save5           {}\n", morph->saving_wand );
   if( morph->str != 0 )
      stream << std::format( "Strength        {}\n", morph->str );
   if( morph->timer != -1 )
      stream << std::format( "Timer           {}\n", morph->timer );
   if( morph->tumble != 0 )
      stream << std::format( "Tumble          {}\n", morph->tumble );
   if( morph->wis != 0 )
      stream << std::format( "Wisdom          {}\n", morph->wis );
   stream << "End\n";
}

void clear_char_morph( char_morph * morph )
{
   morph->affected_by.reset(  );
   morph->no_affected_by.reset(  );
   morph->absorb.reset(  );
   morph->immune.reset(  );
   morph->no_immune.reset(  );
   morph->no_resistant.reset(  );
   morph->no_suscept.reset(  );
   morph->resistant.reset(  );
   morph->suscept.reset(  );
   morph->timer = -1;
   morph->ac = 0;
   morph->cha = 0;
   morph->con = 0;
   morph->damroll = 0;
   morph->dex = 0;
   morph->dodge = 0;
   morph->hit = 0;
   morph->hitroll = 0;
   morph->inte = 0;
   morph->lck = 0;
   morph->mana = 0;
   morph->parry = 0;
   morph->saving_breath = 0;
   morph->saving_para_petri = 0;
   morph->saving_poison_death = 0;
   morph->saving_spell_staff = 0;
   morph->saving_wand = 0;
   morph->str = 0;
   morph->tumble = 0;
   morph->wis = 0;
   morph->morph = nullptr;
}

// This is read from a player's pfile if they were morphed at the time they saved.
void fread_morph_data( char_data * ch, std::ifstream & stream )
{
   char_morph *morph = new char_morph;
   clear_char_morph( morph );
   ch->morph = morph;

   std::string key;
   std::string temp;
   while( stream >> key )
   {
      if( key == "Vnum" )
      {
         int vnum;
         stream >> vnum;

         morph->morph = get_morph_vnum( vnum );
      }
      else if( key == "Name" )
      {
         if( morph->morph )
            if( str_cmp( morph->morph->name, fread_line( stream ) ) )
               bug( "{}: Morph Name doesn't match vnum {}.", __func__, morph->morph->vnum );
      }
      else if( key == "Affect" )
      {
         temp = fread_line( stream );

         flag_string_set( temp, morph->affected_by, aff_flags );
      }
      else if( key == "NoAffect" )
      {
         temp = fread_line( stream );

         flag_string_set( temp, morph->no_affected_by, aff_flags );
      }
      else if( key == "Immune" )
      {
         temp = fread_line( stream );

         flag_string_set( temp, morph->immune, ris_flags );
      }
      else if( key == "Resistant" )
      {
         temp = fread_line( stream );

         flag_string_set( temp, morph->resistant, ris_flags );
      }
      else if( key == "Suscept" )
      {
         temp = fread_line( stream );

         flag_string_set( temp, morph->suscept, ris_flags );
      }
      else if( key == "Absorb" )
      {
         temp = fread_line( stream );

         flag_string_set( temp, morph->absorb, ris_flags );
      }
      else if( key == "NoImmune" )
      {
         temp = fread_line( stream );

         flag_string_set( temp, morph->no_immune, ris_flags );
      }
      else if( key == "NoResistant" )
      {
         temp = fread_line( stream );

         flag_string_set( temp, morph->no_resistant, ris_flags );
      }
      else if( key == "NoSuscept" )
      {
         temp = fread_line( stream );

         flag_string_set( temp, morph->no_suscept, ris_flags );
      }
      else if( key == "Armor" )
         stream >> morph->ac;
      else if( key == "Charisma" )
         stream >> morph->cha;
      else if( key == "Constitution" )
         stream >> morph->con;
      else if( key == "Damroll" )
         stream >> morph->damroll;
      else if( key == "Dexterity" )
         stream >> morph->dex;
      else if( key == "Dodge" )
         stream >> morph->dodge;
      else if( key == "Hit" )
         stream >> morph->hit;
      else if( key == "Hitroll" )
         stream >> morph->hitroll;
      else if( key == "Intelligence" )
         stream >> morph->inte;
      else if( key == "Luck" )
         stream >> morph->lck;
      else if( key == "Mana" )
         stream >> morph->mana;
      else if( key == "Move" )
         stream >> morph->move;
      else if( key == "Parry" )
         stream >> morph->parry;
      else if( key == "Save1" )
         stream >> morph->saving_breath;
      else if( key == "Save2" )
         stream >> morph->saving_para_petri;
      else if( key == "Save3" )
         stream >> morph->saving_poison_death;
      else if( key == "Save4" )
         stream >> morph->saving_spell_staff;
      else if( key == "Save5" )
         stream >> morph->saving_wand;
      else if( key == "Strength" )
         stream >> morph->str;
      else if( key == "Timer" )
         stream >> morph->timer;
      else if( key == "Tumble" )
         stream >> morph->tumble;
      else if( key == "Wisdom" )
         stream >> morph->wis;
      else if( key == "End" )
         return;
      else
      {
         bug( "{}: Bad section '{}' - skipping.", __func__, key );
         fread_to_eol( stream );
      }
   }
   bug( "{}: Fell through to bottom. Possible corrupted pfile. Deleting morph data from player.", __func__ );
}

/* 
 * Following functions are for immortal testing purposes.
 */
CMDF( do_imm_morph )
{
   morph_data *morph;
   char_data *victim = nullptr;
   std::string arg;
   int vnum;

   if( ch->isnpc(  ) )
   {
      ch->print( "Only player characters can use this command.\r\n" );
      return;
   }

   argument = one_argument( argument, arg );

   if( !is_number( arg ) )
   {
      ch->print( "Syntax: morph <vnum>\r\n" );
      return;
   }
   vnum = std::stoi( arg );
   morph = get_morph_vnum( vnum );

   if( morph == nullptr )
   {
      ch->print_fmt( "No such morph {} exists.\r\n", vnum );
      return;
   }
   if( argument.empty(  ) )
      do_morph_char( ch, morph );
   else if( !( victim = ch->get_char_world( argument ) ) )
   {
      ch->print( "No one like that in all the realms.\r\n" );
      return;
   }
   if( victim != nullptr && ch->get_trust(  ) < victim->get_trust(  ) && !victim->isnpc(  ) )
   {
      ch->print( "You can't do that!\r\n" );
      return;
   }
   else if( victim != nullptr )
      do_morph_char( victim, morph );
   ch->print( "Done.\r\n" );
}

/*
 * This is just a wrapper.  --Shaddai
 */
CMDF( do_imm_unmorph )
{
   char_data *victim = nullptr;

   if( argument.empty(  ) )
      do_unmorph_char( ch );
   else if( !( victim = ch->get_char_world( argument ) ) )
   {
      ch->print( "No one like that in all the realms.\r\n" );
      return;
   }
   if( victim != nullptr && ch->get_trust(  ) < victim->get_trust(  ) && !victim->isnpc(  ) )
   {
      ch->print( "You can't do that!\r\n" );
      return;
   }
   else if( victim != nullptr )
      do_unmorph_char( victim );
   ch->print( "Done.\r\n" );
}

/* Added by Samson 6-13-99 - lists available polymorph forms */
CMDF( do_morphlist )
{
   ch->pager( "&GVnum |&YPolymorph Name\r\n" );
   ch->pager( "&G-----+----------------------------------\r\n" );

   for( auto* morph : morphlist )
      ch->pager_fmt( "&G{:<5}  &Y{}\r\n", morph->vnum, morph->name );
}
