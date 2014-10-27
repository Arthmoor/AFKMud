/****************************************************************************
 *                   ^     +----- |  / ^     ^ |     | +-\                  *
 *                  / \    |      | /  |\   /| |     | |  \                 *
 *                 /   \   +---   |<   | \ / | |     | |  |                 *
 *                /-----\  |      | \  |  v  | |     | |  /                 *
 *               /       \ |      |  \ |     | +-----+ +-/                  *
 ****************************************************************************
 * AFKMud Copyright 1997-2007 by Roger Libiez (Samson),                     *
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
 *                     Completely Revised Boards Module                     *
 ****************************************************************************
 * Revised by:   Xorith                                                     *
 * Last Updated: 10/2/03                                                    *
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
#include "roomindex.h"

list<board_data*> bdlist;
list<project_data*> projlist;

char *mini_c_time( time_t, int );
void check_boards( );

/* Global */
time_t board_expire_time_t;

char *const board_flags[] = {
   "r1", "backup_pruned", "private", "announce"
};

int get_board_flag( char *board_flag )
{
   int x;

   for( x = 0; x < MAX_BOARD_FLAGS; ++x )
      if( !str_cmp( board_flag, board_flags[x] ) )
         return x;
   return -1;
}

/* This is a simple function that keeps a string within a certain length. -- Xorith */
char *print_lngstr( char *string, size_t size )
{
   static char rstring[MSL];

   if( strlen( string ) > size )
   {
      mudstrlcpy( rstring, string, size - 2 );
      mudstrlcat( rstring, "...", size + 1 );
   }
   else
      mudstrlcpy( rstring, string, size );

   return rstring;
}

board_chardata::board_chardata()
{
   init_memory( &board_name, &alert, sizeof( alert ) );
}

board_chardata::~board_chardata()
{
   STRFREE( board_name );
}

note_data::note_data()
{
   init_memory( &parent, &expire, sizeof( expire ) );
   rlist.clear();
}

note_data::~note_data()
{
   list<note_data*>::iterator rp;

   DISPOSE( text );
   DISPOSE( subject );
   STRFREE( to_list );
   STRFREE( sender );

   for( rp = rlist.begin(); rp != rlist.end(); )
   {
      note_data *reply = (*rp);
      ++rp;

      rlist.remove( reply );
      deleteptr( reply );
   }
}

board_data::board_data()
{
   init_memory( &flags, &expire, sizeof( expire ) );
   nlist.clear();
}

board_data::~board_data()
{
   list<note_data*>::iterator note;

   STRFREE( name );
   STRFREE( readers );
   STRFREE( posters );
   STRFREE( moderators );
   DISPOSE( desc );
   STRFREE( group );
   DISPOSE( filename );

   for( note = nlist.begin(); note != nlist.end(); )
   {
      note_data *pnote = (*note);
      ++note;

      nlist.remove( pnote );
      deleteptr( pnote );
   }
   bdlist.remove( this );
}

void pc_data::free_pcboards(  )
{
   list<board_chardata*>::iterator bd;

   for( bd = boarddata.begin(); bd != boarddata.end(); )
   {
      board_chardata *chbd = (*bd);
      ++bd;

      boarddata.remove( chbd );
      deleteptr( chbd );
   }
   return;
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
         if( is_name( ch->name, board->moderators ) )
            return true;
      }
   }
   /*
    * else, make sure they're a member of the group. this means no outside mortal moderators... 
    */
   else if( ch->pcdata->clan && is_name( ch->pcdata->clan->name, board->group ) )
   {
      if( IS_LEADER( ch ) || IS_NUMBER1( ch ) || IS_NUMBER2( ch ) )
         return true;

      if( ch->get_trust(  ) >= board->remove_level )
         return true;

      if( board->moderators && board->moderators[0] != '\0' )
      {
         if( is_name( ch->name, board->moderators ) )
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
         if( is_name( ch->name, board->posters ) )
            return true;
      }
   }
   /*
    * else, make sure they're a member of the group. this means no outside mortal posters... 
    */
   else if( ( ch->pcdata->clan && is_name( ch->pcdata->clan->name, board->group ) ) ||
            ( ch->pcdata->deity && is_name( ch->pcdata->deity->name, board->group ) ) )
   {
      if( ch->get_trust(  ) >= board->post_level )
         return true;

      if( board->posters && board->posters[0] != '\0' )
      {
         if( is_name( ch->name, board->posters ) )
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
         if( is_name( ch->name, board->readers ) )
            return true;
      }
   }
   /*
    * else, make sure they're a member of the group. this means no outside mortal readers... 
    */
   else if( ( ch->pcdata->clan && is_name( ch->pcdata->clan->name, board->group ) ) ||
            ( ch->pcdata->deity && is_name( ch->pcdata->deity->name, board->group ) ) )
   {
      if( ch->get_trust(  ) >= board->read_level )
         return true;

      if( board->readers && board->readers[0] != '\0' )
      {
         if( is_name( ch->name, board->readers ) )
            return true;
      }
   }
   return false;
}

bool is_note_to( char_data * ch, note_data * pnote )
{
   if( pnote->to_list && pnote->to_list[0] != '\0' )
   {
      if( is_name( "all", pnote->to_list ) )
         return true;

      if( ch->is_immortal(  ) && is_name( "immortal", pnote->to_list ) )
         return true;

      if( is_name( ch->name, pnote->to_list ) )
         return true;
   }
   return false;
}

/* This will get a board by object. This will not get a global board
   as global boards are noted with a 0 objvnum */
board_data *get_board_by_obj( obj_data * obj )
{
   list<board_data*>::iterator bd;

   for( bd = bdlist.begin(); bd != bdlist.end(); ++bd )
   {
      board_data *board = (*bd);

      if( board->objvnum == obj->pIndexData->vnum )
         return board;
   }
   return NULL;
}

/* Gets a board by name, or a number. The number should be the board # given in do_board_list.
   If ch == NULL, then it'll perform the search without checks. Otherwise, it'll perform the
   search and weed out boards that the ch can't view remotely. */
board_data *get_board( char_data * ch, const char *name )
{
   list<board_data*>::iterator bd;
   int count = 1;

   for( bd = bdlist.begin(); bd != bdlist.end(); ++bd )
   {
      board_data *board = (*bd);

      if( ch != NULL )
      {
         if( !can_read( ch, board ) )
            continue;
         if( board->objvnum > 0 && !can_remove( ch, board ) && !ch->is_immortal(  ) )
            continue;
      }
      if( count == atoi( name ) )
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
   list<obj_data*>::iterator iobj;

   for( iobj = ch->in_room->objects.begin(); iobj != ch->in_room->objects.end(); ++iobj )
   {
      obj_data *obj = (*iobj);
      board_data *board;

      if( ( board = get_board_by_obj( obj ) ) != NULL )
         return board;
   }
   return NULL;
}

board_chardata *get_chboard( char_data * ch, char *board_name )
{
   list<board_chardata*>::iterator bd;

   for( bd = ch->pcdata->boarddata.begin(); bd != ch->pcdata->boarddata.end(); ++bd )
   {
      board_chardata *board = (*bd);

      if( !str_cmp( board->board_name, board_name ) )
         return board;
   }
   return NULL;
}

board_chardata *create_chboard( char_data * ch, char *board_name )
{
   board_chardata *chboard;

   if( ( chboard = get_chboard( ch, board_name ) ) )
      return chboard;

   chboard = new board_chardata;
   chboard->board_name = QUICKLINK( board_name );
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
      bug( "%s: ch->pcdata->pnote already exsists!", __FUNCTION__ );
      return;
   }

   pcnote = new note_data;
   pcnote->sender = QUICKLINK( name );
   pcdata->pnote = pcnote;
   return;
}

void free_boards( void )
{
   list<board_data*>::iterator bd;

   for( bd = bdlist.begin(); bd != bdlist.end(); )
   {
      board_data *board = (*bd);
      ++bd;

      deleteptr( board );
   }
   return;
}

const int BOARDFILEVER = 1;
void write_boards( void )
{
   list<board_data*>::iterator bd;
   FILE *fpout;

   if( !( fpout = fopen( BOARD_DIR BOARD_FILE, "w" ) ) )
   {
      perror( BOARD_DIR BOARD_FILE );
      bug( "%s: Unable to open %s%s for writing!", __FUNCTION__, BOARD_DIR, BOARD_FILE );
      return;
   }

   for( bd = bdlist.begin(); bd != bdlist.end(); )
   {
      board_data *board = (*bd);
      ++bd;

      if( !board->name )
      {
         bug( "%s: Board with a null name! Destroying...", __FUNCTION__ );
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
      fprintf( fpout, "Flags       %s\n", bitset_string( board->flags, board_flags ) );
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
   fprintf( fpout, "Reply-DateStamp      %ld\n", ( long int )pnote->date_stamp );
   fprintf( fpout, "Reply-Text           %s~\n", pnote->text );
   fprintf( fpout, "%s\n", "Reply-End" );
   return;
}

void fwrite_note( note_data * pnote, FILE * fpout )
{
   if( !pnote )
      return;

   if( !pnote->sender )
   {
      bug( "%s: Called on a note without a valid sender!", __FUNCTION__ );
      return;
   }

   fprintf( fpout, "Sender         %s~\n", pnote->sender );
   if( pnote->to_list )
      fprintf( fpout, "To             %s~\n", pnote->to_list );
   if( pnote->subject )
      fprintf( fpout, "Subject        %s~\n", pnote->subject );
   fprintf( fpout, "DateStamp      %ld\n", ( long int )pnote->date_stamp );
   if( pnote->to_list ) /* Comments and Project Logs do not use to_list or Expire */
      fprintf( fpout, "Expire         %d\n", pnote->expire );
   if( pnote->text )
      fprintf( fpout, "Text           %s~\n", pnote->text );
   if( pnote->to_list ) /* comments and projects should not have replies */
   {
      int count = 0;
      list<note_data*>::iterator rp;
      for( rp = pnote->rlist.begin(); rp != pnote->rlist.end(); )
      {
         note_data *reply = (*rp);
         ++rp;

         if( !reply->sender || !reply->to_list || !reply->subject || !reply->text )
         {
            bug( "%s: Destroying a buggy reply on note '%s'!", __FUNCTION__, pnote->subject );
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
   return;
}

void write_board( board_data * board )
{
   FILE *fp;
   char filename[256];

   snprintf( filename, 256, "%s%s.board", BOARD_DIR, board->filename );
   if( !( fp = fopen( filename, "w" ) ) )
   {
      perror( filename );
      bug( "%s: Error opening %s! Board NOT saved!", __FUNCTION__, filename );
      return;
   }

   list<note_data*>::iterator note;
   for( note = board->nlist.begin(); note != board->nlist.end(); )
   {
      note_data *pnote = (*note);
      ++note;

      if( !pnote->sender || !pnote->to_list || !pnote->subject || !pnote->text )
      {
         bug( "%s: Destroying a buggy note on the %s board!", __FUNCTION__, board->name );
         board->nlist.remove( pnote );
         --board->msg_count;
         deleteptr( pnote );
         continue;
      }
      fwrite_note( pnote, fp );
   }
   FCLOSE( fp );
   return;
}

void note_remove( board_data * board, note_data * pnote )
{
   if( !board || !pnote )
   {
      bug( "%s: null %s variable.", __FUNCTION__, board ? "pnote" : "board" );
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
         bug( "%s: EOF encountered reading file!", __FUNCTION__ );
         word = "#END";
      }

      switch ( UPPER( word[0] ) )
      {
         default:
            bug( "%s: no match: %s", __FUNCTION__, word );
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
               board->nlist.clear();
               if( !board->objvnum )
                  board->objvnum = 0;  /* default to global */
               return board;
            }
            else
            {
               bug( "%s: Bad section: %s", __FUNCTION__, word );
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
         bug( "%s: EOF encountered reading file!", __FUNCTION__ );
         word = "End";
      }

      switch ( UPPER( word[0] ) )
      {
         default:
            bug( "%s: no match: %s", __FUNCTION__, word );
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
               board->nlist.clear();
               if( !board->objvnum )
                  board->objvnum = 0;  /* default to global */
               board->desc = str_dup( "Newly converted board!" );
               board->expire = MAX_BOARD_EXPIRE;
               board->filename = str_dup( board->name );
               if( board->posters[0] == '\0' )
               {
                  STRFREE( board->posters );
                  board->posters = NULL;
               }
               if( board->readers[0] == '\0' )
               {
                  STRFREE( board->readers );
                  board->readers = NULL;
               }
               if( board->moderators[0] == '\0' )
               {
                  STRFREE( board->moderators );
                  board->moderators = NULL;
               }
               if( board->group[0] == '\0' )
               {
                  STRFREE( board->group );
                  board->group = NULL;
               }
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
         bug( "%s: EOF encountered reading file!", __FUNCTION__ );
         word = "#END";
      }

      switch ( UPPER( word[0] ) )
      {
         default:
            bug( "%s: no match: %s", __FUNCTION__, word );
            fread_to_eol( fp );
            break;

         case '*':
            fread_to_eol( fp );
            break;

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
                  bug( "%s: Reply found when MAX_REPLY has already been reached!", __FUNCTION__ );
                  continue;
               }
               if( reply != NULL )
               {
                  bug( "%s: Unsupported nested reply found!", __FUNCTION__ );
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
               if( !pnote->date_stamp )
                  pnote->date_stamp = current_time;
               return pnote;
            }
            else
            {
               bug( "%s: Bad section: %s", __FUNCTION__, word );
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
         bug( "%s: EOF encountered reading file!", __FUNCTION__ );
         word = "End";
      }

      /*
       * Keep the damn thing happy. 
       */
      if( !str_cmp( word, "Voting" ) || !str_cmp( word, "Yesvotes" )
          || !str_cmp( word, "Novotes" ) || !str_cmp( word, "Abstentions" ) )
      {
         word = fread_word( fp );
         continue;
      }

      switch ( UPPER( word[0] ) )
      {
         default:
            bug( "%s: no match: %s", __FUNCTION__, word );
            fread_to_eol( fp );
            break;

         case '*':
            fread_to_eol( fp );
            break;

         case 'C':
            if( !str_cmp( word, "Ctime" ) )
            {
               pnote->date_stamp = fread_number( fp );
               pnote->expire = 0;
               if( !pnote->text )
               {
                  pnote->text = str_dup( "---- Delete this post! ----" );
                  pnote->expire = 1;   /* Or we'll do it for you! */
                  bug( "%s: No text on note! Setting to '%s' and expiration to 1 day", __FUNCTION__, pnote->text );
               }

               if( !pnote->date_stamp )
               {
                  bug( "%s: No date_stamp on note -- setting to current time!", __FUNCTION__ );
                  pnote->date_stamp = current_time;
               }

               /*
                * For converted notes, lets make a few exceptions 
                */
               if( !pnote->sender )
               {
                  pnote->sender = STRALLOC( "Converted Msg" );
                  bug( "%s: No sender on converted note! Setting to '%s'", __FUNCTION__, pnote->sender );
               }
               if( !pnote->subject )
               {
                  pnote->subject = str_dup( "Converted Msg" );
                  bug( "%s: No subject on converted note! Setting to '%s'", __FUNCTION__, pnote->subject );
               }
               if( !pnote->to_list )
               {
                  pnote->to_list = STRALLOC( "imm" );
                  bug( "%s: No to_list on converted note! Setting to '%s'", __FUNCTION__, pnote->to_list );
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
                  bug( "%s: No text on note! Setting to '%s' and expiration to 1 day", __FUNCTION__, pnote->text );
               }

               if( !pnote->date_stamp )
               {
                  bug( "%s: No date_stamp on note -- setting to current time!", __FUNCTION__ );
                  pnote->date_stamp = current_time;
               }

               /*
                * For converted notes, lets make a few exceptions 
                */
               if( !pnote->sender )
               {
                  pnote->sender = STRALLOC( "Converted Msg" );
                  bug( "%s: No sender on converted note! Setting to '%s'", __FUNCTION__, pnote->sender );
               }
               if( !pnote->subject )
               {
                  pnote->subject = str_dup( "Converted Msg" );
                  bug( "%s: No subject on converted note! Setting to '%s'", __FUNCTION__, pnote->subject );
               }
               if( !pnote->to_list )
               {
                  pnote->to_list = STRALLOC( "imm" );
                  bug( "%s: No to_list on converted note! Setting to '%s'", __FUNCTION__, pnote->to_list );
               }
               return pnote;
            }
            break;
      }
   }
}

void load_boards( void )
{
   FILE *board_fp, *note_fp;
   board_data *board;
   note_data *pnote;
   char notefile[256];
   bool oldboards = false;

   bdlist.clear();

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
            bug( "%s: Note file '%s' for the '%s' board not found!", __FUNCTION__, notefile, board->name );

         write_board( board );   /* save the converted board */
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
            bug( "%s: Note file '%s' for the '%s' board not found!", __FUNCTION__, notefile, board->name );
      }
   }
   /*
    * Run initial pruning 
    */
   check_boards( );
   return;
}

int unread_notes( char_data * ch, board_data * board )
{
   list<note_data*>::iterator note;
   board_chardata *chboard;
   int count = 0;

   chboard = get_chboard( ch, board->name );

   for( note = board->nlist.begin(); note != board->nlist.end(); ++note )
   {
      note_data *pnote = (*note);

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
   list<note_data*>::iterator note;
   int count = 0;

   for( note = board->nlist.begin(); note != board->nlist.end(); ++note )
   {
      note_data *pnote = (*note);

      count += pnote->reply_count;
   }
   return count;
}

/* This is because private boards don't function right with board->msg_count...  :P */
int total_notes( char_data * ch, board_data * board )
{
   list<note_data*>::iterator note;
   int count = 0;

   for( note = board->nlist.begin(); note != board->nlist.end(); ++note )
   {
      note_data *pnote = (*note);

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
   list<note_data*>::iterator note;
   FILE *fp;
   char filename[256];

   if( board->expire == 0 )
      return;

   time_t now_time = time( 0 );

   for( note = board->nlist.begin(); note != board->nlist.end(); )
   {
      note_data *pnote = (*note);
      ++note;

      if( pnote->expire == 0 )
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
               bug( "%s: Error opening %s!", __FUNCTION__, filename );
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
   return;
}

void check_boards( void )
{
   list<board_data*>::iterator bd;

   log_string( "Starting board pruning..." );
   for( bd = bdlist.begin(); bd != bdlist.end(); ++bd )
   {
      board_data *board = (*bd);

      board_check_expire( board );
   }
   return;
}

void board_announce( char_data * ch, board_data * board, note_data * pnote )
{
   board_chardata *chboard;
   list<descriptor_data*>::iterator ds;

   for( ds = dlist.begin(); ds != dlist.end(); ++ds )
   {
      descriptor_data *d = (*ds);

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

      if( !IS_BOARD_FLAG( board, BOARD_PRIVATE ) )
         vch->printf( "&G[&wBoard Announce&G] &w%s has posted a message on the %s board.\r\n", ch->name, board->name );
      else if( is_note_to( vch, pnote ) )
         vch->printf( "&G[&wBoard Announce&G] &w%s has posted a message for you on the %s board.\r\n",
                      ch->name, board->name );
   }
   return;
}

void note_to_char( char_data * ch, note_data * pnote, board_data * board, short id )
{
   int count = 1;
   char s1[16], s2[16], s3[16];

   snprintf( s1, 16, "%s", ch->color_str( AT_BOARD ) );
   snprintf( s2, 16, "%s", ch->color_str( AT_BOARD2 ) );
   snprintf( s3, 16, "%s", ch->color_str( AT_BOARD3 ) );

   if( pnote == NULL )
   {
      bug( "%s: null pnote!", __FUNCTION__ );
      return;
   }

   if( id > 0 && board != NULL )
      ch->printf( "%s[%sNote #%s%d%s of %s%d%s]%s           -- %s%s%s --&D\r\n",
                  s3, s1, s2, id, s1, s2, total_notes( ch, board ), s3, s1, s2, board->name, s1 );

   /*
    * Using a negative ID# for a bit of beauty -- Xorith 
    */
   if( id == -1 && !board )
      ch->printf( "%s[%sProject Log%s]&D\r\n", s3, s2, s3 );
   if( id == -2 && !board )
      ch->printf( "%s[%sPlayer Comment%s]&D\r\n", s3, s2, s3 );

   ch->printf( "%sFrom:    %s%-15s", s1, s2, pnote->sender ? pnote->sender : "--Error--" );
   if( pnote->to_list )
      ch->printf( " %sTo:     %s%-15s", s1, s2, pnote->to_list );
   ch->print( "&D\r\n" );
   if( pnote->date_stamp != 0 )
      ch->printf( "%sDate:    %s%s&D\r\n", s1, s2, c_time( pnote->date_stamp, ch->pcdata->timezone ) );

   if( board && can_remove( ch, board ) )
   {
      if( pnote->expire == 0 )
         ch->printf( "%sThis note is sticky and will not expire.&D\r\n", s1 );
      else
      {
         int n_life;
         n_life = pnote->expire - ( ( current_time - pnote->date_stamp ) / 86400 );
         ch->printf( "%sThis note will expire in %s%d%s day%s.&D\r\n", s1, s2, n_life, s1, n_life == 1 ? "" : "s" );
      }
   }

   ch->printf( "%sSubject: %s%s&D\r\n", s1, s2, pnote->subject ? pnote->subject : "" );
   ch->printf( "%s------------------------------------------------------------------------------&D\r\n", s1 );
   ch->printf( "%s%s&D\r\n", s2, pnote->text ? pnote->text : "--Error--" );

   if( !pnote->rlist.empty() )
   {
      list<note_data*>::iterator rp;

      for( rp = pnote->rlist.begin(); rp != pnote->rlist.end(); ++rp )
      {
         note_data *reply = (*rp);

         ch->printf( "\r\n%s------------------------------------------------------------------------------&D\r\n", s1 );
         ch->printf( "%s[%sReply #%s%d%s] [%s%s%s]&D\r\n", s3, s1, s2, count, s3, s2,
                     c_time( reply->date_stamp, ch->pcdata->timezone ), s3 );
         ch->printf( "%sFrom:    %s%-15s", s1, s2, reply->sender ? reply->sender : "--Error--" );
         if( reply->to_list )
            ch->printf( "   %sTo:     %s%-15s", s1, s2, reply->to_list );
         ch->print( "&D\r\n" );
         ch->printf( "%s------------------------------------------------------------------------------&D\r\n", s1 );
         ch->printf( "%s%s&D\r\n", s2, reply->text ? reply->text : "--Error--" );
         ++count;
      }
   }
   return;
}

CMDF( do_note_set )
{
   board_data *board;
   short n_num = 0, i = 1;
   char s1[16], s2[16], s3[16], arg[MIL];

   snprintf( s1, 16, "%s", ch->color_str( AT_BOARD ) );
   snprintf( s2, 16, "%s", ch->color_str( AT_BOARD2 ) );
   snprintf( s3, 16, "%s", ch->color_str( AT_BOARD3 ) );

   if( !( board = find_board( ch ) ) )
   {
      if( !argument || argument[0] == '\0' )
      {
         ch->
            printf
            ( "%sModifies the fields on a note.\r\n%sSyntax: %snset %s<%sboard%s> <%snote#%s> <%sfield%s> <%svalue%s>&D\r\n",
              s1, s3, s1, s3, s2, s3, s2, s3, s2, s3, s2, s3 );
         ch->printf( "%s  Valid fields are: %ssender to_list subject expire&D\r\n", s1, s2 );
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

   if( !can_remove( ch, board ) )
   {
      ch->print( "You are unable to modify the notes on this board.\r\n" );
      return;
   }

   if( !argument || argument[0] == '\0' )
   {
      ch->print( "Modify which note?\r\n" );
      return;
   }
   argument = one_argument( argument, arg );
   n_num = atoi( arg );

   if( n_num == 0 )
   {
      ch->print( "Modify which note?\r\n" );
      return;
   }

   i = 1;
   note_data *pnote = NULL;
   list<note_data*>::iterator inote;
   for( inote = board->nlist.begin(); inote != board->nlist.end(); ++inote )
   {
      note_data *note = (*inote);

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
      if( atoi( argument ) < 0 || atoi( argument ) > MAX_BOARD_EXPIRE )
      {
         ch->printf( "%sExpiration days must be a value between %s0%s and %s%d%s.&D\r\n",
                     s1, s2, s1, s2, MAX_BOARD_EXPIRE, s1 );
         return;
      }
      pnote->expire = atoi( argument );
      ch->printf( "%sSet the expiration time for '%s%s%s' to %s%d&D\r\n", s1, s2, pnote->subject, s1, s2, pnote->expire );
      write_board( board );
      return;
   }

   if( !str_cmp( arg, "sender" ) )
   {
      if( !argument || argument[0] == '\0' )
      {
         ch->printf( "%sYou must specify a new sender.&D\r\n", s1 );
         return;
      }
      STRFREE( pnote->sender );
      pnote->sender = STRALLOC( argument );
      ch->printf( "%sSet the sender for '%s%s%s' to %s%s&D\r\n", s1, s2, pnote->subject, s1, s2, pnote->sender );
      write_board( board );
      return;
   }

   if( !str_cmp( arg, "to_list" ) )
   {
      if( !argument || argument[0] == '\0' )
      {
         ch->printf( "%sYou must specify a new to_list.&D\r\n", s1 );
         return;
      }
      STRFREE( pnote->to_list );
      pnote->to_list = STRALLOC( argument );
      ch->printf( "%sSet the to_list for '%s%s%s' to %s%s&D\r\n", s1, s2, pnote->subject, s1, s2, pnote->to_list );
      write_board( board );
      return;
   }

   if( !str_cmp( arg, "subject" ) )
   {
      char buf[MSL];
      if( !argument || argument[0] == '\0' )
      {
         ch->printf( "%sYou must specify a new subject.&D\r\n", s1 );
         return;
      }
      snprintf( buf, MSL, "%s", pnote->subject );
      DISPOSE( pnote->subject );
      pnote->subject = str_dup( argument );
      ch->printf( "%sSet the subject for '%s%s%s' to %s%s&D\r\n", s1, s2, buf, s1, s2, pnote->subject );
      write_board( board );
      return;
   }
   ch->
      printf( "%sModifies the fields on a note.\r\n%sSyntax: %snset %s<%sboard%s> <%snote#%s> <%sfield%s> <%svalue%s>&D\r\n",
              s1, s3, s1, s3, s2, s3, s2, s3, s2, s3, s2, s3 );
   ch->printf( "%s  Valid fields are: %ssender to_list subject expire&D\r\n", s1, s2 );
   return;
}

/* This handles anything in the CON_BOARD state. */
CMDF( do_note_write );
void board_parse( descriptor_data * d, char *argument )
{
   char_data *ch;
   char s1[16], s2[16], s3[16];

   ch = d->character ? d->character : d->original;

   /*
    * Can NPCs even have substates and connected? 
    */
   if( ch->isnpc(  ) )
   {
      bug( "%s: NPC in %s!", __FUNCTION__, __FUNCTION__ );
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
      bug( "%s: In substate -- No pnote on character!", __FUNCTION__ );
      d->connected = CON_PLAYING;
      ch->substate = SUB_NONE;
      return;
   }

   if( !ch->pcdata->board )
   {
      bug( "%s: In substate -- No board on character!", __FUNCTION__ );
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
            ch->printf( "%sThis note will not expire during pruning.&D\r\n", s1 );
         }
         else
         {
            ch->pcdata->pnote->expire = ch->pcdata->board->expire;
            ch->printf( "%sNote set to expire after the default of %s%d%s day%s.&D\r\n",
                        s1, s2, ch->pcdata->pnote->expire, s1, ch->pcdata->pnote->expire == 1 ? "" : "s" );
         }

         if( ch->pcdata->pnote->parent )
            ch->printf( "%sTo whom is this note addressed? %s(%sDefault: %s%s%s)&D   ", s1, s3, s1, s2,
                        ch->pcdata->pnote->parent->sender, s3 );
         else
            ch->printf( "%sTo whom is this note addressed? %s(%sDefault: %sAll%s)&D   ", s1, s3, s1, s2, s3 );
         ch->substate = SUB_BOARD_TO;
         return;

      case SUB_BOARD_TO:
         if( !argument || argument[0] == '\0' )
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

            ch->printf( "%sNo recipient specified. Defaulting to '%s%s%s'&D\r\n", s1, s2, ch->pcdata->pnote->to_list, s1 );
         }
         else if( !str_cmp( argument, "all" )
                  && ( ch->is_immortal(  ) || !IS_BOARD_FLAG( ch->pcdata->board, BOARD_PRIVATE ) ) )
         {
            ch->printf( "%sYou can not send a message to '%sAll%s' on this board!\r\nYou must specify a recipient:&D   ",
                        s1, s2, s1 );
            return;
         }
         else
         {
            struct stat fst;
            char buf[MSL];

            snprintf( buf, MSL, "%s%c/%s", PLAYER_DIR, tolower( argument[0] ), capitalize( argument ) );
            if( stat( buf, &fst ) == -1 || !check_parse_name( capitalize( argument ), false ) )
            {
               ch->printf( "%sNo such player named '%s%s%s'.\r\nTo whom is this note addressed?   &D",
                           s1, s2, argument, s1 );
               return;
            }
            STRFREE( ch->pcdata->pnote->to_list );
            ch->pcdata->pnote->to_list = STRALLOC( argument );
         }

         ch->printf( "%sTo: %s%-15s %sFrom: %s%s&D\r\n", s1, s2, ch->pcdata->pnote->to_list,
                     s1, s2, ch->pcdata->pnote->sender );
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
         if( !argument || argument[0] == '\0' )
         {
            ch->printf( "%sNo subject specified!&D\r\n%sYou must specify a subject:&D  ", s3, s1 );
            return;
         }
         else
         {
            DISPOSE( ch->pcdata->pnote->subject );
            ch->pcdata->pnote->subject = str_dup( argument );
         }

         ch->substate = SUB_BOARD_TEXT;
         ch->printf( "%sTo: %s%-15s %sFrom: %s%s&D\r\n", s1, s2, ch->pcdata->pnote->to_list,
                     s1, s2, ch->pcdata->pnote->sender );
         ch->printf( "%sSubject: %s%s&D\r\n", s1, s2, ch->pcdata->pnote->subject );
         ch->printf( "%sPlease enter the text for your message:&D\r\n", s1 );
         if( !ch->pcdata->pnote->text )
            ch->pcdata->pnote->text = str_dup( "" );
         ch->editor_desc_printf( "A note to %s", ch->pcdata->pnote->to_list );
         ch->start_editing( ch->pcdata->pnote->text );
         return;

      case SUB_BOARD_CONFIRM:
         if( !argument || argument[0] == '\0' )
         {
            note_to_char( ch, ch->pcdata->pnote, NULL, 0 );
            ch->printf( "%sYou %smust%s confirm! Is this correct? %s(%sType: %sY%s or %sN%s)&D   ",
                        s1, s3, s1, s3, s1, s2, s1, s2, s3 );
            return;
         }

         if( !str_cmp( argument, "y" ) || !str_cmp( argument, "yes" ) )
         {
            if( !ch->pcdata->pnote )
            {
               bug( "%s: NULL (ch)%s->pcdata->pnote!", __FUNCTION__, ch->name );
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
               log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );

            ch->printf( "%sYou post the note '%s%s%s' on the %s%s%s board.&D\r\n",
                        s1, s2, ch->pcdata->pnote->subject, s1, s2, ch->pcdata->board->name, s1 );
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
         if( !argument || argument[0] == '\0' )
         {
            ch->printf( "%sPlease choose an option, %sQ%s to quit this menu, or %s/a%s to abort:&D   ", s1, s2, s1, s2, s1 );
            return;
         }

         if( !str_cmp( argument, "1" ) )
         {
            if( ch->pcdata->pnote->parent )
               ch->printf( "%sTo whom is this note addressed? %s(%sDefault: %s%s%s)&D   ", s1, s3, s1, s2,
                           ch->pcdata->pnote->parent->sender, s3 );
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
               ch->printf( "%sPlease choose an option, %sQ%s to quit this menu, or %s/a%s to abort:&D   ",
                           s1, s2, s1, s2, s1 );
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
   return;
}

CMDF( do_note_write )
{
   board_data *board = NULL;
   room_index *board_room;
   char arg[MIL], buf[MSL];
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
            log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
         ch->desc->connected = CON_PLAYING;
         act( AT_GREY, "$n aborts $s note writing.", ch, NULL, NULL, TO_ROOM );
         ch->pcdata->board = NULL;
         return;
      case SUB_BOARD_TEXT:
         if( ch->pcdata->pnote == NULL )
         {
            bug( "%s: SUB_BOARD_TEXT: Null pnote on character (%s)!", __FUNCTION__, ch->name );
            ch->stop_editing(  );
            return;
         }
         if( ch->pcdata->board == NULL )
         {
            bug( "%s: SUB_BOARD_TEXT: Null board on character (%s)!", __FUNCTION__, ch->name );
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
      if( !argument || argument[0] == '\0' )
      {
         ch->printf( "%sWrites a new message for a board.\r\n%sSyntax: %swrite %s<%sboard%s> [%ssubject%s/%snote#%s]&D\r\n",
                     s1, s3, s1, s3, s2, s3, s2, s3, s2, s3 );
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
      n_num = atoi( argument );

   ch->pcdata->board = board;
   snprintf( buf, MSL, "%d", ROOM_VNUM_BOARD );
   board_room = ch->find_location( buf );
   if( !board_room )
   {
      bug( "%s: Missing board room: Vnum %d", __FUNCTION__, ROOM_VNUM_BOARD );
      return;
   }
   ch->printf( "%sTyping '%s/a%s' at any time will abort the note.&D\r\n", s3, s2, s3 );

   if( n_num )
   {
      list<note_data*>::iterator inote;
      note_data *pnote = NULL;
      int i = 1;
      for( inote = board->nlist.begin(); inote != board->nlist.end(); ++inote )
      {
         note_data *note = (*inote);

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

      ch->printf( "%sYou begin to write a reply for %s%s's%s note '%s%s%s'.&D\r\n", s1, s2, pnote->sender, s1, s2,
                  pnote->subject, s1 );
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
         ch->printf( "%sTo: %s%-15s %sFrom: %s%s&D\r\n", s1, s2, ch->pcdata->pnote->to_list,
                     s1, s2, ch->pcdata->pnote->sender );
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
         log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
      return;
   }

   ch->note_attach(  );
   if( argument && argument[0] != '\0' )
   {
      DISPOSE( ch->pcdata->pnote->subject );
      ch->pcdata->pnote->subject = str_dup( argument );
      ch->printf( "%sYou begin to write a new note for the %s%s%s board, titled '%s%s%s'.&D\r\n",
                  s1, s2, board->name, s1, s2, ch->pcdata->pnote->subject, s1 );
   }
   else
      ch->printf( "%sYou begin to write a new note for the %s%s%s board.&D\r\n", s1, s2, board->name, s1 );

   if( can_remove( ch, board ) && !IS_BOARD_FLAG( board, BOARD_PRIVATE ) )
   {
      ch->printf( "%sIs this a sticky note? %s(%sY%s/%sN%s)  (%sDefault: %sN%s)&D   ", s1, s3, s2, s3, s2, s3, s1, s2, s3 );
      ch->substate = SUB_BOARD_STICKY;
   }
   else
   {
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
      log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
   return;
}

CMDF( do_note_read )
{
   board_data *board = NULL;
   list<board_data*>::iterator bd;
   list<note_data*>::iterator inote;
   note_data *pnote = NULL;
   board_chardata *pboard = NULL;
   int n_num = 0, i = 1;
   char arg[MIL];
   char s1[16], s2[16], s3[16];

   if( ch->isnpc(  ) )
      return;

   snprintf( s1, 16, "%s", ch->color_str( AT_BOARD ) );
   snprintf( s2, 16, "%s", ch->color_str( AT_BOARD2 ) );
   snprintf( s3, 16, "%s", ch->color_str( AT_BOARD3 ) );

   // Now specifying "any" will get around local-board checks. -X (3-23-05)
   if( !argument || argument[0] == '\0' || !str_cmp( argument, "any" ) )
   {
      if( !( board = find_board( ch ) ) )
      {
         board = ch->pcdata->board;
         if( !board )
         {
            for( bd = bdlist.begin(); bd != bdlist.end(); ++bd )
            {
               board = (*bd); // Added to fix crashbug -- X (3-23-05)

               // Immortals and Moderators can read from anywhere as intended... -X (3-23-05)
               if( ( !can_remove( ch, board ) || !ch->is_immortal( ) ) && board->objvnum > 0 )
                  continue;
               if( !can_read( ch, board ) )
                  continue;
               pboard = get_chboard( ch, board->name );
               if( pboard && pboard->alert == BD_IGNORE )
                  continue;
               if( unread_notes( ch, board ) > 0 )
                  break;
            }
            if( bd == bdlist.end() )
            {
               ch->printf( "%sThere are no boards with unread messages&D\r\n", s1 );
               return;
            }
         }
      }

      pboard = create_chboard( ch, board->name );

      for( inote = board->nlist.begin(); inote != board->nlist.end(); ++inote )
      {
         note_data *note = (*inote);

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

   n_num = atoi( argument );

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
   for( inote = board->nlist.begin(); inote != board->nlist.end(); ++inote )
   {
      note_data *note = (*inote);

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

   note_to_char( ch, pnote, board, n_num );
   pboard = create_chboard( ch, board->name );
   if( pboard->last_read < pnote->date_stamp )
      pboard->last_read = pnote->date_stamp;
   act( AT_GREY, "$n reads a note.", ch, NULL, NULL, TO_ROOM );
   return;
}

CMDF( do_note_list )
{
   board_data *board = NULL;
   board_chardata *chboard;
   char buf[MSL];
   char unread;
   int count = 0;
   char s1[16], s2[16], s3[16];

   snprintf( s1, 16, "%s", ch->color_str( AT_BOARD ) );
   snprintf( s2, 16, "%s", ch->color_str( AT_BOARD2 ) );
   snprintf( s3, 16, "%s", ch->color_str( AT_BOARD3 ) );

   if( !( board = find_board( ch ) ) )
   {
      if( !argument || argument[0] == '\0' )
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
      ch->printf( "%sNum   %s%-17s %-11s %s&D\r\n", s1,
                  IS_BOARD_FLAG( board, BOARD_PRIVATE ) ? "" : "Replies ", "Date", "Author", "Subject" );

   count = 0;
   list<note_data*>::iterator inote;
   for( inote = board->nlist.begin(); inote != board->nlist.end(); ++inote )
   {
      note_data *note = (*inote);

      if( IS_BOARD_FLAG( board, BOARD_PRIVATE ) && !is_note_to( ch, note ) && !can_remove( ch, board ) )
         continue;

      ++count;
      if( !chboard || chboard->last_read < note->date_stamp )
         unread = '*';
      else
         unread = ' ';

      if( IS_BOARD_FLAG( board, BOARD_PRIVATE ) )
      {
         ch->printf( "%s%2d%s) &C%c %s[%s%-15s%s] %s%-11s %s&D\r\n", s2, count, s3, unread, s3, s2,
                     mini_c_time( note->date_stamp, ch->pcdata->timezone ), s3, s2,
                     note->sender ? note->sender : "--Error--", note->subject ? print_lngstr( note->subject, 37 ) : "" );
      }
      else
      {
         ch->printf( "%s%2d%s) &C%c %s[ %s%3d%s ] [%s%-15s%s] %s%-11s %-20s&D\r\n", s2, count, s3, unread, s3, s2,
                     note->reply_count, s3, s2, mini_c_time( note->date_stamp, ch->pcdata->timezone ), s3, s2,
                     note->sender ? note->sender : "--Error--", note->subject ? print_lngstr( note->subject, 45 ) : "" );
      }
   }
   ch->printf( "\r\n%sThere %s %s%d%s message%s on this board.&D\r\n", s1, count == 1 ? "is" : "are", s2,
               count, s1, count == 1 ? "" : "s" );
   ch->printf( "%sA &C*%s denotes unread messages.&D\r\n", s1, s1 );
   return;
}

CMDF( do_note_remove )
{
   board_data *board;
   note_data *pnote = NULL;
   char arg[MIL];
   short n_num = 0, r_num = 0, i = 0;
   char s1[16], s2[16], s3[16];

   snprintf( s1, 16, "%s", ch->color_str( AT_BOARD ) );
   snprintf( s2, 16, "%s", ch->color_str( AT_BOARD2 ) );
   snprintf( s3, 16, "%s", ch->color_str( AT_BOARD3 ) );

   if( !( board = find_board( ch ) ) )
   {
      if( !argument || argument[0] == '\0' )
      {
         ch->printf( "%sRemoves a note from the board.\r\n%sSyntax: %serase %s<%sboard%s> <%snote#%s>.[%sreply#%s]&D\r\n",
                     s1, s3, s1, s3, s2, s3, s2, s3, s2, s3 );
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

   if( !IS_BOARD_FLAG( board, BOARD_PRIVATE ) && !can_remove( ch, board ) )
   {
      ch->print( "You are unable to remove the notes on this board.\r\n" );
      return;
   }

   if( !argument || argument[0] == '\0' )
   {
      ch->print( "Remove which note?\r\n" );
      return;
   }

   while( 1 )
   {
      if( argument[i] == '.' )
      {
         r_num = atoi( argument + i + 1 );
         argument[i] = '\0';
         n_num = atoi( argument );
         break;
      }
      if( argument[i] == '\0' )
      {
         n_num = atoi( argument );
         r_num = -1;
         break;
      }
      ++i;
   }

   n_num = atoi( argument );

   if( n_num == 0 )
   {
      ch->print( "Remove which note?\r\n" );
      return;
   }

   if( r_num == 0 )
   {
      ch->print( "Remove which reply?\r\n" );
      return;
   }

   i = 1;
   list<note_data*>::iterator inote;
   for( inote = board->nlist.begin(); inote != board->nlist.end(); ++inote )
   {
      note_data *note = (*inote);

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
      ch->print( "Note not found!\r\n" );
      return;
   }

   if( is_name( "all", pnote->to_list ) && !can_remove( ch, board ) )
   {
      ch->print( "You can not remove that note.\r\n" );
      return;
   }

   if( r_num > 0 )
   {
      note_data *reply = NULL;
      list<note_data*>::iterator rp;

      i = 1;
      for( rp = pnote->rlist.begin(); rp != pnote->rlist.end(); ++rp )
      {
         note_data *rpy = (*rp);

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
         ch->print( "Reply not found!\r\n" );
         return;
      }

      ch->printf( "%sYou remove the reply from %s%s%s, titled '%s%s%s' from the %s%s%s board.&D\r\n",
                  s1, s2, reply->sender ? reply->sender : "--Error--", s1,
                  s2, reply->subject ? reply->subject : "--Error--", s1, s2, board->name, s1 );
      note_remove( board, reply );
      act( AT_GREY, "$n removes a reply from the board.", ch, NULL, NULL, TO_ROOM );
      return;
   }

   ch->printf( "%sYou remove the note from %s%s%s, titled '%s%s%s' from the %s%s%s board.&D\r\n",
               s1, s2, pnote->sender ? pnote->sender : "--Error--", s1,
               s2, pnote->subject ? pnote->subject : "--Error--", s1, s2, board->name, s1 );
   note_remove( board, pnote );
   act( AT_GREY, "$n removes a note from the board.", ch, NULL, NULL, TO_ROOM );
   return;
}

CMDF( do_board_list )
{
   obj_data *obj;
   char buf[MSL];
   int count = 0;
   char s1[16], s2[16], s3[16];

   snprintf( s1, 16, "%s", ch->color_str( AT_BOARD ) );
   snprintf( s2, 16, "%s", ch->color_str( AT_BOARD2 ) );
   snprintf( s3, 16, "%s", ch->color_str( AT_BOARD3 ) );

   if( !str_cmp( argument, "flaghelp" ) && ch->is_immortal( ) )
   {
      ch->printf( "%sP%s: Private Board. Readers can only read posts addressed to them.\r\n   Used commonly with mail boards.\r\n", s2, s1 );
      ch->printf( "%sA%s: Announce Board. New posts are announced to all players.\r\n   Used commonly with news boards.\r\n", s2, s1 );
      ch->printf( "%sB%s: Backup/Archived Board. Expired posts are automatically archived.\r\n   If this flag is not set, expired posts are deleted for good.\r\n", s2, s1 );
      ch->printf( "%s-%s: Flag place-holder\r\n", s2, s1 );
      return;
   }

   if( !str_cmp( argument, "info" ) && ch->is_immortal( ) )
   {
      ch->printf( "%s                                                                Read Post Rmv&D\r\n", s1 );
      ch->printf( "%sNum Name                      Filename        Flags Obj#  Room# Lvl  Lvl  Lvl&D\r\n", s1 );
   }
   else
   {
      ch->printf( "%s                              Total   Total&D\r\n", s1 );
      ch->printf( "%sNum  Name             Unread  Posts  Replies Description&D\r\n", s1 );
   }

   list<board_data*>::iterator bd;
   for( bd = bdlist.begin(); bd != bdlist.end(); ++bd )
   {
      board_data *board = (*bd);

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

      if( !str_cmp( argument, "info" ) && ch->is_immortal( ) )
      {
         ch->printf( "%s%2d%s) %s%-25s ", s2, count + 1, s3, s2, print_lngstr( board->name, 25 ) );
         ch->printf( "%-15s ", print_lngstr( board->filename, 15 ) );
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
         ch->printf( "%s%2d%s)  %s%-15s  %s[ %s%3d%s]  ", s2, count + 1, s3, s2, print_lngstr( board->name, 15 ), s3, s2,
                     unread_notes( ch, board ), s3 );
         if( IS_BOARD_FLAG( board, BOARD_PRIVATE ) )
            ch->printf( "[%s%3d%s]          ", s2, total_notes( ch, board ), s3 );
         else
            ch->printf( "[%s%3d%s]  [%s%3d%s]   ", s2, total_notes( ch, board ), s3, s2, total_replies( board ), s3 );

         ch->printf( "%s%-25s", s2, board->desc ? print_lngstr( board->desc, 25 ) : "" );

         if( ch->is_immortal(  ) )
         {
            if( board->objvnum )
               ch->printf( " %s[%sLocal%s]", s3, s2, s3 );
         }
      }
      ch->print( "&D\r\n" );
      ++count;
   }
   if( count == 0 )
      ch->printf( "%sNo boards to list.&D\r\n", s1 );
   else
   {
      ch->printf( "\r\n%s%d%s board%s found.&D\r\n", s2, count, s1, count == 1 ? "" : "s" );
      if( ch->is_immortal( ) && str_cmp( argument, "info" ) )
         ch->printf( "%s(%sUse %sBOARDS INFO%s to display Immortal information.%s)\r\n", s3, s1, s2, s1, s3 );
      else if( !str_cmp( argument, "info" ) && ch->is_immortal( ) )
         ch->printf( "%s(%sUse %sBOARDS FLAGHELP%s for information on the flag letters.%s)\r\n", s3, s1, s2, s1, s3 );
   }
   return;
}

CMDF( do_board_alert )
{
   board_chardata *chboard;
   board_data *board = NULL;
   char arg[MIL];
   int bd_value = -1;
   char s1[16], s2[16], s3[16];
   static char *const bd_alert_string[] = { "None Set", "Announce", "Ignoring" };

   snprintf( s1, 16, "%s", ch->color_str( AT_BOARD ) );
   snprintf( s2, 16, "%s", ch->color_str( AT_BOARD2 ) );
   snprintf( s3, 16, "%s", ch->color_str( AT_BOARD3 ) );

   if( !argument || argument[0] == '\0' )
   {
      // Fixed crash bug here - X (3-23-05)
      list<board_data*>::iterator bd;
      for( bd = bdlist.begin(); bd != bdlist.end(); ++bd )
      {
         board = (*bd);

         if( !can_read( ch, board ) )
            continue;
         chboard = get_chboard( ch, board->name );
         ch->printf( "%s%-20s   %sAlert: %s%s&D\r\n", s2, board->name, s1, s2,
                     chboard ? bd_alert_string[chboard->alert] : bd_alert_string[0] );
      }
      ch->printf( "%sTo change an alert for a board, type: %salert <board> <none|announce|ignore>&D\r\n", s1, s2 );
      return;
   }

   argument = one_argument( argument, arg );
   if( !( board = get_board( ch, arg ) ) )
   {
      ch->printf( "%sSorry, but the board '%s%s%s' does not exsist.&D\r\n", s1, s2, arg, s1 );
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
      ch->printf( "%sSorry, but '%s%s%s' is not a valid argument.&D\r\n", s1, s2, argument, s1 );
      ch->printf( "%sPlease choose one of: %snone announce ignore&D\r\n", s1, s2 );
      return;
   }

   chboard = create_chboard( ch, board->name );
   chboard->alert = bd_value;
   ch->printf( "%sAlert for the %s%s%s board set to %s%s%s.&D\r\n", s1, s2, board->name, s1, s2, argument, s1 );
   return;
}

/* Much like do_board_list, but I cut out some of the details here for simplicity */
CMDF( do_checkboards )
{
   list<board_data*>::iterator bd;
   board_chardata *chboard;
   obj_data *obj;
   char buf[MSL];
   int count = 0;
   char s1[16], s2[16], s3[16];

   snprintf( s1, 16, "%s", ch->color_str( AT_BOARD ) );
   snprintf( s2, 16, "%s", ch->color_str( AT_BOARD2 ) );
   snprintf( s3, 16, "%s", ch->color_str( AT_BOARD3 ) );

   ch->printf( "%s Num  %-20s  Unread  %-40s&D\r\n", s1, "Name", "Description" );

   for( bd = bdlist.begin(); bd != bdlist.end(); ++bd )
   {
      board_data *board = (*bd);

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
      ch->printf( "%s%3d%s)  %s%-20s  %s[%s%3d%s]  %s%-30s", s2, count, s3, s2, board->name,
                  s3, s2, unread_notes( ch, board ), s3, s2, board->desc ? board->desc : "" );

      if( ch->is_immortal(  ) )
      {
         snprintf( buf, MSL, "%d", board->objvnum );
         if( ( obj = ch->get_obj_world( buf ) ) && ( obj->in_room != NULL ) )
            ch->printf( " %s[%sObj# %s%-5d %s@ %sRoom# %s%-5d%s]", s3, s1, s2, board->objvnum,
                        s3, s1, s2, obj->in_room->vnum, s3 );
         else
            ch->printf( " %s[ %sGlobal Board %s]", s3, s2, s3 );
      }
      ch->print( "&D\r\n" );
   }
   if( count == 0 )
      ch->printf( "%sNo unread messages...\r\n", s1 );
   return;
}

CMDF( do_board_stat )
{
   board_data *board;
   char s1[16], s2[16], s3[16];

   snprintf( s1, 16, "%s", ch->color_str( AT_BOARD ) );
   snprintf( s2, 16, "%s", ch->color_str( AT_BOARD2 ) );
   snprintf( s3, 16, "%s", ch->color_str( AT_BOARD3 ) );

   if( !argument || argument[0] == '\0' )
   {
      ch->print( "Usage: bstat <board>\r\n" );
      return;
   }

   if( !( board = get_board( ch, argument ) ) )
   {
      ch->printf( "&wSorry, '&W%s&w' is not a valid board.\r\n", argument );
      return;
   }

   ch->printf( "%sFilename: %s%-20s&D\r\n", s1, s2, board->filename );
   ch->printf( "%sName:       %s%-30s%s ObjVnum:      ", s1, s2, board->name, s1 );
   if( board->objvnum > 0 )
      ch->printf( "%s%d&D\r\n", s2, board->objvnum );
   else
      ch->printf( "%sGlobal Board&D\r\n", s2 );
   ch->printf( "%sReaders:    %s%-30s%s Read Level:   %s%d&D\r\n", s1, s2,
               board->readers ? board->readers : "none set", s1, s2, board->read_level );
   ch->printf( "%sPosters:    %s%-30s%s Post Level:   %s%d&D\r\n", s1, s2,
               board->posters ? board->posters : "none set", s1, s2, board->post_level );
   ch->printf( "%sModerators: %s%-30s%s Remove Level: %s%d&D\r\n", s1, s2,
               board->moderators ? board->moderators : "none set", s1, s2, board->remove_level );
   ch->printf( "%sGroup:      %s%-30s%s Expiration:   %s%d&D\r\n", s1, s2,
               board->group ? board->group : "none set", s1, s2, board->expire );
   ch->printf( "%sFlags: %s[%s%s%s]&D\r\n", s1, s3, s2,
               board->flags.any(  )? bitset_string( board->flags, board_flags ) : "none set", s3 );
   ch->printf( "%sDescription: %s%-30s&D\r\n", s1, s2, board->desc ? board->desc : "none set" );
   return;
}

CMDF( do_board_remove )
{
   board_data *board;
   char arg[MIL];
   char buf[MSL];
   char s1[16], s2[16], s3[16];

   snprintf( s1, 16, "%s", ch->color_str( AT_BOARD ) );
   snprintf( s2, 16, "%s", ch->color_str( AT_BOARD2 ) );
   snprintf( s3, 16, "%s", ch->color_str( AT_BOARD3 ) );

   if( !argument || argument[0] == '\0' )
   {
      ch->printf( "%sYou must select a board to remove.&D\r\n", s1 );
      return;
   }

   argument = one_argument( argument, arg );
   if( !( board = get_board( ch, arg ) ) )
   {
      ch->printf( "%sSorry, '%s%s%s' is not a valid board.&D\r\n", s1, s2, argument, s1 );
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
      ch->printf( "&RRemoving a board will also delete *ALL* posts on that board!\r\n&wTo continue, type: "
                  "&YBOARD REMOVE '%s' YES&w.", board->name );
      return;
   }
   return;
}

CMDF( do_board_make )
{
   board_data *board;
   char arg[MIL];

   argument = one_argument( argument, arg );

   if( !argument || argument[0] == '\0' || !arg || arg[0] == '\0' )
   {
      ch->print( "Usage: makeboard <filename> <name>\r\n" );
      return;
   }

   if( strlen( argument ) > 20 || strlen( arg ) > 20 )
   {
      ch->print( "Please limit board names and filenames to 20 characters!\r\n" );
      return;
   }

   smash_tilde( argument );
   board = new board_data;

   board->name = STRALLOC( argument );
   board->filename = str_dup( arg );
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
   return;
}

CMDF( do_board_set )
{
   board_data *board;
   char arg1[MIL], arg2[MIL], arg3[MIL], buf[MSL];
   int value = -1;
   char s1[16], s2[16], s3[16];

   snprintf( s1, 16, "%s", ch->color_str( AT_BOARD ) );
   snprintf( s2, 16, "%s", ch->color_str( AT_BOARD2 ) );
   snprintf( s3, 16, "%s", ch->color_str( AT_BOARD3 ) );

   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );

   if( !arg1 || arg1[0] == '\0' || ( str_cmp( arg1, "purge" ) && ( !arg2 || arg2[0] == '\0' ) ) )
   {
      ch->print( "Usage: bset <board> <field> value\r\n" );
      ch->print( "   Or: bset purge (this option forces a check on expired notes)\r\n" );
      ch->print( "\r\nField being one of:\r\n" );
      ch->print( "  objvnum read_level post_level remove_level expire group\r\n" );
      ch->print( "  readers posters moderators desc name global flags filename\r\n" );
      return;
   }

   if( is_number( argument ) )
      value = atoi( argument );

   if( !str_cmp( arg1, "purge" ) )
   {
      log_printf( "Manual board pruning started by %s.", ch->name );
      check_boards( );
      return;
   }

   if( !( board = get_board( ch, arg1 ) ) )
   {
      ch->printf( "%sSorry, but '%s%s%s' is not a valid board.&D\r\n", s1, s2, arg1, s1 );
      return;
   }

   if( !str_cmp( arg2, "flags" ) )
   {
      bool fMatch = false, fUnknown = false;

      ch->printf( "%sSetting flags: %s", s1, s2 );
      snprintf( buf, MSL, "\r\n%sUnknown flags: %s", s1, s2 );
      while( argument[0] != '\0' )
      {
         argument = one_argument( argument, arg3 );
         value = get_board_flag( arg3 );
         if( value < 0 || value >= MAX_BOARD_FLAGS )
         {
            fUnknown = true;
            snprintf( buf + strlen( buf ), MSL - strlen( buf ), " %s", arg3 );
         }
         else
         {
            fMatch = true;
            TOGGLE_BOARD_FLAG( board, value );
            if( IS_BOARD_FLAG( board, value ) )
               ch->printf( " +%s", arg3 );
            else
               ch->printf( " -%s", arg3 );
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
      ch->printf( "%sThe objvnum for '%s%s%s' has been set to '%s%d%s'.&D\r\n", s1, s2,
                  board->name, s1, s2, board->objvnum, s1 );
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
         ch->printf( "%s%d%s is out of range.\r\nValues range from %s1%s to %s%d%s.&D\r\n", s2, value, s1, s2,
                     s1, s2, ch->level, s1 );
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
         ch->printf( "%sUnknown field '%s%s%s'.&D\r\n", s1, s2, arg2, s1 );
         return;
      }
      write_boards(  );
      ch->printf( "%sThe %s%s%s for '%s%s%s' has been set to '%s%d%s'.&D\r\n", s1, s3, arg2,
                  s1, s2, board->name, s1, s2, value, s1 );
      return;
   }

   if( !str_cmp( arg2, "readers" ) || !str_cmp( arg2, "posters" ) || !str_cmp( arg2, "moderators" ) )
   {
      if( !str_cmp( arg2, "readers" ) )
      {
         STRFREE( board->readers );
         if( !argument || argument[0] == '\0' )
            board->readers = NULL;
         else
            board->readers = STRALLOC( argument );
      }
      else if( !str_cmp( arg2, "posters" ) )
      {
         STRFREE( board->posters );
         if( !argument || argument[0] == '\0' )
            board->posters = NULL;
         else
            board->posters = STRALLOC( argument );
      }
      else if( !str_cmp( arg2, "moderators" ) )
      {
         STRFREE( board->moderators );
         if( !argument || argument[0] == '\0' )
            board->moderators = NULL;
         else
            board->moderators = STRALLOC( argument );
      }
      else
      {
         ch->printf( "%sUnknown field '%s%s%s'.&D\r\n", s1, s2, arg2, s1 );
         return;
      }
      write_boards(  );
      ch->printf( "%sThe %s%s%s for '%s%s%s' have been set to '%s%s%s'.\r\n", s1, s3, arg2,
                  s1, s2, board->name, s1, s2, argument[0] != '\0' ? argument : "(nothing)", s1 );
      return;
   }

   if( !str_cmp( arg2, "filename" ) )
   {
      char filename[256];

      if( !is_valid_filename( ch, BOARD_DIR, argument ) )
         return;

      if( strlen( argument ) > 20 )
      {
         ch->print( "Please limit board filenames to 20 characters!\n\r" );
         return;
      }
      snprintf( filename, 256, "%s%s.board", BOARD_DIR, board->filename );
      unlink( filename );
      mudstrlcpy( filename, board->filename, 256 );
      DISPOSE( board->filename );
      board->filename = str_dup( argument );
      write_boards(  );
      write_board( board );
      ch->printf( "%sThe filename for '%s%s%s' has been changed to '%s%s%s'.\n\r", s1, s2, filename, s1, s2,
                 board->filename, s1 );
      return;
   }

   if( !str_cmp( arg2, "name" ) )
   {
      if( !argument || argument[0] == '\0' )
      {
         ch->print( "No name specified.\r\n" );
         return;
      }
      if( strlen( argument ) > 20 )
      {
         ch->print( "Please limit board names to 20 characters!\r\n" );
         return;
      }
      mudstrlcpy( buf, board->name, MSL );
      STRFREE( board->name );
      board->name = STRALLOC( argument );
      write_boards(  );
      ch->printf( "%sThe name for '%s%s%s' has been changed to '%s%s%s'.\r\n", s1, s2, buf, s1, s2, board->name, s1 );
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
      ch->printf( "%sFrom now on, notes on the %s%s%s board will expire after %s%d%s days.\r\n", s1, s2,
                  board->name, s1, s2, board->expire, s1 );
      ch->printf( "%sPlease note: This will not effect notes currently on the board. To effect %sALL%s notes, "
                  "type: %sbset <board> expireall <days>&D\r\n", s1, s3, s1, s2 );
      write_boards(  );
      return;
   }

   if( !str_cmp( arg2, "expireall" ) )
   {
      list<note_data*>::iterator inote;

      if( value < 0 || value > MAX_BOARD_EXPIRE )
      {
         ch->printf( "%sExpire time must be a value between %s0%s and %s%d%s.&D\r\n", s1, s2, s1, s2, MAX_BOARD_EXPIRE, s1 );
         return;
      }

      board->expire = value;
      for( inote = board->nlist.begin(); inote != board->nlist.end(); ++inote )
      {
         note_data *note = (*inote);

         note->expire = value;
      }
      ch->printf( "%sAll notes on the %s%s%s board will expire after %s%d%s days.&D\r\n", s1, s2, board->name, s1,
                  s2, board->expire, s1 );
      ch->print( "Performing board pruning now...\r\n" );
      board_check_expire( board );
      write_boards(  );
      return;
   }

   if( !str_cmp( arg2, "desc" ) )
   {
      if( !argument || argument[0] == '\0' )
      {
         ch->print( "No description specified.\r\n" );
         return;
      }
      if( strlen( argument ) > 30 )
      {
         ch->print( "Please limit your board descriptions to 30 characters.\r\n" );
         return;
      }
      DISPOSE( board->desc );
      board->desc = str_dup( argument );
      write_boards(  );
      ch->printf( "%sThe desc for %s%s%s has been set to '%s%s%s'.&D\r\n", s1, s2, board->name, s1, s2, board->desc, s1 );
      return;
   }

   if( !str_cmp( arg2, "group" ) )
   {
      if( !argument || argument[0] == '\0' )
      {
         STRFREE( board->group );
         board->group = NULL;
         ch->print( "Group cleared.\r\n" );
         return;
      }
      STRFREE( board->group );
      board->group = STRALLOC( argument );
      write_boards(  );
      ch->printf( "%sThe group for %s%s%s has been set to '%s%s%s'.&D\r\n", s1, s2, board->name, s1, s2, board->group, s1 );
      return;
   }

   if( !str_cmp( arg2, "raise" ) )
   {
      list<board_data*>::iterator bd;

      for( bd = bdlist.begin(); bd != bdlist.end(); ++bd )
      {
         if( (*bd) == board )
            break;
      }
      if( bd == bdlist.begin() )
      {
         ch->printf( "%sBut '%s%s%s' is already the first board!&D\r\n", s1, s2, board->name, s1 );
         return;
      }
      --bd;

      bdlist.remove( board );
      bdlist.insert( bd, board );

      ch->printf( "%sMoved '%s%s%s' above '%s%s%s'.&D\r\n", s1, s2, board->name, s1, s2, (*bd)->name, s1 );
      return;
   }

   if( !str_cmp( arg2, "lower" ) )
   {
      list<board_data*>::iterator bd;

      for( bd = bdlist.begin(); bd != bdlist.end(); ++bd )
      {
         if( (*bd) == board )
            break;
      }

      if( ++bd == bdlist.end() )
      {
         ch->printf( "%sBut '%s%s%s' is already the last board!&D\r\n", s1, s2, board->name, s1 );
         return;
      }
      --bd;

      ++bd; ++bd;
      if( bd == bdlist.end() )
      {
         bdlist.remove( board );
         bdlist.push_back( board );
      }
      else
      {
         --bd; --bd;

         bdlist.remove( board );
         bdlist.insert( bd, board );
      }
      ch->printf( "%sMoved '%s%s%s' below '%s%s%s'.&D\r\n", s1, s2, board->name, s1, s2, (*bd)->name, s1 );
      return;
   }
   do_board_set( ch, "" );
   return;
}

/* Begin Project Code */

#define PROJ_ADMIN( proj, ch ) ( !((ch))->isnpc() && ( (((proj)->type == 1) && ((ch)->pcdata->realm == REALM_HEAD_CODER)) \
                                  || (((proj)->type == 2) && ((ch)->pcdata->realm == REALM_HEAD_BUILDER)) \
                                  || ((ch)->pcdata->realm == REALM_IMP)))

project_data *get_project_by_number( int pnum )
{
   list<project_data*>::iterator pr;
   int pcount = 1;

   for( pr = projlist.begin(); pr != projlist.end(); ++pr )
   {
      project_data *proj = (*pr);

      if( pcount == pnum )
         return proj;
      else
         ++pcount;
   }
   return NULL;
}

note_data *get_log_by_number( project_data * pproject, int pnum )
{
   list<note_data*>::iterator ilog;
   int pcount = 1;

   for( ilog = pproject->nlist.begin(); ilog != pproject->nlist.end(); ++ilog )
   {
      note_data *plog = (*ilog);

      if( pcount == pnum )
         return plog;
      else
         ++pcount;
   }
   return NULL;
}

void write_projects( void )
{
   list<project_data*>::iterator pr;
   list<note_data*>::iterator ilog;
   FILE *fpout;

   fpout = fopen( PROJECTS_FILE, "w" );
   if( !fpout )
   {
      bug( "%s: FATAL: cannot open projects.txt for writing!", __FUNCTION__ );
      return;
   }
   for( pr = projlist.begin(); pr != projlist.end(); ++pr )
   {
      project_data *proj = (*pr);

      fprintf( fpout, "%s", "Version        2\n" );
      fprintf( fpout, "Name   %s~\n", proj->name );
      fprintf( fpout, "Owner  %s~\n", ( proj->owner ) ? proj->owner : "None" );
      if( proj->coder )
         fprintf( fpout, "Coder  %s~\n", proj->coder );
      fprintf( fpout, "Status %s~\n", ( proj->status ) ? proj->status : "No update." );
      fprintf( fpout, "Date_stamp   %ld\n", ( long int )proj->date_stamp );
      fprintf( fpout, "Type         %d\n", proj->type );
      if( proj->description )
         fprintf( fpout, "Description %s~\n", proj->description );

      for( ilog = proj->nlist.begin(); ilog != proj->nlist.end(); ++ilog )
      {
         note_data *nlog = (*ilog);

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
         bug( "%s: bad key word: %s", __FUNCTION__, word );
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
         bug( "%s: EOF encountered reading file!", __FUNCTION__ );
         word = "End";
      }

      switch ( UPPER( word[0] ) )
      {
         default:
            bug( "%s: no match: %s", __FUNCTION__, word );
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
                  bug( "%s: couldn't read log!", __FUNCTION__ );
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

         case 'S':
            KEY( "Status", project->status, fread_string_nohash( fp ) );
            break;

         case 'T':
            KEY( "Type", project->type, fread_number( fp ) );
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

   projlist.clear();

   if( !( fp = fopen( PROJECTS_FILE, "r" ) ) )
      return;

   while( ( project = read_project( fp ) ) != NULL )
      projlist.push_back( project );

   return;
}

project_data::project_data()
{
   init_memory( &name, &taken, sizeof( taken ) );
   nlist.clear();
}

project_data::~project_data()
{
   list<note_data*>::iterator ilog;

   for( ilog = nlist.begin(); ilog != nlist.end(); )
   {
      note_data *note = (*ilog);
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
   list<project_data*>::iterator proj;

   for( proj = projlist.begin(); proj != projlist.end(); )
   {
      project_data *project = (*proj);
      ++proj;

      deleteptr( project );
   }
   return;
}

/* Last thing left to revampitize -- Xorith */
CMDF( do_project )
{
   project_data *pproject;
   char arg[MIL], buf[MSL];
   int pcount, pnum, ptype = 0;

   if( ch->isnpc(  ) )
      return;

   if( !ch->desc )
   {
      bug( "%s: no descriptor", __FUNCTION__ );
      return;
   }

   switch ( ch->substate )
   {
      default:
         break;
      case SUB_WRITING_NOTE:
         if( !ch->pcdata->pnote )
         {
            bug( "%s: log got lost?", __FUNCTION__ );
            ch->print( "Your log was lost!\r\n" );
            ch->stop_editing(  );
            return;
         }
         if( ch->pcdata->dest_buf != ch->pcdata->pnote )
            bug( "%s: sub_writing_note: ch->pcdata->dest_buf != ch->pcdata->pnote", __FUNCTION__ );
         DISPOSE( ch->pcdata->pnote->text );
         ch->pcdata->pnote->text = ch->copy_buffer( false );
         ch->stop_editing(  );
         return;
      case SUB_PROJ_DESC:
         if( !ch->pcdata->dest_buf )
         {
            ch->print( "Your description was lost!" );
            bug( "%s: sub_project_desc: NULL ch->pcdata->dest_buf", __FUNCTION__ );
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

   if( !str_cmp( arg, "save" ) )
   {
      write_projects(  );
      ch->print( "Projects saved.\r\n" );
      return;
   }

   if( !str_cmp( arg, "code" ) )
   {
      list<project_data*>::iterator pr;

      pcount = 0;
      ch->pager( " #  | Owner       | Project                    \r\n" );
      ch->pager( "----|-------------|----------------------------\r\n" );
      for( pr = projlist.begin(); pr != projlist.end(); ++pr )
      {
         project_data *proj = (*pr);

         ++pcount;
         if( ( proj->status && str_cmp( proj->status, "approved" ) ) || proj->coder != NULL )
            continue;
         ch->pagerf( "%2d%c | %-11s | %-26s\r\n",
                     pcount, proj->type == 1 ? 'C' : 'B', proj->owner ? proj->owner : "(None)",
                     proj->name ? proj->name : "(None)" );
      }
      return;
   }
   if( !str_cmp( arg, "more" ) || !str_cmp( arg, "mine" ) )
   {
      list<project_data*>::iterator pr;
      list<note_data*>::iterator ilog;
      bool MINE = false;

      pcount = 0;

      if( !str_cmp( arg, "mine" ) )
         MINE = true;

      ch->pager( "\r\n" );
      ch->pager( " #  | Owner       | Project                    | Coder       | Status     | Logs\r\n" );
      ch->pager( "----|-------------|----------------------------|-------------|------------|-----\r\n" );
      for( pr = projlist.begin(); pr != projlist.end(); ++pr )
      {
         project_data *proj = (*pr);

         ++pcount;
         if( MINE && ( !proj->owner || str_cmp( ch->name, proj->owner ) )
             && ( !proj->coder || str_cmp( ch->name, proj->coder ) ) )
            continue;
         else if( !MINE && proj->status && !str_cmp( "Done", proj->status ) )
            continue;
         int num_logs = proj->nlist.size();
         ch->pagerf( "%2d%c | %-11s | %-26s | %-11s | %-10s | %3d\r\n",
                     pcount, proj->type == 1 ? 'C' : 'B', proj->owner ? proj->owner : "(None)",
                     print_lngstr( proj->name, 26 ), proj->coder ? proj->coder : "(None)",
                     proj->status ? proj->status : "(None)", num_logs );
      }
      return;
   }
   if( !arg || arg[0] == '\0' || !str_cmp( arg, "list" ) )
   {
      list<project_data*>::iterator pr;
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
      for( pr = projlist.begin(); pr != projlist.end(); ++pr )
      {
         project_data *proj = (*pr);

         ++pcount;
         if( proj->status && !str_cmp( "Done", proj->status ) )
            continue;
         if( !aflag )
         {
            ch->pagerf( "%2d%c | %-11s | %-26s | %-15s | %-12s\r\n",
                        pcount, proj->type == 1 ? 'C' : 'B', proj->owner ? proj->owner : "(None)",
                        print_lngstr( proj->name, 26 ),
                        mini_c_time( proj->date_stamp, ch->pcdata->timezone ),
                        proj->status ? proj->status : "(None)" );
         }
         else if( !proj->taken )
         {
            if( !projects_available )
               projects_available = true;
            ch->pagerf( "%2d%c | %-30s | %s\r\n", pcount, proj->type == 1 ? 'C' : 'B',
                        proj->name ? proj->name : "(None)",
                        mini_c_time( proj->date_stamp, ch->pcdata->timezone ) );
         }
      }
      if( pcount == 0 )
         ch->pager( "No projects exist.\r\n" );
      else if( aflag && !projects_available )
         ch->pager( "No projects available.\r\n" );
      return;
   }

   if( !str_cmp( arg, "add" ) )
   {
      if( ch->get_trust(  ) < LEVEL_GOD && !( ch->pcdata->realm == REALM_HEAD_CODER
                                              || ch->pcdata->realm == REALM_HEAD_BUILDER
                                              || ch->pcdata->realm == REALM_IMP ) )
      {
         ch->print( "You are not powerful enough to add a new project.\r\n" );
         return;
      }

      if( ( ch->pcdata->realm == REALM_IMP ) )
      {
         argument = one_argument( argument, arg );

         if( ( !arg || arg[0] == '\0' ) || ( str_cmp( arg, "code" ) && str_cmp( arg, "build" ) ) )
         {
            ch->print( "Since you are an Implementor, you must specify either Build or Code for a project type.\r\n" );
            ch->print( "Syntax would be: PROJECT ADD [CODE/BUILD] [NAME]\r\n" );
            return;
         }
      }

      switch ( ch->pcdata->realm )
      {
         case REALM_HEAD_CODER:
            ptype = 1;
            break;
         default:
         case REALM_HEAD_BUILDER:
            ptype = 2;
            break;
         case REALM_IMP:
            if( !str_cmp( arg, "code" ) )
               ptype = 1;
            else
               ptype = 2;
      }

      project_data *new_project = new project_data;
      new_project->name = str_dup( argument );
      new_project->coder = NULL;
      new_project->taken = false;
      new_project->description = NULL;
      new_project->type = ptype;
      new_project->date_stamp = current_time;
      projlist.push_back( new_project );
      write_projects(  );
      ch->printf( "New %s Project '%s' added.\r\n", ( new_project->type == 1 ) ? "Code" : "Build", new_project->name );
      return;
   }

   if( !is_number( arg ) )
   {
      ch->printf( "%s is an invalid project!\r\n", arg );
      return;
   }

   pnum = atoi( arg );
   pproject = get_project_by_number( pnum );
   if( !pproject )
   {
      ch->printf( "Project #%d does not exsist.\r\n", pnum );
      return;
   }

   argument = one_argument( argument, arg );

   if( !str_cmp( arg, "description" ) )
   {
      if( ch->get_trust(  ) < LEVEL_GOD && !PROJ_ADMIN( pproject, ch ) )
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
      if( !PROJ_ADMIN( pproject, ch ) && ch->get_trust(  ) < LEVEL_ASCENDANT )
      {
         ch->print( "You are not high enough level to delete a project.\r\n" );
         return;
      }
      snprintf( buf, MSL, "%s", pproject->name );
      deleteptr( pproject );
      write_projects(  );
      ch->printf( "Project '%s' has been deleted.\r\n", buf );
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
      else if( pproject->taken && !PROJ_ADMIN( pproject, ch ) )
      {
         ch->print( "This project is already taken.\r\n" );
         return;
      }
      else if( pproject->taken && PROJ_ADMIN( pproject, ch ) )
         ch->printf( "Taking Project: '%s' from Owner: '%s'!\r\n", pproject->name,
                     pproject->owner ? pproject->owner : "NULL" );

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
         ch->printf( "You removed yourself as the %s.\r\n", pproject->type == 1 ? "coder" : "builder" );
         write_projects(  );
         return;
      }
      else if( pproject->coder && !PROJ_ADMIN( pproject, ch ) )
      {
         ch->printf( "This project already has a %s.\r\n", pproject->type == 1 ? "coder" : "builder" );
         return;
      }
      else if( pproject->coder && PROJ_ADMIN( pproject, ch ) )
      {
         ch->printf( "Removing %s as %s of this project!\r\n", pproject->coder ? pproject->coder : "NULL",
                     pproject->type == 1 ? "coder" : "builder" );
         STRFREE( pproject->coder );
         ch->print( "Coder removed.\r\n" );
         write_projects(  );
         return;
      }

      pproject->coder = QUICKLINK( ch->name );
      write_projects(  );
      ch->printf( "You are now the %s of %s.\r\n", pproject->type == 1 ? "coder" : "builder", pproject->name );
      return;
   }

   if( !str_cmp( arg, "status" ) )
   {
      if( pproject->owner && str_cmp( pproject->owner, ch->name )
          && ch->get_trust(  ) < LEVEL_GREATER && pproject->coder
          && str_cmp( pproject->coder, ch->name ) && !PROJ_ADMIN( pproject, ch ) )
      {
         ch->print( "This is not your project!\r\n" );
         return;
      }
      DISPOSE( pproject->status );
      pproject->status = str_dup( argument );
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
                                 pproject->name ? pproject->name : "(No name)",
                                 ch->pcdata->pnote->subject ? ch->pcdata->pnote->subject : "(No subject)" );
         ch->start_editing( ch->pcdata->pnote->text );
         return;
      }

      argument = one_argument( argument, arg );

      if( !str_cmp( arg, "subject" ) )
      {
         if( !ch->pcdata->pnote )
            ch->note_attach(  );
         DISPOSE( ch->pcdata->pnote->subject );
         ch->pcdata->pnote->subject = str_dup( argument );
         ch->printf( "Log Subject set to: %s\r\n", ch->pcdata->pnote->subject );
         return;
      }

      if( !str_cmp( arg, "post" ) )
      {
         if( ( pproject->owner && str_cmp( ch->name, pproject->owner ) )
             || ( pproject->coder && str_cmp( ch->name, pproject->coder ) )
             || ( ( ch->get_trust(  ) < LEVEL_GREATER ) && !PROJ_ADMIN( pproject, ch ) ) )
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

         if( argument && argument[0] != '\0' )
         {
            int plog_num;
            note_data *tplog;
            plog_num = atoi( argument );

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
         list<note_data*>::iterator ilog;

         if( ( pproject->owner && str_cmp( ch->name, pproject->owner ) )
             || ( pproject->coder && str_cmp( ch->name, pproject->coder ) )
             || ( ( ch->get_trust(  ) < LEVEL_SAVIOR ) && !PROJ_ADMIN( pproject, ch ) ) )
         {
            ch->print( "This is not your project!\r\n" );
            return;
         }

         pcount = 0;
         ch->pagerf( "Project: %-12s: %s\r\n", pproject->owner ? pproject->owner : "(None)", pproject->name );

         for( ilog = pproject->nlist.begin(); ilog != pproject->nlist.end(); ++ilog )
         {
            note_data *tlog = (*ilog);

            ++pcount;
            ch->pagerf( "%2d) %-12s: %s\r\n", pcount, tlog->sender ? tlog->sender : "--Error--",
                        tlog->subject ? tlog->subject : "" );
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

      pnum = atoi( arg );

      plog = get_log_by_number( pproject, pnum );
      if( !plog )
      {
         ch->print( "Invalid log.\r\n" );
         return;
      }

      if( !str_cmp( argument, "delete" ) )
      {
         if( ( pproject->owner && str_cmp( ch->name, pproject->owner ) )
             || ( pproject->coder && str_cmp( ch->name, pproject->coder ) )
             || ( ( ch->get_trust(  ) < LEVEL_ASCENDANT ) && !PROJ_ADMIN( pproject, ch ) ) )
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
             || ( pproject->coder && str_cmp( ch->name, pproject->coder ) )
             || ( ( ch->get_trust(  ) < LEVEL_SAVIOR ) && !PROJ_ADMIN( pproject, ch ) ) )
         {
            ch->print( "This is not your project!\r\n" );
            return;
         }
         note_to_char( ch, plog, NULL, -1 );
         return;
      }
   }
   interpret( ch, "help project" );
   return;
}
