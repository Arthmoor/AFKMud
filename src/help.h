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
 *                           Help System Module                             *
 ****************************************************************************/

#pragma once

inline constexpr std::string_view HELP_FILE = "../system/helps.dat";  // Data file where helps are stored now.

/*
 * Help table types.
 */
class help_data
{
private:
   help_data( const help_data & m );
   help_data & operator=( const help_data & );

public:
   help_data(  );
   ~help_data(  );

   std::string keyword;    // Keywords used to look up the help entry.
   std::string related;    // Other entries that are related to this one.
   std::string text;       // Detailed text of the help entry.
   short level = 0;        // Level at which the entry first becomes visible to players.
   bool webinvis = false;  // Whether or not the entry is visible to the web. Only useful with a MyQSL database.
};
