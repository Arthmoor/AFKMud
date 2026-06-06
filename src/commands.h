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
   std::string fun_name;  /* Added to hold the func name and dump some functions totally - Trax */
   void *fileHandle;
   DO_FUN *do_fun;
   std::bitset<MAX_CMD_FLAG> flags; /* Added for Checking interpret stuff -Shaddai */
   short position;
   short level;
   short log;
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

   std::string name;
   std::string char_no_arg;
   std::string others_no_arg;
   std::string char_found;
   std::string others_found;
   std::string vict_found;
   std::string char_auto;
   std::string others_auto;
   std::string obj_self;
   std::string obj_others;
   short minposition;
};

/*
 * Cmd flag names --Shaddai
 */
extern const char *cmd_flags[];
extern std::vector<std::vector<cmd_type *>> command_table;
extern std::map<std::string, social_type *> social_table;

cmd_type *find_command( const std::string & );
social_type *find_social( const std::string & );
int get_cmdflag( const std::string & );
