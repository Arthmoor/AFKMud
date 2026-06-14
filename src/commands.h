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
 *                             Command Header                               *
 ****************************************************************************/

#pragma once

/*
 *  Defines for the command flags. --Shaddai
 */
enum command_flags
{
   CMD_POSSESS, CMD_POLYMORPHED, CMD_ACTION, CMD_NOSPAM, CMD_GHOST,
   CMD_MUDPROG, CMD_NOFORCE, CMD_LOADED, CMD_NOABORT, MAX_CMD_FLAG
};

/*
 * Structure for a command in the command lookup table.
 */
class cmd_type
{
 private:
   cmd_type( const cmd_type & c );
     cmd_type & operator=( const cmd_type & );

 public:
     cmd_type(  );
    ~cmd_type(  );

   std::string name;
   std::string fun_name;            // Added to hold the func name and dump some functions totally - Trax
   std::bitset<MAX_CMD_FLAG> flags; // Added for Checking interpret stuff -Shaddai
   void *fileHandle = nullptr;      // File handle for a module file loaded from disk. This feature has not been fully implemented yet. [DEPRECATED - Will likely be removed at some point]
   DO_FUN *do_fun = nullptr;        // What function this command points to in the code.
   short position = POS_STANDING;   // Minimum position the player must be in to use this command.
   short level = 0;                 // Level the person needs to be in order to use this command.
   short log = LOG_NORMAL;          // What log level the command uses for the logs.
};

/*
 * Structure for a social in the socials table.
 */
class social_type
{
 private:
   social_type( const social_type & s );
     social_type & operator=( const social_type & );

 public:
     social_type(  );
    ~social_type(  );

   std::string name;                   // Name of the social.
   std::string char_no_arg;            // What the player sees if no target is specified.
   std::string others_no_arg;          // What other people in the room see if no target is specified.
   std::string char_found;             // What the player sees when a target is specified.
   std::string others_found;           // What other people in the room see when a target is specified.
   std::string vict_found;             // What the person being targeted sees.
   std::string char_auto;              // What the player sees when targeting themselves.
   std::string others_auto;            // What other people in the room see if the player targets themselves.
   std::string obj_self;               // What the player sees if they target an object.
   std::string obj_others;             // What other people in the room see if the player targets an object.
   short minposition = POS_RESTING;    // Minimum position the social can be used in. Will almost always be from the resting state.
};

/*
 * Cmd flag names --Shaddai
 */
extern const char *cmd_flags[];
extern std::vector<std::vector<cmd_type *>> command_table;
extern std::map<std::string, social_type *> social_table;

cmd_type *find_command( std::string_view );
social_type *find_social( std::string_view );
int get_cmdflag( std::string_view );
