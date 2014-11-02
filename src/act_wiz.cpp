/****************************************************************************
 *                   ^     +----- |  / ^     ^ |     | +-\                  *
 *                  / \    |      | /  |\   /| |     | |  \                 *
 *                 /   \   +---   |<   | \ / | |     | |  |                 *
 *                /-----\  |      | \  |  v  | |     | |  /                 *
 *               /       \ |      |  \ |     | +-----+ +-/                  *
 ****************************************************************************
 * AFKMud Copyright 1997-2015 by Roger Libiez (Samson),                     *
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
 *                        Wizard/god command module                         *
 ****************************************************************************/

#include <unistd.h>
#include <sys/stat.h>
#include <cstdarg>
#include <cerrno>
#include <sstream>
#include "mud.h"
#include "area.h"
#include "calendar.h"
#include "clans.h"
#include "deity.h"
#include "descriptor.h"
#include "event.h"
#include "finger.h"
#include "liquids.h"
#include "mobindex.h"
#include "mud_prog.h"
#include "objindex.h"
#include "overland.h"
#include "pfiles.h"
#include "raceclass.h"
#include "roomindex.h"
#include "sha256.h"
#include "variables.h"

/* External functions */
#ifdef IMC
void imc_check_wizperms( char_data * );
#endif
void set_chandler(  );
void unset_chandler(  );
void build_wizinfo(  );
void save_sysdata(  );
void write_race_file( int );
int calc_thac0( char_data *, char_data *, int );   /* For mstat */
void remove_member( const string &, const string & ); /* For do_destroy */
CMDF( do_oldwhere );
CMDF( do_help );
CMDF( do_mfind );
CMDF( do_ofind );
CMDF( do_bestow );
void rent_adjust_pfile( const string & );
bool in_arena( char_data * );
void check_stored_objects( char_data *, int );
void remove_from_auth( const string & );
void calc_season(  );
void ostat_plus( char_data *, obj_data *, bool );
void advance_level( char_data * );
void stop_hating( char_data * );
void stop_hunting( char_data * );
void stop_fearing( char_data * );
void check_clan_storeroom( char_data * );

/*
 * Global variables.
 */
#ifdef MULTIPORT
extern bool compilelock;
#endif
extern const char *liquid_types[];
extern bool bootlock;
extern int reboot_counter;

class_type *class_table[MAX_CLASS];
race_type *race_table[MAX_RACE];
char *title_table[MAX_CLASS][MAX_LEVEL + 1][SEX_MAX];
int MAX_PC_CLASS;
int MAX_PC_RACE;
time_t last_restore_all_time = 0;

/* Used to check and see if you should be using a command on a certain person - Samson 5-1-04 */
char_data *get_wizvictim( char_data * ch, const string & argument, bool nonpc )
{
   char_data *victim = NULL;

   if( !( victim = ch->get_char_world( argument ) ) )
   {
      ch->printf( "%s is not online.\r\n", argument.c_str(  ) );
      return NULL;
   }

   if( victim->isnpc(  ) && nonpc )
   {
      ch->print( "You cannot use this command on NPC's.\r\n" );
      return NULL;
   }

   if( victim->level > ch->level )
   {
      ch->printf( "You do not have sufficient access to affect %s.\r\n", victim->name );
      return NULL;
   }
   return victim;
}

/* Password resetting command, added by Samson 2-11-98
   Code courtesy of John Strange - Triad Mud */
/* Upgraded for OS independed SHA-256 encryption - Samson 1-7-06 */
CMDF( do_newpassword )
{
   string arg1;
   char_data *victim;
   const char *pwdnew;

   if( ch->isnpc(  ) )
      return;

   argument = one_argument( argument, arg1 );

   if( ( ch->pcdata->pwd != '\0' ) && ( arg1.empty(  ) || argument.empty(  ) ) )
   {
      ch->print( "Syntax: newpass <char> <newpassword>\r\n" );
      return;
   }

   if( !( victim = get_wizvictim( ch, arg1, true ) ) )
      return;

   /*
    * Immortal level check added to code by Samson 2-11-98 
    */
   if( ch->level < LEVEL_ADMIN )
   {
      if( victim->level >= LEVEL_IMMORTAL )
      {
         ch->print( "You can't change that person's password!\r\n" );
         return;
      }
   }

   if( argument.length(  ) < 5 )
   {
      ch->print( "New password must be at least five characters long.\r\n" );
      return;
   }

   if( argument[0] == '!' )
   {
      ch->print( "New password cannot begin with the '!' character.\r\n" );
      return;
   }

   pwdnew = sha256_crypt( argument.c_str(  ) ); /* SHA-256 Encryption */

   DISPOSE( victim->pcdata->pwd );
   victim->pcdata->pwd = str_dup( pwdnew );
   victim->save(  );
   ch->printf( "&R%s's password has been changed to: %s\r\n&w", victim->name, argument.c_str(  ) );
   victim->printf( "&R%s has changed your password to: %s\r\n&w", ch->name, argument.c_str(  ) );
}

/* Immortal highfive, added by Samson 2-15-98 Inspired by the version used on Crystal Shard */
/* Updated 2-10-02 to parse through act() instead of echo_to_all for the high immortal condition */
CMDF( do_highfive )
{
   char_data *victim;

   if( ch->isnpc(  ) )
      return;

   if( argument.empty(  ) )
   {
      act( AT_SOCIAL, "You jump in the air and highfive everyone in the room!", ch, NULL, NULL, TO_CHAR );
      act( AT_SOCIAL, "$n jumps in the air and highfives everyone in the room!", ch, NULL, ch, TO_ROOM );
      return;
   }

   if( !( victim = ch->get_char_room( argument ) ) )
   {
      ch->print( "Nobody by that name is here.\r\n" );
      return;
   }

   if( victim == ch )
   {
      act( AT_SOCIAL, "Sometimes you really amaze yourself!", ch, NULL, NULL, TO_CHAR );
      act( AT_SOCIAL, "$n gives $mself a highfive!", ch, NULL, ch, TO_ROOM );
      return;
   }

   if( victim->isnpc(  ) )
   {
      act( AT_SOCIAL, "You jump up and highfive $N!", ch, NULL, victim, TO_CHAR );
      act( AT_SOCIAL, "$n jumps up and gives you a highfive!", ch, NULL, victim, TO_VICT );
      act( AT_SOCIAL, "$n jumps up and gives $N a highfive!", ch, NULL, victim, TO_NOTVICT );
      return;
   }

   if( victim->level > LEVEL_TRUEIMM && ch->level > LEVEL_TRUEIMM )
   {
      list < char_data * >::iterator ich;

      for( ich = pclist.begin(  ); ich != pclist.end(  ); ++ich )
      {
         char_data *vch = ( *ich );

         if( vch == ch )
            act( AT_IMMORT, "The whole world rumbles as you highfive $N!", ch, NULL, victim, TO_CHAR );
         else if( vch == victim )
            act( AT_IMMORT, "The whole world rumbles as $n highfives you!", ch, NULL, victim, TO_VICT );
         else
            act( AT_IMMORT, "The whole world rumbles as $n and $N highfive!", ch, vch, victim, TO_THIRD );
      }
   }
   else
   {
      act( AT_SOCIAL, "You jump up and highfive $N!", ch, NULL, victim, TO_CHAR );
      act( AT_SOCIAL, "$n jumps up and gives you a highfive!", ch, NULL, victim, TO_VICT );
      act( AT_SOCIAL, "$n jumps up and gives $N a highfive!", ch, NULL, victim, TO_NOTVICT );
   }
}

CMDF( do_bamfin )
{
   if( !ch->isnpc(  ) )
   {
      smash_tilde( argument );
      DISPOSE( ch->pcdata->bamfin );
      ch->pcdata->bamfin = str_dup( argument.c_str(  ) );
      ch->print( "&YBamfin set.\r\n" );
   }
}

CMDF( do_bamfout )
{
   if( !ch->isnpc(  ) )
   {
      smash_tilde( argument );
      DISPOSE( ch->pcdata->bamfout );
      ch->pcdata->bamfout = str_dup( argument.c_str(  ) );
      ch->print( "&YBamfout set.\r\n" );
   }
}

CMDF( do_rank )
{
   if( ch->isnpc(  ) )
      return;

   if( argument.empty(  ) )
   {
      ch->print( "&[immortal]Usage:  rank <string>.\r\n" );
      ch->print( "   or:  rank none.\r\n" );
      return;
   }
   smash_tilde( argument );
   STRFREE( ch->pcdata->rank );
   if( str_cmp( argument, "none" ) )
      ch->pcdata->rank = STRALLOC( argument.c_str(  ) );
   ch->printf( "&[immortal]Rank set to: %s\r\n", ch->pcdata->rank ? ch->pcdata->rank : "None" );
}

CMDF( do_retire )
{
   char_data *victim;

   ch->set_color( AT_IMMORT );

   if( argument.empty(  ) )
   {
      ch->print( "Retire whom?\r\n" );
      return;
   }

   if( !( victim = get_wizvictim( ch, argument, true ) ) )
      return;

   if( victim->level < LEVEL_SAVIOR )
   {
      ch->printf( "The minimum level for retirement is %d.\r\n", LEVEL_SAVIOR );
      return;
   }
   if( victim->has_pcflag( PCFLAG_RETIRED ) )
   {
      victim->unset_pcflag( PCFLAG_RETIRED );
      ch->printf( "%s returns from retirement.\r\n", victim->name );
      victim->printf( "%s brings you back from retirement.\r\n", ch->name );
   }
   else
   {
      victim->set_pcflag( PCFLAG_RETIRED );
      ch->printf( "%s is now a retired immortal.\r\n", victim->name );
      victim->printf( "Courtesy of %s, you are now a retired immortal.\r\n", ch->name );
   }
}

CMDF( do_delay )
{
   char_data *victim;
   string arg;
   int delay;

   ch->set_color( AT_IMMORT );

   argument = one_argument( argument, arg );
   if( arg.empty(  ) || argument.empty(  ) )
   {
      ch->print( "Syntax: delay <victim> <# of rounds>\r\n" );
      return;
   }

   if( !( victim = get_wizvictim( ch, argument, true ) ) )
      return;

   if( !str_cmp( argument, "none" ) )
   {
      ch->print( "All character delay removed.\r\n" );
      victim->wait = 0;
      return;
   }
   delay = atoi( argument.c_str(  ) );
   if( delay < 1 )
   {
      ch->print( "Pointless. Try a positive number.\r\n" );
      return;
   }
   if( delay > 999 )
      ch->print( "You cruel bastard. Just kill them.\r\n" );

   victim->WAIT_STATE( delay * sysdata->pulseviolence );
   ch->printf( "You've delayed %s for %d rounds.\r\n", victim->name, delay );
}

CMDF( do_deny )
{
   char_data *victim;

   ch->set_color( AT_IMMORT );

   if( argument.empty(  ) )
   {
      ch->print( "Deny whom?\r\n" );
      return;
   }

   if( !( victim = get_wizvictim( ch, argument, true ) ) )
      return;

   victim->set_pcflag( PCFLAG_DENY );
   victim->set_color( AT_IMMORT );
   victim->print( "You are denied access!\r\n" );
   ch->printf( "You have denied access to %s.\r\n", victim->name );
   if( victim->fighting )
      victim->stop_fighting( true );   /* Blodkai, 97 */
   victim->save(  );
   if( victim->desc )
      close_socket( victim->desc, false );
}

CMDF( do_disconnect )
{
   int desc;
   char_data *victim;

   ch->set_color( AT_IMMORT );

   if( argument.empty(  ) )
   {
      ch->print( "You must specify a player or descriptor to disconnect.\r\n" );
      return;
   }

   if( is_number( argument ) )
      desc = atoi( argument.c_str(  ) );
   else if( ( victim = ch->get_char_world( argument ) ) )
   {
      if( victim->desc == NULL )
      {
         ch->printf( "%s does not have a desriptor.\r\n", victim->name );
         return;
      }
      else
         desc = victim->desc->descriptor; /* Seems weird... but this seems faster? --X */
   }
   else
   {
      ch->printf( "Disconnect: '%s' was not found!\r\n", argument.c_str(  ) );
      return;
   }

   list < descriptor_data * >::iterator ds;
   for( ds = dlist.begin(  ); ds != dlist.end(  ); ++ds )
   {
      descriptor_data *d = *ds;

      if( d->descriptor == desc )
      {
         victim = d->original ? d->original : d->character;

         if( victim && victim->get_trust(  ) >= ch->get_trust(  ) )
         {
            ch->print( "You cannot disconnect that person.\r\n" );
            log_printf( "%s tried to disconnect %s but failed.", ch->name, victim->name ? victim->name : "Someone" );
            return;
         }
         if( victim )
            log_printf( "%s will be disconnected %s", ch->name, victim->name ? victim->name : "Someone" );
         else
            log_printf( "%s will be disconnected desc #%d", ch->name, desc );
         d->disconnect = true;
         ch->print( "Ok.\r\n" );
         return;
      }
   }
   bug( "%s: desc '%d' not found!", __FUNCTION__, desc );
   ch->print( "Descriptor not found!\r\n" );
}

void echo_all_printf( short tar, const char *str, ... )
{
   va_list arg;
   char argument[MSL];

   if( !str || str[0] == '\0' )
      return;

   va_start( arg, str );
   vsnprintf( argument, MSL, str, arg );
   va_end( arg );

   echo_to_all( argument, tar );
}

void echo_to_all( const string & argument, short tar )
{
   if( argument.empty(  ) )
      return;

   list < descriptor_data * >::iterator ds;
   for( ds = dlist.begin(  ); ds != dlist.end(  ); ++ds )
   {
      descriptor_data *d = *ds;

      /*
       * Added showing echoes to players who are editing, so they won't
       * * miss out on important info like upcoming reboots. --Narn 
       * CON_PLAYING = 0, and anything else greater than 0 is a player who is actually in the game.
       * * They will need to see anything deemed important enough to use echo_to_all, such as
       * * power failures etc.
       */
      if( d->connected >= CON_PLAYING )
      {
         /*
          * This one is kinda useless except for switched.. 
          */
         if( tar == ECHOTAR_PC && d->character->isnpc(  ) )
            continue;
         else if( tar == ECHOTAR_IMM && !d->character->is_immortal(  ) )
            continue;

         d->character->printf( "%s\r\n", argument.c_str(  ) );
      }
   }
}

CMDF( do_echo )
{
   string arg;
   int target;

   ch->set_color( AT_IMMORT );

   if( ch->has_pcflag( PCFLAG_NO_EMOTE ) )
   {
      ch->print( "You can't do that right now.\r\n" );
      return;
   }

   if( argument.empty(  ) )
   {
      ch->print( "Echo what?\r\n" );
      return;
   }

   string parg = argument;
   argument = one_argument( argument, arg );
   if( !str_cmp( arg, "PC" ) || !str_cmp( arg, "player" ) )
      target = ECHOTAR_PC;
   else if( !str_cmp( arg, "imm" ) )
      target = ECHOTAR_IMM;
   else
   {
      target = ECHOTAR_ALL;
      argument = parg;
   }
   echo_to_all( argument, target );
}

/* Stupid little function for the Voice of God - Samson 4-30-99 */
CMDF( do_voice )
{
   echo_all_printf( ECHOTAR_ALL, "&[immortal]The Voice of God says: %s", argument.c_str(  ) );
}

/* This function shared by do_transfer and do_mptransfer
 *
 * Immortals bypass most restrictions on where to transfer victims.
 * NPCs cannot transfer victims who are:
 * 1. Outside of the level range for the target room's area.
 * 2. Being sent to private rooms.
 * 3. The victim is in hell.
 */
void transfer_char( char_data * ch, char_data * victim, room_index * location )
{
   if( !victim->in_room )
   {
      bug( "%s: victim in NULL room: %s", __FUNCTION__, victim->name );
      return;
   }

   if( ch->isnpc(  ) && location->is_private(  ) )
   {
      progbug( "Mptransfer - Private room", ch );
      return;
   }

   if( !ch->can_see( victim, true ) )
      return;

   if( ch->isnpc(  ) && !victim->isnpc(  ) && victim->pcdata->release_date != 0 )
   {
      progbugf( ch, "Mptransfer - helled character (%s)", victim->name );
      return;
   }

   /*
    * If victim not in area's level range, do not transfer 
    */
   if( ch->isnpc(  ) && !victim->in_hard_range( location->area ) && !location->flags.test( ROOM_PROTOTYPE ) )
      return;

   if( victim->fighting )
      victim->stop_fighting( true );

   if( !ch->isnpc(  ) )
      act( AT_MAGIC, "A swirling vortex arrives to pick up $n!", victim, NULL, NULL, TO_ROOM );

   leave_map( victim, ch, location );

   act( AT_MAGIC, "A swirling vortex arrives, carrying $n!", victim, NULL, NULL, TO_ROOM );

   if( !ch->isnpc(  ) )
   {
      if( ch != victim )
         act( AT_IMMORT, "$n has sent a swirling vortex to transport you.", ch, NULL, victim, TO_VICT );
      ch->print( "Ok.\r\n" );

      if( !victim->is_immortal(  ) && !victim->isnpc(  ) && !victim->in_hard_range( location->area ) )
         ch->print( "Warning: the player's level is not within the area's level range.\r\n" );
   }
}

CMDF( do_transfer )
{
   string arg1;
   room_index *location;
   char_data *victim;

   argument = one_argument( argument, arg1 );
   if( arg1.empty(  ) )
   {
      ch->print( "Transfer whom?\r\n" );
      return;
   }

   ch->set_color( AT_IMMORT );

   /*
    * Thanks to Grodyn for the optional location parameter.
    */
   if( argument.empty(  ) )
      location = ch->in_room;
   else
   {
      if( !( location = ch->find_location( argument ) ) )
      {
         ch->print( "That location does not exist.\r\n" );
         return;
      }
   }

   if( !str_cmp( arg1, "all" ) )
   {
      list < descriptor_data * >::iterator ds;

      for( ds = dlist.begin(  ); ds != dlist.end(  ); ++ds )
      {
         descriptor_data *d = *ds;

         if( d->connected == CON_PLAYING && d->character != ch && d->character->in_room && d->newstate != 2 && ch->can_see( d->character, true ) )
            transfer_char( ch, d->character, location );
      }
      return;
   }

   if( location != ch->in_room )
   {
      ch->print( "You can only transfer to your occupied room.\r\n" );
      ch->print( "If you need to transfer someone to a remote location, use the at command there to transfer them.\r\n" );
      return;
   }

   if( !( victim = get_wizvictim( ch, arg1, false ) ) )
      return;

   if( victim == ch )
   {
      ch->printf( "Are you feeling alright today %s?\r\n", ch->name );
      return;
   }
   transfer_char( ch, victim, location );
}

void location_action( char_data * ch, const string & argument, room_index * location, short map, short x, short y )
{
   if( !location )
   {
      bug( "%s: NULL room!", __FUNCTION__ );
      return;
   }

   if( !ch->in_room )
   {
      bug( "%s: NULL ch->in_room!", __FUNCTION__ );
      return;
   }

   if( location->is_private(  ) )
   {
      if( ch->level < sysdata->level_override_private )
      {
         ch->print( "That room is private right now.\r\n" );
         return;
      }
      else
         ch->print( "Overriding private flag!\r\n" );
   }

   if( location->flags.test( ROOM_ISOLATED ) && ch->level < LEVEL_SUPREME )
   {
      ch->print( "Go away! That room has been sealed for privacy!\r\n" );
      return;
   }

   short origmap = ch->cmap;
   short origx = ch->mx;
   short origy = ch->my;

   /*
    * Bunch of checks to make sure the "ator" is temporarily on the same grid as
    * * the "atee" - Samson
    */
   if( location->flags.test( ROOM_MAP ) && !ch->has_pcflag( PCFLAG_ONMAP ) )
   {
      ch->set_pcflag( PCFLAG_ONMAP );
      ch->cmap = map;
      ch->mx = x;
      ch->my = y;
   }
   else if( location->flags.test( ROOM_MAP ) && ch->has_pcflag( PCFLAG_ONMAP ) )
   {
      ch->cmap = map;
      ch->mx = x;
      ch->my = y;
   }
   else if( !location->flags.test( ROOM_MAP ) && ch->has_pcflag( PCFLAG_ONMAP ) )
   {
      ch->unset_pcflag( PCFLAG_ONMAP );
      ch->cmap = -1;
      ch->mx = -1;
      ch->my = -1;
   }

   ch->set_color( AT_PLAIN );
   room_index *original = ch->in_room;
   ch->from_room(  );
   if( !ch->to_room( location ) )
      log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
   interpret( ch, argument );

   if( ch->has_pcflag( PCFLAG_ONMAP ) && !original->flags.test( ROOM_MAP ) )
      ch->unset_pcflag( PCFLAG_ONMAP );
   else if( !ch->has_pcflag( PCFLAG_ONMAP ) && original->flags.test( ROOM_MAP ) )
      ch->set_pcflag( PCFLAG_ONMAP );

   ch->cmap = origmap;
   ch->mx = origx;
   ch->my = origy;

   /*
    * See if 'ch' still exists before continuing!
    * Handles 'at XXXX quit' case.
    */
   list < char_data * >::iterator ich;
   for( ich = charlist.begin(  ); ich != charlist.end(  ); ++ich )
   {
      char_data *wch = ( *ich );

      if( wch == ch && !ch->char_died(  ) )
      {
         ch->from_room(  );
         if( !ch->to_room( original ) )
            log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
         break;
      }
   }
}

/*  Added atmob and atobj to reduce lag associated with at
 *  --Shaddai
 */
void atmob( char_data * ch, char_data * wch, const string & argument )
{
   if( is_ignoring( wch, ch ) )
   {
      ch->print( "No such location.\r\n" );
      return;
   }
   location_action( ch, argument, wch->in_room, wch->cmap, wch->mx, wch->my );
}

void atobj( char_data * ch, obj_data * obj, const string & argument )
{
   location_action( ch, argument, obj->in_room, obj->cmap, obj->mx, obj->my );
}

/* Smaug 1.02a at command restored by Samson 8-14-98 */
CMDF( do_at )
{
   string arg;
   room_index *location;
   char_data *wch;
   obj_data *obj;

   argument = one_argument( argument, arg );

   if( arg.empty(  ) || argument.empty(  ) )
   {
      ch->print( "At where what?\r\n" );
      return;
   }

   if( !is_number( arg ) )
   {
      if( ( wch = ch->get_char_world( arg ) ) != NULL && wch->in_room != NULL )
      {
         atmob( ch, wch, argument );
         return;
      }

      if( ( obj = ch->get_obj_world( arg ) ) != NULL && obj->in_room != NULL )
      {
         atobj( ch, obj, argument );
         return;
      }
      ch->print( "No such mob or object.\r\n" );
      return;
   }

   if( !( location = ch->find_location( arg ) ) )
   {
      ch->print( "No such location.\r\n" );
      return;
   }
   location_action( ch, argument, location, -1, -1, -1 );
}

CMDF( do_rat )
{
   string arg1, arg2;
   room_index *location, *original;
   int Start, End, vnum;

   ch->set_color( AT_IMMORT );

   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );
   if( arg1.empty(  ) || arg2.empty(  ) || argument.empty(  ) )
   {
      ch->print( "Syntax: rat <start> <end> <command>\r\n" );
      return;
   }

   Start = atoi( arg1.c_str(  ) );
   End = atoi( arg2.c_str(  ) );
   if( Start < 1 || End < Start || Start > End || Start == End || End > sysdata->maxvnum )
   {
      ch->print( "Invalid range.\r\n" );
      return;
   }
   if( !str_cmp( argument, "quit" ) )
   {
      ch->print( "I don't think so!\r\n" );
      return;
   }

   original = ch->in_room;
   for( vnum = Start; vnum <= End; ++vnum )
   {
      if( !( location = get_room_index( vnum ) ) )
         continue;

      ch->from_room(  );
      if( !ch->to_room( location ) )
         log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
      interpret( ch, argument );
   }

   ch->from_room(  );
   if( !ch->to_room( original ) )
      log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
   ch->print( "Done.\r\n" );
}

/* Rewritten by Whir to be viewer friendly - 8/26/98 */
CMDF( do_rstat )
{
   room_index *location;
   const char *dir_text[] = { "n", "e", "s", "w", "u", "d", "ne", "nw", "se", "sw", "?" };

   if( !str_cmp( argument, "exits" ) )
   {
      list < exit_data * >::iterator ex;
      location = ch->in_room;

      ch->printf( "&wExits for room '&G%s.&w' vnum &G%d&w\r\n", location->name, location->vnum );

      int cnt = 0;
      for( ex = location->exits.begin(  ); ex != location->exits.end(  ); ++ex )
      {
         exit_data *pexit = *ex;

         ch->printf( "%2d) &G%2s&w to &G%-5d&w.  Key: &G%d  &wKeywords: '&G%s&w'  &wFlags: &G%s&w.\r\n",
                     ++cnt, dir_text[pexit->vdir], pexit->to_room ? pexit->to_room->vnum : 0, pexit->key, pexit->keyword, bitset_string( pexit->flags, ex_flags ) );

         ch->printf( "Description: &G%s&wExit links back to vnum: &G%d  &wExit's RoomVnum: &G%d&w\r\n\r\n",
                     ( pexit->exitdesc && pexit->exitdesc[0] != '\0' ) ? pexit->exitdesc : "(NONE)\r\n", pexit->rexit ? pexit->rexit->vnum : 0, pexit->rvnum );
      }
      return;
   }

   location = ( argument.empty(  ) )? ch->in_room : ch->find_location( argument );

   if( !location )
   {
      ch->print( "No such location.\r\n" );
      return;
   }

   if( ch->in_room != location && location->is_private(  ) )
   {
      if( ch->get_trust(  ) < LEVEL_GREATER )
      {
         ch->print( "That room is private right now.\r\n" );
         return;
      }
      else
         ch->print( "Overriding private flag!\r\n" );
   }

   if( location->flags.test( ROOM_ISOLATED ) && ch->level < LEVEL_SUPREME )
   {
      ch->print( "Go away! That room has been sealed for privacy!\r\n" );
      return;
   }

   ch->printf( "&wName: &G%s\r\n&wArea: &G%s  ", location->name, location->area ? location->area->name : "None????" );
   ch->printf( "&wFilename: &G%s\r\n", location->area ? location->area->filename : "None????" );
   ch->printf( "&wVnum: &G%d&w  Light: &G%d&w  TeleDelay: &G%d&w  TeleVnum: &G%d&w  Tunnel: &G%d&w  Max Weight: &G%d&w\r\n",
               location->vnum, location->light, location->tele_delay, location->tele_vnum, location->tunnel, location->max_weight );

   ch->printf( "Room flags: &G%s&w\r\n", bitset_string( location->flags, r_flags ) );
   ch->printf( "Sector type: &G%s&w\r\n", sect_types[location->sector_type] );

   ch->printf( "Description:&w\r\n%s&w", location->roomdesc );

   /*
    * NiteDesc rstat added by Dracones 
    */
   if( location->nitedesc && location->nitedesc[0] != '\0' )
      ch->printf( "NiteDesc:&w\r\n%s&w", location->nitedesc );

   if( !location->extradesc.empty(  ) )
   {
      list < extra_descr_data * >::iterator ex;

      ch->print( "Extra description keywords:\r\n&G" );
      for( ex = location->extradesc.begin(  ); ex != location->extradesc.end(  ); ++ex )
      {
         extra_descr_data *ed = *ex;

         ch->print( ed->keyword );
         ch->print( " " );
      }
      ch->print( "&w'.\r\n\r\n" );
   }

   ch->print( "Permanent affects: " );
   if( ch->in_room->permaffects.empty(  ) )
      ch->print( "&GNone&w\r\n" );
   else
   {
      ch->print( "\r\n" );

      list < affect_data * >::iterator paf;
      for( paf = location->permaffects.begin(  ); paf != location->permaffects.end(  ); ++paf )
      {
         affect_data *af = *paf;

         ch->showaffect( af );
      }
   }

   if( ch->in_room->progtypes.none(  ) )
      ch->print( "Roomprogs: &GNone&w\r\n" );
   else
   {
      list < mud_prog_data * >::iterator mprg;

      ch->print( "Roomprogs: &G" );

      for( mprg = ch->in_room->mudprogs.begin(  ); mprg != ch->in_room->mudprogs.end(  ); ++mprg )
      {
         mud_prog_data *mprog = *mprg;

         ch->printf( "%s ", mprog_type_to_name( mprog->type ).c_str(  ) );
      }
      ch->print( "&w\r\n" );
   }

   list < char_data * >::iterator ich;
   ch->print( "Characters/Mobiles in the room:\r\n" );
   for( ich = location->people.begin(  ); ich != location->people.end(  ); ++ich )
   {
      char_data *rch = *ich;

      if( ch->can_see( rch, false ) )
         ch->printf( "(&G%d&w)&G%s&w\r\n", ( rch->isnpc(  )? rch->pIndexData->vnum : 0 ), rch->name );
   }

   list < obj_data * >::iterator iobj;
   ch->print( "\r\nObjects in the room:\r\n" );
   for( iobj = location->objects.begin(  ); iobj != location->objects.end(  ); ++iobj )
   {
      obj_data *obj = *iobj;

      if( ch->can_see_obj( obj, false ) )
         ch->printf( "(&G%d&w)&G%s&w\r\n", obj->pIndexData->vnum, obj->name );
   }
   ch->print( "\r\n" );

   if( !location->exits.empty(  ) )
      ch->print( "------------------- EXITS -------------------\r\n" );

   int cnt = 0;
   list < exit_data * >::iterator ex;
   for( ex = location->exits.begin(  ); ex != location->exits.end(  ); ++ex )
   {
      exit_data *pexit = *ex;

      ch->printf( "%2d) &G%-2s &wto &G%-5d  &wKey: &G%-5d  &wKeywords: &G%s&w  Flags: &G%s&w.\r\n",
                  ++cnt, dir_text[pexit->vdir], pexit->to_room ? pexit->to_room->vnum : 0,
                  pexit->key, ( pexit->keyword && pexit->keyword[0] != '\0' ) ? pexit->keyword : "(none)", bitset_string( pexit->flags, ex_flags ) );

      if( IS_EXIT_FLAG( pexit, EX_OVERLAND ) )
         ch->printf( "    &wExit coordinates: &G%d&wX &G%d&wY\r\n", pexit->mx, pexit->my );
   }
}

/* Rewritten by Whir to be viewer friendly - 8/26/98 */
CMDF( do_ostat )
{
   const char *suf;
   obj_data *obj;

   if( !( obj = ch->get_obj_world( argument ) ) )
   {
      ch->print( "That object doesn't exist!\r\n" );
      return;
   }

   short day = obj->day + 1;

   if( day > 4 && day < 20 )
      suf = "th";
   else if( day % 10 == 1 )
      suf = "st";
   else if( day % 10 == 2 )
      suf = "nd";
   else if( day % 10 == 3 )
      suf = "rd";
   else
      suf = "th";

   ch->printf( "&w|Name   : &G%s&w\r\n", obj->name );

   ch->printf( "|Short  : &G%s&w\r\n|Long   : &G%s&w\r\n", obj->short_descr, ( obj->objdesc && obj->objdesc[0] != '\0' ) ? obj->objdesc : "" );

   if( obj->action_desc && obj->action_desc[0] != '\0' )
      ch->printf( "|Action : &G%s&w\r\n", obj->action_desc );

   ch->printf( "|Area   : %s\r\n", obj->pIndexData->area ? obj->pIndexData->area->name : "(NONE)" );

   ch->printf( "|Vnum   : &G%5d&w |Type  :     &G%9s&w |Count     : &G%3.3d&w |Gcount: &G%3.3d&w\r\n",
               obj->pIndexData->vnum, obj->item_type_name(  ).c_str(  ), obj->pIndexData->count, obj->count );

   ch->printf( "|Number : &G%2.2d&w/&G%2.2d&w |Weight:     &G%4.4d&w/&G%4.4d&w |Wear_loc  : &G%3.2d&w |Layers: &G%d&w\r\n",
               1, obj->get_number(  ), obj->weight, obj->get_weight(  ), obj->wear_loc, obj->pIndexData->layers );

   ch->printf( "|Cost   : &G%5d&w |Ego*  : &G%6d&w |Ego   : &G%6d&w |Timer: &G%d&w\r\n", obj->cost, obj->pIndexData->ego, obj->ego, obj->timer );

   ch->printf( "|In room: &G%5d&w |In obj: &G%s&w |Level :  &G%5d&w |Limit: &G%5d&w\r\n",
               obj->in_room == NULL ? 0 : obj->in_room->vnum, obj->in_obj == NULL ? "(NONE)" : obj->in_obj->short_descr, obj->level, obj->pIndexData->limit );

   ch->printf( "|On map       : &G%s&w\r\n", obj->extra_flags.test( ITEM_ONMAP ) ? map_names[obj->cmap] : "(NONE)" );

   ch->printf( "|Object Coords: &G%d %d&w\r\n", obj->mx, obj->my );
   ch->printf( "|Wear flags   : &G%s&w\r\n", bitset_string( obj->wear_flags, w_flags ) );
   ch->printf( "|Extra flags  : &G%s&w\r\n", bitset_string( obj->extra_flags, o_flags ) );
   ch->printf( "|Carried by   : &G%s&w\r\n", obj->carried_by == NULL ? "(NONE)" : obj->carried_by->name );
   ch->printf( "|Prizeowner   : &G%s&w\r\n", obj->owner == NULL ? "(NONE)" : obj->owner );
   ch->printf( "|Seller       : &G%s&w\r\n", obj->seller == NULL ? "(NONE)" : obj->seller );
   ch->printf( "|Buyer        : &G%s&w\r\n", obj->buyer == NULL ? "(NONE)" : obj->buyer );
   ch->printf( "|Current bid  : &G%d&w\r\n", obj->bid );

   if( obj->year == 0 )
      ch->print( "|Scheduled donation date: &G(NONE)&w\r\n" );
   else
      ch->printf( "|Scheduled donation date: &G %d%s day in the Month of %s, in the year %d.&w", day, suf, month_name[obj->month], obj->year );

   ch->printf( "|Index Values : &G%2d %2d %2d %2d %2d %2d %2d %2d %2d %2d %2d&w\r\n",
               obj->pIndexData->value[0], obj->pIndexData->value[1], obj->pIndexData->value[2], obj->pIndexData->value[3],
               obj->pIndexData->value[4], obj->pIndexData->value[5], obj->pIndexData->value[6], obj->pIndexData->value[7],
               obj->pIndexData->value[8], obj->pIndexData->value[9], obj->pIndexData->value[10] );

   ch->printf( "|Object Values: &G%2d %2d %2d %2d %2d %2d %2d %2d %2d %2d %2d&w\r\n",
               obj->value[0], obj->value[1], obj->value[2], obj->value[3],
               obj->value[4], obj->value[5], obj->value[6], obj->value[7], obj->value[8], obj->value[9], obj->value[10] );

   if( !obj->pIndexData->extradesc.empty(  ) )
   {
      list < extra_descr_data * >::iterator ex;

      ch->print( "|Primary description keywords:   '" );
      for( ex = obj->pIndexData->extradesc.begin(  ); ex != obj->pIndexData->extradesc.end(  ); ++ex )
      {
         extra_descr_data *ed = *ex;

         ch->print( ed->keyword );
         ch->print( " " );
      }
      ch->print( "'.\r\n" );
   }

   if( !obj->extradesc.empty(  ) )
   {
      list < extra_descr_data * >::iterator ex;

      ch->print( "|Secondary description keywords: '" );
      for( ex = obj->extradesc.begin(  ); ex != obj->extradesc.end(  ); ++ex )
      {
         extra_descr_data *ed = *ex;

         ch->print( ed->keyword );
         ch->print( " " );
      }
      ch->print( "'.\r\n" );
   }

   if( obj->pIndexData->progtypes.none(  ) )
      ch->print( "|Objprogs     : &GNone&w\r\n" );
   else
   {
      list < mud_prog_data * >::iterator mprg;

      ch->print( "|Objprogs     : &G" );

      for( mprg = obj->pIndexData->mudprogs.begin(  ); mprg != obj->pIndexData->mudprogs.end(  ); ++mprg )
      {
         mud_prog_data *mprog = *mprg;

         ch->printf( "%s ", mprog_type_to_name( mprog->type ).c_str(  ) );
      }
      ch->print( "&w\r\n" );
   }

   /*
    * Rather useful and cool function provided by Druid to decipher those values 
    */
   ch->print( "\r\n" );
   ostat_plus( ch, obj, false );
   ch->print( "\r\n" );

   affect_data *af;
   list < affect_data * >::iterator paf;
   for( paf = obj->affects.begin(  ); paf != obj->affects.end(  ); ++paf )
   {
      af = *paf;
      ch->showaffect( af );
   }

   for( paf = obj->pIndexData->affects.begin(  ); paf != obj->pIndexData->affects.end(  ); ++paf )
   {
      af = *paf;
      ch->showaffect( af );
   }
}

CMDF( do_moblog )
{
   ch->set_color( AT_LOG );
   ch->print( "\r\n[Date_|_Time]  Current moblog:\r\n" );
   show_file( ch, MOBLOG_FILE );
}

CMDF( do_vstat )
{
   variable_data *vd;
   list < variable_data * >::iterator ivd;
   char_data *victim;

   if( argument.empty(  ) )
   {
      ch->print( "Vstat whom?\r\n" );
      return;
   }

   if( !( victim = get_wizvictim( ch, argument, false ) ) )
      return;

   if( victim->variables.size(  ) < 1 )
   {
      ch->print( "They have no variables currently assigned to them.\r\n" );
      return;
   }

   ch->pagerf( "\r\n&cName: &C%-20s &cRoom : &w%-10d", victim->name, victim->in_room == NULL ? 0 : victim->in_room->vnum );
   ch->pagerf( "\r\n&cVariables:\r\n" );

   /*
    * Variables:
    * Vnum:           Tag:                 Type:     Timer:
    * Flags:
    * Data:
    */
   for( ivd = victim->variables.begin(  ); ivd != victim->variables.end(  ); ++ivd )
   {
      vd = *ivd;

      ch->pagerf( "  &cVnum: &W%-10d &cTag: &W%-15s &cTimer: &W%d\r\n", vd->vnum, vd->tag.c_str(  ), vd->timer );
      ch->pager( "  &cType: " );

      switch( vd->type )
      {
         default:
            ch->pager( "&RINVALID!!!" );
            break;

         case vtSTR:
            if( !vd->varstring.empty() )
               ch->pagerf( "&CString     &cData: &W%s", vd->varstring.c_str() );
            break;

         case vtINT:
            if( vd->vardata > 0 )
               ch->pagerf( "&CInteger    &cData: &W%ld", vd->vardata );
            break;

         case vtXBIT:
            if( vd->varflags.any() )
               ch->pagerf( "&CBitflags: &w[&W%s&w]", vd->varflags.to_string().c_str() );
            break;
      }
      ch->pager( "\r\n\r\n" );
   }
}

/* do_mstat rewritten by Whir to be view-friendly 8/18/98 */
CMDF( do_mstat )
{
   char_data *victim;
   skill_type *skill;
   int iLang = 0;
   char lbuf[256];

   if( !( victim = ch->get_char_world( argument ) ) )
   {
      ch->print( "They aren't here.\r\n" );
      return;
   }

   if( !victim->isnpc(  ) )
   {
      ch->printf( "&w|Name  : &G%10s &w|Clan  : &G%10s &w|PKill : &G%10s &w|Room  : &G%d&w\r\n",
                  victim->name, ( victim->pcdata->clan == NULL ) ? "(NONE)" : victim->pcdata->clan->name.c_str(  ),
                  victim->has_pcflag( PCFLAG_DEADLY ) ? "Yes" : "No", victim->in_room->vnum );

      ch->printf( "|Level : &G%10d &w|Trust : &G%10d &w|Sex   : &G%10s &w|Gold  : &G%d&w\r\n", victim->level, victim->trust, npc_sex[victim->sex], victim->gold );

      ch->printf( "|STR   : &G%10d &w|HPs   : &G%10d &w|MaxHPs: &G%10d &w|Style : &G%s&w\r\n", victim->get_curr_str(  ), victim->hit, victim->max_hit, styles[victim->style] );

      ch->printf( "|INT   : &G%10d &w|Mana  : &G%10d &w|MaxMan: &G%10d &w|Pos   : &G%s&w\r\n",
                  victim->get_curr_int(  ), victim->mana, victim->max_mana, npc_position[victim->position] );

      ch->printf( "|WIS   : &G%10d &w|Move  : &G%10d &w|MaxMov: &G%10d &w|Wimpy : &G%d&w\r\n", victim->get_curr_wis(  ), victim->move, victim->max_move, victim->wimpy );

      ch->printf( "|DEX   : &G%10d &w|AC    : &G%10d &w|Align : &G%10d &w|Favor : &G%d&w\r\n",
                  victim->get_curr_dex(  ), victim->GET_AC(  ), victim->alignment, victim->pcdata->favor );

      ch->printf( "|CON   : &G%10d &w|+Hit  : &G%10d &w|+Dam  : &G%10d &w|Pracs : &G%d&w\r\n",
                  victim->get_curr_con(  ), victim->GET_HITROLL(  ), victim->GET_DAMROLL(  ), victim->pcdata->practice );

      ch->printf( "|CHA   : &G%10d &w|BseAge: &G%10d &w|Agemod: &G%10d &w|\r\n", victim->get_curr_cha(  ), victim->pcdata->age, victim->pcdata->age_bonus );

      if( victim->pcdata->condition[COND_THIRST] == -1 )
         mudstrlcpy( lbuf, "|Thirst: &G    Immune &w", 256 );
      else
         snprintf( lbuf, 256, "|Thirst: &G%10d &w", victim->pcdata->condition[COND_THIRST] );
      if( victim->pcdata->condition[COND_FULL] == -1 )
         mudstrlcat( lbuf, "|Hunger: &G    Immune &w", 256 );
      else
         snprintf( lbuf + strlen( lbuf ), 256 - strlen( lbuf ), "|Hunger: &G%10d &w", victim->pcdata->condition[COND_FULL] );
      ch->printf( "|LCK   : &G%10d &w%s|Drunk : &G%d &w\r\n", victim->get_curr_lck(  ), lbuf, victim->pcdata->condition[COND_DRUNK] );

      ch->printf( "|Class :&G%11s &w|Mental: &G%10d &w|#Attks: &G%10f&w\r\n", capitalize( victim->get_class(  ) ), victim->mental_state, victim->numattacks );

      ch->printf( "|Race  : &G%10s &w|Barehand: &G%d&wd&G%d&w+&G%d&w\r\n",
                  capitalize( victim->get_race(  ) ), victim->barenumdie, victim->baresizedie, victim->GET_DAMROLL(  ) );

      ch->printf( "|Deity :&G%11s &w|Authed:&G%11s &w|SF    :&G%11d &w|PVer  : &G%d&w\r\n",
                  ( victim->pcdata->deity == NULL ) ? "(NONE)" : victim->pcdata->deity->name.c_str(  ),
                  !victim->pcdata->authed_by.empty(  )? victim->pcdata->authed_by.c_str(  ) : "Unknown", victim->spellfail, victim->pcdata->version );

      ch->printf( "|Map   : &G%10s &w|Coords: &G%d %d&w\r\n", victim->has_pcflag( PCFLAG_ONMAP ) ? map_names[victim->cmap] : "(NONE)", victim->mx, victim->my );

      ch->printf( "|Master: &G%10s &w|Leader: &G%s&w\r\n", victim->master ? victim->master->name : "(NONE)", victim->leader ? victim->leader->name : "(NONE)" );

      ch->printf( "|Saves : ---------- | ----------------- | ----------------- | -----------------\r\n" );

      ch->printf( "|Poison: &G%10d &w|Para  : &G%10d &w|Wands : &G%10d &w|Spell : &G%d&w\r\n",
                  victim->saving_poison_death, victim->saving_para_petri, victim->saving_wand, victim->saving_spell_staff );

      ch->printf( "|Death : &G%10d &w|Petri : &G%10d &w|Breath: &G%10d &w|Staves: &G%d&w\r\n",
                  victim->saving_poison_death, victim->saving_para_petri, victim->saving_breath, victim->saving_spell_staff );

      if( victim->desc )
      {
         ch->printf( "|Player's Terminal Program: &G%s&w\r\n", victim->desc->client.c_str(  ) );
         ch->printf( "|Player's Terminal Support: &GMCCP[%s]  &GMSP[%s]\r\n", victim->desc->can_compress ? "&wX&G" : " ", victim->desc->msp_detected ? "&wX&G" : " " );

         ch->printf( "|Terminal Support In Use  : &GMCCP[%s]  &GMSP[%s]\r\n", victim->desc->can_compress ? "&wX&G" : " ", victim->MSP_ON(  )? "&wX&G" : " " );
      }

      ch->printf( "|PC Flags    : &G%s&w\r\n", !victim->has_pcflags(  )? "(NONE)" : bitset_string( victim->get_pcflags(  ), pc_flags ) );

      ch->print( "|Languages: &[score]" );
      for( iLang = 0; iLang < LANG_UNKNOWN; ++iLang )
         if( knows_language( victim, iLang, victim ) )
         {
            if( iLang == victim->speaking )
               ch->set_color( AT_SCORE3 );
            ch->printf( "%s ", lang_names[iLang] );
            ch->set_color( AT_SCORE );
         }
      ch->print( "\r\n" );

      ch->printf( "&w|Affected By : &G%s&w\r\n", !victim->has_aflags(  )? "(NONE)" : bitset_string( victim->get_aflags(  ), aff_flags ) );
      ch->printf( "|Bestowments : &G%s&w\r\n", victim->pcdata->bestowments.empty(  )? "(NONE)" : victim->pcdata->bestowments.c_str(  ) );
      ch->printf( "|Resistances : &G%s&w\r\n", !victim->has_resists(  )? "(NONE)" : bitset_string( victim->get_resists(  ), ris_flags ) );
      ch->printf( "|Immunities  : &G%s&w\r\n", !victim->has_immunes(  )? "(NONE)" : bitset_string( victim->get_immunes(  ), ris_flags ) );
      ch->printf( "|Suscepts    : &G%s&w\r\n", !victim->has_susceps(  )? "(NONE)" : bitset_string( victim->get_susceps(  ), ris_flags ) );
      ch->printf( "|Absorbs     : &G%s&w\r\n", !victim->has_absorbs(  )? "(NONE)" : bitset_string( victim->get_absorbs(  ), ris_flags ) );

      list < affect_data * >::iterator paf;
      for( paf = victim->affects.begin(  ); paf != victim->affects.end(  ); ++paf )
      {
         affect_data *aff = *paf;

         if( ( skill = get_skilltype( aff->type ) ) != NULL )
         {
            char mod[MIL];

            if( aff->location == APPLY_AFFECT || aff->location == APPLY_EXT_AFFECT )
               mudstrlcpy( mod, aff_flags[aff->modifier], MIL );
            else if( aff->location == APPLY_RESISTANT || aff->location == APPLY_IMMUNE || aff->location == APPLY_ABSORB || aff->location == APPLY_SUSCEPTIBLE )
               mudstrlcpy( mod, ris_flags[aff->modifier], MIL );
            else
               snprintf( mod, MIL, "%d", aff->modifier );

            ch->printf( "|%s: '&G%s&w' modifies &G%s&w by &G%s&w for &G%d&w rounds", skill_tname[skill->type], skill->name, a_types[aff->location], mod, aff->duration );
            if( aff->bit > 0 )
               ch->printf( " with bits &G%s&w.\r\n", aff_flags[aff->bit] );
            else
               ch->print( ".\r\n" );
         }
      }

      if( !victim->variables.empty(  ) )
      {
         variable_data *vd;
         list < variable_data * >::iterator ivd;

         ch->pager( "&cVariables  : &w" );
         for( ivd = victim->variables.begin(  ); ivd != victim->variables.end(  ); ++ivd )
         {
            vd = *ivd;

            ch->pagerf( "%s:%d", vd->tag.c_str(  ), vd->vnum );

            switch( vd->type )
            {
               default:
                  ch->pager( "=INVALID!!!" );
                  break;

               case vtSTR:
                  if( !vd->varstring.empty() )
                     ch->pagerf( "=%s", vd->varstring.c_str() );
                  break;

               case vtINT:
                  if( vd->vardata > 0 )
                     ch->pagerf( "=%ld", vd->vardata );
                  break;

               case vtXBIT:
                  if( vd->varflags.any() )
                     ch->pagerf( "=%s", vd->varflags.to_string().c_str() );
                  break;
            }
            ch->pager( "  " );
         }
         ch->pager( "\r\n" );
      }
   }
   else
   {
      ch->printf( "&w|Name  : &G%-50s &w|Room  : &G%d&w\r\n", victim->name, victim->in_room->vnum );

      ch->printf( "|Area  : &G%-50s &w\r\n", victim->pIndexData->area ? victim->pIndexData->area->name : "(NONE)" );

      ch->printf( "|Level : &G%10d &w|Vnum  : &G%10d &w|Sex   : &G%10s &w|Gold  : &G%d&w\r\n", victim->level, victim->pIndexData->vnum, npc_sex[victim->sex], victim->gold );

      ch->printf( "|STR   : &G%10d &w|HPs   : &G%10d &w|MaxHPs: &G%10d &w|Exp   : &G%d&w\r\n", victim->get_curr_str(  ), victim->hit, victim->max_hit, victim->exp );

      ch->printf( "|INT   : &G%10d &w|Mana  : &G%10d &w|MaxMan: &G%10d &w|Pos   : &G%s&w\r\n",
                  victim->get_curr_int(  ), victim->mana, victim->max_mana, npc_position[victim->position] );

      ch->printf( "|WIS   : &G%10d &w|Move  : &G%10d &w|MaxMov: &G%10d &w|H.D.  : &G%d&wd&G%d&w+&G%d&w\r\n",
                  victim->get_curr_wis(  ), victim->move, victim->max_move, victim->pIndexData->hitnodice, victim->pIndexData->hitsizedice, victim->pIndexData->hitplus );

      ch->printf( "|DEX   : &G%10d &w|AC    : &G%10d &w|Align : &G%10d &w|D.D.  : &G%d&wd&G%d&w+&G%d&w\r\n",
                  victim->get_curr_dex(  ), victim->GET_AC(  ), victim->alignment, victim->pIndexData->damnodice,
                  victim->pIndexData->damsizedice, victim->pIndexData->damplus );

      ch->printf( "|CON   : &G%10d &w|Thac0 : &G%10d &w|+Hit  : &G%10d &w|+Dam  : &G%d&w\r\n",
                  victim->get_curr_con(  ), calc_thac0( victim, NULL, 0 ), victim->GET_HITROLL(  ), victim->GET_DAMROLL(  ) );

      ch->printf( "|CHA   : &G%10d &w|Count : &G%10d &w|Timer : &G%10d&w\r\n", victim->get_curr_cha(  ), victim->pIndexData->count, victim->timer );

      ch->printf( "|LCK   : &G%10d &w|#Attks: &G%10f &w|Thac0*: &G%10d &w|Exp*  : &G%d&w\r\n",
                  victim->get_curr_lck(  ), victim->numattacks, victim->mobthac0, victim->pIndexData->exp );

      ch->printf( "|Class : &G%10s &w|Master: &G%s&w\r\n", capitalize( npc_class[victim->Class] ), victim->master ? victim->master->name : "(NONE)" );

      ch->printf( "|Race  : &G%10s &w|Leader: &G%s&w\r\n", capitalize( npc_race[victim->race] ), victim->leader ? victim->leader->name : "(NONE)" );

      ch->printf( "|Map   : &G%10s &w|Coords: &G%d %d    &w|Native Sector: &G%s&w\r\n",
                  victim->has_actflag( ACT_ONMAP ) ? map_names[victim->cmap] : "(NONE)",
                  victim->mx, victim->my, victim->sector < 0 ? "Not set yet" : sect_types[victim->sector] );

      ch->printf( "|Saves : ---------- | ----------------- | ----------------- | -----------------\r\n" );

      ch->printf( "|Poison: &G%10d &w|Para  : &G%10d &w|Wands : &G%10d &w|Spell : &G%d&w\r\n",
                  victim->saving_poison_death, victim->saving_para_petri, victim->saving_wand, victim->saving_spell_staff );

      ch->printf( "|Death : &G%10d &w|Petri : &G%10d &w|Breath: &G%10d &w|Staves: &G%d&w\r\n",
                  victim->saving_poison_death, victim->saving_para_petri, victim->saving_breath, victim->saving_spell_staff );

      ch->printf( "|Short : &G%s&w\r\n", ( victim->short_descr && victim->short_descr[0] != '\0' ) ? victim->short_descr : "(NONE)" );

      ch->printf( "|Long  : &G%s&w\r\n", ( victim->long_descr && victim->long_descr[0] != '\0' ) ? victim->long_descr : "(NONE)" );

      ch->print( "|Languages: &[score]" );
      for( iLang = 0; iLang < LANG_UNKNOWN; ++iLang )
         if( knows_language( victim, iLang, victim ) || ( ch->isnpc(  ) && !victim->has_langs(  ) ) )
         {
            if( iLang == victim->speaking )
               ch->set_color( AT_SCORE3 );
            ch->printf( "%s ", lang_names[iLang] );
            ch->set_color( AT_SCORE );
         }
      ch->print( "\r\n" );

      ch->printf( "&w|Act Flags   : &G%s&w\r\n", bitset_string( victim->get_actflags(  ), act_flags ) );
      ch->printf( "|Affected By : &G%s&w\r\n", !victim->has_aflags(  )? "(NONE)" : bitset_string( victim->get_aflags(  ), aff_flags ) );
      ch->printf( "|Mob Spec Fun: &G%s&w\r\n", !victim->spec_funname.empty(  )? victim->spec_funname.c_str(  ) : "(NONE)" );
      ch->printf( "|Body Parts  : &G%s&w\r\n", bitset_string( victim->get_bparts(  ), part_flags ) );
      ch->printf( "|Resistances : &G%s&w\r\n", !victim->has_resists(  )? "(NONE)" : bitset_string( victim->get_resists(  ), ris_flags ) );
      ch->printf( "|Immunities  : &G%s&w\r\n", !victim->has_immunes(  )? "(NONE)" : bitset_string( victim->get_immunes(  ), ris_flags ) );
      ch->printf( "|Suscepts    : &G%s&w\r\n", !victim->has_susceps(  )? "(NONE)" : bitset_string( victim->get_susceps(  ), ris_flags ) );
      ch->printf( "|Absorbs     : &G%s&w\r\n", !victim->has_absorbs(  )? "(NONE)" : bitset_string( victim->get_absorbs(  ), ris_flags ) );
      ch->printf( "|Attacks     : &G%s&w\r\n", bitset_string( victim->get_attacks(  ), attack_flags ) );
      ch->printf( "|Defenses    : &G%s&w\r\n", bitset_string( victim->get_defenses(  ), defense_flags ) );

      if( victim->pIndexData->progtypes.none(  ) )
         ch->print( "|Mobprogs    : &GNone&w\r\n" );
      else
      {
         list < mud_prog_data * >::iterator mprg;

         ch->print( "|Mobprogs    : &G" );

         for( mprg = victim->pIndexData->mudprogs.begin(  ); mprg != victim->pIndexData->mudprogs.end(  ); ++mprg )
         {
            mud_prog_data *mprog = *mprg;

            ch->printf( "%s ", mprog_type_to_name( mprog->type ).c_str(  ) );
         }
         ch->print( "&w\r\n" );
      }

      list < affect_data * >::iterator paf;
      for( paf = victim->affects.begin(  ); paf != victim->affects.end(  ); ++paf )
      {
         affect_data *af = *paf;

         if( ( skill = get_skilltype( af->type ) ) != NULL )
         {
            char mod[MIL];

            if( af->location == APPLY_AFFECT
                || af->location == APPLY_RESISTANT || af->location == APPLY_IMMUNE || af->location == APPLY_ABSORB || af->location == APPLY_SUSCEPTIBLE )
               mudstrlcpy( mod, aff_flags[af->modifier], MIL );
            else
               snprintf( mod, MIL, "%d", af->modifier );
            ch->printf( "|%s: '&G%s&w' modifies &G%s&w by &G%s&w for &G%d&w rounds", skill_tname[skill->type], skill->name, a_types[af->location], mod, af->duration );
            if( af->bit > 0 )
               ch->printf( " with bits &G%s&w.\r\n", aff_flags[af->bit] );
            else
               ch->print( ".\r\n" );
         }
      }
   }
}

/* Shard-like consolidated stat command - Samson 3-21-98 */
CMDF( do_stat )
{
   char_data *victim;
   string arg;

   argument = one_argument( argument, arg );

   if( arg.empty(  ) )
   {
      ch->print( "Syntax:\r\n" );
      if( ch->level >= LEVEL_DEMI )
         ch->print( "stat mob <vnum or keyword>\r\n" );
      if( ch->level >= LEVEL_SAVIOR )
         ch->print( "stat obj <vnum or keyword>\r\n" );
      ch->print( "stat room <vnum>\r\n" );
      if( ch->level >= LEVEL_DEMI )
         ch->print( "stat player <name>" );
      return;
   }

   if( ( !str_cmp( arg, "mob" ) || !str_cmp( arg, "m" ) ) && ch->level >= LEVEL_DEMI )
   {
      if( !( victim = ch->get_char_world( argument ) ) )
      {
         ch->print( "No such mob is loaded.\r\n" );
         return;
      }

      if( !victim->isnpc(  ) )
      {
         ch->printf( "%s is not a mob!\r\n", victim->name );
         return;
      }
      do_mstat( ch, argument );
      return;
   }

   if( ( !str_cmp( arg, "player" ) || !str_cmp( arg, "p" ) ) && ch->level >= LEVEL_DEMI )
   {
      if( !( victim = ch->get_char_world( argument ) ) )
      {
         ch->print( "No such player is online.\r\n" );
         return;
      }

      if( victim->isnpc(  ) )
      {
         ch->printf( "%s is not a player!\r\n", argument.c_str(  ) );
         return;
      }
      do_mstat( ch, argument );
      return;
   }

   if( ( !str_cmp( arg, "obj" ) || !str_cmp( arg, "o" ) ) && ch->level >= LEVEL_SAVIOR )
   {
      do_ostat( ch, argument );
      return;
   }

   if( !str_cmp( arg, "room" ) || !str_cmp( arg, "r" ) )
   {
      do_rstat( ch, argument );
      return;
   }
   /*
    * echo syntax 
    */
   do_stat( ch, "" );
}

/*****
 * Oftype: Object find Type
 * Find object matching a certain type
 *****/
void find_oftype( char_data * ch, const string & argument )
{
   map < int, obj_index * >::iterator iobj = obj_index_table.begin(  );
   int nMatch, type;

   ch->set_pager_color( AT_PLAIN );

   nMatch = 0;

   /*
    * My God, now isn't this MUCH nicer than what was here before? - Samson 9-18-03 
    */
   type = get_otype( argument );
   if( type < 0 )
   {
      ch->printf( "%s is an invalid item type.\r\n", argument.c_str(  ) );
      return;
   }

   while( iobj != obj_index_table.end(  ) )
   {
      obj_index *pObjIndex = iobj->second;
 
      if( type == pObjIndex->item_type )
      {
         ++nMatch;
         ch->pagerf( "[%5d] %s\r\n", pObjIndex->vnum, capitalize( pObjIndex->short_descr ) );
      }
      ++iobj;
   }

   if( nMatch )
      ch->pagerf( "Number of matches: %d\n", nMatch );
   else
      ch->print( "Sorry, no matching item types found.\r\n" );
}

/* Consolidated find command 3-21-98 (SLAY DWIP) */
CMDF( do_find )
{
   string arg, arg2;

   argument = one_argument( argument, arg );
   argument = one_argument( argument, arg2 );

   if( arg.empty(  ) || arg2.empty(  ) )
   {
      ch->print( "Syntax:\r\n" );
      if( ch->level >= LEVEL_DEMI )
         ch->print( "find mob <keyword>\r\n" );
      ch->print( "find obj <keyword>\r\n" );
      ch->print( "find obj type <item type>\r\n" );
      return;
   }

   if( ( !str_cmp( arg, "mob" ) || !str_cmp( arg, "m" ) ) && ch->level >= LEVEL_DEMI )
   {
      do_mfind( ch, arg2 );
      return;
   }

   if( !str_cmp( arg, "obj" ) || !str_cmp( arg, "o" ) )
   {
      if( str_cmp( arg2, "type" ) )
         do_ofind( ch, arg2 );
      else
      {
         if( argument.empty(  ) )
            do_find( ch, "" );
         else
            find_oftype( ch, argument );
      }
      return;
   }
   /*
    * echo syntax 
    */
   do_find( ch, "" );
}

CMDF( do_mwhere )
{
   if( argument.empty(  ) )
   {
      ch->print( "Mwhere whom?\r\n" );
      return;
   }

   bool found = false;
   list < char_data * >::iterator ich;
   for( ich = charlist.begin(  ); ich != charlist.end(  ); ++ich )
   {
      char_data *victim = *ich;

      if( victim->isnpc(  ) && victim->in_room && hasname( victim->name, argument ) )
      {
         found = true;
         if( victim->has_actflag( ACT_ONMAP ) )
            ch->pagerf( "&Y[&W%5d&Y] &G%-28s &YOverland: &C%s %d %d\r\n", victim->pIndexData->vnum, victim->short_descr, map_names[victim->cmap], victim->mx, victim->my );
         else
            ch->pagerf( "&Y[&W%5d&Y] &G%-28s &Y[&W%5d&Y] &C%s\r\n", victim->pIndexData->vnum, victim->short_descr, victim->in_room->vnum, victim->in_room->name );
      }
   }
   if( !found )
      ch->printf( "You didn't find any %s.\r\n", argument.c_str(  ) );
}

/* Added 'show' argument for lowbie imms without ostat -- Blodkai */
/* Made show the default action :) Shaddai */
/* Trimmed size, added vict info, put lipstick on the pig -- Blod */
CMDF( do_bodybag )
{
   char buf2[MSL], buf3[MSL];
   string arg1, arg2;
   bool found = false, bag = false;

   argument = one_argument( argument, arg1 );
   if( arg1.empty(  ) )
   {
      ch->print( "&PSyntax:  bodybag <character> | bodybag <character> yes/bag/now\r\n" );
      return;
   }

   mudstrlcpy( buf3, " ", MSL );
   snprintf( buf2, MSL, "the corpse of %s", arg1.c_str(  ) );
   argument = one_argument( argument, arg2 );

   if( !arg2.empty(  ) && ( str_cmp( arg2, "yes" ) && str_cmp( arg2, "bag" ) && str_cmp( arg2, "now" ) ) )
   {
      ch->print( "\r\n&PSyntax:  bodybag <character> | bodybag <character> yes/bag/now\r\n" );
      return;
   }
   if( !str_cmp( arg2, "yes" ) || !str_cmp( arg2, "bag" ) || !str_cmp( arg2, "now" ) )
      bag = true;

   ch->pagerf( "\r\n&P%s remains of %s ... ", bag ? "Retrieving" : "Searching for", capitalize( arg1 ).c_str(  ) );

   list < obj_data * >::iterator iobj;
   for( iobj = objlist.begin(  ); iobj != objlist.end(  ); ++iobj )
   {
      obj_data *obj = *iobj;

      if( obj->in_room && !str_cmp( buf2, obj->short_descr ) && ( obj->pIndexData->vnum == OBJ_VNUM_CORPSE_PC ) )
      {
         ch->pager( "\r\n" );
         found = true;
         ch->pagerf( "&P%s:  %s%-12.12s   &PIn:  &w%-22.22s  &P[&w%5d&P]   &PTimer:  %s%2d",
                     bag ? "Bagging" : "Corpse",
                     bag ? "&R" : "&w", capitalize( arg1 ).c_str(  ), obj->in_room->area->name, obj->in_room->vnum,
                     obj->timer < 1 ? "&w" : obj->timer < 5 ? "&R" : obj->timer < 10 ? "&Y" : "&w", obj->timer );
         if( bag )
         {
            obj->from_room(  );
            obj = obj->to_char( ch );
            obj->timer = -1;
            ch->save(  );
         }
      }
   }

   if( !found )
   {
      ch->pager( "&PNo corpse was found.\r\n" );
      return;
   }
   ch->pager( "\r\n" );

   found = false;
   char_data *owner;
   list < char_data * >::iterator ich;
   for( ich = pclist.begin(  ); ich != pclist.end(  ); ++ich )
   {
      owner = *ich;

      if( ch->can_see( owner, true ) && !str_cmp( arg1, owner->name ) )
      {
         found = true;
         break;
      }
   }
   if( !found )
   {
      ch->pagerf( "&P%s is not currently online.\r\n", capitalize( arg1 ).c_str(  ) );
      return;
   }
   if( owner->pcdata->deity )
      ch->pagerf( "&P%s (%d) has %d favor with %s (needed to supplicate: %d)\r\n",
                  owner->name, owner->level, owner->pcdata->favor, owner->pcdata->deity->name.c_str(  ), owner->pcdata->deity->scorpse );
   else
      ch->pagerf( "&P%s (%d) has no deity.\r\n", owner->name, owner->level );
}

/* New owhere by Altrag, 03/14/96 */
CMDF( do_owhere )
{
   char buf[MSL];
   string arg, arg1;
   obj_data *obj;
   bool found;
   int icnt = 0;

   argument = one_argument( argument, arg );
   if( arg.empty(  ) )
   {
      ch->print( "Owhere what?\r\n" );
      return;
   }

   argument = one_argument( argument, arg1 );
   if( !arg1.empty(  ) && !str_prefix( arg1, "nesthunt" ) )
   {
      if( !( obj = ch->get_obj_world( arg ) ) )
      {
         ch->print( "Nesthunt for what object?\r\n" );
         return;
      }
      for( ; obj->in_obj; obj = obj->in_obj )
      {
         ch->pagerf( "&Y[&W%5d&Y] &G%-28s &Cin object &Y[&W%5d&Y] &C%s\r\n",
                     obj->pIndexData->vnum, obj->oshort(  ).c_str(  ), obj->in_obj->pIndexData->vnum, obj->in_obj->short_descr );
         ++icnt;
      }
      snprintf( buf, MSL, "&Y[&W%5d&Y] &G%-28s ", obj->pIndexData->vnum, obj->oshort(  ).c_str(  ) );
      if( obj->carried_by )
         snprintf( buf + strlen( buf ), MSL - strlen( buf ), "&Cinvent   &Y[&W%5d&Y] &C%s\r\n",
                   ( obj->carried_by->isnpc(  )? obj->carried_by->pIndexData->vnum : 0 ), PERS( obj->carried_by, ch, true ) );
      else if( obj->in_room )
      {
         if( obj->extra_flags.test( ITEM_ONMAP ) )
            snprintf( buf + strlen( buf ), MSL - strlen( buf ), "&Coverland &Y[&W%s&Y] &C%d %d\r\n", map_names[obj->cmap], obj->mx, obj->my );
         else
            snprintf( buf + strlen( buf ), MSL - strlen( buf ), "&Croom     &Y[&W%5d&Y] &C%s\r\n", obj->in_room->vnum, obj->in_room->name );
      }
      else if( obj->in_obj )
      {
         bug( "%s: obj->in_obj after NULL!", __FUNCTION__ );
         mudstrlcat( buf, "object??\r\n", MSL );
      }
      else
      {
         bug( "%s: object doesnt have location!", __FUNCTION__ );
         mudstrlcat( buf, "nowhere??\r\n", MSL );
      }
      ch->pager( buf );
      ++icnt;
      ch->pagerf( "Nested %d levels deep.\r\n", icnt );
      return;
   }

   found = false;
   list < obj_data * >::iterator iobj;
   for( iobj = objlist.begin(  ); iobj != objlist.end(  ); ++iobj )
   {
      obj = *iobj;

      if( !hasname( obj->name, arg ) )
         continue;
      found = true;

      snprintf( buf, MSL, "&Y(&W%3d&Y) [&W%5d&Y] &G%-28s ", ++icnt, obj->pIndexData->vnum, obj->oshort(  ).c_str(  ) );
      if( obj->carried_by )
         snprintf( buf + strlen( buf ), MSL - strlen( buf ), "&Cinvent   &Y[&W%5d&Y] &C%s\r\n",
                   ( obj->carried_by->isnpc(  )? obj->carried_by->pIndexData->vnum : 0 ), PERS( obj->carried_by, ch, true ) );
      else if( obj->in_room )
      {
         if( obj->extra_flags.test( ITEM_ONMAP ) )
            snprintf( buf + strlen( buf ), MSL - strlen( buf ), "&Coverland &Y[&W%s&Y] &C%d %d\r\n", map_names[obj->cmap], obj->mx, obj->my );
         else
            snprintf( buf + strlen( buf ), MSL - strlen( buf ), "&Croom     &Y[&W%5d&Y] &C%s\r\n", obj->in_room->vnum, obj->in_room->name );
      }
      else if( obj->in_obj )
         snprintf( buf + strlen( buf ), MSL - strlen( buf ), "&Cobject &Y[&W%5d&Y] &C%s\r\n", obj->in_obj->pIndexData->vnum, obj->in_obj->oshort(  ).c_str(  ) );
      else
      {
         bug( "%s: object doesnt have location!", __FUNCTION__ );
         mudstrlcat( buf, "nowhere??\r\n", MSL );
      }
      ch->pager( buf );
   }
   if( !found )
      ch->printf( "You didn't find any %s.\r\n", arg.c_str(  ) );
   else
      ch->pagerf( "%d matches.\r\n", icnt );
}

CMDF( do_pwhere )
{
   bool found;

   if( argument.empty(  ) )
   {
      list < descriptor_data * >::iterator ds;

      ch->pager( "&[people]Players you can see online:\r\n" );
      found = false;
      for( ds = dlist.begin(  ); ds != dlist.end(  ); ++ds )
      {
         descriptor_data *d = *ds;
         char_data *victim;

         if( ( d->connected == CON_PLAYING || d->connected == CON_EDITING )
             && ( victim = d->character ) != NULL && !victim->isnpc(  ) && victim->in_room && ch->can_see( victim, true ) && !is_ignoring( victim, ch ) )
         {
            found = true;
            if( victim->has_pcflag( PCFLAG_ONMAP ) )
               ch->pagerf( "&G%-28s &Y[&WOverland&Y] &C%s %d %d\r\n", victim->name, map_names[victim->cmap], victim->mx, victim->my );
            else
               ch->pagerf( "&G%-28s &Y[&W%5d&Y]&C %s\r\n", victim->name, victim->in_room->vnum, victim->in_room->name );
         }
      }
      if( !found )
         ch->pager( "None.\r\n" );
   }
   else
   {
      list < char_data * >::iterator ich;

      ch->pager( "You search high and low and find:\r\n" );
      found = false;
      for( ich = pclist.begin(  ); ich != pclist.end(  ); ++ich )
      {
         char_data *victim = *ich;

         if( victim->in_room && ch->can_see( victim, true ) && hasname( victim->name, argument ) )
         {
            found = true;
            ch->pagerf( "&Y%-28s &G[&W%5d&G]&C %s\r\n", PERS( victim, ch, true ), victim->in_room->vnum, victim->in_room->name );
            break;
         }
      }
      if( !found )
         ch->pager( "Nobody by that name.\r\n" );
   }
}

void where_mobile( char_data * ch, const string & argument )
{
   ch->set_color( AT_PLAIN );

   if( !is_number( argument ) )
   {
      do_mwhere( ch, argument );
      return;
   }

   int vnum = atoi( argument.c_str(  ) );

   mob_index *pMobIndex;
   if( !( pMobIndex = get_mob_index( vnum ) ) )
   {
      ch->print( "No mobile has that vnum.\r\n" );
      return;
   }

   int mobcnt = 0;
   bool found = false;
   list < char_data * >::iterator ich;
   for( ich = charlist.begin(  ); ich != charlist.end(  ); ++ich )
   {
      char_data *victim = *ich;

      if( victim->isnpc(  ) && victim->in_room && victim->pIndexData->vnum == vnum )
      {
         found = true;
         ++mobcnt;
         ch->pagerf( "[%5d] %-28s [%5d] %s\r\n", victim->pIndexData->vnum, victim->short_descr, victim->in_room->vnum, victim->in_room->name );
      }
   }

   if( !found )
      ch->pagerf( "No copies of vnum %d are loaded.\r\n", vnum );
   else
      ch->pagerf( "%d matches for vnum %d are loaded.\r\n", mobcnt, vnum );
}

/* Consolidated Where command - Samson 3-21-98 */
CMDF( do_where )
{
   string arg;

   if( ch->level < LEVEL_SAVIOR )
   {
      do_oldwhere( ch, argument );
      return;
   }

   argument = one_argument( argument, arg );
   if( arg.empty(  ) )
   {
      ch->print( "Syntax:\r\n" );
      if( ch->level >= LEVEL_DEMI )
         ch->print( "where mob <keyword or vnum>\r\n" );
      ch->print( "where obj <keyword or vnum>\r\n" );
      if( ch->level >= LEVEL_DEMI )
         ch->print( "where player <name>\r\n" );
      return;
   }

   if( ( !str_cmp( arg, "mob" ) || !str_cmp( arg, "m" ) ) && ch->level >= LEVEL_DEMI )
   {
      where_mobile( ch, argument );
      return;
   }

   if( ( !str_cmp( arg, "player" ) || !str_cmp( arg, "p" ) ) && ch->level >= LEVEL_DEMI )
   {
      do_pwhere( ch, argument );
      return;
   }

   if( !str_cmp( arg, "obj" ) || !str_cmp( arg, "o" ) )
   {
      char buf[MSL];
      obj_index *pObjIndex;
      int icnt = 0;
      bool found = false;

      ch->set_color( AT_PLAIN );

      if( !is_number( argument ) )
      {
         do_owhere( ch, argument );
         return;
      }

      int vnum = atoi( argument.c_str(  ) );

      if( !( pObjIndex = get_obj_index( vnum ) ) )
      {
         ch->print( "No object has that vnum.\r\n" );
         return;
      }

      list < obj_data * >::iterator iobj;
      for( iobj = objlist.begin(  ); iobj != objlist.end(  ); ++iobj )
      {
         obj_data *obj = *iobj;

         if( obj->pIndexData->vnum != vnum )
            continue;
         found = true;

         snprintf( buf, MSL, "(%3d) [%5d] %-28s in ", ++icnt, obj->pIndexData->vnum, obj->oshort(  ).c_str(  ) );
         if( obj->carried_by )
            snprintf( buf + strlen( buf ), MSL - strlen( buf ), "invent [%5d] %s\r\n",
                      ( obj->carried_by->isnpc(  )? obj->carried_by->pIndexData->vnum : 0 ), PERS( obj->carried_by, ch, false ) );
         else if( obj->in_room )
            snprintf( buf + strlen( buf ), MSL - strlen( buf ), "room   [%5d] %s\r\n", obj->in_room->vnum, obj->in_room->name );
         else if( obj->in_obj )
            snprintf( buf + strlen( buf ), MSL - strlen( buf ), "object [%5d] %s\r\n", obj->in_obj->pIndexData->vnum, obj->in_obj->oshort(  ).c_str(  ) );
         else
         {
            bug( "%s: object '%s' doesn't have location!", __FUNCTION__, obj->short_descr );
            mudstrlcat( buf, "nowhere??\r\n", MSL );
         }
         ch->pager( buf );
      }

      if( !found )
         ch->pagerf( "No copies of vnum %d are loaded.\r\n", vnum );
      else
         ch->pagerf( "%d matches for vnum %d are loaded.\r\n", icnt, vnum );

      ch->pagerf( "Checking player files for stored copies of vnum %d....\r\n", vnum );

      check_stored_objects( ch, vnum );
      return;
   }
   /*
    * echo syntax 
    */
   do_where( ch, "" );
}

CMDF( do_reboo )
{
   ch->print( "&YIf you want to REBOOT, spell it out.\r\n" );
}

/* Added security check for online compiler - Samson 4-8-98 */
CMDF( do_reboot )
{
#ifdef MULTIPORT
   if( compilelock )
   {
      ch->print( "&RSorry, the mud cannot be rebooted during a compiler operation.\r\nPlease wait for the compiler to finish.\r\n" );
      return;
   }
#endif

   ch->set_color( AT_IMMORT );

   if( argument.empty(  ) )
   {
      if( bootlock )
      {
         ch->print( "Reboot will be cancelled at the next event poll.\r\n" );
         reboot_counter = -5;
         bootlock = false;
         sysdata->DENY_NEW_PLAYERS = false;
         return;
      }
      sysdata->DENY_NEW_PLAYERS = true;
      reboot_counter = sysdata->rebootcount;

      echo_to_all( "&RReboot countdown started.", ECHOTAR_ALL );
      echo_all_printf( ECHOTAR_ALL, "&YGame reboot in %d minutes.", reboot_counter );
      bootlock = true;
      add_event( 60, ev_reboot_count, NULL );
      return;
   }

   if( str_cmp( argument, "mud now" ) && str_cmp( argument, "nosave" ) )
   {
      ch->print( "Syntax: reboot mud now\r\n" );
      ch->print( "Syntax: reboot nosave\r\n" );
      ch->print( "\r\nOr type 'reboot' with no argument to start a countdown.\r\n" );
      return;
   }

   echo_to_all( "&[immortal]Manual Game Reboot", ECHOTAR_ALL );

   /*
    * Save all characters before booting. 
    */
   if( !str_cmp( argument, "nosave" ) )
      DONTSAVE = true;

   mud_down = true;
}

CMDF( do_shutdow )
{
   ch->print( "&YIf you want to SHUTDOWN, spell it out.\r\n" );
}

/* Added check for online compiler - Samson 4-8-98 */
CMDF( do_shutdown )
{
#ifdef MULTIPORT
   if( compilelock )
   {
      ch->print( "&RSorry, the mud cannot be shutdown during a compiler operation.\r\nPlease wait for the compiler to finish.\r\n" );
      return;
   }
#endif

   ch->set_color( AT_IMMORT );

   if( str_cmp( argument, "mud now" ) && str_cmp( argument, "nosave" ) )
   {
      ch->print( "Syntax:  'shutdown mud now' or 'shutdown nosave'\r\n" );
      return;
   }

   echo_to_all( "&[immortal]Manual Game Shutdown.", ECHOTAR_ALL );
   shutdown_mud( "Manual shutdown" );

   /*
    * Save all characters before booting. 
    */
   if( !str_cmp( argument, "nosave" ) )
      DONTSAVE = true;

   mud_down = true;
}

CMDF( do_snoop )
{
   char_data *victim;
   list < descriptor_data * >::iterator ds;
   descriptor_data *d;

   ch->set_color( AT_IMMORT );

   if( argument.empty(  ) )
   {
      ch->print( "&GSnooping the following people:\r\n\r\n" );
      for( ds = dlist.begin(  ); ds != dlist.end(  ); ++ds )
      {
         d = *ds;

         if( d->snoop_by == ch->desc )
            ch->printf( "%s ", d->original ? d->original->name : d->character->name );
      }
      return;
   }

   if( !( victim = get_wizvictim( ch, argument, true ) ) )
      return;

   if( !victim->desc )
   {
      ch->print( "No descriptor to snoop.\r\n" );
      return;
   }

   if( victim == ch )
   {
      ch->print( "Cancelling all snoops.\r\n" );
      for( ds = dlist.begin(  ); ds != dlist.end(  ); ++ds )
      {
         d = *ds;

         if( d->snoop_by == ch->desc )
            d->snoop_by = NULL;
      }
      return;
   }

   if( victim->desc->snoop_by )
   {
      ch->print( "Busy already.\r\n" );
      return;
   }

   if( ch->desc )
   {
      for( d = ch->desc->snoop_by; d; d = d->snoop_by )
         if( d->character == victim || d->original == victim )
         {
            ch->print( "No snoop loops.\r\n" );
            return;
         }
   }
   victim->desc->snoop_by = ch->desc;
   ch->print( "Ok.\r\n" );
}

CMDF( do_switch )
{
   char_data *victim;

   ch->set_color( AT_IMMORT );

   if( argument.empty(  ) )
   {
      ch->print( "Switch into whom?\r\n" );
      return;
   }

   if( !ch->desc )
      return;

   if( ch->desc->original )
   {
      ch->print( "You are already switched.\r\n" );
      return;
   }

   if( !( victim = ch->get_char_world( argument ) ) )
   {
      ch->print( "They aren't here.\r\n" );
      return;
   }

   if( victim == ch )
   {
      ch->print( "Be serious.\r\n" );
      return;
   }

   if( victim->desc )
   {
      ch->print( "Character in use.\r\n" );
      return;
   }

   if( !victim->isnpc(  ) && !ch->is_imp(  ) )
   {
      ch->print( "You cannot switch into a player!\r\n" );
      return;
   }

   if( victim->switched )
   {
      ch->print( "You can't switch into a player that is switched!\r\n" );
      return;
   }

   if( victim->has_pcflag( PCFLAG_FREEZE ) )
   {
      ch->print( "You shouldn't switch into a player that is frozen!\r\n" );
      return;
   }

   ch->desc->character = victim;
   ch->desc->original = ch;
   victim->desc = ch->desc;
   ch->desc = NULL;
   ch->switched = victim;
   victim->print( "Ok.\r\n" );
}

CMDF( do_return )
{
   if( !ch->isnpc(  ) && ch->get_trust(  ) < LEVEL_IMMORTAL )
   {
      ch->print( "Huh?\r\n" );
      return;
   }
   ch->set_color( AT_IMMORT );

   if( !ch->desc )
      return;

   if( !ch->desc->original )
   {
      ch->print( "You aren't switched.\r\n" );
      return;
   }

   ch->print( "You return to your original body.\r\n" );

   ch->desc->character = ch->desc->original;
   ch->desc->original = NULL;
   ch->desc->character->desc = ch->desc;
   ch->desc->character->switched = NULL;
   ch->desc = NULL;
}

void objinvoke( char_data * ch, string & argument )
{
   string arg, arg2;
   obj_index *pObjIndex;
   obj_data *obj;
   int vnum, level, quantity;

   ch->set_color( AT_IMMORT );

   if( argument.empty(  ) )
   {
      ch->print( "Syntax: load obj <vnum/keyword> <level> <quantity>\r\n" );
      return;
   }

   argument = one_argument( argument, arg );
   argument = one_argument( argument, arg2 );
   if( arg2.empty(  ) )
      level = 1;
   else
   {
      if( !is_number( arg2 ) )
      {
         ch->print( "Syntax: load obj <vnum/keyword> <level>\r\n" );
         return;
      }

      level = atoi( arg2.c_str(  ) );
      if( level < 0 || level > ch->get_trust(  ) )
      {
         ch->print( "Limited to your trust level.\r\n" );
         return;
      }
   }

   if( argument.empty(  ) )
      quantity = 1;
   else
   {
      if( !is_number( argument ) )
      {
         ch->print( "Syntax: load obj <vnum/keyword> <level> <quantity>\r\n" );
         return;
      }

      quantity = atoi( argument.c_str(  ) );
      if( quantity < 1 || quantity > MAX_OINVOKE_QUANTITY )
      {
         ch->printf( "You must oinvoke between 1 and %d items.\r\n", MAX_OINVOKE_QUANTITY );
         return;
      }
   }

   if( !is_number( arg ) )
   {
      string arg3;
      map < int, obj_index * >::iterator iobj = obj_index_table.begin(  );
      int cnt = 0, count = number_argument( arg, arg3 );

      vnum = -1;
      while( iobj != obj_index_table.end(  ) )
      {
         pObjIndex = iobj->second;
 
         if( hasname( pObjIndex->name, arg3 ) && ++cnt == count )
         {
            vnum = pObjIndex->vnum;
            break;
         }
         ++iobj;
      }

      if( vnum == -1 )
      {
         ch->print( "No such object exists.\r\n" );
         return;
      }
   }
   else
      vnum = atoi( arg.c_str(  ) );

   if( !( pObjIndex = get_obj_index( vnum ) ) )
   {
      ch->print( "No object has that vnum.\r\n" );
      return;
   }

   if( !( obj = pObjIndex->create_object( level ) ) )
   {
      log_printf( "create_object: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
      return;
   }

   if( obj->ego >= sysdata->minego && quantity > 1 )
   {
      if( !ch->is_imp(  ) )
      {
         ch->print( "Loading multiple copies of rare items is restricted to KLs and above.\r\n" );
         obj->extract(  ); // It got created, now we need to destroy it.
         return;
      }
   }
   obj->count = quantity;

#ifdef MULTIPORT
   if( obj->ego >= sysdata->minego && mud_port == MAINPORT )
   {
      if( !ch->is_imp(  ) )
      {
         ch->print( "Loading of rare items is restricted to KLs and above on this port.\r\n" );
         obj->extract(  ); // It got created, now we need to destroy it.
         return;
      }
      else
      {
         ch->printf( "WARNING: This item has ego exceeding %d! Destroy this item when finished!\r\n", sysdata->minego );
         log_printf( "%s: %s has loaded a copy of vnum %d.", __FUNCTION__, ch->name, pObjIndex->vnum );
      }
   }
#else
   if( obj->ego >= sysdata->minego )
   {
      ch->printf( "WARNING: This item has ego exceeding %d! Destroy this item when finished!\r\n", sysdata->minego );
      log_printf( "%s: %s has loaded a copy of vnum %d.", __FUNCTION__, ch->name, pObjIndex->vnum );
   }
#endif

   if( obj->wear_flags.test( ITEM_TAKE ) )
   {
      obj = obj->to_char( ch );
      act( AT_IMMORT, "$n waves $s hand, and $p appears in $s inventory!", ch, obj, NULL, TO_ROOM );
      act( AT_IMMORT, "You wave your hand, and $p appears in your inventory!", ch, obj, NULL, TO_CHAR );
   }
   else
   {
      obj = obj->to_room( ch->in_room, ch );
      act( AT_IMMORT, "$n waves $s hand, and $p appears in the room!", ch, obj, NULL, TO_ROOM );
      act( AT_IMMORT, "You wave your hand, and $p appears in the room!", ch, obj, NULL, TO_CHAR );
   }

   /*
    * This is bad, means stats were exceeding ego specs 
    */
   if( obj->ego == -2 )
      ch->print( "&YWARNING: This object exceeds allowable ego specs.\r\n" );

   /*
    * I invoked what? --Blodkai 
    */
   ch->printf( "&Y(&W#%d &Y- &W%s &Y- &Wlvl %d&Y)\r\n", pObjIndex->vnum, pObjIndex->name, obj->level );
}

void mobinvoke( char_data * ch, string & argument )
{
   mob_index *pMobIndex;
   char_data *victim;
   int vnum;

   ch->set_color( AT_IMMORT );

   if( argument.empty(  ) )
   {
      ch->print( "Syntax: load m <vnum/keyword>\r\n" );
      return;
   }

   if( !is_number( argument ) )
   {
      string arg2;
      int cnt = 0;
      int count = number_argument( argument, arg2 );

      vnum = -1;
      map < int, mob_index * >::iterator imob = mob_index_table.begin(  );
      while( imob != mob_index_table.end(  ) )
      {
         pMobIndex = imob->second;

         if( hasname( pMobIndex->player_name, arg2 ) && ++cnt == count )
         {
            vnum = pMobIndex->vnum;
            break;
         }
         ++imob;
      }
      if( vnum == -1 )
      {
         ch->print( "No such mobile exists.\r\n" );
         return;
      }
   }
   else
      vnum = atoi( argument.c_str(  ) );

   if( ch->get_trust(  ) < LEVEL_DEMI )
   {
      area_data *pArea;

      if( ch->isnpc(  ) )
      {
         ch->print( "Huh?\r\n" );
         return;
      }

      if( !( pArea = ch->pcdata->area ) )
      {
         ch->print( "You must have an assigned area to invoke this mobile.\r\n" );
         return;
      }

      if( vnum < pArea->low_vnum || vnum > pArea->hi_vnum )
      {
         ch->print( "That number is not in your allocated range.\r\n" );
         return;
      }
   }

   if( !( pMobIndex = get_mob_index( vnum ) ) )
   {
      ch->print( "No mobile has that vnum.\r\n" );
      return;
   }

   victim = pMobIndex->create_mobile(  );
   if( !victim->to_room( ch->in_room ) )
      log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );

   /*
    * If you load one on the map, make sure it gets placed properly - Samson 8-21-99 
    */
   fix_maps( ch, victim );
   victim->sector = get_terrain( ch->cmap, ch->mx, ch->my );

   act( AT_IMMORT, "$n peers into the ether, and plucks out $N!", ch, NULL, victim, TO_ROOM );
   /*
    * How about seeing what we're invoking for a change. -Blodkai
    */
   ch->printf( "&YYou peer into the ether.... and pluck out %s!\r\n", pMobIndex->short_descr );
   ch->printf( "(&W#%d &Y- &W%s &Y- &Wlvl %d&Y)\r\n", pMobIndex->vnum, pMobIndex->player_name, victim->level );
}

/* Shard-like load command spliced off of ROT codebase - Samson 3-21-98 */
CMDF( do_load )
{
   string arg;

   argument = one_argument( argument, arg );
   if( arg.empty(  ) )
   {
      ch->print( "Syntax:\r\n" );
      if( ch->level >= LEVEL_DEMI )
         ch->print( "load mob <vnum or keyword>\r\n" );
      ch->print( "load obj <vnum or keyword>\r\n" );
      return;
   }

   if( ( !str_cmp( arg, "mob" ) || !str_cmp( arg, "m" ) ) && ch->level >= LEVEL_DEMI )
   {
      mobinvoke( ch, argument );
      return;
   }

   if( !str_cmp( arg, "obj" ) || !str_cmp( arg, "o" ) )
   {
      objinvoke( ch, argument );
      return;
   }
   /*
    * echo syntax 
    */
   do_load( ch, "" );
}

CMDF( do_purge )
{
   ch->set_color( AT_IMMORT );

   if( argument.empty(  ) )
   {
      /*
       * 'purge' 
       */
      list < char_data * >::iterator ich;
      for( ich = ch->in_room->people.begin(  ); ich != ch->in_room->people.end(  ); )
      {
         char_data *victim = *ich;
         ++ich;
         bool found = false;

         /*
          * GACK! Why did this get removed?? 
          */
         if( !victim->isnpc(  ) )
            continue;

         char_data *tch = NULL;
         list < char_data * >::iterator ich2;
         for( ich2 = ch->in_room->people.begin(  ); ich2 != ch->in_room->people.end(  ); ++ich2 )
         {
            tch = *ich2;

            if( !tch->isnpc(  ) && tch->pcdata->dest_buf == victim )
            {
               found = true;
               break;
            }
         }

         if( found && !tch->isnpc(  ) && tch->pcdata->dest_buf == victim )
            continue;

         if( ( !str_cmp( victim->short_descr, "The Negative Magnetic Space Wedgy" )
               || !str_cmp( victim->short_descr, "Krusty" )
               || !str_cmp( victim->short_descr, "Satan" ) || !str_cmp( victim->short_descr, "Mini-Cam" ) ) && str_cmp( ch->name, "Samson" ) )
         {
            ch->print( "Did you REALLY think the Great Lord and Master would allow that?\r\n" );
            continue;
         }

         /*
          * This will work in normal rooms too since they should always be -1,-1,-1 outside of the maps. 
          */
         if( is_same_char_map( ch, victim ) )
            victim->extract( true );
      }

      list < obj_data * >::iterator iobj;
      for( iobj = ch->in_room->objects.begin(  ); iobj != ch->in_room->objects.end(  ); )
      {
         obj_data *obj = *iobj;
         ++iobj;
         bool found = false;

         char_data *tch = NULL;
         list < char_data * >::iterator ich2;
         for( ich2 = ch->in_room->people.begin(  ); ich2 != ch->in_room->people.end(  ); ++ich2 )
         {
            tch = *ich2;
            if( !tch->isnpc(  ) && tch->pcdata->dest_buf == obj )
            {
               found = true;
               break;
            }
         }

         if( found && !tch->isnpc(  ) && tch->pcdata->dest_buf == obj )
            continue;

         /*
          * If target is on a map, make sure your at the right coordinates - Samson 
          */
         if( is_same_obj_map( ch, obj ) )
            obj->extract(  );
      }

      /*
       * Clan storeroom checks
       */
      if( ch->in_room->flags.test( ROOM_CLANSTOREROOM ) )
         check_clan_storeroom( ch );

      act( AT_IMMORT, "$n makes a complex series of gestures.... suddenly things seem a lot cleaner!", ch, NULL, NULL, TO_ROOM );
      act( AT_IMMORT, "You make a complex series of gestures.... suddenly things seem a lot cleaner!", ch, NULL, NULL, TO_CHAR );
      return;
   }

   char_data *victim = NULL;
   obj_data *obj = NULL;
   /*
    * fixed to get things in room first -- i.e., purge portal (obj),
    * * no more purging mobs with that keyword in another room first
    * * -- Tri 
    */
   if( !( victim = ch->get_char_room( argument ) ) && !( obj = ch->get_obj_here( argument ) ) )
   {
      ch->print( "That isn't here.\r\n" );
      return;
   }

   /*
    * Single object purge in room for high level purge - Scryn 8/12
    */
   if( obj )
   {
      list < char_data * >::iterator ich2;
      for( ich2 = ch->in_room->people.begin(  ); ich2 != ch->in_room->people.end(  ); ++ich2 )
      {
         char_data *tch = *ich2;
         if( !tch->isnpc(  ) && tch->pcdata->dest_buf == obj )
         {
            ch->print( "You cannot purge something being edited.\r\n" );
            return;
         }
      }
      obj->separate(  );
      act( AT_IMMORT, "$n snaps $s finger, and $p vanishes from sight!", ch, obj, NULL, TO_ROOM );
      act( AT_IMMORT, "You snap your fingers, and $p vanishes from sight!", ch, obj, NULL, TO_CHAR );
      obj->extract(  );

      /*
       * Clan storeroom checks
       */
      if( ch->in_room->flags.test( ROOM_CLANSTOREROOM ) )
         check_clan_storeroom( ch );

      return;
   }

   if( !victim->isnpc(  ) )
   {
      ch->print( "Not on PC's.\r\n" );
      return;
   }

   if( victim == ch )
   {
      ch->print( "You cannot purge yourself!\r\n" );
      return;
   }

   list < char_data * >::iterator ich2;
   for( ich2 = ch->in_room->people.begin(  ); ich2 != ch->in_room->people.end(  ); ++ich2 )
   {
      char_data *tch = *ich2;
      if( !tch->isnpc(  ) && tch->pcdata->dest_buf == victim )
      {
         ch->print( "You cannot purge something being edited.\r\n" );
         return;
      }
   }

   if( ( !str_cmp( victim->short_descr, "The Negative Magnetic Space Wedgy" )
         || !str_cmp( victim->short_descr, "Krusty" )
         || !str_cmp( victim->short_descr, "Satan" ) || !str_cmp( victim->short_descr, "Mini-Cam" ) ) && str_cmp( ch->name, "Samson" ) )
   {
      ch->print( "Did you REALLY think the Great Lord and Master would allow that?\r\n" );
      return;
   }
   act( AT_IMMORT, "$n snaps $s fingers, and $N vanishes from sight!", ch, NULL, victim, TO_NOTVICT );
   act( AT_IMMORT, "You snap your fingers, and $N vanishes from sight!", ch, NULL, victim, TO_CHAR );
   victim->extract( true );
}

void destroy_immdata( char_data * ch, const char *vicname )
{
   char buf[MSL], buf2[MSL];

   snprintf( buf, MSL, "%s%s", GOD_DIR, capitalize( vicname ) );

   if( !remove( buf ) )
      ch->print( "&RPlayer's immortal data destroyed.\r\n" );
   else if( errno != ENOENT )
   {
      ch->printf( "&RUnknown error #%d - %s (immortal data).  Report to Samson\r\n", errno, strerror( errno ) );
      snprintf( buf2, MSL, "%s balzhuring %s", ch->name, buf );
      perror( buf2 );
   }
   snprintf( buf2, MSL, "%s.are", vicname );

   list < area_data * >::iterator iarea;
   for( iarea = arealist.begin(  ); iarea != arealist.end(  ); ++iarea )
   {
      area_data *area = *iarea;

      if( !str_cmp( area->filename, buf2 ) )
      {
         snprintf( buf, MSL, "%s%s", BUILD_DIR, buf2 );
         area->fold( buf, false );
         deleteptr( area );
         snprintf( buf2, MSL, "%s.bak", buf );
         if( !rename( buf, buf2 ) )
            ch->print( "&RPlayer's area data destroyed. Area saved as backup.\r\n" );
         else if( errno != ENOENT )
         {
            ch->printf( "&RUnknown error #%d - %s (area data).  Report to  Samson.\r\n", errno, strerror( errno ) );
            snprintf( buf2, MSL, "%s destroying %s", ch->name, buf );
            perror( buf2 );
         }
         break;
      }
   }
}

CMDF( do_balzhur )
{
   char_data *victim;

   ch->set_color( AT_BLOOD );

   if( argument.empty(  ) )
   {
      ch->print( "Who is deserving of such a fate?\r\n" );
      return;
   }

   if( !( victim = get_wizvictim( ch, argument, true ) ) )
      return;

   victim->level = 2;
   victim->trust = 0;
   check_switch( victim );
   ch->print( "&WYou summon the Negative Magnetic Space Wedgy to wreak your wrath!\r\n" );
   ch->print( "The Wedgy sneers at you evilly, then vanishes in a puff of smoke.\r\n" );
   victim->print( "&[immortal]You hear an ungodly sound in the distance that makes your blood run cold!\r\n" );
   echo_all_printf( ECHOTAR_ALL, "&[immortal]The Wedgy screams, 'You are MINE %s!!!'", victim->name );
   victim->exp = 2000;
   victim->max_hit = 10;
   victim->max_mana = 100;
   victim->max_move = 150;
   victim->gold = 0;
   victim->pcdata->balance = 0;
   for( int sn = 0; sn < num_skills; ++sn )
      victim->pcdata->learned[sn] = 0;
   victim->pcdata->practice = 0;
   victim->hit = victim->max_hit;
   victim->mana = victim->max_mana;
   victim->move = victim->max_move;

   destroy_immdata( ch, victim->name );

   advance_level( victim );
   make_wizlist(  );
   interpret( victim, "help M_BALZHUR_" );
   victim->print( "&WYou awake after a long period of time...\r\n" );

   list < obj_data * >::iterator iobj;
   for( iobj = victim->carrying.begin(  ); iobj != victim->carrying.end(  ); )
   {
      obj_data *obj = *iobj;
      ++iobj;

      obj->extract(  );
   }
}

CMDF( do_advance )
{
   string arg1;
   char_data *victim;
   int level, iLevel;

   ch->set_color( AT_IMMORT );

   argument = one_argument( argument, arg1 );
   if( arg1.empty(  ) || argument.empty(  ) || !is_number( argument ) )
   {
      ch->print( "Syntax: advance <character> <level>\r\n" );
      return;
   }

   if( !( victim = get_wizvictim( ch, arg1, true ) ) )
      return;

   /*
    * You can demote yourself but not someone else at your own trust.-- Narn
    */
   /*
    * Guys, you have any idea how STUPID this sounds? -- Samson 
    */
   if( ch == victim )
   {
      ch->print( "Sorry, adjusting your own level is forbidden.\r\n" );
      return;
   }

   if( ( level = atoi( argument.c_str(  ) ) ) < 1 || level > LEVEL_AVATAR )
   {
      ch->printf( "Level range is 1 to %d.\r\n", LEVEL_AVATAR );
      return;
   }

   if( level > ch->get_trust(  ) )
   {
      ch->print( "You cannot advance someone beyond your own trust level.\r\n" );
      return;
   }

   /*
    * Check added to keep advance command from going beyond Avatar level - Samson 
    */
   if( victim->level >= LEVEL_AVATAR )
   {
      ch->print( "If your trying to raise an immortal's level, use the promote command.\r\n" );
      ch->print( "If your trying to lower an immortal's level, use the demote command.\r\n" );
      ch->printf( "If your trying to immortalize %s, use the immortalize command.\r\n", victim->name );
      return;
   }

   if( level == victim->level )
   {
      ch->printf( "What would be the point? %s is already level %d.\r\n", victim->name, level );
      return;
   }

   /*
    * Lower level:
    * *   Reset to level 1.
    * *   Then raise again.
    * *   Currently, an imp can lower another imp.
    * *   -- Swiftest
    * *   Can't lower imms >= your trust (other than self) per Narn's change.
    * *   Few minor text changes as well.  -- Blod
    */
   if( level < victim->level )
   {
      int sn;

      victim->set_color( AT_IMMORT );
      if( level < victim->level )
      {
         int tmp = victim->level;

         victim->level = level;
         check_switch( victim );
         victim->level = tmp;

         ch->printf( "Demoting %s from level %d to level %d!\r\n", victim->name, victim->level, level );
         victim->print( "Cursed and forsaken!  The gods have lowered your level...\r\n" );
      }
      else
      {
         ch->printf( "%s is already level %d.  Re-advancing...\r\n", victim->name, level );
         victim->print( "Deja vu! Your mind reels as you re-live your past levels!\r\n" );
      }
      victim->level = 1;
      victim->exp = exp_level( 1 );
      victim->max_hit = 20;
      victim->max_mana = 100;
      victim->max_move = 150;
      for( sn = 0; sn < num_skills; ++sn )
         victim->pcdata->learned[sn] = 0;
      victim->pcdata->practice = 0;
      victim->hit = victim->max_hit;
      victim->mana = victim->max_mana;
      victim->move = victim->max_move;
      advance_level( victim );
      /*
       * Rank fix added by Narn. 
       */
      STRFREE( victim->pcdata->rank );
      /*
       * Stuff added to make sure character's wizinvis level doesn't stay
       * higher than actual level, take wizinvis away from advance < 50 
       */
      if( victim->has_pcflag( PCFLAG_WIZINVIS ) )
         victim->pcdata->wizinvis = victim->trust;
      if( victim->has_pcflag( PCFLAG_WIZINVIS ) && ( victim->level <= LEVEL_AVATAR ) )
      {
         victim->unset_pcflag( PCFLAG_WIZINVIS );
         victim->pcdata->wizinvis = 0;
      }
   }
   else
   {
      ch->printf( "Raising %s from level %d to level %d!\r\n", victim->name, victim->level, level );
      victim->print( "The gods feel fit to raise your level!\r\n" );
   }
   for( iLevel = victim->level; iLevel < level; ++iLevel )
   {
      if( level < LEVEL_IMMORTAL )
         victim->print( "You raise a level!!\r\n" );
      victim->level += 1;
      advance_level( victim );
   }
   victim->exp = exp_level( victim->level );
   victim->trust = 0;
}

/*
 * "Fix" a character's stats - Thoric
 */
void fix_char( char_data * ch )
{
   list < affect_data * >::iterator paf;

   ch->de_equip(  );

   for( paf = ch->affects.begin(  ); paf != ch->affects.end(  ); ++paf )
   {
      affect_data *af = *paf;

      ch->affect_modify( af, false );
   }

   /*
    * As part of returning the char to their "natural naked state",
    * we must strip any room affects from them.
    */
   if( ch->in_room )
   {
      for( paf = ch->in_room->affects.begin(  ); paf != ch->in_room->affects.end(  ); ++paf )
      {
         affect_data *aff = *paf;

         if( aff->location != APPLY_WEARSPELL && aff->location != APPLY_REMOVESPELL && aff->location != APPLY_STRIPSN )
            ch->affect_modify( aff, false );
      }

      for( paf = ch->in_room->permaffects.begin(  ); paf != ch->in_room->permaffects.end(  ); ++paf )
      {
         affect_data *aff = *paf;

         if( aff->location != APPLY_WEARSPELL && aff->location != APPLY_REMOVESPELL && aff->location != APPLY_STRIPSN )
            ch->affect_modify( aff, false );
      }
   }

   ch->set_aflags( race_table[ch->race]->affected );
   ch->mental_state = -10;
   ch->hit = UMAX( 1, ch->hit );
   ch->mana = UMAX( 1, ch->mana );
   ch->move = UMAX( 1, ch->move );
   ch->armor = 100;
   ch->mod_str = 0;
   ch->mod_dex = 0;
   ch->mod_wis = 0;
   ch->mod_int = 0;
   ch->mod_con = 0;
   ch->mod_cha = 0;
   ch->mod_lck = 0;
   ch->damroll = 0;
   ch->hitroll = 0;
   ch->alignment = URANGE( -1000, ch->alignment, 1000 );
   ch->saving_breath = 0;
   ch->saving_wand = 0;
   ch->saving_para_petri = 0;
   ch->saving_spell_staff = 0;
   ch->saving_poison_death = 0;

   ch->carry_weight = 0;
   ch->carry_number = 0;

   ch->ClassSpecificStuff(  );   /* Brought over from DOTD code - Samson 4-6-99 */

   for( paf = ch->affects.begin(  ); paf != ch->affects.end(  ); ++paf )
   {
      affect_data *af = *paf;

      ch->affect_modify( af, true );
   }

   /*
    * Now that the char is fixed, add the room's affects back on.
    */
   if( ch->in_room )
   {
      for( paf = ch->in_room->affects.begin(  ); paf != ch->in_room->affects.end(  ); ++paf )
      {
         affect_data *aff = *paf;

         if( aff->location != APPLY_WEARSPELL && aff->location != APPLY_REMOVESPELL && aff->location != APPLY_STRIPSN )
            ch->affect_modify( aff, true );
      }

      for( paf = ch->in_room->permaffects.begin(  ); paf != ch->in_room->permaffects.end(  ); ++paf )
      {
         affect_data *aff = *paf;

         if( aff->location != APPLY_WEARSPELL && aff->location != APPLY_REMOVESPELL && aff->location != APPLY_STRIPSN )
            ch->affect_modify( aff, true );
      }
   }

   ch->carry_weight = 0;
   ch->carry_number = 0;

   list < obj_data * >::iterator iobj;
   for( iobj = ch->carrying.begin(  ); iobj != ch->carrying.end(  ); ++iobj )
   {
      obj_data *obj = *iobj;

      if( obj->wear_loc == WEAR_NONE )
         ch->carry_number += obj->get_number(  );
      if( !obj->extra_flags.test( ITEM_MAGIC ) )
         ch->carry_weight += obj->get_weight(  );
   }
   ch->re_equip(  );
}

CMDF( do_fixchar )
{
   char_data *victim;

   ch->set_color( AT_IMMORT );

   if( argument.empty(  ) )
   {
      ch->print( "Usage: fixchar <playername>\r\n" );
      return;
   }

   if( !( victim = get_wizvictim( ch, argument, true ) ) )
      return;

   fix_char( victim );
   ch->print( "Done.\r\n" );
}

CMDF( do_immortalize )
{
   char_data *victim;

   ch->set_color( AT_IMMORT );

   if( argument.empty(  ) )
   {
      ch->print( "Syntax:  immortalize <char>\r\n" );
      return;
   }

   if( !( victim = get_wizvictim( ch, argument, true ) ) )
      return;

   /*
    * Added this check, not sure why the code didn't already have it. Samson 1-18-98 
    */
   if( victim->level >= LEVEL_IMMORTAL )
   {
      ch->printf( "Don't be silly, %s is already immortal.\r\n", victim->name );
      return;
   }

   if( victim->level != LEVEL_AVATAR )
   {
      ch->print( "This player is not yet worthy of immortality.\r\n" );
      return;
   }

   ch->print( "Immortalizing a player...\r\n" );
   act( AT_IMMORT, "$n begins to chant softly... then raises $s arms to the sky...", ch, NULL, NULL, TO_ROOM );
   victim->print( "&WYou suddenly feel very strange...\r\n\r\n" );
   interpret( victim, "help M_GODLVL1_" );
   victim->print( "&WYou awake... all your possessions are gone.\r\n" );

   list < obj_data * >::iterator iobj;
   for( iobj = victim->carrying.begin(  ); iobj != victim->carrying.end(  ); )
   {
      obj_data *obj = *iobj;
      ++iobj;

      obj->extract(  );
   }
   victim->level = LEVEL_IMMORTAL;
   advance_level( victim );

   /*
    * Remove clan/guild/order and update accordingly 
    */
   if( victim->pcdata->clan )
   {
      if( victim->speaking == LANG_CLAN )
         victim->speaking = LANG_COMMON;
      victim->unset_lang( LANG_CLAN );
      --victim->pcdata->clan->members;
      if( victim->pcdata->clan->members < 0 )
         victim->pcdata->clan->members = 0;
      if( !str_cmp( victim->name, victim->pcdata->clan->leader ) )
         victim->pcdata->clan->leader.clear(  );
      if( !str_cmp( victim->name, victim->pcdata->clan->number1 ) )
         victim->pcdata->clan->number1.clear(  );
      if( !str_cmp( victim->name, victim->pcdata->clan->number2 ) )
         victim->pcdata->clan->number2.clear(  );
      victim->pcdata->clan = NULL;
      victim->pcdata->clan_name.clear(  );
   }
   victim->exp = exp_level( victim->level );
   victim->trust = 0;

   /*
    * Automatic settings for imms added by Samson 1-18-98 
    */
   fix_char( victim );
   for( int sn = 0; sn < num_skills; ++sn )
      victim->pcdata->learned[sn] = 100;

   victim->set_pcflag( PCFLAG_HOLYLIGHT );
   victim->set_pcflag( PCFLAG_PASSDOOR );
   STRFREE( victim->pcdata->rank );
   victim->save(  );
   make_wizlist(  );
   build_wizinfo(  );
   if( victim->alignment > 350 )
      echo_to_all( "&[immortal]The forces of order grow stronger as another mortal ascends to the heavens!", ECHOTAR_ALL );
   if( victim->alignment <= 350 && victim->alignment >= -350 )
      echo_to_all( "&[immortal]The forces of balance grow stronger as another mortal ascends to the heavens!", ECHOTAR_ALL );
   if( victim->alignment < -350 )
      echo_to_all( "&[immortal]The forces of chaos grow stronger as another mortal ascends to the heavens!", ECHOTAR_ALL );
   /*
    * End added automatic settings 
    */
}

CMDF( do_trust )
{
   string arg1;
   char_data *victim;
   int level;

   ch->set_color( AT_IMMORT );

   argument = one_argument( argument, arg1 );
   if( arg1.empty(  ) || argument.empty(  ) || !is_number( argument ) )
   {
      ch->print( "Syntax: trust <char> <level>.\r\n" );
      return;
   }

   if( !( victim = get_wizvictim( ch, arg1, true ) ) )
      return;

   if( ( level = atoi( argument.c_str(  ) ) ) < 0 || level > MAX_LEVEL )
   {
      ch->printf( "Level must be 0 (reset) or 1 to %d.\r\n", MAX_LEVEL );
      return;
   }

   if( level > ch->get_trust(  ) )
   {
      ch->print( "Limited to your own trust.\r\n" );
      return;
   }

   victim->trust = level;
   ch->print( "Ok.\r\n" );
}

/* Command to silently sneak something into someone's inventory - for immortals only */
CMDF( do_stash )
{
   string arg1, arg2;
   char_data *victim;
   obj_data *obj;

   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );
   if( !str_cmp( arg2, "to" ) && !argument.empty(  ) )
      argument = one_argument( argument, arg2 );

   if( arg1.empty(  ) || arg2.empty(  ) )
   {
      ch->print( "Stash what on whom?\r\n" );
      return;
   }

   if( is_number( arg1 ) )
   {
      /*
       * 'give NNNN coins victim' 
       */
      int amount;

      amount = atoi( arg1.c_str(  ) );
      if( amount <= 0 || ( str_cmp( arg2, "coins" ) && str_cmp( arg2, "coin" ) ) )
      {
         ch->print( "Sorry, you can't do that.\r\n" );
         return;
      }

      argument = one_argument( argument, arg2 );
      if( !str_cmp( arg2, "to" ) && !argument.empty(  ) )
         argument = one_argument( argument, arg2 );
      if( arg2.empty(  ) )
      {
         ch->print( "Stash what on whom?\r\n" );
         return;
      }

      if( !( victim = ch->get_char_world( arg2 ) ) )
      {
         ch->print( "They aren't here.\r\n" );
         return;
      }

      if( victim->isnpc(  ) && !ch->is_imp(  ) )
      {
         ch->print( "Don't stash things on mobs!\r\n" );
         return;
      }

      if( !ch->is_imp(  ) )
      {
         if( ch->gold < amount )
         {
            ch->print( "Very generous of you, but you haven't got that much gold.\r\n" );
            return;
         }
      }

      if( amount < 0 )
      {
         ch->print( "Come on now, don't be an ass!\r\n" );
         return;
      }

      if( !ch->is_imp(  ) )
         ch->gold -= amount;

      victim->gold += amount;

      act( AT_ACTION, "You stash some coins on $N.", ch, NULL, victim, TO_CHAR );

      if( IS_SAVE_FLAG( SV_GIVE ) && !ch->char_died(  ) )
         ch->save(  );

      if( IS_SAVE_FLAG( SV_RECEIVE ) && !victim->char_died(  ) )
         victim->save(  );

      return;
   }

   if( !( obj = ch->get_obj_carry( arg1 ) ) )
   {
      ch->print( "You do not have that item.\r\n" );
      return;
   }

   if( obj->wear_loc != WEAR_NONE )
   {
      ch->print( "You must remove it first.\r\n" );
      return;
   }

   if( !( victim = ch->get_char_world( arg2 ) ) )
   {
      ch->print( "They aren't in the game.\r\n" );
      return;
   }

   if( victim->isnpc(  ) )
   {
      ch->print( "Don't stash things on mobs!\r\n" );
      return;
   }

   if( victim->carry_number + ( obj->get_number(  ) / obj->count ) > victim->can_carry_n(  ) )
   {
      act( AT_PLAIN, "$N has $S hands full.", ch, NULL, victim, TO_CHAR );
      return;
   }

   if( victim->carry_weight + ( obj->get_weight(  ) / obj->count ) > victim->can_carry_w(  ) )
   {
      act( AT_PLAIN, "$N can't carry that much weight.", ch, NULL, victim, TO_CHAR );
      return;
   }

   if( obj->extra_flags.test( ITEM_PROTOTYPE ) && !victim->can_take_proto(  ) )
   {
      act( AT_PLAIN, "You cannot give that to $N!", ch, NULL, victim, TO_CHAR );
      return;
   }

   obj->separate(  );
   obj->from_char(  );

   act( AT_ACTION, "You stash $p on $N.", ch, obj, victim, TO_CHAR );
   obj = obj->to_char( victim );

   if( IS_SAVE_FLAG( SV_GIVE ) && !ch->char_died(  ) )
      ch->save(  );

   if( IS_SAVE_FLAG( SV_RECEIVE ) && !victim->char_died(  ) )
      victim->save(  );
}

/*
 * "Claim" an object.  Will allow an immortal to "grab" an object no matter
 * where it is hiding.  ie: from a player's inventory, from deep inside
 * a container, from a mobile, from anywhere.			-Thoric
 *
 * Yanked from Smaug 1.8
 */
CMDF( do_immget )
{
   string arg, arg1, arg2, arg3, who;
   char_data *vch = NULL;
   bool silently = false, found = false;
   int number, count, vnum;

   number = number_argument( argument, arg );
   argument = arg;
   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );
   argument = one_argument( argument, arg3 );

   if( arg1.empty(  ) )
   {
      ch->print( "Syntax: immget <object> [from who] [silent]\r\n" );
      return;
   }

   if( arg3.empty(  ) )
   {
      if( !arg2.empty(  ) )
      {
         if( !str_cmp( arg2, "silent" ) )
            silently = true;
         else
            who = arg2;
      }
   }
   else
   {
      who = arg2;
      if( !str_cmp( arg3, "silent" ) )
         silently = true;
   }

   if( !who.empty(  ) )
   {
      if( !( vch = ch->get_char_world( who ) ) )
      {
         ch->print( "They aren't here.\r\n" );
         return;
      }
      if( ch->get_trust(  ) < vch->get_trust(  ) && !vch->isnpc(  ) )
      {
         act( AT_TELL, "$n tells you, 'Keep your hands to yourself!'", vch, NULL, ch, TO_VICT );
         return;
      }
   }

   if( is_number( arg1 ) )
      vnum = atoi( arg1.c_str(  ) );
   else
      vnum = -1;

   count = 0;
   obj_data *obj = NULL;
   list < obj_data * >::iterator iobj;
   for( iobj = objlist.begin(  ); iobj != objlist.end(  ); ++iobj )
   {
      obj = *iobj;

      if( ch->can_see_obj( obj, true ) && ( obj->pIndexData->vnum == vnum || hasname( obj->name, arg1 ) ) && ( !vch || vch == obj->who_carrying(  ) ) )
         if( ( count += obj->count ) >= number )
         {
            found = true;
            break;
         }
   }

   if( !found && vnum != -1 )
   {
      ch->print( "You can't find that.\r\n" );
      return;
   }

   count = 0;
   for( iobj = objlist.begin(  ); iobj != objlist.end(  ); ++iobj )
   {
      obj = *iobj;

      if( ch->can_see_obj( obj, true ) && ( obj->pIndexData->vnum == vnum || nifty_is_name_prefix( arg1, obj->name ) ) && ( !vch || vch == obj->who_carrying(  ) ) )
         if( ( count += obj->count ) >= number )
         {
            found = true;
            break;
         }
   }

   if( !found )
   {
      ch->print( "You can't find that.\r\n" );
      return;
   }

   if( !vch && ( vch = obj->who_carrying(  ) ) != NULL )
   {
      if( ch->get_trust(  ) < vch->get_trust(  ) && !vch->isnpc(  ) )
      {
         act( AT_TELL, "$n tells you, 'Keep your hands off $p! It's mine.'", vch, obj, ch, TO_VICT );
         act( AT_IMMORT, "$n tried to lay claim to $p from your possession!", vch, obj, ch, TO_CHAR );
         return;
      }
   }

   obj->separate(  );
   if( obj->item_type == ITEM_PORTAL )
      obj->remove_portal(  );

   if( obj->carried_by )
      obj->from_char(  );
   else if( obj->in_room )
      obj->from_room(  );
   else if( obj->in_obj )
      obj->from_obj(  );

   obj->to_char( ch );
   if( vch )
   {
      if( !silently )
      {
         act( AT_IMMORT, "$n takes $p from you!", ch, obj, vch, TO_VICT );
         act( AT_IMMORT, "$n takes $p from $N!", ch, obj, vch, TO_NOTVICT );
         act( AT_IMMORT, "You take $p from $N!", ch, obj, vch, TO_CHAR );
      }
      else
         act( AT_IMMORT, "You silently take $p from $N.", ch, obj, vch, TO_CHAR );
   }
   else
   {
      if( !silently )
      {
         /*
          * notify people in the room... (not done yet) 
          */
         act( AT_IMMORT, "You take $p!", ch, obj, NULL, TO_CHAR );
      }
      else
         act( AT_IMMORT, "You silently take $p.", ch, obj, NULL, TO_CHAR );
   }
}

room_index *select_random_room( bool pickmap )
{
   room_index *pRoomIndex = NULL;

   // If you need more than 50000 iterations, you have a problem.
   for( int i = 0; i < 50000; ++i )
   {
      pRoomIndex = get_room_index( number_range( 0, sysdata->maxvnum ) );
      if( pRoomIndex )
         if( !pRoomIndex->flags.test( ROOM_PRIVATE )
             && !pRoomIndex->flags.test( ROOM_SOLITARY ) && !pRoomIndex->flags.test( ROOM_PROTOTYPE ) && !pRoomIndex->flags.test( ROOM_ISOLATED ) )
         {
            if( !pickmap && !pRoomIndex->flags.test( ROOM_MAP ) )
               break;
         }
   }
   return pRoomIndex;
}

/* Summer 1997 --Blod */
CMDF( do_scatter )
{
   char_data *victim;
   room_index *pRoomIndex, *from;
   int schance;

   ch->set_color( AT_IMMORT );

   if( argument.empty(  ) )
   {
      ch->print( "Scatter whom?\r\n" );
      return;
   }

   if( !( victim = get_wizvictim( ch, argument, false ) ) )
      return;

   if( victim == ch )
   {
      ch->print( "It's called teleport. Try it.\r\n" );
      return;
   }

   schance = number_range( 1, 2 );

   from = victim->in_room;

   if( schance == 1 || victim->is_immortal(  ) )
   {
      int map, x, y;
      short sector;

      for( ;; )
      {
         map = number_range( 0, MAP_MAX - 1 );
         x = number_range( 0, MAX_X - 1 );
         y = number_range( 0, MAX_Y - 1 );

         sector = get_terrain( map, x, y );
         if( sector == -1 )
            continue;
         if( sect_show[sector].canpass )
            break;
      }
      act( AT_MAGIC, "With the sweep of an arm, $n flings $N to the astral winds.", ch, NULL, victim, TO_NOTVICT );
      act( AT_MAGIC, "With the sweep of an arm, $n flings you to the astral winds.", ch, NULL, victim, TO_VICT );
      act( AT_MAGIC, "With the sweep of an arm, you fling $N to the astral winds.", ch, NULL, victim, TO_CHAR );
      enter_map( victim, NULL, x, y, map );
      collect_followers( victim, from, victim->in_room );
      victim->position = POS_STANDING;
      act( AT_MAGIC, "$n is deposited in a heap by the astral winds.", victim, NULL, NULL, TO_ROOM );
   }
   else
   {
      if( !( pRoomIndex = select_random_room( true ) ) )
      {
         ch->print( "&[immortal]No room selected. Scatter command cancelled." );
         return;
      }

      if( victim->fighting )
         victim->stop_fighting( true );
      act( AT_MAGIC, "With the sweep of an arm, $n flings $N to the astral winds.", ch, NULL, victim, TO_NOTVICT );
      act( AT_MAGIC, "With the sweep of an arm, $n flings you to the astral winds.", ch, NULL, victim, TO_VICT );
      act( AT_MAGIC, "With the sweep of an arm, you fling $N to the astral winds.", ch, NULL, victim, TO_CHAR );
      leave_map( victim, NULL, pRoomIndex );
      victim->position = POS_RESTING;
      act( AT_MAGIC, "$n is deposited in a heap by the astral winds.", victim, NULL, NULL, TO_ROOM );
   }
}

CMDF( do_strew )
{
   string arg1;
   char_data *victim;
   room_index *pRoomIndex;

   ch->set_color( AT_IMMORT );

   argument = one_argument( argument, arg1 );
   if( arg1.empty(  ) || argument.empty(  ) )
   {
      ch->print( "Strew who, what?\r\n" );
      return;
   }

   if( !( victim = get_wizvictim( ch, arg1, false ) ) )
      return;

   if( victim == ch )
   {
      ch->print( "Try taking it out on someone else first.\r\n" );
      return;
   }

   if( !str_cmp( argument, "coins" ) )
   {
      if( victim->gold < 1 )
      {
         ch->print( "Drat, this one's got no gold to start with.\r\n" );
         return;
      }
      victim->gold = 0;
      act( AT_MAGIC, "$n gestures and an unearthly gale sends $N's coins flying!", ch, NULL, victim, TO_NOTVICT );
      act( AT_MAGIC, "You gesture and an unearthly gale sends $N's coins flying!", ch, NULL, victim, TO_CHAR );
      act( AT_MAGIC, "As $n gestures, an unearthly gale sends your currency flying!", ch, NULL, victim, TO_VICT );
      return;
   }

   if( !( pRoomIndex = select_random_room( false ) ) )
   {
      ch->print( "&[immortal]No room selected. Strew command cancelled." );
      return;
   }

   if( !str_cmp( argument, "inventory" ) )
   {
      act( AT_MAGIC, "$n speaks a single word, sending $N's possessions flying!", ch, NULL, victim, TO_NOTVICT );
      act( AT_MAGIC, "You speak a single word, sending $N's possessions flying!", ch, NULL, victim, TO_CHAR );
      act( AT_MAGIC, "$n speaks a single word, sending your possessions flying!", ch, NULL, victim, TO_VICT );

      list < obj_data * >::iterator iobj;
      for( iobj = victim->carrying.begin(  ); iobj != victim->carrying.end(  ); )
      {
         obj_data *obj_lose = *iobj;
         ++iobj;

         obj_lose->from_char(  );
         obj_lose->to_room( pRoomIndex, NULL );
         ch->pagerf( "\t&w%s sent to %d\r\n", capitalize( obj_lose->short_descr ), pRoomIndex->vnum );
      }
      return;
   }
   ch->print( "Strew their coins or inventory?\r\n" );
}

CMDF( do_strip )
{
   char_data *victim;

   if( argument.empty(  ) )
   {
      ch->print( "Strip who?\r\n" );
      return;
   }

   ch->set_color( AT_OBJECT );

   if( !( victim = get_wizvictim( ch, argument, false ) ) )
      return;

   if( victim == ch )
   {
      ch->print( "Kinky.\r\n" );
      return;
   }
   act( AT_OBJECT, "Searching $N ...", ch, NULL, victim, TO_CHAR );

   int count = 0;
   list < obj_data * >::iterator iobj;
   for( iobj = victim->carrying.begin(  ); iobj != victim->carrying.end(  ); )
   {
      obj_data *obj_lose = *iobj;
      ++iobj;

      obj_lose->from_char(  );
      obj_lose->to_char( ch );
      ch->pagerf( "  &G... %s (&g%s) &Gtaken.\r\n", capitalize( obj_lose->short_descr ), obj_lose->name );
      ++count;
   }
   if( !count )
      ch->pager( "&GNothing found to take.\r\n" );
}

const int RESTORE_INTERVAL = 21600;
CMDF( do_restore )
{
   ch->set_color( AT_IMMORT );

   if( argument.empty(  ) )
   {
      ch->print( "Restore whom?\r\n" );
      return;
   }

   if( !str_cmp( argument, "all" ) )
   {
      if( !ch->pcdata )
         return;

      if( ch->get_trust(  ) < LEVEL_SUB_IMPLEM )
      {
         if( ch->isnpc(  ) )
         {
            ch->print( "You can't do that.\r\n" );
            return;
         }
         else
         {
            /*
             * Check if the player did a restore all within the last 18 hours. 
             */
            if( !ch->is_imp(  ) && current_time - last_restore_all_time < RESTORE_INTERVAL )
            {
               ch->print( "Sorry, you can't do a restore all yet.\r\n" );
               interpret( ch, "restoretime" );
               return;
            }
         }
      }
      last_restore_all_time = current_time;
      ch->pcdata->restore_time = current_time;
      ch->save(  );
      ch->print( "Ok.\r\n" );
      list < char_data * >::iterator ich;
      for( ich = pclist.begin(  ); ich != pclist.end(  ); )
      {
         char_data *vch = *ich;
         ++ich;

         if( !vch->is_immortal(  ) && !vch->CAN_PKILL(  ) && !in_arena( vch ) )
         {
            vch->hit = vch->max_hit;
            vch->mana = vch->max_mana;
            vch->move = vch->max_move;
            if( vch->pcdata->condition[COND_FULL] != -1 )
               vch->pcdata->condition[COND_FULL] = sysdata->maxcondval;
            if( vch->pcdata->condition[COND_THIRST] != -1 )
               vch->pcdata->condition[COND_THIRST] = sysdata->maxcondval;
            vch->pcdata->condition[COND_DRUNK] = 0;
            vch->mental_state = 0;
            vch->update_pos(  );
            act( AT_IMMORT, "$n has restored you.", ch, NULL, vch, TO_VICT );
         }
      }
   }
   else
   {
      char_data *victim;

      if( !( victim = ch->get_char_world( argument ) ) )
      {
         ch->print( "They aren't here.\r\n" );
         return;
      }

      if( ch->get_trust(  ) < LEVEL_LESSER && victim != ch && !( victim->has_actflag( ACT_PROTOTYPE ) ) )
      {
         ch->print( "You can't do that.\r\n" );
         return;
      }
      victim->hit = victim->max_hit;
      victim->mana = victim->max_mana;
      victim->move = victim->max_move;
      if( !victim->isnpc(  ) )
      {
         if( victim->pcdata->condition[COND_FULL] != -1 )
            victim->pcdata->condition[COND_FULL] = sysdata->maxcondval;
         if( victim->pcdata->condition[COND_THIRST] != -1 )
            victim->pcdata->condition[COND_THIRST] = sysdata->maxcondval;
         victim->pcdata->condition[COND_DRUNK] = 0;
         victim->mental_state = 0;
      }
      victim->update_pos(  );
      if( ch != victim )
         act( AT_IMMORT, "$n has restored you.", ch, NULL, victim, TO_VICT );
      ch->print( "Ok.\r\n" );
   }
}

CMDF( do_restoretime )
{
   long int time_passed;
   int hour, minute;

   ch->set_color( AT_IMMORT );

   if( !last_restore_all_time )
      ch->print( "There has been no restore all since reboot.\r\n" );
   else
   {
      time_passed = current_time - last_restore_all_time;
      hour = ( int )( time_passed / 3600 );
      minute = ( int )( ( time_passed - ( hour * 3600 ) ) / 60 );
      ch->printf( "The  last restore all was %d hours and %d minutes ago.\r\n", hour, minute );
   }

   if( !ch->pcdata )
      return;

   if( !ch->pcdata->restore_time )
   {
      ch->print( "You have never done a restore all.\r\n" );
      return;
   }

   time_passed = current_time - ch->pcdata->restore_time;
   hour = ( int )( time_passed / 3600 );
   minute = ( int )( ( time_passed - ( hour * 3600 ) ) / 60 );
   ch->printf( "Your last restore all was %d hours and %d minutes ago.\r\n", hour, minute );
}

CMDF( do_freeze )
{
   char_data *victim;

   if( argument.empty(  ) )
   {
      ch->print( "Freeze whom?\r\n" );
      return;
   }

   if( !( victim = get_wizvictim( ch, argument, true ) ) )
      return;

   if( victim->desc && victim->desc->original && victim->desc->original->get_trust(  ) >= ch->get_trust(  ) )
   {
      ch->print( "For some inexplicable reason, you failed.\r\n" );
      return;
   }

   if( victim->has_pcflag( PCFLAG_FREEZE ) )
   {
      victim->unset_pcflag( PCFLAG_FREEZE );
      victim->print( "&CYour frozen form suddenly thaws.\r\n" );
      ch->printf( "&C%s is now unfrozen.\r\n", victim->name );
   }
   else
   {
      if( victim->switched )
         do_return( victim->switched, "" );
      victim->set_pcflag( PCFLAG_FREEZE );
      victim->print( "&CA godly force turns your body to ice!\r\n" );
      ch->printf( "&CYou have frozen %s.\r\n", victim->name );
   }
   victim->save(  );
}

CMDF( do_log )
{
   char_data *victim;

   ch->set_color( AT_IMMORT );

   if( argument.empty(  ) )
   {
      ch->print( "Log whom?\r\n" );
      return;
   }

   if( ch->is_imp(  ) && !str_cmp( argument, "all" ) )
   {
      if( fLogAll )
      {
         fLogAll = false;
         ch->print( "Log ALL off.\r\n" );
      }
      else
      {
         fLogAll = true;
         ch->print( "Log ALL on.\r\n" );
      }
      return;
   }

   if( !( victim = get_wizvictim( ch, argument, true ) ) )
      return;

   if( victim->has_pcflag( PCFLAG_LOG ) )
   {
      victim->unset_pcflag( PCFLAG_LOG );
      ch->printf( "LOG removed from %s.\r\n", victim->name );
   }
   else
   {
      victim->set_pcflag( PCFLAG_LOG );
      ch->printf( "LOG applied to %s.\r\n", victim->name );
   }
}

CMDF( do_litterbug )
{
   char_data *victim;

   ch->set_color( AT_IMMORT );

   if( argument.empty(  ) )
   {
      ch->print( "Set litterbug flag on whom?\r\n" );
      return;
   }

   if( !( victim = get_wizvictim( ch, argument, true ) ) )
      return;

   if( victim->has_pcflag( PCFLAG_LITTERBUG ) )
   {
      victim->unset_pcflag( PCFLAG_LITTERBUG );
      victim->print( "You can drop items again.\r\n" );
      ch->printf( "LITTERBUG removed from %s.\r\n", victim->name );
   }
   else
   {
      victim->set_pcflag( PCFLAG_LITTERBUG );
      victim->print( "A strange force prevents you from dropping any more items!\r\n" );
      ch->printf( "LITTERBUG set on %s.\r\n", victim->name );
   }
}

CMDF( do_noemote )
{
   char_data *victim;

   ch->set_color( AT_IMMORT );

   if( argument.empty(  ) )
   {
      ch->print( "Noemote whom?\r\n" );
      return;
   }

   if( !( victim = get_wizvictim( ch, argument, true ) ) )
      return;

   if( victim->has_pcflag( PCFLAG_NO_EMOTE ) )
   {
      victim->unset_pcflag( PCFLAG_NO_EMOTE );
      victim->print( "You can emote again.\r\n" );
      ch->printf( "NOEMOTE removed from %s.\r\n", victim->name );
   }
   else
   {
      victim->set_pcflag( PCFLAG_NO_EMOTE );
      victim->print( "You can't emote!\r\n" );
      ch->printf( "NOEMOTE applied to %s.\r\n", victim->name );
   }
}

CMDF( do_notell )
{
   char_data *victim;

   ch->set_color( AT_IMMORT );

   if( argument.empty(  ) )
   {
      ch->print( "Notell whom?\r\n" );
      return;
   }

   if( !( victim = get_wizvictim( ch, argument, true ) ) )
      return;

   if( victim->has_pcflag( PCFLAG_NO_TELL ) )
   {
      victim->unset_pcflag( PCFLAG_NO_TELL );
      victim->print( "You can send tells again.\r\n" );
      ch->printf( "NOTELL removed from %s.\r\n", victim->name );
   }
   else
   {
      victim->set_pcflag( PCFLAG_NO_TELL );
      victim->print( "You can't send tells!\r\n" );
      ch->printf( "NOTELL applied to %s.\r\n", victim->name );
   }
}

CMDF( do_notitle )
{
   char_data *victim;
   char buf[MSL];

   ch->set_color( AT_IMMORT );

   if( argument.empty(  ) )
   {
      ch->print( "Notitle whom?\r\n" );
      return;
   }

   if( !( victim = get_wizvictim( ch, argument, true ) ) )
      return;

   if( victim->has_pcflag( PCFLAG_NOTITLE ) )
   {
      victim->unset_pcflag( PCFLAG_NOTITLE );
      victim->print( "You can set your own title again.\r\n" );
      ch->printf( "NOTITLE removed from %s.\r\n", victim->name );
   }
   else
   {
      victim->set_pcflag( PCFLAG_NOTITLE );
      snprintf( buf, MSL, "the %s", title_table[victim->Class][victim->level][victim->sex] );
      victim->set_title( buf );
      victim->print( "You can't set your own title!\r\n" );
      ch->printf( "NOTITLE set on %s.\r\n", victim->name );
   }
}

CMDF( do_nourl )
{
   char_data *victim;

   ch->set_color( AT_IMMORT );

   if( argument.empty(  ) )
   {
      ch->print( "NoURL whom?\r\n" );
      return;
   }

   if( !( victim = get_wizvictim( ch, argument, true ) ) )
      return;

   if( victim->has_pcflag( PCFLAG_NO_URL ) )
   {
      victim->unset_pcflag( PCFLAG_NO_URL );
      victim->print( "You can set a homepage again.\r\n" );
      ch->printf( "NOURL removed from %s.\r\n", victim->name );
   }
   else
   {
      victim->set_pcflag( PCFLAG_NO_URL );
      victim->pcdata->homepage.clear();
      victim->print( "You can't set a homepage!\r\n" );
      ch->printf( "NOURL applied to %s.\r\n", victim->name );
   }
}

CMDF( do_noemail )
{
   char_data *victim;

   ch->set_color( AT_IMMORT );

   if( argument.empty(  ) )
   {
      ch->print( "NoEmail whom?\r\n" );
      return;
   }

   if( !( victim = get_wizvictim( ch, argument, true ) ) )
      return;

   if( victim->has_pcflag( PCFLAG_NO_EMAIL ) )
   {
      victim->unset_pcflag( PCFLAG_NO_EMAIL );
      victim->print( "You can set an email address again.\r\n" );
      ch->printf( "NOEMAIL removed from %s.\r\n", victim->name );
   }
   else
   {
      victim->set_pcflag( PCFLAG_NO_EMAIL );
      victim->pcdata->email.clear();
      victim->print( "You can't set an email address!\r\n" );
      ch->printf( "NOEMAIL applied to %s.\r\n", victim->name );
   }
}

CMDF( do_silence )
{
   char_data *victim;

   ch->set_color( AT_IMMORT );

   if( argument.empty(  ) )
   {
      ch->print( "Silence whom?\r\n" );
      return;
   }

   if( !( victim = get_wizvictim( ch, argument, true ) ) )
      return;

   if( victim->has_pcflag( PCFLAG_SILENCE ) )
   {
      victim->unset_pcflag( PCFLAG_SILENCE );
      victim->print( "You can use channels again.\r\n" );
      ch->printf( "SILENCE removed from %s.\r\n", victim->name );
   }
   else
   {
      victim->set_pcflag( PCFLAG_SILENCE );
      victim->print( "You can't use channels!\r\n" );
      ch->printf( "You SILENCE %s.\r\n", victim->name );
   }
}

CMDF( do_peace )
{
   act( AT_IMMORT, "$n booms, 'PEACE!'", ch, NULL, NULL, TO_ROOM );
   act( AT_IMMORT, "You boom, 'PEACE!'", ch, NULL, NULL, TO_CHAR );

   list < char_data * >::iterator ich;
   for( ich = ch->in_room->people.begin(  ); ich != ch->in_room->people.end(  ); ++ich )
   {
      char_data *rch = *ich;

      if( rch->fighting )
      {
         rch->stop_fighting( true );
         interpret( rch, "sit" );
      }

      /*
       * Added by Narn, Nov 28/95 
       */
      stop_hating( rch );
      stop_hunting( rch );
      stop_fearing( rch );
   }
   ch->print( "&YOk.\r\n" );
}

CMDF( do_lockdown )
{
   sysdata->LOCKDOWN = !sysdata->LOCKDOWN;

   save_sysdata(  );

   if( sysdata->LOCKDOWN )
      echo_to_all( "&RTotal lockdown activated.", ECHOTAR_IMM );
   else
      echo_to_all( "&RTotal lockdown deactivated.", ECHOTAR_IMM );
}

CMDF( do_implock )
{
   sysdata->IMPLOCK = !sysdata->IMPLOCK;

   save_sysdata(  );

   if( sysdata->IMPLOCK )
      echo_to_all( "&RImplock activated.", ECHOTAR_IMM );
   else
      echo_to_all( "&RImplock deactivated.", ECHOTAR_IMM );
}

CMDF( do_wizlock )
{
   sysdata->WIZLOCK = !sysdata->WIZLOCK;

   save_sysdata(  );

   if( sysdata->WIZLOCK )
      echo_to_all( "&RWizlock activated.", ECHOTAR_IMM );
   else
      echo_to_all( "&RWizlock deactivated.", ECHOTAR_IMM );
}

CMDF( do_noresolve )
{
   sysdata->NO_NAME_RESOLVING = !sysdata->NO_NAME_RESOLVING;

   save_sysdata(  );

   if( sysdata->NO_NAME_RESOLVING )
      ch->print( "&YName resolving disabled.\r\n" );
   else
      ch->print( "&YName resolving enabled.\r\n" );
}

/* Output of command reformmated by Samson 2-8-98, and again on 4-7-98 */
CMDF( do_users )
{
   ch->set_pager_color( AT_PLAIN );

   ch->pager( "Desc|     Constate      |Idle|    Player    | HostIP                   \r\n" );
   ch->pager( "----+-------------------+----+--------------+--------------------------\r\n" );

   int count = 0;
   list < descriptor_data * >::iterator ds;
   for( ds = dlist.begin(  ); ds != dlist.end(  ); ++ds )
   {
      descriptor_data *d = *ds;
      const char *st;

      switch ( d->connected )
      {
         case CON_PLAYING:
            st = "Playing";
            break;
         case CON_GET_NAME:
            st = "Get name";
            break;
         case CON_GET_OLD_PASSWORD:
            st = "Get password";
            break;
         case CON_CONFIRM_NEW_NAME:
            st = "Confirm name";
            break;
         case CON_GET_NEW_PASSWORD:
            st = "New password";
            break;
         case CON_CONFIRM_NEW_PASSWORD:
            st = "Confirm password";
            break;
         case CON_GET_NEW_SEX:
            st = "Get sex";
            break;
         case CON_READ_MOTD:
            st = "Reading MOTD";
            break;
         case CON_EDITING:
            st = "In line editor";
            break;
         case CON_PRESS_ENTER:
            st = "Press enter";
            break;
         case CON_PRIZENAME:
            st = "Sindhae prizename";
            break;
         case CON_CONFIRMPRIZENAME:
            st = "Confirming prize";
            break;
         case CON_PRIZEKEY:
            st = "Sindhae keywords";
            break;
         case CON_CONFIRMPRIZEKEY:
            st = "Confirming keywords";
            break;
         case CON_ROLL_STATS:
            st = "Rolling stats";
            break;
         case CON_RAISE_STAT:
            st = "Raising stats";
            break;
         case CON_DELETE:
            st = "Confirm delete";
            break;
         case CON_FORKED:
            st = "Command shell";
            break;
         case CON_GET_PORT_PASSWORD:
            st = "Access code";
            break;
         case CON_COPYOVER_RECOVER:
            st = "Hotboot";
            break;
         case CON_PLOADED:
            st = "Ploaded";
            break;
         case CON_MEDIT:
         case CON_OEDIT:
         case CON_REDIT:
            st = "Using OLC";
            break;
         default:
            st = "Invalid!!!!";
            break;
      }

      if( argument.empty(  ) )
      {
         if( ch->is_imp(  ) || ( d->character && ch->can_see( d->character, true ) && !is_ignoring( d->character, ch ) ) )
         {
            ++count;
            ch->pagerf( " %3d| %-17s |%4d| %-12s | %s \r\n", d->descriptor, st, d->idle / 4,
                        d->original ? d->original->name : d->character ? d->character->name : "(None!)", d->host.c_str(  ) );
         }
      }
      else
      {
         if( ( ch->get_trust(  ) >= LEVEL_SUPREME || ( d->character && ch->can_see( d->character, true ) ) )
             && ( !str_prefix( argument, d->host ) || ( d->character && !str_prefix( argument, d->character->name ) ) ) )
         {
            ++count;
            ch->pagerf( " %3d| %2d|%4d| %-12s | %s \r\n", d->descriptor, d->connected, d->idle / 4,
                        d->original ? d->original->name : d->character ? d->character->name : "(None!)", d->host.c_str(  ) );
         }
      }
   }
   ch->pagerf( "%d user%s.\r\n", count, count == 1 ? "" : "s" );
}

CMDF( do_invis )
{
   string arg;
   short level;

   ch->set_color( AT_IMMORT );

   argument = one_argument( argument, arg );
   if( !arg.empty(  ) )
   {
      if( !is_number( arg ) )
      {
         ch->print( "Usage: invis | invis <level>\r\n" );
         return;
      }
      level = atoi( arg.c_str(  ) );
      if( level < 2 || level > ch->get_trust(  ) )
      {
         ch->print( "Invalid level.\r\n" );
         return;
      }

      if( !ch->isnpc(  ) )
      {
         ch->pcdata->wizinvis = level;
         ch->printf( "Wizinvis level set to %d.\r\n", level );
      }

      else
      {
         ch->mobinvis = level;
         ch->printf( "Mobinvis level set to %d.\r\n", level );
      }
      return;
   }

   if( !ch->isnpc(  ) )
   {
      if( ch->pcdata->wizinvis < 2 )
         ch->pcdata->wizinvis = ch->level;
   }
   else
   {
      if( ch->mobinvis < 2 )
         ch->mobinvis = ch->level;
   }

   if( !ch->isnpc(  ) )
   {
      ch->save(  );
      if( ch->has_pcflag( PCFLAG_WIZINVIS ) )
      {
         ch->unset_pcflag( PCFLAG_WIZINVIS );
         act( AT_IMMORT, "$n slowly fades into existence.", ch, NULL, NULL, TO_ROOM );
         ch->print( "You slowly fade back into existence.\r\n" );
      }
      else
      {
         act( AT_IMMORT, "$n slowly fades into thin air.", ch, NULL, NULL, TO_ROOM );
         ch->print( "You slowly vanish into thin air.\r\n" );
         ch->set_pcflag( PCFLAG_WIZINVIS );
      }
   }
   else
   {
      if( ch->has_actflag( ACT_MOBINVIS ) )
      {
         ch->unset_actflag( ACT_MOBINVIS );
         act( AT_IMMORT, "$n slowly fades into existence.", ch, NULL, NULL, TO_ROOM );
         ch->print( "You slowly fade back into existence.\r\n" );
      }
      else
      {
         act( AT_IMMORT, "$n slowly fades into thin air.", ch, NULL, NULL, TO_ROOM );
         ch->print( "You slowly vanish into thin air.\r\n" );
         ch->set_actflag( ACT_MOBINVIS );
      }
   }
}

CMDF( do_holylight )
{
   ch->set_color( AT_IMMORT );

   if( ch->isnpc(  ) )
      return;

   ch->toggle_pcflag( PCFLAG_HOLYLIGHT );

   if( ch->has_pcflag( PCFLAG_HOLYLIGHT ) )
      ch->print( "Holy light mode on.\r\n" );
   else
      ch->print( "Holy light mode off.\r\n" );
}

/*
 * Load up a player file
 */
CMDF( do_loadup )
{
   descriptor_data *d;
   char fname[256];
   struct stat fst;
   int old_room_vnum;

   ch->set_color( AT_IMMORT );

   if( argument.empty(  ) )
   {
      ch->print( "Usage: loadup <playername>\r\n" );
      return;
   }

   list < descriptor_data * >::iterator ds;
   for( ds = dlist.begin(  ); ds != dlist.end(  ); ++ds )
   {
      d = *ds;
      char_data *temp = d->original ? d->original : d->character;

      if( !str_cmp( temp->name, argument ) )
      {
         ch->print( "They are already playing.\r\n" );
         return;
      }
   }
   d = NULL;
   argument[0] = UPPER( argument[0] );
   snprintf( fname, 256, "%s%c/%s", PLAYER_DIR, tolower( argument[0] ), capitalize( argument ).c_str(  ) );

   if( stat( fname, &fst ) == -1 || !check_parse_name( capitalize( argument ).c_str(  ), false ) )
   {
      ch->print( "&YNo such player exists.\r\n" );
      return;
   }

   if( stat( fname, &fst ) != -1 )
   {
      d = new descriptor_data;
      d->init(  );
      d->connected = CON_PLOADED;

      load_char_obj( d, argument, false, false );
      charlist.push_back( d->character );
      pclist.push_back( d->character );
      if( !d->character->to_room( ch->in_room ) )
         log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
      old_room_vnum = d->character->in_room->vnum;
      if( d->character->get_trust(  ) >= ch->get_trust(  ) )
      {
         interpret( d->character, "say How dare you load my Pfile!" );
         cmdf( d->character, "dino %s", ch->name );
         ch->printf( "I think you'd better leave %s alone!\r\n", argument.c_str(  ) );
         d->character->desc = NULL;
         interpret( d->character, "quit auto" );
         return;
      }
      d->character->desc = NULL;
      d->character = NULL;
      deleteptr( d );
      ch->printf( "&R%s loaded from room %d.\r\n", capitalize( argument ).c_str(  ), old_room_vnum );
      act_printf( AT_IMMORT, ch, NULL, NULL, TO_ROOM, "%s appears from nowhere, eyes glazed over.", capitalize( argument ).c_str(  ) );
      ch->print( "Done.\r\n" );
      return;
   }
   /*
    * else no player file 
    */
   ch->print( "No such player.\r\n" );
}

/*
 * Extract area names from "input" string and place result in "output" string
 * e.g. "aset joe.are sedit susan.are cset" --> "joe.are susan.are"
 * - Gorog
 */
 /*
  * Rewrite by Xorith. 12/1/03
  */
string extract_area_names( char_data * ch )
{
   string tbuf, tarea, area_names;

   if( !ch || ch->isnpc(  ) )
      return NULL;

   if( ch->pcdata->bestowments.empty(  ) )
      return NULL;

   tbuf = ch->pcdata->bestowments;
   tbuf = one_argument( tbuf, tarea );
   if( tarea.empty(  ) )
      return NULL;

   while( !tbuf.empty(  ) )
   {
      if( tarea.find( ".are" ) != string::npos )
      {
         if( area_names.empty(  ) )
            area_names.append( tarea );
         else
            area_names.append( tarea + " " );
      }
      tbuf = one_argument( tbuf, tarea );
   }
   return area_names;
}

/*
 * Remove area names from "input" string and place result in "output" string
 * e.g. "aset joe.are sedit susan.are cset" --> "aset sedit cset"
 * - Gorog
 */
void remove_area_names( char_data * ch )
{
   string buf, tarea;

   if( !ch || ch->isnpc(  ) || ch->pcdata->bestowments.empty(  ) )
      return;

   buf = ch->pcdata->bestowments;
   buf = one_argument( buf, tarea );
   if( tarea.empty(  ) )
      return;

   while( !buf.empty(  ) )
   {
      if( tarea.find( ".are" ) != string::npos )
         removename( ch->pcdata->bestowments, tarea );
      buf = one_argument( buf, tarea );
   }
}

/*
 * Allows members of the Area Council to add Area names to the bestow field.
 * Area names mus end with ".are" so that no commands can be bestowed.
 */
/* Function revamped by Xorith on 12/1/03 */
CMDF( do_bestowarea )
{
   string arg, buf;
   char_data *victim;

   ch->set_color( AT_IMMORT );

#ifdef MULTIPORT
   if( mud_port != BUILDPORT && !ch->is_imp(  ) )
   {
      ch->print( "Only an implementor may bestow an area on this port.\r\n" );
      return;
   }
#endif

   if( argument.empty(  ) )
   {
      ch->print( "Syntax:\r\n"
                 "bestowarea <victim> <filename>.are\r\n"
                 "bestowarea <victim> none                     removes bestowed areas\r\n"
                 "bestowarea <victim> remove <filename>.are    removes a specific area\r\n"
                 "bestowarea <victim> list                     lists bestowed areas\r\n"
                 "bestowarea <victim>                          lists bestowed areas\r\n" );
      return;
   }

   argument = one_argument( argument, arg );

   if( !( victim = get_wizvictim( ch, arg, true ) ) )
      return;

   if( victim->get_trust(  ) < LEVEL_IMMORTAL )
   {
      ch->printf( "%s is not an immortal and may not have an area bestowed.\r\n", victim->name );
      return;
   }

   if( argument.empty(  ) || !str_cmp( argument, "list" ) )
   {
      if( victim->pcdata->bestowments.empty(  ) )
      {
         ch->printf( "%s does not have any areas bestowed upon them.\r\n", victim->name );
         return;
      }
      buf = extract_area_names( victim );
      if( buf.empty(  ) )
         ch->printf( "%s does not have any areas bestowed upon them.\r\n", victim->name );
      else
         ch->printf( "%s's bestowed areas: %s\r\n", victim->name, buf.c_str(  ) );
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
            ch->printf( "%s does not have an area named %s bestowed.\r\n", victim->name, arg.c_str(  ) );
            return;
         }
         removename( victim->pcdata->bestowments, arg );
         ch->printf( "Removed area %s from %s.\r\n", arg.c_str(  ), victim->name );
         victim->save(  );
      }
      return;
   }

   if( !str_cmp( arg, "none" ) )
   {
      if( victim->pcdata->bestowments.empty(  ) || victim->pcdata->bestowments.find( ".are" ) == string::npos )
      {
         ch->printf( "%s has no areas bestowed!\r\n", victim->name );
         return;
      }
      remove_area_names( victim );
      victim->save(  );
      ch->printf( "All of %s's bestowed areas have been removed.\r\n", victim->name );
      return;
   }

   while( !argument.empty(  ) )
   {
      if( arg.find( ".are" ) == string::npos )
      {
         ch->printf( "'%s' is not a valid area to bestow.\r\n", arg.c_str(  ) );
         ch->print( "You can only bestow an area name\r\n" );
         ch->print( "E.G. bestow joe sam.are\r\n" );
         return;
      }

      if( hasname( victim->pcdata->bestowments, arg ) )
      {
         ch->printf( "%s already has the area %s bestowed.\r\n", victim->name, arg.c_str(  ) );
         return;
      }

      smash_tilde( arg );
      addname( victim->pcdata->bestowments, arg );
      victim->printf( "%s has bestowed on you the area: %s\r\n", ch->name, arg.c_str(  ) );
      ch->printf( "%s has been bestowed: %s\r\n", victim->name, arg.c_str(  ) );
      victim->save(  );

      argument = one_argument( argument, arg );
   }
}

CMDF( do_demote )
{
   char_data *victim;

   ch->set_color( AT_IMMORT );
   if( argument.empty(  ) )
   {
      ch->print( "Syntax: demote <char>.\r\n" );
      return;
   }

   if( !( victim = get_wizvictim( ch, argument, true ) ) )
      return;

   if( victim == ch )
   {
      ch->print( "Why on earth would you want to do that?\r\n" );
      return;
   }

   if( victim->level < LEVEL_IMMORTAL )
   {
      ch->printf( "%s is not immortal, and cannot be demoted. Use the advance command.\r\n", victim->name );
      return;
   }

   ch->print( "Demoting an immortal!\r\n" );
   victim->print( "You have been demoted!\r\n" );

   victim->level -= 1;

   /*
    * Rank fix added by Narn. 
    */
   STRFREE( victim->pcdata->rank );

   advance_level( victim );
   victim->exp = exp_level( victim->level );
   victim->trust = 0;

   /*
    * Stuff added to make sure players wizinvis level doesnt stay higher 
    * * than their actual level and to take wizinvis away from advance below 100
    */
   if( victim->has_pcflag( PCFLAG_WIZINVIS ) )
      victim->pcdata->wizinvis = victim->level;

   if( victim->has_pcflag( PCFLAG_WIZINVIS ) && ( victim->level <= LEVEL_AVATAR ) )
   {
      victim->unset_pcflag( PCFLAG_WIZINVIS );
      victim->pcdata->wizinvis = 0;
   }

   if( victim->level == LEVEL_AVATAR )
   {
      char buf[256], buf2[256];
      char *die;

      victim->unset_pcflag( PCFLAG_HOLYLIGHT );
      die = victim->name;
      snprintf( buf, 256, "%s%s", GOD_DIR, capitalize( die ) );
      if( !remove( buf ) )
         ch->print( "Player's immortal data destroyed.\r\n" );
      snprintf( buf2, 256, "%s.are", capitalize( die ) );
      victim->print( "You have been thrown from the heavens by the Gods!\r\nYou are no longer immortal!\r\n" );
      victim->unset_pcflag( PCFLAG_PASSDOOR );
      victim->pcdata->realm = 0;
   }
   funcf( ch, do_bestow, "%s none", victim->name );
   victim->save(  );
   make_wizlist(  );
   build_wizinfo(  );
#ifdef IMC
   imc_check_wizperms( victim );
#endif
}

CMDF( do_promote )
{
   char_data *victim;
   int level;

   ch->set_color( AT_IMMORT );
   if( argument.empty(  ) )
   {
      ch->print( "Syntax: promote <char>.\r\n" );
      return;
   }

   if( !( victim = get_wizvictim( ch, argument, true ) ) )
      return;

   if( victim == ch )
   {
      ch->print( "Nice try :P\r\n" );
      return;
   }

   if( victim->level < LEVEL_AVATAR )
   {
      ch->printf( "%s is not immortal, and cannot be promoted. Use the advance command.\r\n", victim->name );
      return;
   }

   if( victim->level == LEVEL_AVATAR )
   {
      ch->printf( "You must use the immortalize command to make %s an immortal.\r\n", victim->name );
      return;
   }

   level = victim->level + 1;
   if( level > MAX_LEVEL )
   {
      ch->printf( "Cannot promote %s past the max level of %d.\r\n", victim->name, MAX_LEVEL );
      return;
   }

   act( AT_IMMORT, "You make some arcane gestures with your hands, then point a finger at $N!", ch, NULL, victim, TO_CHAR );
   act( AT_IMMORT, "$n makes some arcane gestures with $s hands, then points $s finger at you!", ch, NULL, victim, TO_VICT );
   act( AT_IMMORT, "$n makes some arcane gestures with $s hands, then points $s finger at $N!", ch, NULL, victim, TO_NOTVICT );
   victim->set_color( AT_WHITE );
   victim->print( "&WYou suddenly feel very strange...\r\n\r\n" );

   switch ( level )
   {
      default:
         victim->print( "You have been promoted!\r\n" );
         break;

      case LEVEL_ACOLYTE:
         do_help( victim, "M_GODLVL1_" );
         break;
      case LEVEL_CREATOR:
         do_help( victim, "M_GODLVL2_" );
         break;
      case LEVEL_SAVIOR:
         do_help( victim, "M_GODLVL3_" );
         break;
      case LEVEL_DEMI:
         do_help( victim, "M_GODLVL4_" );
         break;
      case LEVEL_TRUEIMM:
         do_help( victim, "M_GODLVL5_" );
         break;
      case LEVEL_LESSER:
         do_help( victim, "M_GODLVL6_" );
         break;
      case LEVEL_GOD:
         do_help( victim, "M_GODLVL7_" );
         break;
      case LEVEL_GREATER:
         do_help( victim, "M_GODLVL8_" );
         break;
      case LEVEL_ASCENDANT:
         do_help( victim, "M_GODLVL9_" );
         break;
      case LEVEL_SUB_IMPLEM:
         do_help( victim, "M_GODLVL10_" );
         break;
      case LEVEL_IMPLEMENTOR:
         do_help( victim, "M_GODLVL11_" );
         break;
      case LEVEL_KL:
         do_help( victim, "M_GODLVL12_" );
         break;
      case LEVEL_ADMIN:
         do_help( victim, "M_GODLVL13_" );
         break;
      case LEVEL_SUPREME:
         do_help( victim, "M_GODLVL15_" );
   }

   victim->level += 1;
   advance_level( victim );
   victim->exp = exp_level( victim->level );
   victim->trust = 0;
   STRFREE( victim->pcdata->rank );
   victim->pcdata->rank = STRALLOC( "Immortal" );
   victim->save(  );
   make_wizlist(  );
   build_wizinfo(  );
   if( victim->alignment > 350 )
      echo_all_printf( ECHOTAR_ALL, "&WA bright white flash arcs across the sky as %s gains in power!", victim->name );
   if( victim->alignment >= -350 && victim->alignment <= 350 )
      echo_all_printf( ECHOTAR_ALL, "&wA dull grey flash arcs across the sky as %s gains in power!", victim->name );
   if( victim->alignment < -350 )
      echo_all_printf( ECHOTAR_ALL, "&zAn eerie black flash arcs across the sky as %s gains in power!", victim->name );
}

/* Online high level immortal command for displaying what the encryption
 * of a name/password would be, taking in 2 arguments - the name and the
 * password - can still only change the password if you have access to 
 * pfiles and the correct password
 */
/*
 * Roger Libiez (Samson, Alsherok) caught this
 * I forgot that this function even existed... *whoops*
 * Anyway, it is rewritten with bounds checking
 */
CMDF( do_form_password )
{
   const char *pwcheck;

   ch->set_color( AT_IMMORT );

   if( argument.empty(  ) )
   {
      ch->print( "Usage: formpass <password>\r\n" );
      return;
   }

   /*
    * This is arbitrary to discourage weak passwords 
    */
   if( argument.length(  ) < 5 )
   {
      ch->print( "Usage: formpass <password>\r\n" );
      ch->print( "New password must be at least 5 characters in length.\r\n" );
      return;
   }

   if( argument[0] == '!' )
   {
      ch->print( "Usage: formpass <password>\r\n" );
      ch->print( "New password cannot begin with the '!' character.\r\n" );
      return;
   }

   pwcheck = sha256_crypt( argument.c_str(  ) );
   ch->printf( "%s results in the encrypted string: %s\r\n", argument.c_str(  ), pwcheck );
}

/*
 * Purge a player file.  No more player.  -- Altrag
 */
CMDF( do_destro )
{
   ch->print( "&RIf you want to destroy a character, spell it out!\r\n" );
}

CMDF( do_destroy )
{
   char_data *victim = NULL;
   char buf[256];
   const char *name;
   bool found = false;

   ch->set_color( AT_IMMORT );

   if( argument.empty(  ) )
   {
      ch->print( "Destroy what player file?\r\n" );
      return;
   }

   if( !check_parse_name( capitalize( argument ), false ) )
   {
      ch->print( "That's not a valid player file name!\r\n" );
      return;
   }

   list < char_data * >::iterator ich;
   for( ich = pclist.begin(  ); ich != pclist.end(  ); ++ich )
   {
      victim = *ich;

      if( !str_cmp( victim->name, argument ) )
      {
         found = true;
         break;
      }
   }
   remove_from_auth( argument );

   if( !found )
   {
      list < descriptor_data * >::iterator ds;
      descriptor_data *d = NULL;
      bool dfound = false;

      /*
       * Make sure they aren't halfway logged in. 
       */
      for( ds = dlist.begin(  ); ds != dlist.end(  ); ++ds )
      {
         d = *ds;

         if( ( victim = d->character ) && !victim->isnpc(  ) && !str_cmp( victim->name, argument ) )
         {
            dfound = true;
            break;
         }
      }
      if( dfound && d )
         close_socket( d, false );
   }
   else
   {
      if( victim->pcdata->clan )
      {
         clan_data *clan = victim->pcdata->clan;

         if( !str_cmp( victim->name, clan->leader ) )
            clan->leader.clear(  );
         if( !str_cmp( victim->name, clan->number1 ) )
            clan->number1.clear(  );
         if( !str_cmp( victim->name, clan->number2 ) )
            clan->number2.clear(  );
         remove_roster( clan, victim->name );
         --clan->members;
         save_clan( clan );
         if( clan->members < 1 )
            delete_clan( ch, clan );
      }

      if( victim->pcdata->deity )
      {
         deity_data *deity = victim->pcdata->deity;

         --deity->worshippers;
         if( deity->worshippers < 1 )
            deity->worshippers = 0;
         save_deity( deity );
      }
      quitting_char = victim;
      victim->save(  );
      saving_char = NULL;
      victim->extract( true );
      for( int x = 0; x < MAX_WEAR; ++x )
         for( int y = 0; y < MAX_LAYERS; ++y )
            save_equipment[x][y] = NULL;
   }
   name = argument.c_str(  );
   snprintf( buf, 256, "%s%c/%s", PLAYER_DIR, tolower( argument[0] ), capitalize( name ) );
   if( !remove( buf ) )
   {
      ch->printf( "&RPlayer %s destroyed.\r\n", name );
      num_pfiles -= 1;

      destroy_immdata( ch, name );
   }
   else if( errno == ENOENT )
      ch->print( "Player does not exist.\r\n" );
   else
   {
      ch->printf( "&RUnknown error #%d - %s.  Report to Samson.\r\n", errno, strerror( errno ) );
      snprintf( buf, 256, "%s destroying %s", ch->name, argument.c_str(  ) );
      perror( buf );
   }
}

/* Super-AT command:
FOR ALL <action>
FOR MORTALS <action>
FOR GODS <action>
FOR MOBS <action>
FOR EVERYWHERE <action>

Executes action several times, either on ALL players (not including yourself),
MORTALS (including trusted characters), GODS (characters with level higher than
L_HERO), MOBS (Not recommended) or every room (not recommended either!)

If you insert a # in the action, it will be replaced by the name of the target.

If # is a part of the action, the action will be executed for every target
in game. If there is no #, the action will be executed for every room containg
at least one target, but only once per room. # cannot be used with FOR EVERY-
WHERE. # can be anywhere in the action.

Example: 

FOR ALL SMILE -> you will only smile once in a room with 2 players.
FOR ALL TWIDDLE # -> In a room with A and B, you will twiddle A then B.

Destroying the characters this command acts upon MAY cause it to fail. Try to
avoid something like FOR MOBS PURGE (although it actually works at my MUD).

FOR MOBS TRANS 3054 (transfer ALL the mobs to Midgaard temple) does NOT work
though :)

The command works by transporting the character to each of the rooms with 
target in them. Private rooms are not violated.

*/

/* Expand the name of a character into a string that identifies THAT
   character within a room. E.g. the second 'guard' -> 2. guard
*/
const string name_expand( char_data * ch )
{
   string name;
   ostringstream expanded;

   if( !ch->isnpc(  ) )
      return ch->name;

   one_argument( ch->name, name );  /* copy the first word into name */

   if( name.empty(  ) ) /* weird mob .. no keywords */
      return "";

   /*
    * ->people changed to ->first_person -- TRI 
    * ... and back to ->people it is folks! -- Samson
    */
   int count = 1;
   list < char_data * >::iterator ich;
   for( ich = ch->in_room->people.begin(  ); ich != ch->in_room->people.end(  ); ++ich )
   {
      char_data *rch = *ich;

      if( rch == ch )
         continue;

      if( hasname( rch->name, name ) )
         ++count;
   }
   expanded << count << "." << name;
   return expanded.str(  );
}

CMDF( do_for )
{
   string range;
   bool fGods = false, fMortals = false, fMobs = false, fEverywhere = false, found;

   ch->set_color( AT_IMMORT );

   argument = one_argument( argument, range );
   if( range.empty(  ) || argument.empty(  ) )  /* invalid usage? */
   {
      interpret( ch, "help for" );
      return;
   }

   if( !str_prefix( "quit", argument ) )
   {
      ch->print( "Are you trying to crash the MUD or something?\r\n" );
      return;
   }

   if( !str_cmp( range, "all" ) )
   {
      fMortals = true;
      fGods = true;
   }
   else if( !str_cmp( range, "gods" ) )
      fGods = true;
   else if( !str_cmp( range, "mortals" ) )
      fMortals = true;
   else if( !str_cmp( range, "mobs" ) )
      fMobs = true;
   else if( !str_cmp( range, "everywhere" ) )
      fEverywhere = true;
   else
      interpret( ch, "help for" );  /* show syntax */

   /*
    * do not allow # to make it easier 
    */
   if( fEverywhere && strchr( argument.c_str(  ), '#' ) )
   {
      ch->print( "Cannot use FOR EVERYWHERE with the # thingie.\r\n" );
      return;
   }

   ch->set_color( AT_PLAIN );
   if( strchr( argument.c_str(  ), '#' ) )   /* replace # ? */
   {
      list < char_data * >::iterator ich;
      for( ich = charlist.begin(  ); ich != charlist.end(  ); )
      {
         char_data *p = *ich;
         ++ich;

         /*
          * In case someone DOES try to AT MOBS SLAY # 
          */
         found = false;

         if( !( p->in_room ) || p->in_room->is_private(  ) || ( p == ch ) )
            continue;

         if( p->isnpc(  ) && fMobs )
            found = true;
         else if( !p->isnpc(  ) && p->level >= LEVEL_IMMORTAL && fGods )
            found = true;
         else if( !p->isnpc(  ) && p->level < LEVEL_IMMORTAL && fMortals )
            found = true;

         /*
          * It looks ugly to me.. but it works :) 
          */
         if( found ) /* p is 'appropriate' */
         {
            string command = argument; // Buffer to process
            string target = name_expand( p );   // Target for the command
            string::size_type pos = 0;

            // String replacement loop since string_replace() won't accept a char argument
            while( ( pos = command.find( '#', pos ) ) != string::npos )
            {
               command.replace( pos, 1, target );
               pos += target.size(  );
            }

            /*
             * Execute 
             */
            room_index *old_room = ch->in_room;
            ch->from_room(  );
            if( !ch->to_room( p->in_room ) )
               log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
            interpret( ch, command );
            ch->from_room(  );
            if( !ch->to_room( old_room ) )
               log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
         }  /* if found */
      }  /* for every char */
   }
   else  /* just for every room with the appropriate people in it */
   {
      map < int, room_index * >::iterator iroom;

      for( iroom = room_index_table.begin(); iroom != room_index_table.end(); ++iroom ) /* run through all the buckets */
      {
         room_index *room = iroom->second;
         found = false;

         /*
          * Anyone in here at all? 
          */
         if( fEverywhere ) /* Everywhere executes always */
            found = true;
         else if( room->people.empty(  ) )   /* Skip it if room is empty */
            continue;
         /*
          * ->people changed to first_person -- TRI 
          * Check if there is anyone here of the requried type 
          * Stop as soon as a match is found or there are no more ppl in room 
          * ->people to ->first_person -- TRI
          * ... and back to ->people it is! -- Samson
          */
         list < char_data * >::iterator ich;
         for( ich = room->people.begin(  ); ich != room->people.end(  ) && !found; ++ich )
         {
            char_data *p = *ich;

            if( p == ch )  /* do not execute on oneself */
               continue;

            if( p->isnpc(  ) && fMobs )
               found = true;
            else if( !p->isnpc(  ) && ( p->level >= LEVEL_IMMORTAL ) && fGods )
               found = true;
            else if( !p->isnpc(  ) && ( p->level <= LEVEL_IMMORTAL ) && fMortals )
               found = true;
         }  /* for everyone inside the room */

         if( found && !room->is_private(  ) )   /* Any of the required type here AND room not private? */
         {
            /*
             * This may be ineffective. Consider moving character out of old_room
             * once at beginning of command then moving back at the end.
             * This however, is more safe?
             */

            room_index *old_room = ch->in_room;
            ch->from_room(  );
            if( !ch->to_room( room ) )
               log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
            interpret( ch, argument );
            ch->from_room(  );
            if( !ch->to_room( old_room ) )
               log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
         }  /* if found */
      }  /* for every room in a bucket */
   }  /* if strchr */
}  /* do_for */

CMDF( do_hell )
{
   char_data *victim;
   string arg;
   short htime;
   bool h_d = false;
   struct tm *tms;

   ch->set_color( AT_IMMORT );

   argument = one_argument( argument, arg );
   if( arg.empty(  ) )
   {
      ch->print( "Hell who, and for how long?\r\n" );
      return;
   }

   if( !( victim = ch->get_char_world( arg ) ) || victim->isnpc(  ) )
   {
      ch->print( "They aren't here.\r\n" );
      return;
   }

   if( victim->is_immortal(  ) )
   {
      ch->print( "There is no point in helling an immortal.\r\n" );
      return;
   }

   if( victim->pcdata->release_date != 0 )
   {
      ch->printf( "They are already in hell until %24.24s, by %s.\r\n", c_time( victim->pcdata->release_date, ch->pcdata->timezone ), victim->pcdata->helled_by );
      return;
   }

   argument = one_argument( argument, arg );
   if( arg.empty(  ) || !is_number( arg ) )
   {
      ch->print( "Hell them for how long?\r\n" );
      return;
   }

   htime = atoi( arg.c_str(  ) );
   if( htime <= 0 )
   {
      ch->print( "You cannot hell for zero or negative time.\r\n" );
      return;
   }

   argument = one_argument( argument, arg );
   if( arg.empty(  ) || !str_cmp( arg, "hours" ) )
      h_d = true;
   else if( str_cmp( arg, "days" ) )
   {
      ch->print( "Is that value in hours or days?\r\n" );
      return;
   }
   else if( htime > 30 )
   {
      ch->print( "You may not hell a person for more than 30 days at a time.\r\n" );
      return;
   }
   tms = localtime( &current_time );

   if( h_d )
      tms->tm_hour += htime;
   else
      tms->tm_mday += htime;
   victim->pcdata->release_date = mktime( tms );
   victim->pcdata->helled_by = STRALLOC( ch->name );
   ch->printf( "%s will be released from hell at %24.24s.\r\n", victim->name, c_time( victim->pcdata->release_date, ch->pcdata->timezone ) );
   act( AT_MAGIC, "$n disappears in a cloud of hellish light.", victim, NULL, ch, TO_NOTVICT );
   victim->from_room(  );
   if( !victim->to_room( get_room_index( ROOM_VNUM_HELL ) ) )
      log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
   act( AT_MAGIC, "$n appears in a could of hellish light.", victim, NULL, ch, TO_NOTVICT );
   interpret( victim, "look" );
   victim->printf( "The immortals are not pleased with your actions.\r\n"
                   "You shall remain in hell for %d %s%s.\r\n", htime, ( h_d ? "hour" : "day" ), ( htime == 1 ? "" : "s" ) );
   victim->save(  ); /* used to save ch, fixed by Thoric 09/17/96 */
}

CMDF( do_unhell )
{
   char_data *victim;
   room_index *location;

   ch->set_color( AT_IMMORT );

   if( argument.empty(  ) )
   {
      ch->print( "Unhell whom..?\r\n" );
      return;
   }
   location = ch->in_room;
   if( !( victim = ch->get_char_world( argument ) ) || victim->isnpc(  ) )
   {
      ch->print( "No such player character present.\r\n" );
      return;
   }
   if( victim->in_room->vnum != ROOM_VNUM_HELL )
   {
      ch->print( "No one like that is in hell.\r\n" );
      return;
   }

   if( victim->pcdata->clan )
      location = get_room_index( victim->pcdata->clan->recall );
   else
      location = get_room_index( ROOM_VNUM_TEMPLE );
   if( !location )
      location = get_room_index( ROOM_VNUM_LIMBO );
   MOBtrigger = false;
   act( AT_MAGIC, "$n disappears in a cloud of godly light.", victim, NULL, ch, TO_NOTVICT );
   victim->from_room(  );
   if( !victim->to_room( location ) )
      log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
   victim->print( "The gods have smiled on you and released you from hell early!\r\n" );
   interpret( victim, "look" );
   if( victim != ch )
      ch->print( "They have been released.\r\n" );
   if( victim->pcdata->helled_by )
   {
      if( str_cmp( ch->name, victim->pcdata->helled_by ) )
         ch->printf( "(You should probably write a note to %s, explaining the early release.)\r\n", victim->pcdata->helled_by );
      STRFREE( victim->pcdata->helled_by );
   }
   MOBtrigger = false;
   act( AT_MAGIC, "$n appears in a cloud of godly light.", victim, NULL, ch, TO_NOTVICT );
   victim->pcdata->release_date = 0;
   victim->save(  );
}

/* Vnum search command by Swordbearer */
CMDF( do_vsearch )
{
   bool found = false;

   if( argument.empty(  ) )
   {
      ch->print( "Syntax: vsearch <vnum>.\r\n" );
      return;
   }

   int argi = atoi( argument.c_str(  ) );
   if( argi < 1 || argi > sysdata->maxvnum )
   {
      ch->print( "Vnum out of range.\r\n" );
      return;
   }

   int obj_counter = 1;
   list < obj_data * >::iterator iobj;
   for( iobj = objlist.begin(  ); iobj != objlist.end(  ); ++iobj )
   {
      obj_data *obj = *iobj;

      if( !ch->can_see_obj( obj, true ) || !( argi == obj->pIndexData->vnum ) )
         continue;

      found = true;
      obj_data *in_obj;
      for( in_obj = obj; in_obj->in_obj != NULL; in_obj = in_obj->in_obj );

      if( in_obj->carried_by != NULL )
         ch->pagerf( "&Y[&W%2d&Y] &GLevel %d %s carried by %s.\r\n", obj_counter, obj->level, obj->oshort(  ).c_str(  ), PERS( in_obj->carried_by, ch, true ) );
      else
         ch->pagerf( "&Y[&W%2d&Y] [&W%-5d&Y] &G%s in %s.\r\n", obj_counter,
                     ( ( in_obj->in_room ) ? in_obj->in_room->vnum : 0 ), obj->oshort(  ).c_str(  ), ( in_obj->in_room == NULL ) ? "somewhere" : in_obj->in_room->name );

      ++obj_counter;
   }
   if( !found )
      ch->print( "Nothing like that exists.\r\n" );
}

/* 
 * Simple function to let any imm make any player instantly sober.
 * Saw no need for level restrictions on this.
 * Written by Narn, Apr/96 
 */
CMDF( do_sober )
{
   char_data *victim;

   ch->set_color( AT_IMMORT );
   if( argument.empty(  ) )
   {
      ch->print( "Sober up who?\r\n" );
      return;
   }

   if( !( victim = get_wizvictim( ch, argument, true ) ) )
      return;

   victim->pcdata->condition[COND_DRUNK] = 0;
   ch->print( "Ok.\r\n" );
   victim->print( "You feel sober again.\r\n" );
}

void update_calendar( void )
{
   sysdata->daysperyear = sysdata->dayspermonth * sysdata->monthsperyear;
   sysdata->hoursunrise = sysdata->hoursperday / 4;
   sysdata->hourdaybegin = sysdata->hoursunrise + 1;
   sysdata->hournoon = sysdata->hoursperday / 2;
   sysdata->hoursunset = ( ( sysdata->hoursperday / 4 ) * 3 );
   sysdata->hournightbegin = sysdata->hoursunset + 1;
   sysdata->hourmidnight = sysdata->hoursperday;
   calc_season(  );
}

void update_timers( void )
{
   sysdata->pulsetick = sysdata->secpertick * sysdata->pulsepersec;
   sysdata->pulseviolence = 3 * sysdata->pulsepersec;
   sysdata->pulsemobile = 4 * sysdata->pulsepersec;
   sysdata->pulsecalendar = 4 * sysdata->pulsetick;
   sysdata->pulseenvironment = 15 * sysdata->pulsepersec;
   sysdata->pulseskyship = sysdata->pulsemobile;
}

/* Redone cset command, with more in-game changable parameters - Samson 2-19-02 */
CMDF( do_cset )
{
   string arg;
   int value = -1;

   if( argument.empty(  ) )
   {
      ch->pager( "&WThe following options may be set:\r\n\r\n" );
      ch->pager( "&GSite Parameters\r\n" );
      ch->pager( "---------------\r\n" );
      ch->pagerf( "&BMudname  &c: %s &BEmail&c: %s &BPassword&c: [not shown]\r\n", sysdata->mud_name.c_str(  ), sysdata->admin_email.c_str(  ) );
      ch->pagerf( "&BURL      &c: %s &BTelnet&c: %s\r\n", show_tilde( sysdata->http ).c_str(  ), sysdata->telnet.c_str(  ) );
      ch->pagerf( "&BDB Server&c: %s &BDB Name&c: %s &BDB User&c: %s &BDB Password&c: [not shown]\r\n\r\n",
                  sysdata->dbserver.c_str(  ), sysdata->dbname.c_str(  ), sysdata->dbuser.c_str(  ) );

      ch->pager( "&GGame Toggles\r\n" );
      ch->pager( "------------\r\n" );
      ch->pagerf( "&BNameauth&c: %s &BImmhost Checking&c: %s &BTestmode&c: %s &BMinimum Ego&c: %d\r\n",
                  sysdata->WAIT_FOR_AUTH ? "Enabled" : "Disabled", sysdata->check_imm_host ? "Enabled" : "Disabled", sysdata->TESTINGMODE ? "On" : "Off", sysdata->minego );
      ch->pagerf( "&BPet Save&c: %s &BCrashhandler&c: %s &BPfile Pruning&c: %s\r\n",
                  sysdata->save_pets ? "On" : "Off", sysdata->crashhandler ? "Enabled" : "Disabled", sysdata->CLEANPFILES ? "Enabled" : "Disabled" );

      if( sysdata->CLEANPFILES )
      {
         ch->pagerf( "   &BNewbie Purge &c: %d days\r\n", sysdata->newbie_purge );
         ch->pagerf( "   &BRegular Purge&c: %d days\r\n", sysdata->regular_purge );
      }

      ch->pager( "\r\n&GGame Settings - Note: Altering these can have drastic affects on the game. Use with caution.\r\n" );
      ch->pager( "-------------\r\n" );
      ch->pagerf( "&BMaxVnum&c: %d &BOverland Radius&c: %d &BReboot Count&c: %d &BAuction Seconds&c: %d\r\n",
                  sysdata->maxvnum, sysdata->mapsize, sysdata->rebootcount, sysdata->auctionseconds );
      ch->
         pagerf
         ( "&BMin Guild Level&c: %d &BMax Condition Value&c: %d &BMax Ignores&c: %d &BMax Item Impact&c: %d &BInit Weapon Condition&c: %d\r\n",
           sysdata->minguildlevel, sysdata->maxcondval, sysdata->maxign, sysdata->maximpact, sysdata->initcond );
      ch->
         pagerf
         ( "&BForce Players&c: %d &BPrivate Override&c: %d &BGet Notake&c: %d &BAutosave Freq&c: %d &BMax Holidays&c: %d\r\n",
           sysdata->level_forcepc, sysdata->level_override_private, sysdata->level_getobjnotake, sysdata->save_frequency, sysdata->maxholiday );
      ch->pagerf( "&BProto Mod&c: %d &B &BMset Player&c: %d &BBestow Diff&c: %d &BBuild Level&c: %d\r\n",
                  sysdata->level_modify_proto, sysdata->level_mset_player, sysdata->bestow_dif, sysdata->build_level );
      ch->pagerf( "&BRead all mail&c: %d &BTake all mail&c: %d &BRead mail free&c: %d &BWrite mail free&c: %d\r\n",
                  sysdata->read_all_mail, sysdata->take_others_mail, sysdata->read_mail_free, sysdata->write_mail_free );
      ch->
         pagerf
         ( "&BHours per day&c: %d &BDays per week&c: %d &BDays per month&c: %d &BMonths per year&c: %d &RDays per year&W: %d\r\n",
           sysdata->hoursperday, sysdata->daysperweek, sysdata->dayspermonth, sysdata->monthsperyear, sysdata->daysperyear );
      ch->pagerf( "&BSave Flags&c: %s\r\n", bitset_string( sysdata->save_flags, save_flag ) );
      ch->pagerf( "&BWebwho&c: %d &BGame Alarm&c: %d\r\n", sysdata->webwho, sysdata->gameloopalarm );
      ch->pagerf( "\r\n&BSeconds per tick&c: %d   &BPulse per second&c: %d\r\n", sysdata->secpertick, sysdata->pulsepersec );
      ch->pagerf( "   &RPULSE_TICK&W: %d &RPULSE_VIOLENCE&W: %d &RPULSE_MOBILE&W: %d\r\n", sysdata->pulsetick, sysdata->pulseviolence, sysdata->pulsemobile );
      ch->pagerf( "   &RPULSE_CALENDAR&W: %d &RPULSE_ENVIRONMENT&W: %d &RPULSE_SKYSHIP&W: %d\r\n", sysdata->pulsecalendar, sysdata->pulseenvironment, sysdata->pulseskyship );
      return;
   }

   argument = one_argument( argument, arg );

   if( !str_cmp( arg, "help" ) )
   {
      interpret( ch, "help cset" );
      return;
   }

   ch->set_color( AT_IMMORT );

   if( !str_cmp( arg, "mudname" ) )
   {
      if( argument.empty(  ) )
      {
         sysdata->mud_name = "(Name not set)";
         ch->print( "Mud name cleared.\r\n" );
      }
      else
      {
         smash_tilde( argument );
         sysdata->mud_name = argument;
         ch->printf( "Mud name set to: %s\r\n", argument.c_str(  ) );
      }
      save_sysdata(  );
      return;
   }

   if( !str_cmp( arg, "password" ) )
   {
      const char *pwdnew;

      if( argument.length(  ) < 5 )
      {
         ch->print( "New password must be at least five characters long.\r\n" );
         return;
      }

      if( argument[0] == '!' )
      {
         ch->print( "New password cannot begin with the '!' character.\r\n" );
         return;
      }

      pwdnew = sha256_crypt( argument.c_str(  ) ); /* SHA-256 Encryption */
      sysdata->password = pwdnew;
      ch->print( "Mud password changed.\r\n" );
      save_sysdata(  );
      return;
   }

   if( !str_cmp( arg, "email" ) )
   {
      if( argument.empty(  ) )
      {
         sysdata->admin_email = "Not Set";
         ch->print( "Email address cleared.\r\n" );
      }
      else
      {
         smash_tilde( argument );
         sysdata->admin_email = argument;
         ch->printf( "Email address set to %s\r\n", argument.c_str(  ) );
      }
      save_sysdata(  );
      return;
   }

   if( !str_cmp( arg, "url" ) )
   {
      if( argument.empty(  ) )
      {
         sysdata->http = "No page set";
         ch->print( "HTTP address cleared.\r\n" );
      }
      else
      {
         hide_tilde( argument );
         sysdata->http = argument;
         ch->printf( "HTTP address set to %s\r\n", show_tilde( sysdata->http ).c_str(  ) );
      }
      save_sysdata(  );
      return;
   }

   if( !str_cmp( arg, "telnet" ) )
   {
      if( argument.empty(  ) )
      {
         sysdata->telnet = "Not Set";
         ch->print( "Telnet address cleared.\r\n" );
      }
      else
      {
         smash_tilde( argument );
         sysdata->telnet = argument;
         ch->printf( "Telnet address set to %s\r\n", argument.c_str(  ) );
      }
      save_sysdata(  );
      return;
   }

   if( !str_cmp( arg, "db-server" ) )
   {
      if( argument.empty(  ) )
      {
         sysdata->dbserver = "localhost";
         ch->print( "Database server cleared.\r\n" );
      }
      else
      {
         smash_tilde( argument );
         sysdata->dbserver = argument;
         ch->printf( "Database server set to: %s\r\n", argument.c_str(  ) );
      }
      save_sysdata(  );
      return;
   }

   if( !str_cmp( arg, "db-password" ) )
   {
      if( argument.length(  ) < 5 )
      {
         ch->print( "New password must be at least five characters long.\r\n" );
         return;
      }

      if( argument[0] == '!' )
      {
         ch->print( "New password cannot begin with the '!' character.\r\n" );
         return;
      }

      sysdata->dbpass = argument;
      ch->print( "Database password changed.\r\n" );
      save_sysdata(  );
      return;
   }

   if( !str_cmp( arg, "db-name" ) )
   {
      if( argument.empty(  ) )
      {
         sysdata->dbname = "dbname";
         ch->print( "Database name cleared.\r\n" );
      }
      else
      {
         smash_tilde( argument );
         sysdata->dbname = argument;
         ch->printf( "Database name set to: %s\r\n", argument.c_str(  ) );
      }
      save_sysdata(  );
      return;
   }

   if( !str_cmp( arg, "db-user" ) )
   {
      if( argument.empty(  ) )
      {
         sysdata->dbuser = "dbuser";
         ch->print( "Database username cleared.\r\n" );
      }
      else
      {
         smash_tilde( argument );
         sysdata->dbuser = argument;
         ch->printf( "Database username set to: %s\r\n", argument.c_str(  ) );
      }
      save_sysdata(  );
      return;
   }

   if( !str_cmp( arg, "nameauth" ) )
   {
      sysdata->WAIT_FOR_AUTH = !sysdata->WAIT_FOR_AUTH;

      if( sysdata->WAIT_FOR_AUTH )
         ch->print( "Name Authorization system enabled.\r\n" );
      else
         ch->print( "Name Authorization system disabled.\r\n" );
      save_sysdata(  );
      return;
   }

   if( !str_cmp( arg, "immhost-checking" ) )
   {
      sysdata->check_imm_host = !sysdata->check_imm_host;

      if( sysdata->check_imm_host )
         ch->print( "Immhost Checking enabled.\r\n" );
      else
         ch->print( "Immhost Checking disabled.\r\n" );
      save_sysdata(  );
      return;
   }

   if( !str_cmp( arg, "testmode" ) )
   {
      sysdata->TESTINGMODE = !sysdata->TESTINGMODE;

      if( sysdata->TESTINGMODE )
         ch->print( "Server Testmode enabled.\r\n" );
      else
         ch->print( "Server Testmode disabled.\r\n" );
      save_sysdata(  );
      return;
   }

   if( !str_cmp( arg, "pfile-pruning" ) )
   {
      sysdata->CLEANPFILES = !sysdata->CLEANPFILES;

      if( sysdata->CLEANPFILES )
         ch->print( "Pfile Pruning enabled.\r\n" );
      else
         ch->print( "Pfile Pruning disabled.\r\n" );
      save_sysdata(  );
      return;
   }

   if( !str_cmp( arg, "webwho" ) )
   {
      sysdata->webwho = !sysdata->webwho;

      if( sysdata->webwho )
         ch->print( "Web who listing enabled.\r\n" );
      else
         ch->print( "Web who listing disabled.\r\n" );
      save_sysdata(  );
      return;
   }

   if( !str_cmp( arg, "pet-save" ) )
   {
      sysdata->save_pets = !sysdata->save_pets;

      if( sysdata->save_pets )
         ch->print( "Pet saving enabled.\r\n" );
      else
         ch->print( "Pet saving disabled.\r\n" );
      save_sysdata(  );
      return;
   }

   if( !str_cmp( arg, "crashhandler" ) )
   {
      sysdata->crashhandler = !sysdata->crashhandler;

      if( sysdata->crashhandler )
      {
         set_chandler(  );
         ch->print( "Crash handling will be enabled at next reboot.\r\n" );
      }
      else
      {
         unset_chandler(  );
         ch->print( "Crash handling disabled.\r\n" );
      }
      save_sysdata(  );
      return;
   }

   if( !str_cmp( arg, "save-flags" ) )
   {
      int x = get_saveflag( argument );

      if( x == -1 || x >= SV_MAX )
      {
         ch->print( "Not a save flag.\r\n" );
         return;
      }
      sysdata->save_flags.flip( x );
      ch->printf( "%s flag toggled.\r\n", argument.c_str(  ) );
      save_sysdata(  );
      return;
   }

   /*
    * Everything below this point requires an argument, kick them if it's not there. 
    */
   if( argument.empty(  ) )
   {
      do_cset( ch, "help" );
      return;
   }

   /*
    * Everything below here requires numerical arguments, kick them again. 
    */
   if( !is_number( argument ) )
   {
      ch->print( "&RError: Argument must be an integer value.\r\n" );
      return;
   }

   value = atoi( argument.c_str(  ) );

   /*
    * Typical size range of integers 
    */
   if( value < 0 || value > 2000000000 )
   {
      ch->print( "&RError: Invalid integer value. Range is 0 to 2000000000.\r\n" );
      return;
   }

   if( !str_cmp( arg, "webwho" ) )
   {
      sysdata->webwho = value;
      if( value == 0 )
      {
         ch->print( "Webwho refresh has been disabled.\r\n" );
         cancel_event( ev_webwho_refresh, NULL );
      }
      else
      {
         ch->printf( "Webwho refresh set to %d seconds.\r\n", value );
         add_event( value, ev_webwho_refresh, NULL );
      }
      return;
   }

   if( !str_cmp( arg, "game-alarm" ) )
   {
      sysdata->gameloopalarm = value;
      ch->printf( "Game loop alarm time has been set to %d seconds.\r\n", value );
      save_sysdata(  );
      return;
   }

   if( !str_cmp( arg, "reboot-count" ) )
   {
      sysdata->rebootcount = value;
      ch->printf( "Reboot time counter set to %d minutes.\r\n", value );
      save_sysdata(  );
      return;
   }

   if( !str_cmp( arg, "auction-seconds" ) )
   {
      sysdata->auctionseconds = value;
      ch->printf( "Auction timer set to %d seconds.\r\n", value );
      save_sysdata(  );
      return;
   }

   if( !str_cmp( arg, "newbie-purge" ) )
   {
      if( value > 32767 )
      {
         ch->print( "&RError: Cannot set Newbie Purge above 32767.\r\n" );
         return;
      }
      sysdata->newbie_purge = value;
      ch->printf( "Newbie Purge set to %d.\r\n", value );
      save_sysdata(  );
      return;
   }

   if( !str_cmp( arg, "regular-purge" ) )
   {
      if( value > 32767 )
      {
         ch->print( "&RError: Cannot set Regular Purge above 32767.\r\n" );
         return;
      }
      sysdata->regular_purge = value;
      ch->printf( "Regular Purge set to %d.\r\n", value );
      save_sysdata(  );
      return;
   }

   if( !str_cmp( arg, "minimum-ego" ) )
   {
      sysdata->minego = value;
      ch->printf( "Minimum Ego set to %d.\r\n", value );
      save_sysdata(  );
      return;
   }

   if( !str_cmp( arg, "maxvnum" ) )
   {
      list < area_data * >::iterator iarea;
      char lbuf[MSL];
      int vnum = 0;

      for( iarea = arealist.begin(  ); iarea != arealist.end(  ); ++iarea )
      {
         area_data *area = *iarea;
         if( area->hi_vnum > vnum )
         {
            mudstrlcpy( lbuf, area->name, MSL );
            vnum = area->hi_vnum;
         }
      }

      if( value <= vnum )
      {
         ch->printf( "&RError: Cannot set MaxVnum to %d, existing areas extend to %d.\r\n", value, vnum );
         return;
      }

      if( value - vnum < 1000 )
      {
         ch->printf( "Warning: Setting MaxVnum to %d leaves you with less than 1000 vnums beyond the highest area.\r\n", value );
         ch->printf( "Highest area %s ends with vnum %d.\r\n", lbuf, vnum );
      }

      sysdata->maxvnum = value;
      ch->printf( "MaxVnum changed to %d.\r\n", value );
      save_sysdata(  );
      return;
   }

   if( !str_cmp( arg, "overland-radius" ) )
   {
      if( value > 14 )
      {
         ch->print( "&RError: Cannot set Overland Radius larger than 14 due to screen size restrictions.\r\n" );
         return;
      }
      sysdata->mapsize = value;
      ch->printf( "Overland Radius set to %d.\r\n", value );
      save_sysdata(  );
      return;
   }

   if( !str_cmp( arg, "min-guild-level" ) )
   {
      if( value > LEVEL_AVATAR )
      {
         ch->printf( "&RError: Cannot set Min Guild Level above level %d.\r\n", LEVEL_AVATAR );
         return;
      }
      sysdata->minguildlevel = value;
      ch->printf( "Min Guild Level set to %d.\r\n", value );
      save_sysdata(  );
      return;
   }

   if( !str_cmp( arg, "max-condition-value" ) )
   {
      sysdata->maxcondval = value;
      ch->printf( "Max Condition Value set to %d.\r\n", value );
      save_sysdata(  );
      return;
   }

   if( !str_cmp( arg, "max-ignores" ) )
   {
      sysdata->maxign = value;
      ch->printf( "Max Ignores set to %d.\r\n", value );
      save_sysdata(  );
      return;
   }

   if( !str_cmp( arg, "max-item-impact" ) )
   {
      sysdata->maximpact = value;
      ch->printf( "Max Item Impact set to %d.\r\n", value );
      save_sysdata(  );
      return;
   }

   if( !str_cmp( arg, "init-weapon-condition" ) )
   {
      sysdata->initcond = value;
      ch->printf( "Init Weapon Condition set to %d.\r\n", value );
      save_sysdata(  );
      return;
   }

   if( !str_cmp( arg, "force-players" ) )
   {
      if( value > MAX_LEVEL || value < LEVEL_IMMORTAL )
      {
         ch->printf( "&RError: Cannot set Force Players above level %d, or below level %d.\r\n", MAX_LEVEL, LEVEL_IMMORTAL );
         return;
      }
      sysdata->level_forcepc = value;
      ch->printf( "Force Players set to %d.\r\n", value );
      save_sysdata(  );
      return;
   }

   if( !str_cmp( arg, "private-override" ) )
   {
      if( value > MAX_LEVEL || value < LEVEL_IMMORTAL )
      {
         ch->printf( "&RError: Cannot set Private Override above level %d, or below level %d.\r\n", MAX_LEVEL, LEVEL_IMMORTAL );
         return;
      }
      sysdata->level_override_private = value;
      ch->printf( "Private Override set to %d.\r\n", value );
      save_sysdata(  );
      return;
   }

   if( !str_cmp( arg, "get-notake" ) )
   {
      if( value > MAX_LEVEL || value < LEVEL_IMMORTAL )
      {
         ch->printf( "&RError: Cannot set Get Notake above level %d, or below level %d.\r\n", MAX_LEVEL, LEVEL_IMMORTAL );
         return;
      }
      sysdata->level_getobjnotake = value;
      ch->printf( "Get Notake set to %d.\r\n", value );
      save_sysdata(  );
      return;
   }

   if( !str_cmp( arg, "autosave-freq" ) )
   {
      if( value > 32767 )
      {
         ch->print( "&RError: Cannot set Autosave Freq above 32767.\r\n" );
         return;
      }
      sysdata->save_frequency = value;
      ch->printf( "Autosave Freq set to %d.\r\n", value );
      save_sysdata(  );
      return;
   }

   if( !str_cmp( arg, "max-holidays" ) )
   {
      sysdata->maxholiday = value;
      ch->printf( "Max Holiday set to %d.\r\n", value );
      save_sysdata(  );
      return;
   }

   if( !str_cmp( arg, "proto-mod" ) )
   {
      if( value > MAX_LEVEL || value < LEVEL_IMMORTAL )
      {
         ch->printf( "&RError: Cannot set Proto Mod above level %d, or below level %d.\r\n", MAX_LEVEL, LEVEL_IMMORTAL );
         return;
      }
      sysdata->level_modify_proto = value;
      ch->printf( "Proto Mod set to %d.\r\n", value );
      save_sysdata(  );
      return;
   }

   if( !str_cmp( arg, "mset-player" ) )
   {
      if( value > MAX_LEVEL || value < LEVEL_IMMORTAL )
      {
         ch->printf( "&RError: Cannot set Mset Player above level %d, or below level %d.\r\n", MAX_LEVEL, LEVEL_IMMORTAL );
         return;
      }
      sysdata->level_mset_player = value;
      ch->printf( "Mset Player set to %d.\r\n", value );
      save_sysdata(  );
      return;
   }

   if( !str_cmp( arg, "bestow-diff" ) )
   {
      if( value > MAX_LEVEL )
      {
         ch->printf( "&RError: Cannot set Bestow Diff above %d.\r\n", MAX_LEVEL );
         return;
      }
      sysdata->bestow_dif = value;
      ch->printf( "Bestow Diff set to %d.\r\n", value );
      save_sysdata(  );
      return;
   }

   if( !str_cmp( arg, "build-level" ) )
   {
      if( value > MAX_LEVEL || value < LEVEL_IMMORTAL )
      {
         ch->printf( "&RError: Cannot set Build Level above level %d, or below level %d.\r\n", MAX_LEVEL, LEVEL_IMMORTAL );
         return;
      }
      sysdata->build_level = value;
      ch->printf( "Build Level set to %d.\r\n", value );
      save_sysdata(  );
      return;
   }

   if( !str_cmp( arg, "read-all-mail" ) )
   {
      if( value > MAX_LEVEL || value < LEVEL_IMMORTAL )
      {
         ch->printf( "&RError: Cannot set Read all mail above level %d, or below level %d.\r\n", MAX_LEVEL, LEVEL_IMMORTAL );
         return;
      }
      sysdata->read_all_mail = value;
      ch->printf( "Read all mail set to %d.\r\n", value );
      save_sysdata(  );
      return;
   }

   if( !str_cmp( arg, "take-all-mail" ) )
   {
      if( value > MAX_LEVEL || value < LEVEL_IMMORTAL )
      {
         ch->printf( "&RError: Cannot set Take all mail above level %d, or below level %d.\r\n", MAX_LEVEL, LEVEL_IMMORTAL );
         return;
      }
      sysdata->take_others_mail = value;
      ch->printf( "Take all mail set to %d.\r\n", value );
      save_sysdata(  );
      return;
   }

   if( !str_cmp( arg, "read-mail-free" ) )
   {
      if( value > MAX_LEVEL )
      {
         ch->printf( "&RError: Cannot set Read mail free above level %d.\r\n", MAX_LEVEL );
         return;
      }
      sysdata->read_mail_free = value;
      ch->printf( "Read mail free set to %d.\r\n", value );
      save_sysdata(  );
      return;
   }

   if( !str_cmp( arg, "write-mail-free" ) )
   {
      if( value > MAX_LEVEL )
      {
         ch->printf( "&RError: Cannot set Write mail free above level %d.\r\n", MAX_LEVEL );
         return;
      }
      sysdata->write_mail_free = value;
      ch->printf( "Write mail free set to %d.\r\n", value );
      save_sysdata(  );
      return;
   }

   if( !str_cmp( arg, "hours-per-day" ) )
   {
      sysdata->hoursperday = value;
      ch->printf( "Hours per day set to %d.\r\n", value );
      update_calendar(  );
      save_sysdata(  );
      return;
   }

   if( !str_cmp( arg, "days-per-week" ) )
   {
      sysdata->daysperweek = value;
      ch->printf( "Days per week set to %d.\r\n", value );
      update_calendar(  );
      save_sysdata(  );
      return;
   }

   if( !str_cmp( arg, "days-per-month" ) )
   {
      sysdata->dayspermonth = value;
      ch->printf( "Days per month set to %d.\r\n", value );
      update_calendar(  );
      save_sysdata(  );
      return;
   }

   if( !str_cmp( arg, "months-per-year" ) )
   {
      sysdata->monthsperyear = value;
      ch->printf( "Months per year set to %d.\r\n", value );
      update_calendar(  );
      save_sysdata(  );
      return;
   }

   if( !str_cmp( arg, "seconds-per-tick" ) )
   {
      sysdata->secpertick = value;
      ch->printf( "Seconds per tick set to %d.\r\n", value );
      update_timers(  );
      save_sysdata(  );
      return;
   }

   if( !str_cmp( arg, "pulse-per-second" ) )
   {
      sysdata->pulsepersec = value;
      ch->printf( "Pulse per second set to %d.\r\n", value );
      update_timers(  );
      save_sysdata(  );
      return;
   }

   do_cset( ch, "help" );
}

void free_all_titles( void )
{
   int hash, loopa;

   for( hash = 0; hash < MAX_CLASS; ++hash )
   {
      for( loopa = 0; loopa < MAX_LEVEL + 1; ++loopa )
      {
         STRFREE( title_table[hash][loopa][SEX_NEUTRAL] );
         STRFREE( title_table[hash][loopa][SEX_MALE] );
         STRFREE( title_table[hash][loopa][SEX_FEMALE] );
         STRFREE( title_table[hash][loopa][SEX_HERMAPHRODYTE] );
      }
   }
}

class_type::class_type(  )
{
   init_memory( &affected, &fMana, sizeof( fMana ) );
}

class_type::~class_type(  )
{
   STRFREE( who_name );
}

void free_all_classes( void )
{
   int cl;

   for( cl = 0; cl < MAX_CLASS; ++cl )
   {
      class_type *Class = class_table[cl];
      deleteptr( Class );
   }
}

bool load_class_file( const char *fname )
{
   char buf[256];
   class_type *Class;
   int cl = -1, tlev = 0, file_ver = 0;
   FILE *fp;

   snprintf( buf, 256, "%s%s", CLASS_DIR, fname );
   if( !( fp = fopen( buf, "r" ) ) )
   {
      perror( buf );
      return false;
   }

   Class = new class_type;

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

         case 'A':
            if( !str_cmp( word, "Affected" ) )
            {
               if( file_ver < 1 )
               {
                  Class->affected = fread_number( fp );
                  if( Class->affected.any(  ) )
                     Class->affected <<= 1;
               }
               else
                  flag_set( fp, Class->affected, aff_flags );
               break;
            }
            KEY( "Armor", Class->armor, fread_number( fp ) );
            KEY( "Armwear", Class->armwear, fread_number( fp ) ); /* Samson */
            KEY( "AttrPrime", Class->attr_prime, fread_number( fp ) );
            break;

         case 'C':
            KEY( "Class", cl, fread_number( fp ) );
            break;

         case 'E':
            if( !str_cmp( word, "End" ) )
            {
               FCLOSE( fp );
               if( cl < 0 || cl >= MAX_CLASS )
               {
                  bug( "%s: Class (%s) bad/not found (%d)", __FUNCTION__, Class->who_name ? Class->who_name : "name not found", cl );
                  deleteptr( Class );
                  return false;
               }
               class_table[cl] = Class;
               return true;
            }
            break;

         case 'F':
            KEY( "Footwear", Class->footwear, fread_number( fp ) );  /* Samson 1-3-99 */
            break;

         case 'H':
            KEY( "Headwear", Class->headwear, fread_number( fp ) );  /* Samson 1-3-99 */
            KEY( "Held", Class->held, fread_number( fp ) ); /* Samson 1-3-99 */
            KEY( "HpMax", Class->hp_max, fread_number( fp ) );
            KEY( "HpMin", Class->hp_min, fread_number( fp ) );
            break;

         case 'L':
            KEY( "Legwear", Class->legwear, fread_number( fp ) ); /* Samson 1-3-99 */
            break;

         case 'M':
            KEY( "Mana", Class->fMana, fread_number( fp ) );
            break;

         case 'N':
            KEY( "Name", Class->who_name, fread_string( fp ) );
            break;

         case 'R':
            if( !str_cmp( word, "Resist" ) )
            {
               if( file_ver < 1 )
                  Class->resist = fread_number( fp );
               else
                  flag_set( fp, Class->resist, ris_flags );
               break;
            }
            break;

         case 'S':
            KEY( "Shield", Class->shield, fread_number( fp ) );
            if( !str_cmp( word, "Skill" ) )
            {
               int sn, lev, adp;

               word = fread_word( fp );
               lev = fread_number( fp );
               adp = fread_number( fp );
               sn = skill_lookup( word );
               if( cl < 0 || cl >= MAX_CLASS )
                  bug( "%s: Skill %s -- Class bad/not found (%d)", __FUNCTION__, word, cl );
               else if( !IS_VALID_SN( sn ) )
                  bug( "%s: Skill %s unknown. Class: %d", __FUNCTION__, word, cl );
               else
               {
                  skill_table[sn]->skill_level[cl] = lev;
                  skill_table[sn]->skill_adept[cl] = adp;
               }
               break;
            }
            KEY( "Skilladept", Class->skill_adept, fread_number( fp ) );
            if( !str_cmp( word, "Suscept" ) )
            {
               if( file_ver < 1 )
                  Class->suscept = fread_number( fp );
               else
                  flag_set( fp, Class->suscept, ris_flags );
               break;
            }
            break;

         case 'T':
            if( !str_cmp( word, "Title" ) )
            {
               if( cl < 0 || cl >= MAX_CLASS )
               {
                  bug( "%s: Title -- Class bad/not found (%d)", __FUNCTION__, cl );
                  fread_flagstring( fp );
                  fread_flagstring( fp );
               }
               else if( tlev < MAX_LEVEL + 1 )
               {
                  title_table[cl][tlev][SEX_NEUTRAL] = fread_string( fp );
                  title_table[cl][tlev][SEX_MALE] = fread_string( fp );
                  title_table[cl][tlev][SEX_FEMALE] = fread_string( fp );
                  title_table[cl][tlev][SEX_HERMAPHRODYTE] = fread_string( fp );

                  ++tlev;
               }
               else
                  bug( "%s: Too many titles. Class: %d", __FUNCTION__, cl );
               break;
            }
            KEY( "Thac0gain", Class->thac0_gain, fread_float( fp ) );
            KEY( "Thac0", Class->base_thac0, fread_number( fp ) );
            break;

         case 'V':
            KEY( "Version", file_ver, fread_number( fp ) );
            break;

         case 'W':
            KEY( "Weapon", Class->weapon, fread_number( fp ) );
            break;
      }
   }
}

/*
 * Load in all the Class files.
 */
void load_classes(  )
{
   FILE *fpList;
   const char *filename;
   char classlist[256];

   MAX_PC_CLASS = 0;
   /*
    * Pre-init the class_table with blank classes
    */
   for( int i = 0; i < MAX_CLASS; ++i )
      class_table[i] = NULL;

   snprintf( classlist, 256, "%s%s", CLASS_DIR, CLASS_LIST );
   if( !( fpList = fopen( classlist, "r" ) ) )
   {
      perror( classlist );
      exit( 1 );
   }

   for( ;; )
   {
      filename = ( feof( fpList ) ? "$" : fread_word( fpList ) );

      if( filename[0] == '\0' )
      {
         bug( "%s: EOF encountered reading class list!", __FUNCTION__ );
         break;
      }

      if( filename[0] == '$' )
         break;

      if( !load_class_file( filename ) )
         bug( "%s: Cannot load Class file: %s", __FUNCTION__, filename );
      else
         ++MAX_PC_CLASS;
   }
   FCLOSE( fpList );
   for( int i = 0; i < MAX_CLASS; ++i )
   {
      if( !class_table[i] )
      {
         class_type *Class = new class_type;

         Class->who_name = STRALLOC( "No Class Name" );
         class_table[i] = Class;
      }
   }
   return;
}

void write_class_file( int cl )
{
   FILE *fpout;
   char filename[256];
   class_type *Class = class_table[cl];

   snprintf( filename, 256, "%s%s.class", CLASS_DIR, Class->who_name );
   if( !( fpout = fopen( filename, "w" ) ) )
   {
      bug( "%s: Cannot open: %s for writing", __FUNCTION__, filename );
      return;
   }
   fprintf( fpout, "Version     %d\n", CLASSFILEVER );
   fprintf( fpout, "Name        %s~\n", Class->who_name );
   fprintf( fpout, "Class       %d\n", cl );
   fprintf( fpout, "Attrprime   %d\n", Class->attr_prime );
   fprintf( fpout, "Weapon      %d\n", Class->weapon );
   fprintf( fpout, "Armor       %d\n", Class->armor );   /* Samson */
   fprintf( fpout, "Legwear     %d\n", Class->legwear ); /* Samson 1-3-99 */
   fprintf( fpout, "Headwear    %d\n", Class->headwear );   /* Samson 1-3-99 */
   fprintf( fpout, "Armwear     %d\n", Class->armwear ); /* Samson 1-3-99 */
   fprintf( fpout, "Footwear    %d\n", Class->footwear );   /* Samson 1-3-99 */
   fprintf( fpout, "Shield      %d\n", Class->shield );  /* Samson 1-3-99 */
   fprintf( fpout, "Held        %d\n", Class->held ); /* Samson 1-3-99 */
   fprintf( fpout, "Skilladept  %d\n", Class->skill_adept );
   fprintf( fpout, "Thac0       %d\n", Class->base_thac0 );
   fprintf( fpout, "Thac0gain   %f\n", ( float )Class->thac0_gain );
   fprintf( fpout, "Hpmin       %d\n", Class->hp_min );
   fprintf( fpout, "Hpmax       %d\n", Class->hp_max );
   fprintf( fpout, "Mana        %d\n", Class->fMana );
   if( Class->affected.any(  ) )
      fprintf( fpout, "Affected    %s~\n", bitset_string( Class->affected, aff_flags ) );
   if( Class->resist.any(  ) )
      fprintf( fpout, "Resist      %s~\n", bitset_string( Class->resist, ris_flags ) );
   if( Class->suscept.any(  ) )
      fprintf( fpout, "Suscept     %s~\n", bitset_string( Class->suscept, ris_flags ) );
   for( int x = 0; x < num_skills; ++x )
   {
      int y;

      if( !skill_table[x]->name || skill_table[x]->name[0] == '\0' )
         break;
      if( ( y = skill_table[x]->skill_level[cl] ) < LEVEL_IMMORTAL )
         fprintf( fpout, "Skill '%s' %d %d\n", skill_table[x]->name, y, skill_table[x]->skill_adept[cl] );
   }
   for( int x = 0; x <= MAX_LEVEL; ++x )
      fprintf( fpout, "Title\n%s~\n%s~\n%s~\n%s~\n", title_table[cl][x][SEX_NEUTRAL], title_table[cl][x][SEX_MALE], title_table[cl][x][SEX_FEMALE], title_table[cl][x][SEX_HERMAPHRODYTE] );

   fprintf( fpout, "%s", "End\n" );
   FCLOSE( fpout );
}

void save_classes(  )
{
   for( int x = 0; x < MAX_PC_CLASS; ++x )
      write_class_file( x );
}

void write_class_list(  )
{
   FILE *fpList;
   char classlist[256];

   snprintf( classlist, 256, "%s%s", CLASS_DIR, CLASS_LIST );
   if( !( fpList = fopen( classlist, "w" ) ) )
   {
      bug( "%s: Can't open class list for writing.", __FUNCTION__ );
      return;
   }

   for( int i = 0; i < MAX_PC_CLASS; ++i )
      fprintf( fpList, "%s.class\n", class_table[i]->who_name );
   fprintf( fpList, "%s", "$\n" );
   FCLOSE( fpList );
}

/*
 * Display Class information - Thoric
 */
CMDF( do_showclass )
{
   string arg1, arg2;
   class_type *Class;
   int cl = 0, low, hi;

   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );
   if( arg1.empty(  ) )
   {
      ch->print( "Syntax: showclass <class> [level range]\r\n" );
      return;
   }
   if( is_number( arg1 ) && ( cl = atoi( arg1.c_str(  ) ) ) >= 0 && cl < MAX_CLASS )
      Class = class_table[cl];
   else
   {
      Class = NULL;
      for( cl = 0; cl < MAX_CLASS && class_table[cl]; ++cl )
         if( !str_cmp( class_table[cl]->who_name, arg1 ) )
         {
            Class = class_table[cl];
            break;
         }
   }
   if( !Class )
   {
      ch->print( "No such Class.\r\n" );
      return;
   }
   ch->pagerf( "&wCLASS: &W%s\r\n", Class->who_name );
   ch->pagerf( "&wStarting Weapon:  &W%-6d &wStarting Armor:     &W%-6d\r\n", Class->weapon, Class->armor );
   ch->pagerf( "&wStarting Legwear: &W%-6d &wStarting Headwear:  &W%-6d\r\n", Class->legwear, Class->headwear );
   ch->pagerf( "&wStarting Armwear: &W%-6d &wStarting Footwear:  &W%-6d\r\n", Class->armwear, Class->footwear );
   ch->pagerf( "&wStarting Shield:  &W%-6d &wStarting Held Item: &W%-6d\r\n", Class->shield, Class->held );
   ch->pagerf( "&wBaseThac0:        &W%-6d &wThac0Gain: &W%f\r\n", Class->base_thac0, Class->thac0_gain );
   ch->pagerf( "&wMax Skill Adept:  &W%-3d ", Class->skill_adept );
   ch->pagerf( "&wHp Min/Hp Max  : &W%-2d/%-2d           &wMana  : &W%-3s\r\n", Class->hp_min, Class->hp_max, Class->fMana ? "yes" : "no " );
   ch->pagerf( "&wAffected by:  &W%s\r\n", bitset_string( Class->affected, aff_flags ) );
   ch->pagerf( "&wResistant to: &W%s\r\n", bitset_string( Class->resist, ris_flags ) );
   ch->pagerf( "&wSusceptible to: &W%s\r\n", bitset_string( Class->suscept, ris_flags ) );

   if( !arg2.empty(  ) )
   {
      int x, y, cnt;

      low = UMAX( 0, atoi( arg2.c_str(  ) ) );
      hi = URANGE( low, atoi( argument.c_str(  ) ), MAX_LEVEL );
      for( x = low; x <= hi; ++x )
      {
         ch->pagerf( "&wLevel: &W%d     &wExperience required: &W%ld\r\n", x, exp_level( x ) );
         ch->pagerf( "&wNeutral: &W%-20s &wMale: &W%-20s &wFemale: &W%-20s &wHermaphrodyte: &W%s\r\n", title_table[cl][x][SEX_NEUTRAL], title_table[cl][x][SEX_MALE], title_table[cl][x][SEX_FEMALE], title_table[cl][x][SEX_HERMAPHRODYTE] );
         cnt = 0;
         for( y = 0; y < num_skills; ++y )
            if( skill_table[y]->skill_level[cl] == x )
            {
               ch->pagerf( "  &[skill]%-7s %-19s%3d     ", skill_tname[skill_table[y]->type], skill_table[y]->name, skill_table[y]->skill_adept[cl] );
               if( ++cnt % 2 == 0 )
                  ch->pager( "\r\n" );
            }
         if( cnt % 2 != 0 )
            ch->pager( "\r\n" );
         ch->pager( "\r\n" );
      }
   }
}

/*
 * Create a new Class online - Shaddai
 */
bool create_new_class( int Class, const string & argument )
{
   if( Class >= MAX_CLASS || class_table[Class] == NULL )
      return false;
   STRFREE( class_table[Class]->who_name );
   class_table[Class]->who_name = STRALLOC( capitalize( argument ).c_str(  ) );
   class_table[Class]->affected.reset(  );
   class_table[Class]->attr_prime = 0;
   class_table[Class]->resist = 0;
   class_table[Class]->suscept = 0;
   class_table[Class]->weapon = 0;
   class_table[Class]->skill_adept = 0;
   class_table[Class]->base_thac0 = 0;
   class_table[Class]->thac0_gain = 0;
   class_table[Class]->hp_min = 0;
   class_table[Class]->hp_max = 0;
   class_table[Class]->fMana = false;
   for( int i = 0; i < MAX_LEVEL; ++i )
   {
      title_table[Class][i][SEX_NEUTRAL] = STRALLOC( "Not set." );
      title_table[Class][i][SEX_MALE] = STRALLOC( "Not set." );
      title_table[Class][i][SEX_FEMALE] = STRALLOC( "Not set." );
      title_table[Class][i][SEX_HERMAPHRODYTE] = STRALLOC( "Not set." );
   }
   return true;
}

/*
 * Edit Class information - Thoric
 */
CMDF( do_setclass )
{
   string arg1, arg2;
   class_type *Class;
   int cl = 0, value, i;

   ch->set_color( AT_IMMORT );

   smash_tilde( argument );
   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );
   if( arg1.empty(  ) )
   {
      ch->print( "Syntax: setclass <class> <field> <value>\r\n" );
      ch->print( "Syntax: setclass <class> create\r\n" );
      ch->print( "\r\nField being one of:\r\n" );
      ch->print( "  name prime weapon armor legwear headwear\r\n" );
      ch->print( "  armwear footwear shield held basethac0 thac0gain\r\n" );
      ch->print( "  hpmin hpmax mana ntitle mtitle ftitle htitle\r\n" );
      ch->print( "  affected resist suscept skill\r\n" );
      return;
   }

   if( is_number( arg1 ) && ( cl = atoi( arg1.c_str(  ) ) ) >= 0 && cl < MAX_CLASS )
      Class = class_table[cl];
   else
   {
      Class = NULL;
      for( cl = 0; cl < MAX_CLASS && class_table[cl]; ++cl )
      {
         if( !class_table[cl]->who_name )
            continue;
         if( !str_cmp( class_table[cl]->who_name, arg1 ) )
         {
            Class = class_table[cl];
            break;
         }
      }
   }

   if( !str_cmp( arg2, "create" ) && Class )
   {
      ch->print( "That Class already exists!\r\n" );
      return;
   }

   if( !Class && str_cmp( arg2, "create" ) )
   {
      ch->print( "No such Class.\r\n" );
      return;
   }

   if( !str_cmp( arg2, "create" ) )
   {
      char filename[256];

      if( MAX_PC_CLASS >= MAX_CLASS )
      {
         ch->print( "You need to up MAX_CLASS in mud and make clean.\r\n" );
         return;
      }

      snprintf( filename, sizeof( filename ), "%s.class", arg1.c_str(  ) );
      if( !is_valid_filename( ch, CLASS_DIR, filename ) )
         return;

      if( !( create_new_class( MAX_PC_CLASS, arg1 ) ) )
      {
         ch->print( "Couldn't create a new class.\r\n" );
         return;
      }
      write_class_file( MAX_PC_CLASS );
      ++MAX_PC_CLASS;
      write_class_list(  );
      ch->print( "Done.\r\n" );
      return;
   }

   if( argument.empty(  ) )
   {
      ch->print( "You must specify an argument.\r\n" );
      return;
   }

   if( !str_cmp( arg2, "skill" ) )
   {
      skill_type *skill;
      int sn, level, adept;

      argument = one_argument( argument, arg2 );
      if( ( sn = skill_lookup( arg2 ) ) > 0 )
      {
         skill = get_skilltype( sn );
         argument = one_argument( argument, arg2 );
         level = atoi( arg2.c_str(  ) );
         argument = one_argument( argument, arg2 );
         adept = atoi( arg2.c_str(  ) );
         skill->skill_level[cl] = level;
         skill->skill_adept[cl] = adept;
         write_class_file( cl );
         ch->printf( "Skill \"%s\" added at level %d and %d%%.\r\n", skill->name, level, adept );
      }
      else
         ch->printf( "No such skill as %s.\r\n", arg2.c_str(  ) );
      return;
   }

   if( !str_cmp( arg2, "name" ) )
   {
      char buf[256];
      class_type *ccheck = NULL;

      one_argument( argument, arg1 );
      if( arg1.empty(  ) )
      {
         ch->print( "You can't set a class name to nothing.\r\n" );
         return;
      }

      snprintf( buf, sizeof( buf ), "%s.class", arg1.c_str(  ) );
      if( !is_valid_filename( ch, CLASS_DIR, buf ) )
         return;

      for( i = 0; i < MAX_PC_CLASS && class_table[i]; ++i )
      {
         if( !class_table[i]->who_name )
            continue;

         if( !str_cmp( class_table[i]->who_name, arg1 ) )
         {
            ccheck = class_table[i];
            break;
         }
      }
      if( ccheck != NULL )
      {
         ch->printf( "Already a class called %s.\r\n", arg1.c_str(  ) );
         return;
      }

      snprintf( buf, sizeof( buf ), "%s%s.class", CLASS_DIR, Class->who_name );
      unlink( buf );
      STRFREE( Class->who_name );
      Class->who_name = STRALLOC( capitalize( argument ).c_str(  ) );
      ch->printf( "Class renamed to %s.\r\n", arg1.c_str(  ) );
      write_class_file( cl );
      write_class_list(  );
      ch->print( "Done.\r\n" );
      return;
   }

   if( !str_cmp( arg2, "affected" ) )
   {
      if( argument.empty(  ) )
      {
         ch->print( "Usage: setclass <Class> affected <flag> [flag]...\r\n" );
         return;
      }
      while( !argument.empty(  ) )
      {
         argument = one_argument( argument, arg2 );
         value = get_aflag( arg2 );
         if( value < 0 || value >= MAX_AFFECTED_BY )
            ch->printf( "Unknown flag: %s\r\n", arg2.c_str(  ) );
         else
            Class->affected.flip( value );
      }
      ch->print( "Done.\r\n" );
      write_class_file( cl );
      return;
   }

   if( !str_cmp( arg2, "resist" ) )
   {
      if( argument.empty(  ) )
      {
         ch->print( "Usage: setclass <Class> resist <flag> [flag]...\r\n" );
         return;
      }
      while( !argument.empty(  ) )
      {
         argument = one_argument( argument, arg2 );
         value = get_risflag( arg2 );
         if( value < 0 || value >= MAX_RIS_FLAG )
            ch->printf( "Unknown flag: %s\r\n", arg2.c_str(  ) );
         else
            Class->resist.flip( value );
      }
      ch->print( "Done.\r\n" );
      write_class_file( cl );
      return;
   }

   if( !str_cmp( arg2, "suscept" ) )
   {
      if( argument.empty(  ) )
      {
         ch->print( "Usage: setclass <Class> suscept <flag> [flag]...\r\n" );
         return;
      }
      while( !argument.empty(  ) )
      {
         argument = one_argument( argument, arg2 );
         value = get_risflag( arg2 );
         if( value < 0 || value >= MAX_RIS_FLAG )
            ch->printf( "Unknown flag: %s\r\n", arg2.c_str(  ) );
         else
            Class->suscept.flip( value );
      }
      ch->print( "Done.\r\n" );
      write_class_file( cl );
      return;
   }

   if( !str_cmp( arg2, "prime" ) )
   {
      int x = get_atype( argument );

      if( x < APPLY_STR || x > APPLY_LCK )
         ch->print( "Invalid prime attribute!\r\n" );
      else
      {
         Class->attr_prime = x;
         ch->print( "Prime attribute set.\r\n" );
         write_class_file( cl );
      }
      return;
   }

   if( !str_cmp( arg2, "weapon" ) )
   {
      Class->weapon = atoi( argument.c_str(  ) );
      ch->print( "Starting weapon set.\r\n" );
      write_class_file( cl );
      return;
   }

   if( !str_cmp( arg2, "armor" ) )
   {
      Class->armor = atoi( argument.c_str(  ) );
      ch->print( "Starting armor set.\r\n" );
      write_class_file( cl );
      return;
   }

   if( !str_cmp( arg2, "legwear" ) )
   {
      Class->legwear = atoi( argument.c_str(  ) );
      ch->print( "Starting legwear set.\r\n" );
      write_class_file( cl );
      return;
   }

   if( !str_cmp( arg2, "headwear" ) )
   {
      Class->headwear = atoi( argument.c_str(  ) );
      ch->print( "Starting headwear set.\r\n" );
      write_class_file( cl );
      return;
   }

   if( !str_cmp( arg2, "armwear" ) )
   {
      Class->armwear = atoi( argument.c_str(  ) );
      ch->print( "Starting armwear set.\r\n" );
      write_class_file( cl );
      return;
   }

   if( !str_cmp( arg2, "footwear" ) )
   {
      Class->footwear = atoi( argument.c_str(  ) );
      ch->print( "Starting footwear set.\r\n" );
      write_class_file( cl );
      return;
   }

   if( !str_cmp( arg2, "shield" ) )
   {
      Class->shield = atoi( argument.c_str(  ) );
      ch->print( "Starting shield set.\r\n" );
      write_class_file( cl );
      return;
   }

   if( !str_cmp( arg2, "held" ) )
   {
      Class->held = atoi( argument.c_str(  ) );
      ch->print( "Starting held item set.\r\n" );
      write_class_file( cl );
      return;
   }

   if( !str_cmp( arg2, "basethac0" ) )
   {
      Class->base_thac0 = atoi( argument.c_str(  ) );
      ch->print( "Base Thac0 set.\r\n" );
      write_class_file( cl );
      return;
   }

   if( !str_cmp( arg2, "thac0gain" ) )
   {
      Class->thac0_gain = atof( argument.c_str(  ) );
      ch->print( "Thac0gain set.\r\n" );
      write_class_file( cl );
      return;
   }

   if( !str_cmp( arg2, "hpmin" ) )
   {
      Class->hp_min = atoi( argument.c_str(  ) );
      ch->print( "Min HP gain set.\r\n" );
      write_class_file( cl );
      return;
   }

   if( !str_cmp( arg2, "hpmax" ) )
   {
      Class->hp_max = atoi( argument.c_str(  ) );
      ch->print( "Max HP gain set.\r\n" );
      write_class_file( cl );
      return;
   }

   if( !str_cmp( arg2, "mana" ) )
   {
      if( UPPER( argument[0] ) == 'Y' )
         Class->fMana = true;
      else
         Class->fMana = false;
      ch->print( "Mana flag toggled.\r\n" );
      write_class_file( cl );
      return;
   }

   if( !str_cmp( arg2, "ntitle" ) )
   {
      string arg3;
      int x;

      argument = one_argument( argument, arg3 );
      if( arg3.empty(  ) || argument.empty(  ) )
      {
         ch->print( "Syntax: setclass <Class> ntitle <level> <title>\r\n" );
         return;
      }
      if( ( x = atoi( arg3.c_str(  ) ) ) < 0 || x > MAX_LEVEL )
      {
         ch->print( "Invalid level.\r\n" );
         return;
      }
      STRFREE( title_table[cl][x][SEX_NEUTRAL] );
      title_table[cl][x][SEX_NEUTRAL] = STRALLOC( argument.c_str(  ) );
      ch->print( "Done.\r\n" );
      write_class_file( cl );
      return;
   }

   if( !str_cmp( arg2, "mtitle" ) )
   {
      string arg3;
      int x;

      argument = one_argument( argument, arg3 );
      if( arg3.empty(  ) || argument.empty(  ) )
      {
         ch->print( "Syntax: setclass <Class> mtitle <level> <title>\r\n" );
         return;
      }
      if( ( x = atoi( arg3.c_str(  ) ) ) < 0 || x > MAX_LEVEL )
      {
         ch->print( "Invalid level.\r\n" );
         return;
      }
      STRFREE( title_table[cl][x][SEX_MALE] );
      title_table[cl][x][SEX_MALE] = STRALLOC( argument.c_str(  ) );
      ch->print( "Done.\r\n" );
      write_class_file( cl );
      return;
   }

   if( !str_cmp( arg2, "ftitle" ) )
   {
      string arg3, arg4;
      int x;

      argument = one_argument( argument, arg3 );
      argument = one_argument( argument, arg4 );
      if( arg3.empty(  ) || argument.empty(  ) )
      {
         ch->print( "Syntax: setclass <Class> ftitle <level> <title>\r\n" );
         return;
      }
      if( ( x = atoi( arg4.c_str(  ) ) ) < 0 || x > MAX_LEVEL )
      {
         ch->print( "Invalid level.\r\n" );
         return;
      }
      STRFREE( title_table[cl][x][SEX_FEMALE] );
      title_table[cl][x][SEX_FEMALE] = STRALLOC( argument.c_str(  ) );
      ch->print( "Done\r\n" );
      write_class_file( cl );
      return;
   }

   if( !str_cmp( arg2, "htitle" ) )
   {
      string arg3, arg4;
      int x;

      argument = one_argument( argument, arg3 );
      argument = one_argument( argument, arg4 );
      if( arg3.empty(  ) || argument.empty(  ) )
      {
         ch->print( "Syntax: setclass <Class> htitle <level> <title>\r\n" );
         return;
      }
      if( ( x = atoi( arg4.c_str(  ) ) ) < 0 || x > MAX_LEVEL )
      {
         ch->print( "Invalid level.\r\n" );
         return;
      }
      STRFREE( title_table[cl][x][SEX_HERMAPHRODYTE] );
      title_table[cl][x][SEX_HERMAPHRODYTE] = STRALLOC( argument.c_str(  ) );
      ch->print( "Done\r\n" );
      write_class_file( cl );
      return;
   }
   do_setclass( ch, "" );
}

race_type::race_type(  )
{
   int i;

   init_memory( &affected, &hp_regen, sizeof( hp_regen ) );
   for( i = 0; i < MAX_WHERE_NAME; ++i )
      where_name[i] = STRALLOC( where_names[i] );
}

race_type::~race_type(  )
{
   int loopa;

   for( loopa = 0; loopa < MAX_WHERE_NAME; ++loopa )
      STRFREE( where_name[loopa] );
   STRFREE( race_name );
}

void free_all_races( void )
{
   int rc;

   for( rc = 0; rc < MAX_RACE; ++rc )
   {
      race_type *race = race_table[rc];
      deleteptr( race );
   }
}

bool load_race_file( const char *fname )
{
   char buf[256];
   race_type *race;
   int ra = -1, wear = 0, file_ver = 0;
   FILE *fp;

   snprintf( buf, 256, "%s%s", RACE_DIR, fname );
   if( !( fp = fopen( buf, "r" ) ) )
   {
      perror( buf );
      return false;
   }

   race = new race_type;
   race->minalign = -1000;
   race->maxalign = 1000;

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

         case 'A':
            KEY( "Align", race->alignment, fread_number( fp ) );
            KEY( "AC_Plus", race->ac_plus, fread_number( fp ) );
            if( !str_cmp( word, "Affected" ) )
            {
               if( file_ver < 1 )
               {
                  race->affected = fread_number( fp );
                  if( race->affected.any(  ) )
                     race->affected <<= 1;
               }
               else
                  flag_set( fp, race->affected, aff_flags );
               break;
            }
            if( !str_cmp( word, "Attacks" ) )
            {
               if( file_ver < 1 )
                  race->attacks = fread_number( fp );
               else
                  flag_set( fp, race->attacks, attack_flags );
               break;
            }
            break;

         case 'B':
            if( !str_cmp( word, "Bodyparts" ) )
            {
               if( file_ver < 1 )
                  race->body_parts = fread_number( fp );
               else
                  flag_set( fp, race->body_parts, part_flags );
               break;
            }
            break;

         case 'C':
            KEY( "Con_Plus", race->con_plus, fread_number( fp ) );
            KEY( "Cha_Plus", race->cha_plus, fread_number( fp ) );
            if( !str_cmp( word, "Classes" ) )
            {
               if( file_ver < 1 )
                  race->class_restriction = fread_number( fp );
               else
                  flag_set( fp, race->class_restriction, npc_class );
               break;
            }
            break;

         case 'D':
            KEY( "Dex_Plus", race->dex_plus, fread_number( fp ) );
            if( !str_cmp( word, "Defenses" ) )
            {
               if( file_ver < 1 )
                  race->defenses = fread_number( fp );
               else
                  flag_set( fp, race->defenses, defense_flags );
               break;
            }
            break;

         case 'E':
            if( !str_cmp( word, "End" ) )
            {
               FCLOSE( fp );
               if( ra < 0 || ra >= MAX_RACE )
               {
                  bug( "%s: Race (%s) bad/not found (%d)", __FUNCTION__, race->race_name ? race->race_name : "name not found", ra );
                  deleteptr( race );
                  return false;
               }
               race_table[ra] = race;
               return true;
            }

            KEY( "Exp_Mult", race->exp_multiplier, fread_number( fp ) );
            break;

         case 'I':
            KEY( "Int_Plus", race->int_plus, fread_number( fp ) );
            break;

         case 'H':
            KEY( "Height", race->height, fread_number( fp ) );
            KEY( "Hit", race->hit, fread_number( fp ) );
            KEY( "HP_Regen", race->hp_regen, fread_number( fp ) );
            KEY( "Hunger_Mod", race->hunger_mod, fread_number( fp ) );
            break;

         case 'L':
            if( !str_cmp( word, "Language" ) )
            {
               if( file_ver < 1 )
                  race->language = fread_number( fp );
               else
                  flag_set( fp, race->language, lang_names );
               break;
            }
            KEY( "Lck_Plus", race->lck_plus, fread_number( fp ) );
            break;

         case 'M':
            KEY( "Mana", race->mana, fread_number( fp ) );
            KEY( "Mana_Regen", race->mana_regen, fread_number( fp ) );
            KEY( "Min_Align", race->minalign, fread_number( fp ) );
            KEY( "Max_Align", race->maxalign, fread_number( fp ) );
            break;

         case 'N':
            KEY( "Name", race->race_name, fread_string( fp ) );
            break;

         case 'R':
            KEY( "Race", ra, fread_number( fp ) );
            if( !str_cmp( word, "Resist" ) )
            {
               if( file_ver < 1 )
                  race->resist = fread_number( fp );
               else
                  flag_set( fp, race->resist, ris_flags );
               break;
            }
            break;

         case 'S':
            KEY( "Str_Plus", race->str_plus, fread_number( fp ) );
            if( !str_cmp( word, "Suscept" ) )
            {
               if( file_ver < 1 )
                  race->suscept = fread_number( fp );
               else
                  flag_set( fp, race->suscept, ris_flags );
               break;
            }

            if( !str_cmp( word, "Skill" ) )
            {
               int sn, lev, adp;

               word = fread_word( fp );
               lev = fread_number( fp );
               adp = fread_number( fp );
               sn = skill_lookup( word );
               if( ra < 0 || ra >= MAX_RACE )
                  bug( "%s: Skill %s -- race bad/not found (%d)", __FUNCTION__, word, ra );
               else if( !IS_VALID_SN( sn ) )
               {
                  bug( "%s: skill %s = SN %d", __FUNCTION__, word, sn );
                  log_printf( "Skill %s unknown", word );
               }
               else
               {
                  skill_table[sn]->race_level[ra] = lev;
                  skill_table[sn]->race_adept[ra] = adp;
               }
               break;
            }
            break;

         case 'T':
            KEY( "Thirst_Mod", race->thirst_mod, fread_number( fp ) );
            break;

         case 'V':
            KEY( "Version", file_ver, fread_number( fp ) );
            break;

         case 'W':
            KEY( "Weight", race->weight, fread_number( fp ) );
            KEY( "Wis_Plus", race->wis_plus, fread_number( fp ) );
            if( !str_cmp( word, "WhereName" ) )
            {
               if( ra < 0 || ra >= MAX_RACE )
               {
                  bug( "%s: WhereName -- race bad/not found (%d)", __FUNCTION__, ra );
                  fread_flagstring( fp );
                  fread_flagstring( fp );
               }
               else if( wear < MAX_WHERE_NAME )
               {
                  STRFREE( race->where_name[wear] );
                  race->where_name[wear] = fread_string( fp );
                  ++wear;
               }
               else
                  bug( "%s: Too many where_names", __FUNCTION__ );
               break;
            }
            break;
      }
   }
}

/*
 * Load in all the race files.
 */
void load_races(  )
{
   FILE *fpList;
   const char *filename;
   char racelist[256];

   MAX_PC_RACE = 0;
   /*
    * Pre-init the race_table with blank races
    */
   for( int i = 0; i < MAX_RACE; ++i )
      race_table[i] = NULL;

   snprintf( racelist, 256, "%s%s", RACE_DIR, RACE_LIST );
   if( !( fpList = fopen( racelist, "r" ) ) )
   {
      perror( racelist );
      exit( 1 );
   }

   for( ;; )
   {
      filename = ( feof( fpList ) ? "$" : fread_word( fpList ) );

      if( filename[0] == '\0' )
      {
         bug( "%s: EOF encountered reading file!", __FUNCTION__ );
         break;
      }

      if( filename[0] == '$' )
         break;

      if( !load_race_file( filename ) )
         bug( "%s: Cannot load race file: %s", __FUNCTION__, filename );
      else
         ++MAX_PC_RACE;
   }
   for( int i = 0; i < MAX_RACE; ++i )
   {
      if( !race_table[i] )
      {
         race_type *race = new race_type;

         race->race_name = STRALLOC( "Unused Race" );
         race_table[i] = race;
      }
   }
   FCLOSE( fpList );
}

void write_race_file( int ra )
{
   FILE *fpout;
   char filename[256];
   race_type *race = race_table[ra];

   if( !race->race_name )
   {
      bug( "Race %d has null name, not writing .race file.", ra );
      return;
   }

   snprintf( filename, 256, "%s%s.race", RACE_DIR, race->race_name );
   if( !( fpout = fopen( filename, "w" ) ) )
   {
      bug( "Cannot open: %s for writing", filename );
      return;
   }
   fprintf( fpout, "Version     %d\n", RACEFILEVER );
   fprintf( fpout, "Name        %s~\n", race->race_name );
   fprintf( fpout, "Race        %d\n", ra );
   fprintf( fpout, "Classes     %s~\n", bitset_string( race->class_restriction, npc_class ) );
   fprintf( fpout, "Str_Plus    %d\n", race->str_plus );
   fprintf( fpout, "Dex_Plus    %d\n", race->dex_plus );
   fprintf( fpout, "Wis_Plus    %d\n", race->wis_plus );
   fprintf( fpout, "Int_Plus    %d\n", race->int_plus );
   fprintf( fpout, "Con_Plus    %d\n", race->con_plus );
   fprintf( fpout, "Cha_Plus    %d\n", race->cha_plus );
   fprintf( fpout, "Lck_Plus    %d\n", race->lck_plus );
   fprintf( fpout, "Hit         %d\n", race->hit );
   fprintf( fpout, "Mana        %d\n", race->mana );
   if( race->affected.any(  ) )
      fprintf( fpout, "Affected    %s~\n", bitset_string( race->affected, aff_flags ) );
   if( race->resist.any(  ) )
      fprintf( fpout, "Resist      %s~\n", bitset_string( race->resist, ris_flags ) );
   if( race->suscept.any(  ) )
      fprintf( fpout, "Suscept     %s~\n", bitset_string( race->suscept, ris_flags ) );
   if( race->language.any(  ) )
      fprintf( fpout, "Language    %s~\n", bitset_string( race->language, lang_names ) );
   fprintf( fpout, "Align       %d\n", race->alignment );
   fprintf( fpout, "Min_Align  %d\n", race->minalign );
   fprintf( fpout, "Max_Align	 %d\n", race->maxalign );
   fprintf( fpout, "AC_Plus    %d\n", race->ac_plus );
   fprintf( fpout, "Exp_Mult   %d\n", race->exp_multiplier );
   if( race->body_parts.any(  ) )
      fprintf( fpout, "Bodyparts  %s~\n", bitset_string( race->body_parts, part_flags ) );
   if( race->attacks.any(  ) )
      fprintf( fpout, "Attacks    %s~\n", bitset_string( race->attacks, attack_flags ) );
   if( race->defenses.any(  ) )
      fprintf( fpout, "Defenses   %s~\n", bitset_string( race->defenses, defense_flags ) );
   fprintf( fpout, "Height     %d\n", race->height );
   fprintf( fpout, "Weight     %d\n", race->weight );
   fprintf( fpout, "Hunger_Mod  %d\n", race->hunger_mod );
   fprintf( fpout, "Thirst_mod  %d\n", race->thirst_mod );
   fprintf( fpout, "Mana_Regen  %d\n", race->mana_regen );
   fprintf( fpout, "HP_Regen    %d\n", race->hp_regen );
   for( int i = 0; i < MAX_WHERE_NAME; ++i )
      fprintf( fpout, "WhereName  %s~\n", race->where_name[i] );

   for( int x = 0; x < num_skills; ++x )
   {
      int y;

      if( !skill_table[x]->name || skill_table[x]->name[0] == '\0' )
         break;
      if( ( y = skill_table[x]->race_level[ra] ) < LEVEL_IMMORTAL )
         fprintf( fpout, "Skill '%s' %d %d\n", skill_table[x]->name, y, skill_table[x]->race_adept[ra] );
   }
   fprintf( fpout, "%s", "End\n" );
   FCLOSE( fpout );
}

void save_races(  )
{
   for( int x = 0; x < MAX_PC_RACE; ++x )
      write_race_file( x );
}

void write_race_list(  )
{
   FILE *fpList;
   char racelist[256];

   snprintf( racelist, 256, "%s%s", RACE_DIR, RACE_LIST );
   if( !( fpList = fopen( racelist, "w" ) ) )
   {
      bug( "%s: Error opening racelist.", __FUNCTION__ );
      return;
   }
   for( int i = 0; i < MAX_PC_RACE; ++i )
      fprintf( fpList, "%s.race\n", race_table[i]->race_name );
   fprintf( fpList, "%s", "$\n" );
   FCLOSE( fpList );
}

/*
 * Create an instance of a new race. - Shaddai
 */
bool create_new_race( int race, const string & argument )
{
   if( race >= MAX_RACE || race_table[race] == NULL )
      return false;
   for( int i = 0; i < MAX_WHERE_NAME; ++i )
      race_table[race]->where_name[i] = STRALLOC( where_names[i] );
   race_table[race]->race_name = STRALLOC( capitalize( argument ).c_str(  ) );
   race_table[race]->class_restriction.reset(  );
   race_table[race]->str_plus = 0;
   race_table[race]->dex_plus = 0;
   race_table[race]->wis_plus = 0;
   race_table[race]->int_plus = 0;
   race_table[race]->con_plus = 0;
   race_table[race]->cha_plus = 0;
   race_table[race]->lck_plus = 0;
   race_table[race]->hit = 0;
   race_table[race]->mana = 0;
   race_table[race]->affected.reset(  );
   race_table[race]->resist.reset(  );
   race_table[race]->suscept.reset(  );
   race_table[race]->language.reset(  );
   race_table[race]->alignment = 0;
   race_table[race]->minalign = 0;
   race_table[race]->maxalign = 0;
   race_table[race]->ac_plus = 0;
   race_table[race]->exp_multiplier = 0;
   race_table[race]->attacks.reset(  );
   race_table[race]->defenses.reset(  );
   race_table[race]->body_parts.reset(  );
   race_table[race]->height = 0;
   race_table[race]->weight = 0;
   race_table[race]->hunger_mod = 0;
   race_table[race]->thirst_mod = 0;
   race_table[race]->mana_regen = 0;
   race_table[race]->hp_regen = 0;
   return true;
}

CMDF( do_setrace )
{
   race_type *race;
   string arg1, arg2, arg3;
   int value, ra = 0, i;

   ch->set_color( AT_IMMORT );

   smash_tilde( argument );
   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );
   if( arg1.empty(  ) || arg2.empty(  ) )
   {
      ch->print( "Syntax: setrace <race> <field> <value>\r\n" );
      ch->print( "Syntax: setrace <race> create\r\n" );
      ch->print( "\r\nField being one of:\r\n" );
      ch->print( "  name classes strplus dexplus wisplus\r\n" );
      ch->print( "  intplus conplus chaplus lckplus hit\r\n" );
      ch->print( "  mana affected resist suscept language\r\n" );
      ch->print( "  attack defense alignment acplus \r\n" );
      ch->print( "  minalign maxalign height weight\r\n" );
      ch->print( "  hungermod thirstmod expmultiplier\r\n" );
      ch->print( "  saving_poison_death saving_wand\r\n" );
      ch->print( "  saving_para_petri saving_breath\r\n" );
      ch->print( "  saving_spell_staff bodyparts\r\n" );
      ch->print( "  mana_regen hp_regen\r\n" );
      return;
   }
   if( is_number( arg1 ) && ( ra = atoi( arg1.c_str(  ) ) ) >= 0 && ra < MAX_RACE )
      race = race_table[ra];
   else
   {
      race = NULL;
      for( ra = 0; ra < MAX_RACE && race_table[ra]; ++ra )
      {
         if( !race_table[ra]->race_name )
            continue;

         if( !str_cmp( race_table[ra]->race_name, arg1 ) )
         {
            race = race_table[ra];
            break;
         }
      }
   }

   if( !str_cmp( arg2, "create" ) && race )
   {
      ch->print( "That race already exists!\r\n" );
      return;
   }
   else if( !race && str_cmp( arg2, "create" ) )
   {
      ch->print( "No such race.\r\n" );
      return;
   }

   if( !str_cmp( arg2, "create" ) )
   {
      char filename[256];

      if( MAX_PC_RACE >= MAX_RACE )
      {
         ch->print( "You need to up MAX_RACE in mud.h and make clean.\r\n" );
         return;
      }

      snprintf( filename, sizeof( filename ), "%s.race", arg1.c_str(  ) );
      if( !is_valid_filename( ch, RACE_DIR, filename ) )
         return;

      if( ( create_new_race( MAX_PC_RACE, arg1 ) ) == false )
      {
         ch->print( "Couldn't create a new race.\r\n" );
         return;
      }
      write_race_file( MAX_PC_RACE );
      ++MAX_PC_RACE;
      write_race_list(  );
      ch->print( "Done.\r\n" );
      return;
   }

   if( argument.empty(  ) )
   {
      ch->print( "You must specify an argument.\r\n" );
      return;
   }

   if( !str_cmp( arg2, "name" ) )
   {
      char buf[256];
      race_type *rcheck = NULL;

      one_argument( argument, arg1 );

      if( arg1.empty(  ) )
      {
         ch->print( "You can't set a race name to nothing.\r\n" );
         return;
      }

      snprintf( buf, sizeof( buf ), "%s.race", arg1.c_str(  ) );
      if( !is_valid_filename( ch, RACE_DIR, buf ) )
         return;

      for( i = 0; i < MAX_PC_RACE && race_table[i]; ++i )
      {
         if( !race_table[i]->race_name )
            continue;

         if( !str_cmp( race_table[i]->race_name, arg1 ) )
         {
            rcheck = race_table[i];
            break;
         }
      }
      if( rcheck != NULL )
      {
         ch->printf( "Already a race called %s.\r\n", arg1.c_str(  ) );
         return;
      }

      snprintf( buf, sizeof( buf ), "%s%s.race", RACE_DIR, race->race_name );
      unlink( buf );

      STRFREE( race->race_name );
      race->race_name = STRALLOC( capitalize( argument ).c_str(  ) );

      write_race_file( ra );
      write_race_list(  );
      ch->print( "Race name set.\r\n" );
      return;
   }

   if( !str_cmp( arg2, "strplus" ) )
   {
      race->str_plus = ( short )atoi( argument.c_str(  ) );
      write_race_file( ra );
      ch->print( "Done.\r\n" );
      return;
   }
   if( !str_cmp( arg2, "dexplus" ) )
   {
      race->dex_plus = ( short )atoi( argument.c_str(  ) );
      write_race_file( ra );
      ch->print( "Done.\r\n" );
      return;
   }
   if( !str_cmp( arg2, "wisplus" ) )
   {
      race->wis_plus = ( short )atoi( argument.c_str(  ) );
      write_race_file( ra );
      ch->print( "Done.\r\n" );
      return;
   }
   if( !str_cmp( arg2, "intplus" ) )
   {
      race->int_plus = ( short )atoi( argument.c_str(  ) );
      write_race_file( ra );
      ch->print( "Done.\r\n" );
      return;
   }
   if( !str_cmp( arg2, "conplus" ) )
   {
      race->con_plus = ( short )atoi( argument.c_str(  ) );
      write_race_file( ra );
      ch->print( "Done.\r\n" );
      return;
   }
   if( !str_cmp( arg2, "chaplus" ) )
   {
      race->cha_plus = ( short )atoi( argument.c_str(  ) );
      write_race_file( ra );
      ch->print( "Done.\r\n" );
      return;
   }
   if( !str_cmp( arg2, "lckplus" ) )
   {
      race->lck_plus = ( short )atoi( argument.c_str(  ) );
      write_race_file( ra );
      ch->print( "Done.\r\n" );
      return;
   }
   if( !str_cmp( arg2, "hit" ) )
   {
      race->hit = ( short )atoi( argument.c_str(  ) );
      write_race_file( ra );
      ch->print( "Done.\r\n" );
      return;
   }
   if( !str_cmp( arg2, "mana" ) )
   {
      race->mana = ( short )atoi( argument.c_str(  ) );
      write_race_file( ra );
      ch->print( "Done.\r\n" );
      return;
   }
   if( !str_cmp( arg2, "affected" ) )
   {
      if( argument.empty(  ) )
      {
         ch->print( "Usage: setrace <race> affected <flag> [flag]...\r\n" );
         return;
      }
      while( !argument.empty(  ) )
      {
         argument = one_argument( argument, arg3 );
         value = get_aflag( arg3 );
         if( value < 0 || value >= MAX_AFFECTED_BY )
            ch->printf( "Unknown flag: %s\r\n", arg3.c_str(  ) );
         else
            race->affected.flip( value );
      }
      write_race_file( ra );
      ch->print( "Racial affects set.\r\n" );
      return;
   }

   if( !str_cmp( arg2, "bodyparts" ) || str_cmp( arg2, "part" ) )
   {
      if( argument.empty(  ) )
      {
         ch->print( "Usage: setrace <race> bodypart <part> [part]...\r\n" );
         return;
      }
      while( !argument.empty(  ) )
      {
         argument = one_argument( argument, arg3 );
         value = get_partflag( arg3 );
         if( value < 0 || value >= MAX_BPART )
            ch->printf( "Unknown part: %s\r\n", arg3.c_str(  ) );
         else
            race->body_parts.flip( value );
      }
      write_race_file( ra );
      ch->print( "Racial body parts set.\r\n" );
      return;
   }

   if( !str_cmp( arg2, "resist" ) )
   {
      if( argument.empty(  ) )
      {
         ch->print( "Usage: setrace <race> resist <flag> [flag]...\r\n" );
         return;
      }
      while( !argument.empty(  ) )
      {
         argument = one_argument( argument, arg3 );
         value = get_risflag( arg3 );
         if( value < 0 || value >= MAX_RIS_FLAG )
            ch->printf( "Unknown flag: %s\r\n", arg3.c_str(  ) );
         else
            race->resist.flip( value );
      }
      write_race_file( ra );
      ch->print( "Racial resistances set.\r\n" );
      return;
   }

   if( !str_cmp( arg2, "suscept" ) )
   {
      if( argument.empty(  ) )
      {
         ch->print( "Usage: setrace <race> suscept <flag> [flag]...\r\n" );
         return;
      }
      while( !argument.empty(  ) )
      {
         argument = one_argument( argument, arg3 );
         value = get_risflag( arg3 );
         if( value < 0 || value >= MAX_RIS_FLAG )
            ch->printf( "Unknown flag: %s\r\n", arg3.c_str(  ) );
         else
            race->suscept.flip( value );
      }
      write_race_file( ra );
      ch->print( "Racial susceptabilities set.\r\n" );
      return;
   }

   if( !str_cmp( arg2, "language" ) )
   {
      argument = one_argument( argument, arg3 );
      value = get_langnum( arg3 );
      if( value < 0 || value >= LANG_UNKNOWN )
      {
         ch->printf( "Unknown language: %s\r\n", arg3.c_str(  ) );
         return;
      }
      else if( !( value &= VALID_LANGS ) )
      {
         ch->printf( "Player races may not speak %s.\r\n", arg3.c_str(  ) );
         return;
      }
      race->language.flip( value );
      write_race_file( ra );
      ch->print( "Racial language set.\r\n" );
      return;
   }

   if( !str_cmp( arg2, "classes" ) )
   {
      for( i = 0; i < MAX_CLASS; ++i )
      {
         if( !str_cmp( argument, class_table[i]->who_name ) )
         {
            race->class_restriction.flip( i );  /* k, that's boggling */
            write_race_file( ra );
            ch->print( "Classes set.\r\n" );
            return;
         }
      }
      ch->print( "No such class.\r\n" );
      return;
   }

   if( !str_cmp( arg2, "acplus" ) )
   {
      race->ac_plus = atoi( argument.c_str(  ) );
      write_race_file( ra );
      ch->print( "Done.\r\n" );
      return;
   }

   if( !str_cmp( arg2, "alignment" ) )
   {
      race->alignment = atoi( argument.c_str(  ) );
      write_race_file( ra );
      ch->print( "Done.\r\n" );
      return;
   }

   /*
    * not implemented 
    */
   if( !str_cmp( arg2, "defense" ) )
   {
      if( argument.empty(  ) )
      {
         ch->print( "Usage: setrace <race> defense <flag> [flag]...\r\n" );
         return;
      }
      while( !argument.empty(  ) )
      {
         argument = one_argument( argument, arg3 );
         value = get_defenseflag( arg3 );
         if( value < 0 || value >= MAX_DEFENSE_TYPE )
            ch->printf( "Unknown flag: %s\r\n", arg3.c_str(  ) );
         else
            race->defenses.flip( value );
      }
      write_race_file( ra );
      return;
   }

   /*
    * not implemented 
    */
   if( !str_cmp( arg2, "attack" ) )
   {
      if( argument.empty(  ) )
      {
         ch->print( "Usage: setrace <race> attack <flag> [flag]...\r\n" );
         return;
      }
      while( !argument.empty(  ) )
      {
         argument = one_argument( argument, arg3 );
         value = get_attackflag( arg3 );
         if( value < 0 || value >= MAX_ATTACK_TYPE )
            ch->printf( "Unknown flag: %s\r\n", arg3.c_str(  ) );
         else
            race->attacks.flip( value );
      }
      write_race_file( ra );
      return;
   }

   if( !str_cmp( arg2, "minalign" ) )
   {
      race->minalign = atoi( argument.c_str(  ) );
      write_race_file( ra );
      ch->print( "Done.\r\n" );
      return;
   }
   if( !str_cmp( arg2, "maxalign" ) )
   {
      race->maxalign = atoi( argument.c_str(  ) );
      write_race_file( ra );
      ch->print( "Done.\r\n" );
      return;
   }
   if( !str_cmp( arg2, "height" ) )
   {
      race->height = atoi( argument.c_str(  ) );
      write_race_file( ra );
      ch->print( "Done.\r\n" );
      return;
   }
   if( !str_cmp( arg2, "weight" ) )
   {
      race->weight = atoi( argument.c_str(  ) );
      write_race_file( ra );
      ch->print( "Done.\r\n" );
      return;
   }
   if( !str_cmp( arg2, "thirstmod" ) )
   {
      race->thirst_mod = atoi( argument.c_str(  ) );
      write_race_file( ra );
      ch->print( "Done.\r\n" );
      return;
   }
   if( !str_cmp( arg2, "hungermod" ) )
   {
      race->hunger_mod = atoi( argument.c_str(  ) );
      write_race_file( ra );
      ch->print( "Done.\r\n" );
      return;
   }
   if( !str_cmp( arg2, "maxalign" ) )
   {
      race->maxalign = atoi( argument.c_str(  ) );
      write_race_file( ra );
      ch->print( "Done.\r\n" );
      return;
   }
   if( !str_cmp( arg2, "expmultiplier" ) )
   {
      race->exp_multiplier = atoi( argument.c_str(  ) );
      write_race_file( ra );
      ch->print( "Done.\r\n" );
      return;
   }
   if( !str_cmp( arg2, "saving_poison_death" ) )
   {
      race->saving_poison_death = atoi( argument.c_str(  ) );
      write_race_file( ra );
      ch->print( "Done.\r\n" );
      return;
   }
   if( !str_cmp( arg2, "saving_wand" ) )
   {
      race->saving_wand = atoi( argument.c_str(  ) );
      write_race_file( ra );
      ch->print( "Done.\r\n" );
      return;
   }
   if( !str_cmp( arg2, "saving_para_petri" ) )
   {
      race->saving_para_petri = atoi( argument.c_str(  ) );
      write_race_file( ra );
      ch->print( "Done.\r\n" );
      return;
   }
   if( !str_cmp( arg2, "saving_breath" ) )
   {
      race->saving_breath = atoi( argument.c_str(  ) );
      write_race_file( ra );
      ch->print( "Done.\r\n" );
      return;
   }
   if( !str_cmp( arg2, "saving_spell_staff" ) )
   {
      race->saving_spell_staff = atoi( argument.c_str(  ) );
      write_race_file( ra );
      ch->print( "Done.\r\n" );
      return;
   }
   /*
    * unimplemented stuff follows 
    */
   if( !str_cmp( arg2, "mana_regen" ) )
   {
      race->mana_regen = atoi( argument.c_str(  ) );
      write_race_file( ra );
      ch->print( "Done.\r\n" );
      return;
   }
   if( !str_cmp( arg2, "hp_regen" ) )
   {
      race->hp_regen = atoi( argument.c_str(  ) );
      write_race_file( ra );
      ch->print( "Done.\r\n" );
      return;
   }
   do_setrace( ch, "" );
}

CMDF( do_showrace )
{
   race_type *race;
   int ra = 0, i, ct;

   ch->set_pager_color( AT_PLAIN );

   if( argument.empty(  ) )
   {
      ch->print( "Syntax: showrace <race name>\r\n" );
      /*
       * Show the races code addition by Blackmane 
       */
      /*
       * fixed printout by Miki 
       */
      ct = 0;
      for( i = 0; i < MAX_RACE; ++i )
      {
         ++ct;
         ch->pagerf( "%2d> %-11s", i, race_table[i]->race_name );
         if( ct % 5 == 0 )
            ch->pager( "\r\n" );
      }
      ch->pager( "\r\n" );
      return;
   }
   if( is_number( argument ) && ( ra = atoi( argument.c_str(  ) ) ) >= 0 && ra < MAX_RACE )
      race = race_table[ra];
   else
   {
      race = NULL;
      for( ra = 0; ra < MAX_RACE && race_table[ra]; ++ra )
         if( !str_cmp( race_table[ra]->race_name, argument ) )
         {
            race = race_table[ra];
            break;
         }
   }
   if( !race )
   {
      ch->print( "No such race.\r\n" );
      return;
   }

   ch->pagerf( "RACE: %s\r\n", race->race_name );
   ct = 0;
   ch->pager( "Disallowed Classes: " );
   for( i = 0; i < MAX_CLASS; ++i )
   {
      if( race->class_restriction.test( i ) )
      {
         ++ct;
         ch->pagerf( "%s ", class_table[i]->who_name );
         if( ct % 6 == 0 )
            ch->pager( "\r\n" );
      }
   }
   if( ( ct % 6 != 0 ) || ( ct == 0 ) )
      ch->pager( "\r\n" );

   ct = 0;
   ch->pager( "Allowed Classes: " );
   for( i = 0; i < MAX_CLASS; ++i )
   {
      if( !race->class_restriction.test( i ) )
      {
         ++ct;
         ch->pagerf( "%s ", class_table[i]->who_name );
         if( ct % 6 == 0 )
            ch->pager( "\r\n" );
      }
   }
   if( ( ct % 6 != 0 ) || ( ct == 0 ) )
      ch->pager( "\r\n" );

   ch->pagerf( "Str Plus: %-3d\tDex Plus: %-3d\tWis Plus: %-3d\tInt Plus: %-3d\t\r\n", race->str_plus, race->dex_plus, race->wis_plus, race->int_plus );

   ch->pagerf( "Con Plus: %-3d\tCha Plus: %-3d\tLck Plus: %-3d\r\n", race->con_plus, race->cha_plus, race->lck_plus );

   ch->pagerf( "Hit Pts:  %-3d\tMana: %-3d\tAlign: %-4d\tAC: %-d\r\n", race->hit, race->mana, race->alignment, race->ac_plus );

   ch->pagerf( "Min Align: %d\tMax Align: %-d\t\tXP Mult: %-d%%\r\n", race->minalign, race->maxalign, race->exp_multiplier );

   ch->pagerf( "Height: %3d in.\t\tWeight: %4d lbs.\tHungerMod: %d\tThirstMod: %d\r\n", race->height, race->weight, race->hunger_mod, race->thirst_mod );

   ch->pagerf( "Spoken Languages: %s\r\n", bitset_string( race->language, lang_names ) );
   ch->pagerf( "Affected by: %s\r\n", bitset_string( race->affected, aff_flags ) );
   ch->pagerf( "Resistant to: %s\r\n", bitset_string( race->resist, ris_flags ) );
   ch->pagerf( "Susceptible to: %s\r\n", bitset_string( race->suscept, ris_flags ) );

   ch->pagerf( "Saves: (P/D) %d (W) %d (P/P) %d (B) %d (S/S) %d\r\n",
               race->saving_poison_death, race->saving_wand, race->saving_para_petri, race->saving_breath, race->saving_spell_staff );

   ch->pagerf( "Innate Attacks: %s\r\n", bitset_string( race->attacks, attack_flags ) );
   ch->pagerf( "Innate Defenses: %s\r\n", bitset_string( race->defenses, defense_flags ) );
}

/* Simple, small way to make keeping track of small mods easier - Blod */
CMDF( do_fixed )
{
   struct tm *t = localtime( &current_time );

   ch->set_color( AT_OBJECT );
   if( argument.empty(  ) )
   {
      ch->print( "\r\nUsage:  'fixed list' or 'fixed <message>'" );
      if( ch->get_trust(  ) >= LEVEL_ASCENDANT )
         ch->print( " or 'fixed clear now'\r\n" );
      else
         ch->print( "\r\n" );
      return;
   }
   if( !str_cmp( argument, "clear now" ) && ch->get_trust(  ) >= LEVEL_ASCENDANT )
   {
      FILE *fp = fopen( FIXED_FILE, "w" );
      if( fp )
         FCLOSE( fp );
      ch->print( "Fixed file cleared.\r\n" );
      return;
   }
   if( !str_cmp( argument, "list" ) )
   {
      ch->print( "\r\n&g[&GDate  &g|  &GVnum&g]\r\n" );
      show_file( ch, FIXED_FILE );
   }
   else
   {
      append_to_file( FIXED_FILE, "&g|&G%-2.2d/%-2.2d &g| &G%5d&g|  %s:  &G%s",
                      t->tm_mon + 1, t->tm_mday, ch->in_room ? ch->in_room->vnum : 0, ch->isnpc(  )? ch->short_descr : ch->name, argument.c_str(  ) );
      ch->print( "Thanks, your modification has been logged.\r\n" );
   }
}

/*Added by Adjani, 12-23-02. Plan to add more to this, but for now it works. */
CMDF( do_forgefind )
{
   ch->pager( "\r\n" );

   bool found = false;
   list < char_data * >::iterator ich;
   for( ich = charlist.begin(  ); ich != charlist.end(  ); ++ich )
   {
      char_data *smith = *ich;

      if( smith->has_actflag( ACT_SMITH ) || smith->has_actflag( ACT_GUILDFORGE ) )
      {
         found = true;
         ch->pagerf( "Forge: *%5d* %-30s at *%5d*, %s.\r\n", smith->pIndexData->vnum, smith->short_descr, smith->in_room->vnum, smith->in_room->name );
      }
   }
   if( !found )
      ch->print( "You didn't find any forges.\r\n" );
}
