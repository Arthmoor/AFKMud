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
 *                           Special clan module                            *
 ****************************************************************************/

#pragma once

inline constexpr std::string_view CLAN_LIST = "clan.lst";  // List of clans.

enum clan_types
{
   CLAN_CLAN, CLAN_GUILD
};

class roster_data
{
 private:
   roster_data( const roster_data & r );
     roster_data & operator=( const roster_data & );

 public:
     roster_data(  );
    ~roster_data(  );

   std::string name;
   std::chrono::system_clock::time_point joined;
   int kills = 0;    // Number of mobs killed by this member. Taken from ch->pcdata->mkills.
   int deaths = 0;   // Number of times this member was killed by mobs. Taken from ch->pcdata->mdeaths.
   short Class = 0;  // Class of this member.
   short level = 0;  // Level of this member.
};

class clan_data
{
 private:
   clan_data( const clan_data & c );
     clan_data & operator=( const clan_data & );

 public:
     clan_data(  );
    ~clan_data(  );

   std::list<roster_data *> memberlist;
   std::string filename;         // Clan filename
   std::string name;             // Clan name
   std::string motto;            // Clan motto
   std::string clandesc;         // A brief description of the clan
   std::string deity;            // Clan's deity
   std::string leader;           // Head clan leader
   std::string number1;          // First officer
   std::string number2;          // Second officer
   std::string badge;            // Clan badge on who/where/to_room
   std::string leadrank;         // Leader's rank
   std::string onerank;          // Number One's rank
   std::string tworank;          // Number Two's rank
   int pkills[10]{0};            // Number of pkills on behalf of clan
   int pdeaths[10]{0};           // Number of pkills against clan
   int mkills = 0;               // Number of mkills on behalf of clan
   int mdeaths = 0;              // Number of clan deaths due to mobs
   int illegal_pk = 0;           // Number of illegal pk's by clan
   int board = 0;                // Vnum of clan board
   int clanobj1 = 0;             // Vnum of first clan obj
   int clanobj2 = 0;             // Vnum of second clan obj
   int clanobj3 = 0;             // Vnum of third clan obj
   int clanobj4 = 0;             // Vnum of fourth clan obj
   int clanobj5 = 0;             // Vnum of fifth clan obj
   int recall = 0;               // Vnum of clan's recall room
   int storeroom = 0;            // Vnum of clan's storeroom ( storage, not a vendor )
   int guard1 = 0;               // Vnum of clan guard type 1
   int guard2 = 0;               // Vnum of clan guard type 2
   int balance = 0;              // Clan's bank account balance
   int inn = 0;                  // Vnum for clan's inn if they own one
   int idmob = 0;                // Vnum for clan's Identifying mobile if they have one
   int shopkeeper = 0;           // Vnum for clan shopkeeper if they have one
   int auction = 0;              // Vnum for clan auctioneer
   int bank = 0;                 // Vnum for clan banker
   int repair = 0;               // Vnum for clan repairsmith
   int forge = 0;                // Vnum for clan blacksmith
   short Class = 0;              // For guilds
   short tithe = 0;              // Percentage of gold sent to clan account after battle
   short clan_type = CLAN_GUILD; // See clan type defines
   short favour = 0;             // Deity's favor upon the clan
   short members = 1;            // Number of clan members
   short mem_limit = 15;         // Number of clan members allowed
   short alignment = 0;          // Clan's general alignment
   bool getleader = false;       // true if they need to have the code provide a new leader
   bool getone = false;          // true if they need to have the code provide a new number1
   bool gettwo = false;          // true if they need to have the code provide a new number2
};

extern std::list<clan_data *> clanlist;

void add_roster( clan_data *, std::string_view, int, int, int, int );
void update_roster( char_data * );
void remove_roster( clan_data *, std::string_view );
void save_clan( clan_data * );
void save_clan_storeroom( char_data *, clan_data * );
void delete_clan( char_data *, clan_data * );
void verify_clans(  );
clan_data *get_clan( std::string_view );
bool IS_CLANNED( char_data * );
bool IS_GUILDED( char_data * );
bool IS_LEADER( char_data * );
bool IS_NUMBER1( char_data * );
bool IS_NUMBER2( char_data * );
