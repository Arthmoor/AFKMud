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
 *                        Wizard/God Command Module                         *
 ****************************************************************************/

#include <filesystem>
#include "mud.h"
#include "area.h"
#include "calendar.h"
#include "clans.h"
#include "deity.h"
#include "descriptor.h"
#include "event.h"
#include "mobindex.h"
#include "mud_prog.h"
#include "objindex.h"
#include "overland.h"
#include "raceclass.h"
#include "roomindex.h"
#include "smaugaffect.h"
#include "variables.h"

/* External functions */
void set_chandler(  );
void unset_chandler(  );
void build_wizinfo(  );
void save_sysdata(  );
void write_race_file( int );
int calc_thac0( char_data *, char_data *, int );   /* For mstat */
CMDF( do_oldwhere );
CMDF( do_help );
CMDF( do_mfind );
CMDF( do_ofind );
CMDF( do_bestow );
bool in_arena( char_data * );
void check_stored_objects( char_data *, int );
void remove_from_auth( std::string_view );
void calc_season(  );
void ostat_plus( char_data *, obj_data *, bool );
void advance_level( char_data * );
void stop_hating( char_data * );
void stop_hunting( char_data * );
void stop_fearing( char_data * );
void check_clan_storeroom( char_data * );
bool check_parse_name( std::string, bool );
std::string password_hash( std::string_view );

/*
 * Global variables.
 */
#ifdef MULTIPORT
extern bool compilelock;
#endif
extern bool bootlock;
extern int reboot_counter;

class_type *class_table[MAX_CLASS];
race_type *race_table[MAX_RACE];
std::string title_table[MAX_CLASS][MAX_LEVEL + 1][SEX_MAX];
int MAX_PC_CLASS;
int MAX_PC_RACE;
std::chrono::system_clock::time_point last_restore_all_time;

/* Used to check and see if you should be using a command on a certain person - Samson 5-1-04 */
char_data *get_wizvictim( char_data * ch, std::string_view argument, bool nonpc )
{
   char_data *victim = nullptr;

   if( !( victim = ch->get_char_world( argument ) ) )
   {
      ch->print_fmt( "{} is not online.\r\n", argument );
      return nullptr;
   }

   if( victim->isnpc(  ) && nonpc )
   {
      ch->print( "You cannot use this command on NPC's.\r\n" );
      return nullptr;
   }

   if( victim->level >= ch->level && victim != ch )
   {
      ch->print_fmt( "You do not have sufficient access to affect {}.\r\n", victim->name );
      return nullptr;
   }
   return victim;
}

// Password resetting command, added by Samson 2-11-98
// Code courtesy of John Strange - Triad Mud
// Upgraded for OS independent SHA-256 encryption - Samson 1-7-06
// And one more time to OS independent SHA-512 encryption - Samson 6/14/2026
CMDF( do_newpassword )
{
   std::string arg1;
   char_data *victim;

   if( ch->isnpc(  ) )
      return;

   argument = one_argument( argument, arg1 );

   if( ( ch->pcdata->pwd[0] != '\0' ) && ( arg1.empty(  ) || argument.empty(  ) ) )
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

   std::string pwdnew = password_hash( argument ); // SHA-512 Encryption

   victim->pcdata->pwd = pwdnew;
   victim->save(  );
   ch->print_fmt( "&R{}'s password has been changed to: {}\r\n&w", victim->name, argument );
   victim->print_fmt( "&R{} has changed your password to: {}\r\n&w", ch->name, argument );
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
      act( AT_SOCIAL, "You jump in the air and highfive everyone in the room!", ch, nullptr, nullptr, TO_CHAR );
      act( AT_SOCIAL, "$n jumps in the air and highfives everyone in the room!", ch, nullptr, ch, TO_ROOM );
      return;
   }

   if( !( victim = ch->get_char_room( argument ) ) )
   {
      ch->print( "Nobody by that name is here.\r\n" );
      return;
   }

   if( victim == ch )
   {
      act( AT_SOCIAL, "Sometimes you really amaze yourself!", ch, nullptr, nullptr, TO_CHAR );
      act( AT_SOCIAL, "$n gives $mself a highfive!", ch, nullptr, ch, TO_ROOM );
      return;
   }

   if( victim->isnpc(  ) )
   {
      act( AT_SOCIAL, "You jump up and highfive $N!", ch, nullptr, victim, TO_CHAR );
      act( AT_SOCIAL, "$n jumps up and gives you a highfive!", ch, nullptr, victim, TO_VICT );
      act( AT_SOCIAL, "$n jumps up and gives $N a highfive!", ch, nullptr, victim, TO_NOTVICT );
      return;
   }

   if( victim->level > LEVEL_TRUEIMM && ch->level > LEVEL_TRUEIMM )
   {
      for( auto* vch : pclist )
      {
         if( vch == ch )
            act( AT_IMMORT, "The whole world rumbles as you highfive $N!", ch, nullptr, victim, TO_CHAR );
         else if( vch == victim )
            act( AT_IMMORT, "The whole world rumbles as $n highfives you!", ch, nullptr, victim, TO_VICT );
         else
            act( AT_IMMORT, "The whole world rumbles as $n and $N highfive!", ch, vch, victim, TO_THIRD );
      }
   }
   else
   {
      act( AT_SOCIAL, "You jump up and highfive $N!", ch, nullptr, victim, TO_CHAR );
      act( AT_SOCIAL, "$n jumps up and gives you a highfive!", ch, nullptr, victim, TO_VICT );
      act( AT_SOCIAL, "$n jumps up and gives $N a highfive!", ch, nullptr, victim, TO_NOTVICT );
   }
}

CMDF( do_bamfin )
{
   if( !ch->isnpc(  ) )
   {
      smash_tilde( argument );
      ch->pcdata->bamfin = argument;
      ch->print( "&YBamfin set.\r\n" );
   }
}

CMDF( do_bamfout )
{
   if( !ch->isnpc(  ) )
   {
      smash_tilde( argument );
      ch->pcdata->bamfout = argument;
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
   ch->pcdata->rank.clear();
   if( str_cmp( argument, "none" ) )
      ch->pcdata->rank = argument;
   ch->print_fmt( "&[immortal]Rank set to: {}\r\n", !ch->pcdata->rank.empty() ? ch->pcdata->rank : "None" );
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
      ch->print_fmt( "The minimum level for retirement is {}.\r\n", LEVEL_SAVIOR );
      return;
   }

   if( victim->has_pcflag( PCFLAG_RETIRED ) )
   {
      victim->unset_pcflag( PCFLAG_RETIRED );
      ch->print_fmt( "{} returns from retirement.\r\n", victim->name );
      victim->print_fmt( "{} brings you back from retirement.\r\n", ch->name );
   }
   else
   {
      victim->set_pcflag( PCFLAG_RETIRED );
      ch->print_fmt( "{} is now a retired immortal.\r\n", victim->name );
      victim->print_fmt( "Courtesy of {}, you are now a retired immortal.\r\n", ch->name );
   }
}

CMDF( do_delay )
{
   char_data *victim;
   std::string arg;
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
   delay = std::stoi( argument );
   if( delay < 1 )
   {
      ch->print( "Pointless. Try a positive number.\r\n" );
      return;
   }
   if( delay > 999 )
      ch->print( "You cruel bastard. Just kill them.\r\n" );

   victim->WAIT_STATE( delay * sysdata->pulseviolence );
   ch->print_fmt( "You've delayed {} for {} rounds.\r\n", victim->name, delay );
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
   ch->print_fmt( "You have denied access to {}.\r\n", victim->name );
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
      if( victim->desc == nullptr )
      {
         ch->print_fmt( "{} does not have a descriptor.\r\n", victim->name );
         return;
      }
      else
         desc = victim->desc->descriptor; /* Seems weird... but this seems faster? --X */
   }
   else
   {
      ch->print_fmt( "Disconnect: '{}' was not found!\r\n", argument );
      return;
   }

   for( auto* d : dlist )
   {
      if( d->descriptor == desc )
      {
         victim = d->original ? d->original : d->character;

         if( victim && victim->get_trust(  ) >= ch->get_trust(  ) )
         {
            ch->print( "You cannot disconnect that person.\r\n" );
            log_printf( "{} tried to disconnect {} but failed.", ch->name, !victim->name.empty() ? victim->name : "Someone" );
            return;
         }
         if( victim )
            log_printf( "{} will be disconnected {}", ch->name, !victim->name.empty() ? victim->name : "Someone" );
         else
            log_printf( "{} will be disconnected desc #{}", ch->name, desc );
         d->disconnect = true;
         ch->print( "Ok.\r\n" );
         return;
      }
   }
   bug( "{}: desc '{}' not found!", __func__, desc );
   ch->print( "Descriptor not found!\r\n" );
}

// The printf version of this is located in mud.h in the templates section.
void echo_to_all( std::string_view argument, short tar )
{
   if( argument.empty(  ) )
      return;

   for( auto* d : dlist )
   {
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
         else if( tar == ECHOTAR_PK && !d->character->IS_PKILL( ) )
            continue;

         d->character->print_fmt( "{}\r\n", argument );
      }
   }
}

CMDF( do_echo )
{
   std::string arg;
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

   std::string parg = argument;
   argument = one_argument( argument, arg );
   if( !str_cmp( arg, "PC" ) || !str_cmp( arg, "player" ) )
      target = ECHOTAR_PC;
   else if( !str_cmp( arg, "pk" ) )
      target = ECHOTAR_PK;
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
   echo_all_printf( ECHOTAR_ALL, "&[immortal]The Voice of God says: {}", argument );
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
      bug( "{}: victim in nullptr room: {}", __func__, victim->name );
      return;
   }

   if( ch->isnpc(  ) && location->is_private(  ) )
   {
      progbug( "Mptransfer - Private room", ch );
      return;
   }

   if( !ch->can_see( victim, true ) )
      return;

   if( ch->isnpc(  ) && !victim->isnpc(  ) && victim->pcdata->release_date != std::chrono::system_clock::time_point{} )
   {
      progbugf( ch, "Mptransfer - helled character ({})", victim->name );
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
      act( AT_MAGIC, "A swirling vortex arrives to pick up $n!", victim, nullptr, nullptr, TO_ROOM );

   leave_map( victim, ch, location );

   act( AT_MAGIC, "A swirling vortex arrives, carrying $n!", victim, nullptr, nullptr, TO_ROOM );

   if( !ch->isnpc(  ) )
   {
      if( ch != victim )
         act( AT_IMMORT, "$n has sent a swirling vortex to transport you.", ch, nullptr, victim, TO_VICT );
      ch->print( "Ok.\r\n" );

      if( !victim->is_immortal(  ) && !victim->isnpc(  ) && !victim->in_hard_range( location->area ) )
         ch->print( "Warning: the player's level is not within the area's level range.\r\n" );
   }
}

CMDF( do_transfer )
{
   std::string arg1;
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
      for( auto* d : dlist )
      {
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
      ch->print_fmt( "Are you feeling alright today {}?\r\n", ch->name );
      return;
   }
   transfer_char( ch, victim, location );
}

void location_action( char_data * ch, const std::string & argument, room_index * location, continent_data *target_cont, short x, short y )
{
   continent_data *orig_cont = ch->continent;
   short origx = ch->map_x;
   short origy = ch->map_y;

   if( !location )
   {
      bug( "{}: nullptr room!", __func__ );
      return;
   }

   if( !ch->in_room )
   {
      bug( "{}: nullptr ch->in_room!", __func__ );
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

   /*
    * Bunch of checks to make sure the "ator" is temporarily on the same grid as
    * * the "atee" - Samson
    */
   if( location->flags.test( ROOM_MAP ) && !ch->has_pcflag( PCFLAG_ONMAP ) )
   {
      ch->set_pcflag( PCFLAG_ONMAP );
      ch->continent = target_cont;
      ch->map_x = x;
      ch->map_y = y;
   }
   else if( location->flags.test( ROOM_MAP ) && ch->has_pcflag( PCFLAG_ONMAP ) )
   {
      ch->continent = target_cont;
      ch->map_x = x;
      ch->map_y = y;
   }
   else if( !location->flags.test( ROOM_MAP ) && ch->has_pcflag( PCFLAG_ONMAP ) )
   {
      ch->unset_pcflag( PCFLAG_ONMAP );
      ch->continent = nullptr;
      ch->map_x = -1;
      ch->map_y = -1;
   }

   ch->set_color( AT_PLAIN );
   room_index *original = ch->in_room;
   ch->from_room(  );
   if( !ch->to_room( location ) )
      log_printf( "char_to_room: {}:{}, line {}.", __FILE__, __func__, __LINE__ );
   interpret( ch, argument );

   if( ch->has_pcflag( PCFLAG_ONMAP ) && !original->flags.test( ROOM_MAP ) )
      ch->unset_pcflag( PCFLAG_ONMAP );
   else if( !ch->has_pcflag( PCFLAG_ONMAP ) && original->flags.test( ROOM_MAP ) )
      ch->set_pcflag( PCFLAG_ONMAP );

   ch->continent = orig_cont;
   ch->map_x = origx;
   ch->map_y = origy;

   /*
    * See if 'ch' still exists before continuing!
    * Handles 'at XXXX quit' case.
    */
   for( auto* wch : charlist )
   {
      if( wch == ch && !ch->char_died(  ) )
      {
         ch->from_room(  );
         if( !ch->to_room( original ) )
            log_printf( "char_to_room: {}:{}, line {}.", __FILE__, __func__, __LINE__ );
         break;
      }
   }
}

/*  Added atmob and atobj to reduce lag associated with at
 *  --Shaddai
 */
void atmob( char_data * ch, char_data * wch, const std::string & argument )
{
   if( is_ignoring( wch, ch ) )
   {
      ch->print( "No such location.\r\n" );
      return;
   }
   location_action( ch, argument, wch->in_room, wch->continent, wch->map_x, wch->map_y );
}

void atobj( char_data * ch, obj_data * obj, const std::string & argument )
{
   location_action( ch, argument, obj->in_room, obj->continent, obj->map_x, obj->map_y );
}

/* Smaug 1.02a at command restored by Samson 8-14-98 */
CMDF( do_at )
{
   std::string arg;
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
      if( ( wch = ch->get_char_world( arg ) ) != nullptr && wch->in_room != nullptr )
      {
         atmob( ch, wch, argument );
         return;
      }

      if( ( obj = ch->get_obj_world( arg ) ) != nullptr && obj->in_room != nullptr )
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
   location_action( ch, argument, location, nullptr, -1, -1 );
}

CMDF( do_rat )
{
   std::string arg1, arg2;
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
         log_printf( "char_to_room: {}:{}, line {}.", __FILE__, __func__, __LINE__ );
      interpret( ch, argument );
   }

   ch->from_room(  );
   if( !ch->to_room( original ) )
      log_printf( "char_to_room: {}:{}, line {}.", __FILE__, __func__, __LINE__ );
   ch->print( "Done.\r\n" );
}

/* Rewritten by Whir to be viewer friendly - 8/26/98 */
CMDF( do_rstat )
{
   room_index *location;
   const char *dir_text[] = { "n", "e", "s", "w", "u", "d", "ne", "nw", "se", "sw", "?" };

   if( !str_cmp( argument, "exits" ) )
   {
      location = ch->in_room;

      ch->print_fmt( "&wExits for room '&G{}.&w' vnum &G{}&w\r\n", location->name, location->vnum );

      int cnt = 0;
      for( auto* pexit : location->exits )
      {
         ch->print_fmt( "{:2}) &G{:2}&w to &G{:<5}&w.  Key: &G{}  &wKeywords: '&G{}&w'  &wFlags: &G{}&w.\r\n",
                     ++cnt, dir_text[pexit->vdir], pexit->to_room ? pexit->to_room->vnum : 0, pexit->key, pexit->keyword, bitset_string( pexit->flags, ex_flags ) );

         ch->print_fmt( "Description: &G{}&wExit links back to vnum: &G{}  &wExit's RoomVnum: &G{}&w\r\n\r\n",
                     ( !pexit->exitdesc.empty() ) ? pexit->exitdesc : "(NONE)\r\n", pexit->rexit ? pexit->rexit->vnum : 0, pexit->rvnum );
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

   ch->print_fmt( "&wName: &G{}\r\n&wArea: &G{}  ", location->name, location->area ? location->area->name : "None????" );
   ch->print_fmt( "&wFilename: &G{}\r\n", location->area ? location->area->filename : "None????" );
   ch->print_fmt( "&wVnum: &G{}&w  Light: &G{}&w  TeleDelay: &G{}&w  TeleVnum: &G{}&w  Tunnel: &G{}&w  Max Weight: &G{}&w\r\n",
               location->vnum, location->light, location->tele_delay, location->tele_vnum, location->tunnel, location->max_weight );

   ch->print_fmt( "Room flags: &G%s&w\r\n", bitset_string( location->flags, r_flags ) );
   ch->print_fmt( "Sector type: &G%s&w\r\n", sect_types[location->sector_type] );

   ch->print_fmt( "Description:&w\r\n%s&w", location->roomdesc );

   /*
    * NiteDesc rstat added by Dracones 
    */
   if( !location->nitedesc.empty() )
      ch->print_fmt( "NiteDesc:&w\r\n{}&w", location->nitedesc );

   if( !location->extradesc.empty(  ) )
   {
      ch->print( "\r\nExtra description keywords:\r\n&G" );
      for( auto* ed : location->extradesc )
      {
         ch->print( ed->keyword );
         ch->print( " " );
      }
      ch->print( "&w\r\n" );
   }

   ch->print( "\r\nPermanent affects: " );
   if( ch->in_room->permaffects.empty(  ) )
      ch->print( "&GNone&w\r\n" );
   else
   {
      ch->print( "\r\n" );

      for( auto* af : location->permaffects )
         ch->showaffect( af );
   }

   if( ch->in_room->progtypes.none(  ) )
      ch->print( "Roomprogs: &GNone&w\r\n" );
   else
   {
      ch->print( "Roomprogs: &G" );

      for( auto* mprog : ch->in_room->mudprogs )
         ch->print_fmt( "{} ", mprog_type_to_name( mprog->type ) );

      ch->print( "&w\r\n" );
   }

   // Don't bother if nobody is in it
   if( !location->people.empty() )
   {
      ch->print( "\r\nCharacters/Mobiles in the room:\r\n" );
      for( auto* rch : location->people )
      {
         if( ch->can_see( rch, false ) )
            ch->print_fmt( "(&G{}&w)&G{}&w\r\n", ( rch->isnpc(  ) ? rch->pIndexData->vnum : 0 ), rch->name );
      }
   }

   // Don't bother if there are no objects
   if( !location->objects.empty() )
   {
      ch->print( "\r\nObjects in the room:\r\n" );
      for( auto* obj : location->objects )
      {
         if( ch->can_see_obj( obj, false ) )
            ch->print_fmt( "(&G{}&w)&G{}&w\r\n", obj->pIndexData->vnum, obj->name );
      }
      ch->print( "\r\n" );
   }

   // Don't bother if there are no exits
   if( !location->exits.empty(  ) )
   {
      int cnt = 0;

      ch->print( "\r\n------------------- EXITS -------------------\r\n" );

      for( auto* pexit : location->exits )
      {
         ch->print_fmt( "{:2}) &G{:<2} &wto &G{:<5}  &wKey: &G{:<5}  &wKeywords: &G{:<12}&w  Flags: &G{}&w\r\n",
                     ++cnt, dir_text[pexit->vdir], pexit->to_room ? pexit->to_room->vnum : 0,
                     pexit->key, !pexit->keyword.empty() ? pexit->keyword : "(none)", bitset_string( pexit->flags, ex_flags ) );

         if( IS_EXIT_FLAG( pexit, EX_OVERLAND ) )
            ch->print_fmt( "    &wExit coordinates: &G{}&wX &G{}&wY\r\n", pexit->map_x, pexit->map_y );
      }
   }
}

/* Rewritten by Whir to be viewer friendly - 8/26/98 */
CMDF( do_ostat )
{
   std::string suf;
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

   ch->print_fmt( "&w|Name   : &G{}&w\r\n", obj->name );

   ch->print_fmt( "|Short  : &G{}&w\r\n|Long   : &G{}&w\r\n", obj->short_descr, ( !obj->objdesc.empty() ) ? obj->objdesc : "" );

   if( !obj->action_desc.empty() )
      ch->print_fmt( "|Action : &G{}&w\r\n", obj->action_desc );

   ch->print_fmt( "|Area   : {}\r\n", obj->pIndexData->area ? obj->pIndexData->area->name : "(NONE)" );

   ch->print_fmt( "|Vnum   : &G{:5}&w |Type  :     &G{:9}&w |Count     : &G{:3}&w |Gcount: &G{:3}&w\r\n",
               obj->pIndexData->vnum, obj->item_type_name(  ).c_str(  ), obj->pIndexData->count, obj->count );

   ch->print_fmt( "|Number : &G{:2}&w/&G{:2}&w |Weight:     &G{:4}&w/&G{:4}&w |Wear_loc  : &G{:3}&w |Layers: &G{}&w\r\n",
               1, obj->get_number(  ), obj->weight, obj->get_weight(  ), obj->wear_loc, obj->pIndexData->layers );

   ch->print_fmt( "|Cost   : &G{:5}&w |Ego*  : &G{:6}&w |Ego   : &G{:6}&w |Timer: &G{}&w\r\n", obj->cost, obj->pIndexData->ego, obj->ego, obj->timer );

   ch->print_fmt( "|In room: &G{:5}&w |In obj: &G{}&w |Level :  &G{:5}&w |Limit: &G{:5}&w\r\n",
               obj->in_room == nullptr ? 0 : obj->in_room->vnum, obj->in_obj == nullptr ? "(NONE)" : obj->in_obj->short_descr, obj->level, obj->pIndexData->limit );

   ch->print_fmt( "|On map       : &G{}&w\r\n", obj->extra_flags.test( ITEM_ONMAP ) ? obj->continent->name : "(NONE)" );

   ch->print_fmt( "|Object Coords: &G{} {}&w\r\n", obj->map_x, obj->map_y );
   ch->print_fmt( "|Wear flags   : &G{}&w\r\n", bitset_string( obj->wear_flags, w_flags ) );
   ch->print_fmt( "|Extra flags  : &G{}&w\r\n", bitset_string( obj->extra_flags, o_flags ) );
   ch->print_fmt( "|Carried by   : &G{}&w\r\n", obj->carried_by == nullptr ? "(NONE)" : obj->carried_by->name );
   ch->print_fmt( "|Prizeowner   : &G{}&w\r\n", obj->owner.empty() ? "(NONE)" : obj->owner );
   ch->print_fmt( "|Seller       : &G{}&w\r\n", obj->seller.empty() ? "(NONE)" : obj->seller );
   ch->print_fmt( "|Buyer        : &G{}&w\r\n", obj->buyer.empty() ? "(NONE)" : obj->buyer );
   ch->print_fmt( "|Current bid  : &G{}&w\r\n", obj->bid );

   if( obj->year == 0 )
      ch->print( "|Scheduled donation date: &G(NONE)&w\r\n" );
   else
      ch->print_fmt( "|Scheduled donation date: &G {}{} day in the Month of {}, in the year {}.&w", day, suf, month_name[obj->month], obj->year );

   ch->print_fmt( "|Index Values : &G{:2} {:2} {:2} {:2} {:2} {:2} {:2} {:2} {:2} {:2} {:2}&w\r\n",
               obj->pIndexData->value[0], obj->pIndexData->value[1], obj->pIndexData->value[2], obj->pIndexData->value[3],
               obj->pIndexData->value[4], obj->pIndexData->value[5], obj->pIndexData->value[6], obj->pIndexData->value[7],
               obj->pIndexData->value[8], obj->pIndexData->value[9], obj->pIndexData->value[10] );

   ch->print_fmt( "|Object Values: &G{:2} {:2} {:2} {:2} {:2} {:2} {:2} {:2} {:2} {:2} {:2}&w\r\n",
               obj->value[0], obj->value[1], obj->value[2], obj->value[3],
               obj->value[4], obj->value[5], obj->value[6], obj->value[7], obj->value[8], obj->value[9], obj->value[10] );

   if( !obj->pIndexData->extradesc.empty(  ) )
   {
      ch->print( "|Primary description keywords:   " );
      for( auto* ed : obj->pIndexData->extradesc )
      {
         ch->print( ed->keyword );
         ch->print( " " );
      }
   }

   if( !obj->extradesc.empty(  ) )
   {
      ch->print( "|Secondary description keywords: " );
      for( auto* ed : obj->extradesc )
      {
         ch->print( ed->keyword );
         ch->print( " " );
      }
   }

   if( obj->pIndexData->progtypes.none(  ) )
      ch->print( "|Objprogs     : &GNone&w\r\n" );
   else
   {
      ch->print( "|Objprogs     : &G" );

      for( auto* mprog : obj->pIndexData->mudprogs )
         ch->print_fmt( "{} ", mprog_type_to_name( mprog->type ) );

      ch->print( "&w\r\n" );
   }

   /*
    * Rather useful and cool function provided by Druid to decipher those values 
    */
   ch->print( "\r\n" );
   ostat_plus( ch, obj, false );
   ch->print( "\r\n" );

   affect_data *af;
   std::list<affect_data *>::iterator paf;
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

   ch->pager_fmt( "\r\n&cName: &C{:<20} &cRoom : &w{:<10}", victim->name, victim->in_room == nullptr ? 0 : victim->in_room->vnum );
   ch->pager( "\r\n&cVariables:\r\n" );

   /*
    * Variables:
    * Vnum:           Tag:                 Type:     Timer:
    * Flags:
    * Data:
    */
   for( auto* vd : victim->variables )
   {
      ch->pager_fmt( "  &cVnum: &W{:<10} &cTag: &W{:<15} &cTimer: &W{}\r\n", vd->vnum, vd->tag, vd->timer );
      ch->pager( "  &cType: " );

      switch( vd->type )
      {
         default:
            ch->pager( "&RINVALID!!!" );
            break;

         case vtSTR:
            if( !vd->varstring.empty() )
               ch->pager_fmt( "&CString     &cData: &W{}", vd->varstring );
            break;

         case vtINT:
            if( vd->vardata > 0 )
               ch->pager_fmt( "&CInteger    &cData: &W{}", vd->vardata );
            break;

         case vtXBIT:
            if( vd->varflags.any() )
               ch->pager_fmt( "&CBitflags: &w[&W{}&w]", vd->varflags.to_string().c_str() );
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
   std::string lbuf;

   if( !( victim = ch->get_char_world( argument ) ) )
   {
      ch->print( "They aren't here.\r\n" );
      return;
   }

   if( !victim->isnpc(  ) )
   {
      ch->print_fmt( "&w|Name  : &G{:10} &w|Clan  : &G{:10} &w|PKill : &G{:10} &w|Room  : &G{}&w\r\n",
                     victim->name, ( victim->pcdata->clan == nullptr ) ? "(NONE)" : victim->pcdata->clan->name,
                     victim->has_pcflag( PCFLAG_DEADLY ) ? "Yes" : "No", victim->in_room->vnum );

      ch->print_fmt( "|Level : &G{:10} &w|Trust : &G{:10} &w|Sex   : &G{:10} &w|Gold  : &G{}&w\r\n", victim->level, victim->trust, npc_sex[victim->sex], victim->gold );

      ch->print_fmt( "|STR   : &G{:10} &w|HPs   : &G{:10} &w|MaxHPs: &G{:10} &w|Style : &G{}&w\r\n", victim->get_curr_str(  ), victim->hit, victim->max_hit, styles[victim->style] );

      ch->print_fmt( "|INT   : &G{:10} &w|Mana  : &G{:10} &w|MaxMan: &G{:10} &w|Pos   : &G{}&w\r\n",
                     victim->get_curr_int(  ), victim->mana, victim->max_mana, npc_position[victim->position] );

      ch->print_fmt( "|WIS   : &G{:10} &w|Move  : &G{:10} &w|MaxMov: &G{:10} &w|Wimpy : &G{}&w\r\n", victim->get_curr_wis(  ), victim->move, victim->max_move, victim->wimpy );

      ch->print_fmt( "|DEX   : &G{:10} &w|AC    : &G{:10} &w|Align : &G{:10} &w|Favor : &G{}&w\r\n",
                     victim->get_curr_dex(  ), victim->GET_AC(  ), victim->alignment, victim->pcdata->favor );

      ch->print_fmt( "|CON   : &G{:10} &w|+Hit  : &G{:10} &w|+Dam  : &G{:10} &w|Pracs : &G{}&w\r\n",
                     victim->get_curr_con(  ), victim->GET_HITROLL(  ), victim->GET_DAMROLL(  ), victim->pcdata->practice );

      ch->print_fmt( "|CHA   : &G{:10} &w|BseAge: &G{:10} &w|Agemod: &G{:10} &w|\r\n", victim->get_curr_cha(  ), victim->pcdata->age, victim->pcdata->age_bonus );

      if( victim->pcdata->condition[COND_THIRST] == -1 )
         lbuf = "|Thirst: &G    Immune &w";
      else
         lbuf = std::format( "|Thirst: &G{:10} &w", victim->pcdata->condition[COND_THIRST] );
      if( victim->pcdata->condition[COND_FULL] == -1 )
         lbuf.append( "|Hunger: &G    Immune &w" );
      else
         lbuf.append( std::format( "|Hunger: &G{:10} &w", victim->pcdata->condition[COND_FULL] ) );
      ch->print_fmt( "|LCK   : &G{:10} &w{}|Drunk : &G{} &w\r\n", victim->get_curr_lck(  ), lbuf, victim->pcdata->condition[COND_DRUNK] );

      ch->print_fmt( "|Class :&G{:11} &w|Mental: &G{:10} &w|#Attks: &G{:10}&w |Barehand: &G{}&wd&G{}&w+&G{}&w\r\n",
                     capitalize( victim->get_class(  ) ), victim->mental_state, victim->numattacks, victim->barenumdie, victim->baresizedie, victim->GET_DAMROLL(  ) );

      ch->print_fmt( "|Race  : &G{:10} &w|Title : &G{}&w\r\n", capitalize( victim->get_race(  ) ), victim->pcdata->title );

      ch->print_fmt( "|Deity :&G{:11} &w|Authed:&G{:11} &w|SF    :&G{:11} &w|PVer  : &G{}&w\r\n",
                     ( victim->pcdata->deity == nullptr ) ? "(NONE)" : victim->pcdata->deity->name,
                     !victim->pcdata->authed_by.empty(  )? victim->pcdata->authed_by : "Unknown", victim->spellfail, victim->pcdata->version );

      ch->print_fmt( "|Map   : &G{:10} &w|Coords: &G{} {}&w\r\n", victim->has_pcflag( PCFLAG_ONMAP ) ? victim->continent->name : "(NONE)", victim->map_x, victim->map_y );

      ch->print_fmt( "|Master: &G{:10} &w|Leader: &G{}&w\r\n", victim->master ? victim->master->name : "(NONE)", victim->leader ? victim->leader->name : "(NONE)" );

      ch->print_fmt( "|Saves : ---------- | ----------------- | ----------------- | -----------------\r\n" );

      ch->print_fmt( "|Poison: &G{:10} &w|Para  : &G{:10} &w|Wands : &G{:10} &w|Spell : &G{}&w\r\n",
                     victim->saving_poison_death, victim->saving_para_petri, victim->saving_wand, victim->saving_spell_staff );

      ch->print_fmt( "|Death : &G{:10} &w|Petri : &G{:10} &w|Breath: &G{:10} &w|Staves: &G{}&w\r\n",
                     victim->saving_poison_death, victim->saving_para_petri, victim->saving_breath, victim->saving_spell_staff );

      ch->print_fmt( "|Hostname     : &G{}&w\r\n", !victim->pcdata->lasthost.empty() ? victim->pcdata->lasthost : "Unknown" );
      ch->print_fmt( "|IP Address   : &G{}&w\r\n", victim->desc ? victim->desc->ipaddress : "Unknown" );
      ch->print_fmt( "|Previous Host: &G{}&w\r\n", !victim->pcdata->prevhost.empty() ? victim->pcdata->prevhost : "Unknown" );

      if( victim->desc )
      {
         ch->print_fmt( "|Player's Terminal Program: &G{}&w\r\n", victim->desc->client );
         ch->print_fmt( "|Player's Terminal Support: &GMCCP[{}]  &GMSP[{}]&w\r\n", victim->desc->can_compress ? "&wX&G" : " ", victim->desc->msp_detected ? "&wX&G" : " " );
         ch->print_fmt( "|Terminal Support In Use  : &GMCCP[{}]  &GMSP[{}]&w\r\n", victim->desc->can_compress ? "&wX&G" : " ", victim->MSP_ON(  )? "&wX&G" : " " );
      }

      ch->print_fmt( "|PC Flags    : &G{}&w\r\n", !victim->has_pcflags(  )? "(NONE)" : bitset_string( victim->get_pcflags(  ), pc_flags ) );

      ch->print( "|Languages: &[score]" );
      for( iLang = 0; iLang < LANG_UNKNOWN; ++iLang )
      {
         if( knows_language( victim, iLang, victim ) )
         {
            if( iLang == victim->speaking )
               ch->set_color( AT_SCORE3 );
            ch->print_fmt( "{} ", lang_names[iLang] );
            ch->set_color( AT_SCORE );
         }
      }
      ch->print( "\r\n" );

      ch->print_fmt( "&w|Affected By : &G{}&w\r\n", !victim->has_aflags(  )? "(NONE)" : bitset_string( victim->get_aflags(  ), aff_flags ) );
      ch->print_fmt( "|Bestowments : &G{}&w\r\n", victim->pcdata->bestowments.empty(  )? "(NONE)" : victim->pcdata->bestowments );
      ch->print_fmt( "|Resistances : &G{}&w\r\n", !victim->has_resists(  )? "(NONE)" : bitset_string( victim->get_resists(  ), ris_flags ) );
      ch->print_fmt( "|Immunities  : &G{}&w\r\n", !victim->has_immunes(  )? "(NONE)" : bitset_string( victim->get_immunes(  ), ris_flags ) );
      ch->print_fmt( "|Suscepts    : &G{}&w\r\n", !victim->has_susceps(  )? "(NONE)" : bitset_string( victim->get_susceps(  ), ris_flags ) );
      ch->print_fmt( "|Absorbs     : &G{}&w\r\n", !victim->has_absorbs(  )? "(NONE)" : bitset_string( victim->get_absorbs(  ), ris_flags ) );

      for( auto* aff : victim->affects )
      {
         if( ( skill = get_skilltype( aff->type ) ) != nullptr )
         {
            std::string mod;

            if( aff->location == APPLY_AFFECT || aff->location == APPLY_EXT_AFFECT )
               mod = aff_flags[aff->modifier];
            else if( aff->location == APPLY_RESISTANT || aff->location == APPLY_IMMUNE || aff->location == APPLY_ABSORB || aff->location == APPLY_SUSCEPTIBLE )
               mod = ris_flags[aff->modifier];
            else
               mod = std::format( "{}", aff->modifier );

            ch->print_fmt( "|{}: '&G{}&w' modifies &G{}&w by &G{}&w for &G{}&w rounds", skill_tname[skill->type], skill->name, a_types[aff->location], mod, aff->duration );
            if( aff->bit > 0 )
               ch->print_fmt( " with bits &G{}&w.\r\n", aff_flags[aff->bit] );
            else
               ch->print( ".\r\n" );
         }
      }

      if( !victim->variables.empty(  ) )
      {
         ch->pager( "&cVariables  : &w" );
         for( auto* vd : victim->variables )
         {
            ch->pager_fmt( "{}:{}", vd->tag, vd->vnum );

            switch( vd->type )
            {
               default:
                  ch->pager( "=INVALID!!!" );
                  break;

               case vtSTR:
                  if( !vd->varstring.empty() )
                     ch->pager_fmt( "={}", vd->varstring );
               break;

               case vtINT:
                  if( vd->vardata > 0 )
                     ch->pager_fmt( "={}", vd->vardata );
               break;

               case vtXBIT:
                  if( vd->varflags.any() )
                     ch->pager_fmt( "={}", vd->varflags.to_string() );
               break;
            }
            ch->pager( "  " );
         }
         ch->pager( "\r\n" );
      }
   }
   else
   {
      ch->print_fmt( "&w|Name  : &G{:>50} &w|Room  : &G{}&w\r\n", victim->name, victim->in_room->vnum );

      ch->print_fmt( "|Area  : &G{:<50} &w\r\n", victim->pIndexData->area ? victim->pIndexData->area->name : "(NONE)" );

      ch->print_fmt( "|Level : &G{:10} &w|Vnum  : &G{:10} &w|Sex   : &G{:10} &w|Gold  : &G{}&w\r\n", victim->level, victim->pIndexData->vnum, npc_sex[victim->sex], victim->gold );

      ch->print_fmt( "|STR   : &G{:10} &w|HPs   : &G{:10} &w|MaxHPs: &G{:10} &w|Exp   : &G{}&w\r\n", victim->get_curr_str(  ), victim->hit, victim->max_hit, victim->exp );

      ch->print_fmt( "|INT   : &G{:10} &w|Mana  : &G{:10} &w|MaxMan: &G{:10} &w|Pos   : &G{}&w\r\n",
                     victim->get_curr_int(  ), victim->mana, victim->max_mana, npc_position[victim->position] );

      ch->print_fmt( "|WIS   : &G{:10} &w|Move  : &G{:10} &w|MaxMov: &G{:10} &w|H.D.  : &G{}&wd&G{}&w+&G{}&w\r\n",
                     victim->get_curr_wis(  ), victim->move, victim->max_move, victim->pIndexData->hitnodice, victim->pIndexData->hitsizedice, victim->pIndexData->hitplus );

      ch->print_fmt( "|DEX   : &G{:10} &w|AC    : &G{:10} &w|Align : &G{:10} &w|D.D.  : &G{}&wd&G{}&w+&G{}&w\r\n",
                     victim->get_curr_dex(  ), victim->GET_AC(  ), victim->alignment, victim->pIndexData->damnodice,
                     victim->pIndexData->damsizedice, victim->pIndexData->damplus );

      ch->print_fmt( "|CON   : &G{:10} &w|Thac0 : &G{:10} &w|+Hit  : &G{:10} &w|+Dam  : &G{}&w\r\n",
                     victim->get_curr_con(  ), calc_thac0( victim, nullptr, 0 ), victim->GET_HITROLL(  ), victim->GET_DAMROLL(  ) );

      ch->print_fmt( "|CHA   : &G{:10} &w|Count : &G{:10} &w|Timer : &G{:10}&w\r\n", victim->get_curr_cha(  ), victim->pIndexData->count, victim->timer );

      ch->print_fmt( "|LCK   : &G{:10} &w|#Attks: &G{:10} &w|Thac0*: &G{:10} &w|Exp*  : &G{}&w\r\n",
                     victim->get_curr_lck(  ), victim->numattacks, victim->mobthac0, victim->pIndexData->exp );

      ch->print_fmt( "|Class : &G{:10} &w|Master: &G{}&w\r\n", capitalize( npc_class[victim->Class] ), victim->master ? victim->master->name : "(NONE)" );

      ch->print_fmt( "|Race  : &G{:10} &w|Leader: &G{}&w\r\n", capitalize( npc_race[victim->race] ), victim->leader ? victim->leader->name : "(NONE)" );

      ch->print_fmt( "|Map   : &G{:10} &w|Coords: &G{} {}    &w|Native Sector: &G{}&w\r\n",
                     victim->has_actflag( ACT_ONMAP ) ? victim->continent->name : "(NONE)",
                     victim->map_x, victim->map_y, victim->sector < 0 ? "Not set yet" : sect_types[victim->sector] );

      ch->print_fmt( "|Saves : ---------- | ----------------- | ----------------- | -----------------\r\n" );

      ch->print_fmt( "|Poison: &G{:10} &w|Para  : &G{:10} &w|Wands : &G{:10} &w|Spell : &G{}&w\r\n",
                     victim->saving_poison_death, victim->saving_para_petri, victim->saving_wand, victim->saving_spell_staff );

      ch->print_fmt( "|Death : &G{:10} &w|Petri : &G{:10} &w|Breath: &G{:10} &w|Staves: &G{}&w\r\n",
                     victim->saving_poison_death, victim->saving_para_petri, victim->saving_breath, victim->saving_spell_staff );

      ch->print_fmt( "|Short : &G{}&w\r\n", ( !victim->short_descr.empty() ) ? victim->short_descr : "(NONE)" );

      ch->print_fmt( "|Long  : &G{}&w\r\n", ( !victim->long_descr.empty() ) ? victim->long_descr : "(NONE)" );

      ch->print_fmt( "|Languages: &[score]" );
      for( iLang = 0; iLang < LANG_UNKNOWN; ++iLang )
      {
         if( knows_language( victim, iLang, victim ) || ( ch->isnpc(  ) && !victim->has_langs(  ) ) )
         {
            if( iLang == victim->speaking )
               ch->set_color( AT_SCORE3 );
            ch->print_fmt( "{} ", lang_names[iLang] );
            ch->set_color( AT_SCORE );
         }
      }
      ch->print( "\r\n" );

      ch->print_fmt( "&w|Act Flags   : &G{}&w\r\n", bitset_string( victim->get_actflags(  ), act_flags ) );
      ch->print_fmt( "|Affected By : &G{}&w\r\n", !victim->has_aflags(  )? "(NONE)" : bitset_string( victim->get_aflags(  ), aff_flags ) );
      ch->print_fmt( "|Mob Spec Fun: &G{}&w\r\n", !victim->spec_funname.empty(  )? victim->spec_funname : "(NONE)" );
      ch->print_fmt( "|Body Parts  : &G{}&w\r\n", bitset_string( victim->get_bparts(  ), part_flags ) );
      ch->print_fmt( "|Resistances : &G{}&w\r\n", !victim->has_resists(  )? "(NONE)" : bitset_string( victim->get_resists(  ), ris_flags ) );
      ch->print_fmt( "|Immunities  : &G{}&w\r\n", !victim->has_immunes(  )? "(NONE)" : bitset_string( victim->get_immunes(  ), ris_flags ) );
      ch->print_fmt( "|Suscepts    : &G{}&w\r\n", !victim->has_susceps(  )? "(NONE)" : bitset_string( victim->get_susceps(  ), ris_flags ) );
      ch->print_fmt( "|Absorbs     : &G{}&w\r\n", !victim->has_absorbs(  )? "(NONE)" : bitset_string( victim->get_absorbs(  ), ris_flags ) );
      ch->print_fmt( "|Attacks     : &G{}&w\r\n", bitset_string( victim->get_attacks(  ), attack_flags ) );
      ch->print_fmt( "|Defenses    : &G{}&w\r\n", bitset_string( victim->get_defenses(  ), defense_flags ) );

      if( victim->pIndexData->progtypes.none(  ) )
         ch->print( "|Mobprogs    : &GNone&w\r\n" );
      else
      {
         ch->print( "|Mobprogs    : &G" );

         for( auto* mprog : victim->pIndexData->mudprogs )
            ch->print_fmt( "{} ", mprog_type_to_name( mprog->type ) );

         ch->print( "&w\r\n" );
      }

      for( auto* af : victim->affects )
      {
         if( ( skill = get_skilltype( af->type ) ) != nullptr )
         {
            std::string mod;

            if( af->location == APPLY_AFFECT
               || af->location == APPLY_RESISTANT || af->location == APPLY_IMMUNE || af->location == APPLY_ABSORB || af->location == APPLY_SUSCEPTIBLE )
               mod = aff_flags[af->modifier];
            else
               mod = std::format( "{}", af->modifier );
            ch->print_fmt( "|{}: '&G{}&w' modifies &G{}&w by &G{}&w for &G{}&w rounds", skill_tname[skill->type], skill->name, a_types[af->location], mod, af->duration );
            if( af->bit > 0 )
               ch->print_fmt( " with bits &G{}&w.\r\n", aff_flags[af->bit] );
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
   std::string arg;

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
         ch->print_fmt( "{} is not a mob!\r\n", victim->name );
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
         ch->print_fmt( "{} is not a player!\r\n", argument );
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
void find_oftype( char_data * ch, std::string_view argument )
{
   std::map<int, obj_index *>::iterator iobj = obj_index_table.begin(  );
   int nMatch, type;

   ch->set_pager_color( AT_PLAIN );

   nMatch = 0;

   /*
    * My God, now isn't this MUCH nicer than what was here before? - Samson 9-18-03 
    */
   type = get_otype( argument );
   if( type < 0 )
   {
      ch->print_fmt( "{} is an invalid item type.\r\n", argument );
      return;
   }

   while( iobj != obj_index_table.end(  ) )
   {
      obj_index *pObjIndex = iobj->second;

      if( type == pObjIndex->item_type )
      {
         ++nMatch;
         ch->pager_fmt( "[{:5}] %s\r\n", pObjIndex->vnum, capitalize( pObjIndex->short_descr ) );
      }
      ++iobj;
   }

   if( nMatch )
      ch->pager_fmt( "Number of matches: {}\n", nMatch );
   else
      ch->print( "Sorry, no matching item types found.\r\n" );
}

/* Consolidated find command 3-21-98 (SLAY DWIP) */
CMDF( do_find )
{
   std::string arg, arg2;

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
   for( auto* victim : charlist )
   {
      if( victim->isnpc(  ) && victim->in_room && hasname( victim->name, argument ) )
      {
         found = true;
         if( victim->has_actflag( ACT_ONMAP ) )
            ch->pager_fmt( "&Y[&W{:5}&Y] &G{:<28} &YOverland: &C{} {} {}\r\n", victim->pIndexData->vnum, victim->short_descr, victim->continent->name, victim->map_x, victim->map_y );
         else
            ch->pager_fmt( "&Y[&W{:5}&Y] &G{:<28} &Y[&W{:5}&Y] &C{}\r\n", victim->pIndexData->vnum, victim->short_descr, victim->in_room->vnum, victim->in_room->name );
      }
   }
   if( !found )
      ch->print_fmt( "You didn't find any {}.\r\n", argument );
}

/* Added 'show' argument for lowbie imms without ostat -- Blodkai */
/* Made show the default action :) Shaddai */
/* Trimmed size, added vict info, put lipstick on the pig -- Blod */
CMDF( do_bodybag )
{
   std::string arg1, arg2, buf2;
   bool found = false, bag = false;

   argument = one_argument( argument, arg1 );
   if( arg1.empty(  ) )
   {
      ch->print( "&PSyntax:  bodybag <character> | bodybag <character> yes/bag/now\r\n" );
      return;
   }

   buf2 = std::format( "the corpse of {}", arg1 );
   argument = one_argument( argument, arg2 );

   if( !arg2.empty(  ) && ( str_cmp( arg2, "yes" ) && str_cmp( arg2, "bag" ) && str_cmp( arg2, "now" ) ) )
   {
      ch->print( "\r\n&PSyntax:  bodybag <character> | bodybag <character> yes/bag/now\r\n" );
      return;
   }
   if( !str_cmp( arg2, "yes" ) || !str_cmp( arg2, "bag" ) || !str_cmp( arg2, "now" ) )
      bag = true;

   ch->pager_fmt( "\r\n&P{} remains of {} ... ", bag ? "Retrieving" : "Searching for", capitalize( arg1 ) );

   for( auto* obj : objlist )
   {
      if( obj->in_room && !str_cmp( buf2, obj->short_descr ) && ( obj->pIndexData->vnum == OBJ_VNUM_CORPSE_PC ) )
      {
         ch->pager( "\r\n" );
         found = true;
         ch->pager_fmt( "&P{}:  {}{:<12.12}   &PIn:  &w{:<22.22}  &P[&w{:5}&P]   &PTimer:  {}{:2}",
                     bag ? "Bagging" : "Corpse",
                     bag ? "&R" : "&w", capitalize( arg1 ), obj->in_room->area->name, obj->in_room->vnum,
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
   std::list<char_data *>::iterator ich;
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
      ch->pager_fmt( "&P{} is not currently online.\r\n", capitalize( arg1 ) );
      return;
   }
   if( owner->pcdata->deity )
      ch->pager_fmt( "&P{} ({}) has {} favor with {} (needed to supplicate: %d)\r\n",
                  owner->name, owner->level, owner->pcdata->favor, owner->pcdata->deity->name, owner->pcdata->deity->scorpse );
   else
      ch->pager_fmt( "&P{} ({}) has no deity.\r\n", owner->name, owner->level );
}

/* New owhere by Altrag, 03/14/96 */
CMDF( do_owhere )
{
   std::string arg, arg1, buf;
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
         ch->pager_fmt( "&Y[&W{:5}&Y] &G{:<28} &Cin object &Y[&W{:5}&Y] &C{}\r\n",
                     obj->pIndexData->vnum, obj->oshort( ), obj->in_obj->pIndexData->vnum, obj->in_obj->short_descr );
         ++icnt;
      }
      buf = std::format( "&Y[&W{:5}&Y] &G{:<28} ", obj->pIndexData->vnum, obj->oshort(  ) );
      if( obj->carried_by )
         buf.append( std::format( "&Cinvent   &Y[&W{:5}d&Y] &C{}\r\n",
                   ( obj->carried_by->isnpc(  )? obj->carried_by->pIndexData->vnum : 0 ), PERS( obj->carried_by, ch, true ) ) );
      else if( obj->in_room )
      {
         if( obj->extra_flags.test( ITEM_ONMAP ) )
            buf.append( std::format( "&Coverland &Y[&W{}&Y] &C{} {}\r\n", obj->continent->name.c_str( ), obj->map_x, obj->map_y ) );
         else
            buf.append( std::format( "&Croom     &Y[&W{:5}&Y] &C{}\r\n", obj->in_room->vnum, obj->in_room->name ) );
      }
      else if( obj->in_obj )
      {
         bug( "{}: obj->in_obj after nullptr!", __func__ );
         buf.append( "object??\r\n" );
      }
      else
      {
         bug( "{}: object doesnt have location!", __func__ );
         buf.append( "nowhere??\r\n" );
      }
      ch->pager( buf );
      ++icnt;
      ch->pager_fmt( "Nested {} levels deep.\r\n", icnt );
      return;
   }

   found = false;
   std::list<obj_data *>::iterator iobj;
   for( iobj = objlist.begin(  ); iobj != objlist.end(  ); ++iobj )
   {
      obj = *iobj;

      if( !hasname( obj->name, arg ) )
         continue;
      found = true;

      buf = std::format( "&Y(&W{:3}&Y) [&W{:5}&Y] &G{:<28} ", ++icnt, obj->pIndexData->vnum, obj->oshort(  ) );
      if( obj->carried_by )
         buf.append( std::format( "&Cinvent   &Y[&W{:5}&Y] &C{}\r\n",
                   ( obj->carried_by->isnpc(  )? obj->carried_by->pIndexData->vnum : 0 ), PERS( obj->carried_by, ch, true ) ) );
      else if( obj->in_room )
      {
         if( obj->extra_flags.test( ITEM_ONMAP ) )
            buf.append( std::format( "&Coverland &Y[&W{}&Y] &C{} {}\r\n", obj->continent->name.c_str( ), obj->map_x, obj->map_y ) );
         else
            buf.append( std::format( "&Croom     &Y[&W{:5}&Y] &C{}\r\n", obj->in_room->vnum, obj->in_room->name ) );
      }
      else if( obj->in_obj )
         buf.append( std::format( "&Cobject &Y[&W{:5}&Y] &C{}\r\n", obj->in_obj->pIndexData->vnum, obj->in_obj->oshort(  ) ) );
      else
      {
         bug( "{}: object doesn't have location!", __func__ );
         buf.append( "nowhere??\r\n" );
      }
      ch->pager( buf );
   }
   if( !found )
      ch->print_fmt( "You didn't find any {}.\r\n", arg );
   else
      ch->pager_fmt( "{} matches.\r\n", icnt );
}

CMDF( do_pwhere )
{
   bool found;

   if( argument.empty(  ) )
   {
      ch->pager( "&[people]Players you can see online:\r\n" );
      found = false;
      for( auto* d : dlist )
      {
         char_data *victim;

         if( ( d->connected == CON_PLAYING || d->connected == CON_EDITING )
             && ( victim = d->character ) != nullptr && !victim->isnpc(  ) && victim->in_room && ch->can_see( victim, true ) && !is_ignoring( victim, ch ) )
         {
            found = true;
            if( victim->has_pcflag( PCFLAG_ONMAP ) )
               ch->pager_fmt( "&G{:<28} &Y[&WOverland&Y] &C{} {} {}\r\n", victim->name, victim->continent->name.c_str( ), victim->map_x, victim->map_y );
            else
               ch->pager_fmt( "&G{:<28} &Y[&W{:5}&Y]&C {}\r\n", victim->name, victim->in_room->vnum, victim->in_room->name );
         }
      }
      if( !found )
         ch->pager( "None.\r\n" );
   }
   else
   {
      ch->pager( "You search high and low and find:\r\n" );
      found = false;
      for( auto* victim : pclist )
      {
         if( victim->in_room && ch->can_see( victim, true ) && hasname( victim->name, argument ) )
         {
            found = true;
            ch->pager_fmt( "&Y{:<28} &G[&W{:5}&G]&C {}\r\n", PERS( victim, ch, true ), victim->in_room->vnum, victim->in_room->name );
            break;
         }
      }
      if( !found )
         ch->pager( "Nobody by that name.\r\n" );
   }
}

void where_mobile( char_data * ch, const std::string & argument )
{
   ch->set_color( AT_PLAIN );

   if( !is_number( argument ) )
   {
      do_mwhere( ch, argument );
      return;
   }

   int vnum = std::stoi( argument );

   if( !get_mob_index( vnum ) )
   {
      ch->print( "No mobile has that vnum.\r\n" );
      return;
   }

   int mobcnt = 0;
   bool found = false;
   for( auto* victim : charlist )
   {
      if( victim->isnpc(  ) && victim->in_room && victim->pIndexData->vnum == vnum )
      {
         found = true;
         ++mobcnt;
         ch->pager_fmt( "[{:5}] {:<28} [{:5}] {}\r\n", victim->pIndexData->vnum, victim->short_descr, victim->in_room->vnum, victim->in_room->name );
      }
   }

   if( !found )
      ch->pager_fmt( "No copies of vnum {} are loaded.\r\n", vnum );
   else
      ch->pager_fmt( "{} matches for vnum {} are loaded.\r\n", mobcnt, vnum );
}

/* Consolidated Where command - Samson 3-21-98 */
CMDF( do_where )
{
   std::string arg;

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
      std::string buf;
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

      for( auto* obj : objlist )
      {
         if( obj->pIndexData->vnum != vnum )
            continue;
         found = true;

         buf = std::format( "({:3}) [{:5}] {:<28} in ", ++icnt, obj->pIndexData->vnum, obj->oshort(  ) );
         if( obj->carried_by )
            buf.append( std::format( "invent [{:5}] {}\r\n",
                      ( obj->carried_by->isnpc(  )? obj->carried_by->pIndexData->vnum : 0 ), PERS( obj->carried_by, ch, false ) ) );
         else if( obj->in_room )
            buf.append( std::format( "room   [{:5}] {}\r\n", obj->in_room->vnum, obj->in_room->name ) );
         else if( obj->in_obj )
            buf.append( std::format( "object [{:5}] {}\r\n", obj->in_obj->pIndexData->vnum, obj->in_obj->oshort(  ) ) );
         else
         {
            bug( "{}: object '{}' doesn't have location!", __func__, obj->short_descr );
            buf.append( "nowhere??\r\n" );
         }
         ch->pager( buf );
      }

      if( !found )
         ch->pager_fmt( "No copies of vnum {} are loaded.\r\n", vnum );
      else
         ch->pager_fmt( "{} matches for vnum {} are loaded.\r\n", icnt, vnum );

      ch->pager_fmt( "Checking player files for stored copies of vnum {}....\r\n", vnum );

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
         ch->print( "Reboot will be canceled at the next event poll.\r\n" );
         reboot_counter = -5;
         bootlock = false;
         sysdata->DENY_NEW_PLAYERS = false;
         return;
      }
      sysdata->DENY_NEW_PLAYERS = true;
      reboot_counter = sysdata->rebootcount;

      echo_to_all( "&RReboot countdown started.", ECHOTAR_ALL );
      echo_all_printf( ECHOTAR_ALL, "&YGame reboot in {} minutes.", reboot_counter );
      bootlock = true;
      add_event( 60, ev_reboot_count, nullptr );
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
   std::list<descriptor_data *>::iterator ds;
   descriptor_data *d;

   ch->set_color( AT_IMMORT );

   if( argument.empty(  ) )
   {
      ch->print( "&GSnooping the following people:\r\n\r\n" );
      for( ds = dlist.begin(  ); ds != dlist.end(  ); ++ds )
      {
         d = *ds;

         if( d->snoop_by == ch->desc )
            ch->print_fmt( "{} ", d->original ? d->original->name : d->character->name );
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
            d->snoop_by = nullptr;
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
   ch->desc = nullptr;
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
   ch->desc->original = nullptr;
   ch->desc->character->desc = ch->desc;
   ch->desc->character->switched = nullptr;
   ch->desc = nullptr;
}

void objinvoke( char_data * ch, std::string & argument )
{
   std::string arg, arg2;
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
         ch->print_fmt( "You must oinvoke between 1 and {} items.\r\n", MAX_OINVOKE_QUANTITY );
         return;
      }
   }

   if( !is_number( arg ) )
   {
      std::string arg3;
      std::map<int, obj_index *>::iterator iobj = obj_index_table.begin(  );
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
      log_printf( "create_object: {}:{}, line {}.", __FILE__, __func__, __LINE__ );
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
         ch->print_fmt( "WARNING: This item has ego exceeding {}! Destroy this item when finished!\r\n", sysdata->minego );
         log_printf( "{}: {} has loaded a copy of vnum {}.", __func__, ch->name, pObjIndex->vnum );
      }
   }
#else
   if( obj->ego >= sysdata->minego )
   {
      ch->print_fmt( "WARNING: This item has ego exceeding {}! Destroy this item when finished!\r\n", sysdata->minego );
      log_printf( "{}: {} has loaded a copy of vnum {}.", __func__, ch->name, pObjIndex->vnum );
   }
#endif

   if( obj->wear_flags.test( ITEM_TAKE ) )
   {
      obj = obj->to_char( ch );
      act( AT_IMMORT, "$n waves $s hand, and $p appears in $s inventory!", ch, obj, nullptr, TO_ROOM );
      act( AT_IMMORT, "You wave your hand, and $p appears in your inventory!", ch, obj, nullptr, TO_CHAR );
   }
   else
   {
      obj = obj->to_room( ch->in_room, ch );
      act( AT_IMMORT, "$n waves $s hand, and $p appears in the room!", ch, obj, nullptr, TO_ROOM );
      act( AT_IMMORT, "You wave your hand, and $p appears in the room!", ch, obj, nullptr, TO_CHAR );
   }

   /*
    * This is bad, means stats were exceeding ego specs 
    */
   if( obj->ego == -2 )
      ch->print( "&YWARNING: This object exceeds allowable ego specs.\r\n" );

   /*
    * I invoked what? --Blodkai 
    */
   ch->print_fmt( "&Y(&W#{} &Y- &W{} &Y- &Wlvl {}&Y)\r\n", pObjIndex->vnum, pObjIndex->name, obj->level );
}

void mobinvoke( char_data * ch, std::string & argument )
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
      std::string arg2;
      int cnt = 0;
      int count = number_argument( argument, arg2 );

      vnum = -1;
      std::map<int, mob_index *>::iterator imob = mob_index_table.begin(  );
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
      log_printf( "char_to_room: {}:{}, line {}.", __FILE__, __func__, __LINE__ );

   /*
    * If you load one on the map, make sure it gets placed properly - Samson 8-21-99 
    */
   fix_maps( ch, victim );
   victim->sector = victim->continent->get_terrain( ch->map_x, ch->map_y );

   act( AT_IMMORT, "$n peers into the ether, and plucks out $N!", ch, nullptr, victim, TO_ROOM );
   /*
    * How about seeing what we're invoking for a change. -Blodkai
    */
   ch->print_fmt( "&YYou peer into the ether.... and pluck out {}!\r\n", pMobIndex->short_descr );
   ch->print_fmt( "(&W#{} &Y- &W{} &Y- &Wlvl {}&Y)\r\n", pMobIndex->vnum, pMobIndex->player_name, victim->level );
}

/* Shard-like load command spliced off of ROT codebase - Samson 3-21-98 */
CMDF( do_load )
{
   std::string arg;

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
      for( auto it = ch->in_room->people.begin(  ); it != ch->in_room->people.end(  ); )
      {
         char_data *victim = *it;
         ++it;

         bool found = false;

         /*
          * GACK! Why did this get removed?? 
          */
         if( !victim->isnpc(  ) )
            continue;

         char_data *tch = nullptr;
         std::list<char_data *>::iterator ich2;
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

      for( auto it = ch->in_room->objects.begin(  ); it != ch->in_room->objects.end(  ); )
      {
         obj_data *obj = *it;
         ++it;

         bool found = false;

         char_data *tch = nullptr;
         std::list<char_data *>::iterator ich2;
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

      act( AT_IMMORT, "$n makes a complex series of gestures.... suddenly things seem a lot cleaner!", ch, nullptr, nullptr, TO_ROOM );
      act( AT_IMMORT, "You make a complex series of gestures.... suddenly things seem a lot cleaner!", ch, nullptr, nullptr, TO_CHAR );
      return;
   }

   char_data *victim = nullptr;
   obj_data *obj = nullptr;
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
      for( auto* tch : ch->in_room->people )
      {
         if( !tch->isnpc(  ) && tch->pcdata->dest_buf == obj )
         {
            ch->print( "You cannot purge something being edited.\r\n" );
            return;
         }
      }
      obj->separate(  );
      act( AT_IMMORT, "$n snaps $s finger, and $p vanishes from sight!", ch, obj, nullptr, TO_ROOM );
      act( AT_IMMORT, "You snap your fingers, and $p vanishes from sight!", ch, obj, nullptr, TO_CHAR );
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

   for( auto* tch : ch->in_room->people )
   {
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
   act( AT_IMMORT, "$n snaps $s fingers, and $N vanishes from sight!", ch, nullptr, victim, TO_NOTVICT );
   act( AT_IMMORT, "You snap your fingers, and $N vanishes from sight!", ch, nullptr, victim, TO_CHAR );
   victim->extract( true );
}

void destroy_immdata( char_data * ch, std::string_view vicname )
{
   std::error_code ec;
   std::string areafile;

   std::filesystem::path godfile = std::format( "{}{}", GOD_DIR, capitalize( vicname ) );

   if( std::filesystem::remove( godfile, ec ) )
      ch->print( "&RPlayer's immortal data destroyed.\r\n" );
   else if( ec && ec != std::errc::no_such_file_or_directory )
   {
      ch->print_fmt( "&RUnknown error - {} (immortal data). Report to the admins.\r\n", ec.message() );
      log_printf( "Error destroying {}: {}", godfile.string(), ec.message() );
   }

   areafile = std::format( "{}.are", vicname );

   for( auto it = arealist.begin(  ); it != arealist.end(  ); )
   {
      area_data *area = *it;
      ++it;

      if( !str_cmp( area->filename, areafile ) )
      {
         std::filesystem::path buildfile = std::format( "{}{}", BUILD_DIR, areafile );
         std::filesystem::path buildbackup = std::format( "{}{}.bak", BUILD_DIR, areafile );

         area->fold( buildfile.string().c_str(), false );
         deleteptr( area );

         ec.clear();
         std::filesystem::rename( buildfile, buildbackup, ec );

         if( !ec )
            ch->print( "&RPlayer's area data destroyed. Area saved as backup.\r\n" );
         else if( ec != std::errc::no_such_file_or_directory )
         {
            ch->print_fmt( "&RUnknown error - {} (area data). Report to the admins.\r\n", ec.message() );
            log_printf( "Error renaming {} to {}: {}", buildfile.string(), buildbackup.string(), ec.message() );
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
   echo_all_printf( ECHOTAR_ALL, "&[immortal]The Wedgy screams, 'You are MINE {}!!!'", victim->name );
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

   for( auto it = victim->carrying.begin(  ); it != victim->carrying.end(  ); )
   {
      obj_data *obj = *it;
      ++it;

      obj->extract(  );
   }
}

CMDF( do_advance )
{
   std::string arg1;
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

   if( ( level = std::stoi( argument ) ) < 1 || level > LEVEL_AVATAR )
   {
      ch->print_fmt( "Level range is 1 to {}.\r\n", LEVEL_AVATAR );
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
      ch->print_fmt( "If your trying to immortalize {}, use the immortalize command.\r\n", victim->name );
      return;
   }

   if( level == victim->level )
   {
      ch->print_fmt( "What would be the point? {} is already level {}.\r\n", victim->name, level );
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

         ch->print_fmt( "Demoting {} from level {} to level {}!\r\n", victim->name, victim->level, level );
         victim->print( "Cursed and forsaken! The gods have lowered your level...\r\n" );
      }
      else
      {
         ch->print_fmt( "{} is already level {}. Re-advancing...\r\n", victim->name, level );
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
      victim->pcdata->rank.clear();
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
      ch->print_fmt( "Raising {} from level {} to level {}!\r\n", victim->name, victim->level, level );
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
   std::list<affect_data *>::iterator paf;

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
   ch->hit = umax( 1, ch->hit );
   ch->mana = umax( 1, ch->mana );
   ch->move = umax( 1, ch->move );
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
   ch->alignment = urange( -1000, ch->alignment, 1000 );
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

   for( auto* obj : ch->carrying )
   {
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
      ch->print_fmt( "Don't be silly, {} is already immortal.\r\n", victim->name );
      return;
   }

   if( victim->level != LEVEL_AVATAR )
   {
      ch->print( "This player is not yet worthy of immortality.\r\n" );
      return;
   }

   ch->print( "Immortalizing a player...\r\n" );
   act( AT_IMMORT, "$n begins to chant softly... then raises $s arms to the sky...", ch, nullptr, nullptr, TO_ROOM );
   victim->print( "&WYou suddenly feel very strange...\r\n\r\n" );
   interpret( victim, "help M_GODLVL1_" );
   victim->print( "&WYou awake... all your possessions are gone.\r\n" );

   for( auto it = victim->carrying.begin(  ); it != victim->carrying.end(  ); )
   {
      obj_data *obj = *it;
      ++it;

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
      victim->pcdata->clan = nullptr;
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
   victim->pcdata->rank.clear();
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
   std::string arg1;
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
      ch->print_fmt( "Level must be 0 (reset) or 1 to {}.\r\n", MAX_LEVEL );
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
   std::string arg1, arg2;
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

      act( AT_ACTION, "You stash some coins on $N.", ch, nullptr, victim, TO_CHAR );

      if( sysdata->save_flags.test( SV_GIVE ) && !ch->char_died(  ) )
         ch->save(  );

      if( sysdata->save_flags.test( SV_RECEIVE ) && !victim->char_died(  ) )
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
      act( AT_PLAIN, "$N has $S hands full.", ch, nullptr, victim, TO_CHAR );
      return;
   }

   if( victim->carry_weight + ( obj->get_weight(  ) / obj->count ) > victim->can_carry_w(  ) )
   {
      act( AT_PLAIN, "$N can't carry that much weight.", ch, nullptr, victim, TO_CHAR );
      return;
   }

   if( obj->extra_flags.test( ITEM_PROTOTYPE ) && !victim->can_take_proto(  ) )
   {
      act( AT_PLAIN, "You cannot give that to $N!", ch, nullptr, victim, TO_CHAR );
      return;
   }

   obj->separate(  );
   obj->from_char(  );

   act( AT_ACTION, "You stash $p on $N.", ch, obj, victim, TO_CHAR );
   obj = obj->to_char( victim );

   if( sysdata->save_flags.test( SV_GIVE ) && !ch->char_died(  ) )
      ch->save(  );

   if( sysdata->save_flags.test( SV_RECEIVE ) && !victim->char_died(  ) )
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
   std::string arg, arg1, arg2, arg3, who;
   char_data *vch = nullptr;
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
         act( AT_TELL, "$n tells you, 'Keep your hands to yourself!'", vch, nullptr, ch, TO_VICT );
         return;
      }
   }

   if( is_number( arg1 ) )
      vnum = atoi( arg1.c_str(  ) );
   else
      vnum = -1;

   count = 0;
   obj_data *obj = nullptr;
   std::list<obj_data *>::iterator iobj;
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

   if( !vch && ( vch = obj->who_carrying(  ) ) != nullptr )
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
         act( AT_IMMORT, "You take $p!", ch, obj, nullptr, TO_CHAR );
      }
      else
         act( AT_IMMORT, "You silently take $p.", ch, obj, nullptr, TO_CHAR );
   }
}

room_index *select_random_room( bool pickmap )
{
   room_index *pRoomIndex = nullptr;

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
   room_index *pRoomIndex;
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

   if( schance == 1 || victim->is_immortal(  ) )
   {
      continent_data *continent;
      int x, y;
      short sector;

      for( ;; )
      {
         continent = pick_random_continent( );
         x = number_range( 0, MAX_X - 1 );
         y = number_range( 0, MAX_Y - 1 );

         if( continent )
         {
            sector = continent->get_terrain( x, y );
            if( sector == -1 )
               continue;
            if( sect_show[sector].canpass )
               break;
         }
      }

      if( continent )
      {
         act( AT_MAGIC, "With the sweep of an arm, $n flings $N to the astral winds.", ch, nullptr, victim, TO_NOTVICT );
         act( AT_MAGIC, "With the sweep of an arm, $n flings you to the astral winds.", ch, nullptr, victim, TO_VICT );
         act( AT_MAGIC, "With the sweep of an arm, you fling $N to the astral winds.", ch, nullptr, victim, TO_CHAR );
         enter_map( victim, nullptr, x, y, continent->name );
         victim->position = POS_STANDING;
         act( AT_MAGIC, "$n is deposited in a heap by the astral winds.", victim, nullptr, nullptr, TO_ROOM );
      }
      else
      {
         ch->print_fmt( "No valid continents to scatter {} to.\r\n", victim->name );
         return;
      }
   }
   else
   {
      if( !( pRoomIndex = select_random_room( true ) ) )
      {
         ch->print( "&[immortal]No room selected. Scatter command cancelled.\r\n" );
         return;
      }

      if( victim->fighting )
         victim->stop_fighting( true );
      act( AT_MAGIC, "With the sweep of an arm, $n flings $N to the astral winds.", ch, nullptr, victim, TO_NOTVICT );
      act( AT_MAGIC, "With the sweep of an arm, $n flings you to the astral winds.", ch, nullptr, victim, TO_VICT );
      act( AT_MAGIC, "With the sweep of an arm, you fling $N to the astral winds.", ch, nullptr, victim, TO_CHAR );
      leave_map( victim, nullptr, pRoomIndex );
      victim->position = POS_RESTING;
      act( AT_MAGIC, "$n is deposited in a heap by the astral winds.", victim, nullptr, nullptr, TO_ROOM );
   }
}

CMDF( do_strew )
{
   std::string arg1;
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
      act( AT_MAGIC, "$n gestures and an unearthly gale sends $N's coins flying!", ch, nullptr, victim, TO_NOTVICT );
      act( AT_MAGIC, "You gesture and an unearthly gale sends $N's coins flying!", ch, nullptr, victim, TO_CHAR );
      act( AT_MAGIC, "As $n gestures, an unearthly gale sends your currency flying!", ch, nullptr, victim, TO_VICT );
      return;
   }

   if( !( pRoomIndex = select_random_room( false ) ) )
   {
      ch->print( "&[immortal]No room selected. Strew command cancelled." );
      return;
   }

   if( !str_cmp( argument, "inventory" ) )
   {
      act( AT_MAGIC, "$n speaks a single word, sending $N's possessions flying!", ch, nullptr, victim, TO_NOTVICT );
      act( AT_MAGIC, "You speak a single word, sending $N's possessions flying!", ch, nullptr, victim, TO_CHAR );
      act( AT_MAGIC, "$n speaks a single word, sending your possessions flying!", ch, nullptr, victim, TO_VICT );

      for( auto it = victim->carrying.begin(  ); it != victim->carrying.end(  ); )
      {
         obj_data *obj_lose = *it;
         ++it;

         obj_lose->from_char(  );
         obj_lose->to_room( pRoomIndex, nullptr );
         ch->pager_fmt( "\t&w{} sent to {}\r\n", capitalize( obj_lose->short_descr ), pRoomIndex->vnum );
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
   act( AT_OBJECT, "Searching $N ...", ch, nullptr, victim, TO_CHAR );

   int count = 0;
   for( auto it = victim->carrying.begin(  ); it != victim->carrying.end(  ); )
   {
      obj_data *obj_lose = *it;
      ++it;

      obj_lose->from_char(  );
      obj_lose->to_char( ch );
      ch->pager_fmt( "  &G... {} (&g{}) &Gtaken.\r\n", capitalize( obj_lose->short_descr ), obj_lose->name );
      ++count;
   }
   if( !count )
      ch->pager( "&GNothing found to take.\r\n" );
}

inline constexpr auto RESTORE_INTERVAL = std::chrono::hours(18);
CMDF( do_restore )
{
   deity_data *deity = nullptr;
   std::string arg;

   ch->set_color( AT_IMMORT );

   if( argument.empty(  ) )
   {
      ch->print( "Restore whom?\r\n" );
      return;
   }

   argument = one_argument( argument, arg );

   // Restore-by-deity. -- Alty
   if( !str_cmp( arg, "deity" ) )
   {
      argument = one_argument( argument, arg );
      if( !( deity = get_deity( arg ) ) )
      {
         ch->print( "No such deity holds weight on this world.\r\n" );
         return;
      }
   }

   if( deity || !str_cmp( argument, "all" ) )
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

      for( auto it = pclist.begin(  ); it != pclist.end(  ); )
      {
         char_data *vch = *it;
         ++it;

         if( !vch->is_immortal(  ) && !vch->CAN_PKILL(  ) && !in_arena( vch ) )
         {
            if( deity && vch->pcdata->deity != deity )
               continue;

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
            act( AT_IMMORT, "$n has restored you.", ch, nullptr, vch, TO_VICT );
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

      if( victim == ch )
      {
         ch->print( "Restore yourself? Don't be silly.\r\n" );
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
      act( AT_IMMORT, "$n has restored you.", ch, nullptr, victim, TO_VICT );
      ch->print( "Ok.\r\n" );
   }
}

CMDF( do_restoretime )
{
   ch->set_color( AT_IMMORT );

   if( last_restore_all_time == std::chrono::system_clock::time_point{} )
   {
      ch->print( "There has been no restore all since reboot.\r\n" );
   }
   else
   {
      auto elapsed = current_time - last_restore_all_time;

      // Break down the duration into hours and minutes
      auto hours = std::chrono::duration_cast<std::chrono::hours>( elapsed );
      auto minutes = std::chrono::duration_cast<std::chrono::minutes>( elapsed % std::chrono::hours(1) );

      ch->print_fmt("The last restore all was {} hours and {} minutes ago.\r\n", hours.count(), minutes.count() );
   }

   if( !ch->pcdata )
      return;

   // Check personal restore time
   if( ch->pcdata->restore_time == std::chrono::system_clock::time_point{} )
   {
      ch->print( "You have never done a restore all.\r\n" );
   }
   else
   {
      auto elapsed = current_time - ch->pcdata->restore_time;

      auto hours = std::chrono::duration_cast<std::chrono::hours>( elapsed );
      auto minutes = std::chrono::duration_cast<std::chrono::minutes>( elapsed % std::chrono::hours(1) );

      ch->print_fmt( "Your last restore all was {} hours and {} minutes ago.\r\n", hours.count(), minutes.count() );
   }
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
      ch->print_fmt( "&C{} is now unfrozen.\r\n", victim->name );
   }
   else
   {
      if( victim->switched )
         do_return( victim->switched, "" );
      victim->set_pcflag( PCFLAG_FREEZE );
      if( !victim->desc )
         add_login_message( victim->name, 15, "" );
      else
         victim->print( "&CA godly force turns your body to ice!\r\n" );
      ch->print_fmt( "&CYou have frozen {}.\r\n", victim->name );
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
      ch->print_fmt( "LOG removed from {}.\r\n", victim->name );
   }
   else
   {
      victim->set_pcflag( PCFLAG_LOG );
      ch->print_fmt( "LOG applied to {}.\r\n", victim->name );
   }
   victim->save( );
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
      ch->print_fmt( "LITTERBUG removed from {}.\r\n", victim->name );
   }
   else
   {
      victim->set_pcflag( PCFLAG_LITTERBUG );
      victim->print( "A strange force prevents you from dropping any more items!\r\n" );
      ch->print_fmt( "LITTERBUG set on {}.\r\n", victim->name );
   }
   victim->save( );
}

CMDF( do_nobio )
{
   char_data *victim;

   ch->set_color( AT_IMMORT );

   if( argument.empty(  ) )
   {
      ch->print( "Nobio whom?\r\n" );
      return;
   }

   if( !( victim = get_wizvictim( ch, argument, true ) ) )
      return;

   if( victim->has_pcflag( PCFLAG_NOBIO ) )
   {
      victim->unset_pcflag( PCFLAG_NOBIO );
      victim->print( "You can set a bio again.\r\n" );
      ch->print_fmt( "NOBIO removed from {}.\r\n", victim->name );
   }
   else
   {
      victim->set_pcflag( PCFLAG_NOBIO );
      if( !victim->desc )
         add_login_message( victim->name, 9, "" );
      else
         victim->print( "You can't set a bio!\r\n" );
      ch->print_fmt( "NOBIO applied to {}.\r\n", victim->name );
   }
   victim->save( );
}

CMDF( do_nodesc )
{
   char_data *victim;

   ch->set_color( AT_IMMORT );

   if( argument.empty(  ) )
   {
      ch->print( "Nodesc whom?\r\n" );
      return;
   }

   if( !( victim = get_wizvictim( ch, argument, true ) ) )
      return;

   if( victim->has_pcflag( PCFLAG_NODESC ) )
   {
      victim->unset_pcflag( PCFLAG_NODESC );
      victim->print( "You can set a description again.\r\n" );
      ch->print_fmt( "NODESC removed from {}.\r\n", victim->name );
   }
   else
   {
      victim->set_pcflag( PCFLAG_NODESC );
      if( !victim->desc )
         add_login_message( victim->name, 11, "" );
      else
         victim->print( "You can't set a description!\r\n" );
      ch->print_fmt( "NODESC applied to {}.\r\n", victim->name );
   }
   victim->save( );
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
      ch->print_fmt( "NOEMOTE removed from {}.\r\n", victim->name );
   }
   else
   {
      victim->set_pcflag( PCFLAG_NO_EMOTE );
      if ( !victim->desc )
         add_login_message( victim->name, 16, "" );
      else
         victim->print( "You can't emote!\r\n" );
      ch->print_fmt( "NOEMOTE applied to {}.\r\n", victim->name );
   }
   victim->save( );
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
      ch->print_fmt( "NOTELL removed from {}.\r\n", victim->name );
   }
   else
   {
      victim->set_pcflag( PCFLAG_NO_TELL );
      if( !victim->desc )
         add_login_message( victim->name, 14, "" );
      else
         victim->print( "You can't send tells!\r\n" );
      ch->print_fmt( "NOTELL applied to {}.\r\n", victim->name );
   }
   victim->save( );
}

CMDF( do_notitle )
{
   char_data *victim;
   std::string buf;

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
      ch->print_fmt( "NOTITLE removed from {}.\r\n", victim->name );
   }
   else
   {
      victim->set_pcflag( PCFLAG_NOTITLE );
      buf = std::format( "the {}", title_table[victim->Class][victim->level][victim->sex] );
      victim->set_title( buf );
      if( !victim->desc )
         add_login_message( victim->name, 8, "" );
      else
         victim->print( "You can't set your own title!\r\n" );
      ch->print_fmt( "NOTITLE set on {}.\r\n", victim->name );
   }
   victim->save( );
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
      ch->print_fmt( "NOURL removed from {}.\r\n", victim->name );
   }
   else
   {
      victim->set_pcflag( PCFLAG_NO_URL );
      victim->pcdata->homepage.clear();
      if( !victim->desc )
         add_login_message( victim->name, 12, "" );
      else
         victim->print( "You can't set a homepage!\r\n" );
      ch->print_fmt( "NOURL applied to {}.\r\n", victim->name );
   }
   victim->save( );
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
      ch->print_fmt( "NOEMAIL removed from {}.\r\n", victim->name );
   }
   else
   {
      victim->set_pcflag( PCFLAG_NO_EMAIL );
      victim->pcdata->email.clear();
      victim->print( "You can't set an email address!\r\n" );
      ch->print_fmt( "NOEMAIL applied to {}.\r\n", victim->name );
   }
   victim->save( );
}

CMDF( do_nobeep )
{
   char_data *victim;

   ch->set_color( AT_IMMORT );

   if( argument.empty(  ) )
   {
      ch->print( "NoBeep whom?\r\n" );
      return;
   }

   if( !( victim = get_wizvictim( ch, argument, true ) ) )
      return;

   if( victim->has_pcflag( PCFLAG_NO_BEEP ) )
   {
      victim->unset_pcflag( PCFLAG_NO_BEEP );
      victim->print( "You can send beeps again.\r\n" );
      ch->print_fmt( "NOBEEP removed from {}.\r\n", victim->name );
   }
   else
   {
      victim->set_pcflag( PCFLAG_NO_BEEP );
      if( !victim->desc )
         add_login_message( victim->name, 13, "" );
      else
         victim->print( "You can't send beeps anymore!\r\n" );
      ch->print_fmt( "NOBEEP applied to {}.\r\n", victim->name );
   }
   victim->save();
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
      ch->print_fmt( "SILENCE removed from {}.\r\n", victim->name );
   }
   else
   {
      victim->set_pcflag( PCFLAG_SILENCE );
      if( !victim->desc )
         add_login_message( victim->name, 7, "" );
      else
         victim->print( "You can't use channels!\r\n" );
      ch->print_fmt( "You SILENCE {}.\r\n", victim->name );
   }
   victim->save( );
}

CMDF( do_peace )
{
   act( AT_IMMORT, "$n booms, 'PEACE!'", ch, nullptr, nullptr, TO_ROOM );
   act( AT_IMMORT, "You boom, 'PEACE!'", ch, nullptr, nullptr, TO_CHAR );

   for( auto* rch : ch->in_room->people )
   {
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

/* Output of command reformatted by Samson 2-8-98, and again on 4-7-98 */
CMDF( do_users )
{
   ch->set_pager_color( AT_PLAIN );

   ch->pager( "Desc|     Constate      |Idle|    Player    | IP Address        Hostname\r\n" );
   ch->pager( "----+-------------------+----+--------------+-----------------+----------------\r\n" );

   int count = 0;
   for( auto* d : dlist )
   {
      std::string st;

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
            ch->pager_fmt( " {:3}| {:<17} |{:4}| {:<12} | {:<15} | {}\r\n", d->descriptor, st, d->idle / 4,
                        d->original ? d->original->name : d->character ? d->character->name : "(None!)", d->ipaddress, d->hostname );
         }
      }
      else
      {
         if( ( ch->get_trust(  ) >= LEVEL_SUPREME || ( d->character && ch->can_see( d->character, true ) ) )
             && ( !str_prefix( argument, d->hostname ) || ( d->character && !str_prefix( argument, d->character->name ) ) ) )
         {
            ++count;
            ch->pager_fmt( " {:3}| {:2}|{:4}| {:<12} | {:<15} | {} \r\n", d->descriptor, d->connected, d->idle / 4,
                        d->original ? d->original->name : d->character ? d->character->name : "(None!)", d->ipaddress, d->hostname );
         }
      }
   }
   ch->pager_fmt( "{} user{}.\r\n", count, count == 1 ? "" : "s" );
}

CMDF( do_invis )
{
   std::string arg;
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
         ch->print_fmt( "Wizinvis level set to {}.\r\n", level );
      }

      else
      {
         ch->mobinvis = level;
         ch->print_fmt( "Mobinvis level set to {}.\r\n", level );
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
         act( AT_IMMORT, "$n slowly fades into existence.", ch, nullptr, nullptr, TO_ROOM );
         ch->print( "You slowly fade back into existence.\r\n" );
      }
      else
      {
         act( AT_IMMORT, "$n slowly fades into thin air.", ch, nullptr, nullptr, TO_ROOM );
         ch->print( "You slowly vanish into thin air.\r\n" );
         ch->set_pcflag( PCFLAG_WIZINVIS );
      }
   }
   else
   {
      if( ch->has_actflag( ACT_MOBINVIS ) )
      {
         ch->unset_actflag( ACT_MOBINVIS );
         act( AT_IMMORT, "$n slowly fades into existence.", ch, nullptr, nullptr, TO_ROOM );
         ch->print( "You slowly fade back into existence.\r\n" );
      }
      else
      {
         act( AT_IMMORT, "$n slowly fades into thin air.", ch, nullptr, nullptr, TO_ROOM );
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
   std::filesystem::path fname;
   int old_room_vnum;

   ch->set_color( AT_IMMORT );

   if( argument.empty(  ) )
   {
      ch->print( "Usage: loadup <playername>\r\n" );
      return;
   }

   std::list<descriptor_data *>::iterator ds;
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

   d = nullptr;
   fname = std::format( "{}{}/{}", PLAYER_DIR, static_cast<char>( std::tolower( argument.front() ) ), capitalize( argument ) );

   if( !std::filesystem::exists( fname ) || !std::filesystem::is_regular_file( fname ) || !check_parse_name( argument, false ) )
   {
      ch->print( "&YNo such player exists.\r\n" );
      return;
   }

   d = new descriptor_data;
   d->init(  );
   d->connected = CON_PLOADED;

   load_char_obj( d, argument, false, false );
   charlist.push_back( d->character );
   pclist.push_back( d->character );

   if( !d->character->to_room( ch->in_room ) )
      log_printf( "char_to_room: {}:{}, line {}.", __FILE__, __func__, __LINE__ );

   old_room_vnum = d->character->in_room->vnum;
   if( d->character->get_trust(  ) >= ch->get_trust(  ) )
   {
      interpret( d->character, "say How dare you load my Pfile!" );
      cmdf( d->character, "dino {}", ch->name );
      ch->print_fmt( "I think you'd better leave {} alone!\r\n", argument );
      d->character->desc = nullptr;
      interpret( d->character, "quit auto" );
      return;
   }

   d->character->desc = nullptr;
   d->character = nullptr;
   deleteptr( d );

   ch->print_fmt( "&R{} loaded from room {}.\r\n", capitalize( argument ), old_room_vnum );
   act_printf( AT_IMMORT, ch, nullptr, nullptr, TO_ROOM, "{} appears from nowhere, eyes glazed over.", capitalize( argument ) );
   ch->print( "Done.\r\n" );
}

/*
 * Extract area names from "input" string and place result in "output" string
 * e.g. "aset joe.are sedit susan.are cset" --> "joe.are susan.are"
 * - Gorog
 */
 /*
  * Rewrite by Xorith. 12/1/03
  */
std::string extract_area_names( char_data * ch )
{
   std::string tbuf, tarea, area_names;

   if( !ch || ch->isnpc(  ) )
      return "";

   if( ch->pcdata->bestowments.empty(  ) )
      return "";

   tbuf = ch->pcdata->bestowments;
   tbuf = one_argument( tbuf, tarea );
   if( tarea.empty(  ) )
      return "";

   while( !tbuf.empty(  ) )
   {
      if( tarea.find( ".are" ) != std::string::npos )
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
   std::string buf, tarea;

   if( !ch || ch->isnpc(  ) || ch->pcdata->bestowments.empty(  ) )
      return;

   buf = ch->pcdata->bestowments;
   buf = one_argument( buf, tarea );
   if( tarea.empty(  ) )
      return;

   while( !buf.empty(  ) )
   {
      if( tarea.find( ".are" ) != std::string::npos )
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
   std::string arg, buf;
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
      ch->print_fmt( "{} is not an immortal and may not have an area bestowed.\r\n", victim->name );
      return;
   }

   if( argument.empty(  ) || !str_cmp( argument, "list" ) )
   {
      if( victim->pcdata->bestowments.empty(  ) )
      {
         ch->print_fmt( "{} does not have any areas bestowed upon them.\r\n", victim->name );
         return;
      }
      buf = extract_area_names( victim );
      if( buf.empty(  ) )
         ch->print_fmt( "{} does not have any areas bestowed upon them.\r\n", victim->name );
      else
         ch->print_fmt( "{}'s bestowed areas: %s\r\n", victim->name, buf );
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
            ch->print_fmt( "{} does not have an area named %s bestowed.\r\n", victim->name, arg );
            return;
         }
         removename( victim->pcdata->bestowments, arg );
         ch->print_fmt( "Removed area {} from {}.\r\n", arg, victim->name );
         victim->save(  );
      }
      return;
   }

   if( !str_cmp( arg, "none" ) )
   {
      if( victim->pcdata->bestowments.empty(  ) || victim->pcdata->bestowments.find( ".are" ) == std::string::npos )
      {
         ch->print_fmt( "{} has no areas bestowed!\r\n", victim->name );
         return;
      }
      remove_area_names( victim );
      victim->save(  );
      ch->print_fmt( "All of {}'s bestowed areas have been removed.\r\n", victim->name );
      return;
   }

   while( !argument.empty(  ) )
   {
      if( arg.find( ".are" ) == std::string::npos )
      {
         ch->print_fmt( "'{}' is not a valid area to bestow.\r\n", arg );
         ch->print( "You can only bestow an area name\r\n" );
         ch->print( "E.G. bestow joe sam.are\r\n" );
         return;
      }

      if( hasname( victim->pcdata->bestowments, arg ) )
      {
         ch->print_fmt( "{} already has the area %s bestowed.\r\n", victim->name, arg );
         return;
      }

      smash_tilde( arg );
      addname( victim->pcdata->bestowments, arg );
      victim->print_fmt( "{} has bestowed on you the area: %s\r\n", ch->name, arg );
      ch->print_fmt( "{} has been bestowed: %s\r\n", victim->name, arg );
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
      ch->print_fmt( "{} is not immortal, and cannot be demoted. Use the advance command.\r\n", victim->name );
      return;
   }

   ch->print( "Demoting an immortal!\r\n" );
   victim->print( "You have been demoted!\r\n" );

   victim->level -= 1;

   /*
    * Rank fix added by Narn. 
    */
   victim->pcdata->rank.clear();

   advance_level( victim );
   victim->exp = exp_level( victim->level );
   victim->trust = 0;

   /*
    * Stuff added to make sure players wizinvis level doesn't stay higher
    * than their actual level and to take wizinvis away from advance below 100
    */
   if( victim->has_pcflag( PCFLAG_WIZINVIS ) )
   {
      victim->unset_pcflag( PCFLAG_WIZINVIS );
      victim->pcdata->wizinvis = 0;
   }

   if( victim->level == LEVEL_AVATAR )
   {
      std::filesystem::path buf;
      std::error_code ec;

      victim->unset_pcflag( PCFLAG_HOLYLIGHT );
      buf = std::format( "{}{}", GOD_DIR, capitalize( victim->name ) );
      if( std::filesystem::remove( buf, ec ) )
         ch->print( "Player's immortal data destroyed.\r\n" );
      victim->print( "You have been thrown from the heavens by the Gods!\r\nYou are no longer immortal!\r\n" );
      victim->unset_pcflag( PCFLAG_PASSDOOR );
      victim->pcdata->realm = nullptr;
      victim->pcdata->realm_name.clear();
   }
   funcf( ch, do_bestow, "{} none", victim->name );
   victim->save(  );
   make_wizlist(  );
   build_wizinfo(  );
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
      ch->print_fmt( "{} is not immortal, and cannot be promoted. Use the advance command.\r\n", victim->name );
      return;
   }

   if( victim->level == LEVEL_AVATAR )
   {
      ch->print_fmt( "You must use the immortalize command to make {} an immortal.\r\n", victim->name );
      return;
   }

   level = victim->level + 1;
   if( level > MAX_LEVEL )
   {
      ch->print_fmt( "Cannot promote {} past the max level of {}.\r\n", victim->name, MAX_LEVEL );
      return;
   }

   act( AT_IMMORT, "You make some arcane gestures with your hands, then point a finger at $N!", ch, nullptr, victim, TO_CHAR );
   act( AT_IMMORT, "$n makes some arcane gestures with $s hands, then points $s finger at you!", ch, nullptr, victim, TO_VICT );
   act( AT_IMMORT, "$n makes some arcane gestures with $s hands, then points $s finger at $N!", ch, nullptr, victim, TO_NOTVICT );
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
   victim->pcdata->rank = "Immortal";
   victim->save(  );
   make_wizlist(  );
   build_wizinfo(  );
   if( victim->alignment > 350 )
      echo_all_printf( ECHOTAR_ALL, "&WA bright white flash arcs across the sky as {} gains in power!", victim->name );
   if( victim->alignment >= -350 && victim->alignment <= 350 )
      echo_all_printf( ECHOTAR_ALL, "&wA dull grey flash arcs across the sky as {} gains in power!", victim->name );
   if( victim->alignment < -350 )
      echo_all_printf( ECHOTAR_ALL, "&zAn eerie black flash arcs across the sky as {} gains in power!", victim->name );
}

/* Online high level immortal command for displaying what the encryption
 * of a name/password would be, taking in 2 arguments - the name and the
 * password - can still only change the password if you have access to 
 * pfiles and the correct password
 */
/*
 * Samson caught this.
 * I forgot that this function even existed... *whoops*
 * Anyway, it is rewritten with bounds checking
 */
CMDF( do_form_password )
{
   std::string pwcheck;

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

   pwcheck = password_hash( argument );
   ch->print_fmt( "{} results in the encrypted string: {}\r\n", argument, pwcheck );
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
   char_data *victim = nullptr;
   std::filesystem::path buf;
   std::error_code ec;
   bool found = false;

   ch->set_color( AT_IMMORT );

   if( argument.empty(  ) )
   {
      ch->print( "Destroy what player file?\r\n" );
      return;
   }

   if( !check_parse_name( argument, false ) )
   {
      ch->print( "That's not a valid player file name!\r\n" );
      return;
   }

   std::list<char_data *>::iterator ich;
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
      std::list<descriptor_data *>::iterator ds;
      descriptor_data *d = nullptr;
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
      saving_char = nullptr;
      victim->extract( true );
      for( int x = 0; x < MAX_WEAR; ++x )
         for( int y = 0; y < MAX_LAYERS; ++y )
            save_equipment[x][y] = nullptr;
   }

   buf = std::format( "{}{}/{}", PLAYER_DIR, static_cast<char>( std::tolower( argument.front() ) ), capitalize( argument ) );

   if( std::filesystem::remove( buf, ec ) )
      ch->print_fmt( "&RPlayer {} destroyed.\r\n", argument );
   else if( ec && ec != std::errc::no_such_file_or_directory )
   {
      ch->print_fmt( "&RUnknown error: {}. Report to Samson.\r\n", ec.message() );
      log_printf( "Error destroying {}: {}", buf.string(), ec.message() );
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
in game. If there is no #, the action will be executed for every room containing
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
const std::string name_expand( char_data * ch )
{
   std::string name;
   std::ostringstream expanded;

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
   for( auto* rch : ch->in_room->people )
   {
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
   std::string range;
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
   if( fEverywhere && argument.contains( "#" ) )
   {
      ch->print( "Cannot use FOR EVERYWHERE with the # thingie.\r\n" );
      return;
   }

   ch->set_color( AT_PLAIN );
   if( argument.contains( "#" ) )   /* replace # ? */
   {
      for( auto it = charlist.begin(  ); it != charlist.end(  ); )
      {
         char_data *p = *it;
         ++it;

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
            std::string command = argument; // Buffer to process
            std::string target = name_expand( p );   // Target for the command
            std::string::size_type pos = 0;

            // String replacement loop since string_replace() won't accept a char argument
            while( ( pos = command.find( '#', pos ) ) != std::string::npos )
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
               log_printf( "char_to_room: {}:{}, line {}.", __FILE__, __func__, __LINE__ );
            interpret( ch, command );
            ch->from_room(  );
            if( !ch->to_room( old_room ) )
               log_printf( "char_to_room: {}:{}, line {}.", __FILE__, __func__, __LINE__ );
         }  /* if found */
      }  /* for every char */
   }
   else  /* just for every room with the appropriate people in it */
   {
      std::map<int, room_index *>::iterator iroom;

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
         for( auto* p : room->people )
         {
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
               log_printf( "char_to_room: {}:{}, line {}.", __FILE__, __func__, __LINE__ );
            interpret( ch, argument );
            ch->from_room(  );
            if( !ch->to_room( old_room ) )
               log_printf( "char_to_room: {}:{}, line {}.", __FILE__, __func__, __LINE__ );
         }  /* if found */
      }  /* for every room in a bucket */
   }  /* if .contains() */
}  /* do_for */

CMDF( do_hell )
{
   char_data *victim;
   std::string arg;
   short htime;
   bool h_d = false;

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

   if( victim->pcdata->release_date != std::chrono::system_clock::time_point{} )
   {
      ch->print_fmt( "They are already in hell until {:24.24}, by {}.\r\n", c_time( victim->pcdata->release_date, ch->pcdata->timezone_name ), victim->pcdata->helled_by );
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
   std::chrono::system_clock::time_point tms = current_time;

   if( h_d )
      tms += std::chrono::hours( htime );
   else
      tms += std::chrono::days( htime );
   victim->pcdata->release_date = tms;
   victim->pcdata->helled_by = ch->name;
   ch->print_fmt( "{} will be released from hell at {:24.24}.\r\n", victim->name, c_time( victim->pcdata->release_date, ch->pcdata->timezone_name ) );
   act( AT_MAGIC, "$n disappears in a cloud of hellish light.", victim, nullptr, ch, TO_NOTVICT );
   victim->from_room(  );
   if( !victim->to_room( get_room_index( ROOM_VNUM_HELL ) ) )
      log_printf( "char_to_room: {}:{}, line {}.", __FILE__, __func__, __LINE__ );
   act( AT_MAGIC, "$n appears in a could of hellish light.", victim, nullptr, ch, TO_NOTVICT );
   interpret( victim, "look" );
   if( !victim->desc )
      add_login_message( victim->name, 10, "" );
   else
      victim->print_fmt( "The immortals are not pleased with your actions.\r\n"
                   "You shall remain in hell for {} {}{}.\r\n", htime, ( h_d ? "hour" : "day" ), ( htime == 1 ? "" : "s" ) );
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
   act( AT_MAGIC, "$n disappears in a cloud of godly light.", victim, nullptr, ch, TO_NOTVICT );
   victim->from_room(  );
   if( !victim->to_room( location ) )
      log_printf( "char_to_room: {}:{}, line {}.", __FILE__, __func__, __LINE__ );
   victim->print( "The gods have smiled on you and released you from hell early!\r\n" );
   interpret( victim, "look" );
   if( victim != ch )
      ch->print( "They have been released.\r\n" );
   if( !victim->pcdata->helled_by.empty() )
   {
      if( str_cmp( ch->name, victim->pcdata->helled_by ) )
         ch->print_fmt( "(You should probably write a note to {}, explaining the early release.)\r\n", victim->pcdata->helled_by );
      victim->pcdata->helled_by.clear();
   }
   MOBtrigger = false;
   act( AT_MAGIC, "$n appears in a cloud of godly light.", victim, nullptr, ch, TO_NOTVICT );
   victim->pcdata->release_date = std::chrono::system_clock::time_point{};
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

   int argi = std::stoi( argument );
   if( argi < 1 || argi > sysdata->maxvnum )
   {
      ch->print( "Vnum out of range.\r\n" );
      return;
   }

   int obj_counter = 1;
   for( auto* obj : objlist )
   {
      if( !ch->can_see_obj( obj, true ) || !( argi == obj->pIndexData->vnum ) )
         continue;

      found = true;
      obj_data *in_obj;
      for( in_obj = obj; in_obj->in_obj != nullptr; in_obj = in_obj->in_obj );

      if( in_obj->carried_by != nullptr )
         ch->pager_fmt( "&Y[&W{:2}&Y] &GLevel {} {} carried by {}.\r\n", obj_counter, obj->level, obj->oshort(  ), PERS( in_obj->carried_by, ch, true ) );
      else
         ch->pager_fmt( "&Y[&W{:2}&Y] [&W{:<5}&Y] &G{} in {}.\r\n", obj_counter,
                     ( ( in_obj->in_room ) ? in_obj->in_room->vnum : 0 ), obj->oshort(  ), ( in_obj->in_room == nullptr ) ? "somewhere" : in_obj->in_room->name );

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
}

/* Redone cset command, with more in-game changeable parameters - Samson 2-19-02 */
CMDF( do_cset )
{
   std::string arg;
   int value = -1;

   if( argument.empty(  ) )
   {
      ch->pager( "&WThe following options may be set:\r\n\r\n" );
      ch->pager( "&GSite Parameters\r\n" );
      ch->pager( "---------------\r\n" );
      ch->pager_fmt( "&BMudname  &c: {} &BEmail&c: {} &BPassword&c: [not shown]\r\n", sysdata->mud_name, sysdata->admin_email );
      ch->pager_fmt( "&BURL      &c: {} &BTelnet&c: {}\r\n", show_tilde( sysdata->http ), sysdata->telnet );
      ch->pager_fmt( "&BDB Server&c: {} &BDB Name&c: {} &BDB User&c: {} &BDB Password&c: [not shown]\r\n\r\n",
                  sysdata->dbserver, sysdata->dbname, sysdata->dbuser );

      ch->pager( "&GGame Toggles\r\n" );
      ch->pager( "------------\r\n" );
      ch->pager_fmt( "&BNameauth&c: {} &BImmhost Checking&c: {} &BTestmode&c: {} &BMinimum Ego&c: {}\r\n",
                  sysdata->WAIT_FOR_AUTH ? "Enabled" : "Disabled", sysdata->check_imm_host ? "Enabled" : "Disabled", sysdata->TESTINGMODE ? "On" : "Off", sysdata->minego );
      ch->pager_fmt( "&BPet Save&c: {} &BPfile Pruning&c: {}\r\n",
                  sysdata->save_pets ? "On" : "Off", sysdata->CLEANPFILES ? "Enabled" : "Disabled" );

      if( sysdata->CLEANPFILES )
      {
         ch->pager_fmt( "   &BNewbie Purge &c: {} days\r\n", sysdata->newbie_purge );
         ch->pager_fmt( "   &BRegular Purge&c: {} days\r\n", sysdata->regular_purge );
      }

      ch->pager( "\r\n&GGame Settings - Note: Altering these can have drastic affects on the game. Use with caution.\r\n" );
      ch->pager( "-------------\r\n" );
      ch->pager_fmt( "&BMaxVnum&c: {} &BOverland Radius&c: {} &BReboot Count&c: {} &BAuction Seconds&c: {}\r\n",
                  sysdata->maxvnum, sysdata->mapsize, sysdata->rebootcount, sysdata->auctionseconds );
      ch->pager_fmt( "&BMin Guild Level&c: {} &BMax Condition Value&c: {} &BMax Ignores&c: %zu &BMax Item Impact&c: {} &BInit Weapon Condition&c: {}\r\n",
           sysdata->minguildlevel, sysdata->maxcondval, sysdata->maxign, sysdata->maximpact, sysdata->initcond );

      int freq = sysdata->save_frequency.count();
      ch->pager_fmt( "&BForce Players&c: {} &BPrivate Override&c: {} &BGet Notake&c: {} &BAutosave-Freq&c: {} &BMax Holidays&c: %zu\r\n",
           sysdata->level_forcepc, sysdata->level_override_private, sysdata->level_getobjnotake, freq, sysdata->maxholiday );

      ch->pager_fmt( "&BProto Mod&c: {} &B &BMset Player&c: {} &BBestow Diff&c: {} &BBuild Level&c: {}\r\n",
                  sysdata->level_modify_proto, sysdata->level_mset_player, sysdata->bestow_dif, sysdata->build_level );
      ch->pager_fmt( "&BRead all mail&c: {} &BTake all mail&c: {} &BRead mail free&c: {} &BWrite mail free&c: {}\r\n",
                  sysdata->read_all_mail, sysdata->take_others_mail, sysdata->read_mail_free, sysdata->write_mail_free );
      ch->pager_fmt( "&BHours per day&c: {} &BDays per week&c: {} &BDays per month&c: {} &BMonths per year&c: {} &RDays per year&W: {}\r\n",
           sysdata->hoursperday, sysdata->daysperweek, sysdata->dayspermonth, sysdata->monthsperyear, sysdata->daysperyear );
      ch->pager_fmt( "&BSave Flags&c: {}\r\n", bitset_string( sysdata->save_flags, save_flag ) );
      ch->pager_fmt( "&BWebwho&c: {} &BGame Alarm&c: {}\r\n", sysdata->webwho, sysdata->gameloopalarm );
      ch->pager_fmt( "\r\n&BSeconds per tick&c: {}   &BPulse per second&c: {}\r\n", sysdata->secpertick, sysdata->pulsepersec );
      ch->pager_fmt( "   &RPULSE_TICK&W: {} &RPULSE_VIOLENCE&W: {} &RPULSE_MOBILE&W: {}\r\n", sysdata->pulsetick, sysdata->pulseviolence, sysdata->pulsemobile );
      ch->pager_fmt( "   &RPULSE_CALENDAR&W: {} &RPULSE_ENVIRONMENT&W: {}\r\n", sysdata->pulsecalendar, sysdata->pulseenvironment );
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
         ch->print_fmt( "Mud name set to: {}\r\n", argument );
      }
      save_sysdata(  );
      return;
   }

   if( !str_cmp( arg, "password" ) )
   {
      std::string pwdnew;

      if( argument.length(  ) < 12 )
      {
         ch->print( "New password must be at least 12 characters long.\r\n" );
         return;
      }

      if( argument.front() == '!' )
      {
         ch->print( "New password cannot begin with the '!' character.\r\n" );
         return;
      }

      pwdnew = password_hash( argument ); // SHA-512 Encryption
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
         ch->print_fmt( "Email address set to {}\r\n", argument );
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
         ch->print_fmt( "HTTP address set to {}\r\n", show_tilde( sysdata->http ) );
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
         ch->print_fmt( "Telnet address set to {}\r\n", argument );
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
         ch->print_fmt( "Database server set to: {}\r\n", argument );
      }
      save_sysdata(  );
      return;
   }

   if( !str_cmp( arg, "db-password" ) )
   {
      if( argument.length(  ) < 12 )
      {
         ch->print( "New password must be at least 12 characters long.\r\n" );
         return;
      }

      if( argument.front() == '!' )
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
         ch->print_fmt( "Database name set to: {}\r\n", argument );
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
         ch->print_fmt( "Database username set to: {}\r\n", argument );
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

   if( !str_cmp( arg, "save-flags" ) )
   {
      int x = get_saveflag( argument );

      if( x == -1 || x >= SV_MAX )
      {
         ch->print( "Not a save flag.\r\n" );
         return;
      }
      sysdata->save_flags.flip( x );
      ch->print_fmt( "{} flag toggled.\r\n", argument );
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

   value = std::stoi( argument );

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
         cancel_event( ev_webwho_refresh, nullptr );
      }
      else
      {
         ch->print_fmt( "Webwho refresh set to {} seconds.\r\n", value );
         add_event( value, ev_webwho_refresh, nullptr );
      }
      return;
   }

   if( !str_cmp( arg, "game-alarm" ) )
   {
      sysdata->gameloopalarm = value;
      ch->print_fmt( "Game loop alarm time has been set to {} seconds.\r\n", value );
      save_sysdata(  );
      return;
   }

   if( !str_cmp( arg, "reboot-count" ) )
   {
      sysdata->rebootcount = value;
      ch->print_fmt( "Reboot time counter set to {} minutes.\r\n", value );
      save_sysdata(  );
      return;
   }

   if( !str_cmp( arg, "auction-seconds" ) )
   {
      sysdata->auctionseconds = value;
      ch->print_fmt( "Auction timer set to {} seconds.\r\n", value );
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
      ch->print_fmt( "Newbie Purge set to {}.\r\n", value );
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
      ch->print_fmt( "Regular Purge set to {}.\r\n", value );
      save_sysdata(  );
      return;
   }

   if( !str_cmp( arg, "minimum-ego" ) )
   {
      sysdata->minego = value;
      ch->print_fmt( "Minimum Ego set to {}.\r\n", value );
      save_sysdata(  );
      return;
   }

   if( !str_cmp( arg, "maxvnum" ) )
   {
      std::string lbuf;
      int vnum = 0;

      for( auto* area : arealist )
      {
         if( area->hi_vnum > vnum )
         {
            lbuf = area->name;
            vnum = area->hi_vnum;
         }
      }

      if( value <= vnum )
      {
         ch->print_fmt( "&RError: Cannot set MaxVnum to {}, existing areas extend to {}.\r\n", value, vnum );
         return;
      }

      if( value - vnum < 1000 )
      {
         ch->print_fmt( "Warning: Setting MaxVnum to {} leaves you with less than 1000 vnums beyond the highest area.\r\n", value );
         ch->print_fmt( "Highest area {} ends with vnum {}.\r\n", lbuf, vnum );
      }

      sysdata->maxvnum = value;
      ch->print_fmt( "MaxVnum changed to {}.\r\n", value );
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
      ch->print_fmt( "Overland Radius set to {}.\r\n", value );
      save_sysdata(  );
      return;
   }

   if( !str_cmp( arg, "min-guild-level" ) )
   {
      if( value > LEVEL_AVATAR )
      {
         ch->print_fmt( "&RError: Cannot set Min Guild Level above level {}.\r\n", LEVEL_AVATAR );
         return;
      }
      sysdata->minguildlevel = value;
      ch->print_fmt( "Min Guild Level set to {}.\r\n", value );
      save_sysdata(  );
      return;
   }

   if( !str_cmp( arg, "max-condition-value" ) )
   {
      sysdata->maxcondval = value;
      ch->print_fmt( "Max Condition Value set to {}.\r\n", value );
      save_sysdata(  );
      return;
   }

   if( !str_cmp( arg, "max-ignores" ) )
   {
      sysdata->maxign = value;
      ch->print_fmt( "Max Ignores set to {}.\r\n", value );
      save_sysdata(  );
      return;
   }

   if( !str_cmp( arg, "max-item-impact" ) )
   {
      sysdata->maximpact = value;
      ch->print_fmt( "Max Item Impact set to {}.\r\n", value );
      save_sysdata(  );
      return;
   }

   if( !str_cmp( arg, "init-weapon-condition" ) )
   {
      sysdata->initcond = value;
      ch->print_fmt( "Init Weapon Condition set to {}.\r\n", value );
      save_sysdata(  );
      return;
   }

   if( !str_cmp( arg, "force-players" ) )
   {
      if( value > MAX_LEVEL || value < LEVEL_IMMORTAL )
      {
         ch->print_fmt( "&RError: Cannot set Force Players above level {}, or below level {}.\r\n", MAX_LEVEL, LEVEL_IMMORTAL );
         return;
      }
      sysdata->level_forcepc = value;
      ch->print_fmt( "Force Players set to {}.\r\n", value );
      save_sysdata(  );
      return;
   }

   if( !str_cmp( arg, "private-override" ) )
   {
      if( value > MAX_LEVEL || value < LEVEL_IMMORTAL )
      {
         ch->print_fmt( "&RError: Cannot set Private Override above level {}, or below level {}.\r\n", MAX_LEVEL, LEVEL_IMMORTAL );
         return;
      }
      sysdata->level_override_private = value;
      ch->print_fmt( "Private Override set to {}.\r\n", value );
      save_sysdata(  );
      return;
   }

   if( !str_cmp( arg, "get-notake" ) )
   {
      if( value > MAX_LEVEL || value < LEVEL_IMMORTAL )
      {
         ch->print_fmt( "&RError: Cannot set Get Notake above level {}, or below level {}.\r\n", MAX_LEVEL, LEVEL_IMMORTAL );
         return;
      }
      sysdata->level_getobjnotake = value;
      ch->print_fmt( "Get Notake set to {}.\r\n", value );
      save_sysdata(  );
      return;
   }

   if( !str_cmp( arg, "autosave-freq" ) )
   {
      if( value > 32767 )
      {
         ch->print( "&RError: Cannot set Autosave Freq above 32767 minutes.\r\n" );
         return;
      }
      sysdata->save_frequency = std::chrono::minutes( value );
      ch->print_fmt( "Autosave Freq set to {} minutes.\r\n", value );
      save_sysdata(  );
      return;
   }

   if( !str_cmp( arg, "max-holidays" ) )
   {
      sysdata->maxholiday = value;
      ch->print_fmt( "Max Holiday set to {}.\r\n", value );
      save_sysdata(  );
      return;
   }

   if( !str_cmp( arg, "proto-mod" ) )
   {
      if( value > MAX_LEVEL || value < LEVEL_IMMORTAL )
      {
         ch->print_fmt( "&RError: Cannot set Proto Mod above level {}, or below level {}.\r\n", MAX_LEVEL, LEVEL_IMMORTAL );
         return;
      }
      sysdata->level_modify_proto = value;
      ch->print_fmt( "Proto Mod set to {}.\r\n", value );
      save_sysdata(  );
      return;
   }

   if( !str_cmp( arg, "mset-player" ) )
   {
      if( value > MAX_LEVEL || value < LEVEL_IMMORTAL )
      {
         ch->print_fmt( "&RError: Cannot set Mset Player above level {}, or below level {}.\r\n", MAX_LEVEL, LEVEL_IMMORTAL );
         return;
      }
      sysdata->level_mset_player = value;
      ch->print_fmt( "Mset Player set to {}.\r\n", value );
      save_sysdata(  );
      return;
   }

   if( !str_cmp( arg, "bestow-diff" ) )
   {
      if( value > MAX_LEVEL )
      {
         ch->print_fmt( "&RError: Cannot set Bestow Diff above {}.\r\n", MAX_LEVEL );
         return;
      }
      sysdata->bestow_dif = value;
      ch->print_fmt( "Bestow Diff set to {}.\r\n", value );
      save_sysdata(  );
      return;
   }

   if( !str_cmp( arg, "build-level" ) )
   {
      if( value > MAX_LEVEL || value < LEVEL_IMMORTAL )
      {
         ch->print_fmt( "&RError: Cannot set Build Level above level {}, or below level {}.\r\n", MAX_LEVEL, LEVEL_IMMORTAL );
         return;
      }
      sysdata->build_level = value;
      ch->print_fmt( "Build Level set to {}.\r\n", value );
      save_sysdata(  );
      return;
   }

   if( !str_cmp( arg, "read-all-mail" ) )
   {
      if( value > MAX_LEVEL || value < LEVEL_IMMORTAL )
      {
         ch->print_fmt( "&RError: Cannot set Read all mail above level {}, or below level {}.\r\n", MAX_LEVEL, LEVEL_IMMORTAL );
         return;
      }
      sysdata->read_all_mail = value;
      ch->print_fmt( "Read all mail set to {}.\r\n", value );
      save_sysdata(  );
      return;
   }

   if( !str_cmp( arg, "take-all-mail" ) )
   {
      if( value > MAX_LEVEL || value < LEVEL_IMMORTAL )
      {
         ch->print_fmt( "&RError: Cannot set Take all mail above level {}, or below level {}.\r\n", MAX_LEVEL, LEVEL_IMMORTAL );
         return;
      }
      sysdata->take_others_mail = value;
      ch->print_fmt( "Take all mail set to {}.\r\n", value );
      save_sysdata(  );
      return;
   }

   if( !str_cmp( arg, "read-mail-free" ) )
   {
      if( value > MAX_LEVEL )
      {
         ch->print_fmt( "&RError: Cannot set Read mail free above level {}.\r\n", MAX_LEVEL );
         return;
      }
      sysdata->read_mail_free = value;
      ch->print_fmt( "Read mail free set to {}.\r\n", value );
      save_sysdata(  );
      return;
   }

   if( !str_cmp( arg, "write-mail-free" ) )
   {
      if( value > MAX_LEVEL )
      {
         ch->print_fmt( "&RError: Cannot set Write mail free above level {}.\r\n", MAX_LEVEL );
         return;
      }
      sysdata->write_mail_free = value;
      ch->print_fmt( "Write mail free set to {}.\r\n", value );
      save_sysdata(  );
      return;
   }

   if( !str_cmp( arg, "hours-per-day" ) )
   {
      sysdata->hoursperday = value;
      ch->print_fmt( "Hours per day set to {}.\r\n", value );
      update_calendar(  );
      save_sysdata(  );
      return;
   }

   if( !str_cmp( arg, "days-per-week" ) )
   {
      sysdata->daysperweek = value;
      ch->print_fmt( "Days per week set to {}.\r\n", value );
      update_calendar(  );
      save_sysdata(  );
      return;
   }

   if( !str_cmp( arg, "days-per-month" ) )
   {
      sysdata->dayspermonth = value;
      ch->print_fmt( "Days per month set to {}.\r\n", value );
      update_calendar(  );
      save_sysdata(  );
      return;
   }

   if( !str_cmp( arg, "months-per-year" ) )
   {
      sysdata->monthsperyear = value;
      ch->print_fmt( "Months per year set to {}.\r\n", value );
      update_calendar(  );
      save_sysdata(  );
      return;
   }

   if( !str_cmp( arg, "seconds-per-tick" ) )
   {
      sysdata->secpertick = value;
      ch->print_fmt( "Seconds per tick set to {}.\r\n", value );
      update_timers(  );
      save_sysdata(  );
      return;
   }

   if( !str_cmp( arg, "pulse-per-second" ) )
   {
      sysdata->pulsepersec = value;
      ch->print_fmt( "Pulse per second set to {}.\r\n", value );
      update_timers(  );
      save_sysdata(  );
      return;
   }

   do_cset( ch, "help" );
}

class_type::class_type(  )
{
}

class_type::~class_type(  )
{
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

bool load_class_file( std::string_view fname )
{
   std::filesystem::path buf;
   class_type *Class;
   int cl = -1, tlev = 0, file_ver = 0;
   FILE *fp;

   buf = std::format( "{}{}", CLASS_DIR, fname );
   if( !( fp = fopen( buf.c_str(), "r" ) ) )
   {
      bug( "{}: unable to open {} for reading!", __func__, buf.c_str() );
      return false;
   }

   Class = new class_type;

   for( ;; )
   {
      std::string word = ( feof( fp ) ? "End" : fread_word( fp ) );

      if( word[0] == '\0' )
      {
         log_printf( "{}: EOF encountered reading file!", __func__ );
         word = "End";
      }

      switch ( to_upper( word[0] ) )
      {
         default:
            log_printf( "{}: no match: {}", __func__, word );
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
                  bug( "{}: Class ({}) bad/not found ({})", __func__, !Class->who_name.empty() ? Class->who_name : "name not found", cl );
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
            STDSKEY( "Name", Class->who_name );
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
                  bug( "{}: Skill {} -- Class bad/not found ({})", __func__, word, cl );
               else if( !IS_VALID_SN( sn ) )
                  bug( "{}: Skill {} unknown. Class: {}", __func__, word, cl );
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
                  bug( "{}: Title -- Class bad/not found ({})", __func__, cl );
                  fread_flagstring( fp );
                  fread_flagstring( fp );
               }
               else if( tlev < MAX_LEVEL + 1 )
               {
                  if( file_ver < 2 )
                  {
                     fread_string( title_table[cl][tlev][SEX_MALE], fp );
                     fread_string( title_table[cl][tlev][SEX_FEMALE], fp );

                     title_table[cl][tlev][SEX_NEUTRAL] = title_table[cl][tlev][SEX_MALE];
                     title_table[cl][tlev][SEX_HERMAPHRODYTE] = title_table[cl][tlev][SEX_FEMALE];
                  }
                  else
                  {
                     fread_string( title_table[cl][tlev][SEX_NEUTRAL], fp );
                     fread_string( title_table[cl][tlev][SEX_MALE], fp );
                     fread_string( title_table[cl][tlev][SEX_FEMALE], fp );
                     fread_string( title_table[cl][tlev][SEX_HERMAPHRODYTE], fp );
                  }

                  ++tlev;
               }
               else
                  bug( "{}: Too many titles. Class: {}", __func__, cl );
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

   MAX_PC_CLASS = 0;
   /*
    * Pre-init the class_table with blank classes
    */
   for( int i = 0; i < MAX_CLASS; ++i )
      class_table[i] = nullptr;

   std::filesystem::path classlist = std::format( "{}{}", CLASS_DIR, CLASS_LIST );
   if( !( fpList = fopen( classlist.c_str(), "r" ) ) )
   {
      bug( "{}: Unable to load class list file.", __func__ );
      std::exit( EXIT_FAILURE );
   }

   for( ;; )
   {
      std::string filename = ( feof( fpList ) ? "$" : fread_word( fpList ) );

      if( filename.empty() )
      {
         log_printf( "{}: EOF encountered reading class list!", __func__ );
         break;
      }

      if( filename[0] == '$' )
         break;

      if( !load_class_file( filename ) )
         bug( "{}: Cannot load Class file: {}", __func__, filename );
      else
         ++MAX_PC_CLASS;
   }
   FCLOSE( fpList );

   for( int i = 0; i < MAX_CLASS; ++i )
   {
      if( !class_table[i] )
      {
         class_type *Class = new class_type;

         Class->who_name = "No Class Name";
         class_table[i] = Class;
      }
   }
}

// 1: Initial version.
// 2: ???
constexpr int CLASSFILEVER = 2;

void write_class_file( int cl )
{
   FILE *fpout;
   class_type *Class = class_table[cl];

   std::filesystem::path filename = std::format( "{}{}.class", CLASS_DIR, Class->who_name );
   if( !( fpout = fopen( filename.c_str(), "w" ) ) )
   {
      bug( "{}: Cannot open: {} for writing", __func__, filename.string() );
      return;
   }

   fprintf( fpout, "Version     %d\n", CLASSFILEVER );
   fprintf( fpout, "Name        %s~\n", Class->who_name.c_str() );
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
   fprintf( fpout, "Thac0gain   %f\n", Class->thac0_gain );
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

      if( skill_table[x]->name.empty() )
         break;
      if( ( y = skill_table[x]->skill_level[cl] ) < LEVEL_IMMORTAL )
         fprintf( fpout, "Skill '%s' %d %d\n", skill_table[x]->name.c_str(), y, skill_table[x]->skill_adept[cl] );
   }

   for( int x = 0; x <= MAX_LEVEL; ++x )
      fprintf( fpout, "Title\n%s~\n%s~\n%s~\n%s~\n", title_table[cl][x][SEX_NEUTRAL].c_str(), title_table[cl][x][SEX_MALE].c_str(), title_table[cl][x][SEX_FEMALE].c_str(), title_table[cl][x][SEX_HERMAPHRODYTE].c_str() );

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

   std::filesystem::path classlist = std::format( "{}{}", CLASS_DIR, CLASS_LIST );
   if( !( fpList = fopen( classlist.c_str(), "w" ) ) )
   {
      bug( "{}: Can't open class list for writing.", __func__ );
      return;
   }

   for( int i = 0; i < MAX_PC_CLASS; ++i )
      fprintf( fpList, "%s.class\n", class_table[i]->who_name.c_str() );
   fprintf( fpList, "%s", "$\n" );
   FCLOSE( fpList );
}

/*
 * Display Class information - Thoric
 */
CMDF( do_showclass )
{
   std::string arg1, arg2;
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
      Class = nullptr;
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
   ch->pager_fmt( "&wCLASS: &W{}\r\n", Class->who_name );
   ch->pager_fmt( "&wStarting Weapon:  &W{:<6} &wStarting Armor:     &W{:<6}\r\n", Class->weapon, Class->armor );
   ch->pager_fmt( "&wStarting Legwear: &W{:<6} &wStarting Headwear:  &W{:<6}\r\n", Class->legwear, Class->headwear );
   ch->pager_fmt( "&wStarting Armwear: &W{:<6} &wStarting Footwear:  &W{:<6}\r\n", Class->armwear, Class->footwear );
   ch->pager_fmt( "&wStarting Shield:  &W{:<6} &wStarting Held Item: &W{:<6}\r\n", Class->shield, Class->held );
   ch->pager_fmt( "&wBaseThac0:        &W{:<6} &wThac0Gain: &W{}\r\n", Class->base_thac0, Class->thac0_gain );
   ch->pager_fmt( "&wMax Skill Adept:  &W{:<3} ", Class->skill_adept );
   ch->pager_fmt( "&wHp Min/Hp Max  : &W{:<2}/{:<2}           &wMana  : &W{:<3}\r\n", Class->hp_min, Class->hp_max, Class->fMana ? "yes" : "no " );
   ch->pager_fmt( "&wAffected by:  &W{}\r\n", bitset_string( Class->affected, aff_flags ) );
   ch->pager_fmt( "&wResistant to: &W{}\r\n", bitset_string( Class->resist, ris_flags ) );
   ch->pager_fmt( "&wSusceptible to: &W{}\r\n", bitset_string( Class->suscept, ris_flags ) );

   if( !arg2.empty(  ) )
   {
      int x, y, cnt;

      low = umax( 0, atoi( arg2.c_str(  ) ) );
      hi = urange( low, atoi( argument.c_str(  ) ), MAX_LEVEL );
      for( x = low; x <= hi; ++x )
      {
         ch->pager_fmt( "&wLevel: &W{}     &wExperience required: &W{}\r\n", x, exp_level( x ) );
         ch->pager_fmt( "&wNeutral: &W{:<20} &wMale: &W{:<20} &wFemale: &W{:<20} &wHermaphrodite: &W{}\r\n", title_table[cl][x][SEX_NEUTRAL], title_table[cl][x][SEX_MALE], title_table[cl][x][SEX_FEMALE], title_table[cl][x][SEX_HERMAPHRODYTE] );
         cnt = 0;
         for( y = 0; y < num_skills; ++y )
            if( skill_table[y]->skill_level[cl] == x )
            {
               ch->pager_fmt( "  &[skill]{:<7} {:<19}{:3}     ", skill_tname[skill_table[y]->type], skill_table[y]->name, skill_table[y]->skill_adept[cl] );
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
bool create_new_class( int Class, const std::string & argument )
{
   if( Class >= MAX_CLASS || class_table[Class] == nullptr )
      return false;
   class_table[Class]->who_name = capitalize( argument );
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
      title_table[Class][i][SEX_NEUTRAL] = "Not set.";
      title_table[Class][i][SEX_MALE] = "Not set.";
      title_table[Class][i][SEX_FEMALE] = "Not set.";
      title_table[Class][i][SEX_HERMAPHRODYTE] = "Not set.";
   }
   return true;
}

/*
 * Edit Class information - Thoric
 */
CMDF( do_setclass )
{
   std::string arg1, arg2;
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
      Class = nullptr;
      for( cl = 0; cl < MAX_CLASS && class_table[cl]; ++cl )
      {
         if( class_table[cl]->who_name.empty() )
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
      if( MAX_PC_CLASS >= MAX_CLASS )
      {
         ch->print( "You need to up MAX_CLASS in mud and make clean.\r\n" );
         return;
      }

      std::filesystem::path filename = std::format( "{}.class", arg1 );
      if( !is_valid_filename( ch, CLASS_DIR, filename.string() ) )
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
         ch->print_fmt( "Skill '{}' added at level {} and {}%.\r\n", skill->name, level, adept );
      }
      else
         ch->print_fmt( "No such skill as {}.\r\n", arg2 );
      return;
   }

   if( !str_cmp( arg2, "name" ) )
   {
      class_type *ccheck = nullptr;

      one_argument( argument, arg1 );
      if( arg1.empty(  ) )
      {
         ch->print( "You can't set a class name to nothing.\r\n" );
         return;
      }

      std::filesystem::path filename = std::format( "{}.class", arg1 );
      if( !is_valid_filename( ch, CLASS_DIR, filename.string() ) )
         return;

      for( i = 0; i < MAX_PC_CLASS && class_table[i]; ++i )
      {
         if( class_table[i]->who_name.empty() )
            continue;

         if( !str_cmp( class_table[i]->who_name, arg1 ) )
         {
            ccheck = class_table[i];
            break;
         }
      }
      if( ccheck != nullptr )
      {
         ch->print_fmt( "Already a class called {}.\r\n", arg1 );
         return;
      }

      filename = std::format( "{}{}.class", CLASS_DIR, Class->who_name );
      std::filesystem::remove( filename );
      Class->who_name = capitalize( argument );
      ch->print_fmt( "Class renamed to {}.\r\n", arg1 );
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
            ch->print_fmt( "Unknown flag: {}\r\n", arg2 );
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
            ch->print_fmt( "Unknown flag: {}\r\n", arg2 );
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
            ch->print_fmt( "Unknown flag: {}\r\n", arg2 );
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
      if( to_upper( argument.front() ) == 'Y' )
         Class->fMana = true;
      else
         Class->fMana = false;
      ch->print( "Mana flag toggled.\r\n" );
      write_class_file( cl );
      return;
   }

   if( !str_cmp( arg2, "ntitle" ) )
   {
      std::string arg3;
      int x;

      argument = one_argument( argument, arg3 );
      if( arg3.empty(  ) || argument.empty(  ) )
      {
         ch->print( "Syntax: setclass <Class> ntitle <level> <title>\r\n" );
         return;
      }
      if( ( x = std::stoi( arg3 ) ) < 0 || x > MAX_LEVEL )
      {
         ch->print( "Invalid level.\r\n" );
         return;
      }
      title_table[cl][x][SEX_NEUTRAL] = argument;
      ch->print( "Done.\r\n" );
      write_class_file( cl );
      return;
   }

   if( !str_cmp( arg2, "mtitle" ) )
   {
      std::string arg3;
      int x;

      argument = one_argument( argument, arg3 );
      if( arg3.empty(  ) || argument.empty(  ) )
      {
         ch->print( "Syntax: setclass <Class> mtitle <level> <title>\r\n" );
         return;
      }
      if( ( x = std::stoi( arg3 ) ) < 0 || x > MAX_LEVEL )
      {
         ch->print( "Invalid level.\r\n" );
         return;
      }
      title_table[cl][x][SEX_MALE] = argument;
      ch->print( "Done.\r\n" );
      write_class_file( cl );
      return;
   }

   if( !str_cmp( arg2, "ftitle" ) )
   {
      std::string arg3, arg4;
      int x;

      argument = one_argument( argument, arg3 );
      argument = one_argument( argument, arg4 );
      if( arg3.empty(  ) || argument.empty(  ) )
      {
         ch->print( "Syntax: setclass <Class> ftitle <level> <title>\r\n" );
         return;
      }
      if( ( x = std::stoi( arg4 ) ) < 0 || x > MAX_LEVEL )
      {
         ch->print( "Invalid level.\r\n" );
         return;
      }
      title_table[cl][x][SEX_FEMALE] = argument;
      ch->print( "Done\r\n" );
      write_class_file( cl );
      return;
   }

   if( !str_cmp( arg2, "htitle" ) )
   {
      std::string arg3, arg4;
      int x;

      argument = one_argument( argument, arg3 );
      argument = one_argument( argument, arg4 );
      if( arg3.empty(  ) || argument.empty(  ) )
      {
         ch->print( "Syntax: setclass <Class> htitle <level> <title>\r\n" );
         return;
      }
      if( ( x = std::stoi( arg4 ) ) < 0 || x > MAX_LEVEL )
      {
         ch->print( "Invalid level.\r\n" );
         return;
      }
      title_table[cl][x][SEX_HERMAPHRODYTE] = argument;
      ch->print( "Done\r\n" );
      write_class_file( cl );
      return;
   }
   do_setclass( ch, "" );
}

race_type::race_type(  )
{
}

race_type::~race_type(  )
{
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

void set_bodypart_where_names( race_type *race )
{
   race->bodypart_where_names.clear();

   // The order of these goes by the traditional listing as found in act_info.cpp
   if( race->body_parts.test( PART_HANDS ) )
      race->bodypart_where_names.push_back( "<used as light>     " );
   else
      race->bodypart_where_names.push_back( "<BODY PART SLOT ERROR>" );

   if( race->body_parts.test( PART_FINGERS ) )
   {
      race->bodypart_where_names.push_back( "<worn on finger>    " );
      race->bodypart_where_names.push_back( "<worn on finger>    " );
   }
   else
   {
      race->bodypart_where_names.push_back( "<BODY PART SLOT ERROR>" );
      race->bodypart_where_names.push_back( "<BODY PART SLOT ERROR>" );
   }

   race->bodypart_where_names.push_back( "<worn around neck>  " );
   race->bodypart_where_names.push_back( "<worn around neck>  " );
   race->bodypart_where_names.push_back( "<worn on body>      " );
   race->bodypart_where_names.push_back( "<worn on head>      " );

   if( race->body_parts.test( PART_LEGS ) )
      race->bodypart_where_names.push_back( "<worn on legs>      " );
   else
      race->bodypart_where_names.push_back( "<BODY PART SLOT ERROR>" );

   if( race->body_parts.test( PART_FEET ) )
      race->bodypart_where_names.push_back( "<worn on feet>      " );
   else
      race->bodypart_where_names.push_back( "<BODY PART SLOT ERROR>" );

   if( race->body_parts.test( PART_HANDS ) )
      race->bodypart_where_names.push_back( "<worn on hands>     " );
   else
      race->bodypart_where_names.push_back( "<BODY PART SLOT ERROR>" );

   if( race->body_parts.test( PART_ARMS ) )
      race->bodypart_where_names.push_back( "<worn on arms>      " );
   else
      race->bodypart_where_names.push_back( "<BODY PART SLOT ERROR>" );

   // Shields could potentially be on arms, legs, whatever. Don't limit to just hands.
   race->bodypart_where_names.push_back( "<worn as shield>    " );

   race->bodypart_where_names.push_back( "<worn about body>   " );
   race->bodypart_where_names.push_back( "<worn about waist>  " );

   if( race->body_parts.test( PART_HANDS ) )
   {
      race->bodypart_where_names.push_back( "<worn around wrist> " );
      race->bodypart_where_names.push_back( "<worn around wrist> " );
      race->bodypart_where_names.push_back( "<wielded>           " );
      race->bodypart_where_names.push_back( "<held>              " );
      race->bodypart_where_names.push_back( "<dual wielded>      " );
   }
   else
   {
      race->bodypart_where_names.push_back( "<BODY PART SLOT ERROR>" );
      race->bodypart_where_names.push_back( "<BODY PART SLOT ERROR>" );
      race->bodypart_where_names.push_back( "<BODY PART SLOT ERROR>" );
      race->bodypart_where_names.push_back( "<BODY PART SLOT ERROR>" );
      race->bodypart_where_names.push_back( "<BODY PART SLOT ERROR>" );
   }

   if( race->body_parts.test( PART_EAR ) )
      race->bodypart_where_names.push_back( "<worn on ears>      " );
   else
      race->bodypart_where_names.push_back( "<BODY PART SLOT ERROR>" );

   if( race->body_parts.test( PART_EYE ) )
      race->bodypart_where_names.push_back( "<worn over eyes>    " );
   else
      race->bodypart_where_names.push_back( "<BODY PART SLOT ERROR>" );

   if( race->body_parts.test( PART_HANDS ) )
      race->bodypart_where_names.push_back( "<missile wielded>   " );
   else
      race->bodypart_where_names.push_back( "<BODY PART SLOT ERROR>" );

   race->bodypart_where_names.push_back( "<worn on back>      " );
   race->bodypart_where_names.push_back( "<worn on face>      " );

   if( race->body_parts.test( PART_ANKLES ) )
   {
      race->bodypart_where_names.push_back( "<worn on ankle>     " );
      race->bodypart_where_names.push_back( "<worn on ankle>     " );
   }
   else
   {
      race->bodypart_where_names.push_back( "<BODY PART SLOT ERROR>" );
      race->bodypart_where_names.push_back( "<BODY PART SLOT ERROR>" );
   }

   if( race->body_parts.test( PART_HOOVES ) )
      race->bodypart_where_names.push_back( "<worn on hooves>    " );
   else
      race->bodypart_where_names.push_back( "<BODY PART SLOT ERROR>" );

   if( race->body_parts.test( PART_TAIL ) || race->body_parts.test( PART_TAILATTACK ) )
      race->bodypart_where_names.push_back( "<worn on tail>      " );
   else
      race->bodypart_where_names.push_back( "<BODY PART SLOT ERROR>" );

   race->bodypart_where_names.push_back( "<lodged in a rib>   " );

   if( race->body_parts.test( PART_ARMS ) )
      race->bodypart_where_names.push_back( "<lodged in an arm>  " );
   else
      race->bodypart_where_names.push_back( "<BODY PART SLOT ERROR>" );

   if( race->body_parts.test( PART_LEGS ) )
      race->bodypart_where_names.push_back( "<lodged in a leg>   " );
   else
      race->bodypart_where_names.push_back( "<BODY PART SLOT ERROR>" );
}

bool load_race_file( std::string_view fname )
{
   race_type *race;
   int ra = -1, file_ver = 0;
   FILE *fp;

   std::filesystem::path buf = std::format( "{}{}", RACE_DIR, fname );
   if( !( fp = fopen( buf.c_str(), "r" ) ) )
   {
      bug( "{}: Unable to open {} for reading!", __func__, buf.c_str() );
      return false;
   }

   race = new race_type;
   race->minalign = -1000;
   race->maxalign = 1000;

   for( ;; )
   {
      std::string word = ( feof( fp ) ? "End" : fread_word( fp ) );

      if( word[0] == '\0' )
      {
         log_printf( "{}: EOF encountered reading file!", __func__ );
         word = "End";
      }

      switch ( to_upper( word[0] ) )
      {
         default:
            log_printf( "%s: no match: %s", __func__, word );
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

               set_bodypart_where_names( race );
               break;
            }
            break;

         case 'C':
            KEY( "Con_Plus", race->con_plus, fread_number( fp ) );
            KEY( "Cha_Plus", race->cha_plus, fread_number( fp ) );
            if( !str_cmp( word, "Classes" ) )
            {
               if( file_ver < 1 )
                  race->allowed_classes = fread_number( fp );
               else
                  flag_set( fp, race->allowed_classes, npc_class );
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
                  bug( "{}: Race ({}) bad/not found ({})", __func__, !race->race_name.empty() ? race->race_name : "name not found", ra );
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
            STDSKEY( "Name", race->race_name );
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
                  bug( "{}: Skill {} -- race bad/not found ({})", __func__, word, ra );
               else if( !IS_VALID_SN( sn ) )
               {
                  bug( "{}: skill {} = SN {}", __func__, word, sn );
                  log_printf( "Skill '{}' unknown", word );
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

            // WhereNames no longer stored in race files. Ignore them if they're found.
            if( !str_cmp( word, "WhereName" ) )
            {
                  fread_flagstring( fp );
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

   MAX_PC_RACE = 0;
   /*
    * Pre-init the race_table with blank races
    */
   for( int i = 0; i < MAX_RACE; ++i )
      race_table[i] = nullptr;

   std::filesystem::path racelist = std::format( "{}{}", RACE_DIR, RACE_LIST );
   if( !( fpList = fopen( racelist.c_str(), "r" ) ) )
   {
      bug( "{}: Unable to open race list file!", __func__ );
      std::exit( EXIT_FAILURE );
   }

   for( ;; )
   {
      std::string filename = ( feof( fpList ) ? "$" : fread_word( fpList ) );

      if( filename.empty() )
      {
         log_printf( "{}: EOF encountered reading file!", __func__ );
         break;
      }

      if( filename[0] == '$' )
         break;

      if( !load_race_file( filename ) )
         bug( "{}: Cannot load race file: {}", __func__, filename );
      else
         ++MAX_PC_RACE;
   }

   for( int i = 0; i < MAX_RACE; ++i )
   {
      if( !race_table[i] )
      {
         race_type *race = new race_type;

         race->race_name = "Unused Race";
         race_table[i] = race;
      }
   }
   FCLOSE( fpList );
}

// 1: Initial version.
constexpr int RACEFILEVER = 1;

void write_race_file( int ra )
{
   FILE *fpout;
   race_type *race = race_table[ra];

   if( race->race_name.empty() )
   {
      log_printf( "Race {} has null name, not writing .race file.", ra );
      return;
   }

   std::filesystem::path filename = std::format( "{}{}.race", RACE_DIR, race->race_name );
   if( !( fpout = fopen( filename.c_str(), "w" ) ) )
   {
      bug( "Cannot open: {} for writing", filename.string() );
      return;
   }
   fprintf( fpout, "Version     %d\n", RACEFILEVER );
   fprintf( fpout, "Name        %s~\n", race->race_name.c_str() );
   fprintf( fpout, "Race        %d\n", ra );
   fprintf( fpout, "Classes     %s~\n", bitset_string( race->allowed_classes, npc_class ) );
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
   fprintf( fpout, "Align      %d\n", race->alignment );
   fprintf( fpout, "Min_Align  %d\n", race->minalign );
   fprintf( fpout, "Max_Align  %d\n", race->maxalign );
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

   for( int x = 0; x < num_skills; ++x )
   {
      int y;

      if( skill_table[x]->name.empty() )
         break;
      if( ( y = skill_table[x]->race_level[ra] ) < LEVEL_IMMORTAL )
         fprintf( fpout, "Skill '%s' %d %d\n", skill_table[x]->name.c_str(), y, skill_table[x]->race_adept[ra] );
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

   std::filesystem::path racelist = std::format( "{}{}", RACE_DIR, RACE_LIST );
   if( !( fpList = fopen( racelist.c_str(), "w" ) ) )
   {
      bug( "{}: Error opening racelist.", __func__ );
      return;
   }
   for( int i = 0; i < MAX_PC_RACE; ++i )
      fprintf( fpList, "%s.race\n", race_table[i]->race_name.c_str() );
   fprintf( fpList, "%s", "$\n" );
   FCLOSE( fpList );
}

/*
 * Create an instance of a new race. - Shaddai
 */
bool create_new_race( int race, const std::string & argument )
{
   if( race >= MAX_RACE || race_table[race] == nullptr )
      return false;

   race_table[race]->race_name = capitalize( argument );
   race_table[race]->allowed_classes.reset(  );
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
   race_table[race]->bodypart_where_names.clear();
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
   std::string arg1, arg2, arg3;
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
      race = nullptr;
      for( ra = 0; ra < MAX_RACE && race_table[ra]; ++ra )
      {
         if( race_table[ra]->race_name.empty() )
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
      if( MAX_PC_RACE >= MAX_RACE )
      {
         ch->print( "You need to up MAX_RACE in mud.h and make clean.\r\n" );
         return;
      }

      std::filesystem::path filename = std::format( "{}.race", arg1 );
      if( !is_valid_filename( ch, RACE_DIR, filename.string() ) )
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
      race_type *rcheck = nullptr;

      one_argument( argument, arg1 );

      if( arg1.empty(  ) )
      {
         ch->print( "You can't set a race name to nothing.\r\n" );
         return;
      }

      std::filesystem::path filename = std::format( "{}.race", arg1 );
      if( !is_valid_filename( ch, RACE_DIR, filename.string() ) )
         return;

      for( i = 0; i < MAX_PC_RACE && race_table[i]; ++i )
      {
         if( race_table[i]->race_name.empty() )
            continue;

         if( !str_cmp( race_table[i]->race_name, arg1 ) )
         {
            rcheck = race_table[i];
            break;
         }
      }
      if( rcheck != nullptr )
      {
         ch->print_fmt( "Already a race called {}.\r\n", arg1 );
         return;
      }

      filename = std::format( "{}{}.race", RACE_DIR, race->race_name );
      std::filesystem::remove( filename );

      race->race_name = capitalize( argument );

      write_race_file( ra );
      write_race_list(  );
      ch->print( "Race name set.\r\n" );
      return;
   }

   if( !str_cmp( arg2, "strplus" ) )
   {
      race->str_plus = static_cast<short>( std::stoi( argument ) );
      write_race_file( ra );
      ch->print( "Done.\r\n" );
      return;
   }
   if( !str_cmp( arg2, "dexplus" ) )
   {
      race->dex_plus = static_cast<short>( std::stoi( argument ) );
      write_race_file( ra );
      ch->print( "Done.\r\n" );
      return;
   }
   if( !str_cmp( arg2, "wisplus" ) )
   {
      race->wis_plus = static_cast<short>( std::stoi( argument ) );
      write_race_file( ra );
      ch->print( "Done.\r\n" );
      return;
   }
   if( !str_cmp( arg2, "intplus" ) )
   {
      race->int_plus = static_cast<short>( std::stoi( argument ) );
      write_race_file( ra );
      ch->print( "Done.\r\n" );
      return;
   }
   if( !str_cmp( arg2, "conplus" ) )
   {
      race->con_plus = static_cast<short>( std::stoi( argument ) );
      write_race_file( ra );
      ch->print( "Done.\r\n" );
      return;
   }
   if( !str_cmp( arg2, "chaplus" ) )
   {
      race->cha_plus = static_cast<short>( std::stoi( argument ) );
      write_race_file( ra );
      ch->print( "Done.\r\n" );
      return;
   }
   if( !str_cmp( arg2, "lckplus" ) )
   {
      race->lck_plus = static_cast<short>( std::stoi( argument ) );
      write_race_file( ra );
      ch->print( "Done.\r\n" );
      return;
   }
   if( !str_cmp( arg2, "hit" ) )
   {
      race->hit = static_cast<short>( std::stoi( argument ) );
      write_race_file( ra );
      ch->print( "Done.\r\n" );
      return;
   }
   if( !str_cmp( arg2, "mana" ) )
   {
      race->mana = static_cast<short>( std::stoi( argument ) );
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
            ch->print_fmt( "Unknown flag: {}\r\n", arg3 );
         else
            race->affected.flip( value );
      }
      write_race_file( ra );
      ch->print( "Racial affects set.\r\n" );
      return;
   }

   if( !str_cmp( arg2, "bodyparts" ) || !str_cmp( arg2, "part" ) )
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
            ch->print_fmt( "Unknown part: {}\r\n", arg3 );
         else
            race->body_parts.flip( value );
      }
      set_bodypart_where_names( race );
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
            ch->print_fmt( "Unknown flag: {}\r\n", arg3 );
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
            ch->print_fmt( "Unknown flag: {}\r\n", arg3 );
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
         ch->print_fmt( "Unknown language: {}\r\n", arg3 );
         return;
      }
      else if( !( value &= VALID_LANGS ) )
      {
         ch->print_fmt( "Player races may not speak {}.\r\n", arg3 );
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
            race->allowed_classes.flip( i );  /* k, that's boggling */
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
      race->ac_plus = std::stoi( argument );
      write_race_file( ra );
      ch->print( "Done.\r\n" );
      return;
   }

   if( !str_cmp( arg2, "alignment" ) )
   {
      race->alignment = std::stoi( argument );
      write_race_file( ra );
      ch->print( "Done.\r\n" );
      return;
   }

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
            ch->print_fmt( "Unknown flag: {}\r\n", arg3 );
         else
            race->defenses.flip( value );
      }
      write_race_file( ra );
      return;
   }

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
            ch->print_fmt( "Unknown flag: {}\r\n", arg3 );
         else
            race->attacks.flip( value );
      }
      write_race_file( ra );
      return;
   }

   if( !str_cmp( arg2, "minalign" ) )
   {
      race->minalign = std::stoi( argument  );
      write_race_file( ra );
      ch->print( "Done.\r\n" );
      return;
   }
   if( !str_cmp( arg2, "maxalign" ) )
   {
      race->maxalign = std::stoi( argument );
      write_race_file( ra );
      ch->print( "Done.\r\n" );
      return;
   }
   if( !str_cmp( arg2, "height" ) )
   {
      race->height = std::stoi( argument );
      write_race_file( ra );
      ch->print( "Done.\r\n" );
      return;
   }
   if( !str_cmp( arg2, "weight" ) )
   {
      race->weight = std::stoi( argument );
      write_race_file( ra );
      ch->print( "Done.\r\n" );
      return;
   }
   if( !str_cmp( arg2, "thirstmod" ) )
   {
      race->thirst_mod = std::stoi( argument );
      write_race_file( ra );
      ch->print( "Done.\r\n" );
      return;
   }
   if( !str_cmp( arg2, "hungermod" ) )
   {
      race->hunger_mod = std::stoi( argument );
      write_race_file( ra );
      ch->print( "Done.\r\n" );
      return;
   }
   if( !str_cmp( arg2, "maxalign" ) )
   {
      race->maxalign = std::stoi( argument );
      write_race_file( ra );
      ch->print( "Done.\r\n" );
      return;
   }
   if( !str_cmp( arg2, "expmultiplier" ) )
   {
      race->exp_multiplier = std::stoi( argument );
      write_race_file( ra );
      ch->print( "Done.\r\n" );
      return;
   }
   if( !str_cmp( arg2, "saving_poison_death" ) )
   {
      race->saving_poison_death = std::stoi( argument );
      write_race_file( ra );
      ch->print( "Done.\r\n" );
      return;
   }
   if( !str_cmp( arg2, "saving_wand" ) )
   {
      race->saving_wand = std::stoi( argument );
      write_race_file( ra );
      ch->print( "Done.\r\n" );
      return;
   }
   if( !str_cmp( arg2, "saving_para_petri" ) )
   {
      race->saving_para_petri = std::stoi( argument );
      write_race_file( ra );
      ch->print( "Done.\r\n" );
      return;
   }
   if( !str_cmp( arg2, "saving_breath" ) )
   {
      race->saving_breath = std::stoi( argument );
      write_race_file( ra );
      ch->print( "Done.\r\n" );
      return;
   }
   if( !str_cmp( arg2, "saving_spell_staff" ) )
   {
      race->saving_spell_staff = std::stoi( argument );
      write_race_file( ra );
      ch->print( "Done.\r\n" );
      return;
   }

   if( !str_cmp( arg2, "mana_regen" ) )
   {
      race->mana_regen = std::stoi( argument );
      write_race_file( ra );
      ch->print( "Done.\r\n" );
      return;
   }
   if( !str_cmp( arg2, "hp_regen" ) )
   {
      race->hp_regen = std::stoi( argument );
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
         ch->pager_fmt( "{:2}> {:<11}", i, race_table[i]->race_name );
         if( ct % 5 == 0 )
            ch->pager( "\r\n" );
      }
      ch->pager( "\r\n" );
      return;
   }
   if( is_number( argument ) && ( ra = std::stoi( argument ) ) >= 0 && ra < MAX_RACE )
      race = race_table[ra];
   else
   {
      race = nullptr;
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

   ch->pager_fmt( "RACE: {}\r\n", race->race_name );

   ct = 0;
   ch->pager( "Allowed Classes: " );
   for( i = 0; i < MAX_CLASS; ++i )
   {
      if( race->allowed_classes.test( i ) )
      {
         ++ct;
         ch->pager_fmt( "{} ", class_table[i]->who_name );
         if( ct % 6 == 0 )
            ch->pager( "\r\n" );
      }
   }
   if( ( ct % 6 != 0 ) || ( ct == 0 ) )
      ch->pager( "\r\n" );

   ch->pager_fmt( "Str Plus: {:<3}\tDex Plus: {:<3}\tWis Plus: {:<3}\tInt Plus: {:<3}\t\r\n", race->str_plus, race->dex_plus, race->wis_plus, race->int_plus );

   ch->pager_fmt( "Con Plus: {:<3}\tCha Plus: {:<3}\tLck Plus: {:<3}\r\n", race->con_plus, race->cha_plus, race->lck_plus );

   ch->pager_fmt( "Hit Pts:  {:<3}\tMana: {:<3}\tAlign: {:<4}\tAC: {}\r\n", race->hit, race->mana, race->alignment, race->ac_plus );

   ch->pager_fmt( "Min Align: {}\tMax Align: {}\t\tXP Mult: {}%\r\n", race->minalign, race->maxalign, race->exp_multiplier );

   ch->pager_fmt( "Height: {:3} in.\t\tWeight: {:3} lbs.\tHungerMod: {}\tThirstMod: {}\r\n", race->height, race->weight, race->hunger_mod, race->thirst_mod );

   ch->pager_fmt( "Body Parts: {}\r\n", bitset_string( race->body_parts, part_flags ) );
   ch->pager_fmt( "Spoken Languages: {}\r\n", bitset_string( race->language, lang_names ) );
   ch->pager_fmt( "Affected by: {}\r\n", bitset_string( race->affected, aff_flags ) );
   ch->pager_fmt( "Resistant to: {}\r\n", bitset_string( race->resist, ris_flags ) );
   ch->pager_fmt( "Susceptible to: {}\r\n", bitset_string( race->suscept, ris_flags ) );

   ch->pager_fmt( "Saves: (P/D) {} (W) {} (P/P) {} (B) {} (S/S) {}\r\n",
               race->saving_poison_death, race->saving_wand, race->saving_para_petri, race->saving_breath, race->saving_spell_staff );

   ch->pager_fmt( "Innate Attacks: {}\r\n", bitset_string( race->attacks, attack_flags ) );
   ch->pager_fmt( "Innate Defenses: {}\r\n", bitset_string( race->defenses, defense_flags ) );
}

/* Simple, small way to make keeping track of small mods easier - Blod */
CMDF( do_fixed )
{
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
      FILE *fp = fopen( FIXED_FILE.data(), "w" );
      if( fp )
      {
         FCLOSE( fp );
      }
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
      std::string t = std::format( "{:%a %b %d, %Y %I:%M:%S %p}", current_time );
      append_to_file( FIXED_FILE, "&g|&G{} &g| &G{:5}&g|  {}:  &G{}", t, ch->in_room ? ch->in_room->vnum : 0, ch->isnpc(  ) ? ch->short_descr : ch->name, argument );
      ch->print( "Thanks, your modification has been logged.\r\n" );
   }
}

/*Added by Adjani, 12-23-02. Plan to add more to this, but for now it works. */
CMDF( do_forgefind )
{
   ch->pager( "\r\n" );

   bool found = false;
   for( auto* smith : charlist )
   {
      if( smith->has_actflag( ACT_SMITH ) || smith->has_actflag( ACT_GUILDFORGE ) )
      {
         found = true;
         ch->pager_fmt( "Forge: *{:5}* {:<30} at *{:5}*, {}.\r\n", smith->pIndexData->vnum, smith->short_descr, smith->in_room->vnum, smith->in_room->name );
      }
   }
   if( !found )
      ch->print( "You didn't find any forges.\r\n" );
}
