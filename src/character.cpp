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
#include "variables.h"

list < char_data * >charlist;
list < char_data * >pclist;

#ifdef IMC
void imc_freechardata( char_data * );
#endif
void clean_char_queue(  );
void stop_hating( char_data * );
void stop_hunting( char_data * );
void stop_fearing( char_data * );
void free_fight( char_data * );
void queue_extracted_char( char_data *, bool );
room_index *check_room( char_data *, room_index * );
void update_room_reset( char_data *, bool );

extern list < rel_data * >relationlist;

pc_data::~pc_data(  )
{
   if( this->pnote )
      deleteptr( this->pnote );

   STRFREE( this->filename );
   STRFREE( this->prompt );
   STRFREE( this->fprompt );
   DISPOSE( this->pwd );   /* no hash */
   DISPOSE( this->bamfin );   /* no hash */
   DISPOSE( this->bamfout );  /* no hash */
   STRFREE( this->rank );
   STRFREE( this->title );
   DISPOSE( this->bio );   /* no hash */
   STRFREE( this->prompt );
   STRFREE( this->fprompt );
   STRFREE( this->helled_by );
   STRFREE( this->subprompt );
   DISPOSE( this->afkbuf );
   DISPOSE( this->motd_buf );

   this->alias_map.clear(  );
   this->free_comments(  );
   this->free_pcboards(  );
   this->zone.clear(  );
   this->ignore.clear(  );
   this->qbits.clear(  );
   this->say_history.clear();
   this->tell_history.clear();
}

/*
 * Clear a new PC.
 */
pc_data::pc_data(  )
{
   init_memory( &this->area, &this->hotboot, sizeof( this->hotboot ) );

   this->flags.reset(  );
   this->alias_map.clear(  );
   this->zone.clear(  );
   this->ignore.clear(  );
   this->say_history.clear();
   this->tell_history.clear();
   for( int sn = 0; sn < MAX_BEACONS; ++sn )
      this->beacon[sn] = 0;
   this->condition[COND_THIRST] = ( int )( sysdata->maxcondval * .75 );
   this->condition[COND_FULL] = ( int )( sysdata->maxcondval * .75 );
   this->pagerlen = 24;
   this->secedit = SECT_OCEAN;   /* Initialize Map OLC sector - Samson 8-1-99 */
   this->one = ROOM_VNUM_TEMPLE;
   this->lasthost = "Unknown-Host";
   this->logon = current_time;
   this->timezone = -1;
   for( int sn = 0; sn < num_skills; ++sn )
      this->learned[sn] = 0;
}

/*
 * Free a character's memory.
 */
char_data::~char_data(  )
{
   if( !this )
   {
      bug( "%s: nullptr ch!", __func__ );
      return;
   }

   // Hackish fix - if we forget, whoever reads this should remind us someday - Samson 3-28-05
   if( this->desc )
   {
      bug( "%s: char still has descriptor.", __func__ );
      log_printf( "Desc# %d, DescHost %s DescClient %s", this->desc->descriptor, this->desc->hostname.c_str(  ), this->desc->client.c_str(  ) );
      deleteptr( this->desc );
   }

   deleteptr( this->morph );

   list < obj_data * >::iterator iobj;
   for( iobj = this->carrying.begin(  ); iobj != this->carrying.end(  ); )
   {
      obj_data *obj = *iobj;
      ++iobj;

      obj->extract(  );
   }

   list < affect_data * >::iterator paf;
   for( paf = affects.begin(  ); paf != affects.end(  ); )
   {
      affect_data *aff = *paf;
      ++paf;

      this->affect_remove( aff );
   }

   list < timer_data * >::iterator chtimer;
   for( chtimer = timers.begin(  ); chtimer != timers.end(  ); )
   {
      timer_data *ctimer = *chtimer;
      ++chtimer;

      this->extract_timer( ctimer );
   }

   STRFREE( this->name );
   STRFREE( this->short_descr );
   STRFREE( this->long_descr );
   STRFREE( this->chardesc );
   DISPOSE( this->alloc_ptr );

   stop_hunting( this );
   stop_hating( this );
   stop_fearing( this );
   free_fight( this );
   this->abits.clear(  );

   if( this->pcdata )
   {
      if( this->pcdata->editor )
         this->stop_editing(  );
#ifdef IMC
      imc_freechardata( this );
#endif
      deleteptr( this->pcdata );
   }

   list < mprog_act_list * >::iterator pd;
   for( pd = this->mpact.begin(  ); pd != this->mpact.end(  ); )
   {
      mprog_act_list *actp = *pd;
      ++pd;

      this->mpact.remove( actp );
      deleteptr( actp );
   }

   list < variable_data * >::iterator ivd;
   for( ivd = this->variables.begin(  ); ivd != this->variables.end(  ); )
   {
      variable_data *vd = *ivd;
      ++ivd;

      this->variables.remove( vd );
      deleteptr( vd );
   }

   list < char_data * >::iterator ich;
   for( ich = this->pets.begin(  ); ich != this->pets.end(  ); )
   {
      char_data *mob = *ich;
      ++ich;

      mob->extract( true );
      deleteptr( mob );
   }
}

/*
 * Clear a new character.
 */
char_data::char_data(  )
{
   init_memory( &this->master, &this->backtracking, sizeof( this->backtracking ) );

   this->armor = 100;
   this->position = POS_STANDING;
   this->hit = 20;
   this->max_hit = 20;
   this->mana = 100;
   this->max_mana = 100;
   this->move = 150;
   this->max_move = 150;
   this->height = 72;
   this->weight = 180;
   this->spellfail = 101;
   this->race = RACE_HUMAN;
   this->Class = CLASS_WARRIOR;
   this->speaking = LANG_COMMON;
   this->speaks.reset(  );
   this->barenumdie = 1;
   this->baresizedie = 4;
   this->perm_str = 13;
   this->perm_dex = 13;
   this->perm_int = 13;
   this->perm_wis = 13;
   this->perm_cha = 13;
   this->perm_con = 13;
   this->perm_lck = 13;
   this->mx = -1; /* Overland Map - Samson 7-31-99 */
   this->my = -1;
   this->wmap = -1;
   this->wait = 0;
   this->variables.clear(  );
}

void extract_all_chars( void )
{
   clean_char_queue(  );

   list < char_data * >::iterator ich;
   for( ich = charlist.begin(  ); ich != charlist.end(  ); )
   {
      char_data *character = *ich;
      ++ich;

      character->extract( true );
   }
   clean_char_queue(  );
}

bool char_data::MSP_ON(  )
{
   return ( this->has_pcflag( PCFLAG_MSP ) && this->desc && this->desc->msp_detected );
}

bool char_data::IS_OUTSIDE(  )
{
   if( !this->in_room->flags.test( ROOM_INDOORS ) && !this->in_room->flags.test( ROOM_TUNNEL )
       && !this->in_room->flags.test( ROOM_CAVE ) && !this->in_room->flags.test( ROOM_CAVERN ) )
      return true;
   else
      return false;
}

int char_data::GET_AC(  )
{
   return ( this->armor + ( this->IS_AWAKE(  )? dex_app[this->get_curr_dex(  )].defensive : 0 ) );
}

int char_data::GET_HITROLL(  )
{
   return ( this->hitroll + str_app[this->get_curr_str(  )].tohit );
}

/* Thanks to Chriss Baeke for noticing damplus was unused */
int char_data::GET_DAMROLL(  )
{
   return ( this->damroll + this->damplus + str_app[this->get_curr_str(  )].todam );
}

int char_data::GET_ADEPT( int sn )
{
   // Bugfix by Sadiq - Imms should have perfect skills.
   if( this->is_immortal() )
      return 100;

   return skill_table[sn]->skill_adept[this->Class];
}

bool char_data::IS_DRUNK( int drunk )
{
   return ( number_percent(  ) < ( this->pcdata->condition[COND_DRUNK] * 2 / drunk ) );
}

void char_data::print( const string & txt )
{
   if( !txt.empty(  ) && this->desc )
      this->desc->send_color( txt );
}

void char_data::print_room( const string & txt )
{
   if( txt.empty(  ) || !this->in_room )
      return;

   list < char_data * >::iterator ich;
   for( ich = this->in_room->people.begin(  ); ich != this->in_room->people.end(  ); ++ich )
   {
      char_data *rch = *ich;

      if( !rch->char_died(  ) && rch->desc )
      {
         if( is_same_char_map( this, rch ) )
            rch->print( txt );
      }
   }
}

void char_data::pager( const string & txt )
{
   char_data *ch;

   if( !txt.empty(  ) && this->desc )
   {
      descriptor_data *d = this->desc;

      ch = d->original ? d->original : d->character;
      if( !ch->has_pcflag( PCFLAG_PAGERON ) )
      {
         if( ch->desc )
            ch->desc->send_color( txt );
      }
      else
      {
         if( ch->desc )
            ch->desc->pager( colorize( txt, ch->desc ) );
      }
   }
}

void char_data::printf( const char *fmt, ... )
{
   char buf[MSL * 2];
   va_list args;

   va_start( args, fmt );
   vsnprintf( buf, MSL * 2, fmt, args );
   va_end( args );

   this->print( buf );
}

void char_data::pagerf( const char *fmt, ... )
{
   char buf[MSL * 2];
   va_list args;

   va_start( args, fmt );
   vsnprintf( buf, MSL * 2, fmt, args );
   va_end( args );

   this->pager( buf );
}

void char_data::set_color( short AType )
{
   if( !this->desc )
      return;

   if( this->isnpc(  ) )
      return;

   this->desc->write_to_buffer( color_str( AType ) );
   if( !this->desc )
   {
      bug( "%s: nullptr descriptor after WTB! CH: %s", __func__, this->name ? this->name : "Unknown?!?" );
      return;
   }
   this->desc->pagecolor = this->pcdata->colors[AType];
}

void char_data::set_pager_color( short AType )
{
   if( !this->desc )
      return;

   if( this->isnpc(  ) )
      return;

   this->desc->pager( color_str( AType ) );
   if( !this->desc )
   {
      bug( "%s: nullptr descriptor after WTP! CH: %s", __func__, this->name ? this->name : "Unknown?!?" );
      return;
   }
   this->desc->pagecolor = this->pcdata->colors[AType];
}

void char_data::set_title( const string & title )
{
   char buf[75];

   if( this->isnpc(  ) )
   {
      bug( "%s: NPC %s", __func__, this->name );
      return;
   }

   if( isalpha( title[0] ) || isdigit( title[0] ) )
   {
      buf[0] = ' ';
      mudstrlcpy( buf + 1, title.c_str(  ), 75 );
   }
   else
      mudstrlcpy( buf, title.c_str(  ), 75 );

   STRFREE( this->pcdata->title );
   this->pcdata->title = STRALLOC( buf );
}

int char_data::calculate_race_height(  )
{
   int Height;

   if( this->race < MAX_RACE )
      Height = number_range( ( int )( race_table[this->race]->height * 0.75 ), ( int )( race_table[this->race]->height * 1.25 ) );
   else
      Height = 72;   // FIXME: There's got to be some kind of randomness we can spin on this later.

   return Height;
}

int char_data::calculate_race_weight(  )
{
   int Weight;

   if( this->race < MAX_RACE )
      Weight = number_range( ( int )( race_table[this->race]->weight * 0.75 ), ( int )( race_table[this->race]->weight * 1.25 ) );
   else
      Weight = 180;  // FIXME: There's got to be some kind of randomness we can spin on this later.

   return Weight;
}

/*
 * Retrieve a character's trusted level for permission checking.
 */
short char_data::get_trust(  )
{
   char_data *ch;

   if( this->desc && this->desc->original )
      ch = this->desc->original;
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
   if( this->isnpc(  ) )
      return -1;

   return this->pcdata->age + this->pcdata->age_bonus;   // There, now isn't this simpler? :P
}

/* One hopes this will do as planned and determine how old a PC is based on the birthdate
   we record at creation. - Samson 10-25-99 */
short char_data::calculate_age(  )
{
   short age = 0, num_days = 0, ch_days = 0;

   if( this->isnpc(  ) )
      return -1;

   num_days = ( time_info.month + 1 ) * sysdata->dayspermonth;
   num_days += time_info.day;

   ch_days = ( this->pcdata->month + 1 ) * sysdata->dayspermonth;
   ch_days += this->pcdata->day;

   age = time_info.year - this->pcdata->year;

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

   if( this->isnpc(  ) )
      max = 25;
   else
      max = 20;

   return URANGE( 3, this->perm_str + this->mod_str, max );
}

/*
 * Retrieve character's current intelligence.
 */
short char_data::get_curr_int(  )
{
   short max = 0;

   if( this->isnpc(  ) )
      max = 25;
   else
      max = 20;

   return URANGE( 3, this->perm_int + this->mod_int, max );
}

/*
 * Retrieve character's current wisdom.
 */
short char_data::get_curr_wis(  )
{
   short max = 0;

   if( this->isnpc(  ) )
      max = 25;
   else
      max = 20;

   return URANGE( 3, this->perm_wis + this->mod_wis, max );
}

/*
 * Retrieve character's current dexterity.
 */
short char_data::get_curr_dex(  )
{
   short max = 0;

   if( this->isnpc(  ) )
      max = 25;
   else
      max = 20;

   return URANGE( 3, this->perm_dex + this->mod_dex, max );
}

/*
 * Retrieve character's current constitution.
 */
short char_data::get_curr_con(  )
{
   short max = 0;

   if( this->isnpc(  ) )
      max = 25;
   else
      max = 20;

   return URANGE( 3, this->perm_con + this->mod_con, max );
}

/*
 * Retrieve character's current charisma.
 */
short char_data::get_curr_cha(  )
{
   short max = 0;

   if( this->isnpc(  ) )
      max = 25;
   else
      max = 20;

   return URANGE( 3, this->perm_cha + this->mod_cha, max );
}

/*
 * Retrieve character's current luck.
 */
short char_data::get_curr_lck(  )
{
   short max = 0;

   if( this->isnpc(  ) )
      max = 25;
   else
      max = 20;

   return URANGE( 3, this->perm_lck + this->mod_lck, max );
}

/*
 * See if a player/mob can take a piece of prototype eq		-Thoric
 */
bool char_data::can_take_proto(  )
{
   if( this->is_immortal(  ) )
      return true;
   else if( this->has_actflag( ACT_PROTOTYPE ) )
      return true;
   else
      return false;
}

/*
 * True if char can see victim.
 */
bool char_data::can_see( char_data * victim, bool override )
{
   if( !victim )  /* Gorog - panicked attempt to stop crashes */
   {
      bug( "%s: nullptr victim! CH %s tried to see it.", __func__, name );
      return false;
   }

   if( !this )
   {
      if( victim->has_aflag( AFF_INVISIBLE ) || victim->has_aflag( AFF_HIDE ) || victim->has_pcflag( PCFLAG_WIZINVIS ) )
         return false;
      else
         return true;
   }

   if( !this->is_imp(  ) && victim == supermob )
      return false;

   if( this == victim )
      return true;

   if( victim->has_pcflag( PCFLAG_WIZINVIS ) && this->level < victim->pcdata->wizinvis )
      return false;

   /*
    * SB 
    */
   if( victim->has_actflag( ACT_MOBINVIS ) && this->level < victim->mobinvis )
      return false;

   /*
    * Deadlies link-dead over 2 ticks aren't seen by mortals -- Blodkai 
    */
   if( !this->is_immortal(  ) && !this->isnpc(  ) && !victim->isnpc(  ) && victim->IS_PKILL(  ) && victim->timer > 1 && !victim->desc )
      return false;

   if( ( this->has_pcflag( PCFLAG_ONMAP ) || this->has_actflag( ACT_ONMAP ) ) && !override )
   {
      if( !is_same_char_map( this, victim ) )
         return false;
   }

   /*
    * Unless you want to get spammed to death as an immortal on the map, DONT move this above here!!!! 
    */
   if( this->has_pcflag( PCFLAG_HOLYLIGHT ) )
      return true;

   /*
    * The miracle cure for blindness? -- Altrag 
    */
   if( !this->has_aflag( AFF_TRUESIGHT ) )
   {
      if( this->has_aflag( AFF_BLIND ) )
         return false;

      if( this->in_room->is_dark( this ) && !has_aflag( AFF_INFRARED ) )
         return false;

      if( victim->has_aflag( AFF_INVISIBLE ) && !this->has_aflag( AFF_DETECT_INVIS ) )
         return false;

      if( victim->has_aflag( AFF_HIDE ) && !this->has_aflag( AFF_DETECT_HIDDEN ) && !victim->fighting && ( this->isnpc(  )? !victim->isnpc(  ) : victim->isnpc(  ) ) )
         return false;
   }
   return true;
}

/*
 * Find a char in the room.
 */
char_data *char_data::get_char_room( const string & argument )
{
   string arg;
   int vnum = -1, count = 0;
   int number = number_argument( argument, arg );

   if( !str_cmp( arg, "self" ) )
      return this;

   if( this->get_trust(  ) >= LEVEL_SAVIOR && is_number( arg ) )
      vnum = atoi( arg.c_str(  ) );

   list < char_data * >::iterator ich;
   for( ich = this->in_room->people.begin(  ); ich != this->in_room->people.end(  ); ++ich )
   {
      char_data *rch = *ich;

      if( this->can_see( rch, false ) && !is_ignoring( rch, this ) && ( hasname( rch->name, arg ) || ( rch->isnpc(  ) && vnum == rch->pIndexData->vnum ) ) )
      {
         if( number == 0 && !rch->isnpc(  ) )
            return rch;
         else if( ++count == number )
            return rch;
      }
   }

   if( vnum != -1 )
      return nullptr;

   /*
    * If we didn't find an exact match, run through the list of characters
    * again looking for prefix matching, ie gu == guard.
    * Added by Narn, Sept/96
    */
   count = 0;
   for( ich = this->in_room->people.begin(  ); ich != this->in_room->people.end(  ); ++ich )
   {
      char_data *rch = *ich;

      if( !this->can_see( rch, false ) || !nifty_is_name_prefix( arg, rch->name ) || is_ignoring( rch, this ) )
         continue;

      if( number == 0 && !rch->isnpc(  ) )
         return rch;
      else if( ++count == number )
         return rch;
   }
   return nullptr;
}

/*
 * Find a char in the world.
 */
char_data *char_data::get_char_world( const string & argument )
{
   string arg;
   int number = number_argument( argument, arg );

   if( !str_cmp( arg, "self" ) )
      return this;

   /*
    * Allow reference by vnum for saints+ - Thoric
    */
   int vnum;
   if( this->get_trust(  ) >= LEVEL_SAVIOR && is_number( arg ) )
      vnum = atoi( arg.c_str(  ) );
   else
      vnum = -1;

   /*
    * check the room for an exact match [for certain values of exact in the case of mobs]
    */
   int count = 0;
   list < char_data * >::iterator ich;
   for( ich = this->in_room->people.begin(  ); ich != this->in_room->people.end(  ); ++ich )
   {
      char_data *wch = *ich;

      if( !wch->isnpc() )
      {
         if( !str_cmp( wch->name, arg ) && this->can_see( wch, true ) && !is_ignoring( wch, this ) )
            return wch;
      }
      else if( ( is_name2_prefix( arg, wch->name ) && this->can_see( wch, true ) ) || vnum == wch->pIndexData->vnum )
      {
         if( number == count )
            return wch;

         ++count;
      }
   }

   /*
    * check the world for an exact match [for certain values of exact in the case of mobs]
    */
   count = 0;
   for( ich = charlist.begin(  ); ich != charlist.end(  ); ++ich )
   {
      char_data *wch = *ich;

      if( !wch->isnpc() )
      {
         if( !str_cmp( wch->name, arg ) && this->can_see( wch, true ) && !is_ignoring( wch, this ) )
            return wch;
      }
      else if( ( is_name2_prefix( arg, wch->name ) && this->can_see( wch, true ) ) || vnum == wch->pIndexData->vnum )
      {
         if( number == count )
            return wch;

         ++count;
      }
   }

   /*
    * bail out if looking for a vnum match 
    */
   if( vnum != -1 )
      return nullptr;

   /*
    * If we didn't find an exact match, check the room for
    * for a prefix match, ie gu == guard.
    * Added by Narn, Sept/96
    */
   count = 0;
   for( ich = this->in_room->people.begin(  ); ich != this->in_room->people.end(  ); ++ich )
   {
      char_data *wch = *ich;

      if( !this->can_see( wch, true ) || !nifty_is_name_prefix( arg, wch->name ) || is_ignoring( wch, this ) )
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
   for( ich = charlist.begin(  ); ich != charlist.end(  ); ++ich )
   {
      char_data *wch = *ich;

      if( !this->can_see( wch, true ) || !nifty_is_name_prefix( arg, wch->name ) || is_ignoring( wch, this ) )
         continue;
      if( number == 0 && !wch->isnpc(  ) )
         return wch;
      else if( ++count == number )
         return wch;
   }
   return nullptr;
}

room_index *char_data::find_location( const string & arg )
{
   char_data *victim;
   obj_data *obj;

   if( is_number( arg ) )
      return get_room_index( atoi( arg.c_str(  ) ) );

   if( !str_cmp( arg, "pk" ) )   /* "Goto pk", "at pk", etc */
      return get_room_index( last_pkroom );

   if( ( victim = this->get_char_world( arg ) ) != nullptr )
      return victim->in_room;

   if( ( obj = this->get_obj_world( arg ) ) != nullptr )
      return obj->in_room;

   return nullptr;
}

/*
 * True if char can see obj.
 * Override boolean to bypass overland coordinate checks - Samson 10-17-03
 */
bool char_data::can_see_obj( obj_data * obj, bool override )
{
   // This check ALWAYS needs to be first because otherwise imms and the Supermob will see a flood on the overland.
   if( !is_same_obj_map( this, obj ) && !override )
      return false;

   if( this->has_pcflag( PCFLAG_HOLYLIGHT ) )
      return true;

   if( this->isnpc(  ) && this->pIndexData->vnum == MOB_VNUM_SUPERMOB )
      return true;

   if( obj->extra_flags.test( ITEM_BURIED ) )
      return false;

   if( obj->extra_flags.test( ITEM_HIDDEN ) )
      return false;

   if( this->has_aflag( AFF_TRUESIGHT ) )
      return true;

   if( this->has_aflag( AFF_BLIND ) )
      return false;

   /*
    * can see lights in the dark 
    */
   if( obj->item_type == ITEM_LIGHT && obj->value[2] != 0 )
      return true;

   if( this->in_room->is_dark( this ) )
   {
      /*
       * can see glowing items in the dark... invisible or not 
       */
      if( obj->extra_flags.test( ITEM_GLOW ) )
         return true;
      if( !this->has_aflag( AFF_INFRARED ) )
         return false;
   }

   if( obj->extra_flags.test( ITEM_INVIS ) && !this->has_aflag( AFF_DETECT_INVIS ) )
      return false;

   return true;
}

/*
 * True if char can drop obj.
 */
bool char_data::can_drop_obj( obj_data * obj )
{
   if( !obj->extra_flags.test( ITEM_NODROP ) && !obj->extra_flags.test( ITEM_PERMANENT ) )
      return true;

   if( !this->is_immortal(  ) )
      return true;

   if( this->isnpc(  ) && this->pIndexData->vnum == MOB_VNUM_SUPERMOB )
      return true;

   return false;
}

/*
 * Find an obj in player's inventory or wearing via a vnum -Shaddai
 */
obj_data *char_data::get_obj_vnum( int vnum )
{
   list < obj_data * >::iterator iobj;

   for( iobj = this->carrying.begin(  ); iobj != this->carrying.end(  ); ++iobj )
   {
      obj_data *obj = *iobj;

      if( this->can_see_obj( obj, false ) && obj->pIndexData->vnum == vnum )
         return obj;
   }
   return nullptr;
}

/*
 * Find an obj in player's inventory.
 */
obj_data *char_data::get_obj_carry( const string & argument )
{
   string arg;
   obj_data *obj;
   list < obj_data * >::iterator iobj;
   int number, count, vnum;

   number = number_argument( argument, arg );
   if( this->get_trust(  ) >= LEVEL_SAVIOR && is_number( arg ) )
      vnum = atoi( arg.c_str(  ) );
   else
      vnum = -1;

   count = 0;
   for( iobj = this->carrying.begin(  ); iobj != this->carrying.end(  ); ++iobj )
   {
      obj = *iobj;

      if( obj->wear_loc == WEAR_NONE && this->can_see_obj( obj, false ) && ( hasname( obj->name, arg ) || obj->pIndexData->vnum == vnum ) )
         if( ( count += obj->count ) >= number )
            return obj;
   }
   if( vnum != -1 )
      return nullptr;

   /*
    * If we didn't find an exact match, run through the list of objects
    * again looking for prefix matching, ie swo == sword.
    * Added by Narn, Sept/96
    */
   count = 0;
   for( iobj = this->carrying.begin(  ); iobj != this->carrying.end(  ); ++iobj )
   {
      obj = *iobj;

      if( obj->wear_loc == WEAR_NONE && this->can_see_obj( obj, false ) && nifty_is_name_prefix( arg, obj->name ) )
         if( ( count += obj->count ) >= number )
            return obj;
   }
   return nullptr;
}

/*
 * Find an obj in player's equipment.
 */
obj_data *char_data::get_obj_wear( const string & argument )
{
   string arg;
   obj_data *obj;
   list < obj_data * >::iterator iobj;
   int number, count, vnum;

   number = number_argument( argument, arg );

   if( this->get_trust(  ) >= LEVEL_SAVIOR && is_number( arg ) )
      vnum = atoi( arg.c_str(  ) );
   else
      vnum = -1;

   count = 0;
   for( iobj = this->carrying.begin(  ); iobj != this->carrying.end(  ); ++iobj )
   {
      obj = *iobj;

      if( obj->wear_loc != WEAR_NONE && this->can_see_obj( obj, false ) && ( hasname( obj->name, arg ) || obj->pIndexData->vnum == vnum ) )
         if( ++count == number )
            return obj;
   }
   if( vnum != -1 )
      return nullptr;

   /*
    * If we didn't find an exact match, run through the list of objects
    * again looking for prefix matching, ie swo == sword.
    * Added by Narn, Sept/96
    */
   count = 0;
   for( iobj = this->carrying.begin(  ); iobj != this->carrying.end(  ); ++iobj )
   {
      obj = *iobj;

      if( obj->wear_loc != WEAR_NONE && this->can_see_obj( obj, false ) && nifty_is_name_prefix( arg, obj->name ) )
         if( ++count == number )
            return obj;
   }
   return nullptr;
}

/*
 * Find an obj in the room or in inventory.
 */
obj_data *char_data::get_obj_here( const string & argument )
{
   obj_data *obj;

   if( ( obj = get_obj_list( this, argument, this->in_room->objects ) ) )
      return obj;

   if( ( obj = this->get_obj_carry( argument ) ) )
      return obj;

   if( ( obj = this->get_obj_wear( argument ) ) )
      return obj;

   return nullptr;
}

/*
 * Find an obj in the world.
 */
obj_data *char_data::get_obj_world( const string & argument )
{
   string arg;
   obj_data *obj;
   list < obj_data * >::iterator iobj;
   int number, count, vnum;

   if( ( obj = this->get_obj_here( argument ) ) != nullptr )
      return obj;

   number = number_argument( argument, arg );

   /*
    * Allow reference by vnum for saints+       -Thoric
    */
   if( this->get_trust(  ) >= LEVEL_SAVIOR && is_number( arg ) )
      vnum = atoi( arg.c_str(  ) );
   else
      vnum = -1;

   count = 0;
   for( iobj = objlist.begin(  ); iobj != objlist.end(  ); ++iobj )
   {
      obj = *iobj;

      if( this->can_see_obj( obj, true ) && ( hasname( obj->name, arg ) || vnum == obj->pIndexData->vnum ) )
         if( ( count += obj->count ) >= number )
            return obj;
   }

   /*
    * bail out if looking for a vnum 
    */
   if( vnum != -1 )
      return nullptr;

   /*
    * If we didn't find an exact match, run through the list of objects
    * again looking for prefix matching, ie swo == sword.
    * Added by Narn, Sept/96
    */
   count = 0;
   for( iobj = objlist.begin(  ); iobj != objlist.end(  ); ++iobj )
   {
      obj = *iobj;

      if( this->can_see_obj( obj, true ) && nifty_is_name_prefix( arg, obj->name ) )
         if( ( count += obj->count ) >= number )
            return obj;
   }
   return nullptr;
}

/*
 * Find a piece of eq on a character.
 * Will pick the top layer if clothing is layered. - Thoric
 */
obj_data *char_data::get_eq( int iWear )
{
   obj_data *maxobj = nullptr;
   list < obj_data * >::iterator iobj;

   for( iobj = this->carrying.begin(  ); iobj != this->carrying.end(  ); ++iobj )
   {
      obj_data *obj = *iobj;

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

   if( this->is_immortal(  ) )
      return this->level * 200;

   if( !this->isnpc(  ) && this->Class == CLASS_MONK )
      return 20;  /* Monks can only carry 20 items total */

   /*
    * Come now, never heard of pack animals people? Altered so pets can hold up to 10 - Samson 4-12-98 
    */
   if( this->is_pet(  ) )
      return 10;

   if( this->has_actflag( ACT_IMMORTAL ) )
      return this->level * 200;

   if( this->get_eq( WEAR_WIELD ) )
      ++penalty;
   if( this->get_eq( WEAR_DUAL_WIELD ) )
      ++penalty;
   if( this->get_eq( WEAR_MISSILE_WIELD ) )
      ++penalty;
   if( this->get_eq( WEAR_HOLD ) )
      ++penalty;
   if( this->get_eq( WEAR_SHIELD ) )
      ++penalty;

   /*
    * Removed old formula, added something a bit more sensible here
    * Samson 4-12-98. Minimum of 15, (dex+str+level)-10 - penalty, or a max of 40. 
    */
   return URANGE( 15, ( this->get_curr_dex(  ) + this->get_curr_str(  ) + this->level ) - 10 - penalty, 40 );
}

/*
 * Retrieve a character's carry capacity.
 */
int char_data::can_carry_w(  )
{
   if( this->is_immortal(  ) )
      return 1000000;

   if( this->has_actflag( ACT_IMMORTAL ) )
      return 1000000;

   return str_app[this->get_curr_str(  )].carry;
}

/*
 * Equip a char with an obj.
 */
void char_data::equip( obj_data * obj, int iWear )
{
   obj_data *otmp;

   if( ( otmp = this->get_eq( iWear ) ) != nullptr && ( !otmp->pIndexData->layers || !obj->pIndexData->layers ) )
   {
      bug( "%s: already equipped (%d).", __func__, iWear );
      return;
   }

   if( obj->carried_by != this )
   {
      bug( "%s: obj (%s) not being carried by ch (%s)!", __func__, obj->name, this->name );
      return;
   }

   obj->separate(  );   /* just in case */
   if( ( obj->extra_flags.test( ITEM_ANTI_EVIL ) && IS_EVIL(  ) )
       || ( obj->extra_flags.test( ITEM_ANTI_GOOD ) && IS_GOOD(  ) ) || ( obj->extra_flags.test( ITEM_ANTI_NEUTRAL ) && IS_NEUTRAL(  ) ) )
   {
      /*
       * Thanks to Morgenes for the bug fix here!
       */
      if( loading_char != this )
      {
         act( AT_MAGIC, "You are zapped by $p and drop it.", this, obj, nullptr, TO_CHAR );
         act( AT_MAGIC, "$n is zapped by $p and drops it.", this, obj, nullptr, TO_ROOM );
      }
      if( obj->carried_by )
         obj->from_char(  );
      obj->to_room( in_room, this );
      oprog_zap_trigger( this, obj );
      if( IS_SAVE_FLAG( SV_ZAPDROP ) && !this->char_died(  ) )
         this->save(  );
      return;
   }

   this->armor -= obj->apply_ac( iWear );
   obj->wear_loc = iWear;

   this->carry_number -= obj->get_number(  );
   if( obj->extra_flags.test( ITEM_MAGIC ) )
      this->carry_weight -= obj->get_weight(  );

   affect_data *af;
   list < affect_data * >::iterator paf;
   for( paf = obj->pIndexData->affects.begin(  ); paf != obj->pIndexData->affects.end(  ); ++paf )
   {
      af = *paf;
      this->affect_modify( af, true );
   }

   for( paf = obj->affects.begin(  ); paf != obj->affects.end(  ); ++paf )
   {
      af = *paf;
      this->affect_modify( af, true );
   }

   if( obj->item_type == ITEM_LIGHT && ( obj->value[2] != 0 || IS_SET( obj->value[3], PIPE_LIT ) ) && this->in_room )
      ++this->in_room->light;
}

/*
 * Unequip a char with an obj.
 */
void char_data::unequip( obj_data * obj )
{
   obj_data *tobj;

   if( obj->wear_loc == WEAR_NONE )
   {
      bug( "%s: %s already unequipped.", __func__, name );
      return;
   }

   /*
    * Bug fix to prevent dual wielding from getting stuck. Fix by Gianfranco
    * * Finell   -- Shaddai
    */
   if( obj->wear_loc == WEAR_WIELD && this != saving_char
       && this != loading_char && this != quitting_char && ( tobj = this->get_eq( WEAR_DUAL_WIELD ) ) != nullptr )
      tobj->wear_loc = WEAR_WIELD;

   this->carry_number += obj->get_number(  );
   if( obj->extra_flags.test( ITEM_MAGIC ) )
      this->carry_weight += obj->get_weight(  );

   this->armor += obj->apply_ac( obj->wear_loc );
   obj->wear_loc = -1;

   affect_data *af;
   list < affect_data * >::iterator paf;
   for( paf = obj->pIndexData->affects.begin(  ); paf != obj->pIndexData->affects.end(  ); ++paf )
   {
      af = *paf;

      this->affect_modify( af, false );
   }

   if( obj->carried_by )
   {
      for( paf = obj->affects.begin(  ); paf != obj->affects.end(  ); ++paf )
      {
         af = *paf;

         this->affect_modify( af, false );
      }
   }
   this->update_aris(  );

   if( !obj->carried_by )
      return;

   if( obj->item_type == ITEM_LIGHT && ( obj->value[2] != 0 || IS_SET( obj->value[3], PIPE_LIT ) ) && this->in_room && this->in_room->light > 0 )
      --this->in_room->light;
}

/*
 * Apply only affected and RIS on a char
 */
void char_data::aris_affect( affect_data * paf )
{
   int location = paf->location % REVERSE_APPLY;

   /*
    * How the hell have you SmaugDevs gotten away with this for so long! 
    */
   if( paf->bit >= 0 && paf->bit < MAX_AFFECTED_BY )
      this->affected_by.set( paf->bit );

   switch ( location )
   {
      default:
         break;

      case APPLY_AFFECT:
         if( paf->modifier >= 0 && paf->modifier < MAX_AFFECTED_BY )
            this->affected_by.set( paf->modifier );
         break;

      case APPLY_RESISTANT:
         this->resistant |= paf->rismod;
         break;

      case APPLY_IMMUNE:
         this->immune |= paf->rismod;
         break;

      case APPLY_SUSCEPTIBLE:
         this->susceptible |= paf->rismod;
         break;

      case APPLY_ABSORB:
         this->absorb |= paf->rismod;
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
   if( this->is_immortal(  ) )
      return;

   /*
    * So chars using hide skill will continue to hide 
    */
   bool hiding = this->affected_by.test( AFF_HIDE );

   if( !this->isnpc(  ) )  /* Because we don't want NPCs to be purged of EVERY effect the have */
   {
      this->affected_by.reset(  );
      this->resistant.reset(  );
      this->immune.reset(  );
      this->susceptible.reset(  );
      this->absorb.reset(  );
      this->no_affected_by.reset(  );
      this->no_resistant.reset(  );
      this->no_immune.reset(  );
      this->no_susceptible.reset(  );
   }

   /*
    * Add in effects from race 
    * Because NPCs can have races MUCH higher than this that the table doesn't define yet 
    */
   if( this->race <= RACE_19 )
   {
      this->affected_by |= race_table[race]->affected;
      this->resistant |= race_table[race]->resist;
      this->susceptible |= race_table[race]->suscept;
   }

   /*
    * Add in effects from Class 
    * Because NPCs can have classes higher than the table allows 
    */
   if( this->Class <= CLASS_BARD )
   {
      this->affected_by |= class_table[Class]->affected;
      this->resistant |= class_table[Class]->resist;
      this->susceptible |= class_table[Class]->suscept;
   }
   this->ClassSpecificStuff(  ); /* Brought over from DOTD code - Samson 4-6-99 */

   /*
    * Add in effects from deities 
    */
   if( !this->isnpc(  ) && this->pcdata->deity )
   {
      if( this->pcdata->favor > this->pcdata->deity->affectednum[0] )
      {
         if( this->pcdata->deity->affected[0] > 0 )
            this->affected_by.set( this->pcdata->deity->affected[0] );
      }
      if( this->pcdata->favor > this->pcdata->deity->affectednum[1] )
      {
         if( this->pcdata->deity->affected[1] > 0 )
            this->affected_by.set( this->pcdata->deity->affected[1] );
      }
      if( this->pcdata->favor > this->pcdata->deity->affectednum[2] )
      {
         if( this->pcdata->deity->affected[2] > 0 )
            this->affected_by.set( this->pcdata->deity->affected[2] );
      }
      if( this->pcdata->favor > this->pcdata->deity->elementnum[0] )
      {
         if( this->pcdata->deity->element[0] != 0 )
            this->set_resist( this->pcdata->deity->element[0] );
      }
      if( this->pcdata->favor > this->pcdata->deity->elementnum[1] )
      {
         if( this->pcdata->deity->element[1] != 0 )
            this->set_resist( this->pcdata->deity->element[1] );
      }
      if( this->pcdata->favor > this->pcdata->deity->elementnum[2] )
      {
         if( this->pcdata->deity->element[2] != 0 )
            this->set_resist( this->pcdata->deity->element[2] );
      }
      if( this->pcdata->favor < this->pcdata->deity->susceptnum[0] )
      {
         if( this->pcdata->deity->suscept[0] != 0 )
            this->set_suscep( this->pcdata->deity->suscept[0] );
      }
      if( this->pcdata->favor < this->pcdata->deity->susceptnum[1] )
      {
         if( this->pcdata->deity->suscept[1] != 0 )
            this->set_suscep( this->pcdata->deity->suscept[1] );
      }
      if( this->pcdata->favor < this->pcdata->deity->susceptnum[2] )
      {
         if( this->pcdata->deity->suscept[2] != 0 )
            this->set_suscep( this->pcdata->deity->suscept[2] );
      }
   }

   /*
    * Add in effect from spells 
    */
   affect_data *af;
   list < affect_data * >::iterator paf;
   for( paf = this->affects.begin(  ); paf != this->affects.end(  ); ++paf )
   {
      af = *paf;

      this->aris_affect( af );
   }

   /*
    * Add in effects from equipment 
    */
   list < obj_data * >::iterator iobj;
   for( iobj = this->carrying.begin(  ); iobj != this->carrying.end(  ); ++iobj )
   {
      obj_data *obj = *iobj;

      if( obj->wear_loc != WEAR_NONE )
      {
         for( paf = obj->affects.begin(  ); paf != obj->affects.end(  ); ++paf )
         {
            af = *paf;

            this->aris_affect( af );
         }

         for( paf = obj->pIndexData->affects.begin(  ); paf != obj->pIndexData->affects.end(  ); ++paf )
         {
            af = *paf;

            this->aris_affect( af );
         }
      }
   }

   /*
    * Add in effects from the room 
    */
   if( this->in_room )  /* non-existant char booboo-fix --TRI */
   {
      for( paf = this->in_room->affects.begin(  ); paf != this->in_room->affects.end(  ); ++paf )
      {
         af = *paf;

         this->aris_affect( af );
      }

      for( paf = this->in_room->permaffects.begin(  ); paf != this->in_room->permaffects.end(  ); ++paf )
      {
         af = *paf;

         this->aris_affect( af );
      }
   }

   /*
    * Add in effects for polymorph 
    */
   if( !this->isnpc(  ) && this->morph )
   {
      this->affected_by |= this->morph->affected_by;
      this->immune |= this->morph->immune;
      this->resistant |= this->morph->resistant;
      this->susceptible |= this->morph->suscept;
      this->absorb |= this->morph->absorb;
      /*
       * Right now only morphs have no_ things --Shaddai 
       */
      this->no_affected_by |= this->morph->no_affected_by;
      this->no_immune |= this->morph->no_immune;
      this->no_resistant |= this->morph->no_resistant;
      this->no_susceptible |= this->morph->no_suscept;
   }

   /*
    * If they were hiding before, make them hiding again 
    */
   if( hiding )
      this->affected_by.set( AFF_HIDE );
}

// For an NPC to be a pet, all of these conditions must be met.
bool char_data::is_pet(  )
{
   if( !this->isnpc(  ) )
      return false;
   if( !this->has_actflag( ACT_PET ) )
      return false;
   if( !this->is_affected( gsn_charm_person ) )
      return false;
   if( !this->master )
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
   if( !this->isnpc(  ) )
   {
      if( fAdd )
         this->pcdata->learned[sn] += mod;
      else
         this->pcdata->learned[sn] = URANGE( 0, this->pcdata->learned[sn] + mod, this->GET_ADEPT( sn ) );
   }
}

/*
 * Apply or remove an affect to a character.
 */
void char_data::affect_modify( affect_data * paf, bool fAdd )
{
   obj_data *WPos, *DPos, *MPos, *ToDrop;
   int WWeight, DWeight, MWeight, ToDropW;
   int mod, mod2 = -1;
   class skill_type *skill;
   int location = paf->location % REVERSE_APPLY;

   mod = paf->modifier;

   /*
    * God, this is a hackish mess! Thanks alot Smaugdevs! 
    */
   if( paf->location == APPLY_AFFECT || paf->location == APPLY_REMOVE || paf->location == APPLY_EXT_AFFECT )
   {
      if( paf->bit < 0 || paf->bit >= MAX_AFFECTED_BY )
      {
         bug( "%s: %s: Unknown bitflag: '%d' for location %d, with modifier %d", __func__, name, paf->bit, paf->location, paf->modifier );
         return;
      }
      mod2 = mod;
   }

   if( fAdd )
   {
      if( paf->bit >= 0 && paf->bit < MAX_AFFECTED_BY )
         this->set_aflag( paf->bit );

      if( location == APPLY_RECURRINGSPELL )
      {
         mod = abs( mod );
         if( IS_VALID_SN( mod ) && ( skill = skill_table[mod] ) != nullptr && skill->type == SKILL_SPELL )
            this->set_aflag( AFF_RECURRINGSPELL );
         else
            bug( "%s: (%s) APPLY_RECURRINGSPELL with bad sn %d", __func__, this->name, mod );
         return;
      }
   }
   else
   {
      if( paf->bit >= 0 && paf->bit < MAX_AFFECTED_BY )
         this->unset_aflag( paf->bit );
      /*
       * might be an idea to have a duration removespell which returns
       * the spell after the duration... but would have to store
       * the removed spell's information somewhere...    -Thoric
       */
      if( location == APPLY_RECURRINGSPELL )
      {
         mod = abs( mod );
         if( !IS_VALID_SN( mod ) || ( skill = skill_table[mod] ) == nullptr || skill->type != SKILL_SPELL )
            bug( "%s: (%s) APPLY_RECURRINGSPELL with bad sn %d", __func__, this->name, mod );
         this->unset_aflag( AFF_RECURRINGSPELL );
         return;
      }

      switch ( location )
      {
         case APPLY_AFFECT:
         case APPLY_EXT_AFFECT:
            this->unset_aflag( mod2 );
            return;

         case APPLY_RESISTANT:
            this->resistant &= ~( paf->rismod );
            return;

         case APPLY_IMMUNE:
            this->immune &= ~( paf->rismod );
            return;

         case APPLY_SUSCEPTIBLE:
            this->susceptible &= ~( paf->rismod );
            return;

         case APPLY_ABSORB:
            this->absorb &= ~( paf->rismod );
            return;

         case APPLY_REMOVE:
            this->set_aflag( mod2 );
            return;

         default:
            break;
      }
      mod = 0 - mod;
   }

   // Only reaches this pont when an affect is being added.
   switch ( location )
   {
      default:
         bug( "%s: unknown location %d. (%s)", __func__, paf->location, this->name );
         return;

      case APPLY_NONE:
         break;
      case APPLY_STR:
         this->mod_str += mod;
         break;
      case APPLY_DEX:
         this->mod_dex += mod;
         break;
      case APPLY_INT:
         this->mod_int += mod;
         break;
      case APPLY_WIS:
         this->mod_wis += mod;
         break;
      case APPLY_CON:
         this->mod_con += mod;
         break;
      case APPLY_CHA:
         mod_cha += mod;
         break;
      case APPLY_LCK:
         this->mod_lck += mod;
         break;
      case APPLY_ALLSTATS:
         this->mod_str += mod;
         this->mod_dex += mod;
         this->mod_int += mod;
         this->mod_wis += mod;
         this->mod_con += mod;
         this->mod_cha += mod;
         this->mod_lck += mod;
         break;
      case APPLY_SEX:
         this->sex = ( this->sex + mod ) % 3;
         if( this->sex < 0 )
            this->sex += 2;
         this->sex = URANGE( 0, this->sex, 4 );
         break;
      case APPLY_ATTACKS:
         this->numattacks += mod;
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
         this->spellfail += mod;
         break;
      case APPLY_RACE:
         this->race += mod;
         break;
      case APPLY_AGE:
         if( !this->isnpc(  ) )
            this->pcdata->age_bonus += mod;
         break;
      case APPLY_HEIGHT:
         this->height += mod;
         break;
      case APPLY_WEIGHT:
         this->weight += mod;
         break;
      case APPLY_MANA:
         this->max_mana += mod;
         break;
      case APPLY_HIT:
         this->max_hit += mod;
         break;
      case APPLY_MOVE:
         this->max_move += mod;
         break;
      case APPLY_MANA_REGEN:
         this->mana_regen += mod;
         break;
      case APPLY_HIT_REGEN:
         this->hit_regen += mod;
         break;
      case APPLY_MOVE_REGEN:
         this->move_regen += mod;
         break;
      case APPLY_ANTIMAGIC:
         this->amp += mod;
         break;
      case APPLY_AC:
         this->armor += mod;
         break;
      case APPLY_HITROLL:
         this->hitroll += mod;
         break;
      case APPLY_DAMROLL:
         this->damroll += mod;
         break;
      case APPLY_HITNDAM:
         this->hitroll += mod;
         this->damroll += mod;
         break;
      case APPLY_SAVING_POISON:
         this->saving_poison_death += mod;
         break;
      case APPLY_SAVING_ROD:
         this->saving_wand += mod;
         break;
      case APPLY_SAVING_PARA:
         this->saving_para_petri += mod;
         break;
      case APPLY_SAVING_BREATH:
         this->saving_breath += mod;
         break;
      case APPLY_SAVING_SPELL:
         this->saving_spell_staff += mod;
         break;
      case APPLY_SAVING_ALL:
         this->saving_poison_death += mod;
         this->saving_para_petri += mod;
         this->saving_breath += mod;
         this->saving_spell_staff += mod;
         this->saving_wand += mod;
         break;
      case APPLY_AFFECT:
      case APPLY_EXT_AFFECT:
         this->set_aflag( mod2 );
         break;
      case APPLY_RESISTANT:
         this->resistant |= paf->rismod;
         break;
      case APPLY_IMMUNE:
         this->immune |= paf->rismod;
         break;
      case APPLY_SUSCEPTIBLE:
         this->susceptible |= paf->rismod;
         break;
      case APPLY_ABSORB:
         this->absorb |= paf->rismod;
         break;
      case APPLY_WEAPONSPELL:   /* see fight.cpp */
         break;
      case APPLY_REMOVE:
         this->unset_aflag( mod2 );
         break;

      case APPLY_FULL:
         if( !this->isnpc(  ) )
            this->pcdata->condition[COND_FULL] = URANGE( 0, this->pcdata->condition[COND_FULL] + mod, sysdata->maxcondval );
         break;

      case APPLY_THIRST:
         if( !this->isnpc(  ) )
            this->pcdata->condition[COND_THIRST] = URANGE( 0, this->pcdata->condition[COND_THIRST] + mod, sysdata->maxcondval );
         break;

      case APPLY_DRUNK:
         if( !this->isnpc(  ) )
            this->pcdata->condition[COND_DRUNK] = URANGE( 0, this->pcdata->condition[COND_DRUNK] + mod, sysdata->maxcondval );
         break;

      case APPLY_MENTALSTATE:
         this->mental_state = URANGE( -100, this->mental_state + mod, 100 );
         break;

      case APPLY_CONTAGIOUS:
         break;
      case APPLY_ODOR:
         break;

      case APPLY_STRIPSN:
         if( IS_VALID_SN( mod ) )
            this->affect_strip( mod );
         else
            bug( "%s: APPLY_STRIPSN invalid sn %d", __func__, mod );
         break;

         /*
          * spell cast upon wear/removal of an object -Thoric 
          */
      case APPLY_WEARSPELL:
      case APPLY_REMOVESPELL:
         if( ( this->in_room && this->in_room->flags.test( ROOM_NO_MAGIC ) ) || this->has_immune( RIS_MAGIC ) || ( ( paf->location % REVERSE_APPLY ) == APPLY_WEARSPELL && !fAdd ) || ( ( paf->location % REVERSE_APPLY ) == APPLY_REMOVESPELL && !fAdd ) || saving_char == this /* so save/quit doesn't trigger */
             || loading_char == this ) /* so loading doesn't trigger */
            return;

         mod = abs( mod );
         if( IS_VALID_SN( mod ) && ( skill = skill_table[mod] ) != nullptr && skill->type == SKILL_SPELL )
            if( ( ( *skill->spell_fun ) ( mod, this->level, this, this ) ) == rCHAR_DIED || char_died(  ) )
               return;
         break;

         /*
          * skill apply types  -Thoric 
          */
      case APPLY_PALM: /* not implemented yet */
         break;
      case APPLY_PEEK:
         this->modify_skill( gsn_peek, mod, fAdd );
         break;
      case APPLY_TRACK:
         this->modify_skill( gsn_track, mod, fAdd );
         break;
      case APPLY_HIDE:
         this->modify_skill( gsn_hide, mod, fAdd );
         break;
      case APPLY_STEAL:
         this->modify_skill( gsn_steal, mod, fAdd );
         break;
      case APPLY_SNEAK:
         this->modify_skill( gsn_sneak, mod, fAdd );
         break;
      case APPLY_PICK:
         this->modify_skill( gsn_pick_lock, mod, fAdd );
         break;
      case APPLY_BACKSTAB:
         this->modify_skill( gsn_backstab, mod, fAdd );
         break;
      case APPLY_DETRAP:
         this->modify_skill( gsn_detrap, mod, fAdd);
         break;
      case APPLY_DODGE:
         this->modify_skill( gsn_dodge, mod, fAdd );
         break;
      case APPLY_SCAN:
         break;
      case APPLY_GOUGE:
         this->modify_skill( gsn_gouge, mod, fAdd );
         break;
      case APPLY_SEARCH:
         this->modify_skill( gsn_search, mod, fAdd );
         break;
      case APPLY_DIG:
         this->modify_skill( gsn_dig, mod, fAdd );
         break;
      case APPLY_MOUNT:
         this->modify_skill( gsn_mount, mod, fAdd );
         break;
      case APPLY_DISARM:
         this->modify_skill( gsn_disarm, mod, fAdd );
         break;
      case APPLY_KICK:
         this->modify_skill( gsn_kick, mod, fAdd );
         break;
      case APPLY_PARRY:
         this->modify_skill( gsn_parry, mod, fAdd );
         break;
      case APPLY_BASH:
         this->modify_skill( gsn_bash, mod, fAdd );
         break;
      case APPLY_STUN:
         this->modify_skill( gsn_stun, mod, fAdd );
         break;
      case APPLY_PUNCH:
         this->modify_skill( gsn_punch, mod, fAdd );
         break;
      case APPLY_CLIMB:
         this->modify_skill( gsn_climb, mod, fAdd );
         break;
      case APPLY_GRIP:
         this->modify_skill( gsn_grip, mod, fAdd );
         break;
      case APPLY_SCRIBE:
         this->modify_skill( gsn_scribe, mod, fAdd );
         break;
      case APPLY_BREW:
         this->modify_skill( gsn_brew, mod, fAdd );
         break;
      case APPLY_COOK:
         this->modify_skill( gsn_cook, mod, fAdd );
         break;

      case APPLY_EXTRAGOLD:  /* SHUT THE HELL UP LOCATION 89! */
         if( !this->isnpc(  ) )
            this->pcdata->exgold += mod;
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
   ToDrop = WPos = this->get_eq( WEAR_WIELD );
   ToDropW = WWeight = WPos ? WPos->get_weight( ) : 0;
   DWeight = MWeight = 0;

   DPos = this->get_eq( WEAR_DUAL_WIELD );

   if( DPos && ( DWeight = DPos->get_weight( ) ) > ToDropW )
   {
      ToDrop = DPos;
      ToDropW = WWeight;
   }

   MPos = this->get_eq( WEAR_MISSILE_WIELD );
   if( MPos && ( MWeight = MPos->get_weight( ) ) > ToDropW )
   {
      ToDrop = MPos;
      ToDropW = MWeight;
   }

   if( !this->isnpc() && saving_char != this
       /*
        * &&  (wield = get_eq_char(ch, WEAR_WIELD) ) != nullptr
        * &&   get_obj_weight(wield) > str_app[get_curr_str(ch)].wield ) 
        */
       && ( WWeight + DWeight + MWeight ) > str_app[this->get_curr_str( )].wield )
   {
      static int depth;

      if( depth <= 1 )
      {
         ++depth;

         act( AT_ACTION, "You are too weak to wield $p any longer.", this, ToDrop, nullptr, TO_CHAR );
         act( AT_ACTION, "$n stops wielding $p.", this, ToDrop, nullptr, TO_ROOM );

         this->unequip( ToDrop );
         --depth;
      }
   }
}

/*
 * Give an affect to a char.
 */
void char_data::affect_to_char( affect_data * paf )
{
   affect_data *paf_new;

   if( !this )
   {
      bug( "%s: (nullptr, %d)", __func__, paf ? paf->type : 0 );
      return;
   }

   if( !paf )
   {
      bug( "%s: (%s, nullptr)", __func__, name );
      return;
   }

   paf_new = new affect_data;
   this->affects.push_back( paf_new );
   paf_new->type = paf->type;
   paf_new->duration = paf->duration;
   paf_new->location = paf->location;
   paf_new->modifier = paf->modifier;
   paf_new->rismod = paf->rismod;
   paf_new->bit = paf->bit;

   this->affect_modify( paf_new, true );
}

/*
 * Remove an affect from a char.
 */
void char_data::affect_remove( affect_data * paf )
{
   if( affects.empty(  ) )
   {
      bug( "%s: (%s, %d): no affect.", __func__, name, paf ? paf->type : 0 );
      return;
   }

   this->affect_modify( paf, false );

   this->affects.remove( paf );
   deleteptr( paf );
}

/*
 * Strip all affects of a given sn.
 */
void char_data::affect_strip( int sn )
{
   list < affect_data * >::iterator paf;

   for( paf = this->affects.begin(  ); paf != this->affects.end(  ); )
   {
      affect_data *aff = *paf;
      ++paf;

      if( aff->type == sn )
         this->affect_remove( aff );
   }
}

/*
 * Return true if a char is affected by a spell.
 */
bool char_data::is_affected( int sn )
{
   list < affect_data * >::iterator paf;

   for( paf = this->affects.begin(  ); paf != this->affects.end(  ); ++paf )
   {
      affect_data *af = *paf;

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
   list < affect_data * >::iterator paf_old;

   for( paf_old = this->affects.begin(  ); paf_old != this->affects.end(  ); ++paf_old )
   {
      affect_data *af = *paf_old;

      if( af->type == paf->type )
      {
         paf->duration = UMIN( 32500, paf->duration + af->duration );
         if( paf->modifier )
            paf->modifier = UMIN( 5000, paf->modifier + af->modifier );
         else
            paf->modifier = af->modifier;
         this->affect_remove( af );
         break;
      }
   }
   this->affect_to_char( paf );
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
      bug( "%s: nullptr paf", __func__ );
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
            snprintf( buf, MSL, "&wCasts spell: &B'%s'&w\r\n", IS_VALID_SN( paf->modifier ) ? skill_table[paf->modifier]->name : "unknown" );
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
      this->print( buf );
   }
}

/* Auto-calc formula to set numattacks for PCs & mobs - Samson 5-6-99 */
void char_data::set_numattacks(  )
{
   /*
    * Attack formulas modified to use Shard formulas - Samson 5-3-99 
    */
   this->numattacks = 1.0;

   switch ( this->Class )
   {
      case CLASS_MAGE:
      case CLASS_NECROMANCER:
         /*
          * 2.72 attacks at level 100 
          */
         this->numattacks += ( float )( UMIN( 86, this->level ) * .02 );
         break;

      case CLASS_CLERIC:
      case CLASS_DRUID:
         /*
          * 3.5 attacks at level 100 
          */
         this->numattacks += ( float )( this->level / 40.0 );
         break;

      case CLASS_WARRIOR:
      case CLASS_PALADIN:
      case CLASS_RANGER:
      case CLASS_ANTIPALADIN:
         /*
          * 5.3 attacks at level 100 
          */
         this->numattacks += ( float )( UMIN( 86, this->level ) * .05 );
         break;

      case CLASS_ROGUE:
      case CLASS_BARD:
         /*
          * 4 attacks at level 100 
          */
         this->numattacks += ( float )( this->level / 33.3 );
         break;

      case CLASS_MONK:
         /*
          * 7.6 attacks at level 100 
          */
         this->numattacks += ( float )( this->level / 15.0 );
         break;

      default:
         break;
   }
}

int char_data::char_ego(  )
{
   int p_ego, tmp;

   if( !this->isnpc(  ) )
   {
      p_ego = this->level;
      tmp = ( ( this->get_curr_int(  ) + this->get_curr_wis(  ) + this->get_curr_cha(  ) + this->get_curr_lck(  ) ) / 4 );
      tmp = tmp - 17;
      p_ego += tmp;
   }
   else
      p_ego = 10000;
   if( p_ego <= 0 )
      p_ego = this->level;
   return p_ego;
}

timer_data *char_data::get_timerptr( short type )
{
   list < timer_data * >::iterator cht;

   for( cht = this->timers.begin(  ); cht != this->timers.end(  ); ++cht )
   {
      timer_data *ct = *cht;

      if( ct->type == type )
         return ct;
   }
   return nullptr;
}

short char_data::get_timer( short type )
{
   timer_data *chtimer;

   if( ( chtimer = this->get_timerptr( type ) ) != nullptr )
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
   timer_data *chtimer = nullptr;

   if( !( chtimer = get_timerptr( type ) ) )
   {
      chtimer = new timer_data;
      chtimer->count = count;
      chtimer->type = type;
      chtimer->do_fun = fun;
      chtimer->value = value;
      this->timers.push_back( chtimer );
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
   this->timers.remove( chtimer );
   deleteptr( chtimer );
}

void char_data::remove_timer( short type )
{
   timer_data *chtimer = nullptr;

   if( !( chtimer = this->get_timerptr( type ) ) )
      return;
   extract_timer( chtimer );
}

bool char_data::in_hard_range( area_data * tarea )
{
   if( this->is_immortal(  ) )
      return true;
   else if( this->isnpc(  ) )
      return true;
   else if( this->level >= tarea->low_hard_range && this->level <= tarea->hi_hard_range )
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
   int con = this->get_curr_con(  );

   if( this->is_immortal(  ) )
   {
      this->mental_state = 0;
      return;
   }

   c += number_percent(  ) < con ? 1 : 0;

   if( this->mental_state < 0 )
      this->mental_state = URANGE( -100, this->mental_state + c, 0 );
   else if( this->mental_state > 0 )
      this->mental_state = URANGE( 0, this->mental_state - c, 100 );
}

/*
 * Deteriorate mental state					-Thoric
 */
void char_data::worsen_mental_state( int mod )
{
   int c = URANGE( 0, abs( mod ), 20 );
   int con = this->get_curr_con(  );

   if( this->is_immortal(  ) )
   {
      mental_state = 0;
      return;
   }

   c -= number_percent(  ) < con ? 1 : 0;
   if( c < 1 )
      return;

   if( this->mental_state < 0 )
      this->mental_state = URANGE( -100, this->mental_state - c, 100 );
   else if( this->mental_state > 0 )
      this->mental_state = URANGE( -100, this->mental_state + c, 100 );
   else
      this->mental_state -= c;
}

/*
 * Move a char out of a room.
 */
void char_data::from_room(  )
{
   obj_data *obj;
   list < affect_data * >::iterator paf;

   if( !this->in_room )
   {
      bug( "%s: %s not in a room!", __func__, this->name );
      return;
   }

   if( !this->isnpc(  ) )
      --this->in_room->area->nplayer;

   if( ( obj = this->get_eq( WEAR_LIGHT ) ) != nullptr && obj->item_type == ITEM_LIGHT
       && ( obj->value[2] != 0 || IS_SET( obj->value[3], PIPE_LIT ) ) && this->in_room->light > 0 )
      --this->in_room->light;

   // Take some weight off the room.
   this->in_room->weight -= ( this->weight + this->carry_weight );

   /*
    * Character's affect on the room
    */
   affect_data *af;
   for( paf = this->affects.begin(  ); paf != this->affects.end(  ); ++paf )
   {
      af = *paf;

      this->in_room->room_affect( af, false );
   }

   /*
    * Room's affect on the character
    */
   if( !this->char_died(  ) )
   {
      for( paf = this->in_room->affects.begin(  ); paf != this->in_room->affects.end(  ); ++paf )
      {
         af = *paf;

         this->affect_modify( af, false );
      }

      if( this->char_died(  ) )  /* could die from removespell, etc */
         return;
   }

   this->in_room->people.remove( this );

   this->was_in_room = this->in_room;
   this->in_room = nullptr;

   if( !this->isnpc(  ) && this->get_timer( TIMER_SHOVEDRAG ) > 0 )
      this->remove_timer( TIMER_SHOVEDRAG );
}

/*
 * Move a char into a room.
 */
bool char_data::to_room( room_index * pRoomIndex )
{
   obj_data *obj;
   list < affect_data * >::iterator paf;

   // Ok, asshole code, lets see you get past this!
   if( !pRoomIndex || !get_room_index( pRoomIndex->vnum ) )
   {
      bug( "%s: %s -> nullptr room!  Putting char in limbo (%d)", __func__, this->name, ROOM_VNUM_LIMBO );
      if( pRoomIndex )
         log_printf( "Supposedly from Vnum: %d", pRoomIndex->vnum );

      /*
       * This used to just return, but there was a problem with crashing
       * and I saw no reason not to just put the char in limbo.  -Narn
       */
      if( !( pRoomIndex = get_room_index( ROOM_VNUM_LIMBO ) ) )
      {
         bug( "FATAL: Limbo room is MISSING! Expect crash! %s:%s, line %d", __FILE__, __func__, __LINE__ );
         return false;
      }
   }

   this->in_room = pRoomIndex;
   if( this->home_vnum < 1 )
      this->home_vnum = this->in_room->vnum;

   /*
    * Yep - you guessed it. Everything that needs to happen to add Zone X to Player Y's list
    * * takes place in this one teeny weeny little function found in afk.c :) 
    */
   this->update_visits( pRoomIndex );

   pRoomIndex->people.push_back( this );

   if( !this->isnpc(  ) )
      ++this->in_room->area->nplayer;

   if( ( obj = this->get_eq( WEAR_LIGHT ) ) != nullptr 
      && obj->item_type == ITEM_LIGHT && ( obj->value[2] != 0 || IS_SET( obj->value[3], PIPE_LIT ) ) )
      ++pRoomIndex->light;

   // Put some weight on the room.
   pRoomIndex->weight += ( this->weight + this->carry_weight );

   /*
    * Room's affect on the character
    */
   affect_data *af;
   if( !this->char_died(  ) )
   {
      for( paf = pRoomIndex->affects.begin(  ); paf != pRoomIndex->affects.end(  ); ++paf )
      {
         af = *paf;
         this->affect_modify( af, true );
      }

      if( this->char_died(  ) )  /* could die from a wearspell, etc */
         return true;
   }

   /*
    * Character's effect on the room
    */
   for( paf = affects.begin(  ); paf != affects.end(  ); ++paf )
   {
      af = *paf;
      pRoomIndex->room_affect( af, true );
   }

   if( !isnpc(  ) && pRoomIndex->flags.test( ROOM_SAFE ) && get_timer( TIMER_SHOVEDRAG ) <= 0 )
      this->add_timer( TIMER_SHOVEDRAG, 10, nullptr, 0 );  /*-30 Seconds-*/

   /*
    * Delayed Teleport rooms - Thoric
    * Should be the last thing checked in this function
    */
   if( pRoomIndex->flags.test( ROOM_TELEPORT ) && pRoomIndex->tele_delay > 0 )
   {
      teleport_data *teleport;
      list < teleport_data * >::iterator tele;

      for( tele = teleportlist.begin(  ); tele != teleportlist.end(  ); ++tele )
      {
         teleport = *tele;

         if( teleport->room == pRoomIndex )
            return true;
      }
      teleport = new teleport_data;
      teleport->room = pRoomIndex;
      teleport->timer = pRoomIndex->tele_delay;
      teleportlist.push_back( teleport );
   }
   if( !this->was_in_room )
      this->was_in_room = this->in_room;

   if( this->on )
   {
      this->on = nullptr;
      this->position = POS_STANDING;
   }

   if( this->position != POS_STANDING && this->tempnum != -9998 ) /* Hackish hotboot fix! WOO! */
      this->position = POS_STANDING;
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

   if( this->pcdata && this->pcdata->IS_CLANNED() )
      clan_factor = 1 + abs( this->alignment - this->pcdata->clan->alignment ) / 1000;
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
      deity_factor = this->pcdata->favor / -500;
   else
      deity_factor = 0;

   ms = 10 - abs( this->mental_state );

   if( ( number_percent(  ) - this->get_curr_lck(  ) + 13 - ms ) + deity_factor <= percent )
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

   if( IsHumanoid( ch ) || IsPerson( ch ) )
   {
      ch->set_bpart( PART_HEAD );
      ch->set_bpart( PART_ARMS );
      ch->set_bpart( PART_LEGS );
      ch->set_bpart( PART_FEET );
      ch->set_bpart( PART_ANKLES );
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
}

/* Brought over from DOTD code, caclulates such things as the number of
   attacks a PC gets, as well as monk barehand damage and some other
   RIS flags - Samson 4-6-99 
*/
void char_data::ClassSpecificStuff(  )
{
   if( this->isnpc(  ) )
      return;

   if( IsDragon( this ) )
      armor = UMIN( -150, armor );

   this->set_numattacks(  );

   if( this->Class == CLASS_MONK )
   {
      switch ( this->level )
      {
         case 1:
         case 2:
         case 3:
         case 4:
         case 5:
            this->barenumdie = 1;
            this->baresizedie = 3;
            break;
         case 6:
         case 7:
         case 8:
         case 9:
         case 10:
            this->barenumdie = 1;
            this->baresizedie = 4;
            break;
         case 11:
         case 12:
         case 13:
         case 14:
         case 15:
            this->barenumdie = 1;
            this->baresizedie = 6;
            break;
         case 16:
         case 17:
         case 18:
         case 19:
         case 20:
            this->barenumdie = 1;
            this->baresizedie = 8;
            break;
         case 21:
         case 22:
         case 23:
         case 24:
         case 25:
            this->barenumdie = 2;
            this->baresizedie = 3;
            break;
         case 26:
         case 27:
         case 28:
         case 29:
         case 30:
            this->barenumdie = 2;
            this->baresizedie = 4;
            break;
         case 31:
         case 32:
         case 33:
         case 34:
         case 35:
            this->barenumdie = 1;
            this->baresizedie = 10;
            break;
         case 36:
         case 37:
         case 38:
         case 39:
         case 40:
            this->barenumdie = 1;
            this->baresizedie = 12;
            break;
         case 41:
         case 42:
         case 43:
         case 44:
         case 45:
            this->barenumdie = 1;
            this->baresizedie = 15;
            break;
         case 46:
         case 47:
         case 48:
         case 49:
         case 50:
            this->barenumdie = 2;
            this->baresizedie = 5;
            break;
         case 51:
         case 52:
         case 53:
         case 54:
         case 55:
            this->barenumdie = 2;
            this->baresizedie = 6;
            break;
         case 56:
         case 57:
         case 58:
         case 59:
         case 60:
            this->barenumdie = 3;
            this->baresizedie = 5;
            break;
         case 61:
         case 62:
         case 63:
         case 64:
         case 65:
            this->barenumdie = 3;
            this->baresizedie = 6;
            break;
         case 66:
         case 67:
         case 68:
         case 69:
         case 70:
            this->barenumdie = 1;
            this->baresizedie = 20;
            break;
         case 71:
         case 72:
         case 73:
         case 74:
         case 75:
            this->barenumdie = 4;
            this->baresizedie = 5;
            break;
         case 76:
         case 77:
         case 78:
         case 79:
         case 80:
            this->barenumdie = 5;
            this->baresizedie = 4;
            break;
         case 81:
         case 82:
         case 83:
         case 84:
         case 85:
            this->barenumdie = 5;
            this->baresizedie = 5;
            break;
         case 86:
         case 87:
         case 88:
         case 89:
         case 90:
            this->barenumdie = 5;
            this->baresizedie = 6;
            break;
         case 91:
         case 92:
         case 93:
         case 94:
         case 95:
            this->barenumdie = 6;
            this->baresizedie = 6;
            break;
         default:
            this->barenumdie = 7;
            this->baresizedie = 6;
            break;
      }

      if( this->level >= 20 )
         this->set_resist( RIS_HOLD );
      if( this->level >= 36 )
         this->set_resist( RIS_CHARM );
      if( this->level >= 44 )
         this->set_resist( RIS_POISON );
      if( this->level >= 62 )
      {
         this->set_immune( RIS_CHARM );
         this->unset_resist( RIS_CHARM );
      }
      if( this->level >= 70 )
      {
         this->set_immune( RIS_POISON );
         this->unset_resist( RIS_POISON );
      }
      this->armor = 100 - UMIN( 150, this->level * 5 );
      this->max_move = UMAX( this->max_move, ( 150 + ( this->level * 5 ) ) );
   }

   if( this->Class == CLASS_DRUID )
   {
      if( this->level >= 28 )
         this->set_immune( RIS_CHARM );
      if( this->level >= 64 )
         this->set_resist( RIS_POISON );
   }

   if( this->Class == CLASS_NECROMANCER )
   {
      if( this->level >= 20 )
         this->set_resist( RIS_COLD );
      if( this->level >= 40 )
         this->set_resist( RIS_FIRE );
      if( this->level >= 45 )
         this->set_resist( RIS_ENERGY );
      if( this->level >= 85 )
      {
         this->set_immune( RIS_COLD );
         this->unset_resist( RIS_COLD );
      }
      if( this->level >= 90 )
      {
         this->set_immune( RIS_FIRE );
         this->unset_resist( RIS_FIRE );
      }
   }
}

void char_data::set_specfun( void )
{
   this->spec_funname.clear(  );

   switch ( this->Class )
   {
      case CLASS_MAGE:
         this->spec_fun = m_spec_lookup( "spec_cast_mage" );
         this->spec_funname = "spec_cast_mage";
         break;

      case CLASS_CLERIC:
         this->spec_fun = m_spec_lookup( "spec_cast_cleric" );
         this->spec_funname = "spec_cast_cleric";
         break;

      case CLASS_WARRIOR:
         this->spec_fun = m_spec_lookup( "spec_warrior" );
         this->spec_funname = "spec_warrior";
         break;

      case CLASS_ROGUE:
         this->spec_fun = m_spec_lookup( "spec_thief" );
         this->spec_funname = "spec_thief";
         break;

      case CLASS_RANGER:
         this->spec_fun = m_spec_lookup( "spec_ranger" );
         this->spec_funname = "spec_ranger";
         break;

      case CLASS_PALADIN:
         this->spec_fun = m_spec_lookup( "spec_paladin" );
         this->spec_funname = "spec_paladin";
         break;

      case CLASS_DRUID:
         this->spec_fun = m_spec_lookup( "spec_druid" );
         this->spec_funname = "spec_druid";
         break;

      case CLASS_ANTIPALADIN:
         this->spec_fun = m_spec_lookup( "spec_antipaladin" );
         this->spec_funname = "spec_antipaladin";
         break;

      case CLASS_BARD:
         this->spec_fun = m_spec_lookup( "spec_bard" );
         this->spec_funname = "spec_bard";
         break;

      default:
         this->spec_fun = nullptr;
         break;
   }
}

void stop_follower( char_data * ch )
{
   if( !ch->master )
   {
      bug( "%s: %s has null master.", __func__, ch->name );
      return;
   }

   if( ch->has_aflag( AFF_CHARM ) )
   {
      ch->unset_aflag( AFF_CHARM );
      ch->affect_strip( gsn_charm_person );
   }

   if( ch->master->can_see( ch, false ) )
      if( !( !ch->master->isnpc(  ) && ch->is_immortal(  ) && !ch->master->is_immortal(  ) ) )
         act( AT_ACTION, "$n stops following you.", ch, nullptr, ch->master, TO_VICT );
   act( AT_ACTION, "You stop following $N.", ch, nullptr, ch->master, TO_CHAR );

   ch->master = nullptr;
   ch->leader = nullptr;
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
      bug( "%s: non-null master.", __func__ );
      return;
   }

   ch->master = master;
   ch->leader = nullptr;

   if( master->can_see( ch, false ) )
      act( AT_ACTION, "$n now follows you.", ch, nullptr, master, TO_VICT );
   act( AT_ACTION, "You now follow $N.", ch, nullptr, master, TO_CHAR );
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
      mob->set_specfun(  );

      ++ch->pcdata->charmies;
      ch->pets.push_back( mob );
   }
}

void die_follower( char_data * ch )
{
   if( ch->master )
      unbind_follower( ch, ch->master );

   ch->leader = nullptr;

   list < char_data * >::iterator ich;
   for( ich = charlist.begin(  ); ich != charlist.end(  ); ++ich )
   {
      char_data *fch = ( *ich );

      if( fch->master == ch )
         unbind_follower( fch, ch );

      if( fch->leader == ch )
         fch->leader = fch;
   }
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
   list < obj_data * >::iterator iobj;

   for( iobj = ch->carrying.begin(  ); iobj != ch->carrying.end(  ); )
   {
      obj_data *obj = *iobj;
      ++iobj;

      if( obj->wear_loc == WEAR_NONE )
         continue;

      if( !obj->extra_flags.test( ITEM_MUSTMOUNT ) )
         continue;

      if( fell )
      {
         act( AT_ACTION, "As you fall, $p drops to the ground.", ch, obj, nullptr, TO_CHAR );
         act( AT_ACTION, "$n drops $p as $e falls to the ground.", ch, obj, nullptr, TO_ROOM );

         obj->from_char(  );
         obj = obj->to_room( ch->in_room, ch );
         oprog_drop_trigger( ch, obj );   /* mudprogs */
         if( ch->char_died(  ) )
            return;
      }
      else
      {
         act( AT_ACTION, "As you dismount, you remove $p.", ch, obj, nullptr, TO_CHAR );
         act( AT_ACTION, "$n removes $p as $e dismounts.", ch, obj, nullptr, TO_ROOM );
         ch->unequip( obj );
      }
   }
}

/* Automatic corpse retrieval for < 10 characters.
 * Adapted from the Undertaker snippet by Cyrus and Robcon (Rage of Carnage 2).
 */
void retrieve_corpse( char_data * ch, char_data * healer )
{
   char buf[MSL];
   list < obj_data * >::iterator iobj;
   bool found = false;

   /*
    * Avoids the potential for filling the room with hundreds of mob corpses 
    */
   if( ch->isnpc(  ) )
      return;

   snprintf( buf, MSL, "the corpse of %s", ch->name );

   for( iobj = objlist.begin(  ); iobj != objlist.end(  ); ++iobj )
   {
      obj_data *obj = *iobj;
      obj_data *outer_obj;

      if( str_cmp( buf, obj->short_descr ) )
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
         act( AT_MAGIC, "$n closes $s eyes in deep prayer....", healer, nullptr, nullptr, TO_ROOM );
         act( AT_MAGIC, "A moment later $T appears in the room!", healer, nullptr, buf, TO_ROOM );
      }
      else
         act( AT_MAGIC, "From out of nowhere, $T appears in a bright flash!", ch, nullptr, buf, TO_ROOM );

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
}

/*
 * Extract a char from the world.
 */
/* If something has gone awry with your *ch pointers, there's a fairly good chance this thing will trip over it and
 * bring things crashing down around you. Which is why there are so many bug log points. [Samson]
 */
void char_data::extract( bool fPull )
{
   if( !this->in_room )
   {
      bug( "%s: %s in nullptr room. Transferring to Limbo.", __func__, this->name ? this->name : "???" );
      if( !this->to_room( get_room_index( ROOM_VNUM_LIMBO ) ) )
         log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __func__, __LINE__ );
   }

   if( this == supermob && !mud_down )
   {
      bug( "%s: ch == supermob!", __func__ );
      return;
   }

   if( this->char_died(  ) )
   {
      bug( "%s: %s already died!", __func__, this->name );
      /*
       * return; This return is commented out in the hops of allowing the dead mob to be extracted anyway 
       */
   }

   /*
    * shove onto extraction queue 
    */
   queue_extracted_char( this, fPull );

   list < rel_data * >::iterator RQ;
   for( RQ = relationlist.begin(  ); RQ != relationlist.end(  ); )
   {
      rel_data *RQueue = *RQ;
      ++RQ;

      if( fPull && RQueue->Type == relMSET_ON )
      {
         if( this == RQueue->Subject )
            ( ( char_data * ) RQueue->Actor )->pcdata->dest_buf = nullptr;
         else if( this != RQueue->Actor )
            continue;
         relationlist.remove( RQueue );
         deleteptr( RQueue );
      }
   }

   if( fPull && !mud_down )
      die_follower( this );

   if( !mud_down )
      this->stop_fighting( true );

   if( this->mount )
   {
      update_room_reset( this, true );
      this->mount->unset_actflag( ACT_MOUNTED );
      this->mount = nullptr;
      position = POS_STANDING;
   }

   /*
    * check if this NPC was a mount or a pet
    */
   if( this->isnpc(  ) && !mud_down )
   {
      update_room_reset( this, true );
      this->unset_actflag( ACT_MOUNTED );

      list < char_data * >::iterator ich;
      for( ich = charlist.begin(  ); ich != charlist.end(  ); ++ich )
      {
         char_data *wch = *ich;

         if( wch->mount == this )
         {
            wch->mount = nullptr;
            wch->position = POS_SITTING;
            if( wch->in_room == this->in_room )
            {
               act( AT_SOCIAL, "Your faithful mount, $N collapses beneath you...", wch, nullptr, this, TO_CHAR );
               act( AT_SOCIAL, "You hit the ground with a thud.", wch, nullptr, nullptr, TO_CHAR );
               act( AT_PLAIN, "$n falls from $N as $N is slain.", wch, nullptr, this, TO_ROOM );
               check_mount_objs( this, true );  /* Check to see if they have ITEM_MUSTMOUNT stuff */
            }
         }

         if( !wch->isnpc(  ) && !wch->pets.empty(  ) )
         {
            if( this->master == wch )
            {
               unbind_follower( wch, this );
               if( wch->in_room == this->in_room )
                  act( AT_SOCIAL, "You mourn for the loss of $N.", wch, nullptr, this, TO_CHAR );
            }
         }
      }
   }

   /*
    * Bug fix loop to stop PCs from being hunted after death - Samson 8-22-99 
    */
   if( !mud_down )
   {
      list < char_data * >::iterator ich;
      for( ich = charlist.begin(  ); ich != charlist.end(  ); ++ich )
      {
         char_data *wch = *ich;

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

   list < obj_data * >::iterator iobj;
   for( iobj = this->carrying.begin(  ); iobj != this->carrying.end(  ); )
   {
      obj_data *obj = *iobj;
      ++iobj;

      if( obj->ego >= sysdata->minego && this->in_room->flags.test( ROOM_DEATH ) )
         obj->pIndexData->count += obj->count;

      if( !fPull && obj->extra_flags.test( ITEM_PERMANENT ) )
      {
         if( obj->wear_loc != WEAR_NONE )
            this->unequip( obj );
      }
      else
         obj->extract(  );
   }

   room_index *dieroom = this->in_room;   /* Added for checking where to send you at death - Samson */
   this->from_room(  );

   if( !fPull )
   {
      room_index *location = check_room( this, dieroom );   /* added check to see what continent PC is on - Samson 3-29-98 */

      if( !location )
         location = get_room_index( ROOM_VNUM_TEMPLE );

      if( !location )
         location = get_room_index( ROOM_VNUM_LIMBO );

      if( !this->to_room( location ) )
         log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __func__, __LINE__ );

      if( this->has_pcflag( PCFLAG_ONMAP ) )
      {
         this->unset_pcflag( PCFLAG_ONMAP );
         this->unset_pcflag( PCFLAG_MAPEDIT );  /* Just in case they were editing */

         this->mx = -1;
         this->my = -1;
         this->wmap = -1;
      }

      /*
       * Make things a little fancier           -Thoric
       */
      char_data *wch;
      if( ( wch = get_char_room( "healer" ) ) != nullptr )
      {
         act( AT_MAGIC, "$n mutters a few incantations, waves $s hands and points $s finger.", wch, nullptr, nullptr, TO_ROOM );
         act( AT_MAGIC, "$n appears from some strange swirling mists!", this, nullptr, nullptr, TO_ROOM );
         act_printf( AT_MAGIC, wch, nullptr, nullptr, TO_ROOM, "$n says 'Welcome back to the land of the living, %s.'", capitalize( name ) );
         if( this->level < 10 )
         {
            retrieve_corpse( this, wch );
            collect_followers( this, dieroom, location );
         }
      }
      else
      {
         act( AT_MAGIC, "$n appears from some strange swirling mists!", this, nullptr, nullptr, TO_ROOM );
         if( this->level < 10 )
         {
            retrieve_corpse( this, nullptr );
            collect_followers( this, dieroom, location );
         }
      }
      this->position = POS_RESTING;
      return;
   }

   if( this->isnpc(  ) )
   {
      --this->pIndexData->count;
      --nummobsloaded;
   }

   if( this->desc && this->desc->original )
      interpret( this, "return" );

   if( this->switched && this->switched->desc )
      interpret( this->switched, "return" );

   if( !mud_down )
   {
      list < char_data * >::iterator ich;
      for( ich = charlist.begin(  ); ich != charlist.end(  ); ++ich )
      {
         char_data *wch = *ich;

         if( wch->reply == this )
            wch->reply = nullptr;
      }
   }

   if( this->desc )
   {
      if( this->desc->character != this )
         bug( "%s: %s's descriptor points to another char", __func__, this->name );
      else
         close_socket( this->desc, false );
   }

   if( !this->isnpc(  ) && this->level < LEVEL_IMMORTAL )
   {
      --sysdata->playersonline;
      if( sysdata->playersonline < 0 )
      {
         bug( "%s: Player count went negative!", __func__ );
         sysdata->playersonline = 0;
      }
   }
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
}

void start_hating( char_data * ch, char_data * victim )
{
   if( ch->hating )
      stop_hating( ch );

   ch->hating = new hunt_hate_fear;
   ch->hating->name = QUICKLINK( victim->name );
   ch->hating->who = victim;
}

void start_fearing( char_data * ch, char_data * victim )
{
   if( ch->fearing )
      stop_fearing( ch );

   ch->fearing = new hunt_hate_fear;
   ch->fearing->name = QUICKLINK( victim->name );
   ch->fearing->who = victim;
}

void stop_hunting( char_data * ch )
{
   if( ch->hunting )
   {
      STRFREE( ch->hunting->name );
      deleteptr( ch->hunting );
   }
}

void stop_hating( char_data * ch )
{
   if( ch->hating )
   {
      STRFREE( ch->hating->name );
      deleteptr( ch->hating );
   }
}

void stop_fearing( char_data * ch )
{
   if( ch->fearing )
   {
      STRFREE( ch->fearing->name );
      deleteptr( ch->fearing );
   }
}

/*
 * Advancement stuff.
 */
void advance_level( char_data * ch )
{
   char buf[MSL];
   int add_hp, add_mana, add_prac, manamod = 0, manahighdie, manaroll;

   snprintf( buf, MSL, "the %s", title_table[ch->Class][ch->level][ch->sex] );
   ch->set_title( buf );

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

   add_hp = con_app[ch->get_curr_con(  )].hitp + number_range( class_table[ch->Class]->hp_min, class_table[ch->Class]->hp_max );

   manaroll = dice( 1, manahighdie ) + 1; /* Samson 10-10-98 */

   add_mana = ( manaroll * manamod ) / 10;

   add_prac = 4 + wis_app[ch->get_curr_wis(  )].practice;

   add_hp = UMAX( 1, add_hp );

   add_mana = UMAX( 1, add_mana );

   ch->max_hit += add_hp;
   ch->max_mana += add_mana;
   ch->pcdata->practice += add_prac;

   STRFREE( ch->pcdata->rank );
   ch->pcdata->rank = STRALLOC( class_table[ch->Class]->who_name );

   if( ch->level == LEVEL_AVATAR )
   {
      echo_all_printf( ECHOTAR_ALL, "&[immortal]%s has attained the rank of Avatar!", ch->name );
      STRFREE( ch->pcdata->rank );
      ch->pcdata->rank = STRALLOC( "Avatar" );
      interpret( ch, "help M_ADVHERO_" );
   }
   if( ch->level < LEVEL_IMMORTAL )
      ch->printf( "&WYour gain is: %d hp, %d mana, %d prac.\r\n", add_hp, add_mana, add_prac );

   ch->ClassSpecificStuff(  );   /* Brought over from DOTD code - Samson 4-6-99 */
   ch->save(  );
}

void char_data::gain_exp( double gain )
{
   double modgain;

   if( this->isnpc(  ) || this->level >= LEVEL_AVATAR )
      return;

   modgain = gain;

   /*
    * per-race experience multipliers 
    */
   modgain *= ( race_table[this->race]->exp_multiplier / 100.0 );

   /*
    * xp cap to prevent any one event from giving enuf xp to 
    * gain more than one level - FB 
    */
   modgain = umin( modgain, exp_level( this->level + 2 ) - exp_level( this->level + 1 ) );

   this->exp = umax( 0, this->exp + modgain );

   if( NOT_AUTHED( this ) && this->exp >= exp_level( 10 ) ) /* new auth */
   {
      this->print( "You can not ascend to a higher level until your name is authorized.\r\n" );
      this->exp = ( exp_level( 10 ) - 1 );
      return;
   }

   if( this->level < LEVEL_AVATAR && this->exp >= exp_level( level + 1 ) )
   {
      this->print( "&RYou have acheived enough experience to level! Use the levelup command to do so.&w\r\n" );
      if( this->exp >= exp_level( this->level + 2 ) )
      {
         this->exp = exp_level( this->level + 2 ) - 1;
         this->print( "&RYou cannot gain any further experience until you level up.&w\r\n" );
      }
   }
}

const char *char_data::get_class(  )
{
   if( this->isnpc(  ) && this->Class < MAX_NPC_CLASS && this->Class >= 0 )
      return ( npc_class[this->Class] );
   else if( !this->isnpc(  ) && this->Class < MAX_PC_CLASS && this->Class >= 0 )
      return ( class_table[this->Class]->who_name );
   return ( "Unknown" );
}

const char *char_data::get_race(  )
{
   if( this->race < MAX_PC_RACE && this->race >= 0 )
      return ( race_table[this->race]->race_name );
   if( this->race < MAX_NPC_RACE && this->race >= 0 )
      return ( npc_race[this->race] );
   return ( "Unknown" );
}

/* Disabled Limbo transfer - Samson 5-8-99 */
void char_data::stop_idling(  )
{
   if( !this->desc || this->desc->connected != CON_PLAYING || !this->has_pcflag( PCFLAG_IDLING ) )
      return;

   this->timer = 0;

   this->unset_pcflag( PCFLAG_IDLING );
   act( AT_ACTION, "$n returns to normal.", this, nullptr, nullptr, TO_ROOM );
}

void char_data::set_actflag( int value )
{
   try
   {
      this->actflags.set( value );
   }
   catch( exception & e )
   {
      bug( "Flag exception caught: %s", e.what(  ) );
   }
}

void char_data::unset_actflag( int value )
{
   try
   {
      this->actflags.reset( value );
   }
   catch( exception & e )
   {
      bug( "Flag exception caught: %s", e.what(  ) );
   }
}

bool char_data::has_actflag( int value )
{
   return ( this->actflags.test( value ) );
}

void char_data::toggle_actflag( int value )
{
   try
   {
      this->actflags.flip( value );
   }
   catch( exception & e )
   {
      bug( "Flag exception caught: %s", e.what(  ) );
   }
}

bool char_data::has_actflags(  )
{
   if( this->actflags.any(  ) )
      return true;
   return false;
}

bitset < MAX_ACT_FLAG > char_data::get_actflags(  )
{
   return this->actflags;
}

void char_data::set_actflags( bitset < MAX_ACT_FLAG > bits )
{
   try
   {
      this->actflags = bits;
   }
   catch( exception & e )
   {
      bug( "Flag exception caught: %s", e.what(  ) );
   }
}

void char_data::set_file_actflags( FILE * fp )
{
   try
   {
      flag_set( fp, this->actflags, act_flags );
   }
   catch( exception & e )
   {
      bug( "Flag exception caught: %s", e.what(  ) );
   }
}

bool char_data::has_immune( int ris )
{
   return ( this->immune.test( ris ) );
}

void char_data::set_immune( int ris )
{
   try
   {
      this->immune.set( ris );
   }
   catch( exception & e )
   {
      bug( "Flag exception caught: %s", e.what(  ) );
   }
}

void char_data::unset_immune( int ris )
{
   try
   {
      this->immune.reset( ris );
   }
   catch( exception & e )
   {
      bug( "Flag exception caught: %s", e.what(  ) );
   }
}

void char_data::toggle_immune( int ris )
{
   try
   {
      this->immune.flip( ris );
   }
   catch( exception & e )
   {
      bug( "Flag exception caught: %s", e.what(  ) );
   }
}

bool char_data::has_immunes(  )
{
   if( this->immune.any(  ) )
      return true;
   return false;
}

bitset < MAX_RIS_FLAG > char_data::get_immunes(  )
{
   return this->immune;
}

void char_data::set_immunes( bitset < MAX_RIS_FLAG > bits )
{
   try
   {
      this->immune = bits;
   }
   catch( exception & e )
   {
      bug( "Flag exception caught: %s", e.what(  ) );
   }
}

void char_data::set_file_immunes( FILE * fp )
{
   try
   {
      flag_set( fp, this->immune, ris_flags );
   }
   catch( exception & e )
   {
      bug( "Flag exception caught: %s", e.what(  ) );
   }
}

bool char_data::has_noimmune( int ris )
{
   return ( this->no_immune.test( ris ) );
}

void char_data::set_noimmune( int ris )
{
   try
   {
      this->no_immune.set( ris );
   }
   catch( exception & e )
   {
      bug( "Flag exception caught: %s", e.what(  ) );
   }
}

bool char_data::has_noimmunes(  )
{
   if( this->no_immune.any(  ) )
      return true;
   return false;
}

bitset < MAX_RIS_FLAG > char_data::get_noimmunes(  )
{
   return this->no_immune;
}

void char_data::set_noimmunes( bitset < MAX_RIS_FLAG > bits )
{
   try
   {
      this->no_immune = bits;
   }
   catch( exception & e )
   {
      bug( "Flag exception caught: %s", e.what(  ) );
   }
}

void char_data::set_file_noimmunes( FILE * fp )
{
   try
   {
      flag_set( fp, this->no_immune, ris_flags );
   }
   catch( exception & e )
   {
      bug( "Flag exception caught: %s", e.what(  ) );
   }
}

bool char_data::has_resist( int ris )
{
   return ( this->resistant.test( ris ) );
}

void char_data::set_resist( int ris )
{
   try
   {
      this->resistant.set( ris );
   }
   catch( exception & e )
   {
      bug( "Flag exception caught: %s", e.what(  ) );
   }
}

void char_data::unset_resist( int ris )
{
   try
   {
      this->resistant.reset( ris );
   }
   catch( exception & e )
   {
      bug( "Flag exception caught: %s", e.what(  ) );
   }
}

void char_data::toggle_resist( int ris )
{
   try
   {
      this->resistant.flip( ris );
   }
   catch( exception & e )
   {
      bug( "Flag exception caught: %s", e.what(  ) );
   }
}

bool char_data::has_resists(  )
{
   if( this->resistant.any(  ) )
      return true;
   return false;
}

bitset < MAX_RIS_FLAG > char_data::get_resists(  )
{
   return this->resistant;
}

void char_data::set_resists( bitset < MAX_RIS_FLAG > bits )
{
   try
   {
      this->resistant = bits;
   }
   catch( exception & e )
   {
      bug( "Flag exception caught: %s", e.what(  ) );
   }
}

void char_data::set_file_resists( FILE * fp )
{
   try
   {
      flag_set( fp, this->resistant, ris_flags );
   }
   catch( exception & e )
   {
      bug( "Flag exception caught: %s", e.what(  ) );
   }
}

bool char_data::has_noresist( int ris )
{
   return ( this->no_resistant.test( ris ) );
}

void char_data::set_noresist( int ris )
{
   try
   {
      this->no_resistant.set( ris );
   }
   catch( exception & e )
   {
      bug( "Flag exception caught: %s", e.what(  ) );
   }
}

bool char_data::has_noresists(  )
{
   if( this->no_resistant.any(  ) )
      return true;
   return false;
}

bitset < MAX_RIS_FLAG > char_data::get_noresists(  )
{
   return this->no_resistant;
}

void char_data::set_noresists( bitset < MAX_RIS_FLAG > bits )
{
   try
   {
      this->no_resistant = bits;
   }
   catch( exception & e )
   {
      bug( "Flag exception caught: %s", e.what(  ) );
   }
}

void char_data::set_file_noresists( FILE * fp )
{
   try
   {
      flag_set( fp, this->no_resistant, ris_flags );
   }
   catch( exception & e )
   {
      bug( "Flag exception caught: %s", e.what(  ) );
   }
}

bool char_data::has_suscep( int ris )
{
   return ( this->susceptible.test( ris ) );
}

void char_data::set_suscep( int ris )
{
   try
   {
      this->susceptible.set( ris );
   }
   catch( exception & e )
   {
      bug( "Flag exception caught: %s", e.what(  ) );
   }
}

void char_data::unset_suscep( int ris )
{
   try
   {
      this->susceptible.reset( ris );
   }
   catch( exception & e )
   {
      bug( "Flag exception caught: %s", e.what(  ) );
   }
}

void char_data::toggle_suscep( int ris )
{
   try
   {
      this->susceptible.flip( ris );
   }
   catch( exception & e )
   {
      bug( "Flag exception caught: %s", e.what(  ) );
   }
}

bool char_data::has_susceps(  )
{
   if( this->susceptible.any(  ) )
      return true;
   return false;
}

bitset < MAX_RIS_FLAG > char_data::get_susceps(  )
{
   return this->susceptible;
}

void char_data::set_susceps( bitset < MAX_RIS_FLAG > bits )
{
   try
   {
      this->susceptible = bits;
   }
   catch( exception & e )
   {
      bug( "Flag exception caught: %s", e.what(  ) );
   }
}

void char_data::set_file_susceps( FILE * fp )
{
   try
   {
      flag_set( fp, this->susceptible, ris_flags );
   }
   catch( exception & e )
   {
      bug( "Flag exception caught: %s", e.what(  ) );
   }
}

bool char_data::has_nosuscep( int ris )
{
   return ( this->no_susceptible.test( ris ) );
}

void char_data::set_nosuscep( int ris )
{
   try
   {
      this->no_susceptible.set( ris );
   }
   catch( exception & e )
   {
      bug( "Flag exception caught: %s", e.what(  ) );
   }
}

bool char_data::has_nosusceps(  )
{
   if( this->no_susceptible.any(  ) )
      return true;
   return false;
}

bitset < MAX_RIS_FLAG > char_data::get_nosusceps(  )
{
   return this->no_susceptible;
}

void char_data::set_nosusceps( bitset < MAX_RIS_FLAG > bits )
{
   try
   {
      this->no_susceptible = bits;
   }
   catch( exception & e )
   {
      bug( "Flag exception caught: %s", e.what(  ) );
   }
}

void char_data::set_file_nosusceps( FILE * fp )
{
   try
   {
      flag_set( fp, this->no_susceptible, ris_flags );
   }
   catch( exception & e )
   {
      bug( "Flag exception caught: %s", e.what(  ) );
   }
}

bool char_data::has_absorb( int ris )
{
   return ( this->absorb.test( ris ) );
}

void char_data::set_absorb( int ris )
{
   try
   {
      this->absorb.set( ris );
   }
   catch( exception & e )
   {
      bug( "Flag exception caught: %s", e.what(  ) );
   }
}

void char_data::unset_absorb( int ris )
{
   try
   {
      this->absorb.reset( ris );
   }
   catch( exception & e )
   {
      bug( "Flag exception caught: %s", e.what(  ) );
   }
}

void char_data::toggle_absorb( int ris )
{
   try
   {
      this->absorb.flip( ris );
   }
   catch( exception & e )
   {
      bug( "Flag exception caught: %s", e.what(  ) );
   }
}

bool char_data::has_absorbs(  )
{
   if( this->absorb.any(  ) )
      return true;
   return false;
}

bitset < MAX_RIS_FLAG > char_data::get_absorbs(  )
{
   return this->absorb;
}

void char_data::set_absorbs( bitset < MAX_RIS_FLAG > bits )
{
   try
   {
      this->absorb = bits;
   }
   catch( exception & e )
   {
      bug( "Flag exception caught: %s", e.what(  ) );
   }
}

void char_data::set_file_absorbs( FILE * fp )
{
   try
   {
      flag_set( fp, this->absorb, ris_flags );
   }
   catch( exception & e )
   {
      bug( "Flag exception caught: %s", e.what(  ) );
   }
}

bool char_data::has_attack( int attack )
{
   return ( this->attacks.test( attack ) );
}

void char_data::set_attack( int attack )
{
   try
   {
      this->attacks.set( attack );
   }
   catch( exception & e )
   {
      bug( "Flag exception caught: %s", e.what(  ) );
   }
}

void char_data::unset_attack( int attack )
{
   try
   {
      this->attacks.reset( attack );
   }
   catch( exception & e )
   {
      bug( "Flag exception caught: %s", e.what(  ) );
   }
}

void char_data::toggle_attack( int attack )
{
   try
   {
      this->attacks.flip( attack );
   }
   catch( exception & e )
   {
      bug( "Flag exception caught: %s", e.what(  ) );
   }
}

bool char_data::has_attacks(  )
{
   if( this->attacks.any(  ) )
      return true;
   return false;
}

bitset < MAX_ATTACK_TYPE > char_data::get_attacks(  )
{
   return this->attacks;
}

void char_data::set_attacks( bitset < MAX_ATTACK_TYPE > bits )
{
   try
   {
      this->attacks = bits;
   }
   catch( exception & e )
   {
      bug( "Flag exception caught: %s", e.what(  ) );
   }
}

void char_data::set_file_attacks( FILE * fp )
{
   try
   {
      flag_set( fp, this->attacks, attack_flags );
   }
   catch( exception & e )
   {
      bug( "Flag exception caught: %s", e.what(  ) );
   }
}

bool char_data::has_defense( int defense )
{
   return ( this->defenses.test( defense ) );
}

void char_data::set_defense( int defense )
{
   try
   {
      this->defenses.set( defense );
   }
   catch( exception & e )
   {
      bug( "Flag exception caught: %s", e.what(  ) );
   }
}

void char_data::unset_defense( int defense )
{
   try
   {
      this->defenses.reset( defense );
   }
   catch( exception & e )
   {
      bug( "Flag exception caught: %s", e.what(  ) );
   }
}

void char_data::toggle_defense( int defense )
{
   try
   {
      this->defenses.flip( defense );
   }
   catch( exception & e )
   {
      bug( "Flag exception caught: %s", e.what(  ) );
   }
}

bool char_data::has_defenses(  )
{
   if( this->defenses.any(  ) )
      return true;
   return false;
}

bitset < MAX_DEFENSE_TYPE > char_data::get_defenses(  )
{
   return this->defenses;
}

void char_data::set_defenses( bitset < MAX_DEFENSE_TYPE > bits )
{
   try
   {
      this->defenses = bits;
   }
   catch( exception & e )
   {
      bug( "Flag exception caught: %s", e.what(  ) );
   }
}

void char_data::set_file_defenses( FILE * fp )
{
   try
   {
      flag_set( fp, this->defenses, defense_flags );
   }
   catch( exception & e )
   {
      bug( "Flag exception caught: %s", e.what(  ) );
   }
}

bool char_data::has_aflag( int sn )
{
   return ( this->affected_by.test( sn ) );
}

void char_data::set_aflag( int sn )
{
   try
   {
      this->affected_by.set( sn );
   }
   catch( exception & e )
   {
      bug( "Flag exception caught: %s", e.what(  ) );
   }
}

void char_data::unset_aflag( int sn )
{
   try
   {
      this->affected_by.reset( sn );
   }
   catch( exception & e )
   {
      bug( "Flag exception caught: %s", e.what(  ) );
   }
}

void char_data::toggle_aflag( int sn )
{
   try
   {
      this->affected_by.flip( sn );
   }
   catch( exception & e )
   {
      bug( "Flag exception caught: %s", e.what(  ) );
   }
}

bool char_data::has_aflags(  )
{
   if( this->affected_by.any(  ) )
      return true;
   return false;
}

bitset < MAX_AFFECTED_BY > char_data::get_aflags(  )
{
   return this->affected_by;
}

void char_data::set_aflags( bitset < MAX_AFFECTED_BY > bits )
{
   try
   {
      this->affected_by = bits;
   }
   catch( exception & e )
   {
      bug( "Flag exception caught: %s", e.what(  ) );
   }
}

void char_data::set_file_aflags( FILE * fp )
{
   try
   {
      flag_set( fp, this->affected_by, aff_flags );
   }
   catch( exception & e )
   {
      bug( "Flag exception caught: %s", e.what(  ) );
   }
}

bool char_data::has_noaflag( int sn )
{
   return ( no_affected_by.test( sn ) );
}

void char_data::set_noaflag( int sn )
{
   try
   {
      no_affected_by.set( sn );
   }
   catch( exception & e )
   {
      bug( "Flag exception caught: %s", e.what(  ) );
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
      bug( "Flag exception caught: %s", e.what(  ) );
   }
}

void char_data::toggle_noaflag( int sn )
{
   try
   {
      this->no_affected_by.flip( sn );
   }
   catch( exception & e )
   {
      bug( "Flag exception caught: %s", e.what(  ) );
   }
}

bool char_data::has_noaflags(  )
{
   if( this->no_affected_by.any(  ) )
      return true;
   return false;
}

bitset < MAX_AFFECTED_BY > char_data::get_noaflags(  )
{
   return this->no_affected_by;
}

void char_data::set_noaflags( bitset < MAX_AFFECTED_BY > bits )
{
   try
   {
      this->no_affected_by = bits;
   }
   catch( exception & e )
   {
      bug( "Flag exception caught: %s", e.what(  ) );
   }
}

void char_data::set_file_noaflags( FILE * fp )
{
   try
   {
      flag_set( fp, this->no_affected_by, aff_flags );
   }
   catch( exception & e )
   {
      bug( "Flag exception caught: %s", e.what(  ) );
   }
}

bool char_data::has_bpart( int part )
{
   return ( this->body_parts.test( part ) );
}

void char_data::set_bpart( int part )
{
   try
   {
      this->body_parts.set( part );
   }
   catch( exception & e )
   {
      bug( "Flag exception caught: %s", e.what(  ) );
   }
   if( this->has_bparts() )
      this->set_bodypart_where_names();
}

void char_data::unset_bpart( int part )
{
   try
   {
      this->body_parts.reset( part );
   }
   catch( exception & e )
   {
      bug( "Flag exception caught: %s", e.what(  ) );
   }
   if( this->has_bparts() )
      this->set_bodypart_where_names();
}

void char_data::toggle_bpart( int part )
{
   try
   {
      this->body_parts.flip( part );
   }
   catch( exception & e )
   {
      bug( "Flag exception caught: %s", e.what(  ) );
   }
   if( this->has_bparts() )
      this->set_bodypart_where_names();
}

bool char_data::has_bparts(  )
{
   if( this->body_parts.any(  ) )
      return true;
   return false;
}

bitset < MAX_BPART > char_data::get_bparts(  )
{
   return this->body_parts;
}

void char_data::set_bparts( bitset < MAX_BPART > bits )
{
   try
   {
      this->body_parts = bits;
   }
   catch( exception & e )
   {
      bug( "Flag exception caught: %s", e.what(  ) );
   }
   if( this->has_bparts() )
      this->set_bodypart_where_names();
}

void char_data::set_file_bparts( FILE * fp )
{
   try
   {
      flag_set( fp, this->body_parts, part_flags );
   }
   catch( exception & e )
   {
      bug( "Flag exception caught: %s", e.what(  ) );
   }
   if( this->has_bparts() )
      this->set_bodypart_where_names();
}

void char_data::set_bodypart_where_names( )
{
   this->bodypart_where_names.clear();

   // The order of these goes by the traditional listing as found in act_info.cpp
   if( this->body_parts.test( PART_HANDS ) )
      this->bodypart_where_names.push_back( "<used as light>     " );
   else
      this->bodypart_where_names.push_back( "<BODY PART SLOT ERROR>" );

   if( this->body_parts.test( PART_FINGERS ) )
   {
      this->bodypart_where_names.push_back( "<worn on finger>    " );
      this->bodypart_where_names.push_back( "<worn on finger>    " );
   }
   else
   {
      this->bodypart_where_names.push_back( "<BODY PART SLOT ERROR>" );
      this->bodypart_where_names.push_back( "<BODY PART SLOT ERROR>" );
   }

   this->bodypart_where_names.push_back( "<worn around neck>  " );
   this->bodypart_where_names.push_back( "<worn around neck>  " );
   this->bodypart_where_names.push_back( "<worn on body>      " );
   this->bodypart_where_names.push_back( "<worn on head>      " );

   if( this->body_parts.test( PART_LEGS ) )
      this->bodypart_where_names.push_back( "<worn on legs>      " );
   else
      this->bodypart_where_names.push_back( "<BODY PART SLOT ERROR>" );

   if( this->body_parts.test( PART_FEET ) )
      this->bodypart_where_names.push_back( "<worn on feet>      " );
   else
      this->bodypart_where_names.push_back( "<BODY PART SLOT ERROR>" );

   if( this->body_parts.test( PART_HANDS ) )
      this->bodypart_where_names.push_back( "<worn on hands>     " );
   else
      this->bodypart_where_names.push_back( "<BODY PART SLOT ERROR>" );

   if( this->body_parts.test( PART_ARMS ) )
      this->bodypart_where_names.push_back( "<worn on arms>      " );
   else
      this->bodypart_where_names.push_back( "<BODY PART SLOT ERROR>" );

   // Shields could potentially be on arms, legs, whatever. Don't limit to just hands.
   this->bodypart_where_names.push_back( "<worn as shield>    " );

   this->bodypart_where_names.push_back( "<worn about body>   " );
   this->bodypart_where_names.push_back( "<worn about waist>  " );

   if( this->body_parts.test( PART_HANDS ) )
   {
      this->bodypart_where_names.push_back( "<worn around wrist> " );
      this->bodypart_where_names.push_back( "<worn around wrist> " );
      this->bodypart_where_names.push_back( "<wielded>           " );
      this->bodypart_where_names.push_back( "<held>              " );
      this->bodypart_where_names.push_back( "<dual wielded>      " );
   }
   else
   {
      this->bodypart_where_names.push_back( "<BODY PART SLOT ERROR>" );
      this->bodypart_where_names.push_back( "<BODY PART SLOT ERROR>" );
      this->bodypart_where_names.push_back( "<BODY PART SLOT ERROR>" );
      this->bodypart_where_names.push_back( "<BODY PART SLOT ERROR>" );
      this->bodypart_where_names.push_back( "<BODY PART SLOT ERROR>" );
   }

   if( this->body_parts.test( PART_EAR ) )
      this->bodypart_where_names.push_back( "<worn on ears>      " );
   else
      this->bodypart_where_names.push_back( "<BODY PART SLOT ERROR>" );

   if( this->body_parts.test( PART_EYE ) )
      this->bodypart_where_names.push_back( "<worn over eyes>    " );
   else
      this->bodypart_where_names.push_back( "<BODY PART SLOT ERROR>" );

   if( this->body_parts.test( PART_HANDS ) )
      this->bodypart_where_names.push_back( "<missile wielded>   " );
   else
      this->bodypart_where_names.push_back( "<BODY PART SLOT ERROR>" );

   this->bodypart_where_names.push_back( "<worn on back>      " );
   this->bodypart_where_names.push_back( "<worn on face>      " );

   if( this->body_parts.test( PART_ANKLES ) )
   {
      this->bodypart_where_names.push_back( "<worn on ankle>     " );
      this->bodypart_where_names.push_back( "<worn on ankle>     " );
   }
   else
   {
      this->bodypart_where_names.push_back( "<BODY PART SLOT ERROR>" );
      this->bodypart_where_names.push_back( "<BODY PART SLOT ERROR>" );
   }

   if( this->body_parts.test( PART_HOOVES ) )
      this->bodypart_where_names.push_back( "<worn on hooves>    " );
   else
      this->bodypart_where_names.push_back( "<BODY PART SLOT ERROR>" );

   if( this->body_parts.test( PART_TAIL ) || this->body_parts.test( PART_TAILATTACK ) )
      this->bodypart_where_names.push_back( "<worn on tail>      " );
   else
      this->bodypart_where_names.push_back( "<BODY PART SLOT ERROR>" );

   this->bodypart_where_names.push_back( "<lodged in a rib>   " );

   if( this->body_parts.test( PART_ARMS ) )
      this->bodypart_where_names.push_back( "<lodged in an arm>  " );
   else
      this->bodypart_where_names.push_back( "<BODY PART SLOT ERROR>" );

   if( this->body_parts.test( PART_LEGS ) )
      this->bodypart_where_names.push_back( "<lodged in a leg>   " );
   else
      this->bodypart_where_names.push_back( "<BODY PART SLOT ERROR>" );
}

bool char_data::has_pcflag( int bit )
{
   return ( !this->isnpc(  ) && this->pcdata->flags.test( bit ) );
}

void char_data::set_pcflag( int bit )
{
   if( this->isnpc(  ) )
      bug( "%s: Setting PC flag on NPC!", __func__ );
   else
   {
      try
      {
         this->pcdata->flags.set( bit );
      }
      catch( exception & e )
      {
         bug( "Flag exception caught: %s", e.what(  ) );
      }
   }
}

void char_data::unset_pcflag( int bit )
{
   if( this->isnpc(  ) )
      bug( "%s: Removing PC flag on NPC!", __func__ );
   else
   {
      try
      {
         this->pcdata->flags.reset( bit );
      }
      catch( exception & e )
      {
         bug( "Flag exception caught: %s", e.what(  ) );
      }
   }
}

void char_data::toggle_pcflag( int bit )
{
   if( this->isnpc(  ) )
      bug( "%s: Toggling PC flag on NPC!", __func__ );
   else
   {
      try
      {
         this->pcdata->flags.flip( bit );
      }
      catch( exception & e )
      {
         bug( "Flag exception caught: %s", e.what(  ) );
      }
   }
}

bool char_data::has_pcflags(  )
{
   if( this->isnpc(  ) )
   {
      bug( "%s: Checking PC flags on NPC!", __func__ );
      return false;
   }
   else
   {
      if( this->pcdata->flags.any(  ) )
         return true;
      return false;
   }
}

bitset < MAX_PCFLAG > char_data::get_pcflags(  )
{
   if( this->isnpc(  ) )
   {
      bug( "%s: Retreving PC flags on NPC!", __func__ );
      return 0;
   }
   else
      return this->pcdata->flags;
}

void char_data::set_file_pcflags( FILE * fp )
{
   if( this->isnpc(  ) )
   {
      bug( "%s: Setting PC flags on NPC from FILE!", __func__ );
      return;
   }

   try
   {
      flag_set( fp, this->pcdata->flags, pc_flags );
   }
   catch( exception & e )
   {
      bug( "Flag exception caught: %s", e.what(  ) );
   }
}

bool char_data::has_lang( int lang )
{
   return ( this->speaks.test( lang ) );
}

void char_data::set_lang( int lang )
{
   if( lang == -1 )
      this->speaks.set(  );
   else
   {
      try
      {
         this->speaks.set( lang );
      }
      catch( exception & e )
      {
         bug( "Flag exception caught: %s", e.what(  ) );
      }
   }
}

void char_data::unset_lang( int lang )
{
   try
   {
      this->speaks.reset( lang );
   }
   catch( exception & e )
   {
      bug( "Flag exception caught: %s", e.what(  ) );
   }
}

void char_data::toggle_lang( int lang )
{
   try
   {
      this->speaks.flip( lang );
   }
   catch( exception & e )
   {
      bug( "Flag exception caught: %s", e.what(  ) );
   }
}

bool char_data::has_langs(  )
{
   if( speaks.any(  ) )
      return true;
   return false;
}

bitset < LANG_UNKNOWN > char_data::get_langs(  )
{
   return speaks;
}

void char_data::set_langs( bitset < LANG_UNKNOWN > bits )
{
   try
   {
      speaks = bits;
   }
   catch( exception & e )
   {
      bug( "Flag exception caught: %s", e.what(  ) );
   }
}

void char_data::set_file_langs( FILE * fp )
{
   try
   {
      flag_set( fp, speaks, lang_names );
   }
   catch( exception & e )
   {
      bug( "Flag exception caught: %s", e.what(  ) );
   }
}

time_t char_data::time_played( )
{
   if( this->isnpc() )
      return -1;

   return ( ( this->pcdata->played + ( current_time - this->pcdata->logon ) ) / 3600 );
}

CMDF( do_dismiss )
{
   char_data *victim;

   if( argument.empty(  ) )
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
      act( AT_ACTION, "$n dismisses $N.", ch, nullptr, victim, TO_NOTVICT );
      act( AT_ACTION, "You dismiss $N.", ch, nullptr, victim, TO_CHAR );
   }
   else
      ch->print( "You cannot dismiss them.\r\n" );
}

CMDF( do_follow )
{
   char_data *victim;

   if( argument.empty(  ) )
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
      act( AT_PLAIN, "But you'd rather follow $N!", ch, nullptr, ch->master, TO_CHAR );
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

   /*
    * Check to see if the victim a player is trying to follow is ignoring them.
    * If so, they aren't permitted to follow. -Leart
    */
   if( is_ignoring( victim, ch ) )
   {
      /* If the person trying to follow is an imm, then they can still follow */
      if( !ch->is_immortal() || victim->get_trust( ) > ch->get_trust( ) )
      {
         ch->printf( "&[ignore]%s is ignoring you.\r\n", victim->name );
         return;
      }
   }

   if( ch->master )
      stop_follower( ch );

   add_follower( ch, victim );
}

CMDF( do_order )
{
   string arg, argbuf;

   argbuf = argument;
   argument = one_argument( argument, arg );

   if( arg.empty(  ) || argument.empty(  ) )
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
      victim = nullptr;
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
   list < char_data * >::iterator ich;
   for( ich = ch->in_room->people.begin(  ); ich != ch->in_room->people.end(  ); )
   {
      char_data *och = *ich;
      ++ich;

      if( och->has_aflag( AFF_CHARM ) && och->master == ch && ( fAll || och == victim ) && !och->is_immortal() )
      {
         found = true;
         act( AT_ACTION, "$n orders you to '$t'.", ch, argument.c_str(  ), och, TO_VICT );
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
      ch->print( "&YYou can now be affected by hunger and thirst.\r\nIt is advisable for you to purchase food and water soon.\r\n" );

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
}
