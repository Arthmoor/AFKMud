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
 *                         Table load/save Module                           *
 ****************************************************************************/

#include <dlfcn.h> // For libdl - Trax
#include <filesystem>
#include <fstream>
#include "mud.h"
#include "language.h"

SPELLF( spell_notfound );

std::list<lang_data *> langlist;

SPEC_FUN *m_spec_lookup( std::string_view name )
{
   void *funHandle;
   const char *error;

   // Perform the symbol lookup
   funHandle = dlsym( sysdata->dlHandle, name.data() );

   // Check the returned error if this came back NULL
   if( funHandle == NULL )
   {
      // Grab the error message and report it.
      if( ( error = dlerror() ) != NULL )
      {
         bug( "{}: Error locating {} in symbol table. {}", __func__, name, error );
         return nullptr;

         // Edge case. Apparently a symbol can be valid but point to a NULL. This catches those.
         bug( "{}: Symbol {} found as NULL pointer.", __func__, name );
         return nullptr;
      }
   }
   return ( SPEC_FUN * ) funHandle;
}

SPELL_FUN *spell_function( std::string_view name )
{
   void *funHandle;
   const char *error;

   // Perform the symbol lookup
   funHandle = dlsym( sysdata->dlHandle, name.data() );

   // Check the returned error if this came back NULL
   if( funHandle == NULL )
   {
      // Grab the error message and report it.
      if( ( error = dlerror() ) != NULL )
      {
         bug( "{}: Error locating {} in symbol table. {}", __func__, name, error );
         return ( SPELL_FUN * ) spell_notfound;

         // Edge case. Apparently a symbol can be valid but point to a NULL. This catches those.
         bug( "{}: Symbol {} found as NULL pointer.", __func__, name );
         return ( SPELL_FUN * ) spell_notfound;
      }
   }
   return ( SPELL_FUN * ) funHandle;
}

DO_FUN *skill_function( std::string_view name )
{
   void *funHandle;
   const char *error;

   // Perform the symbol lookup
   funHandle = dlsym( sysdata->dlHandle, name.data() );

   // Check the returned error if this came back NULL
   if( funHandle == NULL )
   {
      // Grab the error message and report it.
      if( ( error = dlerror() ) != NULL )
      {
         bug( "{}: Error locating {} in symbol table. {}", __func__, name, error );
         return ( DO_FUN * ) skill_notfound;

         // Edge case. Apparently a symbol can be valid but point to a NULL. This catches those.
         bug( "{}: Symbol {} found as NULL pointer.", __func__, name );
         return ( DO_FUN * ) skill_notfound;
      }
   }
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
   std::list<lcnv_data *>::iterator lcnv;

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
   for( auto it = langlist.begin(  ); it != langlist.end(  ); )
   {
      lang_data *lang = *it;
      ++it;

      deleteptr( lang );
   }
}

/*
 * Tongues / Languages loading/saving functions - Altrag
 */
void fread_cnv( std::ifstream & stream, lang_data * lng, bool pre )
{
   for( ;; )
   {
      char letter = fread_letter( stream );
      if( letter == '~' || letter == EOF )
         break;

      lcnv_data *cnv = new lcnv_data;
      cnv->old = fread_word( stream );
      cnv->lnew = fread_word( stream );
      fread_to_eol( stream );
      if( pre )
         lng->prelist.push_back( cnv );
      else
         lng->cnvlist.push_back( cnv );
   }
}

void load_tongues(  )
{
   lang_data *lng;

   std::ifstream stream( std::filesystem::path{TONGUE_FILE} );
   if( !stream.is_open() )
   {
      bug( "{}: Cannot open {} for reading: {}", __func__, TONGUE_FILE, std::strerror(errno) );
      return;
   }

   for( ;; )
   {
      char letter = fread_letter( stream );
      if( letter == EOF || letter == '\0' )
         break;
      else if( letter == '*' )
      {
         fread_to_eol( stream );
         continue;
      }
      else if( letter != '#' )
      {
         bug( "{}: Letter '{}' not #.", __func__, letter );
         std::exit( EXIT_FAILURE );
      }
      std::string word = fread_word( stream );
      if( !str_cmp( word, "end" ) )
         break;
      fread_to_eol( stream );
      lng = new lang_data;
      lng->name = word;
      fread_cnv( stream, lng, true );
      lng->alphabet = fread_line( stream );
      fread_cnv( stream, lng, false );
      fread_to_eol( stream );
      langlist.push_back( lng );
   }
   stream.close();
}
