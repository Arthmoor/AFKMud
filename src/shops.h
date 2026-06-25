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
 *                       Shop and repair shop module                        *
 ****************************************************************************/

#pragma once

inline constexpr std::string_view SHOP_DIR = "../shops/";  /* Clan/PC shopkeepers - Samson 7-16-00 */

/*
 * Shop types.
 */
constexpr int MAX_TRADE = 5;

struct shop_data
{
   int keeper = 0;               // Vnum of shop keeper mob.
   short buy_type[MAX_TRADE]{0}; // Item types shop will buy.
   short profit_buy = 120;       // Cost multiplier for buying.
   short profit_sell = 90;       // Cost multiplier for selling.
   short open_hour = 0;          // First opening hour.
   short close_hour = 0;         // First closing hour.
};

constexpr int MAX_FIX = 3;
constexpr int SHOP_FIX = 1;
constexpr int SHOP_RECHARGE = 2;

struct repair_data
{
   int keeper = 0;                // Vnum of shop keeper mob.
   short fix_type[MAX_FIX]{0};    // Item types shop will fix.
   short profit_fix = 100;        // Cost multiplier for fixing.
   short shop_type = SHOP_FIX;    // Repair shop type.
   short open_hour = 0;           // First opening hour.
   short close_hour = 0;          // First closing hour.
};

extern std::list<shop_data *> shoplist;
extern std::list<repair_data *> repairlist;
