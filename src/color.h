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
 *               Color Module -- Allow user customizable Colors.            *
 *                                   --Matthew                              *
 *                      Enhanced ANSI parser by Samson                      *
 ****************************************************************************/

#pragma once

std::string color_align( const std::string &, int, int );
int color_strlen( std::string_view );
std::string colorize( std::string_view, descriptor_data * );
std::string colorcode( std::string_view, descriptor_data *, size_t & );

/*
 * Color Alignment Parameters
 */
constexpr int ALIGN_LEFT = 1;
constexpr int ALIGN_CENTER = 2;
constexpr int ALIGN_RIGHT = 3;

/* These are the ANSI codes for foreground text colors */
#define ANSI_BLACK   "\033[0;30m"
#define ANSI_DRED    "\033[0;31m"
#define ANSI_DGREEN  "\033[0;32m"
#define ANSI_ORANGE  "\033[0;33m"
#define ANSI_DBLUE   "\033[0;34m"
#define ANSI_PURPLE  "\033[0;35m"
#define ANSI_CYAN    "\033[0;36m"
#define ANSI_GREY    "\033[0;37m"
#define ANSI_DGREY   "\033[1;30m"
#define ANSI_RED     "\033[1;31m"
#define ANSI_GREEN   "\033[1;32m"
#define ANSI_YELLOW  "\033[1;33m"
#define ANSI_BLUE    "\033[1;34m"
#define ANSI_PINK    "\033[1;35m"
#define ANSI_LBLUE   "\033[1;36m"
#define ANSI_WHITE   "\033[1;37m"

/* These are the ANSI codes for blinking foreground text colors */
#define BLINK_BLACK  "\033[0;5;30m"
#define BLINK_DRED   "\033[0;5;31m"
#define BLINK_DGREEN "\033[0;5;32m"
#define BLINK_ORANGE "\033[0;5;33m"
#define BLINK_DBLUE  "\033[0;5;34m"
#define BLINK_PURPLE "\033[0;5;35m"
#define BLINK_CYAN   "\033[0;5;36m"
#define BLINK_GREY   "\033[0;5;37m"
#define BLINK_DGREY  "\033[1;5;30m"
#define BLINK_RED    "\033[1;5;31m"
#define BLINK_GREEN  "\033[1;5;32m"
#define BLINK_YELLOW "\033[1;5;33m"
#define BLINK_BLUE   "\033[1;5;34m"
#define BLINK_PINK   "\033[1;5;35m"
#define BLINK_LBLUE  "\033[1;5;36m"
#define BLINK_WHITE  "\033[1;5;37m"

/* These are the ANSI codes for background colors */
#define BACK_BLACK   "\033[40m"
#define BACK_DRED    "\033[41m"
#define BACK_DGREEN  "\033[42m"
#define BACK_ORANGE  "\033[43m"
#define BACK_DBLUE   "\033[44m"
#define BACK_PURPLE  "\033[45m"
#define BACK_CYAN    "\033[46m"
#define BACK_GREY    "\033[47m"

/* Other miscellaneous ANSI tags that can be used */
#define ANSI_RESET      "\033[0m"   /* Reset to terminal default */
#define ANSI_BOLD       "\033[1m"   /* For bright color stuff */
#define ANSI_ITALIC     "\033[3m"   /* Italic text */
#define ANSI_UNDERLINE  "\033[4m"   /* Underline text */
#define ANSI_BLINK      "\033[5m"   /* Blinking text */
#define ANSI_REVERSE    "\033[7m"   /* Reverse colors */
#define ANSI_STRIKEOUT  "\033[9m"   /* Overstrike line */

constexpr int AT_BLACK = 0;
constexpr int AT_BLOOD = 1;
constexpr int AT_DGREEN = 2;
constexpr int AT_ORANGE = 3;
constexpr int AT_DBLUE = 4;
constexpr int AT_PURPLE = 5;
constexpr int AT_CYAN = 6;
constexpr int AT_GREY = 7;
constexpr int AT_DGREY = 8;
constexpr int AT_RED = 9;
constexpr int AT_GREEN = 10;
constexpr int AT_YELLOW = 11;
constexpr int AT_BLUE = 12;
constexpr int AT_PINK = 13;
constexpr int AT_LBLUE = 14;
constexpr int AT_WHITE = 15;
constexpr int AT_BLINK = 16;

/* These should be 17 - 32 normaly */
constexpr int AT_BLACK_BLINK = AT_BLACK + AT_BLINK;   // 17
constexpr int AT_BLOOD_BLINK = AT_BLOOD + AT_BLINK;   // 18
constexpr int AT_DGREEN_BLINK = AT_DGREEN + AT_BLINK; // 19
constexpr int AT_ORANGE_BLINK = AT_ORANGE + AT_BLINK; // 20
constexpr int AT_DBLUE_BLINK = AT_DBLUE + AT_BLINK;   // 21
constexpr int AT_PURPLE_BLINK = AT_PURPLE + AT_BLINK; // 22
constexpr int AT_CYAN_BLINK = AT_CYAN + AT_BLINK;     // 23
constexpr int AT_GREY_BLINK = AT_GREY + AT_BLINK;     // 24
constexpr int AT_DGREY_BLINK = AT_DGREY + AT_BLINK;   // 25
constexpr int AT_RED_BLINK = AT_RED + AT_BLINK;       // 26
constexpr int AT_GREEN_BLINK = AT_GREEN + AT_BLINK;   // 27
constexpr int AT_YELLOW_BLINK = AT_YELLOW + AT_BLINK; // 28
constexpr int AT_BLUE_BLINK = AT_BLUE + AT_BLINK;     // 29
constexpr int AT_PINK_BLINK = AT_PINK + AT_BLINK;     // 30
constexpr int AT_LBLUE_BLINK = AT_LBLUE + AT_BLINK;   // 31
constexpr int AT_WHITE_BLINK = AT_WHITE + AT_BLINK;   // 32

constexpr int AT_PLAIN = 33;
constexpr int AT_ACTION = 34;
constexpr int AT_SAY = 35;
constexpr int AT_GOSSIP = 36;
constexpr int AT_YELL = 37;
constexpr int AT_TELL = 38;
constexpr int AT_HIT = 39;
constexpr int AT_HITME = 40;
constexpr int AT_IMMORT = 41;
constexpr int AT_HURT = 42;
constexpr int AT_FALLING = 43;
constexpr int AT_DANGER = 44;
constexpr int AT_MAGIC = 45;
constexpr int AT_CONSIDER = 46;
constexpr int AT_REPORT = 47;
constexpr int AT_POISON = 48;
constexpr int AT_SOCIAL = 49;
constexpr int AT_DYING = 50;
constexpr int AT_DEAD = 51;
constexpr int AT_SKILL = 52;
constexpr int AT_CARNAGE = 53;
constexpr int AT_DAMAGE = 54;
constexpr int AT_FLEE = 55;
constexpr int AT_RMNAME = 56;
constexpr int AT_RMDESC = 57;
constexpr int AT_OBJECT = 58;
constexpr int AT_PERSON = 59;
constexpr int AT_LIST = 60;
constexpr int AT_BYE = 61;
constexpr int AT_GOLD = 62;
constexpr int AT_GTELL = 63;
constexpr int AT_NOTE = 64;
constexpr int AT_HUNGRY = 65;
constexpr int AT_THIRSTY = 66;
constexpr int AT_FIRE = 67;
constexpr int AT_SOBER = 68;
constexpr int AT_WEAROFF = 69;
constexpr int AT_EXITS = 70;
constexpr int AT_SCORE = 71;
constexpr int AT_RESET = 72;
constexpr int AT_LOG = 73;
constexpr int AT_DIEMSG = 74;
constexpr int AT_WARTALK = 75;
constexpr int AT_ARENA = 76;
constexpr int AT_MUSE = 77;
constexpr int AT_THINK = 78;
constexpr int AT_AFLAGS = 79;  /* Added by Samson 9-29-98 for area flag display line */
constexpr int AT_WHO = 80;  /* Added by Samson 9-29-98 for wholist */
constexpr int AT_RACETALK = 81;   /* Added by Samson 9-29-98 for version 1.4 code */
constexpr int AT_IGNORE = 82;  /* Added by Samson 9-29-98 for version 1.4 code */
constexpr int AT_WHISPER = 83; /* Added by Samson 9-29-98 for version 1.4 code */
constexpr int AT_DIVIDER = 84; /* Added by Samson 9-29-98 for version 1.4 code */
constexpr int AT_MORPH = 85;   /* Added by Samson 9-29-98 for version 1.4 code */
constexpr int AT_SHOUT = 86;   /* Added by Samson 9-29-98 for shout channel */
constexpr int AT_RFLAGS = 87;  /* Added by Samson 12-20-98 for room flag display line */
constexpr int AT_STYPE = 88;   /* Added by Samson 12-20-98 for sector display line */
constexpr int AT_ANAME = 89;   /* Added by Samson 12-20-98 for filename display line */
constexpr int AT_AUCTION = 90; /* Added by Samson 12-25-98 for auction channel */
constexpr int AT_SCORE2 = 91;  /* Added by Samson 2-3-99 for DOTD code */
constexpr int AT_SCORE3 = 92;  /* Added by Samson 2-3-99 for DOTD code */
constexpr int AT_SCORE4 = 93;  /* Added by Samson 2-3-99 for DOTD code */
constexpr int AT_WHO2 = 94; /* Added by Samson 2-3-99 for DOTD code */
constexpr int AT_WHO3 = 95; /* Added by Samson 2-3-99 for DOTD code */
constexpr int AT_WHO4 = 96; /* Added by Samson 2-3-99 for DOTD code */
constexpr int AT_unused2 = 97;
constexpr int AT_HELP = 98; /* Added by Samson 1-15-01 for helpfiles */
constexpr int AT_WHO5 = 99; /* Added by Samson 2-7-01 for guild names on who */
constexpr int AT_SCORE5 = 100;  /* Added by Samson 1-14-02 */
constexpr int AT_WHO6 = 101;   /* Added by Samson 1-14-02 */
constexpr int AT_WHO7 = 102;   /* Added by Samson 1-14-02 */
constexpr int AT_PRAC = 103;   /* Added by Samson 1-21-02 */
constexpr int AT_PRAC2 = 104;  /* Added by Samson 1-21-02 */
constexpr int AT_PRAC3 = 105;  /* Added by Samson 1-21-02 */
constexpr int AT_PRAC4 = 106;  /* Added by Samson 1-21-02 */
constexpr int AT_unused = 107; /* Added by Samson 2-27-02 */
constexpr int AT_GUILDTALK = 108; /* Added by Tarl 28 Nov 02 */
constexpr int AT_BOARD = 109;  /* Samson 10-14-03 */
constexpr int AT_BOARD2 = 110; /* Samson 10-14-03 */
constexpr int AT_BOARD3 = 111; /* Samson 10-14-03 */

/* Should ALWAYS be one more than the last numerical value in the list */
constexpr int MAX_COLORS = 112;

extern const short default_set[MAX_COLORS];
