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
 *                         Immortal Realms Module                           *
 ****************************************************************************/

#pragma once

inline constexpr std::string_view REALM_DIR = "../realms/";  // Realm data dir
inline constexpr std::string_view REALM_LIST = "realm.lst";  // List of realms

enum realm_types
{
   REALM_NONE, REALM_QUEST, REALM_PR, REALM_QA, REALM_BUILDER, REALM_CODER, REALM_ADMIN, REALM_OWNER, MAX_REALM
};

extern const char *realm_type_names[MAX_REALM];

class realm_roster_data
{
 private:
   realm_roster_data( const realm_roster_data & r );
     realm_roster_data & operator=( const realm_roster_data & );

 public:
     realm_roster_data(  );
    ~realm_roster_data(  );

   std::string name;
   std::chrono::system_clock::time_point joined;
};

class realm_data
{
 private:
   realm_data( const realm_data & r );
     realm_data & operator=( const realm_data & );

 public:
     realm_data(  );
    ~realm_data(  );

   std::list<realm_roster_data *> memberlist;
   std::string filename;  // Realm filename. Specifies the full path on disk.
   std::string name;      // Realm name
   std::string realmdesc; // A brief description of the Realm
   std::string leader;    // Head Realm leader
   std::string badge;     // Realm badge on who/where/to_room
   std::string leadrank;  // Leader's rank
   int board;             // Vnum of Realm board
   short type;            // See realm type defines
   short members;         // Number of members
};

extern std::list<realm_data *> realmlist;

void add_realm_roster( realm_data *, std::string_view );
void remove_realm_roster( realm_data *, std::string_view );
void save_realm( realm_data * );
void delete_realm( char_data *, realm_data * );
void verify_realms(  );
void free_realms( );
realm_data *get_realm( std::string_view );
bool IS_REALM_LEADER( char_data * );
bool IS_ADMIN_REALM( char_data * );
bool IS_OWNER_REALM( char_data * );
