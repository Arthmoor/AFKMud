/****************************************************************************
 *                   ^     +----- |  / ^     ^ |     | +-\                  *
 *                  / \    |      | /  |\   /| |     | |  \                 *
 *                 /   \   +---   |<   | \ / | |     | |  |                 *
 *                /-----\  |      | \  |  v  | |     | |  /                 *
 *               /       \ |      |  \ |     | +-----+ +-/                  *
 ****************************************************************************
 * AFKMud Copyright 1997-2015 by Roger Libiez (Samson),                     *
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

/* Comment out this define if the child processes throw segfaults */
#define USEGLOB   /* Samson 4-16-98 - For new shell command */

#include <sys/wait.h>   /* Samson 4-16-98 - For new shell command */
#include <fcntl.h>

#ifdef USEGLOB /* Samson 4-16-98 - For new command pipe */
#include <glob.h>
#endif
#include <fstream>
#include <sstream>
#include "mud.h"
#include "clans.h"
#include "commands.h"
#include "descriptor.h"
#include "shell.h"

void unlink_command( cmd_type * );

/* Global variables - Samson */
bool compilelock = false;  /* Reboot/shutdown commands locked during compiles */
list < shell_cmd * >shellcmdlist;

extern char lastplayercmd[MIL * 2];
extern bool bootlock;

#ifndef USEGLOB
/* OLD command shell provided by Ferris - ferris@FootPrints.net Installed by Samson 4-6-98
 * For safety reasons, this is only available if the USEGLOB define is commented out.
 */
/*
 * Local functions.
 */
FILE *popen( const char *, const char * );
int pclose( FILE * );

char *fgetf( char *s, int n, register FILE * iop )
{
   register int c;
   register char *cs;

   c = '\0';
   cs = s;
   while( --n > 0 && ( c = getc( iop ) ) != EOF )
      if( ( *cs++ = c ) == '\0' )
         break;
   *cs = '\0';
   return ( ( c == EOF && cs == s ) ? NULL : s );
}

/* NOT recommended to be used as a conventional command! */
void command_pipe( char_data * ch, const char *argument )
{
   char buf[MSL];
   FILE *fp;

   if( ch->desc->is_compressing )
      ch->desc->compressEnd(  );
   fp = popen( argument, "r" );
   fgetf( buf, MSL, fp );
   pclose( fp );
   ch->pagerf( "&R%s\r\n", buf );
}

/* End OLD shell command code */
#endif

/* New command shell code by Thoric - Installed by Samson 4-16-98 */
CMDF( do_mudexec )
{
   char term[15], col[15], lines[15];  // Thanks. Thanks a whole fucking lot GNU!
   int desc, flags;
   pid_t pid;

   if( !ch->desc )
      return;

   desc = ch->desc->descriptor;

   ch->set_color( AT_PLAIN );

   if( ch->desc->is_compressing )
      ch->desc->compressEnd(  );

   if( ( pid = fork(  ) ) == 0 )
   {
      // This is evil. Don't ever let me see you do this again without a VERY GOOD REASON.
      char pl[MIL];
      char *p;
      char *arg = pl;

      mudstrlcpy( arg, argument.c_str(  ), MIL );
#ifdef USEGLOB
      glob_t g;
#else
      char **argv;
      int argc = 0;
#endif
#ifdef DEBUGGLOB
      int argc = 0;
#endif

      flags = fcntl( desc, F_GETFL, 0 );
      fcntl( desc, F_SETFL, O_NONBLOCK | flags );
      dup2( desc, STDIN_FILENO );
      dup2( desc, STDOUT_FILENO );
      dup2( desc, STDERR_FILENO );
      mudstrlcpy( term, "TERM=vt100", 15 );
      putenv( term );

      mudstrlcpy( col, "COLUMNS=80", 15 );
      putenv( col );

      mudstrlcpy( lines, "LINES=24", 15 );
      putenv( lines );

#ifdef USEGLOB
      g.gl_offs = 1;
      strtok( arg, " " );

      if( ( p = strtok( NULL, " " ) ) != NULL )
         glob( p, GLOB_DOOFFS | GLOB_NOCHECK, NULL, &g );

      if( !g.gl_pathv[g.gl_pathc - 1] )
         g.gl_pathv[g.gl_pathc - 1] = p;

      while( ( p = strtok( NULL, " " ) ) != NULL )
      {
         glob( p, GLOB_DOOFFS | GLOB_NOCHECK | GLOB_APPEND, NULL, &g );
         if( !g.gl_pathv[g.gl_pathc - 1] )
            g.gl_pathv[g.gl_pathc - 1] = p;
      }
      g.gl_pathv[0] = arg;

#ifdef DEBUGGLOB
      for( argc = 0; argc < g.gl_pathc; ++argc )
         printf( "arg %d: %s\r\n", argc, g.gl_pathv[argc] );
      fflush( stdout );
#endif

      execvp( g.gl_pathv[0], g.gl_pathv );
#else
      while( *p )
      {
         while( isspace( *p ) )
            ++p;
         if( *p == '\0' )
            break;
         ++argc;
         while( !isspace( *p ) && *p )
            ++p;
      }
      p = argument.c_str(  );
      argv = calloc( argc + 1, sizeof( char * ) );

      argc = 0;
      argv[argc] = strtok( argument.c_str(  ), " " );
      while( ( argv[++argc] = strtok( NULL, " " ) ) != NULL );

      execvp( argv[0], argv );
#endif

      fprintf( stderr, "Shell process: %s failed!\n", argument.c_str(  ) );
      perror( "mudexec" );
      exit( 0 );
   }
   else if( pid < 2 )
   {
      ch->print( "Process fork failed.\r\n" );
      fprintf( stderr, "%s", "Shell process: fork failed!\n" );
      return;
   }
   else
   {
      ch->desc->process = pid;
      ch->desc->connected = CON_FORKED;
   }
}

/* End NEW shell command code */

/* This function verifies filenames during copy operations - Samson 4-7-98 */
bool copy_file( char_data * ch, const char *filename )
{
   FILE *fp;

   if( !( fp = fopen( filename, "r" ) ) )
   {
      ch->printf( "&RThe file %s does not exist, or cannot be opened. Check your spelling.\r\n", filename );
      return false;
   }
   FCLOSE( fp );
   return true;
}

/* The guts of the compiler code, make any changes to the compiler options here - Samson 4-8-98 */
void compile_code( char_data * ch, const string & argument )
{
   if( !str_cmp( argument, "clean" ) )
   {
      funcf( ch, do_mudexec, "%s", "make -C ../src clean" );
      return;
   }

   if( !str_cmp( argument, "dns" ) )
   {
      funcf( ch, do_mudexec, "%s", "make -C ../src dns" );
      return;
   }
   funcf( ch, do_mudexec, "%s", "make -C ../src" );
}

/* This command compiles the code on the mud, works only on code port - Samson 4-8-98 */
CMDF( do_compile )
{
   if( mud_port != CODEPORT )
   {
      ch->print( "&RThe compiler can only be run on the code port.\r\n" );
      return;
   }

   if( bootlock )
   {
      ch->print( "&RThe reboot timer is running, the compiler cannot be used at this time.\r\n" );
      return;
   }

   if( compilelock )
   {
      ch->print( "&RThe compiler is in use, please wait for the compilation to finish.\r\n" );
      return;
   }

   compilelock = true;
   echo_all_printf( ECHOTAR_IMM, "&RCompiler operation initiated by %s. Reboot and shutdown commands are locked.", ch->name );
   compile_code( ch, argument );
}

/* This command catches the shortcut "copy" - Samson 4-8-98 */
CMDF( do_copy )
{
   ch->print( "&YTo use a copy command, you have to spell it out!\r\n" );
}

/* This command copies Class files from build port to the others - Samson 9-17-98 */
CMDF( do_copyclass )
{
   char buf[MSL];
   string fname, fname2;

   if( ch->isnpc(  ) )
   {
      ch->print( "Mobs cannot use the copyclass command!\r\n" );
      return;
   }

   if( mud_port != BUILDPORT )
   {
      ch->print( "&RThe copyclass command may only be used from the Builders' port.\r\n" );
      return;
   }

   if( argument.empty(  ) )
   {
      ch->print( "You must specify a file to copy.\r\n" );
      return;
   }

   if( !str_cmp( argument, "all" ) )
   {
      fname = "*.class";
      fname2 = "skills.dat";
   }
   else if( !str_cmp( argument, "skills" ) )
      fname = "skills.dat";
   else
   {
      fname = argument;
      snprintf( buf, MSL, "%s%s", BUILDCLASSDIR, fname.c_str(  ) );
      if( !copy_file( ch, buf ) )
         return;
   }

   if( !str_cmp( argument, "all" ) )
   {
      if( !sysdata->TESTINGMODE )
      {
         ch->print( "&RClass and skill files updated to main port.\r\n" );
#ifdef USEGLOB
         funcf( ch, do_mudexec, "cp %s%s %s", BUILDCLASSDIR, fname.c_str(  ), MAINCLASSDIR );
#else
         funcf( ch, command_pipe, "cp %s%s %s", BUILDCLASSDIR, fname.c_str(  ), MAINCLASSDIR );
#endif
         funcf( ch, do_mudexec, "cp %s%s %s", BUILDSYSTEMDIR, fname2.c_str(  ), MAINSYSTEMDIR );
      }
      ch->print( "&GClass and skill files updated to code port.\r\n" );

#ifdef USEGLOB
      funcf( ch, do_mudexec, "cp %s%s %s", BUILDCLASSDIR, fname.c_str(  ), CODECLASSDIR );
#else
      funcf( ch, command_pipe, "cp %s%s %s", BUILDCLASSDIR, fname.c_str(  ), CODECLASSDIR );
#endif
      funcf( ch, do_mudexec, "cp %s%s %s", BUILDSYSTEMDIR, fname2.c_str(  ), CODESYSTEMDIR );
      return;
   }

   if( !str_cmp( argument, "skills" ) )
   {
      if( !sysdata->TESTINGMODE )
      {
         ch->print( "&RSkill file updated to main port.\r\n" );
         funcf( ch, do_mudexec, "cp %s%s %s", BUILDSYSTEMDIR, fname.c_str(  ), MAINSYSTEMDIR );
      }
      ch->print( "&GSkill file updated to code port.\r\n" );
      funcf( ch, do_mudexec, "cp %s%s %s", BUILDSYSTEMDIR, fname.c_str(  ), CODESYSTEMDIR );
      return;
   }

   if( !sysdata->TESTINGMODE )
   {
      ch->printf( "&R%s: file updated to main port.\r\n", argument.c_str(  ) );
      funcf( ch, do_mudexec, "cp %s%s %s", BUILDCLASSDIR, fname.c_str(  ), MAINCLASSDIR );
   }
   ch->printf( "&G%s: file updated to code port.\r\n", argument.c_str(  ) );
   funcf( ch, do_mudexec, "cp %s%s %s", BUILDCLASSDIR, fname.c_str(  ), CODECLASSDIR );
}

/* This command copies zones from build port to the others - Samson 4-7-98 */
CMDF( do_copyzone )
{
   string fname, fname2;

   if( ch->isnpc(  ) )
   {
      ch->print( "Mobs cannot use the copyzone command!\r\n" );
      return;
   }

   if( mud_port != BUILDPORT )
   {
      ch->print( "&RThe copyzone command may only be used from the Builders' port.\r\n" );
      return;
   }

   if( argument.empty(  ) )
   {
      ch->print( "You must specify a file to copy.\r\n" );
      return;
   }

   if( !str_cmp( argument, "all" ) )
      fname = "*.are";
   else
   {
      fname = argument;

      if( !copy_file( ch, argument.c_str(  ) ) )
         return;

      if( !str_cmp( argument, "entry.are" ) )
         fname2 = "entry.are";
      if( !str_cmp( argument, "astral.are" ) )
         fname2 = "astral.are";
      if( !str_cmp( argument, "void.are" ) )
         fname2 = "void.are";
      if( !str_cmp( argument, "immtrain.are" ) )
         fname2 = "immtrain.are";
      if( !str_cmp( argument, "one.are" ) )
         fname2 = "one.are";
   }

   if( !sysdata->TESTINGMODE )
   {
      ch->print( "&RArea file(s) updated to main port.\r\n" );
#ifdef USEGLOB
      funcf( ch, do_mudexec, "cp %s%s %s", BUILDZONEDIR, fname.c_str(  ), MAINZONEDIR );
#else
      funcf( ch, command_pipe, "cp %s%s %s", BUILDZONEDIR, fname.c_str(  ), MAINZONEDIR );
#endif
   }

   if( fname2 == "entry.are" || fname2 == "void.are" || fname2 == "astral.are" || fname2 == "one.are" || fname2 == "immtrain.are" )
   {
      ch->print( "&GArea file(s) updated to code port.\r\n" );
      funcf( ch, do_mudexec, "cp %s%s %s", BUILDZONEDIR, fname2.c_str(  ), CODEZONEDIR );
   }
}

/* This command copies maps from build port to the others - Samson 8-2-99 */
CMDF( do_copymap )
{
   char buf[MSL];
   string fname;

   if( ch->isnpc(  ) )
   {
      ch->print( "Mobs cannot use the copymap command!\r\n" );
      return;
   }

   if( mud_port != BUILDPORT )
   {
      ch->print( "&RThe copymap command may only be used from the Builders' port.\r\n" );
      return;
   }

   if( argument.empty(  ) )
   {
      ch->print( "You must specify a file to copy.\r\n" );
      return;
   }

   if( !str_cmp( argument, "all" ) )
      fname = "*.png";
   else
   {
      fname = argument;
      snprintf( buf, MSL, "%s%s", BUILDMAPDIR, fname.c_str(  ) );
      if( !copy_file( ch, buf ) )
         return;
   }

   if( !sysdata->TESTINGMODE )
   {
      ch->print( "&RMap file(s) updated to main port.\r\n" );
#ifdef USEGLOB
      funcf( ch, do_mudexec, "cp %s%s %s", BUILDMAPDIR, fname.c_str(  ), MAINMAPDIR );
#else
      funcf( ch, command_pipe, "cp %s%s %s", BUILDMAPDIR, fname.c_str(  ), MAINMAPDIR );
#endif

#ifdef USEGLOB
      funcf( ch, do_mudexec, "cp %s*.dat %s", BUILDMAPDIR, MAINMAPDIR );
#else
      funcf( ch, command_pipe, "cp %s*.dat %s", BUILDMAPDIR, MAINMAPDIR );
#endif
   }

   ch->print( "&GMap file(s) updated to code port.\r\n" );
#ifdef USEGLOB
   funcf( ch, do_mudexec, "cp %s%s %s", BUILDMAPDIR, fname.c_str(  ), CODEMAPDIR );
#else
   funcf( ch, command_pipe, "cp %s%s %s", BUILDMAPDIR, fname.c_str(  ), CODEMAPDIR );
#endif

#ifdef USEGLOB
   funcf( ch, do_mudexec, "cp %s*.dat %s", BUILDMAPDIR, CODEMAPDIR );
#else
   funcf( ch, command_pipe, "cp %s*.dat %s", BUILDMAPDIR, CODEMAPDIR );
#endif
}

CMDF( do_copyhelp )
{
   if( ch->isnpc(  ) )
   {
      ch->print( "Mobs cannot use the copyhelp command!\r\n" );
      return;
   }

   if( mud_port != BUILDPORT )
   {
      ch->print( "&RThe copyhelp command may only be used from the Builders' port.\r\n" );
      return;
   }

   if( !sysdata->TESTINGMODE )
   {
      ch->print( "&RHelp file updated to main port.\r\n" );
      funcf( ch, do_mudexec, "cp %shelps.dat %s", BUILDSYSTEMDIR, MAINSYSTEMDIR );
   }
   ch->print( "&Help file updated to code port.\r\n" );
   funcf( ch, do_mudexec, "cp %shelps.dat %s", BUILDSYSTEMDIR, CODESYSTEMDIR );
}

CMDF( do_copybits )
{
   if( ch->isnpc(  ) )
   {
      ch->print( "Mobs cannot use the copybits command!\r\n" );
      return;
   }

   if( mud_port != BUILDPORT )
   {
      ch->print( "&RThe copybits command may only be used from the Builders' port.\r\n" );
      return;
   }

   if( !sysdata->TESTINGMODE )
   {
      ch->print( "&RAbit/Qbit files updated to main port.\r\n" );
      funcf( ch, do_mudexec, "cp %sabit.lst %s", BUILDSYSTEMDIR, MAINSYSTEMDIR );
      funcf( ch, do_mudexec, "cp %sqbit.lst %s", BUILDSYSTEMDIR, MAINSYSTEMDIR );
   }
   ch->print( "&GAbit/Qbit files updated to code port.\r\n" );
   funcf( ch, do_mudexec, "cp %sabit.lst %s", BUILDSYSTEMDIR, CODESYSTEMDIR );
   funcf( ch, do_mudexec, "cp %sqbit.lst %s", BUILDSYSTEMDIR, CODESYSTEMDIR );
}

/* This command copies the social file from build port to the other ports - Samson 5-2-98 */
CMDF( do_copysocial )
{
   if( ch->isnpc(  ) )
   {
      ch->print( "Mobs cannot use the copysocial command!" );
      return;
   }

   if( mud_port != BUILDPORT )
   {
      ch->print( "&RThe copysocial command may only be used from the Builders' port.\r\n" );
      return;
   }

   if( !sysdata->TESTINGMODE )
   {
      /*
       * Build port to Main port 
       */
      ch->print( "&RSocial file updated to main port.\r\n" );
      funcf( ch, do_mudexec, "cp %ssocials.dat %s", BUILDSYSTEMDIR, MAINSYSTEMDIR );
   }

   /*
    * Build port to Code port 
    */
   ch->print( "&GSocial file updated to code port.\r\n" );
   funcf( ch, do_mudexec, "cp %ssocials.dat %s", BUILDSYSTEMDIR, CODESYSTEMDIR );
}

/* This command copies the rune file from build port to the other ports - Samson 5-2-98 */
CMDF( do_copyrunes )
{
   if( ch->isnpc(  ) )
   {
      ch->print( "Mobs cannot use the copyrunes command!" );
      return;
   }

   if( mud_port != BUILDPORT )
   {
      ch->print( "&RThe copyrunes command may only be used from the Builders' port.\r\n" );
      return;
   }

   if( !sysdata->TESTINGMODE )
   {
      /*
       * Build port to Main port 
       */
      ch->print( "&RRune file updated to main port.\r\n" );
      funcf( ch, do_mudexec, "cp %srunes.dat %s", BUILDSYSTEMDIR, MAINSYSTEMDIR );
   }

   /*
    * Build port to Code port 
    */
   ch->print( "&GRune file updated to code port.\r\n" );
   funcf( ch, do_mudexec, "cp %srunes.dat %s", BUILDSYSTEMDIR, CODESYSTEMDIR );
}

CMDF( do_copyslay )
{
   if( ch->isnpc(  ) )
   {
      ch->print( "Mobs cannot use the copyslay command!" );
      return;
   }

   if( mud_port != BUILDPORT )
   {
      ch->print( "&RThe copyslay command may only be used from the Builders' port.\r\n" );
      return;
   }

   if( !sysdata->TESTINGMODE )
   {
      /*
       * Build port to Main port 
       */
      ch->print( "&RSlay file updated to main port.\r\n" );
      funcf( ch, do_mudexec, "cp %sslay.dat %s", BUILDSYSTEMDIR, MAINSYSTEMDIR );
   }

   /*
    * Build port to Code port 
    */
   ch->print( "&GSlay file updated to code port.\r\n" );
   funcf( ch, do_mudexec, "cp %sslay.dat %s", BUILDSYSTEMDIR, CODESYSTEMDIR );
}

/* This command copies the morphs file from build port to the other ports - Samson 5-2-98 */
CMDF( do_copymorph )
{
   if( ch->isnpc(  ) )
   {
      ch->print( "Mobs cannot use the copymorph command!" );
      return;
   }

   if( mud_port != BUILDPORT )
   {
      ch->print( "&RThe copymorph command may only be used from the Builders' port.\r\n" );
      return;
   }

   if( !sysdata->TESTINGMODE )
   {
      /*
       * Build port to Main port 
       */
      ch->print( "&RPolymorph file updated to main port.\r\n" );
      funcf( ch, do_mudexec, "cp %smorph.dat %s", BUILDSYSTEMDIR, MAINSYSTEMDIR );
   }

   /*
    * Build port to Code port 
    */
   ch->print( "&GPolymorph file updated to code port.\r\n" );
   funcf( ch, do_mudexec, "cp %smorph.dat %s", BUILDSYSTEMDIR, CODESYSTEMDIR );
}

/* This command copies the mud binary file from code port to main port and build port - Samson 4-7-98 */
CMDF( do_copycode )
{
   if( ch->isnpc(  ) )
   {
      ch->print( "Mobs cannot use the copycode command!\r\n" );
      return;
   }

   if( mud_port != CODEPORT )
   {
      ch->print( "&RThe copycode command may only be used from the Code Port.\r\n" );
      return;
   }

   /*
    * Code port to Builders' port 
    */
   ch->print( "&GBinary file updated to builder port.\r\n" );
   funcf( ch, do_mudexec, "cp -f %s%s %s%s", TESTCODEDIR, BINARYFILE, BUILDCODEDIR, BINARYFILE );

   ch->print( "&GDNS Resolver file updated to builder port.\r\n" );
#if defined(__CYGWIN__)
   funcf( ch, do_mudexec, "cp -f %sresolver.exe %sresolver.exe", TESTCODEDIR, BUILDCODEDIR );
#else
   funcf( ch, do_mudexec, "cp -f %sresolver %sresolver", TESTCODEDIR, BUILDCODEDIR );
#endif

   ch->print( "&GShared Object files updated to builder port.\r\n" );
   funcf( ch, do_mudexec, "cp -f %smodules/so/*.so %smodules/so", TESTCODEDIR, BUILDCODEDIR );

   /*
    * Code port to Main port 
    */
   if( !sysdata->TESTINGMODE )
   {
      ch->print( "&RBinary file updated to main port.\r\n" );
      funcf( ch, do_mudexec, "cp -f %s%s %s%s", TESTCODEDIR, BINARYFILE, MAINCODEDIR, BINARYFILE );

      ch->print( "&RDNS Resolver file updated to main port.\r\n" );
#if defined(__CYGWIN__)
      funcf( ch, do_mudexec, "cp -f %sresolver.exe %sresolver.exe", TESTCODEDIR, MAINCODEDIR );
#else
      funcf( ch, do_mudexec, "cp -f %sresolver %sresolver", TESTCODEDIR, MAINCODEDIR );
#endif

      ch->print( "&RShared Object files updated to main port.\r\n" );
      funcf( ch, do_mudexec, "cp -f %smodules/so/*.so %smodules/so", TESTCODEDIR, MAINCODEDIR );
   }
}

/* This command copies race files from build port to main port and code port - Samson 10-13-98 */
CMDF( do_copyrace )
{
   char buf[MSL];
   string fname;

   if( ch->isnpc(  ) )
   {
      ch->print( "Mobs cannot use the copyrace command!\r\n" );
      return;
   }

   if( mud_port != BUILDPORT )
   {
      ch->print( "&RThe copyrace command may only be used from the Builders' Port.\r\n" );
      return;
   }

   if( argument.empty(  ) )
   {
      ch->print( "You must specify a file to copy.\r\n" );
      return;
   }

   if( !str_cmp( argument, "all" ) )
      fname = "*.race";
   else
   {
      fname = argument;
      snprintf( buf, MSL, "%s%s", BUILDRACEDIR, fname.c_str(  ) );
      if( !copy_file( ch, buf ) )
         return;
   }

   /*
    * Builders' port to Code port 
    */
   ch->print( "&GRace file(s) updated to code port.\r\n" );
#ifdef USEGLOB
   funcf( ch, do_mudexec, "cp %s%s %s", BUILDRACEDIR, fname.c_str(  ), CODERACEDIR );
#else
   funcf( ch, command_pipe, "cp %s%s %s", BUILDRACEDIR, fname.c_str(  ), CODERACEDIR );
#endif

   if( !sysdata->TESTINGMODE )
   {
      /*
       * Builders' port to Main port 
       */
      ch->print( "&RRace file(s) updated to main port.\r\n" );
#ifdef USEGLOB
      funcf( ch, do_mudexec, "cp %s%s %s", BUILDRACEDIR, fname.c_str(  ), MAINRACEDIR );
#else
      funcf( ch, command_pipe, "cp %s%s %s", BUILDRACEDIR, fname.c_str(  ), MAINRACEDIR );
#endif
   }
}

/* This command copies deity files from build port to main port and code port - Samson 10-13-98 */
CMDF( do_copydeity )
{
   char buf[MSL];
   string fname;

   if( ch->isnpc(  ) )
   {
      ch->print( "Mobs cannot use the copydeity command!\r\n" );
      return;
   }

   if( mud_port != BUILDPORT )
   {
      ch->print( "&RThe copydeity command may only be used from the Builders' Port.\r\n" );
      return;
   }

   if( argument.empty(  ) )
   {
      ch->print( "You must specify a file to copy.\r\n" );
      return;
   }

   if( !str_cmp( argument, "all" ) )
      fname = "*";
   else
   {
      fname = argument;
      snprintf( buf, MSL, "%s%s", BUILDDEITYDIR, fname.c_str(  ) );
      if( !copy_file( ch, buf ) )
         return;
   }

   /*
    * Builders' port to Code port 
    */
   ch->print( "&GDeity file(s) updated to code port.\r\n" );
#ifdef USEGLOB
   funcf( ch, do_mudexec, "cp %s%s %s", BUILDDEITYDIR, fname.c_str(  ), CODEDEITYDIR );
#else
   funcf( ch, command_pipe, "cp %s%s %s", BUILDDEITYDIR, fname.c_str(  ), CODEDEITYDIR );
#endif

   if( !sysdata->TESTINGMODE )
   {
      /*
       * Builders' port to Main port 
       */
      ch->print( "&RDeity file(s) updated to main port.\r\n" );
#ifdef USEGLOB
      funcf( ch, do_mudexec, "cp %s%s %s", BUILDDEITYDIR, fname.c_str(  ), MAINDEITYDIR );
#else
      funcf( ch, command_pipe, "cp %s%s %s", BUILDDEITYDIR, fname.c_str(  ), MAINDEITYDIR );
#endif
   }
}

/*
====================
GREP In-Game command	-Nopey
====================
*/
/* Modified by Samson to be a bit less restrictive. So one can grep anywhere the account will allow. */
CMDF( do_grep )
{
   char buf[MSL];

   ch->set_color( AT_PLAIN );

   if( argument.empty(  ) )
      mudstrlcpy( buf, "grep --help", MSL ); /* Will cause it to forward grep's help options to you */
   else
      snprintf( buf, MSL, "grep -n %s", argument.c_str(  ) );  /* Line numbers are somewhat important */

#ifdef USEGLOB
   do_mudexec( ch, buf );
#else
   command_pipe( ch, buf );
#endif
}

/* IP/DNS resolver - passes to the shell code now - Samson 4-23-00 */
CMDF( do_resolve )
{
   if( argument.empty(  ) )
   {
      ch->print( "Resolve what?\r\n" );
      return;
   }
   funcf( ch, do_mudexec, "host %s", argument.c_str(  ) );
}

shell_cmd::shell_cmd(  )
{
   init_memory( &_do_fun, &_log, sizeof( _log ) );
   flags = 0;
}

shell_cmd::~shell_cmd(  )
{
}

void free_shellcommands( void )
{
   list < shell_cmd * >::iterator scmd;

   for( scmd = shellcmdlist.begin(  ); scmd != shellcmdlist.end(  ); )
   {
      shell_cmd *scommand = *scmd;
      ++scmd;

      shellcmdlist.remove( scommand );
      deleteptr( scommand );
   }
   shellcmdlist.clear(  );
}

void add_shellcommand( shell_cmd * command )
{
   string buf;

   if( !command )
   {
      bug( "%s: NULL command", __FUNCTION__ );
      return;
   }

   if( command->get_name(  ).empty(  ) )
   {
      bug( "%s: Empty command->name", __FUNCTION__ );
      return;
   }

   if( !command->get_func(  ) )
   {
      bug( "%s: NULL command->do_fun for command %s", __FUNCTION__, command->get_name(  ).c_str(  ) );
      return;
   }

   /*
    * make sure the name is all lowercase 
    */
   buf = command->get_name(  );
   strlower( buf );
   command->set_name( buf );

   bool inserted = false;
   list < shell_cmd * >::iterator scmd;
   for( scmd = shellcmdlist.begin(  ); scmd != shellcmdlist.end(  ); ++scmd )
   {
      shell_cmd *shellcmd = *scmd;

      if( shellcmd->get_name(  ).compare( command->get_name(  ) ) > 0 )
      {
         shellcmdlist.insert( scmd, command );
         inserted = true;
         break;
      }
   }
   if( !inserted )
      shellcmdlist.push_back( command );

   /*
    * Kick it out of the main command table if it's there 
    */
   cmd_type *cmd;
   if( ( cmd = find_command( command->get_name(  ) ) ) != NULL )
   {
      log_printf( "Removing command: %s and replacing in shell command table.", cmd->name.c_str(  ) );
      unlink_command( cmd );
      deleteptr( cmd );
   }
}

shell_cmd *find_shellcommand( const string & command )
{
   list < shell_cmd * >::iterator cmd;

   for( cmd = shellcmdlist.begin(  ); cmd != shellcmdlist.end(  ); ++cmd )
   {
      shell_cmd *scmd = *cmd;

      if( !str_prefix( command, scmd->get_name(  ) ) )
         return scmd;
   }
   return NULL;
}

void load_shellcommands( void )
{
   shell_cmd *scmd = NULL;
   ifstream stream;
   int version = 0;

   shellcmdlist.clear(  );

   stream.open( SHELL_COMMAND_FILE );
   if( !stream.is_open(  ) )
   {
      bug( "%s: Cannot open %s", __FUNCTION__, SHELL_COMMAND_FILE );
      exit( 1 );
   }

   do
   {
      string key, value;
      char buf[MIL];

      stream >> key;
      stream.getline( buf, MIL );
      value = buf;

      strip_lspace( key );
      strip_lspace( value );
      strip_tilde( value );

      if( key.empty(  ) )
         continue;

      if( key == "#COMMAND" )
         scmd = new shell_cmd;
      else if( key == "#VERSION" )
         version = atoi( value.c_str(  ) );
      else if( key == "Name" )
         scmd->set_name( value );
      else if( key == "Code" )
      {
         scmd->set_func( skill_function( value ) );
         scmd->set_func_name( value );
      }
      else if( key == "Position" )
         scmd->set_position( value );
      else if( key == "Level" )
         scmd->set_level( atoi( value.c_str(  ) ) );
      else if( key == "Log" )
         scmd->set_log( value );
      else if( key == "Flags" )
      {
         if( version < 3 )
            scmd->flags = atoi( value.c_str(  ) );
         else
            flag_string_set( value, scmd->flags, cmd_flags );
      }
      else if( key == "End" )
      {
         if( scmd->get_name(  ).empty(  ) )
         {
            bug( "%s: Command name not found", __FUNCTION__ );
            deleteptr( scmd );
            continue;
         }

         if( scmd->get_func_name(  ).empty(  ) )
         {
            bug( "%s: Command name not found", __FUNCTION__ );
            deleteptr( scmd );
            continue;
         }

         add_shellcommand( scmd );
         /*
          * Automatically approve all immortal commands for use as a ghost 
          */
         if( scmd->get_level(  ) >= LEVEL_IMMORTAL )
            scmd->flags.set( CMD_GHOST );
      }
      else
         bug( "%s: Bad line in shell commands file: %s %s", __FUNCTION__, key.c_str(  ), value.c_str(  ) );
   }
   while( !stream.eof(  ) );
   stream.close(  );
}

const int SHELLCMDVERSION = 3;
/* Updated to 1 for command position - Samson 4-26-00 */
/* Updated to 2 for log flag - Samson 4-26-00 */
/* Updated to 3 for command flags - Samson 7-9-00 */
void save_shellcommands( void )
{
   ofstream stream;

   stream.open( SHELL_COMMAND_FILE );
   if( !stream.is_open(  ) )
   {
      bug( "Cannot open %s for writing", SHELL_COMMAND_FILE );
      perror( SHELL_COMMAND_FILE );
      return;
   }

   stream << "#VERSION " << SHELLCMDVERSION << endl;

   list < shell_cmd * >::iterator icommand;
   for( icommand = shellcmdlist.begin(  ); icommand != shellcmdlist.end(  ); ++icommand )
   {
      shell_cmd *command = *icommand;

      if( command->get_name(  ).empty(  ) )
      {
         bug( "%s: blank command in list", __FUNCTION__ );
         continue;
      }
      stream << "#COMMAND" << endl;
      stream << "Name        " << command->get_name(  ) << endl;
      // Modded to use new field - Trax
      stream << "Code        " << command->get_func_name(  ) << endl;
      stream << "Position    " << npc_position[command->get_position(  )] << endl;
      stream << "Level       " << command->get_level(  ) << endl;
      stream << "Log         " << log_flag[command->get_log(  )] << endl;
      if( command->flags.any(  ) )
         stream << "Flags       " << bitset_string( command->flags, cmd_flags ) << endl;
      stream << "End" << endl << endl;
   }
   stream.close(  );
}

void shellcommands( char_data * ch, short curr_lvl )
{
   list < shell_cmd * >::iterator icmd;
   int col = 1;

   ch->pager( "\r\n" );
   for( icmd = shellcmdlist.begin(  ); icmd != shellcmdlist.end(  ); ++icmd )
   {
      shell_cmd *cmd = *icmd;

      if( cmd->get_level(  ) == curr_lvl )
      {
         ch->pagerf( "%-12s", cmd->get_cname(  ) );
         if( ++col % 6 == 0 )
            ch->pager( "\r\n" );
      }
   }
   if( col % 6 != 0 )
      ch->pager( "\r\n" );
}

/* Clone of do_cedit modified because shell commands don't use a hash table */
CMDF( do_shelledit )
{
   shell_cmd *command;
   string arg1, arg2;

   ch->set_color( AT_IMMORT );

   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );
   if( arg1.empty(  ) )
   {
      ch->print( "Syntax: shelledit save\r\n" );
      ch->print( "Syntax: shelledit <command> create [code]\r\n" );
      ch->print( "Syntax: shelledit <command> delete\r\n" );
      ch->print( "Syntax: shelledit <command> show\r\n" );
      ch->print( "Syntax: shelledit <command> [field]\r\n" );
      ch->print( "\r\nField being one of:\r\n" );
      ch->print( "  level position log code flags\r\n" );
      return;
   }

   if( !str_cmp( arg1, "save" ) )
   {
      save_shellcommands(  );
      ch->print( "Shell commands saved.\r\n" );
      return;
   }

   command = find_shellcommand( arg1 );
   if( !str_cmp( arg2, "create" ) )
   {
      if( command )
      {
         ch->print( "That command already exists!\r\n" );
         return;
      }
      command = new shell_cmd;
      command->set_name( arg1 );
      command->set_level( ch->get_trust(  ) );
      if( !argument.empty(  ) )
         one_argument( argument, arg2 );
      else
         arg2 = "do_" + arg1;
      command->set_func( skill_function( arg2 ) );
      command->set_func_name( arg2 );
      add_shellcommand( command );
      ch->print( "Shell command added.\r\n" );
      if( command->get_func(  ) == skill_notfound )
         ch->printf( "Code %s not found. Set to no code.\r\n", arg2.c_str(  ) );
      return;
   }

   if( !command )
   {
      ch->print( "Shell command not found.\r\n" );
      return;
   }
   else if( command->get_level(  ) > ch->get_trust(  ) )
   {
      ch->print( "You cannot touch this command.\r\n" );
      return;
   }

   if( arg2.empty(  ) || !str_cmp( arg2, "show" ) )
   {
      ch->printf( "Command:   %s\r\nLevel:     %d\r\nPosition:  %s\r\nLog:       %s\r\nFunc Name: %s\r\nFlags:     %s\r\n",
                  command->get_cname(  ), command->get_level(  ), npc_position[command->get_position(  )], log_flag[command->get_log(  )],
                  command->get_func_cname(  ), bitset_string( command->flags, cmd_flags ) );
      return;
   }

   if( !str_cmp( arg2, "delete" ) )
   {
      deleteptr( command );
      ch->print( "Shell command deleted.\r\n" );
      return;
   }

   if( !str_cmp( arg2, "code" ) )
   {
      DO_FUN *fun = skill_function( argument );

      if( fun == skill_notfound )
      {
         ch->print( "Code not found.\r\n" );
         return;
      }
      command->set_func( fun );
      command->set_func_name( argument );
      ch->print( "Shell command code updated.\r\n" );
      return;
   }

   if( !str_cmp( arg2, "level" ) )
   {
      short level = atoi( argument.c_str(  ) );

      command->set_level( level );
      ch->print( "Command level updated.\r\n" );
      return;
   }

   if( !str_cmp( arg2, "log" ) )
   {
      command->set_log( argument );
      ch->print( "Command log setting updated.\r\n" );
      return;
   }

   if( !str_cmp( arg2, "position" ) )
   {
      command->set_position( argument );
      ch->print( "Command position updated.\r\n" );
      return;
   }

   if( !str_cmp( arg2, "flags" ) )
   {
      int flag;

      flag = get_cmdflag( argument );
      if( flag < 0 || flag >= MAX_CMD_FLAG )
      {
         ch->printf( "Unknown flag %s.\r\n", argument.c_str(  ) );
         return;
      }
      command->flags.flip( flag );
      ch->print( "Command flags updated.\r\n" );
      return;
   }

   if( !str_cmp( arg2, "name" ) )
   {
      one_argument( argument, arg1 );

      if( arg1.empty(  ) )
      {
         ch->print( "Cannot clear name field!\r\n" );
         return;
      }
      command->set_name( arg1 );
      ch->print( "Done.\r\n" );
      return;
   }
   /*
    * display usage message 
    */
   do_shelledit( ch, NULL );
}

bool shell_hook( char_data * ch, const string & command, string & argument )
{
   list < shell_cmd * >::iterator icmd;
   shell_cmd *cmd = NULL;
   char logline[MIL];
   bool found = false;

   int trust = ch->get_trust(  );

   for( icmd = shellcmdlist.begin(  ); icmd != shellcmdlist.end(  ); ++icmd )
   {
      cmd = ( *icmd );

      if( !str_prefix( command, cmd->get_name(  ) )
          && ( cmd->get_level(  ) <= trust
               || ( !ch->isnpc(  ) && !ch->pcdata->bestowments.empty(  )
                    && hasname( ch->pcdata->bestowments, cmd->get_name(  ) ) && cmd->get_level(  ) <= ( trust + sysdata->bestow_dif ) ) ) )
      {
         found = true;
         break;
      }
   }

   if( !found )
      return false;

   snprintf( logline, MIL, "%s %s", command.c_str(  ), argument.c_str(  ) );

   /*
    * Log and snoop.
    */
   snprintf( lastplayercmd, MIL * 2, "%s used command: %s", ch->name, logline );

   if( cmd->get_log(  ) == LOG_NEVER )
      mudstrlcpy( logline, "XXXXXXXX XXXXXXXX XXXXXXXX", MIL );

   int loglvl = cmd->get_log(  );

   if( !ch->isnpc(  ) && cmd->flags.test( CMD_MUDPROG ) )
   {
      ch->print( "Huh?\r\n" );
      return false;
   }

   if( ch->has_pcflag( PCFLAG_LOG ) || fLogAll || loglvl == LOG_BUILD || loglvl == LOG_HIGH || loglvl == LOG_ALWAYS )
   {
      /*
       * Make it so a 'log all' will send most output to the log
       * file only, and not spam the log channel to death - Thoric
       */
      if( fLogAll && loglvl == LOG_NORMAL && ch->has_pcflag( PCFLAG_LOG ) )
         loglvl = LOG_ALL;

      /*
       * Added by Narn to show who is switched into a mob that executes
       * a logged command.  Check for descriptor in case force is used. 
       */
      if( ch->desc && ch->desc->original )
         log_printf_plus( loglvl, ch->level, "Log %s (%s): %s", ch->name, ch->desc->original->name, logline );
      else
         log_printf_plus( loglvl, ch->level, "Log %s: %s", ch->name, logline );
   }

   if( ch->desc && ch->desc->snoop_by )
   {
      ch->desc->snoop_by->write_to_buffer( ch->name );
      ch->desc->snoop_by->write_to_buffer( "% " );
      ch->desc->snoop_by->write_to_buffer( logline );
      ch->desc->snoop_by->write_to_buffer( "\r\n" );
   }

   ( *cmd->get_func(  ) )( ch, argument );
   return true;
}
