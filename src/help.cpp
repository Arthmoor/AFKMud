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
 *                           Help System Module                             *
 ****************************************************************************/

#include <fstream>
#include "mud.h"
#include "commands.h"
#include "help.h"
#include "raceclass.h"
#include "smaugaffect.h"

#define NOHELP_FILE SYSTEM_DIR "nohelp.txt"  /* For tracking missing help entries */

extern char *const spell_saves[];
extern char *const spell_save_effect[];

list<help_data*> helplist;

int top_help;

help_data::help_data()
{
   init_memory( &keyword, &level, sizeof( level ) );
}

help_data::~help_data()
{
   DISPOSE( text );
   DISPOSE( keyword );
   if( !fBootDb )
      helplist.remove( this );
}

void free_helps( void )
{
   list<help_data*>::iterator pHelp;

   for( pHelp = helplist.begin(); pHelp != helplist.end(); )
   {
      help_data *help = *pHelp;
      ++pHelp;

      deleteptr( help );
   }
   return;
}

/*
 * Adds a help page to the list if it is not a duplicate of an existing page.
 * Page is insert-sorted by keyword. - Thoric
 * (The reason for sorting is to keep do_hlist looking nice)
 */
void add_help( help_data * pHelp )
{
   list<help_data*>::iterator tHelp;
   int match;
   bool added = false;

   for( tHelp = helplist.begin(); tHelp != helplist.end(); ++tHelp )
   {
      if( pHelp->level == (*tHelp)->level && !str_cmp( pHelp->keyword, (*tHelp)->keyword ) )
      {
         bug( "%s: duplicate: %s. Deleting.", __FUNCTION__, pHelp->keyword );
         deleteptr( pHelp );
         return;
      }
      else if( ( match = strcmp( pHelp->keyword[0] == '\'' ? pHelp->keyword + 1 : pHelp->keyword,
                                 (*tHelp)->keyword[0] == '\'' ? (*tHelp)->keyword + 1 : (*tHelp)->keyword ) ) < 0
               || ( match == 0 && pHelp->level > (*tHelp)->level ) )
      {
         helplist.push_back( pHelp );
         added = true;
         break;
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

   list<help_data*>::iterator ihlp;
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
   return;
}

void load_helps( void )
{
   help_data *help;
   ifstream stream;

   helplist.clear(  );

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

      if( key == "#HELP" )
         help = new help_data;

      if( key == "Keywords" )
      {
         stream.getline( buf, MSL );
         value = buf;
         strip_lspace( value );
         help->keyword = str_dup( value.c_str() );
      }

      if( key == "Level" )
      {
         stream.getline( buf, MSL );
         value = buf;
         strip_lspace( value );

         help->level = atoi( value.c_str() );
      }

      if( key == "Text" )
      {
         stream.getline( buf, MSL, '¢' );
         value = buf;
         strip_lspace( value );
         help->text = str_dup( value.c_str() );
      }

      if( key == "End" )
         helplist.push_back( help );

   }
   while( !stream.eof(  ) );
   stream.close(  );
   return;
}

int skill_number( char *argument )
{
   int sn;

   if( ( sn = skill_lookup( argument ) ) >= 0 )
      return sn;
   return -1;
}

bool get_skill_help( char_data *ch, char *argument )
{
   skill_type *skill = NULL;
   char buf[MSL], target[MSL];
   int sn;

   if( ( sn = skill_number( argument ) ) >= 0 )
      skill = skill_table[sn];

   // Not a skill/spell, drop back to regular help
   if( sn < 0 || !skill || skill->type == SKILL_HERB )
      return false;

   target[0] = '\0';
   switch( skill->target )
   {
      default:
         break;

      case TAR_CHAR_OFFENSIVE:
         mudstrlcpy( target, "<victim>", MSL );
         break;

      case TAR_CHAR_SELF:
         mudstrlcpy( target, "<self>", MSL );
         break;

      case TAR_OBJ_INV:
         mudstrlcpy( target, "<object>", MSL );
         break;
   }

   snprintf( buf, MSL, "cast '%s'", skill->name );
   ch->printf( "Usage        : %s %s\r\n", skill->type == SKILL_SPELL ? buf : skill->name, target );

   if( skill->affects.empty() )
      ch->print( "Duration     : Instant\r\n" );
   else
   {
      list<smaug_affect*>::iterator paf;
      bool found = false;

      for( paf = skill->affects.begin(); paf != skill->affects.end(); ++paf )
      {
         smaug_affect *af = *paf;

         // Make sure duration isn't null, and is not 0
         if( af->duration && af->duration[0] != '\0' && af->duration[0] != '0' )
         {
            if( !found )
            {
               ch->print( "Duration     :\r\n" );
               found = true;
            }
            ch->printf( "   Affect    : '%s' for '%s' rounds.\r\n", aff_flags[af->bit], af->duration );
         }
      }
   }

   ch->printf( "%-5s Level  : ", skill->type == SKILL_RACIAL ? "Race" : "Class" );

   bool firstpass = true;
   if( skill->type != SKILL_RACIAL )
   {
      for( int iClass = 0; iClass < MAX_PC_CLASS; ++iClass )
      {
         if( skill->skill_level[iClass] > LEVEL_AVATAR )
            continue;

         if( firstpass )
         {
            snprintf( buf, MSL, "%s ", class_table[iClass]->who_name );
            firstpass = false;
         }
         else
            snprintf( buf, MSL, ", %s ", class_table[iClass]->who_name );

         snprintf( buf + strlen(buf), MSL - strlen(buf), "%d", skill->skill_level[iClass] );
         ch->print( buf );
      }
   }
   else
   {
      for( int iRace = 0; iRace < MAX_PC_RACE; ++iRace )
      {
         if( skill->race_level[iRace] > LEVEL_AVATAR )
            continue;

         if( firstpass )
         {
            snprintf( buf, MSL, "%s ", race_table[iRace]->race_name );
            firstpass = false;
         }
         else
            snprintf( buf, MSL, ", %s ", race_table[iRace]->race_name );

         snprintf( buf + strlen(buf), MSL - strlen(buf), "%d", skill->race_level[iRace] );
         ch->print( buf );
      }
   }
   ch->print( "\r\n" );

   if( skill->dice )
      ch->printf( "Damage       : %s\r\n", skill->dice );

   if( skill->type == SKILL_SPELL )
   {
      if( skill->saves > 0 )
         ch->printf( "Save         : vs %s for %s\r\n",
            spell_saves[( int )skill->saves], spell_save_effect[SPELL_SAVE( skill )] );
      ch->printf( "Minimum cost : %d mana\r\n", skill->min_mana );
   }

   if( skill->helptext )
      ch->printf( "\r\n%s\r\n", skill->helptext );
   return true;
}

/*
 * Moved into a separate function so it can be used for other things
 * ie: online help editing - Thoric
 */
help_data *get_help( char_data * ch, char *argument )
{
   char argall[MIL], argone[MIL], argnew[MIL];
   list<help_data*>::iterator hlp;
   help_data *pHelp;
   int lev;

   if( !argument || argument[0] == '\0' )
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
   argall[0] = '\0';
   while( argument[0] != '\0' )
   {
      argument = one_argument( argument, argone );
      if( argall[0] != '\0' )
         mudstrlcat( argall, " ", MIL );
      mudstrlcat( argall, argone, MIL );
   }

   for( hlp = helplist.begin(); hlp != helplist.end(); ++hlp )
   {
      pHelp = (*hlp);

      if( pHelp->level > ch->get_trust(  ) )
         continue;
      if( lev != -2 && pHelp->level != lev )
         continue;

      if( is_name( argall, pHelp->keyword ) )
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
   list<help_data*>::iterator tHelp;
   char *keyword;
   char oneword[MSL], lastmatch[MSL];
   short matched = 0, checked = 0, totalmatched = 0, found = 0;

   ch->set_pager_color( AT_HELP );

   if( !argument || argument[0] == '\0' )
      argument = "summary";

   if( !( pHelp = get_help( ch, argument ) ) )
   {
      // Go look in skill help. If it's there, it gets displayed, and we leave.
      if( get_skill_help( ch, argument ) )
         return;

      ch->pagerf( "&wNo help on '%s' found.&D\r\n", argument );
      ch->pager( "&BSuggested Help Files:&D\r\n" );
      mudstrlcpy( lastmatch, " ", MSL );
      for( tHelp = helplist.begin(); tHelp != helplist.end(); ++tHelp )
      {
         matched = 0;
         if( !(*tHelp)->keyword || (*tHelp)->keyword[0] == '\0' || (*tHelp)->level > ch->get_trust(  ) )
            continue;
         keyword = (*tHelp)->keyword;
         while( keyword && keyword[0] != '\0' )
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
               ch->pagerf( "&G %-20s &D", oneword );
               if( ++found % 4 == 0 )
               {
                  found = 0;
                  ch->pager( "\r\n" );
               }
               mudstrlcpy( lastmatch, oneword, MSL );
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
      if( totalmatched == 1 && lastmatch != NULL && lastmatch[0] != '\0' )
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
      ch->pager( pHelp->text + 1 );
   else
      ch->pager( pHelp->text );
   return;
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
         DISPOSE( pHelp->text );
         pHelp->text = ch->copy_buffer( false );
         ch->stop_editing(  );
         return;

      case SUB_EDIT_ABORT:
         ch->substate = SUB_NONE;
         ch->print( "Aborting helpfile.\r\n" );
         return;
   }

   if( !( pHelp = get_help( ch, argument ) ) )  /* new help */
   {
      list<help_data*>::iterator iHelp;
      string argnew, narg = argument;
      int lev;
      bool new_help = true;

      for( iHelp = helplist.begin(); iHelp != helplist.end(); ++iHelp )
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
         pHelp->keyword = str_dup( strupper( narg.c_str() ) );
         pHelp->level = lev;
         add_help( pHelp );
      }
   }
   ch->substate = SUB_HELP_EDIT;
   ch->pcdata->dest_buf = pHelp;
   if( !pHelp->text || pHelp->text[0] == '\0' )
      pHelp->text = str_dup( "" );
   ch->editor_desc_printf( "Help topic, keyword '%s', level %d.", pHelp->keyword, pHelp->level );
   ch->start_editing( pHelp->text );
}

/*
 * Stupid leading space muncher fix - Thoric
 */
char *help_fix( char *text )
{
   char *fixed;

   if( !text )
      return "";
   fixed = strip_cr( text );
   if( fixed[0] == ' ' )
      fixed[0] = '.';
   return fixed;
}

CMDF( do_hset )
{
   help_data *pHelp;
   char arg1[MIL], arg2[MIL];

   smash_tilde( argument );
   argument = one_argument( argument, arg1 );
   if( !arg1 || arg1[0] == '\0' )
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
      load_helps();
      ch->print( "Help files reloaded.\r\n" );
      return;
   }

   if( !str_cmp( arg1, "save" ) )
   {
      save_helps();
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
      ch->print( "Removed.\r\n" );
      return;
   }

   if( !str_cmp( arg1, "level" ) )
   {
      pHelp->level = atoi( arg2 );
      ch->print( "Done.\r\n" );
      return;
   }

   if( !str_cmp( arg1, "keyword" ) )
   {
      DISPOSE( pHelp->keyword );
      pHelp->keyword = str_dup( strupper( arg2 ) );
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
   char arg[MIL];
   list<help_data*>::iterator ihelp;
   bool minfound, maxfound;
   char *idx;

   maxlimit = ch->get_trust(  );
   minlimit = maxlimit >= LEVEL_GREATER ? -1 : 0;

   min = minlimit;
   max = maxlimit;

   idx = NULL;
   minfound = false;
   maxfound = false;

   for( argument = one_argument( argument, arg ); arg[0] != '\0'; argument = one_argument( argument, arg ) )
   {
      if( !isdigit( arg[0] ) )
      {
         if( idx )
         {
            ch->print( "You may only use a single keyword to index the list.\r\n" );
            return;
         }
         idx = STRALLOC( arg );
      }
      else
      {
         if( !minfound )
         {
            min = URANGE( minlimit, atoi( arg ), maxlimit );
            minfound = true;
         }
         else if( !maxfound )
         {
            max = URANGE( minlimit, atoi( arg ), maxlimit );
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
   for( cnt = 0, ihelp = helplist.begin(); ihelp != helplist.end(); ++ihelp )
   {
      help_data *help = *ihelp;

      if( help->level >= min && help->level <= max && ( !idx || nifty_is_name_prefix( idx, help->keyword ) ) )
      {
         ch->pagerf( "  %3d %s\r\n", help->level, help->keyword );
         ++cnt;
      }
   }
   if( cnt )
      ch->pagerf( "\r\n%d pages found.\r\n", cnt );
   else
      ch->print( "None found.\r\n" );

   STRFREE( idx );
   return;
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
   cmd_type *command;
   help_data *help;
   int hash, sn, total = 0;
   bool fSkills = false, fCmds = false;

   if( !argument || argument[0] == '\0' )
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
      for( hash = 0; hash < 126; ++hash )
         for( command = command_hash[hash]; command; command = command->next )
         {
            /*
             * No entry, or command is above person's level 
             */
            if( !( help = get_help( ch, command->name ) ) && command->level <= ch->level )
            {
               ch->pagerf( "&cNot found: &C%s&w\r\n", command->name );
               ++total;
            }
            else
               continue;
         }
   }

   if( fSkills )  /* run through skill table */
   {
      ch->pager( "\r\n&CMissing Skill/Spell Helps\r\n\r\n" );
      for( sn = 0; sn < top_sn; ++sn )
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
   return;
}
