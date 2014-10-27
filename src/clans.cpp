/****************************************************************************
 *                   ^     +----- |  / ^     ^ |     | +-\                  *
 *                  / \    |      | /  |\   /| |     | |  \                 *
 *                 /   \   +---   |<   | \ / | |     | |  |                 *
 *                /-----\  |      | \  |  v  | |     | |  /                 *
 *               /       \ |      |  \ |     | +-----+ +-/                  *
 ****************************************************************************
 * AFKMud Copyright 1997-2014 by Roger Libiez (Samson),                     *
 * Levi Beckerson (Whir), Michael Ward (Tarl), Erik Wolfe (Dwip),           *
 * Cameron Carroll (Cam), Cyberfox, Karangi, Rathian, Raine,                *
 * Xorith, and Adjani.                                                      *
 * All Rights Reserved.                                                     *
 *                                                                          *
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
 *                           Special clan module                            *
 ****************************************************************************/

#include <sstream>
#include "mud.h"
#include "clans.h"
#include "descriptor.h"
#include "mobindex.h"
#include "mud_prog.h"
#include "objindex.h"
#include "raceclass.h"
#include "roomindex.h"
#include "shops.h"

list < clan_data * >clanlist;

void save_shop( char_data * );

roster_data::roster_data(  )
{
   init_memory( &joined, &deaths, sizeof( deaths ) );
}

roster_data::~roster_data(  )
{
}

void add_roster( clan_data * clan, const string & name, int Class, int level, int kills, int deaths )
{
   roster_data *roster;

   roster = new roster_data;
   roster->name = name;
   roster->Class = Class;
   roster->level = level;
   roster->kills = kills;
   roster->deaths = deaths;
   roster->joined = current_time;
   clan->memberlist.push_back( roster );
}

void remove_roster( clan_data * clan, const string & name )
{
   list < roster_data * >::iterator mem;

   for( mem = clan->memberlist.begin(  ); mem != clan->memberlist.end(  ); ++mem )
   {
      roster_data *member = *mem;

      if( !str_cmp( name, member->name ) )
      {
         clan->memberlist.remove( member );
         deleteptr( member );
         return;
      }
   }
}

void update_roster( char_data * ch )
{
   list < roster_data * >::iterator mem;

   for( mem = ch->pcdata->clan->memberlist.begin(  ); mem != ch->pcdata->clan->memberlist.end(  ); ++mem )
   {
      roster_data *member = *mem;

      if( !str_cmp( ch->name, member->name ) )
      {
         member->level = ch->level;
         member->kills = ch->pcdata->mkills;
         member->deaths = ch->pcdata->mdeaths;
         save_clan( ch->pcdata->clan );
         return;
      }
   }
   /*
    * If we make it here, assume they haven't been added previously 
    */
   add_roster( ch->pcdata->clan, ch->name, ch->Class, ch->level, ch->pcdata->mkills, ch->pcdata->mdeaths );
   save_clan( ch->pcdata->clan );
}

/* For use during clan removal and memory cleanup */
void remove_all_rosters( clan_data * clan )
{
   list < roster_data * >::iterator member;

   for( member = clan->memberlist.begin(  ); member != clan->memberlist.end(  ); )
   {
      roster_data *roster = *member;
      ++member;

      clan->memberlist.remove( roster );
      deleteptr( roster );
   }
}

clan_data::clan_data(  )
{
   init_memory( &pkills, &gettwo, sizeof( gettwo ) );
}

clan_data::~clan_data(  )
{
   remove_all_rosters( this );
   memberlist.clear(  );
   clanlist.remove( this );
}

/*
 * Get pointer to clan structure from clan name.
 */
clan_data *get_clan( const string & name )
{
   list < clan_data * >::iterator cl;

   for( cl = clanlist.begin(  ); cl != clanlist.end(  ); ++cl )
   {
      clan_data *clan = *cl;

      if( !str_cmp( name, clan->name ) )
         return clan;
   }
   return NULL;
}

/*
 * Save items in a clan storage room - Scryn & Thoric
 */
void save_clan_storeroom( char_data * ch, clan_data * clan )
{
   FILE *fp;
   char filename[256];
   short templvl;

   if( !clan )
   {
      bug( "%s: Null clan pointer!", __FUNCTION__ );
      return;
   }

   if( !ch )
   {
      bug( "%s: Null ch pointer!", __FUNCTION__ );
      return;
   }

   snprintf( filename, 256, "%s%s.vault", CLAN_DIR, clan->filename.c_str(  ) );
   if( !( fp = fopen( filename, "w" ) ) )
   {
      bug( "%s: fopen", __FUNCTION__ );
      perror( filename );
   }
   else
   {
      templvl = ch->level;
      ch->level = LEVEL_AVATAR;  /* make sure EQ doesn't get lost */
      if( !ch->in_room->objects.empty(  ) )
         fwrite_obj( ch, ch->in_room->objects, clan, fp, 0, false );
      fprintf( fp, "%s", "#END\n" );
      ch->level = templvl;
      FCLOSE( fp );
   }
}

void check_clan_storeroom( char_data * ch )
{
   list < clan_data * >::iterator cl;

   for( cl = clanlist.begin(  ); cl != clanlist.end(  ); ++cl )
   {
      clan_data *clan = *cl;

      if( clan->storeroom == ch->in_room->vnum )
         save_clan_storeroom( ch, clan );
   }
}

void check_clan_shop( char_data * ch, char_data * victim, obj_data * obj )
{
   list < clan_data * >::iterator cl;

   bool cfound = false;

   if( ch->isnpc(  ) )
      return;

   clan_data *clan = NULL;
   for( cl = clanlist.begin(  ); cl != clanlist.end(  ); ++cl )
   {
      if( victim->pIndexData->vnum == clan->shopkeeper )
      {
         cfound = true;
         break;
      }
   }

   if( ( cfound && clan != ch->pcdata->clan ) || obj->extra_flags.test( ITEM_PERSONAL ) || obj->ego >= sysdata->minego )
   {
      ch->printf( "%s says 'I cannot accept this from you.'\r\n", victim->short_descr );
      ch->printf( "%s hands %s back to you.\r\n", victim->short_descr, obj->short_descr );
      obj->from_char(  );
      obj = obj->to_char( ch );
      ch->save(  );
      return;
   }

   ch->printf( "%s puts %s into inventory.\r\n", victim->short_descr, obj->short_descr );
   save_shop( victim );
}

void free_clans( void )
{
   list < clan_data * >::iterator cl;

   for( cl = clanlist.begin(  ); cl != clanlist.end(  ); )
   {
      clan_data *clan = *cl;
      ++cl;

      deleteptr( clan );
   }
}

void delete_clan( char_data * ch, clan_data * clan )
{
   list < char_data * >::iterator ich;
   room_index *room = NULL;
   mob_index *mob = NULL;
   char filename[256], storeroom[256], record[256];
   string clanname = clan->name;

   mudstrlcpy( filename, clan->filename.c_str(  ), 256 );
   snprintf( storeroom, 256, "%s.vault", filename );
   snprintf( record, 256, "%s.record", filename );

   for( ich = pclist.begin(  ); ich != pclist.end(  ); ++ich )
   {
      char_data *vch = *ich;

      if( !vch->pcdata->clan )
         continue;

      if( vch->pcdata->clan == clan )
      {
         vch->pcdata->clan_name.clear(  );
         vch->pcdata->clan = NULL;
         vch->printf( "The %s known as &W%s&D has been destroyed by the gods!\r\n", clan->clan_type == CLAN_GUILD ? "guild" : "clan", clan->name.c_str(  ) );
      }
   }

   if( !remove( record ) )
   {
      if( !ch )
         log_string( "Clan Pkill record file destroyed." );
      else
         ch->print( "Clan Pkill record file destroyed.\r\n" );
   }

   if( clan->storeroom )
   {
      room = get_room_index( clan->storeroom );

      if( room )
      {
         room->flags.reset( ROOM_CLANSTOREROOM );
         if( !remove( storeroom ) )
         {
            if( !ch )
               log_string( "Clan storeroom file destroyed." );
            else
               ch->print( "Clan storeroom file destroyed.\r\n" );
         }
      }
   }

   if( clan->idmob )
   {
      mob = get_mob_index( clan->idmob );

      if( mob )
      {
         mob->actflags.reset( ACT_GUILDIDMOB );

         for( ich = charlist.begin(  ); ich != charlist.end(  ); ++ich )
         {
            char_data *vch = *ich;

            if( vch->isnpc(  ) && vch->pIndexData == mob )
               vch->unset_actflag( ACT_GUILDIDMOB );
         }
      }
   }

   if( clan->auction )
   {
      mob = get_mob_index( clan->auction );

      if( mob )
      {
         char aucfile[256];

         mob->actflags.reset( ACT_GUILDAUC );

         for( ich = charlist.begin(  ); ich != charlist.end(  ); ++ich )
         {
            char_data *vch = *ich;

            if( vch->isnpc(  ) && vch->pIndexData == mob )
            {
               vch->unset_actflag( ACT_GUILDAUC );
               vch->in_room->flags.reset( ROOM_AUCTION );
            }
         }
         snprintf( aucfile, 256, "%s%s", AUC_DIR, mob->short_descr );
         if( !remove( aucfile ) )
         {
            if( !ch )
               log_string( "Clan auction house file destroyed." );
            else
               ch->print( "Clan auction house file destroyed.\r\n" );
         }
      }
   }

   if( clan->shopkeeper )
   {
      mob = get_mob_index( clan->shopkeeper );

      if( mob )
      {
         char shopfile[256];

         mob->actflags.reset( ACT_GUILDVENDOR );

         for( ich = charlist.begin(  ); ich != charlist.end(  ); ++ich )
         {
            char_data *vch = *ich;

            if( vch->isnpc(  ) && vch->pIndexData == mob )
               vch->unset_actflag( ACT_GUILDVENDOR );
         }
         snprintf( shopfile, 256, "%s%s", SHOP_DIR, mob->short_descr );
         if( !remove( shopfile ) )
         {
            if( !ch )
               log_string( "Clan shop file destroyed." );
            else
               ch->print( "Clan shop file destroyed.\r\n" );
         }
      }
   }

   deleteptr( clan );

   if( !ch )
   {
      if( !remove( filename ) )
         log_printf( "Clan data for %s destroyed - no members left.", clanname.c_str(  ) );
      return;
   }

   if( !remove( filename ) )
   {
      ch->printf( "&RClan data for %s has been destroyed.\r\n", clanname.c_str(  ) );
      log_printf( "Clan data for %s has been destroyed by %s.", clanname.c_str(  ), ch->name );
   }
}

void write_clan_list( void )
{
   list < clan_data * >::iterator cl;
   FILE *fpout;
   char filename[256];

   snprintf( filename, 256, "%s%s", CLAN_DIR, CLAN_LIST );
   fpout = fopen( filename, "w" );
   if( !fpout )
   {
      bug( "%s: FATAL: cannot open clan.lst for writing!", __FUNCTION__ );
      return;
   }
   for( cl = clanlist.begin(  ); cl != clanlist.end(  ); ++cl )
   {
      clan_data *clan = *cl;

      fprintf( fpout, "%s\n", clan->filename.c_str(  ) );
   }
   fprintf( fpout, "%s", "$\n" );
   FCLOSE( fpout );
}

void fwrite_memberlist( FILE * fp, clan_data * clan )
{
   list < roster_data * >::iterator mem;

   for( mem = clan->memberlist.begin(  ); mem != clan->memberlist.end(  ); ++mem )
   {
      roster_data *member = *mem;

      fprintf( fp, "%s", "#ROSTER\n" );
      fprintf( fp, "Name      %s~\n", member->name.c_str(  ) );
      fprintf( fp, "Joined    %ld\n", ( time_t ) member->joined );
      fprintf( fp, "Class     %s~\n", npc_class[member->Class] );
      fprintf( fp, "Level     %d\n", member->level );
      fprintf( fp, "Kills     %d\n", member->kills );
      fprintf( fp, "Deaths    %d\n", member->deaths );
      fprintf( fp, "%s", "End\n\n" );
   }
}

void fread_memberlist( clan_data * clan, FILE * fp )
{
   roster_data *roster;

   roster = new roster_data;

   for( ;; )
   {
      const char *word = feof( fp ) ? "End" : fread_word( fp );

      switch ( UPPER( word[0] ) )
      {
         default:
            bug( "%s: no match: %s", __FUNCTION__, word );
            fread_to_eol( fp );
            break;

         case '*':
            fread_to_eol( fp );
            break;

         case 'C':
            if( !str_cmp( word, "Class" ) )
            {
               int Class = get_npc_class( fread_flagstring( fp ) );

               if( Class < 0 || Class >= MAX_NPC_CLASS )
               {
                  bug( "%s: Invalid class in clan roster", __FUNCTION__ );
                  Class = get_npc_class( "warrior" );
               }
               roster->Class = Class;
               break;
            }
            break;

         case 'D':
            KEY( "Deaths", roster->deaths, fread_number( fp ) );
            break;

         case 'E':
            if( !str_cmp( word, "End" ) )
            {
               clan->memberlist.push_back( roster );
               return;
            }
            break;

         case 'J':
            KEY( "Joined", roster->joined, fread_long( fp ) );
            break;

         case 'K':
            KEY( "Kills", roster->kills, fread_number( fp ) );
            break;

         case 'L':
            KEY( "Level", roster->level, fread_number( fp ) );
            break;

         case 'N':
            STDSKEY( "Name", roster->name );
            break;
      }
   }
}

/*
 * Save a clan's data to its data file
 */
const int CLAN_VERSION = 1;
void save_clan( clan_data * clan )
{
   FILE *fp;
   char filename[256];

   if( !clan )
   {
      bug( "%s: null clan pointer!", __FUNCTION__ );
      return;
   }

   if( clan->filename.empty(  ) )
   {
      bug( "%s: %s has no filename", __FUNCTION__, clan->name.c_str(  ) );
      return;
   }

   snprintf( filename, 256, "%s%s", CLAN_DIR, clan->filename.c_str(  ) );

   if( !( fp = fopen( filename, "w" ) ) )
   {
      bug( "%s: fopen", __FUNCTION__ );
      perror( filename );
   }
   else
   {
      fprintf( fp, "%s", "#CLAN\n" );
      fprintf( fp, "Version      %d\n", CLAN_VERSION );
      fprintf( fp, "Name         %s~\n", clan->name.c_str(  ) );
      fprintf( fp, "Filename     %s~\n", clan->filename.c_str(  ) );
      fprintf( fp, "Motto        %s~\n", clan->motto.c_str(  ) );
      fprintf( fp, "Description  %s~\n", clan->clandesc.c_str(  ) );
      fprintf( fp, "Deity        %s~\n", clan->deity.c_str(  ) );
      fprintf( fp, "Leader       %s~\n", clan->leader.c_str(  ) );
      fprintf( fp, "NumberOne    %s~\n", clan->number1.c_str(  ) );
      fprintf( fp, "NumberTwo    %s~\n", clan->number2.c_str(  ) );
      fprintf( fp, "Badge        %s~\n", clan->badge.c_str(  ) );
      fprintf( fp, "Leadrank     %s~\n", clan->leadrank.c_str(  ) );
      fprintf( fp, "Onerank      %s~\n", clan->onerank.c_str(  ) );
      fprintf( fp, "Tworank      %s~\n", clan->tworank.c_str(  ) );
      fprintf( fp, "PKills       %d %d %d %d %d %d %d %d %d %d\n",
               clan->pkills[0], clan->pkills[1], clan->pkills[2],
               clan->pkills[3], clan->pkills[4], clan->pkills[5], clan->pkills[6], clan->pkills[7], clan->pkills[8], clan->pkills[9] );
      fprintf( fp, "PDeaths      %d %d %d %d %d %d %d %d %d %d\n",
               clan->pdeaths[0], clan->pdeaths[1], clan->pdeaths[2],
               clan->pdeaths[3], clan->pdeaths[4], clan->pdeaths[5], clan->pdeaths[6], clan->pdeaths[7], clan->pdeaths[8], clan->pdeaths[9] );
      fprintf( fp, "MKills       %d\n", clan->mkills );
      fprintf( fp, "MDeaths      %d\n", clan->mdeaths );
      fprintf( fp, "IllegalPK    %d\n", clan->illegal_pk );
      fprintf( fp, "Type         %d\n", clan->clan_type );
      fprintf( fp, "Class        %d\n", clan->Class );
      fprintf( fp, "Favour       %d\n", clan->favour );
      fprintf( fp, "Members      %d\n", clan->members );
      fprintf( fp, "MemLimit     %d\n", clan->mem_limit );
      fprintf( fp, "Alignment    %d\n", clan->alignment );
      fprintf( fp, "Board        %d\n", clan->board );
      fprintf( fp, "ClanObjOne   %d\n", clan->clanobj1 );
      fprintf( fp, "ClanObjTwo   %d\n", clan->clanobj2 );
      fprintf( fp, "ClanObjThree %d\n", clan->clanobj3 );
      fprintf( fp, "ClanObjFour  %d\n", clan->clanobj4 );
      fprintf( fp, "ClanObjFive  %d\n", clan->clanobj5 );
      fprintf( fp, "Recall       %d\n", clan->recall );
      fprintf( fp, "Storeroom    %d\n", clan->storeroom );
      fprintf( fp, "GuardOne     %d\n", clan->guard1 );
      fprintf( fp, "GuardTwo     %d\n", clan->guard2 );
      fprintf( fp, "Tithe	   %d\n", clan->tithe );
      fprintf( fp, "Balance	   %d\n", clan->balance );
      fprintf( fp, "Idmob	   %d\n", clan->idmob );
      fprintf( fp, "Inn		   %d\n", clan->inn );
      fprintf( fp, "Shopkeeper   %d\n", clan->shopkeeper );
      fprintf( fp, "Auction	   %d\n", clan->auction );
      fprintf( fp, "Bank	   %d\n", clan->bank );
      fprintf( fp, "Repair	   %d\n", clan->repair );
      fprintf( fp, "Forge	   %d\n", clan->forge );
      fprintf( fp, "%s", "End\n\n" );
      fwrite_memberlist( fp, clan );
      fprintf( fp, "%s", "#END\n" );
   }
   FCLOSE( fp );
}

/*
 * Read in actual clan data.
 */

/*
 * Reads in PKill and PDeath still for backward compatibility but now it
 * should be written to PKillRange and PDeathRange for multiple level pkill
 * tracking support. --Shaddai
 *
 * Redone properly by Samson, breaks compatibility, but hey. It had to be done.
 *
 * Added a hardcoded limit memlimit to the amount of members a clan can 
 * have set using setclan.  --Shaddai
 */
void fread_clan( clan_data * clan, FILE * fp )
{
   int file_ver = 0;

   for( ;; )
   {
      const char *word = feof( fp ) ? "End" : fread_word( fp );

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
            KEY( "Alignment", clan->alignment, fread_short( fp ) );
            KEY( "Auction", clan->auction, fread_number( fp ) );
            break;

         case 'B':
            STDSKEY( "Badge", clan->badge );
            KEY( "Board", clan->board, fread_number( fp ) );
            KEY( "Balance", clan->balance, fread_number( fp ) );
            KEY( "Bank", clan->bank, fread_number( fp ) );
            break;

         case 'C':
            KEY( "ClanObjOne", clan->clanobj1, fread_number( fp ) );
            KEY( "ClanObjTwo", clan->clanobj2, fread_number( fp ) );
            KEY( "ClanObjThree", clan->clanobj3, fread_number( fp ) );
            KEY( "ClanObjFour", clan->clanobj4, fread_number( fp ) );
            KEY( "ClanObjFive", clan->clanobj5, fread_number( fp ) );
            KEY( "Class", clan->Class, fread_short( fp ) );
            break;

         case 'D':
            STDSKEY( "Deity", clan->deity );
            STDSKEY( "Description", clan->clandesc );
            break;

         case 'E':
            if( !str_cmp( word, "End" ) )
               return;
            break;

         case 'F':
            KEY( "Favour", clan->favour, fread_short( fp ) );
            STDSKEY( "Filename", clan->filename );
            KEY( "Forge", clan->forge, fread_number( fp ) );
            break;

         case 'G':
            KEY( "GuardOne", clan->guard1, fread_number( fp ) );
            KEY( "GuardTwo", clan->guard2, fread_number( fp ) );
            break;

         case 'I':
            KEY( "Idmob", clan->idmob, fread_number( fp ) );
            KEY( "IllegalPK", clan->illegal_pk, fread_number( fp ) );
            KEY( "Inn", clan->inn, fread_number( fp ) );
            break;

         case 'L':
            STDSKEY( "Leader", clan->leader );
            STDSKEY( "Leadrank", clan->leadrank );
            break;

         case 'M':
            KEY( "MDeaths", clan->mdeaths, fread_number( fp ) );
            KEY( "Members", clan->members, fread_short( fp ) );
            KEY( "MemLimit", clan->mem_limit, fread_short( fp ) );
            KEY( "MKills", clan->mkills, fread_number( fp ) );
            STDSKEY( "Motto", clan->motto );
            break;

         case 'N':
            STDSKEY( "Name", clan->name );
            STDSKEY( "NumberOne", clan->number1 );
            STDSKEY( "NumberTwo", clan->number2 );
            break;

         case 'O':
            STDSKEY( "Onerank", clan->onerank );
            break;

         case 'P':
            if( !str_cmp( word, "PDeaths" ) )
            {
               const char *ln;
               int x1, x2, x3, x4, x5, x6, x7, x8, x9, x10;

               ln = fread_line( fp );
               x1 = x2 = x3 = x4 = x5 = x6 = x7 = x8 = x9 = x10 = 0;

               sscanf( ln, "%d %d %d %d %d %d %d %d %d %d", &x1, &x2, &x3, &x4, &x5, &x6, &x7, &x8, &x9, &x10 );
               clan->pdeaths[0] = x1;
               clan->pdeaths[1] = x2;
               clan->pdeaths[2] = x3;
               clan->pdeaths[3] = x4;
               clan->pdeaths[4] = x5;
               clan->pdeaths[5] = x6;
               clan->pdeaths[6] = x7;
               clan->pdeaths[7] = x8;
               clan->pdeaths[8] = x9;
               clan->pdeaths[9] = x10;
               break;
            }

            if( !str_cmp( word, "PKills" ) )
            {
               const char *ln;
               int x1, x2, x3, x4, x5, x6, x7, x8, x9, x10;

               ln = fread_line( fp );
               x1 = x2 = x3 = x4 = x5 = x6 = x7 = x8 = x9 = x10 = 0;

               sscanf( ln, "%d %d %d %d %d %d %d %d %d %d", &x1, &x2, &x3, &x4, &x5, &x6, &x7, &x8, &x9, &x10 );
               clan->pkills[0] = x1;
               clan->pkills[1] = x2;
               clan->pkills[2] = x3;
               clan->pkills[3] = x4;
               clan->pkills[4] = x5;
               clan->pkills[5] = x6;
               clan->pkills[6] = x7;
               clan->pkills[7] = x8;
               clan->pkills[8] = x9;
               clan->pkills[9] = x10;
               break;
            }
            break;

         case 'R':
            KEY( "Recall", clan->recall, fread_number( fp ) );
            KEY( "Repair", clan->repair, fread_number( fp ) );
            break;

         case 'S':
            KEY( "Storeroom", clan->storeroom, fread_number( fp ) );
            KEY( "Shopkeeper", clan->shopkeeper, fread_number( fp ) );
            break;

         case 'T':
            KEY( "Tithe", clan->tithe, fread_short( fp ) );
            STDSKEY( "Tworank", clan->tworank );
            if( !str_cmp( word, "Type" ) )
            {
               short type = fread_short( fp );

               if( file_ver < 1 )
               {
                  if( type > CLAN_GUILD )
                     clan->clan_type = CLAN_GUILD;
                  else
                     clan->clan_type = CLAN_CLAN;
               }
               else
                  clan->clan_type = type;
               break;
            }
            break;

         case 'V':
            KEY( "Version", file_ver, fread_number( fp ) );
            break;
      }
   }
}

/* Sets up a bunch of default values for new clans or during loadup so we don't get wierd stuff - Samson 7-16-00 */
void clean_clan( clan_data * clan )
{
   clan->memberlist.clear(  );
   clan->getleader = false;
   clan->getone = false;
   clan->gettwo = false;
   clan->members = 1;
   clan->mem_limit = 15;
   clan->mkills = 0;
   clan->mdeaths = 0;
   clan->illegal_pk = 0;
   clan->clan_type = CLAN_GUILD;
   clan->favour = 0;
   clan->alignment = 0;
   clan->recall = 0;
   clan->storeroom = 0;
   clan->guard1 = 0;
   clan->guard2 = 0;
   clan->tithe = 0;
   clan->balance = 0;
   clan->inn = 0;
   clan->idmob = 0;
   clan->shopkeeper = 0;
   clan->auction = 0;
   clan->bank = 0;
   clan->repair = 0;
   clan->forge = 0;
   clan->pkills[0] = 0;
   clan->pkills[1] = 0;
   clan->pkills[2] = 0;
   clan->pkills[3] = 0;
   clan->pkills[4] = 0;
   clan->pkills[5] = 0;
   clan->pkills[6] = 0;
   clan->pkills[7] = 0;
   clan->pkills[8] = 0;
   clan->pkills[9] = 0;
   clan->pdeaths[0] = 0;
   clan->pdeaths[1] = 0;
   clan->pdeaths[2] = 0;
   clan->pdeaths[3] = 0;
   clan->pdeaths[4] = 0;
   clan->pdeaths[5] = 0;
   clan->pdeaths[6] = 0;
   clan->pdeaths[7] = 0;
   clan->pdeaths[8] = 0;
   clan->pdeaths[9] = 0;
}

/*
 * Load a clan file
 */
bool load_clan_file( const char *clanfile )
{
   char filename[256];
   clan_data *clan;
   FILE *fp;
   bool found;

   clan = new clan_data;

   clean_clan( clan );  /* Default settings so we don't get wierd ass stuff */

   found = false;
   snprintf( filename, 256, "%s%s", CLAN_DIR, clanfile );

   if( ( fp = fopen( filename, "r" ) ) != NULL )
   {
      found = true;
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
         if( !str_cmp( word, "CLAN" ) )
            fread_clan( clan, fp );
         else if( !str_cmp( word, "ROSTER" ) )
            fread_memberlist( clan, fp );
         else if( !str_cmp( word, "END" ) )
            break;
         else
         {
            bug( "%s: bad section: %s.", __FUNCTION__, word );
            break;
         }
      }
      FCLOSE( fp );
   }

   if( found )
   {
      room_index *storeroom;

      clanlist.push_back( clan );

      if( clan->storeroom == 0 || ( storeroom = get_room_index( clan->storeroom ) ) == NULL )
      {
         log_string( "Storeroom not found" );
         return found;
      }
      snprintf( filename, 256, "%s%s.vault", CLAN_DIR, clan->filename.c_str(  ) );
      if( ( fp = fopen( filename, "r" ) ) != NULL )
      {
         log_string( "Loading clan storage room" );
         rset_supermob( storeroom );

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
               bug( "%s: # not found. %s", __FUNCTION__, clan->name.c_str(  ) );
               break;
            }

            word = fread_word( fp );
            if( !str_cmp( word, "OBJECT" ) ) /* Objects  */
            {
               supermob->tempnum = -9999;
               fread_obj( supermob, fp, OS_CARRY );
            }
            else if( !str_cmp( word, "END" ) )  /* Done     */
               break;
            else
            {
               bug( "%s: %s bad section.", __FUNCTION__, clan->name.c_str(  ) );
               break;
            }
         }
         FCLOSE( fp );

         list < obj_data * >::iterator iobj;
         for( iobj = supermob->carrying.begin(  ); iobj != supermob->carrying.end(  ); )
         {
            obj_data *tobj = *iobj;
            ++iobj;

            tobj->from_char(  );
            if( tobj->ego >= sysdata->minego )
               tobj->extract(  );
            else
               tobj->to_room( storeroom, supermob );
         }
         release_supermob(  );
      }
      else
         log_string( "Cannot open clan vault" );
   }
   else
      deleteptr( clan );

   return found;
}

void verify_clans( void )
{
   list < clan_data * >::iterator iclan;
   list < roster_data * >::iterator member;
   bool change = false;

   log_string( "Cleaning up clan data..." );
   for( iclan = clanlist.begin(  ); iclan != clanlist.end(  ); )
   {
      clan_data *clan = *iclan;
      ++iclan;

      if( clan->leader.empty(  ) && clan->number1.empty(  ) && clan->number2.empty(  ) )
      {
         delete_clan( NULL, clan );
         continue;
      }

      if( clan->members < 1 )
      {
         delete_clan( NULL, clan );
         continue;
      }

      if( clan->members < 3 )
      {
         if( clan->members == 2 && !clan->number2.empty(  ) )
         {
            remove_roster( clan, clan->number2 );
            clan->number2.clear(  );
            save_clan( clan );
         }
         if( clan->members == 1 && !clan->number1.empty(  ) && !clan->number2.empty(  ) )
         {
            remove_roster( clan, clan->number1 );
            remove_roster( clan, clan->number2 );
            clan->number1.clear(  );
            clan->number2.clear(  );
            save_clan( clan );
         }
      }

      if( !exists_player( clan->leader ) )
      {
         if( !exists_player( clan->number1 ) )
         {
            if( !exists_player( clan->number2 ) )
            {
               remove_roster( clan, clan->leader );
               remove_roster( clan, clan->number1 );
               remove_roster( clan, clan->number2 );
               clan->leader.clear(  );
               clan->number1.clear(  );
               clan->number2.clear(  );
               clan->getleader = true;
               clan->getone = true;
               clan->gettwo = true;
               save_clan( clan );
               continue;
            }
         }
      }

      change = false;
      log_printf( "Checking data for %s.....", clan->name.c_str(  ) );

      if( !exists_player( clan->leader ) )
      {
         remove_roster( clan, clan->leader );
         clan->leader.clear(  );
         if( exists_player( clan->number1 ) )
         {
            change = true;
            clan->leader = clan->number1;
            clan->number1.clear(  );
            if( exists_player( clan->number2 ) )
            {
               clan->number1 = clan->number2;
               clan->number2.clear(  );
               clan->gettwo = true;
            }
            else
            {
               remove_roster( clan, clan->number2 );
               clan->number2.clear(  );
               clan->getone = true;
               clan->gettwo = true;
            }
         }
         else if( exists_player( clan->number2 ) )
         {
            remove_roster( clan, clan->number1 );
            change = true;
            clan->leader = clan->number2;
            clan->number1.clear(  );
            clan->number2.clear(  );
            clan->getone = true;
            clan->gettwo = true;
         }
         else
            clan->getleader = true;
      }

      if( !exists_player( clan->number1 ) )
      {
         remove_roster( clan, clan->number1 );
         clan->number1.clear(  );
         if( exists_player( clan->number2 ) )
         {
            change = true;
            clan->number1 = clan->number2;
            clan->number2.clear(  );
            clan->gettwo = true;
         }
         else
         {
            remove_roster( clan, clan->number2 );
            clan->number2.clear(  );
            clan->getone = true;
            clan->gettwo = true;
         }
      }

      if( !exists_player( clan->number2 ) )
      {
         remove_roster( clan, clan->number2 );
         clan->number2.clear(  );
         clan->gettwo = true;
      }
      if( change == true )
         log_printf( "Administration data for %s has changed.", clan->name.c_str(  ) );

      for( member = clan->memberlist.begin(  ); member != clan->memberlist.end(  ); )
      {
         roster_data *roster = *member;
         ++member;

         if( !exists_player( roster->name ) )
         {
            log_printf( "%s removed from roster. Player no longer exists.", roster->name.c_str(  ) );
            remove_roster( clan, roster->name );
         }
      }
      save_clan( clan );
   }
   write_clan_list(  );
}

/*
 * Load in all the clan files.
 */
void load_clans( void )
{
   FILE *fpList;
   const char *filename;
   char clanlistfile[256];

   clanlist.clear(  );

   log_string( "Loading clans..." );

   snprintf( clanlistfile, 256, "%s%s", CLAN_DIR, CLAN_LIST );

   if( !( fpList = fopen( clanlistfile, "r" ) ) )
   {
      perror( clanlistfile );
      exit( 1 );
   }

   for( ;; )
   {
      filename = feof( fpList ) ? "$" : fread_word( fpList );
      log_string( filename );
      if( filename[0] == '$' )
         break;

      if( !load_clan_file( filename ) )
         bug( "%s: Cannot load clan file: %s", __FUNCTION__, filename );
   }
   FCLOSE( fpList );
   verify_clans(  ); /* Check against pfiles to see if clans should still exist */
   log_string( "Done clans" );
}

void check_clan_info( char_data * ch )
{
   clan_data *clan;

   if( ch->isnpc(  ) )
      return;

   if( !ch->pcdata->clan )
      return;

   clan = ch->pcdata->clan;

   if( clan->getleader == true && str_cmp( ch->name, clan->number1 ) && str_cmp( ch->name, clan->number2 ) )
   {
      clan->leader = ch->name;
      clan->getleader = false;
      save_clan( clan );
      ch->printf( "Your %s's leader no longer existed. You have been made the new leader.\r\n", clan->clan_type == CLAN_GUILD ? "guild" : "clan" );
      if( clan->getone == true || clan->gettwo == true )
      {
         ch->
            printf
            ( "Other admins of your %s are also missing, it is advised that you pick new ones to avoid\r\nhaving them chosen for you.\r\n",
              clan->clan_type == CLAN_GUILD ? "guild" : "clan" );
      }
   }

   if( clan->getone == true && str_cmp( ch->name, clan->leader ) && str_cmp( ch->name, clan->number2 ) )
   {
      clan->number1 = ch->name;
      clan->getone = false;
      save_clan( clan );
      ch->printf( "Your %s's first officer no longer existed. You have been made the new first officer.\r\n", clan->clan_type == CLAN_GUILD ? "guild" : "clan" );
   }

   if( clan->gettwo == true && str_cmp( ch->name, clan->leader ) && str_cmp( ch->name, clan->number1 ) )
   {
      clan->number2 = ch->name;
      clan->gettwo = false;
      save_clan( clan );
      ch->printf( "Your %s's second officer no longer existed. You have been made the new second officer.\r\n", clan->clan_type == CLAN_GUILD ? "guild" : "clan" );
   }
}

CMDF( do_make )
{
   string arg;
   obj_index *pObjIndex;
   obj_data *obj;
   clan_data *clan;
   int level;

   if( ch->isnpc(  ) || !ch->pcdata->clan )
   {
      ch->print( "Huh?\r\n" );
      return;
   }

   clan = ch->pcdata->clan;

   if( str_cmp( ch->name, clan->leader ) && str_cmp( ch->name, clan->deity ) && str_cmp( ch->name, clan->number1 ) )
   {
      ch->print( "Huh?\r\n" );
      return;
   }

   argument = one_argument( argument, arg );

   if( arg.empty(  ) )
   {
      ch->print( "Make what?\r\n" );
      return;
   }

   pObjIndex = get_obj_index( clan->clanobj1 );
   level = 40;

   if( !pObjIndex || !hasname( pObjIndex->name, arg ) )
   {
      pObjIndex = get_obj_index( clan->clanobj2 );
      level = 45;
   }
   if( !pObjIndex || !hasname( pObjIndex->name, arg ) )
   {
      pObjIndex = get_obj_index( clan->clanobj3 );
      level = 50;
   }
   if( !pObjIndex || !hasname( pObjIndex->name, arg ) )
   {
      pObjIndex = get_obj_index( clan->clanobj4 );
      level = 35;
   }
   if( !pObjIndex || !hasname( pObjIndex->name, arg ) )
   {
      pObjIndex = get_obj_index( clan->clanobj5 );
      level = 1;
   }
   if( !pObjIndex || !hasname( pObjIndex->name, arg ) )
   {
      ch->print( "You don't know how to make that.\r\n" );
      return;
   }

   if( !( obj = pObjIndex->create_object( level ) ) )
   {
      log_printf( "create_object: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
      return;
   }
   obj->extra_flags.set( ITEM_CLANOBJECT );
   if( obj->wear_flags.test( ITEM_TAKE ) )
      obj = obj->to_char( ch );
   else
      obj = obj->to_room( ch->in_room, ch );
   act( AT_MAGIC, "$n makes $p!", ch, obj, NULL, TO_ROOM );
   act( AT_MAGIC, "You make $p!", ch, obj, NULL, TO_CHAR );
}

CMDF( do_induct )
{
   char_data *victim;
   clan_data *clan;

   if( ch->isnpc(  ) || !ch->pcdata->clan )
   {
      ch->print( "Huh?\r\n" );
      return;
   }

   clan = ch->pcdata->clan;

   if( ( !ch->pcdata->bestowments.empty(  ) && hasname( ch->pcdata->bestowments, "induct" ) )
       || !str_cmp( ch->name, clan->leader ) || !str_cmp( ch->name, clan->number1 ) || !str_cmp( ch->name, clan->number2 ) )
      ;
   else
   {
      ch->print( "Huh?\r\n" );
      return;
   }

   if( argument.empty(  ) )
   {
      ch->print( "Induct whom?\r\n" );
      return;
   }

   if( !( victim = ch->get_char_room( argument ) ) )
   {
      ch->print( "That player is not here.\r\n" );
      return;
   }

   if( victim->isnpc(  ) )
   {
      ch->print( "Not on NPC's.\r\n" );
      return;
   }

   if( victim->is_immortal(  ) )
   {
      ch->print( "You can't induct such a godly presence.\r\n" );
      return;
   }

   if( victim->level < 10 )
   {
      ch->print( "This player is not worthy of joining yet.\r\n" );
      return;
   }

   if( victim->pcdata->clan )
   {
      if( victim->pcdata->clan->clan_type == CLAN_GUILD )
      {
         if( victim->pcdata->clan == clan )
            ch->print( "This player already belongs to your guild!\r\n" );
         else
            ch->print( "This player already belongs to a guild!\r\n" );
         return;
      }
      else
      {
         if( victim->pcdata->clan == clan )
            ch->print( "This player already belongs to your clan!\r\n" );
         else
            ch->print( "This player already belongs to a clan!\r\n" );
         return;
      }
   }
   if( clan->mem_limit && clan->members >= clan->mem_limit )
   {
      ch->print( "Your clan is too big to induct anymore players.\r\n" );
      return;
   }
   ++clan->members;
   if( clan->clan_type != CLAN_GUILD )
      victim->set_lang( LANG_CLAN );

   if( clan->clan_type != CLAN_GUILD )
      victim->set_pcflag( PCFLAG_DEADLY );

   if( clan->clan_type != CLAN_GUILD )
   {
      int sn;

      for( sn = 0; sn < num_skills; ++sn )
      {
         if( skill_table[sn]->guild == clan->Class && skill_table[sn]->name != NULL )
         {
            victim->pcdata->learned[sn] = victim->GET_ADEPT( sn );
            victim->printf( "%s instructs you in the ways of %s.\r\n", ch->name, skill_table[sn]->name );
         }
      }
   }

   victim->pcdata->clan = clan;
   victim->pcdata->clan_name = clan->name;
   act( AT_MAGIC, "You induct $N into $t", ch, clan->name.c_str(  ), victim, TO_CHAR );
   act( AT_MAGIC, "$n inducts $N into $t", ch, clan->name.c_str(  ), victim, TO_NOTVICT );
   act( AT_MAGIC, "$n inducts you into $t", ch, clan->name.c_str(  ), victim, TO_VICT );
   add_roster( clan, victim->name, victim->Class, victim->level, victim->pcdata->mkills, victim->pcdata->mdeaths );
   victim->save(  );
   save_clan( clan );
}

/* Can the character outcast the victim? */
bool can_outcast( clan_data * clan, char_data * ch, char_data * victim )
{
   if( !clan || !ch || !victim )
      return false;
   if( !str_cmp( ch->name, clan->leader ) )
      return true;
   if( !str_cmp( victim->name, clan->leader ) )
      return false;
   if( !str_cmp( ch->name, clan->number1 ) )
      return true;
   if( !str_cmp( victim->name, clan->number1 ) )
      return false;
   if( !str_cmp( ch->name, clan->number2 ) )
      return true;
   if( !str_cmp( victim->name, clan->number2 ) )
      return false;
   return true;
}

CMDF( do_outcast )
{
   char_data *victim;
   clan_data *clan;

   if( ch->isnpc(  ) || !ch->pcdata->clan )
   {
      ch->print( "Huh?\r\n" );
      return;
   }

   clan = ch->pcdata->clan;

   if( ( !ch->pcdata->bestowments.empty(  ) && hasname( ch->pcdata->bestowments, "outcast" ) )
       || !str_cmp( ch->name, clan->leader ) || !str_cmp( ch->name, clan->number1 ) || !str_cmp( ch->name, clan->number2 ) )
      ;
   else
   {
      ch->print( "Huh?\r\n" );
      return;
   }

   if( argument.empty(  ) )
   {
      ch->print( "Outcast whom?\r\n" );
      return;
   }

   if( !( victim = ch->get_char_room( argument ) ) )
   {
      ch->print( "That player is not here.\r\n" );
      return;
   }

   if( victim->isnpc(  ) )
   {
      ch->print( "Not on NPC's.\r\n" );
      return;
   }

   if( victim == ch )
   {
      if( ch->pcdata->clan->clan_type == CLAN_GUILD )
      {
         ch->print( "Kick yourself out of your own guild?\r\n" );
         return;
      }
      else
      {
         ch->print( "Kick yourself out of your own clan?\r\n" );
         return;
      }
   }

   if( victim->pcdata->clan != ch->pcdata->clan )
   {
      if( ch->pcdata->clan->clan_type == CLAN_GUILD )
      {
         ch->print( "This player does not belong to your guild!\r\n" );
         return;
      }
      else
      {
         ch->print( "This player does not belong to your clan!\r\n" );
         return;
      }
   }

   if( !can_outcast( clan, ch, victim ) )
   {
      ch->print( "You are not able to outcast them.\r\n" );
      return;
   }

   if( clan->clan_type != CLAN_GUILD )
   {
      int sn;

      for( sn = 0; sn < num_skills; ++sn )
         if( skill_table[sn]->guild == victim->pcdata->clan->Class && skill_table[sn]->name != NULL )
         {
            victim->pcdata->learned[sn] = 0;
            victim->printf( "You forget the ways of %s.\r\n", skill_table[sn]->name );
         }
   }

   if( victim->speaking == LANG_CLAN )
      victim->speaking = LANG_COMMON;
   victim->unset_lang( LANG_CLAN );
   --clan->members;
   if( clan->members < 0 )
      clan->members = 0;
   if( !str_cmp( victim->name, ch->pcdata->clan->number1 ) )
      ch->pcdata->clan->number1.clear(  );
   if( !str_cmp( victim->name, ch->pcdata->clan->number2 ) )
      ch->pcdata->clan->number2.clear(  );
   if( !str_cmp( victim->name, ch->pcdata->clan->deity ) )
      ch->pcdata->clan->deity.clear(  );
   victim->pcdata->clan = NULL;
   victim->pcdata->clan_name.clear(  );
   act( AT_MAGIC, "You outcast $N from $t", ch, clan->name.c_str(  ), victim, TO_CHAR );
   act( AT_MAGIC, "$n outcasts $N from $t", ch, clan->name.c_str(  ), victim, TO_ROOM );
   act( AT_MAGIC, "$n outcasts you from $t", ch, clan->name.c_str(  ), victim, TO_VICT );

   echo_all_printf( ECHOTAR_ALL, "&[guildtalk]%s has been outcast from %s!", victim->name, clan->name.c_str(  ) );
   remove_roster( clan, victim->name );
   victim->save(  ); /* clan gets saved when pfile is saved */
   save_clan( clan );
}

CMDF( do_setclan );

/* Subfunction of setclan for clan leaders and first officers - Samson 12-6-98 */
void pcsetclan( char_data * ch, string argument )
{
   string arg1;
   clan_data *clan;

   argument = one_argument( argument, arg1 );

   clan = get_clan( ch->pcdata->clan_name );
   if( !clan )
   {
      ch->print( "But you are not a clan member?!?!?.\r\n" );
      return;
   }

   if( str_cmp( ch->name, clan->leader ) && str_cmp( ch->name, clan->number1 ) )
   {
      ch->print( "Only the clan leader or first officer can change clan information.\r\n" );
      return;
   }

   if( arg1.empty(  ) )
   {
      ch->print( "Usage: setclan <field> <player/text>\r\n" );
      ch->print( "\r\nField being one of:\r\n" );
      ch->print( "number2 " );
      if( !str_cmp( ch->name, clan->leader ) )
      {
         ch->print( "disband deity leader number1\r\n" );
         ch->print( "leadrank onerank tworank\r\n" );
         ch->print( "motto desc badge tithe\r\n" );
      }
      return;
   }

   if( !str_cmp( arg1, "number2" ) )
   {
      if( !exists_player( argument ) )
      {
         ch->print( "That person does not exist!\r\n" );
         return;
      }
      clan->number2 = argument;
      clan->gettwo = false;
      ch->print( "Done.\r\n" );
      save_clan( clan );
      return;
   }

   if( str_cmp( ch->name, clan->leader ) )
   {
      do_setclan( ch, "" );
      return;
   }

   if( !str_cmp( arg1, "disband" ) )
   {
      if( argument.empty(  ) )
      {
         ch->print( "This requires confirmation!!!\r\n" );
         ch->print( "If you're sure you want to disband, type 'setclan disband yes'.\r\n" );
         ch->print( "Weigh your decision CAREFULLY.\r\nDisbanding will permanently destroy ALL data related to your clan or guild!\r\n" );
         ch->print( "This includes any shops, auction houses, storerooms etc you may own in a guildhall.\r\n" );
         ch->print( "It is STRONGLY adivsed that you recover any goods left in such places BEFORE you disband.\r\n" );
         ch->print( "Reimbursements for lost goods will NOT be made.\r\n" );
         return;
      }

      if( str_cmp( argument, "yes" ) )
      {
         do_setclan( ch, "disband" );
         return;
      }

      ch->pcdata->clan_name.clear(  );
      ch->pcdata->clan = NULL;

      list < char_data * >::iterator ich;
      for( ich = pclist.begin(  ); ich != pclist.end(  ); ++ich )
      {
         char_data *vch = *ich;

         if( vch->pcdata->clan == clan )
         {
            vch->pcdata->clan_name.clear(  );
            vch->pcdata->clan = NULL;
         }
      }
      echo_all_printf( ECHOTAR_ALL, "&[guildtalk]%s has dissolved %s!", ch->name, clan->name.c_str(  ) );
      log_printf( "%s has dissolved %s", ch->name, clan->name.c_str(  ) );
      delete_clan( ch, clan );
      write_clan_list(  );
      return;
   }

   if( !str_cmp( arg1, "leadrank" ) )
   {
      clan->leadrank = argument;
      ch->print( "Done.\r\n" );
      save_clan( clan );
      return;
   }

   if( !str_cmp( arg1, "onerank" ) )
   {
      clan->onerank = argument;
      ch->print( "Done.\r\n" );
      save_clan( clan );
      return;
   }

   if( !str_cmp( arg1, "tworank" ) )
   {
      clan->tworank = argument;
      ch->print( "Done.\r\n" );
      save_clan( clan );
      return;
   }

   if( !str_cmp( arg1, "deity" ) )
   {
      clan->deity = argument;
      ch->print( "Done.\r\n" );
      save_clan( clan );
      return;
   }

   if( !str_cmp( arg1, "leader" ) )
   {
      if( !exists_player( argument ) )
      {
         ch->print( "That person does not exist!\r\n" );
         return;
      }
      clan->leader = argument;
      clan->getleader = false;
      ch->print( "Done.\r\n" );
      save_clan( clan );
      return;
   }

   if( !str_cmp( arg1, "number1" ) )
   {
      if( !exists_player( argument ) )
      {
         ch->print( "That person does not exist!\r\n" );
         return;
      }
      clan->number1 = argument;
      clan->getone = false;
      ch->print( "Done.\r\n" );
      save_clan( clan );
      return;
   }

   if( !str_cmp( arg1, "badge" ) )
   {
      clan->badge = argument;
      ch->print( "Done.\r\n" );
      save_clan( clan );
      return;
   }

   if( !str_cmp( arg1, "motto" ) )
   {
      clan->motto = argument;
      ch->print( "Done.\r\n" );
      save_clan( clan );
      return;
   }

   if( !str_cmp( arg1, "desc" ) )
   {
      clan->clandesc = argument;
      ch->print( "Done.\r\n" );
      save_clan( clan );
      return;
   }

   if( !str_cmp( arg1, "tithe" ) )
   {
      int value = atoi( argument.c_str(  ) );

      if( value < 0 || value > 100 )
      {
         ch->print( "Invalid tithe - range is from 0 to 100.\r\n" );
         return;
      }
      clan->tithe = value;
      ch->print( "Done.\r\n" );
      save_clan( clan );
      return;
   }

   do_setclan( ch, "" );
}

CMDF( do_setclan )
{
   string arg1, arg2;
   clan_data *clan;

   ch->set_color( AT_PLAIN );
   if( ch->isnpc(  ) )
   {
      ch->print( "Huh?\r\n" );
      return;
   }

   if( !ch->is_immortal(  ) )
   {
      pcsetclan( ch, argument );
      return;
   }

   if( !ch->is_imp(  ) )
   {
      ch->print( "Only implementors can use this command.\r\n" );
      return;
   }

   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );
   if( arg1.empty(  ) || arg2.empty(  ) )
   {
      ch->print( "Usage: setclan <clan> <field> <deity|leader|number1|number2> <player>\r\n" );
      ch->print( "\r\nField being one of:\r\n" );
      ch->print( " deity leader number1 number2\r\n" );
      ch->print( " align leadrank onerank tworank\r\n" );
      ch->print( " board recall storage guard1 guard2\r\n" );
      ch->print( " obj1 obj2 obj3 obj4 obj5\r\n" );
      ch->print( " name motto desc\r\n" );
      ch->print( " favour type class tithe\r\n" );
      ch->print( " balance inn idmob shopkeeper bank repair forge\r\n" );
      ch->print( " filename members memlimit delete\r\n" );
      ch->print( " pkill1-7 pdeath1-7\r\n" );
      return;
   }

   clan = get_clan( arg1 );
   if( !clan )
   {
      ch->print( "No such clan.\r\n" );
      return;
   }

   if( !str_cmp( arg2, "deity" ) )
   {
      clan->deity = argument;
      ch->print( "Done.\r\n" );
      save_clan( clan );
      return;
   }

   if( !str_cmp( arg2, "leadrank" ) )
   {
      clan->leadrank = argument;
      ch->print( "Done.\r\n" );
      save_clan( clan );
      return;
   }

   if( !str_cmp( arg2, "onerank" ) )
   {
      clan->onerank = argument;
      ch->print( "Done.\r\n" );
      save_clan( clan );
      return;
   }

   if( !str_cmp( arg2, "tworank" ) )
   {
      clan->tworank = argument;
      ch->print( "Done.\r\n" );
      save_clan( clan );
      return;
   }

   if( !str_cmp( arg2, "leader" ) )
   {
      clan->leader = argument;
      clan->getleader = false;
      ch->print( "Done.\r\n" );
      save_clan( clan );
      return;
   }

   if( !str_cmp( arg2, "number1" ) )
   {
      clan->number1 = argument;
      clan->getone = false;
      ch->print( "Done.\r\n" );
      save_clan( clan );
      return;
   }

   if( !str_cmp( arg2, "number2" ) )
   {
      clan->number2 = argument;
      clan->gettwo = false;
      ch->print( "Done.\r\n" );
      save_clan( clan );
      return;
   }

   if( !str_cmp( arg2, "badge" ) )
   {
      clan->badge = argument;
      ch->print( "Done.\r\n" );
      save_clan( clan );
      return;
   }

   if( !str_cmp( arg2, "board" ) )
   {
      clan->board = atoi( argument.c_str(  ) );
      ch->print( "Done.\r\n" );
      save_clan( clan );
      return;
   }

   if( !str_cmp( arg2, "obj1" ) )
   {
      clan->clanobj1 = atoi( argument.c_str(  ) );
      ch->print( "Done.\r\n" );
      save_clan( clan );
      return;
   }

   if( !str_cmp( arg2, "obj2" ) )
   {
      clan->clanobj2 = atoi( argument.c_str(  ) );
      ch->print( "Done.\r\n" );
      save_clan( clan );
      return;
   }

   if( !str_cmp( arg2, "obj3" ) )
   {
      clan->clanobj3 = atoi( argument.c_str(  ) );
      ch->print( "Done.\r\n" );
      save_clan( clan );
      return;
   }

   if( !str_cmp( arg2, "obj4" ) )
   {
      clan->clanobj4 = atoi( argument.c_str(  ) );
      ch->print( "Done.\r\n" );
      save_clan( clan );
      return;
   }

   if( !str_cmp( arg2, "obj5" ) )
   {
      clan->clanobj5 = atoi( argument.c_str(  ) );
      ch->print( "Done.\r\n" );
      save_clan( clan );
      return;
   }

   if( !str_cmp( arg2, "guard1" ) )
   {
      clan->guard1 = atoi( argument.c_str(  ) );
      ch->print( "Done.\r\n" );
      save_clan( clan );
      return;
   }

   if( !str_cmp( arg2, "guard2" ) )
   {
      clan->guard2 = atoi( argument.c_str(  ) );
      ch->print( "Done.\r\n" );
      save_clan( clan );
      return;
   }

   if( !str_cmp( arg2, "recall" ) )
   {
      clan->recall = atoi( argument.c_str(  ) );
      ch->print( "Done.\r\n" );
      save_clan( clan );
      return;
   }

   if( !str_cmp( arg2, "storage" ) )
   {
      clan->storeroom = atoi( argument.c_str(  ) );
      ch->print( "Done.\r\n" );
      save_clan( clan );
      return;
   }

   if( !str_cmp( arg2, "tithe" ) )
   {
      int value = atoi( argument.c_str(  ) );

      if( value < 0 || value > 100 )
      {
         ch->print( "Invalid tithe - range is from 0 to 100.\r\n" );
         return;
      }
      clan->tithe = value;
      ch->print( "Done.\r\n" );
      save_clan( clan );
      return;
   }

   if( !str_cmp( arg2, "balance" ) )
   {
      int value = atoi( argument.c_str(  ) );

      if( value < 0 || value > 2000000000 )
      {
         ch->print( "Invalid balance - range is from 0 to 2 billion.\r\n" );
         return;
      }
      clan->balance = value;
      ch->print( "Done.\r\n" );
      save_clan( clan );
      return;
   }

   if( !str_cmp( arg2, "shopkeeper" ) )
   {
      mob_index *mob = NULL;
      int value = 0;

      if( atoi( argument.c_str(  ) ) != 0 )
      {
         if( !( mob = get_mob_index( atoi( argument.c_str(  ) ) ) ) )
         {
            ch->print( "That mobile does not exist.\r\n" );
            return;
         }
         value = mob->vnum;
      }
      clan->shopkeeper = value;
      ch->print( "Done.\r\n" );
      save_clan( clan );
      return;
   }

   if( !str_cmp( arg2, "idmob" ) )
   {
      mob_index *mob = NULL;
      int value = 0;

      if( atoi( argument.c_str(  ) ) != 0 )
      {
         if( !( mob = get_mob_index( atoi( argument.c_str(  ) ) ) ) )
         {
            ch->print( "That mobile does not exist.\r\n" );
            return;
         }
         value = mob->vnum;
      }
      clan->idmob = value;
      ch->print( "Done.\r\n" );
      save_clan( clan );
      return;
   }

   if( !str_cmp( arg2, "repair" ) )
   {
      mob_index *mob = NULL;
      int value = 0;

      if( atoi( argument.c_str(  ) ) != 0 )
      {
         if( !( mob = get_mob_index( atoi( argument.c_str(  ) ) ) ) )
         {
            ch->print( "That mobile does not exist.\r\n" );
            return;
         }
         value = mob->vnum;
      }
      clan->repair = value;
      ch->print( "Done.\r\n" );
      save_clan( clan );
      return;
   }

   if( !str_cmp( arg2, "forge" ) )
   {
      mob_index *mob = NULL;
      int value = 0;

      if( atoi( argument.c_str(  ) ) != 0 )
      {
         if( !( mob = get_mob_index( atoi( argument.c_str(  ) ) ) ) )
         {
            ch->print( "That mobile does not exist.\r\n" );
            return;
         }
         value = mob->vnum;
      }
      clan->forge = value;
      ch->print( "Done.\r\n" );
      save_clan( clan );
      return;
   }

   if( !str_cmp( arg2, "bank" ) )
   {
      mob_index *mob = NULL;
      int value = 0;

      if( atoi( argument.c_str(  ) ) != 0 )
      {
         if( !( mob = get_mob_index( atoi( argument.c_str(  ) ) ) ) )
         {
            ch->print( "That mobile does not exist.\r\n" );
            return;
         }
         value = mob->vnum;
      }
      clan->bank = value;
      ch->print( "Done.\r\n" );
      save_clan( clan );
      return;
   }

   if( !str_cmp( arg2, "inn" ) )
   {
      room_index *room = NULL;
      int value = 0;

      if( atoi( argument.c_str(  ) ) != 0 )
      {
         if( !( room = get_room_index( atoi( argument.c_str(  ) ) ) ) )
         {
            ch->print( "That room does not exist.\r\n" );
            return;
         }
         value = room->vnum;
      }
      clan->inn = value;
      ch->print( "Done.\r\n" );
      save_clan( clan );
      return;
   }

   if( !str_cmp( arg2, "align" ) )
   {
      int value = atoi( argument.c_str(  ) );

      if( value < -1000 || value > 1000 )
      {
         ch->print( "Invalid alignment - range is from -1000 to 1000.\r\n" );
         return;
      }
      clan->alignment = value;
      ch->print( "Done.\r\n" );
      save_clan( clan );
      return;
   }

   if( !str_cmp( arg2, "type" ) )
   {
      if( !str_cmp( argument, "guild" ) )
         clan->clan_type = CLAN_GUILD;
      else
         clan->clan_type = CLAN_CLAN;
      ch->print( "Done.\r\n" );
      save_clan( clan );
      return;
   }

   if( !str_cmp( arg2, "class" ) )
   {
      clan->Class = atoi( argument.c_str(  ) );
      ch->print( "Done.\r\n" );
      save_clan( clan );
      return;
   }

   if( !str_cmp( arg2, "name" ) )
   {
      clan_data *uclan = NULL;

      if( argument.empty(  ) )
      {
         ch->print( "You can't name a clan nothing.\r\n" );
         return;
      }
      if( ( uclan = get_clan( argument ) ) )
      {
         ch->print( "There is already another clan with that name.\r\n" );
         return;
      }
      clan->name = argument;
      ch->print( "Done.\r\n" );
      save_clan( clan );
      return;
   }

   if( !str_cmp( arg2, "motto" ) )
   {
      clan->motto = argument;
      ch->print( "Done.\r\n" );
      save_clan( clan );
      return;
   }

   if( !str_cmp( arg2, "desc" ) )
   {
      clan->clandesc = argument;
      ch->print( "Done.\r\n" );
      save_clan( clan );
      return;
   }

   if( !str_cmp( arg2, "memlimit" ) )
   {
      clan->mem_limit = atoi( argument.c_str(  ) );
      ch->print( "Done.\r\n" );
      save_clan( clan );
      return;
   }

   if( !str_cmp( arg2, "members" ) )
   {
      clan->members = atoi( argument.c_str(  ) );
      ch->print( "Done.\r\n" );
      save_clan( clan );
      return;
   }

   if( !str_cmp( arg2, "filename" ) )
   {
      char filename[256];

      if( !is_valid_filename( ch, CLAN_DIR, argument ) )
         return;

      snprintf( filename, sizeof( filename ), "%s%s", CLAN_DIR, clan->filename.c_str(  ) );
      if( !remove( filename ) )
         ch->print( "Old clan file deleted.\r\n" );

      clan->filename = argument;
      ch->print( "Done.\r\n" );
      save_clan( clan );
      write_clan_list(  );
      return;
   }

   if( !str_cmp( arg2, "delete" ) )
   {
      delete_clan( ch, clan );
      write_clan_list(  );
      return;
   }

   if( !str_prefix( "pkill", arg2 ) )
   {
      int temp_value;

      if( !str_cmp( arg2, "pkill1" ) )
         temp_value = 0;
      else if( !str_cmp( arg2, "pkill2" ) )
         temp_value = 1;
      else if( !str_cmp( arg2, "pkill3" ) )
         temp_value = 2;
      else if( !str_cmp( arg2, "pkill4" ) )
         temp_value = 3;
      else if( !str_cmp( arg2, "pkill5" ) )
         temp_value = 4;
      else if( !str_cmp( arg2, "pkill6" ) )
         temp_value = 5;
      else if( !str_cmp( arg2, "pkill7" ) )
         temp_value = 6;
      else
      {
         do_setclan( ch, "" );
         return;
      }
      clan->pkills[temp_value] = atoi( argument.c_str(  ) );
      ch->print( "Ok.\r\n" );
      return;
   }

   if( !str_prefix( "pdeath", arg2 ) )
   {
      int temp_value;

      if( !str_cmp( arg2, "pdeath1" ) )
         temp_value = 0;
      else if( !str_cmp( arg2, "pdeath2" ) )
         temp_value = 1;
      else if( !str_cmp( arg2, "pdeath3" ) )
         temp_value = 2;
      else if( !str_cmp( arg2, "pdeath4" ) )
         temp_value = 3;
      else if( !str_cmp( arg2, "pdeath5" ) )
         temp_value = 4;
      else if( !str_cmp( arg2, "pdeath6" ) )
         temp_value = 5;
      else if( !str_cmp( arg2, "pdeath7" ) )
         temp_value = 6;
      else
      {
         do_setclan( ch, "" );
         return;
      }
      clan->pdeaths[temp_value] = atoi( argument.c_str(  ) );
      ch->print( "Ok.\r\n" );
      return;
   }
   do_setclan( ch, "" );
}

CMDF( do_showclan )
{
   clan_data *clan;

   ch->set_color( AT_PLAIN );

   if( ch->isnpc(  ) )
   {
      ch->print( "Huh?\r\n" );
      return;
   }

   if( argument.empty(  ) )
   {
      ch->print( "Usage: showclan <clan>\r\n" );
      return;
   }

   clan = get_clan( argument );
   if( !clan )
   {
      ch->print( "No such clan, or guild.\r\n" );
      return;
   }

   ch->printf( "\r\n&w%s     : &W%s\t\tBadge: %s\r\n&wFilename : &W%s\r\n&wMotto    : &W%s\r\n",
               clan->clan_type == CLAN_GUILD ? "Guild" : "Clan",
               clan->name.c_str(  ), !clan->badge.empty(  )? clan->badge.c_str(  ) : "(not set)", clan->filename.c_str(  ), clan->motto.c_str(  ) );
   ch->printf( "&wDesc     : &W%s\r\n&wDeity    : &W%s\r\n", clan->clandesc.c_str(  ), clan->deity.c_str(  ) );
   ch->printf( "&wLeader   : &W%-19.19s\t&wRank: &W%s\r\n", clan->leader.c_str(  ), clan->leadrank.c_str(  ) );
   ch->printf( "&wNumber1  : &W%-19.19s\t&wRank: &W%s\r\n", clan->number1.c_str(  ), clan->onerank.c_str(  ) );
   ch->printf( "&wNumber2  : &W%-19.19s\t&wRank: &W%s\r\n", clan->number2.c_str(  ), clan->tworank.c_str(  ) );
   ch->printf( "&wBalance  : &W%d\r\n", clan->balance );
   ch->printf( "&wTithe    : &W%d\r\n", clan->tithe );
   ch->
      printf
      ( "&wPKills   : &w1-9:&W%-3d &w10-14:&W%-3d &w15-19:&W%-3d &w20-29:&W%-3d &w30-39:&W%-3d &w40-49:&W%-3d &w50:&W%-3d\r\n",
        clan->pkills[0], clan->pkills[1], clan->pkills[2], clan->pkills[3], clan->pkills[4], clan->pkills[5], clan->pkills[6] );
   ch->
      printf
      ( "&wPDeaths  : &w1-9:&W%-3d &w10-14:&W%-3d &w15-19:&W%-3d &w20-29:&W%-3d &w30-39:&W%-3d &w40-49:&W%-3d &w50:&W%-3d\r\n",
        clan->pdeaths[0], clan->pdeaths[1], clan->pdeaths[2], clan->pdeaths[3], clan->pdeaths[4], clan->pdeaths[5], clan->pdeaths[6] );
   ch->printf( "&wIllegalPK: &W%-6d\r\n", clan->illegal_pk );
   ch->printf( "&wMKills   : &W%-6d   &wMDeaths: &W%-6d\r\n", clan->mkills, clan->mdeaths );
   ch->printf( "&wFavor    : &W%-6d   &w\r\n", clan->favour );
   ch->printf( "&wMembers  : &W%-6d   &wMemLimit  : &W%-6d   &wAlign  : &W%-6d\r\n", clan->members, clan->mem_limit, clan->alignment );
   ch->printf( "&wBoard    : &W%-5d    &wRecall : &W%-5d    &wStorage: &W%-5d\r\n", clan->board, clan->recall, clan->storeroom );
   ch->printf( "&wGuard1   : &W%-5d    &wGuard2 : &W%-5d    &wBanker : &W%-5d\r\n", clan->guard1, clan->guard2, clan->bank );
   ch->printf( "&wInn      : &W%-5d    &wShop   : &W%-5d    &wAuction: &W%-5d\r\n", clan->inn, clan->shopkeeper, clan->auction );
   ch->printf( "&wRepair   : &W%-5d    &wForge  : &W%-5d    &wIdmob  : &W%-5d\r\n", clan->repair, clan->forge, clan->idmob );
   ch->printf( "&wObj1( &W%d &w)  Obj2( &W%d &w)  Obj3( &W%d &w)  Obj4( &W%d &w)  Obj5( &W%d &w)\r\n",
               clan->clanobj1, clan->clanobj2, clan->clanobj3, clan->clanobj4, clan->clanobj5 );
}

CMDF( do_makeclan )
{
   string arg, arg2;
   char filename[256];
   clan_data *clan;
   roster_data *member;
   char_data *victim;

   ch->set_color( AT_IMMORT );

   argument = one_argument( argument, arg );
   argument = one_argument( argument, arg2 );

   if( arg.empty(  ) || arg2.empty(  ) || argument.empty(  ) )
   {
      ch->print( "Usage: makeclan <filename> <clan leader> <clan name>\r\n" );
      ch->print( "Filename must be a SINGLE word, no punctuation marks.\r\n" );
      ch->print( "The clan leader must be present online to form a new clan.\r\n" );
      return;
   }

   if( !( victim = ch->get_char_world( arg2 ) ) )
   {
      ch->print( "That player is not online.\r\n" );
      return;
   }

   if( victim->isnpc(  ) )
   {
      ch->print( "An NPC cannot lead a clan!\r\n" );
      return;
   }

   clan = get_clan( argument );
   if( clan )
   {
      ch->print( "There is already a clan with that name.\r\n" );
      return;
   }

   snprintf( filename, 256, "%s%s", CLAN_DIR, strlower( arg.c_str(  ) ) );

   clan = new clan_data;
   clean_clan( clan );  /* Setup default values - Samson 7-16-00 */

   clan->name = argument;
   clan->filename = filename; /* Bug fix - Samson 5-31-99 */
   clan->leader = victim->name;

   member = new roster_data;
   member->name = victim->name;
   member->joined = current_time;
   member->Class = victim->Class;
   member->kills = victim->pcdata->mkills;
   member->deaths = victim->pcdata->mdeaths;
   clan->memberlist.push_back( member );
   clanlist.push_back( clan );

   /*
    * Samson 12-6-98 
    */
   victim->pcdata->clan = clan;
   victim->pcdata->clan_name = clan->name;

   victim->save(  );
   save_clan( victim->pcdata->clan );
   write_clan_list(  );

   ch->printf( "Clan %s created with leader %s.\r\n", clan->name.c_str(  ), clan->leader.c_str(  ) );
}

CMDF( do_roster )
{
   clan_data *clan;
   list < roster_data * >::iterator mem;
   string arg, arg2;
   int total = 0;

   if( ch->isnpc(  ) )
   {
      ch->print( "NPCs can't use this command.\r\n" );
      return;
   }

   if( argument.empty(  ) )
   {
      ch->print( "Usage: roster <clanname>\r\n" );
      ch->print( "Usage: roster <clanname> remove <name>\r\n" );
      return;
   }

   argument = one_argument( argument, arg );
   if( !( clan = get_clan( arg ) ) )
   {
      ch->printf( "No such guild or clan known as %s\r\n", arg.c_str(  ) );
      return;
   }

   if( argument.empty(  ) )
   {
      ch->printf( "Membership roster for the %s %s\r\n\r\n", clan->name.c_str(  ), clan->clan_type == CLAN_GUILD ? "Guild" : "Clan" );
      ch->printf( "%-15.15s  %-15.15s %-6.6s %-6.6s %-6.6s %s\r\n", "Name", "Class", "Level", "Kills", "Deaths", "Joined on" );
      ch->print( "-------------------------------------------------------------------------------------\r\n" );
      for( mem = clan->memberlist.begin(  ); mem != clan->memberlist.end(  ); ++mem )
      {
         roster_data *member = *mem;

         ch->printf( "%-15.15s  %-15.15s %-6d %-6d %-6d %s\r\n",
                     member->name.c_str(  ), capitalize( npc_class[member->Class] ), member->level, member->kills, member->deaths,
                     c_time( member->joined, ch->pcdata->timezone ) );
         ++total;
      }
      ch->printf( "\r\nThere are %d member%s in %s\r\n", total, total == 1 ? "" : "s", clan->name.c_str(  ) );
      return;
   }

   argument = one_argument( argument, arg2 );
   if( !str_cmp( arg2, "remove" ) )
   {
      if( argument.empty(  ) )
      {
         ch->print( "Remove who from the roster?\r\n" );
         return;
      }
      remove_roster( clan, argument );
      save_clan( clan );
      ch->printf( "%s has been removed from the roster for %s\r\n", argument.c_str(  ), clan->name.c_str(  ) );
      return;
   }
   do_roster( ch, "" );
}

/*
 * Added multiple level pkill and pdeath support. --Shaddai
 */
CMDF( do_clans )
{
   clan_data *pclan;
   list < clan_data * >::iterator cl;
   int count = 0;

   if( argument.empty(  ) )
   {
      ch->
         print
         ( "\r\n&RClan          Deity         Leader           Pkills:    Avatar      Other\r\n_________________________________________________________________________\r\n\r\n" );
      for( cl = clanlist.begin(  ); cl != clanlist.end(  ); ++cl )
      {
         clan_data *clan = *cl;

         if( clan->clan_type == CLAN_GUILD )
            continue;
         ch->printf( "&w%-13s %-13s %-13s", clan->name.c_str(  ), clan->deity.c_str(  ), clan->leader.c_str(  ) );
         ch->printf( "&[blood]                %5d      %5d\r\n", clan->pkills[6], ( clan->pkills[2] + clan->pkills[3] + clan->pkills[4] + clan->pkills[5] ) );
         ++count;
      }
      if( !count )
         ch->print( "&RThere are no Clans currently formed.\r\n" );
      else
         ch->
            print
            ( "&R_________________________________________________________________________\r\n\r\nUse 'clans <clan>' for detailed information and a breakdown of victories.\r\n" );
      return;
   }

   pclan = get_clan( argument );
   if( !pclan || pclan->clan_type == CLAN_GUILD )
   {
      ch->print( "&RNo such clan.\r\n" );
      return;
   }
   ch->printf( "\r\n&R%s, '%s'\r\n\r\n", pclan->name.c_str(  ), pclan->motto.c_str(  ) );
   ch->print( "&wVictories:&w\r\n" );
   ch->printf( "    &w15-19...  &r%-4d\r\n    &w20-29...  &r%-4d\r\n    &w30-39...  &r%-4d\r\n    &w40-49...  &r%-4d\r\n",
               pclan->pkills[2], pclan->pkills[3], pclan->pkills[4], pclan->pkills[5] );
   ch->printf( "   &wAvatar...  &r%-4d\r\n", pclan->pkills[6] );
   ch->printf( "&wClan Leader:  %s\r\nNumber One :  %s\r\nNumber Two :  %s\r\nClan Deity :  %s\r\n",
               pclan->leader.c_str(  ), pclan->number1.c_str(  ), pclan->number2.c_str(  ), pclan->deity.c_str(  ) );
   if( !str_cmp( ch->name, pclan->deity ) || !str_cmp( ch->name, pclan->leader ) || !str_cmp( ch->name, pclan->number1 ) || !str_cmp( ch->name, pclan->number2 ) )
      ch->printf( "Members    :  %d\r\n", pclan->members );
   ch->printf( "\r\n&RDescription:  %s\r\n", pclan->clandesc.c_str(  ) );
}

CMDF( do_guilds )
{
   clan_data *porder;
   list < clan_data * >::iterator cl;
   int count = 0;

   if( argument.empty(  ) )
   {
      ch->
         print( "\r\n&gGuild            Deity          Leader           Mkills      Mdeaths\r\n____________________________________________________________________\r\n\r\n" );
      for( cl = clanlist.begin(  ); cl != clanlist.end(  ); ++cl )
      {
         clan_data *clan = ( *cl );

         if( clan->clan_type == CLAN_GUILD )
         {
            ch->printf( "&G%-16s %-14s %-14s   %-7d       %5d\r\n", clan->name.c_str(  ), clan->deity.c_str(  ), clan->leader.c_str(  ), clan->mkills, clan->mdeaths );
            ++count;
         }
      }
      if( !count )
         ch->print( "&gThere are no Guilds currently formed.\r\n" );
      else
         ch->print( "&g____________________________________________________________________\r\n\r\nUse 'guilds <guild>' for more detailed information.\r\n" );
      return;
   }

   porder = get_clan( argument );
   if( !porder || porder->clan_type != CLAN_GUILD )
   {
      ch->print( "&gNo such Guild.\r\n" );
      return;
   }

   ch->printf( "\r\n&gGuild of %s\r\n'%s'\r\n\r\n", porder->name.c_str(  ), porder->motto.c_str(  ) );
   ch->printf( "&GDeity      :  %s\r\nLeader     :  %s\r\nNumber One :  %s\r\nNumber Two :  %s\r\n",
               porder->deity.c_str(  ), porder->leader.c_str(  ), porder->number1.c_str(  ), porder->number2.c_str(  ) );
   if( !str_cmp( ch->name, porder->deity ) || !str_cmp( ch->name, porder->leader ) || !str_cmp( ch->name, porder->number1 ) || !str_cmp( ch->name, porder->number2 ) )
      ch->printf( "Members    :  %d\r\n", porder->members );
   ch->printf( "\r\n&gDescription:\r\n%s\r\n", porder->clandesc.c_str(  ) );
}

CMDF( do_victories )
{
   char filename[256];

   if( ch->isnpc(  ) || !ch->pcdata->clan )
   {
      ch->print( "Huh?\r\n" );
      return;
   }

   if( ch->pcdata->clan->clan_type != CLAN_GUILD )
   {
      snprintf( filename, 256, "%s%s.record", CLAN_DIR, ch->pcdata->clan->name.c_str(  ) );
      if( !str_cmp( ch->name, ch->pcdata->clan->leader ) && !str_cmp( argument, "clean" ) )
      {
         FILE *fp = fopen( filename, "w" );
         if( fp )
            FCLOSE( fp );
         ch->print( "\r\nVictories ledger has been cleared.\r\n" );
         return;
      }
      else
      {
         ch->pager( "\r\nLVL  Character       LVL  Character\r\n" );
         show_file( ch, filename );
         return;
      }
   }
   else
      ch->print( "Huh?\r\n" );
}

CMDF( do_ident )
{
   if( ch->isnpc(  ) )
   {
      ch->print( "Mobs cannot use this command.\r\n" );
      return;
   }

   bool idmob = false;
   char_data *mob = NULL;
   list < char_data * >::iterator ich;
   for( ich = ch->in_room->people.begin(  ); ich != ch->in_room->people.end(  ); ++ich )
   {
      mob = *ich;
      if( mob->isnpc(  ) && ( mob->has_actflag( ACT_IDMOB ) || mob->has_actflag( ACT_GUILDIDMOB ) ) )
      {
         idmob = true;
         break;
      }
   }

   if( !idmob )
   {
      ch->print( "You must go to someone who will identify your items to use this command.\r\n" );
      return;
   }

   if( argument.empty(  ) )
   {
      act( AT_TELL, "$n tells you 'What would you like identified?'\r\n", mob, NULL, ch, TO_VICT );
      return;
   }

   obj_data *obj;
   if( !( obj = ch->get_obj_carry( argument ) ) )
   {
      act( AT_TELL, "$n tells you 'You don't have that item.'", mob, NULL, ch, TO_VICT );
      return;
   }

   bool found = false;
   clan_data *clan = NULL;
   if( mob->has_actflag( ACT_GUILDIDMOB ) )
   {
      list < clan_data * >::iterator cl;
      for( cl = clanlist.begin(  ); cl != clanlist.end(  ); ++cl )
      {
         clan = ( *cl );
         if( mob->pIndexData->vnum == clan->idmob )
         {
            found = true;
            break;
         }
      }
   }

   if( found && clan == ch->pcdata->clan )
      ;
   else
   {
      double idcost = 5000 + ( obj->cost * 0.1 );

      if( ch->gold - idcost < 0 )
      {
         act( AT_TELL, "$n tells you 'You cannot afford to identify that!'", mob, NULL, ch, TO_VICT );
         return;
      }
      act_printf( AT_TELL, mob, NULL, ch, TO_VICT, "$n charges you %d gold for the identification.", idcost );
      ch->gold -= ( int )idcost;
      if( found && clan->bank )
      {
         clan->balance += ( int )idcost;
         save_clan( clan );
      }
   }

   act_printf( AT_LBLUE, mob, NULL, ch, TO_VICT, "$n tells you 'Information on a %s:'", obj->short_descr );
   obj_identify_output( ch, obj );
}

CMDF( do_shove )
{
   string arg, arg2;
   int exit_dir;
   exit_data *pexit;
   char_data *victim;
   bool nogo;
   room_index *to_room;
   int schance = 0;

   argument = one_argument( argument, arg );
   argument = one_argument( argument, arg2 );

   if( !ch->has_pcflag( PCFLAG_DEADLY ) )
   {
      ch->print( "Only deadly characters can shove.\r\n" );
      return;
   }

   if( arg.empty(  ) )
   {
      ch->print( "Shove whom?\r\n" );
      return;
   }

   if( !( victim = ch->get_char_room( arg ) ) )
   {
      ch->print( "They aren't here.\r\n" );
      return;
   }

   if( victim == ch )
   {
      ch->print( "You shove yourself around, to no avail.\r\n" );
      return;
   }

   if( !victim->has_pcflag( PCFLAG_DEADLY ) )
   {
      ch->print( "You can only shove deadly characters.\r\n" );
      return;
   }

   if( ( victim->position ) != POS_STANDING )
   {
      act( AT_PLAIN, "$N isn't standing up.", ch, NULL, victim, TO_CHAR );
      return;
   }

   if( arg2.empty(  ) )
   {
      ch->print( "Shove them in which direction?\r\n" );
      return;
   }

   exit_dir = get_dir( arg2 );
   if( victim->in_room->flags.test( ROOM_SAFE ) && victim->get_timer( TIMER_SHOVEDRAG ) <= 0 )
   {
      ch->print( "That character cannot be shoved right now.\r\n" );
      return;
   }
   victim->position = POS_SHOVE;
   nogo = false;
   if( !( pexit = ch->in_room->get_exit( exit_dir ) ) )
      nogo = true;
   else if( IS_EXIT_FLAG( pexit, EX_CLOSED ) && ( !ch->has_aflag( AFF_PASS_DOOR ) || IS_EXIT_FLAG( pexit, EX_NOPASSDOOR ) ) && !ch->has_pcflag( PCFLAG_PASSDOOR ) )
      nogo = true;

   if( ( IS_EXIT_FLAG( pexit, EX_FORTIFIED )
         || IS_EXIT_FLAG( pexit, EX_HEAVY ) || IS_EXIT_FLAG( pexit, EX_MEDIUM ) || IS_EXIT_FLAG( pexit, EX_LIGHT ) || IS_EXIT_FLAG( pexit, EX_CRUMBLING ) ) )
      nogo = true;

   if( nogo )
   {
      ch->print( "There's no exit in that direction.\r\n" );
      victim->position = POS_STANDING;
      return;
   }
   to_room = pexit->to_room;
   if( to_room->flags.test( ROOM_DEATH ) )
   {
      ch->print( "You cannot shove someone into a death trap.\r\n" );
      victim->position = POS_STANDING;
      return;
   }

   if( ch->in_room->area != to_room->area && !victim->in_hard_range( to_room->area ) )
   {
      ch->print( "That character cannot enter that area.\r\n" );
      victim->position = POS_STANDING;
      return;
   }

   /*
    * Check for Class, assign percentage based on that. 
    */
   if( ch->Class == CLASS_WARRIOR )
      schance = 70;
   if( ch->Class == CLASS_RANGER )
      schance = 60;
   if( ch->Class == CLASS_DRUID )
      schance = 45;
   if( ch->Class == CLASS_CLERIC )
      schance = 35;
   if( ch->Class == CLASS_ROGUE )
      schance = 30;
   if( ch->Class == CLASS_MAGE )
      schance = 15;
   if( ch->Class == CLASS_MONK )
      schance = 80;
   if( ch->Class == CLASS_PALADIN )
      schance = 65;
   if( ch->Class == CLASS_ANTIPALADIN )
      schance = 65;
   if( ch->Class == CLASS_BARD )
      schance = 30;
   if( ch->Class == CLASS_NECROMANCER )
      schance = 15;

   /*
    * Add 3 points to chance for every str point above 15, subtract for below 15 
    */
   schance += ( ( ch->get_curr_str(  ) - 15 ) * 3 );
   schance += ( ch->level - victim->level );

   if( schance < number_percent(  ) )
   {
      ch->print( "You failed.\r\n" );
      victim->position = POS_STANDING;
      return;
   }
   act( AT_ACTION, "You shove $M.", ch, NULL, victim, TO_CHAR );
   act( AT_ACTION, "$n shoves you.", ch, NULL, victim, TO_VICT );
   move_char( victim, ch->in_room->get_exit( exit_dir ), 0, exit_dir, false );
   if( !victim->char_died(  ) )
      victim->position = POS_STANDING;
   ch->WAIT_STATE( 12 );
   /*
    * Remove protection from shove/drag if char shoves -- Blodkai 
    */
   if( ch->in_room->flags.test( ROOM_SAFE ) && ch->get_timer( TIMER_SHOVEDRAG ) <= 0 )
      ch->add_timer( TIMER_SHOVEDRAG, 10, NULL, 0 );
}

CMDF( do_drag )
{
   string arg, arg2;
   int exit_dir;
   char_data *victim;
   exit_data *pexit;
   room_index *to_room;
   bool nogo;
   int dchance = 0;

   argument = one_argument( argument, arg );
   argument = one_argument( argument, arg2 );

   if( ch->isnpc(  ) )
   {
      ch->print( "Only characters can drag.\r\n" );
      return;
   }

   if( arg.empty(  ) )
   {
      ch->print( "Drag whom?\r\n" );
      return;
   }

   if( !( victim = ch->get_char_room( arg ) ) )
   {
      ch->print( "They aren't here.\r\n" );
      return;
   }

   if( victim == ch )
   {
      ch->print( "You take yourself by the scruff of your neck, but go nowhere.\r\n" );
      return;
   }

   if( victim->isnpc(  ) )
   {
      ch->print( "You can only drag characters.\r\n" );
      return;
   }

   if( !victim->has_pcflag( PCFLAG_SHOVEDRAG ) && !victim->has_pcflag( PCFLAG_DEADLY ) )
   {
      ch->print( "That character doesn't seem to appreciate your attentions.\r\n" );
      return;
   }

   if( victim->fighting )
   {
      ch->print( "You try, but can't get close enough.\r\n" );
      return;
   }

   if( !ch->has_pcflag( PCFLAG_DEADLY ) && victim->has_pcflag( PCFLAG_DEADLY ) )
   {
      ch->print( "You cannot drag a deadly character.\r\n" );
      return;
   }

   if( !victim->has_pcflag( PCFLAG_DEADLY ) && victim->position > POS_STUNNED )
   {
      ch->print( "They don't seem to need your assistance.\r\n" );
      return;
   }

   if( arg2.empty(  ) )
   {
      ch->print( "Drag them in which direction?\r\n" );
      return;
   }

   exit_dir = get_dir( arg2 );

   if( victim->in_room->flags.test( ROOM_SAFE ) && victim->get_timer( TIMER_SHOVEDRAG ) <= 0 )
   {
      ch->print( "That character cannot be dragged right now.\r\n" );
      return;
   }

   nogo = false;
   if( !( pexit = ch->in_room->get_exit( exit_dir ) ) )
      nogo = true;
   else if( IS_EXIT_FLAG( pexit, EX_CLOSED ) && ( !ch->has_aflag( AFF_PASS_DOOR ) || IS_EXIT_FLAG( pexit, EX_NOPASSDOOR ) ) && !ch->has_pcflag( PCFLAG_PASSDOOR ) )
      nogo = true;

   if( ( IS_EXIT_FLAG( pexit, EX_FORTIFIED )
         || IS_EXIT_FLAG( pexit, EX_HEAVY ) || IS_EXIT_FLAG( pexit, EX_MEDIUM ) || IS_EXIT_FLAG( pexit, EX_LIGHT ) || IS_EXIT_FLAG( pexit, EX_CRUMBLING ) ) )
      nogo = true;

   if( nogo )
   {
      ch->print( "There's no exit in that direction.\r\n" );
      return;
   }

   to_room = pexit->to_room;
   if( to_room->flags.test( ROOM_DEATH ) )
   {
      ch->print( "You cannot drag someone into a death trap.\r\n" );
      return;
   }

   if( ch->in_room->area != to_room->area && !victim->in_hard_range( to_room->area ) )
   {
      ch->print( "That character cannot enter that area.\r\n" );
      victim->position = POS_STANDING;
      return;
   }

   /*
    * Check for Class, assign percentage based on that. 
    */
   if( ch->Class == CLASS_WARRIOR )
      dchance = 70;
   if( ch->Class == CLASS_RANGER )
      dchance = 60;
   if( ch->Class == CLASS_DRUID )
      dchance = 45;
   if( ch->Class == CLASS_CLERIC )
      dchance = 35;
   if( ch->Class == CLASS_ROGUE )
      dchance = 30;
   if( ch->Class == CLASS_MAGE )
      dchance = 15;
   if( ch->Class == CLASS_MONK )
      dchance = 80;
   if( ch->Class == CLASS_PALADIN )
      dchance = 65;
   if( ch->Class == CLASS_ANTIPALADIN )
      dchance = 65;
   if( ch->Class == CLASS_BARD )
      dchance = 30;
   if( ch->Class == CLASS_NECROMANCER )
      dchance = 15;

   /*
    * Add 3 points to chance for every str point above 15, subtract for below 15 
    */

   dchance += ( ( ch->get_curr_str(  ) - 15 ) * 3 );

   dchance += ( ch->level - victim->level );

   if( dchance < number_percent(  ) )
   {
      ch->print( "You failed.\r\n" );
      victim->position = POS_STANDING;
      return;
   }
   if( victim->position < POS_STANDING )
   {
      short temp;

      temp = victim->position;
      victim->position = POS_DRAG;
      act( AT_ACTION, "You drag $M into the next room.", ch, NULL, victim, TO_CHAR );
      act( AT_ACTION, "$n grabs your hair and drags you.", ch, NULL, victim, TO_VICT );
      move_char( victim, ch->in_room->get_exit( exit_dir ), 0, exit_dir, false );
      if( !victim->char_died(  ) )
         victim->position = temp;
      /*
       * Move ch to the room too.. they are doing dragging - Scryn 
       */
      move_char( ch, ch->in_room->get_exit( exit_dir ), 0, exit_dir, false );
      ch->WAIT_STATE( 12 );
      return;
   }
   ch->print( "You cannot do that to someone who is standing.\r\n" );
}
