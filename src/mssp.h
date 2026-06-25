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
 *                          MSSP Plaintext Module                           *
 ****************************************************************************/

/******************************************************************
* Program writen by:                                              *
*  Greg (Keberus Maou'San) Mosley                                 *
*  Co-Owner/Coder SW: TGA                                         *
*  www.t-n-k-games.com                                            *
*                                                                 *
* Description:                                                    *
*  This program will allow admin to view and set their MSSP       *
*  variables in game, and allows their game to respond to a MSSP  *
*  Server with the MSSP-Plaintext protocol                        *
*******************************************************************
* What it does:                                                   *
*  Allows admin to set/view MSSP variables and transmits the MSSP *
*  information to anyone who does an MSSP-REQUEST at the login    *
*  screen                                                         *
*******************************************************************
* Special Thanks:                                                 *
*  A special thanks to Scandum for coming up with the MSSP        *
*  protocol, Cratylus for the MSSP-Plaintext idea, and Elanthis   *
*  for the GNUC_FORMAT idea ( which I like to use now ).          *
******************************************************************/

/* TERMS OF USE
         I only really have 2 terms...
 1. Give credit where it is due, keep the above header in your code 
    (you don't have to give me credit in mud) and if someone asks 
	don't lie and say you did it.
 2. If you have any comments or questions feel free to email me
    at keberus@gmail.com

  Thats All....
 */

#pragma once

// More details on what these fields expect are available at https://tintin.mudhalla.net/protocols/mssp/
struct msspinfo
{
   msspinfo();

   std::string hostname;         // DNS address of the MUD.
   std::string ip;               // IPv4 address of the MUD.
   std::string ipv6;             // IPv6 address of the MUD.
   std::string contact;          // Contact email for someone at the MUD.
   std::string icon;             // URL of a square image to represent the MUD. Must be a square image equal to or larger than 64x64, and no bigger than 256KB.
   std::string language;         // What language the MUD is presented in.
   std::string location;         // Two or three letter country code, such as "US", "GB", "DE", etc.
   std::string family;           // Codebase family the MUD belongs to. Usually means the major branch like Diku or LP.
   std::string genre;            // The genre of the MUD. ie: Fantasy, Sci-Fi, etc.
   std::string subgenre;         // The subtype of the genre, such as "Medieval Fantasy".
   std::string gamePlay;         // The general gameplay style of the game. Such as "Hack & Slash" or "Roleplaying".
   std::string gameSystem;       // The general ruleset used by the MUD, such as "D&D".
   std::string intermud;         // Type of InterMUD chat system the MUD has. Should be sent empty if it has none.
   std::string status;           // Current status of the game.
   unsigned short ssl = 0;       // Port number for encrypted connections. If zero, will not be sent. Must be >= 1024 to be valid.
   short crawldelay = -1;        // How often the MSSP bot will crawl the MUD, in hours.
   short created = 0;            // The year the MUD was created.
   short minAge = 0;             // Minimum age required to play the MUD. If zero, will not be sent.
   bool ansi = true;             // Does the MUD support ANSI color?
   bool mccp = true;             // Does the MUD support MUD Client Compression Protocol?
   bool mcp = false;             // Does the MUD support MCP? (No, it doesn't, because the codebase has no support for it)
   bool msp = true;              // Does the MUD support the MUD Sound Protocol?
   bool mxp = false;             // Does the MUD support MXP? (No, it doesn't, because the codebase has no support for it)
   bool vt100 = false;           // Does the MUD support VT100 codes? (No, it doesn't, because the codebase has no support for it)
   bool xterm256 = false;        // Does the MUD support XTerm 256 color? (Technically yes, but nothing is actually leveraging it yet)
   bool pay2play = false;        // Do players have to pay in order to play the game?
   bool pay4perks = false;       // Can players pay to receive extra perks?
   bool hiringBuilders = false;  // Is the MUD hiring new builders?
   bool hiringCoders = false;    // Is the MUD hiring new coders?
};    

inline constexpr std::string_view MSSP_FILE = "../system/mssp.dat";

constexpr int MSSP_MINAGE = 0;
constexpr int MSSP_MAXAGE = 21;

constexpr int MSSP_MINCREATED = 2001; // Set to 2001 because AFKMud was not publicly available before that.
constexpr int MSSP_MAXCREATED = 2100;

constexpr int MSSP_MAXVAL = 20000;
constexpr int MAX_MSSP_VAR1 = 4;
constexpr int MAX_MSSP_VAR2 = 3;
