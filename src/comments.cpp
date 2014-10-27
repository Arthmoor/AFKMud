/****************************************************************************
 *                   ^     +----- |  / ^     ^ |     | +-\                  *
 *                  / \    |      | /  |\   /| |     | |  \                 *
 *                 /   \   +---   |<   | \ / | |     | |  |                 *
 *                /-----\  |      | \  |  v  | |     | |  /                 *
 *               /       \ |      |  \ |     | +-----+ +-/                  *
 ****************************************************************************
 * AFKMud Copyright 1997-2009 by Roger Libiez (Samson),                     *
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
 * Comments: 'notes' attached to players to keep track of outstanding       * 
 *           and problem players.  -haus 6/25/1995                          * 
 ****************************************************************************/

#include "mud.h"
#include "descriptor.h"
#include "boards.h"

void fwrite_note( note_data *, FILE * );
note_data *read_note( FILE * );
void note_to_char( char_data *, note_data *, board_data *, short );
char *mini_c_time( time_t, int );

void pc_data::free_comments(  )
{
   list < note_data * >::iterator note;

   for( note = comments.begin(  ); note != comments.end(  ); )
   {
      note_data *comment = *note;

      comments.remove( comment );
      deleteptr( comment );
   }
}

void comment_remove( char_data * ch, note_data * pnote )
{
   if( !ch )
   {
      bug( "%s: Null ch!", __FUNCTION__ );
      return;
   }

   if( ch->pcdata->comments.empty(  ) )
   {
      bug( "%s: %s has an empty comment list already.", __FUNCTION__, ch->name );
      return;
   }

   if( !pnote )
   {
      bug( "%s: Null pnote, removing comment from %s!", __FUNCTION__, ch->name );
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
   string arg;
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
      bug( "%s: no descriptor", __FUNCTION__ );
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
            bug( "%s: note got lost?", __FUNCTION__ );
            ch->print( "Your note got lost!\r\n" );
            ch->stop_editing(  );
            return;
         }
         if( ch->pcdata->dest_buf != ch->pcdata->pnote )
            bug( "%s: sub_writing_note: ch->pcdata->dest_buf != ch->pcdata->pnote", __FUNCTION__ );
         DISPOSE( ch->pcdata->pnote->text );
         ch->pcdata->pnote->text = ch->copy_buffer( false );
         ch->stop_editing(  );
         return;

      case SUB_EDIT_ABORT:
         ch->print( "Aborting note...\r\n" );
         ch->substate = SUB_NONE;
         if( ch->pcdata->pnote )
            deleteptr( ch->pcdata->pnote );
         ch->pcdata->pnote = NULL;
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
      list < note_data * >::iterator inote;
      for( inote = victim->pcdata->comments.begin(  ); inote != victim->pcdata->comments.end(  ); ++inote )
      {
         note_data *note = *inote;

         ++vnum;
         ch->printf( "%2d) %-10s [%s] %s\r\n", vnum, note->sender ? note->sender : "--Error--",
                     mini_c_time( note->date_stamp, -1 ), note->subject ? note->subject : "--Error--" );
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
      list < note_data * >::iterator inote;
      for( inote = victim->pcdata->comments.begin(  ); inote != victim->pcdata->comments.end(  ); ++inote )
      {
         note_data *note = *inote;

         ++vnum;
         if( fAll || vnum == atoi( argument.c_str(  ) ) )
         {
            note_to_char( ch, note, NULL, 0 );
            return;
         }
      }
      ch->print( "No such comment.\r\n" );
      return;
   }

   if( !str_cmp( arg, "write" ) )
   {
      if( !ch->pcdata->pnote )
         ch->note_attach(  );
      ch->substate = SUB_WRITING_NOTE;
      ch->pcdata->dest_buf = ch->pcdata->pnote;
      if( !ch->pcdata->pnote->text || ch->pcdata->pnote->text[0] == '\0' )
         ch->pcdata->pnote->text = str_dup( "" );
      ch->set_editor_desc( "A player comment." );
      ch->start_editing( ch->pcdata->pnote->text );
      return;
   }

   if( !str_cmp( arg, "subject" ) )
   {
      if( !ch->pcdata->pnote )
         ch->note_attach(  );
      DISPOSE( ch->pcdata->pnote->subject );
      ch->pcdata->pnote->subject = str_dup( argument.c_str(  ) );
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
      note_to_char( ch, ch->pcdata->pnote, NULL, 0 );
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
         victim->printf( "%s has just tried to comment your character!\r\n", ch->name );
         return;
      }

      ch->pcdata->pnote->date_stamp = current_time;

      pnote = ch->pcdata->pnote;
      ch->pcdata->pnote = NULL;

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
      list < note_data * >::iterator inote;
      for( inote = victim->pcdata->comments.begin(  ); inote != victim->pcdata->comments.end(  ); ++inote )
      {
         note_data *note = *inote;

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

void pc_data::fwrite_comments( FILE * fp )
{
   list < note_data * >::iterator inote;

   if( comments.empty(  ) )
      return;

   for( inote = comments.begin(  ); inote != comments.end(  ); ++inote )
   {
      note_data *note = *inote;

      fprintf( fp, "%s", "#COMMENT2\n" ); /* Set to COMMENT2 as to tell from older comments */
      fwrite_note( note, fp );
   }
}

void pc_data::fread_comment( FILE * fp )
{
   note_data *pcnote;

   pcnote = read_note( fp );
   comments.push_back( pcnote );
}

/* Function kept for backwards compatibility */
void pc_data::fread_old_comment( FILE * fp )
{
   note_data *pcnote;

   log_string( "Starting comment conversion..." );
   for( ;; )
   {
      char letter;

      do
      {
         letter = getc( fp );
         if( feof( fp ) )
         {
            FCLOSE( fp );
            return;
         }
      }
      while( isspace( letter ) );
      ungetc( letter, fp );

      pcnote = new note_data;
      pcnote->rlist.clear(  );
      init_memory( &pcnote->parent, &pcnote->expire, sizeof( pcnote->expire ) );

      if( !str_cmp( fread_word( fp ), "sender" ) )
         pcnote->sender = fread_string( fp );

      if( !str_cmp( fread_word( fp ), "date" ) )
         fread_to_eol( fp );

      if( !str_cmp( fread_word( fp ), "to" ) )
         fread_to_eol( fp );

      if( !str_cmp( fread_word( fp ), "subject" ) )
         pcnote->subject = fread_string_nohash( fp );

      if( !str_cmp( fread_word( fp ), "text" ) )
         pcnote->text = fread_string_nohash( fp );

      if( !pnote->sender )
         pcnote->sender = STRALLOC( "None" );

      if( !pnote->subject )
         pcnote->subject = str_dup( "Error: Subject not found" );

      if( !pnote->text )
         pcnote->text = str_dup( "Error: Comment text not found." );

      pcnote->date_stamp = current_time;
      comments.push_back( pcnote );
      return;
   }
}
