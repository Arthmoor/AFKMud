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
 *                   Main structure manipulation module                     *
 ****************************************************************************/

#include "mud.h"
#include "descriptor.h"
#include "deity.h"
#include "fight.h"
#include "mobindex.h"
#include "mud_prog.h"
#include "objindex.h"
#include "polymorph.h"
#include "roomindex.h"

extern int cur_qobjs;
extern int cur_qchars;

ch_ret global_retcode;
int falling;

struct extracted_char_data
{
   extracted_char_data *next;
   char_data *ch;
   room_index *room;
   ch_ret retcode;
   bool extract;
};

struct extracted_obj_data
{
   extracted_obj_data *next;
   obj_data *obj;
};

extracted_obj_data *extracted_obj_queue;
extracted_char_data *extracted_char_queue;

void init_memory( void *start, void *end, unsigned int size )
{
   memset( start, 0, ( int )( ( char * )end + size - ( char * )start ) );
}

int umin( int check, int ncheck )
{
   if( check < ncheck )
      return check;
   return ncheck;
}

int umax( int check, int ncheck )
{
   if( check > ncheck )
      return check;
   return ncheck;
}

int urange( int mincheck, int check, int maxcheck )
{
   if( check < mincheck )
      return mincheck;
   if( check > maxcheck )
      return maxcheck;
   return check;
}

/* - Thoric
 * Return how much experience is required for ch to get to a certain level
 */
long exp_level( int level )
{
   long x, y = 0;

   for( x = 1; x < level; ++x )
      y += ( x ) * ( x * 625 );

   return y;
}

/*
 * Pick off one argument from a string and return the rest.
 * Understands quotes.  Delimiters = { ' ', '-' }
 * No longer mangles case either. That used to be annoying.
 *
 * Rewritten by Xorith
 */
string one_argument2( string arg, string & newArg )
{
   string retValue = "";

   // Check to see if we start with a double quote
   if( arg[0] == '"' )
   {
      string::size_type nextIndex = arg.find( '"', 1 );
      if( nextIndex != string::npos )
      {
         newArg = arg.substr( 1, nextIndex - 1 );
         // make sure we strip spaces from the return value 
         int x = 1;
         while( arg[nextIndex + x] == ' ' )
            ++x;
         retValue = arg.substr( nextIndex + x );
         return retValue;
      }
   }

   // Check for single-quote
   if( arg[0] == '\'' )
   {
      string::size_type nextIndex = arg.find( '\'', 1 );
      if( nextIndex != string::npos )
      {
         newArg = arg.substr( 1, nextIndex - 1 );
         // make sure we strip spaces from the return value 
         int x = 1;
         while( arg[nextIndex + x] == ' ' )
            ++x;
         retValue = arg.substr( nextIndex + x );
         return retValue;
      }
   }

   // See which is closest - the next whitespace or hyphen
   string::size_type nextHyphenIndex = arg.find( '-', 0 );
   string::size_type nextSpaceIndex = arg.find( ' ', 0 );

   if( nextHyphenIndex != string::npos )
   {
      if( nextSpaceIndex == string::npos || ( nextHyphenIndex < nextSpaceIndex && ( nextHyphenIndex + 1 != nextSpaceIndex ) ) )
      {
         newArg = arg.substr( 0, nextHyphenIndex );
         retValue = arg.substr( nextHyphenIndex + 1 );
         return retValue;
      }
   }

   if( nextSpaceIndex != string::npos )
   {
      newArg = arg.substr( 0, nextSpaceIndex );
      // make sure we strip spaces from the return value 
      int x = 1;
      while( arg[nextSpaceIndex + x] == ' ' )
         ++x;
      retValue = arg.substr( nextSpaceIndex + 1 );
      return retValue;
   }

   // If we're here, we don't have any spaces, hyphens, quotes, ect...
   // Send what we do have back with newArg and return empty.
   newArg = arg;
   return retValue;
}

char *one_argument2( char *argument, char *arg_first )
{
   char cEnd;
   short count;

   count = 0;

   if( !argument || argument[0] == '\0' )
   {
      arg_first[0] = '\0';
      return argument;
   }

   while( isspace( *argument ) )
      ++argument;

   cEnd = ' ';
   if( *argument == '\'' || *argument == '"' )
      cEnd = *argument++;

   while( *argument != '\0' || ++count >= 255 )
   {
      if( *argument == cEnd || *argument == '-' )
      {
         ++argument;
         break;
      }
      *arg_first = ( *argument );
      ++arg_first;
      ++argument;
   }
   *arg_first = '\0';

   while( isspace( *argument ) )
      ++argument;

   return argument;
}

bool is_name2_prefix( const string & str, string namelist )
{
   string name;

   for( ;; )
   {
      namelist = one_argument2( namelist, name );
      if( name.empty(  ) )
         return false;
      if( !str_prefix( str, name ) )
         return true;
   }
}

bool nifty_is_name_prefix( string str, string namelist )
{
   string name;
   bool valid = false;

   if( str.empty(  ) || namelist.empty(  ) )
      return false;

   for( ;; )
   {
      str = one_argument2( str, name );
      if( !name.empty(  ) )
      {
         valid = true;

         if( !is_name2_prefix( name, namelist ) )
            return false;
      }
      if( str.empty(  ) )
         return valid;
   }
}

bool is_name2_prefix( const char *str, char *namelist )
{
   char name[MIL];

   for( ;; )
   {
      namelist = one_argument2( namelist, name );
      if( name[0] == '\0' )
         return false;
      if( !str_prefix( str, name ) )
         return true;
   }
}

/* Rewrote the 'nifty' functions since they mistakenly allowed for all objects
  to be selected by specifying an empty list like -, '', "", ', " etc,
  example: ofind -, c loc ''  - Luc 08/2000 */
bool nifty_is_name_prefix( char *str, char *namelist )
{
   char name[MIL];
   bool valid = false;

   if( !str || !str[0] || !namelist || !namelist[0] )
      return false;

   for( ;; )
   {
      str = one_argument2( str, name );
      if( *name )
      {
         valid = true;
         if( !is_name2_prefix( name, namelist ) )
            return false;
      }
      if( !*str )
         return valid;
   }
}

/*
 * Stick obj onto extraction queue
 */
void queue_extracted_obj( obj_data * obj )
{
   extracted_obj_data *ood;

   if( !obj )
   {
      bug( "%s: obj = NULL", __FUNCTION__ );
      return;
   }
   ood = new extracted_obj_data;
   ood->obj = obj;
   ood->next = extracted_obj_queue;
   extracted_obj_queue = ood;
   ++cur_qobjs;
}

/*
 * Add ch to the queue of recently extracted characters - Thoric
 */
void queue_extracted_char( char_data * ch, bool extract )
{
   extracted_char_data *ccd;

   if( !ch )
   {
      bug( "%s: ch = NULL", __FUNCTION__ );
      return;
   }
   ccd = new extracted_char_data;
   ccd->ch = ch;
   ccd->room = ch->in_room;
   ccd->extract = extract;
   ccd->retcode = rCHAR_DIED;
   ccd->next = extracted_char_queue;
   extracted_char_queue = ccd;
   ++cur_qchars;
}

/*
 * Check to see if ch died recently - Thoric
 */
bool char_data::char_died(  )
{
   extracted_char_data *ccd;

   for( ccd = extracted_char_queue; ccd; ccd = ccd->next )
      if( ccd->ch == this )
         return true;
   return false;
}

/*
 * Check the recently extracted object queue for obj - Thoric
 */
bool obj_data::extracted(  )
{
   extracted_obj_data *ood;

   for( ood = extracted_obj_queue; ood; ood = ood->next )
      if( this == ood->obj )
         return true;

   return false;
}

/*
 * Find an obj in a list.
 */
obj_data *get_obj_list( char_data * ch, const string & argument, list < obj_data * >source )
{
   string arg;
   list < obj_data * >::iterator iobj;

   int number = number_argument( argument, arg );
   int count = 0;
   for( iobj = source.begin(  ); iobj != source.end(  ); ++iobj )
   {
      obj_data *obj = *iobj;

      if( ch->can_see_obj( obj, false ) && hasname( obj->name, arg ) )
         if( ( count += obj->count ) >= number )
            return obj;
   }

   /*
    * If we didn't find an exact match, run through the list of objects
    * again looking for prefix matching, ie swo == sword.
    * Added by Narn, Sept/96
    */
   count = 0;
   for( iobj = source.begin(  ); iobj != source.end(  ); ++iobj )
   {
      obj_data *obj = *iobj;

      if( ch->can_see_obj( obj, false ) && nifty_is_name_prefix( arg, obj->name ) )
         if( ( count += obj->count ) >= number )
            return obj;
   }
   return NULL;
}

/*
 * How mental state could affect finding an object - Thoric
 * Used by get/drop/put/quaff/recite/etc
 * Increasingly freaky based on mental state and drunkeness
 */
bool ms_find_obj( char_data * ch )
{
   int ms = ch->mental_state;
   int drunk = ch->isnpc(  )? 0 : ch->pcdata->condition[COND_DRUNK];
   const char *t;

   /*
    * we're going to be nice and let nothing weird happen unless
    * you're a tad messed up
    */
   drunk = UMAX( 1, drunk );
   if( abs( ms ) + ( drunk / 3 ) < 30 )
      return false;
   if( ( number_percent(  ) + ( ms < 0 ? 15 : 5 ) ) > abs( ms ) / 2 + drunk / 4 )
      return false;
   if( ms > 15 )  /* range 1 to 20 -- feel free to add more */
      switch ( number_range( UMAX( 1, ( ms / 5 - 15 ) ), ( ms + 4 ) / 5 ) )
      {
         default:
         case 1:
            t = "As you reach for it, you forgot what it was...\r\n";
            break;
         case 2:
            t = "As you reach for it, something inside stops you...\r\n";
            break;
         case 3:
            t = "As you reach for it, it seems to move out of the way...\r\n";
            break;
         case 4:
            t = "You grab frantically for it, but can't seem to get a hold of it...\r\n";
            break;
         case 5:
            t = "It disappears as soon as you touch it!\r\n";
            break;
         case 6:
            t = "You would if it would stay still!\r\n";
            break;
         case 7:
            t = "Whoa!  It's covered in blood!  Ack!  Ick!\r\n";
            break;
         case 8:
            t = "Wow... trails!\r\n";
            break;
         case 9:
            t = "You reach for it, then notice the back of your hand is growing something!\r\n";
            break;
         case 10:
            t = "As you grasp it, it shatters into tiny shards which bite into your flesh!\r\n";
            break;
         case 11:
            t = "What about that huge dragon flying over your head?!?!?\r\n";
            break;
         case 12:
            t = "You stratch yourself instead...\r\n";
            break;
         case 13:
            t = "You hold the universe in the palm of your hand!\r\n";
            break;
         case 14:
            t = "You're too scared.\r\n";
            break;
         case 15:
            t = "Your mother smacks your hand... 'NO!'\r\n";
            break;
         case 16:
            t = "Your hand grasps the worst pile of revoltingness that you could ever imagine!\r\n";
            break;
         case 17:
            t = "You stop reaching for it as it screams out at you in pain!\r\n";
            break;
         case 18:
            t = "What about the millions of burrow-maggots feasting on your arm?!?!\r\n";
            break;
         case 19:
            t = "That doesn't matter anymore... you've found the true answer to everything!\r\n";
            break;
         case 20:
            t = "A supreme entity has no need for that.\r\n";
            break;
      }
   else
   {
      int sub = URANGE( 1, abs( ms ) / 2 + drunk, 60 );
      switch ( number_range( 1, sub / 10 ) )
      {
         default:
         case 1:
            t = "In just a second...\r\n";
            break;
         case 2:
            t = "You can't find that...\r\n";
            break;
         case 3:
            t = "It's just beyond your grasp...\r\n";
            break;
         case 4:
            t = "...but it's under a pile of other stuff...\r\n";
            break;
         case 5:
            t = "You go to reach for it, but pick your nose instead.\r\n";
            break;
         case 6:
            t = "Which one?!?  I see two... no three...\r\n";
            break;
      }
   }
   ch->print( t );
   return true;
}

/*
 * Generic get obj function that supports optional containers.	-Thoric
 * currently only used for "eat" and "quaff".
 */
obj_data *find_obj( char_data * ch, string argument, bool carryonly )
{
   string arg1, arg2;
   obj_data *obj = NULL;

   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );

   if( !str_cmp( arg2, "from" ) && !argument.empty(  ) )
      argument = one_argument( argument, arg2 );

   if( arg2.empty(  ) )
   {
      if( carryonly && !( obj = ch->get_obj_carry( arg1 ) ) )
      {
         ch->print( "You do not have that item.\r\n" );
         return NULL;
      }
      else if( !carryonly && !( obj = ch->get_obj_here( arg1 ) ) )
      {
         ch->printf( "I see no %s here.\r\n", arg1.c_str(  ) );
         return NULL;
      }
      return obj;
   }
   else
   {
      obj_data *container = NULL;

      if( carryonly && !( container = ch->get_obj_carry( arg2 ) ) && !( container = ch->get_obj_wear( arg2 ) ) )
      {
         ch->print( "You do not have that item.\r\n" );
         return NULL;
      }
      if( !carryonly && !( container = ch->get_obj_here( arg2 ) ) )
      {
         ch->printf( "I see no %s here.\r\n", arg2.c_str(  ) );
         return NULL;
      }
      if( !container->extra_flags.test( ITEM_COVERING ) && IS_SET( container->value[1], CONT_CLOSED ) )
      {
         ch->printf( "The %s is closed.\r\n", container->name );
         return NULL;
      }
      if( !( obj = get_obj_list( ch, arg1, container->contents ) ) )
         act( AT_PLAIN, container->extra_flags.test( ITEM_COVERING ) ?
              "I see nothing like that beneath $p." : "I see nothing like that in $p.", ch, container, NULL, TO_CHAR );
      return obj;
   }
}

/*
 * Set off a trap (obj) upon character (ch) - Thoric
 */
ch_ret spring_trap( char_data * ch, obj_data * obj )
{
   int dam, typ, lev;
   const char *txt;
   ch_ret retcode;

   typ = obj->value[1];
   lev = obj->value[2];

   retcode = rNONE;

   switch ( typ )
   {
      default:
         txt = "hit by a trap";
         break;
      case TRAP_TYPE_POISON_GAS:
         txt = "surrounded by a green cloud of gas";
         break;
      case TRAP_TYPE_POISON_DART:
         txt = "hit by a dart";
         break;
      case TRAP_TYPE_POISON_NEEDLE:
         txt = "pricked by a needle";
         break;
      case TRAP_TYPE_POISON_DAGGER:
         txt = "stabbed by a dagger";
         break;
      case TRAP_TYPE_POISON_ARROW:
         txt = "struck with an arrow";
         break;
      case TRAP_TYPE_BLINDNESS_GAS:
         txt = "surrounded by a red cloud of gas";
         break;
      case TRAP_TYPE_SLEEPING_GAS:
         txt = "surrounded by a yellow cloud of gas";
         break;
      case TRAP_TYPE_FLAME:
         txt = "struck by a burst of flame";
         break;
      case TRAP_TYPE_EXPLOSION:
         txt = "hit by an explosion";
         break;
      case TRAP_TYPE_ACID_SPRAY:
         txt = "covered by a spray of acid";
         break;
      case TRAP_TYPE_ELECTRIC_SHOCK:
         txt = "suddenly shocked";
         break;
      case TRAP_TYPE_BLADE:
         txt = "sliced by a razor sharp blade";
         break;
      case TRAP_TYPE_SEX_CHANGE:
         txt = "surrounded by a mysterious aura";
         break;
   }

   /* Values 4 and 5 had never been set anywhere before so make the damage count now.
    * Formula is 1d6 + (trap level) points of non-magical damage.
    * Only applies to trap types that use poison.
    */
   dam = number_range( obj->value[4], obj->value[5] );
   if( dam <= 0 )
      dam = number_range( 1, 6 ) + lev;
   
   act_printf( AT_HITME, ch, NULL, NULL, TO_CHAR, "You are %s!", txt );
   act_printf( AT_ACTION, ch, NULL, NULL, TO_ROOM, "$n is %s.", txt );
   --obj->value[0];
   if( obj->value[0] <= 0 )
      obj->extract(  );
   switch ( typ )
   {
      default:
      case TRAP_TYPE_BLADE:
         retcode = damage( ch, ch, dam, TYPE_UNDEFINED );
         break;

      case TRAP_TYPE_POISON_DART:
      case TRAP_TYPE_POISON_NEEDLE:
      case TRAP_TYPE_POISON_DAGGER:
      case TRAP_TYPE_POISON_ARROW:
         retcode = obj_cast_spell( gsn_poison, lev, ch, ch, NULL );
         if( retcode == rNONE )
            retcode = damage( ch, ch, dam, TYPE_UNDEFINED );
         break;

      case TRAP_TYPE_POISON_GAS:
         retcode = obj_cast_spell( gsn_poison, lev, ch, ch, NULL );
         break;

      case TRAP_TYPE_BLINDNESS_GAS:
         retcode = obj_cast_spell( gsn_blindness, lev, ch, ch, NULL );
         break;

      case TRAP_TYPE_SLEEPING_GAS:
         retcode = obj_cast_spell( skill_lookup( "sleep" ), lev, ch, ch, NULL );
         break;

      case TRAP_TYPE_ACID_SPRAY:
         retcode = obj_cast_spell( skill_lookup( "acid blast" ), lev, ch, ch, NULL );
         break;

      case TRAP_TYPE_SEX_CHANGE:
         retcode = obj_cast_spell( skill_lookup( "change sex" ), lev, ch, ch, NULL );
         break;

      case TRAP_TYPE_FLAME:
         retcode = obj_cast_spell( skill_lookup( "flamestrike" ), lev, ch, ch, NULL );
         break;

      case TRAP_TYPE_EXPLOSION:
         retcode = obj_cast_spell( gsn_fireball, lev, ch, ch, NULL );
         break;

      case TRAP_TYPE_ELECTRIC_SHOCK:
         retcode = obj_cast_spell( skill_lookup( "lightning bolt" ), lev, ch, ch, NULL );
         break;
   }
   return retcode;
}

/*
 * Check an object for a trap - Thoric
 */
ch_ret check_for_trap( char_data * ch, obj_data * obj, int flag )
{
   if( obj->contents.empty(  ) )
      return rNONE;

   ch_ret retcode = rNONE;
   list < obj_data * >::iterator iobj;
   for( iobj = obj->contents.begin(  ); iobj != obj->contents.end(  ); ++iobj )
   {
      obj_data *check = *iobj;

      if( check->item_type == ITEM_TRAP && IS_SET( check->value[3], flag ) )
      {
         retcode = spring_trap( ch, check );
         if( retcode != rNONE )
            return retcode;
      }
   }
   return retcode;
}

/*
 * Check the room for a trap - Thoric
 */
ch_ret check_room_for_traps( char_data * ch, int flag )
{
   if( !ch )
      return rERROR;
   if( !ch->in_room || ch->in_room->objects.empty(  ) )
      return rNONE;

   ch_ret retcode = rNONE;
   list < obj_data * >::iterator iobj;
   for( iobj = ch->in_room->objects.begin(  ); iobj != ch->in_room->objects.end(  ); ++iobj )
   {
      obj_data *check = *iobj;
      if( check->item_type == ITEM_TRAP && IS_SET( check->value[3], flag ) )
      {
         retcode = spring_trap( ch, check );
         if( retcode != rNONE )
            return retcode;
      }
   }
   return retcode;
}

/*
 * Clean out the extracted object queue
 */
void clean_obj_queue( void )
{
   extracted_obj_data *ood;

   for( ood = extracted_obj_queue; ood; ood = extracted_obj_queue )
   {
      extracted_obj_queue = ood->next;

      objlist.remove( ood->obj );
      deleteptr( ood->obj );
      deleteptr( ood );
      --cur_qobjs;
   }
}

/*
 * clean out the extracted character queue
 */
void clean_char_queue( void )
{
   extracted_char_data *ccd;

   for( ccd = extracted_char_queue; ccd; ccd = extracted_char_queue )
   {
      extracted_char_queue = ccd->next;
      if( ccd->extract )
      {
         charlist.remove( ccd->ch );
         if( !ccd->ch->isnpc(  ) )
            pclist.remove( ccd->ch );
         deleteptr( ccd->ch );
      }
      deleteptr( ccd );
      --cur_qchars;
   }
}
