/****************************************************************************
 *                   ^     +----- |  / ^     ^ |     | +-\                  *
 *                  / \    |      | /  |\   /| |     | |  \                 *
 *                 /   \   +---   |<   | \ / | |     | |  |                 *
 *                /-----\  |      | \  |  v  | |     | |  /                 *
 *               /       \ |      |  \ |     | +-----+ +-/                  *
 ****************************************************************************
 * AFKMud Copyright 1997-2020 by Roger Libiez (Samson),                     *
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

#include <dirent.h>
#if !defined(WIN32)
#include <dlfcn.h>   /* Required for libdl - Trax */
#else
#include <unistd.h>
#include <windows.h>
#define dlopen( libname, flags ) LoadLibrary( (libname) )
#define dlclose( libname ) FreeLibrary( (HINSTANCE) (libname) )
#endif
#include "mud.h"
#include "descriptor.h"
#ifdef IMC
#include "imc.h"
#endif
#include "mobindex.h"
#include "mud_prog.h"
#include "roomindex.h"

#ifdef MULTIPORT
extern bool compilelock;
#endif
extern bool bootlock;
extern int control;
extern int num_logins;

void quotes( char_data * );
void set_alarm( long );
bool write_to_descriptor_old( int, const char * );
void update_room_reset( char_data *, bool );
void music_to_char( const string &, int, char_data *, bool );
void reset_sound( char_data * );
void reset_music( char_data * );
void save_timedata(  );
void check_auth_state( char_data * );
affect_data *fread_afk_affect( FILE * );
void fwrite_afk_affect( FILE *, affect_data * );
#if !defined(__CYGWIN__) && defined(SQL)
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
   fprintf( fp, "Coordinates  %d %d %d\n", mob->mx, mob->my, mob->wmap );
   if( mob->name && mob->pIndexData->player_name && str_cmp( mob->name, mob->pIndexData->player_name ) )
      fprintf( fp, "Name     %s~\n", mob->name );
   if( mob->short_descr && mob->pIndexData->short_descr && str_cmp( mob->short_descr, mob->pIndexData->short_descr ) )
      fprintf( fp, "Short	%s~\n", mob->short_descr );
   if( mob->long_descr && mob->pIndexData->long_descr && str_cmp( mob->long_descr, mob->pIndexData->long_descr ) )
      fprintf( fp, "Long	%s~\n", mob->long_descr );
   if( mob->chardesc && mob->pIndexData->chardesc && str_cmp( mob->chardesc, mob->pIndexData->chardesc ) )
      fprintf( fp, "Description %s~\n", mob->chardesc );
   fprintf( fp, "HpManaMove   %d %d %d %d %d %d\n", mob->hit, mob->max_hit, mob->mana, mob->max_mana, mob->move, mob->max_move );
   fprintf( fp, "Position %d\n", mob->position );
   if( mob->has_actflags(  ) )
      fprintf( fp, "Flags %s~\n", bitset_string( mob->get_actflags(  ), act_flags ) );
   if( mob->has_aflags(  ) )
      fprintf( fp, "AffectedBy   %s~\n", bitset_string( mob->get_aflags(  ), aff_flags ) );

   list < affect_data * >::iterator paf;
   for( paf = mob->affects.begin(  ); paf != mob->affects.end(  ); ++paf )
   {
      affect_data *af = *paf;

      if( af->type >= 0 && !( skill = get_skilltype( af->type ) ) )
         continue;

      if( af->type >= 0 && af->type < TYPE_PERSONAL )
         fprintf( fp, "AffectData   '%s' %3d %3d %3d %d\n", skill->name, af->duration, af->modifier, af->location, af->bit );
      else
         fwrite_afk_affect( fp, af );
   }
   mob->de_equip(  );

   if( !mob->carrying.empty(  ) )
      fwrite_obj( mob, mob->carrying, nullptr, fp, 0, true );

   mob->re_equip(  );

   fprintf( fp, "%s", "EndMobile\n\n" );
}

void save_world( void )
{
   map < int, room_index * >::iterator iroom;

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
         char filename[256];
         snprintf( filename, 256, "%s%d", HOTBOOT_DIR, pRoomIndex->vnum );
         if( !( objfp = fopen( filename, "w" ) ) )
         {
            bug( "%s: fopen %d", __func__, pRoomIndex->vnum );
            perror( filename );
            continue;
         }
         fwrite_obj( nullptr, pRoomIndex->objects, nullptr, objfp, 0, true );
         fprintf( objfp, "%s", "#END\n" );
         FCLOSE( objfp );
      }
   }

   FILE *mobfp;
   char filename[256];
   snprintf( filename, 256, "%s%s", SYSTEM_DIR, MOB_FILE );
   if( !( mobfp = fopen( filename, "w" ) ) )
   {
      bug( "%s: fopen mob file", __func__ );
      perror( filename );
   }
   else
   {
      list < char_data * >::iterator ich;
      for( ich = charlist.begin(  ); ich != charlist.end(  ); ++ich )
      {
         char_data *rch = *ich;

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

   const char *word = ( feof( fp ) ? "EndMobile" : fread_word( fp ) );

   if( word[0] == '\0' )
   {
      bug( "%s: EOF encountered reading file!", __func__ );
      word = "EndMobile";
   }

   if( !str_cmp( word, "Vnum" ) )
   {
      int vnum;

      vnum = fread_number( fp );
      if( !get_mob_index( vnum ) )
      {
         bug( "%s: No index data for vnum %d", __func__, vnum );
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
               bug( "%s: EOF encountered reading file!", __func__ );
               word = "EndMobile";
            }
            /*
             * So we don't get so many bug messages when something messes up
             * * --Shaddai 
             */
            if( !str_cmp( word, "EndMobile" ) )
               break;
         }
         bug( "%s: Unable to create mobile for vnum %d", __func__, vnum );
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
            bug( "%s: EOF encountered reading file!", __func__ );
            word = "EndMobile";
         }

         /*
          * So we don't get so many bug messages when something messes up
          * * --Shaddai 
          */
         if( !str_cmp( word, "EndMobile" ) )
            break;
      }
      mob->extract( true );
      bug( "%s: Vnum not found", __func__ );
      return nullptr;
   }

   for( ;; )
   {
      word = ( feof( fp ) ? "EndMobile" : fread_word( fp ) );

      if( word[0] == '\0' )
      {
         bug( "%s: EOF encountered reading file!", __func__ );
         word = "EndMobile";
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
                  char *sname = fread_word( fp );

                  if( ( sn = skill_lookup( sname ) ) < 0 )
                  {
                     if( ( sn = herb_lookup( sname ) ) < 0 )
                        log_printf( "%s: unknown skill.", __func__ );
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
            if( !str_cmp( word, "Coordinates" ) )
            {
               mob->mx = fread_short( fp );
               mob->my = fread_short( fp );
               mob->wmap = fread_short( fp );
               break;
            }
            break;

         case 'D':
            if( !str_cmp( word, "Description" ) )
            {
               STRFREE( mob->chardesc );
               mob->chardesc = fread_string( fp );
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
                  log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __func__, __LINE__ );
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
               STRFREE( mob->long_descr );
               mob->long_descr = fread_string( fp );
               break;
            }
            KEY( "Level", mob->level, fread_number( fp ) );
            break;

         case 'N':
            if( !str_cmp( word, "Name" ) )
            {
               STRFREE( mob->name );
               mob->name = fread_string( fp );
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
               STRFREE( mob->short_descr );
               mob->short_descr = fread_string( fp );
               break;
            }
            break;
      }
   }
}

void read_obj_file( const char *dirname, const char *filename )
{
   FILE *fp;
   char fname[256];
   room_index *room;

   int vnum = atoi( filename );
   snprintf( fname, 256, "%s%s", dirname, filename );

   if( !( room = get_room_index( vnum ) ) )
   {
      bug( "%s: ARGH! Missing room index for %d!", __func__, vnum );
      unlink( fname );
      return;
   }

   if( ( fp = fopen( fname, "r" ) ) != nullptr )
   {
      rset_supermob( room );

      for( ;; )
      {
         char letter;
         char *word;

         letter = fread_letter( fp );
         if( letter == '*' )
         {
            fread_to_eol( fp );
            continue;
         }

         if( letter != '#' )
         {
            bug( "%s: # not found.", __func__ );
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
            bug( "%s: bad section: %s", __func__, word );
            break;
         }
      }
      FCLOSE( fp );
      unlink( fname );

      list < obj_data * >::iterator iobj;
      for( iobj = supermob->carrying.begin(  ); iobj != supermob->carrying.end(  ); )
      {
         obj_data *tobj = *iobj;
         ++iobj;

         if( tobj->extra_flags.test( ITEM_ONMAP ) )
         {
            supermob->set_actflag( ACT_ONMAP );
            supermob->wmap = tobj->wmap;
            supermob->mx = tobj->mx;
            supermob->my = tobj->my;
         }
         tobj->from_char(  );
         tobj = tobj->to_room( room, supermob );
         supermob->unset_actflag( ACT_ONMAP );
         supermob->wmap = -1;
         supermob->mx = -1;
         supermob->my = -1;
      }
      release_supermob(  );
   }
   else
      log_string( "Cannot open obj file" );
}

void load_obj_files( void )
{
   DIR *dp;
   struct dirent *dentry;
   char directory_name[100];

   set_alarm( 30 );
   log_string( "World state: loading objs" );
   snprintf( directory_name, 100, "%s", HOTBOOT_DIR );
   dp = opendir( directory_name );
   dentry = readdir( dp );
   while( dentry )
   {
      /*
       * Added by Tarl 3 Dec 02 because we are now using CVS 
       */
      if( !str_cmp( dentry->d_name, "CVS" ) )
      {
         dentry = readdir( dp );
         continue;
      }
      if( dentry->d_name[0] != '.' )
         read_obj_file( directory_name, dentry->d_name );
      dentry = readdir( dp );
   }
   closedir( dp );
   set_alarm( 0 );
}

void load_world( void )
{
   FILE *mobfp;
   char file1[256];
   char *word;
   int done = 0;
   bool mobfile = false;

   snprintf( file1, 256, "%s%s", SYSTEM_DIR, MOB_FILE );
   if( !( mobfp = fopen( file1, "r" ) ) )
   {
      bug( "%s: fopen mob file", __func__ );
      perror( file1 );
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
            word = fread_word( mobfp );
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
   unlink( file1 );
}

/* Warm reboot stuff, gotta make sure to thank Erwin for this :) */
CMDF( do_hotboot )
{
   list < descriptor_data * >::iterator ds;

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

   for( ds = dlist.begin(  ); ds != dlist.end(  ); ++ds )
   {
      descriptor_data *d = *ds;
      char_data *victim;

      if( ( d->connected == CON_PLAYING || d->connected == CON_EDITING ) && ( victim = d->character ) != nullptr && !victim->isnpc(  ) && victim->in_room )
      {
         if( victim->fighting && victim->level >= 1 && victim->level <= MAX_LEVEL )
         {
            ch->printf( "Cannot hotboot at this time. There are still combats in progress.\r\n" );
            return;
         }
         if( victim->inflight )
         {
            ch->printf( "Cannot hotboot at this time. There are skyship flights in progress.\r\n" );
            return;
         }
      }

      if( d->connected == CON_EDITING && d->character )
      {
         ch->print( "Cannot hotboot at this time. Someone is using the line editor.\r\n" );
         return;
      }
   }

   log_printf( "Hotboot initiated by %s.", ch->name );

   FILE *fp = fopen( HOTBOOT_FILE, "w" );

   if( !fp )
   {
      ch->print( "Hotboot file not writeable, aborted.\r\n" );
      bug( "%s: Could not write to hotboot file: %s. Hotboot aborted.", __func__, HOTBOOT_FILE );
      perror( "do_copyover:fopen" );
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
         fprintf( fp, "%d %d %d %d %d %d %d %s %s %s\n",
                  d->descriptor, d->can_compress, d->is_compressing, d->msp_detected,
                  och->in_room->vnum, d->client_port, d->idle, och->name, d->hostname.c_str(  ), d->client.c_str(  ) );

         /*
          * One of two places this gets changed 
          */
         och->pcdata->hotboot = true;

         och->reset_sound(  );
         och->reset_music(  );
         och->save(  );
         if( !argument.empty(  ) && str_cmp( argument, "debug" ) )
         {
            d->write( "\r\nThe flow of time is halted momentarily as the world is reshaped!\r\n" );
            if( d->is_compressing )
               d->compressEnd(  );
         }
      }
   }

   fprintf( fp, "0 0 0 0 0 0 %d maxp maxp maxp\n", sysdata->maxplayers );
   fprintf( fp, "%s", "-1\n" );
   FCLOSE( fp );

   log_string( "Saving world time...." );
   save_timedata(  );   /* Preserve that up to the second calendar value :) */

#ifdef IMC
   imc_hotboot(  );
#endif

   if( !argument.empty(  ) && !str_cmp( argument, "debug" ) )
   {
      log_string( "Hotboot debug - Aborting before execl" );
      return;
   }

   log_string( "Executing hotboot...." );

   /*
    * exec - descriptors are inherited 
    * Yes, this is ugly, I know - Samson 
    */
   char buf[100], buf2[100], buf3[100];
   snprintf( buf, 100, "%d", mud_port );
   snprintf( buf2, 100, "%d", control );
#ifdef IMC
   if( this_imcmud )
      snprintf( buf3, 100, "%d", this_imcmud->desc );
   else
      mudstrlcpy( buf3, "-1", 100 );
#else
   mudstrlcpy( buf3, "-1", 100 );
#endif

   set_alarm( 0 );
#if !defined(__CYGWIN__) && defined(SQL)
   close_db(  );
#endif
   dlclose( sysdata->dlHandle );
   execl( EXE_FILE, "afkmud", buf, "hotboot", buf2, buf3, ( char * )nullptr );

   /*
    * Failed - sucessful exec will not return 
    */
   perror( "do_copyover: execl" );

   sysdata->dlHandle = dlopen( nullptr, RTLD_LAZY );
   if( !sysdata->dlHandle )
   {
      bug( "%s: FATAL ERROR: Unable to reopen system executable handle!", __func__ );
      exit( 1 );
   }
#if !defined(__CYGWIN__) && defined(SQL)
   init_mysql(  );
#endif
   bug( "%s: Hotboot execution failed!!", __func__ );
   ch->print( "Hotboot FAILED!\r\n" );
}

/* Recover from a hotboot - load players */
void hotboot_recover( void )
{
   FILE *fp;
   char name[100], host[MSL], client[MSL];
   int desc, dcompress, discompressing, room, dport, idle, dmsp, maxp = 0;
   bool fOld;
   int iError;

   if( !( fp = fopen( HOTBOOT_FILE, "r" ) ) )   /* there are some descriptors open which will hang forever then ? */
   {
      perror( "hotboot_recover: fopen" );
      bug( "%s: Hotboot file not found. Exitting.", __func__ );
      exit( 1 );
   }

   unlink( HOTBOOT_FILE ); /* In case something crashes - doesn't prevent reading */
   for( ;; )
   {
      iError = fscanf( fp, "%d %d %d %d %d %d %d %s %s %s\n", &desc, &dcompress, &discompressing, &dmsp, &room, &dport, &idle, name, host, client );

      if( desc == -1 || feof( fp ) )
         break;

      // Anything less than a full match gets tossed.
      if( iError < 10 )
         continue;

      if( !str_cmp( name, "maxp" ) || !str_cmp( host, "maxp" ) )
      {
         maxp = idle;
         continue;
      }

      if( desc == 0 )
         continue;

      /*
       * Write something, and check if it goes error-free 
       */
      if( !dcompress && !write_to_descriptor_old( desc, "\r\nThe ether swirls in chaos.\r\n" ) )
      {
         close( desc ); /* nope */
         continue;
      }

      if( desc == 0 )
      {
         bug( "%s: }RALERT! Assigning socket 0! BAD BAD BAD! Name: %s Host: %s", __func__, name, host );
         continue;
      }

      descriptor_data *d = new descriptor_data;
      d->init(  );
      d->connected = CON_PLOADED;
      d->descriptor = desc;

      d->hostname = host;
      d->client = client;
      d->client_port = dport;
      d->idle = idle;
      d->can_compress = dcompress;
      d->is_compressing = false;
      d->msp_detected = dmsp;

      dlist.push_back( d );
      d->connected = CON_COPYOVER_RECOVER;   /* negative so close_socket will cut them off */

      /*
       * Now, find the pfile 
       */
      fOld = load_char_obj( d, name, false, true );

      if( !fOld ) /* Player file not found?! */
      {
         d->write( "\r\nSomehow, your character was lost during hotboot. Contact the immortals ASAP.\r\n" );
         close_socket( d, false );
      }
      else  /* ok! */
      {
         d->write( "\r\nTime resumes its normal flow.\r\n" );
         d->character->in_room = get_room_index( room );
         if( !d->character->in_room )
            d->character->in_room = get_room_index( ROOM_VNUM_TEMPLE );

         /*
          * Insert in the char_list 
          */
         charlist.push_back( d->character );
         pclist.push_back( d->character );

         if( !d->character->to_room( d->character->in_room ) )
            log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __func__, __LINE__ );
         act( AT_MAGIC, "A puff of ethereal smoke dissipates around you!", d->character, nullptr, nullptr, TO_CHAR );
         act( AT_MAGIC, "$n appears in a puff of ethereal smoke!", d->character, nullptr, nullptr, TO_ROOM );
         d->connected = CON_PLAYING;

         d->character->pcdata->lasthost = d->hostname;

         if( d->can_compress )
         {
            if( !d->compressStart(  ) )
            {
               log_printf( "%s: Error restarting compression for %s on desc %d", __func__, name, desc );
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
            d->character->music( "wilderness.mid", 100, false );

         if( ++num_descriptors > sysdata->maxplayers )
            sysdata->maxplayers = num_descriptors;

         if( d->character->level < LEVEL_IMMORTAL )
            ++sysdata->playersonline;
         quotes( d->character );
         check_auth_state( d->character );   /* new auth */
      }
   }
   FCLOSE( fp );
   if( maxp > sysdata->maxplayers )
      sysdata->maxplayers = maxp;
   num_logins = maxp;
   log_string( "Hotboot recovery complete." );
}
