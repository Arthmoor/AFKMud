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
 *                           IP Banning module                              *
 ****************************************************************************/

/*
 * Portions of this code are Copyright 2014 Peter M. Blair.
 * CIDR handling code is part of his libemail library. https://github.com/petermblair/libemail
 * Permission was granted to use the code in AFKMud and other projects as needed.
 */
#ifndef __BAN_H__
#define __BAN_H__

#define BAN_LIST SYSTEM_DIR "ban.lst" // List of bans

enum ban_types
{
   BAN_NAME, BAN_IP, BAN_CIDR
};

// Begin code from libemail
class IPADDR
{
   public:
      typedef unsigned long decimal_t;

   private:
      decimal_t _decimal;

   public:
      IPADDR( const string& ip );
      IPADDR( decimal_t d );

      const decimal_t & decimal() const;

      bool operator < (const IPADDR & rhs) const { return ( _decimal < rhs._decimal ); };

      const char * str() const;
};

class CIDR
{
 private:
   IPADDR::decimal_t _lower;
   IPADDR::decimal_t _upper;

 public:
   CIDR( const string& cidr );
   CIDR(){ };

   bool overlaps( const CIDR & ) const;

   const CIDR & operator=(const CIDR & c)
   {
      _lower = c.lower();
      _upper = c.upper();
      return *this;
   };

   bool operator()(const CIDR & c )
   {
      return overlaps( c );
   };

   inline bool contains( const IPADDR & ip ) const
   {
      IPADDR::decimal_t id = ip.decimal();

      if( id <= _upper )
         if( id >= _lower )
            return true;
      return false;
   };

   inline IPADDR::decimal_t lower() const { return _lower; };
   inline IPADDR::decimal_t upper() const { return _upper; };
};
// End code from libemail

/*
 * Site ban structure.
 */
class ban_data
{
 private:
   ban_data( const ban_data & b );
     ban_data & operator=( const ban_data & );

 public:
     ban_data(  );
    ~ban_data(  );

   string name;      // Name of the person being banned.
   string ipaddress; // Single IP x.x.x.x or CIDR in the form of x.x.x.x/y
   time_t expires;   // Time this ban expires. -1 indicates it won't.
   short type;       // The ban type.
};

bool is_banned( descriptor_data * );

#endif
