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
 *                           Descriptor Class Info                          *
 ****************************************************************************/

#include <zlib.h>

inline constexpr std::string_view DNS_FILE = "../system/dns.dat";

constexpr int MAX_INBUF_SIZE = 4096;

constexpr int TELOPT_COMPRESS2 = 86;
constexpr int COMPRESS_BUF_SIZE = 8192; // Can no longer rely on being equal to MSL since that is being phased out.

extern const std::array<unsigned char, 4> echo_off_str;
extern const std::array<unsigned char, 4> will_compress2_str;
extern const std::array<unsigned char, 6> start_compress2_str;
extern const std::array<unsigned char, 4> will_msp_str;

struct mccp_data
{
   mccp_data(  );
   /*
    * No destructor needed 
    */

   z_stream *out_compress = nullptr;
   unsigned char *out_compress_buf = nullptr;
};

constexpr int TELOPT_MSP = 90; /* Mud Sound Protocol */
constexpr int MSP_DEFAULT = -99;

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
   bool write( std::string_view );
   bool read(  );
   bool flush_buffer( bool );
   void read_from_buffer(  );
   void write_to_buffer( std::string_view );

   template<typename... Args>
   void buffer_printf( std::format_string<Args...> fmt, Args&&... args )
   {
      this->write_to_buffer( std::format( fmt, std::forward<Args>(args)...) );
   }

   void send_color( std::string_view );
   void pager( std::string_view );
   void show_stats( char_data * );
   void send_greeting(  );
   void show_title(  );
   void process_dns(  );
   void resolve_dns( const std::string & );
   void prompt(  );
   void set_pager_input( std::string_view );
   bool pager_output(  );
   short check_reconnect( std::string_view, bool );
   short check_playing( std::string_view, bool );
   void nanny( std::string & );
   bool is_idle_timeout( );

   /*
    * Functions located in other files 
    */
   bool process_compressed(  );
   bool compressStart(  );
   bool compressEnd(  );
   // bool check_total_bans(  );
   void send_msp_startup(  );

   std::string ipaddress;
   std::string hostname;
   std::string outbuf;
   std::string pagebuf;
   std::string incomm;
   std::string inlast;
   std::string client;                    // Client detection
   std::vector<char> inbuf;
   descriptor_data *snoop_by = nullptr;
   char_data *character = nullptr;
   char_data *original = nullptr;
   olc_data *olc = nullptr;               // Tagith - Oasis OLC
   struct mccp_data *mccp = nullptr;      // Mud Client Compression Protocol
   int pageindex = 0;                         // Location of index value for pager vector<>
   int client_port = 0;
   int descriptor = 0;
   int newstate = 0;
   int repeat = 0;
   int ifd = 0;
   pid_t ipid = 0;
   pid_t process = 0; /* Samson 4-16-98 - For new command shell code */
   short connected = 0;
   short idle = 0;
   char pagecmd = '\0';
   char pagecolor = '\0';
   unsigned char prevcolor ='\0';
   bool fcommand = false;
   bool msp_detected = false;
   bool can_compress = false;
   bool is_compressing = false;
   bool disconnect = false;
};

extern std::list<descriptor_data*> dlist;
void free_all_descs(  );
bool load_char_obj( descriptor_data *, std::string_view, bool, bool );
void close_socket( descriptor_data *, bool );
