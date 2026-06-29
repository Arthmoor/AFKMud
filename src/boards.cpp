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
 *                     Completely Revised Boards Module                     *
 ****************************************************************************
 * Revised by:   Xorith 9/21/07                                             *
 * Last Updated: 6/20/2026 by Samson                                        *
 ****************************************************************************/

#include <filesystem>
#include <fstream>
#include "mud.h"
#include "boards.h"
#include "clans.h"
#include "deity.h"
#include "descriptor.h"
#include "objindex.h"
#include "realms.h"
#include "roomindex.h"

std::list<board_data *> bdlist;
std::list<project_data *> projlist;
std::vector<std::string> board_files;

void check_boards(  );
bool check_parse_name( std::string, bool );

/* Global */
std::chrono::system_clock::time_point board_expire_time_t;

// Changed 'backup_pruned' to 'backup', to be more sane. --Xorith (9/21/07)
// There's no conversion code, so if you want backups - set the flag again. -X
const char *board_flags[] = {
   "r1", "backup", "private", "announce"
};

const char *note_flags[] = {
   "r1", "sticky", "closed", "hidden"
};

int get_note_flag( std::string_view note_flag )
{
   for( int x = 0; x < MAX_NOTE_FLAGS; ++x )
      if( !str_cmp( note_flag, note_flags[x] ) )
         return x;
   return -1;
}

int get_board_flag( std::string_view board_flag )
{
   for( int x = 0; x < MAX_BOARD_FLAGS; ++x )
      if( !str_cmp( board_flag, board_flags[x] ) )
         return x;
   return -1;
}

bool IS_BOARD_FLAG( const board_data * board, int flag )
{
   if( board->flags.test( flag ) )
      return true;
   return false;
}

void TOGGLE_BOARD_FLAG( board_data * board, int flag )
{
   board->flags.flip( flag );
}

bool IS_NOTE_FLAG( const note_data * note, int flag )
{
   if( note->flags.test( flag ) )
      return true;
   return false;
}

void TOGGLE_NOTE_FLAG( note_data * note, int flag )
{
   note->flags.flip( flag );
}

/* This is a simple function that keeps a string within a certain length. -- Xorith */
std::string print_lngstr( std::string_view src, size_t size )
{
   std::string rstring;

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
}

board_chardata::~board_chardata(  )
{
}

note_data::note_data(  )
{
}

note_data::~note_data(  )
{
   for( auto it = rlist.begin(); it != rlist.end(); )
   {
      note_data *reply = *it;
      ++it;

      rlist.remove( reply );
      deleteptr( reply );
   }
}

board_data::board_data(  )
{
}

board_data::~board_data(  )
{
   for( auto it = nlist.begin(); it != nlist.end(); )
   {
      note_data *pnote = *it;
      ++it;

      deleteptr( pnote );
   }
   bdlist.remove( this );
}

void pc_data::free_pcboards(  )
{
   for( auto it = boarddata.begin(); it != boarddata.end(); )
   {
      board_chardata *chbd = *it;
      ++it;

      deleteptr( chbd );
   }
}

void free_boards( void )
{
   for( auto it = bdlist.begin(  ); it != bdlist.end(  ); )
   {
      board_data *board = *it;
      ++it;

      deleteptr( board );
   }
}

void read_reply( note_data * note, note_data * reply, int file_ver, std::ifstream & stream )
{
   std::string key;
   while( stream >> key )
   {
      if( key == "Reply-Sender" )
         note->sender = fread_line( stream );
      else if( key == "Reply-To" )
         note->to_list = fread_line( stream );
      else if( key == "Reply-Subject" )
         note->subject = fread_line( stream );
      else if( key == "Reply-DateStamp" )
      {
         time_t loaded_time;

         stream >> loaded_time;
         note->date_stamp = std::chrono::system_clock::from_time_t( loaded_time );
      }
      else if( key == "Reply-Flags" )
      {
         if( file_ver < 1 )
            stream >> note->flags;
         else
         {
            std::string flags = fread_line( stream );
            flag_string_set( flags, note->flags, note_flags );
         }
      }
      else if( key == "Reply-Text" )
         note->text = fread_line( stream );
      else if( key == "#REPLY-END" )
         return;
      else
         bug( "{}: Bad section '{}' in reply - skipping.", __func__, key );
   }
}

void read_note( note_data * note, int file_ver, std::ifstream & stream )
{
   std::string key;
   while( stream >> key )
   {
      if( key == "Sender" )
         note->sender = fread_line( stream );
      else if( key == "Subject" )
         note->subject = fread_line( stream );
      else if( key == "To" )
         note->to_list = fread_line( stream );
      else if( key == "DateStamp" )
      {
         time_t loaded_time;

         stream >> loaded_time;
         note->date_stamp = std::chrono::system_clock::from_time_t( loaded_time );
      }
      else if( key == "Flags" )
      {
         if( file_ver < 1 )
            stream >> note->flags;
         else
         {
            std::string flags = fread_line( stream );
            flag_string_set( flags, note->flags, note_flags );
         }
      }
      else if( key == "Expire" )
      {
         time_t loaded_time;

         stream >> loaded_time;
         note->expire = std::chrono::system_clock::from_time_t( loaded_time );
      }
      else if( key == "Text" )
         note->text = fread_line( stream );
      else if( key == "Type" )
         stream >> note->type;
      else if( key == "#REPLY" )
      {
         note_data *reply = new note_data;
         read_reply( note, reply, file_ver, stream );
         note->rlist.push_back( reply );
      }
      else if( key == "#NOTE-END" || key == "#LOG-END" || key == "#PLR-COMMENT-END" )
         return;
      else
         bug( "{}: Bad section '{}' in note - skipping.", __func__, key );
   }
}

void board_check_expire( board_data * );

bool read_board_list( void )
{
   std::ifstream stream( std::filesystem::path{BOARD_LIST_FILE} );

   if( !stream.is_open( ) )
   {
      bug( "{}: Cannot open {} for reading: {}", __func__, BOARD_LIST_FILE, std::strerror(errno) );
      return false;
   }

   board_files.clear();

   std::string line;
   while( !stream.eof() )
   {
      line = fread_line( stream );

      if( line.empty() )
         continue;

      board_files.push_back( line );
   }
   stream.close();
   return true;
}

void load_boards( void )
{
   if( !read_board_list() )
   {
      log_printf( "{}: Aborting board reading due to previous error.", __func__ );
      return;
   }

   bdlist.clear(  ); // Probably not necessary but we're going to play it safe.

   for( size_t x = 0; x < board_files.size(); ++x )
   {
      std::ifstream stream;
      std::filesystem::path board_file = std::format( "{}{}", BOARD_DIR, board_files[x] );
      stream.open( board_file );
      if( !stream.is_open() )
      {
         bug( "{}: Cannot open {} for reading: {}", __func__, board_file.string(), std::strerror(errno) );
         continue;
      }

      log_string( board_file.string() );
      std::string key;
      board_data *board = nullptr;

      int file_ver = 0;
      stream >> key;
      if( key == "#VERSION" )
         stream >> file_ver;
      else
      {
         bug( "{}: Invalid format for board {} - No version header.", __func__, board_file.string() );
         continue;
      }

      while( stream >> key )
      {
         if( key == "#BOARD" )
         {
            board = new board_data;
            continue;
         }

         if( key == "#BOARD-END" )
         {
            if( !board )
            {
               bug( "{}: No #BOARD section seen yet!", __func__ );
               continue;
            }
            bdlist.push_back( board );
            board = nullptr;
            continue;
         }

         if( !board )
         {
            bug( "{}: Found keyword '{}' outside of #BOARD block in {}", __func__, key, board_file.string() );
            continue;
         }

         if( key == "Name" )
         {
            board->name = fread_line( stream );
            board->filename = board->name + ".board";
         }
         else if( key == "Desc" )
            board->desc = fread_line( stream );
         else if( key == "ObjVnum" )
            stream >> board->objvnum;
         else if( key == "Expire" )
         {
            time_t loaded_time;

            stream >> loaded_time;
            board->expire = std::chrono::system_clock::from_time_t( loaded_time );
         }
         else if( key == "Flags" )
         {
            if( file_ver < 1 )
               stream >> board->flags;
            else
            {
               std::string flags = fread_line( stream );
               flag_string_set( flags, board->flags, board_flags );
            }
         }
         else if( key == "Readers" )
            board->readers = fread_line( stream );
         else if( key == "Posters" )
            board->posters = fread_line( stream );
         else if( key == "Moderators" )
            board->moderators = fread_line( stream );
         else if( key == "Group" )
            board->group = fread_line( stream );
         else if( key == "ReadLevel" )
            stream >> board->read_level;
         else if( key == "PostLevel" )
            stream >> board->post_level;
         else if( key == "RemoveLevel" )
            stream >> board->remove_level;
         else if( key == "#NOTE-BEGIN" )
         {
            note_data *note = new note_data;
            read_note( note, file_ver, stream );
            board->nlist.push_back( note );
         }
         else
            bug( "{}: Bad section '{}' in {} - skipping.", __func__, key, board_file.string() );
      }
      stream.close();
   }

   /*
    * Run initial pruning
    */
   check_boards(  );
}

void write_reply( note_data * pnote, std::ofstream & stream )
{
   auto date_stamp = std::chrono::system_clock::to_time_t( pnote->date_stamp );

   stream << "#REPLY\n";
   stream << std::format( "Reply-Sender         {}~\n", pnote->sender );
   stream << std::format( "Reply-To             {}~\n", pnote->to_list );
   stream << std::format( "Reply-Subject        {}~\n", pnote->subject );
   stream << std::format( "Reply-DateStamp      {}\n", date_stamp );
   stream << std::format( "Reply-Flags          {}~\n", bitset_string( pnote->flags, note_flags ) );
   stream << std::format( "Reply-Text           {}~\n", pnote->text );
   stream << "#REPLY-END\n\n";
}

void write_note( note_data * pnote, std::ofstream & stream )
{
   if( !pnote )
   {
      bug( "{}: Called with a nullptr note.", __func__ );
      return;
   }

   if( pnote->sender.empty() )
   {
      bug( "{}: Called on a note without a valid sender!", __func__ );
      return;
   }

   if( pnote->type < NOTE_BOARD && pnote->type > NOTE_OLCMAP )
      bug( "{}: Invalid note type passed: %d", __func__, pnote->type );

   auto date_stamp = std::chrono::system_clock::to_time_t( pnote->date_stamp );
   auto expire = std::chrono::system_clock::to_time_t( pnote->expire );

   stream << "\n#NOTE-BEGIN\n";
   stream << std::format( "Sender         {}~\n", pnote->sender );
   if( !pnote->subject.empty() )
      stream << std::format( "Subject        {}~\n", pnote->subject );
   if( !pnote->to_list.empty() )
      stream << std::format( "To             {}~\n", pnote->to_list );
   stream << std::format( "DateStamp      {}\n", date_stamp );
   stream << std::format( "Flags          {}~\n", bitset_string( pnote->flags, note_flags ) );
   if( expire )                  // Comments and Project Logs do not use to_list or Expire
      stream << std::format( "Expire         {}\n", expire );
   if( !pnote->text.empty() )
      stream << std::format( "Text           {}~\n", pnote->text );
   stream << std::format( "Type           {}\n", pnote->type );
   if( pnote->type == NOTE_BOARD ) // Only board notes will have a reply chain.
   {
      int count = 0;

      for( auto it = pnote->rlist.begin(  ); it != pnote->rlist.end(  ); )
      {
         note_data *reply = *it;
         ++it;

         if( reply->sender.empty() || reply->to_list.empty() || reply->subject.empty() || reply->text.empty() )
         {
            log_printf( "{}: Destroying a buggy reply on note '{}'!", __func__, pnote->subject );
            --pnote->reply_count;
            deleteptr( reply );
            continue;
         }
         if( count == MAX_REPLY )
            break;
         write_reply( reply, stream );
         ++count;
      }
   }

   switch( pnote->type )
   {
      default:
      case NOTE_BOARD:
         stream << "#NOTE-END\n\n";
         break;

      case NOTE_PROJECT:
         stream << "#LOG-END\n\n";

      case NOTE_PLAYER:
         stream << "#PLR-COMMENT-END\n\n";

      case NOTE_OLCMAP: // I have no idea if this one actually saves anywhere, but it's here just in case.
         stream << "#OLCMAP-END\n\n";
   }
}

bool write_board_list( void )
{
   std::filesystem::path filename = std::format( "{}{}", BOARD_DIR, BOARD_LIST_FILE );
   std::ofstream stream( filename );
   if( !stream.is_open() )
   {
      bug( "{}: Cannot open {} for writing: {}", __func__, filename.string(), std::strerror(errno) );
      return false;
   }

   for( auto* board : bdlist )
   {
      stream << board->filename << "~\n";
   }
   stream.close();

   if( stream.fail() )
   {
      bug( "{}: Error occurred after closing {}: ", __func__, filename.string(), std::strerror(errno) );
      return false;
   }
   return true;
}

// 0: Original Smaug board format.
// 1: Initial AFKMud version. Turns out it was broken :(
// 2: The whole system was broken, so it's being rebuilt.
constexpr int BOARDFILEVER = 2;

void write_board( board_data * board )
{
   std::filesystem::path filename = std::format( "{}{}", BOARD_DIR, board->filename );
   std::ofstream stream( filename );
   if( !stream.is_open() )
   {
      bug( "{}: Cannot open {} for writing: {}", __func__, filename.string(), std::strerror(errno) );
      return;
   }

   auto expire = std::chrono::system_clock::to_time_t( board->expire );

   stream << std::format( "#VERSION    {}\n", BOARDFILEVER );
   stream << "#BOARD\n";
   stream << std::format( "Name        {}~\n", board->name );
   if( !board->desc.empty() )
      stream << std::format( "Desc        {}~\n", board->desc );
   stream << std::format( "ObjVnum     {}\n", board->objvnum );
   stream << std::format( "Expire      {}\n", expire );
   stream << std::format( "Flags       {}~\n", bitset_string( board->flags, board_flags ) );
   if( !board->readers.empty() )
      stream << std::format( "Readers     {}~\n", board->readers );
   if( !board->posters.empty() )
      stream << std::format( "Posters     {}~\n", board->posters );
   if( !board->moderators.empty() )
      stream << std::format( "Moderators  {}~\n", board->moderators );
   if( !board->group.empty() )
      stream << std::format( "Group       {}~\n", board->group );
   stream << std::format( "ReadLevel   {}\n", board->read_level );
   stream << std::format( "PostLevel   {}\n", board->post_level );
   stream << std::format( "RemoveLevel {}\n", board->remove_level );

   // Write this board's list of notes now.
   for( auto it = board->nlist.begin(  ); it != board->nlist.end(  ); )
   {
      note_data *pnote = *it;
      ++it;

      if( pnote->sender.empty() || pnote->to_list.empty() || pnote->subject.empty() || pnote->text.empty() )
      {
         log_printf( "{}: Destroying a buggy note on the {} board!", __func__, board->name );
         --board->msg_count;
         deleteptr( pnote );
         continue;
      }
      write_note( pnote, stream );
   }
   stream << "#BOARD-END\n";
   stream.close();
   if( stream.fail() )
      bug( "{}: Error occurred after closing {}: ", __func__, filename.string(), std::strerror(errno) );
}

void write_boards( void )
{
   // First make sure the board list file can be written.
   if( !write_board_list() )
   {
      log_printf( "{}: Aborting board writing due to previous error.", __func__ );
      return;
   }

   // Now go down the list of boards one by one and save them to their proper filenames.
   for( auto* board : bdlist )
      write_board( board );
}

bool can_remove( char_data * ch, board_data * board )
{
   /*
    * If there's no group on the board, or if the character is Immortal, use the following... 
    */
   if( board->group.empty() || ch->is_immortal(  ) )
   {
      if( ch->get_trust(  ) >= board->remove_level )
         return true;

      if( !board->moderators.empty() )
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

      if( !board->moderators.empty() )
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
   if( board->group.empty() || ch->is_immortal(  ) )
   {
      if( ch->get_trust(  ) >= board->post_level )
         return true;

      if( !board->posters.empty() )
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

      if( !board->posters.empty() )
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
   if( board->group.empty() || ch->is_immortal(  ) )
   {
      if( ch->get_trust(  ) >= board->read_level )
         return true;

      if( !board->readers.empty() )
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

      if( !board->readers.empty() )
      {
         if( hasname( board->readers, ch->name ) )
            return true;
      }
   }
   return false;
}

bool is_note_to( char_data * ch, note_data * pnote )
{
   if( !pnote->to_list.empty() )
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

board_data *get_board_by_name( std::string_view name )
{
   for( auto* board : bdlist )
   {
      if( !str_cmp( board->name, name ) )
         return board;
   }
   return nullptr;
}

// This will get a board by object. This will not get a global board as global boards are noted with a 0 objvnum.
board_data *get_board_by_obj( obj_data * obj )
{
   for( auto* board : bdlist )
   {
      if( board->objvnum == obj->pIndexData->vnum )
         return board;
   }
   return nullptr;
}

/*
 * Gets a board by name, or a number. The number should be the board # given in do_board_list.
 * If ch == nullptr, then it'll perform the search without checks. Otherwise, it'll perform the
 * search and weed out boards that the ch can't view remotely.
 */
board_data *get_board( char_data * ch, std::string_view name )
{
   int count = 1;

   for( auto* board : bdlist )
   {
      if( ch != nullptr )
      {
         if( !can_read( ch, board ) )
            continue;
         if( board->objvnum > 0 && !can_remove( ch, board ) && !ch->is_immortal(  ) )
            continue;
      }
      if( is_number( name ) ) // Bugfix 6/21/2026 - Passing a text string here crashes the game.
      {
         if( count == std::stoi( name.data() ) )
            return board;
      }
      if( !str_cmp( board->name, name ) )
         return board;
      ++count;
   }
   return nullptr;
}

// This will find a board on an object in the character's current room.
board_data *find_board( char_data * ch )
{
   for( auto* obj : ch->in_room->objects )
   {
      board_data *board;

      if( ( board = get_board_by_obj( obj ) ) != nullptr )
         return board;
   }
   return nullptr;
}

board_chardata *get_chboard( char_data * ch, std::string_view board_name )
{
   for( auto* board : ch->pcdata->boarddata )
   {
      if( !str_cmp( board->board_name, board_name ) )
         return board;
   }
   return nullptr;
}

board_chardata *create_chboard( char_data * ch, std::string_view board_name )
{
   board_chardata *chboard;

   if( ( chboard = get_chboard( ch, board_name ) ) )
      return chboard;

   chboard = new board_chardata;
   chboard->board_name = board_name;
   chboard->last_read = std::chrono::system_clock::time_point{};
   chboard->alert = false;
   ch->pcdata->boarddata.push_back( chboard );
   return chboard;
}

void char_data::note_attach( int type )
{
   note_data *pcnote;

   if( pcdata->pnote )
   {
      bug( "{}: ch->pcdata->pnote already exists!", __func__ );
      return;
   }

   pcnote = new note_data;
   pcnote->type = type;
   pcnote->sender = name;
   pcdata->pnote = pcnote;
}

void note_remove( board_data * board, note_data * pnote )
{
   if( !board || !pnote )
   {
      bug( "{}: null {} variable.", __func__, board ? "pnote" : "board" );
      return;
   }

   // Memory safety check in case a note being replied to by someone is being removed.
   for( auto* ch : pclist )
   {
      if( ch->pcdata->pnote )
      {
         // If a player is replying to the note we are removing, disconnect the link
         if( ch->pcdata->pnote->parent == pnote )
         {
            ch->print( "&RThe note you were replying to has been removed.&D\r\n" );
            ch->pcdata->pnote->parent = nullptr;
         }
      }
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

int unread_notes( char_data * ch, board_data * board )
{
   board_chardata *chboard;
   int count = 0;

   chboard = get_chboard( ch, board->name );

   for( auto* note : board->nlist )
   {
      if( IS_BOARD_FLAG( board, BOARD_PRIVATE ) && !is_note_to( ch, note ) )
         continue;
      if( !chboard )
         ++count;
      else if( note->date_stamp > chboard->last_read )
         ++count;
   }
   return count;
}

int total_replies( board_data * board )
{
   int count = 0;

   for( auto* note : board->nlist )
   {
      count += note->reply_count;
   }
   return count;
}

/* This is because private boards don't function right with board->msg_count...  :P */
int total_notes( char_data * ch, board_data * board )
{
   int count = 0;

   for( auto* note : board->nlist )
   {
      if( IS_BOARD_FLAG( board, BOARD_PRIVATE ) && !is_note_to( ch, note ) && !can_remove( ch, board ) )
         continue;
      else
         ++count;
   }
   return count;
}

/* Only expire root messages. Replies are pointless to expire. */
void board_check_expire( board_data * board )
{
   // This defaults everything to sticky anyway...
   if( board->expire == std::chrono::system_clock::time_point{} )
      return;

   for( auto it = board->nlist.begin(  ); it != board->nlist.end(  ); )
   {
      note_data *pnote = *it;
      ++it;

      if( IS_NOTE_FLAG( pnote, NOTE_STICKY ) )
         continue;

      // The note has no expiration date.
      if( pnote->expire == std::chrono::system_clock::time_point{} )
         continue;

      if( current_time >= pnote->expire )
      {
         log_printf( "Removing expired note '{}'", pnote->subject );
         note_remove( board, pnote );
         continue;
      }
   }
}

void check_boards( void )
{
   log_string( "Starting board pruning..." );

   for( auto* board : bdlist )
      board_check_expire( board );
}

void board_announce( char_data * ch, board_data * board, note_data * pnote )
{
   board_chardata *chboard;

   for( auto* d : dlist )
   {
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
         vch->print_fmt( "&G[&wBoard Announce&G] &w{} has posted a message for you on the {} board.\r\n", ch->name, board->name );
      else if( !IS_BOARD_FLAG( board, BOARD_PRIVATE ) )
         vch->print_fmt( "&G[&wBoard Announce&G] &w{} has posted a message on the {} board.\r\n", ch->name, board->name );
   }
}

void note_to_char( char_data * ch, note_data * pnote, board_data * board, short id )
{
   int count = 1;

   if( pnote == nullptr )
   {
      bug( "{}: null pnote!", __func__ );
      return;
   }

   if( id > 0 && board != nullptr )
      ch->print_fmt( "&[board3][&[board]Note #&[board2]{}&[board] of &[board2]{}&[board3]]&[board]           -- &[board2]{}&[board] --&D\r\n",
            id, total_notes( ch, board ), board->name );

   /*
    * Using a negative ID# for a bit of beauty -- Xorith 
    */
   if( id == -1 && !board )
      ch->print( "&[board3][&[board2]Project Log&[board3]]&D\r\n" );
   if( id == -2 && !board )
      ch->print( "&[board3][&[board2]Player Comment&[board3]]&D\r\n" );

   ch->print_fmt( "&[board]From:    &[board2]{:<15}", !pnote->sender.empty() ? pnote->sender : "--Error--" );
   if( !pnote->to_list.empty() )
      ch->print_fmt( " &[board]To:     &[board2]{:<15}", pnote->to_list );
   ch->print( "&D\r\n" );
   if( pnote->date_stamp != std::chrono::system_clock::time_point{} )
      ch->print_fmt( "&[board]Date:    &[board2]{}&D\r\n", c_time( pnote->date_stamp, ch->pcdata->timezone_name ) );

   if( board && can_remove( ch, board ) )
   {
      if( IS_NOTE_FLAG( pnote, NOTE_STICKY ) )
         ch->print( "&[board]This note is sticky and will not expire.&D\r\n" );
      else
      {
         auto duration_elapsed = current_time - pnote->date_stamp;
         auto days_elapsed = std::chrono::floor<std::chrono::days>( duration_elapsed );

         ch->print_fmt( "&[board]This note will expire in &[board2]{}&[board] day{}.&D\r\n", days_elapsed.count(), days_elapsed.count() == 1 ? "" : "s" );
      }
   }

   ch->print_fmt( "&[board]Subject: &[board2]{}&D\r\n", !pnote->subject.empty() ? pnote->subject : "" );
   ch->print( "&[board]------------------------------------------------------------------------------&D\r\n" );
   ch->print_fmt( "&[board2]{}&D\r\n", !pnote->text.empty() ? pnote->text : "--Error--" );
   if( !pnote->rlist.empty(  ) )
   {
      for( auto* reply : pnote->rlist )
      {
         ch->print( "\r\n&[board]------------------------------------------------------------------------------&D\r\n" );
         ch->print_fmt( "&[board3][&[board]Reply #&[board2]{}&[board3]] [&[board2]{}&[board3]]&D\r\n", count, c_time( reply->date_stamp, ch->pcdata->timezone_name ) );
         ch->print_fmt( "&[board]From:    &[board2]{:<15}", !reply->sender.empty() ? reply->sender : "--Error--" );
         if( !reply->to_list.empty() )
            ch->print_fmt( "   &[board]To:     &[board2]{:<15}", reply->to_list );
         ch->print( "&D\r\n&[board]------------------------------------------------------------------------------&D\r\n" );
         ch->print_fmt( "&[board2]{}&D\r\n", !reply->text.empty() ? reply->text : "--Error--" );
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
   std::string arg;

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
      ch->print_fmt( "&[board]Using current board in room: &[board2]{}&D\r\n", board->name );

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
   n_num = std::stoi( arg );

   if( n_num == 0 )
   {
      ch->print( "Modify which note?\r\n" );
      return;
   }

   i = 1;
   note_data *pnote = nullptr;

   for( auto* note : board->nlist )
   {
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
      if( std::stoi( argument ) < 0 || std::stoi( argument ) > MAX_BOARD_EXPIRE )
      {
         ch->print_fmt( "&[board]Expiration days must be a value between &[board2]0&[board] and &[board2]{}&[board].&D\r\n", MAX_BOARD_EXPIRE );
         return;
      }

      value = std::stoi( argument );
      auto expiry_time = current_time + std::chrono::days( value );
      pnote->expire = expiry_time;

      ch->print_fmt( "&[board]Set the expiration time for '&[board2]{}&[board]' to &[board2]%d&D\r\n", pnote->subject, value );
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
      pnote->sender = argument;
      ch->print_fmt( "&[board]Set the sender for '&[board2]{}&[board]' to &[board2]{}&D\r\n", pnote->subject, pnote->sender );
      write_board( board );
      return;
   }

   if( !str_cmp( arg, "flags" ) )
   {
      std::string buf;
      bool fMatch = false, fUnknown = false;

      ch->print( "&[board]Setting flags: &[board2]" );
      buf = "\r\n&[board]Unknown Flags: &[board2]";
      while( !argument.empty(  ) )
      {
         argument = one_argument( argument, arg );
         value = get_note_flag( arg );
         if( value < 0 || value >= MAX_NOTE_FLAGS )
         {
            fUnknown = true;
            buf.append( std::format( " {}", arg ) );
         }
         else
         {
            fMatch = true;
            TOGGLE_NOTE_FLAG( pnote, value );
            if( IS_NOTE_FLAG( pnote, value ) )
               ch->print_fmt( " +{}", arg );
            else
               ch->print_fmt( " -{}", arg );
         }
      }
      ch->print_fmt( "{}{}&D\r\n", fMatch ? "" : "none", fUnknown ? buf : "" );
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
      pnote->to_list = argument;
      ch->print_fmt( "&[board]Set the to_list for '&[board2]{}&[board]' to &[board2]{}&D\r\n", pnote->subject, pnote->to_list );
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
      ch->print_fmt( "&[board]Set the subject for '&[board2]{}&[board]' to &[board2]{}&D\r\n", pnote->subject, argument );
      pnote->subject = argument;
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
void board_parse( descriptor_data * d, const std::string & argument )
{
   char_data *ch;
   std::string s1, s2, s3;

   ch = d->character ? d->character : d->original;

   /*
    * Can NPCs even have substates and connected? 
    */
   if( ch->isnpc(  ) )
   {
      bug( "{}: NPC in {}!", __func__, __func__ );
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
      bug( "{}: In substate -- No pnote on character!", __func__ );
      d->connected = CON_PLAYING;
      ch->substate = SUB_NONE;
      return;
   }

   if( !ch->pcdata->board )
   {
      bug( "{}: In substate -- No board on character!", __func__ );
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

   s1 = ch->color_str( AT_BOARD );
   s2 = ch->color_str( AT_BOARD2 );
   s3 = ch->color_str( AT_BOARD3 );

   switch ( ch->substate )
   {
      default:
         ch->substate = SUB_NONE;
         d->connected = CON_PLAYING;
         break;

      case SUB_BOARD_STICKY:
         if( !str_cmp( argument, "yes" ) || !str_cmp( argument, "y" ) )
         {
            ch->pcdata->pnote->expire = std::chrono::system_clock::time_point{};
            TOGGLE_NOTE_FLAG( ch->pcdata->pnote, NOTE_STICKY );
            ch->print_fmt( "{}This note will not expire during pruning.&D\r\n", s1 );
         }
         else
         {
            time_t legacy_time = std::chrono::system_clock::to_time_t( ch->pcdata->pnote->expire );
            ch->pcdata->pnote->expire = ch->pcdata->board->expire;
            ch->print_fmt( "{}Note set to expire after the default of {}{}{} day{}.&D\r\n", s1, s2, legacy_time, s1, legacy_time == 1 ? "" : "s" );
         }

         if( ch->pcdata->pnote->parent )
            ch->print_fmt( "{}To whom is this note addressed? {}({}Default: {}{}{})&D   ", s1, s3, s1, s2, ch->pcdata->pnote->parent->sender, s3 );
         else
            ch->print_fmt( "{}To whom is this note addressed? {}({}Default: {}All{})&D   ", s1, s3, s1, s2, s3 );
         ch->substate = SUB_BOARD_TO;
         return;

      case SUB_BOARD_TO:
         if( argument.empty(  ) || !str_cmp( argument, "all" ) )
         {
            if( IS_BOARD_FLAG( ch->pcdata->board, BOARD_PRIVATE ) && !ch->is_immortal(  ) )
            {
               ch->print_fmt( "{}You must specify a recipient:&D   ", s1 );
               return;
            }
            if( ch->pcdata->pnote->parent )
               ch->pcdata->pnote->to_list = ch->pcdata->pnote->parent->sender;
            else
               ch->pcdata->pnote->to_list = "All";

            if( str_cmp( argument, "all" ) )
               ch->print_fmt( "{}No recipient specified. Defaulting to '{}{}{}'&D\r\n", s1, s2, ch->pcdata->pnote->to_list, s1 );
            else
               ch->print_fmt( "{}Recipient set to '{}All{}'&D\r\n", s1, s2, s1 );
         }
         else
         {
            std::filesystem::path buf;

            buf = std::format( "{}{}/{}", PLAYER_DIR, static_cast<char>( std::tolower( argument.front() ) ), capitalize( argument ) );
            if( !std::filesystem::exists( buf ) || !check_parse_name( argument, false ) )
            {
               ch->print_fmt( "{}No such player named '{}{}{}'.\r\nTo whom is this note addressed?   &D", s1, s2, argument, s1 );
               return;
            }
            ch->pcdata->pnote->to_list = argument;
         }

         ch->print_fmt( "{}To: {}{:<15} {}From: {}{}&D\r\n", s1, s2, ch->pcdata->pnote->to_list, s1, s2, ch->pcdata->pnote->sender );
         if( !ch->pcdata->pnote->subject.empty() )
         {
            ch->print_fmt( "{}Subject: {}{}&D\r\n", s1, s2, ch->pcdata->pnote->subject );
            ch->substate = SUB_BOARD_TEXT;
            ch->print_fmt( "{}Please enter the text for your message:&D\r\n", s1 );
            ch->set_editor_desc( std::format( "A note to {}", ch->pcdata->pnote->to_list ) );
            ch->start_editing( ch->pcdata->pnote->text );
            return;
         }
         ch->substate = SUB_BOARD_SUBJECT;
         ch->print_fmt( "{}Please enter a subject for this note:&D   ", s1 );
         return;

      case SUB_BOARD_SUBJECT:
         if( argument.empty(  ) )
         {
            ch->print_fmt( "{}No subject specified!&D\r\n{}You must specify a subject:&D  ", s3, s1 );
            return;
         }
         else
         {
            ch->pcdata->pnote->subject = argument;
         }

         ch->substate = SUB_BOARD_TEXT;
         ch->print_fmt( "{}To: {}{:<15} {}From: {}{}&D\r\n", s1, s2, ch->pcdata->pnote->to_list, s1, s2, ch->pcdata->pnote->sender );
         ch->print_fmt( "{}Subject: {}{}&D\r\n", s1, s2, ch->pcdata->pnote->subject );
         ch->print_fmt( "{}Please enter the text for your message:&D\r\n", s1 );
         ch->set_editor_desc( std::format( "A note to {}", ch->pcdata->pnote->to_list ) );
         ch->start_editing( ch->pcdata->pnote->text );
         return;

      case SUB_BOARD_CONFIRM:
         if( argument.empty(  ) )
         {
            note_to_char( ch, ch->pcdata->pnote, nullptr, 0 );
            ch->print_fmt( "{}You {}must{} confirm! Is this correct? {}({}Type: {}Y{} or {}N{})&D   ", s1, s3, s1, s3, s1, s2, s1, s2, s3 );
            return;
         }

         if( !str_cmp( argument, "y" ) || !str_cmp( argument, "yes" ) )
         {
            if( !ch->pcdata->pnote )
            {
               bug( "{}: nullptr (ch){}->pcdata->pnote!", __func__, ch->name );
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
                  ch->pcdata->pnote->parent = nullptr;
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
               log_printf( "char_to_room: {}:{}, line {}.", __FILE__, __func__, __LINE__ );

            ch->print_fmt( "{}You post the note '{}{}{}' on the {}{}{} board.&D\r\n", s1, s2, ch->pcdata->pnote->subject, s1, s2, ch->pcdata->board->name, s1 );
            act( AT_GREY, "$n posts a note on the board.", ch, nullptr, nullptr, TO_ROOM );
            board_announce( ch, ch->pcdata->board, ch->pcdata->pnote );
            ch->pcdata->pnote = nullptr;
            ch->pcdata->board = nullptr;
            ch->substate = SUB_NONE;
            d->connected = CON_PLAYING;
            return;
         }

         if( !str_cmp( argument, "n" ) || !str_cmp( argument, "no" ) )
         {
            ch->substate = SUB_BOARD_REDO_MENU;
            ch->print_fmt( "{}What would you like to change?&D\r\n", s1 );
            ch->print_fmt( "   {}1{}) {}Recipients  {}[{}{}{}]&D\r\n", s2, s3, s1, s3, s2, ch->pcdata->pnote->to_list, s3 );
            if( ch->pcdata->pnote->parent )
               ch->print_fmt( "   {}2{}) {}You cannot change subjects on a reply.&D\r\n", s2, s3, s1 );
            else
               ch->print_fmt( "   {}2{}) {}Subject     {}[{}{}{}]&D\r\n", s2, s3, s1, s3, s2, ch->pcdata->pnote->subject, s3 );
            ch->print_fmt( "   {}3{}) {}Text\r\n{}{}&D\r\n", s2, s3, s1, s2, ch->pcdata->pnote->text );
            ch->print_fmt( "{}Please choose an option, {}Q{} to quit this menu, or {}/a{} to abort:&D   ", s1, s2, s1, s2, s1 );
            return;
         }
         ch->print_fmt( "{}Please enter either {}Y{} or {}N{}:&D   ", s1, s2, s1, s2, s1 );
         return;

      case SUB_BOARD_REDO_MENU:
         if( argument.empty(  ) )
         {
            ch->print_fmt( "{}Please choose an option, {}Q{} to quit this menu, or {}/a{} to abort:&D   ", s1, s2, s1, s2, s1 );
            return;
         }

         if( !str_cmp( argument, "1" ) )
         {
            if( ch->pcdata->pnote->parent )
               ch->print_fmt( "{}To whom is this note addressed? {}({}Default: {}{}{})&D   ", s1, s3, s1, s2, ch->pcdata->pnote->parent->sender, s3 );
            else
               ch->print_fmt( "{}To whom is this note addressed? {}({}Default: {}All{})&D   ", s1, s3, s1, s2, s3 );
            ch->substate = SUB_BOARD_TO;
            return;
         }

         if( !str_cmp( argument, "2" ) )
         {
            if( ch->pcdata->pnote->parent )
            {
               ch->print_fmt( "{}You cannot change the subject of a reply!&D\r\n", s1 );
               ch->print_fmt( "{}Please choose an option, {}Q{} to quit this menu, or {}/a{} to abort:&D   ", s1, s2, s1, s2, s1 );
               return;
            }
            ch->substate = SUB_BOARD_SUBJECT;
            ch->print_fmt( "{}Please enter a subject for this note:&D   ", s1 );
            return;
         }

         if( !str_cmp( argument, "3" ) )
         {
            ch->substate = SUB_BOARD_TEXT;
            ch->print_fmt( "{}Please enter the text for your message:&D\r\n", s1 );
            ch->set_editor_desc( std::format( "A note to {} about {}", ch->pcdata->pnote->to_list, ch->pcdata->pnote->subject ) );
            ch->start_editing( ch->pcdata->pnote->text );
            return;
         }

         if( !str_cmp( argument, "q" ) )
         {
            ch->substate = SUB_BOARD_CONFIRM;
            note_to_char( ch, ch->pcdata->pnote, nullptr, 0 );
            ch->print_fmt( "{}Is this correct? {}({}Y{}/{}N{})&D   ", s1, s3, s2, s3, s2, s3 );
            return;
         }
         ch->print_fmt( "{}Please choose an option, {}Q{} to quit this menu, or {}/a{} to abort:&D   ", s1, s2, s1, s2, s1 );
         return;
   }
}

// This is like a stripped down nset, intended for anyone with moderator access -X
// Added: 9-23-07
// Command: bmod
// Level: 10
CMDF( do_board_moderate )
{
   board_data *board;
   short n_num = 0, i = 1;
   std::string arg;

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
      ch->print_fmt( "&[board]Using current board in room: &[board2]{}&D\r\n", board->name );

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
   n_num = std::stoi( arg );

   if( n_num == 0 )
   {
      ch->print( "Modify which note?\r\n" );
      return;
   }

   i = 1;
   note_data *pnote = nullptr;

   for( auto* note: board->nlist )
   {
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
         ch->print_fmt( "&[board]The note '&[board2]{}&[board]' is now closed.", pnote->subject );
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
         ch->print_fmt( "&[board]The note '&[board2]{}&[board]' is now open.", pnote->subject );
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
         ch->print_fmt( "&[board]The note '&[board2]{}&[board]' is now hidden.", pnote->subject );
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
         ch->print_fmt( "&[board]The note '&[board2]{}&[board]' is now visible.", pnote->subject );
      }
      return;
   }

   if( !str_cmp( arg, "sticky" ) )
   {
      if( IS_NOTE_FLAG( pnote, NOTE_STICKY ) || board->expire == std::chrono::system_clock::time_point{} )
         ch->print( "&[board]That note is already sticky.&D\r\n" );
      else
      {
         TOGGLE_NOTE_FLAG( pnote, NOTE_STICKY );
         ch->print_fmt( "&[board]The note '&[board2]{}&[board]' is now sticky.", pnote->subject );
      }
      return;
   }

   if( !str_cmp( arg, "unsticky" ) )
   {
      if( board->expire == std::chrono::system_clock::time_point{} )
      {
         ch->print_fmt( "&[board]This note cannot be made unsticky, as &[board2]{}&[board] is set to never expire notes.&D\r\n", board->name );
         return;
      }

      if( !IS_NOTE_FLAG( pnote, NOTE_STICKY ) )
         ch->print( "&[board]That note is already unsticky.&D\r\n" );
      else
      {
         TOGGLE_NOTE_FLAG( pnote, NOTE_STICKY );
         ch->print_fmt( "&[board]The note '&[board2]{}&[board]' is now unsticky.", pnote->subject );
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
   board_data *board = nullptr;
   room_index *board_room;
   std::string arg, buf, s1, s2, s3;
   int n_num = 0;

   if( ch->isnpc(  ) )
      return;

   s1 = ch->color_str( AT_BOARD );
   s2 = ch->color_str( AT_BOARD2 );
   s3 = ch->color_str( AT_BOARD3 );

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
         ch->pcdata->pnote = nullptr;
         ch->from_room(  );
         if( !ch->to_room( ch->orig_room ) )
            log_printf( "char_to_room: {}:{}, line {}.", __FILE__, __func__, __LINE__ );
         ch->desc->connected = CON_PLAYING;
         act( AT_GREY, "$n aborts $s note writing.", ch, nullptr, nullptr, TO_ROOM );
         ch->pcdata->board = nullptr;
         return;

      case SUB_BOARD_TEXT:
         if( ch->pcdata->pnote == nullptr )
         {
            bug( "{}: SUB_BOARD_TEXT: Null pnote on character ({})!", __func__, ch->name );
            ch->stop_editing(  );
            return;
         }
         if( ch->pcdata->board == nullptr )
         {
            bug( "{}: SUB_BOARD_TEXT: Null board on character ({})!", __func__, ch->name );
            ch->stop_editing(  );
            return;
         }
         ch->pcdata->pnote->text = ch->copy_buffer( );
         ch->stop_editing(  );
         if( ch->pcdata->pnote->text.empty() )
         {
            ch->print( "You must enter some text in the body of the note!\r\n" );
            ch->substate = SUB_BOARD_TEXT;
            ch->set_editor_desc( std::format( "A note to {} about {}", ch->pcdata->pnote->to_list, ch->pcdata->pnote->subject ) );
            ch->start_editing( ch->pcdata->pnote->text );
            return;
         }
         note_to_char( ch, ch->pcdata->pnote, nullptr, 0 );
         ch->print_fmt( "{}Is this correct? {}({}Y{}/{}N{})&D   ", s1, s3, s2, s3, s2, s3 );
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
         ch->print_fmt( "{}Writes a new message for a board.\r\n{}Syntax: {}write {}<{}board{}> [{}subject{}/{}note#{}]&D\r\n", s1, s3, s1, s3, s2, s3, s2, s3, s2, s3 );
         ch->print_fmt( "{}Note: Subject and Note# are optional, but you can not specify both.&D\r\n", s1 );
         return;
      }

      argument = one_argument( argument, arg );
      if( !( board = get_board( ch, arg ) ) )
      {
         ch->print_fmt( "{}No board found!&D\r\n", s1 );
         return;
      }
   }
   else
      ch->print_fmt( "{}Using current board in room: {}{}&D\r\n", s1, s2, board->name );

   if( !can_post( ch, board ) )
   {
      ch->print( "You can only read these notes.\r\n" );
      return;
   }

   if( is_number( argument ) )
      n_num = std::stoi( argument );

   ch->pcdata->board = board;
   buf = std::format( "{}", ROOM_VNUM_LIMBO );
   board_room = ch->find_location( buf );
   if( !board_room )
   {
      bug( "{}: Missing board room: Vnum {}", __func__, ROOM_VNUM_LIMBO );
      return;
   }
   ch->print_fmt( "{}Typing '{}/a{}' at any time will abort the note.&D\r\n", s3, s2, s3 );

   if( n_num )
   {
      note_data *pnote = nullptr;
      int i = 1;
      for( auto* note : board->nlist )
      {
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

      ch->print_fmt( "{}You begin to write a reply for {}{}'s{} note '{}{}{}'.&D\r\n", s1, s2, pnote->sender, s1, s2, pnote->subject, s1 );
      act( AT_GREY, "$n departs for a moment, replying to a note.", ch, nullptr, nullptr, TO_ROOM );
      ch->note_attach( NOTE_BOARD );
      if( IS_BOARD_FLAG( board, BOARD_PRIVATE ) )
      {
         ch->pcdata->pnote->to_list = pnote->sender;
      }
      ch->pcdata->pnote->parent = pnote;
      ch->pcdata->pnote->subject = std::format( "Re: {}", ch->pcdata->pnote->parent->subject );
      ch->desc->connected = CON_BOARD;
      if( IS_BOARD_FLAG( board, BOARD_PRIVATE ) )
      {
         ch->substate = SUB_BOARD_TEXT;
         ch->print_fmt( "{}To: {}{:<15} {}From: {}{}&D\r\n", s1, s2, ch->pcdata->pnote->to_list, s1, s2, ch->pcdata->pnote->sender );
         ch->print_fmt( "{}Subject: {}{}&D\r\n", s1, s2, ch->pcdata->pnote->subject );
         ch->print_fmt( "{}Please enter the text for your message:&D\r\n", s1 );
         ch->set_editor_desc( std::format( "A note to {} about {}", ch->pcdata->pnote->to_list, ch->pcdata->pnote->subject ) );
         ch->start_editing( ch->pcdata->pnote->text );
      }
      else
      {
         ch->print_fmt( "{}To whom is this note addressed? {}({}Default: {}{}{})&D   ", s1, s3, s1, s2, pnote->sender, s3 );
         ch->substate = SUB_BOARD_TO;
      }
      ch->orig_room = ch->in_room;
      ch->from_room(  );
      if( !ch->to_room( board_room ) )
         log_printf( "char_to_room: {}:{}, line {}.", __FILE__, __func__, __LINE__ );
      return;
   }

   ch->note_attach( NOTE_BOARD );
   if( !argument.empty(  ) )
   {
      ch->pcdata->pnote->subject = argument;
      ch->print_fmt( "{}You begin to write a new note for the {}{}{} board, titled '{}{}{}'.&D\r\n", s1, s2, board->name, s1, s2, ch->pcdata->pnote->subject, s1 );
   }
   else
      ch->print_fmt( "{}You begin to write a new note for the {}{}{} board.&D\r\n", s1, s2, board->name, s1 );

   if( can_remove( ch, board ) && !IS_BOARD_FLAG( board, BOARD_PRIVATE ) && ch->pcdata->board->expire > std::chrono::system_clock::time_point{} )
   {
      ch->print_fmt( "{}Is this a sticky note? {}({}Y{}/{}N{})  ({}Default: {}N{})&D   ", s1, s3, s2, s3, s2, s3, s1, s2, s3 );
      ch->substate = SUB_BOARD_STICKY;
   }
   else
   {
      if( ch->pcdata->board->expire == std::chrono::system_clock::time_point{} )
         TOGGLE_NOTE_FLAG( ch->pcdata->pnote, NOTE_STICKY );
      else
         ch->pcdata->pnote->expire = ch->pcdata->board->expire;
      ch->substate = SUB_BOARD_TO;
      if( IS_BOARD_FLAG( board, BOARD_PRIVATE ) && !can_remove( ch, board ) )
         ch->print_fmt( "{}To whom is this note addressed?&D   ", s1 );
      else
         ch->print_fmt( "{}To whom is this note addressed? {}({}Default: {}All{})&D   ", s1, s3, s1, s2, s3 );
   }
   act( AT_GREY, "$n begins to write a new note.", ch, nullptr, nullptr, TO_ROOM );
   ch->desc->connected = CON_BOARD;
   ch->orig_room = ch->in_room;
   ch->from_room(  );
   if( !ch->to_room( board_room ) )
      log_printf( "char_to_room: {}:{}, line {}.", __FILE__, __func__, __LINE__ );
}

CMDF( do_note_read )
{
   board_data *board = nullptr;
   note_data *pnote = nullptr;
   board_chardata *pboard = nullptr;
   int n_num = 0, i = 1;
   std::string arg, s1, s2, s3;

   if( ch->isnpc(  ) )
      return;

   s1 = ch->color_str( AT_BOARD );
   s2 = ch->color_str( AT_BOARD2 );
   s3 = ch->color_str( AT_BOARD3 );

   // Now specifying "any" will get around local-board checks. -X (3-23-05)
   if( argument.empty(  ) || !str_cmp( argument, "any" ) )
   {
      if( !( board = find_board( ch ) ) )
      {
         board = ch->pcdata->board;
         bool has_unread = false;
         if( !board )
         {
            std::list<board_data *>::iterator bd;
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
               {
                  has_unread = true;
                  break;
               }
            }

            if( !has_unread )
            {
               ch->print_fmt( "{}There are no boards with unread messages&D\r\n", s1 );
               return;
            }
         }
      }

      pboard = create_chboard( ch, board->name );

      for( auto* note : board->nlist )
      {
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
         ch->print_fmt( "{}There are no more unread messages on this board.&D\r\n", s1 );
         ch->pcdata->board = nullptr;
         return;
      }

      if( IS_NOTE_FLAG( pnote, NOTE_HIDDEN ) && !can_remove( ch, board ) )
      {
         ch->print( "&[board]Sorry, that note has been hidden by the moderators.&D\r\n" );
         return;
      }

      note_to_char( ch, pnote, board, n_num );
      pboard->last_read = pnote->date_stamp;
      act( AT_GREY, "$n reads a note.", ch, nullptr, nullptr, TO_ROOM );
      return;
   }

   if( !( board = find_board( ch ) ) )
   {
      argument = one_argument( argument, arg );
      if( !( board = get_board( ch, arg ) ) )
      {
         ch->print_fmt( "{}No board found!&D\r\n", s1 );
         return;
      }
   }
   else
      ch->print_fmt( "{}Using current board in room: {}{}&D\r\n", s1, s2, board->name );

   if( !can_read( ch, board ) )
   {
      ch->print_fmt( "{}You are unable to comprehend the notes on this board.&D\r\n", s1 );
      return;
   }

   n_num = std::stoi( argument );

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
   for( auto* note : board->nlist )
   {
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
      ch->print_fmt( "{}Note #{}{}{} not found on the {}{}{} board.&D\r\n", s1, s2, n_num, s1, s2, board->name, s1 );
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
   act( AT_GREY, "$n reads a note.", ch, nullptr, nullptr, TO_ROOM );
}

CMDF( do_note_list )
{
   board_data *board = nullptr;
   board_chardata *chboard;
   int count = 0;
   std::string buf, s1, s2, s3;

   s1 = ch->color_str( AT_BOARD );
   s2 = ch->color_str( AT_BOARD2 );
   s3 = ch->color_str( AT_BOARD3 );

   if( !( board = find_board( ch ) ) )
   {
      if( argument.empty(  ) )
      {
         ch->print_fmt( "{}Lists the note on a board.\r\n{}Syntax: {}review {}<{}board{}>&D\r\n", s1, s3, s1, s3, s2, s3 );
         return;
      }

      if( !( board = get_board( ch, argument ) ) )
      {
         ch->print_fmt( "{}No board found!&D\r\n", s1 );
         return;
      }
   }
   else
      ch->print_fmt( "{}Using current board in room: {}{}&D\r\n", s1, s2, board->name );

   if( !can_read( ch, board ) )
   {
      ch->print( "You are unable to comprehend the notes on this board.\r\n" );
      return;
   }

   chboard = get_chboard( ch, board->name );

   buf = std::format( "{}--[ {}Notes on {}{}{} ]--", s3, s1, s2, board->name, s3 );
   ch->print_fmt( "\r\n{}\r\n", color_align( buf, 80, ALIGN_CENTER ) );
   act_printf( AT_GREY, ch, nullptr, nullptr, TO_ROOM, "&w$n reviews the notes on the &W{}&w board.", board->name );

   if( total_notes( ch, board ) == 0 )
   {
      ch->print( "\r\nNo messages...\r\n" );
      return;
   }
   else
      ch->print_fmt( "{}Num   {}{:<17} {:<11} {}&D\r\n", s1, IS_BOARD_FLAG( board, BOARD_PRIVATE ) ? "" : "Replies ", "Date", "Author", "Subject" );

   count = 0;
   for( auto* note : board->nlist )
   {
      std::string unread;

      if( IS_BOARD_FLAG( board, BOARD_PRIVATE ) && !is_note_to( ch, note ) && !can_remove( ch, board ) )
         continue;

      ++count;

      unread = " ";
      if( !chboard || chboard->last_read < note->date_stamp )
         unread.append( "&C*" );

      if( IS_NOTE_FLAG( note, NOTE_HIDDEN ) && !can_remove( ch, board ) )
         continue;

      if( IS_NOTE_FLAG( note, NOTE_CLOSED ) )
         unread.append( "&Y-" );

      if( IS_NOTE_FLAG( note, NOTE_HIDDEN ) )
         unread.append( "&R#" );

      if( IS_BOARD_FLAG( board, BOARD_PRIVATE ) )
      {
         ch->print_fmt( "{}{:2}{}) {} {}[{}{:<15}{}] {}{:<11} {}&D\r\n", s2, count, s3, unread, s3, s2,
                     mini_c_time( note->date_stamp, ch->pcdata->timezone_name ), s3, s2,
                     !note->sender.empty() ? note->sender : "--Error--", !note->subject.empty() ? print_lngstr( note->subject, 37 ) : "" );
      }
      else
      {
         ch->print_fmt( "{}{:2}{}) {} {}[ {}{:3}{} ] [{}{:<15}{}] {}{:<11} {:<20}&D\r\n", s2, count, s3, unread, s3, s2,
                     note->reply_count, s3, s2, mini_c_time( note->date_stamp, ch->pcdata->timezone_name ), s3, s2,
                     !note->sender.empty() ? note->sender : "--Error--", !note->subject.empty() ? print_lngstr( note->subject, 45 ) : "" );
      }
   }
   ch->print_fmt( "\r\n{}There {} {}{}{} message{} on this board.&D\r\n", s1, count == 1 ? "is" : "are", s2, count, s1, count == 1 ? "" : "s" );
   ch->print_fmt( "{}A &C*{} denotes unread messages, while &Y-&[board] indicates a closed note.&D\r\n", s1, s1 );
   if( can_remove( ch, board ) )
      ch->print( "&[board]A &R#&[board] denotes a hidden message.&D\r\n" );
}

CMDF( do_note_remove )
{
   board_data *board;
   note_data *pnote = nullptr;
   std::string arg;
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
      ch->print_fmt( "&[board]Using current board in room: &[board2]{}&D\r\n", board->name );

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

   std::string::size_type pos = argument.find( "." );
   if( pos == std::string::npos )
   {
      n_num = std::stoi( argument );
      r_num = -1;
   }
   else
   {
      n_num = std::stoi( argument.substr( 0, pos - 1 ) );
      r_num = std::stoi( argument.substr( pos + 1, argument.length(  ) ) );
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
   for( auto* note : board->nlist )
   {
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
      note_data *reply = nullptr;

      i = 1;
      for( auto* rpy : pnote->rlist )
      {
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

      ch->print_fmt( "&[board]You remove the reply from &[board2]{}&[board], titled '&[board2]{}&[board]' from the &[board2]{}&[board] board.&D\r\n",
                  !reply->sender.empty() ? reply->sender : "--Error--", !reply->subject.empty() ? reply->subject : "--Error--", board->name );
      note_remove( board, reply );
      act( AT_BOARD, "$n removes a reply from the board.", ch, nullptr, nullptr, TO_ROOM );
      return;
   }

   ch->print_fmt( "&[board]You remove the note from &[board2]{}&[board], titled '&[board2]{}&[board]' from the &[board2]{}&[board] board.&D\r\n",
               !pnote->sender.empty() ? pnote->sender : "--Error--", !pnote->subject.empty() ? pnote->subject : "--Error--", board->name );
   note_remove( board, pnote );
   act( AT_BOARD, "$n removes a note from the board.", ch, nullptr, nullptr, TO_ROOM );
}

CMDF( do_board_list )
{
   obj_data *obj;
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

   for( auto* board : bdlist )
   {
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
         ch->print_fmt( "&[board2]{:2}&[board3]) &[board2]{:<25} ", count + 1, print_lngstr( board->name, 25 ) );
         ch->print_fmt( "{:<15} ", print_lngstr( board->filename, 15 ) );
         if( IS_BOARD_FLAG( board, BOARD_PRIVATE ) )
            ch->print( "P" );
         else
            ch->print( "-" );

         if( IS_BOARD_FLAG( board, BOARD_ANNOUNCE ) )
            ch->print( "A" );
         else
            ch->print( "-" );

         ch->print( "--- " );
         if( board->objvnum )
         {
            std::string buf;

            ch->print_fmt( "{:<5} ", board->objvnum );
            buf = std::format( "{}", board->objvnum );
            if( ( obj = ch->get_obj_world( buf ) ) && ( obj->in_room != nullptr ) )
               ch->print_fmt( "{:<5} ", obj->in_room->vnum );
            else
               ch->print( "----- " );
         }
         else
            ch->print( " N/A   N/A  " );
         ch->print_fmt( "{:<3}  {:<3}  {:<3}", board->read_level, board->post_level, board->remove_level );
      }
      else
      {
         ch->print_fmt( "&[board2]{:2}&[board3])  &[board2]{:<15}  &[board3][ &[board2]{:3}&[board3]]  ", count + 1, print_lngstr( board->name, 15 ), unread_notes( ch, board ) );
         if( IS_BOARD_FLAG( board, BOARD_PRIVATE ) )
            ch->print_fmt( "[&[board2]{:3}&[board3]]          ", total_notes( ch, board ) );
         else
            ch->print_fmt( "[&[board2]{:3}&[board3]]  [&[board2]{:3}&[board3]]   ", total_notes( ch, board ), total_replies( board ) );

         ch->print_fmt( "&[board2]{:<25}", !board->desc.empty() ? print_lngstr( board->desc, 25 ) : "" );

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
      ch->print_fmt( "\r\n&[board2]{}&[board] board{} found.&D\r\n", count, count == 1 ? "" : "s" );
      if( ch->is_immortal(  ) && str_cmp( argument, "info" ) )
         ch->print( "&[board3](&[board]Use &[board2]BOARDS INFO&[board] to display Immortal information.&[board3])\r\n" );
      else if( !str_cmp( argument, "info" ) && ch->is_immortal(  ) )
         ch->print( "&[board3](&[board]Use &[board2]BOARDS FLAGHELP&[board] for information on the flag letters.&[board3])\r\n" );
   }
}

CMDF( do_board_alert )
{
   board_data *board = nullptr;
   board_chardata *chboard;
   std::string arg, s1, s2, s3;
   int bd_value = -1;
   static const char* bd_alert_string[] = { "None Set", "Announce", "Ignoring" };

   s1 = ch->color_str( AT_BOARD );
   s2 = ch->color_str( AT_BOARD2 );
   s3 = ch->color_str( AT_BOARD3 );

   if( argument.empty(  ) )
   {
      // Fixed crash bug here - X (3-23-05)
      for( auto* pboard : bdlist )
      {
         if( !can_read( ch, pboard ) )
            continue;
         chboard = get_chboard( ch, pboard->name );
         ch->print_fmt( "{}{:<20}   {}Alert: {}{}&D\r\n", s2, pboard->name, s1, s2, chboard ? bd_alert_string[chboard->alert] : bd_alert_string[0] );
      }
      ch->print_fmt( "{}To change an alert for a board, type: {}alert <board> <none|announce|ignore>&D\r\n", s1, s2 );
      return;
   }

   argument = one_argument( argument, arg );
   if( !( board = get_board( ch, arg ) ) )
   {
      ch->print_fmt( "{}Sorry, but the board '{}{}{}' does not exsist.&D\r\n", s1, s2, arg, s1 );
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
      ch->print_fmt( "{}Sorry, but '{}{}{}' is not a valid argument.&D\r\n", s1, s2, argument, s1 );
      ch->print_fmt( "{}Please choose one of: {}none announce ignore&D\r\n", s1, s2 );
      return;
   }

   chboard = create_chboard( ch, board->name );
   chboard->alert = bd_value;
   ch->print_fmt( "{}Alert for the {}{}{} board set to {}{}{}.&D\r\n", s1, s2, board->name, s1, s2, argument, s1 );
}

/* Much like do_board_list, but I cut out some of the details here for simplicity */
CMDF( do_checkboards )
{
   board_chardata *chboard;
   obj_data *obj;
   int count = 0;
   std::string s1, s2, s3;

   s1 = ch->color_str( AT_BOARD );
   s2 = ch->color_str( AT_BOARD2 );
   s3 = ch->color_str( AT_BOARD3 );

   ch->print_fmt( "{} Num  {:<20}  Unread  {:<40}&D\r\n", s1, "Name", "Description" );

   for( auto* board : bdlist )
   {
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
      ch->print_fmt( "{}{:3}{})  {}{:<20}  {}[{}{:3}{}]  {}{:<30}", s2, count, s3, s2, board->name, s3, s2, unread_notes( ch, board ), s3, s2, !board->desc.empty() ? board->desc : "" );

      if( ch->is_immortal(  ) )
      {
         std::string buf;

         buf = std::format( "{}", board->objvnum );
         if( ( obj = ch->get_obj_world( buf ) ) && ( obj->in_room != nullptr ) )
            ch->print_fmt( " {}[{}Obj# {}{:<5} {}@ {}Room# {}{:<5}{}]", s3, s1, s2, board->objvnum, s3, s1, s2, obj->in_room->vnum, s3 );
         else
            ch->print_fmt( " {}[ {}Global Board {}]", s3, s2, s3 );
      }
      ch->print( "&D\r\n" );
   }
   if( count == 0 )
      ch->print_fmt( "{}No unread messages...\r\n", s1 );
}

CMDF( do_board_stat )
{
   board_data *board;
   std::string s1, s2, s3;

   s1 = ch->color_str( AT_BOARD );
   s2 = ch->color_str( AT_BOARD2 );
   s3 = ch->color_str( AT_BOARD3 );

   if( argument.empty(  ) )
   {
      ch->print( "Usage: bstat <board>\r\n" );
      return;
   }

   if( !( board = get_board( ch, argument ) ) )
   {
      ch->print_fmt( "&wSorry, '&W{}&w' is not a valid board.\r\n", argument );
      return;
   }

   ch->print_fmt( "{}Name:       {}{:<30}{} ObjVnum:      ", s1, s2, board->name, s1 );
   ch->print_fmt( "{}Filename: {}{:<20}&D\r\n", s1, s2, board->filename );
   if( board->objvnum > 0 )
      ch->print_fmt( "{}{}&D\r\n", s2, board->objvnum );
   else
      ch->print_fmt( "{}Global Board&D\r\n", s2 );
   ch->print_fmt( "{}Readers:    {}{:<30}{} Read Level:   {}{}&D\r\n", s1, s2, !board->readers.empty() ? board->readers : "none set", s1, s2, board->read_level );
   ch->print_fmt( "{}Posters:    {}{:<30}{} Post Level:   {}{}&D\r\n", s1, s2, !board->posters.empty() ? board->posters : "none set", s1, s2, board->post_level );
   ch->print_fmt( "{}Moderators: {}{:<30}{} Remove Level: {}{}&D\r\n", s1, s2, !board->moderators.empty() ? board->moderators : "none set", s1, s2, board->remove_level );
   ch->print_fmt( "{}Group:      {}{:<30}{} Expiration:   {}{}&D\r\n", s1, s2, !board->group.empty() ? board->group : "none set", s1, s2, mini_c_time( board->expire, ch->pcdata->timezone_name ) );
   ch->print_fmt( "{}Flags: {}[{}{}{}]&D\r\n", s1, s3, s2, board->flags.any(  )? bitset_string( board->flags, board_flags ) : "none set", s3 );
   ch->print_fmt( "{}Description: {}{:<30}&D\r\n", s1, s2, !board->desc.empty() ? board->desc : "none set" );
}

CMDF( do_board_remove )
{
   board_data *board;
   std::string arg, s1, s2, s3;

   s1 = ch->color_str( AT_BOARD );
   s2 = ch->color_str( AT_BOARD2 );
   s3 = ch->color_str( AT_BOARD3 );

   if( argument.empty(  ) )
   {
      ch->print_fmt( "{}You must select a board to remove.&D\r\n", s1 );
      return;
   }

   argument = one_argument( argument, arg );
   if( !( board = get_board( ch, arg ) ) )
   {
      ch->print_fmt( "{}Sorry, '{}{}{}' is not a valid board.&D\r\n", s1, s2, argument, s1 );
      return;
   }

   if( !str_cmp( argument, "yes" ) )
   {
      std::filesystem::path buf;

      ch->print_fmt( "&RDeleting board '&W{}&R'.&D\r\n", board->name );
      ch->print( "&wDeleting note file...   " );
      buf = std::format( "{}{}", BOARD_DIR, board->filename );
      std::filesystem::remove( buf );
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
      ch->print_fmt( "&RRemoving a board will also delete *ALL* posts on that board!\r\n&wTo continue, type: " "&YDELETEBOARD '{}' YES&w.", board->name );
      return;
   }
}

CMDF( do_board_make )
{
   if( argument.empty(  ) )
   {
      ch->print( "Usage: makeboard <name>\r\n" );
      return;
   }

   if( argument.length(  ) > 20 )
   {
      ch->print( "Please limit board names to 20 characters!\r\n" );
      return;
   }

   if( has_illegal_file_chars( argument, false ) )
   {
      ch->print( "A board name may not contain a '.', '/', or '\\' in it.\r\n" );
      return;
   }

   if( get_board_by_name( argument ) != nullptr )
   {
      ch->print_fmt( "A board named {} already exists.", argument );
      return;
   }

   smash_tilde( argument );
   board_data *board = new board_data;

   board->name = argument;
   board->filename = argument + ".board";
   board->desc = "This is a new board!";
   board->objvnum = 0;

   auto expiry_time = current_time + std::chrono::days( MAX_BOARD_EXPIRE );
   board->expire = expiry_time;

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
   std::string arg1, arg2, arg3, s1, s2, s3;
   int value = -1;

   s1 = ch->color_str( AT_BOARD );
   s2 = ch->color_str( AT_BOARD2 );
   s3 = ch->color_str( AT_BOARD3 );

   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );

   // Added missing 'expireall', 'raise' and 'lower'. Also added the available flags. --Xorith (9/21/07)
   if( arg1.empty(  ) || ( str_cmp( arg1, "purge" ) && arg2.empty(  ) ) )
   {
      ch->print( "Usage: bset <board> <field> value\r\n" );
      ch->print( "   Or: bset purge (this option forces a check on expired notes)\r\n" );
      ch->print( "\r\nField being one of:\r\n" );
      ch->print( "  objvnum read_level post_level remove_level expire group flags name\r\n" );
      ch->print( "  readers posters moderators desc raise lower global expireall\r\n" );
      ch->print( "\r\nFlags: \r\n private announce\r\n" );
      return;
   }

   if( is_number( argument ) )
      value = std::stoi( argument );

   if( !str_cmp( arg1, "purge" ) )
   {
      log_printf( "Manual board pruning started by {}.", ch->name );
      check_boards(  );
      return;
   }

   if( !( board = get_board( ch, arg1 ) ) )
   {
      ch->print_fmt( "{}Sorry, but '{}{}{}' is not a valid board.&D\r\n", s1, s2, arg1, s1 );
      return;
   }

   if( !str_cmp( arg2, "flags" ) )
   {
      std::string buf;

      bool fMatch = false, fUnknown = false;

      ch->print_fmt( "{}Setting flags: {}", s1, s2 );
      buf = std::format( "\r\n{}Unknown flags: {}", s1, s2 );
      while( !argument.empty(  ) )
      {
         argument = one_argument( argument, arg3 );
         value = get_board_flag( arg3 );
         if( value < 0 || value >= MAX_BOARD_FLAGS )
         {
            fUnknown = true;
            buf.append( std::format( " {}", arg3 ) );
         }
         else
         {
            fMatch = true;
            TOGGLE_BOARD_FLAG( board, value );
            if( IS_BOARD_FLAG( board, value ) )
               ch->print_fmt( " +{}", arg3 );
            else
               ch->print_fmt( " -{}", arg3 );
         }
      }
      ch->print_fmt( "{}{}&D\r\n", fMatch ? "" : "none", fUnknown ? buf : "" );
      write_board( board );
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
      write_board( board );
      ch->print_fmt( "{}The objvnum for '{}{}{}' has been set to '{}{}{}'.&D\r\n", s1, s2, board->name, s1, s2, board->objvnum, s1 );
      return;
   }

   if( !str_cmp( arg2, "global" ) )
   {
      board->objvnum = 0;
      write_board( board );
      ch->print_fmt( "{}{}{} is now a global board.\r\n", s2, board->name, s1 );
      return;
   }

   if( !str_cmp( arg2, "read_level" ) || !str_cmp( arg2, "post_level" ) || !str_cmp( arg2, "remove_level" ) )
   {
      if( value < 0 || value > ch->level )
      {
         ch->print_fmt( "{}{}{} is out of range.\r\nValues range from {}1{} to {}{}{}.&D\r\n", s2, value, s1, s2, s1, s2, ch->level, s1 );
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
         ch->print_fmt( "{}Unknown field '{}{}{}'.&D\r\n", s1, s2, arg2, s1 );
         return;
      }
      write_board( board );
      ch->print_fmt( "{}The {}{}{} for '{}{}{}' has been set to '{}{}{}'.&D\r\n", s1, s3, arg2, s1, s2, board->name, s1, s2, value, s1 );
      return;
   }

   if( !str_cmp( arg2, "readers" ) || !str_cmp( arg2, "posters" ) || !str_cmp( arg2, "moderators" ) )
   {
      if( !str_cmp( arg2, "readers" ) )
      {
         if( !argument.empty(  ) )
            board->readers = argument;
      }
      else if( !str_cmp( arg2, "posters" ) )
      {
         if( !argument.empty(  ) )
            board->posters = argument;
      }
      else if( !str_cmp( arg2, "moderators" ) )
      {
         if( !argument.empty(  ) )
            board->moderators = argument;
      }
      else
      {
         ch->print_fmt( "{}Unknown field '{}{}{}'.&D\r\n", s1, s2, arg2, s1 );
         return;
      }
      write_board( board );
      ch->print_fmt( "{}The {}{}{} for '{}{}{}' have been set to '{}{}{}'.\r\n", s1, s3, arg2, s1, s2, board->name, s1, s2, !argument.empty(  ) ? argument : "(nothing)", s1 );
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

      ch->print_fmt( "{}The name for '{}{}{}' has been changed to '{}{}{}'.\r\n", s1, s2, board->name, s1, s2, argument, s1 );

      // Need to remove the old board file here since we are setting the individual board filenames automatically now.
      std::filesystem::path filename = std::format( "{}{}.board", BOARD_DIR, board->filename );
      std::filesystem::remove( filename );
      board->name = argument;
      board->filename = argument + ".board";

      write_board( board );
      return;
   }

   if( !str_cmp( arg2, "expire" ) )
   {
      if( value < 0 || value > MAX_BOARD_EXPIRE )
      {
         ch->print_fmt( "{}Expire time must be a value between {}0{} and {}{}{}.&D\r\n", s1, s2, s1, s2, MAX_BOARD_EXPIRE, s1 );
         return;
      }

      auto expiry_time = current_time + std::chrono::days( value );
      board->expire = expiry_time;

      ch->print_fmt( "{}From now on, notes on the {}{}{} board will expire after {}{}{} days.\r\n", s1, s2, board->name, s1, s2, value, s1 );
      ch->print_fmt( "{}Please note: This will not effect notes currently on the board. To effect {}ALL{} notes, "
                  "type: {}bset <board> expireall <days>&D\r\n", s1, s3, s1, s2 );
      write_board( board );
      return;
   }

   if( !str_cmp( arg2, "expireall" ) )
   {
      if( value < 0 || value > MAX_BOARD_EXPIRE )
      {
         ch->print_fmt( "{}Expire time must be a value between {}0{} and {}{}{}.&D\r\n", s1, s2, s1, s2, MAX_BOARD_EXPIRE, s1 );
         return;
      }

      auto expiry_time = current_time + std::chrono::days( value );
      board->expire = expiry_time;

      for( auto* note : board->nlist )
      {
         note->expire = expiry_time;
      }
      ch->print_fmt( "{}All notes on the {}{}{} board will expire after {}{}{} days.&D\r\n", s1, s2, board->name, s1, s2, value, s1 );
      ch->print( "Performing board pruning now...\r\n" );
      board_check_expire( board );
      write_board( board );
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
      board->desc = argument;
      write_board( board );
      ch->print_fmt( "{}The desc for {}{}{} has been set to '{}{}{}'.&D\r\n", s1, s2, board->name, s1, s2, board->desc, s1 );
      return;
   }

   if( !str_cmp( arg2, "group" ) )
   {
      if( argument.empty(  ) )
      {
         board->group.clear();
         ch->print( "Group cleared.\r\n" );
         return;
      }
      board->group = argument;
      write_board( board );
      ch->print_fmt( "{}The group for {}{}{} has been set to '{}{}{}'.&D\r\n", s1, s2, board->name, s1, s2, board->group, s1 );
      return;
   }

   if( !str_cmp( arg2, "raise" ) )
   {
      std::list<board_data *>::iterator bd;

      for( bd = bdlist.begin(  ); bd != bdlist.end(  ); ++bd )
      {
         if( *bd == board )
            break;
      }
      if( bd == bdlist.begin(  ) )
      {
         ch->print_fmt( "{}But '{}{}{}' is already the first board!&D\r\n", s1, s2, board->name, s1 );
         return;
      }
      --bd;

      bdlist.remove( board );
      bdlist.insert( bd, board );

      ch->print_fmt( "{}Moved '{}{}{}' above '{}{}{}'.&D\r\n", s1, s2, board->name, s1, s2, ( *bd )->name, s1 );

      write_board_list(); // Bugfix 6/21/2026 - It doesn't do much good to change the order if you aren't going to save the results.
      return;
   }

   if( !str_cmp( arg2, "lower" ) )
   {
      std::list<board_data *>::iterator bd;

      for( bd = bdlist.begin(  ); bd != bdlist.end(  ); ++bd )
      {
         if( *bd == board )
            break;
      }

      if( ++bd == bdlist.end(  ) )
      {
         ch->print_fmt( "{}But '{}{}{}' is already the last board!&D\r\n", s1, s2, board->name, s1 );
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
      ch->print_fmt( "{}Moved '{}{}{}' below '{}{}{}'.&D\r\n", s1, s2, board->name, s1, s2, ( *bd )->name, s1 );

      write_board_list(); // Bugfix 6/21/2026 - It doesn't do much good to change the order if you aren't going to save the results.
      return;
   }
   do_board_set( ch, "" );
}

/* Begin Project Code */
project_data *get_project_by_number( int pnum )
{
   int pcount = 1;

   for( auto* proj : projlist )
   {
      if( pcount == pnum )
         return proj;
      else
         ++pcount;
   }
   return nullptr;
}

note_data *get_log_by_number( project_data * pproject, int pnum )
{
   int pcount = 1;

   for( auto* plog : pproject->nlist )
   {
      if( pcount == pnum )
         return plog;
      else
         ++pcount;
   }
   return nullptr;
}

project_data::project_data(  )
{
}

project_data::~project_data(  )
{
   for( auto it = nlist.begin(  ); it != nlist.end(  ); )
   {
      note_data *note = *it;
      ++it;

      nlist.remove( note );
      deleteptr( note );
   }
   projlist.remove( this );
}

void free_projects( void )
{
   for( auto it = projlist.begin(  ); it != projlist.end(  ); )
   {
      project_data *project = *it;
      ++it;

      deleteptr( project );
   }
   return;
}

// 0: ???
// 1: ???
// 2: The apparent current and only version so far?
constexpr int PROJECTS_VERSION = 2;

void write_projects( void )
{
   std::ofstream stream;

   stream.open( std::filesystem::path( PROJECTS_FILE ) );
   if( !stream.is_open(  ) )
   {
      bug( "{}: Cannot open {} for writing: {}", __func__, PROJECTS_FILE, std::strerror(errno) );
      return;
   }

   stream << std::format( "#VERSION  {}\n", PROJECTS_VERSION );

   for( auto* proj : projlist )
   {
      auto date_stamp = std::chrono::system_clock::to_time_t( proj->date_stamp );

      stream << std::format( "Name   {}~\n", proj->name );
      stream << std::format( "Owner  {}~\n", ( !proj->owner.empty() ) ? proj->owner : "None" );
      if( !proj->coder.empty() )
         stream << std::format( "Coder  %s~\n", proj->coder );
      stream << std::format( "Status {}~\n", ( !proj->status.empty() ) ? proj->status : "No update." );
      stream << std::format( "Date_stamp   {}\n", date_stamp );
      if( !proj->realm_name.empty() )
         stream << std::format( "Realm       {}~\n", proj->realm_name );
      if( !proj->description.empty() )
         stream << std::format( "Description {}~\n", proj->description );

      for( auto* nlog : proj->nlist )
      {
         stream << "#LOG\n";
         write_note( nlog, stream );
      }
      stream << "#END\n";
   }
   stream.close();
   if( stream.fail() )
      bug( "{}: Error occurred after closing {}: ", __func__, PROJECTS_FILE, std::strerror(errno) );
}

project_data *read_project( std::ifstream & stream )
{
   project_data *project = new project_data;

   int file_ver = 0;
   std::string key;
   while( stream >> key )
   {
      note_data *note;

      if( key == "#VERSION" || key == "Version" )
         stream >> file_ver;
      else if( key == "#END" ) // Once the main board data is loaded, now we concentrate on reading the actual board files.
         return project;
      else if( key == "Name" )
         project->name = fread_line( stream );
      else if( key == "Owner" )
         project->owner = fread_line( stream );
      else if( key == "Coder" )
         project->coder = fread_line( stream );
      else if( key == "Status" )
         project->status = fread_line( stream );
      else if( key == "Date_stamp" )
      {
         time_t loaded_time;

         stream >> loaded_time;
         project->date_stamp = std::chrono::system_clock::from_time_t( loaded_time );
      }

      else if( key == "Realm" )
         project->realm_name = fread_line( stream );
      else if( key == "Description" )
         project->description = fread_line( stream );
      else if( key == "#LOG" )
      {
         note = new note_data;
         read_note( note, file_ver, stream );
         project->nlist.push_back( note );
      }
      else
         bug( "{}: Bad section in {} - skipping.", __func__, PROJECTS_FILE );
   }
   bug( "{}: Unexpected end of filestream!", __func__ );
   return nullptr;
}

void load_projects( void )
{
   std::ifstream stream;
   project_data *project;

   projlist.clear(  );

   stream.open( std::filesystem::path( PROJECTS_FILE ) );
   if( !stream.is_open(  ) )
   {
      // No need to report an error here since you may not have any projects yet.
      return;
   }

   while( ( project = read_project( stream ) ) != nullptr )
      projlist.push_back( project );
   stream.close();
}

// Hacky looking ugly define, but fuck having to type all this out all the time.
// That's why we use functions instead. You still didn't have to type this out all the time...
bool IS_PROJECT_ADMIN( char_data * ch, project_data * proj )
{
   if( IS_ADMIN_REALM( ch ) || ( IS_REALM_LEADER( ch ) && !str_cmp( ch->pcdata->realm_name, proj->realm_name ) ) )
      return true;
   return false;
}

/* Last thing left to revampitize -- Xorith */
CMDF( do_project )
{
   project_data *pproject;
   std::string arg;
   int pcount, pnum;

   if( ch->isnpc(  ) )
      return;

   if( !ch->desc )
   {
      bug( "{}: no descriptor", __func__ );
      return;
   }

   switch ( ch->substate )
   {
      default:
         break;

      case SUB_WRITING_NOTE:
         if( !ch->pcdata->pnote )
         {
            bug( "{}: log got lost?", __func__ );
            ch->print( "Your log was lost!\r\n" );
            ch->stop_editing(  );
            return;
         }
         if( ch->pcdata->dest_buf != ch->pcdata->pnote )
            bug( "{}: sub_writing_note: ch->pcdata->dest_buf != ch->pcdata->pnote", __func__ );
         ch->pcdata->pnote->text = ch->copy_buffer( );
         ch->stop_editing(  );
         return;

      case SUB_PROJ_DESC:
         if( !ch->pcdata->dest_buf )
         {
            ch->print( "Your description was lost!" );
            bug( "{}: sub_project_desc: nullptr ch->pcdata->dest_buf", __func__ );
            ch->substate = SUB_NONE;
            return;
         }
         pproject = ( project_data * ) ch->pcdata->dest_buf;
         pproject->description = ch->copy_buffer( );
         ch->stop_editing(  );
         ch->substate = ch->tempnum;
         write_projects(  );
         return;

      case SUB_EDIT_ABORT:
         ch->print( "Aborting...\r\n" );
         ch->substate = SUB_NONE;
         if( ch->pcdata->dest_buf )
            ch->pcdata->dest_buf = nullptr;
         if( ch->pcdata->pnote )
         {
            deleteptr( ch->pcdata->pnote );
            ch->pcdata->pnote = nullptr;
         }
         return;
   }

   ch->set_color( AT_NOTE );
   argument = one_argument( argument, arg );
   smash_tilde( argument );

   if( arg.empty(  ) || !str_cmp( arg, "list" ) )
   {
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
      for( auto* proj : projlist )
      {
         ++pcount;
         if( !proj->status.empty() && !str_cmp( "Done", proj->status ) )
            continue;
         if( !aflag )
         {
            ch->pager_fmt( "{:2}{} | {:<11} | {:<26} | {:<15} | {:<12}\r\n",
                        pcount, proj->realm_name, !proj->owner.empty() ? proj->owner : "(None)",
                        print_lngstr( proj->name, 26 ), mini_c_time( proj->date_stamp, ch->pcdata->timezone_name ), !proj->status.empty() ? proj->status : "(None)" );
         }
         else if( !proj->taken )
         {
            if( !projects_available )
               projects_available = true;
            ch->pager_fmt( "{:2}{} | {:<30} | {}\r\n", pcount, proj->realm_name,
                        !proj->name.empty() ? proj->name : "(None)", mini_c_time( proj->date_stamp, ch->pcdata->timezone_name ) );
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
      pcount = 0;
      ch->pager( " #  | Owner       | Project                    \r\n" );
      ch->pager( "----|-------------|----------------------------\r\n" );

      for( auto* proj : projlist )
      {
         ++pcount;
         if( ( !proj->status.empty() && str_cmp( proj->status, "approved" ) ) || !proj->coder.empty() )
            continue;
         ch->pager_fmt( "{:2}{} | {:<11} | {:<26}\r\n", pcount, proj->realm_name, !proj->owner.empty() ? proj->owner : "(None)", !proj->name.empty() ? proj->name : "(None)" );
      }
      return;
   }

   if( !str_cmp( arg, "more" ) || !str_cmp( arg, "mine" ) )
   {
      bool MINE = false;

      pcount = 0;

      if( !str_cmp( arg, "mine" ) )
         MINE = true;

      ch->pager( "\r\n" );
      ch->pager( " #  | Owner       | Project                    | Coder       | Status     | Logs\r\n" );
      ch->pager( "----|-------------|----------------------------|-------------|------------|-----\r\n" );

      for( auto* proj : projlist )
      {
         ++pcount;
         if( MINE && ( proj->owner.empty() || str_cmp( ch->name, proj->owner ) ) && ( proj->coder.empty() || str_cmp( ch->name, proj->coder ) ) )
            continue;
         else if( !MINE && !proj->status.empty() && !str_cmp( "Done", proj->status ) )
            continue;
         int num_logs = proj->nlist.size(  );
         ch->pager_fmt( "{:2}{} | {:<11} | {:<26} | {:<11} | {:<10} | {:3}\r\n",
                     pcount, proj->realm_name, !proj->owner.empty() ? proj->owner : "(None)",
                     print_lngstr( proj->name, 26 ), !proj->coder.empty() ? proj->coder : "(None)", !proj->status.empty() ? proj->status : "(None)", num_logs );
      }
      return;
   }

   if( !str_cmp( arg, "add" ) )
   {
      realm_data *realm = nullptr;

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
            ch->print_fmt( "{} is not a valid realm.\r\n", arg );
            return;
         }
      }

      project_data *new_project = new project_data;
      new_project->name = argument;
      new_project->taken = false;
      new_project->realm_name = realm->name;
      new_project->date_stamp = current_time;
      projlist.push_back( new_project );
      write_projects(  );
      ch->print_fmt( "New {} Project '{}' added.\r\n", new_project->realm_name, new_project->name );
      return;
   }

   if( !is_number( arg ) )
   {
      ch->print_fmt( "{} is an invalid project!\r\n", arg );
      return;
   }

   pnum = std::stoi( arg );
   pproject = get_project_by_number( pnum );
   if( !pproject )
   {
      ch->print_fmt( "Project #{} does not exist.\r\n", pnum );
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
      ch->set_editor_desc( std::format( "Project description for project '{}'.", !pproject->name.empty() ? pproject->name : "(No name)" ) );
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

      ch->print_fmt( "Project '{}' has been deleted.\r\n", pproject->name );
      deleteptr( pproject );
      write_projects(  );
      return;
   }

   if( !str_cmp( arg, "take" ) )
   {
      if( pproject->taken && !pproject->owner.empty() && !str_cmp( pproject->owner, ch->name ) )
      {
         pproject->taken = false;
         pproject->owner.clear();
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
         ch->print_fmt( "Taking Project: '{}' from Owner: '{}'!\r\n", pproject->name, !pproject->owner.empty() ? pproject->owner : "nullptr" );

      pproject->owner = ch->name;
      pproject->taken = true;
      write_projects(  );
      ch->print_fmt( "You're now the owner of Project '{}'.\r\n", pproject->name );
      return;
   }

   if( ( !str_cmp( arg, "coder" ) ) || ( !str_cmp( arg, "builder" ) ) )
   {
      if( !pproject->coder.empty() && !str_cmp( ch->name, pproject->coder ) )
      {
         pproject->coder.clear();
         ch->print_fmt( "You removed yourself as the {}.\r\n", arg );
         write_projects(  );
         return;
      }
      else if( !pproject->coder.empty() && !IS_PROJECT_ADMIN( ch, pproject ) )
      {
         ch->print_fmt( "This project already has a {}.\r\n", arg );
         return;
      }
      else if( !pproject->coder.empty() && IS_PROJECT_ADMIN( ch, pproject ) )
      {
         ch->print_fmt( "Removing {} as {} of this project!\r\n", pproject->coder, arg );
         pproject->coder.clear();
         ch->print( "Coder removed.\r\n" );
         write_projects(  );
         return;
      }

      pproject->coder = ch->name;
      write_projects(  );
      ch->print_fmt( "You are now the {} of {}.\r\n", arg, pproject->name );
      return;
   }

   if( !str_cmp( arg, "status" ) )
   {
      if( !pproject->owner.empty() && str_cmp( pproject->owner, ch->name ) && !IS_PROJECT_ADMIN( ch, pproject ) )
      {
         ch->print( "This is not your project!\r\n" );
         return;
      }
      pproject->status = argument;
      write_projects(  );
      ch->print_fmt( "Project Status set to: {}\r\n", pproject->status );
      return;
   }

   if( !str_cmp( arg, "show" ) )
   {
      if( !pproject->description.empty() )
         ch->print( pproject->description );
      else
         ch->print( "That project does not have a description.\r\n" );
      return;
   }

   if( !str_cmp( arg, "log" ) )
   {
      if( !str_cmp( argument, "write" ) )
      {
         if( !ch->pcdata->pnote )
            ch->note_attach( NOTE_PROJECT );
         ch->substate = SUB_WRITING_NOTE;
         ch->pcdata->dest_buf = ch->pcdata->pnote;
         ch->set_editor_desc( std::format( "A log note in project '{}', entitled '{}'.",
                                 !pproject->name.empty() ? pproject->name : "(No name)", !ch->pcdata->pnote->subject.empty() ? ch->pcdata->pnote->subject : "(No subject)" ) );
         ch->start_editing( ch->pcdata->pnote->text );
         return;
      }

      argument = one_argument( argument, arg );

      if( !str_cmp( arg, "subject" ) )
      {
         if( !ch->pcdata->pnote )
            ch->note_attach( NOTE_PROJECT );

         ch->pcdata->pnote->subject = argument;
         ch->print_fmt( "Log Subject set to: {}\r\n", ch->pcdata->pnote->subject );
         return;
      }

      if( !str_cmp( arg, "post" ) )
      {
         if( ( !pproject->owner.empty() && str_cmp( ch->name, pproject->owner ) )
             || ( !pproject->coder.empty() && str_cmp( ch->name, pproject->coder ) ) || !IS_PROJECT_ADMIN( ch, pproject ) )
         {
            ch->print( "This is not your project!\r\n" );
            return;
         }

         if( !ch->pcdata->pnote )
         {
            ch->print( "You have no log in progress.\r\n" );
            return;
         }

         if( ch->pcdata->pnote->subject.empty() )
         {
            ch->print( "Your log has no subject.\r\n" );
            return;
         }

         if( ch->pcdata->pnote->text.empty() )
         {
            ch->print( "Your log has no text!\r\n" );
            return;
         }

         ch->pcdata->pnote->date_stamp = current_time;
         note_data *plog = ch->pcdata->pnote;
         ch->pcdata->pnote = nullptr;

         if( !argument.empty(  ) )
         {
            int plog_num;
            note_data *tplog;
            plog_num = std::stoi( argument );

            tplog = get_log_by_number( pproject, plog_num );

            if( !tplog )
            {
               ch->print_fmt( "Log #{} can not be found.\r\n", plog_num );
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
         if( ( !pproject->owner.empty() && str_cmp( ch->name, pproject->owner ) )
             || ( !pproject->coder.empty() && str_cmp( ch->name, pproject->coder ) ) || !IS_PROJECT_ADMIN( ch, pproject ) )
         {
            ch->print( "This is not your project!\r\n" );
            return;
         }

         pcount = 0;
         ch->pager_fmt( "Project: {:<12}: {}\r\n", !pproject->owner.empty() ? pproject->owner : "(None)", pproject->name );

         for( auto* tlog : pproject->nlist )
         {
            ++pcount;
            ch->pager_fmt( "{:2}) {:<12}: {}\r\n", pcount, !tlog->sender.empty() ? tlog->sender : "--Error--", !tlog->subject.empty() ? tlog->subject : "" );
         }
         if( pcount == 0 )
            ch->print( "No logs available.\r\n" );
         else
            ch->print_fmt( "{} log{} found.\r\n", pcount, pcount == 1 ? "" : "s" );
         return;
      }

      if( !is_number( arg ) )
      {
         ch->print( "Invalid log.\r\n" );
         return;
      }

      pnum = std::stoi( arg );

      note_data *plog = get_log_by_number( pproject, pnum );
      if( !plog )
      {
         ch->print( "Invalid log.\r\n" );
         return;
      }

      if( !str_cmp( argument, "delete" ) )
      {
         if( ( !pproject->owner.empty() && str_cmp( ch->name, pproject->owner ) )
             || ( !pproject->coder.empty() && str_cmp( ch->name, pproject->coder ) ) || !IS_PROJECT_ADMIN( ch, pproject ) )
         {
            ch->print( "This is not your project!\r\n" );
            return;
         }
         pproject->nlist.remove( plog );
         deleteptr( plog );
         write_projects(  );
         ch->print_fmt( "Log #{} has been deleted.\r\n", pnum );
         return;
      }

      if( !str_cmp( argument, "read" ) )
      {
         if( ( !pproject->owner.empty() && str_cmp( ch->name, pproject->owner ) )
             || ( !pproject->coder.empty() && str_cmp( ch->name, pproject->coder ) ) || !IS_PROJECT_ADMIN( ch, pproject ) )
         {
            ch->print( "This is not your project!\r\n" );
            return;
         }
         note_to_char( ch, plog, nullptr, -1 );
         return;
      }
   }
   interpret( ch, "help project" );
}
