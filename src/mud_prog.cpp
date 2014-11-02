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
 *  The MUDprograms are heavily based on the original MOBprogram code that  *
 *  was written by N'Atas-ha.                                               *
 *  Much has been added, including the capability to put a "program" on     *
 *  rooms and objects, not to mention many more triggers and ifchecks, as   *
 *  well as "script" support.                                               *
 *                                                                          *
 *  Error reporting has been changed to specify whether the offending       *
 *  program is on a mob, a room or and object, along with the vnum.         *
 *                                                                          *
 *  Mudprog parsing has been rewritten (in mprog_driver). Mprog_process_if  *
 *  and mprog_process_cmnd have been removed, mprog_do_command is new.      *
 *  Full support for nested ifs is in.                                      *
 ****************************************************************************/

#include <cstdarg>
#include "mud.h"
#include "area.h"
#include "bits.h"
#include "clans.h"
#include "deity.h"
#include "descriptor.h"
#include "mobindex.h"
#include "mud_prog.h"
#include "objindex.h"
#include "polymorph.h"
#include "roomindex.h"
#include "variables.h"

bool MOBtrigger;

/* Defines by Narn for new mudprog parsing, used as 
   return values from mprog_do_command. */
const int COMMANDOK = 1;
const int IFTRUE = 2;
const int IFFALSE = 3;
const int ORTRUE = 4;
const int ORFALSE = 5;
const int FOUNDELSE = 6;
const int FOUNDENDIF = 7;
const int IFIGNORED = 8;
const int ORIGNORED = 9;

/*
 * Local function prototypes
 */
void set_supermob( obj_data * );
int mprog_do_command( char *, char_data *, char_data *, obj_data *, char_data *, obj_data *, char_data *, bool, bool );

/*
 *  Mudprogram additions
 */
char_data *supermob;
obj_data *supermob_obj;
list < room_index * >room_act_list;
list < obj_data * >obj_act_list;
list < char_data * >mob_act_list;

// This is only being used by the is_wearing ifcheck
const char *item_w_flags[] = {
   "take", "finger", "finger", "neck", "neck", "body", "head", "legs", "feet",
   "hands", "arms", "shield", "about", "waist", "wrist", "wrist", "wield",
   "hold", "dual", "ears", "eyes", "missile", "back", "face", "ankle", "ankle",
   "lodge_rib", "lodge_arm", "lodge_leg"
};

mud_prog_data::mud_prog_data(  )
{
   init_memory( &arglist, &triggered, sizeof( triggered ) );
}

mud_prog_data::~mud_prog_data(  )
{
   STRFREE( arglist );
   STRFREE( comlist );
}

mprog_act_list::mprog_act_list(  )
{
   init_memory( &ch, &target, sizeof( target ) );
}

mprog_act_list::~mprog_act_list(  )
{
}

/* Used to store sleeping mud progs. -rkb */
enum mp_types
{
   MP_MOB, MP_ROOM, MP_OBJ
};

struct mpsleep_data
{
   mpsleep_data(  );
   ~mpsleep_data(  );

   char_data *mob;
   char_data *actor;
   obj_data *obj;
   room_index *room; /* Room when type is MP_ROOM */
   char_data *victim;
   obj_data *target;
   char *com_list;
   mp_types type; /* Mob, Room or Obj prog */
   int timer;  /* Pulses to sleep */
   int ignorelevel;
   int iflevel;
   bool ifstate[MAX_IFS][DO_ELSE + 1];
   bool single_step;
};

void uphold_supermob( int *curr_serial, int serial, room_index ** supermob_room, obj_data * true_supermob_obj )
{
   if( *curr_serial != serial )
   {
      if( supermob->in_room != *supermob_room )
      {
         supermob->from_room(  );
         if( !supermob->to_room( *supermob_room ) )
            log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
      }

      if( true_supermob_obj && true_supermob_obj != supermob_obj )
      {
         supermob_obj = true_supermob_obj;
         STRFREE( supermob->short_descr );
         supermob->short_descr = QUICKLINK( supermob_obj->short_descr );
         stralloc_printf( &supermob->chardesc, "Object #%d", supermob_obj->pIndexData->vnum );
      }
      else
      {
         if( !true_supermob_obj )
            supermob_obj = NULL;
         STRFREE( supermob->short_descr );
         supermob->short_descr = QUICKLINK( ( *supermob_room )->name );
         stralloc_printf( &supermob->chardesc, "Room #%d", ( *supermob_room )->vnum );
      }
      *curr_serial = serial;
   }
   else
      *supermob_room = supermob->in_room;
}

/*
 * Global variables to handle sleeping mud progs.
 * mpsleep snippet - Samson 6-1-99 
 */
mpsleep_data *current_mpsleep = NULL;
list < mpsleep_data * >sleeplist;

mpsleep_data::mpsleep_data(  )
{
   init_memory( &mob, &single_step, sizeof( single_step ) );
}

mpsleep_data::~mpsleep_data(  )
{
   STRFREE( com_list );
   sleeplist.remove( this );
}

/*
 * Recursive function used by the carryingvnum ifcheck.
 * It loops thru all objects belonging to a char (in nested containers)
 * and returns true if it finds a matching vnum.
 * I declared it static to limit its scope to this file.  --Gorog
 *
 * This recursive function works by using the following method for
 * traversing the nodes in a binary tree:
 *
 *    Start at the root node
 *    if there is a child then visit the child
 *       if there is a sibling then visit the sibling
 *    else
 *    if there is a sibling then visit the sibling
 */
static bool carryingvnum_visit( char_data * ch, list < obj_data * >source, int vnum )
{
   list < obj_data * >::iterator iobj;

   for( iobj = source.begin(  ); iobj != source.end(  ); ++iobj )
   {
      obj_data *obj = *iobj;

      if( obj->wear_loc == -1 && obj->pIndexData->vnum == vnum )
         return true;

      if( !obj->contents.empty(  ) )   /* node has a child? */
      {
         if( carryingvnum_visit( ch, obj->contents, vnum ) )
            return true;
      }
   }
   return false;
}

void free_prog_actlists( void )
{
   room_act_list.clear(  );
   obj_act_list.clear(  );
   mob_act_list.clear(  );
}

/* This routine reads in scripts of MUDprograms from a file */
int mprog_name_to_type( const string & name )
{
   if( !str_cmp( name, "in_file_prog" ) )
      return IN_FILE_PROG;
   if( !str_cmp( name, "act_prog" ) )
      return ACT_PROG;
   if( !str_cmp( name, "speech_prog" ) )
      return SPEECH_PROG;
   if( !str_cmp( name, "speech_and_prog" ) )
      return SPEECH_AND_PROG;
   if( !str_cmp( name, "rand_prog" ) )
      return RAND_PROG;
   if( !str_cmp( name, "fight_prog" ) )
      return FIGHT_PROG;
   if( !str_cmp( name, "hitprcnt_prog" ) )
      return HITPRCNT_PROG;
   if( !str_cmp( name, "death_prog" ) )
      return DEATH_PROG;
   if( !str_cmp( name, "entry_prog" ) )
      return ENTRY_PROG;
   if( !str_cmp( name, "greet_prog" ) )
      return GREET_PROG;
   if( !str_cmp( name, "all_greet_prog" ) )
      return ALL_GREET_PROG;
   if( !str_cmp( name, "give_prog" ) )
      return GIVE_PROG;
   if( !str_cmp( name, "bribe_prog" ) )
      return BRIBE_PROG;
   if( !str_cmp( name, "time_prog" ) )
      return TIME_PROG;
   if( !str_cmp( name, "month_prog" ) )
      return MONTH_PROG;
   if( !str_cmp( name, "hour_prog" ) )
      return HOUR_PROG;
   if( !str_cmp( name, "wear_prog" ) )
      return WEAR_PROG;
   if( !str_cmp( name, "remove_prog" ) )
      return REMOVE_PROG;
   if( !str_cmp( name, "sac_prog" ) )
      return SAC_PROG;
   if( !str_cmp( name, "look_prog" ) )
      return LOOK_PROG;
   if( !str_cmp( name, "exa_prog" ) )
      return EXA_PROG;
   if( !str_cmp( name, "zap_prog" ) )
      return ZAP_PROG;
   if( !str_cmp( name, "get_prog" ) )
      return GET_PROG;
   if( !str_cmp( name, "drop_prog" ) )
      return DROP_PROG;
   if( !str_cmp( name, "damage_prog" ) )
      return DAMAGE_PROG;
   if( !str_cmp( name, "repair_prog" ) )
      return REPAIR_PROG;
   if( !str_cmp( name, "greet_prog" ) )
      return GREET_PROG;
   if( !str_cmp( name, "randiw_prog" ) )
      return RANDIW_PROG;
   if( !str_cmp( name, "speechiw_prog" ) )
      return SPEECHIW_PROG;
   if( !str_cmp( name, "pull_prog" ) )
      return PULL_PROG;
   if( !str_cmp( name, "push_prog" ) )
      return PUSH_PROG;
   if( !str_cmp( name, "sleep_prog" ) )
      return SLEEP_PROG;
   if( !str_cmp( name, "rest_prog" ) )
      return REST_PROG;
   if( !str_cmp( name, "rfight_prog" ) )
      return FIGHT_PROG;
   if( !str_cmp( name, "enter_prog" ) )
      return ENTRY_PROG;
   if( !str_cmp( name, "leave_prog" ) )
      return LEAVE_PROG;
   if( !str_cmp( name, "rdeath_prog" ) )
      return DEATH_PROG;
   if( !str_cmp( name, "script_prog" ) )
      return SCRIPT_PROG;
   if( !str_cmp( name, "use_prog" ) )
      return USE_PROG;
   if( !str_cmp( name, "keyword_prog" ) )
      return KEYWORD_PROG;
   if( !str_cmp( name, "sell_prog" ) )
      return SELL_PROG;
   if( !str_cmp( name, "tell_prog" ) )
      return TELL_PROG;
   if( !str_cmp( name, "tell_and_prog" ) )
      return TELL_AND_PROG;
   if( !str_cmp( name, "command_prog" ) )
      return CMD_PROG;
   if( !str_cmp( name, "emote_prog" ) )
      return EMOTE_PROG;
   if( !str_cmp( name, "login_prog" ) )
      return LOGIN_PROG;
   if( !str_cmp( name, "void_prog" ) )
      return VOID_PROG;
   if( !str_cmp( name, "load_prog" ) )
      return LOAD_PROG;
   return ( ERROR_PROG );
}

void init_supermob( void )
{
   room_index *office;

   supermob = get_mob_index( MOB_VNUM_SUPERMOB )->create_mobile(  );
   office = get_room_index( MOB_VNUM_SUPERMOB );
   if( !supermob->to_room( office ) )
      log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
}

/* Used to get sequential lines of a multi line string (separated by "\r\n")
 * Thus its like one_argument(), but a trifle different. It is destructive
 * to the multi line string argument, and thus clist must not be shared.
 */
char *mprog_next_command( char *clist )
{
   char *pointer = clist;

   while( *pointer != '\n' && *pointer != '\0' )
      ++pointer;

   if( *pointer == '\n' )
      *pointer++ = '\0';
   if( *pointer == '\r' )
      *pointer++ = '\0';

   return ( pointer );
}

/* These two functions do the basic evaluation of ifcheck operators.
 *  It is important to note that the string operations are not what
 *  you probably expect.  Equality is exact and division is substring.
 *  remember that lhs has been stripped of leading space, but can
 *  still have trailing spaces so be careful when editing since:
 *  "guard" and "guard " are not equal.
 */
bool mprog_seval( const char *lhs, const char *opr, const char *rhs, char_data * mob )
{
   if( !str_cmp( opr, "==" ) )
      return ( bool ) ( !str_cmp( lhs, rhs ) );
   if( !str_cmp( opr, "!=" ) )
      return ( bool ) ( str_cmp( lhs, rhs ) );
   if( !str_cmp( opr, "/" ) )
      return ( bool ) ( !str_infix( rhs, lhs ) );
   if( !str_cmp( opr, "!/" ) )
      return ( bool ) ( str_infix( rhs, lhs ) );

   progbugf( mob, "%s: Improper MOBprog operator '%s'", __FUNCTION__, opr );
   return 0;
}

bool mprog_veval( int lhs, const char *opr, int rhs, char_data * mob )
{
   if( !str_cmp( opr, "==" ) )
      return ( lhs == rhs );
   if( !str_cmp( opr, "!=" ) )
      return ( lhs != rhs );
   if( !str_cmp( opr, ">" ) )
      return ( lhs > rhs );
   if( !str_cmp( opr, "<" ) )
      return ( lhs < rhs );
   if( !str_cmp( opr, "<=" ) )
      return ( lhs <= rhs );
   if( !str_cmp( opr, ">=" ) )
      return ( lhs >= rhs );
   if( !str_cmp( opr, "&" ) )
      return ( lhs & rhs );
   if( !str_cmp( opr, "|" ) )
      return ( lhs | rhs );

   progbugf( mob, "Improper MOBprog operator '%s'", opr );
   return 0;
}

#define isoperator(c) ((c)=='='||(c)=='<'||(c)=='>'||(c)=='!'||(c)=='&'||(c)=='|')
const int MAX_IF_ARGS = 6;

/* This function performs the evaluation of the if checks.  It is
 * here that you can add any ifchecks which you so desire. Hopefully
 * it is clear from what follows how one would go about adding your
 * own. The syntax for an if check is: ifcheck ( arg ) [opr val]
 * where the parenthesis are required and the opr and val fields are
 * optional but if one is there then both must be. The spaces are all
 * optional. The evaluation of the opr expressions is farmed out
 * to reduce the redundancy of the mammoth if statement list.
 * If there are errors, then return BERR otherwise return boolean 1,0
 * Redone by Altrag.. kill all that big copy-code that performs the
 * same action on each variable..
 */
int mprog_do_ifcheck( char *ifcheck, char_data * mob, char_data * actor, obj_data * obj, char_data * victim, obj_data * target, char_data * rndm )
{
   char buf[MSL];
   char opr[MIL];
   const char *chck, *cvar;
   char *argv[MAX_IF_ARGS];
   const char *rval = "";
   char *q, *p = buf;
   int argc = 0;
   list < char_data * >::iterator ich;
   list < descriptor_data * >::iterator ds;
   char_data *chkchar = NULL;
   obj_data *chkobj = NULL;
   int lhsvl, rhsvl = 0, lang;

   if( !*ifcheck )
   {
      progbug( "Null ifcheck", mob );
      return BERR;
   }

   /*
    * New parsing by Thoric to allow for multiple arguments inside the
    * brackets, ie: if leveldiff($n, $i) > 10
    * It's also smaller, cleaner and probably faster
    */
   strcpy( buf, ifcheck );
   opr[0] = '\0';
   while( isspace( *p ) )
      ++p;
   argv[argc++] = p;
   while( isalnum( *p ) )
      ++p;
   while( isspace( *p ) )
      *p++ = '\0';
   if( *p != '(' )
   {
      progbug( "Ifcheck Syntax error (missing left bracket)", mob );
      return BERR;
   }

   *p++ = '\0';

   /*
    * Need to check for spaces or if name( $n ) isn't legal --Shaddai 
    */
   while( isspace( *p ) )
      *p++ = '\0';
   for( ;; )
   {
      argv[argc++] = p;
      while( *p == '$' || isalnum( *p ) || *p == ':' )
         ++p;
      while( isspace( *p ) )
         *p++ = '\0';
      switch ( *p )
      {
         case ',':
            *p++ = '\0';
            while( isspace( *p ) )
               *p++ = '\0';
            if( argc >= MAX_IF_ARGS )
            {
               while( *p && *p != ')' )
                  ++p;
               if( *p )
                  *p++ = '\0';
               while( isspace( *p ) )
                  *p++ = '\0';
               goto doneargs;
            }
            break;

         case ')':
            *p++ = '\0';
            while( isspace( *p ) )
               *p++ = '\0';
            goto doneargs;

         default:
            progbug( "Ifcheck Syntax warning (missing right bracket)", mob );
            goto doneargs;
      }
   }
 doneargs:
   q = p;
   while( isoperator( *p ) )
      ++p;
   strncpy( opr, q, p - q );
   opr[p - q] = '\0';
   while( isspace( *p ) )
      *p++ = '\0';
   rval = p;
   /*
    * while ( *p && !isspace(*p) ) ++p;
    */
   while( *p )
      ++p;
   *p = '\0';

   chck = argv[0] ? argv[0] : "";
   cvar = argv[1] ? argv[1] : "";

   /*
    * chck contains check, cvar is the variable in the (), opr is the
    * operator if there is one, and rval is the value if there was an
    * operator.
    */
   if( cvar[0] == '$' )
   {
      switch ( cvar[1] )
      {
         case 'i':
            chkchar = mob;
            break;
         case 'n':
            chkchar = actor;
            break;
         case 't':
            chkchar = victim;
            break;
         case 'r':
            chkchar = rndm;
            break;
         case 'o':
            chkobj = obj;
            break;
         case 'p':
            chkobj = target;
            break;
         default:
            progbugf( mob, "Bad argument '%c' to '%s'", cvar[0], chck );
            return BERR;
      }
      if( !chkchar && !chkobj )
         return BERR;
   }

   if( !str_cmp( chck, "rand" ) )
      return ( number_percent(  ) <= atoi( cvar ) );

   if( !str_cmp( chck, "mobinarea" ) )
   {
      int vnum = atoi( cvar ), world_count, found_count;

      if( vnum < 1 || vnum > sysdata->maxvnum )
      {
         progbug( "Bad vnum to 'mobinarea'", mob );
         return BERR;
      }

      mob_index *m_index = get_mob_index( vnum );

      if( !m_index )
         world_count = 0;
      else
         world_count = m_index->count;

      lhsvl = 0;
      found_count = 0;

      for( ich = charlist.begin(  ); ich != charlist.end(  ) && found_count != world_count; ++ich )
      {
         char_data *tmob = *ich;

         if( tmob->isnpc(  ) && tmob->pIndexData->vnum == vnum )
         {
            ++found_count;

            if( tmob->in_room->area == mob->in_room->area )
               ++lhsvl;
         }
      }
      rhsvl = atoi( rval );

      /*
       * Changed below from 1 to 0 
       */
      if( rhsvl < 0 )
         rhsvl = 0;
      if( !*opr )
         strcpy( opr, "==" );

      return mprog_veval( lhsvl, opr, rhsvl, mob );
   }

   if( !str_cmp( chck, "mobinroom" ) )
   {
      int vnum = atoi( cvar );

      if( vnum < 1 || vnum > sysdata->maxvnum )
      {
         progbug( "Bad vnum to 'mobinroom'", mob );
         return BERR;
      }
      lhsvl = 0;
      for( ich = mob->in_room->people.begin(  ); ich != mob->in_room->people.end(  ); ++ich )
      {
         char_data *oMob = *ich;
         if( oMob->isnpc(  ) && oMob->pIndexData->vnum == vnum )
            ++lhsvl;
      }
      rhsvl = atoi( rval );
      /*
       * Changed below from 1 to 0 
       */
      if( rhsvl < 0 )
         rhsvl = 0;
      if( !*opr )
         strcpy( opr, "==" );
      return mprog_veval( lhsvl, opr, rhsvl, mob );
   }

   if( !str_cmp( chck, "mobinworld" ) )
   {
      int vnum = atoi( cvar );

      if( vnum < 1 || vnum > sysdata->maxvnum )
      {
         progbug( "Bad vnum to 'mobinworld'", mob );
         return BERR;
      }

      mob_index *m_index = get_mob_index( vnum );

      if( !m_index )
         lhsvl = 0;
      else
         lhsvl = m_index->count;

      rhsvl = atoi( rval );
      /*
       * Changed below from 1 to 0 
       */
      if( rhsvl < 0 )
         rhsvl = 0;
      if( !*opr )
         strcpy( opr, "==" );

      return mprog_veval( lhsvl, opr, rhsvl, mob );
   }

   if( !str_cmp( chck, "timeskilled" ) )
   {
      mob_index *pMob;

      if( chkchar )
         pMob = chkchar->pIndexData;
      else if( !( pMob = get_mob_index( atoi( cvar ) ) ) )
      {
         progbug( "TimesKilled ifcheck: bad vnum", mob );
         return BERR;
      }
      return mprog_veval( pMob->killed, opr, atoi( rval ), mob );
   }

   // Imported from Smaug 1.8
   if( !str_cmp( chck, "objinworld" ) )
   {
      int vnum = atoi( cvar );
      obj_index *p_index;

      if( vnum < 1 || vnum > sysdata->maxvnum )
      {
         progbug( "Objinworld: Bad vnum", mob );
         return BERR;
      }

      p_index = ( get_obj_index( vnum ) );

      if( !p_index )
         lhsvl = 0;
      else
         lhsvl = p_index->count;

      rhsvl = atoi( rval );

      if( rhsvl < 0 )
         rhsvl = 0;
      if( !*opr )
         strcpy( opr, "==" );
      return mprog_veval( lhsvl, opr, rhsvl, mob );
   }

   if( !str_cmp( chck, "ovnumhere" ) )
   {
      int vnum = atoi( cvar );

      if( vnum < 1 || vnum > sysdata->maxvnum )
      {
         progbug( "OvnumHere: bad vnum", mob );
         return BERR;
      }
      lhsvl = 0;

      list < obj_data * >::iterator iobj;
      for( iobj = mob->carrying.begin(  ); iobj != mob->carrying.end(  ); ++iobj )
      {
         obj_data *pObj = *iobj;

         if( pObj->pIndexData->vnum == vnum )
            lhsvl += pObj->count;
      }
      for( iobj = mob->in_room->objects.begin(  ); iobj != mob->in_room->objects.end(  ); ++iobj )
      {
         obj_data *pObj = ( *iobj );

         if( pObj->pIndexData->vnum == vnum )
            lhsvl += pObj->count;
      }
      rhsvl = is_number( rval ) ? atoi( rval ) : -1;
      /*
       * Changed from 1 to 0 
       */
      if( rhsvl < 0 )
         rhsvl = 0;
      if( !*opr )
         strcpy( opr, "==" );
      return mprog_veval( lhsvl, opr, rhsvl, mob );
   }

   if( !str_cmp( chck, "otypehere" ) )
   {
      int type;

      if( is_number( cvar ) )
         type = atoi( cvar );
      else
         type = get_otype( cvar );
      if( type < 0 || type >= MAX_ITEM_TYPE )
      {
         progbug( "OtypeHere: bad type", mob );
         return BERR;
      }
      lhsvl = 0;

      list < obj_data * >::iterator iobj;
      for( iobj = mob->carrying.begin(  ); iobj != mob->carrying.end(  ); ++iobj )
      {
         obj_data *pObj = *iobj;
         if( pObj->pIndexData->vnum == type )
            lhsvl += pObj->count;
      }
      for( iobj = mob->in_room->objects.begin(  ); iobj != mob->in_room->objects.end(  ); ++iobj )
      {
         obj_data *pObj = *iobj;
         if( pObj->pIndexData->vnum == type )
            lhsvl += pObj->count;
      }
      rhsvl = is_number( rval ) ? atoi( rval ) : -1;
      /*
       * Change below from 1 to 0 
       */
      if( rhsvl < 0 )
         rhsvl = 0;
      if( !*opr )
         strcpy( opr, "==" );
      return mprog_veval( lhsvl, opr, rhsvl, mob );
   }

   if( !str_cmp( chck, "ovnumroom" ) )
   {
      int vnum = atoi( cvar );

      if( vnum < 1 || vnum > sysdata->maxvnum )
      {
         progbug( "OvnumRoom: bad vnum", mob );
         return BERR;
      }
      lhsvl = 0;
      list < obj_data * >::iterator iobj;
      for( iobj = mob->in_room->objects.begin(  ); iobj != mob->in_room->objects.end(  ); ++iobj )
      {
         obj_data *pObj = *iobj;
         if( pObj->pIndexData->vnum == vnum )
            lhsvl += pObj->count;
      }
      rhsvl = is_number( rval ) ? atoi( rval ) : -1;
      /*
       * Changed below from 1 to 0 so can check for == no items Shaddai 
       */
      if( rhsvl < 0 )
         rhsvl = 0;
      if( !*opr )
         strcpy( opr, "==" );
      return mprog_veval( lhsvl, opr, rhsvl, mob );
   }

   if( !str_cmp( chck, "otyperoom" ) )
   {
      int type;

      if( is_number( cvar ) )
         type = atoi( cvar );
      else
         type = get_otype( cvar );
      if( type < 0 || type >= MAX_ITEM_TYPE )
      {
         progbug( "OtypeRoom: bad type", mob );
         return BERR;
      }
      lhsvl = 0;

      list < obj_data * >::iterator iobj;
      for( iobj = mob->in_room->objects.begin(  ); iobj != mob->in_room->objects.end(  ); ++iobj )
      {
         obj_data *pObj = *iobj;
         if( pObj->pIndexData->vnum == type )
            lhsvl += pObj->count;
      }
      rhsvl = is_number( rval ) ? atoi( rval ) : -1;
      /*
       * Changed below from 1 to 0 
       */
      if( rhsvl < 0 )
         rhsvl = 0;
      if( !*opr )
         strcpy( opr, "==" );
      return mprog_veval( lhsvl, opr, rhsvl, mob );
   }

   if( !str_cmp( chck, "ovnumcarry" ) )
   {
      int vnum = atoi( cvar );

      if( vnum < 1 || vnum > sysdata->maxvnum )
      {
         progbug( "OvnumCarry: bad vnum", mob );
         return BERR;
      }
      lhsvl = 0;
      list < obj_data * >::iterator iobj;
      for( iobj = mob->carrying.begin(  ); iobj != mob->carrying.end(  ); ++iobj )
      {
         obj_data *pObj = *iobj;
         if( pObj->pIndexData->vnum == vnum )
            lhsvl += pObj->count;
      }
      rhsvl = is_number( rval ) ? atoi( rval ) : -1;

      if( rhsvl < 0 )
         rhsvl = 0;
      if( !*opr )
         strcpy( opr, "==" );
      return mprog_veval( lhsvl, opr, rhsvl, mob );
   }

   if( !str_cmp( chck, "otypecarry" ) )
   {
      int type;

      if( is_number( cvar ) )
         type = atoi( cvar );
      else
         type = get_otype( cvar );
      if( type < 0 || type >= MAX_ITEM_TYPE )
      {
         progbug( "OtypeCarry: bad type", mob );
         return BERR;
      }
      lhsvl = 0;
      list < obj_data * >::iterator iobj;
      for( iobj = mob->carrying.begin(  ); iobj != mob->carrying.end(  ); ++iobj )
      {
         obj_data *pObj = *iobj;
         if( pObj->pIndexData->vnum == type )
            lhsvl += pObj->count;
      }
      rhsvl = is_number( rval ) ? atoi( rval ) : -1;
      /*
       * Changed below from 1 to 0 Shaddai 
       */
      if( rhsvl < 0 )
         rhsvl = 0;
      if( !*opr )
         strcpy( opr, "==" );
      return mprog_veval( lhsvl, opr, rhsvl, mob );
   }

   if( !str_cmp( chck, "ovnumwear" ) )
   {
      int vnum = atoi( cvar );

      if( vnum < 1 || vnum > sysdata->maxvnum )
      {
         progbug( "OvnumWear: bad vnum", mob );
         return BERR;
      }
      lhsvl = 0;
      list < obj_data * >::iterator iobj;
      for( iobj = mob->carrying.begin(  ); iobj != mob->carrying.end(  ); ++iobj )
      {
         obj_data *pObj = *iobj;
         if( pObj->wear_loc != WEAR_NONE && pObj->pIndexData->vnum == vnum )
            lhsvl += pObj->count;
      }
      rhsvl = is_number( rval ) ? atoi( rval ) : -1;
      /*
       * Changed below from 1 to 0 
       */
      if( rhsvl < 0 )
         rhsvl = 0;
      if( !*opr )
         strcpy( opr, "==" );
      return mprog_veval( lhsvl, opr, rhsvl, mob );
   }

   if( !str_cmp( chck, "otypewear" ) )
   {
      int type;

      if( is_number( cvar ) )
         type = atoi( cvar );
      else
         type = get_otype( cvar );
      if( type < 0 || type >= MAX_ITEM_TYPE )
      {
         progbug( "OtypeWear: bad type", mob );
         return BERR;
      }
      lhsvl = 0;
      list < obj_data * >::iterator iobj;
      for( iobj = mob->carrying.begin(  ); iobj != mob->carrying.end(  ); ++iobj )
      {
         obj_data *pObj = *iobj;
         if( pObj->wear_loc != WEAR_NONE && pObj->pIndexData->vnum == type )
            lhsvl += pObj->count;
      }
      rhsvl = is_number( rval ) ? atoi( rval ) : -1;
      /*
       * Changed below from 1 to 0 so can have == 0 Shaddai 
       */
      if( rhsvl < 0 )
         rhsvl = 0;
      if( !*opr )
         strcpy( opr, "==" );
      return mprog_veval( lhsvl, opr, rhsvl, mob );
   }

   if( !str_cmp( chck, "ovnuminv" ) )
   {
      int vnum = atoi( cvar );

      if( vnum < 1 || vnum > sysdata->maxvnum )
      {
         progbug( "OvnumInv: bad vnum", mob );
         return BERR;
      }
      lhsvl = 0;
      list < obj_data * >::iterator iobj;
      for( iobj = mob->carrying.begin(  ); iobj != mob->carrying.end(  ); ++iobj )
      {
         obj_data *pObj = *iobj;
         if( pObj->wear_loc == WEAR_NONE && pObj->pIndexData->vnum == vnum )
            lhsvl += pObj->count;
      }
      rhsvl = is_number( rval ) ? atoi( rval ) : -1;
      /*
       * Changed 1 to 0 so can have == 0 
       */
      if( rhsvl < 0 )
         rhsvl = 0;
      if( !*opr )
         strcpy( opr, "==" );
      return mprog_veval( lhsvl, opr, rhsvl, mob );
   }

   if( !str_cmp( chck, "otypeinv" ) )
   {
      int type;

      if( is_number( cvar ) )
         type = atoi( cvar );
      else
         type = get_otype( cvar );
      if( type < 0 || type >= MAX_ITEM_TYPE )
      {
         progbug( "OtypeInv: bad type", mob );
         return BERR;
      }
      lhsvl = 0;
      list < obj_data * >::iterator iobj;
      for( iobj = mob->carrying.begin(  ); iobj != mob->carrying.end(  ); ++iobj )
      {
         obj_data *pObj = *iobj;
         if( pObj->wear_loc == WEAR_NONE && pObj->pIndexData->vnum == type )
            lhsvl += pObj->count;
      }
      rhsvl = is_number( rval ) ? atoi( rval ) : -1;
      /*
       * Changed below from 1 to 0 for == 0 Shaddai 
       */
      if( rhsvl < 0 )
         rhsvl = 0;
      if( !*opr )
         strcpy( opr, "==" );
      return mprog_veval( lhsvl, opr, rhsvl, mob );
   }

   if( chkchar )
   {
      if( !str_cmp( chck, "ispacifist" ) )
         return ( chkchar->has_actflag( ACT_PACIFIST ) );

      // Imported from Smaug 1.8b
      if( !str_cmp( chck, "stopscript" ) )
         return ( chkchar->has_actflag( ACT_STOP_SCRIPT ) );

      if( !str_cmp( chck, "ismobinvis" ) )
         return ( chkchar->has_actflag( ACT_MOBINVIS ) );

      if( !str_cmp( chck, "mobinvislevel" ) )
         return ( chkchar->isnpc(  ) ? mprog_veval( chkchar->mobinvis, opr, atoi( rval ), mob ) : false );

      // Imported from Smaug 1.8b
      if( !str_cmp( chck, "drunk" ) )
         return( !chkchar->isnpc() ? mprog_veval( chkchar->pcdata->condition[COND_DRUNK], opr, atoi(rval), mob) : false );

      if( !str_cmp( chck, "ispc" ) )
         return chkchar->isnpc(  )? false : true;

      if( !str_cmp( chck, "isnpc" ) )
         return chkchar->isnpc(  )? true : false;

      if( !str_cmp( chck, "cansee" ) )
         return mob->can_see( chkchar, false );

      // Imported from Smaug 1.8
      if( !str_cmp( chck, "isriding" ) )
      {
         if( chkchar->mount == mob )
            return true;
         return false;
      }

      if( !str_cmp( chck, "ispassage" ) )
      {
         if( !( find_door( chkchar, rval, true ) ) )
            return false;
         return true;
      }

      if( !str_cmp( chck, "isopen" ) )
      {
         exit_data *pexit;

         if( !( pexit = find_door( chkchar, rval, true ) ) )
            return false;
         if( !IS_EXIT_FLAG( pexit, EX_CLOSED ) )
            return true;
         return false;
      }

      if( !str_cmp( chck, "islocked" ) )
      {
         exit_data *pexit;

         if( !( pexit = find_door( chkchar, rval, true ) ) )
            return false;
         if( IS_EXIT_FLAG( pexit, EX_LOCKED ) )
            return true;
         return false;
      }

      if( !str_cmp( chck, "ispkill" ) )
         return chkchar->IS_PKILL(  ) ? true : false;

      if( !str_cmp( chck, "isdevoted" ) )
         return IS_DEVOTED( chkchar ) ? true : false;

      if( !str_cmp( chck, "canpkill" ) )
         return chkchar->CAN_PKILL(  ) ? true : false;

      if( !str_cmp( chck, "ismounted" ) )
         return ( chkchar->position == POS_MOUNTED );

      if( !str_cmp( chck, "ismorphed" ) )
         return ( chkchar->morph != NULL ) ? true : false;

      if( !str_cmp( chck, "isgood" ) )
         return chkchar->IS_GOOD(  );

      if( !str_cmp( chck, "isneutral" ) )
         return chkchar->IS_NEUTRAL(  );

      if( !str_cmp( chck, "isevil" ) )
         return chkchar->IS_EVIL(  );

      if( !str_cmp( chck, "isfight" ) )
         return chkchar->who_fighting(  ) ? true : false;

      if( !str_cmp( chck, "isimmort" ) )
         return chkchar->is_immortal(  );

      if( !str_cmp( chck, "ischarmed" ) )
      {
         if( chkchar->has_aflag( AFF_CHARM ) || chkchar->has_aflag( AFF_POSSESS ) )
            return true;
         return false;
      }

      // Imported from Smaug 1.8b
      if( !str_cmp( chck, "ispossesed" ) )
         return chkchar->has_aflag( AFF_POSSESS );

      if( !str_cmp( chck, "isflying" ) )
         return chkchar->has_aflag( AFF_FLYING );

      if( !str_cmp( chck, "isfollow" ) )
         return ( chkchar->master != NULL && chkchar->master->in_room == chkchar->in_room ) ? true : false;

      if( !str_cmp( chck, "isaffected" ) )
      {
         int value = get_aflag( rval );

         if( value < 0 || value >= MAX_AFFECTED_BY )
         {
            progbug( "Unknown affect being checked", mob );
            return BERR;
         }
         return chkchar->has_aflag( value );
      }

      /*
       * abits and qbits 
       */
      if( !str_cmp( chck, "hasabit" ) )
      {
         int number;

         if( is_number( rval ) )
         {
            number = atoi( rval );

            if( chkchar->abits.find( number ) == chkchar->abits.end(  ) )
               return mprog_veval( 0, opr, 1, mob );
            else
               return mprog_veval( 1, opr, 1, mob );
         }
         progbug( "hasabit: bad abit number", mob );
         return BERR;
      }

      /*
       * abits and qbits 
       */
      if( !str_cmp( chck, "hasqbit" ) )
      {
         int number;

         if( chkchar->isnpc(  ) )
            return mprog_veval( 0, opr, 1, mob );

         if( is_number( rval ) )
         {
            number = atoi( rval );

            if( chkchar->pcdata->qbits.find( number ) == chkchar->pcdata->qbits.end(  ) )
               return mprog_veval( 0, opr, 1, mob );
            else
               return mprog_veval( 1, opr, 1, mob );
         }
         progbug( "hasqbit: bad qbit number", mob );
         return BERR;
      }

      /*
       * if( !str_cmp( chck, "isflagged" ) ) <--- FIXME: We don't have EXT_BV, conver to std::bitset
       * {
       * variable_data *vd;
       * int vnum = mob->pIndexData->vnum;
       * int flag = 0;
       * 
       * if( argc < 3 )
       * return BERR;
       * 
       * if( argc > 3 )
       * flag = atoi( argv[3] );
       * 
       * if( ( p = strchr( argv[2], ':' ) ) != NULL )
       * {
       * *p++ = '\0';
       * vnum = atoi( p );
       * }
       * 
       * if( ( vd = get_tag( chkchar, argv[2], vnum ) ) == NULL )
       * return false;
       * 
       * flag = abs( flag ) % MAX_BITS;
       * switch ( vd->type )
       * {
       * case vtSTR:
       * case vtINT:
       * return false;
       * case vtXBIT:
       * return xIS_SET( *( EXT_BV * ) vd->data, flag ) ? true : false;
       * }
       * return false;
       * } 
       */

      if( !str_cmp( chck, "istagged" ) )
      {
         variable_data *vd;
         int vnum = mob->pIndexData->vnum;

         if( argc < 3 )
            return BERR;

         if( argc > 3 )
            vnum = atoi( argv[3] );

         if( ( p = strchr( argv[2], ':' ) ) != NULL )
         {
            *p++ = '\0';
            vnum = atoi( p );
         }

         if( ( vd = get_tag( chkchar, argv[2], vnum ) ) == NULL )
            return false;

         if( !*opr && !*rval )
            return true;

         switch ( vd->type )
         {
            case vtSTR:
               return mprog_seval( vd->varstring.c_str(), opr, rval, mob );

            case vtINT:
               return mprog_veval( vd->vardata, opr, atoi( rval ), mob );

            case vtXBIT:
               return false;
         }
         return false;
      }

      if( !str_cmp( chck, "numfighting" ) )
         return mprog_veval( chkchar->num_fighting - 1, opr, atoi( rval ), mob );

      if( !str_cmp( chck, "hitprcnt" ) )
         return mprog_veval( ( chkchar->hit * 100 ) / chkchar->max_hit, opr, atoi( rval ), mob );

      if( !str_cmp( chck, "inroom" ) )
         return mprog_veval( chkchar->in_room->vnum, opr, atoi( rval ), mob );

      if( !str_cmp( chck, "wasinroom" ) )
      {
         if( !chkchar->was_in_room )
            return false;
         return mprog_veval( chkchar->was_in_room->vnum, opr, atoi( rval ), mob );
      }

      // Imported from Smaug 1.8b
      if( !str_cmp( chck, "indoors" ) )
         return ( ( chkchar->IS_OUTSIDE() && chkchar->in_room->sector_type != SECT_INDOORS ) ? false : true );

      // Imported from Smaug 1.8b
      if( !str_cmp( chck, "nomagic" ) )
         return chkchar->in_room->flags.test( ROOM_NO_MAGIC ) ? true : false;

      // Imported from Smaug 1.8b
      if( !str_cmp( chck, "safe" ) )
         return chkchar->in_room->flags.test( ROOM_SAFE ) ? true : false;

      // Imported from Smaug 1.8b
      if( !str_cmp( chck, "nosummon" ) )
         return chkchar->in_room->flags.test( ROOM_NO_SUMMON ) ? true : false;

      // Imported from Smaug 1.8b
      if( !str_cmp( chck, "noastral" ) )
         return chkchar->in_room->flags.test( ROOM_NO_ASTRAL ) ? true : false;

      // Imported from Smaug 1.8b
      if( !str_cmp( chck, "nosupplicate" ) )
         return chkchar->in_room->flags.test( ROOM_NOSUPPLICATE ) ? true : false;

      if( !str_cmp( chck, "norecall" ) )
         return chkchar->in_room->flags.test( ROOM_NO_RECALL ) ? true : false;

      if( !str_cmp( chck, "sex" ) )
         return mprog_veval( chkchar->sex, opr, atoi( rval ), mob );

      if( !str_cmp( chck, "position" ) )
         return mprog_veval( chkchar->position, opr, atoi( rval ), mob );

      if( !str_cmp( chck, "ishelled" ) )
         return chkchar->isnpc(  )? false : mprog_veval( chkchar->pcdata->release_date, opr, atoi( rval ), mob );

      /*
       * Ifcheck provided by Moonwolf@Wolfs Lair 
       */
      if( !str_cmp( chck, "speaking" ) )
      {
         for( lang = 0; lang < LANG_UNKNOWN; ++lang )
            if( chkchar->speaking == lang )
               return mprog_seval( lang_names[lang], opr, rval, mob );

         return false;
      }

      /*
       * Ifcheck provided by Moonwolf@Wolfs Lair 
       */
      if( !str_cmp( chck, "speaks" ) )
      {
         for( lang = 0; lang < LANG_UNKNOWN; ++lang )
            if( chkchar->has_lang( lang ) )
               return mprog_seval( lang_names[lang], opr, rval, mob );

         return false;
      }

      if( !str_cmp( chck, "level" ) )
         return mprog_veval( chkchar->get_trust(  ), opr, atoi( rval ), mob );

      if( !str_cmp( chck, "goldamt" ) )
         return mprog_veval( chkchar->gold, opr, atoi( rval ), mob );

      if( !str_cmp( chck, "class" ) )
         return mprog_seval( npc_class[chkchar->Class], opr, rval, mob );

      if( !str_cmp( chck, "weight" ) )
         return mprog_veval( chkchar->carry_weight, opr, atoi( rval ), mob );

      if( !str_cmp( chck, "hostdesc" ) )
      {
         if( chkchar->isnpc(  ) || chkchar->desc->host.empty(  ) )
            return false;
         return mprog_seval( chkchar->desc->host.c_str(  ), opr, rval, mob );
      }

      if( !str_cmp( chck, "multi" ) )
      {
         lhsvl = 0;

         for( ich = pclist.begin(  ); ich != pclist.end(  ); ++ich )
         {
            char_data *ch = *ich;

            if( !chkchar->isnpc(  ) && ch->desc && chkchar->desc && !str_cmp( ch->desc->host, chkchar->desc->host ) )
               ++lhsvl;
         }
         rhsvl = atoi( rval );
         if( rhsvl < 0 )
            rhsvl = 0;
         if( !*opr )
            strcpy( opr, "==" );
         return mprog_veval( lhsvl, opr, rhsvl, mob );
      }

      if( !str_cmp( chck, "race" ) )
         return mprog_seval( npc_race[chkchar->race], opr, rval, mob );

      if( !str_cmp( chck, "morph" ) )
      {
         if( !chkchar->morph )
            return false;
         if( !chkchar->morph->morph )
            return false;
         return mprog_veval( chkchar->morph->morph->vnum, opr, rhsvl, mob );
      }

      if( !str_cmp( chck, "clan" ) )
      {
         if( chkchar->isnpc(  ) || !chkchar->pcdata->clan )
            return false;
         return mprog_seval( chkchar->pcdata->clan->name.c_str(  ), opr, rval, mob );
      }

      /*
       * Is char wearing some eq on a specific wear loc?  -- Gorog 
       */
      if( !str_cmp( chck, "wearing" ) )
      {
         list < obj_data * >::iterator iobj;
         int i = 0;

         for( iobj = chkchar->carrying.begin(  ); iobj != chkchar->carrying.end(  ); ++iobj )
         {
            obj_data *tobj = *iobj;
            ++i;

            if( chkchar == tobj->carried_by && tobj->wear_loc > -1 && !str_cmp( rval, item_w_flags[tobj->wear_loc] ) )
               return true;
         }
         return false;
      }

      /*
       * Is char wearing some specific vnum?  -- Gorog 
       */
      if( !str_cmp( chck, "wearingvnum" ) )
      {
         if( !is_number( rval ) )
            return false;

         list < obj_data * >::iterator iobj;
         for( iobj = chkchar->carrying.begin(  ); iobj != chkchar->carrying.end(  ); ++iobj )
         {
            obj_data *tobj = *iobj;
            if( chkchar == tobj->carried_by && tobj->wear_loc > -1 && tobj->pIndexData->vnum == atoi( rval ) )
               return true;
         }
         return false;
      }

      /*
       * Is char carrying a specific piece of eq?  -- Gorog 
       */
      if( !str_cmp( chck, "carryingvnum" ) )
      {
         int vnum;

         if( !is_number( rval ) )
            return false;
         vnum = atoi( rval );
         if( chkchar->carrying.empty(  ) )
            return false;
         return ( carryingvnum_visit( chkchar, chkchar->carrying, vnum ) );
      }

      /*
       * Check added to see if the person isleader of == clan Gorog 
       */
      if( !str_cmp( chck, "isleader" ) )
      {
         clan_data *temp;

         if( chkchar->isnpc(  ) )
            return false;

         if( !( temp = get_clan( rval ) ) )
            return false;
         if( mprog_seval( chkchar->name, opr, temp->leader.c_str(  ), mob ) )
            return true;
         else
            return false;
      }

      if( !str_cmp( chck, "isclan1" ) )
      {
         clan_data *temp;

         if( chkchar->isnpc(  ) )
            return false;

         if( !( temp = get_clan( rval ) ) )
            return false;
         if( mprog_seval( chkchar->name, opr, temp->number1.c_str(  ), mob ) )
            return true;
         else
            return false;
      }

      if( !str_cmp( chck, "isclan2" ) )
      {
         clan_data *temp;

         if( chkchar->isnpc(  ) )
            return false;

         if( !( temp = get_clan( rval ) ) )
            return false;
         if( mprog_seval( chkchar->name, opr, temp->number2.c_str(  ), mob ) )
            return true;
         else
            return false;
      }

      if( !str_cmp( chck, "deity" ) )
      {
         if( !IS_DEVOTED( chkchar ) )
            return false;
         return mprog_seval( chkchar->pcdata->deity->name.c_str(  ), opr, rval, mob );
      }

      if( !str_cmp( chck, "guild" ) )
      {
         if( !IS_GUILDED( chkchar ) )
            return false;
         return mprog_seval( chkchar->pcdata->clan->name.c_str(  ), opr, rval, mob );
      }

      if( !str_cmp( chck, "clantype" ) )
      {
         if( !IS_CLANNED( chkchar ) )
            return false;
         return mprog_veval( chkchar->pcdata->clan->clan_type, opr, atoi( rval ), mob );
      }

      if( !str_cmp( chck, "waitstate" ) )
      {
         if( chkchar->isnpc(  ) || !chkchar->wait )
            return false;
         return mprog_veval( chkchar->wait, opr, atoi( rval ), mob );
      }

      if( !str_cmp( chck, "asupressed" ) )
         return mprog_veval( chkchar->get_timer( TIMER_ASUPRESSED ), opr, atoi( rval ), mob );

      if( !str_cmp( chck, "favor" ) )
      {
         if( !IS_DEVOTED( chkchar ) )
            return false;
         return mprog_veval( chkchar->pcdata->favor, opr, atoi( rval ), mob );
      }

      if( !str_cmp( chck, "hps" ) )
         return mprog_veval( chkchar->hit, opr, atoi( rval ), mob );

      if( !str_cmp( chck, "mana" ) )
         return mprog_veval( chkchar->mana, opr, atoi( rval ), mob );

      if( !str_cmp( chck, "str" ) )
         return mprog_veval( chkchar->get_curr_str(  ), opr, atoi( rval ), mob );

      if( !str_cmp( chck, "wis" ) )
         return mprog_veval( chkchar->get_curr_wis(  ), opr, atoi( rval ), mob );

      if( !str_cmp( chck, "int" ) )
         return mprog_veval( chkchar->get_curr_int(  ), opr, atoi( rval ), mob );

      if( !str_cmp( chck, "dex" ) )
         return mprog_veval( chkchar->get_curr_dex(  ), opr, atoi( rval ), mob );

      if( !str_cmp( chck, "con" ) )
         return mprog_veval( chkchar->get_curr_con(  ), opr, atoi( rval ), mob );

      if( !str_cmp( chck, "cha" ) )
         return mprog_veval( chkchar->get_curr_cha(  ), opr, atoi( rval ), mob );

      if( !str_cmp( chck, "lck" ) )
         return mprog_veval( chkchar->get_curr_lck(  ), opr, atoi( rval ), mob );
   }

   if( chkobj )
   {
      if( !str_cmp( chck, "objtype" ) )
         return mprog_veval( chkobj->item_type, opr, atoi( rval ), mob );

      if( !str_cmp( chck, "leverpos" ) )
      {
         int isup = false, wantsup = false;

         if( chkobj->item_type != ITEM_SWITCH && chkobj->item_type != ITEM_LEVER && chkobj->item_type != ITEM_PULLCHAIN && chkobj->item_type != ITEM_BUTTON )
            return false;

         if( IS_SET( obj->value[0], TRIG_UP ) )
            isup = true;
         if( !str_cmp( rval, "up" ) )
            wantsup = true;
         return mprog_veval( wantsup, opr, isup, mob );
      }

      if( !str_cmp( chck, "objval0" ) )
         return mprog_veval( chkobj->value[0], opr, atoi( rval ), mob );

      if( !str_cmp( chck, "objval1" ) )
         return mprog_veval( chkobj->value[1], opr, atoi( rval ), mob );

      if( !str_cmp( chck, "objval2" ) )
         return mprog_veval( chkobj->value[2], opr, atoi( rval ), mob );

      if( !str_cmp( chck, "objval3" ) )
         return mprog_veval( chkobj->value[3], opr, atoi( rval ), mob );

      if( !str_cmp( chck, "objval4" ) )
         return mprog_veval( chkobj->value[4], opr, atoi( rval ), mob );

      if( !str_cmp( chck, "objval5" ) )
         return mprog_veval( chkobj->value[5], opr, atoi( rval ), mob );

      if( !str_cmp( chck, "objval6" ) )
         return mprog_veval( chkobj->value[6], opr, atoi( rval ), mob );

      if( !str_cmp( chck, "objval7" ) )
         return mprog_veval( chkobj->value[7], opr, atoi( rval ), mob );

      if( !str_cmp( chck, "objval8" ) )
         return mprog_veval( chkobj->value[8], opr, atoi( rval ), mob );

      if( !str_cmp( chck, "objval9" ) )
         return mprog_veval( chkobj->value[9], opr, atoi( rval ), mob );

      if( !str_cmp( chck, "objval10" ) )
         return mprog_veval( chkobj->value[10], opr, atoi( rval ), mob );
   }

   /*
    * The following checks depend on the fact that cval[1] can only contain
    * one character, and that NULL checks were made previously.
    */
   if( !str_cmp( chck, "number" ) )
   {
      if( chkchar )
      {
         if( !chkchar->isnpc(  ) )
            return false;
         lhsvl = ( chkchar == mob ) ? chkchar->gold : chkchar->pIndexData->vnum;
         return mprog_veval( lhsvl, opr, atoi( rval ), mob );
      }
      return mprog_veval( chkobj->pIndexData->vnum, opr, atoi( rval ), mob );
   }

   if( !str_cmp( chck, "time" ) )
      return mprog_veval( time_info.hour, opr, atoi( rval ), mob );

   if( !str_cmp( chck, "month" ) )
      return mprog_veval( time_info.month, opr, atoi( rval ), mob );

   if( !str_cmp( chck, "name" ) )
   {
      if( chkchar )
         return mprog_seval( chkchar->name, opr, rval, mob );
      return mprog_seval( chkobj->name, opr, rval, mob );
   }

   if( !str_cmp( chck, "rank" ) )   /* Shaddai */
   {
      if( chkchar && !chkchar->isnpc(  ) )
         return mprog_seval( chkchar->pcdata->rank, opr, rval, mob );
      return false;
   }

   if( !str_cmp( chck, "mortinworld" ) )  /* -- Gorog */
   {
      for( ds = dlist.begin(  ); ds != dlist.end(  ); ++ds )
      {
         descriptor_data *d = *ds;

         if( d->connected == CON_PLAYING && d->character && !d->character->is_immortal(  ) && !str_cmp( d->character->name, cvar ) )
            return true;
      }
      return false;
   }

   if( !str_cmp( chck, "immortinworld" ) )   /* -- Samson */
   {
      for( ds = dlist.begin(  ); ds != dlist.end(  ); ++ds )
      {
         descriptor_data *d = *ds;

         if( d->connected == CON_PLAYING && d->character && d->character->is_immortal(  ) && !str_cmp( d->character->name, cvar ) )
            return true;
      }
      return false;
   }

   if( !str_cmp( chck, "mortinroom" ) )   /* -- Gorog */
   {
      for( ich = mob->in_room->people.begin(  ); ich != mob->in_room->people.end(  ); ++ich )
      {
         char_data *ch = *ich;

         if( ( !ch->isnpc(  ) ) && !ch->is_immortal(  ) && hasname( cvar, ch->name ) )
            return true;
      }
      return false;
   }

   if( !str_cmp( chck, "immortinroom" ) ) /* Tarl */
   {
      for( ich = mob->in_room->people.begin(  ); ich != mob->in_room->people.end(  ); ++ich )
      {
         char_data *ch = *ich;

         if( ( !ch->isnpc(  ) ) && ch->is_immortal(  ) && hasname( cvar, ch->name ) )
            return true;
      }
      return false;
   }

   if( !str_cmp( chck, "daytime" ) )   /* Tarl */
   {
      if( time_info.hour >= sysdata->hoursunrise && time_info.hour <= sysdata->hoursunset )
         return true;
      return false;
   }

   // Imported from Smaug 1.8b
   if( !str_cmp( chck, "inarea" ) )
   {
      if( chkchar )
         return mprog_seval( chkchar->in_room->area->filename,  opr, rval, mob );
      return false;
   }

   if( !str_cmp( chck, "mortinarea" ) )   /* -- Gorog */
   {
      for( ds = dlist.begin(  ); ds != dlist.end(  ); ++ds )
      {
         descriptor_data *d = *ds;

         if( d->connected == CON_PLAYING && d->character && d->character->is_immortal(  )
             && d->character->in_room->area == mob->in_room->area && !str_cmp( d->character->name, cvar ) )
            return true;
      }
      return false;
   }

   if( !str_cmp( chck, "mortcount" ) ) /* -- Gorog */
   {
      room_index *room;
      int count = 0, rvnum = atoi( cvar );

      if( !( room = get_room_index( rvnum ? rvnum : mob->in_room->vnum ) ) )
         return mprog_veval( count, opr, atoi( rval ), mob );

      for( ich = room->people.begin(  ); ich != room->people.end(  ); ++ich )
      {
         char_data *tch = *ich;

         if( ( !tch->isnpc(  ) ) && !tch->is_immortal(  ) )
            ++count;
      }
      return mprog_veval( count, opr, atoi( rval ), mob );
   }

   if( !str_cmp( chck, "immortcount" ) )  /* -- Samson */
   {
      room_index *room;
      int count = 0, rvnum = atoi( cvar );

      if( !( room = get_room_index( rvnum ? rvnum : mob->in_room->vnum ) ) )
         return mprog_veval( count, opr, atoi( rval ), mob );

      for( ich = room->people.begin(  ); ich != room->people.end(  ); ++ich )
      {
         char_data *tch = *ich;

         if( ( !tch->isnpc(  ) ) && tch->is_immortal(  ) )
            ++count;
      }
      return mprog_veval( count, opr, atoi( rval ), mob );
   }

   if( !str_cmp( chck, "mobcount" ) )  /* -- Gorog */
   {
      room_index *room;
      int count = 0, rvnum = atoi( cvar );

      if( !( room = get_room_index( rvnum ? rvnum : mob->in_room->vnum ) ) )
         return mprog_veval( count, opr, atoi( rval ), mob );

      for( ich = room->people.begin(  ); ich != room->people.end(  ); ++ich )
      {
         char_data *tch = *ich;

         if( tch->isnpc(  ) )
            ++count;
      }
      return mprog_veval( count, opr, atoi( rval ), mob );
   }

   if( !str_cmp( chck, "charcount" ) ) /* -- Gorog */
   {
      room_index *room;
      int count = 0, rvnum = atoi( cvar );

      if( !( room = get_room_index( rvnum ? rvnum : mob->in_room->vnum ) ) )
         return mprog_veval( count, opr, atoi( rval ), mob );

      for( ich = room->people.begin(  ); ich != room->people.end(  ); ++ich )
      {
         char_data *tch = *ich;

         if( ( !tch->isnpc(  ) && !tch->is_immortal(  ) ) || tch->isnpc(  ) ) /* mortal or mob */
            ++count;
      }
      return mprog_veval( count, opr, atoi( rval ), mob );
   }

   /*
    * Ok... all the ifchecks are done, so if we didnt find ours then something
    * odd happened.  So report the bug and abort the MUDprogram (return error)
    */
   progbug( "Unknown ifcheck", mob );
   return BERR;
}

#undef isoperator
#undef MAX_IF_ARGS

/* This routine handles the variables for command expansion.
 * If you want to add any go right ahead, it should be fairly
 * clear how it is done and they are quite easy to do, so you
 * can be as creative as you want. The only catch is to check
 * that your variables exist before you use them. At the moment,
 * using $t when the secondary target refers to an object 
 * i.e. >prog_act drops~<nl>if ispc($t)<nl>sigh<nl>endif<nl>~<nl>
 * probably makes the mud crash (vice versa as well) The cure
 * would be to change act() so that vo becomes vict & v_obj.
 * but this would require a lot of small changes all over the code.
 */

/*
 *  There's no reason to make the mud crash when a variable's
 *  fubared.  I added some ifs.  I'm willing to trade some 
 *  performance for stability. -Haus
 *
 *  Added char_died and obj_extracted checks	-Thoric
 */
void mprog_translate( char ch, char *t, char_data * mob, char_data * actor, obj_data * obj, char_data * vict, obj_data * v_obj, char_data * rndm )
{
   static const char *he_she[] = { "it", "he", "she" };
   static const char *him_her[] = { "it", "him", "her" };
   static const char *his_her[] = { "its", "his", "her" };

   *t = '\0';
   switch ( ch )
   {
      case 'i':
         if( mob && !mob->char_died(  ) )
         {
            if( mob->name )
               one_argument( mob->name, t );
         }
         else
            strcpy( t, "someone" );
         break;

      case 'I':
         if( mob && !mob->char_died(  ) )
         {
            if( mob->short_descr )
            {
               strcpy( t, mob->short_descr );
            }
            else
            {
               strcpy( t, "someone" );
            }
         }
         else
            strcpy( t, "someone" );
         break;

      case 'n':
         if( actor && !actor->char_died(  ) )
         {
            if( mob->can_see( actor, false ) )
               one_argument( actor->name, t );
            else
               strcpy( t, "someone" );
            if( !actor->isnpc(  ) )
               *t = UPPER( *t );
         }
         else
            strcpy( t, "someone" );
         break;

      case 'N':
         if( actor && !actor->char_died(  ) )
         {
            if( mob->can_see( actor, false ) )
            {
               if( actor->isnpc(  ) )
                  strcpy( t, actor->short_descr );
               else
               {
                  strcpy( t, actor->name );
                  strcat( t, actor->pcdata->title );
               }
            }
            else
               strcpy( t, "someone" );
         }
         else
            strcpy( t, "someone" );
         break;

      case 't':
         if( vict && !vict->char_died(  ) )
         {
            if( mob->can_see( vict, false ) )
               one_argument( vict->name, t );
            if( !vict->isnpc(  ) )
               *t = UPPER( *t );
         }
         else
            strcpy( t, "someone" );

         break;

      case 'T':
         if( vict && !vict->char_died(  ) )
         {
            if( mob->can_see( vict, false ) )
            {
               if( vict->isnpc(  ) )
                  strcpy( t, vict->short_descr );
               else
               {
                  strcpy( t, vict->name );
                  strcat( t, " " );
                  strcat( t, vict->pcdata->title );
               }
            }
            else
               strcpy( t, "someone" );
         }
         else
            strcpy( t, "someone" );
         break;

      case 'r':
         if( rndm && !rndm->char_died(  ) )
         {
            if( mob->can_see( rndm, false ) )
            {
               one_argument( rndm->name, t );
            }
            if( !rndm->isnpc(  ) )
            {
               *t = UPPER( *t );
            }
         }
         else
            strcpy( t, "someone" );
         break;

      case 'R':
         if( rndm && !rndm->char_died(  ) )
         {
            if( mob->can_see( rndm, false ) )
            {
               if( rndm->isnpc(  ) )
                  strcpy( t, rndm->short_descr );
               else
               {
                  strcpy( t, rndm->name );
                  strcat( t, " " );
                  strcat( t, rndm->pcdata->title );
               }
            }
            else
               strcpy( t, "someone" );
         }
         else
            strcpy( t, "someone" );
         break;

      case 'e':
         if( actor && !actor->char_died(  ) )
            mob->can_see( actor, false ) ? strcpy( t, he_she[actor->sex] ) : strcpy( t, "someone" );
         else
            strcpy( t, "it" );
         break;

      case 'm':
         if( actor && !actor->char_died(  ) )
            mob->can_see( actor, false ) ? strcpy( t, him_her[actor->sex] ) : strcpy( t, "someone" );
         else
            strcpy( t, "it" );
         break;

      case 's':
         if( actor && !actor->char_died(  ) )
            mob->can_see( actor, false ) ? strcpy( t, his_her[actor->sex] ) : strcpy( t, "someone's" );
         else
            strcpy( t, "its'" );
         break;

      case 'E':
         if( vict && !vict->char_died(  ) )
            mob->can_see( vict, false ) ? strcpy( t, he_she[vict->sex] ) : strcpy( t, "someone" );
         else
            strcpy( t, "it" );
         break;

      case 'M':
         if( vict && !vict->char_died(  ) )
            mob->can_see( vict, false ) ? strcpy( t, him_her[vict->sex] ) : strcpy( t, "someone" );
         else
            strcpy( t, "it" );
         break;

      case 'S':
         if( vict && !vict->char_died(  ) )
            mob->can_see( vict, false ) ? strcpy( t, his_her[vict->sex] ) : strcpy( t, "someone's" );
         else
            strcpy( t, "its'" );
         break;

      case 'j':
         if( mob && !mob->char_died(  ) )
         {
            strcpy( t, he_she[mob->sex] );
         }
         else
         {
            strcpy( t, "it" );
         }
         break;

      case 'k':
         if( mob && !mob->char_died(  ) )
         {
            strcpy( t, him_her[mob->sex] );
         }
         else
         {
            strcpy( t, "it" );
         }
         break;

      case 'l':
         if( mob && !mob->char_died(  ) )
         {
            strcpy( t, his_her[mob->sex] );
         }
         else
         {
            strcpy( t, "it" );
         }
         break;

      case 'J':
         if( rndm && !rndm->char_died(  ) )
            mob->can_see( rndm, false ) ? strcpy( t, he_she[rndm->sex] ) : strcpy( t, "someone" );
         else
            strcpy( t, "it" );
         break;

      case 'K':
         if( rndm && !rndm->char_died(  ) )
            mob->can_see( rndm, false ) ? strcpy( t, him_her[rndm->sex] ) : strcpy( t, "someone's" );
         else
            strcpy( t, "its'" );
         break;

      case 'L':
         if( rndm && !rndm->char_died(  ) )
            mob->can_see( rndm, false ) ? strcpy( t, his_her[rndm->sex] ) : strcpy( t, "someone" );
         else
            strcpy( t, "its" );
         break;

      case 'o':
         if( obj && !obj->extracted(  ) )
         {
            mob->can_see_obj( obj, false ) ? one_argument( obj->name, t ) : strcpy( t, "something" );
         }
         else
            strcpy( t, "something" );
         break;

      case 'O':
         if( obj && !obj->extracted(  ) )
         {
            mob->can_see_obj( obj, false ) ? strcpy( t, obj->short_descr ) : strcpy( t, "something" );
         }
         else
            strcpy( t, "something" );
         break;

      case 'p':
         if( v_obj && !v_obj->extracted(  ) )
         {
            mob->can_see_obj( v_obj, false ) ? one_argument( v_obj->name, t ) : strcpy( t, "something" );
         }
         else
            strcpy( t, "something" );
         break;

      case 'P':
         if( v_obj && !v_obj->extracted(  ) )
         {
            mob->can_see_obj( v_obj, false ) ? strcpy( t, v_obj->short_descr ) : strcpy( t, "something" );
         }
         else
            strcpy( t, "something" );
         break;

      case 'a':
         if( obj && !obj->extracted(  ) )
         {
            strcpy( t, aoran( obj->name ) );
/*
          switch ( *( obj->name ) )
	    {
	      case 'a': case 'e': case 'i':
            case 'o': case 'u': strcpy( t, "an" );
	      break;
            default: strcpy( t, "a" );
          }
*/
         }
         else
            strcpy( t, "a" );
         break;

      case 'A':
         if( v_obj && !v_obj->extracted(  ) )
         {
            strcpy( t, aoran( v_obj->name ) );
         }
         else
            strcpy( t, "a" );
         break;

      case '$':
         strcpy( t, "$" );
         break;

      default:
         progbug( "Bad $var", mob );
         break;
   }
}

/*  The main focus of the MOBprograms.  This routine is called 
 *  whenever a trigger is successful.  It is responsible for parsing
 *  the command list and figuring out what to do. However, like all
 *  complex procedures, everything is farmed out to the other guys.
 *
 *  This function rewritten by Narn for Realms of Despair, Dec/95.
 *
 */
void mprog_driver( char *com_list, char_data * mob, char_data * actor, obj_data * obj, char_data * victim, obj_data * target, bool single_step )
{
   bool ifstate[MAX_IFS][DO_ELSE + 1];

   if( mob->has_aflag( AFF_CHARM ) || mob->has_aflag( AFF_POSSESS ) )
      return;

   /*
    * Next couple of checks stop program looping. -- Altrag 
    */
   if( mob == actor )
   {
      progbug( "triggering oneself.", mob );
      return;
   }

   static int prog_nest;
   static int serial;
   int curr_serial;
   room_index *supermob_room;
   obj_data *true_supermob_obj;
   bool rprog_oprog = ( mob == supermob );

   if( rprog_oprog )
   {
      ++serial;
      supermob_room = mob->in_room;
      true_supermob_obj = supermob_obj;
   }
   else
      true_supermob_obj = NULL, supermob_room = NULL;
   curr_serial = serial;

   if( ++prog_nest > MAX_PROG_NEST )
   {
      progbug( "max_prog_nest exceeded.", mob );
      --prog_nest;
      return;
   }

   /*
    * Make sure all ifstate bools are set to false 
    */
   int iflevel, count;
   for( iflevel = 0; iflevel < MAX_IFS; ++iflevel )
   {
      for( count = 0; count < DO_ELSE; ++count )
      {
         ifstate[iflevel][count] = false;
      }
   }

   iflevel = 0;

   /*
    * get a random visible player who is in the room with the mob.
    *
    *  If there isn't a random player in the room, rndm stays NULL.
    *  If you do a $r, $R, $j, or $k with rndm = NULL, you'll crash
    *  in mprog_translate.
    *
    *  Adding appropriate error checking in mprog_translate.
    *    -Haus
    *
    * This used to ignore players MAX_LEVEL - 3 and higher (standard
    * Merc has 4 immlevels).  Thought about changing it to ignore all
    * imms, but decided to just take it out.  If the mob can see you, 
    * you may be chosen as the random player. -Narn
    *
    */
   count = 0;
   char_data *rndm = NULL;
   list < char_data * >::iterator ich;
   for( ich = mob->in_room->people.begin(  ); ich != mob->in_room->people.end(  ); ++ich )
   {
      char_data *vch = *ich;
      if( !vch->isnpc(  ) && mob->can_see( vch, false ) )
      {
         if( number_range( 0, count ) == 0 )
            rndm = vch;
         ++count;
      }
   }
   char tmpcmndlst[MSL];
   mudstrlcpy( tmpcmndlst, com_list, MSL );
   char *command_list = tmpcmndlst;

   /*
    * mpsleep - Restore the environment  -rkb 
    */
   int ignorelevel = 0;
   if( current_mpsleep )
   {
      ignorelevel = current_mpsleep->ignorelevel;
      iflevel = current_mpsleep->iflevel;
      int count2;

      if( single_step )
         mob->mpscriptpos = 0;

      for( count = 0; count < MAX_IFS; ++count )
      {
         for( count2 = 0; count2 <= DO_ELSE; ++count2 )
            ifstate[count][count2] = current_mpsleep->ifstate[count][count2];
      }
      current_mpsleep = NULL;
   }

   if( single_step )
   {
      if( mob->mpscriptpos > strlen( tmpcmndlst ) )
         mob->mpscriptpos = 0;
      else
         command_list += mob->mpscriptpos;
      if( *command_list == '\0' )
      {
         command_list = tmpcmndlst;
         mob->mpscriptpos = 0;
      }
   }

   /*
    * From here on down, the function is all mine.  The original code
    * did not support nested ifs, so it had to be redone.  The max 
    * logiclevel (MAX_IFS) is defined at the beginning of this file, 
    * use it to increase/decrease max allowed nesting.  -Narn 
    */
   while( true )
   {
      /*
       * With these two lines, cmnd becomes the current line from the prog,
       * and command_list becomes everything after that line. 
       */
      char *cmnd = command_list;
      command_list = mprog_next_command( command_list );

      /*
       * Are we at the end? 
       */
      if( cmnd[0] == '\0' )
      {
         if( ifstate[iflevel][IN_IF] || ifstate[iflevel][IN_ELSE] )
         {
            progbug( "Missing endif", mob );
         }
         --prog_nest;
         return;
      }

      /*
       * mpsleep - Check if we should sleep  -rkb 
       */
      if( !str_prefix( "mpsleep", cmnd ) )
      {
         /*
          * We are ignoring it, so just skip to the next one. 
          */
         if( ( ifstate[iflevel][IN_IF] && !ifstate[iflevel][DO_IF] ) || ( ifstate[iflevel][IN_ELSE] && !ifstate[iflevel][DO_ELSE] ) )
            continue;

         mpsleep_data *mpsleep = new mpsleep_data;

         /*
          * State variables 
          */
         mpsleep->ignorelevel = ignorelevel;
         mpsleep->iflevel = iflevel;
         int count2;
         for( count = 0; count < MAX_IFS; ++count )
         {
            for( count2 = 0; count2 <= DO_ELSE; ++count2 )
            {
               mpsleep->ifstate[count][count2] = ifstate[count][count2];
            }
         }

         /*
          * Driver arguments 
          */
         mpsleep->com_list = STRALLOC( command_list );
         mpsleep->mob = mob;
         mpsleep->actor = actor;
         mpsleep->obj = obj;
         mpsleep->victim = victim;
         mpsleep->target = target;
         mpsleep->single_step = single_step;

         /*
          * Time to sleep 
          */
         char arg[MIL];
         cmnd = one_argument( cmnd, arg );
         cmnd = one_argument( cmnd, arg );
         if( arg[0] == '\0' )
            mpsleep->timer = 4;
         else
            mpsleep->timer = atoi( arg );

         if( mpsleep->timer < 1 )
         {
            progbug( "mpsleep - bad arg, using default", mob );
            mpsleep->timer = 4;
         }

         /*
          * Save type of prog, room, object or mob 
          */
         if( mpsleep->mob->pIndexData->vnum == MOB_VNUM_SUPERMOB )
         {
            if( !str_prefix( "Room", mpsleep->mob->chardesc ) )
            {
               mpsleep->type = MP_ROOM;
               mpsleep->room = mpsleep->mob->in_room;
            }
            else if( !str_prefix( "Object", mpsleep->mob->chardesc ) )
               mpsleep->type = MP_OBJ;
         }
         else
            mpsleep->type = MP_MOB;

         sleeplist.push_back( mpsleep );

         --prog_nest;
         return;
      }

      /*
       * Evaluate/execute the command, check what happened. 
       */
      int result = mprog_do_command( cmnd, mob, actor, obj, victim, target, rndm,
                                     ( ifstate[iflevel][IN_IF] && !ifstate[iflevel][DO_IF] )
                                     || ( ifstate[iflevel][IN_ELSE] && !ifstate[iflevel][DO_ELSE] ), ( ignorelevel > 0 ) );

      if( rprog_oprog )
         uphold_supermob( &curr_serial, serial, &supermob_room, true_supermob_obj );

      /*
       * Script prog support - Thoric 
       */
      if( single_step )
      {
         mob->mpscriptpos = command_list - tmpcmndlst;
         --prog_nest;
         return;
      }

      /*
       * This is the complicated part.  Act on the returned value from
       * mprog_do_command according to the current logic state. 
       */
      switch ( result )
      {
         case COMMANDOK:
            /*
             * Ok, this one's a no-brainer. 
             */
            continue;

         case IFTRUE:
            /*
             * An if was evaluated and found true.  Note that we are in an
             * if section and that we want to execute it. 
             */
            ++iflevel;
            if( iflevel == MAX_IFS )
            {
               progbug( "Maximum nested ifs exceeded", mob );
               --prog_nest;
               return;
            }

            ifstate[iflevel][IN_IF] = true;
            ifstate[iflevel][DO_IF] = true;
            break;

         case IFFALSE:
            /*
             * An if was evaluated and found false.  Note that we are in an
             * if section and that we don't want to execute it unless we find
             * an or that evaluates to true. 
             */
            ++iflevel;
            if( iflevel == MAX_IFS )
            {
               progbug( "Maximum nested ifs exceeded", mob );
               --prog_nest;
               return;
            }
            ifstate[iflevel][IN_IF] = true;
            ifstate[iflevel][DO_IF] = false;
            break;

         case ORTRUE:
            /*
             * An or was evaluated and found true.  We should already be in an
             * if section, so note that we want to execute it. 
             */
            if( !ifstate[iflevel][IN_IF] )
            {
               progbug( "Unmatched or", mob );
               --prog_nest;
               return;
            }
            ifstate[iflevel][DO_IF] = true;
            break;

         case ORFALSE:
            /*
             * An or was evaluated and found false.  We should already be in an
             * if section, and we don't need to do much.  If the if was true or
             * there were/will be other ors that evaluate(d) to true, they'll set
             * do_if to true. 
             */
            if( !ifstate[iflevel][IN_IF] )
            {
               progbug( "Unmatched or", mob );
               --prog_nest;
               return;
            }
            continue;

         case FOUNDELSE:
            /*
             * Found an else.  Make sure we're in an if section, bug out if not.
             * If this else is not one that we wish to ignore, note that we're now 
             * in an else section, and look at whether or not we executed the if 
             * section to decide whether to execute the else section.  Ca marche 
             * bien. 
             */
            if( ignorelevel > 0 )
               continue;

            if( ifstate[iflevel][IN_ELSE] )
            {
               progbug( "Found else in an else section", mob );
               --prog_nest;
               return;
            }
            if( !ifstate[iflevel][IN_IF] )
            {
               progbug( "Unmatched else", mob );
               --prog_nest;
               return;
            }

            ifstate[iflevel][IN_ELSE] = true;
            ifstate[iflevel][DO_ELSE] = !ifstate[iflevel][DO_IF];
            ifstate[iflevel][IN_IF] = false;
            ifstate[iflevel][DO_IF] = false;

            break;

         case FOUNDENDIF:
            /*
             * Hmm, let's see... FOUNDENDIF must mean that we found an endif.
             * So let's make sure we were expecting one, return if not.  If this
             * endif matches the if or else that we're executing, note that we are 
             * now no longer executing an if.  If not, keep track of what we're 
             * ignoring. 
             */
            if( !( ifstate[iflevel][IN_IF] || ifstate[iflevel][IN_ELSE] ) )
            {
               progbug( "Unmatched endif", mob );
               --prog_nest;
               return;
            }

            if( ignorelevel > 0 )
            {
               --ignorelevel;
               continue;
            }

            ifstate[iflevel][IN_IF] = false;
            ifstate[iflevel][DO_IF] = false;
            ifstate[iflevel][IN_ELSE] = false;
            ifstate[iflevel][DO_ELSE] = false;

            --iflevel;
            break;

         case IFIGNORED:
            if( !( ifstate[iflevel][IN_IF] || ifstate[iflevel][IN_ELSE] ) )
            {
               progbug( "Parse error, ignoring if while not in if or else", mob );
               --prog_nest;
               return;
            }
            ++ignorelevel;
            break;

         case ORIGNORED:
            if( !( ifstate[iflevel][IN_IF] || ifstate[iflevel][IN_ELSE] ) )
            {
               progbug( "Unmatched or", mob );
               --prog_nest;
               return;
            }
            if( ignorelevel == 0 )
            {
               progbug( "Parse error, mistakenly ignoring or", mob );
               --prog_nest;
               return;
            }
            break;

         default:
         case BERR:
            --prog_nest;
            return;
      }
   }
}

/* This function replaces mprog_process_cmnd.  It is called from 
 * mprog_driver, once for each line in a mud prog.  This function
 * checks what the line is, executes if/or checks and calls interpret
 * to perform the the commands.  Written by Narn, Dec 95.
 */
int mprog_do_command( char *cmnd, char_data * mob, char_data * actor, obj_data * obj, char_data * victim, obj_data * target, char_data * rndm, bool ignore, bool ignore_ors )
{
   char firstword[MIL];
   char *ifcheck;
   char buf[MIL];
   char tmp[MIL];
   char *point, *str, *i;
   int validif, vnum;

   /*
    * Isolate the first word of the line, it gives us a clue what
    * we want to do. 
    */
   ifcheck = one_argument( cmnd, firstword );

   if( !str_cmp( firstword, "if" ) )
   {
      /*
       * Ok, we found an if.  According to the boolean 'ignore', either
       * ignore the ifcheck and report that back to mprog_driver or do
       * the ifcheck and report whether it was successful. 
       */
      if( ignore )
         return IFIGNORED;
      else
         validif = mprog_do_ifcheck( ifcheck, mob, actor, obj, victim, target, rndm );

      if( validif == 1 )
         return IFTRUE;

      if( validif == 0 )
         return IFFALSE;

      return BERR;
   }

   if( !str_cmp( firstword, "or" ) )
   {
      /*
       * Same behavior as with ifs, but use the boolean 'ignore_ors' to
       * decide which way to go. 
       */
      if( ignore_ors )
         return ORIGNORED;
      else
         validif = mprog_do_ifcheck( ifcheck, mob, actor, obj, victim, target, rndm );

      if( validif == 1 )
         return ORTRUE;

      if( validif == 0 )
         return ORFALSE;

      return BERR;
   }

   /*
    * For else and endif, just report back what we found.  Mprog_driver
    * keeps track of logiclevels. 
    */
   if( !str_cmp( firstword, "else" ) )
   {
      return FOUNDELSE;
   }

   if( !str_cmp( firstword, "endif" ) )
   {
      return FOUNDENDIF;
   }

   /*
    * Ok, didn't find an if, an or, an else or an endif.  
    * If the command is in an if or else section that is not to be 
    * performed, the boolean 'ignore' is set to true and we just 
    * return.  If not, we try to execute the command. 
    */

   if( ignore )
      return COMMANDOK;

   /*
    * If the command is 'break', that's all folks. 
    */
   if( !str_cmp( firstword, "break" ) )
      return BERR;

   vnum = mob->pIndexData->vnum;
   point = buf;
   str = cmnd;

   /*
    * This chunk of code taken from mprog_process_cmnd. 
    */
   while( *str != '\0' )
   {
      if( *str != '$' )
      {
         *point++ = *str++;
         continue;
      }
      ++str;
      mprog_translate( *str, tmp, mob, actor, obj, victim, target, rndm );
      i = tmp;
      ++str;
      while( ( *point = *i ) != '\0' )
         ++point, ++i;
   }
   *point = '\0';

   interpret( mob, buf );

   /*
    * If the mob is mentally unstable and does things like fireball
    * itself, let's make sure it's still alive. 
    */
   if( mob->char_died(  ) )
   {
      bug( "%s: Mob died while executing program, vnum %d.", __FUNCTION__, vnum );
      return BERR;
   }

   return COMMANDOK;
}

/***************************************************************************
 * Global function code and brief comments.
 */

/* See if there's any mud programs waiting to be continued -rkb */
void mpsleep_update( void )
{
   list < mpsleep_data * >::iterator tmpmpsleep;
   mpsleep_data *mpsleep;
   bool delete_it;

   for( tmpmpsleep = sleeplist.begin(  ); tmpmpsleep != sleeplist.end(  ); )
   {
      mpsleep = *tmpmpsleep;
      ++tmpmpsleep;
      delete_it = false;

      if( mpsleep->mob )
         delete_it = mpsleep->mob->char_died(  );

      if( mpsleep->actor && !delete_it )
         delete_it = mpsleep->actor->char_died(  );

      if( mpsleep->obj && !delete_it )
         delete_it = mpsleep->obj->extracted(  );

      if( mpsleep->actor )
      {
         if( ( mpsleep->mob ) && ( mpsleep->actor->in_room != mpsleep->mob->in_room ) )
            delete_it = true;
         if( ( mpsleep->obj ) && ( mpsleep->actor->in_room != mpsleep->obj->in_room ) )
            delete_it = true;
         if( ( mpsleep->room ) && ( mpsleep->actor->in_room->vnum != mpsleep->room->vnum ) )
            delete_it = true;
      }

      if( delete_it )
      {
         log_string( "mpsleep_update - Deleting expired prog." );

         deleteptr( mpsleep );
         continue;
      }
   }

   for( tmpmpsleep = sleeplist.begin(  ); tmpmpsleep != sleeplist.end(  ); )
   {
      mpsleep = *tmpmpsleep;
      ++tmpmpsleep;

      if( --mpsleep->timer <= 0 )
      {
         current_mpsleep = mpsleep;

         if( mpsleep->type == MP_ROOM )
            rset_supermob( mpsleep->room );
         else if( mpsleep->type == MP_OBJ )
            set_supermob( mpsleep->obj );

         mprog_driver( mpsleep->com_list, mpsleep->mob, mpsleep->actor, mpsleep->obj, mpsleep->victim, mpsleep->target, mpsleep->single_step );

         release_supermob(  );

         deleteptr( mpsleep );
         continue;
      }
   }
}

bool mprog_keyword_check( const string & argu, const char *argl )
{
   char word[MIL], arg1[MIL], arg2[MIL];
   char *arg, *arglist, *start, *end;

   strcpy( arg1, argu.c_str(  ) );
   arg = strlower( arg1 );

   strcpy( arg2, argl );
   arglist = strlower( arg2 );

   if( ( arglist[0] == 'p' ) && ( arglist[1] == ' ' ) )
   {
      arglist += 2;
      while( ( start = strstr( arg, arglist ) ) )
         if( ( start == arg || *( start - 1 ) == ' ' ) && ( *( end = start + strlen( arglist ) ) == ' ' || *end == '\n' || *end == '\r' || *end == '\0' ) )
            return true;
         else
            arg = start + 1;
   }
   else
   {
      arglist = one_argument( arglist, word );
      for( ; word[0] != '\0'; arglist = one_argument( arglist, word ) )
         while( ( start = strstr( arg, word ) ) )
            if( ( start == arg || *( start - 1 ) == ' ' ) && ( *( end = start + strlen( word ) ) == ' ' || *end == '\n' || *end == '\r' || *end == '\0' ) )
               return true;
            else
               arg = start + 1;
   }
   return false;
}

/* The next two routines are the basic trigger types. Either trigger
 *  on a certain percent, or trigger on a keyword or word phrase.
 *  To see how this works, look at the various trigger routines..
 */
bool mprog_and_wordlist_check( const string & arg, char_data * mob, char_data * actor, obj_data * obj, char_data * victim, obj_data * target, int type )
{
   char temp1[MSL], temp2[MIL], word[MIL];
   list < mud_prog_data * >::iterator mpg;
   mud_prog_data *mprg;
   char *list, *start, *dupl, *end;
   size_t j, i;
   bool executed = false;

   for( mpg = mob->pIndexData->mudprogs.begin(  ); mpg != mob->pIndexData->mudprogs.end(  ); ++mpg )
   {
      mprg = *mpg;

      if( mprg->type == type )
      {
         strcpy( temp1, mprg->arglist );
         list = temp1;
         for( i = 0; i < strlen( list ); ++i )
            list[i] = LOWER( list[i] );

         strcpy( temp2, arg.c_str(  ) );
         dupl = temp2;

         for( i = 0; i < strlen( dupl ); ++i )
            dupl[i] = LOWER( dupl[i] );
         {
            j = 0;
            for( i = 0; i < strlen( list ); ++i )
            {
               if( isspace( list[i] ) )
                  ++j;
            }
            ++j;
            list = one_argument( list, word );
            i = 0;
            for( ; word[0] != '\0'; list = one_argument( list, word ) )
            {
               if( ( start = strstr( dupl, word ) ) )
               {
                  if( ( start == dupl || *( start - 1 ) == ' ' ) && ( *( end = start + strlen( word ) ) == ' ' || *end == '\n' || *end == '\r' || *end == '\0' ) )
                     ++i;
                  if( i == j )
                  {
                     mprog_driver( mprg->comlist, mob, actor, obj, victim, target, false );
                     executed = true;
                     break;
                  }
               }
            }
         }
      }
   }
   return executed;
}

bool mprog_wordlist_check( const string & arg, char_data * mob, char_data * actor, obj_data * obj, char_data * victim, obj_data * target, int type )
{
   char temp1[MSL], temp2[MIL], word[MIL];
   list < mud_prog_data * >::iterator mpg;
   mud_prog_data *mprg;
   char *list, *start, *dupl, *end;
   size_t i;
   bool executed = false;

   for( mpg = mob->pIndexData->mudprogs.begin(  ); mpg != mob->pIndexData->mudprogs.end(  ); ++mpg )
   {
      mprg = *mpg;

      if( mprg->type == type )
      {
         strcpy( temp1, mprg->arglist );
         list = temp1;
         for( i = 0; i < strlen( list ); ++i )
            list[i] = LOWER( list[i] );

         strcpy( temp2, arg.c_str(  ) );
         dupl = temp2;
         for( i = 0; i < strlen( dupl ); ++i )
            dupl[i] = LOWER( dupl[i] );

         if( ( list[0] == 'p' ) && ( list[1] == ' ' ) )
         {
            list += 2;
            while( ( start = strstr( dupl, list ) ) )
               if( ( start == dupl || *( start - 1 ) == ' ' ) && ( *( end = start + strlen( list ) ) == ' ' || *end == '\n' || *end == '\r' || *end == '\0' ) )
               {
                  mprog_driver( mprg->comlist, mob, actor, obj, victim, target, false );
                  executed = true;
                  break;
               }
               else
                  dupl = start + 1;
         }
         else
         {
            list = one_argument( list, word );
            for( ; word[0] != '\0'; list = one_argument( list, word ) )
               while( ( start = strstr( dupl, word ) ) )
                  if( ( start == dupl || *( start - 1 ) == ' ' ) && ( *( end = start + strlen( word ) ) == ' ' || *end == '\n' || *end == '\r' || *end == '\0' ) )
                  {
                     mprog_driver( mprg->comlist, mob, actor, obj, victim, target, false );
                     executed = true;
                     break;
                  }
                  else
                     dupl = start + 1;
         }
      }
   }
   return executed;
}

bool oprog_and_wordlist_check( const string & arg, char_data * mob, char_data * actor, obj_data * obj, char_data * victim, obj_data * target, int type, obj_data * iobj )
{
   char temp1[MSL], temp2[MIL], word[MIL];
   list < mud_prog_data * >::iterator mpg;
   mud_prog_data *mprg;
   char *list, *start, *dupl, *end;
   size_t i, j;
   bool executed = false;

   for( mpg = iobj->pIndexData->mudprogs.begin(  ); mpg != iobj->pIndexData->mudprogs.end(  ); ++mpg )
   {
      mprg = *mpg;

      if( mprg->type == type )
      {
         strcpy( temp1, mprg->arglist );
         list = temp1;
         for( i = 0; i < strlen( list ); ++i )
            list[i] = LOWER( list[i] );

         strcpy( temp2, arg.c_str(  ) );
         dupl = temp2;
         for( i = 0; i < strlen( dupl ); ++i )
            dupl[i] = LOWER( dupl[i] );

         j = 0;
         for( i = 0; i < strlen( list ); ++i )
         {
            if( isspace( list[i] ) )
               ++j;
         }
         ++j;
         list = one_argument( list, word );
         i = 0;
         for( ; word[0] != '\0'; list = one_argument( list, word ) )
            if( ( start = strstr( dupl, word ) ) )
               if( ( start == dupl || *( start - 1 ) == ' ' ) && ( *( end = start + strlen( word ) ) == ' ' || *end == '\n' || *end == '\r' || *end == '\0' ) )
                  ++i;
         if( i == j )
         {
            set_supermob( iobj );
            mprog_driver( mprg->comlist, mob, actor, obj, victim, target, false );
            release_supermob(  );
            executed = true;
            break;
         }
      }
   }
   return executed;
}

bool oprog_wordlist_check( const string & arg, char_data * mob, char_data * actor, obj_data * obj, char_data * victim, obj_data * target, int type, obj_data * iobj )
{
   char temp1[MSL], temp2[MIL], word[MIL];
   list < mud_prog_data * >::iterator mpg;
   mud_prog_data *mprg;
   char *list, *start, *dupl, *end;
   size_t i;
   bool executed = false;

   for( mpg = iobj->pIndexData->mudprogs.begin(  ); mpg != iobj->pIndexData->mudprogs.end(  ); ++mpg )
   {
      mprg = *mpg;

      if( mprg->type == type )
      {
         strcpy( temp1, mprg->arglist );
         list = temp1;
         for( i = 0; i < strlen( list ); ++i )
            list[i] = LOWER( list[i] );

         strcpy( temp2, arg.c_str(  ) );
         dupl = temp2;
         for( i = 0; i < strlen( dupl ); ++i )
            dupl[i] = LOWER( dupl[i] );

         if( ( list[0] == 'p' ) && ( list[1] == ' ' ) )
         {
            list += 2;
            while( ( start = strstr( dupl, list ) ) )
               if( ( start == dupl || *( start - 1 ) == ' ' ) && ( *( end = start + strlen( list ) ) == ' ' || *end == '\n' || *end == '\r' || *end == '\0' ) )
               {
                  set_supermob( iobj );
                  mprog_driver( mprg->comlist, mob, actor, obj, victim, target, false );
                  executed = true;
                  release_supermob(  );
                  break;
               }
               else
                  dupl = start + 1;
         }
         else
         {
            list = one_argument( list, word );
            for( ; word[0] != '\0'; list = one_argument( list, word ) )
               while( ( start = strstr( dupl, word ) ) )
                  if( ( start == dupl || *( start - 1 ) == ' ' ) && ( *( end = start + strlen( word ) ) == ' ' || *end == '\n' || *end == '\r' || *end == '\0' ) )
                  {
                     set_supermob( iobj );
                     mprog_driver( mprg->comlist, mob, actor, obj, victim, target, false );
                     executed = true;
                     release_supermob(  );
                     break;
                  }
                  else
                     dupl = start + 1;
         }
      }
   }
   return executed;
}

bool rprog_and_wordlist_check( const string & arg, char_data * mob, char_data * actor, obj_data * obj, char_data * victim, obj_data * target, int type, room_index * room )
{
   char temp1[MSL], temp2[MIL], word[MIL];
   list < mud_prog_data * >::iterator mpg;
   mud_prog_data *mprg;
   char *list, *start, *dupl, *end;
   size_t i, j;
   bool executed = false;

   if( actor && !actor->char_died(  ) && actor->in_room )
      room = actor->in_room;

   for( mpg = room->mudprogs.begin(  ); mpg != room->mudprogs.end(  ); ++mpg )
   {
      mprg = *mpg;

      if( mprg->type == type )
      {
         strcpy( temp1, mprg->arglist );
         list = temp1;
         for( i = 0; i < strlen( list ); ++i )
            list[i] = LOWER( list[i] );

         strcpy( temp2, arg.c_str(  ) );
         dupl = temp2;
         for( i = 0; i < strlen( dupl ); ++i )
            dupl[i] = LOWER( dupl[i] );

         j = 0;
         for( i = 0; i < strlen( list ); ++i )
         {
            if( isspace( list[i] ) )
            {
               ++j;
            }
         }
         ++j;
         list = one_argument( list, word );
         i = 0;
         for( ; word[0] != '\0'; list = one_argument( list, word ) )
            if( ( start = strstr( dupl, word ) ) )
               if( ( start == dupl || *( start - 1 ) == ' ' ) && ( *( end = start + strlen( word ) ) == ' ' || *end == '\n' || *end == '\r' || *end == '\0' ) )
                  ++i;
         if( i == j )
         {
            rset_supermob( room );
            mprog_driver( mprg->comlist, mob, actor, obj, victim, target, false );
            release_supermob(  );
            executed = true;
            break;
         }
      }
   }
   return executed;
}

bool rprog_wordlist_check( const string & arg, char_data * mob, char_data * actor, obj_data * obj, char_data * victim, obj_data * target, int type, room_index * room )
{
   char temp1[MSL], temp2[MIL], word[MIL];
   list < mud_prog_data * >::iterator mpg;
   mud_prog_data *mprg;
   char *list, *start, *dupl, *end;
   size_t i;
   bool executed = false;

   if( actor && !actor->char_died(  ) && actor->in_room )
      room = actor->in_room;

   for( mpg = room->mudprogs.begin(  ); mpg != room->mudprogs.end(  ); ++mpg )
   {
      mprg = *mpg;

      if( mprg->type == type )
      {
         strcpy( temp1, mprg->arglist );
         list = temp1;
         for( i = 0; i < strlen( list ); ++i )
            list[i] = LOWER( list[i] );

         strcpy( temp2, arg.c_str(  ) );
         dupl = temp2;
         for( i = 0; i < strlen( dupl ); ++i )
            dupl[i] = LOWER( dupl[i] );

         if( ( list[0] == 'p' ) && ( list[1] == ' ' ) )
         {
            list += 2;
            while( ( start = strstr( dupl, list ) ) )
               if( ( start == dupl || *( start - 1 ) == ' ' ) && ( *( end = start + strlen( list ) ) == ' ' || *end == '\n' || *end == '\r' || *end == '\0' ) )
               {
                  rset_supermob( room );
                  mprog_driver( mprg->comlist, mob, actor, obj, victim, target, false );
                  executed = true;
                  release_supermob(  );
                  break;
               }
               else
                  dupl = start + 1;
         }
         else
         {
            list = one_argument( list, word );
            for( ; word[0] != '\0'; list = one_argument( list, word ) )
               while( ( start = strstr( dupl, word ) ) )
                  if( ( start == dupl || *( start - 1 ) == ' ' ) && ( *( end = start + strlen( word ) ) == ' ' || *end == '\n' || *end == '\r' || *end == '\0' ) )
                  {
                     rset_supermob( room );
                     mprog_driver( mprg->comlist, mob, actor, obj, victim, target, false );
                     executed = true;
                     release_supermob(  );
                     break;
                  }
                  else
                     dupl = start + 1;
         }
      }
   }
   return executed;
}

void mprog_percent_check( char_data * mob, char_data * actor, obj_data * obj, char_data * victim, obj_data * target, int type )
{
   list < mud_prog_data * >::iterator mpg;
   mud_prog_data *mprg;

   for( mpg = mob->pIndexData->mudprogs.begin(  ); mpg != mob->pIndexData->mudprogs.end(  ); ++mpg )
   {
      mprg = *mpg;

      if( ( mprg->type == type ) && mprg->arglist != NULL && ( number_percent(  ) <= atoi( mprg->arglist ) ) )
      {
         mprog_driver( mprg->comlist, mob, actor, obj, victim, target, false );
         if( type != GREET_PROG && type != ALL_GREET_PROG && type != LOGIN_PROG && type != VOID_PROG )
            break;
      }
   }
}

void mprog_time_check( char_data * mob, char_data * actor, obj_data * obj, char_data * victim, obj_data * target, int type )
{
   bool trigger_time;
   list < mud_prog_data * >::iterator mpg;
   mud_prog_data *mprg;

   for( mpg = mob->pIndexData->mudprogs.begin(  ); mpg != mob->pIndexData->mudprogs.end(  ); ++mpg )
   {
      mprg = *mpg;

      trigger_time = ( time_info.hour == atoi( mprg->arglist ) );

      if( !trigger_time )
      {
         if( mprg->triggered )
            mprg->triggered = false;
         continue;
      }

      if( ( mprg->type == type ) && ( ( !mprg->triggered ) || ( mprg->type == HOUR_PROG ) ) )
      {
         mprg->triggered = true;
         mprog_driver( mprg->comlist, mob, actor, obj, victim, target, false );
      }
   }
}

// A much simpler version than the old mess - Samson 8/2/05
void mob_act_add( char_data * mob )
{
   list < char_data * >::iterator ich;

   for( ich = mob_act_list.begin(  ); ich != mob_act_list.end(  ); ++ich )
   {
      char_data *ch = *ich;

      if( ch == mob )
         return;
   }
   mob_act_list.push_back( mob );
}

/* The triggers.. These are really basic, and since most appear only
 * once in the code (hmm. i think they all do) it would be more efficient
 * to substitute the code in and make the mprog_xxx_check routines global.
 * However, they are all here in one nice place at the moment to make it
 * easier to see what they look like. If you do substitute them back in,
 * make sure you remember to modify the variable names to the ones in the
 * trigger calls.
 */
void mprog_act_trigger( const string & buf, char_data * mob, char_data * ch, obj_data * obj, char_data * victim, obj_data * target )
{
   mprog_act_list *tmp_act;
   list < mud_prog_data * >::iterator mpg;
   mud_prog_data *mprg;
   bool found = false;

   if( mob->isnpc(  ) && HAS_PROG( mob->pIndexData, ACT_PROG ) )
   {
      /*
       * Don't let a mob trigger itself, nor one instance of a mob trigger another instance. 
       */
      if( ch->isnpc(  ) && ch->pIndexData == mob->pIndexData )
         return;

      /*
       * make sure this is a matching trigger 
       */
      for( mpg = mob->pIndexData->mudprogs.begin(  ); mpg != mob->pIndexData->mudprogs.end(  ); ++mpg )
      {
         mprg = *mpg;

         if( mprg->type == ACT_PROG && mprog_keyword_check( buf, mprg->arglist ) )
         {
            found = true;
            break;
         }
      }
      if( !found )
         return;

      tmp_act = new mprog_act_list;
      tmp_act->buf = buf;
      tmp_act->ch = ch;
      tmp_act->obj = obj;
      tmp_act->victim = victim;
      tmp_act->target = target;
      ++mob->mpactnum;
      mob->mpact.push_back( tmp_act );
      mob_act_add( mob );
   }
}

bool mprog_keyword_trigger( const string & txt, char_data * actor )
{
   bool rValue = false;

   if( HAS_PROG( actor->in_room, KEYWORD_PROG ) )
   {
      list < mud_prog_data * >::iterator mpg;

      for( mpg = actor->in_room->mudprogs.begin(  ); mpg != actor->in_room->mudprogs.end(  ); ++mpg )
      {
         mud_prog_data *mprg = *mpg;

         if( actor->char_died(  ) )
            return rValue;

         if( ( mprg->type = KEYWORD_PROG ) && ( !str_cmp( mprg->arglist, txt ) ) )
         {
            rset_supermob( actor->in_room );
            mprog_driver( mprg->comlist, supermob, actor, NULL, NULL, NULL, false );
            release_supermob(  );
            rValue = true;
         }
      }
   }

   if( actor->char_died(  ) )
      return rValue;

   list < char_data * >::iterator ich;
   for( ich = actor->in_room->people.begin(  ); ich != actor->in_room->people.end(  ); )
   {
      char_data *vmob = *ich;
      ++ich;

      if( vmob->isnpc(  ) && HAS_PROG( vmob->pIndexData, KEYWORD_PROG ) )
      {
         if( actor->isnpc(  ) && actor->pIndexData == vmob->pIndexData )
            continue;

         list < mud_prog_data * >::iterator mpg;
         for( mpg = vmob->pIndexData->mudprogs.begin(  ); mpg != vmob->pIndexData->mudprogs.end(  ); ++mpg )
         {
            mud_prog_data *mprg = *mpg;

            if( actor->char_died(  ) )
               return rValue;

            if( ( mprg->type == KEYWORD_PROG ) && ( !str_cmp( mprg->arglist, txt ) ) )
            {
               mprog_driver( mprg->comlist, vmob, actor, NULL, NULL, NULL, false );
               rValue = true;
            }
         }
      }
   }

   if( actor->char_died(  ) )
      return rValue;

   list < obj_data * >::iterator iobj;
   for( iobj = actor->in_room->objects.begin(  ); iobj != actor->in_room->objects.end(  ); )
   {
      obj_data *vobj = *iobj;
      ++iobj;

      if( HAS_PROG( vobj->pIndexData, KEYWORD_PROG ) )
      {
         list < mud_prog_data * >::iterator mpg;
         char *buf = NULL;

         for( mpg = vobj->pIndexData->mudprogs.begin(  ); mpg != vobj->pIndexData->mudprogs.end(  ); ++mpg )
         {
            mud_prog_data *mprg = *mpg;

            if( actor->char_died(  ) )
               return rValue;

            if( mprg->type != KEYWORD_PROG )
               continue;

            STRFREE( buf );
            buf = STRALLOC( mprg->arglist );
            /*
             * Allow for room or inv only triggers 
             */
            if( mprg->arglist[1] == ')' )
            {
               if( mprg->arglist[0] == 'r' )
               {
                  STRFREE( buf );
                  buf = STRALLOC( mprg->arglist + 2 );
               }
               else
                  continue;
            }

            if( !str_cmp( buf, txt ) )
            {
               set_supermob( vobj );
               mprog_driver( mprg->comlist, supermob, actor, vobj, NULL, NULL, false );
               release_supermob(  );
               rValue = true;
            }
         }
      }
   }
   if( actor->char_died(  ) )
      return rValue;

   for( iobj = actor->carrying.begin(  ); iobj != actor->carrying.end(  ); )
   {
      obj_data *vobj = *iobj;
      ++iobj;

      if( HAS_PROG( vobj->pIndexData, KEYWORD_PROG ) )
      {
         list < mud_prog_data * >::iterator mpg;
         char *buf = NULL;

         for( mpg = vobj->pIndexData->mudprogs.begin(  ); mpg != vobj->pIndexData->mudprogs.end(  ); ++mpg )
         {
            mud_prog_data *mprg = *mpg;

            if( actor->char_died(  ) )
               return rValue;

            if( mprg->type != KEYWORD_PROG )
               continue;
            STRFREE( buf );
            buf = STRALLOC( mprg->arglist );
            /*
             * Allow for room or inv only triggers 
             */
            if( mprg->arglist[1] == ')' )
            {
               if( mprg->arglist[0] == 'i' && vobj->wear_loc == -1 )
               {
                  STRFREE( buf );
                  buf = STRALLOC( mprg->arglist + 2 );
               }
               else if( mprg->arglist[0] == 'e' && vobj->wear_loc > -1 )
               {
                  STRFREE( buf );
                  buf = STRALLOC( mprg->arglist + 2 );
               }
               else if( mprg->arglist[0] == 'c' )
               {
                  STRFREE( buf );
                  buf = STRALLOC( mprg->arglist + 2 );
               }
               else
                  continue;
            }

            if( !str_cmp( buf, txt ) )
            {
               set_supermob( vobj );
               mprog_driver( mprg->comlist, supermob, actor, vobj, NULL, NULL, false );
               release_supermob(  );
               rValue = true;
            }
         }
      }
   }
   return rValue;
}

void mprog_bribe_trigger( char_data * mob, char_data * ch, int amount )
{
   list < mud_prog_data * >::iterator mpg;
   mud_prog_data *mprg, *tprg = NULL;
   obj_data *obj;

   if( mob->isnpc(  ) && mob->can_see( ch, false ) && HAS_PROG( mob->pIndexData, BRIBE_PROG ) )
   {
      /*
       * Don't let a mob trigger itself, nor one instance of a mob trigger another instance. 
       */
      if( ch->isnpc(  ) && ch->pIndexData == mob->pIndexData )
         return;

      if( !( obj = get_obj_index( OBJ_VNUM_MONEY_SOME )->create_object( 1 ) ) )
      {
         log_printf( "create_object: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
         return;
      }
      stralloc_printf( &obj->short_descr, obj->short_descr, amount );
      obj->value[0] = amount;
      obj = obj->to_char( mob );
      mob->gold -= amount;

      for( mpg = mob->pIndexData->mudprogs.begin(  ); mpg != mob->pIndexData->mudprogs.end(  ); ++mpg )
      {
         mprg = *mpg;

         if( ( mprg->type == BRIBE_PROG ) && ( amount >= atoi( mprg->arglist ) ) )
         {
            if( tprg )
            {	
               if( atoi( tprg->arglist ) < atoi( mprg->arglist ) )
                  tprg = mprg; 
            }
            else
               tprg = mprg;
         }

         if( tprg )
            mprog_driver( tprg->comlist, mob, ch, obj, NULL, NULL, false );
      }
   }
}

void mprog_death_trigger( char_data * killer, char_data * mob )
{
   if( mob->isnpc(  ) && killer != mob && HAS_PROG( mob->pIndexData, DEATH_PROG ) )
   {
      mob->position = POS_STANDING;
      mprog_percent_check( mob, killer, NULL, NULL, NULL, DEATH_PROG );
      mob->position = POS_DEAD;
   }
}

/* login and void mob triggers by Edmond */
void mprog_login_trigger( char_data *ch )
{
   list < char_data * >::iterator ich;

   for( ich = ch->in_room->people.begin(  ); ich != ch->in_room->people.end(  ); )
   {
      char_data *vmob = *ich;
      ++ich;

      if( !vmob->isnpc() || !vmob->can_see( ch, false ) || vmob->fighting || !vmob->IS_AWAKE() )
         continue;

      if( ch->isnpc() && ch->pIndexData == vmob->pIndexData )
         continue;

      if( HAS_PROG( vmob->pIndexData, LOGIN_PROG ) )
         mprog_percent_check( vmob, ch, NULL, NULL, NULL, LOGIN_PROG );
   }
   return;
}

void mprog_void_trigger( char_data *ch )
{
   list < char_data * >::iterator ich;

   for( ich = ch->in_room->people.begin(  ); ich != ch->in_room->people.end(  ); )
   {
      char_data *vmob = *ich;
      ++ich;

      if( !vmob->isnpc() || !vmob->can_see( ch, false ) || vmob->fighting || !vmob->IS_AWAKE() )
         continue;

      if( ch->isnpc() && ch->pIndexData == vmob->pIndexData )
         continue;

      if( HAS_PROG( vmob->pIndexData, VOID_PROG ) )
         mprog_percent_check( vmob, ch, NULL, NULL, NULL, VOID_PROG );
   }
   return;
}

void mprog_entry_trigger( char_data * mob )
{
   if( mob->isnpc(  ) && HAS_PROG( mob->pIndexData, ENTRY_PROG ) )
      mprog_percent_check( mob, NULL, NULL, NULL, NULL, ENTRY_PROG );
}

void mprog_fight_trigger( char_data * mob, char_data * ch )
{
   if( mob->isnpc(  ) && HAS_PROG( mob->pIndexData, FIGHT_PROG ) )
      mprog_percent_check( mob, ch, NULL, NULL, NULL, FIGHT_PROG );
}

void mprog_give_trigger( char_data * mob, char_data * ch, obj_data * obj )
{
   char buf[MIL];
   list < mud_prog_data * >::iterator mpg;
   mud_prog_data *mprg;

   if( mob->isnpc(  ) && mob->can_see( ch, false ) && HAS_PROG( mob->pIndexData, GIVE_PROG ) )
   {
      /*
       * Don't let a mob trigger itself, nor one instance of a mob
       * trigger another instance. 
       */
      if( ch->isnpc(  ) && ch->pIndexData == mob->pIndexData )
         return;

      for( mpg = mob->pIndexData->mudprogs.begin(  ); mpg != mob->pIndexData->mudprogs.end(  ); ++mpg )
      {
         mprg = *mpg;

         one_argument( mprg->arglist, buf );

         if( mprg->type == GIVE_PROG && ( !str_cmp( obj->name, mprg->arglist ) || !str_cmp( "all", buf ) ) )
         {
            mprog_driver( mprg->comlist, mob, ch, obj, NULL, NULL, false );
            break;
         }
      }
   }
}

void mprog_sell_trigger( char_data * mob, char_data * ch, obj_data * obj )
{
   char buf[MIL];
   int s_vnum;
   list < mud_prog_data * >::iterator mpg;
   mud_prog_data *mprg;

   if( mob->isnpc(  ) && mob->can_see( ch, false ) && HAS_PROG( mob->pIndexData, SELL_PROG ) )
   {
      if( ch->isnpc(  ) && ch->pIndexData == mob->pIndexData )
         return;

      for( mpg = mob->pIndexData->mudprogs.begin(  ); mpg != mob->pIndexData->mudprogs.end(  ); ++mpg )
      {
         mprg = *mpg;

         one_argument( mprg->arglist, buf );

         if( !is_number( buf ) )
            continue;

         s_vnum = atoi( buf );

         if( mprg->type == SELL_PROG && ( ( s_vnum == obj->pIndexData->vnum ) || ( s_vnum == 0 ) ) )
         {
            mprog_driver( mprg->comlist, mob, ch, obj, NULL, NULL, false );
            break;
         }
      }
   }
}

void mprog_tell_trigger( const string & txt, char_data * actor )
{
   list < char_data * >::iterator ich;

   for( ich = actor->in_room->people.begin(  ); ich != actor->in_room->people.end(  ); ++ich )
   {
      char_data *vmob = *ich;

      if( vmob->isnpc(  ) && HAS_PROG( vmob->pIndexData, TELL_PROG ) )
      {
         if( actor->isnpc(  ) && actor->pIndexData == vmob->pIndexData )
            continue;
         mprog_wordlist_check( txt, vmob, actor, NULL, NULL, NULL, TELL_PROG );
      }
   }
}

void mprog_and_tell_trigger( const string & txt, char_data * actor )
{
   list < char_data * >::iterator ich;

   for( ich = actor->in_room->people.begin(  ); ich != actor->in_room->people.end(  ); )
   {
      char_data *vmob = *ich;
      ++ich;

      if( actor == vmob )
         continue;

      if( actor->isnpc(  ) && actor->pIndexData == vmob->pIndexData )
         continue;

      // Band-aid fix to stop unknown crash
      if( actor->in_room != vmob->in_room )
         break;

      if( vmob->isnpc(  ) && HAS_PROG( vmob->pIndexData, TELL_AND_PROG ) )
         mprog_and_wordlist_check( txt, vmob, actor, NULL, NULL, NULL, TELL_AND_PROG );
   }
}

bool mprog_command_trigger( char_data * actor, const string & txt )
{
   list < char_data * >::iterator ich;

   for( ich = actor->in_room->people.begin(  ); ich != actor->in_room->people.end(  ); ++ich )
   {
      char_data *vmob = *ich;

      if( vmob->isnpc(  ) && HAS_PROG( vmob->pIndexData, CMD_PROG ) )
      {
         if( actor->isnpc(  ) && actor->pIndexData == vmob->pIndexData )
            continue;
         if( mprog_wordlist_check( txt, vmob, actor, NULL, NULL, NULL, CMD_PROG ) )
            return true;
      }
   }
   return false;
}

void mprog_greet_trigger( char_data * ch )
{
   if( !ch->in_room )
   {
      bug( "%s: ch '%s' not in room. Transferring to Limbo.", __FUNCTION__, ch->name );
      if( !ch->to_room( get_room_index( ROOM_VNUM_LIMBO ) ) )
         log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
      return;
   }

   list < char_data * >::iterator ich;
   for( ich = ch->in_room->people.begin(  ); ich != ch->in_room->people.end(  ); )
   {
      char_data *vmob = *ich;
      ++ich;

      if( ch == vmob )
         continue;

      // Band-aid fix to stop unknown crash
      if( ch->in_room != vmob->in_room )
         break;

      /*
       * Don't let a mob trigger itself, nor one instance of a mob
       * trigger another instance. 
       */
      if( ch->isnpc(  ) && ch->pIndexData == vmob->pIndexData )
         continue;

      if( !vmob->isnpc(  ) || !vmob->can_see( ch, false ) || vmob->fighting || !vmob->IS_AWAKE(  ) )
         continue;

      if( HAS_PROG( vmob->pIndexData, GREET_PROG ) )
         mprog_percent_check( vmob, ch, NULL, NULL, NULL, GREET_PROG );
      else if( HAS_PROG( vmob->pIndexData, ALL_GREET_PROG ) )
         mprog_percent_check( vmob, ch, NULL, NULL, NULL, ALL_GREET_PROG );
   }
}

void mprog_hitprcnt_trigger( char_data * mob, char_data * ch )
{
   list < mud_prog_data * >::iterator mpg;
   mud_prog_data *mprg;

   if( mob->isnpc(  ) && HAS_PROG( mob->pIndexData, HITPRCNT_PROG ) )
   {
      for( mpg = mob->pIndexData->mudprogs.begin(  ); mpg != mob->pIndexData->mudprogs.end(  ); ++mpg )
      {
         mprg = *mpg;

         if( mprg->type == HITPRCNT_PROG && ( 100 * mob->hit / mob->max_hit ) < atoi( mprg->arglist ) )
         {
            mprog_driver( mprg->comlist, mob, ch, NULL, NULL, NULL, false );
            break;
         }
      }
   }
}

void mprog_random_trigger( char_data * mob )
{
   if( HAS_PROG( mob->pIndexData, RAND_PROG ) )
      mprog_percent_check( mob, NULL, NULL, NULL, NULL, RAND_PROG );
}

void mprog_time_trigger( char_data * mob )
{
   if( HAS_PROG( mob->pIndexData, TIME_PROG ) )
      mprog_time_check( mob, NULL, NULL, NULL, NULL, TIME_PROG );
}

void mprog_hour_trigger( char_data * mob )
{
   if( HAS_PROG( mob->pIndexData, HOUR_PROG ) )
      mprog_time_check( mob, NULL, NULL, NULL, NULL, HOUR_PROG );
}

/* Added by Tarl 7-21-00 */
void mprog_and_speech_trigger( const string & txt, char_data * actor )
{
   list < char_data * >::iterator ich;

   for( ich = actor->in_room->people.begin(  ); ich != actor->in_room->people.end(  ); )
   {
      char_data *vmob = *ich;
      ++ich;

      if( actor == vmob )
         continue;

      if( actor->isnpc(  ) && actor->pIndexData == vmob->pIndexData )
         continue;

      // Band-aid fix to stop unknown crash
      if( actor->in_room != vmob->in_room )
         break;

      if( vmob->isnpc(  ) && HAS_PROG( vmob->pIndexData, SPEECH_AND_PROG ) )
      {
         mprog_and_wordlist_check( txt, vmob, actor, NULL, NULL, NULL, SPEECH_AND_PROG );
      }
   }
}

void mprog_targetted_speech_trigger( const string & txt, char_data * actor, char_data * victim )
{
   if( victim->isnpc(  ) && HAS_PROG( victim->pIndexData, SPEECH_PROG ) )
   {
      if( actor->isnpc(  ) && actor->pIndexData == victim->pIndexData )
         return;
      mprog_wordlist_check( txt, victim, actor, NULL, NULL, NULL, SPEECH_PROG );
   }
}

void mprog_speech_trigger( const string & txt, char_data * actor )
{
   list < char_data * >::iterator ich;
   room_index *room = actor->in_room;

   for( ich = room->people.begin(  ); ich != room->people.end(  ); )
   {
      char_data *vmob = *ich;
      ++ich;

      if( actor == vmob )
         continue;

      if( actor->isnpc(  ) && actor->pIndexData == vmob->pIndexData )
         continue;

      // Band-aid fix to stop unknown crash
      if( actor->in_room != vmob->in_room )
         break;

      if( vmob->isnpc(  ) && HAS_PROG( vmob->pIndexData, SPEECH_PROG ) )
         mprog_wordlist_check( txt, vmob, actor, NULL, NULL, NULL, SPEECH_PROG );
   }
}

void mprog_script_trigger( char_data * mob )
{
   list < mud_prog_data * >::iterator mpg;
   mud_prog_data *mprg;

   if( HAS_PROG( mob->pIndexData, SCRIPT_PROG ) )
   {
      for( mpg = mob->pIndexData->mudprogs.begin(  ); mpg != mob->pIndexData->mudprogs.end(  ); ++mpg )
      {
         mprg = *mpg;

         if( mprg->type == SCRIPT_PROG && ( mprg->arglist[0] == '\0' || mob->mpscriptpos != 0 || atoi( mprg->arglist ) == time_info.hour ) )
            mprog_driver( mprg->comlist, mob, NULL, NULL, NULL, NULL, true );
      }
   }
}

void oprog_script_trigger( obj_data * obj )
{
   list < mud_prog_data * >::iterator mpg;
   mud_prog_data *mprg;

   if( HAS_PROG( obj->pIndexData, SCRIPT_PROG ) )
   {
      for( mpg = obj->pIndexData->mudprogs.begin(  ); mpg != obj->pIndexData->mudprogs.end(  ); ++mpg )
      {
         mprg = *mpg;

         if( mprg->type == SCRIPT_PROG )
         {
            if( mprg->arglist[0] == '\0' || obj->mpscriptpos != 0 || atoi( mprg->arglist ) == time_info.hour )
            {
               set_supermob( obj );
               mprog_driver( mprg->comlist, supermob, NULL, NULL, NULL, NULL, true );
               obj->mpscriptpos = supermob->mpscriptpos;
               release_supermob(  );
            }
         }
      }
   }
}

bool oprog_command_trigger( char_data * ch, const string & txt )
{
   list < obj_data * >::iterator iobj;

   /*
    * supermob is set and released in oprog_wordlist_check 
    */
   for( iobj = ch->in_room->objects.begin(  ); iobj != ch->in_room->objects.end(  ); ++iobj )
   {
      obj_data *vobj = *iobj;

      if( HAS_PROG( vobj->pIndexData, CMD_PROG ) )
         if( oprog_wordlist_check( txt, supermob, ch, vobj, NULL, NULL, CMD_PROG, vobj ) )
            return true;
   }
   return false;
}

void rprog_script_trigger( room_index * room )
{
   list < mud_prog_data * >::iterator mpg;
   mud_prog_data *mprg;

   if( HAS_PROG( room, SCRIPT_PROG ) )
   {
      for( mpg = room->mudprogs.begin(  ); mpg != room->mudprogs.end(  ); ++mpg )
      {
         mprg = ( *mpg );

         if( mprg->type == SCRIPT_PROG )
         {
            if( mprg->arglist[0] == '\0' || room->mpscriptpos != 0 || atoi( mprg->arglist ) == time_info.hour )
            {
               rset_supermob( room );
               mprog_driver( mprg->comlist, supermob, NULL, NULL, NULL, NULL, true );
               room->mpscriptpos = supermob->mpscriptpos;
               release_supermob(  );
            }
         }
      }
   }
}

bool rprog_command_trigger( char_data * ch, const string & txt )
{
   if( HAS_PROG( ch->in_room, CMD_PROG ) )
   {
      /*
       * supermob is set and released in rprog_wordlist_check 
       */
      if( rprog_wordlist_check( txt, supermob, ch, NULL, NULL, NULL, CMD_PROG, ch->in_room ) )
         return true;
   }
   return false;
}

/*
 *  Mudprogram additions begin here
 */
void set_supermob( obj_data * obj )
{
   room_index *room;
   obj_data *in_obj;

   /*
    * Egad, how could this ever be true?? 
    */
   if( !supermob )
      supermob = get_mob_index( MOB_VNUM_SUPERMOB )->create_mobile(  );

   if( !obj )
      return;

   supermob_obj = obj;

   for( in_obj = obj; in_obj->in_obj; in_obj = in_obj->in_obj )
      ;

   if( in_obj->carried_by )
      room = in_obj->carried_by->in_room;
   else
      room = obj->in_room;

   if( !room )
      return;

   STRFREE( supermob->short_descr );
   supermob->short_descr = QUICKLINK( obj->short_descr );
   supermob->mpscriptpos = obj->mpscriptpos;

   /*
    * Added by Jenny to allow bug messages to show the vnum of the object, and not just supermob's vnum 
    */
   stralloc_printf( &supermob->chardesc, "Object #%d", obj->pIndexData->vnum );

   if( room != NULL )
   {
      supermob->from_room(  );
      if( !supermob->to_room( room ) )
         log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
      if( obj->extra_flags.test( ITEM_ONMAP ) )
      {
         supermob->set_actflag( ACT_ONMAP );
         supermob->cmap = obj->cmap;
         supermob->mx = obj->mx;
         supermob->my = obj->my;
      }
   }
}

void release_supermob(  )
{
   supermob_obj = NULL;
   supermob->from_room(  );
   if( !supermob->to_room( get_room_index( ROOM_VNUM_POLY ) ) )
      log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
   if( supermob->has_actflag( ACT_ONMAP ) )
   {
      supermob->unset_actflag( ACT_ONMAP );
      supermob->cmap = -1;
      supermob->mx = -1;
      supermob->my = -1;
   }
}

bool oprog_percent_check( char_data * mob, char_data * actor, obj_data * obj, char_data * victim, obj_data * target, int type )
{
   list < mud_prog_data * >::iterator mpg;
   mud_prog_data *mprg;
   bool executed = false;

   for( mpg = obj->pIndexData->mudprogs.begin(  ); mpg != obj->pIndexData->mudprogs.end(  ); ++mpg )
   {
      mprg = *mpg;

      if( mprg->type == type && ( number_percent(  ) <= atoi( mprg->arglist ) ) )
      {
         executed = true;
         mprog_driver( mprg->comlist, mob, actor, obj, victim, target, false );
         if( type != GREET_PROG )
            break;
      }
   }
   return executed;
}

/*
 * Triggers follow
 */
void oprog_greet_trigger( char_data * ch )
{
   list < obj_data * >::iterator iobj;

   for( iobj = ch->in_room->objects.begin(  ); iobj != ch->in_room->objects.end(  ); )
   {
      obj_data *vobj = *iobj;
      ++iobj;

      // Band-aid fix to stop unknown loop bug
      if( ch->in_room != vobj->in_room )
         break;

      if( HAS_PROG( vobj->pIndexData, GREET_PROG ) )
      {
         set_supermob( vobj );   /* not very efficient to do here */
         oprog_percent_check( supermob, ch, vobj, NULL, NULL, GREET_PROG );
         release_supermob(  );
      }
   }
}

void oprog_and_speech_trigger( const string & txt, char_data * ch )
{
   list < obj_data * >::iterator iobj;

   for( iobj = ch->in_room->objects.begin(  ); iobj != ch->in_room->objects.end(  ); )
   {
      obj_data *vobj = *iobj;
      ++iobj;

      // Band-aid fix to stop unknown loop bug
      if( ch->in_room != vobj->in_room )
         break;

      if( HAS_PROG( vobj->pIndexData, SPEECH_AND_PROG ) )
         oprog_and_wordlist_check( txt, supermob, ch, vobj, NULL, NULL, SPEECH_AND_PROG, vobj );
   }
}

void oprog_speech_trigger( const string & txt, char_data * ch )
{
   list < obj_data * >::iterator iobj;

   for( iobj = ch->in_room->objects.begin(  ); iobj != ch->in_room->objects.end(  ); )
   {
      obj_data *vobj = *iobj;
      ++iobj;

      // Band-aid fix to stop unknown loop bug
      if( ch->in_room != vobj->in_room )
         break;

      if( HAS_PROG( vobj->pIndexData, SPEECH_PROG ) )
         oprog_wordlist_check( txt, supermob, ch, vobj, NULL, NULL, SPEECH_PROG, vobj );
   }
}

/*
 * Called at top of obj_update
 * make sure to put an if(!obj) continue
 * after it
 */
void oprog_random_trigger( obj_data * obj )
{
   if( HAS_PROG( obj->pIndexData, RAND_PROG ) )
   {
      set_supermob( obj );
      oprog_percent_check( supermob, NULL, obj, NULL, NULL, RAND_PROG );
      release_supermob(  );
   }
}

/*
 * in wear_obj, between each successful equip_char 
 * the subsequent return
 */
void oprog_wear_trigger( char_data * ch, obj_data * obj )
{
   if( HAS_PROG( obj->pIndexData, WEAR_PROG ) )
   {
      set_supermob( obj );
      oprog_percent_check( supermob, ch, obj, NULL, NULL, WEAR_PROG );
      release_supermob(  );
   }
}

bool oprog_use_trigger( char_data * ch, obj_data * obj, char_data * vict, obj_data * targ )
{
   bool executed = false;

   if( HAS_PROG( obj->pIndexData, USE_PROG ) )
   {
      set_supermob( obj );
      if( obj->item_type == ITEM_STAFF || obj->item_type == ITEM_WAND || obj->item_type == ITEM_SCROLL )
      {
         if( vict )
            executed = oprog_percent_check( supermob, ch, obj, vict, targ, USE_PROG );
         else
            executed = oprog_percent_check( supermob, ch, obj, NULL, NULL, USE_PROG );
      }
      else
         executed = oprog_percent_check( supermob, ch, obj, NULL, NULL, USE_PROG );
      release_supermob(  );
   }
   return executed;
}

/*
 * call in remove_obj, right after unequip_char   
 * do a if(!ch) return right after, and return true (?)
 * if !ch
 */
void oprog_remove_trigger( char_data * ch, obj_data * obj )
{
   if( HAS_PROG( obj->pIndexData, REMOVE_PROG ) )
   {
      set_supermob( obj );
      oprog_percent_check( supermob, ch, obj, NULL, NULL, REMOVE_PROG );
      release_supermob(  );
   }
}

/*
 * call in do_sac, right before extract_obj
 */
void oprog_sac_trigger( char_data * ch, obj_data * obj )
{
   if( HAS_PROG( obj->pIndexData, SAC_PROG ) )
   {
      set_supermob( obj );
      oprog_percent_check( supermob, ch, obj, NULL, NULL, SAC_PROG );
      release_supermob(  );
   }
}

/*
 * call in do_get, right before check_for_trap
 * do a if(!ch) return right after
 */
void oprog_get_trigger( char_data * ch, obj_data * obj )
{
   if( HAS_PROG( obj->pIndexData, GET_PROG ) )
   {
      set_supermob( obj );
      oprog_percent_check( supermob, ch, obj, NULL, NULL, GET_PROG );
      release_supermob(  );
   }
}

/*
 * called in damage_obj in act_obj.c
 */
void oprog_damage_trigger( char_data * ch, obj_data * obj )
{
   if( HAS_PROG( obj->pIndexData, DAMAGE_PROG ) )
   {
      set_supermob( obj );
      oprog_percent_check( supermob, ch, obj, NULL, NULL, DAMAGE_PROG );
      release_supermob(  );
   }
}

/*
 * called in do_repair in shops.c
 */
void oprog_repair_trigger( char_data * ch, obj_data * obj )
{
   if( HAS_PROG( obj->pIndexData, REPAIR_PROG ) )
   {
      set_supermob( obj );
      oprog_percent_check( supermob, ch, obj, NULL, NULL, REPAIR_PROG );
      release_supermob(  );
   }
}

/*
 * call twice in do_drop, right after the act( AT_ACTION,...)
 * do a if(!ch) return right after
 */
void oprog_drop_trigger( char_data * ch, obj_data * obj )
{
   if( HAS_PROG( obj->pIndexData, DROP_PROG ) )
   {
      set_supermob( obj );
      oprog_percent_check( supermob, ch, obj, NULL, NULL, DROP_PROG );
      release_supermob(  );
   }
}

/*
 * call towards end of do_examine, right before check_for_trap
 */
void oprog_examine_trigger( char_data * ch, obj_data * obj )
{
   if( HAS_PROG( obj->pIndexData, EXA_PROG ) )
   {
      set_supermob( obj );
      oprog_percent_check( supermob, ch, obj, NULL, NULL, EXA_PROG );
      release_supermob(  );
   }
}

/*
 * call in fight.c, group_gain, after (?) the obj_to_room
 */
void oprog_zap_trigger( char_data * ch, obj_data * obj )
{
   if( HAS_PROG( obj->pIndexData, ZAP_PROG ) )
   {
      set_supermob( obj );
      oprog_percent_check( supermob, ch, obj, NULL, NULL, ZAP_PROG );
      release_supermob(  );
   }
}

/*
 * call in levers.c, towards top of do_push_or_pull
 *  see note there 
 */
void oprog_pull_trigger( char_data * ch, obj_data * obj )
{
   if( HAS_PROG( obj->pIndexData, PULL_PROG ) )
   {
      set_supermob( obj );
      oprog_percent_check( supermob, ch, obj, NULL, NULL, PULL_PROG );
      release_supermob(  );
   }
}

/*
 * call in levers.c, towards top of do_push_or_pull
 *  see note there 
 */
void oprog_push_trigger( char_data * ch, obj_data * obj )
{
   if( HAS_PROG( obj->pIndexData, PUSH_PROG ) )
   {
      set_supermob( obj );
      oprog_percent_check( supermob, ch, obj, NULL, NULL, PUSH_PROG );
      release_supermob(  );
   }
}

void obj_act_add( obj_data * obj );
void oprog_act_trigger( const string & buf, obj_data * mobj, char_data * ch, obj_data * obj, char_data * victim, obj_data * target )
{
   if( HAS_PROG( mobj->pIndexData, ACT_PROG ) )
   {
      mprog_act_list *tmp_act;

      tmp_act = new mprog_act_list;

      tmp_act->buf = buf;
      tmp_act->ch = ch;
      tmp_act->obj = obj;
      tmp_act->victim = victim;
      tmp_act->target = target;
      ++mobj->mpactnum;
      mobj->mpact.push_back( tmp_act );
      obj_act_add( mobj );
   }
}

/*
 *  room_prog support starts here
 */
void rset_supermob( room_index * room )
{
   if( room )
   {
      STRFREE( supermob->short_descr );
      supermob->short_descr = QUICKLINK( room->name );
      STRFREE( supermob->name );
      supermob->name = QUICKLINK( room->name );
      supermob->mpscriptpos = room->mpscriptpos;

      /*
       * Added by Jenny to allow bug messages to show the vnum
       * of the room, and not just supermob's vnum 
       */
      stralloc_printf( &supermob->chardesc, "Room #%d", room->vnum );

      supermob->from_room(  );
      if( !supermob->to_room( room ) )
         log_printf( "char_to_room: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
   }
}

void rprog_percent_check( char_data * mob, char_data * actor, obj_data * obj, char_data * victim, obj_data * target, int type )
{
   list < mud_prog_data * >::iterator mpg;
   mud_prog_data *mprg;

   if( !mob->in_room )
      return;

   for( mpg = mob->in_room->mudprogs.begin(  ); mpg != mob->in_room->mudprogs.end(  ); ++mpg )
   {
      mprg = *mpg;

      if( mprg->type == type && number_percent(  ) <= atoi( mprg->arglist ) )
      {
         mprog_driver( mprg->comlist, mob, actor, obj, victim, target, false );
         if( type != ENTER_PROG )
            break;
      }
   }
}

/*
 * Triggers follow
 */
void room_act_add( room_index * room );
void rprog_act_trigger( const string & buf, room_index * room, char_data * ch, obj_data * obj, char_data * victim, obj_data * target )
{
   if( HAS_PROG( room, ACT_PROG ) )
   {
      mprog_act_list *tmp_act;

      tmp_act = new mprog_act_list;

      tmp_act->buf = buf;
      tmp_act->ch = ch;
      tmp_act->obj = obj;
      tmp_act->victim = victim;
      tmp_act->target = target;
      ++room->mpactnum;
      room->mpact.push_back( tmp_act );
      room_act_add( room );
   }
}

void rprog_leave_trigger( char_data * ch )
{
   if( HAS_PROG( ch->in_room, LEAVE_PROG ) )
   {
      rset_supermob( ch->in_room );
      rprog_percent_check( supermob, ch, NULL, NULL, NULL, LEAVE_PROG );
      release_supermob(  );
   }
}

/* login and void room triggers by Edmond */
void rprog_login_trigger( char_data *ch )
{
   if( HAS_PROG( ch->in_room, LOGIN_PROG ) )
   {
      rset_supermob( ch->in_room );
      rprog_percent_check( supermob, ch, NULL, NULL, NULL, LOGIN_PROG );
      release_supermob();
   }
}

void rprog_void_trigger( char_data *ch )
{
   if( HAS_PROG( ch->in_room, VOID_PROG ) )
   {
      rset_supermob( ch->in_room );
      rprog_percent_check( supermob, ch, NULL, NULL, NULL, VOID_PROG );
      release_supermob();
   }
}

void rprog_enter_trigger( char_data * ch )
{
   if( HAS_PROG( ch->in_room, ENTER_PROG ) )
   {
      rset_supermob( ch->in_room );
      rprog_percent_check( supermob, ch, NULL, NULL, NULL, ENTER_PROG );
      release_supermob(  );
   }
}

void rprog_sleep_trigger( char_data * ch )
{
   if( HAS_PROG( ch->in_room, SLEEP_PROG ) )
   {
      rset_supermob( ch->in_room );
      rprog_percent_check( supermob, ch, NULL, NULL, NULL, SLEEP_PROG );
      release_supermob(  );
   }
}

void rprog_rest_trigger( char_data * ch )
{
   if( HAS_PROG( ch->in_room, REST_PROG ) )
   {
      rset_supermob( ch->in_room );
      rprog_percent_check( supermob, ch, NULL, NULL, NULL, REST_PROG );
      release_supermob(  );
   }
}

void rprog_rfight_trigger( char_data * ch )
{
   if( HAS_PROG( ch->in_room, RFIGHT_PROG ) )
   {
      rset_supermob( ch->in_room );
      rprog_percent_check( supermob, ch, NULL, NULL, NULL, RFIGHT_PROG );
      release_supermob(  );
   }
}

void rprog_death_trigger( char_data * ch )
{
   if( HAS_PROG( ch->in_room, RDEATH_PROG ) )
   {
      rset_supermob( ch->in_room );
      rprog_percent_check( supermob, ch, NULL, NULL, NULL, RDEATH_PROG );
      release_supermob(  );
   }
}

void rprog_and_speech_trigger( const string & txt, char_data * ch )
{
   if( HAS_PROG( ch->in_room, SPEECH_AND_PROG ) )
   {
      /*
       * supermob is set and released in rprog_wordlist_check 
       */
      rprog_and_wordlist_check( txt, supermob, ch, NULL, NULL, NULL, SPEECH_AND_PROG, ch->in_room );
   }
}

void rprog_speech_trigger( const string & txt, char_data * ch )
{
   if( HAS_PROG( ch->in_room, SPEECH_PROG ) )
   {
      /*
       * supermob is set and released in rprog_wordlist_check 
       */
      rprog_wordlist_check( txt, supermob, ch, NULL, NULL, NULL, SPEECH_PROG, ch->in_room );
   }
}

void rprog_random_trigger( char_data * ch )
{
   if( HAS_PROG( ch->in_room, RAND_PROG ) )
   {
      rset_supermob( ch->in_room );
      rprog_percent_check( supermob, ch, NULL, NULL, NULL, RAND_PROG );
      release_supermob(  );
   }
}

void rprog_time_check( char_data * mob, char_data * actor, obj_data * obj, room_index * room, int type )
{
   list < mud_prog_data * >::iterator mpg;
   mud_prog_data *mprg;
   bool trigger_time;

   for( mpg = room->mudprogs.begin(  ); mpg != room->mudprogs.end(  ); ++mpg )
   {
      mprg = *mpg;

      trigger_time = ( time_info.hour == atoi( mprg->arglist ) );

      if( !trigger_time )
      {
         if( mprg->triggered )
            mprg->triggered = false;
         continue;
      }

      if( mprg->type == type && ( ( !mprg->triggered ) || ( mprg->type == HOUR_PROG ) ) )
      {
         mprg->triggered = true;
         mprog_driver( mprg->comlist, mob, actor, obj, NULL, NULL, false );
      }
   }
}

void rprog_time_trigger( char_data * ch )
{
   if( HAS_PROG( ch->in_room, TIME_PROG ) )
   {
      rset_supermob( ch->in_room );
      rprog_time_check( supermob, NULL, NULL, ch->in_room, TIME_PROG );
      release_supermob(  );
   }
}

void rprog_hour_trigger( char_data * ch )
{
   if( HAS_PROG( ch->in_room, HOUR_PROG ) )
   {
      rset_supermob( ch->in_room );
      rprog_time_check( supermob, NULL, NULL, ch->in_room, HOUR_PROG );
      release_supermob(  );
   }
}

/**************************************************************************
*       Month_prog coded by Desden, el Chaman Tibetano (J.L.Sogorb)       *
*                            October 1998                                 *
*                   Email: jlalbatros@mx2.redestb.es                      *
*                                                                         *
**************************************************************************/
void mprog_month_check( char_data * mob, char_data * actor, obj_data * obj, char_data * victim, obj_data * target, int type )
{
   list < mud_prog_data * >::iterator mpg;
   mud_prog_data *mprg;

   for( mpg = mob->pIndexData->mudprogs.begin(  ); mpg != mob->pIndexData->mudprogs.end(  ); ++mpg )
   {
      mprg = *mpg;

      if( mprg->type == type && time_info.month == atoi( mprg->arglist ) )
         mprog_driver( mprg->comlist, mob, actor, obj, victim, target, false );
   }
}

void rprog_month_check( char_data * mob, char_data * actor, obj_data * obj, room_index * room, int type )
{
   list < mud_prog_data * >::iterator mpg;
   mud_prog_data *mprg;

   for( mpg = room->mudprogs.begin(  ); mpg != room->mudprogs.end(  ); ++mpg )
   {
      mprg = *mpg;

      if( mprg->type == type && time_info.month == atoi( mprg->arglist ) )
         mprog_driver( mprg->comlist, mob, actor, obj, NULL, NULL, false );
   }
}

void oprog_month_check( char_data * mob, char_data * actor, obj_data * obj, char_data * victim, obj_data * target, int type )
{
   list < mud_prog_data * >::iterator mpg;
   mud_prog_data *mprg;

   for( mpg = obj->pIndexData->mudprogs.begin(  ); mpg != obj->pIndexData->mudprogs.end(  ); ++mpg )
   {
      mprg = *mpg;

      if( mprg->type == type && time_info.month == atoi( mprg->arglist ) )
         mprog_driver( mprg->comlist, mob, actor, obj, victim, target, false );
   }
}

void mprog_month_trigger( char_data * mob )
{
   if( HAS_PROG( mob->pIndexData, MONTH_PROG ) )
      mprog_month_check( mob, NULL, NULL, NULL, NULL, MONTH_PROG );
}

void rprog_month_trigger( char_data * ch )
{
   if( HAS_PROG( ch->in_room, MONTH_PROG ) )
   {
      rset_supermob( ch->in_room );
      rprog_month_check( supermob, NULL, NULL, ch->in_room, MONTH_PROG );
      release_supermob(  );
   }
}

void oprog_month_trigger( obj_data * obj )
{
   if( HAS_PROG( obj->pIndexData, MONTH_PROG ) )
   {
      set_supermob( obj );
      oprog_month_check( supermob, NULL, obj, NULL, NULL, MONTH_PROG );
      release_supermob(  );
   }
}

void progbugf( char_data * mob, const char *fmt, ... )
{
   char buf[MSL * 2];
   va_list args;

   va_start( args, fmt );
   vsnprintf( buf, MSL * 2, fmt, args );
   va_end( args );

   progbug( buf, mob );
}

/* Written by Jenny, Nov 29/95 */
void progbug( const string & str, char_data * mob )
{
   int vnum = mob->pIndexData ? mob->pIndexData->vnum : 0;

   /*
    * Check if we're dealing with supermob, which means the bug occurred
    * in a room or obj prog.
    */
   if( vnum == MOB_VNUM_SUPERMOB )
   {
      /*
       * It's supermob.  In set_supermob and rset_supermob, the description
       * was set to indicate the object or room, so we just need to show
       * the description in the bug message.
       */
      bug( "%s, %s.", str.c_str(  ), mob->chardesc == NULL ? "(unknown)" : mob->chardesc );
   }
   else
      bug( "%s, Mob #%d.", str.c_str(  ), vnum );
}

// A much simpler version than the old mess - Samson 8/2/05
void room_act_add( room_index * room )
{
   list < room_index * >::iterator pd;

   for( pd = room_act_list.begin(  ); pd != room_act_list.end(  ); ++pd )
   {
      room_index *rpd = *pd;

      if( rpd == room )
         return;
   }
   room_act_list.push_back( room );
}

void room_act_update( void )
{
   list < room_index * >::iterator pd;

   for( pd = room_act_list.begin(  ); pd != room_act_list.end(  ); )
   {
      room_index *room = *pd;
      ++pd;

      list < mprog_act_list * >::iterator mal;
      for( mal = room->mpact.begin(  ); mal != room->mpact.end(  ); )
      {
         mprog_act_list *mpact = *mal;
         ++mal;

         if( mpact->ch != NULL && mpact->ch->in_room == room )
            rprog_wordlist_check( mpact->buf, supermob, mpact->ch, mpact->obj, mpact->victim, mpact->target, ACT_PROG, room );
         room->mpact.remove( mpact );
         deleteptr( mpact );
      }
      room->mpactnum = 0;
      room_act_list.remove( room );
   }
}

// A much simpler version than the old mess - Samson 8/2/05
void obj_act_add( obj_data * obj )
{
   list < obj_data * >::iterator pd;

   for( pd = obj_act_list.begin(  ); pd != obj_act_list.end(  ); ++pd )
   {
      obj_data *opd = *pd;

      if( opd == obj )
         return;
   }
   obj_act_list.push_back( obj );
}

void obj_act_update( void )
{
   list < obj_data * >::iterator pd;
   mprog_act_list *mpact;

   for( pd = obj_act_list.begin(  ); pd != obj_act_list.end(  ); )
   {
      obj_data *obj = *pd;
      ++pd;

      list < mprog_act_list * >::iterator mal;
      for( mal = obj->mpact.begin(  ); mal != obj->mpact.end(  ); )
      {
         mpact = *mal;
         ++mal;

         oprog_wordlist_check( mpact->buf, supermob, mpact->ch, mpact->obj, mpact->victim, mpact->target, ACT_PROG, obj );
         obj->mpact.remove( mpact );
         deleteptr( mpact );
      }
      obj->mpactnum = 0;
      obj_act_list.remove( obj );
   }
}
