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
 *                             Command Header                               *
 ****************************************************************************/

#ifndef __COMMANDS_H__
#define __COMMANDS_H__

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

   string name;
   string fun_name;  /* Added to hold the func name and dump some functions totally - Trax */
   void *fileHandle;
   DO_FUN *do_fun;
     bitset < MAX_CMD_FLAG > flags; /* Added for Checking interpret stuff -Shaddai */
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

   string name;
   string char_no_arg;
   string others_no_arg;
   string char_found;
   string others_found;
   string vict_found;
   string char_auto;
   string others_auto;
   string obj_self;
   string obj_others;
   short minposition;
};

/*
 * Cmd flag names --Shaddai
 */
extern const char *cmd_flags[];
extern vector < vector < cmd_type * > >command_table;
extern map < string, social_type * >social_table;

cmd_type *find_command( const string & );
social_type *find_social( const string & );
int get_cmdflag( const string & );

#endif
