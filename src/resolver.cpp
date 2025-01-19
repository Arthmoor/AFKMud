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
#if defined(__OpenBSD__) || defined(__FreeBSD__) || defined(__CYGWIN__)
#include <sys/types.h>
#include <sys/socket.h>
#endif
#include <netinet/in.h>
#include <netdb.h>

char *resolve_address( int address )
{
   static char addr_str[256];
   struct hostent *from;

   if( ( from = gethostbyaddr( ( char * )&address, sizeof( address ), AF_INET ) ) != nullptr )
   {
      strlcpy( addr_str, strcmp( from->h_name, "localhost" ) ? from->h_name : "local-host", 256 );
   }
   else
   {
      int addr = ntohl( address );
      snprintf( addr_str, 256, "%d.%d.%d.%d", ( addr >> 24 ) & 0xFF, ( addr >> 16 ) & 0xFF, ( addr >> 8 ) & 0xFF, ( addr ) & 0xFF );
   }
   return addr_str;
}

int main( int argc, char *argv[] )
{
   int ip;
   char *address;

   if( argc != 2 )
   {
      printf( "unknown.host\r\n" );
      exit( 0 );
   }

   ip = atoi( argv[1] );

   address = resolve_address( ip );

   printf( "%s\r\n", address );
   exit( 0 );
}
