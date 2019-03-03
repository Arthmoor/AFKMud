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
 *                         Liquidtable Headerfile                           *
 *                     by Noplex (noplex@crimsonblade.org)                  *
 *                   Redistributed in AFKMud with permission                *
 ****************************************************************************
 *                              Version History                             *
 ****************************************************************************
 *  (v1.0) - Liquidtable converted into linked list, original 15 Smaug liqs *
 *           now read from a .dat file in /system                           *
 *  (v1.5) - OLC support added to create, edit, and delete liquids while    *
 *           the game is still running, automatic edit.                     *
 *  (v2.0) - Mixture support code added. Liquids can now be mixed with      *
 *           other liquids to form a result.                                *
 *  (v2.2) - Liquid statistics command added (liquids) shows all information*
 *           about the given liquid.                                        *
 *  (v2.3) - OLC addition for mixtures.                                     *
 *  (v2.4) - Mixtures are now saved into a seperate file and one linked list*
 *           because of some saving and loading issues. All the code has    *
 *           been modified to accept the new format. "liq_can_mix" function *
 *           introduced. "mix" command introduced to mix liquids.           *
 *  (v2.5) - Thanks to Samson for some polishing and bugfixing, we now have *
 *           a (hopefully) fully funcitonal copy =).                        *
 *  (v2.6) - "Fill" and "Empty" functions have been fixed to allow for the  *
 *           new liquidsystem.                                              *
 *  (v2.7) - Forgot to fix blood support... fixed.                          *
 *         - IS_VAMPIRE ifcheck placed in do_drink                          * 
 *  	   - Blood fix for blood on the ground.                               *
 *         - do_look/do_exam fix from Sirek.                                *
 *  (v2.8) - Ability to mix objects into liquids.                           *
 *	     (original code/concept -Sirek)                                      *
 ****************************************************************************/

/*
 * File: liquids.h
 * Name: Liquidtable Module (3.0b)
 * Author: John 'Noplex' Bellone (jbellone@comcast.net)
 * Terms:
 * If this file is to be re-disributed; you must send an email
 * to the author. All headers above the #include calls must be
 * kept intact. All license requirements must be met. License
 * can be found in the included license.txt document or on the
 * website.
 * Description:
 * This module is a rewrite of the original module which allowed for
 * a SMAUG mud to have a fully online editable liquidtable; adding liquids;
 * removing them; and editing them online. It allows an near-endless supply
 * of liquids for builder's to work with.
 * A second addition to this module allowed for builder's to create mixtures;
 * when two liquids were mixed together they would produce a different liquid.
 * Yet another adaptation to the above concept allowed for objects to be mixed
 * with liquids to produce a liquid.
 * This newest version offers a cleaner running code; smaller; and faster in
 * all ways around. Hopefully it'll knock out the old one ten fold ;)
 * Also in the upcoming 'testing' phase of this code; new additions will be added
 * including a better alchemey system for creating poitions as immortals; and as
 * mortals.
 */

#ifndef __LIQUIDS_H__
#define __LIQUIDS_H__

/* hard-coded max liquids */
const int MAX_LIQUIDS = 100;

/* globals */
extern int top_liquid;

enum liquid_struct_types
{
   LIQTYPE_NORMAL, LIQTYPE_ALCOHOL, LIQTYPE_POISON, LIQTYPE_BLOOD, LIQTYPE_TOP
};

class liquid_data
{
 private:
   liquid_data( const liquid_data & l );
     liquid_data & operator=( const liquid_data & );

 public:
     liquid_data(  );
    ~liquid_data(  );

   string name;
   string shortdesc;
   string color;
   int mod[MAX_CONDS];
   int vnum;
   int type;
};

class mixture_data
{
 private:
   mixture_data( const mixture_data & m );
     mixture_data & operator=( const mixture_data & );

 public:
     mixture_data(  );
    ~mixture_data(  );

   string name;
   int data[3];
   bool object;
};

extern liquid_data *liquid_table[MAX_LIQUIDS];

liquid_data *get_liq_vnum( int );
liquid_data *get_liq( const string & );

#endif
