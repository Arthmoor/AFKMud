/****************************************************************************
 *                   ^     +----- |  / ^     ^ |     | +-\                  *
 *                  / \    |      | /  |\   /| |     | |  \                 *
 *                 /   \   +---   |<   | \ / | |     | |  |                 *
 *                /-----\  |      | \  |  v  | |     | |  /                 *
 *               /       \ |      |  \ |     | +-----+ +-/                  *
 ****************************************************************************
 * AFKMud Copyright 1997-2008 by Roger Libiez (Samson),                     *
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
 *                  Internal server shell command module                    *
 ****************************************************************************/

#ifndef __SHELL_H__
#define __SHELL_H__

#define SHELL_COMMAND_FILE "../system/shellcommands.dat"

/* Change this line to the home directory for the server - Samson */
#define HOST_DIR 	"/home/alsherok/"

/* Change this line to the name of your compiled binary - Samson */
#define BINARYFILE "afkmud"

/* Change each of these to reflect your directory structure - Samson */

#define CODEZONEDIR	HOST_DIR "Alsherok/area/"  /* Used in do_copyzone - Samson 8-22-98 */
#define BUILDZONEDIR	HOST_DIR "dist2/area/"  /* Used in do_copyzone - Samson 4-7-98 */
#define MAINZONEDIR	HOST_DIR "dist/area/"   /* Used in do_copyzone - Samson 4-7-98 */
#define TESTCODEDIR     HOST_DIR "Alsherok/src/"   /* Used in do_copycode - Samson 4-7-98 */
#define BUILDCODEDIR    HOST_DIR "dist2/src/"   /* Used in do_copycode - Samson 8-22-98 */
#define MAINCODEDIR	HOST_DIR "dist/src/" /* Used in do_copycode - Samson 4-7-98 */
#define CODESYSTEMDIR   HOST_DIR "Alsherok/system/"   /* Used in do_copysocial - Samson 5-2-98 */
#define BUILDSYSTEMDIR  HOST_DIR "dist2/system/"   /* Used in do_copysocial - Samson 5-2-98 */
#define MAINSYSTEMDIR   HOST_DIR "dist/system/" /* Used in do_copysocial - Samson 5-2-98 */
#define CODECLASSDIR	HOST_DIR "Alsherok/classes/"  /* Used in do_copyclass - Samson 9-17-98 */
#define BUILDCLASSDIR	HOST_DIR "dist2/classes/"  /* Used in do_copyclass - Samson 9-17-98 */
#define MAINCLASSDIR	HOST_DIR "dist/classes/"   /* Used in do_copyclass - Samson 9-17-98 */
#define CODERACEDIR	HOST_DIR "Alsherok/races/" /* Used in do_copyrace - Samson 10-13-98 */
#define BUILDRACEDIR	HOST_DIR "dist2/races/" /* Used in do_copyrace - Samson 10-13-98 */
#define MAINRACEDIR	HOST_DIR "dist/races/"  /* Used in do_copyrace - Samson 10-13-98 */
#define CODEDEITYDIR	HOST_DIR "Alsherok/deity/" /* Used in do_copydeity - Samson 10-13-98 */
#define BUILDDEITYDIR	HOST_DIR "dist2/deity/" /* Used in do_copydeity - Samson 10-13-98 */
#define MAINDEITYDIR	HOST_DIR "dist/deity/"  /* Used in do_copydeity - Samson 10-13-98 */
#define MAINMAPDIR	HOST_DIR "dist/maps/"   /* Used in do_copymap - Samson 8-2-99 */
#define BUILDMAPDIR	HOST_DIR "dist2/maps/"  /* Used in do_copymap - Samson 8-2-99 */
#define CODEMAPDIR	HOST_DIR "Alsherok/maps/"  /* Used in do_copymap - Samson 8-2-99 */

class shell_cmd
{
 private:
   shell_cmd( const shell_cmd& s );
   shell_cmd& operator=( const shell_cmd& );

 public:
   shell_cmd( );
   ~shell_cmd( );

     bitset < MAX_CMD_FLAG > flags; /* Added for Checking interpret stuff -Shaddai */
   DO_FUN *do_fun;
   char *name;
   char *fun_name;   /* Added to hold the func name and dump some functions totally - Trax */
   short position;
   short level;
   short log;
};

extern list<shell_cmd*> shellcmdlist;
shell_cmd *find_shellcommand( char * );
#endif
