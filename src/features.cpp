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
 *             Extended Mud Features Module: MCCP V2, MSP, MXP              *
 ****************************************************************************/

/*
 * Ported to SMAUG by Garil of DOTDII Mud
 * aka Jesse DeFer <dotd@primenet.com>
 * See the web site below for more info.
 */

/* Mud Sound Protocol Functions
 * Original author unknown.
 * Smaug port by Chris Coulter (aka Gabriel Androctus)
 */

/* Mud eXtension Protocol 01/06/2001 Garil@DOTDII aka Jesse DeFer dotd@dotd.com */

/*
 * mccp.c - support functions for mccp (the Mud Client Compression Protocol)
 *
 * see http://mccp.afkmud.com
 *
 * Copyright (c) 1999, Oliver Jowett <oliver@randomly.org>.
 *
 * This code may be freely distributed and used if this copyright notice is
 * retained intact.
 */

#include <cerrno>
#if !defined(WIN32)
#include <arpa/telnet.h>
#include <sys/socket.h>
#else
#include <unistd.h>
#define  TELOPT_ECHO        '\x01'
#define  GA                 '\xF9'
#define  SB                 '\xFA'
#define  SE                 '\xF0'
#define  WILL               '\xFB'
#define  WONT               '\xFC'
#define  DO                 '\xFD'
#define  DONT               '\xFE'
#define  IAC                '\xFF'
const int ENOSR = 63;
#endif
#include "mud.h"
#include "descriptor.h"
#include "roomindex.h"

#if defined(__OpenBSD__) || defined(__FreeBSD__)
const int ENOSR = 63;
#endif

char will_compress2_str[] = { IAC, WILL, TELOPT_COMPRESS2, '\0' };
char start_compress2_str[] = { IAC, SB, TELOPT_COMPRESS2, IAC, SE, '\0' };
char will_msp_str[] = { IAC, WILL, TELOPT_MSP, '\0' };
char start_msp_str[] = { IAC, SB, TELOPT_MSP, IAC, SE, '\0' };
char will_mxp_str[] = { IAC, WILL, TELOPT_MXP, '\0' };
char start_mxp_str[] = { IAC, SB, TELOPT_MXP, IAC, SE, '\0' };
char *mxp_obj_cmd[MAX_ITEM_TYPE][MAX_MXPOBJ];

/* Begin MCCP Information */

bool descriptor_data::process_compressed(  )
{
   int iStart = 0, nBlock, nWrite, len;

   if( !mccp->out_compress )
      return true;

   /*
    * Try to write out some data.. 
    */
   len = mccp->out_compress->next_out - mccp->out_compress_buf;

   if( len > 0 )
   {
      /*
       * we have some data to write 
       */
      for( iStart = 0; iStart < len; iStart += nWrite )
      {
         nBlock = UMIN( len - iStart, 4096 );
         if( ( nWrite = send( descriptor, mccp->out_compress_buf + iStart, nBlock, 0 ) ) < 0 )
         {
            if( errno == EAGAIN || errno == ENOSR )
               break;
            return false;
         }

         if( !nWrite )
            break;
      }

      if( iStart )
      {
         /*
          * We wrote "iStart" bytes 
          */
         if( iStart < len )
            memmove( mccp->out_compress_buf, mccp->out_compress_buf + iStart, len - iStart );

         mccp->out_compress->next_out = mccp->out_compress_buf + len - iStart;
      }
   }
   return true;
}

bool descriptor_data::compressStart(  )
{
   z_stream *s;

   if( mccp->out_compress || is_compressing )
      return true;

   CREATE( s, z_stream, 1 );
   CREATE( mccp->out_compress_buf, unsigned char, COMPRESS_BUF_SIZE );

   s->next_in = NULL;
   s->avail_in = 0;

   s->next_out = mccp->out_compress_buf;
   s->avail_out = COMPRESS_BUF_SIZE;

   s->zalloc = Z_NULL;
   s->zfree = Z_NULL;
   s->opaque = NULL;

   if( deflateInit( s, 9 ) != Z_OK )
   {
      DISPOSE( mccp->out_compress_buf );
      DISPOSE( s );
      return false;
   }

   this->write( start_compress2_str, 0 );
   mccp->out_compress = s;
   is_compressing = true;
   return true;
}

bool descriptor_data::compressEnd(  )
{
   unsigned char dummy[1];

   if( !mccp->out_compress || !is_compressing )
      return true;

   mccp->out_compress->avail_in = 0;
   mccp->out_compress->next_in = dummy;

   if( deflate( mccp->out_compress, Z_FINISH ) != Z_STREAM_END )
      return false;

   if( !process_compressed(  ) ) /* try to send any residual data */
      return false;

   deflateEnd( mccp->out_compress );
   DISPOSE( mccp->out_compress_buf );
   DISPOSE( mccp->out_compress );
   is_compressing = false;
   return true;
}

CMDF( do_compress )
{
   if( !ch->desc )
   {
      ch->print( "What descriptor?!\r\n" );
      return;
   }

   if( !ch->desc->mccp->out_compress )
   {
      if( !ch->desc->compressStart(  ) )
      {
         ch->desc->can_compress = false;
         ch->print( "&RCompression failed to start.\r\n" );
      }
      else
      {
         ch->desc->can_compress = true;
         ch->print( "&GOk, compression enabled.\r\n" );
      }
   }
   else
   {
      ch->desc->compressEnd(  );
      ch->desc->can_compress = false;
      ch->print( "&ROk, compression disabled.\r\n" );
   }
}

/* End MCCP Information */
/* Start MSP Information */

void descriptor_data::send_msp_startup(  )
{
   write_to_buffer( start_msp_str, 0 );
}

/* Trigger sound to character
 *
 * "fname" is the name of the sound file to be played
 * "vol" is the volume level to play the sound at    
 * "repeats" is the number of times to play the sound
 * "priority" is the priority of the sound
 * "type" is the sound Class 
 * "URL" is the optional download URL for the sound file
 * "ch" is the character to play the sound for
 *
 * More detailed information at http://www.zuggsoft.com/
*/
void char_data::sound( const char *fname, int volume, bool toroom )
{
   char *type = "mud";
   int repeats = 1, priority = 50;
   char url[MSL];

   if( !sysdata->http || sysdata->http[0] == '\0' )
      return;

   snprintf( url, MSL, "%s/sounds/", sysdata->http );

   if( !toroom )
   {
      if( !MSP_ON(  ) )
         return;

      if( url[0] != '\0' )
      {
         printf( "!!SOUND(%s V=%d L=%d P=%d T=%s U=%s)\r\n", fname, volume, repeats, priority, type, url );
      }
      else
      {
         printf( "!!SOUND(%s V=%d L=%d P=%d T=%s)\r\n", fname, volume, repeats, priority, type );
      }
   }
   else
   {
      list<char_data*>::iterator ich;

      for( ich = in_room->people.begin(); ich != in_room->people.end(); ++ich )
      {
         char_data *vch = (*ich);

         if( !vch->MSP_ON(  ) )
            continue;

         if( url[0] != '\0' )
            vch->printf( "!!SOUND(%s V=%d L=%d P=%d T=%s U=%s)\r\n", fname, volume, repeats, priority, type, url );
         else
            vch->printf( "!!SOUND(%s V=%d L=%d P=%d T=%s)\r\n", fname, volume, repeats, priority, type );
      }
   }
   return;
}

/* Trigger music file ...
 *
 * "fname" is the name of the music file to be played
 * "vol" is the volume level to play the music at    
 * "repeats" is the number of times to play the music file
 * "continu" specifies whether the file should be restarted if requested again
 * "type" is the sound Class 
 * "URL" is the optional download URL for the sound file
 * "ch" is the character to play the music for 
 *
 * more detailed information at: http://www.zuggsoft.com/
*/
void char_data::music( const char *fname, int volume, bool toroom )
{
   char *type = "mud";
   int repeats = 1, continu = 1;
   char url[MSL];

   if( !sysdata->http || sysdata->http[0] == '\0' )
      return;

   snprintf( url, MSL, "%s/sounds/%s", sysdata->http, fname );

   if( !toroom )
   {
      if( !MSP_ON(  ) )
         return;

      if( url && url[0] != '\0' )
         printf( "!!MUSIC(%s V=%d L=%d C=%d T=%s U=%s)\r\n", fname, volume, repeats, continu, type, url );
      else
         printf( "!!MUSIC(%s V=%d L=%d C=%d T=%s)\r\n", fname, volume, repeats, continu, type );
   }
   else
   {
      list<char_data*>::iterator ich;

      for( ich = in_room->people.begin(); ich != in_room->people.end(); ++ich )
      {
         char_data *vch = (*ich);

         if( !vch->MSP_ON(  ) )
            return;

         if( url && url[0] != '\0' )
            vch->printf( "!!MUSIC(%s V=%d L=%d C=%d T=%s U=%s)\r\n", fname, volume, repeats, continu, type, url );
         else
            vch->printf( "!!MUSIC(%s V=%d L=%d C=%d T=%s)\r\n", fname, volume, repeats, continu, type );
      }
   }
   return;
}

/* stop playing sound */
void char_data::reset_sound(  )
{
   if( MSP_ON(  ) )
      print( "!!SOUND(Off)\r\n" );

   return;
}

/* stop playing music */
void char_data::reset_music(  )
{
   if( MSP_ON(  ) )
      print( "!!MUSIC(Off)\r\n" );

   return;
}

/* sound support -haus */
/* Tied into the code in features.c by Samson */
CMDF( do_mpsoundaround )
{
   char target[MIL], vol[MIL];
   int volume;
   char_data *victim;
   bitset < MAX_ACT_FLAG > actflags;

   if( !ch->isnpc(  ) || ch->has_aflag( AFF_CHARM ) )
   {
      ch->print( "Huh?\r\n" );
      return;
   }

   argument = one_argument( argument, target );

   if( !( victim = ch->get_char_room( target ) ) )
   {
      progbugf( ch, "%s", "Mpsoundaround - No victim specified" );
      return;
   }

   argument = one_argument( argument, vol );

   if( !vol || vol[0] == '\0' )
   {
      progbugf( ch, "%s", "Mpsoundaround - No volume level specified" );
      return;
   }

   if( !is_number( vol ) )
   {
      progbugf( ch, "%s", "mpsoundaround - non-numerical volume level specified" );
      return;
   }

   volume = atoi( vol );

   if( volume < 1 || volume > 100 )
   {
      progbugf( ch, "mpsoundaround - invalid volume %d. range is 1 to 100", volume );
      return;
   }

   if( !argument || argument[0] == '\0' )
   {
      progbugf( ch, "%s", "mpsoundaround - no sound file specified" );
      return;
   }
   actflags = ch->get_actflags();
   ch->unset_actflag( ACT_SECRETIVE );
   victim->sound( argument, volume, true );
   ch->set_actflags( actflags );
}

/* prints message only to victim */
/* Tied into the code in msp.c by Samson */
CMDF( do_mpsoundat )
{
   char target[MIL], vol[MIL];
   int volume;
   char_data *victim;
   bitset < MAX_ACT_FLAG > actflags;

   if( !ch->isnpc(  ) || ch->has_aflag( AFF_CHARM ) )
   {
      ch->print( "Huh?\r\n" );
      return;
   }

   argument = one_argument( argument, target );

   if( !( victim = ch->get_char_room( target ) ) )
   {
      progbugf( ch, "%s", "mpsoundat - No victim specified" );
      return;
   }

   argument = one_argument( argument, vol );

   if( !vol || vol[0] == '\0' )
   {
      progbugf( ch, "%s", "mpsoundat - No volume level specified" );
      return;
   }

   if( !is_number( vol ) )
   {
      progbugf( ch, "%s", "mpsoundat - non-numerical volume level specified" );
      return;
   }

   volume = atoi( vol );

   if( volume < 1 || volume > 100 )
   {
      progbugf( ch, "mpsoundat - invalid volume %d. range is 1 to 100", volume );
      return;
   }

   if( !argument || argument[0] == '\0' )
   {
      progbugf( ch, "%s", "mpsoundat - no sound file specified" );
      return;
   }
   actflags = ch->get_actflags();
   ch->unset_actflag( ACT_SECRETIVE );
   victim->sound( argument, volume, false );
   ch->set_actflags( actflags );
}

/* prints message to room at large. */
/* Tied into the code in msp.c by Samson */
CMDF( do_mpsound )
{
   char vol[MIL];
   int volume;
   bitset < MAX_ACT_FLAG > actflags;

   if( !ch->isnpc(  ) || ch->has_aflag( AFF_CHARM ) )
   {
      ch->print( "Huh?\r\n" );
      return;
   }

   argument = one_argument( argument, vol );

   if( !vol || vol[0] == '\0' )
   {
      progbugf( ch, "%s", "mpsound - No volume level specified" );
      return;
   }

   if( !is_number( vol ) )
   {
      progbugf( ch, "%s", "mpsound - non-numerical volume level specified" );
      return;
   }

   volume = atoi( vol );

   if( volume < 1 || volume > 100 )
   {
      progbugf( ch, "mpsound - invalid volume %d. range is 1 to 100", volume );
      return;
   }

   if( !argument || argument[0] == '\0' )
   {
      progbugf( ch, "%s", "mpsound - no sound file specified" );
      return;
   }
   actflags = ch->get_actflags();
   ch->unset_actflag( ACT_SECRETIVE );
   ch->sound( argument, volume, true );
   ch->set_actflags( actflags );
}

/* prints sound to everyone in the zone - yes, this COULD get rather bad if people go nuts with it */
/* Tied into the code in msp.c by Samson */
CMDF( do_mpsoundzone )
{
   if( !ch->isnpc(  ) || ch->has_aflag( AFF_CHARM ) )
   {
      ch->print( "Huh?\r\n" );
      return;
   }

   char vol[MIL];
   argument = one_argument( argument, vol );

   if( !vol || vol[0] == '\0' )
   {
      progbugf( ch, "%s", "mpsoundzone - No volume level specified" );
      return;
   }

   if( !is_number( vol ) )
   {
      progbugf( ch, "%s", "mpsoundzone - non-numerical volume level specified" );
      return;
   }

   int volume = atoi( vol );

   if( volume < 1 || volume > 100 )
   {
      progbugf( ch, "mpsoundzone - invalid volume %d. range is 1 to 100", volume );
      return;
   }

   if( !argument || argument[0] == '\0' )
   {
      progbugf( ch, "%s", "mpsoundzone - no sound file specified" );
      return;
   }

   bitset<MAX_ACT_FLAG> actflags = ch->get_actflags();
   ch->unset_actflag( ACT_SECRETIVE );

   list<char_data*>::iterator ich;
   for( ich = charlist.begin(); ich != charlist.end(); ++ich )
   {
      char_data *vch = (*ich);

      if( vch->in_room && vch->in_room->area == ch->in_room->area )
         vch->sound( argument, volume, false );
   }
   ch->set_actflags( actflags );
}

/* Music stuff, same as above, at zMUD coders' request -- Blodkai */
/* Tied into the code in msp.c by Samson */
CMDF( do_mpmusicaround )
{
   char target[MIL], vol[MIL];
   int volume;
   char_data *victim;
   bitset < MAX_ACT_FLAG > actflags;

   if( !ch->isnpc(  ) || ch->has_aflag( AFF_CHARM ) )
   {
      ch->print( "Huh?\r\n" );
      return;
   }

   argument = one_argument( argument, target );

   if( !( victim = ch->get_char_room( target ) ) )
   {
      progbugf( ch, "%s", "mpmusicaround - No victim specified" );
      return;
   }

   argument = one_argument( argument, vol );

   if( !vol || vol[0] == '\0' )
   {
      progbugf( ch, "%s", "mpmusicaround - No volume level specified" );
      return;
   }

   if( !is_number( vol ) )
   {
      progbugf( ch, "%s", "mpmusicaround - non-numerical volume level specified" );
      return;
   }

   volume = atoi( vol );

   if( volume < 1 || volume > 100 )
   {
      progbugf( ch, "mpmusicaround - invalid volume %d. range is 1 to 100", volume );
      return;
   }

   if( !argument || argument[0] == '\0' )
   {
      progbugf( ch, "%s", "mpmusicaround - no sound file specified" );
      return;
   }

   actflags = ch->get_actflags();
   ch->unset_actflag( ACT_SECRETIVE );
   victim->music( argument, volume, true );
   ch->set_actflags( actflags );
   return;
}

/* Tied into the code in msp.c by Samson */
CMDF( do_mpmusic )
{
   char target[MIL], vol[MIL];
   int volume;
   char_data *victim;
   bitset < MAX_ACT_FLAG > actflags;

   if( !ch->isnpc(  ) || ch->has_aflag( AFF_CHARM ) )
   {
      ch->print( "Huh?\r\n" );
      return;
   }

   argument = one_argument( argument, target );

   if( !( victim = ch->get_char_room( target ) ) )
   {
      progbugf( ch, "%s", "mpmusic - No victim specified" );
      return;
   }

   argument = one_argument( argument, vol );

   if( !vol || vol[0] == '\0' )
   {
      progbugf( ch, "%s", "mpmusic - No volume level specified" );
      return;
   }

   if( !is_number( vol ) )
   {
      progbugf( ch, "%s", "mpmusic - non-numerical volume level specified" );
      return;
   }

   volume = atoi( vol );

   if( volume < 1 || volume > 100 )
   {
      progbugf( ch, "mpmusic - invalid volume %d. range is 1 to 100", volume );
      return;
   }

   if( !argument || argument[0] == '\0' )
   {
      progbugf( ch, "%s", "mpmusic - no sound file specified" );
      return;
   }
   actflags = ch->get_actflags();
   ch->unset_actflag( ACT_SECRETIVE );
   victim->music( argument, volume, true );
   ch->set_actflags( actflags );
   return;
}

/* Tied into the code in msp.c by Samson */
CMDF( do_mpmusicat )
{
   char target[MIL], vol[MIL];
   int volume;
   char_data *victim;
   bitset < MAX_ACT_FLAG > actflags;

   if( !ch->isnpc(  ) || ch->has_aflag( AFF_CHARM ) )
   {
      ch->print( "Huh?\r\n" );
      return;
   }

   argument = one_argument( argument, target );

   if( !( victim = ch->get_char_room( target ) ) )
   {
      progbugf( ch, "%s", "mpmusicat - No victim specified" );
      return;
   }

   argument = one_argument( argument, vol );

   if( !vol || vol[0] == '\0' )
   {
      progbugf( ch, "%s", "mpmusicat - No volume level specified" );
      return;
   }

   if( !is_number( vol ) )
   {
      progbugf( ch, "%s", "mpmusicat - non-numerical volume level specified" );
      return;
   }

   volume = atoi( vol );

   if( volume < 1 || volume > 100 )
   {
      progbugf( ch, "mpmusicat - invalid volume %d. range is 1 to 100", volume );
      return;
   }

   if( !argument || argument[0] == '\0' )
   {
      progbugf( ch, "%s", "mpmusicat - no sound file specified" );
      return;
   }
   actflags = ch->get_actflags();
   ch->unset_actflag( ACT_SECRETIVE );
   victim->music( argument, volume, false );
   ch->set_actflags( actflags );
   return;
}

/* End MSP Information */
/* Start MXP Information */

void free_mxpobj_cmds( void )
{
   int hash, loopa;

   for( hash = 0; hash < MAX_ITEM_TYPE; ++hash )
   {
      for( loopa = 0; loopa < MAX_MXPOBJ; ++loopa )
      {
         if( mxp_obj_cmd[hash][loopa] != NULL )
            STRFREE( mxp_obj_cmd[hash][loopa] );
      }
   }
   return;
}

void load_mxpobj_cmds( void )
{
   FILE *fp;
   char filename[256];
   char *temp;
   int x, y, num = -1;

   for( x = 0; x < MAX_ITEM_TYPE; ++x )
   {
      for( y = 0; y < MAX_MXPOBJ; ++y )
         mxp_obj_cmd[x][y] = NULL;
   }

   snprintf( filename, 256, "%smxpobjcmds.dat", SYSTEM_DIR );

   if( !( fp = fopen( filename, "r" ) ) )
   {
      bug( "%s: Object commands file not found.", __FUNCTION__ );
      return;
   }

   for( ;; )
   {
      temp = fread_word( fp );

      if( feof( fp ) )
      {
         bug( "%s: Premature end of file.", __FUNCTION__ );
         break;
      }

      if( !str_cmp( temp, "#END" ) )
         break;

      if( str_cmp( temp, "#MXPOBJCMD" ) )
      {
         bug( "%s: Improper command list format.", __FUNCTION__ );
         break;
      }

      num = get_otype( fread_flagstring( fp ) );

      if( num < 0 || num >= MAX_ITEM_TYPE )
      {
         bug( "%s: Bad item type in file.", __FUNCTION__ );
         break;
      }

      mxp_obj_cmd[num][MXP_GROUND] = fread_string( fp );
      mxp_obj_cmd[num][MXP_INV] = fread_string( fp );
      mxp_obj_cmd[num][MXP_EQ] = fread_string( fp );
      mxp_obj_cmd[num][MXP_SHOP] = STRALLOC( "buy" );
      mxp_obj_cmd[num][MXP_STEAL] = STRALLOC( "steal" );
      mxp_obj_cmd[num][MXP_CONT] = STRALLOC( "get" );
   }
   FCLOSE( fp );
   return;
}

char *spacetodash( char *argument )
{
   static char retbuf[64];
   unsigned int x;

   mudstrlcpy( retbuf, argument, 64 );

   for( x = 0; x < strlen( retbuf ); ++x )
      if( retbuf[x] == ' ' )
         retbuf[x] = '-';

   return retbuf;
}

/*
 * Pick off one argument from a string and return the rest.
 */
char *one_argumentx( char *argument, char *arg_first, char cEnd )
{
   short count;

   count = 0;

   while( isspace( *argument ) )
      ++argument;

   while( *argument != '\0' || ++count >= 255 )
   {
      if( *argument == cEnd )
      {
         ++argument;
         break;
      }
      *arg_first = LOWER( *argument );
      ++arg_first;
      ++argument;
   }
   *arg_first = '\0';

   while( isspace( *argument ) )
      ++argument;

   return argument;
}

void descriptor_data::send_mxp_stylesheet(  )
{
   FILE *rpfile;
   int num = 0;
   char BUFF[MSL * 2];

   write_to_buffer( start_mxp_str, 0 );

   if( ( rpfile = fopen( MXP_SS_FILE, "r" ) ) != NULL )
   {
      while( ( BUFF[num] = fgetc( rpfile ) ) != EOF )
         ++num;
      FCLOSE( rpfile );
      BUFF[num] = 0;
      write_to_buffer( BUFF, num );
   }
}

char *mxp_obj_str( char_data * ch, obj_data * obj, int mxpmode, char *mxptail )
{
   static char mxpbuf[MIL];
   char *argument, *argument2;
   char arg[MIL], arg2[MIL];

   if( !ch->MXP_ON(  ) || mxpmode == MXP_NONE )
      return "";

   argument = mxp_obj_cmd[obj->item_type][mxpmode];
   argument2 = mxp_obj_cmd[obj->item_type][mxpmode];

   if( !argument || argument[0] == '\0' || !str_cmp( argument, "invalid!" ) 
    || !argument2 || argument2[0] == '\0' || !str_cmp( argument, "invalid!" ) )
      return "";

   if( mxptail && str_cmp( mxptail, "" ) )
      mxptail = spacetodash( mxptail );

   if( !strchr( argument, '|' ) )
   {
      snprintf( mxpbuf, MIL, MXP_TAG_SECURE "<send \"%s %s %s\" hint=\"%s %s %s\">",
                argument, spacetodash( obj->name ), mxptail, argument, obj->short_descr, mxptail );
      return mxpbuf;
   }

   snprintf( mxpbuf, MIL, "%s", MXP_TAG_SECURE "¢send href=\"" );
   while( *argument && ( argument = one_argumentx( argument, arg, '|' ) ) )
      snprintf( mxpbuf + strlen( mxpbuf ), MIL - strlen( mxpbuf ), "%s %s %s|", arg, spacetodash( obj->name ), mxptail );

   snprintf( mxpbuf + ( strlen( mxpbuf ) - 1 ), MIL - ( strlen( mxpbuf ) - 1 ), "\" hint=\"Right-click for menu|%s",
             mxptail );

   while( *argument2 && ( argument2 = one_argumentx( argument2, arg2, '|' ) ) )
      snprintf( mxpbuf + strlen( mxpbuf ), MIL - strlen( mxpbuf ), "%s %s %s|", arg2, obj->short_descr, mxptail );

   snprintf( mxpbuf + ( strlen( mxpbuf ) - 1 ), MIL - ( strlen( mxpbuf ) - 1 ), "\"%s£", mxptail );

   return mxpbuf;
}

char *mxp_obj_str_close( char_data * ch, obj_data * obj, int mxpmode )
{
   char *argument = NULL, *argument2 = NULL;
   static char mxpbuf[MIL];

   if( !ch->MXP_ON(  ) || mxpmode == MXP_NONE )
      return "";

   argument = mxp_obj_cmd[obj->item_type][mxpmode];
   argument2 = mxp_obj_cmd[obj->item_type][mxpmode];

   if( !argument || argument[0] == '\0' || !str_cmp( argument, "invalid!" ) 
    || !argument2 || argument2[0] == '\0' || !str_cmp( argument, "invalid!" ) )
      return "";

   if( !strchr( argument, '|' ) )
      mudstrlcpy( mxpbuf, "</send>" MXP_TAG_LOCKED, MIL );
   else
      mudstrlcpy( mxpbuf, "¢/send£" MXP_TAG_LOCKED, MIL );
   return mxpbuf;
}

char *mxp_chan_str( char_data * ch, const char *verb )
{
   static char mxpbuf[MIL];

   if( !ch->MXP_ON(  ) || verb[0] == '\0' )
      return "";

   snprintf( mxpbuf, MIL, MXP_TAG_SECURE "¢%s£", verb );

   return mxpbuf;
}

char *mxp_chan_str_close( char_data * ch, const char *verb )
{
   static char mxpbuf[MIL];

   if( !ch->MXP_ON(  ) || verb[0] == '\0' )
      return "";

   snprintf( mxpbuf, MIL, "¢/%s£" MXP_TAG_LOCKED, verb );

   return mxpbuf;
}

/* End MXP Information */
