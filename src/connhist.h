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
 *                       Xorith's Connection History                        *
 ****************************************************************************/

#ifndef __CONNHIST_H__
#define __CONNHIST_H__

/* ConnHistory Feature (header)
 *
 * Based loosely on Samson's Channel History functions. (basic idea)
 * Written by: Xorith 5/7/03, last updated: 9/20/03
 *
 * Stores connection data in an array so that it can be reviewed later.
 *
 */

/* Max number of connections to keep in the history.
 * Don't set this too high... */
const size_t MAX_CONNHISTORY = 30;

/* Change this for your codebase! Currently set for AFKMud */
const int CH_LVL_ADMIN = LEVEL_ADMIN;

/* Path to the conn.hst file */
/* default is: ../system/conn.hst */
#define CH_FILE SYSTEM_DIR "conn.hst"

/* ConnType's for Connection History
 * Be sure to add new types into the update_connhistory function! */
enum conn_hist_types
{
   CONNTYPE_LOGIN, CONNTYPE_QUIT, CONNTYPE_IDLE, CONNTYPE_LINKDEAD, CONNTYPE_NEWPLYR, CONNTYPE_RECONN
};

/* conn history checking error codes */
enum conn_hist_errors
{
   CHK_CONN_OK, CHK_CONN_REMOVED
};

class conn_data
{
 private:
   conn_data( const conn_data & c );
     conn_data & operator=( const conn_data & );

 public:
     conn_data(  );
    ~conn_data(  );

   string user;
   string host;
   string when;
   int type;
   int level;
   int invis_lvl;
};
#endif
