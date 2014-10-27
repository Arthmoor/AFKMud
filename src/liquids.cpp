/****************************************************************************
 *                   ^     +----- |  / ^     ^ |     | +-\                  *
 *                  / \    |      | /  |\   /| |     | |  \                 *
 *                 /   \   +---   |<   | \ / | |     | |  |                 *
 *                /-----\  |      | \  |  v  | |     | |  /                 *
 *               /       \ |      |  \ |     | +-----+ +-/                  *
 ****************************************************************************
 * AFKMud Copyright 1997-2009 by Roger Libiez (Samson),                     *
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
 *                         Liquidtable Sourcefile                           *
 *                     by Noplex (noplex@crimsonblade.org)                  *
 *                   Redistributed in AFKMud with permission                *
 ****************************************************************************
 * 	                       Version History      			          *
 ****************************************************************************
 *  (v1.0) - Liquidtable converted into linked list, original 15 Smaug liqs *
 *           now read from a .dat file in /system                           *
 *  (v1.5) - OLC support added to create, edit, and delete liquids while    *
 *           the game is still running, automatic edit.                     *
 *  (v2.0) - Mixture support code added. Liquids can now be mixed with      *
 *           other liquids to form a result.                                *
 *  (v2.2) - Liquid statistics command added (liquids) shows all information*
 *           about the given liquid.                                        *
 *  (v2.3) - OLC addition for mixtures.                                     *
 *  (v2.4) - Mixtures are now saved into a seperate file and one linked list*
 *           because of some saving and loading issues. All the code has    *
 *           been modified to accept the new format. "liq_can_mix" function *
 *           introduced. "mix" command introduced to mix liquids.           *
 *  (v2.5) - Thanks to Samson for some polishing and bugfixing, we now have *
 *           a (hopefully) fully funcitonal copy =).                        *
 *  (v2.6) - "Fill" and "Empty" functions have been fixed to allow for the  *
 *           new liquidsystem.                                              *
 *  (v2.7) - Forgot to fix blood support... fixed.                          *
 *         - IS_VAMPIRE ifcheck placed in do_drink                          * 
 *  	   - Blood fix for blood on the ground.                               *
 *         - do_look/do_exam fix from Sirek.                                *
 *  (v2.8) - Ability to mix objects into liquids.                           *
 *	     (original code/concept -Sirek)                                   *
 ****************************************************************************/

/*
 * File: liquids.c
 * Name: Liquidtable Module (3.0b)
 * Author: John 'Noplex' Bellone (jbellone@comcast.net)
 * Terms:
 * If this file is to be re-disributed; you must send an email
 * to the author. All headers above the #include calls must be
 * kept intact. All license requirements must be met. License
 * can be found in the included license.txt document or on the
 * website.
 * Description:
 * This module is a rewrite of the original module which allowed for
 * a SMAUG mud to have a fully online editable liquidtable; adding liquids;
 * removing them; and editing them online. It allows an near-endless supply
 * of liquids for builder's to work with.
 * A second addition to this module allowed for builder's to create mixtures;
 * when two liquids were mixed together they would produce a different liquid.
 * Yet another adaptation to the above concept allowed for objects to be mixed
 * with liquids to produce a liquid.
 * This newest version offers a cleaner running code; smaller; and faster in
 * all ways around. Hopefully it'll knock out the old one ten fold ;)
 * Also in the upcoming 'testing' phase of this code; new additions will be added
 * including a better alchemey system for creating poitions as immortals; and as
 * mortals.
 */

#include "mud.h"
#include "liquids.h"
#include "mud_prog.h"
#include "objindex.h"
#include "roomindex.h"

/* globals */
liquid_data *liquid_table[MAX_LIQUIDS];
list < mixture_data * >mixlist;

const char *liquid_types[LIQTYPE_TOP] = {
   "Beverage", "Alcohol", "Poison", "Unused"
};

const char *mod_types[MAX_CONDS] = {
   "Drunk", "Full", "Thirst"
};

/* locals */
int top_liquid;
int liq_count;
int file_version;

/* liquid i/o functions */
liquid_data::liquid_data(  )
{
   init_memory( &mod, &type, sizeof( type ) );
}

liquid_data::~liquid_data(  )
{
   if( vnum >= top_liquid )
   {
      int j;

      for( j = 0; j != vnum; ++j )
         if( j > top_liquid )
            top_liquid = j;
   }
   liquid_table[vnum] = NULL;
   --liq_count;
}

mixture_data::mixture_data(  )
{
   init_memory( &data, &object, sizeof( object ) );
}

mixture_data::~mixture_data(  )
{
   mixlist.remove( this );
}

/* save the liquids to the liquidtable.dat file in the system directory -Nopey */
void save_liquids( void )
{
   FILE *fp = NULL;
   liquid_data *liq = NULL;
   char filename[256];
   int i;

   snprintf( filename, 256, "%sliquids.dat", SYSTEM_DIR );
   if( !( fp = fopen( filename, "w" ) ) )
   {
      bug( "%s: cannot open %s for writing", __FUNCTION__, filename );
      return;
   }

   fprintf( fp, "%s", "#VERSION 3\n" );
   for( i = 0; i <= top_liquid; ++i )
   {
      if( !liquid_table[i] )
         continue;

      liq = liquid_table[i];

      fprintf( fp, "%s", "#LIQUID\n" );
      fprintf( fp, "Name      %s~\n", liq->name.c_str(  ) );
      fprintf( fp, "Shortdesc %s~\n", liq->shortdesc.c_str(  ) );
      fprintf( fp, "Color     %s~\n", liq->color.c_str(  ) );
      fprintf( fp, "Type      %d\n", liq->type );
      fprintf( fp, "Vnum      %d\n", liq->vnum );
      fprintf( fp, "Mod       %d %d %d\n", liq->mod[COND_DRUNK], liq->mod[COND_FULL], liq->mod[COND_THIRST] );
      fprintf( fp, "%s", "End\n\n" );
   }
   fprintf( fp, "%s", "#END\n" );
   FCLOSE( fp );
}

/* read the liquids from a file descriptor and then distribute them accordingly to the struct -Nopey */
liquid_data *fread_liquid( FILE * fp )
{
   liquid_data *liq = new liquid_data;

   liq->vnum = -1;
   liq->type = -1;
   for( int i = 0; i < MAX_CONDS; ++i )
      liq->mod[i] = -1;

   for( ;; )
   {
      const char *word = ( feof( fp ) ? "End" : fread_word( fp ) );

      if( word[0] == '\0' )
      {
         bug( "%s: EOF encountered reading file!", __FUNCTION__ );
         word = "End";
      }

      switch ( UPPER( word[0] ) )
      {
         default:
            bug( "%s: no match: %s", __FUNCTION__, word );
            fread_to_eol( fp );
            break;

         case '*':
            fread_to_eol( fp );
            break;

         case 'C':
            STDSKEY( "Color", liq->color );
            break;

         case 'E':
            if( !str_cmp( word, "End" ) )
            {
               if( liq->vnum <= -1 )
                  return NULL;
               return liq;
            }
            break;

         case 'N':
            STDSKEY( "Name", liq->name );
            break;

         case 'M':
            if( !str_cmp( word, "Mod" ) )
            {
               liq->mod[COND_DRUNK] = fread_number( fp );
               liq->mod[COND_FULL] = fread_number( fp );
               liq->mod[COND_THIRST] = fread_number( fp );
               if( file_version < 3 )
                  fread_number( fp );
            }
            break;

         case 'S':
            STDSKEY( "Shortdesc", liq->shortdesc );
            break;

         case 'T':
            KEY( "Type", liq->type, fread_number( fp ) );
            break;

         case 'V':
            KEY( "Vnum", liq->vnum, fread_number( fp ) );
            break;
      }
   }
}

/* load the liquids from the liquidtable.dat file in the system directory -Nopey */
void load_liquids( void )
{
   FILE *fp = NULL;
   char filename[256];
   int x;

   file_version = 0;
   snprintf( filename, 256, "%sliquids.dat", SYSTEM_DIR );
   if( !( fp = fopen( filename, "r" ) ) )
   {
      bug( "%s: cannot open %s for reading", __FUNCTION__, filename );
      return;
   }

   top_liquid = -1;
   liq_count = 0;

   for( x = 0; x < MAX_LIQUIDS; ++x )
      liquid_table[x] = NULL;

   for( ;; )
   {
      char letter = fread_letter( fp );
      char *word;

      if( letter == '*' )
      {
         fread_to_eol( fp );
         continue;
      }

      if( letter != '#' )
      {
         bug( "%s: # not found (%c)", __FUNCTION__, letter );
         return;
      }

      word = fread_word( fp );
      if( !str_cmp( word, "VERSION" ) )
      {
         file_version = fread_number( fp );
         continue;
      }
      else if( !str_cmp( word, "LIQUID" ) )
      {
         liquid_data *liq = fread_liquid( fp );

         if( !liq )
            bug( "%s: returned NULL liquid", __FUNCTION__ );
         else
         {
            liquid_table[liq->vnum] = liq;
            if( liq->vnum > top_liquid )
               top_liquid = liq->vnum;
            ++liq_count;
         }
         continue;
      }
      else if( !str_cmp( word, "END" ) )
         break;
      else
      {
         bug( "%s: no match for %s", __FUNCTION__, word );
         continue;
      }
   }
   FCLOSE( fp );
}

/* save the mixtures to the mixture table -Nopey */
void save_mixtures( void )
{
   list < mixture_data * >::iterator imix;
   FILE *fp = NULL;
   char filename[256];

   snprintf( filename, 256, "%smixtures.dat", SYSTEM_DIR );
   if( !( fp = fopen( filename, "w" ) ) )
   {
      bug( "%s: cannot open %s for writing", __FUNCTION__, filename );
      return;
   }

   fprintf( fp, "%s", "#VERSION 3\n" );
   for( imix = mixlist.begin(  ); imix != mixlist.end(  ); ++imix )
   {
      mixture_data *mix = *imix;

      fprintf( fp, "%s", "#MIXTURE\n" );
      fprintf( fp, "Name   %s~\n", mix->name.c_str(  ) );
      fprintf( fp, "Data   %d %d %d\n", mix->data[0], mix->data[1], mix->data[2] );
      fprintf( fp, "Object %d\n", mix->object );
      fprintf( fp, "%s", "End\n" );
   }
   fprintf( fp, "%s", "#END\n" );
   FCLOSE( fp );
}

/* read the mixtures into the structure -Nopey */
mixture_data *fread_mixture( FILE * fp )
{
   mixture_data *mix = new mixture_data;

   mix->data[0] = -1;
   mix->data[1] = -1;
   mix->data[2] = -1;
   mix->object = false;

   for( ;; )
   {
      const char *word = ( feof( fp ) ? "End" : fread_word( fp ) );

      if( word[0] == '\0' )
      {
         bug( "%s: EOF encountered reading file!", __FUNCTION__ );
         word = "End";
      }

      switch ( UPPER( word[0] ) )
      {
         default:
            bug( "%s: no match: %s", __FUNCTION__, word );
            fread_to_eol( fp );
            break;

         case '*':
            fread_to_eol( fp );
            break;

         case 'D':
            if( !str_cmp( word, "Data" ) )
            {
               mix->data[0] = fread_number( fp );
               mix->data[1] = fread_number( fp );
               mix->data[2] = fread_number( fp );
            }
            break;

         case 'E':
            if( !str_cmp( word, "End" ) )
            {
               return mix;
            }
            break;

         case 'I':
            KEY( "Into", mix->data[2], fread_number( fp ) );
            break;

         case 'N':
            STDSKEY( "Name", mix->name );
            break;

         case 'O':
            KEY( "Object", mix->object, fread_number( fp ) );
            break;

         case 'W':
            if( !str_cmp( word, "With" ) )
            {
               mix->data[0] = fread_number( fp );
               mix->data[1] = fread_number( fp );
            }
            break;
      }
   }
}

/* load the mixtures from the mixture table - Nopey */
void load_mixtures( void )
{
   FILE *fp = NULL;
   char filename[256];

   mixlist.clear(  );
   file_version = 0;

   snprintf( filename, 256, "%smixtures.dat", SYSTEM_DIR );
   if( !( fp = fopen( filename, "r" ) ) )
   {
      bug( "%s: cannot open %s for reading", __FUNCTION__, filename );
      return;
   }

   for( ;; )
   {
      char letter = fread_letter( fp );
      char *word;

      if( letter == '*' )
      {
         fread_to_eol( fp );
         break;
      }

      if( letter != '#' )
      {
         bug( "%s: # not found (%c)", __FUNCTION__, letter );
         return;
      }

      word = fread_word( fp );
      if( !str_cmp( word, "VERSION" ) )
      {
         file_version = fread_number( fp );
         continue;
      }
      else if( !str_cmp( word, "MIXTURE" ) )
      {
         mixture_data *mix = NULL;

         mix = fread_mixture( fp );
         if( !mix )
            bug( "%s: mixture returned NULL", __FUNCTION__ );
         else
            mixlist.push_back( mix );
      }
      else if( !str_cmp( word, "END" ) )
         break;
      else
      {
         bug( "%s: no match for %s", __FUNCTION__, word );
         break;
      }
   }
   FCLOSE( fp );
}

/* figure out a vnum for the next liquid  -Nopey */
static int figure_liq_vnum( void )
{
   int i;

   /*
    * incase a liquid gets removed; we can fill it's place 
    */
   for( i = 0; liquid_table[i] != NULL; ++i );

   /*
    * add to the top 
    */
   if( i > top_liquid )
      top_liquid = i;

   return i;
}

/* lookup func for liquids      -Nopey */
liquid_data *get_liq( const string & str )
{
   int i;

   if( is_number( str ) )
   {
      i = atoi( str.c_str(  ) );

      return liquid_table[i];
   }
   else
   {
      for( i = 0; i < top_liquid; ++i )
         if( !str_cmp( liquid_table[i]->name, str ) )
            return liquid_table[i];
   }
   return NULL;
}

liquid_data *get_liq_vnum( int vnum )
{
   return liquid_table[vnum];
}

/* lookup func for mixtures - Nopey */
mixture_data *get_mix( const string & str )
{
   list < mixture_data * >::iterator imix;

   for( imix = mixlist.begin(  ); imix != mixlist.end(  ); ++imix )
   {
      mixture_data *mix = *imix;

      if( !str_cmp( mix->name, str ) )
         return mix;
   }
   return NULL;
}

/* Function to display liquid list. - Tarl 9 Jan 03 */
CMDF( do_showliquid )
{
   liquid_data *liq = NULL;
   int i;

   if( !ch->is_immortal(  ) || ch->isnpc(  ) )
   {
      ch->print( "Huh?\r\n" );
      return;
   }

   if( !liquid_table[0] )
   {
      ch->print( "There are currently no liquids loaded.\r\n" );
      ch->print( "WARNING:\r\nHaving no liquids loaded will result in no drinks providing\r\n"
                 "nurishment. This includes water. The default set of liquids\r\n" "should always be backed up.\r\n" );
      return;
   }

   if( !argument.empty(  ) && ( ( liq = get_liq( argument ) ) != NULL ) )
   {
      if( !liq->name.empty(  ) )
         ch->pagerf( "&GLiquid information for:&g %s\r\n", liq->name.c_str(  ) );
      if( !liq->shortdesc.empty(  ) )
         ch->pagerf( "&GLiquid shortdesc:&g\t %s\r\n", liq->shortdesc.c_str(  ) );
      if( !liq->color.empty(  ) )
         ch->pagerf( "&GLiquid color:&g\t %s\r\n", liq->color.c_str(  ) );
      ch->pagerf( "&GLiquid vnum:&g\t %d\r\n", liq->vnum );
      ch->pagerf( "&GLiquid type:&g\t %s\r\n", liquid_types[liq->type] );
      ch->pagerf( "&GLiquid Modifiers\r\n" );
      for( i = 0; i < MAX_CONDS; ++i )
         if( liquid_table[i] )
            ch->pagerf( "&G%s:&g\t %d\r\n", mod_types[i], liq->mod[i] );
      return;
   }
   else if( !argument.empty(  ) && !( liq = get_liq( argument ) ) )
   {
      ch->print( "Invaild liquid-vnum.\r\nUse 'showliquid' to gain a vaild liquidvnum.\r\n" );
      return;
   }
   ch->pager( "&G[&gVnum&G] [&gName&G]\r\n" );
   ch->pager( "-------------------------\r\n" );
   for( i = 0; i <= top_liquid; ++i )
   {
      if( !liquid_table[i] )
         continue;
      ch->pagerf( "  %-7d %s\r\n", liquid_table[i]->vnum, liquid_table[i]->name.c_str(  ) );
   }
   ch->pager( "\r\nUse 'showliquid [vnum]' to view individual liquids.\r\n" );
   ch->pager( "Use 'showmixture' to view the mixturetable.\r\n" );
}

/* olc function for liquids   -Nopey */
CMDF( do_setliquid )
{
   string arg;

   if( !ch->is_immortal(  ) || ch->isnpc(  ) )
   {
      ch->print( "Huh?\r\n" );
      return;
   }

   smash_tilde( argument );
   argument = one_argument( argument, arg );

   if( arg.empty(  ) )
   {
      ch->print( "Syntax: setliquid <vnum> <field> <value>\r\n" "        setliquid create <name>\r\n" "        setliquid delete <vnum>\r\n" );
      ch->print( " Fields being one of the following:\r\n" " name color type shortdesc drunk thrist blood full\r\n" );
      return;
   }

   if( !str_cmp( arg, "create" ) )
   {
      liquid_data *liq = NULL;
      int i;

      if( liq_count >= MAX_LIQUIDS )
      {
         ch->print( "Liquid count is at the hard-coded max. Remove some liquids or raise\r\n" "the hard-coded max number of liquids.\r\n" );
         return;
      }

      if( argument.empty(  ) )
      {
         ch->print( "Syntax: setliquid create <name>\r\n" );
         return;
      }

      liq = new liquid_data;
      liq->name = argument;
      liq->shortdesc = argument;
      liq->vnum = figure_liq_vnum(  );
      liq->type = -1;
      for( i = 0; i < MAX_CONDS; ++i )
         liq->mod[i] = -1;
      liquid_table[liq->vnum] = liq;
      ++liq_count;
      ch->print( "Done.\r\n" );
      save_liquids(  );
      return;
   }
   else if( !str_cmp( arg, "delete" ) )
   {
      liquid_data *liq = NULL;

      if( argument.empty(  ) )
      {
         ch->print( "Syntax: setliquid delete <vnum>\r\n" );
         return;
      }

      if( !is_number( argument ) )
      {
         if( !( liq = get_liq( argument ) ) )
         {
            ch->print( "No such liquid type. Use 'showliquid' to get a valid list.\r\n" );
            return;
         }
      }
      else
      {
         int i = atoi( argument.c_str(  ) );

         if( !( liq = get_liq_vnum( i ) ) )
         {
            ch->print( "No such vnum. Use 'showliquid' to get the vnum.\r\n" );
            return;
         }
      }
      deleteptr( liq );
      ch->print( "Done.\r\n" );
      save_liquids(  );
      return;
   }
   else
   {
      string arg2;
      liquid_data *liq = NULL;

      argument = one_argument( argument, arg2 );
      if( arg2.empty(  ) )
      {
         ch->print( "Syntax: setliquid <vnum> <field> <value>\r\n" );
         ch->print( " Fields being one of the following:\r\n" " name color shortdesc drunk thrist blood full\r\n" );
         return;
      }

      if( !( liq = get_liq( arg ) ) )
      {
         ch->print( "Invaild liquid-name or vnum.\r\n" );
         return;
      }

      if( !str_cmp( arg2, "name" ) )
      {
         if( argument.empty(  ) )
         {
            ch->print( "Syntax: setliquid <vnum> name <name>\r\n" );
            return;
         }
         liq->name = argument;
      }
      else if( !str_cmp( arg2, "color" ) )
      {
         if( argument.empty(  ) )
         {
            ch->print( "Syntax: setliquid <vnum> color <color>\r\n" );
            return;
         }
         liq->color = argument;
      }
      else if( !str_cmp( arg2, "shortdesc" ) )
      {
         if( argument.empty(  ) )
         {
            ch->print( "Syntax: setliquid <vnum> shortdesc <shortdesc>\r\n" );
            return;
         }
         liq->shortdesc = argument;
      }
      else if( !str_cmp( arg2, "type" ) )
      {
         string arg3;
         int i;
         bool found = false;

         argument = one_argument( argument, arg3 );

         /*
          * bah; forgot to add this shit -- 
          */
         for( i = 0; i < LIQTYPE_TOP; ++i )
            if( !str_cmp( arg3, liquid_types[i] ) )
            {
               found = true;
               liq->type = i;
            }
         if( !found )
         {
            ch->print( "Syntax: setliquid <vnum> type <liquidtype>\r\n" );
            return;
         }
      }
      else
      {
         int i;
         bool found = false;
         static const char *arg_names[MAX_CONDS] = { "drunk", "full", "thirst" };

         if( argument.empty(  ) )
         {
            ch->print( "Syntax: setliquid <vnum> <field> <value>\r\n" );
            ch->print( " Fields being one of the following:\r\n" " name color shortdesc drunk thrist blood full\r\n" );
            return;
         }

         for( i = 0; i < MAX_CONDS; ++i )
            if( !str_cmp( arg2, arg_names[i] ) )
            {
               found = true;
               liq->mod[i] = atoi( argument.c_str(  ) );
            }
         if( !found )
         {
            do_setliquid( ch, "" );
            return;
         }
      }
      ch->print( "Done.\r\n" );
      save_liquids(  );
      return;
   }
}

void displaymixture( char_data * ch, mixture_data * mix )
{
   ch->pager( " .-.                ,\r\n" );
   ch->pager( "`._ ,\r\n" );
   ch->pager( "   \\ \\                 o\r\n" );
   ch->pager( "    \\ `-,.\r\n" );
   ch->pager( "   .'o .  `.[]           o\r\n" );
   ch->pager( "<~- - , ,[].'.[] ~>     ___\r\n" );
   ch->pager( " :               :     (-~.)\r\n" );
   ch->pager( "  `             '       `|'\r\n" );
   ch->pager( "   `           '         |\r\n" );
   ch->pager( "    `-.     .-'          |\r\n" );
   ch->pager( "-----{. _ _ .}-------------------\r\n" );

   ch->pagerf( "&gRecipe for Mixture &G%s&g:\r\n", mix->name.c_str(  ) );
   ch->pager( "---------------------------------\r\n" );
   if( !mix->object )   //this is an object
   {
      liquid_data *ingred1 = get_liq_vnum( mix->data[0] );
      liquid_data *ingred2 = get_liq_vnum( mix->data[1] );

      ch->pager( "&wCombine two liquids to create this mixture:\r\n" );
      if( !ingred1 )
         ch->pagerf( "Vnum1 (%d) is invalid, tell an Admin\r\n", mix->data[0] );
      else
         ch->pagerf( "&wOne part &G%s&w (%d)\r\n", ingred1->name.c_str(  ), mix->data[0] );

      if( !ingred2 )
         ch->pagerf( "Vnum2 (%d) is invalid, tell an Admin\r\n", mix->data[1] );
      else
         ch->pagerf( "&wAnd part &G%s&w (%d)&D\r\n", ingred2->name.c_str(  ), mix->data[1] );
   }
   else
   {
      obj_index *obj = get_obj_index( mix->data[0] );
      if( !obj )
      {
         ch->pagerf( "%s has a bad object vnum %d, inform an Admin\r\n", mix->name.c_str(  ), mix->data[0] );
         return;
      }
      else
      {
         liquid_data *ingred1 = get_liq_vnum( mix->data[1] );

         ch->pager( "Combine an object and a liquid in this mixture\r\n" );
         ch->pagerf( "&wMix &G%s&w (%d)\r\n", obj->name, mix->data[0] );
         ch->pagerf( "&winto one part &G%s&w (%d)&D\r\n", ingred1->name.c_str(  ), mix->data[1] );
      }
   }
}

/* Function for showmixture - Tarl 9 Jan 03 */
CMDF( do_showmixture )
{
   mixture_data *mix = NULL;
   list < mixture_data * >::iterator imx;

   if( !ch->is_immortal(  ) || ch->isnpc(  ) )
   {
      ch->print( "Huh?\r\n" );
      return;
   }

   if( !argument.empty(  ) && ( ( mix = get_mix( argument ) ) != NULL ) )
   {
      displaymixture( ch, mix );
      return;
   }
   else if( !argument.empty(  ) && !( mix = get_mix( argument ) ) )
   {
      ch->print( "Invaild mixture-name.\r\nUse 'setmixture list' to gain a vaild name.\r\n" );
      return;
   }

   if( mixlist.empty(  ) )
   {
      ch->print( "There are currently no mixtures loaded.\r\n" );
      return;
   }

   ch->pager( "&G[&gType&G] &G[&gName&G]\r\n" );
   ch->pager( "-----------------------\r\n" );
   for( imx = mixlist.begin(  ); imx != mixlist.end(  ); ++imx )
   {
      mixture_data *mx = *imx;

      ch->pagerf( "  %-12s %s\r\n", mx->object ? "&[objects]Object&D" : "&BLiquids&D", mx->name.c_str(  ) );
   }
   ch->pager( "\r\n&gUse 'showmixture [name]' to view individual mixtures.\r\n" );
   ch->pager( "&gUse 'showliquid' to view the liquidtable.\r\n&D" );
}

/* olc funciton for mixtures  -Nopey */
CMDF( do_setmixture )
{
   string arg;
   liquid_data *liq = NULL;

   if( !ch->is_immortal(  ) || ch->isnpc(  ) )
   {
      ch->print( "Huh?\r\n" );
      return;
   }

   smash_tilde( argument );
   argument = one_argument( argument, arg );

   if( arg.empty(  ) )
   {
      ch->print( "Syntax: setmixture create <name>\r\n"
                 "        setmixture delete <name>\r\n"
                 "        setmixture list [name]\r\n" "        setmixture save - (saves table)\r\n" "        setmixture <name> <field> <value>\r\n" );
      ch->print( " Fields being one of the following:\r\n" " name vnum1 vnum2 into object\r\n" );
      return;
   }

   if( !str_cmp( arg, "list" ) )
   {
      mixture_data *mix = NULL;
      list < mixture_data * >::iterator imx;

      if( !argument.empty(  ) && ( ( mix = get_mix( argument ) ) != NULL ) )
      {
         displaymixture( ch, mix );
         return;
      }
      else if( !argument.empty(  ) && !( mix = get_mix( argument ) ) )
      {
         ch->print( "Invaild mixture-name.\r\nUse 'setmixture list' to gain a vaild name.\r\n" );
         return;
      }

      if( mixlist.empty(  ) )
      {
         ch->print( "There are currently no mixtures loaded.\r\n" );
         return;
      }

      ch->pager( "&G[&gType&G] &G[&gName&G]\r\n" );
      ch->pager( "-----------------------\r\n" );
      for( imx = mixlist.begin(  ); imx != mixlist.end(  ); ++imx )
      {
         mixture_data *mx = *imx;

         ch->pagerf( "  %-8s %s\r\n", mx->object ? "Object" : "Liquids", mx->name.c_str(  ) );
      }

      ch->pagerf( "\r\n&gUse 'showmixture [name]' to view individual mixtures.\r\n" );
      ch->pagerf( "&gUse 'showliquid' to view the liquidtable.&D\r\n" );
      return;
   }
   else if( !str_cmp( arg, "create" ) )
   {
      mixture_data *mix = NULL;

      if( argument.empty(  ) )
      {
         ch->print( "Syntax: setmixture create <name>\r\n" );
         return;
      }

      mix = new mixture_data;
      mix->name = argument;
      mix->data[0] = -1;
      mix->data[1] = -1;
      mix->data[2] = -1;
      mix->object = false;
      mixlist.push_back( mix );
      ch->print( "Done.\r\n" );
      save_mixtures(  );
      return;
   }
   else if( !str_cmp( arg, "save" ) )
   {
      save_mixtures(  );
      ch->print( "Mixture table saved.\r\n" );
      return;
   }
   else if( !str_cmp( arg, "delete" ) )
   {
      mixture_data *mix = NULL;

      if( argument.empty(  ) )
      {
         ch->print( "Syntax: setmixture delete <name>\r\n" );
         return;
      }

      if( !( mix = get_mix( argument ) ) )
      {
         ch->print( "That's not a mixture name.\r\n" );
         return;
      }
      deleteptr( mix );
      ch->print( "Done.\r\n" );
      save_mixtures(  );
      return;
   }
   else
   {
      string arg2;
      mixture_data *mix = NULL;

      if( arg.empty(  ) || !( mix = get_mix( arg ) ) )
      {
         ch->print( "Syntax: setmixture <mixname> <field> <value>\r\n" );
         ch->print( " Fields being one of the following:\r\n" " name vnum1 vnum2 into object\r\n" );
         return;
      }

      argument = one_argument( argument, arg2 );

      if( !str_cmp( arg2, "name" ) )
      {
         if( argument.empty(  ) )
         {
            ch->print( "Syntax: setmixture <mixname> name <name>\r\n" );
            return;
         }
         mix->name = argument;
      }
      else if( !str_cmp( arg2, "vnum1" ) )
      {
         int i = 0;

         if( is_number( argument ) )
            i = atoi( argument.c_str(  ) );
         else
         {
            ch->print( "Invalid liquid vnum.\r\n" );
            ch->print( "Syntax: setmixture <mixname> vnum1 <liqvnum or objvnum>\r\n" );
            return;
         }

         if( mix->object )
         {
            obj_index *obj = get_obj_index( i );
            if( !obj )
            {
               ch->printf( "Invalid object vnum %d\r\n", i );
               return;
            }
            else
            {
               mix->data[0] = i;
               ch->printf( "Mixture object set to %d - %s\r\n", i, obj->name );
            }
         }
         else
         {
            liq = get_liq_vnum( i );
            if( !liq )
            {
               ch->printf( "Liquid vnum %d does not exist\r\n", i );
               return;
            }
            else
            {
               mix->data[0] = i;
               ch->printf( "Mixture Vnum1 set to %s \r\n", liq->name.c_str(  ) );
            }
         }
      }
      else if( !str_cmp( arg2, "vnum2" ) )
      {
         int i = 0;

         if( is_number( argument ) )
            i = atoi( argument.c_str(  ) );
         else
         {
            ch->print( "Invalid liquid vnum.\r\n" );
            ch->print( "Syntax: setmixture <mixname> vnum2 <liqvnum>\r\n" );
            return;
         }

         // Verify liq exists
         liq = get_liq_vnum( i );
         if( !liq )
         {
            ch->printf( "Liquid vnum %d does not exist\r\n", i );
            return;
         }
         else
         {
            mix->data[1] = i;
            ch->printf( "Mixture Vnum2 set to %s \r\n", liq->name.c_str(  ) );
         }
      }
      else if( !str_cmp( arg2, "object" ) )
      {
         mix->object = !mix->object;
         if( mix->object )
            ch->print( "Mixture -vnum2- is now an object-vnum.\r\n" );
         else
            ch->print( "Both mixture vnums are now liquids.\r\n" );
      }
      else if( !str_cmp( arg2, "into" ) )
      {
         int i;

         if( is_number( argument ) )
            i = atoi( argument.c_str(  ) );
         else
         {
            ch->print( "Invalid liquid vnum.\r\n" );
            ch->print( "Syntax: setmixture <mixname> into <liqvnum>\r\n" );
            return;
         }

         liq = get_liq_vnum( i );
         if( !liq )
         {
            ch->printf( "Liquid vnum %d does not exist\r\n", i );
            return;
         }
         else
         {
            mix->data[2] = i;
            ch->printf( "Mixture will now turn into %s \r\n", liq->name.c_str(  ) );
         }
      }
      ch->print( "Changes done. Saving table.\r\n" );
      save_mixtures(  );
      return;
   }
}

/* mix a liquid with a liquid; return the final product - Nopey */
liquid_data *liq_can_mix( obj_data * iObj, obj_data * tObj )
{
   list < mixture_data * >::iterator imix;
   mixture_data *mix = NULL;
   bool mix_found = false;

   for( imix = mixlist.begin(  ); imix != mixlist.end(  ); ++imix )
   {
      mix = *imix;

      if( mix->data[0] == iObj->value[2] || mix->data[1] == iObj->value[2] )
      {
         mix_found = true;
         break;
      }
   }
   if( !mix_found )
      return NULL;

   if( mix->data[2] > -1 )
   {
      liquid_data *liq = NULL;

      if( !( liq = get_liq_vnum( mix->data[2] ) ) )
         return NULL;
      else
      {
         iObj->value[1] += tObj->value[1];
         iObj->value[2] = liq->vnum;
         tObj->value[1] = 0;
         tObj->value[2] = -1;
         return liq;
      }
   }
   return NULL;
}

/* used to mix an object with a liquid to form another liquid; returns the result  -Nopey */
liquid_data *liqobj_can_mix( obj_data * iObj, obj_data * oLiq )
{
   list < mixture_data * >::iterator imix;
   mixture_data *mix = NULL;
   bool mix_found = false;

   for( imix = mixlist.begin(  ); imix != mixlist.end(  ); ++imix )
   {
      mix = *imix;

      if( mix->object && ( mix->data[0] == iObj->value[2] || mix->data[1] == iObj->value[2] ) )
         if( mix->data[0] == oLiq->value[2] || mix->data[1] == oLiq->value[2] )
         {
            mix_found = true;
            break;
         }
   }
   if( !mix_found )
      return NULL;

   if( mix->data[2] > -1 )
   {
      liquid_data *liq = NULL;

      if( !( liq = get_liq_vnum( mix->data[2] ) ) )
         return NULL;
      else
      {
         oLiq->value[1] += iObj->value[1];
         oLiq->value[2] = liq->vnum;
         iObj->separate(  );
         iObj->from_char(  );
         iObj->extract(  );
         return liq;
      }
   }
   return NULL;
}

/* the actual -mix- funciton  -Nopey */
CMDF( do_mix )
{
   string arg;
   obj_data *iObj, *tObj = NULL;

   argument = one_argument( argument, arg );
   /*
    * null arguments 
    */
   if( arg.empty(  ) || argument.empty(  ) )
   {
      ch->print( "What would you like to mix together?\r\n" );
      return;
   }

   /*
    * check for objects in the inventory 
    */
   if( !( iObj = ch->get_obj_carry( arg ) ) || !( tObj = ch->get_obj_carry( argument ) ) )
   {
      ch->print( "You aren't carrying that.\r\n" );
      return;
   }

   /*
    * check itemtypes 
    */
   if( ( iObj->item_type != ITEM_DRINK_CON && iObj->item_type != ITEM_DRINK_MIX ) || ( tObj->item_type != ITEM_DRINK_CON && tObj->item_type != ITEM_DRINK_MIX ) )
   {
      ch->print( "You can't mix that!\r\n" );
      return;
   }

   /*
    * check to see if it's empty or not 
    */
   if( iObj->value[1] <= 0 || tObj->value[1] <= 0 )
   {
      ch->print( "It's empty.\r\n" );
      return;
   }

   /*
    * two liquids 
    */
   if( iObj->item_type == ITEM_DRINK_CON && tObj->item_type == ITEM_DRINK_CON )
   {
      /*
       * check to see if the two liquids can be mixed together and return the final liquid -Nopey 
       */
      if( !liq_can_mix( iObj, tObj ) )
      {
         ch->print( "Those two don't mix well together.\r\n" );
         return;
      }
   }
   else if( iObj->item_type == ITEM_DRINK_MIX && tObj->item_type == ITEM_DRINK_CON )
   {
      if( !liqobj_can_mix( tObj, iObj ) )
      {
         ch->print( "Those two don't mix well together.\r\n" );
         return;
      }
   }
   else if( iObj->item_type == ITEM_DRINK_CON && tObj->item_type == ITEM_DRINK_MIX )
   {
      if( !liqobj_can_mix( iObj, tObj ) )
      {
         ch->print( "Those two don't mix well together.\r\n" );
         return;
      }
   }
   else
   {
      ch->print( "Those two don't mix well together.\r\n" );
      return;
   }
   ch->print( "&cYou mix them together.&g\r\n" );
}

CMDF( do_drink )
{
   string arg;

   argument = one_argument( argument, arg );

   /*
    * munch optional words 
    */
   if( !str_cmp( arg, "from" ) && !argument.empty(  ) )
      argument = one_argument( argument, arg );

   obj_data *obj = NULL;
   if( arg.empty(  ) )
   {
      list < obj_data * >::iterator iobj;

      for( iobj = ch->in_room->objects.begin(  ); iobj != ch->in_room->objects.end(  ); ++iobj )
      {
         obj = *iobj;

         if( obj->item_type == ITEM_FOUNTAIN )
            break;
      }

      if( iobj == ch->in_room->objects.end(  ) )
      {
         ch->print( "Drink what?\r\n" );
         return;
      }
   }
   else
   {
      if( !( obj = ch->get_obj_here( arg ) ) )
      {
         ch->print( "You can't find it.\r\n" );
         return;
      }
   }

   if( obj->count > 1 && obj->item_type != ITEM_FOUNTAIN )
      obj->separate(  );

   if( !ch->isnpc(  ) && ch->pcdata->condition[COND_DRUNK] > sysdata->maxcondval - 8 )
   {
      ch->print( "You fail to reach your mouth.  *Hic*\r\n" );
      return;
   }

   bool immuneH = false, immuneT = false;
   if( !ch->isnpc(  ) )
   {
      if( ch->pcdata->condition[COND_THIRST] == -1 )
         immuneT = true;
      if( ch->pcdata->condition[COND_FULL] == -1 )
         immuneH = true;
   }

   switch ( obj->item_type )
   {
      default:
         if( obj->carried_by == ch )
         {
            act( AT_ACTION, "$n lifts $p up to $s mouth and tries to drink from it...", ch, obj, NULL, TO_ROOM );
            act( AT_ACTION, "You bring $p up to your mouth and try to drink from it...", ch, obj, NULL, TO_CHAR );
         }
         else
         {
            act( AT_ACTION, "$n gets down and tries to drink from $p... (Is $e feeling ok?)", ch, obj, NULL, TO_ROOM );
            act( AT_ACTION, "You get down on the ground and try to drink from $p...", ch, obj, NULL, TO_CHAR );
         }
         break;

      case ITEM_BLOOD:
         ch->print( "It is not in your nature to do such things.\r\n" );
         break;

      case ITEM_POTION:
         if( obj->carried_by == ch )
            cmdf( ch, "quaff %s", obj->name );
         else
            ch->print( "You're not carrying that.\r\n" );
         break;

      case ITEM_FOUNTAIN:
      {
         liquid_data *liq = NULL;

         if( obj->value[1] <= 0 )
            obj->value[1] = sysdata->maxcondval;

         if( !( liq = get_liq_vnum( obj->value[2] ) ) )
         {
            bug( "%s: bad liquid number %d.", __FUNCTION__, obj->value[2] );
            liq = get_liq_vnum( 0 );
         }

         if( !ch->isnpc(  ) && obj->value[2] != 0 )
         {
            gain_condition( ch, COND_THIRST, liq->mod[COND_THIRST] );
            gain_condition( ch, COND_FULL, liq->mod[COND_FULL] );
            gain_condition( ch, COND_DRUNK, liq->mod[COND_DRUNK] );
         }
         else if( !ch->isnpc(  ) && obj->value[2] == 0 )
            ch->pcdata->condition[COND_THIRST] = sysdata->maxcondval;

         if( !oprog_use_trigger( ch, obj, NULL, NULL ) )
         {
            act( AT_ACTION, "$n drinks from the fountain.", ch, NULL, NULL, TO_ROOM );
            ch->print( "You take a long thirst quenching drink.\r\n" );
            ch->sound( "drink.wav", 100, false );
         }
         break;
      }

      case ITEM_DRINK_CON:
      {
         liquid_data *liq = NULL;

         if( obj->value[1] <= 0 )
         {
            ch->print( "It is already empty.\r\n" );
            return;
         }

         /*
          * allow water to be drank; but nothing else on a full stomach -Nopey 
          */
         if( !ch->isnpc(  ) && ( ch->pcdata->condition[COND_THIRST] == sysdata->maxcondval || ch->pcdata->condition[COND_FULL] == sysdata->maxcondval ) )
         {
            ch->print( "Your stomach is too full to drink anymore!\r\n" );
            return;
         }

         if( !( liq = get_liq_vnum( obj->value[2] ) ) )
         {
            bug( "%s: bad liquid number %d.", __FUNCTION__, obj->value[2] );
            liq = get_liq_vnum( 0 );
         }

         if( !oprog_use_trigger( ch, obj, NULL, NULL ) )
         {
            act( AT_ACTION, "$n drinks $T from $p.", ch, obj, liq->shortdesc.c_str(  ), TO_ROOM );
            act( AT_ACTION, "You drink $T from $p.", ch, obj, liq->shortdesc.c_str(  ), TO_CHAR );
            ch->sound( "drink.wav", 100, false );
         }

         int amount = 1;   /* UMIN(amount, obj->value[1]); */

         /*
          * gain conditions accordingly              -Nopey 
          */
         gain_condition( ch, COND_DRUNK, liq->mod[COND_DRUNK] );
         gain_condition( ch, COND_FULL, liq->mod[COND_FULL] );
         gain_condition( ch, COND_THIRST, liq->mod[COND_THIRST] );

         if( liq->type == LIQTYPE_POISON )
         {
            affect_data af;

            act( AT_POISON, "$n sputters and gags.", ch, NULL, NULL, TO_ROOM );
            act( AT_POISON, "You sputter and gag.", ch, NULL, NULL, TO_CHAR );
            ch->mental_state = URANGE( 20, ch->mental_state + 5, 100 );
            af.type = gsn_poison;
            af.duration = obj->value[3];
            af.location = APPLY_NONE;
            af.modifier = 0;
            af.bit = AFF_POISON;
            ch->affect_join( &af );
         }

         if( !ch->isnpc(  ) )
         {
            if( ch->pcdata->condition[COND_DRUNK] > ( sysdata->maxcondval / 2 ) && ch->pcdata->condition[COND_DRUNK] < ( sysdata->maxcondval * .4 ) )
               ch->print( "You feel quite sloshed.\r\n" );
            else if( ch->pcdata->condition[COND_DRUNK] >= ( sysdata->maxcondval * .4 ) && ch->pcdata->condition[COND_DRUNK] < ( sysdata->maxcondval * .6 ) )
               ch->print( "You start to feel a little drunk.\r\n" );
            else if( ch->pcdata->condition[COND_DRUNK] >= ( sysdata->maxcondval * .6 ) && ch->pcdata->condition[COND_DRUNK] < ( sysdata->maxcondval * .9 ) )
               ch->print( "Your vision starts to get blurry.\r\n" );
            else if( ch->pcdata->condition[COND_DRUNK] >= ( sysdata->maxcondval * .9 ) && ch->pcdata->condition[COND_DRUNK] < sysdata->maxcondval )
               ch->print( "You feel very drunk.\r\n" );
            else if( ch->pcdata->condition[COND_DRUNK] == sysdata->maxcondval )
               ch->print( "You feel like your going to pass out.\r\n" );

            if( ch->pcdata->condition[COND_THIRST] > ( sysdata->maxcondval / 2 ) && ch->pcdata->condition[COND_THIRST] < ( sysdata->maxcondval * .4 ) )
               ch->print( "Your stomach begins to slosh around.\r\n" );
            else if( ch->pcdata->condition[COND_THIRST] >= ( sysdata->maxcondval * .4 ) && ch->pcdata->condition[COND_THIRST] < ( sysdata->maxcondval * .6 ) )
               ch->print( "You start to feel bloated.\r\n" );
            else if( ch->pcdata->condition[COND_THIRST] >= ( sysdata->maxcondval * .6 ) && ch->pcdata->condition[COND_THIRST] < ( sysdata->maxcondval * .9 ) )
               ch->print( "You feel bloated.\r\n" );
            else if( ch->pcdata->condition[COND_THIRST] >= ( sysdata->maxcondval * .9 ) && ch->pcdata->condition[COND_THIRST] < sysdata->maxcondval )
               ch->print( "You stomach is almost filled to it's brim!\r\n" );
            else if( ch->pcdata->condition[COND_THIRST] == sysdata->maxcondval )
               ch->print( "Your stomach is full, you can't manage to get anymore down.\r\n" );
         }

         obj->value[1] -= amount;
         if( obj->value[1] <= 0 )   /* Come now, what good is a drink container that vanishes?? */
         {
            obj->value[1] = 0;   /* Prevents negative values - Samson */
            ch->print( "You drink the last drop from your container.\r\n" );
         }
         break;
      }
   }

   if( !ch->isnpc(  ) )
   {
      if( immuneH )
         ch->pcdata->condition[COND_FULL] = -1;
      if( immuneT )
         ch->pcdata->condition[COND_THIRST] = -1;
   }

   if( ch->who_fighting(  ) && ch->IS_PKILL(  ) )
      ch->WAIT_STATE( sysdata->pulsepersec / 3 );
   else
      ch->WAIT_STATE( sysdata->pulsepersec );
}

/* standard liquid functions - Nopey */
CMDF( do_fill )
{
   string arg1, arg2;
   obj_data *obj, *source;
   short dest_item, src_item1, src_item2, src_item3;
   int diff = 0;
   bool all = false;

   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );

   /*
    * munch optional words 
    */
   if( ( !str_cmp( arg2, "from" ) || !str_cmp( arg2, "with" ) ) && !argument.empty(  ) )
      argument = one_argument( argument, arg2 );

   if( arg1.empty(  ) )
   {
      ch->print( "Fill what?\r\n" );
      return;
   }

   if( ms_find_obj( ch ) )
      return;

   if( !( obj = ch->get_obj_carry( arg1 ) ) )
   {
      ch->print( "You do not have that item.\r\n" );
      return;
   }
   else
      dest_item = obj->item_type;

   src_item1 = src_item2 = src_item3 = -1;
   switch ( dest_item )
   {
      default:
         act( AT_ACTION, "$n tries to fill $p... (Don't ask me how)", ch, obj, NULL, TO_ROOM );
         ch->print( "You cannot fill that.\r\n" );
         return;
         /*
          * place all fillable item types here 
          */
      case ITEM_DRINK_CON:
         src_item1 = ITEM_FOUNTAIN;
         src_item2 = ITEM_BLOOD;
         break;
      case ITEM_HERB_CON:
         src_item1 = ITEM_HERB;
         src_item2 = ITEM_HERB_CON;
         break;
      case ITEM_PIPE:
         src_item1 = ITEM_HERB;
         src_item2 = ITEM_HERB_CON;
         break;
      case ITEM_CONTAINER:
         src_item1 = ITEM_CONTAINER;
         src_item2 = ITEM_CORPSE_NPC;
         src_item3 = ITEM_CORPSE_PC;
         break;
   }

   if( dest_item == ITEM_CONTAINER )
   {
      if( IS_SET( obj->value[1], CONT_CLOSED ) )
      {
         act( AT_PLAIN, "The $d is closed.", ch, NULL, obj->name, TO_CHAR );
         return;
      }
      if( obj->get_real_weight(  ) / obj->count >= obj->value[0] )
      {
         ch->print( "It's already full as it can be.\r\n" );
         return;
      }
   }
   else
   {
      diff = sysdata->maxcondval;
      if( diff < 1 || obj->value[1] >= obj->value[0] )
      {
         ch->print( "It's already full as it can be.\r\n" );
         return;
      }
   }

   if( dest_item == ITEM_PIPE && IS_SET( obj->value[3], PIPE_FULLOFASH ) )
   {
      ch->print( "It's full of ashes, and needs to be emptied first.\r\n" );
      return;
   }

   if( !arg2.empty(  ) )
   {
      if( dest_item == ITEM_CONTAINER && ( !str_cmp( arg2, "all" ) || !str_prefix( "all.", arg2 ) ) )
      {
         all = true;
         source = NULL;
      }
      else
         /*
          * This used to let you fill a pipe from an object on the ground.  Seems
          * to me you should be holding whatever you want to fill a pipe with.
          * It's nitpicking, but I needed to change it to get a mobprog to work
          * right.  Check out Lord Fitzgibbon if you're curious.  -Narn 
          */
      if( dest_item == ITEM_PIPE )
      {
         if( !( source = ch->get_obj_carry( arg2 ) ) )
         {
            ch->print( "You don't have that item.\r\n" );
            return;
         }
         if( source->item_type != src_item1 && source->item_type != src_item2 && source->item_type != src_item3 )
         {
            act( AT_PLAIN, "You cannot fill $p with $P!", ch, obj, source, TO_CHAR );
            return;
         }
      }
      else
      {
         if( !( source = ch->get_obj_here( arg2 ) ) )
         {
            ch->print( "You cannot find that item.\r\n" );
            return;
         }
      }
   }
   else
      source = NULL;

   if( !source && dest_item == ITEM_PIPE )
   {
      ch->print( "Fill it with what?\r\n" );
      return;
   }

   if( !source )
   {
      obj->separate(  );

      bool found = false;
      list < obj_data * >::iterator iobj;
      for( iobj = ch->in_room->objects.begin(  ); iobj != ch->in_room->objects.end(  ); )
      {
         source = *iobj;
         ++iobj;

         if( dest_item == ITEM_CONTAINER )
         {
            if( !source->wear_flags.test( ITEM_TAKE ) || source->extra_flags.test( ITEM_BURIED )
                || ( source->extra_flags.test( ITEM_PROTOTYPE ) && !ch->can_take_proto(  ) )
                || ch->carry_weight + source->get_weight(  ) > ch->can_carry_w(  ) || ( source->get_real_weight(  ) + obj->get_real_weight(  ) / obj->count ) > obj->value[0] )
               continue;

            if( all && arg2[3] == '.' && !hasname( source->name, arg2.substr( 4, arg2.length(  ) ) ) )
               continue;

            source->from_room(  );
            if( source->item_type == ITEM_MONEY )
            {
               ch->gold += source->value[0];
               source->extract(  );
            }
            else
               source->to_obj( obj );
            found = true;
         }
         else if( source->item_type == src_item1 || source->item_type == src_item2 || source->item_type == src_item3 )
         {
            found = true;
            break;
         }
      }
      if( !found )
      {
         switch ( src_item1 )
         {
            default:
               ch->print( "There is nothing appropriate here!\r\n" );
               return;
            case ITEM_FOUNTAIN:
               ch->print( "There is no fountain or pool here!\r\n" );
               return;
            case ITEM_BLOOD:
               ch->print( "There is no blood pool here!\r\n" );
               return;
            case ITEM_HERB_CON:
               ch->print( "There are no herbs here!\r\n" );
               return;
            case ITEM_HERB:
               ch->print( "You cannot find any smoking herbs.\r\n" );
               return;
         }
      }
      if( dest_item == ITEM_CONTAINER )
      {
         act( AT_ACTION, "You fill $p.", ch, obj, NULL, TO_CHAR );
         act( AT_ACTION, "$n fills $p.", ch, obj, NULL, TO_ROOM );
         return;
      }
   }

   if( dest_item == ITEM_CONTAINER )
   {
      if( source == obj )
      {
         ch->print( "You can't fill something with itself!\r\n" );
         return;
      }

      switch ( source->item_type )
      {
         default:   /* put something in container */
            if( !source->in_room /* disallow inventory items */
                || !source->wear_flags.test( ITEM_TAKE ) || ( source->extra_flags.test( ITEM_PROTOTYPE )
                                                              && !ch->can_take_proto(  ) )
                || ch->carry_weight + source->get_weight(  ) > ch->can_carry_w(  ) || ( source->get_real_weight(  ) + obj->get_real_weight(  ) / obj->count ) > obj->value[0] )
            {
               ch->print( "You can't do that.\r\n" );
               return;
            }
            obj->separate(  );
            act( AT_ACTION, "You take $P and put it inside $p.", ch, obj, source, TO_CHAR );
            act( AT_ACTION, "$n takes $P and puts it inside $p.", ch, obj, source, TO_ROOM );
            source->from_room(  );
            source->to_obj( obj );
            break;

         case ITEM_MONEY:
            ch->print( "You can't do that... yet.\r\n" );
            break;

         case ITEM_CORPSE_PC:
            if( ch->isnpc(  ) )
            {
               ch->print( "You can't do that.\r\n" );
               return;
            }
            if( source->extra_flags.test( ITEM_CLANCORPSE ) && !ch->is_immortal(  ) )
            {
               ch->print( "Your hands fumble. Maybe you better loot a different way.\r\n" );
               return;
            }
            if( !source->extra_flags.test( ITEM_CLANCORPSE ) || !ch->has_pcflag( PCFLAG_DEADLY ) )
            {
               char name[MIL];
               char *pd = source->short_descr;

               pd = one_argument( pd, name );
               pd = one_argument( pd, name );
               pd = one_argument( pd, name );
               pd = one_argument( pd, name );

               if( str_cmp( name, ch->name ) && !ch->is_immortal(  ) )
               {
                  list < char_data * >::iterator ich;

                  for( ich = pclist.begin(  ); ich != pclist.end(  ); ++ich )
                  {
                     char_data *gch = *ich;

                     if( is_same_group( ch, gch ) && !str_cmp( name, gch->name ) )
                     {
                        ch->print( "That's someone else's corpse.\r\n" );
                        return;
                     }
                  }
               }
            }

         case ITEM_CONTAINER:
            if( source->item_type == ITEM_CONTAINER /* don't remove */  && IS_SET( source->value[1], CONT_CLOSED ) )
            {
               act( AT_PLAIN, "The $d is closed.", ch, NULL, source->name, TO_CHAR );
               return;
            }

         case ITEM_CORPSE_NPC:
            if( source->contents.empty(  ) )
            {
               ch->print( "It's empty.\r\n" );
               return;
            }
            obj->separate(  );

            bool found = false;
            list < obj_data * >::iterator iobj;
            for( iobj = source->contents.begin(  ); iobj != source->contents.end(  ); )
            {
               obj_data *otmp = ( *iobj );
               ++iobj;

               if( !otmp->wear_flags.test( ITEM_TAKE )
                   || ( otmp->extra_flags.test( ITEM_PROTOTYPE ) && !ch->can_take_proto(  ) )
                   || ch->carry_number + otmp->count > ch->can_carry_n(  )
                   || ch->carry_weight + otmp->get_weight(  ) > ch->can_carry_w(  )
                   || ( source->get_real_weight(  ) + obj->get_real_weight(  ) / obj->count ) > obj->value[0] )
                  continue;

               otmp->from_obj(  );
               otmp->to_obj( obj );
               found = true;
            }
            if( found )
            {
               act( AT_ACTION, "You fill $p from $P.", ch, obj, source, TO_CHAR );
               act( AT_ACTION, "$n fills $p from $P.", ch, obj, source, TO_ROOM );
            }
            else
               ch->print( "There is nothing appropriate in there.\r\n" );
            break;
      }
      return;
   }

   if( source->value[1] < 1 )
   {
      ch->print( "There's none left!\r\n" );
      return;
   }
   if( source->count > 1 && source->item_type != ITEM_FOUNTAIN )
      source->separate(  );
   obj->separate(  );

   switch ( source->item_type )
   {
      default:
         bug( "%s: got bad item type: %d", __FUNCTION__, source->item_type );
         ch->print( "Something went wrong...\r\n" );
         return;

      case ITEM_FOUNTAIN:
         if( obj->value[1] != 0 && obj->value[2] != 0 )
         {
            ch->print( "There is already another liquid in it.\r\n" );
            return;
         }
         obj->value[2] = 0;
         obj->value[1] = obj->value[0];
         act( AT_ACTION, "You fill $p from $P.", ch, obj, source, TO_CHAR );
         act( AT_ACTION, "$n fills $p from $P.", ch, obj, source, TO_ROOM );
         return;

      case ITEM_BLOOD:
         if( obj->value[1] != 0 && obj->value[2] != 13 )
         {
            ch->print( "There is already another liquid in it.\r\n" );
            return;
         }
         obj->value[2] = 13;
         if( source->value[1] < diff )
            diff = source->value[1];
         obj->value[1] += diff;
         act( AT_ACTION, "You fill $p from $P.", ch, obj, source, TO_CHAR );
         act( AT_ACTION, "$n fills $p from $P.", ch, obj, source, TO_ROOM );
         if( ( source->value[1] -= diff ) < 1 )
            source->extract(  );
         return;

      case ITEM_HERB:
         if( obj->value[1] != 0 && obj->value[2] != source->value[2] )
         {
            ch->print( "There is already another type of herb in it.\r\n" );
            return;
         }
         obj->value[2] = source->value[2];
         if( source->value[1] < diff )
            diff = source->value[1];
         obj->value[1] += diff;
         act( AT_ACTION, "You fill $p with $P.", ch, obj, source, TO_CHAR );
         act( AT_ACTION, "$n fills $p with $P.", ch, obj, source, TO_ROOM );
         if( ( source->value[1] -= diff ) < 1 )
            source->extract(  );
         return;

      case ITEM_HERB_CON:
         if( obj->value[1] != 0 && obj->value[2] != source->value[2] )
         {
            ch->print( "There is already another type of herb in it.\r\n" );
            return;
         }
         obj->value[2] = source->value[2];
         if( source->value[1] < diff )
            diff = source->value[1];
         obj->value[1] += diff;
         source->value[1] -= diff;
         act( AT_ACTION, "You fill $p from $P.", ch, obj, source, TO_CHAR );
         act( AT_ACTION, "$n fills $p from $P.", ch, obj, source, TO_ROOM );
         return;

      case ITEM_DRINK_CON:
         if( obj->value[1] != 0 && obj->value[2] != source->value[2] )
         {
            ch->print( "There is already another liquid in it.\r\n" );
            return;
         }
         obj->value[2] = source->value[2];
         if( source->value[1] < diff )
            diff = source->value[1];
         obj->value[1] += diff;
         source->value[1] -= diff;
         act( AT_ACTION, "You fill $p from $P.", ch, obj, source, TO_CHAR );
         act( AT_ACTION, "$n fills $p from $P.", ch, obj, source, TO_ROOM );
         return;
   }
}

CMDF( do_empty )
{
   obj_data *obj;
   string arg1, arg2;

   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );
   if( !str_cmp( arg2, "into" ) && !argument.empty(  ) )
      argument = one_argument( argument, arg2 );

   if( arg1.empty(  ) )
   {
      ch->print( "Empty what?\r\n" );
      return;
   }
   if( ms_find_obj( ch ) )
      return;

   if( !( obj = ch->get_obj_carry( arg1 ) ) )
   {
      ch->print( "You aren't carrying that.\r\n" );
      return;
   }
   if( obj->count > 1 )
      obj->separate(  );

   switch ( obj->item_type )
   {
      default:
         act( AT_ACTION, "You shake $p in an attempt to empty it...", ch, obj, NULL, TO_CHAR );
         act( AT_ACTION, "$n begins to shake $p in an attempt to empty it...", ch, obj, NULL, TO_ROOM );
         return;

      case ITEM_PIPE:
         act( AT_ACTION, "You gently tap $p and empty it out.", ch, obj, NULL, TO_CHAR );
         act( AT_ACTION, "$n gently taps $p and empties it out.", ch, obj, NULL, TO_ROOM );
         REMOVE_BIT( obj->value[3], PIPE_FULLOFASH );
         REMOVE_BIT( obj->value[3], PIPE_LIT );
         obj->value[1] = 0;
         return;

      case ITEM_DRINK_CON:
         if( obj->value[1] < 1 )
         {
            ch->print( "It's already empty.\r\n" );
            return;
         }
         act( AT_ACTION, "You empty $p.", ch, obj, NULL, TO_CHAR );
         act( AT_ACTION, "$n empties $p.", ch, obj, NULL, TO_ROOM );
         obj->value[1] = 0;
         return;

      case ITEM_CONTAINER:
      case ITEM_QUIVER:
         if( IS_SET( obj->value[1], CONT_CLOSED ) )
         {
            act( AT_PLAIN, "The $d is closed.", ch, NULL, obj->name, TO_CHAR );
            return;
         }

      case ITEM_KEYRING:
         if( obj->contents.empty(  ) )
         {
            ch->print( "It's already empty.\r\n" );
            return;
         }
         if( arg2.empty(  ) )
         {
            if( ch->in_room->flags.test( ROOM_NODROP ) || ch->has_pcflag( PCFLAG_LITTERBUG ) )
            {
               ch->print( "&[magic]A magical force stops you!\r\n" );
               ch->print( "&[tell]Someone tells you, 'No littering here!'\r\n" );
               return;
            }
            if( ch->in_room->flags.test( ROOM_NODROPALL ) || ch->in_room->flags.test( ROOM_CLANSTOREROOM ) )
            {
               ch->print( "You can't seem to do that here...\r\n" );
               return;
            }
            if( obj->empty( NULL, ch->in_room ) )
            {
               act( AT_ACTION, "You empty $p.", ch, obj, NULL, TO_CHAR );
               act( AT_ACTION, "$n empties $p.", ch, obj, NULL, TO_ROOM );
               if( IS_SAVE_FLAG( SV_EMPTY ) )
                  ch->save(  );
            }
            else
               ch->print( "Hmmm... didn't work.\r\n" );
         }
         else
         {
            obj_data *dest = ch->get_obj_here( arg2 );

            if( !dest )
            {
               ch->print( "You can't find it.\r\n" );
               return;
            }
            if( dest == obj )
            {
               ch->print( "You can't empty something into itself!\r\n" );
               return;
            }
            if( dest->item_type != ITEM_CONTAINER && dest->item_type != ITEM_KEYRING && dest->item_type != ITEM_QUIVER )
            {
               ch->print( "That's not a container!\r\n" );
               return;
            }
            if( IS_SET( dest->value[1], CONT_CLOSED ) )
            {
               act( AT_PLAIN, "The $d is closed.", ch, NULL, dest->name, TO_CHAR );
               return;
            }
            dest->separate(  );
            if( obj->empty( dest, NULL ) )
            {
               act( AT_ACTION, "You empty $p into $P.", ch, obj, dest, TO_CHAR );
               act( AT_ACTION, "$n empties $p into $P.", ch, obj, dest, TO_ROOM );
               if( !dest->carried_by && IS_SAVE_FLAG( SV_EMPTY ) )
                  ch->save(  );
            }
            else
               act( AT_ACTION, "$P is too full.", ch, obj, dest, TO_CHAR );
         }
         return;
   }
}

void free_liquiddata( void )
{
   list < mixture_data * >::iterator mx;
   liquid_data *liq;
   int loopa;

   for( mx = mixlist.begin(  ); mx != mixlist.end(  ); )
   {
      mixture_data *mix = *mx;
      ++mx;

      deleteptr( mix );
   }
   for( loopa = 0; loopa <= top_liquid; ++loopa )
   {
      liq = get_liq_vnum( loopa );

      deleteptr( liq );
   }
}
