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
 * Comments: 'notes' attached to players to keep track of outstanding       * 
 *           and problem players.  -haus 6/25/1995                          * 
 ****************************************************************************/

#include <fstream>
#include "mud.h"
#include "descriptor.h"
#include "boards.h"

void read_note( note_data *, int, std::ifstream & );
void write_note( note_data *, std::ofstream & );
void note_to_char( char_data *, note_data *, board_data *, short );
extern const char *note_flags[];
void TOGGLE_NOTE_FLAG( note_data *, int );

void pc_data::free_comments(  )
{
   for( auto it = comments.begin(  ); it != comments.end(  ); )
   {
      note_data *comment = *it;
      ++it;

      comments.remove( comment );
      deleteptr( comment );
   }
}

void comment_remove( char_data * ch, note_data * pnote )
{
   if( !ch )
   {
      bug( "{}: Null ch!", __func__ );
      return;
   }

   if( ch->pcdata->comments.empty(  ) )
   {
      bug( "{}: {} has an empty comment list already.", __func__, ch->name );
      return;
   }

   if( !pnote )
   {
      bug( "{}: Null pnote, removing comment from {}!", __func__, ch->name );
      return;
   }

   ch->pcdata->comments.remove( pnote );
   deleteptr( pnote );

   /*
    * Rewrite entire list.
    * Right, so you mean to tell me this gets called from here, which calls fwrite_char, which calls fwrite_comments...
    * ... Good GODS.. -- Xorith
    */
   ch->save(  );
}

CMDF( do_comment )
{
   std::string arg;
   note_data *pnote;
   char_data *victim;
   int vnum;

   if( ch->isnpc(  ) )
   {
      ch->print( "Mobs can't use the comment command.\r\n" );
      return;
   }

   if( !ch->desc )
   {
      bug( "{}: no descriptor", __func__ );
      return;
   }

   /*
    * Put in to prevent crashing when someone issues a comment command
    * from within the editor. -Narn 
    */
   if( ch->desc->connected == CON_EDITING )
   {
      ch->print( "You can't use the comment command from within the editor.\r\n" );
      return;
   }

   switch ( ch->substate )
   {
      default:
         break;

      case SUB_WRITING_NOTE:
         if( !ch->pcdata->pnote )
         {
            bug( "{}: note got lost?", __func__ );
            ch->print( "Your note got lost!\r\n" );
            ch->stop_editing(  );
            return;
         }
         if( ch->pcdata->dest_buf != ch->pcdata->pnote )
            bug( "{}: sub_writing_note: ch->pcdata->dest_buf != ch->pcdata->pnote", __func__ );
         ch->pcdata->pnote->text = ch->copy_buffer( );
         ch->stop_editing(  );
         return;

      case SUB_EDIT_ABORT:
         ch->print( "Aborting note...\r\n" );
         ch->substate = SUB_NONE;
         if( ch->pcdata->pnote )
            deleteptr( ch->pcdata->pnote );
         ch->pcdata->pnote = nullptr;
         return;
   }

   ch->set_color( AT_NOTE );
   argument = one_argument( argument, arg );
   smash_tilde( argument );

   if( !str_cmp( arg, "list" ) || !str_cmp( arg, "about" ) )
   {
      if( !( victim = ch->get_char_world( argument ) ) )
      {
         ch->print( "They're not logged on!\r\n" );   /* maybe fix this? */
         return;
      }

      if( victim->isnpc(  ) )
      {
         ch->print( "No comments about mobs.\r\n" );
         return;
      }

      if( victim->get_trust(  ) >= ch->get_trust(  ) )
      {
         ch->print( "You're not of the right caliber to do this...\r\n" );
         return;
      }

      if( victim->pcdata->comments.empty(  ) )
      {
         ch->print( "There are no relevant comments.\r\n" );
         return;
      }

      vnum = 0;
      for( auto* note : victim->pcdata->comments )
      {
         ++vnum;
         ch->print_fmt( "{:2}) {:<10} [{}] {}\r\n", vnum, !note->sender.empty() ? note->sender : "--Error--",
                     mini_c_time( note->date_stamp, ch->pcdata->timezone_name ), !note->subject.empty() ? note->subject : "--Error--" );
         /*
          * Brittany added date to comment list and whois with above change 
          */
      }
      return;
   }

   if( !str_cmp( arg, "read" ) )
   {
      bool fAll;

      argument = one_argument( argument, arg );
      if( !( victim = ch->get_char_world( arg ) ) )
      {
         ch->print( "They're not logged on!\r\n" );   /* maybe fix this? */
         return;
      }

      if( victim->isnpc(  ) )
      {
         ch->print( "No comments about mobs\r\n" );
         return;
      }

      if( victim->get_trust(  ) >= ch->get_trust(  ) )
      {
         ch->print( "You're not of the right caliber to do this...\r\n" );
         return;
      }

      if( victim->pcdata->comments.empty(  ) )
      {
         ch->print( "There are no relevant comments.\r\n" );
         return;
      }

      if( !str_cmp( argument, "all" ) )
         fAll = true;
      else if( is_number( argument ) )
         fAll = false;
      else
      {
         ch->print( "Read which comment?\r\n" );
         return;
      }

      vnum = 0;
      for( auto* note : victim->pcdata->comments )
      {
         ++vnum;
         if( fAll || vnum == atoi( argument.c_str(  ) ) )
         {
            note_to_char( ch, note, nullptr, 0 );
            return;
         }
      }
      ch->print( "No such comment.\r\n" );
      return;
   }

   if( !str_cmp( arg, "write" ) )
   {
      if( !ch->pcdata->pnote )
         ch->note_attach( NOTE_PLAYER );
      ch->substate = SUB_WRITING_NOTE;
      ch->pcdata->dest_buf = ch->pcdata->pnote;
      ch->set_editor_desc( "A player comment." );
      ch->start_editing( ch->pcdata->pnote->text );
      return;
   }

   if( !str_cmp( arg, "subject" ) )
   {
      if( !ch->pcdata->pnote )
         ch->note_attach( NOTE_PLAYER );
      ch->pcdata->pnote->subject = argument;
      ch->print( "Ok.\r\n" );
      return;
   }

   if( !str_cmp( arg, "clear" ) )
   {
      if( ch->pcdata->pnote )
      {
         deleteptr( ch->pcdata->pnote );
         ch->print( "Comment cleared.\r\n" );
         return;
      }
      ch->print( "You arn't working on a comment!\r\n" );
      return;
   }

   if( !str_cmp( arg, "show" ) )
   {
      if( !ch->pcdata->pnote )
      {
         ch->print( "You have no comment in progress.\r\n" );
         return;
      }
      note_to_char( ch, ch->pcdata->pnote, nullptr, 0 );
      return;
   }

   if( !str_cmp( arg, "post" ) )
   {
      if( !ch->pcdata->pnote )
      {
         ch->print( "You have no comment in progress.\r\n" );
         return;
      }

      argument = one_argument( argument, arg );
      if( !( victim = ch->get_char_world( arg ) ) )
      {
         ch->print( "They're not logged on!\r\n" );   /* maybe fix this? */
         return;
      }

      if( victim->isnpc(  ) )
      {
         ch->print( "No comments about mobs\r\n" );
         return;
      }

      if( victim->get_trust(  ) > ch->get_trust(  ) )
      {
         ch->print( "You failed, and they saw!\r\n" );
         victim->print_fmt( "{} has just tried to comment your character!\r\n", ch->name );
         return;
      }

      ch->pcdata->pnote->date_stamp = current_time;

      pnote = ch->pcdata->pnote;
      ch->pcdata->pnote = nullptr;

      victim->pcdata->comments.push_back( pnote );
      victim->save(  );
      ch->print( "Comment posted!\r\n" );
      return;
   }

   if( !str_cmp( arg, "remove" ) )
   {
      argument = one_argument( argument, arg );
      if( !( victim = ch->get_char_world( arg ) ) )
      {
         ch->print( "They're not logged on!\r\n" );   /* maybe fix this? */
         return;
      }

      if( victim->isnpc(  ) )
      {
         ch->print( "No comments about mobs\r\n" );
         return;
      }

      if( ( victim->get_trust(  ) >= ch->get_trust(  ) ) || ( ch->get_trust(  ) < LEVEL_KL ) )
      {
         ch->print( "You're not of the right caliber to do this...\r\n" );
         return;
      }

      if( !is_number( argument ) )
      {
         ch->print( "Comment remove which number?\r\n" );
         return;
      }

      vnum = 0;
      for( auto* note : victim->pcdata->comments )
      {
         ++vnum;
         if( ( LEVEL_KL <= ch->get_trust(  ) ) && ( vnum == atoi( argument.c_str(  ) ) ) )
         {
            comment_remove( victim, note );
            ch->print( "Comment removed..\r\n" );
            return;
         }
      }
      ch->print( "No such comment.\r\n" );
      return;
   }
   ch->print( "Syntax: comment <argument> <args...>\r\n" );
   ch->print( "  Where argument can equal:\r\n" );
   ch->print( "     write, subject, clear, show\r\n" );
   ch->print( "     list <player>, read <player> <#/all>, post <player>\r\n" );
   if( ch->get_trust(  ) >= LEVEL_KL )
      ch->print( "     remove <player> <#>\r\n" );
}

void pc_data::fwrite_comments( std::ofstream & stream )
{
   if( comments.empty(  ) )
      return;

   for( auto* note : comments )
   {
      stream << "#COMMENT2\n"; /* Set to COMMENT2 as to tell from older comments */
      stream << std::format( "Version      {}\n", BOARDFILEVER );
      write_note( note, stream );
   }
}

void pc_data::fread_comment( std::ifstream & stream )
{
   note_data *note = new note_data;
   std::string key;
   int file_ver = 0;

   stream >> key;
   if( key == "Version" )
      stream >> file_ver;
   read_note( note, file_ver, stream );
   comments.push_back( note );
}

/* Function kept for backwards compatibility */
void pc_data::fread_old_comment( std::ifstream & stream )
{
   note_data *pcnote;

   log_string( "Starting comment conversion..." );
   std::string key;
   while( stream >> key )
   {
      pcnote = new note_data;

      if( key == "sender" )
         fread_string( pcnote->sender, stream );

      if( key == "date" )
         fread_to_eol( stream );

      if( key == "to" )
         fread_to_eol( stream );

      if( "subject" )
         fread_string( pcnote->subject, stream );

      if( key == "text" )
         fread_string( pcnote->text, stream );

      if( pnote->sender.empty() )
         pcnote->sender = "None";

      if( pnote->subject.empty() )
         pcnote->subject = "Error: Subject not found";

      if( pnote->text.empty() )
         pcnote->text = "Error: Comment text not found.";

      pcnote->date_stamp = current_time;
      pcnote->type = NOTE_PLAYER;
      comments.push_back( pcnote );
      return;
   }
}
