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
 *                     Line Editor and String Functions                     *
 ****************************************************************************/

#include <cstdarg>
#include <algorithm>
#include "mud.h"
#include "descriptor.h"

/*
 * Copy src to string dst of size siz.  At most siz-1 characters
 * will be copied.  Always NUL terminates (unless siz == 0).
 * Returns strlen(src); if retval >= siz, truncation occurred.
 *
 * Renamed so it can play itself system independent.
 * Samson 10-12-03
 */
size_t mudstrlcpy( char *dst, const char *src, size_t siz )
{
   register char *d = dst;
   register const char *s = src;
   register size_t n = siz;

   /*
    * Copy as many bytes as will fit 
    */
   if( n != 0 && --n != 0 )
   {
      do
      {
         if( ( *d++ = *s++ ) == 0 )
            break;
      }
      while( --n != 0 );
   }

   /*
    * Not enough room in dst, add NUL and traverse rest of src 
    */
   if( n == 0 )
   {
      if( siz != 0 )
         *d = '\0';  /* NUL-terminate dst */
      while( *s++ )
         ;
   }
   return ( s - src - 1 ); /* count does not include NUL */
}

/*
 * Appends src to string dst of size siz (unlike strncat, siz is the
 * full size of dst, not space left).  At most siz-1 characters
 * will be copied.  Always NUL terminates (unless siz <= strlen(dst)).
 * Returns strlen(initial dst) + strlen(src); if retval >= siz,
 * truncation occurred.
 *
 * Renamed so it can play itself system independent.
 * Samson 10-12-03
 */
size_t mudstrlcat( char *dst, const char *src, size_t siz )
{
   register char *d = dst;
   register const char *s = src;
   register size_t n = siz;
   size_t dlen;

   /*
    * Find the end of dst and adjust bytes left but don't go past end 
    */
   while( n-- != 0 && *d != '\0' )
      ++d;
   dlen = d - dst;
   n = siz - dlen;

   if( n == 0 )
      return ( dlen + strlen( s ) );
   while( *s != '\0' )
   {
      if( n != 1 )
      {
         *d++ = *s;
         --n;
      }
      ++s;
   }
   *d = '\0';
   return ( dlen + ( s - src ) );   /* count does not include NUL */
}

void stralloc_printf( char **pointer, const char *fmt, ... )
{
   char buf[MSL * 4];
   va_list args;

   va_start( args, fmt );
   vsnprintf( buf, MSL * 4, fmt, args );
   va_end( args );

   STRFREE( *pointer );
   *pointer = STRALLOC( buf );
}

void strdup_printf( char **pointer, const char *fmt, ... )
{
   char buf[MSL * 4];
   va_list args;

   va_start( args, fmt );
   vsnprintf( buf, MSL * 4, fmt, args );
   va_end( args );

   DISPOSE( *pointer );
   *pointer = str_dup( buf );
}

#if defined(__FreeBSD__)
/*
 * custom str_dup using create - Thoric
 */
char *str_dup( const char *str )
{
   static char *ret;
   int len;

   if( !str )
      return NULL;

   len = strlen( str ) + 1;

   /*
    * Is this RIGHT?!? Or am I reading this WAY wrong?
    * ret = (char *)calloc( len, sizeof(char) ); 
    */
   CREATE( ret, char, len );
   mudstrlcpy( ret, str, len );
   return ret;
}
#else
// A more streamlined C++ish approach to str_dup. FreeBSD doesn't like it's counterpart DISPOSE macro in mud.h though.
char *str_dup( const char *str )
{
   static char *ret;

   if( !str )
      return NULL;

   ret = new char[strlen( str ) + 1];
   mudstrlcpy( ret, str, strlen( str ) + 1 );
   return ret;
}
#endif

/* Does the list have the member in it? */
bool hasname( const string & list, const string & member )
{
   string::size_type x;

   if( list.empty(  ) )
      return false;

   if( ( x = list.find( member ) ) != string::npos )
      return true;

   return false;
}

/* Add a new member to the list, provided it's not already there */
void addname( string & list, const string & member )
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
void removename( string & list, const string & member )
{
   if( !hasname( list, member ) )
      return;

   // Implies the list has more than just this name.
   if( list.length(  ) > member.length(  ) )
   {
      string die = " " + member;
      string::size_type pos = list.find( die );

      list.erase( pos, die.length(  ) );
   }
   else
      list.clear(  );
   strip_lspace( list );
}

// Pick off one argument from a string and return the rest.
string one_argument( const string & argument, string & first )
{
   string::size_type start, stop, stop2;
   char find;

   // Init
   start = 0;

   // Make sure first is clean
   first.clear(  );

   // Empty?
   if( argument.empty(  ) )
      return "";

   // Strip leading spaces
   if( argument[0] == ' ' )
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
   return argument.substr( stop2 );
}

char *one_argument( char *argument, char *arg_first )
{
    return (char*) one_argument((const char*) argument, arg_first);
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

   while( *argument != '\0' || ++count >= 255 )
   {
      if( *argument == cEnd )
      {
         ++argument;
         break;
      }
      *arg_first = ( *argument );
      ++arg_first;
      ++argument;
   }
   *arg_first = '\0';

   while( isspace( *argument ) )
      ++argument;

   return argument;
}

/*
 * Given a string like 14.foo, return 14 and 'foo'
 */
int number_argument( const string & argument, string & arg )
{
   int number;
   string pdot;
   string::size_type x;

   if( ( x = argument.find_first_of( "." ) ) == string::npos )
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

// Compare: astr ><= bstr.
// <0 = astr < bstr
//  0 = astr == bstr
// >0 = astr > bstr
// Case insensitive.
// -- Justice
int str_cmp( const string & astr, const string & bstr )
{
   string::const_iterator a1, a2, b1, b2;

   a1 = astr.begin(  );
   a2 = astr.end(  );

   b1 = bstr.begin(  );
   b2 = bstr.end(  );

   while( a1 != a2 && b1 != b2 )
   {
      if( std::toupper( *a1 ) != std::toupper( *b1 ) )
         return ( std::toupper( *a1 ) < std::toupper( *b1 ) ? -1 : 1 );
      ++a1;
      ++b1;
   }
   return ( bstr.size(  ) == astr.size(  )? 0 : ( astr.size(  ) < bstr.size(  )? -1 : 1 ) );
}

/*
 * Compare strings, case insensitive.
 * Return true if different
 *   (compatibility with historical functions).
 */
bool str_cmp( const char *astr, const char *bstr )
{
   // If neither one exists, then they're equal
   if( !astr && !bstr )
      return false;

   if( !astr )
   {
      bug( "%s: null astr.", __FUNCTION__ );
      if( bstr )
         log_printf_plus( LOG_DEBUG, LEVEL_ADMIN, "astr: (null)  bstr: %s", bstr );
      return true;
   }

   if( !bstr )
   {
      bug( "%s: null bstr.", __FUNCTION__ );
      if( astr )
         log_printf_plus( LOG_DEBUG, LEVEL_ADMIN, "astr: %s  bstr: (null)", astr );
      return true;
   }

   for( ; *astr || *bstr; ++astr, ++bstr )
   {
      if( LOWER( *astr ) != LOWER( *bstr ) )
         return true;
   }
   return false;
}

// Checks to see if astr is a prefix ( beginning part of ) bstr.
// If astr is larger, obviously not. Same if bstr is empty.
// Returns true if astr isn't a prefix of bstr.
// Returns false if it is.
// If that's confusing, that's how this function's traditional usage has always been.
// Thanks to Justice for providing this. http://www.mudbytes.net/index.php?a=topic&t=480&p=5314#p5314
bool str_prefix( const string & astr, const string & bstr )
{
   string::const_iterator a1, a2, b1, b2;

   if( astr.size(  ) > bstr.size(  ) || bstr.empty(  ) )
      return true;

   a1 = astr.begin(  );
   a2 = astr.end(  );

   b1 = bstr.begin(  );
   b2 = bstr.end(  );

   while( a1 != a2 && b1 != b2 )
   {
      if( std::toupper( *a1 ) != std::toupper( *b1 ) )
         return true;
      ++a1;
      ++b1;
   }
   return false;
}

bool str_prefix( const char *needle, const char *haystack )
{
   if( !needle )
   {
      bug( "%s: null needle.", __FUNCTION__ );
      return true;
   }

   if( !haystack )
   {
      bug( "%s: null haystack.", __FUNCTION__ );
      return true;
   }

   for( ; *needle; ++needle, ++haystack )
   {
      if( LOWER( *needle ) != LOWER( *haystack ) )
         return true;
   }
   return false;
}

// Is astr a part of any portion of bstr?
// Return value compatible with historical functions.
bool str_infix( const string & astr, const string & bstr )
{
   if( bstr.find( astr ) != string::npos )
      return false;
   return true;
}

/*
 * Compare strings, case insensitive, for match anywhere.
 * Returns true if astr not part of bstr.
 *   (compatibility with historical functions).
 */
bool str_infix( const char *astr, const char *bstr )
{
   int sstr1, sstr2, ichar;
   char c0;

   if( ( c0 = LOWER( astr[0] ) ) == '\0' )
      return false;

   sstr1 = strlen( astr );
   sstr2 = strlen( bstr );

   for( ichar = 0; ichar <= sstr2 - sstr1; ++ichar )
      if( c0 == LOWER( bstr[ichar] ) && !str_prefix( astr, bstr + ichar ) )
         return false;

   return true;
}

/* FIXME: This sucks. Mind drawing a blank on how to make a std::string version.
 * Compare strings, case insensitive, for suffix matching.
 * Return true if astr not a suffix of bstr
 *   (compatibility with historical functions).
 */
bool str_suffix( const char *astr, const char *bstr )
{
   int sstr1, sstr2;

   sstr1 = strlen( astr );
   sstr2 = strlen( bstr );
   if( sstr1 <= sstr2 && !str_cmp( astr, bstr + sstr2 - sstr1 ) )
      return false;
   else
      return true;
}

// Strips off trailing tildes on lines from files which will no longer need them - Samson 3-1-05
void strip_tilde( string & line )
{
   string_erase( line, '~' );
}

/* Strips off leading spaces and tabs in strings.
 * Useful mainly for input file streams because they suck so much.
 * Samson 10-16-04
 */
void strip_lspace( string & line )
{
   string::size_type space;

   space = line.find_first_not_of( ' ' );
   if( space == string::npos )
      space = 0;
   line = line.substr( space, line.length(  ) );

   space = line.find_first_not_of( '\t' );
   if( space == string::npos )
      space = 0;
   line = line.substr( space, line.length(  ) );
}

/* Strips off trailing spaces in strings. */
void strip_tspace( string & line )
{
   string::size_type space;

   space = line.find_last_not_of( ' ' );
   if( space != string::npos )
      line = line.substr( 0, space + 1 );
}

/* Strip both leading and trailing spaces from a string */
void strip_spaces( string & line )
{
   strip_lspace( line );
   strip_tspace( line );
}

string strip_cr( const string & str )
{
   string newstr = str;
   string::size_type x;

   while( ( x = newstr.find( '\r' ) ) != string::npos )
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

string strip_crlf( string str )
{
   string newstr = str;
   string::size_type x;

   while( ( x = newstr.find( '\r' ) ) != string::npos )
      newstr = newstr.erase( x, 1 );

   while( ( x = newstr.find( '\n' ) ) != string::npos )
      newstr = newstr.erase( x, 1 );

   return newstr;
}

const char *strip_crlf( const char *str )
{
   static char newstr[MSL];
   int i, j = 0;

   if( !str || str[0] == '\0' )
      return "";

   for( i = 0; str[i] != '\0'; ++i )
      if( str[i] != '\r' && str[i] != '\n' )
         newstr[j++] = str[i];
   newstr[j] = '\0';
   return newstr;
}

/* invert_string( original, inverted );
 * Author: Xorith
 * Date: 6-18-05
 */
string invert_string( const string & orig )
{
   string result = "";
   size_t j = 0;

   if( orig.empty(  ) )
      return orig;

   for( size_t i = orig.length(  ) - 1; j < orig.length(  ); --i, ++j )
      result += orig[i];

   return result;
}

/* Provided by Remcon to stop crashes with channel history */
const string add_percent( const string & str )
{
   string newstr = str;

   if( newstr.empty(  ) )
      return "";

   string_replace( newstr, "%", "%%" );

   return newstr;
}

/*
 * Returns an initial-capped string.
 * Rewritten by FearItself@AvP
 */
char *capitalize( const char *str )
{
   static char buf[MSL];
   char *dest = buf;
   enum
   { Normal, Color } state = Normal;
   bool bFirst = true;
   char c;

   while( ( c = *str++ ) )
   {
      if( state == Normal )
      {
         if( c == '&' || c == '{' || c == '}' )
         {
            state = Color;
         }
         else if( isalpha( c ) )
         {
            c = bFirst ? toupper( c ) : tolower( c );
            bFirst = false;
         }
      }
      else
      {
         state = Normal;
      }
      *dest++ = c;
   }
   *dest = c;

   return buf;
}

/*
 * Returns an initial-capped string.
 * Rewritten by FearItself@AvP
 */
string capitalize( const string & str )
{
   string strcap;
   const char *src = str.c_str(  );
   char buf[MSL];
   char *dest = buf;
   enum
   { Normal, Color } state = Normal;
   bool bFirst = true;
   char c;

   while( ( c = *src++ ) )
   {
      if( state == Normal )
      {
         if( c == '&' || c == '{' || c == '}' )
         {
            state = Color;
         }
         else if( isalpha( c ) )
         {
            c = bFirst ? toupper( c ) : tolower( c );
            bFirst = false;
         }
      }
      else
      {
         state = Normal;
      }
      *dest++ = c;
   }
   *dest = c;

   strcap = buf;
   return strcap;
}

/*
 * Returns a lowercase string.
 */
char *strlower( const char *str )
{
   static char strlow[MSL];
   int i;

   for( i = 0; str[i] != '\0'; ++i )
      strlow[i] = LOWER( str[i] );
   strlow[i] = '\0';
   return strlow;
}

void strlower( string & str )
{
   transform( str.begin(  ), str.end(  ), str.begin(  ), ( int ( * )( int ) )std::tolower );
}

/*
 * Returns an uppercase string.
 */
char *strupper( const char *str )
{
   static char strup[MSL];
   int i;

   for( i = 0; str[i] != '\0'; ++i )
      strup[i] = UPPER( str[i] );
   strup[i] = '\0';
   return strup;
}

void strupper( string & str )
{
   transform( str.begin(  ), str.end(  ), str.begin(  ), ( int ( * )( int ) )std::toupper );
}

/*
 * Returns true or false if a letter is a vowel - Thoric
 */
bool isavowel( char letter )
{
   char c;

   c = LOWER( letter );
   if( c == 'a' || c == 'e' || c == 'i' || c == 'o' || c == 'u' )
      return true;
   else
      return false;
}

/*
 * Shove either "a " or "an " onto the beginning of a string - Thoric
 */
const char *aoran( const string & str )
{
   static char temp[MSL];

   if( str.empty(  ) )
   {
      bug( "%s: NULL str", __FUNCTION__ );
      return "";
   }

   if( isavowel( str[0] ) || ( str.length(  ) > 1 && LOWER( str[0] ) == 'y' && !isavowel( str[1] ) ) )
      mudstrlcpy( temp, "an ", MSL );
   else
      mudstrlcpy( temp, "a ", MSL );
   mudstrlcat( temp, str.c_str(  ), MSL );
   return temp;
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

void smash_tilde( string & str )
{
   string_replace( str, "~", "-" );
}

/*
 * Encodes the tildes in a string. - Thoric
 * Used for player-entered strings that go into disk files.
 */
void hide_tilde( string & str )
{
   if( str.find( '~' ) == string::npos )
      return;

   string_replace( str, "~", ( char * )HIDDEN_TILDE );
}

const string show_tilde( const string & str )
{
   string newstr = str;

   if( str.find( HIDDEN_TILDE ) == string::npos )
      return newstr;

   string_replace( newstr, ( char * )HIDDEN_TILDE, "~" );   // <-- Stupid C++ making me use ugly casting.
   return newstr;
}

const char *show_tilde( const char *str )
{
   static char buf[MSL];
   string src = str, newstr;

   newstr = show_tilde( src );
   mudstrlcpy( buf, newstr.c_str(  ), MSL );

   return buf;
}

// The purpose of this is to emulate PHP's explode() function. Take a string, and split it up into a vector using the delimiter value as a marker.
// Thanks to David Haley and Davion@MudBytes for assisting in getting this working.
vector < string > string_explode( const string & src, char delimiter )
{
   vector < string > exploded;

   string::size_type curPos = 0;
   string::size_type delimPos;

   while( ( delimPos = src.find( delimiter, curPos ) ) != string::npos )
   {
      string substr = src.substr( curPos, ( delimPos - curPos ) + 1 );

      exploded.push_back( substr );
      curPos = delimPos + 1;
   }

   // Grab remainder
   if( curPos < src.size(  ) )
      exploded.push_back( src.substr( curPos, src.size(  ) - curPos ) );
   return exploded;
}

// Since C++ wants to screw with me on this, I'll overload it instead. HA!
void string_erase( string & src, char find )
{
   string::size_type pos = 0;

   if( !find )
   {
      bug( "%s: Cannot search for an empty character!", __FUNCTION__ );
      return;
   }

   // If it's not here, bail.
   if( src.find( find ) == string::npos )
      return;

   while( ( pos = src.find( find, pos ) ) != string::npos )
      src.erase( pos, 1 );
}

void string_erase( string & src, const string & find )
{
   string::size_type pos = 0;

   if( find.empty(  ) )
   {
      bug( "%s: Cannot search for an empty string!", __FUNCTION__ );
      return;
   }

   while( ( pos = src.find( find, pos ) ) != string::npos )
      src.erase( pos, find.size(  ) );
}

void string_replace( string & src, const string & find, const string & replace )
{
   string::size_type pos = 0;

   if( find.empty(  ) )
   {
      bug( "%s: Cannot search for an empty string!", __FUNCTION__ );
      return;
   }

   // Bail out if the search string isn't present.
   if( src.find( find ) == string::npos )
      return;

   // If the replacement string is emtpy, they really wanted an erase. Call string_erase.
   if( replace.empty(  ) )
      string_erase( src, find );
   else
   {
      while( ( pos = src.find( find, pos ) ) != string::npos )
      {
         src.replace( pos, find.size(  ), replace );
         pos += replace.size(  );
      }
   }
}

/*
 * Return true if an argument is completely numeric.
 */
bool is_number( const string & arg )
{
   size_t x;
   bool first = true;

   if( arg.empty(  ) )
      return false;

   for( x = 0; x < arg.length(  ); ++x )
   {
      if( first && arg[x] == '-' )
      {
         first = false;
         continue;
      }

      if( !isdigit( arg[x] ) )
         return false;
      first = false;
   }
   return true;
}

/*
 * Return TRUE if an argument is completely numeric.
 */
bool is_number( const char *arg )
{
   bool first = true;

   if( *arg == '\0' )
      return false;

   for( ; *arg != '\0'; ++arg )
   {
      if( first && *arg == '-' )
      {
         first = false;
         continue;
      }

      if( !isdigit( *arg ) )
         return false;
      first = false;
   }
   return true;
}

const int max_buf_lines = 60;

struct editor_data
{
   editor_data(  );
   ~editor_data(  );

   string desc;
   char line[max_buf_lines][81];
   short numlines;
   short on_line;
   short size;
};

editor_data::editor_data(  )
{
   init_memory( &line, &size, sizeof( size ) );
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

void char_data::set_editor_desc( const string & new_desc )
{
   if( !pcdata->editor )
      return;

   pcdata->editor->desc = new_desc;
}

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
   pcdata->dest_buf = NULL;
   pcdata->spare_ptr = NULL;
   substate = SUB_NONE;

   if( !desc )
   {
      bug( "Fatal: %s: no desc", __FUNCTION__ );
      return;
   }
   desc->connected = CON_PLAYING;
}

void char_data::start_editing( string data )
{
   editor_data *edit;
   short lines, size, lpos;
   char c;

   if( !desc )
   {
      bug( "Fatal: %s: no desc", __FUNCTION__ );
      return;
   }
   if( substate == SUB_RESTRICTED )
      bug( "NOT GOOD: %s: ch->substate == SUB_RESTRICTED", __FUNCTION__ );

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
      bug( "Fatal: %s: no desc", __FUNCTION__ );
      return;
   }
   if( substate == SUB_RESTRICTED )
      bug( "NOT GOOD: %s: ch->substate == SUB_RESTRICTED", __FUNCTION__ );

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

string char_data::copy_buffer(  )
{
   char buf[MSL], tmp[100];
   short i, len;

   if( !pcdata->editor )
   {
      bug( "%s: null editor", __FUNCTION__ );
      return "";
   }

   buf[0] = '\0';
   for( i = 0; i < pcdata->editor->numlines; ++i )
   {
      mudstrlcpy( tmp, pcdata->editor->line[i], 100 );
      len = strlen( tmp );
      if( len > 0 && tmp[len - 1] == '~' )
         tmp[len - 1] = '\0';
      else
         mudstrlcat( tmp, "\n", 100 );
      smash_tilde( tmp );
      mudstrlcat( buf, tmp, MSL );
   }
   string newbuf = buf;
   return newbuf;
}

char *char_data::copy_buffer( bool hash )
{
   char buf[MSL], tmp[100];
   short i, len;

   if( !pcdata->editor )
   {
      bug( "%s: null editor", __FUNCTION__ );
      if( hash )
         return STRALLOC( "" );
      return str_dup( "" );
   }

   buf[0] = '\0';
   for( i = 0; i < pcdata->editor->numlines; ++i )
   {
      mudstrlcpy( tmp, pcdata->editor->line[i], 100 );
      len = strlen( tmp );
      if( len > 0 && tmp[len - 1] == '~' )
         tmp[len - 1] = '\0';
      else
         mudstrlcat( tmp, "\n", 100 );
      smash_tilde( tmp );
      mudstrlcat( buf, tmp, MSL );
   }
   if( hash )
   {
      if( buf[0] != '\0' )
         return STRALLOC( buf );
      return NULL;
   }
   return str_dup( buf );
}

/*
 * Simple but nice and handy line editor. - Thoric
 */
void char_data::edit_buffer( string & argument )
{
   descriptor_data *d;
   editor_data *edit;
   string cmd;
   char buf[MIL];
   short x, line;
   bool esave = false;

   if( !( d = desc ) )
   {
      print( "You have no descriptor.\r\n" );
      return;
   }

   if( d->connected != CON_EDITING )
   {
      print( "You can't do that!\r\n" );
      bug( "%s: d->connected != CON_EDITING", __FUNCTION__ );
      return;
   }

   if( substate <= SUB_PAUSE )
   {
      print( "You can't do that!\r\n" );
      bug( "%s: illegal ch->substate (%d)", __FUNCTION__, substate );
      d->connected = CON_PLAYING;
      return;
   }

   if( !pcdata->editor )
   {
      print( "You can't do that!\r\n" );
      bug( "%s: null editor", __FUNCTION__ );
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
         memset( edit, '\0', sizeof( editor_data ) );
         edit->numlines = 0;
         edit->on_line = 0;
         print( "Buffer cleared.\r\n> " );
         return;
      }
      if( !str_cmp( cmd, "r" ) )
      {
         string word1, word2, sptr, lwptr;
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

            lineln = mudstrlcpy( buf, lwptr.c_str(  ), MIL );
            if( lineln > 79 )
               buf[80] = '\0';

            mudstrlcpy( edit->line[x], buf, 81 );
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
            mudstrlcpy( temp_buf + p, edit->line[x], MSL + max_buf_lines - p );
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
            print( "Buffer is full.\r\n> " );
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
                  mudstrlcpy( edit->line[x], edit->line[x - 1], 81 );
               mudstrlcpy( edit->line[line], "", 81 );
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
                  memset( edit, '\0', sizeof( editor_data ) );
                  edit->numlines = 0;
                  edit->on_line = 0;
                  print( "Line deleted.\r\n> " );
                  return;
               }
               for( x = line; x < ( edit->numlines - 1 ); ++x )
                  mudstrlcpy( edit->line[x], edit->line[x + 1], 81 );
               mudstrlcpy( edit->line[edit->numlines--], "", 81 );
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
               char tmpline[MSL];

               snprintf( tmpline, MSL, "%2d> %s\r\n", x + 1, edit->line[x] );
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
         pcdata->dest_buf = NULL;
         pcdata->spare_ptr = NULL;
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
      print( "You buffer is full.\r\n" );
   else
   {
      if( argument.length(  ) > 80 )
      {
         mudstrlcpy( buf, argument.c_str(  ), 80 );
         print( "(Long line trimmed)\r\n> " );
      }
      else
         mudstrlcpy( buf, argument.c_str(  ), MIL );
      mudstrlcpy( edit->line[edit->on_line++], buf, 81 );
      if( edit->on_line > edit->numlines )
         ++edit->numlines;
      if( edit->numlines > max_buf_lines )
      {
         edit->numlines = max_buf_lines;
         print( "Buffer full.\r\n" );
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
