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
 *                        Finger and Wizinfo Module                         *
 ****************************************************************************/

/******************************************************
        Additions and changes by Edge of Acedia
              Rewritten do_finger to better
             handle info of offline players.
           E-mail: nevesfirestar2002@yahoo.com
 ******************************************************/

#include <dirent.h>
#include <sys/stat.h>
#include <fstream>
#include "mud.h"
#include "calendar.h"
#include "descriptor.h"
#include "finger.h"
#include "roomindex.h"

/* Begin wizinfo stuff - Samson 6-6-99 */

list < wizinfo_data * >wizinfolist;

wizinfo_data::wizinfo_data(  )
{
   level = 0;
}

wizinfo_data::~wizinfo_data(  )
{
   wizinfolist.remove( this );
}

/* Construct wizinfo list from god dir info - Samson 6-6-99 */
void add_to_wizinfo( const string & name, wizinfo_data * wiz )
void add_to_wizinfo( const string & name, wizinfo_data * wiz )
{
   list < wizinfo_data * >::iterator wizinfo;

   wiz->name = name;
   if( wiz->email.empty(  ) )
      wiz->email = "Not Set";

   for( wizinfo = wizinfolist.begin(  ); wizinfo != wizinfolist.end(  ); ++wizinfo )
   {
      wizinfo_data *w = *wizinfo;

      if( w->name >= name )
      {
         wizinfolist.insert( wizinfo, wiz );
         return;
      }
   }
   wizinfolist.push_back( wiz );
}

void clear_wizinfo( void )
{
   list < wizinfo_data * >::iterator wiz;

   if( !fBootDb )
   {
      for( wiz = wizinfolist.begin(  ); wiz != wizinfolist.end(  ); )
      {
         wizinfo_data *winfo = *wiz;
         ++wiz;

         deleteptr( winfo );
      }
   }
   wizinfolist.clear(  );
}

void build_wizinfo( void )
{
   DIR *dp;
   struct dirent *dentry;
   ifstream stream;
   wizinfo_data *wiz;
   char buf[256];

   clear_wizinfo(  );   /* Clear out the table before rebuilding a new one */

   dp = opendir( GOD_DIR );

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
         snprintf( buf, 256, "%s%s", GOD_DIR, dentry->d_name );
         stream.open( buf );
         if( stream.is_open(  ) )
         {
            wiz = new wizinfo_data;
            do
            {
               string line, key, value;
               char buf2[MIL];

               stream >> key;
               stream.getline( buf2, MIL );
               value = buf2;

               strip_lspace( key );
               strip_tilde( value );
               strip_lspace( value );

               if( key == "Level" )
                  wiz->level = atoi( value.c_str(  ) );
               else if( key == "ImmRealm" )
                  wiz->realm = value;
               else if( key == "Email" )
                  wiz->email = value;
            }
            while( !stream.eof(  ) );
            add_to_wizinfo( dentry->d_name, wiz );
            stream.close(  );
         }
      }
      dentry = readdir( dp );
   }
   closedir( dp );
}

/* 
 * Wizinfo information.
 * Added by Samson on 6-6-99
 */
CMDF( do_wizinfo )
{
   list < wizinfo_data * >::iterator wiz;

   ch->pager( "&cContact Information for the Immortals:\r\n\r\n" );
   ch->pager( "&cName         Email Address                     Realm\r\n" );
   ch->pager( "&c------------+---------------------------------+----------------\r\n" );

   for( wiz = wizinfolist.begin(  ); wiz != wizinfolist.end(  ); ++wiz )
   {
      wizinfo_data *wi = *wiz;

      // Allows an argument to show only a certain realm
      // --Cynshard
      if( !argument.empty(  ) )
         if( str_cmp( wi->realm, argument ) )
            continue;

      ch->pagerf( "&R%-12s &g%-33s &B%10d &P%s&D\r\n", wi->name.c_str(  ), wi->email.c_str(  ), wi->realm.c_str(  ) );
   }
}

/* End wizinfo stuff - Samson 6-6-99 */

/* Finger snippet courtesy of unknown author. Installed by Samson 4-6-98 */
/* File read/write code redone using standard Smaug I/O routines - Samson 9-12-98 */
/* Data gathering now done via the pfiles, eliminated separate finger files - Samson 12-21-98 */
/* Improvements for offline players by Edge of Acedia 8-26-03 */
/* Further refined by Samson on 8-26-03 */
CMDF( do_finger )
{
   char_data *victim = NULL;
   room_index *temproom, *original = NULL;
   int level = LEVEL_IMMORTAL;
   char buf[MIL], fingload[256];
   const char *suf, *laston = NULL;
   struct stat fst;
   short day = 0;
   bool loaded = false, skip = false;

   if( ch->isnpc(  ) )
   {
      ch->print( "Mobs can't use the finger command.\r\n" );
      return;
   }

   if( argument.empty(  ) )
   {
      ch->print( "Finger whom?\r\n" );
      return;
   }

   snprintf( buf, MIL, "0.%s", argument.c_str(  ) );

   /*
    * If player is online, check for fingerability (yeah, I coined that one)  -Edge 
    */
   if( ( victim = ch->get_char_world( buf ) ) != NULL )
   {
      if( victim->has_pcflag( PCFLAG_PRIVACY ) && !ch->is_immortal(  ) )
      {
         ch->printf( "%s has privacy enabled.\r\n", victim->name );
         return;
      }

      if( victim->is_immortal(  ) && !ch->is_immortal(  ) )
      {
         ch->print( "You cannot finger an immortal.\r\n" );
         return;
      }
   }

   /*
    * Check for offline players - Edge 
    */
   else
   {
      descriptor_data *d;

      snprintf( fingload, 256, "%s%c/%s", PLAYER_DIR, tolower( argument[0] ), capitalize( argument ).c_str(  ) );
      /*
       * Bug fix here provided by Senir to stop /dev/null crash 
       */
      if( stat( fingload, &fst ) == -1 || !check_parse_name( capitalize( argument ), false ) )
      {
         ch->printf( "&YNo such player named '%s'.\r\n", argument.c_str(  ) );
         return;
      }

      laston = ctime( &fst.st_mtime );
      temproom = get_room_index( ROOM_VNUM_LIMBO );
      if( !temproom )
      {
         bug( "%s: Limbo room is not available!", __func__ );
         ch->print( "Fatal error, report to the immortals.\r\n" );
         return;
      }

      d = new descriptor_data;
      d->init(  );
      d->connected = CON_PLOADED;

      argument[0] = UPPER( argument[0] );

      loaded = load_char_obj( d, argument, false, false );  /* Remove second false if compiler complains */
      charlist.push_back( d->character );
      pclist.push_back( d->character );
      original = d->character->in_room;
      if( !d->character->to_room( temproom ) )
         log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __func__, __LINE__ );
      victim = d->character;  /* Hopefully this will work, if not, we're SOL */
      d->character->desc = NULL;
      d->character = NULL;
      deleteptr( d );

      if( victim->has_pcflag( PCFLAG_PRIVACY ) && !ch->is_immortal(  ) )
      {
         ch->printf( "%s has privacy enabled.\r\n", victim->name );
         skip = true;
      }

      if( victim->is_immortal(  ) && !ch->is_immortal(  ) )
      {
         ch->print( "You cannot finger an immortal.\r\n" );
         skip = true;
      }
      if( victim->level < LEVEL_IMMORTAL )
         ++sysdata->playersonline;
      loaded = true;
   }

   if( !skip )
   {
      day = victim->pcdata->day + 1;

      if( day > 4 && day < 20 )
         suf = "th";
      else if( day % 10 == 1 )
         suf = "st";
      else if( day % 10 == 2 )
         suf = "nd";
      else if( day % 10 == 3 )
         suf = "rd";
      else
         suf = "th";

      ch->print( "&w          Finger Info\r\n" );
      ch->print( "          -----------\r\n" );
      ch->printf( "&wName    : &G%-20s &wMUD Age: &G%d\r\n", victim->name, victim->get_age(  ) );
      ch->printf( "&wBirthday: &GDay of %s, %d%s day in the Month of %s, in the year %d.\r\n",
                  day_name[victim->pcdata->day % 13], day, suf, month_name[victim->pcdata->month], victim->pcdata->year );
      ch->printf( "&wLevel   : &G%-20d &w  Class: &G%s\r\n", victim->level, capitalize( victim->get_class(  ) ) );
      ch->printf( "&wSex     : &G%-20s &w  Race : &G%s\r\n", npc_sex[victim->sex], capitalize( victim->get_race(  ) ) );
      ch->printf( "&wTitle   :&G%s\r\n", victim->pcdata->title );
      ch->printf( "&wHomepage: &G%s\r\n", !victim->pcdata->homepage.empty(  )? show_tilde( victim->pcdata->homepage ).c_str(  ) : "Not specified" );
      ch->printf( "&wEmail   : &G%s\r\n", !victim->pcdata->email.empty(  )? victim->pcdata->email.c_str(  ) : "Not specified" );
      if( !loaded )
         ch->printf( "&wLast on : &G%s\r\n", c_time( victim->pcdata->logon, ch->pcdata->timezone ) );
      else
         ch->printf( "&wLast on : &G%s\r\n", laston );
      if( ch->is_immortal(  ) )
      {
         ch->print( "&wImmortal Information\r\n" );
         ch->print( "--------------------\r\n" );
         ch->printf( "&wRealm         : &G%s\r\n", victim->pcdata->realm_name.c_str() );
         ch->printf( "&wIP Info       : &G%s\r\n", victim->pcdata->lasthost.c_str(  ) );
         ch->printf( "&wPrev IP Info  : &G%s\r\n", victim->pcdata->prevhost.c_str(  ) );
         ch->printf( "&wTime played   : &G%ld hours\r\n", ( long int )GET_TIME_PLAYED( victim ) );
         ch->printf( "&wAuthorized by : &G%s\r\n",
                     !victim->pcdata->authed_by.empty(  ) ? victim->pcdata->authed_by.c_str(  ) : ( sysdata->WAIT_FOR_AUTH ? "Not Authed" : "The Code" ) );
         ch->printf( "&wPrivacy Status: &G%s\r\n", victim->has_pcflag( PCFLAG_PRIVACY ) ? "Enabled" : "Disabled" );
         if( victim->level < ch->level )
         {
            level = check_command_level( "comment", LEVEL_IMMORTAL );
            if( level != -1 )
               cmdf( ch, "comment list %s", victim->name );
         }
      }
      ch->printf( "&wBio:\r\n&G%s\r\n", victim->pcdata->bio ? victim->pcdata->bio : "Not created" );
   }

   if( loaded )
   {
      int x, y;

      victim->from_room(  );
      if( !victim->to_room( original ) )
         log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __func__, __LINE__ );

      quitting_char = victim;

      if( sysdata->save_pets )
      {
         list < char_data * >::iterator pet;

         for( pet = victim->pets.begin(  ); pet != victim->pets.end(  ); )
         {
            char_data *cpet = *pet;
            ++pet;

            cpet->extract( true );
         }
      }
      saving_char = NULL;

      /*
       * After extract_char the ch is no longer valid!
       */
      victim->extract( true );
      for( x = 0; x < MAX_WEAR; ++x )
         for( y = 0; y < MAX_LAYERS; ++y )
            save_equipment[x][y] = NULL;
   }
}

/* Added a clone of homepage to let players input their email addy - Samson 4-18-98 */
CMDF( do_email )
{
   if( ch->isnpc(  ) )
      return;

   if( ch->has_pcflag( PCFLAG_NO_EMAIL ) )
   {
      ch->print( "You are not allowed to set an email address.\r\n" );
      return;
   }

   if( argument.empty(  ) )
   {
      if( !ch->pcdata->email.empty(  ) )
         ch->printf( "Your email address is: %s\r\n", ch->pcdata->email.c_str(  ) );
      else
         ch->print( "You have no email address set yet.\r\n" );
      return;
   }

   if( !str_cmp( argument, "clear" ) )
   {
      ch->pcdata->email.clear(  );

      ch->save(  );
      if( ch->is_immortal(  ) );
         build_wizinfo(  );

      ch->print( "Email address cleared.\r\n" );
      return;
   }

   smash_tilde( argument );
   ch->pcdata->email = argument.substr( 0, 75 );

   ch->save(  );
   if( ch->is_immortal(  ) )
      build_wizinfo(  );

   ch->print( "Email address set.\r\n" );
}

CMDF( do_homepage )
{
   string buf;

   if( ch->isnpc(  ) )
      return;

   if( ch->has_pcflag( PCFLAG_NO_URL ) )
   {
      ch->print( "You are not allowed to set a homepage.\r\n" );
      return;
   }

   if( argument.empty(  ) )
   {
      if( !ch->pcdata->homepage.empty(  ) )
         ch->printf( "Your homepage is: %s\r\n", show_tilde( ch->pcdata->homepage ).c_str(  ) );
      else
         ch->print( "You have no homepage set yet.\r\n" );
      return;
   }

   if( !str_cmp( argument, "clear" ) )
   {
      ch->pcdata->homepage.clear(  );
      ch->print( "Homepage cleared.\r\n" );
      return;
   }

   if( strstr( argument.c_str(  ), "://" ) )
      buf = argument;
   else
      buf = "http://" + argument;
   buf = buf.substr( 0, 75 );

   hide_tilde( buf );
   ch->pcdata->homepage = buf;
   ch->print( "Homepage set.\r\n" );
}

CMDF( do_privacy )
{
   if( ch->isnpc(  ) )
   {
      ch->print( "Mobs can't use the privacy toggle.\r\n" );
      return;
   }

   ch->toggle_pcflag( PCFLAG_PRIVACY );

   if( ch->has_pcflag( PCFLAG_PRIVACY ) )
   {
      ch->print( "Privacy flag enabled.\r\n" );
      return;
   }
   else
   {
      ch->print( "Privacy flag disabled.\r\n" );
      return;
   }
}
