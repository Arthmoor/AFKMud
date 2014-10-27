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
char *const realm_string[] = {
   "Immortal", "Builder", "C0DE", "Head Coder", "Head Builder", "Implementor"
};

list<wizinfo_data*> wizinfolist;

wizinfo_data::wizinfo_data()
{
   init_memory( &icq, &level, sizeof( level ) );
}

wizinfo_data::~wizinfo_data()
{
   wizinfolist.remove( this );
}

/* Construct wizinfo list from god dir info - Samson 6-6-99 */
void add_to_wizinfo( string name, wizinfo_data * wiz )
{
   list<wizinfo_data*>::iterator wizinfo;

   wiz->set_name( name );
   if( wiz->get_email().empty() )
      wiz->set_email( "Not Set" );

   for( wizinfo = wizinfolist.begin(); wizinfo != wizinfolist.end(); ++wizinfo )
      if( (*wizinfo)->get_name() >= name )
      {
         wizinfolist.insert( wizinfo, wiz );
         return;
      }
   wizinfolist.push_back( wiz );
   return;
}

void clear_wizinfo( void )
{
   list<wizinfo_data*>::iterator wiz;

   if( !fBootDb )
   {
      for( wiz = wizinfolist.begin(); wiz != wizinfolist.end(); )
      {
         wizinfo_data *winfo = (*wiz);
         ++wiz;

         deleteptr( winfo );
      }
   }
   wizinfolist.clear();
   return;
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
         if( stream.is_open() )
         {
            wiz = new wizinfo_data;
            do
            {
               string line, key, value;
               char buf2[MIL];

               stream >> key;
               strip_lspace( key );

               if( key == "Level" )
               {
                  stream.getline( buf2, MIL );
                  value = buf2;
                  strip_lspace( value );
                  wiz->set_level( atoi( value.c_str() ) );
               }
               if( key == "Realm" )
               {
                  stream.getline( buf2, MIL );
                  value = buf2;
                  strip_lspace( value );
                  wiz->set_realm( atoi( value.c_str() ) );
               }
               if( key == "ICQ" )
               {
                  stream.getline( buf2, MIL );
                  value = buf2;
                  strip_lspace( value );
                  wiz->set_icq( atoi( value.c_str() ) );
               }
               if( key == "Email" )
               {
                  stream.getline( buf2, MIL );
                  value = buf2;
                  strip_tilde( value );
                  strip_lspace( value );
                  wiz->set_email( value );
               }
            }
            while( !stream.eof() );
            add_to_wizinfo( dentry->d_name, wiz );
            stream.close();
         }
      }
      dentry = readdir( dp );
   }
   closedir( dp );
   return;
}

/* 
 * Wizinfo information.
 * Added by Samson on 6-6-99
 */
CMDF( do_wizinfo )
{
   list<wizinfo_data*>::iterator wiz;

   ch->pager( "&cContact Information for the Immortals:\r\n\r\n" );
   ch->pager( "&cName         Email Address                     ICQ#       Realm\r\n" );
   ch->pager( "&c------------+---------------------------------+----------+----------------\r\n" );

   for( wiz = wizinfolist.begin(); wiz != wizinfolist.end(); ++wiz )
   {
      wizinfo_data *wi = (*wiz);

      // Allows an argument to show only a certain realm
      // --Cynshard
      if( argument && argument[0] != '\0' )
         if( str_cmp( realm_string[wi->get_realm()], argument ) )
            continue;

      ch->printf( "&R%-12s &g%-33s &B%10d &P%s&D\r\n",
         wi->get_name().c_str(), wi->get_email().c_str(), wi->get_icq(), realm_string[wi->get_realm()] );
   }
   return;
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
   char *suf, *laston = NULL;
   struct stat fst;
   short day = 0;
   bool loaded = false, skip = false;

   if( ch->isnpc(  ) )
   {
      ch->print( "Mobs can't use the finger command.\r\n" );
      return;
   }

   if( !argument || argument[0] == '\0' )
   {
      ch->print( "Finger whom?\r\n" );
      return;
   }

   snprintf( buf, MIL, "0.%s", argument );

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

      snprintf( fingload, 256, "%s%c/%s", PLAYER_DIR, tolower( argument[0] ), capitalize( argument ) );
      /*
       * Bug fix here provided by Senir to stop /dev/null crash 
       */
      if( stat( fingload, &fst ) == -1 || !check_parse_name( capitalize( argument ), false ) )
      {
         ch->printf( "&YNo such player named '%s'.\r\n", argument );
         return;
      }

      laston = ctime( &fst.st_mtime );
      temproom = get_room_index( ROOM_VNUM_LIMBO );
      if( !temproom )
      {
         bug( "%s: Limbo room is not available!", __FUNCTION__ );
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
         log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
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
      ch->printf( "&wHomepage: &G%s\r\n",
                  victim->pcdata->homepage != NULL ? show_tilde( victim->pcdata->homepage ) : "Not specified" );
      ch->printf( "&wEmail   : &G%s\r\n", victim->pcdata->email != NULL ? victim->pcdata->email : "Not specified" );
      ch->printf( "&wICQ#    : &G%d\r\n", victim->pcdata->icq );
      if( !loaded )
         ch->printf( "&wLast on : &G%s\r\n", c_time( victim->pcdata->logon, ch->pcdata->timezone ) );
      else
         ch->printf( "&wLast on : &G%s\r\n", laston );
      if( ch->is_immortal(  ) )
      {
         ch->print( "&wImmortal Information\r\n" );
         ch->print( "--------------------\r\n" );
         ch->printf( "&wIP Info       : &G%s\r\n", victim->pcdata->lasthost );
         ch->printf( "&wTime played   : &G%ld hours\r\n", ( long int )GET_TIME_PLAYED( victim ) );
         ch->printf( "&wAuthorized by : &G%s\r\n",
                     victim->pcdata->authed_by ? victim->pcdata->authed_by : ( sysdata->
                                                                         WAIT_FOR_AUTH ? "Not Authed" : "The Code" ) );
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
         log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );

      quitting_char = victim;

      if( sysdata->save_pets )
      {
         list<char_data*>::iterator pet;

         for( pet = victim->pets.begin(); pet != victim->pets.end(); )
         {
            char_data *cpet = (*pet);
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
   return;
}

/* Added a clone of homepage to let players input their email addy - Samson 4-18-98 */
CMDF( do_email )
{
   char buf[75];

   if( ch->isnpc(  ) )
      return;

   if( !argument || argument[0] == '\0' )
   {
      if( ch->pcdata->email && ch->pcdata->email[0] != '\0' )
         ch->printf( "Your email address is: %s\r\n", show_tilde( ch->pcdata->email ) );
      else
         ch->print( "You have no email address set yet.\r\n" );
      return;
   }

   if( !str_cmp( argument, "clear" ) )
   {
      DISPOSE( ch->pcdata->email );

      if( ch->is_immortal(  ) );
      {
         ch->save(  );
         build_wizinfo(  );
      }
      ch->print( "Email address cleared.\r\n" );
      return;
   }

   mudstrlcpy( buf, argument, 75 );

   smash_tilde( buf );
   DISPOSE( ch->pcdata->email );
   ch->pcdata->email = str_dup( buf );
   if( ch->is_immortal(  ) );
   {
      ch->save(  );
      build_wizinfo(  );
   }
   ch->print( "Email address set.\r\n" );
}

CMDF( do_icq_number )
{
   int icq;

   if( ch->isnpc(  ) )
      return;

   if( !argument || argument[0] == '\0' )
   {
      ch->printf( "Your ICQ# is: %d\r\n", ch->pcdata->icq );
      return;
   }

   if( !str_cmp( argument, "clear" ) )
   {
      ch->pcdata->icq = 0;

      if( ch->is_immortal(  ) );
      {
         ch->save(  );
         build_wizinfo(  );
      }
      ch->print( "ICQ# cleared.\r\n" );
      return;
   }

   if( !is_number( argument ) )
   {
      ch->print( "You must enter numeric data.\r\n" );
      return;
   }

   icq = atoi( argument );

   if( icq < 1 )
   {
      ch->print( "Valid range is greater than 0.\r\n" );
      return;
   }

   ch->pcdata->icq = icq;

   if( ch->is_immortal(  ) );
   {
      ch->save(  );
      build_wizinfo(  );
   }
   ch->print( "ICQ# set.\r\n" );
   return;
}

CMDF( do_homepage )
{
   char buf[75];

   if( ch->isnpc(  ) )
      return;

   if( !argument || argument[0] == '\0' )
   {
      if( ch->pcdata->homepage && ch->pcdata->homepage != '\0' )
         ch->printf( "Your homepage is: %s\r\n", show_tilde( ch->pcdata->homepage ) );
      else
         ch->print( "You have no homepage set yet.\r\n" );
      return;
   }

   if( !str_cmp( argument, "clear" ) )
   {
      DISPOSE( ch->pcdata->homepage );
      ch->print( "Homepage cleared.\r\n" );
      return;
   }

   if( strstr( argument, "://" ) )
      mudstrlcpy( buf, argument, 75 );
   else
      snprintf( buf, 75, "http://%s", argument );

   hide_tilde( buf );
   DISPOSE( ch->pcdata->homepage );
   ch->pcdata->homepage = str_dup( buf );
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
