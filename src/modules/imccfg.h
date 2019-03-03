/****************************************************************************
 *                   ^     +----- |  / ^     ^ |     | +-\                  *
 *                  / \    |      | /  |\   /| |     | |  \                 *
 *                 /   \   +---   |<   | \ / | |     | |  |                 *
 *                /-----\  |      | \  |  v  | |     | |  /                 *
 *               /       \ |      |  \ |     | +-----+ +-/                  *
 ****************************************************************************
 * AFKMud Copyright 1997-2019 by Roger Libiez (Samson),                     *
 * Levi Beckerson (Whir), Michael Ward (Tarl), Erik Wolfe (Dwip),           *
 * Cameron Carroll (Cam), Cyberfox, Karangi, Rathian, Raine,                *
 * Xorith, and Adjani.                                                      *
 * All Rights Reserved.                                                     *
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
 *                           IMC2 Freedom Client                            *
 ****************************************************************************/

/* Codebase Macros - AFKMud
 *
 * This should cover the following derivatives:
 *
 * AFKMud: Versions 2.0 and up.
 *
 * Other derivatives should work too, please submit any needed changes to imc@intermud.us and be sure
 * you mention it's for AFKMud so that the email can be properly forwarded to me. -- Samson
 */

#ifndef __IMC2CFG_H__
#define __IMC2CFG_H__

#define CH_IMCDATA(ch)        ((ch)->pcdata->imcchardata)
#define CH_IMCLEVEL(ch)       ((ch)->level)
#define CH_IMCNAME(ch)        ((ch)->name)
#define CH_IMCTITLE(ch)       ((ch)->pcdata->title)
#define CH_IMCRANK(ch)        ((ch)->pcdata->rank)
#define CH_IMCSEX(ch)         ((ch)->sex)

#endif
