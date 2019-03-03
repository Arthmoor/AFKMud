/****************************************************************************
 *                   ^     +----- |  / ^     ^ |     | +-\                  *
 *                  / \    |      | /  |\   /| |     | |  \                 *
 *                 /   \   +---   |<   | \ / | |     | |  |                 *
 *                /-----\  |      | \  |  v  | |     | |  /                 *
 *               /       \ |      |  \ |     | +-----+ +-/                  *
 ****************************************************************************
 * AFKMud Copyright 1997-2019 by Roger Libiez (Samson),                     *
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
 *                         MUDprog Quest Variables                          *
 ****************************************************************************/

#include "mud.h"
#include "mobindex.h"
#include "variables.h"

variable_data::variable_data()
{
   init_memory( &this->varflags, &this->timer, sizeof( this->timer ) );
}

variable_data::variable_data( int vtype, int vvnum, const string& vtag )
{
   this->type = vtype;
   this->vnum = vvnum;
   this->tag = vtag;
   this->c_time = current_time;
   this->m_time = current_time;
   this->r_time = 0;
   this->timer = 0;
   this->vardata = 0;
}

variable_data::~variable_data(  )
{
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
   return nullptr;
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
   variable_data *vd = nullptr;
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
   const char *p;
   char *tmp = nullptr;
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

   if( ( p = strchr( arg2.c_str(  ), ':' ) ) != nullptr ) 
   {
      mudstrlcpy( tmp, p, MSL );
      vnum = atoi( tmp );
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
      vd->varstring = argument;
   }
   else
   {
      vd = new variable_data( vtINT, vnum, arg2 );
      vd->vardata = atol( argument.c_str() );
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
   const char *p;
   char *tmp = nullptr;
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

   if( ( p = strchr( arg2.c_str(  ), ':' ) ) != nullptr ) 
   {
      mudstrlcpy( tmp, p, MSL );
      vnum = atoi( tmp );
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
 */
CMDF( do_mpflag )
{
   string::const_iterator ptr;
   string::size_type x;
   char_data *victim;
   variable_data *vd;
   string arg1, arg2, arg3, p;
   int vnum = 0, exp = 0, def = 0, flag = 0;
   bool error = false;

   if( ( !ch->isnpc() && ch->get_trust() < LEVEL_GREATER ) || ch->has_aflag( AFF_CHARM ) )
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
   {
      exp = ch->level * ch->get_curr_int();
      def = 1;
   }

   argument = one_argument( argument, arg2 );
   argument = one_argument( argument, arg3 );

   if( arg1.empty() || arg2.empty() || arg3.empty() )
   {
      progbug( "MPflag: No arguments specified.\r\n", ch );
      return;
   }

   if( ( victim = ch->get_char_room( arg1 ) ) == nullptr )
   {
      progbug( "MPflag: Victim is not present.\r\n", ch );
      return;
   }

   if( ( x = arg2.find_first_of( '!' ) ) == string::npos )
      vnum = ch->pIndexData ? ch->pIndexData->vnum : 0;
   else
   {
      p = arg2.substr( x + 1, arg2.length(  ) );
      vnum = atoi( p.c_str() );
   }

   if( !is_valid_tag( arg2 ) )
   {
      progbug( "Mpflag: invalid characters in tag", ch );
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
   if( error || flag < 0 || flag >= MAX_VAR_BITS )
   {
      progbug( "Mpflag: invalid flag value", ch );
      return;
   }

   if( ( vd = get_tag( victim, arg2, vnum ) ) != nullptr )
   {
      if( vd->type != vtXBIT )
      {
         progbug( "Mpflag: type mismatch", ch );
         return;
      }
      if( !def )
         vd->timer = exp;
   }
   else
   {
      vd = new variable_data( vtXBIT, vnum, arg2 );
      vd->timer = exp;
   }
   vd->varflags.set( flag );
   tag_char( victim, vd, 1 );
}

/*
 * mprmflag <victim> <tag> <flag>
 * <-- FIXME: Convert to std::bitset
 */
CMDF( do_mprmflag )
{
   string::const_iterator ptr;
   string::size_type x;
   char_data *victim;
   variable_data *vd;
   string arg1, arg2, arg3, p;
   int vnum = 0;
   bool error = false;

   if( ( !ch->isnpc() && ch->get_trust() < LEVEL_GREATER ) || ch->has_aflag( AFF_CHARM ) )
   {
      ch->print( "Huh?\r\n" );
      return;
   }

   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );
   argument = one_argument( argument, arg3 );

   if( arg1.empty() || arg2.empty() || arg3.empty() )
   {
      progbug( "MPflag: No arguments specified.\r\n", ch );
      return;
   }

   if( ( victim = ch->get_char_room( arg1 ) ) == nullptr )
   {
      progbug( "MPflag: Victim is not present.\r\n", ch );
      return;
   }

   if( ( x = arg2.find_first_of( '!' ) ) == string::npos )
      vnum = ch->pIndexData ? ch->pIndexData->vnum : 0;
   else
   {
      p = arg2.substr( x + 1, arg2.length(  ) );
      vnum = atoi( p.c_str() );
   }

   if( !is_valid_tag( arg2 ) )
   {
      progbug( "Mprmflag: invalid characters in tag", ch );
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
      progbug( "Mprmflag: invalid flag value", ch );
      return;
   }

   /*
    * Only bother doing anything if the tag exists
    */
   if( ( vd = get_tag( victim, arg2, vnum ) ) != nullptr )
   {
      if( vd->type != vtXBIT )
      {
         progbug( "Mprmflag: type mismatch", ch );
         return;
      }
      vd->varflags.reset( atoi( arg3.c_str() ) );
      tag_char( victim, vd, 1 );
   }
}

void fwrite_variables( char_data * ch, FILE * fp )
{
   list < variable_data * >::iterator ivd;

   for( ivd = ch->variables.begin(  ); ivd != ch->variables.end(  ); ++ivd )
   {
      variable_data *vd = *ivd;

      fprintf( fp, "#VARIABLE\n" );
      fprintf( fp, "Type      %d\n", vd->type );
      fprintf( fp, "Tag       %s~\n", vd->tag.c_str() );
      fprintf( fp, "Varstring %s~\n", vd->varstring.c_str() );      
      fprintf( fp, "Flags     %ld\n", vd->varflags.to_ulong() );
      fprintf( fp, "Vardata   %ld\n", vd->vardata );
      fprintf( fp, "Ctime     %ld\n", vd->c_time );
      fprintf( fp, "Mtime     %ld\n", vd->m_time );
      fprintf( fp, "Rtime     %ld\n", vd->r_time );
      fprintf( fp, "Expires   %ld\n", vd->expires );
      fprintf( fp, "Vnum      %d\n", vd->vnum );
      fprintf( fp, "Timer     %d\n", vd->timer );
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
         log_printf( "%s: EOF encountered reading file!", __func__ );
         word = "End";
      }

      switch ( UPPER( word[0] ) )
      {
         default:
            log_printf( "%s: no match: %s", __func__, word );
            fread_to_eol( fp );
            break;

         case '*':
            fread_to_eol( fp );
            break;

         case 'C':
            KEY( "Ctime", pvd->c_time, fread_long( fp ) );
            break;

         case 'E':
            KEY( "Expires", pvd->expires, fread_long( fp ) );
            if( !str_cmp( word, "End" ) )
            {
               switch( pvd->type )
               {
                  default:
                     bug( "%s: invalid variable type. Discarding.", __func__ );
                     deleteptr( pvd );
                     break;

                  case vtSTR:
                     if( pvd->varstring.empty() )
                     {
                        bug( "%s: vtSTR: Incomplete data. Discarding.", __func__ );
                        deleteptr( pvd );
                     }
                     break;

                  case vtXBIT:
                     if( pvd->varflags.none() )
                     {
                        bug( "%s: vtXBIT: Incomplete data. Discarding.", __func__ );
                        deleteptr( pvd );
                     }
                     break;

                  case vtINT:
                     tag_char( ch, pvd, 1 );
                     break;
               }
               return;
            }
            break;

         case 'F':
            if( !str_cmp( word, "Flags" ) )
            {
               string varbits;

               fread_string( varbits, fp );
               pvd->varflags = bitset<MAX_VAR_BITS>( varbits );

               break;
            }
            break;

         case 'I':
            // Legacy data
            if( !str_cmp( word, "Int" ) )
            {
               pvd->vardata = fread_long( fp );
               break;
            }
            break;

         case 'M':
            KEY( "Mtime", pvd->m_time, fread_long( fp ) );
            break;

         case 'R':
            KEY( "Rtime", pvd->r_time, fread_long( fp ) );
            break;

         case 'S':
            // Legacy data
            if( !str_cmp( word, "Str" ) )
            {
               pvd->varstring = fread_string( fp );
               break;
            }
            break;

         case 'T':
            STDSKEY( "Tag", pvd->tag );
            KEY( "Timer", pvd->timer, fread_long( fp ) );
            KEY( "Type", pvd->type, fread_number( fp ) );
            break;

         case 'V':
            STDSKEY( "Varstring", pvd->varstring );
            KEY( "Vardata", pvd->vardata, fread_long( fp ) );
            KEY( "Vnum", pvd->vnum, fread_number( fp ) );
            break;
      }
   }
   while( !feof( fp ) );

   // If you make it here, something got borked.
   bug( "%s: Fell through to bottom!", __func__ );
   deleteptr( pvd );
}
