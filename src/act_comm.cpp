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
 *                      Player communication module                         *
 ****************************************************************************/

#include <cstdarg>
#include "mud.h"
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
char *mxp_chan_str( char_data *, const char * );
char *mxp_chan_str_close( char_data *, const char * );
void mprog_targetted_speech_trigger( char *, char_data *, char_data * );
void mprog_speech_trigger( char *, char_data * );
void mprog_and_speech_trigger( char *, char_data * );
board_data *find_board( char_data * );
board_data *get_board( char_data *, const char * );
char *mini_c_time( time_t, int );

lang_data *get_lang( const char *name )
{
   list<lang_data*>::iterator lng;

   for( lng = langlist.begin(); lng != langlist.end(); ++lng )
   {
      lang_data *lang = (*lng);

      if( !str_cmp( lang->name, name ) )
         return lang;
   }
   return NULL;
}

// Rewritten by Xorith for more C++
char *translate( int percent, const char *in, const char *name )
{
   lang_data *lng;
   string retVal = "", inVal = in;
   string::size_type inValIndex = 0;
   bool lfound = false;
   static char retBuf[MSL];

   retBuf[0] = '\0';

   if( percent > 99 || !str_cmp( name, "common" ) || 
      ( !( lng = get_lang( name ) ) && !( lng = get_lang( "default" ) ) ) )
   {
      mudstrlcpy( retBuf, inVal.c_str(), MSL );
      return retBuf;
   }

   for( inValIndex = 0; inValIndex < inVal.length(); ++inValIndex )
   {
      lfound = false;
      for( list<lcnv_data*>::iterator cnv = lng->prelist.begin(); cnv != lng->prelist.end(); ++cnv )
      {
         lcnv_data *conv = (*cnv);

         if( !str_prefix( conv->old, inVal.substr( inValIndex ) ) )
         {
            retVal += ( percent && ( rand( ) % 100 ) < percent ) ? conv->old : conv->lnew;
            inValIndex += conv->old.length();
            lfound = true;
            break;
         }
      }
      if( !lfound )
      {
         if( isalpha( inVal[inValIndex] ) && ( !percent || ( rand( ) % 100 ) > percent ) )
            retVal += lng->alphabet[tolower( inVal[inValIndex], locale() ) - 'a' ];
         else
            retVal += inVal[inValIndex];
      }
   }

   if( !lng->cnvlist.empty() )
   {
      for( lfound = false, inVal = retVal, retVal = "", inValIndex = 0; inValIndex < inVal.length(); ++inValIndex )
      {
         lfound = false;
         for( list<lcnv_data*>::iterator cnv = lng->cnvlist.begin(); cnv != lng->cnvlist.end(); ++cnv )
         {
            lcnv_data *conv = (*cnv);

            if( !str_prefix( conv->old, inVal.substr( inValIndex ) ) )
            {
               retVal += conv->lnew;
               inValIndex += conv->old.length();
               lfound = true;
               break;
            }
         }
         if( !lfound )
            retVal += inVal[inValIndex];
      }
   }
   mudstrlcpy( retBuf, retVal.c_str(), MSL );
   return retBuf;
}

char *drunk_speech( const char *argument, char_data * ch )
{
   const char *arg = argument;
   static char buf[MIL * 2];
   char buf1[MIL * 2];
   short drunk;
   char *txt;
   char *txt1;

   if( ch->isnpc(  ) )
   {
      mudstrlcpy( buf, argument, MIL * 2 );
      return buf;
   }

   drunk = ch->pcdata->condition[COND_DRUNK];

   if( drunk <= 0 )
   {
      mudstrlcpy( buf, argument, MIL * 2 );
      return buf;
   }

   buf[0] = '\0';
   buf1[0] = '\0';

   if( !argument )
   {
      bug( "%s: NULL argument", __FUNCTION__ );
      return "";
   }

   txt = buf;
   txt1 = buf1;

   while( *arg != '\0' )
   {
      if( toupper( *arg ) == 'T' )
      {
         if( number_percent(  ) < ( drunk * 2 ) )  /* add 'h' after an 'T' */
         {
            *txt++ = *arg;
            *txt++ = 'h';
         }
         else
            *txt++ = *arg;
      }
      else if( toupper( *arg ) == 'X' )
      {
         if( number_percent(  ) < ( drunk * 2 / 2 ) )
            *txt++ = 'c', *txt++ = 's', *txt++ = 'h';
         else
            *txt++ = *arg;
      }
      else if( number_percent(  ) < ( drunk * 2 / 5 ) )  /* slurred letters */
      {
         short slurn = number_range( 1, 2 );
         short currslur = 0;

         while( currslur < slurn )
            *txt++ = *arg, currslur++;
      }
      else
         *txt++ = *arg;

      ++arg;
   }

   *txt = '\0';

   txt = buf;

   while( *txt != '\0' )   /* Let's mess with the string's caps */
   {
      if( number_percent(  ) < ( 2 * drunk / 2.5 ) )
      {
         if( isupper( *txt ) )
            *txt1 = tolower( *txt );
         else if( islower( *txt ) )
            *txt1 = toupper( *txt );
         else
            *txt1 = *txt;
      }
      else
         *txt1 = *txt;

      txt1++, txt++;
   }

   *txt1 = '\0';
   txt1 = buf1;
   txt = buf;

   while( *txt1 != '\0' )  /* Let's make them stutter */
   {
      if( *txt1 == ' ' )   /* If there's a space, then there's gotta be a */
      {  /* along there somewhere soon */

         while( *txt1 == ' ' )   /* Don't stutter on spaces */
            *txt++ = *txt1++;

         if( ( number_percent(  ) < ( 2 * drunk / 4 ) ) && *txt1 != '\0' )
         {
            short offset = number_range( 0, 2 );
            short pos = 0;

            while( *txt1 != '\0' && pos < offset )
               *txt++ = *txt1++, pos++;

            if( *txt1 == ' ' )   /* Make sure not to stutter a space after */
            {  /* the initial offset into the word */
               *txt++ = *txt1++;
               continue;
            }

            pos = 0;
            offset = number_range( 2, 4 );
            while( *txt1 != '\0' && pos < offset )
            {
               *txt++ = *txt1;
               ++pos;
               if( *txt1 == ' ' || pos == offset ) /* Make sure we don't stick */
               {  /* A hyphen right before a space */
                  --txt1;
                  break;
               }
               *txt++ = '-';
            }
            if( *txt1 != '\0' )
               txt1++;
         }
      }
      else
         *txt++ = *txt1++;
   }
   *txt = '\0';
   return buf;
}

/*
 * Written by Kratas (moon@deathmoon.com)
 * Modified by Samson to be used in place of the ASK channel.
 */
CMDF( do_ask )
{
   char arg[MIL];
   char_data *victim;
   int speaking = -1;

   for( int lang = 0; lang < LANG_UNKNOWN; ++lang )
      if( ch->speaking == lang )
      {
         speaking = lang;
         break;
      }

   argument = one_argument( argument, arg );

   if( !arg || arg[0] == '\0' || !argument || argument[0] == '\0' )
   {
      ch->print( "Ask who what?\r\n" );
      return;
   }

   if( ( victim = ch->get_char_room( arg ) ) == NULL || ( victim->isnpc(  ) && victim->in_room != ch->in_room ) )
   {
      ch->print( "They aren't here.\r\n" );
      return;
   }

   if( ch->in_room->flags.test( ROOM_SILENCE ) )
   {
      ch->print( "You can't do that here.\r\n" );
      return;
   }

   if( victim == ch )
   {
      ch->print( "You ask yourself the question. Did it help?\r\n" );
      return;
   }

   bitset<MAX_ACT_FLAG> actflags = ch->get_actflags();
   if( ch->isnpc(  ) )
      ch->unset_actflag( ACT_SECRETIVE );

   char *sbuf = argument;

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

   if( speaking != -1 && ( !ch->isnpc(  ) || ch->speaking ) )
   {
      int speakswell = UMIN( knows_language( victim, ch->speaking, ch ), knows_language( ch, ch->speaking, victim ) );
      if( speakswell < 75 )
         sbuf = translate( speakswell, argument, lang_names[speaking] );
   }
   sbuf = drunk_speech( sbuf, ch );

   ch->set_actflags( actflags );
   MOBtrigger = false;

   act( AT_SAY, "You ask $N '$t'", ch, drunk_speech( argument, ch ), victim, TO_CHAR );
   act( AT_SAY, "$n asks you '$t'", ch, drunk_speech( argument, ch ), victim, TO_VICT );

   if( ch->in_room->flags.test( ROOM_LOGSPEECH ) )
      append_to_file( LOG_FILE, "%s: %s", ch->isnpc(  )? ch->short_descr : ch->name, argument );

   mprog_targetted_speech_trigger( argument, ch, victim );
   return;
}

void update_sayhistory( char_data * ch, char_data * vch, const char *msg )
{
   char new_msg[MSL];
   int x;

   snprintf( new_msg, MSL, "%ld %s%s said '%s'", current_time,
             ( vch == ch ? ch->color_str( AT_SAY ) : vch->color_str( AT_SAY ) ),
             vch == ch ? "You" : PERS( ch, vch, false ), msg );

   for( x = 0; x < MAX_SAYHISTORY; ++x )
   {
      if( vch->pcdata->say_history[x] == '\0' )
      {
         vch->pcdata->say_history[x] = str_dup( new_msg );
         break;
      }

      if( x == MAX_SAYHISTORY - 1 )
      {
         int i;

         for( i = 1; i < MAX_SAYHISTORY; ++i )
         {
            DISPOSE( vch->pcdata->say_history[i - 1] );
            vch->pcdata->say_history[i - 1] = str_dup( vch->pcdata->say_history[i] );
         }
         DISPOSE( vch->pcdata->say_history[x] );
         vch->pcdata->say_history[x] = str_dup( new_msg );
      }
   }
   return;
}

CMDF( do_say )
{
   int speaking = -1;

   for( int lang = 0; lang < LANG_UNKNOWN; ++lang )
      if( ch->speaking == lang )
      {
         speaking = lang;
         break;
      }

   if( !argument || argument[0] == '\0' )
   {
      if( ch->isnpc(  ) )
      {
         ch->print( "Say what?" );
         return;
      }

      ch->printf( "&cThe last %d things you heard said:\r\n", MAX_SAYHISTORY );

      for( int x = 0; x < MAX_SAYHISTORY; ++x )
      {
         char histbuf[MSL];
         if( ch->pcdata->say_history[x] == NULL )
            break;
         one_argument( ch->pcdata->say_history[x], histbuf );
         ch->printf( "&R[%s]%s\r\n", mini_c_time( atoi( histbuf ), ch->pcdata->timezone ),
                     ch->pcdata->say_history[x] + strlen( histbuf ) );
      }
      return;
   }

   if( ch->in_room->flags.test( ROOM_SILENCE ) )
   {
      ch->print( "You can't do that here.\r\n" );
      return;
   }

   bitset<MAX_ACT_FLAG> actflags = ch->get_actflags();
   if( ch->isnpc(  ) )
      ch->unset_actflag( ACT_SECRETIVE );

   /*
    * Inverts the speech of anyone carrying the burgundy amulet 
    */
   list<obj_data*>::iterator iobj;
   for( iobj = ch->carrying.begin(); iobj != ch->carrying.end(); ++iobj )
   {
      obj_data *obj = (*iobj);

      if( obj->pIndexData->vnum == 1405 ) /* The amulet itself */
      {
         char lbuf[MSL];

         invert_string( argument, lbuf );
         mudstrlcpy( argument, lbuf, MSL );
      }
   }

   list<char_data*>::iterator ich;
   for( ich = ch->in_room->people.begin(); ich != ch->in_room->people.end(); ++ich )
   {
      char_data *vch = (*ich);
      char *sbuf = argument;

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
            vch->printf( "&[ignore]You attempt to ignore %s, but are unable to do so.\r\n", ch->name );
      }
      if( speaking != -1 && ( !ch->isnpc(  ) || ch->speaking ) )
      {
         int speakswell = UMIN( knows_language( vch, ch->speaking, ch ), knows_language( ch, ch->speaking, vch ) );

         if( speakswell < 75 )
            sbuf = translate( speakswell, argument, lang_names[speaking] );
      }
      sbuf = drunk_speech( sbuf, ch );
      MOBtrigger = false;
      act_printf( AT_SAY, ch, sbuf, vch, TO_VICT,
                  "$n %s%s%ss '$t'", mxp_chan_str( vch, "say" ), "say", mxp_chan_str_close( vch, "say" ) );
      if( !vch->isnpc(  ) )
         update_sayhistory( ch, vch, sbuf );
   }
   ch->set_actflags( actflags );
   MOBtrigger = false;

   act_printf( AT_SAY, ch, NULL, drunk_speech( argument, ch ), TO_CHAR,
               "You %s%s%s '$T'", mxp_chan_str( ch, "say" ), "say", mxp_chan_str_close( ch, "say" ) );
   if( !ch->isnpc(  ) )
      update_sayhistory( ch, ch, drunk_speech( argument, ch ) );

   if( ch->in_room->flags.test( ROOM_LOGSPEECH ) )
      append_to_file( LOG_FILE, "%s: %s", ch->isnpc(  )? ch->short_descr : ch->name, argument );

   mprog_speech_trigger( argument, ch );
   mprog_and_speech_trigger( argument, ch );
   if( ch->char_died(  ) )
      return;
   oprog_speech_trigger( argument, ch );
   oprog_and_speech_trigger( argument, ch );
   if( ch->char_died(  ) )
      return;
   rprog_speech_trigger( argument, ch );
   rprog_and_speech_trigger( argument, ch );
   return;
}

CMDF( do_whisper )
{
   char arg[MIL];
   char_data *victim;
   int position;
   int speaking = -1, lang;

   for( lang = 0; lang < LANG_UNKNOWN; ++lang )
      if( ch->speaking == lang )
      {
         speaking = lang;
         break;
      }

   argument = one_argument( argument, arg );

   if( arg[0] == '\0' || argument[0] == '\0' )
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
      act( AT_PLAIN, "$E is currently in a writing buffer. Please try again in a few minutes.", ch, 0, victim, TO_CHAR );
      return;
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
         victim->printf( "&[ignore]You attempt to ignore %s, but are unable to do so.\r\n", ch->name );
   }

   MOBtrigger = false;
   act( AT_WHISPER, "You whisper to $N '$t'", ch, argument, victim, TO_CHAR );
   position = victim->position;
   victim->position = POS_STANDING;
   if( speaking != -1 && ( !ch->isnpc(  ) || ch->speaking ) )
   {
      int speakswell = UMIN( knows_language( victim, ch->speaking, ch ), knows_language( ch, ch->speaking, victim ) );

      if( speakswell < 85 )
         act( AT_WHISPER, "$n whispers to you '$t'", ch,
              translate( speakswell, argument, lang_names[speaking] ), victim, TO_VICT );
      else
         act( AT_WHISPER, "$n whispers to you '$t'", ch, argument, victim, TO_VICT );
   }
   else
      act( AT_WHISPER, "$n whispers to you '$t'", ch, argument, victim, TO_VICT );

   if( ch->in_room->flags.test( ROOM_SILENCE ) )
      act( AT_WHISPER, "$n whispers something to $N.", ch, argument, victim, TO_NOTVICT );

   victim->position = position;
   if( ch->in_room->flags.test( ROOM_LOGSPEECH ) )
      append_to_file( LOG_FILE, "%s: %s (whisper to) %s.", ch->isnpc(  )? ch->short_descr : ch->name, argument,
                      victim->isnpc(  )? victim->short_descr : victim->name );

   if( victim->isnpc(  ) )
   {
      mprog_speech_trigger( argument, ch );
      mprog_and_speech_trigger( argument, ch );
   }
   return;
}

/* Beep command courtesy of Altrag */
/* Installed by Samson on unknown date, allows user to beep other users */
CMDF( do_beep )
{
   char_data *victim = NULL;

   if( ch->pcdata->release_date != 0 )
   {
      ch->print( "Nope, no beeping from hell.\r\n" );
      return;
   }

   if( !argument || !( victim = ch->get_char_world( argument ) ) )
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

   /*
    * PCFLAG_NOBEEP check added by Samson 2-15-98 
    */
   if( victim->has_pcflag( PCFLAG_NOBEEP ) )
   {
      ch->printf( "%s is not accepting beeps at this time.\r\n", victim->name );
      return;
   }

   victim->printf( "%s is beeping you!\a\r\n", PERS( ch, victim, true ) );
   ch->printf( "You beep %s.\r\n", PERS( victim, ch, true ) );
   return;
}

void update_tellhistory( char_data * vch, char_data * ch, const char *msg, bool self )
{
   char_data *tch;
   char new_msg[MSL];
   int x;

   if( self )
   {
      snprintf( new_msg, MSL, "%ld %sYou told %s '%s'",
                current_time, ch->color_str( AT_TELL ), PERS( vch, ch, false ), msg );
      tch = ch;
   }
   else
   {
      snprintf( new_msg, MSL, "%ld %s%s told you '%s'",
                current_time, vch->color_str( AT_TELL ), PERS( ch, vch, false ), msg );
      tch = vch;
   }

   if( tch->isnpc(  ) )
      return;

   for( x = 0; x < MAX_TELLHISTORY; ++x )
   {
      if( !tch->pcdata->tell_history[x] || tch->pcdata->tell_history[x] == '\0' )
      {
         tch->pcdata->tell_history[x] = str_dup( new_msg );
         break;
      }

      if( x == MAX_TELLHISTORY - 1 )
      {
         int i;

         for( i = 1; i < MAX_TELLHISTORY; ++i )
         {
            DISPOSE( tch->pcdata->tell_history[i - 1] );
            tch->pcdata->tell_history[i - 1] = str_dup( tch->pcdata->tell_history[i] );
         }
         DISPOSE( tch->pcdata->tell_history[x] );
         tch->pcdata->tell_history[x] = str_dup( new_msg );
      }
   }
   return;
}

CMDF( do_tell )
{
   char arg[MIL];
   char_data *victim;
   int position;
   char_data *switched_victim = NULL;
   int speaking = -1, lang;

   for( lang = 0; lang < LANG_UNKNOWN; ++lang )
      if( ch->speaking == lang )
      {
         speaking = lang;
         break;
      }

   if( ch->in_room->flags.test( ROOM_SILENCE ) )
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

   if( arg[0] == '\0' || argument[0] == '\0' )
   {
      int x;

      if( ch->isnpc(  ) )
      {
         ch->print( "Tell who what?" );
         return;
      }

      ch->printf( "&cThe last %d things you were told:\r\n", MAX_TELLHISTORY );

      for( x = 0; x < MAX_TELLHISTORY; ++x )
      {
         char histbuf[MSL];
         if( ch->pcdata->tell_history[x] == NULL )
            break;
         one_argument( ch->pcdata->tell_history[x], histbuf );
         ch->printf( "&R[%s]%s\r\n", mini_c_time( atoi( histbuf ), ch->pcdata->timezone ),
                     ch->pcdata->tell_history[x] + strlen( histbuf ) );
      }
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
      ch->print( "That player is afk.\r\n" );
      return;
   }

   if( victim->has_pcflag( PCFLAG_NOTELL ) && !ch->is_immortal(  ) )
      /*
       * Immortal check added to let imms tell players at all times, Adjani, 12-02-2002
       */
   {
      act( AT_PLAIN, "$E has $S tells turned off.", ch, NULL, victim, TO_CHAR );
      return;
   }

   if( victim->has_pcflag( PCFLAG_SILENCE ) )
      ch->print( "That player is silenced.  They will receive your message but can not respond.\r\n" );

   if( !victim->isnpc(  ) && victim->has_aflag( AFF_SILENCE ) )
      ch->print( "That player has been magically muted!\r\n" );

   if( ( !ch->is_immortal(  ) && !victim->IS_AWAKE(  ) ) || ( !victim->isnpc(  ) && victim->in_room->flags.test( ROOM_SILENCE ) ) )
   {
      act( AT_PLAIN, "$E can't hear you.", ch, 0, victim, TO_CHAR );
      return;
   }

   if( victim->desc  /* make sure desc exists first  -Thoric */
       && victim->desc->connected == CON_EDITING && ch->get_trust(  ) < LEVEL_GOD )
   {
      act( AT_PLAIN, "$E is currently in a writing buffer. Please try again in a few minutes.", ch, 0, victim, TO_CHAR );
      return;
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
         victim->printf( "&[ignore]You attempt to ignore %s, but are unable to do so.\r\n", ch->name );
   }

   if( switched_victim )
      victim = switched_victim;

   MOBtrigger = false;  /* BUGFIX - do_tell: Tells were triggering act progs */

   act( AT_TELL, "You tell $N '$t'", ch, argument, victim, TO_CHAR );
   update_tellhistory( victim, ch, argument, true );
   position = victim->position;
   victim->position = POS_STANDING;
   if( speaking != -1 && ( !ch->isnpc(  ) || ch->speaking ) )
   {
      int speakswell = UMIN( knows_language( victim, ch->speaking, ch ), knows_language( ch, ch->speaking, victim ) );

      if( speakswell < 85 )
      {
         act( AT_TELL, "$n tells you '$t'", ch, translate( speakswell, argument, lang_names[speaking] ), victim, TO_VICT );
         update_tellhistory( victim, ch, translate( speakswell, argument, lang_names[speaking] ), false );
      }
      else
      {
         act( AT_TELL, "$n tells you '$t'", ch, argument, victim, TO_VICT );
         update_tellhistory( victim, ch, argument, false );
      }
   }
   else
   {
      act( AT_TELL, "$n tells you '$t'", ch, argument, victim, TO_VICT );
      update_tellhistory( victim, ch, argument, false );
   }

   MOBtrigger = true;   /* BUGFIX - do_tell: Tells were triggering act progs */

   victim->position = position;
   victim->reply = ch;

   if( ch->in_room->flags.test( ROOM_LOGSPEECH ) )
      append_to_file( LOG_FILE, "%s: %s (tell to) %s.", ch->isnpc(  )? ch->short_descr : ch->name,
                      argument, victim->isnpc(  )? victim->short_descr : victim->name );

   if( victim->isnpc(  ) )
   {
      mprog_speech_trigger( argument, ch );
      mprog_and_speech_trigger( argument, ch );
   }
   return;
}

CMDF( do_reply )
{
   char_data *victim;

   if( !( find_board( ch ) ) )
   {
      char arg[MIL]; /* Placed this here since it's only used here -- X */
      if( ( is_number( one_argument( argument, arg ) ) ) && ( get_board( ch, arg ) ) )
      {
         cmdf( ch, "write %s", argument );
         return;
      }
   }
   else if( is_number( argument ) && argument[0] != '\0' )
   {
      cmdf( ch, "write %s", argument );
      return;
   }

   if( !( victim = ch->reply ) )
   {
      ch->print( "Either you have nothing to reply to, or that person has left.\r\n" );
      return;
   }

   if( !argument || argument[0] == '\0' )
   {
      ch->printf( "And what would you like to say in reply to %s?\r\n", victim->name );
      return;
   }

   /*
    * This is a bit shorter than what was here before, no? Accomplished the same bloody thing too. -- Xorith 
    */
   cmdf( ch, "tell %s %s", victim->name, argument );
   return;
}

CMDF( do_emote )
{
   char buf[MSL];
   char *plast;
   char_data *vch;
   int speaking = -1;

   for( int lang = 0; lang < LANG_UNKNOWN; ++lang )
      if( ch->speaking == lang )
      {
         speaking = lang;
         break;
      }

   /*
    * Per Alcane's notice, emote no longer works in silent rooms - Samson 1-14-00 
    */
   if( ch->in_room->flags.test( ROOM_SILENCE ) )
   {
      ch->print( "The room is magically silenced! You cannot express emotions!\r\n" );
      return;
   }

   if( ch->has_pcflag( PCFLAG_NO_EMOTE ) )
   {
      ch->print( "You can't show your emotions.\r\n" );
      return;
   }

   if( !argument || argument[0] == '\0' )
   {
      ch->print( "Emote what?\r\n" );
      return;
   }

   bitset<MAX_ACT_FLAG> actflags = ch->get_actflags();
   if( ch->isnpc(  ) )
      ch->unset_actflag( ACT_SECRETIVE );
   for( plast = argument; *plast != '\0'; ++plast )
      ;

   mudstrlcpy( buf, argument, MSL );
   if( isalpha( plast[-1] ) )
      mudstrlcat( buf, ".", MSL );

   list<char_data*>::iterator ich;
   for( ich = ch->in_room->people.begin(); ich != ch->in_room->people.end(); ++ich )
   {
      vch = (*ich);
      char *sbuf = buf;

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
            vch->printf( "&[ignore]You attempt to ignore %s, but are unable to do so.\r\n", ch->name );
      }
      if( speaking != -1 && ( !ch->isnpc(  ) || ch->speaking ) )
      {
         int speakswell = UMIN( knows_language( vch, ch->speaking, ch ), knows_language( ch, ch->speaking, vch ) );

         if( speakswell < 85 )
            sbuf = translate( speakswell, argument, lang_names[speaking] );
      }
      MOBtrigger = false;
      act( AT_SOCIAL, "$n $t", ch, sbuf, vch, ( vch == ch ? TO_CHAR : TO_VICT ) );
   }
   ch->set_actflags( actflags );
   if( ch->in_room->flags.test( ROOM_LOGSPEECH ) )
      append_to_file( LOG_FILE, "%s %s (emote)", ch->isnpc(  )? ch->short_descr : ch->name, argument );
   return;
}

/* 0 = bug 1 = idea 2 = typo */
void tybuid( char_data * ch, char *argument, int type )
{
   struct tm *t = localtime( &current_time );
   static char *const tybuid_name[] = { "bug", "idea", "typo" };
   static char *const tybuid_file[] = { PBUG_FILE, IDEA_FILE, TYPO_FILE };

   ch->set_color( AT_PLAIN );

   if( !argument || argument[0] == '\0' )
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
         bug( "%s: unable to stat %s file '%s'!", __FUNCTION__, tybuid_name[type], tybuid_file[type] );
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

   append_file( ch, tybuid_file[type], "(%-2.2d/%-2.2d):  %s", t->tm_mon + 1, t->tm_mday, argument );
   ch->printf( "Thank you! Your %s has been recorded.\r\n", tybuid_name[type] );
   return;
}

CMDF( do_bug )
{
   /*
    * 0 = bug 
    */
   tybuid( ch, argument, 0 );
   return;
}

CMDF( do_ide )
{
   ch->print( "&YIf you want to send an idea, type 'idea <message>'.\r\n" );
   ch->print( "If you want to identify an object, use the identify spell.\r\n" );
   return;
}

CMDF( do_idea )
{
   /*
    * 1 = idea 
    */
   tybuid( ch, argument, 1 );
   return;
}

CMDF( do_typo )
{
   /*
    * 2 = typo 
    */
   tybuid( ch, argument, 2 );
   return;
}

/*
 * 'Split' originally by Gnort, God of Chaos.
 */
CMDF( do_split )
{
   if( !argument || argument[0] == '\0' || !is_number( argument ) )
   {
      ch->print( "Split how much?\r\n" );
      return;
   }

   int amount = atoi( argument );

   if( amount < 0 )
   {
      ch->print( "Your group wouldn't like that.\r\n" );
      return;
   }

   if( amount == 0 )
   {
      ch->print( "You hand out zero coins, but no one notices.\r\n" );
      return;
   }

   if( ch->gold < amount )
   {
      ch->print( "You don't have that much gold.\r\n" );
      return;
   }

   int members = 0;
   list<char_data*>::iterator ich;
   for( ich = ch->in_room->people.begin(); ich != ch->in_room->people.end(); ++ich )
   {
      char_data *gch = (*ich);

      if( is_same_group( gch, ch ) )
         ++members;
   }

   if( ch->has_pcflag( PCFLAG_AUTOGOLD ) && members < 2 )
      return;

   if( members < 2 )
   {
      ch->print( "Just keep it all.\r\n" );
      return;
   }

   int share = amount / members;
   int extra = amount % members;

   if( share == 0 )
   {
      ch->print( "Don't even bother, cheapskate.\r\n" );
      return;
   }

   ch->gold -= amount;
   ch->gold += share + extra;

   ch->printf( "&[gold]You split %d gold coins. Your share is %d gold coins.\r\n", amount, share + extra );

   for( ich = ch->in_room->people.begin(); ich != ch->in_room->people.end(); ++ich )
   {
      char_data *gch = (*ich);

      if( gch != ch && is_same_group( gch, ch ) )
      {
         act_printf( AT_GOLD, ch, NULL, gch, TO_VICT, "$n splits %d gold coins. Your share is %d gold coins.", amount,
                     share );
         gch->gold += share;
      }
   }
   return;
}

CMDF( do_gtell )
{
   int speaking = -1;

   for( int lang = 0; lang < LANG_UNKNOWN; ++lang )
      if( ch->speaking == lang )
      {
         speaking = lang;
         break;
      }

   if( !argument || argument[0] == '\0' )
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
   list<char_data*>::iterator ich;
   for( ich = pclist.begin(); ich != pclist.end(); ++ich )
   {
      char_data *gch = (*ich);

      if( is_same_group( gch, ch ) )
      {
         if( speaking != -1 && ( !ch->isnpc(  ) || ch->speaking ) )
         {
            int speakswell = UMIN( knows_language( gch, ch->speaking, ch ), knows_language( ch, ch->speaking, gch ) );

            if( speakswell < 85 )
            {
               if( gch == ch )
                  gch->printf( "&[gtells]You tell the group '%s'\r\n",
                               translate( speakswell, argument, lang_names[speaking] ) );
               else
                  gch->printf( "&[gtells]%s tells the group '%s'\r\n", ch->name,
                               translate( speakswell, argument, lang_names[speaking] ) );
            }
            else
            {
               if( gch == ch )
                  gch->printf( "&[gtells]You tell the group '%s'\r\n", argument );
               else
                  gch->printf( "&[gtells]%s tells the group '%s'\r\n", ch->name, argument );
            }
         }
         else
         {
            if( gch == ch )
               gch->printf( "&[gtells]You tell the group '%s'\r\n", argument );
            else
               gch->printf( "&[gtells]%s tells the group '%s'\r\n", ch->name, argument );
         }
      }
   }
   return;
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
   short sn;

   if( !ch->isnpc(  ) && ch->is_immortal(  ) )
      return 100;

   if( ch->isnpc(  ) && !ch->has_langs() )  /* No langs = knows all for npcs */
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

      if( ch->pcdata->clan == cch->pcdata->clan && ch->pcdata->clan != NULL )
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
         if( ( sn = skill_lookup( lang_names[language] ) ) != -1 )
            return ch->pcdata->learned[sn];
      }
   }
   return 0;
}

CMDF( do_speak )
{
   int lang = -1;

   if( !argument || argument[0] == '\0' )
   {
      ch->print( "&[say]Speak what?\r\n" );
      return;
   }

   lang = get_langnum( argument );
   if( lang < 0 || lang >= LANG_UNKNOWN )
   {
      ch->printf( "&[say]%s is not a known tongue on this world.\r\n", argument );
      return;
   }

   if( lang == LANG_CLAN && ( ch->isnpc(  ) || !ch->pcdata->clan ) )
   {
      ch->printf( "&[say]The %s tongue is only available to guilds and clans.\r\n", argument );
      return;
   }

   if( !ch->is_immortal(  ) && !ch->has_lang( lang ) )
   {
      ch->printf( "&[say]But you have not learned the %s tongue!\r\n", argument );
      return;
   }
   ch->speaking = lang;
   ch->printf( "&[say]You are now speaking the %s tongue.\r\n", argument );
   return;
}

CMDF( do_languages )
{
   char arg[MIL];
   int lang;

   argument = one_argument( argument, arg );
   if( arg[0] != '\0' && !str_prefix( arg, "learn" ) && !ch->is_immortal(  ) && !ch->isnpc(  ) )
   {
      char_data *sch = NULL;
      int sn, prct, prac;
      bool found = false;

      if( !argument || argument[0] == '\0' )
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
         act( AT_PLAIN, "You are already fluent in $t.", ch, lang_names[lang], NULL, TO_CHAR );
         return;
      }

      /*
       * Bug fix - Samson 12-25-98 
       */
      list<char_data*>::iterator ich;
      for( ich = ch->in_room->people.begin(); ich != ch->in_room->people.end(); ++ich )
      {
         sch = (*ich);

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
         act( AT_TELL, "$n tells you 'You do not have enough practices.'", sch, NULL, ch, TO_VICT );
         return;
      }
      ch->pcdata->practice -= prac;
      /*
       * Max 12% (5 + 4 + 3) at 24+ int and 21+ wis. -- Altrag 
       */
      prct = 5 + ( ch->get_curr_int(  ) / 6 ) + ( ch->get_curr_wis(  ) / 7 );
      ch->pcdata->learned[sn] += prct;
      ch->pcdata->learned[sn] = UMIN( ch->pcdata->learned[sn], 99 );
      ch->set_lang( lang );
      if( ch->pcdata->learned[sn] == prct )
         act( AT_PLAIN, "You begin lessons in $t.", ch, lang_names[lang], NULL, TO_CHAR );
      else if( ch->pcdata->learned[sn] < 60 )
         act( AT_PLAIN, "You continue lessons in $t.", ch, lang_names[lang], NULL, TO_CHAR );
      else if( ch->pcdata->learned[sn] < 60 + prct )
         act( AT_PLAIN, "You feel you can start communicating in $t.", ch, lang_names[lang], NULL, TO_CHAR );
      else if( ch->pcdata->learned[sn] < 99 )
         act( AT_PLAIN, "You become more fluent in $t.", ch, lang_names[lang], NULL, TO_CHAR );
      else
         act( AT_PLAIN, "You now speak perfect $t.", ch, lang_names[lang], NULL, TO_CHAR );
      return;
   }
   for( lang = 0; lang < LANG_UNKNOWN; ++lang )
      if( knows_language( ch, lang, ch ) )
      {
         if( ch->speaking == lang || ( ch->isnpc(  ) && !ch->speaking ) )
            ch->set_color( AT_SAY );
         else
            ch->set_color( AT_PLAIN );
         ch->printf( "%s\r\n", lang_names[lang] );
      }
   ch->print( "\r\n" );
   return;
}

#define NAME(ch)        ( ch->isnpc() ? ch->short_descr : ch->name )

char *MORPHNAME( char_data *ch )
{
   if( ch->morph && ch->morph->morph && ch->morph->morph->short_desc != NULL )
      return ch->morph->morph->short_desc;
   else
      return NAME(ch);
}

char *act_string( const char *format, char_data * to, char_data * ch, void *arg1, void *arg2, int flags )
{
   static char *const he_she[] = { "it", "he", "she", "it" };
   static char *const him_her[] = { "it", "him", "her", "it" };
   static char *const his_her[] = { "its", "his", "her", "its" };
   static char buf[MSL];
   char fname[MIL], temp[MSL];
   char *point = buf;
   const char *str = format;
   const char *i;
   char_data *vch = ( char_data * ) arg2;
   obj_data *obj1 = ( obj_data * ) arg1;
   obj_data *obj2 = ( obj_data * ) arg2;

   if( !str )
   {
      bug( "%s: NULL str!", __FUNCTION__ );
      return "";
   }

   if( str[0] == '$' )
      DONT_UPPER = false;

   while( *str != '\0' )
   {
      if( *str != '$' )
      {
         *point++ = *str++;
         continue;
      }
      ++str;
      if( !arg2 && *str >= 'A' && *str <= 'Z' )
      {
         bug( "%s: missing arg2 for code %c:", __FUNCTION__, *str );
         log_printf( "Missing arg2 came from %s", ch->name );
         if( ch->isnpc(  ) )
            log_printf( "NPC vnum: %d", ch->pIndexData->vnum );
         log_string( format );
         i = " <@@@> ";
      }
      else
      {
         switch ( *str )
         {
            default:
               bug( "%s: bad code %c.", __FUNCTION__, *str );
               log_printf( "Bad code came from %s", ch->name );
               i = " <@@@> ";
               break;

            case 't':
               if( arg1 != NULL )
                  i = ( char * )arg1;
               else
               {
                  bug( "%s: bad $t.", __FUNCTION__ );
                  i = " <@@@> ";
               }
               break;

            case 'T':
               if( arg2 != NULL )
                  i = ( char * )arg2;
               else
               {
                  bug( "%s: bad $T.", __FUNCTION__ );
                  i = " <@@@> ";
               }
               break;

            case 'n':
               if( ch->morph == NULL )
                  i = ( to ? PERS( ch, to, false ) : NAME( ch ) );
               else if( flags == STRING_NONE )
                  i = ( to ? MORPHPERS( ch, to, false ) : MORPHNAME( ch ) );
               else
               {
                  snprintf( temp, MSL, "(%s) %s", ( to ? PERS( ch, to, false ) : NAME( ch ) ),
                            ( to ? MORPHPERS( ch, to, false ) : MORPHNAME( ch ) ) );
                  i = temp;
               }
               break;

            case 'N':
               if( vch->morph == NULL )
                  i = ( to ? PERS( vch, to, false ) : NAME( vch ) );
               else if( flags == STRING_NONE )
                  i = ( to ? MORPHPERS( vch, to, false ) : MORPHNAME( vch ) );
               else
               {
                  snprintf( temp, MSL, "(%s) %s", ( to ? PERS( vch, to, false ) : NAME( vch ) ),
                            ( to ? MORPHPERS( vch, to, false ) : MORPHNAME( vch ) ) );
                  i = temp;
               }
               break;

            case 'e':
               if( ch->sex > SEX_HERMAPHRODYTE || ch->sex < 0 )
               {
                  bug( "%s: player %s has sex set at %d!", __FUNCTION__, ch->name, ch->sex );
                  i = "it";
               }
               else
                  i = he_she[URANGE( 0, ch->sex, 2 )];
               break;

            case 'E':
               if( vch->sex > SEX_HERMAPHRODYTE || vch->sex < 0 )
               {
                  bug( "%s: player %s has sex set at %d!", __FUNCTION__, vch->name, vch->sex );
                  i = "it";
               }
               else
                  i = he_she[URANGE( 0, vch->sex, 2 )];
               break;

            case 'm':
               if( ch->sex > SEX_HERMAPHRODYTE || ch->sex < 0 )
               {
                  bug( "%s: player %s has sex set at %d!", __FUNCTION__, ch->name, ch->sex );
                  i = "it";
               }
               else
                  i = him_her[URANGE( 0, ch->sex, 2 )];
               break;

            case 'M':
               if( vch->sex > SEX_HERMAPHRODYTE || vch->sex < 0 )
               {
                  bug( "%s: player %s has sex set at %d!", __FUNCTION__, vch->name, vch->sex );
                  i = "it";
               }
               else
                  i = him_her[URANGE( 0, vch->sex, 2 )];
               break;

            case 's':
               if( ch->sex > SEX_HERMAPHRODYTE || ch->sex < 0 )
               {
                  bug( "%s: player %s has sex set at %d!", __FUNCTION__, ch->name, ch->sex );
                  i = "its";
               }
               else
                  i = his_her[URANGE( 0, ch->sex, 2 )];
               break;

            case 'S':
               if( vch->sex > SEX_HERMAPHRODYTE || vch->sex < 0 )
               {
                  bug( "%s: player %s has sex set at %d!", __FUNCTION__, vch->name, vch->sex );
                  i = "its";
               }
               else
                  i = his_her[URANGE( 0, vch->sex, 2 )];
               break;

            case 'q':
               i = ( to == ch ) ? "" : "s";
               break;

            case 'Q':
               i = ( to == ch ) ? "your" : his_her[URANGE( 0, ch->sex, 2 )];
               break;

            case 'p':
               i = ( !obj1 ? "<BUG>" : ( !to || to->can_see_obj( obj1, false ) ? obj1->oshort(  ) : "something" ) );
               break;

            case 'P':
               i = ( !obj2 ? "<BUG>" : ( !to || to->can_see_obj( obj2, false ) ? obj2->oshort(  ) : "something" ) );
               break;

            case 'd':
               if( !arg2 || ( ( char * )arg2 )[0] == '\0' )
                  i = "door";
               else
               {
                  one_argument( ( char * )arg2, fname );
                  i = fname;
               }
               break;
         }
      }
      ++str;
      while( ( *point = *i ) != '\0' )
         ++point, ++i;
   }
   mudstrlcpy( point, "\r\n", MSL - ( point - buf ) );
   if( !DONT_UPPER )
      buf[0] = UPPER( buf[0] );
   return buf;
}

#undef NAME

void act_printf( short AType, char_data * ch, void *arg1, void *arg2, int type, const char *str, ... )
{
   va_list arg;
   char format[MSL * 2];

   // Discard null and zero-length messages.
   if( !str || str[0] == '\0' )
      return;

   va_start( arg, str );
   vsnprintf( format, MSL * 2, str, arg );
   va_end( arg );

   act( AType, format, ch, arg1, arg2, type );
}

void act( short AType, const char *format, char_data * ch, void *arg1, void *arg2, int type )
{
   char *txt;
   char_data *to;
   char_data *third = ( char_data * ) arg1;
   char_data *vch = ( char_data * ) arg2;

   // Discard null and zero-length messages.
   if( !format || format[0] == '\0' )
      return;

   if( !ch )
   {
      bug( "%s: null ch. (%s)", __FUNCTION__, format );
      return;
   }

   if( !ch->in_room )
   {
      bug( "%s: NULL ch->in_room! (%s:%s)", __FUNCTION__, ch->name, format );
      return;
   }
   else if( type == TO_CHAR )
      to = ch;
   else if( type == TO_THIRD )
      to = third;
   else
      to = (*ch->in_room->people.begin());

   // ACT_SECRETIVE handling
   if( ch->has_actflag( ACT_SECRETIVE ) && type != TO_CHAR )
      return;

   if( type == TO_VICT )
   {
      if( !vch )
      {
         bug( "%s: null vch with TO_VICT.", __FUNCTION__ );
         log_printf( "%s (%s)", ch->name, format );
         return;
      }
      if( !vch->in_room )
      {
         bug( "%s: vch in NULL room!", __FUNCTION__ );
         log_printf( "%s -> %s (%s)", ch->name, vch->name, format );
         return;
      }

      if( is_ignoring( ch, vch ) )
      {
         /*
          * continue unless speaker is an immortal 
          */
         if( !vch->is_immortal(  ) || ch->level > vch->level )
            return;
         else
            ch->printf( "&[ignore]You attempt to ignore %s, but are unable to do so.\r\n", vch->name );
      }
      to = vch;
   }

   if( MOBtrigger && type != TO_CHAR && type != TO_VICT && to )
   {
      obj_data *to_obj;
      list<obj_data*>::iterator iobj;

      txt = act_string( format, NULL, ch, arg1, arg2, STRING_IMM );
      if( HAS_PROG( to->in_room, ACT_PROG ) )
         rprog_act_trigger( txt, to->in_room, ch, ( obj_data * ) arg1, ( void * )arg2 );
      for( iobj = to->in_room->objects.begin(); iobj != to->in_room->objects.end(); ++iobj )
      {
         to_obj = (*iobj);

         if( HAS_PROG( to_obj->pIndexData, ACT_PROG ) )
            oprog_act_trigger( txt, to_obj, ch, ( obj_data * ) arg1, ( void * )arg2 );
      }
   }

   /*
    * Anyone feel like telling me the point of looping through the whole
    * room when we're only sending to one char anyways..? -- Alty 
    *
    * Because, silly, now we can use this sweet little bit of code to make
    * sure that messages to people on the maps go where they need to :P - Samson 
    */
   if( !to )
   {
      bug( "%s: NULL TARGET - CANNOT CONTINUE", __FUNCTION__ );
      return;
   }

   // Strange as this may seem with changing "to" to the current iterated character, it seems to work. - Samson 1-9-04
   list<char_data*>::iterator ich;
   for( ich = to->in_room->people.begin(); ich != to->in_room->people.end(); )
   {
      to = (*ich);
      ++ich;

      if( ( !to->desc && ( to->isnpc(  ) && !HAS_PROG( to->pIndexData, ACT_PROG ) ) ) || !to->IS_AWAKE(  ) )
         continue;

      // OasisOLC II check - Tagith 
      if( to->desc && is_inolc( to->desc ) )
         continue;

      if( type == TO_CHAR )
      {
         if( to != ch )
            continue;
      }

      if( type == TO_THIRD && to != third )
         continue;

      if( type == TO_VICT && ( to != vch || to == ch ) )
         continue;

      if( type == TO_ROOM )
      {
         if( to == ch )
            continue;

         if( is_ignoring( ch, to ) )
            continue;

         if( !is_same_char_map( ch, to ) )
            continue;
      }

      if( type == TO_NOTVICT )
      {
         if( to == ch || to == vch )
            continue;

         if( is_ignoring( ch, to ) )
            continue;

         if( vch != NULL && is_ignoring( vch, to ) )
            continue;

         if( !is_same_char_map( ch, to ) )
            continue;
      }

      if( type == TO_CANSEE )
      {
         if( to == ch )
            continue;

         if( ch->is_immortal(  ) && ch->has_pcflag( PCFLAG_WIZINVIS ) )
         {
            if( to->level < ch->pcdata->wizinvis )
               continue;
         }

         if( is_ignoring( ch, to ) )
            continue;

         if( !is_same_char_map( ch, to ) )
            continue;
      }

      if( to->is_immortal(  ) )
         txt = act_string( format, to, ch, arg1, arg2, STRING_IMM );
      else
         txt = act_string( format, to, ch, arg1, arg2, STRING_NONE );

      if( to->desc )
      {
         to->set_color( AType );
         to->print( txt );
      }
      if( MOBtrigger )
      {
         // Note: use original string, not string with ANSI. -- Alty 
         mprog_act_trigger( txt, to, ch, ( obj_data * ) arg1, ( void * )arg2 );
      }
   }
   MOBtrigger = true;
   return;
}
