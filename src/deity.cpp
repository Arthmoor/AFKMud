/****************************************************************************
 *                   ^     +----- |  / ^     ^ |     | +-\                  *
 *                  / \    |      | /  |\   /| |     | |  \                 *
 *                 /   \   +---   |<   | \ / | |     | |  |                 *
 *                /-----\  |      | \  |  v  | |     | |  /                 *
 *               /       \ |      |  \ |     | +-----+ +-/                  *
 ****************************************************************************
 * AFKMud Copyright 1997-2010 by Roger Libiez (Samson),                     *
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
 *                           Deity handling module                          *
 ****************************************************************************/

/* Put together by Rennard for Realms of Despair.  Brap on... */

#include "mud.h"
#include "deity.h"
#include "mobindex.h"
#include "objindex.h"
#include "raceclass.h"
#include "roomindex.h"

bool check_pets( char_data *, mob_index * );
room_index *recall_room( char_data * );
void bind_follower( char_data *, char_data *, int, int );

list < deity_data * >deitylist;

deity_data::deity_data(  )
{
   init_memory( &race_allowed, &dig_corpse, sizeof( dig_corpse ) );
}

deity_data::~deity_data(  )
{
   deitylist.remove( this );
}

void free_deities( void )
{
   list < deity_data * >::iterator dt;

   for( dt = deitylist.begin(  ); dt != deitylist.end(  ); )
   {
      deity_data *deity = *dt;
      ++dt;

      deleteptr( deity );
   }
}

/* Get pointer to deity structure from deity name */
deity_data *get_deity( const string & name )
{
   list < deity_data * >::iterator ideity;

   for( ideity = deitylist.begin(  ); ideity != deitylist.end(  ); ++ideity )
   {
      deity_data *deity = *ideity;

      if( !str_cmp( name, deity->name ) )
         return deity;
   }
   return NULL;
}

void write_deity_list( void )
{
   list < deity_data * >::iterator ideity;
   FILE *fpout;
   char filename[256];

   snprintf( filename, 256, "%s%s", DEITY_DIR, DEITY_LIST );
   fpout = fopen( filename, "w" );
   if( !fpout )
      bug( "%s: FATAL: cannot open deity.lst for writing!", __FUNCTION__ );
   else
   {
      for( ideity = deitylist.begin(  ); ideity != deitylist.end(  ); ++ideity )
      {
         deity_data *deity = *ideity;

         fprintf( fpout, "%s\n", deity->filename.c_str(  ) );
      }
      fprintf( fpout, "%s", "$\n" );
      FCLOSE( fpout );
   }
}

/* Save a deity's data to its data file */
const int DEITY_VERSION = 3;
/* Added for deity file compatibility. Adjani, 1-31-04 */
/* Raised to 2 by Samson to support multiple class/race settings - 5-17-04 */
/* Raised to 3 by Samson to accomadate RISA flag changes - 7-12-04 */
void save_deity( deity_data * deity )
{
   FILE *fp;
   char filename[256];

   if( !deity )
   {
      bug( "%s: null deity pointer!", __FUNCTION__ );
      return;
   }

   if( deity->filename.empty(  ) )
   {
      bug( "%s: %s has no filename", __FUNCTION__, deity->name.c_str(  ) );
      return;
   }

   snprintf( filename, 256, "%s%s", DEITY_DIR, deity->filename.c_str(  ) );

   if( !( fp = fopen( filename, "w" ) ) )
   {
      bug( "%s: fopen", __FUNCTION__ );
      perror( filename );
   }
   else
   {
      fprintf( fp, "#VERSION %d\n", DEITY_VERSION );  /* Adjani, 1-31-04 */
      fprintf( fp, "%s", "#DEITY\n" );
      fprintf( fp, "Filename        %s~\n", deity->filename.c_str(  ) );
      fprintf( fp, "Name            %s~\n", deity->name.c_str(  ) );
      fprintf( fp, "Description     %s~\n", strip_cr( deity->deitydesc ).c_str(  ) );
      fprintf( fp, "Alignment       %d\n", deity->alignment );
      fprintf( fp, "Worshippers     %d\n", deity->worshippers );
      fprintf( fp, "Flee            %d\n", deity->flee );
      fprintf( fp, "Flee_npcraces   %d %d %d\n", deity->flee_npcrace[0], deity->flee_npcrace[1], deity->flee_npcrace[2] );
      fprintf( fp, "Flee_npcfoes    %d %d %d\n", deity->flee_npcfoe[0], deity->flee_npcfoe[1], deity->flee_npcfoe[2] );
      fprintf( fp, "Kill            %d\n", deity->kill );
      fprintf( fp, "Kill_magic      %d\n", deity->kill_magic );
      fprintf( fp, "Kill_npcraces   %d %d %d\n", deity->kill_npcrace[0], deity->kill_npcrace[1], deity->kill_npcrace[2] );
      fprintf( fp, "Kill_npcfoes    %d %d %d\n", deity->kill_npcfoe[0], deity->kill_npcfoe[1], deity->kill_npcfoe[2] );
      fprintf( fp, "Sac             %d\n", deity->sac );
      fprintf( fp, "Bury_corpse     %d\n", deity->bury_corpse );
      fprintf( fp, "Aid_spell       %d\n", deity->aid_spell );
      fprintf( fp, "Aid             %d\n", deity->aid );
      fprintf( fp, "Steal           %d\n", deity->steal );
      fprintf( fp, "Backstab        %d\n", deity->backstab );
      fprintf( fp, "Die             %d\n", deity->die );
      fprintf( fp, "Die_npcraces    %d %d %d\n", deity->die_npcrace[0], deity->die_npcrace[1], deity->die_npcrace[2] );
      fprintf( fp, "Die_npcfoes     %d %d %d\n", deity->die_npcfoe[0], deity->die_npcfoe[1], deity->die_npcfoe[2] );
      fprintf( fp, "Spell_aid       %d\n", deity->spell_aid );
      fprintf( fp, "Dig_corpse      %d\n", deity->dig_corpse );
      fprintf( fp, "Scorpse         %d\n", deity->scorpse );
      fprintf( fp, "Savatar         %d\n", deity->savatar );
      fprintf( fp, "Smount          %d\n", deity->smount ); /* Added by Tarl 24 Feb 02 */
      fprintf( fp, "Sminion         %d\n", deity->sminion );   /* Added by Tarl 24 Feb 02 */
      fprintf( fp, "Sdeityobj       %d\n", deity->sdeityobj );
      fprintf( fp, "Sdeityobj2      %d\n", deity->sdeityobj2 );   /* Added by Tarl 02 Mar 02 */
      fprintf( fp, "Srecall         %d\n", deity->srecall );
      fprintf( fp, "Sex             %s~\n", deity->sex < 0 ? "none" : npc_sex[deity->sex] ); /* Adjani, 2-18-04 */
      fprintf( fp, "Elements        %s %s %s\n", ris_flags[deity->element[0]], ris_flags[deity->element[1]], ris_flags[deity->element[2]] );
      fprintf( fp, "Affects         %s %s %s\n", aff_flags[deity->affected[0]], aff_flags[deity->affected[1]], aff_flags[deity->affected[2]] );
      fprintf( fp, "Suscepts        %s %s %s\n", ris_flags[deity->suscept[0]], ris_flags[deity->suscept[1]], ris_flags[deity->suscept[2]] );
      fprintf( fp, "Classes         %s~\n", deity->class_allowed.none(  )? "all" : bitset_string( deity->class_allowed, npc_class ) );
      fprintf( fp, "Races           %s~\n", deity->race_allowed.none(  )? "all" : bitset_string( deity->race_allowed, npc_race ) );
      fprintf( fp, "Npcrace         %s~\n", deity->npcrace[0] == -1 ? "none" : npc_race[deity->npcrace[0]] );  /* Adjani, 2-18-04 */
      fprintf( fp, "Npcrace2        %s~\n", deity->npcrace[1] == -1 ? "none" : npc_race[deity->npcrace[1]] );
      fprintf( fp, "Npcrace3        %s~\n", deity->npcrace[2] == -1 ? "none" : npc_race[deity->npcrace[2]] );
      fprintf( fp, "Npcfoe          %s~\n", deity->npcfoe[0] == -1 ? "none" : npc_race[deity->npcfoe[0]] ); /* Adjani, 2-18-04 */
      fprintf( fp, "Npcfoe2         %s~\n", deity->npcfoe[1] == -1 ? "none" : npc_race[deity->npcfoe[1]] );
      fprintf( fp, "Npcfoe3         %s~\n", deity->npcfoe[2] == -1 ? "none" : npc_race[deity->npcfoe[2]] );
      fprintf( fp, "Susceptnums     %d %d %d\n", deity->susceptnum[0], deity->susceptnum[1], deity->susceptnum[2] );
      fprintf( fp, "Elementnums     %d %d %d\n", deity->elementnum[0], deity->elementnum[1], deity->elementnum[2] );
      fprintf( fp, "Affectednums    %d %d %d\n", deity->affectednum[0], deity->affectednum[1], deity->affectednum[2] );
      fprintf( fp, "Spells          %d %d %d\n", deity->spell[0], deity->spell[1], deity->spell[2] ); /* Added by Tarl 24 Mar 02 */
      fprintf( fp, "Sspells         %d %d %d\n", deity->sspell[0], deity->sspell[1], deity->sspell[2] ); /* Added by Tarl 24 Mar 02 */
      fprintf( fp, "Objstat         %d\n", deity->objstat );
      fprintf( fp, "Recallroom      %d\n", deity->recallroom );   /* Samson */
      fprintf( fp, "Avatar          %d\n", deity->avatar ); /* Restored by Samson */
      fprintf( fp, "Mount           %d\n", deity->mount );  /* Added by Tarl 24 Feb 02 */
      fprintf( fp, "Minion          %d\n", deity->minion ); /* Added by Tarl 24 Feb 02 */
      fprintf( fp, "Deityobj        %d\n", deity->deityobj );  /* Restored by Samson */
      fprintf( fp, "Deityobj2       %d\n", deity->deityobj2 ); /* Added by Tarl 02 Mar 02 */
      fprintf( fp, "%s", "End\n\n" );
      fprintf( fp, "%s", "#END\n" );
      FCLOSE( fp );
   }
}

CMDF( do_savedeities )
{
   list < deity_data * >::iterator ideity;

   for( ideity = deitylist.begin(  ); ideity != deitylist.end(  ); ++ideity )
   {
      deity_data *deity = *ideity;

      save_deity( deity );
   }
}

/* Adjani, versions, with help from the ever-patient Xorith. 1-31-04 */
void fread_deity( deity_data * deity, FILE * fp, int filever )
{
   for( ;; )
   {
      const char *word = ( feof( fp ) ? "End" : fread_word( fp ) );

      if( word[0] == '\0' )
      {
         bug( "%s: EOF encountered reading file!", __FUNCTION__ );
         word = "End";
      }

      switch ( UPPER( word[0] ) )
      {
         default:
            bug( "%s: no match: %s", __FUNCTION__, word );
            fread_to_eol( fp );
            break;

         case '*':
            fread_to_eol( fp );
            break;

         case 'A':
            if( !str_cmp( word, "Affects" ) )
            {
               const char *ln;
               char temp[3][MSL];
               int value;

               ln = fread_line( fp );
               temp[0][0] = '\0';
               temp[1][0] = '\0';
               temp[2][0] = '\0';
               sscanf( ln, "%s %s %s", temp[0], temp[1], temp[2] );

               value = get_risflag( temp[0] );
               if( value < 0 || value >= MAX_RIS_FLAG )
                  bug( "%s: Invalid RISA flag for %s: %s", __FUNCTION__, word, temp[0] );
               else
                  deity->affected[0] = value;

               value = get_risflag( temp[1] );
               if( value < 0 || value >= MAX_RIS_FLAG )
                  bug( "%s: Invalid RISA flag for %s: %s", __FUNCTION__, word, temp[1] );
               else
                  deity->affected[1] = value;

               value = get_risflag( temp[2] );
               if( value < 0 || value >= MAX_RIS_FLAG )
                  bug( "%s: Invalid RISA flag for %s: %s", __FUNCTION__, word, temp[2] );
               else
                  deity->affected[2] = value;
               break;
            }

            /*
             * Formatted the code to read a little easier. 
             */
            if( !str_cmp( word, "affected" ) )  /* affected, affected2, affected3 - Adjani 1-31-04 */
            {
               if( filever == 0 )
                  deity->affected[0] = fread_number( fp );
               else
               {
                  const char *flag = fread_flagstring( fp );
                  int value;

                  if( !flag || flag[0] == '\0' )
                     break;

                  value = get_aflag( flag );
                  if( value < 0 || value >= MAX_AFFECTED_BY )
                     bug( "%s: Unknown affectflag for %s: %s", __FUNCTION__, word, flag );
                  else
                     deity->affected[0] = value;
               }
               break;
            }

            if( !str_cmp( word, "affected2" ) )
            {
               if( filever == 0 )
                  deity->affected[1] = fread_number( fp );
               else
               {
                  const char *flag = fread_flagstring( fp );
                  int value;

                  if( !flag || flag[0] == '\0' )
                     break;

                  value = get_aflag( flag );
                  if( value < 0 || value >= MAX_AFFECTED_BY )
                     bug( "%s: Unknown affectflag for %s: %s", __FUNCTION__, word, flag );
                  else
                     deity->affected[1] = value;
               }
               break;
            }

            if( !str_cmp( word, "affected3" ) )
            {
               if( filever == 0 )
                  deity->affected[2] = fread_number( fp );
               else
               {
                  const char *flag = fread_flagstring( fp );
                  int value;

                  if( !flag || flag[0] == '\0' )
                     break;

                  value = get_aflag( flag );
                  if( value < 0 || value >= MAX_AFFECTED_BY )
                     bug( "%s: Unknown affectflag for %s: %s", __FUNCTION__, word, flag );
                  else
                     deity->affected[2] = value;
               }
               break;
            }
            if( !str_cmp( word, "Affectednums" ) )
            {
               deity->affectednum[0] = fread_number( fp );
               deity->affectednum[1] = fread_number( fp );  /* Added by Tarl 24 Feb 02 */
               deity->affectednum[2] = fread_number( fp );  /* Added by Tarl 24 Feb 02 */
               break;
            }
            KEY( "Affectednum", deity->affectednum[0], fread_number( fp ) );
            KEY( "Affectednum2", deity->affectednum[1], fread_number( fp ) ); /* Added by Tarl 24 Feb 02 */
            KEY( "Affectednum3", deity->affectednum[2], fread_number( fp ) ); /* Added by Tarl 24 Feb 02 */
            KEY( "Aid", deity->aid, fread_number( fp ) );
            KEY( "Aid_spell", deity->aid_spell, fread_number( fp ) );
            KEY( "Alignment", deity->alignment, fread_number( fp ) );
            KEY( "Avatar", deity->avatar, fread_number( fp ) );   /* Restored by Samson */
            break;

         case 'B':
            KEY( "Backstab", deity->backstab, fread_number( fp ) );
            KEY( "Bury_corpse", deity->bury_corpse, fread_number( fp ) );
            break;

         case 'C':
            /*
             * Cleaned up way to handle deity classes - Samson 5-17-04 
             */
            if( !str_cmp( word, "Classes" ) )
            {
               string Class, flag;
               int value;

               Class = fread_flagstring( fp );

               if( !str_cmp( Class, "all" ) )
                  deity->class_allowed.reset(  );
               else
               {
                  while( !Class.empty() )
                  {
                     Class = one_argument( Class, flag );
                     value = get_pc_class( flag );
                     if( value < 0 || value > MAX_CLASS )
                        bug( "Unknown PC Class: %s", flag.c_str() );
                     else
                        deity->class_allowed.set( value );
                  }
               }
               break;
            }
            break;

         case 'D':
            KEY( "Deityobj", deity->deityobj, fread_number( fp ) );  /* Restored by Samson */
            KEY( "Deityobj2", deity->deityobj2, fread_number( fp ) );   /* Added by Tarl 02 Mar 02 */
            STDSKEY( "Description", deity->deitydesc );
            KEY( "Die", deity->die, fread_number( fp ) );
            if( !str_cmp( word, "Die_npcraces" ) )
            {
               deity->die_npcrace[0] = fread_number( fp );
               deity->die_npcrace[1] = fread_number( fp );
               deity->die_npcrace[2] = fread_number( fp );
               break;
            }
            if( !str_cmp( word, "Die_npcfoes" ) )
            {
               deity->die_npcfoe[0] = fread_number( fp );
               deity->die_npcfoe[1] = fread_number( fp );
               deity->die_npcfoe[2] = fread_number( fp );
               break;
            }
            KEY( "Die_npcrace", deity->die_npcrace[0], fread_number( fp ) );
            KEY( "Die_npcrace2", deity->die_npcrace[1], fread_number( fp ) ); /* Adjani, 1-24-04 */
            KEY( "Die_npcrace3", deity->die_npcrace[2], fread_number( fp ) );
            KEY( "Die_npcfoe", deity->die_npcfoe[0], fread_number( fp ) );
            KEY( "Die_npcfoe2", deity->die_npcfoe[1], fread_number( fp ) );   /* Adjani, 1-24-04 */
            KEY( "Die_npcfoe3", deity->die_npcfoe[2], fread_number( fp ) );
            KEY( "Dig_corpse", deity->dig_corpse, fread_number( fp ) );
            break;

         case 'E':
            if( !str_cmp( word, "End" ) ) /* Adjani 1-31-04 */
            {
               if( filever == 0 )
               {
                  deity->die_npcrace[1] = 0;
                  deity->die_npcrace[2] = 0;
                  deity->die_npcfoe[1] = 0;
                  deity->die_npcfoe[2] = 0;
                  deity->flee_npcrace[1] = 0;
                  deity->flee_npcrace[2] = 0;
                  deity->flee_npcfoe[1] = 0;
                  deity->flee_npcfoe[2] = 0;
                  deity->kill_npcrace[1] = 0;
                  deity->kill_npcrace[2] = 0;
                  deity->kill_npcfoe[1] = 0;
                  deity->kill_npcfoe[2] = 0;
                  deity->npcrace[1] = 0;
                  deity->npcrace[2] = 0;
                  deity->npcfoe[1] = 0;
                  deity->npcfoe[2] = 0;
               }
               log_printf( "Deity: %s loaded", deity->name.c_str(  ) );
               return;
            }

            if( !str_cmp( word, "Elements" ) )
            {
               const char *ln;
               char temp[3][MSL];
               int value;

               ln = fread_line( fp );
               temp[0][0] = '\0';
               temp[1][0] = '\0';
               temp[2][0] = '\0';
               sscanf( ln, "%s %s %s", temp[0], temp[1], temp[2] );

               value = get_risflag( temp[0] );
               if( value < 0 || value >= MAX_RIS_FLAG )
                  bug( "%s: Invalid RISA flag for %s: %s", __FUNCTION__, word, temp[0] );
               else
                  deity->element[0] = value;

               value = get_risflag( temp[1] );
               if( value < 0 || value >= MAX_RIS_FLAG )
                  bug( "%s: Invalid RISA flag for %s: %s", __FUNCTION__, word, temp[1] );
               else
                  deity->element[1] = value;

               value = get_risflag( temp[2] );
               if( value < 0 || value >= MAX_RIS_FLAG )
                  bug( "%s: Invalid RISA flag for %s: %s", __FUNCTION__, word, temp[2] );
               else
                  deity->element[2] = value;
               break;
            }

            if( !str_cmp( word, "Element" ) )
            {
               const char *elem = NULL;
               int value;

               if( filever < 3 )
               {
                  value = fread_number( fp );

                  if( value > 0 )
                  {
                     elem = flag_string( value, ris_flags );

                     value = get_risflag( elem );
                     if( value < 0 || value >= MAX_RIS_FLAG )
                        bug( "%s: Invalid RISA flag for %s: %s", __FUNCTION__, word, elem );
                     else
                        deity->element[0] = value;
                  }
               }
               else
               {
                  elem = fread_flagstring( fp );
                  value = get_risflag( elem );
                  if( value < 0 || value >= MAX_RIS_FLAG )
                     bug( "%s: Invalid RISA flag for %s: %s", __FUNCTION__, word, elem );
                  else
                     deity->element[0] = value;
               }
               break;
            }

            if( !str_cmp( word, "Element2" ) )  /* Added by Tarl 24 Feb 02 */
            {
               const char *elem = NULL;
               int value;

               if( filever < 3 )
               {
                  value = fread_number( fp );

                  if( value > 0 )
                  {
                     elem = flag_string( value, ris_flags );

                     value = get_risflag( elem );
                     if( value < 0 || value >= MAX_RIS_FLAG )
                        bug( "%s: Invalid RISA flag for %s: %s", __FUNCTION__, word, elem );
                     else
                        deity->element[1] = value;
                  }
               }
               else
               {
                  elem = fread_flagstring( fp );
                  value = get_risflag( elem );
                  if( value < 0 || value >= MAX_RIS_FLAG )
                     bug( "%s: Invalid RISA flag for %s: %s", __FUNCTION__, word, elem );
                  else
                     deity->element[1] = value;
               }
               break;
            }

            if( !str_cmp( word, "Element3" ) )  /* Added by Tarl 24 Feb 02 */
            {
               const char *elem = NULL;
               int value;

               if( filever < 3 )
               {
                  value = fread_number( fp );

                  if( value > 0 )
                  {
                     elem = flag_string( value, ris_flags );

                     value = get_risflag( elem );
                     if( value < 0 || value >= MAX_RIS_FLAG )
                        bug( "%s: Invalid RISA flag for %s: %s", __FUNCTION__, word, elem );
                     else
                        deity->element[2] = value;
                  }
               }
               else
               {
                  elem = fread_flagstring( fp );
                  value = get_risflag( elem );
                  if( value < 0 || value >= MAX_RIS_FLAG )
                     bug( "%s: Invalid RISA flag for %s: %s", __FUNCTION__, word, elem );
                  else
                     deity->element[2] = value;
               }
               break;
            }

            if( !str_cmp( word, "Elementnums" ) )
            {
               deity->elementnum[0] = fread_number( fp );
               deity->elementnum[1] = fread_number( fp );   /* Added by Tarl 24 Feb 02 */
               deity->elementnum[2] = fread_number( fp );   /* Added by Tarl 24 Feb 02 */
               break;
            }
            KEY( "Elementnum", deity->elementnum[0], fread_number( fp ) );
            KEY( "Elementnum2", deity->elementnum[1], fread_number( fp ) );   /* Added by Tarl 24 Feb 02 */
            KEY( "Elementnum3", deity->elementnum[2], fread_number( fp ) );   /* Added by Tarl 24 Feb 02 */
            break;

         case 'F':
            STDSKEY( "Filename", deity->filename );
            KEY( "Flee", deity->flee, fread_number( fp ) );
            if( !str_cmp( word, "Flee_npcraces" ) )
            {
               deity->flee_npcrace[0] = fread_number( fp );
               deity->flee_npcrace[1] = fread_number( fp );
               deity->flee_npcrace[2] = fread_number( fp );
               break;
            }
            if( !str_cmp( word, "Flee_npcfoes" ) )
            {
               deity->flee_npcfoe[0] = fread_number( fp );
               deity->flee_npcfoe[1] = fread_number( fp );
               deity->flee_npcfoe[2] = fread_number( fp );
               break;
            }
            KEY( "Flee_npcrace", deity->flee_npcrace[0], fread_number( fp ) );
            KEY( "Flee_npcrace2", deity->flee_npcrace[1], fread_number( fp ) );  /* Adjani, 1-24-04 */
            KEY( "Flee_npcrace3", deity->flee_npcrace[2], fread_number( fp ) );
            KEY( "Flee_npcfoe", deity->flee_npcfoe[0], fread_number( fp ) );
            KEY( "Flee_npcfoe2", deity->flee_npcfoe[1], fread_number( fp ) ); /* Adjani, 1-24-04 */
            KEY( "Flee_npcfoe3", deity->flee_npcfoe[2], fread_number( fp ) );
            break;

         case 'K':
            KEY( "Kill", deity->kill, fread_number( fp ) );
            if( !str_cmp( word, "Kill_npcraces" ) )
            {
               deity->kill_npcrace[0] = fread_number( fp );
               deity->kill_npcrace[1] = fread_number( fp );
               deity->kill_npcrace[2] = fread_number( fp );
               break;
            }
            if( !str_cmp( word, "Kill_npcfoes" ) )
            {
               deity->kill_npcfoe[0] = fread_number( fp );
               deity->kill_npcfoe[1] = fread_number( fp );
               deity->kill_npcfoe[2] = fread_number( fp );
               break;
            }
            KEY( "Kill_npcrace", deity->kill_npcrace[0], fread_number( fp ) );
            KEY( "Kill_npcrace2", deity->kill_npcrace[1], fread_number( fp ) );  /* Adjani, 1-24-04 */
            KEY( "Kill_npcrace3", deity->kill_npcrace[2], fread_number( fp ) );
            KEY( "Kill_npcfoe", deity->kill_npcfoe[0], fread_number( fp ) );
            KEY( "Kill_npcfoe2", deity->kill_npcfoe[1], fread_number( fp ) ); /* Adjani, 1-24-04 */
            KEY( "Kill_npcfoe3", deity->kill_npcfoe[2], fread_number( fp ) );
            KEY( "Kill_magic", deity->kill_magic, fread_number( fp ) );
            break;

         case 'M':
            KEY( "Minion", deity->minion, fread_number( fp ) );   /* Added by Tarl 24 Feb 02 */
            KEY( "Mount", deity->mount, fread_number( fp ) );  /* Added by Tarl 24 Feb 02 */
            break;

         case 'N':
            STDSKEY( "Name", deity->name );
            if( !str_cmp( word, "npcfoe" ) ) /* npcfoe, npcfoe2, npcfoe3, npcrace, npcrace2, npcrace3 - Adjani 1-31-04 */
            {
               int npcfoe;

               if( filever == 0 )
                  npcfoe = fread_number( fp );
               else
                  npcfoe = get_npc_race( fread_flagstring( fp ) );

               if( npcfoe < -1 || npcfoe >= MAX_NPC_RACE )
               {
                  bug( "%s: Deity %s has invalid %s! Defaulting to human.", __FUNCTION__, deity->name.c_str(  ), word );
                  npcfoe = RACE_HUMAN;
               }
               deity->npcfoe[0] = npcfoe;
               break;
            }

            if( !str_cmp( word, "npcfoe2" ) )
            {
               int npcfoe2;

               if( filever == 0 )
                  npcfoe2 = fread_number( fp );
               else
                  npcfoe2 = get_npc_race( fread_flagstring( fp ) );
               if( npcfoe2 < -1 || npcfoe2 >= MAX_NPC_RACE )
               {
                  bug( "%s: Deity %s has invalid %s! Defaulting to human.", __FUNCTION__, deity->name.c_str(  ), word );
                  npcfoe2 = RACE_HUMAN;
               }
               deity->npcfoe[1] = npcfoe2;
               break;
            }

            if( !str_cmp( word, "npcfoe3" ) )
            {
               int npcfoe3;

               if( filever == 0 )
                  npcfoe3 = fread_number( fp );
               else
                  npcfoe3 = get_npc_race( fread_flagstring( fp ) );
               if( npcfoe3 < -1 || npcfoe3 >= MAX_NPC_RACE )
               {
                  bug( "%s: Deity %s has invalid %s! Defaulting to human.", __FUNCTION__, deity->name.c_str(  ), word );
                  npcfoe3 = RACE_HUMAN;
               }
               deity->npcfoe[2] = npcfoe3;
               break;
            }

            if( !str_cmp( word, "npcrace" ) )
            {
               int npcrace;

               if( filever == 0 )
                  npcrace = fread_number( fp );
               else
                  npcrace = get_npc_race( fread_flagstring( fp ) );

               if( npcrace < -1 || npcrace >= MAX_NPC_RACE )
               {
                  bug( "%s: Deity %s has invalid %s! Defaulting to human.", __FUNCTION__, deity->name.c_str(  ), word );
                  npcrace = RACE_HUMAN;
               }
               deity->npcrace[0] = npcrace;
               break;
            }

            if( !str_cmp( word, "npcrace2" ) )
            {
               int npcrace2;

               if( filever == 0 )
                  npcrace2 = fread_number( fp );
               else
                  npcrace2 = get_npc_race( fread_flagstring( fp ) );
               if( npcrace2 < -1 || npcrace2 >= MAX_NPC_RACE )
               {
                  bug( "%s: Deity %s has invalid %s! Defaulting to human.", __FUNCTION__, deity->name.c_str(  ), word );
                  npcrace2 = RACE_HUMAN;
               }
               deity->npcrace[1] = npcrace2;
               break;
            }

            if( !str_cmp( word, "npcrace3" ) )
            {
               int npcrace3;

               if( filever == 0 )
                  npcrace3 = fread_number( fp );
               else
                  npcrace3 = get_npc_race( fread_flagstring( fp ) );
               if( npcrace3 < -1 || npcrace3 >= MAX_NPC_RACE )
               {
                  bug( "%s: Deity %s has invalid %s! Defaulting to human.", __FUNCTION__, deity->name.c_str(  ), word );
                  npcrace3 = RACE_HUMAN;
               }
               deity->npcrace[2] = npcrace3;
               break;
            }
            break;

         case 'O':
            KEY( "Objstat", deity->objstat, fread_number( fp ) );
            break;

         case 'R':
            /*
             * Cleaned up way to handle deity races - Samson 5-17-04 
             */
            if( !str_cmp( word, "Races" ) )
            {
               string race, flag;
               int value;

               race = fread_flagstring( fp );

               if( !str_cmp( race, "all" ) )
                  deity->race_allowed.reset(  );
               else
               {
                  while( !race.empty() )
                  {
                     race = one_argument( race, flag );
                     value = get_pc_race( flag );
                     if( value < 0 || value > MAX_PC_RACE )
                        bug( "Unknown PC Race: %s", flag.c_str() );
                     else
                        deity->race_allowed.set( value );
                  }
               }
               break;
            }
            KEY( "Recallroom", deity->recallroom, fread_number( fp ) );
            break;

         case 'S':
            KEY( "Sac", deity->sac, fread_number( fp ) );
            KEY( "Savatar", deity->savatar, fread_number( fp ) );
            KEY( "Scorpse", deity->scorpse, fread_number( fp ) );
            KEY( "Sdeityobj", deity->sdeityobj, fread_number( fp ) );
            KEY( "Sdeityobj2", deity->sdeityobj2, fread_number( fp ) ); /* Added by Tarl 02 Mar 02 */
            KEY( "Smount", deity->smount, fread_number( fp ) );   /* Added by Tarl 24 Feb 02 */
            KEY( "Sminion", deity->sminion, fread_number( fp ) ); /* Added by Tarl 24 Feb 02 */
            KEY( "Srecall", deity->srecall, fread_number( fp ) );
            if( !str_cmp( word, "Sex" ) ) /* Adjani, 2-18-04 */
            {
               int sex;

               if( filever == 0 )
                  sex = fread_number( fp );
               else
               {
                  sex = get_npc_sex( fread_flagstring( fp ) );
                  if( sex < -1 || sex >= SEX_MAX )
                  {
                     bug( "%s: Deity %s has invalid %s! Defaulting to neuter.", __FUNCTION__, deity->name.c_str(  ), word );
                     sex = SEX_NEUTRAL;
                  }
                  deity->sex = sex;
               }
               break;
            }
            KEY( "Spell_aid", deity->spell_aid, fread_number( fp ) );
            if( !str_cmp( word, "Spells" ) )
            {
               deity->spell[0] = fread_number( fp );  /* Added by Tarl 24 Mar 02 */
               deity->spell[1] = fread_number( fp );  /* Added by Tarl 24 Mar 02 */
               deity->spell[2] = fread_number( fp );  /* Added by Tarl 24 Mar 02 */
               break;
            }

            if( !str_cmp( word, "Sspells" ) )
            {
               deity->sspell[0] = fread_number( fp ); /* Added by Tarl 24 Mar 02 */
               deity->sspell[1] = fread_number( fp ); /* Added by Tarl 24 Mar 02 */
               deity->sspell[2] = fread_number( fp ); /* Added by Tarl 24 Mar 02 */
               break;
            }

            KEY( "Spell1", deity->spell[0], fread_number( fp ) ); /* Added by Tarl 24 Mar 02 */
            KEY( "Spell2", deity->spell[1], fread_number( fp ) ); /* Added by Tarl 24 Mar 02 */
            KEY( "Spell3", deity->spell[2], fread_number( fp ) ); /* Added by Tarl 24 Mar 02 */
            KEY( "Sspell1", deity->sspell[0], fread_number( fp ) );  /* Added by Tarl 24 Mar 02 */
            KEY( "Sspell2", deity->sspell[1], fread_number( fp ) );  /* Added by Tarl 24 Mar 02 */
            KEY( "Sspell3", deity->sspell[2], fread_number( fp ) );  /* Added by Tarl 24 Mar 02 */
            KEY( "Steal", deity->steal, fread_number( fp ) );
            if( !str_cmp( word, "Suscepts" ) )
            {
               const char *ln;
               char temp[3][MSL];
               int value;

               ln = fread_line( fp );
               temp[0][0] = '\0';
               temp[1][0] = '\0';
               temp[2][0] = '\0';
               sscanf( ln, "%s %s %s", temp[0], temp[1], temp[2] );

               value = get_risflag( temp[0] );
               if( value < 0 || value >= MAX_RIS_FLAG )
                  bug( "%s: Invalid RISA flag for %s: %s", __FUNCTION__, word, temp[0] );
               else
                  deity->suscept[0] = value;

               value = get_risflag( temp[1] );
               if( value < 0 || value >= MAX_RIS_FLAG )
                  bug( "%s: Invalid RISA flag for %s: %s", __FUNCTION__, word, temp[1] );
               else
                  deity->suscept[1] = value;

               value = get_risflag( temp[2] );
               if( value < 0 || value >= MAX_RIS_FLAG )
                  bug( "%s: Invalid RISA flag for %s: %s", __FUNCTION__, word, temp[2] );
               else
                  deity->suscept[2] = value;
               break;
            }

            if( !str_cmp( word, "Suscept" ) )
            {
               const char *elem = NULL;
               int value;

               if( filever < 3 )
               {
                  value = fread_number( fp );

                  if( value > 0 )
                  {
                     elem = flag_string( value, ris_flags );

                     value = get_risflag( elem );
                     if( value < 0 || value >= MAX_RIS_FLAG )
                        bug( "%s: Invalid RISA flag for %s: %s", __FUNCTION__, word, elem );
                     else
                        deity->suscept[0] = value;
                  }
               }
               else
               {
                  elem = fread_flagstring( fp );
                  value = get_risflag( elem );
                  if( value < 0 || value >= MAX_RIS_FLAG )
                     bug( "%s: Invalid RISA flag for %s: %s", __FUNCTION__, word, elem );
                  else
                     deity->suscept[0] = value;
               }
               break;
            }

            if( !str_cmp( word, "Suscept2" ) )  /* Added by Tarl 24 Feb 02 */
            {
               const char *elem = NULL;
               int value;

               if( filever < 3 )
               {
                  value = fread_number( fp );

                  if( value > 0 )
                  {
                     elem = flag_string( value, ris_flags );

                     value = get_risflag( elem );
                     if( value < 0 || value >= MAX_RIS_FLAG )
                        bug( "%s: Invalid RISA flag for %s: %s", __FUNCTION__, word, elem );
                     else
                        deity->suscept[1] = value;
                  }
               }
               else
               {
                  elem = fread_flagstring( fp );
                  value = get_risflag( elem );
                  if( value < 0 || value >= MAX_RIS_FLAG )
                     bug( "%s: Invalid RISA flag for %s: %s", __FUNCTION__, word, elem );
                  else
                     deity->suscept[1] = value;
               }
               break;
            }

            if( !str_cmp( word, "Suscept3" ) )  /* Added by Tarl 24 Feb 02 */
            {
               const char *elem = NULL;
               int value;

               if( filever < 3 )
               {
                  value = fread_number( fp );

                  if( value > 0 )
                  {
                     elem = flag_string( value, ris_flags );

                     value = get_risflag( elem );
                     if( value < 0 || value >= MAX_RIS_FLAG )
                        bug( "%s: Invalid RISA flag for %s: %s", __FUNCTION__, word, elem );
                     else
                        deity->suscept[2] = value;
                  }
               }
               else
               {
                  elem = fread_flagstring( fp );
                  value = get_risflag( elem );
                  if( value < 0 || value >= MAX_RIS_FLAG )
                     bug( "%s: Invalid RISA flag for %s: %s", __FUNCTION__, word, elem );
                  else
                     deity->suscept[2] = value;
               }
               break;
            }

            if( !str_cmp( word, "Susceptnums" ) )
            {
               deity->susceptnum[0] = fread_number( fp );
               deity->susceptnum[1] = fread_number( fp );   /* Added by Tarl 24 Feb 02 */
               deity->susceptnum[2] = fread_number( fp );   /* Added by Tarl 24 Feb 02 */
               break;
            }
            KEY( "Susceptnum", deity->susceptnum[0], fread_number( fp ) );
            KEY( "Susceptnum2", deity->susceptnum[1], fread_number( fp ) );   /* Added by Tarl 24 Feb 02 */
            KEY( "Susceptnum3", deity->susceptnum[2], fread_number( fp ) );   /* Added by Tarl 24 Feb 02 */
            break;

         case 'W':
            KEY( "Worshippers", deity->worshippers, fread_number( fp ) );
            break;
      }
   }
}

/* Load a deity file */
bool load_deity_file( const char *deityfile )
{
   char filename[256];
   deity_data *deity;
   FILE *fp;
   bool found;
   int filever = 0;

   found = false;
   snprintf( filename, 256, "%s%s", DEITY_DIR, deityfile );

   if( ( fp = fopen( filename, "r" ) ) != NULL )
   {
      for( ;; )
      {
         char letter;
         char *word;

         letter = fread_letter( fp );
         if( letter == '*' )
         {
            fread_to_eol( fp );
            continue;
         }

         if( letter != '#' )
         {
            bug( "%s: # not found.", __FUNCTION__ );
            break;
         }

         word = fread_word( fp );
         if( !str_cmp( word, "VERSION" ) )   /* Adjani, 1-31-04 */
         {
            filever = fread_number( fp );
            letter = fread_letter( fp );
            if( letter != '#' )
            {
               bug( "%s: # not found after reading file version %d.", __FUNCTION__, filever );
               break;
            }
            word = fread_word( fp );
         }
         if( !str_cmp( word, "DEITY" ) )
         {
            deity = new deity_data;
            fread_deity( deity, fp, filever );  /* Filever added for file versions. Whee! Adjani, 1-31-04 */
            deitylist.push_back( deity );
            found = true;
            break;
         }
         else
         {
            bug( "%s: bad section: %s.", __FUNCTION__, word );
            break;
         }
      }
      FCLOSE( fp );
   }
   return found;
}

/* Load in all the deity files */
void load_deity( void )
{
   FILE *fpList;
   const char *filename;
   char deitylistfile[256];

   deitylist.clear(  );

   log_string( "Loading deities..." );

   snprintf( deitylistfile, 256, "%s%s", DEITY_DIR, DEITY_LIST );
   if( !( fpList = fopen( deitylistfile, "r" ) ) )
   {
      perror( deitylistfile );
      exit( 1 );
   }

   for( ;; )
   {
      filename = ( feof( fpList ) ? "$" : fread_word( fpList ) );

      if( filename[0] == '\0' )
      {
         bug( "%s: EOF encountered reading file!", __FUNCTION__ );
         break;
      }

      if( filename[0] == '$' )
         break;
      if( !load_deity_file( filename ) )
         bug( "%s: Cannot load deity file: %s", __FUNCTION__, filename );
   }
   FCLOSE( fpList );
   log_string( "Done deities" );
}

CMDF( do_setdeity )
{
   string arg1, arg2, arg3;
   deity_data *deity;
   int value;

   if( ch->isnpc(  ) )
   {
      ch->print( "Huh?\r\n" );
      return;
   }

   switch ( ch->substate )
   {
      default:
         break;

      case SUB_RESTRICTED:
         ch->print( "You cannot do this while in another command.\r\n" );
         return;

      case SUB_DEITYDESC:
         deity = ( deity_data * ) ch->pcdata->dest_buf;
         deity->deitydesc = ch->copy_buffer(  );
         ch->stop_editing(  );
         save_deity( deity );
         ch->substate = ch->tempnum;
         return;

      case SUB_EDIT_ABORT:
         ch->substate = SUB_NONE;
         ch->print( "Aborting description.\r\n" );
         return;
   }

   smash_tilde( argument );
   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );

   if( arg1.empty(  ) )
   {
      ch->print( "Usage: setdeity <deity> <field> <toggle>\r\n" );
      ch->print( "\r\nField being one of:\r\n" );
      ch->print( "filename name description alignment worshippers minion sex race\r\n" );
      ch->print( "class npcfoe npcfoe2 npcfoe3 npcrace npcrace2 npcrace3\r\n" );
      ch->print( "element element2 element3 elementnum elementnum2\r\n" );
      ch->print( "elementnum3 affectednum affectednum2 affectednum3 affected affected2\r\n" );
      ch->print( "affected3 susceptnum susceptnum2 susceptnum3 suscept suscept2 suscept3\r\n" );
      ch->print( "recallroom avatar mount object object2 delete\r\n" );
      ch->print( "\r\nFavor adjustments:\r\n" );
      ch->print( "flee flee_npcrace flee_npcrace2 flee_npcrace3 flee_npcfoe flee_npcfoe2\r\n" );
      ch->print( "flee_npcfoe3 kill kill_npcrace kill_npcrace2 kill_npcrace3 kill_npcfoe\r\n" );
      ch->print( "kill_npcfoe2 kill_npcfoe3 kill_magic die die_npcrace die_npcrace2\r\n" );
      ch->print( "die_npcrace3 die_npcfoe die_npcfoe2 die_npcfoe3 dig_corpse bury_corpse\r\n" );
      ch->print( "steal backstab aid aid_spell spell_aid sac\r\n" );
      ch->print( "\r\nFavor requirements for supplicate:\r\n" );
      ch->print( "scorpse savatar smount sdeityobj sdeityobj2 srecall spell1 spell2\r\n" );
      ch->print( "spell3 sspell1 sspell2 sspell3\r\n" ); /* Added by Tarl 24 Mar 02 */
      ch->print( "Objstat - being one of:\r\n" );
      ch->print( "str int wis con dex cha lck\r\n" );
      ch->print( " 0 - 1 - 2 - 3 - 4 - 5 - 6\r\n" );  /* Entire list tinkered with by Adjani, 1-27-04 */
      return;
   }

   if( !( deity = get_deity( arg1 ) ) )
   {
      ch->print( "No such deity.\r\n" );
      return;
   }

   if( !str_cmp( arg2, "delete" ) )
   {
      char filename[256];

      list < char_data * >::iterator ich;
      for( ich = pclist.begin(  ); ich != pclist.end(  ); ++ich )
      {
         char_data *vch = *ich;

         if( vch->pcdata->deity == deity )
         {
            vch->printf( "&RYour deity, %s, has met its demise!\r\n", vch->pcdata->deity_name.c_str(  ) );

            vch->unset_aflag( ch->pcdata->deity->affected[0] );
            vch->unset_aflag( ch->pcdata->deity->affected[1] );
            vch->unset_aflag( ch->pcdata->deity->affected[2] );
            vch->unset_resist( ch->pcdata->deity->element[0] );
            vch->unset_resist( ch->pcdata->deity->element[1] );
            vch->unset_resist( ch->pcdata->deity->element[2] );
            vch->unset_suscep( ch->pcdata->deity->suscept[0] );
            vch->unset_suscep( ch->pcdata->deity->suscept[1] );
            vch->unset_suscep( ch->pcdata->deity->suscept[2] );
            vch->pcdata->deity = NULL;
            vch->pcdata->deity_name.clear(  );
            vch->update_aris(  );
            vch->save(  );
         }
      }

      snprintf( filename, 256, "%s%s", DEITY_DIR, deity->filename.c_str(  ) );
      deleteptr( deity );
      ch->printf( "&YDeity information for %s deleted.\r\n", arg1.c_str(  ) );

      if( !remove( filename ) )
         ch->printf( "&RDeity file for %s destroyed.\r\n", arg1.c_str(  ) );

      write_deity_list(  );
      return;
   }

   if( !str_cmp( arg2, "name" ) )
   {
      deity_data *udeity;

      if( argument.empty(  ) )
      {
         ch->print( "You can't set a deity's name to nothing.\r\n" );
         return;
      }
      if( ( udeity = get_deity( argument ) ) )
      {
         ch->print( "There is already another deity with that name.\r\n" );
         return;
      }
      deity->name = argument;
      ch->print( "Done.\r\n" );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "avatar" ) )
   {
      deity->avatar = atoi( argument.c_str(  ) );
      ch->print( "Done.\r\n" );
      save_deity( deity );
      return;
   }

   /*
    * Added by Tarl 24 Feb 02 
    */
   if( !str_cmp( arg2, "mount" ) )
   {
      deity->mount = atoi( argument.c_str(  ) );
      ch->print( "Done.\r\n" );
      save_deity( deity );
      return;
   }

   /*
    * Added by Tarl 24 Feb 02 
    */
   if( !str_cmp( arg2, "minion" ) )
   {
      deity->minion = atoi( argument.c_str(  ) );
      ch->print( "Done.\r\n" );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "object" ) )
   {
      deity->deityobj = atoi( argument.c_str(  ) );
      ch->print( "Done.\r\n" );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "object2" ) )
   {
      deity->deityobj2 = atoi( argument.c_str(  ) );
      ch->print( "Done.\r\n" );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "recallroom" ) )
   {
      deity->recallroom = atoi( argument.c_str(  ) );
      ch->print( "Done.\r\n" );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "filename" ) )
   {
      char filename[256];

      if( !is_valid_filename( ch, DEITY_DIR, argument ) )
         return;

      snprintf( filename, 256, "%s%s", DEITY_DIR, deity->filename.c_str(  ) );
      if( !remove( filename ) )
         ch->print( "Old deity file deleted.\r\n" );
      deity->filename = argument;
      ch->print( "Done.\r\n" );
      save_deity( deity );
      write_deity_list(  );
      return;
   }

   if( !str_cmp( arg2, "description" ) )
   {
      if( ch->substate == SUB_REPEATCMD )
         ch->tempnum = SUB_REPEATCMD;
      else
         ch->tempnum = SUB_NONE;
      ch->substate = SUB_DEITYDESC;
      ch->pcdata->dest_buf = deity;
      ch->editor_desc_printf( "Deity description for deity '%s'.", deity->name.c_str(  ) );
      ch->start_editing( deity->deitydesc );
      return;
   }

   if( !str_cmp( arg2, "alignment" ) )
   {
      deity->alignment = atoi( argument.c_str(  ) );
      ch->print( "Done.\r\n" );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "flee" ) )
   {
      deity->flee = atoi( argument.c_str(  ) );
      ch->print( "Done.\r\n" );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "flee_npcrace" ) )
   {
      deity->flee_npcrace[0] = atoi( argument.c_str(  ) );
      ch->print( "Done.\r\n" );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "flee_npcrace2" ) )   /* flee_npcrace2, flee_npcrace3, flee_npcfoe2, flee_npcfoe3 - Adjani, 1-27-04 */
   {
      deity->flee_npcrace[1] = atoi( argument.c_str(  ) );
      ch->print( "Done.\r\n" );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "flee_npcrace3" ) )
   {
      deity->flee_npcrace[2] = atoi( argument.c_str(  ) );
      ch->print( "Done.\r\n" );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "flee_npcfoe" ) )
   {
      deity->flee_npcfoe[0] = atoi( argument.c_str(  ) );
      ch->print( "Done.\r\n" );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "flee_npcfoe2" ) )
   {
      deity->flee_npcfoe[1] = atoi( argument.c_str(  ) );
      ch->print( "Done.\r\n" );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "flee_npcfoe3" ) )
   {
      deity->flee_npcfoe[2] = atoi( argument.c_str(  ) );
      ch->print( "Done.\r\n" );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "kill" ) )
   {
      deity->kill = atoi( argument.c_str(  ) );
      ch->print( "Done.\r\n" );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "kill_npcrace" ) )
   {
      deity->kill_npcrace[0] = atoi( argument.c_str(  ) );
      ch->print( "Done.\r\n" );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "kill_npcrace2" ) )   /* kill_npcrace2, kill_npcrace3, kill_npcfoe2, kill_npcfoe3 - Adjani, 1-24-04 */
   {
      deity->kill_npcrace[1] = atoi( argument.c_str(  ) );
      ch->print( "Done.\r\n" );
      save_deity( deity );
      return;
   }
   if( !str_cmp( arg2, "kill_npcrace3" ) )
   {
      deity->kill_npcrace[2] = atoi( argument.c_str(  ) );
      ch->print( "Done.\r\n" );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "kill_npcfoe" ) )
   {
      deity->kill_npcfoe[0] = atoi( argument.c_str(  ) );
      ch->print( "Done.\r\n" );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "kill_npcfoe2" ) )
   {
      deity->kill_npcfoe[1] = atoi( argument.c_str(  ) );
      ch->print( "Done.\r\n" );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "kill_npcfoe3" ) )
   {
      deity->kill_npcfoe[2] = atoi( argument.c_str(  ) );
      ch->print( "Done.\r\n" );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "kill_magic" ) )
   {
      deity->kill_magic = atoi( argument.c_str(  ) );
      ch->print( "Done.\r\n" );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "sac" ) )
   {
      deity->sac = atoi( argument.c_str(  ) );
      ch->print( "Done.\r\n" );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "bury_corpse" ) )
   {
      deity->bury_corpse = atoi( argument.c_str(  ) );
      ch->print( "Done.\r\n" );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "aid_spell" ) )
   {
      deity->aid_spell = atoi( argument.c_str(  ) );
      ch->print( "Done.\r\n" );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "aid" ) )
   {
      deity->aid = atoi( argument.c_str(  ) );
      ch->print( "Done.\r\n" );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "steal" ) )
   {
      deity->steal = atoi( argument.c_str(  ) );
      ch->print( "Done.\r\n" );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "backstab" ) )
   {
      deity->backstab = atoi( argument.c_str(  ) );
      ch->print( "Done.\r\n" );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "die" ) )
   {
      deity->die = atoi( argument.c_str(  ) );
      ch->print( "Done.\r\n" );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "die_npcrace" ) )
   {
      deity->die_npcrace[0] = atoi( argument.c_str(  ) );
      ch->print( "Done.\r\n" );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "die_npcrace2" ) ) /* die_npcrace2, die_npcrace3, die_npcfoe2, die_npcfoe3 - Adjani, 1-24-04 */
   {
      deity->die_npcrace[1] = atoi( argument.c_str(  ) );
      ch->print( "Done.\r\n" );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "die_npcrace3" ) )
   {
      deity->die_npcrace[2] = atoi( argument.c_str(  ) );
      ch->print( "Done.\r\n" );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "die_npcfoe" ) )
   {
      deity->die_npcfoe[0] = atoi( argument.c_str(  ) );
      ch->print( "Done.\r\n" );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "die_npcfoe2" ) )
   {
      deity->die_npcfoe[1] = atoi( argument.c_str(  ) );
      ch->print( "Done.\r\n" );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "die_npcfoe3" ) )
   {
      deity->die_npcfoe[2] = atoi( argument.c_str(  ) );
      ch->print( "Done.\r\n" );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "spell_aid" ) )
   {
      deity->spell_aid = atoi( argument.c_str(  ) );
      ch->print( "Done.\r\n" );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "dig_corpse" ) )
   {
      deity->dig_corpse = atoi( argument.c_str(  ) );
      ch->print( "Done.\r\n" );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "scorpse" ) )
   {
      deity->scorpse = atoi( argument.c_str(  ) );
      ch->print( "Done.\r\n" );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "savatar" ) )
   {
      deity->savatar = atoi( argument.c_str(  ) );
      ch->print( "Done.\r\n" );
      save_deity( deity );
      return;
   }

   /*
    * Added by Tarl 24 Feb 02 
    */
   if( !str_cmp( arg2, "smount" ) )
   {
      deity->smount = atoi( argument.c_str(  ) );
      ch->print( "Done.\r\n" );
      save_deity( deity );
      return;
   }

   /*
    * Added by Tarl 24 Feb 02 
    */
   if( !str_cmp( arg2, "sminion" ) )
   {
      deity->sminion = atoi( argument.c_str(  ) );
      ch->print( "Done.\r\n" );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "sdeityobj" ) )
   {
      deity->sdeityobj = atoi( argument.c_str(  ) );
      ch->print( "Done.\r\n" );
      save_deity( deity );
      return;
   }

   /*
    * Added by Tarl 24 Feb 02 
    */
   if( !str_cmp( arg2, "sdeityobj2" ) )
   {
      deity->sdeityobj2 = atoi( argument.c_str(  ) );
      ch->print( "Done.\r\n" );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "objstat" ) )
   {
      deity->objstat = atoi( argument.c_str(  ) );
      ch->print( "Done.\r\n" );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "srecall" ) )
   {
      deity->srecall = atoi( argument.c_str(  ) );
      ch->print( "Done.\r\n" );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "worshippers" ) )
   {
      deity->worshippers = atoi( argument.c_str(  ) );
      ch->print( "Done.\r\n" );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "race" ) )
   {
      int i;

      if( !str_cmp( argument, "all" ) )
      {
         deity->race_allowed.reset(  );
         save_deity( deity );
         ch->print( "Deity now allows all races.\r\n" );
         return;
      }
      for( i = 0; i < MAX_PC_RACE; ++i )
      {
         if( !str_cmp( argument, race_table[i]->race_name ) )
         {
            deity->race_allowed.flip( i );   /* k, that's boggling */
            save_deity( deity );
            ch->print( "Races set.\r\n" );
            return;
         }
      }
      ch->print( "No such race.\r\n" );
      return;
   }

   if( !str_cmp( arg2, "npcrace" ) )   /* npcrace, npcrace2, npcrace3 - Adjani, 2-18-04 */
   {
      bool fMatch = false;

      argument = one_argument( argument, arg3 );
      if( !str_cmp( arg3, "none" ) )
      {
         fMatch = true;
         deity->npcrace[0] = -1;
      }
      else
      {
         value = get_npc_race( arg3 );
         if( value < 0 || value > MAX_NPC_RACE )
            ch->printf( "Unknown npcrace: %s\r\n", arg3.c_str(  ) );
         else
         {
            deity->npcrace[0] = value;
            fMatch = true;
         }
      }
      if( fMatch )
         ch->print( "Done.\r\n" );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "npcrace2" ) )
   {
      bool fMatch = false;

      argument = one_argument( argument, arg3 );
      if( !str_cmp( arg3, "none" ) )
      {
         fMatch = true;
         deity->npcrace[1] = -1;
      }
      else
      {
         value = get_npc_race( arg3 );
         if( value < 0 || value > MAX_NPC_RACE )
            ch->printf( "Unknown npcrace2: %s\r\n", arg3.c_str(  ) );
         else
         {
            deity->npcrace[1] = value;
            fMatch = true;
         }
      }
      if( fMatch )
         ch->print( "Done.\r\n" );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "npcrace3" ) )
   {
      bool fMatch = false;

      argument = one_argument( argument, arg3 );
      if( !str_cmp( arg3, "none" ) )
      {
         fMatch = true;
         deity->npcrace[2] = -1;
      }
      else
      {
         value = get_npc_race( arg3 );
         if( value < 0 || value > MAX_NPC_RACE )
            ch->printf( "Unknown npcrace3: %s\r\n", arg3.c_str(  ) );
         else
         {
            deity->npcrace[2] = value;
            fMatch = true;
         }
      }
      if( fMatch )
         ch->print( "Done.\r\n" );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "npcfoe" ) ) /* npcfoe, npcfoe2, npcfoe3 - Adjani, 2-18-04 */
   {
      bool fMatch = false;

      argument = one_argument( argument, arg3 );
      if( !str_cmp( arg3, "none" ) )
      {
         fMatch = true;
         deity->npcfoe[0] = -1;
      }
      else
      {
         value = get_npc_race( arg3 );
         if( value < 0 || value > MAX_NPC_RACE )
            ch->printf( "Unknown npcfoe: %s\r\n", arg3.c_str(  ) );
         else
         {
            deity->npcfoe[0] = value;
            fMatch = true;
         }
      }
      if( fMatch )
         ch->print( "Done.\r\n" );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "npcfoe2" ) )
   {
      bool fMatch = false;

      argument = one_argument( argument, arg3 );
      if( !str_cmp( arg3, "none" ) )
      {
         fMatch = true;
         deity->npcfoe[1] = -1;
      }
      else
      {
         value = get_npc_race( arg3 );
         if( value < 0 || value > MAX_NPC_RACE )
            ch->printf( "Unknown npcfoe2: %s\r\n", arg3.c_str(  ) );
         else
         {
            deity->npcfoe[1] = value;
            fMatch = true;
         }
      }
      if( fMatch )
         ch->print( "Done.\r\n" );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "npcfoe3" ) )
   {
      bool fMatch = false;

      argument = one_argument( argument, arg3 );
      if( !str_cmp( arg3, "none" ) )
      {
         fMatch = true;
         deity->npcfoe[2] = -1;
      }
      else
      {
         value = get_npc_race( arg3 );
         if( value < 0 || value > MAX_NPC_RACE )
            ch->printf( "Unknown npcfoe3: %s\r\n", arg3.c_str(  ) );
         else
         {
            deity->npcfoe[2] = value;
            fMatch = true;
         }
      }
      if( fMatch )
         ch->print( "Done.\r\n" );
      save_deity( deity );
      return;
   }

   /*
    * Samson redid this section to enable deities to mix and match as many classes as they see fit - 5-17-04 
    */
   if( !str_cmp( arg2, "class" ) )
   {
      int i;

      if( !str_cmp( argument, "all" ) )
      {
         deity->class_allowed.reset(  );
         save_deity( deity );
         ch->print( "Deity now allows all classes.\r\n" );
         return;
      }
      for( i = 0; i < MAX_PC_CLASS; ++i )
      {
         if( !str_cmp( argument, class_table[i]->who_name ) )
         {
            deity->class_allowed.flip( i );  /* k, that's boggling */
            save_deity( deity );
            ch->print( "Classes set.\r\n" );
            return;
         }
      }
      ch->print( "No such class.\r\n" );
      return;
   }

   if( !str_cmp( arg2, "susceptnum" ) )
   {
      deity->susceptnum[0] = atoi( argument.c_str(  ) );
      ch->print( "Done.\r\n" );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "susceptnum2" ) )  /* Added by Tarl 24 Feb 02 */
   {
      deity->susceptnum[1] = atoi( argument.c_str(  ) );
      ch->print( "Done.\r\n" );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "susceptnum3" ) )  /* Added by Tarl 24 Feb 02 */
   {
      deity->susceptnum[2] = atoi( argument.c_str(  ) );
      ch->print( "Done.\r\n" );
      save_deity( deity );
      return;
   }
   if( !str_cmp( arg2, "elementnum" ) )
   {
      deity->elementnum[0] = atoi( argument.c_str(  ) );
      ch->print( "Done.\r\n" );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "elementnum2" ) )  /* Added by Tarl 24 Feb 02 */
   {
      deity->elementnum[1] = atoi( argument.c_str(  ) );
      ch->print( "Done.\r\n" );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "elementnum3" ) )  /* Added by Tarl 24 Feb 02 */
   {
      deity->elementnum[2] = atoi( argument.c_str(  ) );
      ch->print( "Done.\r\n" );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "affectednum" ) )
   {
      deity->affectednum[0] = atoi( argument.c_str(  ) );
      ch->print( "Done.\r\n" );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "affectednum2" ) ) /* Added by Tarl 24 Feb 02 */
   {
      deity->affectednum[1] = atoi( argument.c_str(  ) );
      ch->print( "Done.\r\n" );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "affectednum3" ) ) /* Added by Tarl 24 Feb 02 */
   {
      deity->affectednum[2] = atoi( argument.c_str(  ) );
      ch->print( "Done.\r\n" );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "spell1" ) ) /* Added by Tarl 24 Mar 02 */
   {
      if( skill_lookup( argument ) < 0 )
      {
         ch->print( "No skill/spell by that name.\r\n" );
         return;
      }
      deity->spell[0] = skill_lookup( argument );
      ch->print( "Done.\r\n" );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "spell2" ) ) /* Added by Tarl 24 Mar 02 */
   {
      if( skill_lookup( argument ) < 0 )
      {
         ch->print( "No skill/spell by that name.\r\n" );
         return;
      }
      deity->spell[1] = skill_lookup( argument );
      ch->print( "Done.\r\n" );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "spell3" ) ) /* Added by Tarl 24 Mar 02 */
   {
      if( skill_lookup( argument ) < 0 )
      {
         ch->print( "No skill/spell by that name.\r\n" );
         return;
      }
      deity->spell[2] = skill_lookup( argument );
      ch->print( "Done.\r\n" );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "Sspell1" ) )   /* Added by Tarl 24 Mar 02 */
   {
      deity->sspell[0] = atoi( argument.c_str(  ) );
      ch->print( "Done.\r\n" );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "Sspell2" ) )   /* Added by Tarl 24 Mar 02 */
   {
      deity->sspell[1] = atoi( argument.c_str(  ) );
      ch->print( "Done.\r\n" );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "Sspell3" ) )   /* Added by Tarl 24 Mar 02 */
   {
      deity->sspell[2] = atoi( argument.c_str(  ) );
      ch->print( "Done.\r\n" );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "suscept" ) )   /* suscept, suscept2, suscept3 - Adjani, 2-18-04 */
   {
      bool fMatch = false;

      argument = one_argument( argument, arg3 );

      value = get_risflag( arg3 );
      if( value < 0 || value >= MAX_RIS_FLAG )
         ch->printf( "Unknown flag: %s\r\n", arg3.c_str(  ) );
      else
      {
         deity->suscept[0] = value;
         fMatch = true;
      }
      if( fMatch )
      {
         ch->print( "Done.\r\n" );
         save_deity( deity );
      }
      return;
   }

   if( !str_cmp( arg2, "suscept2" ) )  /* Added by Tarl 24 Feb 02 */
   {
      bool fMatch = false;

      argument = one_argument( argument, arg3 );

      value = get_risflag( arg3 );
      if( value < 0 || value >= MAX_RIS_FLAG )
         ch->printf( "Unknown flag: %s\r\n", arg3.c_str(  ) );
      else
      {
         deity->suscept[1] = value;
         fMatch = true;
      }
      if( fMatch )
      {
         ch->print( "Done.\r\n" );
         save_deity( deity );
      }
      return;
   }

   if( !str_cmp( arg2, "suscept3" ) )  /* Added by Tarl 24 Feb 02 */
   {
      bool fMatch = false;

      argument = one_argument( argument, arg3 );

      value = get_risflag( arg3 );
      if( value < 0 || value >= MAX_RIS_FLAG )
         ch->printf( "Unknown flag: %s\r\n", arg3.c_str(  ) );
      else
      {
         deity->suscept[2] = value;
         fMatch = true;
      }
      if( fMatch )
      {
         ch->print( "Done.\r\n" );
         save_deity( deity );
      }
      return;
   }

   if( !str_cmp( arg2, "element" ) )   /* element, element2, element3 - Adjani, 2-18-04 */
   {
      bool fMatch = false;

      argument = one_argument( argument, arg3 );

      value = get_risflag( arg3 );
      if( value < 0 || value >= MAX_RIS_FLAG )
         ch->printf( "Unknown flag: %s\r\n", arg3.c_str(  ) );
      else
      {
         deity->element[0] = value;
         fMatch = true;
      }
      if( fMatch )
      {
         ch->print( "Done.\r\n" );
         save_deity( deity );
      }
      return;
   }

   if( !str_cmp( arg2, "element2" ) )  /* Added by Tarl 24 Feb 02 */
   {
      bool fMatch = false;

      argument = one_argument( argument, arg3 );

      value = get_risflag( arg3 );
      if( value < 0 || value >= MAX_RIS_FLAG )
         ch->printf( "Unknown flag: %s\r\n", arg3.c_str(  ) );
      else
      {
         deity->element[1] = value;
         fMatch = true;
      }
      if( fMatch )
      {
         ch->print( "Done.\r\n" );
         save_deity( deity );
      }
      return;
   }

   if( !str_cmp( arg2, "element3" ) )  /* Added by Tarl 24 Feb 02 */
   {
      bool fMatch = false;

      argument = one_argument( argument, arg3 );

      value = get_risflag( arg3 );
      if( value < 0 || value >= MAX_RIS_FLAG )
         ch->printf( "Unknown flag: %s\r\n", arg3.c_str(  ) );
      else
      {
         deity->element[2] = value;
         fMatch = true;
      }
      if( fMatch )
      {
         ch->print( "Done.\r\n" );
         save_deity( deity );
      }
      return;
   }

   if( !str_cmp( arg2, "sex" ) || !str_cmp( arg2, "gender" ) ) /* Adjani, 2-18-04 */
   {
      bool fMatch = false;

      argument = one_argument( argument, arg3 );
      if( !str_cmp( arg3, "none" ) )
      {
         fMatch = true;
         deity->sex = -1;
      }
      else
      {
         value = get_npc_sex( arg3 );
         if( value < 0 || value > SEX_MAX )
            ch->printf( "Unknown gender: %s\r\n", arg3.c_str(  ) );
         else
         {
            deity->sex = value;
            fMatch = true;
         }
      }
      if( fMatch )
         ch->print( "Done.\r\n" );
      save_deity( deity );
      return;
   }

   if( !str_cmp( arg2, "affected" ) )  /* affected, affected2, affected3 - Adjani, 2-18-04 */
   {
      value = get_aflag( argument );
      if( value < 0 || value >= MAX_AFFECTED_BY )
         ch->printf( "Unknown flag: %s\r\n", argument.c_str(  ) );
      else
      {
         deity->affected[0] = value;
         save_deity( deity );
         ch->printf( "Affected '%s' set.\r\n", argument.c_str(  ) );
      }
      return;
   }

   if( !str_cmp( arg2, "affected2" ) )
   {
      value = get_aflag( argument );
      if( value < 0 || value >= MAX_AFFECTED_BY )
         ch->printf( "Unknown flag: %s\r\n", argument.c_str(  ) );
      else
      {
         deity->affected[1] = value;
         save_deity( deity );
         ch->printf( "Affected2 '%s' set.\r\n", argument.c_str(  ) );
      }
      return;
   }

   if( !str_cmp( arg2, "affected3" ) )
   {
      value = get_aflag( argument );
      if( value < 0 || value >= MAX_AFFECTED_BY )
         ch->printf( "Unknown flag: %s\r\n", argument.c_str(  ) );
      else
      {
         deity->affected[2] = value;
         save_deity( deity );
         ch->printf( "Affected3 '%s' set.\r\n", argument.c_str(  ) );
      }
      return;
   }
}

/* Regrouped by Adjani 12-03-2002 and again on 1-27-04 and once more on 1-31-04. */
CMDF( do_showdeity )
{
   deity_data *deity;

   if( ch->isnpc(  ) )
   {
      ch->print( "Huh?\r\n" );
      return;
   }
   if( argument.empty(  ) )
   {
      ch->print( "Usage: showdeity <deity>\r\n" );
      return;
   }
   if( !( deity = get_deity( argument ) ) )
   {
      ch->print( "No such deity.\r\n" );
      return;
   }
   ch->printf( "\r\nDeity: %-11s Filename: %s \r\n", deity->name.c_str(  ), deity->filename.c_str(  ) );
   ch->printf( "Description:\r\n %s \r\n", deity->deitydesc.c_str(  ) );
   ch->printf( "Alignment:       %-14d  Sex:   %-14s \r\n", deity->alignment, deity->sex == -1 ? "none" : npc_sex[deity->sex] );
   ch->printf( "Races Allowed:   %s\r\n", deity->race_allowed.none(  )? "All" : bitset_string( deity->race_allowed, npc_race ) );
   ch->printf( "Classes Allowed: %s\r\n", deity->class_allowed.none(  )? "All" : bitset_string( deity->class_allowed, npc_class ) );

   ch->printf( "Npcraces:        %-14s", ( deity->npcrace[0] < 0 || deity->npcrace[0] > MAX_NPC_RACE ) ? "none" : npc_race[deity->npcrace[0]] );
   ch->printf( "  %-14s", ( deity->npcrace[1] < 0 || deity->npcrace[1] > MAX_NPC_RACE ) ? "none" : npc_race[deity->npcrace[1]] );
   ch->printf( "  %-14s\r\n", ( deity->npcrace[2] < 0 || deity->npcrace[2] > MAX_NPC_RACE ) ? "none" : npc_race[deity->npcrace[2]] );

   ch->printf( "Npcfoes:         %-14s", ( deity->npcfoe[0] < 0 || deity->npcfoe[0] > MAX_NPC_RACE ) ? "none" : npc_race[deity->npcfoe[0]] );
   ch->printf( "  %-14s", ( deity->npcfoe[1] < 0 || deity->npcfoe[1] > MAX_NPC_RACE ) ? "none" : npc_race[deity->npcfoe[1]] );
   ch->printf( "  %-14s\r\n", ( deity->npcfoe[2] < 0 || deity->npcfoe[2] > MAX_NPC_RACE ) ? "none" : npc_race[deity->npcfoe[2]] );

   ch->printf( "Affects:         %-14s", aff_flags[deity->affected[0]] );
   ch->printf( "  %-14s", aff_flags[deity->affected[1]] );
   ch->printf( "  %-14s\r\n", aff_flags[deity->affected[2]] );

   ch->printf( "Elements:      %-14s", ris_flags[deity->element[0]] );
   ch->printf( "  %-14s", ris_flags[deity->element[1]] );
   ch->printf( "  %-14s\r\n", ris_flags[deity->element[2]] );

   ch->printf( "Suscepts:      %-14s", ris_flags[deity->suscept[0]] );
   ch->printf( "  %-14s", ris_flags[deity->suscept[1]] );
   ch->printf( "  %-14s\r\n", ris_flags[deity->suscept[2]] );

   ch->printf( "Spells:        %-14s  %-14s  %-14s\r\n\r\n", skill_table[deity->spell[0]]->name, skill_table[deity->spell[1]]->name, skill_table[deity->spell[2]]->name );
   ch->printf( "Affectednums:  %-5d %-5d %-7d ", deity->affectednum[0], deity->affectednum[1], deity->affectednum[2] );
   ch->printf( "Elementnums:   %-5d %-5d %-5d\r\n", deity->elementnum[0], deity->elementnum[1], deity->elementnum[2] );
   ch->printf( "Susceptnums:   %-5d %-5d %-7d ", deity->susceptnum[0], deity->susceptnum[1], deity->susceptnum[2] );
   ch->printf( "Sspells:       %-5d %-5d %-5d\r\n", deity->sspell[0], deity->sspell[1], deity->sspell[2] );
   ch->printf( "Flee_npcraces: %-5d %-5d %-7d ", deity->flee_npcrace[0], deity->flee_npcrace[1], deity->flee_npcrace[2] );
   ch->printf( "Flee_npcfoes:  %-5d %-5d %-5d \r\n", deity->flee_npcfoe[0], deity->flee_npcfoe[1], deity->flee_npcfoe[2] );
   ch->printf( "Kill_npcraces: %-5d %-5d %-7d ", deity->kill_npcrace[0], deity->kill_npcrace[1], deity->kill_npcrace[2] );
   ch->printf( "Kill_npcfoes:  %-5d %-5d %-5d \r\n", deity->kill_npcfoe[0], deity->kill_npcfoe[1], deity->kill_npcfoe[2] );
   ch->printf( "Die_npcraces:  %-5d %-5d %-7d ", deity->die_npcrace[0], deity->die_npcrace[1], deity->die_npcrace[2] );
   ch->printf( "Die_npcfoes:   %-5d %-5d %-5d \r\n\r\n", deity->die_npcfoe[0], deity->die_npcfoe[1], deity->die_npcfoe[2] );
   ch->printf( "Kill_magic: %-10d Sac:        %-10d Bury_corpse: %-10d \r\n", deity->kill_magic, deity->sac, deity->bury_corpse );
   ch->printf( "Dig_corpse: %-10d Flee:       %-10d Kill:        %-10d \r\n", deity->flee, deity->kill, deity->dig_corpse );
   ch->printf( "Die:        %-10d Steal:      %-10d Backstab:    %-10d \r\n", deity->die, deity->steal, deity->backstab );
   ch->printf( "Aid:        %-10d Aid_spell:  %-10d Spell_aid:   %-10d \r\n", deity->aid, deity->aid_spell, deity->spell_aid );
   ch->printf( "Object:     %-10d Object2:    %-10d Avatar:      %-10d \r\n", deity->deityobj, deity->deityobj2, deity->avatar );
   ch->printf( "Mount:      %-10d Minion:     %-10d Scorpse:     %-10d \r\n", deity->mount, deity->minion, deity->scorpse );
   ch->printf( "Savatar:    %-10d Smount:     %-10d Sminion:     %-10d \r\n", deity->savatar, deity->smount, deity->sminion );
   ch->printf( "Sdeityobj:  %-10d Sdeityobj2: %-10d Srecall:     %-10d \r\n", deity->sdeityobj, deity->sdeityobj, deity->srecall );
   ch->printf( "Recallroom: %-10d Objstat:    %-10d Worshippers: %-10d \r\n", deity->recallroom, deity->objstat, deity->worshippers );
}

CMDF( do_makedeity )
{
   deity_data *deity;

   if( argument.empty(  ) )
   {
      ch->print( "Usage: makedeity <deity name>\r\n" );
      return;
   }

   smash_tilde( argument );

   if( ( deity = get_deity( argument ) ) )
   {
      ch->print( "A deity with that name already holds weight on this world.\r\n" );
      return;
   }

   deity = new deity_data;
   deitylist.push_back( deity );
   deity->name = argument;
   strlower( argument );
   deity->filename = argument;
   write_deity_list(  );
   save_deity( deity );
   ch->printf( "%s deity has been created\r\n", argument.c_str(  ) );
}

CMDF( do_devote )
{
   deity_data *deity;

   if( ch->isnpc(  ) )
   {
      ch->print( "Huh?\r\n" );
      return;
   }

   if( argument.empty(  ) )
   {
      ch->print( "Devote yourself to which deity?\r\n" );
      return;
   }

   if( !str_cmp( argument, "none" ) )
   {
      affect_data af;

      if( !ch->pcdata->deity )
      {
         ch->print( "You have already chosen to worship no deities.\r\n" );
         return;
      }
      --ch->pcdata->deity->worshippers;
      if( ch->pcdata->deity->worshippers < 0 )
         ch->pcdata->deity->worshippers = 0;
      ch->pcdata->favor = -2500;
      ch->worsen_mental_state( 50 );
      ch->print( "A terrible curse afflicts you as you forsake a deity!\r\n" );
      ch->unset_aflag( ch->pcdata->deity->affected[0] );
      ch->unset_aflag( ch->pcdata->deity->affected[1] );
      ch->unset_aflag( ch->pcdata->deity->affected[2] );
      ch->unset_resist( ch->pcdata->deity->element[0] );
      ch->unset_resist( ch->pcdata->deity->element[1] );
      ch->unset_resist( ch->pcdata->deity->element[2] );
      ch->unset_suscep( ch->pcdata->deity->suscept[0] );
      ch->unset_suscep( ch->pcdata->deity->suscept[1] );
      ch->unset_suscep( ch->pcdata->deity->suscept[2] );
      ch->affect_strip( gsn_blindness );
      af.type = gsn_blindness;
      af.location = APPLY_HITROLL;
      af.modifier = -4;
      af.duration = ( int )( 50 * DUR_CONV );
      af.bit = AFF_BLIND;
      ch->affect_to_char( &af );
      save_deity( ch->pcdata->deity );
      ch->print( "You cease to worship any deity.\r\n" );
      ch->pcdata->deity = NULL;
      ch->pcdata->deity_name.clear(  );
      ch->save(  );
      return;
   }

   if( !( deity = get_deity( argument ) ) )
   {
      ch->print( "No such deity holds weight on this world.\r\n" );
      return;
   }

   if( ch->pcdata->deity )
   {
      ch->print( "You are already devoted to a deity.\r\n" );
      return;
   }

   /*
    * Edited by Tarl 25 Feb 02 
    */
   /*
    * Edited again by Samson 5-17-04 
    */
   if( deity->class_allowed.any(  ) && !deity->class_allowed.test( ch->Class ) )
   {
      ch->printf( "%s does not accept worshippers of class %s.\r\n", deity->name.c_str(  ), class_table[ch->Class]->who_name );
      return;
   }

   if( deity->sex != -1 && deity->sex != ch->sex )
   {
      ch->printf( "%s does not accept %s worshippers.\r\n", deity->name.c_str(  ), npc_sex[ch->sex] );
      return;
   }

   /*
    * Edited by Tarl 25 Feb 02 
    */
   /*
    * Edited again by Samson 5-17-04 
    */
   if( deity->race_allowed.any(  ) && !deity->race_allowed.test( ch->race ) )
   {
      ch->printf( "%s does not accept worshippers of the %s race.\r\n", deity->name.c_str(  ), race_table[ch->race]->race_name );
      return;
   }

   ch->pcdata->deity_name = deity->name;
   ch->pcdata->deity = deity;

   act( AT_MAGIC, "Body and soul, you devote yourself to $t!", ch, ch->pcdata->deity_name.c_str(  ), NULL, TO_CHAR );
   ++ch->pcdata->deity->worshippers;
   save_deity( ch->pcdata->deity );
   ch->save(  );
}

CMDF( do_deities )
{
   deity_data *deity;
   int count = 0;

   if( argument.empty(  ) )
   {
      list < deity_data * >::iterator dt;

      ch->pager( "&gFor detailed information on a deity, try 'deities <deity>' or 'help deities'\r\n" );
      ch->pager( "Deity           Worshippers\r\n" );
      for( dt = deitylist.begin(  ); dt != deitylist.end(  ); ++dt )
      {
         deity = *dt;

         ch->pagerf( "&G%-14s   &g%19d\r\n", deity->name.c_str(  ), deity->worshippers );
         ++count;
      }
      if( !count )
      {
         ch->pager( "&gThere are no deities on this world.\r\n" );
         return;
      }
      return;
   }

   if( !( deity = get_deity( argument ) ) )
   {
      ch->print( "&gThat deity does not exist.\r\n" );
      return;
   }

   ch->pagerf( "&gDeity:        &G%s\r\n", deity->name.c_str(  ) );
   ch->pagerf( "&gDescription:\r\n&G%s", deity->deitydesc.c_str(  ) );
}

/*
Internal function to adjust favor.
Fields are:
0 = flee        5 = sac         10 = backstab
1 = flee_npcrace    6 = bury_corpse     11 = die
2 = kill        7 = aid_spell       12 = die_npcrace
3 = kill_npcrace    8 = aid         13 = spell_aid
4 = kill_magic      9 = steal       14 = dig_corpse
15 = die_npcfoe        16 = flee_npcfoe         17 = kill_npcfoe
*/
void char_data::adjust_favor( int field, int mod )
{
   int oldfavor;

   if( isnpc(  ) || !pcdata->deity )
      return;

   oldfavor = pcdata->favor;

   if( ( alignment - pcdata->deity->alignment > 650 || alignment - pcdata->deity->alignment < -650 ) && pcdata->deity->alignment != 0 )
   {
      pcdata->favor -= 2;
      pcdata->favor = URANGE( -2500, pcdata->favor, 5000 );

      if( pcdata->favor > pcdata->deity->affectednum[0] )
      {
         if( pcdata->deity->affected[0] > 0 && !affected_by.test( pcdata->deity->affected[0] ) )
         {
            printf( "%s looks favorably upon you and bestows %s as a reward.\r\n", pcdata->deity_name.c_str(  ), aff_flags[pcdata->deity->affected[0]] );
            affected_by.set( pcdata->deity->affected[0] );
         }
      }
      if( pcdata->favor > pcdata->deity->affectednum[1] )
      {
         if( pcdata->deity->affected[1] > 0 && !affected_by.test( pcdata->deity->affected[1] ) )
         {
            printf( "%s looks favorably upon you and bestows %s as a reward.\r\n", pcdata->deity_name.c_str(  ), aff_flags[pcdata->deity->affected[1]] );
            affected_by.set( pcdata->deity->affected[1] );
         }
      }
      if( pcdata->favor > pcdata->deity->affectednum[2] )
      {
         if( pcdata->deity->affected[2] > 0 && !affected_by.test( pcdata->deity->affected[2] ) )
         {
            printf( "%s looks favorably upon you and bestows %s as a reward.\r\n", pcdata->deity_name.c_str(  ), aff_flags[pcdata->deity->affected[2]] );
            affected_by.set( pcdata->deity->affected[2] );
         }
      }
      if( pcdata->favor > pcdata->deity->elementnum[0] )
      {
         if( pcdata->deity->element[0] != 0 && !has_resist( pcdata->deity->element[0] ) )
         {
            set_resist( pcdata->deity->element[0] );
            printf( "%s looks favorably upon you and bestows %s resistance upon you.\r\n", pcdata->deity_name.c_str(  ), ris_flags[pcdata->deity->element[0]] );
         }
      }
      if( pcdata->favor > pcdata->deity->elementnum[1] )
      {
         if( pcdata->deity->element[1] != 0 && !has_resist( pcdata->deity->element[1] ) )
         {
            set_resist( pcdata->deity->element[1] );
            printf( "%s looks favorably upon you and bestows %s resistance upon you.\r\n", pcdata->deity_name.c_str(  ), ris_flags[pcdata->deity->element[1]] );
         }
      }
      if( pcdata->favor > pcdata->deity->elementnum[2] )
      {
         if( pcdata->deity->element[2] != 0 && !has_resist( pcdata->deity->element[2] ) )
         {
            set_resist( pcdata->deity->element[2] );
            printf( "%s looks favorably upon you and bestows %s resistance upon you.\r\n", pcdata->deity_name.c_str(  ), ris_flags[pcdata->deity->element[2]] );
         }
      }
      if( pcdata->favor < pcdata->deity->susceptnum[0] )
      {
         if( pcdata->deity->suscept[0] != 0 && !has_suscep( pcdata->deity->suscept[0] ) )
         {
            set_suscep( pcdata->deity->suscept[0] );
            printf( "%s looks poorly upon you and makes you more vulnerable to %s as punishment.\r\n", pcdata->deity_name.c_str(  ), ris_flags[pcdata->deity->suscept[0]] );
         }
      }
      if( pcdata->favor < pcdata->deity->susceptnum[1] )
      {
         if( pcdata->deity->suscept[1] != 0 && !has_suscep( pcdata->deity->suscept[1] ) )
         {
            set_suscep( pcdata->deity->suscept[1] );
            printf( "%s looks poorly upon you and makes you more vulnerable to %s as punishment.\r\n", pcdata->deity_name.c_str(  ), ris_flags[pcdata->deity->suscept[1]] );
         }
      }
      if( pcdata->favor < pcdata->deity->susceptnum[2] )
      {
         if( pcdata->deity->suscept[2] != 0 && !has_suscep( pcdata->deity->suscept[2] ) )
         {
            set_suscep( pcdata->deity->suscept[2] );
            printf( "%s looks poorly upon you and makes you more vulnerable to %s as punishment.\r\n", pcdata->deity_name.c_str(  ), ris_flags[pcdata->deity->suscept[2]] );
         }
      }

      if( ( oldfavor > pcdata->deity->affectednum[0] && pcdata->favor <= pcdata->deity->affectednum[0] )
          || ( oldfavor > pcdata->deity->affectednum[1] && pcdata->favor <= pcdata->deity->affectednum[1] )
          || ( oldfavor > pcdata->deity->affectednum[2] && pcdata->favor <= pcdata->deity->affectednum[2] )
          || ( oldfavor > pcdata->deity->elementnum[0] && pcdata->favor <= pcdata->deity->elementnum[0] )
          || ( oldfavor > pcdata->deity->elementnum[1] && pcdata->favor <= pcdata->deity->elementnum[1] )
          || ( oldfavor > pcdata->deity->elementnum[2] && pcdata->favor <= pcdata->deity->elementnum[2] )
          || ( oldfavor < pcdata->deity->susceptnum[0] && pcdata->favor >= pcdata->deity->susceptnum[0] )
          || ( oldfavor < pcdata->deity->susceptnum[1] && pcdata->favor >= pcdata->deity->susceptnum[1] )
          || ( oldfavor < pcdata->deity->susceptnum[2] && pcdata->favor >= pcdata->deity->susceptnum[2] ) )
      {
         update_aris(  );
      }

      /*
       * Added by Tarl 24 Mar 02 
       */
      if( ( oldfavor > pcdata->deity->sspell[0] ) && ( pcdata->favor <= pcdata->deity->sspell[0] ) )
      {
         if( level < skill_table[pcdata->deity->spell[0]]->skill_level[Class] && level < skill_table[pcdata->deity->spell[0]]->race_level[race] )
         {
            pcdata->learned[pcdata->deity->spell[0]] = 0;
         }
      }

      if( ( oldfavor > pcdata->deity->sspell[1] ) && ( pcdata->favor <= pcdata->deity->sspell[1] ) )
      {
         if( level < skill_table[pcdata->deity->spell[1]]->skill_level[Class] && level < skill_table[pcdata->deity->spell[1]]->race_level[race] )
         {
            pcdata->learned[pcdata->deity->spell[1]] = 0;
         }
      }

      if( ( oldfavor > pcdata->deity->sspell[2] ) && ( pcdata->favor <= pcdata->deity->sspell[2] ) )
      {
         if( level < skill_table[pcdata->deity->spell[2]]->skill_level[Class] && level < skill_table[pcdata->deity->spell[2]]->race_level[race] )
         {
            pcdata->learned[pcdata->deity->spell[2]] = 0;
         }
      }
      return;
   }

   if( mod < 1 )
      mod = 1;
   switch ( field )
   {
      default:
         break;

      case 0:
         pcdata->favor += number_fuzzy( pcdata->deity->flee / mod );
         break;
      case 1:
         pcdata->favor += number_fuzzy( pcdata->deity->flee_npcrace[0] / mod );
         break;
      case 2:
         pcdata->favor += number_fuzzy( pcdata->deity->kill / mod );
         break;
      case 3:
         pcdata->favor += number_fuzzy( pcdata->deity->kill_npcrace[0] / mod );
         break;
      case 4:
         pcdata->favor += number_fuzzy( pcdata->deity->kill_magic / mod );
         break;
      case 5:
         pcdata->favor += number_fuzzy( pcdata->deity->sac / mod );
         break;
      case 6:
         pcdata->favor += number_fuzzy( pcdata->deity->bury_corpse / mod );
         break;
      case 7:
         pcdata->favor += number_fuzzy( pcdata->deity->aid_spell / mod );
         break;
      case 8:
         pcdata->favor += number_fuzzy( pcdata->deity->aid / mod );
         break;
      case 9:
         pcdata->favor += number_fuzzy( pcdata->deity->steal / mod );
         break;
      case 10:
         pcdata->favor += number_fuzzy( pcdata->deity->backstab / mod );
         break;
      case 11:
         pcdata->favor += number_fuzzy( pcdata->deity->die / mod );
         break;
      case 12:
         pcdata->favor += number_fuzzy( pcdata->deity->die_npcrace[0] / mod );
         break;
      case 13:
         pcdata->favor += number_fuzzy( pcdata->deity->spell_aid / mod );
         break;
      case 14:
         pcdata->favor += number_fuzzy( pcdata->deity->dig_corpse / mod );
         break;
      case 15:
         pcdata->favor += number_fuzzy( pcdata->deity->die_npcfoe[0] / mod );
         break;
      case 16:
         pcdata->favor += number_fuzzy( pcdata->deity->flee_npcfoe[0] / mod );
         break;
      case 17:
         pcdata->favor += number_fuzzy( pcdata->deity->kill_npcfoe[0] / mod );
         break;
      case 18:
         pcdata->favor += number_fuzzy( pcdata->deity->flee_npcrace[1] / mod );
         break;
      case 19:
         pcdata->favor += number_fuzzy( pcdata->deity->flee_npcrace[2] / mod );
         break;
      case 20:
         pcdata->favor += number_fuzzy( pcdata->deity->flee_npcfoe[1] / mod );
         break;
      case 21:
         pcdata->favor += number_fuzzy( pcdata->deity->flee_npcfoe[2] / mod );
         break;
      case 22:
         pcdata->favor += number_fuzzy( pcdata->deity->kill_npcrace[1] / mod );
         break;
      case 23:
         pcdata->favor += number_fuzzy( pcdata->deity->kill_npcrace[2] / mod );
         break;
      case 24:
         pcdata->favor += number_fuzzy( pcdata->deity->kill_npcfoe[1] / mod );
         break;
      case 25:
         pcdata->favor += number_fuzzy( pcdata->deity->kill_npcfoe[2] / mod );
         break;
      case 26:
         pcdata->favor += number_fuzzy( pcdata->deity->die_npcrace[1] / mod );
         break;
      case 27:
         pcdata->favor += number_fuzzy( pcdata->deity->die_npcrace[2] / mod );
         break;
      case 28:
         pcdata->favor += number_fuzzy( pcdata->deity->die_npcfoe[1] / mod );
         break;
      case 29:
         pcdata->favor += number_fuzzy( pcdata->deity->die_npcfoe[2] / mod );
         break;
   }
   pcdata->favor = URANGE( -2500, pcdata->favor, 5000 );

   if( pcdata->favor > pcdata->deity->affectednum[0] )
   {
      if( pcdata->deity->affected[0] > 0 && !affected_by.test( pcdata->deity->affected[0] ) )
      {
         printf( "%s looks favorably upon you and bestows %s as a reward.\r\n", pcdata->deity_name.c_str(  ), aff_flags[pcdata->deity->affected[0]] );
         affected_by.set( pcdata->deity->affected[0] );
      }
   }
   if( pcdata->favor > pcdata->deity->affectednum[1] )
   {
      if( pcdata->deity->affected[1] > 0 && !affected_by.test( pcdata->deity->affected[1] ) )
      {
         printf( "%s looks favorably upon you and bestows %s as a reward.\r\n", pcdata->deity_name.c_str(  ), aff_flags[pcdata->deity->affected[1]] );
         affected_by.set( pcdata->deity->affected[1] );
      }
   }
   if( pcdata->favor > pcdata->deity->affectednum[2] )
   {
      if( pcdata->deity->affected[2] > 0 && !affected_by.test( pcdata->deity->affected[2] ) )
      {
         printf( "%s looks favorably upon you and bestows %s as a reward.\r\n", pcdata->deity_name.c_str(  ), aff_flags[pcdata->deity->affected[2]] );
         affected_by.set( pcdata->deity->affected[2] );
      }
   }
   if( pcdata->favor > pcdata->deity->elementnum[0] )
   {
      if( pcdata->deity->element[0] != 0 && !has_resist( pcdata->deity->element[0] ) )
      {
         set_resist( pcdata->deity->element[0] );
         printf( "%s looks favorably upon you and bestows %s resistance upon you.\r\n", pcdata->deity_name.c_str(  ), ris_flags[pcdata->deity->element[0]] );
      }
   }
   if( pcdata->favor > pcdata->deity->elementnum[1] )
   {
      if( pcdata->deity->element[1] != 0 && !has_resist( pcdata->deity->element[1] ) )
      {
         set_resist( pcdata->deity->element[1] );
         printf( "%s looks favorably upon you and bestows %s resistance upon you.\r\n", pcdata->deity_name.c_str(  ), ris_flags[pcdata->deity->element[1]] );
      }
   }
   if( pcdata->favor > pcdata->deity->elementnum[2] )
   {
      if( pcdata->deity->element[2] != 0 && !has_resist( pcdata->deity->element[2] ) )
      {
         set_resist( pcdata->deity->element[2] );
         printf( "%s looks favorably upon you and bestows %s resistance upon you.\r\n", pcdata->deity_name.c_str(  ), ris_flags[pcdata->deity->element[2]] );
      }
   }
   if( pcdata->favor < pcdata->deity->susceptnum[0] )
   {
      if( pcdata->deity->suscept[0] != 0 && !has_suscep( pcdata->deity->suscept[0] ) )
      {
         set_suscep( pcdata->deity->suscept[0] );
         printf( "%s looks poorly upon you and makes you more vulnerable to %s as punishment.\r\n", pcdata->deity_name.c_str(  ), ris_flags[pcdata->deity->suscept[0]] );
      }
   }
   if( pcdata->favor < pcdata->deity->susceptnum[1] )
   {
      if( pcdata->deity->suscept[1] != 0 && !has_suscep( pcdata->deity->suscept[1] ) )
      {
         set_suscep( pcdata->deity->suscept[1] );
         printf( "%s looks poorly upon you and makes you more vulnerable to %s as punishment.\r\n", pcdata->deity_name.c_str(  ), ris_flags[pcdata->deity->suscept[1]] );
      }
   }
   if( pcdata->favor < pcdata->deity->susceptnum[2] )
   {
      if( pcdata->deity->suscept[2] != 0 && !has_suscep( pcdata->deity->suscept[2] ) )
      {
         set_suscep( pcdata->deity->suscept[2] );
         printf( "%s looks poorly upon you and makes you more vulnerable to %s as punishment.\r\n", pcdata->deity_name.c_str(  ), ris_flags[pcdata->deity->suscept[2]] );
      }
   }
   /*
    * Added by Tarl 24 Mar 02 
    */
   if( pcdata->favor > pcdata->deity->sspell[0] )
   {
      if( pcdata->deity->spell[0] != 0 )
      {
         if( pcdata->learned[pcdata->deity->spell[0]] != 0 )
            return;
         pcdata->learned[pcdata->deity->spell[0]] = 95;
      }
   }

   if( pcdata->favor > pcdata->deity->sspell[1] )
   {
      if( pcdata->deity->spell[1] != 0 )
      {
         if( pcdata->learned[pcdata->deity->spell[1]] != 0 )
            return;
         pcdata->learned[pcdata->deity->spell[1]] = 95;
      }
   }

   if( pcdata->favor > pcdata->deity->sspell[2] )
   {
      if( pcdata->deity->spell[2] != 0 )
      {
         if( pcdata->learned[pcdata->deity->spell[2]] != 0 )
            return;
         pcdata->learned[pcdata->deity->spell[2]] = 95;
      }
   }

   /*
    * If favor crosses over the line then strip the affect 
    */
   if( ( oldfavor > pcdata->deity->affectednum[0] && pcdata->favor <= pcdata->deity->affectednum[0] )
       || ( oldfavor > pcdata->deity->affectednum[1] && pcdata->favor <= pcdata->deity->affectednum[1] )
       || ( oldfavor > pcdata->deity->affectednum[2] && pcdata->favor <= pcdata->deity->affectednum[2] )
       || ( oldfavor > pcdata->deity->elementnum[0] && pcdata->favor <= pcdata->deity->elementnum[0] )
       || ( oldfavor > pcdata->deity->elementnum[1] && pcdata->favor <= pcdata->deity->elementnum[1] )
       || ( oldfavor > pcdata->deity->elementnum[2] && pcdata->favor <= pcdata->deity->elementnum[2] )
       || ( oldfavor < pcdata->deity->susceptnum[0] && pcdata->favor >= pcdata->deity->susceptnum[0] )
       || ( oldfavor < pcdata->deity->susceptnum[1] && pcdata->favor >= pcdata->deity->susceptnum[1] )
       || ( oldfavor < pcdata->deity->susceptnum[2] && pcdata->favor >= pcdata->deity->susceptnum[2] ) )
   {
      update_aris(  );
   }

   /*
    * Added by Tarl 24 Mar 02 
    */
   if( ( oldfavor > pcdata->deity->sspell[0] ) && ( pcdata->favor <= pcdata->deity->sspell[0] ) )
   {
      if( level < skill_table[pcdata->deity->spell[0]]->skill_level[Class] && level < skill_table[pcdata->deity->spell[0]]->race_level[race] )
      {
         pcdata->learned[pcdata->deity->spell[0]] = 0;
      }
   }

   if( ( oldfavor > pcdata->deity->sspell[1] ) && ( pcdata->favor <= pcdata->deity->sspell[1] ) )
   {
      if( level < skill_table[pcdata->deity->spell[1]]->skill_level[Class] && level < skill_table[pcdata->deity->spell[1]]->race_level[race] )
      {
         pcdata->learned[pcdata->deity->spell[1]] = 0;
      }
   }

   if( ( oldfavor > pcdata->deity->sspell[2] ) && ( pcdata->favor <= pcdata->deity->sspell[2] ) )
   {
      if( level < skill_table[pcdata->deity->spell[2]]->skill_level[Class] && level < skill_table[pcdata->deity->spell[2]]->race_level[race] )
      {
         pcdata->learned[pcdata->deity->spell[2]] = 0;
      }
   }
}

CMDF( do_supplicate )
{
   int oldfavor;

   if( ch->isnpc(  ) || !ch->pcdata->deity )
   {
      ch->print( "You have no deity to supplicate to.\r\n" );
      return;
   }

   oldfavor = ch->pcdata->favor;

   if( argument.empty(  ) )
   {
      ch->print( "Supplicate for what?\r\n" );
      return;
   }

   if( !str_cmp( argument, "corpse" ) )
   {
      string buf2 = "the corpse of ";
      buf2.append( ch->name );

      if( ch->pcdata->favor < ch->pcdata->deity->scorpse )
      {
         ch->print( "You are not favored enough for a corpse retrieval.\r\n" );
         return;
      }

      if( ch->in_room->flags.test( ROOM_CLANSTOREROOM ) )
      {
         ch->print( "You cannot supplicate in a storage room.\r\n" );
         return;
      }

      bool found = false;
      list < obj_data * >::iterator iobj;
      for( iobj = objlist.begin(  ); iobj != objlist.end(  ); ++iobj )
      {
         obj_data *obj = *iobj;

         if( obj->in_room && !str_cmp( buf2, obj->short_descr ) && ( obj->pIndexData->vnum == OBJ_VNUM_CORPSE_PC ) )
         {
            found = true;
            if( obj->in_room->flags.test( ROOM_NOSUPPLICATE ) )
            {
               act( AT_MAGIC, "The image of your corpse appears, but suddenly wavers away.", ch, NULL, NULL, TO_CHAR );
               return;
            }
            act( AT_MAGIC, "Your corpse appears suddenly, surrounded by a divine presence...", ch, NULL, NULL, TO_CHAR );
            act( AT_MAGIC, "$n's corpse appears suddenly, surrounded by a divine force...", ch, NULL, NULL, TO_ROOM );
            obj->from_room(  );
            obj = obj->to_room( ch->in_room, ch );
            obj->extra_flags.reset( ITEM_BURIED );
         }
      }
      if( !found )
      {
         ch->print( "No corpse of yours litters the world...\r\n" );
         return;
      }
      ch->pcdata->favor -= ch->pcdata->deity->scorpse;
      ch->adjust_favor( -1, 1 );
      return;
   }

   if( !str_cmp( argument, "avatar" ) )
   {
      mob_index *pMobIndex;
      char_data *victim;

      if( ch->pcdata->favor < ch->pcdata->deity->savatar )
      {
         ch->print( "You are not favored enough for that.\r\n" );
         return;
      }

      if( !( pMobIndex = get_mob_index( ch->pcdata->deity->avatar ) ) )
      {
         ch->print( "Your deity has no avatar to pray for.\r\n" );
         return;
      }

      if( check_pets( ch, pMobIndex ) )
      {
         ch->print( "You have already received an avatar of your deity!\r\n" );
         return;
      }

      victim = pMobIndex->create_mobile(  );
      if( !victim->to_room( ch->in_room ) )
         log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
      fix_maps( ch, victim );

      act( AT_MAGIC, "$n summons a powerful avatar!", ch, NULL, NULL, TO_ROOM );
      act( AT_MAGIC, "You summon a powerful avatar!", ch, NULL, NULL, TO_CHAR );
      bind_follower( victim, ch, gsn_charm_person, ( ch->pcdata->favor / 4 ) + 1 );
      victim->level = LEVEL_AVATAR / 2;
      victim->hit = ch->hit + ch->pcdata->favor;
      victim->alignment = ch->pcdata->deity->alignment;
      victim->max_hit = ch->hit + ch->pcdata->favor;

      ch->pcdata->favor -= ch->pcdata->deity->savatar;
      ch->adjust_favor( -1, 1 );
      return;
   }

   /*
    * Added by Tarl 24 Feb 02 
    */
   if( !str_cmp( argument, "mount" ) )
   {
      mob_index *pMobIndex;
      char_data *victim;

      if( ch->pcdata->favor < ch->pcdata->deity->smount )
      {
         ch->print( "You are not favored enough for that.\r\n" );
         return;
      }

      if( !( pMobIndex = get_mob_index( ch->pcdata->deity->mount ) ) )
      {
         ch->print( "Your deity has no mount to pray for.\r\n" );
         return;
      }

      if( check_pets( ch, pMobIndex ) )
      {
         ch->print( "You have already received a mount of your deity!\r\n" );
         return;
      }

      victim = pMobIndex->create_mobile(  );
      if( !victim->to_room( ch->in_room ) )
         log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
      fix_maps( ch, victim );

      act( AT_MAGIC, "$n summons a mount!", ch, NULL, NULL, TO_ROOM );
      act( AT_MAGIC, "You summon a mount!", ch, NULL, NULL, TO_CHAR );
      bind_follower( victim, ch, gsn_charm_person, ( ch->pcdata->favor / 4 ) + 1 );
      ch->pcdata->favor -= ch->pcdata->deity->smount;
      ch->adjust_favor( -1, 1 );
      return;
   }

   /*
    * Added by Tarl 24 Feb 02 
    */
   if( !str_cmp( argument, "minion" ) )
   {
      mob_index *pMobIndex;
      char_data *victim;

      if( ch->pcdata->favor < ch->pcdata->deity->sminion )
      {
         ch->print( "You are not favored enough for that.\r\n" );
         return;
      }

      if( !( pMobIndex = get_mob_index( ch->pcdata->deity->minion ) ) )
      {
         ch->print( "Your deity has no minion to pray for.\r\n" );
         return;
      }

      if( check_pets( ch, pMobIndex ) )
      {
         ch->print( "You have already received a minion of your deity!\r\n" );
         return;
      }

      victim = pMobIndex->create_mobile(  );
      if( !victim->to_room( ch->in_room ) )
         log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
      fix_maps( ch, victim );

      act( AT_MAGIC, "$n summons a minion!", ch, NULL, NULL, TO_ROOM );
      act( AT_MAGIC, "You summon a minion!", ch, NULL, NULL, TO_CHAR );
      bind_follower( victim, ch, gsn_charm_person, ( ch->pcdata->favor / 4 ) + 1 );
      victim->level = LEVEL_AVATAR / 3;
      victim->hit = ( ch->hit / 2 ) + ( ch->pcdata->favor / 4 );
      victim->alignment = ch->pcdata->deity->alignment;
      victim->max_hit = ( ch->hit / 2 ) + ( ch->pcdata->favor / 4 );

      ch->pcdata->favor -= ch->pcdata->deity->sminion;
      ch->adjust_favor( -1, 1 );
      return;
   }

   if( !str_cmp( argument, "object" ) )
   {
      obj_data *obj;
      obj_index *pObjIndex;
      affect_data *paf;

      if( ch->pcdata->favor < ch->pcdata->deity->sdeityobj )
      {
         ch->print( "You are not favored enough for that.\r\n" );
         return;
      }

      if( !( pObjIndex = get_obj_index( ch->pcdata->deity->deityobj ) ) )
      {
         ch->print( "Your deity has no object to pray for.\r\n" );
         return;
      }

      if( !( obj = pObjIndex->create_object( ch->level ) ) )
      {
         log_printf( "create_object: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
         return;
      }
      if( obj->wear_flags.test( ITEM_TAKE ) )
         obj = obj->to_char( ch );
      else
         obj = obj->to_room( ch->in_room, ch );

      act( AT_MAGIC, "$n weaves $p from divine matter!", ch, obj, NULL, TO_ROOM );
      act( AT_MAGIC, "You weave $p from divine matter!", ch, obj, NULL, TO_CHAR );
      ch->pcdata->favor -= ch->pcdata->deity->sdeityobj;
      ch->adjust_favor( -1, 1 );

      paf = new affect_data;
      paf->type = -1;
      paf->duration = -1;
      paf->rismod.reset(  );

      switch ( ch->pcdata->deity->objstat )
      {
         default:
            paf->location = APPLY_NONE;
            break;
         case 0:
            paf->location = APPLY_STR;
            break;
         case 1:
            paf->location = APPLY_INT;
            break;
         case 2:
            paf->location = APPLY_WIS;
            break;
         case 3:
            paf->location = APPLY_CON;
            break;
         case 4:
            paf->location = APPLY_DEX;
            break;
         case 5:
            paf->location = APPLY_CHA;
            break;
         case 6:
            paf->location = APPLY_LCK;
            break;
      }
      paf->modifier = 1;
      paf->bit = 0;
      obj->affects.push_back( paf );
      return;
   }

   if( !str_cmp( argument, "object2" ) )
   {
      obj_data *obj;
      obj_index *pObjIndex;
      affect_data *paf;

      if( ch->pcdata->favor < ch->pcdata->deity->sdeityobj2 )
      {
         ch->print( "You are not favored enough for that.\r\n" );
         return;
      }

      if( !( pObjIndex = get_obj_index( ch->pcdata->deity->deityobj2 ) ) )
      {
         ch->print( "Your deity has no object to pray for.\r\n" );
         return;
      }

      if( !( obj = pObjIndex->create_object( ch->level ) ) )
      {
         log_printf( "create_object: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
         return;
      }
      if( obj->wear_flags.test( ITEM_TAKE ) )
         obj = obj->to_char( ch );
      else
         obj = obj->to_room( ch->in_room, ch );

      stralloc_printf( &obj->name, "sigil %s", ch->pcdata->deity->name.c_str(  ) );

      act( AT_MAGIC, "$n weaves $p from divine matter!", ch, obj, NULL, TO_ROOM );
      act( AT_MAGIC, "You weave $p from divine matter!", ch, obj, NULL, TO_CHAR );
      ch->pcdata->favor -= ch->pcdata->deity->sdeityobj2;
      ch->adjust_favor( -1, 1 );

      paf = new affect_data;
      paf->type = -1;
      paf->duration = -1;
      paf->rismod.reset(  );

      switch ( ch->pcdata->deity->objstat )
      {
         default:
            paf->location = APPLY_NONE;
            break;
         case 0:
            paf->location = APPLY_STR;
            break;
         case 1:
            paf->location = APPLY_INT;
            break;
         case 2:
            paf->location = APPLY_WIS;
            break;
         case 3:
            paf->location = APPLY_CON;
            break;
         case 4:
            paf->location = APPLY_DEX;
            break;
         case 5:
            paf->location = APPLY_CHA;
            break;
         case 6:
            paf->location = APPLY_LCK;
            break;
      }
      paf->modifier = 1;
      paf->bit = 0;
      obj->affects.push_back( paf );
      return;
   }

   if( !str_cmp( argument, "recall" ) )
   {
      room_index *location = NULL;

      if( ch->pcdata->favor < ch->pcdata->deity->srecall )
      {
         ch->print( "Your favor is inadequate for such a supplication.\r\n" );
         return;
      }

      if( ch->in_room->flags.test( ROOM_NOSUPPLICATE ) )
      {
         ch->print( "You have been forsaken!\r\n" );
         return;
      }

      if( ch->get_timer( TIMER_RECENTFIGHT ) > 0 && !ch->is_immortal(  ) )
      {
         ch->print( "You cannot supplicate recall under adrenaline!\r\n" );
         return;
      }

      location = recall_room( ch );

      if( !location )
      {
         bug( "%s: No room index for recall supplication. Deity: %s", __FUNCTION__, ch->pcdata->deity->name.c_str(  ) );
         ch->print( "Your deity has forsaken you!\r\n" );
         return;
      }

      act( AT_MAGIC, "$n disappears in a column of divine power.", ch, NULL, NULL, TO_ROOM );

      leave_map( ch, NULL, location );

      act( AT_MAGIC, "$n appears in the room from a column of divine mist.", ch, NULL, NULL, TO_ROOM );
      ch->pcdata->favor -= ch->pcdata->deity->srecall;
      ch->adjust_favor( -1, 1 );
      return;
   }
   ch->print( "You cannot supplicate for that.\r\n" );
}
