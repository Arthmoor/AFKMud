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
 *                      MySQL Database management module                    *
 ****************************************************************************/
#if !defined(__CYGWIN__)

#include <cstdarg>
#include "mud.h"
#include "sql.h"

MYSQL myconn;

void init_mysql(  )
{
   if( !mysql_init( &myconn ) )
   {
      mysql_close( &myconn );
      bug( "%s: mysql_init() failed.", __func__ );
      log_printf( "Error: %s.", mysql_error( &myconn ) );
      return;
   }

   if( !mysql_real_connect( &myconn, sysdata->dbserver.c_str(  ), sysdata->dbuser.c_str(  ), sysdata->dbpass.c_str(  ), sysdata->dbname.c_str(  ), 0, nullptr, 0 ) )
   {
      mysql_close( &myconn );
      bug( "%s: mysql_real_connect() failed.", __func__ );
      log_printf( "Error: %s.", mysql_error( &myconn ) );
      return;
   }
   mysql_options( &myconn, MYSQL_OPT_RECONNECT, "1" );
   log_string( "Connection to mysql database established." );
}

void close_db( void )
{
   mysql_close( &myconn );
}

int mysql_safe_query( const char *fmt, ... )
{
   va_list argp;
   int i = 0;
   double j = 0;
   char *s = 0, *out = 0;
   const char *p = 0;
   char safe[MSL], query[MSL];

   *query = '\0';
   *safe = '\0';

   va_start( argp, fmt );

   for( p = fmt, out = query; *p != '\0'; p++ )
   {
      if( *p != '%' )
      {
         *out++ = *p;
         continue;
      }

      switch ( *++p )
      {
         default:
            break;

         case 'c':
            i = va_arg( argp, int );
            out += sprintf( out, "%c", i );
            break;

         case 's':
            s = va_arg( argp, char * );
            if( !s )
            {
               out += sprintf( out, " " );
               break;
            }
            mysql_real_escape_string( &myconn, safe, s, strlen( s ) );
            out += sprintf( out, "%s", safe );
            *safe = '\0';
            break;

         case 'd':
            i = va_arg( argp, int );
            out += sprintf( out, "%d", i );
            break;

         case 'f':
            j = va_arg( argp, double );
            out += sprintf( out, "%f", j );
            break;

         case '%':
            out += sprintf( out, "%%" );
            break;
      }
   }

   *out = '\0';
   va_end( argp );
   // log_string( query );

   return ( mysql_real_query( &myconn, query, strlen( query ) ) );
}

#endif
