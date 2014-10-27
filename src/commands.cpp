/****************************************************************************
 *                   ^     +----- |  / ^     ^ |     | +-\                  *
 *                  / \    |      | /  |\   /| |     | |  \                 *
 *                 /   \   +---   |<   | \ / | |     | |  |                 *
 *                /-----\  |      | \  |  v  | |     | |  /                 *
 *               /       \ |      |  \ |     | +-----+ +-/                  *
 ****************************************************************************
 * AFKMud Copyright 1997-2008 by Roger Libiez (Samson),                     *
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
 *                              Command Code                                *
 ****************************************************************************/

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

cmd_type *command_hash[126];   /* hash table for cmd_table */
social_type *social_index[27]; /* hash table for socials   */
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
bool check_ability( char_data *, char *, char * );
bool check_skill( char_data *, char *, char * );
#ifdef MULTIPORT
bool shell_hook( char_data *, char *, char * );
void shellcommands( char_data *, int );
#endif
#ifdef IMC
bool imc_command_hook( char_data *, char *, char * );
#endif
bool local_channel_hook( char_data *, char *, char * );
int get_logflag( char * );
void skill_notfound( char_data *, char * );
char_data *get_wizvictim( char_data *, char *, bool );
char *extract_area_names( char_data * );
bool can_use_mprog( char_data * );

char *const cmd_flags[] = {
   "possessed", "polymorphed", "action", "nospam", "ghost", "mudprog",
   "noforce", "loaded"
};

void check_switches( )
{
   list<char_data*>::iterator ich;

   for( ich = pclist.begin(); ich != pclist.end(); ++ich )
   {
      char_data *ch = *ich;

      check_switch( ch );
   }
}

CMDF( do_switch );
void check_switch( char_data *ch )
{
   cmd_type *cmd;
   int hash, trust = ch->get_trust();

   if( !ch->switched )
      return;

   for( hash = 0; hash < 126; hash++ )
   {
      for( cmd = command_hash[hash]; cmd; cmd = cmd->next )
      {
         if( cmd->do_fun != do_switch )
            continue;
         if( cmd->level <= trust )
            return;

         if( !ch->isnpc() && ch->pcdata->bestowments && is_name( cmd->name, ch->pcdata->bestowments )
          && cmd->level <= trust + sysdata->bestow_dif )
            return;
      }
   }

   ch->switched->set_color( AT_BLUE );
   ch->switched->print( "You suddenly forfeit the power to switch!\r\n" );
   interpret( ch->switched, "return" );
}

int check_command_level( const char *arg, int check )
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
      snprintf( cmd_flag_buf, MSL, "You can't %s while you are possessing someone!\r\n", cmd->name );
   else if( ch->morph != NULL && cmd->flags.test( CMD_POLYMORPHED ) )
      snprintf( cmd_flag_buf, MSL, "You can't %s while you are polymorphed!\r\n", cmd->name );
   else
      cmd_flag_buf[0] = '\0';
   return cmd_flag_buf;
}

void start_timer( struct timeval *starttime )
{
   if( !starttime )
   {
      bug( "%s: NULL stime.", __FUNCTION__ );
      return;
   }
   gettimeofday( starttime, NULL );
   return;
}

time_t end_timer( struct timeval * starttime )
{
   struct timeval etime;

   /*
    * Mark etime before checking stime, so that we get a better reading.. 
    */
   gettimeofday( &etime, NULL );
   if( !starttime || ( !starttime->tv_sec && !starttime->tv_usec ) )
   {
      bug( "%s: bad starttime.", __FUNCTION__ );
      return 0;
   }
   subtract_times( &etime, starttime );
   /*
    * stime becomes time used 
    */
   *starttime = etime;
   return ( etime.tv_sec * 1000000 ) + etime.tv_usec;
}

bool check_social( char_data * ch, char *command, char *argument )
{
   char arg[MIL];
   social_type *social;
   char_data *victim = NULL;

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
          */
         if( !str_cmp( social->name, "snore" ) )
            break;
         ch->print( "In your dreams, or what?\r\n" );
         return true;
   }

   one_argument( argument, arg );
   if( !arg || arg[0] == '\0' )
   {
      act( AT_SOCIAL, social->char_no_arg, ch, NULL, victim, TO_CHAR );
      act( AT_SOCIAL, social->others_no_arg, ch, NULL, victim, TO_ROOM );
      return true;
   }

   if( !( victim = ch->get_char_room( arg ) ) )
   {
      obj_data *obj; /* Object socials */
      if( ( ( obj = get_obj_list( ch, arg, ch->in_room->objects ) )
            || ( obj = get_obj_list( ch, arg, ch->carrying ) ) ) && !victim )
      {
         if( social->obj_self && social->obj_others )
         {
            act( AT_SOCIAL, social->obj_self, ch, NULL, obj, TO_CHAR );
            act( AT_SOCIAL, social->obj_others, ch, NULL, obj, TO_ROOM );
         }
         return true;
      }

      if( !victim )
         ch->print( "They aren't here.\r\n" );

      return true;
   }

   if( victim == ch )
   {
      act( AT_SOCIAL, social->others_auto, ch, NULL, victim, TO_ROOM );
      act( AT_SOCIAL, social->char_auto, ch, NULL, victim, TO_CHAR );
      return true;
   }

   act( AT_SOCIAL, social->others_found, ch, NULL, victim, TO_NOTVICT );
   act( AT_SOCIAL, social->char_found, ch, NULL, victim, TO_CHAR );
   act( AT_SOCIAL, social->vict_found, ch, NULL, victim, TO_VICT );

   if( !ch->isnpc(  ) && victim->isnpc(  ) && !victim->has_aflag( AFF_CHARM )
       && victim->IS_AWAKE(  ) && !HAS_PROG( victim->pIndexData, ACT_PROG ) )
   {
      switch ( number_bits( 4 ) )
      {
         default:
         case 0:
            if( ch->IS_EVIL(  ) || ch->IS_NEUTRAL(  ) )
            {
               act( AT_ACTION, "$n slaps $N.", victim, NULL, ch, TO_NOTVICT );
               act( AT_ACTION, "You slap $N.", victim, NULL, ch, TO_CHAR );
               act( AT_ACTION, "$n slaps you.", victim, NULL, ch, TO_VICT );
            }
            else
            {
               act( AT_ACTION, "$n acts like $N doesn't even exist.", victim, NULL, ch, TO_NOTVICT );
               act( AT_ACTION, "You just ignore $N.", victim, NULL, ch, TO_CHAR );
               act( AT_ACTION, "$n appears to be ignoring you.", victim, NULL, ch, TO_VICT );
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
            act( AT_SOCIAL, social->others_found, victim, NULL, ch, TO_NOTVICT );
            act( AT_SOCIAL, social->char_found, victim, NULL, ch, TO_CHAR );
            act( AT_SOCIAL, social->vict_found, victim, NULL, ch, TO_VICT );
            break;

         case 9:
         case 10:
         case 11:
         case 12:
            act( AT_ACTION, "$n slaps $N.", victim, NULL, ch, TO_NOTVICT );
            act( AT_ACTION, "You slap $N.", victim, NULL, ch, TO_CHAR );
            act( AT_ACTION, "$n slaps you.", victim, NULL, ch, TO_VICT );
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

bool check_alias( char_data *ch, char *command, char *argument )
{
   map<string,string>::iterator al;
   char arg[MIL];

   if( ch->isnpc(  ) )
      return false;

   al = ch->pcdata->alias_map.find( command );

   if( al == ch->pcdata->alias_map.end() )
      return false;

   mudstrlcpy( arg, ch->pcdata->alias_map[command].c_str(), MIL );

   if( ch->pcdata->cmd_recurse == -1 || ++ch->pcdata->cmd_recurse > 50 )
   {
      if( ch->pcdata->cmd_recurse != -1 )
      {
         ch->print( "Unable to further process command, recurses too much.\r\n" );
         ch->pcdata->cmd_recurse = -1;
      }
      return false;
   }

   if( argument && argument[0] != '\0' )
   {
      mudstrlcat( arg, " ", MIL );
      mudstrlcat( arg, argument, MIL );
   }
   interpret( ch, arg );
   return true;
}

/*
 * The main entry point for executing commands.
 * Can be recursively called from 'at', 'order', 'force'.
 */
void interpret( char_data * ch, char *argument )
{
   char command[MIL], logline[MIL];
   char *buf;
   timer_data *chtimer = NULL;
   cmd_type *cmd = NULL;
   int trust, loglvl;
   bool found;
   struct timeval time_used;
   long tmptime;

   if( !ch )
   {
      bug( "%s: null ch!", __FUNCTION__ );
      return;
   }

   if( !ch->in_room )
   {
      bug( "%s: %s null in_room!", __FUNCTION__, ch->name );
      return;
   }

   found = false;
   if( ch->substate == SUB_REPEATCMD )
   {
      DO_FUN *fun;

      if( !( fun = ch->last_cmd ) )
      {
         ch->substate = SUB_NONE;
         bug( "%s: %s SUB_REPEATCMD with NULL last_cmd", __FUNCTION__, ch->name );
         return;
      }
      else
      {
         int x;

         /*
          * yes... we lose out on the hashing speediness here...
          * but the only REPEATCMDS are wizcommands (currently)
          */
         for( x = 0; x < 126; ++x )
         {
            for( cmd = command_hash[x]; cmd; cmd = cmd->next )
               if( cmd->do_fun == fun )
               {
                  found = true;
                  break;
               }
            if( found )
               break;
         }
         if( !found )
         {
            cmd = NULL;
            bug( "%s: SUB_REPEATCMD: last_cmd invalid", __FUNCTION__ );
            return;
         }
         snprintf( logline, MIL, "(%s) %s", cmd->name, argument );
      }
   }

   if( !cmd )
   {
      /*
       * Changed the order of these ifchecks to prevent crashing. 
       */
      if( !argument || argument[0] == '\0' || !str_cmp( argument, "" ) )
      {
         bug( "%s: null argument!", __FUNCTION__ );
         return;
      }

      /*
       * Strip leading spaces.
       */
      while( isspace( *argument ) )
         ++argument;
      if( argument[0] == '\0' )
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
            act( AT_GREY, "$n is no longer afk.", ch, NULL, NULL, TO_CANSEE );
            ch->print( "You are no longer afk.\r\n" );
         }
         return;
      }

      /*
       * Grab the command word.
       * Special parsing so ' can be a command, also no spaces needed after punctuation.
       */
      mudstrlcpy( logline, argument, MIL );
      if( !isalpha( argument[0] ) && !isdigit( argument[0] ) )
      {
         command[0] = argument[0];
         command[1] = '\0';
         ++argument;
         while( isspace( *argument ) )
            ++argument;
      }
      else
         argument = one_argument( argument, command );

      /*
       * Look for command in command table.
       * Check for bestowments
       */
      trust = ch->get_trust(  );
      for( cmd = command_hash[LOWER( command[0] ) % 126]; cmd; cmd = cmd->next )
         if( !str_prefix( command, cmd->name )
             && ( cmd->level <= trust || ( !ch->isnpc(  ) && ch->pcdata->bestowments
                                                                && ch->pcdata->bestowments[0] != '\0'
                                                                && hasname( ch->pcdata->bestowments, cmd->name )
                                                                && cmd->level <= ( trust + sysdata->bestow_dif ) ) ) )
         {
            found = true;
            break;
         }

      if( found && !cmd->do_fun )
      {
         char filename[256];
#if !defined(WIN32)
         const char *error;
#else
         DWORD error;
#endif

         snprintf( filename, 256, "../src/cmd/so/do_%s.so", cmd->name );
         cmd->fileHandle = dlopen( filename, RTLD_NOW );
         if( ( error = dlerror() ) == NULL )
         {
            cmd->do_fun = ( DO_FUN * )( dlsym( cmd->fileHandle, cmd->fun_name ) );
            cmd->flags.set( CMD_LOADED );
         }
         else
         {
            ch->print( "Huh?\r\n" );
            bug( "%s: %s", __FUNCTION__, error );
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
         act( AT_GREY, "$n is no longer afk.", ch, NULL, NULL, TO_CANSEE );
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
      mudstrlcpy( logline, "XXXXXXXX XXXXXXXX XXXXXXXX", MIL );

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
      if( ch->desc && ch->desc->original )
         log_printf_plus( loglvl, ch->level, "Log %s (%s): %s", ch->name, ch->desc->original->name, logline );
      else
         log_printf_plus( loglvl, ch->level, "Log %s: %s", ch->name, logline );
   }

   if( ch->desc && ch->desc->snoop_by )
   {
      ch->desc->snoop_by->write_to_buffer( ch->name, 0 );
      ch->desc->snoop_by->write_to_buffer( "% ", 2 );
      ch->desc->snoop_by->write_to_buffer( logline, 0 );
      ch->desc->snoop_by->write_to_buffer( "\r\n", 2 );
   }

   /*
    * check for a timer delayed command (search, dig, detrap, etc) 
    */
   if( ( chtimer = ch->get_timerptr( TIMER_DO_FUN ) ) != NULL )
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
          && !check_alias( ch, command, argument )
          && !check_social( ch, command, argument )
#ifdef IMC
          && !imc_command_hook( ch, command, argument )
#endif
          )
      {
         exit_data *pexit;

         /*
          * check for an auto-matic exit command 
          */
         if( ( pexit = find_door( ch, command, true ) ) != NULL && IS_EXIT_FLAG( pexit, EX_xAUTO ) )
         {
            if( IS_EXIT_FLAG( pexit, EX_CLOSED )
                && ( !ch->has_aflag( AFF_PASS_DOOR ) || IS_EXIT_FLAG( pexit, EX_NOPASSDOOR ) ) )
            {
               if( !IS_EXIT_FLAG( pexit, EX_SECRET ) )
                  act( AT_PLAIN, "The $d is closed.", ch, NULL, pexit->keyword, TO_CHAR );
               else
                  ch->print( "You cannot do that here.\r\n" );
               return;
            }
            if( ( IS_EXIT_FLAG( pexit, EX_FORTIFIED ) || IS_EXIT_FLAG( pexit, EX_HEAVY )
                  || IS_EXIT_FLAG( pexit, EX_MEDIUM ) || IS_EXIT_FLAG( pexit, EX_LIGHT )
                  || IS_EXIT_FLAG( pexit, EX_CRUMBLING ) ) )
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
      char *tmpargument = argument;
      char tmparg[MIL];

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
   catch( exception &e )
   {
      bug( "&YCommand exception: '%s' on command: %s %s&D", e.what(), cmd->name, argument );
   }
   catch( ... )
   {
      bug( "&YUnknown command exception on command: %s %s&D", cmd->name, argument );
   }

   tmptime = UMIN( time_used.tv_sec, 19 ) * 1000000 + time_used.tv_usec;

   /*
    * laggy command notice: command took longer than 3.0 seconds 
    */
   if( tmptime > 3000000 )
   {
      log_printf_plus( LOG_NORMAL, ch->level, "[*****] LAG: %s: %s %s (R:%d S:%ld.%06ld)", ch->name,
                       cmd->name, ( cmd->log == LOG_NEVER ? "XXX" : argument ),
                       ch->in_room ? ch->in_room->vnum : 0, time_used.tv_sec, time_used.tv_usec );
   }
   mudstrlcpy( lastplayercmd, "No commands pending", MIL * 2 );
   tail_chain(  );
}

void cmdf( char_data * ch, char *fmt, ... )
{
   char buf[MSL * 2];
   va_list args;

   va_start( args, fmt );
   vsnprintf( buf, MSL * 2, fmt, args );
   va_end( args );

   interpret( ch, buf );
}

/* Be damn sure the function you pass here is valid, or Bad Things(tm) will happen. */
void funcf( char_data * ch, DO_FUN * cmd, char *fmt, ... )
{
   char buf[MSL * 2];
   va_list args;

   if( !cmd )
   {
      bug( "%s: Bad function passed to funcf!", __FUNCTION__ );
      return;
   }

   va_start( args, fmt );
   vsnprintf( buf, MSL * 2, fmt, args );
   va_end( args );

   ( cmd ) ( ch, buf );
}

cmd_type::cmd_type()
{
   init_memory( &next, &log, sizeof( log ) );
}

/*
 * Free a command structure - Thoric
 */
cmd_type::~cmd_type()
{
   next = NULL;
   do_fun = NULL;
   DISPOSE( name );
   DISPOSE( fun_name );
}

void free_commands( void )
{
   cmd_type *command, *cmd_next;
   int hash;

   for( hash = 0; hash < 126; ++hash )
   {
      for( command = command_hash[hash]; command; command = cmd_next )
      {
         cmd_next = command->next;

         deleteptr( command );
      }
   }
   return;
}

/*
 * Remove a command from it's hash index - Thoric
 */
void unlink_command( cmd_type *command )
{
   cmd_type *tmp, *tmp_next;
   int hash;

   if( !command )
   {
      bug( "%s: NULL command", __FUNCTION__ );
      return;
   }

   hash = command->name[0] % 126;

   if( command == ( tmp = command_hash[hash] ) )
   {
      command_hash[hash] = tmp->next;
      return;
   }
   for( ; tmp; tmp = tmp_next )
   {
      tmp_next = tmp->next;
      if( command == tmp_next )
      {
         tmp->next = tmp_next->next;
         return;
      }
   }
}

/*
 * Add a command to the command hash table - Thoric
 */
void add_command( cmd_type *command )
{
   int hash, x;
   cmd_type *tmp, *prev;

   if( !command )
   {
      bug( "%s: NULL command", __FUNCTION__ );
      return;
   }

   if( !command->name )
   {
      bug( "%s: NULL command->name", __FUNCTION__ );
      return;
   }

/*   if( !command->do_fun )
   {
      bug( "%s: NULL command->do_fun for command %s", __FUNCTION__, command->name );
      return;
   } */

   /*
    * make sure the name is all lowercase 
    */
   for( x = 0; command->name[x] != '\0'; ++x )
      command->name[x] = LOWER( command->name[x] );

   hash = command->name[0] % 126;

   if( !( prev = tmp = command_hash[hash] ) )
   {
      command->next = command_hash[hash];
      command_hash[hash] = command;
      return;
   }

   /*
    * add to the END of the list 
    */
   for( ; tmp; tmp = tmp->next )
      if( !tmp->next )
      {
         tmp->next = command;
         command->next = NULL;
      }
   return;
}

cmd_type *find_command( string command )
{
   cmd_type *cmd;
   int hash;

   hash = LOWER( command[0] ) % 126;

   for( cmd = command_hash[hash]; cmd; cmd = cmd->next )
      if( !str_prefix( command.c_str(), cmd->name ) )
         return cmd;

   return NULL;
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
   FILE *fpout;
   cmd_type *command;
   int x;

   if( !( fpout = fopen( COMMAND_FILE, "w" ) ) )
   {
      bug( "%s: Cannot open commands.dat for writing", __FUNCTION__ );
      perror( COMMAND_FILE );
      return;
   }

   fprintf( fpout, "#VERSION	%d\n", CMDVERSION );

   for( x = 0; x < 126; ++x )
   {
      for( command = command_hash[x]; command; command = command->next )
      {
         if( !command->name || command->name[0] == '\0' )
         {
            bug( "%s: blank command in hash bucket %d", __FUNCTION__, x );
            continue;
         }
         fprintf( fpout, "%s", "#COMMAND\n" );
         fprintf( fpout, "Name        %s~\n", command->name );
         /*
          * Modded to use new field - Trax 
          */
         fprintf( fpout, "Code        %s\n", command->fun_name ? command->fun_name : "" );
         fprintf( fpout, "Position    %s~\n", npc_position[command->position] );
         fprintf( fpout, "Level       %d\n", command->level );
         fprintf( fpout, "Log         %s~\n", log_flag[command->log] );
         if( command->flags.any(  ) )
            fprintf( fpout, "Flags       %s~\n", bitset_string( command->flags, cmd_flags ) );
         fprintf( fpout, "%s", "End\n\n" );
      }
   }
   fprintf( fpout, "%s", "#END\n" );
   FCLOSE( fpout );
}

/*
 *  Added the flags Aug 25, 1997 --Shaddai
 */
void fread_command( FILE * fp, int version )
{
   cmd_type *command = new cmd_type;

   for( ;; )
   {
      const char *word = ( feof( fp ) ? "End" : fread_word( fp ) );

      if( word[0] == '\0' )
      {
         bug( "%s: EOF encountered reading file!", __FUNCTION__ );
         word = "End";
      }

      switch ( UPPER( word[0] ) )
      {
         default:
            bug( "%s: no match: %s", __FUNCTION__, word );
            fread_to_eol( fp );
            break;

         case '*':
            fread_to_eol( fp );
            break;

         case 'C':
            KEY( "Code", command->fun_name, str_dup( fread_word( fp ) ) );
            break;

         case 'E':
            if( !str_cmp( word, "End" ) )
            {
               if( !command->name )
               {
                  bug( "%s: Name not found", __FUNCTION__ );
                  deleteptr( command );
                  return;
               }

               if( !command->fun_name )
               {
                  bug( "%s: No function name supplied for %s", __FUNCTION__, command->name );
                  deleteptr( command );
                  return;
               }

               /*
                * Mods by Trax
                * Fread in code into char* and try linkage here then
                * deal in the "usual" way I suppose..
                *
                * Updated by Samson 6/11/2006.
                * Commands that aren't in the core are probably defined in the modules area.
                * These commands will be on a separate modules list at some point.
                * For now, the ones being tested can be loaded by the cedit command.
                */
               command->do_fun = skill_function( command->fun_name );
               if( command->do_fun == skill_notfound )
                  command->do_fun = NULL;

               add_command( command );

               /*
                * Automatically approve all immortal commands for use as a ghost 
                */
               if( command->level >= LEVEL_IMMORTAL )
                  command->flags.set( CMD_GHOST );

               // Command should never load at startup with CMD_LOADED. This bypasses the flag being saved during editing.
               command->flags.reset( CMD_LOADED );
               return;
            }
            break;

         case 'F':
            if( !str_cmp( word, "flags" ) )
            {
               if( version < 3 )
                  command->flags = fread_number( fp );
               else
                  flag_set( fp, command->flags, cmd_flags );
               break;
            }
            break;

         case 'L':
            KEY( "Level", command->level, fread_number( fp ) );
            if( !str_cmp( word, "Log" ) )
            {
               if( version < 2 )
               {
                  command->log = fread_number( fp );
                  break;
               }
               else
               {
                  char *lflag = NULL;
                  int lognum;

                  lflag = fread_flagstring( fp );
                  lognum = get_logflag( lflag );

                  if( lognum < 0 || lognum > LOG_ALL )
                  {
                     bug( "%s: Command %s has invalid log flag! Defaulting to normal.", __FUNCTION__, command->name );
                     lognum = LOG_NORMAL;
                  }
                  command->log = lognum;
                  break;
               }
            }
            break;

         case 'N':
            KEY( "Name", command->name, fread_string_nohash( fp ) );
            break;

         case 'P':
            if( !str_cmp( word, "Position" ) )
            {
               char *tpos = NULL;
               int position;

               tpos = fread_flagstring( fp );
               position = get_npc_position( tpos );

               if( position < 0 || position >= POS_MAX )
               {
                  bug( "%s: Command %s has invalid position! Defaulting to standing.", __FUNCTION__, command->name );
                  position = POS_STANDING;
               }
               command->position = position;
               break;
            }
            break;
      }
   }
}

void load_commands( void )
{
   FILE *fp;
   int version = 0;

   if( ( fp = fopen( COMMAND_FILE, "r" ) ) != NULL )
   {
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
            bug( "%s: # not found.", __FUNCTION__ );
            break;
         }

         word = fread_word( fp );
         if( !str_cmp( word, "VERSION" ) )
         {
            version = fread_number( fp );
            continue;
         }
         if( !str_cmp( word, "COMMAND" ) )
         {
            fread_command( fp, version );
            continue;
         }
         else if( !str_cmp( word, "END" ) )
            break;
         else
         {
            bug( "%s: bad section: %s", __FUNCTION__, word );
            continue;
         }
      }
      FCLOSE( fp );
   }
   else
   {
      bug( "%s: Cannot open commands.dat", __FUNCTION__ );
      exit( 1 );
   }
}

/*
 * For use with cedit --Shaddai
 */
int get_cmdflag( char *flag )
{
   unsigned int x;

   for( x = 0; x < ( sizeof( cmd_flags ) / sizeof( cmd_flags[0] ) ); ++x )
      if( !str_cmp( flag, cmd_flags[x] ) )
         return x;
   return -1;
}

/*
 * Command editor/displayer/save/delete - Thoric
 * Added support for interpret flags    - Shaddai
 */
CMDF( do_cedit )
{
   cmd_type *command;
   char arg1[MIL], arg2[MIL];

   ch->set_color( AT_IMMORT );

   smash_tilde( argument );
   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );
   if( !arg1 || arg1[0] == '\0' )
   {
      ch->print( "Syntax: cedit save cmdtable\r\n" );
      if( ch->get_trust(  ) > LEVEL_SUB_IMPLEM )
      {
         ch->print( "Syntax: cedit load <command>\r\n" );
         ch->print( "Syntax: cedit <command> create [code]\r\n" );
         ch->print( "Syntax: cedit <command> delete\r\n" );
         ch->print( "Syntax: cedit <command> show\r\n" );
         ch->print( "Syntax: cedit <command> raise\r\n" );
         ch->print( "Syntax: cedit <command> lower\r\n" );
         ch->print( "Syntax: cedit <command> list\r\n" );
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
      command->name = str_dup( arg1 );
      command->level = ch->get_trust(  );
      if( *argument )
         one_argument( argument, arg2 );
      else
         snprintf( arg2, MIL, "do_%s", arg1 );
      command->do_fun = skill_function( arg2 );
      command->fun_name = str_dup( arg2 );
      add_command( command );
      ch->print( "Command added.\r\n" );
      if( command->do_fun == skill_notfound )
         ch->printf( "Code %s not found.  Set to no code.\r\n", arg2 );
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

   if( !arg2 || arg2[0] == '\0' || !str_cmp( arg2, "show" ) )
   {
      ch->printf( "Command:   %s\r\nLevel:     %d\r\nPosition:  %s\r\nLog:       %s\r\nFunc Name: %s\r\nFlags:     %s\r\n",
                  command->name, command->level, npc_position[command->position], log_flag[command->log],
                  command->fun_name, bitset_string( command->flags, cmd_flags ) );
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
         ch->printf( "The %s command function is already loaded.\r\n", command->name );
         return;
      }

      if( !command->fun_name || command->fun_name[0] == '\0' )
      {
         ch->printf( "The %s command has a NULL function name!\r\n" );
         return;
      }

      snprintf( filename, 256, "../src/cmd/so/do_%s.so", command->name );
      command->fileHandle = dlopen( filename, RTLD_NOW );
      if( ( error = dlerror() ) == NULL )
      {
         command->do_fun = ( DO_FUN * )( dlsym( command->fileHandle, command->fun_name ) );
         command->flags.set( CMD_LOADED );
         ch->printf( "Command %s loaded and available.\r\n", command->name );
         return;
      }

      ch->printf( "Error: %s\r\n", error );
      return;
   }

   if( !str_cmp( arg2, "unload" ) )
   {
      if( !command->flags.test( CMD_LOADED ) )
      {
         ch->printf( "The %s command function is not loaded.\r\n", command->name );
         return;
      }

      dlclose( command->fileHandle );
      command->flags.reset( CMD_LOADED );
      command->do_fun = NULL;
      ch->printf( "The %s command has been unloaded.\r\n", command->name );
      return;
   }

   if( !str_cmp( arg2, "reload" ) )
   {
      if( !command->flags.test( CMD_LOADED ) )
      {
         ch->printf( "The %s command function is not loaded.\r\n", command->name );
         return;
      }

      dlclose( command->fileHandle );
      command->flags.reset( CMD_LOADED );
      command->do_fun = NULL;
      ch->printf( "The %s command has been unloaded.\r\n", command->name );

      funcf( ch, do_cedit, "%s load", command->name );
      return;
   }

   if( !str_cmp( arg2, "raise" ) )
   {
      cmd_type *tmp, *tmp_next;
      int hash = command->name[0] % 126;

      if( ( tmp = command_hash[hash] ) == command )
      {
         ch->print( "That command is already at the top.\r\n" );
         return;
      }
      if( tmp->next == command )
      {
         command_hash[hash] = command;
         tmp_next = tmp->next;
         tmp->next = command->next;
         command->next = tmp;
         ch->printf( "Moved %s above %s.\r\n", command->name, command->next->name );
         return;
      }
      for( ; tmp; tmp = tmp->next )
      {
         tmp_next = tmp->next;
         if( tmp_next->next == command )
         {
            tmp->next = command;
            tmp_next->next = command->next;
            command->next = tmp_next;
            ch->printf( "Moved %s above %s.\r\n", command->name, command->next->name );
            return;
         }
      }
      ch->print( "ERROR -- Not Found!\r\n" );
      return;
   }

   if( !str_cmp( arg2, "lower" ) )
   {
      cmd_type *tmp, *tmp_next;
      int hash = command->name[0] % 126;

      if( command->next == NULL )
      {
         ch->print( "That command is already at the bottom.\r\n" );
         return;
      }
      tmp = command_hash[hash];
      if( tmp == command )
      {
         tmp_next = tmp->next;
         command_hash[hash] = command->next;
         command->next = tmp_next->next;
         tmp_next->next = command;

         ch->printf( "Moved %s below %s.\r\n", command->name, tmp_next->name );
         return;
      }
      for( ; tmp; tmp = tmp->next )
      {
         if( tmp->next == command )
         {
            tmp_next = command->next;
            tmp->next = tmp_next;
            command->next = tmp_next->next;
            tmp_next->next = command;

            ch->printf( "Moved %s below %s.\r\n", command->name, tmp_next->name );
            return;
         }
      }
      ch->print( "ERROR -- Not Found!\r\n" );
      return;
   }

   if( !str_cmp( arg2, "list" ) )
   {
      cmd_type *tmp;
      int hash = command->name[0] % 126;

      ch->pagerf( "Priority placement for [%s]:\r\n", command->name );
      for( tmp = command_hash[hash]; tmp; tmp = tmp->next )
      {
         if( tmp == command )
            ch->set_pager_color( AT_GREEN );
         else
            ch->set_pager_color( AT_PLAIN );
         ch->pagerf( "  %s\r\n", tmp->name );
      }
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
      DISPOSE( command->fun_name );
      command->fun_name = str_dup( argument );
      ch->print( "Command code updated.\r\n" );
      return;
   }

   if( !str_cmp( arg2, "level" ) )
   {
      int level = atoi( argument );

      if( level < 0 || level > ch->get_trust(  ) )
      {
         ch->print( "Level out of range.\r\n" );
         return;
      }
      if( level > command->level && command->do_fun == do_switch )
      {
         command->level = level;
         check_switches( );
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
         ch->printf( "Unknown flag %s.\r\n", argument );
         return;
      }
      command->flags.flip( flag );
      ch->print( "Command flags updated.\r\n" );
      return;
   }

   if( !str_cmp( arg2, "name" ) )
   {
      bool relocate;
      cmd_type *checkcmd;

      one_argument( argument, arg1 );
      if( !arg1 || arg1[0] == '\0' )
      {
         ch->print( "Cannot clear name field!\r\n" );
         return;
      }
      if( ( checkcmd = find_command( arg1 ) ) != NULL )
      {
         ch->printf( "There is already a command named %s.\r\n", arg1 );
         return;
      }
      if( arg1[0] != command->name[0] )
      {
         unlink_command( command );
         relocate = true;
      }
      else
         relocate = false;
      DISPOSE( command->name );
      command->name = str_dup( arg1 );
      if( relocate )
         add_command( command );
      ch->print( "Done.\r\n" );
      return;
   }

   /*
    * display usage message 
    */
   do_cedit( ch, "" );
}

CMDF( do_restrict )
{
   char arg[MIL];
   short level;
   cmd_type *cmd;
   bool found;

   found = false;
   ch->set_color( AT_IMMORT );

   argument = one_argument( argument, arg );
   if( !arg || arg[0] == '\0' )
   {
      ch->print( "Restrict which command?\r\n" );
      return;
   }

   if( !argument || argument[0] == '\0' )
      level = ch->get_trust(  );
   else
   {
      if( !is_number( argument ) )
      {
         ch->print( "Level must be numeric.\r\n" );
         return;
      }
      level = atoi( argument );
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
      cmdf( ch, "%s show", cmd->name );
      return;
   }
   cmd->level = level;
   ch->printf( "You restrict %s to level %d\r\n", cmd->name, level );
   log_printf( "%s restricting %s to level %d", ch->name, cmd->name, level );
   return;
}

char *extract_command_names( char_data * ch )
{
   char tcomm[MSL];
   char *tbuf = NULL;
   static char comm_names[MSL];

   if( !ch || ch->isnpc(  ) )
      return NULL;

   if( !ch->pcdata->bestowments || ch->pcdata->bestowments[0] == '\0' )
      return NULL;

   tcomm[0] = '\0';
   comm_names[0] = '\0';

   tbuf = ch->pcdata->bestowments;
   tbuf = one_argument( tbuf, tcomm );
   if( !tcomm || tcomm[0] == '\0' )
      return NULL;
   while( 1 )
   {
      if( !strstr( tcomm, ".are" ) )
      {
         if( !comm_names || comm_names[0] == '\0' )
            mudstrlcpy( comm_names, tcomm, MSL );
         else
            snprintf( comm_names + strlen( comm_names ), MSL, " %s", tcomm );
      }
      if( !tbuf || tbuf[0] == '\0' )
         break;
      tbuf = one_argument( tbuf, tcomm );

   }
   return comm_names;
}

CMDF( do_bestow )
{
   char arg[MIL];
   char *buf = NULL;
   char_data *victim;
   cmd_type *cmd;

   ch->set_color( AT_IMMORT );

   if( !argument || argument[0] == '\0' )
   {
      ch->print( "Syntax:\r\n"
                 "bestow <victim> <command>           adds a command to the list\r\n"
                 "bestow <victim> none                removes bestowed areas\r\n"
                 "bestow <victim> remove <command>    removes a specific area\r\n"
                 "bestow <victim> list                lists bestowed areas\r\n"
                 "bestow <victim>                     lists bestowed areas\r\n" );
      return;
   }

   argument = one_argument( argument, arg );

   if( !( victim = get_wizvictim( ch, arg, true ) ) )
      return;

   if( !argument || argument[0] == '\0' || !str_cmp( argument, "list" ) )
   {
      if( !victim->pcdata->bestowments || victim->pcdata->bestowments[0] == '\0' )
      {
         ch->printf( "%s has no bestowed commands.\r\n", victim->name );
         return;
      }

      if( !( buf = extract_command_names( victim ) ) || buf[0] == '\0' )
      {
         ch->printf( "%s has no bestowed commands.\r\n", victim->name );
         return;
      }
      ch->printf( "Current bestowed commands on %s: %s.\r\n", victim->name, buf );
      return;
   }

   argument = one_argument( argument, arg );

   if( argument && argument[0] != '\0' && !str_cmp( arg, "remove" ) )
   {
      while( 1 )
      {
         argument = one_argument( argument, arg );
         if( !hasname( victim->pcdata->bestowments, arg ) )
         {
            ch->printf( "%s does not have a command named %s bestowed.\r\n", victim->name, arg );
            return;
         }
         removename( &victim->pcdata->bestowments, arg );
         ch->printf( "Removed command %s from %s.\r\n", arg, victim->name );
         victim->save(  );
         if( !argument || argument[0] == '\0' )
            break;
      }
      return;
   }

   if( !str_cmp( arg, "none" ) )
   {
      if( !victim->pcdata->bestowments || victim->pcdata->bestowments[0] == '\0' ||
          !( buf = extract_command_names( victim ) ) || buf[0] == '\0' )
      {
         ch->printf( "%s has no commands bestowed!\r\n", victim->name );
         return;
      }
      buf = NULL;
      if( strstr( victim->pcdata->bestowments, ".are" ) )
         buf = extract_area_names( victim );
      STRFREE( victim->pcdata->bestowments );
      if( buf && buf[0] != '\0' )
         victim->pcdata->bestowments = STRALLOC( buf );
      else
         victim->pcdata->bestowments = STRALLOC( "" );
      ch->printf( "Command bestowments removed from %s.\r\n", victim->name );
      victim->printf( "%s has removed your bestowed commands.\r\n", ch->name );
      check_switch( victim );
      victim->save(  );
      return;
   }

   while( 1 )
   {
      if( strstr( arg, ".are" ) )
      {
         ch->printf( "'%s' is not a valid command to bestow.\r\n", arg );
         ch->print( "You cannot bestow an area with 'bestow'. Use 'bestowarea'.\r\n" );
         return;
      }

      if( hasname( victim->pcdata->bestowments, arg ) )
      {
         ch->printf( "%s already has '%s' bestowed.\r\n", victim->name, arg );
         return;
      }

      if( !( cmd = find_command( arg ) ) )
      {
         ch->printf( "'%s' is not a valid command.\r\n", arg );
         return;
      }

      if( cmd->level > ch->get_trust(  ) )
      {
         ch->printf( "The command '%s' is beyond you, thus you cannot bestow it.\r\n", arg );
         return;
      }

      smash_tilde( arg );
      addname( &victim->pcdata->bestowments, arg );
      victim->printf( "%s has bestowed on you the command: %s\r\n", ch->name, arg );
      ch->printf( "%s has been bestowed: %s\r\n", victim->name, arg );
      victim->save(  );
      if( !argument || argument[0] == '\0' )
         break;
      else
         argument = one_argument( argument, arg );
   }
   return;
}

/*
 * Thanks to Grodyn for pointing out bugs in this function.
 */
/* Samson 4-7-98, added protection against forced delete */
CMDF( do_force )
{
   cmd_type *cmd = NULL;
   char arg[MIL], arg2[MIL], command[MSL];

   ch->set_color( AT_IMMORT );

   argument = one_argument( argument, arg );
   argument = one_argument( argument, arg2 );
   if( !arg || arg[0] == '\0' || !arg2 || arg2[0] == '\0' )
   {
      ch->print( "Force whom to do what?\r\n" );
      return;
   }

   bool mobsonly = ch->level < sysdata->level_forcepc;
   cmd = find_command( arg2 );
   snprintf( command, MSL, "%s %s", arg2, argument );

   if( !str_cmp( arg, "all" ) )
   {
      if( mobsonly )
      {
         ch->print( "You are not of sufficient level to force players yet.\r\n" );
         return;
      }

      if( cmd && cmd->flags.test( CMD_NOFORCE ) )
      {
         ch->printf( "You cannot force anyone to %s\r\n", cmd->name );
         log_printf( "%s attempted to force all to %s - command is flagged noforce", ch->name, cmd->name );
         return;
      }

      list<char_data*>::iterator ich;
      for( ich = pclist.begin(); ich != pclist.end(); )
      {
         char_data *vch = (*ich);
         ++ich;

         if( vch->get_trust(  ) < ch->get_trust(  ) )
         {
            act( AT_IMMORT, "$n forces you to '$t'.", ch, command, vch, TO_VICT );
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
         ch->printf( "You cannot force anyone to %s\r\n", cmd->name );
         log_printf( "%s attempted to force %s to %s - command is flagged noforce", ch->name, victim->name, cmd->name );
         return;
      }

      if( ch->level < LEVEL_GOD && victim->isnpc(  ) && !str_prefix( "mp", argument ) )
      {
         ch->print( "You can't force a mob to do that!\r\n" );
         return;
      }
      act( AT_IMMORT, "$n forces you to '$t'.", ch, command, victim, TO_VICT );
      interpret( victim, command );
   }
   ch->print( "Ok.\r\n" );
   return;
}

/* lets the mobile force someone to do something.  must be mortal level
   and the all argument only affects those in the room with the mobile */
CMDF( do_mpforce )
{
   cmd_type *cmd = NULL;
   char arg[MIL], arg2[MIL];

   if( !can_use_mprog( ch ) )
      return;

   argument = one_argument( argument, arg );
   argument = one_argument( argument, arg2 );

   if( !arg2 || arg2[0] == '\0' )
   {
      progbugf( ch, "%s", "Mpforce - Bad syntax: Missing command" );
      return;
   }

   cmd = find_command( arg2 );

   if( !str_cmp( arg, "all" ) )
   {
      list<char_data*>::iterator ich;

      for( ich = ch->in_room->people.begin(); ich != ch->in_room->people.end(); )
      {
         char_data *vch = (*ich);
         ++ich;

         if( vch->get_trust(  ) < ch->get_trust(  ) && ch->can_see( vch, false ) )
         {
            if( cmd && cmd->flags.test( CMD_NOFORCE ) )
            {
               progbugf( ch, "Mpforce: Attempted to force all to %s - command is flagged noforce", arg2 );
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
         progbugf( ch, "Mpforce - No such victim %s", arg );
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
         progbugf( ch, "Mpforce: Attempted to force %s to %s - command is flagged noforce", victim->name, cmd->name );
         return;
      }

      char lbuf[MSL];
      snprintf( lbuf, MSL, "%s ", arg2 );
      if( argument && argument[0] != '\0' )
         mudstrlcat( lbuf, argument, MSL );
      interpret( victim, lbuf );
   }
   return;
}

/* Nearly rewritten by Xorith to be more smooth and nice and all that jazz. *grin* */
/* Modified again. I added booleans for sorted and all option checking. It occured to me
   that calling str_cmp three times within the loop is probably costing us a bit in performance. 
   Also added column headers to make the information more obvious, and changed it to print - instead of a 0
   for no level requirement on commands, just for visual appeal. -- Xorith */
CMDF( do_commands )
{
   int col = 0, hash = 0, count = 0, chTrust = 0;
   char letter = ' ';
   cmd_type *command;
   bool sorted = false, all = false;

   // Set up common variables used in the loop - better performance than calling the functions.
   if( argument[0] != '\0' && !str_cmp( argument, "sorted" ) )
      sorted = true;
   if( argument[0] != '\0' && !str_cmp( argument, "all" ) )
      all = true;
   chTrust = ch->get_trust();

   if( argument[0] != '\0' && !( sorted || all ) )
      ch->pagerf( "&[plain]Commands beginning with '%s':\r\n", argument );
   else if( argument[0] != '\0' && sorted )
      ch->pager( "&[plain]Commands -- Tabbed by Letter\r\n" );
   else
      ch->pager( "&[plain]Commands -- All\r\n" );

   // Doesn't fit well in a sorted view... -- X
   if( !sorted )
   {
      ch->pagerf( "&[plain]%-11s Lvl &[plain]%-11s Lvl &[plain]%-11s Lvl &[plain]%-11s Lvl &[plain]%-11s Lvl\r\n",
               "Command", "Command", "Command", "Command", "Command" );
      ch->pager( "-------------------------------------------------------------------------------\r\n" );
   }

   for( hash = 0; hash < 126; ++hash )
      for( command = command_hash[hash]; command; command = command->next )
      {
         if( command->level > LEVEL_AVATAR || command->level > chTrust || command->flags.test( CMD_MUDPROG ) )
            continue;

         if( ( argument[0] != '\0' && !( sorted || all ) ) && str_prefix( argument, command->name ) )
            continue;

         if( argument[0] != '\0' && sorted && letter != command->name[0] )
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
            ch->pagerf( "&[plain]%-11s   - ", command->name );
         else
            ch->pagerf( "&[plain]%-11s %3d ", command->name, command->level );

         ++count;
         if( ++col % 5 == 0 )
            ch->pager( "\r\n" );
      }

   if( col % 5 != 0 )
      ch->pager( "\r\n" );

   if( count )
      ch->pagerf( "&[plain]  %d commands found.\r\n", count );
   else
      ch->pager( "&[plain]  No commands found.\r\n" );

   return;
}

/* Updated to Sadiq's wizhelp snippet - Thanks Sadiq! */
/* Accepts level argument to display an individual level - Added by Samson 2-20-00 */
CMDF( do_wizhelp )
{
   cmd_type *cmd;
   int col = 0, hash, curr_lvl;

   ch->set_pager_color( AT_PLAIN );

   if( !argument || argument[0] == '\0' )
   {
      for( curr_lvl = LEVEL_IMMORTAL; curr_lvl <= ch->level; ++curr_lvl )
      {
         ch->pager( "\r\n\r\n" );
         col = 1;
         ch->pagerf( "[LEVEL %-2d]\r\n", curr_lvl );
         for( hash = 0; hash < 126; ++hash )
            for( cmd = command_hash[hash]; cmd; cmd = cmd->next )
               if( ( cmd->level == curr_lvl ) && cmd->level <= ch->level )
               {
                  ch->pagerf( "%-12s", cmd->name );
                  if( ++col % 6 == 0 )
                     ch->pager( "\r\n" );
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

   curr_lvl = atoi( argument );

   if( curr_lvl < LEVEL_IMMORTAL || curr_lvl > MAX_LEVEL )
   {
      ch->printf( "Valid levels are between %d and %d.\r\n", LEVEL_IMMORTAL, MAX_LEVEL );
      return;
   }

   ch->pager( "\r\n\r\n" );
   col = 1;
   ch->pagerf( "[LEVEL %-2d]\r\n", curr_lvl );
   for( hash = 0; hash < 126; ++hash )
      for( cmd = command_hash[hash]; cmd; cmd = cmd->next )
         if( cmd->level == curr_lvl )
         {
            ch->pagerf( "%-12s", cmd->name );
            if( ++col % 6 == 0 )
               ch->pager( "\r\n" );
         }
   if( col % 6 != 0 )
      ch->pager( "\r\n" );
#ifdef MULTIPORT
   shellcommands( ch, curr_lvl );
#endif
   return;
}

CMDF( do_timecmd )
{
   struct timeval starttime;
   struct timeval endtime;
   static bool timing;
   char arg[MIL];

   ch->print( "Timing\r\n" );
   if( timing )
      return;

   one_argument( argument, arg );
   if( !*arg )
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
   gettimeofday( &starttime, NULL );
   interpret( ch, argument );
   gettimeofday( &endtime, NULL );
   timing = false;
   ch->print( "&[plain]Timing complete.\r\n" );
   subtract_times( &endtime, &starttime );
   ch->printf( "Timing took %ld.%06ld seconds.\r\n", endtime.tv_sec, endtime.tv_usec );
   return;
}

/******************************************************
      Desolation of the Dragon MUD II - Alias Code
      (C) 1997, 1998  Jesse DeFer and Heath Leach
 http://dotd.mudservices.com  dotd@dotd.mudservices.com 
 ******************************************************/

CMDF( do_alias )
{
   map<string,string>::iterator al;
   char arg[MIL];
   char *p;

   if( ch->isnpc(  ) )
      return;

   for( p = argument; *p != '\0'; ++p )
   {
      if( *p == '~' )
      {
         ch->print( "Command not acceptable, cannot use the ~ character.\r\n" );
         return;
      }
   }

   argument = one_argument( argument, arg );

   if( !arg || arg[0] == '\0' )
   {
      if( ch->pcdata->alias_map.empty() )
      {
         ch->print( "You have no aliases defined!\r\n" );
         return;
      }
      ch->pagerf( "%-20s What it does\r\n", "Alias" );
      for( al = ch->pcdata->alias_map.begin(); al != ch->pcdata->alias_map.end(); ++al )
         ch->pagerf( "%-20s %s\r\n", al->first.c_str(), al->second.c_str() );
      return;
   }

   if( !argument || argument[0] == '\0' )
   {
      if( ( al = ch->pcdata->alias_map.find( arg ) ) != ch->pcdata->alias_map.end() )
      {
         ch->pcdata->alias_map.erase( al );
         ch->print( "Deleted Alias.\r\n" );
      }
      else
         ch->print( "That alias does not exist.\r\n" );
      return;
   }

   if( ( al = ch->pcdata->alias_map.find( arg ) ) == ch->pcdata->alias_map.end() )
   {
      ch->pcdata->alias_map[arg] = argument;
      ch->print( "Created Alias.\r\n" );
   }
   else
   {
      ch->pcdata->alias_map[arg] = argument;
      ch->print( "Modified Alias.\r\n" );
   }
}

social_type *find_social( string command )
{
   social_type *social;
   int hash;

   command[0] = LOWER(command[0]);

   if( command[0] < 'a' || command[0] > 'z' )
      hash = 0;
   else
      hash = ( command[0] - 'a' ) + 1;

   for( social = social_index[hash]; social; social = social->next )
      if( !str_prefix( command.c_str(), social->name ) )
         return social;

   return NULL;
}

social_type::social_type()
{
   init_memory( &next, &obj_others, sizeof( obj_others ) );
}

/*
 * Free a social structure - Thoric
 */
social_type::~social_type()
{
   DISPOSE( name );
   DISPOSE( char_no_arg );
   DISPOSE( others_no_arg );
   DISPOSE( char_found );
   DISPOSE( others_found );
   DISPOSE( vict_found );
   DISPOSE( char_auto );
   DISPOSE( others_auto );
   DISPOSE( obj_self );
   DISPOSE( obj_others );
}

void free_socials( void )
{
   social_type *social, *social_next;
   int hash;

   for( hash = 0; hash < 27; ++hash )
   {
      for( social = social_index[hash]; social; social = social_next )
      {
         social_next = social->next;
         deleteptr( social );
      }
   }
   return;
}

/*
 * Remove a social from it's hash index - Thoric
 */
void unlink_social( social_type * social )
{
   social_type *tmp, *tmp_next;
   int hash;

   if( !social )
   {
      bug( "%s: NULL social", __FUNCTION__ );
      return;
   }

   if( social->name[0] < 'a' || social->name[0] > 'z' )
      hash = 0;
   else
      hash = ( social->name[0] - 'a' ) + 1;

   if( social == ( tmp = social_index[hash] ) )
   {
      social_index[hash] = tmp->next;
      return;
   }
   for( ; tmp; tmp = tmp_next )
   {
      tmp_next = tmp->next;
      if( social == tmp_next )
      {
         tmp->next = tmp_next->next;
         return;
      }
   }
}

/*
 * Add a social to the social index table - Thoric
 * Hashed and insert sorted
 */
void add_social( social_type * social )
{
   int hash, x;
   social_type *tmp, *prev;

   if( !social )
   {
      bug( "%s: NULL social", __FUNCTION__ );
      return;
   }

   if( !social->name )
   {
      bug( "%s: NULL social->name", __FUNCTION__ );
      return;
   }

   if( !social->char_no_arg )
   {
      bug( "%s: NULL social->char_no_arg on social %s", __FUNCTION__, social->name );
      return;
   }

   /*
    * make sure the name is all lowercase 
    */
   for( x = 0; social->name[x] != '\0'; ++x )
      social->name[x] = LOWER( social->name[x] );

   if( social->name[0] < 'a' || social->name[0] > 'z' )
      hash = 0;
   else
      hash = ( social->name[0] - 'a' ) + 1;

   if( ( prev = tmp = social_index[hash] ) == NULL )
   {
      social->next = social_index[hash];
      social_index[hash] = social;
      return;
   }

   for( ; tmp; tmp = tmp->next )
   {
      if( !str_cmp( social->name, tmp->name ) )
      {
         bug( "%s: trying to add duplicate name to bucket %d", __FUNCTION__, hash );
         deleteptr( social );
         return;
      }
      else if( x < 0 )
      {
         if( tmp == social_index[hash] )
         {
            social->next = social_index[hash];
            social_index[hash] = social;
            return;
         }
         prev->next = social;
         social->next = tmp;
         return;
      }
      prev = tmp;
   }

   /*
    * add to end 
    */
   prev->next = social;
   social->next = NULL;
   return;
}

/*
 * Save the socials to disk
 */
void save_socials( void )
{
   FILE *fpout;
   social_type *social;
   int x;

   if( !( fpout = fopen( SOCIAL_FILE, "w" ) ) )
   {
      bug( "%s", "Cannot open socials.dat for writting" );
      perror( SOCIAL_FILE );
      return;
   }

   for( x = 0; x < 27; ++x )
   {
      for( social = social_index[x]; social; social = social->next )
      {
         if( !social->name || social->name[0] == '\0' )
         {
            bug( "%s: blank social in hash bucket %d", __FUNCTION__, x );
            continue;
         }
         fprintf( fpout, "%s", "#SOCIAL\n" );
         fprintf( fpout, "Name        %s~\n", social->name );
         if( social->char_no_arg )
            fprintf( fpout, "CharNoArg   %s~\n", social->char_no_arg );
         else
            bug( "%s: NULL char_no_arg in hash bucket %d", __FUNCTION__, x );
         if( social->others_no_arg )
            fprintf( fpout, "OthersNoArg %s~\n", social->others_no_arg );
         if( social->char_found )
            fprintf( fpout, "CharFound   %s~\n", social->char_found );
         if( social->others_found )
            fprintf( fpout, "OthersFound %s~\n", social->others_found );
         if( social->vict_found )
            fprintf( fpout, "VictFound   %s~\n", social->vict_found );
         if( social->char_auto )
            fprintf( fpout, "CharAuto    %s~\n", social->char_auto );
         if( social->others_auto )
            fprintf( fpout, "OthersAuto  %s~\n", social->others_auto );
         if( social->obj_self )
            fprintf( fpout, "ObjSelf     %s~\n", social->obj_self );
         if( social->obj_others )
            fprintf( fpout, "ObjOthers   %s~\n", social->obj_others );
         fprintf( fpout, "%s", "End\n\n" );
      }
   }
   fprintf( fpout, "%s", "#END\n" );
   FCLOSE( fpout );
}

void fread_social( FILE * fp )
{
   social_type *social = new social_type;

   for( ;; )
   {
      const char *word = ( feof( fp ) ? "End" : fread_word( fp ) );

      if( word[0] == '\0' )
      {
         bug( "%s: EOF encountered reading file!", __FUNCTION__ );
         word = "End";
      }

      switch ( UPPER( word[0] ) )
      {
         default:
            bug( "%s: no match: %s", __FUNCTION__, word );
            fread_to_eol( fp );
            break;

         case '*':
            fread_to_eol( fp );
            break;

         case 'C':
            KEY( "CharNoArg", social->char_no_arg, fread_string_nohash( fp ) );
            KEY( "CharFound", social->char_found, fread_string_nohash( fp ) );
            KEY( "CharAuto", social->char_auto, fread_string_nohash( fp ) );
            break;

         case 'E':
            if( !str_cmp( word, "End" ) )
            {
               if( !social->name )
               {
                  bug( "%s: Name not found", __FUNCTION__ );
                  deleteptr( social );
                  return;
               }
               if( !social->char_no_arg )
               {
                  bug( "%s: CharNoArg not found", __FUNCTION__ );
                  deleteptr( social );
                  return;
               }
               add_social( social );
               return;
            }
            break;

         case 'N':
            KEY( "Name", social->name, fread_string_nohash( fp ) );
            break;

         case 'O':
            KEY( "ObjOthers", social->obj_others, fread_string_nohash( fp ) );
            KEY( "ObjSelf", social->obj_self, fread_string_nohash( fp ) );
            KEY( "OthersNoArg", social->others_no_arg, fread_string_nohash( fp ) );
            KEY( "OthersFound", social->others_found, fread_string_nohash( fp ) );
            KEY( "OthersAuto", social->others_auto, fread_string_nohash( fp ) );
            break;

         case 'V':
            KEY( "VictFound", social->vict_found, fread_string_nohash( fp ) );
            break;
      }
   }
}

void load_socials( void )
{
   FILE *fp;

   if( ( fp = fopen( SOCIAL_FILE, "r" ) ) != NULL )
   {
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
            bug( "%s: # not found.", __FUNCTION__ );
            break;
         }

         word = fread_word( fp );
         if( !str_cmp( word, "SOCIAL" ) )
         {
            fread_social( fp );
            continue;
         }
         else if( !str_cmp( word, "END" ) )
            break;
         else
         {
            bug( "%s: bad section: %s", __FUNCTION__, word );
            continue;
         }
      }
      FCLOSE( fp );
   }
   else
   {
      bug( "%s: Cannot open socials.dat", __FUNCTION__ );
      exit( 1 );
   }
}

/*
 * Social editor/displayer/save/delete - Thoric
 */
CMDF( do_sedit )
{
   social_type *social;
   char arg1[MIL], arg2[MIL];

   ch->set_color( AT_SOCIAL );

   smash_tilde( argument );
   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );
   if( !arg1 || arg1[0] == '\0' )
   {
      ch->print( "Syntax: sedit <social> [field] [data]\r\n" );
      ch->print( "Syntax: sedit <social> create\r\n" );
      if( ch->level > LEVEL_GOD )
         ch->print( "Syntax: sedit <social> delete\r\n" );
      ch->print( "Syntax: sedit <save>\r\n" );
      if( ch->level > LEVEL_GREATER )
         ch->print( "Syntax: sedit <social> name <newname>\r\n" );
      ch->print( "\r\nField being one of:\r\n" );
      ch->print( "  cnoarg onoarg cfound ofound vfound cauto oauto objself objothers\r\n" );
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
      social->name = str_dup( arg1 );
      strdup_printf( &social->char_no_arg, "You %s.", arg1 );
      add_social( social );
      ch->print( "Social added.\r\n" );
      return;
   }

   if( !social )
   {
      ch->print( "Social not found.\r\n" );
      return;
   }

   if( !arg2 || arg2[0] == '\0' || !str_cmp( arg2, "show" ) )
   {
      ch->printf( "Social   : %s\r\n\r\nCNoArg   : %s\r\n", social->name, social->char_no_arg );
      ch->printf( "ONoArg   : %s\r\nCFound   : %s\r\nOFound   : %s\r\n",
                  social->others_no_arg ? social->others_no_arg : "(not set)",
                  social->char_found ? social->char_found : "(not set)",
                  social->others_found ? social->others_found : "(not set)" );
      ch->printf( "VFound   : %s\r\nCAuto    : %s\r\nOAuto    : %s\r\n",
                  social->vict_found ? social->vict_found : "(not set)",
                  social->char_auto ? social->char_auto : "(not set)",
                  social->others_auto ? social->others_auto : "(not set)" );
      ch->printf( "ObjSelf  : %s\r\nObjOthers: %s\r\n",
                  social->obj_self ? social->obj_self : "(not set)", social->obj_others ? social->obj_others : "(not set)" );
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
      if( !argument || argument[0] == '\0' || !str_cmp( argument, "clear" ) )
      {
         ch->print( "You cannot clear this field. It must have a message.\r\n" );
         return;
      }
      DISPOSE( social->char_no_arg );
      social->char_no_arg = str_dup( argument );
      ch->print( "Done.\r\n" );
      return;
   }
   if( !str_cmp( arg2, "onoarg" ) )
   {
      DISPOSE( social->others_no_arg );
      if( argument[0] != '\0' && str_cmp( argument, "clear" ) )
         social->others_no_arg = str_dup( argument );
      ch->print( "Done.\r\n" );
      return;
   }
   if( !str_cmp( arg2, "cfound" ) )
   {
      DISPOSE( social->char_found );
      if( argument[0] != '\0' && str_cmp( argument, "clear" ) )
         social->char_found = str_dup( argument );
      ch->print( "Done.\r\n" );
      return;
   }
   if( !str_cmp( arg2, "ofound" ) )
   {
      DISPOSE( social->others_found );
      if( argument[0] != '\0' && str_cmp( argument, "clear" ) )
         social->others_found = str_dup( argument );
      ch->print( "Done.\r\n" );
      return;
   }
   if( !str_cmp( arg2, "vfound" ) )
   {
      DISPOSE( social->vict_found );
      if( argument[0] != '\0' && str_cmp( argument, "clear" ) )
         social->vict_found = str_dup( argument );
      ch->print( "Done.\r\n" );
      return;
   }
   if( !str_cmp( arg2, "cauto" ) )
   {
      DISPOSE( social->char_auto );
      if( argument[0] != '\0' && str_cmp( argument, "clear" ) )
         social->char_auto = str_dup( argument );
      ch->print( "Done.\r\n" );
      return;
   }
   if( !str_cmp( arg2, "oauto" ) )
   {
      DISPOSE( social->others_auto );
      if( argument[0] != '\0' && str_cmp( argument, "clear" ) )
         social->others_auto = str_dup( argument );
      ch->print( "Done.\r\n" );
      return;
   }
   if( !str_cmp( arg2, "objself" ) )
   {
      DISPOSE( social->obj_self );
      if( argument[0] != '\0' && str_cmp( argument, "clear" ) )
         social->obj_self = str_dup( argument );
      ch->print( "Done.\r\n" );
      return;
   }
   if( !str_cmp( arg2, "objothers" ) )
   {
      DISPOSE( social->obj_others );
      if( argument[0] != '\0' && str_cmp( argument, "clear" ) )
         social->obj_others = str_dup( argument );
      ch->print( "Done.\r\n" );
      return;
   }
   if( ch->level > LEVEL_GREATER && !str_cmp( arg2, "name" ) )
   {
      bool relocate;
      social_type *checksocial;

      one_argument( argument, arg1 );
      if( !arg1 || arg1[0] == '\0' )
      {
         ch->print( "Cannot clear name field!\r\n" );
         return;
      }
      if( ( checksocial = find_social( arg1 ) ) != NULL )
      {
         ch->printf( "There is already a social named %s.\r\n", arg1 );
         return;
      }
      if( arg1[0] != social->name[0] )
      {
         unlink_social( social );
         relocate = true;
      }
      else
         relocate = false;
      DISPOSE( social->name );
      social->name = str_dup( arg1 );
      if( relocate )
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
   int col = 0, iHash = 0, count = 0;
   char letter = ' ';
   social_type *social;
   bool sorted = false, all = false;

   if( argument[0] != '\0' && !str_cmp( argument, "sorted" ) )
      sorted = true;
   if( argument[0] != '\0' && !str_cmp( argument, "all" ) )
      all = true;

   if( argument[0] != '\0' && !( sorted || all ) )
      ch->pagerf( "&[plain]Socials beginning with '%s':\r\n", argument );
   else if( argument[0] != '\0' && sorted )
      ch->pager( "&[plain]Socials -- Tabbed by Letter\r\n" );
   else
      ch->pager( "&[plain]Socials -- All\r\n" );

   for( iHash = 0; iHash < 27; ++iHash )
      for( social = social_index[iHash]; social; social = social->next )
      {
         if( ( argument[0] != '\0' && !( sorted || all ) ) && str_prefix( argument, social->name ) )
            continue;

         if( argument[0] != '\0' && sorted && letter != social->name[0] )
         {
            letter = social->name[0];
            if( col % 5 != 0 )
               ch->pager( "\r\n" );
            ch->pagerf( "&c[ &[plain]%c &c]\r\n", toupper( letter ) );
            col = 0;
         }
         ch->pagerf( "&[plain]%-15s ", social->name );
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

   return;
}
