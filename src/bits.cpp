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
 ****************************************************************************/

/* bits.c -- Abits and Qbits for the Rogue Winds by Scion
   Copyright 2000 by Peter Keeler, All Rights Reserved. The content
   of this file may be used by anyone for any purpose so long as this
   original header remains entirely intact and credit is given to the
   original author(s).

   The concept for this was inspired by Mallory's mob scripting system
   from AntaresMUD.

   It is not required, but I'd appreciate hearing back from people
   who use this code. What are you using it for, what have you done
   to it, ideas, comments, etc. So while it's not necessary, I'd love
   to get a note from you at scion@divineright.org. Thanks! -- Scion
*/

#include <filesystem>
#include <fstream>
#include "mud.h"
#include "bits.h"
#include "mud_prog.h"

/* These are the ends of the linked lists that store the mud's library of valid bits. */
std::map<int, std::string> abits;
std::map<int, std::string> qbits;

/* QBITS save, ABITS do not save. There are enough of each to give a range
   of them to builders the same as their vnums. They are identifiable by mobs
   running mob progs, and can be used as little identifiers for players..
   player X has done this and this and this.. think of them like a huge array
   of boolean variables you can put on a player or mob with a mob prog. -- Scion
*/
void free_questbits( void )
{
   qbits.clear(  );
   abits.clear(  );
}

/* Write out the abit and qbit files */
void save_bits( void )
{
   std::map<int, std::string> start_bit;
   std::map<int, std::string>::iterator bit;
   std::ofstream stream;
   std::filesystem::path filename;

   /*
    * Print 2 files 
    */
   for( short mode = 0; mode < 2; ++mode )
   {
      if( mode == 0 )
      {
         filename = std::format( "{}abits.lst", SYSTEM_DIR );
         start_bit = abits;
      }
      else
      {
         filename = std::format( "{}qbits.lst", SYSTEM_DIR );
         start_bit = qbits;
      }

      stream.open( filename );
      if( !stream.is_open(  ) )
      {
         bug( "%s: Cannot open bit list %d for writing", __func__, mode );
         return;
      }

      for( bit = start_bit.begin(  ); bit != start_bit.end(  ); ++bit )
         stream << bit->first << " " << bit->second << std::endl;
      stream.close(  );
   }
}

/* Load the abits and qbits */
void load_oldbits( void )
{
   std::filesystem::path buf;
   int mode = 0, number = -1;
   std::string desc;
   FILE *fp;

   abits.clear(  );
   qbits.clear(  );

   buf = std::format( "{}abit.lst", SYSTEM_DIR );
   if( !( fp = fopen( buf.c_str(), "r" ) ) )
   {
      perror( buf.c_str() );
      return;
   }

   for( ;; )
   {
      const char *word = ( feof( fp ) ? "End" : fread_word( fp ) );

      if( word[0] == '\0' )
      {
         bug( "%s: EOF encountered reading old bits file!", __func__ );
         word = "End";
      }

      switch ( to_upper( word[0] ) )
      {
         default:
            bug( "%s: no match: %s", __func__, word );
            fread_to_eol( fp );
            break;

         case '*':
            fread_to_eol( fp );
            break;

         case '#':
            if( !str_cmp( word, "#END" ) )
            {
               FCLOSE( fp );
               std::filesystem::remove( buf );
               if( mode == 0 )
               {
                  mode = 1;   /* We have two files to read, I reused the same code to read both */
                  buf = std::format( "{}qbit.lst", SYSTEM_DIR );
                  if( !( fp = fopen( buf.c_str(), "r" ) ) )
                  {
                     perror( buf.c_str() );
                     return;
                  }
               }
               else
                  return;
            }
            else
            {
               bug( "%s: Bad section: %s", __func__, word );
               return;
            }

         case 'D':
            STDSKEY( "Desc", desc );
            break;

         case 'E':
            if( !str_cmp( word, "End" ) )
            {
               if( mode == 0 )
                  abits[number] = desc;
               else
                  qbits[number] = desc;
            }
            break;

         case 'N':
            KEY( "Number", number, fread_number( fp ) );
            break;
      }
   }
}

void load_abits( void )
{
   std::ifstream stream;

   log_string( "...abits" );
   std::filesystem::path filename = std::format( "{}abits.lst", SYSTEM_DIR );
   stream.open( filename );

   if( !stream.is_open(  ) )
   {
      bug( "%s: Cannot open abit file.", __func__ );
      return;
   }

   do
   {
      int number;
      std::string desc;
      char line[MSL];

      stream >> number;
      stream.getline( line, MSL );
      desc = line;
      abits[number] = desc;
   }
   while( !stream.eof(  ) );
   stream.close(  );
}

void load_qbits( void )
{
   std::ifstream stream;

   log_string( "...qbits" );
   std::filesystem::path filename = std::format( "{}qbits.lst", SYSTEM_DIR );
   stream.open( filename );
   if( !stream.is_open(  ) )
   {
      bug( "%s: Cannot open qbit file.", __func__ );
      return;
   }

   do
   {
      int number;
      std::string desc;
      char line[MSL];

      stream >> number;
      stream.getline( line, MSL );
      desc = line;
      qbits[number] = desc;
   }
   while( !stream.eof(  ) );
   stream.close(  );
}

void load_bits( void )
{
   abits.clear(  );
   qbits.clear(  );

   std::filesystem::path filename = std::format( "{}abit.lst", SYSTEM_DIR );
   if( std::filesystem::exists( filename ) )
   {
      load_oldbits(  );
      save_bits(  );

      /*
       * If we don't do this, the code will continue to see the old filename and keep reloading it and wiping out any changes made in the new ones.
       * Should only be relevant for very old codebase installs as the old default files were removed in an earlier release.
       */
      std::filesystem::remove( filename );
      filename = std::format( "{}qbit.lst", SYSTEM_DIR );
      std::filesystem::remove( filename );
      return;
   }

   load_abits(  );
   load_qbits(  );
}

/* Add an abit to a character */
void set_abit( char_data * ch, int number )
{
   if( number < 0 || number > MAX_xBITS )
      return;

   if( abits.find( number ) == abits.end(  ) )
      return;

   if( ch->abits.find( number ) != ch->abits.end(  ) )
      ch->abits[number] = abits[number];
}

/* Add a qbit to a character */
void set_qbit( char_data * ch, int number )
{
   if( number < 0 || number > MAX_xBITS )
      return;

   if( qbits.find( number ) == qbits.end(  ) )
      return;

   if( ch->pcdata->qbits.find( number ) == ch->pcdata->qbits.end(  ) )
      ch->pcdata->qbits[number] = qbits[number];
}

/* Take an abit off a character */
void remove_abit( char_data * ch, int number )
{
   if( number < 0 || number > MAX_xBITS )
      return;

   if( ch->abits.empty(  ) )
      return;

   std::map<int, std::string>::iterator bit;
   if( ( bit = ch->abits.find( number ) ) != ch->abits.end(  ) )
      ch->abits.erase( bit );
}

/* Take a qbit off a character */
void remove_qbit( char_data * ch, int number )
{
   if( ch->isnpc(  ) )
      return;

   if( number < 0 || number > MAX_xBITS )
      return;

   if( ch->pcdata->qbits.empty(  ) )
      return;

   std::map<int, std::string>::iterator bit;
   if( ( bit = ch->pcdata->qbits.find( number ) ) != ch->pcdata->qbits.end(  ) )
      ch->pcdata->qbits.erase( bit );
}

/* Show an abit from the mud's linked list, or all of them if 'all' is the argument */
CMDF( do_showabit )
{
   int number;
   std::map<int, std::string>::iterator bit;

   if( abits.empty(  ) )
   {
      ch->print( "There are no Abits defined.\r\n" );
      return;
   }

   if( !str_cmp( argument, "all" ) )
   {
      for( bit = abits.begin(  ); bit != abits.end(  ); ++bit )
         ch->print_fmt( "&RABIT: &Y{} &G{}\r\n", bit->first, bit->second );
      return;
   }

   number = std::stoi( argument );

   if( number < 0 || number > MAX_xBITS )
      return;

   if( ( bit = abits.find( number ) ) != abits.end(  ) )
   {
      ch->print_fmt( "&RABIT: &Y{}\r\n", bit->first );
      ch->print_fmt( "&G{}\r\n", bit->second );
      return;
   }
   ch->print( "That abit does not exist.\r\n" );
}

/* Show a qbit from the mud's linked list or all of them if 'all' is the argument */
CMDF( do_showqbit )
{
   int number;
   std::map<int, std::string>::iterator bit;

   if( qbits.empty(  ) )
   {
      ch->print( "There are no Qbits defined.\r\n" );
      return;
   }

   if( !str_cmp( argument, "all" ) )
   {
      for( bit = qbits.begin(  ); bit != qbits.end(  ); ++bit )
         ch->printf( "&RQBIT: &Y%d &G%s\r\n", bit->first, bit->second.c_str(  ) );
      return;
   }

   number = std::stoi( argument );

   if( number < 0 || number > MAX_xBITS )
      return;

   if( ( bit = qbits.find( number ) ) != qbits.end(  ) )
   {
      ch->print_fmt( "&RQBIT: &Y{}\r\n", bit->first );
      ch->print_fmt( "&G {}\r\n", bit->second );
      return;
   }
   ch->print( "That qbit does not exist.\r\n" );
}

/* setabit <number> <desc> */
/* Set the description for a particular abit */
CMDF( do_setabit )
{
   std::map<int, std::string>::iterator bit;
   int number;
   std::string arg;

   argument = one_argument( argument, arg );

   if( arg.empty(  ) )
   {
      ch->print( "You must supply a bit number!\r\n" );
      return;
   }

   if( !is_number( arg ) )
   {
      ch->print( "You must specify a numerical bit value.\r\n" );
      return;
   }
   number = std::stoi( arg );

   if( argument.empty(  ) )
   {
      ch->print( "You must supply a description, or \"delete\" if you wish to delete.\r\n" );
      return;
   }

   arg = argument;

   if( number < 0 || number > MAX_xBITS )
   {
      ch->print( "That is not a valid number for an abit.\r\n" );
      return;
   }

   if( arg.empty(  ) )
   {
      ch->print( "Syntax: setabit <number> <description>\r\n" );
      ch->print( "Syntax: setabit <number> delete\r\n" );
      return;
   }

   if( ( bit = abits.find( number ) ) != abits.end(  ) )
   {
      if( !str_cmp( arg, "delete" ) )
      {
         abits.erase( number );
         ch->print_fmt( "Abit {} has been destroyed.\r\n", number );
         return;
      }
      abits[number] = arg;
      ch->print_fmt( "Description for abit {} set to '{}'.\r\n", number, arg );
      return;
   }
   abits[number] = arg;
   ch->print_fmt( "Abit {} created.\r\n", number );
   ch->print_fmt( "Description for abit {} set to '{}'.\r\n", number, arg );
   save_bits(  );
}

/* setqbit <number> <desc> */
/* Set the description for a particular qbit */
CMDF( do_setqbit )
{
   std::map< int, std::string>::iterator bit;
   int number;
   std::string arg;

   argument = one_argument( argument, arg );

   if( arg.empty(  ) )
   {
      ch->print( "You must supply a bit number!\r\n" );
      return;
   }

   if( !is_number( arg ) )
   {
      ch->print( "You must specify a numerical bit value.\r\n" );
      return;
   }
   number = std::stoi( arg );

   if( argument.empty(  ) )
   {
      ch->print( "You must supply a description, or \"delete\" if you wish to delete.\r\n" );
      return;
   }

   arg = argument;

   if( number < 0 || number > MAX_xBITS )
   {
      ch->print( "That is not a valid number for a qbit.\r\n" );
      return;
   }

   if( arg.empty(  ) )
   {
      ch->print( "Syntax: setqbit <number> <description>\r\n" );
      ch->print( "Syntax: setqbit <number> delete\r\n" );
      return;
   }

   if( ( bit = qbits.find( number ) ) != qbits.end(  ) )
   {
      if( !str_cmp( arg, "delete" ) )
      {
         qbits.erase( number );
         ch->print_fmt( "Qbit {} has been destroyed.\r\n", number );
         return;
      }
      qbits[number] = arg;
      ch->print_fmt( "Description for qbit {} set to '{}'.\r\n", number, arg );
      return;
   }
   qbits[number] = arg;
   ch->print_fmt( "Qbit {} created.\r\n", number );
   ch->print_fmt( "Description for qbit {} set to '{}'.\r\n", number, arg );
   save_bits(  );
}

/* Imm command to toggle an abit on a character or to list the abits already on a character */
CMDF( do_abit )
{
   std::string buf;
   char_data *victim;

   argument = one_argument( argument, buf );

   if( buf.empty(  ) )
   {
      ch->print( "Whose bits do you want to examine?\r\n" );
      return;
   }

   if( !( victim = ch->get_char_world( buf ) ) )
   {
      ch->print( "They are not in the game.\r\n" );
      return;
   }

   argument = one_argument( argument, buf );

   if( buf.empty(  ) )
   {
      if( victim->abits.empty(  ) )
      {
         ch->print( "They have no abits set on them.\r\n" );
         return;
      }

      ch->print_fmt( "&RABITS for {}:\r\n", victim->isnpc(  ) ? victim->short_descr : victim->name );

      std::map<int, std::string>::iterator bit;
      for( bit = victim->abits.begin(  ); bit != victim->abits.end(  ); ++bit )
         ch->print_fmt( "&Y{:4}: &G{}\r\n", bit->first, bit->second );
   }
   else
   {
      int abit;

      abit = std::stoi( buf );

      if( abit < 0 || abit > MAX_xBITS )
      {
         ch->print( "That is an invalid abit number.\r\n" );
         return;
      }

      if( victim->abits.find( abit ) != victim->abits.end(  ) )
      {
         remove_abit( victim, abit );
         ch->print_fmt( "Removed abit {} from {}.\r\n", abit, victim->isnpc(  ) ? victim->short_descr : victim->name );
      }
      else
      {
         if( abits.find( abit ) == abits.end(  ) )
         {
            ch->print_fmt( "Abit {} is not a valid number.\r\n", abit );
            return;
         }
         set_abit( victim, abit );
         ch->print_fmt( "Added abit {} to {}.\r\n", abit, victim->isnpc(  ) ? victim->short_descr : victim->name );
      }
   }
}

/* Immortal command to toggle a qbit on a character or to list the qbits already on a character */
CMDF( do_qbit )
{
   char_data *victim;
   std::string buf;

   argument = one_argument( argument, buf );

   if( buf.empty(  ) )
   {
      ch->print( "Whose bits do you want to examine?\r\n" );
      return;
   }

   if( !( victim = ch->get_char_world( buf ) ) )
   {
      ch->print( "They are not in the game.\r\n" );
      return;
   }

   if( victim->isnpc(  ) )
   {
      ch->print( "NPCs cannot have qbits.\r\n" );
      return;
   }

   argument = one_argument( argument, buf );

   if( buf.empty(  ) )
   {
      if( victim->pcdata->qbits.empty(  ) )
      {
         ch->print( "They do not have any qbits.\r\n" );
         return;
      }

      ch->print_fmt( "&RQBITS for {}:\r\n", victim->name );

      std::map<int, std::string>::iterator bit;
      for( bit = victim->pcdata->qbits.begin(  ); bit != victim->pcdata->qbits.end(  ); ++bit )
         ch->print_fmt( "&Y{:4}: &G{}\r\n", bit->first, bit->second );
   }
   else
   {
      int qbit;

      qbit = std::stoi( buf );

      if( qbit < 0 || qbit > MAX_xBITS )
      {
         ch->print( "That is an invalid qbit number.\r\n" );
         return;
      }

      if( victim->pcdata->qbits.find( qbit ) != victim->pcdata->qbits.end(  ) )
      {
         remove_qbit( victim, qbit );
         ch->print_fmt( "Removed qbit {} from {}.\r\n", qbit, victim->name );
      }
      else
      {
         if( qbits.find( qbit ) == qbits.end(  ) )
         {
            ch->print_fmt( "Qbit {} is not a valid number.\r\n", qbit );
            return;
         }
         set_qbit( victim, qbit );
         ch->print_fmt( "Added qbit {} to {}.\r\n", qbit, victim->name );
      }
   }
}

/* mpaset <char> */
/* Mob prog version of do_abit */
/* Don't use this with death_progs or anything else that has the potential for the target to
 * be in another room. If the target will be in a different location after the prog, set the
 * bit BEFORE they move.
 */
CMDF( do_mpaset )
{
   std::string arg1, arg2;
   char_data *victim;
   int number;

   if( !ch->isnpc(  ) || ch->desc || ch->has_aflag( AFF_CHARM ) )
   {
      ch->print( "Huh?\r\n" );
      return;
   }

   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );

   if( arg1.empty(  ) )
   {
      progbug( "Mpaset: missing victim", ch );
      return;
   }

   if( arg2.empty(  ) )
   {
      progbug( "Mpaset: missing bit", ch );
      return;
   }

   if( !( victim = ch->get_char_room( arg1 ) ) )
   {
      progbugf( ch, "Mpaset: victim {} not in room", arg1 );
      return;
   }

   number = std::stoi( arg2 );
   if( victim->abits.find( number ) != victim->abits.end(  ) )
      remove_abit( victim, number );
   else
   {
      if( abits.find( number ) == abits.end(  ) )
         return;
      set_abit( victim, number );
   }
}

/* mpqset <char> */
/* Mob prog version of do_qbit */
/* Because this can be used on death_progs, be SURE your victim is correct or you could
 * end up setting a bit on the wrong person. Use 0.$n as your victim target in progs.
 */
CMDF( do_mpqset )
{
   std::string arg1, arg2;
   char_data *victim;
   int number;

   if( !ch->isnpc(  ) || ch->desc || ch->has_aflag( AFF_CHARM ) )
   {
      ch->print( "Huh?\r\n" );
      return;
   }

   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );

   if( arg1.empty(  ) )
   {
      progbug( "Mpqset: missing victim", ch );
      return;
   }

   if( arg2.empty(  ) )
   {
      progbug( "Mpqset: missing bit", ch );
      return;
   }

   if( !( victim = ch->get_char_world( arg1 ) ) )
   {
      progbugf( ch, "Mpqset: victim {} not in game", arg1 );
      return;
   }

   if( victim->isnpc(  ) )
   {
      progbugf( ch, "Mpqset: setting Qbit on NPC {}", victim->short_descr );
      return;
   }

   number = std::stoi( arg2 );
   if( victim->pcdata->qbits.find( number ) != victim->pcdata->qbits.end(  ) )
      remove_qbit( victim, number );
   else
   {
      if( qbits.find( number ) == qbits.end(  ) )
         return;
      set_qbit( victim, number );
   }
}
