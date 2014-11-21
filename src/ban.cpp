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
 *                           IP Banning module                              *
 ****************************************************************************/

/*
 * Portions of this code are Copyright 2014 Peter M. Blair.
 * CIDR handling code is part of his libemail library. https://github.com/petermblair/libemail
 * Permission was granted to use the code in AFKMud and other projects as needed.
 */

#include <arpa/inet.h>
#include <cmath>
#include <fstream>
#include "mud.h"
#include "ban.h"
#include "descriptor.h"
#include "event.h"

/* Global Variables */
list < ban_data * >banlist;

// Begin code from libemail
IPADDR::IPADDR( decimal_t d )
{
}

IPADDR::IPADDR( const string & ip )
{
   struct in_addr addr;

   inet_aton( ip.c_str() , &addr );
   _decimal = ntohl( addr.s_addr );
}

const IPADDR::decimal_t & IPADDR::decimal() const
{
   return _decimal;
}

const char * IPADDR::str() const
{
   struct in_addr addr;

   addr.s_addr = htonl( _decimal );
   return inet_ntoa( addr );
}

CIDR::CIDR( const string & cidr )
{
   string ip, pow;

   string::size_type pos = cidr.find_first_of( "/", 0 );

   if( pos != string::npos )
   {
      ip = cidr.substr( 0, pos );
      pow = cidr.substr( pos + 1, cidr.length() );
   }
   else
   {
      throw( "Could not parse: " + cidr );
   }

   IPADDR i( ip );
   _lower = i.decimal();

   int p = atoi( pow.c_str() );
   _upper = _lower + ( ::pow( 2, 32 - p ) -1 );
}

inline bool CIDR::overlaps( const CIDR & c ) const
{
   if( c.lower() >= _lower && c.lower() <= _upper )
      return true;

   if( c.upper() >= _lower && c.upper() <= _upper )
      return true;
   return false;
}
// End code from libemail

ban_data::ban_data(  )
{
}

ban_data::~ban_data(  )
{
   banlist.remove( this );
}

void free_bans( void )
{
   list < ban_data * >::iterator ban;

   for( ban = banlist.begin(  ); ban != banlist.end(  ); )
   {
      ban_data *pban = *ban;
      ++ban;

      deleteptr( pban );
   }
}

void load_banlist( void )
{
   ban_data *ban = nullptr;
   ifstream stream;

   banlist.clear(  );

   stream.open( BAN_LIST );
   if( !stream.is_open(  ) )
   {
      bug( "%s: Can't open %s", __func__, BAN_LIST );
      return;
   }

   do
   {
      string key, value;
      char buf[MSL];

      stream >> key;
      strip_lspace( key );

      if( key.empty(  ) )
         continue;

      stream.getline( buf, MSL );
      value = buf;

      strip_lspace( value );

      if( key == "#BAN" )
      {
         ban = new ban_data;
         init_memory( &ban->expires, &ban->type, sizeof( ban->type ) );
      }
      else if( key == "Name" )
         ban->name = value;
      else if( key == "IP" )
         ban->ipaddress = value;
      else if( key == "Expires" )
         ban->expires = atol( value.c_str() );
      else if( key == "Type" )
        ban->type = atoi( value.c_str() );
      else if( key == "End" )
      {
         banlist.push_back( ban );
      }
      else
         bug( "%s: Invalid key: %s", __func__, key.c_str(  ) );
   }
   while( !stream.eof(  ) );
   stream.close(  );

   add_event( 3600, ev_ban_check, nullptr );
}

void save_banlist( void )
{
   ofstream stream;

   stream.open( BAN_LIST );
   if( !stream.is_open(  ) )
   {
      bug( "%s: Can't write to %s", __func__, BAN_LIST );
      return;
   }

   list < ban_data * >::iterator bn;
   for( bn = banlist.begin(  ); bn != banlist.end(  ); ++bn )
   {
      ban_data *ban = *bn;

      stream << "#BAN" << endl;
      stream << "Name       " << ban->name << endl;
      stream << "IP         " << ban->ipaddress << endl;
      stream << "Expires    " << ban->expires << endl;
      stream << "Type       " << ban->type << endl;
      stream << "End" << endl << endl;
   }
   stream.close(  );
}

void check_ban_expirations( void )
{
   bool some_expired = false;

   list < ban_data * >::iterator bn;
   for( bn = banlist.begin(  ); bn != banlist.end(  ); )
   {
      ban_data *ban = *bn;
      ++bn;

      if( current_time > ban->expires && ban->expires != -1 )
      {
         deleteptr( ban );
         some_expired = true;
      }
   }

   if( some_expired )
      save_banlist();
}

bool is_valid_ip( const string& ipaddress )
{
   struct sockaddr_in sa;

   int result = inet_pton( AF_INET, ipaddress.c_str(), &(sa.sin_addr) );

   if( result > 0 )
      return true;
   return false;
}

bool is_valid_cidr( const string& cidr )
{
   string::size_type x;
   string ipaddress, cidr_string;
   int cidr_int;

   if( ( x = cidr.find( '/' ) ) != string::npos && x > 0 )
   {
      ipaddress = cidr.substr( 0, x );
      cidr_string = cidr.substr( x + 1, cidr.length(  ) );
      cidr_int = atoi( cidr_string.c_str() );

      if( !is_valid_ip( ipaddress ) )
         return false;

      if( cidr_int < 0 || cidr_int > 255 )
         return false;

      return true;
   }
   return false;
}

bool is_ip_range( const string& ip_address, const string& cidr_address )
{
   // Are we checking a valid IP?
   if( !is_valid_ip( ip_address ) )
   {
      bug( "%s: Invalid IP address format: %s", __func__, ip_address.c_str() );
      return false;
   }

   // Are we checking a valid CIDR?
   if( !is_valid_cidr( cidr_address ) )
   {
      bug( "%s: Invalid CIDR format: %s", __func__, cidr_address.c_str() );
      return false;
   }

   // Convert values to decimal notation.
   IPADDR ip( ip_address );
   CIDR cidr( cidr_address );

   // Compare them. If this matches, the IP is in the CIDR.
   if( cidr.contains( ip ) )
      return true;

   return false;
}

bool is_ip_match( const string& ipaddress, const string& stored_ip )
{
   // Are we checking a valid IP?
   if( !is_valid_ip( ipaddress ) )
   {
      bug( "%s: Invalid IP address format: %s", __func__, ipaddress.c_str() );
      return false;
   }

   // Is the stored IP valid?
   if( !is_valid_ip( stored_ip ) )
   {
      bug( "%s: Invalid stored IP address format: %s", __func__, stored_ip.c_str() );
      return false;
   }

   // Do they match? (just a string comparison)
   if( !str_cmp( ipaddress, stored_ip ) )
      return true;

   return false;
}

void send_ban_notice( descriptor_data *d, ban_data *ban )
{
   long duration = ban->expires - current_time;

   if( ban->type == BAN_NAME )
   {
      if( duration > 86400 )
         d->buffer_printf( "You are banned from this MUD for %ld days.\r\n", (duration / 86400) );
      else if( duration >= 0 && duration <= 86400 )
         d->buffer_printf( "You are banned from this MUD for %ld hours.\r\n", (duration / 3600) );
      else
         d->write_to_buffer( "You have been permanently banned from this MUD.\r\n" );
   }
   else
   {
      char buf[MSL];

      if( duration > 86400 )
         snprintf( buf, MSL, "Your site has been banned from this MUD for %ld days.\r\n", (duration / 86400) );
      else if( duration >= 0 && duration <= 86400 )
         snprintf( buf, MSL, "Your site has been banned from this MUD for %ld hours.\r\n", (duration / 3600) );
      else
         mudstrlcpy( buf, "Your site has been permanently banned from this MUD.\r\n", MSL );

      d->write( buf );
   }
}

bool is_banned( descriptor_data *d )
{
   list < ban_data * >::iterator bn;

   for( bn = banlist.begin(  ); bn != banlist.end(  ); ++bn )
   {
      ban_data *ban = *bn;

      // Check name bans first
      if( ban->type == BAN_NAME && !str_cmp( d->character->name, ban->name ) )
      {
         send_ban_notice( d, ban );
         return true;
      }

      // Check single IP bans
      if( ban->type == BAN_IP && is_ip_match( d->ipaddress, ban->ipaddress ) )
      {
         send_ban_notice( d, ban );
         return true;
      }

      // Check IP Range Bans
      if( ban->type == BAN_CIDR && is_ip_range( d->ipaddress, ban->ipaddress ) )
      {
         send_ban_notice( d, ban );
         return true;
      }
   }
   return false;
}

ban_data *get_ban( const string& name )
{
   list < ban_data * >::iterator bn;

   for( bn = banlist.begin(  ); bn != banlist.end(  ); ++bn )
   {
      ban_data *ban = *bn;

      if( !str_cmp( ban->name, name ) )
         return ban;

      if( !str_cmp( ban->ipaddress, name ) )
         return ban;
   }
   return nullptr;
}

CMDF( do_ban )
{
   ban_data *ban = nullptr;
   char_data *victim = nullptr;
   string arg1, arg2;

   if( argument.empty() )
   {
      ch->print( "&wUsage: ban list\r\n" );
      ch->print( "Usage: ban <player> <duration>\r\n" );
      ch->print( "Usage: ban <cidr or IP> <duration>\r\n" );
      ch->print( "Usage: ban remove <name, cidr, or IP>\r\n" );
      ch->print( "\r\nIf the specified player does not exist, the target will be assumed to be an IP address ban.\r\n" );
      ch->print( "Durations are in whole days and will expire automatically.\r\n" );
      ch->print( "If \"permanent\" is specified for the duration, the ban will never expire.\r\n" );
      return;
   }

   if( !str_cmp( argument, "list" ) )
   {
      list < ban_data * >::iterator bn;

      ch->print( "&w   Player    |   Address or CIDR   | Duration\r\n" );
      ch->print( "-------------+---------------------+-----------------\r\n" );

      for( bn = banlist.begin(  ); bn != banlist.end(  ); ++bn )
      {
         ban = *bn;
         long duration = ban->expires - current_time;

         ch->printf( "%-14s %-20s ", ban->name.c_str(), ban->ipaddress.c_str() );

         if( duration > 86400 )
            ch->printf( "%ld days\r\n", (duration / 86400) );
         else if( duration >= 0 && duration <= 86400 )
            ch->printf( "%ld hours\r\n", (duration / 3600) );
         else
            ch->print( "Permanent\r\n" );
      }
      return;
   }

   argument = one_argument( argument, arg1 );

   if( arg1.empty() )
   {
      do_ban( ch, "" );
      return;
   }

   if( !str_cmp( arg1, "test" ) )
   {
      argument = one_argument( argument, arg2 );

      if( is_ip_range( arg2, argument ) )
         ch->printf( "&w%s is in CIDR %s\r\n", arg2.c_str(), argument.c_str() );
      else
         ch->printf( "&w%s is not within CIDR %s\r\n", arg2.c_str(), argument.c_str() );

      return;
   }

   if( !str_cmp( arg1, "remove" ) )
   {
      ban = get_ban( argument );

      if( !ban )
      {
         ch->printf( "&wNo ban exists for %s\r\n", argument.c_str() );
         return;
      }

      deleteptr( ban );
      save_banlist();

      ch->printf( "&wBan lifted for %s\r\n", argument.c_str() );
      return;
   }

   if( ( victim = ch->get_char_world( arg1 ) ) || exists_player( arg1 ) )
   {
      long duration;
      bool inlist = false;

      if( victim )
      {
         if( victim->isnpc() )
         {
            ch->print( "&wYou cannot ban an NPC.\r\n" );
            return;
         }

         if( victim->level >= ch->level )
         {
            ch->printf( "You do not have sufficient access to affect %s.\r\n", victim->name );
            return;
         }
      }

      if( !str_cmp( argument, "permanent" ) )
         duration = -1;
      else
      {
         if( !is_number( argument ) )
         {
            ch->print( "&wYou must specify a numerical duration, or the word \"permanent\".\r\n" );
            return;
         }
         duration = atol( argument.c_str() );
         duration = duration * 86400;
      }

      if( ( ban = get_ban( arg1 ) ) )
         inlist = true;
      else
         ban = new ban_data;

      ban->name = arg1;
      ban->ipaddress = "0.0.0.0";
      ban->type = BAN_NAME;
      ban->expires = duration == -1 ? -1 : (current_time + duration);

      if( !inlist )
         banlist.push_back( ban );
      save_banlist();

      if( victim && victim->desc )
      {
         send_ban_notice( victim->desc, ban );
         victim->desc->disconnect = true;
      }

      if( inlist )
         ch->printf( "&wBan for %s has been updated.\r\n", arg1.c_str() );
      else
         ch->printf( "&w%s has been added to the ban list.\r\n", arg1.c_str() );
      return;
   }
   else
   {
      short type;
      long duration;
      bool inlist = false;
      bool validip = is_valid_ip( arg1 );
      bool validcidr = is_valid_cidr( arg1 );

      if( validcidr )
         type = BAN_CIDR;
      else if( validip )
         type = BAN_IP;
      else
      {
         ch->printf( "&w%s is not a valid IP or CIDR value.\r\n", arg1.c_str() );
         return;
      }

      if( !str_cmp( argument, "permanent" ) )
         duration = -1;
      else
      {
         if( !is_number( argument ) )
         {
            ch->print( "&wYou must specify a numerical duration, or the word \"permanent\".\r\n" );
            return;
         }
         duration = atol( argument.c_str() );
         duration = duration * 86400;
      }

      if( ( ban = get_ban( arg1 ) ) )
         inlist = true;
      else
         ban = new ban_data;

      ban->name = "All";
      ban->type = type;
      ban->ipaddress = arg1;
      ban->expires = duration == -1 ? -1 : (current_time + duration);

      if( !inlist )
         banlist.push_back( ban );
      save_banlist();

      // Boot off any players matching the IP or range that was just banned
      list < char_data * >::iterator ich;
      for( ich = pclist.begin(  ); ich != pclist.end(  ); ++ich )
      {
         char_data *vch = *ich;

         if( vch->desc && is_banned( vch->desc ) )
         {
            send_ban_notice( vch->desc, ban );
            vch->desc->disconnect = true;
         }
      }

      if( inlist )
         ch->printf( "&wBan for %s has been updated.\r\n", arg1.c_str() );
      else
         ch->printf( "&w%s has been added to the ban list.\r\n", arg1.c_str() );
      return;
   }
}
