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

#include <fstream>
#include <sys/stat.h>
#include <sys/time.h>
#if defined(WIN32)
#include <unistd.h>
void gettimeofday( struct timeval *, struct timezone * );
#endif
#include "mud.h"
#include "descriptor.h"
#include "new_auth.h"

char_data *get_waiting_desc( char_data *, const string & );
CMDF( do_reserve );
CMDF( do_destroy );
bool can_use_mprog( char_data * );

list < auth_data * >authlist;

void name_generator( string & argument )
{
   int start_counter = 0, middle_counter = 0, end_counter = 0;
   char start_string[100][10], middle_string[100][10], end_string[100][10];
   char tempstring[151], name[300];
   struct timeval starttime;
   time_t t;
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
   argument.append( name );
}

CMDF( do_name_generator )
{
   string name;

   name_generator( name );
   ch->printf( "%s\r\n", name.c_str(  ) );
}

/* Added by Tarl 5 Dec 02 to allow picking names from a file. Used for the namegen
   code in reset.c */
void pick_name( string & argument, const char *filename )
{
   FILE *infile;
   struct timeval starttime;
   char names[200][20];
   char name[200], tempstring[151];
   int counter = 0;
   time_t t;

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
   argument.append( name );
}

auth_data::auth_data(  )
{
   state = 0;
}

auth_data::~auth_data(  )
{
   authlist.remove( this );
}

void free_all_auths( void )
{
   list < auth_data * >::iterator au;

   for( au = authlist.begin(  ); au != authlist.end(  ); )
   {
      auth_data *auth = *au;
      ++au;

      deleteptr( auth );
   }
}

void clean_auth_list( void )
{
   list < auth_data * >::iterator auth;

   for( auth = authlist.begin(  ); auth != authlist.end(  ); )
   {
      auth_data *nauth = *auth;
      ++auth;

      if( !exists_player( nauth->name ) )
         deleteptr( nauth );
      else
      {
         time_t tdiff = 0;
         time_t curr_time = time( 0 );
         struct stat fst;
         char file[256];
         int MAX_AUTH_WAIT = 7;

         snprintf( file, 256, "%s%c/%s", PLAYER_DIR, LOWER( nauth->name[0] ), capitalize( nauth->name ).c_str(  ) );

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

void save_auth_list( void )
{
   ofstream stream;
   list < auth_data * >::iterator alist;

   stream.open( AUTH_FILE );
   if( !stream.is_open(  ) )
   {
      bug( "%s: Cannot open auth.dat for writing.", __FUNCTION__ );
      perror( AUTH_FILE );
      return;
   }

   for( alist = authlist.begin(  ); alist != authlist.end(  ); ++alist )
   {
      auth_data *auth = *alist;

      stream << "#AUTH" << endl;
      stream << "Name     " << auth->name << endl;
      stream << "State    " << auth->state << endl;
      if( !auth->authed_by.empty(  ) )
         stream << "AuthedBy " << auth->authed_by << endl;
      if( !auth->change_by.empty(  ) )
         stream << "Change   " << auth->change_by << endl;
      stream << "End" << endl << endl;
   }
   stream.close(  );
}

void clear_auth_list( void )
{
   list < auth_data * >::iterator auth;

   for( auth = authlist.begin(  ); auth != authlist.end(  ); )
   {
      auth_data *nauth = *auth;
      ++auth;

      if( !exists_player( nauth->name ) )
         deleteptr( nauth );
   }
   save_auth_list(  );
}

void load_auth_list( void )
{
   ifstream stream;
   auth_data *auth;

   authlist.clear(  );

   stream.open( AUTH_FILE );
   if( !stream.is_open(  ) )
   {
      bug( "%s: Cannot open auth.dat", __FUNCTION__ );
      return;
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

      if( key == "#AUTH" )
         auth = new auth_data;
      else if( key == "Name" )
         auth->name = value;
      else if( key == "State" )
      {
         auth->state = atoi( value.c_str(  ) );

         if( auth->state == AUTH_ONLINE || auth->state == AUTH_LINK_DEAD )
            /*
             * Crash proofing. Can't be online when  booting up. Would suck for do_auth 
             */
            auth->state = AUTH_OFFLINE;
      }
      else if( key == "AuthedBy" )
         auth->authed_by = value;
      else if( key == "Change" )
         auth->change_by = value;
      else if( key == "End" )
         authlist.push_back( auth );
      else
         log_printf( "%s: Bad line in auth.dat file: %s %s", __FUNCTION__, key.c_str(  ), value.c_str(  ) );
   }
   while( !stream.eof(  ) );
   stream.close(  );

   clear_auth_list(  );
}

int get_auth_state( char_data * ch )
{
   list < auth_data * >::iterator namestate;
   int state;

   state = AUTH_AUTHED;

   for( namestate = authlist.begin(  ); namestate != authlist.end(  ); ++namestate )
   {
      auth_data *auth = *namestate;

      if( !str_cmp( auth->name, ch->name ) )
         return auth->state;
   }
   return state;
}

auth_data *get_auth_name( const string & name )
{
   list < auth_data * >::iterator mname;

   for( mname = authlist.begin(  ); mname != authlist.end(  ); ++mname )
   {
      auth_data *auth = *mname;

      if( !str_cmp( auth->name, name ) )  /* If the name is already in the list, break */
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
      new_name->name = ch->name;
      new_name->state = AUTH_ONLINE;   /* Just entered the game */
      authlist.push_back( new_name );
      save_auth_list(  );
   }
}

void remove_from_auth( const string & name )
{
   auth_data *old_name;

   if( !( old_name = get_auth_name( name ) ) )  /* Its not old */
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
                  "No titles, descriptive words, or names close to any existing\r\n" "Immortal's name. See 'help name'.\r\n", ch->name );
   }
   else if( old_auth->state == AUTH_AUTHED )
   {
      ch->pcdata->authed_by.clear(  );
      if( !old_auth->authed_by.empty(  ) )
      {
         ch->pcdata->authed_by = old_auth->authed_by;
         old_auth->authed_by.clear(  );
      }
      else
         ch->pcdata->authed_by = "The Code";

      ch->printf( "\r\n&GThe MUD Administrators have accepted the name %s.\r\nYou are now free to roam %s.\r\n", ch->name, sysdata->mud_name.c_str(  ) );
      ch->unset_pcflag( PCFLAG_UNAUTHED );
      remove_from_auth( ch->name );
   }
}

/* 
 * Check if the name prefix uniquely identifies a char descriptor
 */
char_data *get_waiting_desc( char_data * ch, const string & name )
{
   list < descriptor_data * >::iterator ds;
   char_data *ret_char = NULL;
   static size_t number_of_hits;

   number_of_hits = 0;
   for( ds = dlist.begin(  ); ds != dlist.end(  ); ++ds )
   {
      descriptor_data *d = *ds;

      if( d->character && ( !str_prefix( name, d->character->name ) ) && IS_WAITING_FOR_AUTH( d->character ) )
      {
         if( ++number_of_hits > 1 )
         {
            ch->printf( "%s does not uniquely identify a char.\r\n", name.c_str(  ) );
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
   string arg1;
   char_data *victim = NULL;
   list < auth_data * >::iterator auth;
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
   if( arg1.empty(  ) )
   {
      auth_data *au;

      ch->print( "To approve a waiting character: auth <name>\r\n" );
      ch->print( "To deny a waiting character:    auth <name> reject\r\n" );
      ch->print( "To ask a waiting character to change names: auth <name> change\r\n" );
      ch->print( "To have the code verify the list: auth fixlist\r\n" );
      ch->print( "To have the code purge inactive entries: auth clean\r\n" );

      ch->print( "\r\n&[divider]--- Characters awaiting approval ---\r\n" );

      for( auth = authlist.begin(  ); auth != authlist.end(  ); ++auth )
      {
         au = *auth;

         if( au->state == AUTH_CHANGE_NAME )
            changename = true;
         else if( au->state == AUTH_AUTHED )
            authed = true;

         if( !au->name.empty(  ) && au->state < AUTH_CHANGE_NAME )
            pending = true;
      }
      if( pending )
      {
         for( auth = authlist.begin(  ); auth != authlist.end(  ); ++auth )
         {
            au = *auth;

            if( au->state < AUTH_CHANGE_NAME )
            {
               switch ( au->state )
               {
                  default:
                     ch->printf( "\t%s\t\tUnknown?\r\n", au->name.c_str(  ) );
                     break;
                  case AUTH_LINK_DEAD:
                     ch->printf( "\t%s\t\tLink Dead\r\n", au->name.c_str(  ) );
                     break;
                  case AUTH_ONLINE:
                     ch->printf( "\t%s\t\tOnline\r\n", au->name.c_str(  ) );
                     break;
                  case AUTH_OFFLINE:
                     ch->printf( "\t%s\t\tOffline\r\n", au->name.c_str(  ) );
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
         for( auth = authlist.begin(  ); auth != authlist.end(  ); ++auth )
         {
            au = *auth;

            if( au->state == AUTH_AUTHED )
               ch->printf( "Name: %s\t Approved by: %s\r\n", au->name.c_str(  ), au->authed_by.c_str(  ) );
         }
      }
      if( changename )
      {
         ch->print( "\r\n&[divider]Change Name:\r\n" );
         ch->print( "---------------------------------------------\r\n" );
         for( auth = authlist.begin(  ); auth != authlist.end(  ); ++auth )
         {
            au = *auth;

            if( au->state == AUTH_CHANGE_NAME )
               ch->printf( "Name: %s\t Change requested by: %s\r\n", au->name.c_str(  ), au->change_by.c_str(  ) );
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
         if( argument.empty(  ) || !str_cmp( argument, "accept" ) || !str_cmp( argument, "yes" ) )
         {
            nauth->state = AUTH_AUTHED;
            nauth->authed_by = ch->name;
            save_auth_list(  );
            log_printf_plus( LOG_AUTH, level, "%s: authorized", nauth->name.c_str(  ) );
            ch->printf( "You have authorized %s.\r\n", nauth->name.c_str(  ) );
            return;
         }
         else if( !str_cmp( argument, "reject" ) )
         {
            log_printf_plus( LOG_AUTH, level, "%s: denied authorization", nauth->name.c_str(  ) );
            ch->printf( "You have denied %s.\r\n", nauth->name.c_str(  ) );
            /*
             * Addition so that denied names get added to reserved list - Samson 10-18-98 
             */
            funcf( ch, do_reserve, "%s add", nauth->name.c_str(  ) );
            do_destroy( ch, nauth->name );
            return;
         }
         else if( !str_cmp( argument, "change" ) )
         {
            nauth->state = AUTH_CHANGE_NAME;
            nauth->change_by = ch->name;
            save_auth_list(  );
            log_printf_plus( LOG_AUTH, level, "%s: name denied", nauth->name.c_str(  ) );
            ch->printf( "You requested %s change names.\r\n", nauth->name.c_str(  ) );
            /*
             * Addition so that requested name changes get added to reserved list - Samson 10-18-98 
             */
            funcf( ch, do_reserve, "%s add", nauth->name.c_str(  ) );
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
         if( argument.empty(  ) || !str_cmp( argument, "accept" ) || !str_cmp( argument, "yes" ) )
         {
            victim->pcdata->authed_by = ch->name;
            log_printf_plus( LOG_AUTH, level, "%s: authorized", victim->name );

            ch->printf( "You have authorized %s.\r\n", victim->name );

            victim->printf( "\r\n&GThe MUD Administrators have accepted the name %s.\r\n" "You are now free to roam the %s.\r\n", victim->name, sysdata->mud_name.c_str(  ) );
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
            nauth->change_by = ch->name;
            save_auth_list(  );
            log_printf_plus( LOG_AUTH, level, "%s: name denied", victim->name );
            victim->printf( "&R\r\nThe MUD Administrators have found the name %s to be unacceptable.\r\n"
                            "You may choose a new name when you reach the end of this area.\r\n"
                            "The name you choose must be medieval and original.\r\n"
                            "No titles, descriptive words, or names close to any existing\r\n" "Immortal's name. See 'help name'.\r\n", victim->name );
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
      ch->print( "No such player pending authorization.\r\n" );
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

   strlower( argument );
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

   list < char_data * >::iterator ich;
   for( ich = charlist.begin(  ); ich != charlist.end(  ); ++ich )
   {
      char_data *tmp = *ich;

      if( !str_cmp( argument, tmp->name ) )
      {
         ch->print( "That name is already taken. Please choose another.\r\n" );
         return;
      }
   }

   char fname[256];
   struct stat fst;
   snprintf( fname, 256, "%s%c/%s", PLAYER_DIR, tolower( argument[0] ), capitalize( argument ).c_str(  ) );
   if( stat( fname, &fst ) != -1 )
   {
      ch->print( "That name is already taken. Please choose another.\r\n" );
      return;
   }
   snprintf( fname, 256, "%s%c/%s", PLAYER_DIR, tolower( ch->name[0] ), capitalize( ch->name ) );
   unlink( fname );  /* cronel, for auth */

   STRFREE( ch->name );
   ch->name = STRALLOC( argument.c_str(  ) );
   STRFREE( ch->pcdata->filename );
   ch->pcdata->filename = STRALLOC( argument.c_str(  ) );
   ch->print( "Your name has been changed and is being submitted for approval.\r\n" );
   auth_name->name = argument;
   auth_name->state = AUTH_ONLINE;
   auth_name->change_by.clear(  );
   save_auth_list(  );
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

   if( argument.empty(  ) )
   {
      progbugf( ch, "%s", "Mpapplyb - bad syntax" );
      return;
   }

   if( !( victim = ch->get_char_room( argument ) ) )
   {
      progbugf( ch, "Mpapplyb - no such player %s in room.", argument.c_str(  ) );
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
      log_printf_plus( LOG_AUTH, level, "%s [%s] New player entering the game.\r\n", victim->name, victim->desc->host.c_str(  ) );
      victim->
         printf( "\r\nYou are now entering the game...\r\n"
                 "However, your character has not been authorized yet and can not\r\n"
                 "advance past level 5 until then. Your character will be saved,\r\n" "but not allowed to fully indulge in %s.\r\n", sysdata->mud_name.c_str(  ) );
   }
}

/* changed for new auth */
void auth_update( void )
{
   list < auth_data * >::iterator auth;
   char buf[MIL], lbuf[MSL];
   int level;
   bool found_imm = false; /* Is at least 1 immortal on? */
   bool found_hit = false; /* was at least one found? */

   if( ( level = check_command_level( "authorize", MAX_LEVEL ) ) == -1 )
      level = LEVEL_IMMORTAL;

   mudstrlcpy( lbuf, "--- Characters awaiting approval ---\r\n", MSL );
   for( auth = authlist.begin(  ); auth != authlist.end(  ); ++auth )
   {
      auth_data *au = *auth;

      if( au->state < AUTH_CHANGE_NAME )
      {
         found_hit = true;
         snprintf( buf, MIL, "Name: %s      Status: %s\r\n", au->name.c_str(  ), ( au->state == AUTH_ONLINE ) ? "Online" : "Offline" );
         mudstrlcat( lbuf, buf, MSL );
      }
   }

   if( found_hit )
   {
      list < descriptor_data * >::iterator ds;

      for( ds = dlist.begin(  ); ds != dlist.end(  ); ++ds )
      {
         descriptor_data *d = *ds;

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
   string arg;

   argument = one_argument( argument, arg );

   if( arg.empty(  ) )
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
      snprintf( buf, MSL, "grep -i -x %s ../system/reserved.lst > /dev/null", arg.c_str(  ) );

      if( system( buf ) == 0 )
      {
         ch->printf( "%s is already a reserved name.\r\n" );
         return;
      }

      append_to_file( RESERVED_LIST, "%s", arg.c_str(  ) );
      ch->print( "Name reserved.\r\n" );
      return;
   }
   ch->print( "Invalid argument.\r\n" );
}
