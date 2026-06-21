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
 *                     Completely Revised Boards Module                     *
 ****************************************************************************
 * Revised by:   Xorith                                                     *
 * Last Updated: 10/2/03                                                    *
 * New Functionality added: 9/23/07 - Xorith                                *
 ****************************************************************************/

#pragma once

inline constexpr std::string_view BOARD_LIST_FILE = "../boards/boards.lst"; // List of board files.
inline constexpr std::string_view PROJECTS_FILE = "../system/projects.txt"; // For projects.

constexpr int MAX_REPLY = 10;         // How many messages in each level?
constexpr int MAX_BOARD_EXPIRE = 180; // Max days notes have to live.
constexpr int BD_IGNORE = 2;
constexpr int BD_ANNOUNCE = 1;

enum bflags
{
   BOARD_R1, BOARD_R2, BOARD_PRIVATE, BOARD_ANNOUNCE, MAX_BOARD_FLAGS
};

enum note_flag_types
{
   NOTE_R1, NOTE_STICKY, NOTE_CLOSED, NOTE_HIDDEN, MAX_NOTE_FLAGS
};

enum note_types
{
   NOTE_BOARD, NOTE_PROJECT, NOTE_PLAYER, NOTE_OLCMAP
};

/* Note Data */
class note_data
{
 private:
   note_data( const note_data& n );
   note_data& operator=( const note_data& );

 public:
   note_data();
   ~note_data();

   note_data *parent = nullptr;                       // Parent for this note.
   std::list<note_data*> rlist;                       // List of replies to this note.
   std::bitset<MAX_NOTE_FLAGS> flags;                 // Note Flags
   std::chrono::system_clock::time_point date_stamp;  // Timestamp when the note was written.
   std::chrono::system_clock::time_point expire;      // When this note expires and will be deleted.
   std::string sender;                                // Who sent the note.
   std::string subject;                               // The subject of the note.
   std::string to_list;                               // Intended recipients of the note.
   std::string text;                                  // The full text of the note.
   short reply_count = 0;                             // Keep a count of our replies.
   short type;                                        // Is this note for a board, project, or something else?
};

class board_data
{
 private:
   board_data( const board_data& b );
   board_data& operator=( const board_data& );

 public:
   board_data();
   ~board_data();

   std::list<note_data*> nlist; /* List of notes on the board */
   std::bitset<MAX_BOARD_FLAGS> flags; /* Board Flags */
   std::chrono::system_clock::time_point expire;  /* The time when the note will expire. */
   std::string name;             // Name of Board
   std::string filename;         // Filename for the board. Set automatically based on the board's name.
   std::string desc;             // Short description of the board
   std::string readers;          // Readers
   std::string posters;          // Posters
   std::string moderators;       // Moderators of this board.
   std::string group;            // In-Game organization that 'owns' the board.
   int objvnum = 0;              // Object Vnum of a physical board.
   short read_level = 0;         // Minimum Level to read this board.
   short post_level = 0;         // Minimum Level to post this board.
   short remove_level = 0;       // Minimum Level to remove a post.
   short msg_count = 0;          // Quick count of messages.
};

/* Project Data */
class project_data
{
 private:
   project_data( const project_data& p );
   project_data& operator=( const project_data& );

 public:
   project_data();
   ~project_data();

   std::list<note_data*> nlist;  // List of note logs for the project.
   std::chrono::system_clock::time_point date_stamp;  // Time the project was created.
   std::string name;             // Name of the person who posted the log entry.
   std::string owner;            // Who owns the project?
   std::string coder;            // Coder assigned to the project.
   std::string realm_name;       // Realm this project belongs to.
   std::string status;           // Status of the project.
   std::string description;      // A detailed description of the project.
   bool taken = false;           // Has someone taken project?
};

class board_chardata
{
 private:
   board_chardata( const board_chardata& b );
   board_chardata& operator=( const board_chardata& );

 public:
   board_chardata();
   ~board_chardata();

   std::string board_name;
   std::chrono::system_clock::time_point last_read;
   short alert = 0;
};
