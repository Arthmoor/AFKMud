/****************************************************************************
 *                   ^     +----- |  / ^     ^ |     | +-\                  *
 *                  / \    |      | /  |\   /| |     | |  \                 *
 *                 /   \   +---   |<   | \ / | |     | |  |                 *
 *                /-----\  |      | \  |  v  | |     | |  /                 *
 *               /       \ |      |  \ |     | +-----+ +-/                  *
 ****************************************************************************
 * AFKMud Copyright 1997-2010 by Roger Libiez (Samson),                     *
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
 *                          Pfile Pruning Module                            *
 ****************************************************************************/

#include <fstream>
#include <dirent.h>
#include <sys/stat.h>
#include "mud.h"
#include "clans.h"
#include "deity.h"
#include "objindex.h"
#include "pfiles.h"

int num_quotes;   /* for quotes */
#define QUOTE_FILE "quotes.dat"

void prune_sales(  );
void remove_from_auth( const string & );
void rare_update(  );
void save_timedata(  );
void adjust_pfile( const string & );

/* Globals */
time_t new_pfile_time_t;
short num_pfiles; /* Count up number of pfiles */
time_t now_time;
short deleted = 0;
short pexempt = 0;
short days = 0;

struct quote_data
{
   quote_data(  );
   ~quote_data(  );

   string quote;
   int number;
};

list < quote_data * >quotelist;

quote_data::quote_data(  )
{
   number = 0;
}

quote_data::~quote_data(  )
{
   quotelist.remove( this );
}

void free_quotes( void )
{
   list < quote_data * >::iterator qt;

   for( qt = quotelist.begin(  ); qt != quotelist.end(  ); )
   {
      quote_data *quote = *qt;
      ++qt;

      deleteptr( quote );
   }
}

quote_data *get_quote( int q )
{
   list < quote_data * >::iterator iquote;

   for( iquote = quotelist.begin(  ); iquote != quotelist.end(  ); ++iquote )
   {
      quote_data *quote = *iquote;

      if( quote->number == q )
         return quote;
   }
   return NULL;
}

void save_quotes( void )
{
   ofstream stream;
   char filename[256];
   int q = 0;

   snprintf( filename, 256, "%s%s", SYSTEM_DIR, QUOTE_FILE );
   stream.open( filename );
   if( !stream.is_open(  ) )
   {
      bug( "%s: Unable to open quote file for writing!", __FUNCTION__ );
      perror( filename );
      return;
   }

   list < quote_data * >::iterator iquote;
   for( iquote = quotelist.begin(  ); iquote != quotelist.end(  ); ++iquote )
   {
      quote_data *quote = *iquote;

      stream << "Quote: " << quote->quote << "¢" << endl;
      quote->number = ++q;
   }
   stream.close(  );
   num_quotes = q;
}

/** Function: load_quotes
  * Descr   : Determines how many (if any) quote files are located within
  *           QUOTE_DIR, for later use by "do_quotes".
  * Returns : (void)
  * Syntax  : (none)
  * Written : v1.0 12/97
  * Author  : Gary McNickle <gary@dharvest.com>
  *
  * Completely rewritten by Samson so it can be OLC'able. 10-15-03
  */
void load_quotes( void )
{
   char filename[256];
   quote_data *quote;
   ifstream stream;

   quotelist.clear(  );
   num_quotes = 0;

   snprintf( filename, 256, "%s%s", SYSTEM_DIR, QUOTE_FILE );
   stream.open( filename );
   if( !stream.is_open(  ) )
   {
      perror( filename );
      return;
   }

   do
   {
      string line, key, value;
      char buf[MIL];

      stream >> key;

      strip_lspace( key );
      strip_lspace( value );

      if( key == "#QUOTE" )
      {
         stream >> key;
         stream.getline( buf, MIL, '~' );
         value = buf;

         quote = new quote_data;

         quote->quote = value;
         quote->number = ++num_quotes;
         quotelist.push_back( quote );
      }
      else if( key == "Quote:" )
      {
         stream.getline( buf, MIL, '¢' );
         value = buf;

         quote = new quote_data;

         quote->quote = value;
         quote->number = ++num_quotes;
         quotelist.push_back( quote );
      }
   }
   while( !stream.eof(  ) );
   stream.close(  );
}

string add_linebreak( const string & str )
{
   string newstr = str;

   if( newstr.empty(  ) )
      return "";

   string_replace( newstr, "~", "\r\n" );
   return newstr;
}

/** Function: do_quotes
  * Descr   : Outputs an ascii file "quote.#" to the player. The number (#)
  *           is determined at random, based on how many quote files are
  *           stored in the QUOTE_DIR directory.
  * Returns : (void)
  * Syntax  : (none)
  * Written : v1.0 12/97
  * Author  : Gary McNickle <gary@dharvest.com>
  *
  * Completely rewritten by Samson so it can be OLC'able. 10-15-03
  */
CMDF( do_quoteset )
{
   quote_data *quote;
   string arg;
   int q;

   if( argument.empty(  ) )
   {
      ch->print( "Usage: quoteset add <quote>\r\n" );
      ch->print( "Usage: quoteset remove <quote#>\r\n" );
      ch->print( "Usage: quoteset list\r\n" );
      ch->print( "\r\nTo add a line break, insert a tilde (~) to signify it.\r\n" );
      return;
   }

   if( !str_cmp( argument, "list" ) )
   {
      list < quote_data * >::iterator qt;

      if( num_quotes == 0 )
      {
         ch->print( "There are no quotes to list.\r\n" );
         return;
      }
      for( qt = quotelist.begin(  ); qt != quotelist.end(  ); ++qt )
      {
         quote = *qt;

         ch->pagerf( "&cQuote #%d:\r\n &W%s&D\r\n\r\n", quote->number, quote->quote.c_str(  ) );
      }
      return;
   }

   argument = one_argument( argument, arg );

   if( !str_cmp( arg, "add" ) )
   {
      if( argument.empty(  ) )
      {
         do_quoteset( ch, "" );
         return;
      }
      argument = add_linebreak( argument );
      quote = new quote_data;
      quote->quote = argument;
      quotelist.push_back( quote );
      save_quotes(  );
      ch->printf( "Quote #%d has been added to the list.\r\n", quote->number );
      return;
   }

   if( str_cmp( arg, "remove" ) || !is_number( argument ) )
   {
      do_quoteset( ch, "" );
      return;
   }

   q = atoi( argument.c_str(  ) );
   if( !( quote = get_quote( q ) ) )
   {
      ch->print( "No quote by that number exists!\r\n" );
      return;
   }
   deleteptr( quote );
   save_quotes(  );
   ch->printf( "Quote #%d has been removed from the list.\r\n", q );
}

void quotes( char_data * ch )
{
   quote_data *quote;
   int q;

   if( num_quotes == 0 )
      return;

   /*
    * Lamers! How can you not like quotes! 
    */
   if( ch->has_pcflag( PCFLAG_NOQUOTE ) )
      return;

   q = number_range( 1, num_quotes );
   quote = get_quote( q );
   if( !quote )
   {
      bug( "Missing quote #%d ?!?", q );
      return;
   }
   ch->printf( "&W\r\n%s&d\r\n", quote->quote.c_str(  ) );
}

CMDF( do_quotes )
{
   quotes( ch );
}

CMDF( do_pcrename )
{
   char_data *victim;
   string arg1;
   char newname[256], oldname[256];

   argument = one_argument( argument, arg1 );
   smash_tilde( argument );

   if( ch->isnpc(  ) )
      return;

   if( arg1.empty(  ) || argument.empty(  ) )
   {
      ch->print( "Syntax: rename <victim> <new name>\r\n" );
      return;
   }

   if( !check_parse_name( argument, true ) )
   {
      ch->print( "Illegal name.\r\n" );
      return;
   }

   /*
    * Just a security precaution so you don't rename someone you don't mean too --Shaddai 
    */
   if( !( victim = ch->get_char_room( arg1 ) ) )
   {
      ch->print( "That person is not in the room.\r\n" );
      return;
   }
   if( victim->isnpc(  ) )
   {
      ch->print( "You can't rename NPC's.\r\n" );
      return;
   }

   if( ch->get_trust(  ) < victim->get_trust(  ) )
   {
      ch->print( "I don't think they would like that!\r\n" );
      return;
   }
   snprintf( newname, 256, "%s%c/%s", PLAYER_DIR, tolower( argument[0] ), capitalize( argument ).c_str(  ) );
   snprintf( oldname, 256, "%s%c/%s", PLAYER_DIR, tolower( victim->pcdata->filename[0] ), capitalize( victim->pcdata->filename ) );

   if( access( newname, F_OK ) == 0 )
   {
      ch->print( "That name already exists.\r\n" );
      return;
   }

   /*
    * Have to remove the old god entry in the directories 
    */
   if( victim->is_immortal(  ) )
   {
      char godname[256];
      snprintf( godname, 256, "%s%s", GOD_DIR, capitalize( victim->pcdata->filename ) );
      remove( godname );
   }

   /*
    * Remember to change the names of the areas 
    */
   if( victim->pcdata->area )
   {
      char filename[256], newfilename[256];

      snprintf( filename, 256, "%s%s.are", BUILD_DIR, victim->name );
      snprintf( newfilename, 256, "%s%s.are", BUILD_DIR, capitalize( argument ).c_str(  ) );
      rename( filename, newfilename );
      snprintf( filename, 256, "%s%s.are.bak", BUILD_DIR, victim->name );
      snprintf( newfilename, 256, "%s%s.are.bak", BUILD_DIR, capitalize( argument ).c_str(  ) );
      rename( filename, newfilename );
   }

   /*
    * If they're in a clan/guild, remove them from the roster for it 
    */
   if( victim->pcdata->clan )
      remove_roster( victim->pcdata->clan, victim->name );

   STRFREE( victim->name );
   victim->name = STRALLOC( capitalize( argument ).c_str(  ) );
   STRFREE( victim->pcdata->filename );
   victim->pcdata->filename = STRALLOC( capitalize( argument ).c_str(  ) );
   if( remove( oldname ) )
   {
      log_printf( "Error: Couldn't delete file %s in do_rename.", oldname );
      ch->print( "Couldn't delete the old file!\r\n" );
   }

   /*
    * Time to save to force the affects to take place 
    */
   if( victim->pcdata->clan )
   {
      add_roster( victim->pcdata->clan, victim->name, victim->Class, victim->level, victim->pcdata->mkills, victim->pcdata->mdeaths );
      save_clan( victim->pcdata->clan );
   }
   victim->save(  );

   /*
    * Now lets update the wizlist 
    */
   if( victim->is_immortal(  ) )
      make_wizlist(  );
   ch->print( "Character was renamed.\r\n" );
}

void search_pfiles( char_data * ch, const char *dirname, const char *filename, int cvnum )
{
   FILE *fpChar;
   char fname[256];

   snprintf( fname, 256, "%s/%s", dirname, filename );
   if( !( fpChar = fopen( fname, "r" ) ) )
   {
      perror( fname );
      return;
   }

   for( ;; )
   {
      int vnum, nest = 0, counter = 1;
      bool done = false;

      char letter = fread_letter( fpChar );

      if( letter == '\0' )
      {
         log_printf( "%s: EOF encountered reading file: %s!", __FUNCTION__, fname );
         break;
      }

      if( letter != '#' )
         continue;

      const char *word = ( feof( fpChar ) ? "End" : fread_word( fpChar ) );

      if( word[0] == '\0' )
      {
         log_printf( "%s: EOF encountered reading file: %s!", __FUNCTION__, fname );
         word = "End";
      }

      if( !str_cmp( word, "End" ) )
         break;

      if( !str_cmp( word, "OBJECT" ) )
      {
         while( !done )
         {
            word = ( feof( fpChar ) ? "End" : fread_word( fpChar ) );

            if( word[0] == '\0' )
            {
               log_printf( "%s: EOF encountered reading file: %s!", __FUNCTION__, fname );
               word = "End";
            }

            switch ( UPPER( word[0] ) )
            {
               default:
                  fread_to_eol( fpChar );
                  break;

               case 'C':
                  KEY( "Count", counter, fread_number( fpChar ) );
                  break;

               case 'E':
                  if( !str_cmp( word, "End" ) )
                  {
                     done = true;
                     break;
                  }

               case 'N':
                  KEY( "Nest", nest, fread_number( fpChar ) );
                  break;

               case 'O':
                  if( !str_cmp( word, "Ovnum" ) )
                  {
                     vnum = fread_number( fpChar );
                     if( !( get_obj_index( vnum ) ) )
                     {
                        bug( "Bad obj vnum in %s: %d", __FUNCTION__, vnum );
                        adjust_pfile( filename );
                     }
                     else
                     {
                        if( vnum == cvnum )
                           ch->pagerf( "Player %s: Counted %d of Vnum %d.\r\n", filename, counter, cvnum );
                     }
                  }
                  break;
            }
         }
      }
   }
   FCLOSE( fpChar );
}

/* Scans the pfiles to count the number of copies of a vnum being stored - Samson 1-3-99 */
void check_stored_objects( char_data * ch, int cvnum )
{
   DIR *dp;
   struct dirent *dentry;
   char directory_name[100];
   int alpha_loop;

   for( alpha_loop = 0; alpha_loop <= 25; ++alpha_loop )
   {
      snprintf( directory_name, 100, "%s%c", PLAYER_DIR, 'a' + alpha_loop );
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
            search_pfiles( ch, directory_name, dentry->d_name, cvnum );
         dentry = readdir( dp );
      }
      closedir( dp );
   }
}

void fread_pfile( FILE * fp, time_t tdiff, const char *fname, bool count )
{
   char *name = NULL;
   char *clan = NULL;
   char *deity = NULL;
   short level = 0, file_ver = 0;
   bitset < MAX_PCFLAG > pact;

   pact.reset(  );

   for( ;; )
   {
      const char *word = ( feof( fp ) ? "End" : fread_word( fp ) );

      if( word[0] == '\0' )
      {
         bug( "%s: EOF encountered reading file!", __FUNCTION__ );
         word = "End";
      }

      if( !str_cmp( word, "End" ) )
         break;

      switch ( UPPER( word[0] ) )
      {
         default:   // Deliberately this way - the bug spam will kill you!
         case '*':
            fread_to_eol( fp );
            break;

         case 'C':
            KEY( "Clan", clan, fread_string( fp ) );
            break;

         case 'D':
            KEY( "Deity", deity, fread_string( fp ) );
            break;

         case 'N':
            KEY( "Name", name, fread_string( fp ) );
            break;

         case 'P':
            if( !str_cmp( word, "PCFlags" ) )
            {
               flag_set( fp, pact, pc_flags );
               break;
            }
            break;

         case 'S':
            if( !str_cmp( word, "Status" ) )
            {
               level = fread_number( fp );
               fread_to_eol( fp );
               break;
            }
            break;

         case 'V':
            KEY( "Version", file_ver, fread_number( fp ) );
            break;
      }
   }

   if( count == false && !pact.test( PCFLAG_EXEMPT ) )
   {
      if( level < 10 && tdiff > sysdata->newbie_purge )
      {
         if( unlink( fname ) == -1 )
            perror( "Unlink" );
         else
         {
            days = sysdata->newbie_purge;
            log_printf( "Player %s was deleted. Exceeded time limit of %d days.", name, days );
            remove_from_auth( name );
            if( clan != NULL )
            {
               clan_data *pclan = get_clan( clan );
               remove_roster( pclan, name );
            }
            ++deleted;
            STRFREE( clan );
            STRFREE( deity );
            STRFREE( name );
            return;
         }
      }

      if( level < LEVEL_IMMORTAL && tdiff > sysdata->regular_purge )
      {
         if( level < LEVEL_IMMORTAL )
         {
            if( unlink( fname ) == -1 )
               perror( "Unlink" );
            else
            {
               days = sysdata->regular_purge;
               log_printf( "Player %s was deleted. Exceeded time limit of %d days.", name, days );
               remove_from_auth( name );
               if( clan != NULL )
               {
                  clan_data *pclan = get_clan( clan );
                  remove_roster( pclan, name );
               }
               ++deleted;
               STRFREE( clan );
               STRFREE( deity );
               STRFREE( name );
               return;
            }
         }
      }
   }

   if( pact.test( PCFLAG_EXEMPT ) || level >= LEVEL_IMMORTAL )
      ++pexempt;

   if( clan != NULL )
   {
      clan_data *guild = get_clan( clan );

      if( guild )
         ++guild->members;
   }

   if( deity != NULL )
   {
      deity_data *god = get_deity( deity );

      if( god )
         ++god->worshippers;
   }
   STRFREE( clan );
   STRFREE( deity );
   STRFREE( name );
}

void read_pfile( const char *dirname, const char *filename, bool count )
{
   FILE *fp;
   char fname[256];
   struct stat fst;
   time_t tdiff;

   now_time = time( 0 );

   snprintf( fname, 256, "%s/%s", dirname, filename );

   if( stat( fname, &fst ) != -1 )
   {
      tdiff = ( now_time - fst.st_mtime ) / 86400;

      if( ( fp = fopen( fname, "r" ) ) != NULL )
      {
         for( ;; )
         {
            char letter;
            const char *word;

            letter = fread_letter( fp );

            if( ( letter != '#' ) && ( !feof( fp ) ) )
               continue;

            word = ( feof( fp ) ? "End" : fread_word( fp ) );

            if( word[0] == '\0' )
            {
               bug( "%s: EOF encountered reading file!", __FUNCTION__ );
               word = "End";
            }

            if( !str_cmp( word, "End" ) )
               break;

            if( !str_cmp( word, "PLAYER" ) )
               fread_pfile( fp, tdiff, fname, count );
            else if( !str_cmp( word, "END" ) )  /* Done */
               break;
         }
         FCLOSE( fp );
      }
   }
}

void pfile_scan( bool count )
{
   DIR *dp;
   struct dirent *dentry;
   char directory_name[100];
   deleted = 0;
   pexempt = 0;

   now_time = time( 0 );

   /*
    * Reset all clans to 0 members prior to scan - Samson 7-26-00 
    */
   list < clan_data * >::iterator cl;
   if( !count )
      for( cl = clanlist.begin(  ); cl != clanlist.end(  ); ++cl )
      {
         clan_data *clan = *cl;
         clan->members = 0;
      }

   /*
    * Reset all deities to 0 worshippers prior to scan - Samson 7-26-00 
    */
   list < deity_data * >::iterator ideity;
   if( !count )
      for( ideity = deitylist.begin(  ); ideity != deitylist.end(  ); ++ideity )
      {
         deity_data *deity = *ideity;
         deity->worshippers = 0;
      }

   short cou = 0;
   for( short alpha_loop = 0; alpha_loop <= 25; ++alpha_loop )
   {
      snprintf( directory_name, 100, "%s%c", PLAYER_DIR, 'a' + alpha_loop );

      // log_string( directory_name ); 

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
            if( !count )
               read_pfile( directory_name, dentry->d_name, count );
            ++cou;
         }
         dentry = readdir( dp );
      }
      closedir( dp );
   }

   if( !count )
      log_string( "Pfile cleanup completed." );
   else
      log_string( "Pfile count completed." );

   log_printf( "Total pfiles scanned: %d", cou );
   log_printf( "Total exempted pfiles: %d", pexempt );

   if( !count )
   {
      log_printf( "Total pfiles deleted: %d", deleted );
      log_printf( "Total pfiles remaining: %d", cou - deleted );
      num_pfiles = cou - deleted;
   }
   else
      num_pfiles = cou;

   if( !count )
   {
      for( cl = clanlist.begin(  ); cl != clanlist.end(  ); ++cl )
      {
         clan_data *clan = *cl;
         save_clan( clan );
      }
      for( ideity = deitylist.begin(  ); ideity != deitylist.end(  ); ++ideity )
      {
         deity_data *deity = *ideity;
         save_deity( deity );
      }
      verify_clans(  );
      prune_sales(  );
   }
}

CMDF( do_pfiles )
{
   char buf[512];

   if( ch->isnpc(  ) )
   {
      ch->print( "Mobs cannot use this command!\r\n" );
      return;
   }

   if( argument.empty(  ) )
   {
      /*
       * Makes a backup copy of existing pfiles just in case - Samson 
       */
      snprintf( buf, 512, "tar -cf %spfiles.tar %s*", PLAYER_DIR, PLAYER_DIR );

      /*
       * GAH, the shell pipe won't process the command that gets pieced
       * together in the preceeding lines! God only knows why. - Samson 
       */
      system( buf );

      log_printf( "Manual pfile cleanup started by %s.", ch->name );
      pfile_scan( false );
      rare_update(  );
      return;
   }

   if( !str_cmp( argument, "settime" ) )
   {
      new_pfile_time_t = current_time + 86400;
      save_timedata(  );
      ch->print( "New cleanup time set for 24 hrs from now.\r\n" );
      return;
   }

   if( !str_cmp( argument, "count" ) )
   {
      log_printf( "Pfile count started by %s.", ch->name );
      pfile_scan( true );
      return;
   }
   ch->print( "Invalid argument.\r\n" );
}

void check_pfiles( time_t reset )
{
   /*
    * This only counts them up on reboot if the cleanup isn't needed - Samson 1-2-00 
    */
   if( reset == 255 && new_pfile_time_t > current_time )
   {
      reset = 0;  /* Call me paranoid, but it might be meaningful later on */
      log_string( "Counting pfiles....." );
      pfile_scan( true );
      return;
   }

   if( new_pfile_time_t <= current_time )
   {
      if( sysdata->CLEANPFILES == true )
      {
         char buf[512];

         /*
          * Makes a backup copy of existing pfiles just in case - Samson 
          */
         snprintf( buf, 512, "tar -cf %spfiles.tar %s*", PLAYER_DIR, PLAYER_DIR );

         /*
          * Would use the shell pipe for this, but alas, it requires a ch in order
          * to work, this also gets called during boot_db before the rare item checks - Samson 
          */
         system( buf );

         new_pfile_time_t = current_time + 86400;
         save_timedata(  );
         log_string( "Automated pfile cleanup beginning...." );
         pfile_scan( false );
         if( reset == 0 )
            rare_update(  );
      }
      else
      {
         new_pfile_time_t = current_time + 86400;
         save_timedata(  );
         log_string( "Counting pfiles....." );
         pfile_scan( true );
         if( reset == 0 )
            rare_update(  );
      }
   }
}
