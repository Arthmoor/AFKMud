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
 *                            Ban module by Shaddai                         *
 ****************************************************************************/

#ifndef __BAN_H__
#define __BAN_H__

#define BAN_LIST "ban.lst" /* List of bans                 */

 /*
  * Ban Types --- Shaddai
  */
const int BAN_WARN = -1;

/*
 * Site ban structure.
 */
struct ban_data
{
 private:
   ban_data( const ban_data& b );
   ban_data& operator=( const ban_data& );

 public:
   ban_data();
   ~ban_data();

   char *name; /* Name of site/class/race banned */
   char *user; /* Name of user from site */
   char *note; /* Why it was banned */
   char *ban_by;  /* Who banned this site */
   char *ban_time;   /* Time it was banned */
   int unban_date;   /* When ban expires */
   short duration;   /* How long it is banned for */
   short level;   /* Level that is banned */
   bool warn;  /* Echo on warn channel */
   bool prefix;   /* Use of *site */
   bool suffix;   /* Use of site* */
};

#endif
