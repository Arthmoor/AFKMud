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
 * Original SMAUG 1.4a written by Thoric (Derek Snider) with Altrag,        *
 * Blodkai, Haus, Narn, Scryn, Swordbearer, Tricops, Gorog, Rennard,        *
 * Grishnakh, Fireblade, and Nivek.                                         *
 *                                                                          *
 * Original MERC 2.1 code by Hatchet, Furey, and Kahn.                      *
 *                                                                          *
 * Original DikuMUD code by: Hans Staerfeldt, Katja Nyboe, Tom Madsen,      *
 * Michael Seifert, and Sebastian Hammer.                                   *
 ****************************************************************************
 *                           Help System Module                             *
 ****************************************************************************/

#include <fstream>
#include <algorithm>
#include "mud.h"
#include "commands.h"
#include "help.h"
#include "raceclass.h"
#include "smaugaffect.h"

#define NOHELP_FILE SYSTEM_DIR "nohelp.txt"  /* For tracking missing help entries */

bool get_skill_help( char_data *, const string & );
int skill_number( const string & );

list < help_data * >helplist;

int top_help;

help_data::help_data(  )
{
   level = 0;
}

help_data::~help_data(  )
{
   if( !fBootDb )
      helplist.remove( this );
}

void free_helps( void )
{
   list < help_data * >::iterator pHelp;

   for( pHelp = helplist.begin(  ); pHelp != helplist.end(  ); )
   {
      help_data *help = *pHelp;
      ++pHelp;

      deleteptr( help );
   }
   top_help = 0;
}

/*
 * Adds a help page to the list if it is not a duplicate of an existing page.
 * Page is insert-sorted by keyword. - Thoric
 * (The reason for sorting is to keep do_hlist looking nice)
 */
void add_help( help_data * pHelp )
{
   list < help_data * >::iterator tHelp;
   bool added = false;

   for( tHelp = helplist.begin(  ); tHelp != helplist.end(  ); ++tHelp )
   {
      help_data *help = *tHelp;

      if( pHelp->level == help->level && !str_cmp( pHelp->keyword, help->keyword ) )
      {
         bug( "%s: duplicate: %s. Deleting.", __FUNCTION__, pHelp->keyword.c_str(  ) );
         deleteptr( pHelp );
         return;
      }
      else  // Yipee! Another crappy ass hack to get past the compiler with!
      {
         string pH, H;

         one_argument( pHelp->keyword, pH );
         one_argument( help->keyword, H );

         if( pH.compare( H ) >= 0 )
         {
            helplist.insert( tHelp, pHelp );
            added = true;
            break;
         }
      }
   }

   if( !added )
      helplist.push_back( pHelp );

   ++top_help;
}

void save_helps( void )
{
   ofstream stream;

   stream.open( HELP_FILE );

   if( !stream.is_open(  ) )
   {
      log_string( "Couldn't write to help file." );
      return;
   }

   list < help_data * >::iterator ihlp;
   for( ihlp = helplist.begin(  ); ihlp != helplist.end(  ); ++ihlp )
   {
      help_data *hlp = *ihlp;

      stream << "#HELP" << endl;
      stream << "Level    " << hlp->level << endl;
      stream << "Keywords " << hlp->keyword << endl;
      stream << "Text " << hlp->text << "¢" << endl;
      stream << "End" << endl << endl;
   }
   stream.close(  );
}

void load_helps( void )
{
   help_data *help;
   ifstream stream;

   helplist.clear(  );
   top_help = 0;

   stream.open( HELP_FILE );
   if( !stream.is_open(  ) )
   {
      log_printf( "No help file found." );
      return;
   }

   do
   {
      string key, value;
      char buf[MSL];

      stream >> key;
      strip_lspace( key );

      if( key.empty(  ) )
         continue;

      if( key == "#HELP" )
         help = new help_data;

      else if( key == "Keywords" )
      {
         stream.getline( buf, MSL );
         value = buf;
         strip_lspace( value );
         std::transform(value.begin(), value.end(), value.begin(), (int(*)(int)) toupper);
         help->keyword = value;
      }

      else if( key == "Level" )
      {
         stream.getline( buf, MSL );
         value = buf;
         strip_lspace( value );

         help->level = atoi( value.c_str(  ) );
      }

      else if( key == "Text" )
      {
         stream.getline( buf, MSL, '¢' );
         value = buf;
         strip_lspace( value );
         help->text = value;
      }

      else if( key == "End" )
      {
         helplist.push_back( help );
         ++top_help;
      }

      else
      {
         stream.getline( buf, MSL );
         value = buf;
         strip_lspace( value );
         log_printf( "Bad line in help file: %s %s", key.c_str(  ), value.c_str(  ) );
      }
   }
   while( !stream.eof(  ) );
   stream.close(  );
}

/*
 * Moved into a separate function so it can be used for other things
 * ie: online help editing - Thoric
 */
help_data *get_help( char_data * ch, string argument )
{
   string argall, argone, argnew;
   list < help_data * >::iterator hlp;
   help_data *pHelp;
   int lev;

   if( argument.empty(  ) )
      argument = "summary";

   if( isdigit( argument[0] ) )
   {
      lev = number_argument( argument, argnew );
      argument = argnew;
   }
   else
      lev = -2;

   /*
    * Tricky argument handling so 'help a b' doesn't match a.
    */
   argall = "";
   while( !argument.empty(  ) )
   {
      argument = one_argument( argument, argone );
      if( !argall.empty(  ) )
         argall.append( 1, ' ' );
      argall.append( argone );
   }
   std::transform(argall.begin(), argall.end(), argall.begin(), (int(*)(int)) toupper);

   for( hlp = helplist.begin(  ); hlp != helplist.end(  ); ++hlp )
   {
      pHelp = *hlp;

      if( pHelp->level > ch->get_trust(  ) )
         continue;
      if( lev != -2 && pHelp->level != lev )
         continue;

      if( hasname( pHelp->keyword, argall ) )
         return pHelp;
   }
   return NULL;
}

/*
 * Now this is cleaner
 */
/* Updated do_help command provided by Remcon of The Lands of Pabulum 03/20/2004 */
CMDF( do_help )
{
   help_data *pHelp;
   list < help_data * >::iterator tHelp;
   string keyword, oneword, lastmatch;
   short matched = 0, checked = 0, totalmatched = 0, found = 0;

   ch->set_pager_color( AT_HELP );

   if( argument.empty(  ) )
      argument = "summary";

   if( !( pHelp = get_help( ch, argument ) ) )
   {
      // Go look in skill help. If it's there, it gets displayed, and we leave.
      if( get_skill_help( ch, argument ) )
         return;

      ch->pagerf( "&wNo help on '%s' found.&D\r\n", argument.c_str(  ) );
      ch->pager( "&BSuggested Help Files:&D\r\n" );
      lastmatch = " ";

      for( tHelp = helplist.begin(  ); tHelp != helplist.end(  ); ++tHelp )
      {
         help_data *help = *tHelp;

         matched = 0;
         if( help->keyword.empty(  ) || help->level > ch->get_trust(  ) )
            continue;
         keyword = help->keyword;

         while( !keyword.empty(  ) )
         {
            matched = 0;   /* Set to 0 for each time we check lol */
            keyword = one_argument( keyword, oneword );
            /*
             * Lets check only up to 10 spots 
             */
            for( checked = 0; checked <= 10; ++checked )
            {
               if( !oneword[checked] || !argument[checked] )
                  break;
               if( LOWER( oneword[checked] ) == LOWER( argument[checked] ) )
                  ++matched;
            }
            if( ( matched > 1 && matched > ( checked / 2 ) ) || ( matched > 0 && checked < 2 ) )
            {
               ch->pagerf( "&G %-20s &D", oneword.c_str(  ) );
               if( ++found % 4 == 0 )
               {
                  found = 0;
                  ch->pager( "\r\n" );
               }
               lastmatch = oneword;
               ++totalmatched;
               break;
            }
         }
      }
      if( totalmatched == 0 )
      {
         ch->pager( "&C&GNo suggested help files.\r\n" );
         return;
      }
      if( totalmatched == 1 && !lastmatch.empty(  ) )
      {
         ch->pager( "&COpening only suggested helpfile.&D\r\n" );
         do_help( ch, lastmatch );
         return;
      }
      if( found > 0 && found <= 3 )
         ch->pager( "\r\n" );
      return;
   }

   ch->set_pager_color( AT_HELP );

   /*
    * Make newbies do a help start. --Shaddai 
    */
   if( !ch->isnpc(  ) && !str_cmp( argument, "start" ) )
      ch->set_pcflag( PCFLAG_HELPSTART );

   if( ch->is_immortal(  ) )
      ch->pagerf( "Help level: %d\r\n", pHelp->level );

   /*
    * Strip leading '.' to allow initial blanks.
    */
   if( pHelp->text[0] == '.' )
      ch->pager( pHelp->text.substr( 1, pHelp->text.length(  ) ) );
   else
      ch->pager( pHelp->text );
}

/*
 * Help editor - Thoric
 */
CMDF( do_hedit )
{
   help_data *pHelp;

   if( !ch->desc )
   {
      ch->print( "You have no descriptor.\r\n" );
      return;
   }

   switch ( ch->substate )
   {
      default:
         break;

      case SUB_HELP_EDIT:
         if( !( pHelp = ( help_data * ) ch->pcdata->dest_buf ) )
         {
            bug( "%s: sub_help_edit: NULL ch->pcdata->dest_buf", __FUNCTION__ );
            ch->stop_editing(  );
            return;
         }
         pHelp->text = ch->copy_buffer(  );
         ch->stop_editing(  );
         return;

      case SUB_EDIT_ABORT:
         ch->substate = SUB_NONE;
         ch->print( "Aborting helpfile.\r\n" );
         return;
   }

   if( !( pHelp = get_help( ch, argument ) ) )  /* new help */
   {
      list < help_data * >::iterator iHelp;
      string argnew, narg = argument;
      int lev;
      bool new_help = true;

      for( iHelp = helplist.begin(  ); iHelp != helplist.end(  ); ++iHelp )
      {
         help_data *help = *iHelp;

         if( !str_cmp( argument, help->keyword ) )
         {
            pHelp = help;
            new_help = false;
            break;
         }
      }

      if( new_help )
      {
         if( isdigit( narg[0] ) )
         {
            lev = number_argument( narg, argnew );
            narg = argnew;
         }
         else
            lev = 0;
         pHelp = new help_data;
         pHelp->keyword = narg;
         strupper( pHelp->keyword );
         pHelp->level = lev;
         add_help( pHelp );
      }
   }
   ch->substate = SUB_HELP_EDIT;
   ch->pcdata->dest_buf = pHelp;
   if( pHelp->text.empty(  ) )
      pHelp->text = "";
   ch->editor_desc_printf( "Help topic, keyword '%s', level %d.", pHelp->keyword.c_str(  ), pHelp->level );
   ch->start_editing( pHelp->text );
}

/*
 * Stupid leading space muncher fix - Thoric
 */
const char *help_fix( char *text )
{
   string fixed;

   if( !text )
      return "";
   fixed = strip_cr( text );
   if( fixed[0] == ' ' )
      fixed[0] = '.';
   return fixed.c_str(  );
}

CMDF( do_hset )
{
   help_data *pHelp;
   string arg1, arg2;

   smash_tilde( argument );
   argument = one_argument( argument, arg1 );
   if( arg1.empty(  ) )
   {
      ch->print( "Syntax: hset <field> [value] [help page]\r\r\n\n" );
      ch->print( "Field being one of:\r\n" );
      ch->printf( "  level keyword remove save%s\r\n", ch->is_imp(  )? " reload" : "" );
      return;
   }

   if( !str_cmp( arg1, "reload" ) && ch->is_imp(  ) )
   {
      log_string( "Unloading existing help files." );
      free_helps(  );
      log_string( "Reloading help files." );
      load_helps(  );
      ch->print( "Help files reloaded.\r\n" );
      return;
   }

   if( !str_cmp( arg1, "save" ) )
   {
      save_helps(  );
      ch->print( "Saved.\r\n" );
      return;
   }

   if( str_cmp( arg1, "remove" ) )
      argument = one_argument( argument, arg2 );

   if( !( pHelp = get_help( ch, argument ) ) )
   {
      ch->print( "Cannot find help on that subject.\r\n" );
      return;
   }

   if( !str_cmp( arg1, "remove" ) )
   {
      deleteptr( pHelp );
      --top_help;
      ch->print( "Removed.\r\n" );
      return;
   }

   if( !str_cmp( arg1, "level" ) )
   {
      pHelp->level = atoi( arg2.c_str(  ) );
      ch->print( "Done.\r\n" );
      return;
   }

   if( !str_cmp( arg1, "keyword" ) )
   {
      pHelp->keyword = arg2;
      strupper( pHelp->keyword );
      ch->print( "Done.\r\n" );
      return;
   }
   do_hset( ch, "" );
}

/*
 * Show help topics in a level range				-Thoric
 * Idea suggested by Gorog
 * prefix keyword indexing added by Fireblade
 */
CMDF( do_hlist )
{
   int min, max, minlimit, maxlimit, cnt;
   string arg;
   list < help_data * >::iterator ihelp;
   bool minfound, maxfound;
   char *idx = NULL;

   maxlimit = ch->get_trust(  );
   minlimit = maxlimit >= LEVEL_GREATER ? -1 : 0;

   min = minlimit;
   max = maxlimit;

   minfound = false;
   maxfound = false;

   for( argument = one_argument( argument, arg ); !arg.empty(  ); argument = one_argument( argument, arg ) )
   {
      if( !isdigit( arg[0] ) )
      {
         if( idx )
         {
            ch->print( "You may only use a single keyword to index the list.\r\n" );
            return;
         }
         idx = STRALLOC( arg.c_str(  ) );
      }
      else
      {
         if( !minfound )
         {
            min = URANGE( minlimit, atoi( arg.c_str(  ) ), maxlimit );
            minfound = true;
         }
         else if( !maxfound )
         {
            max = URANGE( minlimit, atoi( arg.c_str(  ) ), maxlimit );
            maxfound = true;
         }
         else
         {
            ch->print( "You may only use two level limits.\r\n" );
            return;
         }
      }
   }

   if( min > max )
   {
      int temp = min;

      min = max;
      max = temp;
   }

   ch->set_pager_color( AT_HELP );
   ch->pagerf( "Help Topics in level range %d to %d:\r\r\n\n", min, max );
   for( cnt = 0, ihelp = helplist.begin(  ); ihelp != helplist.end(  ); ++ihelp )
   {
      help_data *help = *ihelp;

      if( help->level >= min && help->level <= max && ( !idx || hasname( help->keyword, idx ) ) )
      {
         ch->pagerf( "  %3d %s\r\n", help->level, help->keyword.c_str(  ) );
         ++cnt;
      }
   }
   if( cnt )
      ch->pagerf( "\r\n%d pages found.\r\n", cnt );
   else
      ch->print( "None found.\r\n" );

   STRFREE( idx );
}

/* 
 * Title : Help Check Plus v1.0
 * Author: Chris Coulter (aka Gabriel Androctus)
 * Email : krisco7@bigfoot.com
 * Mud   : Perils of Quiernin (perils.wolfpaw.net 6000)
 * Descr.: A ridiculously simple routine that runs through the command and skill tables
 *         checking for help entries of the same name. If not found, it outputs a line
 *         to the pager. Priceless tool for finding those pesky missing help entries.
*/
CMDF( do_helpcheck )
{
   help_data *help;
   int sn, total = 0;
   bool fSkills = false, fCmds = false;

   if( argument.empty(  ) )
   {
      ch->print( "Syntax: helpcheck [ skills | commands | all ]\r\n" );
      return;
   }

   /*
    * check arguments and set appropriate switches 
    */
   if( !str_cmp( argument, "skills" ) )
      fSkills = true;
   if( !str_cmp( argument, "commands" ) )
      fCmds = true;
   if( !str_cmp( argument, "all" ) )
   {
      fSkills = true;
      fCmds = true;
   }

   if( fCmds ) /* run through command table */
   {
      ch->pager( "&CMissing Commands Helps\r\n\r\n" );

      for( char x = 0; x < 126; ++x )
      {
         const vector < cmd_type * >&cmd_list = command_table[x];
         vector < cmd_type * >::const_iterator icmd;

         for( icmd = cmd_list.begin(  ); icmd != cmd_list.end(  ); ++icmd )
         {
            cmd_type *command = *icmd;

            /*
             * No entry, or command is above person's level 
             */
            if( !( help = get_help( ch, command->name ) ) && command->level <= ch->level )
            {
               ch->pagerf( "&cNot found: &C%s&w\r\n", command->name.c_str(  ) );
               ++total;
            }
            else
               continue;
         }
      }
   }

   if( fSkills )  /* run through skill table */
   {
      ch->pager( "\r\n&CMissing Skill/Spell Helps\r\n\r\n" );
      for( sn = 0; sn < num_skills; ++sn )
      {
         if( !( help = get_help( ch, skill_table[sn]->name ) ) )  /* no help entry */
         {
            ch->pagerf( "&gNot found: &G%s&w\r\n", skill_table[sn]->name );
            ++total;
         }
         else
            continue;
      }
   }
   /*
    * tally up the total number of missing entries and finish up 
    */
   ch->pagerf( "\r\n&Y%d missing entries found.&w\r\n", total );
}
