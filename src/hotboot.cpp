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
 *                             Hotboot module                               *
 ****************************************************************************/

#include <dlfcn.h>   /* Required for libdl - Trax */
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
void music_to_char( std::string_view, int, char_data *, bool );
void reset_sound( char_data * );
void reset_music( char_data * );
void save_timedata(  );
void save_weathermap(  );
void check_auth_state( char_data * );
affect_data *fread_afk_affect( FILE * );
void fwrite_afk_affect( FILE *, affect_data * );
#if defined(SQL)
 void close_db(  );
 void init_mysql(  );
#endif

/*
 * Save the world's objects and mobs in their current positions -- Scion
 */
void save_mobile( FILE * fp, char_data * mob )
{
   skill_type *skill = nullptr;

   if( !mob->isnpc(  ) || !fp )
      return;

   fprintf( fp, "%s", "#MOBILE\n" );
   fprintf( fp, "Vnum	%d\n", mob->pIndexData->vnum );
   fprintf( fp, "Level   %d\n", mob->level );
   fprintf( fp, "Gold	%d\n", mob->gold );
   fprintf( fp, "Resetvnum %d\n", mob->resetvnum );
   fprintf( fp, "Resetnum  %d\n", mob->resetnum );
   if( mob->in_room )
   {
      if( mob->has_actflag( ACT_SENTINEL ) )
      {
         /*
          * Sentinel mobs get stamped with a "home room" when they are created
          * by create_mobile(), so we need to save them in their home room regardless
          * of where they are right now, so they will go to their home room when they
          * enter the game from a reboot or copyover -- Scion 
          */
         fprintf( fp, "Room	%d\n", mob->home_vnum );
      }
      else
         fprintf( fp, "Room	%d\n", mob->in_room->vnum );
   }
   else
      fprintf( fp, "Room	%d\n", ROOM_VNUM_LIMBO );
   if( mob->continent )
   {
      fprintf( fp, "Continent    %s\n", mob->continent->name.c_str( ) );
      fprintf( fp, "Coordinates  %d %d\n", mob->map_x, mob->map_y );
   }
   if( !mob->name.empty() && !mob->pIndexData->player_name.empty() && str_cmp( mob->name, mob->pIndexData->player_name ) )
      fprintf( fp, "Name     %s~\n", mob->name.c_str() );
   if( !mob->short_descr.empty() && !mob->pIndexData->short_descr.empty() && str_cmp( mob->short_descr, mob->pIndexData->short_descr ) )
      fprintf( fp, "Short	%s~\n", mob->short_descr.c_str() );
   if( !mob->long_descr.empty() && !mob->pIndexData->long_descr.empty() && str_cmp( mob->long_descr, mob->pIndexData->long_descr ) )
      fprintf( fp, "Long	%s~\n", mob->long_descr.c_str() );
   if( !mob->chardesc.empty() && !mob->pIndexData->chardesc.empty() && str_cmp( mob->chardesc, mob->pIndexData->chardesc ) )
      fprintf( fp, "Description %s~\n", mob->chardesc.c_str() );
   fprintf( fp, "HpManaMove   %d %d %d %d %d %d\n", mob->hit, mob->max_hit, mob->mana, mob->max_mana, mob->move, mob->max_move );
   fprintf( fp, "Position %d\n", mob->position );
   if( mob->has_actflags(  ) )
      fprintf( fp, "Flags %s~\n", bitset_string( mob->get_actflags(  ), act_flags ) );
   if( mob->has_aflags(  ) )
      fprintf( fp, "AffectedBy   %s~\n", bitset_string( mob->get_aflags(  ), aff_flags ) );

   for( auto* af : mob->affects )
   {
      if( af->type >= 0 && !( skill = get_skilltype( af->type ) ) )
         continue;

      if( af->type >= 0 && af->type < TYPE_PERSONAL )
         fprintf( fp, "AffectData   '%s' %3d %3d %3d %d\n", skill->name.c_str(), af->duration, af->modifier, af->location, af->bit );
      else
         fwrite_afk_affect( fp, af );
   }
   mob->de_equip(  );

   fprintf( fp, "\n" );

   if( !mob->carrying.empty(  ) )
      fwrite_obj( mob, mob->carrying, nullptr, fp, 0, true );

   mob->re_equip(  );

   fprintf( fp, "%s", "EndMobile\n\n" );
}

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

         FILE *objfp;

         std::filesystem::path filename = std::format( "{}{}", HOTBOOT_DIR, pRoomIndex->vnum );
         if( !( objfp = fopen( filename.c_str(), "w" ) ) )
         {
            bug( "{}:{} Cannot open {} for writing: {}", __func__, pRoomIndex->vnum, filename.string(), std::strerror(errno) );
            continue;
         }
         fwrite_obj( nullptr, pRoomIndex->objects, nullptr, objfp, 0, true );
         fprintf( objfp, "%s", "#END\n" );
         FCLOSE( objfp );
      }
   }

   FILE *mobfp;

   std::filesystem::path filename = std::format( "{}{}", SYSTEM_DIR, MOB_FILE );
   if( !( mobfp = fopen( filename.c_str(), "w" ) ) )
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
            save_mobile( mobfp, rch );
      }
      fprintf( mobfp, "%s", "#END\n" );
      FCLOSE( mobfp );
   }
}

char_data *load_mobile( FILE * fp )
{
   char_data *mob = nullptr;
   int inroom = 0;
   room_index *pRoomIndex = nullptr;

   std::string word = ( feof( fp ) ? "EndMobile" : fread_word( fp ) );

   if( word[0] == '\0' )
   {
      bug( "{}: EOF encountered reading file!", __func__ );
      word = "EndMobile";
   }

   if( !str_cmp( word, "Vnum" ) )
   {
      int vnum;

      vnum = fread_number( fp );
      if( !get_mob_index( vnum ) )
      {
         bug( "{}: No index data for vnum {}", __func__, vnum );
         return nullptr;
      }
      mob = get_mob_index( vnum )->create_mobile(  );
      if( !mob )
      {
         for( ;; )
         {
            word = ( feof( fp ) ? "EndMobile" : fread_word( fp ) );

            if( word[0] == '\0' )
            {
               bug( "{}: EOF encountered reading file!", __func__ );
               word = "EndMobile";
            }
            /*
             * So we don't get so many bug messages when something messes up
             * * --Shaddai 
             */
            if( !str_cmp( word, "EndMobile" ) )
               break;
         }
         bug( "{}: Unable to create mobile for vnum {}", __func__, vnum );
         return nullptr;
      }
   }
   else
   {
      for( ;; )
      {
         word = ( feof( fp ) ? "EndMobile" : fread_word( fp ) );

         if( word[0] == '\0' )
         {
            bug( "{}: EOF encountered reading file!", __func__ );
            word = "EndMobile";
         }

         /*
          * So we don't get so many bug messages when something messes up
          * * --Shaddai 
          */
         if( !str_cmp( word, "EndMobile" ) )
            break;
      }

      bug( "{}: Vnum not found", __func__ );
      return nullptr;
   }

   for( ;; )
   {
      word = ( feof( fp ) ? "EndMobile" : fread_word( fp ) );

      if( word[0] == '\0' )
      {
         bug( "{}: EOF encountered reading file!", __func__ );
         word = "EndMobile";
      }

      switch ( to_upper( word[0] ) )
      {
         default:
            bug( "{}: no match: {}", __func__, word );
            fread_to_eol( fp );
            break;

         case '*':
            fread_to_eol( fp );
            break;

         case '#':
            if( !str_cmp( word, "#OBJECT" ) )
            {
               mob->tempnum = -9999;   /* Hackish, yes. Works though doesn't it? */
               fread_obj( mob, fp, OS_CARRY );
            }
            break;

         case 'A':
            if( !str_cmp( word, "AffData" ) || !str_cmp( word, "AffectData" ) )
            {
               affect_data *paf;

               if( !str_cmp( word, "AffData" ) )
                  paf = fread_afk_affect( fp );
               else
               {
                  paf = new affect_data;
                  paf->type = -1;
                  paf->duration = -1;
                  paf->bit = 0;
                  paf->modifier = 0;
                  paf->rismod.reset(  );

                  int sn;
                  std::string sname = fread_word( fp );

                  if( ( sn = skill_lookup( sname ) ) < 0 )
                  {
                     if( ( sn = herb_lookup( sname ) ) < 0 )
                        log_printf( "{}: unknown skill.", __func__ );
                     else
                        sn += TYPE_HERB;
                  }
                  paf->type = sn;
                  paf->duration = fread_number( fp );
                  paf->modifier = fread_number( fp );
                  paf->location = fread_number( fp );
                  if( paf->location == APPLY_WEAPONSPELL
                      || paf->location == APPLY_WEARSPELL || paf->location == APPLY_REMOVESPELL || paf->location == APPLY_STRIPSN || paf->location == APPLY_RECURRINGSPELL )
                     paf->modifier = slot_lookup( paf->modifier );
                  paf->bit = fread_number( fp );
               }
               mob->affects.push_back( paf );
               break;
            }

            if( !str_cmp( word, "AffectedBy" ) )
            {
               mob->set_file_aflags( fp );
               break;
            }
            break;

         case 'C':
            if( !str_cmp( word, "Continent" ) )
            {
               std::string temp;

               fread_string( temp, fp );
               continent_data *continent = find_continent_by_name( temp );

               if( continent )
                  mob->continent = continent;
               break;
            }

            if( !str_cmp( word, "Coordinates" ) )
            {
               mob->map_x = fread_short( fp );
               mob->map_y = fread_short( fp );
               break;
            }
            break;

         case 'D':
            if( !str_cmp( word, "Description" ) )
            {
               fread_string( mob->chardesc, fp );
               break;
            }
            break;

         case 'E':
            if( !str_cmp( word, "EndMobile" ) )
            {
               if( inroom == 0 )
                  inroom = ROOM_VNUM_LIMBO;
               pRoomIndex = get_room_index( inroom );
               if( !pRoomIndex )
                  pRoomIndex = get_room_index( ROOM_VNUM_LIMBO );
               mob->tempnum = -9998;   /* Yet another hackish fix! */
               if( !mob->to_room( pRoomIndex ) )
                  log_printf( "char_to_room: {}:{}, line {}.", __FILE__, __func__, __LINE__ );
               update_room_reset( mob, false );
               return mob;
            }

            if( !str_cmp( word, "End" ) ) /* End of object, need to ignore this. sometimes they creep in there somehow -- Scion */
            {
               ;  /* Trick the system into thinking it matched something */
            }
            break;

         case 'F':
            if( !str_cmp( word, "Flags" ) )
            {
               mob->set_file_actflags( fp );
               break;
            }
            break;

         case 'G':
            KEY( "Gold", mob->gold, fread_number( fp ) );
            break;

         case 'H':
            if( !str_cmp( word, "HpManaMove" ) )
            {
               mob->hit = fread_number( fp );
               mob->max_hit = fread_number( fp );
               mob->mana = fread_number( fp );
               mob->max_mana = fread_number( fp );
               mob->move = fread_number( fp );
               mob->max_move = fread_number( fp );

               if( mob->max_move <= 0 )
                  mob->max_move = 150;

               break;
            }
            break;

         case 'L':
            if( !str_cmp( word, "Long" ) )
            {
               fread_string( mob->long_descr, fp );
               break;
            }
            KEY( "Level", mob->level, fread_number( fp ) );
            break;

         case 'N':
            if( !str_cmp( word, "Name" ) )
            {
               fread_string( mob->name, fp );
               break;
            }
            break;

         case 'P':
            KEY( "Position", mob->position, fread_number( fp ) );
            break;

         case 'R':
            KEY( "Room", inroom, fread_number( fp ) );
            KEY( "Resetvnum", mob->resetvnum, fread_number( fp ) );
            KEY( "Resetnum", mob->resetnum, fread_number( fp ) );
            break;

         case 'S':
            if( !str_cmp( word, "Short" ) )
            {
               fread_string( mob->short_descr, fp );
               break;
            }
            break;
      }
   }
}

void read_obj_file( int vnum )
{
   FILE *fp;
   room_index *room;
   std::filesystem::path fname = std::format( "{}{}", HOTBOOT_DIR, vnum );

   if( !( room = get_room_index( vnum ) ) )
   {
      bug( "{}: ARGH! Missing room index for {}!", __func__, vnum );
      std::filesystem::remove( fname );
      return;
   }

   if( ( fp = fopen( fname.c_str(), "r" ) ) != nullptr )
   {
      // If the supermob still has objects in its possession, this means that a previous room dump failed.
      assert( supermob->carrying.empty( ) );

      rset_supermob( room );

      for( ;; )
      {
         char letter;
         std::string word;

         letter = fread_letter( fp );
         if( letter == '*' )
         {
            fread_to_eol( fp );
            continue;
         }

         if( letter != '#' )
         {
            bug( "{}: # not found.", __func__ );
            break;
         }

         word = fread_word( fp );
         if( !str_cmp( word, "OBJECT" ) ) /* Objects */
         {
            supermob->tempnum = -9999;
            fread_obj( supermob, fp, OS_CARRY );
         }
         else if( !str_cmp( word, "END" ) )  /* Done */
            break;
         else
         {
            bug( "{}: bad section: {}", __func__, word );
            break;
         }
      }
      FCLOSE( fp );
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
         tobj = tobj->to_room( room, supermob );
         supermob->unset_actflag( ACT_ONMAP );
         supermob->continent = nullptr;
         supermob->map_x = -1;
         supermob->map_y = -1;
      }
      release_supermob(  );
   }
   else
      log_printf( "Cannot open obj file: {}", vnum );
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
   FILE *mobfp;
   int done = 0;
   bool mobfile = false;

   std::filesystem::path file1 = std::format( "{}{}", SYSTEM_DIR, MOB_FILE );
   if( !( mobfp = fopen( file1.c_str(), "r" ) ) )
   {
      bug( "{}: Cannot open {} for reading: {}", __func__, file1.string(), std::strerror(errno) );
   }
   else
      mobfile = true;

   if( mobfile )
   {
      set_alarm( 30 );
      log_string( "World state: loading mobs" );
      while( done == 0 )
      {
         if( feof( mobfp ) )
            ++done;
         else
         {
            std::string word = fread_word( mobfp );
            if( str_cmp( word, "#END" ) )
               load_mobile( mobfp );
            else
               ++done;
         }
      }
   }

   FCLOSE( mobfp );
   set_alarm( 0 );

   load_obj_files(  );

   /*
    * Once loaded, the data needs to be purged in the event it causes a crash so that it won't try to reload 
    */
   std::filesystem::remove( file1 );
}

/* Warm reboot stuff, gotta make sure to thank Erwin for this :) */
CMDF( do_hotboot )
{
   std::list<descriptor_data *>::iterator ds;
   bool debugging = false;
   std::ofstream stream;

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

   for( ds = dlist.begin(  ); ds != dlist.end(  ); ++ds )
   {
      descriptor_data *d = *ds;
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

   stream.open( std::filesystem::path( HOTBOOT_FILE ) );

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
         stream << "#DESCRIPTOR" << std::endl;
         stream << "PlayerName    " << och->name << std::endl;
         stream << "DescNumber    " << d->descriptor << std::endl;
         stream << "CanCompress   " << d->can_compress << std::endl;
         stream << "IsCompressing " << d->is_compressing << std::endl;
         stream << "MSSPDetected  " << d->msp_detected << std::endl;
         stream << "InRoomVnum    " << och->in_room->vnum << std::endl;
         stream << "ClientPort    " << d->client_port << std::endl;
         stream << "Idle          " << d->idle << std::endl;
         stream << "HostName      " << d->hostname << std::endl;
         stream << "ClientName    " << d->client << std::endl;
         stream << "End" << std::endl << std::endl;

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

   stream << "#MAXPLAYERS " << sysdata->maxplayers << std::endl << std::endl;
   stream << "#END" << std::endl << std::endl;
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
   dlclose( sysdata->dlHandle );
   execl( EXE_FILE.data(), "afkmud", buf.c_str(), "hotboot", buf2.c_str(), buf3.c_str(), ( char * )nullptr );

   /*
    * Failed - successful exec will not return
    */
   perror( "do_copyover: execl" );

   sysdata->dlHandle = dlopen( nullptr, RTLD_LAZY );
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
   std::ifstream stream;
   std::string playername;
   descriptor_data *d = nullptr;
   bool fOld;
   int rvnum = 0;

   stream.open( std::filesystem::path( HOTBOOT_FILE ) );
   if( !stream.is_open(  ) )   /* there are some descriptors open which will hang forever then ? */
   {
      bug( "{}: Cannot open {} for readin: {}", __func__, HOTBOOT_FILE, std::strerror(errno) );
      std::exit( EXIT_FAILURE );
   }

   std::filesystem::remove( HOTBOOT_FILE ); /* In case something crashes - doesn't prevent reading */
   do
   {
      std::string key, value;
      char buf[MIL];

      stream >> key;
      stream.getline( buf, MIL );
      value = buf;

      strip_lspace( key );
      strip_lspace( value );
      strip_tilde( value );

      if( key.empty(  ) )
         continue;

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
         int maxp = atoi( value.c_str() );

         if( maxp > sysdata->maxplayers )
            sysdata->maxplayers = maxp;
         num_logins = maxp;
      }
      else if( key == "PlayerName" )
         playername = value;
      else if( key == "DescNumber" )
      {
         int desc = atoi( value.c_str() );

         d->descriptor = desc;
      }
      else if( key == "CanCompress" )
      {
         bool dcompress = atoi( value.c_str() );

         d->can_compress = dcompress;
      }
      else if( key == "IsCompressing" )
      {
         bool is_compressing = atoi( value.c_str() );

         d->is_compressing = is_compressing;
      }
      else if( key == "MSSPDetected" )
      {
         bool dmsp = atoi( value.c_str() );

         d->msp_detected = dmsp;
      }
      else if( key == "InRoomVnum" )
         rvnum = atoi( value.c_str() );
      else if( key == "ClientPort" )
      {
         int dport = atoi( value.c_str() );

         d->client_port = dport;
      }
      else if( key == "Idle" )
      {
         int idle = atoi( value.c_str() );

         d->idle = idle;
      }
      else if( key == "HostName" )
         d->hostname = value;
      else if( key == "ClientName" )
         d->client = value;
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
                  log_printf( "{}: Error restarting compression for {} on desc {}", __func__, playername.c_str(), d->descriptor );
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
         log_printf( "{}: Bad line in hotboot file: {} {}", __func__, key, value );
   }
   while( !stream.eof() );
   stream.close(  );

   log_string( "Hotboot recovery complete." );
}
