/****************************************************************************
 *                   ^     +----- |  / ^     ^ |     | +-\                  *
 *                  / \    |      | /  |\   /| |     | |  \                 *
 *                 /   \   +---   |<   | \ / | |     | |  |                 *
 *                /-----\  |      | \  |  v  | |     | |  /                 *
 *               /       \ |      |  \ |     | +-----+ +-/                  *
 ****************************************************************************
 * AFKMud Copyright 1997-2020 by Roger Libiez (Samson),                     *
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

#ifndef __SHOPS_H__
#define __SHOPS_H__

#define SHOP_DIR "../shops/"  /* Clan/PC shopkeepers - Samson 7-16-00 */

/*
 * Shop types.
 */
const int MAX_TRADE = 5;

struct shop_data
{
   int keeper; /* Vnum of shop keeper mob */
   short buy_type[MAX_TRADE]; /* Item types shop will buy */
   short profit_buy; /* Cost multiplier for buying */
   short profit_sell;   /* Cost multiplier for selling */
   short open_hour;  /* First opening hour */
   short close_hour; /* First closing hour */
};

const int MAX_FIX = 3;
const int SHOP_FIX = 1;
const int SHOP_RECHARGE = 2;

struct repair_data
{
   int keeper; /* Vnum of shop keeper mob */
   short fix_type[MAX_FIX];   /* Item types shop will fix */
   short profit_fix; /* Cost multiplier for fixing */
   short shop_type;  /* Repair shop type */
   short open_hour;  /* First opening hour */
   short close_hour; /* First closing hour */
};

extern list < shop_data * >shoplist;
extern list < repair_data * >repairlist;
#endif
