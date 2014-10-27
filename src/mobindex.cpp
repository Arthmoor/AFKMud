/****************************************************************************
 *                   ^     +----- |  / ^     ^ |     | +-\                  *
 *                  / \    |      | /  |\   /| |     | |  \                 *
 *                 /   \   +---   |<   | \ / | |     | |  |                 *
 *                /-----\  |      | \  |  v  | |     | |  /                 *
 *               /       \ |      |  \ |     | +-----+ +-/                  *
 ****************************************************************************
 * AFKMud Copyright 1997-2008 by Roger Libiez (Samson),                     *
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
 *                        Mobile Index Support Function                     *
 ****************************************************************************/

#include "mud.h"
#include "area.h"
#include "mobindex.h"
#include "mud_prog.h"
#include "shops.h"

int race_bodyparts( char_data * );
int mob_xp( char_data * );

mob_index *mob_index_hash[MAX_KEY_HASH];

extern int top_shop;
extern int top_repair;

mob_index::~mob_index(  )
{
   area->mobs.remove( this );

   list<char_data*>::iterator ich;
   for( ich = charlist.begin(); ich != charlist.end(); )
   {
      char_data *ch = *ich;
      ++ich;

      if( ch->pIndexData == this )
         ch->extract( true );
   }

   for( ich = charlist.begin(); ich != charlist.end(); )
   {
      char_data *ch = *ich;
      ++ich;

      if( ch->pIndexData == this )
         ch->extract( true );
      else if( ch->substate == SUB_MPROG_EDIT && ch->pcdata->dest_buf )
      {
         list<mud_prog_data*>::iterator mpg;

         for( mpg = mudprogs.begin(); mpg != mudprogs.end(); )
         {
            mud_prog_data *mp = *mpg;

            if( mp == ch->pcdata->dest_buf )
            {
               ch->print( "Your victim has departed.\r\n" );
               ch->stop_editing( );
               ch->pcdata->dest_buf = NULL;
               ch->substate = SUB_NONE;
               break;
            }
         }
      }
   }

   list<mud_prog_data*>::iterator mpg;
   for( mpg = mudprogs.begin(); mpg != mudprogs.end(); )
   {
      mud_prog_data *mprog = (*mpg);
      ++mpg;

      mudprogs.remove( mprog );
      deleteptr( mprog );
   }
   mudprogs.clear();

   if( pShop )
   {
      shoplist.remove( pShop );
      deleteptr( pShop );
      --top_shop;
   }

   if( rShop )
   {
      repairlist.remove( rShop );
      deleteptr( rShop );
      --top_repair;
   }

   STRFREE( player_name );
   STRFREE( short_descr );
   STRFREE( long_descr );
   STRFREE( chardesc );

   int hash = vnum % MAX_KEY_HASH;
   if( this == mob_index_hash[hash] )
      mob_index_hash[hash] = next;
   else
   {
      mob_index *prev;

      for( prev = mob_index_hash[hash]; prev; prev = prev->next )
         if( prev->next == this )
            break;
      if( prev )
         prev->next = next;
      else
         bug( "%s: mobile %d not in hash bucket %d.", __FUNCTION__, vnum, hash );
   }
   --top_mob_index;
}

mob_index::mob_index(  )
{
   init_memory( &next, &saving_spell_staff, sizeof( saving_spell_staff ) );
}

/*
 * clean out a mobile (index) (leave list pointers intact )	-Thoric
 */
void mob_index::clean_mob(  )
{
   STRFREE( player_name );
   STRFREE( short_descr );
   STRFREE( long_descr );
   STRFREE( chardesc );
   spec_funname.clear();
   spec_fun = NULL;
   pShop = NULL;
   rShop = NULL;
   progtypes.reset(  );

   list<mud_prog_data*>::iterator mpg;
   for( mpg = mudprogs.begin(); mpg != mudprogs.end(); )
   {
      mud_prog_data *mprog = (*mpg);
      ++mpg;

      mudprogs.remove( mprog );
      deleteptr( mprog );
   }
   mudprogs.clear();

   count = 0;
   killed = 0;
   sex = 0;
   level = 0;
   actflags.reset(  );
   affected_by.reset(  );
   alignment = 0;
   mobthac0 = 0;
   ac = 0;
   hitnodice = 0;
   hitsizedice = 0;
   hitplus = 0;
   damnodice = 0;
   damsizedice = 0;
   damplus = 0;
   gold = 0;
   position = 0;
   defposition = 0;
   height = 0;
   weight = 0;
   perm_str = 13;
   perm_dex = 13;
   perm_int = 13;
   perm_wis = 13;
   perm_cha = 13;
   perm_con = 13;
   perm_lck = 13;
   attacks.reset(  );
   defenses.reset(  );
}

/*
 * Translates mob virtual number to its mob index struct.
 * Hash table lookup.
 */
mob_index *get_mob_index( int vnum )
{
   mob_index *pMobIndex;

   if( vnum < 0 )
      vnum = 0;

   for( pMobIndex = mob_index_hash[vnum % MAX_KEY_HASH]; pMobIndex; pMobIndex = pMobIndex->next )
      if( pMobIndex->vnum == vnum )
         return pMobIndex;

   if( fBootDb )
      bug( "%s: bad vnum %d.", __FUNCTION__, vnum );

   return NULL;
}

/*
 * Simple linear interpolation.
 */
int interpolate( int level, int value_00, int value_32 )
{
   return value_00 + level * ( value_32 - value_00 ) / 32;
}

/*
 * Create an instance of a mobile.
 */
/* Modified for mob randomizations by Whir - 4-5-98 */
char_data *mob_index::create_mobile(  )
{
   char_data *mob;

   if( !this )
   {
      bug( "%s: NULL pMobIndex.", __FUNCTION__ );
      exit( 1 );
   }

   mob = new char_data;

   mob->pIndexData = this;

   mob->name = QUICKLINK( player_name );
   if( short_descr && short_descr[0] != '\0' )
      mob->short_descr = QUICKLINK( short_descr );
   if( long_descr && long_descr[0] != '\0' )
      mob->long_descr = QUICKLINK( long_descr );
   if( chardesc && chardesc[0] != '\0' )
      mob->chardesc = QUICKLINK( chardesc );
   mob->spec_fun = spec_fun;
   mob->spec_funname = spec_funname;
   mob->mpscriptpos = 0;
   mob->level = number_fuzzy( level );
   mob->set_actflags( actflags );
   mob->home_vnum = -1;
   mob->sector = -1;
   mob->timer = 0;
   mob->resetvnum = -1;
   mob->resetnum = -1;

   if( mob->has_actflag( ACT_MOBINVIS ) )
      mob->mobinvis = mob->level;

   mob->set_aflags( affected_by );
   mob->alignment = alignment;
   mob->sex = sex;

   /*
    * Bug fix from mailing list by stu (sprice@ihug.co.nz)
    * was:  if ( !ac )
    */
   if( ac )
      mob->armor = ac;
   else
      mob->armor = interpolate( mob->level, 100, -100 );

   /*
    * Formula altered to conform to Shard mobs: leveld8 + bonus 
    * Samson 5-3-99 
    */
   mob->max_hit = dice( mob->level, 8 ) + hitplus;

   mob->hit = mob->max_hit;
   mob->gold = gold;
   mob->position = position;
   mob->defposition = defposition;
   mob->barenumdie = damnodice;
   mob->baresizedie = damsizedice;
   mob->mobthac0 = mobthac0;
   mob->hitplus = hitplus;
   mob->damplus = damplus;
   mob->perm_str = number_range( 9, 18 );
   mob->perm_str = number_range( 9, 18 );
   mob->perm_wis = number_range( 9, 18 );
   mob->perm_int = number_range( 9, 18 );
   mob->perm_dex = number_range( 9, 18 );
   mob->perm_con = number_range( 9, 18 );
   mob->perm_cha = number_range( 9, 18 );
   mob->perm_lck = number_range( 9, 18 );
   mob->max_move = max_move;
   mob->move = mob->max_move;
   mob->max_mana = max_mana;
   mob->mana = mob->max_mana;

   mob->hitroll = 0;
   mob->damroll = 0;
   mob->race = race;
   mob->Class = Class;
   mob->set_bparts( body_parts );

   /*
    * Saving throw calculations now ported from Sillymud - Samson 5-15-98 
    */
   mob->saving_poison_death = UMAX( 20 - mob->level, 2 );
   mob->saving_wand = UMAX( 20 - mob->level, 2 );
   mob->saving_para_petri = UMAX( 20 - mob->level, 2 );
   mob->saving_breath = UMAX( 20 - mob->level, 2 );
   mob->saving_spell_staff = UMAX( 20 - mob->level, 2 );

   mob->height = height;
   mob->weight = weight;
   mob->set_resists( resistant );
   mob->set_immunes( immune );
   mob->set_susceps( susceptible );
   mob->set_absorbs( absorb );
   mob->set_attacks( attacks );
   mob->set_defenses( defenses );

   /*
    * Samson 5-6-99 
    */
   if( numattacks )
      mob->numattacks = numattacks;
   else
      mob->set_numattacks(  );

   mob->set_langs( speaks );
   mob->speaking = speaking;

   if( body_parts.none(  ) )
      race_bodyparts( mob );
   body_parts = mob->get_bparts();

   if( mob->numattacks > 10 )
      log_printf_plus( LOG_BUILD, sysdata->build_level, "Mob vnum %d has too many attacks: %f", vnum, mob->numattacks );

   /*
    * Exp modification added by Samson - 5-15-98
    * * Moved here because of the new exp autocalculations : Samson 5-18-01 
    * * Need to flush all the old values because the old code had a bug in it on top of everything else.
    */
   if( exp < 1 )
   {
      mob->exp = mob_xp( mob );
      exp = -1;
   }
   else
      mob->exp = exp;

   /*
    * Perhaps add this to the index later --Shaddai
    */
   mob->set_noaflags( 0 );
   mob->set_noresists( 0 );
   mob->set_noimmunes( 0 );
   mob->set_nosusceps( 0 );

   /*
    * Insert in list.
    */
   charlist.push_back( mob );
   ++count;
   ++nummobsloaded;
   return mob;
}

/*
 * Create a new INDEX mobile (for online building) - Thoric
 * Option to clone an existing index mobile.
 */
mob_index *make_mobile( int vnum, int cvnum, char *name, area_data *area )
{
   mob_index *cMobIndex = NULL;

   if( cvnum > 0 )
      cMobIndex = get_mob_index( cvnum );

   mob_index *pMobIndex = new mob_index;

   pMobIndex->vnum = vnum;
   pMobIndex->count = 0;
   pMobIndex->killed = 0;
   pMobIndex->player_name = STRALLOC( name );
   pMobIndex->area = area;

   if( !cMobIndex )
   {
      stralloc_printf( &pMobIndex->short_descr, "A newly created %s", name );
      stralloc_printf( &pMobIndex->long_descr, "Some god abandoned a newly created %s here.\r\n", name );
      pMobIndex->short_descr[0] = LOWER( pMobIndex->short_descr[0] );
      pMobIndex->long_descr[0] = UPPER( pMobIndex->long_descr[0] );
      pMobIndex->actflags.reset(  );
      pMobIndex->actflags.set( ACT_IS_NPC );
      pMobIndex->actflags.set( ACT_PROTOTYPE );
      pMobIndex->affected_by.reset(  );
      pMobIndex->pShop = NULL;
      pMobIndex->rShop = NULL;
      pMobIndex->spec_fun = NULL;
      pMobIndex->mudprogs.clear();
      pMobIndex->progtypes.reset(  );
      pMobIndex->alignment = 0;
      pMobIndex->level = 1;
      pMobIndex->mobthac0 = 21;
      pMobIndex->exp = -1;
      pMobIndex->ac = 0;
      pMobIndex->hitnodice = 0;
      pMobIndex->hitsizedice = 0;
      pMobIndex->hitplus = 0;
      pMobIndex->damnodice = 0;
      pMobIndex->damsizedice = 0;
      pMobIndex->damplus = 0;
      pMobIndex->hitroll = 0;
      pMobIndex->damroll = 0;
      pMobIndex->max_move = 150;
      pMobIndex->max_mana = 100;
      pMobIndex->gold = 0;
      pMobIndex->position = POS_STANDING;
      pMobIndex->defposition = POS_STANDING;
      pMobIndex->sex = SEX_NEUTRAL;
      pMobIndex->perm_str = 13;
      pMobIndex->perm_dex = 13;
      pMobIndex->perm_int = 13;
      pMobIndex->perm_wis = 13;
      pMobIndex->perm_cha = 13;
      pMobIndex->perm_con = 13;
      pMobIndex->perm_lck = 13;
      pMobIndex->race = RACE_HUMAN;
      pMobIndex->Class = CLASS_WARRIOR;
      pMobIndex->body_parts.reset(  );
      pMobIndex->resistant.reset(  );
      pMobIndex->immune.reset(  );
      pMobIndex->susceptible.reset(  );
      pMobIndex->absorb.reset(  );
      pMobIndex->attacks.reset(  );
      pMobIndex->defenses.reset(  );
      pMobIndex->numattacks = 0;
      pMobIndex->height = 0;
      pMobIndex->weight = 0;
      pMobIndex->speaks.set( LANG_COMMON );
      pMobIndex->speaking = LANG_COMMON;
   }
   else
   {
      pMobIndex->short_descr = QUICKLINK( cMobIndex->short_descr );
      pMobIndex->long_descr = QUICKLINK( cMobIndex->long_descr );
      if( cMobIndex->chardesc && cMobIndex->chardesc[0] != '\0' )
         pMobIndex->chardesc = QUICKLINK( cMobIndex->chardesc );
      pMobIndex->actflags = cMobIndex->actflags;
      pMobIndex->actflags.set( ACT_PROTOTYPE );
      pMobIndex->affected_by = cMobIndex->affected_by;
      pMobIndex->pShop = NULL;
      pMobIndex->rShop = NULL;
      pMobIndex->spec_fun = cMobIndex->spec_fun;
      pMobIndex->mudprogs.clear();
      pMobIndex->progtypes.reset(  );
      pMobIndex->alignment = cMobIndex->alignment;
      pMobIndex->level = cMobIndex->level;
      pMobIndex->mobthac0 = cMobIndex->mobthac0;
      pMobIndex->ac = cMobIndex->ac;
      pMobIndex->hitnodice = cMobIndex->hitnodice;
      pMobIndex->hitsizedice = cMobIndex->hitsizedice;
      pMobIndex->hitplus = cMobIndex->hitplus;
      pMobIndex->damnodice = cMobIndex->damnodice;
      pMobIndex->damsizedice = cMobIndex->damsizedice;
      pMobIndex->damplus = cMobIndex->damplus;
      pMobIndex->hitroll = 0; /* Yes, this is right. We don't want them to
                               * pMobIndex->damroll = 0;    retain this when saved - Samson 5-5-00 */
      pMobIndex->gold = cMobIndex->gold;
      pMobIndex->exp = cMobIndex->exp;
      pMobIndex->position = cMobIndex->position;
      pMobIndex->defposition = cMobIndex->defposition;
      pMobIndex->sex = cMobIndex->sex;
      pMobIndex->perm_str = cMobIndex->perm_str;
      pMobIndex->perm_dex = cMobIndex->perm_dex;
      pMobIndex->perm_int = cMobIndex->perm_int;
      pMobIndex->perm_wis = cMobIndex->perm_wis;
      pMobIndex->perm_cha = cMobIndex->perm_cha;
      pMobIndex->perm_con = cMobIndex->perm_con;
      pMobIndex->perm_lck = cMobIndex->perm_lck;
      pMobIndex->race = cMobIndex->race;
      pMobIndex->Class = cMobIndex->Class;
      pMobIndex->body_parts = cMobIndex->body_parts;
      pMobIndex->resistant = cMobIndex->resistant;
      pMobIndex->immune = cMobIndex->immune;
      pMobIndex->susceptible = cMobIndex->susceptible;
      pMobIndex->absorb = cMobIndex->absorb;
      pMobIndex->numattacks = cMobIndex->numattacks;
      pMobIndex->attacks = cMobIndex->attacks;
      pMobIndex->defenses = cMobIndex->defenses;
      pMobIndex->height = cMobIndex->height;
      pMobIndex->weight = cMobIndex->weight;
      pMobIndex->speaks = cMobIndex->speaks;
      pMobIndex->speaking = cMobIndex->speaking;
   }

   int iHash = vnum % MAX_KEY_HASH;
   pMobIndex->next = mob_index_hash[iHash];
   mob_index_hash[iHash] = pMobIndex;
   area->mobs.push_back( pMobIndex );
   ++top_mob_index;

   return pMobIndex;
}

/* This procedure is responsible for reading any in_file MUDprograms. */
void mob_index::mprog_read_programs( FILE * fp )
{
   mud_prog_data *mprg;
   char letter;
   const char *word;

   for( ; ; )
   {
      letter = fread_letter( fp );

      if( letter == '|' )
         return;

      if( letter != '>' )
      {
         bug( "%s: vnum %d MUDPROG char", __FUNCTION__, vnum );
         exit( 1 );
      }
      mprg = new mud_prog_data;
      mudprogs.push_back( mprg );

      word = fread_word( fp );
      mprg->type = mprog_name_to_type( word );

      switch( mprg->type )
      {
         case ERROR_PROG:
            bug( "%s: vnum %d MUDPROG type.", __FUNCTION__, vnum );
            exit( 1 );

         case IN_FILE_PROG:
            mprg->arglist = fread_string( fp );
            mprg->fileprog = false;
            mprog_file_read( this, mprg->arglist );
            break;

         default:
            progtypes.set( mprg->type );
            mprg->fileprog = false;
            mprg->arglist = fread_string( fp );
            mprg->comlist = fread_string( fp );
            break;
      }
   }
   return;
}

CMDF( do_mfind )
{
   mob_index *pMobIndex;
   int hash, nMatch;
   bool fAll;

   ch->set_pager_color( AT_PLAIN );

   if( !argument || argument[0] == '\0' )
   {
      ch->print( "Find whom?\r\n" );
      return;
   }

   fAll = !str_cmp( argument, "all" );
   nMatch = 0;

   for( hash = 0; hash < MAX_KEY_HASH; ++hash )
      for( pMobIndex = mob_index_hash[hash]; pMobIndex; pMobIndex = pMobIndex->next )
         if( fAll || nifty_is_name( argument, pMobIndex->player_name ) )
         {
            ++nMatch;
            ch->pagerf( "[%5d] %s\r\n", pMobIndex->vnum, capitalize( pMobIndex->short_descr ) );
         }

   if( nMatch )
      ch->pagerf( "Number of matches: %d\n", nMatch );
   else
      ch->print( "Nothing like that in hell, earth, or heaven.\r\n" );
   return;
}

CMDF( do_mdelete )
{
   mob_index *mob;
   int vnum;

   if( ch->substate == SUB_RESTRICTED )
   {
      ch->print( "You can't do that while in a subprompt.\r\n" );
      return;
   }

   if( !argument || argument[0] == '\0' )
   {
      ch->print( "Delete which mob?\r\n" );
      return;
   }

   if( !is_number( argument ) )
   {
      ch->print( "You must specify the mob's vnum to delete it.\r\n" );
      return;
   }

   vnum = atoi( argument );

   /*
    * Find the mob. 
    */
   if( !( mob = get_mob_index( vnum ) ) )
   {
      ch->print( "No such mob.\r\n" );
      return;
   }

   /*
    * Does the player have the right to delete this mob? 
    */
   if( ch->get_trust(  ) < sysdata->level_modify_proto
       && ( mob->vnum < ch->pcdata->low_vnum || mob->vnum > ch->pcdata->hi_vnum ) )
   {
      ch->print( "That mob is not in your assigned range.\r\n" );
      return;
   }
   deleteptr( mob );
   ch->printf( "Mob %d has been deleted.\r\n", vnum );
   return;
}
