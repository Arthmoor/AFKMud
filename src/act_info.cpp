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
 *                          Informational module                            *
 ****************************************************************************/

#include <filesystem>
#include <fstream>
#include "mud.h"
#include "area.h"
#include "clans.h"
#include "descriptor.h"
#include "fight.h"
#include "liquids.h"
#include "mobindex.h"
#include "mud_prog.h"
#include "objindex.h"
#include "overland.h"
#include "pfiles.h"
#include "polymorph.h"
#include "raceclass.h"
#include "roomindex.h"
#include "weather.h"

constexpr std::string_view HISTORY_FILE = "../system/history.txt"; // Used in do_history - Samson 2-12-98

/*
 * Keep players from defeating examine progs -Druid
 * False = do not trigger
 * True = Trigger
 */
bool EXA_prog_trigger = true;
CMDF( do_track );
CMDF( do_cast );
CMDF( do_dig );
CMDF( do_search );
CMDF( do_detrap );
void save_sysdata(  );
void display_map( char_data * );
void draw_map( char_data *, std::string_view );
std::string password_hash( std::string_view );
bool is_valid_wear_loc( char_data *, int );
bool check_parse_name( std::string, bool );

const char *where_names[] = {
   "<used as light>     ",
   "<worn on finger>    ",
   "<worn on finger>    ",
   "<worn around neck>  ",
   "<worn around neck>  ", /* 5 */
   "<worn on body>      ",
   "<worn on head>      ",
   "<worn on legs>      ",
   "<worn on feet>      ",
   "<worn on hands>     ", /* 10 */
   "<worn on arms>      ",
   "<worn as shield>    ",
   "<worn about body>   ",
   "<worn about waist>  ",
   "<worn around wrist> ", /* 15 */
   "<worn around wrist> ",
   "<wielded>           ",
   "<held>              ",
   "<dual wielded>      ",
   "<worn on ears>      ", /* 20 */
   "<worn on eyes>      ",
   "<missile wielded>   ",
   "<worn on back>      ",
   "<worn on face>      ",
   "<worn on ankle>     ", /* 25 */
   "<worn on ankle>     ",
   "<worn on hooves>    ",
   "<worn on tail>      ",
   "<lodged in a rib>   ",
   "<lodged in an arm>  ", // 30
   "<lodged in a leg>   ",
   "<UNDEFINED>         ",
   "<UNDEFINED>         ",
   "<UNDEFINED>         ",
   "<UNDEFINED>         ", // 35
};

/*
StarMap was written by Nebseni of Clandestine MUD and ported to Smaug
by Desden, el Chaman Tibetano.
*/

constexpr int MAP_WIDTH = 72;
constexpr int MAP_HEIGHT = 8;
/* Should be the string length and number of the constants below.*/

const char *star_map[] = {
   "                                               C. C.                  g*",
   "    O:       R*        G*    G.  W* W. W.          C. C.    Y* Y. Y.    ",
   "  O*.                c.          W.W.     W.            C.       Y..Y.  ",
   "O.O. O.              c.  G..G.           W:      B*                   Y.",
   "     O.    c.     c.                     W. W.                  r*    Y.",
   "     O.c.     c.      G.             P..     W.        p.      Y.   Y:  ",
   "        c.                    G*    P.  P.           p.  p:     Y.   Y. ",
   "                 b*             P.: P*                 p.p:             "
};

/****************** CONSTELLATIONS and STARS *****************************
  Cygnus     Mars        Orion      Dragon       Cassiopeia          Venus
           Ursa Ninor                           Mercurius     Pluto    
               Uranus              Leo                Crown       Raptor
*************************************************************************/

const char *sun_map[] = {
   "\\`|'/",
   "- O -",
   "/.|.\\"
};

const char *moon_map[] = {
   " @@@ ",
   "@@@@@",
   " @@@ "
};

void look_sky( char_data * ch )
{
   std::string buf;
   int starpos, sunpos, moonpos, moonphase, i, linenum;
   WeatherCell *cell = getWeatherCell( ch->in_room->area );

   ch->pager( "You gaze up towards the heavens and see:\r\n" );

   if( isModeratelyCloudy( getCloudCover( cell ) ) )
   {
      ch->print( "There are too many clouds in the sky so you cannot see anything else.\r\n" );
      return;
   }

   sunpos = ( MAP_WIDTH * ( sysdata->hoursperday - time_info.hour ) / sysdata->hoursperday );
   moonpos = ( sunpos + time_info.day * MAP_WIDTH / sysdata->dayspermonth ) % MAP_WIDTH;

   if( ( moonphase = ( ( ( ( MAP_WIDTH + moonpos - sunpos ) % MAP_WIDTH ) + ( MAP_WIDTH / 16 ) ) * 8 ) / MAP_WIDTH ) > 4 )
      moonphase -= 8;
   starpos = ( sunpos + MAP_WIDTH * time_info.month / sysdata->monthsperyear ) % MAP_WIDTH;

   /*
    * The left end of the star_map will be straight overhead at midnight during month 0 
    */
   for( linenum = 0; linenum < MAP_HEIGHT; ++linenum )
   {
      if( ( time_info.hour >= sysdata->hoursunrise && time_info.hour <= sysdata->hoursunset ) && ( linenum < 3 || linenum >= 6 ) )
         continue;

      buf = " ";

      /*
       * for ( i = MAP_WIDTH/4; i <= 3*MAP_WIDTH/4; ++i )
       */
      for( i = 1; i <= MAP_WIDTH; ++i )
      {
         /*
          * plot moon on top of anything else...unless new moon & no eclipse 
          */
         if( ( time_info.hour >= sysdata->hoursunrise && time_info.hour <= sysdata->hoursunset )   /* daytime? */
             && ( moonpos >= MAP_WIDTH / 4 - 2 ) && ( moonpos <= 3 * MAP_WIDTH / 4 + 2 )  /* in sky? */
             && ( i >= moonpos - 2 ) && ( i <= moonpos + 2 )   /* is this pixel near moon? */
             && ( ( sunpos == moonpos && time_info.hour == sysdata->hournoon ) || moonphase != 0 ) /*no eclipse */
             && ( moon_map[linenum - 3][i + 2 - moonpos] == '@' ) )
         {
            if( ( moonphase < 0 && i - 2 - moonpos >= moonphase ) || ( moonphase > 0 && i + 2 - moonpos <= moonphase ) )
               buf.append( "&W@" );
            else
               buf.append( " " );
         }
         else if( ( linenum >= 3 ) && ( linenum < 6 ) && /* nighttime */
                  ( moonpos >= MAP_WIDTH / 4 - 2 ) && ( moonpos <= 3 * MAP_WIDTH / 4 + 2 )   /* in sky? */
                  && ( i >= moonpos - 2 ) && ( i <= moonpos + 2 ) /* is this pixel near moon? */
                  && ( moon_map[linenum - 3][i + 2 - moonpos] == '@' ) )
         {
            if( ( moonphase < 0 && i - 2 - moonpos >= moonphase ) || ( moonphase > 0 && i + 2 - moonpos <= moonphase ) )
               buf.append( "&W@" );
            else
               buf.append( " " );
         }
         else  /* plot sun or stars */
         {
            if( time_info.hour >= sysdata->hoursunrise && time_info.hour <= sysdata->hoursunset )  /* daytime */
            {
               if( i >= sunpos - 2 && i <= sunpos + 2 )
               {
                  buf.append( std::format( "&Y{}", sun_map[linenum - 3][i + 2 - sunpos] ) );
               }
               else
                  buf.append( " " );
            }
            else
            {
               switch ( star_map[linenum][( MAP_WIDTH + i - starpos ) % MAP_WIDTH] )
               {
                  default:
                     buf.append( " " );
                     break;
                  case ':':
                     buf.append( ":" );
                     break;
                  case '.':
                     buf.append( "." );
                     break;
                  case '*':
                     buf.append( "*" );
                     break;
                  case 'G':
                     buf.append( "&G " );
                     break;
                  case 'g':
                     buf.append( "&g " );
                     break;
                  case 'R':
                     buf.append( "&R " );
                     break;
                  case 'r':
                     buf.append( "&r " );
                     break;
                  case 'C':
                     buf.append( "&C " );
                     break;
                  case 'O':
                     buf.append( "&O " );
                     break;
                  case 'B':
                     buf.append( "&B " );
                     break;
                  case 'P':
                     buf.append( "&P " );
                     break;
                  case 'W':
                     buf.append( "&W " );
                     break;
                  case 'b':
                     buf.append( "&b " );
                     break;
                  case 'p':
                     buf.append( "&p " );
                     break;
                  case 'Y':
                     buf.append( "&Y " );
                     break;
                  case 'c':
                     buf.append( "&c " );
                     break;
               }
            }
         }
      }
      buf.append( "&D\r\n" );
      ch->pager( buf );
   }
}

/*
 * Show fancy descriptions for certain spell affects		-Thoric
 */
void show_visible_affects_to_char( char_data * victim, char_data * ch )
{
   std::string name;

   if( victim->isnpc(  ) )
      name = victim->short_descr;
   else
      name = victim->name;
   name[0] = to_upper( name.front() );

   if( victim->has_aflag( AFF_SANCTUARY ) )
   {
      if( victim->IS_GOOD(  ) )
         ch->print_fmt( "&W{} glows with an aura of divine radiance.\r\n", name );
      else if( victim->IS_EVIL(  ) )
         ch->print_fmt( "&z{} shimmers beneath an aura of dark energy.\r\n", name );
      else
         ch->print_fmt( "&w{} is shrouded in flowing shadow and light.\r\n", name );
   }
   if( victim->has_aflag( AFF_BLADEBARRIER ) )
      ch->print_fmt( "&w{} is surrounded by a spinning barrier of sharp blades.\r\n", name );
   if( victim->has_aflag( AFF_FIRESHIELD ) )
      ch->print_fmt( "&[fire]{} is engulfed within a blaze of mystical flame.\r\n", name );
   if( victim->has_aflag( AFF_SHOCKSHIELD ) )
      ch->print_fmt( "&B{} is surrounded by cascading torrents of energy.\r\n", name );
   if( victim->has_aflag( AFF_ACIDMIST ) )
      ch->print_fmt( "&G{} is visible through a cloud of churning mist.\r\n", name );
   if( victim->has_aflag( AFF_VENOMSHIELD ) )
      ch->print_fmt( "&g{} is enshrouded in a choking cloud of gas.\r\n", name );
   if( victim->has_aflag( AFF_HASTE ) )
      ch->print_fmt( "&Y{} appears to be slightly blurred.\r\n", name );
   if( victim->has_aflag( AFF_SLOW ) )
      ch->print_fmt( "&[magic]{} appears to be moving very slowly.\r\n", name );
   /*
    * Scryn 8/13
    */
   if( victim->has_aflag( AFF_ICESHIELD ) )
      ch->print_fmt( "&C{} is ensphered by shards of glistening ice.\r\n", name );
   if( victim->has_aflag( AFF_CHARM ) )
      ch->print_fmt( "&[magic]{} follows %s around everywhere.\r\n", name, victim->master == ch ? "you" : victim->master->name );
   if( !victim->isnpc(  ) && !victim->desc && victim->switched && victim->switched->has_aflag( AFF_POSSESS ) )
      ch->print_fmt( "&[magic]{} appears to be in a deep trance...\r\n", PERS( victim, ch, false ) );
}

void show_condition( char_data * ch, char_data * victim )
{
   std::string buf;
   int percent;

   if( victim->max_hit > 0 )
      percent = ( int )( ( 100.0 * ( double )( victim->hit ) ) / ( double )( victim->max_hit ) );
   else
      percent = -1;

   if( victim != ch )
   {
      buf = PERS( victim, ch, false );
      if( percent >= 100 )
         buf += " is in perfect health.\r\n";
      else if( percent >= 90 )
         buf += " is slightly scratched.\r\n";
      else if( percent >= 80 )
         buf += " has a few bruises.\r\n";
      else if( percent >= 70 )
         buf += " has some cuts.\r\n";
      else if( percent >= 60 )
         buf += " has several wounds.\r\n";
      else if( percent >= 50 )
         buf += " has many nasty wounds.\r\n";
      else if( percent >= 40 )
         buf += " is bleeding freely.\r\n";
      else if( percent >= 30 )
         buf += " is covered in blood.\r\n";
      else if( percent >= 20 )
         buf += " is leaking guts.\r\n";
      else if( percent >= 10 )
         buf += " is almost dead.\r\n";
      else
         buf += " is DYING.\r\n";
   }
   else
   {
      buf = "You";
      if( percent >= 100 )
         buf += " are in perfect health.\r\n";
      else if( percent >= 90 )
         buf += " are slightly scratched.\r\n";
      else if( percent >= 80 )
         buf += " have a few bruises.\r\n";
      else if( percent >= 70 )
         buf += " have some cuts.\r\n";
      else if( percent >= 60 )
         buf += " have several wounds.\r\n";
      else if( percent >= 50 )
         buf += " have many nasty wounds.\r\n";
      else if( percent >= 40 )
         buf += " are bleeding freely.\r\n";
      else if( percent >= 30 )
         buf += " are covered in blood.\r\n";
      else if( percent >= 20 )
         buf += " are leaking guts.\r\n";
      else if( percent >= 10 )
         buf += " are almost dead.\r\n";
      else
         buf += " are DYING.\r\n";
   }

   buf[0] = to_upper( buf[0] );
   ch->print( buf );
}

/* Gave a reason buffer to PCFLAG_AFK -Whir - 8/31/98 */
void show_char_to_char_0( char_data * victim, char_data * ch, int num )
{
   std::string buf = "";

   if( !ch->can_see( victim, true ) )
      return;

   ch->set_color( AT_PERSON );
   if( !victim->isnpc(  ) && !victim->desc )
   {
      if( !victim->switched )
         buf.append( "[(Link Dead)] " );
      else if( !victim->has_aflag( AFF_POSSESS ) )
         buf.append( "(Switched) " );
   }
   if( victim->isnpc(  ) && victim->has_aflag( AFF_POSSESS ) && ch->is_immortal(  ) && victim->desc )
      buf.append( std::format( "({})", victim->desc->original->name ) );
   if( victim->has_pcflag( PCFLAG_AFK ) )
   {
      if( !victim->pcdata->afkbuf.empty() )
         buf.append( std::format( "[AFK {}] ", victim->pcdata->afkbuf ) );
      else
         buf.append( "[AFK] " );
   }

   if( victim->has_pcflag( PCFLAG_WIZINVIS ) || victim->has_actflag( ACT_MOBINVIS ) )
   {
      if( !victim->isnpc(  ) )
         buf.append( std::format( "(Invis {}) ", victim->pcdata->wizinvis ) );
      else
         buf.append( std::format( "(Mobinvis {}) ", victim->mobinvis ) );
   }

   if( !victim->isnpc(  ) && victim->pcdata->clan && !victim->pcdata->clan->badge.empty(  ) && ( victim->pcdata->clan->clan_type != CLAN_GUILD ) )
      ch->printf( "%s ", victim->pcdata->clan->badge.c_str(  ) );
   else
      ch->set_color( AT_PERSON );

   if( victim->has_aflag( AFF_INVISIBLE ) )
      buf.append( "(Invis) " );
   if( victim->has_aflag( AFF_HIDE ) )
      buf.append( "(Hiding) " );
   if( victim->has_aflag( AFF_PASS_DOOR ) )
      buf.append( "(Translucent) " );
   if( victim->has_aflag( AFF_FAERIE_FIRE ) )
      buf.append( "(Pink Aura) " );
   if( victim->IS_EVIL(  ) && ( ch->has_aflag( AFF_DETECT_EVIL ) || ch->Class == CLASS_PALADIN ) )
      buf.append( "(Red Aura) " );
   if( victim->IS_NEUTRAL(  ) && ch->Class == CLASS_PALADIN )
      buf.append( "(Grey Aura) " );
   if( victim->IS_GOOD(  ) && ch->Class == CLASS_PALADIN )
      buf.append( "(White Aura) " );
   if( victim->has_aflag( AFF_BERSERK ) )
      buf.append( "(Wild-eyed) " );
   if( victim->has_pcflag( PCFLAG_LITTERBUG ) )
      buf.append( "(LITTERBUG) " );
   if( ch->is_immortal(  ) && victim->has_actflag( ACT_PROTOTYPE ) )
      buf.append( "(PROTO) " );
   if( victim->isnpc(  ) && ch->mount && ch->mount == victim && ch->in_room == ch->mount->in_room )
      buf.append( "(Mount) " );
   if( victim->desc && victim->desc->connected == CON_EDITING )
      buf.append( "(Writing) " );

   ch->set_color( AT_PERSON );
   if( ( victim->position == victim->defposition && !victim->long_descr.empty() )
       || ( victim->morph && victim->morph->morph && victim->morph->morph->defpos == victim->position ) )
   {
      if( victim->morph != nullptr )
      {
         if( !ch->is_immortal(  ) )
         {
            if( victim->morph->morph != nullptr )
               buf.append( victim->morph->morph->long_desc );
            else
               buf.append( strip_crlf( victim->long_descr ) );
         }
         else
         {
            buf.append( PERS( victim, ch, false ) );
            if( !ch->has_pcflag( PCFLAG_BRIEF ) && !victim->isnpc(  ) )
               buf.append( victim->pcdata->title );
            buf.append( "." );
         }
      }
      else
         buf.append( strip_crlf( victim->long_descr ) );

      if( num > 1 && victim->isnpc(  ) )
         buf.append( std::format( " ({})", num ) );
      buf.append( "\r\n" );
      ch->print( buf );
      show_visible_affects_to_char( victim, ch );
      return;
   }
   else
   {
      if( victim->morph != nullptr && victim->morph->morph != nullptr && !ch->is_immortal(  ) )
         buf.append( MORPHPERS( victim, ch, false ) );
      else
         buf.append( PERS( victim, ch, false ) );
   }

   if( !ch->has_pcflag( PCFLAG_BRIEF ) && !victim->isnpc(  ) )
      buf.append( victim->pcdata->title );

   buf.append( ch->color_str( AT_PERSON ) );

   timer_data *timer;
   if( ( timer = victim->get_timerptr( TIMER_DO_FUN ) ) != nullptr )
   {
      if( timer->do_fun == do_cast )
         buf.append( " is here chanting." );
      else if( timer->do_fun == do_dig )
         buf.append( " is here digging." );
      else if( timer->do_fun == do_search )
         buf.append( " is searching the area for something." );
      else if( timer->do_fun == do_detrap )
         buf.append( " is working with the trap here." );
      else
         buf.append( " is looking rather lost." );
   }
   else
   {
      /*
       * Furniture ideas taken from ROT. Furniture 1.01 is provided by Xerves.
       * * Info rewrite for sleeping/resting/standing/sitting on Objects -- Xerves
       */
      switch ( victim->position )
      {
         default:
            buf.append( " is... wait... WTF?" );
            break;

         case POS_DEAD:
            buf.append( " is DEAD!!" );
            break;

         case POS_MORTAL:
            buf.append( " is mortally wounded." );
            break;

         case POS_INCAP:
            buf.append( " is incapacitated." );
            break;

         case POS_STUNNED:
            buf.append( " is lying here stunned." );
            break;

         case POS_SLEEPING:
            if( victim->on != nullptr )
            {
               if( IS_SET( victim->on->value[2], SLEEP_AT ) )
                  buf.append( std::format( " is sleeping at {}.", victim->on->short_descr ) );
               else if( IS_SET( victim->on->value[2], SLEEP_ON ) )
                  buf.append( std::format( " is sleeping on {}.", victim->on->short_descr ) );
               else
                  buf.append( std::format( " is sleeping in {}.", victim->on->short_descr ) );
            }
            else
            {
               if( ch->position == POS_SITTING || ch->position == POS_RESTING )
                  buf.append( " is sleeping nearby." );
               else
                  buf.append( " is deep in slumber here." );
            }
            break;

         case POS_RESTING:
            if( victim->on != nullptr )
            {
               if( IS_SET( victim->on->value[2], REST_AT ) )
                  buf.append( std::format( " is resting at {}.", victim->on->short_descr ) );
               else if( IS_SET( victim->on->value[2], REST_ON ) )
                  buf.append( std::format( " is resting on {}.", victim->on->short_descr ) );
               else
                  buf.append( std::format( " is resting in %s.", victim->on->short_descr ) );
            }
            else
            {
               if( ch->position == POS_RESTING )
                  buf.append( " is sprawled out alongside you." );
               else if( ch->position == POS_MOUNTED )
                  buf.append( " is sprawled out at the foot of your mount." );
               else
                  buf.append( " is sprawled out here." );
            }
            break;

         case POS_SITTING:
            if( victim->on != nullptr )
            {
               if( IS_SET( victim->on->value[2], SIT_AT ) )
                  buf.append( std::format( " is sitting at {}.", victim->on->short_descr ) );
               else if( IS_SET( victim->on->value[2], SIT_ON ) )
                  buf.append( std::format( " is sitting on {}.", victim->on->short_descr ) );
               else
                  buf.append( std::format( " is sitting in {}.", victim->on->short_descr ) );
            }
            else
               buf.append( " is sitting here." );
            break;

         case POS_STANDING:
            if( victim->on != nullptr )
            {
               if( IS_SET( victim->on->value[2], STAND_AT ) )
                  buf.append( std::format( " is standing at {}.", victim->on->short_descr ) );
               else if( IS_SET( victim->on->value[2], STAND_ON ) )
                  buf.append( std::format( " is standing on {}.", victim->on->short_descr ) );
               else
                  buf.append( std::format( " is standing in {}.", victim->on->short_descr ) );
            }
            else if( victim->is_immortal(  ) )
               buf.append( " radiates with a godly light." );
            else if( ( victim->in_room->sector_type == SECT_UNDERWATER ) && !victim->has_aflag( AFF_AQUA_BREATH ) && !victim->isnpc(  ) )
               buf.append( " is drowning here." );
            else if( victim->in_room->sector_type == SECT_UNDERWATER )
               buf.append( " is here in the water." );
            else if( ( victim->in_room->sector_type == SECT_OCEANFLOOR ) && !victim->has_aflag( AFF_AQUA_BREATH ) && !victim->isnpc(  ) )
               buf.append( " is drowning here." );
            else if( victim->in_room->sector_type == SECT_OCEANFLOOR )
               buf.append( " is standing here in the water." );
            else if( victim->has_aflag( AFF_FLOATING ) || victim->has_aflag( AFF_FLYING ) )
               buf.append( " is hovering here." );
            else
               buf.append( " is standing here." );
            break;

         case POS_SHOVE:
            buf.append( " is being shoved around." );
            break;

         case POS_DRAG:
            buf.append( " is being dragged around." );
            break;

         case POS_MOUNTED:
            buf.append( " is here, upon " );
            if( !victim->mount )
               buf.append( "thin air???" );
            else if( victim->mount == ch )
               buf.append( "your back." );
            else if( victim->in_room == victim->mount->in_room )
            {
               buf.append( PERS( victim->mount, ch, false ) );
               buf.append( "." );
            }
            else
               buf.append( "someone who left??" );
            break;

         case POS_FIGHTING:
         case POS_EVASIVE:
         case POS_DEFENSIVE:
         case POS_AGGRESSIVE:
         case POS_BERSERK:
            buf.append( " is here, fighting " );
            if( !victim->fighting )
            {
               buf.append( "thin air???" );

               /*
                * some bug somewhere.... kinda hackey fix -h 
                */
               if( !victim->mount )
                  victim->position = POS_STANDING;
               else
                  victim->position = POS_MOUNTED;
            }
            else if( victim->who_fighting(  ) == ch )
               buf.append( "YOU!" );
            else if( victim->in_room == victim->fighting->who->in_room )
            {
               buf.append( PERS( victim->fighting->who, ch, false ) );
               buf.append( "." );
            }
            else
               buf.append( "someone who left??" );
            break;
      }
   }

   if( num > 1 && victim->isnpc(  ) )
      buf.append( std::format( " ({})", num ) );

   buf.append( "\r\n" );
   buf = capitalize( buf );

   ch->print( buf );
   show_visible_affects_to_char( victim, ch );
}

void show_race_line( char_data * ch, char_data * victim )
{
   int feet, inches;

   if( !ch->isnpc(  ) && ( victim != ch ) )
   {
      feet = victim->height / 12;
      inches = victim->height % 12;
      if( ch->is_immortal(  ) )
         ch->print_fmt( "{} is a level {} {} {}.\r\n", victim->name, victim->level, npc_race[victim->race], npc_class[victim->Class] );

      ch->print_fmt( "{} is {}'{}\" and weighs {} pounds.\r\n", PERS( victim, ch, false ), feet, inches, victim->weight );
      return;
   }

   if( !victim->isnpc(  ) && ( victim == ch ) )
   {
      feet = victim->height / 12;
      inches = victim->height % 12;
      ch->print_fmt( "You are a level {} {} {}.\r\n", victim->level, npc_race[victim->race], npc_class[victim->Class] );
      ch->print_fmt( "You are {}'{}\" and weigh {} pounds.\r\n", feet, inches, victim->weight );
      return;
   }
}

void show_char_to_char_1( char_data * victim, char_data * ch )
{
   int iWear;
   bool found;

   if( victim->can_see( ch, false ) && !ch->has_pcflag( PCFLAG_WIZINVIS ) )
   {
      act( AT_ACTION, "$n looks at you.", ch, nullptr, victim, TO_VICT );
      if( victim != ch )
         act( AT_ACTION, "$n looks at $N.", ch, nullptr, victim, TO_NOTVICT );
      else
         act( AT_ACTION, "$n looks at $mself.", ch, nullptr, victim, TO_NOTVICT );
   }

   if( !victim->chardesc.empty() )
   {
      if( victim->morph != nullptr && victim->morph->morph != nullptr )
      {
         if( !victim->morph->morph->description.empty() )
            ch->print( victim->morph->morph->description );
         else
            act( AT_PLAIN, "You see nothing special about $M.", ch, nullptr, victim, TO_CHAR );
      }
      else
         ch->print( victim->chardesc );
   }
   else
   {
      if( victim->morph != nullptr && victim->morph->morph != nullptr && !victim->morph->morph->description.empty() )
         ch->print( victim->morph->morph->description );
      else if( victim->isnpc(  ) )
         act( AT_PLAIN, "You see nothing special about $M.", ch, nullptr, victim, TO_CHAR );
      else if( ch != victim )
         act( AT_PLAIN, "$E isn't much to look at...", ch, nullptr, victim, TO_CHAR );
      else
         act( AT_PLAIN, "You're not much to look at...", ch, nullptr, nullptr, TO_CHAR );
   }

   show_race_line( ch, victim );
   show_condition( ch, victim );

   found = false;
   for( iWear = 0; iWear < MAX_WEAR; ++iWear )
   {
      obj_data *obj;

      if( ( obj = victim->get_eq( iWear ) ) != nullptr && ch->can_see_obj( obj, false ) )
      {
         if( !found )
         {
            ch->print( "\r\n" );
            if( victim != ch )
               act( AT_PLAIN, "$N is using:", ch, nullptr, victim, TO_CHAR );
            else
               act( AT_PLAIN, "You are using:", ch, nullptr, nullptr, TO_CHAR );
            found = true;
         }

         // Used to block NPCs here, but the race_table has always supported them so not sure why it did that.
         ch->set_color( AT_OBJECT );
         if( victim->race > 0 && victim->race < MAX_PC_RACE )
         {
            if( !is_valid_wear_loc( victim, iWear ) )
               continue;

            if( victim->has_bparts() )
               ch->print( victim->bodypart_where_names[iWear] );
            else
               ch->print( race_table[victim->race]->bodypart_where_names[iWear] );
         }
         else
            ch->print( where_names[iWear] );

         ch->print_fmt( "{}\r\n", obj->format_to_char( ch, true, 1 ) );
      }
   }

   /*
    * Crash fix here by Thoric
    */
   if( ch->isnpc(  ) || victim == ch )
      return;

   if( ch->is_immortal(  ) )
   {
      if( victim->isnpc(  ) )
         ch->print_fmt( "\r\nMobile #{} '{}' ", victim->pIndexData->vnum, victim->name );
      else
         ch->print_fmt( "\r\n{} ", victim->name );

      ch->print_fmt( "is a level {} {} {}.\r\n",
                  victim->level,
                  victim->isnpc(  )? victim->race < MAX_NPC_RACE && victim->race >= 0 ?
                  npc_race[victim->race] : "unknown" : victim->race < MAX_PC_RACE &&
                  !race_table[victim->race]->race_name.empty() ?
                  race_table[victim->race]->race_name : "unknown",
                  victim->isnpc(  )? victim->Class < MAX_NPC_CLASS && victim->Class >= 0 ?
                  npc_class[victim->Class] : "unknown" : victim->Class < MAX_PC_CLASS &&
                  !class_table[victim->Class]->who_name.empty() ? class_table[victim->Class]->who_name : "unknown" );
   }

   if( number_percent(  ) < ch->LEARNED( gsn_peek ) )
   {
      ch->print_fmt( "\r\nYou peek at {} inventory:\r\n", victim->sex == SEX_MALE ? "his" : victim->sex == SEX_FEMALE ? "her" : "its" );

      show_list_to_char( ch, victim->carrying, true, true );
   }
   else if( ch->pcdata->learned[gsn_peek] > 0 )
      ch->learn_from_failure( gsn_peek );
}

bool is_same_mob( char_data * i, char_data * j )
{
   if( !i->isnpc(  ) || !j->isnpc(  ) )
      return false;

   if( i->pIndexData == j->pIndexData && i->position == j->position &&
       i->get_aflags(  ) == j->get_aflags(  ) && i->get_actflags(  ) == j->get_actflags(  ) &&
       !str_cmp( i->name, j->name ) && !str_cmp( i->short_descr, j->short_descr ) &&
       !str_cmp( i->long_descr, j->long_descr ) &&
       ( ( !i->chardesc.empty() && !j->chardesc.empty() ) &&
       !str_cmp( i->chardesc, j->chardesc ) ) && is_same_char_map( i, j ) )
      return true;

   return false;
}

void show_char_to_char( char_data * ch )
{
   std::map<char_data *, int> chmap;
   std::map<char_data *, int>::iterator mch;
   bool found = false;

   chmap.clear(  );
   std::list<char_data *>::iterator ich;
   for( ich = ch->in_room->people.begin(  ); ich != ch->in_room->people.end(  ); ++ich )
   {
      char_data *rch = *ich;

      if( rch == ch || ( rch == supermob && !ch->is_imp(  ) ) )
         continue;

      found = false;
      for( mch = chmap.begin(  ); mch != chmap.end(  ); ++mch )
      {
         if( is_same_mob( mch->first, rch ) )
         {
            mch->second += 1;
            found = true;
            break;
         }
      }
      if( !found )
         chmap[rch] = 1;
   }

   mch = chmap.begin(  );
   while( mch != chmap.end(  ) )
   {
      if( ch->can_see( mch->first, false ) && !is_ignoring( mch->first, ch ) )
         show_char_to_char_0( mch->first, ch, mch->second );
      else if( ch->in_room->is_dark( ch ) && ch->has_aflag( AFF_INFRARED ) )
         ch->print( "&[blood]The red form of a living creature is here.&D\r\n" );
      ++mch;
   }
}

bool check_blind( char_data * ch )
{
   if( ch->has_pcflag( PCFLAG_HOLYLIGHT ) )
      return true;

   if( ch->has_aflag( AFF_TRUESIGHT ) )
      return true;

   if( ch->has_aflag( AFF_BLIND ) )
   {
      ch->print( "You can't see a thing!\r\n" );
      return false;
   }
   return true;
}

/*
 * Returns classical DIKU door direction based on text in arg	-Thoric
 */
int get_door( std::string_view arg )
{
   int door;

   if( !str_cmp( arg, "n" ) || !str_cmp( arg, "north" ) )
      door = 0;
   else if( !str_cmp( arg, "e" ) || !str_cmp( arg, "east" ) )
      door = 1;
   else if( !str_cmp( arg, "s" ) || !str_cmp( arg, "south" ) )
      door = 2;
   else if( !str_cmp( arg, "w" ) || !str_cmp( arg, "west" ) )
      door = 3;
   else if( !str_cmp( arg, "u" ) || !str_cmp( arg, "up" ) )
      door = 4;
   else if( !str_cmp( arg, "d" ) || !str_cmp( arg, "down" ) )
      door = 5;
   else if( !str_cmp( arg, "ne" ) || !str_cmp( arg, "northeast" ) )
      door = 6;
   else if( !str_cmp( arg, "nw" ) || !str_cmp( arg, "northwest" ) )
      door = 7;
   else if( !str_cmp( arg, "se" ) || !str_cmp( arg, "southeast" ) )
      door = 8;
   else if( !str_cmp( arg, "sw" ) || !str_cmp( arg, "southwest" ) )
      door = 9;
   else
      door = -1;
   return door;
}

CMDF( do_exits )
{
   std::string buf;

   bool fAuto = !str_cmp( argument, "auto" );

   if( !check_blind( ch ) )
      return;

   if( !ch->has_pcflag( PCFLAG_HOLYLIGHT ) && !ch->has_aflag( AFF_TRUESIGHT ) && ch->in_room->is_dark( ch ) )
   {
      ch->print( "&zIt is pitch black ... \r\n" );
      return;
   }

   ch->set_color( AT_EXITS );

   buf = ( fAuto ? "[Exits:" : "Obvious exits:\r\n" );

   bool found = false;
   for( auto* pexit : ch->in_room->exits )
   {
      /*
       * Immortals see all exits, even secret ones 
       */
      if( ch->is_immortal(  ) )
      {
         if( pexit->to_room )
         {
            found = true;
            if( fAuto )
            {
               buf.append( " " );

               buf.append( capitalize( dir_name[pexit->vdir] ) );

               if( IS_EXIT_FLAG( pexit, EX_OVERLAND ) )
                  buf.append( "->(Overland)" );

               /*
                * New code added to display closed, or otherwise invisible exits to immortals 
                * Installed by Samson 1-25-98 
                */
               if( IS_EXIT_FLAG( pexit, EX_CLOSED ) )
                  buf.append( "->(Closed)" );
               if( IS_EXIT_FLAG( pexit, EX_DIG ) )
                  buf.append( "->(Dig)" );
               if( IS_EXIT_FLAG( pexit, EX_WINDOW ) )
                  buf.append( "->(Window)" );
               if( IS_EXIT_FLAG( pexit, EX_HIDDEN ) )
                  buf.append( "->(Hidden)" );
               if( pexit->to_room->flags.test( ROOM_DEATH ) )
                  buf.append( "->(Deathtrap)" );
            }
            else
            {
               buf.append( std::format( "{} - {}\r\n", capitalize( dir_name[pexit->vdir] ), pexit->to_room->name ) );

               /*
                * More new code added to display closed, or otherwise invisible exits to immortals 
                * Installed by Samson 1-25-98 
                */
               if( pexit->to_room->is_dark( ch ) )
                  buf.append( " (Dark)" );
               if( IS_EXIT_FLAG( pexit, EX_CLOSED ) )
                  buf.append( " (Closed)" );
               if( IS_EXIT_FLAG( pexit, EX_DIG ) )
                  buf.append( " (Dig)" );
               if( IS_EXIT_FLAG( pexit, EX_HIDDEN ) )
                  buf.append( " (Hidden)" );
               if( IS_EXIT_FLAG( pexit, EX_WINDOW ) )
                  buf.append( " (Window)" );
               if( pexit->to_room->flags.test( ROOM_DEATH ) )
                  buf.append( " (Deathtrap)" );
               buf.append( "\r\n" );
            }
         }
      }
      else
      {
         if( pexit->to_room && !IS_EXIT_FLAG( pexit, EX_SECRET ) && ( !IS_EXIT_FLAG( pexit, EX_WINDOW ) || IS_EXIT_FLAG( pexit, EX_ISDOOR ) ) && !IS_EXIT_FLAG( pexit, EX_HIDDEN ) && !IS_EXIT_FLAG( pexit, EX_FORTIFIED ) /* Checks for walls, Marcus */
             && !IS_EXIT_FLAG( pexit, EX_HEAVY ) && !IS_EXIT_FLAG( pexit, EX_MEDIUM ) && !IS_EXIT_FLAG( pexit, EX_LIGHT ) && !IS_EXIT_FLAG( pexit, EX_CRUMBLING ) )
         {
            found = true;
            if( fAuto )
            {
               buf.append( " " );

               buf.append( capitalize( dir_name[pexit->vdir] ) );

               if( IS_EXIT_FLAG( pexit, EX_CLOSED ) && !IS_EXIT_FLAG( pexit, EX_DIG ) )
                  buf.append( "->(Closed)" );
               if( ch->has_aflag( AFF_DETECTTRAPS ) && pexit->to_room->flags.test( ROOM_DEATH ) )
                  buf.append( "->(Deathtrap)" );
            }
            else
            {
               buf.append( std::format( "{} - {}\r\n", capitalize( dir_name[pexit->vdir] ),
                         pexit->to_room->is_dark( ch ) ? "Too dark to tell" : pexit->to_room->name ) );
            }
         }
      }
   }

   if( !found )
      buf.append( fAuto ? " none]" : "None]" );
   else
   {
      if( fAuto )
         buf.append( "]" );
   }

   buf.append( "\r\n" );
   ch->print( buf );
}

void print_compass( char_data * ch )
{
   int exit_info[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
   static const char *exit_colors[] = { "&w", "&Y", "&C", "&b", "&w", "&R" };

   for( auto* pexit : ch->in_room->exits )
   {
      if( !pexit->to_room || IS_EXIT_FLAG( pexit, EX_HIDDEN ) || ( IS_EXIT_FLAG( pexit, EX_SECRET ) && IS_EXIT_FLAG( pexit, EX_CLOSED ) ) )
         continue;
      if( IS_EXIT_FLAG( pexit, EX_WINDOW ) )
         exit_info[pexit->vdir] = 2;
      else if( IS_EXIT_FLAG( pexit, EX_SECRET ) )
         exit_info[pexit->vdir] = 3;
      else if( IS_EXIT_FLAG( pexit, EX_CLOSED ) )
         exit_info[pexit->vdir] = 4;
      else if( IS_EXIT_FLAG( pexit, EX_LOCKED ) )
         exit_info[pexit->vdir] = 5;
      else
         exit_info[pexit->vdir] = 1;
   }

   ch->print_fmt( "\r\n&[rmname]{:<50}         {}{}    {}{}    {}{}\r\n",
               ch->in_room->name, exit_colors[exit_info[DIR_NORTHWEST]],
               exit_info[DIR_NORTHWEST] ? "NW" : "- ", exit_colors[exit_info[DIR_NORTH]], exit_info[DIR_NORTH] ? "N" : "-",
               exit_colors[exit_info[DIR_NORTHEAST]], exit_info[DIR_NORTHEAST] ? "NE" : " -" );
   if( ch->is_immortal(  ) && ch->has_pcflag( PCFLAG_ROOMVNUM ) )
      ch->print_fmt( "&w-<---- &YVnum: {:6} &w----------------------------->-        ", ch->in_room->vnum );
   else
      ch->print( "&w-<----------------------------------------------->-        " );
   ch->print_fmt( "{}{}&w<-{}{}&w-&W(*)&w-{}{}&w->{}{}\r\n", exit_colors[exit_info[DIR_WEST]], exit_info[DIR_WEST] ? "W" : "-",
               exit_colors[exit_info[DIR_UP]], exit_info[DIR_UP] ? "U" : "-", exit_colors[exit_info[DIR_DOWN]],
               exit_info[DIR_DOWN] ? "D" : "-", exit_colors[exit_info[DIR_EAST]], exit_info[DIR_EAST] ? "E" : "-" );
   ch->print_fmt( "                                                           {}{}    {}{}    {}{}\r\n",
               exit_colors[exit_info[DIR_SOUTHWEST]], exit_info[DIR_SOUTHWEST] ? "SW" : "- ",
               exit_colors[exit_info[DIR_SOUTH]], exit_info[DIR_SOUTH] ? "S" : "-", exit_colors[exit_info[DIR_SOUTHEAST]], exit_info[DIR_SOUTHEAST] ? "SE" : " -" );
}

const std::string roomdesc( char_data * ch )
{
   std::string rdesc;

   /*
    * view desc or nitedesc --  Dracones 
    */
   if( !ch->has_pcflag( PCFLAG_BRIEF ) )
   {
      if( time_info.hour >= sysdata->hoursunrise && time_info.hour <= sysdata->hoursunset )
      {
         if( !ch->in_room->roomdesc.empty() )
            rdesc = ch->in_room->roomdesc;
      }
      else
      {
         if( !ch->in_room->nitedesc.empty() )
            rdesc = ch->in_room->nitedesc;
         else if( !ch->in_room->roomdesc.empty() )
            rdesc = ch->in_room->roomdesc;
      }
   }
   if( rdesc.empty() )
      rdesc = "(Not set)";
   return rdesc;
}

void print_infoflags( char_data * ch )
{
   area_data *tarea = ch->in_room->area;

   /*
    * Room flag display installed by Samson 12-10-97 
    * Forget exactly who, but thanks to the Smaug archives :) 
    */
   if( ch->is_immortal(  ) && ch->has_pcflag( PCFLAG_AUTOFLAGS ) )
   {
      tarea = ch->in_room->area;

      ch->print_fmt( "&[aflags][Area Flags: {}]\r\n", ( tarea->flags.any(  ) ) ? bitset_string( tarea->flags, area_flags ) : "none" );
      ch->print_fmt( "&[rflags][Room Flags: {}]\r\n", bitset_string( ch->in_room->flags, r_flags ) );
   }

   /*
    * Room Sector display written and installed by Samson 12-10-97 
    * Added Continent/Plane flag display on 3-28-98 
    */
   if( ch->is_immortal(  ) && ch->has_pcflag( PCFLAG_SECTORD ) )
   {
      if( tarea->continent )
         ch->print_fmt( "&[stype][Sector Type: {}] [Continent or Plane: %s]\r\n", sect_types[ch->in_room->sector_type], tarea->continent->name );
      else
         ch->print_fmt( "&[stype][Sector Type: {}] [Continent or Plane: <NOT SET>]\r\n", sect_types[ch->in_room->sector_type] );
   }

   /*
    * Area name and filename display installed by Samson 12-13-97 
    */
   if( ch->is_immortal(  ) && ch->has_pcflag( PCFLAG_ANAME ) )
   {
      ch->print_fmt( "&[aname][Area name: {}]", ch->in_room->area->name );
      if( ch->level >= LEVEL_CREATOR )
         ch->print_fmt( "  [Area filename: {}]\r\n", ch->in_room->area->filename );
      else
         ch->print( "\r\n" );
   }
}

void print_roomname( char_data * ch )
{
   ch->set_color( AT_RMNAME );

   /*
    * Added immortal option to see vnum when looking - Samson 4-3-98 
    */
   ch->print( ch->in_room->name );
   if( ch->is_immortal(  ) && ch->has_pcflag( PCFLAG_ROOMVNUM ) )
      ch->print_fmt( "   &YVnum: {}", ch->in_room->vnum );

   ch->print( "\r\n" );
}

CMDF( do_showmap )
{
   draw_map( ch, roomdesc( ch ) );
}

CMDF( do_look )
{
   std::string arg, arg1, arg2;
   extra_descr_data *ed = nullptr;
   char_data *victim;
   obj_data *obj;
   room_index *original;
   int number, cnt;

   if( !ch->desc )
      return;

   if( ch->position < POS_SLEEPING )
   {
      ch->print( "You can't see anything but stars!\r\n" );
      return;
   }

   if( ch->position == POS_SLEEPING )
   {
      ch->print( "You can't see anything, you're sleeping!\r\n" );
      return;
   }

   if( !check_blind( ch ) )
      return;

   if( !ch->has_pcflag( PCFLAG_HOLYLIGHT ) && !ch->has_aflag( AFF_TRUESIGHT ) && ch->in_room->is_dark( ch ) )
   {
      ch->print( "&zIt is pitch black ... \r\n" );
      if( argument.empty(  ) || !str_cmp( argument, "auto" ) )
         show_char_to_char( ch );
      return;
   }

   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );

   if( arg1.empty(  ) || !str_cmp( arg1, "auto" ) )
   {
      if( ch->has_pcflag( PCFLAG_ONMAP ) || ch->has_actflag( ACT_ONMAP ) )
      {
         display_map( ch );
         if( !ch->inflight )
         {
            show_list_to_char( ch, ch->in_room->objects, false, false );
            show_char_to_char( ch );
         }
         return;
      }

      /*
       * 'look' or 'look auto' 
       */
      if( ch->has_pcflag( PCFLAG_COMPASS ) )
         print_compass( ch );
      else
         print_roomname( ch );

      /*
       * Moved the exits to be under the name of the room 
       * Yannick 24 september 1997                        
       * Added AUTOMAP check because it shows them next to the map now if its active 
       */
      if( ch->has_pcflag( PCFLAG_AUTOEXIT ) && !ch->has_pcflag( PCFLAG_AUTOMAP ) )
         do_exits( ch, "auto" );

      print_infoflags( ch );

      ch->set_color( AT_RMDESC );
      if( ch->has_pcflag( PCFLAG_AUTOMAP ) )
         draw_map( ch, roomdesc( ch ) );
      else
         ch->print( roomdesc( ch ) );

      if( !ch->isnpc(  ) && ch->hunting )
         do_track( ch, ch->hunting->who->name );

      show_list_to_char( ch, ch->in_room->objects, false, false );
      show_char_to_char( ch );
      return;
   }

   if( !str_cmp( arg1, "sky" ) || !str_cmp( arg1, "stars" ) )
   {
      if( ch->has_pcflag( PCFLAG_ONMAP ) || ch->has_actflag( ACT_ONMAP ) )
      {
         look_sky( ch );
         return;
      }

      if( !ch->IS_OUTSIDE(  ) || is_indoor_sector( ch->in_room->sector_type ) )
      {
         ch->print( "You can't see the sky from here.\r\n" );
         return;
      }
      else
      {
         look_sky( ch );
         return;
      }
   }

   if( !str_cmp( arg1, "board" ) )  /* New note interface - Samson */
   {
      if( !( obj = ch->get_obj_here( arg1 ) ) )
      {
         ch->print( "You do not see that here.\r\n" );
         return;
      }
      interpret( ch, "review" );
      return;
   }

   if( !str_cmp( arg1, "mailbag" ) )   /* New mail interface - Samson 4-29-99 */
   {
      if( !( obj = ch->get_obj_here( arg1 ) ) )
      {
         ch->print( "You do not see that here.\r\n" );
         return;
      }
      interpret( ch, "mail list" );
      return;
   }

   if( !str_cmp( arg1, "under" ) )
   {
      int count;

      /*
       * 'look under' 
       */
      if( arg2.empty(  ) )
      {
         ch->print( "Look beneath what?\r\n" );
         return;
      }

      if( !( obj = ch->get_obj_here( arg2 ) ) )
      {
         ch->print( "You do not see that here.\r\n" );
         return;
      }

      if( !obj->wear_flags.test( ITEM_TAKE ) && ch->level < sysdata->level_getobjnotake )
      {
         ch->print( "You can't seem to get a grip on it.\r\n" );
         return;
      }

      if( ch->carry_weight + obj->weight > ch->can_carry_w(  ) )
      {
         ch->print( "It's too heavy for you to look under.\r\n" );
         return;
      }

      count = obj->count;
      obj->count = 1;
      act( AT_PLAIN, "You lift $p and look beneath it:", ch, obj, nullptr, TO_CHAR );
      act( AT_PLAIN, "$n lifts $p and looks beneath it:", ch, obj, nullptr, TO_ROOM );
      obj->count = count;

      if( obj->extra_flags.test( ITEM_COVERING ) )
         show_list_to_char( ch, obj->contents, true, true );
      else
         ch->print( "Nothing.\r\n" );
      if( EXA_prog_trigger )
         oprog_examine_trigger( ch, obj );
      return;
   }

   /*
    * Look in 
    */
   if( !str_cmp( arg1, "i" ) || !str_cmp( arg1, "in" ) )
   {
      int count;

      /*
       * 'look in' 
       */
      if( arg2.empty(  ) )
      {
         ch->print( "Look in what?\r\n" );
         return;
      }

      if( !( obj = ch->get_obj_here( arg2 ) ) )
      {
         ch->print( "You do not see that here.\r\n" );
         return;
      }

      switch ( obj->item_type )
      {
         default:
            ch->print( "That is not a container.\r\n" );
            break;

         case ITEM_DRINK_CON:
            if( obj->value[1] <= 0 )
            {
               ch->print( "It is empty.\r\n" );
               if( EXA_prog_trigger )
                  oprog_examine_trigger( ch, obj );
               break;
            }
            {
               liquid_data *liq = get_liq_vnum( obj->value[2] );

               ch->print_fmt( "It's {} full of a {} liquid.\r\n", ( obj->value[1] * 10 ) < ( obj->value[0] * 10 ) / 4
                           ? "less than halfway" : ( obj->value[1] * 10 ) < 2 * ( obj->value[0] * 10 ) / 4
                           ? "around halfway" : ( obj->value[1] * 10 ) < 3 * ( obj->value[0] * 10 ) / 4
                           ? "more than halfway" : obj->value[1] == obj->value[0] ? "completely" : "almost", liq->color );
            }
            if( EXA_prog_trigger )
               oprog_examine_trigger( ch, obj );
            break;

         case ITEM_PORTAL:
            for( auto* pexit : ch->in_room->exits )
            {
               if( pexit->vdir == DIR_PORTAL && IS_EXIT_FLAG( pexit, EX_PORTAL ) )
               {
                  if( pexit->to_room->is_private(  ) && ch->level < sysdata->level_override_private )
                  {
                     ch->print( "&WThe room ahead is private!&D\r\n" );
                     return;
                  }

                  if( IS_EXIT_FLAG( pexit, EX_OVERLAND ) )
                  {
                     original = ch->in_room;
                     enter_map( ch, pexit, pexit->map_x, pexit->map_y, "-1" );
                     leave_map( ch, nullptr, original );
                  }
                  else
                  {
                     bool visited;

                     visited = ch->has_visited( pexit->to_room->area );
                     original = ch->in_room;
                     ch->from_room(  );
                     if( !ch->to_room( pexit->to_room ) )
                        log_printf( "char_to_room: {}:{}, line {}.", __FILE__, __func__, __LINE__ );
                     do_look( ch, "auto" );
                     ch->from_room(  );
                     if( !ch->to_room( original ) )
                        log_printf( "char_to_room: {}:{}, line {}.", __FILE__, __func__, __LINE__ );
                     if( !visited )
                        ch->remove_visit( pexit->to_room );
                  }
               }
            }
            ch->print( "You see swirling chaos...\r\n" );
            break;

         case ITEM_CONTAINER:
         case ITEM_QUIVER:
         case ITEM_KEYRING:
         case ITEM_CORPSE_NPC:
         case ITEM_CORPSE_PC:
            if( IS_SET( obj->value[1], CONT_CLOSED ) )
            {
               ch->print( "It is closed.\r\n" );
               break;
            }
            count = obj->count;
            obj->count = 1;
            if( obj->item_type == ITEM_KEYRING )
               act( AT_PLAIN, "$p holds:", ch, obj, nullptr, TO_CHAR );
            else
               act( AT_PLAIN, "$p contains:", ch, obj, nullptr, TO_CHAR );
            obj->count = count;

            show_list_to_char( ch, obj->contents, true, true );
            ch->print( "&GItems with a &W*&G after them have other items stored inside.\r\n" );
            if( EXA_prog_trigger )
               oprog_examine_trigger( ch, obj );
            break;
      }
      return;
   }

   /*
    * finally fixed the annoying look 2.obj desc bug  -Thoric 
    */
   number = number_argument( arg1, arg );
   std::list<obj_data *>::iterator iobj;
   for( cnt = 0, iobj = ch->carrying.begin(  ); iobj != ch->carrying.end(  ); ++iobj )
   {
      obj = *iobj;

      if( ch->can_see_obj( obj, false ) )
      {
         ed = get_extra_descr( arg, obj );
         if( ed )
         {
            if( ( cnt += obj->count ) < number )
               continue;
            ch->print( ed->desc );
            if( EXA_prog_trigger )
               oprog_examine_trigger( ch, obj );
            return;
         }

         ed = get_extra_descr( arg, obj->pIndexData );
         if( ed )
         {
            if( ( cnt += obj->count ) < number )
               continue;
            ch->print( ed->desc );
            if( EXA_prog_trigger )
               oprog_examine_trigger( ch, obj );
            return;
         }

         if( nifty_is_name_prefix( arg, obj->name ) )
         {
            if( ( cnt += obj->count ) < number )
               continue;
            ed = get_extra_descr( obj->name, obj->pIndexData );
            if( !ed )
               ed = get_extra_descr( obj->name, obj );
            if( !ed )
               ch->print( "You see nothing special.\r\n" );
            else
               ch->print( ed->desc );

            if( obj->item_type == ITEM_PUDDLE )
            {
               liquid_data *liq = get_liq_vnum( obj->value[2] );

               ch->print_fmt( "It's a puddle of {} liquid.\r\n", ( liq == nullptr ? "clear" : liq->color ) );
            }
            if( EXA_prog_trigger )
               oprog_examine_trigger( ch, obj );
            return;
         }
      }
   }

   for( iobj = ch->in_room->objects.begin(  ); iobj != ch->in_room->objects.end(  ); ++iobj )
   {
      obj = *iobj;

      if( ch->can_see_obj( obj, false ) )
      {
         if( ( ed = get_extra_descr( arg, obj ) ) )
         {
            if( ( cnt += obj->count ) < number )
               continue;
            ch->print( ed->desc );
            if( EXA_prog_trigger )
               oprog_examine_trigger( ch, obj );
            return;
         }

         if( ( ed = get_extra_descr( arg, obj->pIndexData ) ) )
         {
            if( ( cnt += obj->count ) < number )
               continue;
            ch->print( ed->desc );
            if( EXA_prog_trigger )
               oprog_examine_trigger( ch, obj );
            return;
         }

         if( nifty_is_name_prefix( arg, obj->name ) )
         {
            if( ( cnt += obj->count ) < number )
               continue;
            if( !( ed = get_extra_descr( obj->name, obj->pIndexData ) ) )
               ed = get_extra_descr( obj->name, obj );
            if( !ed )
               ch->print( "You see nothing special.\r\n" );
            else
               ch->print( ed->desc );
            if( obj->item_type == ITEM_PUDDLE )
            {
               liquid_data *liq = get_liq_vnum( obj->value[2] );

               ch->print_fmt( "It's a puddle of {} liquid.\r\n", ( liq == nullptr ? "clear" : liq->color ) );
            }
            if( EXA_prog_trigger )
               oprog_examine_trigger( ch, obj );
            return;
         }
      }
   }

   if( ( ed = get_extra_descr( arg1, ch->in_room ) ) )
   {
      ch->print( ed->desc );
      return;
   }

   exit_data *pexit;
   short door = get_door( arg1 );
   if( ( pexit = find_door( ch, arg1, true ) ) != nullptr )
   {
      if( IS_EXIT_FLAG( pexit, EX_CLOSED ) && !IS_EXIT_FLAG( pexit, EX_WINDOW ) )
      {
         if( ( IS_EXIT_FLAG( pexit, EX_SECRET ) || IS_EXIT_FLAG( pexit, EX_DIG ) ) && door != -1 )
            ch->print( "Nothing special there.\r\n" );
         else
         {
            if( pexit->keyword.back() == 's' || ( pexit->keyword.back() == '\'' && pexit->keyword.length() - 2 == 's' ) )
               act( AT_PLAIN, "The $d are closed.", ch, nullptr, pexit->keyword.c_str(), TO_CHAR );
            else
               act( AT_PLAIN, "The $d is closed.", ch, nullptr, pexit->keyword.c_str(), TO_CHAR );
         }
         return;
      }
      if( IS_EXIT_FLAG( pexit, EX_BASHED ) )
      {
         if( pexit->keyword.back() == 's' || ( pexit->keyword.back() == '\'' && pexit->keyword.length() - 2 == 's' ) )
            act( AT_PLAIN, "The $d have been bashed from their hinges.", ch, nullptr, pexit->keyword.c_str(), TO_CHAR );
         else
            act( AT_PLAIN, "The $d has been bashed from its hinges.", ch, nullptr, pexit->keyword.c_str(), TO_CHAR );
      }

      if( !pexit->exitdesc.empty() )
         ch->print_fmt( "{}\r\n", pexit->exitdesc );
      else
         ch->print( "Nothing special there.\r\n" );

      /*
       * Ability to look into the next room        -Thoric
       */
      if( pexit->to_room
          && ( ch->is_affected( gsn_spy ) || ch->is_affected( gsn_scout ) || ch->is_affected( gsn_scry ) || IS_EXIT_FLAG( pexit, EX_xLOOK ) || ch->level >= LEVEL_IMMORTAL ) )
      {
         if( !IS_EXIT_FLAG( pexit, EX_xLOOK ) && ch->level < LEVEL_IMMORTAL )
         {
            ch->set_color( AT_SKILL );
            /*
             * Change by Narn, Sept 96 to allow characters who don't have the
             * scry spell to benefit from objects that are affected by scry.
             * Except that I agree with DOTD logic - scrying doesn't work like this.
             * * Samson - 6-20-99
             */
            if( pexit->to_room->flags.test( ROOM_NOSCRY ) || pexit->to_room->area->flags.test( AFLAG_NOSCRY ) )
            {
               ch->print( "That room is magically protected. You cannot see inside.\r\n" );
               return;
            }

            if( !ch->isnpc(  ) )
            {
               int skill = -1, percent;

               if( ch->is_affected( gsn_spy ) )
                  skill = gsn_spy;
               if( ch->is_affected( gsn_scry ) )
                  skill = gsn_scry;
               if( ch->is_affected( gsn_scout ) )
                  skill = gsn_scout;

               if( skill == -1 )
                  skill = gsn_spy;

               percent = ch->pcdata->learned[skill];
               if( !percent )
                  percent = 35;  /* 95 was too good -Thoric */

               if( number_percent(  ) > percent )
               {
                  ch->print( "You cannot get a good look.\r\n" );
                  return;
               }
            }
         }
         if( pexit->to_room->is_private(  ) && ch->level < sysdata->level_override_private )
         {
            ch->print( "&WThe room ahead is private!&D\r\n" );
            return;
         }

         if( IS_EXIT_FLAG( pexit, EX_OVERLAND ) )
         {
            original = ch->in_room;
            enter_map( ch, pexit, pexit->map_x, pexit->map_y, "-1" );
            leave_map( ch, nullptr, original );
         }
         else
         {
            bool visited;

            visited = ch->has_visited( pexit->to_room->area );
            original = ch->in_room;
            ch->from_room(  );
            if( !ch->to_room( pexit->to_room ) )
               log_printf( "char_to_room: {}:{}, line {}.", __FILE__, __func__, __LINE__ );
            do_look( ch, "auto" );
            ch->from_room(  );
            if( !ch->to_room( original ) )
               log_printf( "char_to_room: {}:{}, line {}.", __FILE__, __func__, __LINE__ );
            if( !visited )
               ch->remove_visit( pexit->to_room );
         }
      }
      return;
   }
   else if( door != -1 )
   {
      ch->print( "Nothing special there.\r\n" );
      return;
   }

   if( ( victim = ch->get_char_room( arg1 ) ) != nullptr )
   {
      if( !is_ignoring( victim, ch ) )
      {
         show_char_to_char_1( victim, ch );
         return;
      }
   }

   ch->print( "You do not see that here.\r\n" );
}

/* A much simpler version of look, this function will show you only
the condition of a mob or pc, or if used without an argument, the
same you would see if you enter the room and have config +brief.
-- Narn, winter '96
*/
CMDF( do_glance )
{
   char_data *victim;
   bool brief;

   if( !ch->desc )
      return;

   if( ch->position < POS_SLEEPING )
   {
      ch->print( "You can't see anything but stars!\r\n" );
      return;
   }

   if( ch->position == POS_SLEEPING )
   {
      ch->print( "You can't see anything, you're sleeping!\r\n" );
      return;
   }

   if( !check_blind( ch ) )
      return;

   ch->set_color( AT_ACTION );

   if( argument.empty(  ) )
   {
      if( ch->has_pcflag( PCFLAG_BRIEF ) )
         brief = true;
      else
         brief = false;
      ch->set_pcflag( PCFLAG_BRIEF );
      do_look( ch, "auto" );
      if( !brief )
         ch->unset_pcflag( PCFLAG_BRIEF );
      return;
   }

   if( !( victim = ch->get_char_room( argument ) ) )
      ch->print( "They're not here.\r\n" );
   else
   {
      if( victim->can_see( ch, false ) )
      {
         act( AT_ACTION, "$n glances at you.", ch, nullptr, victim, TO_VICT );
         act( AT_ACTION, "$n glances at $N.", ch, nullptr, victim, TO_NOTVICT );
      }
      if( ch->is_immortal(  ) && victim != ch )
      {
         if( victim->isnpc(  ) )
            ch->print_fmt( "Mobile #{} '{}' ", victim->pIndexData->vnum, victim->name );
         else
            ch->print_fmt( "{} ", victim->name );
         ch->print_fmt( "is a level {} {} {}.\r\n", victim->level,
                     victim->isnpc(  )? victim->race < MAX_NPC_RACE && victim->race >= 0 ?
                     npc_race[victim->race] : "unknown" : victim->race < MAX_PC_RACE &&
                     !race_table[victim->race]->race_name.empty() ?
                     race_table[victim->race]->race_name : "unknown",
                     victim->isnpc(  )? victim->Class < MAX_NPC_CLASS && victim->Class >= 0 ?
                     npc_class[victim->Class] : "unknown" : victim->Class < MAX_PC_CLASS &&
                     !class_table[victim->Class]->who_name.empty() ? class_table[victim->Class]->who_name : "unknown" );
      }
      show_condition( ch, victim );
   }
}

CMDF( do_examine )
{
   std::string buf;
   obj_data *obj;
   short dam;

   if( !ch )
   {
      bug( "{}: null ch.", __func__ );
      return;
   }

   if( argument.empty(  ) )
   {
      ch->print( "Examine what?\r\n" );
      return;
   }

   EXA_prog_trigger = false;
   do_look( ch, argument );
   EXA_prog_trigger = true;

   /*
    * Support for checking equipment conditions,
    * and support for trigger positions by Thoric
    */
   if( ( obj = ch->get_obj_here( argument ) ) != nullptr )
   {
      switch ( obj->item_type )
      {
         default:
            break;

         case ITEM_ARMOR:
            ch->print_fmt( "Condition: {}\r\n", condtxt( obj->value[1], obj->value[0] ) );
            if( obj->value[2] > 0 )
               ch->print_fmt( "Available sockets: {}\r\n", obj->value[2] );
            if( !obj->socket[0].empty() && str_cmp( obj->socket[0], "None" ) )
               ch->print_fmt( "Socket 1: {} Rune\r\n", obj->socket[0] );
            if( !obj->socket[1].empty() && str_cmp( obj->socket[1], "None" ) )
               ch->print_fmt( "Socket 2: {} Rune\r\n", obj->socket[1] );
            if( !obj->socket[2].empty() && str_cmp( obj->socket[2], "None" ) )
               ch->print_fmt( "Socket 3: {} Rune\r\n", obj->socket[2] );
            break;

         case ITEM_WEAPON:
         case ITEM_MISSILE_WEAPON:
            ch->print_fmt( "Condition: {}\r\n", condtxt( obj->value[6], obj->value[0] ) );
            if( obj->value[7] > 0 )
               ch->print_fmt( "Available sockets: {}\r\n", obj->value[7] );
            if( !obj->socket[0].empty() && str_cmp( obj->socket[0], "None" ) )
               ch->print_fmt( "Socket 1: {} Rune\r\n", obj->socket[0] );
            if( !obj->socket[1].empty() && str_cmp( obj->socket[1], "None" ) )
               ch->print_fmt( "Socket 2: {} Rune\r\n", obj->socket[1] );
            if( !obj->socket[2].empty() && str_cmp( obj->socket[2], "None" ) )
               ch->print_fmt( "Socket 3: {} Rune\r\n", obj->socket[2] );
            break;

         case ITEM_PROJECTILE:
            ch->print_fmt( "Condition: {}\r\n", condtxt( obj->value[5], obj->value[0] ) );
            break;

         case ITEM_COOK:
            buf = "As you examine it carefully you notice that it ";
            dam = obj->value[2];
            if( dam >= 3 )
               buf += "is burned to a crisp.";
            else if( dam == 2 )
               buf += "is a little over cooked.";   /* Bugfix 5-18-99 */
            else if( dam == 1 )
               buf += "is perfectly roasted.";
            else
               buf += "is raw.";
            buf += "\r\n";
            ch->print( buf );

         case ITEM_FOOD:
            if( obj->timer > 0 && obj->value[1] > 0 )
               dam = ( obj->timer * 10 ) / obj->value[1];
            else
               dam = 10;
            if( obj->item_type == ITEM_FOOD )
               buf = "As you examine it carefully you notice that it ";
            else
               buf = "Also it ";
            if( dam >= 10 )
               buf += "is fresh.";
            else if( dam == 9 )
               buf += "is nearly fresh.";
            else if( dam == 8 )
               buf += "is perfectly fine.";
            else if( dam == 7 )
               buf += "looks good.";
            else if( dam == 6 )
               buf += "looks ok.";
            else if( dam == 5 )
               buf += "is a little stale.";
            else if( dam == 4 )
               buf += "is a bit stale.";
            else if( dam == 3 )
               buf += "smells slightly off.";
            else if( dam == 2 )
               buf += "smells quite rank.";
            else if( dam == 1 )
               buf += "smells revolting!";
            else if( dam <= 0 )
               buf += "is crawling with maggots!";
            buf += "\r\n";
            ch->print( buf );
            break;

         case ITEM_SWITCH:
         case ITEM_LEVER:
         case ITEM_PULLCHAIN:
            if( IS_SET( obj->value[0], TRIG_UP ) )
               ch->print( "You notice that it is in the up position.\r\n" );
            else
               ch->print( "You notice that it is in the down position.\r\n" );
            break;

         case ITEM_BUTTON:
            if( IS_SET( obj->value[0], TRIG_UP ) )
               ch->print( "You notice that it is depressed.\r\n" );
            else
               ch->print( "You notice that it is not depressed.\r\n" );
            break;

         case ITEM_CORPSE_PC:
         case ITEM_CORPSE_NPC:
         {
            short timerfrac = obj->timer;
            if( obj->item_type == ITEM_CORPSE_PC )
               timerfrac = ( int )obj->timer / 8 + 1;

            switch ( timerfrac )
            {
               default:
                  ch->print( "This corpse has recently been slain.\r\n" );
                  break;
               case 4:
                  ch->print( "This corpse was slain a little while ago.\r\n" );
                  break;
               case 3:
                  ch->print( "A foul smell rises from the corpse, and it is covered in flies.\r\n" );
                  break;
               case 2:
                  ch->print( "A writhing mass of maggots and decay, you can barely go near this corpse.\r\n" );
                  break;
               case 1:
               case 0:
                  ch->print( "Little more than bones, there isn't much left of this corpse.\r\n" );
                  break;
            }
         }

         case ITEM_CONTAINER:
            if( obj->extra_flags.test( ITEM_COVERING ) )
               break;

         case ITEM_DRINK_CON:
         case ITEM_QUIVER:
            ch->print( "When you look inside, you see:\r\n" );

         case ITEM_KEYRING:
            EXA_prog_trigger = false;
            cmdf( ch, "look in {}", argument );
            EXA_prog_trigger = true;
            break;

         case ITEM_JOURNAL:
         {
            short count = obj->extradesc.size();

            ch->print_fmt( "{} has {} {} written in out of a possible {}.\r\n",
                       obj->short_descr, count, count == 1 ? "page" : "pages", obj->value[0] );
            break;
         }
      }

      if( obj->extra_flags.test( ITEM_COVERING ) )
      {
         EXA_prog_trigger = false;
         cmdf( ch, "look under {}", argument );
         EXA_prog_trigger = true;
      }

      oprog_examine_trigger( ch, obj );
      if( ch->char_died(  ) || obj->extracted(  ) )
         return;
      check_for_trap( ch, obj, TRAP_EXAMINE );
   }
}

CMDF( do_compare )
{
   std::string arg1;
   obj_data *obj1, *obj2 = nullptr;
   int value1, value2;
   const char *msg;

   argument = one_argument( argument, arg1 );
   if( arg1.empty(  ) )
   {
      ch->print( "Compare what to what?\r\n" );
      return;
   }

   if( !( obj1 = ch->get_obj_carry( arg1 ) ) )
   {
      ch->print( "You do not have that item.\r\n" );
      return;
   }

   if( argument.empty(  ) )
   {
      std::list<obj_data *>::iterator iobj;
      for( iobj = ch->carrying.begin(  ); iobj != ch->carrying.end(  ); ++iobj )
      {
         obj2 = *iobj;
         if( obj2->wear_loc != WEAR_NONE && ch->can_see_obj( obj2, false ) && obj1->item_type == obj2->item_type && obj1->wear_flags == obj2->wear_flags )
            break;
      }

      if( !obj2 )
      {
         ch->print( "You aren't wearing anything comparable.\r\n" );
         return;
      }
   }
   else
   {
      if( !( obj2 = ch->get_obj_carry( argument ) ) )
      {
         ch->print( "You do not have that item.\r\n" );
         return;
      }
   }

   if( obj1->item_type != obj2->item_type )
   {
      ch->printf( "Why would you compare a %s to a %s?\r\n", o_types[obj1->item_type], o_types[obj2->item_type] );
      return;
   }
   msg = nullptr;
   value1 = 0;
   value2 = 0;

   if( obj1 == obj2 )
      msg = "You compare $p to itself. It looks about the same.";
   else if( obj1->item_type != obj2->item_type )
      msg = "You can't compare $p and $P.";
   else
   {
      switch ( obj1->item_type )
      {
         default:
            msg = "You can't compare $p and $P.";
            break;

         case ITEM_ARMOR:
            value1 = obj1->value[0];
            value2 = obj2->value[0];
            break;

         case ITEM_WEAPON:
            value1 = obj1->value[1] + obj1->value[2];
            value2 = obj2->value[1] + obj2->value[2];
            break;
      }
   }
   if( !msg )
   {
      if( value1 == value2 )
         msg = "$p and $P look about the same.";
      else if( value1 > value2 )
         msg = "$p looks better than $P.";
      else
         msg = "$p looks worse than $P.";
   }
   act( AT_PLAIN, msg, ch, obj1, obj2, TO_CHAR );
}

CMDF( do_oldwhere )
{
   char_data *victim;
   bool found = false;

   if( argument.empty(  ) )
   {
      ch->pager_fmt( "\r\nPlayers near you in {}:\r\n", ch->in_room->area->name );
      for( auto* d : dlist )
      {
         if( ( d->connected == CON_PLAYING || d->connected == CON_EDITING )
             && ( victim = d->character ) != nullptr && !victim->isnpc(  ) && victim->in_room
             && victim->in_room->area == ch->in_room->area && ch->can_see( victim, true ) && !is_ignoring( victim, ch )
             && !ch->in_room->flags.test( ROOM_NOWHERE ) && !victim->in_room->flags.test( ROOM_NOWHERE )
             && !ch->in_room->area->flags.test( AFLAG_NOWHERE ) && !victim->in_room->area->flags.test( AFLAG_NOWHERE ) )
         {
            found = true;

            ch->pager_fmt( "&[people]{:<13}  ", victim->name );
            if( victim->CAN_PKILL(  ) && victim->pcdata->clan && victim->pcdata->clan->clan_type != CLAN_GUILD )
               ch->pager_fmt( "{:<18}\t", victim->pcdata->clan->badge.c_str(  ) );
            else if( victim->CAN_PKILL(  ) )
               ch->pager( "(&wUnclanned&[people])\t" );
            else
               ch->pager( "\t\t\t" );
            ch->pager_fmt( "&[rmname]{}\r\n", victim->in_room->name );
         }
      }
      if( !found )
         ch->pager( "None\r\n" );
   }
   else
   {
      std::list<char_data *>::iterator ich;
      for( ich = charlist.begin(  ); ich != charlist.end(  ); ++ich )
      {
         victim = *ich;

         if( victim->in_room && victim->in_room->area == ch->in_room->area && !victim->has_aflag( AFF_HIDE )
             && !victim->has_aflag( AFF_SNEAK ) && ch->can_see( victim, true ) && hasname( victim->name, argument ) )
         {
            found = true;
            ch->pager_fmt( "&[people]{:<28} &[rmname]{}\r\n", PERS( victim, ch, true ), victim->in_room->name );
            break;
         }
      }
      if( !found )
         ch->print_fmt( "You didn't fine any {}.\r\n", argument );
   }
}

CMDF( do_consider )
{
   char_data *victim;
   const char *msg;
   int diff;

   if( argument.empty(  ) )
   {
      ch->print( "Consider killing whom?\r\n" );
      return;
   }

   if( !( victim = ch->get_char_room( argument ) ) )
   {
      ch->print( "They're not here.\r\n" );
      return;
   }

   if( victim == ch )
   {
      ch->print( "You decide you're pretty sure you could take yourself in a fight.\r\n" );
      return;
   }

   diff = victim->level - ch->level;

   if( diff <= -10 )
      msg = "You are far more experienced than $N.";
   else if( diff <= -5 )
      msg = "$N is not nearly as experienced as you.";
   else if( diff <= -2 )
      msg = "You are more experienced than $N.";
   else if( diff <= 1 )
      msg = "You are just about as experienced as $N.";
   else if( diff <= 4 )
      msg = "You are not nearly as experienced as $N.";
   else if( diff <= 9 )
      msg = "$N is far more experienced than you!";
   else
      msg = "$N would make a great teacher for you!";
   act( AT_CONSIDER, msg, ch, nullptr, victim, TO_CHAR );

   diff = ( victim->max_hit - ch->max_hit ) / 6;

   if( diff <= -200 )
      msg = "$N looks like a feather!";
   else if( diff <= -150 )
      msg = "You could kill $N with your hands tied!";
   else if( diff <= -100 )
      msg = "Hey! Where'd $N go?";
   else if( diff <= -50 )
      msg = "$N is a wimp.";
   else if( diff <= 0 )
      msg = "$N looks weaker than you.";
   else if( diff <= 50 )
      msg = "$N looks about as strong as you.";
   else if( diff <= 100 )
      msg = "It would take a bit of luck...";
   else if( diff <= 150 )
      msg = "It would take a lot of luck, and equipment!";
   else if( diff <= 200 )
      msg = "Why don't you dig a grave for yourself first?";
   else
      msg = "$N is built like a TANK!";
   act( AT_CONSIDER, msg, ch, nullptr, victim, TO_CHAR );
}

CMDF( do_wimpy )
{
   int wimpy;

   if( !str_cmp( argument, "max" ) )
   {
      if( ch->IS_PKILL(  ) )
         wimpy = ( int )( ch->max_hit / 2.25 );
      else
         wimpy = ( int )( ch->max_hit / 1.2 );
   }
   else if( argument.empty(  ) )
      wimpy = ( int )ch->max_hit / 5;
   else
      wimpy = atoi( argument.c_str(  ) );

   if( wimpy < 0 )
   {
      ch->print( "&YYour courage exceeds your wisdom.\r\n" );
      return;
   }
   if( ch->IS_PKILL(  ) && wimpy > ( int )ch->max_hit / 2.25 )
   {
      ch->print( "&YSuch cowardice ill becomes you.\r\n" );
      return;
   }
   else if( wimpy > ( int )ch->max_hit / 1.2 )
   {
      ch->print( "&YSuch cowardice ill becomes you.\r\n" );
      return;
   }
   ch->wimpy = wimpy;
   ch->printf( "&YWimpy set to %d hit points.\r\n", wimpy );
}

// Encryption upgraded to MD5 - Samson 7-10-00
// Upgraded again to use OS independent MD5 algorithm - Samson 6-26-04
// Upgraded yet again to OS independent SHA-256 encryption - Samson 1-7-06
// And one more time to OS independent SHA-512 encryption - Samson 6/14/2026
CMDF( do_password )
{
   std::string arg1;
   std::string pwdnew;

   if( ch->isnpc(  ) )
      return;

   argument = one_argument( argument, arg1 );

   if( arg1.empty(  ) || argument.empty(  ) )
   {
      ch->print( "Syntax: password <new> <again>.\r\n" );
      return;
   }

   /*
    * This should stop all the mistyped password problems --Shaddai 
    */
   if( str_cmp( arg1, argument ) )
   {
      ch->print( "Passwords don't match try again.\r\n" );
      return;
   }

   if( argument.length(  ) < 5 )
   {
      ch->print( "New password must be at least five characters long.\r\n" );
      return;
   }

   if( arg1[0] == '!' || argument[0] == '!' )
   {
      ch->print( "New password cannot begin with the '!' character." );
      return;
   }

   pwdnew = password_hash( argument ); // SHA-512 Encryption
   ch->pcdata->pwd = pwdnew;
   ch->save(  );
   ch->print( "Ok.\r\n" );
}

/*
 * display WIZLIST file - Thoric
 */
CMDF( do_wizlist )
{
   ch->set_pager_color( AT_IMMORT );
   show_file( ch, WIZLIST_FILE );
}

const char *PCFYN( char_data * ch, int flag )
{
   return( ch->has_pcflag( flag ) ? " &z(&GON&z)&D" : "&z(&ROFF&z)&D" );
}

/*
 * Contributed by Grodyn.
 * Display completely overhauled, 2/97 -- Blodkai
 */
/* Almost completely redone by Zarius - 1/30/2004 */
CMDF( do_config )
{
   if( ch->isnpc(  ) )
      return;

   if( argument.empty(  ) )
   {
      ch->print( "\r\n&gAFKMud Configurations " );
      ch->print( "&G(use 'config +/-<keyword>' to toggle, see 'help config')&D\r\n" );
      ch->print( "&G--------------------------------------------------------------------------------&D\r\n\r\n" );
      ch->print( "&g&uDisplay:&d   " );
      ch->printf( "&wPager        &z: %3s\t", PCFYN( ch, PCFLAG_PAGERON ) );
      ch->printf( "&wGag          &z: %3s\t", PCFYN( ch, PCFLAG_GAG ) );
      ch->printf( "&wBrief        &z: %3s\r\n", PCFYN( ch, PCFLAG_BRIEF ) );
      ch->printf( "           &wCompass      &z: %3s\t", PCFYN( ch, PCFLAG_COMPASS ) );
      ch->printf( "&wBlank        &z: %3s\t", PCFYN( ch, PCFLAG_BLANK ) );
      ch->printf( "&wAnsi         &z: %3s\r\n", PCFYN( ch, PCFLAG_ANSI ) );
      ch->printf( "           &wAutomap      &z: %3s\r\n", PCFYN( ch, PCFLAG_AUTOMAP ) );

      /*
       * Config option for Smartsac added 1-18-00 - Samson 
       */
      ch->print( "\r\n&g&uAuto:&d      " );
      ch->printf( "&wAutosac      &z: %3s\t", PCFYN( ch, PCFLAG_AUTOSAC ) );
      ch->printf( "&wAutogold     &z: %3s\t", PCFYN( ch, PCFLAG_AUTOGOLD ) );
      ch->printf( "&wAutoloot     &z: %3s\r\n", PCFYN( ch, PCFLAG_AUTOLOOT ) );
      ch->printf( "           &wAutoexit     &z: %3s\t", PCFYN( ch, PCFLAG_AUTOEXIT ) );
      ch->printf( "&wSmartsac     &z: %3s\t", PCFYN( ch, PCFLAG_SMARTSAC ) );
      ch->printf( "&wGuildsplit   &z: %3s\r\n", PCFYN( ch, PCFLAG_GUILDSPLIT ) );
      ch->printf( "           &wGroupsplit   &z: %3s\t", PCFYN( ch, PCFLAG_GROUPSPLIT ) );
      ch->printf( "&wAutoassist   &z: %3s\r\n", PCFYN( ch, PCFLAG_AUTOASSIST ) );

      /*
       * PCFLAG_NOBEEP config option added by Samson 2-15-98 
       */
      ch->print( "\r\n&g&uSafeties:&d  " );
      ch->printf( "&wNoRecall     &z: %3s\t", PCFYN( ch, PCFLAG_NORECALL ) );
      ch->printf( "&wNoSummon     &z: %3s\t", PCFYN( ch, PCFLAG_NOSUMMON ) );
      ch->printf( "&wNoBeep       &z: %3s\r\n", PCFYN( ch, PCFLAG_NOBEEP ) );

      if( !ch->has_pcflag( PCFLAG_DEADLY ) )
      {
         ch->printf( "           &wNoTell       &z: %3s\t", PCFYN( ch, PCFLAG_NOTELL ) );
         ch->printf( "&wDrag         &z: %3s\r\n", PCFYN( ch, PCFLAG_SHOVEDRAG ) );
      }
      else
         ch->printf( "           &wNoTell       &z: %3s\r\n", PCFYN( ch, PCFLAG_NOTELL ) );

      ch->print( "\r\n&g&uMisc:&d      " );
      ch->printf( "&wGroupwho     &z: %3s\t", PCFYN( ch, PCFLAG_GROUPWHO ) );
      ch->printf( "&wNoIntro      &z: %3s\r\n", PCFYN( ch, PCFLAG_NOINTRO ) );
      ch->printf( "           &wMSP          &z: %3s\t", PCFYN( ch, PCFLAG_MSP ) );
      ch->printf( "&wCheckboard   &z: %3s\t", PCFYN( ch, PCFLAG_CHECKBOARD ) );
      ch->printf( "&wNoQuote      &z: %3s\r\n", PCFYN( ch, PCFLAG_NOQUOTE ) );

      /*
       * Config option for Room Flag display added by Samson 12-10-97 
       * Config option for Sector Type display added by Samson 12-10-97 
       * Config option Area name and filename added by Samson 12-13-97 
       * Config option Passdoor added by Samson 3-21-98 
       */

      if( ch->is_immortal(  ) )
      {
         ch->print( "\r\n&g&uImmortal :&d " );
         ch->printf( "&wRoomvnum     &z: %3s\t", PCFYN( ch, PCFLAG_ROOMVNUM ) );
         ch->printf( "&wRoomflags    &z: %3s\t", PCFYN( ch, PCFLAG_AUTOFLAGS ) );
         ch->printf( "&wSectortypes  &z: %3s\r\n", PCFYN( ch, PCFLAG_SECTORD ) );
         ch->printf( "           &wFilename     &z: %3s\t", PCFYN( ch, PCFLAG_ANAME ) );
         ch->printf( "&wPassdoor     &z: %3s\r\n", PCFYN( ch, PCFLAG_PASSDOOR ) );
      }

      ch->print( "\r\n&g&uSettings:&d  " );
      ch->printf( "&wPager Length &z(&Y%d&z)    &wWimpy &z(&Y%d&z)&d\r\n", ch->pcdata->pagerlen, ch->wimpy );

      /*
       * Now only shows sentences section if you have any - Zarius 
       */
      if( ch->has_pcflag( PCFLAG_SILENCE ) || ch->has_pcflag( PCFLAG_NO_EMOTE )
          || ch->has_pcflag( PCFLAG_NO_TELL ) || ch->has_pcflag( PCFLAG_NOTITLE ) || ch->has_pcflag( PCFLAG_LITTERBUG ) )
      {  //added NOTITLE - Zarius
         ch->print( "\r\n\r\n&g&uSentences imposed on you:&d\r\n" );
         ch->printf( "&R%s%s%s%s%s&D",
                     ch->has_pcflag( PCFLAG_SILENCE ) ?
                     "For your abuse of channels, you are currently silenced.\r\n" : "",
                     ch->has_pcflag( PCFLAG_NO_EMOTE ) ?
                     "The gods have removed your emotes.\r\n" : "",
                     ch->has_pcflag( PCFLAG_NO_TELL ) ?
                     "You are not permitted to send 'tells' to others.\r\n" : "",
                     ch->has_pcflag( PCFLAG_NOTITLE ) ?
                     "You are not permitted to change your title.\r\n" : "",
                     ch->has_pcflag( PCFLAG_LITTERBUG ) ? "A convicted litterbug. You cannot drop anything.\r\n" : "\r\n" );
      }
   }
   else
   {
      bool fSet;
      int bit = 0;

      if( argument[0] == '+' )
         fSet = true;
      else if( argument[0] == '-' )
         fSet = false;
      else
      {
         ch->print( "Config -option or +option?\r\n" );
         return;
      }

      std::string arg = argument.substr( 1, argument.length(  ) );

      if( !str_prefix( arg, "autoexit" ) )
         bit = PCFLAG_AUTOEXIT;
      else if( !str_prefix( arg, "automap" ) )
         bit = PCFLAG_AUTOMAP;
      else if( !str_prefix( arg, "autoloot" ) )
         bit = PCFLAG_AUTOLOOT;
      else if( !str_prefix( arg, "autosac" ) )
         bit = PCFLAG_AUTOSAC;
      else if( !str_prefix( arg, "smartsac" ) )
         bit = PCFLAG_SMARTSAC;
      else if( !str_prefix( arg, "autogold" ) )
         bit = PCFLAG_AUTOGOLD;
      else if( !str_prefix( arg, "guildsplit" ) )
         bit = PCFLAG_GUILDSPLIT;
      else if( !str_prefix( arg, "groupsplit" ) )
         bit = PCFLAG_GROUPSPLIT;
      else if( !str_prefix( arg, "autoassist" ) )
         bit = PCFLAG_AUTOASSIST;
      else if( !str_prefix( arg, "blank" ) )
         bit = PCFLAG_BLANK;
      else if( !str_prefix( arg, "brief" ) )
         bit = PCFLAG_BRIEF;
      else if( !str_prefix( arg, "msp" ) )
         bit = PCFLAG_MSP;
      else if( !str_prefix( arg, "ansi" ) )
         bit = PCFLAG_ANSI;
      else if( !str_prefix( arg, "compass" ) )
         bit = PCFLAG_COMPASS;
      else if( !str_prefix( arg, "drag" ) )
         bit = PCFLAG_SHOVEDRAG;
      else if( ch->is_immortal(  ) && !str_prefix( arg, "roomvnum" ) )
         bit = PCFLAG_ROOMVNUM;

      if( bit )
      {
         if( ( bit == PCFLAG_SHOVEDRAG ) && ch->has_pcflag( PCFLAG_DEADLY ) )
         {
            ch->print( "Pkill characters can not config that option.\r\n" );
            return;
         }

         if( fSet )
         {
            ch->set_pcflag( bit );
            ch->printf( "&Y%s &wis now &GON\r\n", capitalize( arg ).c_str(  ) );
         }
         else
         {
            ch->unset_pcflag( bit );
            ch->printf( "&Y%s &wis now &ROFF\r\n", capitalize( arg ).c_str(  ) );
         }
         return;
      }
      else
      {
         if( !str_prefix( arg, "norecall" ) )
            bit = PCFLAG_NORECALL;
         else if( !str_prefix( arg, "nointro" ) )
            bit = PCFLAG_NOINTRO;
         else if( !str_prefix( arg, "nosummon" ) )
            bit = PCFLAG_NOSUMMON;
         else if( !str_prefix( arg, "gag" ) )
            bit = PCFLAG_GAG;
         else if( !str_prefix( arg, "pager" ) )
            bit = PCFLAG_PAGERON;
         else if( !str_prefix( arg, "nobeep" ) )
            bit = PCFLAG_NOBEEP;
         else if( !str_prefix( arg, "passdoor" ) )
            bit = PCFLAG_PASSDOOR;
         else if( !str_prefix( arg, "groupwho" ) )
            bit = PCFLAG_GROUPWHO;
         else if( !str_prefix( arg, "notell" ) )
            bit = PCFLAG_NOTELL;
         else if( !str_prefix( arg, "checkboard" ) )
            bit = PCFLAG_CHECKBOARD;
         else if( !str_prefix( arg, "noquote" ) )
            bit = PCFLAG_NOQUOTE;
         else if( ch->is_immortal(  ) )
         {
            if( !str_prefix( arg, "roomflags" ) )
               bit = PCFLAG_AUTOFLAGS;
            else if( !str_prefix( arg, "sectortypes" ) )
               bit = PCFLAG_SECTORD;
            else if( !str_prefix( arg, "filename" ) )
               bit = PCFLAG_ANAME;
            else
            {
               ch->print( "Config -option or +option?\r\n" );
               return;
            }
         }
         else
         {
            ch->print( "Config -option or +option?\r\n" );
            return;
         }

         if( fSet )
         {
            ch->set_pcflag( bit );
            ch->printf( "&Y%s &wis now &GON\r\n", capitalize( arg ).c_str(  ) );
         }
         else
         {
            ch->unset_pcflag( bit );
            ch->printf( "&Y%s &wis now &ROFF\r\n", capitalize( arg ).c_str(  ) );
         }
         return;
      }
   }
}

/* Maintained strictly for license compliance. Do not remove. */
CMDF( do_credits )
{
   interpret( ch, "help credits" );
}

/* History File viewer added by Samson 2-12-98 - At Dwip's request */
CMDF( do_history )
{
   ch->set_pager_color( AT_SOCIAL );
   show_file( ch, HISTORY_FILE );
}

/* Statistical display for the mud, added by Samson 1-31-98 */
CMDF( do_world )
{
   ch->print( "&cBase source code: Smaug 1.8b\r\n" );
   ch->print_fmt( "Current source revision: {} {}\r\n", CODENAME, CODEVERSION );
   ch->print( "The MUD first came online on: Thu Sep 4 1997\r\n" );
   ch->print_fmt( "The MUD last rebooted on: {}\r\n", str_boot_time );
   ch->print_fmt( "The system time is      : {}\r\n", c_time( current_time, -1 ) );
   ch->print_fmt( "Your local time is      : {}\r\n", c_time( current_time, ch->pcdata->timezone ) );
   ch->print_fmt( "\r\nTotal number of zones in the world: {}\r\n", top_area );
   ch->print_fmt( "Total number of rooms in the world: {}\r\n", top_room );
   ch->print_fmt( "\r\nNumber of distinct mobs in the world: {}\r\n", top_mob_index );
   ch->print_fmt( "Number of mobs loaded into the world: {}\r\n", nummobsloaded );
   ch->print_fmt( "\r\nNumber of distinct objects in the world: {}\r\n", top_obj_index );
   ch->print_fmt( "Number of objects loaded into the world: {}\r\n", numobjsloaded );
   ch->print_fmt( "\r\nCurrent number of registered players: {}\r\n", num_pfiles );
   ch->print_fmt( "All time high number of players on at one time: {}\r\n", sysdata->alltimemax );
}

/* Reason buffer added to AFK. See also show_char_to_char_0. -Whir - 8/31/98 */
CMDF( do_afk )
{
   if( ch->isnpc(  ) )
      return;

   if( ch->has_pcflag( PCFLAG_AFK ) )
   {
      ch->unset_pcflag( PCFLAG_AFK );
      ch->print( "You are no longer afk.\r\n" );
      ch->pcdata->afkbuf.clear();
      act( AT_GREY, "$n is no longer afk.", ch, nullptr, nullptr, TO_ROOM );
   }
   else
   {
      ch->set_pcflag( PCFLAG_AFK );
      ch->print( "You are now afk.\r\n" );
      ch->pcdata->afkbuf.clear();
      if( argument.empty(  ) )
         ch->pcdata->afkbuf = argument;
      act( AT_GREY, "$n is now afk.", ch, nullptr, nullptr, TO_ROOM );
   }
}

CMDF( do_pager )
{
   if( ch->isnpc(  ) )
      return;

   if( argument.empty(  ) )
   {
      if( ch->has_pcflag( PCFLAG_PAGERON ) )
      {
         ch->print( "Pager disabled.\r\n" );
         do_config( ch, "-pager" );
      }
      else
      {
         ch->printf( "Pager is now enabled at %d lines.\r\n", ch->pcdata->pagerlen );
         do_config( ch, "+pager" );
      }
      return;
   }
   if( !is_number( argument ) )
   {
      ch->print( "Set page pausing to how many lines?\r\n" );
      return;
   }
   ch->pcdata->pagerlen = atoi( argument.c_str(  ) );
   if( ch->pcdata->pagerlen < 5 )
      ch->pcdata->pagerlen = 5;
   ch->printf( "Page pausing set to %d lines.\r\n", ch->pcdata->pagerlen );
}

void pc_data::save_ignores( FILE * fp )
{
   for( const auto& name : ignore )
      fprintf( fp, "Ignored      %s~\n", name.c_str(  ) );
}

void pc_data::load_ignores( FILE * fp )
{
   /*
    * Get the name 
    */
   const char* temp = fread_flagstring( fp );

   std::filesystem::path fname = std::format( "{}{}/{}", PLAYER_DIR, static_cast<char>( std::tolower( temp[0] ) ), capitalize( temp ) );

   /*
    * If there isn't a pfile for the name then don't add it to the list
    */
   if( std::filesystem::exists( fname ) )
      return;

   std::string ig = temp;
   /*
    * Add the name unless the limit has been reached 
    */
   if( ignore.size(  ) >= sysdata->maxign )
      bug( "{}: too many ignored names", __func__ );
   else
      ignore.push_back( ig );
}

/*
 * The ignore command allows players to ignore up to MAX_IGN
 * other players. Players may ignore other characters whether
 * they are online or not. This is to prevent people from
 * spamming someone and then logging off quickly to evade
 * being ignored.
 * Syntax:
 *	ignore		-	lists players currently ignored
 *	ignore none	-	sets it so no players are ignored
 *	ignore <player>	-	start ignoring player if not already
 *				ignored otherwise stop ignoring player
 *	ignore reply	-	start ignoring last player to send a
 *				tell to ch, to deal with invis spammers
 * Last Modified: June 26, 1997
 * - Fireblade
 */
CMDF( do_ignore )
{
   char_data *victim;

   if( ch->isnpc(  ) )
      return;

   /*
    * If no arguments, then list players currently ignored
    */
   if( argument.empty(  ) )
   {
      ch->print( "\r\n&[divider]----------------------------------------\r\n" );
      ch->print( "&GYou are currently ignoring:\r\n" );
      ch->print( "&[divider]----------------------------------------\r\n" );

      if( ch->pcdata->ignore.empty(  ) )
      {
         ch->print( "&[ignore]\t    no one\r\n" );
         return;
      }

      ch->print( "&[ignore]" );
      for( auto& temp : ch->pcdata->ignore )
         ch->printf( "\t  - %s\r\n", temp.c_str(  ) );
      return;
   }

   /*
    * Clear players ignored if given arg "none" 
    */
   else if( !str_cmp( argument, "none" ) )
   {
      for( auto it = ch->pcdata->ignore.begin(  ); it != ch->pcdata->ignore.end(  ); )
      {
         std::string ig = *it;
         ++it;

         ch->pcdata->ignore.remove( ig );
      }
      ch->print( "&[ignore]You now ignore no one.\r\n" );
      return;
   }

   /*
    * Prevent someone from ignoring themself... 
    */
   else if( !str_cmp( argument, "self" ) || !str_cmp( ch->name, argument ) )
   {
      ch->print( "&[ignore]Cannot ignore yourself.\r\n" );
      return;
   }

   else
   {
      std::filesystem::path fname = std::format( "{}{}/{}", PLAYER_DIR, static_cast<char>( std::tolower( argument.front() ) ), capitalize( argument ) );
      std::filesystem::path fname2 = std::format( "{}/{}", GOD_DIR, capitalize( argument ) );

      victim = nullptr;

      /*
       * get the name of the char who last sent tell to ch 
       */
      if( !str_cmp( argument, "reply" ) )
      {
         if( !ch->reply )
         {
            ch->printf( "&[ignore]%s is not here.\r\n", argument.c_str(  ) );
            return;
         }
         else
            argument = ch->reply->name;
      }

      /*
       * Loop through the linked list of ignored players, keep track of how many are being ignored 
       */
      size_t i = 0;
      for( auto& temp : ch->pcdata->ignore )
      {
         ++i;

         /*
          * If the argument matches a name in list remove it.
          */
         if( !str_cmp( temp, capitalize( argument ) ) )
         {
            ch->printf( "&[ignore]You no longer ignore %s.\r\n", temp.c_str(  ) );
            ch->pcdata->ignore.remove( temp );
            return;
         }
      }

      /*
       * if there wasn't a match check to see if the name is valid.
       * * This if-statement may seem like overkill but it is intended to prevent people from doing the
       * * spam and log thing while still allowing ya to ignore new chars without pfiles yet... 
       */
      if( !std::filesystem::exists( fname ) && ( !( victim = ch->get_char_world( argument ) ) || victim->isnpc(  ) || str_cmp( capitalize( argument ), victim->name ) != 0 ) )
      {
         ch->printf( "&[ignore]No player exists by the name %s.\r\n", argument.c_str(  ) );
         return;
      }

      if( !check_parse_name( argument, false ) )
      {
         ch->print( "That's not a valid name to ignore!\r\n" );
         return;
      }

      if( std::filesystem::exists( fname2 ) )
      {
         ch->print( "&[ignore]You cannot ignore an immortal.\r\n" );
         return;
      }

      if( victim )
         argument = victim->name;

      /*
       * If its valid and the list size limit has not been reached create a node and at it to the list 
       */
      if( i < sysdata->maxign )
      {
         std::string inew = capitalize( argument );
         ch->pcdata->ignore.push_back( inew );
         ch->printf( "&[ignore]You now ignore %s.\r\n", inew.c_str(  ) );
         return;
      }
      else
      {
         ch->printf( "&[ignore]You may only ignore %zu players.\r\n", sysdata->maxign );
         return;
      }
   }
}

/*
 * This function simply checks to see if ch is ignoring ign_ch.
 * Last Modified: October 10, 1997
 * - Fireblade
 */
bool is_ignoring( char_data * ch, char_data * ign_ch )
{
   if( !ch )   /* Paranoid bug check, you never know. */
   {
      bug( "{}: nullptr CH!", __func__ );
      return false;
   }

   if( !ign_ch )  /* Bail out, webwho can access this and ign_ch will be nullptr */
      return false;

   if( ch->isnpc(  ) || ign_ch->isnpc(  ) )
      return false;

   for( auto& ign : ch->pcdata->ignore )
   {
      if( !str_cmp( ign, ign_ch->name ) )
         return true;
   }
   return false;
}
