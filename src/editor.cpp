/****************************************************************************
 *                   ^     +----- |  / ^     ^ |     | +-\                  *
 *                  / \    |      | /  |\   /| |     | |  \                 *
 *                 /   \   +---   |<   | \ / | |     | |  |                 *
 *                /-----\  |      | \  |  v  | |     | |  /                 *
 *               /       \ |      |  \ |     | +-----+ +-/                  *
 ****************************************************************************
 * AFKMud Copyright 1997-2007 by Roger Libiez (Samson),                     *
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
#include "mud.h"
#include "descriptor.h"

const int max_buf_lines = 60;

struct editor_data
{
   editor_data();
   ~editor_data();

   char *desc;
   char line[49][81];
   short numlines;
   short on_line;
   short size;
};

editor_data::editor_data()
{
   init_memory( &desc, &size, sizeof( size ) );
}

editor_data::~editor_data()
{
   STRFREE( desc );
}

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

void stralloc_printf( char **pointer, char *fmt, ... )
{
   char buf[MSL * 4];
   va_list args;

   va_start( args, fmt );
   vsnprintf( buf, MSL * 4, fmt, args );
   va_end( args );

   STRFREE( *pointer );
   *pointer = STRALLOC( buf );
   return;
}

void strdup_printf( char **pointer, char *fmt, ... )
{
   char buf[MSL * 4];
   va_list args;

   va_start( args, fmt );
   vsnprintf( buf, MSL * 4, fmt, args );
   va_end( args );

   DISPOSE( *pointer );
   *pointer = str_dup( buf );
   return;
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

   ret = new char[strlen(str) +1];
   mudstrlcpy( ret, str, strlen(str) + 1 );
   return ret;
}  
#endif

vector < string > vector_argument( string arg, int chop )
{
   vector < string > v;
   char cEnd;
   int passes = 0;

   if( arg.find( '\'' ) < arg.find( '"' ) )
      cEnd = '\'';
   else
      cEnd = '"';

   while( !arg.empty(  ) )
   {
      string::size_type space = arg.find( ' ' );

      if( space == string::npos )
         space = arg.length(  );

      string::size_type quote = arg.find( cEnd );

      if( quote != string::npos && quote < space )
      {
         arg = arg.substr( quote + 1, arg.length(  ) );
         if( ( quote = arg.find( cEnd ) ) != string::npos )
            space = quote;
         else
            space = arg.length(  );
      }

      string piece = arg.substr( 0, space );
      strip_lspace( piece );
      if( !piece.empty(  ) )
         v.push_back( piece );

      if( space < arg.length(  ) - 1 )
         arg = arg.substr( space + 1, arg.length(  ) );
      else
         break;

      /*
       * A -1 indicates you want to proceed until arg is exhausted
       */
      if( ++passes == chop && chop != -1 )
      {
         strip_lspace( piece );
         v.push_back( arg );
         break;
      }
   }
   return v;
}

/*
 * Pick off one argument from a string and return the rest.
 * Understands quotes. No longer mangles case either. That used to be annoying.
 */
char *one_argument( char *argument, char *arg_first )
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
int number_argument( string argument, string &arg )
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
   number = atoi( pdot.c_str() );
   arg = argument.substr( x+1, argument.length() );
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

/* Bruce Eckel's Thinking in C++, 2nd Ed
 * Case insensitive compare function:
 *
 * Modifed a bit by Samson cause all I want to know is if it's the same string or not.
 * Returns true if it's the same, false if it's not.
 */
bool scomp( const string & s1, const string & s2 )
{
   // Select the first element of each string:
   string::const_iterator p1 = s1.begin(  ), p2 = s2.begin(  );

   // First check - are they even the same length? No? Then get out. Samson 10-24-04
   if( s1.size(  ) != s2.size(  ) )
      return false;

   // Don't run past the end:
   while( p1 != s1.end(  ) && p2 != s2.end(  ) )
   {
      // Compare upper-cased chars:
      if( toupper( *p1 ) != toupper( *p2 ) )
         // Report which was lexically  greater:
         // Modified here to return false - they don't match now. Samson 10-24-04
         return false;
      ++p1;
      ++p2;
   }

   // If they match up to the detected eos, say 
   // which was longer. Return 0 if the same.
   // Modified here to return true - they match at this stage. Samson 10-24-04
   return true;
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
         log_printf_plus( LOG_DEBUG, LEVEL_ADMIN, "astr: (null)  bstr: %s\n", bstr );
      return true;
   }

   if( !bstr )
   {
      bug( "%s: null bstr.", __FUNCTION__ );
      if( astr )
         log_printf_plus( LOG_DEBUG, LEVEL_ADMIN, "astr: %s  bstr: (null)\n", astr );
      return true;
   }

   for( ; *astr || *bstr; ++astr, ++bstr )
   {
      if( LOWER( *astr ) != LOWER( *bstr ) )
         return true;
   }
   return false;
}

/*
 * Compare strings, case insensitive, for prefix matching.
 * Return true if needle not a prefix of haystack
 *   (compatibility with historical functions).
 */
bool str_prefix( const string needle, const string haystack )
{
   unsigned int x;

   if( needle.empty() )
   {
      bug( "%s: null needle.", __FUNCTION__ );
      return true;
   }

   if( haystack.empty() )
   {
      bug( "%s: null haystack.", __FUNCTION__ );
      return true;
   }

   for( x = 0; x < needle.length(); ++x )
   {
      if( LOWER( needle[x] ) != LOWER( haystack[x] ) )
         return true;
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
	if( LOWER(*needle) != LOWER(*haystack) )
	   return true;
   }
   return false;
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

/*
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

/* Scan a whole argument for a single word - return true if found - Samson 7-24-00 */
/* Code by Orion Elder */
bool arg_cmp( char *haystack, char *needle )
{
   char argument[MSL];

   for( ;; )
   {
      haystack = one_argument( haystack, argument );

      if( !argument || argument[0] == '\0' )
         return false;
      else if( !str_cmp( argument, needle ) )
         return true;
      else
         continue;
   }
   return false;
}

// Strips off trailing tildes on lines from files which will no longer need them - Samson 3-1-05
void strip_tilde( string & line )
{
   string::size_type tilde;

   tilde = line.find_last_not_of( '~' );
   if( tilde != string::npos )
      line = line.substr( 0, tilde + 1 );
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
void strip_tspace( string &line )
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

string strip_cr( string str )
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
char *strip_cr( char *str )
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

char *strip_crlf( char *str )
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
void invert_string( const char *orig, char *inv )
{
   const char *o_ptr = orig;
   char *i_ptr = inv;

   if( orig == inv || orig == NULL || inv == NULL )
      return;

   for( o_ptr += strlen( orig ) - 1; o_ptr != orig - 1; --o_ptr, ++i_ptr )
      (*i_ptr) = (*o_ptr);
   (*i_ptr) = '\0';
   return;
}

/* Provided by Remcon to stop crashes with channel history */
char *add_percent( char *str )
{
   static char newstr[MSL];
   int i, j;

   if( !str || str[0] == '\0' )
      return "";

   for( i = j = 0; str[i] != '\0'; ++i )
   {
      if( str[i] == '%' )
         newstr[j++] = '%';
      newstr[j++] = str[i];
   }
   newstr[j] = '\0';
   return newstr;
}

/*
 * Returns an initial-capped string.
 */
char *capitalize( const char *str )
{
   static char strcap[MSL];
   int i;

   for( i = 0; str[i] != '\0'; ++i )
      strcap[i] = LOWER( str[i] );
   strcap[i] = '\0';
   strcap[0] = UPPER( strcap[0] );
   return strcap;
}

/*
 * Returns an initial-capped string.
 */
string capitalize( const string str )
{
   string strcap;
   char strc[MSL];
   unsigned int i;

   for( i = 0; i < str.length(); ++i )
      strc[i] = LOWER( str[i] );
   strc[i] = '\0';
   strc[0] = UPPER( strc[0] );
   strcap = strc;
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
char *aoran( const char *str )
{
   static char temp[MSL];

   if( !str )
   {
      bug( "%s: NULL str", __FUNCTION__ );
      return "";
   }

   if( isavowel( str[0] ) || ( strlen( str ) > 1 && LOWER( str[0] ) == 'y' && !isavowel( str[1] ) ) )
      mudstrlcpy( temp, "an ", MSL );
   else
      mudstrlcpy( temp, "a ", MSL );
   mudstrlcat( temp, str, MSL );
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

   return;
}

void smash_tilde( string& str )
{
   string::size_type x;

   while( ( x = str.find( '~' ) ) != string::npos )
      str = str.erase( x, 1 );
}

/*
 * Encodes the tildes in a string. - Thoric
 * Used for player-entered strings that go into disk files.
 */
void hide_tilde( char *str )
{
   for( ; *str != '\0'; ++str )
      if( *str == '~' )
         *str = HIDDEN_TILDE;

   return;
}

char *show_tilde( const char *str )
{
   static char buf[MSL];
   char *bufptr;

   bufptr = buf;
   for( ; *str != '\0'; ++str, ++bufptr )
   {
      if( *str == HIDDEN_TILDE )
         *bufptr = '~';
      else
         *bufptr = *str;
   }
   *bufptr = '\0';
   return buf;
}

/*
   Original Code from SW:FotE 1.1
   Reworked strrep function. 
   Fixed a few glaring errors. It also will not overrun the bounds of a string.
   -- Xorith
*/
char *strrep( const char *src, const char *sch, const char *rep )
{
   int lensrc = strlen( src ), lensch = strlen( sch ), lenrep = strlen( rep ), x, y, in_p;
   static char newsrc[MSL];
   bool searching = false;

   newsrc[0] = '\0';
   for( x = 0, in_p = 0; x < lensrc; ++x, ++in_p )
   {
      if( src[x] == sch[0] )
      {
         searching = true;
         for( y = 0; y < lensch; ++y )
            if( src[x + y] != sch[y] )
               searching = false;

         if( searching )
         {
            for( y = 0; y < lenrep; ++y, ++in_p )
            {
               if( in_p == ( MSL - 1 ) )
               {
                  newsrc[in_p] = '\0';
                  return newsrc;
               }
               newsrc[in_p] = rep[y];
            }
            x += lensch - 1;
            --in_p;
            searching = false;
            continue;
         }
      }
      if( in_p == ( MSL - 1 ) )
      {
         newsrc[in_p] = '\0';
         return newsrc;
      }
      newsrc[in_p] = src[x];
   }
   newsrc[in_p] = '\0';
   return newsrc;
}

/* A rather dangerous function if passed with funky data - so just don't, ok? - Samson */
char *strrepa( const char *src, const char *sch[], const char *rep[] )
{
   static char newsrc[MSL];
   int x;

   mudstrlcpy( newsrc, src, MSL );
   for( x = 0; sch[x]; ++x )
      mudstrlcpy( newsrc, strrep( newsrc, sch[x], rep[x] ), MSL );

   return newsrc;
}

/*
 * Return true if an argument is completely numeric.
 */
bool is_number( string arg )
{
   unsigned int x;
   bool first = true;

   if( arg.empty() )
      return false;

   for( x = 0; x < arg.length(); ++x )
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

void editor_print_info( char_data * ch, editor_data * edd, short max_size, char *argument )
{
   ch->printf( "Currently editing: %s\r\n"
               "Total lines: %4d   On line:  %4d\r\n"
               "Buffer size: %4d   Max size: %4d\r\n",
               edd->desc ? edd->desc : "(Null description)", edd->numlines, edd->on_line, edd->size, max_size );
}

void char_data::set_editor_desc( char *new_desc )
{
   if( !pcdata->editor )
      return;

   STRFREE( pcdata->editor->desc );
   pcdata->editor->desc = STRALLOC( new_desc );
}

void char_data::editor_desc_printf( char *desc_fmt, ... )
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
   if( !data.empty() )
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
         if( lines >= 49 || size > 4096 )
         {
            edit->line[lines][lpos] = '\0';
            break;
         }
      }
   }

   if( lpos > 0 && lpos < 78 && lines < 49 )
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
   if( !data )
      bug( "%s: data is NULL!", __FUNCTION__ );
   else
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
         if( lines >= 49 || size > 4096 )
         {
            edit->line[lines][lpos] = '\0';
            break;
         }
      }

   if( lpos > 0 && lpos < 78 && lines < 49 )
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

string char_data::copy_buffer()
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
      if( tmp && tmp[len - 1] == '~' )
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
      if( tmp && tmp[len - 1] == '~' )
         tmp[len - 1] = '\0';
      else
         mudstrlcat( tmp, "\n", 100 );
      smash_tilde( tmp );
      mudstrlcat( buf, tmp, MSL );
   }
   if( hash )
      return STRALLOC( buf );
   return str_dup( buf );
}

/*
 * Simple but nice and handy line editor. - Thoric
 */
void char_data::edit_buffer( char *argument )
{
   descriptor_data *d;
   editor_data *edit;
   char cmd[MIL], buf[MIL];
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
      if( !str_cmp( cmd + 1, "?" ) )
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
      if( !str_cmp( cmd + 1, "c" ) )
      {
         memset( edit, '\0', sizeof( editor_data ) );
         edit->numlines = 0;
         edit->on_line = 0;
         print( "Buffer cleared.\r\n> " );
         return;
      }
      if( !str_cmp( cmd + 1, "r" ) )
      {
         char word1[MIL], word2[MIL];
         char *sptr, *wptr, *lwptr;
         int count, wordln, word2ln, lineln;

         sptr = one_argument( argument, word1 );
         sptr = one_argument( sptr, word1 );
         sptr = one_argument( sptr, word2 );
         if( word1[0] == '\0' || word2[0] == '\0' )
         {
            print( "Need word to replace, and replacement.\r\n> " );
            return;
         }
         /* Changed to a case-sensitive version of string compare --Cynshard */
         if( !strcmp( word1, word2 ) )
         {
            print( "Done.\r\n> " );
            return;
         }
         count = 0;
         wordln = strlen( word1 );
         word2ln = strlen( word2 );
         printf( "Replacing all occurrences of %s with %s...\r\n", word1, word2 );
         for( x = 0; x < edit->numlines; ++x )
         {
            lwptr = edit->line[x];
            while( ( wptr = strstr( lwptr, word1 ) ) != NULL )
            {
               ++count;
               lineln = snprintf( buf, MIL, "%s%s", word2, wptr + wordln );
               if( lineln + wptr - edit->line[x] > 79 )
                  buf[lineln] = '\0';
               mudstrlcpy( wptr, buf, 81 - ( wptr - lwptr ) );
               lwptr = wptr + word2ln;
            }
         }
         printf( "Found and replaced %d occurrence(s).\r\n> ", count );
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
      if( !str_cmp( cmd + 1, "f" ) )
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

      if( !str_cmp( cmd + 1, "p" ) )
      {
         editor_print_info( this, edit, max_buf_lines, argument );
         return;
      }

      if( !str_cmp( cmd + 1, "i" ) )
      {
         if( edit->numlines >= max_buf_lines )
            print( "Buffer is full.\r\n> " );
         else
         {
            if( argument[2] == ' ' )
               line = atoi( argument + 2 ) - 1;
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

      if( !str_cmp( cmd + 1, "d" ) )
      {
         if( edit->numlines == 0 )
            print( "Buffer is empty.\r\n> " );
         else
         {
            if( argument[2] == ' ' )
               line = atoi( argument + 2 ) - 1;
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

      if( !str_cmp( cmd + 1, "g" ) )
      {
         if( edit->numlines == 0 )
            print( "Buffer is empty.\r\n> " );
         else
         {
            if( argument[2] == ' ' )
               line = atoi( argument + 2 ) - 1;
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

      if( !str_cmp( cmd + 1, "l" ) )
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
               desc->write_to_buffer( tmpline, 0 );
            }
            print( "------------------\r\n> " );
         }
         return;
      }

      if( !str_cmp( cmd + 1, "a" ) )
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

      if( get_trust(  ) > LEVEL_IMMORTAL && !str_cmp( cmd + 1, "!" ) )
      {
         DO_FUN *lastcmd;
         int Csubstate = substate;

         lastcmd = last_cmd;
         substate = SUB_RESTRICTED;
         interpret( this, argument + 3 );
         substate = Csubstate;
         last_cmd = lastcmd;
         set_color( AT_GREEN );
         print( "\r\n> " );
         return;
      }

      if( !str_cmp( cmd + 1, "s" ) )
      {
         d->connected = CON_PLAYING;

         if( !last_cmd )
            return;

         ( *last_cmd ) ( this, "" );
         return;
      }
   }

   if( edit->size + strlen( argument ) + 1 >= MSL - 2 )
      print( "You buffer is full.\r\n" );
   else
   {
      if( strlen( argument ) > 80 )
      {
         mudstrlcpy( buf, argument, 80 );
         print( "(Long line trimmed)\r\n> " );
      }
      else
         mudstrlcpy( buf, argument, MIL );
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
