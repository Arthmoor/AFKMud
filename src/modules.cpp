/****************************************************************************
 *                   ^     +----- |  / ^     ^ |     | +-\                  *
 *                  / \    |      | /  |\   /| |     | |  \                 *
 *                 /   \   +---   |<   | \ / | |     | |  |                 *
 *                /-----\  |      | \  |  v  | |     | |  /                 *
 *               /       \ |      |  \ |     | +-----+ +-/                  *
 ****************************************************************************
 * AFKMud Copyright 1997-2009 by Roger Libiez (Samson),                     *
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
 *                     Dynamically loaded module support                    *
 ****************************************************************************/

#include <dirent.h>
#if !defined(WIN32)
#include <dlfcn.h>   /* Required for libdl - Trax */
#else
#include <windows.h>
#define dlopen( libname, flags ) LoadLibrary( (libname) )
#endif
#include "mud.h"

map < string, void *>module;

bool registered_func( const char *func )
{
   return false;
}

void register_function( const char *name )
{
}

// Call up each module in the map and execute its initializer.
void init_modules(  )
{
   map < string, void *>::iterator mod;
   const char *error;

   while( mod != module.end(  ) )
   {
      typedef void INIT(  );
      INIT *mod_init;

      mod_init = ( INIT * ) dlsym( mod->second, "module_init" );
      if( ( error = dlerror(  ) ) )
      {
         log_printf( "Module entry failure: %s", error );
         continue;
      }

      // If this works, it should have fired off something.
      mod_init(  );
   }
}

void load_modules(  )
{
   DIR *dp;
   struct dirent *dentry;
   char directory_name[100];

   return;  // castrated for now.

   mudstrlcpy( directory_name, "../src/modules/so", 100 );
   dp = opendir( directory_name );
   dentry = readdir( dp );
   while( dentry )
   {
      /*
       * Added by Tarl 3 Dec 02 because we are now using CVS 
       */
      if( !str_cmp( dentry->d_name, "CVS" ) )
      {
         dentry = readdir( dp );
         continue;
      }
      if( dentry->d_name[0] != '.' )
      {
         string filename = dentry->d_name;
         string name;
         string::size_type ps;
         char path[100];

         if( ( ps = filename.find( '.' ) ) == string::npos )
            name = filename;
         else
            name = filename.substr( 0, ps );

         snprintf( path, 100, "%s/%s", directory_name, name.c_str(  ) );
         module[name] = dlopen( path, RTLD_NOW );
         if( !module[name] )
         {
            log_string( dlerror(  ) );
            continue;
         }
      }
      dentry = readdir( dp );
   }
   closedir( dp );

   init_modules(  );
}
