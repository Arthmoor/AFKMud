/****************************************************************************
 *                   ^     +----- |  / ^     ^ |     | +-\                  *
 *                  / \    |      | /  |\   /| |     | |  \                 *
 *                 /   \   +---   |<   | \ / | |     | |  |                 *
 *                /-----\  |      | \  |  v  | |     | |  /                 *
 *               /       \ |      |  \ |     | +-----+ +-/                  *
 ****************************************************************************
 * AFKMud Copyright 1997-2019 by Roger Libiez (Samson),                     *
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
 *                          Dynamic Channel System                          *
 ****************************************************************************/

#ifndef __CHANNELS_H__
#define __CHANNELS_H__

const int MAX_CHANHISTORY = 20;
#define CHANNEL_FILE SYSTEM_DIR "channels.dat"

enum channel_types
{
   CHAN_GLOBAL, CHAN_ZONE, CHAN_GUILD, CHAN_ROOM, CHAN_PK, CHAN_LOG
};

enum channel_flags
{
   CHAN_KEEPHISTORY, CHAN_INTERPORT, CHAN_ALWAYSON, CHAN_MAXFLAG
};
/* Interport flag will only operate when multiport code is active */

class chan_history
{
 private:
   chan_history( const chan_history & m );
     chan_history & operator=( const chan_history & );

 public:
     chan_history(  );
    ~chan_history(  );

   string name;
   char *format;
   time_t timestamp;
   int level;
   int invis;
};

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
   list < chan_history * > history;
   int level;
   int type;
};

mud_channel *find_channel( const string & );

#endif
