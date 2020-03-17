/****************************************************************************
 *                   ^     +----- |  / ^     ^ |     | +-\                  *
 *                  / \    |      | /  |\   /| |     | |  \                 *
 *                 /   \   +---   |<   | \ / | |     | |  |                 *
 *                /-----\  |      | \  |  v  | |     | |  /                 *
 *               /       \ |      |  \ |     | +-----+ +-/                  *
 ****************************************************************************
 * AFKMud Copyright 1997-2019 by Roger Libiez (Samson),                     *
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
 *                           Object Support Functions                       *
 ****************************************************************************/

#include <limits.h>
#include "mud.h"
#include "mobindex.h"
#include "mud_prog.h"
#include "object.h"
#include "objindex.h"
#include "roomindex.h"

list < obj_data * >objlist;

extern list < rel_data * >relationlist;
extern int top_affect;

void clean_obj_queue(  );
void queue_extracted_obj( obj_data * );

/* Deallocates the memory used by a single object after it's been extracted. */
obj_data::~obj_data(  )
{
   list < affect_data * >::iterator paf;
   list < rel_data * >::iterator RQ;
   list < extra_descr_data * >::iterator ed;
   list < mprog_act_list * >::iterator pd;

   /*
    * remove affects 
    */
   for( paf = affects.begin(  ); paf != affects.end(  ); )
   {
      affect_data *aff = *paf;
      ++paf;

      affects.remove( aff );
      deleteptr( aff );
      --top_affect;
   }
   affects.clear(  );

   /*
    * remove extra descriptions 
    */
   for( ed = extradesc.begin(  ); ed != extradesc.end(  ); )
   {
      extra_descr_data *desc = *ed;
      ++ed;

      extradesc.remove( desc );
      deleteptr( desc );
      --top_ed;
   }
   extradesc.clear(  );

   for( pd = mpact.begin(  ); pd != mpact.end(  ); )
   {
      mprog_act_list *actp = *pd;
      ++pd;

      deleteptr( actp );
   }
   mpact.clear(  );

   for( RQ = relationlist.begin(  ); RQ != relationlist.end(  ); )
   {
      rel_data *RQueue = *RQ;
      ++RQ;

      if( RQueue->Type == relOSET_ON )
      {
         if( this == RQueue->Subject )
            ( ( char_data * ) RQueue->Actor )->pcdata->dest_buf = nullptr;
         else
            continue;
         relationlist.remove( RQueue );
         deleteptr( RQueue );
      }
   }
   STRFREE( name );
   STRFREE( objdesc );
   STRFREE( short_descr );
   STRFREE( action_desc );
   STRFREE( socket[0] );
   STRFREE( socket[1] );
   STRFREE( socket[2] );
   STRFREE( owner );
   STRFREE( seller );
   STRFREE( buyer );
}

obj_data::obj_data(  )
{
   init_memory( &in_obj, &mpscriptpos, sizeof( mpscriptpos ) );
}

void extract_all_objs(  )
{
   list < obj_data * >::iterator iobj;

   clean_obj_queue(  );
   for( iobj = objlist.begin(  ); iobj != objlist.end(  ); )
   {
      obj_data *object = *iobj;
      ++iobj;

      if( object->in_room )
         object->from_room(  );
      objlist.remove( object );
      deleteptr( object );
   }
}

/* Make objects in rooms that are nofloor fall - Scryn 1/23/96 */
void obj_data::fall( bool through )
{
   exit_data *pexit;
   room_index *to;
   static int fall_count;
   static bool is_falling; /* Stop loops from the call to obj_to_room()  -- Altrag */
   bool destroyobj;

   if( !in_room || is_falling )
      return;

   if( fall_count > 30 )
   {
      bug( "%s: object falling in loop more than 30 times", __func__ );
      extract(  );
      fall_count = 0;
      return;
   }

   if( in_room->flags.test( ROOM_NOFLOOR ) && CAN_GO( this, DIR_DOWN ) && !extra_flags.test( ITEM_MAGIC ) )
   {
      pexit = in_room->get_exit( DIR_DOWN );
      to = pexit->to_room;

      if( through )
         ++fall_count;
      else
         fall_count = 0;

      if( in_room == to )
      {
         bug( "%s: Object falling into same room, room %d", __func__, to->vnum );
         extract(  );
         return;
      }

      if( !in_room->people.empty(  ) )
      {
         act( AT_PLAIN, "$p falls far below...", ( *in_room->people.begin(  ) ), this, nullptr, TO_ROOM );
         act( AT_PLAIN, "$p falls far below...", ( *in_room->people.begin(  ) ), this, nullptr, TO_CHAR );
      }
      from_room(  );
      is_falling = true;
      to_room( to, nullptr );
      is_falling = false;

      if( !in_room->people.empty(  ) )
      {
         act( AT_PLAIN, "$p falls from above...", ( *in_room->people.begin(  ) ), this, nullptr, TO_ROOM );
         act( AT_PLAIN, "$p falls from above...", ( *in_room->people.begin(  ) ), this, nullptr, TO_CHAR );
      }

      if( !in_room->flags.test( ROOM_NOFLOOR ) && through )
      {
         /*
          * Hey, someone took physics I see.... 
          */
         /*
          * int dam = (int)9.81 * sqrt( fall_count * 2 / 9.81 ) * weight / 2; 
          */
         int dam = fall_count * weight / 2;

         /*
          * Damage players 
          */
         if( !in_room->people.empty(  ) && number_percent(  ) > 15 )
         {
            list < char_data * >::iterator ich;
            char_data *vch = nullptr;
            int chcnt = 0;

            for( ich = in_room->people.begin(  ); ich != in_room->people.end(  ); ++ich, ++chcnt )
               if( number_range( 0, chcnt ) == 0 )
                  vch = *ich;

            if( vch )
            {
               act( AT_WHITE, "$p falls on $n!", vch, this, nullptr, TO_ROOM );
               act( AT_WHITE, "$p falls on you!", vch, this, nullptr, TO_CHAR );

               if( vch->has_actflag( ACT_HARDHAT ) )
                  act( AT_WHITE, "$p bounces harmlessly off your head!", vch, this, nullptr, TO_CHAR );
               else
                  damage( vch, vch, dam * vch->level, TYPE_UNDEFINED );
            }
         }

         /*
          * Damage objects 
          */
         destroyobj = false;
         switch ( item_type )
         {
            case ITEM_WEAPON:
            case ITEM_MISSILE_WEAPON:
               if( ( value[6] - dam ) <= 0 )
                  destroyobj = true;
               else
                  value[6] -= dam;
               break;

            case ITEM_PROJECTILE:
               if( ( value[5] - dam ) <= 0 )
                  destroyobj = true;
               else
                  value[5] -= dam;
               break;

            case ITEM_ARMOR:
               if( ( value[0] - dam ) <= 0 )
                  destroyobj = true;
               else
                  value[0] -= dam;
               break;

            default:
               if( ( dam * 15 ) > get_resistance(  ) )
                  destroyobj = true;
               break;
         }
         if( destroyobj )
         {
            if( !in_room->people.empty(  ) )
            {
               act( AT_PLAIN, "$p is destroyed by the fall!", ( *in_room->people.begin(  ) ), this, nullptr, TO_ROOM );
               act( AT_PLAIN, "$p is destroyed by the fall!", ( *in_room->people.begin(  ) ), this, nullptr, TO_CHAR );
            }
            make_scraps(  );
         }
      }
      fall( true );
   }
}

/*
 * how resistant an object is to damage				-Thoric
 */
short obj_data::get_resistance(  )
{
   short resist;

   resist = number_fuzzy( sysdata->maximpact );

   /*
    * magical items are more resistant 
    */
   if( extra_flags.test( ITEM_MAGIC ) )
      resist += number_fuzzy( 12 );
   /*
    * metal objects are definately stronger 
    */
   if( extra_flags.test( ITEM_METAL ) )
      resist += number_fuzzy( 5 );
   /*
    * organic objects are most likely weaker 
    */
   if( extra_flags.test( ITEM_ORGANIC ) )
      resist -= number_fuzzy( 5 );
   /*
    * blessed objects should have a little bonus 
    */
   if( extra_flags.test( ITEM_BLESS ) )
      resist += number_fuzzy( 5 );
   /*
    * lets make store inventory pretty tough 
    */
   if( extra_flags.test( ITEM_INVENTORY ) )
      resist += 20;

   /*
    * okay... let's add some bonus/penalty for item level... 
    */
   resist += ( level / 10 ) - 2;

   /*
    * and lasty... take armor or weapon's condition into consideration 
    */
   if( item_type == ITEM_ARMOR || item_type == ITEM_WEAPON )
      resist += ( value[0] / 2 ) - 2;

   return URANGE( 10, resist, 99 );
}

const string obj_data::oshort(  )
{
   string desc = short_descr;

   if( count > 1 )
   {
      char buf[MSL];

      snprintf( buf, MSL, "%s (%d)", short_descr, count );
      desc = buf;
   }
   return desc;
}

const string obj_data::format_to_char( char_data * ch, bool fShort, int num )
{
   string buf;
   bool glowsee = false;

   /*
    * can see glowing invis items in the dark
    */
   if( extra_flags.test( ITEM_GLOW ) && extra_flags.test( ITEM_INVIS ) && !ch->has_aflag( AFF_TRUESIGHT ) && !ch->has_aflag( AFF_DETECT_INVIS ) )
      glowsee = true;

   if( extra_flags.test( ITEM_INVIS ) )
      buf.append( "(Invis) " );
   if( ( ch->has_aflag( AFF_DETECT_EVIL ) || ch->Class == CLASS_PALADIN ) && extra_flags.test( ITEM_EVIL ) )
      buf.append( "(Red Aura) " );
   if( ch->Class == CLASS_PALADIN && ( extra_flags.test( ITEM_ANTI_EVIL ) && !extra_flags.test( ITEM_ANTI_NEUTRAL ) && !extra_flags.test( ITEM_ANTI_GOOD ) ) )
      buf.append( "(Flaming Red) " );
   if( ch->Class == CLASS_PALADIN && ( !extra_flags.test( ITEM_ANTI_EVIL ) && extra_flags.test( ITEM_ANTI_NEUTRAL ) && !extra_flags.test( ITEM_ANTI_GOOD ) ) )
      buf.append( "(Flaming Grey) " );
   if( ch->Class == CLASS_PALADIN && ( !extra_flags.test( ITEM_ANTI_EVIL ) && !extra_flags.test( ITEM_ANTI_NEUTRAL ) && extra_flags.test( ITEM_ANTI_GOOD ) ) )
      buf.append( "(Flaming White) " );
   if( ch->Class == CLASS_PALADIN && ( extra_flags.test( ITEM_ANTI_EVIL ) && extra_flags.test( ITEM_ANTI_NEUTRAL ) && !extra_flags.test( ITEM_ANTI_GOOD ) ) )
      buf.append( "(Smouldering Red-Grey) " );
   if( ch->Class == CLASS_PALADIN && ( extra_flags.test( ITEM_ANTI_EVIL ) && !extra_flags.test( ITEM_ANTI_NEUTRAL ) && extra_flags.test( ITEM_ANTI_GOOD ) ) )
      buf.append( "(Smouldering Red-White) " );
   if( ch->Class == CLASS_PALADIN && ( !extra_flags.test( ITEM_ANTI_EVIL ) && extra_flags.test( ITEM_ANTI_NEUTRAL ) && extra_flags.test( ITEM_ANTI_GOOD ) ) )
      buf.append( "(Smouldering Grey-White) " );

   if( ch->has_aflag( AFF_DETECT_MAGIC ) && extra_flags.test( ITEM_MAGIC ) )
      buf.append( "(Magical) " );
   if( !glowsee && extra_flags.test( ITEM_GLOW ) )
      buf.append( "(Glowing) " );
   if( extra_flags.test( ITEM_HUM ) )
      buf.append( "(Humming) " );
   if( extra_flags.test( ITEM_HIDDEN ) )
      buf.append( "(Hidden) " );
   if( extra_flags.test( ITEM_BURIED ) )
      buf.append( "(Buried) " );
   if( ch->is_immortal(  ) && extra_flags.test( ITEM_PROTOTYPE ) )
      buf.append( "(PROTO) " );
   if( ( ch->has_aflag( AFF_DETECTTRAPS ) || ch->has_pcflag( PCFLAG_HOLYLIGHT ) ) && is_trapped(  ) )
      buf.append( "(Trap) " );

   if( fShort )
   {
      if( glowsee && ( ch->isnpc(  ) || !ch->has_pcflag( PCFLAG_HOLYLIGHT ) ) )
         buf.append( "the faint glow of something" );
      else if( short_descr )
      {
         buf.append( short_descr );
         if( num > 1 )
         {
            char v[MIL];

            snprintf( v, MIL, " (%d)", num );

            buf.append( v );
         }

         if( !contents.empty(  ) )
         {
            list < obj_data * >::iterator iobj;
            bool showdot = false;

            for( iobj = contents.begin(  ); iobj != contents.end(  ); ++iobj )
            {
               obj_data *obj = *iobj;

               if( ( obj->item_type == ITEM_TRAP && ch->has_aflag( AFF_DETECTTRAPS ) ) || obj->item_type != ITEM_TRAP )
               {
                  showdot = true;
                  break;
               }
            }
            if( showdot )
               buf.append( " &W*" );
         }
      }
   }
   else
   {
      if( glowsee && ( ch->isnpc(  ) || !ch->has_pcflag( PCFLAG_HOLYLIGHT ) ) )
         buf.append( "You see the faint glow of something nearby." );
      if( objdesc && objdesc[0] != '\0' )
      {
         buf.append( objdesc );
         if( num > 1 )
         {
            char v[MIL];

            snprintf( v, MIL, " (%d)", num );

            buf.append( v );
         }
         if( !contents.empty(  ) )
         {
            list < obj_data * >::iterator iobj;
            bool showdot = false;

            for( iobj = contents.begin(  ); iobj != contents.end(  ); ++iobj )
            {
               obj_data *obj = *iobj;

               if( ( obj->item_type == ITEM_TRAP && ch->has_aflag( AFF_DETECTTRAPS ) ) || obj->item_type != ITEM_TRAP )
               {
                  showdot = true;
                  break;
               }
            }
            if( showdot )
               buf.append( " &W*" );
         }
      }
   }
   buf.append( ch->color_str( AT_OBJECT ) );
   return buf;
}

bool is_same_obj( obj_data * src, obj_data * dest )
{
   if( !src || !dest )
      return false;

   if( src == dest )
      return true;

   if( src->pIndexData->vnum == OBJ_VNUM_TREASURE || dest->pIndexData->vnum == OBJ_VNUM_TREASURE )
      return false;

   if( src->pIndexData == dest->pIndexData
       && !str_cmp( src->name, dest->name )
       && !str_cmp( src->short_descr, dest->short_descr )
       && !str_cmp( src->objdesc, dest->objdesc )
       && !str_cmp( src->socket[0], dest->socket[0] )
       && !str_cmp( src->socket[1], dest->socket[1] )
       && !str_cmp( src->socket[2], dest->socket[2] )
       && src->item_type == dest->item_type
       && src->extra_flags == dest->extra_flags
       && src->wear_flags == dest->wear_flags
       && src->wear_loc == dest->wear_loc
       && src->weight == dest->weight
       && src->cost == dest->cost
       && src->level == dest->level
       && src->timer == dest->timer
       && src->value[0] == dest->value[0]
       && src->value[1] == dest->value[1]
       && src->value[2] == dest->value[2]
       && src->value[3] == dest->value[3]
       && src->value[4] == dest->value[4]
       && src->value[5] == dest->value[5]
       && src->value[6] == dest->value[6]
       && src->value[7] == dest->value[7]
       && src->value[8] == dest->value[8]
       && src->value[9] == dest->value[9]
       && src->value[10] == dest->value[10]
       && src->contents == dest->contents
       && src->count + dest->count > 0
       && src->wmap == dest->wmap
       && src->mx == dest->mx && src->my == dest->my && !str_cmp( src->seller, dest->seller ) && !str_cmp( src->buyer, dest->buyer ) && src->bid == dest->bid )
      return true;

   return false;
}

/*
 * Some increasingly freaky hallucinated objects - Thoric
 * (Hats off to Albert Hoffman's "problem child")
 */
const char *hallucinated_object( int ms, bool fShort )
{
   int sms = URANGE( 1, ( ms + 10 ) / 5, 20 );

   if( fShort )
      switch ( number_range( 6 - URANGE( 1, sms / 2, 5 ), sms ) )
      {
         default:
         case 1:
            return "a sword";
         case 2:
            return "a stick";
         case 3:
            return "something shiny";
         case 4:
            return "something";
         case 5:
            return "something interesting";
         case 6:
            return "something colorful";
         case 7:
            return "something that looks cool";
         case 8:
            return "a nifty thing";
         case 9:
            return "a cloak of flowing colors";
         case 10:
            return "a mystical flaming sword";
         case 11:
            return "a swarm of insects";
         case 12:
            return "a deathbane";
         case 13:
            return "a figment of your imagination";
         case 14:
            return "your gravestone";
         case 15:
            return "the long lost boots of Ranger Samson";
         case 16:
            return "a glowing tome of arcane knowledge";
         case 17:
            return "a long sought secret";
         case 18:
            return "the meaning of it all";
         case 19:
            return "the answer";
         case 20:
            return "the key to life, the universe and everything";
      }

   switch ( number_range( 6 - URANGE( 1, sms / 2, 5 ), sms ) )
   {
      default:
      case 1:
         return "A nice looking sword catches your eye.";
      case 2:
         return "The ground is covered in small sticks.";
      case 3:
         return "Something shiny catches your eye.";
      case 4:
         return "Something catches your attention.";
      case 5:
         return "Something interesting catches your eye.";
      case 6:
         return "Something colorful flows by.";
      case 7:
         return "Something that looks cool calls out to you.";
      case 8:
         return "A nifty thing of great importance stands here.";
      case 9:
         return "A cloak of flowing colors asks you to wear it.";
      case 10:
         return "A mystical flaming sword awaits your grasp.";
      case 11:
         return "A swarm of insects buzzes in your face!";
      case 12:
         return "The extremely rare Deathbane lies at your feet.";
      case 13:
         return "A figment of your imagination is at your command.";
      case 14:
         return "You notice a gravestone here... upon closer examination, it reads your name.";
      case 15:
         return "The long lost boots of Ranger Samson lie off to the side.";
      case 16:
         return "A glowing tome of arcane knowledge hovers in the air before you.";
      case 17:
         return "A long sought secret of all mankind is now clear to you.";
      case 18:
         return "The meaning of it all, so simple, so clear... of course!";
      case 19:
         return "The answer.  One.  It's always been One.";
      case 20:
         return "The key to life, the universe and everything awaits your hand.";
   }
   return "Whoa!!!";
}

void show_list_to_char( char_data * ch, list < obj_data * >source, bool fShort, bool fShowNothing )
{
   map < obj_data *, int >objmap;
   int ms;
   bool found = false;

   /*
    * if there's no list... then don't do all this crap!  -Thoric
    */
   if( source.empty(  ) )
   {
      if( fShowNothing )
      {
         ch->print( "     " );
         ch->set_color( AT_OBJECT );
         ch->print( "Nothing.\r\n" );
      }
      return;
   }

   ms = ( ch->mental_state ? ch->mental_state : 1 ) * ( ch->isnpc(  )? 1 : ( ch->pcdata->condition[COND_DRUNK] ? ( ch->pcdata->condition[COND_DRUNK] / 12 ) : 1 ) );

   map < obj_data *, int >::iterator mobj;
   objmap.clear(  );
   list < obj_data * >::iterator iobj;
   for( iobj = source.begin(  ); iobj != source.end(  ); ++iobj )
   {
      obj_data *obj = *iobj;

      if( obj->extra_flags.test( ITEM_AUCTION ) )
         continue;

      found = false;
      for( mobj = objmap.begin(  ); mobj != objmap.end(  ); ++mobj )
      {
         if( is_same_obj( mobj->first, obj ) )
         {
            mobj->second += obj->count;
            found = true;
            break;
         }
      }
      if( !found )
         objmap[obj] = obj->count;
   }

   int wcount = 0;
   mobj = objmap.begin(  );
   while( mobj != objmap.end(  ) )
   {
      if( mobj->first->wear_loc == WEAR_NONE && ch->can_see_obj( mobj->first, false ) && ( mobj->first->item_type != ITEM_TRAP || ch->has_aflag( AFF_DETECTTRAPS ) ) )
      {
         switch ( mobj->first->item_type )
         {
            default:
               ch->set_color( AT_OBJECT );
               break;

            case ITEM_BLOOD:
               ch->set_color( AT_BLOOD );
               break;

            case ITEM_CORPSE_PC:
            case ITEM_CORPSE_NPC:
               ch->set_color( AT_ORANGE );
               break;

            case ITEM_MONEY:
            case ITEM_TREASURE:
               ch->set_color( AT_YELLOW );
               break;

            case ITEM_COOK:
            case ITEM_FOOD:
               ch->set_color( AT_HUNGRY );
               break;

            case ITEM_DRINK_CON:
            case ITEM_FOUNTAIN:
            case ITEM_PUDDLE:
               ch->set_color( AT_THIRSTY );
               break;

            case ITEM_FIRE:
               ch->set_color( AT_FIRE );
               break;

            case ITEM_SCROLL:
            case ITEM_WAND:
            case ITEM_STAFF:
               ch->set_color( AT_MAGIC );
               break;
         }
         ++wcount;
         if( abs( ms ) > 40 && number_bits( 1 ) == 0 )
            ch->print( hallucinated_object( ms, fShort ) );
         else
            ch->print( mobj->first->format_to_char( ch, fShort, mobj->second ) );
         ch->print( "\r\n" );
      }
      ++mobj;
   }

   if( wcount == 0 && fShowNothing )
   {
      ch->print( "     " );
      ch->set_color( AT_OBJECT );
      ch->print( "Nothing.\r\n" );
   }
}

/*
 * Give an obj to a char.
 */
obj_data *obj_data::to_char( char_data * ch )
{
   obj_data *otmp, *oret = this;
   bool skipgroup, grouped;
   int oweight = get_weight(  );
   int onum = get_number(  );
   int owear_loc = wear_loc;
   bitset < MAX_ITEM_FLAG > oextra_flags = extra_flags;

   skipgroup = false;
   grouped = false;

   if( extra_flags.test( ITEM_PROTOTYPE ) )
   {
      if( !ch->is_immortal(  ) && ( !ch->isnpc(  ) || !ch->has_actflag( ACT_PROTOTYPE ) ) )
         return to_room( ch->in_room, ch );
   }

   if( loading_char == ch )
   {
      int x, y;
      for( x = 0; x < MAX_WEAR; ++x )
         for( y = 0; y < MAX_LAYERS; ++y )
            if( ch->isnpc(  ) )
            {
               if( mob_save_equipment[x][y] == this )
               {
                  skipgroup = true;
                  break;
               }
            }
            else
            {
               if( save_equipment[x][y] == this )
               {
                  skipgroup = true;
                  break;
               }
            }
   }
   if( ch->isnpc(  ) && ch->pIndexData->pShop )
      skipgroup = true;

   list < obj_data * >::iterator iobj;
   if( !skipgroup )
      for( iobj = ch->carrying.begin(  ); iobj != ch->carrying.end(  ); ++iobj )
      {
         otmp = *iobj;
         if( ( oret = group_obj( otmp, this ) ) == otmp )
         {
            grouped = true;
            break;
         }
      }
   if( !grouped )
   {
      if( !ch->isnpc(  ) || !ch->pIndexData->pShop )
      {
         ch->carrying.push_back( this );
         carried_by = ch;
         in_room = nullptr;
         in_obj = nullptr;
         if( ch != supermob )
         {
            extra_flags.reset( ITEM_ONMAP );
            wmap = -1;
            mx = -1;
            my = -1;
         }
      }
      else
      {
         bool inserted = false;

         /*
          * If ch is a shopkeeper, add the obj using an insert sort 
          */
         for( iobj = ch->carrying.begin(  ); iobj != ch->carrying.end(  ); ++iobj )
         {
            otmp = *iobj;
            if( level > otmp->level )
            {
               ch->carrying.insert( iobj, this );
               inserted = true;
               break;
            }
            else if( level == otmp->level && !str_cmp( short_descr, otmp->short_descr ) )
            {
               ch->carrying.insert( iobj, this );
               inserted = true;
               break;
            }
         }
         if( !inserted )
            ch->carrying.push_back( this );

         carried_by = ch;
         in_room = nullptr;
         in_obj = nullptr;
         if( ch != supermob )
         {
            extra_flags.reset( ITEM_ONMAP );
            wmap = -1;
            mx = -1;
            my = -1;
         }
      }
   }
   if( owear_loc == WEAR_NONE )
   {
      ch->carry_number += onum;
      ch->carry_weight += oweight;
   }
   else if( !oextra_flags.test( ITEM_MAGIC ) )
      ch->carry_weight += oweight;
   return ( oret ? oret : this );
}

/*
 * Take an obj from its character.
 */
void obj_data::from_char(  )
{
   char_data *ch;

   if( !( ch = carried_by ) )
   {
      bug( "%s: null ch.", __func__ );
      log_printf( "Object was vnum %d - %s", pIndexData->vnum, short_descr );
      return;
   }

   if( wear_loc != WEAR_NONE )
      ch->unequip( this );

   /*
    * obj may drop during unequip... 
    */
   if( !carried_by )
      return;

   ch->carrying.remove( this );

   if( extra_flags.test( ITEM_COVERING ) && !contents.empty(  ) )
      empty( nullptr, nullptr );

   in_room = nullptr;
   carried_by = nullptr;
   ch->carry_number -= get_number(  );
   ch->carry_weight -= get_weight(  );
}

/*
 * Find the ac value of an obj, including position effect.
 */
int obj_data::apply_ac( int iWear )
{
   if( item_type != ITEM_ARMOR )
      return 0;

   switch ( iWear )
   {
      case WEAR_BODY:
         return 3 * value[0];

      case WEAR_HEAD:
      case WEAR_LEGS:
      case WEAR_ABOUT:
         return 2 * value[0];

      default:
      case WEAR_FEET:
      case WEAR_HANDS:
      case WEAR_ARMS:
      case WEAR_SHIELD:
      case WEAR_FINGER_L:
      case WEAR_FINGER_R:
      case WEAR_NECK_1:
      case WEAR_NECK_2:
      case WEAR_WAIST:
      case WEAR_WRIST_L:
      case WEAR_WRIST_R:
      case WEAR_HOLD:
      case WEAR_EYES:
      case WEAR_FACE:
      case WEAR_BACK:
      case WEAR_ANKLE_L:
      case WEAR_ANKLE_R:
         return value[0];
   }
   return 0;
}

/*
 * Move an obj out of a room.
 */
void obj_data::from_room(  )
{
   room_index *room;

   if( !( room = in_room ) )
   {
      bug( "%s: %s not in a room!", __func__, name );
      return;
   }

   /*
    * Should handle all cases of picking stuff up from maps - Samson 
    */
   extra_flags.reset( ITEM_ONMAP );
   mx = -1;
   my = -1;
   wmap = -1;

   list < affect_data * >::iterator paf;
   for( paf = affects.begin(  ); paf != affects.end(  ); ++paf )
   {
      affect_data *af = *paf;

      room->room_affect( af, false );
   }

   for( paf = pIndexData->affects.begin(  ); paf != pIndexData->affects.end(  ); ++paf )
   {
      affect_data *af = *paf;

      room->room_affect( af, false );
   }

   room->objects.remove( this );

   /*
    * uncover contents 
    */
   if( extra_flags.test( ITEM_COVERING ) && !contents.empty(  ) )
      empty( nullptr, in_room );

   if( item_type == ITEM_FIRE )
      in_room->light -= count;

   in_room->weight -= this->get_weight( );
   carried_by = nullptr;
   in_obj = nullptr;
   in_room = nullptr;
   if( pIndexData->vnum == OBJ_VNUM_CORPSE_PC && falling < 1 )
      write_corpse( this, short_descr + 14 );
}

/*
 * Move an obj into a room.
 */
obj_data *obj_data::to_room( room_index * pRoomIndex, char_data * ch )
{
   short ocount = count;
   short oitem_type = item_type;
   list < affect_data * >::iterator paf;

   for( paf = affects.begin(  ); paf != affects.end(  ); ++paf )
   {
      affect_data *af = *paf;
      pRoomIndex->room_affect( af, true );
   }

   for( paf = pIndexData->affects.begin(  ); paf != pIndexData->affects.end(  ); ++paf )
   {
      affect_data *af = *paf;
      pRoomIndex->room_affect( af, true );
   }

   pRoomIndex->weight += this->get_weight( );

   list < obj_data * >::iterator iobj;
   for( iobj = pRoomIndex->objects.begin(  ); iobj != pRoomIndex->objects.end(  ); ++iobj )
   {
      obj_data *oret, *otmp = *iobj;
      if( ( oret = group_obj( otmp, this ) ) == otmp )
      {
         if( oitem_type == ITEM_FIRE )
            pRoomIndex->light += ocount;
         return oret;
      }
   }

   // Want to see if pushing to the front of the list instead helps with stuff like sacing corpses.
   pRoomIndex->objects.push_front( this );

   in_room = pRoomIndex;
   carried_by = nullptr;
   in_obj = nullptr;
   room_vnum = pRoomIndex->vnum; /* hotboot tracker */
   if( oitem_type == ITEM_FIRE )
      pRoomIndex->light += count;
   ++falling;
   fall( false );
   --falling;

   /*
    * Hoping that this will cover all instances of objects from character to room - Samson 8-22-99 
    */
   if( ch != nullptr )
   {
      if( ch->has_actflag( ACT_ONMAP ) || ch->has_pcflag( PCFLAG_ONMAP ) )
      {
         extra_flags.set( ITEM_ONMAP );
         wmap = ch->wmap;
         mx = ch->mx;
         my = ch->my;
      }
      else
      {
         extra_flags.reset( ITEM_ONMAP );
         wmap = -1;
         mx = -1;
         my = -1;
      }
   }

   if( pIndexData->vnum == OBJ_VNUM_CORPSE_PC && falling < 1 )
      write_corpse( this, short_descr + 14 );
   return this;
}

/*
 * Move an object into an object.
 */
obj_data *obj_data::to_obj( obj_data * obj_to )
{
   if( this == obj_to )
   {
      bug( "%s: trying to put object inside itself: vnum %d", __func__, pIndexData->vnum );
      return this;
   }

   char_data *who;
   if( !obj_to->in_magic_container(  ) && ( who = obj_to->who_carrying(  ) ) != nullptr )
      who->carry_weight += get_weight(  );

   if( obj_to->in_room )
      obj_to->in_room->weight += this->get_weight( );

   list < obj_data * >::iterator iobj;
   for( iobj = obj_to->contents.begin(  ); iobj != obj_to->contents.end(  ); ++iobj )
   {
      obj_data *oret, *otmp = *iobj;
      if( ( oret = group_obj( otmp, this ) ) == otmp )
         return oret;
   }

   obj_to->contents.push_back( this );
   in_obj = obj_to;
   in_room = nullptr;
   carried_by = nullptr;

   return this;
}

/*
 * Move an object out of an object.
 */
void obj_data::from_obj(  )
{
   obj_data *obj_from;
   bool magic;

   if( !( obj_from = in_obj ) )
   {
      bug( "%s: %s null obj_from.", __func__, name );
      return;
   }

   magic = obj_from->in_magic_container(  );

   obj_from->contents.remove( this );

   /*
    * uncover contents 
    */
   if( extra_flags.test( ITEM_COVERING ) && !contents.empty(  ) )
      empty( in_obj, nullptr );

   in_obj = nullptr;
   in_room = nullptr;
   carried_by = nullptr;
   if( obj_from->in_room )
      obj_from->in_room->weight -= this->get_weight( );

   /*
    * This will hopefully cover all objs coming from containers going to the maps - Samson 8-22-99 
    */
   if( obj_from->extra_flags.test( ITEM_ONMAP ) )
   {
      extra_flags.set( ITEM_ONMAP );
      wmap = obj_from->wmap;
      mx = obj_from->mx;
      my = obj_from->my;
   }

   if( !magic )
      for( ; obj_from; obj_from = obj_from->in_obj )
         if( obj_from->carried_by )
            obj_from->carried_by->carry_weight -= get_weight(  );
}

/*
 * Extract an obj from the world.
 */
void obj_data::extract(  )
{
   if( extracted(  ) )
   {
      bug( "%s: obj %d already extracted!", __func__, pIndexData->vnum );
      /*
       * return; Seeing if we can get it to either extract it for real, or die trying! 
       */
   }

   if( item_type == ITEM_PORTAL )
      remove_portal(  );

   if( carried_by )
      from_char(  );
   else if( in_room )
      from_room(  );
   else if( in_obj )
      from_obj(  );

   list < obj_data * >::iterator iobj;
   for( iobj = contents.begin(  ); iobj != contents.end(  ); )
   {
      obj_data *obj = *iobj;
      ++iobj;

      obj->extract(  );
   }
   contents.clear(  );

   /*
    * shove onto extraction queue 
    */
   queue_extracted_obj( this );
   pIndexData->count -= count;
   numobjsloaded -= count;
   --physicalobjects;
}

int obj_data::get_number(  )
{
   return count;
}

/*
 * Return weight of an object, including weight of contents (unless magic).
 */
int obj_data::get_weight(  )
{
   list < obj_data * >::iterator iobj;
   int oweight = count * weight;

   if( item_type != ITEM_CONTAINER || !extra_flags.test( ITEM_MAGIC ) )
      for( iobj = contents.begin(  ); iobj != contents.end(  ); ++iobj )
      {
         obj_data *obj = *iobj;
         oweight += obj->get_weight(  );
      }
   return oweight;
}

/*
 * Return real weight of an object, including weight of contents.
 */
int obj_data::get_real_weight(  )
{
   list < obj_data * >::iterator iobj;
   int oweight = count * weight;

   for( iobj = contents.begin(  ); iobj != contents.end(  ); ++iobj )
   {
      obj_data *obj = *iobj;
      oweight += obj->get_real_weight(  );
   }
   return oweight;
}

/*
 * Return ascii name of an item type.
 */
const string obj_data::item_type_name(  )
{
   if( item_type < 1 || item_type >= MAX_ITEM_TYPE )
   {
      bug( "%s: unknown type %d.", __func__, item_type );
      return "(unknown)";
   }
   return o_types[item_type];
}

/*
 * return true if an object contains a trap - Thoric
 */
bool obj_data::is_trapped(  )
{
   if( contents.empty(  ) )
      return false;

   list < obj_data * >::iterator iobj;
   for( iobj = contents.begin(  ); iobj != contents.end(  ); ++iobj )
   {
      obj_data *obj = *iobj;
      if( obj->item_type == ITEM_TRAP )
         return true;
   }
   return false;
}

/*
 * If an object contains a trap, return the pointer to the trap - Thoric
 */
obj_data *obj_data::get_trap(  )
{
   if( contents.empty(  ) )
      return nullptr;

   list < obj_data * >::iterator iobj;
   for( iobj = contents.begin(  ); iobj != contents.end(  ); ++iobj )
   {
      obj_data *obj = *iobj;
      if( obj->item_type == ITEM_TRAP )
         return obj;
   }
   return nullptr;
}

/*
 * Return a pointer to the first object of a certain type found that
 * a player is carrying/wearing
 */
obj_data *get_objtype( char_data * ch, short type )
{
   list < obj_data * >::iterator iobj;

   for( iobj = ch->carrying.begin(  ); iobj != ch->carrying.end(  ); ++iobj )
   {
      obj_data *obj = *iobj;
      if( obj->item_type == type )
         return obj;
   }
   return nullptr;
}

/*
 * Make a simple clone of an object (no extras...yet) - Thoric
 */
obj_data *obj_data::clone(  )
{
   obj_data *oclone;

   oclone = new obj_data;

   oclone->pIndexData = pIndexData;
   oclone->name = QUICKLINK( name );
   oclone->short_descr = QUICKLINK( short_descr );
   oclone->objdesc = QUICKLINK( objdesc );
   if( action_desc && action_desc[0] != '\0' )
      oclone->action_desc = QUICKLINK( action_desc );
   if( socket[0] && socket[0][0] != '\0' )
      oclone->socket[0] = QUICKLINK( socket[0] );
   if( socket[1] && socket[1][0] != '\0' )
      oclone->socket[1] = QUICKLINK( socket[1] );
   if( socket[2] && socket[2][0] != '\0' )
      oclone->socket[2] = QUICKLINK( socket[2] );
   oclone->item_type = item_type;
   oclone->extra_flags = extra_flags;
   oclone->wear_flags = wear_flags;
   oclone->wear_loc = wear_loc;
   oclone->weight = weight;
   oclone->cost = cost;
   oclone->level = level;
   oclone->timer = timer;
   oclone->wmap = wmap;
   oclone->mx = mx;
   oclone->my = my;
   for( int x = 0; x < MAX_OBJ_VALUE; ++x )
      oclone->value[x] = value[x];
   oclone->count = 1;
   ++pIndexData->count;
   ++numobjsloaded;
   ++physicalobjects;
   objlist.push_back( oclone );
   return oclone;
}

/*
 * If possible group obj2 into obj1 - Thoric
 * This code, along with clone_object, obj->count, and special support
 * for it implemented throughout handler.c and save.c should show improved
 * performance on MUDs with players that hoard tons of potions and scrolls
 * as this will allow them to be grouped together both in memory, and in
 * the player files.
 */
obj_data *group_obj( obj_data * obj, obj_data * obj2 )
{
   if( !obj || !obj2 )
      return nullptr;

   if( obj == obj2 )
      return obj;

   if( obj->pIndexData->vnum == OBJ_VNUM_TREASURE || obj2->pIndexData->vnum == OBJ_VNUM_TREASURE )
      return obj2;

   if( obj->pIndexData == obj2->pIndexData
    && !str_cmp( obj->name, obj2->name )
    && !str_cmp( obj->short_descr, obj2->short_descr )
    && !str_cmp( obj->objdesc, obj2->objdesc )
    && ( obj->action_desc && obj2->action_desc && !str_cmp( obj->action_desc, obj2->action_desc ) )
    && !str_cmp( obj->socket[0], obj2->socket[0] )
    && !str_cmp( obj->socket[1], obj2->socket[1] )
    && !str_cmp( obj->socket[2], obj2->socket[2] )
    && obj->item_type == obj2->item_type
    && obj->extra_flags == obj2->extra_flags
    && obj->wear_flags == obj2->wear_flags
    && obj->wear_loc == obj2->wear_loc
    && obj->weight == obj2->weight
    && obj->cost == obj2->cost
    && obj->level == obj2->level
    && obj->timer == obj2->timer
    && obj->value[0] == obj2->value[0]
    && obj->value[1] == obj2->value[1]
    && obj->value[2] == obj2->value[2]
    && obj->value[3] == obj2->value[3]
    && obj->value[4] == obj2->value[4]
    && obj->value[5] == obj2->value[5]
    && obj->value[6] == obj2->value[6]
    && obj->value[7] == obj2->value[7]
    && obj->value[8] == obj2->value[8]
    && obj->value[9] == obj2->value[9]
    && obj->value[10] == obj2->value[10]
    && obj->extradesc.empty(  ) && obj2->extradesc.empty(  )
    && obj->affects.empty(  ) && obj2->affects.empty(  )
    && obj->contents.empty(  ) && obj2->contents.empty(  )
    && obj->wmap == obj2->wmap
    && obj->mx == obj2->mx
    && obj->my == obj2->my
    && !str_cmp( obj->seller, obj2->seller )
    && !str_cmp( obj->buyer, obj2->buyer )
    && obj->bid == obj2->bid
    && obj->count + obj2->count <= SHRT_MAX )   /* prevent count overflow */
   {
      obj->count += obj2->count;
      obj->pIndexData->count += obj2->count; /* to be decremented in */
      numobjsloaded += obj2->count; /* extract_obj */
      obj2->extract(  );
      return obj;
   }
   return obj2;
}

/*
 * Split off a grouped object - Thoric
 * decreased obj's count to num, and creates a new object containing the rest
 */
void obj_data::split( int num )
{
   int ocount = count;
   obj_data *rest;

   if( ocount <= num || num == 0 )
      return;

   rest = clone(  );
   --pIndexData->count; /* since clone_object() ups this value */
   --numobjsloaded;
   rest->count = count - num;
   count = num;

   if( carried_by )
   {
      carried_by->carrying.push_back( rest );
      rest->carried_by = carried_by;
      rest->in_room = nullptr;
      rest->in_obj = nullptr;
   }
   else if( in_room )
   {
      in_room->objects.push_back( rest );
      rest->carried_by = nullptr;
      rest->in_room = in_room;
      rest->in_obj = nullptr;
   }
   else if( in_obj )
   {
      in_obj->contents.push_back( rest );
      rest->in_obj = in_obj;
      rest->in_room = nullptr;
      rest->carried_by = nullptr;
   }
}

void obj_data::separate(  )
{
   split( 1 );
}

/*
 * Empty an obj's contents... optionally into another obj, or a room
 */
bool obj_data::empty( obj_data * destobj, room_index * destroom )
{
   obj_data *otmp;
   list < obj_data * >::iterator iobj;
   char_data *ch = carried_by;
   bool movedsome = false;

   if( destobj || ( !destroom && !ch && ( destobj = in_obj ) != nullptr ) )
   {
      for( iobj = contents.begin(  ); iobj != contents.end(  ); )
      {
         otmp = *iobj;
         ++iobj;

         /*
          * only keys on a keyring 
          */
         if( destobj->item_type == ITEM_KEYRING && otmp->item_type != ITEM_KEY )
            continue;
         if( destobj->item_type == ITEM_QUIVER && otmp->item_type != ITEM_PROJECTILE )
            continue;
         if( ( destobj->item_type == ITEM_CONTAINER || destobj->item_type == ITEM_KEYRING || destobj->item_type == ITEM_QUIVER )
             && ( otmp->get_real_weight( ) + destobj->get_real_weight( ) > destobj->value[0]
                  || ( destobj->in_room && destobj->in_room->max_weight && destobj->in_room->max_weight < otmp->get_real_weight( ) + destobj->in_room->weight ) ) )
            continue;
         otmp->from_obj(  );
         otmp->to_obj( destobj );
         movedsome = true;
      }
      return movedsome;
   }

   if( destroom || ( !ch && ( destroom = in_room ) != nullptr ) )
   {
      for( iobj = contents.begin(  ); iobj != contents.end(  ); )
      {
         otmp = *iobj;
         ++iobj;

         if( destroom->max_weight && destroom->max_weight < otmp->get_real_weight( ) + destroom->weight )
            continue;

         if( ch && HAS_PROG( otmp->pIndexData, DROP_PROG ) && otmp->count > 1 )
         {
            otmp->separate(  );
            otmp->from_obj(  );
         }
         else
            otmp->from_obj(  );
         otmp->to_room( destroom, ch );
         if( ch )
         {
            oprog_drop_trigger( ch, otmp );  /* mudprogs */
            if( ch->char_died(  ) )
               ch = nullptr;
         }
         movedsome = true;
      }
      return movedsome;
   }

   if( ch )
   {
      for( iobj = contents.begin(  ); iobj != contents.end(  ); )
      {
         otmp = *iobj;
         ++iobj;

         otmp->from_obj(  );
         otmp->to_char( ch );
         movedsome = true;
      }
      return movedsome;
   }
   bug( "%s: could not determine a destination for vnum %d", __func__, pIndexData->vnum );
   return false;
}

void obj_data::remove_portal(  )
{
   room_index *fromRoom, *toRoom;

   if( !( fromRoom = in_room ) )
   {
      bug( "%s: portal->in_room is nullptr", __func__ );
      return;
   }

   exit_data *pexit;
   bool found = false;
   list < exit_data * >::iterator iexit;
   for( iexit = fromRoom->exits.begin(  ); iexit != fromRoom->exits.end(  ); ++iexit )
   {
      pexit = *iexit;

      if( IS_EXIT_FLAG( pexit, EX_PORTAL ) )
      {
         found = true;
         break;
      }
   }

   if( !found )
   {
      bug( "%s: portal not found in room %d!", __func__, fromRoom->vnum );
      return;
   }

   if( pexit->vdir != DIR_PORTAL )
      bug( "%s: exit in dir %d != DIR_PORTAL", __func__, pexit->vdir );

   if( !( toRoom = pexit->to_room ) )
      bug( "%s: toRoom is nullptr", __func__ );

   fromRoom->extract_exit( pexit );
}

/*
 * Who's carrying an item -- recursive for nested objects	-Thoric
 */
char_data *obj_data::who_carrying(  )
{
   if( in_obj )
      return in_obj->who_carrying(  );

   return carried_by;
}

/*
 * Return true if an object is, or nested inside a magic container
 */
bool obj_data::in_magic_container(  )
{
   if( item_type == ITEM_CONTAINER && extra_flags.test( ITEM_MAGIC ) )
      return true;
   if( in_obj )
      return in_obj->in_magic_container(  );
   return false;
}

/*
 * Turn an object into scraps. - Thoric
 */
void obj_data::make_scraps(  )
{
   obj_data *scraps, *tmpobj;
   char_data *ch = nullptr;

   separate(  );
   if( !( scraps = get_obj_index( OBJ_VNUM_SCRAPS )->create_object( 1 ) ) )
   {
      log_printf( "create_object: %s:%s, line %d.", __FILE__, __func__, __LINE__ );
      return;
   }
   scraps->timer = number_range( 5, 15 );
   if( extra_flags.test( ITEM_ONMAP ) )
   {
      scraps->extra_flags.set( ITEM_ONMAP );
      scraps->wmap = wmap;
      scraps->mx = mx;
      scraps->my = my;
   }

   /*
    * don't make scraps of scraps of scraps of ... 
    */
   if( pIndexData->vnum == OBJ_VNUM_SCRAPS )
   {
      stralloc_printf( &scraps->short_descr, "%s", "some debris" );
      stralloc_printf( &scraps->objdesc, "%s", "Bits of debris lie on the ground here." );
   }
   else
   {
      stralloc_printf( &scraps->short_descr, scraps->short_descr, short_descr );
      stralloc_printf( &scraps->objdesc, scraps->objdesc, short_descr );
   }

   if( carried_by )
   {
      act( AT_OBJECT, "$p falls to the ground in scraps!", carried_by, this, nullptr, TO_CHAR );
      if( this == carried_by->get_eq( WEAR_WIELD ) && ( tmpobj = carried_by->get_eq( WEAR_DUAL_WIELD ) ) != nullptr )
         tmpobj->wear_loc = WEAR_WIELD;

      scraps->to_room( carried_by->in_room, carried_by );
   }
   else if( in_room )
   {
      if( !in_room->people.empty(  ) )
      {
         act( AT_OBJECT, "$p is reduced to little more than scraps.", ( *in_room->people.begin(  ) ), this, nullptr, TO_ROOM );
         act( AT_OBJECT, "$p is reduced to little more than scraps.", ( *in_room->people.begin(  ) ), this, nullptr, TO_CHAR );
      }
      scraps->to_room( in_room, nullptr );
   }
   if( ( item_type == ITEM_CONTAINER || item_type == ITEM_KEYRING || item_type == ITEM_QUIVER || item_type == ITEM_CORPSE_PC ) && !contents.empty(  ) )
   {
      if( ch && ch->in_room )
      {
         act( AT_OBJECT, "The contents of $p fall to the ground.", ch, this, nullptr, TO_ROOM );
         act( AT_OBJECT, "The contents of $p fall to the ground.", ch, this, nullptr, TO_CHAR );
      }
      if( carried_by )
         empty( nullptr, carried_by->in_room );
      else if( in_room )
         empty( nullptr, in_room );
      else if( in_obj )
         empty( in_obj, nullptr );
   }
   extract(  );
}

/*
 * Calculate the tohit bonus on the object and return RIS values.
 * -- Altrag
 */
int obj_data::hitroll(  )
{
   int tohit = 0;
   list < affect_data * >::iterator paf;

   for( paf = pIndexData->affects.begin(  ); paf != pIndexData->affects.end(  ); ++paf )
   {
      affect_data *af = *paf;

      if( af->location == APPLY_HITROLL || af->location == APPLY_HITNDAM )
         tohit += af->modifier;
   }

   for( paf = affects.begin(  ); paf != affects.end(  ); ++paf )
   {
      affect_data *af = *paf;

      if( af->location == APPLY_HITROLL || af->location == APPLY_HITNDAM )
         tohit += af->modifier;
   }
   return tohit;
}

/*
 * Function to strip off the "a" or "an" or "the" or "some" from an object's
 * short description for the purpose of using it in a sentence sent to
 * the owner of the object.  (Ie: an object with the short description
 * "a long dark blade" would return "long dark blade" for use in a sentence
 * like "Your long dark blade".  The object name isn't always appropriate
 * since it contains keywords that may not look proper.		-Thoric
 */
const string obj_data::myobj(  )
{
   if( !str_prefix( "a ", short_descr ) )
      return short_descr + 2;
   if( !str_prefix( "an ", short_descr ) )
      return short_descr + 3;
   if( !str_prefix( "the ", short_descr ) )
      return short_descr + 4;
   if( !str_prefix( "some ", short_descr ) )
      return short_descr + 5;
   return short_descr;
}

void obj_identify_output( char_data * ch, obj_data * obj )
{
   skill_type *sktmp;

   ch->set_color( AT_OBJECT );
   ch->printf( "Object: %s\r\n", obj->short_descr );
   if( ch->level >= LEVEL_IMMORTAL )
      ch->printf( "Vnum: %d\r\n", obj->pIndexData->vnum );
   ch->printf( "Keywords: %s\r\n", obj->name );
   ch->printf( "Type: %s\r\n", obj->item_type_name(  ).c_str(  ) );
   ch->printf( "Wear Flags : %s\r\n", bitset_string( obj->wear_flags, w_flags ) );
   ch->printf( "Layers     : %d\r\n", obj->pIndexData->layers );
   ch->printf( "Extra Flags: %s\r\n", bitset_string( obj->extra_flags, o_flags ) );
   ch->printf( "Weight: %d  Value: %d  Ego: %d\r\n", obj->weight, obj->cost, obj->ego );
   ch->set_color( AT_MAGIC );

   switch ( obj->item_type )
   {
      default:
         break;

      case ITEM_CONTAINER:
         ch->printf( "%s appears to be %s.\r\n", capitalize( obj->short_descr ),
                     obj->value[0] < 76 ? "of a small capacity" :
                     obj->value[0] < 150 ? "of a small to medium capacity" :
                     obj->value[0] < 300 ? "of a medium capacity" :
                     obj->value[0] < 550 ? "of a medium to large capacity" : obj->value[0] < 751 ? "of a large capacity" : "of a giant capacity" );
         break;

      case ITEM_PILL:
      case ITEM_SCROLL:
      case ITEM_POTION:
         ch->printf( "Level %d spells of:", obj->value[0] );

         if( obj->value[1] >= 0 && ( sktmp = get_skilltype( obj->value[1] ) ) != nullptr )
            ch->printf( " '%s'", sktmp->name );

         if( obj->value[2] >= 0 && ( sktmp = get_skilltype( obj->value[2] ) ) != nullptr )
            ch->printf( " '%s'", sktmp->name );

         if( obj->value[3] >= 0 && ( sktmp = get_skilltype( obj->value[3] ) ) != nullptr )
            ch->printf( " '%s'", sktmp->name );

         ch->print( ".\r\n" );
         break;

      case ITEM_SALVE:
         ch->printf( "Has %d of %d applications of level %d", obj->value[2], obj->value[1], obj->value[0] );
         if( obj->value[4] >= 0 && ( sktmp = get_skilltype( obj->value[4] ) ) != nullptr )
            ch->printf( " '%s'", sktmp->name );

         if( obj->value[5] >= 0 && ( sktmp = get_skilltype( obj->value[5] ) ) != nullptr )
            ch->printf( " '%s'", sktmp->name );

         ch->print( ".\r\n" );
         break;

      case ITEM_WAND:
      case ITEM_STAFF:
         ch->printf( "Has %d of %d charges of level %d", obj->value[2], obj->value[1], obj->value[0] );

         if( obj->value[3] >= 0 && ( sktmp = get_skilltype( obj->value[3] ) ) != nullptr )
            ch->printf( " '%s'", sktmp->name );

         ch->print( ".\r\n" );
         break;

      case ITEM_WEAPON:
         ch->printf( "Damage is %d to %d (average %d)%s\r\n",
                     obj->value[1], obj->value[2], ( obj->value[1] + obj->value[2] ) / 2, obj->extra_flags.test( ITEM_POISONED ) ? ", and is poisonous." : "." );
         ch->printf( "Skill needed: %s\r\n", weapon_skills[obj->value[4]] );
         ch->printf( "Damage type:  %s\r\n", attack_table[obj->value[3]] );
         ch->printf( "Current condition: %s\r\n", condtxt( obj->value[6], obj->value[0] ).c_str(  ) );
         if( obj->value[7] > 0 )
            ch->printf( "Available sockets: %d\r\n", obj->value[7] );
         if( obj->socket[0] && str_cmp( obj->socket[0], "None" ) )
            ch->printf( "Socket 1: %s Rune\r\n", obj->socket[0] );
         if( obj->socket[1] && str_cmp( obj->socket[1], "None" ) )
            ch->printf( "Socket 2: %s Rune\r\n", obj->socket[1] );
         if( obj->socket[2] && str_cmp( obj->socket[2], "None" ) )
            ch->printf( "Socket 3: %s Rune\r\n", obj->socket[2] );
         break;

      case ITEM_MISSILE_WEAPON:
         ch->printf( "Bonus damage added to projectiles is %d to %d (average %d).\r\n", obj->value[1], obj->value[2], ( obj->value[1] + obj->value[2] ) / 2 );
         ch->printf( "Skill needed:      %s\r\n", weapon_skills[obj->value[4]] );
         ch->printf( "Projectiles fired: %s\r\n", projectiles[obj->value[5]] );
         ch->printf( "Current condition: %s\r\n", condtxt( obj->value[6], obj->value[0] ).c_str(  ) );
         if( obj->value[7] > 0 )
            ch->printf( "Available sockets: %d\r\n", obj->value[7] );
         if( obj->socket[0] && str_cmp( obj->socket[0], "None" ) )
            ch->printf( "Socket 1: %s Rune\r\n", obj->socket[0] );
         if( obj->socket[1] && str_cmp( obj->socket[1], "None" ) )
            ch->printf( "Socket 2: %s Rune\r\n", obj->socket[1] );
         if( obj->socket[2] && str_cmp( obj->socket[2], "None" ) )
            ch->printf( "Socket 3: %s Rune\r\n", obj->socket[2] );
         break;

      case ITEM_PROJECTILE:
         ch->printf( "Damage is %d to %d (average %d)%s\r\n",
                     obj->value[1], obj->value[2], ( obj->value[1] + obj->value[2] ) / 2, obj->extra_flags.test( ITEM_POISONED ) ? ", and is poisonous." : "." );
         ch->printf( "Damage type: %s\r\n", attack_table[obj->value[3]] );
         ch->printf( "Projectile type: %s\r\n", projectiles[obj->value[4]] );
         ch->printf( "Current condition: %s\r\n", condtxt( obj->value[5], obj->value[0] ).c_str(  ) );
         break;

      case ITEM_ARMOR:
         ch->printf( "Current AC value: %d\r\n", obj->value[1] );
         ch->printf( "Current condition: %s\r\n", condtxt( obj->value[1], obj->value[0] ).c_str(  ) );
         if( obj->value[2] > 0 )
            ch->printf( "Available sockets: %d\r\n", obj->value[2] );
         if( obj->socket[0] && str_cmp( obj->socket[0], "None" ) )
            ch->printf( "Socket 1: %s Rune\r\n", obj->socket[0] );
         if( obj->socket[1] && str_cmp( obj->socket[1], "None" ) )
            ch->printf( "Socket 2: %s Rune\r\n", obj->socket[1] );
         if( obj->socket[2] && str_cmp( obj->socket[2], "None" ) )
            ch->printf( "Socket 3: %s Rune\r\n", obj->socket[2] );
         break;
   }

   list < affect_data * >::iterator paf;
   for( paf = obj->affects.begin(  ); paf != obj->affects.end(  ); ++paf )
   {
      affect_data *af = *paf;

      ch->showaffect( af );
   }

   for( paf = obj->pIndexData->affects.begin(  ); paf != obj->pIndexData->affects.end(  ); ++paf )
   {
      affect_data *af = *paf;

      ch->showaffect( af );
   }
}
