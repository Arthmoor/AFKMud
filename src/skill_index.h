/****************************************************************************
 *                   ^     +----- |  / ^     ^ |     | +-\                  *
 *                  / \    |      | /  |\   /| |     | |  \                 *
 *                 /   \   +---   |<   | \ / | |     | |  |                 *
 *                /-----\  |      | \  |  v  | |     | |  /                 *
 *               /       \ |      |  \ |     | +-----+ +-/                  *
 ****************************************************************************
 * AFKMud Copyright 1997-2020 by Roger Libiez (Samson),                     *
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
 *                     Skill Search Index Definitions                       *
 ****************************************************************************/

// This probably isn't necessary, but prefer to sort by the contents of the
// string rather than the binary value that std::less uses.
class string_sort
{
 public:
   string_sort(  );
   bool operator(  ) ( const string &, const string & ) const;
};

#if defined(__APPLE__)
//Just deciding to go with default std:less for MacOSX
typedef map < string, int > SKILL_INDEX;
#else
typedef map < string, int, string_sort > SKILL_INDEX;
#endif

extern SKILL_INDEX skill_table__index;
extern SKILL_INDEX skill_table__spell;
extern SKILL_INDEX skill_table__skill;
extern SKILL_INDEX skill_table__racial;
extern SKILL_INDEX skill_table__combat;
extern SKILL_INDEX skill_table__tongue;
extern SKILL_INDEX skill_table__lore;

class find__skill_prefix
{
 public:
   string value;
   char_data *actor;

     find__skill_prefix( char_data *, const string & );
   bool operator(  ) ( pair < string, int >compare );
};

class find__skill_exact
{
 public:
   string value;
   char_data *actor;

     find__skill_exact( char_data *, const string & );
   bool operator(  ) ( pair < string, int >compare );
};
