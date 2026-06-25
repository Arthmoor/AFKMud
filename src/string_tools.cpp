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
 *                          String Function Tools                           *
 ****************************************************************************/

#include <algorithm>
#include <ranges>
#include "mud.h"

// Pick off one argument from a string and return the rest.
std::string one_argument( std::string_view argument, std::string & first )
{
   std::string::size_type start, stop, stop2;
   char find;

   // Init
   start = 0;

   // Make sure first is clean
   first.clear(  );

   // Empty?
   if( argument.empty(  ) )
      return "";

   // Strip leading spaces
   if( argument.front() == ' ' )
   {
      start = argument.find_first_not_of( ' ' );

      // Empty?
      if( start == argument.npos )
         return "";
   }

   // Quotes or space?
   switch ( argument[start] )
   {
      case '\'':
         find = '\'';
         ++start;
         break;

      case '\"':
         find = '\"';
         ++start;
         break;

      default:
         find = ' ';
   }

   // Find end of argument.
   stop = argument.find_first_of( find, start );

   // Empty leftovers?
   if( stop == argument.npos )
   {
      first = argument.substr( start );
      return "";
   }

   // Update first
   first = argument.substr( start, ( stop - start ) );

   // Strip leading spaces from leftovers
   stop2 = argument.find_first_not_of( ' ', stop + 1 );

   // Empty leftovers?
   if( stop2 == argument.npos )
      return "";

   // Return leftovers.
   return std::string{argument.substr( stop2 )};
}

/*
 * Pick off one argument from a string and return the rest.
 * Understands quotes.  Delimiters = { ' ', '-' }
 * No longer mangles case either. That used to be annoying.
 *
 * Rewritten by Xorith
 */
std::string one_argument2( std::string arg, std::string & newArg )
{
   std::string retValue = "";

   // Check to see if we start with a double quote
   if( arg[0] == '"' )
   {
      std::string::size_type nextIndex = arg.find( '"', 1 );
      if( nextIndex != std::string::npos )
      {
         newArg = arg.substr( 1, nextIndex - 1 );
         // make sure we strip spaces from the return value
         int x = 1;
         while( arg[nextIndex + x] == ' ' )
            ++x;
         retValue = arg.substr( nextIndex + x );
         return retValue;
      }
   }

   // Check for single-quote
   if( arg[0] == '\'' )
   {
      std::string::size_type nextIndex = arg.find( '\'', 1 );
      if( nextIndex != std::string::npos )
      {
         newArg = arg.substr( 1, nextIndex - 1 );
         // make sure we strip spaces from the return value
         int x = 1;
         while( arg[nextIndex + x] == ' ' )
            ++x;
         retValue = arg.substr( nextIndex + x );
         return retValue;
      }
   }

   // See which is closest - the next whitespace or hyphen
   std::string::size_type nextHyphenIndex = arg.find( '-', 0 );
   std::string::size_type nextSpaceIndex = arg.find( ' ', 0 );

   if( nextHyphenIndex != std::string::npos )
   {
      if( nextSpaceIndex == std::string::npos || ( nextHyphenIndex < nextSpaceIndex && ( nextHyphenIndex + 1 != nextSpaceIndex ) ) )
      {
         newArg = arg.substr( 0, nextHyphenIndex );
         retValue = arg.substr( nextHyphenIndex + 1 );
         return retValue;
      }
   }

   if( nextSpaceIndex != std::string::npos )
   {
      newArg = arg.substr( 0, nextSpaceIndex );
      // make sure we strip spaces from the return value
      int x = 1;
      while( arg[nextSpaceIndex + x] == ' ' )
         ++x;
      retValue = arg.substr( nextSpaceIndex + 1 );
      return retValue;
   }

   // If we're here, we don't have any spaces, hyphens, quotes, ect...
   // Send what we do have back with newArg and return empty.
   newArg = arg;
   return retValue;
}

/*
 * Given a string like 14.foo, return 14 and 'foo'
 */
int number_argument( std::string_view argument, std::string & arg )
{
   int number;
   std::string pdot;
   std::string::size_type x;

   if( ( x = argument.find_first_of( "." ) ) == std::string::npos )
   {
      arg = argument;
      return 1;
   }

   pdot = argument.substr( 0, x );
   number = atoi( pdot.c_str(  ) );
   arg = argument.substr( x + 1, argument.length(  ) );
   return number;
}


/*
 * Does the list have the member in it?
 *
 * Holy Christ this was the DUMBEST IDEA EVER.
 * "not a" -> Log: Samson: not a -> You can't send tells! NOTELL applied to Samson.
 * So... yeah. U dun goof'd.
 * Fixing it by turning this into a wrapper for is_name2_prefix cause there's a mountain of code using this.
 */
bool hasname( std::string_view list, std::string_view member )
{
   if( is_name2_prefix( member, list.data() ) )
      return true;

   return false;
}

/* Add a new member to the list, provided it's not already there */
void addname( std::string & list, const std::string & member )
{
   if( hasname( list, member ) )
      return;

   if( list.empty(  ) )
      list = member;
   else
      list.append( " " + member );
   strip_lspace( list );
}

/* Remove a member from a list, provided it's there. */
void removename( std::string & list, const std::string & member )
{
   if( !hasname( list, member ) )
      return;

   // Implies the list has more than just this name.
   if( list.length(  ) > member.length(  ) )
   {
      std::string die = " " + member;
      std::string::size_type pos = list.find( die );

      if( pos != std::string::npos )
         list.erase( pos, die.length( ) );
      else
      {
         pos = list.find( member );

         if( pos != std::string::npos && pos == 0 )
            list.erase( pos, member.length( ) );
      }
   }
   else
      list.clear(  );
   strip_lspace( list );
}

// Historical compatibility: Returns FALSE when they match, TRUE when they don't. Blame the Smaug devs.
bool str_cmp( std::string_view astr, std::string_view bstr )
{
   // If neither one exists, then they're equal.
   if( astr.empty() && bstr.empty() )
      return false;

   auto case_insensitive_equals = []( std::string_view a, std::string_view b ) {
      return std::ranges::equal(a, b, [](char c1, char c2) {
         return std::tolower( static_cast<unsigned char>(c1) ) == std::tolower( static_cast<unsigned char>(c2) );
      });
   };

   if( case_insensitive_equals( astr, bstr ) )
      return false; // They match.
   return true; // They do not match.
}

// Checks to see if astr is a prefix ( beginning part of ) bstr.
// If astr is larger, obviously not. Same if bstr is empty.
// Returns true if astr isn't a prefix of bstr.
// Returns false if it is.
// If that's confusing, that's how this function's traditional usage has always been.
// Thanks to Justice for providing this.
bool str_prefix( std::string_view astr, std::string_view bstr )
{
   if( astr.size() > bstr.size() )
      return true;

   auto [it1, it2] = std::ranges::mismatch( astr, bstr, [](unsigned char a, unsigned char b ) { return std::tolower(a) == std::tolower(b); } );

   return it1 != astr.end();
}

/*
 * Compare strings, case insensitive, for match anywhere.
 * Returns true if astr not part of bstr.
 *   (compatibility with historical functions).
 */
bool str_infix( std::string_view astr, std::string_view bstr )
{
   if( bstr.find( astr ) != std::string::npos )
      return false;
   return true;
}

/*
 * Return true if astr not a suffix of bstr
 *   (compatibility with historical functions).
 */
bool str_suffix( std::string_view astr, std::string_view bstr )
{
   if( astr.length() <= bstr.length() && !str_cmp( astr, bstr.substr( bstr.length() - astr.length() ) ) )
      return false;
   else
      return true;
}

// Son, you just got promoted. You are now what hasname in editor.cpp is calling.
bool is_name2_prefix( std::string_view str, std::string namelist )
{
   std::string name;

   for( ;; )
   {
      namelist = one_argument2( namelist, name );

      if( name.empty(  ) )
         return false;

      if( !str_prefix( str, name ) )
         return true;
   }
}

/*
 * Rewrote the 'nifty' functions since they mistakenly allowed for all objects
 * to be selected by specifying an empty list l*ike -, '', "", ', " etc,
 * example: ofind -, c loc ''  - Luc 08/2000
 */
bool nifty_is_name_prefix( std::string str, std::string namelist )
{
   std::string name;
   bool valid = false;

   if( str.empty(  ) || namelist.empty(  ) )
      return false;

   for( ;; )
   {
      str = one_argument2( str, name );
      if( !name.empty(  ) )
      {
         valid = true;

         if( !is_name2_prefix( name, namelist ) )
            return false;
      }
      if( str.empty(  ) )
         return valid;
   }
}

/*
 * Strips off leading spaces and tabs in strings.
 * Useful mainly for input file streams.
 * Samson 10-16-04
 */
void strip_lspace( std::string & line )
{
   std::string::size_type space;

   space = line.find_first_not_of( " \t" );
   if( space == std::string::npos )
      space = 0;
   line = line.substr( space, line.length(  ) );
}

// Strips off trailing spaces in strings.
void strip_tspace( std::string & line )
{
   std::string::size_type space;

   space = line.find_last_not_of( " \t" );
   if( space != std::string::npos )
      line = line.substr( 0, space + 1 );
}

// Strip both leading and trailing spaces from a string.
void strip_spaces( std::string & line )
{
   strip_tspace( line );
   strip_lspace( line );
}

// Strip only the carriage returns from a string.
std::string strip_cr( std::string_view str )
{
   std::string result( str );

   std::erase( result, '\r' );

   return result;
}

// Strip off carriage returns and line feeds.
std::string strip_crlf( std::string_view str )
{
   std::string result( str );

   std::erase( result, '\r' );
   std::erase( result, '\n' );

   return result;
}

// Strips off any leading and trailing spaces, plus any stray tabs, carriage returns, or newlines.
void strip_whitespace( std::string & str )
{
   // This should be every conceivable whitespace character to run into.
   const std::string_view whitespace = " \t\r\n\v\f";

   // Find first non-whitespace character.
   const auto start = str.find_first_not_of( whitespace );
   if( start == std::string::npos )
   {
      str.clear(); // The string is entirely whitespace.
      return;
   }

   // Find last non-whitespace character.
   const auto end = str.find_last_not_of( whitespace );

   // Update the string.
   str = str.substr( start, end - start + 1 );
}

/*
 * invert_string( original, inverted );
 * Author: Xorith
 * Date: 6-18-05
 */
std::string invert_string( std::string_view orig )
{
   std::string result;
   result.reserve( orig.size() );
   size_t j = 0;

   if( orig.empty(  ) )
      return "";

   for( size_t i = orig.length(  ) - 1; j < orig.length(  ); --i, ++j )
      result += orig[i];

   return result;
}

/* Provided by Remcon to stop crashes with channel history */
// The std::format version now.
const std::string escape_formatting( std::string str )
{
   if( str.empty() )
      return "";

   string_replace( str, "{", "{{" );
   string_replace( str, "}", "}}" );

   return str;
}

/*
 * Returns an initial-capped string.
 * Rewritten by FearItself@AvP
 */
std::string capitalize( std::string_view str )
{
   std::string result;
   result.reserve( str.size() );

   enum { Normal, Color } state = Normal;
   bool first_alpha_found = true;

   for( unsigned char c : str )
   {
      if( state == Color )
      {
         state = Normal;
      }
      else if( c == '&' || c == '{' || c == '}' )
      {
         state = Color;
      }
      else if( std::isalpha(c) )
      {
         if( first_alpha_found )
         {
            c = static_cast<unsigned char>( std::toupper(c) );
            first_alpha_found = false;
         }
         else
         {
            c = static_cast<unsigned char>( std::tolower(c) );
         }
      }

      result.push_back( static_cast<char>(c) );
   }

   return result;
}

// Returns a lowercase string.
void strlower( std::string & str )
{
   std::transform( str.begin(  ), str.end(  ), str.begin(  ), ( int ( * )( int ) )std::tolower );
}

// Returns an uppercase string.
void strupper( std::string & str )
{
   std::transform( str.begin(  ), str.end(  ), str.begin(  ), ( int ( * )( int ) )std::toupper );
}

// These two replace the old LOWER and UPPER macros. These are almost always used in file reads.
char to_lower( char c )
{
   return static_cast<char>( std::tolower( static_cast<unsigned char>(c) ) );
}

char to_upper( char c )
{
   return static_cast<char>( std::toupper( static_cast<unsigned char>(c) ) );
}

/*
 * Returns true or false if a letter is a vowel - Thoric
 */
bool isavowel( char letter )
{
   char c = std::tolower( static_cast<unsigned char>( letter ) );
   return c == 'a' || c == 'e' || c == 'i' || c == 'o' || c == 'u';
}

/*
 * Shove either "a " or "an " onto the beginning of a string - Thoric
 */
const char *aoran( std::string_view str )
{
   if( str.empty() )
   {
      bug( "{}: empty str", __func__ );
      return "";
   }

   std::string temp;

   char first_lower = std::tolower( static_cast<unsigned char>( str[0] ) );
   char second_lower = ( str.length() > 1 ) ? std::tolower( static_cast<unsigned char>( str[1] ) ) : '\0';

   if( isavowel( str[0] ) || ( first_lower == 'y' && !isavowel( second_lower ) ) )
      temp = std::format( "an {}", str );
   else
      temp = std::format( "a {}", str );

   return temp.c_str();
}

/*
 * Removes the tildes from a string.
 * Used for player-entered strings that go into disk files.
 */
void smash_tilde( std::string & str )
{
   string_replace( str, "~", "-" );
}

/*
 * Encodes the tildes in a string. - Thoric
 * Used for player-entered strings that go into disk files.
 */
void hide_tilde( std::string & str )
{
   if( str.find( '~' ) == std::string::npos )
      return;

   string_replace( str, "~", ( char * )HIDDEN_TILDE );
}

const std::string show_tilde( const std::string & str )
{
   std::string newstr = str;

   if( str.find( HIDDEN_TILDE ) == std::string::npos )
      return newstr;

   string_replace( newstr, ( char * )HIDDEN_TILDE, "~" );   // <-- Stupid C++ making me use ugly casting.
   return newstr;
}

// Strips off trailing tildes on lines from files which will no longer need them - Samson 3-1-05
void strip_tilde( std::string & line )
{
   std::erase( line, '~' );
}

// The purpose of this is to emulate PHP's explode() function. Take a string, and split it up into a vector using the delimiter value as a marker.
// Thanks to David Haley and Davion@MudBytes for assisting in getting this working.
std::vector<std::string> string_explode( std::string_view src, char delimiter )
{
   std::vector<std::string> exploded;

   auto chunks = src | std::views::split( delimiter );

   for( auto&& chunk : chunks )
   {
      std::string line{ std::string_view( chunk ) };

      if( line.empty() )
         continue;

      line += delimiter;

      exploded.emplace_back( std::move( line ) );
   }

   return exploded;
}

// Since C++ wants to screw with me on this, I'll overload it instead. HA!
void string_erase( std::string & src, char find )
{
   std::erase( src, find );
}

void string_erase( std::string & src, std::string_view find )
{
   if( find.empty() )
   {
      bug( "{}: Cannot search for an empty string!", __func__ );
      return;
   }

   std::string::size_type pos = 0;
   const std::string::size_type find_len = find.size();

   while( ( pos = src.find( find, pos ) ) != std::string::npos )
   {
      src.erase( pos, find_len );
   }
}

void string_replace( std::string & src, std::string_view find, std::string_view replace )
{
   if( find.empty() )
   {
      bug( "{}: Cannot search for an empty string!", __func__ );
      return;
   }

   size_t pos = src.find( find );
   if( pos == std::string::npos )
      return;

   if( replace.empty() )
   {
      string_erase( src, find );
      return;
   }

   std::string result;
   result.reserve( src.size() + ( replace.size() > find.size() ? (src.size() / 2) : 0 ) );

   size_t last_pos = 0;
   while( ( pos = src.find( find, last_pos ) ) != std::string::npos )
   {
      result.append( src, last_pos, pos - last_pos );
      result.append( replace );
      last_pos = pos + find.size();
   }
   result.append( src, last_pos, std::string_view::npos );

   src = std::move( result );
}

/*
 * Return true if an argument is completely numeric.
 */
bool is_number( std::string_view arg )
{
   if( arg.empty() )
      return false;

   // Check for negative numbers.
   if( arg[0] == '-' )
   {
      arg.remove_prefix(1);

      // A lone '-' is not a number.
      if( arg.empty() )
         return false;
   }

   // Ensure all remaining characters are digits.
   return std::ranges::all_of( arg, [](unsigned char c ) { return std::isdigit(c); } );
}

// I r lazy and just want a good way to output the contents of the various string arrays.
const char *print_array_string( const char *flagarray[], size_t arraySize )
{
   static std::string s;
   int columns = 0;

   s.clear();

   for( size_t i = 0; i < arraySize; ++i )
   {
      s.append( flagarray[i] );

      if( !( ++columns % 6 ) )
         s.append( "\r\n" );
      else
         s.append( 1, '\t' );
   }
   strip_tspace(s); // get rid of final space

   if( !( columns % 6 ) )
      s.append( "\r\n" );

   return s.c_str();
}
