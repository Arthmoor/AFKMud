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
 *                           Auction House module                           *
 ****************************************************************************/

#ifndef __AUCTION_H__
#define __AUCTION_H__

#define SALES_FILE AUC_DIR "sales.dat"

struct auction_data
{
   obj_data *item;   /* a pointer to the item */
   char_data *seller;   /* a pointer to the seller - which may NOT quit */
   char_data *buyer; /* a pointer to the buyer - which may NOT quit */
   int bet; /* last bet - or 0 if noone has bet anything */
   int starting;
   short going;   /* 1,2, sold */
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
   void set_aucmob( const string & name )
   {
      aucmob = name;
   }
   string get_aucmob(  )
   {
      return aucmob;
   }

   void set_seller( const string & name )
   {
      seller = name;
   }
   string get_seller(  )
   {
      return seller;
   }

   void set_buyer( const string & name )
   {
      buyer = name;
   }
   string get_buyer(  )
   {
      return buyer;
   }

   void set_item( const string & name )
   {
      item = name;
   }
   string get_item(  )
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
   string aucmob;
   string seller;
   string buyer;
   string item;
   int bid;
   bool collected;
};

extern auction_data *auction;
#endif
