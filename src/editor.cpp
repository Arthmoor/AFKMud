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
 *                     Line Editor and String Functions                     *
 ****************************************************************************/

#include <algorithm>
#include <ranges>
#include "mud.h"
#include "descriptor.h"

// FIXME: Tagging this for removal once all calls using it have been converted to std::string
void strdup_printf( char **pointer, const char *fmt, ... )
{
   char buf[MSL * 4];
   va_list args;

   va_start( args, fmt );
   vsnprintf( buf, MSL * 4, fmt, args );
   va_end( args );

   DISPOSE( *pointer );
   *pointer = strdup( buf );
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

char *one_argument( char *argument, char *arg_first )
{
    return (char*) one_argument( (const char*) argument, arg_first );
}

/*
 * Pick off one argument from a string and return the rest.
 * Understands quotes. No longer mangles case either. That used to be annoying.
 */
const char *one_argument( const char *argument, char *arg_first )
{
   char cEnd;
   int count;

   count = 0;

   while( isspace( *argument ) )
      ++argument;

   cEnd = ' ';
   if( *argument == '\'' || *argument == '"' )
      cEnd = *argument++;

   while( *argument != '\0' && count < 255 )
   {
      if( *argument == cEnd )
      {
         ++argument;
         break;
      }
      *arg_first++ = *argument++;
      count++;
   }
   *arg_first = '\0';

   while( isspace( *argument ) )
      ++argument;

   return argument;
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
 * Given a string like 14.foo, return 14 and 'foo'
 */
int number_argument( char *argument, char *arg )
{
   char *pdot;
   int number;

   for( pdot = argument; *pdot != '\0'; ++pdot )
   {
      if( *pdot == '.' )
      {
         *pdot = '\0';
         number = atoi( argument );
         *pdot = '.';
         strcpy( arg, pdot + 1 );   /* Leave this one alone! BAD THINGS(TM) will happen if you don't! */
         return number;
      }
   }
   strcpy( arg, argument );   /* Leave this one alone! BAD THINGS(TM) will happen if you don't! */
   return 1;
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

// Strips off trailing tildes on lines from files which will no longer need them - Samson 3-1-05
void strip_tilde( std::string & line )
{
   std::erase( line, '~' );
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

// Strip both leading and trailing spaces from a string
void strip_spaces( std::string & line )
{
   strip_tspace( line );
   strip_lspace( line );
}

std::string strip_cr( const std::string & str )
{
   std::string newstr = str;
   std::string::size_type x;

   while( ( x = newstr.find( '\r' ) ) != std::string::npos )
      newstr = newstr.erase( x, 1 );
   return newstr;
}

/*
 * Remove carriage returns from a line
 */
const char *strip_cr( const char *str )
{
   static char newstr[MSL];
   int i, j = 0;

   if( !str || str[0] == '\0' )
      return "";

   for( i = 0; str[i] != '\0'; ++i )
      if( str[i] != '\r' )
         newstr[j++] = str[i];
   newstr[j] = '\0';
   return newstr;
}

// Strip off carriage returns and line feeds.
std::string strip_crlf( std::string_view str )
{
   std::string newstr = str.data();

   std::erase( newstr, '\r' );
   std::erase( newstr, '\n' );

   return newstr;
}

// Strips off any leading and trailing spaces, plus any stray tabs, carriage returns, or newlines.
void strip_whitespace( std::string & str )
{
   str = strip_crlf( str );
   strip_spaces( str );
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
const std::string add_percent( std::string str )
{
   if( str.empty() )
      return "";

   string_replace( str, "%", "%%" );

   return str;
}

// The std::format version of the above.
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

/*
 * Returns a lowercase string.
 */
char *strlower( const char *str )
{
   static char strlow[MSL];
   int i;

   for( i = 0; str[i] != '\0'; ++i )
      strlow[i] = tolower( str[i] );
   strlow[i] = '\0';
   return strlow;
}

void strlower( std::string & str )
{
   std::transform( str.begin(  ), str.end(  ), str.begin(  ), ( int ( * )( int ) )std::tolower );
}

// These two replace the old LOWER and UPPER macros.
char to_lower( char c )
{
   return static_cast<char>( std::tolower( static_cast<unsigned char>(c) ) );
}

char to_upper( char c )
{
   return static_cast<char>( std::toupper( static_cast<unsigned char>(c) ) );
}

/*
 * Returns an uppercase string.
 */
char *strupper( const char *str )
{
   static char strup[MSL];
   int i;

   for( i = 0; str[i] != '\0'; ++i )
      strup[i] = toupper( str[i] );
   strup[i] = '\0';
   return strup;
}

void strupper( std::string & str )
{
   std::transform( str.begin(  ), str.end(  ), str.begin(  ), ( int ( * )( int ) )std::toupper );
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
void smash_tilde( char *str )
{
   for( ; *str != '\0'; ++str )
      if( *str == '~' )
         *str = '-';
}

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

const char *show_tilde( const char *str )
{
   static char buf[MSL];
   std::string src = str, newstr;

   newstr = show_tilde( src );
   strlcpy( buf, newstr.c_str(  ), MSL );

   return buf;
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

constexpr int max_buf_lines = 60;

struct editor_data
{
   editor_data(  );
   ~editor_data(  );

   std::string desc;
   char line[max_buf_lines][81]{0};
   short numlines = 0;
   short on_line = 0;
   short size = 0;
};

editor_data::editor_data(  )
{
}

editor_data::~editor_data(  )
{
}

void editor_print_info( char_data * ch, editor_data * edd, short max_size )
{
   ch->printf( "Currently editing: %s\r\n"
               "Total lines: %4d   On line:  %4d\r\n"
               "Buffer size: %4d   Max size: %4d\r\n", !edd->desc.empty(  )? edd->desc.c_str(  ) : "(Null description)", edd->numlines, edd->on_line, edd->size, max_size );
}

void char_data::set_editor_desc( std::string_view new_desc )
{
   if( !pcdata->editor )
      return;

   pcdata->editor->desc = new_desc;
}

// FIXME: Switch to std::format logic. Use character.cpp as an example.
void char_data::editor_desc_printf( const char *desc_fmt, ... )
{
   char buf[MSL * 2];   /* umpf.. */
   va_list args;

   va_start( args, desc_fmt );
   vsnprintf( buf, MSL * 2, desc_fmt, args );
   va_end( args );

   set_editor_desc( buf );
}

void char_data::stop_editing(  )
{
   deleteptr( pcdata->editor );
   pcdata->dest_buf = nullptr;
   pcdata->spare_ptr = nullptr;
   substate = SUB_NONE;

   if( !desc )
   {
      bug( "Fatal: {}: no desc", __func__ );
      return;
   }
   desc->connected = CON_PLAYING;
}

void char_data::start_editing( std::string data )
{
   editor_data *edit;
   short lines, size, lpos;
   char c;

   if( !desc )
   {
      bug( "Fatal: {}: no desc", __func__ );
      return;
   }
   if( substate == SUB_RESTRICTED )
      bug( "NOT GOOD: {}: ch->substate == SUB_RESTRICTED", __func__ );

   set_color( AT_GREEN );
   print( "Begin entering your text now (/? = help /s = save /c = clear /l = list)\r\n" );
   print( "-----------------------------------------------------------------------\r\n> " );
   if( pcdata->editor )
      stop_editing(  );

   edit = new editor_data;
   edit->numlines = 0;
   edit->on_line = 0;
   edit->size = 0;
   size = 0;
   lpos = 0;
   lines = 0;
   if( !data.empty(  ) )
   {
      for( ;; )
      {
         c = data[size++];
         if( c == '\0' )
         {
            edit->line[lines][lpos] = '\0';
            break;
         }
         else if( c == '\r' );
         else if( c == '\n' || lpos > 79 )
         {
            edit->line[lines][lpos] = '\0';
            ++lines;
            lpos = 0;
         }
         else
            edit->line[lines][lpos++] = c;
         if( lines >= max_buf_lines || size > MSL )
         {
            edit->line[lines][lpos] = '\0';
            break;
         }
      }
   }

   if( lpos > 0 && lpos < 78 && lines < max_buf_lines )
   {
      edit->line[lines][lpos] = '~';
      edit->line[lines][lpos + 1] = '\0';
      ++lines;
      lpos = 0;
   }
   edit->numlines = lines;
   edit->size = size;
   edit->on_line = lines;
   pcdata->editor = edit;
   desc->connected = CON_EDITING;
}

void char_data::start_editing( char *data )
{
   editor_data *edit;
   short lines, size, lpos;
   char c;

   if( !desc )
   {
      bug( "Fatal: {}: no desc", __func__ );
      return;
   }
   if( substate == SUB_RESTRICTED )
      bug( "NOT GOOD: {}: ch->substate == SUB_RESTRICTED", __func__ );

   set_color( AT_GREEN );
   print( "Begin entering your text now (/? = help /s = save /c = clear /l = list)\r\n" );
   print( "-----------------------------------------------------------------------\r\n> " );
   if( pcdata->editor )
      stop_editing(  );

   edit = new editor_data;
   edit->numlines = 0;
   edit->on_line = 0;
   edit->size = 0;
   size = 0;
   lpos = 0;
   lines = 0;
   if( data )
   {
      for( ;; )
      {
         c = data[size++];
         if( c == '\0' )
         {
            edit->line[lines][lpos] = '\0';
            break;
         }
         else if( c == '\r' );
         else if( c == '\n' || lpos > 79 )
         {
            edit->line[lines][lpos] = '\0';
            ++lines;
            lpos = 0;
         }
         else
            edit->line[lines][lpos++] = c;
         if( lines >= max_buf_lines || size > MSL )
         {
            edit->line[lines][lpos] = '\0';
            break;
         }
      }
   }

   if( lpos > 0 && lpos < 78 && lines < max_buf_lines )
   {
      edit->line[lines][lpos] = '~';
      edit->line[lines][lpos + 1] = '\0';
      ++lines;
      lpos = 0;
   }
   edit->numlines = lines;
   edit->size = size;
   edit->on_line = lines;
   pcdata->editor = edit;
   desc->connected = CON_EDITING;
}

std::string char_data::copy_buffer(  )
{
   if( !pcdata->editor )
   {
      bug( "{}: null editor", __func__ );
      return "";
   }

   std::ostringstream oss;

   for( int i = 0; i < pcdata->editor->numlines; ++i )
   {
      std::string_view line{pcdata->editor->line[i]};

      if( !line.empty() && line.back() == '~' )
         oss << line.substr( 0, line.size() - 1 );
      else
         oss << line << '\n';
   }
   return oss.str();
}

char *char_data::copy_buffer( bool hash )
{
   char buf[MSL], tmp[100];
   short i, len;

   if( !pcdata->editor )
   {
      bug( "{}: null editor", __func__ );
      if( hash )
         return STRALLOC( "" );
      return strdup( "" );
   }

   buf[0] = '\0';
   for( i = 0; i < pcdata->editor->numlines; ++i )
   {
      strlcpy( tmp, pcdata->editor->line[i], 100 );
      len = strlen( tmp );
      if( len > 0 && tmp[len - 1] == '~' )
         tmp[len - 1] = '\0';
      else
         strlcat( tmp, "\n", 100 );
      smash_tilde( tmp );
      strlcat( buf, tmp, MSL );
   }
   if( hash )
   {
      if( buf[0] != '\0' )
         return STRALLOC( buf );
      return nullptr;
   }
   return strdup( buf );
}

/*
 * Simple but nice and handy line editor. - Thoric
 */
void char_data::edit_buffer( std::string & argument )
{
   descriptor_data *d;
   editor_data *edit;
   std::string cmd;
   char buf[MIL];
   int x, line;
   bool esave = false;

   if( !( d = desc ) )
   {
      print( "You have no descriptor.\r\n" );
      return;
   }

   if( d->connected != CON_EDITING )
   {
      print( "You can't do that!\r\n" );
      bug( "{}: d->connected != CON_EDITING", __func__ );
      return;
   }

   if( substate <= SUB_PAUSE )
   {
      print( "You can't do that!\r\n" );
      bug( "{}: illegal ch->substate ({})", __func__, substate );
      d->connected = CON_PLAYING;
      return;
   }

   if( !pcdata->editor )
   {
      print( "You can't do that!\r\n" );
      bug( "{}: null editor", __func__ );
      d->connected = CON_PLAYING;
      return;
   }

   edit = pcdata->editor;

   if( argument[0] == '/' || argument[0] == '\\' )
   {
      one_argument( argument, cmd );
      cmd = cmd.substr( 1, cmd.length(  ) );

      if( !str_cmp( cmd, "?" ) )
      {
         print( "Editing commands\r\n---------------------------------\r\n" );
         print( "/l              list buffer\r\n" );
         print( "/c              clear buffer\r\n" );
         print( "/d [line]       delete line\r\n" );
         print( "/g <line>       goto line\r\n" );
         print( "/i <line>       insert line\r\n" );
         print( "/f <format>     format text in buffer\r\n" );
         print( "/r <old> <new>  global replace\r\n" );
         print( "/a              abort editing\r\n" );
         print( "/p              show buffer information\r\n" );

         if( get_trust(  ) > LEVEL_IMMORTAL )
            print( "/! <command>    execute command (do not use another editing command)\r\n" );
         print( "/s              save buffer\r\n\r\n> " );
         return;
      }

      if( !str_cmp( cmd, "c" ) )
      {
         delete edit;
         edit = new editor_data;

         print( "Buffer cleared.\r\n> " );
         return;
      }

      if( !str_cmp( cmd, "r" ) )
      {
         std::string word1, word2, sptr, lwptr;
         int lineln;

         sptr = one_argument( argument, word1 );
         sptr = one_argument( sptr, word1 );
         sptr = one_argument( sptr, word2 );
         if( word1.empty(  ) || word2.empty(  ) )
         {
            print( "Need word to replace, and replacement.\r\n> " );
            return;
         }

         /*
          * Changed to a case-sensitive version of string compare --Cynshard 
          */
         if( !strcmp( word1.c_str(  ), word2.c_str(  ) ) )
         {
            print( "Done.\r\n> " );
            return;
         }

         printf( "Replacing all occurrences of %s with %s...\r\n", word1.c_str(  ), word2.c_str(  ) );
         for( x = 0; x < edit->numlines; ++x )
         {
            lwptr = edit->line[x];
            string_replace( lwptr, word1, word2 );

            lineln = strlcpy( buf, lwptr.c_str(  ), MIL );
            if( lineln > 79 )
               buf[80] = '\0';

            strlcpy( edit->line[x], buf, 81 );
         }
         printf( "Found and replaced \"%s\" with \"%s\".\r\n> ", word1.c_str(  ), word2.c_str(  ) );
         return;
      }

      /*
       * added format command - shogar 
       */
      /*
       * This has been redone to be more efficient, and to make format
       * start at beginning of buffer, not whatever line you happened
       * to be on, at the time.   
       */
      if( !str_cmp( cmd, "f" ) )
      {
         char temp_buf[MSL + max_buf_lines];
         int ep, old_p, end_mark, p = 0;

         pager( "Reformating...\r\n" );

         for( x = 0; x < edit->numlines; ++x )
         {
            strlcpy( temp_buf + p, edit->line[x], MSL + max_buf_lines - p );
            p += strlen( edit->line[x] );
            temp_buf[p] = ' ';
            ++p;
         }

         temp_buf[p] = '\0';
         end_mark = p;
         p = 75;
         old_p = 0;
         edit->on_line = 0;
         edit->numlines = 0;

         while( old_p < end_mark )
         {
            while( temp_buf[p] != ' ' && p > old_p )
               --p;

            if( p == old_p )
               p += 75;

            if( p > end_mark )
               p = end_mark;

            ep = 0;
            for( x = old_p; x < p; ++x )
            {
               edit->line[edit->on_line][ep] = temp_buf[x];
               ++ep;
            }
            edit->line[edit->on_line][ep] = '\0';

            ++edit->on_line;
            ++edit->numlines;

            old_p = p + 1;
            p += 75;
         }
         pager( "Reformating done.\r\n> " );
         return;
      }

      if( !str_cmp( cmd, "p" ) )
      {
         editor_print_info( this, edit, max_buf_lines );
         return;
      }

      if( !str_cmp( cmd, "i" ) )
      {
         if( edit->numlines >= max_buf_lines )
            print( "Your buffer is full.\r\n> " );
         else
         {
            if( argument[2] == ' ' )
               line = atoi( argument.c_str(  ) + 2 ) - 1;
            else
               line = edit->on_line;
            if( line < 0 )
               line = edit->on_line;
            if( line < 0 || line > edit->numlines )
               print( "Out of range.\r\n> " );
            else
            {
               for( x = ++edit->numlines; x > line; --x )
                  strlcpy( edit->line[x], edit->line[x - 1], 81 );
               strlcpy( edit->line[line], "", 81 );
               print( "Line inserted.\r\n> " );
            }
         }
         return;
      }

      if( !str_cmp( cmd, "d" ) )
      {
         if( edit->numlines == 0 )
            print( "Buffer is empty.\r\n> " );
         else
         {
            if( argument[2] == ' ' )
               line = atoi( argument.c_str(  ) + 2 ) - 1;
            else
               line = edit->on_line;
            if( line < 0 )
               line = edit->on_line;
            if( line < 0 || line > edit->numlines )
               print( "Out of range.\r\n> " );
            else
            {
               if( line == 0 && edit->numlines == 1 )
               {
                  delete edit;
                  edit = new editor_data;

                  print( "Line deleted.\r\n> " );
                  return;
               }
               for( x = line; x < ( edit->numlines - 1 ); ++x )
                  strlcpy( edit->line[x], edit->line[x + 1], 81 );
               strlcpy( edit->line[edit->numlines--], "", 81 );
               if( edit->on_line > edit->numlines )
                  edit->on_line = edit->numlines;
               print( "Line deleted.\r\n> " );
            }
         }
         return;
      }

      if( !str_cmp( cmd, "g" ) )
      {
         if( edit->numlines == 0 )
            print( "Buffer is empty.\r\n> " );
         else
         {
            if( argument[2] == ' ' )
               line = atoi( argument.c_str(  ) + 2 ) - 1;
            else
            {
               print( "Goto what line?\r\n> " );
               return;
            }
            if( line < 0 )
               line = edit->on_line;
            if( line < 0 || line > edit->numlines )
               print( "Out of range.\r\n> " );
            else
            {
               edit->on_line = line;
               printf( "(On line %d)\r\n> ", line + 1 );
            }
         }
         return;
      }

      if( !str_cmp( cmd, "l" ) )
      {
         if( edit->numlines == 0 )
            print( "Buffer is empty.\r\n> " );
         else
         {
            print( "------------------\r\n" );
            for( x = 0; x < edit->numlines; ++x )
            {
               /*
                * Quixadhal - We cannot use ch_printf here, or we can't see
                * * what color codes exist in the strings!
                */
               std::string tmpline = std::format( "{:2}> {}\r\n", x + 1, edit->line[x] );
               desc->write_to_buffer( tmpline );
            }
            print( "------------------\r\n> " );
         }
         return;
      }

      if( !str_cmp( cmd, "a" ) )
      {
         if( !last_cmd )
         {
            stop_editing(  );
            return;
         }
         d->connected = CON_PLAYING;
         substate = SUB_EDIT_ABORT;
         deleteptr( pcdata->editor );
         print( "Done.\r\n" );
         pcdata->dest_buf = nullptr;
         pcdata->spare_ptr = nullptr;
         ( *last_cmd ) ( this, "" );
         return;
      }

      if( get_trust(  ) > LEVEL_IMMORTAL && !str_cmp( cmd, "!" ) )
      {
         DO_FUN *lastcmd;
         int Csubstate = substate;

         lastcmd = last_cmd;
         substate = SUB_RESTRICTED;
         interpret( this, argument.substr( 3, argument.length(  ) ) );
         substate = Csubstate;
         last_cmd = lastcmd;
         set_color( AT_GREEN );
         print( "\r\n> " );
         return;
      }

      if( !str_cmp( cmd, "s" ) )
      {
         d->connected = CON_PLAYING;

         if( !last_cmd )
            return;

         ( *last_cmd ) ( this, "" );
         return;
      }
   }

   if( edit->size + argument.length(  ) + 1 >= MSL - 2 )
      print( "Your buffer is full.\r\n" );
   else
   {
      if( argument.length(  ) > 80 )
      {
         strlcpy( buf, argument.c_str(  ), 80 );
         print( "(Long line trimmed)\r\n> " );
      }
      else
         strlcpy( buf, argument.c_str(  ), 80 );
      strlcpy( edit->line[edit->on_line++], buf, 81 );
      while( edit->on_line > edit->numlines )
         ++edit->numlines;
      if( edit->numlines >= max_buf_lines )
      {
         edit->numlines = max_buf_lines;
         print( "Your buffer is full.\r\n" );
         esave = true;
      }
   }

   if( esave )
   {
      d->connected = CON_PLAYING;

      if( !last_cmd )
         return;

      ( *last_cmd ) ( this, "" );
      return;
   }
   print( "> " );
}
