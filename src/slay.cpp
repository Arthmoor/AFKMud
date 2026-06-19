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
 *         Slay V2.0 - Online editable configurable slay options            *
 ****************************************************************************/

/* -----------------------------------------------------------------------
The following snippet was written by Gary McNickle (dharvest) for
Rom 2.4 specific MUDs and is released into the public domain. My thanks to
the originators of Diku, and Rom, as well as to all those others who have
released code for this mud base.  Goes to show that the freeware idea can
actually work. ;)  In any case, all I ask is that you credit this code
properly, and perhaps drop me a line letting me know it's being used.

from: gary@dharvest.com
website: https://www.dharvest.com
or https://www.dharvest.com/resource.html (rom related)

Ported to Smaug 1.02a code by Samson
Updated to Smaug 1.4a code by Samson

Send any comments, flames, bug-reports, suggestions, requests, etc... 
to the above email address.
----------------------------------------------------------------------- */

#include <filesystem>
#include <fstream>
#include <memory>
#include "mud.h"
#include "slay.h"

void raw_kill( char_data *, char_data * );

std::list<std::unique_ptr<slay_data>> slaylist;

slay_data::slay_data(  )
{
   set_color( AT_IMMORT );
}

void free_slays( void )
{
   slaylist.clear();
}

/* Load the slay file */
void load_slays( )
{
   std::ifstream stream( std::filesystem::path{SLAY_FILE} );

   if( !stream )
   {
      log_string( "No slay.dat file found." );
      return;
   }

   slaylist.clear();
   int file_ver = 0;
   std::unique_ptr<slay_data> current_slay = nullptr;

   auto read_line = [&]( char delimiter = '\n' ) -> std::string
   {
      std::string line;
      std::getline( stream, line, delimiter );
      strip_spaces( line );

      return line;
   };

   std::string key;
   while( stream >> key )
   {
      std::string_view sv = key;

      if( sv == "#VERSION" )
      {
         file_ver = std::stoi( read_line() );
      }
      else if( sv == "#SLAY" )
      {
         current_slay = std::make_unique<slay_data>();
      }
      else if( sv == "End" )
      {
         if( current_slay )
            slaylist.push_back( std::move( current_slay ) );
      }
      else if( current_slay )
      {
         char delim = '~';

         if( file_ver == 1 )
            delim = (char)0xA2; // This was a stupid idea and it needs to be undone now.

         if( sv == "Type" ) current_slay->set_type( read_line() );
         else if( sv == "Owner" ) current_slay->set_owner( read_line() );
         else if( sv == "Color" ) current_slay->set_color( std::stoi( read_line() ) );
         else if( sv == "Cmessage" ) current_slay->set_cmsg( read_line( delim ) );
         else if( sv == "Vmessage" ) current_slay->set_vmsg( read_line( delim ) );
         else if( sv == "Rmessage" ) current_slay->set_rmsg( read_line( delim ) );
         else log_printf( "{}: Bad line: {}", __func__, key );
      }
   }
}

// 0: Original format with tilde delimiter.
// 1: Modified to use that stupid \xa2 thing as a delimiter.
// 2: Back to the tilde since the \xa2 delimiter was not reliable.
constexpr int SLAY_VERSION = 2;

/* Online slay editing, save the slay table to disk - Samson 8-3-98 */
void save_slays( )
{
   std::ofstream stream( std::filesystem::path{SLAY_FILE} );

   if( !stream )
   {
      bug( "{}: Cannot open {} for writing: {}", __func__, SLAY_FILE, std::strerror(errno) );
      return;
   }

   stream << "#VERSION " << SLAY_VERSION << "\n\n";

   for( const auto& slay : slaylist )
   {
      stream << "#SLAY\n";

      if( !slay->get_type().empty() )
         stream << "Type      " << slay->get_type() << "\n";

      if( !slay->get_owner().empty() )
         stream << "Owner     " << slay->get_owner() << "\n";

      if( slay->get_color() > 0 && slay->get_color() < MAX_COLORS )
         stream << "Color     " << slay->get_color() << "\n";

      if( !slay->get_cmsg().empty() )
         stream << "Cmessage  " << slay->get_cmsg() << "~\n";

      if( !slay->get_vmsg().empty() )
         stream << "Vmessage  " << slay->get_vmsg() << "~\n";

      if( !slay->get_rmsg().empty() )
         stream << "Rmessage  " << slay->get_rmsg() << "~\n";

      stream << "End\n\n";
   }
   stream.close();
   if( stream.fail() )
      bug( "{}: Error occurred after closing {}: ", __func__, SLAY_FILE, std::strerror(errno) );
}

/** Function: do_slay
  * Descr   : Slays (kills) a player, optionally sending one of several
  *           predefined "slay option" messages to those involved.
  * Returns : (void)
  * Syntax  : slay (who) [option]
  * Written : v1.0 12/97
  * Author  : Gary McNickle <gary@dharvest.com>
  * Ported to Smaug 1.02a by: Samson
  * Updated to work with Smaug 1.4 by Samson 8-3-98
  * v2.0 added support for online editing
  */
CMDF( do_slay )
{
   char_data *victim;
   std::string who;
   bool found = false;

   if( ch->isnpc(  ) )
   {
      ch->print( "Mobs can't use the slay command.\r\n" );
      return;
   }

   argument = one_argument( argument, who );

   if( who.empty(  ) || !str_prefix( who, "list" ) )
   {
      ch->pager( "&gSyntax: slay <victim> [type]\r\n" );
      ch->pager( "Where type is one of the following...\r\n\r\n" );
      ch->pager( "&GSlay                      &ROwner\r\n" );
      ch->pager( "&g-------------------------+---------------\r\n" );

      for( const auto& slay : slaylist )
      {
         ch->pager_fmt( "&G{:<27} &g{}\r\n", slay->get_type(  ), slay->get_owner(  ) );
      }
      ch->print( "\r\n&gTyping just 'slay <player>' will work too...\r\n" );
      return;
   }

   if( !( victim = ch->get_char_room( who ) ) )
   {
      ch->print( "They aren't here.\r\n" );
      return;
   }

   if( ch == victim )
   {
      ch->print( "Suicide is a mortal sin.\r\n" );
      return;
   }

   if( !victim->isnpc(  ) && victim->level > ch->level )
   {
      ch->print( "You cannot slay someone who is above your level.\r\n" );
      return;
   }

   if( argument.empty(  ) )
   {
      act( AT_IMMORT, "You brutally slay $N!", ch, nullptr, victim, TO_CHAR );
      act( AT_IMMORT, "$n chops you up into little pieces!", ch, nullptr, victim, TO_VICT );
      act( AT_IMMORT, "$n brutally slays $N!", ch, nullptr, victim, TO_NOTVICT );
      raw_kill( ch, victim );
      return;
   }
   else
   {
      for( const auto& slay : slaylist )
      {
         if( ( !str_cmp( argument, slay->get_type(  ) ) && !str_cmp( "Any", slay->get_owner(  ) ) )
             || ( !str_cmp( slay->get_owner(  ), ch->name ) && !str_cmp( argument, slay->get_type(  ) ) ) )
         {
            found = true;
            act( slay->get_color(  ), slay->get_cmsg(  ).c_str(  ), ch, nullptr, victim, TO_CHAR );
            act( slay->get_color(  ), slay->get_vmsg(  ).c_str(  ), ch, nullptr, victim, TO_VICT );
            act( slay->get_color(  ), slay->get_rmsg(  ).c_str(  ), ch, nullptr, victim, TO_NOTVICT );
            raw_kill( ch, victim );
            return;
         }
      }
   }

   if( !found )
   {
      ch->print( "&RSlay type not defined, or not owned by you.\r\n" );
      ch->print( "Type \"slay list\" for a complete listing of types available to you.\r\n" );
   }
}

slay_data *get_slay( std::string_view name )
{
   for( const auto& slay : slaylist )
   {
      if( !str_cmp( name, slay->get_type(  ) ) )
         return slay.get();
   }
   return nullptr;
}

/* Create a slaytype online - Samson 8-3-98 */
CMDF( do_makeslay )
{
   if( ch->isnpc(  ) )
   {
      ch->print( "Huh?\r\n" );
      return;
   }

   if( argument.empty(  ) )
   {
      ch->print( "Usage: makeslay <slaytype>\r\n" );
      return;
   }

   /*
    * Glaring oversight just noticed - Samson 7-5-99 
    */
   if( get_slay( argument ) != nullptr )
   {
      ch->print( "That slay type already exists.\r\n" );
      return;
   }

   auto slay = std::make_unique<slay_data>();
   slay->set_type( argument );
   slay->set_owner( "Any" );
   slay->set_color( AT_IMMORT );
   slay->set_cmsg( "You brutally slay $N!" );
   slay->set_vmsg( "$n chops you up into little pieces!" );
   slay->set_rmsg( "$n brutally slays $N!" );
   slaylist.push_back( std::move( slay ) );
   ch->print_fmt( "New slaytype {} added. Set to default values.\r\n", argument );
   save_slays(  );
}

/* Set slay values online - Samson 8-3-98 */
CMDF( do_setslay )
{
   std::string arg1, arg2;
   slay_data *slay;

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

      case SUB_SLAYCMSG:
         slay = ( slay_data * ) ch->pcdata->dest_buf;
         slay->set_cmsg( ch->copy_buffer( ) );
         ch->stop_editing(  );
         ch->substate = ch->tempnum;
         save_slays(  );
         return;

      case SUB_SLAYVMSG:
         slay = ( slay_data * ) ch->pcdata->dest_buf;
         slay->set_vmsg( ch->copy_buffer( ) );
         ch->stop_editing(  );
         ch->substate = ch->tempnum;
         save_slays(  );
         return;

      case SUB_SLAYRMSG:
         slay = ( slay_data * ) ch->pcdata->dest_buf;
         slay->set_rmsg( ch->copy_buffer( ) );
         ch->stop_editing(  );
         ch->substate = ch->tempnum;
         save_slays(  );
         return;

      case SUB_EDIT_ABORT:
         ch->substate = SUB_NONE;
         ch->print( "Aborting slay description.\r\n" );
         return;
   }

   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );

   if( arg1.empty(  ) )
   {
      ch->print( "Usage: setslay <slaytype> <field> <value>\r\n" );
      ch->print( "Usage: setslay save\r\n" );
      ch->print( "\r\nField being one of:\r\n" );
      ch->print( "owner color cmsg vmsg rmsg\r\n" );
      return;
   }

   if( !str_cmp( arg1, "save" ) )
   {
      save_slays(  );
      ch->print( "Slay table saved.\r\n" );
      return;
   }

   if( !( slay = get_slay( arg1 ) ) )
   {
      ch->print( "No such slaytype.\r\n" );
      return;
   }

   if( !str_cmp( arg2, "owner" ) )
   {
      slay->set_owner( argument );
      ch->print( "New owner set.\r\n" );
      save_slays(  );
      return;
   }

   if( !str_cmp( arg2, "cmsg" ) )
   {
      if( ch->substate == SUB_REPEATCMD )
         ch->tempnum = SUB_REPEATCMD;
      else
         ch->tempnum = SUB_NONE;
      ch->substate = SUB_SLAYCMSG;
      ch->pcdata->dest_buf = slay;
      slay->set_cmsg( "" );
      ch->set_editor_desc( "A custom slay message." );
      ch->start_editing( slay->get_cmsg(  ) );
      return;
   }

   if( !str_cmp( arg2, "vmsg" ) )
   {
      if( ch->substate == SUB_REPEATCMD )
         ch->tempnum = SUB_REPEATCMD;
      else
         ch->tempnum = SUB_NONE;
      ch->substate = SUB_SLAYVMSG;
      ch->pcdata->dest_buf = slay;
      slay->set_vmsg( "" );
      ch->set_editor_desc( "A custom slay message." );
      ch->start_editing( slay->get_vmsg(  ) );
      return;
   }

   if( !str_cmp( arg2, "rmsg" ) )
   {
      if( ch->substate == SUB_REPEATCMD )
         ch->tempnum = SUB_REPEATCMD;
      else
         ch->tempnum = SUB_NONE;
      ch->substate = SUB_SLAYRMSG;
      ch->pcdata->dest_buf = slay;
      slay->set_rmsg( "" );
      ch->set_editor_desc( "A custom slay message." );
      ch->start_editing( slay->get_rmsg(  ) );
      return;
   }

   if( !str_cmp( arg2, "color" ) )
   {
      slay->set_color( atoi( argument.c_str(  ) ) );
      ch->print( "Slay color set.\r\n" );
      save_slays(  );
      return;
   }
   do_setslay( ch, "" );
}

/* Online slay editor, show details of a slaytype - Samson 8-3-98 */
CMDF( do_showslay )
{
   slay_data *slay;

   if( ch->isnpc(  ) )
   {
      ch->print( "Huh?\r\n" );
      return;
   }

   if( argument.empty(  ) )
   {
      ch->print( "Usage: showslay <slaytype>\r\n" );
      return;
   }

   if( !( slay = get_slay( argument ) ) )
   {
      ch->print( "No such slaytype.\r\n" );
      return;
   }

   ch->print_fmt( "\r\nSlaytype: {}\r\n", slay->get_type(  ) );
   ch->print_fmt( "Owner:    {}\r\n", slay->get_owner(  ) );
   ch->print_fmt( "Color:    {}\r\n", slay->get_color(  ) );
   ch->print_fmt( "&RCmessage: \r\n{}", slay->get_cmsg(  ) );
   ch->print_fmt( "\r\n&YVmessage: \r\n{}", slay->get_vmsg(  ) );
   ch->print_fmt( "\r\n&GRmessage: \r\n{}", slay->get_rmsg(  ) );
}

/* Of course, to create means you need to be able to destroy as well :P - Samson 8-3-98 */
CMDF( do_destroyslay )
{
   slay_data *pslay;

   if( ch->isnpc(  ) )
   {
      ch->print( "Huh?\r\n" );
      return;
   }

   if( argument.empty(  ) )
   {
      ch->print( "Destroy which slaytype?\r\n" );
      return;
   }

   if( !( pslay = get_slay( argument ) ) )
   {
      ch->print( "No such slaytype.\r\n" );
      return;
   }

   deleteptr( pslay );
   ch->print_fmt( "Slaytype \"{}%s\" has beed deleted.\r\n", argument );
   save_slays(  );
}
