/****************************************************************************
 *                   ^     +----- |  / ^     ^ |     | +-\                  *
 *                  / \    |      | /  |\   /| |     | |  \                 *
 *                 /   \   +---   |<   | \ / | |     | |  |                 *
 *                /-----\  |      | \  |  v  | |     | |  /                 *
 *               /       \ |      |  \ |     | +-----+ +-/                  *
 ****************************************************************************
 * AFKMud Copyright 1997-2025 by Roger Libiez (Samson),                     *
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

#include <filesystem>
#include <format>
#include <fstream>
#include "mud.h"
#include "calendar.h"
#include "descriptor.h"
#include "finger.h"
#include "roomindex.h"

bool check_parse_name( std::string, bool );

/* Begin wizinfo stuff - Samson 6-6-99 */

std::list<wizinfo_data *> wizinfolist;

wizinfo_data::wizinfo_data(  )
{
   level = 0;
}

wizinfo_data::~wizinfo_data(  )
{
   wizinfolist.remove( this );
}

/* Construct wizinfo list from god dir info - Samson 6-6-99 */
void add_to_wizinfo( std::string_view name, wizinfo_data * wiz )
{
   std::list<wizinfo_data *>::iterator wizinfo;

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
   if( !fBootDb )
   {
      for( auto it = wizinfolist.begin(); it != wizinfolist.end(); )
      {
         wizinfo_data *winfo = *it;
         ++it;

         deleteptr( winfo );
      }
   }
   wizinfolist.clear(  );
}

void build_wizinfo( void )
{
   // Always start with an empty list.
   clear_wizinfo(  );

   // Walk the file list in GOD_DIR.
   for( const auto& entry : std::filesystem::directory_iterator( GOD_DIR ) )
   {
      // An actual file entry and not another folder.
      if( entry.is_regular_file( ) )
      {
         std::ifstream file( entry.path( ) );
         std::string word;
         std::string irealm, iemail;

         wizinfo_data *wiz = new wizinfo_data;
         int ilevel = 0;

         while( file >> word )
         {
            if( word == "Level" )
               file >> ilevel;
            if( word == "ImmRealm" )
               file >> irealm;
            if( word == "Email" )
               file >> iemail;
         }
         add_to_wizinfo( entry.path( ).filename( ).string( ), wiz );
      }
   }
}

/* 
 * Wizinfo information.
 * Added by Samson on 6-6-99
 */
CMDF( do_wizinfo )
{
   ch->pager( "&cContact Information for the Immortals:\r\n\r\n" );
   ch->pager( "&cName         Email Address                     Realm\r\n" );
   ch->pager( "&c------------+---------------------------------+----------------\r\n" );

   for( auto* wi : wizinfolist )
   {
      // Allows an argument to show only a certain realm
      // --Cynshard
      if( !argument.empty(  ) )
         if( str_cmp( wi->realm, argument ) )
            continue;

      ch->pagerf( "&R%-12s &g%-33s &P%s&D\r\n", wi->name.c_str(  ), wi->email.c_str(  ), wi->realm.c_str(  ) );
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
   char_data *victim = nullptr;
   room_index *temproom, *original = nullptr;
   int level = LEVEL_IMMORTAL;
   const char *suf;
   short day = 0;
   bool loaded = false, skip = false;
   std::string time_str;

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

   std::string buf = std::format( "0.{}", argument );

   /*
    * If player is online, check for fingerability (yeah, I coined that one)  -Edge 
    */
   if( ( victim = ch->get_char_world( buf ) ) != nullptr )
   {
      if( victim->has_pcflag( PCFLAG_PRIVACY ) && !ch->is_immortal(  ) )
      {
         ch->print_fmt( "%s has privacy enabled.\r\n", victim->name );
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

      std::filesystem::path fingload = std::format( "{}{}/{}", PLAYER_DIR, static_cast<char>( std::tolower( argument.front() ) ), capitalize( argument ) );
      // Bug fix here provided by Senir to stop /dev/null crash
      if( !std::filesystem::exists( fingload ) || !check_parse_name( argument, false ) )
      {
         ch->print_fmt( "&YNo such player named '%s'.\r\n", argument.c_str(  ) );
         return;
      }

      auto laston = std::filesystem::last_write_time( fingload );
      time_str = std::format( "{:%Y-%m-%d %H:%M:%S}", laston );

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

      argument[0] = to_upper( argument[0] );

      loaded = load_char_obj( d, argument, false, false );  /* Remove second false if compiler complains */
      charlist.push_back( d->character );
      pclist.push_back( d->character );
      original = d->character->in_room;
      if( !d->character->to_room( temproom ) )
         log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __func__, __LINE__ );
      victim = d->character;  /* Hopefully this will work, if not, we're SOL */
      d->character->desc = nullptr;
      d->character = nullptr;
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
      ch->print_fmt( "&wName    : &G{:<20} &wMUD Age: &G{}\r\n", victim->name, victim->get_age(  ) );
      ch->print_fmt( "&wBirthday: &GDay of {}, {}{} day in the Month of {}, in the year {}.\r\n",
                  day_name[victim->pcdata->day % 13], day, suf, month_name[victim->pcdata->month], victim->pcdata->year );
      ch->print_fmt( "&wLevel   : &G{:<20} &w  Class: &G{}\r\n", victim->level, capitalize( victim->get_class(  ) ) );
      ch->print_fmt( "&wSex     : &G{:<20} &w  Race : &G{}\r\n", npc_sex[victim->sex], capitalize( victim->get_race(  ) ) );
      ch->print_fmt( "&wTitle   : &G{}\r\n", victim->pcdata->title );
      ch->print_fmt( "&wHomepage: &G{}\r\n", !victim->pcdata->homepage.empty(  )? show_tilde( victim->pcdata->homepage ) : "Not specified" );
      ch->print_fmt( "&wEmail   : &G{}\r\n", !victim->pcdata->email.empty(  )? victim->pcdata->email : "Not specified" );
      if( !loaded )
         ch->print_fmt( "&wLast on : &G{}\r\n", c_time( victim->pcdata->logon, ch->pcdata->timezone ) );
      else
         ch->print_fmt( "&wLast on : &G{}\r\n", time_str );
      if( ch->is_immortal(  ) )
      {
         ch->print( "&wImmortal Information\r\n" );
         ch->print( "--------------------\r\n" );
         ch->print_fmt( "&wRealm         : &G{}\r\n", victim->pcdata->realm_name );
         ch->print_fmt( "&wHost Info     : &G{}\r\n", victim->pcdata->lasthost );
         ch->print_fmt( "&wIP Info       : &G{}\r\n", victim->desc ? victim->desc->ipaddress : "Unknown" );
         ch->print_fmt( "&wPrev IP Info  : &G{}\r\n", victim->pcdata->prevhost );

         std::string time_played = std::format( "{}", victim->time_played( ) );
         ch->print_fmt( "&wTime played   : &G{} hours\r\n", time_played );

         ch->print_fmt( "&wAuthorized by : &G{}\r\n",
                     !victim->pcdata->authed_by.empty(  ) ? victim->pcdata->authed_by : ( sysdata->WAIT_FOR_AUTH ? "Not Authed" : "The Code" ) );
         ch->print_fmt( "&wPrivacy Status: &G{}\r\n", victim->has_pcflag( PCFLAG_PRIVACY ) ? "Enabled" : "Disabled" );
         if( victim->level < ch->level )
         {
            level = check_command_level( "comment", LEVEL_IMMORTAL );
            if( level != -1 )
               cmdf( ch, "comment list %s", victim->name );
         }
      }
      ch->print_fmt( "&wBio:\r\n&G{}\r\n", victim->pcdata->bio ? victim->pcdata->bio : "Not created" );
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
         for( auto it = victim->pets.begin(); it != victim->pets.end(); )
         {
            char_data *cpet = *it;
            ++it;

            cpet->extract( true );
         }
      }
      saving_char = nullptr;

      /*
       * After extract_char the ch is no longer valid!
       */
      victim->extract( true );
      for( x = 0; x < MAX_WEAR; ++x )
         for( y = 0; y < MAX_LAYERS; ++y )
            save_equipment[x][y] = nullptr;
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
      if( ch->is_immortal(  ) )
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
   std::string buf;

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
      buf = "https://" + argument;
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
