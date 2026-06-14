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

#pragma once

constexpr int MAX_VAR_BITS = 128;

enum variable_types
{
   vtNONE, vtINT, vtXBIT, vtSTR
};

/*
 * Variable structure used for putting variable tags on players, mobs
 * or anything else.  Will be persistent (save) for players.
 */
struct variable_data
{
   variable_data( );
   variable_data( int, int, std::string_view );
    ~variable_data(  );

   std::string tag;                                 // Variable name
   std::string varstring;                           // String data
   std::bitset<MAX_VAR_BITS> varflags;
   std::chrono::system_clock::time_point c_time;    // Time created
   std::chrono::system_clock::time_point m_time;    // Time last modified
   std::chrono::system_clock::time_point r_time;    // Time last read
   std::chrono::system_clock::time_point expires;   // Expiry date
   long vardata = 0;                                // long int value
   int type = 0;                                    // Variable type (string = 1, long int = 2, bits = 3)
   int vnum = 0;                                    // Vnum of mob that set this
   int timer = 0;                                   // Expiry timer
};

variable_data *get_tag( char_data *, std::string_view, int );
