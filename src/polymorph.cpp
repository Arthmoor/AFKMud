/****************************************************************************
 *                   ^     +----- |  / ^     ^ |     | +-\                  *
 *                  / \    |      | /  |\   /| |     | |  \                 *
 *                 /   \   +---   |<   | \ / | |     | |  |                 *
 *                /-----\  |      | \  |  v  | |     | |  /                 *
 *               /       \ |      |  \ |     | +-----+ +-/                  *
 ****************************************************************************
 * AFKMud Copyright 1997-2020 by Roger Libiez (Samson),                     *
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
 *                          Shaddai's Polymorph                             *
 ****************************************************************************/

#include "mud.h"
#include "deity.h"
#include "objindex.h"
#include "polymorph.h"
#include "raceclass.h"

#define MKEY( literal, field, value ) \
if( !str_cmp( word, (literal) ) )     \
{                                     \
   DISPOSE( (field) );                \
   (field) = (value);                 \
   break;                             \
}

list < morph_data * >morphlist;
int morph_vnum = 0;

/*
 * Local functions
 */
void copy_morph( morph_data *, morph_data * );
CMDF( do_morphstat );

char_morph::char_morph(  )
{
   init_memory( &morph, &cast_allowed, sizeof( cast_allowed ) );
}

char_morph::~char_morph(  )
{
}

/*
 * Given the Morph's name, returns the pointer to the morph structure.
 * --Shaddai
 */
morph_data *get_morph( const string & arg )
{
   list < morph_data * >::iterator imorph;

   if( arg.empty(  ) )
      return nullptr;

   for( imorph = morphlist.begin(  ); imorph != morphlist.end(  ); ++imorph )
   {
      morph_data *morph = *imorph;

      if( !str_cmp( morph->name, arg ) )
         return morph;
   }
   return nullptr;
}

/*
 * Given the Morph's vnum, returns the pointer to the morph structure.
 * --Shaddai
 */
morph_data *get_morph_vnum( int vnum )
{
   list < morph_data * >::iterator imorph;

   if( vnum < 1 )
      return nullptr;

   for( imorph = morphlist.begin(  ); imorph != morphlist.end(  ); ++imorph )
   {
      morph_data *morph = *imorph;

      if( morph->vnum == vnum )
         return morph;
   }
   return nullptr;
}

/*
 * Checks to see if the char meets all the requirements to morph into
 * the provided morph.  Doesn't Look at NPC's Class or race as they
 * are different from pc's, but still checks level and if they can
 * cast it or not.  --Shaddai
 */
bool can_morph( char_data * ch, morph_data * morph, bool is_cast )
{
   if( morph == nullptr )
      return false;
   if( ch->is_immortal(  ) || ch->isnpc(  ) )   /* Let immortals morph to anything Also NPC can do any morph  */
      return true;
   if( morph->no_cast && is_cast )
      return false;
   if( ch->level < morph->level )
      return false;
   if( morph->pkill == ONLY_PKILL && !ch->IS_PKILL(  ) )
      return false;
   if( morph->pkill == ONLY_PEACEFULL && ch->IS_PKILL(  ) )
      return false;
   if( morph->sex != -1 && morph->sex != ch->sex )
      return false;
   if( morph->Class.any(  ) && !morph->Class.test( ch->Class ) )
      return false;
   if( morph->race.any(  ) && !morph->race.test( ch->race ) )
      return false;
   if( morph->deity && ( !ch->pcdata->deity || !get_deity( morph->deity ) ) )
      return false;
   if( morph->timeto != -1 && morph->timefrom != -1 )
   {
      int tmp, i;
      bool found = false;

      /*
       * i is a sanity check, just in case things go haywire so it doesn't
       * * loop forever here. -Shaddai
       */
      for( i = 0, tmp = morph->timefrom; i < 25 && tmp != morph->timeto; ++i )
      {
         if( tmp == time_info.hour )
         {
            found = true;
            break;
         }
         if( tmp == sysdata->hourmidnight - 1 )
            tmp = 0;
         else
            ++tmp;
      }
      if( !found )
         return false;
   }
   if( morph->dayfrom != -1 && morph->dayto != -1 && ( morph->dayto < ( time_info.day + 1 ) || morph->dayfrom > ( time_info.day + 1 ) ) )
      return false;
   return true;
}

/*
 * Find a morph you can use -- Shaddai
 */
morph_data *find_morph( char_data * ch, const string & target, bool is_cast )
{
   list < morph_data * >::iterator imorph;

   if( target.empty(  ) )
      return nullptr;

   for( imorph = morphlist.begin(  ); imorph != morphlist.end(  ); ++imorph )
   {
      morph_data *morph = *imorph;

      if( str_cmp( morph->name, target ) )
         continue;
      if( can_morph( ch, morph, is_cast ) )
         return morph;
   }
   return nullptr;
}

/* 
 * Write one morph structure to a file. It doesn't print the variable to file
 * if it hasn't been set why waste disk-space :)  --Shaddai
 */
void fwrite_morph( FILE * fp, morph_data * morph )
{
   if( !morph )
      return;

   fprintf( fp, "Version         %d\n", MORPHFILEVER );
   fprintf( fp, "Morph           %s\n", morph->name );
   if( morph->obj[0] != 0 || morph->obj[1] != 0 || morph->obj[2] != 0 )
      fprintf( fp, "Objs            %d %d %d\n", morph->obj[0], morph->obj[1], morph->obj[2] );
   if( morph->objuse[0] != 0 || morph->objuse[1] != 0 || morph->objuse[2] != 0 )
      fprintf( fp, "Objuse          %d %d %d\n", morph->objuse[0], morph->objuse[1], morph->objuse[2] );
   if( morph->vnum != 0 )
      fprintf( fp, "Vnum            %d\n", morph->vnum );
   if( morph->damroll[0] != '\0' )
      fprintf( fp, "Damroll         %s~\n", morph->damroll );
   if( morph->defpos != POS_STANDING )
      fprintf( fp, "Defpos          %d\n", morph->defpos );
   if( morph->description && morph->description[0] != '\0' )
      fprintf( fp, "Description     %s~\n", strip_cr( morph->description ) );
   if( morph->help && morph->help[0] != '\0' )
      fprintf( fp, "Help            %s~\n", strip_cr( morph->help ) );
   if( morph->hit && morph->hit[0] != '\0' )
      fprintf( fp, "Hit             %s~\n", morph->hit );
   if( morph->hitroll && morph->hitroll[0] != '\0' )
      fprintf( fp, "Hitroll         %s~\n", morph->hitroll );
   if( morph->key_words && morph->key_words[0] != '\0' )
      fprintf( fp, "Keywords        %s~\n", morph->key_words );
   if( morph->long_desc && morph->long_desc[0] != '\0' )
      fprintf( fp, "Longdesc        %s~\n", strip_cr( morph->long_desc ) );
   if( morph->mana && morph->mana[0] != '\0' )
      fprintf( fp, "Mana            %s~\n", morph->mana );
   if( morph->morph_other && morph->morph_other[0] != '\0' )
      fprintf( fp, "MorphOther      %s~\n", morph->morph_other );
   if( morph->morph_self && morph->morph_self[0] != '\0' )
      fprintf( fp, "MorphSelf       %s~\n", morph->morph_self );
   if( morph->move && morph->move[0] != '\0' )
      fprintf( fp, "Move            %s~\n", morph->move );
   if( morph->no_skills && morph->no_skills[0] != '\0' )
      fprintf( fp, "NoSkills        %s~\n", morph->no_skills );
   if( morph->short_desc && morph->short_desc[0] != '\0' )
      fprintf( fp, "ShortDesc       %s~\n", morph->short_desc );
   if( morph->skills && morph->skills[0] != '\0' )
      fprintf( fp, "Skills          %s~\n", morph->skills );
   if( morph->unmorph_other && morph->unmorph_other[0] != '\0' )
      fprintf( fp, "UnmorphOther    %s~\n", morph->unmorph_other );
   if( morph->unmorph_self && morph->unmorph_self[0] != '\0' )
      fprintf( fp, "UnmorphSelf     %s~\n", morph->unmorph_self );
   if( morph->affected_by.any(  ) )
      fprintf( fp, "Affected        %s~\n", bitset_string( morph->affected_by, aff_flags ) );
   if( morph->no_affected_by.any(  ) )
      fprintf( fp, "NoAffected      %s~\n", bitset_string( morph->no_affected_by, aff_flags ) );
   if( morph->Class.any(  ) )
      fprintf( fp, "Class           %s~\n", bitset_string( morph->Class, npc_class ) );
   if( morph->race.any(  ) )
      fprintf( fp, "Race            %s~\n", bitset_string( morph->race, npc_race ) );
   if( morph->resistant.any(  ) )
      fprintf( fp, "Resistant       %s\n", bitset_string( morph->resistant, ris_flags ) );
   if( morph->suscept.any(  ) )
      fprintf( fp, "Suscept         %s~\n", bitset_string( morph->suscept, ris_flags ) );
   if( morph->immune.any(  ) )
      fprintf( fp, "Immune          %s~\n", bitset_string( morph->immune, ris_flags ) );
   if( morph->absorb.any(  ) )
      fprintf( fp, "Absorb		%s~\n", bitset_string( morph->absorb, ris_flags ) );
   if( morph->no_immune.any(  ) )
      fprintf( fp, "NoImmune        %s~\n", bitset_string( morph->no_immune, ris_flags ) );
   if( morph->no_resistant.any(  ) )
      fprintf( fp, "NoResistant     %s~\n", bitset_string( morph->no_resistant, ris_flags ) );
   if( morph->no_suscept.any(  ) )
      fprintf( fp, "NoSuscept       %s~\n", bitset_string( morph->no_suscept, ris_flags ) );
   if( morph->timer != 0 )
      fprintf( fp, "Timer           %d\n", morph->timer );
   if( morph->used != 0 )
      fprintf( fp, "Used            %d\n", morph->used );
   if( morph->sex != -1 )
      fprintf( fp, "Sex             %d\n", morph->sex );
   if( morph->pkill != 0 )
      fprintf( fp, "Pkill           %d\n", morph->pkill );
   if( morph->timefrom != -1 )
      fprintf( fp, "TimeFrom        %d\n", morph->timefrom );
   if( morph->timeto != -1 )
      fprintf( fp, "TimeTo          %d\n", morph->timeto );
   if( morph->dayfrom != -1 )
      fprintf( fp, "DayFrom         %d\n", morph->dayfrom );
   if( morph->dayto != -1 )
      fprintf( fp, "DayTo           %d\n", morph->dayto );
   if( morph->manaused != 0 )
      fprintf( fp, "ManaUsed        %d\n", morph->manaused );
   if( morph->moveused != 0 )
      fprintf( fp, "MoveUsed        %d\n", morph->moveused );
   if( morph->hpused != 0 )
      fprintf( fp, "HpUsed          %d\n", morph->hpused );
   if( morph->favourused != 0 )
      fprintf( fp, "FavourUsed      %d\n", morph->favourused );
   if( morph->ac != 0 )
      fprintf( fp, "Armor           %d\n", morph->ac );
   if( morph->cha != 0 )
      fprintf( fp, "Charisma        %d\n", morph->cha );
   if( morph->con != 0 )
      fprintf( fp, "Constitution    %d\n", morph->con );
   if( morph->dex != 0 )
      fprintf( fp, "Dexterity       %d\n", morph->dex );
   if( morph->dodge != 0 )
      fprintf( fp, "Dodge           %d\n", morph->dodge );
   if( morph->inte != 0 )
      fprintf( fp, "Intelligence    %d\n", morph->inte );
   if( morph->lck != 0 )
      fprintf( fp, "Luck            %d\n", morph->lck );
   if( morph->level != 0 )
      fprintf( fp, "Level           %d\n", morph->level );
   if( morph->parry != 0 )
      fprintf( fp, "Parry        	%d\n", morph->parry );
   if( morph->saving_breath != 0 )
      fprintf( fp, "SaveBreath      %d\n", morph->saving_breath );
   if( morph->saving_para_petri != 0 )
      fprintf( fp, "SavePara        %d\n", morph->saving_para_petri );
   if( morph->saving_poison_death != 0 )
      fprintf( fp, "SavePoison      %d\n", morph->saving_poison_death );
   if( morph->saving_spell_staff != 0 )
      fprintf( fp, "SaveSpell       %d\n", morph->saving_spell_staff );
   if( morph->saving_wand != 0 )
      fprintf( fp, "SaveWand        %d\n", morph->saving_wand );
   if( morph->str != 0 )
      fprintf( fp, "Strength        %d\n", morph->str );
   if( morph->tumble != 0 )
      fprintf( fp, "Tumble          %d\n", morph->tumble );
   if( morph->wis != 0 )
      fprintf( fp, "Wisdom          %d\n", morph->wis );
   if( morph->no_cast )
      fprintf( fp, "NoCast          %d\n", morph->no_cast );
   if( morph->cast_allowed )
      fprintf( fp, "CastAllowed     %d\n", morph->cast_allowed );
   fprintf( fp, "%s", "End\n\n" );
}

/*
 * This function saves the morph data.  Should be put in on reboot or shutdown
 * to make use of the sort algorithim. --Shaddai
 */
void save_morphs( void )
{
   list < morph_data * >::iterator imorph;
   FILE *fp;

   if( !( fp = fopen( SYSTEM_DIR MORPH_FILE, "w" ) ) )
   {
      bug( "%s: fopen", __func__ );
      perror( SYSTEM_DIR MORPH_FILE );
      return;
   }
   for( imorph = morphlist.begin(  ); imorph != morphlist.end(  ); ++imorph )
   {
      morph_data *morph = *imorph;

      fwrite_morph( fp, morph );
   }
   fprintf( fp, "%s", "#END\n" );
   FCLOSE( fp );
}

/*
 *  Command used to set all the morphing information.
 *  Must use the morphset save command, to write the commands to file.
 *  Hp/Mana/Move/Hitroll/Damroll can be set using variables such
 *  as 1d2+10.  No boundry checks are in place yet on those, so care must
 *  be taken when using these.  --Shaddai
 */
CMDF( do_morphset )
{
   string arg1, arg2, arg3;
   string origarg = argument;
   char buf[MSL];
   int value;
   morph_data *morph = nullptr;

   if( ch->isnpc(  ) )
   {
      ch->print( "Mob's can't morphset\r\n" );
      return;
   }

   if( !ch->desc )
   {
      ch->print( "You have no descriptor\r\n" );
      return;
   }

   switch ( ch->substate )
   {
      default:
         break;
      case SUB_MORPH_DESC:
         if( !ch->pcdata->dest_buf )
         {
            ch->print( "Fatal error: report to Samson.\r\n" );
            bug( "%s: sub_morph_desc: nullptr ch->pcdata->dest_buf", __func__ );
            ch->substate = SUB_NONE;
            return;
         }
         morph = ( morph_data * ) ch->pcdata->dest_buf;
         DISPOSE( morph->description );
         morph->description = ch->copy_buffer( false );
         ch->stop_editing(  );
         ch->substate = ch->tempnum;
         if( ch->substate == SUB_REPEATCMD )
            ch->pcdata->dest_buf = morph;
         return;

      case SUB_MORPH_HELP:
         if( !ch->pcdata->dest_buf )
         {
            ch->print( "Fatal error: report to Samson.\r\n" );
            bug( "%s: sub_morph_help: nullptr ch->pcdata->dest_buf", __func__ );
            ch->substate = SUB_NONE;
            return;
         }
         morph = ( morph_data * ) ch->pcdata->dest_buf;
         DISPOSE( morph->help );
         morph->help = ch->copy_buffer( false );
         ch->stop_editing(  );
         ch->substate = ch->tempnum;
         if( ch->substate == SUB_REPEATCMD )
            ch->pcdata->dest_buf = morph;
         return;

      case SUB_EDIT_ABORT:
         ch->substate = SUB_NONE;
         ch->print( "Aborting description.\r\n" );
         return;
   }
   morph = nullptr;
   smash_tilde( argument );

   if( ch->substate == SUB_REPEATCMD )
   {
      morph = ( morph_data * ) ch->pcdata->dest_buf;

      if( !morph )
      {
         ch->print( "Someone deleted your morph!\r\n" );
         argument = "done";
      }

      if( argument.empty(  ) )
      {
         do_morphstat( ch, morph->name );
         return;
      }

      if( !str_cmp( argument, "stat" ) )
      {
         funcf( ch, do_morphstat, "%s help", morph->name );
         return;
      }

      if( !str_cmp( argument, "done" ) || !str_cmp( argument, "off" ) )
      {
         ch->print( "Morphset mode off.\r\n" );
         ch->substate = SUB_NONE;
         ch->pcdata->dest_buf = nullptr;
         STRFREE( ch->pcdata->subprompt );
         return;
      }
   }

   if( morph )
   {
      arg1 = morph->name;
      argument = one_argument( argument, arg2 );
      arg3 = argument;
   }
   else
   {
      argument = one_argument( argument, arg1 );
      argument = one_argument( argument, arg2 );
      arg3 = argument;
   }

   if( !str_cmp( arg1, "on" ) )
   {
      ch->print( "Syntax: morphset <morph> on.\r\n" );
      return;
   }
   value = is_number( arg3 ) ? atoi( arg3.c_str(  ) ) : -1;

   if( atoi( arg3.c_str(  ) ) < -1 && value == -1 )
      value = atoi( arg3.c_str(  ) );

   if( ch->substate != SUB_REPEATCMD && !arg1.empty(  ) && !str_cmp( arg1, "save" ) )
   {
      save_morphs(  );
      ch->print( "Morph data saved.\r\n" );
      return;
   }

   if( arg1.empty(  ) || ( arg2.empty(  ) && ch->substate != SUB_REPEATCMD ) || !str_cmp( arg1, "?" ) )
   {
      if( ch->substate == SUB_REPEATCMD )
      {
         if( morph )
            ch->print( "Syntax: <field>  <value>\r\n" );
         else
            ch->print( "Syntax: <morph> <field>  <value>\r\n" );
      }
      else
         ch->print( "Syntax: morphset <morph> <field>  <value>\r\n" );
      ch->print( "Syntax: morphset save\r\n\r\n" );
      ch->print( "&cField being one of:\r\n" );
      ch->print( "&c-------------------------------------------------\r\n" );
      ch->print( "  ac, affected, cha, Class, con, damroll, dayto,\r\n" );
      ch->print( "  dayfrom, deity, description, defpos, dex, dodge,\r\n" );
      ch->print( "  favourused, help, hitroll, hp, hpused, immune,\r\n" );
      ch->print( "  int, str, keyword, lck, level, long, mana, manaused,\r\n" );
      ch->print( "  morphother, morphself, move, moveused, name, noaffected,\r\n" );
      ch->print( "  nocast, castallow, noimmune, noresistant, noskill, nosusceptible,\r\n" );
      ch->print( "  obj1, obj2, obj3, objuse1, objuse2, objuse3, parry,\r\n" );
      ch->print( "  pkill, race, resistant, sav1, sav2, sav3, sav4, sav5,\r\n" );
      ch->print( "  sex, short, skills, susceptible, timefrom, timer, timeto,\r\n" );
      ch->print( "  tumble, unmorphother, unmorphself, wis.\r\n" );
      ch->print( "&c-------------------------------------------------\r\n" );
      return;
   }

   if( !morph )
   {
      if( !is_number( arg1 ) )
         morph = get_morph( arg1 );
      else
         morph = get_morph_vnum( atoi( arg1.c_str(  ) ) );

      if( morph == nullptr )
      {
         ch->print( "That morph does not exist.\r\n" );
         return;
      }
   }

   if( !str_cmp( arg2, "on" ) )
   {
      ch->CHECK_SUBRESTRICTED(  );
      ch->printf( "Morphset mode on. (Editing %s).\r\n", morph->name );
      ch->substate = SUB_REPEATCMD;
      ch->pcdata->dest_buf = morph;
      stralloc_printf( &ch->pcdata->subprompt, "<&CMorphset &W%s&w> %%i", morph->name );
      return;
   }

   if( !str_cmp( arg2, "str" ) )
   {
      if( value < -10 || value > 10 )
      {
         ch->print( "Strength must be a value from -10 to 10.\r\n" );
         return;
      }
      morph->str = value;
   }
   else if( !str_cmp( arg2, "int" ) )
   {
      if( value < -10 || value > 10 )
      {
         ch->print( "Intelligence must be a value from -10 to 10.\r\n" );
         return;
      }
      morph->inte = value;
   }
   else if( !str_cmp( arg2, "wis" ) )
   {
      if( value < -10 || value > 10 )
      {
         ch->print( "Wisdom must be a value from -10 to 10.\r\n" );
         return;
      }
      morph->wis = value;
   }
   else if( !str_cmp( arg2, "defpos" ) )
   {
      if( value < 0 || value > POS_STANDING )
      {
         ch->printf( "Position range is 0 to %d.\r\n", POS_STANDING );
         return;
      }
      morph->defpos = value;
   }
   else if( !str_cmp( arg2, "dex" ) )
   {
      if( value < -10 || value > 10 )
      {
         ch->print( "Dexterity must be a value from -10 to 10.\r\n" );
         return;
      }
      morph->dex = value;
   }
   else if( !str_cmp( arg2, "con" ) )
   {
      if( value < -10 || value > 10 )
      {
         ch->print( "Constitution must be a value from -10 to 10.\r\n" );
         return;
      }
      morph->con = value;
   }
   else if( !str_cmp( arg2, "cha" ) )
   {
      if( value < -10 || value > 10 )
      {
         ch->print( "Charisma must be a value from -10 to 10.\r\n" );
         return;
      }
      morph->cha = value;
   }
   else if( !str_cmp( arg2, "lck" ) )
   {
      if( value < -10 || value > 10 )
      {
         ch->print( "Luck must be a value from -10 to 10.\r\n" );
         return;
      }
      morph->lck = value;
   }
   else if( !str_cmp( arg2, "sex" ) )
   {
      value = get_npc_sex( arg3 );

      if( value < 0 || value >= SEX_MAX )
      {
         ch->print( "Invalid sex.\r\n" );
         return;
      }
      morph->sex = value;
   }
   else if( !str_cmp( arg2, "pkill" ) )
   {
      if( !str_cmp( arg3, "pkill" ) )
         morph->pkill = ONLY_PKILL;
      else if( !str_cmp( arg3, "peace" ) )
         morph->pkill = ONLY_PEACEFULL;
      else if( !str_cmp( arg3, "none" ) )
         morph->pkill = 0;
      else
      {
         ch->print( "Usuage: morphset <morph> pkill [pkill|peace|none]\r\n" );
         return;
      }
   }
   else if( !str_cmp( arg2, "manaused" ) )
   {
      if( value < 0 || value > 2000 )
      {
         ch->print( "Mana used is a value from 0 to 2000.\r\n" );
         return;
      }
      morph->manaused = value;
   }
   else if( !str_cmp( arg2, "moveused" ) )
   {
      if( value < 0 || value > 2000 )
      {
         ch->print( "Move used is a value from 0 to 2000.\r\n" );
         return;
      }
      morph->moveused = value;
   }
   else if( !str_cmp( arg2, "hpused" ) )
   {
      if( value < 0 || value > 2000 )
      {
         ch->print( "Hp used is a value from 0 to 2000.\r\n" );
         return;
      }
      morph->hpused = value;
   }
   else if( !str_cmp( arg2, "favourused" ) )
   {
      if( value < 0 || value > 2000 )
      {
         ch->print( "Favour used is a value from 0 to 2000.\r\n" );
         return;
      }
      morph->favourused = value;
   }
   else if( !str_cmp( arg2, "timeto" ) )
   {
      if( value < 0 || value > sysdata->hoursperday - 1 )
      {
         ch->printf( "Timeto is a value from 0 to %d.\r\n", sysdata->hoursperday - 1 );
         return;
      }
      morph->timeto = value;
   }
   else if( !str_cmp( arg2, "timefrom" ) )
   {
      if( value < 0 || value > sysdata->hoursperday - 1 )
      {
         ch->printf( "Timefrom is a value from 0 to %d.\r\n", sysdata->hoursperday - 1 );
         return;
      }
      morph->timefrom = value;
   }
   else if( !str_cmp( arg2, "dayto" ) )
   {
      if( value < 0 || value > sysdata->dayspermonth - 1 )
      {
         ch->printf( "Dayto is a value from 0 to %d.\r\n", sysdata->dayspermonth - 1 );
         return;
      }
      morph->dayto = value;
   }
   else if( !str_cmp( arg2, "dayfrom" ) )
   {
      if( value < 0 || value > sysdata->dayspermonth - 1 )
      {
         ch->printf( "Dayfrom is a value from 0 to %d.\r\n", sysdata->dayspermonth - 1 );
         return;
      }
      morph->dayfrom = value;
   }
   else if( !str_cmp( arg2, "sav1" ) || !str_cmp( arg2, "savepoison" ) )
   {
      if( value < -30 || value > 30 )
      {
         ch->print( "Saving throw range is -30 to 30.\r\n" );
         return;
      }
      morph->saving_poison_death = value;
   }
   else if( !str_cmp( arg2, "sav2" ) || !str_cmp( arg2, "savewand" ) )
   {
      if( value < -30 || value > 30 )
      {
         ch->print( "Saving throw range is -30 to 30.\r\n" );
         return;
      }
      morph->saving_wand = value;
   }
   else if( !str_cmp( arg2, "sav3" ) || !str_cmp( arg2, "savepara" ) )
   {
      if( value < -30 || value > 30 )
      {
         ch->print( "Saving throw range is -30 to 30.\r\n" );
         return;
      }
      morph->saving_para_petri = value;
   }
   else if( !str_cmp( arg2, "sav4" ) || !str_cmp( arg2, "savebreath" ) )
   {
      if( value < -30 || value > 30 )
      {
         ch->print( "Saving throw range is -30 to 30.\r\n" );
         return;
      }
      morph->saving_breath = value;
   }
   else if( !str_cmp( arg2, "sav5" ) || !str_cmp( arg2, "savestaff" ) )
   {
      if( value < -30 || value > 30 )
      {
         ch->print( "Saving throw range is -30 to 30.\r\n" );
         return;
      }
      morph->saving_spell_staff = value;
   }
   else if( !str_cmp( arg2, "timer" ) )
   {
      if( value < -1 || value == 0 )
      {
         ch->print( "Timer must be -1 (None) or greater than 0.\r\n" );
         return;
      }
      morph->timer = value;
   }
   else if( !str_cmp( arg2, "hp" ) )
   {
      argument = one_argument( argument, arg3 );
      DISPOSE( morph->hit );
      if( str_cmp( arg3, "0" ) )
         morph->hit = str_dup( arg3.c_str(  ) );
   }
   else if( !str_cmp( arg2, "mana" ) )
   {
      argument = one_argument( argument, arg3 );
      DISPOSE( morph->mana );
      if( str_cmp( arg3, "0" ) )
         morph->mana = str_dup( arg3.c_str(  ) );
   }
   else if( !str_cmp( arg2, "move" ) )
   {
      argument = one_argument( argument, arg3 );
      DISPOSE( morph->move );
      if( str_cmp( arg3, "0" ) )
         morph->move = str_dup( arg3.c_str(  ) );
   }
   else if( !str_cmp( arg2, "ac" ) )
   {
      if( value > 500 || value < -500 )
      {
         ch->print( "Ac range is -500 to 500.\r\n" );
         return;
      }
      morph->ac = value;
   }
   else if( !str_cmp( arg2, "hitroll" ) )
   {
      argument = one_argument( argument, arg3 );
      DISPOSE( morph->hitroll );
      if( str_cmp( arg3, "0" ) )
         morph->hitroll = str_dup( arg3.c_str(  ) );
   }
   else if( !str_cmp( arg2, "damroll" ) )
   {
      argument = one_argument( argument, arg3 );
      DISPOSE( morph->damroll );
      if( str_cmp( arg3, "0" ) )
         morph->damroll = str_dup( arg3.c_str(  ) );
   }
   else if( !str_cmp( arg2, "dodge" ) )
   {
      if( value > 100 || value < -100 )
      {
         ch->print( "Dodge range is -100 to 100.\r\n" );
         return;
      }
      morph->dodge = value;
   }
   else if( !str_prefix( "obj", arg2 ) )
   {
      int oindex;
      string temp;

      if( arg2.length(  ) <= 3 )
      {
         ch->print( "Obj 1, 2, or 3.\r\n" );
         return;
      }
      temp = arg2.substr( 3, arg3.length(  ) );
      oindex = atoi( temp.c_str(  ) );
      if( oindex > 3 || oindex < 1 )
      {
         ch->print( "Obj 1, 2, or 3.\r\n" );
         return;
      }
      if( !( get_obj_index( value ) ) )
      {
         ch->print( "No such vnum.\r\n" );
         return;
      }
      morph->obj[oindex - 1] = value;
   }
   else if( !str_cmp( arg2, "parry" ) )
   {
      if( value > 100 || value < -100 )
      {
         ch->print( "Parry range is -100 to 100.\r\n" );
         return;
      }
      morph->parry = value;
   }
   else if( !str_cmp( arg2, "tumble" ) )
   {
      if( value > 100 || value < -100 )
      {
         ch->print( "Tumble range is -100 to 100.\r\n" );
         return;
      }
      morph->tumble = value;
   }
   else if( !str_cmp( arg2, "level" ) )
   {
      if( value < 0 || value > LEVEL_AVATAR )
      {
         ch->printf( "Level range is 0 to %d.\r\n", LEVEL_AVATAR );
         return;
      }
      morph->level = value;
   }
   else if( !str_prefix( arg2, "objuse" ) )
   {
      int oindex;
      string temp;

      if( arg2.length(  ) <= 6 )
      {
         ch->print( "Objuse 1, 2 or 3?\r\n" );
         return;
      }
      temp = arg2.substr( 6, arg2.length(  ) );
      oindex = atoi( temp.c_str(  ) );
      if( oindex > 3 || oindex < 1 )
      {
         ch->print( "Objuse 1, 2, or 3?\r\n" );
         return;
      }
      if( value )
         morph->objuse[oindex - 1] = true;
      else
         morph->objuse[oindex - 1] = false;
   }
   else if( !str_cmp( arg2, "nocast" ) )
   {
      if( value )
         morph->no_cast = true;
      else
         morph->no_cast = false;
   }
   else if( !str_cmp( arg2, "castallow" ) )
   {
      if( !str_cmp( arg3, "true" ) )
         morph->cast_allowed = true;
      else
         morph->cast_allowed = false;
   }
   else if( !str_cmp( arg2, "resistant" ) || !str_cmp( arg2, "r" ) )
   {
      if( argument.empty(  ) )
      {
         ch->print( "Usage: morphset <morph> resistant <flag> [flag]...\r\n" );
         return;
      }

      while( !argument.empty(  ) )
      {
         argument = one_argument( argument, arg3 );
         value = get_risflag( arg3 );
         if( value < 0 || value >= MAX_RIS_FLAG )
            ch->printf( "Unknown flag: %s\r\n", arg3.c_str(  ) );
         else
            morph->resistant.flip( value );
      }
   }
   else if( !str_cmp( arg2, "susceptible" ) || !str_cmp( arg2, "s" ) )
   {
      if( argument.empty(  ) )
      {
         ch->print( "Usage: morphset <morph> susceptible <flag> [flag]...\r\n" );
         return;
      }

      while( !argument.empty(  ) )
      {
         argument = one_argument( argument, arg3 );
         value = get_risflag( arg3 );
         if( value < 0 || value >= MAX_RIS_FLAG )
            ch->printf( "Unknown flag: %s\r\n", arg3.c_str(  ) );
         else
            morph->suscept.flip( value );
      }
   }
   else if( !str_cmp( arg2, "immune" ) || !str_cmp( arg2, "i" ) )
   {
      if( argument.empty(  ) )
      {
         ch->print( "Usage: morphset <morph> immune <flag> [flag]...\r\n" );
         return;
      }

      while( !argument.empty(  ) )
      {
         argument = one_argument( argument, arg3 );
         value = get_risflag( arg3 );
         if( value < 0 || value >= MAX_RIS_FLAG )
            ch->printf( "Unknown flag: %s\r\n", arg3.c_str(  ) );
         else
            morph->immune.flip( value );
      }
   }
   else if( !str_cmp( arg2, "absorb" ) || !str_cmp( arg2, "a" ) )
   {
      if( argument.empty(  ) )
      {
         ch->print( "Usage: morphset <morph> absorb <flag> [flag]...\r\n" );
         return;
      }

      while( !argument.empty(  ) )
      {
         argument = one_argument( argument, arg3 );
         value = get_risflag( arg3 );
         if( value < 0 || value >= MAX_RIS_FLAG )
            ch->printf( "Unknown flag: %s\r\n", arg3.c_str(  ) );
         else
            morph->absorb.flip( value );
      }
   }
   else if( !str_cmp( arg2, "noresistant" ) || !str_cmp( arg2, "nr" ) )
   {
      if( argument.empty(  ) )
      {
         ch->print( "Usage: morphset <morph> noresistant <flag> [flag]...\r\n" );
         return;
      }

      while( !argument.empty(  ) )
      {
         argument = one_argument( argument, arg3 );
         value = get_risflag( arg3 );
         if( value < 0 || value >= MAX_RIS_FLAG )
            ch->printf( "Unknown flag: %s\r\n", arg3.c_str(  ) );
         else
            morph->no_resistant.flip( value );
      }
   }
   else if( !str_cmp( arg2, "nosusceptible" ) || !str_cmp( arg2, "ns" ) )
   {
      if( argument.empty(  ) )
      {
         ch->print( "Usage: morphset <morph> nosusceptible <flag> [flag]...\r\n" );
         return;
      }

      while( !argument.empty(  ) )
      {
         argument = one_argument( argument, arg3 );
         value = get_risflag( arg3 );
         if( value < 0 || value >= MAX_RIS_FLAG )
            ch->printf( "Unknown flag: %s\r\n", arg3.c_str(  ) );
         else
            morph->no_suscept.flip( value );
      }
   }
   else if( !str_cmp( arg2, "noimmune" ) || !str_cmp( arg2, "ni" ) )
   {
      if( argument.empty(  ) )
      {
         ch->print( "Usage: morphset <morph> noimmune <flag> [flag]...\r\n" );
         return;
      }

      while( !argument.empty(  ) )
      {
         argument = one_argument( argument, arg3 );
         value = get_risflag( arg3 );
         if( value < 0 || value >= MAX_RIS_FLAG )
            ch->printf( "Unknown flag: %s\r\n", arg3.c_str(  ) );
         else
            morph->no_immune.flip( value );
      }
   }
   else if( !str_cmp( arg2, "affected" ) || !str_cmp( arg2, "aff" ) )
   {
      if( argument.empty(  ) )
      {
         ch->print( "Usage: morphset <morph> affected <flag> [flag]...\r\n" );
         return;
      }

      while( !argument.empty(  ) )
      {
         argument = one_argument( argument, arg3 );
         value = get_aflag( arg3 );
         if( value < 0 || value >= MAX_AFFECTED_BY )
            ch->printf( "Unknown flag: %s\r\n", arg3.c_str(  ) );
         else
            morph->affected_by.flip( value );
      }
   }
   else if( !str_cmp( arg2, "noaffected" ) || !str_cmp( arg2, "naff" ) )
   {
      if( argument.empty(  ) )
      {
         ch->print( "Usage: morphset <morph> noaffected <flag> [flag]...\r\n" );
         return;
      }

      while( !argument.empty(  ) )
      {
         argument = one_argument( argument, arg3 );
         value = get_aflag( arg3 );
         if( value < 0 || value >= MAX_AFFECTED_BY )
            ch->printf( "Unknown flag: %s\r\n", arg3.c_str(  ) );
         else
            morph->no_affected_by.flip( value );
      }
   }
   else if( !str_cmp( arg2, "short" ) )
   {
      DISPOSE( morph->short_desc );
      morph->short_desc = str_dup( arg3.c_str(  ) );
   }
   else if( !str_cmp( arg2, "morphother" ) )
   {
      DISPOSE( morph->morph_other );
      morph->morph_other = str_dup( arg3.c_str(  ) );
   }
   else if( !str_cmp( arg2, "morphself" ) )
   {
      DISPOSE( morph->morph_self );
      morph->morph_self = str_dup( arg3.c_str(  ) );
   }
   else if( !str_cmp( arg2, "unmorphother" ) )
   {
      DISPOSE( morph->unmorph_other );
      morph->unmorph_other = str_dup( arg3.c_str(  ) );
   }
   else if( !str_cmp( arg2, "unmorphself" ) )
   {
      DISPOSE( morph->unmorph_self );
      morph->unmorph_self = str_dup( arg3.c_str(  ) );
   }
   else if( !str_cmp( arg2, "keyword" ) )
   {
      DISPOSE( morph->key_words );
      morph->key_words = str_dup( arg3.c_str(  ) );
   }
   else if( !str_cmp( arg2, "long" ) )
      strdup_printf( &morph->long_desc, "%s\r\n", arg3.c_str(  ) );
   else if( !str_cmp( arg2, "description" ) || !str_cmp( arg2, "desc" ) )
   {
      if( !arg3.empty(  ) )
      {
         DISPOSE( morph->description );
         morph->description = str_dup( arg3.c_str(  ) );
      }
      ch->CHECK_SUBRESTRICTED(  );
      if( ch->substate == SUB_REPEATCMD )
         ch->tempnum = SUB_REPEATCMD;
      else
         ch->tempnum = SUB_NONE;
      ch->substate = SUB_MORPH_DESC;
      ch->pcdata->dest_buf = morph;
      if( !morph->description || morph->description[0] == '\0' )
         morph->description = str_dup( "" );
      ch->set_editor_desc( "A morph description." );
      ch->start_editing( morph->description );
      return;
   }
   else if( !str_cmp( arg2, "name" ) )
   {
      DISPOSE( morph->name );
      morph->name = str_dup( arg3.c_str(  ) );
   }
   else if( !str_cmp( arg2, "help" ) )
   {
      if( !arg3.empty(  ) )
      {
         DISPOSE( morph->help );
         morph->help = str_dup( arg3.c_str(  ) );
      }
      ch->CHECK_SUBRESTRICTED(  );
      if( ch->substate == SUB_REPEATCMD )
         ch->tempnum = SUB_REPEATCMD;
      else
         ch->tempnum = SUB_NONE;
      ch->substate = SUB_MORPH_HELP;
      ch->pcdata->dest_buf = morph;
      if( !morph->help || morph->help[0] == '\0' )
         morph->help = str_dup( "" );
      ch->set_editor_desc( "A morph help file." );
      ch->start_editing( morph->help );
      return;
   }
   else if( !str_cmp( arg2, "skills" ) )
   {
      if( arg3.empty(  ) || !str_cmp( arg3, "none" ) )
      {
         DISPOSE( morph->skills );
         return;
      }
      if( !morph->skills )
         mudstrlcpy( buf, arg3.c_str(  ), MSL );
      else
         snprintf( buf, MSL, "%s %s", morph->skills, arg3.c_str(  ) );
      DISPOSE( morph->skills );
      morph->skills = str_dup( buf );
   }
   else if( !str_cmp( arg2, "noskills" ) )
   {
      if( arg3.empty(  ) || !str_cmp( arg3, "none" ) )
      {
         DISPOSE( morph->no_skills );
         return;
      }
      if( !morph->no_skills )
         mudstrlcpy( buf, arg3.c_str(  ), MSL );
      else
         snprintf( buf, MSL, "%s %s", morph->no_skills, arg3.c_str(  ) );
      DISPOSE( morph->no_skills );
      morph->no_skills = str_dup( buf );
   }
   else if( !str_cmp( arg2, "Class" ) )
   {
      value = get_pc_class( arg3 );

      if( value < 0 || value >= MAX_PC_CLASS )
      {
         ch->printf( "Unknown PC Class: %s", arg3.c_str(  ) );
         return;
      }
      morph->Class.flip( value );
   }
   else if( !str_cmp( arg2, "race" ) )
   {
      value = get_pc_race( arg3 );

      if( value < 0 || value >= MAX_PC_RACE )
      {
         ch->printf( "Unknown PC race: %s", arg3.c_str(  ) );
         return;
      }
      morph->race.flip( value );
   }
   else if( ch->substate == SUB_REPEATCMD )
   {
      ch->substate = SUB_RESTRICTED;
      interpret( ch, origarg );
      ch->substate = SUB_REPEATCMD;
      ch->last_cmd = do_morphset;
   }
   else
   {
      do_morphset( ch, "" );
      return;
   }
   ch->print( "Done.\r\n" );
}

/*
 *  Shows morph info on a given morph.
 *  To see the description and help file, must use morphstat <morph> help
 *  Shaddai
 */
CMDF( do_morphstat )
{
   morph_data *morph;
   string arg;
   int count = 1;

   ch->set_pager_color( AT_CYAN );

   argument = one_argument( argument, arg );
   if( arg.empty(  ) )
   {
      ch->print( "Morphstat what?\r\n" );
      return;
   }

   if( ch->isnpc(  ) )
   {
      ch->print( "Mob's can't morphstat\r\n" );
      return;
   }

   if( !str_cmp( arg, "list" ) )
   {
      list < morph_data * >::iterator ipoly;

      if( morphlist.empty(  ) )
      {
         ch->print( "No morph's currently exist.\r\n" );
         return;
      }
      for( ipoly = morphlist.begin(  ); ipoly != morphlist.end(  ); ++ipoly )
      {
         morph_data *poly = *ipoly;

         ch->pagerf( "&c[&C%2d&c]   Name:  &C%-13s    &cVnum:  &C%4d  &cUsed:  &C%3d\r\n", count, poly->name, poly->vnum, poly->used );
         ++count;
      }
      return;
   }
   if( !is_number( arg ) )
      morph = get_morph( arg );
   else
      morph = get_morph_vnum( atoi( arg.c_str(  ) ) );

   if( morph == nullptr )
   {
      ch->print( "No such morph exists.\r\n" );
      return;
   }

   if( argument.empty(  ) )
   {
      ch->pagerf( "  &cMorph Name: &C%-20s  Vnum: %4d\r\n", morph->name, morph->vnum );
      ch->pager( "&B[----------------------------------------------------------------------------]\r\n" );
      ch->pager( "                           &BMorph Restrictions\r\n" );
      ch->pager( "&B[----------------------------------------------------------------------------]\r\n" );
      ch->pagerf( "  &cClasses Allowed   : &w%s\r\n", bitset_string( morph->Class, npc_class ) );
      ch->pagerf( "  &cRaces Not Allowed: &w%s\r\n", bitset_string( morph->race, npc_race ) );
      ch->pagerf( "  &cSex:  &C%s   &cPkill:   &C%s   &cTime From:   &C%d   &cTime To:    &C%d\r\n",
                  morph->sex == SEX_MALE ? "male" :
                  morph->sex == SEX_FEMALE ? "female" : "neutral",
                  morph->pkill == ONLY_PKILL ? "YES" : morph->pkill == ONLY_PEACEFULL ? "NO" : "n/a", morph->timefrom, morph->timeto );
      ch->pagerf( "  &cDay From:  &C%d  &cDay To:  &C%d\r\n", morph->dayfrom, morph->dayto );
      ch->pagerf( "  &cLevel:  &C%d       &cMorph Via Spell   : &C%s\r\n", morph->level, ( morph->no_cast ) ? "NO" : "yes" );
      ch->pagerf( " &cCasting Allowed   : &C%s\r\n", ( morph->cast_allowed ? "Yes" : "No" ) );
      ch->pagerf( "  &cUSAGES:  Mana:  &C%d  &cMove:  &C%d  &cHp:  &C%d  &cFavour:  &C%d\r\n", morph->manaused, morph->moveused, morph->hpused, morph->favourused );
      ch->pagerf( "  &cObj1: &C%d  &cObjuse1: &C%s   &cObj2: &C%d  &cObjuse2: &C%s   &cObj3: &C%d  &cObjuse3: &c%s\r\n",
                  morph->obj[0], ( morph->objuse[0] ? "YES" : "no" ), morph->obj[1], ( morph->objuse[1] ? "YES" : "no" ), morph->obj[2], ( morph->objuse[2] ? "YES" : "no" ) );
      ch->pagerf( "  &cTimer: &w%d\r\n", morph->timer );
      ch->pager( "&B[----------------------------------------------------------------------------]\r\n" );
      ch->pager( "                       &BEnhancements to the Player\r\n" );
      ch->pager( "&B[----------------------------------------------------------------------------]\r\n" );
      ch->
         pagerf
         ( "  &cStr: &C%2d&c )( Int: &C%2d&c )( Wis: &C%2d&c )( Dex: &C%2d&c )( Con: &C%2d&c )( Cha: &C%2d&c )( Lck: &C%2d&c )\r\n",
           morph->str, morph->inte, morph->wis, morph->dex, morph->con, morph->cha, morph->lck );
      ch->pagerf( "  &cSave versus: &w%d %d %d %d %d       &cDodge: &w%d  &cParry: &w%d  &cTumble: &w%d\r\n",
                  morph->saving_poison_death, morph->saving_wand, morph->saving_para_petri, morph->saving_breath,
                  morph->saving_spell_staff, morph->dodge, morph->parry, morph->tumble );
      ch->pagerf( "  &cHps     : &w%s    &cMana   : &w%s    &cMove      : &w%s\r\n", morph->hit, morph->mana, morph->move );
      ch->pagerf( "  &cDamroll : &w%s    &cHitroll: &w%s    &cAC     : &w%d\r\n", morph->damroll, morph->hitroll, morph->ac );
      ch->pager( "&B[----------------------------------------------------------------------------]\r\n" );
      ch->pager( "                          &BAffects to the Player\r\n" );
      ch->pager( "&B[----------------------------------------------------------------------------]\r\n" );
      ch->pagerf( "  &cAffected by: &C%s\r\n", bitset_string( morph->affected_by, aff_flags ) );
      ch->pagerf( "  &cImmune     : &w%s\r\n", bitset_string( morph->immune, ris_flags ) );
      ch->pagerf( "  &cSusceptible: &w%s\r\n", bitset_string( morph->suscept, ris_flags ) );
      ch->pagerf( "  &cResistant  : &w%s\r\n", bitset_string( morph->resistant, ris_flags ) );
      ch->pagerf( "  &cAbsorb     : &w%s\r\n", bitset_string( morph->absorb, ris_flags ) );
      ch->pagerf( "  &cSkills     : &w%s\r\n", morph->skills );
      ch->pager( "&B[----------------------------------------------------------------------------]\r\n" );
      ch->pager( "                     &BPrevented affects to the Player\r\n" );
      ch->pager( "&B[----------------------------------------------------------------------------]\r\n" );
      ch->pagerf( "  &cNot affected by: &C%s\r\n", bitset_string( morph->no_affected_by, aff_flags ) );
      ch->pagerf( "  &cNot Immune     : &w%s\r\n", bitset_string( morph->no_immune, ris_flags ) );
      ch->pagerf( "  &cNot Susceptible: &w%s\r\n", bitset_string( morph->no_suscept, ris_flags ) );
      ch->pagerf( "  &cNot Resistant  : &w%s\r\n", bitset_string( morph->no_resistant, ris_flags ) );
      ch->pagerf( "  &cNot Skills     : &w%s\r\n", morph->no_skills );
      ch->pager( "&B[----------------------------------------------------------------------------]\r\n" );
      ch->pager( "\r\n" );
   }
   else if( !str_cmp( argument, "help" ) || !str_cmp( argument, "desc" ) )
   {
      ch->pagerf( "  &cMorph Name  : &C%-20s\r\n", morph->name );
      ch->pagerf( "  &cDefault Pos : &w%d\r\n", morph->defpos );
      ch->pagerf( "  &cKeywords    : &w%s\r\n", morph->key_words );
      ch->pagerf( "  &cShortdesc   : &w%s\r\n", ( morph->short_desc && morph->short_desc[0] == '\0' ) ? "(none set)" : morph->short_desc );
      ch->pagerf( "  &cLongdesc    : &w%s", ( morph->long_desc && morph->long_desc[0] == '\0' ) ? "(none set)\r\n" : morph->long_desc );
      ch->pagerf( "  &cMorphself   : &w%s\r\n", morph->morph_self );
      ch->pagerf( "  &cMorphother  : &w%s\r\n", morph->morph_other );
      ch->pagerf( "  &cUnMorphself : &w%s\r\n", morph->unmorph_self );
      ch->pagerf( "  &cUnMorphother: &w%s\r\n", morph->unmorph_other );
      ch->pager( "&B[----------------------------------------------------------------------------]\r\n" );
      ch->pagerf( "                                  &cHelp:\r\n&w%s\r\n", morph->help );
      ch->pager( "&B[----------------------------------------------------------------------------]\r\n" );
      ch->pagerf( "                               &cDescription:\r\n&w%s\r\n", morph->description );
      ch->pager( "&B[----------------------------------------------------------------------------]\r\n" );
   }
   else
   {
      ch->print( "Syntax: morphstat <morph>\r\n" );
      ch->print( "Syntax: morphstat <morph> <help/desc>\r\n" );
   }
}

/*
 * This function sends the morph/unmorph message to the people in the room.
 * -- Shaddai
 */
void send_morph_message( char_data * ch, morph_data * morph, bool is_morph )
{
   if( morph == nullptr )
      return;
   if( is_morph )
   {
      act( AT_MORPH, morph->morph_other, ch, nullptr, nullptr, TO_ROOM );
      act( AT_MORPH, morph->morph_self, ch, nullptr, nullptr, TO_CHAR );
   }
   else
   {
      act( AT_MORPH, morph->unmorph_other, ch, nullptr, nullptr, TO_ROOM );
      act( AT_MORPH, morph->unmorph_self, ch, nullptr, nullptr, TO_CHAR );
   }
}

/*
 * Create new player morph, a scailed down version of original morph
 * so if morph gets changed stats don't get messed up.
 */
char_morph *make_char_morph( morph_data * morph )
{
   char_morph *ch_morph;

   if( morph == nullptr )
      return nullptr;

   ch_morph = new char_morph;
   ch_morph->morph = morph;
   ch_morph->ac = morph->ac;
   ch_morph->str = morph->str;
   ch_morph->inte = morph->inte;
   ch_morph->wis = morph->wis;
   ch_morph->dex = morph->dex;
   ch_morph->cha = morph->cha;
   ch_morph->lck = morph->lck;
   ch_morph->saving_breath = morph->saving_breath;
   ch_morph->saving_para_petri = morph->saving_para_petri;
   ch_morph->saving_poison_death = morph->saving_poison_death;
   ch_morph->saving_spell_staff = morph->saving_spell_staff;
   ch_morph->saving_wand = morph->saving_wand;
   ch_morph->timer = morph->timer;
   ch_morph->hitroll = 0;
   ch_morph->damroll = 0;
   ch_morph->hit = 0;
   ch_morph->mana = 0;
   ch_morph->move = 0;
   ch_morph->affected_by = morph->affected_by;
   ch_morph->immune = morph->immune;
   ch_morph->absorb = morph->absorb;
   ch_morph->resistant = morph->resistant;
   ch_morph->suscept = morph->suscept;
   ch_morph->no_affected_by = morph->no_affected_by;
   ch_morph->no_immune = morph->no_immune;
   ch_morph->no_resistant = morph->no_resistant;
   ch_morph->no_suscept = morph->no_suscept;
   ch_morph->cast_allowed = morph->cast_allowed;
   return ch_morph;
}

/*
 * Workhorse of the polymorph code, this turns the player into the proper
 * morph.  Doesn't check if you can morph or not so must use can_morph before
 * this is called.  That is so we can let people morph into things without
 * checking. Also, if anything is changed here, make sure it gets changed
 * in do_unmorph otherwise you will be giving your player stats for free.
 * This also does not send the message to people when you morph good to
 * use in save functions.
 * --Shaddai
 */
void do_morph( char_data * ch, morph_data * morph )
{
   char_morph *ch_morph;

   if( !morph )
      return;

   if( ch->morph )
      do_unmorph_char( ch );

   ch_morph = make_char_morph( morph );
   ch->armor += morph->ac;
   ch->mod_str += morph->str;
   ch->mod_int += morph->inte;
   ch->mod_wis += morph->wis;
   ch->mod_dex += morph->dex;
   ch->mod_cha += morph->cha;
   ch->mod_lck += morph->lck;
   ch->mod_con += morph->con; /* Was missing. Caught by Matteo2303 */
   ch->saving_breath += morph->saving_breath;
   ch->saving_para_petri += morph->saving_para_petri;
   ch->saving_poison_death += morph->saving_poison_death;
   ch->saving_spell_staff += morph->saving_spell_staff;
   ch->saving_wand += morph->saving_wand;
   ch_morph->hitroll = morph->hitroll ? dice_parse( ch, morph->level, morph->hitroll ) : 0;
   ch->hitroll += ch_morph->hitroll;
   ch_morph->damroll = morph->damroll ? dice_parse( ch, morph->level, morph->damroll ) : 0;
   ch->damroll += ch_morph->damroll;
   ch_morph->hit = morph->hit ? dice_parse( ch, morph->level, morph->hit ) : 0;
   if( ( ch->hit + ch_morph->hit ) > 32700 )
      ch_morph->hit = ( 32700 - ch->hit );
   ch->hit += ch_morph->hit;
   ch_morph->move = morph->move ? dice_parse( ch, morph->level, morph->move ) : 0;
   if( ( ch->move + ch_morph->move ) > 32700 )
      ch_morph->move = ( 32700 - ch->move );
   ch->move += ch_morph->move;

   ch_morph->mana = morph->mana ? dice_parse( ch, morph->level, morph->mana ) : 0;
   if( ( ch->mana + ch_morph->mana ) > 32700 )
      ch_morph->mana = ( 32700 - ch->mana );
   ch->mana += ch_morph->mana;

   ch->get_aflags(  ) |= morph->affected_by;
   ch->get_immunes(  ) |= morph->immune;
   ch->get_resists(  ) |= morph->resistant;
   ch->get_susceps(  ) |= morph->suscept;
   ch->get_absorbs(  ) |= morph->absorb;
   ch->get_aflags(  ) &= ~( morph->no_affected_by );
   ch->get_immunes(  ) &= ~( morph->no_immune );
   ch->get_resists(  ) &= ~( morph->no_resistant );
   ch->get_susceps(  ) &= ~( morph->no_suscept );
   ch->morph = ch_morph;
   ++morph->used;
}

/*
 * These functions wrap the sending of morph stuff, with morphing the code.
 * In most cases this is what should be called in the code when using spells,
 * obj morphing, etc... --Shaddai
 */
int do_morph_char( char_data * ch, morph_data * morph )
{
   bool canmorph = true;
   obj_data *obj, *tmpobj;

   if( ch->morph )
      canmorph = false;

   if( morph->obj[0] )
   {
      if( !( obj = ch->get_obj_vnum( morph->obj[0] ) ) )
         canmorph = false;
      else if( morph->objuse[0] )
      {
         act( AT_OBJECT, "$p disappears in a whisp of smoke!", obj->carried_by, obj, nullptr, TO_CHAR );
         if( obj == obj->carried_by->get_eq( WEAR_WIELD ) && ( tmpobj = obj->carried_by->get_eq( WEAR_DUAL_WIELD ) ) != nullptr )
            tmpobj->wear_loc = WEAR_WIELD;
         obj->separate(  );
         obj->extract(  );
      }
   }

   if( morph->obj[1] )
   {
      if( !( obj = ch->get_obj_vnum( morph->obj[1] ) ) )
         canmorph = false;
      else if( morph->objuse[1] )
      {
         act( AT_OBJECT, "$p disappears in a whisp of smoke!", obj->carried_by, obj, nullptr, TO_CHAR );
         if( obj == obj->carried_by->get_eq( WEAR_WIELD ) && ( tmpobj = obj->carried_by->get_eq( WEAR_DUAL_WIELD ) ) != nullptr )
            tmpobj->wear_loc = WEAR_WIELD;
         obj->separate(  );
         obj->extract(  );
      }
   }

   if( morph->obj[2] )
   {
      if( !( obj = ch->get_obj_vnum( morph->obj[2] ) ) )
         canmorph = false;
      else if( morph->objuse[2] )
      {
         act( AT_OBJECT, "$p disappears in a whisp of smoke!", obj->carried_by, obj, nullptr, TO_CHAR );
         if( obj == obj->carried_by->get_eq( WEAR_WIELD ) && ( tmpobj = obj->carried_by->get_eq( WEAR_DUAL_WIELD ) ) != nullptr )
            tmpobj->wear_loc = WEAR_WIELD;
         obj->separate(  );
         obj->extract(  );
      }
   }

   if( morph->hpused )
   {
      if( ch->hit < morph->hpused )
         canmorph = false;
      else
         ch->hit -= morph->hpused;
   }

   if( morph->moveused )
   {
      if( ch->move < morph->moveused )
         canmorph = false;
      else
         ch->move -= morph->moveused;
   }

   if( morph->manaused )
   {
      if( ch->mana < morph->manaused )
         canmorph = false;
      else
         ch->mana -= morph->manaused;
   }

   if( morph->favourused )
   {
      if( ch->isnpc(  ) || !ch->pcdata->deity || ch->pcdata->favor < morph->favourused )
         canmorph = false;
      else
      {
         ch->pcdata->favor -= morph->favourused;
         ch->adjust_favor( -1, 1 );
      }
   }

   if( !canmorph )
   {
      ch->print( "You begin to transform, but something goes wrong.\r\n" );
      return false;
   }
   send_morph_message( ch, morph, true );
   do_morph( ch, morph );
   return true;
}

/*
 * This makes sure to take all the affects given to the player by the morph
 * away.  Several things to keep in mind.  If you add something here make
 * sure you add it to do_morph as well (Unless you want to charge your players
 * for morphing ;) ).  Also make sure that their pfile saves with the morph
 * affects off, as the morph may not exist when they log back in.  This
 * function also does not send the message to people when you morph so it is
 * good to use in save functions and other places you don't want people to
 * see the stuff.
 * --Shaddai
 */
void do_unmorph( char_data * ch )
{
   char_morph *morph;

   if( !( morph = ch->morph ) )
      return;

   ch->armor -= morph->ac;
   ch->mod_str -= morph->str;
   ch->mod_int -= morph->inte;
   ch->mod_wis -= morph->wis;
   ch->mod_dex -= morph->dex;
   ch->mod_cha -= morph->cha;
   ch->mod_lck -= morph->lck;
   ch->saving_breath -= morph->saving_breath;
   ch->saving_para_petri -= morph->saving_para_petri;
   ch->saving_poison_death -= morph->saving_poison_death;
   ch->saving_spell_staff -= morph->saving_spell_staff;
   ch->saving_wand -= morph->saving_wand;
   ch->hitroll -= morph->hitroll;
   ch->damroll -= morph->damroll;
   ch->hit -= morph->hit;
   ch->move -= morph->move;
   ch->mana -= morph->mana;
   /*
    * Added by Tarl 21 Mar 02 to fix polymorph massive mana bug 
    */
   if( ch->mana > ch->max_mana )
      ch->mana = ch->max_mana;
   ch->get_aflags(  ) &= ~( morph->affected_by );
   ch->get_immunes(  ) &= ~( morph->immune );
   ch->get_resists(  ) &= ~( morph->resistant );
   ch->get_susceps(  ) &= ~( morph->suscept );
   ch->get_absorbs(  ) &= ~( morph->absorb );
   deleteptr( ch->morph );
   ch->update_aris(  );
}

void do_unmorph_char( char_data * ch )
{
   morph_data *temp;

   if( !ch->morph )
      return;

   temp = ch->morph->morph;
   do_unmorph( ch );
   send_morph_message( ch, temp, false );
}

/* Morph revert command ( God only knows why the Smaugers left this out ) - Samson 6-14-99 */
CMDF( do_revert )
{
   if( !ch->morph )
   {
      ch->print( "But you aren't polymorphed?!?\r\n" );
      return;
   }
   do_unmorph_char( ch );
}

void setup_morph_vnum( void )
{
   list < morph_data * >::iterator imorph;
   int vnum = morph_vnum;

   for( imorph = morphlist.begin(  ); imorph != morphlist.end(  ); ++imorph )
   {
      morph_data *morph = *imorph;

      if( morph->vnum > vnum )
         vnum = morph->vnum;
   }
   if( vnum < 1000 )
      vnum = 1000;
   else
      ++vnum;

   for( imorph = morphlist.begin(  ); imorph != morphlist.end(  ); ++imorph )
   {
      morph_data *morph = *imorph;

      if( morph->vnum == 0 )
      {
         morph->vnum = vnum++;
      }
   }
   morph_vnum = vnum;
}

/*
 * This function set's up all the default values for a morph
 */
void morph_defaults( morph_data * morph )
{
   morph->affected_by.reset(  );
   morph->name = nullptr;
   morph->Class.reset(  );
   morph->sex = -1;
   morph->timefrom = -1;
   morph->timeto = -1;
   morph->dayfrom = -1;
   morph->dayto = -1;
   morph->pkill = 0;
   morph->manaused = 0;
   morph->moveused = 0;
   morph->hpused = 0;
   morph->favourused = 0;
   morph->immune.reset(  );
   morph->absorb.reset(  );
   morph->no_affected_by.reset(  );
   morph->no_immune.reset(  );
   morph->no_resistant.reset(  );
   morph->no_suscept.reset(  );
   morph->obj[0] = 0;
   morph->obj[1] = 0;
   morph->obj[2] = 0;
   morph->objuse[0] = false;
   morph->objuse[1] = false;
   morph->objuse[2] = false;
   morph->race.reset(  );
   morph->resistant.reset(  );
   morph->suscept.reset(  );
   morph->used = 0;
   morph->ac = 0;
   morph->defpos = POS_STANDING;
   morph->dex = 0;
   morph->dodge = 0;
   morph->cha = 0;
   morph->con = 0;
   morph->inte = 0;
   morph->lck = 0;
   morph->level = 0;
   morph->parry = 0;
   morph->saving_breath = 0;
   morph->saving_para_petri = 0;
   morph->saving_poison_death = 0;
   morph->saving_spell_staff = 0;
   morph->saving_wand = 0;
   morph->str = 0;
   morph->tumble = 0;
   morph->wis = 0;
   morph->no_cast = false;
   morph->cast_allowed = false;
   morph->timer = -1;
   morph->vnum = 0;
}

/*
 * Read in one morph structure
 */
morph_data *fread_morph( FILE * fp )
{
   morph_data *morph;
   string arg, temp;
   int i, file_ver = 0;

   const char *word = ( feof( fp ) ? "End" : fread_word( fp ) );

   if( word[0] == '\0' )
   {
      bug( "%s: EOF encountered reading file!", __func__ );
      word = "End";
   }

   if( !str_cmp( word, "End" ) )
      return nullptr;

   morph = new morph_data;
   morph_defaults( morph );
   morph->name = str_dup( word );

   for( ;; )
   {
      word = ( feof( fp ) ? "End" : fread_word( fp ) );

      if( word[0] == '\0' )
      {
         bug( "%s: EOF encountered reading file!", __func__ );
         word = "End";
      }

      switch ( UPPER( word[0] ) )
      {
         default:
            bug( "%s: no match: %s", __func__, word );
            fread_to_eol( fp );
            /*
             * Bailing out on this morph as something may be messed up 
             * * this is going to have lots of bugs from the load_morphs this
             * * way, but it is better than possibly having the memory messed
             * * up! --Shaddai
             */
            deleteptr( morph );
            return nullptr;
            break;

         case 'A':
            if( !str_cmp( word, "Absorb" ) )
            {
               if( file_ver < 1 )
                  morph->absorb = fread_number( fp );
               else
                  flag_set( fp, morph->absorb, ris_flags );
               break;
            }
            KEY( "Armor", morph->ac, fread_number( fp ) );
            if( !str_cmp( word, "Affected" ) )
            {
               if( file_ver < 1 )
                  morph->affected_by = fread_number( fp );
               else
                  flag_set( fp, morph->affected_by, aff_flags );
               break;
            }
            break;

         case 'C':
            KEY( "Charisma", morph->cha, fread_number( fp ) );
            if( !str_cmp( word, "Class" ) )
            {
               arg = fread_flagstring( fp );
               while( !arg.empty() )
               {
                  arg = one_argument( arg, temp );
                  for( i = 0; i < MAX_PC_CLASS; ++i )
                     if( !str_cmp( temp, class_table[i]->who_name ) )
                     {
                        morph->Class.set( i );
                        break;
                     }
               }
               break;
            }
            KEY( "Constitution", morph->con, fread_number( fp ) );
            if( !str_cmp( word, "CastAllowed" ) )
            {
               morph->cast_allowed = fread_number( fp );
               break;
            }
            break;

         case 'D':
            MKEY( "Damroll", morph->damroll, fread_string_nohash( fp ) );
            KEY( "DayFrom", morph->dayfrom, fread_number( fp ) );
            KEY( "DayTo", morph->dayto, fread_number( fp ) );
            KEY( "Defpos", morph->defpos, fread_number( fp ) );
            MKEY( "Description", morph->description, fread_string_nohash( fp ) );
            KEY( "Dexterity", morph->dex, fread_number( fp ) );
            KEY( "Dodge", morph->dodge, fread_number( fp ) );
            break;

         case 'E':
            if( !str_cmp( word, "End" ) )
               return morph;
            break;

         case 'F':
            KEY( "FavourUsed", morph->favourused, fread_number( fp ) );
            break;

         case 'H':
            MKEY( "Help", morph->help, fread_string_nohash( fp ) );
            MKEY( "Hit", morph->hit, fread_string_nohash( fp ) );
            MKEY( "Hitroll", morph->hitroll, fread_string_nohash( fp ) );
            KEY( "HpUsed", morph->hpused, fread_number( fp ) );
            break;

         case 'I':
            KEY( "Intelligence", morph->inte, fread_number( fp ) );
            if( !str_cmp( word, "Immune" ) )
            {
               if( file_ver < 1 )
                  morph->immune = fread_number( fp );
               else
                  flag_set( fp, morph->immune, ris_flags );
               break;
            }
            break;

         case 'K':
            MKEY( "Keywords", morph->key_words, fread_string_nohash( fp ) );
            break;

         case 'L':
            KEY( "Level", morph->level, fread_number( fp ) );
            MKEY( "Longdesc", morph->long_desc, fread_string_nohash( fp ) );
            KEY( "Luck", morph->lck, fread_number( fp ) );
            break;

         case 'M':
            MKEY( "Mana", morph->mana, fread_string_nohash( fp ) );
            KEY( "ManaUsed", morph->manaused, fread_number( fp ) );
            MKEY( "MorphOther", morph->morph_other, fread_string_nohash( fp ) );
            MKEY( "MorphSelf", morph->morph_self, fread_string_nohash( fp ) );
            MKEY( "Move", morph->move, fread_string_nohash( fp ) );  /* EEK! This was set wrong! Caught by Matteo 2303 */
            KEY( "MoveUsed", morph->moveused, fread_number( fp ) );
            break;

         case 'N':
            MKEY( "NoSkills", morph->no_skills, fread_string_nohash( fp ) );
            if( !str_cmp( word, "NoAffected" ) )
            {
               if( file_ver < 1 )
                  morph->no_affected_by = fread_number( fp );
               else
                  flag_set( fp, morph->no_affected_by, aff_flags );
               break;
            }

            if( !str_cmp( word, "NoResistant" ) )
            {
               if( file_ver < 1 )
                  morph->no_resistant = fread_number( fp );
               else
                  flag_set( fp, morph->no_resistant, ris_flags );
               break;
            }

            if( !str_cmp( word, "NoSuscept" ) )
            {
               if( file_ver < 1 )
                  morph->no_suscept = fread_number( fp );
               else
                  flag_set( fp, morph->no_suscept, ris_flags );
               break;
            }

            if( !str_cmp( word, "NoImmune" ) )
            {
               if( file_ver < 1 )
                  morph->no_immune = fread_number( fp );
               else
                  flag_set( fp, morph->no_immune, ris_flags );
               break;
            }
            KEY( "NoCast", morph->no_cast, fread_number( fp ) );
            break;

         case 'O':
            if( !str_cmp( word, "Objs" ) )
            {
               morph->obj[0] = fread_number( fp );
               morph->obj[1] = fread_number( fp );
               morph->obj[2] = fread_number( fp );
               break;
            }
            if( !str_cmp( word, "Objuse" ) )
            {
               morph->objuse[0] = fread_number( fp );
               morph->objuse[1] = fread_number( fp );
               morph->objuse[2] = fread_number( fp );
            }
            break;

         case 'P':
            KEY( "Parry", morph->parry, fread_number( fp ) );
            KEY( "Pkill", morph->pkill, fread_number( fp ) );
            break;

         case 'R':
            if( !str_cmp( word, "Race" ) )
            {
               arg = fread_flagstring( fp );
               arg = one_argument( arg, temp );
               while( temp[0] != '\0' )
               {
                  for( i = 0; i < MAX_PC_RACE; ++i )
                     if( !str_cmp( temp, race_table[i]->race_name ) )
                     {
                        morph->race.set( i );
                        break;
                     }
                  arg = one_argument( arg, temp );
               }
               break;
            }

            if( !str_cmp( word, "Resistant" ) )
            {
               if( file_ver < 1 )
                  morph->resistant = fread_number( fp );
               else
                  flag_set( fp, morph->resistant, ris_flags );
               break;
            }
            break;

         case 'S':
            KEY( "SaveBreath", morph->saving_breath, fread_number( fp ) );
            KEY( "SavePara", morph->saving_para_petri, fread_number( fp ) );
            KEY( "SavePoison", morph->saving_poison_death, fread_number( fp ) );
            KEY( "SaveSpell", morph->saving_spell_staff, fread_number( fp ) );
            KEY( "SaveWand", morph->saving_wand, fread_number( fp ) );
            KEY( "Sex", morph->sex, fread_number( fp ) );
            MKEY( "ShortDesc", morph->short_desc, fread_string_nohash( fp ) );
            MKEY( "Skills", morph->skills, fread_string_nohash( fp ) );
            KEY( "Strength", morph->str, fread_number( fp ) );

            if( !str_cmp( word, "Suscept" ) )
            {
               if( file_ver < 1 )
                  morph->suscept = fread_number( fp );
               else
                  flag_set( fp, morph->suscept, ris_flags );
               break;
            }
            break;

         case 'T':
            KEY( "Timer", morph->timer, fread_number( fp ) );
            KEY( "TimeFrom", morph->timefrom, fread_number( fp ) );
            KEY( "TimeTo", morph->timeto, fread_number( fp ) );
            KEY( "Tumble", morph->tumble, fread_number( fp ) );
            break;

         case 'U':
            MKEY( "UnmorphOther", morph->unmorph_other, fread_string_nohash( fp ) );
            MKEY( "UnmorphSelf", morph->unmorph_self, fread_string_nohash( fp ) );
            KEY( "Used", morph->used, fread_number( fp ) );
            break;

         case 'V':
            KEY( "Version", file_ver, fread_number( fp ) );
            KEY( "Vnum", morph->vnum, fread_number( fp ) );
            break;

         case 'W':
            KEY( "Wisdom", morph->wis, fread_number( fp ) );
            break;
      }
   }
}

/*
 *  This function loads in the morph data.  Note that this function MUST be
 *  used AFTER the race and Class tables have been read in and setup.
 *  --Shaddai
 */
void load_morphs( void )
{
   FILE *fp = nullptr;
   bool my_continue = true;

   if( !( fp = fopen( SYSTEM_DIR MORPH_FILE, "r" ) ) )
   {
      bug( "%s: fopen", __func__ );
      perror( SYSTEM_DIR MORPH_FILE );
      return;
   }

   while( my_continue )
   {
      morph_data *morph = nullptr;
      const char *word = ( feof( fp ) ? "#END" : fread_word( fp ) );

      if( word[0] == '\0' )
      {
         bug( "%s: EOF encountered reading file!", __func__ );
         word = "#END";
      }

      switch ( UPPER( word[0] ) )
      {
         default:
            bug( "%s: no match: %s", __func__, word );
            fread_to_eol( fp );
            break;

         case '#':
            if( !str_cmp( word, "#END" ) )
            {
               FCLOSE( fp );
               my_continue = false;
               break;
            }
            break;

         case 'M':
            if( !str_cmp( word, "Morph" ) )
               morph = fread_morph( fp );
            break;
      }
      if( morph )
         morphlist.push_back( morph );
   }
   setup_morph_vnum(  );
   log_string( "Done." );
}

/*
 * This function copys one morph structure to another
 */
void copy_morph( morph_data * morph, morph_data * temp )
{
   morph->damroll = str_dup( temp->damroll );
   morph->description = str_dup( temp->description );
   morph->help = str_dup( temp->help );
   morph->hit = str_dup( temp->hit );
   morph->hitroll = str_dup( temp->hitroll );
   morph->key_words = str_dup( temp->key_words );
   morph->long_desc = str_dup( temp->long_desc );
   morph->mana = str_dup( temp->mana );
   morph->morph_other = str_dup( temp->morph_other );
   morph->morph_self = str_dup( temp->morph_self );
   morph->move = str_dup( temp->move );
   morph->name = str_dup( temp->name );
   morph->short_desc = str_dup( temp->short_desc );
   morph->skills = str_dup( temp->skills );
   morph->no_skills = str_dup( temp->no_skills );
   morph->unmorph_other = str_dup( temp->unmorph_other );
   morph->unmorph_self = str_dup( temp->unmorph_self );
   morph->affected_by = temp->affected_by;
   morph->Class = temp->Class;
   morph->sex = temp->sex;
   morph->timefrom = temp->timefrom;
   morph->timeto = temp->timeto;
   morph->timefrom = temp->timefrom;
   morph->dayfrom = temp->dayfrom;
   morph->dayto = temp->dayto;
   morph->pkill = temp->pkill;
   morph->manaused = temp->manaused;
   morph->moveused = temp->moveused;
   morph->hpused = temp->hpused;
   morph->favourused = temp->favourused;
   morph->immune = temp->immune;
   morph->absorb = temp->absorb;
   morph->no_affected_by = temp->no_affected_by;
   morph->no_immune = temp->no_immune;
   morph->no_resistant = temp->no_resistant;
   morph->no_suscept = temp->no_suscept;
   morph->obj[0] = temp->obj[0];
   morph->obj[1] = temp->obj[1];
   morph->obj[2] = temp->obj[2];
   morph->objuse[0] = temp->objuse[0];
   morph->objuse[1] = temp->objuse[1];
   morph->objuse[2] = temp->objuse[2];
   morph->race = temp->race;
   morph->resistant = temp->resistant;
   morph->suscept = temp->suscept;
   morph->ac = temp->ac;
   morph->defpos = temp->defpos;
   morph->dex = temp->dex;
   morph->dodge = temp->dodge;
   morph->cha = temp->cha;
   morph->con = temp->con;
   morph->inte = temp->inte;
   morph->lck = temp->lck;
   morph->level = temp->level;
   morph->parry = temp->parry;
   morph->saving_breath = temp->saving_breath;
   morph->saving_para_petri = temp->saving_para_petri;
   morph->saving_poison_death = temp->saving_poison_death;
   morph->saving_spell_staff = temp->saving_spell_staff;
   morph->saving_wand = temp->saving_wand;
   morph->str = temp->str;
   morph->tumble = temp->tumble;
   morph->wis = temp->wis;
   morph->no_cast = temp->no_cast;
   morph->cast_allowed = temp->cast_allowed;
   morph->timer = temp->timer;
}

/*
 * Player command to create a new morph
 */
CMDF( do_morphcreate )
{
   morph_data *morph, *temp = nullptr;
   string arg1;

   argument = one_argument( argument, arg1 );

   if( arg1.empty(  ) )
   {
      ch->print( "Usage: morphcreate <name>\r\n" );
      ch->print( "Usage: morphcreate <name/vnum> copy\r\n" );
      return;
   }

   if( !argument.empty(  ) && !str_cmp( argument, "copy" ) )
   {
      if( is_number( arg1 ) )
      {
         if( !( temp = get_morph_vnum( atoi( arg1.c_str(  ) ) ) ) )
         {
            ch->printf( "No such morph vnum %d exists.\r\n", atoi( arg1.c_str(  ) ) );
            return;
         }
      }
      else if( !( temp = get_morph( arg1 ) ) )
      {
         ch->printf( "No such morph %s exists.\r\n", arg1.c_str(  ) );
         return;
      }
   }
   smash_tilde( arg1 );
   morph = new morph_data;
   morph_defaults( morph );
   if( !argument.empty(  ) && !str_cmp( argument, "copy" ) && temp )
      copy_morph( morph, temp );
   else
      morph->name = str_dup( arg1.c_str(  ) );
   if( !morph->short_desc || morph->short_desc[0] == '\0' )
      morph->short_desc = str_dup( arg1.c_str(  ) );
   morph->vnum = morph_vnum;
   ++morph_vnum;
   morphlist.push_back( morph );
   ch->printf( "Morph %s created with vnum %d.\r\n", morph->name, morph->vnum );
}

void unmorph_all( morph_data * morph )
{
   list < char_data * >::iterator ich;

   for( ich = pclist.begin(  ); ich != pclist.end(  ); ++ich )
   {
      char_data *vch = *ich;

      if( vch->morph == nullptr || vch->morph->morph == nullptr || vch->morph->morph != morph )
         continue;
      do_unmorph_char( vch );
   }
}

morph_data::morph_data(  )
{
   init_memory( &affected_by, &cast_allowed, sizeof( cast_allowed ) );
}

/* 
 * Function that releases all the memory for a morph struct.  Carefull not
 * to use the memory afterwards as it doesn't exist.
 * --Shaddai 
 */
morph_data::~morph_data(  )
{
   unmorph_all( this );

   DISPOSE( damroll );
   DISPOSE( description );
   DISPOSE( help );
   DISPOSE( hit );
   DISPOSE( hitroll );
   DISPOSE( key_words );
   DISPOSE( long_desc );
   DISPOSE( mana );
   DISPOSE( morph_other );
   DISPOSE( morph_self );
   DISPOSE( move );
   DISPOSE( name );
   DISPOSE( short_desc );
   DISPOSE( skills );
   DISPOSE( no_skills );
   DISPOSE( unmorph_other );
   DISPOSE( unmorph_self );
   morphlist.remove( this );
}

void free_morphs( void )
{
   list < morph_data * >::iterator morph;

   for( morph = morphlist.begin(  ); morph != morphlist.end(  ); )
   {
      morph_data *poly = *morph;
      ++morph;

      deleteptr( poly );
   }
}

/*
 * Player function to delete a morph. --Shaddai
 * NOTE Need to check all players and force them to unmorph first
 */
CMDF( do_morphdestroy )
{
   morph_data *morph;

   if( argument.empty(  ) )
   {
      ch->print( "Destroy which morph?\r\n" );
      return;
   }
   if( is_number( argument ) )
      morph = get_morph_vnum( atoi( argument.c_str(  ) ) );
   else
      morph = get_morph( argument );

   if( !morph )
   {
      ch->printf( "Unkown morph %s.\r\n", argument.c_str(  ) );
      return;
   }
   deleteptr( morph );
   ch->print( "Morph deleted.\r\n" );
}

void fwrite_morph_data( char_data * ch, FILE * fp )
{
   char_morph *morph;

   if( ch->morph == nullptr )
      return;

   morph = ch->morph;
   fprintf( fp, "%s", "#MorphData\n" );
   /*
    * Only Print Out what is necessary 
    */
   if( morph->morph != nullptr )
   {
      fprintf( fp, "Vnum %d\n", morph->morph->vnum );
      fprintf( fp, "Name %s~\n", morph->morph->name );
   }
   if( morph->affected_by.any(  ) )
      fprintf( fp, "Affect          %s~\n", bitset_string( morph->affected_by, aff_flags ) );
   if( morph->no_affected_by.any(  ) )
      fprintf( fp, "NoAffect        %s~\n", bitset_string( morph->no_affected_by, aff_flags ) );
   if( morph->immune.any(  ) )
      fprintf( fp, "Immune          %s~\n", bitset_string( morph->immune, ris_flags ) );
   if( morph->resistant.any(  ) )
      fprintf( fp, "Resistant       %s~\n", bitset_string( morph->resistant, ris_flags ) );
   if( morph->suscept.any(  ) )
      fprintf( fp, "Suscept         %s~\n", bitset_string( morph->suscept, ris_flags ) );
   if( morph->absorb.any(  ) )
      fprintf( fp, "Absorb          %s~\n", bitset_string( morph->absorb, ris_flags ) );
   if( morph->no_immune.any(  ) )
      fprintf( fp, "NoImmune        %s~\n", bitset_string( morph->no_immune, ris_flags ) );
   if( morph->no_resistant.any(  ) )
      fprintf( fp, "NoResistant     %s~\n", bitset_string( morph->no_resistant, ris_flags ) );
   if( morph->no_suscept.any(  ) )
      fprintf( fp, "NoSuscept       %s~\n", bitset_string( morph->no_suscept, ris_flags ) );
   if( morph->ac != 0 )
      fprintf( fp, "Armor           %d\n", morph->ac );
   if( morph->cha != 0 )
      fprintf( fp, "Charisma        %d\n", morph->cha );
   if( morph->con != 0 )
      fprintf( fp, "Constitution    %d\n", morph->con );
   if( morph->damroll != 0 )
      fprintf( fp, "Damroll         %d\n", morph->damroll );
   if( morph->dex != 0 )
      fprintf( fp, "Dexterity       %d\n", morph->dex );
   if( morph->dodge != 0 )
      fprintf( fp, "Dodge           %d\n", morph->dodge );
   if( morph->hit != 0 )
      fprintf( fp, "Hit             %d\n", morph->hit );
   if( morph->hitroll != 0 )
      fprintf( fp, "Hitroll         %d\n", morph->hitroll );
   if( morph->inte != 0 )
      fprintf( fp, "Intelligence    %d\n", morph->inte );
   if( morph->lck != 0 )
      fprintf( fp, "Luck            %d\n", morph->lck );
   if( morph->mana != 0 )
      fprintf( fp, "Mana            %d\n", morph->mana );
   if( morph->move != 0 )
      fprintf( fp, "Move            %d\n", morph->move );
   if( morph->parry != 0 )
      fprintf( fp, "Parry           %d\n", morph->parry );
   if( morph->saving_breath != 0 )
      fprintf( fp, "Save1           %d\n", morph->saving_breath );
   if( morph->saving_para_petri != 0 )
      fprintf( fp, "Save2           %d\n", morph->saving_para_petri );
   if( morph->saving_poison_death != 0 )
      fprintf( fp, "Save3           %d\n", morph->saving_poison_death );
   if( morph->saving_spell_staff != 0 )
      fprintf( fp, "Save4           %d\n", morph->saving_spell_staff );
   if( morph->saving_wand != 0 )
      fprintf( fp, "Save5           %d\n", morph->saving_wand );
   if( morph->str != 0 )
      fprintf( fp, "Strength        %d\n", morph->str );
   if( morph->timer != -1 )
      fprintf( fp, "Timer           %d\n", morph->timer );
   if( morph->tumble != 0 )
      fprintf( fp, "Tumble          %d\n", morph->tumble );
   if( morph->wis != 0 )
      fprintf( fp, "Wisdom          %d\n", morph->wis );
   fprintf( fp, "%s", "End\n" );
}

void clear_char_morph( char_morph * morph )
{
   morph->affected_by.reset(  );
   morph->no_affected_by.reset(  );
   morph->absorb.reset(  );
   morph->immune.reset(  );
   morph->no_immune.reset(  );
   morph->no_resistant.reset(  );
   morph->no_suscept.reset(  );
   morph->resistant.reset(  );
   morph->suscept.reset(  );
   morph->timer = -1;
   morph->ac = 0;
   morph->cha = 0;
   morph->con = 0;
   morph->damroll = 0;
   morph->dex = 0;
   morph->dodge = 0;
   morph->hit = 0;
   morph->hitroll = 0;
   morph->inte = 0;
   morph->lck = 0;
   morph->mana = 0;
   morph->parry = 0;
   morph->saving_breath = 0;
   morph->saving_para_petri = 0;
   morph->saving_poison_death = 0;
   morph->saving_spell_staff = 0;
   morph->saving_wand = 0;
   morph->str = 0;
   morph->tumble = 0;
   morph->wis = 0;
   morph->morph = nullptr;
}

void fread_morph_data( char_data * ch, FILE * fp )
{
   char_morph *morph = new char_morph;
   clear_char_morph( morph );
   ch->morph = morph;

   for( ;; )
   {
      const char *word = ( feof( fp ) ? "End" : fread_word( fp ) );

      if( word[0] == '\0' )
      {
         bug( "%s: EOF encountered reading file!", __func__ );
         word = "End";
      }

      switch ( UPPER( word[0] ) )
      {
         default:
            bug( "%s: no match: %s", __func__, word );
            fread_to_eol( fp );
            break;

         case 'A':
            if( !str_cmp( word, "Absorb" ) )
            {
               flag_set( fp, morph->absorb, ris_flags );
               break;
            }
            if( !str_cmp( word, "Affect" ) )
            {
               flag_set( fp, morph->affected_by, aff_flags );
               break;
            }
            KEY( "Armor", morph->ac, fread_number( fp ) );
            break;

         case 'C':
            KEY( "Charisma", morph->cha, fread_number( fp ) );
            KEY( "Constitution", morph->con, fread_number( fp ) );
            break;

         case 'D':
            KEY( "Damroll", morph->damroll, fread_number( fp ) );
            KEY( "Dexterity", morph->dex, fread_number( fp ) );
            KEY( "Dodge", morph->dodge, fread_number( fp ) );
            break;

         case 'E':
            if( !str_cmp( word, "End" ) )
               return;
            break;

         case 'H':
            KEY( "Hit", morph->hit, fread_number( fp ) );
            KEY( "Hitroll", morph->hitroll, fread_number( fp ) );
            break;

         case 'I':
            if( !str_cmp( word, "Immune" ) )
            {
               flag_set( fp, morph->immune, ris_flags );
               break;
            }
            KEY( "Intelligence", morph->inte, fread_number( fp ) );
            break;

         case 'L':
            KEY( "Luck", morph->lck, fread_number( fp ) );
            break;

         case 'M':
            KEY( "Mana", morph->mana, fread_number( fp ) );
            KEY( "Move", morph->move, fread_number( fp ) );
            break;

         case 'N':
            if( !str_cmp( "Name", word ) )
            {
               if( morph->morph )
                  if( str_cmp( morph->morph->name, fread_flagstring( fp ) ) )
                     bug( "%s: Morph Name doesn't match vnum %d.", __func__, morph->morph->vnum );
               break;
            }

            if( !str_cmp( word, "NoAffect" ) )
            {
               flag_set( fp, morph->no_affected_by, aff_flags );
               break;
            }

            if( !str_cmp( word, "NoResistant" ) )
            {
               flag_set( fp, morph->no_resistant, ris_flags );
               break;
            }

            if( !str_cmp( word, "NoSuscept" ) )
            {
               flag_set( fp, morph->no_suscept, ris_flags );
               break;
            }

            if( !str_cmp( word, "NoImmune" ) )
            {
               flag_set( fp, morph->no_immune, ris_flags );
               break;
            }
            break;

         case 'P':
            KEY( "Parry", morph->parry, fread_number( fp ) );
            break;

         case 'R':
            if( !str_cmp( word, "Resistant" ) )
            {
               flag_set( fp, morph->resistant, ris_flags );
               break;
            }
            break;

         case 'S':
            KEY( "Save1", morph->saving_breath, fread_number( fp ) );
            KEY( "Save2", morph->saving_para_petri, fread_number( fp ) );
            KEY( "Save3", morph->saving_poison_death, fread_number( fp ) );
            KEY( "Save4", morph->saving_spell_staff, fread_number( fp ) );
            KEY( "Save5", morph->saving_wand, fread_number( fp ) );
            KEY( "Strength", morph->str, fread_number( fp ) );
            if( !str_cmp( word, "Suscept" ) )
            {
               flag_set( fp, morph->suscept, ris_flags );
               break;
            }
            break;

         case 'T':
            KEY( "Timer", morph->timer, fread_number( fp ) );
            KEY( "Tumble", morph->tumble, fread_number( fp ) );
            break;

         case 'V':
            if( !str_cmp( "Vnum", word ) )
            {
               morph->morph = get_morph_vnum( fread_number( fp ) );
               break;
            }
            break;

         case 'W':
            KEY( "Wisdom", morph->wis, fread_number( fp ) );
            break;
      }
   }
}

/* 
 * Following functions are for immortal testing purposes.
 */
CMDF( do_imm_morph )
{
   morph_data *morph;
   char_data *victim = nullptr;
   string arg;
   int vnum;

   if( ch->isnpc(  ) )
   {
      ch->print( "Only player characters can use this command.\r\n" );
      return;
   }

   argument = one_argument( argument, arg );

   if( !is_number( arg ) )
   {
      ch->print( "Syntax: morph <vnum>\r\n" );
      return;
   }
   vnum = atoi( arg.c_str(  ) );
   morph = get_morph_vnum( vnum );

   if( morph == nullptr )
   {
      ch->printf( "No such morph %d exists.\r\n", vnum );
      return;
   }
   if( argument.empty(  ) )
      do_morph_char( ch, morph );
   else if( !( victim = ch->get_char_world( argument ) ) )
   {
      ch->print( "No one like that in all the realms.\r\n" );
      return;
   }
   if( victim != nullptr && ch->get_trust(  ) < victim->get_trust(  ) && !victim->isnpc(  ) )
   {
      ch->print( "You can't do that!\r\n" );
      return;
   }
   else if( victim != nullptr )
      do_morph_char( victim, morph );
   ch->print( "Done.\r\n" );
}

/*
 * This is just a wrapper.  --Shaddai
 */
CMDF( do_imm_unmorph )
{
   char_data *victim = nullptr;

   if( argument.empty(  ) )
      do_unmorph_char( ch );
   else if( !( victim = ch->get_char_world( argument ) ) )
   {
      ch->print( "No one like that in all the realms.\r\n" );
      return;
   }
   if( victim != nullptr && ch->get_trust(  ) < victim->get_trust(  ) && !victim->isnpc(  ) )
   {
      ch->print( "You can't do that!\r\n" );
      return;
   }
   else if( victim != nullptr )
      do_unmorph_char( victim );
   ch->print( "Done.\r\n" );
}

/* Added by Samson 6-13-99 - lists available polymorph forms */
CMDF( do_morphlist )
{
   list < morph_data * >::iterator imorph;

   ch->pager( "&GVnum |&YPolymorph Name\r\n" );
   ch->pager( "&G-----+----------------------------------\r\n" );

   for( imorph = morphlist.begin(  ); imorph != morphlist.end(  ); ++imorph )
   {
      morph_data *morph = *imorph;

      ch->pagerf( "&G%-5d  &Y%s\r\n", morph->vnum, morph->name );
   }
}
