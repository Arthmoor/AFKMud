/****************************************************************************
 *                   ^     +----- |  / ^     ^ |     | +-\                  *
 *                  / \    |      | /  |\   /| |     | |  \                 *
 *                 /   \   +---   |<   | \ / | |     | |  |                 *
 *                /-----\  |      | \  |  v  | |     | |  /                 *
 *               /       \ |      |  \ |     | +-----+ +-/                  *
 ****************************************************************************
 * AFKMud Copyright 1997-2026 by Roger Libiez (Samson),                     *
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

#pragma once

constexpr int MAX_CHANHISTORY = 20;
inline constexpr std::string_view CHANNEL_FILE = "../system/channels.dat";

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

   std::string name;    // Name of the person who sent the message.
   std::string format;  // Formatting of how the channel is displayed.
   std::chrono::system_clock::time_point timestamp; // Time when the message was sent.
   int level = 0;       // Level of the person who sent the message.
   int invis = 0;       // Whether or not the person who sent the message was invisible at the time.
};

class mud_channel
{
 private:
   mud_channel( const mud_channel & m );
     mud_channel & operator=( const mud_channel & );

 public:
     mud_channel(  );
    ~mud_channel(  );

   std::bitset<CHAN_MAXFLAG> flags;    // Flags for this channel.
   std::string name;                   // The channel's name.
   std::string colorname;              // Custom color tag the channel uses.
   std::list<chan_history *> history;  // List of recent messages on the channel.
   int level = LEVEL_IMMORTAL;         // Minimum level allowed to use the channel.
   int type = CHAN_GLOBAL;             // The type of channel.
};

mud_channel *find_channel( std::string_view );
