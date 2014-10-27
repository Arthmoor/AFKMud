/****************************************************************************
 *                   ^     +----- |  / ^     ^ |     | +-\                  *
 *                  / \    |      | /  |\   /| |     | |  \                 *
 *                 /   \   +---   |<   | \ / | |     | |  |                 *
 *                /-----\  |      | \  |  v  | |     | |  /                 *
 *               /       \ |      |  \ |     | +-----+ +-/                  *
 ****************************************************************************
 * AFKMud Copyright 1997-2007 by Roger Libiez (Samson),                     *
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
 *                        Character Support Functions                       *
 ****************************************************************************/

#include <cstdarg>
#include "mud.h"
#include "area.h"
#include "bits.h"
#include "boards.h"
#include "deity.h"
#include "descriptor.h"
#include "fight.h"
#include "mobindex.h"
#include "mud_prog.h"
#include "new_auth.h"
#include "objindex.h"
#include "polymorph.h"
#include "raceclass.h"
#include "roomindex.h"

list<char_data*> charlist;
list<char_data*> pclist;

#ifdef IMC
void imc_freechardata( char_data * );
#endif
void clean_char_queue( );
void stop_hating( char_data * );
void stop_hunting( char_data * );
void stop_fearing( char_data * );
void free_fight( char_data * );
void queue_extracted_char( char_data *, bool );
room_index *check_room( char_data *, room_index * );
void set_title( char_data *, char * );

extern list<rel_data*> relationlist;

pc_data::~pc_data(  )
{
   if( pnote )
      deleteptr( pnote );

   STRFREE( filename );
   STRFREE( deity_name );
   STRFREE( clan_name );
   STRFREE( prompt );
   STRFREE( fprompt );
   DISPOSE( pwd );   /* no hash */
   DISPOSE( bamfin );   /* no hash */
   DISPOSE( bamfout );  /* no hash */
   STRFREE( rank );
   STRFREE( title );
   DISPOSE( bio );   /* no hash */
   DISPOSE( bestowments ); /* no hash */
   DISPOSE( homepage ); /* no hash */
   DISPOSE( email ); /* no hash */
   DISPOSE( lasthost ); /* no hash */
   STRFREE( authed_by );
   STRFREE( prompt );
   STRFREE( fprompt );
   STRFREE( helled_by );
   STRFREE( subprompt );
   STRFREE( chan_listen ); /* DOH! Forgot about this! */
   DISPOSE( afkbuf );
   DISPOSE( motd_buf );

   alias_map.clear();
   free_comments(  );
   free_pcboards(  );
   zone.clear();
   ignore.clear();
   qbits.clear();

   /*
    * God. How does one forget something like this anyway? BAD SAMSON! 
    */
   for( int x = 0; x < MAX_SAYHISTORY; ++x )
      DISPOSE( say_history[x] );

   /*
    * Dammit! You forgot another one you git! 
    */
   for( int x = 0; x < MAX_TELLHISTORY; ++x )
      DISPOSE( tell_history[x] );
}

/*
 * Clear a new PC.
 */
pc_data::pc_data(  )
{
   int sn;

   init_memory( &area, &hotboot, sizeof( hotboot ) );

   flags.reset(  );
   alias_map.clear();
   zone.clear();
   ignore.clear();
   for( sn = 0; sn < MAX_SAYHISTORY; ++sn )
      say_history[sn] = NULL;
   for( sn = 0; sn < MAX_TELLHISTORY; ++sn )
      tell_history[sn] = NULL;
   for( sn = 0; sn < MAX_BEACONS; ++sn )
      beacon[sn] = 0;
   condition[COND_THIRST] = ( int )( sysdata->maxcondval * .75 );
   condition[COND_FULL] = ( int )( sysdata->maxcondval * .75 );
   pagerlen = 24;
   secedit = SECT_OCEAN;   /* Initialize Map OLC sector - Samson 8-1-99 */
   one = ROOM_VNUM_TEMPLE;  /* Initialize the rent recall room to default - Samson 12-20-00 */
   lasthost = str_dup( "Unknown-Host" );
   logon = current_time;
   timezone = -1;
   for( sn = 0; sn < top_sn; ++sn )
      learned[sn] = 0;
}

/*
 * Free a character's memory.
 */
char_data::~char_data(  )
{
   if( !this )
   {
      bug( "%s: NULL ch!", __FUNCTION__ );
      return;
   }

   // Hackish fix - if we forget, whoever reads this should remind us someday - Samson 3-28-05
   if( this->desc )
   {
      bug( "%s: char still has descriptor.", __FUNCTION__ );
      log_printf( "Desc# %d, DescHost %s DescClient %s", this->desc->descriptor, this->desc->host, this->desc->client );
      deleteptr( this->desc );
   }

   deleteptr( morph );

   list<obj_data*>::iterator iobj;
   for( iobj = carrying.begin(); iobj != carrying.end(); )
   {
      obj_data *obj = (*iobj);
      ++iobj;

      obj->extract(  );
   }

   list<affect_data*>::iterator paf;
   for( paf = affects.begin(); paf != affects.end(); )
   {
      affect_data *aff = (*paf);
      ++paf;

      affect_remove( aff );
   }

   list<timer_data*>::iterator chtimer;
   for( chtimer = timers.begin(); chtimer != timers.end(); )
   {
      timer_data *ctimer = (*chtimer);
      ++chtimer;

      extract_timer( ctimer );
   }

   STRFREE( name );
   STRFREE( short_descr );
   STRFREE( long_descr );
   STRFREE( chardesc );
   DISPOSE( alloc_ptr );

   stop_hunting( this );
   stop_hating( this );
   stop_fearing( this );
   free_fight( this );
   abits.clear();

   if( pcdata )
   {
      if( pcdata->editor )
         stop_editing(  );
#ifdef IMC
      imc_freechardata( this );
#endif
      deleteptr( pcdata );
   }

   list<mprog_act_list*>::iterator pd;
   for( pd = mpact.begin(); pd != mpact.end(); )
   {
      mprog_act_list *actp = (*pd);
      ++pd;

      deleteptr( actp );
   }
}

/*
 * Clear a new character.
 */
char_data::char_data(  )
{
   init_memory( &master, &backtracking, sizeof( backtracking ) );

   armor = 100;
   position = POS_STANDING;
   hit = 20;
   max_hit = 20;
   mana = 100;
   max_mana = 100;
   move = 150;
   max_move = 150;
   height = 72;
   weight = 180;
   spellfail = 101;
   race = RACE_HUMAN;
   Class = CLASS_WARRIOR;
   speaking = LANG_COMMON;
   speaks.reset(  );
   barenumdie = 1;
   baresizedie = 4;
   perm_str = 13;
   perm_dex = 13;
   perm_int = 13;
   perm_wis = 13;
   perm_cha = 13;
   perm_con = 13;
   perm_lck = 13;
   mx = -1; /* Overland Map - Samson 7-31-99 */
   my = -1;
   map = -1;
   wait = 0;
}

void extract_all_chars( void )
{
   clean_char_queue();

   list<char_data*>::iterator ich;
   for( ich = charlist.begin(); ich != charlist.end(); )
   {
      char_data *character = (*ich);
      ++ich;

      character->extract( true );
   }
   clean_char_queue(  );
}

bool char_data::MSP_ON(  )
{
   return ( has_pcflag( PCFLAG_MSP ) && desc && desc->msp_detected );
}

bool char_data::MXP_ON(  )
{
   return ( has_pcflag( PCFLAG_MXP ) && desc && desc->mxp_detected );
}

bool char_data::IS_OUTSIDE(  )
{
   if( !in_room->flags.test( ROOM_INDOORS ) && !in_room->flags.test( ROOM_TUNNEL )
       && !in_room->flags.test( ROOM_CAVE ) && !in_room->flags.test( ROOM_CAVERN ) )
      return true;
   else
      return false;
}

int char_data::GET_AC(  )
{
   return ( armor + ( IS_AWAKE(  )? dex_app[get_curr_dex(  )].defensive : 0 ) );
}

int char_data::GET_HITROLL(  )
{
   return ( hitroll + str_app[get_curr_str(  )].tohit );
}

/* Thanks to Chriss Baeke for noticing damplus was unused */
int char_data::GET_DAMROLL(  )
{
   return ( damroll + damplus + str_app[get_curr_str(  )].todam );
}
int char_data::GET_ADEPT( int sn )
{
   return skill_table[sn]->skill_adept[Class];
}

bool char_data::IS_DRUNK( int drunk )
{
   return ( number_percent(  ) < ( pcdata->condition[COND_DRUNK] * 2 / drunk ) );
}

void char_data::print( string txt )
{
   if( !txt.empty() && desc )
      desc->send_color( txt.c_str() );
   return;
}

void char_data::print( const char *txt )
{
   if( txt && desc )
      desc->send_color( txt );
   return;
}

void char_data::print_room( const char *txt )
{
   if( !txt || !in_room )
      return;

   list<char_data*>::iterator ich;
   for( ich = in_room->people.begin(); ich != in_room->people.end(); ++ich )
   {
      char_data *rch = (*ich);

      if( !rch->char_died(  ) && rch->desc )
      {
         if( is_same_char_map( this, rch ) )
            rch->print( txt );
      }
   }
}

void char_data::pager( const char *txt )
{
   char_data *ch;

   if( txt && desc )
   {
      descriptor_data *d = desc;

      ch = d->original ? d->original : d->character;
      if( !ch->has_pcflag( PCFLAG_PAGERON ) )
      {
         if( ch->desc )
            ch->desc->send_color( txt );
      }
      else
      {
         if( ch->desc )
            ch->desc->pager( colorize( txt, ch->desc ), 0 );
      }
   }
   return;
}

void char_data::printf( const char *fmt, ... )
{
   char buf[MSL * 2];
   va_list args;

   va_start( args, fmt );
   vsnprintf( buf, MSL * 2, fmt, args );
   va_end( args );

   print( buf );
}

void char_data::pagerf( const char *fmt, ... )
{
   char buf[MSL * 2];
   va_list args;

   va_start( args, fmt );
   vsnprintf( buf, MSL * 2, fmt, args );
   va_end( args );

   pager( buf );
}

void char_data::set_color( short AType )
{
   if( !desc )
      return;

   if( isnpc(  ) )
      return;

   desc->write_to_buffer( color_str( AType ), 0 );
   if( !desc )
   {
      bug( "%s: NULL descriptor after WTB! CH: %s", __FUNCTION__, name ? name : "Unknown?!?" );
      return;
   }
   desc->pagecolor = pcdata->colors[AType];
}

void char_data::set_pager_color( short AType )
{
   if( desc )
      return;

   if( isnpc(  ) )
      return;

   desc->pager( color_str( AType ), 0 );
   if( !desc )
   {
      bug( "%s: NULL descriptor after WTP! CH: %s", __FUNCTION__, name ? name : "Unknown?!?" );
      return;
   }
   desc->pagecolor = pcdata->colors[AType];
}

/*
 * Retrieve a character's trusted level for permission checking.
 */
short char_data::get_trust(  )
{
   char_data *ch;

   if( desc && desc->original )
      ch = desc->original;
   else
      ch = this;

   if( ch->trust != 0 )
      return ch->trust;

   if( ch->isnpc(  ) && ch->level >= LEVEL_AVATAR )
      return LEVEL_AVATAR;

   if( !ch->isnpc(  ) && ch->level >= LEVEL_IMMORTAL && ch->pcdata->flags.test( PCFLAG_RETIRED ) )
      return LEVEL_IMMORTAL;

   return ch->level;
}

/*
 * Retrieve a character's age.
 */
/* Aging advances at the same rate as the calendar date now - Samson */
short char_data::get_age(  )
{
   if( isnpc(  ) )
      return -1;

   return pcdata->age + pcdata->age_bonus;   // There, now isn't this simpler? :P
}

/* One hopes this will do as planned and determine how old a PC is based on the birthdate
   we record at creation. - Samson 10-25-99 */
short char_data::calculate_age(  )
{
   short age = 0, num_days = 0, ch_days = 0;

   if( isnpc(  ) )
      return -1;

   num_days = ( time_info.month + 1 ) * sysdata->dayspermonth;
   num_days += time_info.day;

   ch_days = ( pcdata->month + 1 ) * sysdata->dayspermonth;
   ch_days += pcdata->day;

   age = time_info.year - pcdata->year;

   if( ch_days - num_days > 0 )
      age -= 1;

   return age;
}

/*
 * Retrieve character's current strength.
 */
short char_data::get_curr_str(  )
{
   short max = 0;

   if( isnpc(  ) )
      max = 25;
   else
      max = 20;

   return URANGE( 3, perm_str + mod_str, max );
}

/*
 * Retrieve character's current intelligence.
 */
short char_data::get_curr_int(  )
{
   short max = 0;

   if( isnpc(  ) )
      max = 25;
   else
      max = 20;

   return URANGE( 3, perm_int + mod_int, max );
}

/*
 * Retrieve character's current wisdom.
 */
short char_data::get_curr_wis(  )
{
   short max = 0;

   if( isnpc(  ) )
      max = 25;
   else
      max = 20;

   return URANGE( 3, perm_wis + mod_wis, max );
}

/*
 * Retrieve character's current dexterity.
 */
short char_data::get_curr_dex(  )
{
   short max = 0;

   if( isnpc(  ) )
      max = 25;
   else
      max = 20;

   return URANGE( 3, perm_dex + mod_dex, max );
}

/*
 * Retrieve character's current constitution.
 */
short char_data::get_curr_con(  )
{
   short max = 0;

   if( isnpc(  ) )
      max = 25;
   else
      max = 20;

   return URANGE( 3, perm_con + mod_con, max );
}

/*
 * Retrieve character's current charisma.
 */
short char_data::get_curr_cha(  )
{
   short max = 0;

   if( isnpc(  ) )
      max = 25;
   else
      max = 20;

   return URANGE( 3, perm_cha + mod_cha, max );
}

/*
 * Retrieve character's current luck.
 */
short char_data::get_curr_lck(  )
{
   short max = 0;

   if( isnpc(  ) )
      max = 25;
   else
      max = 20;

   return URANGE( 3, perm_lck + mod_lck, max );
}

/*
 * See if a player/mob can take a piece of prototype eq		-Thoric
 */
bool char_data::can_take_proto(  )
{
   if( is_immortal(  ) )
      return true;
   else if( has_actflag( ACT_PROTOTYPE ) )
      return true;
   else
      return false;
}

/*
 * True if char can see victim.
 */
bool char_data::can_see( char_data *victim, bool override )
{
   if( !victim )  /* Gorog - panicked attempt to stop crashes */
   {
      bug( "%s: NULL victim! CH %s tried to see it.", __FUNCTION__, name );
      return false;
   }

   if( !this )
   {
      if( victim->has_aflag( AFF_INVISIBLE ) || victim->has_aflag( AFF_HIDE ) || victim->has_pcflag( PCFLAG_WIZINVIS ) )
         return false;
      else
         return true;
   }

   if( !is_imp(  ) && victim == supermob )
      return false;

   if( this == victim )
      return true;

   if( victim->has_pcflag( PCFLAG_WIZINVIS ) && level < victim->pcdata->wizinvis )
      return false;

   /*
    * SB 
    */
   if( victim->has_actflag( ACT_MOBINVIS ) && level < victim->mobinvis )
      return false;

   /*
    * Deadlies link-dead over 2 ticks aren't seen by mortals -- Blodkai 
    */
   if( !is_immortal(  ) && !isnpc(  ) && !victim->isnpc(  ) && victim->IS_PKILL(  ) && victim->timer > 1 && !victim->desc )
      return false;

   if( ( has_pcflag( PCFLAG_ONMAP ) || has_actflag( ACT_ONMAP ) ) && !override )
   {
      if( !is_same_char_map( this, victim ) )
         return false;
   }

   /*
    * Unless you want to get spammed to death as an immortal on the map, DONT move this above here!!!! 
    */
   if( has_pcflag( PCFLAG_HOLYLIGHT ) )
      return true;

   /*
    * The miracle cure for blindness? -- Altrag 
    */
   if( !has_aflag( AFF_TRUESIGHT ) )
   {
      if( has_aflag( AFF_BLIND ) )
         return false;

      if( in_room->is_dark( this ) && !has_aflag( AFF_INFRARED ) )
         return false;

      if( victim->has_aflag( AFF_INVISIBLE ) && !has_aflag( AFF_DETECT_INVIS ) )
         return false;

      if( victim->has_aflag( AFF_HIDE ) && !has_aflag( AFF_DETECT_HIDDEN ) && !victim->fighting
          && ( isnpc(  )? !victim->isnpc(  ) : victim->isnpc(  ) ) )
         return false;
   }
   return true;
}

/*
 * Find a char in the room.
 */
char_data *char_data::get_char_room( string argument )
{
   string arg;
   int number = number_argument( argument, arg );

   if( scomp( arg, "self" ) )
      return this;

   int vnum;
   if( get_trust(  ) >= LEVEL_SAVIOR && is_number( arg ) )
      vnum = atoi( arg.c_str() );
   else
      vnum = -1;

   int count = 0;

   list<char_data*>::iterator ich;
   for( ich = in_room->people.begin(); ich != in_room->people.end(); ++ich )
   {
      char_data *rch = (*ich);

      if( can_see( rch, false ) && !is_ignoring( rch, this )
          && ( nifty_is_name( arg, rch->name ) || ( rch->isnpc(  ) && vnum == rch->pIndexData->vnum ) ) )
      {
         if( number == 0 && !rch->isnpc(  ) )
            return rch;
         else if( ++count == number )
            return rch;
      }
   }

   if( vnum != -1 )
      return NULL;

   /*
    * If we didn't find an exact match, run through the list of characters
    * again looking for prefix matching, ie gu == guard.
    * Added by Narn, Sept/96
    */
   count = 0;
   for( ich = in_room->people.begin(); ich != in_room->people.end(); ++ich )
   {
      char_data *rch = (*ich);

      if( !can_see( rch, false ) || !nifty_is_name_prefix( arg, rch->name ) || is_ignoring( rch, this ) )
         continue;

      if( number == 0 && !rch->isnpc(  ) )
         return rch;
      else if( ++count == number )
         return rch;
   }
   return NULL;
}

/*
 * Find a char in the world.
 */
char_data *char_data::get_char_world( string argument )
{
   string arg;
   int number = number_argument( argument, arg );

   if( scomp( arg, "self" ) )
      return this;

   /*
    * Allow reference by vnum for saints+ - Thoric
    */
   int vnum;
   if( get_trust(  ) >= LEVEL_SAVIOR && is_number( arg ) )
      vnum = atoi( arg.c_str() );
   else
      vnum = -1;

   /*
    * check the room for an exact match 
    */
   int count = 0;
   list<char_data*>::iterator ich;
   for( ich = in_room->people.begin(); ich != in_room->people.end(); ++ich )
   {
      char_data *wch = (*ich);

      if( can_see( wch, true ) && !is_ignoring( wch, this )
          && ( nifty_is_name( arg, wch->name ) || ( wch->isnpc(  ) && vnum == wch->pIndexData->vnum ) ) )
      {
         if( number == 0 && !wch->isnpc(  ) )
            return wch;
         else if( ++count == number )
            return wch;
      }
   }

   /*
    * check the world for an exact match 
    */
   for( ich = charlist.begin(); ich != charlist.end(); ++ich )
   {
      char_data *wch = (*ich);

      if( can_see( wch, true ) && !is_ignoring( wch, this )
          && ( nifty_is_name( arg, wch->name ) || ( wch->isnpc(  ) && vnum == wch->pIndexData->vnum ) ) )
      {
         if( number == 0 && !wch->isnpc(  ) )
            return wch;
         else if( ++count == number )
            return wch;
      }
   }

   /*
    * bail out if looking for a vnum match 
    */
   if( vnum != -1 )
      return NULL;

   /*
    * If we didn't find an exact match, check the room for
    * for a prefix match, ie gu == guard.
    * Added by Narn, Sept/96
    */
   count = 0;
   for( ich = in_room->people.begin(); ich != in_room->people.end(); ++ich )
   {
      char_data *wch = (*ich);

      if( !can_see( wch, true ) || !nifty_is_name_prefix( arg, wch->name ) || is_ignoring( wch, this ) )
         continue;
      if( number == 0 && !wch->isnpc(  ) )
         return wch;
      else if( ++count == number )
         return wch;
   }

   /*
    * If we didn't find a prefix match in the room, run through the full list
    * of characters looking for prefix matching, ie gu == guard.
    * Added by Narn, Sept/96
    */
   count = 0;
   for( ich = charlist.begin(); ich != charlist.end(); ++ich )
   {
      char_data *wch = (*ich);

      if( !can_see( wch, true ) || !nifty_is_name_prefix( arg, wch->name ) || is_ignoring( wch, this ) )
         continue;
      if( number == 0 && !wch->isnpc(  ) )
         return wch;
      else if( ++count == number )
         return wch;
   }
   return NULL;
}

room_index *char_data::find_location( char *arg )
{
   char_data *victim;
   obj_data *obj;

   if( is_number( arg ) )
      return get_room_index( atoi( arg ) );

   if( !str_cmp( arg, "pk" ) )   /* "Goto pk", "at pk", etc */
      return get_room_index( last_pkroom );

   if( ( victim = get_char_world( arg ) ) != NULL )
      return victim->in_room;

   if( ( obj = get_obj_world( arg ) ) != NULL )
      return obj->in_room;

   return NULL;
}

/*
 * True if char can see obj.
 * Override boolean to bypass overland coordinate checks - Samson 10-17-03
 */
bool char_data::can_see_obj( obj_data * obj, bool override )
{
   if( has_pcflag( PCFLAG_HOLYLIGHT ) )
      return true;

   if( isnpc(  ) && pIndexData->vnum == MOB_VNUM_SUPERMOB )
      return true;

   if( !is_same_obj_map( this, obj ) && !override )
      return false;

   if( obj->extra_flags.test( ITEM_BURIED ) )
      return false;

   if( obj->extra_flags.test( ITEM_HIDDEN ) )
      return false;

   if( has_aflag( AFF_TRUESIGHT ) )
      return true;

   if( has_aflag( AFF_BLIND ) )
      return false;

   /*
    * can see lights in the dark 
    */
   if( obj->item_type == ITEM_LIGHT && obj->value[2] != 0 )
      return true;

   if( in_room->is_dark( this ) )
   {
      /*
       * can see glowing items in the dark... invisible or not 
       */
      if( obj->extra_flags.test( ITEM_GLOW ) )
         return true;
      if( !has_aflag( AFF_INFRARED ) )
         return false;
   }

   if( obj->extra_flags.test( ITEM_INVIS ) && !has_aflag( AFF_DETECT_INVIS ) )
      return false;

   return true;
}

/*
 * True if char can drop obj.
 */
bool char_data::can_drop_obj( obj_data * obj )
{
   if( !obj->extra_flags.test( ITEM_NODROP ) )
      return true;

   if( !isnpc(  ) && level >= LEVEL_IMMORTAL )
      return true;

   if( isnpc(  ) && pIndexData->vnum == MOB_VNUM_SUPERMOB )
      return true;

   return false;
}

/*
 * Find an obj in player's inventory or wearing via a vnum -Shaddai
 */
obj_data *char_data::get_obj_vnum( int vnum )
{
   list<obj_data*>::iterator iobj;

   for( iobj = carrying.begin(); iobj != carrying.end(); ++iobj )
   {
      obj_data *obj = (*iobj);

      if( can_see_obj( obj, false ) && obj->pIndexData->vnum == vnum )
         return obj;
   }
   return NULL;
}

/*
 * Find an obj in player's inventory.
 */
obj_data *char_data::get_obj_carry( string argument )
{
   string arg;
   obj_data *obj;
   list<obj_data*>::iterator iobj;
   int number, count, vnum;

   number = number_argument( argument, arg );
   if( get_trust(  ) >= LEVEL_SAVIOR && is_number( arg ) )
      vnum = atoi( arg.c_str() );
   else
      vnum = -1;

   count = 0;
   for( iobj = carrying.begin(); iobj != carrying.end(); ++iobj )
   {
      obj = (*iobj);

      if( obj->wear_loc == WEAR_NONE && can_see_obj( obj, false )
          && ( nifty_is_name( arg, obj->name ) || obj->pIndexData->vnum == vnum ) )
         if( ( count += obj->count ) >= number )
            return obj;
   }
   if( vnum != -1 )
      return NULL;

   /*
    * If we didn't find an exact match, run through the list of objects
    * again looking for prefix matching, ie swo == sword.
    * Added by Narn, Sept/96
    */
   count = 0;
   for( iobj = carrying.begin(); iobj != carrying.end(); ++iobj )
   {
      obj = (*iobj);

      if( obj->wear_loc == WEAR_NONE && can_see_obj( obj, false ) && nifty_is_name_prefix( arg, obj->name ) )
         if( ( count += obj->count ) >= number )
            return obj;
   }
   return NULL;
}

/*
 * Find an obj in player's equipment.
 */
obj_data *char_data::get_obj_wear( string argument )
{
   string arg;
   obj_data *obj;
   list<obj_data*>::iterator iobj;
   int number, count, vnum;

   number = number_argument( argument, arg );

   if( get_trust(  ) >= LEVEL_SAVIOR && is_number( arg ) )
      vnum = atoi( arg.c_str() );
   else
      vnum = -1;

   count = 0;
   for( iobj = carrying.begin(); iobj != carrying.end(); ++iobj )
   {
      obj = (*iobj);

      if( obj->wear_loc != WEAR_NONE && can_see_obj( obj, false )
          && ( nifty_is_name( arg, obj->name ) || obj->pIndexData->vnum == vnum ) )
         if( ++count == number )
            return obj;
   }
   if( vnum != -1 )
      return NULL;

   /*
    * If we didn't find an exact match, run through the list of objects
    * again looking for prefix matching, ie swo == sword.
    * Added by Narn, Sept/96
    */
   count = 0;
   for( iobj = carrying.begin(); iobj != carrying.end(); ++iobj )
   {
      obj = (*iobj);

      if( obj->wear_loc != WEAR_NONE && can_see_obj( obj, false ) && nifty_is_name_prefix( arg, obj->name ) )
         if( ++count == number )
            return obj;
   }
   return NULL;
}

/*
 * Find an obj in the room or in inventory.
 */
obj_data *char_data::get_obj_here( string argument )
{
   obj_data *obj;

   if( ( obj = get_obj_list( this, argument, in_room->objects ) ) )
      return obj;

   if( ( obj = get_obj_carry( argument ) ) )
      return obj;

   if( ( obj = get_obj_wear( argument ) ) )
      return obj;

   return NULL;
}

/*
 * Find an obj in the world.
 */
obj_data *char_data::get_obj_world( string argument )
{
   string arg;
   obj_data *obj;
   list<obj_data*>::iterator iobj;
   int number, count, vnum;

   if( ( obj = get_obj_here( argument ) ) != NULL )
      return obj;

   number = number_argument( argument, arg );

   /*
    * Allow reference by vnum for saints+       -Thoric
    */
   if( get_trust(  ) >= LEVEL_SAVIOR && is_number( arg ) )
      vnum = atoi( arg.c_str() );
   else
      vnum = -1;

   count = 0;
   for( iobj = objlist.begin(); iobj != objlist.end(); ++iobj )
   {
      obj = (*iobj);

      if( can_see_obj( obj, true ) && ( nifty_is_name( arg, obj->name ) || vnum == obj->pIndexData->vnum ) )
         if( ( count += obj->count ) >= number )
            return obj;
   }

   /*
    * bail out if looking for a vnum 
    */
   if( vnum != -1 )
      return NULL;

   /*
    * If we didn't find an exact match, run through the list of objects
    * again looking for prefix matching, ie swo == sword.
    * Added by Narn, Sept/96
    */
   count = 0;
   for( iobj = objlist.begin(); iobj != objlist.end(); ++iobj )
   {
      obj = (*iobj);

      if( can_see_obj( obj, true ) && nifty_is_name_prefix( arg, obj->name ) )
         if( ( count += obj->count ) >= number )
            return obj;
   }
   return NULL;
}

/*
 * Find a piece of eq on a character.
 * Will pick the top layer if clothing is layered. - Thoric
 */
obj_data *char_data::get_eq( int iWear )
{
   obj_data *maxobj = NULL;
   list<obj_data*>::iterator iobj;

   for( iobj = carrying.begin(); iobj != carrying.end(); ++iobj )
   {
      obj_data *obj = (*iobj);

      if( obj->wear_loc == iWear )
      {
         if( !obj->pIndexData->layers )
            return obj;
         else if( !maxobj || obj->pIndexData->layers > maxobj->pIndexData->layers )
            maxobj = obj;
      }
   }
   return maxobj;
}

/*
 * Retrieve a character's carry capacity.
 * Vastly reduced (finally) due to containers - Thoric
 */
int char_data::can_carry_n(  )
{
   int penalty = 0;

   if( !isnpc(  ) && is_immortal(  ) )
      return level * 200;

   if( !isnpc(  ) && Class == CLASS_MONK )
      return 20;  /* Monks can only carry 20 items total */

   /*
    * Come now, never heard of pack animals people? Altered so pets can hold up to 10 - Samson 4-12-98 
    */
   if( is_pet() )
      return 10;

   if( has_actflag( ACT_IMMORTAL ) )
      return level * 200;

   if( get_eq( WEAR_WIELD ) )
      ++penalty;
   if( get_eq( WEAR_DUAL_WIELD ) )
      ++penalty;
   if( get_eq( WEAR_MISSILE_WIELD ) )
      ++penalty;
   if( get_eq( WEAR_HOLD ) )
      ++penalty;
   if( get_eq( WEAR_SHIELD ) )
      ++penalty;

   /*
    * Removed old formula, added something a bit more sensible here
    * Samson 4-12-98. Minimum of 15, (dex+str+level)-10 - penalty, or a max of 40. 
    */
   return URANGE( 15, ( get_curr_dex(  ) + get_curr_str(  ) + level ) - 10 - penalty, 40 );
}

/*
 * Retrieve a character's carry capacity.
 */
int char_data::can_carry_w(  )
{
   if( !isnpc(  ) && is_immortal(  ) )
      return 1000000;

   if( has_actflag( ACT_IMMORTAL ) )
      return 1000000;

   return str_app[get_curr_str(  )].carry;
}

/*
 * Equip a char with an obj.
 */
void char_data::equip( obj_data * obj, int iWear )
{
   obj_data *otmp;

   if( ( otmp = get_eq( iWear ) ) != NULL && ( !otmp->pIndexData->layers || !obj->pIndexData->layers ) )
   {
      bug( "%s: already equipped (%d).", __FUNCTION__, iWear );
      return;
   }

   if( obj->carried_by != this )
   {
      bug( "%s: obj (%s) not being carried by ch (%s)!", __FUNCTION__, obj->name, name );
      return;
   }

   obj->separate(  );   /* just in case */
   if( ( obj->extra_flags.test( ITEM_ANTI_EVIL ) && IS_EVIL(  ) )
       || ( obj->extra_flags.test( ITEM_ANTI_GOOD ) && IS_GOOD(  ) )
       || ( obj->extra_flags.test( ITEM_ANTI_NEUTRAL ) && IS_NEUTRAL(  ) ) )
   {
      /*
       * Thanks to Morgenes for the bug fix here!
       */
      if( loading_char != this )
      {
         act( AT_MAGIC, "You are zapped by $p and drop it.", this, obj, NULL, TO_CHAR );
         act( AT_MAGIC, "$n is zapped by $p and drops it.", this, obj, NULL, TO_ROOM );
      }
      if( obj->carried_by )
         obj->from_char(  );
      obj->to_room( in_room, this );
      oprog_zap_trigger( this, obj );
      if( IS_SAVE_FLAG( SV_ZAPDROP ) && !char_died(  ) )
         save(  );
      return;
   }

   armor -= obj->apply_ac( iWear );
   obj->wear_loc = iWear;

   carry_number -= obj->get_number(  );
   if( obj->extra_flags.test( ITEM_MAGIC ) )
      carry_weight -= obj->get_weight(  );

   affect_data *af;
   list<affect_data*>::iterator paf;
   for( paf = obj->pIndexData->affects.begin(); paf != obj->pIndexData->affects.end(); ++paf )
   {
      af = (*paf);
      affect_modify( af, true );
   }
   for( paf = obj->affects.begin(); paf != obj->affects.end(); ++paf )
   {
      af = (*paf);
      affect_modify( af, true );
   }
   if( obj->item_type == ITEM_LIGHT && obj->value[2] != 0 && in_room )
      ++in_room->light;

   return;
}

/*
 * Unequip a char with an obj.
 */
void char_data::unequip( obj_data * obj )
{
   if( obj->wear_loc == WEAR_NONE )
   {
      bug( "%s: %s already unequipped.", __FUNCTION__, name );
      return;
   }

   carry_number += obj->get_number(  );
   if( obj->extra_flags.test( ITEM_MAGIC ) )
      carry_weight += obj->get_weight(  );

   armor += obj->apply_ac( obj->wear_loc );
   obj->wear_loc = -1;

   affect_data *af;
   list<affect_data*>::iterator paf;
   for( paf = obj->pIndexData->affects.begin(); paf != obj->pIndexData->affects.end(); ++paf )
   {
      af = (*paf);
      affect_modify( af, false );
   }
   if( obj->carried_by )
   {
      for( paf = obj->affects.begin(); paf != obj->affects.end(); ++paf )
      {
         af = (*paf);
         affect_modify( af, false );
      }
   }
   update_aris(  );

   if( !obj->carried_by )
      return;

   if( obj->item_type == ITEM_LIGHT && obj->value[2] != 0 && in_room && in_room->light > 0 )
      --in_room->light;

   return;
}

/*
 * Apply only affected and RIS on a char
 */
void char_data::aris_affect( affect_data * paf )
{
   /*
    * How the hell have you SmaugDevs gotten away with this for so long! 
    */
   if( paf->bit >= 0 && paf->bit < MAX_AFFECTED_BY )
      affected_by.set( paf->bit );

   switch ( paf->location % REVERSE_APPLY )
   {
      default:
         break;

      case APPLY_AFFECT:
         if( paf->modifier >= 0 && paf->modifier < MAX_AFFECTED_BY )
            affected_by.set( paf->modifier );
         break;

      case APPLY_RESISTANT:
         resistant |= paf->rismod;
         break;

      case APPLY_IMMUNE:
         immune |= paf->rismod;
         break;

      case APPLY_SUSCEPTIBLE:
         susceptible |= paf->rismod;
         break;

      case APPLY_ABSORB:
         absorb |= paf->rismod;
         break;
   }
}

/*
 * Update affecteds and RIS for a character in case things get messed.
 * This should only really be used as a quick fix until the cause
 * of the problem can be hunted down. - FB
 * Last modified: June 30, 1997
 *
 * Quick fix?  Looks like a good solution for a lot of problems.
 */

/* Temp mod to bypass immortals so they can keep their mset affects,
 * just a band-aid until we get more time to look at it -- Blodkai */
void char_data::update_aris(  )
{
   if( is_immortal(  ) )
      return;

   /*
    * So chars using hide skill will continue to hide 
    */
   bool hiding = affected_by.test( AFF_HIDE );

   if( !isnpc(  ) ) /* Because we don't want NPCs to be purged of EVERY effect the have */
   {
      affected_by.reset(  );
      resistant.reset(  );
      immune.reset(  );
      susceptible.reset(  );
      absorb.reset(  );
      no_affected_by.reset(  );
      no_resistant.reset(  );
      no_immune.reset(  );
      no_susceptible.reset(  );
   }

   /*
    * Add in effects from race 
    * Because NPCs can have races MUCH higher than this that the table doesn't define yet 
    */
   if( race <= RACE_19 )
   {
      affected_by |= race_table[race]->affected;
      resistant |= race_table[race]->resist;
      susceptible |= race_table[race]->suscept;
   }

   /*
    * Add in effects from Class 
    * Because NPCs can have classes higher than the table allows 
    */
   if( Class <= CLASS_BARD )
   {
      affected_by |= class_table[Class]->affected;
      resistant |= class_table[Class]->resist;
      susceptible |= class_table[Class]->suscept;
   }
   ClassSpecificStuff(  );   /* Brought over from DOTD code - Samson 4-6-99 */

   /*
    * Add in effects from deities 
    */
   if( !isnpc(  ) && pcdata->deity )
   {
      if( pcdata->favor > pcdata->deity->affectednum[0] )
      {
         if( pcdata->deity->affected[0] > 0 )
            affected_by.set( pcdata->deity->affected[0] );
      }
      if( pcdata->favor > pcdata->deity->affectednum[1] )
      {
         if( pcdata->deity->affected[1] > 0 )
            affected_by.set( pcdata->deity->affected[1] );
      }
      if( pcdata->favor > pcdata->deity->affectednum[2] )
      {
         if( pcdata->deity->affected[2] > 0 )
            affected_by.set( pcdata->deity->affected[2] );
      }
      if( pcdata->favor > pcdata->deity->elementnum[0] )
      {
         if( pcdata->deity->element[0] != 0 )
            set_resist( pcdata->deity->element[0] );
      }
      if( pcdata->favor > pcdata->deity->elementnum[1] )
      {
         if( pcdata->deity->element[1] != 0 )
            set_resist( pcdata->deity->element[1] );
      }
      if( pcdata->favor > pcdata->deity->elementnum[2] )
      {
         if( pcdata->deity->element[2] != 0 )
            set_resist( pcdata->deity->element[2] );
      }
      if( pcdata->favor < pcdata->deity->susceptnum[0] )
      {
         if( pcdata->deity->suscept[0] != 0 )
            set_suscep( pcdata->deity->suscept[0] );
      }
      if( pcdata->favor < pcdata->deity->susceptnum[1] )
      {
         if( pcdata->deity->suscept[1] != 0 )
            set_suscep( pcdata->deity->suscept[1] );
      }
      if( pcdata->favor < pcdata->deity->susceptnum[2] )
      {
         if( pcdata->deity->suscept[2] != 0 )
            set_suscep( pcdata->deity->suscept[2] );
      }
   }

   /*
    * Add in effect from spells 
    */
   affect_data *af;
   list<affect_data*>::iterator paf;
   for( paf = affects.begin(); paf != affects.end(); ++paf )
   {
      af = (*paf);
      aris_affect( af );
   }

   /*
    * Add in effects from equipment 
    */
   list<obj_data*>::iterator iobj;
   for( iobj = carrying.begin(); iobj != carrying.end(); ++iobj )
   {
      obj_data *obj = (*iobj);

      if( obj->wear_loc != WEAR_NONE )
      {
         for( paf = obj->affects.begin(); paf != obj->affects.end(); ++paf )
         {
            af = (*paf);
            aris_affect( af );
         }

         for( paf = obj->pIndexData->affects.begin(); paf != obj->pIndexData->affects.end(); ++paf )
         {
            af = (*paf);
            aris_affect( af );
         }
      }
   }

   /*
    * Add in effects from the room 
    */
   if( in_room )  /* non-existant char booboo-fix --TRI */
   {
      for( paf = in_room->affects.begin(); paf != in_room->affects.end(); ++paf )
      {
         af = (*paf);
         aris_affect( af );
      }
   }

   /*
    * Add in effects for polymorph 
    */
   if( !isnpc(  ) && morph )
   {
      affected_by |= morph->affected_by;
      immune |= morph->immune;
      resistant |= morph->resistant;
      susceptible |= morph->suscept;
      absorb |= morph->absorb;
      /*
       * Right now only morphs have no_ things --Shaddai 
       */
      no_affected_by |= morph->no_affected_by;
      no_immune |= morph->no_immune;
      no_resistant |= morph->no_resistant;
      no_susceptible |= morph->no_suscept;
   }

   /*
    * If they were hiding before, make them hiding again 
    */
   if( hiding )
      affected_by.set( AFF_HIDE );

   return;
}

// For an NPC to be a pet, all of these conditions must be met.
bool char_data::is_pet()
{
   if( !isnpc() )
      return false;
   if( !has_actflag( ACT_PET ) )
      return false;
   if( !is_affected( gsn_charm_person ) )
      return false;
   if( !master )
      return false;
   return true;
}

/*
 * Modify a skill (hopefully) properly - Thoric
 *
 * On "adding" a skill modifying affect, the value set is unimportant
 * upon removing the affect, the skill it enforced to a proper range.
 */
void char_data::modify_skill( int sn, int mod, bool fAdd )
{
   if( !isnpc(  ) )
   {
      if( fAdd )
         pcdata->learned[sn] += mod;
      else
         pcdata->learned[sn] = URANGE( 0, pcdata->learned[sn] + mod, GET_ADEPT( sn ) );
   }
}

/*
 * Apply or remove an affect to a character.
 */
void char_data::affect_modify( affect_data * paf, bool fAdd )
{
   obj_data *wield;
   int mod, mod2 = -1;
   struct skill_type *skill;
   ch_ret retcode;

   mod = paf->modifier;

   /*
    * God, this is a hackish mess! Thanks alot Smaugdevs! 
    */
   if( paf->location == APPLY_AFFECT || paf->location == APPLY_REMOVE || paf->location == APPLY_EXT_AFFECT )
   {
      if( paf->bit < 0 || paf->bit >= MAX_AFFECTED_BY )
      {
         bug( "%s: %s: Unknown bitflag: '%d' for location %d, with modifier %d",
              __FUNCTION__, name, paf->bit, paf->location, paf->modifier );
         return;
      }
      mod2 = mod;
   }

   if( fAdd )
   {
      if( paf->bit >= 0 && paf->bit < MAX_AFFECTED_BY )
         set_aflag( paf->bit );

      if( paf->location % REVERSE_APPLY == APPLY_RECURRINGSPELL )
      {
         mod = abs( mod );
         if( IS_VALID_SN( mod ) && ( skill = skill_table[mod] ) != NULL && skill->type == SKILL_SPELL )
            set_aflag( AFF_RECURRINGSPELL );
         else
            bug( "%s: (%s) APPLY_RECURRINGSPELL with bad sn %d", __FUNCTION__, name, mod );
         return;
      }
   }
   else
   {
      if( paf->bit >= 0 && paf->bit < MAX_AFFECTED_BY )
         affected_by.reset( paf->bit );
      /*
       * might be an idea to have a duration removespell which returns
       * the spell after the duration... but would have to store
       * the removed spell's information somewhere...    -Thoric
       */
      if( paf->location % REVERSE_APPLY == APPLY_RECURRINGSPELL )
      {
         mod = abs( mod );
         if( !IS_VALID_SN( mod ) || ( skill = skill_table[mod] ) == NULL || skill->type != SKILL_SPELL )
            bug( "%s: (%s) APPLY_RECURRINGSPELL with bad sn %d", __FUNCTION__, name, mod );
         unset_aflag( AFF_RECURRINGSPELL );
         return;
      }

      switch ( paf->location % REVERSE_APPLY )
      {
         case APPLY_AFFECT:
            affected_by.reset( mod2 );
            return;
         case APPLY_EXT_AFFECT:
            affected_by.reset( mod2 );
            return;
         case APPLY_RESISTANT:
            resistant &= ~( paf->rismod );
            return;
         case APPLY_IMMUNE:
            immune &= ~( paf->rismod );
            return;
         case APPLY_SUSCEPTIBLE:
            susceptible &= ~( paf->rismod );
            return;
         case APPLY_ABSORB:
            absorb &= ~( paf->rismod );
            return;
         case APPLY_REMOVE:
            affected_by.set( mod2 );
            return;
         default:
            break;
      }
      mod = 0 - mod;
   }

   switch ( paf->location % REVERSE_APPLY )
   {
      default:
         bug( "%s: unknown location %d.", __FUNCTION__, paf->location );
         return;

      case APPLY_NONE:
         break;
      case APPLY_STR:
         mod_str += mod;
         break;
      case APPLY_DEX:
         mod_dex += mod;
         break;
      case APPLY_INT:
         mod_int += mod;
         break;
      case APPLY_WIS:
         mod_wis += mod;
         break;
      case APPLY_CON:
         mod_con += mod;
         break;
      case APPLY_CHA:
         mod_cha += mod;
         break;
      case APPLY_LCK:
         mod_lck += mod;
         break;
      case APPLY_ALLSTATS:
         mod_str += mod;
         mod_dex += mod;
         mod_int += mod;
         mod_wis += mod;
         mod_con += mod;
         mod_cha += mod;
         mod_lck += mod;
         break;
      case APPLY_SEX:
         sex = ( sex + mod ) % 3;
         if( sex < 0 )
            sex += 2;
         sex = URANGE( 0, sex, 2 );
         break;
      case APPLY_ATTACKS:
         numattacks += mod;
         break;
      case APPLY_CLASS:
         break;
      case APPLY_LEVEL:
         break;
      case APPLY_GOLD:
         break;
      case APPLY_EXP:
         break;
      case APPLY_SF:
         spellfail += mod;
         break;
      case APPLY_RACE:
         race += mod;
         break;
      case APPLY_AGE:
         if( !isnpc(  ) )
            pcdata->age_bonus += mod;
         break;
      case APPLY_HEIGHT:
         height += mod;
         break;
      case APPLY_WEIGHT:
         weight += mod;
         break;
      case APPLY_MANA:
         max_mana += mod;
         break;
      case APPLY_HIT:
         max_hit += mod;
         break;
      case APPLY_MOVE:
         max_move += mod;
         break;
      case APPLY_MANA_REGEN:
         mana_regen += mod;
         break;
      case APPLY_HIT_REGEN:
         hit_regen += mod;
         break;
      case APPLY_MOVE_REGEN:
         move_regen += mod;
         break;
      case APPLY_ANTIMAGIC:
         amp += mod;
         break;
      case APPLY_AC:
         armor += mod;
         break;
      case APPLY_HITROLL:
         hitroll += mod;
         break;
      case APPLY_DAMROLL:
         damroll += mod;
         break;
      case APPLY_HITNDAM:
         hitroll += mod;
         damroll += mod;
         break;
      case APPLY_SAVING_POISON:
         saving_poison_death += mod;
         break;
      case APPLY_SAVING_ROD:
         saving_wand += mod;
         break;
      case APPLY_SAVING_PARA:
         saving_para_petri += mod;
         break;
      case APPLY_SAVING_BREATH:
         saving_breath += mod;
         break;
      case APPLY_SAVING_SPELL:
         saving_spell_staff += mod;
         break;
      case APPLY_SAVING_ALL:
         saving_poison_death += mod;
         saving_para_petri += mod;
         saving_breath += mod;
         saving_spell_staff += mod;
         saving_wand += mod;
         break;
      case APPLY_AFFECT:
         affected_by.set( mod2 );
         break;
      case APPLY_EXT_AFFECT:
         affected_by.set( mod2 );
         break;
      case APPLY_RESISTANT:
         resistant |= paf->rismod;
         break;
      case APPLY_IMMUNE:
         immune |= paf->rismod;
         break;
      case APPLY_SUSCEPTIBLE:
         susceptible |= paf->rismod;
         break;
      case APPLY_ABSORB:
         absorb |= paf->rismod;
         break;
      case APPLY_WEAPONSPELL:   /* see fight.c */
         break;
      case APPLY_REMOVE:
         affected_by.reset( mod2 );
         break;

      case APPLY_FULL:
         if( !isnpc(  ) )
            pcdata->condition[COND_FULL] = URANGE( 0, pcdata->condition[COND_FULL] + mod, sysdata->maxcondval );
         break;

      case APPLY_THIRST:
         if( !isnpc(  ) )
            pcdata->condition[COND_THIRST] = URANGE( 0, pcdata->condition[COND_THIRST] + mod, sysdata->maxcondval );
         break;

      case APPLY_DRUNK:
         if( !isnpc(  ) )
            pcdata->condition[COND_DRUNK] = URANGE( 0, pcdata->condition[COND_DRUNK] + mod, sysdata->maxcondval );
         break;

      case APPLY_MENTALSTATE:
         mental_state = URANGE( -100, mental_state + mod, 100 );
         break;

      case APPLY_CONTAGIOUS:
         break;
      case APPLY_ODOR:
         break;
      case APPLY_STRIPSN:
         if( IS_VALID_SN( mod ) )
            affect_strip( mod );
         else
            bug( "%s: APPLY_STRIPSN invalid sn %d", __FUNCTION__, mod );
         break;

         /*
          * spell cast upon wear/removal of an object -Thoric 
          */
      case APPLY_WEARSPELL:
      case APPLY_REMOVESPELL:
         if( ( in_room && in_room->flags.test( ROOM_NO_MAGIC ) ) || has_immune( RIS_MAGIC ) || ( ( paf->location % REVERSE_APPLY ) == APPLY_WEARSPELL && !fAdd ) || ( ( paf->location % REVERSE_APPLY ) == APPLY_REMOVESPELL && !fAdd ) || saving_char == this  /* so save/quit doesn't trigger */
             || loading_char == this ) /* so loading doesn't trigger */
            return;

         mod = abs( mod );
         if( IS_VALID_SN( mod ) && ( skill = skill_table[mod] ) != NULL && skill->type == SKILL_SPELL )
            if( ( retcode = ( *skill->spell_fun ) ( mod, level, this, this ) ) == rCHAR_DIED || char_died(  ) )
               return;
         break;


      /*
       * skill apply types  -Thoric 
       */
      case APPLY_PALM: /* not implemented yet */
         break;
      case APPLY_PEEK:
         modify_skill( gsn_peek, mod, fAdd );
         break;
      case APPLY_TRACK:
         modify_skill( gsn_track, mod, fAdd );
         break;
      case APPLY_HIDE:
         modify_skill( gsn_hide, mod, fAdd );
         break;
      case APPLY_STEAL:
         modify_skill( gsn_steal, mod, fAdd );
         break;
      case APPLY_SNEAK:
         modify_skill( gsn_sneak, mod, fAdd );
         break;
      case APPLY_PICK:
         modify_skill( gsn_pick_lock, mod, fAdd );
         break;
      case APPLY_BACKSTAB:
         modify_skill( gsn_backstab, mod, fAdd );
         break;
         /*
          * case APPLY_DETRAP:   modify_skill( gsn_find_traps,mod, fAdd);  break; 
          */
      case APPLY_DODGE:
         modify_skill( gsn_dodge, mod, fAdd );
         break;
      case APPLY_SCAN:
         break;
      case APPLY_GOUGE:
         modify_skill( gsn_gouge, mod, fAdd );
         break;
      case APPLY_SEARCH:
         modify_skill( gsn_search, mod, fAdd );
         break;
      case APPLY_DIG:
         modify_skill( gsn_dig, mod, fAdd );
         break;
      case APPLY_MOUNT:
         modify_skill( gsn_mount, mod, fAdd );
         break;
      case APPLY_DISARM:
         modify_skill( gsn_disarm, mod, fAdd );
         break;
      case APPLY_KICK:
         modify_skill( gsn_kick, mod, fAdd );
         break;
      case APPLY_PARRY:
         modify_skill( gsn_parry, mod, fAdd );
         break;
      case APPLY_BASH:
         modify_skill( gsn_bash, mod, fAdd );
         break;
      case APPLY_STUN:
         modify_skill( gsn_stun, mod, fAdd );
         break;
      case APPLY_PUNCH:
         modify_skill( gsn_punch, mod, fAdd );
         break;
      case APPLY_CLIMB:
         modify_skill( gsn_climb, mod, fAdd );
         break;
      case APPLY_GRIP:
         modify_skill( gsn_grip, mod, fAdd );
         break;
      case APPLY_SCRIBE:
         modify_skill( gsn_scribe, mod, fAdd );
         break;
      case APPLY_BREW:
         modify_skill( gsn_brew, mod, fAdd );
         break;
      case APPLY_COOK:
         modify_skill( gsn_cook, mod, fAdd );
         break;

      case APPLY_EXTRAGOLD:  /* SHUT THE HELL UP LOCATION 89! */
         if( !isnpc(  ) )
            pcdata->exgold += mod;
         break;

         /*
          *  Applys that dont generate effects on pcs
          */
      case APPLY_EAT_SPELL:
      case APPLY_ALIGN_SLAYER:
      case APPLY_RACE_SLAYER:
         break;

         /*
          * Room apply types
          */
      case APPLY_ROOMFLAG:
      case APPLY_SECTORTYPE:
      case APPLY_ROOMLIGHT:
      case APPLY_TELEVNUM:
         break;
   }

   /*
    * Check for weapon wielding.
    * Guard against recursion (for weapons with affects).
    */
   if( !isnpc(  ) && saving_char != this && ( wield = get_eq( WEAR_WIELD ) ) != NULL
       && wield->get_weight(  ) > str_app[get_curr_str(  )].wield )
   {
      static int depth;

      if( depth == 0 )
      {
         ++depth;
         act( AT_ACTION, "You are too weak to wield $p any longer.", this, wield, NULL, TO_CHAR );
         act( AT_ACTION, "$n stops wielding $p.", this, wield, NULL, TO_ROOM );
         unequip( wield );
         --depth;
      }
   }
   return;
}

/*
 * Give an affect to a char.
 */
void char_data::affect_to_char( affect_data * paf )
{
   affect_data *paf_new;

   if( !this )
   {
      bug( "%s: (NULL, %d)", __FUNCTION__, paf ? paf->type : 0 );
      return;
   }

   if( !paf )
   {
      bug( "%s: (%s, NULL)", __FUNCTION__, name );
      return;
   }

   paf_new = new affect_data;
   affects.push_back( paf_new );
   paf_new->type = paf->type;
   paf_new->duration = paf->duration;
   paf_new->location = paf->location;
   paf_new->modifier = paf->modifier;
   paf_new->rismod = paf->rismod;
   paf_new->bit = paf->bit;

   affect_modify( paf_new, true );
   return;
}

/*
 * Remove an affect from a char.
 */
void char_data::affect_remove( affect_data * paf )
{
   if( affects.empty() )
   {
      bug( "%s: (%s, %d): no affect.", __FUNCTION__, name, paf ? paf->type : 0 );
      return;
   }

   affect_modify( paf, false );

   affects.remove( paf );
   deleteptr( paf );
   return;
}

/*
 * Strip all affects of a given sn.
 */
void char_data::affect_strip( int sn )
{
   list<affect_data*>::iterator paf;

   for( paf = affects.begin(); paf != affects.end(); )
   {
      affect_data *aff = (*paf);
      ++paf;

      if( aff->type == sn )
         affect_remove( aff );
   }
   return;
}

/*
 * Return true if a char is affected by a spell.
 */
bool char_data::is_affected( int sn )
{
   list<affect_data*>::iterator paf;

   for( paf = affects.begin(); paf != affects.end(); ++paf )
   {
      affect_data *af = (*paf);

      if( af->type == sn )
         return true;
   }
   return false;
}

/*
 * Add or enhance an affect.
 * Limitations put in place by Thoric, they may be high... but at least
 * they're there :)
 */
void char_data::affect_join( affect_data * paf )
{
   list<affect_data*>::iterator paf_old;

   for( paf_old = affects.begin(); paf_old != affects.end(); ++paf_old )
   {
      affect_data *af = (*paf_old);

      if( af->type == paf->type )
      {
         paf->duration = UMIN( 32500, paf->duration + af->duration );
         if( paf->modifier )
            paf->modifier = UMIN( 5000, paf->modifier + af->modifier );
         else
            paf->modifier = af->modifier;
         affect_remove( af );
         break;
      }
   }
   affect_to_char( paf );
   return;
}

/*
 * Show an affect verbosely to a character			-Thoric
 */
void char_data::showaffect( affect_data * paf )
{
   char buf[MSL];
   int i;

   if( !paf )
   {
      bug( "%s: NULL paf", __FUNCTION__ );
      return;
   }
   if( paf->location != APPLY_NONE && ( paf->modifier != 0 || paf->rismod.any(  ) ) )
   {
      switch ( paf->location )
      {
         default:
            snprintf( buf, MSL, "&wAffects: &B%15s&w by &B%3d&w.\r\n", a_types[paf->location], paf->modifier );
            break;

         case APPLY_AFFECT:
            snprintf( buf, MSL, "&wAffects: &B%15s&w by &B%s&w\r\n", a_types[paf->location], aff_flags[paf->modifier] );
            break;

         case APPLY_WEAPONSPELL:
         case APPLY_WEARSPELL:
         case APPLY_REMOVESPELL:
         case APPLY_EAT_SPELL:
            snprintf( buf, MSL, "&wCasts spell: &B'%s'&w\r\n",
                      IS_VALID_SN( paf->modifier ) ? skill_table[paf->modifier]->name : "unknown" );
            break;

         case APPLY_RESISTANT:
         case APPLY_IMMUNE:
         case APPLY_SUSCEPTIBLE:
         case APPLY_ABSORB:
            snprintf( buf, MSL, "&wAffects: &B%15s&w by &B", a_types[paf->location] );
            for( i = 0; i < MAX_RIS_FLAG; ++i )
               if( paf->rismod.test( i ) )
               {
                  mudstrlcat( buf, " ", MSL );
                  mudstrlcat( buf, ris_flags[i], MSL );
               }
            mudstrlcat( buf, "&w\r\n", MSL );
            break;
      }
      print( buf );
   }
}

/* Auto-calc formula to set numattacks for PCs & mobs - Samson 5-6-99 */
void char_data::set_numattacks(  )
{
   /*
    * Attack formulas modified to use Shard formulas - Samson 5-3-99 
    */
   numattacks = 1.0;

   switch ( Class )
   {
      case CLASS_MAGE:
      case CLASS_NECROMANCER:
         /*
          * 2.72 attacks at level 100 
          */
         numattacks += ( float )( UMIN( 86, level ) * .02 );
         break;

      case CLASS_CLERIC:
      case CLASS_DRUID:
         /*
          * 3.5 attacks at level 100 
          */
         numattacks += ( float )( level / 40.0 );
         break;

      case CLASS_WARRIOR:
      case CLASS_PALADIN:
      case CLASS_RANGER:
      case CLASS_ANTIPALADIN:
         /*
          * 5.3 attacks at level 100 
          */
         numattacks += ( float )( UMIN( 86, level ) * .05 );
         break;

      case CLASS_ROGUE:
      case CLASS_BARD:
         /*
          * 4 attacks at level 100 
          */
         numattacks += ( float )( level / 33.3 );
         break;

      case CLASS_MONK:
         /*
          * 7.6 attacks at level 100 
          */
         numattacks += ( float )( level / 15.0 );
         break;

      default:
         break;
   }
   return;
}

int char_data::char_ego(  )
{
   int p_ego, tmp;

   if( !isnpc(  ) )
   {
      p_ego = level;
      tmp = ( ( get_curr_int(  ) + get_curr_wis(  ) + get_curr_cha(  ) + get_curr_lck(  ) ) / 4 );
      tmp = tmp - 17;
      p_ego += tmp;
   }
   else
      p_ego = 10000;
   if( p_ego <= 0 )
      p_ego = level;
   return p_ego;
}

timer_data *char_data::get_timerptr( short type )
{
   list<timer_data*>::iterator cht;

   for( cht = timers.begin(); cht != timers.end(); ++cht )
   {
      timer_data *ct = (*cht);

      if( ct->type == type )
         return ct;
   }
   return NULL;
}

short char_data::get_timer( short type )
{
   timer_data *chtimer;

   if( ( chtimer = get_timerptr( type ) ) != NULL )
      return chtimer->count;
   else
      return 0;
}

/*
 * Add a timer to ch - Thoric
 * Support for "call back" time delayed commands
 */
void char_data::add_timer( short type, short count, DO_FUN * fun, int value )
{
   timer_data *chtimer = NULL;

   if( !( chtimer = get_timerptr( type ) ) )
   {
      chtimer = new timer_data;
      chtimer->count = count;
      chtimer->type = type;
      chtimer->do_fun = fun;
      chtimer->value = value;
      timers.push_back( chtimer );
   }
   else
   {
      chtimer->count = count;
      chtimer->do_fun = fun;
      chtimer->value = value;
   }
}

void char_data::extract_timer( timer_data * chtimer )
{
   timers.remove( chtimer );
   deleteptr( chtimer );
   return;
}

void char_data::remove_timer( short type )
{
   timer_data *chtimer = NULL;

   if( !( chtimer = get_timerptr( type ) ) )
      return;
   extract_timer( chtimer );
}

bool char_data::in_hard_range( area_data * tarea )
{
   if( is_immortal(  ) )
      return true;
   else if( isnpc(  ) )
      return true;
   else if( level >= tarea->low_hard_range && level <= tarea->hi_hard_range )
      return true;
   else
      return false;
}

/*
 * Improve mental state - Thoric
 */
void char_data::better_mental_state( int mod )
{
   int c = URANGE( 0, abs( mod ), 20 );
   int con = get_curr_con(  );

   if( is_immortal(  ) )
   {
      mental_state = 0;
      return;
   }

   c += number_percent(  ) < con ? 1 : 0;

   if( mental_state < 0 )
      mental_state = URANGE( -100, mental_state + c, 0 );
   else if( mental_state > 0 )
      mental_state = URANGE( 0, mental_state - c, 100 );
}

/*
 * Deteriorate mental state					-Thoric
 */
void char_data::worsen_mental_state( int mod )
{
   int c = URANGE( 0, abs( mod ), 20 );
   int con = get_curr_con(  );

   if( is_immortal(  ) )
   {
      mental_state = 0;
      return;
   }

   c -= number_percent(  ) < con ? 1 : 0;
   if( c < 1 )
      return;

   if( mental_state < 0 )
      mental_state = URANGE( -100, mental_state - c, 100 );
   else if( mental_state > 0 )
      mental_state = URANGE( -100, mental_state + c, 100 );
   else
      mental_state -= c;
}

/*
 * Move a char out of a room.
 */
void char_data::from_room(  )
{
   obj_data *obj;
   list<affect_data*>::iterator paf;

   if( !in_room )
   {
      bug( "%s: %s not in a room!", __FUNCTION__, name );
      return;
   }

   if( !isnpc(  ) )
      --in_room->area->nplayer;

   if( ( obj = get_eq( WEAR_LIGHT ) ) != NULL && obj->item_type == ITEM_LIGHT && obj->value[2] != 0 && in_room->light > 0 )
      --in_room->light;

   /*
    * Character's affect on the room
    */
   affect_data *af;
   for( paf = affects.begin(); paf != affects.end(); ++paf )
   {
      af = (*paf);
      in_room->room_affect( af, false );
   }

   /*
    * Room's affect on the character
    */
   if( !char_died(  ) )
   {
      for( paf = in_room->affects.begin(); paf != in_room->affects.end(); ++paf )
      {
         af = (*paf);
         affect_modify( af, false );
      }

      if( char_died(  ) )  /* could die from removespell, etc */
         return;
   }

   in_room->people.remove( this );

   was_in_room = in_room;
   in_room = NULL;

   if( !isnpc(  ) && get_timer( TIMER_SHOVEDRAG ) > 0 )
      remove_timer( TIMER_SHOVEDRAG );

   return;
}

/*
 * Move a char into a room.
 */
bool char_data::to_room( room_index * pRoomIndex )
{
   obj_data *obj;
   list<affect_data*>::iterator paf;

   // Ok, asshole code, lets see you get past this!
   if( !pRoomIndex || !get_room_index( pRoomIndex->vnum ) )
   {
      bug( "%s: %s -> NULL room!  Putting char in limbo (%d)", __FUNCTION__, name, ROOM_VNUM_LIMBO );
      /*
       * This used to just return, but there was a problem with crashing
       * and I saw no reason not to just put the char in limbo.  -Narn
       */
      if( !( pRoomIndex = get_room_index( ROOM_VNUM_LIMBO ) ) )
      {
         bug( "FATAL: Limbo room is MISSING! Expect crash! %s:%s, line %d", __FILE__, __FUNCTION__, __LINE__ );
         return false;
      }
   }

   in_room = pRoomIndex;
   if( home_vnum < 1 )
      home_vnum = in_room->vnum;

   /*
    * Yep - you guessed it. Everything that needs to happen to add Zone X to Player Y's list
    * * takes place in this one teeny weeny little function found in afk.c :) 
    */
   update_visits( pRoomIndex );

   pRoomIndex->people.push_back( this );

   if( !isnpc(  ) )
      ++in_room->area->nplayer;

   if( ( obj = get_eq( WEAR_LIGHT ) ) != NULL && obj->item_type == ITEM_LIGHT && obj->value[2] != 0 )
      ++pRoomIndex->light;

   /*
    * Room's effect on the character
    */
   affect_data *af;
   if( !char_died(  ) )
   {
      for( paf = pRoomIndex->affects.begin(); paf != pRoomIndex->affects.end(); ++paf )
      {
         af = (*paf);
         affect_modify( af, true );
      }

      if( char_died(  ) )  /* could die from a wearspell, etc */
         return true;
   }

   /*
    * Character's effect on the room
    */
   for( paf = affects.begin(); paf != affects.end(); ++paf )
   {
      af = (*paf);
      pRoomIndex->room_affect( af, true );
   }

   if( !isnpc(  ) && pRoomIndex->flags.test( ROOM_SAFE ) && get_timer( TIMER_SHOVEDRAG ) <= 0 )
      add_timer( TIMER_SHOVEDRAG, 10, NULL, 0 );  /*-30 Seconds-*/

   /*
    * Delayed Teleport rooms - Thoric
    * Should be the last thing checked in this function
    */
   if( pRoomIndex->flags.test( ROOM_TELEPORT ) && pRoomIndex->tele_delay > 0 )
   {
      teleport_data *teleport;
      list<teleport_data*>::iterator tele;

      for( tele = teleportlist.begin(); tele != teleportlist.end(); ++tele )
      {
         teleport = (*tele);

         if( teleport->room == pRoomIndex )
            return true;
      }
      teleport = new teleport_data;
      teleport->room = pRoomIndex;
      teleport->timer = pRoomIndex->tele_delay;
      teleportlist.push_back( teleport );
   }
   if( !was_in_room )
      was_in_room = in_room;

   if( on )
   {
      on = NULL;
      position = POS_STANDING;
   }
   if( position != POS_STANDING && tempnum != -9998 ) /* Hackish hotboot fix! WOO! */
      position = POS_STANDING;
   return true;
}

/*
 * Scryn, standard luck check 2/2/96
 */
bool char_data::chance( short percent )
{
   short deity_factor, ms;

/* Code for clan stuff put in by Narn, Feb/96.  The idea is to punish clan
members who don't keep their alignment in tune with that of their clan by
making it harder for them to succeed at pretty much everything.  Clan_factor
will vary from 1 to 3, with 1 meaning there is no effect on the player's
change of success, and with 3 meaning they have half the chance of doing
whatever they're trying to do. 

Note that since the neutral clannies can only be off by 1000 points, their
maximum penalty will only be half that of the other clan types.

   if( pcdata && pcdata->IS_CLANNED() )
      clan_factor = 1 + abs( alignment - pcdata->clan->alignment ) / 1000;
   else
      clan_factor = 1;
*/
/* Mental state bonus/penalty:  Your mental state is a ranged value with
 * zero (0) being at a perfect mental state (bonus of 10).
 * negative values would reflect how sedated one is, and
 * positive values would reflect how stimulated one is.
 * In most circumstances you'd do best at a perfectly balanced state.
 */

   if( IS_DEVOTED( this ) )
      deity_factor = pcdata->favor / -500;
   else
      deity_factor = 0;

   ms = 10 - abs( mental_state );

   if( ( number_percent(  ) - get_curr_lck(  ) + 13 - ms ) + deity_factor <= percent )
      return true;
   else
      return false;
}

int IsHumanoid( char_data * ch )
{
   switch ( ch->race )
   {
      case RACE_HUMAN:
      case RACE_GNOME:
      case RACE_HIGH_ELF:
      case RACE_GOLD_ELF:
      case RACE_WILD_ELF:
      case RACE_SEA_ELF:
      case RACE_DWARF:
      case RACE_MINOTAUR:
      case RACE_DUERGAR:
      case RACE_CENTAUR:
      case RACE_HALFLING:
      case RACE_ORC:
      case RACE_LYCANTH:
      case RACE_UNDEAD:
      case RACE_UNDEAD_VAMPIRE:
      case RACE_UNDEAD_LICH:
      case RACE_UNDEAD_WIGHT:
      case RACE_UNDEAD_GHAST:
      case RACE_UNDEAD_SPECTRE:
      case RACE_UNDEAD_ZOMBIE:
      case RACE_UNDEAD_SKELETON:
      case RACE_UNDEAD_GHOUL:
      case RACE_GIANT:
      case RACE_GIANT_HILL:
      case RACE_GIANT_FROST:
      case RACE_GIANT_FIRE:
      case RACE_GIANT_CLOUD:
      case RACE_GIANT_STORM:
      case RACE_GIANT_STONE:
      case RACE_GOBLIN:
      case RACE_DEVIL:
      case RACE_TROLL:
      case RACE_VEGMAN:
      case RACE_MFLAYER:
      case RACE_ENFAN:
      case RACE_PATRYN:
      case RACE_SARTAN:
      case RACE_ROO:
      case RACE_SMURF:
      case RACE_TROGMAN:
      case RACE_IGUANADON:
      case RACE_SKEXIE:
      case RACE_TYTAN:
      case RACE_DROW:
      case RACE_GOLEM:
      case RACE_DEMON:
      case RACE_DRAAGDIM:
      case RACE_ASTRAL:
      case RACE_GOD:
      case RACE_HALF_ELF:
      case RACE_HALF_ORC:
      case RACE_HALF_TROLL:
      case RACE_HALF_OGRE:
      case RACE_HALF_GIANT:
      case RACE_GNOLL:
      case RACE_TIEFLING:
      case RACE_AASIMAR:
      case RACE_VAGABOND:
         return true;

      default:
         return false;
   }
}

int IsRideable( char_data * ch )
{
   if( ch->isnpc(  ) )
   {
      switch ( ch->race )
      {
         case RACE_HORSE:
         case RACE_DRAGON:
         case RACE_DRAGON_RED:
         case RACE_DRAGON_BLACK:
         case RACE_DRAGON_GREEN:
         case RACE_DRAGON_WHITE:
         case RACE_DRAGON_BLUE:
         case RACE_DRAGON_SILVER:
         case RACE_DRAGON_GOLD:
         case RACE_DRAGON_BRONZE:
         case RACE_DRAGON_COPPER:
         case RACE_DRAGON_BRASS:
            return true;
         default:
            return false;
      }
   }
   else
      return false;
}

int IsUndead( char_data * ch )
{
   switch ( ch->race )
   {
      case RACE_UNDEAD:
      case RACE_GHOST:
      case RACE_UNDEAD_VAMPIRE:
      case RACE_UNDEAD_LICH:
      case RACE_UNDEAD_WIGHT:
      case RACE_UNDEAD_GHAST:
      case RACE_UNDEAD_SPECTRE:
      case RACE_UNDEAD_ZOMBIE:
      case RACE_UNDEAD_SKELETON:
      case RACE_UNDEAD_GHOUL:
      case RACE_UNDEAD_SHADOW:
         return true;
      default:
         return false;
   }
}

int IsLycanthrope( char_data * ch )
{
   switch ( ch->race )
   {
      case RACE_LYCANTH:
         return true;
      default:
         return false;
   }
}

int IsDiabolic( char_data * ch )
{
   switch ( ch->race )
   {
      case RACE_DEMON:
      case RACE_DEVIL:
      case RACE_TIEFLING:
      case RACE_DAEMON:
      case RACE_DEMODAND:
         return true;
      default:
         return false;
   }
}

int IsReptile( char_data * ch )
{
   switch ( ch->race )
   {
      case RACE_REPTILE:
      case RACE_DRAGON:
      case RACE_DRAGON_RED:
      case RACE_DRAGON_BLACK:
      case RACE_DRAGON_GREEN:
      case RACE_DRAGON_WHITE:
      case RACE_DRAGON_BLUE:
      case RACE_DRAGON_SILVER:
      case RACE_DRAGON_GOLD:
      case RACE_DRAGON_BRONZE:
      case RACE_DRAGON_COPPER:
      case RACE_DRAGON_BRASS:
      case RACE_DINOSAUR:
      case RACE_SNAKE:
      case RACE_TROGMAN:
      case RACE_IGUANADON:
      case RACE_SKEXIE:
         return true;
      default:
         return false;
   }
}

int HasHands( char_data * ch )
{
   if( IsHumanoid( ch ) )
      return true;
   if( IsUndead( ch ) )
      return true;
   if( IsLycanthrope( ch ) )
      return true;
   if( IsDiabolic( ch ) )
      return true;
   if( ch->race == RACE_GOLEM || ch->race == RACE_SPECIAL )
      return true;
   if( ch->is_immortal(  ) )
      return true;
   return false;
}

int IsPerson( char_data * ch )
{
   switch ( ch->race )
   {
      case RACE_HUMAN:
      case RACE_HIGH_ELF:
      case RACE_WILD_ELF:
      case RACE_DROW:
      case RACE_DWARF:
      case RACE_DUERGAR:
      case RACE_HALFLING:
      case RACE_GNOME:
      case RACE_DEEP_GNOME:
      case RACE_GOLD_ELF:
      case RACE_SEA_ELF:
      case RACE_GOBLIN:
      case RACE_ORC:
      case RACE_TROLL:
      case RACE_SKEXIE:
      case RACE_MFLAYER:
      case RACE_HALF_ORC:
      case RACE_HALF_OGRE:
      case RACE_HALF_GIANT:
         return true;

      default:
         return false;
   }
}

int IsDragon( char_data * ch )
{
   switch ( ch->race )
   {
      case RACE_DRAGON:
      case RACE_DRAGON_RED:
      case RACE_DRAGON_BLACK:
      case RACE_DRAGON_GREEN:
      case RACE_DRAGON_WHITE:
      case RACE_DRAGON_BLUE:
      case RACE_DRAGON_SILVER:
      case RACE_DRAGON_GOLD:
      case RACE_DRAGON_BRONZE:
      case RACE_DRAGON_COPPER:
      case RACE_DRAGON_BRASS:
         return true;
      default:
         return false;
   }
}

int IsGiantish( char_data * ch )
{
   switch ( ch->race )
   {
      case RACE_ENFAN:
      case RACE_GOBLIN:
      case RACE_ORC:
      case RACE_GIANT:
      case RACE_GIANT_HILL:
      case RACE_GIANT_FROST:
      case RACE_GIANT_FIRE:
      case RACE_GIANT_CLOUD:
      case RACE_GIANT_STORM:
      case RACE_GIANT_STONE:
      case RACE_TYTAN:
      case RACE_TROLL:
      case RACE_DRAAGDIM:

      case RACE_HALF_ORC:
      case RACE_HALF_OGRE:
      case RACE_HALF_GIANT:
         return true;
      default:
         return false;
   }
}

int IsGiant( char_data * ch )
{
   if( !ch )
      return false;

   switch ( ch->race )
   {
      case RACE_GIANT:
      case RACE_GIANT_HILL:
      case RACE_GIANT_FROST:
      case RACE_GIANT_FIRE:
      case RACE_GIANT_CLOUD:
      case RACE_GIANT_STORM:
      case RACE_GIANT_STONE:
      case RACE_HALF_GIANT:
      case RACE_TYTAN:
      case RACE_GOD:
      case RACE_GIANT_SKELETON:
      case RACE_TROLL:
         return true;
      default:
         return false;
   }
}

void race_bodyparts( char_data * ch )
{
   if( ch->race < MAX_RACE )
   {
      if( race_table[ch->race]->body_parts.any(  ) )
         ch->set_bparts( race_table[ch->race]->body_parts );
      return;
   }

   ch->set_bpart( PART_GUTS );

   if( IsHumanoid( ch ) || IsPerson( ch ) )
   {
      ch->set_bpart( PART_HEAD );
      ch->set_bpart( PART_ARMS );
      ch->set_bpart( PART_LEGS );
      ch->set_bpart( PART_FEET );
      ch->set_bpart( PART_FINGERS );
      ch->set_bpart( PART_EAR );
      ch->set_bpart( PART_EYE );
      ch->set_bpart( PART_BRAINS );
      ch->set_bpart( PART_GUTS );
      ch->set_bpart( PART_HEART );
   }

   if( IsUndead( ch ) )
   {
      ch->unset_bpart( PART_BRAINS );
      ch->unset_bpart( PART_HEART );
   }

   if( IsDragon( ch ) )
   {
      ch->set_bpart( PART_WINGS );
      ch->set_bpart( PART_TAIL );
      ch->set_bpart( PART_SCALES );
      ch->set_bpart( PART_CLAWS );
      ch->set_bpart( PART_TAILATTACK );
   }

   if( IsReptile( ch ) )
   {
      ch->set_bpart( PART_TAIL );
      ch->set_bpart( PART_SCALES );
      ch->set_bpart( PART_CLAWS );
      ch->set_bpart( PART_FORELEGS );
   }

   if( HasHands( ch ) )
      ch->set_bpart( PART_HANDS );

   return;
}

/* Brought over from DOTD code, caclulates such things as the number of
   attacks a PC gets, as well as monk barehand damage and some other
   RIS flags - Samson 4-6-99 
*/
void char_data::ClassSpecificStuff(  )
{
   if( isnpc(  ) )
      return;

   if( IsDragon( this ) )
      armor = UMIN( -150, armor );

   set_numattacks(  );

   if( Class == CLASS_MONK )
   {
      switch ( level )
      {
         case 1:
         case 2:
         case 3:
         case 4:
         case 5:
            barenumdie = 1;
            baresizedie = 3;
            break;
         case 6:
         case 7:
         case 8:
         case 9:
         case 10:
            barenumdie = 1;
            baresizedie = 4;
            break;
         case 11:
         case 12:
         case 13:
         case 14:
         case 15:
            barenumdie = 1;
            baresizedie = 6;
            break;
         case 16:
         case 17:
         case 18:
         case 19:
         case 20:
            barenumdie = 1;
            baresizedie = 8;
            break;
         case 21:
         case 22:
         case 23:
         case 24:
         case 25:
            barenumdie = 2;
            baresizedie = 3;
            break;
         case 26:
         case 27:
         case 28:
         case 29:
         case 30:
            barenumdie = 2;
            baresizedie = 4;
            break;
         case 31:
         case 32:
         case 33:
         case 34:
         case 35:
            barenumdie = 1;
            baresizedie = 10;
            break;
         case 36:
         case 37:
         case 38:
         case 39:
         case 40:
            barenumdie = 1;
            baresizedie = 12;
            break;
         case 41:
         case 42:
         case 43:
         case 44:
         case 45:
            barenumdie = 1;
            baresizedie = 15;
            break;
         case 46:
         case 47:
         case 48:
         case 49:
         case 50:
            barenumdie = 2;
            baresizedie = 5;
            break;
         case 51:
         case 52:
         case 53:
         case 54:
         case 55:
            barenumdie = 2;
            baresizedie = 6;
            break;
         case 56:
         case 57:
         case 58:
         case 59:
         case 60:
            barenumdie = 3;
            baresizedie = 5;
            break;
         case 61:
         case 62:
         case 63:
         case 64:
         case 65:
            barenumdie = 3;
            baresizedie = 6;
            break;
         case 66:
         case 67:
         case 68:
         case 69:
         case 70:
            barenumdie = 1;
            baresizedie = 20;
            break;
         case 71:
         case 72:
         case 73:
         case 74:
         case 75:
            barenumdie = 4;
            baresizedie = 5;
            break;
         case 76:
         case 77:
         case 78:
         case 79:
         case 80:
            barenumdie = 5;
            baresizedie = 4;
            break;
         case 81:
         case 82:
         case 83:
         case 84:
         case 85:
            barenumdie = 5;
            baresizedie = 5;
            break;
         case 86:
         case 87:
         case 88:
         case 89:
         case 90:
            barenumdie = 5;
            baresizedie = 6;
            break;
         case 91:
         case 92:
         case 93:
         case 94:
         case 95:
            barenumdie = 6;
            baresizedie = 6;
            break;
         default:
            barenumdie = 7;
            baresizedie = 6;
            break;
      }

      if( level >= 20 )
         set_resist( RIS_HOLD );
      if( level >= 36 )
         set_resist( RIS_CHARM );
      if( level >= 44 )
         set_resist( RIS_POISON );
      if( level >= 62 )
      {
         set_immune( RIS_CHARM );
         unset_resist( RIS_CHARM );
      }
      if( level >= 70 )
      {
         set_immune( RIS_POISON );
         unset_resist( RIS_POISON );
      }
      armor = 100 - UMIN( 150, level * 5 );
      max_move = UMAX( max_move, ( 150 + ( level * 5 ) ) );
   }

   if( Class == CLASS_DRUID )
   {
      if( level >= 28 )
         set_immune( RIS_CHARM );
      if( level >= 64 )
         set_resist( RIS_POISON );
   }

   if( Class == CLASS_NECROMANCER )
   {
      if( level >= 20 )
         set_resist( RIS_COLD );
      if( level >= 40 )
         set_resist( RIS_FIRE );
      if( level >= 45 )
         set_resist( RIS_ENERGY );
      if( level >= 85 )
      {
         set_immune( RIS_COLD );
         unset_resist( RIS_COLD );
      }
      if( level >= 90 )
      {
         set_immune( RIS_FIRE );
         unset_resist( RIS_FIRE );
      }
   }
}

void char_data::set_specfun( void )
{
   spec_funname.clear();

   switch( Class )
   {
      case CLASS_MAGE:
         spec_fun = m_spec_lookup( "spec_cast_mage" );
         spec_funname = "spec_cast_mage";
         break;

      case CLASS_CLERIC:
         spec_fun = m_spec_lookup( "spec_cast_cleric" );
         spec_funname = "spec_cast_cleric";
         break;

      case CLASS_WARRIOR:
         spec_fun = m_spec_lookup( "spec_warrior" );
         spec_funname = "spec_warrior";
         break;

      case CLASS_ROGUE:
         spec_fun = m_spec_lookup( "spec_thief" );
         spec_funname = "spec_thief";
         break;

      case CLASS_RANGER:
         spec_fun = m_spec_lookup( "spec_ranger" );
         spec_funname = "spec_ranger";
         break;

      case CLASS_PALADIN:
         spec_fun = m_spec_lookup( "spec_paladin" );
         spec_funname = STRALLOC( "spec_paladin" );
         break;

      case CLASS_DRUID:
         spec_fun = m_spec_lookup( "spec_druid" );
         spec_funname = STRALLOC( "spec_druid" );
         break;

      case CLASS_ANTIPALADIN:
         spec_fun = m_spec_lookup( "spec_antipaladin" );
         spec_funname = STRALLOC( "spec_antipaladin" );
         break;

      case CLASS_BARD:
         spec_fun = m_spec_lookup( "spec_bard" );
         spec_funname = STRALLOC( "spec_bard" );
         break;

      default:
         spec_fun = NULL;
         break;
   }
}

void stop_follower( char_data * ch )
{
   if( !ch->master )
   {
      bug( "%s: %s has null master.", __FUNCTION__, ch->name );
      return;
   }

   if( ch->has_aflag( AFF_CHARM ) )
   {
      ch->unset_aflag( AFF_CHARM );
      ch->affect_strip( gsn_charm_person );
   }

   if( ch->master->can_see( ch, false ) )
      if( !( !ch->master->isnpc(  ) && ch->is_immortal(  ) && !ch->master->is_immortal(  ) ) )
         act( AT_ACTION, "$n stops following you.", ch, NULL, ch->master, TO_VICT );
   act( AT_ACTION, "You stop following $N.", ch, NULL, ch->master, TO_CHAR );

   ch->master = NULL;
   ch->leader = NULL;
   return;
}

/* Small utility functions to bind & unbind charmies to their owners etc. - Samson 4-19-00 */
void unbind_follower( char_data * mob, char_data * ch )
{
   stop_follower( mob );

   if( !ch->isnpc(  ) && mob->isnpc(  ) )
   {
      mob->unset_actflag( ACT_PET );
      mob->spec_fun = mob->pIndexData->spec_fun;
      --ch->pcdata->charmies;
      ch->pets.remove( mob );
   }
}

void add_follower( char_data * ch, char_data * master )
{
   if( ch->master )
   {
      bug( "%s: non-null master.", __FUNCTION__ );
      return;
   }

   ch->master = master;
   ch->leader = NULL;

   if( master->can_see( ch, false ) )
      act( AT_ACTION, "$n now follows you.", ch, NULL, master, TO_VICT );
   act( AT_ACTION, "You now follow $N.", ch, NULL, master, TO_CHAR );

   return;
}

void bind_follower( char_data * mob, char_data * ch, int sn, int duration )
{
   affect_data af;

   /*
    * -2 duration means the mob just got loaded from a saved PC and its duration is
    * already set, and will run out when its done 
    */
   if( duration != -2 )
   {
      af.type = sn;
      af.duration = duration;
      af.location = 0;
      af.modifier = 0;
      af.bit = AFF_CHARM;
      mob->affect_to_char( &af );
   }

   add_follower( mob, ch );
   fix_maps( ch, mob );

   if( !ch->isnpc(  ) && mob->isnpc(  ) )
   {
      mob->set_actflag( ACT_PET );
      mob->set_specfun();

      ++ch->pcdata->charmies;
      ch->pets.push_back( mob );
   }
   return;
}

void die_follower( char_data * ch )
{
   if( ch->master )
      unbind_follower( ch, ch->master );

   ch->leader = NULL;

   list<char_data*>::iterator ich;
   for( ich = charlist.begin(); ich != charlist.end(); ++ich )
   {
      char_data *fch = (*ich);

      if( fch->master == ch )
         unbind_follower( fch, ch );

      if( fch->leader == ch )
         fch->leader = fch;
   }
   return;
}

/*
 * Something from original DikuMUD that Merc yanked out.
 * Used to prevent following loops, which can cause problems if people
 * follow in a loop through an exit leading back into the same room
 * (Which exists in many maze areas) - Thoric
 */
bool circle_follow( char_data * ch, char_data * victim )
{
   char_data *tmp;

   for( tmp = victim; tmp; tmp = tmp->master )
      if( tmp == ch )
         return true;
   return false;
}

/* Check a char for ITEM_MUSTMOUNT eq and remove it - Samson 3-18-01 */
void check_mount_objs( char_data * ch, bool fell )
{
   list<obj_data*>::iterator iobj;

   for( iobj = ch->carrying.begin(); iobj != ch->carrying.end(); )
   {
      obj_data *obj = (*iobj);
      ++iobj;

      if( obj->wear_loc == WEAR_NONE )
         continue;

      if( !obj->extra_flags.test( ITEM_MUSTMOUNT ) )
         continue;

      if( fell )
      {
         act( AT_ACTION, "As you fall, $p drops to the ground.", ch, obj, NULL, TO_CHAR );
         act( AT_ACTION, "$n drops $p as $e falls to the ground.", ch, obj, NULL, TO_ROOM );

         obj->from_char(  );
         obj = obj->to_room( ch->in_room, ch );
         oprog_drop_trigger( ch, obj );   /* mudprogs */
         if( ch->char_died(  ) )
            return;
      }
      else
      {
         act( AT_ACTION, "As you dismount, you remove $p.", ch, obj, NULL, TO_CHAR );
         act( AT_ACTION, "$n removes $p as $e dismounts.", ch, obj, NULL, TO_ROOM );
         ch->unequip( obj );
      }
   }
   return;
}

/* Automatic corpse retrieval for < 10 characters.
 * Adapted from the Undertaker snippet by Cyrus and Robcon (Rage of Carnage 2).
 */
void retrieve_corpse( char_data * ch, char_data * healer )
{
   char buf[MSL];
   list<obj_data*>::iterator iobj;
   bool found = false;

   /*
    * Avoids the potential for filling the room with hundreds of mob corpses 
    */
   if( ch->isnpc(  ) )
      return;

   mudstrlcpy( buf, "the corpse of ", MSL );
   mudstrlcat( buf, ch->name, MSL );
   for( iobj = objlist.begin(); iobj != objlist.end(); ++iobj )
   {
      obj_data *obj = (*iobj);
      obj_data *outer_obj;

      if( !nifty_is_name( buf, obj->short_descr ) )
         continue;

      /*
       * This will prevent NPC corpses from being retreived if the person has a mob's name 
       */
      if( obj->item_type == ITEM_CORPSE_NPC )
         continue;

      found = true;

      /*
       * Could be carried by act_scavengers, or other idiots so ... 
       */
      outer_obj = obj;
      while( outer_obj->in_obj )
         outer_obj = outer_obj->in_obj;

      outer_obj->separate(  );
      outer_obj->from_room(  );
      outer_obj->to_room( ch->in_room, ch );

      if( healer )
      {
         act( AT_MAGIC, "$n closes $s eyes in deep prayer....", healer, NULL, NULL, TO_ROOM );
         act( AT_MAGIC, "A moment later $T appears in the room!", healer, NULL, buf, TO_ROOM );
      }
      else
         act( AT_MAGIC, "From out of nowhere, $T appears in a bright flash!", ch, NULL, buf, TO_ROOM );

      if( ch->level > 7 )
      {
         ch->print( "&RReminder: Automatic corpse retrieval ceases once you've reached level 10.\r\n" );
         ch->print( "Upon reaching level 10 it is strongly advised that you choose a deity to devote to!\r\n" );
      }
   }

   if( !found )
   {
      ch->print( "&RThere is no corpse to retrieve. Perhaps you've fallen victim to a Death Trap?\r\n" );
      if( ch->level > 7 )
      {
         ch->print( "&RReminder: Automatic corpse retrieval ceases once you've reached level 10.\r\n" );
         ch->print( "Upon reaching level 10 it is strongly advised that you choose a deity to devote to!\r\n" );
      }
   }
   return;
}

/*
 * Extract a char from the world.
 */
/* If something has gone awry with your *ch pointers, there's a fairly good chance this thing will trip over it and
 * bring things crashing down around you. Which is why there are so many bug log points. [Samson]
 */
void char_data::extract( bool fPull )
{
   if( !in_room )
   {
      bug( "%s: %s in NULL room. Transferring to Limbo.", __FUNCTION__, name ? name : "???" );
      if( !to_room( get_room_index( ROOM_VNUM_LIMBO ) ) )
         log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
   }

   if( this == supermob && !mud_down )
   {
      bug( "%s: ch == supermob!", __FUNCTION__ );
      return;
   }

   if( char_died(  ) )
   {
      bug( "%s: %s already died!", __FUNCTION__, name );
      /*
       * return; This return is commented out in the hops of allowing the dead mob to be extracted anyway 
       */
   }

   /*
    * shove onto extraction queue 
    */
   queue_extracted_char( this, fPull );

   list<rel_data*>::iterator RQ;
   for( RQ = relationlist.begin(); RQ != relationlist.end(); )
   {
      rel_data *RQueue = (*RQ);
      ++RQ;

      if( fPull && RQueue->Type == relMSET_ON )
      {
         if( this == RQueue->Subject )
            ( ( char_data * ) RQueue->Actor )->pcdata->dest_buf = NULL;
         else if( this != RQueue->Actor )
            continue;
         relationlist.remove( RQueue );
         deleteptr( RQueue );
      }
   }

   if( fPull && !mud_down )
      die_follower( this );

   if( !mud_down )
      stop_fighting( true );

   if( mount )
   {
      mount->unset_actflag( ACT_MOUNTED );
      mount = NULL;
      position = POS_STANDING;
   }

   /*
    * check if this NPC was a mount or a pet
    */
   if( isnpc(  ) && !mud_down )
   {
      unset_actflag( ACT_MOUNTED );

      list<char_data*>::iterator ich;
      for( ich = charlist.begin(); ich != charlist.end(); ++ich )
      {
         char_data *wch = (*ich);

         if( wch->mount == this )
         {
            wch->mount = NULL;
            wch->position = POS_SITTING;
            if( wch->in_room == in_room )
            {
               act( AT_SOCIAL, "Your faithful mount, $N collapses beneath you...", wch, NULL, this, TO_CHAR );
               act( AT_SOCIAL, "You hit the ground with a thud.", wch, NULL, NULL, TO_CHAR );
               act( AT_PLAIN, "$n falls from $N as $N is slain.", wch, NULL, this, TO_ROOM );
               check_mount_objs( this, true );  /* Check to see if they have ITEM_MUSTMOUNT stuff */
            }
         }
         if( !wch->isnpc(  ) && !wch->pets.empty() )
         {
            if( master == wch )
            {
               unbind_follower( wch, this );
               if( wch->in_room == in_room )
                  act( AT_SOCIAL, "You mourn for the loss of $N.", wch, NULL, this, TO_CHAR );
            }
         }
      }
   }

   /*
    * Bug fix loop to stop PCs from being hunted after death - Samson 8-22-99 
    */
   if( !mud_down )
   {
      list<char_data*>::iterator ich;
      for( ich = charlist.begin(); ich != charlist.end(); ++ich )
      {
         char_data *wch = (*ich);

         if( !wch->isnpc(  ) )
            continue;

         if( wch->hating && wch->hating->who == this )
            stop_hating( wch );

         if( wch->fearing && wch->fearing->who == this )
            stop_fearing( wch );

         if( wch->hunting && wch->hunting->who == this )
            stop_hunting( wch );
      }
   }

   list<obj_data*>::iterator iobj;
   for( iobj = carrying.begin(); iobj != carrying.end(); )
   {
      obj_data *obj = (*iobj);
      ++iobj;

      if( obj->ego >= sysdata->minego && in_room->flags.test( ROOM_DEATH ) )
         obj->pIndexData->count += obj->count;
      obj->extract(  );
   }

   room_index *dieroom = in_room; /* Added for checking where to send you at death - Samson */
   from_room(  );

   if( !fPull )
   {
      room_index *location = check_room( this, dieroom ); /* added check to see what continent PC is on - Samson 3-29-98 */

      if( !location )
         location = get_room_index( ROOM_VNUM_TEMPLE );

      if( !location )
         location = get_room_index( ROOM_VNUM_LIMBO );

      if( !to_room( location ) )
         log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );

      if( has_pcflag( PCFLAG_ONMAP ) )
      {
         unset_pcflag( PCFLAG_ONMAP );
         unset_pcflag( PCFLAG_MAPEDIT ); /* Just in case they were editing */

         mx = -1;
         my = -1;
         map = -1;
      }

      /*
       * Make things a little fancier           -Thoric
       */
      char_data *wch;
      if( ( wch = get_char_room( "healer" ) ) != NULL )
      {
         act( AT_MAGIC, "$n mutters a few incantations, waves $s hands and points $s finger.", wch, NULL, NULL, TO_ROOM );
         act( AT_MAGIC, "$n appears from some strange swirling mists!", this, NULL, NULL, TO_ROOM );
         act_printf( AT_MAGIC, wch, NULL, NULL, TO_ROOM,
                     "$n says 'Welcome back to the land of the living, %s.'", capitalize( name ) );
         if( level < 10 )
         {
            retrieve_corpse( this, wch );
            collect_followers( this, dieroom, location );
         }
      }
      else
      {
         act( AT_MAGIC, "$n appears from some strange swirling mists!", this, NULL, NULL, TO_ROOM );
         if( level < 10 )
         {
            retrieve_corpse( this, NULL );
            collect_followers( this, dieroom, location );
         }
      }
      position = POS_RESTING;
      return;
   }

   if( isnpc(  ) )
   {
      --pIndexData->count;
      --nummobsloaded;
   }

   if( desc && desc->original )
      interpret( this, "return" );

   if( switched && switched->desc )
      interpret( switched, "return" );

   if( !mud_down )
   {
      list<char_data*>::iterator ich;
      for( ich = charlist.begin(); ich != charlist.end(); ++ich )
      {
         char_data *wch = (*ich);

         if( wch->reply == this )
            wch->reply = NULL;
      }
   }

   if( desc )
   {
      if( desc->character != this )
         bug( "%s: %s's descriptor points to another char", __FUNCTION__, name );
      else
         close_socket( desc, false );
   }

   if( !isnpc() && level < LEVEL_IMMORTAL )
   {
      --sysdata->playersonline;
      if( sysdata->playersonline < 0 )
      {
         bug( "%s: Player count went negative!", __FUNCTION__ );
         sysdata->playersonline = 0;
      }
   }
   return;
}

/*
 * hunting, hating and fearing code - Thoric
 */
bool is_hunting( char_data * ch, char_data * victim )
{
   if( !ch->hunting || ch->hunting->who != victim )
      return false;

   return true;
}

bool is_hating( char_data * ch, char_data * victim )
{
   if( !ch->hating || ch->hating->who != victim )
      return false;

   return true;
}

bool is_fearing( char_data * ch, char_data * victim )
{
   if( !ch->fearing || ch->fearing->who != victim )
      return false;

   return true;
}

void start_hunting( char_data * ch, char_data * victim )
{
   if( ch->hunting )
      stop_hunting( ch );

   ch->hunting = new hunt_hate_fear;
   ch->hunting->name = QUICKLINK( victim->name );
   ch->hunting->who = victim;
   return;
}

void start_hating( char_data * ch, char_data * victim )
{
   if( ch->hating )
      stop_hating( ch );

   ch->hating = new hunt_hate_fear;
   ch->hating->name = QUICKLINK( victim->name );
   ch->hating->who = victim;
   return;
}

void start_fearing( char_data * ch, char_data * victim )
{
   if( ch->fearing )
      stop_fearing( ch );

   ch->fearing = new hunt_hate_fear;
   ch->fearing->name = QUICKLINK( victim->name );
   ch->fearing->who = victim;
   return;
}

void stop_hunting( char_data * ch )
{
   if( ch->hunting )
   {
      STRFREE( ch->hunting->name );
      deleteptr( ch->hunting );
   }
   return;
}

void stop_hating( char_data * ch )
{
   if( ch->hating )
   {
      STRFREE( ch->hating->name );
      deleteptr( ch->hating );
   }
   return;
}

void stop_fearing( char_data * ch )
{
   if( ch->fearing )
   {
      STRFREE( ch->fearing->name );
      deleteptr( ch->fearing );
   }
   return;
}

/*
 * Advancement stuff.
 */
void advance_level( char_data * ch )
{
   char buf[MSL];
   int add_hp, add_mana, add_prac, manamod = 0, manahighdie, manaroll;

   snprintf( buf, MSL, "the %s", title_table[ch->Class][ch->level][ch->sex == SEX_FEMALE ? 1 : 0] );
   set_title( ch, buf );

   /*
    * Updated mana gaining to give pure mage and pure cleric more per level 
    * Any changes here should be reflected in save.c where hp/mana/movement is loaded 
    */
   if( ch->CAN_CAST(  ) )
   {
      switch ( ch->Class )
      {
         case CLASS_MAGE:
         case CLASS_CLERIC:
            manamod = 20;
            break;
         case CLASS_DRUID:
         case CLASS_NECROMANCER:
            manamod = 17;
            break;
         default:
            manamod = 13;
            break;
      }
   }
   else  /* For non-casting classes, cause alot of racials still require mana */
      manamod = 9;

   /*
    * Samson 10-10-98 
    */
   manahighdie = ( ch->get_curr_int(  ) + ch->get_curr_wis(  ) ) / 6;
   if( manahighdie < 2 )
      manahighdie = 2;

   add_hp =
      con_app[ch->get_curr_con(  )].hitp + number_range( class_table[ch->Class]->hp_min, class_table[ch->Class]->hp_max );

   manaroll = dice( 1, manahighdie ) + 1; /* Samson 10-10-98 */

   add_mana = ( manaroll * manamod ) / 10;

   add_prac = 4 + wis_app[ch->get_curr_wis(  )].practice;

   add_hp = UMAX( 1, add_hp );

   add_mana = UMAX( 1, add_mana );

   ch->max_hit += add_hp;
   ch->max_mana += add_mana;
   ch->pcdata->practice += add_prac;

   ch->unset_pcflag( PCFLAG_BOUGHT_PET );

   STRFREE( ch->pcdata->rank );
   ch->pcdata->rank = STRALLOC( class_table[ch->Class]->who_name );

   if( ch->level == LEVEL_AVATAR )
   {
      echo_all_printf( ECHOTAR_ALL, "&[immortal]%s has just achieved Avatarhood!", ch->name );
      STRFREE( ch->pcdata->rank );
      ch->pcdata->rank = STRALLOC( "Avatar" );
      interpret( ch, "help M_ADVHERO_" );
   }
   if( ch->level < LEVEL_IMMORTAL )
      ch->printf( "&WYour gain is: %d hp, %d mana, %d prac.\r\n", add_hp, add_mana, add_prac );

   ch->ClassSpecificStuff(  );   /* Brought over from DOTD code - Samson 4-6-99 */
   ch->save(  );
   return;
}

void char_data::gain_exp( double gain )
{
   double modgain;

   if( isnpc(  ) || level >= LEVEL_AVATAR )
      return;

   modgain = gain;

   /*
    * per-race experience multipliers 
    */
   modgain *= ( race_table[race]->exp_multiplier / 100.0 );

   /*
    * xp cap to prevent any one event from giving enuf xp to 
    */
   /*
    * gain more than one level - FB 
    */
   modgain = UMIN( modgain, exp_level( level + 2 ) - exp_level( level + 1 ) );

   exp = ( int )( UMAX( 0, exp + modgain ) );

   if( NOT_AUTHED( this ) && exp >= exp_level( 10 ) ) /* new auth */
   {
      print( "You can not ascend to a higher level until your name is authorized.\r\n" );
      exp = ( exp_level( 10 ) - 1 );
      return;
   }

   if( level < LEVEL_AVATAR && exp >= exp_level( level + 1 ) )
   {
      print( "&RYou have acheived enough experience to level! Use the levelup command to do so.&w\r\n" );
      if( exp >= exp_level( level + 2 ) )
      {
         exp = exp_level( level + 2 ) - 1;
         print( "&RYou cannot gain any further experience until you level up.&w\r\n" );
      }
   }
   return;
}

char *char_data::get_class(  )
{
   if( isnpc(  ) && Class < MAX_NPC_CLASS && Class >= 0 )
      return ( npc_class[Class] );
   else if( !isnpc(  ) && Class < MAX_PC_CLASS && Class >= 0 )
      return ( class_table[Class]->who_name );
   return ( "Unknown" );
}

char *char_data::get_race(  )
{
   if( race < MAX_PC_RACE && race >= 0 )
      return ( race_table[race]->race_name );
   if( race < MAX_NPC_RACE && race >= 0 )
      return ( npc_race[race] );
   return ( "Unknown" );
}

/* Disabled Limbo transfer - Samson 5-8-99 */
void char_data::stop_idling()
{
   if( !desc || desc->connected != CON_PLAYING || !has_pcflag( PCFLAG_IDLING ) )
      return;

   timer = 0;

   unset_pcflag( PCFLAG_IDLING );
   act( AT_ACTION, "$n returns to normal.", this, NULL, NULL, TO_ROOM );
   return;
}

void char_data::set_actflag( int value )
{
   try
   {
      actflags.set( value );
   }
   catch( exception & e )
   {
      bug( "Flag exception caught: %s", e.what() );
   }
}

void char_data::unset_actflag( int value )
{
   try
   {
      actflags.reset( value );
   }
   catch( exception & e )
   {
      bug( "Flag exception caught: %s", e.what() );
   }
}

bool char_data::has_actflag( int value )
{
   return( actflags.test( value ) );
}

void char_data::toggle_actflag( int value )
{
   try
   {
      actflags.flip( value );
   }
   catch( exception & e )
   {
      bug( "Flag exception caught: %s", e.what() );
   }
}

bool char_data::has_actflags()
{
   if( actflags.any() )
      return true;
   return false;
}

bitset<MAX_ACT_FLAG> char_data::get_actflags()
{
   return actflags;
}

void char_data::set_actflags( bitset<MAX_ACT_FLAG> bits )
{
   try
   {
      actflags = bits;
   }
   catch( exception & e )
   {
      bug( "Flag exception caught: %s", e.what() );
   }
}

void char_data::set_file_actflags( FILE *fp )
{
   try
   {
      flag_set( fp, actflags, act_flags );
   }
   catch( exception & e )
   {
      bug( "Flag exception caught: %s", e.what() );
   }
}

bool char_data::has_immune( int ris )
{
   return( immune.test( ris ) );
}

void char_data::set_immune( int ris )
{
   try
   {
      immune.set( ris );
   }
   catch( exception & e )
   {
      bug( "Flag exception caught: %s", e.what() );
   }
}

void char_data::unset_immune( int ris )
{
   try
   {
      immune.reset( ris ); 
   }
   catch( exception & e )
   {
      bug( "Flag exception caught: %s", e.what() );
   }
}

void char_data::toggle_immune( int ris )
{
   try
   {
      immune.flip( ris );
   }
   catch( exception & e )
   {
      bug( "Flag exception caught: %s", e.what() );
   }
}

bool char_data::has_immunes()
{
   if( immune.any() )
      return true;
   return false;
}

bitset<MAX_RIS_FLAG> char_data::get_immunes()
{
   return immune;
}

void char_data::set_immunes( bitset<MAX_RIS_FLAG> bits )
{
   try
   {
      immune = bits;
   }
   catch( exception & e )
   {
      bug( "Flag exception caught: %s", e.what() );
   }
}

void char_data::set_file_immunes( FILE *fp )
{
   try
   {
      flag_set( fp, immune, ris_flags );
   }
   catch( exception & e )
   {
      bug( "Flag exception caught: %s", e.what() );
   }
}

bool char_data::has_noimmune( int ris )
{
   return( no_immune.test( ris ) );
}

void char_data::set_noimmune( int ris )
{
   try
   {
      no_immune.set( ris );
   }
   catch( exception & e )
   {
      bug( "Flag exception caught: %s", e.what() );
   }
}

bool char_data::has_noimmunes()
{
   if( no_immune.any() )
      return true;
   return false;
}

bitset<MAX_RIS_FLAG> char_data::get_noimmunes()
{
   return no_immune;
}

void char_data::set_noimmunes( bitset<MAX_RIS_FLAG> bits )
{
   try
   {
      no_immune = bits;
   }
   catch( exception & e )
   {
      bug( "Flag exception caught: %s", e.what() );
   }
}

void char_data::set_file_noimmunes( FILE *fp )
{
   try
   {
      flag_set( fp, no_immune, ris_flags );
   }
   catch( exception & e )
   {
      bug( "Flag exception caught: %s", e.what() );
   }
}

bool char_data::has_resist( int ris )
{
   return( resistant.test( ris ) );
}

void char_data::set_resist( int ris )
{
   try
   {
      resistant.set( ris );
   }
   catch( exception & e )
   {
      bug( "Flag exception caught: %s", e.what() );
   }
}

void char_data::unset_resist( int ris )
{
   try
   {
      resistant.reset( ris );
   }
   catch( exception & e )
   {
      bug( "Flag exception caught: %s", e.what() );
   }
}

void char_data::toggle_resist( int ris )
{
   try
   {
      resistant.flip( ris );
   }
   catch( exception & e )
   {
      bug( "Flag exception caught: %s", e.what() );
   }
}

bool char_data::has_resists()
{
   if( resistant.any() )
      return true;
   return false;
}

bitset<MAX_RIS_FLAG> char_data::get_resists()
{
   return resistant;
}

void char_data::set_resists( bitset<MAX_RIS_FLAG> bits )
{
   try
   {
      resistant = bits;
   }
   catch( exception & e )
   {
      bug( "Flag exception caught: %s", e.what() );
   }
}

void char_data::set_file_resists( FILE *fp )
{
   try
   {
      flag_set( fp, resistant, ris_flags );
   }
   catch( exception & e )
   {
      bug( "Flag exception caught: %s", e.what() );
   }
}

bool char_data::has_noresist( int ris )
{
   return( no_resistant.test( ris ) );
}

void char_data::set_noresist( int ris )
{
   try
   {
      no_resistant.set( ris );
   }
   catch( exception & e )
   {
      bug( "Flag exception caught: %s", e.what() );
   }
}

bool char_data::has_noresists()
{
   if( no_resistant.any() )
      return true;
   return false;
}

bitset<MAX_RIS_FLAG> char_data::get_noresists()
{
   return no_resistant;
}

void char_data::set_noresists( bitset<MAX_RIS_FLAG> bits )
{
   try
   {
      no_resistant = bits;
   }
   catch( exception & e )
   {
      bug( "Flag exception caught: %s", e.what() );
   }
}

void char_data::set_file_noresists( FILE *fp )
{
   try
   {
      flag_set( fp, no_resistant, ris_flags );
   }
   catch( exception & e )
   {
      bug( "Flag exception caught: %s", e.what() );
   }
}

bool char_data::has_suscep( int ris )
{
   return( susceptible.test( ris ) );
}

void char_data::set_suscep( int ris )
{
   try
   {
      susceptible.set( ris );
   }
   catch( exception & e )
   {
      bug( "Flag exception caught: %s", e.what() );
   }
}

void char_data::unset_suscep( int ris )
{
   try
   {
      susceptible.reset( ris );
   }
   catch( exception & e )
   {
      bug( "Flag exception caught: %s", e.what() );
   }
}

void char_data::toggle_suscep( int ris )
{
   try
   {
      susceptible.flip( ris );
   }
   catch( exception & e )
   {
      bug( "Flag exception caught: %s", e.what() );
   }
}

bool char_data::has_susceps()
{
   if( susceptible.any() )
      return true;
   return false;
}

bitset<MAX_RIS_FLAG> char_data::get_susceps()
{
   return susceptible;
}

void char_data::set_susceps( bitset<MAX_RIS_FLAG> bits )
{
   try
   {
      susceptible = bits;
   }
   catch( exception & e )
   {
      bug( "Flag exception caught: %s", e.what() );
   }
}

void char_data::set_file_susceps( FILE *fp )
{
   try
   {
      flag_set( fp, susceptible, ris_flags );
   }
   catch( exception & e )
   {
      bug( "Flag exception caught: %s", e.what() );
   }
}

bool char_data::has_nosuscep( int ris )
{
   return( no_susceptible.test( ris ) );
}

void char_data::set_nosuscep( int ris )
{
   try
   {
      no_susceptible.set( ris );
   }
   catch( exception & e )
   {
      bug( "Flag exception caught: %s", e.what() );
   }
}

bool char_data::has_nosusceps()
{
   if( no_susceptible.any() )
      return true;
   return false;
}

bitset<MAX_RIS_FLAG> char_data::get_nosusceps()
{
   return no_susceptible;
}

void char_data::set_nosusceps( bitset<MAX_RIS_FLAG> bits )
{
   try
   {
      no_susceptible = bits;
   }
   catch( exception & e )
   {
      bug( "Flag exception caught: %s", e.what() );
   }
}

void char_data::set_file_nosusceps( FILE *fp )
{
   try
   {
      flag_set( fp, no_susceptible, ris_flags );
   }
   catch( exception & e )
   {
      bug( "Flag exception caught: %s", e.what() );
   }
}

bool char_data::has_absorb( int ris )
{
   return( absorb.test( ris ) );
}

void char_data::set_absorb( int ris )
{
   try
   {
      absorb.set( ris );
   }
   catch( exception & e )
   {
      bug( "Flag exception caught: %s", e.what() );
   }
}

void char_data::unset_absorb( int ris )
{
   try
   {
      absorb.reset( ris );
   }
   catch( exception & e )
   {
      bug( "Flag exception caught: %s", e.what() );
   }
}

void char_data::toggle_absorb( int ris )
{
   try
   {
      absorb.flip( ris );
   }
   catch( exception & e )
   {
      bug( "Flag exception caught: %s", e.what() );
   }
}

bool char_data::has_absorbs()
{
   if( absorb.any() )
      return true;
   return false;
}

bitset<MAX_RIS_FLAG> char_data::get_absorbs()
{
   return absorb;
}

void char_data::set_absorbs( bitset<MAX_RIS_FLAG> bits )
{
   try
   {
      absorb = bits;
   }
   catch( exception & e )
   {
      bug( "Flag exception caught: %s", e.what() );
   }
}

void char_data::set_file_absorbs( FILE *fp )
{
   try
   {
      flag_set( fp, absorb, ris_flags );
   }
   catch( exception & e )
   {
      bug( "Flag exception caught: %s", e.what() );
   }
}

bool char_data::has_attack( int attack )
{
   return( attacks.test( attack ) );
}

void char_data::set_attack( int attack )
{
   try
   {
      attacks.set( attack );
   }
   catch( exception & e )
   {
      bug( "Flag exception caught: %s", e.what() );
   }
}

void char_data::unset_attack( int attack )
{
   try
   {
      attacks.reset( attack );
   }
   catch( exception & e )
   {
      bug( "Flag exception caught: %s", e.what() );
   }
}

void char_data::toggle_attack( int attack )
{
   try
   {
      attacks.flip( attack );
   }
   catch( exception & e )
   {
      bug( "Flag exception caught: %s", e.what() );
   }
}

bool char_data::has_attacks()
{
   if( attacks.any() )
      return true;
   return false;
}

bitset<MAX_ATTACK_TYPE> char_data::get_attacks()
{
   return attacks;
}

void char_data::set_attacks( bitset<MAX_ATTACK_TYPE> bits )
{
   try
   {
      attacks = bits;
   }
   catch( exception & e )
   {
      bug( "Flag exception caught: %s", e.what() );
   }
}

void char_data::set_file_attacks( FILE *fp )
{
   try
   {
      flag_set( fp, attacks, attack_flags );
   }
   catch( exception & e )
   {
      bug( "Flag exception caught: %s", e.what() );
   }
}

bool char_data::has_defense( int defense )
{
   return( defenses.test( defense ) );
}

void char_data::set_defense( int defense )
{
   try
   {
      defenses.set( defense );
   }
   catch( exception & e )
   {
      bug( "Flag exception caught: %s", e.what() );
   }
}

void char_data::unset_defense( int defense )
{
   try
   {
      defenses.reset( defense );
   }
   catch( exception & e )
   {
      bug( "Flag exception caught: %s", e.what() );
   }
}

void char_data::toggle_defense( int defense )
{
   try
   {
      defenses.flip( defense );
   }
   catch( exception & e )
   {
      bug( "Flag exception caught: %s", e.what() );
   }
}

bool char_data::has_defenses()
{
   if( defenses.any() )
      return true;
   return false;
}

bitset<MAX_DEFENSE_TYPE> char_data::get_defenses()
{
   return defenses;
}

void char_data::set_defenses( bitset<MAX_DEFENSE_TYPE> bits )
{
   try
   {
      defenses = bits;
   }
   catch( exception & e )
   {
      bug( "Flag exception caught: %s", e.what() );
   }
}

void char_data::set_file_defenses( FILE *fp )
{
   try
   {
      flag_set( fp, defenses, defense_flags );
   }
   catch( exception & e )
   {
      bug( "Flag exception caught: %s", e.what() );
   }
}

bool char_data::has_aflag( int sn )
{
   return( affected_by.test( sn ) );
}

void char_data::set_aflag( int sn )
{
   try
   {
      affected_by.set( sn );
   }
   catch( exception & e )
   {
      bug( "Flag exception caught: %s", e.what() );
   }
}

void char_data::unset_aflag( int sn )
{
   try
   {
      affected_by.reset( sn );
   }
   catch( exception & e )
   {
      bug( "Flag exception caught: %s", e.what() );
   }
}

void char_data::toggle_aflag( int sn )
{
   try
   {
      affected_by.flip( sn );
   }
   catch( exception & e )
   {
      bug( "Flag exception caught: %s", e.what() );
   }
}

bool char_data::has_aflags()
{
   if( affected_by.any() )
      return true;
   return false;
}

bitset<MAX_AFFECTED_BY> char_data::get_aflags()
{
   return affected_by;
}

void char_data::set_aflags( bitset<MAX_AFFECTED_BY> bits )
{
   try
   {
      affected_by = bits;
   }
   catch( exception & e )
   {
      bug( "Flag exception caught: %s", e.what() );
   }
}

void char_data::set_file_aflags( FILE *fp )
{
   try
   {
      flag_set( fp, affected_by, aff_flags );
   }
   catch( exception & e )
   {
      bug( "Flag exception caught: %s", e.what() );
   }
}

bool char_data::has_noaflag( int sn )
{
   return( no_affected_by.test( sn ) );
}

void char_data::set_noaflag( int sn )
{
   try
   {
      no_affected_by.set( sn );
   }
   catch( exception & e )
   {
      bug( "Flag exception caught: %s", e.what() );
   }
}

void char_data::unset_noaflag( int sn )
{
   try
   {
      no_affected_by.reset( sn );
   }
   catch( exception & e )
   {
      bug( "Flag exception caught: %s", e.what() );
   }
}

void char_data::toggle_noaflag( int sn )
{
   try
   {
      no_affected_by.flip( sn );
   }
   catch( exception & e )
   {
      bug( "Flag exception caught: %s", e.what() );
   }
}

bool char_data::has_noaflags()
{
   if( no_affected_by.any() )
      return true;
   return false;
}

bitset<MAX_AFFECTED_BY> char_data::get_noaflags()
{
   return no_affected_by;
}

void char_data::set_noaflags( bitset<MAX_AFFECTED_BY> bits )
{
   try
   {
      no_affected_by = bits;
   }
   catch( exception & e )
   {
      bug( "Flag exception caught: %s", e.what() );
   }
}

void char_data::set_file_noaflags( FILE *fp )
{
   try
   {
      flag_set( fp, no_affected_by, aff_flags );
   }
   catch( exception & e )
   {
      bug( "Flag exception caught: %s", e.what() );
   }
}

bool char_data::has_bpart( int part )
{
   return( body_parts.test( part ) );
}

void char_data::set_bpart( int part )
{
   try
   {
      body_parts.set( part );
   }
   catch( exception & e )
   {
      bug( "Flag exception caught: %s", e.what() );
   }
}

void char_data::unset_bpart( int part )
{
   try
   {
      body_parts.reset( part );
   }
   catch( exception & e )
   {
      bug( "Flag exception caught: %s", e.what() );
   }
}

void char_data::toggle_bpart( int part )
{
   try
   {
      body_parts.flip( part );
   }
   catch( exception & e )
   {
      bug( "Flag exception caught: %s", e.what() );
   }
}

bool char_data::has_bparts()
{
   if( body_parts.any() )
      return true;
   return false;
}

bitset<MAX_BPART> char_data::get_bparts()
{
   return body_parts;
}

void char_data::set_bparts( bitset<MAX_BPART> bits )
{
   try
   {
      body_parts = bits;
   }
   catch( exception & e )
   {
      bug( "Flag exception caught: %s", e.what() );
   }
}

void char_data::set_file_bparts( FILE *fp )
{
   try
   {
      flag_set( fp, body_parts, part_flags );
   }
   catch( exception & e )
   {
      bug( "Flag exception caught: %s", e.what() );
   }
}

bool char_data::has_pcflag( int bit )
{
   return( !isnpc(  ) && pcdata->flags.test( bit ) );
}

void char_data::set_pcflag( int bit )
{
   if( isnpc() )
      bug( "%s: Setting PC flag on NPC!", __FUNCTION__ );
   else
   {
      try
      {
         pcdata->flags.set( bit );
      }
      catch( exception & e )
      {
         bug( "Flag exception caught: %s", e.what() );
      }
   }
}

void char_data::unset_pcflag( int bit )
{
   if( isnpc() )
      bug( "%s: Removing PC flag on NPC!", __FUNCTION__ );
   else
   {
      try
      {
         pcdata->flags.reset( bit );
      }
      catch( exception & e )
      {
         bug( "Flag exception caught: %s", e.what() );
      }
   }
}

void char_data::toggle_pcflag( int bit )
{
   if( isnpc() )
      bug( "%s: Toggling PC flag on NPC!", __FUNCTION__ );
   else
   {
      try
      {
         pcdata->flags.flip( bit );
      }
      catch( exception & e )
      {
         bug( "Flag exception caught: %s", e.what() );
      }
   }
}

bool char_data::has_pcflags()
{
   if( isnpc() )
   {
      bug( "%s: Checking PC flags on NPC!", __FUNCTION__ );
      return false;
   }
   else
   {
      if( pcdata->flags.any() )
         return true;
      return false;
   }
}

bitset<MAX_PCFLAG> char_data::get_pcflags()
{
   if( isnpc() )
   {
      bug( "%s: Retreving PC flags on NPC!", __FUNCTION__ );
      return 0;
   }
   else
      return pcdata->flags;
}

void char_data::set_file_pcflags( FILE *fp )
{
   if( isnpc() )
   {
      bug( "%s: Setting PC flags on NPC from FILE!", __FUNCTION__ );
      return;
   }

   try
   {
      flag_set( fp, pcdata->flags, pc_flags );
   }
   catch( exception & e )
   {
      bug( "Flag exception caught: %s", e.what() );
   }
}

bool char_data::has_lang( int lang )
{
   return( speaks.test( lang ) );
}

void char_data::set_lang( int lang )
{
   if( lang == -1 )
      speaks.set();
   else
   {
      try
      {
         speaks.set( lang );
      }
      catch( exception & e )
      {
         bug( "Flag exception caught: %s", e.what() );
      }
   }
}

void char_data::unset_lang( int lang )
{
   try
   {
      speaks.reset( lang );
   }
   catch( exception & e )
   {
      bug( "Flag exception caught: %s", e.what() );
   }
}

void char_data::toggle_lang( int lang )
{
   try
   {
      speaks.flip( lang );
   }
   catch( exception & e )
   {
      bug( "Flag exception caught: %s", e.what() );
   }
}

bool char_data::has_langs()
{
   if( speaks.any() )
      return true;
   return false;
}

bitset<LANG_UNKNOWN> char_data::get_langs()
{
   return speaks;
}

void char_data::set_langs( bitset<LANG_UNKNOWN> bits )
{
   try
   {
      speaks = bits;
   }
   catch( exception & e )
   {
      bug( "Flag exception caught: %s", e.what() );
   }
}

void char_data::set_file_langs( FILE *fp )
{
   try
   {
      flag_set( fp, speaks, lang_names );
   }
   catch( exception & e )
   {
      bug( "Flag exception caught: %s", e.what() );
   }
}

CMDF( do_dismiss )
{
   char_data *victim;

   if( !argument || argument[0] == '\0' )
   {
      ch->print( "Dismiss whom?\r\n" );
      return;
   }

   if( !( victim = ch->get_char_room( argument ) ) )
   {
      ch->print( "They aren't here.\r\n" );
      return;
   }

   if( ( victim->has_aflag( AFF_CHARM ) ) && ( victim->isnpc(  ) ) && ( victim->master == ch ) )
   {
      unbind_follower( victim, ch );
      stop_hating( victim );
      stop_hunting( victim );
      stop_fearing( victim );
      act( AT_ACTION, "$n dismisses $N.", ch, NULL, victim, TO_NOTVICT );
      act( AT_ACTION, "You dismiss $N.", ch, NULL, victim, TO_CHAR );
   }
   else
      ch->print( "You cannot dismiss them.\r\n" );
   return;
}

CMDF( do_follow )
{
   char_data *victim;

   if( !argument || argument[0] == '\0' )
   {
      ch->print( "Follow whom?\r\n" );
      return;
   }

   if( !( victim = ch->get_char_room( argument ) ) )
   {
      ch->print( "They aren't here.\r\n" );
      return;
   }

   if( ch->has_aflag( AFF_CHARM ) && ch->master )
   {
      act( AT_PLAIN, "But you'd rather follow $N!", ch, NULL, ch->master, TO_CHAR );
      return;
   }

   if( victim == ch )
   {
      if( !ch->master )
      {
         ch->print( "You already follow yourself.\r\n" );
         return;
      }
      stop_follower( ch );
      return;
   }

   if( circle_follow( ch, victim ) )
   {
      ch->print( "Following in loops is not allowed... sorry.\r\n" );
      return;
   }

   if( ch->master )
      stop_follower( ch );

   add_follower( ch, victim );
   return;
}

CMDF( do_order )
{
   char arg[MIL], argbuf[MIL];

   mudstrlcpy( argbuf, argument, MIL );
   argument = one_argument( argument, arg );

   if( arg[0] == '\0' || argument[0] == '\0' )
   {
      ch->print( "Order whom to do what?\r\n" );
      return;
   }

   if( ch->has_aflag( AFF_CHARM ) )
   {
      ch->print( "You feel like taking, not giving, orders.\r\n" );
      return;
   }

   bool fAll;
   char_data *victim;
   if( !str_cmp( arg, "all" ) )
   {
      fAll = true;
      victim = NULL;
   }
   else
   {
      fAll = false;
      if( !( victim = ch->get_char_room( arg ) ) )
      {
         ch->print( "They aren't here.\r\n" );
         return;
      }

      if( victim == ch )
      {
         ch->print( "Aye aye, right away!\r\n" );
         return;
      }

      if( !victim->has_aflag( AFF_CHARM ) || victim->master != ch )
      {
         interpret( victim, "say Do it yourself!" );
         return;
      }
   }

   bool found = false;
   list<char_data*>::iterator ich;
   for( ich = ch->in_room->people.begin(); ich != ch->in_room->people.end(); )
   {
      char_data *och = (*ich);
      ++ich;

      if( och->has_aflag( AFF_CHARM ) && och->master == ch && ( fAll || och == victim ) )
      {
         found = true;
         act( AT_ACTION, "$n orders you to '$t'.", ch, argument, och, TO_VICT );
         interpret( och, argument );
      }
   }

   if( found )
   {
      ch->print( "Ok.\r\n" );
      ch->WAIT_STATE( 12 );
   }
   else
      ch->print( "You have no followers here.\r\n" );
   return;
}

CMDF( do_levelup )
{
   if( ch->exp < exp_level( ch->level + 1 ) )
   {
      ch->print( "&RYou don't have enough experience to level yet, go forth and adventure!\r\n" );
      return;
   }

   ++ch->level;
   advance_level( ch );
   log_printf_plus( LOG_INFO, LEVEL_IMMORTAL, "%s has advanced to level %d!", ch->name, ch->level );
   cmdf( ch, "gtell I just reached level %d!", ch->level );

   if( ch->level == 4 )
      ch->
         print
         ( "&YYou can now be affected by hunger and thirst.\r\nIt is adviseable for you to purchase food and water soon.\r\n" );

   if( number_range( 1, 2 ) == 1 )
      ch->sound( "level.mid", 100, false );
   else
      ch->sound( "level2.mid", 100, false );
   if( ch->level % 20 == 0 )
   {
      ch->desc->show_stats( ch );
      ch->tempnum = 1;
      ch->desc->connected = CON_RAISE_STAT;
   }
   return;
}
