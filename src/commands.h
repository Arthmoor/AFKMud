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
   CMD_MUDPROG, CMD_NOFORCE, CMD_LOADED, MAX_CMD_FLAG
};

/*
 * Structure for a command in the command lookup table.
 */
class cmd_type
{
 private:
   cmd_type( const cmd_type& c );
   cmd_type& operator=( const cmd_type& );

 public:
   cmd_type();
   ~cmd_type();

   cmd_type *next;
   void *fileHandle;
   DO_FUN *do_fun;
     bitset < MAX_CMD_FLAG > flags; /* Added for Checking interpret stuff -Shaddai */
   char *name;
   char *fun_name;   /* Added to hold the func name and dump some functions totally - Trax */
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
   social_type( const social_type& s );
   social_type& operator=( const social_type& );

 public:
   social_type();
   ~social_type();

   social_type *next;
   char *name;
   char *char_no_arg;
   char *others_no_arg;
   char *char_found;
   char *others_found;
   char *vict_found;
   char *char_auto;
   char *others_auto;
   char *obj_self;
   char *obj_others;
};

/*
 * Cmd flag names --Shaddai
 */
extern char *const cmd_flags[];
extern cmd_type *command_hash[126];
extern social_type *social_index[27];

extern char *const cmd_flags[];

cmd_type *find_command( string );
social_type *find_social( string );
int get_cmdflag( char * );

#endif
