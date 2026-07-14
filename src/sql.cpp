/****************************************************************************
 *                   ^     +----- |  / ^     ^ |     | +-\                  *
 *                  / \    |      | /  |\   /| |     | |  \                 *
 *                 /   \   +---   |<   | \ / | |     | |  |                 *
 *                /-----\  |      | \  |  v  | |     | |  /                 *
 *               /       \ |      |  \ |     | +-----+ +-/                  *
 ****************************************************************************
 * AFKMud Copyright 1997-2026 by Roger Libiez (Samson),                     *
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
 *                      MySQL Database Management Module                    *
 ****************************************************************************/

#include <stdexcept>
#include "mud.h"
#include "sql.h"

/*
 * Internal Database Manager class to encapsulate the C API
 */
MySQLDatabase::MySQLDatabase( const std::string & host, const std::string & user, const std::string & pass, const std::string & dbname )
{
   if( !mysql_init( &conn ) )
      throw std::runtime_error( "mysql_init() failed" );

   if( !mysql_real_connect( &conn, host.c_str(), user.c_str(), pass.c_str(), dbname.c_str(), 0, nullptr, 0 ) )
   {
      std::string err = mysql_error( &conn );
      mysql_close( &conn );

      throw std::runtime_error( err );
   }
   mysql_options( &conn, MYSQL_OPT_RECONNECT, "1" );
}

MySQLDatabase::~MySQLDatabase()
{
   mysql_close( &conn );
}

bool MySQLDatabase::ping()
{
   return mysql_ping( &conn ) == 0;
}

std::string MySQLDatabase::get_error()
{
   return mysql_error( &conn );
}

int MySQLDatabase::execute( std::string_view sql )
{
   return mysql_real_query( &conn, sql.data(), sql.length() );
}

// Global instance management
std::unique_ptr<MySQLDatabase> db = nullptr;

/*
 * Initialize the MySQL connection
 */
void init_mysql( )
{
   try
   {
      db = std::make_unique<MySQLDatabase>( sysdata->dbserver, sysdata->dbuser, sysdata->dbpass, sysdata->dbname );

      log_string( "Connection to mysql database established." );
   }
   catch( const std::exception & e )
   {
      bug( "{}: Connection failed: {}", __func__, e.what() );
      std::exit( EXIT_FAILURE );
   }
}

/*
 * Terminate the MySQL connection
 */
void close_db( )
{
   db.reset( );
}

/*
 * Modernized safe query.
 * Replaces the legacy va_list/sprintf implementation.
 */
int mysql_safe_query( std::string_view fmt, auto&&... args )
{
   if( !db )
   {
      bug( "{}: DB not initialized.", __func__ );
      return -1;
   }

   try
   {
      std::string query = std::vformat( fmt, std::make_format_args( args... ) );

      return db->execute( query );
   }
   catch( const std::exception & e )
   {
      bug( "{}: Formatting error: {}", __func__, e.what() );
      return -1;
   }
}
