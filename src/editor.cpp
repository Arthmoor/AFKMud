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
 *                           Line Editor Functions                          *
 ****************************************************************************/

#include "mud.h"
#include "descriptor.h"

constexpr int max_buf_lines = 60;

struct editor_data
{
   editor_data(  );
   ~editor_data(  );

   std::string desc;
   char line[max_buf_lines][81]{0};
   short numlines = 0;
   short on_line = 0;
   short size = 0;
};

editor_data::editor_data(  )
{
}

editor_data::~editor_data(  )
{
}

void editor_print_info( char_data * ch, editor_data * edd, short max_size )
{
   ch->print_fmt( "Currently editing: {}\r\n"
               "Total lines: {:4}   On line:  {:4}\r\n"
               "Buffer size: {:4}   Max size: {:4}\r\n", !edd->desc.empty(  ) ? edd->desc : "(Null description)", edd->numlines, edd->on_line, edd->size, max_size );
}

void char_data::set_editor_desc( std::string_view new_desc )
{
   if( !pcdata->editor )
      return;

   pcdata->editor->desc = new_desc;
}

void char_data::stop_editing(  )
{
   deleteptr( pcdata->editor );
   pcdata->dest_buf = nullptr;
   pcdata->spare_ptr = nullptr;
   substate = SUB_NONE;

   if( !desc )
   {
      bug( "Fatal: {}: no desc", __func__ );
      return;
   }
   desc->connected = CON_PLAYING;
}

void char_data::start_editing( const std::string & data )
{
   editor_data *edit;
   short lines, size, lpos;

   if( !desc )
   {
      bug( "Fatal: {}: no desc", __func__ );
      return;
   }
   if( substate == SUB_RESTRICTED )
      bug( "NOT GOOD: {}: ch->substate == SUB_RESTRICTED", __func__ );

   set_color( AT_GREEN );
   print( "Begin entering your text now (/? = help /s = save /c = clear /l = list)\r\n" );
   print( "-----------------------------------------------------------------------\r\n> " );
   if( pcdata->editor )
      stop_editing(  );

   edit = new editor_data;
   edit->numlines = 0;
   edit->on_line = 0;
   edit->size = 0;
   size = 0;
   lpos = 0;
   lines = 0;
   if( !data.empty(  ) )
   {
      for( ;; )
      {
         char c = data[size++];
         if( c == '\0' )
         {
            edit->line[lines][lpos] = '\0';
            break;
         }
         else if( c == '\r' );
         else if( c == '\n' || lpos > 79 )
         {
            edit->line[lines][lpos] = '\0';
            ++lines;
            lpos = 0;
         }
         else
            edit->line[lines][lpos++] = c;

         if( lines >= max_buf_lines )
         {
            lines = max_buf_lines - 1;
            edit->line[lines][80] = '\0';
            break;
         }

         if( size > MSL )
         {
            edit->line[lines][lpos] = '\0';
            break;
         }
      }
   }

   if( lpos > 0 && lpos < 78 && lines < max_buf_lines )
   {
      edit->line[lines][lpos] = '~';
      edit->line[lines][lpos + 1] = '\0';
      ++lines;
      lpos = 0;
   }
   edit->numlines = lines;
   edit->size = size;
   edit->on_line = lines;
   pcdata->editor = edit;
   desc->connected = CON_EDITING;
}

std::string char_data::copy_buffer(  )
{
   if( !pcdata->editor )
   {
      bug( "{}: null editor", __func__ );
      return "";
   }

   std::ostringstream oss;

   for( int i = 0; i < pcdata->editor->numlines; ++i )
   {
      std::string_view line{pcdata->editor->line[i]};

      if( !line.empty() && line.back() == '~' )
         oss << line.substr( 0, line.size() - 1 );
      else
         oss << line << '\n';
   }
   return oss.str();
}

/*
 * Simple but nice and handy line editor. - Thoric
 */
void char_data::edit_buffer( const std::string & argument )
{
   descriptor_data *d;
   editor_data *edit;
   std::string cmd;
   char buf[MIL];
   int line;
   bool esave = false;

   if( !( d = desc ) )
   {
      print( "You have no descriptor.\r\n" );
      return;
   }

   if( d->connected != CON_EDITING )
   {
      print( "You can't do that!\r\n" );
      bug( "{}: d->connected != CON_EDITING", __func__ );
      return;
   }

   if( substate <= SUB_PAUSE )
   {
      print( "You can't do that!\r\n" );
      bug( "{}: illegal ch->substate ({})", __func__, substate );
      d->connected = CON_PLAYING;
      return;
   }

   if( !pcdata->editor )
   {
      print( "You can't do that!\r\n" );
      bug( "{}: null editor", __func__ );
      d->connected = CON_PLAYING;
      return;
   }

   edit = pcdata->editor;

   if( argument[0] == '/' || argument[0] == '\\' )
   {
      one_argument( argument, cmd );
      cmd = cmd.substr( 1, cmd.length(  ) );

      if( !str_cmp( cmd, "?" ) )
      {
         print( "Editing commands\r\n---------------------------------\r\n" );
         print( "/l              list buffer\r\n" );
         print( "/c              clear buffer\r\n" );
         print( "/d [line]       delete line\r\n" );
         print( "/g <line>       goto line\r\n" );
         print( "/i <line>       insert line\r\n" );
         print( "/f <format>     format text in buffer\r\n" );
         print( "/r <old> <new>  global replace\r\n" );
         print( "/a              abort editing\r\n" );
         print( "/p              show buffer information\r\n" );

         if( get_trust(  ) > LEVEL_IMMORTAL )
            print( "/! <command>    execute command (do not use another editing command)\r\n" );
         print( "/s              save buffer\r\n\r\n> " );
         return;
      }

      if( !str_cmp( cmd, "c" ) )
      {
         delete edit;
         edit = new editor_data;

         print( "Buffer cleared.\r\n> " );
         return;
      }

      if( !str_cmp( cmd, "r" ) )
      {
         std::string word1, word2, sptr;

         sptr = one_argument( argument, word1 );
         sptr = one_argument( sptr, word1 );
         sptr = one_argument( sptr, word2 );
         if( word1.empty(  ) || word2.empty(  ) )
         {
            print( "Need word to replace, and replacement.\r\n> " );
            return;
         }

         /*
          * Changed to a case-sensitive version of string compare --Cynshard 
          */
         if( word1 == word2 )
         {
            print( "Done.\r\n> " );
            return;
         }

         print_fmt( "Replacing all occurrences of {} with {}...\r\n", word1, word2 );
         for( int x = 0; x < edit->numlines; ++x )
         {
            std::string lwptr = edit->line[x];
            string_replace( lwptr, word1, word2 );

            int lineln = strlcpy( buf, lwptr.c_str(  ), MIL );
            if( lineln > 79 )
               buf[80] = '\0';

            strlcpy( edit->line[x], buf, 81 );
         }
         print_fmt( "Found and replaced \"{}\" with \"{}\".\r\n> ", word1, word2 );
         return;
      }

      /*
       * added format command - shogar 
       */
      /*
       * This has been redone to be more efficient, and to make format
       * start at beginning of buffer, not whatever line you happened
       * to be on, at the time.   
       */
      if( !str_cmp( cmd, "f" ) )
      {
         char temp_buf[MSL + max_buf_lines];
         int old_p, end_mark, p = 0;

         pager( "Reformatting...\r\n" );

         for( int x = 0; x < edit->numlines; ++x )
         {
            strlcpy( temp_buf + p, edit->line[x], MSL + max_buf_lines - p );
            p += strlen( edit->line[x] );
            temp_buf[p] = ' ';
            ++p;
         }

         temp_buf[p] = '\0';
         end_mark = p;
         p = 75;
         old_p = 0;
         edit->on_line = 0;
         edit->numlines = 0;

         while( old_p < end_mark )
         {
            while( temp_buf[p] != ' ' && p > old_p )
               --p;

            if( p == old_p )
               p += 75;

            if( p > end_mark )
               p = end_mark;

            int ep = 0;
            for( int x = old_p; x < p; ++x )
            {
               edit->line[edit->on_line][ep] = temp_buf[x];
               ++ep;
            }
            edit->line[edit->on_line][ep] = '\0';

            ++edit->on_line;
            ++edit->numlines;

            old_p = p + 1;
            p += 75;
         }
         pager( "Reformatting done.\r\n> " );
         return;
      }

      if( !str_cmp( cmd, "p" ) )
      {
         editor_print_info( this, edit, max_buf_lines );
         return;
      }

      if( !str_cmp( cmd, "i" ) )
      {
         if( edit->numlines >= max_buf_lines )
            print( "Your buffer is full.\r\n> " );
         else
         {
            if( argument[2] == ' ' )
               line = std::stoi( argument.substr(2) ) - 1;
            else
               line = edit->on_line;
            if( line < 0 )
               line = edit->on_line;
            if( line < 0 || line > edit->numlines )
               print( "Out of range.\r\n> " );
            else
            {
               for( int x = ++edit->numlines; x > line; --x )
                  strlcpy( edit->line[x], edit->line[x - 1], 81 );
               strlcpy( edit->line[line], "", 81 );
               print( "Line inserted.\r\n> " );
            }
         }
         return;
      }

      if( !str_cmp( cmd, "d" ) )
      {
         if( edit->numlines == 0 )
            print( "Buffer is empty.\r\n> " );
         else
         {
            if( argument[2] == ' ' )
               line = std::stoi( argument.substr(2) ) - 1;
            else
               line = edit->on_line;
            if( line < 0 )
               line = edit->on_line;
            if( line < 0 || line > edit->numlines )
               print( "Out of range.\r\n> " );
            else
            {
               if( line == 0 && edit->numlines == 1 )
               {
                  delete edit;
                  edit = new editor_data;

                  print( "Line deleted.\r\n> " );
                  return;
               }
               for( int x = line; x < ( edit->numlines - 1 ); ++x )
                  strlcpy( edit->line[x], edit->line[x + 1], 81 );
               strlcpy( edit->line[edit->numlines--], "", 81 );
               if( edit->on_line > edit->numlines )
                  edit->on_line = edit->numlines;
               print( "Line deleted.\r\n> " );
            }
         }
         return;
      }

      if( !str_cmp( cmd, "g" ) )
      {
         if( edit->numlines == 0 )
            print( "Buffer is empty.\r\n> " );
         else
         {
            if( argument[2] == ' ' )
               line = std::stoi( argument.substr(2) ) - 1;
            else
            {
               print( "Goto what line?\r\n> " );
               return;
            }
            if( line < 0 )
               line = edit->on_line;
            if( line < 0 || line > edit->numlines )
               print( "Out of range.\r\n> " );
            else
            {
               edit->on_line = line;
               print_fmt( "(On line {})\r\n> ", line + 1 );
            }
         }
         return;
      }

      if( !str_cmp( cmd, "l" ) )
      {
         if( edit->numlines == 0 )
            print( "Buffer is empty.\r\n> " );
         else
         {
            print( "------------------\r\n" );
            for( int x = 0; x < edit->numlines; ++x )
            {
               /*
                * Quixadhal - We cannot use ch_printf here, or we can't see
                * * what color codes exist in the strings!
                */
               std::string tmpline = std::format( "{:2}> {}\r\n", x + 1, edit->line[x] );
               desc->write_to_buffer( tmpline );
            }
            print( "------------------\r\n> " );
         }
         return;
      }

      if( !str_cmp( cmd, "a" ) )
      {
         if( !last_cmd )
         {
            stop_editing(  );
            return;
         }
         d->connected = CON_PLAYING;
         substate = SUB_EDIT_ABORT;
         deleteptr( pcdata->editor );
         print( "Done.\r\n" );
         pcdata->dest_buf = nullptr;
         pcdata->spare_ptr = nullptr;
         ( *last_cmd ) ( this, "" );
         return;
      }

      if( get_trust(  ) > LEVEL_IMMORTAL && !str_cmp( cmd, "!" ) )
      {
         DO_FUN *lastcmd;
         int Csubstate = substate;

         lastcmd = last_cmd;
         substate = SUB_RESTRICTED;
         interpret( this, argument.substr( 3, argument.length(  ) ) );
         substate = Csubstate;
         last_cmd = lastcmd;
         set_color( AT_GREEN );
         print( "\r\n> " );
         return;
      }

      if( !str_cmp( cmd, "s" ) )
      {
         d->connected = CON_PLAYING;

         if( !last_cmd )
            return;

         ( *last_cmd ) ( this, "" );
         return;
      }
   }

   if( edit->size + argument.length(  ) + 1 >= MSL - 2 )
      print( "Your buffer is full.\r\n" );
   else
   {
      if( argument.length(  ) > 80 )
      {
         strlcpy( buf, argument.c_str(  ), 80 );
         print( "(Long line trimmed)\r\n> " );
      }
      else
         strlcpy( buf, argument.c_str(  ), 80 );
      strlcpy( edit->line[edit->on_line++], buf, 81 );
      while( edit->on_line > edit->numlines )
         ++edit->numlines;
      if( edit->numlines >= max_buf_lines )
      {
         edit->numlines = max_buf_lines;
         print( "Your buffer is full.\r\n" );
         esave = true;
      }
   }

   if( esave )
   {
      d->connected = CON_PLAYING;

      if( !last_cmd )
         return;

      ( *last_cmd ) ( this, "" );
      return;
   }
   print( "> " );
}
