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
 *                       Descriptor Support Functions                       *
 ****************************************************************************/

#include <arpa/telnet.h>
#include <netdb.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <filesystem>
#include <format>
#include <fstream>
#include "mud.h"
#include "ban.h"
#include "channels.h"
#include "commands.h"
#include "connhist.h"
#include "descriptor.h"
#include "fight.h"
#include "new_auth.h"
#include "raceclass.h"
#include "roomindex.h"
#include "sha256.h"
#ifdef MULTIPORT
#include "shell.h"
#endif
#include "auction.h"

struct dns_data
{
   dns_data(  );
   ~dns_data(  );

   std::string ip;
   std::string name;
   std::chrono::system_clock::time_point time;
};

// Login Messages
class lmsg_data
{
private:
   lmsg_data( const lmsg_data & p );
   lmsg_data & operator=( const lmsg_data & );

public:
   lmsg_data(  );
   ~lmsg_data(  );

   char *name;
   char *text;
   short type;
};

int maxdesc, newdesc;
std::list<descriptor_data *> dlist;
std::list<dns_data *> dnslist;
std::list<lmsg_data *> login_messages;

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
auth_data *get_auth_name( std::string_view );
void save_sysdata(  );
void save_auth_list(  );
void check_holiday( char_data * );
void update_connhistory( descriptor_data *, int ); /* connhist.c */
void check_auth_state( char_data * );
void oedit_parse( descriptor_data *, std::string & );
void medit_parse( descriptor_data *, std::string & );
void redit_parse( descriptor_data *, std::string & );
void board_parse( descriptor_data *, const std::string & );
bool check_immortal_domain( char_data *, std::string_view );
void scan_rares( char_data * );
void break_camp( char_data * );
void setup_newbie( char_data *, bool );
void sale_count( char_data * );  /* For new auction system - Samson 6-24-99 */
void check_clan_info( char_data * );
void name_stamp_stats( char_data * );
void quotes( char_data * );
void reset_colors( char_data * );
void mprog_login_trigger( char_data * );
void rprog_login_trigger( char_data * );
CMDF( do_help );
CMDF( do_destroy );
bool check_parse_name( std::string, bool );

/* Terminal detection stuff start */
constexpr unsigned char IS            = '\x00';
constexpr unsigned char TERMINAL_TYPE = '\x18';
constexpr unsigned char SEND          = '\x01';

const std::array<unsigned char, 4> term_call_back_str = { IAC, SB, TERMINAL_TYPE, IS };
const std::array<unsigned char, 7> req_termtype_str = { IAC, SB, TERMINAL_TYPE, SEND, IAC, SE, '\0' };
const std::array<unsigned char, 4> do_term_type = { IAC, DO, TERMINAL_TYPE, '\0' };

/* Terminal detection stuff end */

const std::array<unsigned char, 4> echo_off_str = { IAC, WILL, TELOPT_ECHO, '\0' };
const std::array<unsigned char, 4> echo_on_str = { IAC, WONT, TELOPT_ECHO, '\0' };

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

      ch->printf( "Bytes received     : %ld\r\n", tr_in );
      ch->printf( "Bytes sent         : %ld\r\n", tr_out );
      ch->printf( "Bytes saved by MCCP: %ld\r\n", tr_saved );
   }
   /*
    * Just so we can start it over 
    */
   if( !str_cmp( argument, "clear" ) )
      clear_trdata(  );
}

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
      bug( "%s: }RALERT! Closing socket 0! BAD BAD BAD!", __func__ );
}

descriptor_data::descriptor_data(  )
{
   init_memory( &snoop_by, &disconnect, sizeof( disconnect ) );
   hostname.clear(  );
   ipaddress.clear( );
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
   for( auto it = dlist.begin(); it != dlist.end(); )
   {
      descriptor_data *d = *it;
      ++it;

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
   ifd = -1;   /* Descriptor pipes, used for DNS resolution and such */
   ipid = -1;
   client = "Unidentified";   // Terminal detect
   msp_detected = false;
   can_compress = false;
   is_compressing = false;
   outbuf.clear(  );
   pagebuf.clear(  );
   pageindex = 0;
   mccp = new mccp_data;
}

bool descriptor_data::is_idle_timeout( )
{
   ++idle;

   // Define limits
   int limit = 0;

   if( !character )
      limit = 360;      // 2 mins
   else if( connected != CON_PLAYING )
      limit = 2400; // 10 mins
   else if( character->level < LEVEL_IMMORTAL )
      limit = 14400; // 1 hr
   else
      limit = 32000; // Immortals

   if( idle > limit )
   {
      if( character && character->level >= LEVEL_IMMORTAL )
      {
         idle = 0;
         return false; // Don't kick immortals
      }
      write( "Idle timeout... disconnecting.\r\n" );
      return true; // Kick
   }
   return false;
}

const char *const login_msg[] = {
/*0*/ "",
/*1*/ "\r\n&GYou did not have enough money for the residence you bid on.\r\n"
      "It has been readded to the auction and you've been penalized.\r\n",
/*2*/ "\r\n&GThere was an error in looking up the seller for the residence\r\n"
      "you had bid on. Residence removed and no interaction has taken place.\r\n",
/*3*/ "\r\n&GThere was no bidder on your residence. Your residence has been\r\n"
      "removed from auction and you have been penalized.\r\n",
/*4*/ "\r\n&GYou have successfully received your new residence.\r\n",
/*5*/ "\r\n&GYou have successfully sold your residence.\r\n",
/*6*/ "\r\n&RYou have been outcast from your clan/guild.\r\n"
      "Contact a leader of that organization if you have any questions.\r\n",
/*7*/ "\r\n&RYou have been silenced. Contact an immortal if you wish to discuss your sentence.\r\n",
/*8*/ "\r\n&RYou have lost your ability to set your title. Contact an immortal if you wish to discuss your sentence.\r\n",
/*9*/ "\r\n&RYou have lost your ability to set your bio. Contact an immortal if you wish to discuss your sentence.\r\n",
/*10*/ "\r\n&RYou have been sent to hell. You will be automatically released when your sentence is up.\r\n"
      "Contact an immortal if you wish to discuss your sentence.\r\n",
/*11*/ "\r\n&RYou have lost your ability to set your own description.\r\n"
      "Contact an immortal if you wish to discuss your sentence.\r\n",
/*12*/ "\r\n&RYou have lost your ability to set your homepage address.\r\n"
      "Contact an immortal if you wish to discuss your sentence.\r\n",
/*13*/ "\r\n&RYou have lost your ability to beep other players.\r\n"
      "Contact an immortal if you wish to discuss your sentence.\r\n",
/*14*/ "\r\n&RYou have lost your ability to send tells. Contact an immortal if you wish to discuss your sentence.\r\n",
/*15*/ "\r\n&CYour character has been frozen. Contact an immortal if you wish to discuss your sentence.\r\n",
/*16*/ "\r\n&RYou have lost your ability to emote. Contact an immortal if you wish to discuss your sentence.\r\n",
/*17*/ "RESERVED FOR LINKDEAD DEATH MESSAGES",
/*18*/ "RESERVED FOR CODE-SENT MESSAGES"
};

/* MAX_MSG = 18 - IF ADDING MESSAGE TYPES, ENSURE YOU BUMP THIS VALUE IN MUDCFG.H */

lmsg_data::~lmsg_data(  )
{
   STRFREE( this->name );
   STRFREE( this->text );
}

lmsg_data::lmsg_data(  )
{
   init_memory( &name, &type, sizeof( type ) );
}

void fread_loginmsg( FILE * fp )
{
   lmsg_data *lmsg = nullptr;

   lmsg = new lmsg_data;

   for( ;; )
   {
      const char *word = feof( fp ) ? "End" : fread_word( fp );

      switch ( to_upper( word[0] ) )
      {
         default:
            bug( "%s: no match: %s", __func__, word );
            fread_to_eol( fp );
            break;

         case '*':
            fread_to_eol( fp );
            break;

         case 'E':
            if( !str_cmp( word, "End" ) )
            {
               if( !lmsg->name || lmsg->name[0] == '\0' )
               {
                  bug( "%s: Login message with no name", __func__ );
                  deleteptr( lmsg );
                  return;
               }
               else
               {
                  if( !exists_player( lmsg->name ) )
                  {
                     bug( "%s: Login message expired - %s no longer exists", __func__, lmsg->name );

                     deleteptr( lmsg );
                     return;
                  }
               }

               login_messages.push_back( lmsg );
               return;
            }
            break;

         case 'N':
            KEY( "Name", lmsg->name, fread_string( fp ) );
            break;

         case 'T':
            KEY( "Type", lmsg->type, fread_short( fp ) );
            KEY( "Text", lmsg->text, fread_string( fp ) );
            break;
      }
   }
}

/* load_loginmsg, check_loginmsg, fread_loginmsg, etc.. all support the do_message */
/* command - hugely modified from the original housing module by Edmond June 02     */
void load_loginmsg(  )
{
   FILE *fp;

   login_messages.clear();

   std::filesystem::path filename = std::format( "{}{}", SYSTEM_DIR, LOGIN_MSG );
   if( ( fp = fopen( filename.c_str(), "r" ) ) == nullptr )
   {
      bug( "%s: Cannot open login message file.", __func__ );
      return;
   }

   for( ;; )
   {
      char letter;
      const char *word;

      letter = fread_letter( fp );

      if( letter == '*' )
      {
         fread_to_eol( fp );
         continue;
      }

      if( letter != '#' )
      {
         bug( "%s: # not found. ", __func__ );
         break;
      }

      word = fread_word( fp );

      if( !str_cmp( word, "LOGINMSG" ) )
      {
         fread_loginmsg( fp );
         continue;
      }
      else if( !str_cmp( word, "END" ) )
         break;
      else
      {
         bug( "%s: bad section: %s", __func__, word );
         continue;
      }
   }

   FCLOSE( fp );
}

void save_loginmsg(  )
{
   FILE *fp;

   std::filesystem::path filename = std::format( "{}{}", SYSTEM_DIR, LOGIN_MSG );
   if( ( fp = fopen( filename.c_str(), "w" ) ) == nullptr )
   {
      bug( "%s: Cannot open login message file.", __func__ );
      return;
   }

   for( auto* lmsg : login_messages )
   {
      fprintf( fp, "%s", "#LOGINMSG\n" );
      fprintf( fp, "Name  %s~\n", lmsg->name );
      if( lmsg->text )
         fprintf( fp, "Text  %s~\n", lmsg->text );
      fprintf( fp, "Type  %d\n", lmsg->type );
      fprintf( fp, "%s", "End\n" );
   }

   fprintf( fp, "%s", "#END\n" );
   FCLOSE( fp );
}

void add_loginmsg( const char *name, short type, const char *argument )
{
   lmsg_data *lmsg;

   if( type < 0 || !name || name[0] == '\0' )
   {
      bug( "%s: bad name or type", __func__ );
      return;
   }

   lmsg = new lmsg_data;

   lmsg->type = type;
   lmsg->name = STRALLOC( name );
   if( argument && argument[0] != '\0' )
      lmsg->text = STRALLOC( argument );

   login_messages.push_back( lmsg );
   save_loginmsg(  );
}

void check_loginmsg( char_data * ch )
{
   if( !ch || ch->isnpc() )
      return;

   for( auto it = login_messages.begin(); it != login_messages.end(); )
   {
      lmsg_data *lmsg = *it;
      ++it;

      if( !str_cmp( lmsg->name, ch->name ) )
      {
         if( lmsg->type > MAX_MSG )
            bug( "%s: Error: Unknown login msg: %d for %s.", __func__, lmsg->type, ch->name );

         switch ( lmsg->type )
         {
            case 0: /* Imm sent message */
            {
               if( !lmsg->text || lmsg->text[0] == '\0' )
               {
                  bug( "%s: nullptr loginmsg text for type 0", __func__ );

                  login_messages.remove( lmsg );
                  deleteptr( lmsg );
                  continue;
               }
               ch->printf( "\r\n&YThe game administrators have left you the following message:\r\n%s\r\n", lmsg->text );
               break;
            }

            case 17:   /* Death message */
            {
               if( !lmsg->text || lmsg->text[0] == '\0' )
               {
                  bug( "%s: nullptr loginmsg text for type 17", __func__ );

                  login_messages.remove( lmsg );
                  deleteptr( lmsg );
                  continue;
               }
               ch->printf( "\r\n&RYou were killed by %s while your character was link-dead.\r\n", lmsg->text );
               ch->print( "You should look for your corpse immediately.\r\n" );
               break;
            }

            case 18:   /* Code-sent message for 'World change' notice */
            {
               if( !lmsg->text || lmsg->text[0] == '\0' )
               {
                  bug( "%s: nullptr loginmsg text for type 18", __func__ );

                  login_messages.remove( lmsg );
                  deleteptr( lmsg );
                  continue;
               }
               ch->printf( "\r\n&GA change in the Realms has affected you personally:\r\n%s\r\n", lmsg->text );
               break;
            }

            default:
               ch->print( login_msg[lmsg->type] );
               break;
         }

         login_messages.remove( lmsg );
         deleteptr( lmsg );

         save_loginmsg(  );
      }
   }
}

/*Prompt, fprompt made to include exp and victim's condition, prettyfied too - Adjani, 12-07-2002*/
char *default_fprompt( char_data * ch )
{
   static char buf[60];

   strlcpy( buf, "&z[&W%h&whp ", 60 );
   strlcat( buf, "&W%m&wm", 60 );
   strlcat( buf, " &W%v&wmv&z] ", 60 );
   strlcat( buf, " [&R%c&z] ", 60 );
   if( ch->isnpc(  ) || ch->is_immortal(  ) )
      strlcat( buf, "&W%i%R&D", 60 );
   return buf;
}

char *default_prompt( char_data * ch )
{
   static char buf[60];

   strlcpy( buf, "&z[&W%h&whp ", 60 );
   strlcat( buf, "&W%m&wm", 60 );
   strlcat( buf, " &W%v&wmv&z] ", 60 );
   strlcat( buf, " [&c%X&wexp&z]&D ", 60 );
   if( ch->isnpc(  ) || ch->is_immortal(  ) )
      strlcat( buf, "&W%i%R&D", 60 );
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
 *
 * Added block checking to prevent random booting of the descriptor. Thanks go
 * out to Rustry for his suggestions. -Orion
 */
bool write_to_descriptor_old( int desc, std::string_view text )
{
   if( text.empty() )
      return true;

   size_t offset = 0;

   while( offset < text.size() )
   {
      size_t nBlock = std::min( text.size() - offset, size_t{4096} );

      int nWrite = send( desc, text.data() + offset, static_cast<int>( nBlock ), 0 );

      if( nWrite > 0 )
      {
         update_trdata(2, nWrite);
         offset += nWrite;
      }
      else if( nWrite == -1 )
      {
         int iErr = errno;

         if( iErr == EWOULDBLOCK || iErr == EAGAIN )
         {
            // Non-blocking: continue loop to try again later
            continue;
         }
         else
         {
            perror( "write_to_descriptor_old" );
            return false;
         }
      }
      else if( nWrite == 0 )
      {
         // Socket closed by peer
         return false;
      }
   }
   return true;
}

/*
 * This function handles both compressed and uncompressed sending.
 * Updated to run with the block checks by Orion... if it doesn't work, blame him.;P -Orion
 */
bool descriptor_data::write( std::string_view text )
{
   if( text.empty() )
      return true;

   size_t length = text.size();
   size_t mccpsaved = length;

   // Won't send more then it has to so make sure we check if its under length.
   if( mccpsaved > length )
      mccpsaved = length;

   // Lambda to encapsulate the repetitive non-blocking send logic
   auto send_chunk = [this]( const char* data, size_t len ) -> int
   {
      int nWrite = send( this->descriptor, data, static_cast<int>(len), 0 );

      if( nWrite > 0 )
      {
         update_trdata( 2, nWrite );
      }
      else if( nWrite == -1 && ( errno == EWOULDBLOCK || errno == EAGAIN ) )
      {
         return 0; // Would block, try again later
      }
      else
      {
         perror( "Write_to_descriptor" );
         return -1; // Fatal error
      }
      return nWrite;
   };

   // Use this if MCCP compression is enabled for the descriptor.
   if( mccp->out_compress )
   {
      auto& z = *mccp->out_compress;
      z.next_in = reinterpret_cast<unsigned char*>( const_cast<char*>( text.data() ) );
      z.avail_in = static_cast<uInt>( text.size() );

      while( z.avail_in > 0 )
      {
         z.avail_out = static_cast<uInt>( COMPRESS_BUF_SIZE - ( z.next_out - mccp->out_compress_buf ) );

         if( z.avail_out > 0 )
         {
            if( deflate( &z, Z_SYNC_FLUSH ) != Z_OK )
               return false;
         }

         size_t buffered_len = z.next_out - mccp->out_compress_buf;
         size_t bytes_sent_total = 0;

         while( bytes_sent_total < buffered_len )
         {
            size_t nBlock = std::min( buffered_len - bytes_sent_total, size_t{4096} );
            int nWrite = send_chunk( reinterpret_cast<char*>(mccp->out_compress_buf + bytes_sent_total), nBlock );

            if( nWrite == -1 )
               return false;
            if( nWrite == 0 )
               break;

            bytes_sent_total += nWrite;
            mccpsaved -= nWrite;
         }

         if( bytes_sent_total > 0 )
         {
            size_t remaining = buffered_len - bytes_sent_total;

            std::memmove( mccp->out_compress_buf, mccp->out_compress_buf + bytes_sent_total, remaining );
            z.next_out = mccp->out_compress_buf + remaining;
         }
         else
         {
            break; // Could not send anything, exit compression loop to wait
         }
      }
      if( mccpsaved > 0 )
         update_trdata( 3, mccpsaved );
      return true;
   }

   // If you end up down here, then text is being sent uncompressed.
   size_t offset = 0;
   while( offset < text.size() )
   {
      size_t nBlock = std::min( text.size() - offset, size_t{4096} );
      int nWrite = send_chunk( text.data() + offset, nBlock );

      if( nWrite == -1 )
         return false;
      offset += nWrite;
   }
   return true;
}

bool descriptor_data::read( )
{
   /*
    * Hold horses if pending command already.
    */
   if( !this->incomm.empty() )
      return true;

   size_t iStart = std::strlen( this->inbuf );
   const size_t buffer_limit = sizeof( this->inbuf ) - 10;

   if( iStart >= buffer_limit )
   {
      log_printf( "%s input overflow!", this->hostname.c_str() );
      this->write( "\r\n*** PUT A LID ON IT!!! ***\r\n" );
      return false;
   }

   while( true )
   {
      ssize_t nRead = recv( this->descriptor, this->inbuf + iStart, buffer_limit - iStart, 0 );

      if( nRead > 0 )
      {
         iStart += nRead;
         this->inbuf[iStart] = '\0';

         update_trdata( 1, static_cast<size_t>( nRead ) );

         if( this->inbuf[iStart - 1] == '\n' || this->inbuf[iStart - 1] == '\r' )
            break;

         continue;
      }
      else if( nRead == 0 )
      {
         if( this->connected >= CON_PLAYING )
         {
            log_string_plus( LOG_COMM, LEVEL_IMMORTAL, "EOF encountered on read." );
            return false;
         }
         break;
      }
      else // nRead < 0
      {
         int iErr = errno;

         if( iErr == EWOULDBLOCK || iErr == EAGAIN )
            break; // Kernel buffer empty

         log_printf_plus( LOG_COMM, LEVEL_IMMORTAL, "%s: Descriptor error on #%d", __func__, this->descriptor );
         log_printf_plus( LOG_COMM, LEVEL_IMMORTAL, "Descriptor belongs to: %s", ( this->character && this->character->name ) ? this->character->name : this->hostname.c_str(  ) );
         perror( __func__ );
         return false;
      }
   }
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
      std::string_view sv( this->outbuf );
      std::string_view chunk = sv.substr( 0, 4096 );

      if( snoop_by )
      {
         if( character && character->name )
         {
            if( original && original->name )
               snoop_by->buffer_printf( "{} ({})", character->name, original->name );
            else
               snoop_by->write_to_buffer( character->name );
         }
         snoop_by->write_to_buffer( "% " );
         snoop_by->write_to_buffer( std::string( chunk ) );
      }

      this->outbuf.erase( 0, 4096 );

      if( !this->write( std::string( chunk ) ) )
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
            snoop_by->buffer_printf( "{} ({})", character->name, original->name );
         else
            snoop_by->write_to_buffer( character->name );
      }
      snoop_by->write_to_buffer( "% " );
      snoop_by->write_to_buffer( outbuf );
   }

   /*
    * OS-dependent output.
    */
   if( !this->write( this->outbuf ) )
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
void descriptor_data::read_from_buffer( )
{
   /*
    * Hold horses if pending command already.
    */
   if( !this->incomm.empty(  ) )
      return;

   /*
    * Thanks Nick! [Even though this has been refactored :P]
    */
   for( size_t i = 0; i < MAX_INBUF_SIZE && inbuf[i] != '\0'; ++i )
   {
      if( static_cast<unsigned char>( inbuf[i] ) == IAC )
      {
         if( std::memcmp( &inbuf[i], term_call_back_str.data(), term_call_back_str.size() ) == 0 )
         {
            char tmp[100]{};
            size_t pos = i;
            size_t p_idx = i + sizeof( term_call_back_str );
            size_t x = 0;

            while( x < ( sizeof(tmp) - 1 ) && inbuf[p_idx] != '\0' && static_cast<unsigned char>( inbuf[p_idx] ) != IAC )
               tmp[x++] = inbuf[p_idx++];

            tmp[x] = '\0';
            if( tmp[0] != '\0' )
               this->client = tmp;

            p_idx += 2; // Skip IAC and SE
            size_t len = ( MAX_INBUF_SIZE - p_idx );
            std::memmove( &inbuf[pos], &inbuf[p_idx], len );
            std::memset (&inbuf[MAX_INBUF_SIZE - ( p_idx - pos )], 0, ( p_idx - pos ) );
         }
      }
   }

   /*
    * Look for at least one new line.
    */
   size_t i = 0;
   while( i < MAX_INBUF_SIZE && inbuf[i] != '\n' && inbuf[i] != '\r' && inbuf[i] != '\0' )
      ++i;

   if( i >= MAX_INBUF_SIZE || inbuf[i] == '\0' )
      return;

   /*
    * Canonical input processing.
    */
   int k = 0;
   int iac = 0;
   for( size_t idx = 0; idx < i; ++idx )
   {
      if( k >= MIL / 2 )
      {
         this->write( "Line too long.\r\n" );
         inbuf[idx] = '\n';
         inbuf[idx + 1] = '\0';
         break;
      }

      if( this->can_compress && !this->is_compressing )
         this->compressStart();

      unsigned char c = static_cast<unsigned char>( inbuf[idx]) ;

      if( c == IAC )
         iac = 1;
      else if( iac == 1 && ( c == DO || c == DONT || c == WILL ) )
         iac = 2;
      else if( iac == 2 )
      {
         iac = 0;

         if( c == TELOPT_COMPRESS2 )
         {
            if( static_cast<unsigned char>( inbuf[idx - 1] ) == DO )
            {
               if( this->compressStart() )
                  this->can_compress = true;
            }
            else if( static_cast<unsigned char>( inbuf[idx - 1] ) == DONT )
            {
               if( this->compressEnd() )
                  this->can_compress = false;
            }
         }
         else if( c == TELOPT_MSP )
         {
            if( static_cast<unsigned char>( inbuf[idx - 1] ) == DO )
            {
               this->msp_detected = true;
               this->send_msp_startup();
            }
            else if( static_cast<unsigned char>( inbuf[idx - 1] ) == DONT )
               this->msp_detected = false;
         }
         else if( c == TERMINAL_TYPE )
         {
            if( static_cast<unsigned char>( inbuf[idx - 1] ) == WILL )
               this->write_to_buffer( std::string_view{ reinterpret_cast<const char*>( req_termtype_str.data() ), req_termtype_str.size() } );
         }
      }
      else if( c == '\b' && k > 0 )
      {
         --k;
         this->incomm.pop_back();
      }
      /*
       * Note to the future curious: Leave this alone. Extended ascii isn't standardized yet.
       * You'd think being the 21st century and all that this wouldn't be the case, but you can
       * thank the bastards in Redmond for this.
       */
      else if( isascii(c) && isprint(c) )
      {
         this->incomm += static_cast<char>(c);
         ++k;
      }
   }

   /*
    * Finish off the line.
    */
   if( k == 0 )
   {
      this->incomm += ' ';
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
          * As we know, code rarely does what we expect, and there could still be problems here.
          * The only other solution seen as viable beyond this is to remove the spamguard entirely.
          */
         cmd_type* cmd = nullptr;
         std::string c_str = this->incomm, arg;

         c_str = one_argument( c_str, arg );
         cmd = find_command( arg );

         if( !cmd && this->character && this->character->pcdata->alias_map.contains( arg ) )
         {
            auto al = this->character->pcdata->alias_map.find( arg );
            if( !al->second.empty() )
            {
               std::string d = al->second, arg2;
               d = one_argument( d, arg2 );
               cmd = find_command( arg2 );
            }
         }

         if( !cmd )
         {
            if( find_channel( arg ) != nullptr && !str_cmp( this->incomm, this->inlast ) )
               ++this->repeat;
         }
         else if( cmd->flags.test(CMD_NOSPAM) && !str_cmp( this->incomm, this->inlast ) )
            ++this->repeat;

         // Your first warning...
         if( this->repeat == 3 && this->character && this->character->level < LEVEL_IMMORTAL )
            this->character->print( "}R\r\nYou have repeated the same command 3 times now.\r\nRepeating it 7 more will result in an autofreeze by the spamguard code.&D\r\n" );

         // Second warning...
         if( this->repeat == 6 && this->character && this->character->level < LEVEL_IMMORTAL )
            this->character->print( "}R\r\nYou have repeated the same command 6 times now.\r\nRepeating it 4 more will result in an autofreeze by the spamguard code.&D\r\n" );

         // You can't say we didn't warn you!
         if( this->repeat >= 10 && this->character && this->character->level < LEVEL_IMMORTAL )
         {
            affect_data af;

            ++this->character->pcdata->spam;

            log_printf( "%s was autofrozen by the spamguard - spamming: %s", this->character->name, this->incomm.c_str(  ) );
            log_printf( "%s has spammed %d times this login.", this->character->name, this->character->pcdata->spam );

            this->write( "\r\n*** PUT A LID ON IT!!! ***\r\nYou cannot enter the same command more than 10 consecutive times!\r\n" );
            this->write( "The Spamguard has spoken!\r\n" );
            this->write( "It suddenly becomes very cold, and very QUIET!\r\n" );
            this->incomm = "spam";

            af.type = skill_lookup( "spamguard" );
            af.duration = 115 * this->character->pcdata->spam; // One game hour per offense, this can get ugly FAST
            af.modifier = 0;
            af.location = APPLY_NONE;
            af.bit = AFF_SPAMGUARD;

            this->character->affect_to_char( &af );
            this->character->set_pcflag(PCFLAG_IDLING);
            this->repeat = 0; // Just so it doesn't get haywire
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

   size_t shift_i = i;

   while( inbuf[shift_i] == '\n' || inbuf[shift_i] == '\r' )
      ++shift_i;

   size_t move_len = MAX_INBUF_SIZE - shift_i;
   std::memmove( inbuf, &inbuf[shift_i], move_len );
   std::memset( &inbuf[move_len], 0, shift_i );
}

/*
 * Append onto an output buffer.
 */
void descriptor_data::write_to_buffer( std::string_view text )
{
   if( MPSilent )
      return;

   /*
    * Normally a bug... but can happen if loadup is used.
    */
   if( this->outbuf.empty(  ) && !this->fcommand )
      this->outbuf = "\r\n";

   this->outbuf.append( text );
}

/* Writes to a descriptor, usually best used when there's no character to send to ( like logins ) */
void descriptor_data::send_color( std::string_view text )
{
   if( text.empty(  ) || !this->descriptor )
      return;

   this->write_to_buffer( colorize( text, this ) );
}

void descriptor_data::pager( std::string_view text )
{
   if( this->pagebuf.empty(  ) && !this->fcommand )
      this->pagebuf = "\r\n";

   this->pagebuf.append( text );
}

void descriptor_data::set_pager_input( std::string_view argument )
{
   std::string arg = std::string{argument};

   strip_lspace( arg );
   this->pagecmd = arg.front();
}

bool descriptor_data::pager_output(  )
{
   char_data *ch;
   int pclines;
   size_t start, end;
   bool ret;

   if( this->pagebuf.empty(  ) || this->pagecmd == -1 )
      return true;

   ch = this->original ? this->original : this->character;
   pclines = umax( ch->pcdata->pagerlen, 5 );

   switch ( to_lower( this->pagecmd ) )
   {
      default:
         start = this->pageindex;
         end = start + pclines;
         break;

      case 'b':
         start = this->pageindex - ( pclines * 2 );
         end = this->pageindex - ( pclines + 2 );
         break;

      case 'r':
         start = this->pageindex - pclines;
         end = this->pageindex - 1;
         break;

      case 'n':
         start = this->pageindex;
         end = 0x7FFFFFFF;   /* As many lines as possible */
         break;

      case 'q':
         this->flush_buffer( true );
         this->pagebuf.clear(  );
         this->pageindex = 0;
         this->pagecmd = 0;
         return true;
   }

   // This is going to get seriously messed up if people use the wrong line termination.
   std::vector<std::string> pagelines = string_explode( this->pagebuf, '\n' );

   if( end > pagelines.size() )
      end = pagelines.size();

   for( size_t x = start; x <= end; ++x )
   {
      if( x >= pagelines.size(  ) )
      {
         this->flush_buffer( true );
         this->pagebuf.clear(  );
         this->pageindex = 0;
         this->pagecmd = 0;
         return true;
      }

      this->write( pagelines[x].c_str(  ) );
      this->pageindex = x + 1;
   }

   this->pagecmd = -1;
   if( ch->has_pcflag( PCFLAG_ANSI ) )
      if( !this->write( ANSI_LBLUE ) )
         return false;

   if( ( ret = this->write( "(C)ontinue, (N)on-stop, (R)efresh, (B)ack, (Q)uit: [C] " ) ) == false )
      return false;

   if( ch->has_pcflag( PCFLAG_ANSI ) )
   {
      ret = this->write( ch->color_str( this->pagecolor ) );
   }
   return ret;
}

void descriptor_data::send_greeting(  )
{
   std::filesystem::path filename = GREETING_FILE;

   // Read the file directly into a std::string
   if( std::ifstream in{ filename, std::ios::in | std::ios::binary } )
   {
      std::string content;

      // Efficiently read the entire file into the string
      content.assign( std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>() );

      if( !content.empty() )
      {
         send_color( content.c_str() );
      }
   }
}

/*
 * Dump a text file to a player, a line at a time		-Thoric
 */
// NOTE: Must be passed directly to d->write() because the colorize function eats ANSI codes and such. - Samson 6/4/2026.
void show_file( char_data * ch, std::string_view filename )
{
   if( !ch || !ch->desc )
      return;

   std::ifstream fp{ filename.data() };
   if( !fp.is_open() )
      return;

   std::string line;
   std::string output;

   while( std::getline( fp, line ) )
   {
      if( !line.empty() && line.back() == '\r' )
         line.pop_back();

      output += line + "\r\n";
   }

   // Write the formatted content directly to the descriptor.
   output.insert( 0, "\033[H" );
   ch->desc->write( output );
}

void descriptor_data::show_title(  )
{
   char_data *ch = character;

   if( !ch->has_pcflag( PCFLAG_NOINTRO ) )
      show_file( ch, ANSITITLE_FILE );
   else
      write_to_buffer( "Press enter...\r\n" );
   connected = CON_PRESS_ENTER;
}

void descriptor_data::show_stats( char_data * ch )
{
   ch->set_color( AT_SKILL );
   buffer_printf( "\r\n1. Str: {}\r\n", ch->perm_str );
   buffer_printf( "2. Int: {}\r\n", ch->perm_int );
   buffer_printf( "3. Wis: {}\r\n", ch->perm_wis );
   buffer_printf( "4. Dex: {}\r\n", ch->perm_dex );
   buffer_printf( "5. Con: {}\r\n", ch->perm_con );
   buffer_printf( "6. Cha: {}\r\n", ch->perm_cha );
   buffer_printf( "7. Lck: {}\r\n\r\n", ch->perm_lck );
   ch->set_color( AT_ACTION );
   write_to_buffer( "Choose an attribute to raise by +1\r\n" );
}

dns_data::dns_data(  )
{
   time = std::chrono::system_clock::time_point{};
}

dns_data::~dns_data(  )
{
   dnslist.remove( this );
}

void free_dns_list( void )
{
   for( auto it = dnslist.begin(  ); it != dnslist.end(  ); )
   {
      dns_data *ip = *it;
      ++it;

      deleteptr( ip );
   }
}

void save_dns( void )
{
   std::ofstream stream;

   stream.open( std::filesystem::path( DNS_FILE ) );
   if( !stream.is_open(  ) )
   {
      bug( "%s: Cannot open DND cache file.", __func__ );
      return;
   }

   for( auto* ip : dnslist )
   {
      stream << "#CACHE" << std::endl;
      stream << "IP   " << ip->ip << std::endl;
      stream << "Name " << ip->name << std::endl;
      stream << "Time " << ip->time << std::endl;
      stream << "End" << std::endl << std::endl;
   }
   stream.close(  );
}

void prune_dns( void )
{
   constexpr auto CACHE_DURATION = std::chrono::days{14};

   for( auto it = dnslist.begin(  ); it != dnslist.end(  ); )
   {
      dns_data *cache = *it;
      ++it;

      /*
       * Stay in cache for 14 days 
       */
      if( current_time - cache->time >= CACHE_DURATION || !str_cmp( cache->ip, "Unknown??" ) || !str_cmp( cache->name, "Unknown??" ) )
         deleteptr( cache );
   }
   save_dns(  );
}

void add_dns( std::string_view dhost, std::string_view address )
{
   dns_data *cache;

   cache = new dns_data;
   cache->ip = dhost;
   cache->name = address;
   cache->time = current_time;
   dnslist.push_back( cache );

   save_dns(  );
}

std::string in_dns_cache( std::string_view ip )
{
   for( auto* dns : dnslist )
   {
      if( !str_cmp( ip, dns->ip ) )
         return dns->name;
   }
   return "";
}

void load_dns( void )
{
   dns_data *cache = nullptr;
   std::ifstream stream;

   dnslist.clear(  );

   stream.open( std::filesystem::path( DNS_FILE ) );
   if( stream.is_open(  ) )
   {
      do
      {
         std::string key, value;
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
         {
            time_t loaded_time = std::stol( value );
            cache->time = std::chrono::system_clock::from_time_t( loaded_time );
         }
         else if( key == "End" )
         {
            if( cache->ip.empty(  ) )
               cache->ip = "Unknown??";
            if( cache->name.empty(  ) )
               cache->name = "Unknown??";

            dnslist.push_back( cache );
         }
         else
            log_printf( "%s: Bad line in DNS cache file: %s %s", __func__, key.c_str(  ), value.c_str(  ) );
      }
      while( !stream.eof(  ) );
      stream.close(  );
   }
   prune_dns(  ); /* Clean out entries beyond 14 days */
}

/*
 * DNS Resolver code by Trax of Forever's End
 * Almost the same as read_from_buffer...
 */
// LOL, well, not so much now I think... - Samson 6/3/2026
bool read_from_dns( int fd, std::string & output )
{
   std::vector<char> buffer(4096);

   ssize_t nRead = read( fd, buffer.data(), buffer.size() );

   if( nRead > 0 )
   {
      output.assign( buffer.data(), nRead );

      std::erase_if( output, [](char c ) { return c == '\r' || c == '\n'; } );

      return !output.empty();
   }
   else if( nRead == -1 && errno != EWOULDBLOCK )
   {
      perror( "Read_from_dns" );
   }
   return false;
}

/* DNS Resolver code by Trax of Forever's End */
/*
 * Process input that we got from resolve_dns.
 */
void descriptor_data::process_dns(  )
{
   std::string address;
   int status;

   address = "";

   if( !read_from_dns( ifd, address ) || address.empty() )
      return;

   if( !address.empty() )
   {
      /*
       * The resolver will only return 2 error states, described in the string comparisons here.
       * If either of these come back from it, do not add it to the cache, and do not set the host on the descriptior to it either.
       * The descriptor's IP will have alredy been set by default before the resolver was called.
       */
      if( str_cmp( address, "bad.resolver.call" ) && str_cmp( address, "somehow.has.no.ip?" ) )
      {
         add_dns( hostname, address );
         hostname = address;
         if( character && character->pcdata )
            character->pcdata->lasthost = address;
      }
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
void descriptor_data::resolve_dns( const std::string & ip )
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

   fcntl( fds[0], F_SETFL, O_NONBLOCK );

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
      int i;

      ifd = fds[0];
      ipid = pid;

      for( i = 2; i < 255; ++i )
         close( i );

#if defined(__CYGWIN__)
      execl( "../src/resolver.exe", "AFKMud Resolver", ip.c_str(), ( char * )nullptr );
#else
      execl( "../src/resolver", "AFKMud Resolver", ip.c_str(), ( char * )nullptr );
#endif
      /*
       * Still here --> hmm. An error. 
       */
      bug( "%s: Exec failed; Closing child.", __func__ );
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

void new_descriptor( int new_desc )
{
   descriptor_data *dnew;
   struct sockaddr_in6 sock;
   int desc;
   char host_buf[NI_MAXHOST];
   int r;
   socklen_t size;

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

   r = fcntl( desc, F_GETFL, 0 );
   if( r < 0 || fcntl( desc, F_SETFL, O_NONBLOCK | O_NDELAY | r ) < 0 )
   {
      perror( "New_descriptor: fcntl: O_NONBLOCK" );
      close( desc );
      return;
   }

   if( check_bad_desc( new_desc ) )
      return;

   dnew = new descriptor_data;
   dnew->init(  );

   if( getnameinfo( ( struct sockaddr * )&sock, size, host_buf, sizeof( host_buf ), NULL, 0, NI_NUMERICHOST ) == 0 )
   {
      /*
       * If using a dual-stack socket, IPv4 addresses often appear as
       * ::ffff:192.168.1.1. We can strip the prefix if desired,
       * but getnameinfo provides the clean format by default.
       */
      char *final_ip = host_buf;

      // Optional: Strip "::ffff:" prefix if you strictly want IPv4 notation
      if( strncmp( host_buf, "::ffff:", 7 ) == 0 )
      {
         final_ip = host_buf + 7;
      }

      dnew->ipaddress = final_ip;
   }
   else
   {
      dnew->ipaddress = "unknown";
   }

   if( desc == 0 )
   {
      bug( "%s: }RALERT! Assigning socket 0! BAD BAD BAD! Host: %s", __func__, dnew->ipaddress.c_str( ) );
      deleteptr( dnew );
      set_alarm( 0 );
      return;
   }
   dnew->descriptor = desc;
   dnew->client_port = ntohs( sock.sin6_port );
   dnew->hostname = dnew->ipaddress;

   if( !sysdata->NO_NAME_RESOLVING )
   {
      std::string buf = in_dns_cache( dnew->ipaddress );

      if( buf.empty(  ) )
         dnew->resolve_dns( dnew->ipaddress );
      else
         dnew->hostname = buf;
   }

   // Ban notice is given in the ban.cpp file
   if( is_banned( dnew ) )
   {
      deleteptr( dnew );
      set_alarm( 0 );
      return;
   }

   if( dlist.empty(  ) )
      log_string( "Waking up autonomous update handlers." );

   dlist.push_back( dnew );

   /*
    * Terminal detect 
    */
   dnew->write_to_buffer( std::string_view{ reinterpret_cast<const char*>( do_term_type.data() ), do_term_type.size() } );

   /*
    * MCCP Compression 
    */
   dnew->write_to_buffer( std::string_view{ reinterpret_cast<const char*>( will_compress2_str.data() ), will_compress2_str.size() } );

   /*
    * Mud Sound Protocol 
    */
   dnew->write_to_buffer( std::string_view{ reinterpret_cast<const char*>( will_msp_str.data() ), will_msp_str.size() } );

   /*
    * Send the greeting. No longer handled kludgely by a global variable.
    */
   dnew->send_greeting(  );

   dnew->send_color( "Enter your character's name, or type new: &d" );

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

   for( auto* d : dlist )
   {
      maxdesc = umax( maxdesc, d->descriptor );
      FD_SET( d->descriptor, &in_set );
      FD_SET( d->descriptor, &out_set );
      FD_SET( d->descriptor, &exc_set );

      if( d->ifd != -1 && d->ipid != -1 )
      {
         maxdesc = umax( maxdesc, d->ifd );
         FD_SET( d->ifd, &in_set );
      }
   }

   if( select( maxdesc + 1, &in_set, &out_set, &exc_set, &null_time ) < 0 )
   {
      perror( "accept_new: select: poll" );
      exit( 1 );
   }

   if( FD_ISSET( ctrl, &exc_set ) )
   {
      bug( "%s: Exception raised on controlling descriptor %d", __func__, ctrl );
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
   if( !character )
   {
      bug( "%s: nullptr character", __func__ );
      return;
   }

   auto* ch = character;
   auto* och = original ? original : character;

   std::string_view cprompt;

   if( !ch->has_pcflag( PCFLAG_HELPSTART ) )
      cprompt = "&w[Type HELP START]";
   else if( !ch->isnpc() && ch->substate != SUB_NONE && ch->pcdata->subprompt && *ch->pcdata->subprompt )
      cprompt = ch->pcdata->subprompt;
   else if( ch->isnpc() || ( !ch->fighting && ( !ch->pcdata->prompt || !*ch->pcdata->prompt ) ) )
      cprompt = default_prompt( ch );
   else if( ch->fighting )
      cprompt = ( ch->pcdata->fprompt && *ch->pcdata->fprompt) ? ch->pcdata->fprompt : default_fprompt( ch );
   else
      cprompt = ch->pcdata->prompt;

   std::string output;
   if( och->has_pcflag( PCFLAG_ANSI ) )
   {
      output += ANSI_RESET;
      prevcolor = 0x08;
   }

   auto get_health_string = [](int hit, int max_hit) -> std::string_view
   {
      int percent = ( 100 * hit ) / max_hit;

      if( max_hit <= 0 )
         return "DYING";

      if( percent >= 100 )
         return "perfect health";
      if( percent >= 90 )
         return "slightly scratched";
      if( percent >= 80 )
         return "few bruises";
      if( percent >= 70 )
         return "some cuts";
      if( percent >= 60 )
         return "several wounds";
      if( percent >= 50 )
         return "nasty wounds";
      if( percent >= 40 )
         return "bleeding freely";
      if( percent >= 30 )
         return "covered in blood";
      if( percent >= 20 )
         return "leaking guts";
      if( percent >= 10 )
         return "almost dead";
      return "DYING";
   };

   for( size_t i = 0; i < cprompt.length(); ++i )
   {
      if( cprompt[i] != '%' || ( i + 1 >= cprompt.length() ) )
      {
         output += cprompt[i];
         continue;
      }

      char cmd = cprompt[++i];
      if( cmd == '%')
      {
         output += '%';
         continue;
      }

      switch( cmd )
      {
         case 'h': output += std::to_string( ch->hit ); break;
         case 'H': output += std::to_string( ch->max_hit ); break;
         case 'm': output += std::to_string( ch->mana ); break;
         case 'M': output += std::to_string( ch->max_mana ); break;
         case 'v': output += std::to_string( ch->move ); break;
         case 'V': output += std::to_string( ch->max_move ); break;
         case 'g': output += std::to_string( ch->gold ); break;
         case 'u': output += std::to_string( num_descriptors ); break;
         case 'U': output += std::to_string( sysdata->maxplayers ); break;
         case 'x': output += std::to_string( ch->exp ); break;
         case 'X': output += std::to_string( exp_level( ch->level + 1 ) - ch->exp ); break;
         case 'w': output += std::to_string( ch->carry_weight ); break;
         case 'W': output += std::to_string( ch->can_carry_w() ); break;
         case 'a':
            if( ch->level >= 10 )
               output += std::to_string( ch->alignment );
            else
               output += ( ch->IS_GOOD() ? "good" : ( ch->IS_EVIL() ? "evil" : "neutral" ) );
            break;
         case 'A':
            if( ch->has_aflag( AFF_INVISIBLE ) )
               output += 'I';
            if( ch->has_aflag( AFF_HIDE ) )
               output += 'H';
            if( ch->has_aflag( AFF_SNEAK ) )
               output += 'S';
            break;
         case 'C': // Tank
         case 'c':
         {
            auto* v = ( ch->fighting && ch->fighting->who ) ? ( cmd == 'C' ? ch->fighting->who->fighting ? ch->fighting->who->fighting->who : nullptr : ch->fighting->who ) : nullptr;
            output += v ? get_health_string( v->hit, v->max_hit ) : "N/A";
            break;
         }
         case 'N': // Tank Name
         case 'n':
         {
            auto* v = ( ch->fighting && ch->fighting->who ) ? ( cmd == 'N' ? ch->fighting->who->fighting ? ch->fighting->who->fighting->who : nullptr : ch->fighting->who ) : nullptr;
            if( !v )
               output += "N/A";
            else
            {
               std::string name = ( ch == v ) ? "You" : ( v->isnpc() ? v->short_descr : v->name );

               if( !name.empty() )
                  name[0] = to_upper( name[0] );
               output += name;
            }
            break;
         }
         case 'T':
            if( time_info.hour < sysdata->hoursunrise || time_info.hour >= sysdata->hournightbegin )
               output += "night";
            else if( time_info.hour < sysdata->hourdaybegin )
               output += "dawn";
            else if( time_info.hour < sysdata->hoursunset )
               output += "day";
            else
               output += "dusk";
            break;
         case 'r':
            if( och->is_immortal() )
               output += std::to_string( ch->in_room->vnum );
            break;
         case 'F':
            if( och->is_immortal() )
               output += bitset_string( ch->in_room->flags, r_flags );
            break;
         case 'R':
            if( och->has_pcflag( PCFLAG_ROOMVNUM ) )
               output += std::format( "<#{}> ", ch->in_room->vnum );
            break;
         case 'o':
            if( auction && auction->item )
               output += auction->item->name;
            break; // display name of object on auction
         case 'S':
            if( ch->style == STYLE_BERSERK )
               output += 'B';
            else if( ch->style == STYLE_AGGRESSIVE )
               output += 'A';
            else if( ch->style == STYLE_DEFENSIVE )
               output += 'D';
            else if( ch->style == STYLE_EVASIVE )
               output += 'E';
            else
               output += 'S';
            break;
         case 'i':
            if( ch->has_pcflag( PCFLAG_WIZINVIS ) || ch->has_actflag( ACT_MOBINVIS ) )
               output += std::format( "(Invis {}) ", ( ch->isnpc() ? ch->mobinvis : ch->pcdata->wizinvis ) );
            else if( ch->has_aflag(AFF_INVISIBLE ) )
               output += "(Invis) ";
            break;
         case 'I':
            output += std::to_string( ch->has_actflag( ACT_MOBINVIS ) ? ch->mobinvis : ( ch->has_pcflag( PCFLAG_WIZINVIS ) ? ch->pcdata->wizinvis : 0 ) );
            break;
         case 'Z':
            if( sysdata->WIZLOCK )
               output += "[Wizlock]";
            if( sysdata->IMPLOCK )
               output += "[Implock]";
            if( sysdata->LOCKDOWN )
               output += "[Lockdown]";
            if( bootlock )
               output += "[Rebooting]";
            break;
         default:
            bug( "%s: bad command char '%c'.", __func__, cmd );
            break;
      }
   }

   ch->print( output );

   /*
    * The miracle cure for color bleeding? 
    */
   ch->print( ANSI_RESET );
}

void close_socket( descriptor_data * d, bool force )
{
   char_data *ch;
   auth_data *old_auth;

   if( d->ipid != -1 )
   {
      int status;

      kill( d->ipid, SIGKILL );
      waitpid( d->ipid, &status, 0 );
   }
   if( d->ifd != -1 )
      close( d->ifd );

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
   for( auto* s : dlist )
   {
      if( s->snoop_by == d )
         s->snoop_by = nullptr;
   }

   /*
    * Check for switched people who go link-dead. -- Altrag 
    */
   if( d->original )
   {
      if( ( ch = d->character ) != nullptr )
         interpret( ch, "return" );
      else
      {
         bug( "%s: original without character %s", __func__, ( d->original->name ? d->original->name : "unknown" ) );
         d->character = d->original;
         d->original = nullptr;
      }
   }

   ch = d->character;

   if( ch )
   {
      // Put this check here because seeing that they lost link after quitting/renting was annoying. We already know this.
      if( ch->in_room )
         log_printf_plus( LOG_COMM, ch->level, "Closing link to %s.", ch->pcdata->filename.c_str() );

      /*
       * Link dead auth -- Rantic 
       */
      old_auth = get_auth_name( ch->name );
      if( old_auth != nullptr && old_auth->state == AUTH_ONLINE )
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
            act( AT_ACTION, "$n has lost $s link.", ch, nullptr, nullptr, TO_CANSEE );
         ch->desc = nullptr;
      }
      else
      {
         /*
          * clear descriptor pointer to get rid of bug message in log 
          */
         ch->desc = nullptr;
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
   {
      if( !ch->in_room->flags.test( ROOM_WATCHTOWER ) )
         ch->music( "wilderness.mid", 100, false );
   }

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
short descriptor_data::check_reconnect( std::string_view name, bool fConn )
{
   for( auto* ch : pclist )
   {
      if( ( !fConn || !ch->desc ) && !ch->pcdata->filename.empty() && !str_cmp( name, ch->pcdata->filename ) )
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
               character->desc = nullptr;
               deleteptr( character );
            }
            return BERR;
         }

         if( fConn == false )
         {
            character->pcdata->pwd = ch->pcdata->pwd;
         }
         else
         {
            /*
             * clear descriptor pointer to get rid of bug message in log 
             */
            character->desc = nullptr;
            deleteptr( character );
            character = ch;
            ch->desc = this;
            ch->timer = 0;
            ch->print( "Reconnecting.\r\n\r\n" );
            update_connhistory( ch->desc, CONNTYPE_RECONN );

            /* Login trigger by Edmond */
            rprog_login_trigger( ch );
            mprog_login_trigger( ch );

            act( AT_ACTION, "$n has reconnected.", ch, nullptr, nullptr, TO_CANSEE );
            log_printf_plus( LOG_COMM, ch->level, "%s [%s] reconnected.", ch->name, hostname.c_str(  ) );
            connected = CON_PLAYING;
            check_auth_state( ch ); /* Link dead support -- Rantic */
            show_status( ch );
            check_loginmsg( ch );
         }
         return 1;
      }
   }
   return 0;
}

/*
 * Check if already playing.
 */
short descriptor_data::check_playing( std::string_view name, bool kick )
{
   for( auto* d : dlist )
   {
      if( d != this && ( d->character || d->original ) && !str_cmp( name, d->original ? d->original->pcdata->filename : d->character->pcdata->filename ) )
      {
         int cstate = d->connected;
         char_data *ch = d->original ? d->original : d->character;
         if( !ch->name
             || ( cstate != CON_PLAYING && cstate != CON_EDITING && cstate != CON_DELETE
                  && cstate != CON_ROLL_STATS && cstate != CON_PRIZENAME && cstate != CON_CONFIRMPRIZENAME
                  && cstate != CON_PRIZEKEY && cstate != CON_CONFIRMPRIZEKEY && cstate != CON_RAISE_STAT ) )
         {
            write_to_buffer( "Already connected - try again.\r\n" );
            log_printf_plus( LOG_COMM, ch->level, "%s already connected.", ch->pcdata->filename.c_str() );
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
         character->desc = nullptr;
         deleteptr( character );
         character = ch;
         ch->desc = this;
         ch->timer = 0;
         if( ch->switched )
            interpret( ch->switched, "return" );
         ch->switched = nullptr;
         ch->print( "Reconnecting.\r\n\r\n" );

         /* Login trigger by Edmond */
         rprog_login_trigger( ch );
         mprog_login_trigger( ch );

         act( AT_ACTION, "$n has reconnected, kicking off old link.", ch, nullptr, nullptr, TO_CANSEE );
         log_printf_plus( LOG_COMM, ch->level, "%s [%s] reconnected, kicking off old link.", ch->name, hostname.c_str(  ) );
         connected = cstate;
         check_auth_state( ch ); /* Link dead support -- Rantic */
         show_status( ch );
         check_loginmsg( ch );

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

   else if( !ch->is_immortal(  ) && ch->pcdata->release_date > std::chrono::system_clock::time_point{} && ch->pcdata->release_date > current_time )
   {
      if( !ch->to_room( get_room_index( ROOM_VNUM_HELL ) ) )
         log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __func__, __LINE__ );
   }

   else if( ch->in_room && ( ch->is_immortal(  ) || !ch->in_room->flags.test( ROOM_PROTOTYPE ) ) )
   {
      if( !ch->to_room( ch->in_room ) )
         log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __func__, __LINE__ );
   }

   else if( ch->is_immortal(  ) )
   {
      if( !ch->to_room( get_room_index( ROOM_VNUM_CHAT ) ) )
         log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __func__, __LINE__ );
   }

   else
   {
      if( !ch->to_room( get_room_index( ROOM_VNUM_TEMPLE ) ) )
         log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __func__, __LINE__ );
   }

   if( ch->get_timer( TIMER_SHOVEDRAG ) > 0 )
      ch->remove_timer( TIMER_SHOVEDRAG );

   if( ch->get_timer( TIMER_PKILLED ) > 0 )
      ch->remove_timer( TIMER_PKILLED );

   act( AT_ACTION, "$n has entered the game.", ch, nullptr, nullptr, TO_CANSEE );

   /* Login trigger by Edmond */
   rprog_login_trigger( ch );
   mprog_login_trigger( ch );

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

   ch->pcdata->lasthost = ch->desc->hostname;

   sale_count( ch ); /* New auction system - Samson 6-24-99 */

   check_auth_state( ch ); /* new auth */
   check_clan_info( ch );  /* see if this guy got a promo to clan admin */

   /*
    * @shrug, why not? :P 
    */
   if( ch->has_pcflag( PCFLAG_ONMAP ) )
   {
      if( !ch->in_room->flags.test( ROOM_WATCHTOWER ) )
         ch->music( "wilderness.mid", 100, false );
   }

   quotes( ch );

   if( ch->tempnum > 0 )
   {
      ch->print( "&YUpdating character for changes to experience system.\r\n" );
      ch->desc->show_stats( ch );
      ch->desc->connected = CON_RAISE_STAT;
   }

   check_loginmsg( ch );

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
   ch->desc->buffer_printf( "\r\nWelcome to {}...\r\n", sysdata->mud_name );
   char_to_game( ch );
}

CMDF( do_shatest )
{
   ch->print_fmt( "{}\r\n", argument );
   ch->print_fmt( "{}\r\n", sha256_crypt( argument ) );
}

/*
 * Deal with sockets that haven't logged in yet.
 * Function modified from original form on varying dates - Samson
 * Password encryption sections upgraded to use SHA-256 Encryption - Samson 7-10-00
 */
void descriptor_data::nanny( std::string & argument )
{
   char_data *ch;
   bool fOld;
   short chk;

   strip_lspace( argument );

   ch = character;

   switch ( connected )
   {
      default:
         bug( "%s: bad d->connected %d.", __func__, connected );
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
            // Uncomment below if you want to know when an MSSP request occurs
            //log_printf( "IP: %s requested MSSP data!", this->ipaddress.c_str() );
            close_socket( this, false );
            return;
         }

         /*
          * Old players can keep their characters. -- Alty 
          */
         strlower( argument );   // Modification to force proper name display
         argument[0] = to_upper( argument[0] ); /* Samson 5-22-98 */
         if( !check_parse_name( argument, ( newstate != 0 ) ) )
         {
            write_to_buffer( "You have chosen a name which is unacceptable.\r\n" );
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

         log_printf_plus( LOG_COMM, LEVEL_KL, "Incoming connection: %s, port %d.", hostname.c_str(  ), client_port );

         fOld = load_char_obj( this, argument, true, false );
         if( !character )
         {
            log_printf( "Bad player file %s@%s.", argument.c_str(  ), hostname.c_str(  ) );
            buffer_printf( "Your playerfile is corrupt...Please notify {}\r\n", sysdata->admin_email );
            close_socket( this, false );
            return;
         }
         ch = character;

         // Ban notice is given in the ban.cpp file
         if( is_banned( this ) )
         {
            close_socket( this, false );
            return;
         }

         if( ch->has_pcflag( PCFLAG_DENY ) )
         {
            log_printf_plus( LOG_COMM, LEVEL_IMMORTAL, "Denying access to %s@%s.", argument.c_str(  ), hostname.c_str(  ) );
            if( newstate != 0 )
            {
               write_to_buffer( "That name is already taken.  Please choose another: " );
               connected = CON_GET_NAME;
               character->desc = nullptr;
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
         if( ch->is_immortal(  ) && sysdata->check_imm_host && !check_immortal_domain( ch, ipaddress ) )
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
               character->desc = nullptr;
               deleteptr( character ); /* Big Memory Leak before --Shaddai */
               return;
            }
            /*
             * Old player 
             */
            write_to_buffer( "\r\nEnter your password: " );
            write_to_buffer( std::string_view{ reinterpret_cast<const char*>( echo_off_str.data() ), echo_off_str.size() } );
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
               character->desc = nullptr;
               deleteptr( character ); /* Big Memory Leak before --Shaddai */
               return;
            }
            strlower( argument );   /* Added to force names to display properly */
            argument = capitalize( argument );  /* Samson 5-22-98 */
            STRFREE( ch->name );
            ch->name = STRALLOC( argument.c_str(  ) );
            buffer_printf( "Did I get that right, {} (Y/N)? ", argument );
            connected = CON_CONFIRM_NEW_NAME;
            return;
         }

      case CON_GET_OLD_PASSWORD:
      {
         write_to_buffer( "\r\n" );

         if( str_cmp( sha256_crypt( argument ), ch->pcdata->pwd ) )
         {
            write_to_buffer( "Wrong password, disconnecting.\r\n" );
            /*
             * clear descriptor pointer to get rid of bug message in log 
             */
            character->desc = nullptr;
            close_socket( this, false );
            return;
         }

         write_to_buffer( std::string_view{ reinterpret_cast<const char*>( echo_on_str.data() ), echo_on_str.size() } );

         if( check_playing( ch->pcdata->filename, true ) )
            return;

         chk = check_reconnect( ch->pcdata->filename, true );
         if( chk == BERR )
         {
            if( character && character->desc )
               character->desc = nullptr;
            close_socket( this, false );
            return;
         }
         if( chk == 1 )
            return;

         std::string filename = ch->pcdata->filename;
         character->desc = nullptr;
         deleteptr( character );
         fOld = load_char_obj( this, filename, false, false );
         if( !fOld )
         {
            log_printf( "Bad player file %s@%s.", argument.c_str(  ), hostname.c_str(  ) );
            buffer_printf( "Your playerfile is corrupt...Please notify {}\r\n", sysdata->admin_email );
            close_socket( this, false );
            return;
         }
         ch = character;

         if( ch->position > POS_SITTING && ch->position < POS_STANDING )
            ch->position = POS_STANDING;

         log_printf_plus( LOG_COMM, LEVEL_KL, "%s [%s] has connected.", ch->name, hostname.c_str(  ) );
         show_title(  );
         break;
      }

      case CON_CONFIRM_NEW_NAME:
         switch ( argument[0] )
         {
            case 'y':
            case 'Y':
               buffer_printf( "\r\nMake sure to use a password that won't be easily guessed by someone else." "\r\nPick a good password for {}: {}", ch->name, std::string_view{ reinterpret_cast<const char*>( echo_off_str.data() ), echo_off_str.size() } );
               connected = CON_GET_NEW_PASSWORD;
               break;

            case 'n':
            case 'N':
               write_to_buffer( "Ok, what IS it, then? " );
               /*
                * clear descriptor pointer to get rid of bug message in log 
                */
               character->desc = nullptr;
               deleteptr( character );
               connected = CON_GET_NAME;
               break;

            default:
               write_to_buffer( "Please type Yes or No. " );
               break;
         }
         break;

      case CON_GET_NEW_PASSWORD:
      {
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

         std::string pwdnew = sha256_crypt( argument ); /* SHA-256 Encryption */
         ch->pcdata->pwd = pwdnew;
         write_to_buffer( "\r\nPlease retype the password to confirm: " );
         connected = CON_CONFIRM_NEW_PASSWORD;
         break;
      }

      case CON_CONFIRM_NEW_PASSWORD:
         write_to_buffer( "\r\n" );

         if( str_cmp( sha256_crypt( argument ), ch->pcdata->pwd ) )
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
            write_to_buffer( std::string_view{ reinterpret_cast<const char*>( echo_off_str.data() ), echo_off_str.size() } );
            connected = CON_GET_PORT_PASSWORD;
            return;
         }
#endif

         write_to_buffer( std::string_view{ reinterpret_cast<const char*>( echo_on_str.data() ), echo_on_str.size() } );

         write_to_buffer( "\r\nPlease note: You will be able to pick race and class after entering the game." );
         write_to_buffer( "\r\nWhat is your sex? " );
         write_to_buffer( "\r\n(M)ale, (F)emale, (N)euter, or (H)ermaphrodite ?" );
         connected = CON_GET_NEW_SEX;
         break;

      case CON_GET_PORT_PASSWORD:
         write_to_buffer( "\r\n" );

         if( str_cmp( sha256_crypt( argument ), sysdata->password ) )
         {
            write_to_buffer( "Invalid access code.\r\n" );
            /*
             * clear descriptor pointer to get rid of bug message in log 
             */
            character->desc = nullptr;
            close_socket( this, false );
            return;
         }
         write_to_buffer( std::string_view{ reinterpret_cast<const char*>( echo_on_str.data() ), echo_on_str.size() } );
         write_to_buffer( "\r\nPlease note: You will be able to pick race and class after entering the game." );
         write_to_buffer( "\r\nWhat is your sex? " );
         write_to_buffer( "\r\n(M)ale, (F)emale, (N)euter, or (H)ermaphrodite ?" );
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

         if( str_cmp( sha256_crypt( argument ), ch->pcdata->pwd ) )
         {
            write_to_buffer( "Wrong password entered, deletion cancelled.\r\n" );
            write_to_buffer( std::string_view{ reinterpret_cast<const char*>( echo_on_str.data() ), echo_on_str.size() } );
            connected = CON_PLAYING;
            return;
         }
         else
         {
            room_index *donate = get_room_index( ROOM_VNUM_DONATION );

            write_to_buffer( "\r\nYou've deleted your character!!!\r\n" );
            log_printf( "Player: %s has deleted.", capitalize( ch->name ).c_str() );

            if( donate != nullptr && ch->level > 1 )  /* No more deleting to remove goodies from play */
            {
               std::list<obj_data *>::iterator iobj;

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
            bug( "%s: Prize object turned nullptr somehow!", __func__ );
            write_to_buffer( "A fatal internal error has occured. Seek immortal assistance.\r\n" );
            ch->from_room(  );
            if( !ch->to_room( get_room_index( ROOM_VNUM_REDEEM ) ) )
               log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __func__, __LINE__ );
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
            bug( "%s: Prize object turned nullptr somehow!", __func__ );
            write_to_buffer( "A fatal internal error has occured. Seek immortal assistance.\r\n" );
            ch->from_room(  );
            if( !ch->to_room( get_room_index( ROOM_VNUM_REDEEM ) ) )
               log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __func__, __LINE__ );
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
            bug( "%s: Prize object turned nullptr somehow!", __func__ );
            write_to_buffer( "A fatal internal error has occured. Seek immortal assistance.\r\n" );
            ch->from_room(  );
            if( !ch->to_room( get_room_index( ROOM_VNUM_REDEEM ) ) )
               log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __func__, __LINE__ );
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
            bug( "%s: Prize object turned nullptr somehow!", __func__ );
            write_to_buffer( "A fatal internal error has occured. Seek immortal assistance.\r\n" );
            ch->from_room(  );
            if( !ch->to_room( get_room_index( ROOM_VNUM_REDEEM ) ) )
               log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __func__, __LINE__ );
            connected = CON_PLAYING;
            break;
         }

         switch ( argument[0] )
         {
            case 'y':
            case 'Y':
               write_to_buffer( "\r\nPrize keywords confirmed.\r\n\r\n" );
               ch->pcdata->spare_ptr = nullptr;
               ch->from_room(  );
               if( !ch->to_room( get_room_index( ROOM_VNUM_ENDREDEEM ) ) )
                  log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __func__, __LINE__ );
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
            bug( "%s: CON_RAISE_STAT: ch loop counter is < 1 for %s", __func__, ch->name );
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
            bug( "%s: CON_RAISE_STAT: %s is unable to raise anything.", __func__, ch->name );
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
                  buffer_printf( "You cannot raise your strength beyond {}\r\n", ch->perm_str );
               else
               {
                  ch->tempnum -= 1;
                  ch->perm_str += 1;
                  buffer_printf( "You've raised your strength to {}!\r\n", ch->perm_str );
               }
               if( ch->tempnum < 1 )
                  connected = CON_PLAYING;
               else
                  show_stats( ch );
               break;

            case '2':
               if( ch->perm_int >= 18 + race_table[ch->race]->int_plus )
                  buffer_printf( "You cannot raise your intelligence beyond {}\r\n", ch->perm_int );
               else
               {
                  ch->tempnum -= 1;
                  ch->perm_int += 1;
                  buffer_printf( "You've raised your intelligence to {}!\r\n", ch->perm_int );
               }
               if( ch->tempnum < 1 )
                  connected = CON_PLAYING;
               else
                  show_stats( ch );
               break;

            case '3':
               if( ch->perm_wis >= 18 + race_table[ch->race]->wis_plus )
                  buffer_printf( "You cannot raise your wisdom beyond {}\r\n", ch->perm_wis );
               else
               {
                  ch->tempnum -= 1;
                  ch->perm_wis += 1;
                  buffer_printf( "You've raised your wisdom to {}!\r\n", ch->perm_wis );
               }
               if( ch->tempnum < 1 )
                  connected = CON_PLAYING;
               else
                  show_stats( ch );
               break;

            case '4':
               if( ch->perm_dex >= 18 + race_table[ch->race]->dex_plus )
                  buffer_printf( "You cannot raise your dexterity beyond {}\r\n", ch->perm_dex );
               else
               {
                  ch->tempnum -= 1;
                  ch->perm_dex += 1;
                  buffer_printf( "You've raised your dexterity to {}!\r\n", ch->perm_dex );
               }
               if( ch->tempnum < 1 )
                  connected = CON_PLAYING;
               else
                  show_stats( ch );
               break;

            case '5':
               if( ch->perm_con >= 18 + race_table[ch->race]->con_plus )
                  buffer_printf( "You cannot raise your constitution beyond {}\r\n", ch->perm_con );
               else
               {
                  ch->tempnum -= 1;
                  ch->perm_con += 1;
                  buffer_printf( "You've raised your constitution to {}!\r\n", ch->perm_con );
               }
               if( ch->tempnum < 1 )
                  connected = CON_PLAYING;
               else
                  show_stats( ch );
               break;

            case '6':
               if( ch->perm_cha >= 18 + race_table[ch->race]->cha_plus )
                  buffer_printf( "You cannot raise your charisma beyond {}\r\n", ch->perm_cha );
               else
               {
                  ch->tempnum -= 1;
                  ch->perm_cha += 1;
                  buffer_printf( "You've raised your charisma to {}!\r\n", ch->perm_cha );
               }
               if( ch->tempnum < 1 )
                  connected = CON_PLAYING;
               else
                  show_stats( ch );
               break;

            case '7':
               if( ch->perm_lck >= 18 + race_table[ch->race]->lck_plus )
                  buffer_printf( "You cannot raise your luck beyond {}\r\n", ch->perm_lck );
               else
               {
                  ch->tempnum -= 1;
                  ch->perm_lck += 1;
                  buffer_printf( "You've raised your luck to {}!\r\n", ch->perm_lck );
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
               buffer_printf( "\r\nStr: {}\r\n", attribtext( ch->perm_str ) );
               buffer_printf( "Int: {}\r\n", attribtext( ch->perm_int ) );
               buffer_printf( "Wis: {}\r\n", attribtext( ch->perm_wis ) );
               buffer_printf( "Dex: {}\r\n", attribtext( ch->perm_dex ) );
               buffer_printf( "Con: {}\r\n", attribtext( ch->perm_con ) );
               buffer_printf( "Cha: {}\r\n", attribtext( ch->perm_cha ) );
               buffer_printf( "Lck: {}\r\n", attribtext( ch->perm_lck ) );
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
         buffer_printf( "\r\nWelcome to {}...\r\n", sysdata->mud_name );
         char_to_game( ch );
      }
         break;
   }  /* End Nanny switch */
}

CMDF( do_cache )
{
   ch->pager( "&YCached DNS Information\r\n" );
   ch->pager( "IP               | Address\r\n" );
   ch->pager( "------------------------------------------------------------------------------\r\n" );
   for( auto* dns : dnslist )
      ch->pagerf( "&w%16.16s  &g%s\r\n", dns->ip.c_str(  ), dns->name.c_str(  ) );

   ch->pagerf( "\r\n&W%zu IPs in the cache.\r\n", dnslist.size(  ) );
}

/* Send login messages to characters - big modification to the original */
/* login message stuff from the housing module - Edmond - June 02       */
CMDF( do_message )
{
   std::string name, arg1, arg2;
   short type = 0;

   if( argument.empty() )
   {
      ch->print( "Leave a login message for who?\r\n" );
      return;
   }

   argument = one_argument( argument, name );
   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );

   if( !str_cmp( name, "list" ) && ch->get_trust( ) >= LEVEL_GREATER )
   {
      for( auto* lmsg : login_messages )
      {
         ch->print_fmt( "&CName: &c{:<20} &CType: &c{}\r\n", capitalize( lmsg->name ), lmsg->type );

         if( lmsg->text )
            ch->print_fmt( "&CText:\r\n  &c{}\r\n", lmsg->text );

         ch->print( "\r\n" );
      }
      ch->print( "Ok.\r\n" );
      return;
   }

   std::filesystem::path checkname = std::format( "{}{}/{}", PLAYER_DIR, static_cast<char>( std::tolower( name.front() ) ), capitalize( name ) );

   if( exists_player( name ) )
   {
      for( auto* temp : pclist )
      {
         if( !str_cmp( name, temp->name ) && temp->desc )
         {
            ch->print( "They are online, wouldn't tells be just as easy?\r\n" );
            return;
         }
      }

      if( !str_cmp( arg1, "type" ) )
      {
         if( is_number( arg2 ) )
         {
            type = atoi( arg2.c_str() );

            if( type > MAX_MSG )
            {
               ch->print( "Invalid login message.\r\n" );
               return;
            }
            argument.clear();
         }
         else
         {
            ch->print( "Which type?\r\n" );
            return;
         }
      }

      if( !type && argument.empty() )
      {
         ch->print( "Send them what message?\r\n" );
         return;
      }

      add_loginmsg( name.c_str(), type, argument.c_str() );
      ch->print_fmt( "You have sent {} the following message:\r\n", capitalize( name ) );

      if( type == 0 )
         ch->print( argument );
      else
         ch->print( login_msg[type] );
      ch->print( "\r\n" );
   }
   else
   {
      ch->print( "That player does not exist.\r\n" );
      return;
   }
}
