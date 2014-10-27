/****************************************************************************
 * [S]imulated [M]edieval [A]dventure multi[U]ser [G]ame      |   \\._.//   *
 * -----------------------------------------------------------|   (0...0)   *
 * SMAUG 1.4 (C) 1994, 1995, 1996, 1998  by Derek Snider      |    ).:.(    *
 * -----------------------------------------------------------|    {o o}    *
 * SMAUG code team: Thoric, Altrag, Blodkai, Narn, Haus,      |   / ' ' \   *
 * Scryn, Rennard, Swordbearer, Gorog, Grishnakh, Nivek,      |~'~.VxvxV.~'~*
 * Tricops, Fireblade, Edmond, Conran                         |             *
 * ------------------------------------------------------------------------ *
 * Merc 2.1 Diku Mud improvments copyright (C) 1992, 1993 by Michael        *
 * Chastain, Michael Quan, and Mitchell Tse.                                *
 * Original Diku Mud copyright (C) 1990, 1991 by Sebastian Hammer,          *
 * Michael Seifert, Hans Henrik St{rfeldt, Tom Madsen, and Katja Nyboe.     *
 * ------------------------------------------------------------------------ *
 * 			Variable Handling Module (Thoric)                         *
 ****************************************************************************/

#include "mud.h"
#include "mobindex.h"
#include "variables.h"

variable_data::variable_data(  )
{
   init_memory( &this->tag, &this->timer, sizeof( this->timer ) );
}

variable_data::variable_data( int vtype, int vvnum, const string& vtag )
{
   this->type = vtype;
   // this->flags = 0;
   this->vnum = vvnum;

   this->tag = vtag;
   this->c_time = current_time;
   this->m_time = current_time;
   this->r_time = 0;
   this->timer = 0;

   switch ( vtype )
   {
      case vtINT:
      case vtSTR:
         this->data = NULL;
         break;
         /*
          * case vtXBIT: <--- FIXME: Convert to std::bitset
          * CREATE( this->data, EXT_BV, 1 );
          * break; 
          */
   }
}

variable_data::~variable_data(  )
{
   switch ( this->type )
   {
      case vtSTR:
         if( this->data )
         {
            delete( ( char * )this->data );
            this->data = NULL;
         }
         break;
   }
}

/*
 * Return the specified tag from a character
 */
variable_data *get_tag( char_data * ch, const string& tag, int vnum )
{
   list < variable_data * >::iterator ivd;

   for( ivd = ch->variables.begin(  ); ivd != ch->variables.end(  ); ++ivd )
   {
      variable_data *vd = *ivd;

      if( ( !vnum || vnum == vd->vnum ) && !str_cmp( tag, vd->tag ) )
         return vd;
   }
   return NULL;
}

/*
 * Remove the specified tag from a character
 */
bool remove_tag( char_data * ch, const string& tag, int vnum )
{
   list < variable_data * >::iterator ivd;
   bool deleted = false;

   if( ch->variables.empty(  ) )
      return false;

   for( ivd = ch->variables.begin(  ); ivd != ch->variables.end(  ); )
   {
      ++ivd;
      variable_data *vd = *ivd;

      if( ( !vnum || vnum == vd->vnum ) && !str_cmp( tag, vd->tag ) )
      {
         ch->variables.remove( vd );
         deleteptr( vd );
         deleted = true;
      }
   }

   if( deleted )
      return true;
   return false;
}

/*
 * Tag a variable onto a character  Will replace if specified to do so,
 * otherwise if already exists, fail
 */
int tag_char( char_data * ch, variable_data * var, bool replace )
{
   variable_data *vd = NULL;
   list < variable_data * >::iterator ivd;
   bool found = false;

   for( ivd = ch->variables.begin(  ); ivd != ch->variables.end(  ); ++ivd )
   {
      vd = *ivd;

      if( vd == var )   /* same variable -- leave it be */
      {
         var->m_time = current_time;
         return 0;
      }
      if( vd->vnum == var->vnum && !str_cmp( vd->tag, var->tag ) )
      {
         if( !replace )
            return -1;
         found = true;
         break;
      }
   }

   if( found )
   {
      var->m_time = current_time;
      var->c_time = vd->c_time;
      var->r_time = vd->r_time;

      ch->variables.push_back( var );
      ch->variables.remove( vd );
      deleteptr( vd );
      return 0;
   }
   ch->variables.push_back( var );
   return 0;
}

bool is_valid_tag( const string& tagname )
{
   string::const_iterator ptr = tagname.begin();

   if( !isalpha( tagname[0] ) )
      return false;

   while( ptr != tagname.end() )
   {
      if( !isalnum( *ptr ) && *ptr != '_' )
         return false;
      ++ptr;
   }
   return true;
}

/*
 *  "tag" is a text identifier to refer to the variable and can
 *  be suffixed with a colon and a mob vnum  ie:  questobj:1101
 *  vnum 0 is used to denote a global tag (local to the victim)
 *  otherwise tags are separated by vnum
 *
 *  mptag	<victim> <tag> [value]
 *  mprmtag	<victim> <tag>
 *  mpflag	<victim> <tag> <flag>
 *  mprmflag    <victim> <tag> <flag>
 *
 *  if istagged($n,tag) [== value]
 *  if isflagged($n,tag[,bit])
 */

/*
 * mptag <victim> <tag> [value]
 */
CMDF( do_mptag )
{
   string::const_iterator ptr;
   char_data *victim;
   variable_data *vd;
   char *p;
   string arg1, arg2;
   int vnum = 0, exp = 0;
   bool error = false;

   if( ( !ch->isnpc(  ) && ch->get_trust(  ) < LEVEL_GREATER ) || ch->is_affected( AFF_CHARM ) )
   {
      ch->print( "Huh?\r\n" );
      return;
   }

   argument = one_argument( argument, arg1 );
   if( !str_cmp( arg1, "noexpire" ) )
   {
      exp = 0;
      argument = one_argument( argument, arg1 );
   }
   else if( !str_cmp( arg1, "timer" ) )
   {
      argument = one_argument( argument, arg1 );
      exp = atoi( arg1.c_str() );
      argument = one_argument( argument, arg1 );
   }
   else
      exp = ch->level * ch->get_curr_int(  );
   argument = one_argument( argument, arg2 );

   if( arg1.empty() || arg2.empty() )
   {
      ch->print( "MPtag whom with what?\r\n" );
      return;
   }

   if( !( victim = ch->get_char_room( arg1 ) ) )
   {
      ch->print( "They aren't here.\r\n" );
      return;
   }

   if( ( p = strchr( arg2.c_str(), ':' ) ) != NULL )
   {
      *p++ = '\0';
      vnum = atoi( p );
   }
   else
      vnum = ch->pIndexData ? ch->pIndexData->vnum : 0;

   if( !is_valid_tag( arg2 ) )
   {
      progbug( "Mptag: invalid characters in tag", ch );
      return;
   }

   error = false;
   for( ptr = argument.begin(); ptr != argument.end(); ++ptr )
   {
      if( !isdigit( *ptr ) && !isspace( *ptr ) )
      {
         error = true;
         break;
      }
   }

   if( error )
   {
      vd = new variable_data( vtSTR, vnum, arg2 );
      vd->data = str_dup( argument.c_str() );
   }

   else
   {
      vd = new variable_data( vtINT, vnum, arg2 );
      vd->data = ( void * )( atol( argument.c_str() ) );
   }
   vd->timer = exp;
   tag_char( victim, vd, 1 );
}

/*
 * mprmtag <victim> <tag>
 */
CMDF( do_mprmtag )
{
   char_data *victim;
   char *p;
   string arg1, arg2;
   int vnum = 0;

   if( ( !ch->isnpc(  ) && ch->get_trust(  ) < LEVEL_GREATER ) || ch->is_affected( AFF_CHARM ) )
   {
      ch->print( "Huh?\r\n" );
      return;
   }

   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );

   if( arg1.empty() || arg2.empty() )
   {
      ch->print( "MPtag whom with what?\r\n" );
      return;
   }

   if( !( victim = ch->get_char_room( arg1 ) ) )
   {
      ch->print( "They aren't here.\r\n" );
      return;
   }

   if( ( p = strchr( arg2.c_str(), ':' ) ) != NULL )
   {
      *p++ = '\0';
      vnum = atoi( p );
   }
   else
      vnum = ch->pIndexData ? ch->pIndexData->vnum : 0;

   if( !is_valid_tag( arg2 ) )
   {
      progbug( "Mptag: invalid characters in tag", ch );
      return;
   }

   if( !remove_tag( victim, arg2, vnum ) )
      progbug( "Mptag: could not find tag", ch );
}

/*
 * mpflag <victim> <tag> <flag>
 * <--- FIXME: Convert to std::bitset
void do_mpflag( char_data * ch, string& argument )
{
   string::const_iterator ptr;
   char_data *victim;
   variable_data *vd;
   char *p;
   string arg1, arg2, arg3;
   int vnum = 0, exp = 0, def = 0, flag = 0;
   bool error = false;

   if( ( !IS_NPC( ch ) && get_trust( ch ) < LEVEL_GREATER ) || IS_AFFECTED( ch, AFF_CHARM ) )
   {
      send_to_char( "Huh?\r\n", ch );
      return;
   }

   argument = one_argument( argument, arg1 );
   if( !str_cmp( arg1, "noexpire" ) )
   {
      exp = 0;
      argument = one_argument( argument, arg1 );
   }
   else if( !str_cmp( arg1, "timer" ) )
   {
      argument = one_argument( argument, arg1 );
      exp = atoi( arg1.c_str() );
      argument = one_argument( argument, arg1 );
   }
   else
   {
      exp = ch->level * get_curr_int( ch );
      def = 1;
   }
   argument = one_argument( argument, arg2 );
   argument = one_argument( argument, arg3 );

   if( arg1.empty() || arg2.empty() || arg3.empty() )
   {
      send_to_char( "MPflag whom with what?\r\n", ch );
      return;
   }

   if( ( victim = get_char_room( ch, arg1 ) ) == NULL )
   {
      send_to_char( "They aren't here.\r\n", ch );
      return;
   }

   if( ( p = strchr( arg2.c_str(), ':' ) ) != NULL )
   {
      *p++ = '\0';
      vnum = atoi( p );
   }
   else
      vnum = ch->pIndexData ? ch->pIndexData->vnum : 0;

   if( !is_valid_tag( arg2 ) )
   {
      progbug( "Mpflag:  invalid characters in tag", ch );
      return;
   }

   error = false;
   for( ptr = arg3.begin(); ptr != arg3.end(); ++ptr )
   {
      if( !isdigit( *ptr ) && !isspace( *ptr ) )
      {
         error = true;
         break;
      }
   }

   flag = atoi( arg3.c_str() );
   if( error || flag < 0 || flag >= MAX_BITS )
   {
      progbug( "Mpflag:  invalid flag value", ch );
      return;
   }

   if( ( vd = get_tag( victim, arg2, vnum ) ) != NULL )
   {
      if( vd->type != vtXBIT )
      {
         progbug( "Mpflag:  type mismatch", ch );
         return;
      }
      if( !def )
         vd->timer = exp;
   }
   else
   {
      vd = make_variable( vtXBIT, vnum, arg2 );
      vd->timer = exp;
   }
   xSET_BIT( *( EXT_BV * ) vd->data, flag );
   tag_char( victim, vd, 1 );
} */

/*
 * mprmflag <victim> <tag> <flag>
 * <-- FIXME: Convert to std::bitset
void do_mprmflag( char_data * ch, string& argument )
{
   string::const_iterator ptr;
   char_data *victim;
   variable_data *vd;
   char *p;
   string arg1, arg2, arg3;
   int vnum = 0;
   bool error = false;

   if( ( !IS_NPC( ch ) && get_trust( ch ) < LEVEL_GREATER ) || IS_AFFECTED( ch, AFF_CHARM ) )
   {
      send_to_char( "Huh?\r\n", ch );
      return;
   }

   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );
   argument = one_argument( argument, arg3 );

   if( arg1.empty() || arg2.empty() || arg3.empty() )
   {
      send_to_char( "MPrmflag whom with what?\r\n", ch );
      return;
   }

   if( ( victim = get_char_room( ch, arg1 ) ) == NULL )
   {
      send_to_char( "They aren't here.\r\n", ch );
      return;
   }

   if( ( p = strchr( arg2.c_str(), ':' ) ) != NULL )
   {
      *p++ = '\0';
      vnum = atoi( p );
   }
   else
      vnum = ch->pIndexData ? ch->pIndexData->vnum : 0;

   if( !is_valid_tag( arg2 ) )
   {
      progbug( "Mprmflag:  invalid characters in tag", ch );
      return;
   }

   error = false;
   for( ptr = arg3.begin(); ptr != arg3.end(); ++ptr )
   {
      if( !isdigit( *ptr ) && !isspace( *ptr ) )
      {
         error = true;
         break;
      }
   }

   if( error )
   {
      progbug( "Mprmflag:  invalid flag value", ch );
      return;
   }

   *
    * Only bother doing anything if the tag exists
    *
   if( ( vd = get_tag( victim, arg2, vnum ) ) != NULL )
   {
      if( vd->type != vtXBIT )
      {
         progbug( "Mprmflag:  type mismatch", ch );
         return;
      }
      if( !vd->data )
      {
         progbug( "Mprmflag:  missing data???", ch );
         return;
      }
      xREMOVE_BIT( *( EXT_BV * ) vd->data, atoi( arg3 ) );
      tag_char( victim, vd, 1 );
   }
} */

void fwrite_variables( char_data * ch, FILE * fp )
{
   list < variable_data * >::iterator ivd;

   for( ivd = ch->variables.begin(  ); ivd != ch->variables.end(  ); ++ivd )
   {
      variable_data *vd = *ivd;

      fprintf( fp, "#VARIABLE\n" );
      fprintf( fp, "Type    %d\n", vd->type );
      // fprintf( fp, "Flags   %d\n", vd->flags );
      fprintf( fp, "Vnum    %d\n", vd->vnum );
      fprintf( fp, "Ctime   %ld\n", vd->c_time );
      fprintf( fp, "Mtime   %ld\n", vd->m_time );
      fprintf( fp, "Rtime   %ld\n", vd->r_time );
      fprintf( fp, "Timer   %d\n", vd->timer );
      fprintf( fp, "Tag     %s~\n", vd->tag.c_str() );
      switch ( vd->type )
      {
         case vtSTR:
            fprintf( fp, "Str     %s~\n", ( char * )vd->data );
            break;
            /*
             * case vtXBIT: FIXME: Convert to std::bitset
             * fprintf( fp, "Xbit    %s\n", print_bitvector( ( EXT_BV * ) vd->data ) );
             * break; 
             */
         case vtINT:
            fprintf( fp, "Int     %ld\n", ( long )vd->data );
            break;
      }
      fprintf( fp, "%s", "End\n\n" );
   }
}

void fread_variable( char_data * ch, FILE * fp )
{
   variable_data *pvd = new variable_data;

   do
   {
      const char *word = ( feof( fp ) ? "End" : fread_word( fp ) );

      if( word[0] == '\0' )
      {
         log_printf( "%s: EOF encountered reading file!", __FUNCTION__ );
         word = "End";
      }

      switch ( UPPER( word[0] ) )
      {
         default:
            log_printf( "%s: no match: %s", __FUNCTION__, word );
            fread_to_eol( fp );
            break;

         case '*':
            fread_to_eol( fp );
            break;

         case 'C':
            KEY( "Ctime", pvd->c_time, fread_number( fp ) );
            break;

         case 'E':
            if( !str_cmp( word, "End" ) )
            {
               switch ( pvd->type )
               {
                  default:
                  {
                     bug( "%s: invalid/incomplete variable: %s", __FUNCTION__, pvd->tag.c_str() );
                     deleteptr( pvd );
                     break;
                  }
                  case vtSTR:
                     // case vtXBIT:
                     if( !pvd->data )
                     {
                        bug( "%s: invalid/incomplete variable: %s", __FUNCTION__, pvd->tag.c_str() );
                        deleteptr( pvd );
                        break;
                     }
                  case vtINT:
                     tag_char( ch, pvd, 1 );
                     break;
               }
               return;
            }
            break;

            /*
             * case 'F': <--- FIXME: Convert to std::bitset
             * KEY( "Flags", pvd->flags, fread_number( fp ) );
             * break; 
             */

         case 'I':
            if( !str_cmp( word, "Int" ) )
            {
               if( pvd->type != vtINT )
                  bug( "%s: Type mismatch -- type(%d) != vtInt", __FUNCTION__, pvd->type );
               else
               {
                  pvd->data = ( void * )( ( long )fread_number( fp ) );
               }
               break;
            }
            break;

         case 'M':
            KEY( "Mtime", pvd->m_time, fread_number( fp ) );
            break;

         case 'R':
            KEY( "Rtime", pvd->r_time, fread_number( fp ) );
            break;

         case 'S':
            if( !str_cmp( word, "Str" ) )
            {
               if( pvd->type != vtSTR )
                  bug( "%s: Type mismatch -- type(%d) != vtSTR", __FUNCTION__, pvd->type );
               else
               {
                  pvd->data = fread_string_nohash( fp );
               }
               break;
            }
            break;

         case 'T':
            KEY( "Tag", pvd->tag, fread_string_nohash( fp ) );
            KEY( "Timer", pvd->timer, fread_number( fp ) );
            KEY( "Type", pvd->type, fread_number( fp ) );
            break;

         case 'V':
            KEY( "Vnum", pvd->vnum, fread_number( fp ) );
            break;

            /*
             * case 'X': <--- FIXME: Convert to std::bitset
             * if( !str_cmp( word, "Xbit" ) )
             * {
             * if( pvd->type != vtXBIT )
             * bug( "%s: Type mismatch -- type(%d) != vtXBIT", __FUNCTION__, pvd->type );
             * else
             * {
             * CREATE( pvd->data, EXT_BV, 1 );
             * *( EXT_BV * ) pvd->data = fread_bitvector( fp );
             * }
             * break;
             * }
             * break; 
             */
      }
   }
   while( !feof( fp ) );

   // If you make it here, something got borked.
   bug( "%s: Fell through to bottom!", __FUNCTION__ );
   deleteptr( pvd );
}
