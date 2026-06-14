/****************************************************************************
 *                   ^     +----- |  / ^     ^ |     | +-\                  *
 *                  / \    |      | /  |\   /| |     | |  \                 *
 *                 /   \   +---   |<   | \ / | |     | |  |                 *
 *                /-----\  |      | \  |  v  | |     | |  /                 *
 *               /       \ |      |  \ |     | +-----+ +-/                  *
 ****************************************************************************
 * AFKMud Copyright 1997-2025 by Roger Libiez (Samson),                     *
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
 *                            Character Class Info                          *
 ****************************************************************************/

#pragma once

enum timer_types
{
   TIMER_NONE, TIMER_RECENTFIGHT, TIMER_SHOVEDRAG, TIMER_DO_FUN,
   TIMER_APPLIED, TIMER_PKILLED, TIMER_ASUPRESSED
};

struct timer_data
{
   DO_FUN *do_fun = nullptr;
   int value = 0;
   int count = 0;
   short type = 0;
};

/*
 * Data which only PC's have.
 */
class pc_data
{
 private:
   pc_data( const pc_data & p );
     pc_data & operator=( const pc_data & );

 protected:
   std::bitset<MAX_PCFLAG> flags;                        // Whether the player is deadly and whatever else we add. Also covers the old PLR_FLAGS

 public:
     friend class char_data;

     pc_data(  );
    ~pc_data(  );

   /*
    * External references 
    */
   void save_ignores( FILE * );
   void load_ignores( FILE * );
   void save_zonedata( FILE * );
   void load_zonedata( FILE * );
   void free_comments(  );
   void free_pcboards(  );
   void fwrite_comments( FILE * );
   void fread_comment( FILE * );
   void fread_old_comment( FILE * );

   std::map<std::string, std::string> alias_map;         // Command aliases. These are player defined aliases for the game's commands.
   std::map<int, std::string> qbits;                     // abit/qbit code. Used for tracking quests and other things.
   std::list<class board_chardata *> boarddata;          // Board stuff.
   std::list<class note_data *> comments;                // Comments attached to their player file by immortals.
   std::list<std::string> zone;                          // List of zones this PC has visited - Samson 7-11-00
   std::list<std::string> ignore;                        // List of players to ignore. Unlike Discord or a forum, this is actually effective.
   std::vector<std::string> say_history;                 // A running history of things they've said or heard said in a local room.
   std::vector<std::string> tell_history;                // A running history of tells they've received.
   std::string bestowments;                              // Special bestowed commands they would not normally have access to.
   std::string chan_listen;                              // Which channels the person is listening to. For dynamic channels - Samson 3-2-02
   std::string clan_name;                                // Name of the clan/guild this person belongs to if any.
   std::string realm_name;                               // Name of the immortal realm this person belongs to.
   std::string deity_name;                               // Name of the deity this person worships.
   std::string lasthost;                                 // Stores host info so it doesn't have to depend on a descriptor, for things like finger.
   std::string prevhost;                                 // Stores IP logged into prior to the lasthost field.
   std::string homepage;                                 // The person's homepage in the big bad world out there.
   std::string email;                                    // The person's email address.
   std::string pwd;                                      // The person's password. This value is SHA-256 encrypted.
   std::string authed_by;                                // The immortal who authorized this player's name.
   std::string filename;                                 // For the safe mset name -Shaddai [The player's on disk filename]
   std::chrono::system_clock::time_point release_date;   // Auto-helling.. Altrag [When they will be released by the game if staff does not intervene]
   std::chrono::system_clock::time_point motd;           // Last time they read an MOTD - Samson 12-31-00
   std::chrono::system_clock::time_point imotd;          // Last time they read an IMOTD for immortals - 12-31-00
   std::chrono::system_clock::time_point logon;          // When they last logged on.
   std::chrono::hours played;                            // Total hours they have in the game so far.
   std::chrono::system_clock::time_point save_time;
   std::chrono::system_clock::time_point restore_time;   // The last time the person did a restore all.
   area_data *area = nullptr;                            // For the area a PC has been assigned to build.
   class clan_data *clan = nullptr;                      // What clan, guild, or order they are a member of.
   class realm_data *realm = nullptr;                    // What immortal realm they are a part of.
   class deity_data *deity = nullptr;                    // Which deity they worship.
   struct editor_data *editor = nullptr;                 // Contains information on what they are currently editing.
   class note_data *pnote = nullptr;                     // Board stuff.
   class board_data *board = nullptr;                    // Board stuff.
   struct game_board_data *game_board = nullptr;         // Chess board they are playing on.
 public:
   void *spare_ptr = nullptr;                            // Um... sure.
   void *dest_buf = nullptr;                             // This one is to assign to different things. [Um... sure]
   char *bamfin = nullptr;                               // Message displayed when an immortal enters a room, if set.
   char *bamfout = nullptr;                              // Message displayed when an immortal leaves a room, if set.
   char *rank = nullptr;                                 // Rank they hold in a clan, guild, order, or realm.
   char *title = nullptr;                                // Custom title, displayed on the who output.
   char *helled_by = nullptr;                            // Who put this person in hell.
   char *bio = nullptr;                                  // Personal Bio. Visible using the finger command.
   char *prompt = nullptr;                               // User configurable general prompt.
   char *fprompt = nullptr;                              // User configurable fight prompt.
   char *subprompt = nullptr;                            // Substate prompt.
   char *afkbuf = nullptr;                               // afk reason buffer - Samson 8-31-98
   char *motd_buf = nullptr;                             // A temp buffer for editing MOTDs - 12-31-00
   int pkills = 0;                                       // Number of pkills on behalf of clan.
   int pdeaths = 0;                                      // Number of times pkilled (legally).
   int mkills = 0;                                       // Number of mobs killed.
   int mdeaths = 0;                                      // Number of deaths due to mobs.
   int illegal_pk = 0;                                   // Number of illegal pk's committed.
   int low_vnum = 0;                                     // Low end of vnum range for area editing.
   int hi_vnum = 0;                                      // High end of vnum range for area editing.
   int secedit = 0;                                      // Overland Map OLC. Which type of sector they are currently making. - Samson 8-1-99 */
   int home = 0;                                         // Vnum of their current home. [This feature has not yet been implemented fully]
   int balance = 0;                                      // Bank balance - Samson
   int exgold = 0;                                       // Extragold affect - Samson
   int one = 0;                                          // Last room they rented in on primary continent - Samson 12-20-00
   int spam = 0;                                         // How many times have they triggered the spamguard? - 3-18-01
   int timezone = -1;                                    // The user's current real world timezone.
   int version = 0;                                      // Temporary variable to track pfile password conversion.
   short learned[MAX_SKILL];                             // Skill levels they have achieved for all the skills/spells in the game.
   short wizinvis = 0;                                   // wizinvis level
   short condition[MAX_CONDS];                           // Current levels of drunkenness, thirst, and hunger.
   short favor = 0;                                      // How much favor they have with their chosen deity.
   short practice = 0;                                   // Number of remaining practice sessions available for use at a skill trainer.
   short pagerlen = 24;                                  // For on screen pager (NOT menus)
   short camp = 0;                                       // Did the player camp or rent? Samson 9-19-98
   short colors[MAX_COLORS];                             // Custom color codes - Samson 9-28-98
   short beacon[MAX_BEACONS];                            // For beacon spell, recall points - Samson 2-7-99
   short charmies = 0;                                   // Number of Charmies.
   short cmd_recurse = -1;                               // Command recursion check for the alias code.
   short age_bonus = 0;                                  // Modifier to the player's in-game age.
   short age = 0;                                        // The player's in-game age, according to the game calendar.
   short day = 0;                                        // The in-game day the player created their character.
   short month = 0;                                      // The in-game month the player created their character.
   short year = 0;                                       // The in-game year the player created their character.
   short daysidle = 0;                                   // Number of days they've been idle from the game. Counts while offline as well. Used in rare item handling.
   bool hotboot = false;  /* Used only to force hotboot to save keys etc that normally get stripped - Samson 6-22-01 */
};

/*
 * One character (PC or NPC).
 */
class char_data
{
 private:
   char_data( const char_data & c );
     char_data & operator=( const char_data & );

 public:
     char_data(  );
    ~char_data(  );

   /*
    * Internal to character.cpp
    */
   void print( std::string_view );

   /*
    * This is a fully std::string implementation of the older ::printf function. No more faffing about with char buffers.
    * GCC, you are a colossal asshole for not letting me just keep the same function name because of some stupid committee decision.
    * Samson 5-24-2026.
    */
   template<typename... Args>
   void print_fmt( std::format_string<Args...> fmt, Args&&... args )
   {
      this->print( std::format( fmt, std::forward<Args>(args)...) );
   }
   void printf( const char *, ... ) __attribute__ ( ( format( printf, 2, 3 ) ) ); // Legacy C style formatting.

   void pager( std::string_view );

   template<typename... Args>
   void pager_fmt( std::format_string<Args...> fmt, Args&&... args )
   {
      this->pager( std::format( fmt, std::forward<Args>(args)...) );
   }
   void pagerf( const char *, ... ) __attribute__ ( ( format( printf, 2, 3 ) ) );

   void print_room( std::string_view );
   void set_color( short );
   void set_pager_color( short );
   void set_title( std::string_view );
   int calculate_race_height(  );
   int calculate_race_weight(  );
   short get_trust(  );
   short get_age(  );
   short calculate_age(  );
   short get_curr_str(  );
   short get_curr_int(  );
   short get_curr_wis(  );
   short get_curr_dex(  );
   short get_curr_con(  );
   short get_curr_cha(  );
   short get_curr_lck(  );
   bool can_take_proto(  );
   bool can_see( char_data *, bool );
   char_data *get_char_room( std::string_view );
   char_data *get_char_world( std::string_view );
   room_index *find_location( std::string_view );
   bool can_see_obj( obj_data *, bool );
   bool can_drop_obj( obj_data * );
   obj_data *get_obj_vnum( int );
   obj_data *get_obj_carry( std::string_view );
   obj_data *get_obj_wear( std::string_view );
   obj_data *get_obj_here( std::string_view );
   obj_data *get_obj_world( std::string_view );
   obj_data *get_eq( int );
   int can_carry_n(  );
   int can_carry_w(  );
   void equip( obj_data *, int );
   void unequip( obj_data * );
   void aris_affect( affect_data * );
   void update_aris(  );
   void modify_skill( int, int, bool );
   void affect_modify( affect_data *, bool );
   void affect_to_char( affect_data * );
   void affect_remove( affect_data * );
   void affect_strip( int );
   bool is_affected( int );
   void affect_join( affect_data * );
   void showaffect( affect_data * );
   void set_numattacks(  );
   int char_ego(  );
   short get_timer( short );
   timer_data *get_timerptr( short );
   void add_timer( short, short, DO_FUN *, int );
   void extract_timer( timer_data * );
   void remove_timer( short );
   bool in_hard_range( area_data * );
   void better_mental_state( int );
   void worsen_mental_state( int );
   void from_room(  );
   bool to_room( room_index * );
   bool chance( short );
   void ClassSpecificStuff(  );
   void extract( bool );
   void gain_exp( double );
   void set_specfun(  );
   const char *get_class(  );
   const char *get_race(  );
   void stop_idling(  );
   bool IS_OUTSIDE(  );
   int GET_AC(  );
   int GET_HITROLL(  );
   int GET_DAMROLL(  );
   int GET_ADEPT( int );
   bool IS_DRUNK( int );
   bool MSP_ON(  );

   /*
    * External references in other files 
    */
   bool char_died(  );
   void music( std::string_view, int, bool );
   void sound( std::string_view, int, bool );
   void reset_sound(  );
   void reset_music(  );
   const char *color_str( short );
   void save(  );
   void de_equip(  );
   void re_equip(  );
   void update_pos(  );
   char_data *who_fighting(  );
   void stop_fighting( bool );
   void editor_desc_printf( const char *, ... ) __attribute__ ( ( format( printf, 2, 3 ) ) );
   void start_editing( char * );
   void start_editing( std::string );
   void stop_editing(  );
   char *copy_buffer( bool );
   std::string copy_buffer(  );
   void set_editor_desc( std::string_view );
   void edit_buffer( std::string & );
   void note_attach(  );
   bool can_charm(  );
   void learn_from_failure( int );
   void learn_racials( int );
   bool has_visited( area_data * );
   void update_visits( room_index * );
   void remove_visit( room_index * );
   void adjust_favor( int, int );
   std::chrono::hours time_played( );

   void set_actflag( int );
   void unset_actflag( int );
   bool has_actflag( int );
   void toggle_actflag( int );
   bool has_actflags(  );
   std::bitset<MAX_ACT_FLAG> get_actflags(  );
   void set_actflags( std::bitset<MAX_ACT_FLAG> );
   void set_file_actflags( FILE * );

   bool has_immune( int );
   void set_immune( int );
   void unset_immune( int );
   void toggle_immune( int );
   bool has_immunes(  );
   std::bitset<MAX_RIS_FLAG> get_immunes(  );
   void set_immunes( std::bitset<MAX_RIS_FLAG> );
   void set_file_immunes( FILE * );

   bool has_noimmune( int );
   void set_noimmune( int );
   bool has_noimmunes(  );
   std::bitset<MAX_RIS_FLAG> get_noimmunes(  );
   void set_noimmunes( std::bitset<MAX_RIS_FLAG> );
   void set_file_noimmunes( FILE * );

   bool has_resist( int );
   void set_resist( int );
   void unset_resist( int );
   void toggle_resist( int );
   bool has_resists(  );
   std::bitset<MAX_RIS_FLAG> get_resists(  );
   void set_resists( std::bitset<MAX_RIS_FLAG> );
   void set_file_resists( FILE * );

   bool has_noresist( int );
   void set_noresist( int );
   bool has_noresists(  );
   std::bitset<MAX_RIS_FLAG> get_noresists(  );
   void set_noresists( std::bitset<MAX_RIS_FLAG> );
   void set_file_noresists( FILE * );

   bool has_suscep( int );
   void set_suscep( int );
   void unset_suscep( int );
   void toggle_suscep( int );
   bool has_susceps(  );
   std::bitset<MAX_RIS_FLAG> get_susceps(  );
   void set_susceps( std::bitset<MAX_RIS_FLAG> );
   void set_file_susceps( FILE * );

   bool has_nosuscep( int );
   void set_nosuscep( int );
   bool has_nosusceps(  );
   std::bitset<MAX_RIS_FLAG> get_nosusceps(  );
   void set_nosusceps( std::bitset<MAX_RIS_FLAG> );
   void set_file_nosusceps( FILE * );

   bool has_absorb( int );
   void set_absorb( int );
   void unset_absorb( int );
   void toggle_absorb( int );
   bool has_absorbs(  );
   std::bitset<MAX_RIS_FLAG> get_absorbs(  );
   void set_absorbs( std::bitset< MAX_RIS_FLAG> );
   void set_file_absorbs( FILE * );

   bool has_attack( int );
   void set_attack( int );
   void unset_attack( int );
   void toggle_attack( int );
   bool has_attacks(  );
   std::bitset<MAX_ATTACK_TYPE> get_attacks(  );
   void set_attacks( std::bitset<MAX_ATTACK_TYPE> );
   void set_file_attacks( FILE * );

   bool has_defense( int );
   void set_defense( int );
   void unset_defense( int );
   void toggle_defense( int );
   bool has_defenses(  );
   std::bitset<MAX_DEFENSE_TYPE> get_defenses(  );
   void set_defenses( std::bitset<MAX_DEFENSE_TYPE> );
   void set_file_defenses( FILE * );

   bool has_aflag( int );
   void set_aflag( int );
   void unset_aflag( int );
   void toggle_aflag( int );
   bool has_aflags(  );
   std::bitset<MAX_AFFECTED_BY> get_aflags(  );
   void set_aflags( std::bitset<MAX_AFFECTED_BY> );
   void set_file_aflags( FILE * );

   bool has_noaflag( int );
   void set_noaflag( int );
   void unset_noaflag( int );
   void toggle_noaflag( int );
   bool has_noaflags(  );
   std::bitset<MAX_AFFECTED_BY> get_noaflags(  );
   void set_noaflags( std::bitset<MAX_AFFECTED_BY> );
   void set_file_noaflags( FILE * );

   bool has_bpart( int );
   void set_bpart( int );
   void unset_bpart( int );
   void toggle_bpart( int );
   bool has_bparts(  );
   std::bitset<MAX_BPART> get_bparts(  );
   void set_bparts( std::bitset<MAX_BPART> );
   void set_file_bparts( FILE * );
   void set_bodypart_where_names( );

   bool has_pcflag( int );
   void set_pcflag( int );
   void unset_pcflag( int );
   void toggle_pcflag( int );
   bool has_pcflags(  );
   std::bitset<MAX_PCFLAG> get_pcflags(  );
   void set_file_pcflags( FILE * );

   bool has_lang( int );
   void set_lang( int );
   void unset_lang( int );
   void toggle_lang( int );
   bool has_langs(  );
   std::bitset<LANG_UNKNOWN> get_langs(  );
   void set_langs( std::bitset<LANG_UNKNOWN> );
   void set_file_langs( FILE * );

   bool isnpc(  )
   {
      return ( actflags.test( ACT_IS_NPC ) );
   }
   bool is_immortal(  )
   {
      return ( !isnpc(  ) && level >= LEVEL_IMMORTAL );
   }
   bool is_imp(  )
   {
      return ( !isnpc(  ) && level >= LEVEL_KL );
   }
   bool is_pet(  );

   bool IS_PKILL(  )
   {
      return ( has_pcflag( PCFLAG_DEADLY ) );
   }
   bool CAN_PKILL(  )
   {
      return ( IS_PKILL(  ) );
   }
   bool IS_MOUNTED(  )
   {
      return ( position == POS_MOUNTED && mount != nullptr );
   }
   bool CAN_CAST(  )
   {
      return ( ( Class != CLASS_WARRIOR && Class != CLASS_ROGUE && Class != CLASS_MONK ) || is_immortal(  ) );
   }
   bool IS_GOOD(  )
   {
      return ( alignment >= 350 );
   }
   bool IS_EVIL(  )
   {
      return ( alignment <= -350 );
   }
   bool IS_NEUTRAL(  )
   {
      return ( !IS_GOOD(  ) && !IS_EVIL(  ) );
   }
   bool IS_AWAKE(  )
   {
      return ( position > POS_SLEEPING );
   }
   void WAIT_STATE( short npulse )
   {
      ( wait = umax( wait, ( is_immortal(  )? 0 : npulse ) ) );
   }
   bool IS_FLOATING(  )
   {
      return ( has_aflag( AFF_FLYING ) || has_aflag( AFF_FLOATING ) );
   }
   int LEARNED( int sn )
   {
      return ( isnpc(  )? 80 : urange( 0, pcdata->learned[sn], 101 ) );
   }

   void CHECK_SUBRESTRICTED(  )
   {
      if( substate == SUB_RESTRICTED )
      {
         print( "You cannot use this command from within another command.\r\n" );
         return;
      }
   }

   std::map<int, std::string> abits; /* abit/qbit code */
   std::list<char_data *> pets;
   std::list<obj_data *> carrying;
   std::list<affect_data *> affects;
   std::list<timer_data *> timers;
   std::list<class mprog_act_list *> mpact;  /* Mudprogs */
   std::list<struct variable_data *> variables;  // Quest flags
   std::vector<std::string> bodypart_where_names; /* Body part wear messages */
   std::string spec_funname;
   char_data *master = nullptr;
   char_data *leader = nullptr;
   char_data *reply = nullptr;
   char_data *switched = nullptr;
   char_data *mount = nullptr;
   char_data *my_skyship = nullptr;  /* Bond skyship to player */
   char_data *my_rider = nullptr; /* Bond player to skyship */
   pc_data *pcdata = nullptr;  /* For data only players will have */
   descriptor_data *desc = nullptr;  /* A player's connection data */
   mob_index *pIndexData = nullptr;  /* Pointer to the mob index class for an NPC */
   obj_data *on = nullptr;  /* Xerves' Furniture Code - Samson 7-20-00 */
   room_index *in_room = nullptr;
   room_index *was_in_room = nullptr;
   room_index *orig_room = nullptr;  /* Xorith's boards */
   class ship_data *on_ship = nullptr; /* Ship char is on, or nullptr if not - Samson 1-6-00 */
   struct fighting_data *fighting = nullptr;
   struct hunt_hate_fear *hunting = nullptr;
   struct hunt_hate_fear *fearing = nullptr;
   struct hunt_hate_fear *hating = nullptr;
   class char_morph *morph = nullptr;
   class continent_data *continent = nullptr;  /* Which map are they on? - Samson 8-3-99 */
   DO_FUN *last_cmd = nullptr;
   DO_FUN *prev_cmd = nullptr; /* mapping */
   SPEC_FUN *spec_fun = nullptr;
 private:
   std::bitset<MAX_ACT_FLAG> actflags;
   std::bitset<MAX_AFFECTED_BY> affected_by;
   std::bitset<MAX_AFFECTED_BY> no_affected_by;
   std::bitset<MAX_ATTACK_TYPE> attacks;
   std::bitset<MAX_DEFENSE_TYPE> defenses;
   std::bitset<MAX_BPART> body_parts;
   std::bitset<MAX_RIS_FLAG> resistant;
   std::bitset<MAX_RIS_FLAG> no_resistant;
   std::bitset<MAX_RIS_FLAG> immune;
   std::bitset<MAX_RIS_FLAG> no_immune;
   std::bitset<MAX_RIS_FLAG> susceptible;
   std::bitset<MAX_RIS_FLAG> no_susceptible;
   std::bitset<MAX_RIS_FLAG> absorb;  /* Absorption flag for RIS data - Samson 3-16-00 */
   std::bitset<LANG_UNKNOWN> speaks;
 public:
   char *name = nullptr;
   char *short_descr = nullptr;
   char *long_descr = nullptr;
   char *chardesc = nullptr;
   char *alloc_ptr = nullptr;  /* Must strdup and free this one */
   float numattacks = 0.0;
   int speaking = 0;  /* Don't bitset this - it should only be a single language at a time */
   int mpactnum = 0;
   int tempnum = 0;
   int gold = 0;
   int exp = 0;
   int carry_weight = 0;
   int carry_number = 0;
   int home_vnum = -1; /* For sentinel mobs only, used during hotboot world save - Samson 4-1-01 */
   int zzzzz = 0;  /* skyship is idling      */
   int dcoordx = 0;   /* Destination X coord   */
   int dcoordy = 0;   /* Destination Y coord   */
   int lcoordx = 0;   /* Launch X coord  */
   int lcoordy = 0;   /* Launch Y coord  */
   int heading = 0;   /* The skyship's directional heading */
   int resetvnum = -1;
   int resetnum = -1;
   short substate = 0;
   short num_fighting = 0;
   short sex = 0;
   short Class = 0;
   short race = 0;
   short level = 0;
   short trust = 0;
   short timer = 0;
   short wait = 0;
   short hit = 0;
   short max_hit = 0;
   short hit_regen = 0;
   short mana = 0;
   short max_mana = 0;
   short mana_regen = 0;
   short move = 0;
   short max_move = 0;
   short move_regen = 0;
   short spellfail = 0;
   short amp = 0;
   short saving_poison_death = 0;
   short saving_wand = 0;
   short saving_para_petri = 0;
   short saving_breath = 0;
   short saving_spell_staff = 0;
   short alignment = 0;
   short barenumdie = 0;
   short baresizedie = 0;
   short mobthac0 = 0;
   short hitroll = 0;
   short damroll = 0;
   short hitplus = 0;
   short damplus = 0;
   short position = 0;
   short defposition = 0;
   short style = 0;
   short height = 0;
   short weight = 0;
   short armor = 0;
   short wimpy = 0;
   short perm_str = 0;
   short perm_int = 0;
   short perm_wis = 0;
   short perm_dex = 0;
   short perm_con = 0;
   short perm_cha = 0;
   short perm_lck = 0;
   short mod_str = 0;
   short mod_int = 0;
   short mod_wis = 0;
   short mod_dex = 0;
   short mod_con = 0;
   short mod_cha = 0;
   short mod_lck = 0;
   short mental_state = 0;  /* simplified */
   short mobinvis = 0;   /* Mobinvis level SB */
   short map_x = 0;   /* Coordinates on the overland map - Samson 7-31-99 */
   short map_y = 0;
   short sector = -1;  /* Type of terrain to restrict a wandering mob to on overland - Samson 7-27-00 */
   unsigned short mpscriptpos = 0;
   bool has_skyship = false; /* Identifies has skyship */
   bool inflight= false; /* skyship is in flight   */
   bool backtracking = false;   /* Unsafe landing flag   */
};

extern std::list<char_data *> charlist;
extern std::list<char_data *> pclist;
extern char_data *quitting_char;
extern char_data *loading_char;
extern char_data *saving_char;
const std::string PERS( char_data *, char_data *, bool );
