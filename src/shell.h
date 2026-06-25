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
 *                  Internal server shell command module                    *
 ****************************************************************************/

#pragma once

inline constexpr std::string_view SHELL_COMMAND_FILE = "../system/shellcommands.dat";

/* Change this line to the home directory for the server account that runs the MUD - Samson */
inline constexpr std::string_view HOST_DIR = "/home/afkmud/";

/* Change this line to the name of your compiled binary - Samson */
inline constexpr std::string_view BINARYFILE = "afkmud";

/* Change each of these to reflect your directory structure - Samson */

inline constexpr std::string_view CODEZONEDIR = "/home/afkmud/afkmud/area/";     /* Used in do_copyzone - Samson 8-22-98 */
inline constexpr std::string_view BUILDZONEDIR = "/home/afkmud/dist2/area/";     /* Used in do_copyzone - Samson 4-7-98 */
inline constexpr std::string_view MAINZONEDIR = "/home/afkmud/dist/area/";       /* Used in do_copyzone - Samson 4-7-98 */
inline constexpr std::string_view TESTCODEDIR = "/home/afkmud/afkmud/src/";      /* Used in do_copycode - Samson 4-7-98 */
inline constexpr std::string_view BUILDCODEDIR = "/home/afkmud/dist2/src/";      /* Used in do_copycode - Samson 8-22-98 */
inline constexpr std::string_view MAINCODEDIR = "/home/afkmud/dist/src/";        /* Used in do_copycode - Samson 4-7-98 */
inline constexpr std::string_view CODESYSTEMDIR = "/home/afkmud/afkmud/system/"; /* Used in do_copysocial - Samson 5-2-98 */
inline constexpr std::string_view BUILDSYSTEMDIR = "/home/afkmud/dist2/system/"; /* Used in do_copysocial - Samson 5-2-98 */
inline constexpr std::string_view MAINSYSTEMDIR = "/home/afkmud/dist/system/";   /* Used in do_copysocial - Samson 5-2-98 */
inline constexpr std::string_view CODECLASSDIR = "/home/afkmud/afkmud/classes/"; /* Used in do_copyclass - Samson 9-17-98 */
inline constexpr std::string_view BUILDCLASSDIR = "/home/afkmud/dist2/classes/"; /* Used in do_copyclass - Samson 9-17-98 */
inline constexpr std::string_view MAINCLASSDIR = "/home/afkmud/dist/classes/";   /* Used in do_copyclass - Samson 9-17-98 */
inline constexpr std::string_view CODERACEDIR = "/home/afkmud/afkmud/races/";    /* Used in do_copyrace - Samson 10-13-98 */
inline constexpr std::string_view BUILDRACEDIR = "/home/afkmud/dist2/races/";    /* Used in do_copyrace - Samson 10-13-98 */
inline constexpr std::string_view MAINRACEDIR = "/home/afkmud/dist/races/";      /* Used in do_copyrace - Samson 10-13-98 */
inline constexpr std::string_view CODEDEITYDIR = "/home/afkmud/afkmud/deity/";   /* Used in do_copydeity - Samson 10-13-98 */
inline constexpr std::string_view BUILDDEITYDIR = "/home/afkmud/dist2/deity/";   /* Used in do_copydeity - Samson 10-13-98 */
inline constexpr std::string_view MAINDEITYDIR = "/home/afkmud/dist/deity/";     /* Used in do_copydeity - Samson 10-13-98 */
inline constexpr std::string_view MAINMAPDIR = "/home/afkmud/dist/maps/";        /* Used in do_copymap - Samson 8-2-99 */
inline constexpr std::string_view BUILDMAPDIR = "/home/afkmud/dist2/maps/";      /* Used in do_copymap - Samson 8-2-99 */
inline constexpr std::string_view CODEMAPDIR = "/home/afkmud/afkmud/maps/";      /* Used in do_copymap - Samson 8-2-99 */

class shell_cmd
{
 private:
   shell_cmd( const shell_cmd & s );
     shell_cmd & operator=( const shell_cmd & );

 public:
     shell_cmd(  );
    ~shell_cmd(  );

   std::bitset<MAX_CMD_FLAG> flags; /* Added for Checking interpret stuff -Shaddai */

   void set_func( DO_FUN * fun )
   {
      _do_fun = fun;
   }
   DO_FUN *get_func(  )
   {
      return _do_fun;
   }

   void set_name( std::string_view name )
   {
      _name = name;
   }
   std::string get_name(  )
   {
      return _name;
   }
   const char *get_cname(  )
   {
      return _name.c_str(  );
   }

   void set_func_name( std::string_view name )
   {
      _func_name = name;
   }
   std::string get_func_name(  )
   {
      if( _func_name.empty(  ) )
         return "";
      return _func_name;
   }
   const char *get_func_cname(  )
   {
      if( _func_name.empty(  ) )
         return "";
      return _func_name.c_str(  );
   }

   void set_position( std::string_view pos )
   {
      short newpos = get_npc_position( pos );
      if( newpos < 0 || newpos > POS_MAX )
         newpos = POS_STANDING;
      _position = newpos;
   }
   short get_position(  )
   {
      return _position;
   }

   void set_level( short lvl )
   {
      lvl = urange( 1, lvl, LEVEL_SUPREME );
      _level = lvl;
   }
   short get_level(  )
   {
      return _level;
   }

   void set_log( std::string_view log )
   {
      short newlog = get_logflag( log );
      if( newlog < 0 || newlog > LOG_ALL )
         newlog = 0;
      _log = newlog;
   }
   short get_log(  )
   {
      return _log;
   }

 private:
   std::string _name;
   std::string _func_name; // Added to hold the func name and dump some functions totally - Trax
   DO_FUN *_do_fun = nullptr;
   short _position = 0;
   short _level = 0;
   short _log = 0;
};

extern std::list<shell_cmd *> shellcmdlist;
shell_cmd *find_shellcommand( std::string_view );
