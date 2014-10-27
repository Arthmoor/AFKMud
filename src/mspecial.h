/****************************************************************************
 *                   ^     +----- |  / ^     ^ |     | +-\                  *
 *                  / \    |      | /  |\   /| |     | |  \                 *
 *                 /   \   +---   |<   | \ / | |     | |  |                 *
 *                /-----\  |      | \  |  v  | |     | |  /                 *
 *               /       \ |      |  \ |     | +-----+ +-/                  *
 ****************************************************************************
 * AFKMud Copyright 1997-2014 by Roger Libiez (Samson),                     *
 * Levi Beckerson (Whir), Michael Ward (Tarl), Erik Wolfe (Dwip),           *
 * Cameron Carroll (Cam), Cyberfox, Karangi, Rathian, Raine,                *
 * Xorith, and Adjani.                                                      *
 * All Rights Reserved.                                                     *
 *                                                                          *
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
 *                   "Special procedure" module for Mobs                    *
 ****************************************************************************/

/******************************************************
            Desolation of the Dragon MUD II
      (C) 1997, 1998  Jesse DeFer and Heath Leach
 http://dotd.mudservices.com  dotd@dotd.mudservices.com 
 ******************************************************/

#ifndef __MSPECIALS_H__
#define __MSPECIALS_H__

extern list < string > speclist;

/* Any spec_fun added here needs to be added to specfuns.dat as well.
 * If you don't know what that means, ask Samson to take care of it.
 */

/* Generic Mobs */
SPECF( spec_janitor );  /* Scavenges trash */
SPECF( spec_snake ); /* Poisons people with its bite */
SPECF( spec_poison );   /* For area conversion compatibility - DON'T REMOVE THIS */
SPECF( spec_fido );  /* Eats corpses */
SPECF( spec_cast_adept );  /* For healer mobs */
SPECF( spec_RustMonster ); /* Eats anything on the ground */
SPECF( spec_wanderer );

/* Generic Cityguards */
SPECF( spec_GenericCityguard );
SPECF( spec_guard );

/* Generic Citizens */
SPECF( spec_GenericCitizen );

/* Class Procs */
SPECF( spec_warrior );  /* Warriors */
SPECF( spec_thief ); /* Rogues */
SPECF( spec_cast_mage );   /* Mages */
SPECF( spec_cast_cleric ); /* Clerics */
SPECF( spec_cast_undead ); /* Necromancers */
SPECF( spec_ranger );   /* Rangers */
SPECF( spec_paladin );  /* Paladins */
SPECF( spec_druid ); /* Druids */
SPECF( spec_antipaladin ); /* Antipaladins */
SPECF( spec_bard );  /* Bards */

/* Dragon stuff */
SPECF( spec_breath_any );
SPECF( spec_breath_acid );
SPECF( spec_breath_fire );
SPECF( spec_breath_frost );
SPECF( spec_breath_gas );
SPECF( spec_breath_lightning );
#endif
