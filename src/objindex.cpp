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
 *                        Object Index Support Functions                    *
 ****************************************************************************/

#include "mud.h"
#include "area.h"
#include "mud_prog.h"
#include "objindex.h"
#include "smaugaffect.h"

std::map<int, obj_index *> obj_index_table;

extern int top_affect;

obj_index::~obj_index(  )
{
   area->objects.remove( this );

   /*
    * Remove references to object index 
    */
   for( auto it = objlist.begin(  ); it != objlist.end(  ); )
   {
      obj_data *o = *it;
      ++it;

      if( o->pIndexData == this )
         o->extract(  );
   }

   for( auto* ch : pclist )
   {
      if( ch->substate == SUB_OBJ_EXTRA && ch->pcdata->dest_buf )
      {
         for( auto* ed : extradesc )
         {
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
         for( auto* mp : mudprogs )
         {
            if( mp == ch->pcdata->dest_buf )
            {
               ch->print( "You suddenly forget which object you were working on.\r\n" );
               ch->stop_editing(  );
               ch->pcdata->dest_buf = nullptr;
               ch->substate = SUB_NONE;
               break;
            }
         }
      }
   }

   for( auto it = extradesc.begin(  ); it != extradesc.end(  ); )
   {
      extra_descr_data *desc = *it;
      ++it;

      extradesc.remove( desc );
      deleteptr( desc );
      --top_ed;
   }
   extradesc.clear(  );

   for( auto it = affects.begin(  ); it != affects.end(  ); )
   {
      affect_data *aff = *it;
      ++it;

      affects.remove( aff );
      deleteptr( aff );
      --top_affect;
   }
   affects.clear(  );

   for( auto it = mudprogs.begin(  ); it != mudprogs.end(  ); )
   {
      mud_prog_data *mprog = *it;
      ++it;

      mudprogs.remove( mprog );
      deleteptr( mprog );
   }
   mudprogs.clear(  );

   std::map<int, obj_index *>::iterator mobj;
   if( ( mobj = obj_index_table.find( vnum ) ) != obj_index_table.end(  ) )
      obj_index_table.erase( mobj );
   --top_obj_index;
}

obj_index::obj_index(  )
{
}

/*
 * clean out an object (index) (leave list pointers intact ) - Thoric
 */
void obj_index::clean_obj(  )
{
   name.clear();
   short_descr.clear();
   objdesc.clear();
   action_desc.clear();
   socket[0].clear();
   socket[1].clear();
   socket[2].clear();
   item_type = 0;
   extra_flags.reset(  );
   wear_flags.reset(  );
   count = 0;
   weight = 0;
   cost = 0;
   level = 1;
   for( int x = 0; x < MAX_OBJ_VALUE; ++x )
      value[x] = 0;

   for( auto it = affects.begin(  ); it != affects.end(  ); )
   {
      affect_data *aff = *it;
      ++it;

      affects.remove( aff );
      deleteptr( aff );
      --top_affect;
   }
   affects.clear(  );

   /*
    * remove extra descriptions 
    */
   for( auto it = extradesc.begin(  ); it != extradesc.end(  ); )
   {
      extra_descr_data *desc = *it;
      ++it;

      extradesc.remove( desc );
      deleteptr( desc );
      --top_ed;
   }
   extradesc.clear(  );

   for( auto it = mudprogs.begin(  ); it != mudprogs.end(  ); )
   {
      mud_prog_data *mprog = *it;
      ++it;

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
   std::map<int, obj_index *>::iterator iobj;

   if( vnum < 0 )
      vnum = 0;

   if( ( iobj = obj_index_table.find( vnum ) ) != obj_index_table.end(  ) )
      return iobj->second;

   if( fBootDb )
      bug( "{}: bad vnum {}.", __func__, vnum );

   return nullptr;
}

/*
 * Create an instance of an object.
 */
obj_data *obj_index::create_object( int olevel )
{
   obj_data *obj;

   obj = new obj_data;
   obj->pIndexData = this;

   obj->level = olevel;
   obj->name = this->name;
   if( !this->short_descr.empty() )
      obj->short_descr = this->short_descr;
   if( !this->objdesc.empty() )
      obj->objdesc = this->objdesc;
   if( !this->action_desc.empty() )
      obj->action_desc = this->action_desc;
   obj->socket[0] = this->socket[0];
   obj->socket[1] = this->socket[1];
   obj->socket[2] = this->socket[2];
   obj->item_type = this->item_type;
   obj->extra_flags = this->extra_flags;
   obj->wear_flags = this->wear_flags;
   for( int x = 0; x < MAX_OBJ_VALUE; ++x )
      obj->value[x] = this->value[x];
   obj->weight = this->weight;
   obj->cost = this->cost;

   if( ego == -2 )   /* Calculate */
      obj->ego = set_ego(  );
   else
      obj->ego = this->ego;

   /*
    * Mess with object properties.
    */
   switch ( obj->item_type )
   {
      default:
         bug( "{}: vnum {} bad type.", __func__, vnum );
         log_printf( "------------------------>    {}", obj->item_type );
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
         return ( umax( minego * -1, 1000 * mod ) );

      case APPLY_HIT:
         if( mod > 50 )
            return -2;
         if( mod > 20 && mod <= 50 )
            return ( 3000 * mod );
         if( mod > 10 && mod <= 20 )
            return ( 2000 * mod );
         return ( umax( minego * -1, 1000 * mod ) );

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
   int oego = 0, calc = 0, minego = ( sysdata->minego * 1000 );

   if( extra_flags.test( ITEM_DEATHROT ) )
      return 0;

   for( auto* af : affects )
   {
      calc = calc_aff_ego( af->location, af->modifier );
      if( calc == -2 )
      {
         switch ( af->location )
         {
            default:
               log_printf( "{}: Affect {} exceeds maximum item ego specs. Object {}", __func__, a_types[af->location], vnum );
               break;

            case APPLY_WEAPONSPELL:
            case APPLY_WEARSPELL:
            case APPLY_REMOVESPELL:
            case APPLY_EAT_SPELL:
               log_printf( "{}: Item spell {} exceeds allowable item ego specs. Object {}", __func__,
                           IS_VALID_SN( af->modifier ) ? skill_table[af->modifier]->name : "unknown", vnum );
               break;

            case APPLY_RESISTANT:
            case APPLY_IMMUNE:
            case APPLY_SUSCEPTIBLE:
            case APPLY_ABSORB:
               log_printf( "{}: Item RISA flags exceed allowable item ego specs. Object {}", __func__, vnum );
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
obj_index *make_object( int vnum, int cvnum, const std::string & name, area_data * area )
{
   obj_index *cObjIndex = nullptr;

   if( cvnum > 0 )
      cObjIndex = get_obj_index( cvnum );

   obj_index *pObjIndex = new obj_index;

   pObjIndex->vnum = vnum;
   pObjIndex->name = name;
   pObjIndex->affects.clear(  );
   pObjIndex->extradesc.clear(  );
   pObjIndex->area = area;
   pObjIndex->count = 0;

   if( !cObjIndex )
   {
      pObjIndex->short_descr = std::format( "A newly created {}", name );
      pObjIndex->objdesc = std::format( "Some god dropped a newly created {} here.", name );
      pObjIndex->socket[0] = "None";
      pObjIndex->socket[1] = "None";
      pObjIndex->socket[2] = "None";
      pObjIndex->short_descr[0] = to_lower( pObjIndex->short_descr[0] );
      pObjIndex->objdesc[0] = to_upper( pObjIndex->objdesc[0] );
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
      pObjIndex->short_descr = cObjIndex->short_descr;
      if( !cObjIndex->objdesc.empty() )
         pObjIndex->objdesc = cObjIndex->objdesc;
      if( !cObjIndex->action_desc.empty() )
         pObjIndex->action_desc = cObjIndex->action_desc;
      pObjIndex->socket[0] = cObjIndex->socket[0];
      pObjIndex->socket[1] = cObjIndex->socket[1];
      pObjIndex->socket[2] = cObjIndex->socket[2];
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

      for( auto* ed : cObjIndex->extradesc )
      {
         extra_descr_data *ned = new extra_descr_data;

         ned->keyword = ed->keyword;
         ned->desc = ed->desc;
         pObjIndex->extradesc.push_back( ned );
         ++top_ed;
      }

      for( auto* af : cObjIndex->affects )
      {
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

   obj_index_table.insert( std::map<int, obj_index *>::value_type( vnum, pObjIndex ) );
   area->objects.push_back( pObjIndex );
   ++top_obj_index;

   return pObjIndex;
}

/* This procedure is responsible for reading any in_file OBJprograms.
 */
void obj_index::oprog_read_programs( FILE * fp )
{
   for( ;; )
   {
      char letter = fread_letter( fp );

      if( letter == '|' )
         return;

      if( letter != '>' )
      {
         bug( "{}: vnum {} MUDPROG char", __func__, vnum );
         std::exit( EXIT_FAILURE );
      }
      mud_prog_data *mprg = new mud_prog_data;
      mudprogs.push_back( mprg );

      std::string word = fread_word( fp );
      mprg->type = mprog_name_to_type( word );

      switch ( mprg->type )
      {
         case ERROR_PROG:
            bug( "{}: vnum {} MUDPROG type.", __func__, vnum );
            std::exit( EXIT_FAILURE );

         case IN_FILE_PROG:
            fread_string( mprg->arglist, fp );
            mprg->fileprog = false;
            mprog_file_read( this, mprg->arglist );
            break;

         default:
            progtypes.set( mprg->type );
            mprg->fileprog = false;
            fread_string( mprg->arglist, fp );
            fread_string( mprg->comlist, fp );
            break;
      }
   }
}

CMDF( do_ofind )
{
   std::map<int, obj_index *>::iterator iobj = obj_index_table.begin(  );
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
         ch->pager_fmt( "[{:5}] {}\r\n", pObjIndex->vnum, capitalize( pObjIndex->short_descr ) );
      }
      ++iobj;
   }

   if( nMatch )
      ch->pager_fmt( "Number of matches: {}\n", nMatch );
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

   vnum = stoi( argument );

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
   if( ch->get_trust(  ) < sysdata->level_modify_proto && ch->pcdata->area != obj->area )
   {
      ch->print( "That object is not in your assigned range.\r\n" );
      return;
   }

   deleteptr( obj );
   ch->printf( "Object %d has been deleted.\r\n", vnum );
}
