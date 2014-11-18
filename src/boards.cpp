/****************************************************************************
 *                   ^     +----- |  / ^     ^ |     | +-\                  *
 *                  / \    |      | /  |\   /| |     | |  \                 *
 *                 /   \   +---   |<   | \ / | |     | |  |                 *
 *                /-----\  |      | \  |  v  | |     | |  /                 *
 *               /       \ |      |  \ |     | +-----+ +-/                  *
 ****************************************************************************
 * AFKMud Copyright 1997-2015 by Roger Libiez (Samson),                     *
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
 *                     Completely Revised Boards Module                     *
 ****************************************************************************
 * Revised by:   Xorith                                                     *
 * Last Updated: 09/21/07                                                   *
 ****************************************************************************/

#include <sys/stat.h>
#if defined(WIN32)
#include <unistd.h>
#endif
#include "mud.h"
#include "boards.h"
#include "clans.h"
#include "deity.h"
#include "descriptor.h"
#include "objindex.h"
#include "realms.h"
#include "roomindex.h"

list < board_data * >bdlist;
list < project_data * >projlist;

char *mini_c_time( time_t, int );
void check_boards(  );

/* Global */
time_t board_expire_time_t;

// Changed 'backup_pruned' to 'backup', to be more sane. --Xorith (9/21/07)
// There's no conversion code, so if you want backups - set the flag again. -X
const char *board_flags[] = {
   "r1", "backup", "private", "announce"
};

const char *note_flags[] = {
   "r1", "sticky", "closed", "hidden"
};

int get_note_flag( const string & note_flag )
{
   for( int x = 0; x < MAX_NOTE_FLAGS; ++x )
      if( !str_cmp( note_flag, note_flags[x] ) )
         return x;
   return -1;
}

int get_board_flag( const string & board_flag )
{
   for( int x = 0; x < MAX_BOARD_FLAGS; ++x )
      if( !str_cmp( board_flag, board_flags[x] ) )
         return x;
   return -1;
}

/* This is a simple function that keeps a string within a certain length. -- Xorith */
string print_lngstr( const string & src, size_t size )
{
   string rstring;

   if( src.length(  ) > size )
   {
      rstring = src.substr( 0, size - 2 );
      rstring.append( "..." );
   }
   else
      rstring = src;

   return rstring;
}

board_chardata::board_chardata(  )
{
   init_memory( &last_read, &alert, sizeof( alert ) );
}

board_chardata::~board_chardata(  )
{
}

note_data::note_data(  )
{
   init_memory( &parent, &expire, sizeof( expire ) );
   rlist.clear(  );
}

note_data::~note_data(  )
{
   list < note_data * >::iterator rp;

   DISPOSE( text );
   DISPOSE( subject );
   STRFREE( to_list );
   STRFREE( sender );

   for( rp = rlist.begin(  ); rp != rlist.end(  ); )
   {
      note_data *reply = *rp;
      ++rp;

      rlist.remove( reply );
      deleteptr( reply );
   }
}

board_data::board_data(  )
{
   init_memory( &flags, &expire, sizeof( expire ) );
   nlist.clear(  );
}

board_data::~board_data(  )
{
   list < note_data * >::iterator note;

   STRFREE( name );
   STRFREE( readers );
   STRFREE( posters );
   STRFREE( moderators );
   DISPOSE( desc );
   STRFREE( group );
   DISPOSE( filename );

   for( note = nlist.begin(  ); note != nlist.end(  ); )
   {
      note_data *pnote = *note;
      ++note;

      nlist.remove( pnote );
      deleteptr( pnote );
   }
   bdlist.remove( this );
}

void pc_data::free_pcboards(  )
{
   list < board_chardata * >::iterator bd;

   for( bd = boarddata.begin(  ); bd != boarddata.end(  ); )
   {
      board_chardata *chbd = *bd;
      ++bd;

      boarddata.remove( chbd );
      deleteptr( chbd );
   }
}

bool can_remove( char_data * ch, board_data * board )
{
   /*
    * If there's no group on the board, or if the character is Immortal, use the following... 
    */
   if( !board->group || board->group[0] == '\0' || ch->is_immortal(  ) )
   {
      if( ch->get_trust(  ) >= board->remove_level )
         return true;

      if( board->moderators && board->moderators[0] != '\0' )
      {
         if( hasname( board->moderators, ch->name ) )
            return true;
      }
   }
   /*
    * else, make sure they're a member of the group. this means no outside mortal moderators... 
    */

   // Changed this to allow moderators for deity-set boards. --Xorith (9/21/07)
   else if( ( ch->pcdata->clan && hasname( board->group, ch->pcdata->clan->name ) ) || ( ch->pcdata->deity && hasname( board->group, ch->pcdata->deity->name ) ) )
   {
      if( ch->pcdata->clan && hasname( board->group, ch->pcdata->clan->name ) && ( IS_LEADER( ch ) || IS_NUMBER1( ch ) || IS_NUMBER2( ch ) ) )
         return true;

      if( ch->get_trust(  ) >= board->remove_level )
         return true;

      if( board->moderators && board->moderators[0] != '\0' )
      {
         if( hasname( board->moderators, ch->name ) )
            return true;
      }
   }

   /*
    * If I ever put in some sort of deity priest code, similar check here as above 
    */
   return false;
}

bool can_post( char_data * ch, board_data * board )
{
   /*
    * It makes sense to me that if you are a moderator then you can automatically read/post 
    */
   if( can_remove( ch, board ) )
      return true;

   /*
    * If there's no group on the board, or if the character is Immortal, use the following... 
    */
   if( !board->group || board->group[0] == '\0' || ch->is_immortal(  ) )
   {
      if( ch->get_trust(  ) >= board->post_level )
         return true;

      if( board->posters && board->posters[0] != '\0' )
      {
         if( hasname( board->posters, ch->name ) )
            return true;
      }
   }
   /*
    * else, make sure they're a member of the group. this means no outside mortal posters... 
    */
   else if( ( ch->pcdata->clan && hasname( board->group, ch->pcdata->clan->name ) ) || ( ch->pcdata->deity && hasname( board->group, ch->pcdata->deity->name ) ) )
   {
      if( ch->get_trust(  ) >= board->post_level )
         return true;

      if( board->posters && board->posters[0] != '\0' )
      {
         if( hasname( board->posters, ch->name ) )
            return true;
      }
   }
   return false;
}

bool can_read( char_data * ch, board_data * board )
{
   /*
    * It makes sense to me that if you are a moderator then you can automatically read/post 
    */
   if( can_remove( ch, board ) )
      return true;

   /*
    * If there's no group on the board, or if the character is Immortal, use the following... 
    */
   if( !board->group || board->group[0] == '\0' || ch->is_immortal(  ) )
   {
      if( ch->get_trust(  ) >= board->read_level )
         return true;

      if( board->readers && board->readers[0] != '\0' )
      {
         if( hasname( board->readers, ch->name ) )
            return true;
      }
   }
   /*
    * else, make sure they're a member of the group. this means no outside mortal readers... 
    */
   else if( ( ch->pcdata->clan && hasname( board->group, ch->pcdata->clan->name ) ) || ( ch->pcdata->deity && hasname( board->group, ch->pcdata->deity->name ) ) )
   {
      if( ch->get_trust(  ) >= board->read_level )
         return true;

      if( board->readers && board->readers[0] != '\0' )
      {
         if( hasname( board->readers, ch->name ) )
            return true;
      }
   }
   return false;
}

bool is_note_to( char_data * ch, note_data * pnote )
{
   if( pnote->to_list && pnote->to_list[0] != '\0' )
   {
      if( hasname( pnote->to_list, "all" ) )
         return true;

      if( ch->is_immortal(  ) && hasname( pnote->to_list, "immortal" ) )
         return true;

      if( hasname( pnote->to_list, ch->name ) )
         return true;
   }
   return false;
}

/* This will get a board by object. This will not get a global board
   as global boards are noted with a 0 objvnum */
board_data *get_board_by_obj( obj_data * obj )
{
   list < board_data * >::iterator bd;

   for( bd = bdlist.begin(  ); bd != bdlist.end(  ); ++bd )
   {
      board_data *board = *bd;

      if( board->objvnum == obj->pIndexData->vnum )
         return board;
   }
   return NULL;
}

/* Gets a board by name, or a number. The number should be the board # given in do_board_list.
   If ch == NULL, then it'll perform the search without checks. Otherwise, it'll perform the
   search and weed out boards that the ch can't view remotely. */
board_data *get_board( char_data * ch, const string & name )
{
   list < board_data * >::iterator bd;
   int count = 1;

   for( bd = bdlist.begin(  ); bd != bdlist.end(  ); ++bd )
   {
      board_data *board = *bd;

      if( ch != NULL )
      {
         if( !can_read( ch, board ) )
            continue;
         if( board->objvnum > 0 && !can_remove( ch, board ) && !ch->is_immortal(  ) )
            continue;
      }
      if( count == atoi( name.c_str(  ) ) )
         return board;
      if( !str_cmp( board->name, name ) )
         return board;
      ++count;
   }
   return NULL;
}

/* This will find a board on an object in the character's current room */
board_data *find_board( char_data * ch )
{
   list < obj_data * >::iterator iobj;

   for( iobj = ch->in_room->objects.begin(  ); iobj != ch->in_room->objects.end(  ); ++iobj )
   {
      obj_data *obj = *iobj;
      board_data *board;

      if( ( board = get_board_by_obj( obj ) ) != NULL )
         return board;
   }
   return NULL;
}

board_chardata *get_chboard( char_data * ch, const string & board_name )
{
   list < board_chardata * >::iterator bd;

   for( bd = ch->pcdata->boarddata.begin(  ); bd != ch->pcdata->boarddata.end(  ); ++bd )
   {
      board_chardata *board = *bd;

      if( !str_cmp( board->board_name, board_name ) )
         return board;
   }
   return NULL;
}

board_chardata *create_chboard( char_data * ch, const string & board_name )
{
   board_chardata *chboard;

   if( ( chboard = get_chboard( ch, board_name ) ) )
      return chboard;

   chboard = new board_chardata;
   chboard->board_name = board_name;
   chboard->last_read = 0;
   chboard->alert = false;
   ch->pcdata->boarddata.push_back( chboard );
   return chboard;
}

void char_data::note_attach(  )
{
   note_data *pcnote;

   if( pcdata->pnote )
   {
      bug( "%s: ch->pcdata->pnote already exsists!", __func__ );
      return;
   }

   pcnote = new note_data;
   pcnote->sender = QUICKLINK( name );
   pcdata->pnote = pcnote;
}

void free_boards( void )
{
   list < board_data * >::iterator bd;

   for( bd = bdlist.begin(  ); bd != bdlist.end(  ); )
   {
      board_data *board = *bd;
      ++bd;

      deleteptr( board );
   }
}

const int BOARDFILEVER = 1;
void write_boards( void )
{
   list < board_data * >::iterator bd;
   FILE *fpout;

   if( !( fpout = fopen( BOARD_DIR BOARD_FILE, "w" ) ) )
   {
      perror( BOARD_DIR BOARD_FILE );
      bug( "%s: Unable to open %s%s for writing!", __func__, BOARD_DIR, BOARD_FILE );
      return;
   }

   for( bd = bdlist.begin(  ); bd != bdlist.end(  ); )
   {
      board_data *board = *bd;
      ++bd;

      if( !board->name )
      {
         bug( "%s: Board with a null name! Destroying...", __func__ );
         deleteptr( board );
         continue;
      }
      fprintf( fpout, "FileVer     %d\n", BOARDFILEVER );
      fprintf( fpout, "Name        %s~\n", board->name );
      fprintf( fpout, "Filename    %s~\n", board->filename );
      if( board->desc )
         fprintf( fpout, "Desc        %s~\n", board->desc );
      fprintf( fpout, "ObjVnum     %d\n", board->objvnum );
      fprintf( fpout, "Expire      %d\n", board->expire );
      fprintf( fpout, "Flags       %s~\n", bitset_string( board->flags, board_flags ) );
      if( board->readers )
         fprintf( fpout, "Readers     %s~\n", board->readers );
      if( board->posters )
         fprintf( fpout, "Posters     %s~\n", board->posters );
      if( board->moderators )
         fprintf( fpout, "Moderators  %s~\n", board->moderators );
      if( board->group )
         fprintf( fpout, "Group       %s~\n", board->group );
      fprintf( fpout, "ReadLevel   %d\n", board->read_level );
      fprintf( fpout, "PostLevel   %d\n", board->post_level );
      fprintf( fpout, "RemoveLevel %d\n", board->remove_level );
      fprintf( fpout, "%s\n", "#END" );
   }
   FCLOSE( fpout );
}

void fwrite_reply( note_data * pnote, FILE * fpout )
{
   fprintf( fpout, "Reply-Sender         %s~\n", pnote->sender );
   fprintf( fpout, "Reply-To             %s~\n", pnote->to_list );
   fprintf( fpout, "Reply-Subject        %s~\n", pnote->subject );
   fprintf( fpout, "Reply-DateStamp      %ld\n", pnote->date_stamp );
   fprintf( fpout, "Reply-Flags          %s~\n", bitset_string( pnote->flags, note_flags ) );
   fprintf( fpout, "Reply-Text           %s~\n", pnote->text );
   fprintf( fpout, "%s\n", "Reply-End" );
}

void fwrite_note( note_data * pnote, FILE * fpout )
{
   if( !pnote )
      return;

   if( !pnote->sender )
   {
      bug( "%s: Called on a note without a valid sender!", __func__ );
      return;
   }

   fprintf( fpout, "Sender         %s~\n", pnote->sender );
   if( pnote->to_list )
      fprintf( fpout, "To             %s~\n", pnote->to_list );
   if( pnote->subject )
      fprintf( fpout, "Subject        %s~\n", pnote->subject );
   fprintf( fpout, "DateStamp      %ld\n", pnote->date_stamp );
   fprintf( fpout, "Flags          %s~\n", bitset_string( pnote->flags, note_flags ) );
   if( pnote->to_list ) /* Comments and Project Logs do not use to_list or Expire */
      fprintf( fpout, "Expire         %d\n", pnote->expire );
   if( pnote->text )
      fprintf( fpout, "Text           %s~\n", pnote->text );
   if( pnote->to_list ) /* comments and projects should not have replies */
   {
      int count = 0;
      list < note_data * >::iterator rp;
      for( rp = pnote->rlist.begin(  ); rp != pnote->rlist.end(  ); )
      {
         note_data *reply = *rp;
         ++rp;

         if( !reply->sender || !reply->to_list || !reply->subject || !reply->text )
         {
            bug( "%s: Destroying a buggy reply on note '%s'!", __func__, pnote->subject );
            pnote->rlist.remove( reply );
            --pnote->reply_count;
            deleteptr( reply );
            continue;
         }
         if( count == MAX_REPLY )
            break;
         fprintf( fpout, "%s\n", "Reply" );
         fwrite_reply( reply, fpout );
         ++count;
      }
   }
   fprintf( fpout, "%s\n", "#END" );
}

void write_board( board_data * board )
{
   FILE *fp;
   char filename[256];

   snprintf( filename, 256, "%s%s.board", BOARD_DIR, board->filename );
   if( !( fp = fopen( filename, "w" ) ) )
   {
      perror( filename );
      bug( "%s: Error opening %s! Board NOT saved!", __func__, filename );
      return;
   }

   list < note_data * >::iterator note;
   for( note = board->nlist.begin(  ); note != board->nlist.end(  ); )
   {
      note_data *pnote = *note;
      ++note;

      if( !pnote->sender || !pnote->to_list || !pnote->subject || !pnote->text )
      {
         bug( "%s: Destroying a buggy note on the %s board!", __func__, board->name );
         board->nlist.remove( pnote );
         --board->msg_count;
         deleteptr( pnote );
         continue;
      }
      fwrite_note( pnote, fp );
   }
   FCLOSE( fp );
}

void note_remove( board_data * board, note_data * pnote )
{
   if( !board || !pnote )
   {
      bug( "%s: null %s variable.", __func__, board ? "pnote" : "board" );
      return;
   }

   if( pnote->parent )
   {
      pnote->parent->rlist.remove( pnote );
      --pnote->parent->reply_count;
   }
   else
   {
      board->nlist.remove( pnote );
      --board->msg_count;
   }
   deleteptr( pnote );
   write_board( board );
}

board_data *read_board( FILE * fp )
{
   board_data *board;
   int file_ver = 0;
   char letter;

   do
   {
      letter = getc( fp );
      if( feof( fp ) )
      {
         FCLOSE( fp );
         return NULL;
      }
   }
   while( isspace( letter ) );

   ungetc( letter, fp );

   board = new board_data;

   for( ;; )
   {
      const char *word = ( feof( fp ) ? "#END" : fread_word( fp ) );

      if( word[0] == '\0' )
      {
         bug( "%s: EOF encountered reading file!", __func__ );
         word = "#END";
      }

      switch ( UPPER( word[0] ) )
      {
         default:
            bug( "%s: no match: %s", __func__, word );
            fread_to_eol( fp );
            break;

         case '*':
            fread_to_eol( fp );
            break;

         case 'D':
            KEY( "Desc", board->desc, fread_string_nohash( fp ) );
            break;

         case 'E':
            KEY( "Expire", board->expire, fread_number( fp ) );
            break;

         case 'F':
            KEY( "Filename", board->filename, fread_string_nohash( fp ) );
            KEY( "FileVer", file_ver, fread_number( fp ) );
            if( !str_cmp( word, "Flags" ) )
            {
               if( file_ver < 1 )
                  board->flags = fread_long( fp );
               else
                  flag_set( fp, board->flags, board_flags );
               break;
            }
            break;

         case 'G':
            KEY( "Group", board->group, fread_string( fp ) );
            break;

         case 'M':
            KEY( "Moderators", board->moderators, fread_string( fp ) );
            break;

         case 'N':
            KEY( "Name", board->name, fread_string( fp ) );
            break;

         case 'O':
            KEY( "ObjVnum", board->objvnum, fread_number( fp ) );
            break;

         case 'P':
            KEY( "Posters", board->posters, fread_string( fp ) );
            KEY( "PostLevel", board->post_level, fread_number( fp ) );
            break;

         case 'R':
            KEY( "Readers", board->readers, fread_string( fp ) );
            KEY( "ReadLevel", board->read_level, fread_number( fp ) );
            KEY( "RemoveLevel", board->remove_level, fread_number( fp ) );
            break;

         case '#':
            if( !str_cmp( word, "#END" ) )
            {
               board->nlist.clear(  );
               if( !board->objvnum )
                  board->objvnum = 0;  /* default to global */
               return board;
            }
            else
            {
               bug( "%s: Bad section: %s", __func__, word );
               deleteptr( board );
               return NULL;
            }
      }
   }
}

board_data *read_old_board( FILE * fp )
{
   board_data *board;
   char letter;

   do
   {
      letter = getc( fp );
      if( feof( fp ) )
      {
         FCLOSE( fp );
         return NULL;
      }
   }
   while( isspace( letter ) );

   ungetc( letter, fp );

   board = new board_data;

   for( ;; )
   {
      const char *word = ( feof( fp ) ? "End" : fread_word( fp ) );

      if( word[0] == '\0' )
      {
         bug( "%s: EOF encountered reading file!", __func__ );
         word = "End";
      }

      switch ( UPPER( word[0] ) )
      {
         default:
            bug( "%s: no match: %s", __func__, word );
            fread_to_eol( fp );
            break;

         case '*':
            fread_to_eol( fp );
            break;

         case 'E':
            KEY( "Extra_readers", board->readers, fread_string( fp ) );
            KEY( "Extra_removers", board->moderators, fread_string( fp ) );
            if( !str_cmp( word, "End" ) )
            {
               board->nlist.clear(  );
               if( !board->objvnum )
                  board->objvnum = 0;  /* default to global */
               board->desc = str_dup( "Newly converted board!" );
               board->expire = MAX_BOARD_EXPIRE;
               board->filename = str_dup( board->name );
               if( board->posters[0] == '\0' )
                  STRFREE( board->posters );
               if( board->readers[0] == '\0' )
                  STRFREE( board->readers );
               if( board->moderators[0] == '\0' )
                  STRFREE( board->moderators );
               if( board->group[0] == '\0' )
                  STRFREE( board->group );
               return board;
            }
            break;

         case 'F':
            KEY( "Filename", board->name, fread_string( fp ) );
            break;

         case 'M':
            KEY( "Min_read_level", board->read_level, fread_number( fp ) );
            KEY( "Min_remove_level", board->remove_level, fread_number( fp ) );
            KEY( "Min_post_level", board->post_level, fread_number( fp ) );
            if( !str_cmp( word, "Max_posts" ) )
            {
               word = fread_word( fp );
               break;
            }
            break;

         case 'P':
            KEY( "Post_group", board->posters, fread_string( fp ) );
            break;

         case 'R':
            KEY( "Read_group", board->group, fread_string( fp ) );
            break;

         case 'T':
            if( !str_cmp( word, "Type" ) )
            {
               if( fread_number( fp ) == 1 )
                  TOGGLE_BOARD_FLAG( board, BOARD_PRIVATE );
               break;
            }
            break;

         case 'V':
            KEY( "Vnum", board->objvnum, fread_number( fp ) );
            break;
      }
   }
}

note_data *read_note( FILE * fp )
{
   note_data *pnote = NULL;
   note_data *reply = NULL;
   char letter;

   do
   {
      letter = getc( fp );
      if( feof( fp ) )
      {
         FCLOSE( fp );
         return NULL;
      }
   }
   while( isspace( letter ) );
   ungetc( letter, fp );

   pnote = new note_data;

   for( ;; )
   {
      const char *word = ( feof( fp ) ? "#END" : fread_word( fp ) );

      if( word[0] == '\0' )
      {
         bug( "%s: EOF encountered reading file!", __func__ );
         word = "#END";
      }

      switch ( UPPER( word[0] ) )
      {
         default:
            bug( "%s: no match: %s", __func__, word );
            fread_to_eol( fp );
            break;

         case '*':
            fread_to_eol( fp );
            break;

         case 'F':
            if( !str_cmp( word, "Flags" ) )
            {
               flag_set( fp, pnote->flags, note_flags );
               break;
            }

         case 'D':
            if( !str_cmp( word, "Date" ) )
            {
               fread_to_eol( fp );
               break;
            }
            KEY( "DateStamp", pnote->date_stamp, fread_number( fp ) );
            break;

         case 'S':
            KEY( "Sender", pnote->sender, fread_string( fp ) );
            KEY( "Subject", pnote->subject, fread_string_nohash( fp ) );
            break;

         case 'T':
            KEY( "To", pnote->to_list, fread_string( fp ) );
            KEY( "Text", pnote->text, fread_string_nohash( fp ) );
            break;

         case 'E':
            KEY( "Expire", pnote->expire, fread_number( fp ) );
            break;

         case 'R':
            if( !str_cmp( word, "Reply" ) )
            {
               if( pnote->reply_count == MAX_REPLY )
               {
                  bug( "%s: Reply found when MAX_REPLY has already been reached!", __func__ );
                  continue;
               }
               if( reply != NULL )
               {
                  bug( "%s: Unsupported nested reply found!", __func__ );
                  continue;
               }
               reply = new note_data;
               break;
            }

            if( !str_cmp( word, "Reply-Date" ) && reply != NULL )
            {
               fread_to_eol( fp );
               break;
            }

            if( !str_cmp( word, "Reply-Flags" ) )
            {
               flag_set( fp, reply->flags, note_flags );
               break;
            }

            if( !str_cmp( word, "Reply-DateStamp" ) && reply != NULL )
            {
               reply->date_stamp = fread_number( fp );
               break;
            }

            if( !str_cmp( word, "Reply-Sender" ) && reply != NULL )
            {
               reply->sender = fread_string( fp );
               break;
            }

            if( !str_cmp( word, "Reply-Subject" ) && reply != NULL )
            {
               reply->subject = fread_string_nohash( fp );
               break;
            }

            if( !str_cmp( word, "Reply-To" ) && reply != NULL )
            {
               reply->to_list = fread_string( fp );
               break;
            }

            if( !str_cmp( word, "Reply-Text" ) && reply != NULL )
            {
               reply->text = fread_string_nohash( fp );
               break;
            }

            if( !str_cmp( word, "Reply-End" ) && reply != NULL )
            {
               reply->expire = 0;
               if( !reply->date_stamp )
                  reply->date_stamp = current_time;
               reply->parent = pnote;
               pnote->rlist.push_back( reply );
               pnote->reply_count += 1;
               reply = NULL;
               break;
            }

         case '#':
            if( !str_cmp( word, "#END" ) )
            {
               // Use the new sticky flag :)
               if( pnote->expire == 0 )
                  TOGGLE_NOTE_FLAG( pnote, NOTE_STICKY );

               if( !pnote->date_stamp )
                  pnote->date_stamp = current_time;
               return pnote;
            }
            else
            {
               bug( "%s: Bad section: %s", __func__, word );
               deleteptr( pnote );
               return NULL;
            }
      }
   }
}

note_data *read_old_note( FILE * fp )
{
   note_data *pnote = NULL;
   char letter;

   do
   {
      letter = getc( fp );
      if( feof( fp ) )
      {
         FCLOSE( fp );
         return NULL;
      }
   }
   while( isspace( letter ) );
   ungetc( letter, fp );

   pnote = new note_data;

   for( ;; )
   {
      const char *word = ( feof( fp ) ? "End" : fread_word( fp ) );

      if( word[0] == '\0' )
      {
         bug( "%s: EOF encountered reading file!", __func__ );
         word = "End";
      }

      /*
       * Keep the damn thing happy. 
       */
      if( !str_cmp( word, "Voting" ) || !str_cmp( word, "Yesvotes" ) || !str_cmp( word, "Novotes" ) || !str_cmp( word, "Abstentions" ) )
      {
         word = fread_word( fp );
         continue;
      }

      switch ( UPPER( word[0] ) )
      {
         default:
            bug( "%s: no match: %s", __func__, word );
            fread_to_eol( fp );
            break;

         case '*':
            fread_to_eol( fp );
            break;

         case 'C':
            // Since this confused me too, Ctime happens to be the last field in the old note file :) --X
            if( !str_cmp( word, "Ctime" ) )
            {
               pnote->date_stamp = fread_number( fp );
               pnote->expire = MAX_BOARD_EXPIRE;   // Going to default to the max expire time -X

               if( !pnote->text )
               {
                  pnote->text = str_dup( "---- Delete this post! ----" );
                  pnote->expire = 1;   /* Or we'll do it for you! */
                  bug( "%s: No text on note! Setting to '%s' and expiration to 1 day", __func__, pnote->text );
               }

               if( !pnote->date_stamp )
               {
                  bug( "%s: No date_stamp on note -- setting to current time!", __func__ );
                  pnote->date_stamp = current_time;
               }

               /*
                * For converted notes, lets make a few exceptions 
                */
               if( !pnote->sender )
               {
                  pnote->sender = STRALLOC( "Converted Msg" );
                  bug( "%s: No sender on converted note! Setting to '%s'", __func__, pnote->sender );
               }
               if( !pnote->subject )
               {
                  pnote->subject = str_dup( "Converted Msg" );
                  bug( "%s: No subject on converted note! Setting to '%s'", __func__, pnote->subject );
               }
               if( !pnote->to_list )
               {
                  pnote->to_list = STRALLOC( "imm" );
                  bug( "%s: No to_list on converted note! Setting to '%s'", __func__, pnote->to_list );
               }
               return pnote;
            }
            break;

         case 'S':
            KEY( "Sender", pnote->sender, fread_string( fp ) );
            KEY( "Subject", pnote->subject, fread_string_nohash( fp ) );
            break;

         case 'T':
            KEY( "To", pnote->to_list, fread_string( fp ) );
            KEY( "Text", pnote->text, fread_string_nohash( fp ) );
            break;

         case 'E':
            if( !str_cmp( word, "End" ) )
            {
               pnote->expire = 0;
               if( !pnote->text )
               {
                  pnote->text = str_dup( "---- Delete this post! ----" );
                  pnote->expire = 1;   /* Or we'll do it for you! */
                  bug( "%s: No text on note! Setting to '%s' and expiration to 1 day", __func__, pnote->text );
               }

               if( !pnote->date_stamp )
               {
                  bug( "%s: No date_stamp on note -- setting to current time!", __func__ );
                  pnote->date_stamp = current_time;
               }

               /*
                * For converted notes, lets make a few exceptions 
                */
               if( !pnote->sender )
               {
                  pnote->sender = STRALLOC( "Converted Msg" );
                  bug( "%s: No sender on converted note! Setting to '%s'", __func__, pnote->sender );
               }
               if( !pnote->subject )
               {
                  pnote->subject = str_dup( "Converted Msg" );
                  bug( "%s: No subject on converted note! Setting to '%s'", __func__, pnote->subject );
               }
               if( !pnote->to_list )
               {
                  pnote->to_list = STRALLOC( "imm" );
                  bug( "%s: No to_list on converted note! Setting to '%s'", __func__, pnote->to_list );
               }
               return pnote;
            }
            break;
      }
   }
}

void board_check_expire( board_data * );
void load_boards( void )
{
   FILE *board_fp, *note_fp;
   board_data *board;
   note_data *pnote;
   char notefile[256];
   bool oldboards = false;
   char backupCmd[1024];

   bdlist.clear(  );

   if( !( board_fp = fopen( BOARD_DIR BOARD_FILE, "r" ) ) )
   {
      if( !( board_fp = fopen( BOARD_DIR OLD_BOARD_FILE, "r" ) ) )
         return;
      oldboards = true;
      log_string( "Converting older boards..." );
   }

   if( oldboards )
   {
      while( ( board = read_old_board( board_fp ) ) != NULL )
      {
         bdlist.push_back( board );
         snprintf( notefile, 256, "%s%s", BOARD_DIR, board->filename );

         // This seems cheap, but functional. I want to back up old boards...
         snprintf( backupCmd, 1024, "cp %s %s.old", notefile, notefile );
         if( (system( backupCmd )) == -1 )
            bug( "%s: Cannot execute backup command for old boards.", __func__ );

         if( ( note_fp = fopen( notefile, "r" ) ) != NULL )
         {
            log_string( notefile );
            while( ( pnote = read_old_note( note_fp ) ) != NULL )
            {
               board->nlist.push_back( pnote );
               board->msg_count += 1;
            }
         }
         else
            bug( "%s: Note file '%s' for the '%s' board not found!", __func__, notefile, board->name );

         write_board( board );   /* save the converted board */

         // Since I'm now setting the default expire time, I think it's only fair to
         // Do an initial archiving to remove any old posts, and keep them somewhere that the admins
         // can easily restore from. --X
         TOGGLE_BOARD_FLAG( board, BOARD_BU_PRUNED );
         board_check_expire( board );
         TOGGLE_BOARD_FLAG( board, BOARD_BU_PRUNED );
      }
      write_boards(  ); /* save the new boards file */
   }
   else
   {
      while( ( board = read_board( board_fp ) ) != NULL )
      {
         bdlist.push_back( board );
         snprintf( notefile, 256, "%s%s.board", BOARD_DIR, board->filename );
         if( ( note_fp = fopen( notefile, "r" ) ) != NULL )
         {
            log_string( notefile );
            while( ( pnote = read_note( note_fp ) ) != NULL )
            {
               board->nlist.push_back( pnote );
               board->msg_count += 1;
            }
         }
         else
            bug( "%s: Note file '%s' for the '%s' board not found!", __func__, notefile, board->name );
      }
   }
   /*
    * Run initial pruning 
    */
   check_boards(  );
}

int unread_notes( char_data * ch, board_data * board )
{
   list < note_data * >::iterator note;
   board_chardata *chboard;
   int count = 0;

   chboard = get_chboard( ch, board->name );

   for( note = board->nlist.begin(  ); note != board->nlist.end(  ); ++note )
   {
      note_data *pnote = *note;

      if( IS_BOARD_FLAG( board, BOARD_PRIVATE ) && !is_note_to( ch, pnote ) )
         continue;
      if( !chboard )
         ++count;
      else if( pnote->date_stamp > chboard->last_read )
         ++count;
   }
   return count;
}

int total_replies( board_data * board )
{
   list < note_data * >::iterator note;
   int count = 0;

   for( note = board->nlist.begin(  ); note != board->nlist.end(  ); ++note )
   {
      note_data *pnote = *note;

      count += pnote->reply_count;
   }
   return count;
}

/* This is because private boards don't function right with board->msg_count...  :P */
int total_notes( char_data * ch, board_data * board )
{
   list < note_data * >::iterator note;
   int count = 0;

   for( note = board->nlist.begin(  ); note != board->nlist.end(  ); ++note )
   {
      note_data *pnote = *note;

      if( IS_BOARD_FLAG( board, BOARD_PRIVATE ) && !is_note_to( ch, pnote ) && !can_remove( ch, board ) )
         continue;
      else
         ++count;
   }
   return count;
}

/* Only expire root messages. Replies are pointless to expire. */
void board_check_expire( board_data * board )
{
   list < note_data * >::iterator note;
   FILE *fp;
   char filename[256];

   // This defaults everything to sticky anyway...
   if( board->expire == 0 )
      return;

   time_t now_time = time( 0 );

   for( note = board->nlist.begin(  ); note != board->nlist.end(  ); )
   {
      note_data *pnote = *note;
      ++note;

      if( IS_NOTE_FLAG( pnote, NOTE_STICKY ) )
         continue;

      time_t time_diff = ( now_time - pnote->date_stamp ) / 86400;
      if( time_diff >= pnote->expire )
      {
         if( IS_BOARD_FLAG( board, BOARD_BU_PRUNED ) )
         {
            snprintf( filename, 256, "%s%s.purged", BOARD_DIR, board->name );
            if( !( fp = fopen( filename, "a" ) ) )
            {
               perror( filename );
               bug( "%s: Error opening %s!", __func__, filename );
               return;
            }
            fwrite_note( pnote, fp );
            log_printf( "Saving expired note '%s' to '%s'", pnote->subject, filename );
            FCLOSE( fp );
         }
         log_printf( "Removing expired note '%s'", pnote->subject );
         note_remove( board, pnote );
         continue;
      }
   }
}

void check_boards( void )
{
   list < board_data * >::iterator bd;

   log_string( "Starting board pruning..." );
   for( bd = bdlist.begin(  ); bd != bdlist.end(  ); ++bd )
   {
      board_data *board = *bd;

      board_check_expire( board );
   }
}

void board_announce( char_data * ch, board_data * board, note_data * pnote )
{
   board_chardata *chboard;
   list < descriptor_data * >::iterator ds;

   for( ds = dlist.begin(  ); ds != dlist.end(  ); ++ds )
   {
      descriptor_data *d = *ds;

      if( d->connected != CON_PLAYING )
         continue;

      char_data *vch = d->character ? d->character : d->original;
      if( !vch )
         continue;

      if( !can_read( vch, board ) )
         continue;

      if( !IS_BOARD_FLAG( board, BOARD_ANNOUNCE ) )
      {
         if( !( chboard = get_chboard( ch, board->name ) ) )
            continue;
         if( chboard->alert != BD_ANNOUNCE )
            continue;
      }

      // Changed this so no matter what, if the note's to you,  you get a personal message. --X (9/21/07)
      if( is_note_to( vch, pnote ) )
         vch->printf( "&G[&wBoard Announce&G] &w%s has posted a message for you on the %s board.\r\n", ch->name, board->name );
      else if( !IS_BOARD_FLAG( board, BOARD_PRIVATE ) )
         vch->printf( "&G[&wBoard Announce&G] &w%s has posted a message on the %s board.\r\n", ch->name, board->name );
   }
}

void note_to_char( char_data * ch, note_data * pnote, board_data * board, short id )
{
   int count = 1;

   if( pnote == NULL )
   {
      bug( "%s: null pnote!", __func__ );
      return;
   }

   if( id > 0 && board != NULL )
      ch->printf( "&[board3][&[board]Note #&[board2]%d&[board] of &[board2]%d&[board3]]&[board]           -- &[board2]%s&[board] --&D\r\n", id, total_notes( ch, board ),
                  board->name );

   /*
    * Using a negative ID# for a bit of beauty -- Xorith 
    */
   if( id == -1 && !board )
      ch->print( "&[board3][&[board2]Project Log&[board3]]&D\r\n" );
   if( id == -2 && !board )
      ch->print( "&[board3][&[board2]Player Comment&[board3]]&D\r\n" );

   ch->printf( "&[board]From:    &[board2]%-15s", pnote->sender ? pnote->sender : "--Error--" );
   if( pnote->to_list )
      ch->printf( " &[board]To:     &[board2]%-15s", pnote->to_list );
   ch->print( "&D\r\n" );
   if( pnote->date_stamp != 0 )
      ch->printf( "&[board]Date:    &[board2]%s&D\r\n", c_time( pnote->date_stamp, ch->pcdata->timezone ) );

   if( board && can_remove( ch, board ) )
   {
      if( IS_NOTE_FLAG( pnote, NOTE_STICKY ) )
         ch->print( "&[board]This note is sticky and will not expire.&D\r\n" );
      else
      {
         int n_life;
         n_life = pnote->expire - ( ( current_time - pnote->date_stamp ) / 86400 );
         ch->printf( "&[board]This note will expire in &[board2]%d&[board] day%s.&D\r\n", n_life, n_life == 1 ? "" : "s" );
      }
   }

   ch->printf( "&[board]Subject: &[board2]%s&D\r\n", pnote->subject ? pnote->subject : "" );
   ch->print( "&[board]------------------------------------------------------------------------------&D\r\n" );
   ch->printf( "&[board2]%s&D\r\n", pnote->text ? pnote->text : "--Error--" );
   if( !pnote->rlist.empty(  ) )
   {
      list < note_data * >::iterator rp;

      for( rp = pnote->rlist.begin(  ); rp != pnote->rlist.end(  ); ++rp )
      {
         note_data *reply = *rp;

         ch->print( "\r\n&[board]------------------------------------------------------------------------------&D\r\n" );
         ch->printf( "&[board3][&[board]Reply #&[board2]%d&[board3]] [&[board2]%s&[board3]]&D\r\n", count, c_time( reply->date_stamp, ch->pcdata->timezone ) );
         ch->printf( "&[board]From:    &[board2]%-15s", reply->sender ? reply->sender : "--Error--" );
         if( reply->to_list )
            ch->printf( "   &[board]To:     &[board2]%-15s", reply->to_list );
         ch->print( "&D\r\n&[board]------------------------------------------------------------------------------&D\r\n" );
         ch->printf( "&[board2]%s&D\r\n", reply->text ? reply->text : "--Error--" );
         ++count;
      }
   }
   ch->print( "&[board]------------------------------------------------------------------------------&D\r\n" );
   if( board && can_remove( ch, board ) && IS_NOTE_FLAG( pnote, NOTE_HIDDEN ) )
      ch->print( "&[board]This note is &[board2]hidden&[board]. Only moderators may see it.&D\r\n" );
   if( IS_NOTE_FLAG( pnote, NOTE_CLOSED ) )
      ch->print( "&[board]This note is &RClosed&[board]. No further replies may be made.&D\r\n" );
}

CMDF( do_note_set )
{
   board_data *board;
   short n_num = 0, i = 1;
   int value = -1;
   string arg;

   if( !( board = find_board( ch ) ) )
   {
      if( argument.empty(  ) )
      {
         ch->
            print
            ( "&[board]Modifies the fields on a note.\r\n&[board3]Syntax: &[board]nset &[board3]<&[board2]board&[board3]> <&[board2]note#&[board3]> <&[board2]field&[board3]> <&[board2]value&[board3]>&D\r\n" );
         ch->print( "&[board]  Valid fields are: &[board2]sender to_list subject expire flags&D\r\n" );
         ch->print( "&[board]  Flags: &[board2]sticky hidden closed&D\r\n" );
         return;
      }
      argument = one_argument( argument, arg );

      if( !( board = get_board( ch, arg ) ) )
      {
         ch->print( "&[board]No board found!&D\r\n" );
         return;
      }
   }
   else
      ch->printf( "&[board]Using current board in room: &[board2]%s&D\r\n", board->name );

   if( !can_remove( ch, board ) )
   {
      ch->print( "You are unable to modify the notes on this board.\r\n" );
      return;
   }

   if( argument.empty(  ) )
   {
      ch->print( "Modify which note?\r\n" );
      return;
   }
   argument = one_argument( argument, arg );
   n_num = atoi( arg.c_str(  ) );

   if( n_num == 0 )
   {
      ch->print( "Modify which note?\r\n" );
      return;
   }

   i = 1;
   note_data *pnote = NULL;
   list < note_data * >::iterator inote;
   for( inote = board->nlist.begin(  ); inote != board->nlist.end(  ); ++inote )
   {
      note_data *note = *inote;

      if( i == n_num )
      {
         pnote = note;
         break;
      }
      else
         ++i;
   }

   if( !pnote )
   {
      ch->print( "Note not found!\r\n" );
      return;
   }
   argument = one_argument( argument, arg );

   if( !str_cmp( arg, "expire" ) )
   {
      if( atoi( argument.c_str(  ) ) < 0 || atoi( argument.c_str(  ) ) > MAX_BOARD_EXPIRE )
      {
         ch->printf( "&[board]Expiration days must be a value between &[board2]0&[board] and &[board2]%d&[board].&D\r\n", MAX_BOARD_EXPIRE );
         return;
      }
      pnote->expire = atoi( argument.c_str(  ) );
      ch->printf( "&[board]Set the expiration time for '&[board2]%s&[board]' to &[board2]%d&D\r\n", pnote->subject, pnote->expire );
      write_board( board );
      return;
   }

   if( !str_cmp( arg, "sender" ) )
   {
      if( argument.empty(  ) )
      {
         ch->print( "&[board]You must specify a new sender.&D\r\n" );
         return;
      }
      STRFREE( pnote->sender );
      pnote->sender = STRALLOC( argument.c_str(  ) );
      ch->printf( "&[board]Set the sender for '&[board2]%s&[board]' to &[board2]%s&D\r\n", pnote->subject, pnote->sender );
      write_board( board );
      return;
   }

   if( !str_cmp( arg, "flags" ) )
   {
      char buf[MSL];
      bool fMatch = false, fUnknown = false;

      ch->print( "&[board]Setting flags: &[board2]" );
      mudstrlcpy( buf, "\r\n&[board]Unknown Flags: &[board2]", MSL );
      while( !argument.empty(  ) )
      {
         argument = one_argument( argument, arg );
         value = get_note_flag( arg );
         if( value < 0 || value >= MAX_NOTE_FLAGS )
         {
            fUnknown = true;
            snprintf( buf + strlen( buf ), MSL - strlen( buf ), " %s", arg.c_str(  ) );
         }
         else
         {
            fMatch = true;
            TOGGLE_NOTE_FLAG( pnote, value );
            if( IS_NOTE_FLAG( pnote, value ) )
               ch->printf( " +%s", arg.c_str(  ) );
            else
               ch->printf( " -%s", arg.c_str(  ) );
         }
      }
      ch->printf( "%s%s&D\r\n", fMatch ? "" : "none", fUnknown ? buf : "" );
      write_boards(  );
      return;
   }

   if( !str_cmp( arg, "to_list" ) )
   {
      if( argument.empty(  ) )
      {
         ch->print( "&[board]You must specify a new to_list.&D\r\n" );
         return;
      }
      STRFREE( pnote->to_list );
      pnote->to_list = STRALLOC( argument.c_str(  ) );
      ch->printf( "&[board]Set the to_list for '&[board2]%s&[board]' to &[board2]%s&D\r\n", pnote->subject, pnote->to_list );
      write_board( board );
      return;
   }

   if( !str_cmp( arg, "subject" ) )
   {
      if( argument.empty(  ) )
      {
         ch->print( "&[board]You must specify a new subject.&D\r\n" );
         return;
      }
      ch->printf( "&[board]Set the subject for '&[board2]%s&[board]' to &[board2]%s&D\r\n", pnote->subject, argument.c_str(  ) );
      DISPOSE( pnote->subject );
      pnote->subject = str_dup( argument.c_str(  ) );
      write_board( board );
      return;
   }
   ch->
      print
      ( "&[board]Modifies the fields on a note.\r\n&[board3]Syntax: &[board]nset &[board3]<&[board2]board&[board3]> <&[board2]note#&[board3]> <&[board2]field&[board3]> <&[board2]value&[board3]>&D\r\n" );
   ch->print( "&[board]  Valid fields are: &[board2]sender to_list subject expire flags&D\r\n" );
   ch->print( "&[board]  Flags: &[board2]sticky hidden closed&D\r\n" );
}

/* This handles anything in the CON_BOARD state. */
CMDF( do_note_write );
void board_parse( descriptor_data * d, const string & argument )
{
   char_data *ch;
   char s1[16], s2[16], s3[16];

   ch = d->character ? d->character : d->original;

   /*
    * Can NPCs even have substates and connected? 
    */
   if( ch->isnpc(  ) )
   {
      bug( "%s: NPC in %s!", __func__, __func__ );
      d->connected = CON_PLAYING;
      ch->substate = SUB_NONE;
      return;
   }

   /*
    * If a board sub-system is in place, these checks will need to reflect the proper substates
    * that would require them. For instance. a main menu would not require a note on the char. -- X 
    */
   if( !ch->pcdata->pnote )
   {
      bug( "%s: In substate -- No pnote on character!", __func__ );
      d->connected = CON_PLAYING;
      ch->substate = SUB_NONE;
      return;
   }

   if( !ch->pcdata->board )
   {
      bug( "%s: In substate -- No board on character!", __func__ );
      d->connected = CON_PLAYING;
      ch->substate = SUB_NONE;
      return;
   }

   /*
    * This will always kick a player out of the subsystem. I kept it '/a' as to not cause confusion
    * with players in the editor. 
    */
   if( !str_cmp( argument, "/a" ) )
   {
      ch->substate = SUB_EDIT_ABORT;
      d->connected = CON_PLAYING;
      do_note_write( ch, "" );
      return;
   }

   snprintf( s1, 16, "%s", ch->color_str( AT_BOARD ) );
   snprintf( s2, 16, "%s", ch->color_str( AT_BOARD2 ) );
   snprintf( s3, 16, "%s", ch->color_str( AT_BOARD3 ) );

   switch ( ch->substate )
   {
      default:
         ch->substate = SUB_NONE;
         d->connected = CON_PLAYING;
         break;

      case SUB_BOARD_STICKY:
         if( !str_cmp( argument, "yes" ) || !str_cmp( argument, "y" ) )
         {
            ch->pcdata->pnote->expire = 0;
            TOGGLE_NOTE_FLAG( ch->pcdata->pnote, NOTE_STICKY );
            ch->printf( "%sThis note will not expire during pruning.&D\r\n", s1 );
         }
         else
         {
            ch->pcdata->pnote->expire = ch->pcdata->board->expire;
            ch->printf( "%sNote set to expire after the default of %s%d%s day%s.&D\r\n", s1, s2, ch->pcdata->pnote->expire, s1, ch->pcdata->pnote->expire == 1 ? "" : "s" );
         }

         if( ch->pcdata->pnote->parent )
            ch->printf( "%sTo whom is this note addressed? %s(%sDefault: %s%s%s)&D   ", s1, s3, s1, s2, ch->pcdata->pnote->parent->sender, s3 );
         else
            ch->printf( "%sTo whom is this note addressed? %s(%sDefault: %sAll%s)&D   ", s1, s3, s1, s2, s3 );
         ch->substate = SUB_BOARD_TO;
         return;

      case SUB_BOARD_TO:
         if( argument.empty(  ) || !str_cmp( argument, "all" ) )
         {
            if( IS_BOARD_FLAG( ch->pcdata->board, BOARD_PRIVATE ) && !ch->is_immortal(  ) )
            {
               ch->printf( "%sYou must specify a recipient:&D   ", s1 );
               return;
            }
            STRFREE( ch->pcdata->pnote->to_list );
            if( ch->pcdata->pnote->parent )
               ch->pcdata->pnote->to_list = STRALLOC( ch->pcdata->pnote->parent->sender );
            else
               ch->pcdata->pnote->to_list = STRALLOC( "All" );

            if( str_cmp( argument, "all" ) )
               ch->printf( "%sNo recipient specified. Defaulting to '%s%s%s'&D\r\n", s1, s2, ch->pcdata->pnote->to_list, s1 );
            else
               ch->printf( "%sRecipient set to '%sAll%s'&D\r\n", s1, s2, s1 );
         }
         else
         {
            struct stat fst;
            char buf[256];

            snprintf( buf, 256, "%s%c/%s", PLAYER_DIR, tolower( argument[0] ), capitalize( argument ).c_str(  ) );
            if( stat( buf, &fst ) == -1 || !check_parse_name( capitalize( argument ), false ) )
            {
               ch->printf( "%sNo such player named '%s%s%s'.\r\nTo whom is this note addressed?   &D", s1, s2, argument.c_str(  ), s1 );
               return;
            }
            STRFREE( ch->pcdata->pnote->to_list );
            ch->pcdata->pnote->to_list = STRALLOC( argument.c_str(  ) );
         }

         ch->printf( "%sTo: %s%-15s %sFrom: %s%s&D\r\n", s1, s2, ch->pcdata->pnote->to_list, s1, s2, ch->pcdata->pnote->sender );
         if( ch->pcdata->pnote->subject )
         {
            ch->printf( "%sSubject: %s%s&D\r\n", s1, s2, ch->pcdata->pnote->subject );
            ch->substate = SUB_BOARD_TEXT;
            ch->printf( "%sPlease enter the text for your message:&D\r\n", s1 );
            if( !ch->pcdata->pnote->text )
               ch->pcdata->pnote->text = str_dup( "" );
            ch->editor_desc_printf( "A note to %s", ch->pcdata->pnote->to_list );
            ch->start_editing( ch->pcdata->pnote->text );
            return;
         }
         ch->substate = SUB_BOARD_SUBJECT;
         ch->printf( "%sPlease enter a subject for this note:&D   ", s1 );
         return;

      case SUB_BOARD_SUBJECT:
         if( argument.empty(  ) )
         {
            ch->printf( "%sNo subject specified!&D\r\n%sYou must specify a subject:&D  ", s3, s1 );
            return;
         }
         else
         {
            DISPOSE( ch->pcdata->pnote->subject );
            ch->pcdata->pnote->subject = str_dup( argument.c_str(  ) );
         }

         ch->substate = SUB_BOARD_TEXT;
         ch->printf( "%sTo: %s%-15s %sFrom: %s%s&D\r\n", s1, s2, ch->pcdata->pnote->to_list, s1, s2, ch->pcdata->pnote->sender );
         ch->printf( "%sSubject: %s%s&D\r\n", s1, s2, ch->pcdata->pnote->subject );
         ch->printf( "%sPlease enter the text for your message:&D\r\n", s1 );
         if( !ch->pcdata->pnote->text )
            ch->pcdata->pnote->text = str_dup( "" );
         ch->editor_desc_printf( "A note to %s", ch->pcdata->pnote->to_list );
         ch->start_editing( ch->pcdata->pnote->text );
         return;

      case SUB_BOARD_CONFIRM:
         if( argument.empty(  ) )
         {
            note_to_char( ch, ch->pcdata->pnote, NULL, 0 );
            ch->printf( "%sYou %smust%s confirm! Is this correct? %s(%sType: %sY%s or %sN%s)&D   ", s1, s3, s1, s3, s1, s2, s1, s2, s3 );
            return;
         }

         if( !str_cmp( argument, "y" ) || !str_cmp( argument, "yes" ) )
         {
            if( !ch->pcdata->pnote )
            {
               bug( "%s: NULL (ch)%s->pcdata->pnote!", __func__, ch->name );
               d->connected = CON_PLAYING;
               ch->substate = SUB_NONE;
               return;
            }
            ch->pcdata->pnote->date_stamp = current_time;
            if( ch->pcdata->pnote->parent )
            {
               if( !IS_BOARD_FLAG( ch->pcdata->board, BOARD_PRIVATE ) )
               {
                  ++ch->pcdata->pnote->parent->reply_count;
                  ch->pcdata->pnote->parent->date_stamp = ch->pcdata->pnote->date_stamp;
                  ch->pcdata->pnote->parent->rlist.push_back( ch->pcdata->pnote );
               }
               else
               {
                  ch->pcdata->pnote->parent = NULL;
                  ch->pcdata->board->nlist.push_back( ch->pcdata->pnote );
                  ++ch->pcdata->board->msg_count;
               }
            }
            else
            {
               ch->pcdata->board->nlist.push_back( ch->pcdata->pnote );
               ++ch->pcdata->board->msg_count;
            }
            write_board( ch->pcdata->board );

            ch->from_room(  );
            if( !ch->to_room( ch->orig_room ) )
               log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __func__, __LINE__ );

            ch->printf( "%sYou post the note '%s%s%s' on the %s%s%s board.&D\r\n", s1, s2, ch->pcdata->pnote->subject, s1, s2, ch->pcdata->board->name, s1 );
            act( AT_GREY, "$n posts a note on the board.", ch, NULL, NULL, TO_ROOM );
            board_announce( ch, ch->pcdata->board, ch->pcdata->pnote );
            ch->pcdata->pnote = NULL;
            ch->pcdata->board = NULL;
            ch->substate = SUB_NONE;
            d->connected = CON_PLAYING;
            return;
         }

         if( !str_cmp( argument, "n" ) || !str_cmp( argument, "no" ) )
         {
            ch->substate = SUB_BOARD_REDO_MENU;
            ch->printf( "%sWhat would you like to change?&D\r\n", s1 );
            ch->printf( "   %s1%s) %sRecipients  %s[%s%s%s]&D\r\n", s2, s3, s1, s3, s2, ch->pcdata->pnote->to_list, s3 );
            if( ch->pcdata->pnote->parent )
               ch->printf( "   %s2%s) %sYou cannot change subjects on a reply.&D\r\n", s2, s3, s1 );
            else
               ch->printf( "   %s2%s) %sSubject     %s[%s%s%s]&D\r\n", s2, s3, s1, s3, s2, ch->pcdata->pnote->subject, s3 );
            ch->printf( "   %s3%s) %sText\r\n%s%s&D\r\n", s2, s3, s1, s2, ch->pcdata->pnote->text );
            ch->printf( "%sPlease choose an option, %sQ%s to quit this menu, or %s/a%s to abort:&D   ", s1, s2, s1, s2, s1 );
            return;
         }
         ch->printf( "%sPlease enter either %sY%s or %sN%s:&D   ", s1, s2, s1, s2, s1 );
         return;

      case SUB_BOARD_REDO_MENU:
         if( argument.empty(  ) )
         {
            ch->printf( "%sPlease choose an option, %sQ%s to quit this menu, or %s/a%s to abort:&D   ", s1, s2, s1, s2, s1 );
            return;
         }

         if( !str_cmp( argument, "1" ) )
         {
            if( ch->pcdata->pnote->parent )
               ch->printf( "%sTo whom is this note addressed? %s(%sDefault: %s%s%s)&D   ", s1, s3, s1, s2, ch->pcdata->pnote->parent->sender, s3 );
            else
               ch->printf( "%sTo whom is this note addressed? %s(%sDefault: %sAll%s)&D   ", s1, s3, s1, s2, s3 );
            ch->substate = SUB_BOARD_TO;
            return;
         }

         if( !str_cmp( argument, "2" ) )
         {
            if( ch->pcdata->pnote->parent )
            {
               ch->printf( "%sYou cannot change the subject of a reply!&D\r\n", s1 );
               ch->printf( "%sPlease choose an option, %sQ%s to quit this menu, or %s/a%s to abort:&D   ", s1, s2, s1, s2, s1 );
               return;
            }
            ch->substate = SUB_BOARD_SUBJECT;
            ch->printf( "%sPlease enter a subject for this note:&D   ", s1 );
            return;
         }

         if( !str_cmp( argument, "3" ) )
         {
            ch->substate = SUB_BOARD_TEXT;
            ch->printf( "%sPlease enter the text for your message:&D\r\n", s1 );
            if( !ch->pcdata->pnote->text )
               ch->pcdata->pnote->text = str_dup( "" );
            ch->editor_desc_printf( "A note to %s about %s", ch->pcdata->pnote->to_list, ch->pcdata->pnote->subject );
            ch->start_editing( ch->pcdata->pnote->text );
            return;
         }

         if( !str_cmp( argument, "q" ) )
         {
            ch->substate = SUB_BOARD_CONFIRM;
            note_to_char( ch, ch->pcdata->pnote, NULL, 0 );
            ch->printf( "%sIs this correct? %s(%sY%s/%sN%s)&D   ", s1, s3, s2, s3, s2, s3 );
            return;
         }
         ch->printf( "%sPlease choose an option, %sQ%s to quit this menu, or %s/a%s to abort:&D   ", s1, s2, s1, s2, s1 );
         return;
   }
}

// This is like a stripped down nset, indended for anyone with moderator access -X
// Added: 9-23-07
// Command: bmod
// Level: 10
CMDF( do_board_moderate )
{
   board_data *board;
   short n_num = 0, i = 1;
   string arg;

   if( !( board = find_board( ch ) ) )
   {
      if( argument.empty(  ) )
      {
         ch->
            print
            ( "&[board]Administrates notes on boards.\r\n&[board3]Syntax: &[board]bmod &[board3]<&[board2]board&[board3]> <&[board2]note#&[board3]> <&[board2]action&[board3]>&D\r\n" );
         ch->print( "&[board]  Valid actions are: &[board2]close, open, hide, unhide, sticky, unsticky&D\r\n" );
         return;
      }
      argument = one_argument( argument, arg );

      if( !( board = get_board( ch, arg ) ) )
      {
         ch->print( "&[board]No board found!&D\r\n" );
         return;
      }
   }
   else
      ch->printf( "&[board]Using current board in room: &[board2]%s&D\r\n", board->name );

   if( !can_remove( ch, board ) )
   {
      ch->print( "You are not a moderator of this board.\r\n" );
      return;
   }

   if( argument.empty(  ) )
   {
      ch->print( "Modify which note?\r\n" );
      return;
   }
   argument = one_argument( argument, arg );
   n_num = atoi( arg.c_str(  ) );

   if( n_num == 0 )
   {
      ch->print( "Modify which note?\r\n" );
      return;
   }

   i = 1;
   note_data *pnote = NULL;
   list < note_data * >::iterator inote;
   for( inote = board->nlist.begin(  ); inote != board->nlist.end(  ); ++inote )
   {
      note_data *note = *inote;

      if( i == n_num )
      {
         pnote = note;
         break;
      }
      else
         ++i;
   }

   if( !pnote )
   {
      ch->print( "Note not found!\r\n" );
      return;
   }
   argument = one_argument( argument, arg );

   if( !str_cmp( arg, "close" ) )
   {
      if( IS_NOTE_FLAG( pnote, NOTE_CLOSED ) )
         ch->print( "&[board]That note is already closed.&D\r\n" );
      else
      {
         TOGGLE_NOTE_FLAG( pnote, NOTE_CLOSED );
         ch->printf( "&[board]The note '&[board2]%s&[board]' is now closed.", pnote->subject );
      }
      return;
   }

   if( !str_cmp( arg, "open" ) )
   {
      if( !IS_NOTE_FLAG( pnote, NOTE_CLOSED ) )
         ch->print( "&[board]That note is already open.&D\r\n" );
      else
      {
         TOGGLE_NOTE_FLAG( pnote, NOTE_CLOSED );
         ch->printf( "&[board]The note '&[board2]%s&[board]' is now open.", pnote->subject );
      }
      return;
   }

   if( !str_cmp( arg, "hide" ) )
   {
      if( IS_NOTE_FLAG( pnote, NOTE_HIDDEN ) )
         ch->print( "&[board]That note is already hidden.&D\r\n" );
      else
      {
         TOGGLE_NOTE_FLAG( pnote, NOTE_HIDDEN );
         ch->printf( "&[board]The note '&[board2]%s&[board]' is now hidden.", pnote->subject );
      }
      return;
   }

   if( !str_cmp( arg, "unhide" ) )
   {
      if( !IS_NOTE_FLAG( pnote, NOTE_HIDDEN ) )
         ch->print( "&[board]That note is already visible.&D\r\n" );
      else
      {
         TOGGLE_NOTE_FLAG( pnote, NOTE_HIDDEN );
         ch->printf( "&[board]The note '&[board2]%s&[board]' is now visible.", pnote->subject );
      }
      return;
   }

   if( !str_cmp( arg, "sticky" ) )
   {
      if( IS_NOTE_FLAG( pnote, NOTE_STICKY ) || board->expire == 0 )
         ch->print( "&[board]That note is already sticky.&D\r\n" );
      else
      {
         TOGGLE_NOTE_FLAG( pnote, NOTE_STICKY );
         ch->printf( "&[board]The note '&[board2]%s&[board]' is now sticky.", pnote->subject );
      }
      return;
   }

   if( !str_cmp( arg, "unsticky" ) )
   {
      if( board->expire == 0 )
      {
         ch->printf( "&[board]This note cannot be made unsticky, as &[board2]%s&[board] is set to never expire notes.&D\r\n", board->name );
         return;
      }

      if( !IS_NOTE_FLAG( pnote, NOTE_STICKY ) )
         ch->print( "&[board]That note is already unsticky.&D\r\n" );
      else
      {
         TOGGLE_NOTE_FLAG( pnote, NOTE_STICKY );
         ch->printf( "&[board]The note '&[board2]%s&[board]' is now unsticky.", pnote->subject );
      }
      return;
   }

   ch->
      print
      ( "&[board]Administrates notes on boards.\r\n&[board3]Syntax: &[board]bmod &[board3]<&[board2]board&[board3]> <&[board2]note#&[board3]> <&[board2]action&[board3]>&D\r\n" );
   ch->print( "&[board]  Valid actions are: &[board2]close, open, hide, unhide, sticky, unsticky&D\r\n" );
}

CMDF( do_note_write )
{
   board_data *board = NULL;
   room_index *board_room;
   string arg;
   char buf[MSL];
   int n_num = 0;
   char s1[16], s2[16], s3[16];

   if( ch->isnpc(  ) )
      return;

   snprintf( s1, 16, "%s", ch->color_str( AT_BOARD ) );
   snprintf( s2, 16, "%s", ch->color_str( AT_BOARD2 ) );
   snprintf( s3, 16, "%s", ch->color_str( AT_BOARD3 ) );

   switch ( ch->substate )
   {
      default:
         break;
         /*
          * Handle editor abort 
          */

      case SUB_EDIT_ABORT:
         ch->print( "Aborting note...\r\n" );
         ch->substate = SUB_NONE;
         if( ch->pcdata->pnote )
            deleteptr( ch->pcdata->pnote );
         ch->pcdata->pnote = NULL;
         ch->from_room(  );
         if( !ch->to_room( ch->orig_room ) )
            log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __func__, __LINE__ );
         ch->desc->connected = CON_PLAYING;
         act( AT_GREY, "$n aborts $s note writing.", ch, NULL, NULL, TO_ROOM );
         ch->pcdata->board = NULL;
         return;

      case SUB_BOARD_TEXT:
         if( ch->pcdata->pnote == NULL )
         {
            bug( "%s: SUB_BOARD_TEXT: Null pnote on character (%s)!", __func__, ch->name );
            ch->stop_editing(  );
            return;
         }
         if( ch->pcdata->board == NULL )
         {
            bug( "%s: SUB_BOARD_TEXT: Null board on character (%s)!", __func__, ch->name );
            ch->stop_editing(  );
            return;
         }
         DISPOSE( ch->pcdata->pnote->text );
         ch->pcdata->pnote->text = ch->copy_buffer( false );
         ch->stop_editing(  );
         if( ch->pcdata->pnote->text[0] == '\0' )
         {
            ch->print( "You must enter some text in the body of the note!\r\n" );
            ch->substate = SUB_BOARD_TEXT;
            if( !ch->pcdata->pnote->text )
               ch->pcdata->pnote->text = str_dup( "" );
            ch->editor_desc_printf( "A note to %s about %s", ch->pcdata->pnote->to_list, ch->pcdata->pnote->subject );
            ch->start_editing( ch->pcdata->pnote->text );
            return;
         }
         note_to_char( ch, ch->pcdata->pnote, NULL, 0 );
         ch->printf( "%sIs this correct? %s(%sY%s/%sN%s)&D   ", s1, s3, s2, s3, s2, s3 );
         ch->desc->connected = CON_BOARD;
         ch->substate = SUB_BOARD_CONFIRM;
         return;
   }

   /*
    * Stop people from dropping into the subsystem to escape a fight 
    */
   if( ch->fighting )
   {
      ch->print( "Don't you think it best to finish your fight first?\r\n" );
      return;
   }

   if( !( board = find_board( ch ) ) )
   {
      if( argument.empty(  ) )
      {
         ch->printf( "%sWrites a new message for a board.\r\n%sSyntax: %swrite %s<%sboard%s> [%ssubject%s/%snote#%s]&D\r\n", s1, s3, s1, s3, s2, s3, s2, s3, s2, s3 );
         ch->printf( "%sNote: Subject and Note# are optional, but you can not specify both.&D\r\n", s1 );
         return;
      }

      argument = one_argument( argument, arg );
      if( !( board = get_board( ch, arg ) ) )
      {
         ch->printf( "%sNo board found!&D\r\n", s1 );
         return;
      }
   }
   else
      ch->printf( "%sUsing current board in room: %s%s&D\r\n", s1, s2, board->name );

   if( !can_post( ch, board ) )
   {
      ch->print( "You can only read these notes.\r\n" );
      return;
   }

   if( is_number( argument ) )
      n_num = atoi( argument.c_str(  ) );

   ch->pcdata->board = board;
   snprintf( buf, MSL, "%d", ROOM_VNUM_BOARD );
   board_room = ch->find_location( buf );
   if( !board_room )
   {
      bug( "%s: Missing board room: Vnum %d", __func__, ROOM_VNUM_BOARD );
      return;
   }
   ch->printf( "%sTyping '%s/a%s' at any time will abort the note.&D\r\n", s3, s2, s3 );

   if( n_num )
   {
      list < note_data * >::iterator inote;
      note_data *pnote = NULL;
      int i = 1;
      for( inote = board->nlist.begin(  ); inote != board->nlist.end(  ); ++inote )
      {
         note_data *note = *inote;

         if( IS_BOARD_FLAG( board, BOARD_PRIVATE ) && !is_note_to( ch, note ) )
            continue;
         if( i == n_num )
         {
            pnote = note;
            break;
         }
         ++i;
      }

      if( !pnote )
      {
         ch->print( "Note not found!\r\n" );
         return;
      }

      if( IS_NOTE_FLAG( pnote, NOTE_CLOSED ) && !can_remove( ch, board ) )
      {
         ch->print( "&[board]That note has been closed, no further replies can be made.&D\r\n" );
         return;
      }

      ch->printf( "%sYou begin to write a reply for %s%s's%s note '%s%s%s'.&D\r\n", s1, s2, pnote->sender, s1, s2, pnote->subject, s1 );
      act( AT_GREY, "$n departs for a moment, replying to a note.", ch, NULL, NULL, TO_ROOM );
      ch->note_attach(  );
      if( IS_BOARD_FLAG( board, BOARD_PRIVATE ) )
      {
         STRFREE( ch->pcdata->pnote->to_list );
         ch->pcdata->pnote->to_list = STRALLOC( pnote->sender );
      }
      ch->pcdata->pnote->parent = pnote;
      DISPOSE( ch->pcdata->pnote->subject );
      strdup_printf( &ch->pcdata->pnote->subject, "Re: %s", ch->pcdata->pnote->parent->subject );
      ch->desc->connected = CON_BOARD;
      if( IS_BOARD_FLAG( board, BOARD_PRIVATE ) )
      {
         ch->substate = SUB_BOARD_TEXT;
         ch->printf( "%sTo: %s%-15s %sFrom: %s%s&D\r\n", s1, s2, ch->pcdata->pnote->to_list, s1, s2, ch->pcdata->pnote->sender );
         ch->printf( "%sSubject: %s%s&D\r\n", s1, s2, ch->pcdata->pnote->subject );
         ch->printf( "%sPlease enter the text for your message:&D\r\n", s1 );
         if( !ch->pcdata->pnote->text )
            ch->pcdata->pnote->text = str_dup( "" );
         ch->editor_desc_printf( "A note to %s about %s", ch->pcdata->pnote->to_list, ch->pcdata->pnote->subject );
         ch->start_editing( ch->pcdata->pnote->text );
      }
      else
      {
         ch->printf( "%sTo whom is this note addressed? %s(%sDefault: %s%s%s)&D   ", s1, s3, s1, s2, pnote->sender, s3 );
         ch->substate = SUB_BOARD_TO;
      }
      ch->orig_room = ch->in_room;
      ch->from_room(  );
      if( !ch->to_room( board_room ) )
         log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __func__, __LINE__ );
      return;
   }

   ch->note_attach(  );
   if( !argument.empty(  ) )
   {
      DISPOSE( ch->pcdata->pnote->subject );
      ch->pcdata->pnote->subject = str_dup( argument.c_str(  ) );
      ch->printf( "%sYou begin to write a new note for the %s%s%s board, titled '%s%s%s'.&D\r\n", s1, s2, board->name, s1, s2, ch->pcdata->pnote->subject, s1 );
   }
   else
      ch->printf( "%sYou begin to write a new note for the %s%s%s board.&D\r\n", s1, s2, board->name, s1 );

   if( can_remove( ch, board ) && !IS_BOARD_FLAG( board, BOARD_PRIVATE ) && ch->pcdata->board->expire > 0 )
   {
      ch->printf( "%sIs this a sticky note? %s(%sY%s/%sN%s)  (%sDefault: %sN%s)&D   ", s1, s3, s2, s3, s2, s3, s1, s2, s3 );
      ch->substate = SUB_BOARD_STICKY;
   }
   else
   {
      if( ch->pcdata->board->expire == 0 )
         TOGGLE_NOTE_FLAG( ch->pcdata->pnote, NOTE_STICKY );
      else
         ch->pcdata->pnote->expire = ch->pcdata->board->expire;
      ch->substate = SUB_BOARD_TO;
      if( IS_BOARD_FLAG( board, BOARD_PRIVATE ) && !can_remove( ch, board ) )
         ch->printf( "%sTo whom is this note addressed?&D   ", s1 );
      else
         ch->printf( "%sTo whom is this note addressed? %s(%sDefault: %sAll%s)&D   ", s1, s3, s1, s2, s3 );
   }
   act( AT_GREY, "$n begins to write a new note.", ch, NULL, NULL, TO_ROOM );
   ch->desc->connected = CON_BOARD;
   ch->orig_room = ch->in_room;
   ch->from_room(  );
   if( !ch->to_room( board_room ) )
      log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __func__, __LINE__ );
}

CMDF( do_note_read )
{
   board_data *board = NULL;
   list < board_data * >::iterator bd;
   list < note_data * >::iterator inote;
   note_data *pnote = NULL;
   board_chardata *pboard = NULL;
   int n_num = 0, i = 1;
   string arg;
   char s1[16], s2[16], s3[16];

   if( ch->isnpc(  ) )
      return;

   snprintf( s1, 16, "%s", ch->color_str( AT_BOARD ) );
   snprintf( s2, 16, "%s", ch->color_str( AT_BOARD2 ) );
   snprintf( s3, 16, "%s", ch->color_str( AT_BOARD3 ) );

   // Now specifying "any" will get around local-board checks. -X (3-23-05)
   if( argument.empty(  ) || !str_cmp( argument, "any" ) )
   {
      if( !( board = find_board( ch ) ) )
      {
         board = ch->pcdata->board;
         if( !board )
         {
            for( bd = bdlist.begin(  ); bd != bdlist.end(  ); ++bd )
            {
               board = *bd;   // Added to fix crashbug -- X (3-23-05)

               // Immortals and Moderators can read from anywhere as intended... -X (3-23-05)
               if( ( !can_remove( ch, board ) || !ch->is_immortal(  ) ) && board->objvnum > 0 )
                  continue;
               if( !can_read( ch, board ) )
                  continue;
               pboard = get_chboard( ch, board->name );
               if( pboard && pboard->alert == BD_IGNORE )
                  continue;
               if( unread_notes( ch, board ) > 0 )
                  break;
            }
            if( bd == bdlist.end(  ) )
            {
               ch->printf( "%sThere are no boards with unread messages&D\r\n", s1 );
               return;
            }
         }
      }

      pboard = create_chboard( ch, board->name );

      for( inote = board->nlist.begin(  ); inote != board->nlist.end(  ); ++inote )
      {
         note_data *note = *inote;

         if( IS_BOARD_FLAG( board, BOARD_PRIVATE ) && !is_note_to( ch, note ) )
            continue;
         ++n_num;
         if( note->date_stamp > pboard->last_read )
         {
            pnote = note;
            break;
         }
      }

      if( !pnote )
      {
         ch->printf( "%sThere are no more unread messages on this board.&D\r\n", s1 );
         ch->pcdata->board = NULL;
         return;
      }

      if( IS_NOTE_FLAG( pnote, NOTE_HIDDEN ) && !can_remove( ch, board ) )
      {
         ch->print( "&[board]Sorry, that note has been hidden by the moderators.&D\r\n" );
         return;
      }

      note_to_char( ch, pnote, board, n_num );
      pboard->last_read = pnote->date_stamp;
      act( AT_GREY, "$n reads a note.", ch, NULL, NULL, TO_ROOM );
      return;
   }

   if( !( board = find_board( ch ) ) )
   {
      argument = one_argument( argument, arg );
      if( !( board = get_board( ch, arg ) ) )
      {
         ch->printf( "%sNo board found!&D\r\n", s1 );
         return;
      }
   }
   else
      ch->printf( "%sUsing current board in room: %s%s&D\r\n", s1, s2, board->name );

   if( !can_read( ch, board ) )
   {
      ch->printf( "%sYou are unable to comprehend the notes on this board.&D\r\n", s1 );
      return;
   }

   n_num = atoi( argument.c_str(  ) );

   /*
    * If we have a board, but no note specified, then treat it as if we're just scanning through 
    */
   if( n_num == 0 )
   {
      if( board->objvnum == 0 )
         ch->pcdata->board = board;
      interpret( ch, "read" );
      return;
   }

   i = 1;
   for( inote = board->nlist.begin(  ); inote != board->nlist.end(  ); ++inote )
   {
      note_data *note = *inote;

      if( IS_BOARD_FLAG( board, BOARD_PRIVATE ) && !is_note_to( ch, note ) && !can_remove( ch, board ) )
         continue;
      if( i == n_num )
      {
         pnote = note;
         break;
      }
      ++i;
   }

   if( !pnote )
   {
      ch->printf( "%sNote #%s%d%s not found on the %s%s%s board.&D\r\n", s1, s2, n_num, s1, s2, board->name, s1 );
      return;
   }

   if( IS_NOTE_FLAG( pnote, NOTE_HIDDEN ) && !can_remove( ch, board ) )
   {
      ch->print( "&[board]Sorry, that note has been hidden by the moderators.&D\r\n" );
      return;
   }

   note_to_char( ch, pnote, board, n_num );
   pboard = create_chboard( ch, board->name );
   if( pboard->last_read < pnote->date_stamp )
      pboard->last_read = pnote->date_stamp;
   act( AT_GREY, "$n reads a note.", ch, NULL, NULL, TO_ROOM );
}

CMDF( do_note_list )
{
   board_data *board = NULL;
   board_chardata *chboard;
   char buf[MSL];
   int count = 0;
   char s1[16], s2[16], s3[16];

   snprintf( s1, 16, "%s", ch->color_str( AT_BOARD ) );
   snprintf( s2, 16, "%s", ch->color_str( AT_BOARD2 ) );
   snprintf( s3, 16, "%s", ch->color_str( AT_BOARD3 ) );

   if( !( board = find_board( ch ) ) )
   {
      if( argument.empty(  ) )
      {
         ch->printf( "%sLists the note on a board.\r\n%sSyntax: %sreview %s<%sboard%s>&D\r\n", s1, s3, s1, s3, s2, s3 );
         return;
      }

      if( !( board = get_board( ch, argument ) ) )
      {
         ch->printf( "%sNo board found!&D\r\n", s1 );
         return;
      }
   }
   else
      ch->printf( "%sUsing current board in room: %s%s&D\r\n", s1, s2, board->name );

   if( !can_read( ch, board ) )
   {
      ch->print( "You are unable to comprehend the notes on this board.\r\n" );
      return;
   }

   chboard = get_chboard( ch, board->name );

   snprintf( buf, MSL, "%s--[ %sNotes on %s%s%s ]--", s3, s1, s2, board->name, s3 );
   ch->printf( "\r\n%s\r\n", color_align( buf, 80, ALIGN_CENTER ) );
   act_printf( AT_GREY, ch, NULL, NULL, TO_ROOM, "&w$n reviews the notes on the &W%s&w board.", board->name );

   if( total_notes( ch, board ) == 0 )
   {
      ch->print( "\r\nNo messages...\r\n" );
      return;
   }
   else
      ch->printf( "%sNum   %s%-17s %-11s %s&D\r\n", s1, IS_BOARD_FLAG( board, BOARD_PRIVATE ) ? "" : "Replies ", "Date", "Author", "Subject" );

   count = 0;
   list < note_data * >::iterator inote;
   for( inote = board->nlist.begin(  ); inote != board->nlist.end(  ); ++inote )
   {
      note_data *note = *inote;
      char unread[4];

      if( IS_BOARD_FLAG( board, BOARD_PRIVATE ) && !is_note_to( ch, note ) && !can_remove( ch, board ) )
         continue;

      ++count;

      mudstrlcpy( unread, " ", 4 );
      if( !chboard || chboard->last_read < note->date_stamp )
         mudstrlcpy( unread, "&C*", 4 );

      if( IS_NOTE_FLAG( note, NOTE_HIDDEN ) && !can_remove( ch, board ) )
         continue;

      if( IS_NOTE_FLAG( note, NOTE_CLOSED ) )
         mudstrlcpy( unread, "&Y-", 4 );

      if( IS_NOTE_FLAG( note, NOTE_HIDDEN ) )
         mudstrlcpy( unread, "&R#", 4 );

      if( IS_BOARD_FLAG( board, BOARD_PRIVATE ) )
      {
         ch->printf( "%s%2d%s) %s %s[%s%-15s%s] %s%-11s %s&D\r\n", s2, count, s3,
                     unread, s3, s2,
                     mini_c_time( note->date_stamp, ch->pcdata->timezone ), s3, s2,
                     note->sender ? note->sender : "--Error--", note->subject ? print_lngstr( note->subject, 37 ).c_str(  ) : "" );
      }
      else
      {
         ch->printf( "%s%2d%s) %s %s[ %s%3d%s ] [%s%-15s%s] %s%-11s %-20s&D\r\n", s2, count, s3,
                     unread, s3, s2,
                     note->reply_count, s3, s2, mini_c_time( note->date_stamp, ch->pcdata->timezone ), s3, s2,
                     note->sender ? note->sender : "--Error--", note->subject ? print_lngstr( note->subject, 45 ).c_str(  ) : "" );
      }
   }
   ch->printf( "\r\n%sThere %s %s%d%s message%s on this board.&D\r\n", s1, count == 1 ? "is" : "are", s2, count, s1, count == 1 ? "" : "s" );
   ch->printf( "%sA &C*%s denotes unread messages, while &Y-&[board] indicates a closed note.&D\r\n", s1, s1 );
   if( can_remove( ch, board ) )
      ch->print( "&[board]A &R#&[board] denotes a hidden message.&D\r\n" );
}

CMDF( do_note_remove )
{
   board_data *board;
   note_data *pnote = NULL;
   string arg;
   short n_num = 0, r_num = 0, i = 0;

   if( !( board = find_board( ch ) ) )
   {
      if( argument.empty(  ) )
      {
         ch->
            print
            ( "&[board]Removes a note from the board.\r\n&[board3]Syntax: &[board]erase &[board3]<&[board2]board&[board3]> <&[board2]note#&[board3]>.[&[board2]reply#&[board3]]&D\r\n" );

         return;
      }

      argument = one_argument( argument, arg );
      if( !( board = get_board( ch, arg ) ) )
      {
         ch->print( "&[board]No board found!&D\r\n" );
         return;
      }
   }
   else
      ch->printf( "&[board]Using current board in room: &[board2]%s&D\r\n", board->name );

   if( !IS_BOARD_FLAG( board, BOARD_PRIVATE ) && !can_remove( ch, board ) )
   {
      ch->print( "&[board]You are unable to remove the notes on this board.&D\r\n" );
      return;
   }

   if( argument.empty(  ) )
   {
      ch->print( "&[board]Remove which note?&D\r\n" );
      return;
   }

   string::size_type pos = argument.find( "." );
   if( pos == string::npos )
   {
      n_num = atoi( argument.c_str(  ) );
      r_num = -1;
   }
   else
   {
      n_num = atoi( argument.substr( 0, pos - 1 ).c_str(  ) );
      r_num = atoi( argument.substr( pos + 1, argument.length(  ) ).c_str(  ) );
   }

   if( n_num == 0 )
   {
      ch->print( "&[board]Remove which note?&D\r\n" );
      return;
   }

   if( r_num == 0 )
   {
      ch->print( "&[board]Remove which reply?&D\r\n" );
      return;
   }

   i = 1;
   list < note_data * >::iterator inote;
   for( inote = board->nlist.begin(  ); inote != board->nlist.end(  ); ++inote )
   {
      note_data *note = *inote;

      if( IS_BOARD_FLAG( board, BOARD_PRIVATE ) && !is_note_to( ch, note ) && !can_remove( ch, board ) )
         continue;
      if( i == n_num )
      {
         pnote = note;
         break;
      }
      ++i;
   }

   if( !pnote )
   {
      ch->print( "&[board]Note not found!&D\r\n" );
      return;
   }

   if( hasname( pnote->to_list, "all" ) && !can_remove( ch, board ) )
   {
      ch->print( "&[board]You can not remove that note.&D\r\n" );
      return;
   }

   if( r_num > 0 )
   {
      note_data *reply = NULL;
      list < note_data * >::iterator rp;

      i = 1;
      for( rp = pnote->rlist.begin(  ); rp != pnote->rlist.end(  ); ++rp )
      {
         note_data *rpy = *rp;

         if( i == r_num )
         {
            reply = rpy;
            break;
         }
         else
            ++i;
      }

      if( !reply )
      {
         ch->print( "&[board]Reply not found!&D\r\n" );
         return;
      }

      ch->printf( "&[board]You remove the reply from &[board2]%s&[board], titled '&[board2]%s&[board]' from the &[board2]%s&[board] board.&D\r\n",
                  reply->sender ? reply->sender : "--Error--", reply->subject ? reply->subject : "--Error--", board->name );
      note_remove( board, reply );
      act( AT_BOARD, "$n removes a reply from the board.", ch, NULL, NULL, TO_ROOM );
      return;
   }

   ch->printf( "&[board]You remove the note from &[board2]%s&[board], titled '&[board2]%s&[board]' from the &[board2]%s&[board] board.&D\r\n",
               pnote->sender ? pnote->sender : "--Error--", pnote->subject ? pnote->subject : "--Error--", board->name );
   note_remove( board, pnote );
   act( AT_BOARD, "$n removes a note from the board.", ch, NULL, NULL, TO_ROOM );
}

CMDF( do_board_list )
{
   obj_data *obj;
   char buf[MSL];
   int count = 0;

   if( !str_cmp( argument, "flaghelp" ) && ch->is_immortal(  ) )
   {
      ch->print( "&[board2]P&[board]: Private Board. Readers can only read posts addressed to them.\r\n   Used commonly with mail boards.\r\n" );
      ch->print( "&[board2]A&[board]: Announce Board. New posts are announced to all players.\r\n   Used commonly with news boards.\r\n" );
      ch->
         print
         ( "&[board2]B&[board]: Backup/Archived Board. Expired posts are automatically archived.\r\n   If this flag is not set, expired posts are deleted for good.\r\n" );
      ch->print( "&[board2]-&[board]: Flag place-holder\r\n" );
      return;
   }

   if( !str_cmp( argument, "info" ) && ch->is_immortal(  ) )
   {
      ch->print( "&[board]                                                                Read Post Rmv&D\r\n" );
      ch->print( "&[board]Num Name                      Filename        Flags Obj#  Room# Lvl  Lvl  Lvl&D\r\n" );
   }
   else
   {
      ch->print( "&[board]                              Total   Total&D\r\n" );
      ch->print( "&[board]Num  Name             Unread  Posts  Replies Description&D\r\n" );
   }

   list < board_data * >::iterator bd;
   for( bd = bdlist.begin(  ); bd != bdlist.end(  ); ++bd )
   {
      board_data *board = *bd;

      /*
       * If you can't read the board, you can't see it either. 
       */
      if( !can_read( ch, board ) )
         continue;

      /*
       * Not a global board and you're not a moderator or immortal, you can't see it. 
       */
      if( board->objvnum > 0 && ( !can_remove( ch, board ) && !ch->is_immortal(  ) ) )
         continue;

      /*
       * Everyone who can see it sees this same information
       */

      if( !str_cmp( argument, "info" ) && ch->is_immortal(  ) )
      {
         ch->printf( "&[board2]%2d&[board3]) &[board2]%-25s ", count + 1, print_lngstr( board->name, 25 ).c_str(  ) );
         ch->printf( "%-15s ", print_lngstr( board->filename, 15 ).c_str(  ) );
         if( IS_BOARD_FLAG( board, BOARD_PRIVATE ) )
            ch->print( "P" );
         else
            ch->print( "-" );

         if( IS_BOARD_FLAG( board, BOARD_ANNOUNCE ) )
            ch->print( "A" );
         else
            ch->print( "-" );

         if( IS_BOARD_FLAG( board, BOARD_BU_PRUNED ) )
            ch->print( "B" );
         else
            ch->print( "-" );
         ch->print( "-- " );
         if( board->objvnum )
         {
            ch->printf( "%-5d ", board->objvnum );
            snprintf( buf, MSL, "%d", board->objvnum );
            if( ( obj = ch->get_obj_world( buf ) ) && ( obj->in_room != NULL ) )
               ch->printf( "%-5d ", obj->in_room->vnum );
            else
               ch->print( "----- " );
         }
         else
            ch->print( " N/A   N/A  " );
         ch->printf( "%-3d  %-3d  %-3d", board->read_level, board->post_level, board->remove_level );
      }
      else
      {
         ch->printf( "&[board2]%2d&[board3])  &[board2]%-15s  &[board3][ &[board2]%3d&[board3]]  ", count + 1, print_lngstr( board->name, 15 ).c_str(  ),
                     unread_notes( ch, board ) );
         if( IS_BOARD_FLAG( board, BOARD_PRIVATE ) )
            ch->printf( "[&[board2]%3d&[board3]]          ", total_notes( ch, board ) );
         else
            ch->printf( "[&[board2]%3d&[board3]]  [&[board2]%3d&[board3]]   ", total_notes( ch, board ), total_replies( board ) );

         ch->printf( "&[board2]%-25s", board->desc ? print_lngstr( board->desc, 25 ).c_str(  ) : "" );

         if( ch->is_immortal(  ) )
         {
            if( board->objvnum )
               ch->print( " &[board3][&[board2]Local&[board3]]" );
         }
      }
      ch->print( "&D\r\n" );
      ++count;
   }
   if( count == 0 )
      ch->print( "&[board]No boards to list.&D\r\n" );
   else
   {
      ch->printf( "\r\n&[board2]%d&[board] board%s found.&D\r\n", count, count == 1 ? "" : "s" );
      if( ch->is_immortal(  ) && str_cmp( argument, "info" ) )
         ch->print( "&[board3](&[board]Use &[board2]BOARDS INFO&[board] to display Immortal information.&[board3])\r\n" );
      else if( !str_cmp( argument, "info" ) && ch->is_immortal(  ) )
         ch->print( "&[board3](&[board]Use &[board2]BOARDS FLAGHELP&[board] for information on the flag letters.&[board3])\r\n" );
   }
}

CMDF( do_board_alert )
{
   board_chardata *chboard;
   board_data *board = NULL;
   string arg;
   int bd_value = -1;
   char s1[16], s2[16], s3[16];
   static const char* bd_alert_string[] = { "None Set", "Announce", "Ignoring" };

   snprintf( s1, 16, "%s", ch->color_str( AT_BOARD ) );
   snprintf( s2, 16, "%s", ch->color_str( AT_BOARD2 ) );
   snprintf( s3, 16, "%s", ch->color_str( AT_BOARD3 ) );

   if( argument.empty(  ) )
   {
      // Fixed crash bug here - X (3-23-05)
      list < board_data * >::iterator bd;
      for( bd = bdlist.begin(  ); bd != bdlist.end(  ); ++bd )
      {
         board = *bd;

         if( !can_read( ch, board ) )
            continue;
         chboard = get_chboard( ch, board->name );
         ch->printf( "%s%-20s   %sAlert: %s%s&D\r\n", s2, board->name, s1, s2, chboard ? bd_alert_string[chboard->alert] : bd_alert_string[0] );
      }
      ch->printf( "%sTo change an alert for a board, type: %salert <board> <none|announce|ignore>&D\r\n", s1, s2 );
      return;
   }

   argument = one_argument( argument, arg );
   if( !( board = get_board( ch, arg ) ) )
   {
      ch->printf( "%sSorry, but the board '%s%s%s' does not exsist.&D\r\n", s1, s2, arg.c_str(  ), s1 );
      return;
   }

   if( !str_cmp( argument, "none" ) )
      bd_value = 0;
   if( !str_cmp( argument, "announce" ) )
      bd_value = 1;
   if( !str_cmp( argument, "ignore" ) )
      bd_value = 2;

   if( bd_value == -1 )
   {
      ch->printf( "%sSorry, but '%s%s%s' is not a valid argument.&D\r\n", s1, s2, argument.c_str(  ), s1 );
      ch->printf( "%sPlease choose one of: %snone announce ignore&D\r\n", s1, s2 );
      return;
   }

   chboard = create_chboard( ch, board->name );
   chboard->alert = bd_value;
   ch->printf( "%sAlert for the %s%s%s board set to %s%s%s.&D\r\n", s1, s2, board->name, s1, s2, argument.c_str(  ), s1 );
}

/* Much like do_board_list, but I cut out some of the details here for simplicity */
CMDF( do_checkboards )
{
   list < board_data * >::iterator bd;
   board_chardata *chboard;
   obj_data *obj;
   int count = 0;
   char s1[16], s2[16], s3[16];

   snprintf( s1, 16, "%s", ch->color_str( AT_BOARD ) );
   snprintf( s2, 16, "%s", ch->color_str( AT_BOARD2 ) );
   snprintf( s3, 16, "%s", ch->color_str( AT_BOARD3 ) );

   ch->printf( "%s Num  %-20s  Unread  %-40s&D\r\n", s1, "Name", "Description" );

   for( bd = bdlist.begin(  ); bd != bdlist.end(  ); ++bd )
   {
      board_data *board = *bd;

      /*
       * If you can't read the board, you can't see it either. 
       */
      if( !can_read( ch, board ) )
         continue;

      /*
       * Not a global board and you're not a moderator or immortal, you can't see it. 
       */
      if( board->objvnum > 0 && ( !can_remove( ch, board ) && !ch->is_immortal(  ) ) )
         continue;

      /*
       * We only want to see boards with NEW posts 
       */
      if( unread_notes( ch, board ) == 0 )
         continue;

      chboard = get_chboard( ch, board->name );
      /*
       * If we're ignoring it, then who cares? Well we do if the Immortals set it to ANNOUNCE 
       */
      if( chboard && chboard->alert == BD_IGNORE && !IS_BOARD_FLAG( board, BOARD_ANNOUNCE ) )
         continue;
      ++count;
      /*
       * Everyone who can see it sees this same information 
       */
      ch->printf( "%s%3d%s)  %s%-20s  %s[%s%3d%s]  %s%-30s", s2, count, s3, s2, board->name, s3, s2, unread_notes( ch, board ), s3, s2, board->desc ? board->desc : "" );

      if( ch->is_immortal(  ) )
      {
         char buf[MSL];

         snprintf( buf, MSL, "%d", board->objvnum );
         if( ( obj = ch->get_obj_world( buf ) ) && ( obj->in_room != NULL ) )
            ch->printf( " %s[%sObj# %s%-5d %s@ %sRoom# %s%-5d%s]", s3, s1, s2, board->objvnum, s3, s1, s2, obj->in_room->vnum, s3 );
         else
            ch->printf( " %s[ %sGlobal Board %s]", s3, s2, s3 );
      }
      ch->print( "&D\r\n" );
   }
   if( count == 0 )
      ch->printf( "%sNo unread messages...\r\n", s1 );
}

CMDF( do_board_stat )
{
   board_data *board;
   char s1[16], s2[16], s3[16];

   snprintf( s1, 16, "%s", ch->color_str( AT_BOARD ) );
   snprintf( s2, 16, "%s", ch->color_str( AT_BOARD2 ) );
   snprintf( s3, 16, "%s", ch->color_str( AT_BOARD3 ) );

   if( argument.empty(  ) )
   {
      ch->print( "Usage: bstat <board>\r\n" );
      return;
   }

   if( !( board = get_board( ch, argument ) ) )
   {
      ch->printf( "&wSorry, '&W%s&w' is not a valid board.\r\n", argument.c_str(  ) );
      return;
   }

   ch->printf( "%sFilename: %s%-20s&D\r\n", s1, s2, board->filename );
   ch->printf( "%sName:       %s%-30s%s ObjVnum:      ", s1, s2, board->name, s1 );
   if( board->objvnum > 0 )
      ch->printf( "%s%d&D\r\n", s2, board->objvnum );
   else
      ch->printf( "%sGlobal Board&D\r\n", s2 );
   ch->printf( "%sReaders:    %s%-30s%s Read Level:   %s%d&D\r\n", s1, s2, board->readers ? board->readers : "none set", s1, s2, board->read_level );
   ch->printf( "%sPosters:    %s%-30s%s Post Level:   %s%d&D\r\n", s1, s2, board->posters ? board->posters : "none set", s1, s2, board->post_level );
   ch->printf( "%sModerators: %s%-30s%s Remove Level: %s%d&D\r\n", s1, s2, board->moderators ? board->moderators : "none set", s1, s2, board->remove_level );
   ch->printf( "%sGroup:      %s%-30s%s Expiration:   %s%d&D\r\n", s1, s2, board->group ? board->group : "none set", s1, s2, board->expire );
   ch->printf( "%sFlags: %s[%s%s%s]&D\r\n", s1, s3, s2, board->flags.any(  )? bitset_string( board->flags, board_flags ) : "none set", s3 );
   ch->printf( "%sDescription: %s%-30s&D\r\n", s1, s2, board->desc ? board->desc : "none set" );
}

CMDF( do_board_remove )
{
   board_data *board;
   string arg;
   char buf[MSL];
   char s1[16], s2[16], s3[16];

   snprintf( s1, 16, "%s", ch->color_str( AT_BOARD ) );
   snprintf( s2, 16, "%s", ch->color_str( AT_BOARD2 ) );
   snprintf( s3, 16, "%s", ch->color_str( AT_BOARD3 ) );

   if( argument.empty(  ) )
   {
      ch->printf( "%sYou must select a board to remove.&D\r\n", s1 );
      return;
   }

   argument = one_argument( argument, arg );
   if( !( board = get_board( ch, arg ) ) )
   {
      ch->printf( "%sSorry, '%s%s%s' is not a valid board.&D\r\n", s1, s2, argument.c_str(  ), s1 );
      return;
   }

   if( !str_cmp( argument, "yes" ) )
   {
      ch->printf( "&RDeleting board '&W%s&R'.&D\r\n", board->name );
      ch->print( "&wDeleting note file...   " );
      snprintf( buf, MSL, "%s%s.board", BOARD_DIR, board->filename );
      unlink( buf );
      ch->print( "&RDeleted&D\r\n&wDeleting board...   " );
      deleteptr( board );
      ch->print( "&RDeleted&D\r\n&wSaving boards...   " );
      write_boards(  );
      ch->print( "&GDone.&D\r\n" );
      return;
   }
   else
   {
      // Changed BOARD REMOVE to DELETEBOARD to reflect default AFKMud command name. -- Xorith (9/21/07)
      ch->printf( "&RRemoving a board will also delete *ALL* posts on that board!\r\n&wTo continue, type: " "&YDELETEBOARD '%s' YES&w.", board->name );
      return;
   }
}

CMDF( do_board_make )
{
   board_data *board;
   string arg;

   argument = one_argument( argument, arg );

   if( argument.empty(  ) || arg.empty(  ) )
   {
      ch->print( "Usage: makeboard <filename> <name>\r\n" );
      return;
   }

   if( argument.length(  ) > 20 || arg.length(  ) > 20 )
   {
      ch->print( "Please limit board names and filenames to 20 characters!\r\n" );
      return;
   }

   smash_tilde( argument );
   board = new board_data;

   board->name = STRALLOC( argument.c_str(  ) );
   board->filename = str_dup( arg.c_str(  ) );
   board->desc = str_dup( "This is a new board!" );
   board->objvnum = 0;
   board->expire = MAX_BOARD_EXPIRE;
   board->read_level = ch->level;
   board->post_level = ch->level;
   board->remove_level = ch->level;
   bdlist.push_back( board );
   write_board( board );
   write_boards(  );

   ch->print( "&wNew board created: \r\n" );
   do_board_stat( ch, board->name );
   ch->print( "&wNote: Boards default to &CGlobal&w when created. See '&Ybset&w' to change this!\r\n" );
}

CMDF( do_board_set )
{
   board_data *board;
   string arg1, arg2, arg3;
   int value = -1;
   char s1[16], s2[16], s3[16];

   snprintf( s1, 16, "%s", ch->color_str( AT_BOARD ) );
   snprintf( s2, 16, "%s", ch->color_str( AT_BOARD2 ) );
   snprintf( s3, 16, "%s", ch->color_str( AT_BOARD3 ) );

   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );

   // Added missing 'expireall', 'raise' and 'lower'. Also added the available flags. --Xorith (9/21/07)
   if( arg1.empty(  ) || ( str_cmp( arg1, "purge" ) && arg2.empty(  ) ) )
   {
      ch->print( "Usage: bset <board> <field> value\r\n" );
      ch->print( "   Or: bset purge (this option forces a check on expired notes)\r\n" );
      ch->print( "\r\nField being one of:\r\n" );
      ch->print( "  objvnum read_level post_level remove_level expire group flags name\r\n" );
      ch->print( "  readers posters moderators desc raise lower global filename expireall\r\n" );
      ch->print( "\r\nFlags: \r\nbackup private announce\r\n" );
      return;
   }

   if( is_number( argument ) )
      value = atoi( argument.c_str(  ) );

   if( !str_cmp( arg1, "purge" ) )
   {
      log_printf( "Manual board pruning started by %s.", ch->name );
      check_boards(  );
      return;
   }

   if( !( board = get_board( ch, arg1 ) ) )
   {
      ch->printf( "%sSorry, but '%s%s%s' is not a valid board.&D\r\n", s1, s2, arg1.c_str(  ), s1 );
      return;
   }

   if( !str_cmp( arg2, "flags" ) )
   {
      char buf[MSL];

      bool fMatch = false, fUnknown = false;

      ch->printf( "%sSetting flags: %s", s1, s2 );
      snprintf( buf, MSL, "\r\n%sUnknown flags: %s", s1, s2 );
      while( !argument.empty(  ) )
      {
         argument = one_argument( argument, arg3 );
         value = get_board_flag( arg3 );
         if( value < 0 || value >= MAX_BOARD_FLAGS )
         {
            fUnknown = true;
            snprintf( buf + strlen( buf ), MSL - strlen( buf ), " %s", arg3.c_str(  ) );
         }
         else
         {
            fMatch = true;
            TOGGLE_BOARD_FLAG( board, value );
            if( IS_BOARD_FLAG( board, value ) )
               ch->printf( " +%s", arg3.c_str(  ) );
            else
               ch->printf( " -%s", arg3.c_str(  ) );
         }
      }
      ch->printf( "%s%s&D\r\n", fMatch ? "" : "none", fUnknown ? buf : "" );
      write_boards(  );
      return;
   }

   if( !str_cmp( arg2, "objvnum" ) )
   {
      if( !get_obj_index( value ) )
      {
         ch->print( "No such object.\r\n" );
         return;
      }
      board->objvnum = value;
      write_boards(  );
      ch->printf( "%sThe objvnum for '%s%s%s' has been set to '%s%d%s'.&D\r\n", s1, s2, board->name, s1, s2, board->objvnum, s1 );
      return;
   }

   if( !str_cmp( arg2, "global" ) )
   {
      board->objvnum = 0;
      write_boards(  );
      ch->printf( "%s%s%s is now a global board.\r\n", s2, board->name, s1 );
      return;
   }

   if( !str_cmp( arg2, "read_level" ) || !str_cmp( arg2, "post_level" ) || !str_cmp( arg2, "remove_level" ) )
   {
      if( value < 0 || value > ch->level )
      {
         ch->printf( "%s%d%s is out of range.\r\nValues range from %s1%s to %s%d%s.&D\r\n", s2, value, s1, s2, s1, s2, ch->level, s1 );
         return;
      }
      if( !str_cmp( arg2, "read_level" ) )
         board->read_level = value;
      else if( !str_cmp( arg2, "post_level" ) )
         board->post_level = value;
      else if( !str_cmp( arg2, "remove_level" ) )
         board->remove_level = value;
      else
      {
         ch->printf( "%sUnknown field '%s%s%s'.&D\r\n", s1, s2, arg2.c_str(  ), s1 );
         return;
      }
      write_boards(  );
      ch->printf( "%sThe %s%s%s for '%s%s%s' has been set to '%s%d%s'.&D\r\n", s1, s3, arg2.c_str(  ), s1, s2, board->name, s1, s2, value, s1 );
      return;
   }

   if( !str_cmp( arg2, "readers" ) || !str_cmp( arg2, "posters" ) || !str_cmp( arg2, "moderators" ) )
   {
      if( !str_cmp( arg2, "readers" ) )
      {
         STRFREE( board->readers );
         if( !argument.empty(  ) )
            board->readers = STRALLOC( argument.c_str(  ) );
      }
      else if( !str_cmp( arg2, "posters" ) )
      {
         STRFREE( board->posters );
         if( !argument.empty(  ) )
            board->posters = STRALLOC( argument.c_str(  ) );
      }
      else if( !str_cmp( arg2, "moderators" ) )
      {
         STRFREE( board->moderators );
         if( !argument.empty(  ) )
            board->moderators = STRALLOC( argument.c_str(  ) );
      }
      else
      {
         ch->printf( "%sUnknown field '%s%s%s'.&D\r\n", s1, s2, arg2.c_str(  ), s1 );
         return;
      }
      write_boards(  );
      ch->printf( "%sThe %s%s%s for '%s%s%s' have been set to '%s%s%s'.\r\n", s1, s3, arg2.c_str(  ),
                  s1, s2, board->name, s1, s2, !argument.empty(  )? argument.c_str(  ) : "(nothing)", s1 );
      return;
   }

   if( !str_cmp( arg2, "filename" ) )
   {
      char filename[256];

      if( !is_valid_filename( ch, BOARD_DIR, argument ) )
         return;

      if( argument.length(  ) > 20 )
      {
         ch->print( "Please limit board filenames to 20 characters!\n\r" );
         return;
      }
      snprintf( filename, 256, "%s%s.board", BOARD_DIR, board->filename );
      unlink( filename );
      mudstrlcpy( filename, board->filename, 256 );
      DISPOSE( board->filename );
      board->filename = str_dup( argument.c_str(  ) );
      write_boards(  );
      write_board( board );
      ch->printf( "%sThe filename for '%s%s%s' has been changed to '%s%s%s'.\n\r", s1, s2, filename, s1, s2, board->filename, s1 );
      return;
   }

   if( !str_cmp( arg2, "name" ) )
   {
      if( argument.empty(  ) )
      {
         ch->print( "No name specified.\r\n" );
         return;
      }
      if( argument.length(  ) > 20 )
      {
         ch->print( "Please limit board names to 20 characters!\r\n" );
         return;
      }
      write_boards(  );

      ch->printf( "%sThe name for '%s%s%s' has been changed to '%s%s%s'.\r\n", s1, s2, board->name, s1, s2, argument.c_str(  ), s1 );
      STRFREE( board->name );
      board->name = STRALLOC( argument.c_str(  ) );
      return;
   }

   if( !str_cmp( arg2, "expire" ) )
   {
      if( value < 0 || value > MAX_BOARD_EXPIRE )
      {
         ch->printf( "%sExpire time must be a value between %s0%s and %s%d%s.&D\r\n", s1, s2, s1, s2, MAX_BOARD_EXPIRE, s1 );
         return;
      }

      board->expire = value;
      ch->printf( "%sFrom now on, notes on the %s%s%s board will expire after %s%d%s days.\r\n", s1, s2, board->name, s1, s2, board->expire, s1 );
      ch->printf( "%sPlease note: This will not effect notes currently on the board. To effect %sALL%s notes, "
                  "type: %sbset <board> expireall <days>&D\r\n", s1, s3, s1, s2 );
      write_boards(  );
      return;
   }

   if( !str_cmp( arg2, "expireall" ) )
   {
      list < note_data * >::iterator inote;

      if( value < 0 || value > MAX_BOARD_EXPIRE )
      {
         ch->printf( "%sExpire time must be a value between %s0%s and %s%d%s.&D\r\n", s1, s2, s1, s2, MAX_BOARD_EXPIRE, s1 );
         return;
      }

      board->expire = value;
      for( inote = board->nlist.begin(  ); inote != board->nlist.end(  ); ++inote )
      {
         note_data *note = *inote;

         note->expire = value;
      }
      ch->printf( "%sAll notes on the %s%s%s board will expire after %s%d%s days.&D\r\n", s1, s2, board->name, s1, s2, board->expire, s1 );
      ch->print( "Performing board pruning now...\r\n" );
      board_check_expire( board );
      write_boards(  );
      return;
   }

   if( !str_cmp( arg2, "desc" ) )
   {
      if( argument.empty(  ) )
      {
         ch->print( "No description specified.\r\n" );
         return;
      }
      if( argument.length(  ) > 30 )
      {
         ch->print( "Please limit your board descriptions to 30 characters.\r\n" );
         return;
      }
      DISPOSE( board->desc );
      board->desc = str_dup( argument.c_str(  ) );
      write_boards(  );
      ch->printf( "%sThe desc for %s%s%s has been set to '%s%s%s'.&D\r\n", s1, s2, board->name, s1, s2, board->desc, s1 );
      return;
   }

   if( !str_cmp( arg2, "group" ) )
   {
      if( argument.empty(  ) )
      {
         STRFREE( board->group );
         ch->print( "Group cleared.\r\n" );
         return;
      }
      STRFREE( board->group );
      board->group = STRALLOC( argument.c_str(  ) );
      write_boards(  );
      ch->printf( "%sThe group for %s%s%s has been set to '%s%s%s'.&D\r\n", s1, s2, board->name, s1, s2, board->group, s1 );
      return;
   }

   if( !str_cmp( arg2, "raise" ) )
   {
      list < board_data * >::iterator bd;

      for( bd = bdlist.begin(  ); bd != bdlist.end(  ); ++bd )
      {
         if( *bd == board )
            break;
      }
      if( bd == bdlist.begin(  ) )
      {
         ch->printf( "%sBut '%s%s%s' is already the first board!&D\r\n", s1, s2, board->name, s1 );
         return;
      }
      --bd;

      bdlist.remove( board );
      bdlist.insert( bd, board );

      ch->printf( "%sMoved '%s%s%s' above '%s%s%s'.&D\r\n", s1, s2, board->name, s1, s2, ( *bd )->name, s1 );
      return;
   }

   if( !str_cmp( arg2, "lower" ) )
   {
      list < board_data * >::iterator bd;

      for( bd = bdlist.begin(  ); bd != bdlist.end(  ); ++bd )
      {
         if( *bd == board )
            break;
      }

      if( ++bd == bdlist.end(  ) )
      {
         ch->printf( "%sBut '%s%s%s' is already the last board!&D\r\n", s1, s2, board->name, s1 );
         return;
      }
      --bd;

      ++bd;
      ++bd;
      if( bd == bdlist.end(  ) )
      {
         bdlist.remove( board );
         bdlist.push_back( board );
      }
      else
      {
         --bd;
         --bd;

         bdlist.remove( board );
         bdlist.insert( bd, board );
      }
      ch->printf( "%sMoved '%s%s%s' below '%s%s%s'.&D\r\n", s1, s2, board->name, s1, s2, ( *bd )->name, s1 );
      return;
   }
   do_board_set( ch, "" );
}

/* Begin Project Code */
project_data *get_project_by_number( int pnum )
{
   list < project_data * >::iterator pr;
   int pcount = 1;

   for( pr = projlist.begin(  ); pr != projlist.end(  ); ++pr )
   {
      project_data *proj = *pr;

      if( pcount == pnum )
         return proj;
      else
         ++pcount;
   }
   return NULL;
}

note_data *get_log_by_number( project_data * pproject, int pnum )
{
   list < note_data * >::iterator ilog;
   int pcount = 1;

   for( ilog = pproject->nlist.begin(  ); ilog != pproject->nlist.end(  ); ++ilog )
   {
      note_data *plog = *ilog;

      if( pcount == pnum )
         return plog;
      else
         ++pcount;
   }
   return NULL;
}

void write_projects( void )
{
   list < project_data * >::iterator pr;
   list < note_data * >::iterator ilog;
   FILE *fpout;

   fpout = fopen( PROJECTS_FILE, "w" );
   if( !fpout )
   {
      bug( "%s: FATAL: cannot open projects.txt for writing!", __func__ );
      return;
   }
   for( pr = projlist.begin(  ); pr != projlist.end(  ); ++pr )
   {
      project_data *proj = *pr;

      fprintf( fpout, "%s", "Version        2\n" );
      fprintf( fpout, "Name   %s~\n", proj->name );
      fprintf( fpout, "Owner  %s~\n", ( proj->owner ) ? proj->owner : "None" );
      if( proj->coder )
         fprintf( fpout, "Coder  %s~\n", proj->coder );
      fprintf( fpout, "Status %s~\n", ( proj->status ) ? proj->status : "No update." );
      fprintf( fpout, "Date_stamp   %ld\n", proj->date_stamp );
      if( !proj->realm_name.empty() )
         fprintf( fpout, "Realm       %s~\n", proj->realm_name.c_str() );
      if( proj->description )
         fprintf( fpout, "Description %s~\n", proj->description );

      for( ilog = proj->nlist.begin(  ); ilog != proj->nlist.end(  ); ++ilog )
      {
         note_data *nlog = *ilog;

         fprintf( fpout, "%s\n", "Log" );
         fwrite_note( nlog, fpout );
      }
      fprintf( fpout, "%s\n", "End" );
   }
   FCLOSE( fpout );
}

note_data *read_old_log( FILE * fp )
{
   note_data *nlog = new note_data;
   char *word;

   for( ;; )
   {
      word = fread_word( fp );

      if( !str_cmp( word, "Sender" ) )
         nlog->sender = fread_string( fp );
      else if( !str_cmp( word, "Date" ) )
         fread_to_eol( fp );
      else if( !str_cmp( word, "Subject" ) )
         nlog->subject = fread_string_nohash( fp );
      else if( !str_cmp( word, "Text" ) )
         nlog->text = fread_string_nohash( fp );
      else if( !str_cmp( word, "Endlog" ) )
      {
         fread_to_eol( fp );
         nlog->date_stamp = current_time;
         return nlog;
      }
      else
      {
         deleteptr( nlog );
         bug( "%s: bad key word: %s", __func__, word );
         return NULL;
      }
   }
}

project_data *read_project( FILE * fp )
{
   char letter;
   int version = 0;

   do
   {
      letter = getc( fp );
      if( feof( fp ) )
      {
         FCLOSE( fp );
         return NULL;
      }
   }
   while( isspace( letter ) );
   ungetc( letter, fp );

   project_data *project = new project_data;

   for( ;; )
   {
      const char *word = ( feof( fp ) ? "End" : fread_word( fp ) );

      if( word[0] == '\0' )
      {
         bug( "%s: EOF encountered reading file!", __func__ );
         word = "End";
      }

      switch ( UPPER( word[0] ) )
      {
         default:
            bug( "%s: no match: %s", __func__, word );
            fread_to_eol( fp );
            break;

         case '*':
            fread_to_eol( fp );
            break;

         case 'C':
            KEY( "Coder", project->coder, fread_string( fp ) );
            break;

         case 'D':
            /*
             * For passive compatibility with older board files 
             */
            if( !str_cmp( word, "Date" ) )
            {
               fread_to_eol( fp );
               break;
            }
            KEY( "Date_stamp", project->date_stamp, fread_number( fp ) );
            KEY( "Description", project->description, fread_string_nohash( fp ) );
            break;

         case 'E':
            if( !str_cmp( word, "End" ) )
            {
               if( project->date_stamp == 0 )
                  project->date_stamp = current_time;
               if( !project->status )
                  project->status = STRALLOC( "No update." );
               if( str_cmp( project->owner, "None" ) )
                  project->taken = true;
               return project;
            }
            break;

         case 'L':
            if( !str_cmp( word, "Log" ) )
            {
               note_data *nlog;

               fread_to_eol( fp );

               if( version == 2 )
                  nlog = read_note( fp );
               else
                  nlog = read_old_log( fp );

               if( !nlog )
                  bug( "%s: couldn't read log!", __func__ );
               else
                  project->nlist.push_back( nlog );
               break;
            }
            break;

         case 'N':
            KEY( "Name", project->name, fread_string_nohash( fp ) );
            break;

         case 'O':
            KEY( "Owner", project->owner, fread_string( fp ) );
            break;

         case 'R':
            if( !str_cmp( word, "Realm" ) )
            {
               realm_data *realm = get_realm( fread_flagstring( fp ) );

               if( !realm )
                  log_printf( "Project %s has no valid realm.", project->name );
               else
                  project->realm_name = realm->name;
               break;
            }
            break;

         case 'S':
            KEY( "Status", project->status, fread_string_nohash( fp ) );
            break;

         case 'V':
            if( !str_cmp( word, "Version" ) )
            {
               version = fread_number( fp );
               break;
            }
      }
   }
}

void load_projects( void ) /* Copied load_boards structure for simplicity */
{
   FILE *fp;
   project_data *project;

   projlist.clear(  );

   if( !( fp = fopen( PROJECTS_FILE, "r" ) ) )
      return;

   while( ( project = read_project( fp ) ) != NULL )
      projlist.push_back( project );

   // Bugfix - CPPcheck flagged this. It's possible for it to be closed in fread_project before getting back here.
   if( fp )
      FCLOSE( fp );
}

project_data::project_data(  )
{
   init_memory( &name, &taken, sizeof( taken ) );
   nlist.clear(  );
}

project_data::~project_data(  )
{
   list < note_data * >::iterator ilog;

   for( ilog = nlist.begin(  ); ilog != nlist.end(  ); )
   {
      note_data *note = *ilog;
      ++ilog;

      nlist.remove( note );
      deleteptr( note );
   }
   STRFREE( coder );
   DISPOSE( description );
   DISPOSE( name );
   STRFREE( owner );
   DISPOSE( status );
   projlist.remove( this );
}

void free_projects( void )
{
   list < project_data * >::iterator proj;

   for( proj = projlist.begin(  ); proj != projlist.end(  ); )
   {
      project_data *project = *proj;
      ++proj;

      deleteptr( project );
   }
   return;
}

// Hacky looking ugly define, but fuck having to type all this out all the time.
#define IS_PROJECT_ADMIN(ch, proj) ( IS_ADMIN_REALM((ch)) || (IS_REALM_LEADER((ch)) && !str_cmp( (ch)->pcdata->realm_name, (proj)->realm_name )) )

/* Last thing left to revampitize -- Xorith */
CMDF( do_project )
{
   project_data *pproject;
   string arg;
   int pcount, pnum;

   if( ch->isnpc(  ) )
      return;

   if( !ch->desc )
   {
      bug( "%s: no descriptor", __func__ );
      return;
   }

   switch ( ch->substate )
   {
      default:
         break;

      case SUB_WRITING_NOTE:
         if( !ch->pcdata->pnote )
         {
            bug( "%s: log got lost?", __func__ );
            ch->print( "Your log was lost!\r\n" );
            ch->stop_editing(  );
            return;
         }
         if( ch->pcdata->dest_buf != ch->pcdata->pnote )
            bug( "%s: sub_writing_note: ch->pcdata->dest_buf != ch->pcdata->pnote", __func__ );
         DISPOSE( ch->pcdata->pnote->text );
         ch->pcdata->pnote->text = ch->copy_buffer( false );
         ch->stop_editing(  );
         return;

      case SUB_PROJ_DESC:
         if( !ch->pcdata->dest_buf )
         {
            ch->print( "Your description was lost!" );
            bug( "%s: sub_project_desc: NULL ch->pcdata->dest_buf", __func__ );
            ch->substate = SUB_NONE;
            return;
         }
         pproject = ( project_data * ) ch->pcdata->dest_buf;
         DISPOSE( pproject->description );
         pproject->description = ch->copy_buffer( false );
         ch->stop_editing(  );
         ch->substate = ch->tempnum;
         write_projects(  );
         return;

      case SUB_EDIT_ABORT:
         ch->print( "Aborting...\r\n" );
         ch->substate = SUB_NONE;
         if( ch->pcdata->dest_buf )
            ch->pcdata->dest_buf = NULL;
         if( ch->pcdata->pnote )
         {
            deleteptr( ch->pcdata->pnote );
            ch->pcdata->pnote = NULL;
         }
         return;
   }

   ch->set_color( AT_NOTE );
   argument = one_argument( argument, arg );
   smash_tilde( argument );

   if( arg.empty(  ) || !str_cmp( arg, "list" ) )
   {
      list < project_data * >::iterator pr;
      bool aflag = false, projects_available = false;

      if( !str_cmp( argument, "available" ) )
         aflag = true;

      ch->pager( "\r\n" );
      if( !aflag )
      {
         ch->pager( " #  | Owner       | Project                    | Date            | Status       \r\n" );
         ch->pager( "----|-------------|----------------------------|-----------------|--------------\r\n" );
      }
      else
      {
         ch->pager( " #  | Project                        | Date            \r\n" );
         ch->pager( "----|--------------------------------|-----------------\r\n" );
      }
      pcount = 0;
      for( pr = projlist.begin(  ); pr != projlist.end(  ); ++pr )
      {
         project_data *proj = *pr;

         ++pcount;
         if( proj->status && !str_cmp( "Done", proj->status ) )
            continue;
         if( !aflag )
         {
            ch->pagerf( "%2d%s | %-11s | %-26s | %-15s | %-12s\r\n",
                        pcount, proj->realm_name.c_str(), proj->owner ? proj->owner : "(None)",
                        print_lngstr( proj->name, 26 ).c_str(  ), mini_c_time( proj->date_stamp, ch->pcdata->timezone ), proj->status ? proj->status : "(None)" );
         }
         else if( !proj->taken )
         {
            if( !projects_available )
               projects_available = true;
            ch->pagerf( "%2d%s | %-30s | %s\r\n", pcount, proj->realm_name.c_str(),
                        proj->name ? proj->name : "(None)", mini_c_time( proj->date_stamp, ch->pcdata->timezone ) );
         }
      }
      if( pcount == 0 )
         ch->pager( "No projects exist.\r\n" );
      else if( aflag && !projects_available )
         ch->pager( "No projects available.\r\n" );
      return;
   }

   if( !str_cmp( arg, "save" ) )
   {
      write_projects(  );
      ch->print( "Projects saved.\r\n" );
      return;
   }

   if( !str_cmp( arg, "code" ) )
   {
      list < project_data * >::iterator pr;

      pcount = 0;
      ch->pager( " #  | Owner       | Project                    \r\n" );
      ch->pager( "----|-------------|----------------------------\r\n" );
      for( pr = projlist.begin(  ); pr != projlist.end(  ); ++pr )
      {
         project_data *proj = ( *pr );

         ++pcount;
         if( ( proj->status && str_cmp( proj->status, "approved" ) ) || proj->coder != NULL )
            continue;
         ch->pagerf( "%2d%s | %-11s | %-26s\r\n", pcount, proj->realm_name.c_str(), proj->owner ? proj->owner : "(None)", proj->name ? proj->name : "(None)" );
      }
      return;
   }

   if( !str_cmp( arg, "more" ) || !str_cmp( arg, "mine" ) )
   {
      list < project_data * >::iterator pr;
      list < note_data * >::iterator ilog;
      bool MINE = false;

      pcount = 0;

      if( !str_cmp( arg, "mine" ) )
         MINE = true;

      ch->pager( "\r\n" );
      ch->pager( " #  | Owner       | Project                    | Coder       | Status     | Logs\r\n" );
      ch->pager( "----|-------------|----------------------------|-------------|------------|-----\r\n" );
      for( pr = projlist.begin(  ); pr != projlist.end(  ); ++pr )
      {
         project_data *proj = *pr;

         ++pcount;
         if( MINE && ( !proj->owner || str_cmp( ch->name, proj->owner ) ) && ( !proj->coder || str_cmp( ch->name, proj->coder ) ) )
            continue;
         else if( !MINE && proj->status && !str_cmp( "Done", proj->status ) )
            continue;
         int num_logs = proj->nlist.size(  );
         ch->pagerf( "%2d%s | %-11s | %-26s | %-11s | %-10s | %3d\r\n",
                     pcount, proj->realm_name.c_str(), proj->owner ? proj->owner : "(None)",
                     print_lngstr( proj->name, 26 ).c_str(  ), proj->coder ? proj->coder : "(None)", proj->status ? proj->status : "(None)", num_logs );
      }
      return;
   }

   if( !str_cmp( arg, "add" ) )
   {
      realm_data *realm = NULL;

      if( !IS_REALM_LEADER(ch) && !IS_ADMIN_REALM(ch) )
      {
         ch->print( "Only realm leaders or administrators may add a new project.\r\n" );
         return;
      }

      if( IS_ADMIN_REALM(ch) )
      {
         argument = one_argument( argument, arg );

         if( arg.empty(  ) )
         {
            ch->print( "As an administrator, you must specify the project's realm before creating it.\r\n" );
            return;
         }

         realm = get_realm( arg );
         if( !realm )
         {
            ch->printf( "%s is not a valid realm.\r\n", arg.c_str() );
            return;
         }
      }

      project_data *new_project = new project_data;
      new_project->name = str_dup( argument.c_str(  ) );
      new_project->coder = NULL;
      new_project->taken = false;
      new_project->description = NULL;
      new_project->realm_name = realm->name;
      new_project->date_stamp = current_time;
      projlist.push_back( new_project );
      write_projects(  );
      ch->printf( "New %s Project '%s' added.\r\n", new_project->realm_name.c_str(), new_project->name );
      return;
   }

   if( !is_number( arg ) )
   {
      ch->printf( "%s is an invalid project!\r\n", arg.c_str(  ) );
      return;
   }

   pnum = atoi( arg.c_str(  ) );
   pproject = get_project_by_number( pnum );
   if( !pproject )
   {
      ch->printf( "Project #%d does not exsist.\r\n", pnum );
      return;
   }

   argument = one_argument( argument, arg );

   if( !str_cmp( arg, "description" ) )
   {
      if( !IS_PROJECT_ADMIN(ch, pproject) && str_cmp( ch->name, pproject->owner ) )
         ch->CHECK_SUBRESTRICTED(  );
      ch->tempnum = SUB_NONE;
      ch->substate = SUB_PROJ_DESC;
      ch->pcdata->dest_buf = pproject;
      if( !pproject->description )
         pproject->description = str_dup( "" );
      ch->editor_desc_printf( "Project description for project '%s'.", pproject->name ? pproject->name : "(No name)" );
      ch->start_editing( pproject->description );
      return;
   }

   if( !str_cmp( arg, "delete" ) )
   {
      if( !IS_PROJECT_ADMIN(ch, pproject) )
      {
         ch->print( "You do not have the authority to delete this project.\r\n" );
         return;
      }

      ch->printf( "Project '%s' has been deleted.\r\n", pproject->name );
      deleteptr( pproject );
      write_projects(  );
      return;
   }

   if( !str_cmp( arg, "take" ) )
   {
      if( pproject->taken && pproject->owner && !str_cmp( pproject->owner, ch->name ) )
      {
         pproject->taken = false;
         STRFREE( pproject->owner );
         ch->print( "You removed yourself as the owner.\r\n" );
         write_projects(  );
         return;
      }
      else if( pproject->taken && !IS_PROJECT_ADMIN(ch, pproject) )
      {
         ch->print( "This project is already taken.\r\n" );
         return;
      }
      else if( pproject->taken && IS_PROJECT_ADMIN(ch, pproject) )
         ch->printf( "Taking Project: '%s' from Owner: '%s'!\r\n", pproject->name, pproject->owner ? pproject->owner : "NULL" );

      STRFREE( pproject->owner );
      pproject->owner = QUICKLINK( ch->name );
      pproject->taken = true;
      write_projects(  );
      ch->printf( "You're now the owner of Project '%s'.\r\n", pproject->name );
      return;
   }

   if( ( !str_cmp( arg, "coder" ) ) || ( !str_cmp( arg, "builder" ) ) )
   {
      if( pproject->coder && !str_cmp( ch->name, pproject->coder ) )
      {
         STRFREE( pproject->coder );
         ch->printf( "You removed yourself as the %s.\r\n", arg.c_str() );
         write_projects(  );
         return;
      }
      else if( pproject->coder && !IS_PROJECT_ADMIN(ch, pproject) )
      {
         ch->printf( "This project already has a %s.\r\n", arg.c_str() );
         return;
      }
      else if( pproject->coder && IS_PROJECT_ADMIN(ch, pproject) )
      {
         ch->printf( "Removing %s as %s of this project!\r\n", pproject->coder, arg.c_str() );
         STRFREE( pproject->coder );
         ch->print( "Coder removed.\r\n" );
         write_projects(  );
         return;
      }

      pproject->coder = QUICKLINK( ch->name );
      write_projects(  );
      ch->printf( "You are now the %s of %s.\r\n", arg.c_str(), pproject->name );
      return;
   }

   if( !str_cmp( arg, "status" ) )
   {
      if( pproject->owner && str_cmp( pproject->owner, ch->name ) && !IS_PROJECT_ADMIN(ch, pproject) )
      {
         ch->print( "This is not your project!\r\n" );
         return;
      }
      DISPOSE( pproject->status );
      pproject->status = str_dup( argument.c_str(  ) );
      write_projects(  );
      ch->printf( "Project Status set to: %s\r\n", pproject->status );
      return;
   }

   if( !str_cmp( arg, "show" ) )
   {
      if( pproject->description )
         ch->print( pproject->description );
      else
         ch->print( "That project does not have a description.\r\n" );
      return;
   }

   if( !str_cmp( arg, "log" ) )
   {
      note_data *plog;

      if( !str_cmp( argument, "write" ) )
      {
         if( !ch->pcdata->pnote )
            ch->note_attach(  );
         ch->substate = SUB_WRITING_NOTE;
         ch->pcdata->dest_buf = ch->pcdata->pnote;
         if( !ch->pcdata->pnote->text )
            ch->pcdata->pnote->text = str_dup( "" );
         ch->editor_desc_printf( "A log note in project '%s', entitled '%s'.",
                                 pproject->name ? pproject->name : "(No name)", ch->pcdata->pnote->subject ? ch->pcdata->pnote->subject : "(No subject)" );
         ch->start_editing( ch->pcdata->pnote->text );
         return;
      }

      argument = one_argument( argument, arg );

      if( !str_cmp( arg, "subject" ) )
      {
         if( !ch->pcdata->pnote )
            ch->note_attach(  );
         DISPOSE( ch->pcdata->pnote->subject );
         ch->pcdata->pnote->subject = str_dup( argument.c_str(  ) );
         ch->printf( "Log Subject set to: %s\r\n", ch->pcdata->pnote->subject );
         return;
      }

      if( !str_cmp( arg, "post" ) )
      {
         if( ( pproject->owner && str_cmp( ch->name, pproject->owner ) )
             || ( pproject->coder && str_cmp( ch->name, pproject->coder ) ) || !IS_PROJECT_ADMIN(ch, pproject) )
         {
            ch->print( "This is not your project!\r\n" );
            return;
         }

         if( !ch->pcdata->pnote )
         {
            ch->print( "You have no log in progress.\r\n" );
            return;
         }

         if( !ch->pcdata->pnote->subject )
         {
            ch->print( "Your log has no subject.\r\n" );
            return;
         }

         if( !ch->pcdata->pnote->text || ch->pcdata->pnote->text[0] == '\0' )
         {
            ch->print( "Your log has no text!\r\n" );
            return;
         }

         ch->pcdata->pnote->date_stamp = current_time;
         plog = ch->pcdata->pnote;
         ch->pcdata->pnote = NULL;

         if( !argument.empty(  ) )
         {
            int plog_num;
            note_data *tplog;
            plog_num = atoi( argument.c_str(  ) );

            tplog = get_log_by_number( pproject, plog_num );

            if( !tplog )
            {
               ch->printf( "Log #%d can not be found.\r\n", plog_num );
               return;
            }

            plog->parent = tplog;
            ++tplog->reply_count;
            tplog->rlist.push_back( plog );
            ch->print( "Your reply was posted successfully." );
            write_projects(  );
            return;
         }
         pproject->nlist.push_back( plog );
         write_projects(  );
         ch->print( "Log Posted!\r\n" );
         return;
      }

      if( !str_cmp( arg, "list" ) )
      {
         list < note_data * >::iterator ilog;

         if( ( pproject->owner && str_cmp( ch->name, pproject->owner ) )
             || ( pproject->coder && str_cmp( ch->name, pproject->coder ) ) || !IS_PROJECT_ADMIN(ch, pproject) )
         {
            ch->print( "This is not your project!\r\n" );
            return;
         }

         pcount = 0;
         ch->pagerf( "Project: %-12s: %s\r\n", pproject->owner ? pproject->owner : "(None)", pproject->name );

         for( ilog = pproject->nlist.begin(  ); ilog != pproject->nlist.end(  ); ++ilog )
         {
            note_data *tlog = *ilog;

            ++pcount;
            ch->pagerf( "%2d) %-12s: %s\r\n", pcount, tlog->sender ? tlog->sender : "--Error--", tlog->subject ? tlog->subject : "" );
         }
         if( pcount == 0 )
            ch->print( "No logs available.\r\n" );
         else
            ch->printf( "%d log%s found.\r\n", pcount, pcount == 1 ? "" : "s" );
         return;
      }

      if( !is_number( arg ) )
      {
         ch->print( "Invalid log.\r\n" );
         return;
      }

      pnum = atoi( arg.c_str(  ) );

      plog = get_log_by_number( pproject, pnum );
      if( !plog )
      {
         ch->print( "Invalid log.\r\n" );
         return;
      }

      if( !str_cmp( argument, "delete" ) )
      {
         if( ( pproject->owner && str_cmp( ch->name, pproject->owner ) )
             || ( pproject->coder && str_cmp( ch->name, pproject->coder ) ) || !IS_PROJECT_ADMIN(ch, pproject) )
         {
            ch->print( "This is not your project!\r\n" );
            return;
         }
         pproject->nlist.remove( plog );
         deleteptr( plog );
         write_projects(  );
         ch->printf( "Log #%d has been deleted.\r\n", pnum );
         return;
      }

      if( !str_cmp( argument, "read" ) )
      {
         if( ( pproject->owner && str_cmp( ch->name, pproject->owner ) )
             || ( pproject->coder && str_cmp( ch->name, pproject->coder ) ) || !IS_PROJECT_ADMIN(ch, pproject) )
         {
            ch->print( "This is not your project!\r\n" );
            return;
         }
         note_to_char( ch, plog, NULL, -1 );
         return;
      }
   }
   interpret( ch, "help project" );
}
