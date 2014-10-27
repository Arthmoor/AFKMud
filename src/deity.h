/****************************************************************************
 *                   ^     +----- |  / ^     ^ |     | +-\                  *
 *                  / \    |      | /  |\   /| |     | |  \                 *
 *                 /   \   +---   |<   | \ / | |     | |  |                 *
 *                /-----\  |      | \  |  v  | |     | |  /                 *
 *               /       \ |      |  \ |     | +-----+ +-/                  *
 ****************************************************************************
 * AFKMud Copyright 1997-2007 by Roger Libiez (Samson),                     *
 * Levi Beckerson (Whir), Michael Ward (Tarl), Erik Wolfe (Dwip),           *
 * Cameron Carroll (Cam), Cyberfox, Karangi, Rathian, Raine,                *
 * Xorith, and Adjani.                                                      *
 * All Rights Reserved.                                                     *
 * Registered with the United States Copyright Office: TX 5-877-286         *
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
 *                           Deity handling module                          *
 ****************************************************************************/

#ifndef __DEITY_H__
#define __DEITY_H__

#define DEITY_LIST "deity.lst"   /* List of deities     */

#define IS_DEVOTED(ch) ( !(ch)->isnpc() && (ch)->pcdata->deity )

class deity_data
{
 private:
   deity_data( const deity_data& d );
   deity_data& operator=( const deity_data& );

 public:
   deity_data();
   ~deity_data();

   bitset<MAX_RACE> race_allowed; /* Samson 5-17-04 */
   bitset<MAX_CLASS> class_allowed;  /* Samson 5-17-04 */
   char *filename;
   char *name;
   char *deitydesc;
   int element[3];  /* Elements 1 and 2 added by Tarl 2/24/02 - Consolidated by Samson 12/19/04 */
   int suscept[3];  /* Suscept 1 and 2 added by Tarl 2/24/02 - Consolidated by Samson 12/19/04 */
   int affected[3]; /* Affects 1 and 2 added by Tarl 2/24/02 - Consolidated by Samson 12/19/04 */
   int npcrace[3];  /* Consolidated by Samson 12/19/04 */
   int npcfoe[3];   /* Consolidated by Samson 12/19/04 */
   int susceptnum[3]; /* Susceptnum 1 and 2 added by Tarl 2/24/02 - Consolidated by Samson 12/19/04 */
   int elementnum[3]; /* Elementnum 1 and 2 added by Tarl 2/24/02 - Consolidated by Samson 12/19/04 */
   int affectednum[3]; /* Affectednum 1 and 2 added by Tarl 2/24/02 - Consolidated by Samson 12/19/04 */
   int sex;
   int objstat;
   int recallroom;   /* Samson 4-13-98 */
   int avatar; /* Restored by Samson 8-1-98 */
   int mount;  /* Added by Tarl 24 Feb 02 */
   int minion; /* Added by Tarl 24 Feb 02 */
   int deityobj;  /* Restored by Samson 8-1-98 */
   int deityobj2;
   short alignment;
   short worshippers;
   short scorpse;
   short sdeityobj;
   short sdeityobj2;
   short savatar;
   short srecall;
   short smount;  /* Added by Tarl 24 Feb 02 */
   short sminion; /* Added by Tarl 24 Feb 02 */
   short spell[3];  /* Added by Tarl 24 Mar 02 - Consolidated by Samson 12/19/04 */
   short sspell[3]; /* Added by Tarl 24 Mar 02 - Consolidated by Samson 12/19/04 */
   short flee;
   short flee_npcrace[3]; /* Consolidated by Samson 12/19/04 */
   short flee_npcfoe[3];  /* Consolidated by Samson 12/19/04 */
   short kill;
   short kill_magic;
   short kill_npcrace[3]; /* Consolidated by Samson 12/19/04 */
   short kill_npcfoe[3];  /* Consolidated by Samson 12/19/04 */
   short die;
   short die_npcrace[3]; /* Consolidated by Samson 12/19/04 */
   short die_npcfoe[3];  /* Consolidated by Samson 12/19/04 */
   short sac;
   short bury_corpse;
   short aid_spell;
   short aid;
   short backstab;
   short steal;
   short spell_aid;
   short dig_corpse;
};

extern list<deity_data*> deitylist;
void save_deity( deity_data * );
deity_data *get_deity( char * );
#endif
