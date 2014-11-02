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

#ifndef __MUDPROG_H__
#define __MUDPROG_H__

extern char_data *supermob;
extern obj_data *supermob_obj;

/*
 * MudProg macros. - Thoric
 */
#define HAS_PROG( what, prog ) (what)->progtypes.test((prog))

/* Ifstate defines, used to create and access ifstate array in mprog_driver. */
const int MAX_IFS = 20; /* should always be generous */
const int IN_IF = 0;
const int IN_ELSE = 1;
const int DO_IF = 2;
const int DO_ELSE = 3;

const int MAX_PROG_NEST = 20;

void oprog_greet_trigger( char_data * );
void oprog_speech_trigger( const string &, char_data * );
void oprog_random_trigger( obj_data * );
void oprog_month_trigger( obj_data * );
void oprog_remove_trigger( char_data *, obj_data * );
void oprog_sac_trigger( char_data *, obj_data * );
void oprog_get_trigger( char_data *, obj_data * );
void oprog_damage_trigger( char_data *, obj_data * );
void oprog_repair_trigger( char_data *, obj_data * );
void oprog_drop_trigger( char_data *, obj_data * );
void oprog_examine_trigger( char_data *, obj_data * );
void oprog_zap_trigger( char_data *, obj_data * );
void oprog_pull_trigger( char_data *, obj_data * );
void oprog_push_trigger( char_data *, obj_data * );
void oprog_and_speech_trigger( const string &, char_data * );
void oprog_wear_trigger( char_data *, obj_data * );
bool oprog_use_trigger( char_data *, obj_data *, char_data *, obj_data * );
void oprog_act_trigger( const string &, obj_data *, char_data *, obj_data *, char_data *, obj_data * );
void rprog_act_trigger( const string &, room_index *, char_data *, obj_data *, char_data *, obj_data * );
void rprog_leave_trigger( char_data * );
void rprog_enter_trigger( char_data * );
void rprog_sleep_trigger( char_data * );
void rprog_rest_trigger( char_data * );
void rprog_rfight_trigger( char_data * );
void rprog_death_trigger( char_data * );
void rprog_speech_trigger( const string &, char_data * );
void rprog_random_trigger( char_data * );
void rprog_time_trigger( char_data * );
void rprog_month_trigger( char_data * );
void rprog_hour_trigger( char_data * );
void rprog_and_speech_trigger( const string &, char_data * );
void rprog_login_trigger( char_data * );
void rprog_void_trigger( char_data * );
void mprog_hitprcnt_trigger( char_data *, char_data * );
void mprog_fight_trigger( char_data *, char_data * );
void mprog_death_trigger( char_data *, char_data * );
bool mprog_keyword_trigger( const string &, char_data * );
void mprog_bribe_trigger( char_data *, char_data *, int );
void mprog_give_trigger( char_data *, char_data *, obj_data * );
bool mprog_wordlist_check( const string &, char_data *, char_data *, obj_data *, char_data *, obj_data *, int );
void mprog_act_trigger( const string &, char_data *, char_data *, obj_data *, char_data *, obj_data * );
void mprog_random_trigger( char_data * );
void mprog_script_trigger( char_data * );
void mprog_hour_trigger( char_data * );
void mprog_time_trigger( char_data * );
void mprog_month_trigger( char_data * );
void mprog_targetted_speech_trigger( const string &, char_data *, char_data * );
void mprog_speech_trigger( const string &, char_data * );
void mprog_and_speech_trigger( const string &, char_data * );
void mprog_tell_trigger( const string &, char_data * );
void mprog_and_tell_trigger( const string &, char_data * );
int mprog_name_to_type( const string & );

/* mud prog defines */
const int ERROR_PROG = -1;
const int IN_FILE_PROG = -2;

/*
 * For backwards compatability
 */
const int RDEATH_PROG = DEATH_PROG;
const int ENTER_PROG = ENTRY_PROG;
const int RFIGHT_PROG = FIGHT_PROG;
const int RGREET_PROG = GREET_PROG;
const int OGREET_PROG = GREET_PROG;

/* Mob program structures */
class mprog_act_list
{
 private:
   mprog_act_list( const mprog_act_list & m );
     mprog_act_list & operator=( const mprog_act_list & );

 public:
     mprog_act_list(  );
    ~mprog_act_list(  );

   string buf;
   char_data *ch;
   obj_data *obj;
   char_data *victim;
   obj_data *target;
};

struct mud_prog_data
{
 private:
   mud_prog_data( const mud_prog_data & m );
     mud_prog_data & operator=( const mud_prog_data & );

 public:
     mud_prog_data(  );
    ~mud_prog_data(  );

   char *arglist;
   char *comlist;
   int resetdelay;
   short type;
   bool triggered;
   bool fileprog;
};

template < class N > void fread_afk_mudprog( FILE * fp, mud_prog_data * mprg, N * prog_target )
{
   for( ;; )
   {
      const char *word = ( feof( fp ) ? "#ENDPROG" : fread_word( fp ) );

      if( word[0] == '\0' )
      {
         log_printf( "%s: EOF encountered reading file!", __FUNCTION__ );
         word = "#ENDPROG";
      }

      if( !str_cmp( word, "#ENDPROG" ) )
         return;

      switch ( word[0] )
      {
         default:
            log_printf( "%s: no match: %s", __FUNCTION__, word );
            fread_to_eol( fp );
            break;

         case 'A':
            if( !str_cmp( word, "Arglist" ) )
            {
               mprg->arglist = fread_string( fp );
               mprg->fileprog = false;

               switch ( mprg->type )
               {
                  case IN_FILE_PROG:
                     mprog_file_read( prog_target, mprg->arglist );
                     break;
                  default:
                     break;
               }
               break;
            }
            break;

         case 'C':
            KEY( "Comlist", mprg->comlist, fread_string( fp ) );
            break;

         case 'P':
            if( !str_cmp( word, "Progtype" ) )
            {
               mprg->type = mprog_name_to_type( fread_flagstring( fp ) );
               prog_target->progtypes.set( mprg->type );
               break;
            }
            break;
      }
   }
}

template < class N > void mprog_file_read( N * prog_target, const char *f )
{
   mud_prog_data *mprg = NULL;
   char MUDProgfile[256];
   FILE *progfile;
   char letter;

   snprintf( MUDProgfile, 256, "%s%s", PROG_DIR, f );

   if( !( progfile = fopen( MUDProgfile, "r" ) ) )
   {
      bug( "%s: couldn't open mudprog file", __FUNCTION__ );
      return;
   }

   for( ;; )
   {
      letter = fread_letter( progfile );

      if( letter != '#' )
      {
         bug( "%s: MUDPROG char", __FUNCTION__ );
         break;
      }

      const char *word = ( feof( progfile ) ? "ENDFILE" : fread_word( progfile ) );

      if( word[0] == '\0' )
      {
         log_printf( "%s: EOF encountered reading file!", __FUNCTION__ );
         word = "ENDFILE";
      }

      if( !str_cmp( word, "ENDFILE" ) )
         break;

      if( !str_cmp( word, "MUDPROG" ) )
      {
         mprg = new mud_prog_data;

         for( ;; )
         {
            word = ( feof( progfile ) ? "#ENDPROG" : fread_word( progfile ) );

            if( word[0] == '\0' )
            {
               log_printf( "%s: EOF encountered reading file!", __FUNCTION__ );
               word = "#ENDPROG";
            }

            if( !str_cmp( word, "#ENDPROG" ) )
            {
               prog_target->progtypes.set( mprg->type );
               prog_target->mudprogs.push_back( mprg );
               break;
            }

            switch ( word[0] )
            {
               default:
                  log_printf( "%s: no match: %s", __FUNCTION__, word );
                  fread_to_eol( progfile );
                  break;

               case 'A':
                  if( !str_cmp( word, "Arglist" ) )
                  {
                     mprg->arglist = fread_string( progfile );
                     mprg->fileprog = false;

                     switch ( mprg->type )
                     {
                        case IN_FILE_PROG:
                           bug( "%s: Nested file programs are not allowed.", __FUNCTION__ );
                           deleteptr( mprg );
                           break;

                        default:
                           break;
                     }
                     break;
                  }
                  break;

               case 'C':
                  KEY( "Comlist", mprg->comlist, fread_string( progfile ) );
                  break;

               case 'P':
                  if( !str_cmp( word, "Progtype" ) )
                  {
                     mprg->type = mprog_name_to_type( fread_flagstring( progfile ) );
                     break;
                  }
                  break;
            }
         }
      }
   }
   FCLOSE( progfile );
}

extern list < room_index * >room_act_list;
extern list < obj_data * >obj_act_list;
extern list < char_data * >mob_act_list;
#endif
