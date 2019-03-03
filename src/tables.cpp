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
 *                         Table load/save Module                           *
 ****************************************************************************/

#if !defined(WIN32)
#include <dlfcn.h>
#else
#include <windows.h>
#define dlsym( handle, name ) ( (void*)GetProcAddress( (HINSTANCE) (handle), (name) ) )
#define dlerror() GetLastError()
#endif
#include "mud.h"
#include "language.h"

SPELLF( spell_notfound );

list < lang_data * >langlist;

SPELL_FUN *spell_function( const string & name )
{
   void *funHandle;

   funHandle = dlsym( sysdata->dlHandle, name.c_str(  ) );
   if( dlerror(  ) )
      return ( SPELL_FUN * ) spell_notfound;

   return ( SPELL_FUN * ) funHandle;
}

DO_FUN *skill_function( const string & name )
{
   void *funHandle;

   funHandle = dlsym( sysdata->dlHandle, name.c_str(  ) );
   if( dlerror(  ) )
      return ( DO_FUN * ) skill_notfound;

   return ( DO_FUN * ) funHandle;
}

lcnv_data::lcnv_data(  )
{
   old.clear(  );
   lnew.clear(  );
}

lang_data::lang_data(  )
{
}

lang_data::~lang_data(  )
{
   list < lcnv_data * >::iterator lcnv;

   for( lcnv = prelist.begin(  ); lcnv != prelist.end(  ); )
   {
      lcnv_data *lpre = *lcnv;
      ++lcnv;

      prelist.remove( lpre );
      deleteptr( lpre );
   }

   for( lcnv = cnvlist.begin(  ); lcnv != cnvlist.end(  ); )
   {
      lcnv_data *lc = *lcnv;
      ++lcnv;

      cnvlist.remove( lc );
      deleteptr( lc );
   }
   langlist.remove( this );
}

void free_tongues( void )
{
   list < lang_data * >::iterator ln;

   for( ln = langlist.begin(  ); ln != langlist.end(  ); )
   {
      lang_data *lang = *ln;
      ++ln;

      deleteptr( lang );
   }
}

/*
 * Tongues / Languages loading/saving functions - Altrag
 */
void fread_cnv( FILE * fp, lang_data * lng, bool pre )
{
   lcnv_data *cnv;
   char letter;

   for( ;; )
   {
      letter = fread_letter( fp );
      if( letter == '~' || letter == EOF )
         break;
      ungetc( letter, fp );

      cnv = new lcnv_data;
      cnv->old = fread_word( fp );
      cnv->lnew = fread_word( fp );
      fread_to_eol( fp );
      if( pre )
         lng->prelist.push_back( cnv );
      else
         lng->cnvlist.push_back( cnv );
   }
}

void load_tongues(  )
{
   FILE *fp;
   lang_data *lng;
   char *word;
   char letter;

   if( !( fp = fopen( TONGUE_FILE, "r" ) ) )
   {
      perror( "Load_tongues" );
      return;
   }
   for( ;; )
   {
      letter = fread_letter( fp );
      if( letter == EOF )
         break;
      else if( letter == '*' )
      {
         fread_to_eol( fp );
         continue;
      }
      else if( letter != '#' )
      {
         bug( "%s: Letter '%c' not #.", __func__, letter );
         exit( 1 );
      }
      word = fread_word( fp );
      if( !str_cmp( word, "end" ) )
         break;
      fread_to_eol( fp );
      lng = new lang_data;
      lng->name = word;
      fread_cnv( fp, lng, true );
      fread_string( lng->alphabet, fp );
      fread_cnv( fp, lng, false );
      fread_to_eol( fp );
      langlist.push_back( lng );
   }
   FCLOSE( fp );
}
