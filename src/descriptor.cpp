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
 *                       Descriptor Support Functions                       *
 ****************************************************************************/

#include <fstream>
#include <fcntl.h>
#if defined(WIN32)
#include <unistd.h>
#include <winsock2.h>
#define  TELOPT_ECHO        '\x01'
#define  GA                 '\xF9'
#define  SB                 '\xFA'
#define  SE                 '\xF0'
#define  WILL               '\xFB'
#define  WONT               '\xFC'
#define  DO                 '\xFD'
#define  DONT               '\xFE'
#define  IAC                '\xFF'
#define EWOULDBLOCK WSAEWOULDBLOCK
#else
#include <sys/socket.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <arpa/telnet.h>
#endif
#if defined(__FreeBSD__)
#include <sys/types.h>
#include <csignal>
#endif
#include <cstdarg>
#include <cerrno>
#include "mud.h"
#include "auction.h"
#include "channels.h"
#include "commands.h"
#include "connhist.h"
#include "descriptor.h"
#include "fight.h"
#ifdef IMC
#include "imc.h"
#endif
#include "md5.h"
#include "new_auth.h"
#include "raceclass.h"
#include "roomindex.h"
#include "sha256.h"
#ifdef MULTIPORT
#include "shell.h"
#endif

int maxdesc, newdesc;
list < descriptor_data * >dlist;

extern const char *alarm_section;
extern fd_set in_set;
extern fd_set out_set;
extern fd_set exc_set;
extern bool bootlock;
extern int crash_count;
extern int num_logins;
#ifdef MULTIPORT
extern bool compilelock;
#endif

void send_mssp_data( descriptor_data * );
void set_alarm( long );
auth_data *get_auth_name( const string & );
void save_sysdata(  );
void save_auth_list(  );
void check_holiday( char_data * );
void update_connhistory( descriptor_data *, int ); /* connhist.c */
void check_auth_state( char_data * );
void oedit_parse( descriptor_data *, string & );
void medit_parse( descriptor_data *, string & );
void redit_parse( descriptor_data *, string & );
void board_parse( descriptor_data *, const string & );
bool check_immortal_domain( char_data *, const string & );
void scan_rares( char_data * );
void break_camp( char_data * );
void setup_newbie( char_data *, bool );
void sale_count( char_data * );  /* For new auction system - Samson 6-24-99 */
void check_clan_info( char_data * );
void name_stamp_stats( char_data * );
void quotes( char_data * );
void reset_colors( char_data * );
// bool check_bans( char_data * );
CMDF( do_help );
CMDF( do_destroy );

/* Terminal detection stuff start */
#define  IS                 '\x00'
#define  TERMINAL_TYPE      '\x18'
#define  SEND	          '\x01'

const char term_call_back_str[] = { ( char )IAC, SB, TERMINAL_TYPE, IS };
const char req_termtype_str[] = { ( char )IAC, SB, TERMINAL_TYPE, SEND, ( char )IAC, SE, '\0' };
const char do_term_type[] = { ( char )IAC, DO, TERMINAL_TYPE, '\0' };

/* Terminal detection stuff end */

const char echo_off_str[] = { ( char )IAC, ( char )WILL, TELOPT_ECHO, '\0' };
const char echo_on_str[] = { ( char )IAC, ( char )WONT, TELOPT_ECHO, '\0' };
const char go_ahead_str[] = { ( char )IAC, ( char )GA, '\0' };

extern char will_msp_str[];

long tr_saved;
long tr_in;
long tr_out;

/* Update the info in transfer input and output */
/* I think its a good idea to know the bandwidth and
 * how much we are saving because of mccp :)
 */
/* types:
 *    1 = in_transfer
 *    2 = out_transfer
 *    3 = saved_transfer - How much got saved because of mccp
 */
void update_trdata( int type, int size )
{
   /*
    * If size is 0 what's the point?
    */
   if( size == 0 )
      return;

   /*
    * If not a valid type return 
    */
   if( type < 1 || type > 3 )
      return;

   if( type == 1 )
      tr_in += size;
   else if( type == 2 )
      tr_out += size;
   else if( type == 3 )
      tr_saved += size;
}

/* Clear both transfers */
void clear_trdata( void )
{
   tr_in = 0;
   tr_out = 0;
   tr_saved = 0;
}

CMDF( do_trdata )
{
   /*
    * Display the transfer data 
    */
   if( argument.empty(  ) )
   {
      ch->print( "Type &Ytrdata clear&d to reset this data.\r\n\r\n" );

      ch->printf( "Bytes received     : %d\r\n", tr_in );
      ch->printf( "Bytes sent         : %d\r\n", tr_out );
      ch->printf( "Bytes saved by MCCP: %d\r\n", tr_saved );
   }
   /*
    * Just so we can start it over 
    */
   if( !str_cmp( argument, "clear" ) )
      clear_trdata(  );
}

#define DNS_FILE SYSTEM_DIR "dns.dat"

struct dns_data
{
   dns_data(  );
   ~dns_data(  );

   string ip;
   string name;
   time_t time;
};

list < dns_data * >dnslist;

descriptor_data::~descriptor_data(  )
{
   if( can_compress && is_compressing )
   {
      if( !compressEnd(  ) )
         log_printf( "Error stopping compression on desc %d", descriptor );
   }
   deleteptr( mccp );

   if( descriptor > 0 )
      close( descriptor );
   else if( descriptor == 0 )
      bug( "%s: }RALERT! Closing socket 0! BAD BAD BAD!", __FUNCTION__ );
}

descriptor_data::descriptor_data(  )
{
   init_memory( &snoop_by, &is_compressing, sizeof( is_compressing ) );
   host.clear(  );
   outbuf.clear(  );
   pagebuf.clear(  );
   incomm.clear(  );
   inlast.clear(  );
   client.clear(  );
}

mccp_data::mccp_data(  )
{
   init_memory( &out_compress, &out_compress_buf, sizeof( out_compress_buf ) );
}

void free_all_descs( void )
{
   list < descriptor_data * >::iterator ds;

   for( ds = dlist.begin(  ); ds != dlist.end(  ); )
   {
      descriptor_data *d = *ds;
      ++ds;

      dlist.remove( d );
      deleteptr( d );
   }
}

void descriptor_data::init(  )
{
   descriptor = -1;
   idle = 0;
   repeat = 0;
   connected = CON_GET_NAME;
   prevcolor = 0x08;
#if !defined(WIN32)
   ifd = -1;   /* Descriptor pipes, used for DNS resolution and such */
   ipid = -1;
#endif
   client = "Unidentified";   // Terminal detect
   msp_detected = false;
   can_compress = false;
   is_compressing = false;
   outbuf.clear(  );
   pagebuf.clear(  );
   pageindex = 0;
   mccp = new mccp_data;
}

/*Prompt, fprompt made to include exp and victim's condition, prettyfied too - Adjani, 12-07-2002*/
char *default_fprompt( char_data * ch )
{
   static char buf[60];

   mudstrlcpy( buf, "&z[&W%h&whp ", 60 );
   mudstrlcat( buf, "&W%m&wm", 60 );
   mudstrlcat( buf, " &W%v&wmv&z] ", 60 );
   mudstrlcat( buf, " [&R%c&z] ", 60 );
   if( ch->isnpc(  ) || ch->is_immortal(  ) )
      mudstrlcat( buf, "&W%i%R&D", 60 );
   return buf;
}

char *default_prompt( char_data * ch )
{
   static char buf[60];

   mudstrlcpy( buf, "&z[&W%h&whp ", 60 );
   mudstrlcat( buf, "&W%m&wm", 60 );
   mudstrlcat( buf, " &W%v&wmv&z] ", 60 );
   mudstrlcat( buf, " [&c%X&wexp&z]&D ", 60 );
   if( ch->isnpc(  ) || ch->is_immortal(  ) )
      mudstrlcat( buf, "&W%i%R&D", 60 );
   return buf;
}

bool check_bad_desc( int desc )
{
   if( FD_ISSET( desc, &exc_set ) )
   {
      FD_CLR( desc, &in_set );
      FD_CLR( desc, &out_set );
      log_string( "Bad FD caught and disposed." );
      return true;
   }
   return false;
}

/*
* This is the MCCP version. Use write_to_descriptor_old to send non-compressed
* text.
* Updated to run with the block checks by Orion... if it doesn't work, blame
* him.;P -Orion
*/
bool descriptor_data::write( const char *txt )
{
   int nWrite = 0, nBlock, iErr;
   size_t iStart = 0, len, mccpsaved, length = strlen( txt );

   mccpsaved = length;
   /*
    * Won't send more then it has to so make sure we check if its under length 
    */
   if( mccpsaved > strlen( txt ) )
      mccpsaved = strlen( txt );

   if( this && mccp->out_compress )
   {
      mccp->out_compress->next_in = ( unsigned char * )txt;
      mccp->out_compress->avail_in = length;

      while( mccp->out_compress->avail_in )
      {
         mccp->out_compress->avail_out = COMPRESS_BUF_SIZE - ( mccp->out_compress->next_out - mccp->out_compress_buf );

         if( mccp->out_compress->avail_out )
         {
            int status = deflate( mccp->out_compress, Z_SYNC_FLUSH );

            if( status != Z_OK )
               return false;
         }

         len = mccp->out_compress->next_out - mccp->out_compress_buf;
         if( len > 0 )
         {
            for( iStart = 0; iStart < len; iStart += nWrite )
            {
               nBlock = UMIN( len - iStart, 4096 );
#if defined(WIN32)
               nWrite = send( descriptor, ( const char * )( mccp->out_compress_buf + iStart ), nBlock, 0 );
#else
               nWrite = send( descriptor, mccp->out_compress_buf + iStart, nBlock, 0 );
#endif
               if( nWrite > 0 )
               {
                  update_trdata( 2, nWrite );
                  mccpsaved -= nWrite;
               }
               if( nWrite == -1 )
               {
                  iErr = errno;
                  if( iErr == EWOULDBLOCK )
                  {
                     nWrite = 0;
                     continue;
                  }
                  else
                  {
                     perror( "Write_to_descriptor" );
                     return false;
                  }
               }
               if( !nWrite )
                  break;
            }
            if( !iStart )
               break;

            if( iStart < len )
               memmove( mccp->out_compress_buf, mccp->out_compress_buf + iStart, len - iStart );

            mccp->out_compress->next_out = mccp->out_compress_buf + len - iStart;
         }
      }
      if( mccpsaved > 0 )
         update_trdata( 3, mccpsaved );
      return true;
   }

   for( iStart = 0; iStart < length; iStart += nWrite )
   {
      nBlock = UMIN( length - iStart, 4096 );
      nWrite = send( descriptor, txt + iStart, nBlock, 0 );

      if( nWrite > 0 )
         update_trdata( 2, nWrite );

      if( nWrite == -1 )
      {
         iErr = errno;
         if( iErr == EWOULDBLOCK )
         {
            nWrite = 0;
            continue;
         }
         else
         {
            perror( "Write_to_descriptor" );
            return false;
         }
      }
   }
   return true;
}

/*
 *
 * Added block checking to prevent random booting of the descriptor. Thanks go
 * out to Rustry for his suggestions. -Orion
 */
bool write_to_descriptor_old( int desc, const char *txt )
{
   size_t iStart = 0, length = strlen( txt );
   int nWrite = 0, nBlock = 0, iErr = 0;

   for( iStart = 0; iStart < length; iStart += nWrite )
   {
      nBlock = UMIN( length - iStart, 4096 );
      nWrite = send( desc, txt + iStart, nBlock, 0 );

      if( nWrite > 0 )
         update_trdata( 2, nWrite );

      if( nWrite == -1 )
      {
         iErr = errno;
         if( iErr == EWOULDBLOCK )
         {
            nWrite = 0;
            continue;
         }
         else
         {
            perror( "Write_to_descriptor" );
            return false;
         }
      }
   }
   return true;
}

bool descriptor_data::read(  )
{
   size_t iStart, iErr;

   /*
    * Hold horses if pending command already. 
    */
   if( !this->incomm.empty(  ) )
      return true;

   /*
    * Check for overflow. 
    */
   iStart = strlen( this->inbuf );
   if( iStart >= sizeof( this->inbuf ) - 10 )
   {
      log_printf( "%s input overflow!", this->host.c_str(  ) );
      this->write( "\r\n*** PUT A LID ON IT!!! ***\r\n" );
      return false;
   }

   for( ;; )
   {
      int nRead;

      nRead = recv( this->descriptor, this->inbuf + iStart, sizeof( this->inbuf ) - 10 - iStart, 0 );
      iErr = errno;
      if( nRead > 0 )
      {
         iStart += nRead;

         update_trdata( 1, nRead );
         if( this->inbuf[iStart - 1] == '\n' || this->inbuf[iStart - 1] == '\r' )
            break;
      }
      else if( nRead == 0 && this->connected >= CON_PLAYING )
      {
         log_string_plus( LOG_COMM, LEVEL_IMMORTAL, "EOF encountered on read." );
         return false;
      }
      else if( iErr == EWOULDBLOCK )
         break;
      else
      {
         log_printf_plus( LOG_COMM, LEVEL_IMMORTAL, "%s: Descriptor error on #%d", __FUNCTION__, this->descriptor );
         log_printf_plus( LOG_COMM, LEVEL_IMMORTAL, "Descriptor belongs to: %s", ( this->character && this->character->name ) ? this->character->name : this->host.c_str(  ) );
         perror( __FUNCTION__ );
         return false;
      }
   }
   this->inbuf[iStart] = '\0';
   return true;
}

/*
 * Low level output function.
 */
bool descriptor_data::flush_buffer( bool fPrompt )
{
   /*
    * If buffer has more than 4K inside, spit out .5K at a time   -Thoric
    *
    * Samson says: Lets do 4K chunks!
    */
   if( !mud_down && this->outbuf.length(  ) > 4096 )
   {
      char buf[4096];

      memcpy( buf, this->outbuf.c_str(  ), 4096 );
      this->outbuf = this->outbuf.substr( 4097, this->outbuf.length(  ) - 4097 );
      if( snoop_by )
      {
         buf[4095] = '\0';
         if( character && character->name )
         {
            if( original && original->name )
               snoop_by->buffer_printf( "%s (%s)", character->name, original->name );
            else
               snoop_by->write_to_buffer( character->name );
         }
         snoop_by->write_to_buffer( "% " );
         snoop_by->write_to_buffer( buf );
      }
      if( !this->write( buf ) )
         return false;
      return true;
   }

   /*
    * Bust a prompt.
    */
   if( fPrompt && !mud_down && connected == CON_PLAYING )
   {
      char_data *ch;

      ch = original ? original : character;
      if( ch->has_pcflag( PCFLAG_BLANK ) )
         write_to_buffer( "\r\n" );

      if( !ch->isnpc(  ) )
         prompt(  );
      else
         write_to_buffer( ANSI_RESET );

      if( ch->has_pcflag( PCFLAG_TELNET_GA ) )
         write_to_buffer( go_ahead_str );
   }

   /*
    * Short-circuit if nothing to write.
    */
   if( this->outbuf.empty(  ) )
      return true;

   /*
    * Snoop-o-rama.
    */
   if( snoop_by )
   {
      /*
       * without check, 'force mortal quit' while snooped caused crash, -h 
       */
      if( character && character->name )
      {
         /*
          * Show original snooped names. -- Altrag 
          */
         if( original && original->name )
            snoop_by->buffer_printf( "%s (%s)", character->name, original->name );
         else
            snoop_by->write_to_buffer( character->name );
      }
      snoop_by->write_to_buffer( "% " );
      snoop_by->write_to_buffer( outbuf );
   }

   /*
    * OS-dependent output.
    */
   if( !this->write( this->outbuf.c_str(  ) ) )
   {
      this->outbuf.erase(  );
      return false;
   }
   this->outbuf.erase(  );
   return true;
}

/*
 * Transfer one line from input buffer to input line.
 */
void descriptor_data::read_from_buffer(  )
{
   affect_data af;   /* Spamguard abusers beware! Muahahahahah! */
   int i, j, k, iac = 0;
   unsigned char *p;

   /*
    * Hold horses if pending command already.
    */
   if( !this->incomm.empty(  ) )
      return;

   /*
    * Thanks Nick!
    */
   for( p = ( unsigned char * )inbuf; *p; ++p )
   {
      if( *p == IAC )
      {
         if( memcmp( p, term_call_back_str, sizeof( term_call_back_str ) ) == 0 )
         {
            int pos = ( char * )p - inbuf;   /* where we are in buffer */
            int len = sizeof( inbuf ) - pos - sizeof( term_call_back_str );   /* how much to go */
            char tmp[100];
            size_t x = 0;
            unsigned char *oldp = p;

            p += sizeof( term_call_back_str );  /* skip TERMINAL_TYPE / IS characters */

            for( x = 0; x < ( sizeof( tmp ) - 1 ) && *p != 0   /* null marks end of buffer */
                 && *p != IAC;   /* should terminate with IAC */
                 ++x, ++p )
               tmp[x] = *p;

            tmp[x] = '\0';
            if( tmp[0] != '\0' )
               this->client = tmp;

            p += 2;  /* skip IAC and SE */
            len -= strlen( tmp ) + 2;
            if( len < 0 )
               len = 0;

            /*
             * remove string from input buffer 
             */
            memmove( oldp, p, len );
         }  /* end of getting terminal type */
      }  /* end of finding an IAC */
   }

   /*
    * Look for at least one new line.
    */
   for( i = 0; this->inbuf[i] != '\n' && this->inbuf[i] != '\r' && i < MAX_INBUF_SIZE; ++i )
   {
      if( this->inbuf[i] == '\0' )
         return;
   }

   /*
    * Canonical input processing.
    */
   for( i = 0, k = 0; this->inbuf[i] != '\n' && this->inbuf[i] != '\r'; ++i )
   {
      if( k >= MIL / 2 )   /* Reasonable enough to allow unless someone floods it with color tags */
      {
         this->write( "Line too long.\r\n" );
         this->inbuf[i] = '\n';
         this->inbuf[i + 1] = '\0';
         break;
      }

      if( this->can_compress && !this->is_compressing )
         this->compressStart(  );

      if( this->inbuf[i] == ( signed char )IAC )
         iac = 1;
      else if( iac == 1 && ( this->inbuf[i] == ( signed char )DO || this->inbuf[i] == ( signed char )DONT || this->inbuf[i] == ( signed char )WILL ) )
         iac = 2;
      else if( iac == 2 )
      {
         iac = 0;
         if( this->inbuf[i] == ( signed char )TELOPT_COMPRESS2 )
         {
            if( this->inbuf[i - 1] == ( signed char )DO )
            {
               if( this->compressStart(  ) )
                  this->can_compress = true;
            }
            else if( this->inbuf[i - 1] == ( signed char )DONT )
            {
               if( this->compressEnd(  ) )
                  this->can_compress = false;
            }
         }
         else if( this->inbuf[i] == ( signed char )TELOPT_MSP )
         {
            if( this->inbuf[i - 1] == ( signed char )DO )
            {
               this->msp_detected = true;
               this->send_msp_startup(  );
            }
            else if( this->inbuf[i - 1] == ( signed char )DONT )
               this->msp_detected = false;
         }
         else if( this->inbuf[i] == ( signed char )TERMINAL_TYPE )
         {
            if( this->inbuf[i - 1] == ( signed char )WILL )
               this->write_to_buffer( req_termtype_str );
         }
      }
      else if( this->inbuf[i] == '\b' && k > 0 )
         --k;
      /*
       * Note to the future curious: Leave this alone. Extended ascii isn't standardized yet.
       * * You'd think being the 21st century and all that this wouldn't be the case, but you can
       * * thank the bastards in Redmond for this.
       */
      else if( isascii( this->inbuf[i] ) && isprint( this->inbuf[i] ) )
      {
         this->incomm.append( 1, this->inbuf[i] );
         ++k;
      }
   }

   /*
    * Finish off the line.
    */
   if( k == 0 )
   {
      this->incomm.append( 1, ' ' );
      ++k;
   }

   /*
    * Deal with bozos with #repeat 1000 ...
    */
   if( k > 1 || this->incomm[0] == '!' )
   {
      if( this->incomm[0] != '!' && str_cmp( this->incomm, this->inlast ) )
         this->repeat = 0;
      else
      {
         /*
          * What this is SUPPOSED to do is make sure the command or alias being used isn't a public channel.
          * * As we know, code rarely does what we expect, and there could still be problems here.
          * * The only other solution seen as viable beyond this is to remove the spamguard entirely.
          */
         cmd_type *cmd = NULL;
         mud_channel *channel = NULL;
         map < string, string >::iterator al;
         string c = this->incomm, arg;

         c = one_argument( c, arg );
         cmd = find_command( arg );
         if( this->character )
            al = character->pcdata->alias_map.find( arg );

         if( !cmd && this->character && al != this->character->pcdata->alias_map.end(  ) )
         {
            if( !al->second.empty(  ) )
            {
               string d = al->second, arg2;

               d = one_argument( d, arg2 );
               cmd = find_command( arg2 );
            }
         }

         if( !cmd )
         {
            if( ( channel = find_channel( arg ) ) != NULL && !str_cmp( this->incomm, this->inlast ) )
               ++this->repeat;
         }
         else if( cmd->flags.test( CMD_NOSPAM ) && !str_cmp( this->incomm, this->inlast ) )
            ++this->repeat;
#ifdef IMC
         {
            imc_channel *imcchan;

            if( ( imcchan = imc_findchannel( arg ) ) != NULL && !str_cmp( this->incomm, this->inlast ) )
               ++this->repeat;
         }
#endif
         if( this->repeat == 3 && this->character && this->character->level < LEVEL_IMMORTAL )
            this->character->print( "}R\r\nYou have repeated the same command 3 times now.\r\nRepeating it 7 more will result in an autofreeze by the spamguard code.&D\r\n" );

         if( this->repeat == 6 && this->character && this->character->level < LEVEL_IMMORTAL )
            this->character->print( "}R\r\nYou have repeated the same command 6 times now.\r\nRepeating it 4 more will result in an autofreeze by the spamguard code.&D\r\n" );

         if( this->repeat >= 10 && this->character && this->character->level < LEVEL_IMMORTAL )
         {
            ++this->character->pcdata->spam;
            log_printf( "%s was autofrozen by the spamguard - spamming: %s", this->character->name, this->incomm.c_str(  ) );
            log_printf( "%s has spammed %d times this login.", this->character->name, this->character->pcdata->spam );

            this->write( "\r\n*** PUT A LID ON IT!!! ***\r\nYou cannot enter the same command more than 10 consecutive times!\r\n" );
            this->write( "The Spamguard has spoken!\r\n" );
            this->write( "It suddenly becomes very cold, and very QUIET!\r\n" );
            this->incomm = "spam";

            af.type = skill_lookup( "spamguard" );
            af.duration = 115 * this->character->pcdata->spam; /* One game hour per offense, this can get ugly FAST */
            af.modifier = 0;
            af.location = APPLY_NONE;
            af.bit = AFF_SPAMGUARD;
            this->character->affect_to_char( &af );
            this->character->set_pcflag( PCFLAG_IDLING );
            this->repeat = 0; /* Just so it doesn't get haywire */
         }
      }
   }

   /*
    * Do '!' substitution.
    */
   if( this->incomm[0] == '!' )
      this->incomm = this->inlast;
   else
      this->inlast = this->incomm;

   /*
    * Shift the input buffer.
    */
   while( this->inbuf[i] == '\n' || this->inbuf[i] == '\r' )
      ++i;
   for( j = 0; ( this->inbuf[j] = this->inbuf[i + j] ) != '\0'; ++j )
      ;
}

/*
 * Append onto an output buffer.
 */
void descriptor_data::write_to_buffer( const string & txt )
{
   if( !this )
   {
      bug( "%s: NULL descriptor", __FUNCTION__ );
      return;
   }

   /*
    * Normally a bug... but can happen if loadup is used.
    */
   if( this->outbuf.empty(  ) && !this->fcommand )
      this->outbuf = "\r\n";

   this->outbuf.append( txt );
}

void descriptor_data::buffer_printf( const char *fmt, ... )
{
   char buf[MSL * 4];
   va_list args;

   va_start( args, fmt );
   vsnprintf( buf, MSL * 4, fmt, args );
   va_end( args );

   this->write_to_buffer( buf );
}

/* Writes to a descriptor, usually best used when there's no character to send to ( like logins ) */
void descriptor_data::send_color( const string & txt )
{
   if( !this )
   {
      bug( "%s: NULL *d", __FUNCTION__ );
      return;
   }

   if( txt.empty(  ) || !this->descriptor )
      return;

   this->write_to_buffer( colorize( txt, this ) );
}

void descriptor_data::pager( const string & txt )
{
   if( !this )
   {
      bug( "%s: NULL *d", __FUNCTION__ );
      return;
   }

   if( this->pagebuf.empty(  ) && !this->fcommand )
      this->pagebuf = "\r\n";

   this->pagebuf.append( txt );
}

void descriptor_data::set_pager_input( const string & argument )
{
   string arg = argument;

   strip_lspace( arg );
   this->pagecmd = arg[0];
}

bool descriptor_data::pager_output(  )
{
   char_data *ch;
   int pclines, start, end;
   register int lines;
   bool ret;

   if( !this || this->pagebuf.empty(  ) || this->pagecmd == -1 )
      return true;
   ch = this->original ? this->original : this->character;
   pclines = UMAX( ch->pcdata->pagerlen, 5 ) - 1;

   switch ( LOWER( this->pagecmd ) )
   {
      default:
         lines = 0;
         break;
      case 'b':
         lines = -1 - ( pclines * 2 );
         break;
      case 'r':
         lines = -1 - pclines;
         break;
      case 'n':
         lines = 0;
         pclines = 0x7FFFFFFF;   /* As many lines as possible */
         break;
      case 'q':
         this->flush_buffer( true );
         this->pagebuf.clear(  );
         this->pageindex = 0;
         return true;
   }

   // This is going to get seriously messed up if people use the wrong line termination.
   vector < string > pagelines = string_explode( this->pagebuf, '\n' );

   if( lines < 0 )
      start = this->pageindex + lines;
   else
      start = this->pageindex;
   end = start + pclines;
   for( int x = start; x < end; ++x )
   {
      if( x < 0 )
         continue;

      if( ( size_t ) x >= pagelines.size(  ) )
      {
         this->flush_buffer( true );
         this->pagebuf.clear(  );
         this->pageindex = 0;
         return true;
      }

      this->write( pagelines[x].c_str(  ) );
      this->pageindex = x;
   }

   this->pagecmd = -1;
   if( ch->has_pcflag( PCFLAG_ANSI ) )
      if( !this->write( ANSI_LBLUE ) )
         return false;

   if( ( ret = this->write( "(C)ontinue, (N)on-stop, (R)efresh, (B)ack, (Q)uit: [C] " ) ) == false )
      return false;

   /*
    * Telnet GA bit here suggested by Garil 
    */
   if( ch->has_pcflag( PCFLAG_TELNET_GA ) )
      this->write_to_buffer( go_ahead_str );
   if( ch->has_pcflag( PCFLAG_ANSI ) )
   {
      char buf[32];

      snprintf( buf, 32, "%s", ch->color_str( this->pagecolor ) );
      ret = this->write( buf );
   }
   return ret;
}

void descriptor_data::send_greeting(  )
{
   FILE *rpfile;
   int num = 0;
   char BUFF[MSL], filename[256];

   snprintf( filename, 256, "%sgreeting.dat", MOTD_DIR );
   if( ( rpfile = fopen( filename, "r" ) ) != NULL )
   {
      while( ( ( BUFF[num] = fgetc( rpfile ) ) != EOF ) && num < MSL - 2 )
         ++num;
      FCLOSE( rpfile );
      BUFF[num] = '\0';
      send_color( BUFF );
   }
}

void descriptor_data::show_title(  )
{
   char_data *ch;

   ch = character;

   if( !ch->has_pcflag( PCFLAG_NOINTRO ) )
      show_file( ch, ANSITITLE_FILE );
   else
      write_to_buffer( "Press enter...\r\n" );
   connected = CON_PRESS_ENTER;
}

void descriptor_data::show_stats( char_data * ch )
{
   ch->set_color( AT_SKILL );
   buffer_printf( "\r\n1. Str: %d\r\n", ch->perm_str );
   buffer_printf( "2. Int: %d\r\n", ch->perm_int );
   buffer_printf( "3. Wis: %d\r\n", ch->perm_wis );
   buffer_printf( "4. Dex: %d\r\n", ch->perm_dex );
   buffer_printf( "5. Con: %d\r\n", ch->perm_con );
   buffer_printf( "6. Cha: %d\r\n", ch->perm_cha );
   buffer_printf( "7. Lck: %d\r\n\r\n", ch->perm_lck );
   ch->set_color( AT_ACTION );
   write_to_buffer( "Choose an attribute to raise by +1\r\n" );
}

dns_data::dns_data(  )
{
   time = 0;
}

dns_data::~dns_data(  )
{
   dnslist.remove( this );
}

void free_dns_list( void )
{
   list < dns_data * >::iterator dns;

   for( dns = dnslist.begin(  ); dns != dnslist.end(  ); )
   {
      dns_data *ip = *dns;
      ++dns;

      deleteptr( ip );
   }
}

void save_dns( void )
{
   list < dns_data * >::iterator dns;
   ofstream stream;

   stream.open( DNS_FILE );
   if( !stream.is_open(  ) )
   {
      bug( "%s: stream.open()", __FUNCTION__ );
      perror( DNS_FILE );
   }
   else
   {
      for( dns = dnslist.begin(  ); dns != dnslist.end(  ); ++dns )
      {
         dns_data *ip = *dns;

         stream << "#CACHE" << endl;
         stream << "IP   " << ip->ip << endl;
         stream << "Name " << ip->name << endl;
         stream << "Time " << ip->time << endl;
         stream << "End" << endl << endl;
      }
      stream.close(  );
   }
}

void prune_dns( void )
{
   list < dns_data * >::iterator dns;

   for( dns = dnslist.begin(  ); dns != dnslist.end(  ); )
   {
      dns_data *cache = *dns;
      ++dns;

      /*
       * Stay in cache for 14 days 
       */
      if( current_time - cache->time >= 1209600 || !str_cmp( cache->ip, "Unknown??" ) || !str_cmp( cache->name, "Unknown??" ) )
         deleteptr( cache );
   }
   save_dns(  );
}

void add_dns( const string & dhost, const string & address )
{
   dns_data *cache;

   cache = new dns_data;
   cache->ip = dhost;
   cache->name = address;
   cache->time = current_time;
   dnslist.push_back( cache );

   save_dns(  );
}

string in_dns_cache( const string & ip )
{
   list < dns_data * >::iterator idns;

   for( idns = dnslist.begin(  ); idns != dnslist.end(  ); ++idns )
   {
      dns_data *dns = *idns;

      if( !str_cmp( ip, dns->ip ) )
         return dns->name;
   }
   return "";
}

void load_dns( void )
{
   dns_data *cache = NULL;
   ifstream stream;

   dnslist.clear(  );

   stream.open( DNS_FILE );
   if( stream.is_open(  ) )
   {
      do
      {
         string key, value;
         char buf[MIL];

         stream >> key;
         stream.getline( buf, MIL );
         value = buf;

         strip_lspace( key );
         strip_lspace( value );
         strip_tilde( value );

         if( key.empty(  ) )
            continue;

         if( key == "#CACHE" )
            cache = new dns_data;
         else if( key == "IP" )
            cache->ip = value;
         else if( key == "Name" )
            cache->name = value;
         else if( key == "Time" )
            cache->time = atoi( value.c_str(  ) );
         else if( key == "End" )
         {
            if( cache->ip.empty(  ) )
               cache->ip = "Unknown??";
            if( cache->name.empty(  ) )
               cache->name = "Unknown??";

            dnslist.push_back( cache );
         }
         else
            log_printf( "%s: Bad line in DNS cache file: %s %s", __FUNCTION__, key.c_str(  ), value.c_str(  ) );
      }
      while( !stream.eof(  ) );
      stream.close(  );
   }
   prune_dns(  ); /* Clean out entries beyond 14 days */
}

/* DNS Resolver code by Trax of Forever's End */
/*
 * Almost the same as read_from_buffer...
 */
bool read_from_dns( int fd, char *buffer )
{
   static char inbuf[MSL * 2];
   size_t iStart, i, j, k;

   /*
    * Check for overflow. 
    */
   iStart = strlen( inbuf );
   if( iStart >= sizeof( inbuf ) - 10 )
   {
      bug( "%s: DNS input overflow!!!", __FUNCTION__ );
      return false;
   }

   /*
    * Snarf input. 
    */
   for( ;; )
   {
      int nRead;

      nRead = read( fd, inbuf + iStart, sizeof( inbuf ) - 10 - iStart );
      if( nRead > 0 )
      {
         iStart += nRead;
         if( inbuf[iStart - 2] == '\n' || inbuf[iStart - 2] == '\r' )
            break;
      }
      else if( nRead == 0 )
         return false;
      else if( errno == EWOULDBLOCK )
         break;
      else
      {
         perror( "Read_from_dns" );
         return false;
      }
   }

   inbuf[iStart] = '\0';

   /*
    * Look for at least one new line.
    */
   for( i = 0; inbuf[i] != '\n' && inbuf[i] != '\r'; ++i )
   {
      if( inbuf[i] == '\0' )
         return false;
   }

   /*
    * Canonical input processing.
    */
   for( i = 0, k = 0; inbuf[i] != '\n' && inbuf[i] != '\r'; ++i )
   {
      if( inbuf[i] == '\b' && k > 0 )
         --k;
      else if( isascii( inbuf[i] ) && isprint( inbuf[i] ) )
         buffer[k++] = inbuf[i];
   }

   /*
    * Finish off the line.
    */
   if( k == 0 )
      buffer[k++] = ' ';
   buffer[k] = '\0';

   /*
    * Shift the input buffer.
    */
   while( inbuf[i] == '\n' || inbuf[i] == '\r' )
      ++i;
   for( j = 0; ( inbuf[j] = inbuf[i + j] ) != '\0'; ++j )
      ;

   return true;
}

#if !defined(WIN32)
/* DNS Resolver code by Trax of Forever's End */
/*
 * Process input that we got from resolve_dns.
 */
void descriptor_data::process_dns(  )
{
   char address[MIL];
   int status;

   address[0] = '\0';

   if( !read_from_dns( ifd, address ) || address[0] == '\0' )
      return;

   if( address[0] != '\0' )
   {
      add_dns( host, address );  /* Add entry to DNS cache */
      host = address;
      if( character && character->pcdata )
         character->pcdata->lasthost = address;
   }

   /*
    * close descriptor and kill dns process 
    */
   if( ifd != -1 )
   {
      close( ifd );
      ifd = -1;
   }

   /*
    * we don't have to check here, 
    * cos the child is probably dead already. (but out of safety we do)
    * 
    * (later) I found this not to be true. The call to waitpid( ) is
    * necessary, because otherwise the child processes become zombie
    * and keep lingering around... The waitpid( ) removes them.
    */
   if( ipid != -1 )
   {
      waitpid( ipid, &status, 0 );
      ipid = -1;
   }
}

/* DNS Resolver hook. Code written by Trax of Forever's End */
void descriptor_data::resolve_dns( long ip )
{
   int fds[2];
   pid_t pid;

   /*
    * create pipe first 
    */
   if( pipe( fds ) != 0 )
   {
      perror( "resolve_dns: pipe: " );
      return;
   }

   if( dup2( fds[1], STDOUT_FILENO ) != STDOUT_FILENO )
   {
      perror( "resolve_dns: dup2(stdout): " );
      return;
   }

   if( ( pid = fork(  ) ) > 0 )
   {
      /*
       * parent process 
       */
      ifd = fds[0];
      ipid = pid;
      close( fds[1] );
   }
   else if( pid == 0 )
   {
      /*
       * child process 
       */
      char str_ip[64];
      int i;

      ifd = fds[0];
      ipid = pid;

      for( i = 2; i < 255; ++i )
         close( i );

      snprintf( str_ip, 64, "%ld", ip );
#if defined(__CYGWIN__)
      execl( "../src/resolver.exe", "AFKMud Resolver", str_ip, ( char * )NULL );
#else
      execl( "../src/resolver", "AFKMud Resolver", str_ip, ( char * )NULL );
#endif
      /*
       * Still here --> hmm. An error. 
       */
      bug( "%s: Exec failed; Closing child.", __FUNCTION__ );
      ifd = -1;
      ipid = -1;
      exit( 0 );
   }
   else
   {
      /*
       * error 
       */
      perror( "resolve_dns: failed fork" );
      close( fds[0] );
      close( fds[1] );
   }
}
#endif

void new_descriptor( int new_desc )
{
   descriptor_data *dnew;
   struct sockaddr_in sock;
   int desc;
#if defined(WIN32)
   ULONG r;
   int size;
#else
   int r;
   socklen_t size;
#endif

   size = sizeof( sock );
   if( check_bad_desc( new_desc ) )
   {
      set_alarm( 0 );
      return;
   }
   set_alarm( 20 );
   alarm_section = "new_descriptor: accept";
   if( ( desc = accept( new_desc, ( struct sockaddr * )&sock, &size ) ) < 0 )
   {
      perror( "New_descriptor: accept" );
      set_alarm( 0 );
      return;
   }
   if( check_bad_desc( new_desc ) )
   {
      set_alarm( 0 );
      return;
   }

   set_alarm( 20 );
   alarm_section = "new_descriptor: after accept";

#if defined(WIN32)
   r = 1;
   if( ioctlsocket( desc, FIONBIO, &r ) == SOCKET_ERROR )
   {
      perror( "New_descriptor: fcntl: O_NONBLOCK" );
      close( desc );
      return;
   }
#else
   r = fcntl( desc, F_GETFL, 0 );
   if( r < 0 || fcntl( desc, F_SETFL, O_NONBLOCK | O_NDELAY | r ) < 0 )
   {
      perror( "New_descriptor: fcntl: O_NONBLOCK" );
      close( desc );
      return;
   }
#endif

   if( check_bad_desc( new_desc ) )
      return;

   dnew = new descriptor_data;
   dnew->init(  );
   if( desc == 0 )
   {
      bug( "%s: }RALERT! Assigning socket 0! BAD BAD BAD! Host: %s", __FUNCTION__, inet_ntoa( sock.sin_addr ) );
      deleteptr( dnew );
      set_alarm( 0 );
      return;
   }
   dnew->descriptor = desc;
   dnew->client_port = ntohs( sock.sin_port );

   dnew->host = inet_ntoa( sock.sin_addr );

#if !defined(WIN32)
   if( !sysdata->NO_NAME_RESOLVING )
   {
      string buf = in_dns_cache( dnew->host );

      if( buf.empty(  ) )
         dnew->resolve_dns( sock.sin_addr.s_addr );
      else
         dnew->host = buf;
   }
#endif

/* FIXME: Write a new ban module so this code has meaning again.
   if( dnew->check_total_bans(  ) )
   {
      dnew->write( "Your site has been banned from this Mud.\r\n" );
      deleteptr( dnew );
      set_alarm( 0 );
      return;
   }
*/

   if( dlist.size(  ) < 1 )
      log_string( "Waking up autonomous update handlers." );

   dlist.push_back( dnew );

   /*
    * Terminal detect 
    */
   dnew->write_to_buffer( do_term_type );

   /*
    * MCCP Compression 
    */
   dnew->write_to_buffer( will_compress2_str );

   /*
    * Mud Sound Protocol 
    */
   dnew->write_to_buffer( will_msp_str );

   /*
    * Send the greeting. No longer handled kludgely by a global variable.
    */
   dnew->send_greeting(  );

   dnew->write_to_buffer( "Enter your character's name, or type new: " );

   if( ++num_descriptors > sysdata->maxplayers )
      sysdata->maxplayers = num_descriptors;
   if( sysdata->maxplayers > sysdata->alltimemax )
   {
      sysdata->time_of_max = c_time( current_time, -1 );
      sysdata->alltimemax = sysdata->maxplayers;
      log_printf_plus( LOG_INFO, LEVEL_IMMORTAL, "Broke all-time maximum player record: %d", sysdata->alltimemax );
      save_sysdata(  );
   }
   set_alarm( 0 );
}

void accept_new( int ctrl )
{
   static struct timeval null_time;

   /*
    * Poll all active descriptors.
    */
   FD_ZERO( &in_set );
   FD_ZERO( &out_set );
   FD_ZERO( &exc_set );
   FD_SET( ctrl, &in_set );

   maxdesc = ctrl;
   newdesc = 0;

   list < descriptor_data * >::iterator ds;
   for( ds = dlist.begin(  ); ds != dlist.end(  ); ++ds )
   {
      descriptor_data *d = *ds;

      maxdesc = UMAX( maxdesc, d->descriptor );
      FD_SET( d->descriptor, &in_set );
      FD_SET( d->descriptor, &out_set );
      FD_SET( d->descriptor, &exc_set );
#if !defined(WIN32)
      if( d->ifd != -1 && ( *ds )->ipid != -1 )
      {
         maxdesc = UMAX( maxdesc, d->ifd );
         FD_SET( d->ifd, &in_set );
      }
#endif
   }

   if( select( maxdesc + 1, &in_set, &out_set, &exc_set, &null_time ) < 0 )
   {
      perror( "accept_new: select: poll" );
      exit( 1 );
   }

   if( FD_ISSET( ctrl, &exc_set ) )
   {
      bug( "%s: Exception raised on controlling descriptor %d", __FUNCTION__, ctrl );
      FD_CLR( ctrl, &in_set );
      FD_CLR( ctrl, &out_set );
   }
   else if( FD_ISSET( ctrl, &in_set ) )
   {
      newdesc = ctrl;
      new_descriptor( newdesc );
   }
}

void descriptor_data::prompt(  )
{
   char_data *ch = character;
   char_data *och = ( original ? original : character );
   char_data *victim;
   bool ansi = ( och->has_pcflag( PCFLAG_ANSI ) );
   const char *cprompt;
   const char *helpstart = "&w[Type HELP START]";
   char buf[MSL];
   char *pbuf = buf;
   size_t pstat;
   int percent;

   if( !ch )
   {
      bug( "%s: NULL ch", __FUNCTION__ );
      return;
   }

   if( !ch->has_pcflag( PCFLAG_HELPSTART ) )
      cprompt = helpstart;

   else if( !ch->isnpc(  ) && ch->substate != SUB_NONE && ch->pcdata->subprompt && ch->pcdata->subprompt[0] != '\0' )
      cprompt = ch->pcdata->subprompt;

   else if( ch->isnpc(  ) || ( !ch->fighting && ( !ch->pcdata->prompt || !*ch->pcdata->prompt ) ) )
      cprompt = default_prompt( ch );

   else if( ch->fighting )
   {
      if( !ch->pcdata->fprompt || !*ch->pcdata->fprompt )
         cprompt = default_fprompt( ch );
      else
         cprompt = ch->pcdata->fprompt;
   }
   else
      cprompt = ch->pcdata->prompt;

   if( ansi )
   {
      mudstrlcpy( pbuf, ANSI_RESET, MSL - ( pbuf - buf ) );
      prevcolor = 0x08;
      pbuf += 4;
   }

   for( ; *cprompt; ++cprompt )
   {
      /*
       * '%' = prompt commands
       * Note: foreground changes will revert background to 0 (black)
       */
      if( *cprompt != '%' )
      {
         *( pbuf++ ) = *cprompt;
         continue;
      }

      ++cprompt;
      if( !*cprompt )
         break;
      if( *cprompt == *( cprompt - 1 ) )
      {
         *( pbuf++ ) = *cprompt;
         continue;
      }
      switch ( *( cprompt - 1 ) )
      {
         default:
            bug( "%s: bad command char '%c'.", __FUNCTION__, *( cprompt - 1 ) );
            break;

         case '%':
            *pbuf = '\0';
            pstat = 0x80000000;
            switch ( *cprompt )
            {
               default:
               case '%':
                  *pbuf++ = '%';
                  *pbuf = '\0';
                  break;

               case 'a':
                  if( ch->level >= 10 )
                     pstat = ch->alignment;
                  else if( ch->IS_GOOD(  ) )
                     mudstrlcpy( pbuf, "good", MSL - ( pbuf - buf ) );
                  else if( ch->IS_EVIL(  ) )
                     mudstrlcpy( pbuf, "evil", MSL - ( pbuf - buf ) );
                  else
                     mudstrlcpy( pbuf, "neutral", MSL - ( pbuf - buf ) );
                  break;

               case 'A':
                  snprintf( pbuf, MSL - ( pbuf - buf ), "%s%s%s", ch->has_aflag( AFF_INVISIBLE ) ? "I" : "",
                            ch->has_aflag( AFF_HIDE ) ? "H" : "", ch->has_aflag( AFF_SNEAK ) ? "S" : "" );
                  break;

               case 'C':  /* Tank */
                  if( !ch->fighting || ( victim = ch->fighting->who ) == NULL )
                     mudstrlcpy( pbuf, "N/A", MSL - ( pbuf - buf ) );
                  else if( !victim->fighting || ( victim = victim->fighting->who ) == NULL )
                     mudstrlcpy( pbuf, "N/A", MSL - ( pbuf - buf ) );
                  else
                  {
                     if( victim->max_hit > 0 )
                        percent = ( 100 * victim->hit ) / victim->max_hit;
                     else
                        percent = -1;
                     if( percent >= 100 )
                        mudstrlcpy( pbuf, "perfect health", MSL - ( pbuf - buf ) );
                     else if( percent >= 90 )
                        mudstrlcpy( pbuf, "slightly scratched", MSL - ( pbuf - buf ) );
                     else if( percent >= 80 )
                        mudstrlcpy( pbuf, "few bruises", MSL - ( pbuf - buf ) );
                     else if( percent >= 70 )
                        mudstrlcpy( pbuf, "some cuts", MSL - ( pbuf - buf ) );
                     else if( percent >= 60 )
                        mudstrlcpy( pbuf, "several wounds", MSL - ( pbuf - buf ) );
                     else if( percent >= 50 )
                        mudstrlcpy( pbuf, "nasty wounds", MSL - ( pbuf - buf ) );
                     else if( percent >= 40 )
                        mudstrlcpy( pbuf, "bleeding freely", MSL - ( pbuf - buf ) );
                     else if( percent >= 30 )
                        mudstrlcpy( pbuf, "covered in blood", MSL - ( pbuf - buf ) );
                     else if( percent >= 20 )
                        mudstrlcpy( pbuf, "leaking guts", MSL - ( pbuf - buf ) );
                     else if( percent >= 10 )
                        mudstrlcpy( pbuf, "almost dead", MSL - ( pbuf - buf ) );
                     else
                        mudstrlcpy( pbuf, "DYING", MSL - ( pbuf - buf ) );
                  }
                  break;

               case 'c':
                  if( !ch->fighting || ( victim = ch->fighting->who ) == NULL )
                     mudstrlcpy( pbuf, "N/A", MSL - ( pbuf - buf ) );
                  else
                  {
                     if( victim->max_hit > 0 )
                        percent = ( 100 * victim->hit ) / victim->max_hit;
                     else
                        percent = -1;
                     if( percent >= 100 )
                        mudstrlcpy( pbuf, "perfect health", MSL - ( pbuf - buf ) );
                     else if( percent >= 90 )
                        mudstrlcpy( pbuf, "slightly scratched", MSL - ( pbuf - buf ) );
                     else if( percent >= 80 )
                        mudstrlcpy( pbuf, "few bruises", MSL - ( pbuf - buf ) );
                     else if( percent >= 70 )
                        mudstrlcpy( pbuf, "some cuts", MSL - ( pbuf - buf ) );
                     else if( percent >= 60 )
                        mudstrlcpy( pbuf, "several wounds", MSL - ( pbuf - buf ) );
                     else if( percent >= 50 )
                        mudstrlcpy( pbuf, "nasty wounds", MSL - ( pbuf - buf ) );
                     else if( percent >= 40 )
                        mudstrlcpy( pbuf, "bleeding freely", MSL - ( pbuf - buf ) );
                     else if( percent >= 30 )
                        mudstrlcpy( pbuf, "covered in blood", MSL - ( pbuf - buf ) );
                     else if( percent >= 20 )
                        mudstrlcpy( pbuf, "leaking guts", MSL - ( pbuf - buf ) );
                     else if( percent >= 10 )
                        mudstrlcpy( pbuf, "almost dead", MSL - ( pbuf - buf ) );
                     else
                        mudstrlcpy( pbuf, "DYING", MSL - ( pbuf - buf ) );
                  }
                  break;

               case 'h':
                  pstat = ch->hit;
                  break;

               case 'H':
                  pstat = ch->max_hit;
                  break;

               case 'm':
                  pstat = ch->mana;
                  break;

               case 'M':
                  pstat = ch->max_mana;
                  break;

               case 'N':  /* Tank */
                  if( !ch->fighting || ( victim = ch->fighting->who ) == NULL )
                     mudstrlcpy( pbuf, "N/A", MSL - ( pbuf - buf ) );
                  else if( !victim->fighting || ( victim = victim->fighting->who ) == NULL )
                     mudstrlcpy( pbuf, "N/A", MSL - ( pbuf - buf ) );
                  else
                  {
                     if( ch == victim )
                        mudstrlcpy( pbuf, "You", MSL - ( pbuf - buf ) );
                     else if( victim->isnpc(  ) )
                        mudstrlcpy( pbuf, victim->short_descr, MSL - ( pbuf - buf ) );
                     else
                        mudstrlcpy( pbuf, victim->name, MSL - ( pbuf - buf ) );
                     pbuf[0] = UPPER( pbuf[0] );
                  }
                  break;

               case 'n':
                  if( !ch->fighting || ( victim = ch->fighting->who ) == NULL )
                     mudstrlcpy( pbuf, "N/A", MSL - ( pbuf - buf ) );
                  else
                  {
                     if( ch == victim )
                        mudstrlcpy( pbuf, "You", MSL - ( pbuf - buf ) );
                     else if( victim->isnpc(  ) )
                        mudstrlcpy( pbuf, victim->short_descr, MSL - ( pbuf - buf ) );
                     else
                        mudstrlcpy( pbuf, victim->name, MSL - ( pbuf - buf ) );
                     pbuf[0] = UPPER( pbuf[0] );
                  }
                  break;

               case 'T':
                  if( time_info.hour < sysdata->hoursunrise )
                     mudstrlcpy( pbuf, "night", MSL - ( pbuf - buf ) );
                  else if( time_info.hour < sysdata->hourdaybegin )
                     mudstrlcpy( pbuf, "dawn", MSL - ( pbuf - buf ) );
                  else if( time_info.hour < sysdata->hoursunset )
                     mudstrlcpy( pbuf, "day", MSL - ( pbuf - buf ) );
                  else if( time_info.hour < sysdata->hournightbegin )
                     mudstrlcpy( pbuf, "dusk", MSL - ( pbuf - buf ) );
                  else
                     mudstrlcpy( pbuf, "night", MSL - ( pbuf - buf ) );
                  break;

               case 'u':
                  pstat = num_descriptors;
                  break;

               case 'U':
                  pstat = sysdata->maxplayers;
                  break;

               case 'v':
                  pstat = ch->move;
                  break;

               case 'V':
                  pstat = ch->max_move;
                  break;

               case 'g':
                  pstat = ch->gold;
                  break;

               case 'r':
                  if( och->is_immortal(  ) )
                     pstat = ch->in_room->vnum;
                  break;

               case 'F':
                  if( och->is_immortal(  ) )
                     mudstrlcpy( pbuf, bitset_string( ch->in_room->flags, r_flags ), MSL - ( pbuf - buf ) );
                  break;

               case 'R':
                  if( och->has_pcflag( PCFLAG_ROOMVNUM ) )
                     snprintf( pbuf, MSL - ( pbuf - buf ), "<#%d> ", ch->in_room->vnum );
                  break;

               case 'x':
                  pstat = ch->exp;
                  break;

               case 'X':
                  pstat = exp_level( ch->level + 1 ) - ch->exp;
                  break;

               case 'o':  /* display name of object on auction */
                  if( auction->item )
                     mudstrlcpy( pbuf, auction->item->name, MSL - ( pbuf - buf ) );
                  break;

               case 'S':
                  if( ch->style == STYLE_BERSERK )
                     mudstrlcpy( pbuf, "B", MSL - ( pbuf - buf ) );
                  else if( ch->style == STYLE_AGGRESSIVE )
                     mudstrlcpy( pbuf, "A", MSL - ( pbuf - buf ) );
                  else if( ch->style == STYLE_DEFENSIVE )
                     mudstrlcpy( pbuf, "D", MSL - ( pbuf - buf ) );
                  else if( ch->style == STYLE_EVASIVE )
                     mudstrlcpy( pbuf, "E", MSL - ( pbuf - buf ) );
                  else
                     mudstrlcpy( pbuf, "S", MSL - ( pbuf - buf ) );
                  break;

               case 'i':
                  if( ch->has_pcflag( PCFLAG_WIZINVIS ) || ch->has_actflag( ACT_MOBINVIS ) )
                     snprintf( pbuf, MSL - ( pbuf - buf ), "(Invis %d) ", ( ch->isnpc(  )? ch->mobinvis : ch->pcdata->wizinvis ) );
                  else if( ch->has_aflag( AFF_INVISIBLE ) )
                     mudstrlcpy( pbuf, "(Invis) ", MSL - ( pbuf - buf ) );
                  break;

               case 'I':
                  if( ch->has_actflag( ACT_MOBINVIS ) )
                     pstat = ch->mobinvis;
                  else if( ch->has_pcflag( PCFLAG_WIZINVIS ) )
                     pstat = ch->pcdata->wizinvis;
                  else
                     pstat = 0;
                  break;

               case 'Z':
                  if( sysdata->WIZLOCK )
                     mudstrlcpy( pbuf, "[Wizlock]", MSL - ( pbuf - buf ) );
                  if( sysdata->IMPLOCK )
                     mudstrlcpy( pbuf, "[Implock]", MSL - ( pbuf - buf ) );
                  if( sysdata->LOCKDOWN )
                     mudstrlcpy( pbuf, "[Lockdown]", MSL - ( pbuf - buf ) );
                  if( bootlock )
                     mudstrlcpy( pbuf, "[Rebooting]", MSL - ( pbuf - buf ) );
                  break;
            }
            if( pstat != 0x80000000 )
               snprintf( pbuf, MSL - ( pbuf - buf ), "%zd", pstat );
            pbuf += strlen( pbuf );
            break;
      }
   }
   *pbuf = '\0';

   ch->print( buf );

   /*
    * The miracle cure for color bleeding? 
    */
   ch->print( ANSI_RESET );
}

void close_socket( descriptor_data * d, bool force )
{
   char_data *ch;
   auth_data *old_auth;

#if !defined(WIN32)
   if( d->ipid != -1 )
   {
      int status;

      kill( d->ipid, SIGKILL );
      waitpid( d->ipid, &status, 0 );
   }
   if( d->ifd != -1 )
      close( d->ifd );
#endif

   /*
    * flush outbuf 
    */
   if( !force && !d->outbuf.empty(  ) )
      d->flush_buffer( false );

   /*
    * say bye to whoever's snooping this descriptor 
    */
   if( d->snoop_by )
      d->snoop_by->write_to_buffer( "Your victim has left the game.\r\n" );

   /*
    * stop snooping everyone else 
    */
   list < descriptor_data * >::iterator ds;
   for( ds = dlist.begin(  ); ds != dlist.end(  ); ++ds )
   {
      descriptor_data *s = *ds;

      if( s->snoop_by == d )
         s->snoop_by = NULL;
   }

   /*
    * Check for switched people who go link-dead. -- Altrag 
    */
   if( d->original )
   {
      if( ( ch = d->character ) != NULL )
         interpret( ch, "return" );
      else
      {
         bug( "%s: original without character %s", __FUNCTION__, ( d->original->name ? d->original->name : "unknown" ) );
         d->character = d->original;
         d->original = NULL;
      }
   }

   ch = d->character;

   if( ch )
   {
      // Put this check here because seeing that they lost link after quitting/renting was annoying. We already know this.
      if( ch->in_room )
         log_printf_plus( LOG_COMM, ch->level, "Closing link to %s.", ch->pcdata->filename );

      /*
       * Link dead auth -- Rantic 
       */
      old_auth = get_auth_name( ch->name );
      if( old_auth != NULL && old_auth->state == AUTH_ONLINE )
      {
         old_auth->state = AUTH_LINK_DEAD;
         save_auth_list(  );
      }

      if( d->connected == CON_EDITING )
      {
         if( ch->last_cmd )
            ch->last_cmd( ch, "" );
         else
            ch->stop_editing(  );
         d->connected = CON_PLAYING;
      }

      if( d->connected == CON_PLAYING
          || d->connected == CON_EDITING
          || d->connected == CON_DELETE
          || d->connected == CON_ROLL_STATS
          || d->connected == CON_RAISE_STAT
          || d->connected == CON_PRIZENAME
          || d->connected == CON_CONFIRMPRIZENAME
          || d->connected == CON_PRIZEKEY
          || d->connected == CON_CONFIRMPRIZEKEY || d->connected == CON_OEDIT || d->connected == CON_REDIT || d->connected == CON_MEDIT || d->connected == CON_BOARD )
      {
         if( ch->in_room )
            act( AT_ACTION, "$n has lost $s link.", ch, NULL, NULL, TO_CANSEE );
         ch->desc = NULL;
      }
      else
      {
         /*
          * clear descriptor pointer to get rid of bug message in log 
          */
         ch->desc = NULL;
         deleteptr( ch );
      }
   }
   dlist.remove( d );
   if( d->descriptor == maxdesc )
      --maxdesc;
   --num_descriptors;
   deleteptr( d );

   if( dlist.size(  ) < 1 )
      log_string( "No more people online. Autonomous actions suspended." );
}

/* This may seem silly, but this stuff was called in several spots. So I consolidated. - Samson 2-7-04 */
void show_stateflags( char_data * ch )
{
#ifdef MULTIPORT
   /*
    * Make sure Those who can reboot know the compiler is running - Samson 
    */
   if( ch->level >= LEVEL_GOD && compilelock )
      ch->print( "\r\n&RNOTE: The system compiler is in use. Reboot and Shutdown commands are locked.\r\n" );
#endif

   if( bootlock )
      ch->print( "\r\n&GNOTE: The system is preparing for a reboot.\r\n" );

   if( sysdata->LOCKDOWN && ch->level == LEVEL_SUPREME )
      ch->print( "\r\n&RReminder, Mr. Imp sir, the game is under lockdown. Nobody else can connect now.\r\n" );

   if( sysdata->IMPLOCK && ch->is_imp(  ) )
      ch->printf( "\r\n&RNOTE: The game is implocked. Only level %d and above gods can log on.\r\n", LEVEL_KL );

   if( sysdata->WIZLOCK )
      ch->print( "\r\n&YNOTE: The game is wizlocked. No mortals can log on.\r\n" );

   if( crash_count > 0 && ch->is_imp(  ) )
      ch->printf( "\r\n}RThere ha%s been %d intercepted SIGSEGV since reboot. Check the logs.\r\n", crash_count == 1 ? "s" : "ve", crash_count );
}

void show_status( char_data * ch )
{
   if( !ch->is_immortal(  ) && ch->pcdata->motd < sysdata->motd )
   {
      show_file( ch, MOTD_FILE );
      ch->pcdata->motd = current_time;
   }
   else if( ch->is_immortal(  ) && ch->pcdata->imotd < sysdata->imotd )
   {
      show_file( ch, IMOTD_FILE );
      ch->pcdata->imotd = current_time;
   }

   if( exists_file( SPEC_MOTD ) )
      show_file( ch, SPEC_MOTD );

   interpret( ch, "look" );

   /*
    * @shrug, why not? :P 
    */
   if( ch->has_pcflag( PCFLAG_ONMAP ) )
      ch->music( "wilderness.mid", 100, false );

   ch->print( "&R\r\n" );
   if( ch->has_pcflag( PCFLAG_CHECKBOARD ) )
      interpret( ch, "checkboards" );

   if( str_cmp( ch->desc->client, "Unidentified" ) )
      log_printf_plus( LOG_COMM, LEVEL_IMMORTAL, "%s client detected for %s.", capitalize( ch->desc->client ).c_str(  ), ch->name );
   if( ch->desc->can_compress )
      log_printf_plus( LOG_COMM, LEVEL_IMMORTAL, "MCCP support detected for %s.", ch->name );
   if( ch->desc->msp_detected )
      log_printf_plus( LOG_COMM, LEVEL_IMMORTAL, "MSP support detected for %s.", ch->name );
   quotes( ch );
   show_stateflags( ch );
}

/*
 * Look for link-dead player to reconnect.
 */
short descriptor_data::check_reconnect( const string & name, bool fConn )
{
   list < char_data * >::iterator ich;

   for( ich = pclist.begin(  ); ich != pclist.end(  ); ++ich )
   {
      char_data *ch = *ich;

      if( ( !fConn || !ch->desc ) && ch->pcdata->filename && !str_cmp( name, ch->pcdata->filename ) )
      {
         if( fConn && ch->switched )
         {
            write_to_buffer( "Already playing.\r\nName: " );
            connected = CON_GET_NAME;
            if( character )
            {
               /*
                * clear descriptor pointer to get rid of bug message in log 
                */
               character->desc = NULL;
               deleteptr( character );
            }
            return BERR;
         }
         if( fConn == false )
         {
            DISPOSE( character->pcdata->pwd );
            character->pcdata->pwd = str_dup( ch->pcdata->pwd );
         }
         else
         {
            /*
             * clear descriptor pointer to get rid of bug message in log 
             */
            character->desc = NULL;
            deleteptr( character );
            character = ch;
            ch->desc = this;
            ch->timer = 0;
            ch->print( "Reconnecting.\r\n\r\n" );
            update_connhistory( ch->desc, CONNTYPE_RECONN );

            act( AT_ACTION, "$n has reconnected.", ch, NULL, NULL, TO_CANSEE );
            log_printf_plus( LOG_COMM, ch->level, "%s [%s] reconnected.", ch->name, host.c_str(  ) );
            connected = CON_PLAYING;
            check_auth_state( ch ); /* Link dead support -- Rantic */
            show_status( ch );
         }
         return true;
      }
   }
   return false;
}

/*
 * Check if already playing.
 */
short descriptor_data::check_playing( const string & name, bool kick )
{
   list < descriptor_data * >::iterator ds;
   char_data *ch;
   int cstate;

   for( ds = dlist.begin(  ); ds != dlist.end(  ); ++ds )
   {
      descriptor_data *d = *ds;

      if( d != this && ( d->character || d->original ) && !str_cmp( name, d->original ? d->original->pcdata->filename : d->character->pcdata->filename ) )
      {
         cstate = d->connected;
         ch = d->original ? d->original : d->character;
         if( !ch->name
             || ( cstate != CON_PLAYING && cstate != CON_EDITING && cstate != CON_DELETE
                  && cstate != CON_ROLL_STATS && cstate != CON_PRIZENAME && cstate != CON_CONFIRMPRIZENAME
                  && cstate != CON_PRIZEKEY && cstate != CON_CONFIRMPRIZEKEY && cstate != CON_RAISE_STAT ) )
         {
            write_to_buffer( "Already connected - try again.\r\n" );
            log_printf_plus( LOG_COMM, ch->level, "%s already connected.", ch->pcdata->filename );
            return BERR;
         }
         if( !kick )
            return true;
         write_to_buffer( "Already playing... Kicking off old connection.\r\n" );
         d->write_to_buffer( "Kicking off old connection... bye!\r\n" );
         close_socket( d, false );
         /*
          * clear descriptor pointer to get rid of bug message in log 
          */
         character->desc = NULL;
         deleteptr( character );
         character = ch;
         ch->desc = this;
         ch->timer = 0;
         if( ch->switched )
            interpret( ch->switched, "return" );
         ch->switched = NULL;
         ch->print( "Reconnecting.\r\n\r\n" );

         act( AT_ACTION, "$n has reconnected, kicking off old link.", ch, NULL, NULL, TO_CANSEE );
         log_printf_plus( LOG_COMM, ch->level, "%s [%s] reconnected, kicking off old link.", ch->name, host.c_str(  ) );
         connected = cstate;
         check_auth_state( ch ); /* Link dead support -- Rantic */
         show_status( ch );
         return true;
      }
   }
   return false;
}

void char_to_game( char_data * ch )
{
   if( str_cmp( ch->desc->client, "Unidentified" ) )
      log_printf_plus( LOG_COMM, LEVEL_IMMORTAL, "%s client detected for %s.", capitalize( ch->desc->client ).c_str(  ), ch->name );
   if( ch->desc->can_compress )
      log_printf_plus( LOG_COMM, LEVEL_IMMORTAL, "MCCP support detected for %s.", ch->name );
   if( ch->desc->msp_detected )
      log_printf_plus( LOG_COMM, LEVEL_IMMORTAL, "MSP support detected for %s.", ch->name );

   charlist.push_back( ch );
   pclist.push_back( ch );
   ch->desc->connected = CON_PLAYING;

   /*
    * Connection History Updating 
    */
   if( ch->level == 0 )
      update_connhistory( ch->desc, CONNTYPE_NEWPLYR );
   else
      update_connhistory( ch->desc, CONNTYPE_LOGIN );

   if( ch->level == 0 )
      setup_newbie( ch, true );

   else if( !ch->is_immortal(  ) && ch->pcdata->release_date > 0 && ch->pcdata->release_date > current_time )
   {
      if( !ch->to_room( get_room_index( ROOM_VNUM_HELL ) ) )
         log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
   }

   else if( ch->in_room && ( ch->is_immortal(  ) || !ch->in_room->flags.test( ROOM_PROTOTYPE ) ) )
   {
      if( !ch->to_room( ch->in_room ) )
         log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
   }

   else if( ch->is_immortal(  ) )
   {
      if( !ch->to_room( get_room_index( ROOM_VNUM_CHAT ) ) )
         log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
   }

   else
   {
      if( !ch->to_room( get_room_index( ROOM_VNUM_TEMPLE ) ) )
         log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
   }

   if( ch->get_timer( TIMER_SHOVEDRAG ) > 0 )
      ch->remove_timer( TIMER_SHOVEDRAG );

   if( ch->get_timer( TIMER_PKILLED ) > 0 )
      ch->remove_timer( TIMER_PKILLED );

   act( AT_ACTION, "$n has entered the game.", ch, NULL, NULL, TO_CANSEE );

   if( !ch->was_in_room && ch->in_room == get_room_index( ROOM_VNUM_TEMPLE ) )
      ch->was_in_room = get_room_index( ROOM_VNUM_TEMPLE );

   else if( ch->was_in_room == get_room_index( ROOM_VNUM_TEMPLE ) )
      ch->was_in_room = get_room_index( ROOM_VNUM_TEMPLE );

   else if( !ch->was_in_room )
      ch->was_in_room = ch->in_room;

   interpret( ch, "look" );

   check_holiday( ch );

   ch->print( "&R\r\n" );
   if( ch->has_pcflag( PCFLAG_CHECKBOARD ) )
      interpret( ch, "checkboards" );

   if( ch->pcdata->camp == 1 )
      break_camp( ch );
   else
      scan_rares( ch );
   ch->pcdata->daysidle = 0;

   ch->pcdata->lasthost = ch->desc->host;

   sale_count( ch ); /* New auction system - Samson 6-24-99 */

   check_auth_state( ch ); /* new auth */
   check_clan_info( ch );  /* see if this guy got a promo to clan admin */

   /*
    * @shrug, why not? :P 
    */
   if( ch->has_pcflag( PCFLAG_ONMAP ) )
      ch->music( "wilderness.mid", 100, false );

   quotes( ch );

   if( ch->tempnum > 0 )
   {
      ch->print( "&YUpdating character for changes to experience system.\r\n" );
      ch->desc->show_stats( ch );
      ch->desc->connected = CON_RAISE_STAT;
   }
   ch->save(  );  /* Just making sure their status is saved at least once after login, in case of crashes */
   ++num_logins;

   if( ch->level < LEVEL_IMMORTAL )
      ++sysdata->playersonline;
}

void display_motd( char_data * ch )
{
   if( ch->is_immortal(  ) && ch->pcdata->imotd < sysdata->imotd )
   {
      if( ch->has_pcflag( PCFLAG_ANSI ) )
         ch->pager( "\033[2J" );
      else
         ch->pager( "\014" );

      show_file( ch, IMOTD_FILE );
      ch->pager( "\r\nPress [ENTER] " );
      ch->pcdata->imotd = current_time;
      ch->desc->connected = CON_READ_MOTD;
      return;
   }
   if( ch->level < LEVEL_IMMORTAL && ch->level > 0 && ch->pcdata->motd < sysdata->motd )
   {
      if( ch->has_pcflag( PCFLAG_ANSI ) )
         ch->pager( "\033[2J" );
      else
         ch->pager( "\014" );

      show_file( ch, MOTD_FILE );
      ch->pager( "\r\nPress [ENTER] " );
      ch->pcdata->motd = current_time;
      ch->desc->connected = CON_READ_MOTD;
      return;
   }
   if( ch->level == 0 )
   {
      if( ch->has_pcflag( PCFLAG_ANSI ) )
         ch->pager( "\033[2J" );
      else
         ch->pager( "\014" );

      do_help( ch, "nmotd" );
      ch->pager( "\r\nPress [ENTER] " );
      ch->desc->connected = CON_READ_MOTD;
      return;
   }

   if( exists_file( SPEC_MOTD ) )
   {
      if( ch->has_pcflag( PCFLAG_ANSI ) )
         ch->pager( "\033[2J" );
      else
         ch->pager( "\014" );

      show_file( ch, SPEC_MOTD );
   }
   ch->desc->buffer_printf( "\r\nWelcome to %s...\r\n", sysdata->mud_name.c_str(  ) );
   char_to_game( ch );
}

char *md5_crypt( const char *pwd )
{
   md5_state_t state;
   static md5_byte_t digest[17];
   static char passwd[17];
   unsigned int x;

   md5_init( &state );
   md5_append( &state, ( const md5_byte_t * )pwd, strlen( pwd ) );
   md5_finish( &state, digest );

   mudstrlcpy( passwd, ( const char * )digest, 16 );

   /*
    * The listed exceptions below will fubar the MD5 authentication packets, so change them 
    */
   for( x = 0; x < strlen( passwd ); ++x )
   {
      if( passwd[x] == '\n' )
         passwd[x] = 'n';
      if( passwd[x] == '\r' )
         passwd[x] = 'r';
      if( passwd[x] == '\t' )
         passwd[x] = 't';
      if( passwd[x] == ' ' )
         passwd[x] = 's';
      if( ( int )passwd[x] == 11 )
         passwd[x] = 'x';
      if( ( int )passwd[x] == 12 )
         passwd[x] = 'X';
      if( passwd[x] == '~' )
         passwd[x] = '+';
      if( passwd[x] == EOF )
         passwd[x] = 'E';
   }
   return ( passwd );
}

CMDF( do_shatest )
{
   ch->printf( "%s\r\n", argument.c_str(  ) );
   ch->printf( "%s\r\n", sha256_crypt( argument.c_str(  ) ) );
   ch->printf( "%s\r\n", md5_crypt( argument.c_str(  ) ) );
}

/*
 * Deal with sockets that haven't logged in yet.
 */
/* Function modified from original form on varying dates - Samson */
/* Password encryption sections upgraded to use SHA-256 Encryption - Samson 7-10-00 */
void descriptor_data::nanny( string & argument )
{
   char buf[MSL];
   char_data *ch;
   const char *pwdnew;
   bool fOld;
   short chk;

   strip_lspace( argument );

   ch = character;

   switch ( connected )
   {
      default:
         bug( "%s: bad d->connected %d.", __FUNCTION__, connected );
         close_socket( this, true );
         return;

      case CON_OEDIT:
         oedit_parse( this, argument );
         break;

      case CON_REDIT:
         redit_parse( this, argument );
         break;

      case CON_MEDIT:
         medit_parse( this, argument );
         break;

      case CON_BOARD:
         board_parse( this, argument );
         break;

      case CON_GET_NAME:
         if( argument.empty(  ) )
         {
            close_socket( this, false );
            return;
         }

         if( !str_cmp( argument, "MSSP-REQUEST" ) )
         {
            send_mssp_data( this );
            //Uncomment below if you want to know when an MSSP request occurs
            //log_printf( "IP: %s requested MSSP data!", d->host );
            close_socket( this, false );
            return;
         }

         /*
          * Old players can keep their characters. -- Alty 
          */
         strlower( argument );   // Modification to force proper name display
         argument[0] = UPPER( argument[0] ); /* Samson 5-22-98 */
         if( !check_parse_name( argument, ( newstate != 0 ) ) )
         {
            write_to_buffer( "You have chosen a name which is unnacceptable.\r\n" );
            write_to_buffer( "Acceptable names cannot be:\r\n\r\n" );
            write_to_buffer( "- Nonsensical, unpronounceable or ridiculous.\r\n" );
            write_to_buffer( "- Profane or derogatory as interpreted in any language.\r\n" );
            write_to_buffer( "- Modern, futuristic, or common, such as 'Jill' or 'Laser'.\r\n" );
            write_to_buffer( "- Similar to that of any Immortal, monster, or object.\r\n" );
            write_to_buffer( "- Comprised of non-alphanumeric characters.\r\n" );
            write_to_buffer( "- Comprised of various capital letters, such as 'BrACkkA' or 'CORTO'.\r\n" );
            write_to_buffer( "- Comprised of ranks or titles, such as 'Lord' or 'Master'.\r\n" );
            write_to_buffer( "- Composed of singular descriptive nouns, adverbs or adjectives,\r\n" );
            write_to_buffer( "  as in 'Heart', 'Big', 'Flying', 'Broken', 'Slick' or 'Tricky'.\r\n" );
            write_to_buffer( "- Any of the above in reverse, i.e., writing Joe as 'Eoj'.\r\n" );
            write_to_buffer( "- Anything else the Immortal staff deems inappropriate.\r\n\r\n" );
            write_to_buffer( "Please choose a new name: " );
            return;
         }

         if( !str_cmp( argument, "New" ) )
         {
            if( newstate == 0 )
            {
               /*
                * New player 
                * Don't allow new players if DENY_NEW_PLAYERS is true 
                */
               if( sysdata->DENY_NEW_PLAYERS == true )
               {
                  write_to_buffer( "The mud is currently preparing for a reboot.\r\n" );
                  write_to_buffer( "New players are not accepted during this time.\r\n" );
                  write_to_buffer( "Please try again in a few minutes.\r\n" );
                  close_socket( this, false );
               }
               write_to_buffer( "\r\nChoosing a name is one of the most important parts of this game...\r\n"
                                "Make sure to pick a name appropriate to the character you are going\r\n"
                                "to role play, and be sure that it suits a medieval theme.\r\n"
                                "If the name you select is not acceptable, you will be asked to choose\r\n" "another one.\r\n\r\nPlease choose a name for your character: " );
               ++newstate;
               connected = CON_GET_NAME;
               return;
            }
            else
            {
               write_to_buffer( "You have chosen a name which is unnacceptable.\r\n" );
               write_to_buffer( "Acceptable names cannot be:\r\n\r\n" );
               write_to_buffer( "- Nonsensical, unpronounceable or ridiculous.\r\n" );
               write_to_buffer( "- Profane or derogatory as interpreted in any language.\r\n" );
               write_to_buffer( "- Modern, futuristic, or common, such as 'Jill' or 'Laser'.\r\n" );
               write_to_buffer( "- Similar to that of any Immortal, monster, or object.\r\n" );
               write_to_buffer( "- Comprised of non-alphanumeric characters.\r\n" );
               write_to_buffer( "- Comprised of various capital letters, such as 'BrACkkA' or 'CORTO'.\r\n" );
               write_to_buffer( "- Comprised of ranks or titles, such as 'Lord' or 'Master'.\r\n" );
               write_to_buffer( "- Composed of singular descriptive nouns, adverbs or adjectives,\r\n" );
               write_to_buffer( "  as in 'Heart', 'Big', 'Flying', 'Broken', 'Slick' or 'Tricky'.\r\n" );
               write_to_buffer( "- Any of the above in reverse, i.e., writing Joe as 'Eoj'.\r\n" );
               write_to_buffer( "- Anything else the Immortal staff deems inappropriate.\r\n\r\n" );
               write_to_buffer( "Please choose a new name: " );
               return;
            }
         }

         if( check_playing( argument, false ) == BERR )
         {
            write_to_buffer( "Name: " );
            return;
         }

         log_printf_plus( LOG_COMM, LEVEL_KL, "Incoming connection: %s, port %d.", host.c_str(  ), client_port );

         fOld = load_char_obj( this, argument, true, false );
         if( !character )
         {
            log_printf( "Bad player file %s@%s.", argument.c_str(  ), host.c_str(  ) );
            buffer_printf( "Your playerfile is corrupt...Please notify %s\r\n", sysdata->admin_email.c_str(  ) );
            close_socket( this, false );
            return;
         }
         ch = character;

/* FIXME: Write new ban code so this has meaning again.
         if( check_bans( ch ) )
         {
            write_to_buffer( "Your site has been banned from this Mud.\r\n" );
            close_socket( this, false );
            return;
         }
*/

         if( ch->has_pcflag( PCFLAG_DENY ) )
         {
            log_printf_plus( LOG_COMM, LEVEL_IMMORTAL, "Denying access to %s@%s.", argument.c_str(  ), host.c_str(  ) );
            if( newstate != 0 )
            {
               write_to_buffer( "That name is already taken.  Please choose another: " );
               connected = CON_GET_NAME;
               character->desc = NULL;
               deleteptr( character ); /* Big Memory Leak before --Shaddai */
               return;
            }
            write_to_buffer( "You are denied access.\r\n" );
            close_socket( this, false );
            return;
         }

         /*
          * Make sure the immortal host is from the correct place. Shaddai 
          */
         if( ch->is_immortal(  ) && sysdata->check_imm_host && !check_immortal_domain( ch, host ) )
         {
            write_to_buffer( "Invalid host profile. This event has been logged for inspection.\r\n" );
            close_socket( this, false );
            return;
         }

         chk = check_reconnect( argument, false );
         if( chk == BERR )
            return;

         if( chk )
            fOld = true;

         if( ( sysdata->WIZLOCK || sysdata->IMPLOCK || sysdata->LOCKDOWN ) && !ch->is_immortal(  ) )
         {
            write_to_buffer( "The game is wizlocked. Only immortals can connect now.\r\n" );
            write_to_buffer( "Please try back later.\r\n" );
            log_printf_plus( LOG_COMM, LEVEL_IMMORTAL, "Player: %s disconnected due to %s.", ch->name,
                             sysdata->WIZLOCK ? "wizlock" : sysdata->IMPLOCK ? "implock" : "lockdown" );
            close_socket( this, false );
            return;
         }
         else if( sysdata->LOCKDOWN && ch->level < LEVEL_SUPREME )
         {
            write_to_buffer( "The game is locked down. Only the head implementor can connect now.\r\n" );
            write_to_buffer( "Please try back later.\r\n" );
            log_printf_plus( LOG_COMM, LEVEL_SUPREME, "Immortal: %s disconnected due to lockdown.", ch->name );
            close_socket( this, false );
            return;
         }
         else if( sysdata->IMPLOCK && !ch->is_imp(  ) )
         {
            write_to_buffer( "The game is implocked. Only implementors can connect now.\r\n" );
            write_to_buffer( "Please try back later.\r\n" );
            log_printf_plus( LOG_COMM, LEVEL_KL, "Immortal: %s disconnected due to implock.", ch->name );
            close_socket( this, false );
            return;
         }
         else if( bootlock && !ch->is_immortal(  ) )
         {
            write_to_buffer( "The game is preparing to reboot. Please try back in about 5 minutes.\r\n" );
            log_printf_plus( LOG_COMM, LEVEL_IMMORTAL, "Player: %s disconnected due to bootlock.", ch->name );
            close_socket( this, false );
            return;
         }

         if( fOld )
         {
            if( newstate != 0 )
            {
               write_to_buffer( "That name is already taken.  Please choose another: " );
               connected = CON_GET_NAME;
               character->desc = NULL;
               deleteptr( character ); /* Big Memory Leak before --Shaddai */
               return;
            }
            /*
             * Old player 
             */
            write_to_buffer( "\r\nEnter your password: " );
            write_to_buffer( echo_off_str );
            connected = CON_GET_OLD_PASSWORD;
            return;
         }
         else
         {
            if( newstate == 0 )
            {
               /*
                * No such player 
                */
               write_to_buffer( "\r\nNo such player exists.\r\nPlease check your spelling, or type new to start a new player.\r\n\r\nName: " );
               connected = CON_GET_NAME;
               character->desc = NULL;
               deleteptr( character ); /* Big Memory Leak before --Shaddai */
               return;
            }
            strlower( argument );   /* Added to force names to display properly */
            argument = capitalize( argument );  /* Samson 5-22-98 */
            STRFREE( ch->name );
            ch->name = STRALLOC( argument.c_str(  ) );
            buffer_printf( "Did I get that right, %s (Y/N)? ", argument.c_str(  ) );
            connected = CON_CONFIRM_NEW_NAME;
            return;
         }

      case CON_GET_OLD_PASSWORD:
         write_to_buffer( "\r\n" );

         if( ch->pcdata->version < 22 )
         {
            if( str_cmp( md5_crypt( argument.c_str(  ) ), ch->pcdata->pwd ) )
            {
               write_to_buffer( "Wrong password, disconnecting.\r\n" );
               /*
                * clear descriptor pointer to get rid of bug message in log 
                */
               character->desc = NULL;
               close_socket( this, false );
               return;
            }
         }
         else if( str_cmp( sha256_crypt( argument.c_str(  ) ), ch->pcdata->pwd ) )
         {
            write_to_buffer( "Wrong password, disconnecting.\r\n" );
            /*
             * clear descriptor pointer to get rid of bug message in log 
             */
            character->desc = NULL;
            close_socket( this, false );
            return;
         }

         write_to_buffer( echo_on_str );

         if( check_playing( ch->pcdata->filename, true ) )
            return;

         chk = check_reconnect( ch->pcdata->filename, true );
         if( chk == BERR )
         {
            if( character && character->desc )
               character->desc = NULL;
            close_socket( this, false );
            return;
         }
         if( chk == true )
            return;

         mudstrlcpy( buf, ch->pcdata->filename, MSL );
         character->desc = NULL;
         deleteptr( character );
         fOld = load_char_obj( this, buf, false, false );
         if( !character )
         {
            log_printf( "Bad player file %s@%s.", argument.c_str(  ), host.c_str(  ) );
            buffer_printf( "Your playerfile is corrupt...Please notify %s\r\n", sysdata->admin_email.c_str(  ) );
            close_socket( this, false );
            return;
         }
         ch = character;

         if( ch->position > POS_SITTING && ch->position < POS_STANDING )
            ch->position = POS_STANDING;

         if( ch->pcdata->version < 22 )
         {
            DISPOSE( ch->pcdata->pwd );
            ch->pcdata->pwd = str_dup( sha256_crypt( argument.c_str(  ) ) );
         }
         log_printf_plus( LOG_COMM, LEVEL_KL, "%s [%s] has connected.", ch->name, host.c_str(  ) );
         show_title(  );
         break;

      case CON_CONFIRM_NEW_NAME:
         switch ( argument[0] )
         {
            case 'y':
            case 'Y':
               buffer_printf( "\r\nMake sure to use a password that won't be easily guessed by someone else." "\r\nPick a good password for %s: %s", ch->name, echo_off_str );
               connected = CON_GET_NEW_PASSWORD;
               break;

            case 'n':
            case 'N':
               write_to_buffer( "Ok, what IS it, then? " );
               /*
                * clear descriptor pointer to get rid of bug message in log 
                */
               character->desc = NULL;
               deleteptr( character );
               connected = CON_GET_NAME;
               break;

            default:
               write_to_buffer( "Please type Yes or No. " );
               break;
         }
         break;

      case CON_GET_NEW_PASSWORD:
         write_to_buffer( "\r\n" );

         if( argument.length(  ) < 5 )
         {
            write_to_buffer( "Password must be at least five characters long.\r\nPassword: " );
            return;
         }

         if( argument[0] == '!' )
         {
            write_to_buffer( "Password cannot begin with the '!' character.\r\nPassword: " );
            return;
         }

         pwdnew = sha256_crypt( argument.c_str(  ) ); /* SHA-256 Encryption */
         DISPOSE( ch->pcdata->pwd );
         ch->pcdata->pwd = str_dup( pwdnew );
         write_to_buffer( "\r\nPlease retype the password to confirm: " );
         connected = CON_CONFIRM_NEW_PASSWORD;
         break;

      case CON_CONFIRM_NEW_PASSWORD:
         write_to_buffer( "\r\n" );

         if( str_cmp( sha256_crypt( argument.c_str(  ) ), ch->pcdata->pwd ) )
         {
            write_to_buffer( "Passwords don't match.\r\nRetype password: " );
            connected = CON_GET_NEW_PASSWORD;
            return;
         }

#ifdef MULTIPORT
         if( mud_port != MAINPORT )
         {
            write_to_buffer( "This is a restricted access port. Only immortals and their test players are allowed.\r\n" );
            write_to_buffer( "Enter access code: " );
            write_to_buffer( echo_off_str );
            connected = CON_GET_PORT_PASSWORD;
            return;
         }
#endif

         write_to_buffer( echo_on_str );

         write_to_buffer( "\r\nPlease note: You will be able to pick race and class after entering the game." );
         write_to_buffer( "\r\nWhat is your sex? " );
         write_to_buffer( "\r\n(M)ale, (F)emale, (N)euter, or (H)ermaphrodyte ?" );
         connected = CON_GET_NEW_SEX;
         break;

      case CON_GET_PORT_PASSWORD:
         write_to_buffer( "\r\n" );

         if( str_cmp( sha256_crypt( argument.c_str(  ) ), sysdata->password ) )
         {
            write_to_buffer( "Invalid access code.\r\n" );
            /*
             * clear descriptor pointer to get rid of bug message in log 
             */
            character->desc = NULL;
            close_socket( this, false );
            return;
         }
         write_to_buffer( echo_on_str );
         write_to_buffer( "\r\nPlease note: You will be able to pick race and class after entering the game." );
         write_to_buffer( "\r\nWhat is your sex? " );
         write_to_buffer( "\r\n(M)ale, (F)emale, (N)euter, or (H)ermaphrodyte ?" );
         connected = CON_GET_NEW_SEX;
         break;

      case CON_GET_NEW_SEX:
         switch ( argument[0] )
         {
            case 'm':
            case 'M':
               ch->sex = SEX_MALE;
               break;
            case 'f':
            case 'F':
               ch->sex = SEX_FEMALE;
               break;
            case 'n':
            case 'N':
               ch->sex = SEX_NEUTRAL;
               break;
            case 'h':
            case 'H':
               ch->sex = SEX_HERMAPHRODYTE;
               break;
            default:
               write_to_buffer( "That's not a gender.\r\nWhat IS your gender? " );
               return;
         }

         /*
          * ANSI defaults to on now. This is 2003 folks. Grow up. Samson 7-24-03 
          */
         ch->set_pcflag( PCFLAG_ANSI );
         reset_colors( ch );  /* Added for new configurable color code - Samson 3-27-98 */
         ch->set_pcflag( PCFLAG_BLANK );
         ch->set_pcflag( PCFLAG_AUTOMAP );

         /*
          * MSP terminal support. Activate once at creation - Samson 8-21-01 
          */
         if( ch->desc->msp_detected )
            ch->set_pcflag( PCFLAG_MSP );

         write_to_buffer( "Press [ENTER] " );
         show_title(  );
         ch->level = 0;
         ch->position = POS_STANDING;
         connected = CON_PRESS_ENTER;
         return;

      case CON_PRESS_ENTER:
         ch->set_pager_color( AT_PLAIN ); /* Set default color so they don't see blank space */

         if( ch->position == POS_MOUNTED )
            ch->position = POS_STANDING;

         /*
          * MOTD messages now handled in separate function - Samson 12-31-00 
          */
         display_motd( ch );
         break;

         /*
          * Con state for self delete code, added by Samson 1-18-98
          * Code courtesy of Waldemar Thiel (Swiv) 
          */
      case CON_DELETE:
         write_to_buffer( "\r\n" );

         if( str_cmp( sha256_crypt( argument.c_str(  ) ), ch->pcdata->pwd ) )
         {
            write_to_buffer( "Wrong password entered, deletion cancelled.\r\n" );
            write_to_buffer( echo_on_str );
            connected = CON_PLAYING;
            return;
         }
         else
         {
            room_index *donate = get_room_index( ROOM_VNUM_DONATION );

            write_to_buffer( "\r\nYou've deleted your character!!!\r\n" );
            log_printf( "Player: %s has deleted.", capitalize( ch->name ) );

            if( donate != NULL && ch->level > 1 )  /* No more deleting to remove goodies from play */
            {
               list < obj_data * >::iterator iobj;

               for( iobj = ch->carrying.begin(  ); iobj != ch->carrying.end(  ); )
               {
                  obj_data *obj = ( *iobj );
                  ++iobj;

                  if( obj->wear_loc != WEAR_NONE )
                     ch->unequip( obj );
               }

               for( iobj = ch->carrying.begin(  ); iobj != ch->carrying.end(  ); )
               {
                  obj_data *obj = ( *iobj );
                  ++iobj;

                  obj->separate(  );
                  obj->from_char(  );
                  if( obj->extra_flags.test( ITEM_PERSONAL ) || !ch->can_drop_obj( obj ) )
                     obj->extract(  );
                  else
                  {
                     obj = obj->to_room( donate, ch );
                     obj->extra_flags.test( ITEM_DONATION );
                  }
               }
            }
            do_destroy( ch, ch->name );
            return;
         }

         /*
          * Begin Sindhae Section 
          */
      case CON_PRIZENAME:
      {
         obj_data *prize;

         prize = ( obj_data * ) ch->pcdata->spare_ptr;
         if( !prize )
         {
            bug( "%s: Prize object turned NULL somehow!", __FUNCTION__ );
            write_to_buffer( "A fatal internal error has occured. Seek immortal assistance.\r\n" );
            ch->from_room(  );
            if( !ch->to_room( get_room_index( ROOM_VNUM_REDEEM ) ) )
               log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
            connected = CON_PLAYING;
            break;
         }

         if( argument.empty(  ) )
         {
            write_to_buffer( "You must specify a name for your prize.\r\n\r\n" );
            ch->printf( "You are editing %s.\r\n", prize->short_descr );
            write_to_buffer( "[SINDHAE] Prizename: " );
            return;
         }

         if( !str_cmp( argument, "help" ) )
         {
            do_help( ch, "prizenaming" );
            ch->printf( "\r\n&RYou are editing %s&R.\r\n", prize->short_descr );
            write_to_buffer( "[SINDHAE] Prizename: " );
            return;
         }

         if( !str_cmp( argument, "help pcolors" ) )
         {
            do_help( ch, "pcolors" );
            ch->printf( "\r\n&RYou are editing %s&R.\r\n", prize->short_descr );
            write_to_buffer( "[SINDHAE] Prizename: " );
            return;
         }

         /*
          * God, I hope this works 
          */
         STRFREE( prize->short_descr );
         prize->short_descr = STRALLOC( argument.c_str(  ) );

         ch->print( "\r\nYour prize will look like this when worn:\r\n" );
         ch->print( prize->format_to_char( ch, true, 1 ) );
         ch->set_color( AT_RED );
         write_to_buffer( "\r\nIs this correct? (Y/N)" );
         ch->pcdata->spare_ptr = prize;
         connected = CON_CONFIRMPRIZENAME;
      }
         break;

      case CON_CONFIRMPRIZENAME:
      {
         obj_data *prize;

         prize = ( obj_data * ) ch->pcdata->spare_ptr;
         if( !prize )
         {
            bug( "%s: Prize object turned NULL somehow!", __FUNCTION__ );
            write_to_buffer( "A fatal internal error has occured. Seek immortal assistance.\r\n" );
            ch->from_room(  );
            if( !ch->to_room( get_room_index( ROOM_VNUM_REDEEM ) ) )
               log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
            connected = CON_PLAYING;
            break;
         }

         switch ( argument[0] )
         {
            case 'y':
            case 'Y':
               write_to_buffer( "\r\nPrize name is confirmed.\r\n\r\n" );
               ch->set_color( AT_YELLOW );
               write_to_buffer( "You will now select at least one keyword for your prize.\r\n" );
               write_to_buffer( "A keyword is a word you use to manipulate the object.\r\n" );
               write_to_buffer( "Your keywords many NOT contain any color tags.\r\n\r\n" );
               ch->printf( "&RYou are editing %s&R.\r\n", prize->short_descr );
               write_to_buffer( "[SINDHAE] Prizekey: " );
               ch->pcdata->spare_ptr = prize;
               connected = CON_PRIZEKEY;
               break;

            case 'n':
            case 'N':
               write_to_buffer( "\r\nOk, then please enter the correct name.\r\n" );
               ch->printf( "\r\n&RYou are editing %s&R.\r\n", prize->short_descr );
               write_to_buffer( "[SINDHAE] Prizename: " );
               ch->pcdata->spare_ptr = prize;
               connected = CON_PRIZENAME;
               return;

            default:
               write_to_buffer( "Yes or No? " );
               return;
         }
      }
         break;

      case CON_PRIZEKEY:
      {
         obj_data *prize;

         prize = ( obj_data * ) ch->pcdata->spare_ptr;
         if( !prize )
         {
            bug( "%s: Prize object turned NULL somehow!", __FUNCTION__ );
            write_to_buffer( "A fatal internal error has occured. Seek immortal assistance.\r\n" );
            ch->from_room(  );
            if( !ch->to_room( get_room_index( ROOM_VNUM_REDEEM ) ) )
               log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
            connected = CON_PLAYING;
            break;
         }

         if( argument.empty(  ) )
         {
            write_to_buffer( "You must specify keywords for your prize.\r\n\r\n" );
            ch->printf( "&RYou are editing %s&R.\r\n", prize->short_descr );
            write_to_buffer( "[SINDHAE] Prizekey: " );
            return;
         }

         if( !str_cmp( argument, "help" ) )
         {
            do_help( ch, "prizekey" );
            ch->printf( "\r\n&RYou are editing %s&R.", prize->short_descr );
            write_to_buffer( "\r\n[SINDHAE] Prizekey: " );
            return;
         }
         stralloc_printf( &prize->name, "%s %s", ch->name, argument.c_str(  ) );
         ch->printf( "\r\nYou chose these keywords: %s\r\n", prize->name );
         write_to_buffer( "Is this correct? (Y/N)" );
         ch->pcdata->spare_ptr = prize;
         connected = CON_CONFIRMPRIZEKEY;
      }
         break;

      case CON_CONFIRMPRIZEKEY:
      {
         obj_data *prize;

         prize = ( obj_data * ) ch->pcdata->spare_ptr;
         if( !prize )
         {
            bug( "%s: Prize object turned NULL somehow!", __FUNCTION__ );
            write_to_buffer( "A fatal internal error has occured. Seek immortal assistance.\r\n" );
            ch->from_room(  );
            if( !ch->to_room( get_room_index( ROOM_VNUM_REDEEM ) ) )
               log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
            connected = CON_PLAYING;
            break;
         }

         switch ( argument[0] )
         {
            case 'y':
            case 'Y':
               write_to_buffer( "\r\nPrize keywords confirmed.\r\n\r\n" );
               ch->pcdata->spare_ptr = NULL;
               ch->from_room(  );
               if( !ch->to_room( get_room_index( ROOM_VNUM_ENDREDEEM ) ) )
                  log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
               interpret( ch, "look" );
               ch->set_color( AT_BLUE );
               if( ch->Class == CLASS_BARD && prize->item_type == ITEM_INSTRUMENT )
                  stralloc_printf( &prize->name, "minstru %s", prize->name );

               write_to_buffer( "Congratulations! You've completed the redemption.\r\n" );
               write_to_buffer( "Your prize should be in your inventory.\r\n" );
               write_to_buffer( "If anything appears wrong, seek immortal assistance.\r\n\r\n" );
               connected = CON_PLAYING;
               break;

            case 'n':
            case 'N':
               write_to_buffer( "\r\nOk, then please enter the correct keywords.\r\n" );
               ch->printf( "\r\n&RYou are editing %s&R.", prize->short_descr );
               write_to_buffer( "[SINDHAE] Prizekey: " );
               connected = CON_PRIZEKEY;
               return;

            default:
               write_to_buffer( "Yes or No? " );
               return;
         }
      }
         break;
         /*
          * End Sindhae Section 
          */

      case CON_RAISE_STAT:
      {
         if( ch->tempnum < 1 )
         {
            bug( "%s: CON_RAISE_STAT: ch loop counter is < 1 for %s", __FUNCTION__, ch->name );
            connected = CON_PLAYING;
            return;
         }

         if( ch->perm_str >= 18 + race_table[ch->race]->str_plus
             && ch->perm_int >= 18 + race_table[ch->race]->int_plus
             && ch->perm_wis >= 18 + race_table[ch->race]->wis_plus
             && ch->perm_dex >= 18 + race_table[ch->race]->dex_plus
             && ch->perm_con >= 18 + race_table[ch->race]->con_plus
             && ch->perm_cha >= 18 + race_table[ch->race]->cha_plus && ch->perm_lck >= 18 + race_table[ch->race]->lck_plus )
         {
            bug( "%s: CON_RAISE_STAT: %s is unable to raise anything.", __FUNCTION__, ch->name );
            write_to_buffer( "All of your stats are already at their maximum values!\r\n" );
            connected = CON_PLAYING;
            return;
         }

         switch ( argument[0] )
         {
            default:
               ch->print( "Invalid choice. Choose 1-7.\r\n" );
               return;

            case '1':
               if( ch->perm_str >= 18 + race_table[ch->race]->str_plus )
                  buffer_printf( "You cannot raise your strength beyond %d\r\n", ch->perm_str );
               else
               {
                  ch->tempnum -= 1;
                  ch->perm_str += 1;
                  buffer_printf( "You've raised your strength to %d!\r\n", ch->perm_str );
               }
               if( ch->tempnum < 1 )
                  connected = CON_PLAYING;
               else
                  show_stats( ch );
               break;

            case '2':
               if( ch->perm_int >= 18 + race_table[ch->race]->int_plus )
                  buffer_printf( "You cannot raise your intelligence beyond %d\r\n", ch->perm_int );
               else
               {
                  ch->tempnum -= 1;
                  ch->perm_int += 1;
                  buffer_printf( "You've raised your intelligence to %d!\r\n", ch->perm_int );
               }
               if( ch->tempnum < 1 )
                  connected = CON_PLAYING;
               else
                  show_stats( ch );
               break;

            case '3':
               if( ch->perm_wis >= 18 + race_table[ch->race]->wis_plus )
                  buffer_printf( "You cannot raise your wisdom beyond %d\r\n", ch->perm_wis );
               else
               {
                  ch->tempnum -= 1;
                  ch->perm_wis += 1;
                  buffer_printf( "You've raised your wisdom to %d!\r\n", ch->perm_wis );
               }
               if( ch->tempnum < 1 )
                  connected = CON_PLAYING;
               else
                  show_stats( ch );
               break;

            case '4':
               if( ch->perm_dex >= 18 + race_table[ch->race]->dex_plus )
                  buffer_printf( "You cannot raise your dexterity beyond %d\r\n", ch->perm_dex );
               else
               {
                  ch->tempnum -= 1;
                  ch->perm_dex += 1;
                  buffer_printf( "You've raised your dexterity to %d!\r\n", ch->perm_dex );
               }
               if( ch->tempnum < 1 )
                  connected = CON_PLAYING;
               else
                  show_stats( ch );
               break;

            case '5':
               if( ch->perm_con >= 18 + race_table[ch->race]->con_plus )
                  buffer_printf( "You cannot raise your constitution beyond %d\r\n", ch->perm_con );
               else
               {
                  ch->tempnum -= 1;
                  ch->perm_con += 1;
                  buffer_printf( "You've raised your constitution to %d!\r\n", ch->perm_con );
               }
               if( ch->tempnum < 1 )
                  connected = CON_PLAYING;
               else
                  show_stats( ch );
               break;

            case '6':
               if( ch->perm_cha >= 18 + race_table[ch->race]->cha_plus )
                  buffer_printf( "You cannot raise your charisma beyond %d\r\n", ch->perm_cha );
               else
               {
                  ch->tempnum -= 1;
                  ch->perm_cha += 1;
                  buffer_printf( "You've raised your charisma to %d!\r\n", ch->perm_cha );
               }
               if( ch->tempnum < 1 )
                  connected = CON_PLAYING;
               else
                  show_stats( ch );
               break;

            case '7':
               if( ch->perm_lck >= 18 + race_table[ch->race]->lck_plus )
                  buffer_printf( "You cannot raise your luck beyond %d\r\n", ch->perm_lck );
               else
               {
                  ch->tempnum -= 1;
                  ch->perm_lck += 1;
                  buffer_printf( "You've raised your luck to %d!\r\n", ch->perm_lck );
               }
               if( ch->tempnum < 1 )
                  connected = CON_PLAYING;
               else
                  show_stats( ch );
               break;
         }
         break;
      }

      case CON_ROLL_STATS:
      {
         switch ( argument[0] )
         {
            case 'y':
            case 'Y':
               write_to_buffer( "\r\nYour stats have been set." );
               connected = CON_PLAYING;
               break;

            case 'n':
            case 'N':
               name_stamp_stats( ch );
               buffer_printf( "\r\nStr: %s\r\n", attribtext( ch->perm_str ).c_str(  ) );
               buffer_printf( "Int: %s\r\n", attribtext( ch->perm_int ).c_str(  ) );
               buffer_printf( "Wis: %s\r\n", attribtext( ch->perm_wis ).c_str(  ) );
               buffer_printf( "Dex: %s\r\n", attribtext( ch->perm_dex ).c_str(  ) );
               buffer_printf( "Con: %s\r\n", attribtext( ch->perm_con ).c_str(  ) );
               buffer_printf( "Cha: %s\r\n", attribtext( ch->perm_cha ).c_str(  ) );
               buffer_printf( "Lck: %s\r\n", attribtext( ch->perm_lck ).c_str(  ) );
               write_to_buffer( "\r\nKeep these stats? (Y/N)" );
               return;

            default:
               write_to_buffer( "Yes or No? " );
               return;
         }
         break;
      }

      case CON_READ_MOTD:
      {
         if( exists_file( SPEC_MOTD ) )
         {
            if( ch->has_pcflag( PCFLAG_ANSI ) )
               ch->pager( "\033[2J" );
            else
               ch->pager( "\014" );

            show_file( ch, SPEC_MOTD );
         }
         buffer_printf( "\r\nWelcome to %s...\r\n", sysdata->mud_name.c_str(  ) );
         char_to_game( ch );
      }
         break;
   }  /* End Nanny switch */
}

CMDF( do_cache )
{
   list < dns_data * >::iterator idns;

   ch->pager( "&YCached DNS Information\r\n" );
   ch->pager( "IP               | Address\r\n" );
   ch->pager( "------------------------------------------------------------------------------\r\n" );
   for( idns = dnslist.begin(  ); idns != dnslist.end(  ); ++idns )
   {
      dns_data *dns = *idns;
      ch->pagerf( "&w%16.16s  &g%s\r\n", dns->ip.c_str(  ), dns->name.c_str(  ) );
   }
   ch->pagerf( "\r\n&W%d IPs in the cache.\r\n", dnslist.size(  ) );
}
