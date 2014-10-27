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
 *                           IP Banning module                              *
 ****************************************************************************/

#include "mud.h"
#include "ban.h"
#include "descriptor.h"

/* Global Variables */
list < ban_data * >banlist;

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
}

void save_banlist( void )
{
}

// FIXME: This whole thing needs a sweeping rewrite anyway - no time like right now buddy! CIDR handling is a must.
