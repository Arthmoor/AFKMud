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
 *                             Hotboot Module                               *
 ****************************************************************************/

#include <cassert>
#include <filesystem>
#include <fstream>
#include "mud.h"
#include "descriptor.h"
#include "mobindex.h"
#include "mud_prog.h"
#include "overland.h"
#include "roomindex.h"
#include "smaugaffect.h"

#ifdef MULTIPORT
extern bool compilelock;
#endif

extern bool bootlock;
extern int control;
extern int num_logins;

void quotes( char_data * );
void set_alarm( long );
bool write_to_descriptor_old( int, std::string_view );
void update_room_reset( char_data *, bool );
void reset_sound( char_data * );
void reset_music( char_data * );
void save_timedata(  );
void save_weathermap(  );
void check_auth_state( char_data * );
void fwrite_mobile( char_data *, std::ofstream &, bool );
char_data *fread_mobile( std::ifstream &, bool, bool );
void close_libdl( );
void reopen_libdl( );
#if defined(SQL)
 void close_db(  );
 void init_mysql(  );
#endif

/*
 * Save the world's objects and mobs in their current positions -- Scion
 */
void save_world( void )
{
   std::map<int, room_index *>::iterator iroom;

   log_string( "Preserving world state...." );

   for( iroom = room_index_table.begin(); iroom != room_index_table.end(); ++iroom )
   {
      room_index *pRoomIndex = iroom->second;

      if( pRoomIndex )
      {
         if( pRoomIndex->objects.empty(  )   /* Skip room if nothing in it */
             || pRoomIndex->flags.test( ROOM_CLANSTOREROOM )   /* These rooms save on their own */
             || pRoomIndex->flags.test( ROOM_AUCTION )   /* These also save on their own */
             )
            continue;

         std::filesystem::path filename = std::format( "{}{}", HOTBOOT_DIR, pRoomIndex->vnum );
         std::ofstream stream( filename );
         if( !stream.is_open() )
         {
            bug( "{}:{} Cannot open {} for writing: {}", __func__, pRoomIndex->vnum, filename.string(), std::strerror(errno) );
            continue;
         }
         fwrite_obj( nullptr, pRoomIndex->objects, nullptr, stream, 0, true );
         stream << "#END\n";
         stream.close();
      }
   }

   std::filesystem::path filename = std::format( "{}{}", SYSTEM_DIR, MOB_FILE );
   std::ofstream stream( filename );
   if( !stream.is_open() )
   {
      bug( "{}: Cannot open {} for writing: {}", __func__, filename.string(), std::strerror(errno) );
   }
   else
   {
      for( auto* rch : charlist )
      {
         if( !rch->isnpc(  ) || rch == supermob || rch->has_actflag( ACT_PROTOTYPE ) || rch->has_actflag( ACT_PET ) )
            continue;
         else
         {
            rch->hotboot = true;
            fwrite_mobile( rch, stream, false );
            rch->hotboot = false;
         }
      }
      stream << "#END\n";
      stream.close();
   }
}

void read_obj_file( int vnum )
{
   room_index *room;

   std::filesystem::path fname = std::format( "{}{}", HOTBOOT_DIR, vnum );

   if( !( room = get_room_index( vnum ) ) )
   {
      bug( "{}: ARGH! Missing room index for {}!", __func__, vnum );
      std::filesystem::remove( fname );
      return;
   }

   std::ifstream stream( fname );
   if( stream.is_open() )
   {
      // If the supermob still has objects in its possession, this means that a previous room dump failed.
      assert( supermob->carrying.empty( ) );

      rset_supermob( room );

      std::string key;
      while( stream >> key )
      {
         if( key == "#OBJECT" ) /* Objects */
         {
            supermob->tempnum = -9999;
            fread_obj( supermob, stream, OS_CARRY );
         }
         else if( key == "#END" )  /* Done */
            break;
         else
         {
            bug( "{}: Bad section '{}' in {} - skipping.", __func__, key, fname.string() );
            fread_to_eol( stream );
         }
      }
      stream.close();
      std::filesystem::remove( fname );

      for( auto it = supermob->carrying.begin(); it != supermob->carrying.end(); )
      {
         obj_data *tobj = *it;
         ++it;

         if( tobj->extra_flags.test( ITEM_ONMAP ) )
         {
            supermob->set_actflag( ACT_ONMAP );
            supermob->continent = tobj->continent;
            supermob->map_x = tobj->map_x;
            supermob->map_y = tobj->map_y;
         }
         tobj->from_char(  );
         tobj->to_room( room, supermob );
         supermob->unset_actflag( ACT_ONMAP );
         supermob->continent = nullptr;
         supermob->map_x = -1;
         supermob->map_y = -1;
      }
      release_supermob(  );
   }
   else
      bug( "{}: Cannot open {} for reading: {}", __func__, fname.string(), std::strerror(errno) );
}

void load_obj_files( )
{
   set_alarm( 30 );
   log_string( "World state: loading objs" );

   std::filesystem::path hotboot_dir{ HOTBOOT_DIR };

   if( std::filesystem::exists( hotboot_dir ) && std::filesystem::is_directory( hotboot_dir ) )
   {
      for( const auto& entry : std::filesystem::directory_iterator( hotboot_dir ) )
      {
         const std::string filename = entry.path().filename().string();

         int vnum = std::stoi( filename );
         read_obj_file( vnum );
      }
   }
   set_alarm( 0 );
}

void load_world( void )
{
   bool mobfile = false;

   std::filesystem::path file1 = std::format( "{}{}", SYSTEM_DIR, MOB_FILE );
   std::ifstream stream( file1 );
   if( !stream.is_open() )
   {
      bug( "{}: Cannot open {} for reading: {}", __func__, file1.string(), std::strerror(errno) );
   }
   else
      mobfile = true;

   if( mobfile )
   {
      int done = 0;
      set_alarm( 30 );
      log_string( "World state: loading mobs" );
      while( done == 0 )
      {
         if( stream.eof() )
            ++done;
         else
         {
            std::string word = fread_word( stream );
            if( word != "#END" )
               fread_mobile( stream, false, true );
            else
               ++done;
         }
      }
   }
   stream.close();
   // Once loaded, the data needs to be purged in the event it causes a crash so that it won't try to reload
   std::filesystem::remove( file1 );

   set_alarm( 0 );
   load_obj_files(  );
}

/* Warm reboot stuff, gotta make sure to thank Erwin for this :) */
CMDF( do_hotboot )
{
   std::list<descriptor_data *>::iterator ds;
   bool debugging = false;

#ifdef MULTIPORT
   if( compilelock )
   {
      ch->print( "Cannot initiate hotboot while the system compiler is running.\r\n" );
      return;
   }
#endif

   if( bootlock )
   {
      ch->print( "Cannot initiate hotboot. A standard reboot is in progress.\r\n" );
      return;
   }

   if( !argument.empty(  ) )
   {
      if( str_cmp( argument, "debug" ) )
      {
         ch->print_fmt( "'{}' is not a valid option.\r\n", argument );
         ch->print( "Acceptable Syntax:\r\n\r\n" );
         ch->print( "Hotboot\r\n" );
         ch->print( "Hotboot debug\r\n" );
         return;
      }
      debugging = true;
   }

   for( auto* d : dlist )
   {
      char_data *victim;

      if( ( d->connected == CON_PLAYING || d->connected == CON_EDITING ) && ( victim = d->character ) != nullptr && !victim->isnpc(  ) && victim->in_room )
      {
         if( victim->fighting && victim->level >= 1 && victim->level <= MAX_LEVEL )
         {
            ch->print( "Cannot hotboot at this time. There are still combats in progress.\r\n" );
            return;
         }
         if( victim->inflight )
         {
            ch->print( "Cannot hotboot at this time. There are skyship flights in progress.\r\n" );
            return;
         }
      }

      if( d->connected == CON_EDITING && d->character )
      {
         ch->print( "Cannot hotboot at this time. Someone is using the line editor.\r\n" );
         return;
      }
   }

   log_printf( "Hotboot initiated by {}.", ch->name );

   std::ofstream stream( std::filesystem::path{HOTBOOT_FILE} );
   if( !stream.is_open(  ) )
   {
      ch->print( "Hotboot file not writeable, aborted.\r\n" );
      bug( "{}: Cannot open {} for writing: {}", __func__, HOTBOOT_FILE, std::strerror(errno) );
      return;
   }

   /*
    * And this one here will save the status of all objects and mobs in the game.
    * * This really should ONLY ever be used here. The less we do stuff like this the better.
    */
   save_world(  );

   log_string( "Saving player files and connection states...." );
   if( ch && ch->desc )
      ch->desc->write( ANSI_RESET );

   /*
    * For each playing descriptor, save its state 
    */
   for( ds = dlist.begin(  ); ds != dlist.end(  ); )
   {
      descriptor_data *d = *ds;
      ++ds;

      char_data *och = d->original ? d->original : d->character;

      if( !d->character || d->connected < CON_PLAYING )  /* drop those logging on */
      {
         d->write( "\r\nSorry, we are rebooting. Come back in a few minutes.\r\n" );
         close_socket( d, false );  /* throw'em out */
      }
      else
      {
         stream << "#DESCRIPTOR";
         stream << std::format( "PlayerName    {}\n", och->name );
         stream << std::format( "DescNumber    {}\n", d->descriptor );
         stream << std::format( "CanCompress   {}\n", d->can_compress ) ;
         stream << std::format( "IsCompressing {}\n", d->is_compressing );
         stream << std::format( "MSSPDetected  {}\n", d->msp_detected );
         stream << std::format( "InRoomVnum    {}\n", och->in_room->vnum );
         stream << std::format( "ClientPort    {}\n", d->client_port );
         stream << std::format( "Idle          {}\n", d->idle );
         stream << std::format( "HostName      {}\n", d->hostname );
         stream << std::format( "ClientName    {}\n", d->client );
         stream << "End\n\n";

         if( !debugging )
         {
            /*
             * One of two places this gets changed 
             */
            och->pcdata->hotboot = true;
            och->reset_sound(  );
            och->reset_music(  );
            och->save(  );

            d->write( "\r\nThe flow of time is halted momentarily as the world is reshaped!\r\n" );
            if( d->is_compressing )
               d->compressEnd(  );
         }
      }
   }

   stream << std::format( "#MAXPLAYERS {}\n\n", sysdata->maxplayers );
   stream << "#END\n";
   stream.close();
   if( stream.fail() )
      bug( "{}: Error occurred after closing {}: ", __func__, HOTBOOT_FILE, std::strerror(errno) );

   log_string( "Saving world time..." );
   save_timedata(  );   /* Preserve that up to the second calendar value :) */
   save_weathermap(  ); /* Same for the weather */

   if( debugging )
   {
      log_string( "Hotboot debug - Aborting before execl" );
      return;
   }

   log_string( "Executing hotboot..." );

   /*
    * exec - descriptors are inherited 
    * Yes, this is ugly, I know - Samson 
    */
   std::string buf = std::to_string( mud_port );
   std::string buf2 = std::to_string( control );
   std::string buf3 = "-1";

   set_alarm( 0 );
#if defined(SQL)
   close_db(  );
#endif
   close_libdl( );
   execl( EXE_FILE.data(), "afkmud", buf.c_str(), "hotboot", buf2.c_str(), buf3.c_str(), ( char * )nullptr );

   /*
    * Failed - successful exec will not return
    */
   perror( "do_copyover: execl" );

   reopen_libdl( );
   if( !sysdata->dlHandle )
   {
      bug( "{}: FATAL ERROR: Unable to reopen system executable handle!", __func__ );
      std::exit( EXIT_FAILURE );
   }
#if defined(SQL)
   init_mysql(  );
#endif
   bug( "{}: Hotboot execution failed!!", __func__ );
   ch->print( "Hotboot FAILED!\r\n" );
}

/* Recover from a hotboot - load players */
void hotboot_recover( void )
{
   std::ifstream stream( std::filesystem::path{HOTBOOT_FILE} );
   if( !stream.is_open(  ) )   /* there are some descriptors open which will hang forever then ? */
   {
      bug( "{}: Cannot open {} for reading: {}", __func__, HOTBOOT_FILE, std::strerror(errno) );
      std::exit( EXIT_FAILURE );
   }

   std::filesystem::remove( HOTBOOT_FILE ); // In case something crashes - doesn't prevent reading

   std::string playername;
   bool fOld;
   int rvnum = 0;
   descriptor_data *d = nullptr;
   std::string key;
   while( stream >> key )
   {
      if( key == "#DESCRIPTOR" )
      {
         d = new descriptor_data;

         d->init(  );
         d->connected = CON_PLOADED;
      }
      else if( key == "#END" )
         break;
      else if( key == "#MAXPLAYERS" )
      {
         int maxp;
         stream >> maxp;

         if( maxp > sysdata->maxplayers )
            sysdata->maxplayers = maxp;
         num_logins = maxp;
      }
      else if( key == "PlayerName" )
         playername = fread_line( stream, '\n' );
      else if( key == "DescNumber" )
         stream >> d->descriptor;
      else if( key == "CanCompress" )
         stream >> d->can_compress;
      else if( key == "IsCompressing" )
         stream >> d->is_compressing;
      else if( key == "MSSPDetected" )
          stream >> d->msp_detected;
      else if( key == "InRoomVnum" )
         stream >> rvnum;
      else if( key == "ClientPort" )
         stream >> d->client_port;
      else if( key == "Idle" )
         stream >> d->idle;
      else if( key == "HostName" )
         d->hostname = fread_line( stream, '\n' );
      else if( key == "ClientName" )
         d->client = fread_line( stream, '\n' );
      else if( key == "End" )
      {
         if( d->descriptor == 0 )
         { 
            bug( "{}: }}RALERT! Assigning socket 0! BAD BAD BAD! Name: {} Host: {}", __func__, playername, d->hostname );
            deleteptr( d );
            continue;
         }

         /*
         * Write something, and check if it goes error-free 
         */
         if( !d->can_compress && !write_to_descriptor_old( d->descriptor, "\r\nThe ether swirls in chaos.\r\n" ) )
         {
            close( d->descriptor ); /* nope */
            deleteptr( d );
            continue;
         }

         dlist.push_back( d );
         d->connected = CON_COPYOVER_RECOVER;   /* negative so close_socket will cut them off */

         /*
         * Now, find the pfile 
         */
         fOld = load_char_obj( d, playername, false, true );

         if( !fOld ) /* Player file not found?! */
         {
            d->write( "\r\nSomehow, your character was lost during hotboot. Contact the immortals ASAP.\r\n" );
            close_socket( d, false );
         }
         else  /* ok! */
         {
            d->write( "\r\nTime resumes its normal flow.\r\n" );
            d->character->in_room = get_room_index( rvnum );

            if( !d->character->in_room )
               d->character->in_room = get_room_index( ROOM_VNUM_TEMPLE );

            /*
            * Insert in the char_list 
            */
            charlist.push_back( d->character );
            pclist.push_back( d->character );

            if( !d->character->to_room( d->character->in_room ) )
               log_printf( "char_to_room: {}:{}, line {}.", __FILE__, __func__, __LINE__ );
            act( AT_MAGIC, "A puff of ethereal smoke dissipates around you!", d->character, nullptr, nullptr, TO_CHAR );
            act( AT_MAGIC, "$n appears in a puff of ethereal smoke!", d->character, nullptr, nullptr, TO_ROOM );
            d->connected = CON_PLAYING;

            d->character->pcdata->lasthost = d->hostname;

            if( d->can_compress )
            {
               if( !d->compressStart(  ) )
               {
                  log_printf( "{}: Error restarting compression for {} on desc {}", __func__, playername, d->descriptor );
                  d->can_compress = false;
                  d->is_compressing = false;
               }
            }

            /*
            * Mud Sound Protocol 
            */
            if( d->msp_detected )
               d->send_msp_startup(  );

            /*
            * @shrug, why not? :P 
            */
            if( d->character->has_pcflag( PCFLAG_ONMAP ) )
            {
               if( !d->character->in_room->flags.test( ROOM_WATCHTOWER ) )
                  d->character->music( "wilderness.mp3", 100, false );
            }

            if( ++num_descriptors > sysdata->maxplayers )
               sysdata->maxplayers = num_descriptors;

            if( d->character->level < LEVEL_IMMORTAL )
               ++sysdata->playersonline;
            quotes( d->character );
            check_auth_state( d->character );   /* new auth */
         }
      } // key == "End"
      else
      {
         bug( "{}: Bad section '{}' in {} - skipping.", __func__, key, HOTBOOT_FILE );
         fread_to_eol( stream );
      }
   }
   stream.close();

   log_string( "Hotboot recovery complete." );
}
