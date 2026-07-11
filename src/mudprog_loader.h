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
 *                         MUDProg Template Loader                          *
 ****************************************************************************/

template < class N > void fread_afk_mudprog( std::ifstream & stream, mud_prog_data * mprg, N * prog_target )
{
   std::string key;
   while( stream >> key )
   {
      if( key == "Arglist" )
      {
         mprg->arglist = fread_line( stream );
         mprg->fileprog = false;

         switch( mprg->type )
         {
            case IN_FILE_PROG:
               mprog_file_read( prog_target, mprg->arglist );
               break;
            default:
               break;
         }
      }
      else if( key == "Comlist" )
         mprg->comlist = fread_line( stream );
      else if( key == "Progtype" )
      {
         mprg->type = mprog_name_to_type( fread_line( stream ) );
         prog_target->progtypes.set( mprg->type );
      }
      else if( key == "#ENDPROG" )
         return;
      else
      {
         bug( "{}: Bad section '{}' in mudprogs - skipping.", __func__, key );
         fread_to_eol( stream );
      }
   }
}

template < class N > void mprog_file_read( N * prog_target, std::string_view file )
{
   mud_prog_data *mprg = nullptr;

   std::filesystem::path filename = std::format( "{}{}", PROG_DIR, file );
   std::ifstream stream( filename );
   if( !stream.is_open() )
   {
      bug( "{}: Cannot open {} for reading: {}", __func__, filename.string(), std::strerror(errno) );
      return;
   }

   std::string key;
   while( stream >> key )
   {
      if( key == "#ENDFILE" )
         break;
      else if( key == "#MUDPROG" )
      {
         mprg = new mud_prog_data;

         while( stream >> key )
         {
            if( key == "#ENDPROG" )
            {
               prog_target->progtypes.set( mprg->type );
               prog_target->mudprogs.push_back( mprg );
               break;
            }
            else if( key == "Arglist" )
            {
               fread_string( mprg->arglist, stream );
               mprg->fileprog = false;

               switch ( mprg->type )
               {
                  case IN_FILE_PROG:
                     bug( "{}: Nested file programs are not allowed.", __func__ );
                     deleteptr( mprg );
                     break;

                  default:
                     break;
               }
            }
            else if( key == "Comlist" )
               mprg->comlist = fread_line( stream );
            else if( key == "Progtype" )
               mprg->type = mprog_name_to_type( fread_line( stream ) );
            else
            {
               bug( "{}: Bad section '{}' in {} - skipping.", __func__, key, filename.string() );
               fread_to_eol( stream );
            }
         }
      }
   }
   stream.close();
}
