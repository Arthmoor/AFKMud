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
 *                            Ban module by Shaddai                         *
 ****************************************************************************/

#include "mud.h"
#include "ban.h"
#include "descriptor.h"

/* Global Variables */
list<ban_data*> banlist;

ban_data::ban_data()
{
   init_memory( &name, &suffix, sizeof( suffix ) );
}

ban_data::~ban_data()
{
   DISPOSE( name );
   DISPOSE( ban_time );
   DISPOSE( note );
   DISPOSE( user );
   STRFREE( ban_by );
   banlist.remove( this );
}

bool check_expire( ban_data * pban )
{
   if( pban->unban_date < 0 )
      return false;

   if( pban->unban_date <= current_time )
   {
      log_printf( "%s ban has expired.", pban->name );
      return true;
   }
   return false;
}

void free_bans( void )
{
   list<ban_data*>::iterator ban;

   for( ban = banlist.begin(); ban != banlist.end(); )
   {
      ban_data *pban = (*ban);
      ++ban;

      deleteptr( pban );
   }
   return;
}

/*
 * Load up one Class or one race ban structure.
 */
void fread_ban( FILE * fp )
{
   ban_data *pban = new ban_data;

   pban->name = fread_string_nohash( fp );
   pban->user = NULL;
   pban->level = fread_number( fp );
   pban->duration = fread_number( fp );
   pban->unban_date = fread_number( fp );
   pban->prefix = fread_number( fp );
   pban->suffix = fread_number( fp );
   pban->warn = fread_number( fp );
   pban->ban_by = fread_string( fp );
   pban->ban_time = fread_string_nohash( fp );
   pban->note = fread_string_nohash( fp );

   for( unsigned int i = 0; i < strlen( pban->name ); ++i )
   {
      if( pban->name[i] == '@' )
      {
         char *temp;
         char *temp2;

         temp = str_dup( pban->name );
         temp[i] = '\0';
         temp2 = &pban->name[i + 1];
         DISPOSE( pban->name );
         pban->name = str_dup( temp2 );
         pban->user = str_dup( temp );
         DISPOSE( temp );
         break;
      }
   }
   banlist.push_back( pban );
   return;
}

/*
 * Load all those nasty bans up :)
 * 	Shaddai
 */
void load_banlist( void )
{
   FILE *fp;

   banlist.clear();

   if( !( fp = fopen( SYSTEM_DIR BAN_LIST, "r" ) ) )
   {
      bug( "%s: Cannot open %s", __FUNCTION__, BAN_LIST );
      perror( BAN_LIST );
      return;
   }

   for( ;; )
   {
      const char *word = ( feof( fp ) ? "END" : fread_word( fp ) );

      if( word[0] == '\0' )
      {
         bug( "%s: EOF encountered reading banlist!", __FUNCTION__ );
         word = "END";
      }

      switch ( UPPER( word[0] ) )
      {
         default:
            bug( "%s: no match: %s", __FUNCTION__, word );
            fread_to_eol( fp );
            break;

         case 'E':
            if( !str_cmp( word, "END" ) ) /* File should always contain END */
            {
               FCLOSE( fp );
               return;
            }
         case 'S':
            if( !str_cmp( word, "SITE" ) )
               fread_ban( fp );
            break;
      }
   }  /* End of for loop */
}

/*
 * Saves all bans, for sites, classes and races.
 * 	Shaddai
 */
void save_banlist( void )
{
   FILE *fp;

   if( !( fp = fopen( SYSTEM_DIR BAN_LIST, "w" ) ) )
   {
      bug( "%s: Cannot open %s", __FUNCTION__, BAN_LIST );
      perror( BAN_LIST );
      return;
   }

   /*
    * Print out all the site bans 
    */
   list<ban_data*>::iterator iban;
   for( iban = banlist.begin(); iban != banlist.end(); ++iban )
   {
      ban_data *ban = (*iban);

      fprintf( fp, "%s", "SITE\n" );
      if( ban->user )
         fprintf( fp, "%s@%s~\n", ban->user, ban->name );
      else
         fprintf( fp, "%s~\n", ban->name );
      fprintf( fp, "%d %d %d %d %d %d\n", ban->level, ban->duration,
               ban->unban_date, ban->prefix, ban->suffix, ban->warn );
      fprintf( fp, "%s~\n%s~\n%s~\n", ban->ban_by, ban->ban_time, ban->note );
   }
   fprintf( fp, "%s", "END\n" ); /* File must have an END even if empty */
   FCLOSE( fp );
   return;
}

/*
 *  This actually puts the new ban into the proper linked list and
 *  initializes its data.  Shaddai
 */
/* ban <address> <type> <duration> */
void add_ban( char_data * ch, char *arg1, char *arg2, int bantime )
{
   ban_data *pban;
   struct tm *tms;
   char *name;
   int level;
   bool prefix = false, suffix = false, user_name = false;
   char *temp_host = NULL, *temp_user = NULL;

   /*
    * Should we check to see if they have dropped link sometime in between 
    * * writing the note and now?  Not sure but for right now we won't since
    * * do_ban checks for that.  Shaddai
    */
   switch ( ch->substate )
   {
      default:
         bug( "%s: illegal substate: %d", __FUNCTION__, ch->substate );
         return;

      case SUB_RESTRICTED:
         ch->print( "You cannot use this command from within another command.\r\n" );
         return;

      case SUB_NONE:
      {
         smash_tilde( arg1 ); /* Make sure the immortals don't put a ~ in it. */

         if( arg1[0] == '\0' || arg2[0] == '\0' )
            return;

         if( is_number( arg2 ) )
         {
            level = atoi( arg2 );
            if( level < 0 || level > LEVEL_SUPREME )
            {
               ch->printf( "Level range is from 0 to %d.\r\n", LEVEL_SUPREME );
               return;
            }
         }
         else if( !str_cmp( arg2, "all" ) )
            level = LEVEL_SUPREME;
         else if( !str_cmp( arg2, "newbie" ) )
            level = 1;
         else if( !str_cmp( arg2, "mortal" ) )
            level = LEVEL_AVATAR;
         else if( !str_cmp( arg2, "warn" ) )
            level = BAN_WARN;
         else
         {
            bug( "%s: Bad string for flag: %s", __FUNCTION__, arg2 );
            return;
         }

         for( unsigned int x = 0; x < strlen( arg1 ); ++x )
         {
            if( arg1[x] == '@' )
            {
               user_name = true;
               temp_host = str_dup( &arg1[x + 1] );
               arg1[x] = '\0';
               temp_user = str_dup( arg1 );
               break;
            }
         }
         if( !user_name )
            name = arg1;
         else
            name = temp_host;

         if( !name ) /* Double check to make sure name isnt null */
         {
            /* Free this stuff if its there */
            if( user_name )
            {
               DISPOSE( temp_host );
               DISPOSE( temp_user );
            }
            ch->print( "Name was null.\r\n" );
            return;
         }

         if( name[0] == '*' )
         {
            prefix = true;
            ++name;
         }

         if( name[strlen( name ) - 1] == '*' )
         {
            suffix = true;
            name[strlen( name ) - 1] = '\0';
         }

         list<ban_data*>::iterator temp;
         for( temp = banlist.begin(); temp != banlist.end(); ++temp )
         {
            ban_data *ban = (*temp);

            if( !str_cmp( ban->name, name ) )
            {
               if( ban->level == level && ( prefix && ban->prefix )
                   && ( suffix && ban->suffix ) && ( !user_name || ( user_name && !str_cmp( ban->user, temp_user ) ) ) )
               {
                  /* Free this stuff if its there */
                  if( user_name )
                  {
                     DISPOSE( temp_host );
                     DISPOSE( temp_user );
                  }
                  ch->print( "That entry already exists.\r\n" );
                  return;
               }
               else
               {
                  ban->suffix = suffix;
                  ban->prefix = prefix;
                  if( ban->level == BAN_WARN )
                     ban->warn = true;
                  ban->level = level;
                  strdup_printf( &ban->ban_time, "%24.24s", c_time( current_time, -1 ) );
                  STRFREE( ban->ban_by );
                  if( user_name )
                  {
                     DISPOSE( temp_host );
                     DISPOSE( temp_user );
                  }
                  ban->ban_by = QUICKLINK( ch->name );
                  ch->print( "Updated entry.\r\n" );
                  save_banlist(  );
                  return;
               }
            }
         }
         pban = new ban_data;
         pban->ban_by = QUICKLINK( ch->name );
         pban->suffix = suffix;
         pban->prefix = prefix;
         pban->name = str_dup( name );
         pban->level = level;
         if( user_name )
         {
            pban->user = str_dup( temp_user );
            DISPOSE( temp_host );
            DISPOSE( temp_user );
         }
         banlist.push_back( pban );
         strdup_printf( &pban->ban_time, "%24.24s", c_time( current_time, -1 ) );
         if( bantime > 0 )
         {
            pban->duration = bantime;
            tms = localtime( &current_time );
            tms->tm_mday += bantime;
            pban->unban_date = mktime( tms );
         }
         else
         {
            pban->duration = -1;
            pban->unban_date = -1;
         }
         if( pban->level == BAN_WARN )
            pban->warn = true;
         ch->substate = SUB_BAN_DESC;
         ch->pcdata->dest_buf = pban;
         if( !pban->note )
            pban->note = str_dup( "" );
         ch->start_editing( pban->note );
         ch->set_editor_desc( "A ban description." );
         return;
      }

      case SUB_BAN_DESC:
         pban = ( ban_data * ) ch->pcdata->dest_buf;
         if( !pban )
         {
            bug( "%s: sub_ban_desc: NULL ch->pcdata->dest_buf", __FUNCTION__ );
            ch->substate = SUB_NONE;
            return;
         }
         DISPOSE( pban->note );
         pban->note = ch->copy_buffer( false );
         ch->stop_editing(  );
         ch->substate = ch->tempnum;
         save_banlist(  );
         if( pban->duration > 0 )
         {
            if( !pban->user )
               ch->printf( "%s banned for %d days.\r\n", pban->name, pban->duration );
            else
               ch->printf( "%s@%s banned for %d days.\r\n", pban->user, pban->name, pban->duration );
         }
         else
         {
            if( !pban->user )
               ch->printf( "%s banned forever.\r\n", pban->name );
            else
               ch->printf( "%s@%s banned forever.\r\n", pban->user, pban->name );
         }
         return;

      case SUB_EDIT_ABORT:
         ch->substate = SUB_NONE;
         ch->print( "Aborting ban note.\r\n" );
         return;
   }
}

/*
 * Print the bans out to the screen.  Shaddai
 */

void show_bans( char_data * ch )
{
   list<ban_data*>::iterator iban;
   int bnum = 1;

   ch->set_pager_color( AT_IMMORT );

   ch->pager( "Banned sites:\r\n" );
   ch->pager( "[ #] Warn (Lv) Time                      By              For    Site\r\n" );
   ch->pager( "---- ---- ----- ------------------------ --------------- ----   ---------------\r\n" );
   ch->set_pager_color( AT_PLAIN );
   for( iban = banlist.begin(); iban != banlist.end(); ++iban, ++bnum )
   {
      ban_data *ban = (*iban);

      if( !ban->user )
         ch->pagerf( "[%2d] %-4s (%3d) %-24s %-15s %4d  %c%s%c\r\n",
                     bnum, ( ban->warn ) ? "YES" : "no", ban->level, ban->ban_time, ban->ban_by, ban->duration,
                     ( ban->prefix ) ? '*' : ' ', ban->name, ( ban->suffix ) ? '*' : ' ' );
      else
         ch->pagerf( "[%2d] %-4s (%2d) %-24s %-15s %4d  %s@%c%s%c\r\n",
                     bnum, ( ban->warn ) ? "YES" : "no", ban->level, ban->ban_time, ban->ban_by, ban->duration,
                     ban->user, ( ban->prefix ) ? '*' : ' ', ban->name, ( ban->suffix ) ? '*' : ' ' );
   }
   return;
}

/*
 * The main command for ban, lots of arguments so be carefull what you
 * change here.		Shaddai
 */
/* ban <address> <type> <duration> */
CMDF( do_ban )
{
   char arg1[MIL], arg2[MIL], *temp;
   int value = 0, bantime = -1;

   if( ch->isnpc(  ) ) /* Don't want mobs banning sites ;) */
   {
      ch->print( "Monsters are too dumb to do that!\r\n" );
      return;
   }

   if( !ch->desc )   /* No desc means no go :) */
   {
      bug( "%s: no descriptor", __FUNCTION__ );
      return;
   }

   ch->set_color( AT_IMMORT );
   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );

   /*
    * Do we have a time duration for the ban? 
    */
   if( argument && argument[0] != '\0' && is_number( argument ) )
      bantime = atoi( argument );
   else
      bantime = -1;

   /*
    * -1 is default, but no reason the time should be greater than 1000
    * * or less than 1, after all if it is greater than 1000 you are talking
    * * around 3 years.
    */
   if( bantime != -1 && ( bantime < 1 || bantime > 1000 ) )
   {
      ch->print( "Time value is -1 (forever) or from 1 to 1000.\r\n" );
      return;
   }

   /*
    * Need to be carefull with sub-states or everything will get messed up.
    */
   switch ( ch->substate )
   {
      default:
         bug( "%s: illegal substate: %d", __FUNCTION__, ch->substate );
         return;

      case SUB_RESTRICTED:
         ch->print( "You cannot use this command from within another command.\r\n" );
         return;

      case SUB_NONE:
         ch->tempnum = SUB_NONE;
         break;

         /*
          * Returning to end the editing of the note 
          */
      case SUB_BAN_DESC:
         add_ban( ch, "", "", 0 );
         return;
   }

   if( arg1[0] == '\0' )
   {
      show_bans( ch );
      ch->print( "Syntax: ban <address> <type> <duration>\r\n" );
      ch->print( "Syntax: ban show <number>\r\n" );
      ch->print( "    No arguments lists the current bans.\r\n" );
      ch->print( "    Duration is how long the ban lasts in days.\r\n" );
      ch->print( "    Type is one of the following:\r\n" );
      ch->print( "      Newbie: Only new characters are banned.\r\n" );
      ch->print( "      Mortal: All mortals, including avatars, are banned.\r\n" );
      ch->print( "      All:    ALL players, including immortals, are banned.\r\n" );
      ch->print( "      Warn:   Simply warns when someone logs on from the site.\r\n" );
      ch->print( "       Or type can be a level.\r\n" );
      return;
   }

   /*
    * If no args are sent after the Class/site/race, show the current banned
    * * items.  Shaddai
    */

   /*
    * Modified by Samson - We only ban site on Alsherok, not races and classes.
    * Not even sure WHY the smaugers felt this necessary. 
    */
   if( !str_cmp( arg1, "show" ) )
   {
      /*
       * This will show the note attached to a ban 
       */
      if( arg2[0] == '\0' )
      {
         do_ban( ch, "" );
         return;
      }
      temp = arg2;

      if( arg2[0] == '#' ) /* Use #1 to show the first ban */
      {
         temp = arg2;
         ++temp;
         if( !is_number( temp ) )
         {
            ch->print( "Which ban # to show?\r\n" );
            return;
         }
         value = atoi( temp );
         if( value < 1 )
         {
            ch->print( "You must specify a number greater than 0.\r\n" );
            return;
         }
      }
      if( temp[0] == '*' )
         ++temp;
      if( temp[strlen( temp ) - 1] == '*' )
         temp[strlen( temp ) - 1] = '\0';

      ban_data *pban = NULL;
      list<ban_data*>::iterator iban;
      for( iban = banlist.begin(); iban != banlist.end(); ++iban )
      {
         ban_data *ban = (*iban);

         if( value == 1 || !str_cmp( ban->name, temp ) )
         {
            pban = ban;
            break;
         }
         else if( value > 1 )
            --value;
      }

      if( !pban )
      {
         ch->print( "No such ban.\r\n" );
         return;
      }
      ch->printf( "Banned by: %s\r\n", pban->ban_by );
      ch->print( pban->note );
      return;
   }

   if( arg1 != '\0' )
   {
      if( arg2[0] == '\0' )
      {
         do_ban( ch, "" );
         return;
      }
      add_ban( ch, arg1, arg2, bantime );
      return;
   }
   return;
}

/*
 * Allow a already banned site/Class or race.  Shaddai
 */
CMDF( do_allow )
{
   char arg1[MIL];
   char *temp = NULL;
   bool fMatch = false;
   int value = 0;

   if( ch->isnpc(  ) ) /* No mobs allowing sites */
   {
      ch->print( "Monsters are too dumb to do that!\r\n" );
      return;
   }

   if( !ch->desc )   /* No desc is a bad thing */
   {
      bug( "%s: no descriptor", __FUNCTION__ );
      return;
   }

   argument = one_argument( argument, arg1 );

   ch->set_color( AT_IMMORT );

   if( !arg1 || arg1[0] == '\0' )
   {
      ch->print( "Syntax: allow <address>\r\n" );
      return;
   }

   if( arg1[0] == '#' ) /* Use #1 to ban the first ban in the list specified */
   {
      temp = arg1;
      ++temp;
      if( !is_number( temp ) )
      {
         ch->print( "Which ban # to allow?\r\n" );
         return;
      }
      value = atoi( temp );
   }

   if( arg1[0] != '\0' )
   {
      if( !value )
      {
         if( strlen( arg1 ) < 2 )
         {
            ch->print( "You have to have at least 2 chars for a ban\r\n" );
            ch->print( "If you are trying to allow by number use #\r\n" );
            return;
         }

         temp = arg1;
         if( arg1[0] == '*' )
            ++temp;
         if( temp[strlen( temp ) - 1] == '*' )
            temp[strlen( temp ) - 1] = '\0';
      }

      list<ban_data*>::iterator iban;
      for( iban = banlist.begin(); iban != banlist.end(); )
      {
         ban_data *pban = (*iban);
         ++iban;

         /*
          * Need to make sure we dispose properly of the ban_data 
          * * Or memory problems will be created.
          * * Shaddai
          */
         if( value == 1 || !str_cmp( pban->name, temp ) )
         {
            fMatch = true;
            deleteptr( pban );
            break;
         }
         if( value > 1 )
            --value;
      }
   }
   if( fMatch )
   {
      save_banlist(  );
      ch->printf( "%s is now allowed.\r\n", arg1 );
   }
   else
      ch->printf( "%s was not banned.\r\n", arg1 );
   return;
}

/* 
 *  Sets the warn flag on bans.
 */
CMDF( do_warn )
{
   char arg1[MSL], arg2[MSL];
   char *name;
   int count = -1, bancount = 1;

   /*
    * Don't want mobs or link-deads doing this.
    */
   if( ch->isnpc(  ) )
   {
      ch->print( "Monsters are too dumb to do that!\r\n" );
      return;
   }

   if( !ch->desc )
   {
      bug( "%s: no descriptor", __FUNCTION__ );
      return;
   }

   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );

   if( arg1[0] == '\0' || arg2[0] == '\0' )
   {
      ch->print( "Syntax: warn site <field>\r\n" );
      ch->print( "Field is either #(ban_number) or the site.\r\n" );
      ch->print( "Example:  warn 1.2.3.4\r\n" );
      return;
   }

   if( arg2[0] == '#' )
   {
      name = arg2;
      ++name;
      if( !is_number( name ) )
      {
         do_warn( ch, "" );
         return;
      }
      count = atoi( name );
      if( count < 1 )
      {
         ch->print( "The number has to be above 0.\r\n" );
         return;
      }
   }

   list<ban_data*>::iterator iban;
   for( iban = banlist.begin(); iban != banlist.end(); ++bancount )
   {
      ban_data *pban = (*iban);
      ++iban;

      if( !str_cmp( pban->name, arg2 ) || bancount == count )
      {
         /*
          * If it is just a warn delete it, otherwise remove the warn flag. 
          */
         if( pban->warn )
         {
            if( pban->level == BAN_WARN )
            {
               deleteptr( pban );
               ch->print( "Warn has been deleted.\r\n" );
            }
            else
            {
               pban->warn = false;
               ch->print( "Warn turned off.\r\n" );
            }
         }
         else
         {
            pban->warn = true;
            ch->print( "Warn turned on.\r\n" );
         }
         save_banlist(  );
         return;
      }
   }
   ch->printf( "%s was not found in the ban list.\r\n", arg2 );
   return;
}

/*
 * Check for totally banned sites.  Need this because we don't have a
 * char struct yet.  Shaddai
 */
bool descriptor_data::check_total_bans(  )
{
   list<ban_data*>::iterator iban;
   char new_host[MSL];
   int i;

   for( i = 0; i < ( int )strlen( host ); ++i )
      new_host[i] = LOWER( host[i] );
   new_host[i] = '\0';

   for( iban = banlist.begin(); iban != banlist.end(); )
   {
      ban_data *pban = (*iban);
      ++iban;

      if( pban->level != LEVEL_SUPREME )
         continue;
      if( pban->prefix && pban->suffix && strstr( new_host, pban->name ) )
      {
         if( check_expire( pban ) )
         {
            deleteptr( pban );
            save_banlist(  );
            return false;
         }
         else
            return true;
      }
      /*
       *   Bug of switched checks noticed by Cronel
       */
      if( pban->suffix && !str_prefix( pban->name, new_host ) )
      {
         if( check_expire( pban ) )
         {
            deleteptr( pban );
            save_banlist(  );
            return false;
         }
         else
            return true;
      }
      if( pban->prefix && !str_suffix( pban->name, new_host ) )
      {
         if( check_expire( pban ) )
         {
            deleteptr( pban );
            save_banlist(  );
            return false;
         }
         else
            return true;
      }
      if( !str_cmp( pban->name, new_host ) )
      {
         if( check_expire( pban ) )
         {
            deleteptr( pban );
            save_banlist(  );
            return false;
         }
         else
            return true;
      }
   }
   return false;
}

/*
 * The workhose, checks for bans. Shaddai
 */
bool check_bans( char_data * ch )
{
   list<ban_data*>::iterator iban;
   char new_host[MSL];
   bool fMatch = false;
   int i;

   for( i = 0; i < ( int )( strlen( ch->desc->host ) ); ++i )
      new_host[i] = LOWER( ch->desc->host[i] );
   new_host[i] = '\0';

   for( iban = banlist.begin(); iban != banlist.end(); )
   {
      ban_data *pban = (*iban);
      ++iban;

      if( pban->prefix && pban->suffix && strstr( pban->name, new_host ) )
         fMatch = true;
      else if( pban->prefix && !str_suffix( pban->name, new_host ) )
         fMatch = true;
      else if( pban->suffix && !str_prefix( pban->name, new_host ) )
         fMatch = true;
      else if( !str_cmp( pban->name, new_host ) )
         fMatch = true;
      else
         fMatch = false;
      if( fMatch )
      {
         if( check_expire( pban ) )
         {
            deleteptr( pban );
            save_banlist(  );
            return false;
         }
         if( ch->level > pban->level )
         {
            if( pban->warn )
               log_printf( "%s logging in from site %s.", ch->name, ch->desc->host );
            return false;
         }
         else
            return true;
      }
   }
   return false;
}
