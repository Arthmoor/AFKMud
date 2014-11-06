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
 *                    Shop, Banking, and Repair Module                      *
 ****************************************************************************/

/* 

Clan shopkeeper section installed by Samson 7-16-00 
Code used is based on the Vendor snippet provided by:

Vendor Snippit V1.01
By: Jimmy M. (Meckteck and Legonas, depending on where i am)
E-Mail: legonas@netzero.net
ICQ #28394032
Released 02/28/00

Tested and Created on the Smaug1.4a code base.

----------------------------------------
Northwind: home of Legends             +
http://www.kilnar.com/~northwind/      +
telnet://northwind.kilnar.com:5555/    +
----------------------------------------

*/

#include <dirent.h>
#include "mud.h"
#include "area.h"
#include "clans.h"
#include "descriptor.h"
#include "mobindex.h"
#include "mud_prog.h"
#include "objindex.h"
#include "roomindex.h"
#include "shops.h"

void auction_sell( char_data *, char_data *, string & );
void auction_buy( char_data *, char_data *, const string & );
void auction_value( char_data *, char_data *, const string & );
bool can_wear_obj( char_data *, obj_data * );
bool can_mmodify( char_data *, char_data * );
void bind_follower( char_data *, char_data *, int, int );
char_data *find_auctioneer( char_data * );

list < shop_data * >shoplist;
list < repair_data * >repairlist;

void fwrite_mobile( char_data *, FILE *, bool );
char_data *fread_mobile( FILE *, bool );

void mprog_sell_trigger( char_data *, char_data *, obj_data * );

/*
 * Local functions
 */
int get_cost( char_data *, char_data *, obj_data *, bool );
int get_repaircost( char_data *, obj_data * );

void save_shop( char_data * mob )
{
   FILE *fp = NULL;
   char filename[256];

   if( !mob->isnpc(  ) )
      return;

   snprintf( filename, 256, "%s%s", SHOP_DIR, capitalize( mob->short_descr ) );

   if( !( fp = fopen( filename, "w" ) ) )
   {
      bug( "%s: fopen", __FUNCTION__ );
      perror( filename );
   }
   fwrite_mobile( mob, fp, true );
   fprintf( fp, "%s", "#END\n" );
   FCLOSE( fp );
}

void load_shopkeepers( void )
{
   DIR *dp;
   struct dirent *dentry;
   char directory_name[100];

   snprintf( directory_name, 100, "%s", SHOP_DIR );
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
         FILE *fp;
         char filename[256];

         snprintf( filename, 256, "%s%s", directory_name, dentry->d_name );
         if( ( fp = fopen( filename, "r" ) ) != NULL )
         {
            char_data *mob = NULL;

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
               if( !strcmp( word, "SHOP" ) )
                  mob = fread_mobile( fp, true );
               else if( !strcmp( word, "OBJECT" ) )
               {
                  mob->tempnum = -9999;
                  fread_obj( mob, fp, OS_CARRY );
               }
               else if( !strcmp( word, "END" ) )
                  break;
            }
            FCLOSE( fp );
            if( mob )
            {
               list < clan_data * >::iterator cl;
               for( cl = clanlist.begin(  ); cl != clanlist.end(  ); ++cl )
               {
                  clan_data *clan = *cl;

                  if( clan->shopkeeper == mob->pIndexData->vnum && clan->bank )
                  {
                     clan->balance += mob->gold;
                     mob->gold = 0;
                     save_shop( mob );
                  }
               }
               list < obj_data * >::iterator iobj;
               for( iobj = mob->carrying.begin(  ); iobj != mob->carrying.end(  ); )
               {
                  obj_data *obj = *iobj;
                  ++iobj;

                  if( obj->ego >= sysdata->minego )
                     obj->extract(  );
               }
            }
         }
      }
      dentry = readdir( dp );
   }
   closedir( dp );
}

/*
 * Shopping commands.
 */
/* Modified to make keepers say when they open/close - Samson 4-23-98 */
char_data *find_keeper( char_data * ch )
{
   char_data *keeper = NULL;
   list < char_data * >::iterator ich;
   shop_data *pShop;

   pShop = NULL;
   for( ich = ch->in_room->people.begin(  ); ich != ch->in_room->people.end(  ); ++ich )
   {
      keeper = *ich;
      if( keeper->isnpc(  ) && ( pShop = keeper->pIndexData->pShop ) != NULL )
         break;
   }

   if( !pShop )
   {
      ch->print( "You can't do that here.\r\n" );
      return NULL;
   }

   /*
    * Disallow sales during battle
    */
   char_data *whof;
   if( ( whof = keeper->who_fighting(  ) ) != NULL )
   {
      if( whof == ch )
         ch->print( "I don't think that's a good idea...\r\n" );
      else
         interpret( keeper, "say I'm too busy for that!" );
      return NULL;
   }

   if( ch->who_fighting(  ) )
   {
      ch->printf( "%s doesn't seem to want to get involved.\r\n", PERS( keeper, ch, false ) );
      return NULL;
   }

   /*
    * Check to see if show is open.
    * Supports closing times after midnight
    */
   if( pShop->open_hour > pShop->close_hour )
   {
      if( time_info.hour < pShop->open_hour && time_info.hour > pShop->close_hour )
      {
         if( pShop->open_hour < sysdata->hournoon )
            cmdf( keeper, "say Sorry, come back later. I open at %dam.", ( pShop->open_hour == 0 ) ? ( pShop->open_hour + sysdata->hournoon ) : ( pShop->open_hour ) );
         else
            cmdf( keeper, "say Sorry, come back later. I open at %dpm.",
                  ( pShop->open_hour == sysdata->hournoon ) ? ( pShop->open_hour ) : ( pShop->open_hour - sysdata->hournoon ) );
         return NULL;
      }
   }
   else  /* Severely hacked up to work with Alsherok's 28hr clock - Samson 5-13-99 */
   {
      if( time_info.hour < pShop->open_hour )
      {
         if( pShop->open_hour < sysdata->hournoon )
            cmdf( keeper, "say Sorry, come back later. I open at %dam.", ( pShop->open_hour == 0 ) ? ( pShop->open_hour + sysdata->hournoon ) : ( pShop->open_hour ) );
         else
            cmdf( keeper, "say Sorry, come back later. I open at %dpm.",
                  ( pShop->open_hour == sysdata->hournoon ) ? ( pShop->open_hour ) : ( pShop->open_hour - sysdata->hournoon ) );
         return NULL;
      }
      if( time_info.hour > pShop->close_hour )
      {
         if( pShop->close_hour < sysdata->hournoon )
            cmdf( keeper, "say Sorry, come back tomorrow. I close at %dam.", ( pShop->close_hour == 0 ) ? ( pShop->close_hour + sysdata->hournoon ) : ( pShop->close_hour ) );
         else
            cmdf( keeper, "say Sorry, come back tomorrow. I close at %dpm.",
                  ( pShop->close_hour == sysdata->hournoon ) ? ( pShop->close_hour ) : ( pShop->close_hour - sysdata->hournoon ) );
         return NULL;
      }
   }

   if( keeper->position == POS_SLEEPING )
   {
      ch->print( "While they're asleep?\r\n" );
      return NULL;
   }

   if( keeper->position < POS_SLEEPING )
   {
      ch->print( "I don't think they can hear you...\r\n" );
      return NULL;
   }

   /*
    * Invisible or hidden people.
    */
   if( !keeper->can_see( ch, false ) )
   {
      interpret( keeper, "say I don't trade with folks I can't see." );
      return NULL;
   }

   int speakswell = UMIN( knows_language( keeper, ch->speaking, ch ), knows_language( ch, ch->speaking, keeper ) );

   if( ( number_percent(  ) % 65 ) > speakswell )
   {
      if( speakswell > 60 )
         cmdf( keeper, "say %s, Could you repeat that? I didn't quite catch it.", ch->name );
      else if( speakswell > 50 )
         cmdf( keeper, "say %s, Could you say that a little more clearly please?", ch->name );
      else if( speakswell > 40 )
         cmdf( keeper, "say %s, Sorry... What was that you wanted?", ch->name );
      else
         cmdf( keeper, "say %s, I can't understand you.", ch->name );
      return NULL;
   }
   return keeper;
}

CMDF( do_setprice )
{
   char_data *keeper;
   string arg1;
   obj_data *obj;
   bool found = false;

   if( ch->isnpc(  ) )
   {
      ch->print( "Mobs cannot use this command.\r\n" );
      return;
   }

   if( !ch->pcdata->clan )
   {
      ch->print( "But you aren't even in a clan or guild!\r\n" );
      return;
   }

   if( !( keeper = find_keeper( ch ) ) )
   {
      ch->print( "What shopkeeper?\r\n" );
      return;
   }

   argument = one_argument( argument, arg1 );

   if( arg1.empty(  ) || argument.empty(  ) )
   {
      ch->print( "Syntax: setprice <item> <price>\r\n" );
      ch->print( "Item should be something the shopkeeper already has.\r\n" );
      ch->print( "Give the shopkeeper an item to list it for sale.\r\n" );
      ch->print( "To remove an item from his inventory, set the price to -1.\r\n" );
      return;
   }

   if( ch->fighting )
   {
      ch->print( "Not during combat!\r\n" );
      return;
   }

   if( !keeper->has_actflag( ACT_GUILDVENDOR ) )
   {
      ch->print( "This shopkeeper doesn't work for a clan or guild!\r\n" );
      return;
   }

   clan_data *clan = NULL;
   list < clan_data * >::iterator cl;
   for( cl = clanlist.begin(  ); cl != clanlist.end(  ); ++cl )
   {
      clan = *cl;

      if( clan->shopkeeper == keeper->pIndexData->vnum )
      {
         found = true;
         break;
      }
   }

   if( found && clan != ch->pcdata->clan )
   {
      ch->printf( "Beat it! I don't work for your %s!\r\n", clan->clan_type == CLAN_GUILD ? "guild" : "clan" );
      return;
   }

   if( str_cmp( ch->name, clan->leader ) && str_cmp( ch->name, clan->number1 ) && str_cmp( ch->name, clan->number2 ) )
   {
      ch->printf( "Only the %s admins can set prices on inventory.\r\n", clan->clan_type == CLAN_GUILD ? "guild" : "clan" );
      return;
   }

   if( ( obj = keeper->get_obj_carry( arg1 ) ) != NULL )
   {
      int price;

      price = atoi( argument.c_str(  ) );

      if( price < -1 || price > 2000000000 )
      {
         ch->print( "Valid price range is between -1 and 2 billion.\r\n" );
         return;
      }

      if( price == -1 )
      {
         obj->from_char(  );
         obj->to_char( ch );
         obj->cost = obj->pIndexData->cost;
         ch->printf( "%s has been removed from sale.\r\n", obj->short_descr );
         ch->save(  );
      }
      else
      {
         obj->cost = price;
         ch->printf( "The price for %s has been changed to %d.\r\n", obj->short_descr, price );
      }
      save_shop( keeper );
      return;
   }
   ch->print( "He doesnt have that item!\r\n" );
}

/*
 * repair commands.
 */
/* Modified to make keeper show open/close hour - Samson 4-23-98 */
char_data *find_fixer( char_data * ch )
{
   char_data *keeper = NULL;
   list < char_data * >::iterator ich;
   repair_data *rShop = NULL;

   for( ich = ch->in_room->people.begin(  ); ich != ch->in_room->people.end(  ); ++ich )
   {
      keeper = *ich;
      if( keeper->isnpc(  ) && ( rShop = keeper->pIndexData->rShop ) != NULL )
         break;
   }

   if( !rShop )
   {
      ch->print( "You can't do that here.\r\n" );
      return NULL;
   }

   /*
    * Disallow sales during battle
    */
   char_data *whof;
   if( ( whof = keeper->who_fighting(  ) ) != NULL )
   {
      if( whof == ch )
         ch->print( "I don't think that's a good idea...\r\n" );
      else
         interpret( keeper, "say I'm too busy for that!" );
      return NULL;
   }

   /*
    * According to rlog, this is the second time I've done this
    * * so mobiles can repair in combat.  -- Blod, 1/98
    */
   if( !ch->isnpc(  ) && ch->who_fighting(  ) )
   {
      ch->printf( "%s doesn't seem to want to get involved.\r\n", PERS( keeper, ch, false ) );
      return NULL;
   }

   /*
    * Check to see if show is open.
    * Supports closing times after midnight
    */
   if( rShop->open_hour > rShop->close_hour )
   {
      if( time_info.hour < rShop->open_hour && time_info.hour > rShop->close_hour )
      {
         if( rShop->open_hour < sysdata->hournoon )
            cmdf( keeper, "say Sorry, come back later. I open at %dam.", ( rShop->open_hour == 0 ) ? ( rShop->open_hour + sysdata->hournoon ) : ( rShop->open_hour ) );
         else
            cmdf( keeper, "say Sorry, come back later. I open at %dpm.",
                  ( rShop->open_hour == sysdata->hournoon ) ? ( rShop->open_hour ) : ( rShop->open_hour - sysdata->hournoon ) );
         return NULL;
      }
   }
   else  /* Severely hacked up to work with Alsherok's 28hr clock - Samson 5-13-99 */
   {
      if( time_info.hour < rShop->open_hour )
      {
         if( rShop->open_hour < sysdata->hournoon )
            cmdf( keeper, "say Sorry, come back later. I open at %dam.", ( rShop->open_hour == 0 ) ? ( rShop->open_hour + sysdata->hournoon ) : ( rShop->open_hour ) );
         else
            cmdf( keeper, "say Sorry, come back later. I open at %dpm.",
                  ( rShop->open_hour == sysdata->hournoon ) ? ( rShop->open_hour ) : ( rShop->open_hour - sysdata->hournoon ) );
         return NULL;
      }
      if( time_info.hour > rShop->close_hour )
      {
         if( rShop->close_hour < sysdata->hournoon )
            cmdf( keeper, "say Sorry, come back tomorrow. I close at %dam.", ( rShop->close_hour == 0 ) ? ( rShop->close_hour + sysdata->hournoon ) : ( rShop->close_hour ) );
         else
            cmdf( keeper, "say Sorry, come back tomorrow. I close at %dpm.",
                  ( rShop->close_hour == sysdata->hournoon ) ? ( rShop->close_hour ) : ( rShop->close_hour - sysdata->hournoon ) );
         return NULL;
      }
   }

   if( keeper->position == POS_SLEEPING )
   {
      ch->print( "While they're asleep?\r\n" );
      return NULL;
   }

   if( keeper->position < POS_SLEEPING )
   {
      ch->print( "I don't think they can hear you...\r\n" );
      return NULL;
   }

   /*
    * Invisible or hidden people.
    */
   if( !keeper->can_see( ch, false ) )
   {
      interpret( keeper, "say I don't trade with folks I can't see." );
      return NULL;
   }

   int speakswell = UMIN( knows_language( keeper, ch->speaking, ch ), knows_language( ch, ch->speaking, keeper ) );

   if( ( number_percent(  ) % 65 ) > speakswell )
   {
      if( speakswell > 60 )
         cmdf( keeper, "say %s, Could you repeat that? I didn't quite catch it.", ch->name );
      else if( speakswell > 50 )
         cmdf( keeper, "say %s, Could you say that a little more clearly please?", ch->name );
      else if( speakswell > 40 )
         cmdf( keeper, "say %s, Sorry... What was that you wanted?", ch->name );
      else
         cmdf( keeper, "say %s I can't understand you.", ch->name );
      return NULL;
   }
   return keeper;
}

int get_cost( char_data * ch, char_data * keeper, obj_data * obj, bool fBuy )
{
   shop_data *pShop;
   int cost;
   int profitmod;

   if( !obj || ( pShop = keeper->pIndexData->pShop ) == NULL )
      return 0;

   if( fBuy )
   {
      profitmod = 13 - ch->get_curr_cha(  ) + ( ( URANGE( 5, ch->level, LEVEL_AVATAR ) - 20 ) / 2 );
      cost = ( int )( obj->cost * UMAX( ( pShop->profit_sell + 1 ), pShop->profit_buy + profitmod ) ) / 100;
      cost = ( int )( cost * ( 80 + UMIN( ch->level, LEVEL_AVATAR ) ) ) / 100;
   }
   else
   {
      int itype;

      profitmod = ch->get_curr_cha(  ) - 13;
      cost = 0;
      for( itype = 0; itype < MAX_TRADE; ++itype )
      {
         if( obj->item_type == pShop->buy_type[itype] )
         {
            cost = ( int )( obj->cost * UMIN( ( pShop->profit_buy - 1 ), pShop->profit_sell + profitmod ) ) / 100;
            break;
         }
      }
   }

   if( obj->item_type == ITEM_STAFF || obj->item_type == ITEM_WAND )
   {
      if( obj->value[2] < 0 && obj->value[1] < 0 )
         ;
      else
         cost = ( int )( cost * obj->value[2] / ( obj->value[1] > 0 ? obj->value[1] : 1 ) );
   }

   /*
    * alright - now that all THAT crap is over and done with.... lets simplify for
    * * guild vendors since the guild determines what they want to sell it for :)
    */
   if( keeper->has_actflag( ACT_GUILDVENDOR ) )
      cost = obj->cost;

   return cost;
}

int get_repaircost( char_data * keeper, obj_data * obj )
{
   repair_data *rShop;
   int cost;
   int itype;
   bool found;

   if( !obj || !( rShop = keeper->pIndexData->rShop ) )
      return 0;

   cost = 0;
   found = false;
   for( itype = 0; itype < MAX_FIX; ++itype )
   {
      if( obj->item_type == rShop->fix_type[itype] )
      {
         cost = ( int )( obj->cost * rShop->profit_fix / 1000 );
         found = true;
         break;
      }
   }

   if( !found )
      cost = -1;

   if( cost == 0 )
      cost = 1;

   if( found && cost > 0 )
   {
      switch ( obj->item_type )
      {
         default:
            break;

         case ITEM_ARMOR:
            if( obj->value[0] >= obj->value[1] )
               cost = -2;
            else
               cost *= ( obj->value[1] - obj->value[0] );
            break;

         case ITEM_WEAPON:
         case ITEM_MISSILE_WEAPON:
            if( obj->value[6] >= obj->value[0] )
               cost = -2;
            else
               cost *= ( obj->value[0] - obj->value[6] );
            break;

         case ITEM_PROJECTILE:
            if( obj->value[5] >= obj->value[0] )
               cost = -2;
            else
               cost *= ( obj->value[0] - obj->value[5] );
            break;

         case ITEM_WAND:
         case ITEM_STAFF:
            if( obj->value[2] >= obj->value[1] )
               cost = -2;
            else
               cost *= ( obj->value[1] - obj->value[2] );
      }
   }
   return cost;
}

CMDF( do_buy )
{
   char_data *auc;
   string arg;
   double maxgold;

   if( ch->isnpc(  ) )
   {
      ch->print( "Sorry, Mobs cannot buy items!\r\n" );
      return;
   }

   if( ( auc = find_auctioneer( ch ) ) )
   {
      auction_buy( ch, auc, argument );
      return;
   }

   argument = one_argument( argument, arg );

   if( arg.empty(  ) )
   {
      ch->print( "Buy what?\r\n" );
      return;
   }

   if( ch->in_room->flags.test( ROOM_PET_SHOP ) )
   {
      char_data *pet;
      room_index *pRoomIndexNext, *in_room;

      if( ch->isnpc(  ) )
         return;

      pRoomIndexNext = get_room_index( ch->in_room->vnum + 1 );
      if( !pRoomIndexNext )
      {
         bug( "%s: bad pet shop at vnum %d.", __FUNCTION__, ch->in_room->vnum );
         ch->print( "Sorry, you can't buy that here.\r\n" );
         return;
      }

      in_room = ch->in_room;
      ch->in_room = pRoomIndexNext;
      pet = ch->get_char_room( arg );
      ch->in_room = in_room;

      if( pet == NULL || !pet->has_actflag( ACT_PET ) )
      {
         ch->print( "Sorry, you can't buy that here.\r\n" );
         return;
      }

      if( ch->gold < 10 * pet->level * pet->level )
      {
         ch->print( "You can't afford it.\r\n" );
         return;
      }

      if( ch->level < pet->level )
      {
         ch->print( "You're not ready for this pet.\r\n" );
         return;
      }

      if( !ch->can_charm(  ) )
      {
         ch->print( "You already have too many followers to support!\r\n" );
         return;
      }

      maxgold = 10 * pet->level * pet->level;

      if( number_percent(  ) < ch->pcdata->learned[gsn_bargain] )
      {
         double x = ( ( double )ch->pcdata->learned[gsn_bargain] / 3 ) / 100;
         double pct = maxgold * x;
         maxgold = maxgold - pct;

         ch->printf( "&[skill]Your bargaining skills have reduced the price by %d gold!\r\n", ( int )pct );
      }
      else
      {
         if( ch->pcdata->learned[gsn_bargain] > 0 )
         {
            ch->print( "&[skill]Your bargaining skills failed to reduce the price.\r\n" );
            ch->learn_from_failure( gsn_bargain );
         }
      }

      ch->gold -= ( int )maxgold;
      pet = pet->pIndexData->create_mobile(  );

      argument = one_argument( argument, arg );
      if( !arg.empty(  ) )
         stralloc_printf( &pet->name, "%s %s", pet->name, arg.c_str(  ) );

      stralloc_printf( &pet->chardesc, "%sA neck tag says 'I belong to %s'.\r\n", pet->chardesc, ch->name );

      if( !pet->to_room( ch->in_room ) )
         log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
      bind_follower( pet, ch, gsn_charm_person, -1 );
      ch->print( "Enjoy your pet.\r\n" );
      act( AT_ACTION, "$n bought $N as a pet.", ch, NULL, pet, TO_ROOM );
      return;
   }
   else
   {
      char_data *keeper;
      clan_data *clan = NULL;
      obj_data *obj;
      double cost;
      int noi = 1;   /* Number of items */
      short mnoi = 50;  /* Max number of items to be bought at once */
      bool found = false;

      if( !( keeper = find_keeper( ch ) ) )
         return;

      maxgold = keeper->level * keeper->level * 50000;

      if( keeper->has_actflag( ACT_GUILDVENDOR ) )
      {
         list < clan_data * >::iterator cl;

         for( cl = clanlist.begin(  ); cl != clanlist.end(  ); ++cl )
         {
            clan = *cl;

            if( keeper->pIndexData->vnum == clan->shopkeeper )
            {
               found = true;
               break;
            }
         }
      }

      if( is_number( arg ) )
      {
         noi = atoi( arg.c_str(  ) );
         argument = one_argument( argument, arg );
         if( noi > mnoi )
         {
            act( AT_TELL, "$n tells you 'I don't sell that many items at once.'", keeper, NULL, ch, TO_VICT );
            ch->reply = keeper;
            return;
         }
      }

      obj = keeper->get_obj_carry( arg );
      cost = ( get_cost( ch, keeper, obj, true ) * noi );

      if( cost <= 0 || !ch->can_see_obj( obj, false ) )
      {
         act( AT_TELL, "$n tells you 'I don't sell that -- try 'list'.'", keeper, NULL, ch, TO_VICT );
         ch->reply = keeper;
         return;
      }

      /*
       * Moved down here to see if this solves the crash bug 
       */
      if( found )
         cost = ( obj->cost * noi );

      if( !obj->extra_flags.test( ITEM_INVENTORY ) && ( noi > 1 ) )
      {
         interpret( keeper, "laugh" );
         act( AT_TELL, "$n tells you 'I don't have enough of those in stock to sell more than one at a time.'", keeper, NULL, ch, TO_VICT );
         ch->reply = keeper;
         return;
      }

      if( ch->gold < cost )
      {
         act( AT_TELL, "$n tells you 'You can't afford to buy $p.'", keeper, obj, ch, TO_VICT );
         ch->reply = keeper;
         return;
      }

      if( obj->extra_flags.test( ITEM_PROTOTYPE ) && !ch->is_immortal(  ) )
      {
         act( AT_TELL, "$n tells you 'This is a only a prototype!  I can't sell you that...'", keeper, NULL, ch, TO_VICT );
         ch->reply = keeper;
         return;
      }

      if( ch->carry_number + obj->get_number(  ) > ch->can_carry_n(  ) )
      {
         ch->print( "You can't carry that many items.\r\n" );
         return;
      }

      if( ch->carry_weight + ( obj->get_weight(  ) * noi ) + ( noi > 1 ? 2 : 0 ) > ch->can_carry_w(  ) )
      {
         ch->print( "You can't carry that much weight.\r\n" );
         return;
      }

      if( noi == 1 )
      {
         act( AT_ACTION, "$n buys $p.", ch, obj, NULL, TO_ROOM );
         act( AT_ACTION, "You buy $p.", ch, obj, NULL, TO_CHAR );
      }
      else
      {
         act_printf( AT_ACTION, ch, obj, NULL, TO_ROOM, "$n buys %d $p%s.", noi, ( obj->short_descr[strlen( obj->short_descr ) - 1] == 's' ? "" : "s" ) );
         act_printf( AT_ACTION, ch, obj, NULL, TO_CHAR, "You buy %d $p%s.", noi, ( obj->short_descr[strlen( obj->short_descr ) - 1] == 's' ? "" : "s" ) );
         act( AT_ACTION, "$N puts them into a bag and hands it to you.", ch, NULL, keeper, TO_CHAR );
      }

      if( number_percent(  ) < ch->pcdata->learned[gsn_bargain] )
      {
         double x = ( ( double )ch->pcdata->learned[gsn_bargain] / 3 ) / 100;
         double pct = cost * x;
         cost = cost - pct;

         ch->printf( "Your bargaining skills have reduced the price by %d gold!\r\n", ( int )pct );
      }
      else
      {
         if( ch->pcdata->learned[gsn_bargain] > 0 )
         {
            ch->print( "Your bargaining skills failed to reduce the price.\r\n" );
            ch->learn_from_failure( gsn_bargain );
         }
      }

      ch->gold -= ( int )cost;

      if( obj->extra_flags.test( ITEM_INVENTORY ) )
      {
         obj_data *buy_obj, *bag;

         if( !( buy_obj = obj->pIndexData->create_object( obj->level ) ) )
         {
            ch->gold += ( int )cost;
            log_printf( "create_object: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
            return;
         }

         /*
          * Due to grouped objects and carry limitations in SMAUG
          * The shopkeeper gives you a bag with multiple-buy,
          * and also, only one object needs be created with a count
          * set to the number bought.     -Thoric
          */
         if( noi > 1 )
         {
            if( !( bag = get_obj_index( OBJ_VNUM_SHOPPING_BAG )->create_object( 1 ) ) )
            {
               ch->gold += ( int )cost;
               log_printf( "create_object: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
               return;
            }
            bag->extra_flags.set( ITEM_GROUNDROT );
            bag->timer = 10;  /* Blodkai, 4/97 */
            /*
             * perfect size bag ;) 
             */
            bag->value[0] = bag->weight + ( buy_obj->weight * noi );
            buy_obj->count = noi;
            obj->pIndexData->count += ( noi - 1 );
            numobjsloaded += ( noi - 1 );
            buy_obj->to_obj( bag );
            bag->to_char( ch );
         }
         else
            buy_obj->to_char( ch );

         mprog_sell_trigger( keeper, ch, obj );
      }
      else
      {
         obj->from_char(  );
         obj->to_char( ch );
         if( obj->pIndexData->vnum == OBJ_VNUM_TREASURE )
            obj->timer = 0;
      }

      if( found && clan->bank )
      {
         clan->balance += ( int )cost;
         save_clan( clan );
         save_shop( keeper );
      }
      else
      {
         keeper->gold += ( int )cost;
         if( keeper->has_actflag( ACT_GUILDVENDOR ) )
            save_shop( keeper );
      }
      return;
   }
}

/* Ok, sod all that crap about levels and dividers and stuff.
 * Lets just have a nice, simple list command.
 * Samson 8-27-03
 */
CMDF( do_list )
{
   if( ch->in_room->flags.test( ROOM_AUCTION ) )
   {
      room_index *aucvault, *original;
      char_data *auc;

      if( !( aucvault = get_room_index( ch->in_room->vnum + 1 ) ) )
      {
         bug( "%s: bad auction house at vnum %d.", __FUNCTION__, ch->in_room->vnum );
         ch->print( "You can't do that here.\r\n" );
         return;
      }

      if( !( auc = find_auctioneer( ch ) ) )
      {
         ch->print( "The auctioneer is not here!\r\n" );
         return;
      }

      ch->pager( "Items available for bidding:\r\n" );

      original = ch->in_room;

      ch->from_room(  );
      if( !ch->to_room( aucvault ) )
         log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
      show_list_to_char( ch, ch->in_room->objects, true, false );
      ch->from_room(  );
      if( !ch->to_room( original ) )
         log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
      return;
   }

   if( ch->in_room->flags.test( ROOM_PET_SHOP ) )
   {
      room_index *pRoomIndexNext;

      if( !( pRoomIndexNext = get_room_index( ch->in_room->vnum + 1 ) ) )
      {
         bug( "%s: bad pet shop at vnum %d.", __FUNCTION__, ch->in_room->vnum );
         ch->print( "You can't do that here.\r\n" );
         return;
      }

      bool found = false;
      list < char_data * >::iterator ich;
      for( ich = pRoomIndexNext->people.begin(  ); ich != pRoomIndexNext->people.end(  ); ++ich )
      {
         char_data *pet = *ich;

         if( pet->has_actflag( ACT_PET ) )
         {
            if( !found )
            {
               found = true;
               ch->pager( "Pets for sale:\r\n" );
            }
            ch->pagerf( "[%2d] %8d - %s\r\n", pet->level, 10 * pet->level * pet->level, pet->short_descr );
         }
      }
      if( !found )
         ch->print( "Sorry, we're out of pets right now.\r\n" );
   }
   else
   {
      char_data *keeper;

      if( !( keeper = find_keeper( ch ) ) )
         return;

      ch->pager( "&w[Price] Item\r\n" );

      list < obj_data * >::iterator iobj;
      for( iobj = keeper->carrying.begin(  ); iobj != keeper->carrying.end(  ); ++iobj )
      {
         obj_data *obj = *iobj;
         int cost;

         if( obj->wear_loc == WEAR_NONE && ch->can_see_obj( obj, false ) && ( cost = get_cost( ch, keeper, obj, true ) ) > 0 )
         {
            ch->pagerf( "[%6d] %s%s\r\n", cost, obj->short_descr, can_wear_obj( ch, obj ) ? "" : " &R*&w" );
         }
      }
      ch->pager( "A &R*&w indicates an item you are not able to use.\r\n" );
   }
}

CMDF( do_sell )
{
   char_data *keeper, *auc;
   clan_data *clan = NULL;
   obj_data *obj;
   int cost;
   bool found = false;

   if( ch->isnpc(  ) )
   {
      ch->print( "Sorry, Mobs cannot sell items!\r\n" );
      return;
   }

   if( ( auc = find_auctioneer( ch ) ) )
   {
      auction_sell( ch, auc, argument );
      return;
   }


   if( argument.empty(  ) )
   {
      ch->print( "Sell what?\r\n" );
      return;
   }

   if( !( keeper = find_keeper( ch ) ) )
      return;

   if( keeper->has_actflag( ACT_GUILDVENDOR ) )
   {
      list < clan_data * >::iterator cl;

      for( cl = clanlist.begin(  ); cl != clanlist.end(  ); ++cl )
      {
         clan = *cl;

         if( keeper->pIndexData->vnum == clan->shopkeeper )
         {
            found = true;
            break;
         }
      }
   }

   if( !( obj = ch->get_obj_carry( argument ) ) )
   {
      act( AT_TELL, "$n tells you 'You don't have that item.'", keeper, NULL, ch, TO_VICT );
      ch->reply = keeper;
      return;
   }

   /*
    * Bug report and solution thanks to animal@netwin.co.nz 
    */
   if( !keeper->can_see_obj( obj, false ) )
   {
      ch->print( "What are you trying to sell me? I don't buy thin air!\r\n" );
      return;
   }

   if( !ch->can_drop_obj( obj ) )
   {
      ch->print( "You can't let go of it!\r\n" );
      return;
   }

   if( obj->extra_flags.test( ITEM_DONATION ) )
   {
      ch->print( "You cannot sell donated goods!\r\n" );
      return;
   }

   if( obj->extra_flags.test( ITEM_PERSONAL ) )
   {
      ch->print( "Personal items may not be sold.\r\n" );
      return;
   }

   if( obj->timer > 0 )
   {
      act( AT_TELL, "$n tells you, '$p is depreciating in value too quickly...'", keeper, obj, ch, TO_VICT );
      return;
   }

   if( ( cost = get_cost( ch, keeper, obj, false ) ) <= 0 )
   {
      act( AT_ACTION, "$n looks uninterested in $p.", keeper, obj, ch, TO_VICT );
      return;
   }

   if( found && clan->bank && cost >= clan->balance )
   {
      act_printf( AT_TELL, keeper, obj, ch, TO_VICT, "$n tells you, '$p is worth more than %s can afford...'", clan->name.c_str(  ) );
      return;
   }
   else
   {
      if( cost >= keeper->gold )
      {
         act( AT_TELL, "$n tells you, '$p is worth more than I can afford...'", keeper, obj, ch, TO_VICT );
         return;
      }
   }

   obj->separate(  );
   act( AT_ACTION, "$n sells $p.", ch, obj, NULL, TO_ROOM );
   act_printf( AT_ACTION, ch, obj, NULL, TO_CHAR, "You sell $p for %d gold piece%s.", cost, cost == 1 ? "" : "s" );
   ch->gold += cost;

   if( obj->item_type == ITEM_TRASH )
      obj->extract(  );
   else
   {
      obj->from_char(  );
      obj->to_char( keeper );
      if( obj->pIndexData->vnum == OBJ_VNUM_TREASURE )
         obj->timer = 50;
   }

   if( found && clan->bank )
   {
      clan->balance -= cost;
      if( clan->balance < 0 )
         clan->balance = 0;
      save_clan( clan );
      save_shop( keeper );
   }
   else
   {
      keeper->gold -= cost;
      if( keeper->gold < 0 )
         keeper->gold = 0;
      if( keeper->has_actflag( ACT_GUILDVENDOR ) )
         save_shop( keeper );
   }
}

CMDF( do_value )
{
   char_data *keeper, *auc;
   obj_data *obj;
   int cost;

   if( ( auc = find_auctioneer( ch ) ) )
   {
      auction_value( ch, auc, argument );
      return;
   }

   if( argument.empty(  ) )
   {
      ch->print( "Value what?\r\n" );
      return;
   }

   if( !( keeper = find_keeper( ch ) ) )
      return;

   if( !( obj = ch->get_obj_carry( argument ) ) )
   {
      act( AT_TELL, "$n tells you 'You don't have that item.'", keeper, NULL, ch, TO_VICT );
      ch->reply = keeper;
      return;
   }

   if( !ch->can_drop_obj( obj ) )
   {
      ch->print( "You can't let go of it!\r\n" );
      return;
   }

   if( ( cost = get_cost( ch, keeper, obj, false ) ) <= 0 )
   {
      act( AT_ACTION, "$n looks uninterested in $p.", keeper, obj, ch, TO_VICT );
      return;
   }
   act_printf( AT_TELL, keeper, obj, ch, TO_VICT, "$n tells you 'I'd give you %d gold coins for $p.'", cost );
   ch->reply = keeper;
}

/*
 * Repair a single object. Used when handling "repair all" - Gorog
 */
void repair_one_obj( char_data * ch, char_data * keeper, obj_data * obj, const string & arg, const string & fixstr, const string & fixstr2 )
{
   int cost;
   bool found = false;

   if( ch->isnpc(  ) )
   {
      ch->print( "Mobs cannot use the repair command.\r\n" );
      return;
   }

   if( !ch->can_drop_obj( obj ) )
   {
      ch->printf( "You can't let go of %s.\r\n", obj->name );
      return;
   }

   if( ( cost = get_repaircost( keeper, obj ) ) < 0 )
   {
      if( cost != -2 )
         act( AT_TELL, "$n tells you, 'Sorry, I can't do anything with $p.'", keeper, obj, ch, TO_VICT );
      else
         act( AT_TELL, "$n tells you, '$p looks fine to me!'", keeper, obj, ch, TO_VICT );
      return;
   }

   clan_data *clan = NULL;
   if( keeper->has_actflag( ACT_GUILDREPAIR ) )
   {
      list < clan_data * >::iterator cl;

      for( cl = clanlist.begin(  ); cl != clanlist.end(  ); ++cl )
      {
         clan = *cl;

         if( clan->repair == keeper->pIndexData->vnum )
         {
            found = true;
            break;
         }
      }
   }

   if( found && clan == ch->pcdata->clan )
      cost = 0;

   /*
    * "repair all" gets a 10% surcharge - Gorog 
    */
   if( ( cost = strcmp( "all", arg.c_str(  ) )? cost : 11 * cost / 10 ) > ch->gold )
   {
      act_printf( AT_TELL, ch, NULL, keeper, TO_CHAR, "$N tells you, 'It will cost %d piece%s of gold to %s %s...'", cost, cost == 1 ? "" : "s", fixstr.c_str(  ), obj->name );
      act( AT_TELL, "$N tells you, 'Which I see you can't afford.'", ch, NULL, keeper, TO_CHAR );
   }
   else
   {
      act_printf( AT_ACTION, ch, obj, keeper, TO_ROOM, "$n gives $p to $N, who quickly %s it.", fixstr2.c_str(  ) );
      act_printf( AT_ACTION, ch, obj, keeper, TO_CHAR, "$N charges you %d gold piece%s to %s $p.", cost, cost == 1 ? "" : "s", fixstr.c_str(  ) );
      ch->gold -= cost;

      if( found && clan->bank )
      {
         clan->balance += cost;
         save_clan( clan );
      }
      else
      {
         keeper->gold += cost;
         if( keeper->gold < 0 )
            keeper->gold = 0;
      }

      switch ( obj->item_type )
      {
         default:
            ch->print( "For some reason, you think you got ripped off...\r\n" );
            break;
         case ITEM_ARMOR:
            obj->value[0] = obj->value[1];
            break;
         case ITEM_WEAPON:
         case ITEM_MISSILE_WEAPON:
            obj->value[6] = obj->value[0];
            break;
         case ITEM_PROJECTILE:
            obj->value[5] = obj->value[0];
            break;
         case ITEM_WAND:
         case ITEM_STAFF:
            obj->value[2] = obj->value[1];
            break;
      }
      oprog_repair_trigger( ch, obj );
   }
}

CMDF( do_repair )
{
   char_data *keeper;
   string fixstr, fixstr2;

   if( argument.empty(  ) )
   {
      ch->print( "Repair what?\r\n" );
      return;
   }

   if( !( keeper = find_fixer( ch ) ) )
      return;

   switch ( keeper->pIndexData->rShop->shop_type )
   {
      default:
      case SHOP_FIX:
         fixstr = "repair";
         fixstr2 = "repairs";
         break;
      case SHOP_RECHARGE:
         fixstr = "recharge";
         fixstr2 = "recharges";
         break;
   }

   if( !str_cmp( argument, "all" ) )
   {
      list < obj_data * >::iterator iobj;

      for( iobj = ch->carrying.begin(  ); iobj != ch->carrying.end(  ); ++iobj )
      {
         obj_data *obj = *iobj;

         if( ch->can_see_obj( obj, false ) && keeper->can_see_obj( obj, false )
             && ( obj->item_type == ITEM_ARMOR || obj->item_type == ITEM_WEAPON || obj->item_type == ITEM_WAND || obj->item_type == ITEM_STAFF ) )
            repair_one_obj( ch, keeper, obj, argument, fixstr, fixstr2 );
      }
      return;
   }

   obj_data *obj;
   if( !( obj = ch->get_obj_carry( argument ) ) )
   {
      act( AT_TELL, "$n tells you 'You don't have that item.'", keeper, NULL, ch, TO_VICT );
      ch->reply = keeper;
      return;
   }
   repair_one_obj( ch, keeper, obj, argument, fixstr, fixstr2 );
}

void appraise_all( char_data * ch, char_data * keeper, const string & fixstr )
{
   list < obj_data * >::iterator iobj;
   int cost = 0, total = 0;

   for( iobj = ch->carrying.begin(  ); iobj != ch->carrying.end(  ); ++iobj )
   {
      obj_data *obj = *iobj;

      if( obj->wear_loc == WEAR_NONE && ch->can_see_obj( obj, false )
          && ( obj->item_type == ITEM_ARMOR || obj->item_type == ITEM_WEAPON || obj->item_type == ITEM_WAND || obj->item_type == ITEM_STAFF ) )
      {
         if( !ch->can_drop_obj( obj ) )
            ch->printf( "You can't let go of %s.\r\n", obj->name );
         else if( ( cost = get_repaircost( keeper, obj ) ) < 0 )
         {
            if( cost != -2 )
               act( AT_TELL, "$n tells you, 'Sorry, I can't do anything with $p.'", keeper, obj, ch, TO_VICT );
            else
               act( AT_TELL, "$n tells you, '$p looks fine to me!'", keeper, obj, ch, TO_VICT );
         }
         else
         {
            act_printf( AT_TELL, ch, NULL, keeper, TO_CHAR,
                        "$N tells you, 'It will cost %d piece%s of gold to %s %s'", cost, cost == 1 ? "" : "s", fixstr.c_str(  ), obj->name );
            total += cost;
         }
      }
   }
   if( total > 0 )
   {
      ch->print( "\r\n" );
      act_printf( AT_TELL, ch, NULL, keeper, TO_CHAR, "$N tells you, 'It will cost %d piece%s of gold in total.'", total, cost == 1 ? "" : "s" );
      act_printf( AT_TELL, ch, NULL, keeper, TO_CHAR, "$N tells you, 'Remember there is a 10%% surcharge for repair all.'" );
   }
}

CMDF( do_appraise )
{
   char_data *keeper;
   obj_data *obj;
   int cost;
   string fixstr;

   if( argument.empty(  ) )
   {
      ch->print( "Appraise what?\r\n" );
      return;
   }

   if( !( keeper = find_fixer( ch ) ) )
      return;

   switch ( keeper->pIndexData->rShop->shop_type )
   {
      default:
      case SHOP_FIX:
         fixstr = "repair";
         break;
      case SHOP_RECHARGE:
         fixstr = "recharge";
         break;
   }

   if( !str_cmp( argument, "all" ) )
   {
      appraise_all( ch, keeper, fixstr );
      return;
   }

   if( !( obj = ch->get_obj_carry( argument ) ) )
   {
      act( AT_TELL, "$n tells you 'You don't have that item.'", keeper, NULL, ch, TO_VICT );
      ch->reply = keeper;
      return;
   }

   if( !ch->can_drop_obj( obj ) )
   {
      ch->print( "You can't let go of it.\r\n" );
      return;
   }

   if( ( cost = get_repaircost( keeper, obj ) ) < 0 )
   {
      if( cost != -2 )
         act( AT_TELL, "$n tells you, 'Sorry, I can't do anything with $p.'", keeper, obj, ch, TO_VICT );
      else
         act( AT_TELL, "$n tells you, '$p looks fine to me!'", keeper, obj, ch, TO_VICT );
      return;
   }

   act_printf( AT_TELL, ch, NULL, keeper, TO_CHAR, "$N tells you, 'It will cost %d piece%s of gold to %s that...'", cost, cost == 1 ? "" : "s", fixstr.c_str(  ) );
   if( cost > ch->gold )
      act( AT_TELL, "$N tells you, 'Which I see you can't afford.'", ch, NULL, keeper, TO_CHAR );
}

/* ------------------ Shop Building and Editing Section ----------------- */
CMDF( do_makeshop )
{
   shop_data *shop;
   char_data *keeper;
   mob_index *mob;
   int vnum;

   if( argument.empty(  ) )
   {
      ch->print( "Usage: makeshop <mobname>\r\n" );
      return;
   }

   if( !( keeper = ch->get_char_world( argument ) ) )
   {
      ch->print( "Mobile not found.\r\n" );
      return;
   }
   if( !keeper->isnpc(  ) )
   {
      ch->print( "PCs can't keep shops.\r\n" );
      return;
   }
   mob = keeper->pIndexData;
   vnum = mob->vnum;

   if( !can_mmodify( ch, keeper ) )
      return;

   if( mob->pShop )
   {
      ch->print( "This mobile already has a shop.\r\n" );
      return;
   }
   shop = new shop_data;
   shop->keeper = vnum;
   shop->profit_buy = 120;
   shop->profit_sell = 90;
   shop->open_hour = 0;
   shop->close_hour = sysdata->hoursperday;
   shoplist.push_back( shop );
   mob->pShop = shop;
   ch->print( "Done.\r\n" );
}

CMDF( do_destroyshop )
{
   mob_index *mob;

   if( argument.empty(  ) )
   {
      ch->print( "Usage: destroyshop <mob vnum>\r\n" );
      return;
   }

   if( !is_number( argument ) )
   {
      ch->print( "Mob vnums must be specified as numbers.\r\n" );
      return;
   }

   if( !( mob = get_mob_index( atoi( argument.c_str(  ) ) ) ) )
   {
      ch->print( "No mob with that vnum exists.\r\n" );
      return;
   }

   if( !mob->pShop )
   {
      ch->print( "That mob does not have a shop assigned.\r\n" );
      return;
   }

   shoplist.remove( mob->pShop );
   deleteptr( mob->pShop );
   ch->print( "Shop data deleted.\r\n" );
}

CMDF( do_shopset )
{
   shop_data *shop;
   char_data *keeper;
   mob_index *mob, *mob2;
   string arg1, arg2;
   int value;

   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );

   if( arg1.empty(  ) || arg2.empty(  ) )
   {
      ch->print( "Usage: shopset <mob vnum> <field> value\r\n" );
      ch->print( "\r\nField being one of:\r\n" );
      ch->print( "  buy0 buy1 buy2 buy3 buy4 buy sell open close keeper remove\r\n" );
      return;
   }

   if( !( keeper = ch->get_char_world( arg1 ) ) )
   {
      ch->print( "Mobile not found.\r\n" );
      return;
   }

   if( !keeper->isnpc(  ) )
   {
      ch->print( "PCs can't keep shops.\r\n" );
      return;
   }

   mob = keeper->pIndexData;

   if( !can_mmodify( ch, keeper ) )
      return;

   if( !mob->pShop )
   {
      ch->print( "This mobile doesn't keep a shop.\r\n" );
      return;
   }
   shop = mob->pShop;
   value = atoi( argument.c_str(  ) );

   if( !str_cmp( arg2, "remove" ) )
   {
      mob->pShop = NULL;
      shoplist.remove( shop );
      deleteptr( shop );

      ch->print( "Shop data has been removed from this mob.\r\n" );
      return;
   }

   if( !str_cmp( arg2, "buy0" ) )
   {
      if( !is_number( argument ) )
         value = get_otype( argument );
      if( value < 0 || value >= MAX_ITEM_TYPE )
      {
         ch->print( "Invalid item type!\r\n" );
         return;
      }
      shop->buy_type[0] = value;
      ch->print( "Done.\r\n" );
      return;
   }

   if( !str_cmp( arg2, "buy1" ) )
   {
      if( !is_number( argument ) )
         value = get_otype( argument );
      if( value < 0 || value >= MAX_ITEM_TYPE )
      {
         ch->print( "Invalid item type!\r\n" );
         return;
      }
      shop->buy_type[1] = value;
      ch->print( "Done.\r\n" );
      return;
   }

   if( !str_cmp( arg2, "buy2" ) )
   {
      if( !is_number( argument ) )
         value = get_otype( argument );
      if( value < 0 || value >= MAX_ITEM_TYPE )
      {
         ch->print( "Invalid item type!\r\n" );
         return;
      }
      shop->buy_type[2] = value;
      ch->print( "Done.\r\n" );
      return;
   }

   if( !str_cmp( arg2, "buy3" ) )
   {
      if( !is_number( argument ) )
         value = get_otype( argument );
      if( value < 0 || value >= MAX_ITEM_TYPE )
      {
         ch->print( "Invalid item type!\r\n" );
         return;
      }
      shop->buy_type[3] = value;
      ch->print( "Done.\r\n" );
      return;
   }

   if( !str_cmp( arg2, "buy4" ) )
   {
      if( !is_number( argument ) )
         value = get_otype( argument );
      if( value < 0 || value >= MAX_ITEM_TYPE )
      {
         ch->print( "Invalid item type!\r\n" );
         return;
      }
      shop->buy_type[4] = value;
      ch->print( "Done.\r\n" );
      return;
   }

   if( !str_cmp( arg2, "buy" ) )
   {
      if( value <= ( shop->profit_sell + 5 ) || value > 1000 )
      {
         ch->print( "Out of range.\r\n" );
         return;
      }
      shop->profit_buy = value;
      ch->print( "Done.\r\n" );
      return;
   }

   if( !str_cmp( arg2, "sell" ) )
   {
      if( value < 0 || value >= ( shop->profit_buy - 5 ) )
      {
         ch->print( "Out of range.\r\n" );
         return;
      }
      shop->profit_sell = value;
      ch->print( "Done.\r\n" );
      return;
   }

   if( !str_cmp( arg2, "open" ) )
   {
      if( value < 0 || value > sysdata->hoursperday )
      {
         ch->printf( "Out of range. Valid values are from 0 to %d.\r\n", sysdata->hoursperday );
         return;
      }
      shop->open_hour = value;
      ch->print( "Done.\r\n" );
      return;
   }

   if( !str_cmp( arg2, "close" ) )
   {
      if( value < 0 || value > sysdata->hoursperday )
      {
         ch->printf( "Out of range. Valid values are from 0 to %d.\r\n", sysdata->hoursperday );
         return;
      }
      shop->close_hour = value;
      ch->print( "Done.\r\n" );
      return;
   }

   if( !str_cmp( arg2, "keeper" ) )
   {
      int vnum = atoi( argument.c_str(  ) );
      if( !( mob2 = get_mob_index( vnum ) ) )
      {
         ch->print( "Mobile not found.\r\n" );
         return;
      }
      if( !can_mmodify( ch, keeper ) )
         return;
      if( mob2->pShop )
      {
         ch->print( "That mobile already has a shop.\r\n" );
         return;
      }
      mob->pShop = NULL;
      mob2->pShop = shop;
      shop->keeper = value;
      ch->print( "Done.\r\n" );
      return;
   }
   do_shopset( ch, "" );
}

CMDF( do_shopstat )
{
   shop_data *shop;
   char_data *keeper;
   mob_index *mob;

   if( argument.empty(  ) )
   {
      ch->print( "Usage: shopstat <keeper name>\r\n" );
      return;
   }

   if( !( keeper = ch->get_char_world( argument ) ) )
   {
      ch->print( "Mobile not found.\r\n" );
      return;
   }
   if( !keeper->isnpc(  ) )
   {
      ch->print( "PCs can't keep shops.\r\n" );
      return;
   }
   mob = keeper->pIndexData;

   if( !mob->pShop )
   {
      ch->print( "This mobile doesn't keep a shop.\r\n" );
      return;
   }
   shop = mob->pShop;

   ch->printf( "Keeper: %d  %s\r\n", shop->keeper, mob->short_descr );
   ch->printf( "buy0 [%s]  buy1 [%s]  buy2 [%s]  buy3 [%s]  buy4 [%s]\r\n",
               o_types[shop->buy_type[0]], o_types[shop->buy_type[1]], o_types[shop->buy_type[2]], o_types[shop->buy_type[3]], o_types[shop->buy_type[4]] );
   ch->printf( "Profit:  buy %3d%%  sell %3d%%\r\n", shop->profit_buy, shop->profit_sell );
   ch->printf( "Hours:   open %2d  close %2d\r\n", shop->open_hour, shop->close_hour );
}

CMDF( do_shops )
{
   list < shop_data * >::iterator ishop;
   mob_index *mob;

   if( shoplist.empty(  ) )
   {
      ch->print( "There are no shops.\r\n" );
      return;
   }

   ch->pager( "&WFor details on each shopkeeper, use the shopstat command.\r\n" );
   for( ishop = shoplist.begin(  ); ishop != shoplist.end(  ); ++ishop )
   {
      shop_data *shop = *ishop;

      if( !( mob = get_mob_index( shop->keeper ) ) )
      {
         bug( "%s: Bad mob index on shopkeeper %d", __FUNCTION__, shop->keeper );
         continue;
      }
      ch->pagerf( "&WShopkeeper: &G%-20.20s &WVnum: &G%-5d &WArea: &G%s\r\n", mob->short_descr, mob->vnum, mob->area->name );
   }
}

/* -------------- Repair Shop Building and Editing Section -------------- */
CMDF( do_makerepair )
{
   repair_data *repair;
   char_data *keeper;
   mob_index *mob;
   int vnum;

   if( argument.empty(  ) )
   {
      ch->print( "Usage: makerepair <mobvnum>\r\n" );
      return;
   }

   if( !( keeper = ch->get_char_world( argument ) ) )
   {
      ch->print( "Mobile not found.\r\n" );
      return;
   }
   if( !keeper->isnpc(  ) )
   {
      ch->print( "PCs can't keep shops.\r\n" );
      return;
   }
   mob = keeper->pIndexData;
   vnum = mob->vnum;

   if( !can_mmodify( ch, keeper ) )
      return;

   if( mob->rShop )
   {
      ch->print( "This mobile already has a repair shop.\r\n" );
      return;
   }

   repair = new repair_data;
   repair->keeper = vnum;
   repair->profit_fix = 100;
   repair->shop_type = SHOP_FIX;
   repair->open_hour = 0;
   repair->close_hour = sysdata->hoursperday;
   repairlist.push_back( repair );
   mob->rShop = repair;
   ch->print( "Done.\r\n" );
}

CMDF( do_destroyrepair )
{
   mob_index *mob;

   if( argument.empty(  ) )
   {
      ch->print( "Usage: destroyrepair <mob vnum>\r\n" );
      return;
   }

   if( !is_number( argument ) )
   {
      ch->print( "Mob vnums must be specified as numbers.\r\n" );
      return;
   }

   if( !( mob = get_mob_index( atoi( argument.c_str(  ) ) ) ) )
   {
      ch->print( "No mob with that vnum exists.\r\n" );
      return;
   }

   if( !mob->rShop )
   {
      ch->print( "That mob does not have a repair shop assigned.\r\n" );
      return;
   }

   repairlist.remove( mob->rShop );
   deleteptr( mob->rShop );
   ch->print( "Repair shop data deleted.\r\n" );
}

CMDF( do_repairset )
{
   repair_data *repair;
   char_data *keeper;
   mob_index *mob, *mob2;
   string arg1, arg2;
   int value;

   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );

   if( arg1.empty(  ) || arg2.empty(  ) )
   {
      ch->print( "Usage: repairset <mob vnum> <field> value\r\n" );
      ch->print( "\r\nField being one of:\r\n" );
      ch->print( "  fix0 fix1 fix2 profit type open close keeper\r\n" );
      return;
   }

   if( !( keeper = ch->get_char_world( arg1 ) ) )
   {
      ch->print( "Mobile not found.\r\n" );
      return;
   }
   if( !keeper->isnpc(  ) )
   {
      ch->print( "PCs can't keep shops.\r\n" );
      return;
   }
   mob = keeper->pIndexData;

   if( !can_mmodify( ch, keeper ) )
      return;

   if( !mob->rShop )
   {
      ch->print( "This mobile doesn't keep a repair shop.\r\n" );
      return;
   }
   repair = mob->rShop;
   value = atoi( argument.c_str(  ) );

   if( !str_cmp( arg2, "fix0" ) )
   {
      if( !is_number( argument ) )
         value = get_otype( argument );
      if( value < 0 || value >= MAX_ITEM_TYPE )
      {
         ch->print( "Invalid item type!\r\n" );
         return;
      }
      repair->fix_type[0] = value;
      ch->print( "Done.\r\n" );
      return;
   }

   if( !str_cmp( arg2, "fix1" ) )
   {
      if( !is_number( argument ) )
         value = get_otype( argument );
      if( value < 0 || value >= MAX_ITEM_TYPE )
      {
         ch->print( "Invalid item type!\r\n" );
         return;
      }
      repair->fix_type[1] = value;
      ch->print( "Done.\r\n" );
      return;
   }

   if( !str_cmp( arg2, "fix2" ) )
   {
      if( !is_number( argument ) )
         value = get_otype( argument );
      if( value < 0 || value >= MAX_ITEM_TYPE )
      {
         ch->print( "Invalid item type!\r\n" );
         return;
      }
      repair->fix_type[2] = value;
      ch->print( "Done.\r\n" );
      return;
   }

   if( !str_cmp( arg2, "profit" ) )
   {
      if( value < 1 || value > 1000 )
      {
         ch->print( "Out of range.\r\n" );
         return;
      }
      repair->profit_fix = value;
      ch->print( "Done.\r\n" );
      return;
   }

   if( !str_cmp( arg2, "type" ) )
   {
      if( value < 1 || value > 2 )
      {
         ch->print( "Out of range.\r\n" );
         return;
      }
      repair->shop_type = value;
      ch->print( "Done.\r\n" );
      return;
   }

   if( !str_cmp( arg2, "open" ) )
   {
      if( value < 0 || value > sysdata->hoursperday )
      {
         ch->printf( "Out of range. Valid values are from 0 to %d.\r\n", sysdata->hoursperday );
         return;
      }
      repair->open_hour = value;
      ch->print( "Done.\r\n" );
      return;
   }

   if( !str_cmp( arg2, "close" ) )
   {
      if( value < 0 || value > sysdata->hoursperday )
      {
         ch->printf( "Out of range. Valid values are from 0 to %d.\r\n", sysdata->hoursperday );
         return;
      }
      repair->close_hour = value;
      ch->print( "Done.\r\n" );
      return;
   }

   if( !str_cmp( arg2, "keeper" ) )
   {
      int vnum = atoi( argument.c_str(  ) );
      if( !( mob2 = get_mob_index( vnum ) ) )
      {
         ch->print( "Mobile not found.\r\n" );
         return;
      }
      if( !can_mmodify( ch, keeper ) )
         return;
      if( mob2->rShop )
      {
         ch->print( "That mobile already has a repair shop.\r\n" );
         return;
      }
      mob->rShop = NULL;
      mob2->rShop = repair;
      repair->keeper = value;
      ch->print( "Done.\r\n" );
      return;
   }
   do_repairset( ch, "" );
}

CMDF( do_repairstat )
{
   repair_data *repair;
   char_data *keeper;
   mob_index *mob;

   if( argument.empty(  ) )
   {
      ch->print( "Usage: repairstat <keeper vnum>\r\n" );
      return;
   }

   if( !( keeper = ch->get_char_world( argument ) ) )
   {
      ch->print( "Mobile not found.\r\n" );
      return;
   }
   if( !keeper->isnpc(  ) )
   {
      ch->print( "PCs can't keep shops.\r\n" );
      return;
   }
   mob = keeper->pIndexData;

   if( !mob->rShop )
   {
      ch->print( "This mobile doesn't keep a repair shop.\r\n" );
      return;
   }
   repair = mob->rShop;

   ch->printf( "Keeper: %d  %s\r\n", repair->keeper, mob->short_descr );
   ch->printf( "fix0 [%s]  fix1 [%s]  fix2 [%s]\r\n", o_types[repair->fix_type[0]], o_types[repair->fix_type[1]], o_types[repair->fix_type[2]] );
   ch->printf( "Profit: %3d%%  Type: %d\r\n", repair->profit_fix, repair->shop_type );
   ch->printf( "Hours:   open %2d  close %2d\r\n", repair->open_hour, repair->close_hour );
}

CMDF( do_repairshops )
{
   list < repair_data * >::iterator irepair;
   mob_index *mob;

   if( repairlist.empty(  ) )
   {
      ch->print( "There are no repair shops.\r\n" );
      return;
   }

   ch->pager( "&WFor details on each repairsmith, use the repairstat command.\r\n" );
   for( irepair = repairlist.begin(  ); irepair != repairlist.end(  ); ++irepair )
   {
      repair_data *repair = *irepair;

      if( !( mob = get_mob_index( repair->keeper ) ) )
      {
         bug( "%s: Bad mob index on repairsmith %d", __FUNCTION__, repair->keeper );
         continue;
      }
      ch->pagerf( "&WRepairsmith: &G%-20.20s &WVnum: &G%-5d &WArea: &G%s\r\n", mob->short_descr, mob->vnum, mob->area->name );
   }
}

/***************************************************************************  
 *                          SMAUG Banking Support Code                     *
 ***************************************************************************
 *                                                                         *
 * This code may be used freely, as long as credit is given in the help    *
 * file. Thanks.                                                           *
 *                                                                         *
 *                                        -= Minas Ravenblood =-           *
 *                                 Implementor of The Apocalypse Theatre   *
 *                                      (email: krisco7@hotmail.com)       *
 *                                                                         *
 ***************************************************************************/

/* Modifications to original source by Samson. Updated to include support for guild/clan bank accounts */

/* You can add this or just put it in the do_bank code. I don't really know
   why I made a seperate function for this, but I did. If you do add it,
   don't forget to declare it - Minas */

/* Finds banker mobs in a room. */
char_data *find_banker( char_data * ch )
{
   list < char_data * >::iterator ich;

   for( ich = ch->in_room->people.begin(  ); ich != ch->in_room->people.end(  ); ++ich )
   {
      char_data *banker = *ich;

      if( banker->has_actflag( ACT_BANKER ) || banker->has_actflag( ACT_GUILDBANK ) )
         return banker;
   }
   return NULL;
}

/* SMAUG Bank Support
 * Coded by Minas Ravenblood for The Apocalypse Theatre
 * (email: krisco7@hotmail.com)
 */
/* Deposit, withdraw, balance and transfer commands */
CMDF( do_deposit )
{
   char_data *banker;
   int amount;
   bool found = false;

   if( !( banker = find_banker( ch ) ) )
   {
      ch->print( "You're not in a bank!\r\n" );
      return;
   }

   if( ch->isnpc(  ) )
   {
      interpret( banker, "say Sorry, we don't do business with mobs." );
      return;
   }

   clan_data *clan = NULL;
   if( banker->has_actflag( ACT_GUILDBANK ) )
   {
      list < clan_data * >::iterator cl;

      for( cl = clanlist.begin(  ); cl != clanlist.end(  ); ++cl )
      {
         clan = *cl;
         if( clan->bank == banker->pIndexData->vnum )
         {
            found = true;
            break;
         }
      }
   }

   if( found && ch->pcdata->clan != clan )
   {
      interpret( banker, "say Beat it! This is a private vault!" );
      return;
   }

   if( argument.empty(  ) )
   {
      ch->print( "How much gold do you wish to deposit?\r\n" );
      return;
   }

   if( str_cmp( argument, "all" ) && !is_number( argument ) )
   {
      ch->print( "How much gold do you wish to deposit?\r\n" );
      return;
   }

   if( !str_cmp( argument, "all" ) )
      amount = ch->gold;
   else
      amount = atoi( argument.c_str(  ) );

   if( amount > ch->gold )
   {
      ch->print( "You don't have that much gold to deposit.\r\n" );
      return;
   }

   if( amount <= 0 )
   {
      ch->print( "Oh, I see.. you're a comedian.\r\n" );
      return;
   }

   ch->gold -= amount;

   if( found )
   {
      clan->balance += amount;
      save_clan( clan );
   }
   else
      ch->pcdata->balance += amount;

   ch->printf( "&[gold]You deposit %d gold.\r\n", amount );
   act_printf( AT_GOLD, ch, NULL, NULL, TO_ROOM, "$n deposits %d gold.", amount );
   ch->sound( "gold.wav", 100, false );
   ch->save(  );  /* Prevent money duplication for clan accounts - Samson */
}

CMDF( do_withdraw )
{
   char_data *banker;
   int amount;
   bool found = false;

   if( !( banker = find_banker( ch ) ) )
   {
      ch->print( "You're not in a bank!\r\n" );
      return;
   }

   if( ch->isnpc(  ) )
   {
      interpret( banker, "say Sorry, we don't do business with mobs." );
      return;
   }

   clan_data *clan = NULL;
   if( banker->has_actflag( ACT_GUILDBANK ) )
   {
      list < clan_data * >::iterator cl;

      for( cl = clanlist.begin(  ); cl != clanlist.end(  ); ++cl )
      {
         clan = *cl;
         if( clan->bank == banker->pIndexData->vnum )
         {
            found = true;
            break;
         }
      }
   }

   if( found && ch->pcdata->clan != clan )
   {
      interpret( banker, "say Beat it! This is a private vault!" );
      return;
   }

   if( found && str_cmp( ch->name, clan->leader ) && str_cmp( ch->name, clan->number1 ) && str_cmp( ch->name, clan->number2 ) )
   {
      ch->printf( "Sorry, only the %s officers are allowed to withdraw funds.\r\n", clan->clan_type == CLAN_GUILD ? "guild" : "clan" );
      return;
   }

   if( argument.empty(  ) )
   {
      ch->print( "How much gold do you wish to withdraw?\r\n" );
      return;
   }

   if( str_cmp( argument, "all" ) && !is_number( argument ) )
   {
      ch->print( "How much gold do you wish to withdraw?\r\n" );
      return;
   }

   if( !str_cmp( argument, "all" ) )
   {
      if( found )
         amount = clan->balance;
      else
         amount = ch->pcdata->balance;
   }
   else
      amount = atoi( argument.c_str(  ) );

   if( found )
   {
      if( amount > clan->balance )
      {
         ch->print( "There isn't that much gold in the vault!\r\n" );
         return;
      }
   }
   else
   {
      if( amount > ch->pcdata->balance )
      {
         ch->print( "But you do not have that much gold in your account!\r\n" );
         return;
      }
   }

   if( amount <= 0 )
   {
      ch->print( "Oh I see.. you're a comedian.\r\n" );
      return;
   }

   if( found )
   {
      clan->balance -= amount;
      save_clan( clan );
   }
   else
      ch->pcdata->balance -= amount;

   ch->gold += amount;
   ch->printf( "&[gold]You withdraw %d gold.\r\n", amount );
   act_printf( AT_GOLD, ch, NULL, NULL, TO_ROOM, "$n withdraws %d gold.", amount );
   ch->sound( "gold.wav", 100, false );
   ch->save(  );  /* Prevent money duplication for clan accounts - Samson */
}

CMDF( do_balance )
{
   char_data *banker;
   bool found = false;

   if( !( banker = find_banker( ch ) ) )
   {
      ch->print( "You're not in a bank!\r\n" );
      return;
   }

   if( ch->isnpc(  ) )
   {
      interpret( banker, "say Sorry, %s, we don't do business with mobs." );
      return;
   }

   clan_data *clan = NULL;
   if( banker->has_actflag( ACT_GUILDBANK ) )
   {
      list < clan_data * >::iterator cl;

      for( cl = clanlist.begin(  ); cl != clanlist.end(  ); ++cl )
      {
         clan = *cl;

         if( clan->bank == banker->pIndexData->vnum )
         {
            found = true;
            break;
         }
      }
   }

   if( found && ch->pcdata->clan != clan )
   {
      interpret( banker, "say Beat it! This is a private vault!" );
      return;
   }

   if( found )
   {
      ch->printf( "&[gold]The %s has %d gold in the vault.\r\n", clan->clan_type == CLAN_GUILD ? "guild" : "clan", clan->balance );
   }
   else
      ch->printf( "&[gold]You have %d gold in the bank.\r\n", ch->pcdata->balance );
}

/* End of new bank support */
