/****************************************************************************
 *                   ^     +----- |  / ^     ^ |     | +-\                  *
 *                  / \    |      | /  |\   /| |     | |  \                 *
 *                 /   \   +---   |<   | \ / | |     | |  |                 *
 *                /-----\  |      | \  |  v  | |     | |  /                 *
 *               /       \ |      |  \ |     | +-----+ +-/                  *
 ****************************************************************************
 * AFKMud Copyright 1997-2012 by Roger Libiez (Samson),                     *
 * Levi Beckerson (Whir), Michael Ward (Tarl), Erik Wolfe (Dwip),           *
 * Cameron Carroll (Cam), Cyberfox, Karangi, Rathian, Raine,                *
 * Xorith, and Adjani.                                                      *
 * All Rights Reserved.                                                     *
 *                                                                          *
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
 *                            Rare Items Module                             *
 ****************************************************************************/

#include <sys/stat.h>
#include <dirent.h>
#include "mud.h"
#include "area.h"
#include "auction.h"
#include "clans.h"
#include "connhist.h"
#include "descriptor.h"
#include "new_auth.h"
#include "objindex.h"
#include "overland.h"
#include "roomindex.h"

#ifdef MULTIPORT
extern bool compilelock;
#endif
extern bool bootlock;

auth_data *get_auth_name( const string & );
void check_pfiles( time_t );
void update_connhistory( descriptor_data *, int );
void show_stateflags( char_data * );
void quotes( char_data * );

/* Removes rare items the player cannot maintain. */
void rare_purge( char_data * ch, obj_data * obj )
{
   list < obj_data * >::iterator iobj;

   if( obj->ego >= sysdata->minego )
      obj->extract(  );

   for( iobj = obj->contents.begin(  ); iobj != obj->contents.end(  ); )
   {
      obj_data *tobj = *iobj;
      ++iobj;

      rare_purge( ch, tobj );
   }
}

// Looks for any -1 rent items. These cannot be kept past rent/quit/camp.
void inventory_scan( char_data * ch, obj_data * obj )
{
   if( obj->ego == -1 )
   {
      if( obj->wear_loc != WEAR_NONE )
         ch->unequip( obj );
      obj->separate(  );
      ch->printf( "%s dissapears in a cloud of smoke!\r\n", obj->short_descr );
      obj->extract(  );
   }

   list < obj_data * >::iterator iobj;
   for( iobj = obj->contents.begin(  ); iobj != obj->contents.end(  ); )
   {
      obj_data *tobj = *iobj;
      ++iobj;

      inventory_scan( ch, tobj );
   }
}

void expire_items( char_data * ch )
{
   list < obj_data * >::iterator iobj;
   for( iobj = ch->carrying.begin(  ); iobj != ch->carrying.end(  ); )
   {
      obj_data *obj = *iobj;
      ++iobj;

      // If the player is less powerful than the object, it has a chance of simply disappearing without notice.
      if( ch->char_ego(  ) < obj->ego && number_bits( 3 ) > 4 )
      {
         log_printf( "obj %d, %s, removed from %s. Random ego check.", obj->pIndexData->vnum, obj->short_descr, ch->name );
         rare_purge( ch, obj );
         continue;
      }

      // The larger the item's ego, the larger it's implied worth. So you get less time to stash it offline.
      if( ch->pcdata->daysidle > 100 - obj->ego )
      {
         log_printf( "obj %d, %s, removed from %s. Exceeded ego idle time.", obj->pIndexData->vnum, obj->short_descr, ch->name );
         rare_purge( ch, obj );
         continue;
      }
   }
}

void char_leaving( char_data * ch, int howleft )
{
   auth_data *old_auth = NULL;

   /*
    * new auth 
    */
   old_auth = get_auth_name( ch->name );
   if( old_auth != NULL )
      if( old_auth->state == AUTH_ONLINE )
         old_auth->state = AUTH_OFFLINE;  /* Logging off */

   ch->pcdata->camp = howleft;

   if( howleft == 0 )   /* Rented at an inn */
   {
      switch ( ch->in_room->area->continent )
      {
         case ACON_ONE:
            ch->pcdata->one = ch->in_room->vnum;
            break;
         default:
            break;
      }
   }

   /*
    * Get 'em dismounted until we finish mount saving -- Blodkai, 4/97 
    */
   if( ch->position == POS_MOUNTED )
      interpret( ch, "dismount" );

   if( ch->morph )
      interpret( ch, "revert" );

   if( ch->desc )
   {
      if( ch->timer > 24 )
         update_connhistory( ch->desc, CONNTYPE_IDLE );
      else
         update_connhistory( ch->desc, CONNTYPE_QUIT );
   }

   list < obj_data * >::iterator iobj;
   for( iobj = ch->carrying.begin(  ); iobj != ch->carrying.end(  ); )
   {
      obj_data *obj = *iobj;
      ++iobj;

      inventory_scan( ch, obj );
   }
   quotes( ch );
   quitting_char = ch;
   ch->save(  );

   if( sysdata->save_pets )
   {
      list < char_data * >::iterator pet;

      for( pet = ch->pets.begin(  ); pet != ch->pets.end(  ); )
      {
         char_data *cpet = *pet;
         ++pet;

         cpet->extract( true );
      }
   }

   /*
    * Synch clandata up only when clan member quits now. --Shaddai 
    */
   if( ch->pcdata->clan )
      save_clan( ch->pcdata->clan );

   saving_char = NULL;
   ch->extract( true );

   for( int x = 0; x < MAX_WEAR; ++x )
      for( int y = 0; y < MAX_LAYERS; ++y )
         save_equipment[x][y] = NULL;
}

CMDF( do_quit )
{
   int level = ch->level;

   if( ch->isnpc(  ) )
   {
      ch->print( "NPCs cannot use the quit command.\r\n" );
      return;
   }

   if( !str_cmp( argument, "auto" ) )
   {
      log_printf_plus( LOG_COMM, level, "%s has idled out.", ch->name );
      char_leaving( ch, 3 );
      return;
   }

   if( !ch->is_immortal(  ) )
   {
      if( ch->in_room->area->flags.test( AFLAG_NOQUIT ) )
      {
         ch->print( "You may not quit in this area, it isn't safe!\r\n" );
         return;
      }

      if( ch->in_room->flags.test( ROOM_NOQUIT ) )
      {
         ch->print( "You may not quit here, it isn't safe!\r\n" );
         return;
      }
   }

   if( ch->position == POS_FIGHTING )
   {
      ch->print( "&RNo way! You are fighting.\r\n" );
      return;
   }

   if( ch->position < POS_STUNNED )
   {
      ch->print( "&[blood]You're not DEAD yet.\r\n" );
      return;
   }

   if( ch->get_timer( TIMER_RECENTFIGHT ) > 0 && !ch->is_immortal(  ) )
   {
      ch->print( "&RYour adrenaline is pumping too hard to quit now!\r\n" );
      return;
   }

   if( auction->item != NULL && ( ( ch == auction->buyer ) || ( ch == auction->seller ) ) )
   {
      ch->print( "&[auction]Wait until you have bought/sold the item on auction.\r\n" );
      return;
   }

   if( ch->inflight )
   {
      ch->print( "&YSkyships are not equipped with parachutes. Wait until you land.\r\n" );
      return;
   }

   ch->print( "&WYou make a hasty break for the confines of reality...\r\n" );
   act( AT_SAY, "A strange voice says, 'We await your return, $n...'", ch, NULL, NULL, TO_CHAR );
   ch->print( "&d\r\n" );
   act( AT_BYE, "$n has left the game.", ch, NULL, NULL, TO_ROOM );

   log_printf_plus( LOG_COMM, ch->level, "%s has quit.", ch->name );
   char_leaving( ch, 3 );
}

/* Checks room to see if an Innkeeper mob is present
   Code courtesy of the Smaug mailing list - Installed by Samson */
char_data *find_innkeeper( char_data * ch )
{
   list < char_data * >::iterator ich;

   for( ich = ch->in_room->people.begin(  ); ich != ch->in_room->people.end(  ); ++ich )
   {
      char_data *innkeeper = *ich;

      if( innkeeper->has_actflag( ACT_INNKEEPER ) )
         return innkeeper;
   }
   return NULL;
}

void scan_rares( char_data * ch )
{
   expire_items( ch );

   if( !ch->is_immortal(  ) )
   {
      switch ( ch->pcdata->camp )
      {
         default:
            break;
         case 0:
            log_printf_plus( LOG_COMM, LEVEL_IMMORTAL, "%s checks out of the inn.", ch->name );
            break;
         case 2:
            log_printf_plus( LOG_COMM, LEVEL_IMMORTAL, "%s reconnecting after idling out.", ch->name );
            break;
         case 3:
            log_printf_plus( LOG_COMM, LEVEL_IMMORTAL, "%s reconnecting after quitting.", ch->name );
            break;
      }
   }
   else
   {
      log_printf_plus( LOG_COMM, ch->level, "%s returns from beyond the void.", ch->name );
      show_stateflags( ch );
   }
}

CMDF( do_rent )
{
   char_data *innkeeper, *victim;
   int level = ch->get_trust(  );

   if( !( innkeeper = find_innkeeper( ch ) ) )
   {
      ch->print( "You can only rent at an inn.\r\n" );
      return;
   }

   victim = innkeeper;

   if( ch->isnpc(  ) )
   {
      ch->print( "Get Real! Mobs can't rent!\r\n" );
      return;
   }

   if( auction->item != NULL && ( ( ch == auction->buyer ) || ( ch == auction->seller ) ) )
   {
      ch->print( "Wait until you have bought/sold the item on auction.\r\n" );
      return;
   }

   act( AT_WHITE, "$n takes your equipment into storage, and shows you to your room.", victim, NULL, ch, TO_VICT );
   act( AT_SAY, "A strange voice says, 'We await your return, $n...'", ch, NULL, NULL, TO_CHAR );
   ch->print( "&d\r\n" );
   act( AT_BYE, "$n shows $N to $S room, and stores $S equipment.", victim, NULL, ch, TO_NOTVICT );

   log_printf_plus( LOG_COMM, level, "%s rented in: %s, %s", ch->name, ch->in_room->name, ch->in_room->area->name );

   char_leaving( ch, 0 );
}

/*
 * Make a campfire.
 */
void make_campfire( room_index * in_room, char_data * ch, short timer )
{
   obj_data *fire;

   if( !( fire = get_obj_index( OBJ_VNUM_CAMPFIRE )->create_object( 1 ) ) )
   {
      log_printf( "create_object: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
      return;
   }
   fire->timer = number_fuzzy( timer );
   fire->to_room( in_room, ch );
}

CMDF( do_camp )
{
   if( ch->isnpc(  ) )
   {
      ch->print( "Mobs cannot camp!\r\n" );
      return;
   }

   if( !ch->has_pcflag( PCFLAG_ONMAP ) )
   {
      if( ch->in_room->flags.test( ROOM_NOCAMP ) )
      {
         ch->print( "You may not setup camp in this spot, find another.\r\n" );
         return;
      }

      if( ch->in_room->area->flags.test( AFLAG_NOCAMP ) )
      {
         ch->print( "It is not safe to camp in this area!\r\n" );
         return;
      }

      if( ch->in_room->flags.test( ROOM_INDOORS ) || ch->in_room->flags.test( ROOM_CAVE ) || ch->in_room->flags.test( ROOM_CAVERN ) )
      {
         ch->print( "You must be outdoors to make camp.\r\n" );
         return;
      }

      switch ( ch->in_room->sector_type )
      {
         case SECT_UNDERWATER:
         case SECT_OCEANFLOOR:
            ch->print( "You cannot camp underwater, you'd drown!\r\n" );
            return;
         case SECT_RIVER:
            ch->print( "The river would sweep any such camp away!\r\n" );
            return;
         case SECT_WATER_SWIM:
         case SECT_WATER_NOSWIM:
         case SECT_OCEAN:
            ch->print( "You cannot camp on the open water like that!\r\n" );
            /*
             * At some future date, add code to check for a large boat, assuming
             * we ever get code to support boats of any real size 
             */
            return;
         case SECT_AIR:
            ch->print( "Yeah, sure, set camp in thin air???\r\n" );
            return;
         case SECT_CITY:
         case SECT_ROAD:
         case SECT_BRIDGE:
            ch->print( "This spot is too heavily travelled to setup camp.\r\n" );
            return;
         case SECT_INDOORS:
            ch->print( "You must be outdoors to make camp.\r\n" );
            return;
         case SECT_ICE:
            ch->print( "It isn't safe to setup camp on the ice.\r\n" );
            return;
         case SECT_LAVA:
            ch->print( "What? You want to barbecue yourself?\r\n" );
            return;
         default:
            break;
      }
   }
   else
   {
      short sector = map_sector[ch->cmap][ch->mx][ch->my];

      switch ( sector )
      {
         case SECT_RIVER:
            ch->print( "The river would sweep any such camp away!\r\n" );
            return;
         case SECT_WATER_SWIM:
         case SECT_WATER_NOSWIM:
            ch->print( "You cannot camp on the open water like that!\r\n" );
            /*
             * At some future date, add code to check for a large boat, assuming
             * we ever get code to support boats of any real size 
             */
            return;
         case SECT_CITY:
         case SECT_ROAD:
         case SECT_BRIDGE:
            ch->print( "This spot is too heavily travelled to setup camp.\r\n" );
            return;
         case SECT_ICE:
            ch->print( "It isn't safe to setup camp on the ice.\r\n" );
            return;
         default:
            break;
      }
   }

   if( auction->item != NULL && ( ( ch == auction->buyer ) || ( ch == auction->seller ) ) )
   {
      ch->print( "Wait until you have bought/sold the item on auction.\r\n" );
      return;
   }

   list < obj_data * >::iterator iobj;
   bool fbed = false, fgear = false, flint = false;
   for( iobj = ch->carrying.begin(  ); iobj != ch->carrying.end(  ); ++iobj )
   {
      obj_data *obj = *iobj;
      if( obj->item_type == ITEM_CAMPGEAR )
      {
         if( obj->value[0] == 1 )
            fbed = true;
         if( obj->value[0] == 2 )
            fgear = true;
         if( obj->value[0] == 3 )
            flint = true;
      }
   }

   if( !fbed || !fgear || !flint )
   {
      ch->print( "You must have a bedroll and camping gear before making camp.\r\n" );
      return;
   }

   bool fire = false;

   for( iobj = ch->in_room->objects.begin(  ); iobj != ch->in_room->objects.end(  ); ++iobj )
   {
      obj_data *campfire = *iobj;
      if( campfire->item_type == ITEM_FIRE )
      {
         if( ch->has_pcflag( PCFLAG_ONMAP ) )
         {
            if( is_same_obj_map( ch, campfire ) )
            {
               fire = true;
               break;
            }
         }

         if( !ch->has_pcflag( PCFLAG_ONMAP ) )
         {
            fire = true;
            break;
         }
      }
   }
   if( !fire )
      make_campfire( ch->in_room, ch, 40 );

   ch->print( "After tending to your fire and securing your belongings, you make camp for the night.\r\n" );
   act( AT_GREEN, "$n secures $s belongings and makes camp for the night.\r\n", ch, NULL, NULL, TO_ROOM );

   log_printf( "%s has made camp for the night in %s.", ch->name, ch->in_room->area->name );
   char_leaving( ch, 1 );
}

/* Take one rare item away once found - Samson 9-19-98 */
int thief_raid( char_data * ch, obj_data * obj, int robbed )
{
   if( robbed == 0 )
   {
      if( obj->ego >= sysdata->minego )
      {
         ch->printf( "&YThe thieves stole %s!\r\n", obj->short_descr );
         log_printf_plus( LOG_COMM, LEVEL_IMMORTAL, "Thieves stole %s from %s!", obj->short_descr, ch->name );
         obj->separate(  );
         obj->extract(  );
         robbed = 1;
      }
      list < obj_data * >::iterator iobj;
      for( iobj = obj->contents.begin(  ); iobj != obj->contents.end(  ); ++iobj )
      {
         obj_data *tobj = *iobj;
         thief_raid( ch, tobj, robbed );
      }
   }
   return robbed;
}

/* Take all rare items away - Samson 9-19-98 */
int bandit_raid( char_data * ch, obj_data * obj, int robbed )
{
   if( obj->ego >= sysdata->minego )
   {
      ch->printf( "&YThe bandits stole %s!\r\n", obj->short_descr );
      log_printf_plus( LOG_COMM, LEVEL_IMMORTAL, "Bandits stole %s from %s!", obj->short_descr, ch->name );
      obj->extract(  );
      robbed = 1;
   }
   list < obj_data * >::iterator iobj;
   for( iobj = obj->contents.begin(  ); iobj != obj->contents.end(  ); ++iobj )
   {
      obj_data *tobj = *iobj;
      bandit_raid( ch, tobj, robbed );
   }
   return robbed;
}

/* Run through and check player camp to see if they got robbed among other things.
   Samson 9-19-98 */
void break_camp( char_data * ch )
{
   int robbed = 0, robchance = number_range( 1, 100 );

   if( robchance > 85 )
   {
      list < obj_data * >::iterator iobj;
      if( robchance < 98 )
      {
         log_printf_plus( LOG_COMM, LEVEL_IMMORTAL, "Thieves raided %s's camp!", ch->name );
         ch->print( "&RYour camp was visited by thieves while you were away!\r\n" );
         ch->print( "&RYour belongings have been rummaged through....\r\n" );

         for( iobj = ch->carrying.begin(  ); iobj != ch->carrying.end(  ); ++iobj )
         {
            obj_data *obj = *iobj;
            robbed = thief_raid( ch, obj, robbed );
         }
      }
      else
      {
         log_printf_plus( LOG_COMM, LEVEL_IMMORTAL, "Bandits raided %s's camp!", ch->name );
         ch->print( "&RYour camp was visited by bandits while you were away!\r\n" );
         ch->print( "&RYour belongings have been rummaged through....\r\n" );

         for( iobj = ch->carrying.begin(  ); iobj != ch->carrying.end(  ); ++iobj )
         {
            obj_data *obj = *iobj;
            robbed = bandit_raid( ch, obj, robbed );
         }
      }
   }
   log_printf_plus( LOG_COMM, LEVEL_IMMORTAL, "%s breaks camp and enters the game.", ch->name );
   ch->pcdata->camp = 0;
}

/*
 * Bring up the pfile for rare item adjustments
 */
void adjust_pfile( const string & name )
{
   char_data *ch;
   char fname[256];
   struct stat fst;

   list < char_data * >::iterator ich;
   for( ich = pclist.begin(  ); ich != pclist.end(  ); ++ich )
   {
      char_data *temp = *ich;

      if( !str_cmp( name, temp->name ) )
      {
         log_printf( "Skipping rare item adjustments for %s, player is online.", temp->name );
         if( temp->is_immortal(  ) )   /* Get the rare items off the immortals */
         {
            list < obj_data * >::iterator iobj;

            log_printf( "Immortal: Removing rare items from %s.", temp->name );
            for( iobj = temp->carrying.begin(  ); iobj != temp->carrying.end(  ); )
            {
               obj_data *tobj = *iobj;
               ++iobj;

               rare_purge( temp, tobj );
            }
         }
         return;
      }
   }

   room_index *temproom, *original;
   if( !( temproom = get_room_index( ROOM_VNUM_RAREUPDATE ) ) )
   {
      bug( "%s: Error in rare item adjustment, temporary loading room is missing!", __FUNCTION__ );
      return;
   }

   snprintf( fname, 256, "%s%c/%s", PLAYER_DIR, tolower( name[0] ), capitalize( name ).c_str(  ) );

   if( stat( fname, &fst ) != -1 )
   {
      descriptor_data *d = new descriptor_data;
      d->init(  );
      d->connected = CON_PLOADED;

      load_char_obj( d, name, false, false );
      charlist.push_back( d->character );
      pclist.push_back( d->character );
      original = d->character->in_room;
      if( !d->character->to_room( temproom ) )
         log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
      ch = d->character;   /* Hopefully this will work, if not, we're SOL */
      d->character->desc = NULL;
      d->character = NULL;
      deleteptr( d );

      log_printf( "Updating rare items for %s", ch->name );

      ++ch->pcdata->daysidle;
      expire_items( ch );

      ch->from_room(  );
      if( !ch->to_room( original ) )
         log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );

      quitting_char = ch;
      ch->save(  );

      if( sysdata->save_pets )
      {
         list < char_data * >::iterator pet;

         for( pet = ch->pets.begin(  ); pet != ch->pets.end(  ); )
         {
            char_data *cpet = *pet;
            ++pet;

            cpet->extract( true );
         }
      }

      /*
       * Synch clandata up only when clan member quits now. --Shaddai 
       */
      if( ch->pcdata->clan )
         save_clan( ch->pcdata->clan );

      saving_char = NULL;

      /*
       * After extract_char the ch is no longer valid!
       */
      ch->extract( true );
      for( int x = 0; x < MAX_WEAR; ++x )
         for( int y = 0; y < MAX_LAYERS; ++y )
            save_equipment[x][y] = NULL;

      log_printf( "Rare items for %s updated sucessfully.", name.c_str(  ) );
   }
}

/* Rare item counting function taken from the Tartarus codebase, a
 * ROM 2.4b derivitive by Ceran. Modified for Smaug compatibility by Samson
 */
int scan_pfiles( const char *dirname, const char *filename, bool updating )
{
   FILE *fpChar;
   char fname[256];
   int adjust = 0;

   snprintf( fname, 256, "%s/%s", dirname, filename );

   // log_string( fname );

   if( !( fpChar = fopen( fname, "r" ) ) )
   {
      perror( fname );
      return 0;
   }

   for( ;; )
   {
      int vnum = 0, counter = 1;
      char letter;
      const char *word;
      string tempstring;
      obj_index *pObjIndex = NULL;

      letter = fread_letter( fpChar );

      if( ( letter != '#' ) && ( !feof( fpChar ) ) )
         continue;

      word = ( feof( fpChar ) ? "End" : fread_word( fpChar ) );

      if( word[0] == '\0' )
      {
         bug( "%s: EOF encountered reading file!", __FUNCTION__ );
         word = "End";
      }

      // log_string( word );

      if( !str_cmp( word, "End" ) )
         break;

      if( !str_cmp( word, "OBJECT" ) )
      {
         word = ( feof( fpChar ) ? "End" : fread_word( fpChar ) );

         if( word[0] == '\0' )
         {
            bug( "%s: EOF encountered reading file!", __FUNCTION__ );
            word = "End";
         }

         if( !str_cmp( word, "End" ) )
            break;

         if( !str_cmp( word, "Nest" ) )
         {
            fread_number( fpChar );
            word = feof( fpChar ) ? "End" : fread_word( fpChar );
         }

         if( !str_cmp( word, "Count" ) )
         {
            counter = fread_number( fpChar );
            word = feof( fpChar ) ? "End" : fread_word( fpChar );
         }

         if( !str_cmp( word, "Name" ) )
         {
            tempstring = fread_flagstring( fpChar );
            word = feof( fpChar ) ? "End" : fread_word( fpChar );
         }

         if( !str_cmp( word, "ShortDescr" ) )
         {
            tempstring = fread_flagstring( fpChar );
            word = feof( fpChar ) ? "End" : fread_word( fpChar );
         }

         if( !str_cmp( word, "Description" ) )
         {
            tempstring = fread_flagstring( fpChar );
            word = feof( fpChar ) ? "End" : fread_word( fpChar );
         }

         if( !str_cmp( word, "ActionDesc" ) )
         {
            tempstring = fread_flagstring( fpChar );
            word = feof( fpChar ) ? "End" : fread_word( fpChar );
         }

         if( !str_cmp( word, "Ovnum" ) )
         {
            vnum = fread_number( fpChar );
            if( ( pObjIndex = get_obj_index( vnum ) ) == NULL )
            {
               bug( "%s: %s has bad obj vnum.", __FUNCTION__, filename );
               adjust = 1; /* So it can clean out the bad object - Samson 4-16-00 */
            }
            word = feof( fpChar ) ? "End" : fread_word( fpChar );
         }

         if( !str_cmp( word, "Ego" ) && pObjIndex )
         {
            int ego = fread_number( fpChar );
            if( ego >= sysdata->minego )
            {
               if( !updating )
               {
                  pObjIndex->count += counter;
                  log_printf( "%s: Counted %d of Vnum %d", filename, counter, vnum );
               }
               else
                  adjust = 1;
            }
         }
      }
   }
   FCLOSE( fpChar );
   return adjust;
}

void corpse_scan( const char *dirname, const char *filename )
{
   FILE *fpChar;
   char fname[256];

   snprintf( fname, 256, "%s/%s", dirname, filename );

   if( !( fpChar = fopen( fname, "r" ) ) )
   {
      perror( fname );
      return;
   }

   for( ;; )
   {
      int vnum, counter = 1;
      char letter;
      const char *word;
      obj_index *pObjIndex;

      letter = fread_letter( fpChar );

      if( ( letter != '#' ) && ( !feof( fpChar ) ) )
         continue;

      word = ( feof( fpChar ) ? "End" : fread_word( fpChar ) );

      if( word[0] == '\0' )
      {
         bug( "%s: EOF encountered reading file!", __FUNCTION__ );
         word = "End";
      }

      if( !str_cmp( word, "End" ) )
         break;

      if( !str_cmp( word, "OBJECT" ) )
      {
         word = ( feof( fpChar ) ? "End" : fread_word( fpChar ) );

         if( word[0] == '\0' )
         {
            bug( "%s: EOF encountered reading file!", __FUNCTION__ );
            word = "End";
         }

         if( !str_cmp( word, "End" ) )
            break;

         if( !str_cmp( word, "Nest" ) )
         {
            fread_number( fpChar );
            word = feof( fpChar ) ? "End" : fread_word( fpChar );
         }

         if( !str_cmp( word, "Count" ) )
         {
            counter = fread_number( fpChar );
            word = feof( fpChar ) ? "End" : fread_word( fpChar );
         }

         if( !str_cmp( word, "Ovnum" ) )
         {
            vnum = fread_number( fpChar );
            if( ( get_obj_index( vnum ) ) == NULL )
               bug( "%s: %s's corpse has bad obj vnum.", __FUNCTION__, filename );
            else
            {
               int ego = 0;
               pObjIndex = get_obj_index( vnum );
               if( pObjIndex->ego == -2 )
                  ego = pObjIndex->set_ego(  );
               if( ego >= sysdata->minego )
               {
                  pObjIndex->count += counter;
                  log_printf( "%s: Counted %d of Vnum %d", filename, counter, vnum );
               }
            }
         }
      }
   }
   FCLOSE( fpChar );
}

void mobfile_scan( void )
{
   FILE *fpChar;
   char fname[256];

   snprintf( fname, 256, "%s%s", SYSTEM_DIR, MOB_FILE );

   if( !( fpChar = fopen( fname, "r" ) ) )
   {
      perror( fname );
      return;
   }

   for( ;; )
   {
      int vnum, counter = 1;
      char letter;
      const char *word;
      obj_index *pObjIndex;

      letter = fread_letter( fpChar );

      if( ( letter != '#' ) && ( !feof( fpChar ) ) )
         continue;

      word = ( feof( fpChar ) ? "End" : fread_word( fpChar ) );

      if( word[0] == '\0' )
      {
         bug( "%s: EOF encountered reading file!", __FUNCTION__ );
         word = "End";
      }

      if( !str_cmp( word, "End" ) )
         break;

      if( !str_cmp( word, "OBJECT" ) )
      {
         word = ( feof( fpChar ) ? "End" : fread_word( fpChar ) );

         if( word[0] == '\0' )
         {
            bug( "%s: EOF encountered reading file!", __FUNCTION__ );
            word = "End";
         }

         if( !str_cmp( word, "End" ) )
            break;

         if( !str_cmp( word, "Nest" ) )
         {
            fread_number( fpChar );
            word = feof( fpChar ) ? "End" : fread_word( fpChar );
         }

         if( !str_cmp( word, "Count" ) )
         {
            counter = fread_number( fpChar );
            word = feof( fpChar ) ? "End" : fread_word( fpChar );
         }

         if( !str_cmp( word, "Ovnum" ) )
         {
            vnum = fread_number( fpChar );
            if( ( get_obj_index( vnum ) ) == NULL )
               bug( "%s: bad obj vnum %d.", __FUNCTION__, vnum );
            else
            {
               int ego = 0;
               pObjIndex = get_obj_index( vnum );
               if( pObjIndex->ego == -2 )
                  ego = pObjIndex->set_ego(  );
               if( ego >= sysdata->minego )
               {
                  pObjIndex->count += counter;
                  log_printf( "%s: Counted %d of Vnum %d", fname, counter, vnum );
               }
            }
         }
      }
   }
   FCLOSE( fpChar );
}

void objfile_scan( const char *dirname, const char *filename )
{
   FILE *fpChar;
   char fname[256];

   snprintf( fname, 256, "%s%s", dirname, filename );

   if( !( fpChar = fopen( fname, "r" ) ) )
   {
      perror( fname );
      return;
   }

   for( ;; )
   {
      int vnum, counter = 1;
      char letter;
      const char *word;
      obj_index *pObjIndex;

      letter = fread_letter( fpChar );

      if( ( letter != '#' ) && ( !feof( fpChar ) ) )
         continue;

      word = ( feof( fpChar ) ? "End" : fread_word( fpChar ) );

      if( word[0] == '\0' )
      {
         bug( "%s: EOF encountered reading file!", __FUNCTION__ );
         word = "End";
      }

      if( !str_cmp( word, "End" ) )
         break;

      if( !str_cmp( word, "OBJECT" ) )
      {
         word = ( feof( fpChar ) ? "End" : fread_word( fpChar ) );

         if( word[0] == '\0' )
         {
            bug( "%s: EOF encountered reading file!", __FUNCTION__ );
            word = "End";
         }

         if( !str_cmp( word, "End" ) )
            break;

         if( !str_cmp( word, "Nest" ) )
         {
            fread_number( fpChar );
            word = feof( fpChar ) ? "End" : fread_word( fpChar );
         }

         if( !str_cmp( word, "Count" ) )
         {
            counter = fread_number( fpChar );
            word = feof( fpChar ) ? "End" : fread_word( fpChar );
         }

         if( !str_cmp( word, "Ovnum" ) )
         {
            vnum = fread_number( fpChar );
            if( ( get_obj_index( vnum ) ) == NULL )
               bug( "%s: bad obj vnum %d.", __FUNCTION__, vnum );
            else
            {
               int ego = 0;
               pObjIndex = get_obj_index( vnum );
               if( pObjIndex->ego == -2 )
                  ego = pObjIndex->set_ego(  );
               if( ego >= sysdata->minego )
               {
                  pObjIndex->count += counter;
                  log_printf( "%s: Counted %d of Vnum %d", fname, counter, vnum );
               }
            }
         }
      }
   }
   FCLOSE( fpChar );
}

void load_equipment_totals( bool fCopyOver )
{
   DIR *dp;
   struct dirent *dentry;
   char directory_name[100];
   short alpha_loop;

   check_pfiles( 255 ); /* Clean up stragglers to get a better count - Samson 1-1-00 */

   log_string( "Updating rare item counts....." );
   log_string( "Checking player files...." );

   for( alpha_loop = 0; alpha_loop <= 25; ++alpha_loop )
   {
      snprintf( directory_name, 100, "%s%c", PLAYER_DIR, 'a' + alpha_loop );

      dp = opendir( directory_name );
      dentry = readdir( dp );
      while( dentry )
      {
         /*
          * Added by Tarl 3 Dec 02 because we are now using CVS 
          */
         if( !str_cmp( dentry->d_name, "CVS" ) )
         {
            dentry = readdir( dp );
            continue;
         }
         if( dentry->d_name[0] != '.' )
         {
            scan_pfiles( directory_name, dentry->d_name, false );
         }
         dentry = readdir( dp );
      }
      closedir( dp );
   }

   log_string( "Checking corpses...." );

   snprintf( directory_name, 100, "%s", CORPSE_DIR );
   dp = opendir( directory_name );
   dentry = readdir( dp );
   while( dentry )
   {
      /*
       * Added by Tarl 3 Dec 02 because we are now using CVS 
       */
      if( !str_cmp( dentry->d_name, "CVS" ) )
      {
         dentry = readdir( dp );
         continue;
      }
      if( dentry->d_name[0] != '.' )
      {
         corpse_scan( directory_name, dentry->d_name );
      }
      dentry = readdir( dp );
   }
   closedir( dp );

   if( fCopyOver )
   {
      log_string( "Scanning world-state mob file...." );
      mobfile_scan(  );

      log_string( "Scanning world-state obj files...." );
      snprintf( directory_name, 100, "%s", HOTBOOT_DIR );
      dp = opendir( directory_name );
      dentry = readdir( dp );
      while( dentry )
      {
         /*
          * Added by Tarl 3 Dec 02 because we are now using CVS 
          */
         if( !str_cmp( dentry->d_name, "CVS" ) )
         {
            dentry = readdir( dp );
            continue;
         }
         if( dentry->d_name[0] != '.' )
         {
            objfile_scan( directory_name, dentry->d_name );
         }
         dentry = readdir( dp );
      }
      closedir( dp );
   }
}

void rare_update( void )
{
   DIR *dp;
   struct dirent *dentry;
   char directory_name[100];
   int adjust = 0;
   short alpha_loop;

   log_string( "Checking daily rare items for players...." );

   for( alpha_loop = 0; alpha_loop <= 25; ++alpha_loop )
   {
      snprintf( directory_name, 100, "%s%c", PLAYER_DIR, 'a' + alpha_loop );
      dp = opendir( directory_name );
      dentry = readdir( dp );
      while( dentry )
      {
         /*
          * Added by Tarl 3 Dec 02 because we are now using CVS 
          */
         if( !str_cmp( dentry->d_name, "CVS" ) )
         {
            dentry = readdir( dp );
            continue;
         }
         if( dentry->d_name[0] != '.' )
         {
            adjust = scan_pfiles( directory_name, dentry->d_name, true );
            if( adjust == 1 )
            {
               adjust_pfile( dentry->d_name );
               adjust = 0;
            }
         }
         dentry = readdir( dp );
      }
      closedir( dp );
   }
   log_string( "Daily rare item updates completed." );
}
