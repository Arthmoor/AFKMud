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
 *                         MUDprog Quest Variables                          *
 ****************************************************************************/

const int MAX_VAR_BITS = 128;

enum variable_types
{
   vtNONE, vtINT, vtXBIT, vtSTR
};

/*
 * Variable structure used for putting variable tags on players, mobs
 * or anything else.  Will be persistant (save) for players.
 */
struct variable_data
{
   variable_data( );
   variable_data( int, int, const string& );
    ~variable_data(  );

   string tag; // Variable name
   string varstring; // String data
   bitset < MAX_VAR_BITS > varflags;
   long vardata;     // long int value
   time_t c_time;    // Time created
   time_t m_time;    // Time last modified
   time_t r_time;    // Time last read
   time_t expires;   // Expiry date
   int type;         // Variable type (string = 1, long int = 2, bits = 3)
   int vnum;         // Vnum of mob that set this
   int timer;        // Expiry timer
};

variable_data *get_tag( char_data *, const string &, int );
