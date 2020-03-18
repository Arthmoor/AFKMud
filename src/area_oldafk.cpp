/****************************************************************************
 *                   ^     +----- |  / ^     ^ |     | +-\                  *
 *                  / \    |      | /  |\   /| |     | |  \                 *
 *                 /   \   +---   |<   | \ / | |     | |  |                 *
 *                /-----\  |      | \  |  v  | |     | |  /                 *
 *               /       \ |      |  \ |     | +-----+ +-/                  *
 ****************************************************************************
 * AFKMud Copyright 1997-2020 by Roger Libiez (Samson),                     *
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
 *                  Area Conversion Support for AFKMud 1.x                  *
 ****************************************************************************/

#if defined(WIN32)
#include <unistd.h>
#endif
#include "mud.h"
#include "area.h"
#include "mobindex.h"
#include "mud_prog.h"
#include "objindex.h"
#include "overland.h"
#include "roomindex.h"
#include "shops.h"

extern int top_prog;
extern int top_affect;
extern int top_shop;
extern int top_repair;

void save_sysdata(  );
int get_continent( const string & );
void validate_treasure_settings( area_data * );

/*
 * Load a mob section. Old style AFKMud area file.
 */
void load_mobiles( area_data * tarea, FILE * fp )
{
   mob_index *pMobIndex;
   int x1, x2, x3, x4, x5, x6, x7, value;
   string flag;

   if( !tarea )
   {
      bug( "%s: no #AREA seen yet.", __func__ );
      if( fBootDb )
      {
         shutdown_mud( "No #AREA" );
         exit( 1 );
      }
      else
         return;
   }

   for( ;; )
   {
      int vnum;
      char letter;
      bool oldmob, tmpBootDb;

      letter = fread_letter( fp );
      if( letter != '#' )
      {
         bug( "%s: # not found.", __func__ );
         if( fBootDb )
         {
            shutdown_mud( "# not found" );
            exit( 1 );
         }
         else
            return;
      }

      vnum = fread_number( fp );
      if( vnum == 0 )
         break;

      tmpBootDb = fBootDb;
      fBootDb = false;
      if( get_mob_index( vnum ) )
      {
         if( tmpBootDb )
         {
            bug( "%s: vnum %d duplicated.", __func__, vnum );
            shutdown_mud( "duplicate vnum" );
            exit( 1 );
         }
         else
         {
            pMobIndex = get_mob_index( vnum );
            log_printf_plus( LOG_BUILD, sysdata->build_level, "Cleaning mobile: %d", vnum );
            pMobIndex->clean_mob(  );
            oldmob = true;
         }
      }
      else
      {
         oldmob = false;
         pMobIndex = new mob_index;
         pMobIndex->clean_mob(  );
      }
      fBootDb = tmpBootDb;

      pMobIndex->vnum = vnum;
      if( fBootDb )
      {
         if( !tarea->low_vnum )
            tarea->low_vnum = vnum;
         if( vnum > tarea->hi_vnum )
            tarea->hi_vnum = vnum;
      }
      pMobIndex->area = tarea;
      pMobIndex->player_name = fread_string( fp );
      pMobIndex->short_descr = fread_string( fp );
      pMobIndex->long_descr = fread_string( fp );

      const char *desc = fread_flagstring( fp );
      if( desc && desc[0] != '\0' && str_cmp( desc, "(null)" ) )
      {
         pMobIndex->chardesc = STRALLOC( desc );
         if( str_prefix( "namegen", desc ) )
            pMobIndex->chardesc[0] = UPPER( pMobIndex->chardesc[0] );
      }

      if( pMobIndex->long_descr != nullptr && str_prefix( "namegen", pMobIndex->long_descr ) )
         pMobIndex->long_descr[0] = UPPER( pMobIndex->long_descr[0] );

      flag_set( fp, pMobIndex->actflags, act_flags );
      flag_set( fp, pMobIndex->affected_by, aff_flags );

      pMobIndex->actflags.set( ACT_IS_NPC );
      pMobIndex->pShop = nullptr;
      pMobIndex->rShop = nullptr;

      const char *ln = fread_line( fp );
      x1 = x2 = x3 = x4 = x5 = 0;
      x6 = 150;
      x7 = 100;
      float x8 = 0;
      sscanf( ln, "%d %d %d %d %d %d %d %f", &x1, &x2, &x3, &x4, &x5, &x6, &x7, &x8 );

      pMobIndex->alignment = x1;
      pMobIndex->gold = x2;
      pMobIndex->height = x4;
      pMobIndex->weight = x5;
      pMobIndex->max_move = x6;
      pMobIndex->max_mana = x7;
      pMobIndex->numattacks = x8;

      if( pMobIndex->max_move < 1 )
         pMobIndex->max_move = 150;

      if( pMobIndex->max_mana < 1 )
         pMobIndex->max_mana = 100;

      /*
       * To catch old area file mobs and force those with preset gold amounts to use the treasure generator instead 
       */
      if( tarea->version < 17 && pMobIndex->gold > 0 )
         pMobIndex->gold = -1;

      pMobIndex->level = fread_number( fp );
      pMobIndex->mobthac0 = fread_number( fp );
      pMobIndex->ac = fread_number( fp );
      pMobIndex->hitnodice = pMobIndex->level;
      pMobIndex->hitsizedice = 8;
      pMobIndex->hitplus = fread_number( fp );
      pMobIndex->damnodice = fread_number( fp );
      /*
       * 'd' 
       */
      fread_letter( fp );
      pMobIndex->damsizedice = fread_number( fp );
      /*
       * '+' 
       */
      fread_letter( fp );
      pMobIndex->damplus = fread_number( fp );

      flag_set( fp, pMobIndex->speaks, lang_names );

      string speaking = fread_flagstring( fp );

      speaking = one_argument( speaking, flag );
      value = get_langnum( flag );
      if( value < 0 || value >= LANG_UNKNOWN )
         bug( "Unknown speaking language: %s", flag.c_str() );
      else
         pMobIndex->speaking = value;

      if( pMobIndex->speaks.none(  ) )
         pMobIndex->speaks.set( LANG_COMMON );
      if( !pMobIndex->speaking )
         pMobIndex->speaking = LANG_COMMON;

      int position = get_npc_position( fread_flagstring( fp ) );
      if( position < 0 || position >= POS_MAX )
      {
         bug( "%s: vnum %d: Mobile in invalid position! Defaulting to standing.", __func__, vnum );
         position = POS_STANDING;
      }
      pMobIndex->position = position;

      position = get_npc_position( fread_flagstring( fp ) );
      if( position < 0 || position >= POS_MAX )
      {
         bug( "%s: vnum %d: Mobile in invalid default position! Defaulting to standing.", __func__, vnum );
         position = POS_STANDING;
      }
      pMobIndex->defposition = position;

      int sex = get_npc_sex( fread_flagstring( fp ) );
      if( sex < 0 || sex >= SEX_MAX )
      {
         bug( "%s: vnum %d: Mobile has invalid sex! Defaulting to neuter.", __func__, vnum );
         sex = SEX_NEUTRAL;
      }
      pMobIndex->sex = sex;

      int race = get_npc_race( fread_flagstring( fp ) );
      if( race < 0 || race >= MAX_NPC_RACE )
      {
         bug( "%s: vnum %d: Mob has invalid race! Defaulting to monster.", __func__, vnum );
         race = get_npc_race( "monster" );
      }
      pMobIndex->race = race;

      int Class = get_npc_class( fread_flagstring( fp ) );
      if( Class < 0 || Class >= MAX_NPC_CLASS )
      {
         bug( "%s: vnum %d: Mob has invalid Class! Defaulting to warrior.", __func__, vnum );
         Class = get_npc_class( "warrior" );
      }
      pMobIndex->Class = Class;

      flag_set( fp, pMobIndex->body_parts, part_flags );
      flag_set( fp, pMobIndex->resistant, ris_flags );
      flag_set( fp, pMobIndex->immune, ris_flags );
      flag_set( fp, pMobIndex->susceptible, ris_flags );
      flag_set( fp, pMobIndex->absorb, ris_flags );
      flag_set( fp, pMobIndex->attacks, attack_flags );
      flag_set( fp, pMobIndex->defenses, defense_flags );

      letter = fread_letter( fp );
      if( letter == '>' )
      {
         ungetc( letter, fp );
         pMobIndex->mprog_read_programs( fp );
      }
      else
         ungetc( letter, fp );

      if( !oldmob )
      {
         mob_index_table.insert( map < int, mob_index * >::value_type( pMobIndex->vnum, pMobIndex ) );
         tarea->mobs.push_back( pMobIndex );
         ++top_mob_index;
      }
   }
}

/*
 * Load an obj section. Old style AFKMud area file.
 */
void load_objects( area_data * tarea, FILE * fp )
{
   obj_index *pObjIndex;
   char letter;
   char temp[3][MSL];
   int x1, x2, x3, x4, x5, x6, x7, x8, x9, x10, x11;

   if( !tarea )
   {
      bug( "%s: no #AREA seen yet.", __func__ );
      if( fBootDb )
      {
         shutdown_mud( "No #AREA" );
         exit( 1 );
      }
      else
         return;
   }

   for( ;; )
   {
      int vnum, value;
      bool tmpBootDb, oldobj;

      letter = fread_letter( fp );
      if( letter != '#' )
      {
         bug( "%s: # not found.", __func__ );
         if( fBootDb )
         {
            shutdown_mud( "# not found" );
            exit( 1 );
         }
         else
            return;
      }

      vnum = fread_number( fp );
      if( vnum == 0 )
         break;

      tmpBootDb = fBootDb;
      fBootDb = false;
      if( get_obj_index( vnum ) )
      {
         if( tmpBootDb )
         {
            bug( "%s: vnum %d duplicated.", __func__, vnum );
            shutdown_mud( "duplicate vnum" );
            exit( 1 );
         }
         else
         {
            pObjIndex = get_obj_index( vnum );
            log_printf_plus( LOG_BUILD, sysdata->build_level, "Cleaning object: %d", vnum );
            pObjIndex->clean_obj(  );
            oldobj = true;
         }
      }
      else
      {
         oldobj = false;
         pObjIndex = new obj_index;
         pObjIndex->clean_obj(  );
      }
      fBootDb = tmpBootDb;

      pObjIndex->vnum = vnum;
      if( fBootDb )
      {
         if( !tarea->low_vnum )
            tarea->low_vnum = vnum;
         if( vnum > tarea->hi_vnum )
            tarea->hi_vnum = vnum;
      }
      pObjIndex->area = tarea;
      pObjIndex->name = fread_string( fp );
      pObjIndex->short_descr = fread_string( fp );

      const char *desc = fread_flagstring( fp );
      if( desc && desc[0] != '\0' && str_cmp( desc, "(null)" ) )
         pObjIndex->objdesc = STRALLOC( desc );

      const char *desc2 = fread_flagstring( fp );
      if( desc2 && desc2[0] != '\0' && str_cmp( desc2, "(null)" ) )
         pObjIndex->action_desc = STRALLOC( desc2 );

      if( pObjIndex->objdesc != nullptr )
         pObjIndex->objdesc[0] = UPPER( pObjIndex->objdesc[0] );

      value = get_otype( fread_flagstring( fp ) );
      if( value < 0 )
      {
         bug( "%s: vnum %d: Object has invalid type! Defaulting to trash.", __func__, vnum );
         value = get_otype( "trash" );
      }
      pObjIndex->item_type = value;

      flag_set( fp, pObjIndex->extra_flags, o_flags );
      flag_set( fp, pObjIndex->wear_flags, w_flags );

      // Magic Flags
      // These things were never used, but will leave this here to allow it to get ignored if it's been inserted.
      fread_flagstring( fp );

      const char *ln = fread_line( fp );
      x1 = x2 = x3 = x4 = x5 = x6 = x7 = x8 = x9 = x10 = x11 = 0;
      sscanf( ln, "%d %d %d %d %d %d %d %d %d %d %d", &x1, &x2, &x3, &x4, &x5, &x6, &x7, &x8, &x9, &x10, &x11 );

      if( x1 == 0 && ( pObjIndex->item_type == ITEM_WEAPON || pObjIndex->item_type == ITEM_MISSILE_WEAPON ) )
      {
         x1 = sysdata->initcond;
         x7 = x1;
      }

      if( x1 == 0 && pObjIndex->item_type == ITEM_PROJECTILE )
      {
         x1 = sysdata->initcond;
         x6 = x1;
      }

      pObjIndex->value[0] = x1;
      pObjIndex->value[1] = x2;
      pObjIndex->value[2] = x3;
      pObjIndex->value[3] = x4;
      pObjIndex->value[4] = x5;
      pObjIndex->value[5] = x6;
      pObjIndex->value[6] = x7;
      pObjIndex->value[7] = x8;
      pObjIndex->value[8] = x9;
      pObjIndex->value[9] = x10;
      pObjIndex->value[10] = x11;

      ln = fread_line( fp );
      x1 = x2 = x3 = x5 = 0;
      x4 = 9999;
      temp[0][0] = '\0';
      temp[1][0] = '\0';
      temp[2][0] = '\0';
      sscanf( ln, "%d %d %d %d %d %s %s %s", &x1, &x2, &x3, &x4, &x5, temp[0], temp[1], temp[2] );
      pObjIndex->weight = x1;
      pObjIndex->weight = UMAX( 1, pObjIndex->weight );
      pObjIndex->cost = x2;
      pObjIndex->ego = x3;
      pObjIndex->limit = x4;
      pObjIndex->layers = x5;

      if( !temp[0] || temp[0][0] == '\0' )
         pObjIndex->socket[0] = STRALLOC( "None" );
      else
         pObjIndex->socket[0] = STRALLOC( temp[0] );

      if( !temp[1] || temp[1][0] == '\0' )
         pObjIndex->socket[1] = STRALLOC( "None" );
      else
         pObjIndex->socket[1] = STRALLOC( temp[1] );

      if( !temp[2] || temp[2][0] == '\0' )
         pObjIndex->socket[2] = STRALLOC( "None" );
      else
         pObjIndex->socket[2] = STRALLOC( temp[2] );

      switch ( pObjIndex->item_type )
      {
         default:
            break;

         case ITEM_PILL:
         case ITEM_POTION:
         case ITEM_SCROLL:
            pObjIndex->value[1] = skill_lookup( fread_word( fp ) );
            pObjIndex->value[2] = skill_lookup( fread_word( fp ) );
            pObjIndex->value[3] = skill_lookup( fread_word( fp ) );
            break;

         case ITEM_STAFF:
         case ITEM_WAND:
            pObjIndex->value[3] = skill_lookup( fread_word( fp ) );
            break;

         case ITEM_SALVE:
            pObjIndex->value[4] = skill_lookup( fread_word( fp ) );
            pObjIndex->value[5] = skill_lookup( fread_word( fp ) );
            break;
      }

      for( ;; )
      {
         letter = fread_letter( fp );

         if( letter == 'A' )
         {
            affect_data *paf;
            bool setaff = true;

            paf = new affect_data;
            paf->location = APPLY_NONE;
            paf->type = -1;
            paf->duration = -1;
            paf->bit = 0;
            paf->modifier = 0;
            paf->rismod.reset(  );

            if( tarea->version < 20 )
            {
               char *aff = nullptr;
               char *risa = nullptr;
               char flag[MIL];

               paf->location = fread_number( fp );

               if( paf->location == APPLY_WEAPONSPELL
                   || paf->location == APPLY_WEARSPELL
                   || paf->location == APPLY_REMOVESPELL || paf->location == APPLY_STRIPSN || paf->location == APPLY_RECURRINGSPELL || paf->location == APPLY_EAT_SPELL )
                  paf->modifier = slot_lookup( fread_number( fp ) );
               else if( paf->location == APPLY_AFFECT )
               {
                  paf->modifier = fread_number( fp );
                  aff = flag_string( paf->modifier, aff_flags );
                  value = get_aflag( aff );
                  if( value < 0 || value >= MAX_AFFECTED_BY )
                  {
                     bug( "%s: Unsupportable value for affect flag: %s", __func__, aff );
                     setaff = false;
                  }
                  else
                  {
                     ++value;
                     paf->modifier = value;
                  }
               }
               else if( paf->location == APPLY_RESISTANT || paf->location == APPLY_IMMUNE || paf->location == APPLY_SUSCEPTIBLE || paf->location == APPLY_ABSORB )
               {
                  value = fread_number( fp );
                  risa = flag_string( value, ris_flags );

                  while( risa[0] != '\0' )
                  {
                     risa = one_argument( risa, flag );
                     value = get_risflag( flag );
                     if( value < 0 || value >= MAX_RIS_FLAG )
                        bug( "%s: Unsupportable value for RISA flag: %s", __func__, flag );
                     else
                        paf->rismod.set( value );
                  }
               }
               else
                  paf->modifier = fread_number( fp );
            }
            else
            {
               char *loc = nullptr;
               char *aff = nullptr;

               loc = fread_word( fp );
               value = get_atype( loc );
               if( value < 0 || value >= MAX_APPLY_TYPE )
               {
                  bug( "%s: Invalid apply type: %s", __func__, loc );
                  setaff = false;
               }
               paf->location = value;

               if( paf->location == APPLY_WEAPONSPELL
                   || paf->location == APPLY_WEARSPELL
                   || paf->location == APPLY_REMOVESPELL || paf->location == APPLY_STRIPSN || paf->location == APPLY_RECURRINGSPELL || paf->location == APPLY_EAT_SPELL )
                  paf->modifier = skill_lookup( fread_word( fp ) );
               else if( paf->location == APPLY_AFFECT )
               {
                  aff = fread_word( fp );
                  value = get_aflag( aff );
                  if( value < 0 || value >= MAX_AFFECTED_BY )
                  {
                     bug( "%s: Unsupportable value for affect flag: %s", __func__, aff );
                     setaff = false;
                  }
                  else
                     paf->modifier = value;
               }
               else if( paf->location == APPLY_RESISTANT || paf->location == APPLY_IMMUNE || paf->location == APPLY_SUSCEPTIBLE || paf->location == APPLY_ABSORB )
                  flag_set( fp, paf->rismod, ris_flags );
               else
                  paf->modifier = fread_number( fp );
            }

            if( !setaff )
               deleteptr( paf );
            else
            {
               pObjIndex->affects.push_back( paf );
               ++top_affect;
            }
         }

         else if( letter == 'E' )
         {
            extra_descr_data *ed = new extra_descr_data;

            fread_string( ed->keyword, fp );
            fread_string( ed->desc, fp );
            pObjIndex->extradesc.push_back( ed );
            ++top_ed;
         }
         else if( letter == '>' )
         {
            ungetc( letter, fp );
            pObjIndex->oprog_read_programs( fp );
         }
         else
         {
            ungetc( letter, fp );
            break;
         }
      }

      if( !oldobj )
      {
         obj_index_table.insert( map < int, obj_index * >::value_type( pObjIndex->vnum, pObjIndex ) );

         if( pObjIndex->ego > 90 )
            pObjIndex->ego = -2;
         tarea->objects.push_back( pObjIndex );
         ++top_obj_index;
      }
   }
}

/*
 * Load a reset section. This is maintained only for legacy areas.
 */
void load_resets( area_data * tarea, FILE * fp )
{
   room_index *pRoomIndex = nullptr;
   bool not01 = false;
   int count = 0;

   if( !tarea )
   {
      bug( "%s: no #AREA seen yet.", __func__ );
      if( fBootDb )
      {
         shutdown_mud( "No #AREA" );
         exit( 1 );
      }
      else
         return;
   }

   if( tarea->rooms.empty(  ) )
   {
      bug( "%s: No #ROOMS section found. Cannot load resets.", __func__ );
      if( fBootDb )
      {
         shutdown_mud( "No #ROOMS" );
         exit( 1 );
      }
      else
         return;
   }

   for( ;; )
   {
      exit_data *pexit;
      char letter;
      int extra, arg1, arg2, arg3 = 0;
      short arg4, arg5, arg6, arg7 = 0;

      if( ( letter = fread_letter( fp ) ) == 'S' )
         break;

      if( letter == '*' )
      {
         fread_to_eol( fp );
         continue;
      }

      extra = fread_number( fp );
      if( letter == 'M' || letter == 'O' )
         extra = 0;
      arg1 = fread_number( fp );
      arg2 = fread_number( fp );
      if( letter != 'G' && letter != 'R' )
         arg3 = fread_number( fp );
      arg4 = arg5 = arg6 = -1;

      if( tarea->version > 18 )
      {
         if( letter == 'O' || letter == 'M' )
         {
            arg4 = fread_short( fp );
            arg5 = fread_short( fp );
            arg6 = fread_short( fp );
         }
      }
      fread_to_eol( fp );
      ++count;

      // Legacy resets are assumed to fire off 100% of the time
      switch ( letter )
      {
         default:
         case 'M':
         case 'O':
            arg7 = 100;
            break;

         case 'E':
         case 'P':
         case 'T':
         case 'D':
            arg4 = 100;
            break;

         case 'G':
         case 'H':
         case 'R':
            arg3 = 100;
            break;
      }

      /*
       * Validate parameters.
       * We're calling the index functions for the side effect.
       */
      switch ( letter )
      {
         default:
            bug( "%s: bad command '%c'.", __func__, letter );
            if( fBootDb )
               boot_log( "%s: %s (%d) bad command '%c'.", __func__, tarea->filename, count, letter );
            return;

         case 'M':
            if( get_mob_index( arg1 ) == nullptr && fBootDb )
               boot_log( "%s: %s (%d) 'M': mobile %d doesn't exist.", __func__, tarea->filename, count, arg1 );

            if( ( pRoomIndex = get_room_index( arg3 ) ) == nullptr && fBootDb )
               boot_log( "%s: %s (%d) 'M': room %d doesn't exist.", __func__, tarea->filename, count, arg3 );
            else
               pRoomIndex->add_reset( letter, arg1, arg2, arg3, arg4, arg5, arg6, arg7, -2, -2, -2, -2 );

            if( arg4 != -1 && ( arg4 < 0 || arg4 >= MAP_MAX ) )
               boot_log( "%s: %s (%d) 'M': Map %d does not exist.", __func__, tarea->filename, count, arg4 );
            if( arg5 != -1 && ( arg5 < 0 || arg5 >= MAX_X ) )
               boot_log( "%s: %s (%d) 'M': X coordinate %d is out of range.", __func__, tarea->filename, count, arg5 );
            if( arg6 != -1 && ( arg6 < 0 || arg6 >= MAX_Y ) )
               boot_log( "%s: %s (%d) 'M': Y coordinate %d is out of range.", __func__, tarea->filename, count, arg6 );
            break;

         case 'O':
            if( get_obj_index( arg1 ) == nullptr && fBootDb )
               boot_log( "%s: %s (%d) '%c': object %d doesn't exist.", __func__, tarea->filename, count, letter, arg1 );

            if( ( pRoomIndex = get_room_index( arg3 ) ) == nullptr && fBootDb )
               boot_log( "%s: %s (%d) '%c': room %d doesn't exist.", __func__, tarea->filename, count, letter, arg3 );
            else
            {
               if( !pRoomIndex )
                  bug( "%s: Unable to add room reset - room not found.", __func__ );
               else
                  pRoomIndex->add_reset( letter, arg1, arg2, arg3, arg4, arg5, arg6, arg7, -2, -2, -2, -2 );
            }
            if( arg4 != -1 && ( arg4 < 0 || arg4 >= MAP_MAX ) )
               boot_log( "%s: %s (%d) 'M': Map %d does not exist.", __func__, tarea->filename, count, arg4 );
            if( arg5 != -1 && ( arg5 < 0 || arg5 >= MAX_X ) )
               boot_log( "%s: %s (%d) 'M': X coordinate %d is out of range.", __func__, tarea->filename, count, arg5 );
            if( arg6 != -1 && ( arg6 < 0 || arg6 >= MAX_Y ) )
               boot_log( "%s: %s (%d) 'M': Y coordinate %d is out of range.", __func__, tarea->filename, count, arg6 );
            break;

         case 'P':
            if( get_obj_index( arg1 ) == nullptr && fBootDb )
               boot_log( "%s: %s (%d) '%c': object %d doesn't exist.", __func__, tarea->filename, count, letter, arg1 );
            if( arg3 > 0 )
            {
               if( get_obj_index( arg3 ) == nullptr && fBootDb )
                  boot_log( "%s: %s (%d) 'P': destination object %d doesn't exist.", __func__, tarea->filename, count, arg3 );
               if( extra > 1 )
                  not01 = true;
            }
            if( !pRoomIndex )
               bug( "%s: Unable to add room reset - room not found.", __func__ );
            else
            {
               if( arg3 == 0 )
                  arg3 = OBJ_VNUM_DUMMYOBJ;  // This may look stupid, but for some reason it works.
               pRoomIndex->add_reset( letter, extra, arg1, arg2, arg3, arg4, -2, -2, -2, -2, -2, -2 );
            }
            break;

         case 'G':
         case 'E':
            if( get_obj_index( arg1 ) == nullptr && fBootDb )
               boot_log( "%s: %s (%d) '%c': object %d doesn't exist.", __func__, tarea->filename, count, letter, arg1 );
            if( !pRoomIndex )
               bug( "%s: Unable to add room reset - room not found.", __func__ );
            else
               pRoomIndex->add_reset( letter, arg1, arg2, arg3, arg4, -2, -2, -2, -2, -2, -2, -2 );
            break;

         case 'T':
            if( IS_SET( extra, TRAP_OBJ ) )
               bug( "%s: Unable to add legacy object trap reset. Must be converted manually.", __func__ );
            else
            {
               if( !( pRoomIndex = get_room_index( arg3 ) ) )
                  bug( "%s: Unable to add trap reset - room not found.", __func__ );
               else
                  pRoomIndex->add_reset( letter, extra, arg1, arg2, arg3, arg4, -2, -2, -2, -2, -2, -2 );
            }
            break;

         case 'H':
            bug( "%s: Unable to convert legacy hide reset. Must be converted manually.", __func__ );
            break;

         case 'D':
            if( !( pRoomIndex = get_room_index( arg1 ) ) )
            {
               bug( "%s: 'D': room %d doesn't exist.", __func__, arg1 );
               log_printf( "Reset: %c %d %d %d %d", letter, extra, arg1, arg2, arg3 );
               if( fBootDb )
                  boot_log( "%s: %s (%d) 'D': room %d doesn't exist.", __func__, tarea->filename, count, arg1 );
               break;
            }

            if( arg2 < 0 || arg2 > MAX_DIR + 1 || !( pexit = pRoomIndex->get_exit( arg2 ) ) || !IS_EXIT_FLAG( pexit, EX_ISDOOR ) )
            {
               bug( "%s: 'D': exit %d not door.", __func__, arg2 );
               log_printf( "Reset: %c %d %d %d %d", letter, extra, arg1, arg2, arg3 );
               if( fBootDb )
                  boot_log( "%s: %s (%d) 'D': exit %d not door.", __func__, tarea->filename, count, arg2 );
            }

            if( arg3 < 0 || arg3 > 2 )
            {
               bug( "%s: 'D': bad 'locks': %d.", __func__, arg3 );
               if( fBootDb )
                  boot_log( "%s: %s (%d) 'D': bad 'locks': %d.", __func__, tarea->filename, count, arg3 );
            }
            pRoomIndex->add_reset( letter, arg1, arg2, arg3, arg4, -2, -2, -2, -2, -2, -2, -2 );
            break;

         case 'R':
            if( !( pRoomIndex = get_room_index( arg1 ) ) && fBootDb )
               boot_log( "%s: %s (%d) 'R': room %d doesn't exist.", __func__, tarea->filename, count, arg1 );
            else
               pRoomIndex->add_reset( letter, arg1, arg2, arg3, -2, -2, -2, -2, -2, -2, -2, -2 );
            if( arg2 < 0 || arg2 > 10 )
            {
               bug( "%s: 'R': bad exit %d.", __func__, arg2 );
               if( fBootDb )
                  boot_log( "%s: %s (%d) 'R': bad exit %d.", __func__, tarea->filename, count, arg2 );
               break;
            }
            break;
      }
   }
   if( !not01 )
   {
      list < room_index * >::iterator iroom;

      for( iroom = tarea->rooms.begin(  ); iroom != tarea->rooms.end(  ); ++iroom )
      {
         pRoomIndex = *iroom;

         pRoomIndex->renumber_put_resets(  );
      }
   }
}

/*
 * Load a room section. Old style AFKMud area file.
 */
void load_rooms( area_data * tarea, FILE * fp )
{
   room_index *pRoomIndex;
   const char *ln;
   int area_number, count = 0, value;

   for( ;; )
   {
      char letter;
      int vnum, door;
      bool tmpBootDb, oldroom;
      int x1, x2, x3, x4, x5, x6;

      letter = fread_letter( fp );
      if( letter != '#' )
      {
         bug( "%s: # not found.", __func__ );
         if( fBootDb )
         {
            shutdown_mud( "# not found" );
            exit( 1 );
         }
         else
            return;
      }

      vnum = fread_number( fp );
      if( vnum == 0 )
         break;

      tmpBootDb = fBootDb;
      fBootDb = false;
      if( get_room_index( vnum ) != nullptr )
      {
         if( tmpBootDb )
         {
            bug( "%s: vnum %d duplicated.", __func__, vnum );
            shutdown_mud( "duplicate vnum" );
            exit( 1 );
         }
         else
         {
            pRoomIndex = get_room_index( vnum );
            log_printf_plus( LOG_BUILD, sysdata->build_level, "Cleaning room: %d", vnum );
            pRoomIndex->clean_room(  );
            oldroom = true;
         }
      }
      else
      {
         oldroom = false;
         pRoomIndex = new room_index;
      }

      fBootDb = tmpBootDb;
      pRoomIndex->area = tarea;
      pRoomIndex->vnum = vnum;

      if( !pRoomIndex->resets.empty(  ) )
      {
         if( fBootDb )
         {
            list < reset_data * >::iterator rst;

            bug( "%s: WARNING: resets already exist for this room.", __func__ );
            for( rst = pRoomIndex->resets.begin(  ); rst != pRoomIndex->resets.end(  ); ++rst )
            {
               reset_data *rtmp = *rst;

               ++count;
               if( !rtmp->resets.empty(  ) )
                  count += rtmp->resets.size(  );
            }
         }
         else
         {
            /*
             * Clean out the old resets
             */
            log_printf_plus( LOG_BUILD, sysdata->build_level, "Cleaning resets: %s", tarea->name );
            pRoomIndex->clean_resets(  );
         }
      }

      if( fBootDb )
      {
         if( !tarea->low_vnum )
            tarea->low_vnum = vnum;
         if( vnum > tarea->hi_vnum )
            tarea->hi_vnum = vnum;
      }
      pRoomIndex->name = fread_string( fp );

      const char *desc = fread_flagstring( fp );
      if( desc && desc[0] != '\0' && str_cmp( desc, "(null)" ) )
         pRoomIndex->roomdesc = str_dup( desc );

      /*
       * Check for NiteDesc's  -- Dracones 
       */
      if( tarea->version > 13 )
      {
         const char *ndesc = fread_flagstring( fp );
         if( ndesc && ndesc[0] != '\0' && str_cmp( ndesc, "(null)" ) )
            pRoomIndex->nitedesc = str_dup( ndesc );
      }

      int sector = get_sectypes( fread_flagstring( fp ) );
      if( sector < 0 || sector >= SECT_MAX )
      {
         bug( "%s: Room #%d has bad sector type.", __func__, vnum );
         sector = 1;
      }

      pRoomIndex->sector_type = sector;
      pRoomIndex->winter_sector = -1;

      flag_set( fp, pRoomIndex->flags, r_flags );

      area_number = fread_number( fp );

      if( area_number > 0 )
      {
         ln = fread_line( fp );
         x1 = x2 = x3 = x4 = 0;
         sscanf( ln, "%d %d %d %d", &x1, &x2, &x3, &x4 );

         pRoomIndex->tele_delay = x1;
         pRoomIndex->tele_vnum = x2;
         pRoomIndex->tunnel = x3;
         pRoomIndex->baselight = x4;
      }
      else
      {
         pRoomIndex->tele_delay = 0;
         pRoomIndex->tele_vnum = 0;
         pRoomIndex->tunnel = 0;
         pRoomIndex->baselight = 0;
      }
      pRoomIndex->light = pRoomIndex->baselight;

      if( pRoomIndex->sector_type < 0 || pRoomIndex->sector_type >= SECT_MAX )
      {
         bug( "%s: vnum %d has bad sector_type %d.", __func__, vnum, pRoomIndex->sector_type );
         pRoomIndex->sector_type = 1;
      }

      for( ;; )
      {
         letter = fread_letter( fp );

         if( letter == 'S' )
            break;

         if( letter == 'A' )
         {
            affect_data *paf;
            char *loc = nullptr;
            char *aff = nullptr;

            paf = new affect_data;
            paf->type = -1;
            paf->duration = -1;
            paf->bit = 0;
            paf->modifier = 0;
            paf->rismod.reset(  );

            loc = fread_word( fp );
            value = get_atype( loc );
            if( value < 0 || value >= MAX_APPLY_TYPE )
               bug( "%s: Invalid apply type: %s", __func__, loc );
            else
               paf->location = value;

            if( paf->location == APPLY_WEAPONSPELL
                || paf->location == APPLY_WEARSPELL
                || paf->location == APPLY_REMOVESPELL || paf->location == APPLY_STRIPSN || paf->location == APPLY_RECURRINGSPELL || paf->location == APPLY_EAT_SPELL )
               paf->modifier = skill_lookup( fread_word( fp ) );
            else if( paf->location == APPLY_AFFECT )
            {
               aff = fread_word( fp );
               value = get_aflag( aff );
               if( value < 0 || value >= MAX_AFFECTED_BY )
                  bug( "%s: Unsupportable value for affect flag: %s", __func__, aff );
               else
                  paf->modifier = value;
            }
            else if( paf->location == APPLY_RESISTANT || paf->location == APPLY_IMMUNE || paf->location == APPLY_SUSCEPTIBLE || paf->location == APPLY_ABSORB )
               flag_set( fp, paf->rismod, ris_flags );
            else
               paf->modifier = fread_number( fp );

            paf->type = fread_short( fp );
            paf->duration = fread_number( fp );
            paf->bit = fread_number( fp );

            if( paf->bit >= MAX_AFFECTED_BY )
               deleteptr( paf );
            else
               pRoomIndex->permaffects.push_back( paf );
         }
         else if( letter == 'D' )
         {
            door = get_dir( fread_flagstring( fp ) );

            if( door < 0 || door > DIR_SOMEWHERE )
            {
               bug( "%s: vnum %d has bad door number %d.", __func__, vnum, door );
               if( fBootDb )
                  exit( 1 );
            }
            else
            {
               exit_data *pexit = pRoomIndex->make_exit( nullptr, door );
               pexit->exitdesc = fread_string( fp );
               pexit->keyword = fread_string( fp );

               flag_set( fp, pexit->flags, ex_flags );

               ln = fread_line( fp );
               x1 = x2 = x3 = x4 = x5 = x6 = 0;
               sscanf( ln, "%d %d %d %d %d %d", &x1, &x2, &x3, &x4, &x5, &x6 );

               pexit->key = x1;
               pexit->vnum = x2;
               pexit->vdir = door;
               pexit->mx = x3;
               pexit->my = x4;
               pexit->pulltype = x5;
               pexit->pull = x6;

               if( tarea->version < 13 )
               {
                  pexit->mx -= 1;
                  pexit->my -= 1;
               }
            }
         }
         else if( letter == 'E' )
         {
            extra_descr_data *ed = new extra_descr_data;

            fread_string( ed->keyword, fp );
            fread_string( ed->desc, fp );
            pRoomIndex->extradesc.push_back( ed );
            ++top_ed;
         }

         else if( letter == 'R' )
            pRoomIndex->load_reset( fp, false );
         else if( letter == '>' )
         {
            ungetc( letter, fp );
            pRoomIndex->rprog_read_programs( fp );
         }
         else
         {
            bug( "%s: vnum %d has flag '%c' not 'ADESR'.", __func__, vnum, letter );
            shutdown_mud( "Room flag not ADESR" );
            exit( 1 );
         }
      }

      if( !oldroom )
      {
         room_index_table.insert( map < int, room_index * >::value_type( pRoomIndex->vnum, pRoomIndex ) );

         tarea->rooms.push_back( pRoomIndex );
         ++top_room;
      }
   }
}

// Old style AFKMud area file.
void load_shops( FILE * fp )
{
   shop_data *pShop;

   for( ;; )
   {
      mob_index *pMobIndex;
      int iTrade;

      pShop = new shop_data;
      pShop->keeper = fread_number( fp );
      if( pShop->keeper == 0 )
      {
         deleteptr( pShop );
         break;
      }
      for( iTrade = 0; iTrade < MAX_TRADE; ++iTrade )
         pShop->buy_type[iTrade] = fread_number( fp );
      pShop->profit_buy = fread_number( fp );
      pShop->profit_sell = fread_number( fp );
      pShop->profit_buy = URANGE( pShop->profit_sell + 5, pShop->profit_buy, 1000 );
      pShop->profit_sell = URANGE( 0, pShop->profit_sell, pShop->profit_buy - 5 );
      pShop->open_hour = fread_number( fp );
      pShop->close_hour = fread_number( fp );
      fread_to_eol( fp );
      pMobIndex = get_mob_index( pShop->keeper );
      pMobIndex->pShop = pShop;
      shoplist.push_back( pShop );
      ++top_shop;
   }
}

// Old style AFKMud area file.
void load_repairs( FILE * fp )
{
   repair_data *rShop;

   for( ;; )
   {
      mob_index *pMobIndex;
      int iFix;

      rShop = new repair_data;
      rShop->keeper = fread_number( fp );
      if( rShop->keeper == 0 )
      {
         deleteptr( rShop );
         break;
      }
      for( iFix = 0; iFix < MAX_FIX; ++iFix )
         rShop->fix_type[iFix] = fread_number( fp );
      rShop->profit_fix = fread_number( fp );
      rShop->shop_type = fread_number( fp );
      rShop->open_hour = fread_number( fp );
      rShop->close_hour = fread_number( fp );
      fread_to_eol( fp );
      pMobIndex = get_mob_index( rShop->keeper );
      pMobIndex->rShop = rShop;
      repairlist.push_back( rShop );
      ++top_repair;
   }
}

// Return true for failure, false if ok at end.
bool load_oldafk_area( FILE *fpArea, area_data *tarea, int area_version )
{
   if( !fpArea )
   {
      bug( "%s: Bad FILE pointer passed!", __func__ );
      return true;
   }

   if( !tarea )
   {
      bug( "%s: Bad area pointer passed!", __func__ );
      return true;
   }

   tarea->version = area_version;

   for( ;; )
   {
      if( fread_letter( fpArea ) != '#' )
      {
         bug( "%s: # not found %s", __func__, tarea->filename );
         exit( 1 );
      }

      const char *word = fread_word( fpArea );

      if( word[0] == '$' )
         break;

      // Resuming from where area.cpp left off. AUTHOR should be next valid field.
      else if( !str_cmp( word, "AUTHOR" ) )
      {
         STRFREE( tarea->author );
         tarea->author = fread_string( fpArea );
      }
      else if( !str_cmp( word, "VNUMS" ) )
      {
         tarea->low_vnum = fread_number( fpArea );
         tarea->hi_vnum = fread_number( fpArea );

         /*
          * Protection against forgetting to raise the MaxVnum value before adding a new zone that would exceed it.
          * Potentially dangerous if some blockhead makes insanely high vnums and then installs the area.
          */
         if( tarea->hi_vnum >= sysdata->maxvnum )
         {
            sysdata->maxvnum = tarea->hi_vnum + 1;
            log_printf( "MaxVnum value raised to %d to accomadate new zone.", sysdata->maxvnum );
            save_sysdata(  );
         }
      }
      else if( !str_cmp( word, "RANGES" ) )
      {
         int x1, x2, x3, x4;
         const char *ln;

         for( ;; )
         {
            ln = fread_line( fpArea );

            if( ln[0] == '$' )
               break;

            x1 = x2 = x3 = x4 = 0;
            sscanf( ln, "%d %d %d %d", &x1, &x2, &x3, &x4 );

            tarea->low_soft_range = x1;
            tarea->hi_soft_range = x2;
            tarea->low_hard_range = x3;
            tarea->hi_hard_range = x4;
         }
      }
      else if( !str_cmp( word, "RESETMSG" ) )
      {
         DISPOSE( tarea->resetmsg );
         tarea->resetmsg = fread_string_nohash( fpArea );
      }
      /*
       * Frequency loader - Samson 5-10-99 
       */
      else if( !str_cmp( word, "RESETFREQUENCY" ) )
      {
         tarea->reset_frequency = fread_number( fpArea );
         tarea->age = tarea->reset_frequency;
      }
      else if( !str_cmp( word, "FLAGS" ) )
      {
         flag_set( fpArea, tarea->flags, area_flags );
      }
      else if( !str_cmp( word, "CONTINENT" ) )
      {
         int value;

         value = get_continent( fread_flagstring( fpArea ) );

         if( value < 0 || value > ACON_MAX )
         {
            tarea->continent = 0;
            bug( "%s: Invalid area continent, set to 'alsherok' by default.", __func__ );
         }
         else
            tarea->continent = value;
      }
      else if( !str_cmp( word, "COORDS" ) )
      {
         short x, y;

         x = fread_short( fpArea );
         y = fread_short( fpArea );

         if( x < 0 || x >= MAX_X )
         {
            bug( "%s: Area has bad x coord - setting X to 0", __func__ );
            x = 0;
         }

         if( y < 0 || y >= MAX_Y )
         {
            bug( "%s: Area has bad y coord - setting Y to 0", __func__ );
            y = 0;
         }
         tarea->mx = x;
         tarea->my = y;
      }
      else if( !str_cmp( word, "CLIMATE" ) )
      {
         tarea->weather->climate_temp = fread_number( fpArea );
         tarea->weather->climate_precip = fread_number( fpArea );
         tarea->weather->climate_wind = fread_number( fpArea );
      }
      else if( !str_cmp( word, "TREASURE" ) )
      {
         unsigned short x1, x2, x3, x4;
         const char *ln = fread_line( fpArea );

         x1 = 20;
         x2 = 74;
         x3 = 85;
         x4 = 93;
         sscanf( ln, "%hu %hu %hu %hu", &x1, &x2, &x3, &x4 );

         tarea->tg_nothing = x1;
         tarea->tg_gold = x2;
         tarea->tg_item = x3;
         tarea->tg_gem = x4;

         ln = fread_line( fpArea );

         x1 = 20;
         x2 = 50;
         x3 = 60;
         x4 = 75;
         sscanf( ln, "%hu %hu %hu %hu", &x1, &x2, &x3, &x4 );

         tarea->tg_scroll = x1;
         tarea->tg_potion = x2;
         tarea->tg_wand = x3;
         tarea->tg_armor = x4;

         validate_treasure_settings( tarea );
      }
      else if( !str_cmp( word, "NEIGHBOR" ) )
      {
         neighbor_data *anew;

         anew = new neighbor_data;
         anew->address = nullptr;
         anew->name = fread_string( fpArea );
         tarea->weather->neighborlist.push_back( anew );
      }
      else if( !str_cmp( word, "MOBILES" ) )
         load_mobiles( tarea, fpArea );
      else if( !str_cmp( word, "OBJECTS" ) )
         load_objects( tarea, fpArea );
      else if( !str_cmp( word, "RESETS" ) )
         load_resets( tarea, fpArea );
      else if( !str_cmp( word, "ROOMS" ) )
         load_rooms( tarea, fpArea );
      else if( !str_cmp( word, "SHOPS" ) )
         load_shops( fpArea );
      else if( !str_cmp( word, "REPAIRS" ) )
         load_repairs( fpArea );
      else if( !str_cmp( word, "SPECIALS" ) )
      {
         bool done = false;

         for( ;; )
         {
            mob_index *pMobIndex;
            char *temp;
            char letter;

            switch ( letter = fread_letter( fpArea ) )
            {
               default:
                  bug( "%s: letter '%c' not *MSOR.", __func__, letter );
                  exit( 1 );

               case 'S':
                  done = true;
                  break;

               case '*':
                  break;

               case 'M':
                  pMobIndex = get_mob_index( fread_number( fpArea ) );
                  temp = fread_word( fpArea );
                  if( !pMobIndex )
                  {
                     bug( "%s: 'M': Invalid mob vnum!", __func__ );
                     break;
                  }
                  if( !( pMobIndex->spec_fun = m_spec_lookup( temp ) ) )
                  {
                     bug( "%s: 'M': vnum %d, no spec_fun called %s.", __func__, pMobIndex->vnum, temp );
                     pMobIndex->spec_funname.clear(  );
                  }
                  else
                     pMobIndex->spec_funname = temp;
                  break;
            }
            if( done )
               break;
            fread_to_eol( fpArea );
         }
      }
      else
      {
         bug( "%s: %s bad section name %s", __func__, tarea->filename, word );
         return true;
      }
   }
   return false;
}
