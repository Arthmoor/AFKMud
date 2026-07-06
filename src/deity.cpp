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
 *                           Deity handling module                          *
 ****************************************************************************/

/* Put together by Rennard for Realms of Despair.  Brap on... */

#include <filesystem>
#include <fstream>
#include "mud.h"
#include "deity.h"
#include "mobindex.h"
#include "objindex.h"
#include "raceclass.h"
#include "roomindex.h"
#include "smaugaffect.h"

bool check_pets( char_data *, mob_index * );
room_index *recall_room( char_data * );
void bind_follower( char_data *, char_data *, int, int );

std::list < deity_data * >deitylist;

deity_data::deity_data(  )
{
}

deity_data::~deity_data(  )
{
   deitylist.remove( this );
}

void free_deities( void )
{
   for( auto it = deitylist.begin(  ); it != deitylist.end(  ); )
   {
      deity_data *deity = *it;
      ++it;

      deleteptr( deity );
   }
}

/* Get pointer to deity structure from deity name */
deity_data *get_deity( std::string_view name )
{
   for( auto* deity : deitylist )
   {
      if( !str_cmp( name, deity->name ) )
         return deity;
   }
   return nullptr;
}

bool IS_DEVOTED( char_data *ch )
{
   if( !ch->isnpc() && ch->pcdata->deity != nullptr )
      return true;
   return false;
}

void write_deity_list( void )
{
   std::filesystem::path filename = std::format( "{}{}", DEITY_DIR, DEITY_LIST );
   std::ofstream stream( std::filesystem::path{filename} );
   if( !stream.is_open(  ) )
   {
      bug( "{}: Cannot open {} for writing: {}", __func__, filename.string(), std::strerror(errno) );
      return;
   }

   for( auto* deity : deitylist )
      stream << std::format( "{}\n", deity->filename );

   stream << "$\n";
   stream.close();
   if( stream.fail() )
      bug( "{}: Error occurred after closing {}: ", __func__, filename.string(), std::strerror(errno) );
}

/* Save a deity's data to its data file */
constexpr int DEITY_VERSION = 3;
/* Added for deity file compatibility. Adjani, 1-31-04 */
/* Raised to 2 by Samson to support multiple class/race settings - 5-17-04 */
/* Raised to 3 by Samson to accommodate RISA flag changes - 7-12-04 */
void save_deity( deity_data * deity )
{
   if( !deity )
   {
      bug( "{}: null deity pointer!", __func__ );
      return;
   }

   if( deity->filename.empty(  ) )
   {
      bug( "{}: {} has no filename", __func__, deity->name );
      return;
   }

   std::filesystem::path filename = std::format( "{}{}", DEITY_DIR, deity->filename );
   std::ofstream stream( std::filesystem::path{filename} );
   if( !stream.is_open() )
   {
      bug( "{}: Cannot open {} for writing: {}", __func__, filename.string(), std::strerror(errno) );
      return;
   }

   stream << std::format( "#VERSION {}\n", DEITY_VERSION );  /* Adjani, 1-31-04 */
   stream << "#DEITY\n";
   stream << std::format( "Filename        {}~\n", deity->filename );
   stream << std::format( "Name            {}~\n", deity->name );
   stream << std::format( "Description     {}~\n", strip_cr( deity->deitydesc ) );
   stream << std::format( "Alignment       {}\n", deity->alignment );
   stream << std::format( "Worshippers     {}\n", deity->worshippers );
   stream << std::format( "Flee            {}\n", deity->flee );
   stream << std::format( "Flee_npcraces   {} {} {}\n", deity->flee_npcrace[0], deity->flee_npcrace[1], deity->flee_npcrace[2] );
   stream << std::format( "Flee_npcfoes    {} {} {}\n", deity->flee_npcfoe[0], deity->flee_npcfoe[1], deity->flee_npcfoe[2] );
   stream << std::format( "Kill            {}\n", deity->kill );
   stream << std::format( "Kill_magic      {}\n", deity->kill_magic );
   stream << std::format( "Kill_npcraces   {} {} {}\n", deity->kill_npcrace[0], deity->kill_npcrace[1], deity->kill_npcrace[2] );
   stream << std::format( "Kill_npcfoes    {} {} {}\n", deity->kill_npcfoe[0], deity->kill_npcfoe[1], deity->kill_npcfoe[2] );
   stream << std::format( "Sac             {}\n", deity->sac );
   stream << std::format( "Bury_corpse     {}\n", deity->bury_corpse );
   stream << std::format( "Aid_spell       {}\n", deity->aid_spell );
   stream << std::format( "Aid             {}\n", deity->aid );
   stream << std::format( "Steal           {}\n", deity->steal );
   stream << std::format( "Backstab        {}\n", deity->backstab );
   stream << std::format( "Die             {}\n", deity->die );
   stream << std::format( "Die_npcraces    {} {} {}\n", deity->die_npcrace[0], deity->die_npcrace[1], deity->die_npcrace[2] );
   stream << std::format( "Die_npcfoes     {} {} {}\n", deity->die_npcfoe[0], deity->die_npcfoe[1], deity->die_npcfoe[2] );
   stream << std::format( "Spell_aid       {}\n", deity->spell_aid );
   stream << std::format( "Dig_corpse      {}\n", deity->dig_corpse );
   stream << std::format( "Scorpse         {}\n", deity->scorpse );
   stream << std::format( "Savatar         {}\n", deity->savatar );
   stream << std::format( "Smount          {}\n", deity->smount ); /* Added by Tarl 24 Feb 02 */
   stream << std::format( "Sminion         {}\n", deity->sminion );   /* Added by Tarl 24 Feb 02 */
   stream << std::format( "Sdeityobj       {}\n", deity->sdeityobj );
   stream << std::format( "Sdeityobj2      {}\n", deity->sdeityobj2 );   /* Added by Tarl 02 Mar 02 */
   stream << std::format( "Srecall         {}\n", deity->srecall );
   stream << std::format( "Sex             {}~\n", deity->sex < 0 ? "none" : npc_sex[deity->sex] ); /* Adjani, 2-18-04 */
   stream << std::format( "Elements        {} {} {}\n", ris_flags[deity->element[0]], ris_flags[deity->element[1]], ris_flags[deity->element[2]] );
   stream << std::format( "Affects         {} {} {}\n", aff_flags[deity->affected[0]], aff_flags[deity->affected[1]], aff_flags[deity->affected[2]] );
   stream << std::format( "Suscepts        {} {} {}\n", ris_flags[deity->suscept[0]], ris_flags[deity->suscept[1]], ris_flags[deity->suscept[2]] );
   stream << std::format( "Classes         {}~\n", deity->class_allowed.none(  )? "all" : bitset_string( deity->class_allowed, npc_class ) );
   stream << std::format( "Races           {}~\n", deity->race_allowed.none(  )? "all" : bitset_string( deity->race_allowed, npc_race ) );
   stream << std::format( "Npcrace         {}~\n", deity->npcrace[0] == -1 ? "none" : npc_race[deity->npcrace[0]] );  /* Adjani, 2-18-04 */
   stream << std::format( "Npcrace2        {}~\n", deity->npcrace[1] == -1 ? "none" : npc_race[deity->npcrace[1]] );
   stream << std::format( "Npcrace3        {}~\n", deity->npcrace[2] == -1 ? "none" : npc_race[deity->npcrace[2]] );
   stream << std::format( "Npcfoe          {}~\n", deity->npcfoe[0] == -1 ? "none" : npc_race[deity->npcfoe[0]] ); /* Adjani, 2-18-04 */
   stream << std::format( "Npcfoe2         {}~\n", deity->npcfoe[1] == -1 ? "none" : npc_race[deity->npcfoe[1]] );
   stream << std::format( "Npcfoe3         {}~\n", deity->npcfoe[2] == -1 ? "none" : npc_race[deity->npcfoe[2]] );
   stream << std::format( "Susceptnums     {} {} {}\n", deity->susceptnum[0], deity->susceptnum[1], deity->susceptnum[2] );
   stream << std::format( "Elementnums     {} {} {}\n", deity->elementnum[0], deity->elementnum[1], deity->elementnum[2] );
   stream << std::format( "Affectednums    {} {} {}\n", deity->affectednum[0], deity->affectednum[1], deity->affectednum[2] );
   stream << std::format( "Spells          {} {} {}\n", deity->spell[0], deity->spell[1], deity->spell[2] ); /* Added by Tarl 24 Mar 02 */
   stream << std::format( "Sspells         {} {} {}\n", deity->sspell[0], deity->sspell[1], deity->sspell[2] ); /* Added by Tarl 24 Mar 02 */
   stream << std::format( "Objstat         {}\n", deity->objstat );
   stream << std::format( "Recallroom      {}\n", deity->recallroom );   /* Samson */
   stream << std::format( "Avatar          {}\n", deity->avatar ); /* Restored by Samson */
   stream << std::format( "Mount           {}\n", deity->mount );  /* Added by Tarl 24 Feb 02 */
   stream << std::format( "Minion          {}\n", deity->minion ); /* Added by Tarl 24 Feb 02 */
   stream << std::format( "Deityobj        {}\n", deity->deityobj );  /* Restored by Samson */
   stream << std::format( "Deityobj2       {}\n", deity->deityobj2 ); /* Added by Tarl 02 Mar 02 */
   stream << "End\n\n";
   stream << "#END\n";
   stream.close();
   if( stream.fail() )
      bug( "{}: Error occurred after closing {}: ", __func__, filename.string(), std::strerror(errno) );
}

CMDF( do_savedeities )
{
   for( auto* deity : deitylist )
      save_deity( deity );
}

/* Adjani, versions, with help from the ever-patient Xorith. 1-31-04 */
void fread_deity( deity_data * deity, std::ifstream & stream, int file_ver )
{
   std::string key;
   while( stream >> key )
   {
      if( key == "Filename" )
         deity->filename = fread_line( stream );
      else if( key == "Name" )
         deity->name = fread_line( stream );
      else if( key == "Description" )
         deity->deitydesc = fread_line( stream );
      else if( key == "Alignment" )
         stream >> deity->alignment;
      else if( key == "Worshippers" )
         stream >> deity->worshippers;
      else if( key == "Flee" )
         stream >> deity->flee;
      else if( key == "Flee_npcraces" )
         stream >> deity->flee_npcrace[0] >> deity->flee_npcrace[1] >> deity->flee_npcrace[2];
      else if( key == "Flee_npcfoes" )
         stream >> deity->flee_npcfoe[0] >> deity->flee_npcfoe[1] >> deity->flee_npcfoe[2];
      else if( key == "Kill" )
         stream >> deity->kill;
      else if( key == "Kill_magic" )
         stream >> deity->kill_magic;
      else if( key == "Kill_npcraces" )
         stream >> deity->kill_npcrace[0] >> deity->kill_npcrace[1] >> deity->kill_npcrace[2];
      else if( key == "Kill_npcfoes" )
         stream >> deity->kill_npcfoe[0] >> deity->kill_npcfoe[1] >> deity->kill_npcfoe[2];
      else if( key == "Sac" )
         stream >> deity->sac;
      else if( key == "Bury_corpse" )
         stream >> deity->bury_corpse;
      else if( key == "Aid_spell" )
         stream >> deity->aid_spell;
      else if( key == "Aid" )
         stream >> deity->aid;
      else if( key == "Steal" )
         stream >> deity->steal;
      else if( key == "Backstab" )
         stream >> deity->backstab;
      else if( key == "Die" )
         stream >> deity->die;
      else if( key == "Die_npcraces" )
         stream >> deity->die_npcrace[0] >> deity->die_npcrace[1] >> deity->die_npcrace[2];
      else if( key == "Die_npcfoes" )
         stream >> deity->die_npcfoe[0] >> deity->die_npcfoe[1] >> deity->die_npcfoe[2];
      else if( key == "Spell_aid" )
         stream >> deity->spell_aid;
      else if( key == "Dig_corpse" )
         stream >> deity->dig_corpse;
      else if( key == "Scorpse" )
         stream >> deity->scorpse;
      else if( key == "Savatar" )
         stream >> deity->savatar;
      else if( key == "Smount" )
         stream >> deity->smount;
      else if( key == "Sminion" )
         stream >> deity->sminion;
      else if( key == "Sdeityobj" )
         stream >> deity->sdeityobj;
      else if( key == "Sdeityobj2" )
         stream >> deity->sdeityobj2;
      else if( key == "Srecall" )
         stream >> deity->srecall;
      else if( key == "Sex" )
      {
         int sex;

         if( file_ver == 0 )
            stream >> sex;
         else
         {
            sex = get_npc_sex( fread_line( stream ) );
            if( sex < -1 || sex >= SEX_MAX )
            {
               bug( "{}: Deity {} has invalid {}! Defaulting to neuter.", __func__, deity->name, key );
               sex = SEX_NEUTRAL;
            }
            deity->sex = sex;
         }
      }
      else if( key == "Elements" )
      {
         std::string temp = fread_word( stream );
         std::string temp2 = fread_word( stream );
         std::string temp3 = fread_word( stream );
         int value;

         value = get_risflag( temp );
         if( value < 0 || value >= MAX_RIS_FLAG )
            bug( "{}: Invalid RISA flag for {}: {}", __func__, key, temp );
         else
            deity->element[0] = value;

         value = get_risflag( temp2 );
         if( value < 0 || value >= MAX_RIS_FLAG )
            bug( "{}: Invalid RISA flag for {}: {}", __func__, key, temp2 );
         else
            deity->element[1] = value;

         value = get_risflag( temp3 );
         if( value < 0 || value >= MAX_RIS_FLAG )
            bug( "{}: Invalid RISA flag for {}: {}", __func__, key, temp3 );
         else
            deity->element[2] = value;
      }
      else if( key == "Affects" )
      {
         std::string temp = fread_word( stream );
         std::string temp2 = fread_word( stream );
         std::string temp3 = fread_word( stream );
         int value;

         value = get_risflag( temp );
         if( value < 0 || value >= MAX_RIS_FLAG )
            bug( "{}: Invalid RISA flag for {}: {}", __func__, key, temp );
         else
            deity->affected[0] = value;

         value = get_risflag( temp2 );
         if( value < 0 || value >= MAX_RIS_FLAG )
            bug( "{}: Invalid RISA flag for {}: {}", __func__, key, temp2 );
         else
            deity->affected[1] = value;

         value = get_risflag( temp3 );
         if( value < 0 || value >= MAX_RIS_FLAG )
            bug( "{}: Invalid RISA flag for {}: {}", __func__, key, temp3 );
         else
            deity->affected[2] = value;
      }
      else if( key == "Suscepts" )
      {
         std::string temp = fread_word( stream );
         std::string temp2 = fread_word( stream );
         std::string temp3 = fread_word( stream );
         int value;

         value = get_risflag( temp );
         if( value < 0 || value >= MAX_RIS_FLAG )
            bug( "{}: Invalid RISA flag for {}: {}", __func__, key, temp );
         else
            deity->suscept[0] = value;

         value = get_risflag( temp2 );
         if( value < 0 || value >= MAX_RIS_FLAG )
            bug( "{}: Invalid RISA flag for {}: {}", __func__, key, temp2 );
         else
            deity->suscept[1] = value;

         value = get_risflag( temp3 );
         if( value < 0 || value >= MAX_RIS_FLAG )
            bug( "{}: Invalid RISA flag for {}: {}", __func__, key, temp3 );
         else
            deity->suscept[2] = value;
      }
      else if( key == "Classes" )
      {
         std::string Class, flag;
         int value;

         Class = fread_line( stream );

         if( !str_cmp( Class, "all" ) )
            deity->class_allowed.reset(  );
         else
         {
            while( !Class.empty() )
            {
               Class = one_argument( Class, flag );
               value = get_pc_class( flag );
               if( value < 0 || value > MAX_CLASS )
                  bug( "Unknown PC Class: {}", flag );
               else
                  deity->class_allowed.set( value );
            }
         }
      }
      else if( key == "Races" )
      {
         /*
          * Cleaned up way to handle deity races - Samson 5-17-04
          */
         std::string race, flag;
         int value;

         race = fread_line( stream );

         if( !str_cmp( race, "all" ) )
            deity->race_allowed.reset(  );
         else
         {
            while( !race.empty() )
            {
               race = one_argument( race, flag );
               value = get_pc_race( flag );
               if( value < 0 || value > MAX_PC_RACE )
                  bug( "Unknown PC Race: {}", flag );
               else
                  deity->race_allowed.set( value );
            }
         }
      }
      else if( key == "Npcrace" )
      {
         int npcrace;

         if( file_ver == 0 )
            stream >> npcrace;
         else
            npcrace = get_npc_race( fread_line( stream ) );

         if( npcrace < -1 || npcrace >= MAX_NPC_RACE )
         {
            bug( "{}: Deity {} has invalid {}! Defaulting to human.", __func__, deity->name, key );
            npcrace = RACE_HUMAN;
         }
         deity->npcrace[0] = npcrace;
      }
      else if( key == "Npcrace2" )
      {
         int npcrace2;

         if( file_ver == 0 )
            stream >> npcrace2;
         else
            npcrace2 = get_npc_race( fread_line( stream ) );
         if( npcrace2 < -1 || npcrace2 >= MAX_NPC_RACE )
         {
            bug( "{}: Deity {} has invalid {}! Defaulting to human.", __func__, deity->name, key );
            npcrace2 = RACE_HUMAN;
         }
         deity->npcrace[1] = npcrace2;
      }
      else if( key == "Npcrace3" )
      {
         int npcrace3;

         if( file_ver == 0 )
            stream >> npcrace3;
         else
            npcrace3 = get_npc_race( fread_line( stream ) );
         if( npcrace3 < -1 || npcrace3 >= MAX_NPC_RACE )
         {
            bug( "{}: Deity {} has invalid {}! Defaulting to human.", __func__, deity->name, key );
            npcrace3 = RACE_HUMAN;
         }
         deity->npcrace[2] = npcrace3;
      }
      else if( key == "Npcfoe" )
      {
         int npcfoe;

         if( file_ver == 0 )
            stream >> npcfoe;
         else
            npcfoe = get_npc_race( fread_line( stream ) );

         if( npcfoe < -1 || npcfoe >= MAX_NPC_RACE )
         {
            bug( "{}: Deity {} has invalid {}! Defaulting to human.", __func__, deity->name, key );
            npcfoe = RACE_HUMAN;
         }
         deity->npcfoe[0] = npcfoe;
      }
      else if( key == "Npcfoe2" )
      {
         int npcfoe2;

         if( file_ver == 0 )
            stream >> npcfoe2;
         else
            npcfoe2 = get_npc_race( fread_line( stream ) );
         if( npcfoe2 < -1 || npcfoe2 >= MAX_NPC_RACE )
         {
            bug( "{}: Deity {} has invalid {}! Defaulting to human.", __func__, deity->name, key );
            npcfoe2 = RACE_HUMAN;
         }
         deity->npcfoe[1] = npcfoe2;
      }
      else if( key == "Npcfoe3" )
      {
         int npcfoe3;

         if( file_ver == 0 )
            stream >> npcfoe3;
         else
            npcfoe3 = get_npc_race( fread_line( stream ) );
         if( npcfoe3 < -1 || npcfoe3 >= MAX_NPC_RACE )
         {
            bug( "{}: Deity {} has invalid {}! Defaulting to human.", __func__, deity->name, key );
            npcfoe3 = RACE_HUMAN;
         }
         deity->npcfoe[2] = npcfoe3;
      }
      else if( key == "Susceptnums" )
         stream >> deity->susceptnum[0] >> deity->susceptnum[1] >>  deity->susceptnum[2];
      else if( key =="Elementnums" )
         stream >> deity->elementnum[0] >> deity->elementnum[1] >> deity->elementnum[2];
      else if( key == "Affectednums" )
         stream >> deity->affectednum[0] >> deity->affectednum[1] >> deity->affectednum[2];
      else if( key == "Spells" ) // Added by Tarl 24 Mar 02
         stream >> deity->spell[0] >> deity->spell[1] >> deity->spell[2];
      else if( key == "Sspells" ) // Added by Tarl 24 Mar 02
         stream >> deity->sspell[0] >> deity->sspell[1] >> deity->sspell[2];
      else if( key == "Objstat" )
         stream >> deity->objstat;
      else if( key == "Recallroom" ) // Samson
         stream >> deity->recallroom;
      else if( key == "Avatar" ) // Restored by Samson
         stream >> deity->avatar;
      else if( key == "Mount" ) // Added by Tarl 24 Feb 02
         stream >> deity->mount;
      else if( key == "Minion" ) // Added by Tarl 24 Feb 02
         stream >> deity->minion;
      else if( key == "Deityobj" ) // Restored by Samson
         stream >> deity->deityobj;
      else if( key == "Deityobj2" ) // Added by Tarl 02 Mar 02
         stream >> deity->deityobj2;
      else if( key == "End" )
         return;
      else
      {
         bug( "{}: Bad section '{}' in {} - skipping.", __func__, key, !deity->filename.empty() ? deity->filename : "" );
         fread_to_eol( stream );
      }
   }
}

/* Load a deity file */
bool load_deity_file( std::string_view deityfile )
{
   std::filesystem::path filename = std::format( "{}{}", DEITY_DIR, deityfile );
   std::ifstream stream( std::filesystem::path{filename} );
   if( !stream.is_open() )
   {
      bug( "{}: Cannot open {} for reading: {}", __func__, filename.string(), std::strerror(errno) );
      return false;
   }

   int file_ver = 0;
   deity_data *deity = nullptr;
   std::string key;
   while( stream >> key )
   {
      if( key == "#VERSION" ) // Adjani, 1-31-04
         stream >> file_ver;
      else if( key == "#DEITY" )
      {
         deity = new deity_data;
         fread_deity( deity, stream, file_ver ); // File_ver added for file versions. Whee! Adjani, 1-31-04
         deitylist.push_back( deity );
         stream.close();
         return true;
      }
      else
      {
         bug( "{}: Bad section '{}' in {} - skipping.", __func__, key, filename.string() );
         fread_to_eol( stream );
      }
   }
   return false;
}

/* Load in all the deity files */
void load_deity( void )
{
   log_string( "Loading deities..." );

   std::filesystem::path deitylistfile = std::format( "{}{}", DEITY_DIR, DEITY_LIST );
   std::ifstream stream( std::filesystem::path{deitylistfile} );
   if( !stream.is_open(  ) )
   {
      bug( "{}: Cannot open {} for reading: {}", __func__, deitylistfile.string(), std::strerror(errno) );
      std::exit( EXIT_FAILURE );
   }

   deitylist.clear(  );

   for( ;; )
   {
      std::string filename = fread_line( stream, '\n' );

      if( filename[0] == '$' )
         break;

      if( !load_deity_file( filename ) )
         bug( "{}: Cannot load deity file: {} - {}", __func__, filename, std::strerror(errno) );
   }
   stream.close();
   log_string( "Done: Deities." );
}

CMDF( do_setdeity )
{
   std::string arg1, arg2, arg3;
   deity_data *deity;
   int value;

   if( ch->isnpc(  ) )
   {
      ch->print( "Huh?\r\n" );
      return;
   }

   switch ( ch->substate )
   {
      default:
         break;

      case SUB_RESTRICTED:
         ch->print( "You cannot do this while in another command.\r\n" );
         return;

      case SUB_DEITYDESC:
         deity = ( deity_data * ) ch->pcdata->dest_buf;
         deity->deitydesc = ch->copy_buffer( );
         ch->stop_editing(  );
         save_deity( deity );
         ch->substate = ch->tempnum;
         return;

      case SUB_EDIT_ABORT:
         ch->substate = SUB_NONE;
         ch->print( "Aborting description.\r\n" );
         return;
   }

   smash_tilde( argument );
   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );

   if( arg1.empty(  ) )
   {
      ch->print( "Usage: setdeity <deity> <field> <toggle>\r\n" );
      ch->print( "\r\nField being one of:\r\n" );
      ch->print( "filename name description alignment worshippers minion sex race\r\n" );
      ch->print( "class npcfoe npcfoe2 npcfoe3 npcrace npcrace2 npcrace3\r\n" );
      ch->print( "element element2 element3 elementnum elementnum2\r\n" );
      ch->print( "elementnum3 affectednum affectednum2 affectednum3 affected affected2\r\n" );
      ch->print( "affected3 susceptnum susceptnum2 susceptnum3 suscept suscept2 suscept3\r\n" );
      ch->print( "recallroom avatar mount object object2 delete\r\n" );
      ch->print( "\r\nFavor adjustments:\r\n" );
      ch->print( "flee flee_npcrace flee_npcrace2 flee_npcrace3 flee_npcfoe flee_npcfoe2\r\n" );
      ch->print( "flee_npcfoe3 kill kill_npcrace kill_npcrace2 kill_npcrace3 kill_npcfoe\r\n" );
      ch->print( "kill_npcfoe2 kill_npcfoe3 kill_magic die die_npcrace die_npcrace2\r\n" );
      ch->print( "die_npcrace3 die_npcfoe die_npcfoe2 die_npcfoe3 dig_corpse bury_corpse\r\n" );
      ch->print( "steal backstab aid aid_spell spell_aid sac\r\n" );
      ch->print( "\r\nFavor requirements for supplicate:\r\n" );
      ch->print( "scorpse savatar smount sdeityobj sdeityobj2 srecall spell1 spell2\r\n" );
      ch->print( "spell3 sspell1 sspell2 sspell3\r\n" ); /* Added by Tarl 24 Mar 02 */
      ch->print( "Objstat - being one of:\r\n" );
      ch->print( "str int wis con dex cha lck\r\n" );
      ch->print( " 0 - 1 - 2 - 3 - 4 - 5 - 6\r\n" );  /* Entire list tinkered with by Adjani, 1-27-04 */
      return;
   }

   if( !( deity = get_deity( arg1 ) ) )
   {
      ch->print( "No such deity.\r\n" );
      return;
   }

   if( !str_cmp( arg2, "delete" ) )
   {
      for( auto* vch : pclist )
      {
         if( vch->pcdata->deity == deity )
         {
            std::string buf = std::format( "&R\r\nYour deity, {}, has met its demise!\r\n", vch->pcdata->deity_name );
            if( !vch->desc )
               add_login_message( vch->name, 18, buf );
            else
               vch->print( buf );

            vch->unset_aflag( ch->pcdata->deity->affected[0] );
            vch->unset_aflag( ch->pcdata->deity->affected[1] );
            vch->unset_aflag( ch->pcdata->deity->affected[2] );
            vch->unset_resist( ch->pcdata->deity->element[0] );
            vch->unset_resist( ch->pcdata->deity->element[1] );
            vch->unset_resist( ch->pcdata->deity->element[2] );
            vch->unset_suscep( ch->pcdata->deity->suscept[0] );
            vch->unset_suscep( ch->pcdata->deity->suscept[1] );
            vch->unset_suscep( ch->pcdata->deity->suscept[2] );
            vch->pcdata->deity = nullptr;
            vch->pcdata->deity_name.clear(  );
            vch->update_aris(  );
            vch->save(  );
         }
      }

      std::filesystem::path filename = std::format( "{}{}", DEITY_DIR, deity->filename );
      deleteptr( deity );
      ch->print_fmt( "&YDeity information for {} deleted.\r\n", arg1 );

      if( std::filesystem::remove( filename ) )
         ch->print_fmt( "&RDeity file for {} destroyed.\r\n", arg1 );

      write_deity_list(  );
      return;
   }

   if( !str_cmp( arg2, "name" ) )
   {
      deity_data *udeity;

      if( argument.empty(  ) )
      {
         ch->print( "You can't set a deity's name to nothing.\r\n" );
         return;
      }
      if( ( udeity = get_deity( argument ) ) )
      {
         ch->print( "There is already another deity with that name.\r\n" );
         return;
      }
      deity->name = argument;
      ch->print( "Done.\r\n" );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "avatar" ) )
   {
      deity->avatar = std::stoi( argument );
      ch->print( "Done.\r\n" );
      save_deity( deity );
      return;
   }

   /*
    * Added by Tarl 24 Feb 02 
    */
   if( !str_cmp( arg2, "mount" ) )
   {
      deity->mount = std::stoi( argument );
      ch->print( "Done.\r\n" );
      save_deity( deity );
      return;
   }

   /*
    * Added by Tarl 24 Feb 02 
    */
   if( !str_cmp( arg2, "minion" ) )
   {
      deity->minion = std::stoi( argument );
      ch->print( "Done.\r\n" );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "object" ) )
   {
      deity->deityobj = std::stoi( argument );
      ch->print( "Done.\r\n" );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "object2" ) )
   {
      deity->deityobj2 = std::stoi( argument );
      ch->print( "Done.\r\n" );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "recallroom" ) )
   {
      deity->recallroom = std::stoi( argument );
      ch->print( "Done.\r\n" );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "filename" ) )
   {
      if( !is_valid_filename( ch, DEITY_DIR, argument ) )
         return;

      std::filesystem::path filename = std::format( "{}{}", DEITY_DIR, deity->filename );
      if( std::filesystem::remove( filename ) )
         ch->print( "Old deity file deleted.\r\n" );
      deity->filename = argument;
      ch->print( "Done.\r\n" );
      save_deity( deity );
      write_deity_list(  );
      return;
   }

   if( !str_cmp( arg2, "description" ) )
   {
      if( ch->substate == SUB_REPEATCMD )
         ch->tempnum = SUB_REPEATCMD;
      else
         ch->tempnum = SUB_NONE;
      ch->substate = SUB_DEITYDESC;
      ch->pcdata->dest_buf = deity;
      ch->set_editor_desc( std::format( "Deity description for deity '{}'.", deity->name ) );
      ch->start_editing( deity->deitydesc );
      return;
   }

   if( !str_cmp( arg2, "alignment" ) )
   {
      deity->alignment = std::stoi( argument );
      ch->print( "Done.\r\n" );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "flee" ) )
   {
      deity->flee = std::stoi( argument );
      ch->print( "Done.\r\n" );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "flee_npcrace" ) )
   {
      deity->flee_npcrace[0] = std::stoi( argument );
      ch->print( "Done.\r\n" );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "flee_npcrace2" ) )   /* flee_npcrace2, flee_npcrace3, flee_npcfoe2, flee_npcfoe3 - Adjani, 1-27-04 */
   {
      deity->flee_npcrace[1] = std::stoi( argument );
      ch->print( "Done.\r\n" );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "flee_npcrace3" ) )
   {
      deity->flee_npcrace[2] = std::stoi( argument );
      ch->print( "Done.\r\n" );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "flee_npcfoe" ) )
   {
      deity->flee_npcfoe[0] = std::stoi( argument );
      ch->print( "Done.\r\n" );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "flee_npcfoe2" ) )
   {
      deity->flee_npcfoe[1] = std::stoi( argument );
      ch->print( "Done.\r\n" );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "flee_npcfoe3" ) )
   {
      deity->flee_npcfoe[2] = std::stoi( argument );
      ch->print( "Done.\r\n" );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "kill" ) )
   {
      deity->kill = std::stoi( argument );
      ch->print( "Done.\r\n" );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "kill_npcrace" ) )
   {
      deity->kill_npcrace[0] = std::stoi( argument );
      ch->print( "Done.\r\n" );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "kill_npcrace2" ) )   /* kill_npcrace2, kill_npcrace3, kill_npcfoe2, kill_npcfoe3 - Adjani, 1-24-04 */
   {
      deity->kill_npcrace[1] = std::stoi( argument );
      ch->print( "Done.\r\n" );
      save_deity( deity );
      return;
   }
   if( !str_cmp( arg2, "kill_npcrace3" ) )
   {
      deity->kill_npcrace[2] = std::stoi( argument );
      ch->print( "Done.\r\n" );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "kill_npcfoe" ) )
   {
      deity->kill_npcfoe[0] = std::stoi( argument );
      ch->print( "Done.\r\n" );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "kill_npcfoe2" ) )
   {
      deity->kill_npcfoe[1] = std::stoi( argument );
      ch->print( "Done.\r\n" );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "kill_npcfoe3" ) )
   {
      deity->kill_npcfoe[2] = std::stoi( argument );
      ch->print( "Done.\r\n" );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "kill_magic" ) )
   {
      deity->kill_magic = std::stoi( argument );
      ch->print( "Done.\r\n" );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "sac" ) )
   {
      deity->sac = std::stoi( argument );
      ch->print( "Done.\r\n" );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "bury_corpse" ) )
   {
      deity->bury_corpse = std::stoi( argument );
      ch->print( "Done.\r\n" );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "aid_spell" ) )
   {
      deity->aid_spell = std::stoi( argument );
      ch->print( "Done.\r\n" );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "aid" ) )
   {
      deity->aid = std::stoi( argument );
      ch->print( "Done.\r\n" );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "steal" ) )
   {
      deity->steal = std::stoi( argument );
      ch->print( "Done.\r\n" );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "backstab" ) )
   {
      deity->backstab = std::stoi( argument );
      ch->print( "Done.\r\n" );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "die" ) )
   {
      deity->die = std::stoi( argument );
      ch->print( "Done.\r\n" );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "die_npcrace" ) )
   {
      deity->die_npcrace[0] = std::stoi( argument );
      ch->print( "Done.\r\n" );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "die_npcrace2" ) ) /* die_npcrace2, die_npcrace3, die_npcfoe2, die_npcfoe3 - Adjani, 1-24-04 */
   {
      deity->die_npcrace[1] = std::stoi( argument );
      ch->print( "Done.\r\n" );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "die_npcrace3" ) )
   {
      deity->die_npcrace[2] = std::stoi( argument );
      ch->print( "Done.\r\n" );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "die_npcfoe" ) )
   {
      deity->die_npcfoe[0] = std::stoi( argument );
      ch->print( "Done.\r\n" );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "die_npcfoe2" ) )
   {
      deity->die_npcfoe[1] = std::stoi( argument );
      ch->print( "Done.\r\n" );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "die_npcfoe3" ) )
   {
      deity->die_npcfoe[2] = std::stoi( argument );
      ch->print( "Done.\r\n" );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "spell_aid" ) )
   {
      deity->spell_aid = std::stoi( argument );
      ch->print( "Done.\r\n" );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "dig_corpse" ) )
   {
      deity->dig_corpse = std::stoi( argument );
      ch->print( "Done.\r\n" );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "scorpse" ) )
   {
      deity->scorpse = std::stoi( argument );
      ch->print( "Done.\r\n" );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "savatar" ) )
   {
      deity->savatar = std::stoi( argument );
      ch->print( "Done.\r\n" );
      save_deity( deity );
      return;
   }

   /*
    * Added by Tarl 24 Feb 02 
    */
   if( !str_cmp( arg2, "smount" ) )
   {
      deity->smount = std::stoi( argument );
      ch->print( "Done.\r\n" );
      save_deity( deity );
      return;
   }

   /*
    * Added by Tarl 24 Feb 02 
    */
   if( !str_cmp( arg2, "sminion" ) )
   {
      deity->sminion = std::stoi( argument );
      ch->print( "Done.\r\n" );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "sdeityobj" ) )
   {
      deity->sdeityobj = std::stoi( argument );
      ch->print( "Done.\r\n" );
      save_deity( deity );
      return;
   }

   /*
    * Added by Tarl 24 Feb 02 
    */
   if( !str_cmp( arg2, "sdeityobj2" ) )
   {
      deity->sdeityobj2 = std::stoi( argument );
      ch->print( "Done.\r\n" );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "objstat" ) )
   {
      deity->objstat = std::stoi( argument );
      ch->print( "Done.\r\n" );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "srecall" ) )
   {
      deity->srecall = std::stoi( argument );
      ch->print( "Done.\r\n" );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "worshippers" ) )
   {
      deity->worshippers = std::stoi( argument );
      ch->print( "Done.\r\n" );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "race" ) )
   {
      int i;

      if( !str_cmp( argument, "all" ) )
      {
         deity->race_allowed.reset(  );
         save_deity( deity );
         ch->print( "Deity now allows all races.\r\n" );
         return;
      }
      for( i = 0; i < MAX_PC_RACE; ++i )
      {
         if( !str_cmp( argument, race_table[i]->race_name ) )
         {
            deity->race_allowed.flip( i );   /* k, that's boggling */
            save_deity( deity );
            ch->print( "Races set.\r\n" );
            return;
         }
      }
      ch->print( "No such race.\r\n" );
      return;
   }

   if( !str_cmp( arg2, "npcrace" ) )   /* npcrace, npcrace2, npcrace3 - Adjani, 2-18-04 */
   {
      bool fMatch = false;

      argument = one_argument( argument, arg3 );
      if( !str_cmp( arg3, "none" ) )
      {
         fMatch = true;
         deity->npcrace[0] = -1;
      }
      else
      {
         value = get_npc_race( arg3 );
         if( value < 0 || value > MAX_NPC_RACE )
            ch->print_fmt( "Unknown npcrace: {}\r\n", arg3 );
         else
         {
            deity->npcrace[0] = value;
            fMatch = true;
         }
      }
      if( fMatch )
         ch->print( "Done.\r\n" );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "npcrace2" ) )
   {
      bool fMatch = false;

      argument = one_argument( argument, arg3 );
      if( !str_cmp( arg3, "none" ) )
      {
         fMatch = true;
         deity->npcrace[1] = -1;
      }
      else
      {
         value = get_npc_race( arg3 );
         if( value < 0 || value > MAX_NPC_RACE )
            ch->print_fmt( "Unknown npcrace2: {}\r\n", arg3 );
         else
         {
            deity->npcrace[1] = value;
            fMatch = true;
         }
      }
      if( fMatch )
         ch->print( "Done.\r\n" );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "npcrace3" ) )
   {
      bool fMatch = false;

      argument = one_argument( argument, arg3 );
      if( !str_cmp( arg3, "none" ) )
      {
         fMatch = true;
         deity->npcrace[2] = -1;
      }
      else
      {
         value = get_npc_race( arg3 );
         if( value < 0 || value > MAX_NPC_RACE )
            ch->print_fmt( "Unknown npcrace3: {}\r\n", arg3 );
         else
         {
            deity->npcrace[2] = value;
            fMatch = true;
         }
      }
      if( fMatch )
         ch->print( "Done.\r\n" );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "npcfoe" ) ) /* npcfoe, npcfoe2, npcfoe3 - Adjani, 2-18-04 */
   {
      bool fMatch = false;

      argument = one_argument( argument, arg3 );
      if( !str_cmp( arg3, "none" ) )
      {
         fMatch = true;
         deity->npcfoe[0] = -1;
      }
      else
      {
         value = get_npc_race( arg3 );
         if( value < 0 || value > MAX_NPC_RACE )
            ch->print_fmt( "Unknown npcfoe: {}\r\n", arg3 );
         else
         {
            deity->npcfoe[0] = value;
            fMatch = true;
         }
      }
      if( fMatch )
         ch->print( "Done.\r\n" );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "npcfoe2" ) )
   {
      bool fMatch = false;

      argument = one_argument( argument, arg3 );
      if( !str_cmp( arg3, "none" ) )
      {
         fMatch = true;
         deity->npcfoe[1] = -1;
      }
      else
      {
         value = get_npc_race( arg3 );
         if( value < 0 || value > MAX_NPC_RACE )
            ch->print_fmt( "Unknown npcfoe2: {}\r\n", arg3 );
         else
         {
            deity->npcfoe[1] = value;
            fMatch = true;
         }
      }
      if( fMatch )
         ch->print( "Done.\r\n" );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "npcfoe3" ) )
   {
      bool fMatch = false;

      argument = one_argument( argument, arg3 );
      if( !str_cmp( arg3, "none" ) )
      {
         fMatch = true;
         deity->npcfoe[2] = -1;
      }
      else
      {
         value = get_npc_race( arg3 );
         if( value < 0 || value > MAX_NPC_RACE )
            ch->print_fmt( "Unknown npcfoe3: {}\r\n", arg3 );
         else
         {
            deity->npcfoe[2] = value;
            fMatch = true;
         }
      }
      if( fMatch )
         ch->print( "Done.\r\n" );
      save_deity( deity );
      return;
   }

   /*
    * Samson redid this section to enable deities to mix and match as many classes as they see fit - 5-17-04 
    */
   if( !str_cmp( arg2, "class" ) )
   {
      int i;

      if( !str_cmp( argument, "all" ) )
      {
         deity->class_allowed.reset(  );
         save_deity( deity );
         ch->print( "Deity now allows all classes.\r\n" );
         return;
      }
      for( i = 0; i < MAX_PC_CLASS; ++i )
      {
         if( !str_cmp( argument, class_table[i]->who_name ) )
         {
            deity->class_allowed.flip( i );  /* k, that's boggling */
            save_deity( deity );
            ch->print( "Classes set.\r\n" );
            return;
         }
      }
      ch->print( "No such class.\r\n" );
      return;
   }

   if( !str_cmp( arg2, "susceptnum" ) )
   {
      deity->susceptnum[0] = std::stoi( argument );
      ch->print( "Done.\r\n" );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "susceptnum2" ) )  /* Added by Tarl 24 Feb 02 */
   {
      deity->susceptnum[1] = std::stoi( argument );
      ch->print( "Done.\r\n" );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "susceptnum3" ) )  /* Added by Tarl 24 Feb 02 */
   {
      deity->susceptnum[2] = std::stoi( argument );
      ch->print( "Done.\r\n" );
      save_deity( deity );
      return;
   }
   if( !str_cmp( arg2, "elementnum" ) )
   {
      deity->elementnum[0] = std::stoi( argument );
      ch->print( "Done.\r\n" );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "elementnum2" ) )  /* Added by Tarl 24 Feb 02 */
   {
      deity->elementnum[1] = std::stoi( argument );
      ch->print( "Done.\r\n" );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "elementnum3" ) )  /* Added by Tarl 24 Feb 02 */
   {
      deity->elementnum[2] = std::stoi( argument );
      ch->print( "Done.\r\n" );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "affectednum" ) )
   {
      deity->affectednum[0] = std::stoi( argument );
      ch->print( "Done.\r\n" );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "affectednum2" ) ) /* Added by Tarl 24 Feb 02 */
   {
      deity->affectednum[1] = std::stoi( argument );
      ch->print( "Done.\r\n" );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "affectednum3" ) ) /* Added by Tarl 24 Feb 02 */
   {
      deity->affectednum[2] = std::stoi( argument );
      ch->print( "Done.\r\n" );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "spell1" ) ) /* Added by Tarl 24 Mar 02 */
   {
      if( skill_lookup( argument ) < 0 )
      {
         ch->print( "No skill/spell by that name.\r\n" );
         return;
      }
      deity->spell[0] = skill_lookup( argument );
      ch->print( "Done.\r\n" );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "spell2" ) ) /* Added by Tarl 24 Mar 02 */
   {
      if( skill_lookup( argument ) < 0 )
      {
         ch->print( "No skill/spell by that name.\r\n" );
         return;
      }
      deity->spell[1] = skill_lookup( argument );
      ch->print( "Done.\r\n" );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "spell3" ) ) /* Added by Tarl 24 Mar 02 */
   {
      if( skill_lookup( argument ) < 0 )
      {
         ch->print( "No skill/spell by that name.\r\n" );
         return;
      }
      deity->spell[2] = skill_lookup( argument );
      ch->print( "Done.\r\n" );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "Sspell1" ) )   /* Added by Tarl 24 Mar 02 */
   {
      deity->sspell[0] = std::stoi( argument );
      ch->print( "Done.\r\n" );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "Sspell2" ) )   /* Added by Tarl 24 Mar 02 */
   {
      deity->sspell[1] = std::stoi( argument );
      ch->print( "Done.\r\n" );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "Sspell3" ) )   /* Added by Tarl 24 Mar 02 */
   {
      deity->sspell[2] = std::stoi( argument );
      ch->print( "Done.\r\n" );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "suscept" ) )   /* suscept, suscept2, suscept3 - Adjani, 2-18-04 */
   {
      bool fMatch = false;

      argument = one_argument( argument, arg3 );

      value = get_risflag( arg3 );
      if( value < 0 || value >= MAX_RIS_FLAG )
         ch->print_fmt( "Unknown flag: {}\r\n", arg3 );
      else
      {
         deity->suscept[0] = value;
         fMatch = true;
      }
      if( fMatch )
      {
         ch->print( "Done.\r\n" );
         save_deity( deity );
      }
      return;
   }

   if( !str_cmp( arg2, "suscept2" ) )  /* Added by Tarl 24 Feb 02 */
   {
      bool fMatch = false;

      argument = one_argument( argument, arg3 );

      value = get_risflag( arg3 );
      if( value < 0 || value >= MAX_RIS_FLAG )
         ch->print_fmt( "Unknown flag: {}\r\n", arg3 );
      else
      {
         deity->suscept[1] = value;
         fMatch = true;
      }
      if( fMatch )
      {
         ch->print( "Done.\r\n" );
         save_deity( deity );
      }
      return;
   }

   if( !str_cmp( arg2, "suscept3" ) )  /* Added by Tarl 24 Feb 02 */
   {
      bool fMatch = false;

      argument = one_argument( argument, arg3 );

      value = get_risflag( arg3 );
      if( value < 0 || value >= MAX_RIS_FLAG )
         ch->print_fmt( "Unknown flag: {}\r\n", arg3 );
      else
      {
         deity->suscept[2] = value;
         fMatch = true;
      }
      if( fMatch )
      {
         ch->print( "Done.\r\n" );
         save_deity( deity );
      }
      return;
   }

   if( !str_cmp( arg2, "element" ) )   /* element, element2, element3 - Adjani, 2-18-04 */
   {
      bool fMatch = false;

      argument = one_argument( argument, arg3 );

      value = get_risflag( arg3 );
      if( value < 0 || value >= MAX_RIS_FLAG )
         ch->print_fmt( "Unknown flag: {}\r\n", arg3 );
      else
      {
         deity->element[0] = value;
         fMatch = true;
      }
      if( fMatch )
      {
         ch->print( "Done.\r\n" );
         save_deity( deity );
      }
      return;
   }

   if( !str_cmp( arg2, "element2" ) )  /* Added by Tarl 24 Feb 02 */
   {
      bool fMatch = false;

      argument = one_argument( argument, arg3 );

      value = get_risflag( arg3 );
      if( value < 0 || value >= MAX_RIS_FLAG )
         ch->print_fmt( "Unknown flag: {}\r\n", arg3 );
      else
      {
         deity->element[1] = value;
         fMatch = true;
      }
      if( fMatch )
      {
         ch->print( "Done.\r\n" );
         save_deity( deity );
      }
      return;
   }

   if( !str_cmp( arg2, "element3" ) )  /* Added by Tarl 24 Feb 02 */
   {
      bool fMatch = false;

      argument = one_argument( argument, arg3 );

      value = get_risflag( arg3 );
      if( value < 0 || value >= MAX_RIS_FLAG )
         ch->print_fmt( "Unknown flag: {}\r\n", arg3 );
      else
      {
         deity->element[2] = value;
         fMatch = true;
      }
      if( fMatch )
      {
         ch->print( "Done.\r\n" );
         save_deity( deity );
      }
      return;
   }

   if( !str_cmp( arg2, "sex" ) || !str_cmp( arg2, "gender" ) ) /* Adjani, 2-18-04 */
   {
      bool fMatch = false;

      argument = one_argument( argument, arg3 );
      if( !str_cmp( arg3, "none" ) )
      {
         fMatch = true;
         deity->sex = -1;
      }
      else
      {
         value = get_npc_sex( arg3 );
         if( value < 0 || value > SEX_MAX )
            ch->print_fmt( "Unknown gender: {}\r\n", arg3 );
         else
         {
            deity->sex = value;
            fMatch = true;
         }
      }
      if( fMatch )
         ch->print( "Done.\r\n" );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "affected" ) )  /* affected, affected2, affected3 - Adjani, 2-18-04 */
   {
      value = get_aflag( argument );
      if( value < 0 || value >= MAX_AFFECTED_BY )
         ch->print_fmt( "Unknown flag: {}\r\n", argument );
      else
      {
         deity->affected[0] = value;
         save_deity( deity );
         ch->print_fmt( "Affected '{}' set.\r\n", argument );
      }
      return;
   }

   if( !str_cmp( arg2, "affected2" ) )
   {
      value = get_aflag( argument );
      if( value < 0 || value >= MAX_AFFECTED_BY )
         ch->print_fmt( "Unknown flag: {}\r\n", argument );
      else
      {
         deity->affected[1] = value;
         save_deity( deity );
         ch->print_fmt( "Affected2 '{}' set.\r\n", argument );
      }
      return;
   }

   if( !str_cmp( arg2, "affected3" ) )
   {
      value = get_aflag( argument );
      if( value < 0 || value >= MAX_AFFECTED_BY )
         ch->print_fmt( "Unknown flag: {}\r\n", argument );
      else
      {
         deity->affected[2] = value;
         save_deity( deity );
         ch->print_fmt( "Affected3 '{}' set.\r\n", argument );
      }
      return;
   }
}

/* Regrouped by Adjani 12-03-2002 and again on 1-27-04 and once more on 1-31-04. */
CMDF( do_showdeity )
{
   deity_data *deity;

   if( ch->isnpc(  ) )
   {
      ch->print( "Huh?\r\n" );
      return;
   }
   if( argument.empty(  ) )
   {
      ch->print( "Usage: showdeity <deity>\r\n" );
      return;
   }
   if( !( deity = get_deity( argument ) ) )
   {
      ch->print( "No such deity.\r\n" );
      return;
   }
   ch->print_fmt( "\r\nDeity: {:<11} Filename: {} \r\n", deity->name, deity->filename );
   ch->print_fmt( "Description:\r\n {} \r\n", deity->deitydesc );
   ch->print_fmt( "Alignment:       {:<14}  Sex:   {:<14} \r\n", deity->alignment, deity->sex == -1 ? "none" : npc_sex[deity->sex] );
   ch->print_fmt( "Races Allowed:   {}\r\n", deity->race_allowed.none(  ) ? "All" : bitset_string( deity->race_allowed, npc_race ) );
   ch->print_fmt( "Classes Allowed: {}\r\n", deity->class_allowed.none(  ) ? "All" : bitset_string( deity->class_allowed, npc_class ) );

   ch->print_fmt( "Npcraces:        {:<14}", ( deity->npcrace[0] < 0 || deity->npcrace[0] > MAX_NPC_RACE ) ? "none" : npc_race[deity->npcrace[0]] );
   ch->print_fmt( "  {:<14}", ( deity->npcrace[1] < 0 || deity->npcrace[1] > MAX_NPC_RACE ) ? "none" : npc_race[deity->npcrace[1]] );
   ch->print_fmt( "  {:<14}\r\n", ( deity->npcrace[2] < 0 || deity->npcrace[2] > MAX_NPC_RACE ) ? "none" : npc_race[deity->npcrace[2]] );

   ch->print_fmt( "Npcfoes:         {:<14}", ( deity->npcfoe[0] < 0 || deity->npcfoe[0] > MAX_NPC_RACE ) ? "none" : npc_race[deity->npcfoe[0]] );
   ch->print_fmt( "  {:<14}", ( deity->npcfoe[1] < 0 || deity->npcfoe[1] > MAX_NPC_RACE ) ? "none" : npc_race[deity->npcfoe[1]] );
   ch->print_fmt( "  {:<14}\r\n", ( deity->npcfoe[2] < 0 || deity->npcfoe[2] > MAX_NPC_RACE ) ? "none" : npc_race[deity->npcfoe[2]] );

   ch->print_fmt( "Affects:         {:<14}", aff_flags[deity->affected[0]] );
   ch->print_fmt( "  {:<14}", aff_flags[deity->affected[1]] );
   ch->print_fmt( "  {:<14}\r\n", aff_flags[deity->affected[2]] );

   ch->print_fmt( "Elements:      {:<14}", ris_flags[deity->element[0]] );
   ch->print_fmt( "  {:<14}", ris_flags[deity->element[1]] );
   ch->print_fmt( "  {:<14}\r\n", ris_flags[deity->element[2]] );

   ch->print_fmt( "Suscepts:      {:<14}", ris_flags[deity->suscept[0]] );
   ch->print_fmt( "  {:<14}", ris_flags[deity->suscept[1]] );
   ch->print_fmt( "  {:<14}\r\n", ris_flags[deity->suscept[2]] );

   ch->print_fmt( "Spells:        {:<14}  {:<14}  {:<14}\r\n\r\n", skill_table[deity->spell[0]]->name, skill_table[deity->spell[1]]->name, skill_table[deity->spell[2]]->name );
   ch->print_fmt( "Affectednums:  {:<5} {:<5} {:<7} ", deity->affectednum[0], deity->affectednum[1], deity->affectednum[2] );
   ch->print_fmt( "Elementnums:   {:<5} {:<5} {:<5}\r\n", deity->elementnum[0], deity->elementnum[1], deity->elementnum[2] );
   ch->print_fmt( "Susceptnums:   {:<5} {:<5} {:<7} ", deity->susceptnum[0], deity->susceptnum[1], deity->susceptnum[2] );
   ch->print_fmt( "Sspells:       {:<5} {:<5} {:<5}\r\n", deity->sspell[0], deity->sspell[1], deity->sspell[2] );
   ch->print_fmt( "Flee_npcraces: {:<5} {:<5} {:<7} ", deity->flee_npcrace[0], deity->flee_npcrace[1], deity->flee_npcrace[2] );
   ch->print_fmt( "Flee_npcfoes:  {:<5} {:<5} {:<5} \r\n", deity->flee_npcfoe[0], deity->flee_npcfoe[1], deity->flee_npcfoe[2] );
   ch->print_fmt( "Kill_npcraces: {:<5} {:<5} {:<7} ", deity->kill_npcrace[0], deity->kill_npcrace[1], deity->kill_npcrace[2] );
   ch->print_fmt( "Kill_npcfoes:  {:<5} {:<5} {:<5} \r\n", deity->kill_npcfoe[0], deity->kill_npcfoe[1], deity->kill_npcfoe[2] );
   ch->print_fmt( "Die_npcraces:  {:<5} {:<5} {:<7} ", deity->die_npcrace[0], deity->die_npcrace[1], deity->die_npcrace[2] );
   ch->print_fmt( "Die_npcfoes:   {:<5} {:<5} {:<5} \r\n\r\n", deity->die_npcfoe[0], deity->die_npcfoe[1], deity->die_npcfoe[2] );
   ch->print_fmt( "Kill_magic: {:<10} Sac:        {:<10} Bury_corpse: {:<10} \r\n", deity->kill_magic, deity->sac, deity->bury_corpse );
   ch->print_fmt( "Dig_corpse: {:<10} Flee:       {:<10} Kill:        {:<10} \r\n", deity->flee, deity->kill, deity->dig_corpse );
   ch->print_fmt( "Die:        {:<10} Steal:      {:<10} Backstab:    {:<10} \r\n", deity->die, deity->steal, deity->backstab );
   ch->print_fmt( "Aid:        {:<10} Aid_spell:  {:<10} Spell_aid:   {:<10} \r\n", deity->aid, deity->aid_spell, deity->spell_aid );
   ch->print_fmt( "Object:     {:<10} Object2:    {:<10} Avatar:      {:<10} \r\n", deity->deityobj, deity->deityobj2, deity->avatar );
   ch->print_fmt( "Mount:      {:<10} Minion:     {:<10} Scorpse:     {:<10} \r\n", deity->mount, deity->minion, deity->scorpse );
   ch->print_fmt( "Savatar:    {:<10} Smount:     {:<10} Sminion:     {:<10} \r\n", deity->savatar, deity->smount, deity->sminion );
   ch->print_fmt( "Sdeityobj:  {:<10} Sdeityobj2: {:<10} Srecall:     {:<10} \r\n", deity->sdeityobj, deity->sdeityobj, deity->srecall );
   ch->print_fmt( "Recallroom: {:<10} Objstat:    {:<10} Worshippers: {:<10} \r\n", deity->recallroom, deity->objstat, deity->worshippers );
}

CMDF( do_makedeity )
{
   deity_data *deity;

   if( argument.empty(  ) )
   {
      ch->print( "Usage: makedeity <deity name>\r\n" );
      return;
   }

   smash_tilde( argument );

   if( ( deity = get_deity( argument ) ) )
   {
      ch->print( "A deity with that name already holds weight on this world.\r\n" );
      return;
   }

   deity = new deity_data;
   deitylist.push_back( deity );
   deity->name = argument;
   strlower( argument );
   deity->filename = argument;
   write_deity_list(  );
   save_deity( deity );
   ch->print_fmt( "{} deity has been created.\r\n", argument );
}

CMDF( do_devote )
{
   deity_data *deity;

   if( ch->isnpc(  ) )
   {
      ch->print( "Huh?\r\n" );
      return;
   }

   if( argument.empty(  ) )
   {
      ch->print( "Devote yourself to which deity?\r\n" );
      return;
   }

   if( !str_cmp( argument, "none" ) )
   {
      affect_data af;

      if( !ch->pcdata->deity )
      {
         ch->print( "You have already chosen to worship no deities.\r\n" );
         return;
      }
      --ch->pcdata->deity->worshippers;
      if( ch->pcdata->deity->worshippers < 0 )
         ch->pcdata->deity->worshippers = 0;
      ch->pcdata->favor = -2500;
      ch->worsen_mental_state( 50 );
      ch->print( "A terrible curse afflicts you as you forsake a deity!\r\n" );
      ch->unset_aflag( ch->pcdata->deity->affected[0] );
      ch->unset_aflag( ch->pcdata->deity->affected[1] );
      ch->unset_aflag( ch->pcdata->deity->affected[2] );
      ch->unset_resist( ch->pcdata->deity->element[0] );
      ch->unset_resist( ch->pcdata->deity->element[1] );
      ch->unset_resist( ch->pcdata->deity->element[2] );
      ch->unset_suscep( ch->pcdata->deity->suscept[0] );
      ch->unset_suscep( ch->pcdata->deity->suscept[1] );
      ch->unset_suscep( ch->pcdata->deity->suscept[2] );
      ch->affect_strip( gsn_blindness );
      af.type = gsn_blindness;
      af.location = APPLY_HITROLL;
      af.modifier = -4;
      af.duration = ( int )( 50 * DUR_CONV );
      af.bit = AFF_BLIND;
      ch->affect_to_char( &af );
      save_deity( ch->pcdata->deity );
      ch->print( "You cease to worship any deity.\r\n" );
      ch->pcdata->deity = nullptr;
      ch->pcdata->deity_name.clear(  );
      ch->save(  );
      return;
   }

   if( !( deity = get_deity( argument ) ) )
   {
      ch->print( "No such deity holds weight on this world.\r\n" );
      return;
   }

   if( ch->pcdata->deity )
   {
      ch->print( "You are already devoted to a deity.\r\n" );
      return;
   }

   /*
    * Edited by Tarl 25 Feb 02 
    */
   /*
    * Edited again by Samson 5-17-04 
    */
   if( deity->class_allowed.any(  ) && !deity->class_allowed.test( ch->Class ) )
   {
      ch->print_fmt( "{} does not accept worshippers of class {}.\r\n", deity->name, class_table[ch->Class]->who_name );
      return;
   }

   if( deity->sex != -1 && deity->sex != ch->sex )
   {
      ch->print_fmt( "{} does not accept {} worshippers.\r\n", deity->name, npc_sex[ch->sex] );
      return;
   }

   /*
    * Edited by Tarl 25 Feb 02 
    */
   /*
    * Edited again by Samson 5-17-04 
    */
   if( deity->race_allowed.any(  ) && !deity->race_allowed.test( ch->race ) )
   {
      ch->print_fmt( "{} does not accept worshippers of the {} race.\r\n", deity->name, race_table[ch->race]->race_name );
      return;
   }

   ch->pcdata->deity_name = deity->name;
   ch->pcdata->deity = deity;

   act( AT_MAGIC, "Body and soul, you devote yourself to $t!", ch, ch->pcdata->deity_name.c_str(  ), nullptr, TO_CHAR );
   ++ch->pcdata->deity->worshippers;
   save_deity( ch->pcdata->deity );
   ch->save(  );
}

CMDF( do_deities )
{
   deity_data *deity;
   int count = 0;

   if( argument.empty(  ) )
   {
      std::list<deity_data *>::iterator dt;

      ch->pager( "&gFor detailed information on a deity, try 'deities <deity>' or 'help deities'\r\n" );
      ch->pager( "Deity           Worshippers\r\n" );
      for( dt = deitylist.begin(  ); dt != deitylist.end(  ); ++dt )
      {
         deity = *dt;

         ch->pager_fmt( "&G{:<14}   &g{:19}\r\n", deity->name, deity->worshippers );
         ++count;
      }
      if( !count )
      {
         ch->pager( "&gThere are no deities on this world.\r\n" );
         return;
      }
      return;
   }

   if( !( deity = get_deity( argument ) ) )
   {
      ch->print( "&gThat deity does not exist.\r\n" );
      return;
   }

   ch->pager_fmt( "&gDeity:        &G{}\r\n", deity->name );
   ch->pager_fmt( "&gDescription:\r\n&G%s", deity->deitydesc.c_str(  ) );
}

/*
Internal function to adjust favor.
Fields are:
0 = flee            5 = sac         10 = backstab
1 = flee_npcrace    6 = bury_corpse 11 = die
2 = kill            7 = aid_spell   12 = die_npcrace
3 = kill_npcrace    8 = aid         13 = spell_aid
4 = kill_magic      9 = steal       14 = dig_corpse
15 = die_npcfoe    16 = flee_npcfoe 17 = kill_npcfoe
*/
void char_data::adjust_favor( int field, int mod )
{
   if( isnpc(  ) || !pcdata->deity )
      return;

   int oldfavor = pcdata->favor;

   if( ( alignment - pcdata->deity->alignment > 650 || alignment - pcdata->deity->alignment < -650 ) && pcdata->deity->alignment != 0 )
   {
      pcdata->favor -= 2;
      pcdata->favor = urange( -2500, pcdata->favor, 5000 );

      if( pcdata->favor > pcdata->deity->affectednum[0] )
      {
         if( pcdata->deity->affected[0] > 0 && !affected_by.test( pcdata->deity->affected[0] ) )
         {
            print_fmt( "{} looks favorably upon you and bestows {} as a reward.\r\n", pcdata->deity_name, aff_flags[pcdata->deity->affected[0]] );
            affected_by.set( pcdata->deity->affected[0] );
         }
      }
      if( pcdata->favor > pcdata->deity->affectednum[1] )
      {
         if( pcdata->deity->affected[1] > 0 && !affected_by.test( pcdata->deity->affected[1] ) )
         {
            print_fmt( "{} looks favorably upon you and bestows {} as a reward.\r\n", pcdata->deity_name, aff_flags[pcdata->deity->affected[1]] );
            affected_by.set( pcdata->deity->affected[1] );
         }
      }
      if( pcdata->favor > pcdata->deity->affectednum[2] )
      {
         if( pcdata->deity->affected[2] > 0 && !affected_by.test( pcdata->deity->affected[2] ) )
         {
            print_fmt( "{} looks favorably upon you and bestows {} as a reward.\r\n", pcdata->deity_name, aff_flags[pcdata->deity->affected[2]] );
            affected_by.set( pcdata->deity->affected[2] );
         }
      }
      if( pcdata->favor > pcdata->deity->elementnum[0] )
      {
         if( pcdata->deity->element[0] != 0 && !has_resist( pcdata->deity->element[0] ) )
         {
            set_resist( pcdata->deity->element[0] );
            print_fmt( "{} looks favorably upon you and bestows {} resistance upon you.\r\n", pcdata->deity_name, ris_flags[pcdata->deity->element[0]] );
         }
      }
      if( pcdata->favor > pcdata->deity->elementnum[1] )
      {
         if( pcdata->deity->element[1] != 0 && !has_resist( pcdata->deity->element[1] ) )
         {
            set_resist( pcdata->deity->element[1] );
            print_fmt( "{} looks favorably upon you and bestows {} resistance upon you.\r\n", pcdata->deity_name, ris_flags[pcdata->deity->element[1]] );
         }
      }
      if( pcdata->favor > pcdata->deity->elementnum[2] )
      {
         if( pcdata->deity->element[2] != 0 && !has_resist( pcdata->deity->element[2] ) )
         {
            set_resist( pcdata->deity->element[2] );
            print_fmt( "{} looks favorably upon you and bestows {} resistance upon you.\r\n", pcdata->deity_name, ris_flags[pcdata->deity->element[2]] );
         }
      }
      if( pcdata->favor < pcdata->deity->susceptnum[0] )
      {
         if( pcdata->deity->suscept[0] != 0 && !has_suscep( pcdata->deity->suscept[0] ) )
         {
            set_suscep( pcdata->deity->suscept[0] );
            print_fmt( "{} looks poorly upon you and makes you more vulnerable to {} as punishment.\r\n", pcdata->deity_name, ris_flags[pcdata->deity->suscept[0]] );
         }
      }
      if( pcdata->favor < pcdata->deity->susceptnum[1] )
      {
         if( pcdata->deity->suscept[1] != 0 && !has_suscep( pcdata->deity->suscept[1] ) )
         {
            set_suscep( pcdata->deity->suscept[1] );
            print_fmt( "{} looks poorly upon you and makes you more vulnerable to {} as punishment.\r\n", pcdata->deity_name, ris_flags[pcdata->deity->suscept[1]] );
         }
      }
      if( pcdata->favor < pcdata->deity->susceptnum[2] )
      {
         if( pcdata->deity->suscept[2] != 0 && !has_suscep( pcdata->deity->suscept[2] ) )
         {
            set_suscep( pcdata->deity->suscept[2] );
            print_fmt( "{} looks poorly upon you and makes you more vulnerable to {} as punishment.\r\n", pcdata->deity_name, ris_flags[pcdata->deity->suscept[2]] );
         }
      }

      if( ( oldfavor > pcdata->deity->affectednum[0] && pcdata->favor <= pcdata->deity->affectednum[0] )
          || ( oldfavor > pcdata->deity->affectednum[1] && pcdata->favor <= pcdata->deity->affectednum[1] )
          || ( oldfavor > pcdata->deity->affectednum[2] && pcdata->favor <= pcdata->deity->affectednum[2] )
          || ( oldfavor > pcdata->deity->elementnum[0] && pcdata->favor <= pcdata->deity->elementnum[0] )
          || ( oldfavor > pcdata->deity->elementnum[1] && pcdata->favor <= pcdata->deity->elementnum[1] )
          || ( oldfavor > pcdata->deity->elementnum[2] && pcdata->favor <= pcdata->deity->elementnum[2] )
          || ( oldfavor < pcdata->deity->susceptnum[0] && pcdata->favor >= pcdata->deity->susceptnum[0] )
          || ( oldfavor < pcdata->deity->susceptnum[1] && pcdata->favor >= pcdata->deity->susceptnum[1] )
          || ( oldfavor < pcdata->deity->susceptnum[2] && pcdata->favor >= pcdata->deity->susceptnum[2] ) )
      {
         update_aris(  );
      }

      /*
       * Added by Tarl 24 Mar 02 
       */
      if( ( oldfavor > pcdata->deity->sspell[0] ) && ( pcdata->favor <= pcdata->deity->sspell[0] ) )
      {
         if( level < skill_table[pcdata->deity->spell[0]]->skill_level[Class] && level < skill_table[pcdata->deity->spell[0]]->race_level[race] )
         {
            pcdata->learned[pcdata->deity->spell[0]] = 0;
         }
      }

      if( ( oldfavor > pcdata->deity->sspell[1] ) && ( pcdata->favor <= pcdata->deity->sspell[1] ) )
      {
         if( level < skill_table[pcdata->deity->spell[1]]->skill_level[Class] && level < skill_table[pcdata->deity->spell[1]]->race_level[race] )
         {
            pcdata->learned[pcdata->deity->spell[1]] = 0;
         }
      }

      if( ( oldfavor > pcdata->deity->sspell[2] ) && ( pcdata->favor <= pcdata->deity->sspell[2] ) )
      {
         if( level < skill_table[pcdata->deity->spell[2]]->skill_level[Class] && level < skill_table[pcdata->deity->spell[2]]->race_level[race] )
         {
            pcdata->learned[pcdata->deity->spell[2]] = 0;
         }
      }
      return;
   }

   if( mod < 1 )
      mod = 1;
   switch ( field )
   {
      default:
         break;

      case 0:
         pcdata->favor += number_fuzzy( pcdata->deity->flee / mod );
         break;
      case 1:
         pcdata->favor += number_fuzzy( pcdata->deity->flee_npcrace[0] / mod );
         break;
      case 2:
         pcdata->favor += number_fuzzy( pcdata->deity->kill / mod );
         break;
      case 3:
         pcdata->favor += number_fuzzy( pcdata->deity->kill_npcrace[0] / mod );
         break;
      case 4:
         pcdata->favor += number_fuzzy( pcdata->deity->kill_magic / mod );
         break;
      case 5:
         pcdata->favor += number_fuzzy( pcdata->deity->sac / mod );
         break;
      case 6:
         pcdata->favor += number_fuzzy( pcdata->deity->bury_corpse / mod );
         break;
      case 7:
         pcdata->favor += number_fuzzy( pcdata->deity->aid_spell / mod );
         break;
      case 8:
         pcdata->favor += number_fuzzy( pcdata->deity->aid / mod );
         break;
      case 9:
         pcdata->favor += number_fuzzy( pcdata->deity->steal / mod );
         break;
      case 10:
         pcdata->favor += number_fuzzy( pcdata->deity->backstab / mod );
         break;
      case 11:
         pcdata->favor += number_fuzzy( pcdata->deity->die / mod );
         break;
      case 12:
         pcdata->favor += number_fuzzy( pcdata->deity->die_npcrace[0] / mod );
         break;
      case 13:
         pcdata->favor += number_fuzzy( pcdata->deity->spell_aid / mod );
         break;
      case 14:
         pcdata->favor += number_fuzzy( pcdata->deity->dig_corpse / mod );
         break;
      case 15:
         pcdata->favor += number_fuzzy( pcdata->deity->die_npcfoe[0] / mod );
         break;
      case 16:
         pcdata->favor += number_fuzzy( pcdata->deity->flee_npcfoe[0] / mod );
         break;
      case 17:
         pcdata->favor += number_fuzzy( pcdata->deity->kill_npcfoe[0] / mod );
         break;
      case 18:
         pcdata->favor += number_fuzzy( pcdata->deity->flee_npcrace[1] / mod );
         break;
      case 19:
         pcdata->favor += number_fuzzy( pcdata->deity->flee_npcrace[2] / mod );
         break;
      case 20:
         pcdata->favor += number_fuzzy( pcdata->deity->flee_npcfoe[1] / mod );
         break;
      case 21:
         pcdata->favor += number_fuzzy( pcdata->deity->flee_npcfoe[2] / mod );
         break;
      case 22:
         pcdata->favor += number_fuzzy( pcdata->deity->kill_npcrace[1] / mod );
         break;
      case 23:
         pcdata->favor += number_fuzzy( pcdata->deity->kill_npcrace[2] / mod );
         break;
      case 24:
         pcdata->favor += number_fuzzy( pcdata->deity->kill_npcfoe[1] / mod );
         break;
      case 25:
         pcdata->favor += number_fuzzy( pcdata->deity->kill_npcfoe[2] / mod );
         break;
      case 26:
         pcdata->favor += number_fuzzy( pcdata->deity->die_npcrace[1] / mod );
         break;
      case 27:
         pcdata->favor += number_fuzzy( pcdata->deity->die_npcrace[2] / mod );
         break;
      case 28:
         pcdata->favor += number_fuzzy( pcdata->deity->die_npcfoe[1] / mod );
         break;
      case 29:
         pcdata->favor += number_fuzzy( pcdata->deity->die_npcfoe[2] / mod );
         break;
   }
   pcdata->favor = urange( -2500, pcdata->favor, 5000 );

   if( pcdata->favor > pcdata->deity->affectednum[0] )
   {
      if( pcdata->deity->affected[0] > 0 && !affected_by.test( pcdata->deity->affected[0] ) )
      {
         print_fmt( "{} looks favorably upon you and bestows {} as a reward.\r\n", pcdata->deity_name, aff_flags[pcdata->deity->affected[0]] );
         affected_by.set( pcdata->deity->affected[0] );
      }
   }
   if( pcdata->favor > pcdata->deity->affectednum[1] )
   {
      if( pcdata->deity->affected[1] > 0 && !affected_by.test( pcdata->deity->affected[1] ) )
      {
         print_fmt( "{} looks favorably upon you and bestows {} as a reward.\r\n", pcdata->deity_name, aff_flags[pcdata->deity->affected[1]] );
         affected_by.set( pcdata->deity->affected[1] );
      }
   }
   if( pcdata->favor > pcdata->deity->affectednum[2] )
   {
      if( pcdata->deity->affected[2] > 0 && !affected_by.test( pcdata->deity->affected[2] ) )
      {
         print_fmt( "{} looks favorably upon you and bestows {} as a reward.\r\n", pcdata->deity_name, aff_flags[pcdata->deity->affected[2]] );
         affected_by.set( pcdata->deity->affected[2] );
      }
   }
   if( pcdata->favor > pcdata->deity->elementnum[0] )
   {
      if( pcdata->deity->element[0] != 0 && !has_resist( pcdata->deity->element[0] ) )
      {
         set_resist( pcdata->deity->element[0] );
         print_fmt( "{} looks favorably upon you and bestows {} resistance upon you.\r\n", pcdata->deity_name, ris_flags[pcdata->deity->element[0]] );
      }
   }
   if( pcdata->favor > pcdata->deity->elementnum[1] )
   {
      if( pcdata->deity->element[1] != 0 && !has_resist( pcdata->deity->element[1] ) )
      {
         set_resist( pcdata->deity->element[1] );
         print_fmt( "{} looks favorably upon you and bestows {} resistance upon you.\r\n", pcdata->deity_name, ris_flags[pcdata->deity->element[1]] );
      }
   }
   if( pcdata->favor > pcdata->deity->elementnum[2] )
   {
      if( pcdata->deity->element[2] != 0 && !has_resist( pcdata->deity->element[2] ) )
      {
         set_resist( pcdata->deity->element[2] );
         print_fmt( "{} looks favorably upon you and bestows {} resistance upon you.\r\n", pcdata->deity_name, ris_flags[pcdata->deity->element[2]] );
      }
   }
   if( pcdata->favor < pcdata->deity->susceptnum[0] )
   {
      if( pcdata->deity->suscept[0] != 0 && !has_suscep( pcdata->deity->suscept[0] ) )
      {
         set_suscep( pcdata->deity->suscept[0] );
         print_fmt( "{} looks poorly upon you and makes you more vulnerable to {} as punishment.\r\n", pcdata->deity_name, ris_flags[pcdata->deity->suscept[0]] );
      }
   }
   if( pcdata->favor < pcdata->deity->susceptnum[1] )
   {
      if( pcdata->deity->suscept[1] != 0 && !has_suscep( pcdata->deity->suscept[1] ) )
      {
         set_suscep( pcdata->deity->suscept[1] );
         print_fmt( "{} looks poorly upon you and makes you more vulnerable to {} as punishment.\r\n", pcdata->deity_name, ris_flags[pcdata->deity->suscept[1]] );
      }
   }
   if( pcdata->favor < pcdata->deity->susceptnum[2] )
   {
      if( pcdata->deity->suscept[2] != 0 && !has_suscep( pcdata->deity->suscept[2] ) )
      {
         set_suscep( pcdata->deity->suscept[2] );
         print_fmt( "{} looks poorly upon you and makes you more vulnerable to {} as punishment.\r\n", pcdata->deity_name, ris_flags[pcdata->deity->suscept[2]] );
      }
   }
   /*
    * Added by Tarl 24 Mar 02 
    */
   if( pcdata->favor > pcdata->deity->sspell[0] )
   {
      if( pcdata->deity->spell[0] != 0 )
      {
         if( pcdata->learned[pcdata->deity->spell[0]] != 0 )
            return;
         pcdata->learned[pcdata->deity->spell[0]] = 95;
      }
   }

   if( pcdata->favor > pcdata->deity->sspell[1] )
   {
      if( pcdata->deity->spell[1] != 0 )
      {
         if( pcdata->learned[pcdata->deity->spell[1]] != 0 )
            return;
         pcdata->learned[pcdata->deity->spell[1]] = 95;
      }
   }

   if( pcdata->favor > pcdata->deity->sspell[2] )
   {
      if( pcdata->deity->spell[2] != 0 )
      {
         if( pcdata->learned[pcdata->deity->spell[2]] != 0 )
            return;
         pcdata->learned[pcdata->deity->spell[2]] = 95;
      }
   }

   /*
    * If favor crosses over the line then strip the affect 
    */
   if( ( oldfavor > pcdata->deity->affectednum[0] && pcdata->favor <= pcdata->deity->affectednum[0] )
       || ( oldfavor > pcdata->deity->affectednum[1] && pcdata->favor <= pcdata->deity->affectednum[1] )
       || ( oldfavor > pcdata->deity->affectednum[2] && pcdata->favor <= pcdata->deity->affectednum[2] )
       || ( oldfavor > pcdata->deity->elementnum[0] && pcdata->favor <= pcdata->deity->elementnum[0] )
       || ( oldfavor > pcdata->deity->elementnum[1] && pcdata->favor <= pcdata->deity->elementnum[1] )
       || ( oldfavor > pcdata->deity->elementnum[2] && pcdata->favor <= pcdata->deity->elementnum[2] )
       || ( oldfavor < pcdata->deity->susceptnum[0] && pcdata->favor >= pcdata->deity->susceptnum[0] )
       || ( oldfavor < pcdata->deity->susceptnum[1] && pcdata->favor >= pcdata->deity->susceptnum[1] )
       || ( oldfavor < pcdata->deity->susceptnum[2] && pcdata->favor >= pcdata->deity->susceptnum[2] ) )
   {
      update_aris(  );
   }

   /*
    * Added by Tarl 24 Mar 02 
    */
   if( ( oldfavor > pcdata->deity->sspell[0] ) && ( pcdata->favor <= pcdata->deity->sspell[0] ) )
   {
      if( level < skill_table[pcdata->deity->spell[0]]->skill_level[Class] && level < skill_table[pcdata->deity->spell[0]]->race_level[race] )
      {
         pcdata->learned[pcdata->deity->spell[0]] = 0;
      }
   }

   if( ( oldfavor > pcdata->deity->sspell[1] ) && ( pcdata->favor <= pcdata->deity->sspell[1] ) )
   {
      if( level < skill_table[pcdata->deity->spell[1]]->skill_level[Class] && level < skill_table[pcdata->deity->spell[1]]->race_level[race] )
      {
         pcdata->learned[pcdata->deity->spell[1]] = 0;
      }
   }

   if( ( oldfavor > pcdata->deity->sspell[2] ) && ( pcdata->favor <= pcdata->deity->sspell[2] ) )
   {
      if( level < skill_table[pcdata->deity->spell[2]]->skill_level[Class] && level < skill_table[pcdata->deity->spell[2]]->race_level[race] )
      {
         pcdata->learned[pcdata->deity->spell[2]] = 0;
      }
   }
}

CMDF( do_supplicate )
{
   if( ch->isnpc(  ) || !ch->pcdata->deity )
   {
      ch->print( "You have no deity to supplicate to.\r\n" );
      return;
   }

   if( argument.empty(  ) )
   {
      ch->print( "Supplicate for what?\r\n" );
      return;
   }

   if( !str_cmp( argument, "corpse" ) )
   {
      std::string buf2 = "the corpse of ";
      buf2.append( ch->name );

      if( ch->pcdata->favor < ch->pcdata->deity->scorpse )
      {
         ch->print( "You are not favored enough for a corpse retrieval.\r\n" );
         return;
      }

      if( ch->in_room->flags.test( ROOM_CLANSTOREROOM ) )
      {
         ch->print( "You cannot supplicate in a storage room.\r\n" );
         return;
      }

      bool found = false;
      for( auto* obj : objlist )
      {
         if( obj->in_room && !str_cmp( buf2, obj->short_descr ) && ( obj->pIndexData->vnum == OBJ_VNUM_CORPSE_PC ) )
         {
            found = true;
            if( obj->in_room->flags.test( ROOM_NOSUPPLICATE ) )
            {
               act( AT_MAGIC, "The image of your corpse appears, but suddenly wavers away.", ch, nullptr, nullptr, TO_CHAR );
               return;
            }
            act( AT_MAGIC, "Your corpse appears suddenly, surrounded by a divine presence...", ch, nullptr, nullptr, TO_CHAR );
            act( AT_MAGIC, "$n's corpse appears suddenly, surrounded by a divine force...", ch, nullptr, nullptr, TO_ROOM );
            obj->from_room(  );
            obj = obj->to_room( ch->in_room, ch );
            obj->extra_flags.reset( ITEM_BURIED );
         }
      }

      if( !found )
      {
         ch->print( "No corpse of yours litters the world...\r\n" );
         return;
      }
      ch->pcdata->favor -= ch->pcdata->deity->scorpse;
      ch->adjust_favor( -1, 1 );
      return;
   }

   if( !str_cmp( argument, "avatar" ) )
   {
      mob_index *pMobIndex;
      char_data *victim;

      if( ch->pcdata->favor < ch->pcdata->deity->savatar )
      {
         ch->print( "You are not favored enough for that.\r\n" );
         return;
      }

      if( !( pMobIndex = get_mob_index( ch->pcdata->deity->avatar ) ) )
      {
         ch->print( "Your deity has no avatar to pray for.\r\n" );
         return;
      }

      if( check_pets( ch, pMobIndex ) )
      {
         ch->print( "You have already received an avatar of your deity!\r\n" );
         return;
      }

      victim = pMobIndex->create_mobile(  );
      if( !victim->to_room( ch->in_room ) )
         log_printf( "char_to_room: {}:{}, line {}.", __FILE__, __func__, __LINE__ );
      fix_maps( ch, victim );

      act( AT_MAGIC, "$n summons a powerful avatar!", ch, nullptr, nullptr, TO_ROOM );
      act( AT_MAGIC, "You summon a powerful avatar!", ch, nullptr, nullptr, TO_CHAR );
      bind_follower( victim, ch, gsn_charm_person, ( ch->pcdata->favor / 4 ) + 1 );
      victim->level = LEVEL_AVATAR / 2;
      victim->hit = ch->hit + ch->pcdata->favor;
      victim->alignment = ch->pcdata->deity->alignment;
      victim->max_hit = ch->hit + ch->pcdata->favor;

      ch->pcdata->favor -= ch->pcdata->deity->savatar;
      ch->adjust_favor( -1, 1 );
      return;
   }

   /*
    * Added by Tarl 24 Feb 02 
    */
   if( !str_cmp( argument, "mount" ) )
   {
      mob_index *pMobIndex;
      char_data *victim;

      if( ch->pcdata->favor < ch->pcdata->deity->smount )
      {
         ch->print( "You are not favored enough for that.\r\n" );
         return;
      }

      if( !( pMobIndex = get_mob_index( ch->pcdata->deity->mount ) ) )
      {
         ch->print( "Your deity has no mount to pray for.\r\n" );
         return;
      }

      if( check_pets( ch, pMobIndex ) )
      {
         ch->print( "You have already received a mount of your deity!\r\n" );
         return;
      }

      victim = pMobIndex->create_mobile(  );
      if( !victim->to_room( ch->in_room ) )
         log_printf( "char_to_room: {}:{}, line {}.", __FILE__, __func__, __LINE__ );
      fix_maps( ch, victim );

      act( AT_MAGIC, "$n summons a mount!", ch, nullptr, nullptr, TO_ROOM );
      act( AT_MAGIC, "You summon a mount!", ch, nullptr, nullptr, TO_CHAR );
      bind_follower( victim, ch, gsn_charm_person, ( ch->pcdata->favor / 4 ) + 1 );
      ch->pcdata->favor -= ch->pcdata->deity->smount;
      ch->adjust_favor( -1, 1 );
      return;
   }

   /*
    * Added by Tarl 24 Feb 02 
    */
   if( !str_cmp( argument, "minion" ) )
   {
      mob_index *pMobIndex;
      char_data *victim;

      if( ch->pcdata->favor < ch->pcdata->deity->sminion )
      {
         ch->print( "You are not favored enough for that.\r\n" );
         return;
      }

      if( !( pMobIndex = get_mob_index( ch->pcdata->deity->minion ) ) )
      {
         ch->print( "Your deity has no minion to pray for.\r\n" );
         return;
      }

      if( check_pets( ch, pMobIndex ) )
      {
         ch->print( "You have already received a minion of your deity!\r\n" );
         return;
      }

      victim = pMobIndex->create_mobile(  );
      if( !victim->to_room( ch->in_room ) )
         log_printf( "char_to_room: {}:{}, line {}.", __FILE__, __func__, __LINE__ );
      fix_maps( ch, victim );

      act( AT_MAGIC, "$n summons a minion!", ch, nullptr, nullptr, TO_ROOM );
      act( AT_MAGIC, "You summon a minion!", ch, nullptr, nullptr, TO_CHAR );
      bind_follower( victim, ch, gsn_charm_person, ( ch->pcdata->favor / 4 ) + 1 );
      victim->level = LEVEL_AVATAR / 3;
      victim->hit = ( ch->hit / 2 ) + ( ch->pcdata->favor / 4 );
      victim->alignment = ch->pcdata->deity->alignment;
      victim->max_hit = ( ch->hit / 2 ) + ( ch->pcdata->favor / 4 );

      ch->pcdata->favor -= ch->pcdata->deity->sminion;
      ch->adjust_favor( -1, 1 );
      return;
   }

   if( !str_cmp( argument, "object" ) )
   {
      obj_data *obj;
      obj_index *pObjIndex;
      affect_data *paf;

      if( ch->pcdata->favor < ch->pcdata->deity->sdeityobj )
      {
         ch->print( "You are not favored enough for that.\r\n" );
         return;
      }

      if( !( pObjIndex = get_obj_index( ch->pcdata->deity->deityobj ) ) )
      {
         ch->print( "Your deity has no object to pray for.\r\n" );
         return;
      }

      if( !( obj = pObjIndex->create_object( ch->level ) ) )
      {
         log_printf( "create_object: {}:{}, line {}.", __FILE__, __func__, __LINE__ );
         return;
      }
      if( obj->wear_flags.test( ITEM_TAKE ) )
         obj = obj->to_char( ch );
      else
         obj = obj->to_room( ch->in_room, ch );

      act( AT_MAGIC, "$n weaves $p from divine matter!", ch, obj, nullptr, TO_ROOM );
      act( AT_MAGIC, "You weave $p from divine matter!", ch, obj, nullptr, TO_CHAR );
      ch->pcdata->favor -= ch->pcdata->deity->sdeityobj;
      ch->adjust_favor( -1, 1 );

      paf = new affect_data;
      paf->type = -1;
      paf->duration = -1;
      paf->rismod.reset(  );

      switch ( ch->pcdata->deity->objstat )
      {
         default:
            paf->location = APPLY_NONE;
            break;
         case 0:
            paf->location = APPLY_STR;
            break;
         case 1:
            paf->location = APPLY_INT;
            break;
         case 2:
            paf->location = APPLY_WIS;
            break;
         case 3:
            paf->location = APPLY_CON;
            break;
         case 4:
            paf->location = APPLY_DEX;
            break;
         case 5:
            paf->location = APPLY_CHA;
            break;
         case 6:
            paf->location = APPLY_LCK;
            break;
      }
      paf->modifier = 1;
      paf->bit = 0;
      obj->affects.push_back( paf );
      return;
   }

   if( !str_cmp( argument, "object2" ) )
   {
      obj_data *obj;
      obj_index *pObjIndex;
      affect_data *paf;

      if( ch->pcdata->favor < ch->pcdata->deity->sdeityobj2 )
      {
         ch->print( "You are not favored enough for that.\r\n" );
         return;
      }

      if( !( pObjIndex = get_obj_index( ch->pcdata->deity->deityobj2 ) ) )
      {
         ch->print( "Your deity has no object to pray for.\r\n" );
         return;
      }

      if( !( obj = pObjIndex->create_object( ch->level ) ) )
      {
         log_printf( "create_object: {}:{}, line {}.", __FILE__, __func__, __LINE__ );
         return;
      }
      if( obj->wear_flags.test( ITEM_TAKE ) )
         obj = obj->to_char( ch );
      else
         obj = obj->to_room( ch->in_room, ch );

      obj->name = std::format( "sigil {}", ch->pcdata->deity->name );

      act( AT_MAGIC, "$n weaves $p from divine matter!", ch, obj, nullptr, TO_ROOM );
      act( AT_MAGIC, "You weave $p from divine matter!", ch, obj, nullptr, TO_CHAR );
      ch->pcdata->favor -= ch->pcdata->deity->sdeityobj2;
      ch->adjust_favor( -1, 1 );

      paf = new affect_data;
      paf->type = -1;
      paf->duration = -1;
      paf->rismod.reset(  );

      switch ( ch->pcdata->deity->objstat )
      {
         default:
            paf->location = APPLY_NONE;
            break;
         case 0:
            paf->location = APPLY_STR;
            break;
         case 1:
            paf->location = APPLY_INT;
            break;
         case 2:
            paf->location = APPLY_WIS;
            break;
         case 3:
            paf->location = APPLY_CON;
            break;
         case 4:
            paf->location = APPLY_DEX;
            break;
         case 5:
            paf->location = APPLY_CHA;
            break;
         case 6:
            paf->location = APPLY_LCK;
            break;
      }
      paf->modifier = 1;
      paf->bit = 0;
      obj->affects.push_back( paf );
      return;
   }

   if( !str_cmp( argument, "recall" ) )
   {
      room_index *location = nullptr;

      if( ch->pcdata->favor < ch->pcdata->deity->srecall )
      {
         ch->print( "Your favor is inadequate for such a supplication.\r\n" );
         return;
      }

      if( ch->in_room->flags.test( ROOM_NOSUPPLICATE ) )
      {
         ch->print( "You have been forsaken!\r\n" );
         return;
      }

      if( ch->get_timer( TIMER_RECENTFIGHT ) > 0 && !ch->is_immortal(  ) )
      {
         ch->print( "You cannot supplicate recall under adrenaline!\r\n" );
         return;
      }

      location = recall_room( ch );

      if( !location )
      {
         bug( "{}: No room index for recall supplication. Deity: {}", __func__, ch->pcdata->deity->name );
         ch->print( "Your deity has forsaken you!\r\n" );
         return;
      }

      act( AT_MAGIC, "$n disappears in a column of divine power.", ch, nullptr, nullptr, TO_ROOM );

      leave_map( ch, nullptr, location );

      act( AT_MAGIC, "$n appears in the room from a column of divine mist.", ch, nullptr, nullptr, TO_ROOM );
      ch->pcdata->favor -= ch->pcdata->deity->srecall;
      ch->adjust_favor( -1, 1 );
      return;
   }
   ch->print( "You cannot supplicate for that.\r\n" );
}
