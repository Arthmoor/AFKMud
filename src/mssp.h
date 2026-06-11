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

struct msspinfo
{
   msspinfo();

   std::string hostname;
   std::string ip;
   std::string contact;
   std::string icon;
   std::string language;
   std::string location;
   std::string family;
   std::string genre;
   std::string gamePlay;
   std::string gameSystem;
   std::string intermud;
   std::string status;
   std::string subgenre;
   std::string equipmentSystem;
   std::string multiplaying;
   std::string playerKilling;
   std::string questSystem;
   std::string roleplaying;
   std::string trainingSystem;
   std::string worldOriginality;
   short created;
   short minAge;
   short worlds;
   bool ansi;
   bool mccp;
   bool mcp;
   bool msp;
   bool ssl;
   bool mxp;
   bool pueblo;
   bool vt100;
   bool xterm256;
   bool pay2play;
   bool pay4perks;
   bool hiringBuilders;
   bool hiringCoders;
   bool adultMaterial;
   bool multiclassing;
   bool newbieFriendly;
   bool playerCities;
   bool playerClans;
   bool playerCrafting;
   bool playerGuilds;
};    

inline constexpr std::string_view MSSP_FILE = "../system/mssp.dat";

constexpr int MSSP_MINAGE = 0;
constexpr int MSSP_MAXAGE = 21;

constexpr int MSSP_MINCREATED = 2001;
constexpr int MSSP_MAXCREATED = 2100;

constexpr int MSSP_MAXVAL = 20000;
constexpr int MAX_MSSP_VAR1 = 4;
constexpr int MAX_MSSP_VAR2 = 3;
