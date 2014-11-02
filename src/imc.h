/****************************************************************************
 *                   ^     +----- |  / ^     ^ |     | +-\                  *
 *                  / \    |      | /  |\   /| |     | |  \                 *
 *                 /   \   +---   |<   | \ / | |     | |  |                 *
 *                /-----\  |      | \  |  v  | |     | |  /                 *
 *               /       \ |      |  \ |     | +-----+ +-/                  *
 ****************************************************************************
 * AFKMud Copyright 1997-2015 by Roger Libiez (Samson),                     *
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
 *                           IMC2 Freedom Client                            *
 ****************************************************************************/

/* IMC2 Freedom Client - Developed by Mud Domain.
 *
 * Copyright ©2004 by Roger Libiez ( Samson )
 * Contributions by Johnathan Walker ( Xorith ), Copyright ©2004
 * Additional contributions by Jesse Defer ( Garil ), Copyright ©2004
 * Additional contributions by Rogel, Copyright ©2004
 * Comments and suggestions welcome: http://www.mudbytes.net/imc2-support-forum
 * License terms are available in the imc2freedom.license file.
 */

#ifndef __IMC2_H__
#define __IMC2_H__

#include <sstream>

/* The all important version ID string, which is hardcoded for now out of laziness.
 * This name was chosen to represent the ideals of not only the code, but of the
 * network which spawned it.
 */
#define IMC_VERSION_STRING "IMC2 Freedom C++ CL-2.1a "
#define IMC_VERSION 2

/* Number of entries to keep in the channel histories */
const int MAX_IMCHISTORY = 20;
const int MAX_IMCTELLHISTORY = 20;

/* Remcon: Ask and ye shall receive. */
#define IMC_DIR          "../imc/"

#define IMC_CHANNEL_FILE IMC_DIR "imc.channels"
#define IMC_CONFIG_FILE  IMC_DIR "imc.config"
#define IMC_BAN_FILE     IMC_DIR "imc.ignores"
#define IMC_UCACHE_FILE  IMC_DIR "imc.ucache"
#define IMC_COLOR_FILE   IMC_DIR "imc.color"
#define IMC_HELP_FILE    IMC_DIR "imc.help"
#define IMC_CMD_FILE     IMC_DIR "imc.commands"
#define IMC_HOTBOOT_FILE IMC_DIR "imc.hotboot"
#define IMC_WHO_FILE     IMC_DIR "imc.who"
#define IMC_WEBLIST      IMC_DIR "imc.weblist"

/* Make sure you set the macros in the imccfg.h file properly or things get ugly from here. */
#include "imccfg.h"

const unsigned int IMC_BUFF_SIZE = 16384;

/* Connection states stuff */
enum imc_constates
{
   IMC_OFFLINE, IMC_AUTH1, IMC_AUTH2, IMC_ONLINE
};

enum imc_permissions
{
   IMCPERM_NOTSET, IMCPERM_NONE, IMCPERM_MORT, IMCPERM_IMM, IMCPERM_ADMIN, IMCPERM_IMP
};

/* Player flags */
enum imc_flags
{
   IMC_TELL, IMC_DENYTELL, IMC_BEEP, IMC_DENYBEEP, IMC_INVIS, IMC_PRIVACY, IMC_DENYFINGER,
   IMC_AFK, IMC_COLORFLAG, IMC_PERMOVERRIDE, IMC_NOTIFY, IMC_MAXFLAG
};

/* Flag macros */
#define IMCIS_SET(var, bit)         (var).test((bit))
#define IMCSET_BIT(var, bit)        (var).set((bit))
#define IMCREMOVE_BIT(var, bit)     (var).reset((bit))

/* Channel flags, only one so far, but you never know when more might be useful */
enum imc_channel_flags
{
   IMCCHAN_LOG, IMC_MAXCHANFLAG
};

#define IMCPERM(ch)           (CH_IMCDATA((ch))->imcperm)
#define IMCFLAG(ch)           (CH_IMCDATA((ch))->imcflag)
#define IMC_LISTEN(ch)        (CH_IMCDATA((ch))->imc_listen)
#define IMC_DENY(ch)          (CH_IMCDATA((ch))->imc_denied)
#define IMC_RREPLY(ch)        (CH_IMCDATA((ch))->rreply)
#define IMC_RREPLY_NAME(ch)   (CH_IMCDATA((ch))->rreply_name)
#define IMC_EMAIL(ch)         (CH_IMCDATA((ch))->email)
#define IMC_HOMEPAGE(ch)      (CH_IMCDATA((ch))->homepage)
#define IMC_AIM(ch)           (CH_IMCDATA((ch))->aim)
#define IMC_ICQ(ch)           (CH_IMCDATA((ch))->icq)
#define IMC_YAHOO(ch)         (CH_IMCDATA((ch))->yahoo)
#define IMC_MSN(ch)           (CH_IMCDATA((ch))->msn)
#define IMC_COMMENT(ch)       (CH_IMCDATA((ch))->comment)
#define IMCTELLHISTORY(ch,x)  (CH_IMCDATA((ch))->imc_tellhistory[(x)])
#define IMCISINVIS(ch)        ( IMCIS_SET( IMCFLAG((ch)), IMC_INVIS ) )
#define IMCAFK(ch)            ( IMCIS_SET( IMCFLAG((ch)), IMC_AFK ) )

class imc_packet;

typedef void IMC_FUN( char_data * ch, string argument );
#define IMC_CMD( name ) extern "C" void (name)( char_data *ch, string argument )

typedef void PACKET_FUN( imc_packet * q, string packet );
#define PFUN( name ) extern "C" void (name)( imc_packet *q, string packet )

/* Oh yeah, baby, that raunchy looking Merc structure just got the facelift of the century.
 * Thanks to Thoric and friends for the slick idea.
 */
struct imc_command_table
{
   list < string > aliaslist;
   IMC_FUN *function;
   string funcname;
   string name;
   int level;
   bool connected;
};

struct imc_help_table
{
   string hname;
   string text;
   int level;
};

struct imc_ucache_data
{
   string name;
   time_t time;
   int gender;
};

struct imc_chardata
{
   list < string > imc_ignore;
   bitset < IMC_MAXFLAG > imcflag;  /* Flags set on the player */
   string email;  /* Person's email address - for imcfinger - Samson 3-21-04 */
   string homepage;  /* Person's homepage - Samson 3-21-04 */
   string aim; /* Person's AOL Instant Messenger screenname - Samson 3-21-04 */
   string yahoo;  /* Person's Y! screenname - Samson 3-21-04 */
   string msn; /* Person's MSN Messenger screenname - Samson 3-21-04 */
   string comment;   /* Person's personal comment - Samson 3-21-04 */
   string imc_listen;   /* Channels the player is listening to */
   string imc_denied;   /* Channels the player has been denied use of */
   string rreply; /* IMC reply-to */
   string rreply_name;  /* IMC reply-to shown to char */
   string imc_tellhistory[MAX_IMCTELLHISTORY];  /* History of received imctells - Samson 1-21-04 */
   int icq; /* Person's ICQ UIN Number - Samson 3-21-04 */
   int imcperm;   /* Permission level for the player */
};

struct imc_channel
{
   string chname; /* name of channel */
   string owner;  /* owner (singular) of channel */
   string operators; /* current operators of channel */
   string invited;
   string excluded;
   string local_name;   /* Operational localname */
   string regformat;
   string emoteformat;
   string socformat;
   string history[MAX_IMCHISTORY];
     bitset < IMC_MAXCHANFLAG > flags;
   short level;
   bool open;
   bool refreshed;
};

class imc_packet
{
 public:
   imc_packet(  );
   imc_packet( const string &, const string &, const string & );
   void send(  );

   ostringstream data;
   string from;
   string to;
   string type;
   string route;  /* This is only used internally and not sent */
};

/* The mud's connection data for the server */
struct imc_siteinfo
{
   imc_siteinfo(  );

   string servername;   // The name of the IMC server this MUD connects to. Set during startup.
   string rhost;  // DNS/IP of the IMC server this MUD will connect to
   string network;   // Network name of the server, set at keepalive - Samson
   string serverpw;  // Server password
   string clientpw;  // Client password
   string localname; // One word localname used on the network
   string fullname;  // FULL name of the MUD
   string ihost;  // DNS/IP address for the MUD
   string email;  // Contact email address for the MUD's administrator
   string www; // This MUD's homepage
   string details;   // BRIEF description of the MUD
   string versionid; // Transient version id for the imclist
   int iport;  // The port number used to log on to the MUD
   int minlevel;  // Minimum player level allowed to access IMC
   int immlevel;  // First level at which users become Immortals
   int adminlevel;   // Admin level: Level at which users become higher administrators
   int implevel;  // Implementor level. Duh. The big cheese. The top banana. God king of all mankind, etc.
   unsigned short rport;   // Remote port number of the server this MUD connects to.
   bool sha256;   // Client will support SHA256 authentication
   bool sha256pass;  // Client is using SHA256 authentication
   bool autoconnect; // Do we autoconnect on bootup or not? - Samson

   /*
    * Conection parameters - These don't save in the config file 
    */
   char inbuf[IMC_BUFF_SIZE]; // The input buffer for the IMC connection
   string incomm; // Um....
   char *outbuf;  // The output buffer for the IMC connection
   unsigned long outsize;  // The size of the current output buffer
   size_t outtop; // Uh....
   int desc;   // Descriptor number assigned to the IMC socket
   unsigned short state;   // The state of the connection to the IMC server
};

struct imc_remoteinfo
{
   string rname;
   string version;
   string network;
   string path;
   string url;
   string host;
   string port;
   bool expired;
};

struct who_template
{
   string head;
   string plrheader;
   string immheader;
   string plrline;
   string immline;
   string tail;
   string master;
};

extern imc_siteinfo *this_imcmud;

bool imc_command_hook( char_data *, string &, string & );
void imc_hotboot(  );
void imc_startup( bool, int, bool );
void imc_shutdown( bool );
void imc_initchar( char_data * );
void imc_loadchar( char_data *, FILE *, const char * );
void imc_savechar( char_data *, FILE * );
void imc_freechardata( char_data * );
void imc_loop(  );
imc_channel *imc_findchannel( const string & ); /* Externalized for comm.c spamguard checks */
void imc_register_packet_handler( const string &, PACKET_FUN * );
void ev_imcweb_refresh( void * );
string escape_string( const string & );
map < string, string > imc_getData( string & );
char_data *imc_find_user( const string & );
string imc_nameof( string );
string imc_mudof( string );
void imc_send_tell( string, string, const string &, int );
void imc_to_char( string, char_data * );
void imc_to_pager( string, char_data * );
void imc_printf( char_data *, const char *, ... ) __attribute__ ( ( format( printf, 2, 3 ) ) );
char_data *imc_make_skeleton( const string & );
#endif
