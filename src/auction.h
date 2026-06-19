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
 *                           Auction House module                           *
 ****************************************************************************/

#pragma once

inline constexpr std::string_view SALES_FILE = "../aucvaults/sales.dat";

struct auction_data
{
   obj_data *item = nullptr;      // Pointer to the item currently being auctioned.
   char_data *seller = nullptr;   // Pointer to the seller - which may NOT quit.
   char_data *buyer = nullptr;    // Pointer to the buyer - which may NOT quit.
   int bet = 0;                   // Last bid - or 0 if noone has bid anything.
   int starting = 0;              // The starting bid for the item on sale.
   short going = 0;               // Going once... Going twice... Sold!
};

/* Holds data for sold items - Samson 6-23-99 */
class sale_data
{
 private:
   sale_data( const sale_data & s );
     sale_data & operator=( const sale_data & );

 public:
     sale_data(  );
    ~sale_data(  );
   void set_aucmob( std::string_view name )
   {
      aucmob = name;
   }
   std::string get_aucmob(  )
   {
      return aucmob;
   }

   void set_seller( std::string_view name )
   {
      seller = name;
   }
   std::string get_seller(  )
   {
      return seller;
   }

   void set_buyer( std::string_view name )
   {
      buyer = name;
   }
   std::string get_buyer(  )
   {
      return buyer;
   }

   void set_item( std::string_view name )
   {
      item = name;
   }
   std::string get_item(  )
   {
      return item;
   }

   void set_bid( int value )
   {
      bid = value;
   }
   int get_bid(  )
   {
      return bid;
   }

   void set_collected( bool value )
   {
      collected = value;
   }
   bool get_collected(  )
   {
      return collected;
   }

 private:
   std::string aucmob;  // Name of the mob that is conducting the auction.
   std::string seller;  // Name of the player selling the item.
   std::string buyer;   // Name of the buyer who won the auction.
   std::string item;    // Name of the item being auctioned.
   int bid = 0;         // Current bid on the item.
   bool collected = 0;  // Weather or not the buyer has collected the item from the mob.
};

extern auction_data *auction;
void save_aucvault( char_data *, std::string_view );
void add_sale( std::string_view, std::string_view, std::string_view, std::string_view, int, bool );
void send_auction_broadcast( std::string_view );

/*
 * This template formats a message to be sent over the auction channel.
 * Once formatted it's then sent to the broadcast function in auction.cpp.
 * And yes, to the previous comment that was here, this is indeed the right method.
 */
inline void talk_auction( std::string_view fmt, auto&&... args )
{
   std::string buf;

   try
   {
      buf = std::vformat( fmt, std::make_format_args( args... ) );
   }
   catch( const std::exception & e )
   {
      // In case someone bodged a call to this that isn't formatted right.
      bug( "{}: Auction formatting error: {}", __func__, e.what() );
      return;
   }

   send_auction_broadcast( buf );
}
