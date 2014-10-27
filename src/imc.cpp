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
 *                            IMC2 Freedom Client                           *
 ****************************************************************************/

/* IMC2 Freedom Client - Developed by Mud Domain.
 *
 * Copyright (C)2004 by Roger Libiez ( Samson )
 * Contributions by Johnathan Walker ( Xorith ), Copyright (C)2004
 * Additional contributions by Jesse Defer ( Garil ), Copyright (C)2004
 * Additional contributions by Rogel, Copyright (c) 2004
 * Comments and suggestions welcome: imc@imc2-intermud.org
 * License terms are available in the imc2freedom.license file.
 */

#include <sys/time.h>
#include <fcntl.h>
#if defined(WIN32)
 #include <winsock2.h>
 #include <windows.h>
 #define dlsym( handle, name ) ( (void*)GetProcAddress( (HINSTANCE) (handle), (name) ) )
 #define dlerror() GetLastError()
 #define EINPROGRESS WSAEINPROGRESS
 void gettimeofday( struct timeval *, struct timezone * );
#else
 #include <sys/socket.h>
 #include <arpa/inet.h>
 #include <netdb.h>
 #include <fnmatch.h>
 #include <dlfcn.h>
#endif
#include <cstdarg>
#include <cerrno>
#include <fstream>
#include <iomanip>
#include "mud.h"
#include "commands.h"
#include "descriptor.h"
#include "event.h"
#include "imc.h"
#include "sha256.h"
#include "roomindex.h"

extern int num_logins;  /* Logins since reboot - external */

int imcwait;   /* Reconnect timer */
int imcconnect_attempts;   /* How many times have we tried to reconnect? */
unsigned long imc_sequencenumber;   /* sequence# for outgoing packets */
bool imcpacketdebug = false;
time_t imcucache_clock; /* prune ucache stuff regularly */
time_t imc_time;  /* Current clock time for the client */

void imclog( const char *, ... ) __attribute__ ( ( format( printf, 1, 2 ) ) );
void imcbug( const char *, ... ) __attribute__ ( ( format( printf, 1, 2 ) ) );
void imc_printf( char_data *, const char *, ... ) __attribute__ ( ( format( printf, 2, 3 ) ) );
void imcpager_printf( char_data *, const char *, ... ) __attribute__ ( ( format( printf, 2, 3 ) ) );
IMC_FUN *imc_function( string );
string imc_send_social( char_data *, string, int );
void imc_save_config( );
void imc_save_channels( );
void to_channel( const char *, char *, int );
void update_trdata( int, int );

char *const imcperm_names[] = {
   "Notset", "None", "Mort", "Imm", "Admin", "Imp"
};

char *const imcflag_names[] = {
   "imctell", "imcdenytell", "imcbeep", "imcdenybeep", "imcinvis", "imcprivate",
   "imcdenyfinger", "imcafk", "imccolor", "imcpermoverride", "imcnotify"
};

imc_siteinfo *this_imcmud;

map<string,string> color_imcmap;
map<string,string> color_mudmap;
map<string,PACKET_FUN*> phandler;
list<string> imc_banlist;
list<imc_channel*>imc_chanlist;
list<imc_help_table*>imc_helplist;
list<imc_remoteinfo*>imc_reminfolist;
list<imc_ucache_data*>imc_ucachelist;
list<imc_command_table*>imc_commandlist;
who_template *whot = NULL;

/**********************
 * Logging functions. *
 **********************/

/* Generic log function which will route the log messages to the appropriate system logging function */
void imclog( const char *format, ... )
{
   char buf[LGST], buf2[LGST];
   char *strtime;
   va_list ap;

   va_start( ap, format );
   vsnprintf( buf, LGST, format, ap );
   va_end( ap );

   snprintf( buf2, LGST, "IMC: %s", buf );

   strtime = c_time( imc_time, -1 );
   fprintf( stderr, "%s :: %s\n", strtime, buf2 );

   to_channel( buf, "ilog", LEVEL_ADMIN );
   return;
}

/* Generic bug logging function which will route the message to the appropriate function that handles bug logs */
void imcbug( const char *format, ... )
{
   char buf[LGST], buf2[LGST];
   va_list ap;

   va_start( ap, format );
   vsnprintf( buf, LGST, format, ap );
   va_end( ap );

   snprintf( buf2, LGST, "IMC: %s", buf );

   bug( "%s", buf2 );
   return;
}

/********************************
 * User level output functions. *
 *******************************/

string imc_strip_colors( string cbuf )
{
   map < string, string >::iterator cmap = color_imcmap.begin(  );
   map < string, string >::iterator cmap2 = color_mudmap.begin(  );

   if( cbuf.empty(  ) )
      return "";

   while( cmap != color_imcmap.end(  ) )
   {
      string::size_type iToken = 0;

      while( ( iToken = cbuf.find( cmap->first ) ) != string::npos )
         cbuf = cbuf.erase( iToken, 2 );
      ++cmap;
   }

   while( cmap2 != color_mudmap.end(  ) )
   {
      string::size_type iToken = 0;

      while( ( iToken = cbuf.find( cmap2->first ) ) != string::npos )
         cbuf = cbuf.erase( iToken, 2 );
      ++cmap2;
   }
   return cbuf;
}

/* Now tell me this isn't cleaner than the mess that was here before. -- Xorith */
/* Yes, Xorith it is. Now, how about this update? Much less hassle with no hardcoded table! -- Samson */
/* Oh baby, now take a look at this freshly C++ized version! Thanks Noplex :) */
/* convert from imc color -> mud color */
string color_itom( string cbuf, char_data * ch )
{
   if( cbuf.empty(  ) )
      return "";

   if( IMCIS_SET( IMCFLAG( ch ), IMC_COLORFLAG ) )
   {
      map < string, string >::iterator cmap = color_imcmap.begin(  );

      while( cmap != color_imcmap.end(  ) )
      {
         string::size_type iToken = 0;

         while( ( iToken = cbuf.find( cmap->first ) ) != string::npos )
            cbuf = cbuf.replace( iToken, 2, cmap->second );
         ++cmap;
      }
   }
   else
      cbuf = imc_strip_colors( cbuf );

   return cbuf;
}

/* convert from mud color -> imc color */
string color_mtoi( string cbuf )
{
   map < string, string >::iterator cmap = color_mudmap.begin(  );

   if( cbuf.empty(  ) )
      return "";

   while( cmap != color_mudmap.end(  ) )
   {
      string::size_type iToken = 0;

      while( ( iToken = cbuf.find( cmap->first ) ) != string::npos )
         cbuf = cbuf.replace( iToken, 2, cmap->second );
      ++cmap;
   }
   return cbuf;
}

/* Generic send_to_char type function to send to the proper code for each codebase */
void imc_to_char( string txt, char_data * ch )
{
   ch->printf( "%s\033[0m", color_itom( txt, ch ).c_str(  ) );
   return;
}

/* Modified version of Smaug's ch_printf_color function */
void imc_printf( char_data * ch, const char *fmt, ... )
{
   char buf[LGST];
   va_list args;

   va_start( args, fmt );
   vsnprintf( buf, LGST, fmt, args );
   va_end( args );

   imc_to_char( buf, ch );
}

/* Generic send_to_pager type function to send to the proper code for each codebase */
void imc_to_pager( string txt, char_data * ch )
{
   ch->printf( "%s&D", color_itom( txt, ch ).c_str(  ) );
   return;
}

/* Generic pager_printf type function */
void imcpager_printf( char_data * ch, const char *fmt, ... )
{
   string txt;
   char buf[LGST];
   va_list args;

   va_start( args, fmt );
   vsnprintf( buf, LGST, fmt, args );
   va_end( args );

   txt = buf;
   imc_to_pager( txt, ch );
   return;
}

/********************************
 * Low level utility functions. *
 ********************************/

/* Check for a name in a list */
bool imc_hasname( string list, string member )
{
   vector < string > arg = vector_argument( list, -1 );
   vector < string >::iterator vec = arg.begin(  );

   if( arg.empty(  ) )
      return false;

   while( vec != arg.end(  ) )
   {
      if( scomp( ( *vec ), member ) )
         return true;
      ++vec;
   }
   return false;
}

/* Add a new member to the list, provided it's not already there */
void imc_addname( string & list, string member )
{
   if( imc_hasname( list, member ) )
      return;

   if( list.empty(  ) )
      list = member;
   else
      list.append( " " + member );
   strip_lspace( list );
   return;
}

/* Remove a member from a list, provided it's there. */
void imc_removename( string & list, string member )
{
   vector < string > arg = vector_argument( list, -1 );
   vector < string >::iterator vec = arg.begin(  );
   ostringstream newlist;

   if( !imc_hasname( list, member ) )
      return;

   while( vec != arg.end(  ) )
   {
      if( scomp( ( *vec ), member ) )
      {
         ++vec;
         continue;
      }
      newlist << ( *vec ) << " ";
      ++vec;
   }
   list = newlist.str(  );
   strip_spaces( list );
   return;
}

string imc_nameof( string src )
{
   string::size_type x;
   string name;

   if( ( x = src.find( '@' ) ) != string::npos && x > 0 )
      return src.substr( 0, x );

   return src;
}

string imc_mudof( string src )
{
   string::size_type x;
   string name;

   if( ( x = src.find( '@' ) ) != string::npos && x > 0 )
      return src.substr( x + 1, src.length(  ) );

   return src;
}

string imc_channel_nameof( string src )
{
   string::size_type x;
   string name;

   if( ( x = src.find( ':' ) ) != string::npos && x > 0 )
      return src.substr( x + 1, src.length(  ) );

   return src;
}

string imc_channel_mudof( string src )
{
   string::size_type x;
   string name;

   if( ( x = src.find( ':' ) ) != string::npos && x > 0 )
      return src.substr( 0, x );

   return src;
}

string imc_makename( string person, string mud )
{
   ostringstream name;

   name << person << "@" << mud;
   return name.str(  );
}

string imcgetname( string from )
{
   string mud, name;

   mud = imc_mudof( from );
   name = imc_nameof( from );

   if( mud == this_imcmud->localname )
      return name;
   return from;
}

/*
 * Returns a char_data class which matches the string
 */
char_data *imc_find_user( string name )
{
   list<descriptor_data*>::iterator ds;
   char_data *vch = NULL;

   for( ds = dlist.begin(); ds != dlist.end(); ++ds )
   {
      descriptor_data *d = (*ds);

      if( ( vch = d->character ? d->character : d->original ) != NULL && !strcasecmp( CH_IMCNAME( vch ), name.c_str(  ) )
          && d->connected == CON_PLAYING )
         return vch;
   }
   return NULL;
}

void imc_check_wizperms( char_data *ch )
{
   if( this_imcmud && ch->level < this_imcmud->adminlevel )
      ch->pcdata->imcchardata->imcperm = IMCPERM_IMM;
   if( this_imcmud && ch->level < this_imcmud->immlevel )
      ch->pcdata->imcchardata->imcperm = IMCPERM_MORT;
}

/* check if a packet from a given source should be ignored */
bool imc_isbanned( string who )
{
   list<string>::iterator iban;

   for( iban = imc_banlist.begin(  ); iban != imc_banlist.end(  ); ++iban )
   {
      string name = (*iban);

      if( scomp( name, imc_mudof( who ) ) )
         return true;
   }
   return false;
}

/* Beefed up to include wildcard ignores. */
bool imc_isignoring( char_data * ch, string ignore )
{
   list<string>::iterator iign;

   /*
    * Wildcard support thanks to Xorith 
    */
   for( iign = CH_IMCDATA( ch )->imc_ignore.begin(  ); iign != CH_IMCDATA( ch )->imc_ignore.end(  ); ++iign )
   {
      string ign = (*iign);

#if defined(WIN32)
      if( !str_prefix( ign, ignore ) )
#else
      if( !fnmatch( ign.c_str(  ), ignore.c_str(  ), 0 ) )
#endif
         return true;
   }
   return false;
}

/* There should only one of these..... */
void imc_delete_info( void )
{
   // DISPOSE( this_imcmud->outbuf );
   free( this_imcmud->outbuf );
   this_imcmud->outbuf = NULL;
   deleteptr( this_imcmud );
}

/* create a new info entry, insert into list */
void imc_new_reminfo( string mud, string version, string netname, string url, string path )
{
   imc_remoteinfo *p = new imc_remoteinfo;

   p->rname = mud;

   if( url.empty(  ) )
      p->url = "Unknown";
   else
      p->url = url;

   if( version.empty(  ) )
      p->version = "Unknown";
   else
      p->version = version;

   if( netname.empty(  ) )
      p->network = this_imcmud->network;
   else
      p->network = netname;

   if( path.empty(  ) )
      p->path = "UNKNOWN";
   else
      p->path = path;

   p->expired = false;

   list<imc_remoteinfo*>::iterator irin;
   for( irin = imc_reminfolist.begin(  ); irin != imc_reminfolist.end(  ); ++irin )
   {
      imc_remoteinfo *rin = (*irin);
      if( rin->rname >= mud )
      {
         imc_reminfolist.insert( irin, p );
         return;
      }
   }
   imc_reminfolist.push_back( p );
   return;
}

/* find an info entry for "name" */
imc_remoteinfo *imc_find_reminfo( string name )
{
   list<imc_remoteinfo*>::iterator irin;

   for( irin = imc_reminfolist.begin(  ); irin != imc_reminfolist.end(  ); ++irin )
   {
      imc_remoteinfo *rin = (*irin);

      if( scomp( rin->rname, name ) )
         return rin;
   }
   return NULL;
}

bool check_mud( char_data * ch, string mud )
{
   imc_remoteinfo *r = imc_find_reminfo( mud );

   if( !r )
   {
      imc_printf( ch, "~W%s ~cis not a valid mud name.\r\n", mud.c_str(  ) );
      return false;
   }

   if( r->expired )
   {
      imc_printf( ch, "~W%s ~cis not connected right now.\r\n", r->rname.c_str(  ) );
      return false;
   }
   return true;
}

bool check_mudof( char_data * ch, string mud )
{
   return check_mud( ch, imc_mudof( mud ) );
}

int get_imcflag( const char *flag )
{
   for( unsigned int x = 0; x < ( sizeof( imcflag_names ) / sizeof( imcflag_names[0] ) ); ++x )
      if( !strcasecmp( flag, imcflag_names[x] ) )
         return x;
   return -1;
}

int get_imcpermvalue( string flag )
{
   for( unsigned int x = 0; x < ( sizeof( imcperm_names ) / sizeof( imcperm_names[0] ) ); ++x )
      if( scomp( flag, imcperm_names[x] ) )
         return x;
   return -1;
}

bool imccheck_permissions( char_data * ch, int checkvalue, int targetvalue, bool enforceequal )
{
   if( checkvalue < 0 || checkvalue > IMCPERM_IMP )
   {
      imc_to_char( "Invalid permission setting.\r\n", ch );
      return false;
   }

   if( checkvalue > IMCPERM( ch ) )
   {
      imc_to_char( "You cannot set permissions higher than your own.\r\n", ch );
      return false;
   }

   if( checkvalue == IMCPERM( ch ) && IMCPERM( ch ) != IMCPERM_IMP && enforceequal )
   {
      imc_to_char( "You cannot set permissions equal to your own. Someone higher up must do this.\r\n", ch );
      return false;
   }

   if( IMCPERM( ch ) < targetvalue )
   {
      imc_to_char( "You cannot alter the permissions of someone or something above your own.\r\n", ch );
      return false;
   }
   return true;
}

imc_channel *imc_findchannel( string name )
{
   list<imc_channel*>::iterator ichn;

   for( ichn = imc_chanlist.begin(  ); ichn != imc_chanlist.end(  ); ++ichn )
   {
      imc_channel *chn = (*ichn);

      if( ( !chn->chname.empty(  ) && scomp( chn->chname, name ) )
          || ( !chn->local_name.empty(  ) && scomp( chn->local_name, name ) ) )
         return chn;
   }
   return NULL;
}

void imc_freechan( imc_channel * c )
{
   if( !c )
   {
      imcbug( "%s: Freeing NULL channel!", __FUNCTION__ );
      return;
   }
   deleteptr( c );
   return;
}

void imcformat_channel( char_data * ch, imc_channel * d, int format, bool all )
{
   ostringstream buf, buf2, buf3;

   if( all )
   {
      list<imc_channel*>::iterator ichn;
      for( ichn = imc_chanlist.begin(  ); ichn != imc_chanlist.end(  ); ++ichn )
      {
         imc_channel *chn = (*ichn);

         if( chn->local_name.empty(  ) )
            continue;

         if( format == 1 || format == 4 )
         {
            buf << "~R[~Y" << chn->local_name << "~R] ~C%s: ~c%s";
            chn->regformat = buf.str(  );
         }
         if( format == 2 || format == 4 )
         {
            buf2 << "~R[~Y" << chn->local_name << "~R] ~c%s %s";
            chn->emoteformat = buf2.str(  );
         }
         if( format == 3 || format == 4 )
         {
            buf3 << "~R[~Y" << chn->local_name << "~R] ~c%s";
            chn->socformat = buf3.str(  );
         }
      }
   }
   else
   {
      if( ch && d->local_name.empty(  ) )
      {
         imc_to_char( "This channel is not yet locally configured.\r\n", ch );
         return;
      }

      if( format == 1 || format == 4 )
      {
         buf << "~R[~Y" << d->local_name << "~R] ~C%s: ~c%s";
         d->regformat = buf.str(  );
      }
      if( format == 2 || format == 4 )
      {
         buf2 << "~R[~Y" << d->local_name << "~R] ~c%s %s";
         d->emoteformat = buf2.str(  );
      }
      if( format == 3 || format == 4 )
      {
         buf3 << "~R[~Y" << d->local_name << "~R] ~c%s";
         d->socformat = buf3.str(  );
      }
   }
   imc_save_channels(  );
   return;
}

void imc_new_channel( string chan, string owner, string ops, string invite, string exclude, bool copen, int perm,
                      string lname )
{
   if( chan.empty(  ) )
   {
      imclog( "%s: NULL channel name received, skipping", __FUNCTION__ );
      return;
   }

   if( chan.find( ':' ) == string::npos )
   {
      imclog( "%s: Improperly formatted channel name: %s", __FUNCTION__, chan.c_str(  ) );
      return;
   }

   imc_channel *c = new imc_channel;
   init_memory( &c->flags, &c->refreshed, sizeof( c->refreshed ) );

   c->chname = chan;
   c->owner = owner;
   c->operators = ops;
   c->invited = invite;
   c->excluded = exclude;

   if( !lname.empty(  ) )
      c->local_name = lname;

   else
      c->local_name = imc_channel_nameof( c->chname );

   c->level = perm;
   c->refreshed = true;
   c->open = copen;
   imcformat_channel( NULL, c, 4, false );
   imc_chanlist.push_back( c );
   return;
}

/******************************************
 * Packet handling and routing functions. *
 ******************************************/

void imc_register_packet_handler( string name, PACKET_FUN * func )
{
   if( phandler.find( name ) != phandler.end() )
   {
      imclog( "Unable to register packet type %s. Another module has already registered it.", name.c_str(  ) );
      return;
   }
   phandler[name] = func;
   return;
}

string escape_string( string sData )
{
   string sDataReturn = "";
   unsigned int i = 0;
   bool quote = false;

   if( sData.find( ' ' ) != string::npos )
      quote = true;

   for( i = 0; i < sData.length(  ); ++i )
   {
      if( sData[i] == '"' )
         sDataReturn += "\\\"";
      else if( sData[i] == '\\' )
         sDataReturn += "\\\\";
      else if( sData[i] == '\n' )
         sDataReturn += "\\n";
      else if( sData[i] == '\r' )
         sDataReturn += "\\r";
      else
         sDataReturn += sData[i];
   }

   if( quote )
      sDataReturn = '"' + sDataReturn + '"';

   return sDataReturn;
}

string unescape_string( string sData )
{
   string sDataReturn = "";
   unsigned int i = 0;

   for( i = 0; i < sData.length(  ); ++i )
   {
      if( sData[i] == '\\' && sData[i + 1] == '\"' )
      {
         sDataReturn += '"';
         ++i;
      }
      else if( sData[i] == '\\' & sData[i+1] == '\\' )
      {
         sDataReturn += "\\";
         i++;
      }
      else if( sData[i] == '\\' && sData[i + 1] == 'n' )
      {
         sDataReturn += "\n";
         ++i;
      }
      else if( sData[i] == '\\' && sData[i + 1] == 'r' )
      {
         sDataReturn += "\r";
         ++i;
      }
      else
         sDataReturn += sData[i];
   }
   return sDataReturn;
}

string imcParseWord( string & line )
{
   if( line.empty(  ) )
      return "";

   // check for spaces; if there is no space check for line breaking; if all 
   // else fails go directly to the end of the string ; length ; 
   string::size_type iSpace = line.find( ' ' );

   if( iSpace == string::npos )
   {
      iSpace = line.find( '\n' );
      if( iSpace == string::npos )
      {
         iSpace = line.find( '\r' );
         if( iSpace == string::npos )
            iSpace = line.length(  );
      }
   }

   // added the sequence to check for quotes; no packet should have a quote 
   // character unless it has strings with spaces inside of it
   string::size_type iQuote = line.find( '"' );

   if( iQuote < iSpace && line[iQuote - 1] != '\\' )
   {
      line = line.substr( iQuote + 1, line.length(  ) );
      while( ( iQuote = line.find( '"', iSpace + 1 ) ) != string::npos )
      {
         iSpace = iQuote;

         if( iQuote > 0 && line[iQuote - 1] == '\\' )
            continue;
         else
            break;
      }
   }

   string sWord = line.substr( 0, iSpace );
   if( iSpace < ( line.length(  ) - 1 ) )
      line = line.substr( iSpace + 1, line.length(  ) );

   return sWord;
}

map < string, string > imc_getData( string packet )
{
   map < string, string > dataMap;
   string::size_type iEqual = 0;

   dataMap.clear(  );

   while( ( iEqual = packet.find( '=' ) ) != string::npos )
   {
      string key = packet.substr( 0, iEqual );

      if( iEqual < ( packet.length(  ) - 1 ) )
         packet = packet.substr( iEqual + 1, packet.length(  ) );
      else
      {
         key.clear(  );
         break;
      }
      strip_lspace( key );
      strip_lspace( packet );

      string value = imcParseWord( packet );
      dataMap[key] = unescape_string( value );
   }
   return dataMap;
}

void imc_write_buffer( string txt )
{
   char output[IMC_BUFF_SIZE];
   size_t length;

   /*
    * This should never happen 
    */
   if( !this_imcmud || this_imcmud->desc < 1 )
   {
      imcbug( "%s: Configuration or socket is invalid!", __FUNCTION__ );
      return;
   }

   /*
    * This should never happen either 
    */
   if( !this_imcmud->outbuf )
   {
      imcbug( "%s: Output buffer has not been allocated!", __FUNCTION__ );
      return;
   }

   snprintf( output, IMC_BUFF_SIZE, "%s\r\n", txt.c_str(  ) );
   length = strlen( output );

   /*
    * Expand the buffer as needed.
    */
   while( this_imcmud->outtop + length >= this_imcmud->outsize )
   {
      if( this_imcmud->outsize > 64000 )
      {
         /*
          * empty buffer 
          */
         this_imcmud->outtop = 0;
         imcbug( "Buffer overflow: %ld. Purging.", this_imcmud->outsize );
         return;
      }
      this_imcmud->outsize *= 2;
      RECREATE( this_imcmud->outbuf, char, this_imcmud->outsize );
   }

   /*
    * Copy.
    */
   strncpy( this_imcmud->outbuf + this_imcmud->outtop, output, length );   /* Leave this one alone! BAD THINGS(TM) will happen if you don't! */
   this_imcmud->outtop += length;
   this_imcmud->outbuf[this_imcmud->outtop] = '\0';
   return;
}

/*
 * Convert a packet to text to then send to the buffer
 */
void imc_packet::send(  )
{
   ostringstream txt;

   /*
    * Assemble your buffer, and at the same time disassemble the packet struct to free the memory 
    */
   txt << from << " " << ++imc_sequencenumber << " " << this_imcmud->localname << " " << type << " " << to;
   txt << " " << data.str(  );

   delete this;

   imc_write_buffer( txt.str(  ) );
   return;
}

imc_packet::imc_packet( string pfrom, string ptype, string pto )
{
   if( ptype.empty(  ) )
   {
      imcbug( "%s: Attempt to build packet with no type field.", __FUNCTION__ );
      ptype = "BORKED";
   }

   if( pfrom.empty(  ) )
   {
      imcbug( "%s: Attempt to build %s packet with no from field.", __FUNCTION__, ptype.c_str(  ) );
      pfrom = "BORKED";
   }

   if( pto.empty(  ) )
   {
      imcbug( "%s: Attempt to build %s packet with no to field.", __FUNCTION__, ptype.c_str(  ) );
      pto = "BORKED";
   }

   from = pfrom + "@" + this_imcmud->localname;
   type = ptype;
   to = pto;
}

imc_packet::imc_packet(  )
{
   from.clear(  );
   type.clear(  );
   to.clear(  );
}

void imc_update_tellhistory( char_data * ch, string msg )
{
   ostringstream new_msg;
   struct tm *local = localtime( &imc_time );
   int x;

   new_msg << "~R[" << setw( 2 ) << local->tm_hour << ":" << setw( 2 ) << local->tm_min << "] " << msg;

   for( x = 0; x < MAX_IMCTELLHISTORY; ++x )
   {
      if( IMCTELLHISTORY( ch, x ).empty(  ) )
      {
         IMCTELLHISTORY( ch, x ) = new_msg.str(  );
         break;
      }

      if( x == MAX_IMCTELLHISTORY - 1 )
      {
         int i;

         for( i = 1; i < MAX_IMCTELLHISTORY; ++i )
            IMCTELLHISTORY( ch, i - 1 ) = IMCTELLHISTORY( ch, i );

         IMCTELLHISTORY( ch, x ) = new_msg.str(  );
      }
   }
   return;
}

void imc_send_tell( string from, string to, string txt, int reply )
{
   imc_packet *p;

   p = new imc_packet( from, "tell", to );
   p->data << "text=" << escape_string( txt );
   if( reply > 0 )
      p->data << " isreply=" << reply;
   p->send(  );

   return;
}

PFUN( imc_recv_tell )
{
   char_data *victim;
   map < string, string > keymap = imc_getData( packet );
   ostringstream buf;

   if( !( victim = imc_find_user( imc_nameof( q->to ) ) ) || IMCPERM( victim ) < IMCPERM_MORT )
   {
      buf << "No player named " << q->to << " exists here.";
      imc_send_tell( "*", q->from, buf.str(  ), 1 );
      return;
   }

   if( imc_nameof( q->from ) != "ICE" )
   {
      if( IMCISINVIS( victim ) )
      {
         if( imc_nameof( q->from ) != "*" )
         {
            buf << q->to << " is not receiving tells.";
            imc_send_tell( "*", q->from, buf.str(  ), 1 );
         }
         return;
      }

      if( imc_isignoring( victim, q->from ) )
      {
         if( imc_nameof( q->from ) != "*" )
         {
            buf << q->to << " is not receiving tells.";
            imc_send_tell( "*", q->from, buf.str(  ), 1 );
         }
         return;
      }

      if( IMCIS_SET( IMCFLAG( victim ), IMC_TELL ) || IMCIS_SET( IMCFLAG( victim ), IMC_DENYTELL ) )
      {
         if( imc_nameof( q->from ) != "*" )
         {
            buf << q->to << " is not receiving tells.";
            imc_send_tell( "*", q->from, buf.str(  ), 1 );
         }
         return;
      }

      if( IMCAFK( victim ) )
      {
         if( imc_nameof( q->from ) != "*" )
         {
            buf << q->to << " is currently AFK. Try back later.";
            imc_send_tell( "*", q->from, buf.str(  ), 1 );
         }
         return;
      }

      if( imc_nameof( q->from ) != "*" )
      {
         IMC_RREPLY( victim ) = q->from;
         IMC_RREPLY_NAME( victim ) = imcgetname( q->from );
      }
   }

   /*
    * Tell social 
    */
   if( keymap["isreply"] == "2" )
      buf << "~WImctell: ~c" << keymap["text"] << "\r\n";
   else
      buf << "~C" << imcgetname( q->from ) << " ~cimctells you ~c'~W" << keymap["text"] << "~c'~!\r\n";
   imc_to_char( buf.str(  ), victim );
   imc_update_tellhistory( victim, buf.str(  ) );
   return;
}

PFUN( imc_recv_emote )
{
   char_data *ch;
   list<descriptor_data*>::iterator ds;
   map < string, string > keymap = imc_getData( packet );
   int level;

   level = get_imcpermvalue( keymap["level"] );
   if( level < 0 || level > IMCPERM_IMP )
      level = IMCPERM_IMM;

   for( ds = dlist.begin(); ds != dlist.end(); ++ds )
   {
      descriptor_data *d = (*ds);

      if( d->connected == CON_PLAYING && ( ch = d->original ? d->original : d->character ) != NULL
          && IMCPERM( ch ) >= level )
         imc_printf( ch, "~p[~GIMC~p] %s %s\r\n", imcgetname( q->from ).c_str(  ), keymap["text"].c_str(  ) );
   }
   return;
}

void update_imchistory( imc_channel * channel, string message )
{
   ostringstream buf;
   struct tm *local;
   int x;

   if( !channel )
   {
      imcbug( "%s: NULL channel received!", __FUNCTION__ );
      return;
   }

   if( message.empty(  ) )
   {
      imcbug( "%s: NULL message received!", __FUNCTION__ );
      return;
   }

   for( x = 0; x < MAX_IMCHISTORY; ++x )
   {
      if( channel->history[x].empty(  ) )
      {
         local = localtime( &imc_time );
         buf << "~R[" << setw( 2 ) << local->tm_mon + 1 << "/" << setw( 2 ) << local->tm_mday;
         buf << " " << setw( 2 ) << local->tm_hour << ":" << setw( 2 ) << local->tm_min << "] ~G" << message;
         channel->history[x] = buf.str(  );

         if( IMCIS_SET( channel->flags, IMCCHAN_LOG ) )
         {
            ofstream stream;
            ostringstream fname;

            fname << IMC_DIR << channel->local_name << ".log";
            stream.open( fname.str(  ).c_str(  ), ios::app );
            if( !stream.is_open(  ) )
            {
               perror( fname.str(  ).c_str(  ) );
               imcbug( "Could not open file %s!", fname.str(  ).c_str(  ) );
            }
            else
            {
               stream << imc_strip_colors( channel->history[x] ) << endl;
               stream.close(  );
            }
         }
         break;
      }

      if( x == MAX_IMCHISTORY - 1 )
      {
         int y;

         for( y = 1; y < MAX_IMCHISTORY; ++y )
         {
            int z = y - 1;

            if( !channel->history[z].empty(  ) )
               channel->history[z] = channel->history[y];
         }

         local = localtime( &imc_time );
         buf << "~R[" << setw( 2 ) << local->tm_mon + 1 << "/" << setw( 2 ) << local->tm_mday;
         buf << " " << setw( 2 ) << local->tm_hour << ":" << setw( 2 ) << local->tm_min << "] ~G" << message;
         channel->history[x] = buf.str(  );

         if( IMCIS_SET( channel->flags, IMCCHAN_LOG ) )
         {
            ofstream stream;
            ostringstream fname;

            fname << IMC_DIR << channel->local_name << ".log";
            stream.open( fname.str(  ).c_str(  ), ios::app );
            if( !stream.is_open(  ) )
            {
               perror( fname.str(  ).c_str(  ) );
               imcbug( "Could not open file %s!", fname.str(  ).c_str(  ) );
            }
            else
            {
               stream << imc_strip_colors( channel->history[x] ) << endl;
               stream.close(  );
            }
         }
      }
   }
   return;
}

void imc_display_channel( imc_channel * c, string from, string txt, int emote )
{
   char_data *ch;
   list<descriptor_data*>::iterator ds;
   ostringstream name;
   char buf[LGST];

   if( c->local_name.empty(  ) || !c->refreshed )
      return;

   if( emote < 2 )
      snprintf( buf, LGST, emote ? c->emoteformat.c_str(  ) : c->regformat.c_str(  ), from.c_str(  ), txt.c_str(  ) );
   else
      snprintf( buf, LGST, c->socformat.c_str(  ), txt.c_str(  ) );

   for( ds = dlist.begin(); ds != dlist.end(); ++ds )
   {
      descriptor_data *d = (*ds);

      ch = d->original ? d->original : d->character;

      if( !ch || d->connected != CON_PLAYING )
         continue;

      /*
       * Freaking stupid PC_DATA crap! 
       */
      if( ch->isnpc(  ) )
         continue;

      if( IMCPERM( ch ) < c->level || !imc_hasname( IMC_LISTEN( ch ), c->local_name ) )
         continue;

      if( !c->open )
      {
         name << CH_IMCNAME( ch ) << "@" << this_imcmud->localname;
         if( !imc_hasname( c->invited, name.str(  ) ) && c->owner != name.str(  ) )
            continue;
      }
      imc_printf( ch, "%s\r\n", buf );
   }
   update_imchistory( c, buf );
}

PFUN( imc_recv_pbroadcast )
{
   imc_channel *c;
   map < string, string > keymap = imc_getData( packet );
   int em;

   em = atoi( keymap["emote"].c_str(  ) );
   if( em < 0 || em > 2 )
      em = 0;

   if( !( c = imc_findchannel( keymap["channel"] ) ) )
      return;

   imc_display_channel( c, keymap["realfrom"], keymap["text"], em );
   return;
}

PFUN( imc_recv_broadcast )
{
   imc_channel *c;
   map < string, string > keymap = imc_getData( packet );
   int em;

   em = atoi( keymap["emote"].c_str(  ) );
   if( em < 0 || em > 2 )
      em = 0;

   if( !( c = imc_findchannel( keymap["channel"] ) ) )
      return;

   if( keymap["sender"].empty(  ) )
      imc_display_channel( c, q->from, keymap["text"], em );
   else
      imc_display_channel( c, keymap["sender"], keymap["text"], em );
   return;
}

/* Send/recv private channel messages */
void imc_sendmessage( imc_channel * c, string name, string text, int emote )
{
   imc_packet *p;

   /*
    * Private channel 
    */
   if( !c->open )
   {
      ostringstream to;

      to << "IMC@" << imc_channel_mudof( c->chname );
      p = new imc_packet( name, "ice-msg-p", to.str(  ) );
   }
   /*
    * Public channel 
    */
   else
      p = new imc_packet( name, "ice-msg-b", "*@*" );

   p->data << "channel=" << c->chname;
   p->data << " text=" << escape_string( text );
   p->data << " emote=" << emote;
   p->data << " echo=1";

   p->send(  );

   return;
}

string get_local_chanwho( imc_channel *c )
{
   int count = 0, col = 0;
   list<descriptor_data*>::iterator ds;
   char_data *person;
   ostringstream buf;

   buf << "The following people are listening to " << c->local_name << " on " << this_imcmud->localname << ":" << endl << endl;
   for( ds = dlist.begin(); ds != dlist.end(); ++ds )
   {
      descriptor_data *d = (*ds);

      person = d->original ? d->original : d->character;

      if( !person )
         continue;

      if( IMCISINVIS( person ) )
         continue;

      if( !imc_hasname( IMC_LISTEN( person ), c->local_name ) )
         continue;

      buf << setw( 15 ) << setiosflags( ios::left ) << CH_IMCNAME( person );
      ++count;
      if( ++col % 3 == 0 )
      {
         col = 0;
         buf << endl;
      }
   }
   if( col != 0 )
      buf << endl;

   /*
    * Send no response to a broadcast request if nobody is listening. 
    */
   if( count == 0 )
      buf << "Nobody" << endl;
   else
      buf << endl;
   return buf.str();
}

PFUN( imc_recv_chanwhoreply )
{
   imc_channel *c;
   char_data *victim;
   map < string, string > keymap = imc_getData( packet );

   if( !( c = imc_findchannel( keymap["channel"] ) ) )
      return;

   if( !( victim = imc_find_user( imc_nameof( q->to ) ) ) )
      return;

   imc_printf( victim, "~G%s", keymap["list"].c_str(  ) );
   return;
}

PFUN( imc_recv_chanwho )
{
   imc_packet *p;
   imc_channel *c;
   map < string, string > keymap = imc_getData( packet );
   ostringstream buf;

   int level = get_imcpermvalue( keymap["level"] );
   if( level < 0 || level > IMCPERM_IMP )
      level = IMCPERM_ADMIN;

   if( !( c = imc_findchannel( keymap["channel"] ) ) )
      return;

   if( c->local_name.empty(  ) )
      buf << "Channel " << keymap["lname"] << " is not locally configured on " << this_imcmud->localname << endl;
   else if( c->level > level )
      buf << "Channel " << keymap["lname"] << " is above your permission level on " << this_imcmud->localname << endl;
   else
   {
      string cwho = get_local_chanwho( c );

      if( ( cwho.empty() || cwho == "" || cwho == "Nobody" ) && q->to == "*" )
         return;
      buf << cwho;
   }

   p = new imc_packet( "*", "ice-chan-whoreply", q->from );
   p->data << "channel=" << c->chname;
   p->data << " list=" << escape_string( buf.str(  ) );
   p->send(  );

   return;
}

void imc_sendnotify( char_data * ch, string chan, bool chon )
{
   imc_packet *p;
   imc_channel *channel;

   if( !IMCIS_SET( IMCFLAG( ch ), IMC_NOTIFY ) )
      return;

   if( !( channel = imc_findchannel( chan ) ) )
      return;

   p = new imc_packet( CH_IMCNAME( ch ), "channel-notify", "*@*" );
   p->data << "channel=" << channel->chname;
   p->data << " status=" << chon;
   p->send(  );

   return;
}

PFUN( imc_recv_channelnotify )
{
   imc_channel *c;
   char_data *ch;
   list<descriptor_data*>::iterator ds;
   map < string, string > keymap = imc_getData( packet );
   char buf[LGST];
   bool chon = false;

   chon = atoi( keymap["status"].c_str(  ) );

   if( !( c = imc_findchannel( keymap["channel"] ) ) )
      return;

   if( c->local_name.empty(  ) )
      return;

   if( chon == true )
      snprintf( buf, LGST, c->emoteformat.c_str(  ), q->from.c_str(  ), "has joined the channel." );
   else
      snprintf( buf, LGST, c->emoteformat.c_str(  ), q->from.c_str(  ), "has left the channel." );

   for( ds = dlist.begin(); ds != dlist.end(); ++ds )
   {
      descriptor_data *d = (*ds);

      ch = d->original ? d->original : d->character;

      if( !ch || d->connected != CON_PLAYING )
         continue;

      /*
       * Freaking stupid PC_DATA crap! 
       */
      if( ch->isnpc(  ) )
         continue;

      if( IMCPERM( ch ) < c->level || !imc_hasname( IMC_LISTEN( ch ), c->local_name ) )
         continue;

      if( !IMCIS_SET( IMCFLAG( ch ), IMC_NOTIFY ) )
         continue;

      imc_printf( ch, "%s\r\n", buf );
   }
   return;
}

string imccenterline( string src, int length )
{
   string stripped, outbuf;
   int front, amount;

   stripped = imc_strip_colors( src );
   amount = length - stripped.length(  ); /* Determine amount to put in front of line */

   if( amount < 1 )
      amount = 1;

   outbuf = "";
   front = amount / 2;
   while( --front > 0 )
      outbuf += ' ';

   outbuf += src;

   if( ( amount / 2 ) * 2 == amount )
      amount /= 2;
   else
      amount = ( amount / 2 ) + 1;

   while( --amount > 0 )
      outbuf += ' ';

   return outbuf;
}

string imcrankbuffer( char_data * ch )
{
   ostringstream rbuf;

   if( IMCPERM( ch ) >= IMCPERM_IMM )
   {
      if( CH_IMCRANK( ch ) && CH_IMCRANK( ch )[0] != '\0' )
         rbuf << "~Y" << color_mtoi( CH_IMCRANK( ch ) );
      else
         rbuf << "~YStaff";
   }
   else
   {
      if( CH_IMCRANK( ch ) && CH_IMCRANK( ch )[0] != '\0' )
         rbuf << "~B" << color_mtoi( CH_IMCRANK( ch ) );
      else
         rbuf << "~BPlayer";
   }
   string rank = imccenterline( rbuf.str(), 20 );
   return rank;
}

void imc_send_whoreply( string to, string txt )
{
   imc_packet *p;

   p = new imc_packet( "*", "who-reply", to );
   p->data << "text=" << escape_string( txt );
   p->send(  );

   return;
}

void imc_send_who( string from, string to, string type )
{
   imc_packet *p;

   p = new imc_packet( from, "who", to );
   p->data << "type=" << escape_string( type );
   p->send(  );

   return;
}

vector < string > break_newlines( string arg )
{
   vector < string > v;
   char cEnd;

   if( arg.find( '\'' ) < arg.find( '"' ) )
      cEnd = '\'';
   else
      cEnd = '"';

   while( !arg.empty(  ) )
   {
      string::size_type space = arg.find( '\n' );

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
   }
   return v;
}

string multiline_center( string splitme )
{
   vector<string> arg = break_newlines( splitme );
   vector<string>::iterator vec = arg.begin();
   string head = "";

   while( vec != arg.end() )
   {
      string line = (*vec);
      string::size_type iToken = 0;

      if( ( iToken = line.find( "<center>" ) ) != string::npos )
      {
         line.erase( iToken, 8 );
         line = imccenterline( line, 78 );
      }
      head.append( line );
      head.append( "\n" );
      ++vec;
   }
   return head;
}

string process_who_head( int plrcount, int maxcount )
{
   string head = whot->head;
   string::size_type iToken = 0;
   ostringstream pcount, mcount;

   pcount << plrcount;
   mcount << maxcount;

   while( ( iToken = head.find( "<%plrcount%>" ) ) != string::npos )
      head = head.replace( iToken, 12, pcount.str() );
   while( ( iToken = head.find( "<%maxcount%>" ) ) != string::npos )
      head = head.replace( iToken, 12, mcount.str() );

   head = multiline_center( head ); // This splits and then looks for <center> directives
   return head;
}

string process_who_tail( int plrcount, int maxcount )
{
   string tail = whot->tail;
   string::size_type iToken = 0;
   ostringstream pcount, mcount;

   pcount << plrcount;
   mcount << maxcount;

   while( ( iToken = tail.find( "<%plrcount%>" ) ) != string::npos )
      tail = tail.replace( iToken, 12, pcount.str() );

   while( ( iToken = tail.find( "<%maxcount%>" ) ) != string::npos )
      tail = tail.replace( iToken, 12, mcount.str() );

   tail = multiline_center( tail ); // This splits and then looks for <center> directives
   return tail;
}

string process_plrline( string plrrank, string plrflags, string plrname, string plrtitle )
{
   string pline = whot->plrline;
   string::size_type iToken = 0;

   while( ( iToken = pline.find( "<%charrank%>" ) ) != string::npos )
      pline = pline.replace( iToken, 12, plrrank );

   while( ( iToken = pline.find( "<%charflags%>" ) ) != string::npos )
      pline = pline.replace( iToken, 13, plrflags );

   while( ( iToken = pline.find( "<%charname%>" ) ) != string::npos )
      pline = pline.replace( iToken, 12, plrname );

   while( ( iToken = pline.find( "<%chartitle%>" ) ) != string::npos )
      pline = pline.replace( iToken, 13, plrtitle );

   pline.append( "\n" );
   return pline;
}

string process_immline( string plrrank, string plrflags, string plrname, string plrtitle )
{
   string pline = whot->immline;
   string::size_type iToken = 0;

   while( ( iToken = pline.find( "<%charrank%>" ) ) != string::npos )
      pline = pline.replace( iToken, 12, plrrank );

   while( ( iToken = pline.find( "<%charflags%>" ) ) != string::npos )
      pline = pline.replace( iToken, 13, plrflags );

   while( ( iToken = pline.find( "<%charname%>" ) ) != string::npos )
      pline = pline.replace( iToken, 12, plrname );

   while( ( iToken = pline.find( "<%chartitle%>" ) ) != string::npos )
      pline = pline.replace( iToken, 13, plrtitle );

   pline.append( "\n" );
   return pline;
}

string process_who_template( string head, string tail, string plrlines, string immlines, string plrheader, string immheader )
{
   string master = whot->master;
   string::size_type iToken = 0;

   while( ( iToken = master.find( "<%head%>" ) ) != string::npos )
      master = master.replace( iToken, 8, head );

   while( ( iToken = master.find( "<%tail%>" ) ) != string::npos )
      master = master.replace( iToken, 8, tail );

   while( ( iToken = master.find( "<%plrheader%>" ) ) != string::npos )
      master = master.replace( iToken, 13, plrheader );

   while( ( iToken = master.find( "<%immheader%>" ) ) != string::npos )
      master = master.replace( iToken, 13, immheader );

   while( ( iToken = master.find( "<%plrline%>" ) ) != string::npos )
      master = master.replace( iToken, 11, plrlines );

   while( ( iToken = master.find( "<%immline%>" ) ) != string::npos )
      master = master.replace( iToken, 11, immlines );
   return master;
}

string imc_assemble_who( void )
{
   char_data *person;
   list<descriptor_data*>::iterator ds;
   string buf, plrlines = "", immlines = "", plrheader = "", immheader = "";
   ostringstream whoreply, whobuf, whobuf2;

   int pcount = 0;

   bool plr = false;
   for( ds = dlist.begin(); ds != dlist.end(); ++ds )
   {
      descriptor_data *d = (*ds);

      person = d->original ? d->original : d->character;
      if( person && d->connected == CON_PLAYING )
      {
         if( IMCPERM( person ) <= IMCPERM_NONE || IMCPERM( person ) >= IMCPERM_IMM )
            continue;

         if( IMCISINVIS( person ) )
            continue;

         ++pcount;

         if( !plr )
         {
            plrheader = whot->plrheader;
            plr = true;
         }

         string rank = imcrankbuffer( person );

         string flags;

         if( IMCAFK( person ) )
            flags = "AFK";
         else
            flags = "---";

         string name = CH_IMCNAME( person );
         string title = color_mtoi( CH_IMCTITLE( person ) );
         string plrline = process_plrline( rank, flags, name, title );
         plrlines.append( plrline );
      }
   }

   bool imm = false;
   for( ds = dlist.begin(); ds != dlist.end(); ++ds )
   {
      descriptor_data *d = (*ds);

      person = d->original ? d->original : d->character;

      if( person && d->connected == CON_PLAYING )
      {
         if( IMCPERM( person ) <= IMCPERM_NONE || IMCPERM( person ) < IMCPERM_IMM )
            continue;

         if( IMCISINVIS( person ) )
            continue;

         ++pcount;

         if( !imm )
         {
            immheader = whot->immheader;
            imm = true;
         }

         string rank = imcrankbuffer( person );

         string flags;

         if( IMCAFK( person ) )
            flags = "AFK";
         else
            flags = "---";

         string name = CH_IMCNAME( person );
         string title = color_mtoi( CH_IMCTITLE( person ) );
         string immline = process_immline( rank, flags, name, title );
         immlines.append( immline );
      }
   }

   string head = process_who_head( pcount, sysdata->maxplayers );
   string tail = process_who_tail( pcount, sysdata->maxplayers );
   string master = process_who_template( head, tail, plrlines, immlines, plrheader, immheader );

   whoreply << master;
   whoreply << "~Y[~W" << num_logins << " logins since last reboot on " << str_boot_time << "~Y]";

   return whoreply.str();
}

void imc_process_who( string from )
{
   string whoreply = imc_assemble_who();
   imc_send_whoreply( from, whoreply );
}

/* Finger code */
void imc_process_finger( string from, string type )
{
   char_data *victim;
   vector < string > arg = vector_argument( type, 1 );
   ostringstream buf;

   if( type.empty(  ) )
      return;

   if( arg.size(  ) < 2 )
      return;

   if( !( victim = imc_find_user( arg[1] ) ) )
   {
      imc_send_whoreply( from, "No such player is online.\r\n" );
      return;
   }

   if( IMCISINVIS( victim ) || IMCPERM( victim ) < IMCPERM_MORT )
   {
      imc_send_whoreply( from, "No such player is online.\r\n" );
      return;
   }

   buf << "\r\n~cPlayer Profile for ~W" << CH_IMCNAME( victim ) << "~c:" << endl;
   buf << "~W-------------------------------\r\n";
   buf << "~cStatus: ~W" << ( IMCAFK( victim ) ? "AFK" : "Lurking about" ) << endl;
   buf << "~cPermission level: ~W" << imcperm_names[IMCPERM( victim )] << endl;
   buf << "~cListening to channels [Names may not match your mud]: ~W" << ( !IMC_LISTEN( victim ).
                                                                            empty(  )? IMC_LISTEN( victim ) : "None" ) <<
      endl;

   if( !IMCIS_SET( IMCFLAG( victim ), IMC_PRIVACY ) )
   {
      buf << "~cEmail   : ~W" << ( !IMC_EMAIL( victim ).empty(  )? IMC_EMAIL( victim ) : "None" ) << endl;
      buf << "~cHomepage: ~W" << ( !IMC_HOMEPAGE( victim ).empty(  )? IMC_HOMEPAGE( victim ) : "None" ) << endl;
      buf << "~cICQ     : ~W" << IMC_ICQ( victim ) << endl;
      buf << "~cAIM     : ~W" << ( !IMC_AIM( victim ).empty(  )? IMC_AIM( victim ) : "None" ) << endl;
      buf << "~cYahoo   : ~W" << ( !IMC_YAHOO( victim ).empty(  )? IMC_YAHOO( victim ) : "None" ) << endl;
      buf << "~cMSN     : ~W" << ( !IMC_MSN( victim ).empty(  )? IMC_MSN( victim ) : "None" ) << endl;
   }

   buf << "~W" << ( !IMC_COMMENT( victim ).empty(  )? IMC_COMMENT( victim ) : "" ) << endl;
   imc_send_whoreply( from, buf.str(  ) );
}

PFUN( imc_recv_who )
{
   map < string, string > keymap = imc_getData( packet );
   ostringstream buf;

   if( keymap["type"] == "who" )
   {
      imc_process_who( q->from );
      return;
   }
   else if( strstr( keymap["type"].c_str(  ), "finger" ) )
   {
      imc_process_finger( q->from, keymap["type"] );
      return;
   }
   else if( keymap["type"] == "info" )
   {
      buf << "\r\n~WMUD Name    : ~c" << this_imcmud->localname << endl;
      buf << "~WHost        : ~c" << this_imcmud->ihost << endl;
      buf << "~WAdmin Email : ~c" << this_imcmud->email << endl;
      buf << "~WWebsite     : ~c" << this_imcmud->www << endl;
      buf << "~WIMC2 Version: ~c" << this_imcmud->versionid << endl;
      buf << "~WDetails     : ~c" << this_imcmud->details << endl;
   }
   else
      buf << keymap["type"] << " is not a valid option. Options are: who, finger, or info." << endl;

   imc_send_whoreply( q->from, buf.str(  ) );
   return;
}

PFUN( imc_recv_whoreply )
{
   char_data *victim;
   map < string, string > keymap = imc_getData( packet );

   if( !( victim = imc_find_user( imc_nameof( q->to ) ) ) )
      return;

   imc_to_pager( keymap["text"], victim );
   return;
}

void imc_send_whoisreply( string to, string data )
{
   imc_packet *p;

   p = new imc_packet( "*", "whois-reply", to );
   p->data << "text=" << escape_string( data );
   p->send(  );
   return;
}

PFUN( imc_recv_whoisreply )
{
   char_data *victim;
   map < string, string > keymap = imc_getData( packet );

   if( ( victim = imc_find_user( imc_nameof( q->to ) ) ) )
      imc_to_char( keymap["text"], victim );
   return;
}

void imc_send_whois( string from, string user )
{
   imc_packet *p;

   p = new imc_packet( from, "whois", user );
   p->send(  );
   return;
}

PFUN( imc_recv_whois )
{
   char_data *victim;
   ostringstream buf;

   if( ( victim = imc_find_user( imc_nameof( q->to ) ) ) && !IMCISINVIS( victim ) )
   {
      buf << "~RIMC Locate: ~Y" << CH_IMCNAME( victim ) << "@" << this_imcmud->localname << ": ~cOnline.\r\n";
      imc_send_whoisreply( q->from, buf.str(  ) );
   }
   return;
}

PFUN( imc_recv_beep )
{
   char_data *victim = NULL;
   ostringstream buf;

   if( !( victim = imc_find_user( imc_nameof( q->to ) ) ) || IMCPERM( victim ) < IMCPERM_MORT )
   {
      buf << "No player named " << q->to << " exists here.";
      imc_send_tell( "*", q->from, buf.str(  ), 1 );
      return;
   }

   if( IMCISINVIS( victim ) )
   {
      if( imc_nameof( q->from ) != "*" )
      {
         buf << q->to << " is not receiving beeps.";
         imc_send_tell( "*", q->from, buf.str(  ), 1 );
      }
      return;
   }

   if( imc_isignoring( victim, q->from ) )
   {
      if( imc_nameof( q->from ) != "*" )
      {
         buf << q->to << " is not receiving beeps.";
         imc_send_tell( "*", q->from, buf.str(  ), 1 );
      }
      return;
   }

   if( IMCIS_SET( IMCFLAG( victim ), IMC_BEEP ) || IMCIS_SET( IMCFLAG( victim ), IMC_DENYBEEP ) )
   {
      if( imc_nameof( q->from ) != "*" )
      {
         buf << q->to << " is not receiving beeps.";
         imc_send_tell( "*", q->from, buf.str(  ), 1 );
      }
      return;
   }

   if( IMCAFK( victim ) )
   {
      if( imc_nameof( q->from ) != "*" )
      {
         buf << q->to << " is currently AFK. Try back later.";
         imc_send_tell( "*", q->from, buf.str(  ), 1 );
      }
      return;
   }

   /*
    * always display the true name here 
    */
   imc_printf( victim, "~c\a%s imcbeeps you.~!\r\n", q->from.c_str(  ) );
}

void imc_send_beep( string from, string to )
{
   imc_packet *p;

   p = new imc_packet( from, "beep", to );
   p->send(  );

   return;
}

PFUN( imc_recv_isalive )
{
   imc_remoteinfo *r;
   map < string, string > keymap = imc_getData( packet );

   if( !( r = imc_find_reminfo( imc_mudof( q->from ) ) ) )
   {
      imc_new_reminfo( imc_mudof( q->from ), keymap["versionid"], keymap["networkname"], keymap["url"], q->route );
      return;
   }

   r->expired = false;

   if( !keymap["url"].empty(  ) )
      r->url = keymap["url"];

   if( !keymap["versionid"].empty(  ) )
      r->version = keymap["versionid"];

   if( !keymap["networkname"].empty(  ) )
      r->network = keymap["networkname"];

   if( !keymap["host"].empty(  ) )
      r->host = keymap["host"];

   if( !keymap["port"].empty(  ) )
      r->port = keymap["port"];

   if( !q->route.empty(  ) )
      r->path = q->route;

   return;
}

PFUN( imc_send_keepalive )
{
   imc_packet *p;

   if( q )
      p = new imc_packet( "*", "is-alive", q->from );
   else
      p = new imc_packet( "*", "is-alive", packet );
   p->data << "versionid=" << escape_string( this_imcmud->versionid );
   p->data << " url=" << this_imcmud->www;
   p->data << " host=" << this_imcmud->ihost;
   p->data << " port=" << this_imcmud->iport;
   p->send(  );

   return;
}

void imc_request_keepalive( void )
{
   imc_packet *p;

   p = new imc_packet( "*", "keepalive-request", "*@*" );
   p->send(  );

   imc_send_keepalive( NULL, "*@*" );
   return;
}

void imc_firstrefresh( void )
{
   imc_packet *p;

   p = new imc_packet( "*", "ice-refresh", "IMC@$" );
   p->send(  );

   return;
}

PFUN( imc_recv_iceupdate )
{
   imc_channel *c;
   map < string, string > keymap = imc_getData( packet );
   int perm;
   bool copen;

   if( keymap["policy"] == "open" )
      copen = true;
   else
      copen = false;

   perm = get_imcpermvalue( keymap["level"] );
   if( perm < 0 || perm > IMCPERM_IMP )
      perm = IMCPERM_ADMIN;

   if( !( c = imc_findchannel( keymap["channel"] ) ) )
   {
      imc_new_channel( keymap["channel"], keymap["owner"], keymap["operators"], keymap["invited"],
                       keymap["excluded"], copen, perm, keymap["localname"] );
      return;
   }

   if( keymap["channel"].empty(  ) )
   {
      imclog( "%s: NULL channel name received, skipping", __FUNCTION__ );
      return;
   }

   c->chname = keymap["channel"];
   c->owner = keymap["owner"];
   c->operators = keymap["operators"];
   c->invited = keymap["invited"];
   c->excluded = keymap["excluded"];
   c->open = copen;
   if( c->level == IMCPERM_NOTSET )
      c->level = perm;

   c->refreshed = true;
   return;
}

PFUN( imc_recv_icedestroy )
{
   imc_channel *c;
   map < string, string > keymap = imc_getData( packet );

   if( !( c = imc_findchannel( keymap["channel"] ) ) )
      return;

   imc_chanlist.remove( c );
   imc_freechan( c );
   imc_save_channels(  );
}

int imctodikugender( int gender )
{
   int sex = 0;

   if( gender == 0 )
      sex = SEX_MALE;

   if( gender == 1 )
      sex = SEX_FEMALE;

   if( gender > 1 )
      sex = SEX_NEUTRAL;

   return sex;
}

int dikutoimcgender( int gender )
{
   int sex = 0;

   if( gender > 2 || gender < 0 )
      sex = 2;

   if( gender == SEX_MALE )
      sex = 0;

   if( gender == SEX_FEMALE )
      sex = 1;

   return sex;
}

int imc_get_ucache_gender( string name )
{
   list<imc_ucache_data*>::iterator iuch;

   for( iuch = imc_ucachelist.begin(  ); iuch != imc_ucachelist.end(  ); ++iuch )
   {
      imc_ucache_data *uch = (*iuch);

      if( scomp( uch->name, name ) )
         return uch->gender;
   }

   /*
    * -1 means you aren't in the list and need to be put there. 
    */
   return -1;
}

/* Saves the ucache info to disk because it would just be spamcity otherwise */
void imc_save_ucache( void )
{
   ofstream stream;

   stream.open( IMC_UCACHE_FILE );
   if( !stream.is_open(  ) )
   {
      imclog( "%s", "Couldn't write to IMC2 ucache file." );
      return;
   }

   list<imc_ucache_data*>::iterator iuch;
   for( iuch = imc_ucachelist.begin(  ); iuch != imc_ucachelist.end(  ); ++iuch )
   {
      imc_ucache_data *uch = (*iuch);

      stream << "#UCACHE" << endl;
      stream << "Name " << uch->name << endl;
      stream << "Sex  " << uch->gender << endl;
      stream << "Time " << uch->time << endl;
      stream << "End\n" << endl;
   }
   stream.close(  );
   return;
}

void imc_prune_ucache( void )
{
   list<imc_ucache_data*>::iterator iuch;
   for( iuch = imc_ucachelist.begin(  ); iuch != imc_ucachelist.end(  ); )
   {
      imc_ucache_data *cache = (*iuch);
      ++iuch;

      /*
       * Info older than 30 days is removed since this person likely hasn't logged in at all 
       */
      if( imc_time - cache->time >= 2592000 )
      {
         imc_ucachelist.remove( cache );
         deleteptr( cache );
      }
   }
   imc_save_ucache(  );
   return;
}

/* Updates user info if they exist, adds them if they don't. */
void imc_ucache_update( string name, int gender )
{
   list<imc_ucache_data*>::iterator iuch;
   for( iuch = imc_ucachelist.begin(  ); iuch != imc_ucachelist.end(  ); ++iuch )
   {
      imc_ucache_data *uch = (*iuch);

      if( scomp( uch->name, name ) )
      {
         uch->gender = gender;
         uch->time = imc_time;
         return;
      }
   }

   imc_ucache_data *user = new imc_ucache_data;
   user->name = name;
   user->gender = gender;
   user->time = imc_time;
   imc_ucachelist.push_back( user );

   imc_save_ucache(  );
   return;
}

void imc_send_ucache_update( string visname, int gender )
{
   imc_packet *p;

   p = new imc_packet( visname, "user-cache", "*@*" );
   p->data << "gender=" << gender;

   p->send(  );
   return;
}

PFUN( imc_recv_ucache )
{
   map < string, string > keymap = imc_getData( packet );
   int sex, gender;

   gender = atoi( keymap["gender"].c_str(  ) );

   sex = imc_get_ucache_gender( q->from );

   if( sex == gender )
      return;

   imc_ucache_update( q->from, gender );
   return;
}

void imc_send_ucache_request( string targetuser )
{
   imc_packet *p;
   ostringstream to;

   to << "*@" << imc_mudof( targetuser );
   p = new imc_packet( "*", "user-cache-request", to.str(  ) );
   p->data << "user=" << targetuser;
   p->send(  );

   return;
}

PFUN( imc_recv_ucache_request )
{
   imc_packet *p;
   map < string, string > keymap = imc_getData( packet );
   ostringstream to;
   int gender;

   gender = imc_get_ucache_gender( keymap["user"] );

   /*
    * Gender of -1 means they aren't in the mud's ucache table. Don't waste the reply packet. 
    */
   if( gender == -1 )
      return;

   to << "*@" << imc_mudof( q->from );
   p = new imc_packet( "*", "user-cache-reply", to.str(  ) );
   p->data << "user=" << keymap["user"];
   p->data << " gender=" << gender;
   p->send(  );

   return;
}

PFUN( imc_recv_ucache_reply )
{
   map < string, string > keymap = imc_getData( packet );
   int sex, gender;

   gender = atoi( keymap["gender"].c_str(  ) );

   sex = imc_get_ucache_gender( keymap["user"] );

   if( sex == gender )
      return;

   imc_ucache_update( keymap["user"], gender );
   return;
}

PFUN( imc_recv_closenotify )
{
   imc_remoteinfo *r;
   map < string, string > keymap = imc_getData( packet );

   if( !( r = imc_find_reminfo( keymap["host"] ) ) )
      return;

   r->expired = true;
   return;
}

void imc_register_default_packets( void )
{
   /*
    * Once registered, these are not cleared unless the mud is shut down 
    */
   if( !phandler.empty(  ) )
      return;

   imc_register_packet_handler( "keepalive-request", imc_send_keepalive );
   imc_register_packet_handler( "is-alive", imc_recv_isalive );
   imc_register_packet_handler( "ice-update", imc_recv_iceupdate );
   imc_register_packet_handler( "ice-msg-r", imc_recv_pbroadcast );
   imc_register_packet_handler( "ice-msg-b", imc_recv_broadcast );
   imc_register_packet_handler( "user-cache", imc_recv_ucache );
   imc_register_packet_handler( "user-cache-request", imc_recv_ucache_request );
   imc_register_packet_handler( "user-cache-reply", imc_recv_ucache_reply );
   imc_register_packet_handler( "tell", imc_recv_tell );
   imc_register_packet_handler( "emote", imc_recv_emote );
   imc_register_packet_handler( "ice-destroy", imc_recv_icedestroy );
   imc_register_packet_handler( "who", imc_recv_who );
   imc_register_packet_handler( "who-reply", imc_recv_whoreply );
   imc_register_packet_handler( "whois", imc_recv_whois );
   imc_register_packet_handler( "whois-reply", imc_recv_whoisreply );
   imc_register_packet_handler( "beep", imc_recv_beep );
   imc_register_packet_handler( "ice-chan-who", imc_recv_chanwho );
   imc_register_packet_handler( "ice-chan-whoreply", imc_recv_chanwhoreply );
   imc_register_packet_handler( "channel-notify", imc_recv_channelnotify );
   imc_register_packet_handler( "close-notify", imc_recv_closenotify );
}

PACKET_FUN *pfun_lookup( string type )
{
   if( phandler.find( type ) != phandler.end() )
      return phandler[type];
   return NULL;
}

void imc_parse_packet( string packet )
{
   imc_packet *p = NULL;
   PACKET_FUN *pfun;
   string pkt;
   vector < string > arg = vector_argument( packet, 5 );
   unsigned long seq = 0;

   if( arg.size(  ) < 5 )
   {
      imcbug( "Invalid packet format for: %s", packet.c_str(  ) );
      return;
   }

   p = new imc_packet;
   p->from = arg[0];
   seq = atol( arg[1].c_str(  ) );
   p->route = arg[2];
   p->type = arg[3];
   p->to = arg[4];
   if( arg.size(  ) > 5 )
      pkt = arg[5];
   else
      pkt = "dummy";

   /* Banned muds are silently dropped - thanks to WynterNyght@IoG for noticing this was missing. */
   if( imc_isbanned( p->from ) )
   {
      deleteptr( p );
      return;
   }

   if( !( pfun = pfun_lookup( p->type ) ) )
   {
      if( imcpacketdebug )
      {
         imclog( "PACKET: From %s, Seq %lu, Route %s, Type %s, To %s, EXTRA %s",
                 p->from.c_str(  ), seq, p->route.c_str(  ), p->type.c_str(  ), p->to.c_str(  ), arg[5].c_str(  ) );
         imclog( "No packet handler function has been defined for %s", p->type.c_str(  ) );
      }
      deleteptr( p );
      return;
   }
   try
   {
      ( *pfun ) ( p, pkt );
   }
   catch( exception & e )
   {
      imclog( "Packet Exception: '%s' Caused by packet: %s %lu %s %s %s %s", e.what(  ),
              p->from.c_str(  ), seq, p->route.c_str(  ), p->type.c_str(  ), p->to.c_str(  ), arg[5].c_str(  ) );
   }
   catch( ... )
   {
      imclog( "Unknown packet exception: Generated by packet: %s %lu %s %s %s %s",
              p->from.c_str(  ), seq, p->route.c_str(  ), p->type.c_str(  ), p->to.c_str(  ), arg[5].c_str(  ) );
   }

   /*
    * This might seem slow, but we need to track muds who don't send is-alive packets 
    */
   if( !( imc_find_reminfo( imc_mudof( p->from ) ) ) )
      imc_new_reminfo( imc_mudof( p->from ), "Unknown", this_imcmud->network, "Unknown", p->route );

   deleteptr( p );
   return;
}

void imc_finalize_connection( string name, string netname )
{
   this_imcmud->state = IMC_ONLINE;

   if( !netname.empty(  ) )
      this_imcmud->network = netname;

   this_imcmud->servername = name;
   imclog( "Connected to %s. Network ID: %s", name.c_str(  ), ( !netname.empty(  ) )? netname.c_str(  ) : "Unknown" );

   imcconnect_attempts = 0;
   imc_request_keepalive(  );
   imc_firstrefresh(  );
   return;
}

/* Handle an autosetup response from a supporting server - Samson 8-12-03 */
void imc_handle_autosetup( string source, string servername, string cmd, string txt, string encrypt )
{
   if( cmd == "reject" )
   {
      if( txt == "connected" )
      {
         imclog( "There is already a mud named %s connected to the network.", this_imcmud->localname.c_str(  ) );
         imc_shutdown( false );
         return;
      }
      if( txt == "private" )
      {
         imclog( "%s is a private server. Autosetup denied.", servername.c_str(  ) );
         imc_shutdown( false );
         return;
      }
      if( txt == "full" )
      {
         imclog( "%s has reached its connection limit. Autosetup denied.", servername.c_str(  ) );
         imc_shutdown( false );
         return;
      }
      if( txt == "ban" )
      {
         imclog( "%s has banned your connection. Autosetup denied.", servername.c_str(  ) );
         imc_shutdown( false );
         return;
      }
      imclog( "%s: Invalid 'reject' response. Autosetup failed.", servername.c_str(  ) );
      imclog( "Data received: %s %s %s %s %s", source.c_str(  ), servername.c_str(  ), cmd.c_str(  ), txt.c_str(  ),
              encrypt.c_str(  ) );
      imc_shutdown( false );
      return;
   }

   if( cmd == "accept" )
   {
      imclog( "%s", "Autosetup completed successfully." );
      if( !encrypt.empty(  ) && encrypt == "SHA256-SET" )
      {
         imclog( "%s", "SHA-256 Authentication has been enabled." );
         this_imcmud->sha256pass = true;
         imc_save_config(  );
      }
      imc_finalize_connection( servername, txt );
      return;
   }

   imclog( "%s: Invalid autosetup response.", servername.c_str(  ) );
   imclog( "Data received: %s %s %s %s %s", source.c_str(  ), servername.c_str(  ), cmd.c_str(  ), txt.c_str(  ),
           encrypt.c_str(  ) );
   imc_shutdown( false );
   return;
}

bool imc_write_socket( void )
{
   const char *ptr = this_imcmud->outbuf;
   int nleft = this_imcmud->outtop, nwritten = 0;

   if( nleft <= 0 )
      return 1;

   while( nleft > 0 )
   {
      if( ( nwritten = send( this_imcmud->desc, ptr, nleft, 0 ) ) <= 0 )
      {
         if( nwritten == -1 && errno == EAGAIN )
         {
            char *p2 = this_imcmud->outbuf;

            ptr += nwritten;

            while( *ptr != '\0' )
               *p2++ = *ptr++;

            *p2 = '\0';

            this_imcmud->outtop = strlen( this_imcmud->outbuf );
            return true;
         }

         if( nwritten < 0 )
            imclog( "Write error on socket: %s", strerror( errno ) );
         else
            imclog( "%s", "Connection close detected on socket write." );

         imc_shutdown( true );
         return false;
      }
      update_trdata( 2, nwritten );
      nleft -= nwritten;
      ptr += nwritten;
   }

   if( imcpacketdebug )
   {
      imclog( "Packet Sent: %s", this_imcmud->outbuf );
      imclog( "Bytes sent: %d", this_imcmud->outtop );
   }
   this_imcmud->outbuf[0] = '\0';
   this_imcmud->outtop = 0;
   return 1;
}

void imc_process_authentication( string packet )
{
   vector < string > arg = vector_argument( packet, 6 );
   ostringstream response;

   if( arg.size(  ) < 2 )
   {
      imclog( "%s", "Incomplete authentication packet. Unable to connect." );
      imc_shutdown( false );
      return;
   }

   if( arg[0] == "SHA256-AUTH-INIT" )
   {
      char pwd[SMST];
      char *cryptpwd;
      long auth_value = 0;

      if( arg.size(  ) < 3 )
      {
         imclog( "SHA-256 Authentication failure: No auth_value was returned by %s.", arg[1].c_str(  ) );
         imc_shutdown( false );
         return;
      }

      /*
       * Lets encrypt this bastard now! 
       */
      auth_value = atol( arg[2].c_str(  ) );
      snprintf( pwd, SMST, "%ld%s%s", auth_value, this_imcmud->clientpw.c_str(  ), this_imcmud->serverpw.c_str(  ) );
      cryptpwd = sha256_crypt( pwd );

      response << "SHA256-AUTH-RESP " << this_imcmud->localname << " " << cryptpwd << " version=" << IMC_VERSION;
      imc_write_buffer( response.str(  ) );
      return;
   }

   /*
    * SHA-256 response is pretty simple. If you blew the authentication, it happened on the server anyway. 
    */
   if( arg[0] == "SHA256-AUTH-APPR" )
   {
      imclog( "%s", "SHA-256 Authentication completed." );
      imc_finalize_connection( arg[1], arg[2] );
      return;
   }

   if( arg.size(  ) < 3 )
   {
      imclog( "%s", "Incomplete authentication packet. Unable to connect." );
      imc_shutdown( false );
      return;
   }

   /*
    * The old way. Nice and icky, but still very much required for compatibility. 
    */
   if( arg[0] == "PW" )
   {
      if( this_imcmud->serverpw != arg[2] )
      {
         imclog( "%s sent an improper serverpassword.", arg[1].c_str(  ) );
         imc_shutdown( false );
         return;
      }

      imclog( "%s", "Standard Authentication completed." );
      if( arg.size(  ) > 5 && arg[5] == "SHA256-SET" )
      {
         imclog( "%s", "SHA-256 Authentication has been enabled." );
         this_imcmud->sha256pass = true;
         imc_save_config(  );
      }
      imc_finalize_connection( arg[1], arg[4] );
      return;
   }

   /*
    * Should only be received from servers supporting this obviously 
    */
   if( arg.size(  ) < 5 )
   {
      imclog( "%s", "Incomplete authentication packet. Unable to connect." );
      imc_shutdown( false );
      return;
   }

   if( arg[0] == "autosetup" )
   {
      imc_handle_autosetup( arg[0], arg[1], arg[2], arg[3], arg[4] );
      return;
   }

   imclog( "Invalid authentication response received from %s!!", arg[1].c_str(  ) );
   imclog( "Data received: %s %s %s %s %s", arg[0].c_str(  ), arg[1].c_str(  ), arg[2].c_str(  ), arg[3].c_str(  ), arg[4].c_str(  ) );
   imc_shutdown( false );
   return;
}

/*
 * Transfer one line from input buffer to input line.
 */
bool imc_read_buffer( void )
{
   unsigned int i = 0, j = 0, k = 0;
   unsigned char ended = 0;

   if( this_imcmud->inbuf[0] == '\0' )
      return 0;

   k = strlen( this_imcmud->incomm );

   if( k < 1 )
      k = 0;

   for( i = 0; this_imcmud->inbuf[i] != '\0'
        && this_imcmud->inbuf[i] != '\n' && this_imcmud->inbuf[i] != '\r' && i < IMC_BUFF_SIZE; ++i )
   {
      this_imcmud->incomm[k++] = this_imcmud->inbuf[i];
   }

   while( this_imcmud->inbuf[i] == '\n' || this_imcmud->inbuf[i] == '\r' )
   {
      ended = 1;
      ++i;
   }

   this_imcmud->incomm[k] = '\0';

   while( ( this_imcmud->inbuf[j] = this_imcmud->inbuf[i + j] ) != '\0' )
      ++j;

   this_imcmud->inbuf[j] = '\0';
   return ended;
}

bool imc_read_socket( void )
{
   unsigned int iStart, iErr;
   bool begin = true;

   iStart = strlen( this_imcmud->inbuf );

   for( ;; )
   {
      int nRead;

      nRead = recv( this_imcmud->desc, this_imcmud->inbuf + iStart, sizeof( this_imcmud->inbuf ) - 10 - iStart, 0 );
      iErr = errno;
      if( nRead > 0 )
      {
         iStart += nRead;

         update_trdata( 1, nRead );
         if( iStart >= sizeof( this_imcmud->inbuf ) - 10 )
            break;

         begin = false;
      }
      else if( nRead == 0 && this_imcmud->state == IMC_ONLINE )
      {
         if( !begin )
            break;

         imclog( "%s", "EOF encountered on IMC2 socket." );
         return false;
      }
      else if( iErr == EAGAIN )
         break;
      else if( nRead == -1 )
      {
         imclog( "%s: Descriptor error on #%d: %s", __FUNCTION__, this_imcmud->desc, strerror( iErr ) );
         return false;
      }
   }
   this_imcmud->inbuf[iStart] = '\0';
   return true;
}

void imc_loop( void )
{
   fd_set in_set, out_set;
   struct timeval last_time, null_time;

   gettimeofday( &last_time, NULL );
   imc_time = ( time_t ) last_time.tv_sec;

   if( imcwait > 0 )
      --imcwait;

   /*
    * Condition reached only if network shutdown after startup 
    */
   if( imcwait == 1 )
   {
      if( ++imcconnect_attempts > 3 )
      {
         if( this_imcmud->sha256pass )
         {
            imclog( "%s", "Unable to reconnect using SHA-256, trying standard authentication." );
            this_imcmud->sha256pass = false;
            imc_save_config();
            imcconnect_attempts = 0;
         }
         else
         {
            imcwait = -2;
            imclog( "%s", "Unable to reestablish connection to server. Abandoning reconnect." );
            return;
         }
      }
      imc_startup( true, -1, false );
      return;
   }

   if( this_imcmud->state == IMC_OFFLINE || this_imcmud->desc == -1 )
      return;

   /*
    * Will prune the cache once every 24hrs after bootup time 
    */
   if( imcucache_clock <= imc_time )
   {
      imcucache_clock = imc_time + 86400;
      imc_prune_ucache(  );
   }

   FD_ZERO( &in_set );
   FD_ZERO( &out_set );
   FD_SET( this_imcmud->desc, &in_set );
   FD_SET( this_imcmud->desc, &out_set );

   null_time.tv_sec = null_time.tv_usec = 0;

   if( select( this_imcmud->desc + 1, &in_set, &out_set, NULL, &null_time ) < 0 )
   {
      perror( "imc_loop: select: poll" );
      imc_shutdown( true );
      return;
   }

   if( FD_ISSET( this_imcmud->desc, &in_set ) )
   {
      if( !imc_read_socket(  ) )
      {
         if( this_imcmud->inbuf && this_imcmud->inbuf[0] != '\0' )
         {
            if( imc_read_buffer( ) )
            {
               if( !strcasecmp( this_imcmud->incomm, "SHA-256 authentication is required." ) )
               {
                  imclog( "%s", "Unable to reconnect using standard authentication, trying SHA-256." );
                  this_imcmud->sha256pass = true;
                  imc_save_config();
               }
               else
                  imclog( "Buffer contents: %s", this_imcmud->incomm );
            }
         }
         FD_CLR( this_imcmud->desc, &out_set );
         imc_shutdown( true );
         return;
      }

      while( imc_read_buffer(  ) )
      {
         if( imcpacketdebug )
            imclog( "Packet received: %s", this_imcmud->incomm );

         switch ( this_imcmud->state )
         {
            default:
            case IMC_OFFLINE:
            case IMC_AUTH1:  /* Auth1 can only be set when still trying to contact the server */
               break;

            case IMC_AUTH2:  /* Now you've contacted the server and need to process the authentication response */
               imc_process_authentication( this_imcmud->incomm );
               this_imcmud->incomm[0] = '\0';
               break;

            case IMC_ONLINE: /* You're up, pass the bastard off to the packet parser */
               imc_parse_packet( this_imcmud->incomm );
               this_imcmud->incomm[0] = '\0';
               break;
         }
      }
   }

   if( this_imcmud->desc > 0 && this_imcmud->outtop > 0 && FD_ISSET( this_imcmud->desc, &out_set ) && !imc_write_socket(  ) )
   {
      this_imcmud->outtop = 0;
      imc_shutdown( true );
   }
   return;
}

/************************************
 * User login and logout functions. *
 ************************************/

void imc_adjust_perms( char_data * ch )
{
   if( !this_imcmud )
      return;

   /*
    * Ugly hack to let the permission system adapt freely, but retains the ability to override that adaptation
    * * in the event you need to restrict someone to a lower level, or grant someone a higher level. This of
    * * course comes at the cost of forgetting you may have done so and caused the override flag to be set, but hey.
    * * This isn't a perfect system and never will be. Samson 2-8-04.
    */
   if( !IMCIS_SET( IMCFLAG( ch ), IMC_PERMOVERRIDE ) )
   {
      if( CH_IMCLEVEL( ch ) < this_imcmud->minlevel )
         IMCPERM( ch ) = IMCPERM_NONE;
      else if( CH_IMCLEVEL( ch ) >= this_imcmud->minlevel && CH_IMCLEVEL( ch ) < this_imcmud->immlevel )
         IMCPERM( ch ) = IMCPERM_MORT;
      else if( CH_IMCLEVEL( ch ) >= this_imcmud->immlevel && CH_IMCLEVEL( ch ) < this_imcmud->adminlevel )
         IMCPERM( ch ) = IMCPERM_IMM;
      else if( CH_IMCLEVEL( ch ) >= this_imcmud->adminlevel && CH_IMCLEVEL( ch ) < this_imcmud->implevel )
         IMCPERM( ch ) = IMCPERM_ADMIN;
      else if( CH_IMCLEVEL( ch ) >= this_imcmud->implevel )
         IMCPERM( ch ) = IMCPERM_IMP;
   }
   return;
}

void imc_char_login( char_data * ch )
{
   ostringstream buf;
   int gender, sex;

   if( !this_imcmud )
      return;

   imc_adjust_perms( ch );

   if( this_imcmud->state != IMC_ONLINE )
   {
      if( IMCPERM( ch ) >= IMCPERM_IMM && imcwait == -2 )
         imc_to_char( "~RThe IMC2 connection is down. Attempts to reconnect were abandoned due to excessive failures.\r\n",
                      ch );
      return;
   }

   if( IMCPERM( ch ) < IMCPERM_MORT )
      return;

   buf << CH_IMCNAME( ch ) << "@" << this_imcmud->localname;
   gender = imc_get_ucache_gender( buf.str(  ) );
   sex = dikutoimcgender( CH_IMCSEX( ch ) );

   if( gender == sex )
      return;

   imc_ucache_update( buf.str(  ), sex );
   if( !IMCIS_SET( IMCFLAG( ch ), IMC_INVIS ) )
      imc_send_ucache_update( CH_IMCNAME( ch ), sex );

   return;
}

void imc_loadchar( char_data * ch, FILE * fp, const char *word )
{
   if( ch->isnpc(  ) )
      return;

   if( IMCPERM( ch ) == IMCPERM_NOTSET )
      imc_adjust_perms( ch );

   switch ( word[0] )
   {
      default:
         bug( "%s: no match: %s", __FUNCTION__, word );
         fread_to_eol( fp );
         break;

      case 'I':
         KEY( "IMCPerm", IMCPERM( ch ), fread_number( fp ) );
         STDSLINE( "IMCEmail", IMC_EMAIL( ch ) );
         STDSLINE( "IMCAIM", IMC_AIM( ch ) );
         KEY( "IMCICQ", IMC_ICQ( ch ), fread_number( fp ) );
         STDSLINE( "IMCYahoo", IMC_YAHOO( ch ) );
         STDSLINE( "IMCMSN", IMC_MSN( ch ) );
         STDSLINE( "IMCHomepage", IMC_HOMEPAGE( ch ) );
         STDSLINE( "IMCComment", IMC_COMMENT( ch ) );
         if( !strcasecmp( word, "IMCFlags" ) )
         {
            IMCFLAG( ch ) = fread_number( fp );
            imc_char_login( ch );
            break;
         }

         if( !strcasecmp( word, "IMCFlag" ) )
         {
            string iflags;

            fread_line( iflags, fp );
            vector < string > arg = vector_argument( iflags, -1 );
            vector < string >::iterator vec = arg.begin(  );

            while( vec != arg.end(  ) )
            {
               int value = get_imcflag( ( *vec ).c_str(  ) );
               if( value < 0 || value >= IMC_MAXFLAG )
                  imclog( "Unknown imcflag: %s", ( *vec ).c_str(  ) );
               else
                  IMCFLAG( ch ).set( value );
               ++vec;
            }
            imc_char_login( ch );
            break;
         }

         if( !strcasecmp( word, "IMClisten" ) )
         {
            fread_line( IMC_LISTEN( ch ), fp );
            if( !IMC_LISTEN( ch ).empty(  ) && this_imcmud->state == IMC_ONLINE )
            {
               imc_channel *channel = NULL;
               vector < string > arg = vector_argument( IMC_LISTEN( ch ), -1 );
               vector < string >::iterator vec = arg.begin(  );

               while( vec != arg.end(  ) )
               {
                  if( !( channel = imc_findchannel( ( *vec ) ) ) )
                     imc_removename( IMC_LISTEN( ch ), ( *vec ) );
                  if( channel && IMCPERM( ch ) < channel->level )
                     imc_removename( IMC_LISTEN( ch ), ( *vec ) );
                  if( imc_hasname( IMC_LISTEN( ch ), ( *vec ) ) )
                     imc_sendnotify( ch, ( *vec ), true );
                  ++vec;
               }
            }
            break;
         }

         if( !strcasecmp( word, "IMCdeny" ) )
         {
            fread_string( IMC_DENY( ch ), fp );
            if( !IMC_DENY( ch ).empty(  ) && this_imcmud->state == IMC_ONLINE )
            {
               imc_channel *channel = NULL;
               vector < string > arg = vector_argument( IMC_DENY( ch ), -1 );
               vector < string >::iterator vec = arg.begin(  );

               while( vec != arg.end(  ) )
               {
                  if( !( channel = imc_findchannel( ( *vec ) ) ) )
                     imc_removename( IMC_DENY( ch ), ( *vec ) );
                  if( channel && IMCPERM( ch ) < channel->level )
                     imc_removename( IMC_DENY( ch ), ( *vec ) );
                  ++vec;
               }
            }
            break;
         }

         if( !strcasecmp( word, "IMCignore" ) )
         {
            string newign;

            fread_string( newign, fp );
            CH_IMCDATA( ch )->imc_ignore.push_back( newign );
            break;
         }
         break;
   }
}

void imc_savechar( char_data * ch, FILE * fp )
{
   if( ch->isnpc(  ) )
      return;

   fprintf( fp, "IMCPerm      %d\n", IMCPERM( ch ) );
   fprintf( fp, "IMCFlag      %s\n", bitset_string( IMCFLAG( ch ), imcflag_names ) );
   if( !IMC_LISTEN( ch ).empty(  ) )
      fprintf( fp, "IMCListen    %s\n", IMC_LISTEN( ch ).c_str(  ) );
   if( !IMC_DENY( ch ).empty(  ) )
      fprintf( fp, "IMCDeny      %s\n", IMC_DENY( ch ).c_str(  ) );
   if( !IMC_EMAIL( ch ).empty(  ) )
      fprintf( fp, "IMCEmail     %s\n", IMC_EMAIL( ch ).c_str(  ) );
   if( !IMC_HOMEPAGE( ch ).empty(  ) )
      fprintf( fp, "IMCHomepage  %s\n", IMC_HOMEPAGE( ch ).c_str(  ) );
   if( IMC_ICQ( ch ) )
      fprintf( fp, "IMCICQ       %d\n", IMC_ICQ( ch ) );
   if( !IMC_AIM( ch ).empty(  ) )
      fprintf( fp, "IMCAIM       %s\n", IMC_AIM( ch ).c_str(  ) );
   if( !IMC_YAHOO( ch ).empty(  ) )
      fprintf( fp, "IMCYahoo     %s\n", IMC_YAHOO( ch ).c_str(  ) );
   if( !IMC_MSN( ch ).empty(  ) )
      fprintf( fp, "IMCMSN       %s\n", IMC_MSN( ch ).c_str(  ) );
   if( !IMC_COMMENT( ch ).empty(  ) )
      fprintf( fp, "IMCComment   %s\n", IMC_COMMENT( ch ).c_str(  ) );

   list<string>::iterator iign;
   for( iign = CH_IMCDATA( ch )->imc_ignore.begin(  ); iign != CH_IMCDATA( ch )->imc_ignore.end(  ); ++iign )
   {
      string ign = (*iign);
      fprintf( fp, "IMCignore    %s\n", ign.c_str(  ) );
   }
   return;
}

void imc_freechardata( char_data * ch )
{
   if( ch->isnpc(  ) )
      return;

   if( CH_IMCDATA( ch ) == NULL )
      return;

   CH_IMCDATA( ch )->imc_ignore.clear(  );

   deleteptr( CH_IMCDATA( ch ) );
   return;
}

void imc_initchar( char_data * ch )
{
   if( ch->isnpc(  ) )
      return;

   CH_IMCDATA( ch ) = new imc_chardata;

   IMC_ICQ( ch ) = 0;
   CH_IMCDATA( ch )->imc_ignore.clear(  );
   IMCFLAG( ch ).reset(  );
   IMCSET_BIT( IMCFLAG( ch ), IMC_COLORFLAG );
   IMCPERM( ch ) = IMCPERM_NOTSET;

   return;
}

/*******************************************
 * Network Startup and Shutdown functions. *
 *******************************************/

void imc_loadhistfile( string filename, imc_channel *channel )
{
   ifstream stream;

   stream.open( filename.c_str() );
   if( !stream.is_open() )
      return;

   for( int x = 0; x < MAX_IMCHISTORY; ++x )
   {
      if( stream.eof(  ) )
         break;
      else
      {
         char line[LGST];

         stream.getline( line, LGST );
         channel->history[x] = line;
      }
   }
   stream.close(  );
   unlink( filename.c_str(  ) );
}

void imc_loadhistory( void )
{
   string filename;
   list<imc_channel*>::iterator ichn;

   for( ichn = imc_chanlist.begin(  ); ichn != imc_chanlist.end(  ); ++ichn )
   {
      imc_channel *chn = (*ichn);

      if( chn->local_name.empty(  ) )
         continue;

      filename = IMC_DIR + chn->local_name + ".hist";

      if( !exists_file( filename.c_str(  ) ) )
         continue;

      imc_loadhistfile( filename, chn );
   }
}

void imc_savehistfile( string filename, imc_channel *channel )
{
   ofstream stream;

   stream.open( filename.c_str() );
   if( !stream.is_open() )
      return;

   for( int x = 0; x < MAX_IMCHISTORY; ++x )
   {
      if( !channel->history[x].empty() )
         stream << channel->history[x] << endl;
   }
   stream.close();
}

void imc_savehistory( void )
{
   string filename;

   list<imc_channel*>::iterator ichn;
   for( ichn = imc_chanlist.begin(  ); ichn != imc_chanlist.end(  ); ++ichn )
   {
      imc_channel *chn = (*ichn);

      if( chn->local_name.empty(  ) )
         continue;

      if( chn->history[0].empty(  ) )
         continue;

      filename = IMC_DIR + chn->local_name + ".hist";
      imc_savehistfile( filename, chn );
   }
}

void imc_save_channels( void )
{
   ofstream stream;

   stream.open( IMC_CHANNEL_FILE );
   if( !stream.is_open(  ) )
   {
      imcbug( "Can't write to %s", IMC_CHANNEL_FILE );
      return;
   }

   list<imc_channel*>::iterator ichn;
   for( ichn = imc_chanlist.begin(  ); ichn != imc_chanlist.end(  ); ++ichn )
   {
      imc_channel *chn = (*ichn);

      if( chn->local_name.empty(  ) )
         continue;

      stream << "#IMCCHAN" << endl;
      stream << "ChanName   " << chn->chname << endl;
      stream << "ChanLocal  " << chn->local_name << endl;
      stream << "ChanRegF   " << chn->regformat << endl;
      stream << "ChanEmoF   " << chn->emoteformat << endl;
      stream << "ChanSocF   " << chn->socformat << endl;
      stream << "ChanLevel  " << chn->level << endl;
      stream << "End" << endl << endl;
   }
   stream.close(  );
}

void imc_loadchannels( void )
{
   imc_channel *c;
   ifstream stream;

   imc_chanlist.clear(  );

   imclog( "%s", "Loading channels..." );

   stream.open( IMC_CHANNEL_FILE );
   if( !stream.is_open(  ) )
   {
      imcbug( "%s", "Can't open imc channel file" );
      return;
   }

   do
   {
      string key, value;
      char buf[LGST];

      stream >> key;
      strip_lspace( key );

      stream.getline( buf, LGST );
      value = buf;
      strip_lspace( value );

      if( key == "#IMCCHAN" )
      {
         c = new imc_channel;
         init_memory( &c->flags, &c->refreshed, sizeof( c->refreshed ) );
      }
      if( key == "ChanName" )
         c->chname = value;
      if( key == "ChanLocal" )
         c->local_name = value;
      if( key == "ChanRegF" )
         c->regformat = value;
      if( key == "ChanEmoF" )
         c->emoteformat = value;
      if( key == "ChanSocF" )
         c->socformat = value;
      if( key == "ChanLevel" )
         c->level = atoi( value.c_str(  ) );
      if( key == "End" )
      {
         for( int x = 0; x < MAX_IMCHISTORY; ++x )
            c->history[x].clear(  );

         c->refreshed = false;   /* Prevents crash trying to use a bogus channel */
         imclog( "configured %s as %s", c->chname.c_str(  ), c->local_name.c_str(  ) );
         imc_chanlist.push_back( c );
      }
   }
   while( !stream.eof(  ) );
   stream.close(  );
   return;
}

/* Save current mud-level ban list. Short, simple. */
void imc_savebans( void )
{
   ofstream stream;

   stream.open( IMC_BAN_FILE );
   if( !stream.is_open(  ) )
   {
      imcbug( "%s: error opening ban file for write", __FUNCTION__ );
      return;
   }

   stream << "#BANLIST" << endl;

   list<string>::iterator iban;
   for( iban = imc_banlist.begin(  ); iban != imc_banlist.end(  ); ++iban )
   {
      string ban = (*iban);
      stream << ban << endl;
   }
   stream.close(  );
   return;
}

void imc_readbans( void )
{
   ifstream stream;

   imc_banlist.clear(  );

   imclog( "%s", "Loading ban list..." );

   stream.open( IMC_BAN_FILE );
   if( !stream.is_open(  ) )
   {
      imcbug( "%s: couldn't open ban file", __FUNCTION__ );
      return;
   }

   do
   {
      string line;

      stream >> line;
      if( line.empty(  ) || line[0] == '#' )
         continue;

      imc_banlist.push_back( line );
   }
   while( !stream.eof(  ) );
   stream.close(  );
   return;
}

void imc_savecolor( void )
{
   ofstream stream;
   map < string, string >::iterator cmap = color_imcmap.begin(  );
   map < string, string >::iterator cmap2 = color_mudmap.begin(  );

   stream.open( IMC_COLOR_FILE );
   if( !stream.is_open(  ) )
   {
      imclog( "%s", "Couldn't write to IMC2 color file." );
      return;
   }

   stream << "#IMC<->MUD COLORMAP" << endl;
   while( cmap != color_imcmap.end(  ) )
   {
      stream << cmap->first << " " << cmap->second << endl;
      ++cmap;
   }

   stream << "#MUD<->IMC COLORMAP" << endl;
   while( cmap2 != color_mudmap.end(  ) )
   {
      stream << cmap2->first << " " << cmap2->second << endl;
      ++cmap2;
   }

   stream.close(  );
   return;
}

void imc_load_color_table( void )
{
   ifstream stream;
   bool cflip = false;

   imclog( "%s", "Loading IMC2 color table..." );

   stream.open( IMC_COLOR_FILE );
   if( !stream.is_open(  ) )
   {
      imclog( "%s", "No color table found." );
      return;
   }

   color_imcmap.clear(  );
   color_mudmap.clear(  );

   do
   {
      string line, key, value;
      char buf[SMST];

      stream.getline( buf, SMST );
      line = buf;
      if( line.empty(  ) )
         continue;

      istringstream lstream( line );
      lstream >> key >> value;
      strip_lspace( key );
      strip_lspace( value );

      if( key == "#IMC<->MUD" )
      {
         cflip = false;
         continue;
      }

      if( key == "#MUD<->IMC" )
      {
         cflip = true;
         continue;
      }

      if( !cflip )
         color_imcmap[key] = value;
      else
         color_mudmap[key] = value;
   }
   while( !stream.eof(  ) );
   stream.close(  );
   return;
}

void imc_savehelps( void )
{
   ofstream stream;

   stream.open( IMC_HELP_FILE );

   if( !stream.is_open(  ) )
   {
      imclog( "%s", "Couldn't write to IMC2 help file." );
      return;
   }

   list<imc_help_table*>::iterator ihlp;
   for( ihlp = imc_helplist.begin(  ); ihlp != imc_helplist.end(  ); ++ihlp )
   {
      imc_help_table *hlp = (*ihlp);

      stream << "#HELP" << endl;
      stream << "Name " << hlp->hname << endl;
      stream << "Perm " << imcperm_names[hlp->level] << endl;
      stream << "Text " << hlp->text << "¢" << endl;
      stream << "End" << endl << endl;
   }
   stream.close(  );
   return;
}

void imc_load_helps( void )
{
   imc_help_table *help;
   ifstream stream;

   imc_helplist.clear(  );

   imclog( "%s", "Loading IMC2 help file..." );

   stream.open( IMC_HELP_FILE );
   if( !stream.is_open(  ) )
   {
      imclog( "%s", "No help file found." );
      return;
   }

   do
   {
      string key, value;
      char buf[LGST];

      stream >> key;
      strip_lspace( key );

      if( key == "#HELP" )
         help = new imc_help_table;

      if( key == "Name" )
      {
         stream.getline( buf, LGST );
         value = buf;
         strip_lspace( value );
         help->hname = value;
      }

      if( key == "Perm" )
      {
         stream.getline( buf, LGST );
         value = buf;
         strip_lspace( value );
         int permvalue = get_imcpermvalue( value );

         if( permvalue < 0 || permvalue > IMCPERM_IMP )
         {
            imclog( "%s: Command %s loaded with invalid permission %s. Set to Imp.", __FUNCTION__, help->hname.c_str(  ),
                    value.c_str(  ) );
            help->level = IMCPERM_IMP;
         }
         else
            help->level = permvalue;
      }

      if( key == "Text" )
      {
         stream.getline( buf, LGST, '¢' );
         value = buf;
         strip_lspace( value );
         help->text = value;
      }

      if( key == "End" )
         imc_helplist.push_back( help );

   }
   while( !stream.eof(  ) );
   stream.close(  );
   return;
}

void imc_savecommands( void )
{
   ofstream stream;

   stream.open( IMC_CMD_FILE );
   if( !stream.is_open(  ) )
   {
      imclog( "%s", "Couldn't write to commands file." );
      return;
   }

   list<imc_command_table*>::iterator icom;
   for( icom = imc_commandlist.begin(  ); icom != imc_commandlist.end(  ); ++icom )
   {
      imc_command_table *com = (*icom);
      list<string>::iterator ials;

      stream << "#COMMAND" << endl;
      stream << "Name      " << com->name << endl;
      if( !com->funcname.empty() )
         stream << "Code      " << com->funcname << endl;
      stream << "Perm      " << imcperm_names[com->level] << endl;
      stream << "Connected " << com->connected << endl;

      for( ials = com->aliaslist.begin(  ); ials != com->aliaslist.end(  ); ++ials )
      {
         string als = (*ials);
         stream << "Alias     " << als << endl;
      }
      stream << "End" << endl << endl;
   }
   stream.close(  );
   return;
}

bool imc_load_commands( void )
{
   imc_command_table *cmd;
   ifstream stream;

   imc_commandlist.clear(  );

   imclog( "%s", "Loading IMC2 command table..." );

   stream.open( IMC_CMD_FILE );
   if( !stream.is_open(  ) )
   {
      imclog( "%s", "No command table found." );
      return false;
   }

   do
   {
      string line, key, value;
      char buf[SMST];

      stream.getline( buf, SMST );
      line = buf;
      if( line.empty(  ) )
         continue;

      istringstream lstream( line );
      lstream >> key >> value;
      strip_lspace( key );
      strip_lspace( value );

      if( key == "#COMMAND" )
      {
         cmd = new imc_command_table;
         cmd->function = NULL;
      }
      if( key == "Name" )
         cmd->name = value;
      if( key == "Code" )
      {
         cmd->funcname = value;
         cmd->function = imc_function( value );
         if( !cmd->function )
         {
            imcbug( "%s: Command %s loaded with invalid function. Set to NULL.", __FUNCTION__, cmd->name.c_str(  ) );
            cmd->funcname.clear();
         }
      }
      if( key == "Perm" )
      {
         int permvalue = get_imcpermvalue( value );

         if( permvalue < 0 || permvalue > IMCPERM_IMP )
         {
            imcbug( "imc_readcommand: Command %s loaded with invalid permission. Set to Imp.", cmd->name.c_str(  ) );
            cmd->level = IMCPERM_IMP;
         }
         else
            cmd->level = permvalue;
      }
      if( key == "Connected" )
         cmd->connected = atoi( value.c_str(  ) );
      if( key == "Alias" )
         cmd->aliaslist.push_back( value );
      if( key == "End" )
         imc_commandlist.push_back( cmd );
   }
   while( !stream.eof(  ) );
   stream.close(  );
   return true;
}

void imc_load_ucache( void )
{
   imc_ucache_data *user;
   ifstream stream;

   stream.open( IMC_UCACHE_FILE );

   imclog( "%s", "Loading ucache data..." );

   if( !stream.is_open(  ) )
   {
      imclog( "%s", "No ucache data found." );
      return;
   }

   imc_ucachelist.clear(  );

   do
   {
      string line, key, value;
      char buf[SMST];

      stream.getline( buf, SMST );
      line = buf;
      if( line.empty(  ) )
         continue;

      istringstream lstream( line );
      lstream >> key >> value;
      strip_lspace( key );
      strip_lspace( value );

      if( key == "#UCACHE" )
         user = new imc_ucache_data;
      if( key == "Name" )
         user->name = value;
      if( key == "Sex" )
         user->gender = atoi( value.c_str(  ) );
      if( key == "Time" )
         user->time = atol( value.c_str(  ) );
      if( key == "End" )
         imc_ucachelist.push_back( user );
   }
   while( !stream.eof(  ) );
   stream.close(  );
   return;
}

void imc_save_config( void )
{
   ofstream stream;

   stream.open( IMC_CONFIG_FILE );
   if( !stream.is_open(  ) )
   {
      imclog( "%s", "Couldn't write to config file." );
      return;
   }

   stream << "# " << this_imcmud->versionid << " configuration file." << endl;
   stream << "# This file can now support the use of tildes in your strings." << endl;
   stream << "# This information can be edited online using the 'imcconfig' command." << endl;
   stream << "LocalName      " << this_imcmud->localname << endl;
   stream << "Autoconnect    " << this_imcmud->autoconnect << endl;
   stream << "MinPlayerLevel " << this_imcmud->minlevel << endl;
   stream << "MinImmLevel    " << this_imcmud->immlevel << endl;
   stream << "AdminLevel     " << this_imcmud->adminlevel << endl;
   stream << "Implevel       " << this_imcmud->implevel << endl;
   stream << "InfoName       " << this_imcmud->fullname << endl;
   stream << "InfoHost       " << this_imcmud->ihost << endl;
   stream << "InfoPort       " << this_imcmud->iport << endl;
   stream << "InfoEmail      " << this_imcmud->email << endl;
   stream << "InfoWWW        " << this_imcmud->www << endl;
   stream << "InfoDetails    " << this_imcmud->details << endl;
   stream << "\n# Your server connection information goes here." << endl;
   stream << "# This information should be available from the network you plan to join." << endl;
   stream << "ServerAddr     " << this_imcmud->rhost << endl;
   stream << "ServerPort     " << this_imcmud->rport << endl;
   stream << "ClientPwd      " << this_imcmud->clientpw << endl;
   stream << "ServerPwd      " << this_imcmud->serverpw << endl;
   stream << "#SHA256 auth: 0 = disabled, 1 = enabled." << endl;
   stream << "SHA256         " << this_imcmud->sha256 << endl;

   if( this_imcmud->sha256pass )
   {
      stream << "#Your server is expecting SHA-256 authentication now. Do not remove this line unless told to do so." << endl;
      stream << "SHA256Pwd   " << this_imcmud->sha256pass << endl;
   }

/*
   imc2_settings_t::iterator it=settingsMap.begin();
	while(it != settingsMap.end()) {

		if( (it->first)[0]=='$')
			stream << it->second << std::endl;
		else
			stream << it->first << "\t\t" << it->second << std::endl;
		++it;
	}
*/
   stream.close(  );
   return;
}

bool imc_load_config( int desc )
{
   ifstream stream;
   ostringstream lib_buf;

   if( this_imcmud != NULL )
      imc_delete_info(  );
   this_imcmud = NULL;

   stream.open( IMC_CONFIG_FILE );

   if( !stream.is_open(  ) )
      return false;

   imclog( "%s", "Loading IMC2 network data..." );

   this_imcmud = new imc_siteinfo;
   init_memory( &this_imcmud->iport, &this_imcmud->state, sizeof( this_imcmud->state ) );

   /*
    * If someone can think of better default values, I'm all ears. Until then, keep your bitching to yourselves. 
    */
   this_imcmud->minlevel = 10;
   this_imcmud->immlevel = 101;
   this_imcmud->adminlevel = 113;
   this_imcmud->implevel = 115;
   this_imcmud->network = "Unknown";
   this_imcmud->sha256 = true;
   this_imcmud->sha256pass = false;
   this_imcmud->desc = desc;
   this_imcmud->inbuf[0] = '\0';
   this_imcmud->outsize = 1000;
   CREATE( this_imcmud->outbuf, char, this_imcmud->outsize );

   do
   {
      string line, key, value;
      char buf[LGST];

      stream.getline( buf, SMST );
      line = buf;
      if( line[0] == '#' || line.empty(  ) )
         continue;

      if( line.find( "InfoDetails" ) != string::npos )
      {
         vector < string > arg = vector_argument( line, 1 );
         if( arg.size(  ) < 2 )
            continue;
         key = arg[0];
         value = arg[1];
      }
      else if( line.find( "InfoName" ) != string::npos )
      {
         vector < string > arg = vector_argument( line, 1 );
         if( arg.size(  ) < 2 )
            continue;
         key = arg[0];
         value = arg[1];
      }
      else
      {
         istringstream lstream( line );
         lstream >> key >> value;
      }
      strip_lspace( key );
      strip_lspace( value );

      // Oh, this is ugly as... well... yeah. Surely there must be a better way? - Samson
      if( key == "LocalName" )
         this_imcmud->localname = value;
      else if( key == "Autoconnect" )
         this_imcmud->autoconnect = atoi( value.c_str(  ) );
      else if( key == "MinPlayerLevel" )
         this_imcmud->minlevel = atoi( value.c_str(  ) );
      else if( key == "MinImmLevel" )
         this_imcmud->immlevel = atoi( value.c_str(  ) );
      else if( key == "AdminLevel" )
         this_imcmud->adminlevel = atoi( value.c_str(  ) );
      else if( key == "Implevel" )
         this_imcmud->implevel = atoi( value.c_str(  ) );
      else if( key == "InfoName" )
         this_imcmud->fullname = value;
      else if( key == "InfoHost" )
         this_imcmud->ihost = value;
      else if( key == "InfoPort" )
         this_imcmud->iport = atoi( value.c_str(  ) );
      else if( key == "InfoEmail" )
         this_imcmud->email = value;
      else if( key == "InfoWWW" )
         this_imcmud->www = value;
      else if( key == "InfoDetails" )
         this_imcmud->details = value;
      else if( key == "RouterAddr" || key == "ServerAddr" )
         this_imcmud->rhost = value;
      else if( key == "RouterPort" || key == "ServerPort" )
         this_imcmud->rport = atoi( value.c_str(  ) );
      else if( key == "ClientPwd" )
         this_imcmud->clientpw = value;
      else if( key == "ServerPwd" )
         this_imcmud->serverpw = value;
      else if( key == "SHA256" )
         this_imcmud->sha256 = atoi( value.c_str(  ) );
      else if( key == "SHA256Pwd" )
         this_imcmud->sha256pass = atoi( value.c_str(  ) );
      else
         imclog( "Unknown key %s with value %s", key.c_str(  ), value.c_str(  ) );
   }
   while( !stream.eof(  ) );
   stream.close(  );

   if( !this_imcmud )
   {
      imclog( "%s", "imc_load_config: No server connection information!!" );
      imclog( "%s", "Network configuration aborted." );
      return false;
   }

   if( this_imcmud->rhost.empty(  ) || this_imcmud->clientpw.empty(  ) || this_imcmud->serverpw.empty(  ) )
   {
      imclog( "%s", "imc_load_config: Missing required configuration info." );
      imclog( "%s", "Network configuration aborted." );
      return false;
   }

   if( this_imcmud->localname.empty(  ) )
   {
      imclog( "%s", "imc_load_config: Mud name not loaded in configuration file." );
      imclog( "%s", "Network configuration aborted." );
      return false;
   }

   if( this_imcmud->fullname.empty(  ) )
   {
      imclog( "%s", "imc_load_config: Missing InfoName parameter in configuration file." );
      imclog( "%s", "Network configuration aborted." );
      return false;
   }

   if( this_imcmud->ihost.empty(  ) )
   {
      imclog( "%s", "imc_load_config: Missing InfoHost parameter in configuration file." );
      imclog( "%s", "Network configuration aborted." );
      return false;
   }

   if( this_imcmud->email.empty(  ) )
   {
      imclog( "%s", "imc_load_config: Missing InfoEmail parameter in configuration file." );
      imclog( "%s", "Network configuration aborted." );
      return false;
   }

   if( this_imcmud->www.empty(  ) )
      this_imcmud->www = "Not specified";

   if( this_imcmud->details.empty(  ) )
      this_imcmud->details = "No details provided.";

   lib_buf << IMC_VERSION_STRING << CODENAME << " " << CODEVERSION;
   this_imcmud->versionid = lib_buf.str(  );

   return true;
}

string parse_who_header( string head )
{
   string::size_type iToken = 0;
   ostringstream iport;

   iport << this_imcmud->iport;

   while( ( iToken = head.find( "<%mudfullname%>" ) ) != string::npos )
     head = head.replace( iToken, 15, this_imcmud->fullname );
   while( ( iToken = head.find( "<%mudtelnet%>" ) ) != string::npos )
     head = head.replace( iToken, 13, this_imcmud->ihost );
   while( ( iToken = head.find( "<%mudport%>" ) ) != string::npos )
     head = head.replace( iToken, 11, iport.str() );
   while( ( iToken = head.find( "<%mudurl%>" ) ) != string::npos )
     head = head.replace( iToken, 10, this_imcmud->www );
   return head;
}

string parse_who_tail( string tail )
{
   string::size_type iToken = 0;
   ostringstream iport;

   iport << this_imcmud->iport;

   while( ( iToken = tail.find( "<%mudfullname%>" ) ) != string::npos )
     tail = tail.replace( iToken, 15, this_imcmud->fullname );
   while( ( iToken = tail.find( "<%mudtelnet%>" ) ) != string::npos )
     tail = tail.replace( iToken, 13, this_imcmud->ihost );
   while( ( iToken = tail.find( "<%mudport%>" ) ) != string::npos )
     tail = tail.replace( iToken, 11, iport.str() );
   while( ( iToken = tail.find( "<%mudurl%>" ) ) != string::npos )
     tail = tail.replace( iToken, 10, this_imcmud->www );

   return tail;
}

void imc_load_who_template( void )
{
   ifstream stream;

   stream.open( IMC_WHO_FILE );

   if( !stream.is_open(  ) )
   {
      imclog( "%s: Unable to load template file for imcwho", __FUNCTION__ );
      whot = NULL;
      return;
   }

   if( whot )
      deleteptr( whot );
   whot = new who_template;

   do
   {
      string key, value;
      char buf[LGST];

      stream >> key;
      strip_lspace( key );

      if( key == "Head:" )
      {
         stream.getline( buf, LGST, '¢' );
         value = buf;
         strip_lspace( value );
         whot->head = parse_who_header( value );
      }
      else if( key == "Tail:" )
      {
         stream.getline( buf, LGST, '¢' );
         value = buf;
         strip_lspace( value );
         whot->tail = parse_who_tail( value );
      }
      else if( key == "Plrline:" )
      {
         stream.getline( buf, LGST, '¢' );
         value = buf;
         strip_lspace( value );
         whot->plrline = value;
      }
      else if( key == "Immline:" )
      {
         stream.getline( buf, LGST, '¢' );
         value = buf;
         strip_lspace( value );
         whot->immline = value;
      }
      else if( key == "Immheader:" )
      {
         stream.getline( buf, LGST, '¢' );
         value = buf;
         strip_lspace( value );
         whot->immheader = value;
      }
      else if( key == "Plrheader:" )
      {
         stream.getline( buf, LGST, '¢' );
         value = buf;
         strip_lspace( value );
         whot->plrheader = value;
      }
      else if( key == "Master:" )
      {
         stream.getline( buf, LGST, '¢' );
         value = buf;
         strip_lspace( value );
         whot->master = value;
      }
   }
   while( !stream.eof(  ) );
   stream.close(  );
}

void imc_load_templates( void )
{
   imc_load_who_template();
}

int ipv4_connect( void )
{
   struct sockaddr_in sa;
   struct hostent *hostp;
   int desc = -1;
#if defined(WIN32)
   ULONG r;
#else
   int r;
#endif

   memset( &sa, 0, sizeof( sa ) );
   sa.sin_family = AF_INET;

   /*
    * warning: this blocks. It would be better to farm the query out to
    * * another process, but that is difficult to do without lots of changes
    * * to the core mud code. You may want to change this code if you have an
    * * existing resolver process running.
    */
#if !defined(WIN32)
   if( !inet_aton( this_imcmud->rhost.c_str(  ), &sa.sin_addr ) )
   {
      hostp = gethostbyname( this_imcmud->rhost.c_str(  ) );
      if( !hostp )
      {
         imclog( "%s: Cannot resolve server hostname.", __FUNCTION__ );
         imc_shutdown( false );
         return -1;
      }
      memcpy( &sa.sin_addr, hostp->h_addr, hostp->h_length );
   }
#else
   sa.sin_addr.s_addr = inet_addr(this_imcmud->rhost.c_str());
#endif

   sa.sin_port = htons( this_imcmud->rport );

   desc = socket( AF_INET, SOCK_STREAM, 0 );
   if( desc < 0 )
   {
      perror( "socket" );
      return -1;
   }

#if defined(WIN32)
   r = 1;
   if( ioctlsocket( desc, FIONBIO, &r ) == SOCKET_ERROR )
   {
      perror( "imc_connect: ioctlsocket failed" );
      close( desc );
      return -1;
   }
#else
   r = fcntl( desc, F_GETFL, 0 );
   if( r < 0 || fcntl( desc, F_SETFL, O_NONBLOCK | r ) < 0 )
   {
      perror( "imc_connect: fcntl" );
      close( desc );
      return -1;
   }
#endif

   if( connect( desc, ( struct sockaddr * )&sa, sizeof( sa ) ) == -1 )
   {
      if( errno != EINPROGRESS )
      {
         perror( "connect" );
         close( desc );
         return -1;
      }
   }
   return desc;
}

bool imc_server_connect( void )
{
#if defined(IPV6)
   struct addrinfo hints, *ai_list, *ai;
   char rport[SMST];
   int n, r;
#endif
   ostringstream buf;
   int desc = 0;

   if( !this_imcmud )
   {
      imcbug( "%s", "No connection data loaded" );
      return false;
   }

   if( this_imcmud->state != IMC_AUTH1 )
   {
      imcbug( "%s", "Connection is not in proper state." );
      return false;
   }

   if( this_imcmud->desc > 0 )
   {
      imcbug( "%s", "Already connected" );
      return false;
   }

#if defined(IPV6)
   snprintf( rport, SMST, "%hu", this_imcmud->rport );
   memset( &hints, 0, sizeof( struct addrinfo ) );
   hints.ai_family = AF_UNSPEC;
   hints.ai_socktype = SOCK_STREAM;
   hints.ai_protocol = IPPROTO_TCP;
   n = getaddrinfo( this_imcmud->rhost.c_str(  ), rport, &hints, &ai_list );

   if( n )
   {
      imclog( "%s: getaddrinfo: %s", __FUNCTION__, gai_strerror( n ) );
      return false;
   }

   for( ai = ai_list; ai; ai = ai->ai_next )
   {
      desc = socket( ai->ai_family, ai->ai_socktype, ai->ai_protocol );
      if( desc < 0 )
         continue;

      if( connect( desc, ai->ai_addr, ai->ai_addrlen ) == 0 )
         break;
      close( desc );
   }
   freeaddrinfo( ai_list );
   if( ai == NULL )
   {
      imclog( "%s: socket or connect: failed for %s port %hu", __FUNCTION__, this_imcmud->rhost.c_str(  ),
              this_imcmud->rport );
      imcwait = 100; // So it will try again according to the reconnect count.
      return false;
   }

   r = fcntl( desc, F_GETFL, 0 );
   if( r < 0 || fcntl( desc, F_SETFL, O_NONBLOCK | r ) < 0 )
   {
      perror( "imc_connect: fcntl" );
      close( desc );
      return false;
   }
#else
   desc = ipv4_connect(  );
   if( desc < 1 )
      return false;
#endif

   imclog( "%s", "Connecting to server." );

   this_imcmud->state = IMC_AUTH2;
   this_imcmud->desc = desc;

   /*
    * The MUD is electing to enable SHA-256 - this is the default setting 
    */
   if( this_imcmud->sha256 )
   {
      /*
       * No SHA-256 setup enabled.
       * * Situations where this might happen:
       * *
       * * 1. You are connecting for the first time. This is expected.
       * * 2. You are connecting to an older server which does not support it, so you will continue connecting this way.
       * * 3. You got stupid and deleted the SHA-256 line in your config file after it got there. Ooops.
       * * 4. The server lost your data. In which case you'll need to do #3 because authentication will fail.
       * * 5. You let your connection lapse, and #4 happened because of it.
       * * 6. Gremlins. When in doubt, blame them.
       */
      if( !this_imcmud->sha256pass )
         buf << "PW " << this_imcmud->localname << " " << this_imcmud->
            clientpw << " version=" << IMC_VERSION << " autosetup " << this_imcmud->serverpw << " SHA256";

      /*
       * You have SHA-256 working. Excellent. Lets send the new packet for it.
       * * Situations where this will fail:
       * *
       * * 1. You're a new connection, and for whatever dumb reason, the SHA-256 line is in your config already.
       * * 2. You have SHA-256 enabled and you're switching to a new server. This is generally not going to work well.
       * * 3. Something happened and the hashing failed. Resulting in authentication failure. Ooops.
       * * 4. The server lost your connection data.
       * * 5. You let your connection lapse, and #4 happened because of it.
       * * 6. Gremlins. When in doubt, blame them.
       */
      else
         buf << "SHA256-AUTH-REQ " << this_imcmud->localname;
   }
   /*
    * The MUD is electing not to use SHA-256 for whatever reason - this must be specifically set 
    */
   else
      buf << "PW " << this_imcmud->localname << " " << this_imcmud->
         clientpw << " version=" << IMC_VERSION << " autosetup " << this_imcmud->serverpw;

   imc_write_buffer( buf.str(  ) );
   return true;
}

void imc_delete_templates( void )
{
   deleteptr( whot );
}

void free_imcdata( bool complete )
{
   list<imc_channel*>::iterator chn;
   for( chn = imc_chanlist.begin(  ); chn != imc_chanlist.end(  ); )
   {
      imc_channel *c = (*chn);
      ++chn;

      imc_freechan( c );
   }
   imc_chanlist.clear(  );

   list<imc_remoteinfo*>::iterator rin;
   for( rin = imc_reminfolist.begin(  ); rin != imc_reminfolist.end(  ); )
   {
      imc_remoteinfo *r = (*rin);
      ++rin;

      deleteptr( r );
   }
   imc_reminfolist.clear(  );

   imc_banlist.clear(  );

   list<imc_ucache_data*>::iterator uch;
   for( uch = imc_ucachelist.begin(  ); uch != imc_ucachelist.end(  ); )
   {
      imc_ucache_data *ucache = (*uch);
      ++uch;

      deleteptr( ucache );
   }
   imc_ucachelist.clear(  );

   /*
    * This stuff is only killed off if the mud itself shuts down. For those of you Valgrinders out there. 
    */
   if( complete )
   {
      imc_delete_templates();

      list<imc_command_table*>::iterator com;
      for( com = imc_commandlist.begin(  ); com != imc_commandlist.end(  ); )
      {
         imc_command_table *cmd = (*com);
         ++com;

         cmd->aliaslist.clear(  );
         deleteptr( cmd );
      }
      imc_commandlist.clear(  );

      list<imc_help_table*>::iterator hlp;
      for( hlp = imc_helplist.begin(  ); hlp != imc_helplist.end(  ); )
      {
         imc_help_table *help = (*hlp);
         ++hlp;

         deleteptr( help );
      }
      imc_helplist.clear(  );

      color_imcmap.clear(  );
      color_mudmap.clear(  );
      phandler.clear();
   }
   return;
}

void imc_hotboot( void )
{
   ofstream stream;

   if( this_imcmud && this_imcmud->state == IMC_ONLINE )
   {
      stream.open( IMC_HOTBOOT_FILE );
      if( !stream.is_open(  ) )
         imcbug( "%s: Unable to open IMC hotboot file for write.", __FUNCTION__ );
      else
      {
         stream << ( !this_imcmud->network.empty(  )? this_imcmud->network : "Unknown" ) << " " << ( !this_imcmud->
                                                                                                     servername.
                                                                                                     empty(  )? this_imcmud->
                                                                                                     servername : "Unknown" )
            << endl;
         stream.close(  );
         imc_savehistory(  );
      }
   }
}

/* Shutdown IMC2 */
void imc_shutdown( bool reconnect )
{
   if( this_imcmud && this_imcmud->state == IMC_OFFLINE )
      return;

   imclog( "%s", "Shutting down network." );

   if( this_imcmud->desc > 0 )
      close( this_imcmud->desc );
   this_imcmud->desc = -1;

   imc_savehistory(  );
   free_imcdata( false );

   this_imcmud->state = IMC_OFFLINE;
   cancel_event( ev_imcweb_refresh, NULL );
   if( reconnect )
   {
      imcwait = 100; /* About 20 seconds or so */
      imclog( "%s", "Connection to server was lost. Reconnecting in approximately 20 seconds." );
   }
}

/* Startup IMC2 */
bool imc_startup_network( bool connected )
{
   imclog( "%s", "IMC2 Network Initializing..." );

   if( connected )
   {
      ifstream stream;

      stream.open( IMC_HOTBOOT_FILE );
      if( !stream.is_open(  ) )
         imcbug( "%s: Unable to load IMC hotboot file.", __FUNCTION__ );
      else
      {
         stream >> this_imcmud->network >> this_imcmud->servername;
         stream.close(  );
         unlink( IMC_HOTBOOT_FILE );
      }
      this_imcmud->state = IMC_ONLINE;
      this_imcmud->inbuf[0] = '\0';
      this_imcmud->outsize = IMC_BUFF_SIZE;
      CREATE( this_imcmud->outbuf, char, this_imcmud->outsize );
      imc_request_keepalive(  );
      imc_firstrefresh(  );
      add_event( 60, ev_imcweb_refresh, NULL );
      return true;
   }

   this_imcmud->state = IMC_AUTH1;

   /*
    * Connect to server 
    */
   if( !imc_server_connect(  ) )
   {
      this_imcmud->state = IMC_OFFLINE;
      return false;
   }
   add_event( 60, ev_imcweb_refresh, NULL );
   return true;
}

void imc_startup( bool force, int desc, bool connected )
{
   imcwait = 0;

   if( this_imcmud && this_imcmud->state > IMC_OFFLINE )
   {
      imclog( "%s: Network startup called when already engaged!", __FUNCTION__ );
      return;
   }

   imc_time = time( NULL );
   imc_sequencenumber = imc_time;

   /*
    * The Command table is required for operation. Like.... duh? 
    */
   if( imc_commandlist.empty(  ) )
   {
      if( !imc_load_commands(  ) )
      {
         imcbug( "%s: Unable to load command table!", __FUNCTION__ );
         return;
      }
   }

   /*
    * Configuration required for network operation. 
    */
   if( !imc_load_config( desc ) )
      return;

   /*
    * Lets register all the default packet handlers 
    */
   imc_register_default_packets(  );

   /*
    * Help information should persist even when the network is not connected... 
    */
   if( imc_helplist.empty(  ) )
      imc_load_helps(  );

   /*
    * ... as should the color table. 
    */
   if( color_imcmap.empty(  ) || color_mudmap.empty(  ) )
      imc_load_color_table(  );

   /*
    * ... and the templates. Checks for whot being defined, but the others are loaded here to, so....
    */
   if( !whot )
      imc_load_templates();

   if( ( !this_imcmud->autoconnect && !force && !connected ) || ( connected && this_imcmud->desc < 1 ) )
   {
      imclog( "%s", "IMC2 network data loaded. Autoconnect not set. IMC2 will need to be connected manually." );
      return;
   }
   else
      imclog( "%s", "IMC2 network data loaded." );

   if( this_imcmud->autoconnect || force || connected )
   {
      if( imc_startup_network( connected ) )
      {
         imc_loadchannels(  );
         imc_loadhistory(  );
         imc_readbans(  );
         imc_load_ucache(  );
         return;
      }
   }
   return;
}

/*****************************************
 * User level commands and social hooks. *
 *****************************************/

/* The imccommand command, aka icommand. Channel manipulation at the server level etc. */
IMC_CMD( imccommand )
{
   vector < string > arg = vector_argument( argument, 2 );
   ostringstream to;
   imc_packet *p;
   imc_channel *c;

   if( arg.size(  ) < 2 )
   {
      imc_to_char( "~wSyntax: imccommand <command> <server:channel> [<data..>]\r\n", ch );
      imc_to_char( "~wCommand access will depend on your privledges and what each individual server allows.\r\n", ch );
      return;
   }

   if( !( c = imc_findchannel( arg[1] ) ) && !scomp( arg[0], "create" ) )
   {
      imc_printf( ch, "There is no channel called %s known.\r\n", arg[1].c_str(  ) );
      return;
   }

   to << "IMC@" << ( c ? imc_channel_mudof( c->chname ) : imc_channel_mudof( arg[1] ) );
   p = new imc_packet( CH_IMCNAME( ch ), "ice-cmd", to.str(  ) );
   p->data << "channel=" << ( c ? c->chname : arg[1] );
   p->data << " command=" << arg[0];
   if( arg.size(  ) > 2 )
      p->data << " data=" << arg[2];
   p->send(  );

   imc_to_char( "Command sent.\r\n", ch );
   return;
}

/* need exactly 2 %s's, and no other format specifiers */
bool verify_format( const char *fmt, short sneed )
{
   const char *c;
   int i = 0;

   c = fmt;
   while( ( c = strchr( c, '%' ) ) != NULL )
   {
      if( *( c + 1 ) == '%' ) /* %% */
      {
         c += 2;
         continue;
      }

      if( *( c + 1 ) != 's' ) /* not %s */
         return false;

      ++c;
      ++i;
   }
   if( i != sneed )
      return false;

   return true;
}

/* The imcsetup command, channel manipulation at the mud level etc, formatting and the like.
 * This command will not do "localization" since this is handled automatically now by ice-update packets.
 */
IMC_CMD( imcsetup )
{
   vector < string > arg = vector_argument( argument, 3 );
   imc_channel *c = NULL;
   bool all = false;

   if( arg.size(  ) < 2 )
   {
      imc_to_char( "~wSyntax: imcsetup <command> <channel> [<data..>]\r\n", ch );
      imc_to_char( "~wWhere 'command' is one of the following:\r\n", ch );
      imc_to_char( "~wdelete rename perm regformat emoteformat socformat\r\n\r\n", ch );
      imc_to_char( "~wWhere 'channel' is one of the following:\r\n", ch );

      list<imc_channel*>::iterator ichn;
      for( ichn = imc_chanlist.begin(  ); ichn != imc_chanlist.end(  ); ++ichn )
      {
         c = (*ichn);

         if( !c->local_name.empty(  ) )
            imc_printf( ch, "~w%s ", c->local_name.c_str(  ) );
         else
            imc_printf( ch, "~w%s ", c->chname.c_str(  ) );
      }
      imc_to_char( "\r\n", ch );
      return;
   }

   if( scomp( arg[1], "all" ) )
      all = true;
   else
   {
      if( !( c = imc_findchannel( arg[1] ) ) )
      {
         imc_to_char( "Unknown channel.\r\n", ch );
         return;
      }
   }

   /*
    * Permission check -- Xorith 
    */
   if( c && c->level > IMCPERM( ch ) )
   {
      imc_to_char( "You cannot modify that channel.", ch );
      return;
   }

   if( scomp( arg[0], "delete" ) )
   {
      if( all )
      {
         imc_to_char( "You cannot perform a delete all on channels.\r\n", ch );
         return;
      }

      imc_to_char( "Channel is no longer locally configured.\r\n", ch );

      imc_chanlist.remove( c );
      imc_freechan( c );
      imc_save_channels(  );
      return;
   }

   if( scomp( arg[0], "rename" ) )
   {
      if( all )
      {
         imc_to_char( "You cannot perform a rename all on channels.\r\n", ch );
         return;
      }

      if( arg.size(  ) < 3 )
      {
         imc_to_char( "~wMissing 'newname' argument for 'imcsetup rename'\r\n", ch );  /* Lets be more kind! -- X */
         imc_to_char( "~wSyntax: imcsetup rename <local channel> <newname>\r\n", ch ); /* Fixed syntax message -- X */
         return;
      }

      if( imc_findchannel( arg[2] ) )
      {
         imc_to_char( "New channel name already exists.\r\n", ch );
         return;
      }

      /*
       * Small change here to give better feedback to the ch -- Xorith 
       */
      imc_printf( ch, "Renamed channel '%s' to '%s'.\r\n", c->local_name.c_str(  ), arg[2].c_str(  ) );
      c->local_name = arg[2];

      /*
       * Reset the format with the new local name 
       */
      imcformat_channel( ch, c, 4, false );
      imc_save_channels(  );
      return;
   }

   if( scomp( arg[0], "resetformats" ) )
   {
      if( all )
      {
         imcformat_channel( ch, NULL, 4, true );
         imc_to_char( "All channel formats have been reset to default.\r\n", ch );
      }
      else
      {
         imcformat_channel( ch, c, 4, false );
         imc_to_char( "The formats for this channel have been reset to default.\r\n", ch );
      }
      return;
   }

   if( scomp( arg[0], "regformat" ) )
   {
      if( arg.size(  ) < 3 )
      {
         imc_to_char( "Syntax: imcsetup regformat <localchannel|all> <string>\r\n", ch ); /* Syntax Fix -- Xorith */
         return;
      }

      if( !verify_format( arg[2].c_str(  ), 2 ) )
      {
         imc_to_char( "Bad format - must contain exactly 2 %s's.\r\n", ch );
         return;
      }

      if( all )
      {
         list<imc_channel*>::iterator ichn;
         for( ichn = imc_chanlist.begin(  ); ichn != imc_chanlist.end(  ); ++ichn )
         {
            imc_channel *chn = (*ichn);
            chn->regformat = arg[2];
         }
         imc_to_char( "All channel regular formats have been updated.\r\n", ch );
      }
      else
      {
         c->regformat = arg[2];
         imc_to_char( "The regular format for this channel has been changed successfully.\r\n", ch );
      }
      imc_save_channels(  );
      return;
   }

   if( scomp( arg[0], "emoteformat" ) )
   {
      if( arg.size(  ) < 3 )
      {
         imc_to_char( "Syntax: imcsetup emoteformat <localchannel|all> <string>\r\n", ch );  /* Syntax Fix -- Xorith */
         return;
      }

      if( !verify_format( arg[2].c_str(  ), 2 ) )
      {
         imc_to_char( "Bad format - must contain exactly 2 %s's.\r\n", ch );
         return;
      }

      if( all )
      {
         list<imc_channel*>::iterator ichn;
         for( ichn = imc_chanlist.begin(  ); ichn != imc_chanlist.end(  ); ++ichn )
         {
            imc_channel *chn = (*ichn);
            chn->emoteformat = arg[2];
         }
         imc_to_char( "All channel emote formats have been updated.\r\n", ch );
      }
      else
      {
         c->emoteformat = arg[2];
         imc_to_char( "The emote format for this channel has been changed successfully.\r\n", ch );
      }
      imc_save_channels(  );
      return;
   }

   if( scomp( arg[0], "socformat" ) )
   {
      if( arg.size(  ) < 3 )
      {
         imc_to_char( "Syntax: imcsetup socformat <localchannel|all> <string>\r\n", ch ); /* Xorith */
         return;
      }

      if( !verify_format( arg[2].c_str(  ), 1 ) )
      {
         imc_to_char( "Bad format - must contain exactly 1 %s.\r\n", ch );
         return;
      }

      if( all )
      {
         list<imc_channel*>::iterator ichn;
         for( ichn = imc_chanlist.begin(  ); ichn != imc_chanlist.end(  ); ++ichn )
         {
            imc_channel *chn = (*ichn);
            chn->socformat = arg[2];
         }
         imc_to_char( "All channel social formats have been updated.\r\n", ch );
      }
      else
      {
         c->socformat = arg[2];
         imc_to_char( "The social format for this channel has been changed successfully.\r\n", ch );
      }
      imc_save_channels(  );
      return;
   }

   if( scomp( arg[0], "perm" ) || scomp( arg[0], "permission" ) || scomp( arg[0], "level" ) )
   {
      int permvalue = -1;

      if( all )
      {
         imc_to_char( "You cannot do a permissions all for channels.\r\n", ch );
         return;
      }

      if( arg.size(  ) < 3 )
      {
         imc_to_char( "Syntax: imcsetup perm <localchannel> <permission>\r\n", ch );
         return;
      }

      permvalue = get_imcpermvalue( arg[2] );
      if( permvalue < 0 || permvalue > IMCPERM_IMP )
      {
         imc_to_char( "Unacceptable permission setting.\r\n", ch );
         return;
      }

      /*
       * Added permission checking here -- Xorith 
       */
      if( permvalue > IMCPERM( ch ) )
      {
         imc_to_char( "You cannot set a permission higher than your own.\r\n", ch );
         return;
      }

      c->level = permvalue;

      imc_to_char( "Channel permissions changed.\r\n", ch );
      imc_save_channels(  );
      return;
   }
   imcsetup( ch, "" );
   return;
}

/* The imcchanlist command. Basic listing of channels. */
IMC_CMD( imcchanlist )
{
   imc_channel *c = NULL;
   int count = 0; /* Count -- Xorith */
   char col = 'C';   /* Listening Color -- Xorith */

   if( imc_chanlist.empty(  ) )
   {
      imc_to_char( "~WThere are no known channels on this network.\r\n", ch );
      return;
   }

   if( !argument.empty(  ) )
   {
      if( !( c = imc_findchannel( argument ) ) )
      {
         imc_printf( ch, "There is no channel called %s here.\r\n", argument.c_str(  ) );
         return;
      }
   }

   if( c )
   {
      imc_printf( ch, "~WChannel  : %s\r\n\r\n", c->chname.c_str(  ) );
      imc_printf( ch, "~cLocalname: ~w%s\r\n", c->local_name.c_str(  ) );
      imc_printf( ch, "~cPerms    : ~w%s\r\n", imcperm_names[c->level] );
      imc_printf( ch, "~cPolicy   : %s\r\n", c->open ? "~gOpen" : "~yPrivate" );
      imc_printf( ch, "~cRegFormat: ~w%s\r\n", c->regformat.c_str(  ) );
      imc_printf( ch, "~cEmoFormat: ~w%s\r\n", c->emoteformat.c_str(  ) );
      imc_printf( ch, "~cSocFormat: ~w%s\r\n\r\n", c->socformat.c_str(  ) );
      imc_printf( ch, "~cOwner    : ~w%s\r\n", c->owner.c_str(  ) );
      imc_printf( ch, "~cOperators: ~w%s\r\n", c->operators.c_str(  ) );
      imc_printf( ch, "~cInvite   : ~w%s\r\n", c->invited.c_str(  ) );
      imc_printf( ch, "~cExclude  : ~w%s\r\n", c->excluded.c_str(  ) );
      return;
   }

   imc_printf( ch, "~c%-15s ~C%-15s ~B%-15s ~b%-7s ~!%s\r\n\r\n", "Name", "Local name", "Owner", "Perm", "Policy" );
   list<imc_channel*>::iterator ichn;
   for( ichn = imc_chanlist.begin(  ); ichn != imc_chanlist.end(  ); ++ichn )
   {
      imc_channel *chn = (*ichn);

      if( IMCPERM( ch ) < chn->level )
         continue;

      /*
       * If it's locally configured and we're not listening, then color it red -- Xorith 
       */
      if( !chn->local_name.empty(  ) )
      {
         if( !imc_hasname( IMC_LISTEN( ch ), chn->local_name ) )
            col = 'R';
         else
            col = 'C';  /* Otherwise, keep it Cyan -- X */
      }

      imc_printf( ch, "~c%-15.15s ~%c%-*.*s ~B%-15.15s ~b%-7s %s\r\n", chn->chname.c_str(  ), col,
                  !chn->local_name.empty(  )? 15 : 17, !chn->local_name.empty(  )? 15 : 17,
                  !chn->local_name.empty(  )? chn->local_name.c_str(  ) : "~Y(not local)  ",
                  chn->owner.c_str(  ), imcperm_names[chn->level],
                  chn->refreshed ? ( chn->open ? "~gOpen" : "~yPrivate" ) : "~Runknown" );
      ++count; /* Keep a count -- Xorith */
   }
   /*
    * Show the count and a bit of text explaining the red color -- Xorith 
    */
   imc_printf( ch, "\r\n~W%d ~cchannels found.", count );
   imc_to_char( "\r\n~RRed ~clocal name indicates a channel not being listened to.\r\n", ch );
   return;
}

IMC_CMD( imclisten )
{
   if( argument.empty(  ) )
   {
      imc_to_char( "~cCurrently tuned into:\r\n", ch );
      if( !IMC_LISTEN( ch ).empty(  ) )
         imc_printf( ch, "~W%s", IMC_LISTEN( ch ).c_str(  ) );
      else
         imc_to_char( "~WNone", ch );
      imc_to_char( "\r\n", ch );
      return;
   }

   if( scomp( argument, "all" ) )
   {
      list<imc_channel*>::iterator ichn;
      for( ichn = imc_chanlist.begin(  ); ichn != imc_chanlist.end(  ); ++ichn )
      {
         imc_channel *chn = (*ichn);

         if( chn->local_name.empty(  ) )
            continue;

         if( IMCPERM( ch ) >= chn->level && !imc_hasname( IMC_LISTEN( ch ), chn->local_name ) )
         {
            imc_addname( IMC_LISTEN( ch ), chn->local_name );
            imc_sendnotify( ch, chn->local_name, true );
         }
      }
      imc_to_char( "~YYou are now listening to all available IMC2 channels.\r\n", ch );
      return;
   }

   if( scomp( argument, "none" ) )
   {
      list<imc_channel*>::iterator ichn;
      for( ichn = imc_chanlist.begin(  ); ichn != imc_chanlist.end(  ); ++ichn )
      {
         imc_channel *chn = (*ichn);

         if( chn->local_name.empty(  ) )
            continue;

         if( imc_hasname( IMC_LISTEN( ch ), chn->local_name ) )
            imc_sendnotify( ch, chn->local_name, false );
      }
      IMC_LISTEN( ch ).clear(  );
      imc_to_char( "~YYou no longer listen to any available IMC2 channels.\r\n", ch );
      return;
   }

   imc_channel *c;
   if( !( c = imc_findchannel( argument ) ) )
   {
      imc_to_char( "No such channel configured locally.\r\n", ch );
      return;
   }

   if( IMCPERM( ch ) < c->level )
   {
      imc_to_char( "No such channel configured locally.\r\n", ch );
      return;
   }

   if( imc_hasname( IMC_LISTEN( ch ), c->local_name ) )
   {
      imc_removename( IMC_LISTEN( ch ), c->local_name );
      imc_to_char( "Channel off.\r\n", ch );
      imc_sendnotify( ch, c->local_name, false );
   }
   else
   {
      imc_addname( IMC_LISTEN( ch ), c->local_name );
      imc_to_char( "Channel on.\r\n", ch );
      imc_sendnotify( ch, c->local_name, true );
   }
}

IMC_CMD( imctell )
{
   vector < string > arg = vector_argument( argument, 1 );
   ostringstream buf1;

   if( IMCIS_SET( IMCFLAG( ch ), IMC_DENYTELL ) )
   {
      imc_to_char( "You are not authorized to use imctell.\r\n", ch );
      return;
   }

   if( arg.size(  ) < 1 )
   {
      int x;

      imc_to_char( "~wUsage: imctell user@mud <message>\r\n", ch );
      imc_to_char( "~wUsage: imctell [on]/[off]\r\n\r\n", ch );
      imc_printf( ch, "~cThe last %d things you were told:\r\n", MAX_IMCTELLHISTORY );

      for( x = 0; x < MAX_IMCTELLHISTORY; ++x )
      {
         if( IMCTELLHISTORY( ch, x ).empty(  ) )
            break;
         imc_to_char( IMCTELLHISTORY( ch, x ), ch );
      }
      return;
   }

   if( scomp( arg[0], "on" ) )
   {
      IMCREMOVE_BIT( IMCFLAG( ch ), IMC_TELL );
      imc_to_char( "You now send and receive imctells.\r\n", ch );
      return;
   }

   if( scomp( arg[0], "off" ) )
   {
      IMCSET_BIT( IMCFLAG( ch ), IMC_TELL );
      imc_to_char( "You no longer send and receive imctells.\r\n", ch );
      return;
   }

   if( IMCIS_SET( IMCFLAG( ch ), IMC_TELL ) )
   {
      imc_to_char( "You have imctells turned off.\r\n", ch );
      return;
   }

   if( IMCISINVIS( ch ) )
   {
      imc_to_char( "You are invisible.\r\n", ch );
      return;
   }

   if( !check_mudof( ch, arg[0] ) )
      return;

   /*
    * Tell socials. Suggested by Darien@Sandstorm 
    */
   if( arg[1][0] == '@' )
   {
      string p, p2, buf2;

      arg[1] = arg[1].substr( 1, arg[1].length(  ) );
      strip_lspace( arg[1] );
      buf2 = arg[1];
      p = imc_send_social( ch, arg[1], 1 );
      if( p.empty(  ) )
         return;

      imc_send_tell( CH_IMCNAME( ch ), arg[0], p, 2 );
      p2 = imc_send_social( ch, buf2, 2 );
      if( p2.empty(  ) )
         return;
      buf1 << "~WImctell ~C" << arg[0] << ": ~c" << p2 << "\r\n";
   }
   else if( arg[1][0] == ',' )
   {
      arg[1] = arg[1].substr( 1, arg[1].length(  ) );
      strip_lspace( arg[1] );
      imc_send_tell( CH_IMCNAME( ch ), arg[0], color_mtoi( arg[1] ), 1 );
      buf1 << "~WImctell: ~c" << arg[0] << " " << arg[1] << "\r\n";
   }
   else
   {
      imc_send_tell( CH_IMCNAME( ch ), arg[0], color_mtoi( arg[1] ), 0 );
      buf1 << "~cYou imctell ~C" << arg[0] << " ~c'~W" << arg[1] << "~c'\r\n";
   }
   imc_to_char( buf1.str(  ), ch );
   imc_update_tellhistory( ch, buf1.str(  ) );
   return;
}

IMC_CMD( imcreply )
{
   ostringstream buf1;

   /*
    * just check for deny 
    */
   if( IMCIS_SET( IMCFLAG( ch ), IMC_DENYTELL ) )
   {
      imc_to_char( "You are not authorized to use imcreply.\r\n", ch );
      return;
   }

   if( IMCIS_SET( IMCFLAG( ch ), IMC_TELL ) )
   {
      imc_to_char( "You have imctells turned off.\r\n", ch );
      return;
   }

   if( IMCISINVIS( ch ) )
   {
      imc_to_char( "You are invisible.\r\n", ch );
      return;
   }

   if( IMC_RREPLY( ch ).empty(  ) )
   {
      imc_to_char( "You haven't received an imctell yet.\r\n", ch );
      return;
   }

   if( argument.empty(  ) )
   {
      imc_to_char( "imcreply what?\r\n", ch );
      return;
   }

   if( !check_mudof( ch, IMC_RREPLY( ch ) ) )
      return;

   /*
    * Tell socials. Suggested by Darien@Sandstorm 
    */
   if( argument[0] == '@' )
   {
      string p, p2, buf2;

      argument = argument.substr( 1, argument.length(  ) );
      strip_lspace( argument );
      buf2 = argument;
      p = imc_send_social( ch, argument, 1 );
      if( p.empty(  ) )
         return;

      imc_send_tell( CH_IMCNAME( ch ), IMC_RREPLY( ch ), p, 2 );
      p2 = imc_send_social( ch, buf2, 2 );
      if( p2.empty(  ) )
         return;
      buf1 << "~WImctell ~C" << IMC_RREPLY( ch ) << ": ~c" << p2 << "\r\n";
   }
   else if( argument[0] == ',' )
   {
      argument = argument.substr( 1, argument.length(  ) );
      strip_lspace( argument );
      imc_send_tell( CH_IMCNAME( ch ), IMC_RREPLY( ch ), color_mtoi( argument ), 1 );
      buf1 << "~WImctell ~c" << IMC_RREPLY( ch ) << " " << argument << "\r\n";
   }
   else
   {
      imc_send_tell( CH_IMCNAME( ch ), IMC_RREPLY( ch ), color_mtoi( argument ), 0 );
      buf1 << "~cYou imctell ~C" << IMC_RREPLY( ch ) << " ~c'~W" << argument << "~c'\r\n";
   }
   imc_to_char( buf1.str(  ), ch );
   imc_update_tellhistory( ch, buf1.str(  ) );
   return;
}

IMC_CMD( imcwho )
{
   if( argument.empty(  ) )
   {
      imc_to_char( "imcwho which mud? See imclist for a list of connected muds.\r\n", ch );
      return;
   }

   /* Now why didn't I think of this before for local who testing?
    * Meant for testing only, so it needs >= Imm perms
    * Otherwise people could use it to bypass wizinvis locally.
    */
   if( scomp( argument, this_imcmud->localname ) && IMCPERM(ch) >= IMCPERM_IMM )
   {
      imc_to_char( imc_assemble_who(), ch );
      return;
   }

   if( !check_mud( ch, argument ) )
      return;

   imc_send_who( CH_IMCNAME( ch ), argument, "who" );
}

IMC_CMD( imclocate )
{
   ostringstream user;

   if( argument.empty(  ) )
   {
      imc_to_char( "imclocate who?\r\n", ch );
      return;
   }

   user << argument << "@*";
   imc_send_whois( CH_IMCNAME( ch ), user.str(  ) );
   return;
}

IMC_CMD( imcfinger )
{
   vector < string > arg = vector_argument( argument, 1 );

   if( IMCIS_SET( IMCFLAG( ch ), IMC_DENYFINGER ) )
   {
      imc_to_char( "You are not authorized to use imcfinger.\r\n", ch );
      return;
   }

   if( arg.size(  ) < 1 )
   {
      imc_to_char( "~wUsage: imcfinger person@mud\r\n", ch );
      imc_to_char( "~wUsage: imcfinger <field> <value>\r\n", ch );
      imc_to_char( "~wWhere field is one of:\r\n\r\n", ch );
      imc_to_char( "~wdisplay email homepage icq aim yahoo msn privacy comment\r\n", ch );
      return;
   }

   if( scomp( arg[0], "display" ) )
   {
      imc_to_char( "~GYour current information:\r\n\r\n", ch );
      imc_printf( ch, "~GEmail   : ~g%s\r\n", !IMC_EMAIL( ch ).empty(  )? IMC_EMAIL( ch ).c_str(  ) : "None" );
      imc_printf( ch, "~GHomepage: ~g%s\r\n", !IMC_HOMEPAGE( ch ).empty(  )? IMC_HOMEPAGE( ch ).c_str(  ) : "None" );
      imc_printf( ch, "~GICQ     : ~g%d\r\n", IMC_ICQ( ch ) );
      imc_printf( ch, "~GAIM     : ~g%s\r\n", !IMC_AIM( ch ).empty(  )? IMC_AIM( ch ).c_str(  ) : "None" );
      imc_printf( ch, "~GYahoo   : ~g%s\r\n", !IMC_YAHOO( ch ).empty(  )? IMC_YAHOO( ch ).c_str(  ) : "None" );
      imc_printf( ch, "~GMSN     : ~g%s\r\n", !IMC_MSN( ch ).empty(  )? IMC_MSN( ch ).c_str(  ) : "None" );
      imc_printf( ch, "~GComment : ~g%s\r\n", !IMC_COMMENT( ch ).empty(  )? IMC_COMMENT( ch ).c_str(  ) : "None" );
      imc_printf( ch, "~GPrivacy : ~g%s\r\n", IMCIS_SET( IMCFLAG( ch ), IMC_PRIVACY ) ? "Enabled" : "Disabled" );
      return;
   }

   if( scomp( arg[0], "privacy" ) )
   {
      if( IMCIS_SET( IMCFLAG( ch ), IMC_PRIVACY ) )
      {
         IMCREMOVE_BIT( IMCFLAG( ch ), IMC_PRIVACY );
         imc_to_char( "Privacy flag removed. Your information will now be visible on imcfinger.\r\n", ch );
      }
      else
      {
         IMCSET_BIT( IMCFLAG( ch ), IMC_PRIVACY );
         imc_to_char( "Privacy flag enabled. Your information will no longer be visible on imcfinger.\r\n", ch );
      }
      return;
   }

   if( arg.size(  ) < 2 )
   {
      ostringstream name;

      if( this_imcmud->state != IMC_ONLINE )
      {
         imc_to_char( "The mud is not currently connected to IMC2.\r\n", ch );
         return;
      }

      if( !check_mudof( ch, arg[0] ) )
         return;

      name << "finger " << imc_nameof( arg[0] );
      imc_send_who( CH_IMCNAME( ch ), imc_mudof( arg[0] ), name.str(  ) );
      return;
   }

   if( scomp( arg[0], "email" ) )
   {
      IMC_EMAIL( ch ) = arg[1];
      imc_printf( ch, "Your email address has changed to: %s\r\n", IMC_EMAIL( ch ).c_str(  ) );
      return;
   }

   if( scomp( arg[0], "homepage" ) )
   {
      IMC_HOMEPAGE( ch ) = arg[1];
      imc_printf( ch, "Your homepage has changed to: %s\r\n", IMC_HOMEPAGE( ch ).c_str(  ) );
      return;
   }

   if( scomp( arg[0], "icq" ) )
   {
      IMC_ICQ( ch ) = atoi( arg[1].c_str(  ) );
      imc_printf( ch, "Your ICQ Number has changed to: %d\r\n", IMC_ICQ( ch ) );
      return;
   }

   if( scomp( arg[0], "aim" ) )
   {
      IMC_AIM( ch ) = arg[1];
      imc_printf( ch, "Your AIM Screenname has changed to: %s\r\n", IMC_AIM( ch ).c_str(  ) );
      return;
   }

   if( scomp( arg[0], "yahoo" ) )
   {
      IMC_YAHOO( ch ) = arg[1];
      imc_printf( ch, "Your Yahoo Screenname has changed to: %s\r\n", IMC_YAHOO( ch ).c_str(  ) );
      return;
   }

   if( scomp( arg[0], "msn" ) )
   {
      IMC_MSN( ch ) = arg[1];
      imc_printf( ch, "Your MSN Screenname has changed to: %s\r\n", IMC_MSN( ch ).c_str(  ) );
      return;
   }

   if( scomp( arg[0], "comment" ) )
   {
      if( arg[1].length(  ) > 78 )
      {
         imc_to_char( "You must limit the comment line to 78 characters or less.\r\n", ch );
         return;
      }
      IMC_COMMENT( ch ) = arg[1];
      imc_printf( ch, "Your comment line has changed to: %s\r\n", IMC_COMMENT( ch ).c_str(  ) );
      return;
   }
   imcfinger( ch, "" );
   return;
}

/* Removed imcquery and put in imcinfo. -- Xorith */
IMC_CMD( imcinfo )
{
   if( argument.empty(  ) )
   {
      imc_to_char( "Syntax: imcinfo <mud>\r\n", ch );
      return;
   }

   if( !check_mud( ch, argument ) )
      return;

   imc_send_who( CH_IMCNAME( ch ), argument, "info" );
   return;
}

IMC_CMD( imcbeep )
{
   if( IMCIS_SET( IMCFLAG( ch ), IMC_DENYBEEP ) )
   {
      imc_to_char( "You are not authorized to use imcbeep.\r\n", ch );
      return;
   }

   if( argument.empty(  ) )
   {
      imc_to_char( "Usage: imcbeep user@mud\r\n", ch );
      imc_to_char( "Usage: imcbeep [on]/[off]\r\n", ch );
      return;
   }

   if( scomp( argument, "on" ) )
   {
      IMCREMOVE_BIT( IMCFLAG( ch ), IMC_BEEP );
      imc_to_char( "You now send and receive imcbeeps.\r\n", ch );
      return;
   }

   if( scomp( argument, "off" ) )
   {
      IMCSET_BIT( IMCFLAG( ch ), IMC_BEEP );
      imc_to_char( "You no longer send and receive imcbeeps.\r\n", ch );
      return;
   }

   if( IMCIS_SET( IMCFLAG( ch ), IMC_BEEP ) )
   {
      imc_to_char( "You have imcbeep turned off.\r\n", ch );
      return;
   }

   if( IMCISINVIS( ch ) )
   {
      imc_to_char( "You are invisible.\r\n", ch );
      return;
   }

   if( !check_mudof( ch, argument ) )
      return;

   imc_send_beep( CH_IMCNAME( ch ), argument );
   imc_printf( ch, "~cYou imcbeep ~Y%s~c.\r\n", argument.c_str(  ) );
   return;
}

string imc_serverinpath( string path )
{
   string::size_type x, y;

   if( path.empty(  ) )
      return "";

   if( ( x = path.find_first_of( '!' ) ) == string::npos )
      return path;

   string piece = path.substr( x + 1, path.length(  ) );

   if( ( y = piece.find_first_of( '!' ) ) == string::npos )
      return piece;

   return piece.substr( 0, y );
}

void web_imc_list( )
{
   string netname, serverpath, font;
   char urldump[MIL];
   ofstream stream;

   stream.open( IMC_WEBLIST );
   if( !stream.is_open() )
   {
      bug( "%s: fopen", __FUNCTION__ );
      perror( IMC_WEBLIST );
   }

   stream << "<table><tr><td colspan=\"5\"><font color=\"white\">Active muds on " << this_imcmud->network << ":</font></td></tr>" << endl;
   stream << "<tr><td><font color=\"#008080\">Name</font></td><td><font color=\"blue\">IMC2 Version</font></td><td><font color=\"green\">Network</font></td><td><font color=\"#00FF00\">Route</font></td></tr>" << endl;

   if( this_imcmud->www.empty() || this_imcmud->www == "??" || this_imcmud->www == "Unknown"
       || this_imcmud->www.find( "http://" ) == string::npos )
   {
      stream << "<tr><td><font color=\"#008080\">" << this_imcmud->localname << "</font></td><td><font color=\"blue\">" << this_imcmud->versionid << "</font></td><td><font color=\"green\">" << this_imcmud->network << "</font></td><td><font color=\"#00FF00\">" << this_imcmud->servername << "</font></td></tr>" << endl;
   }
   else
   {
      mudstrlcpy( urldump, this_imcmud->www.c_str(), MIL );
      stream << "<tr><td><font color=\"#008080\"><a href=\"" << urldump << "\" class=\"dcyan\" target=\"_blank\">" << this_imcmud->localname << "</a></font></td><td><font color=\"blue\">" << this_imcmud->versionid << "</font></td><td><font color=\"green\">" << this_imcmud->network << "</font></td><td><font color=\"#00FF00\">" << this_imcmud->servername << "</font></td></tr>" << endl;
   }

   int count = 1;
   list<imc_remoteinfo*>::iterator irin;
   for( irin = imc_reminfolist.begin(  ); irin != imc_reminfolist.end(  ); ++irin, ++count )
   {
      imc_remoteinfo *rin = (*irin);

      if( scomp( rin->network, "unknown" ) )
         netname = this_imcmud->network;
      else
         netname = rin->network;

      serverpath = imc_serverinpath( rin->path );

      if( rin->expired )
         font = "red";
      else
         font = "#008080";

      if( rin->url.empty() || rin->url == "??" || rin->url == "Unknown"
       || rin->url.find( "http://" ) == string::npos )
      {
         stream << "<tr><td><font color=\"" << font << "\">" << rin->rname << "</font></td><td><font color=\"blue\">" << rin->version << "</font></td><td><font color=\"green\">" << netname << "</font></td><td><font color=\"#00FF00\">" << serverpath << "</font></td></tr>" << endl;
      }
      else
      {
         mudstrlcpy( urldump, rin->url.c_str(), MIL );
         stream << "<tr><td><font color=\"" << font << "\"><a href=\"" << urldump << "\" class=\"" << ( rin->expired ? "red" : "dcyan" ) << "\" target=\"_blank\">" << rin->rname << "</a></font></td><td><font color=\"blue\">" << rin->version << "</font></td><td><font color=\"green\">" << netname << "</font></td><td><font color=\"#00FF00\">" << serverpath << "</font></td></tr>" << endl;
      }
   }
   stream << "<tr><td colspan=\"5\"><font color=\"white\">Red mud names indicate connections that are down.</font></td></tr>" << endl;
   stream << "<tr><td colspan=\"5\">" << count << " connections on " << this_imcmud->network << " found.</td></tr>" << endl;
   stream << "<tr><td colspan=\"5\">Listing last updated on: " << c_time( current_time, -1 ) << "</td></tr></table>" << endl;
   return;
}

void ev_imcweb_refresh( void *data )
{
   web_imc_list();
   add_event( 60, ev_imcweb_refresh, NULL );
}

IMC_CMD( imclist )
{
   /*
    * Silly little thing, but since imcchanlist <channel> works... why not? -- Xorith 
    */
   if( !argument.empty(  ) )
   {
      imcinfo( ch, argument );
      return;
   }

   imcpager_printf( ch, "~WActive muds on %s:\r\n", this_imcmud->network.c_str(  ) );
   imcpager_printf( ch, "~c%-15.15s ~B%-40.40s ~g%-15.15s ~G%s", "Name", "IMC2 Version", "Network", "Server" );

   /*
    * Put local mud on the list, why was this not done? It's a mud isn't it? 
    */
   imcpager_printf( ch, "\r\n\r\n~c%-15.15s ~B%-40.40s ~g%-15.15s ~G%s",
                    this_imcmud->localname.c_str(  ), this_imcmud->versionid.c_str(  ),
                    this_imcmud->network.c_str(  ), this_imcmud->servername.c_str(  ) );

   int count = 1;
   list<imc_remoteinfo*>::iterator irin;
   for( irin = imc_reminfolist.begin(  ); irin != imc_reminfolist.end(  ); ++irin, ++count )
   {
      imc_remoteinfo *rin = (*irin);
      string netname;

      if( scomp( rin->network, "unknown" ) )
         netname = this_imcmud->network;
      else
         netname = rin->network;

      string serverpath = imc_serverinpath( rin->path );

      imcpager_printf( ch, "\r\n~%c%-15.15s ~B%-40.40s ~g%-15.15s ~G%s",
                       rin->expired ? 'R' : 'c', rin->rname.c_str(  ), rin->version.c_str(  ),
                       netname.c_str(  ), serverpath.c_str(  ) );
   }
   imcpager_printf( ch, "\r\n~WRed mud names indicate connections that are down." );
   imcpager_printf( ch, "\r\n~W%d muds on %s found.\r\n", count, this_imcmud->network.c_str(  ) );
}

IMC_CMD( imcconnect )
{
   if( this_imcmud && this_imcmud->state > IMC_OFFLINE )
   {
      imc_to_char( "The IMC2 network connection appears to already be engaged!\r\n", ch );
      return;
   }
   imcconnect_attempts = 0;
   imcwait = 0;
   imc_startup( true, -1, false );
   return;
}

IMC_CMD( imcdisconnect )
{
   if( this_imcmud && this_imcmud->state == IMC_OFFLINE )
   {
      imc_to_char( "The IMC2 network connection does not appear to be engaged!\r\n", ch );
      return;
   }
   imc_shutdown( false );
   return;
}

IMC_CMD( imcconfig )
{
   vector < string > arg = vector_argument( argument, 1 );

   if( arg.empty(  ) )
   {
      imc_to_char( "~wSyntax: &Gimc <field> [value]\r\n\r\n", ch );
      imc_to_char( "~wConfiguration info for your mud. Changes save when edited.\r\n", ch );
      imc_to_char( "~wYou may set the following:\r\n\r\n", ch );
      imc_to_char( "~wShow           : ~GDisplays your current configuration.\r\n", ch );
      imc_to_char( "~wLocalname      : ~GThe name IMC2 knows your mud by.\r\n", ch );
      imc_to_char( "~wAutoconnect    : ~GToggles automatic connection on reboots.\r\n", ch );
      imc_to_char( "~wMinPlayerLevel : ~GSets the minimum level IMC2 can see your players at.\r\n", ch );
      imc_to_char( "~wMinImmLevel    : ~GSets the level at which immortal commands become available.\r\n", ch );
      imc_to_char( "~wAdminlevel     : ~GSets the level at which administrative commands become available.\r\n", ch );
      imc_to_char( "~wImplevel       : ~GSets the level at which immplementor commands become available.\r\n", ch );
      imc_to_char( "~wInfoname       : ~GName of your mud, as seen from the imcquery info sheet.\r\n", ch );
      imc_to_char( "~wInfohost       : ~GTelnet address of your mud.\r\n", ch );
      imc_to_char( "~wInfoport       : ~GTelnet port of your mud.\r\n", ch );
      imc_to_char( "~wInfoemail      : ~GEmail address of the mud's IMC administrator.\r\n", ch );
      imc_to_char( "~wInfoWWW        : ~GThe Web address of your mud.\r\n", ch );
      imc_to_char( "~wInfoDetails    : ~GSHORT Description of your mud.\r\n", ch );
      imc_to_char( "~wServerAddr     : ~GDNS or IP address of the server you mud connects to.\r\n", ch );
      imc_to_char( "~wServerPort     : ~GPort of the server your mud connects to.\r\n", ch );
      imc_to_char( "~wClientPwd      : ~GClient password for your mud.\r\n", ch );
      imc_to_char( "~wServerPwd      : ~GServer password for your mud.\r\n", ch );
      return;
   }

   if( scomp( arg[0], "sha256" ) )
   {
      this_imcmud->sha256 = !this_imcmud->sha256;

      if( this_imcmud->sha256 )
         imc_to_char( "SHA-256 support enabled.\r\n", ch );
      else
         imc_to_char( "SHA-256 support disabled.\r\n", ch );
      imc_save_config(  );
      return;
   }

   if( scomp( arg[0], "sha256pass" ) )
   {
      this_imcmud->sha256pass = !this_imcmud->sha256pass;

      if( this_imcmud->sha256pass )
         imc_to_char( "SHA-256 Authentication enabled.\r\n", ch );
      else
         imc_to_char( "SHA-256 Authentication disabled.\r\n", ch );
      imc_save_config(  );
      return;
   }

   if( scomp( arg[0], "autoconnect" ) )
   {
      this_imcmud->autoconnect = !this_imcmud->autoconnect;

      if( this_imcmud->autoconnect )
         imc_to_char( "Autoconnect enabled.\r\n", ch );
      else
         imc_to_char( "Autoconnect disabled.\r\n", ch );
      imc_save_config(  );
      return;
   }

   if( scomp( arg[0], "show" ) )
   {
      imc_printf( ch, "~wLocalname      : ~G%s\r\n", this_imcmud->localname.c_str(  ) );
      imc_printf( ch, "~wAutoconnect    : ~G%s\r\n", this_imcmud->autoconnect ? "Enabled" : "Disabled" );
      imc_printf( ch, "~wMinPlayerLevel : ~G%d\r\n", this_imcmud->minlevel );
      imc_printf( ch, "~wMinImmLevel    : ~G%d\r\n", this_imcmud->immlevel );
      imc_printf( ch, "~wAdminlevel     : ~G%d\r\n", this_imcmud->adminlevel );
      imc_printf( ch, "~wImplevel       : ~G%d\r\n", this_imcmud->implevel );
      imc_printf( ch, "~wInfoname       : ~G%s\r\n", this_imcmud->fullname.c_str(  ) );
      imc_printf( ch, "~wInfohost       : ~G%s\r\n", this_imcmud->ihost.c_str(  ) );
      imc_printf( ch, "~wInfoport       : ~G%d\r\n", this_imcmud->iport );
      imc_printf( ch, "~wInfoemail      : ~G%s\r\n", this_imcmud->email.c_str(  ) );
      imc_printf( ch, "~wInfoWWW        : ~G%s\r\n", this_imcmud->www.c_str(  ) );
      imc_printf( ch, "~wInfoDetails    : ~G%s\r\n\r\n", this_imcmud->details.c_str(  ) );
      imc_printf( ch, "~wServerAddr     : ~G%s\r\n", this_imcmud->rhost.c_str(  ) );
      imc_printf( ch, "~wServerPort     : ~G%d\r\n", this_imcmud->rport );
      imc_printf( ch, "~wClientPwd      : ~G%s\r\n", this_imcmud->clientpw.c_str(  ) );
      imc_printf( ch, "~wServerPwd      : ~G%s\r\n", this_imcmud->serverpw.c_str(  ) );
      if( this_imcmud->sha256 )
         imc_to_char( "~RThis mud has enabled SHA-256 authentication.\r\n", ch );
      else
         imc_to_char( "~RThis mud has disabled SHA-256 authentication.\r\n", ch );
      if( this_imcmud->sha256 && this_imcmud->sha256pass )
         imc_to_char( "~RThe mud is using SHA-256 encryption to authenticate.\r\n", ch );
      else
         imc_to_char( "~RThe mud is using plain text passwords to authenticate.\r\n", ch );
      return;
   }

   if( arg.size(  ) < 2 )
   {
      imcconfig( ch, "" );
      return;
   }

   if( scomp( arg[0], "minplayerlevel" ) )
   {
      int value = atoi( arg[1].c_str(  ) );

      imc_printf( ch, "Minimum level set to %d\r\n", value );
      this_imcmud->minlevel = value;
      imc_save_config(  );
      return;
   }

   if( scomp( arg[0], "minimmlevel" ) )
   {
      int value = atoi( arg[1].c_str(  ) );

      imc_printf( ch, "Immortal level set to %d\r\n", value );
      this_imcmud->immlevel = value;
      imc_save_config(  );
      return;
   }

   if( scomp( arg[0], "adminlevel" ) )
   {
      int value = atoi( arg[1].c_str(  ) );

      imc_printf( ch, "Admin level set to %d\r\n", value );
      this_imcmud->adminlevel = value;
      imc_save_config(  );
      return;
   }

   if( scomp( arg[0], "implevel" ) && IMCPERM( ch ) == IMCPERM_IMP )
   {
      int value = atoi( arg[1].c_str(  ) );

      imc_printf( ch, "Implementor level set to %d\r\n", value );
      this_imcmud->implevel = value;
      imc_save_config(  );
      return;
   }

   if( scomp( arg[0], "infoname" ) )
   {
      this_imcmud->fullname = arg[1];
      imc_save_config(  );
      imc_printf( ch, "Infoname change to %s\r\n", arg[1].c_str(  ) );
      return;
   }

   if( scomp( arg[0], "infohost" ) )
   {
      this_imcmud->ihost = arg[1];
      imc_save_config(  );
      imc_printf( ch, "Infohost changed to %s\r\n", arg[1].c_str(  ) );
      return;
   }

   if( scomp( arg[0], "infoport" ) )
   {
      this_imcmud->iport = atoi( arg[1].c_str(  ) );
      imc_save_config(  );
      imc_printf( ch, "Infoport changed to %d\r\n", this_imcmud->iport );
      return;
   }

   if( scomp( arg[0], "infoemail" ) )
   {
      this_imcmud->email = arg[1];
      imc_save_config(  );
      imc_printf( ch, "Infoemail changed to %s\r\n", arg[1].c_str(  ) );
      return;
   }

   if( scomp( arg[0], "infowww" ) )
   {
      this_imcmud->www = arg[1];
      imc_save_config(  );
      imc_printf( ch, "InfoWWW changed to %s\r\n", arg[1].c_str(  ) );
      imc_send_keepalive( NULL, "*@*" );
      return;
   }

   if( scomp( arg[0], "infodetails" ) )
   {
      this_imcmud->details = arg[1];
      imc_save_config(  );
      imc_to_char( "Infodetails updated.\r\n", ch );
      return;
   }

   if( this_imcmud->state != IMC_OFFLINE )
   {
      imc_printf( ch, "Cannot alter %s while the mud is connected to IMC.\r\n", arg[0].c_str(  ) );
      return;
   }

   if( scomp( arg[0], "serveraddr" ) )
   {
      this_imcmud->rhost = arg[1];
      imc_printf( ch, "ServerAddr changed to %s\r\n", arg[1].c_str(  ) );
      imc_save_config(  );
      return;
   }

   if( scomp( arg[0], "serverport" ) )
   {
      this_imcmud->rport = atoi( arg[1].c_str(  ) );
      imc_printf( ch, "ServerPort changed to %d\r\n", this_imcmud->rport );
      imc_save_config(  );
      return;
   }

   if( scomp( arg[0], "clientpwd" ) )
   {
      this_imcmud->clientpw = arg[1];
      imc_printf( ch, "Clientpwd changed to %s\r\n", arg[1].c_str(  ) );
      imc_save_config(  );
      return;
   }

   if( scomp( arg[0], "serverpwd" ) )
   {
      this_imcmud->serverpw = arg[1];
      imc_printf( ch, "Serverpwd changed to %s\r\n", arg[1].c_str(  ) );
      imc_save_config(  );
      return;
   }

   if( scomp( arg[0], "localname" ) )
   {
      this_imcmud->localname = arg[1];
      this_imcmud->sha256pass = false;
      imc_save_config(  );
      imc_printf( ch, "Localname changed to %s\r\n", arg[1].c_str(  ) );
      return;
   }
   imcconfig( ch, "" );
   return;
}

/* Modified this command so it's a little more helpful -- Xorith */
IMC_CMD( imcignore )
{
   vector < string > arg = vector_argument( argument, 1 );

   if( arg.empty(  ) )
   {
      imc_to_char( "~wYou currently ignore the following:\r\n", ch );

      list<string>::iterator iign;
      for( iign = CH_IMCDATA( ch )->imc_ignore.begin(  ); iign != CH_IMCDATA( ch )->imc_ignore.end(  ); ++iign )
      {
         string ign = (*iign);
         imc_printf( ch, " ~w%s\r\n", ign.c_str(  ) );
      }
      if( CH_IMCDATA( ch )->imc_ignore.empty() )
         imc_to_char( "~w none\r\n", ch );
      else
         imc_printf( ch, "~w\r\n[total %d]\r\n", (int)CH_IMCDATA( ch )->imc_ignore.size() );
      imc_to_char( "~wFor help on imcignore, type: IMCIGNORE HELP\r\n", ch );
      return;
   }

   if( scomp( arg[0], "help" ) )
   {
      imc_to_char( "~wTo see your current ignores  : ~GIMCIGNORE\r\n", ch );
      imc_to_char( "~wTo add an ignore             : ~GIMCIGNORE ADD <argument>\r\n", ch );
      imc_to_char( "~wTo delete an ignore          : ~GIMCIGNORE DELETE <argument>\r\n", ch );
      imc_to_char( "~WSee your MUD's help for more information.\r\n", ch );
      return;
   }

   if( arg.size(  ) < 2 )
   {
      imc_to_char( "~wMust specify both action and name.\r\n", ch );
      imc_to_char( "~wPlease see IMCIGNORE HELP for details.\r\n", ch );
      return;
   }

   if( scomp( arg[0], "delete" ) )
   {
      list<string>::iterator iign;
      for( iign = CH_IMCDATA( ch )->imc_ignore.begin(  ); iign != CH_IMCDATA( ch )->imc_ignore.end(  ); ++iign )
      {
         string ign = (*iign);

         if( scomp( ign, arg[1] ) )
         {
            CH_IMCDATA( ch )->imc_ignore.remove( ign );
            imc_to_char( "~wEntry deleted.\r\n", ch );
            return;
         }
      }
      imc_to_char( "Entry not found.\r\nPlease check your ignores by typing IMCIGNORE with no arguments.\r\n", ch );
      return;
   }

   if( scomp( arg[0], "add" ) )
   {
      CH_IMCDATA( ch )->imc_ignore.push_back( arg[1] );
      imc_printf( ch, "~w%s will now be ignored.\r\n", arg[1].c_str(  ) );
      return;
   }
   imcignore( ch, "help" );
   return;
}

/* Made this command a little more helpful --Xorith */
IMC_CMD( imcban )
{
   vector < string > arg = vector_argument( argument, 1 );

   if( arg.empty(  ) )
   {
      imc_to_char( "The mud currently bans the following:\r\n", ch );

      list<string>::iterator iban;
      for( iban = imc_banlist.begin(  ); iban != imc_banlist.end(  ); ++iban )
      {
         string ban = (*iban);
         imc_printf( ch, "~ w%s\r\n", ban.c_str(  ) );
      }
      if( imc_banlist.empty() )
         imc_to_char( "~w none\r\n", ch );
      else
         imc_printf( ch, "~w\r\n[total %d]\r\n", (int)imc_banlist.size() );
      imc_to_char( "~wType: IMCBAN HELP for more information.\r\n", ch );
      return;
   }

   if( scomp( arg[0], "help" ) )
   {
      imc_to_char( "~wTo see the current bans             : ~GIMCBAN\r\n", ch );
      imc_to_char( "~wTo add a MUD to the ban list        : ~GIMCBAN ADD <argument>\r\n", ch );
      imc_to_char( "~wTo delete a MUD from the ban list   : ~GIMCBAN DELETE <argument>\r\n", ch );
      imc_to_char( "~WSee your MUD's help for more information.\r\n", ch );
      return;
   }

   if( arg.size(  ) < 2 )
   {
      imc_to_char( "Must specify both action and name.\r\nPlease type IMCBAN HELP for more information.\r\n", ch );
      return;
   }

   if( scomp( arg[0], "delete" ) )
   {
      imc_banlist.remove( arg[1] );
      imc_savebans(  );
      imc_to_char( "Entry deleted.\r\n", ch );
      return;
   }

   if( scomp( arg[0], "add" ) )
   {
      imc_banlist.push_back( arg[1] );
      imc_savebans(  );
      imc_printf( ch, "Mud %s will now be banned.\r\n", arg[1].c_str(  ) );
      return;
   }
   imcban( ch, "" );
   return;
}

IMC_CMD( imc_deny_channel )
{
   vector < string > arg = vector_argument( argument, 2 );
   char_data *victim;
   imc_channel *channel;

   if( arg.size(  ) < 2 )
   {
      imc_to_char( "~wUsage: imcdeny <person> <local channel name>\r\n", ch );
      imc_to_char( "~wUsage: imcdeny <person> [tell/beep/finger]\r\n", ch );
      return;
   }

   if( !( victim = imc_find_user( arg[0] ) ) )
   {
      imc_to_char( "No such person is currently online.\r\n", ch );
      return;
   }

   if( IMCPERM( ch ) <= IMCPERM( victim ) )
   {
      imc_to_char( "You cannot alter their settings.\r\n", ch );
      return;
   }

   if( scomp( arg[1], "tell" ) )
   {
      if( !IMCIS_SET( IMCFLAG( victim ), IMC_DENYTELL ) )
      {
         IMCSET_BIT( IMCFLAG( victim ), IMC_DENYTELL );
         imc_printf( ch, "%s can no longer use imctells.\r\n", CH_IMCNAME( victim ) );
         return;
      }
      IMCREMOVE_BIT( IMCFLAG( victim ), IMC_DENYTELL );
      imc_printf( ch, "%s can use imctells again.\r\n", CH_IMCNAME( victim ) );
      return;
   }

   if( scomp( arg[1], "beep" ) )
   {
      if( !IMCIS_SET( IMCFLAG( victim ), IMC_DENYBEEP ) )
      {
         IMCSET_BIT( IMCFLAG( victim ), IMC_DENYBEEP );
         imc_printf( ch, "%s can no longer use imcbeeps.\r\n", CH_IMCNAME( victim ) );
         return;
      }
      IMCREMOVE_BIT( IMCFLAG( victim ), IMC_DENYBEEP );
      imc_printf( ch, "%s can use imcbeeps again.\r\n", CH_IMCNAME( victim ) );
      return;
   }

   if( scomp( arg[1], "finger" ) )
   {
      if( !IMCIS_SET( IMCFLAG( victim ), IMC_DENYFINGER ) )
      {
         IMCSET_BIT( IMCFLAG( victim ), IMC_DENYFINGER );
         imc_printf( ch, "%s can no longer use imcfingers.\r\n", CH_IMCNAME( victim ) );
         return;
      }
      IMCREMOVE_BIT( IMCFLAG( victim ), IMC_DENYFINGER );
      imc_printf( ch, "%s can use imcfingers again.\r\n", CH_IMCNAME( victim ) );
      return;
   }

   /*
    * Assumed to be denying a channel by this stage. 
    */
   if( !( channel = imc_findchannel( arg[1] ) ) )
   {
      imc_to_char( "Unknown or unconfigured local channel. Check your channel name.\r\n", ch );
      return;
   }

   if( imc_hasname( IMC_DENY( victim ), channel->local_name ) )
   {
      imc_printf( ch, "%s can now listen to %s\r\n", CH_IMCNAME( victim ), channel->local_name.c_str(  ) );
      imc_removename( IMC_DENY( victim ), channel->local_name );
   }
   else
   {
      imc_printf( ch, "%s can no longer listen to %s\r\n", CH_IMCNAME( victim ), channel->local_name.c_str(  ) );
      imc_addname( IMC_DENY( victim ), channel->local_name );
   }
   return;
}

IMC_CMD( imcpermstats )
{
   char_data *victim;

   if( argument.empty(  ) )
   {
      imc_to_char( "Usage: imcperms <user>\r\n", ch );
      return;
   }

   if( !( victim = imc_find_user( argument ) ) )
   {
      imc_to_char( "No such person is currently online.\r\n", ch );
      return;
   }

   if( IMCPERM( victim ) < 0 || IMCPERM( victim ) > IMCPERM_IMP )
   {
      imc_printf( ch, "%s has an invalid permission setting!\r\n", CH_IMCNAME( victim ) );
      return;
   }

   imc_printf( ch, "~GPermissions for %s: %s\r\n", CH_IMCNAME( victim ), imcperm_names[IMCPERM( victim )] );
   imc_printf( ch, "~gThese permissions were obtained %s.\r\n",
               IMCIS_SET( IMCFLAG( victim ), IMC_PERMOVERRIDE ) ? "manually via imcpermset" : "automatically by level" );
   return;
}

IMC_CMD( imcpermset )
{
   vector < string > arg = vector_argument( argument, 1 );
   char_data *victim;
   int permvalue;

   if( arg.size(  ) < 2 )
   {
      imc_to_char( "~wUsage: imcpermset <user> <permission>\r\n", ch );
      imc_to_char( "~wPermission can be one of: None, Mort, Imm, Admin, Imp\r\n", ch );
      return;
   }

   if( !( victim = imc_find_user( arg[0] ) ) )
   {
      imc_to_char( "No such person is currently online.\r\n", ch );
      return;
   }

   if( scomp( arg[1], "override" ) )
      permvalue = -1;
   else
   {
      permvalue = get_imcpermvalue( arg[1] );

      if( !imccheck_permissions( ch, permvalue, IMCPERM( victim ), true ) )
         return;
   }

   /*
    * Just something to avoid looping through the channel clean-up --Xorith 
    */
   if( IMCPERM( victim ) == permvalue )
   {
      imc_printf( ch, "%s already has a permission level of %s.\r\n", CH_IMCNAME( victim ), imcperm_names[permvalue] );
      return;
   }

   if( permvalue == -1 )
   {
      IMCREMOVE_BIT( IMCFLAG( victim ), IMC_PERMOVERRIDE );
      imc_printf( ch, "~YPermission flag override has been removed from %s\r\n", CH_IMCNAME( victim ) );
      return;
   }

   IMCPERM( victim ) = permvalue;
   IMCSET_BIT( IMCFLAG( victim ), IMC_PERMOVERRIDE );

   imc_printf( ch, "~YPermission level for %s has been changed to %s\r\n", CH_IMCNAME( victim ), imcperm_names[permvalue] );
   /*
    * Channel Clean-Up added by Xorith 9-24-03 
    */
   /*
    * Note: Let's not clean up IMC_DENY for a player. Never know... 
    */
   if( !IMC_LISTEN( victim ).empty(  ) && this_imcmud->state == IMC_ONLINE )
   {
      imc_channel *channel = NULL;
      vector < string > arg2 = vector_argument( IMC_LISTEN( victim ), -1 );
      vector < string >::iterator vec = arg2.begin(  );

      while( vec != arg2.end(  ) )
      {
         if( !( channel = imc_findchannel( *vec ) ) )
            imc_removename( IMC_LISTEN( victim ), ( *vec ) );
         if( channel && IMCPERM( victim ) < channel->level )
         {
            imc_removename( IMC_LISTEN( victim ), ( *vec ) );
            imc_printf( ch, "~WRemoving '%s' level channel: '%s', exceeding new permission of '%s'\r\n",
                        imcperm_names[channel->level], channel->local_name.c_str(  ), imcperm_names[IMCPERM( victim )] );
         }
         ++vec;
      }
   }
   return;
}

IMC_CMD( imcinvis )
{
   if( IMCIS_SET( IMCFLAG( ch ), IMC_INVIS ) )
   {
      IMCREMOVE_BIT( IMCFLAG( ch ), IMC_INVIS );
      imc_to_char( "You are now imcvisible.\r\n", ch );
   }
   else
   {
      IMCSET_BIT( IMCFLAG( ch ), IMC_INVIS );
      imc_to_char( "You are now imcinvisible.\r\n", ch );
   }
   return;
}

IMC_CMD( imcchanwho )
{
   imc_channel *c;
   imc_packet *p;
   vector < string > arg = vector_argument( argument, -1 );

   if( arg.size(  ) < 2 )
   {
      imc_to_char( "Usage: imcchanwho <channel> [<mud> <mud> <mud> <...>|<all>]\r\n", ch );
      return;
   }

   if( !( c = imc_findchannel( arg[0] ) ) )
   {
      imc_to_char( "No such channel.\r\n", ch );
      return;
   }

   if( IMCPERM( ch ) < c->level )
   {
      imc_to_char( "No such channel.\r\n", ch );
      return;
   }

   if( !c->refreshed )
   {
      imc_printf( ch, "%s has not been refreshed yet.\r\n", c->chname.c_str(  ) );
      return;
   }

   if( !scomp( arg[1], "all" ) )
   {
      vector < string >::iterator vec = arg.begin(  );

      while( vec != arg.end(  ) )
      {
         if( ( *vec ) == arg[0] )
         {
            ++vec;
            continue;
         }

         if( !check_mud( ch, ( *vec ) ) )
         {
            ++vec;
            continue;
         }

         p = new imc_packet( CH_IMCNAME( ch ), "ice-chan-who", ( *vec ) );
         p->data << "level=" << IMCPERM( ch );
         p->data << " channel=" << c->chname;
         p->data << " lname=" << ( !c->local_name.empty(  )? c->local_name : c->chname );
         p->send(  );
         ++vec;
      }
      return;
   }

   p = new imc_packet( CH_IMCNAME( ch ), "ice-chan-who", "*" );
   p->data << "level=" << IMCPERM( ch );
   p->data << " channel=" << c->chname;
   p->data << " lname=" << ( !c->local_name.empty(  )? c->local_name : c->chname );
   p->send(  );

   imc_printf( ch, "~G%s", get_local_chanwho( c ).c_str() );
   return;
}

IMC_CMD( imcremoteadmin )
{
   vector < string > arg = vector_argument( argument, 3 );
   imc_remoteinfo *r;
   imc_packet *p;

   if( arg.size(  ) < 3 )
   {
      imc_to_char( "~wSyntax: imcadmin <server> <password> <command> [<data..>]\r\n", ch );
      imc_to_char( "~wYou must be an approved server administrator to use remote commands.\r\n", ch );
      return;
   }

   if( !( r = imc_find_reminfo( arg[0] ) ) )
   {
      imc_printf( ch, "~W%s ~cis not a valid server name.\r\n", arg[0].c_str(  ) );
      return;
   }

   if( r->expired )
   {
      imc_printf( ch, "~W%s ~cis not connected right now.\r\n", r->rname.c_str(  ) );
      return;
   }

   p = new imc_packet( CH_IMCNAME( ch ), "remote-admin", "IMC@" + r->rname );
   p->data << "command=" << arg[2];
   if( arg.size(  ) > 3 )
      p->data << " data=" << arg[3];
   if( this_imcmud->sha256pass )
   {
      char cryptpw[LGST];
      char *hash;

      snprintf( cryptpw, LGST, "%ld%s", imc_sequencenumber + 1, arg[1].c_str(  ) );
      hash = sha256_crypt( cryptpw );
      p->data << " hash=" << hash;
   }
   p->send(  );

   imc_to_char( "~wRemote command sent.\r\n", ch );
   return;
}

IMC_CMD( imchelp )
{
   list<imc_help_table*>::iterator ihlp;
   ostringstream buf;
   int col, perm;

   if( argument.empty(  ) )
   {
      buf << "~gHelp is available for the following commands:" << endl;
      buf << "~G---------------------------------------------" << endl;
      for( perm = IMCPERM_MORT; perm <= IMCPERM( ch ); ++perm )
      {
         col = 0;
         buf << endl << "~g" << imcperm_names[perm] << " helps:~G" << endl;
         for( ihlp = imc_helplist.begin(  ); ihlp != imc_helplist.end(  ); ++ihlp )
         {
            imc_help_table *hlp = (*ihlp);

            if( hlp->level != perm )
               continue;

            buf << setw( 15 ) << setiosflags( ios::left ) << hlp->hname;
            if( ++col % 6 == 0 )
               buf << endl;
         }
         if( col % 6 != 0 )
            buf << endl;
      }
      imc_to_pager( buf.str(  ), ch );
      return;
   }

   for( ihlp = imc_helplist.begin(  ); ihlp != imc_helplist.end(  ); ++ihlp )
   {
      imc_help_table *hlp = (*ihlp);

      if( hlp->hname == argument )
      {
         if( hlp->text.empty(  ) )
            imc_printf( ch, "~gNo inforation available for topic ~W%s~g.\r\n", hlp->hname.c_str(  ) );
         else
            imc_printf( ch, "~g%s\r\n", hlp->text.c_str(  ) );
         return;
      }
   }
   imc_printf( ch, "~gNo help exists for topic ~W%s~g.\r\n", argument.c_str(  ) );
   return;
}

IMC_CMD( imccolor )
{
   vector < string > arg = vector_argument( argument, 1 );

   if( !arg.empty(  ) && scomp( arg[0], "save" ) && IMCPERM( ch ) == IMCPERM_IMP )
   {
      imc_savecolor(  );
      return;
   }

   if( IMCIS_SET( IMCFLAG( ch ), IMC_COLORFLAG ) )
   {
      IMCREMOVE_BIT( IMCFLAG( ch ), IMC_COLORFLAG );
      imc_to_char( "IMC2 color is now off.\r\n", ch );
   }
   else
   {
      IMCSET_BIT( IMCFLAG( ch ), IMC_COLORFLAG );
      imc_to_char( "~RIMC2 c~Yo~Gl~Bo~Pr ~Ris now on. Enjoy :)\r\n", ch );
   }
   return;
}

IMC_CMD( imcafk )
{
   if( IMCIS_SET( IMCFLAG( ch ), IMC_AFK ) )
   {
      IMCREMOVE_BIT( IMCFLAG( ch ), IMC_AFK );
      imc_to_char( "You are no longer AFK to IMC2.\r\n", ch );
   }
   else
   {
      IMCSET_BIT( IMCFLAG( ch ), IMC_AFK );
      imc_to_char( "You are now AFK to IMC2.\r\n", ch );
   }
   return;
}

IMC_CMD( imcdebug )
{
   imcpacketdebug = !imcpacketdebug;

   if( imcpacketdebug )
      imc_to_char( "Packet debug enabled.\r\n", ch );
   else
      imc_to_char( "Packet debug disabled.\r\n", ch );
   return;
}

/* This is very possibly going to be spammy as hell */
IMC_CMD( imc_show_ucache_contents )
{
   imc_ucache_data *user;
   int users = 0;

   imc_to_pager( "Cached user information\r\n", ch );
   imc_to_pager( "User                          | Gender ( 0 = Male, 1 = Female, 2 = Other )\r\n", ch );
   imc_to_pager( "--------------------------------------------------------------------------\r\n", ch );

   list<imc_ucache_data*>::iterator uch;
   for( uch = imc_ucachelist.begin(  ); uch != imc_ucachelist.end(  ); ++uch )
   {
      user = (*uch);

      imcpager_printf( ch, "%-30s %d\r\n", user->name.c_str(  ), user->gender );
      ++users;
   }
   imcpager_printf( ch, "%d users being cached.\r\n", users );
   return;
}

IMC_CMD( imccedit )
{
   vector < string > arg = vector_argument( argument, 2 );
   imc_command_table *cmd;
   bool found = false, aliasfound = false;

   if( arg.size(  ) < 2 )
   {
      imc_to_char( "Usage: imccedit <command> <create|delete|alias|rename|code|permission|connected> <field>.\r\n", ch );
      return;
   }

   list<imc_command_table*>::iterator icom;
   for( icom = imc_commandlist.begin(  ); icom != imc_commandlist.end(  ); ++icom )
   {
      cmd = (*icom);

      if( scomp( cmd->name, arg[0] ) )
      {
         found = true;
         break;
      }

      list<string>::iterator ials;
      for( ials = cmd->aliaslist.begin(  ); ials != cmd->aliaslist.end(  ); ++ials )
      {
         string als = (*ials);
         if( scomp( als, arg[0] ) )
            aliasfound = true;
      }
   }

   if( scomp( arg[1], "create" ) )
   {
      if( found )
      {
         imc_printf( ch, "~gA command named ~W%s ~galready exists.\r\n", arg[0].c_str(  ) );
         return;
      }

      if( aliasfound )
      {
         imc_printf( ch, "~g%s already exists as an alias for another command.\r\n", arg[0].c_str(  ) );
         return;
      }

      cmd = new imc_command_table;
      cmd->name = arg[0];
      cmd->level = IMCPERM( ch );
      cmd->connected = false;
      cmd->aliaslist.clear(  );
      imc_printf( ch, "~gCommand ~W%s ~gcreated.\r\n", cmd->name.c_str(  ) );
      if( arg.size(  ) >= 2 )
      {
         cmd->function = imc_function( arg[2] );
         if( !cmd->function )
            imc_printf( ch, "~gFunction ~W%s ~gdoes not exist - set to NULL.\r\n", arg[2].c_str(  ) );
      }
      else
      {
         imc_to_char( "~gFunction set to NULL.\r\n", ch );
         cmd->function = NULL;
      }
      imc_commandlist.push_back( cmd );
      imc_savecommands(  );
      return;
   }

   if( !found )
   {
      imc_printf( ch, "~gNo command named ~W%s ~gexists.\r\n", arg[0].c_str(  ) );
      return;
   }

   if( !imccheck_permissions( ch, cmd->level, cmd->level, false ) )
      return;

   if( scomp( arg[1], "delete" ) )
   {
      imc_printf( ch, "~gCommand ~W%s ~ghas been deleted.\r\n", cmd->name.c_str(  ) );

      cmd->aliaslist.clear(  );
      imc_commandlist.remove( cmd );
      deleteptr( cmd );
      imc_savecommands(  );
      return;
   }

   /*
    * MY GOD! What an inefficient mess you've made Samson! 
    */
   if( scomp( arg[1], "alias" ) )
   {
      if( arg.size(  ) < 3 )
      {
         imc_to_char( "You must specify an alias to set or unset.\r\n", ch );
         return;
      }

      list<string>::iterator ials;
      for( ials = cmd->aliaslist.begin(  ); ials != cmd->aliaslist.end(  ); ++ials )
      {
         string als = (*ials);
         if( scomp( als, arg[2] ) )
         {
            imc_printf( ch, "~W%s ~ghas been removed as an alias for ~W%s\r\n", arg[2].c_str(  ), cmd->name.c_str(  ) );
            cmd->aliaslist.remove( als );
            imc_savecommands(  );
            return;
         }
      }

      for( icom = imc_commandlist.begin(  ); icom != imc_commandlist.end(  ); ++icom )
      {
         imc_command_table *com = (*icom);

         if( scomp( com->name, arg[2] ) )
         {
            imc_printf( ch, "~W%s &gis already a command name.\r\n", arg[2].c_str(  ) );
            return;
         }
         for( ials = com->aliaslist.begin(  ); ials != com->aliaslist.end(  ); ++ials )
         {
            string als = (*ials);
            if( scomp( arg[2], als ) )
            {
               imc_printf( ch, "~W%s ~gis already an alias for ~W%s\r\n", arg[2].c_str(  ), com->name.c_str(  ) );
               return;
            }
         }
      }
      cmd->aliaslist.push_back( arg[2] );
      imc_printf( ch, "~W%s ~ghas been added as an alias for ~W%s\r\n", arg[2].c_str(  ), cmd->name.c_str(  ) );
      imc_savecommands(  );
      return;
   }

   if( scomp( arg[1], "connected" ) )
   {
      cmd->connected = !cmd->connected;

      if( cmd->connected )
         imc_printf( ch, "~gCommand ~W%s ~gwill now require a connection to IMC2 to use.\r\n", cmd->name.c_str(  ) );
      else
         imc_printf( ch, "~gCommand ~W%s ~gwill no longer require a connection to IMC2 to use.\r\n", cmd->name.c_str(  ) );
      imc_savecommands(  );
      return;
   }

   if( scomp( arg[1], "show" ) )
   {
      ostringstream buf;

      imc_printf( ch, "~gCommand       : ~W%s\r\n", cmd->name.c_str(  ) );
      imc_printf( ch, "~gPermission    : ~W%s\r\n", imcperm_names[cmd->level] );
      imc_printf( ch, "~gFunction      : ~W%s\r\n", cmd->funcname.c_str() );
      imc_printf( ch, "~gConnection Req: ~W%s\r\n", cmd->connected ? "Yes" : "No" );
      if( !cmd->aliaslist.empty(  ) )
      {
         int col = 0;
         buf << "~gAliases       : ~W";

         list<string>::iterator ials;
         for( ials = cmd->aliaslist.begin(  ); ials != cmd->aliaslist.end(  ); ++ials )
         {
            string als = (*ials);
            buf << als << " ";
            if( ++col % 10 == 0 )
               buf << endl;
         }
         if( col % 10 != 0 )
            buf << endl;
         imc_to_char( buf.str(  ), ch );
      }
      return;
   }

   if( arg.size(  ) < 3 )
   {
      imc_to_char( "Required argument missing.\r\n", ch );
      imccedit( ch, "" );
      return;
   }

   if( scomp( arg[1], "rename" ) )
   {
      imc_printf( ch, "~gCommand ~W%s ~ghas been renamed to ~W%s.\r\n", cmd->name.c_str(  ), arg[2].c_str(  ) );
      cmd->name = arg[2];
      imc_savecommands(  );
      return;
   }

   if( scomp( arg[1], "code" ) )
   {
      cmd->function = imc_function( arg[2] );
      if( !cmd->function )
         imc_printf( ch, "~gFunction ~W%s ~gdoes not exist - set to NULL.\r\n", arg[2].c_str(  ) );
      else
      {
         imc_printf( ch, "~gFunction set to ~W%s.\r\n", arg[2].c_str(  ) );
         cmd->funcname = arg[2];
      }
      imc_savecommands(  );
      return;
   }

   if( scomp( arg[1], "perm" ) || scomp( arg[1], "permission" ) || scomp( arg[1], "level" ) )
   {
      int permvalue = get_imcpermvalue( arg[2] );

      if( !imccheck_permissions( ch, permvalue, cmd->level, false ) )
         return;

      cmd->level = permvalue;
      imc_printf( ch, "~gCommand ~W%s ~gpermission level has been changed to ~W%s.\r\n", cmd->name.c_str(  ),
                  imcperm_names[permvalue] );
      imc_savecommands(  );
      return;
   }
   imccedit( ch, "" );
   return;
}

IMC_CMD( imchedit )
{
   vector < string > arg = vector_argument( argument, 2 );
   imc_help_table *help = NULL;
   bool found = false;

   if( arg.size(  ) < 3 )
   {
      imc_to_char( "~wUsage: imchedit <topic> [name|perm] <field>\r\n", ch );
      imc_to_char( "~wWhere <field> can be either name, or permission level.\r\n", ch );
      return;
   }

   list<imc_help_table*>::iterator hlp;
   for( hlp = imc_helplist.begin(  ); hlp != imc_helplist.end(  ); ++hlp )
   {
      help = *hlp;

      if( scomp( help->hname, arg[0] ) )
      {
         found = true;
         break;
      }
   }

   if( !found || !help )
   {
      imc_printf( ch, "~gNo help exists for topic ~W%s~g. You will need to add it to the helpfile manually.\r\n",
                  arg[0].c_str(  ) );
      return;
   }

   if( scomp( arg[1], "name" ) )
   {
      imc_printf( ch, "~W%s ~ghas been renamed to ~W%s.\r\n", help->hname.c_str(  ), arg[2].c_str(  ) );
      help->hname = arg[2];
      imc_savehelps(  );
      return;
   }

   if( scomp( arg[1], "perm" ) )
   {
      int permvalue = get_imcpermvalue( arg[2] );

      if( !imccheck_permissions( ch, permvalue, help->level, false ) )
         return;

      imc_printf( ch, "~gPermission level for ~W%s ~ghas been changed to ~W%s.\r\n", help->hname.c_str(  ),
                  imcperm_names[permvalue] );
      help->level = permvalue;
      imc_savehelps(  );
      return;
   }
   imchedit( ch, "" );
   return;
}

IMC_CMD( imcnotify )
{
   if( IMCIS_SET( IMCFLAG( ch ), IMC_NOTIFY ) )
   {
      IMCREMOVE_BIT( IMCFLAG( ch ), IMC_NOTIFY );
      imc_to_char( "You no longer see channel notifications.\r\n", ch );
   }
   else
   {
      IMCSET_BIT( IMCFLAG( ch ), IMC_NOTIFY );
      imc_to_char( "You now see channel notifications.\r\n", ch );
   }
   return;
}

IMC_CMD( imcrefresh )
{
   list<imc_remoteinfo*>::iterator rin;
   for( rin = imc_reminfolist.begin(  ); rin != imc_reminfolist.end(  ); )
   {
      imc_remoteinfo *r = (*rin);
      ++rin;

      deleteptr( r );
   }
   imc_reminfolist.clear(  );
   imc_request_keepalive(  );
   imc_firstrefresh(  );

   imc_to_char( "Mud list is being refreshed.\r\n", ch );
   return;
}

IMC_CMD( imctemplates )
{
   imc_to_char( "Refreshing all templates.\r\n", ch );
   imc_load_who_template();
}

IMC_CMD( imclast )
{
   imc_packet *p = new imc_packet( CH_IMCNAME( ch ), "imc-laston", this_imcmud->servername );
   if( !argument.empty(  ) )
      p->data << "username=" << argument;
   p->send(  );

   return;
}

IMC_CMD( imc_other )
{
   ostringstream buf;
   int col, perm;

   buf << "~gThe following commands are available:" << endl;
   buf << "~G-------------------------------------" << endl;
   for( perm = IMCPERM_MORT; perm <= IMCPERM( ch ); ++perm )
   {
      col = 0;
      buf << endl << "~g" << imcperm_names[perm] << " commands:~G" << endl;

      list<imc_command_table*>::iterator icom;
      for( icom = imc_commandlist.begin(  ); icom != imc_commandlist.end(  ); ++icom )
      {
         imc_command_table *com = (*icom);

         if( com->level != perm )
            continue;

         buf << setw( 15 ) << setiosflags( ios::left ) << com->name;
         if( ++col % 6 == 0 )
            buf << endl;
      }
      if( col % 6 != 0 )
         buf << endl;
   }
   imc_to_pager( buf.str(  ), ch );
   imc_to_pager( "\r\n~gFor information about a specific command, see ~Wimchelp <command>~g.\r\n", ch );
   return;
}

string imc_find_social( char_data * ch, string sname, string person, string mud, int victim )
{
   string socname;
   social_type *social;

   if( !( social = find_social( sname ) ) )
   {
      imc_printf( ch, "~YSocial ~W%s~Y does not exist on this mud.\r\n", sname.c_str(  ) );
      return "";
   }

   if( !person.empty(  ) && !mud.empty(  ) )
   {
      if( scomp( person, CH_IMCNAME( ch ) ) && scomp( mud, this_imcmud->localname ) )
      {
         if( !social->others_auto )
         {
            imc_printf( ch, "~YSocial ~W%s~Y: Missing others_auto.\r\n", social->name );
            return "";
         }
         socname = social->others_auto;
      }
      else
      {
         if( victim == 0 )
         {
            if( !social->others_found )
            {
               imc_printf( ch, "~YSocial ~W%s~Y: Missing others_found.\r\n", social->name );
               return "";
            }
            socname = social->others_found;
         }
         else if( victim == 1 )
         {
            if( !social->vict_found )
            {
               imc_printf( ch, "~YSocial ~W%s~Y: Missing vict_found.\r\n", social->name );
               return "";
            }
            socname = social->vict_found;
         }
         else
         {
            if( !social->char_found )
            {
               imc_printf( ch, "~YSocial ~W%s~Y: Missing char_found.\r\n", social->name );
               return "";
            }
            socname = social->char_found;
         }
      }
   }
   else
   {
      if( victim == 0 || victim == 1 )
      {
         if( !social->others_no_arg )
         {
            imc_printf( ch, "~YSocial ~W%s~Y: Missing others_no_arg.\r\n", social->name );
            return "";
         }
         socname = social->others_no_arg;
      }
      else
      {
         if( !social->char_no_arg )
         {
            imc_printf( ch, "~YSocial ~W%s~Y: Missing char_no_arg.\r\n", social->name );
            return "";
         }
         socname = social->char_no_arg;
      }
   }
   return socname;
}

/* Revised 10/10/03 by Xorith: Recognize the need to capitalize for a new sentence. */
string imc_act_string( string format, char_data * ch, char_data * victim )
{
   static char *const he_she[] = { "it", "he", "she" };
   static char *const him_her[] = { "it", "him", "her" };
   static char *const his_her[] = { "its", "his", "her" };
   string buf = "", i;
   unsigned int x = 0;
   bool should_upper = false;

   if( format.empty(  ) || !ch )
      return "";

   while( x < format.length(  ) )
   {
      if( format[x] == '.' || format[x] == '?' || format[x] == '!' )
         should_upper = true;
      else if( should_upper == true && !isspace( format[x] ) && format[x] != '$' )
         should_upper = false;

      if( format[x] != '$' )
      {
         buf += format[x];
         ++x;
         continue;
      }
      ++x;

      if( ( !victim ) && ( format[x] == 'N' || format[x] == 'E' || format[x] == 'M' || format[x] == 'S' || format[x] == 'K' ) )
         i = " !!!!! ";
      else
      {
         switch ( format[x] )
         {
            default:
               i = " !!!!! ";
               break;
            case 'n':
               i = imc_makename( CH_IMCNAME( ch ), this_imcmud->localname );
               break;
            case 'N':
               i = CH_IMCNAME( victim );
               break;

            case 'e':
               i = should_upper ?
                  capitalize( he_she[URANGE( 0, CH_IMCSEX( ch ), 2 )] ) : he_she[URANGE( 0, CH_IMCSEX( ch ), 2 )];
               break;

            case 'E':
               i = should_upper ?
                  capitalize( he_she[URANGE( 0, CH_IMCSEX( victim ), 2 )] ) : he_she[URANGE( 0, CH_IMCSEX( victim ), 2 )];
               break;

            case 'm':
               i = should_upper ?
                  capitalize( him_her[URANGE( 0, CH_IMCSEX( ch ), 2 )] ) : him_her[URANGE( 0, CH_IMCSEX( ch ), 2 )];
               break;

            case 'M':
               i = should_upper ?
                  capitalize( him_her[URANGE( 0, CH_IMCSEX( victim ), 2 )] ) : him_her[URANGE( 0, CH_IMCSEX( victim ), 2 )];
               break;

            case 's':
               i = should_upper ?
                  capitalize( his_her[URANGE( 0, CH_IMCSEX( ch ), 2 )] ) : his_her[URANGE( 0, CH_IMCSEX( ch ), 2 )];
               break;

            case 'S':
               i = should_upper ?
                  capitalize( his_her[URANGE( 0, CH_IMCSEX( victim ), 2 )] ) : his_her[URANGE( 0, CH_IMCSEX( victim ), 2 )];
               break;

            case 'k':
            {
               vector < string > arg = vector_argument( CH_IMCNAME( ch ), 1 );

               if( !arg.empty(  ) )
                  i = arg[0];
               else
                  i = " !!!!! ";
               break;
            }

            case 'K':
            {
               vector < string > arg = vector_argument( CH_IMCNAME( victim ), 1 );

               if( !arg.empty(  ) )
                  i = arg[0];
               else
                  i = " !!!!! ";
               break;
            }
         }
      }
      ++x;
      unsigned int y = 0;
      while( y < i.length(  ) )
      {
         buf += i[y];
         ++y;
      }
   }
   return buf;
}

char_data *imc_make_skeleton( string name )
{
   char_data *skeleton;

   skeleton = new char_data;

   skeleton->name = STRALLOC( name.c_str(  ) );
   skeleton->short_descr = STRALLOC( name.c_str(  ) );
   skeleton->in_room = get_room_index( ROOM_VNUM_LIMBO );

   return skeleton;
}

/* Socials can now be called anywhere you want them - like for instance, tells.
 * Thanks to Darien@Sandstorm for this suggestion. -- Samson 2-21-04
 */
string imc_send_social( char_data * ch, string argument, int telloption )
{
   vector < string > arg = vector_argument( argument, 1 );
   char_data *skeleton = NULL;
   string msg, person, mud, socbuf;
   string::size_type ps;

   if( arg.size(  ) > 1 )
   {
      if( ( ps = arg[1].find( '@' ) ) == string::npos )
      {
         imc_to_char( "You need to specify a person@mud for a target.\r\n", ch );
         return "";
      }
      else
      {
         person = arg[1].substr( 0, ps );
         mud = arg[1].substr( ps + 1, arg[1].length(  ) );
      }
   }

   socbuf = imc_find_social( ch, arg[0], person, mud, telloption );
   if( socbuf.empty(  ) )
      return "";

   if( arg.size(  ) > 1 )
   {
      int sex;

      sex = imc_get_ucache_gender( arg[1] );
      if( sex == -1 )
      {
         imc_send_ucache_request( arg[1] );
         sex = SEX_MALE;
      }
      else
         sex = imctodikugender( sex );

      skeleton = imc_make_skeleton( arg[1] );
      CH_IMCSEX( skeleton ) = sex;
   }

   msg = imc_act_string( socbuf, ch, skeleton );
   if( skeleton )
      deleteptr( skeleton );
   return ( color_mtoi( msg ) );
}

IMC_FUN *imc_function( string name )
{
   void *funHandle;
#if !defined(WIN32)
   const char *error;
#else
   DWORD error;
#endif

   funHandle = dlsym( sysdata->dlHandle, name.c_str() );
   if( ( error = dlerror(  ) ) )
   {
      imcbug( "%s: %s", __FUNCTION__, error );
      return NULL;
   }
   return (IMC_FUN*)funHandle;
}

/* Check for IMC channels, return true to stop command processing, false otherwise */
bool imc_command_hook( char_data * ch, char *command, char *argument )
{
   imc_channel *c;
   imc_command_table *cmd = NULL;
   string newcmd, newarg, p;

   if( ch->isnpc(  ) )
      return false;

   if( !this_imcmud )
   {
      imcbug( "%s", "Ooops. IMC being called with no configuration!" );
      return false;
   }

   if( imc_commandlist.empty(  ) )
   {
      imcbug( "%s", "ACK! There's no damn command data loaded!" );
      return false;
   }

   if( IMCPERM( ch ) <= IMCPERM_NONE )
      return false;

   newcmd = command;
   newarg = argument;

   /*
    * Simple command interpreter menu. Nothing overly fancy etc, but it beats trying to tie directly into the mud's
    * * own internal structures. Especially with the differences in codebases.
    */
   list<imc_command_table*>::iterator com;
   for( com = imc_commandlist.begin(  ); com != imc_commandlist.end(  ); ++com )
   {
      cmd = (*com);

      if( IMCPERM( ch ) < cmd->level )
         continue;

      list<string>::iterator ials;
      for( ials = cmd->aliaslist.begin(  ); ials != cmd->aliaslist.end(  ); ++ials )
      {
         string als = (*ials);

         if( scomp( newcmd, als ) )
         {
            newcmd = cmd->name;
            break;
         }
      }

      if( scomp( newcmd, cmd->name ) )
      {
         if( cmd->connected == true && this_imcmud->state < IMC_ONLINE )
         {
            imc_to_char( "The mud is not currently connected to IMC2.\r\n", ch );
            return true;
         }

         if( cmd->function == NULL )
         {
            imc_to_char( "That command has no code set. Inform the administration.\r\n", ch );
            imcbug( "%s: Command %s has no code set!", __FUNCTION__, cmd->name.c_str(  ) );
            return true;
         }

         try
         {
            ( *cmd->function ) ( ch, newarg );
         }
         catch( exception &e )
         {
            imclog( "Command Exception: '%s' on command: %s %s", e.what(), cmd->name.c_str(), newarg.c_str() );
         }
         catch( ... )
         {
            imclog( "Unknown command exception on command: %s %s", cmd->name.c_str(), newarg.c_str() );
         }
         return true;
      }
   }

   /*
    * Assumed to be aiming for a channel if you get this far down 
    */
   c = imc_findchannel( newcmd );

   if( !c || c->level > IMCPERM( ch ) )
      return false;

   if( imc_hasname( IMC_DENY( ch ), c->local_name.c_str(  ) ) )
   {
      imc_printf( ch, "You have been denied the use of %s by the administration.\r\n", c->local_name.c_str(  ) );
      return true;
   }

   if( !c->refreshed )
   {
      imc_printf( ch, "The %s channel has not yet been refreshed by the server.\r\n", c->local_name.c_str(  ) );
      return true;
   }

   if( newarg.empty(  ) )
   {
      int y;

      imc_printf( ch, "~cThe last %d %s messages:\r\n", MAX_IMCHISTORY, c->local_name.c_str(  ) );
      for( y = 0; y < MAX_IMCHISTORY; ++y )
      {
         if( !c->history[y].empty(  ) )
            imc_printf( ch, "%s\r\n", c->history[y].c_str(  ) );
         else
            break;
      }
      return true;
   }

   if( IMCPERM( ch ) >= IMCPERM_ADMIN && scomp( newarg, "log" ) )
   {
      if( !IMCIS_SET( c->flags, IMCCHAN_LOG ) )
      {
         IMCSET_BIT( c->flags, IMCCHAN_LOG );
         imc_printf( ch, "~RFile logging enabled for %s, PLEASE don't forget to undo this when it isn't needed!\r\n",
                     c->local_name.c_str(  ) );
      }
      else
      {
         IMCREMOVE_BIT( c->flags, IMCCHAN_LOG );
         imc_printf( ch, "~GFile logging disabled for %s.\r\n", c->local_name.c_str(  ) );
      }
      imc_save_channels(  );
      return true;
   }

   if( !imc_hasname( IMC_LISTEN( ch ), c->local_name ) )
   {
      imc_printf( ch, "You are not currently listening to %s. Use the imclisten command to listen to this channel.\r\n",
                  c->local_name.c_str(  ) );
      return true;
   }

   switch ( newarg[0] )
   {
      case ',':
      {
         /*
          * Strip the , and then extra spaces - Remcon 6-28-03 
          */
         newarg = newarg.substr( 1, newarg.length(  ) );
         strip_lspace( newarg );
         imc_sendmessage( c, CH_IMCNAME( ch ), color_mtoi( newarg ), 1 );
      }
         break;

      case '@':
      {
         /*
          * Strip the @ and then extra spaces - Remcon 6-28-03 
          */
         newarg = newarg.substr( 1, newarg.length(  ) );
         strip_lspace( newarg );
         p = imc_send_social( ch, newarg, 0 );
         if( p.empty(  ) )
            return true;
         imc_sendmessage( c, CH_IMCNAME( ch ), p, 2 );
      }
         break;

      default:
         imc_sendmessage( c, CH_IMCNAME( ch ), color_mtoi( newarg ), 0 );
         break;
   }
   return true;
}
