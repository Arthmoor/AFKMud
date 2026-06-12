/****************************************************************************
 *                   ^     +----- |  / ^     ^ |     | +-\                  *
 *                  / \    |      | /  |\   /| |     | |  \                 *
 *                 /   \   +---   |<   | \ / | |     | |  |                 *
 *                /-----\  |      | \  |  v  | |     | |  /                 *
 *               /       \ |      |  \ |     | +-----+ +-/                  *
 ****************************************************************************
 * AFKMud Copyright 1997-2025 by Roger Libiez (Samson),                     *
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
 *                      Player communication module                         *
 ****************************************************************************/

#include <format>
#include "mud.h"
#include "area.h"
#include "boards.h"
#include "descriptor.h"
#include "language.h"
#include "mobindex.h"
#include "mud_prog.h"
#include "objindex.h"
#include "polymorph.h"
#include "raceclass.h"
#include "roomindex.h"

bool is_inolc( descriptor_data * );
board_data *find_board( char_data * );
board_data *get_board( char_data *, std::string_view );

std::string NAME( char_data * ch )
{
   return( ch->isnpc() ? ch->short_descr : ch->name );
}

std::string MORPHNAME( char_data * ch )
{
   if( ch->morph && ch->morph->morph && ch->morph->morph->short_desc != nullptr )
      return ch->morph->morph->short_desc;

   return( ch->isnpc() ? ch->short_descr : ch->name );
}

lang_data *get_lang( std::string_view name )
{
   for( auto* lang: langlist )
   {
      if( !str_cmp( lang->name, name ) )
         return lang;
   }
   return nullptr;
}

// Rewritten by Xorith for more C++
std::string translate( int percent, std::string_view in, std::string_view name )
{
   lang_data* lng = get_lang( name );

   if( percent > 99 || name == "common" || ( !lng && !( lng = get_lang( "default" ) ) ) )
   {
      return std::string{in};
   }

   std::string retVal;
   retVal.reserve( in.length() );

   for( size_t i = 0; i < in.length(); )
   {
      bool found = false;

      for( auto* conv : lng->prelist )
      {
         if( in.substr(i).starts_with( conv->old ) )
         {
            retVal += ( percent > 0 && number_percent() <= percent ) ? conv->old : conv->lnew;
            i += conv->old.length();
            found = true;
            break;
         }
      }

      if( !found )
      {
         char c = in[i];

         if( std::isalpha( static_cast<unsigned char>(c) ) && ( percent == 0 || number_percent() > percent ) )
         {
            retVal += lng->alphabet[std::tolower( static_cast<unsigned char>(c) ) - 'a'];
         }
         else
         {
            retVal += c;
         }
         i++;
      }
   }

   // Secondary conversion pass (if cnvlist exists)
   if( !lng->cnvlist.empty() )
   {
      std::string pass2;

      for( size_t i = 0; i < retVal.length(); )
      {
         bool found = false;

         for( auto* conv : lng->cnvlist )
         {
            if( retVal.substr(i).starts_with( conv->old ) )
            {
               pass2 += conv->lnew;
               i += conv->old.length();
               found = true;
               break;
            }
         }
         if( !found )
            pass2 += retVal[i++];
      }
      return pass2;
   }
   return retVal;
}

/*
 * Written by Kratas (moon@deathmoon.com)
 * Modified by Samson to be used in place of the ASK channel.
 */
CMDF( do_ask )
{
   std::string arg, sbuf;
   char_data *victim;
   int speaking = -1;

   for( int lang = 0; lang < LANG_UNKNOWN; ++lang )
   {
      if( ch->speaking == lang )
      {
         speaking = lang;
         break;
      }
   }

   argument = one_argument( argument, arg );

   if( arg.empty(  ) || argument.empty(  ) )
   {
      ch->print( "Ask who what?\r\n" );
      return;
   }

   if( ( victim = ch->get_char_room( arg ) ) == nullptr || ( victim->isnpc(  ) && victim->in_room != ch->in_room ) )
   {
      ch->print( "They aren't here.\r\n" );
      return;
   }

   if( ch->in_room->flags.test( ROOM_SILENCE ) || ch->in_room->area->flags.test( AFLAG_SILENCE ) )
   {
      ch->print( "You can't do that here.\r\n" );
      return;
   }

   if( victim == ch )
   {
      ch->print( "You ask yourself the question. Did it help?\r\n" );
      return;
   }

   std::bitset<MAX_ACT_FLAG> actflags = ch->get_actflags(  );
   if( ch->isnpc(  ) )
      ch->unset_actflag( ACT_SECRETIVE );

   if( victim == ch )
   {
      ch->print( "You ask yourself the question. Did it help?\r\n" );
      return;
   }

   /*
    * Check to see if a player on a map is at the same coords as the recipient 
    */
   if( !is_same_char_map( ch, victim ) )
   {
      ch->print( "They aren't here.\r\n" );
      return;
   }

   /*
    * Check to see if character is ignoring speaker 
    */
   if( is_ignoring( victim, ch ) )
   {
      if( !ch->is_immortal(  ) || victim->get_trust(  ) > ch->get_trust(  ) )
         return;
      else
         victim->printf( "&[ignore]You attempt to ignore %s, but are unable to do so.\r\n", ch->name );
   }

   sbuf = argument;
   if( speaking != -1 && ( !ch->isnpc(  ) || ch->speaking ) )
   {
      int speakswell = umin( knows_language( victim, ch->speaking, ch ), knows_language( ch, ch->speaking, victim ) );
      if( speakswell < 75 )
         sbuf = translate( speakswell, argument, lang_names[speaking] );
   }

   ch->set_actflags( actflags );
   MOBtrigger = false;

   act( AT_SAY, "You ask $N '$t'", ch, sbuf.c_str(  ), victim, TO_CHAR );
   act( AT_SAY, "$n asks you '$t'", ch, sbuf.c_str(  ), victim, TO_VICT );

   if( ch->in_room->flags.test( ROOM_LOGSPEECH ) )
      append_to_file( LOG_FILE, "{}: {}", ch->isnpc(  )? ch->short_descr : ch->name, argument );

   mprog_targetted_speech_trigger( argument, ch, victim );
}

void update_sayhistory( char_data * ch, char_data * vch, std::string_view msg )
{
   std::string message;

   if( vch->isnpc() )
      return;

   message = std::format( "&R[{}] {}{} said '{}'", mini_c_time( current_time, vch->pcdata->timezone ),
         ( vch == ch ? ch->color_str( AT_SAY ) : vch->color_str( AT_SAY ) ), vch == ch ? "You" : PERS( ch, vch, false ), msg );

   vch->pcdata->say_history.push_back( message );

   if( vch->pcdata->say_history.size() >= MAX_SAYHISTORY )
      vch->pcdata->say_history.erase( vch->pcdata->say_history.begin() );
}

CMDF( do_say )
{
   int speaking = -1;

   for( int lang = 0; lang < LANG_UNKNOWN; ++lang )
   {
      if( ch->speaking == lang )
      {
         speaking = lang;
         break;
      }
   }

   if( argument.empty(  ) )
   {
      if( ch->isnpc(  ) )
      {
         ch->print( "Say what?\r\n" );
         return;
      }

      ch->printf( "&cThe last %d things you heard said:\r\n", MAX_SAYHISTORY );

      for( size_t x = 0; x < ch->pcdata->say_history.size(); ++x )
         ch->printf( "%s\r\n", ch->pcdata->say_history[x].c_str() );

      return;
   }

   if( ch->in_room->flags.test( ROOM_SILENCE ) || ch->in_room->area->flags.test( AFLAG_SILENCE ) )
   {
      ch->print( "You can't do that here.\r\n" );
      return;
   }

   // Adaptation of Smaug 1.8b feature. Stop whitespace abuse now!
   strip_spaces( argument );

   std::bitset < MAX_ACT_FLAG > actflags = ch->get_actflags(  );
   if( ch->isnpc(  ) )
      ch->unset_actflag( ACT_SECRETIVE );

   for( auto* vch : ch->in_room->people )
   {
      std::string sbuf = argument;

      if( vch == ch )
         continue;

      /*
       * Check to see if a player on a map is at the same coords as the recipient
       */
      if( !is_same_char_map( ch, vch ) )
         continue;

      /*
       * Check to see if character is ignoring speaker
       */
      if( is_ignoring( vch, ch ) )
      {
         /*
          * continue unless speaker is an immortal
          */
         if( !ch->is_immortal(  ) || vch->level > ch->level )
            continue;
         else
            vch->printf( "&[ignore]You attempt to ignore %s, but are unable to do so.\r\n", !vch->can_see( ch, false ) ? "Someone" : ch->name );
      }

      if( speaking != -1 && ( !ch->isnpc(  ) || ch->speaking ) )
      {
         int speakswell = umin( knows_language( vch, ch->speaking, ch ), knows_language( ch, ch->speaking, vch ) );

         if( speakswell < 75 )
            sbuf = translate( speakswell, argument, lang_names[speaking] );
      }

      MOBtrigger = false;
      act( AT_SAY, "$n says '$t'", ch, sbuf.c_str(  ), vch, TO_VICT );
      if( !vch->isnpc(  ) )
         update_sayhistory( ch, vch, sbuf );
   }
   ch->set_actflags( actflags );
   MOBtrigger = false;

   act( AT_SAY, "You say '$T'", ch, nullptr, argument.c_str(  ), TO_CHAR );

   if( !ch->isnpc(  ) )
      update_sayhistory( ch, ch, argument );

   if( ch->in_room->flags.test( ROOM_LOGSPEECH ) )
      append_to_file( LOG_FILE, "{}: {}", ch->isnpc(  )? ch->short_descr : ch->name, argument );

   mprog_speech_trigger( argument, ch );
   if( ch->char_died(  ) )
      return;

   mprog_and_speech_trigger( argument, ch );
   if( ch->char_died(  ) )
      return;

   oprog_speech_trigger( argument, ch );
   if( ch->char_died(  ) )
      return;

   oprog_and_speech_trigger( argument, ch );
   if( ch->char_died(  ) )
      return;

   rprog_speech_trigger( argument, ch );
   if( ch->char_died(  ) )
      return;

   rprog_and_speech_trigger( argument, ch );
}

CMDF( do_whisper )
{
   std::string arg;
   char_data *victim;
   int position;
   int speaking = -1, lang;

   for( lang = 0; lang < LANG_UNKNOWN; ++lang )
   {
      if( ch->speaking == lang )
      {
         speaking = lang;
         break;
      }
   }

   if( ch->in_room->flags.test( ROOM_SILENCE ) || ch->in_room->area->flags.test( AFLAG_SILENCE ) )
   {
      ch->print( "You can't do that here.\r\n" );
      return;
   }

   argument = one_argument( argument, arg );

   if( arg.empty(  ) || argument.empty(  ) )
   {
      ch->print( "Whisper to whom what?\r\n" );
      return;
   }

   if( !( victim = ch->get_char_room( arg ) ) )
   {
      ch->print( "They aren't here.\r\n" );
      return;
   }

   if( ch == victim )
   {
      ch->print( "You have a nice little chat with yourself.\r\n" );
      return;
   }

   if( !victim->isnpc(  ) && ( victim->switched ) && !victim->switched->has_aflag( AFF_POSSESS ) )
   {
      ch->print( "That player is switched.\r\n" );
      return;
   }
   else if( !victim->isnpc(  ) && ( !victim->desc ) )
   {
      ch->print( "That player is link-dead.\r\n" );
      return;
   }

   if( victim->has_pcflag( PCFLAG_AFK ) )
   {
      ch->print( "That player is afk.\r\n" );
      return;
   }

   if( victim->has_pcflag( PCFLAG_SILENCE ) )
      ch->print( "That player is silenced. They will receive your message but can not respond.\r\n" );

   if( victim->has_aflag( AFF_SILENCE ) )
      ch->print( "That player has been magically muted!\r\n" );

   if( victim->desc && victim->desc->connected == CON_EDITING && ch->get_trust(  ) < LEVEL_GOD )
   {
      act( AT_PLAIN, "$E is currently in a writing buffer. Please try again in a few minutes.", ch, nullptr, victim, TO_CHAR );
      return;
   }

   /*
    * Stopping people from sending tells whispers etc to people on their ignore list. -Leart
    */
   if( is_ignoring( ch, victim ) )
   {
      /* If the sender is an imm then they can bypass this check */
      if( !ch->is_immortal( ) || victim->get_trust( ) > ch->get_trust( ) )
      {
         ch->printf( "&[ignore]You are currently ignoring %s.\r\n"
            "Please type 'ignore %s' to stop ignoring them, then try whispering to them again.\r\n", victim->name, victim->name );
         return;
   	}
   }

   /*
    * Check to see if target of tell is ignoring the sender 
    */
   if( is_ignoring( victim, ch ) )
   {
      /*
       * If the sender is an imm then they cannot be ignored 
       */
      if( !ch->is_immortal(  ) || victim->get_trust(  ) > ch->get_trust(  ) )
         return;
      else
         victim->printf( "&[ignore]You attempt to ignore %s, but are unable to do so.\r\n", !victim->can_see( ch, false ) ? "Someone" : ch->name );
   }

   // Adaptation of Smaug 1.8b feature. Stop whitespace abuse now!
   strip_spaces( argument );

   MOBtrigger = false;
   act( AT_WHISPER, "You whisper to $N '$t'", ch, argument.c_str(  ), victim, TO_CHAR );
   position = victim->position;
   victim->position = POS_STANDING;

   if( speaking != -1 && ( !ch->isnpc(  ) || ch->speaking ) )
   {
      int speakswell = umin( knows_language( victim, ch->speaking, ch ), knows_language( ch, ch->speaking, victim ) );

      if( speakswell < 85 )
         act( AT_WHISPER, "$n whispers to you '$t'", ch, translate( speakswell, argument, lang_names[speaking] ).c_str(  ), victim, TO_VICT );
      else
         act( AT_WHISPER, "$n whispers to you '$t'", ch, argument.c_str(  ), victim, TO_VICT );
   }
   else
      act( AT_WHISPER, "$n whispers to you '$t'", ch, argument.c_str(  ), victim, TO_VICT );

   MOBtrigger = true;
   victim->position = position;

   if( ch->in_room->flags.test( ROOM_SILENCE ) || ch->in_room->area->flags.test( AFLAG_SILENCE ) )
      act( AT_WHISPER, "$n whispers something to $N.", ch, argument.c_str(  ), victim, TO_NOTVICT );

   if( ch->in_room->flags.test( ROOM_LOGSPEECH ) )
      append_to_file( LOG_FILE, "{}: {} (whisper to) {}.", ch->isnpc(  )? ch->short_descr : ch->name, argument,
                      victim->isnpc(  )? victim->short_descr : victim->name );

   if( victim->isnpc(  ) )
   {
      mprog_speech_trigger( argument, ch );
      if( ch->char_died(  ) )
         return;

      mprog_and_speech_trigger( argument, ch );
   }
}

/* Beep command courtesy of Altrag */
/* Installed by Samson on unknown date, allows user to beep other users */
CMDF( do_beep )
{
   char_data *victim = nullptr;

   if( ch->has_pcflag( PCFLAG_NO_BEEP ) )
   {
      ch->print( "You are not allowed to send beeps!\r\n" );
      return;
   }

   if( ch->pcdata->release_date != std::chrono::system_clock::time_point{} )
   {
      ch->print( "Nope, no beeping from hell.\r\n" );
      return;
   }

   if( argument.empty(  ) || !( victim = ch->get_char_world( argument ) ) )
   {
      ch->print( "Beep who?\r\n" );
      return;
   }

   /*
    * NPC check added by Samson 2-15-98 
    */
   if( victim->isnpc(  ) || is_ignoring( victim, ch ) )
   {
      ch->print( "Beep who?\r\n" );
      return;
   }

   if( victim->has_pcflag( PCFLAG_NO_BEEP ) )
   {
      ch->print_fmt( "{} is not allowed to receive beeps!\r\n", victim->name );
      return;
   }

   /*
    * PCFLAG_NOBEEP check added by Samson 2-15-98 
    */
   if( victim->has_pcflag( PCFLAG_NOBEEP ) )
   {
      ch->print_fmt( "{} is not accepting beeps at this time.\r\n", victim->name );
      return;
   }

   victim->print_fmt( "{} is beeping you!\a\r\n", PERS( ch, victim, true ) );
   ch->print_fmt( "You beep {}.\r\n", PERS( victim, ch, true ) );
}

void update_tellhistory( char_data * vch, char_data * ch, std::string_view msg, bool self )
{
   char_data *tch;
   std::string message;

   if( self )
   {
      message = std::format( "&R[{}] {}You told {} '{}'",
         ch->isnpc() ? ( mini_c_time( current_time, -1 ) ) : ( mini_c_time( current_time, ch->pcdata->timezone ) ),
         ch->color_str( AT_TELL ), PERS( vch, ch, false ), msg );
      tch = ch;
   }
   else
   {
      message = std::format( "&R[{}] {}{} told you '{}'",
         vch->isnpc() ? ( mini_c_time( current_time, -1 ) ) : ( mini_c_time( current_time, vch->pcdata->timezone ) ),
         vch->color_str( AT_TELL ), PERS( ch, vch, false ), msg );
      tch = vch;
   }

   if( tch->isnpc(  ) )
      return;

   tch->pcdata->tell_history.push_back( message );

   if( tch->pcdata->tell_history.size() >= MAX_TELLHISTORY )
      tch->pcdata->tell_history.erase( tch->pcdata->tell_history.begin() );
}

CMDF( do_tell )
{
   std::string arg;
   char_data *victim;
   int position;
   char_data *switched_victim = nullptr;
   continent_data *orig_continent;
   int speaking = -1, lang;
   /*
    * Hackish solution to stop that damned "someone chat" bug - Matarael 17.3.2002 
    */
   bool mapped = false;
   int origx = -1, origy = -1;

   for( lang = 0; lang < LANG_UNKNOWN; ++lang )
   {
      if( ch->speaking == lang )
      {
         speaking = lang;
         break;
      }
   }

   if( ch->in_room->flags.test( ROOM_SILENCE ) || ch->in_room->area->flags.test( AFLAG_SILENCE ) )
   {
      ch->print( "You can't do that here.\r\n" );
      return;
   }

   if( ch->has_pcflag( PCFLAG_SILENCE ) || ch->has_pcflag( PCFLAG_NO_TELL ) || ch->has_aflag( AFF_SILENCE ) )
   {
      ch->print( "You can't do that.\r\n" );
      return;
   }

   argument = one_argument( argument, arg );

   if( arg.empty(  ) || argument.empty(  ) )
   {
      if( ch->isnpc(  ) )
      {
         ch->print( "Tell who what?\r\n" );
         return;
      }

      ch->printf( "&cThe last %d things you were told:\r\n", MAX_TELLHISTORY );

      for( size_t x = 0; x < ch->pcdata->tell_history.size(); ++x )
         ch->printf( "%s\r\n", ch->pcdata->tell_history[x].c_str() );
      return;
   }

   if( !( victim = ch->get_char_world( arg ) ) || ( victim->isnpc(  ) && victim->in_room != ch->in_room ) )
   {
      ch->print( "They aren't here.\r\n" );
      return;
   }

   if( ch == victim )
   {
      ch->print( "You have a nice little chat with yourself.\r\n" );
      return;
   }

   if( !victim->isnpc(  ) && ( victim->switched ) && ch->is_immortal(  ) && !victim->switched->has_aflag( AFF_POSSESS ) )
   {
      ch->print( "That player is switched.\r\n" );
      return;
   }

   else if( !victim->isnpc(  ) && ( victim->switched ) && victim->switched->has_aflag( AFF_POSSESS ) )
      switched_victim = victim->switched;

   else if( !victim->isnpc(  ) && ( !victim->desc ) )
   {
      ch->print( "That player is link-dead.\r\n" );
      return;
   }

   if( victim->has_pcflag( PCFLAG_AFK ) )
   {
      if( ch->is_immortal() )
         ch->print( "That player is AFK, but will receive your message.\r\n" );
      else
      {
         ch->print( "That player is afk.\r\n" );
         return;
      }
   }

   if( victim->has_pcflag( PCFLAG_NOTELL ) && !ch->is_immortal(  ) )
      /*
       * Immortal check added to let imms tell players at all times, Adjani, 12-02-2002
       */
   {
      act( AT_PLAIN, "$E has $S tells turned off.", ch, nullptr, victim, TO_CHAR );
      return;
   }

   if( victim->has_pcflag( PCFLAG_SILENCE ) )
      ch->print( "That player is silenced. They will receive your message but can not respond.\r\n" );

   if( !victim->isnpc(  ) && victim->has_aflag( AFF_SILENCE ) )
      ch->print( "That player has been magically muted!\r\n" );

   if( ( !ch->is_immortal(  ) && !victim->IS_AWAKE(  ) ) || ( !victim->isnpc(  ) && ( victim->in_room->flags.test( ROOM_SILENCE ) || victim->in_room->area->flags.test( AFLAG_SILENCE ) ) ) )
   {
      act( AT_PLAIN, "$E can't hear you.", ch, nullptr, victim, TO_CHAR );
      return;
   }

   if( victim->desc  /* make sure desc exists first  -Thoric */
       && victim->desc->connected == CON_EDITING && ch->get_trust(  ) < LEVEL_GOD )
   {
      act( AT_PLAIN, "$E is currently in a writing buffer. Please try again in a few minutes.", ch, 0, victim, TO_CHAR );
      return;
   }

   /*
    * Stopping people from sending tells whispers etc to people on their ignore list. -Leart
    */
   if( is_ignoring( ch, victim ) )
   {
      /* If the sender is an imm then they can bypass this check */
      if( !ch->is_immortal( ) || victim->get_trust( ) > ch->get_trust( ) )
      {
         ch->printf( "&[ignore]You are currently ignoring %s.\r\n"
            "Please type 'ignore %s' to stop ignoring them, then try sending your tell to them again.\r\n", victim->name, victim->name );
         return;
   	}
   }

   /*
    * Check to see if target of tell is ignoring the sender 
    */
   if( is_ignoring( victim, ch ) )
   {
      /*
       * If the sender is an imm then they cannot be ignored 
       */
      if( !ch->is_immortal(  ) || victim->level > ch->level )
      {
         /*
          * Drop the command into oblivion, why tell the other guy you're ignoring them? 
          */
         ch->print( "They aren't here.\r\n" );
         return;
      }
      else
         victim->printf( "&[ignore]You attempt to ignore %s, but are unable to do so.\r\n", !victim->can_see( ch, false ) ? "Someone" : ch->name );
   }

   if( switched_victim )
      victim = switched_victim;

   /*
    * Hackish solution to stop that damned "someone chat" bug - Matarael 17.3.2002 
    */
   if( ch->has_pcflag( PCFLAG_ONMAP ) )
   {
      mapped = true;
      origx = ch->map_x;
      origy = ch->map_y;
      orig_continent = ch->continent;
   }

   if( ch->isnpc(  ) && ch->has_actflag( ACT_ONMAP ) )
   {
      mapped = true;
      origx = ch->map_x;
      origy = ch->map_y;
      orig_continent = ch->continent;
   }
   fix_maps( victim, ch );

   MOBtrigger = false;  /* BUGFIX - do_tell: Tells were triggering act progs */

   act( AT_TELL, "You tell $N '$t'", ch, argument.c_str(  ), victim, TO_CHAR );
   update_tellhistory( victim, ch, argument, true );
   position = victim->position;
   victim->position = POS_STANDING;
   if( speaking != -1 && ( !ch->isnpc(  ) || ch->speaking ) )
   {
      int speakswell = umin( knows_language( victim, ch->speaking, ch ), knows_language( ch, ch->speaking, victim ) );

      if( speakswell < 85 )
      {
         act( AT_TELL, "$n tells you '$t'", ch, translate( speakswell, argument, lang_names[speaking] ).c_str(  ), victim, TO_VICT );
         update_tellhistory( victim, ch, translate( speakswell, argument, lang_names[speaking] ), false );
      }
      else
      {
         act( AT_TELL, "$n tells you '$t'", ch, argument.c_str(  ), victim, TO_VICT );
         update_tellhistory( victim, ch, argument, false );
      }
   }
   else
   {
      act( AT_TELL, "$n tells you '$t'", ch, argument.c_str(  ), victim, TO_VICT );
      update_tellhistory( victim, ch, argument, false );
   }

   MOBtrigger = true;   /* BUGFIX - do_tell: Tells were triggering act progs */

   /*
    * Hackish solution to stop that damned "someone chat" bug - Matarael 17.3.2002 
    */
   if( mapped )
   {
      ch->continent = orig_continent;
      ch->map_x = origx;
      ch->map_y = origy;
      if( ch->isnpc(  ) )
         ch->set_actflag( ACT_ONMAP );
      else
         ch->set_pcflag( PCFLAG_ONMAP );
   }
   else
   {
      if( ch->isnpc(  ) )
         ch->unset_actflag( ACT_ONMAP );
      else
         ch->unset_pcflag( PCFLAG_ONMAP );
      ch->continent = nullptr;
      ch->map_x = -1;
      ch->map_y = -1;
   }

   victim->position = position;
   victim->reply = ch;

   if( ch->in_room->flags.test( ROOM_LOGSPEECH ) )
      append_to_file( LOG_FILE, "{}: {} (tell to) {}.", ch->isnpc(  ) ? ch->short_descr : ch->name, argument, victim->isnpc(  ) ? victim->short_descr : victim->name );

   if( victim->isnpc(  ) )
   {
      mprog_tell_trigger( argument, ch );
      if( ch->char_died(  ) )
         return;

      mprog_and_tell_trigger( argument, ch );
   }
}

CMDF( do_reply )
{
   char_data *victim;

   if( !( find_board( ch ) ) )
   {
      std::string arg; /* Placed this here since it's only used here -- X */

      if( ( is_number( one_argument( argument, arg ) ) ) && ( get_board( ch, arg ) ) )
      {
         cmdf( ch, "write %s", argument.c_str(  ) );
         return;
      }
   }
   else if( is_number( argument ) && !argument.empty(  ) )
   {
      cmdf( ch, "write %s", argument.c_str(  ) );
      return;
   }

   if( !( victim = ch->reply ) )
   {
      ch->print( "Either you have nothing to reply to, or that person has left.\r\n" );
      return;
   }

   if( argument.empty(  ) )
   {
      ch->printf( "And what would you like to say in reply to %s?\r\n", victim->name );
      return;
   }

   /*
    * This is a bit shorter than what was here before, no? Accomplished the same bloody thing too. -- Xorith 
    */
   cmdf( ch, "tell %s %s", victim->name, argument.c_str(  ) );
}

CMDF( do_emote )
{
   int speaking = -1;

   for( int lang = 0; lang < LANG_UNKNOWN; ++lang )
   {
      if( ch->speaking == lang )
      {
         speaking = lang;
         break;
      }
   }

   /*
    * Per Alcane's notice, emote no longer works in silent rooms - Samson 1-14-00 
    */
   if( ch->in_room->flags.test( ROOM_SILENCE ) || ch->in_room->area->flags.test( AFLAG_SILENCE ) )
   {
      ch->print( "The room is magically silenced! You cannot express emotions!\r\n" );
      return;
   }

   if( ch->has_pcflag( PCFLAG_NO_EMOTE ) )
   {
      ch->print( "You can't show your emotions.\r\n" );
      return;
   }

   if( argument.empty(  ) )
   {
      ch->print( "Emote what?\r\n" );
      return;
   }

   std::bitset < MAX_ACT_FLAG > actflags = ch->get_actflags(  );
   if( ch->isnpc(  ) )
      ch->unset_actflag( ACT_SECRETIVE );

   for( auto* vch : ch->in_room->people )
   {
      std::string sbuf = argument;

      /*
       * Check to see if a player on a map is at the same coords as the recipient 
       * don't need to verify the PCFLAG_ONMAP flags here, it's a room occupants check 
       */
      if( !is_same_char_map( ch, vch ) )
         continue;

      /*
       * Check to see if character is ignoring emoter 
       */
      if( is_ignoring( vch, ch ) )
      {
         /*
          * continue unless emoter is an immortal 
          */
         if( !ch->is_immortal(  ) || vch->level > ch->level )
            continue;
         else
            vch->printf( "&[ignore]You attempt to ignore %s, but are unable to do so.\r\n", !vch->can_see( ch, false ) ? "Someone" : ch->name );
      }

      if( speaking != -1 && ( !ch->isnpc(  ) || ch->speaking ) )
      {
         int speakswell = umin( knows_language( vch, ch->speaking, ch ), knows_language( ch, ch->speaking, vch ) );

         if( speakswell < 85 )
            sbuf = translate( speakswell, argument, lang_names[speaking] );
      }

      MOBtrigger = false;
      act( AT_SOCIAL, "$n $t", ch, sbuf.c_str(  ), vch, ( vch == ch ? TO_CHAR : TO_VICT ) );
   }
   ch->set_actflags( actflags );
   if( ch->in_room->flags.test( ROOM_LOGSPEECH ) )
      append_to_file( LOG_FILE, "{} {} (emote)", ch->isnpc(  ) ? ch->short_descr : ch->name, argument );
}

/* 0 = bug 1 = idea 2 = typo */
void tybuid( char_data * ch, std::string_view argument, int type )
{
   static const char *tybuid_name[] = { "bug", "idea", "typo" };
   static const char *tybuid_file[] = { PBUG_FILE.data(), IDEA_FILE.data(), TYPO_FILE.data() };

   ch->set_color( AT_PLAIN );

   if( argument.empty(  ) )
   {
      if( type == 1 )
         ch->print( "\r\nUsage:  'idea <message>'\r\n" );
      else
         ch->printf( "Usage:  '%s <message>'  (your location is automatically recorded)\r\n", tybuid_name[type] );
      if( ch->get_trust(  ) >= LEVEL_ASCENDANT )
         ch->printf( "  '%s list' or '%s clear now'\r\n", tybuid_name[type], tybuid_name[type] );
      return;
   }

   if( !str_cmp( argument, "clear now" ) && ch->get_trust(  ) >= LEVEL_ASCENDANT )
   {
      FILE *fp;
      if( !( fp = fopen( tybuid_file[type], "w" ) ) )
      {
         bug( "%s: unable to stat %s file '%s'!", __func__, tybuid_name[type], tybuid_file[type] );
         return;
      }
      FCLOSE( fp );
      ch->printf( "The %s file has been cleared.\r\n", tybuid_name[type] );
      return;
   }

   if( !str_cmp( argument, "list" ) && ch->get_trust(  ) >= LEVEL_ASCENDANT )
   {
      show_file( ch, tybuid_file[type] );
      return;
   }

   std::string t = std::format( "{:%a %b %d, %Y %I:%M:%S %p}", current_time );
   append_file( ch->in_room ? ch->in_room->vnum : 0, ch->name, tybuid_file[type], "({}:  {}", t, argument );
   ch->printf( "Thank you! Your %s has been recorded.\r\n", tybuid_name[type] );
}

CMDF( do_bug )
{
   /*
    * 0 = bug 
    */
   tybuid( ch, argument, 0 );
}

CMDF( do_ide )
{
   ch->print( "&YIf you want to send an idea, type 'idea <message>'.\r\n" );
   ch->print( "If you want to identify an object, use the identify spell.\r\n" );
}

CMDF( do_idea )
{
   /*
    * 1 = idea 
    */
   tybuid( ch, argument, 1 );
}

CMDF( do_typo )
{
   /*
    * 2 = typo 
    */
   tybuid( ch, argument, 2 );
}

CMDF( do_gtell )
{
   int speaking = -1;

   for( int lang = 0; lang < LANG_UNKNOWN; ++lang )
   {
      if( ch->speaking == lang )
      {
         speaking = lang;
         break;
      }
   }

   if( argument.empty(  ) )
   {
      ch->print( "Tell your group what?\r\n" );
      return;
   }

   if( ch->has_pcflag( PCFLAG_NO_TELL ) )
   {
      ch->print( "Your message didn't get through!\r\n" );
      return;
   }

   /*
    * Note use of send_to_char, so gtell works on sleepers.
    */
   for( auto* gch : pclist )
   {
      if( is_same_group( gch, ch ) )
      {
         if( speaking != -1 && ( !ch->isnpc(  ) || ch->speaking ) )
         {
            int speakswell = umin( knows_language( gch, ch->speaking, ch ), knows_language( ch, ch->speaking, gch ) );

            if( speakswell < 85 )
            {
               if( gch == ch )
                  gch->printf( "&[gtells]You tell the group '%s'\r\n", translate( speakswell, argument, lang_names[speaking] ).c_str(  ) );
               else
                  gch->printf( "&[gtells]%s tells the group '%s'\r\n", ch->name, translate( speakswell, argument, lang_names[speaking] ).c_str(  ) );
            }
            else
            {
               if( gch == ch )
                  gch->printf( "&[gtells]You tell the group '%s'\r\n", argument.c_str(  ) );
               else
                  gch->printf( "&[gtells]%s tells the group '%s'\r\n", ch->name, argument.c_str(  ) );
            }
         }
         else
         {
            if( gch == ch )
               gch->printf( "&[gtells]You tell the group '%s'\r\n", argument.c_str(  ) );
            else
               gch->printf( "&[gtells]%s tells the group '%s'\r\n", ch->name, argument.c_str(  ) );
         }
      }
   }
}

/*
 * It is very important that this be an equivalence relation:
 * (1) A ~ A
 * (2) if A ~ B then B ~ A
 * (3) if A ~ B  and B ~ C, then A ~ C
 */
bool is_same_group( char_data * ach, char_data * bch )
{
   if( ach->leader )
      ach = ach->leader;
   if( bch->leader )
      bch = bch->leader;
   return ach == bch;
}

/*
 * Language support functions. -- Altrag
 * 07/01/96
 *
 * Modified to return how well the language is known 04/04/98 - Thoric
 * Currently returns 100% for known languages... but should really return
 * a number based on player's wisdom (maybe 50+((25-wisdom)*2) ?)
 */
int knows_language( char_data * ch, int language, char_data * cch )
{
   if( !ch->isnpc(  ) && ch->is_immortal(  ) )
      return 100;

   if( ch->isnpc(  ) && !ch->has_langs(  ) ) /* No langs = knows all for npcs */
      return 100;

   if( ch->isnpc(  ) && ch->has_lang( language ) && !ch->has_lang( LANG_CLAN ) )
      return 100;

   /*
    * everyone KNOWS common tongue 
    */
   if( language == LANG_COMMON )
      return 100;

   if( language == LANG_CLAN )
   {
      /*
       * Clan = common for mobs.. snicker.. -- Altrag 
       */
      if( ch->isnpc(  ) || cch->isnpc(  ) )
         return 100;

      if( ch->pcdata->clan == cch->pcdata->clan && ch->pcdata->clan != nullptr )
         return 100;
   }

   if( !ch->isnpc(  ) )
   {
      /*
       * Racial languages for PCs 
       */
      if( race_table[ch->race]->language.test( language ) )
         return 100;

      if( ch->has_lang( language ) )
      {
         short sn;

         if( ( sn = skill_lookup( lang_names[language] ) ) != -1 )
            return ch->pcdata->learned[sn];
      }
   }
   return 0;
}

CMDF( do_speak )
{
   int lang = -1;

   if( argument.empty(  ) )
   {
      ch->print( "&[say]Speak what?\r\n" );
      return;
   }

   lang = get_langnum( argument );
   if( lang < 0 || lang >= LANG_UNKNOWN )
   {
      ch->printf( "&[say]%s is not a known tongue on this world.\r\n", argument.c_str(  ) );
      return;
   }

   if( lang == LANG_CLAN && ( ch->isnpc(  ) || !ch->pcdata->clan ) )
   {
      ch->printf( "&[say]The %s tongue is only available to guilds and clans.\r\n", argument.c_str(  ) );
      return;
   }

   if( !ch->is_immortal(  ) && !ch->has_lang( lang ) )
   {
      ch->printf( "&[say]But you have not learned the %s tongue!\r\n", argument.c_str(  ) );
      return;
   }
   ch->speaking = lang;
   ch->printf( "&[say]You are now speaking the %s tongue.\r\n", argument.c_str(  ) );
}

CMDF( do_languages )
{
   std::string arg;
   int lang;

   argument = one_argument( argument, arg );
   if( !arg.empty(  ) && !str_prefix( arg, "learn" ) && !ch->is_immortal(  ) && !ch->isnpc(  ) )
   {
      int sn, prct, prac;
      bool found = false;

      if( argument.empty(  ) )
      {
         ch->print( "Learn which language?\r\n" );
         return;
      }
      lang = get_langnum( argument );

      if( lang < 0 || lang >= LANG_UNKNOWN || lang == LANG_CLAN )
      {
         ch->print( "That is not a language.\r\n" );
         return;
      }

      if( !( VALID_LANGS & lang ) )
      {
         ch->print( "You may not learn that language.\r\n" );
         return;
      }

      if( ( sn = skill_lookup( lang_names[lang] ) ) < 0 )
      {
         ch->print( "That is not a language.\r\n" );
         return;
      }

      if( race_table[ch->race]->language.test( lang ) || lang == LANG_COMMON || ch->pcdata->learned[sn] >= 99 )
      {
         ch->printf( "You are already fluent in %s\r\n", lang_names[lang] );
         return;
      }

      /*
       * Bug fix - Samson 12-25-98 
       */
      char_data *sch;
      std::list<char_data *>::iterator ich;
      for( ich = ch->in_room->people.begin(  ); ich != ch->in_room->people.end(  ); ++ich )
      {
         sch = *ich;

         if( sch->has_actflag( ACT_SCHOLAR ) && knows_language( sch, ch->speaking, ch )
             && knows_language( sch, lang, sch ) && ( !sch->speaking || knows_language( ch, sch->speaking, sch ) ) )
         {
            found = true;
            break;
         }
      }
      if( !found )
      {
         ch->print( "There is no one who can teach that language here.\r\n" );
         return;
      }
      /*
       * 0..16 cha = 2 pracs, 17..25 = 1 prac. -- Altrag 
       */
      prac = 2 - ( ch->get_curr_cha(  ) / 17 );
      if( ch->pcdata->practice < prac )
      {
         act( AT_TELL, "$n tells you 'You do not have enough practices.'", sch, nullptr, ch, TO_VICT );
         return;
      }
      ch->pcdata->practice -= prac;
      /*
       * Max 12% (5 + 4 + 3) at 24+ int and 21+ wis. -- Altrag 
       */
      prct = 5 + ( ch->get_curr_int(  ) / 6 ) + ( ch->get_curr_wis(  ) / 7 );
      ch->pcdata->learned[sn] += prct;
      ch->pcdata->learned[sn] = umin( ch->pcdata->learned[sn], 99 );
      ch->set_lang( lang );
      if( ch->pcdata->learned[sn] == prct )
         ch->printf( "You begin lessons in %s.\r\n", lang_names[lang] );
      else if( ch->pcdata->learned[sn] < 60 )
         ch->printf( "You continue lessons in %s.\r\n", lang_names[lang] );
      else if( ch->pcdata->learned[sn] < 60 + prct )
         ch->printf( "You feel you can start communicating in %s.\r\n", lang_names[lang] );
      else if( ch->pcdata->learned[sn] < 99 )
         ch->printf( "You become more fluent in %s.\r\n", lang_names[lang] );
      else
         ch->printf( "You now speak perfect %s.\r\n", lang_names[lang] );
      return;
   }

   for( lang = 0; lang < LANG_UNKNOWN; ++lang )
   {
      if( knows_language( ch, lang, ch ) )
      {
         if( ch->speaking == lang || ( ch->isnpc(  ) && !ch->speaking ) )
            ch->set_color( AT_SAY );
         else
            ch->set_color( AT_PLAIN );
         ch->printf( "%s\r\n", lang_names[lang] );
      }
   }
   ch->print( "\r\n" );
}

std::string act_string( std::string_view format, char_data * to, char_data * ch, const void *arg1, const void *arg2, int flags )
{
   const char *he_she[] = { "it", "he", "she", "it" };
   const char *him_her[] = { "it", "him", "her", "it" };
   const char *his_her[] = { "its", "his", "her", "its" };
   std::string buf;
   bool should_upper = false;
   char_data *vch = ( char_data * ) arg2;
   obj_data *obj1 = ( obj_data * ) arg1;
   obj_data *obj2 = ( obj_data * ) arg2;

   if( format.empty(  ) )
   {
      bug( "%s: nullptr str!", __func__ );
      return "";
   }

   if( format[0] == '$' )
      DONT_UPPER = false;

   // No $ token in the string, return now with the contents as is.
   if( format.find( '$', 0 ) == std::string::npos )
   {
      buf = format;

      buf.append( "\r\n" );
      if( !DONT_UPPER )
         buf[0] = to_upper( buf[0] );
      return buf;
   }

   std::string_view::const_iterator ptr = format.begin(  );
   while( ptr != format.end(  ) )
   {
      if( *ptr == '.' || *ptr == '?' || *ptr == '!' )
         should_upper = true;
      else if( should_upper == true && !isspace( *ptr ) && *ptr != '$' )
         should_upper = false;

      if( *ptr != '$' )
      {
         buf.append( 1, *ptr );
         ++ptr;
         continue;
      }
      ++ptr;

      if( !arg2 && *ptr >= 'A' && *ptr <= 'Z' )
      {
         bug( "%s: missing arg2 for code %c:", __func__, *ptr );
         log_printf( "Missing arg2 came from %s", ch->name );
         if( ch->isnpc(  ) )
            log_printf( "NPC vnum: %d", ch->pIndexData->vnum );
         log_string( format );
         buf.append( " <@@@> " );
      }
      else
      {
         switch ( *ptr )
         {
            default:
               bug( "%s: bad code %c.", __func__, *ptr );
               log_printf( "Bad code came from %s", ch->name );
               buf.append( " <@@@> " );
               break;

            case 't':
               if( arg1 != nullptr )
                  buf.append( ( char * )arg1 );
               else
               {
                  bug( "%s: bad $t.", __func__ );
                  buf.append( " <@@@> " );
               }
               break;

            case 'T':
               if( arg2 != nullptr )
                  buf.append( ( char * )arg2 );
               else
               {
                  bug( "%s: bad $T.", __func__ );
                  buf.append( " <@@@> " );
               }
               break;

            case 'n':
               if( ch->morph == nullptr )
                  buf.append( ( to ? PERS( ch, to, false ) : NAME( ch ) ) );
               else if( flags == STRING_NONE )
                  buf.append( ( to ? MORPHPERS( ch, to, false ) : MORPHNAME( ch ) ) );
               else
               {
                  buf.append( std::format( "({}) {}", ( to ? PERS( ch, to, false ) : NAME( ch ) ), ( to ? MORPHPERS( ch, to, false ) : MORPHNAME( ch ) ) ) );
               }
               break;

            case 'N':
               if( vch->morph == nullptr )
                  buf.append( ( to ? PERS( vch, to, false ) : NAME( vch ) ) );
               else if( flags == STRING_NONE )
                  buf.append( ( to ? MORPHPERS( vch, to, false ) : MORPHNAME( vch ) ) );
               else
               {
                  buf.append( std::format( "({}) {}", ( to ? PERS( vch, to, false ) : NAME( vch ) ), ( to ? MORPHPERS( vch, to, false ) : MORPHNAME( vch ) ) ) );
               }
               break;

            case 'e':
               // Just silently correct
               if( ch->sex >= SEX_MAX || ch->sex < 0 )
                  ch->sex = SEX_NEUTRAL;
               if( should_upper )
                  buf.append( capitalize( he_she[urange( 0, ch->sex, SEX_MAX - 1 )] ) );
               else
                  buf.append( he_she[urange( 0, ch->sex, SEX_MAX - 1 )] );
               break;

            case 'E':
               // Just silently correct
               if( vch->sex >= SEX_MAX || vch->sex < 0 )
                  vch->sex = SEX_NEUTRAL;
               if( should_upper )
                  buf.append( capitalize( he_she[urange( 0, vch->sex, SEX_MAX - 1 )] ) );
               else
                  buf.append( he_she[urange( 0, vch->sex, SEX_MAX - 1 )] );
               break;

            case 'm':
               // Just silently correct
               if( ch->sex >= SEX_MAX || ch->sex < 0 )
                  ch->sex = SEX_NEUTRAL;
               if( should_upper )
                  buf.append( capitalize( him_her[urange( 0, ch->sex, SEX_MAX - 1 )] ) );
               else
                  buf.append( him_her[urange( 0, ch->sex, SEX_MAX - 1 )] );
               break;

            case 'M':
               // Just silently correct
               if( vch->sex >= SEX_MAX || vch->sex < 0 )
                  vch->sex = SEX_NEUTRAL;
               if( should_upper )
                  buf.append( capitalize( him_her[urange( 0, vch->sex, SEX_MAX - 1 )] ) );
               else
                  buf.append( him_her[urange( 0, vch->sex, SEX_MAX - 1 )] );
               break;

            case 's':
               // Just silently correct
               if( ch->sex >= SEX_MAX || ch->sex < 0 )
                  ch->sex = SEX_NEUTRAL;
               if( should_upper )
                  buf.append( capitalize( his_her[urange( 0, ch->sex, SEX_MAX - 1 )] ) );
               else
                  buf.append( his_her[urange( 0, ch->sex, SEX_MAX - 1 )] );
               break;

            case 'S':
               // Just silently correct
               if( vch->sex >= SEX_MAX || vch->sex < 0 )
                  vch->sex = SEX_NEUTRAL;
               if( should_upper )
                  buf.append( capitalize( his_her[urange( 0, vch->sex, SEX_MAX - 1 )] ) );
               else
                  buf.append( his_her[urange( 0, vch->sex, SEX_MAX - 1 )] );
               break;

            case 'q':
               buf.append( ( to == ch ) ? "" : "s" );
               break;

            case 'Q':
               buf.append( ( to == ch ) ? "your" : his_her[urange( 0, ch->sex, SEX_MAX - 1 )] );
               break;

            case 'p':
               buf.append( ( !obj1 ? "<BUG>" : ( !to || to->can_see_obj( obj1, false ) ? obj1->oshort(  ) : "something" ) ) );
               break;

            case 'P':
               buf.append( ( !obj2 ? "<BUG>" : ( !to || to->can_see_obj( obj2, false ) ? obj2->oshort(  ) : "something" ) ) );
               break;

            case 'd':
               if( !arg2 || ( ( char * )arg2 )[0] == '\0' )
                  buf.append( "door" );
               else
               {
                  std::string fname;

                  one_argument( reinterpret_cast<const char*>( arg2 ), fname );

                  buf.append( fname );
               }
               break;
         }
      }
      ++ptr;
   }

   buf.append( "\r\n" );
   if( !DONT_UPPER )
      buf[0] = to_upper( buf[0] );
   return buf;
}

void act( short AType, std::string_view format, char_data *ch, const void *arg1, const void *arg2, int type )
{
   std::string txt;
   char_data *to = nullptr;

   // Safely cast the const void* pointers
   auto *third = const_cast<char_data*>(static_cast<const char_data*>(arg1));
   auto *vch = const_cast<char_data*>(static_cast<const char_data*>(arg2));
   auto *obj1 = const_cast<obj_data*>(static_cast<const obj_data*>(arg1));
   auto *obj2 = const_cast<obj_data*>(static_cast<const obj_data*>(arg2));

   constexpr int ACTF_NONE = 0;
   constexpr int ACTF_TXT  = BV00;
   constexpr int ACTF_CH   = BV01;
   constexpr int ACTF_OBJ  = BV02;
   int flags1 = ACTF_NONE, flags2 = ACTF_NONE;

   // Discard null and zero-length messages.
   if( format.empty() )
      return;

   if( !ch )
   {
      bug( "%s: null ch. (%s)", __func__, format.data() );
      return;
   }

   // Do some proper type checking here..  Sort of.  We base it on the $* params.
   // This is kinda lame really, but I suppose in some weird sense it beats having
   // to pass like 8 different nullptr parameters every time we need to call act()..
   for( auto ptr = format.begin(); ptr != format.end(); ++ptr )
   {
      if( *ptr == '$' )
      {
         if( ++ptr == format.end() )
            break;

         switch ( *ptr )
         {
            default:
               bug( "Act: bad code %c for format %s.", *ptr, format.data() );
               break;

            case 't':
               flags1 |= ACTF_TXT;
               obj1 = nullptr;
               break;

            case 'T':
            case 'd':
               flags2 |= ACTF_TXT;
               vch = nullptr;
               obj2 = nullptr;
               break;

            case 'n': case 'e': case 'm': case 's': case 'q':
               break;

            case 'N': case 'E': case 'M': case 'S': case 'Q':
               flags2 |= ACTF_CH;
               obj2 = nullptr;
               break;

            case 'p':
               flags1 |= ACTF_OBJ;
               break;

            case 'P':
               flags2 |= ACTF_OBJ;
               vch = nullptr;
               break;
         }
      }
   }

   if( flags1 != ACTF_NONE && flags1 != ACTF_TXT && flags1 != ACTF_CH && flags1 != ACTF_OBJ )
   {
      bug( "%s: arg1 has more than one type in format %s. Setting all nullptr.", __func__, format.data() );
      obj1 = nullptr;
   }

   if( flags2 != ACTF_NONE && flags2 != ACTF_TXT && flags2 != ACTF_CH && flags2 != ACTF_OBJ )
   {
      bug( "%s: arg2 has more than one type in format %s. Setting all nullptr.", __func__, format.data() );
      vch = nullptr;
      obj2 = nullptr;
   }

   if( !ch->in_room )
   {
      bug( "%s: nullptr ch->in_room! (%s:%s)", __func__, ch->name, format.data() );
      return;
   }
   else if( type == TO_CHAR )
   {
      to = ch;
   }
   else if( type == TO_THIRD )
   {
      to = third;
   }
   else
   {
      to = ch->in_room->people.front();
   }

   // ACT_SECRETIVE handling
   if( ch->has_actflag( ACT_SECRETIVE ) && type != TO_CHAR )
      return;

   if( type == TO_VICT )
   {
      if( !vch )
      {
         bug( "%s: null vch with TO_VICT.", __func__ );
         log_printf( "%s (%s)", ch->name, format.data() );
         return;
      }
      if( !vch->in_room )
      {
         bug( "%s: vch in nullptr room!", __func__ );
         log_printf( "%s -> %s (%s)", ch->name, vch->name, format.data() );
         return;
      }

      if( is_ignoring( ch, vch ) )
      {
         /*
          * continue unless speaker is an immortal
          */
         if( !vch->is_immortal() || ch->level > vch->level )
            return;
         else
            ch->printf( "&[ignore]You attempt to ignore %s, but are unable to do so.\r\n", vch->name );
      }
      to = vch;
   }

   if( MOBtrigger && type != TO_CHAR && type != TO_VICT && to )
   {
      txt = act_string( format, nullptr, ch, arg1, arg2, STRING_IMM );
      if( HAS_PROG( to->in_room, ACT_PROG ) )
         rprog_act_trigger( txt, to->in_room, ch, obj1, vch, obj2 );
      for( auto* to_obj : to->in_room->objects )
      {
         if( HAS_PROG( to_obj->pIndexData, ACT_PROG ) )
            oprog_act_trigger( txt, to_obj, ch, obj1, vch, obj2 );
      }
   }

   if( !to )
   {
      bug( "%s: nullptr TARGET - CANNOT CONTINUE", __func__ );
      return;
   }

   /*
    * Anyone feel like telling me the point of looping through the whole
    * room when we're only sending to one char anyways..? -- Alty
    *
    * Because, silly, now we can use this sweet little bit of code to make
    * sure that messages to people on the maps go where they need to :P - Samson
    */
   for( auto* iterated_char : to->in_room->people )
   {
      to = iterated_char; // Strange as this may seem with changing "to" to the current iterated character, it seems to work. - Samson 1-9-04

      if( ( !to->desc && ( to->isnpc() && !HAS_PROG( to->pIndexData, ACT_PROG ) ) ) || !to->IS_AWAKE() )
         continue;

      // OasisOLC II check - Tagith
      if( to->desc && is_inolc( to->desc ) )
         continue;

      if( type == TO_CHAR && to != ch )
         continue;

      if( type == TO_THIRD && to != third )
         continue;

      if( type == TO_VICT && ( to != vch || to == ch ) )
         continue;

      if( type == TO_ROOM )
      {
         if( to == ch || is_ignoring( ch, to ) || !is_same_char_map( ch, to ) )
            continue;
      }

      if( type == TO_NOTVICT )
      {
         if( to == ch || to == vch || is_ignoring( ch, to ) ||
            ( vch != nullptr && is_ignoring( vch, to ) ) || !is_same_char_map( ch, to ) )
            continue;
      }

      if( type == TO_CANSEE )
      {
         if( to == ch || is_ignoring( ch, to ) || !is_same_char_map( ch, to ) )
            continue;

         if( ch->is_immortal() && ch->has_pcflag( PCFLAG_WIZINVIS ) )
         {
            if( to->level < ch->pcdata->wizinvis )
               continue;
         }
      }

      txt = to->is_immortal() ? act_string( format, to, ch, arg1, arg2, STRING_IMM ) : act_string( format, to, ch, arg1, arg2, STRING_NONE );

      if( to->desc )
      {
         to->set_color( AType );
         to->print( txt );
      }
      if( MOBtrigger )
      {
         // Note: use original string, not string with ANSI. -- Alty
         mprog_act_trigger( txt, to, ch, obj1, vch, obj2 );
      }
   }
   MOBtrigger = true;
}
