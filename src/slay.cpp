/****************************************************************************
 *                   ^     +----- |  / ^     ^ |     | +-\                  *
 *                  / \    |      | /  |\   /| |     | |  \                 *
 *                 /   \   +---   |<   | \ / | |     | |  |                 *
 *                /-----\  |      | \  |  v  | |     | |  /                 *
 *               /       \ |      |  \ |     | +-----+ +-/                  *
 ****************************************************************************
 * AFKMud Copyright 1997-2012 by Roger Libiez (Samson),                     *
 * Levi Beckerson (Whir), Michael Ward (Tarl), Erik Wolfe (Dwip),           *
 * Cameron Carroll (Cam), Cyberfox, Karangi, Rathian, Raine,                *
 * Xorith, and Adjani.                                                      *
 * All Rights Reserved.                                                     *
 *                                                                          *
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
website: http://www.dharvest.com
or http://www.dharvest.com/resource.html (rom related)

Ported to Smaug 1.02a code by Samson
Updated to Smaug 1.4a code by Samson

Send any comments, flames, bug-reports, suggestions, requests, etc... 
to the above email address.
----------------------------------------------------------------------- */

#include <fstream>
#include "mud.h"
#include "slay.h"

void raw_kill( char_data *, char_data * );

list < slay_data * >slaylist;

/* Load the slay file */
void load_slays( void )
{
   slay_data *slay;
   ifstream stream;
   int file_ver = 0;

   slaylist.clear(  );

   stream.open( SLAY_FILE );
   if( !stream.is_open(  ) )
   {
      log_string( "No slay file found." );
      return;
   }

   do
   {
      string key, value;
      char buf[MIL];

      stream >> key;
      strip_lspace( key );

      if( key.empty(  ) )
         continue;

      if( key == "#VERSION" )
      {
         stream.getline( buf, MIL );
         value = buf;
         strip_lspace( value );
         file_ver = atoi( value.c_str(  ) );
      }
      else if( key == "#SLAY" )
         slay = new slay_data;
      else if( key == "Type" )
      {
         stream.getline( buf, MIL );
         value = buf;
         strip_lspace( value );
         if( file_ver < 1 )
            strip_tilde( value );
         slay->set_type( value );
      }
      else if( key == "Owner" )
      {
         stream.getline( buf, MIL );
         value = buf;
         strip_lspace( value );
         if( file_ver < 1 )
            strip_tilde( value );
         slay->set_owner( value );
      }
      else if( key == "Color" )
      {
         stream.getline( buf, MIL );
         value = buf;
         strip_lspace( value );
         slay->set_color( atoi( value.c_str(  ) ) );
      }
      else if( key == "Cmessage" )
      {
         if( file_ver < 1 )
            stream.getline( buf, MIL, '~' );
         else
            stream.getline( buf, MIL, '¢' );
         value = buf;
         strip_lspace( value );
         slay->set_cmsg( value );
      }
      else if( key == "Vmessage" )
      {
         if( file_ver < 1 )
            stream.getline( buf, MIL, '~' );
         else
            stream.getline( buf, MIL, '¢' );
         value = buf;
         strip_lspace( value );
         slay->set_vmsg( value );
      }
      else if( key == "Rmessage" )
      {
         if( file_ver < 1 )
            stream.getline( buf, MIL, '~' );
         else
            stream.getline( buf, MIL, '¢' );
         value = buf;
         strip_lspace( value );
         slay->set_rmsg( value );
      }
      else if( key == "End" )
         slaylist.push_back( slay );
      else
         log_printf( "%s: Bad line in slay file: %s", __FUNCTION__, key.c_str(  ) );
   }
   while( !stream.eof(  ) );
   stream.close(  );
}

/* Online slay editing, save the slay table to disk - Samson 8-3-98 */
void save_slays( void )
{
   ofstream stream;

   stream.open( SLAY_FILE );
   if( !stream.is_open(  ) )
   {
      bug( "%s: fopen", __FUNCTION__ );
      perror( SLAY_FILE );
   }
   else
   {
      list < slay_data * >::iterator sl;

      stream << "#VERSION 1" << endl;
      for( sl = slaylist.begin(  ); sl != slaylist.end(  ); ++sl )
      {
         slay_data *slay = *sl;

         stream << "#SLAY" << endl;
         stream << "Type      " << slay->get_type(  ) << endl;
         stream << "Owner     " << slay->get_owner(  ) << endl;
         stream << "Color     " << slay->get_color(  ) << endl;
         stream << "Cmessage  " << slay->get_cmsg(  ) << "¢" << endl;
         stream << "Vmessage  " << slay->get_vmsg(  ) << "¢" << endl;
         stream << "Rmessage  " << slay->get_rmsg(  ) << "¢" << endl;
         stream << "End" << endl << endl;
      }
      stream.close(  );
   }
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
   string who;
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
      ch->pager( "&YSlay                 &ROwner\r\n" );
      ch->pager( "&g-------------------------+---------------\r\n" );

      list < slay_data * >::iterator sl;
      for( sl = slaylist.begin(  ); sl != slaylist.end(  ); ++sl )
      {
         slay_data *slay = *sl;
         ch->pagerf( "&G%-14s &g%13s\r\n", slay->get_type(  ).c_str(  ), slay->get_owner(  ).c_str(  ) );
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
      act( AT_IMMORT, "You brutally slay $N!", ch, NULL, victim, TO_CHAR );
      act( AT_IMMORT, "$n chops you up into little pieces!", ch, NULL, victim, TO_VICT );
      act( AT_IMMORT, "$n brutally slays $N!", ch, NULL, victim, TO_NOTVICT );
      raw_kill( ch, victim );
      return;
   }
   else
   {
      list < slay_data * >::iterator sl;
      for( sl = slaylist.begin(  ); sl != slaylist.end(  ); ++sl )
      {
         slay_data *slay = *sl;

         if( ( !str_cmp( argument, slay->get_type(  ) ) && !str_cmp( "Any", slay->get_owner(  ) ) )
             || ( !str_cmp( slay->get_owner(  ), ch->name ) && !str_cmp( argument, slay->get_type(  ) ) ) )
         {
            found = true;
            act( slay->get_color(  ), slay->get_cmsg(  ).c_str(  ), ch, NULL, victim, TO_CHAR );
            act( slay->get_color(  ), slay->get_vmsg(  ).c_str(  ), ch, NULL, victim, TO_VICT );
            act( slay->get_color(  ), slay->get_rmsg(  ).c_str(  ), ch, NULL, victim, TO_NOTVICT );
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

slay_data *get_slay( const string & name )
{
   list < slay_data * >::iterator sl;

   for( sl = slaylist.begin(  ); sl != slaylist.end(  ); ++sl )
   {
      slay_data *slay = *sl;

      if( !str_cmp( name, slay->get_type(  ) ) )
         return slay;
   }
   return NULL;
}

/* Create a slaytype online - Samson 8-3-98 */
CMDF( do_makeslay )
{
   slay_data *slay;

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
   if( ( slay = get_slay( argument ) ) != NULL )
   {
      ch->print( "That slay type already exists.\r\n" );
      return;
   }

   slay = new slay_data;
   slay->set_type( argument );
   slay->set_owner( "Any" );
   slay->set_color( AT_IMMORT );
   slay->set_cmsg( "You brutally slay $N!" );
   slay->set_vmsg( "$n chops you up into little pieces!" );
   slay->set_rmsg( "$n brutally slays $N!" );
   slaylist.push_back( slay );
   ch->printf( "New slaytype %s added. Set to default values.\r\n", argument.c_str(  ) );
   save_slays(  );
}

/* Set slay values online - Samson 8-3-98 */
CMDF( do_setslay )
{
   string arg1, arg2;
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
         slay->set_cmsg( ch->copy_buffer(  ) );
         ch->stop_editing(  );
         ch->substate = ch->tempnum;
         save_slays(  );
         return;

      case SUB_SLAYVMSG:
         slay = ( slay_data * ) ch->pcdata->dest_buf;
         slay->set_vmsg( ch->copy_buffer(  ) );
         ch->stop_editing(  );
         ch->substate = ch->tempnum;
         save_slays(  );
         return;

      case SUB_SLAYRMSG:
         slay = ( slay_data * ) ch->pcdata->dest_buf;
         slay->set_rmsg( ch->copy_buffer(  ) );
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

   ch->printf( "\r\nSlaytype: %s\r\n", slay->get_type(  ).c_str(  ) );
   ch->printf( "Owner:    %s\r\n", slay->get_owner(  ).c_str(  ) );
   ch->printf( "Color:    %d\r\n", slay->get_color(  ) );
   ch->printf( "&RCmessage: \r\n%s", slay->get_cmsg(  ).c_str(  ) );
   ch->printf( "\r\n&YVmessage: \r\n%s", slay->get_vmsg(  ).c_str(  ) );
   ch->printf( "\r\n&GRmessage: \r\n%s", slay->get_rmsg(  ).c_str(  ) );
}

slay_data::slay_data(  )
{
   set_color( AT_IMMORT );
}

slay_data::~slay_data(  )
{
   slaylist.remove( this );
}

void free_slays( void )
{
   list < slay_data * >::iterator dslay;

   for( dslay = slaylist.begin(  ); dslay != slaylist.end(  ); )
   {
      slay_data *slay = *dslay;
      ++dslay;

      deleteptr( slay );
   }
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
   ch->printf( "Slaytype \"%s\" has beed deleted.\r\n", argument.c_str(  ) );
   save_slays(  );
}
