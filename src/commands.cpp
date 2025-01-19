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
 *                              Command Code                                *
 ****************************************************************************/

#include <fstream>
#include <sys/time.h>
#include <cstdarg>
#if !defined(WIN32)
#include <dlfcn.h>
#else
#include <windows.h>
#define dlsym( handle, name ) ( (void*)GetProcAddress( (HINSTANCE) (handle), (name) ) )
#define dlerror() GetLastError()
#endif
#include "mud.h"
#include "clans.h"
#include "commands.h"
#include "descriptor.h"
#include "mobindex.h"
#include "mud_prog.h"
#include "roomindex.h"

vector < vector < cmd_type * > >command_table;

extern char lastplayercmd[MIL * 2];
#if defined(WIN32)
void gettimeofday( struct timeval *, struct timezone * );
#endif

/*
 * Log-all switch.
 */
bool fLogAll = false;

/*
 * Externals
 */
void subtract_times( struct timeval *, struct timeval * );
bool check_ability( char_data *, const string &, const string & );
bool check_skill( char_data *, const string &, const string & );
#ifdef MULTIPORT
bool shell_hook( char_data *, const string &, string & );
void shellcommands( char_data *, short );
#endif
bool local_channel_hook( char_data *, const string &, string & );
string extract_area_names( char_data * );
bool can_use_mprog( char_data * );
bool mprog_command_trigger( char_data *, const string & );
bool oprog_command_trigger( char_data *, const string & );
bool rprog_command_trigger( char_data *, const string & );

const char *cmd_flags[] = {
   "possessed", "polymorphed", "action", "nospam", "ghost", "mudprog",
   "noforce", "loaded", "noabort"
};

/*
 * For use with cedit --Shaddai
 */
int get_cmdflag( const string & flag )
{
   for( size_t x = 0; x < ( sizeof( cmd_flags ) / sizeof( cmd_flags[0] ) ); ++x )
      if( !str_cmp( flag.c_str(  ), cmd_flags[x] ) )
         return x;
   return -1;
}

void check_switches(  )
{
   list < char_data * >::iterator ich;

   for( ich = pclist.begin(  ); ich != pclist.end(  ); ++ich )
   {
      char_data *ch = *ich;

      check_switch( ch );
   }
}

CMDF( do_switch );
void check_switch( char_data * ch )
{
   if( !ch->switched )
      return;

   for( char x = 0; x < 126; ++x )
   {
      const vector < cmd_type * >&cmd_list = command_table[x];
      vector < cmd_type * >::const_iterator icmd;

      for( icmd = cmd_list.begin(  ); icmd != cmd_list.end(  ); ++icmd )
      {
         cmd_type *cmd = *icmd;

         if( cmd->do_fun != do_switch )
            continue;
         if( cmd->level <= ch->get_trust(  ) )
            return;

         if( !ch->isnpc(  ) && !ch->pcdata->bestowments.empty(  ) && hasname( ch->pcdata->bestowments, cmd->name ) && cmd->level <= ch->get_trust(  ) + sysdata->bestow_dif )
            return;
      }
   }

   ch->switched->set_color( AT_BLUE );
   ch->switched->print( "You suddenly forfeit the power to switch!\r\n" );
   interpret( ch->switched, "return" );
}

int check_command_level( const string & arg, int check )
{
   cmd_type *cmd = find_command( arg );

   if( !cmd )
      return LEVEL_IMMORTAL;

   if( check < cmd->level )
      return -1;

   return cmd->level;
}

/*
 *  This function checks the command against the command flags to make
 *  sure they can use the command online.  This allows the commands to be
 *  edited online to allow or disallow certain situations.  May be an idea
 *  to rework this so we can edit the message sent back online, as well as
 *  maybe a crude parsing language so we can add in new checks online without
 *  haveing to hard-code them in.     -- Shaddai   August 25, 1997
 */

/* Needed a global here */
char cmd_flag_buf[MSL];

char *check_cmd_flags( char_data * ch, cmd_type * cmd )
{
   if( ch->has_aflag( AFF_POSSESS ) && cmd->flags.test( CMD_POSSESS ) )
      snprintf( cmd_flag_buf, MSL, "You can't %s while you are possessing someone!\r\n", cmd->name.c_str(  ) );
   else if( ch->morph != nullptr && cmd->flags.test( CMD_POLYMORPHED ) )
      snprintf( cmd_flag_buf, MSL, "You can't %s while you are polymorphed!\r\n", cmd->name.c_str(  ) );
   else
      cmd_flag_buf[0] = '\0';
   return cmd_flag_buf;
}

void start_timer( struct timeval *starttime )
{
   if( !starttime )
   {
      bug( "%s: nullptr stime.", __func__ );
      return;
   }
   gettimeofday( starttime, nullptr );
   return;
}

time_t end_timer( struct timeval * starttime )
{
   struct timeval etime;

   /*
    * Mark etime before checking stime, so that we get a better reading.. 
    */
   gettimeofday( &etime, nullptr );
   if( !starttime || ( !starttime->tv_sec && !starttime->tv_usec ) )
   {
      bug( "%s: bad starttime.", __func__ );
      return 0;
   }
   subtract_times( &etime, starttime );
   /*
    * stime becomes time used 
    */
   *starttime = etime;
   return ( etime.tv_sec * 1000000 ) + etime.tv_usec;
}

bool check_social( char_data * ch, const string & command, const string & argument )
{
   string arg;
   social_type *social;
   char_data *victim = nullptr;

   if( !( social = find_social( command ) ) )
      return false;

   if( ch->has_pcflag( PCFLAG_NO_EMOTE ) )
   {
      ch->print( "You are not allowed to use socials.\r\n" );
      return true;
   }

   switch ( ch->position )
   {
      default:
         break;

      case POS_DEAD:
         ch->print( "Lie still; you are DEAD.\r\n" );
         return true;

      case POS_INCAP:
      case POS_MORTAL:
         ch->print( "You are hurt far too bad for that.\r\n" );
         return true;

      case POS_STUNNED:
         ch->print( "You are too stunned to do that.\r\n" );
         return true;

      case POS_SLEEPING:
         /*
          * I just know this is the path to a 12" 'if' statement.  :(
          * But two players asked for it already!  -- Furey
          *
          * No 12 inch or 12 foot long if statement bro, add a position value to solve it! - Samson
          */
         if( social->minposition == POS_SLEEPING )
            break;
         ch->print( "In your dreams, or what?\r\n" );
         return true;
   }

   one_argument( argument, arg );
   if( arg.empty(  ) )
   {
      act( AT_SOCIAL, social->char_no_arg, ch, nullptr, victim, TO_CHAR );
      act( AT_SOCIAL, social->others_no_arg, ch, nullptr, victim, TO_ROOM );
      return true;
   }

   if( !( victim = ch->get_char_room( arg ) ) )
   {
      obj_data *obj; /* Object socials */

      if( ( ( obj = get_obj_list( ch, arg, ch->in_room->objects ) ) || ( obj = get_obj_list( ch, arg, ch->carrying ) ) ) && !victim )
      {
         if( !social->obj_self.empty(  ) && !social->obj_others.empty(  ) )
         {
            act( AT_SOCIAL, social->obj_self, ch, nullptr, obj, TO_CHAR );
            act( AT_SOCIAL, social->obj_others, ch, nullptr, obj, TO_ROOM );
         }
         return true;
      }

      if( !victim )
         ch->print( "They aren't here.\r\n" );

      return true;
   }

   if( victim == ch )
   {
      act( AT_SOCIAL, social->others_auto, ch, nullptr, victim, TO_ROOM );
      act( AT_SOCIAL, social->char_auto, ch, nullptr, victim, TO_CHAR );
      return true;
   }

   act( AT_SOCIAL, social->others_found, ch, nullptr, victim, TO_NOTVICT );
   act( AT_SOCIAL, social->char_found, ch, nullptr, victim, TO_CHAR );
   act( AT_SOCIAL, social->vict_found, ch, nullptr, victim, TO_VICT );

   if( !ch->isnpc(  ) && victim->isnpc(  ) && !victim->has_aflag( AFF_CHARM ) && victim->IS_AWAKE(  ) && !victim->desc && !HAS_PROG( victim->pIndexData, ACT_PROG ) )
   {
      switch ( number_bits( 4 ) )
      {
         default:
         case 0:
            if( ch->IS_EVIL(  ) || ch->IS_NEUTRAL(  ) )
            {
               act( AT_ACTION, "$n slaps $N.", victim, nullptr, ch, TO_NOTVICT );
               act( AT_ACTION, "You slap $N.", victim, nullptr, ch, TO_CHAR );
               act( AT_ACTION, "$n slaps you.", victim, nullptr, ch, TO_VICT );
            }
            else
            {
               act( AT_ACTION, "$n acts like $N doesn't even exist.", victim, nullptr, ch, TO_NOTVICT );
               act( AT_ACTION, "You just ignore $N.", victim, nullptr, ch, TO_CHAR );
               act( AT_ACTION, "$n appears to be ignoring you.", victim, nullptr, ch, TO_VICT );
            }
            break;

         case 1:
         case 2:
         case 3:
         case 4:
         case 5:
         case 6:
         case 7:
         case 8:
            act( AT_SOCIAL, social->others_found, victim, nullptr, ch, TO_NOTVICT );
            act( AT_SOCIAL, social->char_found, victim, nullptr, ch, TO_CHAR );
            act( AT_SOCIAL, social->vict_found, victim, nullptr, ch, TO_VICT );
            break;

         case 9:
         case 10:
         case 11:
         case 12:
            act( AT_ACTION, "$n slaps $N.", victim, nullptr, ch, TO_NOTVICT );
            act( AT_ACTION, "You slap $N.", victim, nullptr, ch, TO_CHAR );
            act( AT_ACTION, "$n slaps you.", victim, nullptr, ch, TO_VICT );
            break;
      }
   }
   return true;
}

/*
 * Character not in position for command?
 */
bool check_pos( char_data * ch, short position )
{
   if( ch->isnpc(  ) && ch->position > POS_SLEEPING )
      return true;

   if( ch->position < position )
   {
      switch ( ch->position )
      {
         default:
            ch->print( "Uh... yeah... we don't know, but it's not good....\r\n" );
            break;

         case POS_DEAD:
            ch->print( "A little difficult to do when you are DEAD...\r\n" );
            break;

         case POS_MORTAL:
         case POS_INCAP:
            ch->print( "You are hurt far too bad for that.\r\n" );
            break;

         case POS_STUNNED:
            ch->print( "You are too stunned to do that.\r\n" );
            break;

         case POS_SLEEPING:
            ch->print( "In your dreams, or what?\r\n" );
            break;

         case POS_RESTING:
            ch->print( "Nah... You feel too relaxed...\r\n" );
            break;

         case POS_SITTING:
            ch->print( "You can't do that sitting down.\r\n" );
            break;

         case POS_FIGHTING:
            if( position <= POS_EVASIVE )
               ch->print( "This fighting style is too demanding for that!\r\n" );
            else
               ch->print( "No way!  You are still fighting!\r\n" );
            break;

         case POS_DEFENSIVE:
            if( position <= POS_EVASIVE )
               ch->print( "This fighting style is too demanding for that!\r\n" );
            else
               ch->print( "No way!  You are still fighting!\r\n" );
            break;

         case POS_AGGRESSIVE:
            if( position <= POS_EVASIVE )
               ch->print( "This fighting style is too demanding for that!\r\n" );
            else
               ch->print( "No way!  You are still fighting!\r\n" );
            break;

         case POS_BERSERK:
            if( position <= POS_EVASIVE )
               ch->print( "This fighting style is too demanding for that!\r\n" );
            else
               ch->print( "No way!  You are still fighting!\r\n" );
            break;

         case POS_EVASIVE:
            ch->print( "No way!  You are still fighting!\r\n" );
            break;
      }
      return false;
   }
   return true;
}

bool check_alias( char_data * ch, const string & command, const string & argument )
{
   map < string, string >::iterator al;
   string arg;

   if( ch->isnpc(  ) )
      return false;

   al = ch->pcdata->alias_map.find( command );

   if( al == ch->pcdata->alias_map.end(  ) )
      return false;

   arg = ch->pcdata->alias_map[command];

   if( ch->pcdata->cmd_recurse == -1 || ++ch->pcdata->cmd_recurse > 50 )
   {
      if( ch->pcdata->cmd_recurse != -1 )
      {
         ch->print( "Unable to further process command, recurses too much.\r\n" );
         ch->pcdata->cmd_recurse = -1;
      }
      return false;
   }

   if( !argument.empty(  ) )
      arg.append( " " + argument );
   interpret( ch, arg );
   return true;
}

/*
 * The main entry point for executing commands.
 * Can be recursively called from 'at', 'order', 'force'.
 */
void interpret( char_data * ch, string argument )
{
   string command;
   string origarg = argument;
   char logline[MIL];
   char *buf;
   timer_data *chtimer = nullptr;
   cmd_type *cmd = nullptr;
   int trust, loglvl;
   bool found;
   struct timeval time_used;
   long tmptime;

   if( !ch )
   {
      bug( "%s: null ch!", __func__ );
      return;
   }

   if( !ch->in_room )
   {
      bug( "%s: %s null in_room!", __func__, ch->name );
      return;
   }

   found = false;
   if( ch->substate == SUB_REPEATCMD )
   {
      DO_FUN *fun;

      if( !( fun = ch->last_cmd ) )
      {
         ch->substate = SUB_NONE;
         bug( "%s: %s SUB_REPEATCMD with nullptr last_cmd", __func__, ch->name );
         return;
      }
      else
      {
         /*
          * yes... we lose out on the hashing speediness here...
          * but the only REPEATCMDS are wizcommands (currently)
          */
         for( char x = 0; x < 126; ++x )
         {
            const vector < cmd_type * >&cmd_list = command_table[x];
            vector < cmd_type * >::const_iterator icmd;

            for( icmd = cmd_list.begin(  ); icmd != cmd_list.end(  ); ++icmd )
            {
               cmd = *icmd;

               if( cmd->do_fun != fun )
                  continue;
               found = true;
               break;
            }
            if( found )
               break;
         }

         if( !found )
         {
            cmd = nullptr;
            bug( "%s: SUB_REPEATCMD: last_cmd invalid", __func__ );
            return;
         }
         snprintf( logline, MIL, "(%s) %s", cmd->name.c_str(  ), argument.c_str(  ) );
      }
   }

   if( !cmd )
   {
      /*
       * Changed the order of these ifchecks to prevent crashing. 
       */
      if( argument.empty(  ) || !str_cmp( argument, "" ) )
      {
         bug( "%s: null argument!", __func__ );
         return;
      }

      strip_lspace( argument );
      if( argument.empty(  ) )
         return;

      /*
       * Implement freeze command.
       */
      if( ch->has_pcflag( PCFLAG_FREEZE ) )
      {
         ch->print( "You're totally frozen!\r\n" );
         return;
      }

      /*
       * Spamguard autofreeze - Samson 3-18-01 
       */
      if( !ch->isnpc(  ) && ch->has_aflag( AFF_SPAMGUARD ) )
      {
         ch->print( "&RYou've been autofrozen by the spamguard!!\r\n" );
         ch->print( "&YTriggering the spamfreeze more than once will only worsen your situation.\r\n" );
         return;
      }

      if( ( mprog_keyword_trigger( argument, ch ) ) == true )
      {
         /*
          * Added because keywords are checked before commands 
          */
         if( ch->has_pcflag( PCFLAG_AFK ) )
         {
            ch->unset_pcflag( PCFLAG_AFK );
            ch->unset_pcflag( PCFLAG_IDLING );
            DISPOSE( ch->pcdata->afkbuf );
            act( AT_GREY, "$n is no longer afk.", ch, nullptr, nullptr, TO_CANSEE );
            ch->print( "You are no longer afk.\r\n" );
         }
         return;
      }

      /*
       * Grab the command word.
       * Special parsing so ' can be a command, also no spaces needed after punctuation.
       */
      strlcpy( logline, argument.c_str(  ), MIL );
      if( !isalpha( argument[0] ) && !isdigit( argument[0] ) )
      {
         command = argument[0];
         argument = argument.substr( 1, argument.length(  ) );
         strip_lspace( argument );
      }
      else
         argument = one_argument( argument, command );

      /*
       * Look for command in command table.
       * Check for bestowments
       */
      trust = ch->get_trust(  );

      const vector < cmd_type * >&cmd_list = command_table[LOWER( command[0] ) % 126];
      vector < cmd_type * >::const_iterator icmd;

      for( icmd = cmd_list.begin(  ); icmd != cmd_list.end(  ); ++icmd )
      {
         cmd = *icmd;

         if( !str_prefix( command, cmd->name )
             && ( cmd->level <= trust || ( !ch->isnpc(  ) && !ch->pcdata->bestowments.empty(  ) && hasname( ch->pcdata->bestowments, cmd->name )
                                           && cmd->level <= ( trust + sysdata->bestow_dif ) ) ) )
         {
            found = true;
            break;
         }
      }

      if( found && !cmd->do_fun )
      {
         char filename[256];
#if !defined(WIN32)
         const char *error;
#else
         DWORD error;
#endif

         snprintf( filename, 256, "../src/cmd/so/do_%s.so", cmd->name.c_str(  ) );
         cmd->fileHandle = dlopen( filename, RTLD_NOW );
         if( ( error = dlerror(  ) ) == nullptr )
         {
            cmd->do_fun = ( DO_FUN * ) ( dlsym( cmd->fileHandle, cmd->fun_name.c_str(  ) ) );
            cmd->flags.set( CMD_LOADED );
         }
         else
         {
            ch->print( "Huh?\r\n" );
            bug( "%s: %s", __func__, error );
            return;
         }
      }

      /*
       * Turn off afk bit when any command performed.
       */
      if( ch->has_pcflag( PCFLAG_AFK ) && ( str_cmp( command, "AFK" ) ) )
      {
         ch->unset_pcflag( PCFLAG_AFK );
         ch->unset_pcflag( PCFLAG_IDLING );
         DISPOSE( ch->pcdata->afkbuf );
         act( AT_GREY, "$n is no longer afk.", ch, nullptr, nullptr, TO_CANSEE );
         ch->print( "You are no longer afk.\r\n" );
      }

      /*
       * Check command for action flag, undo hide if it needs to be - Samson 7-7-00 
       */
      if( found && cmd->flags.test( CMD_ACTION ) )
      {
         ch->affect_strip( gsn_hide );
         ch->unset_aflag( AFF_HIDE );
      }
   }

   /*
    * Log and snoop.
    */
   snprintf( lastplayercmd, MIL * 2, "%s used command: %s", ch->name, logline );

   if( found && cmd->log == LOG_NEVER )
      strlcpy( logline, "XXXXXXXX XXXXXXXX XXXXXXXX", MIL );

   loglvl = found ? cmd->log : LOG_NORMAL;

   /*
    * Cannot perform commands that aren't ghost approved 
    */
   if( found && !cmd->flags.test( CMD_GHOST ) && ch->has_pcflag( PCFLAG_GHOST ) )
   {
      ch->print( "&YBut you're a GHOST! You can't do that until you've been resurrected!\r\n" );
      return;
   }

   if( found && !ch->isnpc(  ) && cmd->flags.test( CMD_MUDPROG ) )
   {
      ch->print( "Huh?\r\n" );
      return;
   }

   if( ch->has_pcflag( PCFLAG_LOG ) || fLogAll || loglvl == LOG_BUILD || loglvl == LOG_HIGH || loglvl == LOG_ALWAYS )
   {
      /*
       * Make it so a 'log all' will send most output to the log
       * file only, and not spam the log channel to death - Thoric
       */
      if( fLogAll && loglvl == LOG_NORMAL && ch->has_pcflag( PCFLAG_LOG ) )
         loglvl = LOG_ALL;

      /*
       * Added by Narn to show who is switched into a mob that executes
       * a logged command.  Check for descriptor in case force is used. 
       */
      if( !ch->isnpc(  ) )
      {
         if( ch->desc && ch->desc->original )
            log_printf_plus( loglvl, ch->level, "Log %s (%s): %s", ch->name, ch->desc->original->name, logline );
         else
            log_printf_plus( loglvl, ch->level, "Log %s: %s", ch->name, logline );
      }
   }

   if( ch->desc && ch->desc->snoop_by )
   {
      ch->desc->snoop_by->write_to_buffer( ch->name );
      ch->desc->snoop_by->write_to_buffer( "% " );
      ch->desc->snoop_by->write_to_buffer( logline );
      ch->desc->snoop_by->write_to_buffer( "\r\n" );
   }

   /*
    * check for a timer delayed command (search, dig, detrap, etc) 
    */
   if( ( ( chtimer = ch->get_timerptr( TIMER_DO_FUN ) ) != nullptr ) && ( !found || !cmd->flags.test( CMD_NOABORT ) ) )
   {
      int tempsub;

      tempsub = ch->substate;
      ch->substate = SUB_TIMER_DO_ABORT;
      ( chtimer->do_fun ) ( ch, "" );
      if( ch->char_died(  ) )
         return;
      if( ch->substate != SUB_TIMER_CANT_ABORT )
      {
         ch->substate = tempsub;
         ch->extract_timer( chtimer );
      }
      else
      {
         ch->substate = tempsub;
         return;
      }
   }

   /*
    * Look for command in skill and socials table.
    */
   if( !found )
   {
#ifdef MULTIPORT
      if( !shell_hook( ch, command, argument ) && !local_channel_hook( ch, command, argument )
#else
      if( !local_channel_hook( ch, command, argument )
#endif
          && !check_skill( ch, command, argument )
          && !check_ability( ch, command, argument )
          && !rprog_command_trigger( ch, origarg )
          && !mprog_command_trigger( ch, origarg ) && !oprog_command_trigger( ch, origarg ) && !check_alias( ch, command, argument ) && !check_social( ch, command, argument )
          )
      {
         exit_data *pexit;

         /*
          * check for an auto-matic exit command 
          */
         if( ( pexit = find_door( ch, command, true ) ) != nullptr && IS_EXIT_FLAG( pexit, EX_xAUTO ) )
         {
            if( IS_EXIT_FLAG( pexit, EX_CLOSED ) && ( !ch->has_aflag( AFF_PASS_DOOR ) || IS_EXIT_FLAG( pexit, EX_NOPASSDOOR ) ) )
            {
               if( !IS_EXIT_FLAG( pexit, EX_SECRET ) )
                  act( AT_PLAIN, "The $d is closed.", ch, nullptr, pexit->keyword, TO_CHAR );
               else
                  ch->print( "You cannot do that here.\r\n" );
               return;
            }
            if( ( IS_EXIT_FLAG( pexit, EX_FORTIFIED ) || IS_EXIT_FLAG( pexit, EX_HEAVY )
                  || IS_EXIT_FLAG( pexit, EX_MEDIUM ) || IS_EXIT_FLAG( pexit, EX_LIGHT ) || IS_EXIT_FLAG( pexit, EX_CRUMBLING ) ) )
            {
               ch->print( "You cannot do that here.\r\n" );
               return;
            }
            move_char( ch, pexit, 0, pexit->vdir, false );
            return;
         }
         ch->print( "Huh?\r\n" );
      }
      return;
   }

   /*
    * Character not in position for command?
    */
   if( !check_pos( ch, cmd->position ) )
      return;

   /*
    * So we can check commands for things like Posses and Polymorph
    * *  But still keep the online editing ability.  -- Shaddai
    * *  Send back the message to print out, so we have the option
    * *  this function might be usefull elsewhere.  Also using the
    * *  send_to_char_color so we can colorize the strings if need be. --Shaddai
    */
   buf = check_cmd_flags( ch, cmd );

   if( buf[0] != '\0' )
   {
      ch->print( buf );
      return;
   }

   /*
    * Dispatch the command.
    */
   ch->prev_cmd = ch->last_cmd;  /* haus, for automapping */
   ch->last_cmd = cmd->do_fun;   /* Usurped in hopes of using it for the spamguard instead */
   if( !str_cmp( cmd->name, "slay" ) )
   {
      string tmpargument = argument;
      string tmparg;

      tmpargument = one_argument( tmpargument, tmparg );

      if( !str_cmp( tmparg, "Samson" ) )
      {
         ch->print( "The forces of the universe prevent you from taking this action....\r\n" );
         return;
      }
   }

   try
   {
      start_timer( &time_used );
      ( *cmd->do_fun ) ( ch, argument );
      end_timer( &time_used );
   }
   catch( exception & e )
   {
      bug( "&YCommand exception: '%s' on command: %s %s&D", e.what(  ), cmd->name.c_str(  ), argument.c_str(  ) );
   }
   catch( ... )
   {
      bug( "&YUnknown command exception on command: %s %s&D", cmd->name.c_str(  ), argument.c_str(  ) );
   }

   tmptime = UMIN( time_used.tv_sec, 19 ) * 1000000 + time_used.tv_usec;

   /*
    * laggy command notice: command took longer than 3.0 seconds 
    */
   if( tmptime > 3000000 )
   {
      log_printf_plus( LOG_NORMAL, ch->level, "[*****] LAG: %s: %s %s (R:%d S:%ld.%06ld)", ch->name,
                       cmd->name.c_str(  ), ( cmd->log == LOG_NEVER ? "XXX" : argument.c_str(  ) ), ch->in_room ? ch->in_room->vnum : 0, time_used.tv_sec, time_used.tv_usec );
   }
   strlcpy( lastplayercmd, "No commands pending", MIL * 2 );
}

void cmdf( char_data * ch, const char *fmt, ... )
{
   char buf[MSL * 2];
   va_list args;

   va_start( args, fmt );
   vsnprintf( buf, MSL * 2, fmt, args );
   va_end( args );

   interpret( ch, buf );
}

/* Be damn sure the function you pass here is valid, or Bad Things(tm) will happen. */
void funcf( char_data * ch, DO_FUN * cmd, const char *fmt, ... )
{
   char buf[MSL * 2];
   va_list args;

   if( !cmd )
   {
      bug( "%s: Bad function passed to funcf!", __func__ );
      return;
   }

   va_start( args, fmt );
   vsnprintf( buf, MSL * 2, fmt, args );
   va_end( args );

   ( cmd ) ( ch, buf );
}

cmd_type::cmd_type(  )
{
   init_memory( &fileHandle, &log, sizeof( log ) );
}

/*
 * Free a command structure - Thoric
 */
cmd_type::~cmd_type(  )
{
   do_fun = nullptr;
}

void free_commands( void )
{
   for( char x = 0; x < 126; ++x )
   {
      vector < cmd_type * >&cmd_list = command_table[x];
      vector < cmd_type * >::iterator icmd;

      for( icmd = cmd_list.begin(  ); icmd != cmd_list.end(  ); ++icmd )
      {
         cmd_type *cmd = *icmd;

         deleteptr( cmd );
      }
      cmd_list.clear(  );
   }
   command_table.clear(  );
}

/*
 * Remove a command from it's hash index - Thoric
 */
void unlink_command( cmd_type * command )
{
   if( !command )
   {
      bug( "%s: nullptr command", __func__ );
      return;
   }

   vector < cmd_type * >&cmd_list = command_table[command->name[0] % 126];
   vector < cmd_type * >::iterator icmd;

   for( icmd = cmd_list.begin(  ); icmd != cmd_list.end(  ); ++icmd )
   {
      cmd_type *cmd = *icmd;

      if( !str_cmp( cmd->name, command->name ) )
      {
         cmd_list.erase( icmd );
         break;
      }
   }
}

/*
 * Add a command to the command hash table - Thoric
 */
void add_command( cmd_type * command )
{
   if( !command )
   {
      bug( "%s: nullptr command", __func__ );
      return;
   }

   if( command->name.empty(  ) )
   {
      bug( "%s: Empty command->name", __func__ );
      return;
   }

   // Force all commands to be lowercase names
   strlower( command->name );

   vector < cmd_type * >&cmd_list = command_table[command->name[0] % 126];
   cmd_list.push_back( command );
}

cmd_type *find_command( const string & command )
{
   vector < cmd_type * >::const_iterator icmd;
   const vector < cmd_type * >&cmd_list = command_table[command[0] % 126];

   for( icmd = cmd_list.begin(  ); icmd != cmd_list.end(  ); ++icmd )
   {
      cmd_type *cmd = *icmd;

      if( !str_prefix( command, cmd->name ) )
         return cmd;
   }
   return nullptr;
}

/*
 * Save the commands to disk
 * Added flags Aug 25, 1997 --Shaddai
 */
const int CMDVERSION = 3;
/* Updated to 1 for command position - Samson 4-26-00 */
/* Updated to 2 for log flag - Samson 4-26-00 */
/* Updated to 3 for command flags - Samson 7-9-00 */
void save_commands( void )
{
   ofstream stream;

   stream.open( COMMAND_FILE );
   if( !stream.is_open(  ) )
   {
      bug( "%s: Cannot open commands.dat for writing", __func__ );
      perror( COMMAND_FILE );
      return;
   }

   stream << "#VERSION " << CMDVERSION << endl;

   for( char x = 0; x < 126; ++x )
   {
      const vector < cmd_type * >&cmd_list = command_table[x];
      vector < cmd_type * >::const_iterator icmd;

      for( icmd = cmd_list.begin(  ); icmd != cmd_list.end(  ); ++icmd )
      {
         cmd_type *command = *icmd;

         if( command->name.empty(  ) )
         {
            bug( "%s: blank command in command table", __func__ );
            continue;
         }
         stream << "#COMMAND" << endl;
         stream << "Name        " << command->name << endl;
         /*
          * Modded to use new field - Trax 
          */
         if( !command->fun_name.empty(  ) )
            stream << "Code        " << command->fun_name << endl;
         stream << "Position    " << npc_position[command->position] << endl;
         stream << "Level       " << command->level << endl;
         stream << "Log         " << log_flag[command->log] << endl;
         if( command->flags.any(  ) )
            stream << "Flags       " << bitset_string( command->flags, cmd_flags ) << endl;
         stream << "End" << endl << endl;
      }
   }
   stream.close(  );
}

void load_commands( void )
{
   ifstream stream;
   cmd_type *cmd = nullptr;
   int version = 0;

   command_table.clear(  );
   command_table.resize( 126 );

   stream.open( COMMAND_FILE );
   if( !stream.is_open(  ) )
   {
      bug( "%s: No command file found.", __func__ );
      exit( 1 );
   }

   do
   {
      string key, value;
      char buf[MIL];

      stream >> key;
      stream.getline( buf, MIL );
      value = buf;

      strip_lspace( key );
      strip_lspace( value );
      strip_tilde( value );

      if( key.empty(  ) )
         continue;

      if( key == "#VERSION" )
         version = atoi( value.c_str(  ) );
      else if( key == "#COMMAND" )
         cmd = new cmd_type;
      else if( key == "Name" )
         cmd->name = value;
      else if( key == "Code" )
      {
         cmd->fun_name = value;
         cmd->do_fun = skill_function( value );
         if( cmd->do_fun == skill_notfound )
            cmd->do_fun = nullptr;
      }
      else if( key == "Position" )
      {
         int pos = get_npc_position( value );
         if( pos < 0 || pos >= POS_MAX )
         {
            bug( "%s: Command %s has invalid position! Defaulting to standing.", __func__, cmd->name.c_str(  ) );
            pos = POS_STANDING;
         }
         cmd->position = pos;
      }
      else if( key == "Level" )
         cmd->level = URANGE( 0, atoi( value.c_str(  ) ), MAX_LEVEL );
      else if( key == "Log" )
      {
         if( version < 2 )
            cmd->log = atoi( value.c_str(  ) );
         else
         {
            int lognum = get_logflag( value );

            if( lognum < 0 || lognum > LOG_ALL )
            {
               bug( "%s: Command %s has invalid log flag! Defaulting to normal.", __func__, cmd->name.c_str(  ) );
               lognum = LOG_NORMAL;
            }
            cmd->log = lognum;
         }
      }
      else if( key == "Flags" )
         flag_string_set( value, cmd->flags, cmd_flags );
      else if( key == "End" )
         add_command( cmd );
      else
         log_printf( "%s: Bad line in command file: %s %s", __func__, key.c_str(  ), value.c_str(  ) );
   }
   while( !stream.eof(  ) );
   stream.close(  );
}

/*
 * Command editor/displayer/save/delete - Thoric
 * Added support for interpret flags    - Shaddai
 */
CMDF( do_cedit )
{
   cmd_type *command;
   string arg1, arg2;

   ch->set_color( AT_IMMORT );

   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );
   if( arg1.empty(  ) )
   {
      ch->print( "Syntax: cedit save cmdtable\r\n" );
      if( ch->get_trust(  ) > LEVEL_SUB_IMPLEM )
      {
         ch->print( "Syntax: cedit load <command>\r\n" );
         ch->print( "Syntax: cedit <command> create [code]\r\n" );
         ch->print( "Syntax: cedit <command> delete\r\n" );
         ch->print( "Syntax: cedit <command> show\r\n" );
         ch->print( "Syntax: cedit <command> [field]\r\n" );
         ch->print( "\r\nField being one of:\r\n" );
         ch->print( "  level position log code flags\r\n" );
      }
      return;
   }

   if( ch->level > LEVEL_GREATER && !str_cmp( arg1, "save" ) && !str_cmp( arg2, "cmdtable" ) )
   {
      save_commands(  );
      ch->print( "Saved.\r\n" );
      return;
   }

   command = find_command( arg1 );
   if( ch->get_trust(  ) > LEVEL_SUB_IMPLEM && !str_cmp( arg2, "create" ) )
   {
      if( command )
      {
         ch->print( "That command already exists!\r\n" );
         return;
      }
      command = new cmd_type;
      command->name = arg1;
      command->level = ch->get_trust(  );
      if( !argument.empty(  ) )
         one_argument( argument, arg2 );
      else
         arg2 = "do_" + arg1;
      command->do_fun = skill_function( arg2 );
      command->fun_name = arg2;
      add_command( command );
      ch->print( "Command added.\r\n" );
      if( command->do_fun == skill_notfound )
         ch->printf( "Code %s not found. Set to no code.\r\n", arg2.c_str(  ) );
      return;
   }

   if( !command )
   {
      ch->print( "Command not found.\r\n" );
      return;
   }
   else if( command->level > ch->get_trust(  ) )
   {
      ch->print( "You cannot touch this command.\r\n" );
      return;
   }

   if( arg2.empty(  ) || !str_cmp( arg2, "show" ) )
   {
      ch->printf( "Command:   %s\r\nLevel:     %d\r\nPosition:  %s\r\nLog:       %s\r\nFunc Name: %s\r\nFlags:     %s\r\n",
                  command->name.c_str(  ), command->level, npc_position[command->position], log_flag[command->log],
                  command->fun_name.c_str(  ), bitset_string( command->flags, cmd_flags ) );
      return;
   }

   if( ch->get_trust(  ) <= LEVEL_SUB_IMPLEM )
   {
      do_cedit( ch, "" );
      return;
   }

   if( !str_cmp( arg2, "load" ) )
   {
      char filename[256];
#if !defined(WIN32)
      const char *error;
#else
      DWORD error;
#endif

      if( command->flags.test( CMD_LOADED ) )
      {
         ch->printf( "The %s command function is already loaded.\r\n", command->name.c_str(  ) );
         return;
      }

      if( command->fun_name.empty(  ) )
      {
         ch->printf( "The %s command has a nullptr function name!\r\n", command->name.c_str() );
         return;
      }

      snprintf( filename, 256, "../src/cmd/so/do_%s.so", command->name.c_str(  ) );
      command->fileHandle = dlopen( filename, RTLD_NOW );
      if( ( error = dlerror(  ) ) == nullptr )
      {
         command->do_fun = ( DO_FUN * ) ( dlsym( command->fileHandle, command->fun_name.c_str(  ) ) );
         command->flags.set( CMD_LOADED );
         ch->printf( "Command %s loaded and available.\r\n", command->name.c_str(  ) );
         return;
      }

      ch->printf( "Error: %s\r\n", error );
      return;
   }

   if( !str_cmp( arg2, "unload" ) )
   {
      if( !command->flags.test( CMD_LOADED ) )
      {
         ch->printf( "The %s command function is not loaded.\r\n", command->name.c_str(  ) );
         return;
      }

      dlclose( command->fileHandle );
      command->flags.reset( CMD_LOADED );
      command->do_fun = nullptr;
      ch->printf( "The %s command has been unloaded.\r\n", command->name.c_str(  ) );
      return;
   }

   if( !str_cmp( arg2, "reload" ) )
   {
      if( !command->flags.test( CMD_LOADED ) )
      {
         ch->printf( "The %s command function is not loaded.\r\n", command->name.c_str(  ) );
         return;
      }

      dlclose( command->fileHandle );
      command->flags.reset( CMD_LOADED );
      command->do_fun = nullptr;
      ch->printf( "The %s command has been unloaded.\r\n", command->name.c_str(  ) );

      funcf( ch, do_cedit, "%s load", command->name.c_str(  ) );
      return;
   }

   if( !str_cmp( arg2, "delete" ) )
   {
      unlink_command( command );
      deleteptr( command );
      ch->print( "Command deleted.\r\n" );
      return;
   }

   if( !str_cmp( arg2, "code" ) )
   {
      DO_FUN *fun = skill_function( argument );

      if( fun == skill_notfound )
      {
         ch->print( "Code not found.\r\n" );
         return;
      }
      command->do_fun = fun;
      command->fun_name = argument;
      ch->print( "Command code updated.\r\n" );
      return;
   }

   if( !str_cmp( arg2, "level" ) )
   {
      int level = atoi( argument.c_str(  ) );

      if( level < 0 || level > ch->get_trust(  ) )
      {
         ch->print( "Level out of range.\r\n" );
         return;
      }
      if( level > command->level && command->do_fun == do_switch )
      {
         command->level = level;
         check_switches(  );
      }
      else
         command->level = level;
      ch->print( "Command level updated.\r\n" );
      return;
   }

   if( !str_cmp( arg2, "log" ) )
   {
      int clog = get_logflag( argument );

      if( clog < 0 || clog > LOG_ALL )
      {
         ch->print( "Log out of range.\r\n" );
         return;
      }
      command->log = clog;
      ch->print( "Command log setting updated.\r\n" );
      return;
   }

   if( !str_cmp( arg2, "position" ) )
   {
      int pos;

      pos = get_npc_position( argument );

      if( pos < 0 || pos >= POS_MAX )
      {
         ch->print( "Invalid position.\r\n" );
         return;
      }
      command->position = pos;
      ch->print( "Command position updated.\r\n" );
      return;
   }

   if( !str_cmp( arg2, "flags" ) )
   {
      int flag;

      flag = get_cmdflag( argument );
      if( flag < 0 || flag >= MAX_CMD_FLAG )
      {
         ch->printf( "Unknown flag %s.\r\n", argument.c_str(  ) );
         return;
      }
      command->flags.flip( flag );
      ch->print( "Command flags updated.\r\n" );
      return;
   }

   if( !str_cmp( arg2, "name" ) )
   {
      cmd_type *checkcmd;

      one_argument( argument, arg1 );
      if( arg1.empty(  ) )
      {
         ch->print( "Cannot clear name field!\r\n" );
         return;
      }
      if( ( checkcmd = find_command( arg1 ) ) != nullptr )
      {
         ch->printf( "There is already a command named %s.\r\n", arg1.c_str(  ) );
         return;
      }
      unlink_command( command );
      command->name = arg1;
      add_command( command );
      ch->print( "Done.\r\n" );
      return;
   }

// So you should be able to call... swap(it, it+1) or swap(it, it-1)
// Raise = it-1
// Lower = it+1
// Where it = iterator of command being moved. Safety check it != cmd_list.begin()
   /*
    * display usage message 
    */
   do_cedit( ch, "" );
}

CMDF( do_restrict )
{
   string arg;
   short level;
   cmd_type *cmd;

   ch->set_color( AT_IMMORT );

   argument = one_argument( argument, arg );
   if( arg.empty(  ) )
   {
      ch->print( "Restrict which command?\r\n" );
      return;
   }

   if( argument.empty(  ) )
      level = ch->get_trust(  );
   else
   {
      if( !is_number( argument ) )
      {
         ch->print( "Level must be numeric.\r\n" );
         return;
      }
      level = atoi( argument.c_str(  ) );
   }
   level = UMAX( UMIN( ch->get_trust(  ), level ), 0 );

   cmd = find_command( arg );
   if( !cmd )
   {
      ch->print( "No command by that name.\r\n" );
      return;
   }

   if( cmd->level > ch->get_trust(  ) )
   {
      ch->print( "You may not restrict that command.\r\n" );
      return;
   }

   if( !str_prefix( argument, "show" ) )
   {
      cmdf( ch, "%s show", cmd->name.c_str(  ) );
      return;
   }

   cmd->level = level;
   ch->printf( "You restrict %s to level %d\r\n", cmd->name.c_str(  ), level );
   log_printf( "%s restricting %s to level %d", ch->name, cmd->name.c_str(  ), level );
}

string extract_command_names( char_data * ch )
{
   string tbuf, tcomm, comm_names;

   if( !ch || ch->isnpc(  ) )
      return "";

   if( ch->pcdata->bestowments.empty(  ) )
      return "";

   tbuf = ch->pcdata->bestowments;
   tbuf = one_argument( tbuf, tcomm );

   if( tcomm.empty(  ) )
      return "";

   while( !tbuf.empty(  ) )
   {
      if( !strstr( tcomm.c_str(  ), ".are" ) )
      {
         if( comm_names.empty(  ) )
            comm_names = tcomm;
         else
            comm_names.append( " " + tcomm );
      }
      tbuf = one_argument( tbuf, tcomm );
   }
   return comm_names;
}

CMDF( do_bestow )
{
   string arg, buf;
   char_data *victim;
   cmd_type *cmd;

   ch->set_color( AT_IMMORT );

   if( argument.empty(  ) )
   {
      ch->print( "Syntax:\r\n"
                 "bestow <victim> <command>           adds a command to the list\r\n"
                 "bestow <victim> none                removes bestowed areas\r\n"
                 "bestow <victim> remove <command>    removes a specific area\r\n"
                 "bestow <victim> list                lists bestowed areas\r\n" "bestow <victim>                     lists bestowed areas\r\n" );
      return;
   }

   argument = one_argument( argument, arg );

   if( !( victim = get_wizvictim( ch, arg, true ) ) )
      return;

   if( argument.empty(  ) || !str_cmp( argument, "list" ) )
   {
      if( victim->pcdata->bestowments.empty(  ) )
      {
         ch->printf( "%s has no bestowed commands.\r\n", victim->name );
         return;
      }

      buf = extract_command_names( victim );
      if( buf.empty(  ) )
      {
         ch->printf( "%s has no bestowed commands.\r\n", victim->name );
         return;
      }
      ch->printf( "Current bestowed commands on %s: %s.\r\n", victim->name, buf.c_str(  ) );
      return;
   }

   argument = one_argument( argument, arg );

   if( !argument.empty(  ) && !str_cmp( arg, "remove" ) )
   {
      while( !argument.empty(  ) )
      {
         argument = one_argument( argument, arg );
         if( !hasname( victim->pcdata->bestowments, arg ) )
         {
            ch->printf( "%s does not have a command named %s bestowed.\r\n", victim->name, arg.c_str(  ) );
            return;
         }
         removename( victim->pcdata->bestowments, arg );
         ch->printf( "Removed command %s from %s.\r\n", arg.c_str(  ), victim->name );
      }
      victim->save(  );
      return;
   }

   if( !str_cmp( arg, "none" ) )
   {
      buf = extract_command_names( victim );
      if( victim->pcdata->bestowments.empty(  ) )
      {
         ch->printf( "%s has no commands bestowed!\r\n", victim->name );
         return;
      }

      buf = extract_area_names( victim );
      victim->pcdata->bestowments = buf;

      ch->printf( "Command bestowments removed from %s.\r\n", victim->name );
      victim->printf( "%s has removed your bestowed commands.\r\n", ch->name );
      check_switch( victim );
      victim->save(  );
      return;
   }

   while( !argument.empty(  ) )
   {
      if( strstr( arg.c_str(  ), ".are" ) )
      {
         ch->printf( "'%s' is not a valid command to bestow.\r\n", arg.c_str(  ) );
         ch->print( "You cannot bestow an area with 'bestow'. Use 'bestowarea'.\r\n" );
         return;
      }

      if( hasname( victim->pcdata->bestowments, arg ) )
      {
         ch->printf( "%s already has '%s' bestowed.\r\n", victim->name, arg.c_str(  ) );
         return;
      }

      if( !( cmd = find_command( arg ) ) )
      {
         ch->printf( "'%s' is not a valid command.\r\n", arg.c_str(  ) );
         return;
      }

      if( cmd->level > ch->get_trust(  ) )
      {
         ch->printf( "The command '%s' is beyond you, thus you cannot bestow it.\r\n", arg.c_str(  ) );
         return;
      }

      smash_tilde( arg );
      addname( victim->pcdata->bestowments, arg );
      victim->printf( "%s has bestowed on you the command: %s\r\n", ch->name, arg.c_str(  ) );
      ch->printf( "%s has been bestowed: %s\r\n", victim->name, arg.c_str(  ) );

      argument = one_argument( argument, arg );
   }
   victim->save(  );
}

/*
 * Thanks to Grodyn for pointing out bugs in this function.
 */
/* Samson 4-7-98, added protection against forced delete */
CMDF( do_force )
{
   cmd_type *cmd = nullptr;
   string arg, arg2, command;

   ch->set_color( AT_IMMORT );

   argument = one_argument( argument, arg );
   argument = one_argument( argument, arg2 );
   if( arg.empty(  ) || arg2.empty(  ) )
   {
      ch->print( "Force whom to do what?\r\n" );
      return;
   }

   bool mobsonly = ch->level < sysdata->level_forcepc;
   cmd = find_command( arg2 );
   command = arg2 + " " + argument;

   if( !str_cmp( arg, "all" ) )
   {
      if( mobsonly )
      {
         ch->print( "You are not of sufficient level to force players yet.\r\n" );
         return;
      }

      if( cmd && cmd->flags.test( CMD_NOFORCE ) )
      {
         ch->printf( "You cannot force anyone to %s\r\n", cmd->name.c_str(  ) );
         log_printf( "%s attempted to force all to %s - command is flagged noforce", ch->name, cmd->name.c_str(  ) );
         return;
      }

      list < char_data * >::iterator ich;
      for( ich = pclist.begin(  ); ich != pclist.end(  ); )
      {
         char_data *vch = *ich;
         ++ich;

         if( vch->get_trust(  ) < ch->get_trust(  ) )
         {
            act( AT_IMMORT, "$n forces you to '$t'.", ch, command.c_str(  ), vch, TO_VICT );
            interpret( vch, command );
         }
      }
   }
   else
   {
      char_data *victim;

      if( !( victim = ch->get_char_world( arg ) ) )
      {
         ch->print( "They aren't here.\r\n" );
         return;
      }

      if( victim == ch )
      {
         ch->print( "Aye aye, right away!\r\n" );
         return;
      }

      if( ( victim->get_trust(  ) >= ch->get_trust(  ) ) || ( mobsonly && !victim->isnpc(  ) ) )
      {
         ch->print( "Do it yourself!\r\n" );
         return;
      }

      if( cmd && cmd->flags.test( CMD_NOFORCE ) )
      {
         ch->printf( "You cannot force anyone to %s\r\n", cmd->name.c_str(  ) );
         log_printf( "%s attempted to force %s to %s - command is flagged noforce", ch->name, victim->name, cmd->name.c_str(  ) );
         return;
      }

      if( ch->level < LEVEL_GOD && victim->isnpc(  ) && !str_prefix( "mp", argument ) )
      {
         ch->print( "You can't force a mob to do that!\r\n" );
         return;
      }
      act( AT_IMMORT, "$n forces you to '$t'.", ch, command.c_str(  ), victim, TO_VICT );
      interpret( victim, command );
   }
   ch->print( "Ok.\r\n" );
}

/* lets the mobile force someone to do something.  must be mortal level
   and the all argument only affects those in the room with the mobile */
CMDF( do_mpforce )
{
   cmd_type *cmd = nullptr;
   string arg, arg2;

   if( !can_use_mprog( ch ) )
      return;

   argument = one_argument( argument, arg );
   argument = one_argument( argument, arg2 );

   if( arg2.empty(  ) )
   {
      progbugf( ch, "%s", "Mpforce - Bad syntax: Missing command" );
      return;
   }

   cmd = find_command( arg2 );

   if( !str_cmp( arg, "all" ) )
   {
      list < char_data * >::iterator ich;

      for( ich = ch->in_room->people.begin(  ); ich != ch->in_room->people.end(  ); )
      {
         char_data *vch = *ich;
         ++ich;

         if( vch->get_trust(  ) < ch->get_trust(  ) && ch->can_see( vch, false ) )
         {
            if( cmd && cmd->flags.test( CMD_NOFORCE ) )
            {
               progbugf( ch, "Mpforce: Attempted to force all to %s - command is flagged noforce", arg2.c_str(  ) );
               return;
            }
            interpret( vch, argument );
         }
      }
   }
   else
   {
      char_data *victim;

      if( !( victim = ch->get_char_room( arg ) ) )
      {
         progbugf( ch, "Mpforce - No such victim %s", arg.c_str(  ) );
         return;
      }

      if( victim == ch )
      {
         progbugf( ch, "%s", "Mpforce - Forcing oneself" );
         return;
      }

      if( !victim->isnpc(  ) && ( !victim->desc ) && victim->is_immortal(  ) )
      {
         progbugf( ch, "Mpforce - Attempting to force link dead immortal %s", victim->name );
         return;
      }

      /*
       * Commands with a CMD_NOFORCE flag will not be allowed to be mpforced Samson 3-3-04 
       */
      if( cmd && cmd->flags.test( CMD_NOFORCE ) )
      {
         progbugf( ch, "Mpforce: Attempted to force %s to %s - command is flagged noforce", victim->name, cmd->name.c_str(  ) );
         return;
      }

      string lbuf = arg2 + " " + argument;
      interpret( victim, lbuf );
   }
}

/* Nearly rewritten by Xorith to be more smooth and nice and all that jazz. *grin* */
/* Modified again. I added booleans for sorted and all option checking. It occured to me
   that calling str_cmp three times within the loop is probably costing us a bit in performance. 
   Also added column headers to make the information more obvious, and changed it to print - instead of a 0
   for no level requirement on commands, just for visual appeal. -- Xorith */
CMDF( do_commands )
{
   int col = 0, count = 0, chTrust = 0;
   char letter = ' ';
   cmd_type *command;
   bool sorted = false, all = false;

   // Set up common variables used in the loop - better performance than calling the functions.
   if( !argument.empty(  ) && !str_cmp( argument, "sorted" ) )
      sorted = true;
   if( !argument.empty(  ) && !str_cmp( argument, "all" ) )
      all = true;
   chTrust = ch->get_trust(  );

   if( !argument.empty(  ) && !( sorted || all ) )
      ch->pagerf( "&[plain]Commands beginning with '%s':\r\n", argument.c_str(  ) );
   else if( !argument.empty(  ) && sorted )
      ch->pager( "&[plain]Commands -- Tabbed by Letter\r\n" );
   else
      ch->pager( "&[plain]Commands -- All\r\n" );

   // Doesn't fit well in a sorted view... -- X
   if( !sorted )
   {
      ch->pagerf( "&[plain]%-11s Lvl &[plain]%-11s Lvl &[plain]%-11s Lvl &[plain]%-11s Lvl &[plain]%-11s Lvl\r\n", "Command", "Command", "Command", "Command", "Command" );
      ch->pager( "-------------------------------------------------------------------------------\r\n" );
   }

   for( char x = 0; x < 126; ++x )
   {
      const vector < cmd_type * >&cmd_list = command_table[x];
      vector < cmd_type * >::const_iterator icmd;

      for( icmd = cmd_list.begin(  ); icmd != cmd_list.end(  ); ++icmd )
      {
         command = *icmd;

         if( command->level > LEVEL_AVATAR || command->level > chTrust || command->flags.test( CMD_MUDPROG ) )
            continue;

         if( ( !argument.empty(  ) && !( sorted || all ) ) && str_prefix( argument, command->name ) )
            continue;

         if( !argument.empty(  ) && sorted && letter != command->name[0] )
         {
            if( command->name[0] < 97 || command->name[0] > 122 )
            {
               if( !count )
                  ch->pager( "&c[ &[plain]Misc&c ]\r\n" );
            }
            else
            {
               letter = command->name[0];
               if( col % 5 != 0 )
                  ch->pager( "\r\n" );
               ch->pagerf( "&c[ &[plain]%c &c]\r\n", toupper( letter ) );
               col = 0;
            }
         }

         if( command->level == 0 )
            ch->pagerf( "&[plain]%-11s   - ", command->name.c_str(  ) );
         else
            ch->pagerf( "&[plain]%-11s %3d ", command->name.c_str(  ), command->level );

         ++count;
         if( ++col % 5 == 0 )
            ch->pager( "\r\n" );
      }
   }

   if( col % 5 != 0 )
      ch->pager( "\r\n" );

   if( count )
      ch->pagerf( "&[plain]  %d commands found.\r\n", count );
   else
      ch->pager( "&[plain]  No commands found.\r\n" );
}

/* Updated to Sadiq's wizhelp snippet - Thanks Sadiq! */
/* Accepts level argument to display an individual level - Added by Samson 2-20-00 */
CMDF( do_wizhelp )
{
   cmd_type *cmd;
   int col = 0, curr_lvl;

   ch->set_pager_color( AT_PLAIN );

   if( argument.empty(  ) )
   {
      for( curr_lvl = LEVEL_IMMORTAL; curr_lvl <= ch->level; ++curr_lvl )
      {
         ch->pager( "\r\n\r\n" );
         col = 1;
         ch->pagerf( "[LEVEL %-2d]\r\n", curr_lvl );

         for( char x = 0; x < 126; ++x )
         {
            const vector < cmd_type * >&cmd_list = command_table[x];
            vector < cmd_type * >::const_iterator icmd;

            for( icmd = cmd_list.begin(  ); icmd != cmd_list.end(  ); ++icmd )
            {
               cmd = *icmd;

               if( ( cmd->level == curr_lvl ) && cmd->level <= ch->level )
               {
                  ch->pagerf( "%-12s", cmd->name.c_str(  ) );
                  if( ++col % 6 == 0 )
                     ch->pager( "\r\n" );
               }
            }
         }
#ifdef MULTIPORT
         shellcommands( ch, curr_lvl );
#endif
      }
      if( col % 6 != 0 )
         ch->pager( "\r\n" );
      return;
   }

   if( !is_number( argument ) )
   {
      ch->print( "Syntax: wizhelp [level]\r\n" );
      return;
   }

   curr_lvl = atoi( argument.c_str(  ) );

   if( curr_lvl < LEVEL_IMMORTAL || curr_lvl > MAX_LEVEL )
   {
      ch->printf( "Valid levels are between %d and %d.\r\n", LEVEL_IMMORTAL, MAX_LEVEL );
      return;
   }

   ch->pager( "\r\n\r\n" );
   col = 1;
   ch->pagerf( "[LEVEL %-2d]\r\n", curr_lvl );

   for( char x = 0; x < 126; ++x )
   {
      const vector < cmd_type * >&cmd_list = command_table[x];
      vector < cmd_type * >::const_iterator icmd;

      for( icmd = cmd_list.begin(  ); icmd != cmd_list.end(  ); ++icmd )
      {
         cmd = *icmd;

         if( cmd->level == curr_lvl )
         {
            ch->pagerf( "%-12s", cmd->name.c_str(  ) );
            if( ++col % 6 == 0 )
               ch->pager( "\r\n" );
         }
      }
   }
   if( col % 6 != 0 )
      ch->pager( "\r\n" );
#ifdef MULTIPORT
   shellcommands( ch, curr_lvl );
#endif
}

CMDF( do_timecmd )
{
   struct timeval starttime;
   struct timeval endtime;
   static bool timing;
   string arg;

   ch->print( "Timing\r\n" );
   if( timing )
      return;

   one_argument( argument, arg );
   if( arg.empty(  ) )
   {
      ch->print( "No command to time.\r\n" );
      return;
   }
   if( !str_cmp( arg, "update" ) )
   {
      if( timechar )
         ch->print( "Another person is already timing updates.\r\n" );
      else
      {
         timechar = ch;
         ch->print( "Setting up to record next update loop.\r\n" );
      }
      return;
   }
   ch->print( "&[plain]Starting timer.\r\n" );
   timing = true;
   gettimeofday( &starttime, nullptr );
   interpret( ch, argument );
   gettimeofday( &endtime, nullptr );
   timing = false;
   ch->print( "&[plain]Timing complete.\r\n" );
   subtract_times( &endtime, &starttime );
   ch->printf( "Timing took %ld.%06ld seconds.\r\n", endtime.tv_sec, endtime.tv_usec );
}

/******************************************************
      Desolation of the Dragon MUD II - Alias Code
      (C) 1997, 1998  Jesse DeFer and Heath Leach
 http://dotd.mudservices.com  dotd@dotd.mudservices.com 
 ******************************************************/

CMDF( do_alias )
{
   map < string, string >::iterator al;
   string arg;

   if( ch->isnpc(  ) )
      return;

   if( argument.find( '~' ) != string::npos )
   {
      ch->print( "Command not acceptable, cannot use the ~ character.\r\n" );
      return;
   }

   argument = one_argument( argument, arg );

   if( arg.empty(  ) )
   {
      if( ch->pcdata->alias_map.empty(  ) )
      {
         ch->print( "You have no aliases defined!\r\n" );
         return;
      }
      ch->pagerf( "%-20s What it does\r\n", "Alias" );
      for( al = ch->pcdata->alias_map.begin(  ); al != ch->pcdata->alias_map.end(  ); ++al )
         ch->pagerf( "%-20s %s\r\n", al->first.c_str(  ), al->second.c_str(  ) );
      return;
   }

   if( argument.empty(  ) )
   {
      if( ( al = ch->pcdata->alias_map.find( arg ) ) != ch->pcdata->alias_map.end(  ) )
      {
         ch->pcdata->alias_map.erase( al );
         ch->print( "Deleted Alias.\r\n" );
      }
      else
         ch->print( "That alias does not exist.\r\n" );
      return;
   }

   if( ( al = ch->pcdata->alias_map.find( arg ) ) == ch->pcdata->alias_map.end(  ) )
      ch->print( "Created Alias.\r\n" );
   else
      ch->print( "Modified Alias.\r\n" );

   ch->pcdata->alias_map[arg] = argument;
}

social_type *find_social( const string & command )
{
   map < string, social_type * >::iterator isoc;

   if( ( isoc = social_table.find( command ) ) != social_table.end(  ) )
      return isoc->second;

   return nullptr;
}

map < string, social_type * >social_table;
social_type::social_type(  )
{
   init_memory( &minposition, &minposition, sizeof( minposition ) );

   minposition = POS_RESTING; // Most socials should default to this.
}

/*
 * Free a social structure - Thoric
 */
social_type::~social_type(  )
{
}

void free_socials( void )
{
   map < string, social_type * >::iterator isoc;

   for( isoc = social_table.begin(  ); isoc != social_table.end(  ); ++isoc )
   {
      social_type *soc = isoc->second;

      deleteptr( soc );
   }
   social_table.clear(  );
}

/*
 * Remove a social from it's hash index - Thoric
 */
void unlink_social( social_type * social )
{
   if( !social )
   {
      bug( "%s: nullptr social", __func__ );
      return;
   }

   social_table.erase( social->name );
}

/*
 * Add a social to the social index table - Thoric
 * Hashed and insert sorted
 */
void add_social( social_type * social )
{
   if( !social )
   {
      bug( "%s: nullptr social", __func__ );
      return;
   }

   if( social->name.empty(  ) )
   {
      bug( "%s: nullptr social->name", __func__ );
      return;
   }

   if( social->char_no_arg.empty(  ) )
   {
      bug( "%s: nullptr social->char_no_arg on social %s", __func__, social->name.c_str(  ) );
      return;
   }

   // Force all socials to be lowercase names
   strlower( social->name );

   social_table[social->name] = social;
}

/*
 * Save the socials to disk
 */
void save_socials( void )
{
   ofstream stream;

   stream.open( SOCIAL_FILE );
   if( !stream.is_open(  ) )
   {
      bug( "%s", "Cannot open socials.dat for writting" );
      perror( SOCIAL_FILE );
      return;
   }

   map < string, social_type * >::iterator isoc;
   for( isoc = social_table.begin(  ); isoc != social_table.end(  ); ++isoc )
   {
      social_type *social = isoc->second;

      if( social->name.empty(  ) )
      {
         bug( "%s: blank social in social table", __func__ );
         continue;
      }

      stream << "#SOCIAL" << endl;
      stream << "Name        " << social->name << endl;
      if( !social->char_no_arg.empty(  ) )
         stream << "CharNoArg   " << social->char_no_arg << endl;
      else
         bug( "%s: nullptr char_no_arg in social_table for %s", __func__, social->name.c_str(  ) );
      if( !social->others_no_arg.empty(  ) )
         stream << "OthersNoArg " << social->others_no_arg << endl;
      if( !social->char_found.empty(  ) )
         stream << "CharFound   " << social->char_found << endl;
      if( !social->others_found.empty(  ) )
         stream << "OthersFound " << social->others_found << endl;
      if( !social->vict_found.empty(  ) )
         stream << "VictFound   " << social->vict_found << endl;
      if( !social->char_auto.empty(  ) )
         stream << "CharAuto    " << social->char_auto << endl;
      if( !social->others_auto.empty(  ) )
         stream << "OthersAuto  " << social->others_auto << endl;
      if( !social->obj_self.empty(  ) )
         stream << "ObjSelf     " << social->obj_self << endl;
      if( !social->obj_others.empty(  ) )
         stream << "ObjOthers   " << social->obj_others << endl;
      stream << "MinPosition " << npc_position[social->minposition] << endl;
      stream << "End" << endl << endl;
   }
   stream.close(  );
}

void load_socials( void )
{
   ifstream stream;
   social_type *social = nullptr;

   social_table.clear(  );

   stream.open( SOCIAL_FILE );
   if( !stream.is_open(  ) )
   {
      bug( "%s: Cannot open socials.dat", __func__ );
      exit( 1 );
   }

   do
   {
      string key, value;
      char buf[MIL];

      stream >> key;
      stream.getline( buf, MIL );
      value = buf;

      strip_lspace( key );
      strip_tilde( value );
      strip_lspace( value );

      if( key.empty(  ) )
         continue;

      if( key == "#SOCIAL" )
         social = new social_type;
      else if( key == "Name" )
         social->name = value;
      else if( key == "CharNoArg" )
         social->char_no_arg = value;
      else if( key == "OthersNoArg" )
         social->others_no_arg = value;
      else if( key == "CharFound" )
         social->char_found = value;
      else if( key == "OthersFound" )
         social->others_found = value;
      else if( key == "VictFound" )
         social->vict_found = value;
      else if( key == "CharAuto" )
         social->char_auto = value;
      else if( key == "OthersAuto" )
         social->others_auto = value;
      else if( key == "ObjSelf" )
         social->obj_self = value;
      else if( key == "ObjOthers" )
         social->obj_others = value;
      else if( key == "MinPosition" )
      {
         int minpos = get_npc_position( value );

         if( minpos < POS_SLEEPING || minpos >= POS_MAX )
            minpos = POS_RESTING;

         social->minposition = minpos;
      }
      else if( key == "End" )
         social_table[social->name] = social;
      else
         log_printf( "Bad line in socials file: %s %s", key.c_str(  ), value.c_str(  ) );
   }
   while( !stream.eof(  ) );
   stream.close(  );
}

/*
 * Social editor/displayer/save/delete - Thoric
 */
CMDF( do_sedit )
{
   social_type *social;
   string arg1, arg2;

   ch->set_color( AT_SOCIAL );

   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );
   if( arg1.empty(  ) )
   {
      ch->print( "Syntax: sedit <social> [field] [data]\r\n" );
      ch->print( "Syntax: sedit <social> create\r\n" );
      if( ch->level > LEVEL_GOD )
         ch->print( "Syntax: sedit <social> delete\r\n" );
      ch->print( "Syntax: sedit <save>\r\n" );
      if( ch->level > LEVEL_GREATER )
         ch->print( "Syntax: sedit <social> name <newname>\r\n" );
      ch->print( "\r\nField being one of:\r\n" );
      ch->print( "  cnoarg onoarg cfound ofound vfound cauto oauto objself objothers minposition\r\n" );
      return;
   }

   if( !str_cmp( arg1, "save" ) )
   {
      save_socials(  );
      ch->print( "Social file updated.\r\n" );
      return;
   }

   social = find_social( arg1 );
   if( !str_cmp( arg2, "create" ) )
   {
      if( social )
      {
         ch->print( "That social already exists!\r\n" );
         return;
      }
      social = new social_type;
      social->name = arg1;
      social->char_no_arg = "You " + arg1 + ".";
      add_social( social );
      ch->print( "Social added.\r\n" );
      return;
   }

   if( !social )
   {
      ch->print( "Social not found.\r\n" );
      return;
   }

   if( arg2.empty(  ) || !str_cmp( arg2, "show" ) )
   {
      ch->printf( "Social   : %s\r\n\r\nCNoArg   : %s\r\n", social->name.c_str(  ), social->char_no_arg.c_str(  ) );
      ch->printf( "ONoArg   : %s\r\nCFound   : %s\r\nOFound   : %s\r\n",
                  !social->others_no_arg.empty(  )? social->others_no_arg.c_str(  ) : "(not set)",
                  !social->char_found.empty(  )? social->char_found.c_str(  ) : "(not set)", !social->others_found.empty(  )? social->others_found.c_str(  ) : "(not set)" );
      ch->printf( "VFound   : %s\r\nCAuto    : %s\r\nOAuto    : %s\r\n",
                  !social->vict_found.empty(  )? social->vict_found.c_str(  ) : "(not set)",
                  !social->char_auto.empty(  )? social->char_auto.c_str(  ) : "(not set)", !social->others_auto.empty(  )? social->others_auto.c_str(  ) : "(not set)" );
      ch->printf( "ObjSelf  : %s\r\nObjOthers: %s\r\n",
                  !social->obj_self.empty(  )? social->obj_self.c_str(  ) : "(not set)", !social->obj_others.empty(  )? social->obj_others.c_str(  ) : "(not set)" );
      ch->printf( "MinPos   : %s\r\n", npc_position[social->minposition] );
      return;
   }

   if( ch->level > LEVEL_GOD && !str_cmp( arg2, "delete" ) )
   {
      unlink_social( social );
      deleteptr( social );
      ch->print( "Deleted.\r\n" );
      return;
   }

   if( !str_cmp( arg2, "cnoarg" ) )
   {
      if( argument.empty(  ) || !str_cmp( argument, "clear" ) )
      {
         ch->print( "You cannot clear this field. It must have a message.\r\n" );
         return;
      }
      social->char_no_arg = argument;
      ch->print( "Done.\r\n" );
      return;
   }

   if( !str_cmp( arg2, "onoarg" ) )
   {
      social->others_no_arg.clear(  );
      if( !argument.empty(  ) && str_cmp( argument, "clear" ) )
         social->others_no_arg = argument;
      ch->print( "Done.\r\n" );
      return;
   }

   if( !str_cmp( arg2, "cfound" ) )
   {
      social->char_found.clear(  );
      if( !argument.empty(  ) && str_cmp( argument, "clear" ) )
         social->char_found = argument;
      ch->print( "Done.\r\n" );
      return;
   }

   if( !str_cmp( arg2, "ofound" ) )
   {
      social->others_found.clear(  );
      if( !argument.empty(  ) && str_cmp( argument, "clear" ) )
         social->others_found = argument;
      ch->print( "Done.\r\n" );
      return;
   }

   if( !str_cmp( arg2, "vfound" ) )
   {
      social->vict_found.clear(  );
      if( !argument.empty(  ) && str_cmp( argument, "clear" ) )
         social->vict_found = argument;
      ch->print( "Done.\r\n" );
      return;
   }

   if( !str_cmp( arg2, "cauto" ) )
   {
      social->char_auto.clear(  );
      if( !argument.empty(  ) && str_cmp( argument, "clear" ) )
         social->char_auto = argument;
      ch->print( "Done.\r\n" );
      return;
   }

   if( !str_cmp( arg2, "oauto" ) )
   {
      social->others_auto.clear(  );
      if( !argument.empty(  ) && str_cmp( argument, "clear" ) )
         social->others_auto = argument;
      ch->print( "Done.\r\n" );
      return;
   }

   if( !str_cmp( arg2, "objself" ) )
   {
      social->obj_self.clear(  );
      if( !argument.empty(  ) && str_cmp( argument, "clear" ) )
         social->obj_self = argument;
      ch->print( "Done.\r\n" );
      return;
   }

   if( !str_cmp( arg2, "objothers" ) )
   {
      social->obj_others.clear(  );
      if( !argument.empty(  ) && str_cmp( argument, "clear" ) )
         social->obj_others = argument;
      ch->print( "Done.\r\n" );
      return;
   }

   if( !str_cmp( arg2, "minposition" ) )
   {
      int minpos = get_npc_position( arg2 );

      if( minpos < POS_SLEEPING || minpos >= POS_MAX )
      {
         ch->printf( "%s is not a valid position.\r\n", arg2.c_str() );
         return;
      }
      social->minposition = minpos;
      ch->print( "Done.\r\n" );
      return;
   }

   if( ch->level > LEVEL_GREATER && !str_cmp( arg2, "name" ) )
   {
      social_type *checksocial;

      one_argument( argument, arg1 );
      if( arg1.empty(  ) )
      {
         ch->print( "Cannot clear name field!\r\n" );
         return;
      }

      if( ( checksocial = find_social( arg1 ) ) != nullptr )
      {
         ch->printf( "There is already a social named %s.\r\n", arg1.c_str(  ) );
         return;
      }

      unlink_social( social );
      social->name = arg1;
      add_social( social );
      ch->print( "Done.\r\n" );
      return;
   }

   /*
    * display usage message 
    */
   do_sedit( ch, "" );
}

/* Modified by Xorith to work more like do_commands and to be cleaner */
/* Modified again. I added booleans for sorted and all option checking. It occured to me
   that calling str_cmp three times within the loop is probably costing us a bit in performance. -- Xorith */
CMDF( do_socials )
{
   int col = 0, count = 0;
   char letter = ' ';
   social_type *social;
   bool sorted = false, all = false;

   if( !argument.empty(  ) && !str_cmp( argument, "sorted" ) )
      sorted = true;
   if( !argument.empty(  ) && !str_cmp( argument, "all" ) )
      all = true;

   if( !argument.empty(  ) && !( sorted || all ) )
      ch->pagerf( "&[plain]Socials beginning with '%s':\r\n", argument.c_str(  ) );
   else if( !argument.empty(  ) && sorted )
      ch->pager( "&[plain]Socials -- Tabbed by Letter\r\n" );
   else
      ch->pager( "&[plain]Socials -- All\r\n" );

   map < string, social_type * >::iterator isoc;
   for( isoc = social_table.begin(  ); isoc != social_table.end(  ); ++isoc )
   {
      social = isoc->second;

      if( ( !argument.empty(  ) && !( sorted || all ) ) && str_prefix( argument, social->name ) )
         continue;

      if( !argument.empty(  ) && sorted && letter != social->name[0] )
      {
         letter = social->name[0];
         if( col % 5 != 0 )
            ch->pager( "\r\n" );
         ch->pagerf( "&c[ &[plain]%c &c]\r\n", toupper( letter ) );
         col = 0;
      }
      ch->pagerf( "&[plain]%-15s ", social->name.c_str(  ) );
      ++count;
      if( ++col % 5 == 0 )
         ch->pager( "\r\n" );
   }

   if( col % 5 != 0 )
      ch->pager( "\r\n" );

   if( count )
      ch->pagerf( "&[plain]   %d socials found.\r\n", count );
   else
      ch->pager( "&[plain]   No socials found.\r\n" );
}
