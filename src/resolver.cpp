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
 *                      External DNS Resolver Module                        *
 ****************************************************************************/

/***************************************************************************
 *                          SMC version 0.9.7b3                            *
 *          Additions to Rom2.3 (C) 1995, 1996 by Tom Adriaenssen          *
 *                                                                         *
 * Share and enjoy! But please give the original authors some credit.      *
 *                                                                         *
 * Ideas, tips, or comments can be send to:                                *
 *          tadriaen@zorro.ruca.ua.ac.be                                   *
 *          shadow@www.dma.be                                              *
 ***************************************************************************/

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#if defined(__OpenBSD__) || defined(__FreeBSD__) || defined(__CYGWIN__)
#include <sys/types.h>
#include <sys/socket.h>
#endif
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>

using namespace std;

char *resolve_address( const string & address )
{
   struct in6_addr addr6;
   struct in_addr addr4;
   static char addr_str[256];
   struct hostent *from;

   // ::1 is localhost, so if they're not connecting from that, check both cases to be sure they'll work.
   if( address != "::1" )
   {
      if( inet_pton( AF_INET6, address.c_str(), &addr6 ) == 0 )
      {
         if( inet_aton( address.c_str(), &addr4 ) == 0 )
         {
            // In theory this should only happen if the connection doesn't even show a valid IP.
            printf( "somehow.has.no.ip?\r\n" );
            exit( 0 );
         }
      }
   }

   // Skip all of the below if IPv6 is localhost. That doesn't resolve to a useful result for whatever reason.
   if( address == "::1" )
      strlcpy( addr_str, "localhost", 256 );
   else
   {
      if( ( from = gethostbyaddr( &addr6, sizeof( addr6 ), AF_INET6 ) ) != nullptr )
      {
         strlcpy( addr_str, strcmp( from->h_name, "localhost" ) ? from->h_name : "localhost", 256 );
      }
      else
      {
         string::size_type pos = address.find_last_of( ":", address.length() );

         if( pos != string::npos )
         {
            strlcpy( addr_str, address.c_str(), 256 );
         }
         else
         {
            inet_aton( address.c_str(), &addr4 );

            if( ( from = gethostbyaddr( &addr4, sizeof( addr4 ), AF_INET ) ) )
               strlcpy( addr_str, strcmp( from->h_name, "localhost" ) ? from->h_name : "localhost", 256 );
            else // If the above fails, just copy the original IP address. We don't care why. They may not have a reverse lookup.
            {
               strlcpy( addr_str, address.c_str(), 256 );
            }
         }
      }
   }

   return addr_str;
}

int main( int argc, char *argv[] )
{
   int ip;
   string input = argv[1]; // Ordinarily this unsafe to just accept, but the only way this gets called is through the MUD anyway.
   char *address;

   if( argc != 2 )
   {
      // No idea what could cause this, but the SMC guys thought it was necessary, so return an error.
      printf( "bad.resolver.call\r\n" );
      exit( 0 );
   }

   address = resolve_address( input );

   // If we've made it this far, then either the resolution succeeded or the IP address is being passed through. So let's pass this along now.
   printf( "%s\r\n", address );
   exit( 0 );
}
