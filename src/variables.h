/****************************************************************************
 *                   ^     +----- |  / ^     ^ |     | +-\                  *
 *                  / \    |      | /  |\   /| |     | |  \                 *
 *                 /   \   +---   |<   | \ / | |     | |  |                 *
 *                /-----\  |      | \  |  v  | |     | |  /                 *
 *               /       \ |      |  \ |     | +-----+ +-/                  *
 ****************************************************************************
 * AFKMud Copyright 1997-2010 by Roger Libiez (Samson),                     *
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
 *                         MUDprog Quest Variables                          *
 ****************************************************************************/

typedef enum
{
   vtNONE, vtINT, vtSTR //, vtXBIT <--- FIXME: Can't very well use this right now. We don't have EXT_BV anymore.
} variable_types;

/*
 * Variable structure used for putting variable tags on players, mobs
 * or anything else.  Will be persistant (save) for players.
 */
struct variable_data
{
   variable_data(  );
   variable_data( int, int, const string & );
    ~variable_data(  );

   string tag; // Variable name
   void *data; // Data pointer
   char type;  // Type of data
   time_t c_time; // Time created
   time_t m_time; // Time last modified
   time_t r_time; // Time last read
   time_t expires;   // Expiry date
   // int flags;      // Flags for future use
   int vnum;   // Vnum of mob that set this
   int timer;  // Expiry timer
};

variable_data *get_tag( char_data *, const string &, int );
