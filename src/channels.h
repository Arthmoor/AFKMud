/****************************************************************************
 *                   ^     +----- |  / ^     ^ |     | +-\                  *
 *                  / \    |      | /  |\   /| |     | |  \                 *
 *                 /   \   +---   |<   | \ / | |     | |  |                 *
 *                /-----\  |      | \  |  v  | |     | |  /                 *
 *               /       \ |      |  \ |     | +-----+ +-/                  *
 ****************************************************************************
 * AFKMud Copyright 1997-2012 by Roger Libiez (Samson),                     *
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
 *                          Dynamic Channel System                          *
 ****************************************************************************/

#ifndef __CHANNELS_H__
#define __CHANNELS_H__

const int MAX_CHANHISTORY = 20;
#define CHANNEL_FILE SYSTEM_DIR "channels.dat"

enum channel_types
{
   CHAN_GLOBAL, CHAN_ZONE, CHAN_GUILD, CHAN_unused, CHAN_PK, CHAN_LOG
};

enum channel_flags
{
   CHAN_KEEPHISTORY, CHAN_INTERPORT, CHAN_MAXFLAG
};
/* Interport flag will only operate when multiport code is active */

class mud_channel
{
 private:
   mud_channel( const mud_channel & m );
     mud_channel & operator=( const mud_channel & );

 public:
     mud_channel(  );
    ~mud_channel(  );

     bitset < CHAN_MAXFLAG > flags;
   string name;
   string colorname;
   char *history[MAX_CHANHISTORY][2];  /* Not saved */
   time_t htime[MAX_CHANHISTORY];   /* Not saved *//* Xorith */
   int hlevel[MAX_CHANHISTORY];  /* Not saved */
   int hinvis[MAX_CHANHISTORY];  /* Not saved */
   int level;
   int type;
};
#endif

mud_channel *find_channel( const string & );
