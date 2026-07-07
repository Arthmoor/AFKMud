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
 *                          Auction House module                            *
 ****************************************************************************/

#include <filesystem>
#include <fstream>
#include "mud.h"
#include "area.h"
#include "clans.h"
#include "descriptor.h"
#include "event.h"
#include "mobindex.h"
#include "mud_prog.h"
#include "objindex.h"
#include "roomindex.h"
// Had to move this down here because the C++ committee has too many wild hairs up their buts about how variadic functions work.
#include "auction.h"

auction_data *auction; // Global auction pointer.

std::list<sale_data *> salelist;

void save_sales( void )
{
   std::ofstream stream( std::filesystem::path{SALES_FILE} );
   if( !stream.is_open(  ) )
   {
      bug( "{}: Cannot open {} for writing: {}", __func__, SALES_FILE, std::strerror(errno) );
      return;
   }

   for( auto* sale : salelist )
   {
      stream << "#SALE\n";
      stream << std::format( "Aucmob    {}\n", sale->get_aucmob() );
      stream << std::format( "Seller    {}\n", sale->get_seller() );
      stream << std::format( "Buyer     {}\n", sale->get_buyer() );
      stream << std::format( "Item      {}\n", sale->get_item() );
      stream << std::format( "Bid       {}\n", sale->get_bid() );
      stream << std::format( "Collected {}\n", sale->get_collected() );
      stream << "End\n\n";
   }
   stream.close(  );
   if( stream.fail() )
      bug( "{}: Error occurred after closing {}: ", __func__, SALES_FILE, std::strerror(errno) );
}

sale_data::sale_data(  )
{
}

sale_data::~sale_data(  )
{
   salelist.remove( this );

   if( !mud_down )
      save_sales(  );
}

void add_sale( std::string_view aucmob, std::string_view seller, std::string_view buyer, std::string_view item, int bidamt, bool collected )
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
   for( auto it = salelist.begin(); it != salelist.end(); )
   {
      sale_data *sale = *it;
      ++it;

      deleteptr( sale );
   }
}

sale_data *check_sale( std::string_view aucmob, std::string_view pcname, std::string_view objname )
{
   for( auto* sale : salelist )
   {
      if( !str_cmp( aucmob, sale->get_aucmob(  ) ) )
      {
         if( !str_cmp( pcname, sale->get_seller(  ) ) )
         {
            if( !str_cmp( objname, sale->get_item(  ) ) )
               return sale;
         }
      }
   }
   return nullptr;
}

void sale_count( char_data * ch )
{
   short salecount = 0;

   for( auto* sale : salelist )
   {
      if( !str_cmp( sale->get_seller(  ), ch->name ) )
         ++salecount;
   }

   if( salecount > 0 )
   {
      ch->print_fmt( "&[auction]While you were gone, auctioneers sold {} items for you.\r\n", salecount );
      ch->print( "Use the 'saleslist' command to see which auctioneer sold what.\r\n" );
   }
}

CMDF( do_saleslist )
{
   short salecount = 0;

   for( auto* sale : salelist )
   {
      const std::string seller = sale->get_seller(  );

      if( !str_cmp( seller, ch->name ) || ch->is_imp(  ) )
      {
         ch->print_fmt( "&[auction]{} sold {} for {} while {} were away.\r\n", sale->get_aucmob(  ), sale->get_item(  ),
                     ( !str_cmp( seller, ch->name ) ? "you" : seller ), ( !str_cmp( seller, ch->name ) ? "you" : "they" ) );
         ++salecount;
      }
   }

   if( salecount == 0 )
      ch->print( "&[auction]You haven't sold any items at auction yet.\r\n" );
}

void prune_sales( void )
{
   for( auto it = salelist.begin(); it != salelist.end(); )
   {
      sale_data *sale = *it;
      ++it;

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
   std::ifstream stream( std::filesystem::path{SALES_FILE} );
   if( !stream.is_open() )
   {
      bug( "{}: Cannot open {} for reading: {}", __func__, SALES_FILE, std::strerror(errno) );
      return;
   }

   salelist.clear(  );
   sale_data *sale = nullptr;

   std::string key;
   while( stream >> key )
   {
      if( key == "#SALE" )
         sale = new sale_data;
      else if( key == "Aucmob" )
         sale->set_aucmob( fread_line( stream, '\n' ) );
      else if( key == "Seller" )
         sale->set_seller( fread_line( stream, '\n' ) );
      else if( key == "Buyer" )
         sale->set_buyer( fread_line( stream, '\n' ) );
      else if( key == "Item" )
         sale->set_item( fread_line( stream, '\n' ) );
      else if( key == "Bid" )
      {
         int bid;
         stream >> bid;

         sale->set_bid( bid );
      }
      else if( key == "Collected" )
      {
         bool collected;
         stream >> collected;

         sale->set_collected( collected );
      }
      else if( key == "End" )
         salelist.push_back( sale );
      else
      {
         bug( "{}: Bad section '{}' in {} - skipping.", __func__, key, SALES_FILE );
         fread_to_eol( stream );
      }
   }
   stream.close(  );
   prune_sales(  );
}

void read_aucvault( std::string_view dirname, std::string_view filename )
{
   room_index *aucvault;
   char_data *aucmob;
   FILE *fp;

   if( !( aucmob = supermob->get_char_world( filename ) ) )
   {
      bug( "{}: Um. Missing mob for {}'s auction vault.", __func__, filename );
      return;
   }

   aucvault = get_room_index( aucmob->in_room->vnum + 1 );

   if( !aucvault )
   {
      bug( "Ooops! The vault room for {}'s auction house is missing!", aucmob->short_descr );
      return;
   }

   std::filesystem::path fname = std::format( "{}{}", dirname, filename );
   if( ( fp = fopen( fname.c_str(), "r" ) ) != nullptr )
   {
      log_printf( "Loading auction house vault: {}", filename );
      rset_supermob( aucvault );

      for( ;; )
      {
         char letter;
         std::string word;

         letter = fread_letter( fp );
         if( letter == '*' )
         {
            fread_to_eol( fp );
            continue;
         }

         if( letter != '#' )
         {
            bug( "{}: # not found. {}", __func__, aucmob->short_descr );
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
            bug( "{}: bad section. {}", __func__, aucmob->short_descr );
            break;
         }
      }
      FCLOSE( fp );

      for( auto it = supermob->carrying.begin(  ); it != supermob->carrying.end(  ); )
      {
         obj_data *tobj = *it;
         ++it;

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
   if( !std::filesystem::exists( AUC_DIR ) || !std::filesystem::is_directory( AUC_DIR ) )
      return;

   for( const auto& entry : std::filesystem::directory_iterator( AUC_DIR ) )
   {
      const auto& path = entry.path();
      const std::string filename = path.filename().string();

      if( filename.empty() || filename[0] == '.' || filename == "sales.dat" )
         continue;

      read_aucvault( AUC_DIR, filename.c_str() );
   }
}

void save_aucvault( char_data * ch, std::string_view aucmob )
{
   room_index *aucvault;
   FILE *fp;

   if( !ch )
   {
      bug( "{}: nullptr ch!", __func__ );
      return;
   }

   aucvault = get_room_index( ch->in_room->vnum + 1 );

   std::filesystem::path filename = std::format( "{}{}", AUC_DIR, aucmob );
   if( !( fp = fopen( filename.c_str(), "w" ) ) )
   {
      bug( "{}: Unable to open {} for writing.", __func__, filename.string() );
      return;
   }

   short templvl = ch->level;
   ch->level = LEVEL_AVATAR;  /* make sure EQ doesn't get lost */

   if( !aucvault->objects.empty(  ) )
      fwrite_obj( ch, aucvault->objects, nullptr, fp, 0, false );
   fprintf( fp, "%s", "#END\n" );
   ch->level = templvl;
   FCLOSE( fp );
}

char_data *find_auctioneer( char_data * ch )
{
   for( auto* auc : ch->in_room->people )
   {
      if( auc->isnpc(  ) && ( auc->has_actflag( ACT_AUCTION ) || auc->has_actflag( ACT_GUILDAUC ) ) )
         return auc;
   }
   return nullptr;
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

   switch ( to_upper( s[0] ) )
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
         return 0;   /* not k nor m nor nullptr - return 0! */
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
    * return 0 if non-digit character was found, other than nullptr 
    */
   if( s[0] != '\0' && !isdigit( s[0] ) )
      return 0;

   /*
    * anything left is likely extra digits (ie: 14k4443  -> 3 is extra) 
    */
   return number;
}

// Because people on the C++ committee are raging assholes, this had to be split from the inline function over in auction.h.
void send_auction_broadcast( std::string_view buf )
{
   for( auto* d : dlist )
   {
      char_data *original = d->original ? d->original : d->character;   // if switched

      if( d->connected == CON_PLAYING && hasname( original->pcdata->chan_listen, "auction" )
         && !original->in_room->flags.test( ROOM_SILENCE ) && !original->in_room->area->flags.test( AFLAG_SILENCE ) && original->level > 1 )
         act_printf( AT_AUCTION, original, nullptr, nullptr, TO_CHAR, "Auction: {}", buf );
   }
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

/* put an item on auction, or see the stats on the current item or bet */
void bid( char_data * ch, char_data * buyer, std::string_view argument )
{
   obj_data *obj;
   std::string arg, arg1, arg2;

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
      if( auction->item != nullptr )
      {
         obj = auction->item;

         /*
          * show item data here 
          */
         if( auction->bet > 0 )
            ch->print_fmt( "\r\nCurrent bid on this item is {} gold.\r\n", auction->bet );
         else
            ch->print( "\r\nNo bids on this item have been received.\r\n" );

         if( ch->is_immortal(  ) )
            ch->print_fmt( "Seller: {}.  Bidder: {}.  Round: {}.\r\n", auction->seller->name, auction->buyer->name, ( auction->going + 1 ) );
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
      if( auction->item == nullptr )
      {
         ch->print( "There is no auction to stop.\r\n" );
         return;
      }
      else  /* stop the auction */
      {
         room_index *aucvault;

         aucvault = get_room_index( auction->seller->in_room->vnum + 1 );

         talk_auction( "Sale of {} has been stopped by an Immortal.", auction->item->short_descr );

         auction->item->to_room( aucvault, auction->seller );
         /*
          * Make sure vault is saved when we stop an auction - Samson 6-23-99 
          */
         save_aucvault( auction->seller, auction->seller->short_descr );

         auction->item = nullptr;
         if( auction->buyer != nullptr && auction->buyer != auction->seller )
            auction->buyer->print( "Your bid has been canceled.\r\n" );
         return;
      }
   }

   if( !str_cmp( arg1, "bid" ) )
   {
      if( auction->item != nullptr )
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

         talk_auction( "A bid of {} gold has been received on {}.\r\n", newbet, auction->item->short_descr );

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
      bug( "{}: Auctioneer {} isn't carrying the item!", __func__, ch->short_descr );
      return;
   }

   if( auction->item == nullptr )
   {
      switch ( obj->item_type )
      {
         default:
            log_printf( "{}: Auctioneer {} tried to auction invalid item type!", __func__, ch->short_descr );
            return;

            /*
             * insert any more item types here... items with a timer MAY NOT BE AUCTIONED! 
             */
         case ITEM_LIGHT:
         case ITEM_SCROLL:
         case ITEM_WAND:
         case ITEM_STAFF:
         case ITEM_WEAPON:
         case ITEM_SCABBARD:
         case ITEM_TREASURE:
         case ITEM_ARMOR:
         case ITEM_POTION:
         case ITEM_CLOTHING:
         case ITEM_DRINK_CON:
         case ITEM_KEY:
         case ITEM_FOOD:
         case ITEM_PEN:
         case ITEM_BOAT:
         case ITEM_PILL:
         case ITEM_PIPE:
         case ITEM_HERB_CON:
         case ITEM_HERB:
         case ITEM_INCENSE:
         case ITEM_BOOK:
         case ITEM_RUNE:
         case ITEM_RUNEPOUCH:
         case ITEM_MATCH:
         case ITEM_MAP:
         case ITEM_PAPER:
         case ITEM_TINDER:
         case ITEM_LOCKPICK:
         case ITEM_OIL:
         case ITEM_FUEL:
         case ITEM_PIECE:
         case ITEM_MISSILE_WEAPON:
         case ITEM_PROJECTILE:
         case ITEM_QUIVER:
         case ITEM_SHOVEL:
         case ITEM_SALVE:
         case ITEM_COOK:
         case ITEM_KEYRING:
         case ITEM_CAMPGEAR:
         case ITEM_DRINK_MIX:
         case ITEM_INSTRUMENT:
         case ITEM_ORE:
            obj->separate(  );
            obj->from_char(  );
            auction->item = obj;
            auction->bet = 0;
            if( buyer == nullptr )
               auction->buyer = ch;
            else
               auction->buyer = buyer;
            auction->seller = ch;
            auction->going = 0;
            auction->starting = obj->bid;

            if( auction->starting > 0 )
               auction->bet = auction->starting;

            talk_auction( "Bidding begins on {} at {} gold.", obj->short_descr, auction->starting );

            /*
             * Setup the auction event 
             */
            add_event( sysdata->auctionseconds, ev_auction, nullptr );
            return;
      }  /* switch */
   }
   else
   {
      act( AT_TELL, "Try again later - $p is being auctioned right now!", ch, auction->item, nullptr, TO_CHAR );
      if( !ch->is_immortal(  ) )
         ch->WAIT_STATE( sysdata->pulseviolence );
   }
}

// Called when the MUD starts.
void init_auction( void )
{
   auction = new auction_data;
   auction->item = nullptr;
}

// Called when the MUD shuts down.
void clear_auction( void )
{
   if( auction->item )
      bid( supermob, nullptr, "stop" );
}

CMDF( do_bid )
{
   std::string buf;

   if( ch->is_immortal(  ) )
   {
      if( argument.empty(  ) )
      {
         bid( ch, nullptr, "" );
         return;
      }

      if( !str_cmp( argument, "stop" ) )
      {
         bid( ch, nullptr, "stop" );
         return;
      }
   }
   buf = "bid " + argument;
   bid( ch, nullptr, buf );
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
      bug( "{}: Auction mob in non-auction room {}!", __func__, ch->in_room->vnum );
      return;
   }

   aucvault = get_room_index( ch->in_room->vnum + 1 );

   if( !aucvault )
   {
      ch->print( "This is not an auction house!\r\n" );
      bug( "{}: Missing auction vault for room {}!", __func__, ch->in_room->vnum );
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
      log_printf( "char_to_room: {}:{}, line {}.", __FILE__, __func__, __LINE__ );
   obj = get_obj_list( ch, argument, ch->in_room->objects );
   ch->from_room(  );
   if( !ch->to_room( original ) )
      log_printf( "char_to_room: {}:{}, line {}.", __FILE__, __func__, __LINE__ );

   if( !obj || obj->buyer.empty() )
   {
      act_printf( AT_TELL, auc, nullptr, ch, TO_VICT, "$n tells you 'There isn't a {} being offered.'", argument );
      return;
   }

   if( obj->seller.empty() )
   {
      act_printf( AT_TELL, auc, nullptr, ch, TO_VICT, "$n tells you 'There isn't a {} being offered.'", argument );
      bug( "{}: Object with no seller - {}", __func__, obj->short_descr );
      return;
   }

   idcost = 5000 + ( obj->cost * 0.1 );

   if( ch->gold - idcost < 0 )
   {
      act( AT_TELL, "$n tells you 'You cannot afford to identify that!'", auc, nullptr, ch, TO_VICT );
      return;
   }

   clan_data *clan = nullptr;
   if( auc->has_actflag( ACT_GUILDAUC ) )
   {
      std::list<clan_data *>::iterator cl;

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
      act_printf( AT_AUCTION, auc, nullptr, ch, TO_VICT, "$n charges you {:0.0f} gold for the identification.", idcost );
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
      bug( "{}: Auction mob in non-auction room {}!", __func__, ch->in_room->vnum );
      return;
   }

   aucvault = get_room_index( ch->in_room->vnum + 1 );

   if( !aucvault )
   {
      ch->print( "This is not an auction house!\r\n" );
      bug( "{}: Missing auction vault for room {}!", __func__, ch->in_room->vnum );
      return;
   }

   if( argument.empty(  ) )
   {
      ch->print( "Are you here to collect an item, or some money?\r\n" );
      return;
   }

   if( !str_cmp( argument, "money" ) )
   {
      double totalfee = 0, totalnet = 0, fee, net;
      bool getsome = false;

      for( auto it = salelist.begin(); it != salelist.end(); )
      {
         sale_data *sold = *it;
         ++it;

         if( str_cmp( sold->get_seller(  ), ch->name ) )
            continue;

         if( !sold->get_collected(  ) && str_cmp( sold->get_buyer(  ), "The Code" ) )
         {
            act_printf( AT_AUCTION, auc, nullptr, ch, TO_VICT, "{} has not collected {} yet.", sold->get_buyer(  ), sold->get_item(  ) );
            continue;
         }

         getsome = true;

         fee = ( sold->get_bid(  ) * 0.05 );
         net = sold->get_bid(  ) - fee;

         act_printf( AT_AUCTION, auc, nullptr, ch, TO_VICT, "$n sold {} to {} for {} gold.", sold->get_item(  ), sold->get_buyer(  ), sold->get_bid(  ) );

         totalfee += fee;
         totalnet += net;

         deleteptr( sold );
      }

      if( !getsome )
      {
         act( AT_TELL, "$n tells you 'But you have not sold anything here!'", auc, nullptr, ch, TO_VICT );
         return;
      }

      act_printf( AT_AUCTION, auc, nullptr, ch, TO_VICT, "$n collects his fee of {:0.0f}, and hands you {:0.0f} gold.", totalfee, totalnet );
      act( AT_AUCTION, "$n collects his fees and hands $N some gold.", auc, nullptr, ch, TO_NOTVICT );

      ch->gold += ( int )totalnet;
      ch->save(  );

      clan_data *clan = nullptr;
      if( auc->has_actflag( ACT_GUILDAUC ) )
      {
         std::list < clan_data * >::iterator cl;

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
      log_printf( "char_to_room: {}:{}, line {}.", __FILE__, __func__, __LINE__ );
   obj = get_obj_list( ch, argument, ch->in_room->objects );
   ch->from_room(  );
   if( !ch->to_room( original ) )
      log_printf( "char_to_room: {}:{}, line {}.", __FILE__, __func__, __LINE__ );

   if( !obj )
   {
      act_printf( AT_TELL, auc, nullptr, ch, TO_VICT, "$n tells you 'There isn't a {} being sold.'", argument );
      return;
   }

   if( !str_cmp( obj->seller, ch->name ) && obj->buyer.empty() )
   {
      double fee = ( obj->cost * .05 );

      if( obj->ego >= sysdata->minego )
         fee = ( obj->ego * .20 );  /* 20% handling charge for rare goods being returned */

      act( AT_AUCTION, "$n returns $p to $N.", auc, obj, ch, TO_NOTVICT );
      act( AT_AUCTION, "$n returns $p to you.", auc, obj, ch, TO_VICT );

      obj->from_room(  );
      obj->to_char( ch );
      save_aucvault( auc, auc->short_descr );

      clan_data *clan = nullptr;
      if( auc->has_actflag( ACT_GUILDAUC ) )
      {
         std::list < clan_data * >::iterator cl;

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
         act_printf( AT_AUCTION, auc, nullptr, ch, TO_VICT, "$n charges you a fee of {:0.0f} for $s services.", fee );
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
      act_printf( AT_TELL, auc, nullptr, ch, TO_VICT, "$n tells you 'But you didn't win the bidding on {}!'", obj->short_descr );
      return;
   }

   if( ch->gold < obj->bid )
   {
      act( AT_TELL, "$n tells you 'You can't afford the bid, come back when you have the gold.", auc, nullptr, ch, TO_VICT );
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

   obj->seller.clear();
   obj->buyer.clear();
   obj->bid = 0;
}

void auction_value( char_data * ch, char_data * auc, std::string_view argument )
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
      bug( "{}: Auction mob in non-auction room {}!", __func__, ch->in_room->vnum );
      return;
   }

   aucvault = get_room_index( ch->in_room->vnum + 1 );

   if( !aucvault )
   {
      ch->print( "This is not an auction house!\r\n" );
      bug( "{}: Missing auction vault for room {}!", __func__, ch->in_room->vnum );
      return;
   }

   original = ch->in_room;

   ch->from_room(  );
   if( !ch->to_room( aucvault ) )
      log_printf( "char_to_room: {}:{}, line {}.", __FILE__, __func__, __LINE__ );
   obj = get_obj_list( ch, argument, ch->in_room->objects );
   ch->from_room(  );
   if( !ch->to_room( original ) )
      log_printf( "char_to_room: {}:{}, line {}.", __FILE__, __func__, __LINE__ );

   if( !obj || obj->buyer.empty() )
   {
      act_printf( AT_TELL, auc, nullptr, ch, TO_VICT, "$n tells you 'There isn't a {} being offered.'", argument );
      return;
   }

   if( obj->seller.empty() )
   {
      act_printf( AT_TELL, auc, nullptr, ch, TO_VICT, "$n tells you 'There isn't a {} being offered.'", argument );
      bug( "{}: Object with no seller - {}", __func__, obj->short_descr );
      return;
   }
   ch->print_fmt( "&[auction]{} : Offered by {}. Minimum bid: {}\r\n", obj->short_descr, obj->seller, obj->bid );
}

void auction_buy( char_data * ch, char_data * auc, std::string_view argument )
{
   room_index *aucvault, *original;
   obj_data *obj;
   std::ostringstream buf;

   if( argument.empty(  ) )
   {
      ch->print( "Start bidding on what item?\r\n" );
      return;
   }

   if( !ch->in_room->flags.test( ROOM_AUCTION ) )
   {
      ch->print( "This is not an auction house!\r\n" );
      bug( "{}: Auction mob in non-auction room {}!", __func__, ch->in_room->vnum );
      return;
   }

   aucvault = get_room_index( ch->in_room->vnum + 1 );

   if( !aucvault )
   {
      ch->print( "This is not an auction house!\r\n" );
      bug( "{}: Missing auction vault for room {}!", __func__, ch->in_room->vnum );
      return;
   }

   if( auction->item )
   {
      act( AT_TELL, "$n tells you 'Wait until the current item has been sold.'", auc, nullptr, ch, TO_VICT );
      return;
   }

   original = ch->in_room;

   ch->from_room(  );
   if( !ch->to_room( aucvault ) )
      log_printf( "char_to_room: {}:{}, line {}.", __FILE__, __func__, __LINE__ );
   obj = get_obj_list( ch, argument, ch->in_room->objects );
   ch->from_room(  );
   if( !ch->to_room( original ) )
      log_printf( "char_to_room: {}:{}, line {}.", __FILE__, __func__, __LINE__ );

   if( !obj )
   {
      act_printf( AT_TELL, auc, nullptr, ch, TO_VICT, "$n tells you 'There isn't a {} being offered.'", argument );
      return;
   }

   if( obj->seller.empty() )
   {
      act_printf( AT_TELL, auc, nullptr, ch, TO_VICT, "$n tells you 'There isn't a {} being offered.'", argument );
      bug( "{}: Object with no seller - {}", __func__, obj->short_descr );
      return;
   }

   if( !obj->buyer.empty() )
   {
      act_printf( AT_TELL, auc, nullptr, ch, TO_VICT, "$n tells you 'That item has already been sold to {}.'", obj->buyer );
      return;
   }

   if( !str_cmp( obj->seller, ch->name ) )
   {
      act( AT_TELL, "$n tells you 'You can't buy your own item!'", auc, nullptr, ch, TO_VICT );
      return;
   }

   if( ch->gold < obj->bid )
   {
      act( AT_TELL, "$n tells you 'You don't have the money to back that bid!'", auc, nullptr, ch, TO_VICT );
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

void auction_sell( char_data * ch, char_data * auc, std::string & argument )
{
   std::string arg;
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
      bug( "{}: Auction mob in non-auction room {}!", __func__, ch->in_room->vnum );
      return;
   }

   if( !( aucvault = get_room_index( ch->in_room->vnum + 1 ) ) )
   {
      ch->print( "This is not an auction house!\r\n" );
      bug( "{}: Missing auction vault for room {}!", __func__, ch->in_room->vnum );
      return;
   }

   argument = one_argument( argument, arg );

   if( !( obj = ch->get_obj_carry( arg ) ) )
   {
      act( AT_TELL, "$n tells you 'You don't have that item.'", auc, nullptr, ch, TO_VICT );
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

   if( obj->extra_flags.test( ITEM_CLANOBJECT ) )
   {
      ch->print( "You can't auction clan/guild items.\r\n" );
      return;
   }

   if( obj->extra_flags.test( ITEM_PERMANENT ) )
   {
      ch->print( "This item cannot leave your possession.\r\n" );
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

   for( auto* sobj : aucvault->objects )
   {
      if( sobj && sobj->pIndexData->vnum == obj->pIndexData->vnum )
      {
         act( AT_TELL, "$n tells you '$p is already being offered. Come back later.'", auc, obj, ch, TO_VICT );
         return;
      }
   }

   short sellcount = 0;
   for( auto* sobj2 : aucvault->objects )
   {
      if( sobj2 && !str_cmp( sobj2->seller, ch->name ) )
         ++sellcount;
   }

   if( sellcount > 9 )
   {
      act( AT_TELL, "$n tells you 'You may not have more than 10 items on sale at once.'", auc, nullptr, ch, TO_VICT );
      return;
   }

   obj->separate(  );

   obj->seller = ch->name;
   obj->buyer.clear();
   obj->bid = minbid;
   act( AT_AUCTION, "$n offers $p up for auction.", ch, obj, nullptr, TO_ROOM );
   act( AT_AUCTION, "You put $p up for auction.", ch, obj, nullptr, TO_CHAR );

   talk_auction( "{} accepts {} at a minimum bid of {}.", auc->short_descr, obj->short_descr, obj->bid );

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
      bug( "{}: No vault room preset for auction house {}!", __func__, aucroom->vnum );
      return;
   }

   for( auto* aucobj : aucvault->objects )
   {
      if( ( aucobj->day == time_info.day && aucobj->month == time_info.month && aucobj->year == time_info.year ) || ( time_info.year - aucobj->year > 1 ) )
      {
         clan_data *clan = nullptr;

         aucobj->separate(  );
         aucobj->from_room(  );
         aucobj->to_char( aucmob );
         save_aucvault( aucmob, aucmob->short_descr );

         if( aucmob->has_actflag( ACT_GUILDAUC ) )
         {
            std::list<clan_data *>::iterator cl;

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
            aucobj->to_room( clanroom, nullptr );
            aucmob->from_room(  );
            if( !aucmob->to_room( clanroom ) )
               log_printf( "char_to_room: {}:{}, line {}.", __FILE__, __func__, __LINE__ );
            save_clan_storeroom( aucmob, clan );
            aucmob->from_room(  );
            if( !aucmob->to_room( aucroom ) )
               log_printf( "char_to_room: {}:{}, line {}.", __FILE__, __func__, __LINE__ );
            talk_auction( "{} has turned {} over to {}.", aucmob->short_descr, aucobj->short_descr, clan->name );
         }
         else
         {
            aucobj->extra_flags.set( ITEM_DONATION );
            aucobj->from_char(  );
            aucobj->to_room( get_room_index( ROOM_VNUM_DONATION ), nullptr );
            talk_auction( "{} donated {} to charity.", aucmob->short_descr, aucobj->short_descr );
         }
      }
   }
}

/* Sweep old crap from auction houses on daily basis (game time)- Samson 11-1-99 */
void clean_auctions( void )
{
   std::map<int, room_index *>::iterator iroom;

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
