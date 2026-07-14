/****************************************************************************
 *                   ^     +----- |  / ^     ^ |     | +-\                  *
 *                  / \    |      | /  |\   /| |     | |  \                 *
 *                 /   \   +---   |<   | \ / | |     | |  |                 *
 *                /-----\  |      | \  |  v  | |     | |  /                 *
 *               /       \ |      |  \ |     | +-----+ +-/                  *
 ****************************************************************************
 * AFKMud Copyright 1997-2026 by Roger Libiez (Samson),                     *
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

#include <fstream>
#include "mud.h"
#include "mobindex.h"
#include "variables.h"

variable_data::variable_data()
{
}

variable_data::variable_data( int vtype, int vvnum, std::string_view vtag )
{
   this->type = vtype;
   this->vnum = vvnum;
   this->tag = vtag;
   this->c_time = current_time;
   this->m_time = current_time;
   this->r_time = std::chrono::system_clock::time_point{};
}

variable_data::~variable_data(  )
{
}

/*
 * Return the specified tag from a character
 */
variable_data *get_tag( const char_data * ch, std::string_view tag, int vnum )
{
   for( auto* vd : ch->variables )
   {
      if( ( !vnum || vnum == vd->vnum ) && !str_cmp( tag, vd->tag ) )
         return vd;
   }
   return nullptr;
}

/*
 * Remove the specified tag from a character
 */
bool remove_tag( char_data * ch, std::string_view tag, int vnum )
{
   bool deleted = false;

   if( ch->variables.empty(  ) )
      return false;

   for( auto it = ch->variables.begin(  ); it != ch->variables.end(  ); )
   {
      variable_data *vd = *it;
      ++it;

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
   std::list<variable_data *>::iterator ivd;
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

bool is_valid_tag( std::string_view tagname )
{
   if( tagname.empty() || !std::isalpha( static_cast<unsigned char>( tagname[0] ) ) )
      return false;

   for( char c : tagname )
   {
      if ( !std::isalnum( static_cast<unsigned char>( c ) ) && c != '_' )
         return false;
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
   std::string::const_iterator ptr;
   char_data *victim;
   variable_data *vd;
   std::string arg1, arg2;
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
      exp = std::stoi( arg1 );
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

   std::string_view arg_view = arg2;
   auto pos = arg_view.find( ':' );

   if( pos != std::string_view::npos )
   {
      std::string_view remainder = arg_view.substr( pos + 1 );
      vnum = std::stoi( remainder.data() );
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
      vd->vardata = std::stoi( argument );
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
   std::string arg1, arg2;
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

   std::string_view arg_view = arg2;
   auto pos = arg_view.find( ':' );

   if( pos != std::string_view::npos )
   {
      std::string_view remainder = arg_view.substr( pos + 1 );
      vnum = std::stoi( remainder.data() );
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
   std::string::const_iterator ptr;
   std::string::size_type x;
   char_data *victim;
   variable_data *vd;
   std::string arg1, arg2, arg3, p;
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
      exp = std::stoi( arg1 );
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

   if( ( x = arg2.find_first_of( '!' ) ) == std::string::npos )
      vnum = ch->pIndexData ? ch->pIndexData->vnum : 0;
   else
   {
      p = arg2.substr( x + 1, arg2.length(  ) );
      vnum = std::stoi( p );
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

   flag = std::stoi( arg3 );
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
   std::string::const_iterator ptr;
   std::string::size_type x;
   char_data *victim;
   variable_data *vd;
   std::string arg1, arg2, arg3, p;
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

   if( ( x = arg2.find_first_of( '!' ) ) == std::string::npos )
      vnum = ch->pIndexData ? ch->pIndexData->vnum : 0;
   else
   {
      p = arg2.substr( x + 1, arg2.length(  ) );
      vnum = std::stoi( p );
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
      vd->varflags.reset( std::stoi( arg3 ) );
      tag_char( victim, vd, 1 );
   }
}

void fwrite_variables( const char_data * ch, std::ofstream & stream )
{
   for( auto* vd : ch->variables )
   {
      auto vc_time = std::chrono::system_clock::to_time_t( vd->c_time );
      auto vm_time = std::chrono::system_clock::to_time_t( vd->m_time );
      auto vr_time = std::chrono::system_clock::to_time_t( vd->r_time );
      auto expires = std::chrono::system_clock::to_time_t( vd->expires );

      stream << "#VARIABLE\n";
      stream << std::format( "Type      {}\n", vd->type );
      stream << std::format( "Tag       {}~\n", vd->tag );
      stream << std::format( "Varstring {}~\n", vd->varstring );
      stream << std::format( "Flags     {}\n", vd->varflags.to_ulong() );
      stream << std::format( "Vardata   {}\n", vd->vardata );
      stream << std::format( "Ctime     {}\n", vc_time );
      stream << std::format( "Mtime     {}\n", vm_time );
      stream << std::format( "Rtime     {}\n", vr_time );
      stream << std::format( "Expires   {}\n", expires );
      stream << std::format( "Vnum      {}\n", vd->vnum );
      stream << std::format( "Timer     {}\n", vd->timer );
      stream << "End\n\n";
   }
}

void fread_variable( char_data * ch, std::ifstream & stream )
{
   variable_data *vd = new variable_data;

   std::string key;
   while( stream >> key )
   {
      if( key == "Type" )
         stream >> vd->type;
      else if( key == "Tag" )
         vd->tag = fread_line( stream );
      else if( key == "Varstring" )
         vd->varstring = fread_line( stream );
      else if( key == "Flags" )
      {
         long varbits;
         stream >> varbits;

         vd->varflags = std::bitset<MAX_VAR_BITS>( varbits );
      }
      else if( key == "Vardata" )
         stream >> vd->vardata;
      else if( key == "Ctime")
      {
         time_t loaded_time;
         stream >> loaded_time;

         vd->c_time = std::chrono::system_clock::from_time_t( loaded_time );
      }
      else if( key == "Mtime" )
      {
         time_t loaded_time;
         stream >> loaded_time;

         vd->m_time = std::chrono::system_clock::from_time_t( loaded_time );
      }
      else if( key == "Rtime" )
      {
         time_t loaded_time;
         stream >> loaded_time;

         vd->r_time = std::chrono::system_clock::from_time_t( loaded_time );
      }
      else if( key == "Expires" )
      {
         time_t loaded_time;
         stream >> loaded_time;

         vd->expires = std::chrono::system_clock::from_time_t( loaded_time );
      }
      else if( key == "Vnum" )
         stream >> vd->vnum;
      else if( key == "Timer" )
         stream >> vd->timer;
      else if( key == "End" )
      {
         switch( vd->type )
         {
            default:
               bug( "{}: invalid variable type. Discarding.", __func__ );
               deleteptr( vd );
               break;

            case vtSTR:
               if( vd->varstring.empty() )
               {
                  bug( "{}: vtSTR: Incomplete data. Discarding.", __func__ );
                  deleteptr( vd );
               }
               break;

            case vtXBIT:
               if( vd->varflags.none() )
               {
                  bug( "{}: vtXBIT: Incomplete data. Discarding.", __func__ );
                  deleteptr( vd );
               }
               break;

            case vtINT:
               tag_char( ch, vd, 1 );
               break;
         }
         return;
      }
   }
   // If you make it here, something got borked.
   bug( "{}: Fell through to bottom!", __func__ );
   deleteptr( vd );
}
