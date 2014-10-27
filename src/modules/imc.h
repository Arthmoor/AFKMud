/****************************************************************************
 *                   ^     +----- |  / ^     ^ |     | +-\                  *
 *                  / \    |      | /  |\   /| |     | |  \                 *
 *                 /   \   +---   |<   | \ / | |     | |  |                 *
 *                /-----\  |      | \  |  v  | |     | |  /                 *
 *               /       \ |      |  \ |     | +-----+ +-/                  *
 ****************************************************************************
 * AFKMud Copyright 1997-2012 by Roger Libiez (Samson),                     *
 * Levi Beckerson (Whir), Michael Ward (Tarl), Erik Wolfe (Dwip),           *
 * Cameron Carroll (Cam), Cyberfox, Karangi, Rathian, Raine,                *
 * Xorith, and Adjani.                                                      *
 * All Rights Reserved.                                                     *
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
 *                           IMC2 Freedom Client                            *
 ****************************************************************************/

/* IMC2 Freedom Client - Developed by Mud Domain.
 *
 * Copyright (C)2004 by Roger Libiez ( Samson )
 * Contributions by Johnathan Walker ( Xorith ), Copyright (C)2004
 * Additional contributions by Jesse Defer ( Garil ), Copyright (C)2004
 * Additional contributions by Rogel, Copyright (c) 2004
 * Comments and suggestions welcome: imc@intermud.us
 * License terms are available in the imc2freedom.license file.
 */

#ifndef __IMC2_H__
#define __IMC2_H__

#include <sstream>

/* The all important version ID string, which is hardcoded for now out of laziness.
 * This name was chosen to represent the ideals of not only the code, but of the
 * network which spawned it.
 */
#define IMC_VERSION_STRING "IMC2 Freedom C++ CL-2.1 "
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
   imc_packet( string from, string type, string to );
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
   string servername;   /* name of server */
   string rhost;  /* DNS/IP of server */
   string network;   /* Network name of the server, set at keepalive - Samson */
   string serverpw;  /* server password */
   string clientpw;  /* client password */
   string localname; /* One word localname */
   string fullname;  /* FULL name of mud */
   string ihost;  /* host address of mud */
   string email;  /* contact address (email) */
   string www; /* homepage */
   string details;   /* BRIEF description of mud */
   string versionid; /* Transient version id for the imclist */
   int iport;  /* The port the mud itself is on */
   int minlevel;  /* Minimum player level */
   int immlevel;  /* Immortal level */
   int adminlevel;   /* Admin level */
   int implevel;  /* Implementor level */
   unsigned short rport;   /* remote port of server */
   bool sha256;   /* Client will support SHA256 authentication */
   bool sha256pass;  /* Client is using SHA256 authentication */
   bool autoconnect; /* Do we autoconnect on bootup or not? - Samson */

   /*
    * Conection parameters - These don't save in the config file 
    */
   char inbuf[IMC_BUFF_SIZE]; /* input buffer */
   char incomm[IMC_BUFF_SIZE];
   char *outbuf;  /* output buffer */
   unsigned long outsize;
   int outtop;
   int desc;   /* descriptor */
   unsigned short state;   /* connection state */
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

bool imc_command_hook( char_data *, char *, char * );
void imc_hotboot( );
void imc_startup( bool, int, bool );
void imc_shutdown( bool );
void imc_initchar( char_data * );
void imc_loadchar( char_data *, FILE *, const char * );
void imc_savechar( char_data *, FILE * );
void imc_freechardata( char_data * );
void imc_loop( );
imc_channel *imc_findchannel( string ); /* Externalized for comm.c spamguard checks */
void imc_register_packet_handler( string, PACKET_FUN * );
void ev_imcweb_refresh( void * );

#endif
