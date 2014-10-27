/****************************************************************************
 *                   ^     +----- |  / ^     ^ |     | +-\                  *
 *                  / \    |      | /  |\   /| |     | |  \                 *
 *                 /   \   +---   |<   | \ / | |     | |  |                 *
 *                /-----\  |      | \  |  v  | |     | |  /                 *
 *               /       \ |      |  \ |     | +-----+ +-/                  *
 ****************************************************************************
 * AFKMud Copyright 1997-2012 by Roger Libiez (Samson),                     *
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
 *                          Spell handling module                           *
 ****************************************************************************/

#include <sys/time.h>
#include "mud.h"
#include "area.h"
#include "clans.h"
#include "fight.h"
#include "liquids.h"
#include "mobindex.h"
#include "objindex.h"
#include "overland.h"
#include "polymorph.h"
#include "raceclass.h"
#include "roomindex.h"
#include "smaugaffect.h"

int astral_target;   /* Added for Astral Walk spell - Samson */

ch_ret ranged_attack( char_data *, string, obj_data *, obj_data *, short, short );
SPELLF( spell_null );
liquid_data *get_liq_vnum( int );
bool is_safe( char_data *, char_data * );
bool check_illegal_pk( char_data *, char_data * );
bool in_arena( char_data * );
void start_hunting( char_data *, char_data * );
void start_hating( char_data *, char_data * );
void start_timer( struct timeval * );
time_t end_timer( struct timeval * );
int recall( char_data *, int );
bool circle_follow( char_data *, char_data * );
void add_follower( char_data *, char_data * );
void stop_follower( char_data * );
morph_data *find_morph( char_data *, const string &, bool );
void raw_kill( char_data *, char_data * );
ch_ret check_room_for_traps( char_data *, int );
room_index *recall_room( char_data * );
bool beacon_check( char_data *, room_index * );
void bind_follower( char_data *, char_data *, int, int );
int IsUndead( char_data * );
int IsDragon( char_data * );

bool EqWBits( char_data * ch, int bit )
{
   list < obj_data * >::iterator iobj;

   for( iobj = ch->carrying.begin(  ); iobj != ch->carrying.end(  ); ++iobj )
   {
      obj_data *obj = *iobj;

      if( obj->wear_loc != WEAR_NONE && obj->extra_flags.test( bit ) )
         return true;
   }
   return false;
}

/*
 * Is immune to a damage type
 */
bool is_immune( char_data * ch, short damtype )
{
   switch ( damtype )
   {
      case SD_FIRE:
         return ( ch->has_immune( RIS_FIRE ) );
      case SD_COLD:
         return ( ch->has_immune( RIS_COLD ) );
      case SD_ELECTRICITY:
         return ( ch->has_immune( RIS_ELECTRICITY ) );
      case SD_ENERGY:
         return ( ch->has_immune( RIS_ENERGY ) );
      case SD_ACID:
         return ( ch->has_immune( RIS_ACID ) );
      case SD_POISON:
         return ( ch->has_immune( RIS_POISON ) );
      case SD_DRAIN:
         return ( ch->has_immune( RIS_DRAIN ) );
   }
   return false;
}

/*
 * Fancy message handling for a successful casting		-Thoric
 */
void successful_casting( skill_type * skill, char_data * ch, char_data * victim, obj_data * obj )
{
   short chitroom = ( skill->type == SKILL_SPELL ? AT_MAGIC : AT_ACTION );
   short chit = ( skill->type == SKILL_SPELL ? AT_MAGIC : AT_HIT );
   short chitme = ( skill->type == SKILL_SPELL ? AT_MAGIC : AT_HITME );

   if( skill->target != TAR_CHAR_OFFENSIVE )
   {
      chit = chitroom;
      chitme = chitroom;
   }

   if( ch && ch != victim )
   {
      if( skill->hit_char && skill->hit_char[0] != '\0' )
         act( chit, skill->hit_char, ch, obj, victim, TO_CHAR );
      else if( skill->type == SKILL_SPELL )
         act( chit, "Ok.", ch, NULL, NULL, TO_CHAR );
   }
   if( ch && skill->hit_room && skill->hit_room[0] != '\0' )
      act( chitroom, skill->hit_room, ch, obj, victim, TO_NOTVICT );
   if( ch && victim && skill->hit_vict && skill->hit_vict[0] != '\0' )
   {
      if( ch != victim )
         act( chitme, skill->hit_vict, ch, obj, victim, TO_VICT );
      else
         act( chitme, skill->hit_vict, ch, obj, victim, TO_CHAR );
   }
   else if( ch && ch == victim && skill->type == SKILL_SPELL )
      act( chitme, "Ok.", ch, NULL, NULL, TO_CHAR );
   else if( ch && ch == victim && skill->type == SKILL_SKILL )
   {
      if( skill->hit_char && ( skill->hit_char[0] != '\0' ) )
         act( chit, skill->hit_char, ch, obj, victim, TO_CHAR );
      else
         act( chit, "Ok.", ch, NULL, NULL, TO_CHAR );
   }
}

/*
 * Fancy message handling for a failed casting			-Thoric
 */
void failed_casting( skill_type * skill, char_data * ch, char_data * victim, obj_data * obj )
{
   short chitroom = ( skill->type == SKILL_SPELL ? AT_MAGIC : AT_ACTION );
   short chit = ( skill->type == SKILL_SPELL ? AT_MAGIC : AT_HIT );
   short chitme = ( skill->type == SKILL_SPELL ? AT_MAGIC : AT_HITME );

   if( skill->target != TAR_CHAR_OFFENSIVE )
   {
      chit = chitroom;
      chitme = chitroom;
   }

   if( ch && ch != victim )
   {
      if( skill->miss_char && skill->miss_char[0] != '\0' )
         act( chit, skill->miss_char, ch, obj, victim, TO_CHAR );
      else if( skill->type == SKILL_SPELL )
         act( chit, "You failed.", ch, NULL, NULL, TO_CHAR );
   }
   if( ch && skill->miss_room && skill->miss_room[0] != '\0' && str_cmp( skill->miss_room, "supress" ) )
      act( chitroom, skill->miss_room, ch, obj, victim, TO_NOTVICT );

   if( ch && victim && skill->miss_vict && skill->miss_vict[0] != '\0' )
   {
      if( ch != victim )
         act( chitme, skill->miss_vict, ch, obj, victim, TO_VICT );
      else
         act( chitme, skill->miss_vict, ch, obj, victim, TO_CHAR );
   }
   else if( ch && ch == victim )
   {
      if( skill->miss_char && skill->miss_char[0] != '\0' )
         act( chitme, skill->miss_char, ch, obj, victim, TO_CHAR );
      else if( skill->type == SKILL_SPELL )
         act( chitme, "You failed.", ch, NULL, NULL, TO_CHAR );
   }
}

/*
 * Fancy message handling for being immune to something		-Thoric
 */
void immune_casting( skill_type * skill, char_data * ch, char_data * victim, obj_data * obj )
{
   short chitroom = ( skill->type == SKILL_SPELL ? AT_MAGIC : AT_ACTION );
   short chit = ( skill->type == SKILL_SPELL ? AT_MAGIC : AT_HIT );
   short chitme = ( skill->type == SKILL_SPELL ? AT_MAGIC : AT_HITME );

   if( skill->target != TAR_CHAR_OFFENSIVE )
   {
      chit = chitroom;
      chitme = chitroom;
   }

   if( ch && ch != victim )
   {
      if( skill->imm_char && skill->imm_char[0] != '\0' )
         act( chit, skill->imm_char, ch, obj, victim, TO_CHAR );
      else if( skill->miss_char && skill->miss_char[0] != '\0' )
         act( chit, skill->miss_char, ch, obj, victim, TO_CHAR );
      else if( skill->type == SKILL_SPELL || skill->type == SKILL_SKILL )
         act( chit, "That appears to have no effect.", ch, NULL, NULL, TO_CHAR );
   }
   if( ch && skill->imm_room && skill->imm_room[0] != '\0' )
      act( chitroom, skill->imm_room, ch, obj, victim, TO_NOTVICT );
   else if( ch && skill->miss_room && skill->miss_room[0] != '\0' )
      act( chitroom, skill->miss_room, ch, obj, victim, TO_NOTVICT );
   if( ch && victim && skill->imm_vict && skill->imm_vict[0] != '\0' )
   {
      if( ch != victim )
         act( chitme, skill->imm_vict, ch, obj, victim, TO_VICT );
      else
         act( chitme, skill->imm_vict, ch, obj, victim, TO_CHAR );
   }
   else if( ch && victim && skill->miss_vict && skill->miss_vict[0] != '\0' )
   {
      if( ch != victim )
         act( chitme, skill->miss_vict, ch, obj, victim, TO_VICT );
      else
         act( chitme, skill->miss_vict, ch, obj, victim, TO_CHAR );
   }
   else if( ch && ch == victim )
   {
      if( skill->imm_char && skill->imm_char[0] != '\0' )
         act( chit, skill->imm_char, ch, obj, victim, TO_CHAR );
      else if( skill->miss_char && skill->miss_char[0] != '\0' )
         act( chit, skill->miss_char, ch, obj, victim, TO_CHAR );
      else if( skill->type == SKILL_SPELL || skill->type == SKILL_SKILL )
         act( chit, "That appears to have no affect.", ch, NULL, NULL, TO_CHAR );
   }
}

/*
 * Utter mystical words for an sn.
 */
void say_spell( char_data * ch, int sn )
{
   char buf[MSL], buf2[MSL];
   char *pName;
   int iSyl, length;
   skill_type *skill = get_skilltype( sn );

   struct syl_type
   {
      const char *old;
      const char *snew;
   };

   static const struct syl_type syl_table[] = {
      {" ", " "},
      {"ar", "abra"},
      {"au", "kada"},
      {"bless", "fido"},
      {"blind", "nose"},
      {"bur", "mosa"},
      {"cu", "judi"},
      {"de", "oculo"},
      {"en", "unso"},
      {"light", "dies"},
      {"lo", "hi"},
      {"mor", "zak"},
      {"move", "sido"},
      {"ness", "lacri"},
      {"ning", "illa"},
      {"per", "duda"},
      {"polymorph", "iaddahs"},
      {"ra", "gru"},
      {"re", "candus"},
      {"son", "sabru"},
      {"tect", "infra"},
      {"tri", "cula"},
      {"ven", "nofo"},
      {"a", "a"}, {"b", "b"}, {"c", "q"}, {"d", "e"},
      {"e", "z"}, {"f", "y"}, {"g", "o"}, {"h", "p"},
      {"i", "u"}, {"j", "y"}, {"k", "t"}, {"l", "r"},
      {"m", "w"}, {"n", "i"}, {"o", "a"}, {"p", "s"},
      {"q", "d"}, {"r", "f"}, {"s", "g"}, {"t", "h"},
      {"u", "j"}, {"v", "z"}, {"w", "x"}, {"x", "n"},
      {"y", "l"}, {"z", "k"},
      {"", ""}
   };

   buf[0] = '\0';
   for( pName = skill->name; *pName != '\0'; pName += length )
   {
      for( iSyl = 0; ( length = strlen( syl_table[iSyl].old ) ) != 0; ++iSyl )
      {
         if( !str_prefix( syl_table[iSyl].old, pName ) )
         {
            mudstrlcat( buf, syl_table[iSyl].snew, MSL );
            break;
         }
      }
      if( length == 0 )
         length = 1;
   }

   if( ch->Class == CLASS_BARD )
   {
      mudstrlcpy( buf2, "$n plays a song.", MSL );
      snprintf( buf, MSL, "$n plays the song, '%s'.", skill->name );
   }

   else
   {
      snprintf( buf2, MSL, "$n utters the words, '%s'.", buf );
      snprintf( buf, MSL, "$n utters the words, '%s'.", skill->name );
   }

   list < char_data * >::iterator ich;
   for( ich = ch->in_room->people.begin(  ); ich != ch->in_room->people.end(  ); ++ich )
   {
      char_data *rch = *ich;

      if( rch != ch )
      {
         if( is_same_char_map( ch, rch ) )
            act( AT_MAGIC, ch->Class == rch->Class ? buf : buf2, ch, NULL, rch, TO_VICT );
      }
   }
}

/*
 * Make adjustments to saving throw based in RIS - Thoric
 */
int ris_save( char_data * ch, int rchance, int ris )
{
   short modifier;

   modifier = 10;
   if( ch->has_immune( ris ) || ch->has_absorb( ris ) )
      return 1000;
   if( ch->has_resist( ris ) )
      modifier -= 2;
   if( ch->has_suscep( ris ) )
   {
      if( ch->isnpc(  ) && ch->has_immune( ris ) )
         modifier += 0;
      else
         modifier += 2;
   }
   if( modifier <= 0 )
      return 1000;
   if( modifier == 10 )
      return rchance;
   return ( rchance * modifier ) / 10;
}

/* - Thoric
 * Fancy dice expression parsing complete with order of operations,
 * simple exponent support, dice support as well as a few extra
 * variables: L = level, H = hp, M = mana, V = move, S = str, X = dex
 *            I = int, W = wis, C = con, A = cha, U = luck, A = age
 *
 * Used for spell dice parsing, ie: 3d8+L-6
 *
 */
int rd_parse( char_data * ch, int level, char *pexp )
{
   int lop = 0, gop = 0, eop = 0;
   size_t x, len = 0;
   char operation;
   char *sexp[2];
   int total = 0;

   /*
    * take care of nulls coming in 
    */
   if( !pexp || !strlen( pexp ) )
      return 0;

   /*
    * get rid of brackets if they surround the entire expresion
    */
   if( ( *pexp == '(' ) && pexp[strlen( pexp ) - 1] == ')' )
   {
      pexp[strlen( pexp ) - 1] = '\0';
      ++pexp;
   }

   /*
    * check if the expresion is just a number 
    */
   len = strlen( pexp );
   if( len == 1 && isalpha( pexp[0] ) )
   {
      switch ( pexp[0] )
      {
         default:
            return -1;
         case 'L':
         case 'l':
            return level;
         case 'H':
         case 'h':
            return ch->hit;
         case 'M':
         case 'm':
            return ch->mana;
         case 'V':
         case 'v':
            return ch->move;
         case 'S':
         case 's':
            return ch->get_curr_str(  );
         case 'I':
         case 'i':
            return ch->get_curr_int(  );
         case 'W':
         case 'w':
            return ch->get_curr_wis(  );
         case 'X':
         case 'x':
            return ch->get_curr_dex(  );
         case 'C':
         case 'c':
            return ch->get_curr_con(  );
         case 'A':
         case 'a':
            return ch->get_curr_cha(  );
         case 'U':
         case 'u':
            return ch->get_curr_lck(  );
         case 'Y':
         case 'y':
            return ch->get_age(  );
      }
   }

   for( x = 0; x < len; ++x )
      if( !isdigit( pexp[x] ) && !isspace( pexp[x] ) )
         break;
   if( x == len )
      return atoi( pexp );

   /*
    * break it into 2 parts 
    */
   for( x = 0; x < strlen( pexp ); ++x )
      switch ( pexp[x] )
      {
         default:
            break;
         case '^':
            if( !total )
               eop = x;
            break;
         case '-':
         case '+':
            if( !total )
               lop = x;
            break;
         case '*':
         case '/':
         case '%':
         case 'd':
         case 'D':
         case '<':
         case '>':
         case '{':
         case '}':
         case '=':
            if( !total )
               gop = x;
            break;
         case '(':
            ++total;
            break;
         case ')':
            --total;
            break;
      }
   if( lop )
      x = lop;
   else if( gop )
      x = gop;
   else
      x = eop;
   operation = pexp[x];
   pexp[x] = '\0';
   sexp[0] = pexp;
   sexp[1] = ( char * )( pexp + x + 1 );

   /*
    * work it out 
    */
   total = rd_parse( ch, level, sexp[0] );

   switch ( operation )
   {
      default:
         break;
      case '-':
         total -= rd_parse( ch, level, sexp[1] );
         break;
      case '+':
         total += rd_parse( ch, level, sexp[1] );
         break;
      case '*':
         total *= rd_parse( ch, level, sexp[1] );
         break;
      case '/':
         total /= rd_parse( ch, level, sexp[1] );
         break;
      case '%':
         total %= rd_parse( ch, level, sexp[1] );
         break;
      case 'd':
      case 'D':
         total = dice( total, rd_parse( ch, level, sexp[1] ) );
         break;
      case '<':
         total = ( total < rd_parse( ch, level, sexp[1] ) );
         break;
      case '>':
         total = ( total > rd_parse( ch, level, sexp[1] ) );
         break;
      case '=':
         total = ( total == rd_parse( ch, level, sexp[1] ) );
         break;
      case '{':
         total = UMIN( total, rd_parse( ch, level, sexp[1] ) );
         break;
      case '}':
         total = UMAX( total, rd_parse( ch, level, sexp[1] ) );
         break;
      case '^':
      {
         unsigned int y = rd_parse( ch, level, sexp[1] ), z = total;

         for( x = 1; x < y; ++x, z *= total );
         total = z;
         break;
      }
   }
   return total;
}

/* wrapper function so as not to destroy exp */
int dice_parse( char_data * ch, int level, const string & xexp )
{
   char buf[MIL];

   mudstrlcpy( buf, xexp.c_str(  ), MIL );
   return rd_parse( ch, level, buf );
}

/*
 * Compute a saving throw.
 * Negative apply's make saving throw better.
 */
bool saves_poison_death( int level, char_data * victim )
{
   int save;

   save = LEVEL_AVATAR + ( victim->level - level - victim->saving_poison_death ) * 5;
   save = URANGE( 5, save, 95 );
   return victim->chance( save );
}

bool saves_wands( int level, char_data * victim )
{
   int save;

   if( victim->has_immune( RIS_MAGIC ) )
      return true;

   save = LEVEL_AVATAR + ( victim->level - level - victim->saving_wand ) * 5;
   save = URANGE( 5, save, 95 );
   return victim->chance( save );
}

bool saves_para_petri( int level, char_data * victim )
{
   int save;

   save = LEVEL_AVATAR + ( victim->level - level - victim->saving_para_petri ) * 5;
   save = URANGE( 5, save, 95 );
   return victim->chance( save );
}

bool saves_breath( int level, char_data * victim )
{
   int save;

   save = LEVEL_AVATAR + ( victim->level - level - victim->saving_breath ) * 5;
   save = URANGE( 5, save, 95 );
   return victim->chance( save );
}

bool saves_spell_staff( int level, char_data * victim )
{
   int save;

   if( victim->has_immune( RIS_MAGIC ) )
      return true;

   if( victim->isnpc(  ) && level > 10 )
      level -= 5;
   save = LEVEL_AVATAR + ( victim->level - level - victim->saving_spell_staff ) * 5;
   save = URANGE( 5, save, 95 );
   return victim->chance( save );
}

/*
 * Process the spell's required components, if any		-Thoric
 * -----------------------------------------------
 * T###		check for item of type ###
 * V#####	check for item of vnum #####
 * Kword	check for item with keyword 'word'
 * G#####	check if player has ##### amount of gold
 * H####	check if player has #### amount of hitpoints
 *
 * Special operators:
 * ! spell fails if player has this
 * + don't consume this component
 * @ decrease component's value[0], and extract if it reaches 0
 * # decrease component's value[1], and extract if it reaches 0
 * $ decrease component's value[2], and extract if it reaches 0
 * % decrease component's value[3], and extract if it reaches 0
 * ^ decrease component's value[4], and extract if it reaches 0
 * & decrease component's value[5], and extract if it reaches 0
 */
bool process_spell_components( char_data * ch, int sn )
{
   skill_type *skill = get_skilltype( sn );
   char *comp = skill->components;
   char *check;
   char arg[MIL];
   bool consume, fail, found;
   int val, value;
   obj_data *obj;
   list < obj_data * >::iterator iobj;

   /*
    * if no components necessary, then everything is cool 
    */
   if( !comp || comp[0] == '\0' )
      return true;

   while( comp[0] != '\0' )
   {
      comp = one_argument( comp, arg );
      consume = true;
      fail = found = false;
      val = -1;
      switch ( arg[1] )
      {
         default:
            check = arg + 1;
            break;
         case '!':
            check = arg + 2;
            fail = true;
            break;
         case '+':
            check = arg + 2;
            consume = false;
            break;
         case '@':
            check = arg + 2;
            val = 0;
            break;
         case '#':
            check = arg + 2;
            val = 1;
            break;
         case '$':
            check = arg + 2;
            val = 2;
            break;
         case '%':
            check = arg + 2;
            val = 3;
            break;
         case '^':
            check = arg + 2;
            val = 4;
            break;
         case '&':
            check = arg + 2;
            val = 5;
            break;
            /*
             * reserve '*', '(' and ')' for v6, v7 and v8   
             */
      }
      value = atoi( check );
      obj = NULL;
      switch ( UPPER( arg[0] ) )
      {
         default:
            break;

         case 'T':
            for( iobj = ch->carrying.begin(  ); iobj != ch->carrying.end(  ); ++iobj )
            {
               obj = *iobj;

               if( obj->item_type == value )
               {
                  if( fail )
                  {
                     ch->print( "Something disrupts the casting of this spell...\r\n" );
                     return false;
                  }
                  found = true;
                  break;
               }
            }
            break;

         case 'V':
            for( iobj = ch->carrying.begin(  ); iobj != ch->carrying.end(  ); ++iobj )
            {
               obj = *iobj;

               if( obj->pIndexData->vnum == value )
               {
                  if( fail )
                  {
                     ch->print( "Something disrupts the casting of this spell...\r\n" );
                     return false;
                  }
                  found = true;
                  break;
               }
            }
            break;

         case 'K':
            for( iobj = ch->carrying.begin(  ); iobj != ch->carrying.end(  ); ++iobj )
            {
               obj = *iobj;

               if( hasname( obj->name, check ) )
               {
                  if( fail )
                  {
                     ch->print( "Something disrupts the casting of this spell...\r\n" );
                     return false;
                  }
                  found = true;
                  break;
               }
            }
            break;

         case 'G':
            if( ch->gold >= value )
            {
               if( fail )
               {
                  ch->print( "Something disrupts the casting of this spell...\r\n" );
                  return false;
               }
               else
               {
                  if( consume )
                  {
                     ch->print( "&[gold]You feel a little lighter...\r\n" );
                     ch->gold -= value;
                  }
                  continue;
               }
            }
            break;

         case 'H':
            if( ch->hit >= value )
            {
               if( fail )
               {
                  ch->print( "Something disrupts the casting of this spell...\r\n" );
                  return false;
               }
               else
               {
                  if( consume )
                  {
                     ch->print( "&[blood]You feel a little weaker...\r\n" );
                     ch->hit -= value;
                     ch->update_pos(  );
                  }
                  continue;
               }
            }
            break;
      }
      /*
       * having this component would make the spell fail... if we get
       * here, then the caster didn't have that component 
       */
      if( fail )
         continue;
      if( !found )
      {
         ch->print( "Something is missing...\r\n" );
         return false;
      }
      if( obj )
      {
         if( val >= 0 && val < 6 )
         {
            obj->separate(  );
            if( obj->value[val] <= 0 )
            {
               act( AT_MAGIC, "$p disappears in a puff of smoke!", ch, obj, NULL, TO_CHAR );
               act( AT_MAGIC, "$p disappears in a puff of smoke!", ch, obj, NULL, TO_ROOM );
               obj->extract(  );
               return false;
            }
            else if( --obj->value[val] == 0 )
            {
               act( AT_MAGIC, "$p glows briefly, then disappears in a puff of smoke!", ch, obj, NULL, TO_CHAR );
               act( AT_MAGIC, "$p glows briefly, then disappears in a puff of smoke!", ch, obj, NULL, TO_ROOM );
               obj->extract(  );
            }
            else
               act( AT_MAGIC, "$p glows briefly and a whisp of smoke rises from it.", ch, obj, NULL, TO_CHAR );
         }
         else if( consume )
         {
            obj->separate(  );
            act( AT_MAGIC, "$p glows brightly, then disappears in a puff of smoke!", ch, obj, NULL, TO_CHAR );
            act( AT_MAGIC, "$p glows brightly, then disappears in a puff of smoke!", ch, obj, NULL, TO_ROOM );
            obj->extract(  );
         }
         else
         {
            int count = obj->count;

            obj->count = 1;
            act( AT_MAGIC, "$p glows briefly.", ch, obj, NULL, TO_CHAR );
            obj->count = count;
         }
      }
   }
   return true;
}

int pAbort;

/*
 * Locate targets.
 */
/* Turn off annoying message and just abort if needed */
bool silence_locate_targets;

void *locate_targets( char_data * ch, const string & arg, int sn )
{
   char_data *victim = NULL;
   obj_data *obj = NULL;
   skill_type *skill = get_skilltype( sn );
   void *vo = NULL;

   switch ( skill->target )
   {
      default:
         bug( "%s: bad target for sn %d.", __FUNCTION__, sn );
         return &pAbort;

      case TAR_IGNORE:
         break;

      case TAR_CHAR_OFFENSIVE:
         if( arg.empty(  ) )
         {
            if( !( victim = ch->who_fighting(  ) ) )
            {
               if( !silence_locate_targets )
                  ch->print( "Cast the spell on whom?\r\n" );
               return &pAbort;
            }
         }
         else if( !( victim = ch->get_char_room( arg ) ) )
         {
            if( !silence_locate_targets )
               ch->print( "They aren't here.\r\n" );
            return &pAbort;
         }

         if( is_safe( ch, victim ) )
            return &pAbort;

         if( !ch->isnpc(  ) && !victim->isnpc(  ) && victim->CAN_PKILL(  ) && !ch->CAN_PKILL(  ) && !in_arena( ch ) && !in_arena( victim ) )
         {
            ch->print( "&[magic]The gods will not permit you to cast spells on that character.\r\n" );
            return &pAbort;
         }

         if( ch == victim )
         {
            if( SPELL_FLAG( get_skilltype( sn ), SF_NOSELF ) )
            {
               if( !silence_locate_targets )
                  ch->print( "You can't cast this on yourself!\r\n" );
               return &pAbort;
            }
            if( !silence_locate_targets )
               ch->print( "Cast this on yourself?  Okay...\r\n" );
         }

         if( !ch->isnpc(  ) )
         {
            if( !victim->isnpc(  ) )
            {
               if( ch->get_timer( TIMER_PKILLED ) > 0 )
               {
                  if( !silence_locate_targets )
                     ch->print( "You have been killed in the last 5 minutes.\r\n" );
                  return &pAbort;
               }

               if( victim->get_timer( TIMER_PKILLED ) > 0 )
               {
                  if( !silence_locate_targets )
                     ch->print( "This player has been killed in the last 5 minutes.\r\n" );
                  return &pAbort;
               }
               if( !ch->CAN_PKILL(  ) || !victim->CAN_PKILL(  ) )
               {
                  if( !silence_locate_targets )
                     ch->print( "You cannot attack another player.\r\n" );
                  return &pAbort;
               }
               if( victim != ch )
               {
                  if( !silence_locate_targets )
                     ch->print( "You really shouldn't do this to another player...\r\n" );
                  else if( victim->who_fighting(  ) != victim )
                  {
                     /*
                      * Only auto-attack those that are hitting you. 
                      */
                     return &pAbort;
                  }
               }
            }

            if( ch->has_aflag( AFF_CHARM ) && ch->master == victim )
            {
               if( !silence_locate_targets )
                  ch->print( "You can't do that on your own follower.\r\n" );
               return &pAbort;
            }
         }

         if( check_illegal_pk( ch, victim ) )
         {
            ch->print( "You cannot cast that on another player!\r\n" );
            return &pAbort;
         }
         vo = ( void * )victim;
         break;

      case TAR_CHAR_DEFENSIVE:
         if( arg.empty(  ) )
            victim = ch;
         else if( !( victim = ch->get_char_room( arg ) ) )
         {
            if( !silence_locate_targets )
               ch->print( "They aren't here.\r\n" );
            return &pAbort;
         }

         if( ch == victim && SPELL_FLAG( get_skilltype( sn ), SF_NOSELF ) )
         {
            if( !silence_locate_targets )
               ch->print( "You can't cast this on yourself!\r\n" );
            return &pAbort;
         }
         vo = ( void * )victim;
         break;

      case TAR_CHAR_SELF:
         if( !arg.empty(  ) && !hasname( ch->name, arg ) )
         {
            if( !silence_locate_targets )
               ch->print( "You cannot cast this spell on another.\r\n" );
            return &pAbort;
         }
         vo = ( void * )ch;
         break;

      case TAR_OBJ_INV:
         if( arg.empty(  ) )
         {
            if( !silence_locate_targets )
               ch->print( "What should the spell be cast upon?\r\n" );
            return &pAbort;
         }

         if( !( obj = ch->get_obj_carry( arg ) ) )
         {
            if( !silence_locate_targets )
               ch->print( "You are not carrying that.\r\n" );
            return &pAbort;
         }
         vo = ( void * )obj;
         break;
   }
   return vo;
}

/*
 * The kludgy global is for spells who want more stuff from command line.
 */
string target_name;
string ranged_target_name;

/*
 * Cast a spell.  Multi-caster and component support by Thoric
 */
CMDF( do_cast )
{
   string arg1, arg2;
   static string staticbuf;
   char_data *victim;
   void *vo = NULL;
   int mana, max = 0, sn;
   ch_ret retcode;
   bool dont_wait = false;
   skill_type *skill = NULL;
   struct timeval time_used;

   retcode = rNONE;

   switch ( ch->substate )
   {
      default:
         /*
          * no ordering charmed mobs to cast spells 
          */

         if( !ch->CAN_CAST(  ) )
         {
            ch->print( "I suppose you think you're a mage?\r\n" );
            return;
         }

         if( ch->isnpc(  ) && ch->has_aflag( AFF_CHARM ) )
         {
            ch->print( "You can't seem to do that right now...\r\n" );
            return;
         }
         if( ch->in_room->flags.test( ROOM_NO_MAGIC ) )
         {
            ch->print( "&[magic]Your magical energies were disperssed mysteriously.\r\n" );
            return;
         }

         target_name = one_argument( argument, arg1 );
         one_argument( target_name, arg2 );
         ranged_target_name = target_name;
         if( ch->morph != NULL )
         {
            if( !( ch->morph->cast_allowed ) )
            {
               ch->print( "This morph isn't permitted to cast.\r\n" );
               return;
            }
         }

         if( arg1.empty(  ) )
         {
            if( ch->Class == CLASS_BARD )
            {
               ch->print( "What do you wish to play?\r\n" );
               return;
            }

            ch->print( "Cast which what where?\r\n" );
            return;
         }

         /*
          * Regular mortal spell casting 
          */
         if( ch->get_trust(  ) < LEVEL_GOD )
         {
            if( ( sn = find_spell( ch, arg1, true ) ) < 0 || ( !ch->isnpc(  ) && ch->level < skill_table[sn]->skill_level[ch->Class] ) )
            {
               ch->print( "You can't cast that!\r\n" );
               return;
            }
            if( !( skill = get_skilltype( sn ) ) )
            {
               ch->print( "You can't do that right now...\r\n" );
               return;
            }
            if( ch->has_aflag( AFF_SILENCE ) )  /* Silence spell prevents casting */
            {
               ch->print( "&[magic]You are magically silenced, you cannot utter a sound!\r\n" );
               return;
            }
            if( ch->has_aflag( AFF_BASH ) )  /* Being bashed prevents casting too */
            {
               ch->print( "&[magic]You've been stunned by a blow to the head. Wait for it to wear off.\r\n" );
               return;
            }
         }
         else
         {
            /*
             * Godly "spell builder" spell casting with debugging messages
             */
            if( ( sn = skill_lookup( arg1 ) ) < 0 )
            {
               ch->print( "We didn't create that yet...\r\n" );
               return;
            }
            if( sn >= MAX_SKILL )
            {
               ch->print( "Hmm... that might hurt.\r\n" );
               return;
            }
            if( !( skill = get_skilltype( sn ) ) )
            {
               ch->print( "Something is severely wrong with that one...\r\n" );
               return;
            }
            if( skill->type != SKILL_SPELL )
            {
               ch->print( "That isn't a spell.\r\n" );
               return;
            }
            if( !skill->spell_fun )
            {
               ch->print( "We didn't finish that one yet...\r\n" );
               return;
            }
         }

         /*
          * Something else removed by Merc - Thoric
          */
         /*
          * Band-aid alert!  !isnpc check -- Blod 
          */
         /*
          * Removed !isnpc check -- Tarl 
          */
         if( ch->position < skill->minimum_position )
         {
            switch ( ch->position )
            {
               default:
                  ch->print( "You can't concentrate enough.\r\n" );
                  break;

               case POS_SITTING:
                  ch->print( "You can't summon enough energy sitting down.\r\n" );
                  break;

               case POS_RESTING:
                  ch->print( "You're too relaxed to cast that spell.\r\n" );
                  break;

               case POS_FIGHTING:
                  if( skill->minimum_position <= POS_EVASIVE )
                     ch->print( "This fighting style is too demanding for that!\r\n" );
                  else
                     ch->print( "No way!  You are still fighting!\r\n" );
                  break;

               case POS_DEFENSIVE:
                  if( skill->minimum_position <= POS_EVASIVE )
                     ch->print( "This fighting style is too demanding for that!\r\n" );
                  else
                     ch->print( "No way!  You are still fighting!\r\n" );
                  break;

               case POS_AGGRESSIVE:
                  if( skill->minimum_position <= POS_EVASIVE )
                     ch->print( "This fighting style is too demanding for that!\r\n" );
                  else
                     ch->print( "No way!  You are still fighting!\r\n" );
                  break;

               case POS_BERSERK:
                  if( skill->minimum_position <= POS_EVASIVE )
                     ch->print( "This fighting style is too demanding for that!\r\n" );
                  else
                     ch->print( "No way!  You are still fighting!\r\n" );
                  break;

               case POS_EVASIVE:
                  ch->print( "No way!  You are still fighting!\r\n" );
                  break;

               case POS_SLEEPING:
                  ch->print( "You dream about great feats of magic.\r\n" );
                  break;
            }
            return;
         }

         if( skill->spell_fun == spell_null )
         {
            ch->print( "That's not a spell!\r\n" );
            return;
         }

         if( !skill->spell_fun )
         {
            ch->print( "You cannot cast that... yet.\r\n" );
            return;
         }

         if( !ch->isnpc(  ) && !ch->is_immortal(  ) && skill->guild != CLASS_NONE && ( !ch->pcdata->clan || skill->guild != ch->pcdata->clan->Class ) )
         {
            ch->print( "That is only available to members of a certain guild.\r\n" );
            return;
         }

         /*
          * Mystaric, 980908 - Added checks for spell sector type 
          */
         if( !ch->in_room || ( skill->spell_sector && !IS_SET( skill->spell_sector, ( 1 << ch->in_room->sector_type ) ) ) )
         {
            ch->print( "You can not cast that here.\r\n" );
            return;
         }

         mana = ch->isnpc(  )? 0 : UMAX( skill->min_mana, 100 / ( 2 + ch->level - skill->skill_level[ch->Class] ) );

         /*
          * Locate targets.
          */
         vo = locate_targets( ch, arg2, sn );
         if( vo == &pAbort )
            return;

         if( !ch->isnpc(  ) && ch->mana < mana )
         {
            ch->print( "You don't have enough mana.\r\n" );
            return;
         }

         if( skill->participants <= 1 )
            break;

         /*
          * multi-participant spells         -Thoric 
          */
         ch->add_timer( TIMER_DO_FUN, UMIN( skill->beats / 10, 3 ), do_cast, 1 );
         act( AT_MAGIC, "You begin to chant...", ch, NULL, NULL, TO_CHAR );
         act( AT_MAGIC, "$n begins to chant...", ch, NULL, NULL, TO_ROOM );
         strdup_printf( &ch->alloc_ptr, "%s %s", arg2.c_str(  ), target_name.c_str(  ) );
         ch->tempnum = sn;
         return;

      case SUB_TIMER_DO_ABORT:
         DISPOSE( ch->alloc_ptr );
         if( IS_VALID_SN( ( sn = ch->tempnum ) ) )
         {
            if( !( skill = get_skilltype( sn ) ) )
            {
               ch->print( "Something went wrong...\r\n" );
               bug( "%s: SUB_TIMER_DO_ABORT: bad sn %d", __FUNCTION__, sn );
               return;
            }
            mana = ch->isnpc(  )? 0 : UMAX( skill->min_mana, 100 / ( 2 + ch->level - skill->skill_level[ch->Class] ) );
            if( !ch->is_immortal(  ) ) /* so imms dont lose mana */
               ch->mana -= mana / 3;
         }
         ch->set_color( AT_MAGIC );
         ch->print( "You stop chanting...\r\n" );
         /*
          * should add chance of backfire here 
          */
         return;

      case 1:
         sn = ch->tempnum;
         if( !( skill = get_skilltype( sn ) ) )
         {
            ch->print( "Something went wrong...\r\n" );
            bug( "%s: substate 1: bad sn %d", __FUNCTION__, sn );
            return;
         }
         if( !ch->alloc_ptr || !IS_VALID_SN( sn ) || skill->type != SKILL_SPELL )
         {
            ch->print( "Something cancels out the spell!\r\n" );
            bug( "%s: ch->alloc_ptr NULL or bad sn (%d)", __FUNCTION__, sn );
            return;
         }
         mana = ch->isnpc(  )? 0 : UMAX( skill->min_mana, 100 / ( 2 + ch->level - skill->skill_level[ch->Class] ) );
         staticbuf = ch->alloc_ptr;
         target_name = one_argument( staticbuf, arg2 );
         DISPOSE( ch->alloc_ptr );
         ch->substate = SUB_NONE;
         if( skill->participants > 1 )
         {
            int cnt = 1;
            char_data *tmp;
            list < char_data * >::iterator ich;
            timer_data *t = NULL;

            for( ich = ch->in_room->people.begin(  ); ich != ch->in_room->people.end(  ); ++ich )
            {
               tmp = *ich;

               if( tmp != ch && ( t = tmp->get_timerptr( TIMER_DO_FUN ) ) != NULL
                   && t->count >= 1 && t->do_fun == do_cast && tmp->tempnum == sn && tmp->alloc_ptr && !str_cmp( tmp->alloc_ptr, staticbuf ) )
                  ++cnt;
            }
            if( cnt >= skill->participants )
            {
               for( ich = ch->in_room->people.begin(  ); ich != ch->in_room->people.end(  ); ++ich )
               {
                  tmp = *ich;

                  if( tmp != ch && ( t = tmp->get_timerptr( TIMER_DO_FUN ) ) != NULL
                      && t->count >= 1 && t->do_fun == do_cast && tmp->tempnum == sn && tmp->alloc_ptr && !str_cmp( tmp->alloc_ptr, staticbuf ) )
                  {
                     tmp->extract_timer( t );
                     act( AT_MAGIC, "Channeling your energy into $n, you help cast the spell!", ch, NULL, tmp, TO_VICT );
                     act( AT_MAGIC, "$N channels $S energy into you!", ch, NULL, tmp, TO_CHAR );
                     act( AT_MAGIC, "$N channels $S energy into $n!", ch, NULL, tmp, TO_NOTVICT );
                     tmp->mana -= mana;
                     tmp->substate = SUB_NONE;
                     tmp->tempnum = -1;
                     DISPOSE( tmp->alloc_ptr );
                  }
               }
               dont_wait = true;
               ch->print( "You concentrate all the energy into a burst of mystical words!\r\n" );
               vo = locate_targets( ch, arg2, sn );
               if( vo == &pAbort )
                  return;
            }
            else
            {
               ch->print( "&[magic]There was not enough power for the spell to succeed...\r\n" );

               if( !ch->is_immortal(  ) ) /* so imms dont lose mana */
                  ch->mana -= mana / 2;
               ch->learn_from_failure( sn );
               return;
            }
         }
   }

   /*
    * uttering those magic words unless casting "ventriloquate" 
    */
   if( str_cmp( skill->name, "ventriloquate" ) )
      say_spell( ch, sn );

   if( !dont_wait )
      ch->WAIT_STATE( skill->beats );

   /*
    * Getting ready to cast... check for spell components   -Thoric
    */
   if( !process_spell_components( ch, sn ) )
   {
      if( !ch->is_immortal(  ) ) /* so imms dont lose mana */
         ch->mana -= mana / 2;
      ch->learn_from_failure( sn );
      return;
   }

   if( ch->is_immortal(  ) || ch->isnpc(  ) )
      max = 1;
   else
   {
      max = ch->spellfail + ( skill->difficulty * 5 ) + ( ch->pcdata->condition[COND_DRUNK] * 2 );
      switch ( ch->Class )
      {
         default:
            break;

         case CLASS_MAGE:
            if( EqWBits( ch, ITEM_ANTI_MAGE ) )
               max += 10;
            break;

         case CLASS_CLERIC:
            if( EqWBits( ch, ITEM_ANTI_CLERIC ) )
               max += 10;
            break;

         case CLASS_DRUID:
            if( EqWBits( ch, ITEM_ANTI_DRUID ) )
               max += 10;
            break;

         case CLASS_PALADIN:
            if( EqWBits( ch, ITEM_ANTI_PALADIN ) )
               max += 10;
            break;

         case CLASS_ANTIPALADIN:
            if( EqWBits( ch, ITEM_ANTI_APAL ) )
               max += 10;
            break;

         case CLASS_BARD:
            if( EqWBits( ch, ITEM_ANTI_BARD ) )
               max += 10;
            break;

         case CLASS_RANGER:
            if( EqWBits( ch, ITEM_ANTI_RANGER ) )
               max += 10;
            break;

         case CLASS_NECROMANCER:
            if( EqWBits( ch, ITEM_ANTI_NECRO ) )
               max += 10;
            break;
      }
   }

   /*
    * This had better work.... 
    */
   victim = ( char_data * ) vo;

   if( ch->pcdata && number_range( 1, max ) > ch->pcdata->learned[sn] )
   {
      /*
       * Some more interesting loss of concentration messages  -Thoric 
       */
      switch ( number_bits( 2 ) )
      {
         default:
         case 0: /* too busy */
            if( ch->fighting )
               ch->print( "This round of battle is too hectic to concentrate properly.\r\n" );
            else
               ch->print( "You lost your concentration.\r\n" );
            break;

         case 1: /* irritation */
            if( number_bits( 2 ) == 0 )
            {
               switch ( number_bits( 2 ) )
               {
                  default:
                  case 0:
                     ch->print( "A tickle in your nose prevents you from keeping your concentration.\r\n" );
                     break;
                  case 1:
                     ch->print( "An itch on your leg keeps you from properly casting your spell.\r\n" );
                     break;
                  case 2:
                     ch->print( "Something in your throat prevents you from uttering the proper phrase.\r\n" );
                     break;
                  case 3:
                     ch->print( "A twitch in your eye disrupts your concentration for a moment.\r\n" );
                     break;
               }
            }
            else
               ch->print( "Something distracts you, and you lose your concentration.\r\n" );
            break;

         case 2: /* not enough time */
            if( ch->fighting )
               ch->print( "There wasn't enough time this round to complete the casting.\r\n" );
            else
               ch->print( "You lost your concentration.\r\n" );
            break;

         case 3:
            ch->print( "You get a mental block mid-way through the casting.\r\n" );
            break;
      }
      if( !ch->is_immortal(  ) ) /* so imms dont lose mana */
         ch->mana -= mana / 2;
      ch->learn_from_failure( sn );
      return;
   }
   else
   {
      ch->mana -= mana;

      /*
       * check for immunity to magic if victim is known...
       * and it is a TAR_CHAR_DEFENSIVE/SELF spell
       * otherwise spells will have to check themselves
       */
      if( ( ( skill->target == TAR_CHAR_DEFENSIVE || skill->target == TAR_CHAR_SELF ) && victim && victim->has_immune( RIS_MAGIC ) ) )
      {
         immune_casting( skill, ch, victim, NULL );
         retcode = rSPELL_FAILED;
      }
      else
      {
         start_timer( &time_used );
         retcode = ( *skill->spell_fun ) ( sn, ch->level, ch, vo );
         end_timer( &time_used );
      }
   }

   if( retcode == rCHAR_DIED || retcode == rERROR || ch->char_died(  ) )
      return;

   /*
    * learning 
    */
   if( retcode == rSPELL_FAILED )
      ch->learn_from_failure( sn );

   /*
    * favor adjustments 
    */
   if( victim && victim != ch && !victim->isnpc(  ) && skill->target == TAR_CHAR_DEFENSIVE )
      ch->adjust_favor( 7, 1 );

   if( victim && victim != ch && !ch->isnpc(  ) && skill->target == TAR_CHAR_DEFENSIVE )
      victim->adjust_favor( 13, 1 );

   if( victim && victim != ch && !ch->isnpc(  ) && skill->target == TAR_CHAR_OFFENSIVE )
      ch->adjust_favor( 4, 1 );

   /*
    * Fixed up a weird mess here, and added double safeguards  -Thoric
    */
   if( skill->target == TAR_CHAR_OFFENSIVE && victim && !victim->char_died(  ) && victim != ch )
   {
      list < char_data * >::iterator ich;

      for( ich = ch->in_room->people.begin(  ); ich != ch->in_room->people.end(  ); )
      {
         char_data *vch = *ich;
         ++ich;

         if( vch == victim )
         {
            if( vch->master != ch && !vch->fighting )
               retcode = multi_hit( vch, ch, TYPE_UNDEFINED );
            break;
         }
      }
   }
}

/* Wrapper function for Bards - Samson 10-26-98 */
CMDF( do_play )
{
   if( ch->Class != CLASS_BARD )
   {
      ch->print( "Only a bard can play songs!\r\n" );
      return;
   }

   bool found = false;
   list < obj_data * >::iterator iobj;
   for( iobj = ch->carrying.begin(  ); iobj != ch->carrying.end(  ); ++iobj )
   {
      obj_data *obj = *iobj;

      if( obj->item_type == ITEM_INSTRUMENT && obj->wear_loc == WEAR_HOLD )
      {
         found = true;
         break;
      }
   }

   if( !found )
   {
      ch->print( "You are not holding an instrument!\r\n" );
      return;
   }

   do_cast( ch, argument );
}

/*
 * Cast spells at targets using a magical object.
 */
ch_ret obj_cast_spell( int sn, int level, char_data * ch, char_data * victim, obj_data * obj )
{
   void *vo;
   ch_ret retcode = rNONE;
   int levdiff = ch->level - level;
   skill_type *skill = get_skilltype( sn );
   struct timeval time_used;

   if( sn == -1 )
      return retcode;

   if( !skill || !skill->spell_fun )
   {
      bug( "%s: bad sn %d.", __FUNCTION__, sn );
      return rERROR;
   }

   if( ch->in_room->flags.test( ROOM_NO_MAGIC ) )
   {
      ch->print( "&[magic]The magic from the spell is dispersed...\r\n" );
      return rNONE;
   }

   if( ch->in_room->flags.test( ROOM_SAFE ) && skill->target == TAR_CHAR_OFFENSIVE )
   {
      ch->print( "&[magic]This is a safe room...\r\n" );
      return rNONE;
   }

   /*
    * Basically this was added to cut down on level 5 players using level
    * 40 scrolls in battle too often ;)      -Thoric
    */
   if( ( skill->target == TAR_CHAR_OFFENSIVE || number_bits( 7 ) == 1 ) /* 1/128 chance if non-offensive */
       && skill->type != SKILL_HERB && !ch->chance( 95 + levdiff ) )
   {
      switch ( number_bits( 2 ) )
      {
         default:
         case 0:
            failed_casting( skill, ch, victim, NULL );
            break;
         case 1:
            act( AT_MAGIC, "The $t spell backfires!", ch, skill->name, victim, TO_CHAR );
            if( victim )
               act( AT_MAGIC, "$n's $t spell backfires!", ch, skill->name, victim, TO_VICT );
            act( AT_MAGIC, "$n's $t spell backfires!", ch, skill->name, victim, TO_NOTVICT );
            return damage( ch, ch, number_range( 1, level ), TYPE_UNDEFINED );
         case 2:
            failed_casting( skill, ch, victim, NULL );
            break;
         case 3:
            act( AT_MAGIC, "The $t spell backfires!", ch, skill->name, victim, TO_CHAR );
            if( victim )
               act( AT_MAGIC, "$n's $t spell backfires!", ch, skill->name, victim, TO_VICT );
            act( AT_MAGIC, "$n's $t spell backfires!", ch, skill->name, victim, TO_NOTVICT );
            return damage( ch, ch, number_range( 1, level ), TYPE_UNDEFINED );
      }
      return rNONE;
   }

   target_name.clear(  );
   switch ( skill->target )
   {
      default:
         bug( "%s: bad target for sn %d.", __FUNCTION__, sn );
         return rERROR;

      case TAR_IGNORE:
         vo = NULL;
         if( victim )
            target_name = victim->name;
         else if( obj )
            target_name = obj->name;
         break;

      case TAR_CHAR_OFFENSIVE:
         if( victim != ch )
         {
            if( !victim )
               victim = ch->who_fighting(  );
            if( !victim || ( !victim->isnpc(  ) && !in_arena( victim ) ) )
            {
               ch->print( "You can't do that.\r\n" );
               return rNONE;
            }
         }
         if( ch != victim && is_safe( ch, victim ) )
            return rNONE;
         vo = ( void * )victim;
         break;

      case TAR_CHAR_DEFENSIVE:
         if( victim == NULL )
            victim = ch;
         vo = ( void * )victim;
         if( skill->type != SKILL_HERB && victim->has_immune( RIS_MAGIC ) )
         {
            immune_casting( skill, ch, victim, NULL );
            return rNONE;
         }
         break;

      case TAR_CHAR_SELF:
         vo = ( void * )ch;
         if( skill->type != SKILL_HERB && ch->has_immune( RIS_MAGIC ) )
         {
            immune_casting( skill, ch, victim, NULL );
            return rNONE;
         }
         break;

      case TAR_OBJ_INV:
         if( obj == NULL )
         {
            ch->print( "You can't do that.\r\n" );
            return rNONE;
         }
         vo = ( void * )obj;
         break;
   }
   start_timer( &time_used );
   retcode = ( *skill->spell_fun ) ( sn, level, ch, vo );
   end_timer( &time_used );

   if( retcode == rSPELL_FAILED )
      retcode = rNONE;

   if( retcode == rCHAR_DIED || retcode == rERROR )
      return retcode;

   if( ch->char_died(  ) )
      return rCHAR_DIED;

   if( skill->target == TAR_CHAR_OFFENSIVE && victim != ch && !victim->char_died(  ) )
   {
      list < char_data * >::iterator ich;

      for( ich = ch->in_room->people.begin(  ); ich != ch->in_room->people.end(  ); )
      {
         char_data *vch = *ich;
         ++ich;

         if( victim == vch && !vch->fighting && vch->master != ch )
         {
            retcode = multi_hit( vch, ch, TYPE_UNDEFINED );
            break;
         }
      }
   }
   return retcode;
}

/*
 * Spell functions.
 */

/* Stock spells start here */

/* Do not delete - used by the liquidate skill! */
SPELLF( spell_midas_touch )
{
   int val;
   obj_data *obj = ( obj_data * ) vo;

   if( obj->extra_flags.test( ITEM_NODROP ) || obj->extra_flags.test( ITEM_SINDHAE ) )
   {
      ch->print( "You can't seem to let go of it.\r\n" );
      return rSPELL_FAILED;
   }

   if( obj->extra_flags.test( ITEM_PROTOTYPE ) && ch->get_trust(  ) < LEVEL_IMMORTAL )
      /*
       * was victim instead of ch!  Thanks Nick Gammon 
       */
   {
      ch->print( "That item is not for mortal hands to touch!\r\n" );
      return rSPELL_FAILED;   /* Thoric */
   }

   if( obj->ego >= sysdata->minego )
   {
      ch->print( "Rare items cannot be liquidated in this manner!\r\n" );
      return rNONE;
   }

   if( !obj->wear_flags.test( ITEM_TAKE ) || ( obj->item_type == ITEM_CORPSE_NPC ) || ( obj->item_type == ITEM_CORPSE_PC ) )
   {
      ch->print( "You cannot seem to turn this item to gold!\r\n" );
      return rNONE;
   }

   obj->separate(  );   /* nice, alty :) */

   val = obj->cost / 2;
   val = UMAX( 0, val );
   ch->gold += val;

   if( obj->extracted(  ) )
      return rNONE;

   obj->extract(  );

   ch->print( "You transmogrify the item to gold!\r\n" );
   return rNONE;
}

SPELLF( spell_cure_poison )
{
   char_data *victim = ( char_data * ) vo;
   skill_type *skill = get_skilltype( sn );

   if( victim->has_immune( RIS_MAGIC ) )
   {
      immune_casting( skill, ch, victim, NULL );
      return rSPELL_FAILED;
   }

   if( victim->is_affected( gsn_poison ) )
   {
      victim->affect_strip( gsn_poison );
      victim->set_color( AT_MAGIC );
      victim->print( "A warm feeling runs through your body.\r\n" );
      victim->mental_state = URANGE( -100, victim->mental_state, -10 );
      if( ch != victim )
      {
         act( AT_MAGIC, "A flush of health washes over $N.", ch, NULL, victim, TO_NOTVICT );
         act( AT_MAGIC, "You lift the poison from $N's body.", ch, NULL, victim, TO_CHAR );
      }
      return rNONE;
   }
   else
   {
      ch->set_color( AT_MAGIC );
      if( ch != victim )
         ch->print( "You work your cure, but it has no apparent effect.\r\n" );
      else
         ch->print( "You don't seem to be poisoned.\r\n" );
      return rSPELL_FAILED;
   }
}

SPELLF( spell_cure_blindness )
{
   char_data *victim = ( char_data * ) vo;
   skill_type *skill = get_skilltype( sn );

   ch->set_color( AT_MAGIC );
   if( victim->has_immune( RIS_MAGIC ) )
   {
      immune_casting( skill, ch, victim, NULL );
      return rSPELL_FAILED;
   }

   if( !victim->is_affected( gsn_blindness ) )
   {
      if( ch != victim )
         ch->print( "You work your cure, but it has no apparent effect.\r\n" );
      else
         ch->print( "You don't seem to be blind.\r\n" );
      return rSPELL_FAILED;
   }
   victim->affect_strip( gsn_blindness );
   victim->set_color( AT_MAGIC );
   victim->print( "Your vision returns!\r\n" );
   if( ch != victim )
      ch->print( "You work your cure, restoring vision.\r\n" );
   return rNONE;
}

/* Must keep */
SPELLF( spell_call_lightning )
{
   if( ( !ch->IS_OUTSIDE(  ) || INDOOR_SECTOR( ch->in_room->sector_type ) ) && !ch->has_pcflag( PCFLAG_ONMAP ) && !ch->has_actflag( ACT_ONMAP ) )
   {
      ch->print( "You must be outdoors to cast this spell.\r\n" );
      return rSPELL_FAILED;
   }

   if( ch->in_room->area->weather->precip <= 0 )
   {
      ch->print( "You need bad weather.\r\n" );
      return rSPELL_FAILED;
   }

   int dam = dice( level, 6 );

   ch->set_color( AT_MAGIC );
   ch->print( "God's lightning strikes your foes!\r\n" );
   act( AT_MAGIC, "$n calls God's lightning to strike $s foes!", ch, NULL, NULL, TO_ROOM );

   bool ch_died = false;
   list < char_data * >::iterator ich;
   for( ich = ch->in_room->people.begin(  ); ich != ch->in_room->people.end(  ); )
   {
      ch_ret retcode = rNONE;
      char_data *vch = *ich;
      ++ich;

      if( vch->has_pcflag( PCFLAG_WIZINVIS ) && vch->pcdata->wizinvis >= LEVEL_IMMORTAL )
         continue;

      if( !is_same_char_map( ch, vch ) )
         continue;

      if( vch != ch && ( ch->isnpc(  )? !vch->isnpc(  ) : vch->isnpc(  ) ) )
         retcode = damage( ch, vch, saves_spell_staff( level, vch ) ? dam / 2 : dam, sn );
      if( retcode == rCHAR_DIED || ch->char_died(  ) )
      {
         ch_died = true;
         continue;
      }

      if( !ch_died && vch->IS_OUTSIDE(  ) && vch->IS_AWAKE(  ) )
      {
         if( number_bits( 3 ) == 0 )
            vch->print( "&BLightning flashes in the sky.\r\n" );
      }
   }
   if( ch_died )
      return rCHAR_DIED;
   else
      return rNONE;
}

/* Do not remove */
SPELLF( spell_change_sex )
{
   char_data *victim = ( char_data * ) vo;
   affect_data af;
   skill_type *skill = get_skilltype( sn );

   if( victim->has_immune( RIS_MAGIC ) )
   {
      immune_casting( skill, ch, victim, NULL );
      return rSPELL_FAILED;
   }
   if( victim->is_affected( sn ) )
   {
      failed_casting( skill, ch, victim, NULL );
      return rSPELL_FAILED;
   }
   af.type = sn;
   af.duration = ( int )( 10 * level * DUR_CONV );
   af.location = APPLY_SEX;
   do
   {
      af.modifier = number_range( 0, SEX_MAX ) - victim->sex;
   }
   while( af.modifier == 0 );
   af.bit = 0;
   victim->affect_to_char( &af );
   successful_casting( skill, ch, victim, NULL );
   return rNONE;
}

bool char_data::can_charm(  )
{
   if( isnpc(  ) || is_immortal(  ) )
      return true;
   if( ( ( get_curr_cha(  ) / 5 ) + 1 ) > pcdata->charmies )
      return true;
   return false;
}

SPELLF( spell_charm_person )
{
   char_data *victim = ( char_data * ) vo;
   int schance;
   skill_type *skill = get_skilltype( sn );

   if( victim == ch )
   {
      ch->print( "You like yourself even better!\r\n" );
      return rSPELL_FAILED;
   }

   if( victim->has_immune( RIS_MAGIC ) || victim->has_immune( RIS_CHARM ) )
   {
      immune_casting( skill, ch, victim, NULL );
      return rSPELL_FAILED;
   }

   if( !victim->isnpc(  ) && !ch->isnpc(  ) )
   {
      ch->print( "I don't think so...\r\n" );
      victim->print( "You feel charmed...\r\n" );
      return rSPELL_FAILED;
   }

   schance = ris_save( victim, level, RIS_CHARM );

   if( victim->has_aflag( AFF_CHARM ) || schance == 1000 || ch->has_aflag( AFF_CHARM )
       || level < victim->level || circle_follow( victim, ch ) || !ch->can_charm(  ) || saves_spell_staff( schance, victim ) )
   {
      failed_casting( skill, ch, victim, NULL );
      return rSPELL_FAILED;
   }

   if( victim->master )
      stop_follower( victim );

   bind_follower( victim, ch, sn, ( int )( number_fuzzy( ( int )( ( level + 1 ) / 5 ) + 1 ) * DUR_CONV ) );
   successful_casting( skill, ch, victim, NULL );

   if( victim->isnpc(  ) )
   {
      start_hating( victim, ch );
      start_hunting( victim, ch );
   }
   return rNONE;
}

SPELLF( spell_control_weather )
{
   skill_type *skill = get_skilltype( sn );
   weather_data *weath;
   int change;
   weath = ch->in_room->area->weather;

   change = number_range( -rand_factor, rand_factor ) + ( ch->level * 3 ) / ( 2 * max_vector );

   if( !str_cmp( target_name, "warmer" ) )
      weath->temp_vector += change;
   else if( !str_cmp( target_name, "colder" ) )
      weath->temp_vector -= change;
   else if( !str_cmp( target_name, "wetter" ) )
      weath->precip_vector += change;
   else if( !str_cmp( target_name, "drier" ) )
      weath->precip_vector -= change;
   else if( !str_cmp( target_name, "windier" ) )
      weath->wind_vector += change;
   else if( !str_cmp( target_name, "calmer" ) )
      weath->wind_vector -= change;
   else
   {
      ch->print( "Do you want it to get warmer, colder, wetter, drier, windier, or calmer?\r\n" );
      return rSPELL_FAILED;
   }

   weath->temp_vector = URANGE( -max_vector, weath->temp_vector, max_vector );
   weath->precip_vector = URANGE( -max_vector, weath->precip_vector, max_vector );
   weath->wind_vector = URANGE( -max_vector, weath->wind_vector, max_vector );

   weather_update(  );
   successful_casting( skill, ch, NULL, NULL );
   return rNONE;
}

SPELLF( spell_create_water )
{
   obj_data *obj = ( obj_data * ) vo;
   liquid_data *liq = NULL;
   weather_data *weath;
   int water;

   if( obj->item_type != ITEM_DRINK_CON )
   {
      ch->print( "It is unable to hold water.\r\n" );
      return rSPELL_FAILED;
   }

   if( !( liq = get_liq_vnum( obj->value[2] ) ) )
   {
      bug( "%s: bad liquid number %d", __FUNCTION__, obj->value[2] );
      ch->print( "There's already another liquid in the container.\r\n" );
      return rSPELL_FAILED;
   }

   if( str_cmp( liq->name, "water" ) && obj->value[1] != 0 )
   {
      ch->print( "There's already another liquid in the container.\r\n" );
      return rSPELL_FAILED;
   }

   weath = ch->in_room->area->weather;

   water = UMIN( level * ( weath->precip >= 0 ? 4 : 2 ), obj->value[0] - obj->value[1] );

   if( water > 0 )
   {
      obj->separate(  );
      obj->value[2] = liq->vnum;
      obj->value[1] += water;

      /*
       * Don't overfill the container! 
       */
      if( obj->value[1] > obj->value[0] )
         obj->value[1] = obj->value[0];

      if( !hasname( obj->name, "water" ) )
         stralloc_printf( &obj->name, "%s water", obj->name );

      act( AT_MAGIC, "$p is filled.", ch, obj, NULL, TO_CHAR );
   }
   return rNONE;
}

SPELLF( spell_detect_poison )
{
   obj_data *obj = ( obj_data * ) vo;

   ch->set_color( AT_MAGIC );
   if( obj->item_type == ITEM_DRINK_CON || obj->item_type == ITEM_FOOD || obj->item_type == ITEM_COOK )
   {
      if( obj->item_type == ITEM_COOK && obj->value[2] == 0 )
         ch->print( "It looks undercooked.\r\n" );
      else if( obj->value[3] != 0 )
         ch->print( "You smell poisonous fumes.\r\n" );
      else
         ch->print( "It looks very delicious.\r\n" );
   }
   else
      ch->print( "It doesn't look poisoned.\r\n" );
   return rNONE;
}

/* MUST BE KEPT!!!! Used by defense types for mobiles!!! */
SPELLF( spell_dispel_evil )
{
   char_data *victim = ( char_data * ) vo;
   int dam;
   skill_type *skill = get_skilltype( sn );

   if( !ch->isnpc(  ) && ch->IS_EVIL(  ) )
      victim = ch;

   if( victim->IS_GOOD(  ) )
   {
      act( AT_MAGIC, "The Gods protect $N.", ch, NULL, victim, TO_ROOM );
      return rSPELL_FAILED;
   }

   if( victim->IS_NEUTRAL(  ) )
   {
      act( AT_MAGIC, "$N does not seem to be affected.", ch, NULL, victim, TO_CHAR );
      return rSPELL_FAILED;
   }

   if( victim->has_immune( RIS_MAGIC ) )
   {
      immune_casting( skill, ch, victim, NULL );
      return rSPELL_FAILED;
   }

   dam = dice( level, 8 );

   if( ch->Class == CLASS_PALADIN )
      dam = dice( level, 6 );

   if( saves_spell_staff( level, victim ) )
      return rSPELL_FAILED;
   return damage( ch, victim, dam, sn );
}

/* Redone by Samson. In a desparate attempt to get this thing to behave
 * properly and such. It's MUCH more of a pain in the ass than it might seem.
 */
SPELLF( spell_dispel_magic )
{
   char_data *victim = ( char_data * ) vo;
   skill_type *skill = NULL;

   ch->set_color( AT_MAGIC );

   if( victim != ch )
   {  /* Edited by Tarl 27 Mar 02 to correct */
      if( saves_spell_staff( level, victim ) )
      {
         ch->print( "&[magic]You weave arcane gestures, but the spell does nothing.\r\n" );
         return rSPELL_FAILED;
      }
   }

   /*
    * Remove ALL affects generated by spells, and kill the AFF_X bit for it as well 
    */
   list < affect_data * >::iterator paf;
   for( paf = victim->affects.begin(  ); paf != victim->affects.end(  ); )
   {
      affect_data *aff = ( *paf );
      ++paf;

      if( ( skill = get_skilltype( aff->type ) ) != NULL )
      {
         if( skill->type == SKILL_SPELL )
         {
            if( skill->msg_off )
               victim->printf( "%s\r\n", skill->msg_off );
            victim->unset_aflag( aff->bit );
            victim->affect_remove( aff );
         }
      }
   }

   /*
    * Now we get to do the hard part - step thru and look for things to dispel when it's not set by a spell. 
    */
   if( victim->has_aflag( AFF_SANCTUARY ) )
      victim->unset_aflag( AFF_SANCTUARY );

   if( victim->has_aflag( AFF_FAERIE_FIRE ) )
      victim->unset_aflag( AFF_FAERIE_FIRE );

   if( victim->has_aflag( AFF_CURSE ) )
      victim->unset_aflag( AFF_CURSE );

   if( victim->has_aflag( AFF_PROTECT ) )
      victim->unset_aflag( AFF_PROTECT );

   if( victim->has_aflag( AFF_SLEEP ) )
      victim->unset_aflag( AFF_SLEEP );

   if( victim->has_aflag( AFF_CHARM ) )
      victim->unset_aflag( AFF_CHARM );

   if( victim->has_aflag( AFF_ACIDMIST ) )
      victim->unset_aflag( AFF_ACIDMIST );

   if( victim->has_aflag( AFF_TRUESIGHT ) )
      victim->unset_aflag( AFF_TRUESIGHT );

   if( victim->has_aflag( AFF_FIRESHIELD ) )
      victim->unset_aflag( AFF_FIRESHIELD );

   if( victim->has_aflag( AFF_SHOCKSHIELD ) )
      victim->unset_aflag( AFF_SHOCKSHIELD );

   if( victim->has_aflag( AFF_VENOMSHIELD ) )
      victim->unset_aflag( AFF_VENOMSHIELD );

   if( victim->has_aflag( AFF_ICESHIELD ) )
      victim->unset_aflag( AFF_ICESHIELD );

   if( victim->has_aflag( AFF_BLADEBARRIER ) )
      victim->unset_aflag( AFF_BLADEBARRIER );

   if( victim->has_aflag( AFF_SILENCE ) )
      victim->unset_aflag( AFF_SILENCE );

   if( victim->has_aflag( AFF_HASTE ) )
      victim->unset_aflag( AFF_HASTE );

   if( victim->has_aflag( AFF_SLOW ) )
      victim->unset_aflag( AFF_SLOW );

   ch->set_color( AT_MAGIC );
   ch->printf( "You weave arcane gestures, and %s's spells are negated!\r\n", victim->isnpc(  )? victim->short_descr : victim->name );

   /*
    * Have to reset victim's racial and eq affects etc 
    */
   if( !victim->isnpc(  ) )
      victim->update_aris(  );

   if( victim->isnpc(  ) )
   {
      if( !ch->fighting || ( ch->fighting && ch->fighting->who != victim ) )
         multi_hit( ch, victim, TYPE_UNDEFINED );
   }
   return rNONE;
}

SPELLF( spell_polymorph )
{
   morph_data *morph;
   skill_type *skill = get_skilltype( sn );

   morph = find_morph( ch, target_name, true );
   if( !morph )
   {
      ch->print( "You can't morph into anything like that!\r\n" );
      return rSPELL_FAILED;
   }

   if( !do_morph_char( ch, morph ) )
   {
      failed_casting( skill, ch, NULL, NULL );
      return rSPELL_FAILED;
   }
   return rNONE;
}

SPELLF( spell_disruption )
{
   char_data *victim = ( char_data * ) vo;
   int dam;

   dam = dice( level, 8 );

   if( ch->Class == CLASS_PALADIN );
   dam = dice( level, 6 );

   if( !IsUndead( victim ) )
   {
      ch->print( "Your spell fizzles into the ether.\r\n" );
      return rSPELL_FAILED;
   }

   if( saves_spell_staff( level, victim ) )
      dam = 0;
   return damage( ch, victim, dam, sn );
}

SPELLF( spell_enchant_weapon )
{
   obj_data *obj = ( obj_data * ) vo;
   affect_data *paf;

   if( obj->item_type != ITEM_WEAPON || obj->extra_flags.test( ITEM_MAGIC ) || !obj->affects.empty(  ) )
   {
      act( AT_MAGIC, "Your magic twists and winds around $p but cannot take hold.", ch, obj, NULL, TO_CHAR );
      act( AT_MAGIC, "$n's magic twists and winds around $p but cannot take hold.", ch, obj, NULL, TO_NOTVICT );
      return rSPELL_FAILED;
   }

   /*
    * Bug fix here. -- Alty 
    */
   obj->separate(  );
   paf = new affect_data;
   paf->type = -1;
   paf->duration = -1;
   paf->location = APPLY_HITROLL;
   paf->modifier = level / 15;
   paf->bit = 0;
   obj->affects.push_back( paf );

   paf = new affect_data;
   paf->type = -1;
   paf->duration = -1;
   paf->location = APPLY_DAMROLL;
   paf->modifier = level / 15;
   paf->bit = 0;
   obj->affects.push_back( paf );

   obj->extra_flags.set( ITEM_MAGIC );

   if( ch->IS_GOOD(  ) )
   {
      obj->extra_flags.set( ITEM_ANTI_EVIL );
      act( AT_BLUE, "$p gleams with flecks of blue energy.", ch, obj, NULL, TO_ROOM );
      act( AT_BLUE, "$p gleams with flecks of blue energy.", ch, obj, NULL, TO_CHAR );
   }
   else if( ch->IS_EVIL(  ) )
   {
      obj->extra_flags.set( ITEM_ANTI_GOOD );
      act( AT_BLOOD, "A crimson stain flows slowly over $p.", ch, obj, NULL, TO_CHAR );
      act( AT_BLOOD, "A crimson stain flows slowly over $p.", ch, obj, NULL, TO_ROOM );
   }
   else
   {
      obj->extra_flags.set( ITEM_ANTI_EVIL );
      obj->extra_flags.set( ITEM_ANTI_GOOD );
      act( AT_YELLOW, "$p glows with a disquieting light.", ch, obj, NULL, TO_ROOM );
      act( AT_YELLOW, "$p glows with a disquieting light.", ch, obj, NULL, TO_CHAR );
   }
   return rNONE;
}

/*
 * Drain XP, MANA, HP.
 * Caster gains HP.
 */
/* If this thing ever gets looked at in the future, the exp drain calculation needs fixing! */
SPELLF( spell_energy_drain )
{
   char_data *victim = ( char_data * ) vo;
   int dam;
   int schance;
   skill_type *skill = get_skilltype( sn );

   if( victim->has_immune( RIS_MAGIC ) )
   {
      immune_casting( skill, ch, victim, NULL );
      return rSPELL_FAILED;
   }

   schance = ris_save( victim, victim->level, RIS_DRAIN );
   if( schance == 1000 || saves_spell_staff( schance, victim ) )
   {
      failed_casting( skill, ch, victim, NULL );   /* SB */
      return rSPELL_FAILED;
   }

   ch->alignment = UMAX( -1000, ch->alignment - 200 );
   if( victim->level <= 1 )
      dam = victim->hit * 12;
   else
   {
      victim->gain_exp( 0 - number_range( level / 2, 3 * level / 2 ) );
      dam = 1;
   }

   if( ch->hit > ch->max_hit )
      ch->hit = ch->max_hit;
   return damage( ch, victim, dam, sn );
}

SPELLF( spell_faerie_fog )
{
   act( AT_MAGIC, "$n conjures a cloud of purple smoke.", ch, NULL, NULL, TO_ROOM );
   act( AT_MAGIC, "You conjure a cloud of purple smoke.", ch, NULL, NULL, TO_CHAR );

   list < char_data * >::iterator ich;
   for( ich = ch->in_room->people.begin(  ); ich != ch->in_room->people.end(  ); ++ich )
   {
      char_data *vch = *ich;

      if( vch->has_pcflag( PCFLAG_WIZINVIS ) )
         continue;

      if( !is_same_char_map( ch, vch ) )
         continue;

      if( vch == ch || saves_spell_staff( level, vch ) )
         continue;

      vch->affect_strip( gsn_invis );
      vch->affect_strip( gsn_mass_invis );
      vch->affect_strip( gsn_sneak );
      vch->unset_aflag( AFF_HIDE );
      vch->unset_aflag( AFF_INVISIBLE );
      vch->unset_aflag( AFF_SNEAK );
      act( AT_MAGIC, "$n is revealed!", vch, NULL, NULL, TO_ROOM );
      act( AT_MAGIC, "You are revealed!", vch, NULL, NULL, TO_CHAR );
   }
   return rNONE;
}

SPELLF( spell_gate )
{
   mob_index *temp;
   char_data *mob;

   if( !( temp = get_mob_index( MOB_VNUM_GATE ) ) )
   {
      bug( "%s: Servitor daemon vnum %d doesn't exist.", __FUNCTION__, MOB_VNUM_GATE );
      return rSPELL_FAILED;
   }

   if( !ch->can_charm(  ) )
   {
      ch->print( "You already have too many followers to support!\r\n" );
      return rSPELL_FAILED;
   }

   mob = temp->create_mobile(  );
   if( !mob->to_room( ch->in_room ) )
      log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
   fix_maps( ch, mob );
   act( AT_MAGIC, "You bring forth $N from another plane!", ch, NULL, mob, TO_CHAR );
   act( AT_MAGIC, "$n brings forth $N from another plane!", ch, NULL, mob, TO_ROOM );
   bind_follower( mob, ch, sn, ( int )( number_fuzzy( ( int )( ( level + 1 ) / 5 ) + 1 ) * DUR_CONV ) );
   return rNONE;
}

/* Modified by Scryn to work on mobs/players/objs */
/* Made it show short descrs instead of keywords, seeing as you need
   to know the keyword anyways, we may as well make it look nice -- Alty */
/* Output of information reformatted to look better - Samson 2-8-98 */
SPELLF( spell_identify )
{
   obj_data *obj;
   char_data *victim;
   skill_type *sktmp;
   skill_type *skill = get_skilltype( sn );
   char *name;

   if( target_name.empty(  ) )
   {
      ch->print( "What should the spell be cast upon?\r\n" );
      return rSPELL_FAILED;
   }

   if( ( obj = ch->get_obj_carry( target_name ) ) != NULL )
   {
      obj_identify_output( ch, obj );
      return rNONE;
   }

   else if( ( victim = ch->get_char_room( target_name ) ) != NULL )
   {
      if( victim->has_immune( RIS_MAGIC ) )
      {
         immune_casting( skill, ch, victim, NULL );
         return rSPELL_FAILED;
      }

      /*
       * If they are morphed or a NPC use the appropriate short_desc otherwise
       * * use their name -- Shaddai
       */

      if( victim->morph && victim->morph->morph )
         name = capitalize( victim->morph->morph->short_desc );
      else if( victim->isnpc(  ) )
         name = capitalize( victim->short_descr );
      else
         name = victim->name;

      ch->printf( "%s appears to be between level %d and %d.\r\n", name, victim->level - ( victim->level % 5 ), victim->level - ( victim->level % 5 ) + 5 );

      if( victim->isnpc(  ) && victim->morph )
         ch->printf( "%s appears to truly be %s.\r\n", name, ( ch->level > victim->level + 10 ) ? victim->name : "someone else" );

      ch->printf( "%s looks like %s, and follows the ways of the %s.\r\n", name, aoran( victim->get_race(  ) ), victim->get_class(  ) );

      if( ( ch->chance( 50 ) && ch->level >= victim->level + 10 ) || ch->is_immortal(  ) )
      {
         ch->printf( "%s appears to be affected by: ", name );

         if( victim->affects.empty(  ) )
         {
            ch->print( "nothing.\r\n" );
            return rNONE;
         }

         list < affect_data * >::iterator affidx = victim->affects.end(  );
         --affidx;
         list < affect_data * >::iterator paf;
         for( paf = victim->affects.begin(  ); paf != victim->affects.end(  ); ++paf )
         {
            affect_data *af = *paf;

            if( victim->affects.begin(  ) != affidx )
            {
               if( paf != affidx && ( sktmp = get_skilltype( af->type ) ) != NULL )
                  ch->printf( "%s, ", sktmp->name );

               if( paf == affidx && ( sktmp = get_skilltype( af->type ) ) != NULL )
               {
                  ch->printf( "and %s.\r\n", sktmp->name );
                  return rNONE;
               }
            }
            else
            {
               if( ( sktmp = get_skilltype( af->type ) ) != NULL )
                  ch->printf( "%s.\r\n", sktmp->name );
               else
                  ch->print( "\r\n" );
               return rNONE;
            }
         }
      }
   }
   else
   {
      ch->printf( "You can't find %s!\r\n", target_name.c_str(  ) );
      return rSPELL_FAILED;
   }
   return rNONE;
}

SPELLF( spell_invis )
{
   char_data *victim;
   skill_type *skill = get_skilltype( sn );

   /*
    * Modifications on 1/2/96 to work on player/object - Scryn 
    */
   if( target_name.empty(  ) )
      victim = ch;
   else
      victim = ch->get_char_room( target_name );

   if( victim && is_same_char_map( ch, victim ) )
   {
      affect_data af;

      if( victim->has_immune( RIS_MAGIC ) )
      {
         immune_casting( skill, ch, victim, NULL );
         return rSPELL_FAILED;
      }

      if( victim->has_aflag( AFF_INVISIBLE ) )
      {
         failed_casting( skill, ch, victim, NULL );
         return rSPELL_FAILED;
      }

      act( AT_MAGIC, "$n fades out of existence.", victim, NULL, NULL, TO_ROOM );
      af.type = sn;
      af.duration = ( int )( ( ( level / 4 ) + 12 ) * DUR_CONV );
      af.location = APPLY_NONE;
      af.modifier = 0;
      af.bit = AFF_INVISIBLE;
      victim->affect_to_char( &af );
      act( AT_MAGIC, "You fade out of existence.", victim, NULL, NULL, TO_CHAR );
      return rNONE;
   }
   else
   {
      obj_data *obj;

      obj = ch->get_obj_carry( target_name );

      if( obj )
      {
         obj->separate(  );   /* Fix multi-invis bug --Blod */
         if( obj->extra_flags.test( ITEM_INVIS ) || ch->chance( 40 + level / 10 ) )
         {
            failed_casting( skill, ch, NULL, NULL );
            return rSPELL_FAILED;
         }
         obj->wear_flags.set( ITEM_INVIS );
         act( AT_MAGIC, "$p fades out of existence.", ch, obj, NULL, TO_CHAR );
         return rNONE;
      }
   }
   ch->printf( "You can't find %s!\r\n", target_name.c_str(  ) );
   return rSPELL_FAILED;
}

SPELLF( spell_know_alignment )
{
   char_data *victim = ( char_data * ) vo;
   const char *msg;
   int ap;
   skill_type *skill = get_skilltype( sn );

   if( !victim )
   {
      failed_casting( skill, ch, victim, NULL );
      return rSPELL_FAILED;
   }

   if( victim->has_immune( RIS_MAGIC ) )
   {
      immune_casting( skill, ch, victim, NULL );
      return rSPELL_FAILED;
   }

   ap = victim->alignment;

   if( ap > 700 )
      msg = "$N has an aura as white as the driven snow.";
   else if( ap > 350 )
      msg = "$N is of excellent moral character.";
   else if( ap > 100 )
      msg = "$N is often kind and thoughtful.";
   else if( ap > -100 )
      msg = "$N doesn't have a firm moral commitment.";
   else if( ap > -350 )
      msg = "$N lies to $S friends.";
   else if( ap > -700 )
      msg = "$N would just as soon kill you as look at you.";
   else
      msg = "I'd rather just not say anything at all about $N.";

   act( AT_MAGIC, msg, ch, NULL, victim, TO_CHAR );
   return rNONE;
}

SPELLF( spell_locate_object )
{
   obj_data *in_obj;
   list < obj_data * >::iterator iobj;
   int cnt, found = 0;

   for( iobj = objlist.begin(  ); iobj != objlist.end(  ); ++iobj )
   {
      obj_data *obj = *iobj;

      if( !ch->can_see_obj( obj, true ) || !hasname( obj->name, target_name ) )
         continue;
      if( ( obj->extra_flags.test( ITEM_PROTOTYPE ) || obj->extra_flags.test( ITEM_NOLOCATE ) ) && !ch->is_immortal(  ) )
         continue;

      ++found;

      for( cnt = 0, in_obj = obj; in_obj->in_obj && cnt < 100; in_obj = in_obj->in_obj, ++cnt )
         ;
      if( cnt >= MAX_NEST )
      {
         bug( "%s: object [%d] %s is nested more than %d times!", __FUNCTION__, obj->pIndexData->vnum, obj->short_descr, MAX_NEST );
         continue;
      }

      ch->set_color( AT_MAGIC );
      if( in_obj->carried_by )
      {
         if( in_obj->carried_by->is_immortal(  ) && !in_obj->carried_by->isnpc(  )
             && ( ch->level < in_obj->carried_by->pcdata->wizinvis ) && in_obj->carried_by->has_pcflag( PCFLAG_WIZINVIS ) )
         {
            found--;
            continue;
         }
         ch->pagerf( "%s carried by %s.\r\n", obj->oshort(  ).c_str(  ), PERS( in_obj->carried_by, ch, false ) );
      }
      else
         ch->pagerf( "%s in %s.\r\n", obj->oshort(  ).c_str(  ), in_obj->in_room == NULL ? "somewhere" : in_obj->in_room->name );
   }

   if( !found )
   {
      ch->print( "Nothing like that exists.\r\n" );
      return rSPELL_FAILED;
   }
   return rNONE;
}

SPELLF( spell_remove_trap )
{
   obj_data *trap, *obj = NULL;
   list < obj_data * >::iterator iobj;
   skill_type *skill = get_skilltype( sn );

   if( target_name.empty(  ) )
   {
      ch->print( "Remove trap on what?\r\n" );
      return rSPELL_FAILED;
   }

   if( ch->in_room->objects.empty(  ) )
   {
      ch->print( "You can't find that here.\r\n" );
      return rNONE;
   }

   bool found = false;
   for( iobj = ch->in_room->objects.begin(  ); iobj != ch->in_room->objects.end(  ); ++iobj )
   {
      obj = *iobj;

      if( ch->can_see_obj( obj, false ) && hasname( obj->name, target_name ) )
      {
         found = true;
         break;
      }
   }

   if( !found )
   {
      ch->print( "You can't find that here.\r\n" );
      return rSPELL_FAILED;
   }

   if( !( trap = obj->get_trap(  ) ) )
   {
      failed_casting( skill, ch, NULL, NULL );
      return rSPELL_FAILED;
   }

   if( !ch->chance( 70 + ch->get_curr_wis(  ) ) )
   {
      ch->print( "Ooops!\r\n" );
      ch_ret retcode = spring_trap( ch, trap );
      if( retcode == rNONE )
         retcode = rSPELL_FAILED;
      return retcode;
   }
   trap->extract(  );
   successful_casting( skill, ch, NULL, NULL );
   return rNONE;
}

SPELLF( spell_sleep )
{
   affect_data af;
   int retcode, schance, tmp;
   char_data *victim;
   skill_type *skill = get_skilltype( sn );

   if( !( victim = ch->get_char_room( target_name ) ) )
   {
      ch->print( "They aren't here.\r\n" );
      return rSPELL_FAILED;
   }

   if( !victim->isnpc(  ) && victim->fighting )
   {
      ch->print( "You cannot sleep a fighting player.\r\n" );
      return rSPELL_FAILED;
   }

   if( is_safe( ch, victim ) )
      return rSPELL_FAILED;

   if( victim->has_immune( RIS_MAGIC ) )
   {
      immune_casting( skill, ch, victim, NULL );
      return rSPELL_FAILED;
   }

   if( SPELL_FLAG( skill, SF_PKSENSITIVE ) && !ch->isnpc(  ) && !victim->isnpc(  ) )
      tmp = level / 2;
   else
      tmp = level;

   if( victim->has_aflag( AFF_SLEEP ) || ( schance = ris_save( victim, tmp, RIS_SLEEP ) ) == 1000
       || level < victim->level || ( victim != ch && victim->in_room->flags.test( ROOM_SAFE ) ) || saves_spell_staff( schance, victim ) )
   {
      failed_casting( skill, ch, victim, NULL );
      if( ch == victim )
         return rSPELL_FAILED;
      if( !victim->fighting )
      {
         retcode = multi_hit( victim, ch, TYPE_UNDEFINED );
         if( retcode == rNONE )
            retcode = rSPELL_FAILED;
         return retcode;
      }
   }
   af.type = sn;
   af.duration = ( int )( ( 4 + level ) * DUR_CONV );
   af.location = APPLY_NONE;
   af.modifier = 0;
   af.bit = AFF_SLEEP;
   victim->affect_join( &af );

   if( victim->IS_AWAKE(  ) )
   {
      act( AT_MAGIC, "You feel very sleepy ..... zzzzzz.", victim, NULL, NULL, TO_CHAR );
      act( AT_MAGIC, "$n goes to sleep.", victim, NULL, NULL, TO_ROOM );
      victim->position = POS_SLEEPING;
   }
   if( victim->isnpc(  ) )
      start_hating( victim, ch );

   return rNONE;
}

SPELLF( spell_summon )
{
   char_data *victim;

   if( !( victim = ch->get_char_world( target_name ) ) || !victim->in_room )
   {
      ch->print( "Nobody matching that name is on.\r\n" );
      return rSPELL_FAILED;
   }

   if( victim == ch )
   {
      ch->print( "You can't be serious??\r\n" );
      return rSPELL_FAILED;
   }

   if( victim->isnpc(  ) )
   {
      ch->print( "Summoning NPCs is not permitted.\r\n" );
      return rSPELL_FAILED;
   }

   if( ch->in_room->flags.test( ROOM_NO_SUMMON ) || victim->in_room->flags.test( ROOM_NO_SUMMON )
       || victim->in_room->area->flags.test( AFLAG_NOSUMMON ) || ch->in_room->area->flags.test( AFLAG_NOSUMMON ) )
   {
      ch->print( "Arcane magic blocks the spell.\r\n" );
      return rSPELL_FAILED;
   }

   if( victim->is_immortal(  ) && !ch->is_immortal(  ) )
   {
      ch->print( "Summoning immortals is not permitted.\r\n" );
      return rSPELL_FAILED;
   }

   if( victim->has_pcflag( PCFLAG_NOSUMMON ) && !ch->isnpc(  ) )
   {
      ch->print( "That person does not permit summonings.\r\n" );
      return rSPELL_FAILED;
   }

   /*
    * Check to see if victim has been here at least once - Samson 7-11-00 
    */
   if( !victim->has_visited( ch->in_room->area ) )
   {
      ch->printf( "%s has not yet formed a magical bond with this area!\r\n", victim->name );
      return rSPELL_FAILED;
   }

   if( victim->fighting )
   {
      ch->print( "You cannot obtain an accurate fix, they are moving around too much!\r\n" );
      return rSPELL_FAILED;
   }

   if( !ch->isnpc(  ) && !ch->is_immortal(  ) )
   {
      act( AT_MAGIC, "You feel a wave of nausea overcome you...", ch, NULL, NULL, TO_CHAR );
      act( AT_MAGIC, "$n collapses, stunned!", ch, NULL, NULL, TO_ROOM );
      ch->position = POS_STUNNED;
   }

   act( AT_MAGIC, "$n disappears suddenly.", victim, NULL, NULL, TO_ROOM );

   leave_map( victim, ch, ch->in_room );

   act( AT_MAGIC, "$n arrives suddenly.", victim, NULL, NULL, TO_ROOM );
   act( AT_MAGIC, "$N has summoned you!", victim, NULL, ch, TO_CHAR );

   return rNONE;
}

/*
 * Travel via the astral plains to quickly travel to desired location
 *	-Thoric
 *
 * Uses SMAUG spell messages is available to allow use as a SMAUG spell
 */
/* Modified by Samson, arrives in Astral Plane area at randomly chosen location */
SPELLF( spell_astral_walk )
{
   room_index *location;

   if( !( location = get_room_index( astral_target ) ) || ch->in_room->flags.test( ROOM_NO_ASTRAL ) || ch->in_room->area->flags.test( AFLAG_NOASTRAL ) )
   {
      ch->print( "&[magic]A mysterious force prevents you from opening a gateway.\r\n" );
      return rSPELL_FAILED;
   }

   act( AT_MAGIC, "$n opens a gateway in the air, and steps through onto another plane!", ch, NULL, NULL, TO_ROOM );

   room_index *original = ch->in_room;
   leave_map( ch, NULL, location );

   act( AT_MAGIC, "You open a gateway onto another plane!", ch, NULL, NULL, TO_CHAR );
   act( AT_MAGIC, "$n appears from a gateway in thin air!", ch, NULL, NULL, TO_ROOM );

   list < char_data * >::iterator ich;
   for( ich = original->people.begin(  ); ich != original->people.end(  ); )
   {
      char_data *vch = ( *ich );
      ++ich;

      if( !is_same_group( vch, ch ) )
         continue;

      act( AT_MAGIC, "The gateway remains open long enough for you to follow $s through.", ch, NULL, vch, TO_VICT );
      leave_map( vch, ch, location );
   }
   astral_target = number_range( 4350, 4449 );
   return rNONE;
}

SPELLF( spell_teleport )
{
   char_data *victim = ( char_data * ) vo;
   room_index *pRoomIndex, *from;
   skill_type *skill = get_skilltype( sn );
   int schance;

   if( !victim->in_room )
   {
      ch->print( "No such person is here.\r\n" );
      return rSPELL_FAILED;
   }

   if( victim->in_room->flags.test( ROOM_NOTELEPORT ) || victim->in_room->area->flags.test( AFLAG_NOTELEPORT ) )
   {
      ch->print( "Arcane magic disrupts the spell.\r\n" );
      return rSPELL_FAILED;
   }

   if( ( !ch->isnpc(  ) && victim->fighting ) || ( victim != ch && ( saves_spell_staff( level, victim ) || saves_wands( level, victim ) ) ) )
   {
      failed_casting( skill, ch, victim, NULL );
      return rSPELL_FAILED;
   }

   schance = number_range( 1, 2 );

   if( ch->on )
   {
      ch->on = NULL;
      ch->position = POS_STANDING;
   }
   if( ch->position != POS_STANDING )
      ch->position = POS_STANDING;

   from = victim->in_room;

   if( schance == 1 )
   {
      short map, x, y, sector;

      for( ;; )
      {
         map = number_range( 0, MAP_MAX - 1 );
         x = number_range( 0, MAX_X - 1 );
         y = number_range( 0, MAX_Y - 1 );

         sector = get_terrain( map, x, y );
         if( sector == -1 )
            continue;
         if( sect_show[sector].canpass )
            break;
      }
      act( AT_MAGIC, "$n slowly fades out of view.", victim, NULL, NULL, TO_ROOM );
      enter_map( victim, NULL, x, y, map );
      collect_followers( victim, from, victim->in_room );
      if( !victim->isnpc(  ) )
         act( AT_MAGIC, "$n slowly fades into view.", victim, NULL, NULL, TO_ROOM );
   }
   else
   {
      for( ;; )
      {
         pRoomIndex = get_room_index( number_range( 0, sysdata->maxvnum ) );
         if( pRoomIndex )
            if( !pRoomIndex->flags.test( ROOM_NOTELEPORT )
                && !pRoomIndex->area->flags.test( AFLAG_NOTELEPORT ) && !pRoomIndex->flags.test( ROOM_PROTOTYPE ) && victim->in_hard_range( pRoomIndex->area ) )
               break;
      }
      act( AT_MAGIC, "$n slowly fades out of view.", victim, NULL, NULL, TO_ROOM );
      leave_map( victim, NULL, pRoomIndex );
      if( !victim->isnpc(  ) )
         act( AT_MAGIC, "$n slowly fades into view.", victim, NULL, NULL, TO_ROOM );
   }
   return rNONE;
}

SPELLF( spell_ventriloquate )
{
   char buf1[MSL], buf2[MSL];
   string speaker;

   target_name = one_argument( target_name, speaker );

   snprintf( buf1, MSL, "%s says '%s'.\r\n", speaker.c_str(  ), target_name.c_str(  ) );
   snprintf( buf2, MSL, "Someone makes %s say '%s'.\r\n", speaker.c_str(  ), target_name.c_str(  ) );
   buf1[0] = UPPER( buf1[0] );

   list < char_data * >::iterator ich;
   for( ich = ch->in_room->people.begin(  ); ich != ch->in_room->people.end(  ); ++ich )
   {
      char_data *vch = *ich;

      if( !hasname( vch->name, speaker ) && is_same_char_map( ch, vch ) )
         vch->printf( "&[say]%s\r\n", saves_spell_staff( level, vch ) ? buf2 : buf1 );
   }
   return rNONE;
}

/* MUST BE KEPT!!!! Used by attack types for mobiles!!! */
SPELLF( spell_weaken )
{
   char_data *victim = ( char_data * ) vo;
   affect_data af;
   skill_type *skill = get_skilltype( sn );

   ch->set_color( AT_MAGIC );
   if( victim->has_immune( RIS_MAGIC ) )
   {
      immune_casting( skill, ch, victim, NULL );
      return rSPELL_FAILED;
   }
   if( victim->is_affected( sn ) || saves_wands( level, victim ) )
   {
      ch->print( "Your magic fails to take hold.\r\n" );
      return rSPELL_FAILED;
   }
   af.type = sn;
   af.duration = ( int )( level / 2 * DUR_CONV );
   af.location = APPLY_STR;
   af.modifier = -2;
   af.bit = 0;
   victim->affect_to_char( &af );
   victim->print( "&[magic]Your muscles seem to atrophy!\r\n" );
   if( ch != victim )
   {
      if( ( ( ( !victim->isnpc(  ) && class_table[victim->Class]->attr_prime == APPLY_STR ) || victim->isnpc(  ) )
            && victim->get_curr_str(  ) < 25 ) || victim->get_curr_str(  ) < 20 )
      {
         act( AT_MAGIC, "$N labors weakly as your spell atrophies $S muscles.", ch, NULL, victim, TO_CHAR );
         act( AT_MAGIC, "$N labors weakly as $n's spell atrophies $S muscles.", ch, NULL, victim, TO_NOTVICT );
      }
      else
      {
         act( AT_MAGIC, "You induce a mild atrophy in $N's muscles.", ch, NULL, victim, TO_CHAR );
         act( AT_MAGIC, "$n induces a mild atrophy in $N's muscles.", ch, NULL, victim, TO_NOTVICT );
      }
   }
   return rNONE;
}

/*
 * A spell as it should be				-Thoric
 */
SPELLF( spell_word_of_recall )
{
   string arg3;
   int call = -1;
   int target = -1;

   target_name = one_argument( target_name, arg3 );

   if( arg3.empty(  ) )
      call = recall( ch, -1 );

   else
   {
      target = atoi( arg3.c_str(  ) );

      if( target < 0 || target >= MAX_BEACONS )
      {
         ch->print( "Beacon value is out of range.\r\n" );
         return rSPELL_FAILED;
      }
      call = recall( ch, target );
   }

   if( call == 1 )
      return rNONE;
   else
      return rSPELL_FAILED;
}

/*
 * NPC spells.
 */
SPELLF( spell_acid_breath )
{
   char_data *victim = ( char_data * ) vo;
   obj_data *obj_lose;
   list < obj_data * >::iterator iobj;
   int dam, hpch;

   if( ch->chance( 2 * level ) && !saves_breath( level, victim ) )
   {
      for( iobj = victim->carrying.begin(  ); iobj != victim->carrying.end(  ); )
      {
         obj_lose = *iobj;
         ++iobj;

         int iWear;

         if( number_bits( 2 ) != 0 )
            continue;

         switch ( obj_lose->item_type )
         {
            default:
               break;

            case ITEM_ARMOR:
               if( obj_lose->value[0] > 0 )
               {
                  obj_lose->separate(  );
                  act( AT_DAMAGE, "$p is pitted and etched!", victim, obj_lose, NULL, TO_CHAR );
                  if( ( iWear = obj_lose->wear_loc ) != WEAR_NONE )
                     victim->armor += obj_lose->apply_ac( iWear );
                  obj_lose->value[0] -= 1;
                  obj_lose->cost = 0;
                  if( iWear != WEAR_NONE )
                     victim->armor -= obj_lose->apply_ac( iWear );
               }
               break;

            case ITEM_CONTAINER:
               obj_lose->separate(  );
               act( AT_DAMAGE, "$p fumes and dissolves!", victim, obj_lose, NULL, TO_CHAR );
               act( AT_OBJECT, "The contents of $p held by $N spill onto the ground.", victim, obj_lose, victim, TO_ROOM );
               act( AT_OBJECT, "The contents of $p spill out onto the ground!", victim, obj_lose, NULL, TO_CHAR );
               obj_lose->empty( NULL, victim->in_room );
               obj_lose->extract(  );
               break;
         }
      }
   }
   hpch = UMAX( 10, ch->hit );
   dam = number_range( hpch / 16 + 1, hpch / 8 );
   if( saves_breath( level, victim ) )
      dam /= 2;
   return damage( ch, victim, dam, sn );
}

SPELLF( spell_fire_breath )
{
   char_data *victim = ( char_data * ) vo;
   obj_data *obj_lose;
   list < obj_data * >::iterator iobj;
   int dam, hpch;

   if( ch->chance( 2 * level ) && !saves_breath( level, victim ) )
   {
      for( iobj = victim->carrying.begin(  ); iobj != victim->carrying.end(  ); )
      {
         obj_lose = *iobj;
         ++iobj;
         const char *msg;

         if( number_bits( 2 ) != 0 )
            continue;

         switch ( obj_lose->item_type )
         {
            default:
               continue;
            case ITEM_CONTAINER:
               msg = "$p ignites and burns!";
               break;
            case ITEM_POTION:
               msg = "$p bubbles and boils!";
               break;
            case ITEM_SCROLL:
               msg = "$p crackles and burns!";
               break;
            case ITEM_STAFF:
               msg = "$p smokes and chars!";
               break;
            case ITEM_WAND:
               msg = "$p sparks and sputters!";
               break;
            case ITEM_COOK:
            case ITEM_FOOD:
               msg = "$p blackens and crisps!";
               break;
            case ITEM_PILL:
               msg = "$p melts and drips!";
               break;
         }

         obj_lose->separate(  );
         act( AT_DAMAGE, msg, victim, obj_lose, NULL, TO_CHAR );
         if( obj_lose->item_type == ITEM_CONTAINER )
         {
            act( AT_OBJECT, "The contents of $p held by $N spill onto the ground.", victim, obj_lose, victim, TO_ROOM );
            act( AT_OBJECT, "The contents of $p spill out onto the ground!", victim, obj_lose, NULL, TO_CHAR );
            obj_lose->empty( NULL, victim->in_room );
         }
         obj_lose->extract(  );
      }
   }

   hpch = UMAX( 10, ch->hit );
   dam = number_range( hpch / 16 + 1, hpch / 8 );
   if( saves_breath( level, victim ) )
      dam /= 2;
   return damage( ch, victim, dam, sn );
}

SPELLF( spell_frost_breath )
{
   char_data *victim = ( char_data * ) vo;
   obj_data *obj_lose;
   list < obj_data * >::iterator iobj;
   int dam, hpch;

   if( ch->chance( 2 * level ) && !saves_breath( level, victim ) )
   {
      for( iobj = victim->carrying.begin(  ); iobj != victim->carrying.end(  ); )
      {
         obj_lose = *iobj;
         ++iobj;
         const char *msg;

         if( number_bits( 2 ) != 0 )
            continue;

         switch ( obj_lose->item_type )
         {
            default:
               continue;
            case ITEM_CONTAINER:
            case ITEM_DRINK_CON:
            case ITEM_POTION:
               msg = "$p freezes and shatters!";
               break;
         }

         obj_lose->separate(  );
         act( AT_DAMAGE, msg, victim, obj_lose, NULL, TO_CHAR );
         if( obj_lose->item_type == ITEM_CONTAINER )
         {
            act( AT_OBJECT, "The contents of $p held by $N spill onto the ground.", victim, obj_lose, victim, TO_ROOM );
            act( AT_OBJECT, "The contents of $p spill out onto the ground!", victim, obj_lose, NULL, TO_CHAR );
            obj_lose->empty( NULL, victim->in_room );
         }
         obj_lose->extract(  );
      }
   }

   hpch = UMAX( 10, ch->hit );
   dam = number_range( hpch / 16 + 1, hpch / 8 );
   if( saves_breath( level, victim ) )
      dam /= 2;
   return damage( ch, victim, dam, sn );
}

SPELLF( spell_gas_breath )
{
   if( ch->in_room->flags.test( ROOM_SAFE ) )
   {
      ch->print( "&[magic]You fail to breathe.\r\n" );
      return rNONE;
   }

   bool ch_died = false;
   list < char_data * >::iterator ich;
   for( ich = ch->in_room->people.begin(  ); ich != ch->in_room->people.end(  ); )
   {
      char_data *vch = *ich;
      ++ich;

      if( vch->has_pcflag( PCFLAG_WIZINVIS ) && vch->pcdata->wizinvis >= LEVEL_IMMORTAL )
         continue;

      if( vch->has_pcflag( PCFLAG_ONMAP ) || vch->has_actflag( ACT_ONMAP ) )
      {
         if( !is_same_char_map( vch, ch ) )
            continue;
      }

      if( ch->isnpc(  )? !vch->isnpc(  ) : vch->isnpc(  ) )
      {
         int hpch = UMAX( 10, ch->hit );
         int dam = number_range( hpch / 16 + 1, hpch / 8 );
         if( saves_breath( level, vch ) )
            dam /= 2;
         if( damage( ch, vch, dam, sn ) == rCHAR_DIED || ch->char_died(  ) )
            ch_died = true;
      }
   }
   if( ch_died )
      return rCHAR_DIED;
   else
      return rNONE;
}

SPELLF( spell_lightning_breath )
{
   char_data *victim = ( char_data * ) vo;
   int dam, hpch;

   hpch = UMAX( 10, ch->hit );
   dam = number_range( hpch / 16 + 1, hpch / 8 );
   if( saves_breath( level, victim ) )
      dam /= 2;
   return damage( ch, victim, dam, sn );
}

SPELLF( spell_null )
{
   ch->print( "That's not a spell!\r\n" );
   return rNONE;
}

/* don't remove, may look redundant, but is important */
SPELLF( spell_notfound )
{
   ch->print( "That's not a spell!\r\n" );
   return rNONE;
}

/*
 * Haus' Spell Additions
 */

/* to do: portal           (like mpcreatepassage)
 *        sharpness        (makes weapon of caster's level)
 *        repair           (repairs armor)
 *        blood burn       (offensive)  * name: net book of spells *
 *        spirit scream    (offensive)  * name: net book of spells *
 *        something about saltpeter or brimstone
 */

/* Working on DM's transport eq suggestion - Scryn 8/13 */
SPELLF( spell_transport )
{
   char_data *victim;
   string arg3;
   obj_data *obj;
   skill_type *skill = get_skilltype( sn );

   target_name = one_argument( target_name, arg3 );

   if( !( victim = ch->get_char_world( target_name ) )
       || victim == ch
       || victim->in_room->flags.test( ROOM_PRIVATE )
       || victim->in_room->flags.test( ROOM_SOLITARY )
       || victim->in_room->flags.test( ROOM_NOTELEPORT )
       || victim->in_room->area->flags.test( AFLAG_NOTELEPORT )
       || ch->in_room->flags.test( ROOM_NOTELEPORT )
       || ch->in_room->area->flags.test( AFLAG_NOTELEPORT )
       || victim->in_room->flags.test( ROOM_PROTOTYPE ) || victim->has_actflag( ACT_PROTOTYPE ) || ( victim->isnpc(  ) && saves_spell_staff( level, victim ) ) )
   {
      failed_casting( skill, ch, victim, NULL );
      return rSPELL_FAILED;
   }

   if( victim->in_room == ch->in_room && is_same_char_map( victim, ch ) )
   {
      ch->print( "They are right beside you!" );
      return rSPELL_FAILED;
   }

   if( !( obj = ch->get_obj_carry( arg3 ) ) || ( victim->carry_weight + obj->get_weight(  ) ) > victim->can_carry_w(  ) || victim->has_actflag( ACT_PROTOTYPE ) )
   {
      failed_casting( skill, ch, victim, NULL );
      return rSPELL_FAILED;
   }

   obj->separate(  );   /* altrag shoots, haus alley-oops! */

   if( obj->extra_flags.test( ITEM_NODROP ) || obj->extra_flags.test( ITEM_SINDHAE ) )
   {
      ch->print( "You can't seem to let go of it.\r\n" );
      return rSPELL_FAILED;   /* nice catch, caine */
   }

   if( obj->extra_flags.test( ITEM_PROTOTYPE ) && victim->level < LEVEL_IMMORTAL )
   {
      ch->print( "That item is not for mortal hands to touch!\r\n" );
      return rSPELL_FAILED;   /* Thoric */
   }

   act( AT_MAGIC, "$p slowly dematerializes...", ch, obj, NULL, TO_CHAR );
   act( AT_MAGIC, "$p slowly dematerializes from $n's hands..", ch, obj, NULL, TO_ROOM );
   obj->from_char(  );
   obj->to_char( victim );
   act( AT_MAGIC, "$p from $n appears in your hands!", ch, obj, victim, TO_VICT );
   act( AT_MAGIC, "$p appears in $n's hands!", victim, obj, NULL, TO_ROOM );
   ch->save(  );
   victim->save(  );
   return rNONE;
}

/*
 * Syntax portal (mob/char) 
 * opens a 2-way EX_PORTAL from caster's room to room inhabited by  
 *  mob or character won't mess with existing exits
 *
 * do_mp_open_passage, combined with spell_astral
 */
SPELLF( spell_portal )
{
   char_data *victim;
   skill_type *skill = get_skilltype( sn );

   /*
    * No go if all kinds of things aren't just right, including the caster
    * and victim are not both pkill or both peaceful. -- Narn
    */
   if( !( victim = ch->get_char_world( target_name ) ) || !victim->in_room || victim->in_room->flags.test( ROOM_PROTOTYPE ) || victim->has_actflag( ACT_PROTOTYPE ) )
   {
      ch->print( "Nobody matching that target name is around.\r\n" );
      return rSPELL_FAILED;
   }

   if( victim == ch )
   {
      ch->print( "What?? Make a portal to yourself?\r\n" );
      return rSPELL_FAILED;
   }

   if( ch->has_pcflag( PCFLAG_ONMAP ) || victim->has_pcflag( PCFLAG_ONMAP ) || ch->has_actflag( ACT_ONMAP ) || victim->has_actflag( ACT_ONMAP ) )
   {
      ch->print( "Portals cannot be created to or from overland maps.\r\n" );
      return rSPELL_FAILED;
   }

   if( victim->in_room == ch->in_room )
   {
      ch->print( "They are right beside you!" );
      return rSPELL_FAILED;
   }

   if( victim->in_room->flags.test( ROOM_NO_PORTAL ) || victim->in_room->area->flags.test( AFLAG_NOPORTAL )
       || ch->in_room->flags.test( ROOM_NO_PORTAL ) || ch->in_room->area->flags.test( AFLAG_NOPORTAL ) )
   {
      ch->print( "Arcane magic blocks the formation of the portal.\r\n" );
      return rSPELL_FAILED;
   }

   /*
    * Check to see if ch has actually been there at least once - Samson 7-11-00 
    */
   if( !ch->has_visited( victim->in_room->area ) )
   {
      ch->print( "You have not formed a magical bond with that area yet!\r\n" );
      return rSPELL_FAILED;
   }

   if( victim->isnpc(  ) && saves_spell_staff( level, victim ) )
   {
      ch->print( "Your target resisted the attempt to create the portal.\r\n" );
      victim->print( "Your powers have thwarted an attempted portal...\r\n" );
      return rSPELL_FAILED;
   }

   int targetRoomVnum = victim->in_room->vnum;
   room_index *fromRoom = ch->in_room;
   room_index *targetRoom = victim->in_room;
   exit_data *pexit;

   /*
    * Check if there already is a portal in either room. 
    */
   list < exit_data * >::iterator iexit;
   for( iexit = fromRoom->exits.begin(  ); iexit != fromRoom->exits.end(  ); ++iexit )
   {
      pexit = *iexit;
      if( IS_EXIT_FLAG( pexit, EX_PORTAL ) )
      {
         ch->print( "There is already a portal in this room.\r\n" );
         return rSPELL_FAILED;
      }

      if( pexit->vdir == DIR_PORTAL )
      {
         ch->print( "You may not create a portal in this room.\r\n" );
         return rSPELL_FAILED;
      }
   }

   for( iexit = targetRoom->exits.begin(  ); iexit != targetRoom->exits.end(  ); ++iexit )
   {
      pexit = *iexit;
      if( pexit->vdir == DIR_PORTAL )
      {
         failed_casting( skill, ch, victim, NULL );
         return rSPELL_FAILED;
      }
   }

   pexit = fromRoom->make_exit( targetRoom, DIR_PORTAL );
   pexit->keyword = STRALLOC( "portal" );
   pexit->exitdesc = STRALLOC( "You gaze into the shimmering portal...\r\n" );
   pexit->key = -1;
   SET_EXIT_FLAG( pexit, EX_PORTAL );
   SET_EXIT_FLAG( pexit, EX_xENTER );
   SET_EXIT_FLAG( pexit, EX_HIDDEN );
   SET_EXIT_FLAG( pexit, EX_xLOOK );
   pexit->vnum = targetRoomVnum;

   obj_data *portalObj;
   if( !( portalObj = get_obj_index( OBJ_VNUM_PORTAL )->create_object( 1 ) ) )
   {
      log_printf( "create_object: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
      return rSPELL_FAILED;
   }
   portalObj->timer = 3;
   stralloc_printf( &portalObj->short_descr, "a portal created by %s", ch->name );

   /*
    * support for new casting messages 
    */
   if( !skill->hit_char || skill->hit_char[0] == '\0' )
      ch->print( "&[magic]You utter an incantation, and a portal forms in front of you!\r\n" );
   else
      act( AT_MAGIC, skill->hit_char, ch, NULL, victim, TO_CHAR );
   if( !skill->hit_room || skill->hit_room[0] == '\0' )
      act( AT_MAGIC, "$n utters an incantation, and a portal forms in front of you!", ch, NULL, NULL, TO_ROOM );
   else
      act( AT_MAGIC, skill->hit_room, ch, NULL, victim, TO_ROOM );
   if( !skill->hit_vict || skill->hit_vict[0] == '\0' )
      act( AT_MAGIC, "A shimmering portal forms in front of you!", victim, NULL, NULL, TO_ROOM );
   else
      act( AT_MAGIC, skill->hit_vict, victim, NULL, victim, TO_ROOM );
   portalObj = portalObj->to_room( ch->in_room, ch );

   pexit = targetRoom->make_exit( fromRoom, DIR_PORTAL );
   pexit->keyword = STRALLOC( "portal" );
   pexit->exitdesc = STRALLOC( "You gaze into the shimmering portal...\r\n" );
   pexit->key = -1;
   SET_EXIT_FLAG( pexit, EX_PORTAL );
   SET_EXIT_FLAG( pexit, EX_xENTER );
   SET_EXIT_FLAG( pexit, EX_HIDDEN );
   pexit->vnum = targetRoomVnum;

   portalObj = get_obj_index( OBJ_VNUM_PORTAL )->create_object( 1 );
   portalObj->timer = 3;
   stralloc_printf( &portalObj->short_descr, "a portal created by %s", ch->name );
   portalObj = portalObj->to_room( targetRoom, victim );
   return rNONE;
}

SPELLF( spell_farsight )
{
   room_index *location, *original;
   char_data *victim;
   skill_type *skill = get_skilltype( sn );
   int origmap, origx, origy;
   bool visited;

   /*
    * The spell fails if the victim isn't playing, the victim is the caster,
    * the target room has private, solitary, noastral, death or proto flags,
    * the caster's room is norecall, the victim is too high in level, the 
    * victim is a proto mob, the victim makes the saving throw or the pkill 
    * flag on the caster is not the same as on the victim.  Got it?
    */
   /*
    * Sure do, now bite me, I'm changing it anyway guys :P - Samson 
    */
   if( !( victim = ch->get_char_world( target_name ) )
       || victim == ch || !victim->in_room || victim->in_room->flags.test( ROOM_PROTOTYPE ) || victim->has_actflag( ACT_PROTOTYPE ) )
   {
      ch->print( "Nobody matching that target name is around.\r\n" );
      return rSPELL_FAILED;
   }

   if( victim->in_room->flags.test( ROOM_NOSCRY ) || ch->in_room->flags.test( ROOM_NOSCRY )
       || victim->in_room->area->flags.test( AFLAG_NOSCRY ) || ch->in_room->area->flags.test( AFLAG_NOSCRY ) )
   {
      ch->print( "Arcane magic blocks your vision.\r\n" );
      return rSPELL_FAILED;
   }

   if( victim->isnpc(  ) && saves_spell_staff( level, victim ) )
   {
      ch->print( "Your target resisted the spell.\r\n" );
      victim->print( "You feel as though someone is watching you....\r\n" );
      return rSPELL_FAILED;
   }

   location = victim->in_room;
   if( !location )
   {
      failed_casting( skill, ch, victim, NULL );
      return rSPELL_FAILED;
   }
   successful_casting( skill, ch, victim, NULL );

   original = ch->in_room;
   origmap = ch->cmap;
   origx = ch->mx;
   origy = ch->my;

   /*
    * Bunch of checks to make sure the caster is on the same grid as the target - Samson 
    */
   if( location->flags.test( ROOM_MAP ) && !ch->has_pcflag( PCFLAG_ONMAP ) )
   {
      ch->set_pcflag( PCFLAG_ONMAP );
      ch->cmap = victim->cmap;
      ch->mx = victim->mx;
      ch->my = victim->my;
   }
   else if( location->flags.test( ROOM_MAP ) && ch->has_pcflag( PCFLAG_ONMAP ) )
   {
      ch->cmap = victim->cmap;
      ch->mx = victim->mx;
      ch->my = victim->my;
   }
   else if( !location->flags.test( ROOM_MAP ) && ch->has_pcflag( PCFLAG_ONMAP ) )
   {
      ch->unset_pcflag( PCFLAG_ONMAP );
      ch->cmap = -1;
      ch->mx = -1;
      ch->my = -1;
   }

   visited = ch->has_visited( location->area );
   ch->from_room(  );
   if( !ch->to_room( location ) )
      log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );

   interpret( ch, "look" );
   ch->from_room(  );
   if( !ch->to_room( original ) )
      log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
   if( !visited )
      ch->remove_visit( location );

   if( ch->has_pcflag( PCFLAG_ONMAP ) && !original->flags.test( ROOM_MAP ) )
      ch->unset_pcflag( PCFLAG_ONMAP );
   else if( !ch->has_pcflag( PCFLAG_ONMAP ) && original->flags.test( ROOM_MAP ) )
      ch->set_pcflag( PCFLAG_ONMAP );

   ch->cmap = origmap;
   ch->mx = origx;
   ch->my = origy;
   return rNONE;
}

SPELLF( spell_recharge )
{
   obj_data *obj = ( obj_data * ) vo;

   if( obj->item_type == ITEM_STAFF || obj->item_type == ITEM_WAND )
   {
      obj->separate(  );
      if( obj->value[2] == obj->value[1] || obj->value[1] > ( obj->pIndexData->value[1] * 4 ) )
      {
         act( AT_FIRE, "$p bursts into flames, injuring you!", ch, obj, NULL, TO_CHAR );
         act( AT_FIRE, "$p bursts into flames, charring $n!", ch, obj, NULL, TO_ROOM );
         obj->extract(  );
         if( damage( ch, ch, obj->level * 2, TYPE_UNDEFINED ) == rCHAR_DIED || ch->char_died(  ) )
            return rCHAR_DIED;
         else
            return rSPELL_FAILED;
      }

      if( ch->chance( 2 ) )
      {
         act( AT_YELLOW, "$p glows with a blinding magical luminescence.", ch, obj, NULL, TO_CHAR );
         obj->value[1] *= 2;
         obj->value[2] = obj->value[1];
         return rNONE;
      }
      else if( ch->chance( 5 ) )
      {
         act( AT_YELLOW, "$p glows brightly for a few seconds...", ch, obj, NULL, TO_CHAR );
         obj->value[2] = obj->value[1];
         return rNONE;
      }
      else if( ch->chance( 10 ) )
      {
         act( AT_WHITE, "$p disintegrates into a void.", ch, obj, NULL, TO_CHAR );
         act( AT_WHITE, "$n's attempt at recharging fails, and $p disintegrates.", ch, obj, NULL, TO_ROOM );
         obj->extract(  );
         return rSPELL_FAILED;
      }
      else if( ch->chance( 50 - ( ch->level / 2 ) ) )
      {
         ch->print( "Nothing happens.\r\n" );
         return rSPELL_FAILED;
      }
      else
      {
         act( AT_MAGIC, "$p feels warm to the touch.", ch, obj, NULL, TO_CHAR );
         --obj->value[1];
         obj->value[2] = obj->value[1];
         return rNONE;
      }
   }
   else
   {
      ch->print( "You can't recharge that!\r\n" );
      return rSPELL_FAILED;
   }
}

/*
 * Idea from AD&D 2nd edition player's handbook (c)1989 TSR Hobbies Inc.
 * -Thoric
 */
SPELLF( spell_plant_pass )
{
   char_data *victim;
   skill_type *skill = get_skilltype( sn );

   if( !( victim = ch->get_char_world( target_name ) )
       || victim == ch
       || !victim->in_room
       || victim->in_room->flags.test( ROOM_PRIVATE ) || victim->in_room->flags.test( ROOM_SOLITARY )
       || victim->in_room->flags.test( ROOM_NO_ASTRAL ) || victim->in_room->flags.test( ROOM_DEATH )
       || victim->in_room->flags.test( ROOM_PROTOTYPE )
       || ( victim->in_room->sector_type != SECT_FOREST && victim->in_room->sector_type != SECT_FIELD )
       || ( ch->in_room->sector_type != SECT_FOREST && ch->in_room->sector_type != SECT_FIELD )
       || ch->in_room->flags.test( ROOM_NO_RECALL )
       || victim->level >= level + 15
       || ( victim->CAN_PKILL(  ) && !ch->isnpc(  ) && !ch->IS_PKILL(  ) )
       || victim->has_actflag( ACT_PROTOTYPE )
       || ( victim->isnpc(  ) && saves_spell_staff( level, victim ) )
       || !ch->in_hard_range( victim->in_room->area ) || ( victim->in_room->area->flags.test( AFLAG_NOPKILL ) && ch->IS_PKILL(  ) ) )
   {
      failed_casting( skill, ch, victim, NULL );
      return rSPELL_FAILED;
   }

   /*
    * Check to see if ch has visited the target area - Samson 7-11-00 
    */
   if( !ch->has_visited( victim->in_room->area ) )
   {
      ch->print( "You have not yet formed a magical bond with that area!\r\n" );
      return rSPELL_FAILED;
   }

   if( ch->in_room->sector_type == SECT_FOREST )
      act( AT_MAGIC, "$n melds into a nearby tree!", ch, NULL, NULL, TO_ROOM );
   else
      act( AT_MAGIC, "$n melds into the grass!", ch, NULL, NULL, TO_ROOM );

   leave_map( ch, victim, victim->in_room );

   if( ch->in_room->sector_type == SECT_FOREST )
      act( AT_MAGIC, "$n appears from behind a nearby tree!", ch, NULL, NULL, TO_ROOM );
   else
      act( AT_MAGIC, "$n grows up from the grass!", ch, NULL, NULL, TO_ROOM );

   return rNONE;
}

/* Scryn 2/2/96 */
SPELLF( spell_remove_invis )
{
   obj_data *obj;
   skill_type *skill = get_skilltype( sn );

   if( target_name.empty(  ) )
   {
      ch->print( "What should the spell be cast upon?\r\n" );
      return rSPELL_FAILED;
   }

   obj = ch->get_obj_carry( target_name );

   if( obj )
   {
      if( !obj->extra_flags.test( ITEM_INVIS ) )
      {
         ch->print( "Its not invisible!\r\n" );
         return rSPELL_FAILED;
      }
      obj->extra_flags.reset( ITEM_INVIS );
      act( AT_MAGIC, "$p becomes visible again.", ch, obj, NULL, TO_CHAR );

      ch->print( "Ok.\r\n" );
      return rNONE;
   }
   else
   {
      char_data *victim;

      victim = ch->get_char_room( target_name );

      if( victim )
      {
         if( !ch->can_see( victim, false ) )
         {
            ch->printf( "You don't see %s!\r\n", target_name.c_str(  ) );
            return rSPELL_FAILED;
         }

         if( !victim->has_aflag( AFF_INVISIBLE ) )
         {
            ch->print( "They are not invisible!\r\n" );
            return rSPELL_FAILED;
         }

         if( is_safe( ch, victim ) )
         {
            failed_casting( skill, ch, victim, NULL );
            return rSPELL_FAILED;
         }

         if( victim->has_immune( RIS_MAGIC ) )
         {
            immune_casting( skill, ch, victim, NULL );
            return rSPELL_FAILED;
         }
         if( !victim->isnpc(  ) )
         {
            if( ch->chance( 50 ) && ch->level + 10 < victim->level )
            {
               failed_casting( skill, ch, victim, NULL );
               return rSPELL_FAILED;
            }
            else
            {
               if( check_illegal_pk( ch, victim ) )
               {
                  ch->print( "You cannot cast that on another player!\r\n" );
                  return rSPELL_FAILED;
               }
            }
         }
         else
         {
            if( ch->chance( 50 ) && ch->level + 15 < victim->level )
            {
               failed_casting( skill, ch, victim, NULL );
               return rSPELL_FAILED;
            }
         }

         victim->affect_strip( gsn_invis );
         victim->affect_strip( gsn_mass_invis );
         victim->unset_aflag( AFF_INVISIBLE );
         successful_casting( skill, ch, victim, NULL );
         return rNONE;
      }
      ch->printf( "You can't find %s!\r\n", target_name.c_str(  ) );
      return rSPELL_FAILED;
   }
}

/* New Animate Dead by Whir 8/29/98
 * Original by Scryn and Altrag
 */
SPELLF( spell_animate_dead )
{
   char_data *mob;
   obj_data *corpse, *obj;
   list < obj_data * >::iterator iobj;
   bool found;
   mob_index *pMobIndex;
   string arg;
   skill_type *skill = get_skilltype( sn );  /* 4370 */
   const char *corpse_name = NULL;
   int sindex = -1;

   found = false;

   target_name = one_argument( target_name, arg );

   for( iobj = ch->in_room->objects.begin(  ); iobj != ch->in_room->objects.end(  ); ++iobj )
   {
      corpse = *iobj;

      if( corpse->item_type == ITEM_CORPSE_NPC && corpse->cost != -5 && is_same_obj_map( ch, corpse ) )
      {
         found = true;
         break;
      }
   }

   if( !found )
   {
      ch->print( "You can't find a suitable corpse.\r\n" );
      return rSPELL_FAILED;
   }

   if( !( pMobIndex = get_mob_index( corpse->value[4] ) ) )
   {
      bug( "%s: Can't find mob for value[4] of corpse", __FUNCTION__ );
      ch->print( "Ooops. Something didn't go quite right here....\r\n" );
      return rSPELL_FAILED;
   }

   if( !ch->can_charm(  ) )
   {
      ch->print( "You already have too many followers to support!\r\n" );
      return rSPELL_FAILED;
   }

   if( ch->mana - ( pMobIndex->level * 4 ) < 0 )
   {
      ch->print( "You don't have enough mana to raise this corpse.\r\n" );
      return rSPELL_FAILED;
   }
   else
      ch->mana -= ( pMobIndex->level * 4 );

   if( ch->is_immortal(  ) || ch->chance( 75 ) )
   {
      if( !str_cmp( arg, "skeleton" ) )
      {
         sindex = MOB_VNUM_ANIMATED_SKELETON;
         corpse_name = "skeleton";
      }
      else if( !str_cmp( arg, "zombie" ) )
      {
         sindex = MOB_VNUM_ANIMATED_ZOMBIE;
         corpse_name = "zombie";
      }
      else if( !str_cmp( arg, "ghoul" ) )
      {
         sindex = MOB_VNUM_ANIMATED_GHOUL;
         corpse_name = "ghoul";
      }
      else if( !str_cmp( arg, "crypt thing" ) )
      {
         sindex = MOB_VNUM_ANIMATED_CRYPT_THING;
         corpse_name = "crypt thing";
      }
      else if( !str_cmp( arg, "mummy" ) )
      {
         sindex = MOB_VNUM_ANIMATED_MUMMY;
         corpse_name = "mummy";
      }
      else if( !str_cmp( arg, "ghost" ) )
      {
         sindex = MOB_VNUM_ANIMATED_GHOST;
         corpse_name = "ghost";
      }
      else if( !str_cmp( arg, "death knight" ) )
      {
         sindex = MOB_VNUM_ANIMATED_DEATH_KNIGHT;
         corpse_name = "death knight";
      }
      else if( !str_cmp( arg, "dracolich" ) )
      {
         sindex = MOB_VNUM_ANIMATED_DRACOLICH;
         corpse_name = "dracolich";
      }
      if( sindex == -1 )
      {
         ch->print( "Turn it into WHAT?????\r\n" );
         return rSPELL_FAILED;
      }
      mob = get_mob_index( sindex )->create_mobile(  );

      /*
       * Bugfix by Tarl so only dragons become dracoliches. 29 July 2002 
       */
      if( !str_cmp( corpse_name, "dracolich" ) )
      {
         if( !IsDragon( mob ) )
         {
            ch->print( "Only dead dragons can become dracoliches.\r\n" );
            if( !mob->to_room( get_room_index( ROOM_VNUM_POLY ) ) )  /* Send to here to prevent bugs */
               log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
            return rSPELL_FAILED;
         }
      }

      if( corpse->level < mob->level )
      {
         ch->printf( "The spirit of this corpse is not powerful enough to become a %s.\r\n", corpse_name );
         if( !mob->to_room( get_room_index( ROOM_VNUM_POLY ) ) )  /* Send to here to prevent bugs */
            log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
         return rSPELL_FAILED;
      }

      if( ch->level < mob->level )
      {
         ch->printf( "You are not powerful enough to animate a %s yet.\r\n", corpse_name );
         if( !mob->to_room( get_room_index( ROOM_VNUM_POLY ) ) )  /* Send to here to prevent bugs */
            log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
         return rSPELL_FAILED;
      }

      if( !mob->to_room( ch->in_room ) )
         log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );

      act( AT_MAGIC, "$n makes $T rise from the grave!", ch, NULL, pMobIndex->short_descr, TO_ROOM );
      act( AT_MAGIC, "You make $T rise from the grave!", ch, NULL, pMobIndex->short_descr, TO_CHAR );

      stralloc_printf( &mob->name, "%s %s", corpse_name, pMobIndex->player_name );
      stralloc_printf( &mob->short_descr, "The %s", corpse_name );
      stralloc_printf( &mob->long_descr, "A %s struggles with the horror of its undeath.\r\n", corpse_name );
      bind_follower( mob, ch, sn, ( int )( number_fuzzy( ( int )( ( level + 1 ) / 4 ) + 1 ) * DUR_CONV ) );

      if( !corpse->contents.empty(  ) )
         for( iobj = corpse->contents.begin(  ); iobj != corpse->contents.end(  ); )
         {
            obj = ( *iobj );
            ++iobj;

            obj->from_obj(  );
            obj->to_room( corpse->in_room, mob );
         }

      corpse->separate(  );
      corpse->extract(  );
      return rNONE;
   }
   else
   {
      failed_casting( skill, ch, NULL, NULL );
      return rSPELL_FAILED;
   }
}

/* Ignores pickproofs, but can't unlock containers. -- Altrag 17/2/96 */
/* Oh ye of little faith Altrag.... it does now :) */
SPELLF( spell_knock )
{
   exit_data *pexit;
   obj_data *obj;

   ch->set_color( AT_MAGIC );
   /*
    * shouldn't know why it didn't work, and shouldn't work on pickproof
    * exits.  -Thoric
    */
   /*
    * agreed, it shouldn't work on pickproof anything - but explaining why
    * * a spell failed won't hurt you any Thoric :)
    */

   /*
    * Checks exit keywords first - Samson 
    */
   if( ( pexit = find_door( ch, target_name, true ) ) )
   {
      if( !IS_EXIT_FLAG( pexit, EX_CLOSED ) )
      {
         ch->printf( "The %s is already open.\r\n", pexit->keyword );
         return rSPELL_FAILED;
      }
      if( !IS_EXIT_FLAG( pexit, EX_LOCKED ) )
      {
         ch->printf( "The %s is not locked.\r\n", pexit->keyword );
         return rSPELL_FAILED;
      }
      if( IS_EXIT_FLAG( pexit, EX_PICKPROOF ) )
      {
         ch->print( "The lock is too strong to be forced open.\r\n" );
         return rSPELL_FAILED;
      }
      REMOVE_EXIT_FLAG( pexit, EX_LOCKED );
      ch->print( "*Click*\r\n" );
      if( pexit->rexit && pexit->rexit->to_room == ch->in_room )
         REMOVE_EXIT_FLAG( pexit->rexit, EX_LOCKED );
      check_room_for_traps( ch, TRAP_UNLOCK | trap_door[pexit->vdir] );
      return rNONE;
   }

   /*
    * Then checks objects in the room to see if there's a match - Samson 
    */
   if( ( obj = ch->get_obj_here( target_name ) ) )
   {
      if( obj->item_type != ITEM_CONTAINER )
      {
         ch->printf( "%s is not even a container!\r\n", obj->short_descr );
         return rSPELL_FAILED;
      }
      if( !IS_SET( obj->value[1], CONT_CLOSED ) )
      {
         ch->print( "It's not closed.\r\n" );
         return rSPELL_FAILED;
      }
      if( !IS_SET( obj->value[1], CONT_LOCKED ) )
      {
         ch->print( "It's already unlocked.\r\n" );
         return rSPELL_FAILED;
      }
      if( IS_SET( obj->value[1], CONT_PICKPROOF ) )
      {
         ch->print( "The lock is too strong to be forced open.\r\n" );
         return rSPELL_FAILED;
      }
      REMOVE_BIT( obj->value[1], CONT_LOCKED );
      ch->print( "*Click*\r\n" );
      act( AT_MAGIC, "$n magically unlocks $p.", ch, obj, NULL, TO_ROOM );
      /*
       * Need to figure out how to check for traps here 
       */
      return rNONE;
   }
   ch->print( "What were you trying to knock?\r\n" );
   return rSPELL_FAILED;
}

/* Tells to sleepers in are. -- Altrag 17/2/96 */
SPELLF( spell_dream )
{
   char_data *victim;
   string arg;

   target_name = one_argument( target_name, arg );
   ch->set_color( AT_MAGIC );

   if( !( victim = ch->get_char_world( arg ) ) || victim->in_room->area != ch->in_room->area )
   {
      ch->print( "They aren't here.\r\n" );
      return rSPELL_FAILED;
   }

   if( victim->position != POS_SLEEPING )
   {
      ch->print( "They aren't asleep.\r\n" );
      return rSPELL_FAILED;
   }

   if( target_name.empty(  ) )
   {
      ch->print( "What do you want them to dream about?\r\n" );
      return rSPELL_FAILED;
   }
   victim->printf( "&[tell]You have dreams about %s telling you '%s'.\r\n", PERS( ch, victim, false ), target_name.c_str(  ) );
   successful_casting( get_skilltype( sn ), ch, victim, NULL );
   return rNONE;
}

/* MUST BE KEPT!!! Used by attack types for mobiles!! */
SPELLF( spell_spiral_blast )
{
   char_data *victim = ( char_data * ) vo;
   int dam;
   bool ch_died;

   ch_died = false;

   if( !( victim = ch->get_char_room( target_name ) ) )
   {
      ch->print( "They aren't here.\r\n" );
      return rSPELL_FAILED;
   }

   if( is_safe( ch, victim ) )
      return rSPELL_FAILED;

   if( ch->in_room->flags.test( ROOM_SAFE ) )
   {
      ch->print( "&[magic]You fail to breathe.\r\n" );
      return rNONE;
   }

   act( AT_MAGIC, "Swirling colours radiate from $n, encompassing $N.", ch, ch, victim, TO_ROOM );
   act( AT_MAGIC, "Swirling colours radiate from you, encompassing $N.", ch, ch, victim, TO_CHAR );

   dam = dice( level, 9 );
   if( saves_breath( level, victim ) )
      dam /= 2;
   if( damage( ch, victim, dam, sn ) == rCHAR_DIED || ch->char_died(  ) )
      ch_died = true;

   if( ch_died )
      return rCHAR_DIED;
   else
      return rNONE;
}

   /*******************************************************
	 * Everything after this point is part of SMAUG SPELLS *
	 *******************************************************/

/*
 * saving throw check						-Thoric
 */
bool check_save( int sn, int level, char_data * ch, char_data * victim )
{
   skill_type *skill = get_skilltype( sn );
   bool saved = false;

   if( SPELL_FLAG( skill, SF_PKSENSITIVE ) && !ch->isnpc(  ) && !victim->isnpc(  ) )
      level /= 2;

   if( skill->saves )
      switch ( skill->saves )
      {
         default:
            break;
         case SS_POISON_DEATH:
            saved = saves_poison_death( level, victim );
            break;
         case SS_ROD_WANDS:
            saved = saves_wands( level, victim );
            break;
         case SS_PARA_PETRI:
            saved = saves_para_petri( level, victim );
            break;
         case SS_BREATH:
            saved = saves_breath( level, victim );
            break;
         case SS_SPELL_STAFF:
            saved = saves_spell_staff( level, victim );
            break;
      }
   return saved;
}

SPELLF( spell_affectchar )
{
   affect_data af;
   smaug_affect *saf;
   list < smaug_affect * >::iterator saff;
   skill_type *skill = get_skilltype( sn );
   char_data *victim = ( char_data * ) vo;
   int schance;
   bool affected = false, first = true;
   ch_ret retcode = rNONE;

   if( SPELL_FLAG( skill, SF_RECASTABLE ) )
      victim->affect_strip( sn );
   for( saff = skill->affects.begin(  ); saff != skill->affects.end(  ); ++saff )
   {
      saf = *saff;

      if( saf->location >= REVERSE_APPLY )
      {
         if( !SPELL_FLAG( skill, SF_ACCUMULATIVE ) )
         {
            if( first == true )
            {
               if( SPELL_FLAG( skill, SF_RECASTABLE ) )
                  ch->affect_strip( sn );
               if( ch->is_affected( sn ) )
                  affected = true;
            }
            first = false;
            if( affected == true )
               continue;
         }
         victim = ch;
      }
      else
         victim = ( char_data * ) vo;

      /*
       * Check if char has this bitvector already 
       */
      af.bit = saf->bit;
      if( saf->bit >= 0 && victim->has_aflag( saf->bit ) && !SPELL_FLAG( skill, SF_ACCUMULATIVE ) )
         continue;

      /*
       * necessary for affect_strip to work properly...
       */
      switch ( saf->bit )
      {
         default:
            af.type = sn;
            break;
         case AFF_POISON:
            af.type = gsn_poison;
            schance = ris_save( victim, level, RIS_POISON );
            if( schance == 1000 )
            {
               retcode = rVICT_IMMUNE;
               if( SPELL_FLAG( skill, SF_STOPONFAIL ) )
                  return retcode;
               continue;
            }
            if( saves_poison_death( schance, victim ) )
            {
               if( SPELL_FLAG( skill, SF_STOPONFAIL ) )
                  return retcode;
               continue;
            }
            victim->mental_state = URANGE( 30, victim->mental_state + 2, 100 );
            break;

         case AFF_BLIND:
            af.type = gsn_blindness;
            break;
         case AFF_CURSE:
            af.type = gsn_curse;
            break;
         case AFF_INVISIBLE:
            af.type = gsn_invis;
            break;
         case AFF_SLEEP:
            af.type = gsn_sleep;
            schance = ris_save( victim, level, RIS_SLEEP );
            if( schance == 1000 )
            {
               retcode = rVICT_IMMUNE;
               if( SPELL_FLAG( skill, SF_STOPONFAIL ) )
                  return retcode;
               continue;
            }
            break;

         case AFF_CHARM:
            af.type = gsn_charm_person;
            schance = ris_save( victim, level, RIS_CHARM );
            if( schance == 1000 )
            {
               retcode = rVICT_IMMUNE;
               if( SPELL_FLAG( skill, SF_STOPONFAIL ) )
                  return retcode;
               continue;
            }
            break;

         case AFF_PARALYSIS:
            af.type = gsn_paralyze;
            schance = ris_save( victim, level, RIS_PARALYSIS );
            if( schance == 1000 )
            {
               retcode = rVICT_IMMUNE;
               if( SPELL_FLAG( skill, SF_STOPONFAIL ) )
                  return retcode;
               continue;
            }
            break;
      }
      {
         int tmp = dice_parse( ch, level, saf->duration );
         af.duration = UMIN( tmp, 32700 );
      }
      if( saf->location == APPLY_AFFECT || saf->location == APPLY_EXT_AFFECT )
      {
         af.modifier = saf->bit;
      }
      else if( saf->location == APPLY_RESISTANT || saf->location == APPLY_IMMUNE || saf->location == APPLY_ABSORB || saf->location == APPLY_SUSCEPTIBLE )
      {
         af.modifier = get_risflag( saf->modifier );
      }
      else
         af.modifier = dice_parse( ch, level, saf->modifier );
      af.location = saf->location % REVERSE_APPLY;
      if( af.duration == 0 )
      {
         switch ( af.location )
         {
            case APPLY_HIT:
               victim->hit = URANGE( 0, victim->hit + af.modifier, victim->max_hit );
               victim->update_pos(  );
               if( victim->isnpc(  ) && victim->hit <= 0 )
                  damage( ch, victim, 5, TYPE_UNDEFINED );
               break;
            case APPLY_MANA:
               victim->mana = URANGE( 0, victim->mana + af.modifier, victim->max_mana );
               victim->update_pos(  );
               break;
            case APPLY_MOVE:
               victim->move = URANGE( 0, victim->move + af.modifier, victim->max_move );
               victim->update_pos(  );
               break;
            default:
               victim->affect_modify( &af, true );
               break;
         }
      }
      else if( SPELL_FLAG( skill, SF_ACCUMULATIVE ) )
         victim->affect_join( &af );
      else
         victim->affect_to_char( &af );
   }
   victim->update_pos(  );
   return retcode;
}

/*
 * Generic offensive spell damage attack - Thoric
 */
SPELLF( spell_attack )
{
   char_data *victim = ( char_data * ) vo;
   skill_type *skill = get_skilltype( sn );
   bool saved = check_save( sn, level, ch, victim );
   int dam;
   ch_ret retcode = rNONE;

   if( saved && SPELL_SAVE( skill ) == SE_NEGATE )
   {
      failed_casting( skill, ch, victim, NULL );
      return rSPELL_FAILED;
   }
   if( skill->dice )
      dam = UMAX( 0, dice_parse( ch, level, skill->dice ) );
   else
      dam = dice( 1, level / 2 );
   if( saved )
   {
      switch ( SPELL_SAVE( skill ) )
      {
         default:
            break;
         case SE_3QTRDAM:
            dam = ( dam * 3 ) / 4;
            break;
         case SE_HALFDAM:
            dam >>= 1;
            break;
         case SE_QUARTERDAM:
            dam >>= 2;
            break;
         case SE_EIGHTHDAM:
            dam >>= 3;
            break;

         case SE_ABSORB:  /* victim absorbs spell for hp's */
            act( AT_MAGIC, "$N absorbs your $t!", ch, skill->noun_damage, victim, TO_CHAR );
            act( AT_MAGIC, "You absorb $N's $t!", victim, skill->noun_damage, ch, TO_CHAR );
            act( AT_MAGIC, "$N absorbs $n's $t!", ch, skill->noun_damage, victim, TO_NOTVICT );
            victim->hit = URANGE( 0, victim->hit + dam, victim->max_hit );
            victim->update_pos(  );
            if( !skill->affects.empty(  ) )
               retcode = spell_affectchar( sn, level, ch, victim );
            return retcode;

         case SE_REFLECT: /* reflect the spell to the caster */
            return spell_attack( sn, level, victim, ch );
      }
   }
   retcode = damage( ch, victim, dam, sn );
   if( retcode == rNONE && !skill->affects.empty(  ) && !ch->char_died(  ) && !victim->char_died(  )
       && ( !victim->is_affected( sn ) || SPELL_FLAG( skill, SF_ACCUMULATIVE ) || SPELL_FLAG( skill, SF_RECASTABLE ) ) )
      retcode = spell_affectchar( sn, level, ch, victim );
   return retcode;
}

/*
 * Generic area attack - Thoric
 */
SPELLF( spell_area_attack )
{
   ch_ret retcode = rNONE;
   list < char_data * >::iterator ich;
   skill_type *skill = get_skilltype( sn );

   if( ch->in_room->flags.test( ROOM_SAFE ) )
   {
      failed_casting( skill, ch, NULL, NULL );
      return rSPELL_FAILED;
   }

   bool affects = ( !skill->affects.empty(  )? true : false );
   if( skill->hit_char && skill->hit_char[0] != '\0' )
      act( AT_MAGIC, skill->hit_char, ch, NULL, NULL, TO_CHAR );
   if( skill->hit_room && skill->hit_room[0] != '\0' )
      act( AT_MAGIC, skill->hit_room, ch, NULL, NULL, TO_ROOM );

   for( ich = ch->in_room->people.begin(  ); ich != ch->in_room->people.end(  ); )
   {
      char_data *vch = *ich;
      ++ich;

      if( vch->has_pcflag( PCFLAG_WIZINVIS ) && vch->pcdata->wizinvis >= LEVEL_IMMORTAL )
         continue;

      /*
       * Verify they're in the same spot 
       */
      if( vch == ch || !is_same_char_map( vch, ch ) )
         continue;

      if( ch->isnpc(  )? !vch->isnpc(  ) : vch->isnpc(  ) )
      {
         int dam;
         bool saved = check_save( sn, level, ch, vch );

         if( saved && SPELL_SAVE( skill ) == SE_NEGATE )
         {
            failed_casting( skill, ch, vch, NULL );
            continue;
         }
         else if( skill->dice )
            dam = dice_parse( ch, level, skill->dice );
         else
            dam = dice( 1, level / 2 );
         if( saved )
         {
            switch ( SPELL_SAVE( skill ) )
            {
               default:
                  break;
               case SE_3QTRDAM:
                  dam = ( dam * 3 ) / 4;
                  break;
               case SE_HALFDAM:
                  dam >>= 1;
                  break;
               case SE_QUARTERDAM:
                  dam >>= 2;
                  break;
               case SE_EIGHTHDAM:
                  dam >>= 3;
                  break;

               case SE_ABSORB:  /* victim absorbs spell for hp's */
                  act( AT_MAGIC, "$N absorbs your $t!", ch, skill->noun_damage, vch, TO_CHAR );
                  act( AT_MAGIC, "You absorb $N's $t!", vch, skill->noun_damage, ch, TO_CHAR );
                  act( AT_MAGIC, "$N absorbs $n's $t!", ch, skill->noun_damage, vch, TO_NOTVICT );
                  vch->hit = URANGE( 0, vch->hit + dam, vch->max_hit );
                  vch->update_pos(  );
                  continue;

               case SE_REFLECT: /* reflect the spell to the caster */
                  retcode = spell_attack( sn, level, vch, ch );
                  if( ch->char_died(  ) )
                  {
                     break;
                  }
                  continue;
            }
         }
         if( ch->in_room->flags.test( ROOM_CAVE ) && vch == ch )
            dam = dam / 2;
         if( ch->in_room->flags.test( ROOM_CAVERN ) && vch == ch )
            dam = 0;

         retcode = damage( ch, vch, dam, sn );
      }
      if( retcode == rNONE && affects && !ch->char_died(  ) && !vch->char_died(  )
          && ( !vch->is_affected( sn ) || SPELL_FLAG( skill, SF_ACCUMULATIVE ) || SPELL_FLAG( skill, SF_RECASTABLE ) ) )
         retcode = spell_affectchar( sn, level, ch, vch );
      if( retcode == rCHAR_DIED || ch->char_died(  ) )
      {
         break;
      }
   }
   return retcode;
}

/*
 * Generic spell affect - Thoric
 */
SPELLF( spell_affect )
{
   skill_type *skill = get_skilltype( sn );
   char_data *victim = ( char_data * ) vo;
   bool groupsp, areasp, hitchar = false, hitroom = false, hitvict = false;
   ch_ret retcode;

   if( skill->affects.empty(  ) )
   {
      bug( "%s: spell_affect has no affects sn %d", __FUNCTION__, sn );
      return rNONE;
   }
   if( SPELL_FLAG( skill, SF_GROUPSPELL ) )
      groupsp = true;
   else
      groupsp = false;

   if( SPELL_FLAG( skill, SF_AREA ) )
      areasp = true;
   else
      areasp = false;

   if( !groupsp && !areasp )
   {
      /*
       * Can't find a victim 
       */
      if( !victim )
      {
         failed_casting( skill, ch, victim, NULL );
         return rSPELL_FAILED;
      }

      if( ( skill->type != SKILL_HERB && victim->has_immune( RIS_MAGIC ) ) || is_immune( victim, SPELL_DAMAGE( skill ) ) )
      {
         immune_casting( skill, ch, victim, NULL );
         return rSPELL_FAILED;
      }

      /*
       * Spell is already on this guy 
       */
      if( victim->is_affected( sn ) && !SPELL_FLAG( skill, SF_ACCUMULATIVE ) && !SPELL_FLAG( skill, SF_RECASTABLE ) )
      {
         failed_casting( skill, ch, victim, NULL );
         return rSPELL_FAILED;
      }

      if( skill->affects.size(  ) == 1 )
      {
         smaug_affect *saf = ( *skill->affects.begin(  ) );

         if( saf->location == APPLY_STRIPSN && !victim->is_affected( dice_parse( ch, level, saf->modifier ) ) )
         {
            failed_casting( skill, ch, victim, NULL );
            return rSPELL_FAILED;
         }
      }

      if( check_save( sn, level, ch, victim ) )
      {
         failed_casting( skill, ch, victim, NULL );
         return rSPELL_FAILED;
      }
   }
   else
   {
      if( skill->hit_char && skill->hit_char[0] != '\0' )
      {
         if( strstr( skill->hit_char, "$N" ) )
            hitchar = true;
         else
            act( AT_MAGIC, skill->hit_char, ch, NULL, NULL, TO_CHAR );
      }
      if( skill->hit_room && skill->hit_room[0] != '\0' )
      {
         if( strstr( skill->hit_room, "$N" ) )
            hitroom = true;
         else
            act( AT_MAGIC, skill->hit_room, ch, NULL, NULL, TO_ROOM );
      }
      if( skill->hit_vict && skill->hit_vict[0] != '\0' )
         hitvict = true;
      if( victim )
         victim = ( *victim->in_room->people.begin(  ) );
      else
         victim = ( *ch->in_room->people.begin(  ) );
   }
   if( !victim )
   {
      bug( "%s: could not find victim: sn %d", __FUNCTION__, sn );
      failed_casting( skill, ch, victim, NULL );
      return rSPELL_FAILED;
   }

   if( !groupsp && !areasp )
   {
      retcode = spell_affectchar( sn, level, ch, victim );
      if( retcode == rVICT_IMMUNE )
         immune_casting( skill, ch, victim, NULL );
      else
         successful_casting( skill, ch, victim, NULL );
      return rNONE;
   }

   list < char_data * >::iterator ich;
   for( ich = victim->in_room->people.begin(  ); ich != victim->in_room->people.end(  ); ++ich )
   {
      char_data *vch = *ich;

      if( groupsp || areasp )
      {
         if( ( groupsp && !is_same_group( vch, ch ) ) || vch->has_immune( RIS_MAGIC )
             || is_immune( vch, SPELL_DAMAGE( skill ) ) || check_save( sn, level, ch, vch ) || ( !SPELL_FLAG( skill, SF_RECASTABLE ) && vch->is_affected( sn ) ) )
            continue;

         if( hitvict && ch != vch )
         {
            act( AT_MAGIC, skill->hit_vict, ch, NULL, vch, TO_VICT );
            if( hitroom )
            {
               act( AT_MAGIC, skill->hit_room, ch, NULL, vch, TO_NOTVICT );
               act( AT_MAGIC, skill->hit_room, ch, NULL, vch, TO_CHAR );
            }
         }
         else if( hitroom )
            act( AT_MAGIC, skill->hit_room, ch, NULL, vch, TO_ROOM );
         if( ch == vch )
         {
            if( hitvict )
               act( AT_MAGIC, skill->hit_vict, ch, NULL, ch, TO_CHAR );
            else if( hitchar )
               act( AT_MAGIC, skill->hit_char, ch, NULL, ch, TO_CHAR );
         }
         else if( hitchar )
            act( AT_MAGIC, skill->hit_char, ch, NULL, vch, TO_CHAR );
      }
      retcode = spell_affectchar( sn, level, ch, vch );
   }
   return rNONE;
}

/*
 * Generic inventory object spell - Thoric
 */
SPELLF( spell_obj_inv )
{
   obj_data *obj = ( obj_data * ) vo;
   liquid_data *liq = NULL;
   skill_type *skill = get_skilltype( sn );

   if( !obj )
   {
      failed_casting( skill, ch, NULL, NULL );
      return rNONE;
   }

   switch ( SPELL_ACTION( skill ) )
   {
      default:
      case SA_NONE:
         return rNONE;

      case SA_CREATE:
         if( SPELL_FLAG( skill, SF_WATER ) ) /* create water */
         {
            int water;
            weather_data *weath = ch->in_room->area->weather;

            if( obj->item_type != ITEM_DRINK_CON )
            {
               ch->print( "It is unable to hold water.\r\n" );
               return rSPELL_FAILED;
            }

            if( !( liq = get_liq_vnum( obj->value[2] ) ) )
            {
               bug( "%s: bad liquid number %d", __FUNCTION__, obj->value[2] );
               ch->print( "There's already another liquid in the container.\r\n" );
               return rSPELL_FAILED;
            }

            if( str_cmp( liq->name, "water" ) && obj->value[1] != 0 )
            {
               ch->print( "There's already another liquid in the container.\r\n" );
               return rSPELL_FAILED;
            }

            water = UMIN( ( skill->dice ? dice_parse( ch, level, skill->dice ) : level ) * ( weath->precip >= 0 ? 2 : 1 ), obj->value[0] - obj->value[1] );

            if( water > 0 )
            {
               obj->separate(  );
               obj->value[2] = liq->vnum;
               obj->value[1] += water;

               /*
                * Don't overfill the container! 
                */
               if( obj->value[1] > obj->value[0] )
                  obj->value[1] = obj->value[0];

               if( !hasname( obj->name, "water" ) )
                  stralloc_printf( &obj->name, "%s water", obj->name );
            }
            successful_casting( skill, ch, NULL, obj );
            return rNONE;
         }
         if( SPELL_DAMAGE( skill ) == SD_FIRE ) /* burn object */
         {
            /*
             * return rNONE; 
             */
         }
         if( SPELL_DAMAGE( skill ) == SD_POISON || SPELL_CLASS( skill ) == SC_DEATH )  /* poison object */
         {
            switch ( obj->item_type )
            {
               default:
                  failed_casting( skill, ch, NULL, obj );
                  break;
               case ITEM_COOK:
               case ITEM_FOOD:
               case ITEM_DRINK_CON:
                  obj->separate(  );
                  obj->value[3] = 1;
                  successful_casting( skill, ch, NULL, obj );
                  break;
            }
            return rNONE;
         }
         if( SPELL_CLASS( skill ) == SC_LIFE /* purify food/water */
             && ( obj->item_type == ITEM_FOOD || obj->item_type == ITEM_DRINK_CON || obj->item_type == ITEM_COOK ) )
         {
            switch ( obj->item_type )
            {
               default:
                  failed_casting( skill, ch, NULL, obj );
                  break;
               case ITEM_COOK:
               case ITEM_FOOD:
               case ITEM_DRINK_CON:
                  obj->separate(  );
                  obj->value[3] = 0;
                  successful_casting( skill, ch, NULL, obj );
                  break;
            }
            return rNONE;
         }

         if( SPELL_CLASS( skill ) != SC_NONE )
         {
            failed_casting( skill, ch, NULL, obj );
            return rNONE;
         }
         switch ( SPELL_POWER( skill ) )  /* clone object */
         {
               obj_data *clone;

            default:
            case SP_NONE:
               if( ch->level - obj->level < 10 || obj->cost > ch->level * ch->get_curr_int(  ) * ch->get_curr_wis(  ) )
               {
                  failed_casting( skill, ch, NULL, obj );
                  return rNONE;
               }
               break;
            case SP_MINOR:
               if( ch->level - obj->level < 20 || obj->cost > ch->level * ch->get_curr_int(  ) / 5 )
               {
                  failed_casting( skill, ch, NULL, obj );
                  return rNONE;
               }
               break;
            case SP_GREATER:
               if( ch->level - obj->level < 5 || obj->cost > ch->level * 10 * ch->get_curr_int(  ) * ch->get_curr_wis(  ) )
               {
                  failed_casting( skill, ch, NULL, obj );
                  return rNONE;
               }
               break;
            case SP_MAJOR:
               if( ch->level - obj->level < 0 || obj->cost > ch->level * 50 * ch->get_curr_int(  ) * ch->get_curr_wis(  ) )
               {
                  failed_casting( skill, ch, NULL, obj );
                  return rNONE;
               }
               clone = obj->clone(  );
               clone->timer = skill->dice ? dice_parse( ch, level, skill->dice ) : 0;
               clone->to_char( ch );
               successful_casting( skill, ch, NULL, obj );
               break;
         }
         return rNONE;

      case SA_DESTROY:
      case SA_RESIST:
      case SA_SUSCEPT:
      case SA_DIVINATE:
         if( SPELL_DAMAGE( skill ) == SD_POISON )  /* detect poison */
         {
            if( obj->item_type == ITEM_DRINK_CON || obj->item_type == ITEM_FOOD || obj->item_type == ITEM_COOK )
            {
               if( obj->item_type == ITEM_COOK && obj->value[2] == 0 )
                  ch->print( "It looks undercooked.\r\n" );
               else if( obj->value[3] != 0 )
                  ch->print( "You smell poisonous fumes.\r\n" );
               else
                  ch->print( "It looks very delicious.\r\n" );
            }
            else
               ch->print( "It doesn't look poisoned.\r\n" );
            return rNONE;
         }
         return rNONE;
      case SA_OBSCURE: /* make obj invis */
         if( obj->extra_flags.test( ITEM_INVIS ) || ch->chance( skill->dice ? dice_parse( ch, level, skill->dice ) : 20 ) )
         {
            failed_casting( skill, ch, NULL, NULL );
            return rSPELL_FAILED;
         }
         successful_casting( skill, ch, NULL, obj );
         obj->extra_flags.set( ITEM_INVIS );
         return rNONE;

      case SA_CHANGE:
         return rNONE;
   }
}

/*
 * Generic object creating spell				-Thoric
 */
SPELLF( spell_create_obj )
{
   skill_type *skill = get_skilltype( sn );
   int lvl, vnum = skill->value;
   obj_data *obj;

   switch ( SPELL_POWER( skill ) )
   {
      default:
      case SP_NONE:
         lvl = 10;
         break;
      case SP_MINOR:
         lvl = 0;
         break;
      case SP_GREATER:
         lvl = level / 2;
         break;
      case SP_MAJOR:
         lvl = level;
         break;
   }

   if( !( obj = get_obj_index( vnum )->create_object( lvl ) ) )
   {
      log_printf( "create_object: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
      failed_casting( skill, ch, NULL, NULL );
      return rNONE;
   }
   obj->timer = skill->dice ? dice_parse( ch, level, skill->dice ) : 0;
   successful_casting( skill, ch, NULL, obj );
   if( obj->wear_flags.test( ITEM_TAKE ) )
      obj->to_char( ch );
   else
      obj->to_room( ch->in_room, ch );
   return rNONE;
}

/*
 * Generic mob creating spell					-Thoric
 */
SPELLF( spell_create_mob )
{
   skill_type *skill = get_skilltype( sn );
   int lvl, vnum = skill->value;
   char_data *mob;
   mob_index *mi;
   affect_data af;

   /*
    * set maximum mob level 
    */
   switch ( SPELL_POWER( skill ) )
   {
      default:
      case SP_NONE:
         lvl = 20;
         break;
      case SP_MINOR:
         lvl = 5;
         break;
      case SP_GREATER:
         lvl = level / 2;
         break;
      case SP_MAJOR:
         lvl = level;
         break;
   }

   /*
    * Add predetermined mobiles here
    */
   if( vnum == 0 )
   {
      if( !str_cmp( target_name, "cityguard" ) )
         vnum = MOB_VNUM_CITYGUARD;
   }

   if( !( mi = get_mob_index( vnum ) ) || ( mob = mi->create_mobile(  ) ) )
   {
      failed_casting( skill, ch, NULL, NULL );
      return rNONE;
   }
   mob->level = UMIN( lvl, skill->dice ? dice_parse( ch, level, skill->dice ) : mob->level );
   mob->armor = interpolate( mob->level, 100, -100 );

   mob->max_hit = mob->level * 8 + number_range( mob->level * mob->level / 4, mob->level * mob->level );
   mob->hit = mob->max_hit;
   mob->gold = 0;
   successful_casting( skill, ch, mob, NULL );
   if( !mob->to_room( ch->in_room ) )
      log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
   add_follower( mob, ch );
   af.type = sn;
   af.duration = ( int )( number_fuzzy( ( int )( ( level + 1 ) / 3 ) + 1 ) * DUR_CONV );
   af.location = 0;
   af.modifier = 0;
   af.bit = AFF_CHARM;
   mob->affect_to_char( &af );
   return rNONE;
}

/*
 * Generic handler for new "SMAUG" spells			-Thoric
 */
SPELLF( spell_smaug )
{
   struct skill_type *skill = get_skilltype( sn );

   /*
    * Put this check in to prevent crashes from this getting a bad skill 
    */
   if( !skill )
   {
      bug( "%s: Called with a null skill for sn %d", __FUNCTION__, sn );
      return rERROR;
   }

   switch ( skill->target )
   {
      default:
         return rNONE;

      case TAR_IGNORE:
         if( SPELL_FLAG( skill, SF_NOMOUNT ) )
         {
            ch->print( "You can't do that while mounted.\r\n" );
            return rNONE;
         }

         /*
          * offensive area spell 
          */
         if( SPELL_FLAG( skill, SF_AREA )
             && ( ( SPELL_ACTION( skill ) == SA_DESTROY && SPELL_CLASS( skill ) == SC_LIFE ) || ( SPELL_ACTION( skill ) == SA_CREATE && SPELL_CLASS( skill ) == SC_DEATH ) ) )
            return spell_area_attack( sn, level, ch, vo );

         if( SPELL_ACTION( skill ) == SA_CREATE )
         {
            if( SPELL_FLAG( skill, SF_OBJECT ) )   /* create object */
               return spell_create_obj( sn, level, ch, vo );
            if( SPELL_CLASS( skill ) == SC_LIFE )  /* create mob */
               return spell_create_mob( sn, level, ch, vo );
         }

         /*
          * affect a distant player 
          */
         if( SPELL_FLAG( skill, SF_DISTANT ) && SPELL_FLAG( skill, SF_CHARACTER ) )
            return spell_affect( sn, level, ch, ch->get_char_world( target_name ) );

         /*
          * affect a player in this room (should have been TAR_CHAR_XXX) 
          */
         if( SPELL_FLAG( skill, SF_CHARACTER ) )
            return spell_affect( sn, level, ch, ch->get_char_room( target_name ) );

         if( skill->range > 0 && ( ( SPELL_ACTION( skill ) == SA_DESTROY && SPELL_CLASS( skill ) == SC_LIFE )
                                   || ( SPELL_ACTION( skill ) == SA_CREATE && SPELL_CLASS( skill ) == SC_DEATH ) ) )
            return ranged_attack( ch, ranged_target_name, NULL, NULL, sn, skill->range );

         /*
          * will fail, or be an area/group affect 
          */
         return spell_affect( sn, level, ch, vo );

      case TAR_CHAR_OFFENSIVE:
         if( SPELL_FLAG( skill, SF_NOMOUNT ) )
         {
            ch->print( "You can't do that while mounted.\r\n" );
            return rNONE;
         }

         /*
          * a regular damage inflicting spell attack 
          */
         if( ( SPELL_ACTION( skill ) == SA_DESTROY && SPELL_CLASS( skill ) == SC_LIFE ) || ( SPELL_ACTION( skill ) == SA_CREATE && SPELL_CLASS( skill ) == SC_DEATH ) )
            return spell_attack( sn, level, ch, vo );

         /*
          * a nasty spell affect 
          */
         return spell_affect( sn, level, ch, vo );

      case TAR_CHAR_DEFENSIVE:
      case TAR_CHAR_SELF:
         if( SPELL_FLAG( skill, SF_NOFIGHT ) && ch->position > POS_SITTING && ch->position < POS_STANDING )
         {
            ch->print( "You can't concentrate enough for that!\r\n" );
            return rNONE;
         }

         if( SPELL_FLAG( skill, SF_NOMOUNT ) )
         {
            ch->print( "You can't do that while mounted.\r\n" );
            return rNONE;
         }

         if( vo && SPELL_ACTION( skill ) == SA_DESTROY )
         {
            char_data *victim = ( char_data * ) vo;

            /*
             * cure poison 
             */
            if( SPELL_DAMAGE( skill ) == SD_POISON )
            {
               if( victim->is_affected( gsn_poison ) )
               {
                  victim->affect_strip( gsn_poison );
                  victim->mental_state = URANGE( -100, victim->mental_state, -10 );
                  successful_casting( skill, ch, victim, NULL );
                  return rNONE;
               }
               failed_casting( skill, ch, victim, NULL );
               return rSPELL_FAILED;
            }
            /*
             * cure blindness 
             */
            if( SPELL_CLASS( skill ) == SC_ILLUSION )
            {
               if( victim->is_affected( gsn_blindness ) )
               {
                  victim->affect_strip( gsn_blindness );
                  successful_casting( skill, ch, victim, NULL );
                  return rNONE;
               }
               failed_casting( skill, ch, victim, NULL );
               return rSPELL_FAILED;
            }
         }
         return spell_affect( sn, level, ch, vo );

      case TAR_OBJ_INV:
         if( SPELL_FLAG( skill, SF_NOMOUNT ) )
         {
            ch->print( "You can't do that while mounted.\r\n" );
            return rNONE;
         }
         return spell_obj_inv( sn, level, ch, vo );
   }
}

/* Everything from here down has been added by Alsherok */

SPELLF( spell_treespeak )
{
   affect_data af;

   af.type = skill_lookup( "treetalk" );
   af.duration = 93; /* One hour long */
   af.modifier = 0;
   af.location = APPLY_NONE;
   af.bit = AFF_TREETALK;
   ch->affect_to_char( &af );

   ch->print( "&[magic]You enter an altered state and are now speaking the language of trees.\r\n" );
   return rNONE;
}

SPELLF( spell_tree_transport )
{
   room_index *target;
   obj_data *obj, *tree = NULL;
   list < obj_data * >::iterator iobj;
   bool found = false;

   for( iobj = ch->in_room->objects.begin(  ); iobj != ch->in_room->objects.end(  ); ++iobj )
   {
      tree = *iobj;
      if( tree->item_type == ITEM_TREE )
      {
         if( is_same_obj_map( ch, tree ) )
         {
            found = true;
            break;
         }
      }
   }

   if( !found )
   {
      ch->print( "You need to have a tree nearby.\r\n" );
      return rSPELL_FAILED;
   }

   if( !( obj = ch->get_obj_world( target_name ) ) )
   {
      ch->print( "No trees exist in the world with that name.\r\n" );
      return rSPELL_FAILED;
   }

   if( obj->item_type != ITEM_TREE )
   {
      ch->print( "That's not a tree!\r\n" );
      return rSPELL_FAILED;
   }

   if( !obj->in_room )
   {
      ch->print( "That tree is nowhere to be found.\r\n" );
      return rSPELL_FAILED;
   }

   target = obj->in_room;

   if( !ch->has_visited( target->area ) )
   {
      ch->print( "You have not visited that area yet!\r\n" );
      return rSPELL_FAILED;
   }

   act( AT_MAGIC, "$n touches $p, and slowly vanishes within!", ch, tree, NULL, TO_ROOM );
   act( AT_MAGIC, "You touch $p, and join your forms.", ch, tree, NULL, TO_CHAR );

   /*
    * Need to explicitly set coordinates and map information with objects 
    */
   leave_map( ch, NULL, target );
   ch->cmap = obj->cmap;
   ch->mx = obj->mx;
   ch->my = obj->my;

   act( AT_MAGIC, "$p rustles slightly, and $n magically steps from within!", ch, obj, NULL, TO_ROOM );
   act( AT_MAGIC, "You are instantly transported to $p!", ch, obj, NULL, TO_CHAR );
   return rNONE;
}

SPELLF( spell_group_towngate )
{
   if( target_name.empty(  ) )
   {
      ch->print( "Where do you wish to go??\r\n" );
      ch->print( "bywater, maldoth, palainth, greyhaven, dragongate, venetorium or graecia ?\r\n" );
      return rSPELL_FAILED;
   }

   if( ch->in_room->area->flags.test( AFLAG_NOPORTAL ) || ch->in_room->flags.test( ROOM_NO_PORTAL ) )
   {
      ch->print( "Arcane magic blocks the formation of your gate.\r\n" );
      return rSPELL_FAILED;
   }

   room_index *room = NULL;
   if( !str_cmp( target_name, "bywater" ) )
      room = get_room_index( 7035 );
   /*
    * Updated Vnum for bywater -- Tarl 16 July 2002 
    */
   if( !str_cmp( target_name, "maldoth" ) )
      room = get_room_index( 10058 );

   if( !str_cmp( target_name, "palainth" ) )
      room = get_room_index( 5150 );

   if( !str_cmp( target_name, "greyhaven" ) )
      room = get_room_index( 4118 );

   if( !str_cmp( target_name, "dragongate" ) )
      room = get_room_index( 19339 );

   if( !str_cmp( target_name, "venetorium" ) )
      room = get_room_index( 5562 );

   if( !str_cmp( target_name, "graecia" ) )
      room = get_room_index( 13806 );

   if( !room )
   {
      ch->print( "Where do you wish to go??\r\n" );
      ch->print( "bywater, maldoth, palainth, greyhaven, dragongate, venetorium or graecia ?\r\n" );
      return rSPELL_FAILED;
   }

   int groupcount = 0, groupvisit = 0;
   list < char_data * >::iterator ich;
   for( ich = ch->in_room->people.begin(  ); ich != ch->in_room->people.end(  ); ++ich )
   {
      char_data *rch = *ich;

      if( is_same_group( rch, ch ) && !rch->isnpc(  ) )
         ++groupcount;
   }

   for( ich = ch->in_room->people.begin(  ); ich != ch->in_room->people.end(  ); ++ich )
   {
      char_data *rch = *ich;

      if( is_same_group( rch, ch ) && rch->has_visited( room->area ) && !rch->isnpc(  ) )
         ++groupvisit;
   }

   room_index *original = ch->in_room;

   if( groupcount == groupvisit )
   {
      leave_map( ch, NULL, room );  /* This will work, regardless. Trust me. */

      for( ich = original->people.begin(  ); ich != original->people.end(  ); )
      {
         char_data *rch = *ich;
         ++ich;

         if( is_same_group( rch, ch ) )
            leave_map( rch, ch, room );
      }
      return rNONE;
   }
   else
   {
      ch->printf( "You have not yet all visited %s!\r\n", room->area->name );
      return rSPELL_FAILED;
   }
}

/* Might and magic towngate spell added 12 Mar 01 by Samson/Tarl. */
SPELLF( spell_towngate )
{
   room_index *room = NULL;

   if( target_name.empty(  ) )
   {
      ch->print( "Where do you wish to go??\r\n" );
      ch->print( "bywater, maldoth, palainth, greyhaven, dragongate, venetorium or graecia ?\r\n" );
      return rSPELL_FAILED;
   }

   if( ch->in_room->area->flags.test( AFLAG_NOPORTAL ) || ch->in_room->flags.test( ROOM_NO_PORTAL ) )
   {
      ch->print( "Arcane magic blocks the formation of your gate.\r\n" );
      return rSPELL_FAILED;
   }

   if( !str_cmp( target_name, "bywater" ) )
      room = get_room_index( 7035 );
   /*
    * Updated Vnum for bywater -- Tarl 16 July 2002 
    */

   if( !str_cmp( target_name, "maldoth" ) )
      room = get_room_index( 10058 );

   if( !str_cmp( target_name, "palainth" ) )
      room = get_room_index( 5150 );

   if( !str_cmp( target_name, "greyhaven" ) )
      room = get_room_index( 4118 );

   if( !str_cmp( target_name, "dragongate" ) )
      room = get_room_index( 19339 );

   if( !str_cmp( target_name, "venetorium" ) )
      room = get_room_index( 5562 );

   if( !str_cmp( target_name, "graecia" ) )
      room = get_room_index( 13806 );

   if( !room )
   {
      ch->print( "Where do you wish to go??\r\n" );
      ch->print( "bywater, maldoth, palainth, greyhaven, dragongate, venetorium or graecia ?\r\n" );
      return rSPELL_FAILED;
   }

   if( !ch->has_visited( room->area ) )
   {
      ch->printf( "You have not yet visited %s!\r\n", room->area->name );
      return rSPELL_FAILED;
   }

   leave_map( ch, NULL, room );
   return rNONE;
}

SPELLF( spell_enlightenment )
{
   affect_data af;

   if( ch->has_aflag( AFF_ENLIGHTEN ) )
   {
      act( AT_MAGIC, "You may only be enlightened once a day.", ch, NULL, NULL, TO_CHAR );
      return rSPELL_FAILED;
   }
   if( ch->hit > ch->max_hit * 2 )
   {
      act( AT_MAGIC, "You cannot acheive enlightenment at this time.", ch, NULL, NULL, TO_CHAR );
      return rSPELL_FAILED;
   }

   af.type = skill_lookup( "enlightenment" );
   af.duration = 2616;  /* One day long */
   af.modifier = 0;
   af.location = APPLY_NONE;
   af.bit = AFF_ENLIGHTEN;
   ch->affect_to_char( &af );

   af.type = skill_lookup( "enlightenment" );
   af.duration = ch->level;
   af.modifier = ch->max_hit;
   af.location = APPLY_HIT;
   af.bit = AFF_ENLIGHTEN;
   ch->affect_to_char( &af );

   /*
    * Safety net just in case 
    */
   if( ch->hit > ch->max_hit )
      ch->hit = ch->max_hit;

   act( AT_MAGIC, "You have acheived enlightenment!", ch, NULL, NULL, TO_CHAR );
   return rNONE;
}

SPELLF( spell_rejuv )
{
   char_data *victim;

   if( target_name.empty(  ) )
      victim = ch;
   else
      victim = ch->get_char_room( target_name );

   if( victim->isnpc(  ) )
   {
      act( AT_MAGIC, "Rejuvenating a monster would be folly.", ch, NULL, NULL, TO_CHAR );
      return rSPELL_FAILED;
   }

   if( victim->is_immortal(  ) )
   {
      act( AT_MAGIC, "$N has no need of rejuvenation.", ch, NULL, victim, TO_CHAR );
      return rSPELL_FAILED;
   }

   if( !is_same_group( ch, victim ) )
   {
      act( AT_MAGIC, "You must be grouped with your target.", ch, NULL, NULL, TO_CHAR );
      return rSPELL_FAILED;
   }

   act( AT_MAGIC, "$n lays $s hands upon you and chants.", ch, NULL, victim, TO_VICT );
   act( AT_MAGIC, "You lay your hands upon $N and chant.", ch, NULL, victim, TO_CHAR );
   act( AT_MAGIC, "$n lays $s hands upon $N and chants.", ch, NULL, victim, TO_NOTVICT );

   if( number_range( 1, 100 ) != 42 || ch->is_immortal(  ) )
   {  /* All is well */
      if( victim->get_age(  ) > 20 )
      {
         act( AT_MAGIC, "You feel younger!", ch, NULL, victim, TO_CHAR );
         act( AT_MAGIC, "$n looks younger!", victim, NULL, NULL, TO_ROOM );
         victim->pcdata->age_bonus -= 1;
         ch->alignment += 50;
         if( ch->alignment > 1000 )
            ch->alignment = 1000;
      }
      else
         act( AT_MAGIC, "You don't feel any younger.", ch, NULL, victim, TO_CHAR );
   }
   else
      switch ( number_range( 1, 6 ) )
      {
         default:
         case 1:
         case 2:
         case 3: /* Death */
            if( number_range( 1, 3 ) == 1 )
            {  /* 1/3 chance for caster to get it */
               victim = ch;
            }
            act( AT_MAGIC, "Your heart gives out from the strain.", ch, NULL, victim, TO_VICT );
            act( AT_MAGIC, "$n's heart gives out from the strain.", victim, NULL, NULL, TO_ROOM );
            raw_kill( ch, victim ); /* Ooops. The target died :) */
            log_printf( "%s killed during rejuvenation by %s", victim->name, ch->name );
            break;
         case 4:
         case 5: /* Age */
            if( number_range( 1, 3 ) == 1 )
            {
               victim = ch;
            }
            act( AT_MAGIC, "You age horribly!", ch, NULL, victim, TO_VICT );
            act( AT_MAGIC, "$n ages horribly!", victim, NULL, NULL, TO_ROOM );
            victim->pcdata->age_bonus += 10;
            break;
         case 6: /* Drain *
                   * if (number_range(1, 3) == 1)
                   * {
                   * spell_energy_drain(60, ch, ch, 0);
                   * }
                   * else
                   * {
                   * spell_energy_drain(60, ch, victim, 0);
                   * } */
            break;
      }
   return rNONE;
}

SPELLF( spell_slow )
{
   char_data *victim;
   skill_type *skill = get_skilltype( sn );

   if( target_name.empty(  ) )
      victim = ch;
   else
      victim = ch->get_char_room( target_name );

   if( victim && is_same_char_map( ch, victim ) )
   {
      affect_data af;

      if( victim->has_immune( RIS_MAGIC ) )
      {
         immune_casting( skill, ch, victim, NULL );
         return rSPELL_FAILED;
      }

      if( victim->has_aflag( AFF_SLOW ) )
      {
         failed_casting( skill, ch, victim, NULL );
         return rSPELL_FAILED;
      }

      act( AT_MAGIC, "$n has been slowed to a crawl!", victim, NULL, NULL, TO_ROOM );
      af.type = sn;
      af.duration = level;
      af.location = APPLY_NONE;
      af.modifier = 0;
      af.bit = AFF_SLOW;
      victim->affect_to_char( &af );
      act( AT_MAGIC, "You have been slowed to a crawl!", victim, NULL, NULL, TO_CHAR );
      return rNONE;
   }
   ch->printf( "You can't find %s!\r\n", target_name.c_str(  ) );
   return rSPELL_FAILED;
}

SPELLF( spell_haste )
{
   char_data *victim;
   skill_type *skill = get_skilltype( sn );

   if( target_name.empty(  ) )
      victim = ch;
   else
      victim = ch->get_char_room( target_name );

   if( victim && is_same_char_map( ch, victim ) )
   {
      affect_data af;

      if( victim->has_immune( RIS_MAGIC ) )
      {
         immune_casting( skill, ch, victim, NULL );
         return rSPELL_FAILED;
      }

      if( victim->has_aflag( AFF_HASTE ) )
      {
         failed_casting( skill, ch, victim, NULL );
         return rSPELL_FAILED;
      }

      act( AT_MAGIC, "$n is endowed with the speed of Quicklings!", victim, NULL, NULL, TO_ROOM );
      af.type = sn;
      af.duration = level;
      af.location = APPLY_NONE;
      af.modifier = 0;
      af.bit = AFF_HASTE;
      victim->affect_to_char( &af );
      act( AT_MAGIC, "You are endowed with the speed of Quicklings!", victim, NULL, NULL, TO_CHAR );
      if( !victim->isnpc(  ) )
         victim->pcdata->age_bonus += 1;
      return rNONE;
   }
   ch->printf( "You can't find %s!\r\n", target_name.c_str(  ) );
   return rSPELL_FAILED;
}

SPELLF( spell_warsteed )
{
   mob_index *temp;
   char_data *mob;

   if( ( !ch->IS_OUTSIDE(  ) || INDOOR_SECTOR( ch->in_room->sector_type ) ) && !ch->has_pcflag( PCFLAG_ONMAP ) )
   {
      ch->print( "You must be outdoors to cast this spell.\r\n" );
      return rSPELL_FAILED;
   }

   if( ch->Class == CLASS_PALADIN )
   {
      if( !( temp = get_mob_index( MOB_VNUM_WARMOUNTTHREE ) ) )
      {
         bug( "%s: Paladin Warmount vnum %d doesn't exist.", __FUNCTION__, MOB_VNUM_WARMOUNTTHREE );
         return rSPELL_FAILED;
      }
   }
   else
   {
      if( !( temp = get_mob_index( MOB_VNUM_WARMOUNTFOUR ) ) )
      {
         bug( "%s: Antipaladin warmount vnum %d doesn't exist.", __FUNCTION__, MOB_VNUM_WARMOUNTFOUR );
         return rSPELL_FAILED;
      }
   }

   if( !ch->can_charm(  ) )
   {
      ch->print( "You already have too many followers to support!\r\n" );
      return rSPELL_FAILED;
   }

   mob = temp->create_mobile(  );
   if( !mob->to_room( ch->in_room ) )
      log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
   fix_maps( ch, mob );
   ch->print( "&[magic]You summon a spectacular flying steed into being!\r\n" );
   bind_follower( mob, ch, sn, ch->level * 10 );
   return rNONE;
}

SPELLF( spell_warmount )
{
   mob_index *temp;
   char_data *mob;

   if( ( !ch->IS_OUTSIDE(  ) || INDOOR_SECTOR( ch->in_room->sector_type ) ) && !ch->has_pcflag( PCFLAG_ONMAP ) )
   {
      ch->print( "You must be outdoors to cast this spell.\r\n" );
      return rSPELL_FAILED;
   }

   if( ch->Class == CLASS_PALADIN )
   {
      if( !( temp = get_mob_index( MOB_VNUM_WARMOUNT ) ) )
      {
         bug( "%s: Paladin Warmount vnum %d doesn't exist.", __FUNCTION__, MOB_VNUM_WARMOUNT );
         return rSPELL_FAILED;
      }
   }
   else
   {
      if( !( temp = get_mob_index( MOB_VNUM_WARMOUNTTWO ) ) )
      {
         bug( "%s: Antipaladin warmount vnum %d doesn't exist.", __FUNCTION__, MOB_VNUM_WARMOUNTTWO );
         return rSPELL_FAILED;
      }
   }

   if( !ch->can_charm(  ) )
   {
      ch->print( "You already have too many followers to support!\r\n" );
      return rSPELL_FAILED;
   }

   mob = temp->create_mobile(  );
   if( !mob->to_room( ch->in_room ) )
      log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
   fix_maps( ch, mob );
   ch->print( "&[magic]You summon a spectacular steed into being!\r\n" );
   bind_follower( mob, ch, sn, ch->level * 10 );
   return rNONE;
}

SPELLF( spell_fireseed )
{
   obj_data *obj;
   obj_index *objcheck;
   short seedcount = 4;

   objcheck = get_obj_index( OBJ_VNUM_FIRESEED );
   if( objcheck != NULL )
   {
      while( seedcount > 0 )
      {
         obj = get_obj_index( OBJ_VNUM_FIRESEED )->create_object( ch->level );
         obj->to_char( ch );
         --seedcount;
      }
      ch->print( "&[magic]With a wave of your hands, some fireseeds appear in your inventory.\r\n" );
      return rNONE;
   }
   else
   {
      bug( "%s: Fireseed object %d not found.", __FUNCTION__, OBJ_VNUM_FIRESEED );
      return rSPELL_FAILED;
   }
}

SPELLF( spell_despair )
{
   list < char_data * >::iterator ich;
   bool despair = false;

   /*
    * Add check for proper bard instrument in future 
    */
   for( ich = ch->in_room->people.begin(  ); ich != ch->in_room->people.end(  ); )
   {
      char_data *vch = *ich;
      ++ich;

      if( !vch->isnpc(  ) )
         continue;

      if( !is_same_char_map( ch, vch ) )
         continue;
      else
      {
         if( saves_spell_staff( level, vch ) )
            continue;
         else
         {
            interpret( vch, "flee" );
            despair = true;
         }
      }
   }
   if( despair )
      ch->print( "&[magic]Your magic strikes fear into the hearts of the occupants!\r\n" );
   else
      ch->print( "&[magic]You weave your magic, but there was no noticable affect.\r\n" );
   return rNONE;
}

SPELLF( spell_enrage )
{
   list < char_data * >::iterator ich;
   bool anger = false;

   /*
    * Add check for proper bard instrument in future 
    */

   for( ich = ch->in_room->people.begin(  ); ich != ch->in_room->people.end(  ); ++ich )
   {
      char_data *vch = *ich;

      if( !vch->isnpc(  ) )
         continue;

      if( !is_same_char_map( ch, vch ) )
         continue;
      else
      {
         if( !vch->has_actflag( ACT_AGGRESSIVE ) )
         {
            vch->set_actflag( ACT_AGGRESSIVE );
            anger = true;
         }
      }
   }
   if( anger )
      ch->print( "&[magic]The occupants of the room become highly enraged!\r\n" );
   else
      ch->print( "&[magic]You weave your magic, but there was no noticable affect.\r\n" );
   return rNONE;
}

SPELLF( spell_calm )
{
   list < char_data * >::iterator ich;
   bool soothe = false;

   /*
    * Add check for proper bard instrument in future 
    */

   for( ich = ch->in_room->people.begin(  ); ich != ch->in_room->people.end(  ); ++ich )
   {
      char_data *vch = *ich;

      if( !vch->isnpc(  ) )
         continue;

      if( !is_same_char_map( ch, vch ) )
         continue;
      else
      {
         if( vch->has_actflag( ACT_AGGRESSIVE ) )
         {
            vch->unset_actflag( ACT_AGGRESSIVE );
            soothe = true;
         }
         if( vch->has_actflag( ACT_META_AGGR ) )
         {
            vch->unset_actflag( ACT_META_AGGR );
            soothe = true;
         }
      }
   }
   if( soothe )
      ch->print( "&[magic]A soothing calm settles upon the occupants in the room.\r\n" );
   else
      ch->print( "&[magic]You weave your magic, but there was no noticable affect.\r\n" );
   return rNONE;
}

SPELLF( spell_gust_of_wind )
{
   if( ( !ch->IS_OUTSIDE(  ) || INDOOR_SECTOR( ch->in_room->sector_type ) ) && !ch->has_pcflag( PCFLAG_ONMAP ) )
   {
      ch->print( "You must be outdoors to cast this spell.\r\n" );
      return rSPELL_FAILED;
   }

   bool ch_died = false;
   list < char_data * >::iterator ich;
   for( ich = ch->in_room->people.begin(  ); ich != ch->in_room->people.end(  ); ++ich )
   {
      char_data *vch = *ich;

      if( !vch->isnpc(  ) )
         continue;

      if( !is_same_char_map( ch, vch ) )
         continue;
      else
      {
         int dam = dice( 8, 5 );

         vch->position = POS_SITTING;
         ch_ret retcode = damage( ch, vch, saves_spell_staff( level, vch ) ? dam / 2 : dam, sn );
         if( retcode == rCHAR_DIED || ch->char_died(  ) )
            ch_died = true;
      }
   }
   ch->print( "&[magic]You arouse a stiff gust of wind, knocking everyone to the floor.\r\n" );

   if( ch_died )
      return rCHAR_DIED;
   else
      return rNONE;
}

SPELLF( spell_sunray )
{
   if( ( !ch->IS_OUTSIDE(  ) || INDOOR_SECTOR( ch->in_room->sector_type ) ) && !ch->has_pcflag( PCFLAG_ONMAP ) )
   {
      ch->print( "You must be outdoors to cast this spell.\r\n" );
      return rSPELL_FAILED;
   }

   if( time_info.sunlight == SUN_SET || time_info.sunlight == SUN_DARK )
   {
      ch->print( "This spell cannot be cast without daylight.\r\n" );
      return rSPELL_FAILED;
   }

   int mobcount = 0;
   bool ch_died = false;
   list < char_data * >::iterator ich;
   for( ich = ch->in_room->people.begin(  ); ich != ch->in_room->people.end(  ); ++ich )
   {
      char_data *vch = *ich;

      if( !vch->isnpc(  ) )
         continue;

      if( !is_same_char_map( ch, vch ) )
         continue;
      else
      {
         if( IsUndead( vch ) )
         {
            int dam = dice( 8, 10 );
            ch_ret retcode = damage( ch, vch, saves_spell_staff( level, vch ) ? dam / 2 : dam, sn );
            if( retcode == rCHAR_DIED || ch->char_died(  ) )
               ch_died = true;
            ch->print( "&[magic]The burning rays of the sun sear your flesh!\r\n" );
         }

         if( vch->has_aflag( AFF_BLIND ) )
            continue;

         affect_data af;
         af.type = gsn_blindness;
         af.location = APPLY_HITROLL;
         af.modifier = -8;
         af.duration = ( int )( ( 1 + ( level / 3 ) ) * DUR_CONV );
         af.bit = AFF_BLIND;
         vch->affect_to_char( &af );
         vch->print( "&[magic]You are blinded!\r\n" );
         ++mobcount;
      }
   }
   if( mobcount > 0 )
      ch->print( "&[magic]You focus the rays of the sun, blinding everyone!\r\n" );
   else
      ch->print( "&[magic]You focus the rays of the sun, but little else.\r\n" );

   if( ch_died )
      return rCHAR_DIED;
   else
      return rNONE;
}

SPELLF( spell_creeping_doom )
{
   mob_index *temp;
   char_data *mob;

   if( ( !ch->IS_OUTSIDE(  ) || INDOOR_SECTOR( ch->in_room->sector_type ) ) && !ch->has_pcflag( PCFLAG_ONMAP ) )
   {
      ch->print( "You must be outdoors to cast this spell.\r\n" );
      return rSPELL_FAILED;
   }
   if( !( temp = get_mob_index( MOB_VNUM_CREEPINGDOOM ) ) )
   {
      bug( "%s: Creeping Doom vnum %d doesn't exist.", __FUNCTION__, MOB_VNUM_CREEPINGDOOM );
      return rSPELL_FAILED;
   }

   mob = temp->create_mobile(  );
   if( !mob->to_room( ch->in_room ) )
      log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
   fix_maps( ch, mob );
   ch->print( "&[magic]You summon a vile swarm of insects into being!\r\n" );
   return rNONE;
}

SPELLF( spell_heroes_feast )
{
   list < char_data * >::iterator ich;
   int heal = dice( 1, 4 ) + 4;

   for( ich = ch->in_room->people.begin(  ); ich != ch->in_room->people.end(  ); ++ich )
   {
      char_data *gch = *ich;

      if( is_same_group( gch, ch ) )
      {
         if( gch->isnpc(  ) )
            continue;
         if( !is_same_char_map( ch, gch ) )
            continue;
         else
         {
            gch->move = gch->max_move;
            gch->hit += heal;
            if( gch->hit > gch->max_hit )
               gch->hit = gch->max_hit;
            gch->pcdata->condition[COND_FULL] = sysdata->maxcondval;
            gch->pcdata->condition[COND_THIRST] = sysdata->maxcondval;
            gch->pcdata->condition[COND_DRUNK] = 0;
            gch->print( "You partake of a magnificent feast!\r\n" );
         }
      }
   }
   return rNONE;
}

SPELLF( spell_remove_paralysis )
{
   char_data *victim = ( char_data * ) vo;
   skill_type *skill = get_skilltype( sn );

   if( victim->has_immune( RIS_MAGIC ) )
   {
      immune_casting( skill, ch, victim, NULL );
      return rSPELL_FAILED;
   }

   if( !victim->is_affected( gsn_paralyze ) )
   {
      if( ch != victim )
         ch->print( "You work your cure, but it has no apparent effect.\r\n" );
      else
         ch->print( "You don't seem to be paralyzed.\r\n" );
      return rSPELL_FAILED;
   }
   victim->affect_strip( gsn_paralyze );
   victim->print( "&[magic]You can move again!\r\n" );
   if( ch != victim )
      ch->print( "&[magic]You work your cure, removing the paralysis.\r\n" );
   return rNONE;
}

SPELLF( spell_remove_silence )
{
   char_data *victim = ( char_data * ) vo;
   skill_type *skill = get_skilltype( sn );

   if( victim->has_immune( RIS_MAGIC ) )
   {
      immune_casting( skill, ch, victim, NULL );
      return rSPELL_FAILED;
   }

   if( !victim->is_affected( gsn_silence ) )
   {
      if( ch != victim )
         ch->print( "You work your cure, but it has no apparent effect.\r\n" );
      else
         ch->print( "You don't seem to be silenced.\r\n" );
      return rSPELL_FAILED;
   }
   victim->affect_strip( gsn_silence );
   victim->print( "&[magic]Your voice returns!\r\n" );
   if( ch != victim )
      ch->print( "&[magic]You work your cure, removing the silence.\r\n" );
   return rNONE;
}

SPELLF( spell_enchant_armor )
{
   obj_data *obj = ( obj_data * ) vo;
   affect_data *paf;

   if( obj->item_type != ITEM_ARMOR || obj->extra_flags.test( ITEM_MAGIC ) || !obj->affects.empty(  ) )
   {
      act( AT_MAGIC, "Your magic twists and winds around $p but cannot take hold.", ch, obj, NULL, TO_CHAR );
      act( AT_MAGIC, "$n's magic twists and winds around $p but cannot take hold.", ch, obj, NULL, TO_NOTVICT );
      return rSPELL_FAILED;
   }

   /*
    * Bug fix here. -- Alty 
    */
   obj->separate(  );
   paf = new affect_data;
   paf->type = -1;
   paf->duration = -1;
   paf->location = APPLY_AC;
/* Modified by Tarl (11 Mar 01) to adjust the bonus based on level. */
   switch ( level / 10 )
   {
      case 1:
         paf->modifier = -1;
         break;
      case 2:
         paf->modifier = -2;
         break;
      case 3:
         paf->modifier = -3;
         break;
      case 4:
         paf->modifier = -4;
         break;
      case 5:
         paf->modifier = -5;
         break;
      case 6:
         paf->modifier = -6;
         break;
      case 7:
         paf->modifier = -7;
         break;
      case 8:
         paf->modifier = -8;
         break;
      case 9:
         paf->modifier = -9;
         break;
      case 10:
         paf->modifier = -10;
         break;
      default:
         paf->modifier = -10;
   }
   paf->bit = 0;
   obj->affects.push_back( paf );

   obj->extra_flags.set( ITEM_MAGIC );

   if( ch->IS_GOOD(  ) )
   {
      obj->extra_flags.set( ITEM_ANTI_EVIL );
      act( AT_BLUE, "$p glows blue.", ch, obj, NULL, TO_CHAR );
   }
   else if( ch->IS_EVIL(  ) )
   {
      obj->extra_flags.set( ITEM_ANTI_GOOD );
      act( AT_RED, "$p glows red.", ch, obj, NULL, TO_CHAR );
   }
   else
   {
      obj->extra_flags.set( ITEM_ANTI_EVIL );
      obj->extra_flags.set( ITEM_ANTI_GOOD );
      act( AT_YELLOW, "$p glows yellow.", ch, obj, NULL, TO_CHAR );
   }
   ch->print( "Ok.\r\n" );
   return rNONE;
}

SPELLF( spell_remove_curse )
{
   obj_data *obj;
   list < obj_data * >::iterator iobj;
   char_data *victim = ( char_data * ) vo;
   skill_type *skill = get_skilltype( sn );

   if( victim->has_immune( RIS_MAGIC ) )
   {
      immune_casting( skill, ch, victim, NULL );
      return rSPELL_FAILED;
   }

   if( victim->is_affected( gsn_curse ) )
   {
      victim->affect_strip( gsn_curse );
      victim->print( "&[magic]The weight of your curse is lifted.\r\n" );
      if( ch != victim )
      {
         act( AT_MAGIC, "You dispel the curses afflicting $N.", ch, NULL, victim, TO_CHAR );
         act( AT_MAGIC, "$n's dispels the curses afflicting $N.", ch, NULL, victim, TO_NOTVICT );
      }
   }
   else if( !victim->carrying.empty(  ) )
   {
      for( iobj = victim->carrying.begin(  ); iobj != victim->carrying.end(  ); ++iobj )
      {
         obj = *iobj;
         if( !obj->in_obj && ( obj->extra_flags.test( ITEM_NOREMOVE ) || obj->extra_flags.test( ITEM_NODROP ) ) )
         {
            if( obj->extra_flags.test( ITEM_SINDHAE ) )
               continue;
            if( obj->extra_flags.test( ITEM_NOREMOVE ) )
               obj->extra_flags.reset( ITEM_NOREMOVE );
            if( obj->extra_flags.test( ITEM_NODROP ) )
               obj->extra_flags.reset( ITEM_NODROP );
            victim->print( "&[magic]You feel a burden released.\r\n" );
            if( ch != victim )
            {
               act( AT_MAGIC, "You dispel the curses afflicting $N.", ch, NULL, victim, TO_CHAR );
               act( AT_MAGIC, "$n's dispels the curses afflicting $N.", ch, NULL, victim, TO_NOTVICT );
            }
            return rNONE;
         }
      }
   }
   return rNONE;
}

/* A simple beacon spell, written by Quzah (quzah@geocities.com) Enjoy. */
/* Hacked to death by Samson 2-8-99 */
/* NOTE: Spell will work on overland, but results would be less than desireable.
 * Until it gets fixed, it's probably best to set the overland zones with nobeacon flags.
 */
SPELLF( spell_beacon )
{
   int a;

   if( ch->in_room == NULL )
   {
      return rNONE;
   }

   if( ch->in_room->area->flags.test( AFLAG_NORECALL ) || ch->in_room->area->flags.test( AFLAG_NOBEACON ) )
   {
      ch->print( "&[magic]Arcane magic disperses the spell, you cannot place a beacon in this area.\r\n" );
      return rSPELL_FAILED;
   }

   if( ch->in_room->flags.test( ROOM_NO_RECALL ) || ch->in_room->flags.test( ROOM_NOBEACON ) )
   {
      ch->print( "&[magic]Arcane magic disperses the spell, you cannot place a beacon in this room.\r\n" );
      return rSPELL_FAILED;
   }

   /*
    * Set beacons with this spell, up to 5 
    */

   for( a = 0; a < MAX_BEACONS; ++a )
   {
      if( ch->pcdata->beacon[a] == 0 || ch->pcdata->beacon[a] == ch->in_room->vnum )
         break;
   }

   if( a >= MAX_BEACONS )
   {
      ch->print( "&[magic]No more beacon space is available. You will need to clear one.\r\n" );
      return rSPELL_FAILED;
   }

   ch->pcdata->beacon[a] = ch->in_room->vnum;
   ch->print( "&[magic]A magical beacon forms, tuned to your ether pattern.\r\n" );
   return rNONE;
}

/* Lists beacons set by the beacon spell - Samson 2-7-99 */
CMDF( do_beacon )
{
   string arg;
   room_index *pRoomIndex = NULL;
   int a;

   if( ch->isnpc(  ) )
      return;

   argument = one_argument( argument, arg );
   /*
    * Edited by Tarl to include Area name. 24 Mar 02 
    */
   if( arg.empty(  ) )
   {
      ch->print( "To clear a set beacon: beacon clear #\r\n\r\n" );
      ch->print( " ## | Location name                           | Area\r\n" );
      ch->print( "----+-----------------------------------------+-------------\r\n" );

      for( a = 0; a < MAX_BEACONS; ++a )
      {
         pRoomIndex = get_room_index( ch->pcdata->beacon[a] );
         if( pRoomIndex != NULL )
            ch->printf( " %2d | %-39s | %s\r\n", a, pRoomIndex->name, pRoomIndex->area->name );
         else
            ch->printf( " %2d | Not Set\r\n", a );
         pRoomIndex = NULL;
      }
      return;
   }

   if( !str_cmp( arg, "clear" ) )
   {
      a = atoi( argument.c_str(  ) );

      if( a < 0 || a >= MAX_BEACONS )
      {
         ch->print( "Beacon value is out of range.\r\n" );
         return;
      }

      pRoomIndex = get_room_index( ch->pcdata->beacon[a] );

      if( !pRoomIndex )
      {
         ch->pcdata->beacon[a] = 0;
         ch->printf( "Beacon %d was already empty.\r\n", a );
         return;
      }

      ch->printf( "You sever your ether ties to %s.\r\n", pRoomIndex->name );
      ch->pcdata->beacon[a] = 0;
      return;
   }
   do_beacon( ch, "" );
}

/* New continent and plane based recall, moved from skills.c - Samson 3-28-98 */
int recall( char_data * ch, int target )
{
   room_index *location, *beacon;
   char_data *opponent;

   location = NULL;
   beacon = NULL;

   if( ( opponent = ch->who_fighting(  ) ) != NULL )
   {
      ch->print( "You cannot recall while fighting!\r\n" );
      return -1;
   }

   if( !ch->isnpc(  ) && target != -1 )
   {
      if( ch->pcdata->beacon[target] == 0 )
      {
         ch->print( "You have no beacon set in that slot!\r\n" );
         return -1;
      }

      beacon = get_room_index( ch->pcdata->beacon[target] );

      if( !beacon )
      {
         ch->print( "The beacon no longer exists.....\r\n" );
         ch->pcdata->beacon[target] = 0;
         return -1;
      }
      if( !beacon_check( ch, beacon ) )
         return -1;
      location = beacon;
   }

   if( !location )
      location = recall_room( ch );

   if( !location )
   {
      bug( "%s: No recall room found!", __FUNCTION__ );
      ch->print( "You cannot recall on this plane!\r\n" );
      return -1;
   }

   if( ch->in_room == location )
   {
      ch->print( "Is there a point to recalling when your already there?\r\n" );
      return -1;
   }

   if( ch->in_room->area->flags.test( AFLAG_NORECALL ) )
   {
      ch->print( "A mystical force blocks any retreat from this area.\r\n" );
      return -1;
   }

   if( ch->in_room->flags.test( ROOM_NO_RECALL ) )
   {
      ch->print( "A mystical force in the room blocks your attempt.\r\n" );
      return -1;
   }

   act( AT_MAGIC, "$n disappears in a swirl of smoke.", ch, NULL, NULL, TO_ROOM );

   leave_map( ch, NULL, location );

   act( AT_MAGIC, "$n appears from a puff of smoke!", ch, NULL, NULL, TO_ROOM );
   return 1;
}

/* Simplified recall spell for use ONLY on items - Samson 3-4-99 */
SPELLF( spell_recall )
{
   int call = -1;

   call = recall( ch, -1 );

   if( call == 1 )
      return rNONE;
   else
      return rSPELL_FAILED;
}

SPELLF( spell_chain_lightning )
{
   list < char_data * >::iterator ich;
   char_data *victim = ( char_data * ) vo;
   bool ch_died = false;
   ch_ret retcode = rNONE;

   if( victim )
      damage( ch, victim, dice( level, 6 ), sn );

   for( ich = ch->in_room->people.begin(  ); ich != ch->in_room->people.end(  ); )
   {
      char_data *vch = *ich;
      ++ich;

      if( victim == vch )
         continue;

      if( !is_same_char_map( ch, vch ) )
         continue;

      if( !vch->isnpc(  ) && vch->pcdata->wizinvis >= LEVEL_IMMORTAL )
         continue;

      int dam = dice( UMAX( 1, level - 1 ), 6 );

      if( vch != ch && ( ch->isnpc(  )? !vch->isnpc(  ) : vch->isnpc(  ) ) )
         retcode = damage( ch, vch, dam, sn );

      if( retcode == rCHAR_DIED || ch->char_died(  ) )
      {
         ch_died = true;
         continue;
      }

      if( vch->char_died(  ) )
         continue;

      if( !ch_died && vch->in_room->area == ch->in_room->area && vch->in_room != ch->in_room )
         vch->print( "&[magic]You hear a loud thunderclap...\r\n" );
   }

   if( ch_died )
      return rCHAR_DIED;
   else
      return rNONE;
}
