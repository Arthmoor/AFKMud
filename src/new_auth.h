/****************************************************************************
 *                   ^     +----- |  / ^     ^ |     | +-\                  *
 *                  / \    |      | /  |\   /| |     | |  \                 *
 *                 /   \   +---   |<   | \ / | |     | |  |                 *
 *                /-----\  |      | \  |  v  | |     | |  /                 *
 *               /       \ |      |  \ |     | +-----+ +-/                  *
 ****************************************************************************
 * AFKMud Copyright 1997-2007 by Roger Libiez (Samson),                     *
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
 *                      New Name Authorization module                       *
 ****************************************************************************/

#ifndef __NEWAUTH_H__
#define __NEWAUTH_H__

#define AUTH_FILE SYSTEM_DIR "auth.dat"
#define RESERVED_LIST SYSTEM_DIR "reserved.lst" /* List of reserved names  */

/* New auth stuff --Rantic */
enum auth_types
{
   AUTH_ONLINE, AUTH_OFFLINE, AUTH_LINK_DEAD, AUTH_CHANGE_NAME, AUTH_unused, AUTH_AUTHED
};

int get_auth_state( char_data * ch );

/* new auth -- Rantic */
#define NOT_AUTHED(ch) ( get_auth_state((ch)) != AUTH_AUTHED && (ch)->has_pcflag( PCFLAG_UNAUTHED ) )
#define IS_WAITING_FOR_AUTH(ch) ( (ch)->desc && get_auth_state((ch)) == AUTH_ONLINE && (ch)->has_pcflag( PCFLAG_UNAUTHED ) )

class auth_data
{
 private:
   auth_data( const auth_data& a );
   auth_data& operator=( const auth_data& );

 public:
   auth_data();
   ~auth_data();

   char *name; /* Name of character awaiting authorization */
   char *authed_by;  /* Name of immortal who authorized the name */
   char *change_by;  /* Name of immortal requesting name change */
   short state;   /* Current state of authed */
};
#endif
