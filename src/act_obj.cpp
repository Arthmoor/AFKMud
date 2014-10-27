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
 *                       Object manipulation module                         *
 ****************************************************************************/

#include "mud.h"
#include "clans.h"
#include "deity.h"
#include "mud_prog.h"
#include "objindex.h"
#include "roomindex.h"

void check_clan_storeroom( char_data * );
void check_clan_shop( char_data *, char_data *, obj_data * );
obj_data *create_money( int );

void get_obj( char_data * ch, obj_data * obj, obj_data * container )
{
   int weight, amt;  /* gold per-race multipliers */

   if( !obj->wear_flags.test( ITEM_TAKE ) && ( ch->level < sysdata->level_getobjnotake ) )
   {
      ch->print( "You can't take that.\r\n" );
      return;
   }

   if( obj->extra_flags.test( ITEM_PROTOTYPE ) && !ch->can_take_proto(  ) )
   {
      ch->print( "A godly force prevents you from getting close to it.\r\n" );
      return;
   }

   if( ch->carry_number + obj->get_number(  ) > ch->can_carry_n(  ) )
   {
      act( AT_PLAIN, "$d: you can't carry that many items.", ch, NULL, obj->short_descr, TO_CHAR );
      return;
   }

   if( obj->extra_flags.test( ITEM_COVERING ) )
      weight = obj->weight;
   else
      weight = obj->get_weight(  );

   /*
    * Money weight shouldn't count 
    */
   if( obj->item_type != ITEM_MONEY )
   {
      if( obj->in_obj )
      {
         obj_data *tobj = obj->in_obj;
         int inobj = 1;
         bool checkweight = false;

         /*
          * need to make it check weight if its in a magic container 
          */
         if( tobj->item_type == ITEM_CONTAINER && tobj->extra_flags.test( ITEM_MAGIC ) )
            checkweight = true;

         while( tobj->in_obj )
         {
            tobj = tobj->in_obj;
            ++inobj;

            /*
             * need to make it check weight if its in a magic container 
             */
            if( tobj->item_type == ITEM_CONTAINER && tobj->extra_flags.test( ITEM_MAGIC ) )
               checkweight = true;
         }

         /*
          * need to check weight if not carried by ch or in a magic container. 
          */
         if( !tobj->carried_by || tobj->carried_by != ch || checkweight )
         {
            if( ( ch->carry_weight + weight ) > ch->can_carry_w(  ) )
            {
               act( AT_PLAIN, "$d: you can't carry that much weight.", ch, NULL, obj->short_descr, TO_CHAR );
               return;
            }
         }
      }
      else if( ( ch->carry_weight + weight ) > ch->can_carry_w(  ) )
      {
         act( AT_PLAIN, "$d: you can't carry that much weight.", ch, NULL, obj->short_descr, TO_CHAR );
         return;
      }
   }

   if( container )
   {
      if( container->item_type == ITEM_KEYRING && !container->extra_flags.test( ITEM_COVERING ) )
      {
         act( AT_ACTION, "You remove $p from $P", ch, obj, container, TO_CHAR );
         act( AT_ACTION, "$n removes $p from $P", ch, obj, container, TO_ROOM );
      }
      else
      {
         act( AT_ACTION, container->extra_flags.test( ITEM_COVERING ) ? "You get $p from beneath $P." : "You get $p from $P", ch, obj, container, TO_CHAR );
         act( AT_ACTION, container->extra_flags.test( ITEM_COVERING ) ? "$n gets $p from beneath $P." : "$n gets $p from $P", ch, obj, container, TO_ROOM );
      }
      obj->from_obj(  );
   }
   else
   {
      act( AT_ACTION, "You get $p.", ch, obj, container, TO_CHAR );
      act( AT_ACTION, "$n gets $p.", ch, obj, container, TO_ROOM );
      obj->from_room(  );
   }

   /*
    * Clan storeroom checks 
    */
   if( ch->in_room->flags.test( ROOM_CLANSTOREROOM ) && ( !container || container->carried_by == NULL ) )
      check_clan_storeroom( ch );

   if( obj->item_type != ITEM_CONTAINER )
      check_for_trap( ch, obj, TRAP_GET );

   if( ch->char_died(  ) )
      return;

   if( obj->item_type == ITEM_MONEY )
   {
      /*
       * Handle grouping -- noticed by Guy Petty 
       */
      amt = obj->value[0] * obj->count;
      ch->gold += amt;
      obj->extract(  );
      return;
   }
   else
      obj->to_char( ch );

   if( ch->char_died(  ) || obj->extracted(  ) )
      return;
   oprog_get_trigger( ch, obj );
}

CMDF( do_get )
{
   string arg1, arg2;
   obj_data *obj, *container;
   short number = 0;
   bool found;

   argument = one_argument( argument, arg1 );

   if( ch->carry_number < 0 || ch->carry_weight < 0 )
   {
      ch->print( "Uh oh ... better contact an immortal about your number or weight of items carried.\r\n" );
      log_printf( "%s has negative carry_number or carry_weight!", ch->name );
      return;
   }

   if( is_number( arg1 ) )
   {
      number = atoi( arg1.c_str(  ) );
      if( number < 1 )
      {
         ch->print( "That was easy...\r\n" );
         return;
      }
      if( ( ch->carry_number + number ) > ch->can_carry_n(  ) )
      {
         ch->print( "You can't carry that many.\r\n" );
         return;
      }
      argument = one_argument( argument, arg1 );
   }

   argument = one_argument( argument, arg2 );
   /*
    * munch optional words 
    */
   if( !str_cmp( arg2, "from" ) && !argument.empty(  ) )
      argument = one_argument( argument, arg2 );

   /*
    * Get type. 
    */
   if( arg1.empty(  ) )
   {
      ch->print( "Get what?\r\n" );
      return;
   }

   if( ms_find_obj( ch ) )
      return;

   if( arg2.empty(  ) )
   {
      if( number <= 1 && str_cmp( arg1, "all" ) && str_prefix( "all.", arg1 ) )
      {
         /*
          * 'get obj' 
          */
         if( !( obj = get_obj_list( ch, arg1, ch->in_room->objects ) ) )
         {
            ch->printf( "I see no %s here.\r\n", arg1.c_str(  ) );
            return;
         }

         if( ch->char_ego(  ) < obj->ego && obj->ego >= sysdata->minego )
         {
            act( AT_OBJECT, "$P glows brightly and clings tightly to the ground, refusing to be picked up.", ch, NULL, obj, TO_CHAR );
            return;
         }

         obj->separate(  );
         get_obj( ch, obj, NULL );

         if( ch->char_died(  ) )
            return;
         if( IS_SAVE_FLAG( SV_GET ) )
            ch->save(  );
      }
      else
      {
         short cnt = 0;
         bool fAll;
         string chk;

         if( ch->in_room->flags.test( ROOM_DONATION ) )
         {
            ch->print( "The gods frown upon such a display of greed!\r\n" );
            return;
         }
         if( !str_cmp( arg1, "all" ) )
            fAll = true;
         else
            fAll = false;
         if( number > 1 )
            chk = arg1;
         else
         {
            if( str_cmp( arg1, "all" ) )
               chk = arg1.substr( 4, arg1.length(  ) );
            else
               chk = "";
         }

         /*
          * 'get all' or 'get all.obj' 
          */
         found = false;
         list < obj_data * >::iterator iobj;
         for( iobj = ch->in_room->objects.begin(  ); iobj != ch->in_room->objects.end(  ); )
         {
            obj = *iobj;
            ++iobj;

            if( ( fAll || hasname( obj->name, chk ) ) && ch->can_see_obj( obj, false ) )
            {
               if( !is_same_obj_map( ch, obj ) )
               {
                  found = false;
                  continue;
               }

               if( ch->char_ego(  ) < obj->ego && obj->ego >= sysdata->minego )
               {
                  act( AT_OBJECT, "$p glows brightly and clings tightly to the ground, refusing to be picked up.", ch, obj, NULL, TO_CHAR );
                  continue;
               }

               found = true;
               if( number && ( cnt + obj->count ) > number )
                  obj->split( number - cnt );
               cnt += obj->count;
               get_obj( ch, obj, NULL );

               if( ch->char_died(  ) || ch->carry_number >= ch->can_carry_n(  ) || ch->carry_weight >= ch->can_carry_w(  ) || ( number && cnt >= number ) )
               {
                  if( IS_SAVE_FLAG( SV_GET ) && !ch->char_died(  ) )
                     ch->save(  );
                  return;
               }
            }
         }

         if( !found )
         {
            if( fAll )
               ch->print( "I see nothing here.\r\n" );
            else
               ch->printf( "I see no %s here.\r\n", chk.c_str(  ) );
         }
         else if( IS_SAVE_FLAG( SV_GET ) )
            ch->save(  );
      }
   }
   else
   {
      /*
       * 'get ... container' 
       */
      if( !str_cmp( arg2, "all" ) || !str_prefix( "all.", arg2 ) )
      {
         ch->print( "You can't do that.\r\n" );
         return;
      }

      if( !( container = ch->get_obj_here( arg2 ) ) )
      {
         ch->printf( "I see no %s here.\r\n", arg2.c_str(  ) );
         return;
      }

      switch ( container->item_type )
      {
         default:
            if( !container->extra_flags.test( ITEM_COVERING ) )
            {
               ch->print( "That's not a container.\r\n" );
               return;
            }
            if( ch->carry_weight + container->weight > ch->can_carry_w(  ) )
            {
               ch->print( "It's too heavy for you to lift.\r\n" );
               return;
            }
            break;

         case ITEM_CONTAINER:
         case ITEM_CORPSE_NPC:
         case ITEM_KEYRING:
         case ITEM_QUIVER:
            break;

         case ITEM_CORPSE_PC:
         {
            char name[MIL];
            char *pd;

            if( ch->isnpc(  ) )
            {
               ch->print( "You can't do that.\r\n" );
               return;
            }

            pd = container->short_descr;
            pd = one_argument( pd, name );
            pd = one_argument( pd, name );
            pd = one_argument( pd, name );
            pd = one_argument( pd, name );

            if( container->extra_flags.test( ITEM_CLANCORPSE ) && !ch->isnpc(  ) && ( ch->get_timer( TIMER_PKILLED ) > 0 ) && str_cmp( name, ch->name ) )
            {
               ch->print( "You cannot loot from that corpse...yet.\r\n" );
               return;
            }

            /*
             * Killer/owner loot only if die to pkill blow --Blod 
             * Added check for immortal so IMMS can get things out of
             * * corpses --Shaddai 
             */
            if( container->extra_flags.test( ITEM_CLANCORPSE ) && !ch->isnpc(  ) && !ch->is_immortal(  )
                && container->action_desc[0] != '\0' && str_cmp( name, ch->name ) && str_cmp( container->action_desc, ch->name ) )
            {
               ch->print( "You did not inflict the death blow upon this corpse.\r\n" );
               return;
            }

            if( container->extra_flags.test( ITEM_CLANCORPSE ) && ch->has_pcflag( PCFLAG_DEADLY )
                && container->value[4] - ch->level < 6 && container->value[4] - ch->level > -6 )
               break;

            if( str_cmp( name, ch->name ) && !ch->is_immortal(  ) )
            {
               bool fGroup = false;
               list < char_data * >::iterator ich;
               for( ich = pclist.begin(  ); ich != pclist.end(  ); ++ich )
               {
                  char_data *gch = *ich;

                  if( is_same_group( ch, gch ) && !str_cmp( name, gch->name ) )
                  {
                     fGroup = true;
                     break;
                  }
               }

               if( !fGroup )
               {
                  ch->print( "That's someone else's corpse.\r\n" );
                  return;
               }
            }
         }
      }

      if( !container->extra_flags.test( ITEM_COVERING ) && IS_SET( container->value[1], CONT_CLOSED ) )
      {
         act( AT_PLAIN, "The $d is closed.", ch, NULL, container->name, TO_CHAR );
         return;
      }

      if( number <= 1 && str_cmp( arg1, "all" ) && str_prefix( "all.", arg1 ) )
      {
         /*
          * 'get obj container' 
          */
         if( !( obj = get_obj_list( ch, arg1, container->contents ) ) )
         {
            ch->printf( "I see nothing like that %s the %s.\r\n", container->extra_flags.test( ITEM_COVERING ) ? "beneath" : "in", arg2.c_str(  ) );
            return;
         }
         obj->separate(  );
         get_obj( ch, obj, container );
         /*
          * Oops no wonder corpses were duping oopsie did I do that
          * * --Shaddai
          */
         if( container->item_type == ITEM_CORPSE_PC )
            write_corpse( container, container->short_descr + 14 );
         check_for_trap( ch, container, TRAP_GET );
         if( ch->char_died(  ) )
            return;
         if( IS_SAVE_FLAG( SV_GET ) )
            ch->save(  );
      }
      else
      {
         int cnt = 0;
         bool fAll;
         string chk;

         /*
          * 'get all container' or 'get all.obj container' 
          */
         if( container->extra_flags.test( ITEM_CLANCORPSE ) && !ch->is_immortal(  ) && !ch->isnpc(  ) && str_cmp( ch->name, container->name + 7 ) )
         {
            ch->print( "The gods frown upon such wanton greed!\r\n" );
            return;
         }

         if( !str_cmp( arg1, "all" ) )
            fAll = true;
         else
            fAll = false;
         if( number > 1 )
            chk = arg1;
         else
         {
            if( str_cmp( arg1, "all" ) )
               chk = arg1.substr( 4, arg1.length(  ) );
            else
               chk = "";
         }

         found = false;
         list < obj_data * >::iterator iobj;
         for( iobj = container->contents.begin(  ); iobj != container->contents.end(  ); )
         {
            obj = *iobj;
            ++iobj;

            if( ( fAll || hasname( obj->name, chk ) ) && ch->can_see_obj( obj, false ) )
            {
               found = true;
               if( number && ( cnt + obj->count ) > number )
                  obj->split( number - cnt );
               cnt += obj->count;
               get_obj( ch, obj, container );
               if( ch->char_died(  ) || ch->carry_number >= ch->can_carry_n(  ) || ch->carry_weight >= ch->can_carry_w(  ) || ( number && cnt >= number ) )
               {
                  if( container->item_type == ITEM_CORPSE_PC )
                     write_corpse( container, container->short_descr + 14 );
                  if( found && IS_SAVE_FLAG( SV_GET ) )
                     ch->save(  );
                  return;
               }
            }
         }

         if( !found )
         {
            if( fAll )
            {
               if( container->item_type == ITEM_KEYRING && !container->extra_flags.test( ITEM_COVERING ) )
                  ch->printf( "The %s holds no keys.\r\n", arg2.c_str(  ) );
               else
                  ch->printf( "I see nothing %s the %s.\r\n", container->extra_flags.test( ITEM_COVERING ) ? "beneath" : "in", arg2.c_str(  ) );
            }
            else
            {
               if( container->item_type == ITEM_KEYRING && !container->extra_flags.test( ITEM_COVERING ) )
                  ch->printf( "The %s does not hold that key.\r\n", arg2.c_str(  ) );
               else
                  ch->printf( "I see nothing %s the %s.\r\n", container->extra_flags.test( ITEM_COVERING ) ? "beneath" : "in", arg2.c_str(  ) );
            }
         }
         else
            check_for_trap( ch, container, TRAP_GET );
         if( ch->char_died(  ) )
            return;
         /*
          * Oops no wonder corpses were duping oopsie did I do that
          * * --Shaddai
          */
         if( container->item_type == ITEM_CORPSE_PC )
            write_corpse( container, container->short_descr + 14 );
         if( found && IS_SAVE_FLAG( SV_GET ) )
            ch->save(  );
      }
   }
}

CMDF( do_put )
{
   string arg1, arg2;
   obj_data *container, *obj = NULL;
   short count;
   int number = 0;
   bool save_char = false;

   argument = one_argument( argument, arg1 );
   if( is_number( arg1 ) )
   {
      number = atoi( arg1.c_str(  ) );
      if( number < 1 )
      {
         ch->print( "That was easy...\r\n" );
         return;
      }
      argument = one_argument( argument, arg1 );
   }

   argument = one_argument( argument, arg2 );

   /*
    * munch optional words 
    */
   if( ( !str_cmp( arg2, "into" ) || !str_cmp( arg2, "inside" )
         || !str_cmp( arg2, "in" ) || !str_cmp( arg2, "under" ) || !str_cmp( arg2, "onto" ) || !str_cmp( arg2, "on" ) ) && argument[0] != '\0' )
      argument = one_argument( argument, arg2 );

   if( arg1.empty(  ) || arg2.empty(  ) )
   {
      ch->print( "Put what in what?\r\n" );
      return;
   }

   if( ms_find_obj( ch ) )
      return;

   if( !str_cmp( arg2, "all" ) || !str_prefix( "all.", arg2 ) )
   {
      ch->print( "You can't do that.\r\n" );
      return;
   }

   if( !( container = ch->get_obj_here( arg2 ) ) )
   {
      ch->printf( "I see no %s here.\r\n", arg2.c_str(  ) );
      return;
   }

   if( !container->carried_by && IS_SAVE_FLAG( SV_PUT ) )
      save_char = true;

   if( container->extra_flags.test( ITEM_COVERING ) )
   {
      if( ch->carry_weight + container->weight > ch->can_carry_w(  ) )
      {
         ch->print( "It's too heavy for you to lift.\r\n" );
         return;
      }
   }
   else
   {
      if( container->item_type != ITEM_CONTAINER && container->item_type != ITEM_KEYRING && container->item_type != ITEM_QUIVER )
      {
         ch->print( "That's not a container.\r\n" );
         return;
      }

      if( IS_SET( container->value[1], CONT_CLOSED ) )
      {
         ch->printf( "The %s is closed.\r\n", container->name );
         return;
      }
   }

   if( number <= 1 && str_cmp( arg1, "all" ) && str_prefix( "all.", arg1 ) )
   {
      /*
       * 'put obj container' 
       */
      if( !( obj = ch->get_obj_carry( arg1 ) ) )
      {
         ch->print( "You do not have that item.\r\n" );
         return;
      }

      if( obj == container )
      {
         ch->print( "You can't fold it into itself.\r\n" );
         return;
      }

      if( !ch->can_drop_obj( obj ) )
      {
         ch->print( "You can't let go of it.\r\n" );
         return;
      }

      if( container->item_type == ITEM_KEYRING && obj->item_type != ITEM_KEY )
      {
         ch->print( "That's not a key.\r\n" );
         return;
      }

      if( container->item_type == ITEM_QUIVER && obj->item_type != ITEM_PROJECTILE )
      {
         ch->print( "That's not a projectile.\r\n" );
         return;
      }

      // Fix by Luc - Sometime in 2000?
      int tweight = ( container->get_real_weight(  ) / container->count ) + ( obj->get_real_weight(  ) / obj->count );
      if( container->extra_flags.test( ITEM_COVERING ) )
      {
         if( container->item_type == ITEM_CONTAINER ? tweight > container->value[0] : tweight - container->weight > container->weight )
         {
            ch->print( "It won't fit under there.\r\n" );
            return;
         }
      }
      else if( tweight - container->weight > container->value[0] )
      {
         ch->print( "It won't fit.\r\n" );
         return;
      }

      obj->separate(  );
      container->separate(  );
      obj->from_char(  );
      obj = obj->to_obj( container );
      check_for_trap( ch, container, TRAP_PUT );
      if( ch->char_died(  ) )
         return;
      count = obj->count;
      obj->count = 1;
      if( container->item_type == ITEM_KEYRING && !container->extra_flags.test( ITEM_COVERING ) )
      {
         act( AT_ACTION, "$n slips $p onto $P.", ch, obj, container, TO_ROOM );
         act( AT_ACTION, "You slip $p onto $P.", ch, obj, container, TO_CHAR );
      }
      else
      {
         act( AT_ACTION, container->extra_flags.test( ITEM_COVERING ) ? "$n hides $p beneath $P." : "$n puts $p in $P.", ch, obj, container, TO_ROOM );
         act( AT_ACTION, container->extra_flags.test( ITEM_COVERING ) ? "You hide $p beneath $P." : "You put $p in $P.", ch, obj, container, TO_CHAR );
      }
      obj->count = count;

      /*
       * Oops no wonder corpses were duping oopsie did I do that
       * * --Shaddai
       */
      if( container->item_type == ITEM_CORPSE_PC )
         write_corpse( container, container->short_descr + 14 );
      if( save_char )
         ch->save(  );
      /*
       * Clan storeroom check 
       */
      if( ch->in_room->flags.test( ROOM_CLANSTOREROOM ) && container->carried_by == NULL )
      {
         if( obj == NULL )
         {
            bug( "%s: clanstoreroom check: Object is NULL!", __FUNCTION__ );
            return;
         }
         if( obj->ego < sysdata->minego )
            check_clan_storeroom( ch );
         else
         {
            ch->printf( "%s is a rare item and therefore cannot be stored here.\r\n", obj->short_descr );
            obj->from_obj(  );
            obj->to_char( ch );
         }
      }
   }
   else
   {
      bool found = false;
      int cnt = 0;
      bool fAll;
      string chk;

      if( !str_cmp( arg1, "all" ) )
         fAll = true;
      else
         fAll = false;
      if( number > 1 )
         chk = arg1;
      else
      {
         if( str_cmp( arg1, "all" ) )
            chk = arg1.substr( 4, arg1.length(  ) );
         else
            chk = "";
      }

      container->separate(  );
      /*
       * 'put all container' or 'put all.obj container' 
       */
      list < obj_data * >::iterator iobj;
      for( iobj = ch->carrying.begin(  ); iobj != ch->carrying.end(  ); )
      {
         obj = *iobj;
         ++iobj;

         if( ( fAll || hasname( obj->name, chk ) )
             && ch->can_see_obj( obj, false ) && obj->wear_loc == WEAR_NONE && obj != container
             && ch->can_drop_obj( obj ) && ( container->item_type != ITEM_KEYRING || obj->item_type == ITEM_KEY )
             && ( container->item_type != ITEM_QUIVER || obj->item_type == ITEM_PROJECTILE ) && obj->get_weight(  ) + container->get_weight(  ) <= container->value[0] )
         {
            if( number && ( cnt + obj->count ) > number )
               obj->split( number - cnt );
            cnt += obj->count;
            obj->from_char(  );
            if( container->item_type == ITEM_KEYRING )
            {
               act( AT_ACTION, "$n slips $p onto $P.", ch, obj, container, TO_ROOM );
               act( AT_ACTION, "You slip $p onto $P.", ch, obj, container, TO_CHAR );
            }
            else
            {
               act( AT_ACTION, "$n puts $p in $P.", ch, obj, container, TO_ROOM );
               act( AT_ACTION, "You put $p in $P.", ch, obj, container, TO_CHAR );
            }
            obj = obj->to_obj( container );
            found = true;

            check_for_trap( ch, container, TRAP_PUT );
            if( ch->char_died(  ) )
               return;
            if( number && cnt >= number )
               break;
         }
      }

      /*
       * Don't bother to save anything if nothing was dropped - Thoric
       */
      if( !found )
      {
         if( fAll )
            ch->print( "You are not carrying anything.\r\n" );
         else
            ch->printf( "You are not carrying any %s.\r\n", chk.c_str(  ) );
         return;
      }

      if( save_char )
         ch->save(  );
      /*
       * Oops no wonder corpses were duping oopsie did I do that
       * * --Shaddai
       */
      if( container->item_type == ITEM_CORPSE_PC )
         write_corpse( container, container->short_descr + 14 );

      /*
       * Clan storeroom check 
       */
      if( ch->in_room->flags.test( ROOM_CLANSTOREROOM ) && container->carried_by == NULL )
      {
         if( obj == NULL )
         {
            bug( "%s: clanstoreroom check: Object is NULL!", __FUNCTION__ );
            return;
         }
         if( obj->ego < sysdata->minego )
            check_clan_storeroom( ch );
         else
         {
            ch->printf( "%s is a rare item and therefore cannot be stored here.\r\n", obj->short_descr );
            obj->from_obj(  );
            obj->to_char( ch );
         }
      }
   }
}

CMDF( do_drop )
{
   string arg;
   obj_data *obj;
   bool found;
   int number = 0;

   argument = one_argument( argument, arg );
   if( is_number( arg ) )
   {
      number = atoi( arg.c_str(  ) );
      if( number < 1 )
      {
         ch->print( "That was easy...\r\n" );
         return;
      }
      argument = one_argument( argument, arg );
   }

   if( arg.empty(  ) )
   {
      ch->print( "Drop what?\r\n" );
      return;
   }

   if( ms_find_obj( ch ) )
      return;

   if( ch->has_pcflag( PCFLAG_LITTERBUG ) )
   {
      ch->print( "&[immortal]A godly force prevents you from dropping anything...\r\n" );
      return;
   }

   if( ch->in_room->flags.test( ROOM_NODROP ) && ch != supermob )
   {
      ch->print( "&[magic]A magical force stops you!\r\n" );
      ch->print( "&[tell]Someone tells you, 'No littering here!'\r\n" );
      return;
   }

   list < obj_data * >::iterator iobj;
   if( number > 0 )
   {
      /*
       * 'drop NNNN coins' 
       */
      if( !str_cmp( arg, "coins" ) || !str_cmp( arg, "coin" ) )
      {
         if( ch->gold < number )
         {
            ch->print( "You haven't got that many coins.\r\n" );
            return;
         }

         ch->gold -= number;

         for( iobj = ch->in_room->objects.begin(  ); iobj != ch->in_room->objects.end(  ); )
         {
            obj = *iobj;
            ++iobj;

            switch ( obj->pIndexData->vnum )
            {
               default:
               case OBJ_VNUM_MONEY_ONE:
                  number += 1;
                  obj->extract(  );
                  break;

               case OBJ_VNUM_MONEY_SOME:
                  number += obj->value[0];
                  obj->extract(  );
                  break;
            }
         }

         act( AT_ACTION, "$n drops some gold.", ch, NULL, NULL, TO_ROOM );
         create_money( number )->to_room( ch->in_room, ch );
         ch->print( "You let the gold slip from your hand.\r\n" );
         if( IS_SAVE_FLAG( SV_DROP ) )
            ch->save(  );
         return;
      }
   }

   if( number <= 1 && str_cmp( arg, "all" ) && str_prefix( "all.", arg ) )
   {
      /*
       * 'drop obj' 
       */
      if( !( obj = ch->get_obj_carry( arg ) ) )
      {
         ch->print( "You do not have that item.\r\n" );
         return;
      }

      if( !ch->can_drop_obj( obj ) )
      {
         ch->print( "You can't let go of it.\r\n" );
         return;
      }

      obj->separate(  );
      act( AT_ACTION, "$n drops $t.", ch, aoran( obj->short_descr ), NULL, TO_ROOM );
      act( AT_ACTION, "You drop $t.", ch, aoran( obj->short_descr ), NULL, TO_CHAR );

      obj->from_char(  );
      obj->to_room( ch->in_room, ch );

      oprog_drop_trigger( ch, obj );   /* mudprogs */

      if( ch->char_died(  ) || obj->extracted(  ) )
         return;

      if( ch->in_room->flags.test( ROOM_DONATION ) )
         obj->extra_flags.set( ITEM_DONATION );

      /*
       * For when an immortal or anyone else moves a corpse, since picking it up deletes the corpse save - Samson 
       */
      if( obj->item_type == ITEM_CORPSE_PC )
         write_corpse( obj, obj->short_descr + 14 );

      /*
       * Clan storeroom saving 
       */
      if( ch->in_room->flags.test( ROOM_CLANSTOREROOM ) )
      {
         if( obj->ego < sysdata->minego )
            check_clan_storeroom( ch );
         else
         {
            ch->printf( "%s is a rare item and cannot be stored here.\r\n", obj->short_descr );
            obj->from_room(  );
            obj->to_char( ch );
         }
      }
   }
   else
   {
      int cnt = 0;
      string chk;
      bool fAll;

      if( !str_cmp( arg, "all" ) )
         fAll = true;
      else
         fAll = false;
      if( number > 1 )
         chk = arg;
      else
      {
         if( str_cmp( arg, "all" ) )
            chk = arg.substr( 4, arg.length(  ) );
         else
            chk = "";
      }

      /*
       * 'drop all' or 'drop all.obj' 
       */
      if( ch->in_room->flags.test( ROOM_NODROPALL ) )
      {
         ch->print( "You can't seem to do that here...\r\n" );
         return;
      }
      found = false;
      list < obj_data * >droplist;
      droplist.clear(  );
      for( iobj = ch->carrying.begin(  ); iobj != ch->carrying.end(  ); )
      {
         obj = *iobj;
         ++iobj;

         if( ( fAll || hasname( obj->name, chk ) ) && ch->can_see_obj( obj, false ) && obj->wear_loc == WEAR_NONE && ch->can_drop_obj( obj ) )
         {
            found = true;
            if( HAS_PROG( obj->pIndexData, DROP_PROG ) && obj->count > 1 )
            {
               ++cnt;
               obj->separate(  );
               obj->from_char(  );
            }
            else
            {
               if( number && ( cnt + obj->count ) > number )
                  obj->split( number - cnt );
               cnt += obj->count;
               obj->from_char(  );
            }
            /*
             * Edited by Whir for being too spammy (see above)- 1/29/98 
             */
            obj->to_room( ch->in_room, ch );

            if( ch->in_room->flags.test( ROOM_DONATION ) )
               obj->extra_flags.set( ITEM_DONATION );

            if( ch->in_room->flags.test( ROOM_CLANSTOREROOM ) )
            {
               if( obj->ego < sysdata->minego )
                  check_clan_storeroom( ch );
               else
               {
                  ch->printf( "%s is a rare item and cannot be stored here.\r\n", obj->short_descr );
                  droplist.push_back( obj );
                  continue;
               }
            }
            oprog_drop_trigger( ch, obj );   /* mudprogs */
            if( ch->char_died(  ) )
               return;
            if( number && cnt >= number )
               break;
         }
      }

      if( !found )
      {
         if( fAll )
            ch->print( "You are not carrying anything.\r\n" );
         else
            ch->printf( "You are not carrying any %s.\r\n", chk.c_str(  ) );
      }
      else
      {
         if( fAll )
         {
            /*
             * Added by Whir - 1/28/98 
             */
            act( AT_ACTION, "$n drops everything $e owns.", ch, NULL, NULL, TO_ROOM );
            ch->print( "&[action]You drop everything you own.\r\n" );

            // Drop all command with rare items in it will spawn an evil infinite loop so we handle that here now.
            if( droplist.size(  ) > 0 )
            {
               list < obj_data * >::iterator dobj;

               for( dobj = droplist.begin(  ); dobj != droplist.end(  ); )
               {
                  obj_data *drop = *dobj;
                  ++dobj;

                  drop->from_room(  );
                  drop->to_char( ch );
               }
               droplist.clear(  );
            }
         }
         else
         {
            ch->printf( "&[action]You drop every %s you own.\r\n", chk.c_str(  ) );
            act( AT_ACTION, "$n drops every $t $e owns.", ch, chk.c_str(  ), NULL, TO_ROOM );
         }
      }
   }
   if( IS_SAVE_FLAG( SV_DROP ) )
      ch->save(  );  /* duping protector */
}

CMDF( do_give )
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
      ch->print( "Give what to whom?\r\n" );
      return;
   }

   if( ms_find_obj( ch ) )
      return;

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
         ch->print( "Give what to whom?\r\n" );
         return;
      }

      if( !( victim = ch->get_char_room( arg2 ) ) )
      {
         ch->print( "They aren't here.\r\n" );
         return;
      }

      if( ch->gold < amount )
      {
         ch->print( "Very generous of you, but you haven't got that much gold.\r\n" );
         return;
      }

      ch->gold -= amount;
      victim->gold += amount;

      act_printf( AT_ACTION, ch, NULL, victim, TO_VICT, "$n gives you %d coin%s.", amount, amount == 1 ? "" : "s" );
      act( AT_ACTION, "$n gives $N some gold.", ch, NULL, victim, TO_NOTVICT );
      act( AT_ACTION, "You give $N some gold.", ch, NULL, victim, TO_CHAR );
      mprog_bribe_trigger( victim, ch, amount );
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

   if( !( victim = ch->get_char_room( arg2 ) ) )
   {
      ch->print( "They aren't here.\r\n" );
      return;
   }

   if( !ch->can_drop_obj( obj ) )
   {
      ch->print( "You can't let go of it.\r\n" );
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

   if( !victim->can_see_obj( obj, false ) )
   {
      act( AT_PLAIN, "$N can't see it.", ch, NULL, victim, TO_CHAR );
      return;
   }

   if( obj->extra_flags.test( ITEM_PROTOTYPE ) && !victim->can_take_proto(  ) )
   {
      act( AT_PLAIN, "You cannot give that to $N!", ch, NULL, victim, TO_CHAR );
      return;
   }

   if( victim->char_ego(  ) < obj->ego && obj->ego >= sysdata->minego )
   {
      act( AT_SAY, "$p glows brightly and resists being given to $N.", ch, obj, victim, TO_CHAR );
      return;
   }

   obj->separate(  );
   obj->from_char(  );
   act( AT_ACTION, "$n gives $p to $N.", ch, obj, victim, TO_NOTVICT );
   act( AT_ACTION, "$n gives you $p.", ch, obj, victim, TO_VICT );
   act( AT_ACTION, "You give $p to $N.", ch, obj, victim, TO_CHAR );
   obj->to_char( victim );
   mprog_give_trigger( victim, ch, obj );

   /*
    * Added by Tarl 5 May 2002 to ensure pets aren't given rent items.
    * * Moved up here to prevent a minor bug with the fact the item is purged on save ;)
    */
   if( ( obj->extra_flags.test( ITEM_PERSONAL ) || obj->ego >= sysdata->minego ) && victim->is_pet(  ) )
   {
      ch->printf( "%s says 'I'm sorry master, but I can't carry this.'\r\n", victim->short_descr );
      ch->printf( "%s hands %s back to you.\r\n", victim->short_descr, obj->short_descr );
      obj->separate(  );
      obj->from_char(  );
      obj->to_char( ch );
      ch->save(  );
      return;
   }
   if( IS_SAVE_FLAG( SV_GIVE ) && !ch->char_died(  ) )
      ch->save(  );
   if( IS_SAVE_FLAG( SV_RECEIVE ) && !victim->char_died(  ) )
      victim->save(  );

   if( victim->has_actflag( ACT_GUILDVENDOR ) )
      check_clan_shop( ch, victim, obj );
}

/*
 * Remove an object.
 */
bool remove_obj( char_data * ch, int iWear, bool fReplace )
{
   obj_data *obj, *tmpobj;

   if( !( obj = ch->get_eq( iWear ) ) )
      return true;

   if( !fReplace && ch->carry_number + obj->get_number(  ) > ch->can_carry_n(  ) )
   {
      act( AT_PLAIN, "$d: you can't carry that many items.", ch, NULL, obj->short_descr, TO_CHAR );
      return false;
   }

   if( !fReplace )
      return false;

   if( obj->extra_flags.test( ITEM_NOREMOVE ) )
   {
      act( AT_PLAIN, "You can't remove $p.", ch, obj, NULL, TO_CHAR );
      return false;
   }

   if( obj == ch->get_eq( WEAR_WIELD ) && ( tmpobj = ch->get_eq( WEAR_DUAL_WIELD ) ) != NULL )
      tmpobj->wear_loc = WEAR_WIELD;

   ch->unequip( obj );

   act( AT_ACTION, "$n stops using $p.", ch, obj, NULL, TO_ROOM );
   act( AT_ACTION, "You stop using $p.", ch, obj, NULL, TO_CHAR );
   oprog_remove_trigger( ch, obj );
   /*
    * Added check in case, the trigger forces them to rewear the item
    * * --Shaddai
    */
   if( !( obj = ch->get_eq( iWear ) ) )
      return true;
   else
      return false;
}

/*
 * See if char could be capable of dual-wielding		-Thoric
 */
bool could_dual( char_data * ch )
{
   if( ch->isnpc(  ) || ch->pcdata->learned[gsn_dual_wield] )
      return true;

   return false;
}

// Remcon's version of this is hugely cleaner than the others
bool can_dual( char_data * ch )
{
   obj_data *wield = NULL;
   bool nwield = false;

   if( !could_dual( ch ) )
      return false;

   wield = ch->get_eq( WEAR_WIELD );

   /*
    * Check for missile wield or dual wield 
    */
   if( ch->get_eq( WEAR_MISSILE_WIELD ) || ch->get_eq( WEAR_DUAL_WIELD ) )
      nwield = true;

   if( wield && nwield )
   {
      ch->print( "You are already wielding two weapons!\r\n" );
      return false;
   }

   if( ( wield || nwield ) && ch->get_eq( WEAR_SHIELD ) )
   {
      ch->print( "You cannot dual wield, you're already holding a shield!\r\n" );
      return false;
   }

   if( ( wield || nwield ) && ch->get_eq( WEAR_HOLD ) )
   {
      ch->print( "You cannot hold another weapon, you're already holding something in that hand!\r\n" );
      return false;
   }

   if( wield->extra_flags.test( ITEM_TWOHAND ) )
   {
      ch->print( "You cannot wield a second weapon while already wielding a two-handed weapon!\r\n" );
      return false;
   }
   return true;
}

/*
 * Check to see if there is room to wear another object on this location
 * (Layered clothing support)
 */
// FIX ME - The entire layering system is seriously screwed up
bool can_layer( char_data * ch, obj_data * obj, short wear_loc )
{
   list < obj_data * >::iterator iobj;
   short bitlayers = 0;
   short objlayers = obj->pIndexData->layers;

   for( iobj = ch->carrying.begin(  ); iobj != ch->carrying.end(  ); ++iobj )
   {
      obj_data *otmp = *iobj;
      if( otmp->wear_loc == wear_loc )
      {
         if( !otmp->pIndexData->layers )
            return false;
         else
            bitlayers |= otmp->pIndexData->layers;
      }
   }
   if( ( bitlayers && !objlayers ) || bitlayers > objlayers )
      return false;
   if( !bitlayers || ( ( bitlayers & ~objlayers ) == bitlayers ) )
      return true;
   return false;
}

bool can_wear_obj( char_data * ch, obj_data * obj )
{
#define CWO(c,s) ( ch->Class == (c) && obj->extra_flags.test( (s) ) )

   if( CWO( CLASS_MAGE, ITEM_ANTI_MAGE ) )
      return false;
   if( CWO( CLASS_WARRIOR, ITEM_ANTI_WARRIOR ) )
      return false;
   if( CWO( CLASS_CLERIC, ITEM_ANTI_CLERIC ) )
      return false;
   if( CWO( CLASS_ROGUE, ITEM_ANTI_ROGUE ) )
      return false;
   if( CWO( CLASS_DRUID, ITEM_ANTI_DRUID ) )
      return false;
   if( CWO( CLASS_RANGER, ITEM_ANTI_RANGER ) )
      return false;
   if( CWO( CLASS_PALADIN, ITEM_ANTI_PALADIN ) )
      return false;
   if( CWO( CLASS_MONK, ITEM_ANTI_MONK ) )
      return false;
   if( CWO( CLASS_NECROMANCER, ITEM_ANTI_NECRO ) )
      return false;
   if( CWO( CLASS_ANTIPALADIN, ITEM_ANTI_APAL ) )
      return false;
   if( CWO( CLASS_BARD, ITEM_ANTI_BARD ) )
      return false;
#undef CWO

#define CWC(c,s) ( ch->Class != (c) && obj->extra_flags.test( (s) ) )

   if( CWC( CLASS_CLERIC, ITEM_ONLY_CLERIC ) )
      return false;
   if( CWC( CLASS_MAGE, ITEM_ONLY_MAGE ) )
      return false;
   if( CWC( CLASS_ROGUE, ITEM_ONLY_ROGUE ) )
      return false;
   if( CWC( CLASS_WARRIOR, ITEM_ONLY_WARRIOR ) )
      return false;
   if( CWC( CLASS_BARD, ITEM_ONLY_BARD ) )
      return false;
   if( CWC( CLASS_DRUID, ITEM_ONLY_DRUID ) )
      return false;
   if( CWC( CLASS_MONK, ITEM_ONLY_MONK ) )
      return false;
   if( CWC( CLASS_RANGER, ITEM_ONLY_RANGER ) )
      return false;
   if( CWC( CLASS_PALADIN, ITEM_ONLY_PALADIN ) )
      return false;
   if( CWC( CLASS_NECROMANCER, ITEM_ONLY_NECRO ) )
      return false;
   if( CWC( CLASS_ANTIPALADIN, ITEM_ONLY_APAL ) )
      return false;
#undef CWC

#define CWS(c,s) ( ch->sex == (c) && obj->extra_flags.test( (s) ) )

   if( CWS( SEX_NEUTRAL, ITEM_ANTI_NEUTER ) )
      return false;
   if( CWS( SEX_MALE, ITEM_ANTI_MEN ) )
      return false;
   if( CWS( SEX_FEMALE, ITEM_ANTI_WOMEN ) )
      return false;
   if( CWS( SEX_HERMAPHRODYTE, ITEM_ANTI_HERMA ) )
      return false;
#undef CWS
   return true;
}

/*
 * Wear one object.
 * Optional replacement of existing objects.
 * Big repetitive code, ick.
 *
 * Restructured a bit to allow for specifying body location	-Thoric
 * & Added support for layering on certain body locations
 */
void wear_obj( char_data * ch, obj_data * obj, bool fReplace, int wear_bit )
{
   obj_data *tmpobj = NULL;
   int bit, tmp;

   if( obj->extra_flags.test( ITEM_PERSONAL ) && str_cmp( ch->name, obj->owner ) )
   {
      ch->print( "That item is personalized and belongs to someone else.\r\n" );
      if( obj->carried_by )
         obj->from_char(  );
      obj->to_room( ch->in_room, ch );
      return;
   }

   obj->separate(  );

   if( ch->char_ego(  ) < obj->ego )
   {
      ch->print( "You can't figure out how to use it.\r\n" );
      act( AT_ACTION, "$n tries to use $p, but can't figure it out.", ch, obj, NULL, TO_ROOM );
      return;
   }

   if( !ch->is_immortal(  ) && !can_wear_obj( ch, obj ) )
   {
      act( AT_MAGIC, "You are forbidden to use that item.", ch, NULL, NULL, TO_CHAR );
      act( AT_ACTION, "$n tries to use $p, but is forbidden to do so.", ch, obj, NULL, TO_ROOM );
      return;
   }

   /*
    * Check to see if an item they're trying to equip requires that they be mounted - Samson 3-18-01 
    */
   if( !ch->is_immortal(  ) && obj->extra_flags.test( ITEM_MUSTMOUNT ) && !ch->IS_MOUNTED(  ) )
   {
      act( AT_ACTION, "You must be mounted to use $p.", ch, obj, NULL, TO_CHAR );
      act( AT_ACTION, "$n tries to use $p while not mounted.", ch, obj, NULL, TO_ROOM );
      return;
   }

   /*
    * Added by Tarl 8 May 2002 so that horses can't wield weapons and armor. 
    */
   if( ch->isnpc(  ) )
   {
      if( ch->race == RACE_HORSE )
      {
         act( AT_ACTION, "$n snorts in surprise. Maybe they can't use that?", ch, obj, NULL, TO_ROOM );
         return;
      }
   }

   if( wear_bit > -1 )
   {
      bit = wear_bit;
      if( !obj->wear_flags.test( bit ) )
      {
         if( fReplace )
         {
            switch ( bit )
            {
               case ITEM_HOLD:
                  ch->print( "You cannot hold that.\r\n" );
                  break;
               case ITEM_WIELD:
               case ITEM_MISSILE_WIELD:
                  ch->print( "You cannot wield that.\r\n" );
                  break;
               default:
                  ch->printf( "You cannot wear that on your %s.\r\n", w_flags[bit] );
            }
         }
         return;
      }
   }
   else
   {
      for( bit = -1, tmp = 1; tmp < MAX_WEAR_FLAG; ++tmp )
      {
         if( obj->wear_flags.test( tmp ) )
         {
            bit = tmp;
            break;
         }
      }
   }

   /*
    * currently cannot have a light in non-light position 
    */
   if( obj->item_type == ITEM_LIGHT )
   {
      if( !remove_obj( ch, WEAR_LIGHT, fReplace ) )
         return;
      if( !oprog_use_trigger( ch, obj, NULL, NULL ) )
      {
         act( AT_ACTION, "$n holds $p as a light.", ch, obj, NULL, TO_ROOM );
         act( AT_ACTION, "You hold $p as your light.", ch, obj, NULL, TO_CHAR );
      }
      ch->equip( obj, WEAR_LIGHT );
      oprog_wear_trigger( ch, obj );
      return;
   }

   if( bit == -1 )
   {
      if( fReplace )
         ch->print( "You can't wear, wield, or hold that.\r\n" );
      return;
   }

   switch ( bit )
   {
      default:
         bug( "%s: uknown/unused item_wear bit %d. Obj vnum: %d", __FUNCTION__, bit, obj->pIndexData->vnum );
         if( fReplace )
            ch->print( "You can't wear, wield, or hold that.\r\n" );
         return;

      case ITEM_LODGE_RIB:
         act( AT_ACTION, "$p strikes you and deeply imbeds itself in your ribs!", ch, obj, NULL, TO_CHAR );
         act( AT_ACTION, "$p strikes $n and deeply imbeds itself in $s ribs!", ch, obj, NULL, TO_ROOM );
         ch->equip( obj, WEAR_LODGE_RIB );
         break;

      case ITEM_LODGE_ARM:
         act( AT_ACTION, "$p strikes you and deeply imbeds itself in your arm!", ch, obj, NULL, TO_CHAR );
         act( AT_ACTION, "$p strikes $n and deeply imbeds itself in $s arm!", ch, obj, NULL, TO_ROOM );
         ch->equip( obj, WEAR_LODGE_ARM );
         break;

      case ITEM_LODGE_LEG:
         act( AT_ACTION, "$p strikes you and deeply imbeds itself in your leg!", ch, obj, NULL, TO_CHAR );
         act( AT_ACTION, "$p strikes $n and deeply imbeds itself in $s leg!", ch, obj, NULL, TO_ROOM );
         ch->equip( obj, WEAR_LODGE_LEG );
         break;

      case ITEM_HOLD:
         if( ch->get_eq( WEAR_DUAL_WIELD ) || ( ch->get_eq( WEAR_WIELD ) && ( ch->get_eq( WEAR_MISSILE_WIELD ) || ch->get_eq( WEAR_SHIELD ) ) ) )
         {
            if( ch->get_eq( WEAR_SHIELD ) )
               ch->print( "You cannot hold something while wielding a weapon and a shield!\r\n" );
            else
               ch->print( "You cannot hold something AND two weapons!\r\n" );
            return;
         }
         tmpobj = ch->get_eq( WEAR_WIELD );
         if( tmpobj && tmpobj->extra_flags.test( ITEM_TWOHAND ) )
         {
            ch->print( "You cannot hold something while wielding a two-handed weapon!\r\n" );
            return;
         }
         if( !remove_obj( ch, WEAR_HOLD, fReplace ) )
            return;
         if( obj->item_type == ITEM_WAND || obj->item_type == ITEM_STAFF || obj->item_type == ITEM_FOOD
             || obj->item_type == ITEM_COOK || obj->item_type == ITEM_PILL || obj->item_type == ITEM_POTION
             || obj->item_type == ITEM_SCROLL || obj->item_type == ITEM_DRINK_CON || obj->item_type == ITEM_BLOOD
             || obj->item_type == ITEM_PIPE || obj->item_type == ITEM_HERB || obj->item_type == ITEM_KEY || !oprog_use_trigger( ch, obj, NULL, NULL ) )
         {
            act( AT_ACTION, "$n holds $p in $s hands.", ch, obj, NULL, TO_ROOM );
            act( AT_ACTION, "You hold $p in your hands.", ch, obj, NULL, TO_CHAR );
         }
         ch->equip( obj, WEAR_HOLD );
         oprog_wear_trigger( ch, obj );
         return;

      case ITEM_WEAR_FINGER:
         if( ch->get_eq( WEAR_FINGER_L ) && ch->get_eq( WEAR_FINGER_R ) && !remove_obj( ch, WEAR_FINGER_L, fReplace ) && !remove_obj( ch, WEAR_FINGER_R, fReplace ) )
            return;

         if( !ch->get_eq( WEAR_FINGER_L ) )
         {
            if( !oprog_use_trigger( ch, obj, NULL, NULL ) )
            {
               act( AT_ACTION, "$n slips $s left finger into $p.", ch, obj, NULL, TO_ROOM );
               act( AT_ACTION, "You slip your left finger into $p.", ch, obj, NULL, TO_CHAR );
            }
            ch->equip( obj, WEAR_FINGER_L );
            oprog_wear_trigger( ch, obj );
            return;
         }

         if( !ch->get_eq( WEAR_FINGER_R ) )
         {
            if( !oprog_use_trigger( ch, obj, NULL, NULL ) )
            {
               act( AT_ACTION, "$n slips $s right finger into $p.", ch, obj, NULL, TO_ROOM );
               act( AT_ACTION, "You slip your right finger into $p.", ch, obj, NULL, TO_CHAR );
            }
            ch->equip( obj, WEAR_FINGER_R );
            oprog_wear_trigger( ch, obj );
            return;
         }

         ch->print( "You already wear something on both fingers.\r\n" );
         return;

      case ITEM_WEAR_NECK:
         if( ch->get_eq( WEAR_NECK_1 ) != NULL && ch->get_eq( WEAR_NECK_2 ) != NULL && !remove_obj( ch, WEAR_NECK_1, fReplace ) && !remove_obj( ch, WEAR_NECK_2, fReplace ) )
            return;

         if( !ch->get_eq( WEAR_NECK_1 ) )
         {
            if( !oprog_use_trigger( ch, obj, NULL, NULL ) )
            {
               act( AT_ACTION, "$n wears $p around $s neck.", ch, obj, NULL, TO_ROOM );
               act( AT_ACTION, "You wear $p around your neck.", ch, obj, NULL, TO_CHAR );
            }
            ch->equip( obj, WEAR_NECK_1 );
            oprog_wear_trigger( ch, obj );
            return;
         }

         if( !ch->get_eq( WEAR_NECK_2 ) )
         {
            if( !oprog_use_trigger( ch, obj, NULL, NULL ) )
            {
               act( AT_ACTION, "$n wears $p around $s neck.", ch, obj, NULL, TO_ROOM );
               act( AT_ACTION, "You wear $p around your neck.", ch, obj, NULL, TO_CHAR );
            }
            ch->equip( obj, WEAR_NECK_2 );
            oprog_wear_trigger( ch, obj );
            return;
         }

         ch->print( "You already wear two neck items.\r\n" );
         return;

      case ITEM_WEAR_BODY:
         if( !can_layer( ch, obj, WEAR_BODY ) )
         {
            ch->print( "It won't fit overtop of what you're already wearing.\r\n" );
            return;
         }
         if( !oprog_use_trigger( ch, obj, NULL, NULL ) )
         {
            act( AT_ACTION, "$n fits $p on $s body.", ch, obj, NULL, TO_ROOM );
            act( AT_ACTION, "You fit $p on your body.", ch, obj, NULL, TO_CHAR );
         }
         ch->equip( obj, WEAR_BODY );
         oprog_wear_trigger( ch, obj );
         return;

      case ITEM_WEAR_HEAD:
         if( !remove_obj( ch, WEAR_HEAD, fReplace ) )
            return;
         if( !oprog_use_trigger( ch, obj, NULL, NULL ) )
         {
            act( AT_ACTION, "$n dons $p upon $s head.", ch, obj, NULL, TO_ROOM );
            act( AT_ACTION, "You don $p upon your head.", ch, obj, NULL, TO_CHAR );
         }
         ch->equip( obj, WEAR_HEAD );
         oprog_wear_trigger( ch, obj );
         return;

      case ITEM_WEAR_EYES:
         if( !remove_obj( ch, WEAR_EYES, fReplace ) )
            return;
         if( !oprog_use_trigger( ch, obj, NULL, NULL ) )
         {
            act( AT_ACTION, "$n places $p on $s eyes.", ch, obj, NULL, TO_ROOM );
            act( AT_ACTION, "You place $p on your eyes.", ch, obj, NULL, TO_CHAR );
         }
         ch->equip( obj, WEAR_EYES );
         oprog_wear_trigger( ch, obj );
         return;

      case ITEM_WEAR_FACE:
         if( !remove_obj( ch, WEAR_FACE, fReplace ) )
            return;
         if( !oprog_use_trigger( ch, obj, NULL, NULL ) )
         {
            act( AT_ACTION, "$n places $p on $s face.", ch, obj, NULL, TO_ROOM );
            act( AT_ACTION, "You place $p on your face.", ch, obj, NULL, TO_CHAR );
         }
         ch->equip( obj, WEAR_FACE );
         oprog_wear_trigger( ch, obj );
         return;

      case ITEM_WEAR_EARS:
         if( !remove_obj( ch, WEAR_EARS, fReplace ) )
            return;
         if( !oprog_use_trigger( ch, obj, NULL, NULL ) )
         {
            act( AT_ACTION, "$n wears $p on $s ears.", ch, obj, NULL, TO_ROOM );
            act( AT_ACTION, "You wear $p on your ears.", ch, obj, NULL, TO_CHAR );
         }
         ch->equip( obj, WEAR_EARS );
         oprog_wear_trigger( ch, obj );
         return;

      case ITEM_WEAR_LEGS:
         if( !can_layer( ch, obj, WEAR_LEGS ) )
         {
            ch->print( "It won't fit overtop of what you're already wearing.\r\n" );
            return;
         }
         if( !oprog_use_trigger( ch, obj, NULL, NULL ) )
         {
            act( AT_ACTION, "$n slips into $p.", ch, obj, NULL, TO_ROOM );
            act( AT_ACTION, "You slip into $p.", ch, obj, NULL, TO_CHAR );
         }
         ch->equip( obj, WEAR_LEGS );
         oprog_wear_trigger( ch, obj );
         return;

      case ITEM_WEAR_FEET:
         if( !can_layer( ch, obj, WEAR_FEET ) )
         {
            ch->print( "It won't fit overtop of what you're already wearing.\r\n" );
            return;
         }
         if( !oprog_use_trigger( ch, obj, NULL, NULL ) )
         {
            act( AT_ACTION, "$n wears $p on $s feet.", ch, obj, NULL, TO_ROOM );
            act( AT_ACTION, "You wear $p on your feet.", ch, obj, NULL, TO_CHAR );
         }
         ch->equip( obj, WEAR_FEET );
         oprog_wear_trigger( ch, obj );
         return;

      case ITEM_WEAR_HANDS:
         if( !can_layer( ch, obj, WEAR_HANDS ) )
         {
            ch->print( "It won't fit overtop of what you're already wearing.\r\n" );
            return;
         }
         if( !oprog_use_trigger( ch, obj, NULL, NULL ) )
         {
            act( AT_ACTION, "$n wears $p on $s hands.", ch, obj, NULL, TO_ROOM );
            act( AT_ACTION, "You wear $p on your hands.", ch, obj, NULL, TO_CHAR );
         }
         ch->equip( obj, WEAR_HANDS );
         oprog_wear_trigger( ch, obj );
         return;

      case ITEM_WEAR_ARMS:
         if( !can_layer( ch, obj, WEAR_ARMS ) )
         {
            ch->print( "It won't fit overtop of what you're already wearing.\r\n" );
            return;
         }
         if( !oprog_use_trigger( ch, obj, NULL, NULL ) )
         {
            act( AT_ACTION, "$n wears $p on $s arms.", ch, obj, NULL, TO_ROOM );
            act( AT_ACTION, "You wear $p on your arms.", ch, obj, NULL, TO_CHAR );
         }
         ch->equip( obj, WEAR_ARMS );
         oprog_wear_trigger( ch, obj );
         return;

      case ITEM_WEAR_ABOUT:
         if( !can_layer( ch, obj, WEAR_ABOUT ) )
         {
            ch->print( "It won't fit overtop of what you're already wearing.\r\n" );
            return;
         }
         if( !oprog_use_trigger( ch, obj, NULL, NULL ) )
         {
            act( AT_ACTION, "$n wears $p about $s body.", ch, obj, NULL, TO_ROOM );
            act( AT_ACTION, "You wear $p about your body.", ch, obj, NULL, TO_CHAR );
         }
         ch->equip( obj, WEAR_ABOUT );
         oprog_wear_trigger( ch, obj );
         return;

      case ITEM_WEAR_BACK:
         if( !remove_obj( ch, WEAR_BACK, fReplace ) )
            return;
         if( !oprog_use_trigger( ch, obj, NULL, NULL ) )
         {
            act( AT_ACTION, "$n slings $p on $s back.", ch, obj, NULL, TO_ROOM );
            act( AT_ACTION, "You sling $p on your back.", ch, obj, NULL, TO_CHAR );
         }
         ch->equip( obj, WEAR_BACK );
         oprog_wear_trigger( ch, obj );
         return;

      case ITEM_WEAR_WAIST:
         if( !can_layer( ch, obj, WEAR_WAIST ) )
         {
            ch->print( "It won't fit overtop of what you're already wearing.\r\n" );
            return;
         }
         if( !oprog_use_trigger( ch, obj, NULL, NULL ) )
         {
            act( AT_ACTION, "$n wears $p about $s waist.", ch, obj, NULL, TO_ROOM );
            act( AT_ACTION, "You wear $p about your waist.", ch, obj, NULL, TO_CHAR );
         }
         ch->equip( obj, WEAR_WAIST );
         oprog_wear_trigger( ch, obj );
         return;

      case ITEM_WEAR_WRIST:
         if( ch->get_eq( WEAR_WRIST_L ) && ch->get_eq( WEAR_WRIST_R ) && !remove_obj( ch, WEAR_WRIST_L, fReplace ) && !remove_obj( ch, WEAR_WRIST_R, fReplace ) )
            return;

         if( !ch->get_eq( WEAR_WRIST_L ) )
         {
            if( !oprog_use_trigger( ch, obj, NULL, NULL ) )
            {
               act( AT_ACTION, "$n fits $p around $s left wrist.", ch, obj, NULL, TO_ROOM );
               act( AT_ACTION, "You fit $p around your left wrist.", ch, obj, NULL, TO_CHAR );
            }
            ch->equip( obj, WEAR_WRIST_L );
            oprog_wear_trigger( ch, obj );
            return;
         }

         if( !ch->get_eq( WEAR_WRIST_R ) )
         {
            if( !oprog_use_trigger( ch, obj, NULL, NULL ) )
            {
               act( AT_ACTION, "$n fits $p around $s right wrist.", ch, obj, NULL, TO_ROOM );
               act( AT_ACTION, "You fit $p around your right wrist.", ch, obj, NULL, TO_CHAR );
            }
            ch->equip( obj, WEAR_WRIST_R );
            oprog_wear_trigger( ch, obj );
            return;
         }

         ch->print( "You already wear two wrist items.\r\n" );
         return;

      case ITEM_WEAR_ANKLE:
         if( ch->get_eq( WEAR_ANKLE_L ) && ch->get_eq( WEAR_ANKLE_R ) && !remove_obj( ch, WEAR_ANKLE_L, fReplace ) && !remove_obj( ch, WEAR_ANKLE_R, fReplace ) )
            return;

         if( !ch->get_eq( WEAR_ANKLE_L ) )
         {
            if( !oprog_use_trigger( ch, obj, NULL, NULL ) )
            {
               act( AT_ACTION, "$n fits $p around $s left ankle.", ch, obj, NULL, TO_ROOM );
               act( AT_ACTION, "You fit $p around your left ankle.", ch, obj, NULL, TO_CHAR );
            }
            ch->equip( obj, WEAR_ANKLE_L );
            oprog_wear_trigger( ch, obj );
            return;
         }

         if( !ch->get_eq( WEAR_ANKLE_R ) )
         {
            if( !oprog_use_trigger( ch, obj, NULL, NULL ) )
            {
               act( AT_ACTION, "$n fits $p around $s right ankle.", ch, obj, NULL, TO_ROOM );
               act( AT_ACTION, "You fit $p around your right ankle.", ch, obj, NULL, TO_CHAR );
            }
            ch->equip( obj, WEAR_ANKLE_R );
            oprog_wear_trigger( ch, obj );
            return;
         }

         ch->print( "You already wear two ankle items.\r\n" );
         return;

      case ITEM_WEAR_SHIELD:
         if( ch->get_eq( WEAR_DUAL_WIELD ) || ( ch->get_eq( WEAR_WIELD ) && ch->get_eq( WEAR_MISSILE_WIELD ) ) || ( ch->get_eq( WEAR_WIELD ) && ch->get_eq( WEAR_HOLD ) ) )
         {
            if( ch->get_eq( WEAR_HOLD ) )
               ch->print( "You can't use a shield while wielding a weapon and holding something!\r\n" );
            else
               ch->print( "You can't use a shield AND two weapons!\r\n" );
            return;
         }
         tmpobj = ch->get_eq( WEAR_WIELD );
         if( tmpobj && tmpobj->extra_flags.test( ITEM_TWOHAND ) )
         {
            ch->print( "You cannot wear a shield while wielding a two-handed weapon!\r\n" );
            return;
         }
         if( !remove_obj( ch, WEAR_SHIELD, fReplace ) )
            return;
         if( !oprog_use_trigger( ch, obj, NULL, NULL ) )
         {
            act( AT_ACTION, "$n uses $p as a shield.", ch, obj, NULL, TO_ROOM );
            act( AT_ACTION, "You use $p as a shield.", ch, obj, NULL, TO_CHAR );
         }
         ch->equip( obj, WEAR_SHIELD );
         oprog_wear_trigger( ch, obj );
         return;

      case ITEM_MISSILE_WIELD:
      case ITEM_WIELD:
         if( !could_dual( ch ) )
         {
            if( !remove_obj( ch, WEAR_MISSILE_WIELD, fReplace ) )
               return;
            if( !remove_obj( ch, WEAR_WIELD, fReplace ) )
               return;
            tmpobj = NULL;
         }
         else
         {
            obj_data *mw, *dw, *hd, *sd;
            tmpobj = ch->get_eq( WEAR_WIELD );
            mw = ch->get_eq( WEAR_MISSILE_WIELD );
            dw = ch->get_eq( WEAR_DUAL_WIELD );
            hd = ch->get_eq( WEAR_HOLD );
            sd = ch->get_eq( WEAR_SHIELD );

            if( hd && sd )
            {
               ch->print( "You are already holding something and wearing a shield.\r\n" );
               return;
            }

            if( tmpobj )
            {
               if( !can_dual( ch ) )
                  return;

               if( mw || dw )
               {
                  ch->print( "You're already wielding two weapons.\r\n" );
                  return;
               }

               if( hd || sd )
               {
                  ch->print( "You're already wielding a weapon AND holding something.\r\n" );
                  return;
               }

               if( tmpobj->extra_flags.test( ITEM_TWOHAND ) )
               {
                  ch->print( "You are already wielding a two-handed weapon.\r\n" );
                  return;
               }

               if( obj->get_weight(  ) + tmpobj->get_weight(  ) > str_app[ch->get_curr_str(  )].wield )
               {
                  ch->print( "It is too heavy for you to wield.\r\n" );
                  return;
               }

               if( !oprog_use_trigger( ch, obj, NULL, NULL ) )
               {
                  act( AT_ACTION, "$n dual-wields $p.", ch, obj, NULL, TO_ROOM );
                  act( AT_ACTION, "You dual-wield $p.", ch, obj, NULL, TO_CHAR );
               }
               if( bit == ITEM_MISSILE_WIELD )
                  ch->equip( obj, WEAR_MISSILE_WIELD );
               else
                  ch->equip( obj, WEAR_DUAL_WIELD );
               oprog_wear_trigger( ch, obj );
               return;
            }

            if( mw )
            {
               if( !can_dual( ch ) )
                  return;

               if( bit == ITEM_MISSILE_WIELD )
               {
                  ch->print( "You're already wielding a missile weapon.\r\n" );
                  return;
               }

               if( tmpobj || dw )
               {
                  ch->print( "You're already wielding two weapons.\r\n" );
                  return;
               }

               if( hd || sd )
               {
                  ch->print( "You're already wielding a weapon AND holding something.\r\n" );
                  return;
               }

               if( mw->extra_flags.test( ITEM_TWOHAND ) )
               {
                  ch->print( "You are already wielding a two-handed weapon.\r\n" );
                  return;
               }

               if( obj->get_weight(  ) + mw->get_weight(  ) > str_app[ch->get_curr_str(  )].wield )
               {
                  ch->print( "It is too heavy for you to wield.\r\n" );
                  return;
               }

               if( !oprog_use_trigger( ch, obj, NULL, NULL ) )
               {
                  act( AT_ACTION, "$n wields $p.", ch, obj, NULL, TO_ROOM );
                  act( AT_ACTION, "You wield $p.", ch, obj, NULL, TO_CHAR );
               }
               ch->equip( obj, WEAR_WIELD );
               oprog_wear_trigger( ch, obj );
               return;
            }
         }

         if( obj->get_weight(  ) > str_app[ch->get_curr_str(  )].wield )
         {
            ch->print( "It is too heavy for you to wield.\r\n" );
            return;
         }

         if( !oprog_use_trigger( ch, obj, NULL, NULL ) )
         {
            act( AT_ACTION, "$n wields $p.", ch, obj, NULL, TO_ROOM );
            act( AT_ACTION, "You wield $p.", ch, obj, NULL, TO_CHAR );
         }
         if( bit == ITEM_MISSILE_WIELD )
            ch->equip( obj, WEAR_MISSILE_WIELD );
         else
            ch->equip( obj, WEAR_WIELD );
         oprog_wear_trigger( ch, obj );
         return;
   }
}

CMDF( do_wear )
{
   string arg1, arg2;
   obj_data *obj;
   int wear_bit;

   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );
   if( ( !str_cmp( arg2, "on" ) || !str_cmp( arg2, "upon" ) || !str_cmp( arg2, "around" ) ) && !argument.empty(  ) )
      argument = one_argument( argument, arg2 );

   if( arg1.empty(  ) )
   {
      ch->print( "Wear, wield, or hold what?\r\n" );
      return;
   }

   if( ms_find_obj( ch ) )
      return;

   if( !str_cmp( arg1, "all" ) )
   {
      list < obj_data * >::iterator iobj;

      for( iobj = ch->carrying.begin(  ); iobj != ch->carrying.end(  ); )
      {
         obj = *iobj;
         ++iobj;

         if( obj->wear_loc == WEAR_NONE && ch->can_see_obj( obj, false ) )
         {
            wear_obj( ch, obj, false, -1 );
            if( ch->char_died(  ) )
               return;
         }
      }
      return;
   }
   else
   {
      if( !( obj = ch->get_obj_carry( arg1 ) ) )
      {
         ch->print( "You do not have that item.\r\n" );
         return;
      }
      if( !arg2.empty(  ) )
         wear_bit = get_wflag( arg2 );
      else
         wear_bit = -1;
      wear_obj( ch, obj, true, wear_bit );
   }
}

CMDF( do_remove )
{
   obj_data *obj, *obj2;

   if( argument.empty(  ) )
   {
      ch->print( "Remove what?\r\n" );
      return;
   }

   if( ms_find_obj( ch ) )
      return;

   if( !str_cmp( argument, "all" ) )   /* SB Remove all */
   {
      list < obj_data * >::iterator iobj;
      for( iobj = ch->carrying.begin(  ); iobj != ch->carrying.end(  ); )
      {
         obj = *iobj;
         ++iobj;

         if( obj->wear_loc != WEAR_NONE && ch->can_see_obj( obj, false ) )
            remove_obj( ch, obj->wear_loc, true );
      }
      return;
   }

   if( !( obj = ch->get_obj_wear( argument ) ) )
   {
      ch->print( "You are not using that item.\r\n" );
      return;
   }
   if( ( obj2 = ch->get_eq( obj->wear_loc ) ) != obj )
   {
      act( AT_PLAIN, "You must remove $p first.", ch, obj2, NULL, TO_CHAR );
      return;
   }
   remove_obj( ch, obj->wear_loc, true );
}

CMDF( do_bury )
{
   obj_data *obj;

   if( argument.empty(  ) )
   {
      ch->print( "What do you wish to bury?\r\n" );
      return;
   }

   if( ms_find_obj( ch ) )
      return;

   bool shovel = false;
   list < obj_data * >::iterator iobj;
   for( iobj = ch->carrying.begin(  ); iobj != ch->carrying.end(  ); ++iobj )
   {
      obj_data *tobj = *iobj;
      if( tobj->item_type == ITEM_SHOVEL )
      {
         shovel = true;
         break;
      }
   }

   if( !( obj = get_obj_list( ch, argument, ch->in_room->objects ) ) )
   {
      ch->print( "You can't find it.\r\n" );
      return;
   }
   obj->separate(  );
   if( !obj->wear_flags.test( ITEM_TAKE ) )
   {
      if( !obj->extra_flags.test( ITEM_CLANCORPSE ) || !ch->has_pcflag( PCFLAG_DEADLY ) )
      {
         act( AT_PLAIN, "You cannot bury $p.", ch, obj, NULL, TO_CHAR );
         return;
      }
   }

   switch ( ch->in_room->sector_type )
   {
      default:
         break;

      case SECT_CITY:
      case SECT_INDOORS:
         ch->print( "The floor is too hard to dig through.\r\n" );
         return;

      case SECT_WATER_SWIM:
      case SECT_WATER_NOSWIM:   /* Water checks for river and no_swim added by Samson */
      case SECT_UNDERWATER:
      case SECT_RIVER:
         ch->print( "You cannot bury something here.\r\n" );
         return;

      case SECT_AIR:
         ch->print( "What?  In the air?!\r\n" );
         return;
   }

   if( obj->weight > ( UMAX( 5, ( ch->can_carry_w(  ) / 10 ) ) ) && !shovel )
   {
      ch->print( "You'd need a shovel to bury something that big.\r\n" );
      return;
   }

   short move = ( obj->weight * 50 * ( shovel ? 1 : 5 ) ) / UMAX( 1, ch->can_carry_w(  ) );
   move = URANGE( 2, move, 1000 );
   if( move > ch->move )
   {
      ch->print( "You don't have the energy to bury something of that size.\r\n" );
      return;
   }
   ch->move -= move;
   if( obj->item_type == ITEM_CORPSE_NPC || obj->item_type == ITEM_CORPSE_PC )
      ch->adjust_favor( 6, 1 );

   act( AT_ACTION, "You solemnly bury $p...", ch, obj, NULL, TO_CHAR );
   act( AT_ACTION, "$n solemnly buries $p...", ch, obj, NULL, TO_ROOM );
   obj->extra_flags.set( ITEM_BURIED );
   ch->WAIT_STATE( URANGE( 10, move / 2, 100 ) );
}

CMDF( do_sacrifice )
{
   string name;
   obj_data *obj;

   if( argument.empty(  ) || !str_cmp( argument, ch->name ) )
   {
      act( AT_ACTION, "$n offers $mself to $s deity, who graciously declines.", ch, NULL, NULL, TO_ROOM );
      ch->print( "Your deity appreciates your offer and may accept it later.\r\n" );
      return;
   }

   if( ms_find_obj( ch ) )
      return;

   if( !( obj = get_obj_list( ch, argument, ch->in_room->objects ) ) )
   {
      ch->print( "You can't find it.\r\n" );
      return;
   }

   obj->separate(  );
   if( !obj->wear_flags.test( ITEM_TAKE ) )
   {
      act( AT_PLAIN, "$p is not an acceptable sacrifice.", ch, obj, 0, TO_CHAR );
      return;
   }
   if( !ch->isnpc(  ) && ch->pcdata->deity && ch->pcdata->deity->name[0] != '\0' )
      name = ch->pcdata->deity->name;

   else if( !ch->isnpc(  ) && ch->pcdata->clan && !ch->pcdata->clan->deity.empty(  ) )
      name = ch->pcdata->clan->deity;

   else
      name = "The Wedgy";

   if( obj->item_type == ITEM_CORPSE_NPC || obj->item_type == ITEM_CORPSE_PC )
   {
      int xp;

      ch->adjust_favor( 5, 1 );

      if( obj->item_type == ITEM_CORPSE_NPC )
         xp = obj->level * 10;
      else
         xp = obj->value[4] * 10;

      ch->gold += xp;
      if( !str_cmp( name, "The Wedgy" ) )
         ch->printf( "The Wedgy swoops down from the void to scoop up %s!\r\n", obj->short_descr );
      else
         ch->printf( "%s grants you %d gold for your sacrifice.\r\n", name.c_str(  ), xp );
   }
   else
   {
      ch->gold += 1;
      ch->printf( "%s gives you one gold coin for your sacrifice.\r\n", name.c_str(  ) );
   }
   act_printf( AT_ACTION, ch, obj, NULL, TO_ROOM, "$n sacrifices $p to %s.", name.c_str(  ) );
   oprog_sac_trigger( ch, obj );
   if( obj->extracted(  ) )
      return;
   obj->separate(  );
   obj->extract(  );
}

CMDF( do_brandish )
{
   obj_data *staff;
   int sn;

   if( !( staff = ch->get_eq( WEAR_HOLD ) ) )
   {
      ch->print( "You hold nothing in your hand.\r\n" );
      return;
   }

   if( staff->item_type != ITEM_STAFF )
   {
      ch->print( "You can brandish only with a staff.\r\n" );
      return;
   }

   if( ( sn = staff->value[3] ) < 0 || sn >= num_skills || skill_table[sn]->spell_fun == NULL )
   {
      log_printf( "Object: %s Vnum %d, bad sn: %d.", staff->short_descr, staff->pIndexData->vnum, sn );
      return;
   }

   ch->WAIT_STATE( 2 * sysdata->pulseviolence );

   if( staff->value[2] > 0 )
   {
      list < char_data * >::iterator ich;

      if( !oprog_use_trigger( ch, staff, NULL, NULL ) )
      {
         act( AT_MAGIC, "$n brandishes $p.", ch, staff, NULL, TO_ROOM );
         act( AT_MAGIC, "You brandish $p.", ch, staff, NULL, TO_CHAR );
      }

      for( ich = ch->in_room->people.begin(  ); ich != ch->in_room->people.end(  ); )
      {
         char_data *vch = *ich;
         ++ich;

         if( vch->has_pcflag( PCFLAG_WIZINVIS ) && vch->pcdata->wizinvis > ch->level )
            continue;
         else
            switch ( skill_table[sn]->target )
            {
               default:
                  log_printf( "Object: %s Vnum %d, bad target for sn %d.", staff->short_descr, staff->pIndexData->vnum, sn );
                  return;

               case TAR_IGNORE:
                  if( vch != ch )
                     continue;
                  break;

               case TAR_CHAR_OFFENSIVE:
                  if( ch->isnpc(  )? vch->isnpc(  ) : !vch->isnpc(  ) )
                     continue;
                  break;

               case TAR_CHAR_DEFENSIVE:
                  if( ch->isnpc(  )? !vch->isnpc(  ) : vch->isnpc(  ) )
                     continue;
                  break;

               case TAR_CHAR_SELF:
                  if( vch != ch )
                     continue;
                  break;
            }

         ch_ret retcode = obj_cast_spell( staff->value[3], staff->value[0], ch, vch, NULL );
         if( retcode == rCHAR_DIED )
         {
            bug( "%s: char died", __FUNCTION__ );
            return;
         }
      }
   }

   if( --staff->value[2] <= 0 )  /* Modified to prevent extraction when reaching zero - Samson */
   {
      staff->value[2] = 0; /* Added to prevent the damn things from getting negative values - Samson 4-21-98 */
      act( AT_MAGIC, "$p blazes brightly in $n's hands, but does nothing.", ch, staff, NULL, TO_ROOM );
      act( AT_MAGIC, "$p blazes brightly, but does nothing.", ch, staff, NULL, TO_CHAR );
   }
}

CMDF( do_zap )
{
   char_data *victim;
   obj_data *wand, *obj = NULL;
   ch_ret retcode;

   if( ( argument.empty(  ) ) && !ch->fighting )
   {
      ch->print( "Zap whom or what?\r\n" );
      return;
   }

   if( !( wand = ch->get_eq( WEAR_HOLD ) ) )
   {
      ch->print( "You hold nothing in your hand.\r\n" );
      return;
   }

   if( wand->item_type != ITEM_WAND )
   {
      ch->print( "You can zap only with a wand.\r\n" );
      return;
   }

   if( argument.empty(  ) )
   {
      if( ch->fighting )
         victim = ch->who_fighting(  );
      else
      {
         ch->print( "Zap whom or what?\r\n" );
         return;
      }
   }
   else
   {
      if( !( victim = ch->get_char_room( argument ) ) && !( obj = ch->get_obj_here( argument ) ) )
      {
         ch->print( "You can't find it.\r\n" );
         return;
      }
   }

   ch->WAIT_STATE( 2 * sysdata->pulseviolence );

   if( wand->value[2] > 0 )
   {
      if( victim )
      {
         if( !oprog_use_trigger( ch, wand, victim, NULL ) )
         {
            act( AT_MAGIC, "$n aims $p at $N.", ch, wand, victim, TO_ROOM );
            act( AT_MAGIC, "You aim $p at $N.", ch, wand, victim, TO_CHAR );
         }
      }
      else
      {
         if( !oprog_use_trigger( ch, wand, NULL, obj ) )
         {
            act( AT_MAGIC, "$n aims $p at $P.", ch, wand, obj, TO_ROOM );
            act( AT_MAGIC, "You aim $p at $P.", ch, wand, obj, TO_CHAR );
         }
      }

      retcode = obj_cast_spell( wand->value[3], wand->value[0], ch, victim, obj );
      if( retcode == rCHAR_DIED )
      {
         bug( "%s: char died", __FUNCTION__ );
         return;
      }
   }

   if( --wand->value[2] <= 0 )   /* Modified to prevent extraction when reaching zero - Samson */
   {
      wand->value[2] = 0;  /* To prevent negative values - Samson */
      act( AT_MAGIC, "$p hums softly, but does nothing.", ch, wand, NULL, TO_ROOM );
      act( AT_MAGIC, "$p hums softly, but does nothing.", ch, wand, NULL, TO_CHAR );
   }
}
