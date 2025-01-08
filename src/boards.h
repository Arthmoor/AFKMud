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

#ifndef __BOARDS_H__
#define __BOARDS_H__

#define BOARD_FILE "boards.lst"  /* New board file */
#define OLD_BOARD_FILE "boards.txt" /* Old board file */
#define PROJECTS_FILE SYSTEM_DIR "projects.txt" /* For projects    */
#define MAX_REPLY 10 /* How many messages in each level? */
#define MAX_BOARD_EXPIRE 180  /* Max days notes have to live. */

#define IS_BOARD_FLAG( board, flag )     (board)->flags.test((flag))
#define TOGGLE_BOARD_FLAG( board, flag ) (board)->flags.flip((flag))

#define IS_NOTE_FLAG( note, flag )     (note)->flags.test((flag))
#define TOGGLE_NOTE_FLAG( note, flag ) (note)->flags.flip((flag))

const int BD_IGNORE = 2;
const int BD_ANNOUNCE = 1;

/* As a safety precaution, players who are writing a note are moved here... */
#define ROOM_VNUM_BOARD ROOM_VNUM_LIMBO

enum bflags
{
   BOARD_R1, BOARD_BU_PRUNED, BOARD_PRIVATE, BOARD_ANNOUNCE, MAX_BOARD_FLAGS
};

enum note_flags
{
   NOTE_R1, NOTE_STICKY, NOTE_CLOSED, NOTE_HIDDEN, MAX_NOTE_FLAGS
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

   list<note_data*> rlist;
   bitset<MAX_NOTE_FLAGS> flags; /* Note Flags */
   note_data *parent;
   char *sender;
   char *to_list;
   char *subject;
   char *text;
   time_t date_stamp;   /* Global Board Use */
   short reply_count;   /* Keep a count of our replies */
   short expire;  /* Global Board Use */
};

class board_data
{
 private:
   board_data( const board_data& b );
   board_data& operator=( const board_data& );

 public:
   board_data();
   ~board_data();

   list<note_data*> nlist; /* List of notes on the board */
   bitset<MAX_BOARD_FLAGS> flags; /* Board Flags */
   char *name; /* Name of Board */
   char *filename;   /* Filename for the board */
   char *desc; /* Short description of the board */
   char *readers; /* Readers */
   char *posters; /* Posers */
   char *moderators; /* Moderators of this board */
   char *group;   /* In-Game organization that 'owns' the board */
   int objvnum;   /* Object Vnum of a physical board */
   short read_level; /* Minimum Level to read this board */
   short post_level; /* Minimum Level to post this board */
   short remove_level;  /* Minimum Level to remove a post */
   short msg_count;  /* Quick count of messages */
   short expire;  /* Days until notes are archived. */
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

   list<note_data*> nlist; /* List of note logs for the project */
   string realm_name; // Realm this project belongs to.
   char *name;
   char *owner;
   char *coder;
   char *status;
   char *description;
   time_t date_stamp;
   bool taken;        // Has someone taken project?
};

class board_chardata
{
 private:
   board_chardata( const board_chardata& b );
   board_chardata& operator=( const board_chardata& );

 public:
   board_chardata();
   ~board_chardata();

   string board_name;
   time_t last_read;
   short alert;
};
#endif
