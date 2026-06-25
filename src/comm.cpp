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
 *                      Network Communication Module                        *
 ****************************************************************************/

#include <dlfcn.h>
#include <netdb.h>
#include <sys/wait.h>
#include <filesystem>
#include <fstream>
#include <thread>
#include "mud.h"
#include "descriptor.h"
#include "clans.h"
#include "connhist.h"
#include "mud_prog.h"
#include "objindex.h"
#include "pfiles.h"
#include "raceclass.h"
#include "realms.h"
#include "roomindex.h"
#include "shops.h"
#include "web.h"

/*
 * Global variables.
 */
int num_descriptors = 0;                                 // Tracks the current number of people connected to the game.
int num_logins = 0;                                      // Tracks the number of people who have logged on since the MUD rebooted.
bool mud_down = false;                                   // Whether or not the MUD considered itself online and running. Changing to "true" causes a graceful shutdown.
std::chrono::system_clock::time_point current_time;      // Time of this game pulse. current_time is considered a MUD-wide authority on what time it is in the real world.
std::chrono::system_clock::time_point mud_start_time;    // Used only by MSSP for now. Records the MUD's start time.
std::string str_boot_time;                               // A string representation of the MUD's startup time.
std::string lastplayercmd = "No commands issued yet.";   // Global buffer to hold the last command a player used.
int control = -1;                                        // Controlling descriptor the MUD uses to accept new connections on.
fd_set in_set;                                           // Set of descriptors for reading on each pass through game_loop().
fd_set out_set;                                          // Set of descriptors for writing on each pass through game_loop().
fd_set exc_set;                                          // Set of descriptors with exceptions on each pass through game_loop(). These will be flagged for disconnection.
const char *alarm_section = "(unknown)";                 // Message sent when the "lag alarm" goes off to help identify where it happened.
bool winter_freeze = false;                              // Whether or not the MUD has entered the winter season and if flagged rooms should switch to their frozen states.
int mud_port = 7500;                                     // The default port the MUD listens on for new connections.
bool DONTSAVE = false;                                   // Flag to decide whether or not players get saved during reboots, shutdowns, etc. The only two places it gets changed are in do_reboot() and do_shutdown().
bool bootlock = false;                                   // Lock flag to prevent people from logging on while a reboot or shutdown count is running.
bool DONT_UPPER = false;                                 // Weird flag that prevents capitalization in act_string() and in several places in mud_comm.cpp().

extern int newdesc;
#ifdef MULTIPORT
extern bool compilelock;
#endif

void game_loop(  );
void cleanup_memory(  );
void clear_trdata(  );
void run_events( std::chrono::system_clock::time_point );

/*
 * External functions
 */
void boot_db( bool );
void accept_new( int );
void check_auth_state( char_data * );

#ifdef MULTIPORT
#if !defined(__CYGWIN__)
void mud_recv_message(  );
#endif
#endif
void save_ships(  );
void save_timedata(  );
void save_weathermap(  );
void save_morphs(  );
void hotboot_recover(  );
void update_connhistory( descriptor_data *, int ); /* connhist.cpp */
void free_connhistory( int ); /* connhist.cpp */

/* Used during memory cleanup */
void free_immhosts();
void free_mssp_info( void );
void free_morphs(  );
void free_quotes(  );
void free_envs(  );
void free_sales(  );
void free_bans(  );
void free_all_auths(  );
void free_runedata(  );
void free_slays(  );
void free_holidays(  );
void free_ships(  );
void free_liquiddata(  );
void free_mudchannels(  );
void free_commands(  );
void free_deities(  );
void free_socials(  );
void free_boards(  );
void free_teleports(  );
void close_all_areas(  );
void free_prog_actlists(  );
void free_questbits(  );
void free_projects(  );
void free_specfuns(  );
void clear_wizinfo(  );
void free_tongues(  );
void free_skills(  );
void free_all_events(  );
#ifdef MULTIPORT
void free_shellcommands(  );
#endif
void free_dns_list(  );
void extract_all_chars(  );
void extract_all_objs(  );
void free_all_classes(  );
void free_all_races(  );
void free_all_chess_games(  );
void free_continents(  );
void free_helps(  );
void free_wizlist_data( );
void free_wizlist_web_data( );
void clear_auction( );
void free_weapon_table( );
#if defined(SQL)
 void close_db(  );
#endif

inline constexpr std::array<std::string_view, 21> directory_table = {
   AREA_CONVERT_DIR, PLAYER_DIR, GOD_DIR, BACKUP_DIR, BUILD_DIR, SYSTEM_DIR,
   PROG_DIR, CORPSE_DIR, CLASS_DIR, RACE_DIR, MOTD_DIR, HOTBOOT_DIR, AUC_DIR,
   BOARD_DIR, COLOR_DIR, MAP_DIR, DEITY_DIR, WEB_DIR, SHOP_DIR, CLAN_DIR,
   REALM_DIR
};

void directory_check( void )
{
   std::filesystem::path buf;

   // Successful directory check will drop this file in the area dir once done.
   if( std::filesystem::exists( "DIR_CHECK_PASSED" ) )
      return;

   log_string( "Checking for required directories...\n" );

   // This should really never happen but you never know.
   if( !std::filesystem::exists( "../log" ) )
   {
      fprintf( stderr, "Creating required directory: ../log\n" );
      buf = "../log";

      if( !std::filesystem::create_directory( buf ) )
      {
         fprintf( stderr, "FATAL ERROR :: Unable to create required directory: ../log\n" );
         std::exit( EXIT_FAILURE );
      }
   }

   for( size_t x = 0; x < sizeof( directory_table ) / sizeof( directory_table[0] ); ++x )
   {
      if( !std::filesystem::exists( directory_table[x] ) )
      {
         buf = directory_table[x];
         log_printf( "Creating required directory: {}", directory_table[x] );

         if( !std::filesystem::create_directory( buf ) )
         {
            log_printf( "FATAL ERROR :: Unable to create required directory: {}. Must be corrected manually.", directory_table[x] );
            std::exit( EXIT_FAILURE );
         }
      }

      if( !str_cmp( directory_table[x], PLAYER_DIR ) )
      {
         for( char c = 'a'; c <= 'z'; ++c )
         {
            std::filesystem::path dirname = std::format( "{}{}", PLAYER_DIR, c );
            if( !std::filesystem::exists( dirname ) )
            {
               log_printf( "Creating required directory: {}", dirname.string() );

               if( !std::filesystem::create_directory( dirname ) )
               {
                  log_printf( "FATAL ERROR :: Unable to create required directory: {}. Must be corrected manually.", dirname.string() );
                  std::exit( EXIT_FAILURE );
               }
            }
         }
      }
   }

   if( chdir( "../area" ) == -1 )
   {
      fprintf( stderr, "FATAL ERROR :: Unable to change directories during directory check! Cannot continue." );
      std::exit( EXIT_FAILURE );
   }

   // Made it? Sweet. Drop the check file so we don't do this on every last reboot.
   std::ofstream outfile( "DIR_CHECK_PASSED" );
   if( !outfile )
   {
      fprintf( stderr, "FATAL ERROR :: Unable to generate DIR_CHECK_PASSED\n" );
      std::exit( EXIT_FAILURE );
   }
   outfile.close();

   log_string( "Directory check passed." );
}

void set_alarm( long seconds )
{
   alarm( seconds );
}

void open_mud_log()
{
   const std::filesystem::path log_dir = "../log";
   std::filesystem::path log_path;
   bool found = false;

   // Look for the current logfile that's been opened by the startup script.
   for( int logindex = 1000; logindex < 100000; ++logindex )
   {
      log_path = log_dir / ( std::to_string( logindex ) + ".log" );

      if( !std::filesystem::exists( log_path ) )
      {
         found = true;
         break;
      }
   }

   // Dude, if you have over 100000 logs you have problems!
   if( !found )
   {
      fprintf( stderr, "You have too damn many log files! Clean them up!\n" );
      std::exit( EXIT_FAILURE );
   }

   // Open log file for appending.
   FILE* error_log = fopen( log_path.c_str(), "a" );
   if( !error_log )
   {
      fprintf( stderr, "Unable to append to %s.\n", log_path.c_str() );
      std::exit( EXIT_FAILURE );
   }

   // Redirect stderr.
   dup2( fileno( error_log ), STDERR_FILENO );
   FCLOSE( error_log );
}

// Called in init_socket
struct SocketGuard
{
   int fd = -1;

   SocketGuard( int f ) : fd( f ) {}
   ~SocketGuard()
   {
      if( fd != -1 )
         close( fd );
   }

   // Release ownership so it doesn't close when returning
   int release()
   {
      int temp = fd;

      fd = -1;

      return temp;
   }
};

/*
 * This function supports connections for both IPv6 and IPv4.
 * On a server which only has one type of address, it will still bind to both.
 * Every major operating system these days supports both, even if they only have one type of address.
 */
int init_socket( const int mudport )
{
   std::string port_str = std::to_string( mudport );
   struct addrinfo hints{}, *res;

   hints.ai_family = AF_INET6;       // IPv6
   hints.ai_socktype = SOCK_STREAM;  // TCP
   hints.ai_flags = AI_PASSIVE;      // For bind()

   if( int err = getaddrinfo( nullptr, port_str.c_str(), &hints, &res ) )
   {
      throw std::runtime_error( gai_strerror( err ) );
   }

   SocketGuard guard( socket( res->ai_family, res->ai_socktype, res->ai_protocol ) );
   if( guard.fd < 0 )
      throw std::system_error( errno, std::generic_category(), "socket" );

   int opt = 1;

   // Set V6ONLY to 0 to allow dual-stack (IPv4 and IPv6)
   int v6only = 0;
   setsockopt( guard.fd, IPPROTO_IPV6, IPV6_V6ONLY, &v6only, sizeof( v6only ) );
   setsockopt( guard.fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt) );

   if( bind( guard.fd, res->ai_addr, res->ai_addrlen ) < 0 )
   {
      freeaddrinfo( res );
      throw std::system_error( errno, std::generic_category(), "bind" );
   }

   freeaddrinfo( res );

   if( listen( guard.fd, 50 ) < 0 )
   {
      throw std::system_error( errno, std::generic_category(), "listen" );
   }

   return guard.release();
}

/* This functions purpose is to open up all of the various things the mud needs to have
 * up before the game_loop is entered. If something needs to be added to the mud
 * startup procedures it should be placed in here.
 */
void init_mud( bool fCopyOver, int gameport )
{
   // Scan for and create necessary dirs if they don't exit.
   directory_check(  );

   /*
    * If this all goes well, we should be able to open a new log file during hotboot 
    */
   if( fCopyOver )
   {
      open_mud_log(  );
      log_string( "Hotboot: Spawning new log file." );
   }

   log_string( "Booting Database" );
   boot_db( fCopyOver );

   if( !fCopyOver )  /* We have already the port if copyover'ed */
   {
      log_string( "Initializing main socket..." );
      control = init_socket( gameport );
      log_string( "Main socket initialized." );
   }

   clear_trdata(  ); // Begin the transfer data tracking

#ifdef MULTIPORT
   switch ( gameport )
   {
      case MAINPORT:
         log_printf( "{} game server ready on port {}.", sysdata->mud_name, gameport );
         break;
      case BUILDPORT:
         log_printf( "{} builders' server ready on port {}.", sysdata->mud_name, gameport );
         break;
      case CODEPORT:
         log_printf( "{} coding server ready on port {}.", sysdata->mud_name, gameport );
         break;
      default:
         log_printf( "{} - running on unsupported port {}!!", sysdata->mud_name, gameport );
         break;
   }
#else
   log_printf( "{} ready on port {}.", sysdata->mud_name, gameport );
#endif

   if( fCopyOver )
   {
      log_string( "Initiating hotboot recovery." );
      hotboot_recover(  );
   }
}

/*
 * This function is called from 'main' or 'SigTerm'. Its purpose is to clean up
 * the various loose ends the mud will have running before it shuts down. Put anything
 * which needs to be added to the shutdown procedures in here.
 */
void close_mud( void )
{
   if( !DONTSAVE )
   {
      log_string( "Saving players...." );
      for( auto* d : dlist )
      {
         char_data *vch = ( d->character ? d->character : d->original );

         if( !vch )
            continue;

         if( !vch->isnpc(  ) )
         {
            vch->save(  );
            log_printf( "{} saved.", vch->name );
            d->write( "You have been saved to disk.\033[0m\r\n" );
         }
      }
   }

   // Save game world time - Samson 1-21-99 
   log_string( "Saving game world time...." );
   save_timedata(  );

   log_string( "Saving weather map data..." );
   save_weathermap(  );

   // Save ship information - Samson 1-8-01 
   save_ships(  );

   fflush( stderr ); /* make sure stderr is flushed */

   close( control );
}

static void SigUser1( int signum )
{
   log_string( "Received User1 signal from server." );
}

static void SigUser2( int signum )
{
   log_string( "Received User2 signal from server." );
}

#ifdef MULTIPORT
static void SigChld( int signum )
{
   int pid, status;

   while( 1 )
   {
      pid = waitpid( WAIT_ANY, &status, WNOHANG );
      if( pid < 0 )
         break;

      if( pid == 0 )
         break;

      for( auto* d : dlist )
      {
         if( d->connected == CON_FORKED && d->process == pid )
         {
            if( compilelock )
            {
               echo_to_all( "&GCompiler operation completed. Reboot and shutdown commands unlocked.", ECHOTAR_IMM );
               compilelock = false;
            }
            d->process = 0;
            d->connected = CON_PLAYING;
            char_data *ch = d->original ? d->original : d->character;
            if( ch )
               ch->printf( "Process exited with status code %d.\r\n", status );
         }
      }
   }
}
#endif

static void SigTerm( int signum )
{
   echo_to_all( "&RATTENTION!! Message from game server: &YEmergency shutdown called.\a", ECHOTAR_ALL );
   echo_to_all( "&YExecuting emergency shutdown procedure.", ECHOTAR_ALL );
   log_string( "Message from server: Executing emergency shutdown procedure." );
   shutdown_mud( "Emergency Shutdown" );

   for( auto* vch : pclist )
   {
      /*
       * One of two places this gets changed 
       */
      vch->pcdata->hotboot = true;
   }

   close_mud(  );
   log_string( "Emergency shutdown complete." );

   /*
    * Using exit here instead of mud_down because the thing sometimes failed to kill when asked!! 
    */
   std::exit( EXIT_SUCCESS );
}

/*
 * LAG alarm!							-Thoric
 */
static void caught_alarm( int signum )
{
   echo_to_all( "&[immortal]Alas, the hideous malevolent entity known only as 'Lag' rises once more!", ECHOTAR_ALL );
   bug( "&RALARM CLOCK! In section {}", alarm_section );

   if( newdesc )
   {
      FD_CLR( newdesc, &in_set );
      FD_CLR( newdesc, &out_set );
      FD_CLR( newdesc, &exc_set );
      log_string( "clearing newdesc" );
   }

   if( fBootDb )
   {
      log_string( "Terminating program. Infinite loop detected during bootup." );
      shutdown_mud( "Infinite loop detected during bootup." );
      abort(  );
   }

   log_string( "&RPossible infinite loop detected during game operation. Restarting game_loop()." );
   signal( SIGALRM, caught_alarm ); // Have to reset the signal handler or the next hit will deadlock.
   game_loop(  );

   /*
    * Clean up the loose ends. 
    */
   close_mud(  );

   /*
    * That's all, folks.
    */
   log_string( "Normal termination of game." );
   log_string( "Cleaning up Memory.\033[0m" );
   cleanup_memory(  );
   std::exit( EXIT_SUCCESS );
}

#ifdef MULTIPORT
bool process_forked( descriptor_data * d )
{
   if( !d->process )
   {
      d->connected = CON_PLAYING;
      return false;
   }

   int status;
   int result = waitpid( d->process, &status, WNOHANG );

   if( result == -1 || status > 0 )
   {
      d->connected = CON_PLAYING;
      d->process = 0;

      if( result > 0 )
      {
         auto* ch = d->original ? d->original : d->character;
         if( ch )
            ch->printf( "Process exited with status code %d.\r\n", status );
      }
   }
   return ( d->connected == CON_FORKED ); // Return true if still forked (continue loop)
}
#endif

void process_input( void )
{
   /*
    * Kick out descriptors with raised exceptions
    * or have been idle, then check for input.
    */
   for( auto it = dlist.begin(  ); it != dlist.end(  ); )
   {
      descriptor_data *d = *it;
      ++it;

#ifdef MULTIPORT
      if( d->connected == CON_FORKED )
      {
         if( process_forked( d ) )
            continue;
      }
#endif

      // Check for disconnect, or an exception. Hasta la vista baby!
      if( d->disconnect || FD_ISSET( d->descriptor, &exc_set ) )
      {
         if( d->character && d->connected >= CON_PLAYING )
            d->character->save(  );
         d->outbuf.clear(  );
         close_socket( d, true );
         continue;
      }

      // Check to see if they idled out.
      if( d->is_idle_timeout() )
      {
         update_connhistory( d, CONNTYPE_IDLE );
         d->outbuf.clear( );
         close_socket( d, true );
         continue;
      }
      else
      {
         d->fcommand = false;

         // Check to see if they have input ready.
         if( FD_ISSET( d->descriptor, &in_set ) )
         {
            d->idle = 0;

            if( d->character )
               d->character->timer = 0;

            // Something bad must have happened while attempting to read. Clear the output flags and disconnect them.
            if( !d->read(  ) )
            {
               FD_CLR( d->descriptor, &out_set );
               if( d->character && d->connected >= CON_PLAYING )
                  d->character->save(  );
               d->outbuf.clear(  );
               update_connhistory( d, CONNTYPE_LINKDEAD );
               close_socket( d, false );
               continue;
            }
         }

         // Check for input from the dns 
         if( ( d->connected == CON_PLAYING || d->character ) && d->ifd != -1 && FD_ISSET( d->ifd, &in_set ) )
            d->process_dns(  );

         // If they're waiting on a cooldown to finish, decrement and process them again in the next loop.
         if( d->character && d->character->wait > 0 )
         {
            --d->character->wait;
            continue;
         }

         // WOO! Time to finally read in what they typed!
         d->read_from_buffer(  );

         // If it wasn't just a blank line, it's time to process that.
         if( !d->incomm.empty(  ) )
         {
            d->fcommand = true;

            if( d->character && !d->character->has_aflag( AFF_SPAMGUARD ) )
               d->character->stop_idling(  );

            std::string cmdline = std::move( d->incomm );
            d->incomm.clear(  );

            if( !d->pagebuf.empty(  ) )
               d->set_pager_input( cmdline );
            else
            {
               switch ( d->connected )
               {
                  default:
                     d->nanny( cmdline );
                     break;

                  case CON_EDITING:
                     d->character->edit_buffer( cmdline );
                     break;

                  case CON_PLAYING:
                     auto& ch = d->original ? d->original : d->character;

                     ch->pcdata->cmd_recurse = 0;

                     interpret( d->character, cmdline );
                     break;
               }
            }
         }
      }
   }
}

void process_output( void )
{
   /*
    * Output.
    */
   for( auto it = dlist.begin(  ); it != dlist.end(  ); )
   {
      descriptor_data *d = *it;
      ++it;

      // True if the disconnect flag is set. Hasta la vista baby!
      if( d->disconnect )
      {
         d->outbuf.clear(  );
         close_socket( d, true );
         continue;
      }

      if( ( d->fcommand || d->outbuf.length(  ) > 0 || d->pagebuf.length(  ) > 0 ) && FD_ISSET( d->descriptor, &out_set ) )
      {
         if( !d->pagebuf.empty(  ) )
         {
            // Something bad must have happened here if no output went through.
            if( !d->pager_output(  ) )
            {
               if( d->character && d->connected >= CON_PLAYING )
                  d->character->save(  );
               d->outbuf.clear(  );
               close_socket( d, false );
            }
         }
         // Something bad must have happened here if no output went through.
         else if( !d->flush_buffer( true ) )
         {
            if( d->character && d->connected >= CON_PLAYING )
               d->character->save(  );
            d->outbuf.clear(  );
            close_socket( d, false );
         }
      }
   }
}

// Synchronize to a clock. This is the beating heart of the MUD.
void pulse_sync( )
{
   using namespace std::chrono_literals;

   // Use steady_clock to ensure timing is monotonic and unaffected by NTP syncs.
   static auto next_pulse = std::chrono::steady_clock::now();

   // Calculate the duration of one pulse based on pulses per second. Defaults to 4 per second.
   // Since sysdata->pulsepersec is editable during runtime, be VERY careful with the values.
   auto pulse_duration = std::chrono::nanoseconds( 1'000'000'000 / sysdata->pulsepersec );
   next_pulse += pulse_duration;

   // Sleep until the exact time the next pulse should occur.
   // If the system is running behind, this will return immediately.
   std::this_thread::sleep_until( next_pulse );

   // Update global time
   current_time = std::chrono::system_clock::now();
}

// It all looks so simple from here, doesn't it?
void game_loop( void )
{
   // Main loop 
   while( !mud_down )
   {
      accept_new( control );

      // Primitive, yet effective infinite loop catcher. At least that's the idea.
      set_alarm( 30 );
      alarm_section = "game_loop";

      // If no descriptors are present, why bother processing input for them?
      if( !dlist.empty(  ) )
         process_input(  );

#if !defined(__CYGWIN__)
#ifdef MULTIPORT
      mud_recv_message(  );
#endif
#endif

      // Autonomous game motion. Stops processing when there are no people at all online.
      if( !dlist.empty(  ) )
         update_handler(  );

      // Event handling. Will continue to process even with nobody around. Keeps areas fresh this way.
      run_events( current_time );

      // If no descriptors are present, why bother processing output for them?
      if( !dlist.empty(  ) )
         process_output(  );

      // Update pulse timer.
      pulse_sync();

      // Dunno if it needs to be reset, but I'll do it anyway. End of the loop here. 
      set_alarm( 0 );
   }
   // End of main game loop.
   // Returns back to 'main', and will result in MUD shutdown.
}

/*
 * Clean all memory on exit to help find leaks
 * Yeah I know, one big ugly function -Druid
 * Added to AFKMud by Samson on 5-8-03.
 * Do not remove this function again lizard! ASan needs it or you get spam on a controlled shutdown.
 */
void cleanup_memory( void )
{
   fprintf( stdout, "%s", "Quote List.\n" );
   free_quotes(  );

   fprintf( stdout, "%s", "Random Environment Data.\n" );
   free_envs(  );

   fprintf( stdout, "%s", "Auction Sale Data.\n" );
   free_sales(  );

   fprintf( stdout, "%s", "Project Data.\n" );
   free_projects(  );

   fprintf( stdout, "%s", "Ban Data.\n" );
   free_bans(  );

   fprintf( stdout, "%s", "Auth List.\n" );
   free_all_auths(  );

   fprintf( stdout, "%s", "Morph Data.\n" );
   free_morphs(  );

   fprintf( stdout, "%s", "Rune Data.\n" );
   free_runedata(  );

   fprintf( stdout, "%s", "Immortal Hosts Data.\n" );
   free_immhosts();

   fprintf( stdout, "%s", "Connection History Data.\n" );
   free_connhistory( 0 );

   fprintf( stdout, "%s", "Slay Table.\n" );
   free_slays(  );

   fprintf( stdout, "%s", "Holidays.\n" );
   free_holidays(  );

   fprintf( stdout, "%s", "Specfun List.\n" );
   free_specfuns(  );

   fprintf( stdout, "%s", "Wizinfo Data.\n" );
   clear_wizinfo(  );

   fprintf( stdout, "%s", "Ships.\n" );
   free_ships(  );

   fprintf( stdout, "%s", "Overland Data.\n" );
   free_continents(  );

   fprintf( stdout, "%s", "Mixtures and Liquids.\n" );
   free_liquiddata(  );

   fprintf( stdout, "%s", "Treasure weapon table.\n" );
   free_weapon_table(  );

   fprintf( stdout, "%s", "DNS Cache data.\n" );
   free_dns_list(  );

   fprintf( stdout, "%s", "Local Channels.\n" );
   free_mudchannels(  );

   // Helps
   fprintf( stdout, "%s", "Helps.\n" );
   free_helps(  );

   // Commands
   fprintf( stdout, "%s", "Commands.\n" );
   free_commands(  );

#ifdef MULTIPORT
   // Shell Commands
   fprintf( stdout, "%s", "Shell Commands.\n" );
   free_shellcommands(  );
#endif

   // Socials
   fprintf( stdout, "%s", "Socials.\n" );
   free_socials(  );

   // Languages
   fprintf( stdout, "%s", "Languages.\n" );
   free_tongues(  );

   // Boards
   fprintf( stdout, "%s", "Boards.\n" );
   free_boards(  );

   // Events
   fprintf( stdout, "%s", "Events.\n" );
   free_all_events(  );

   // Find and eliminate all running chess games
   fprintf( stdout, "%s", "Ending chess games.\n" );
   free_all_chess_games(  );

   // Whack supermob
   fprintf( stdout, "%s", "Whacking supermob.\n" );
   if( supermob )
   {
      supermob->from_room(  );
      charlist.remove( supermob );
      deleteptr( supermob );
   }

   // Free Characters
   fprintf( stdout, "%s", "Characters.\n" );
   extract_all_chars(  );

   // Free Objects
   fprintf( stdout, "%s", "Objects.\n" );
   extract_all_objs(  );

   // Descriptors
   fprintf( stdout, "%s", "Descriptors.\n" );
   free_all_descs(  );

   // Deities
   fprintf( stdout, "%s", "Deities.\n" );
   free_deities(  );

   // Clans
   fprintf( stdout, "%s", "Clans.\n" );
   free_clans(  );

   // Realms
   fprintf( stdout, "%s", "Realms.\n" );
   free_realms(  );

   // Races
   fprintf( stdout, "%s", "Races.\n" );
   free_all_races(  );

   // Classes
   fprintf( stdout, "%s", "Classes.\n" );
   free_all_classes(  );

   // Teleport lists
   fprintf( stdout, "%s", "Teleport Data.\n" );
   free_teleports(  );

   // Areas - this includes killing off the hash tables and such
   fprintf( stdout, "%s", "Area Data Tables.\n" );
   close_all_areas(  );

   // Get rid of auction pointer MUST BE AFTER OBJECTS DESTROYED
   fprintf( stdout, "%s", "Auction.\n" );
   clear_auction(  );

   // Skills
   fprintf( stdout, "%s", "Skills and Herbs.\n" );
   free_skills(  );

   // Prog Act lists
   fprintf( stdout, "%s", "Mudprog act lists.\n" );
   free_prog_actlists(  );

   // Questbit data
   fprintf( stdout, "%s", "Abit/Qbit Data.\n" );
   free_questbits(  );

   free_mssp_info();

   // Wizlist
   fprintf( stdout, "%s", "Wizlist Data\n" );
   free_wizlist_data( );

   // Web version
   fprintf( stdout, "%s", "Web Wizlist Data\n" );
   free_wizlist_web_data( );

#if defined(SQL)
   fprintf( stdout, "%s", "Closing database connection.\n" );
   close_db(  );
#endif

   // Last but not least, close the libdl and dispose of sysdata - Samson
   fprintf( stdout, "%s", "System data.\n" );
   dlclose( sysdata->dlHandle );
   deleteptr( sysdata );
   fprintf( stdout, "%s", "Memory cleanup complete, exiting.\n" );
}

void bailout( int sig )
{
   log_string( "Bailout signal received. Shutting down gracefully..." );

   mud_down = true;

   // Perform universal cleanup
   close_mud();
   cleanup_memory();

   std::exit( EXIT_SUCCESS );
}

// Heh, nice one Darien :)
void moron_check( void )
{
   uid_t uid;

   if( ( uid = getuid(  ) ) == 0 )
   {
      log_string( "Warning, you are a moron. Do not run as root." );
      std::exit( EXIT_FAILURE );
   }
}

int main( int argc, char **argv )
{
   // Init time.
   current_time = std::chrono::system_clock::now(); // This is used throughout the codebase to represent the current time. Saves on a whole lot of calls to std::chrono this way. Updated in pulse_sync().
   str_boot_time = c_time( current_time, -1 );      // Records when the mud was last rebooted.
   new_pfile_time_t = current_time + std::chrono::hours( 24 ); // For the pfile cleanup. Starts 24 hours after bootup.
   mud_start_time = current_time;

   moron_check(  ); // Don't let someone run as root, cause that's dumb.

   bool fCopyOver = false;

   // Get the port number.
   if( argc > 1 )
   {
      if( !is_number( argv[1] ) )
      {
         fprintf( stderr, "Usage: %s [port #]\n", argv[0] );
         std::exit( EXIT_FAILURE );
      }
      else if( ( mud_port = atoi( argv[1] ) ) <= 1024 )
      {
         fprintf( stderr, "%s", "Port number must be above 1024.\n" );
         std::exit( EXIT_FAILURE );
      }

      if( argv[2] && argv[2][0] )
      {
         fCopyOver = true;
         control = atoi( argv[3] );
      }
      else
         fCopyOver = false;
   }

   // Initialize all startup functions of the mud. 
   init_mud( fCopyOver, mud_port );

   // Interrupt signal from terminal. Like if you use the qstart script and hit CTRL+C. This will call the bailout function above and gracefully shut down.
   signal( SIGINT, bailout );

   /*
    * Set various signal traps, waiting until after completing all bootup operations
    * before doing so because crashes during bootup should not be intercepted. Samson 3-11-04
    */
   signal( SIGTERM, SigTerm );   /* Catch kill signals */
   signal( SIGPIPE, SIG_IGN );
   signal( SIGALRM, caught_alarm );
   signal( SIGUSR1, SigUser1 );  /* Catch user defined signals */
   signal( SIGUSR2, SigUser2 );

#ifdef MULTIPORT
   signal( SIGCHLD, SigChld );
#endif

   // Descriptor list will be populated if this was a hotboot.
   if( dlist.empty(  ) )
      log_string( "No people online yet. Suspending autonomous update handlers." );

   // Sick isn't it? The whole game being run inside of one little statement..... :P 
   game_loop(  );

   // Clean up the loose ends. 
   close_mud(  );

   // That's all, folks.
   log_string( "Normal termination of game." );
   log_string( "Cleaning up Memory.\033[0m" );
   cleanup_memory(  );
   std::exit( EXIT_SUCCESS );
}
