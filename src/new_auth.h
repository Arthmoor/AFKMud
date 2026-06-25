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
 *                      New Name Authorization module                       *
 ****************************************************************************/

#pragma once

inline constexpr std::string_view AUTH_FILE = "../system/auth.dat";
inline constexpr std::string_view RESERVED_LIST = "../system/reserved.lst"; // List of reserved names
inline constexpr std::string_view NAMEGEN_FILE = "../system/namegen.txt";   // Used for the name generator

/* New auth stuff --Rantic */
enum auth_types
{
   AUTH_ONLINE, AUTH_OFFLINE, AUTH_LINK_DEAD, AUTH_CHANGE_NAME, AUTH_unused, AUTH_AUTHED
};

int get_auth_state( char_data * ch );

class auth_data
{
 private:
   auth_data( const auth_data & a );
     auth_data & operator=( const auth_data & );

 public:
     auth_data(  );
    ~auth_data(  );

   std::string name;      // Name of character awaiting authorization
   std::string authed_by; // Name of immortal who authorized the name
   std::string change_by; // Name of immortal requesting name change
   short state = 0;       // Current state of authed
};

bool NOT_AUTHED( char_data * );
