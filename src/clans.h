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

#ifndef __CLANS_H__
#define __CLANS_H__

#define CLAN_DIR "../clans/"  /* Clan data dir     */
#define CLAN_LIST "clan.lst"  /* List of clans     */

enum clan_types
{
   CLAN_CLAN, CLAN_GUILD
};

#define IS_CLANNED(ch) (!(ch)->isnpc() && (ch)->pcdata->clan && (ch)->pcdata->clan->clan_type != CLAN_GUILD )
#define IS_GUILDED(ch) (!(ch)->isnpc() && (ch)->pcdata->clan && (ch)->pcdata->clan->clan_type == CLAN_GUILD )
#define IS_LEADER(ch)  ( !(ch)->isnpc() && (ch)->pcdata->clan && !str_cmp( (ch)->name, (ch)->pcdata->clan->leader  ) )
#define IS_NUMBER1(ch) ( !(ch)->isnpc() && (ch)->pcdata->clan && !str_cmp( (ch)->name, (ch)->pcdata->clan->number1 ) )
#define IS_NUMBER2(ch) ( !(ch)->isnpc() && (ch)->pcdata->clan && !str_cmp( (ch)->name, (ch)->pcdata->clan->number2 ) )

class roster_data
{
 private:
   roster_data( const roster_data & r );
     roster_data & operator=( const roster_data & );

 public:
     roster_data(  );
    ~roster_data(  );

   string name;
   time_t joined;
   int kills;
   int deaths;
   short Class;
   short level;
};

class clan_data
{
 private:
   clan_data( const clan_data & c );
     clan_data & operator=( const clan_data & );

 public:
     clan_data(  );
    ~clan_data(  );

     list < roster_data * >memberlist;
   string filename;  // Clan filename
   string name;   // Clan name
   string motto;  // Clan motto
   string clandesc;  // A brief description of the clan
   string deity;  // Clan's deity
   string leader; // Head clan leader
   string number1;   // First officer
   string number2;   // Second officer
   string badge;  // Clan badge on who/where/to_room
   string leadrank;  // Leader's rank
   string onerank;   // Number One's rank
   string tworank;   // Number Two's rank
   int pkills[10];   // Number of pkills on behalf of clan
   int pdeaths[10];  // Number of pkills against clan
   int mkills; // Number of mkills on behalf of clan
   int mdeaths;   // Number of clan deaths due to mobs
   int illegal_pk;   // Number of illegal pk's by clan
   int board;  // Vnum of clan board
   int clanobj1;  // Vnum of first clan obj
   int clanobj2;  // Vnum of second clan obj
   int clanobj3;  // Vnum of third clan obj
   int clanobj4;  // Vnum of fourth clan obj
   int clanobj5;  // Vnum of fifth clan obj
   int recall; // Vnum of clan's recall room
   int storeroom; // Vnum of clan's storeroom ( storage, not a vendor )
   int guard1; // Vnum of clan guard type 1
   int guard2; // Vnum of clan guard type 2
   int balance;   // Clan's bank account balance
   int inn; // Vnum for clan's inn if they own one
   int idmob;  // Vnum for clan's Identifying mobile if they have one
   int shopkeeper;   // Vnum for clan shopkeeper if they have one
   int auction;   // Vnum for clan auctioneer
   int bank;   // Vnum for clan banker
   int repair; // Vnum for clan repairsmith
   int forge;  // Vnum for clan blacksmith
   short Class;   // For guilds
   short tithe;   // Percentage of gold sent to clan account after battle
   short clan_type;  // See clan type defines
   short favour;  // Deity's favour upon the clan
   short members; // Number of clan members
   short mem_limit;  // Number of clan members allowed
   short alignment;  // Clan's general alignment
   bool getleader;   // true if they need to have the code provide a new leader
   bool getone;   // true if they need to have the code provide a new number1
   bool gettwo;   // true if they need to have the code provide a new number2
};

extern list < clan_data * >clanlist;

void add_roster( clan_data *, const string &, int, int, int, int );
void update_roster( char_data * );
void remove_roster( clan_data *, const string & );
void save_clan( clan_data * );
void save_clan_storeroom( char_data *, clan_data * );
void delete_clan( char_data *, clan_data * );
void verify_clans(  );
clan_data *get_clan( const string & );
#endif
