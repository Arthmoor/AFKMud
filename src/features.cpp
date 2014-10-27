/****************************************************************************
 *                   ^     +----- |  / ^     ^ |     | +-\                  *
 *                  / \    |      | /  |\   /| |     | |  \                 *
 *                 /   \   +---   |<   | \ / | |     | |  |                 *
 *                /-----\  |      | \  |  v  | |     | |  /                 *
 *               /       \ |      |  \ |     | +-----+ +-/                  *
 ****************************************************************************
 * AFKMud Copyright 1997-2014 by Roger Libiez (Samson),                     *
 * Levi Beckerson (Whir), Michael Ward (Tarl), Erik Wolfe (Dwip),           *
 * Cameron Carroll (Cam), Cyberfox, Karangi, Rathian, Raine,                *
 * Xorith, and Adjani.                                                      *
 * All Rights Reserved.                                                     *
 *                                                                          *
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
 *               Extended Mud Features Module: MCCP V2, MSP                 *
 ****************************************************************************/

/* Mud Sound Protocol Functions
 * Original author unknown.
 * Smaug port by Chris Coulter (aka Gabriel Androctus)
 */

/*
 * mccp.c - support functions for mccp (the Mud Client Compression Protocol)
 *
 * see http://mccp.smaugmuds.org
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

const unsigned char will_compress2_str[] = { IAC, WILL, TELOPT_COMPRESS2, '\0' };
const unsigned char start_compress2_str[] = { IAC, SB, TELOPT_COMPRESS2, IAC, SE, '\0' };
const unsigned char will_msp_str[] = { IAC, WILL, TELOPT_MSP, '\0' };
const unsigned char start_msp_str[] = { IAC, SB, TELOPT_MSP, IAC, SE, '\0' };

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

   this->write( (const char*)start_compress2_str );
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

   if( deflate( mccp->out_compress, Z_FINISH ) == Z_STREAM_END )
      process_compressed();   /* try to send any residual data */

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
   write_to_buffer( (const char*)start_msp_str );
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
void char_data::sound( const string & fname, int volume, bool toroom )
{
   const char *type = "mud";
   int repeats = 1, priority = 50;
   char url[MSL];

   if( sysdata->http.empty(  ) )
      return;

   snprintf( url, MSL, "%s/sounds/", sysdata->http.c_str(  ) );

   if( !toroom )
   {
      if( !MSP_ON(  ) )
         return;

      if( url[0] != '\0' )
         printf( "!!SOUND(%s V=%d L=%d P=%d T=%s U=%s)\r\n", fname.c_str(  ), volume, repeats, priority, type, url );
      else
         printf( "!!SOUND(%s V=%d L=%d P=%d T=%s)\r\n", fname.c_str(  ), volume, repeats, priority, type );
   }
   else
   {
      list < char_data * >::iterator ich;

      for( ich = in_room->people.begin(  ); ich != in_room->people.end(  ); ++ich )
      {
         char_data *vch = *ich;

         if( !vch->MSP_ON(  ) )
            continue;

         if( url[0] != '\0' )
            vch->printf( "!!SOUND(%s V=%d L=%d P=%d T=%s U=%s)\r\n", fname.c_str(  ), volume, repeats, priority, type, url );
         else
            vch->printf( "!!SOUND(%s V=%d L=%d P=%d T=%s)\r\n", fname.c_str(  ), volume, repeats, priority, type );
      }
   }
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
void char_data::music( const string & fname, int volume, bool toroom )
{
   const char *type = "mud";
   int repeats = 1, continu = 1;
   char url[MSL];

   if( sysdata->http.empty(  ) )
      return;

   snprintf( url, MSL, "%s/sounds/%s", sysdata->http.c_str(  ), fname.c_str(  ) );

   if( !toroom )
   {
      if( !MSP_ON(  ) )
         return;

      if( url[0] != '\0' )
         printf( "!!MUSIC(%s V=%d L=%d C=%d T=%s U=%s)\r\n", fname.c_str(  ), volume, repeats, continu, type, url );
      else
         printf( "!!MUSIC(%s V=%d L=%d C=%d T=%s)\r\n", fname.c_str(  ), volume, repeats, continu, type );
   }
   else
   {
      list < char_data * >::iterator ich;

      for( ich = in_room->people.begin(  ); ich != in_room->people.end(  ); ++ich )
      {
         char_data *vch = *ich;

         if( !vch->MSP_ON(  ) )
            return;

         if( url[0] != '\0' )
            vch->printf( "!!MUSIC(%s V=%d L=%d C=%d T=%s U=%s)\r\n", fname.c_str(  ), volume, repeats, continu, type, url );
         else
            vch->printf( "!!MUSIC(%s V=%d L=%d C=%d T=%s)\r\n", fname.c_str(  ), volume, repeats, continu, type );
      }
   }
}

/* stop playing sound */
void char_data::reset_sound(  )
{
   if( MSP_ON(  ) )
      print( "!!SOUND(Off)\r\n" );
}

/* stop playing music */
void char_data::reset_music(  )
{
   if( MSP_ON(  ) )
      print( "!!MUSIC(Off)\r\n" );
}

/* sound support -haus */
CMDF( do_mpsoundaround )
{
   string target, vol;
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

   if( vol.empty(  ) )
   {
      progbugf( ch, "%s", "Mpsoundaround - No volume level specified" );
      return;
   }

   if( !is_number( vol ) )
   {
      progbugf( ch, "%s", "mpsoundaround - non-numerical volume level specified" );
      return;
   }

   volume = atoi( vol.c_str(  ) );

   if( volume < 1 || volume > 100 )
   {
      progbugf( ch, "mpsoundaround - invalid volume %d. range is 1 to 100", volume );
      return;
   }

   if( argument.empty(  ) )
   {
      progbugf( ch, "%s", "mpsoundaround - no sound file specified" );
      return;
   }
   actflags = ch->get_actflags(  );
   ch->unset_actflag( ACT_SECRETIVE );
   victim->sound( argument, volume, true );
   ch->set_actflags( actflags );
}

/* prints message only to victim */
CMDF( do_mpsoundat )
{
   string target, vol;
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

   if( vol.empty(  ) )
   {
      progbugf( ch, "%s", "mpsoundat - No volume level specified" );
      return;
   }

   if( !is_number( vol ) )
   {
      progbugf( ch, "%s", "mpsoundat - non-numerical volume level specified" );
      return;
   }

   volume = atoi( vol.c_str(  ) );

   if( volume < 1 || volume > 100 )
   {
      progbugf( ch, "mpsoundat - invalid volume %d. range is 1 to 100", volume );
      return;
   }

   if( argument.empty(  ) )
   {
      progbugf( ch, "%s", "mpsoundat - no sound file specified" );
      return;
   }
   actflags = ch->get_actflags(  );
   ch->unset_actflag( ACT_SECRETIVE );
   victim->sound( argument, volume, false );
   ch->set_actflags( actflags );
}

/* prints message to room at large. */
CMDF( do_mpsound )
{
   string vol;
   int volume;
   bitset < MAX_ACT_FLAG > actflags;

   if( !ch->isnpc(  ) || ch->has_aflag( AFF_CHARM ) )
   {
      ch->print( "Huh?\r\n" );
      return;
   }

   argument = one_argument( argument, vol );

   if( vol.empty(  ) )
   {
      progbugf( ch, "%s", "mpsound - No volume level specified" );
      return;
   }

   if( !is_number( vol ) )
   {
      progbugf( ch, "%s", "mpsound - non-numerical volume level specified" );
      return;
   }

   volume = atoi( vol.c_str(  ) );

   if( volume < 1 || volume > 100 )
   {
      progbugf( ch, "mpsound - invalid volume %d. range is 1 to 100", volume );
      return;
   }

   if( argument.empty(  ) )
   {
      progbugf( ch, "%s", "mpsound - no sound file specified" );
      return;
   }
   actflags = ch->get_actflags(  );
   ch->unset_actflag( ACT_SECRETIVE );
   ch->sound( argument, volume, true );
   ch->set_actflags( actflags );
}

/* prints sound to everyone in the zone - yes, this COULD get rather bad if people go nuts with it */
CMDF( do_mpsoundzone )
{
   string vol;

   if( !ch->isnpc(  ) || ch->has_aflag( AFF_CHARM ) )
   {
      ch->print( "Huh?\r\n" );
      return;
   }

   argument = one_argument( argument, vol );

   if( vol.empty(  ) )
   {
      progbugf( ch, "%s", "mpsoundzone - No volume level specified" );
      return;
   }

   if( !is_number( vol ) )
   {
      progbugf( ch, "%s", "mpsoundzone - non-numerical volume level specified" );
      return;
   }

   int volume = atoi( vol.c_str(  ) );

   if( volume < 1 || volume > 100 )
   {
      progbugf( ch, "mpsoundzone - invalid volume %d. range is 1 to 100", volume );
      return;
   }

   if( argument.empty(  ) )
   {
      progbugf( ch, "%s", "mpsoundzone - no sound file specified" );
      return;
   }

   bitset < MAX_ACT_FLAG > actflags = ch->get_actflags(  );
   ch->unset_actflag( ACT_SECRETIVE );

   list < char_data * >::iterator ich;
   for( ich = charlist.begin(  ); ich != charlist.end(  ); ++ich )
   {
      char_data *vch = *ich;

      if( vch->in_room && vch->in_room->area == ch->in_room->area )
         vch->sound( argument, volume, false );
   }
   ch->set_actflags( actflags );
}

/* Music stuff, same as above, at zMUD coders' request -- Blodkai */
CMDF( do_mpmusicaround )
{
   string target, vol;
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

   if( vol.empty(  ) )
   {
      progbugf( ch, "%s", "mpmusicaround - No volume level specified" );
      return;
   }

   if( !is_number( vol ) )
   {
      progbugf( ch, "%s", "mpmusicaround - non-numerical volume level specified" );
      return;
   }

   volume = atoi( vol.c_str(  ) );

   if( volume < 1 || volume > 100 )
   {
      progbugf( ch, "mpmusicaround - invalid volume %d. range is 1 to 100", volume );
      return;
   }

   if( argument.empty(  ) )
   {
      progbugf( ch, "%s", "mpmusicaround - no sound file specified" );
      return;
   }

   actflags = ch->get_actflags(  );
   ch->unset_actflag( ACT_SECRETIVE );
   victim->music( argument, volume, true );
   ch->set_actflags( actflags );
}

CMDF( do_mpmusic )
{
   string target, vol;
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

   if( vol.empty(  ) )
   {
      progbugf( ch, "%s", "mpmusic - No volume level specified" );
      return;
   }

   if( !is_number( vol ) )
   {
      progbugf( ch, "%s", "mpmusic - non-numerical volume level specified" );
      return;
   }

   volume = atoi( vol.c_str(  ) );

   if( volume < 1 || volume > 100 )
   {
      progbugf( ch, "mpmusic - invalid volume %d. range is 1 to 100", volume );
      return;
   }

   if( argument.empty(  ) )
   {
      progbugf( ch, "%s", "mpmusic - no sound file specified" );
      return;
   }
   actflags = ch->get_actflags(  );
   ch->unset_actflag( ACT_SECRETIVE );
   victim->music( argument, volume, true );
   ch->set_actflags( actflags );
}

CMDF( do_mpmusicat )
{
   string target, vol;
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

   if( vol.empty(  ) )
   {
      progbugf( ch, "%s", "mpmusicat - No volume level specified" );
      return;
   }

   if( !is_number( vol ) )
   {
      progbugf( ch, "%s", "mpmusicat - non-numerical volume level specified" );
      return;
   }

   volume = atoi( vol.c_str(  ) );

   if( volume < 1 || volume > 100 )
   {
      progbugf( ch, "mpmusicat - invalid volume %d. range is 1 to 100", volume );
      return;
   }

   if( argument.empty(  ) )
   {
      progbugf( ch, "%s", "mpmusicat - no sound file specified" );
      return;
   }
   actflags = ch->get_actflags(  );
   ch->unset_actflag( ACT_SECRETIVE );
   victim->music( argument, volume, false );
   ch->set_actflags( actflags );
}

/* End MSP Information */
