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
 *               Extended Mud Features Module: MCCP V2, MSP                 *
 ****************************************************************************/

/*
 * Mud Sound Protocol Functions
 * Original author unknown.
 * Smaug port by Chris Coulter (aka Gabriel Androctus)
 */

/*
 * mccp.c - support functions for mccp (the Mud Client Compression Protocol)
 *
 * see https://smaugmuds.afkmods.com/mccp/
 *
 * Copyright (c) 1999, Oliver Jowett <oliver@randomly.org>.
 *
 * This code may be freely distributed and used if this copyright notice is
 * retained intact.
 */

#include <arpa/telnet.h>
#include <sys/socket.h>
#include "mud.h"
#include "descriptor.h"
#include "mud_prog.h"
#include "roomindex.h"

const std::array<unsigned char, 4> will_compress2_str = { IAC, WILL, TELOPT_COMPRESS2, '\0' };
const std::array<unsigned char, 6> start_compress2_str = { IAC, SB, TELOPT_COMPRESS2, IAC, SE, '\0' };
const std::array<unsigned char, 4> will_msp_str = { IAC, WILL, TELOPT_MSP, '\0' };
const std::array<unsigned char, 6> start_msp_str = { IAC, SB, TELOPT_MSP, IAC, SE, '\0' };

// Begin MCCP Information
bool descriptor_data::process_compressed( )
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
         nBlock = umin( len - iStart, 4096 );

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
            std::memmove( mccp->out_compress_buf, mccp->out_compress_buf + iStart, len - iStart );

         mccp->out_compress->next_out = mccp->out_compress_buf + len - iStart;
      }
   }
   return true;
}

bool descriptor_data::compressStart( )
{
   if( mccp->out_compress || is_compressing )
      return true;

   auto s = std::make_unique<z_stream>();
   auto out_compress_buf = std::make_unique<unsigned char[]>( COMPRESS_BUF_SIZE );

   s->next_in = nullptr;
   s->avail_in = 0;

   s->next_out = out_compress_buf.get();
   s->avail_out = COMPRESS_BUF_SIZE;

   s->zalloc = Z_NULL;
   s->zfree = Z_NULL;
   s->opaque = nullptr;

   if( deflateInit( s.get(), 9 ) != Z_OK )
      return false;

   this->write( std::string_view{ reinterpret_cast<const char*>(start_compress2_str.data()), start_compress2_str.size() } );

   mccp->out_compress = s.release();
   mccp->out_compress_buf = out_compress_buf.release();

   is_compressing = true;
   return true;
}

bool descriptor_data::compressEnd( )
{
   unsigned char dummy[1];

   if( !mccp->out_compress || !is_compressing )
      return true;

   mccp->out_compress->avail_in = 0;
   mccp->out_compress->next_in = dummy;

   if( deflate( mccp->out_compress, Z_FINISH ) == Z_STREAM_END )
      process_compressed();   /* try to send any residual data */

   deflateEnd( mccp->out_compress );

   delete[] mccp->out_compress_buf;
   mccp->out_compress_buf = nullptr;

   delete mccp->out_compress;
   mccp->out_compress = nullptr;

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
// End MCCP Information

// Start MSP Information
void descriptor_data::send_msp_startup(  )
{
   write_to_buffer( std::string_view{ reinterpret_cast<const char*>( start_msp_str.data() ), start_msp_str.size() } );
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
 * More detailed information at https://www.zuggsoft.com/zmud/msp.htm
*/
void char_data::sound( std::string_view fname, int volume, bool toroom )
{
   const char *type = "mud";
   int repeats = 1, priority = 50;

   if( sysdata->http.empty(  ) )
      return;

   std::string url = std::format( "{}/sounds/{}", sysdata->http, fname );

   if( !toroom )
   {
      if( !MSP_ON(  ) )
         return;

      if( !url.empty() )
         print_fmt( "!!SOUND({} V={} L={} P={} T={} U={})\r\n", fname, volume, repeats, priority, type, url );
      else
         print_fmt( "!!SOUND({} V={} L={} P={} T={})\r\n", fname, volume, repeats, priority, type );
   }
   else
   {
      for( auto* vch : in_room->people )
      {
         if( !vch->MSP_ON(  ) )
            continue;

         if( !url.empty() )
            vch->print_fmt( "!!SOUND({} V={} L={} P={} T={} U={})\r\n", fname, volume, repeats, priority, type, url );
         else
            vch->print_fmt( "!!SOUND({} V={} L={} P={} T={})\r\n", fname, volume, repeats, priority, type );
      }
   }
}

/* Trigger music file ...
 *
 * "fname" is the name of the music file to be played
 * "vol" is the volume level to play the music at    
 * "repeats" is the number of times to play the music file
 * "continu" specifies whether the file should be restarted if requested again [Yes, the 'e' being dropped here is intentional]
 * "type" is the sound Class 
 * "URL" is the optional download URL for the sound file
 * "ch" is the character to play the music for 
 *
 * more detailed information at: https://www.zuggsoft.com/zmud/msp.htm
*/
void char_data::music( std::string_view fname, int volume, bool toroom )
{
   const char *type = "mud";
   int repeats = 1, continu = 1;

   if( sysdata->http.empty(  ) )
      return;

   std::string url = std::format( "{}/sounds/{}", sysdata->http, fname );

   if( !toroom )
   {
      if( !MSP_ON(  ) )
         return;

      if( url[0] != '\0' )
         print_fmt( "!!MUSIC({} V={} L={} C={} T={} U={})\r\n", fname, volume, repeats, continu, type, url );
      else
         print_fmt( "!!MUSIC({} V={} L={} C={} T={})\r\n", fname, volume, repeats, continu, type );
   }
   else
   {
      for( auto* vch : in_room->people )
      {
         if( !vch->MSP_ON(  ) )
            return;

         if( !url.empty() )
            vch->print_fmt( "!!MUSIC({} V={} L={} C={} T={} U={})\r\n", fname, volume, repeats, continu, type, url );
         else
            vch->print_fmt( "!!MUSIC({} V={} L={} C={} T={})\r\n", fname, volume, repeats, continu, type );
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
   std::string target, vol;
   int volume;
   char_data *victim;
   std::bitset<MAX_ACT_FLAG> actflags;

   if( !ch->isnpc(  ) || ch->has_aflag( AFF_CHARM ) )
   {
      ch->print( "Huh?\r\n" );
      return;
   }

   argument = one_argument( argument, target );

   if( !( victim = ch->get_char_room( target ) ) )
   {
      progbug( "Mpsoundaround - No victim specified", ch );
      return;
   }

   argument = one_argument( argument, vol );

   if( vol.empty(  ) )
   {
      progbug( "Mpsoundaround - No volume level specified", ch );
      return;
   }

   if( !is_number( vol ) )
   {
      progbug( "mpsoundaround - non-numerical volume level specified", ch );
      return;
   }

   volume = std::stoi( vol );

   if( volume < 1 || volume > 100 )
   {
      progbugf( ch, "mpsoundaround - invalid volume {}. range is 1 to 100", volume );
      return;
   }

   if( argument.empty(  ) )
   {
      progbug( "mpsoundaround - no sound file specified", ch );
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
   std::string target, vol;
   int volume;
   char_data *victim;
   std::bitset<MAX_ACT_FLAG> actflags;

   if( !ch->isnpc(  ) || ch->has_aflag( AFF_CHARM ) )
   {
      ch->print( "Huh?\r\n" );
      return;
   }

   argument = one_argument( argument, target );

   if( !( victim = ch->get_char_room( target ) ) )
   {
      progbug( "mpsoundat - No victim specified", ch );
      return;
   }

   argument = one_argument( argument, vol );

   if( vol.empty(  ) )
   {
      progbug( "mpsoundat - No volume level specified", ch );
      return;
   }

   if( !is_number( vol ) )
   {
      progbug( "mpsoundat - non-numerical volume level specified", ch );
      return;
   }

   volume = std::stoi( vol );

   if( volume < 1 || volume > 100 )
   {
      progbugf( ch, "mpsoundat - invalid volume {}. range is 1 to 100", volume );
      return;
   }

   if( argument.empty(  ) )
   {
      progbug( "mpsoundat - no sound file specified", ch );
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
   std::string vol;
   std::bitset<MAX_ACT_FLAG> actflags;

   if( !ch->isnpc(  ) || ch->has_aflag( AFF_CHARM ) )
   {
      ch->print( "Huh?\r\n" );
      return;
   }

   argument = one_argument( argument, vol );

   if( vol.empty(  ) )
   {
      progbug( "mpsound - No volume level specified", ch );
      return;
   }

   if( !is_number( vol ) )
   {
      progbug( "mpsound - non-numerical volume level specified", ch );
      return;
   }

   int volume = std::stoi( vol );

   if( volume < 1 || volume > 100 )
   {
      progbugf( ch, "mpsound - invalid volume {}. range is 1 to 100", volume );
      return;
   }

   if( argument.empty(  ) )
   {
      progbug( "mpsound - no sound file specified", ch );
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
   std::string vol;

   if( !ch->isnpc(  ) || ch->has_aflag( AFF_CHARM ) )
   {
      ch->print( "Huh?\r\n" );
      return;
   }

   argument = one_argument( argument, vol );

   if( vol.empty(  ) )
   {
      progbug( "mpsoundzone - No volume level specified", ch );
      return;
   }

   if( !is_number( vol ) )
   {
      progbug( "mpsoundzone - non-numerical volume level specified", ch );
      return;
   }

   int volume = std::stoi( vol );

   if( volume < 1 || volume > 100 )
   {
      progbugf( ch, "mpsoundzone - invalid volume {}. range is 1 to 100", volume );
      return;
   }

   if( argument.empty(  ) )
   {
      progbug( "mpsoundzone - no sound file specified", ch );
      return;
   }

   std::bitset<MAX_ACT_FLAG> actflags = ch->get_actflags(  );
   ch->unset_actflag( ACT_SECRETIVE );

   for( auto* vch : charlist )
   {
      if( vch->in_room && vch->in_room->area == ch->in_room->area )
         vch->sound( argument, volume, false );
   }
   ch->set_actflags( actflags );
}

/* Music stuff, same as above, at zMUD coders' request -- Blodkai */
CMDF( do_mpmusicaround )
{
   std::string target, vol;
   char_data *victim;
   std::bitset<MAX_ACT_FLAG> actflags;

   if( !ch->isnpc(  ) || ch->has_aflag( AFF_CHARM ) )
   {
      ch->print( "Huh?\r\n" );
      return;
   }

   argument = one_argument( argument, target );

   if( !( victim = ch->get_char_room( target ) ) )
   {
      progbug( "mpmusicaround - No victim specified", ch );
      return;
   }

   argument = one_argument( argument, vol );

   if( vol.empty(  ) )
   {
      progbug( "mpmusicaround - No volume level specified", ch );
      return;
   }

   if( !is_number( vol ) )
   {
      progbug( "mpmusicaround - non-numerical volume level specified", ch );
      return;
   }

   int volume = std::stoi( vol );

   if( volume < 1 || volume > 100 )
   {
      progbugf( ch, "mpmusicaround - invalid volume {}. range is 1 to 100", volume );
      return;
   }

   if( argument.empty(  ) )
   {
      progbug( "mpmusicaround - no sound file specified", ch );
      return;
   }

   actflags = ch->get_actflags(  );
   ch->unset_actflag( ACT_SECRETIVE );
   victim->music( argument, volume, true );
   ch->set_actflags( actflags );
}

CMDF( do_mpmusic )
{
   std::string target, vol;
   char_data *victim;
   std::bitset<MAX_ACT_FLAG> actflags;

   if( !ch->isnpc(  ) || ch->has_aflag( AFF_CHARM ) )
   {
      ch->print( "Huh?\r\n" );
      return;
   }

   argument = one_argument( argument, target );

   if( !( victim = ch->get_char_room( target ) ) )
   {
      progbug( "mpmusic - No victim specified", ch );
      return;
   }

   argument = one_argument( argument, vol );

   if( vol.empty(  ) )
   {
      progbug( "mpmusic - No volume level specified", ch );
      return;
   }

   if( !is_number( vol ) )
   {
      progbug( "mpmusic - non-numerical volume level specified", ch );
      return;
   }

   int volume = std::stoi( vol );

   if( volume < 1 || volume > 100 )
   {
      progbugf( ch, "mpmusic - invalid volume {}. range is 1 to 100", volume );
      return;
   }

   if( argument.empty(  ) )
   {
      progbug( "mpmusic - no sound file specified", ch );
      return;
   }
   actflags = ch->get_actflags(  );
   ch->unset_actflag( ACT_SECRETIVE );
   victim->music( argument, volume, true );
   ch->set_actflags( actflags );
}

CMDF( do_mpmusicat )
{
   std::string target, vol;
   char_data *victim;
   std::bitset<MAX_ACT_FLAG> actflags;

   if( !ch->isnpc(  ) || ch->has_aflag( AFF_CHARM ) )
   {
      ch->print( "Huh?\r\n" );
      return;
   }

   argument = one_argument( argument, target );

   if( !( victim = ch->get_char_room( target ) ) )
   {
      progbug( "mpmusicat - No victim specified", ch );
      return;
   }

   argument = one_argument( argument, vol );

   if( vol.empty(  ) )
   {
      progbug( "mpmusicat - No volume level specified", ch );
      return;
   }

   if( !is_number( vol ) )
   {
      progbug( "mpmusicat - non-numerical volume level specified", ch );
      return;
   }

   int volume = std::stoi( vol );

   if( volume < 1 || volume > 100 )
   {
      progbugf( ch, "mpmusicat - invalid volume {}. range is 1 to 100", volume );
      return;
   }

   if( argument.empty(  ) )
   {
      progbug( "mpmusicat - no sound file specified", ch );
      return;
   }
   actflags = ch->get_actflags(  );
   ch->unset_actflag( ACT_SECRETIVE );
   victim->music( argument, volume, false );
   ch->set_actflags( actflags );
}
// End MSP Information
