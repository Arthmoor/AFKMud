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
 * Original SMAUG 1.4a written by Thoric (Derek Snider) with Altrag,        *
 * Blodkai, Haus, Narn, Scryn, Swordbearer, Tricops, Gorog, Rennard,        *
 * Grishnakh, Fireblade, and Nivek.                                         *
 *                                                                          *
 * Original MERC 2.1 code by Hatchet, Furey, and Kahn.                      *
 *                                                                          *
 * Original DikuMUD code by: Hans Staerfeldt, Katja Nyboe, Tom Madsen,      *
 * Michael Seifert, and Sebastian Hammer.                                   *
 ****************************************************************************
 *                             Event Processing                             *
 *      This code is derived from the IMC2 0.10 event processing code.      *
 *      Originally written by Oliver Jowett. Licensed under the LGPL.       *
 *               See the document COPYING.LGPL for details.                 *
 ****************************************************************************/

#ifndef __EVENT_H__
#define __EVENT_H__

struct event_info
{
   void ( *callback ) ( void *data );
   void *data;
   time_t when;
};

void add_event( time_t, void ( *callback ) ( void * ), void * );
void cancel_event( void ( *callback ) ( void * ), void * );
event_info *find_event( void ( *callback ) ( void * ), void * );
void ev_violence( void * );
void ev_area_reset( void * );
void ev_auction( void * );
void ev_reboot_count( void * );
void ev_skyship( void * );
void ev_pfile_check( void * );
void ev_board_check( void * );
void ev_dns_check( void * );
void ev_webwho_refresh( void * );
#if !defined(__CYGWIN__) && defined(SQL)
void ev_mysql_ping( void * );
#endif
#endif
