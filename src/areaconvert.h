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
 *                       Smaug Zone Conversion Support                      *
 ****************************************************************************/

#pragma once

constexpr int AREA_STOCK_VERSION = 3;       // The current top version for stock Smaug 1.8b files
constexpr int AREA_SMAUGWIZ_VERSION = 1000; // The current top version for SmaugWiz files
constexpr int AREA_FUSS_VERSION = 1;        // The current top version for SmaugFUSS files

/* Extended bitvector material is now kept only for legacy purposes to convert old areas. */
typedef struct extended_bitvector EXT_BV;

/*
 * Defines for extended bitvectors
 */
constexpr int int_bits = 32; // Can't use INT_MAX here because of XBI. EXT_BV is built around being 32 bits anyway so here we are.

constexpr int XBM = 31;  /* extended bitmask   ( INTBITS - 1 )  */
constexpr int RSV = 5;   /* right-shift value  ( sqrt(XBM+1) )  */
constexpr int XBI = 4;   /* integers in an extended bitvector   */
constexpr int MAX_BITS = XBI * int_bits;

/*
 * Structure for extended bitvectors -- Thoric
 */
struct extended_bitvector
{
   unsigned int bits[XBI]; /* Needs to be unsigned to compile in Redhat 6 - Samson */
};

EXT_BV fread_bitvector( FILE * );
char *ext_flag_string( EXT_BV *, char *const flagarray[] );

extern int top_affect;
extern int top_exit;
extern int top_reset;
extern int top_shop;
extern int top_repair;
extern FILE *fpArea;

template <typename T>
bool xIS_SET( const T& var, int bit )
{
   return ( var.bits[bit >> RSV] & ( 1 << ( bit & XBM ) ) ) != 0;
}
