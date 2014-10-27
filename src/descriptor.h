/****************************************************************************
 *                   ^     +----- |  / ^     ^ |     | +-\                  *
 *                  / \    |      | /  |\   /| |     | |  \                 *
 *                 /   \   +---   |<   | \ / | |     | |  |                 *
 *                /-----\  |      | \  |  v  | |     | |  /                 *
 *               /       \ |      |  \ |     | +-----+ +-/                  *
 ****************************************************************************
 * AFKMud Copyright 1997-2008 by Roger Libiez (Samson),                     *
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
 *                           Descriptor Class Info                          *
 ****************************************************************************/

#ifndef __DESCRIPTOR_H__
#define __DESCRIPTOR_H__

#include <zlib.h>

const int TELOPT_COMPRESS2 = 86;
const int COMPRESS_BUF_SIZE = MSL;

extern char will_compress2_str[];
extern char start_compress2_str[];

struct mccp_data
{
   mccp_data();
   /* No destructor needed */

   z_stream *out_compress;
   unsigned char *out_compress_buf;
};

const int TELOPT_MSP = 90; /* Mud Sound Protocol */
const int MSP_DEFAULT = -99;

/* Mud eXtension Protocol 01/06/2001 Garil@DOTDII aka Jesse DeFer dotd@dotd.com */

#define MXP_VERSION "0.3"

const int TELOPT_MXP = 91;

enum mxp_objmenus
{
   MXP_NONE = -1, MXP_GROUND = 0, MXP_INV, MXP_EQ, MXP_SHOP, MXP_STEAL, MXP_CONT, MAX_MXPOBJ
};

extern char *mxp_obj_cmd[MAX_ITEM_TYPE][MAX_MXPOBJ];

#define MXP_TAG_OPEN	"\033[0z"
#define MXP_TAG_SECURE	"\033[1z"
#define MXP_TAG_LOCKED	"\033[2z"

/* ALT-155 is used as the MXP BEGIN tag: <
   ALT-156 is used as the MXP END tag: >
   ALT-157 is used for entity tags: &
*/
#define MXP_TAG_ROOMEXIT              MXP_TAG_SECURE"¢RExits£"
#define MXP_TAG_ROOMEXIT_CLOSE        "¢/RExits£"MXP_TAG_LOCKED

#define MXP_TAG_ROOMNAME              MXP_TAG_SECURE"¢RName£"
#define MXP_TAG_ROOMNAME_CLOSE        "¢/RName£"MXP_TAG_LOCKED

#define MXP_TAG_ROOMDESC              MXP_TAG_SECURE"¢RDesc£"
#define MXP_TAG_ROOMDESC_CLOSE        MXP_TAG_SECURE"¢/RDesc£"MXP_TAG_LOCKED

#define MXP_TAG_PROMPT                MXP_TAG_SECURE"¢Prompt£"
#define MXP_TAG_PROMPT_CLOSE          "¢/Prompt£"MXP_TAG_LOCKED
#define MXP_TAG_HP                    "¢Hp£"
#define MXP_TAG_HP_CLOSE              "¢/Hp£"
#define MXP_TAG_MAXHP                 "¢MaxHp£"
#define MXP_TAG_MAXHP_CLOSE           "¢/MaxHp£"
#define MXP_TAG_MANA                  "¢Mana£"
#define MXP_TAG_MANA_CLOSE            "¢/Mana£"
#define MXP_TAG_MAXMANA               "¢MaxMana£"
#define MXP_TAG_MAXMANA_CLOSE         "¢/MaxMana£"

#define MXP_SS_FILE     "../system/mxp.style"

class descriptor_data
{
 private:
   descriptor_data( const descriptor_data& d );
   descriptor_data& operator=( const descriptor_data& );

 public:
   descriptor_data(  );
   ~descriptor_data(  );

   /*
    * Internal to descriptor.cpp 
    */
   void init(  );
   bool write( char *, size_t );
   bool read(  );
   bool flush_buffer( bool );
   void read_from_buffer(  );
   void write_to_buffer( const char *, size_t );
   void buffer_printf( const char *, ... );
   void send_color( const char * );
   void pager( const char *, size_t );
   void show_stats( char_data * );
   void send_greeting(  );
   void show_title(  );
   void process_dns(  );
   void resolve_dns( long );
   void prompt(  );
   void set_pager_input( char * );
   bool pager_output(  );
   short check_reconnect( char *, bool );
   short check_playing( char *, bool );
   void nanny( char * );

   /*
    * Functions located in other files 
    */
   bool process_compressed(  );
   bool compressStart(  );
   bool compressEnd(  );
   bool check_total_bans(  );
   void send_msp_startup(  );
   void send_mxp_stylesheet(  );

   descriptor_data *snoop_by;
   char_data *character;
   char_data *original;
   olc_data *olc; /* Tagith - Oasis OLC */
   struct mccp_data *mccp; /* Mud Client Compression Protocol */
   char *host;
   char *outbuf;
   char *pagebuf;
   char *pagepoint;
   char *client;  /* Client detection */
   char inbuf[MAX_INBUF_SIZE];
   char incomm[MIL];
   char inlast[MIL];
   unsigned long outsize;
   unsigned long pagesize;
   int client_port;
   int descriptor;
   int newstate;
   int repeat;
   int outtop;
   int pagetop;
#if !defined(WIN32)
   int ifd;
   pid_t ipid;
#endif
   pid_t process; /* Samson 4-16-98 - For new command shell code */
   short connected;
   short idle;
   char pagecmd;
   char pagecolor;
   unsigned char prevcolor;
   bool fcommand;
   bool msp_detected;
   bool mxp_detected;
   bool can_compress;
   bool is_compressing;
};

extern list<descriptor_data*> dlist;
void free_all_descs( );
bool load_char_obj( descriptor_data *, char *, bool, bool );
void close_socket( descriptor_data *, bool );
void convert_mxp_tags( char *, const char * );
#endif
