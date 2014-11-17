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
 *                        Object Index Support Functions                    *
 ****************************************************************************/

#include "mud.h"
#include "area.h"
#include "objindex.h"
#include "mud_prog.h"

map < int, obj_index * >obj_index_table;

extern int top_affect;

obj_index::~obj_index(  )
{
   area->objects.remove( this );

   /*
    * Remove references to object index 
    */
   list < obj_data * >::iterator iobj;
   for( iobj = objlist.begin(  ); iobj != objlist.end(  ); )
   {
      obj_data *o = *iobj;
      ++iobj;

      if( o->pIndexData == this )
         o->extract(  );
   }

   list < char_data * >::iterator ich;
   for( ich = pclist.begin(  ); ich != pclist.end(  ); ++ich )
   {
      char_data *ch = *ich;

      if( ch->substate == SUB_OBJ_EXTRA && ch->pcdata->dest_buf )
      {
         list < extra_descr_data * >::iterator ex;

         for( ex = extradesc.begin(  ); ex != extradesc.end(  ); ++ex )
         {
            extra_descr_data *ed = *ex;

            if( ed == ch->pcdata->dest_buf )
            {
               ch->print( "You suddenly forget which object you were editing!\r\n" );
               ch->stop_editing(  );
               ch->substate = SUB_NONE;
               break;
            }
         }
      }
      else if( ch->substate == SUB_MPROG_EDIT && ch->pcdata->dest_buf )
      {
         list < mud_prog_data * >::iterator mpg;

         for( mpg = mudprogs.begin(  ); mpg != mudprogs.end(  ); )
         {
            mud_prog_data *mp = *mpg;

            if( mp == ch->pcdata->dest_buf )
            {
               ch->print( "You suddenly forget which object you were working on.\r\n" );
               ch->stop_editing(  );
               ch->pcdata->dest_buf = NULL;
               ch->substate = SUB_NONE;
               break;
            }
         }
      }
   }

   list < extra_descr_data * >::iterator ed;
   for( ed = extradesc.begin(  ); ed != extradesc.end(  ); )
   {
      extra_descr_data *desc = *ed;
      ++ed;

      extradesc.remove( desc );
      deleteptr( desc );
      --top_ed;
   }
   extradesc.clear(  );

   list < affect_data * >::iterator paf;
   for( paf = affects.begin(  ); paf != affects.end(  ); )
   {
      affect_data *aff = *paf;
      ++paf;

      affects.remove( aff );
      deleteptr( aff );
      --top_affect;
   }
   affects.clear(  );

   list < mud_prog_data * >::iterator mpg;
   for( mpg = mudprogs.begin(  ); mpg != mudprogs.end(  ); )
   {
      mud_prog_data *mprog = *mpg;
      ++mpg;

      mudprogs.remove( mprog );
      deleteptr( mprog );
   }
   mudprogs.clear(  );

   STRFREE( name );
   STRFREE( short_descr );
   STRFREE( objdesc );
   STRFREE( action_desc );
   STRFREE( socket[0] );
   STRFREE( socket[1] );
   STRFREE( socket[2] );

   map < int, obj_index * >::iterator mobj;
   if( ( mobj = obj_index_table.find( vnum ) ) != obj_index_table.end(  ) )
      obj_index_table.erase( mobj );
   --top_obj_index;
}

obj_index::obj_index(  )
{
   init_memory( &next, &layers, sizeof( layers ) );
}

/*
 * clean out an object (index) (leave list pointers intact ) - Thoric
 */
void obj_index::clean_obj(  )
{
   STRFREE( name );
   STRFREE( short_descr );
   STRFREE( objdesc );
   STRFREE( action_desc );
   STRFREE( socket[0] );
   STRFREE( socket[1] );
   STRFREE( socket[2] );
   item_type = 0;
   extra_flags.reset(  );
   wear_flags.reset(  );
   count = 0;
   weight = 0;
   cost = 0;
   level = 1;
   for( int x = 0; x < MAX_OBJ_VALUE; ++x )
      value[x] = 0;

   list < affect_data * >::iterator paf;
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
   list < extra_descr_data * >::iterator ed;
   for( ed = extradesc.begin(  ); ed != extradesc.end(  ); )
   {
      extra_descr_data *desc = *ed;
      ++ed;

      extradesc.remove( desc );
      deleteptr( desc );
      --top_ed;
   }
   extradesc.clear(  );

   list < mud_prog_data * >::iterator mpg;
   for( mpg = mudprogs.begin(  ); mpg != mudprogs.end(  ); )
   {
      mud_prog_data *mprog = *mpg;
      ++mpg;

      mudprogs.remove( mprog );
      deleteptr( mprog );
   }
   mudprogs.clear(  );
}

/*
 * Translates obj virtual number to its obj index struct.
 * Hash table lookup.
 */
obj_index *get_obj_index( int vnum )
{
   map < int, obj_index * >::iterator iobj;

   if( vnum < 0 )
      vnum = 0;

   if( ( iobj = obj_index_table.find( vnum ) ) != obj_index_table.end(  ) )
      return iobj->second;

   if( fBootDb )
      bug( "%s: bad vnum %d.", __func__, vnum );

   return NULL;
}

/*
 * Create an instance of an object.
 */
obj_data *obj_index::create_object( int olevel )
{
   obj_data *obj;

   if( !this )
   {
      bug( "%s: NULL pObjIndex.", __func__ );
      return NULL;
   }

   obj = new obj_data;
   obj->pIndexData = this;

   obj->in_room = NULL;
   obj->in_obj = NULL;
   obj->carried_by = NULL;
   obj->extradesc.clear(  );
   obj->affects.clear(  );
   obj->level = olevel;
   obj->wear_loc = -1;
   obj->count = 1;
   obj->name = QUICKLINK( name );
   if( short_descr && short_descr[0] != '\0' )
      obj->short_descr = QUICKLINK( short_descr );
   if( objdesc && objdesc[0] != '\0' )
      obj->objdesc = QUICKLINK( objdesc );
   if( action_desc && action_desc[0] != '\0' )
      obj->action_desc = QUICKLINK( action_desc );
   obj->owner = NULL;
   obj->buyer = NULL;
   obj->seller = NULL;
   obj->bid = 0;
   obj->socket[0] = QUICKLINK( socket[0] );
   obj->socket[1] = QUICKLINK( socket[1] );
   obj->socket[2] = QUICKLINK( socket[2] );
   obj->item_type = item_type;
   obj->extra_flags = extra_flags;
   obj->wear_flags = wear_flags;
   for( int x = 0; x < MAX_OBJ_VALUE; ++x )
      obj->value[x] = value[x];
   obj->weight = weight;
   obj->cost = cost;
   obj->timer = 0;
   if( ego == -2 )   /* Calculate */
      obj->ego = set_ego(  );
   else
      obj->ego = ego;

   obj->mx = -1;
   obj->my = -1;
   obj->wmap = -1;
   obj->day = 0;
   obj->month = 0;
   obj->year = 0;

   /*
    * Mess with object properties.
    */
   switch ( obj->item_type )
   {
      default:
         bug( "%s: vnum %d bad type.", __func__, vnum );
         log_printf( "------------------------>    %d", obj->item_type );
         break;

      case ITEM_TREE:
      case ITEM_LIGHT:
      case ITEM_TREASURE:
      case ITEM_FURNITURE:
      case ITEM_TRASH:
      case ITEM_CONTAINER:
      case ITEM_CAMPGEAR:
      case ITEM_DRINK_CON:
      case ITEM_KEY:
      case ITEM_KEYRING:
      case ITEM_ODOR:
      case ITEM_CLOTHING:
         break;

      case ITEM_COOK:
      case ITEM_FOOD:
         /*
          * optional food condition (rotting food)    -Thoric
          * value1 is the max condition of the food
          * value4 is the optional initial condition
          */
         if( obj->value[4] )
            obj->timer = obj->value[4];
         else
            obj->timer = obj->value[1];
         break;

      case ITEM_BOAT:
      case ITEM_INSTRUMENT:
      case ITEM_CORPSE_NPC:
      case ITEM_CORPSE_PC:
      case ITEM_FOUNTAIN:
      case ITEM_PUDDLE:
      case ITEM_BLOOD:
      case ITEM_BLOODSTAIN:
      case ITEM_SCRAPS:
      case ITEM_PIPE:
      case ITEM_HERB_CON:
      case ITEM_HERB:
      case ITEM_INCENSE:
      case ITEM_FIRE:
      case ITEM_BOOK:
      case ITEM_SWITCH:
      case ITEM_LEVER:
      case ITEM_PULLCHAIN:
      case ITEM_BUTTON:
      case ITEM_RUNE:
      case ITEM_RUNEPOUCH:
      case ITEM_MATCH:
      case ITEM_TRAP:
      case ITEM_MAP:
      case ITEM_PORTAL:
      case ITEM_PAPER:
      case ITEM_PEN:
      case ITEM_TINDER:
      case ITEM_LOCKPICK:
      case ITEM_SPIKE:
      case ITEM_DISEASE:
      case ITEM_OIL:
      case ITEM_FUEL:
      case ITEM_QUIVER:
      case ITEM_SHOVEL:
      case ITEM_ORE:
      case ITEM_PIECE:
      case ITEM_JOURNAL:
         break;

      case ITEM_SALVE:
         obj->value[3] = number_fuzzy( obj->value[3] );
         break;

      case ITEM_SCROLL:
         obj->value[0] = number_fuzzy( obj->value[0] );
         break;

      case ITEM_WAND:
      case ITEM_STAFF:
         obj->value[0] = number_fuzzy( obj->value[0] );
         obj->value[1] = number_fuzzy( obj->value[1] );
         obj->value[2] = obj->value[1];
         break;

      case ITEM_WEAPON:
      case ITEM_MISSILE_WEAPON:
      case ITEM_PROJECTILE:
         if( obj->value[1] && obj->value[2] )
            obj->value[2] *= obj->value[1];
         else
         {
            obj->value[1] = number_fuzzy( number_fuzzy( 1 * level / 4 + 2 ) );
            obj->value[2] = number_fuzzy( number_fuzzy( 3 * level / 4 + 6 ) );
         }
         if( obj->value[0] == 0 )
            obj->value[0] = sysdata->initcond;
         if( obj->item_type == ITEM_PROJECTILE )
            obj->value[5] = obj->value[0];
         else
            obj->value[6] = obj->value[0];
         break;

      case ITEM_ARMOR:
         if( obj->value[0] == 0 )
            obj->value[0] = number_fuzzy( level / 4 + 2 );
         if( obj->value[1] == 0 )
            obj->value[1] = obj->value[0];
         break;

      case ITEM_POTION:
      case ITEM_PILL:
         obj->value[0] = number_fuzzy( number_fuzzy( obj->value[0] ) );
         break;

      case ITEM_MONEY:
         obj->value[0] = obj->cost;
         if( obj->value[0] == 0 )
            obj->value[0] = 1;
         break;
   }

   objlist.push_back( obj );
   ++count;
   ++numobjsloaded;
   ++physicalobjects;

   return obj;
}

/* Calculates item ego value for object affects, used for automatic item ego calculation */
int calc_aff_ego( int location, int mod )
{
   int calc = 0, x, minego = ( sysdata->minego * 1000 );

   /*
    * No sense in going through all this for a modifier that does nothing, eh? 
    */
   if( mod == 0 || location == APPLY_NONE )
      return 0;

   /*
    * ooOOooOOooOOoo, yes, I know, another awful looking switch! :P
    * Don't change those multipliers for mods of < 0 either, since the mod itself is negative,
    * it will produce the desired affect of returning a negative ego value.
    */
   switch ( location )
   {
      default:
         return 0;

      case APPLY_STR:
      case APPLY_DEX:
         if( mod < 0 )
            return ( 5000 * mod );
         else
            return ( 6000 * mod );

      case APPLY_INT:
      case APPLY_WIS:
         if( mod < 0 )
            return ( 3000 * mod );
         else
            return ( 4000 * mod );

      case APPLY_CON:
         if( mod < 0 )
            return ( 4000 * mod );
         else
            return ( 5000 * mod );

      case APPLY_AGE:
         return ( 1000 * ( mod / 20 ) );

      case APPLY_MANA:
         if( mod > 75 )
            return -2;
         if( mod > 20 && mod <= 75 )
            return ( 2000 * mod );
         return ( UMAX( minego * -1, 1000 * mod ) );

      case APPLY_HIT:
         if( mod > 50 )
            return -2;
         if( mod > 20 && mod <= 50 )
            return ( 3000 * mod );
         if( mod > 10 && mod <= 20 )
            return ( 2000 * mod );
         return ( UMAX( minego * -1, 1000 * mod ) );

      case APPLY_MOVE:
         if( mod < 0 )
            return ( 1000 * ( mod / 10 ) );
         else
            return ( 1000 * ( mod / 5 ) );

      case APPLY_AC:
         if( mod > 12 )
            return -2;
         else
            return ( -1000 * mod ); /* Negative AC is good.... */

      case APPLY_BACKSTAB:
      case APPLY_GOUGE:
      case APPLY_DISARM:
      case APPLY_BASH:
      case APPLY_STUN:
      case APPLY_GRIP:
         if( mod > 10 )
            return -2;
         else
            return ( 1000 * mod );

      case APPLY_SF:   /* spellfail */
         if( mod < -25 )
            return -2;
         else
            return ( -1000 * mod ); /* Not a typo */

      case APPLY_KICK:
      case APPLY_PUNCH:
         if( mod > 20 )
            return -2;
         else
            return ( 1000 * mod );

      case APPLY_HIT_REGEN:
      case APPLY_MANA_REGEN:
         if( mod > 25 )
            return -2;
         else
            return ( 1000 * mod );

      case APPLY_HITROLL:
         if( mod > 6 )
            return -2;
         else
            return ( 2000 * mod );

      case APPLY_DODGE:
      case APPLY_PARRY:
         if( mod > 5 )
            return -2;
         else
            return ( 2000 * mod );

      case APPLY_SCRIBE:
      case APPLY_BREW:
         if( mod > 10 )
            return -2;
         else
            return ( 2000 * mod );

      case APPLY_DAMROLL:
         if( mod > 6 )
            return -2;
         else
            return ( 3000 * mod );

      case APPLY_HITNDAM:
         if( mod > 6 )
            return -2;
         else
            return ( 5000 * mod );

      case APPLY_SAVING_ALL:
         return ( -10000 * mod );   /* Not a typo */

      case APPLY_ATTACKS:
      case APPLY_ALLSTATS:
         return ( 20000 * mod );

      case APPLY_SAVING_POISON:
      case APPLY_SAVING_ROD:
      case APPLY_SAVING_PARA:
      case APPLY_SAVING_BREATH:
      case APPLY_SAVING_SPELL:
         if( mod < 0 )
            return ( -3000 * mod ); /* No, this wasn't a typo */
         else
            return ( -2000 * mod ); /* No, this wasn't a typo either */

      case APPLY_CHA:
         if( mod < 0 )
            return ( 2000 * mod );
         else
            return ( 3000 * mod );

      case APPLY_AFFECT:  /* Otta be some way to fiddle here too */
         return 0;

      case APPLY_RESISTANT:
         for( x = 0; x < MAX_RIS_FLAG; ++x )
            if( IS_SET( mod, 1 << x ) )
               calc += 20000;
         if( calc > 40000 )
            return -2;
         else
            return calc;

      case APPLY_IMMUNE:
      case APPLY_STRIPSN:
         return -1;

      case APPLY_SUSCEPTIBLE:
         for( x = 0; x < MAX_RIS_FLAG; ++x )
            if( IS_SET( mod, 1 << x ) )
               ++calc;
         if( calc == 1 )
            return -15000;
         else if( calc == 2 )
            return -25000;
         else
            return 0;

      case APPLY_ABSORB:
         for( x = 0; x < MAX_RIS_FLAG; ++x )
            if( IS_SET( mod, 1 << x ) )
               ++calc;
         if( calc == 1 )
            return 50000;
         else if( calc > 1 )
            return -2;
         else
            return 0;

      case APPLY_WEAPONSPELL:
      case APPLY_WEARSPELL:
      case APPLY_REMOVESPELL:
      case APPLY_EAT_SPELL:
         if( IS_VALID_SN( mod ) )
            return ( skill_table[mod]->ego * 1000 );
         else
            return 0;

      case APPLY_LCK:
         if( mod < 0 )
            return ( 4000 * mod );
         else
            return ( 5000 * mod );

      case APPLY_PICK:
      case APPLY_TRACK:
      case APPLY_STEAL:
      case APPLY_SNEAK:
      case APPLY_HIDE:
      case APPLY_DETRAP:
      case APPLY_SEARCH:
      case APPLY_CLIMB:
      case APPLY_DIG:
      case APPLY_COOK:
      case APPLY_PEEK:
      case APPLY_EXTRAGOLD:
         if( mod > 20 )
            return -2;
         else
            return ( 1000 * ( mod / 2 ) );

      case APPLY_MOUNT:
         if( mod > 50 )
            return -2;
         else
            return ( 1000 * ( mod / 5 ) );

      case APPLY_MOVE_REGEN:
         if( mod > 50 )
            return -2;
         else
            return ( 1000 * ( mod / 5 ) );
   }
}

int obj_index::set_ego(  )
{
   list < affect_data * >::iterator paf;
   int oego = 0, calc = 0, minego = ( sysdata->minego * 1000 );

   if( extra_flags.test( ITEM_DEATHROT ) )
      return 0;

   for( paf = affects.begin(  ); paf != affects.end(  ); ++paf )
   {
      affect_data *af = *paf;

      calc = calc_aff_ego( af->location, af->modifier );
      if( calc == -2 )
      {
         switch ( af->location )
         {
            default:
               log_printf( "%s: Affect %s exceeds maximum item ego specs. Object %d", __func__, a_types[af->location], vnum );
               break;

            case APPLY_WEAPONSPELL:
            case APPLY_WEARSPELL:
            case APPLY_REMOVESPELL:
            case APPLY_EAT_SPELL:
               log_printf( "%s: Item spell %s exceeds allowable item ego specs. Object %d", __func__,
                           IS_VALID_SN( af->modifier ) ? skill_table[af->modifier]->name : "unknown", vnum );
               break;

            case APPLY_RESISTANT:
            case APPLY_IMMUNE:
            case APPLY_SUSCEPTIBLE:
            case APPLY_ABSORB:
               log_printf( "%s: Item RISA flags exceed allowable item ego specs. Object %d", __func__, vnum );
               break;
         }
         return -2;
      }
      if( calc == -1 )
         return -1;
      oego += calc;
   }
   if( item_type == ITEM_WEAPON || item_type == ITEM_MISSILE_WEAPON || item_type == ITEM_PROJECTILE )
   {
      calc = ( value[1] + value[2] ) / 2;
      if( calc == 15 || calc == 16 )
         oego += 20000;
      else if( calc > 16 )
         oego += 25000;
   }

   if( oego >= minego - 5000 && oego <= minego - 1 )
      oego = minego - 1;

   if( oego < 0 )
      oego = 0;

   // The old item_ego function has been folded into the following checks
   if( oego < minego - 5000 )
      return 0;

   if( hasname( name, "scroll" ) || hasname( name, "potion" ) || hasname( name, "bag" ) || hasname( name, "key" ) )
   {
      return 0;
   }

   if( oego > 90000 )
      oego = 90;
   else
      oego /= 1000;

   return oego;
}

/*
 * Create a new INDEX object (for online building)		-Thoric
 * Option to clone an existing index object.
 */
obj_index *make_object( int vnum, int cvnum, const string & name, area_data * area )
{
   obj_index *cObjIndex = NULL;

   if( cvnum > 0 )
      cObjIndex = get_obj_index( cvnum );

   obj_index *pObjIndex = new obj_index;

   pObjIndex->vnum = vnum;
   pObjIndex->name = STRALLOC( name.c_str(  ) );
   pObjIndex->affects.clear(  );
   pObjIndex->extradesc.clear(  );
   pObjIndex->area = area;
   pObjIndex->count = 0;

   if( !cObjIndex )
   {
      stralloc_printf( &pObjIndex->short_descr, "A newly created %s", name.c_str(  ) );
      stralloc_printf( &pObjIndex->objdesc, "Some god dropped a newly created %s here.", name.c_str(  ) );
      pObjIndex->socket[0] = STRALLOC( "None" );
      pObjIndex->socket[1] = STRALLOC( "None" );
      pObjIndex->socket[2] = STRALLOC( "None" );
      pObjIndex->short_descr[0] = LOWER( pObjIndex->short_descr[0] );
      pObjIndex->objdesc[0] = UPPER( pObjIndex->objdesc[0] );
      pObjIndex->item_type = ITEM_TRASH;
      pObjIndex->extra_flags.reset(  );
      pObjIndex->extra_flags.set( ITEM_PROTOTYPE );
      pObjIndex->wear_flags.reset(  );
      for( int x = 0; x < MAX_OBJ_VALUE; ++x )
         pObjIndex->value[x] = 0;
      pObjIndex->weight = 1;
      pObjIndex->cost = 0;
      pObjIndex->ego = -2;
      pObjIndex->limit = 9999;
   }
   else
   {
      pObjIndex->short_descr = QUICKLINK( cObjIndex->short_descr );
      if( cObjIndex->objdesc && cObjIndex->objdesc[0] != '\0' )
         pObjIndex->objdesc = QUICKLINK( cObjIndex->objdesc );
      if( cObjIndex->action_desc && cObjIndex->action_desc[0] != '\0' )
         pObjIndex->action_desc = QUICKLINK( cObjIndex->action_desc );
      pObjIndex->socket[0] = QUICKLINK( cObjIndex->socket[0] );
      pObjIndex->socket[1] = QUICKLINK( cObjIndex->socket[1] );
      pObjIndex->socket[2] = QUICKLINK( cObjIndex->socket[2] );
      pObjIndex->item_type = cObjIndex->item_type;
      pObjIndex->extra_flags = cObjIndex->extra_flags;
      pObjIndex->extra_flags.set( ITEM_PROTOTYPE );
      pObjIndex->wear_flags = cObjIndex->wear_flags;
      for( int x = 0; x < MAX_OBJ_VALUE; ++x )
         pObjIndex->value[x] = cObjIndex->value[x];
      pObjIndex->weight = cObjIndex->weight;
      pObjIndex->cost = cObjIndex->cost;
      pObjIndex->ego = cObjIndex->ego;
      pObjIndex->limit = cObjIndex->limit;
      pObjIndex->extradesc = cObjIndex->extradesc;

      list < extra_descr_data * >::iterator ced;
      for( ced = cObjIndex->extradesc.begin(  ); ced != cObjIndex->extradesc.end(  ); ++ced )
      {
         extra_descr_data *ed = *ced;
         extra_descr_data *ned = new extra_descr_data;

         ned->keyword = ed->keyword;
         ned->desc = ed->desc;
         pObjIndex->extradesc.push_back( ned );
         ++top_ed;
      }

      list < affect_data * >::iterator cpaf;
      for( cpaf = cObjIndex->affects.begin(  ); cpaf != cObjIndex->affects.end(  ); ++cpaf )
      {
         affect_data *af = *cpaf;
         affect_data *paf = new affect_data;

         paf->type = af->type;
         paf->duration = af->duration;
         paf->location = af->location;
         paf->modifier = af->modifier;
         paf->bit = af->bit;
         paf->rismod = af->rismod;
         pObjIndex->affects.push_back( paf );
         ++top_affect;
      }
   }

   obj_index_table.insert( map < int, obj_index * >::value_type( vnum, pObjIndex ) );
   area->objects.push_back( pObjIndex );
   ++top_obj_index;

   return pObjIndex;
}

/* This procedure is responsible for reading any in_file OBJprograms.
 */
void obj_index::oprog_read_programs( FILE * fp )
{
   mud_prog_data *mprg;
   char letter;
   const char *word;

   for( ;; )
   {
      letter = fread_letter( fp );

      if( letter == '|' )
         return;

      if( letter != '>' )
      {
         bug( "%s: vnum %d MUDPROG char", __func__, vnum );
         exit( 1 );
      }
      mprg = new mud_prog_data;
      mudprogs.push_back( mprg );

      word = fread_word( fp );
      mprg->type = mprog_name_to_type( word );

      switch ( mprg->type )
      {
         case ERROR_PROG:
            bug( "%s: vnum %d MUDPROG type.", __func__, vnum );
            exit( 1 );

         case IN_FILE_PROG:
            mprg->arglist = fread_string( fp );
            mprg->fileprog = false;
            mprog_file_read( this, mprg->arglist );
            break;

         default:
            progtypes.set( mprg->type );
            mprg->fileprog = false;
            mprg->arglist = fread_string( fp );
            mprg->comlist = fread_string( fp );
            break;
      }
   }
}

CMDF( do_ofind )
{
   map < int, obj_index * >::iterator iobj = obj_index_table.begin(  );
   bool fAll = !str_cmp( argument, "all" );
   int nMatch = 0;

   ch->set_pager_color( AT_PLAIN );

   if( argument.empty(  ) )
   {
      ch->print( "Find what?\r\n" );
      return;
   }

   while( iobj != obj_index_table.end(  ) )
   {
      obj_index *pObjIndex = iobj->second;

      if( fAll || hasname( pObjIndex->name, argument ) )
      {
         ++nMatch;
         ch->pagerf( "[%5d] %s\r\n", pObjIndex->vnum, capitalize( pObjIndex->short_descr ) );
      }
      ++iobj;
   }

   if( nMatch )
      ch->pagerf( "Number of matches: %d\n", nMatch );
   else
      ch->print( "Nothing like that in hell, earth, or heaven.\r\n" );
}

CMDF( do_odelete )
{
   obj_index *obj;
   int vnum;

   if( ch->substate == SUB_RESTRICTED )
   {
      ch->print( "You can't do that while in a subprompt.\r\n" );
      return;
   }

   if( argument.empty(  ) )
   {
      ch->print( "Delete which object?\r\n" );
      return;
   }

   if( !is_number( argument ) )
   {
      ch->print( "You must specify the object's vnum to delete it.\r\n" );
      return;
   }

   vnum = atoi( argument.c_str(  ) );

   /*
    * Find the obj. 
    */
   if( !( obj = get_obj_index( vnum ) ) )
   {
      ch->print( "No such object.\r\n" );
      return;
   }

   /*
    * Does the player have the right to delete this object? 
    */
   if( ch->get_trust(  ) < sysdata->level_modify_proto && ( obj->vnum < ch->pcdata->low_vnum || obj->vnum > ch->pcdata->hi_vnum ) )
   {
      ch->print( "That object is not in your assigned range.\r\n" );
      return;
   }
   deleteptr( obj );
   ch->printf( "Object %d has been deleted.\r\n", vnum );
}
