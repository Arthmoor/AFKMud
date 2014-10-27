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
 *                      New Name Authorization module                       *
 ****************************************************************************/

/*
 *  New name authorization system
 *  Author: Rantic (supfly@geocities.com)
 *  of FrozenMUD (empire.digiunix.net 4000)
 *
 *  Permission to use and distribute this code is granted provided
 *  this header is retained and unaltered, and the distribution
 *  package contains all the original files unmodified.
 *  If you modify this code and use/distribute modified versions
 *  you must give credit to the original author(s).
 */

#include <sys/stat.h>
#include <sys/time.h>
#if defined(WIN32)
#include <unistd.h>
void gettimeofday( struct timeval *, struct timezone * );
#endif
#include "mud.h"
#include "descriptor.h"
#include "new_auth.h"
#include "mud_prog.h"

char_data *get_waiting_desc( char_data *, char * );
CMDF( do_reserve );
CMDF( do_destroy );
bool can_use_mprog( char_data * );

list<auth_data*> authlist;

void name_generator( char *argument )
{
   int start_counter = 0;
   int middle_counter = 0;
   int end_counter = 0;
   char start_string[100][10];
   char middle_string[100][10];
   char end_string[100][10];
   char tempstring[151];
   struct timeval starttime;
   time_t t;
   char name[300];
   FILE *infile;

   tempstring[0] = '\0';

   if( !( infile = fopen( NAMEGEN_FILE, "r" ) ) )
   {
      log_string( "Can't find NAMEGEN file." );
      return;
   }

   fgets( tempstring, 150, infile );
   tempstring[strlen( tempstring ) - 1] = '\0';
   while( str_cmp( tempstring, "[start]" ) != 0 )
   {
      fgets( tempstring, 150, infile );
      tempstring[strlen( tempstring ) - 1] = '\0'; /* remove linefeed */
   }

   while( str_cmp( tempstring, "[middle]" ) != 0 )
   {
      fgets( tempstring, 150, infile );
      tempstring[strlen( tempstring ) - 1] = '\0'; /* remove linefeed */
      if( tempstring[0] != '/' )
         mudstrlcpy( start_string[start_counter++], tempstring, 100 );
   }
   while( str_cmp( tempstring, "[end]" ) != 0 )
   {
      fgets( tempstring, 150, infile );
      tempstring[strlen( tempstring ) - 1] = '\0'; /* remove linefeed */
      if( tempstring[0] != '/' )
         mudstrlcpy( middle_string[middle_counter++], tempstring, 100 );
   }
   while( str_cmp( tempstring, "[finish]" ) != 0 )
   {
      fgets( tempstring, 150, infile );
      tempstring[strlen( tempstring ) - 1] = '\0'; /* remove linefeed */
      if( tempstring[0] != '/' )
         mudstrlcpy( end_string[end_counter++], tempstring, 100 );
   }
   FCLOSE( infile );
   gettimeofday( &starttime, NULL );
   srand( ( unsigned )time( &t ) + starttime.tv_usec );
   --start_counter;
   --middle_counter;
   --end_counter;

   mudstrlcpy( name, start_string[rand(  ) % start_counter], 300 );  /* get a start */
   mudstrlcat( name, middle_string[rand(  ) % middle_counter], 300 );   /* get a middle */
   mudstrlcat( name, end_string[rand(  ) % end_counter], 300 );   /* get an ending */
   mudstrlcat( argument, name, 300 );
   return;
}

CMDF( do_name_generator )
{
   char name[300];

   name[0] = '\0';

   name_generator( name );
   ch->print( name );
   return;
}

/* Added by Tarl 5 Dec 02 to allow picking names from a file. Used for the namegen
   code in reset.c */
void pick_name( char *argument, char *filename )
{
   struct timeval starttime;
   time_t t;
   char name[200];
   char tempstring[151];
   int counter = 0;
   FILE *infile;
   char names[200][20];

   tempstring[0] = '\0';

   infile = fopen( filename, "r" );
   if( infile == NULL )
   {
      log_printf( "Can't find %s", filename );
      return;
   }

   fgets( tempstring, 150, infile );
   tempstring[strlen( tempstring ) - 1] = '\0';
   while( str_cmp( tempstring, "[start]" ) != 0 )
   {
      fgets( tempstring, 100, infile );
      tempstring[strlen( tempstring ) - 1] = '\0'; /* remove linefeed */
   }
   while( str_cmp( tempstring, "[finish]" ) != 0 )
   {
      fgets( tempstring, 100, infile );
      tempstring[strlen( tempstring ) - 1] = '\0';
      if( tempstring[0] != '/' )
         mudstrlcpy( names[counter++], tempstring, 200 );
   }
   FCLOSE( infile );
   gettimeofday( &starttime, NULL );
   srand( ( unsigned )time( &t ) + starttime.tv_usec );
   --counter;

   mudstrlcpy( name, names[rand(  ) % counter], 200 );
   mudstrlcat( argument, name, 200 );
   return;
}

bool exists_player( char *name )
{
   struct stat fst;
   char buf[256];
   char_data *victim = NULL;

   /*
    * Stands to reason that if there ain't a name to look at, they damn well don't exist! 
    */
   if( !name || name[0] == '\0' || !str_cmp( name, "" ) )
      return false;

   snprintf( buf, 256, "%s%c/%s", PLAYER_DIR, tolower( name[0] ), capitalize( name ) );

   if( stat( buf, &fst ) != -1 )
      return true;

   else if( ( victim = supermob->get_char_world( name ) ) != NULL )
      return true;

   return false;
}

bool exists_player( string name )
{
   struct stat fst;
   char buf[256];
   char_data *victim = NULL;

   /*
    * Stands to reason that if there ain't a name to look at, they damn well don't exist! 
    */
   if( name.empty() )
      return false;

   snprintf( buf, 256, "%s%c/%s", PLAYER_DIR, tolower( name[0] ), capitalize( name ).c_str() );

   if( stat( buf, &fst ) != -1 )
      return true;

   else if( ( victim = supermob->get_char_world( name ) ) != NULL )
      return true;

   return false;
}

auth_data::auth_data()
{
   init_memory( &name, &state, sizeof( state ) );
}

auth_data::~auth_data()
{
   STRFREE( authed_by );
   STRFREE( change_by );
   STRFREE( name );
   authlist.remove( this );
}

void free_all_auths( void )
{
   list<auth_data*>::iterator au;

   for( au = authlist.begin(); au != authlist.end(); )
   {
      auth_data *auth = (*au);
      ++au;

      deleteptr( auth );
   }
   return;
}

void clean_auth_list( void )
{
   list<auth_data*>::iterator auth;

   for( auth = authlist.begin(); auth != authlist.end(); )
   {
      auth_data *nauth = (*auth);
      ++auth;

      if( !exists_player( nauth->name ) )
         deleteptr( nauth );
      else
      {
         time_t tdiff = 0;
         time_t curr_time = time( 0 );
         struct stat fst;
         char file[256], name[MSL];
         int MAX_AUTH_WAIT = 7;

         mudstrlcpy( name, nauth->name, MSL );
         snprintf( file, 256, "%s%c/%s", PLAYER_DIR, LOWER( nauth->name[0] ), capitalize( nauth->name ) );

         if( stat( file, &fst ) != -1 )
            tdiff = ( curr_time - fst.st_mtime ) / 86400;
         else
            bug( "%s: File %s does not exist!", __FUNCTION__, file );

         if( tdiff > MAX_AUTH_WAIT )
         {
            if( unlink( file ) == -1 )
               perror( "Unlink: do_auth: \"clean\"" );
            else
               log_printf( "%s deleted for inactivity: %ld days", file, ( long int )tdiff );
         }
      }
   }
}

void write_auth_file( FILE * fpout, auth_data *alist )
{
   fprintf( fpout, "Name     %s~\n", alist->name );
   fprintf( fpout, "State    %d\n", alist->state );
   if( alist->authed_by )
      fprintf( fpout, "AuthedBy %s~\n", alist->authed_by );
   if( alist->change_by )
      fprintf( fpout, "Change   %s~\n", alist->change_by );
   fprintf( fpout, "%s", "End\n\n" );
}

void fread_auth( FILE * fp )
{
   auth_data *new_auth = new auth_data;

   for( ;; )
   {
      const char *word = ( feof( fp ) ? "End" : fread_word( fp ) );

      if( word[0] == '\0' )
      {
         bug( "%s: EOF encountered reading file!", __FUNCTION__ );
         word = "End";
      }

      switch ( UPPER( word[0] ) )
      {
         default:
            bug( "%s: no match: %s", __FUNCTION__, word );
            fread_to_eol( fp );
            break;

         case '*':
            fread_to_eol( fp );
            break;

         case 'A':
            KEY( "AuthedBy", new_auth->authed_by, fread_string( fp ) );
            break;

         case 'C':
            KEY( "Change", new_auth->change_by, fread_string( fp ) );
            break;

         case 'E':
            if( !str_cmp( word, "End" ) )
            {
               authlist.push_back( new_auth );
               return;
            }
            break;

         case 'N':
            KEY( "Name", new_auth->name, fread_string( fp ) );
            break;

         case 'S':
            if( !str_cmp( word, "State" ) )
            {
               new_auth->state = fread_number( fp );
               if( new_auth->state == AUTH_ONLINE || new_auth->state == AUTH_LINK_DEAD )
               /*
                * Crash proofing. Can't be online when  booting up. Would suck for do_auth 
                */
               new_auth->state = AUTH_OFFLINE;
               break;
            }
            break;
      }
   }
}

void save_auth_list( void )
{
   FILE *fpout;
   list<auth_data*>::iterator alist;

   if( !( fpout = fopen( AUTH_FILE, "w" ) ) )
   {
      bug( "%s: Cannot open auth.dat for writing.", __FUNCTION__ );
      perror( AUTH_FILE );
      return;
   }

   for( alist = authlist.begin(); alist != authlist.end(); ++alist )
   {
      auth_data *auth = (*alist);

      fprintf( fpout, "%s", "#AUTH\n" );
      write_auth_file( fpout, auth );
   }

   fprintf( fpout, "%s", "#END\n" );
   FCLOSE( fpout );
}

void clear_auth_list( void )
{
   list<auth_data*>::iterator auth;

   for( auth = authlist.begin(); auth != authlist.end(); )
   {
      auth_data *nauth = (*auth);
      ++auth;

      if( !exists_player( nauth->name ) )
         deleteptr( nauth );
   }
   save_auth_list(  );
}

void load_auth_list( void )
{
   FILE *fp;
   int x;

   authlist.clear();

   if( ( fp = fopen( AUTH_FILE, "r" ) ) != NULL )
   {
      x = 0;
      for( ;; )
      {
         char letter;
         char *word;

         letter = fread_letter( fp );
         if( letter == '*' )
         {
            fread_to_eol( fp );
            continue;
         }

         if( letter != '#' )
         {
            bug( "%s: # not found.", __FUNCTION__ );
            break;
         }

         word = fread_word( fp );
         if( !str_cmp( word, "AUTH" ) )
         {
            fread_auth( fp );
            continue;
         }
         else if( !str_cmp( word, "END" ) )
            break;
         else
         {
            bug( "%s: bad section: %s", __FUNCTION__, word );
            continue;
         }
      }
      FCLOSE( fp );
   }
   else
   {
      bug( "%s: Cannot open auth.dat", __FUNCTION__ );
      return;
   }
   clear_auth_list(  );
}

int get_auth_state( char_data * ch )
{
   list<auth_data*>::iterator namestate;
   int state;

   state = AUTH_AUTHED;

   for( namestate = authlist.begin(); namestate != authlist.end(); ++namestate )
   {
      auth_data *auth = (*namestate);

      if( !str_cmp( auth->name, ch->name ) )
         return auth->state;
   }
   return state;
}

auth_data *get_auth_name( char *name )
{
   list<auth_data*>::iterator mname;

   for( mname = authlist.begin(); mname != authlist.end(); ++mname )
   {
      auth_data *auth = (*mname);

      if( !str_cmp( auth->name, name ) ) /* If the name is already in the list, break */
         return auth;
   }
   return NULL;
}

void add_to_auth( char_data * ch )
{
   auth_data *new_name;

   if( ( new_name = get_auth_name( ch->name ) ) != NULL )
      return;
   else
   {
      new_name = new auth_data;
      new_name->name = QUICKLINK( ch->name );
      new_name->state = AUTH_ONLINE;   /* Just entered the game */
      authlist.push_back( new_name );
      save_auth_list(  );
   }
}

void remove_from_auth( char *name )
{
   auth_data *old_name;

   if( !( old_name = get_auth_name( name ) ) ) /* Its not old */
      return;
   else
   {
      deleteptr( old_name );
      save_auth_list(  );
   }
}

void check_auth_state( char_data * ch )
{
   auth_data *old_auth;
   int level;

   level = check_command_level( "authorize", MAX_LEVEL );
   if( level == -1 )
      level = LEVEL_IMMORTAL;

   if( !( old_auth = get_auth_name( ch->name ) ) )
      return;

   if( old_auth->state == AUTH_OFFLINE || old_auth->state == AUTH_LINK_DEAD )
   {
      old_auth->state = AUTH_ONLINE;
      save_auth_list(  );
   }
   else if( old_auth->state == AUTH_CHANGE_NAME )
   {
      ch->printf( "&R\r\nThe MUD Administrators have found the name %s\r\n"
                  "to be unacceptable. You must choose a new one.\r\n"
                  "The name you choose must be medieval and original.\r\n"
                  "No titles, descriptive words, or names close to any existing\r\n"
                  "Immortal's name. See 'help name'.\r\n", ch->name );
   }
   else if( old_auth->state == AUTH_AUTHED )
   {
      STRFREE( ch->pcdata->authed_by );
      if( old_auth->authed_by )
      {
         ch->pcdata->authed_by = QUICKLINK( old_auth->authed_by );
         STRFREE( old_auth->authed_by );
      }
      else
         ch->pcdata->authed_by = STRALLOC( "The Code" );

      ch->printf( "\r\n&GThe MUD Administrators have accepted the name %s.\r\nYou are now free to roam %s.\r\n",
                  ch->name, sysdata->mud_name );
      ch->unset_pcflag( PCFLAG_UNAUTHED );
      remove_from_auth( ch->name );
      return;
   }
   return;
}

/* 
 * Check if the name prefix uniquely identifies a char descriptor
 */
char_data *get_waiting_desc( char_data * ch, char *name )
{
   list<descriptor_data*>::iterator ds;
   char_data *ret_char = NULL;
   static unsigned int number_of_hits;

   number_of_hits = 0;
   for( ds = dlist.begin(); ds != dlist.end(); ++ds )
   {
      descriptor_data *d = (*ds);

      if( d->character && ( !str_prefix( name, d->character->name ) ) && IS_WAITING_FOR_AUTH( d->character ) )
      {
         if( ++number_of_hits > 1 )
         {
            ch->printf( "%s does not uniquely identify a char.\r\n", name );
            return NULL;
         }
         ret_char = d->character;   /* return current char on exit */
      }
   }
   if( number_of_hits == 1 )
      return ret_char;
   else
   {
      ch->print( "No one like that waiting for authorization.\r\n" );
      return NULL;
   }
}

/* new auth */
CMDF( do_authorize )
{
   char arg1[MIL];
   char_data *victim = NULL;
   list<auth_data*>::iterator auth;
   auth_data *nauth = NULL;
   int level;
   bool offline, authed, changename, pending;

   offline = authed = changename = pending = false;

   /*
    * Checks level of authorize command, for log messages. - Samson 10-18-98 
    */
   if( ( level = check_command_level( "authorize", MAX_LEVEL ) ) == -1 )
      level = LEVEL_IMMORTAL;

   ch->set_color( AT_IMMORT );

   argument = one_argument( argument, arg1 );
   if( !arg1 || arg1[0] == '\0' )
   {
      ch->print( "To approve a waiting character: auth <name>\r\n" );
      ch->print( "To deny a waiting character:    auth <name> reject\r\n" );
      ch->print( "To ask a waiting character to change names: auth <name> change\r\n" );
      ch->print( "To have the code verify the list: auth fixlist\r\n" );
      ch->print( "To have the code purge inactive entries: auth clean\r\n" );

      ch->print( "\r\n&[divider]--- Characters awaiting approval ---\r\n" );

      auth_data *au;
      for( auth = authlist.begin(); auth != authlist.end(); ++auth )
      {
         au = (*auth);

         if( au->state == AUTH_CHANGE_NAME )
            changename = true;
         else if( au->state == AUTH_AUTHED )
            authed = true;

         if( au->name != NULL && au->state < AUTH_CHANGE_NAME )
            pending = true;
      }
      if( pending )
      {
         for( auth = authlist.begin(); auth != authlist.end(); ++auth )
         {
            au = (*auth);

            if( au->state < AUTH_CHANGE_NAME )
            {
               switch ( au->state )
               {
                  default:
                     ch->printf( "\t%s\t\tUnknown?\r\n", au->name );
                     break;
                  case AUTH_LINK_DEAD:
                     ch->printf( "\t%s\t\tLink Dead\r\n", au->name );
                     break;
                  case AUTH_ONLINE:
                     ch->printf( "\t%s\t\tOnline\r\n", au->name );
                     break;
                  case AUTH_OFFLINE:
                     ch->printf( "\t%s\t\tOffline\r\n", au->name );
                     break;
               }
            }
         }
      }
      else
         ch->print( "\tNone\r\n" );

      if( authed )
      {
         ch->print( "\r\n&[divider]Authorized Characters:\r\n" );
         ch->print( "---------------------------------------------\r\n" );
         for( auth = authlist.begin(); auth != authlist.end(); ++auth )
         {
            au = (*auth);

            if( au->state == AUTH_AUTHED )
               ch->printf( "Name: %s\t Approved by: %s\r\n", au->name, au->authed_by );
         }
      }
      if( changename )
      {
         ch->print( "\r\n&[divider]Change Name:\r\n" );
         ch->print( "---------------------------------------------\r\n" );
         for( auth = authlist.begin(); auth != authlist.end(); ++auth )
         {
            au = (*auth);

            if( au->state == AUTH_CHANGE_NAME )
               ch->printf( "Name: %s\t Change requested by: %s\r\n", au->name, au->change_by );
         }
      }
      return;
   }

   if( !str_cmp( arg1, "fixlist" ) )
   {
      ch->pager( "Checking authorization list...\r\n" );
      clear_auth_list(  );
      ch->pager( "Done.\r\n" );
      return;
   }

   if( !str_cmp( arg1, "clean" ) )
   {
      ch->pager( "Cleaning authorization list...\r\n" );
      clean_auth_list(  );
      ch->pager( "Checking authorization list...\r\n" );
      clear_auth_list(  );
      ch->pager( "Done.\r\n" );
      return;
   }

   if( ( nauth = get_auth_name( arg1 ) ) != NULL )
   {
      if( nauth->state == AUTH_OFFLINE || nauth->state == AUTH_LINK_DEAD )
      {
         offline = true;
         if( !argument || argument[0] == '\0' || !str_cmp( argument, "accept" ) || !str_cmp( argument, "yes" ) )
         {
            nauth->state = AUTH_AUTHED;
            nauth->authed_by = QUICKLINK( ch->name );
            save_auth_list(  );
            log_printf_plus( LOG_AUTH, level, "%s: authorized", nauth->name );
            ch->printf( "You have authorized %s.\r\n", nauth->name );
            return;
         }
         else if( !str_cmp( argument, "reject" ) )
         {
            log_printf_plus( LOG_AUTH, level, "%s: denied authorization", nauth->name );
            ch->printf( "You have denied %s.\r\n", nauth->name );
            /*
             * Addition so that denied names get added to reserved list - Samson 10-18-98 
             */
            funcf( ch, do_reserve, "%s add", nauth->name );
            do_destroy( ch, nauth->name );
            return;
         }
         else if( !str_cmp( argument, "change" ) )
         {
            nauth->state = AUTH_CHANGE_NAME;
            nauth->change_by = QUICKLINK( ch->name );
            save_auth_list(  );
            log_printf_plus( LOG_AUTH, level, "%s: name denied", nauth->name );
            ch->printf( "You requested %s change names.\r\n", nauth->name );
            /*
             * Addition so that requested name changes get added to reserved list - Samson 10-18-98 
             */
            funcf( ch, do_reserve, "%s add", nauth->name );
            return;
         }
         else
         {
            ch->print( "Invalid argument.\r\n" );
            return;
         }
      }
      else
      {
         if( !( victim = get_waiting_desc( ch, arg1 ) ) )
            return;

         victim->set_color( AT_IMMORT );
         if( !argument || argument[0] == '\0' || !str_cmp( argument, "accept" ) || !str_cmp( argument, "yes" ) )
         {
            STRFREE( victim->pcdata->authed_by );
            victim->pcdata->authed_by = QUICKLINK( ch->name );
            log_printf_plus( LOG_AUTH, level, "%s: authorized", victim->name );

            ch->printf( "You have authorized %s.\r\n", victim->name );

            victim->printf( "\r\n&GThe MUD Administrators have accepted the name %s.\r\n"
                         "You are now free to roam the %s.\r\n", victim->name, sysdata->mud_name );
            victim->unset_pcflag( PCFLAG_UNAUTHED );
            remove_from_auth( victim->name );
            return;
         }
         else if( !str_cmp( argument, "reject" ) )
         {
            victim->print( "&RYou have been denied access.\r\n" );
            log_printf_plus( LOG_AUTH, level, "%s: denied authorization", victim->name );
            ch->printf( "You have denied %s.\r\n", victim->name );
            remove_from_auth( victim->name );
            /*
             * Addition to add denied names to reserved list - Samson 10-18-98 
             */
            funcf( ch, do_reserve, "%s add", victim->name );
            do_destroy( ch, victim->name );
            return;
         }
         else if( !str_cmp( argument, "change" ) )
         {
            nauth->state = AUTH_CHANGE_NAME;
            nauth->change_by = QUICKLINK( ch->name );
            save_auth_list(  );
            log_printf_plus( LOG_AUTH, level, "%s: name denied", victim->name );
            victim->printf( "&R\r\nThe MUD Administrators have found the name %s to be unacceptable.\r\n"
                         "You may choose a new name when you reach the end of this area.\r\n"
                         "The name you choose must be medieval and original.\r\n"
                         "No titles, descriptive words, or names close to any existing\r\n"
                         "Immortal's name. See 'help name'.\r\n", victim->name );
            ch->printf( "You requested %s change names.\r\n", victim->name );
            /*
             * Addition to put denied name on reserved list - Samson 10-18-98 
             */
            funcf( ch, do_reserve, "%s add", victim->name );
            return;
         }
         else
         {
            ch->print( "Invalid argument.\r\n" );
            return;
         }
      }
   }
   else
   {
      ch->print( "No such player pending authorization.\r\n" );
      return;
   }
}

/* new auth */
CMDF( do_name )
{
   auth_data *auth_name;

   if( !( auth_name = get_auth_name( ch->name ) ) )
   {
      ch->print( "Huh?\r\n" );
      return;
   }

   argument[0] = UPPER( argument[0] );

   if( !check_parse_name( argument, true ) )
   {
      ch->print( "Illegal name, try another.\r\n" );
      return;
   }

   if( !str_cmp( ch->name, argument ) )
   {
      ch->print( "That's already your name!\r\n" );
      return;
   }

   list<char_data*>::iterator ich;
   for( ich = charlist.begin(); ich != charlist.end(); ++ich )
   {
      char_data *tmp = (*ich);

      if( !str_cmp( argument, tmp->name ) )
      {
         ch->print( "That name is already taken. Please choose another.\r\n" );
         return;
      }
   }

   char fname[256];
   struct stat fst;
   snprintf( fname, 256, "%s%c/%s", PLAYER_DIR, tolower( argument[0] ), capitalize( argument ) );
   if( stat( fname, &fst ) != -1 )
   {
      ch->print( "That name is already taken. Please choose another.\r\n" );
      return;
   }
   snprintf( fname, 256, "%s%c/%s", PLAYER_DIR, tolower( ch->name[0] ), capitalize( ch->name ) );
   unlink( fname );  /* cronel, for auth */

   STRFREE( ch->name );
   ch->name = STRALLOC( argument );
   STRFREE( ch->pcdata->filename );
   ch->pcdata->filename = STRALLOC( argument );
   ch->print( "Your name has been changed and is being submitted for approval.\r\n" );
   STRFREE( auth_name->name );
   auth_name->name = STRALLOC( argument );
   auth_name->state = AUTH_ONLINE;
   STRFREE( auth_name->change_by );
   save_auth_list(  );
   return;
}

/* changed for new auth */
CMDF( do_mpapplyb )
{
   char_data *victim;
   int level;

   /*
    * Checks to see level of authorize command.
    * Makes no sense to see the auth channel if you can't auth. - Samson 12-28-98 
    */
   if( ( level = check_command_level( "authorize", MAX_LEVEL ) ) == -1 )
      level = LEVEL_IMMORTAL;

   if( !can_use_mprog( ch ) )
      return;

   if( !argument || argument[0] == '\0' )
   {
      progbugf( ch, "%s", "Mpapplyb - bad syntax" );
      return;
   }

   if( !( victim = ch->get_char_room( argument ) ) )
   {
      progbugf( ch, "Mpapplyb - no such player %s in room.", argument );
      return;
   }

   if( !victim->desc )
   {
      progbugf( ch, "Mpapplyb - linkdead target %s.", victim->name );
      return;
   }

   if( victim->fighting )
      victim->stop_fighting( true );

   if( NOT_AUTHED( victim ) )
   {
      log_printf_plus( LOG_AUTH, level, "%s [%s] New player entering the game.\r\n", victim->name, victim->desc->host );
      victim->printf( "\r\nYou are now entering the game...\r\n"
                   "However, your character has not been authorized yet and can not\r\n"
                   "advance past level 5 until then. Your character will be saved,\r\n"
                   "but not allowed to fully indulge in %s.\r\n", sysdata->mud_name );
   }
   return;
}

/* changed for new auth */
void auth_update( void )
{
   list<auth_data*>::iterator auth;
   char buf[MIL], lbuf[MSL];
   int level;
   bool found_imm = false; /* Is at least 1 immortal on? */
   bool found_hit = false; /* was at least one found? */

   if( ( level = check_command_level( "authorize", MAX_LEVEL ) ) == -1 )
      level = LEVEL_IMMORTAL;

   mudstrlcpy( lbuf, "--- Characters awaiting approval ---\r\n", MSL );
   for( auth = authlist.begin(); auth != authlist.end(); ++auth )
   {
      auth_data *au = (*auth);

      if( au->state < AUTH_CHANGE_NAME )
      {
         found_hit = true;
         snprintf( buf, MIL, "Name: %s      Status: %s\r\n", au->name,
                   ( au->state == AUTH_ONLINE ) ? "Online" : "Offline" );
         mudstrlcat( lbuf, buf, MSL );
      }
   }

   if( found_hit )
   {
      list<descriptor_data*>::iterator ds;

      for( ds = dlist.begin(); ds != dlist.end(); ++ds )
      {
         descriptor_data *d = (*ds);

         if( d->connected == CON_PLAYING && d->character && d->character->is_immortal(  ) && d->character->level >= level )
            found_imm = true;
      }
      if( found_imm )
         log_string_plus( LOG_AUTH, level, lbuf );
   }
}

/* Modified to require an "add" or "remove" argument in addition to name - Samson 10-18-98 */
/* Gutted to append to an external file now rather than load the pile into memory at boot - Samson 11-21-03 */
CMDF( do_reserve )
{
   char arg[MIL];

   argument = one_argument( argument, arg );

   if( !arg || arg[0] == '\0' )
   {
      ch->print( "To add a name: reserve <name> add\r\n" );
      ch->print( "To remove a name: Someone with shell access has to do this now.\r\n" );
      return;
   }

   if( !str_cmp( argument, "add" ) )
   {
      char buf[MSL];

      /*
       * This grep idea was borrowed from SunderMud.
       * * Reserved names list was getting much too large to load into memory.
       * * Placed last so as to avoid problems from any of the previous conditions causing a problem in shell.
       */
      snprintf( buf, MSL, "grep -i -x %s ../system/reserved.lst > /dev/null", arg );

      if( system( buf ) == 0 )
      {
         ch->printf( "%s is already a reserved name.\r\n" );
         return;
      }

      append_to_file( RESERVED_LIST, "%s", arg );
      ch->print( "Name reserved.\r\n" );
      return;
   }
   ch->print( "Invalid argument.\r\n" );
}
