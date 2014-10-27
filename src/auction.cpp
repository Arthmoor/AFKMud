/****************************************************************************
 *                   ^     +----- |  / ^     ^ |     | +-\                  *
 *                  / \    |      | /  |\   /| |     | |  \                 *
 *                 /   \   +---   |<   | \ / | |     | |  |                 *
 *                /-----\  |      | \  |  v  | |     | |  /                 *
 *               /       \ |      |  \ |     | +-----+ +-/                  *
 ****************************************************************************
 * AFKMud Copyright 1997-2014 by Roger Libiez (Samson),                     *
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
 *                          Auction House module                            *
 ****************************************************************************/

#include <cstdarg>
#include <fstream>
#include <sstream>
#include <dirent.h>
#include "mud.h"
#include "auction.h"
#include "clans.h"
#include "descriptor.h"
#include "event.h"
#include "mobindex.h"
#include "mud_prog.h"
#include "objindex.h"
#include "roomindex.h"

list < sale_data * >salelist;

void bid( char_data *, char_data *, const string & );

void save_sales( void )
{
   ofstream stream;

   stream.open( SALES_FILE );
   if( !stream.is_open(  ) )
   {
      bug( "%s: fopen", __FUNCTION__ );
      perror( SALES_FILE );
   }
   else
   {
      list < sale_data * >::iterator sl;
      for( sl = salelist.begin(  ); sl != salelist.end(  ); ++sl )
      {
         sale_data *sale = *sl;

         stream << "#SALE" << endl;
         stream << "Aucmob    " << sale->get_aucmob(  ) << endl;
         stream << "Seller    " << sale->get_seller(  ) << endl;
         stream << "Buyer     " << sale->get_buyer(  ) << endl;
         stream << "Item      " << sale->get_item(  ) << endl;
         stream << "Bid       " << sale->get_bid(  ) << endl;
         stream << "Collected " << sale->get_collected(  ) << endl;
         stream << "End" << endl << endl;
      }
      stream.close(  );
   }
}

sale_data::sale_data(  )
{
   init_memory( &bid, &collected, sizeof( collected ) );
}

sale_data::~sale_data(  )
{
   salelist.remove( this );

   if( !mud_down )
      save_sales(  );
}

void add_sale( const string & aucmob, const string & seller, const string & buyer, const string & item, int bidamt, bool collected )
{
   sale_data *sale = new sale_data;

   sale->set_aucmob( aucmob );
   sale->set_seller( seller );
   sale->set_buyer( buyer );
   sale->set_item( item );
   sale->set_bid( bidamt );
   sale->set_collected( collected );

   salelist.push_back( sale );
   save_sales(  );
}

void free_sales( void )
{
   list < sale_data * >::iterator sl;

   for( sl = salelist.begin(  ); sl != salelist.end(  ); )
   {
      sale_data *sale = *sl;
      ++sl;

      deleteptr( sale );
   }
}

sale_data *check_sale( const string & aucmob, const string & pcname, const string & objname )
{
   list < sale_data * >::iterator sl;

   for( sl = salelist.begin(  ); sl != salelist.end(  ); ++sl )
   {
      sale_data *sale = *sl;

      if( !str_cmp( aucmob, sale->get_aucmob(  ) ) )
      {
         if( !str_cmp( pcname, sale->get_seller(  ) ) )
         {
            if( !str_cmp( objname, sale->get_item(  ) ) )
               return sale;
         }
      }
   }
   return NULL;
}

void sale_count( char_data * ch )
{
   list < sale_data * >::iterator sl;
   short salecount = 0;

   for( sl = salelist.begin(  ); sl != salelist.end(  ); ++sl )
   {
      sale_data *sale = *sl;

      if( !str_cmp( sale->get_seller(  ), ch->name ) )
         ++salecount;
   }

   if( salecount > 0 )
   {
      ch->printf( "&[auction]While you were gone, auctioneers sold %d items for you.\r\n", salecount );
      ch->print( "Use the 'saleslist' command to see which auctioneer sold what.\r\n" );
   }
}

CMDF( do_saleslist )
{
   list < sale_data * >::iterator sl;
   short salecount = 0;

   for( sl = salelist.begin(  ); sl != salelist.end(  ); ++sl )
   {
      sale_data *sale = *sl;
      const string seller = sale->get_seller(  );

      if( !str_cmp( seller, ch->name ) || ch->is_imp(  ) )
      {
         ch->printf( "&[auction]%s sold %s for %s while %s were away.\r\n",
                     sale->get_aucmob(  ).c_str(  ), sale->get_item(  ).c_str(  ),
                     ( !str_cmp( seller, ch->name ) ? "you" : seller.c_str(  ) ), ( !str_cmp( seller, ch->name ) ? "you" : "they" ) );
         ++salecount;
      }
   }

   if( salecount == 0 )
      ch->print( "&[auction]You haven't sold any items at auction yet.\r\n" );
}

void prune_sales( void )
{
   list < sale_data * >::iterator sl;

   for( sl = salelist.begin(  ); sl != salelist.end(  ); )
   {
      sale_data *sale = *sl;
      ++sl;

      if( !exists_player( sale->get_buyer(  ) ) && !exists_player( sale->get_seller(  ) ) )
      {
         deleteptr( sale );
         continue;
      }
      if( !exists_player( sale->get_seller(  ) ) && sale->get_collected(  ) )
      {
         deleteptr( sale );
         continue;
      }
      if( !exists_player( sale->get_buyer(  ) ) && !sale->get_collected(  ) && str_cmp( sale->get_buyer(  ), "The Code" ) )
      {
         sale->set_buyer( "The Code" );
         continue;
      }
   }
   save_sales(  );
}

void load_sales( void )
{
   sale_data *sale = NULL;
   ifstream stream;

   salelist.clear(  );

   stream.open( SALES_FILE );
   if( !stream.is_open(  ) )
   {
      log_string( "No sales file found." );
      return;
   }

   do
   {
      string key, value;
      char buf[MIL];

      stream >> key;
      stream.getline( buf, MIL );
      value = buf;

      strip_lspace( key );
      strip_lspace( value );
      strip_tilde( value );

      if( key.empty(  ) )
         continue;

      if( key == "#SALE" )
         sale = new sale_data;
      else if( key == "Aucmob" )
         sale->set_aucmob( value );
      else if( key == "Seller" )
         sale->set_seller( value );
      else if( key == "Buyer" )
         sale->set_buyer( value );
      else if( key == "Item" )
         sale->set_item( value );
      else if( key == "Bid" )
         sale->set_bid( atoi( value.c_str(  ) ) );
      else if( key == "Collected" )
         sale->set_collected( atoi( value.c_str(  ) ) );
      else if( key == "End" )
         salelist.push_back( sale );
      else
         log_printf( "%s: Bad line in sales file: %s %s", __FUNCTION__, key.c_str(  ), value.c_str(  ) );
   }
   while( !stream.eof(  ) );
   stream.close(  );
   prune_sales(  );
}

void read_aucvault( const char *dirname, const char *filename )
{
   room_index *aucvault;
   char_data *aucmob;
   FILE *fp;
   char fname[256];

   if( !( aucmob = supermob->get_char_world( filename ) ) )
   {
      bug( "Um. Missing mob for %s's auction vault.", filename );
      return;
   }

   aucvault = get_room_index( aucmob->in_room->vnum + 1 );

   if( !aucvault )
   {
      bug( "Ooops! The vault room for %s's auction house is missing!", aucmob->short_descr );
      return;
   }

   snprintf( fname, 256, "%s%s", dirname, filename );
   if( ( fp = fopen( fname, "r" ) ) != NULL )
   {
      log_printf( "Loading auction house vault: %s", filename );
      rset_supermob( aucvault );

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
            bug( "%s: # not found. %s", __FUNCTION__, aucmob->short_descr );
            break;
         }

         word = fread_word( fp );
         if( !str_cmp( word, "OBJECT" ) ) /* Objects  */
         {
            supermob->tempnum = -9999;
            fread_obj( supermob, fp, OS_CARRY );
         }
         else if( !str_cmp( word, "END" ) )  /* Done */
            break;
         else
         {
            bug( "%s: bad section. %s", __FUNCTION__, aucmob->short_descr );
            break;
         }
      }
      FCLOSE( fp );

      list < obj_data * >::iterator iobj;
      for( iobj = supermob->carrying.begin(  ); iobj != supermob->carrying.end(  ); )
      {
         obj_data *tobj = *iobj;
         ++iobj;

         if( tobj->year == 0 )
         {
            tobj->day = time_info.day;
            tobj->month = time_info.month;
            tobj->year = time_info.year + 1;
         }
         tobj->from_char(  );
         tobj->to_room( aucvault, supermob );
      }
      release_supermob(  );
   }
   else
      log_string( "Cannot open auction vault" );
}

void load_aucvaults( void )
{
   DIR *dp;
   struct dirent *dentry;
   char directory_name[100];

   mudstrlcpy( directory_name, AUC_DIR, 100 );
   dp = opendir( directory_name );
   dentry = readdir( dp );
   while( dentry )
   {
      if( dentry->d_name[0] != '.' )
      {
         /*
          * Added by Tarl 3 Dec 02 because we are now using CVS 
          */
         if( str_cmp( dentry->d_name, "CVS" ) )
         {
            if( str_cmp( dentry->d_name, "sales.dat" ) )
               read_aucvault( directory_name, dentry->d_name );
         }
      }
      dentry = readdir( dp );
   }
   closedir( dp );
}

void save_aucvault( char_data * ch, const string & aucmob )
{
   room_index *aucvault;
   FILE *fp;
   char filename[256];
   short templvl;

   if( !ch )
   {
      bug( "%s: NULL ch!", __FUNCTION__ );
      return;
   }

   aucvault = get_room_index( ch->in_room->vnum + 1 );

   snprintf( filename, 256, "%s%s", AUC_DIR, aucmob.c_str(  ) );
   if( !( fp = fopen( filename, "w" ) ) )
   {
      bug( "%s: fopen", __FUNCTION__ );
      perror( filename );
   }
   else
   {
      templvl = ch->level;
      ch->level = LEVEL_AVATAR;  /* make sure EQ doesn't get lost */
      if( !aucvault->objects.empty(  ) )
         fwrite_obj( ch, aucvault->objects, NULL, fp, 0, false );
      fprintf( fp, "%s", "#END\n" );
      ch->level = templvl;
      FCLOSE( fp );
   }
}

char_data *find_auctioneer( char_data * ch )
{
   list < char_data * >::iterator ich;

   for( ich = ch->in_room->people.begin(  ); ich != ch->in_room->people.end(  ); ++ich )
   {
      char_data *auc = *ich;
      if( auc->isnpc(  ) && ( auc->has_actflag( ACT_AUCTION ) || auc->has_actflag( ACT_GUILDAUC ) ) )
         return auc;
   }
   return NULL;
}

/*
 * The following code was written by Erwin Andreasen for the automated
 * auction command.
 *
 * Completely cleaned up by Thoric
 */

/*
  util function, converts an 'advanced' ASCII-number-string into a number.
  Used by parsebet() but could also be used by do_give or do_wimpy.

  Advanced strings can contain 'k' (or 'K') and 'm' ('M') in them, not just
  numbers. The letters multiply whatever is left of them by 1,000 and
  1,000,000 respectively. Example:

  14k = 14 * 1,000 = 14,000
  23m = 23 * 1,000,0000 = 23,000,000

  If any digits follow the 'k' or 'm', the are also added, but the number
  which they are multiplied is divided by ten, each time we get one left. This
  is best illustrated in an example :)

  14k42 = 14 * 1000 + 14 * 100 + 2 * 10 = 14420

  Of course, it only pays off to use that notation when you can skip many 0's.
  There is not much point in writing 66k666 instead of 66666, except maybe
  when you want to make sure that you get 66,666.

  More than 3 (in case of 'k') or 6 ('m') digits after 'k'/'m' are automatically
  disregarded. Example:

  14k1234 = 14,123

  If the number contains any other characters than digits, 'k' or 'm', the
  function returns 0. It also returns 0 if 'k' or 'm' appear more than
  once.
*/

int advatoi( const char *s )
{
   int number = 0;   /* number to be returned */
   int multiplier = 0;  /* multiplier used to get the extra digits right */

   /*
    * as long as the current character is a digit add to current number
    */
   while( isdigit( s[0] ) )
      number = ( number * 10 ) + ( *s++ - '0' );

   switch ( UPPER( s[0] ) )
   {
      case 'K':
         number *= ( multiplier = 1000 );
         ++s;
         break;
      case 'M':
         number *= ( multiplier = 1000000 );
         ++s;
         break;
      case '\0':
         break;
      default:
         return 0;   /* not k nor m nor NULL - return 0! */
   }

   /*
    * if any digits follow k/m, add those too 
    */
   while( isdigit( s[0] ) && ( multiplier > 1 ) )
   {
      /*
       * the further we get to right, the less the digit 'worth' 
       */
      multiplier /= 10;
      number = number + ( ( *s++ - '0' ) * multiplier );
   }

   /*
    * return 0 if non-digit character was found, other than NULL 
    */
   if( s[0] != '\0' && !isdigit( s[0] ) )
      return 0;

   /*
    * anything left is likely extra digits (ie: 14k4443  -> 3 is extra) 
    */
   return number;
}

/*
  This function allows the following kinds of bets to be made:

  Absolute bet
  ============

  bet 14k, bet 50m66, bet 100k

  Relative bet
  ============

  These bets are calculated relative to the current bet. The '+' symbol adds
  a certain number of percent to the current bet. The default is 25, so
  with a current bet of 1000, bet + gives 1250, bet +50 gives 1500 etc.
  Please note that the number must follow exactly after the +, without any
  spaces!

  The '*' or 'x' bet multiplies the current bet by the number specified,
  defaulting to 2. If the current bet is 1000, bet x  gives 2000, bet x10
  gives 10,000 etc.

*/
int parsebet( const int currentbet, const char *s )
{
   /*
    * check to make sure it's not blank 
    */
   if( s[0] != '\0' )
   {
      /*
       * if first char is a digit, use advatoi 
       */
      if( isdigit( s[0] ) )
         return advatoi( s );
      if( s[0] == '+' ) /* add percent (default 25%) */
      {
         if( s[1] == '\0' )
            return ( currentbet * 125 ) / 100;
         return ( currentbet * ( 100 + atoi( s + 1 ) ) ) / 100;
      }
      if( s[0] == '*' || s[0] == 'x' ) /* multiply (default is by 2) */
      {
         if( s[1] == '\0' )
            return ( currentbet * 2 );
         return ( currentbet * atoi( s + 1 ) );
      }
   }
   return 0;
}

/*
 * this function sends raw argument over the AUCTION: channel
 * I am not too sure if this method is right..
 */
void talk_auction( const char *fmt, ... )
{
   list < descriptor_data * >::iterator ds;
   char buf[MSL];
   va_list args;

   va_start( args, fmt );
   vsnprintf( buf, MSL, fmt, args );
   va_end( args );

   for( ds = dlist.begin(  ); ds != dlist.end(  ); ++ds )
   {
      descriptor_data *d = *ds;
      char_data *original = d->original ? d->original : d->character;   /* if switched */

      if( d->connected == CON_PLAYING && hasname( original->pcdata->chan_listen, "auction" ) && !original->in_room->flags.test( ROOM_SILENCE ) && original->level > 1 )
         act_printf( AT_AUCTION, original, NULL, NULL, TO_CHAR, "Auction: %s", buf );
   }
}

/* put an item on auction, or see the stats on the current item or bet */
void bid( char_data * ch, char_data * buyer, const string & argument )
{
   obj_data *obj;
   string arg, arg1, arg2;

   arg = argument;
   arg = one_argument( arg, arg1 );
   arg = one_argument( arg, arg2 );

   ch->set_color( AT_AUCTION );

   /*
    * Normal NPCs not allowed to auction - Samson 6-23-99 
    */
   if( ch->isnpc(  ) && ch != supermob )
   {
      if( !ch->has_actflag( ACT_GUILDAUC ) && !ch->has_actflag( ACT_AUCTION ) )
         return;
   }

   if( arg1.empty(  ) )
   {
      if( auction->item != NULL )
      {
         obj = auction->item;

         /*
          * show item data here 
          */
         if( auction->bet > 0 )
            ch->printf( "\r\nCurrent bid on this item is %d gold.\r\n", auction->bet );
         else
            ch->printf( "\r\nNo bids on this item have been received.\r\n" );

         if( ch->is_immortal(  ) )
            ch->printf( "Seller: %s.  Bidder: %s.  Round: %d.\r\n", auction->seller->name, auction->buyer->name, ( auction->going + 1 ) );
         return;
      }
      else
      {
         ch->print( "\r\nThere is nothing being auctioned right now.\r\n" );
         return;
      }
   }

   if( ( ch->is_immortal(  ) || ch == supermob ) && !str_cmp( arg1, "stop" ) )
   {
      if( auction->item == NULL )
      {
         ch->print( "There is no auction to stop.\r\n" );
         return;
      }
      else  /* stop the auction */
      {
         room_index *aucvault;

         aucvault = get_room_index( auction->seller->in_room->vnum + 1 );

         talk_auction( "Sale of %s has been stopped by an Immortal.", auction->item->short_descr );

         auction->item->to_room( aucvault, auction->seller );
         /*
          * Make sure vault is saved when we stop an auction - Samson 6-23-99 
          */
         save_aucvault( auction->seller, auction->seller->short_descr );

         auction->item = NULL;
         if( auction->buyer != NULL && auction->buyer != auction->seller )
            auction->buyer->print( "Your bid has been cancelled.\r\n" );
         return;
      }
   }

   if( !str_cmp( arg1, "bid" ) )
   {
      if( auction->item != NULL )
      {
         int newbet;

         if( ch == auction->seller )
         {
            ch->print( "You can't bid on your own item!\r\n" );
            return;
         }

         if( !str_cmp( ch->name, auction->item->seller ) )
         {
            ch->print( "You cannot bid on your own item!\r\n" );
            return;
         }

         /*
          * make - perhaps - a bet now 
          */
         if( arg2.empty(  ) )
         {
            ch->print( "Bid how much?\r\n" );
            return;
         }

         newbet = parsebet( auction->bet, arg2.c_str(  ) );

         if( newbet < auction->starting + 500 )
         {
            ch->print( "You must place a bid that is higher than the starting bid.\r\n" );
            return;
         }

         /*
          * to avoid slow auction, use a bigger amount than 100 if the bet
          * is higher up - changed to 10000 for our high economy
          */
         if( newbet < ( auction->bet + 500 ) )
         {
            ch->print( "You must at least bid 500 coins over the current bid.\r\n" );
            return;
         }

         if( newbet > ch->gold )
         {
            ch->print( "You don't have that much money!\r\n" );
            return;
         }

         if( newbet > 2000000000 )
         {
            ch->print( "You can't bid over 2 billion coins.\r\n" );
            return;
         }

         /*
          * Is it the item they really want to bid on? --Shaddai 
          */
         if( arg.empty(  ) || !hasname( auction->item->name, arg ) )
         {
            ch->print( "That item is not being auctioned right now.\r\n" );
            return;
         }
         /*
          * the actual bet is OK! 
          */

         auction->buyer = ch;
         auction->bet = newbet;
         auction->going = 0;

         talk_auction( "A bid of %d gold has been received on %s.\r\n", newbet, auction->item->short_descr );

         return;
      }
      else
      {
         ch->print( "There isn't anything being auctioned right now.\r\n" );
         return;
      }
   }
/* finally... */

   if( !ch->isnpc(  ) ) /* Blocks PCs from bypassing the auctioneer - Samson 6-23-99 */
   {
      ch->print( "Only an auctioneer can start the bidding for an item!\r\n" );
      return;
   }

   if( ms_find_obj( ch ) )
      return;

   if( !( obj = ch->get_obj_carry( arg1 ) ) )   /* does char have the item ? */
   {
      bug( "%s: Auctioneer %s isn't carrying the item!", __FUNCTION__, ch->short_descr );
      return;
   }

   if( auction->item == NULL )
   {
      switch ( obj->item_type )
      {
         default:
            log_printf( "%s: Auctioneer %s tried to auction invalid item type!", __FUNCTION__, ch->short_descr );
            return;

            /*
             * insert any more item types here... items with a timer MAY NOT BE AUCTIONED! 
             */
         case ITEM_LIGHT:
         case ITEM_TREASURE:
         case ITEM_POTION:
         case ITEM_CONTAINER:
         case ITEM_INSTRUMENT:
         case ITEM_KEYRING:
         case ITEM_QUIVER:
         case ITEM_DRINK_CON:
         case ITEM_FOOD:
         case ITEM_COOK:
         case ITEM_PEN:
         case ITEM_CAMPGEAR:
         case ITEM_BOAT:
         case ITEM_PILL:
         case ITEM_PIPE:
         case ITEM_HERB_CON:
         case ITEM_INCENSE:
         case ITEM_FIRE:
         case ITEM_RUNEPOUCH:
         case ITEM_MAP:
         case ITEM_BOOK:
         case ITEM_RUNE:
         case ITEM_MATCH:
         case ITEM_HERB:
         case ITEM_WEAPON:
         case ITEM_MISSILE_WEAPON:
         case ITEM_ARMOR:
         case ITEM_STAFF:
         case ITEM_WAND:
         case ITEM_SCROLL:
         case ITEM_ORE:
         case ITEM_PROJECTILE:
            obj->separate(  );
            obj->from_char(  );
            auction->item = obj;
            auction->bet = 0;
            if( buyer == NULL )
               auction->buyer = ch;
            else
               auction->buyer = buyer;
            auction->seller = ch;
            auction->going = 0;
            auction->starting = obj->bid;

            if( auction->starting > 0 )
               auction->bet = auction->starting;

            talk_auction( "Bidding begins on %s at %d gold.", obj->short_descr, auction->starting );

            /*
             * Setup the auction event 
             */
            add_event( sysdata->auctionseconds, ev_auction, NULL );
            return;
      }  /* switch */
   }
   else
   {
      act( AT_TELL, "Try again later - $p is being auctioned right now!", ch, auction->item, NULL, TO_CHAR );
      if( !ch->is_immortal(  ) )
         ch->WAIT_STATE( sysdata->pulseviolence );
   }
}

CMDF( do_bid )
{
   string buf;

   if( ch->is_immortal(  ) )
   {
      if( argument.empty(  ) )
      {
         bid( ch, NULL, "" );
         return;
      }

      if( !str_cmp( argument, "stop" ) )
      {
         bid( ch, NULL, "stop" );
         return;
      }
   }
   buf = "bid " + argument;
   bid( ch, NULL, buf );
}

CMDF( do_identify )
{
   room_index *aucvault, *original;
   obj_data *obj;
   char_data *auc;
   double idcost;
   bool found = false;

   if( ch->isnpc(  ) )
   {
      ch->print( "Mobs cannot use this command.\r\n" );
      return;
   }

   ch->set_color( AT_AUCTION );

   if( !( auc = find_auctioneer( ch ) ) )
   {
      ch->print( "You must go to an auction house to use this command.\r\n" );
      return;
   }

   if( !ch->in_room->flags.test( ROOM_AUCTION ) )
   {
      ch->print( "This is not an auction house!\r\n" );
      bug( "%s: Auction mob in non-auction room %d!", __FUNCTION__, ch->in_room->vnum );
      return;
   }

   aucvault = get_room_index( ch->in_room->vnum + 1 );

   if( !aucvault )
   {
      ch->print( "This is not an auction house!\r\n" );
      bug( "%s: Missing auction vault for room %d!", __FUNCTION__, ch->in_room->vnum );
      return;
   }

   if( argument.empty(  ) )
   {
      ch->print( "What would you like identified?\r\n" );
      return;
   }

   original = ch->in_room;

   ch->from_room(  );
   if( !ch->to_room( aucvault ) )
      log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
   obj = get_obj_list( ch, argument, ch->in_room->objects );
   ch->from_room(  );
   if( !ch->to_room( original ) )
      log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );

   if( !obj || ( obj->buyer != NULL && str_cmp( obj->buyer, "" ) ) )
   {
      act_printf( AT_TELL, auc, NULL, ch, TO_VICT, "$n tells you 'There isn't a %s being offered.'", argument.c_str(  ) );
      return;
   }

   if( !str_cmp( obj->seller, "" ) || !obj->seller )
   {
      act_printf( AT_TELL, auc, NULL, ch, TO_VICT, "$n tells you 'There isn't a %s being offered.'", argument.c_str(  ) );
      bug( "%s: Object with no seller - %s", __FUNCTION__, obj->short_descr );
      return;
   }

   idcost = 5000 + ( obj->cost * 0.1 );

   if( ch->gold - idcost < 0 )
   {
      act( AT_TELL, "$n tells you 'You cannot afford to identify that!'", auc, NULL, ch, TO_VICT );
      return;
   }

   clan_data *clan = NULL;
   if( auc->has_actflag( ACT_GUILDAUC ) )
   {
      list < clan_data * >::iterator cl;

      for( cl = clanlist.begin(  ); cl != clanlist.end(  ); ++cl )
      {
         clan = *cl;
         if( clan->auction == auc->pIndexData->vnum )
         {
            found = true;
            break;
         }
      }
   }

   if( found && clan == ch->pcdata->clan )
      ;
   else
   {
      act_printf( AT_AUCTION, auc, NULL, ch, TO_VICT, "$n charges you %d gold for the identification.", idcost );
      ch->gold -= ( int )idcost;
      if( found && clan->bank )
      {
         clan->balance += ( int )idcost;
         save_clan( clan );
      }
   }
   obj_identify_output( ch, obj );
}

CMDF( do_collect )
{
   room_index *aucvault, *original;
   obj_data *obj;
   char_data *auc;
   bool found = false;

   if( ch->isnpc(  ) )
   {
      ch->print( "Mobs cannot use this command.\r\n" );
      return;
   }

   if( !( auc = find_auctioneer( ch ) ) )
   {
      ch->print( "You must go to an auction house to collect for an auction.\r\n" );
      return;
   }

   if( !ch->in_room->flags.test( ROOM_AUCTION ) )
   {
      ch->print( "This is not an auction house!\r\n" );
      bug( "%s: Auction mob in non-auction room %d!", __FUNCTION__, ch->in_room->vnum );
      return;
   }

   aucvault = get_room_index( ch->in_room->vnum + 1 );

   if( !aucvault )
   {
      ch->print( "This is not an auction house!\r\n" );
      bug( "%s: Missing auction vault for room %d!", __FUNCTION__, ch->in_room->vnum );
      return;
   }

   if( argument.empty(  ) )
   {
      ch->print( "Are you here to collect an item, or some money?\r\n" );
      return;
   }

   if( !str_cmp( argument, "money" ) )
   {
      list < sale_data * >::iterator sl;
      double totalfee = 0, totalnet = 0, fee, net;
      bool getsome = false;

      for( sl = salelist.begin(  ); sl != salelist.end(  ); )
      {
         sale_data *sold = *sl;
         ++sl;

         if( str_cmp( sold->get_seller(  ), ch->name ) )
            continue;

         if( !sold->get_collected(  ) && str_cmp( sold->get_buyer(  ), "The Code" ) )
         {
            act_printf( AT_AUCTION, auc, NULL, ch, TO_VICT, "%s has not collected %s yet.", sold->get_buyer(  ).c_str(  ), sold->get_item(  ).c_str(  ) );
            continue;
         }

         getsome = true;

         fee = ( sold->get_bid(  ) * 0.05 );
         net = sold->get_bid(  ) - fee;

         act_printf( AT_AUCTION, auc, NULL, ch, TO_VICT, "$n sold %s to %s for %d gold.", sold->get_item(  ).c_str(  ), sold->get_buyer(  ).c_str(  ), sold->get_bid(  ) );

         totalfee += fee;
         totalnet += net;

         deleteptr( sold );
      }

      if( !getsome )
      {
         act( AT_TELL, "$n tells you 'But you have not sold anything here!'", auc, NULL, ch, TO_VICT );
         return;
      }

      act_printf( AT_AUCTION, auc, NULL, ch, TO_VICT, "$n collects his fee of %d, and hands you %d gold.", totalfee, totalnet );
      act( AT_AUCTION, "$n collects his fees and hands $N some gold.", auc, NULL, ch, TO_NOTVICT );

      ch->gold += ( int )totalnet;
      ch->save(  );

      clan_data *clan = NULL;
      if( auc->has_actflag( ACT_GUILDAUC ) )
      {
         list < clan_data * >::iterator cl;

         for( cl = clanlist.begin(  ); cl != clanlist.end(  ); ++cl )
         {
            clan = *cl;
            if( clan->auction == auc->pIndexData->vnum )
            {
               found = true;
               break;
            }
         }
      }

      if( found && clan->bank )
      {
         clan->balance += ( int )totalfee;
         save_clan( clan );
      }
      return;
   }

   original = ch->in_room;

   ch->from_room(  );
   if( !ch->to_room( aucvault ) )
      log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
   obj = get_obj_list( ch, argument, ch->in_room->objects );
   ch->from_room(  );
   if( !ch->to_room( original ) )
      log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );

   if( !obj )
   {
      act_printf( AT_TELL, auc, NULL, ch, TO_VICT, "$n tells you 'There isn't a %s being sold.'", argument.c_str(  ) );
      return;
   }

   if( !str_cmp( obj->seller, ch->name ) && ( !str_cmp( obj->buyer, "" ) || obj->buyer == NULL ) )
   {
      double fee = ( obj->cost * .05 );

      if( obj->ego >= sysdata->minego )
         fee = ( obj->ego * .20 );  /* 20% handling charge for rare goods being returned */

      act( AT_AUCTION, "$n returns $p to $N.", auc, obj, ch, TO_NOTVICT );
      act( AT_AUCTION, "$n returns $p to you.", auc, obj, ch, TO_VICT );

      obj->from_room(  );
      obj->to_char( ch );
      save_aucvault( auc, auc->short_descr );

      clan_data *clan = NULL;
      if( auc->has_actflag( ACT_GUILDAUC ) )
      {
         list < clan_data * >::iterator cl;

         for( cl = clanlist.begin(  ); cl != clanlist.end(  ); ++cl )
         {
            if( clan->auction == auc->pIndexData->vnum )
            {
               found = true;
               break;
            }
         }
      }

      if( found && clan == ch->pcdata->clan )
         ;
      else
      {
         ch->gold -= ( int )fee;
         ch->save(  );
         act_printf( AT_AUCTION, auc, NULL, ch, TO_VICT, "$n charges you a fee of %d for $s services.", fee );
         if( found && clan->bank )
         {
            clan->balance += ( int )fee;
            save_clan( clan );
         }
      }
      return;
   }

   if( str_cmp( obj->buyer, ch->name ) )
   {
      act_printf( AT_TELL, auc, NULL, ch, TO_VICT, "$n tells you 'But you didn't win the bidding on %s!'", obj->short_descr );
      return;
   }

   if( ch->gold < obj->bid )
   {
      act( AT_TELL, "$n tells you 'You can't afford the bid, come back when you have the gold.", auc, NULL, ch, TO_VICT );
      return;
   }

   ch->gold -= obj->bid;
   obj->separate(  );
   obj->from_room(  );
   obj->to_char( ch );
   obj->extra_flags.reset( ITEM_AUCTION );
   save_aucvault( auc, auc->short_descr );
   ch->save(  );

   sale_data *sold = check_sale( auc->short_descr, obj->seller, obj->short_descr );

   if( sold )
   {
      sold->set_collected( true );
      save_sales(  );
   }

   act( AT_ACTION, "$n collects your money, and hands you $p.", auc, obj, ch, TO_VICT );
   act( AT_ACTION, "$n collects $N's money, and hands $M $p.", auc, obj, ch, TO_NOTVICT );

   STRFREE( obj->seller );
   STRFREE( obj->buyer );
   obj->bid = 0;
}

void auction_value( char_data * ch, char_data * auc, const string & argument )
{
   room_index *aucvault, *original;
   obj_data *obj;

   if( argument.empty(  ) )
   {
      ch->print( "Check bid on what item?\r\n" );
      return;
   }

   if( !ch->in_room->flags.test( ROOM_AUCTION ) )
   {
      ch->print( "This is not an auction house!\r\n" );
      bug( "%s: Auction mob in non-auction room %d!", __FUNCTION__, ch->in_room->vnum );
      return;
   }

   aucvault = get_room_index( ch->in_room->vnum + 1 );

   if( !aucvault )
   {
      ch->print( "This is not an auction house!\r\n" );
      bug( "%s: Missing auction vault for room %d!", __FUNCTION__, ch->in_room->vnum );
      return;
   }

   original = ch->in_room;

   ch->from_room(  );
   if( !ch->to_room( aucvault ) )
      log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
   obj = get_obj_list( ch, argument, ch->in_room->objects );
   ch->from_room(  );
   if( !ch->to_room( original ) )
      log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );

   if( !obj || ( obj->buyer != NULL && str_cmp( obj->buyer, "" ) ) )
   {
      act_printf( AT_TELL, auc, NULL, ch, TO_VICT, "$n tells you 'There isn't a %s being offered.'", argument.c_str(  ) );
      return;
   }

   if( !str_cmp( obj->seller, "" ) || obj->seller == NULL )
   {
      act_printf( AT_TELL, auc, NULL, ch, TO_VICT, "$n tells you 'There isn't a %s being offered.'", argument.c_str(  ) );
      bug( "%s: Object with no seller - %s", __FUNCTION__, obj->short_descr );
      return;
   }
   ch->printf( "&[auction]%s : Offered by %s. Minimum bid: %d\r\n", obj->short_descr, obj->seller, obj->bid );
}

void auction_buy( char_data * ch, char_data * auc, const string & argument )
{
   room_index *aucvault, *original;
   obj_data *obj;
   ostringstream buf;

   if( argument.empty(  ) )
   {
      ch->print( "Start bidding on what item?\r\n" );
      return;
   }

   if( !ch->in_room->flags.test( ROOM_AUCTION ) )
   {
      ch->print( "This is not an auction house!\r\n" );
      bug( "%s: Auction mob in non-auction room %d!", __FUNCTION__, ch->in_room->vnum );
      return;
   }

   aucvault = get_room_index( ch->in_room->vnum + 1 );

   if( !aucvault )
   {
      ch->print( "This is not an auction house!\r\n" );
      bug( "%s: Missing auction vault for room %d!", __FUNCTION__, ch->in_room->vnum );
      return;
   }

   if( auction->item )
   {
      act( AT_TELL, "$n tells you 'Wait until the current item has been sold.'", auc, NULL, ch, TO_VICT );
      return;
   }

   original = ch->in_room;

   ch->from_room(  );
   if( !ch->to_room( aucvault ) )
      log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
   obj = get_obj_list( ch, argument, ch->in_room->objects );
   ch->from_room(  );
   if( !ch->to_room( original ) )
      log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );

   if( !obj )
   {
      act_printf( AT_TELL, auc, NULL, ch, TO_VICT, "$n tells you 'There isn't a %s being offered.'", argument.c_str(  ) );
      return;
   }

   if( !str_cmp( obj->seller, "" ) || obj->seller == NULL )
   {
      act_printf( AT_TELL, auc, NULL, ch, TO_VICT, "$n tells you 'There isn't a %s being offered.'", argument.c_str(  ) );
      bug( "%s: Object with no seller - %s", __FUNCTION__, obj->short_descr );
      return;
   }

   if( obj->buyer != NULL && str_cmp( obj->buyer, "" ) )
   {
      act_printf( AT_TELL, auc, NULL, ch, TO_VICT, "$n tells you 'That item has already been sold to %s.'", obj->buyer );
      return;
   }

   if( !str_cmp( obj->seller, ch->name ) )
   {
      act( AT_TELL, "$n tells you 'You can't buy your own item!'", auc, NULL, ch, TO_VICT );
      return;
   }

   if( ch->gold < obj->bid )
   {
      act( AT_TELL, "$n tells you 'You don't have the money to back that bid!'", auc, NULL, ch, TO_VICT );
      return;
   }

   obj->separate(  );
   obj->from_room(  );
   obj->to_char( auc );
   /*
    * Could lose the item, but prevents cheaters from duplicating items - Samson 6-23-99 
    */
   save_aucvault( auc, auc->short_descr );

   buf << obj->name << " " << obj->bid;
   bid( auc, ch, buf.str(  ) );
}

void auction_sell( char_data * ch, char_data * auc, string & argument )
{
   string arg;
   room_index *aucvault;
   obj_data *obj;

   if( argument.empty(  ) )
   {
      ch->print( "Offer what for auction?\r\n" );
      return;
   }

   if( !ch->in_room->flags.test( ROOM_AUCTION ) )
   {
      ch->print( "This is not an auction house!\r\n" );
      bug( "%s: Auction mob in non-auction room %d!", __FUNCTION__, ch->in_room->vnum );
      return;
   }

   if( !( aucvault = get_room_index( ch->in_room->vnum + 1 ) ) )
   {
      ch->print( "This is not an auction house!\r\n" );
      bug( "%s: Missing auction vault for room %d!", __FUNCTION__, ch->in_room->vnum );
      return;
   }

   argument = one_argument( argument, arg );

   if( !( obj = ch->get_obj_carry( arg ) ) )
   {
      act( AT_TELL, "$n tells you 'You don't have that item.'", auc, NULL, ch, TO_VICT );
      return;
   }

   if( !ch->can_drop_obj( obj ) )
   {
      ch->print( "You can't let go of it, it's cursed!\r\n" );
      return;
   }

   if( obj->timer > 0 )
   {
      act( AT_TELL, "$n tells you, '$p is decaying too rapidly...'", auc, obj, ch, TO_VICT );
      return;
   }

   if( obj->extra_flags.test( ITEM_NOAUCTION ) )
   {
      ch->print( "That item cannot be auctioned!\r\n" );
      return;
   }

   if( obj->extra_flags.test( ITEM_DONATION ) )
   {
      ch->print( "You cannot auction off donated goods!\r\n" );
      return;
   }

   if( obj->ego >= sysdata->minego || obj->ego == -1 )
   {
      ch->print( "Sorry, rare items cannot be sold here.\r\n" );
      return;
   }

   if( obj->extra_flags.test( ITEM_PERSONAL ) )
   {
      ch->print( "Personal items may not be sold here.\r\n" );
      return;
   }

   int minbid = atoi( argument.c_str(  ) );

   if( minbid < 1 )
   {
      ch->print( "You must specify a bid greater than 0.\r\n" );
      return;
   }

   list < obj_data * >::iterator iobj;
   for( iobj = aucvault->objects.begin(  ); iobj != aucvault->objects.end(  ); ++iobj )
   {
      obj_data *sobj = *iobj;
      if( sobj && sobj->pIndexData->vnum == obj->pIndexData->vnum )
      {
         act( AT_TELL, "$n tells you '$p is already being offered. Come back later.'", auc, obj, ch, TO_VICT );
         return;
      }
   }

   short sellcount = 0;
   for( iobj = aucvault->objects.begin(  ); iobj != aucvault->objects.end(  ); ++iobj )
   {
      obj_data *sobj = *iobj;
      if( sobj && !str_cmp( sobj->seller, ch->name ) )
         ++sellcount;
   }

   if( sellcount > 9 )
   {
      act( AT_TELL, "$n tells you 'You may not have more than 10 items on sale at once.'", auc, NULL, ch, TO_VICT );
      return;
   }

   obj->separate(  );

   STRFREE( obj->seller );
   obj->seller = STRALLOC( ch->name );
   STRFREE( obj->buyer );
   obj->bid = minbid;
   act( AT_AUCTION, "$n offers $p up for auction.", ch, obj, NULL, TO_ROOM );
   act( AT_AUCTION, "You put $p up for auction.", ch, obj, NULL, TO_CHAR );

   talk_auction( "%s accepts %s at a minimum bid of %d.", auc->short_descr, obj->short_descr, obj->bid );

   obj->day = time_info.day;
   obj->month = time_info.month;
   obj->year = time_info.year + 1;

   obj->from_char(  );
   obj->to_room( aucvault, ch );
   ch->save(  );
   save_aucvault( auc, auc->short_descr );
}

void sweep_house( room_index * aucroom )
{
   char_data *aucmob;
   room_index *aucvault;
   bool found = false;

   if( !( aucmob = find_auctioneer( supermob ) ) )
      return;

   if( !( aucvault = get_room_index( aucroom->vnum + 1 ) ) )
   {
      bug( "%s: No vault room preset for auction house %d!", __FUNCTION__, aucroom->vnum );
      return;
   }

   list < obj_data * >::iterator iobj;
   for( iobj = aucvault->objects.begin(  ); iobj != aucvault->objects.end(  ); ++iobj )
   {
      obj_data *aucobj = *iobj;

      if( ( aucobj->day == time_info.day && aucobj->month == time_info.month && aucobj->year == time_info.year ) || ( time_info.year - aucobj->year > 1 ) )
      {
         clan_data *clan = NULL;

         aucobj->separate(  );
         aucobj->from_room(  );
         aucobj->to_char( aucmob );
         save_aucvault( aucmob, aucmob->short_descr );

         if( aucmob->has_actflag( ACT_GUILDAUC ) )
         {
            list < clan_data * >::iterator cl;

            for( cl = clanlist.begin(  ); cl != clanlist.end(  ); ++cl )
            {
               clan = *cl;

               if( clan->auction == aucmob->pIndexData->vnum )
               {
                  found = true;
                  break;
               }
            }
         }

         if( found && clan->storeroom )
         {
            room_index *clanroom = get_room_index( clan->storeroom );
            aucobj->from_char(  );
            aucobj->to_room( clanroom, NULL );
            aucmob->from_room(  );
            if( !aucmob->to_room( clanroom ) )
               log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
            save_clan_storeroom( aucmob, clan );
            aucmob->from_room(  );
            if( !aucmob->to_room( aucroom ) )
               log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
            talk_auction( "%s has turned %s over to %s.", aucmob->short_descr, aucobj->short_descr, clan->name.c_str(  ) );
         }
         else
         {
            aucobj->extra_flags.set( ITEM_DONATION );
            aucobj->from_char(  );
            aucobj->to_room( get_room_index( ROOM_VNUM_DONATION ), NULL );
            talk_auction( "%s donated %s to charity.", aucmob->short_descr, aucobj->short_descr );
         }
      }
   }
}

/* Sweep old crap from auction houses on daily basis (game time)- Samson 11-1-99 */
void clean_auctions( void )
{
   map < int, room_index * >::iterator iroom;

   for( iroom = room_index_table.begin(); iroom != room_index_table.end(); ++iroom )
   {
      room_index *pRoomIndex = iroom->second;

      if( pRoomIndex && pRoomIndex->flags.test( ROOM_AUCTION ) )
      {
         rset_supermob( pRoomIndex );
         sweep_house( pRoomIndex );
         release_supermob(  );
      }
   }
}
