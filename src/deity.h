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
 *                           Deity handling module                          *
 ****************************************************************************/

#pragma once

inline constexpr std::string_view DEITY_LIST = "deity.lst"; // List of deities.

class deity_data
{
 private:
   deity_data( const deity_data & d );
     deity_data & operator=( const deity_data & );

 public:
     deity_data(  );
    ~deity_data(  );

   std::string name;                      // Name of the deity.
   std::string filename;                  // Filename for this deity on disk.
   std::string deitydesc;                 // Detailed description of the deity.
   std::bitset<MAX_RACE> race_allowed;    // Races which cannot worship this deity - Samson 5-17-04
   std::bitset<MAX_CLASS> class_allowed;  // Classes which cannot worship this deity - Samson 5-17-04
   int element[3]{0};                     // Favor levels at which resistance boons are given. Elements 1 and 2 added by Tarl 2/24/02 - Consolidated by Samson 12/19/04
   int suscept[3]{0};                     // Favor levels at which susceptibility penalties are given. Elements 1 and 2 added by Tarl 2/24/02 - Consolidated by Samson 12/19/04
   int affected[3]{0};                    // Favor levels at which effects are applied to the player. 1 and 2 added by Tarl 2/24/02 - Consolidated by Samson 12/19/04
   int npcrace[3]{0};                     // Favor is given for killing NPCs of these races - Consolidated by Samson 12/19/04
   int npcfoe[3]{0};                      // Favor is given for killing NPCs of these races too - Consolidated by Samson 12/19/04
   int elementnum[3]{0};                  // The resistance boons that are given. Elementnum 1 and 2 added by Tarl 2/24/02 - Consolidated by Samson 12/19/04
   int susceptnum[3]{0};                  // The susceptibility penalties that are applied. Susceptnum 1 and 2 added by Tarl 2/24/02 - Consolidated by Samson 12/19/04
   int affectednum[3]{0};                 // The effects that are applied to the player. Affectednum 1 and 2 added by Tarl 2/24/02 - Consolidated by Samson 12/19/04
   int sex = 0;                           // Deity will not accept worshippers of this sex.
   int objstat = 0;                       // An attribute bonus applied by a supplicated object.
   int recallroom = 0;                    // Room Vnum where the player will appear when supplicating for a recall - Samson 4-13-98
   int avatar = 0;                        // Mob Vnum to summon when supplicating for an avatar to help the player in combat - Restored by Samson 8-1-98
   int mount = 0;                         // Mob Vnum to summon when supplicating for a mount - Added by Tarl 24 Feb 02
   int minion = 0;                        // Mob Vnum to summon when supplicating for a pet/minion - Added by Tarl 24 Feb 02
   int deityobj = 0;                      // Object Vnum to create when supplicating for an object - Restored by Samson 8-1-98 */
   int deityobj2 = 0;                     // Second object Vnum to create when supplicating for an object.
   short alignment = 0;                   // The deity's alignment. (Good/Neutral/Evil). This value plays a roll in favor adjustments.
   short worshippers = 0;                 // Number of worshippers the deity has in the playerbase.
   short scorpse = 0;                     // Amount of favor needed to supplicate for corpse retrieval.
   short sdeityobj = 0;                   // Amount of favor needed to supplicate for the deity's first tier object.
   short sdeityobj2 = 0;                  // Amount of favor needed to supplicate for the deity's second tier object.
   short savatar = 0;                     // Amount of favor needed to supplicate for an avatar.
   short srecall = 0;                     // Amount of favor needed to supplicate for a recall.
   short smount = 0;                      // Amount of favor needed to supplicate for a mount - Added by Tarl 24 Feb 02
   short sminion = 0;                     // Amount of favor needed to supplicate for a minion - Added by Tarl 24 Feb 02
   short spell[3]{0};                     // Spells added to the player at each favor level - Added by Tarl 24 Mar 02 - Consolidated by Samson 12/19/04
   short sspell[3]{0};                    // Favor levels needed to acquire the deity's spells - Added by Tarl 24 Mar 02 - Consolidated by Samson 12/19/04
   short flee = 0;                        // Base favor adjustment for fleeing from NPCs.
   short flee_npcrace[3]{0};              // Specific larger favor adjustments for fleeing from specific NPC races - Consolidated by Samson 12/19/04
   short flee_npcfoe[3]{0};               // Specific larger favor adjustments for feeling from specific NPC races designated as enemies - Consolidated by Samson 12/19/04
   short kill = 0;                        // Base favor adjustment for killing another player in combat.
   short kill_magic = 0;                  // Favor adjustment for killing an NPC with offensive magic.
   short kill_npcrace[3]{0};              // Specific larger favor adjustment for killing specific NPC races - Consolidated by Samson 12/19/04
   short kill_npcfoe[3]{0};               // Specific larger favor adjustment for killing specific NPC races designated as enemies - Consolidated by Samson 12/19/04
   short die = 0;                         // Favor adjustment for being killed in combat with an NPC.
   short die_npcrace[3]{0};               // Favor adjustment for a player being killed by an NPC of their same race - Consolidated by Samson 12/19/04
   short die_npcfoe[3]{0};                // Favor adjustment for a player being killed by an enemy NPC of their same race - Consolidated by Samson 12/19/04
   short sac = 0;                         // Favor adjustment for sacrificing a corpse.
   short bury_corpse = 0;                 // Favor adjustment for burying a corpse.
   short aid_spell = 0;                   // Favor adjustment for casting defensive spells on another player.
   short aid = 0;                         // Favor adjustment for successfully rescuing another player in combat, or using the "Aid" skill on another player.
   short backstab = 0;                    // Favor adjustment for successfully executing a backstab or circle attack.
   short steal = 0;                       // Favor adjustment for successfully picking someone's pocket, or successfully picking a lock.
   short spell_aid = 0;                   // Appears to be the same function as aid_spell ?
   short dig_corpse = 0;                  // Favor adjustment for digging up a buried corpse.
};

extern std::list<deity_data *> deitylist;
void save_deity( deity_data * );
deity_data *get_deity( std::string_view );
bool IS_DEVOTED( char_data * );
