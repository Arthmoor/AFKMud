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
 *                        Tracking/hunting module                           *
 ****************************************************************************/

#include "mud.h"
#include "fight.h"
#include "roomindex.h"

const int BFS_ERROR = -1;
const int BFS_ALREADY_THERE = -2;
const int BFS_NO_PATH = -3;
const int BFS_MARK = ROOM_TRACK;

#define TRACK_THROUGH_DOORS

void start_hunting( char_data *, char_data * );
void set_fighting( char_data *, char_data * );
bool mob_fire( char_data *, const string & );
char_data *scan_for_vic( char_data *, exit_data *, const string & );
void stop_hating( char_data * );
void stop_hunting( char_data * );
void stop_fearing( char_data * );
int IsHumanoid( char_data * );

/* You can define or not define TRACK_THOUGH_DOORS, above, depending on
 whether or not you want track to find paths which lead through closed
 or hidden doors.
 */
struct bfs_data
{
   room_index *room;
   char dir;
   bfs_data *next;
};

static bfs_data *queue_head = NULL, *queue_tail = NULL, *room_queue = NULL;

/* Utility macros */
#define MARK(room)	( (room)->flags.set( BFS_MARK ) )
#define UNMARK(room)	( (room)->flags.reset( BFS_MARK ) )
#define IS_MARKED(room)	( (room)->flags.test( BFS_MARK ) )

bool valid_edge( exit_data * pexit )
{
   if( pexit->to_room && !IS_MARKED( pexit->to_room )
#ifndef TRACK_THROUGH_DOORS
       && !IS_EXIT_FLAG( pexit, EX_CLOSED )
#endif
       )
      return true;
   else
      return false;
}

void bfs_enqueue( room_index * room, char dir )
{
   bfs_data *curr;

   curr = new bfs_data;
   curr->room = room;
   curr->dir = dir;
   curr->next = NULL;

   if( queue_tail )
   {
      queue_tail->next = curr;
      queue_tail = curr;
   }
   else
      queue_head = queue_tail = curr;
}

void bfs_dequeue( void )
{
   bfs_data *curr;

   curr = queue_head;

   if( !( queue_head = queue_head->next ) )
      queue_tail = NULL;
   deleteptr( curr );
}

void bfs_clear_queue( void )
{
   while( queue_head )
      bfs_dequeue(  );
}

void room_enqueue( room_index * room )
{
   bfs_data *curr;

   curr = new bfs_data;
   curr->room = room;
   curr->next = room_queue;

   room_queue = curr;
}

void clean_room_queue( void )
{
   bfs_data *curr, *curr_next;

   for( curr = room_queue; curr; curr = curr_next )
   {
      UNMARK( curr->room );
      curr_next = curr->next;
      deleteptr( curr );
   }
   room_queue = NULL;
}

int find_first_step( room_index * src, room_index * target, int maxdist )
{
   if( !src || !target )
   {
      bug( "%s: NULL source and target rooms passed to find_first_step!", __func__ );
      return BFS_ERROR;
   }

   if( src == target )
      return BFS_ALREADY_THERE;

   if( src->area != target->area )
      return BFS_NO_PATH;

   room_enqueue( src );
   MARK( src );

   /*
    * first, enqueue the first steps, saving which direction we're going. 
    */
   int curr_dir;
   list < exit_data * >::iterator iexit;
   for( iexit = src->exits.begin(  ); iexit != src->exits.end(  ); ++iexit )
   {
      exit_data *pexit = *iexit;

      if( valid_edge( pexit ) )
      {
         curr_dir = pexit->vdir;
         MARK( pexit->to_room );
         room_enqueue( pexit->to_room );
         bfs_enqueue( pexit->to_room, curr_dir );
      }
   }

   int count = 0;
   while( queue_head )
   {
      if( ++count > maxdist )
      {
         bfs_clear_queue(  );
         clean_room_queue(  );
         return BFS_NO_PATH;
      }
      if( queue_head->room == target )
      {
         curr_dir = queue_head->dir;
         bfs_clear_queue(  );
         clean_room_queue(  );
         return curr_dir;
      }
      else
      {
         for( iexit = queue_head->room->exits.begin(  ); iexit != queue_head->room->exits.end(  ); ++iexit )
         {
            exit_data *pexit = *iexit;

            if( valid_edge( pexit ) )
            {
               curr_dir = pexit->vdir;
               MARK( pexit->to_room );
               room_enqueue( pexit->to_room );
               bfs_enqueue( pexit->to_room, queue_head->dir );
            }
         }
         bfs_dequeue(  );
      }
   }
   clean_room_queue(  );
   return BFS_NO_PATH;
}

CMDF( do_track )
{
   if( !ch->isnpc(  ) && ch->pcdata->learned[gsn_track] <= 0 )
   {
      ch->print( "You do not know of this skill yet.\r\n" );
      return;
   }

   if( argument.empty(  ) )
   {
      ch->print( "Whom are you trying to track?\r\n" );
      return;
   }

   ch->WAIT_STATE( skill_table[gsn_track]->beats );

   bool found = false;
   char_data *vict = NULL;
   list < char_data * >::iterator ich;
   for( ich = charlist.begin(  ); ich != charlist.end(  ); ++ich )
   {
      vict = *ich;

      if( vict->isnpc(  ) && vict->in_room && hasname( vict->name, argument ) )
      {
         found = true;
         break;
      }
   }

   if( !found )
   {
      ch->print( "You can't find a trail to anyone like that.\r\n" );
      return;
   }

   if( vict->has_aflag( AFF_NOTRACK ) )
   {
      ch->print( "You can't find a trail to anyone like that.\r\n" );
      return;
   }
   if( ch->has_actflag( ACT_IMMORTAL ) || ch->has_actflag( ACT_PACIFIST ) )
   {
      ch->print( "You are too peaceful to track anyone.\r\n" );
      stop_hunting( ch );
      return;
   }
   int maxdist = ch->pcdata ? ch->pcdata->learned[gsn_track] : 10;

   if( ch->Class == CLASS_ROGUE )
      maxdist *= 3;
   switch ( ch->race )
   {
      default:
         break;
      case RACE_HIGH_ELF:
      case RACE_WILD_ELF:
         maxdist *= 2;
         break;
      case RACE_DEVIL:
      case RACE_DEMON:
      case RACE_GOD:
         maxdist = 30000;
         break;
   }

   if( ch->is_immortal(  ) )
      maxdist = 30000;

   if( maxdist <= 0 )
      return;

   int dir = find_first_step( ch->in_room, vict->in_room, maxdist );

   switch ( dir )
   {
      case BFS_ERROR:
         ch->print( "&RHmm... something seems to be wrong.\r\n" );
         break;
      case BFS_ALREADY_THERE:
         if( ch->hunting )
         {
            ch->print( "&RYou have found your prey!\r\n" );
            stop_hunting( ch );
         }
         else
            ch->print( "&RYou're already in the same room!\r\n" );
         break;
      case BFS_NO_PATH:
         if( ch->hunting )
            stop_hunting( ch );
         ch->print( "&RYou can't sense a trail from here.\r\n" );
         ch->learn_from_failure( gsn_track );
         break;
      default:
         if( ch->hunting && ch->hunting->who != vict )
            stop_hunting( ch );
         start_hunting( ch, vict );
         ch->printf( "&RYou sense a trail %s from here...\r\n", dir_name[dir] );
         break;
   }
}

void found_prey( char_data * ch, char_data * victim )
{
   char victname[MSL];

   if( !victim )
   {
      bug( "%s: null victim", __func__ );
      return;
   }

   if( victim->in_room == NULL )
   {
      bug( "%s: null victim->in_room: %s", __func__, victim->name );
      return;
   }

   mudstrlcpy( victname, victim->name, MSL );

   if( !ch->can_see( victim, false ) )
   {
      if( number_percent(  ) < 90 )
         return;
      if( IsHumanoid( ch ) )
      {
         switch ( number_bits( 3 ) )
         {
            case 0:
               cmdf( ch, "say Don't make me find you, %s!", victname );
               break;
            case 1:
               act( AT_ACTION, "$n sniffs around the room for $N.", ch, NULL, victim, TO_NOTVICT );
               act( AT_ACTION, "You sniff around the room for $N.", ch, NULL, victim, TO_CHAR );
               act( AT_ACTION, "$n sniffs around the room for you.", ch, NULL, victim, TO_VICT );
               interpret( ch, "say I can smell your blood!" );
               break;
            case 2:
               cmdf( ch, "yell I'm going to tear %s apart!", victname );
               break;
            case 3:
               interpret( ch, "say Just wait until I find you..." );
               break;
            default:
               break;
         }
      }
      return;
   }

   if( ch->in_room->flags.test( ROOM_SAFE ) )
   {
      if( number_percent(  ) < 90 )
         return;
      if( IsHumanoid( ch ) )
      {
         switch ( number_bits( 3 ) )
         {
            case 0:
               interpret( ch, "say C'mon out, you coward!" );
               cmdf( ch, "yell %s is a bloody coward!", victname );
               break;
            case 1:
               cmdf( ch, "say Let's take this outside, %s", victname );
               break;
            case 2:
               cmdf( ch, "yell %s is a yellow-bellied wimp!", victname );
               break;
            case 3:
               act( AT_ACTION, "$n takes a few swipes at $N.", ch, NULL, victim, TO_NOTVICT );
               act( AT_ACTION, "You try to take a few swipes $N.", ch, NULL, victim, TO_CHAR );
               act( AT_ACTION, "$n takes a few swipes at you.", ch, NULL, victim, TO_VICT );
               break;
            default:
               break;
         }
      }
      return;
   }

   if( IsHumanoid( ch ) && !ch->has_actflag( ACT_IMMORTAL ) )
   {
      switch ( number_bits( 2 ) )
      {
         case 0:
            cmdf( ch, "yell Your blood is mine, %s!", victname );
            break;
         case 1:
            cmdf( ch, "say Alas, we meet again, %s!", victname );
            break;
         case 2:
            cmdf( ch, "say What do you want on your tombstone, %s?", victname );
            break;
         case 3:
            act( AT_ACTION, "$n lunges at $N from out of nowhere!", ch, NULL, victim, TO_NOTVICT );
            act( AT_ACTION, "You lunge at $N catching $M off guard!", ch, NULL, victim, TO_CHAR );
            act( AT_ACTION, "$n lunges at you from out of nowhere!", ch, NULL, victim, TO_VICT );
            break;
         default:
            break;
      }
   }
   else
   {
      if( ch->has_actflag( ACT_IMMORTAL ) )  /*So peaceful mobiles won't fight or spam. Adjani, 06-19-03 */
      {
         ch->print( "You are too peaceful to fight.\r\n" );
         stop_hating( ch );
         stop_hunting( ch );
         stop_fearing( ch );
         ch->stop_fighting( true );
         return;
      }
   }
   stop_hunting( ch );
   set_fighting( ch, victim );
   multi_hit( ch, victim, TYPE_UNDEFINED );
}

void hunt_vic( char_data * ch )
{
   if( !ch || !ch->hunting || ch->position < POS_BERSERK )
      return;

   /*
    * make sure the char still exists 
    */
   bool found = false;
   list < char_data * >::iterator ich;
   for( ich = charlist.begin(  ); ich != charlist.end(  ); ++ich )
   {
      char_data *tmp = *ich;

      if( ch->hunting->who == tmp )
      {
         found = true;
         break;
      }
   }

   if( !found )
   {
      interpret( ch, "say Damn! I lost track of my quarry!!" );
      stop_hunting( ch );
      return;
   }

   if( ch->in_room == ch->hunting->who->in_room )
   {
      if( ch->fighting )
         return;
      found_prey( ch, ch->hunting->who );
      return;
   }

   short ret = find_first_step( ch->in_room, ch->hunting->who->in_room, 500 + ch->level * 25 );
   if( ret < 0 )
   {
      interpret( ch, "say Damn! I lost track of my quarry!" );
      stop_hunting( ch );
      return;
   }
   else
   {
      exit_data *pexit;

      if( !( pexit = ch->in_room->get_exit( ret ) ) )
      {
         bug( "%s: lost exit?", __func__ );
         return;
      }

      /*
       * Segment copied from update.c, why should a hunting mob get an automatic move into a room
       * it should otherwise be unable to occupy? Exception for sentinel mobs, you attack it, it
       * gets to hunt you down, and they'll ignore EX_NOMOB in this section too 
       */
      if( !ch->has_actflag( ACT_PROTOTYPE ) && pexit->to_room
          /*
           * &&   !IS_EXIT_FLAG( pexit, EX_CLOSED ) - Testing to see if mobs can open doors this way 
           */
          /*
           * Keep em from wandering through my walls, Marcus 
           */
          && !IS_EXIT_FLAG( pexit, EX_FORTIFIED )
          && !IS_EXIT_FLAG( pexit, EX_HEAVY )
          && !IS_EXIT_FLAG( pexit, EX_MEDIUM )
          && !IS_EXIT_FLAG( pexit, EX_LIGHT )
          && !IS_EXIT_FLAG( pexit, EX_CRUMBLING )
          && !pexit->to_room->flags.test( ROOM_NO_MOB )
          && !pexit->to_room->flags.test( ROOM_DEATH ) && ( !ch->has_actflag( ACT_STAY_AREA ) || pexit->to_room->area == ch->in_room->area ) )
      {
         if( pexit->to_room->sector_type == SECT_WATER_NOSWIM && !ch->has_aflag( AFF_AQUA_BREATH ) )
            return;

         if( pexit->to_room->sector_type == SECT_RIVER && !ch->has_aflag( AFF_AQUA_BREATH ) )
            return;

         if( IS_EXIT_FLAG( pexit, EX_CLOSED ) && !pexit->to_room->flags.test( ROOM_NO_MOB ) )
            cmdf( ch, "open %s", pexit->keyword );
         move_char( ch, pexit, 0, pexit->vdir, false );
      }

      /*
       * Crash bug fix by Shaddai 
       */
      if( ch->char_died(  ) )
         return;

      if( !ch->hunting )
      {
         if( !ch->in_room )
         {
            bug( "%s: no ch->in_room! Name: %s. Placing mob in limbo.", __func__, ch->name );
            if( !ch->to_room( get_room_index( ROOM_VNUM_LIMBO ) ) )
               log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __func__, __LINE__ );
            return;
         }
         interpret( ch, "say Damn! I lost track of my quarry!" );
         return;
      }
      if( ch->in_room == ch->hunting->who->in_room )
         found_prey( ch, ch->hunting->who );
      else
      {
         char_data *vch;

         /*
          * perform a ranged attack if possible 
          * Changed who to name as scan_for_vic expects the name and
          * * Not the char struct. --Shaddai
          */
         if( ( vch = scan_for_vic( ch, pexit, ch->hunting->name ) ) != NULL )
         {
            if( !mob_fire( ch, ch->hunting->who->name ) )
            {
               /*
                * ranged spell attacks go here 
                */
            }
         }
      }
   }
}
