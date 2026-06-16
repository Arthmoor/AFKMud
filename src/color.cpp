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
 *               Color Module -- Allow user customizable Colors.            *
 *                                   --Matthew                              *
 *                      Enhanced ANSI parser by Samson                      *
 ****************************************************************************/

/*
* The following instructions assume you know at least a little bit about
* coding.  I firmly believe that if you can't code (at least a little bit),
* you don't belong running a mud.  So, with that in mind, I don't hold your
* hand through these instructions.
*
* You may use this code provided that:
*
*     1)  You understand that the authors _DO NOT_ support this code
*         Any help you need must be obtained from other sources.  The
*         authors will ignore any and all requests for help.
*     2)  You will mention the authors if someone asks about the code.
*         You will not take credit for the code, but you can take credit
*         for any enhancements you make.
*     3)  This message remains intact.
*
* If you would like to find out how to send the authors large sums of money,
* you may e-mail the following address:
*
* Matthew Bafford & Christopher Wigginton
* wiggy@mudservices.com
*/

/*
 * To add new color types:
 *
 * 1.  Edit color.h, and:
 *     1.  Add a new AT_ define.
 *     2.  Increment MAX_COLORS by however many AT_'s you added.
 * 2.  Edit color.c and:
 *     1.  Add the name(s) for your new color(s) to the end of the pc_displays array.
 *     2.  Add the default color(s) to the end of the default_set array.
 */

#include <filesystem>
#include "mud.h"
#include "descriptor.h"

const char *pc_displays[MAX_COLORS] = {
   "black", "dred", "dgreen", "orange",        // 3
   "dblue", "purple", "cyan", "grey",          // 7
   "dgrey", "red", "green", "yellow",          // 11
   "blue", "pink", "lblue", "white", "blink",  // 16
   "bblack", "bdred", "bdgreen", "bdorange",   // 20
   "bdblue", "bpurple", "bcyan", "bgrey",      // 24
   "bdgrey", "bred", "bgreen", "byellow",      // 28
   "bblue", "bpink", "blblue", "bwhite",       // 32
   "plain", "action", "say", "chat",           // 36
   "yells", "tell", "hit", "hitme",            // 40
   "immortal", "hurt", "falling", "danger",    // 44
   "magic", "consider", "report", "poison",    // 48
   "social", "dying", "dead", "skill",         // 52
   "carnage", "damage", "fleeing", "rmname",   // 56
   "rmdesc", "objects", "people", "list",      // 60
   "bye", "gold", "gtells", "note",            // 64
   "hungry", "thirsty", "fire", "sober",       // 68
   "wearoff", "exits", "score", "reset",       // 72
   "log", "die_msg", "wartalk", "arena",       // 76
   "muse", "think", "aflags", "who",           // 80
   "racetalk", "ignore", "whisper", "divider", // 84
   "morph", "shout", "rflags", "stype",        // 88
   "aname", "auction", "score2", "score3",     // 92
   "score4", "who2", "who3", "who4",           // 96
   "UNUSED", "helpfiles", "who5", "score5",    // 100
   "who6", "who7", "prac", "prac2",            // 104
   "prac3", "prac4", "UNUSED", "guildtalk",    // 108
   "board", "board2", "board3"                 // 111
};

/* All defaults are set to Alsherok default scheme, if you don't 
like it, change it around to suite your own needs - Samson */
const short default_set[MAX_COLORS] = {
   AT_BLACK, AT_BLOOD, AT_DGREEN, AT_ORANGE, /*  3 */
   AT_DBLUE, AT_PURPLE, AT_CYAN, AT_GREY,    /*  7 */
   AT_DGREY, AT_RED, AT_GREEN, AT_YELLOW,    /* 11 */
   AT_BLUE, AT_PINK, AT_LBLUE, AT_WHITE, AT_BLINK, /* 16 */
   AT_BLACK_BLINK, AT_BLOOD_BLINK, AT_DGREEN_BLINK, AT_ORANGE_BLINK, /* 20 */
   AT_DBLUE_BLINK, AT_PURPLE_BLINK, AT_CYAN_BLINK, AT_GREY_BLINK,    /* 24 */
   AT_DGREY_BLINK, AT_RED_BLINK, AT_GREEN_BLINK, AT_YELLOW_BLINK,    /* 28 */
   AT_BLUE_BLINK, AT_PINK_BLINK, AT_LBLUE_BLINK, AT_WHITE_BLINK,     /* 32 */
   AT_GREY, AT_GREY, AT_BLUE,             /* 35 */
   AT_GREEN, AT_LBLUE, AT_WHITE, AT_GREY, /* 36 */
   AT_GREY, AT_YELLOW, AT_GREY, AT_GREY,  /* 43 */
   AT_RED_BLINK, AT_BLUE, AT_GREY, AT_GREY,    /* 47 */
   AT_DGREEN, AT_CYAN, AT_GREY, AT_GREY,  /* 51 */
   AT_BLUE, AT_GREY, AT_GREY, AT_GREY,    /* 55 */
   AT_RED, AT_GREY, AT_BLUE, AT_PINK,     /* 59 */
   AT_GREY, AT_GREY, AT_YELLOW, AT_GREY,  /* 63 */
   AT_GREY, AT_ORANGE, AT_BLUE, AT_RED,   /* 67 */
   AT_GREY, AT_GREY, AT_GREEN, AT_DGREEN, /* 71 */
   AT_DGREEN, AT_ORANGE, AT_GREY, AT_RED, /* 75 */
   AT_GREY, AT_DGREEN, AT_RED, AT_BLUE,   /* 79 */
   AT_RED, AT_CYAN, AT_YELLOW, AT_PINK,   /* 83 */
   AT_DGREEN, AT_PINK, AT_WHITE, AT_BLUE, /* 87 */
   AT_BLUE, AT_BLUE, AT_GREEN, AT_GREY,   /* 91 */
   AT_GREEN, AT_GREEN, AT_YELLOW, AT_DGREY,  /* 95 */
   AT_GREEN, AT_PINK, AT_DGREEN, AT_CYAN, /* 99 */
   AT_RED, AT_WHITE, AT_BLUE, AT_DGREEN,  /* 103 */
   AT_CYAN, AT_BLOOD, AT_RED, AT_DGREEN,  /* 107 */
   AT_GREEN, AT_GREY, AT_GREEN, AT_WHITE  /* 111 */
};

const char *valid_color[] = {
   "black", "dred", "dgreen", "orange", "dblue", "purple", "cyan", "grey",
   "dgrey", "red", "green", "yellow", "blue", "pink", "lblue", "white", "\0"
};

void show_colorthemes( char_data * ch )
{
   int count = 0, col = 0;

   ch->pager( "&YThe following themes are available:\r\n" );

   for( const auto& entry : std::filesystem::directory_iterator( COLOR_DIR ) )
   {
      const auto& path = entry.path();
      const std::string filename = path.filename().string();

      if( filename.empty() || filename[0] == '.' )
         continue;

      ++count;
      ch->pagerf( "%s%-15.15s", ch->color_str( AT_PLAIN ), filename.c_str() );
      if( ++col % 6 == 0 )
         ch->pager( "\r\n" );
   }

   if( count == 0 )
      ch->pager( "No themes defined yet.\r\n" );

   if( col % 6 != 0 )
      ch->pager( "\r\n" );
}

void show_colors( char_data * ch )
{
   short count;

   ch->pager( "&BSyntax: color [color type] [color] | default\r\n" );
   ch->pager( "&BSyntax: color _reset_ (Resets all colors to default set)\r\n" );
   ch->pager( "&BSyntax: color _all_ [color] (Sets all color types to [color])\r\n" );
   ch->pager( "&BSyntax: color theme [name] (Sets all color types to a defined theme)\r\n\r\n" );

   ch->pager( "&W********************************[ COLORS ]*********************************\r\n" );

   for( count = 0; count < 16; ++count )
   {
      if( ( count % 8 ) == 0 && count != 0 )
         ch->pager( "\r\n" );
      ch->pagerf( "%s%-10s", ch->color_str( count ), pc_displays[count] );
   }

   ch->pager( "\r\n\r\n&W******************************[ COLOR TYPES ]******************************\r\n" );

   for( count = 33; count < MAX_COLORS; ++count )
   {
      if( ( count % 8 ) == 0 && count != 33 )
         ch->pager( "\r\n" );
      ch->pagerf( "%s%-10s%s", ch->color_str( count ), pc_displays[count], ANSI_RESET );
   }
   ch->pager( "\r\n\r\n" );
   ch->pager( "&YAvailable colors are:\r\n" );

   for( count = 0; valid_color[count][0] != '\0'; ++count )
   {
      if( ( count % 8 ) == 0 && count != 0 )
         ch->pager( "\r\n" );

      ch->pagerf( "%s%-10s", ch->color_str( AT_PLAIN ), valid_color[count] );
   }
   ch->pager( "\r\n\r\n" );
   show_colorthemes( ch );
}

void reset_colors( char_data * ch )
{
   if( !ch->isnpc(  ) )
   {
      std::filesystem::path filename = std::format( "{}{}", COLOR_DIR, "default" );

      if( std::filesystem::exists( filename ) )
      {
         FILE *fp;
         int max_colors = 0;

         if( !( fp = fopen( filename.c_str(), "r" ) ) )
         {
            memcpy( &ch->pcdata->colors, &default_set, sizeof( default_set ) );
            return;
         }

         while( !feof( fp ) )
         {
            char *word = fread_word( fp );

            if( !str_cmp( word, "MaxColors" ) )
            {
               max_colors = fread_number( fp );
               continue;
            }

            if( !str_cmp( word, "Colors" ) )
            {
               for( int x = 0; x < max_colors; ++x )
                  ch->pcdata->colors[x] = fread_number( fp );
               continue;
            }

            if( !str_cmp( word, "End" ) )
            {
               FCLOSE( fp );
               return;
            }
         }
         FCLOSE( fp );
         return;
      }
      else
         memcpy( &ch->pcdata->colors, &default_set, sizeof( default_set ) );
   }
   else
      log_printf( "%s: Attempting to reset NPC colors: %s", __func__, ch->short_descr );
}

CMDF( do_color )
{
   bool dMatch, cMatch;
   short count = 0, y = 0;
   std::string arg, arg2;

   dMatch = false;
   cMatch = false;

   if( ch->isnpc(  ) )
   {
      ch->print( "Only PC's can change colors.\r\n" );
      return;
   }

   if( argument.empty(  ) )
   {
      show_colors( ch );
      return;
   }

   argument = one_argument( argument, arg );

   if( !str_cmp( arg, "savetheme" ) && ch->is_imp(  ) )
   {
      FILE *fp;
      std::filesystem::path filename;

      if( argument.empty(  ) )
      {
         ch->print( "You must specify a name for this theme to save it.\n\r" );
         return;
      }

      if( strstr( argument.c_str(  ), ".." ) || strstr( argument.c_str(  ), "/" ) || strstr( argument.c_str(  ), "\\" ) )
      {
         ch->print( "Invalid theme name.\r\n" );
         return;
      }

      filename = std::format( "{}{}", COLOR_DIR, argument );
      if( !( fp = fopen( filename.c_str(), "w" ) ) )
      {
         ch->printf( "Unable to write to color file %s\n\r", filename.c_str() );
         return;
      }
      fprintf( fp, "%s", "#COLORTHEME\n" );
      fprintf( fp, "Name         %s~\n", argument.c_str(  ) );
      fprintf( fp, "MaxColors    %d\n", MAX_COLORS );
      fprintf( fp, "%s", "Colors      " );
      for( int x = 0; x < MAX_COLORS; ++x )
         fprintf( fp, " %d", ch->pcdata->colors[x] );
      fprintf( fp, "%s", "\nEnd\n" );
      FCLOSE( fp );
      ch->printf( "Color theme %s saved.\r\n", argument.c_str(  ) );
      return;
   }

   if( !str_cmp( arg, "theme" ) )
   {
      FILE *fp;
      std::filesystem::path filename;
      int max_colors = 0;

      if( argument.empty(  ) )
      {
         show_colorthemes( ch );
         return;
      }

      if( strstr( argument.c_str(  ), ".." ) || strstr( argument.c_str(  ), "/" ) || strstr( argument.c_str(  ), "\\" ) )
      {
         ch->print( "Invalid theme.\r\n" );
         return;
      }

      filename = std::format( "{}{}", COLOR_DIR, argument );
      if( !( fp = fopen( filename.c_str(), "r" ) ) )
      {
         ch->printf( "There is no theme called %s.\r\n", filename.c_str(  ) );
         return;
      }

      while( !feof( fp ) )
      {
         char *word = fread_word( fp );
         if( !str_cmp( word, "MaxColors" ) )
         {
            max_colors = fread_number( fp );
            continue;
         }

         if( !str_cmp( word, "Colors" ) )
         {
            for( int x = 0; x < max_colors; ++x )
               ch->pcdata->colors[x] = fread_number( fp );
            continue;
         }

         if( !str_cmp( word, "End" ) )
         {
            FCLOSE( fp );
            ch->printf( "Color theme has been changed to %s.\r\n", argument.c_str(  ) );
            ch->save(  );
            return;
         }
      }
      FCLOSE( fp );
      ch->printf( "An error occured while trying to set color theme %s.\r\n", argument.c_str(  ) );
      return;
   }

   if( !str_cmp( arg, "ansitest" ) )
   {
      ch->desc->buffer_printf( "{}Black\r\n", ANSI_BLACK );
      ch->desc->buffer_printf( "{}Dark Red\r\n", ANSI_DRED );
      ch->desc->buffer_printf( "{}Dark Green\r\n", ANSI_DGREEN );
      ch->desc->buffer_printf( "{}Orange/Brown\r\n", ANSI_ORANGE );
      ch->desc->buffer_printf( "{}Dark Blue\r\n", ANSI_DBLUE );
      ch->desc->buffer_printf( "{}Purple\r\n", ANSI_PURPLE );
      ch->desc->buffer_printf( "{}Cyan\r\n", ANSI_CYAN );
      ch->desc->buffer_printf( "{}Grey\r\n", ANSI_GREY );
      ch->desc->buffer_printf( "{}Dark Grey\r\n", ANSI_DGREY );
      ch->desc->buffer_printf( "{}Red\r\n", ANSI_RED );
      ch->desc->buffer_printf( "{}Green\r\n", ANSI_GREEN );
      ch->desc->buffer_printf( "{}Yellow\r\n", ANSI_YELLOW );
      ch->desc->buffer_printf( "{}Blue\r\n", ANSI_BLUE );
      ch->desc->buffer_printf( "{}Pink\r\n", ANSI_PINK );
      ch->desc->buffer_printf( "{}Light Blue\r\n", ANSI_LBLUE );
      ch->desc->buffer_printf( "{}White\r\n", ANSI_WHITE );
      ch->desc->buffer_printf( "{}Blinking Black\r\n", BLINK_BLACK );
      ch->desc->buffer_printf( "{}Blinking Dark Red\r\n", BLINK_DRED );
      ch->desc->buffer_printf( "{}Blinking Dark Green\r\n", BLINK_DGREEN );
      ch->desc->buffer_printf( "{}Blinking Orange/Brown\r\n", BLINK_ORANGE );
      ch->desc->buffer_printf( "{}Blinking Dark Blue\r\n", BLINK_DBLUE );
      ch->desc->buffer_printf( "{}Blinking Purple\r\n", BLINK_PURPLE );
      ch->desc->buffer_printf( "{}Blinking Cyan\r\n", BLINK_CYAN );
      ch->desc->buffer_printf( "{}Blinking Grey\r\n", BLINK_GREY );
      ch->desc->buffer_printf( "{}Blinking Dark Grey\r\n", BLINK_DGREY );
      ch->desc->buffer_printf( "{}Blinking Red\r\n", BLINK_RED );
      ch->desc->buffer_printf( "{}Blinking Green\r\n", BLINK_GREEN );
      ch->desc->buffer_printf( "{}Blinking Yellow\r\n", BLINK_YELLOW );
      ch->desc->buffer_printf( "{}Blinking Blue\r\n", BLINK_BLUE );
      ch->desc->buffer_printf( "{}Blinking Pink\r\n", BLINK_PINK );
      ch->desc->buffer_printf( "{}Blinking Light Blue\r\n", BLINK_LBLUE );
      ch->desc->buffer_printf( "{}Blinking White\r\n", BLINK_WHITE );
      ch->desc->write_to_buffer( ANSI_RESET );

      ch->desc->buffer_printf( "{}{}Black Background\r\n", ANSI_WHITE, BACK_BLACK );
      ch->desc->buffer_printf( "{}{}Dark Red Background\r\n", ANSI_BLACK, BACK_DRED );
      ch->desc->buffer_printf( "{}{}Dark Green Background\r\n", ANSI_BLACK, BACK_DGREEN );
      ch->desc->buffer_printf( "{}{}Orange/Brown Background\r\n", ANSI_BLACK, BACK_ORANGE );
      ch->desc->buffer_printf( "{}{}Dark Blue Background\r\n", ANSI_BLACK, BACK_DBLUE );
      ch->desc->buffer_printf( "{}{}Purple Background\r\n", ANSI_BLACK, BACK_PURPLE );
      ch->desc->buffer_printf( "{}{}Cyan Background\r\n", ANSI_BLACK, BACK_CYAN );
      ch->desc->buffer_printf( "{}{}Grey Background\r\n", ANSI_BLACK, BACK_GREY );
      ch->desc->buffer_printf( "{}{}Italics{}\r\n", ANSI_GREY, ANSI_ITALIC, ANSI_RESET );
      ch->desc->buffer_printf( "{}Strikeout{}\r\n", ANSI_STRIKEOUT, ANSI_RESET );
      ch->desc->buffer_printf( "{}Underline\r\n", ANSI_UNDERLINE );
      ch->desc->write_to_buffer( ANSI_RESET );
      return;
   }

   if( !str_prefix( arg, "_reset_" ) )
   {
      reset_colors( ch );
      ch->print( "All color types reset to default colors.\r\n" );
      return;
   }

   argument = one_argument( argument, arg2 );

   if( arg.empty(  ) )
   {
      ch->print( "Change which color type?\r\n" );
      return;
   }

   if( !str_prefix( arg, "_all_" ) )
   {
      dMatch = true;
      count = -1;

      /*
       * search for a valid color setting 
       */
      for( y = 0; y < 16; ++y )
      {
         if( !str_cmp( arg2, valid_color[y] ) )
         {
            cMatch = true;
            break;
         }
      }
   }
   else if( arg.empty(  ) || arg2.empty(  ) )
      cMatch = false;
   else
   {
      /*
       * search for the display type and str_cmp
       */
      for( count = 0; count < MAX_COLORS; ++count )
      {
         if( !str_prefix( arg, pc_displays[count] ) )
         {
            dMatch = true;
            break;
         }
      }

      if( !dMatch )
      {
         ch->printf( "%s is an invalid color type.\r\n", arg.c_str(  ) );
         ch->print( "Type color with no arguments to see available options.\r\n" );
         return;
      }

      if( !str_cmp( arg2, "default" ) )
      {
         ch->pcdata->colors[count] = default_set[count];
         ch->printf( "Display %s set back to default.\r\n", pc_displays[count] );
         return;
      }

      /*
       * search for a valid color setting
       */
      for( y = 0; y < 16; ++y )
      {
         if( !str_cmp( arg2, valid_color[y] ) )
         {
            cMatch = true;
            break;
         }
      }
   }

   if( !cMatch )
   {
      if( !arg.empty(  ) )
         ch->pagerf( "Invalid color for type %s.\r\n", arg.c_str(  ) );
      else
         ch->pager( "Invalid color.\r\n" );

      ch->pager( "Choices are:\r\n" );

      for( count = 0; count < 16; ++count )
      {
         if( count % 5 == 0 && count != 0 )
            ch->pager( "\r\n" );

         ch->pagerf( "%-10s", valid_color[count] );
      }
      ch->pagerf( "%-10s\r\n", "default" );
      return;
   }
   else
      ch->pagerf( "Color type %s set to color %s.\r\n", count == -1 ? "_all_" : pc_displays[count], valid_color[y] );

   if( !str_cmp( argument, "blink" ) )
      y += AT_BLINK;

   if( count == -1 )
   {
      int ccount;

      for( ccount = 0; ccount < MAX_COLORS; ++ccount )
         ch->pcdata->colors[ccount] = y;

      ch->set_pager_color( y );

      ch->pagerf( "All color types set to color %s%s.%s\r\n", valid_color[y > AT_BLINK ? y - AT_BLINK : y], y > AT_BLINK ? " [BLINKING]" : "", ANSI_RESET );
   }
   else
   {
      ch->pcdata->colors[count] = y;

      ch->set_color( count );

      if( !str_cmp( argument, "blink" ) )
         ch->printf( "Display %s set to color %s [BLINKING]%s\r\n", pc_displays[count], valid_color[y - AT_BLINK], ANSI_RESET );
      else
         ch->printf( "Display %s set to color %s.\r\n", pc_displays[count], valid_color[y] );
   }
}

const char *char_data::color_str( short AType )
{
   if( !has_pcflag( PCFLAG_ANSI ) )
      return ( "" );

   switch ( pcdata->colors[AType] )
   {
      case 0:
         return ( ANSI_BLACK );
      case 1:
         return ( ANSI_DRED );
      case 2:
         return ( ANSI_DGREEN );
      case 3:
         return ( ANSI_ORANGE );
      case 4:
         return ( ANSI_DBLUE );
      case 5:
         return ( ANSI_PURPLE );
      case 6:
         return ( ANSI_CYAN );
      case 7:
         return ( ANSI_GREY );
      case 8:
         return ( ANSI_DGREY );
      case 9:
         return ( ANSI_RED );
      case 10:
         return ( ANSI_GREEN );
      case 11:
         return ( ANSI_YELLOW );
      case 12:
         return ( ANSI_BLUE );
      case 13:
         return ( ANSI_PINK );
      case 14:
         return ( ANSI_LBLUE );
      case 15:
         return ( ANSI_WHITE );

         /*
          * 17 thru 32 are for blinking colors 
          */
      case 17:
         return ( BLINK_BLACK );
      case 18:
         return ( BLINK_DRED );
      case 19:
         return ( BLINK_DGREEN );
      case 20:
         return ( BLINK_ORANGE );
      case 21:
         return ( BLINK_DBLUE );
      case 22:
         return ( BLINK_PURPLE );
      case 23:
         return ( BLINK_CYAN );
      case 24:
         return ( BLINK_GREY );
      case 25:
         return ( BLINK_DGREY );
      case 26:
         return ( BLINK_RED );
      case 27:
         return ( BLINK_GREEN );
      case 28:
         return ( BLINK_YELLOW );
      case 29:
         return ( BLINK_BLUE );
      case 30:
         return ( BLINK_PINK );
      case 31:
         return ( BLINK_LBLUE );
      case 32:
         return ( BLINK_WHITE );

      default:
         return ( ANSI_RESET );
   }
}

/* Random Ansi Color Code -- Xorith */
const char *random_ansi( short type )
{
   switch ( type )
   {
      default:
      case 1: /* Default ANSI Fore-ground */
         switch ( number_range( 1, 15 ) )
         {
            case 1:
               return ( ANSI_DRED );
            case 2:
               return ( ANSI_DGREEN );
            case 3:
               return ( ANSI_ORANGE );
            case 4:
               return ( ANSI_DBLUE );
            case 5:
               return ( ANSI_PURPLE );
            case 6:
               return ( ANSI_CYAN );
            case 7:
               return ( ANSI_GREY );
            case 8:
               return ( ANSI_DGREY );
            case 9:
               return ( ANSI_RED );
            case 10:
               return ( ANSI_GREEN );
            case 11:
               return ( ANSI_YELLOW );
            case 12:
               return ( ANSI_BLUE );
            case 13:
               return ( ANSI_PINK );
            case 14:
               return ( ANSI_LBLUE );
            case 15:
               return ( ANSI_WHITE );
            default:
               return ( ANSI_RESET );
         }

      case 2: /* ANSI Blinking */
         switch ( number_range( 1, 14 ) )
         {
            case 1:
               return ( BLINK_DGREEN );
            case 2:
               return ( BLINK_ORANGE );
            case 3:
               return ( BLINK_DBLUE );
            case 4:
               return ( BLINK_PURPLE );
            case 5:
               return ( BLINK_CYAN );
            case 6:
               return ( BLINK_GREY );
            case 7:
               return ( BLINK_DGREY );
            case 8:
               return ( BLINK_RED );
            case 9:
               return ( BLINK_GREEN );
            case 10:
               return ( BLINK_YELLOW );
            case 11:
               return ( BLINK_BLUE );
            case 12:
               return ( BLINK_PINK );
            case 13:
               return ( BLINK_LBLUE );
            default:
            case 14:
               return ( BLINK_WHITE );
         }

      case 3: /* ANSI Background */
         switch ( number_range( 1, 7 ) )
         {
            case 1:
               return ( BACK_DRED );
            case 2:
               return ( BACK_DGREEN );
            case 3:
               return ( BACK_ORANGE );
            case 4:
               return ( BACK_DBLUE );
            case 5:
               return ( BACK_PURPLE );
            case 6:
               return ( BACK_CYAN );
            case 7:
               return ( BACK_GREY );
            default:
               return ( BACK_GREY );
         }
   }
}

/*
 * Quixadhal - I rewrote this from scratch.  It now returns the number of
 * characters in the SOURCE string that should be skipped, it always fills
 * the DESTINATION string with a valid translation (even if that is itself,
 * or an empty string), and the default for ANSI is false, since mobs and
 * logfiles shouldn't need colour.
 */
std::string colorcode( std::string_view src, descriptor_data * d, size_t & consumed )
{
   consumed = 0;

   if( src.size() < 2 )
      return ""; // Need at least 2 chars for a token

   char type = src[0];
   char code = src[1];

   // Handle Escaped characters (&&, {{, }})
   if( code == type )
   {
      consumed = 2;
      return std::string( 1, type );
   }

   // Determine ANSI support
   bool ansi = false;
   char_data* ch = nullptr;

   if( d )
   {
      ch = d->original ? d->original : d->character;
      ansi = ch ? ch->has_pcflag( PCFLAG_ANSI ) : true;
   }

   if( !ansi )
   {
      consumed = 2;
      return "";
   }

   if( type == '&' && code == '[' )
   {
      size_t close_pos = src.find( ']' );

      if( close_pos != std::string_view::npos )
      {
         std::string key( src.substr( 2, close_pos - 2 ) );

         consumed = close_pos + 1;

         for( int i = 0; i < MAX_COLORS; ++i )
         {
            if( key == pc_displays[i] && ch && ch->desc )
            {
               return ch->color_str( i );
            }
         }
         return "";
      }
   }

   consumed = 2;
   switch( type )
   {
      case '&': // Foreground
         switch( code )
         {
            case 'Z': return random_ansi( 1 );
            case 'i': case 'I': return ANSI_ITALIC;
            case 'v': case 'V': return ANSI_REVERSE;
            case 'u': case 'U': return ANSI_UNDERLINE;
            case 's': case 'S': return ANSI_STRIKEOUT;
            case 'd': return ANSI_RESET;
            case 'D': return std::string( ANSI_RESET ) + ( ch && ch->desc ? ch->color_str( ch->desc->pagecolor ) : "" );
            case 'x': return ANSI_BLACK;    case 'O': return ANSI_ORANGE;
            case 'c': return ANSI_CYAN;     case 'z': return ANSI_DGREY;
            case 'g': return ANSI_DGREEN;   case 'G': return ANSI_GREEN;
            case 'P': return ANSI_PINK;     case 'r': return ANSI_DRED;
            case 'b': return ANSI_DBLUE;    case 'w': return ANSI_GREY;
            case 'Y': return ANSI_YELLOW;   case 'C': return ANSI_LBLUE;
            case 'p': return ANSI_PURPLE;   case 'R': return ANSI_RED;
            case 'B': return ANSI_BLUE;     case 'W': return ANSI_WHITE;
            default: break;
         }
         break;

      case '{': // Background
         switch( code )
         {
            case 'Z': return random_ansi( 3 );
            case 'x': return BACK_BLACK;   case 'r': return BACK_DRED;
            case 'g': return BACK_DGREEN;  case 'O': return BACK_ORANGE;
            case 'b': return BACK_DBLUE;   case 'p': return BACK_PURPLE;
            case 'c': return BACK_CYAN;    case 'w': return BACK_GREY;
            default: break;
         }
         break;

      case '}': // Blink
         switch( code )
         {
            case 'Z': return random_ansi( 2 );
            case 'x': return BLINK_BLACK;   case 'O': return BLINK_ORANGE;
            case 'c': return BLINK_CYAN;    case 'z': return BLINK_DGREY;
            case 'g': return BLINK_DGREEN;  case 'G': return BLINK_GREEN;
            case 'P': return BLINK_PINK;    case 'r': return BLINK_DRED;
            case 'b': return BLINK_DBLUE;   case 'w': return BLINK_GREY;
            case 'Y': return BLINK_YELLOW;  case 'C': return BLINK_LBLUE;
            case 'p': return BLINK_PURPLE;  case 'R': return BLINK_RED;
            case 'B': return BLINK_BLUE;    case 'W': return BLINK_WHITE;
            default: break;
         }
         break;

      default: break;
   }

   // Default: return the original sequence as a string
   return std::string( src.substr( 0, 2 ) );
}

/*
 * Quixadhal - I rewrote this too, so that it uses colorcode.  It may not
 * be as efficient as just walking over the string and counting, but it
 * keeps us from duplicating the code several times.
 *
 * This function returns the intended screen length of a string which has
 * color codes embedded in it.  It does this by stripping the codes out
 * entirely (A nullptr descriptor means ANSI will be false).
 */
int color_strlen( std::string_view src )
{
   int visual_len = 0;
   size_t i = 0;

   while( i < src.length() )
   {
      if( src[i] == '&' || src[i] == '{' || src[i] == '}' )
      {
         size_t consumed = 0;

         // We don't need a buffer, just the skip count
         // You can adjust colorcode to return just the skip count if needed
         std::string translation = colorcode( src.substr(i), nullptr, consumed );

         if( consumed > 0 )
         {
            i += consumed;
         }
         else
         {
            visual_len++;
            i++;
         }
      }
      else
      {
         visual_len++;
         i++;
      }
   }
   return visual_len;
}

/*
 * Quixadhal - And this one needs to use the new version too.
 */
std::string color_align( const std::string & argument, int size, int align )
{
   int visual_len = color_strlen( argument );

   // If the string is already long enough, return it immediately.
   if( visual_len >= size )
   {
      return argument;
   }

   int space = size - visual_len;

   switch( align )
   {
      case ALIGN_RIGHT:
      {
         std::string pad( space, ' ' );
         return pad + argument;
      }
      case ALIGN_LEFT:
      {
         std::string pad( space, ' ' );
         return argument + pad;
      }
      case ALIGN_CENTER:
      {
         int left = space / 2;
         int right = space - left;
         std::string pad_left( left, ' ' );
         std::string pad_right( right, ' ' );
         return pad_left + argument + pad_right;
      }
      default:
         return argument;
   }
}

/*
 * Quixadhal - This takes a string and converts any and all color tokens
 * in it to the desired output tokens, using the provided character's
 * preferences.
 */
std::string colorize( std::string_view txt, descriptor_data * d )
{
   if( txt.empty() || !d )
      return txt.data();

   std::string result;

   result.reserve( txt.length() );

   for( size_t i = 0; i < txt.length(); )
   {
      if( txt[i] == '&' || txt[i] == '{' || txt[i] == '}' || ( txt.compare( i, 4, "http" ) == 0 || txt.compare( i, 5, "https" ) == 0 ) )
      {
         // Handle HTTP/HTTPS link: find the next space or end of string
         if( txt.compare( i, 4, "http" ) == 0 || txt.compare( i, 5, "https" ) == 0 )
         {
            size_t end = txt.find_first_of( " \t\n\r", i );

            if( end == std::string::npos )
               end = txt.length();

            result.append( txt, i, end - i );
            i = end;
            continue;
         }

         // Handle color tokens
         size_t consumed = 0;
         std::string translation = colorcode( txt.substr( i ), d, consumed );

         if( consumed > 0 )
         {
            result += translation;
            i += consumed;
         }
         else
         {
            result += txt[i++];
         }
      }
      else
      {
         result += txt[i++];
      }
   }
   return result;
}
