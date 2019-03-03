/****************************************************************************
 *                   ^     +----- |  / ^     ^ |     | +-\                  *
 *                  / \    |      | /  |\   /| |     | |  \                 *
 *                 /   \   +---   |<   | \ / | |     | |  |                 *
 *                /-----\  |      | \  |  v  | |     | |  /                 *
 *               /       \ |      |  \ |     | +-----+ +-/                  *
 ****************************************************************************
 * AFKMud Copyright 1997-2019 by Roger Libiez (Samson),                     *
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
 *                           Descriptor Class Info                          *
 ****************************************************************************/

#ifndef __DESCRIPTOR_H__
#define __DESCRIPTOR_H__

#include <zlib.h>

const int MAX_INBUF_SIZE = 4096;

const int TELOPT_COMPRESS2 = 86;
const int COMPRESS_BUF_SIZE = MSL;

extern const unsigned char will_compress2_str[];
extern const unsigned char start_compress2_str[];
extern const unsigned char will_msp_str[];

struct mccp_data
{
   mccp_data(  );
   /*
    * No destructor needed 
    */

   z_stream *out_compress;
   unsigned char *out_compress_buf;
};

const int TELOPT_MSP = 90; /* Mud Sound Protocol */
const int MSP_DEFAULT = -99;

class descriptor_data
{
 private:
   descriptor_data( const descriptor_data & d );
     descriptor_data & operator=( const descriptor_data & );

 public:
     descriptor_data(  );
    ~descriptor_data(  );

   /*
    * Internal to descriptor.cpp 
    */
   void init(  );
   bool write( const char * );
   bool read(  );
   bool flush_buffer( bool );
   void read_from_buffer(  );
   void write_to_buffer( const string & );
   void buffer_printf( const char *, ... ) __attribute__ ( ( format( printf, 2, 3 ) ) );
   void send_color( const string & );
   void pager( const string & );
   void show_stats( char_data * );
   void send_greeting(  );
   void show_title(  );
   void process_dns(  );
   void resolve_dns( long );
   void prompt(  );
   void set_pager_input( const string & );
   bool pager_output(  );
   short check_reconnect( const string &, bool );
   short check_playing( const string &, bool );
   void nanny( string & );

   /*
    * Functions located in other files 
    */
   bool process_compressed(  );
   bool compressStart(  );
   bool compressEnd(  );
   // bool check_total_bans(  );
   void send_msp_startup(  );

   string ipaddress;
   string hostname;
   string outbuf;
   string pagebuf;
   string incomm;
   string inlast;
   string client; /* Client detection */
   descriptor_data *snoop_by;
   char_data *character;
   char_data *original;
   olc_data *olc; /* Tagith - Oasis OLC */
   struct mccp_data *mccp; /* Mud Client Compression Protocol */
   char inbuf[MAX_INBUF_SIZE];
   int pageindex; // Location of index value for pager vector<>
   int client_port;
   int descriptor;
   int newstate;
   int repeat;
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
   bool can_compress;
   bool is_compressing;
   bool disconnect;
};

extern list < descriptor_data * >dlist;
void free_all_descs(  );
bool load_char_obj( descriptor_data *, const string &, bool, bool );
void close_socket( descriptor_data *, bool );
#endif
