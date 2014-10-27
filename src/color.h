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
 * Original SMAUG 1.4a written by Thoric (Derek Snider) with Altrag,        *
 * Blodkai, Haus, Narn, Scryn, Swordbearer, Tricops, Gorog, Rennard,        *
 * Grishnakh, Fireblade, and Nivek.                                         *
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

#ifndef __COLOR_H__
#define __COLOR_H__

char *color_align( const char *, int, int );
int color_strlen( const char * );
const char *colorize( const string &, descriptor_data * );
int colorcode( const char *src, char *dst, descriptor_data * d, int dstlen, int *vislen );

/*
 * Color Alignment Parameters
 */
const int ALIGN_LEFT = 1;
const int ALIGN_CENTER = 2;
const int ALIGN_RIGHT = 3;

/* These are the ANSI codes for foreground text colors */
#define ANSI_BLACK    	"\033[0;30m"
#define ANSI_DRED    	"\033[0;31m"
#define ANSI_DGREEN     "\033[0;32m"
#define ANSI_ORANGE    	"\033[0;33m"
#define ANSI_DBLUE    	"\033[0;34m"
#define ANSI_PURPLE    	"\033[0;35m"
#define ANSI_CYAN	  	"\033[0;36m"
#define ANSI_GREY		"\033[0;37m"
#define ANSI_DGREY	"\033[1;30m"
#define ANSI_RED		"\033[1;31m"
#define ANSI_GREEN	"\033[1;32m"
#define ANSI_YELLOW   	"\033[1;33m"
#define ANSI_BLUE		"\033[1;34m"
#define ANSI_PINK		"\033[1;35m"
#define ANSI_LBLUE   	"\033[1;36m"
#define ANSI_WHITE   	"\033[1;37m"

/* These are the ANSI codes for blinking foreground text colors */
#define BLINK_BLACK	"\033[0;5;30m"
#define BLINK_DRED	"\033[0;5;31m"
#define BLINK_DGREEN	"\033[0;5;32m"
#define BLINK_ORANGE	"\033[0;5;33m"
#define BLINK_DBLUE	"\033[0;5;34m"
#define BLINK_PURPLE	"\033[0;5;35m"
#define BLINK_CYAN	"\033[0;5;36m"
#define BLINK_GREY	"\033[0;5;37m"
#define BLINK_DGREY	"\033[1;5;30m"
#define BLINK_RED		"\033[1;5;31m"
#define BLINK_GREEN	"\033[1;5;32m"
#define BLINK_YELLOW	"\033[1;5;33m"
#define BLINK_BLUE	"\033[1;5;34m"
#define BLINK_PINK	"\033[1;5;35m"
#define BLINK_LBLUE	"\033[1;5;36m"
#define BLINK_WHITE	"\033[1;5;37m"

/* These are the ANSI codes for background colors */
#define BACK_BLACK 	"\033[40m"
#define BACK_DRED  	"\033[41m"
#define BACK_DGREEN	"\033[42m"
#define BACK_ORANGE     "\033[43m"
#define BACK_DBLUE      "\033[44m"
#define BACK_PURPLE     "\033[45m"
#define BACK_CYAN       "\033[46m"
#define BACK_GREY       "\033[47m"

/* Other miscelaneous ANSI tags that can be used */
#define ANSI_RESET	"\033[0m"   /* Reset to terminal default */
#define ANSI_BOLD		"\033[1m"   /* For bright color stuff */
#define ANSI_ITALIC	"\033[3m"   /* Italic text */
#define ANSI_UNDERLINE  "\033[4m"   /* Underline text */
#define ANSI_BLINK	"\033[5m"   /* Blinking text */
#define ANSI_REVERSE    "\033[7m"   /* Reverse colors */
#define ANSI_STRIKEOUT  "\033[9m"   /* Overstrike line */

const int AT_BLACK = 0;
const int AT_BLOOD = 1;
const int AT_DGREEN = 2;
const int AT_ORANGE = 3;
const int AT_DBLUE = 4;
const int AT_PURPLE = 5;
const int AT_CYAN = 6;
const int AT_GREY = 7;
const int AT_DGREY = 8;
const int AT_RED = 9;
const int AT_GREEN = 10;
const int AT_YELLOW = 11;
const int AT_BLUE = 12;
const int AT_PINK = 13;
const int AT_LBLUE = 14;
const int AT_WHITE = 15;
const int AT_BLINK = 16;

/* These should be 17 - 31 normaly */
const int AT_BLACK_BLINK = AT_BLACK + AT_BLINK;
const int AT_BLOOD_BLINK = AT_BLOOD + AT_BLINK;
const int AT_DGREEN_BLINK = AT_DGREEN + AT_BLINK;
const int AT_ORANGE_BLINK = AT_ORANGE + AT_BLINK;
const int AT_DBLUE_BLINK = AT_DBLUE + AT_BLINK;
const int AT_PURPLE_BLINK = AT_PURPLE + AT_BLINK;
const int AT_CYAN_BLINK = AT_CYAN + AT_BLINK;
const int AT_GREY_BLINK = AT_GREY + AT_BLINK;
const int AT_DGREY_BLINK = AT_DGREY + AT_BLINK;
const int AT_RED_BLINK = AT_RED + AT_BLINK;
const int AT_GREEN_BLINK = AT_GREEN + AT_BLINK;
const int AT_YELLOW_BLINK = AT_YELLOW + AT_BLINK;
const int AT_BLUE_BLINK = AT_BLUE + AT_BLINK;
const int AT_PINK_BLINK = AT_PINK + AT_BLINK;
const int AT_LBLUE_BLINK = AT_LBLUE + AT_BLINK;
const int AT_WHITE_BLINK = AT_WHITE + AT_BLINK;

const int AT_PLAIN = 32;
const int AT_ACTION = 33;
const int AT_SAY = 34;
const int AT_GOSSIP = 35;
const int AT_YELL = 36;
const int AT_TELL = 37;
const int AT_HIT = 38;
const int AT_HITME = 39;
const int AT_IMMORT = 40;
const int AT_HURT = 41;
const int AT_FALLING = 42;
const int AT_DANGER = 43;
const int AT_MAGIC = 44;
const int AT_CONSIDER = 45;
const int AT_REPORT = 46;
const int AT_POISON = 47;
const int AT_SOCIAL = 48;
const int AT_DYING = 49;
const int AT_DEAD = 50;
const int AT_SKILL = 51;
const int AT_CARNAGE = 52;
const int AT_DAMAGE = 53;
const int AT_FLEE = 54;
const int AT_RMNAME = 55;
const int AT_RMDESC = 56;
const int AT_OBJECT = 57;
const int AT_PERSON = 58;
const int AT_LIST = 59;
const int AT_BYE = 60;
const int AT_GOLD = 61;
const int AT_GTELL = 62;
const int AT_NOTE = 63;
const int AT_HUNGRY = 64;
const int AT_THIRSTY = 65;
const int AT_FIRE = 66;
const int AT_SOBER = 67;
const int AT_WEAROFF = 68;
const int AT_EXITS = 69;
const int AT_SCORE = 70;
const int AT_RESET = 71;
const int AT_LOG = 72;
const int AT_DIEMSG = 73;
const int AT_WARTALK = 74;
const int AT_ARENA = 75;
const int AT_MUSE = 76;
const int AT_THINK = 77;
const int AT_AFLAGS = 78;  /* Added by Samson 9-29-98 for area flag display line */
const int AT_WHO = 79;  /* Added by Samson 9-29-98 for wholist */
const int AT_RACETALK = 80;   /* Added by Samson 9-29-98 for version 1.4 code */
const int AT_IGNORE = 81;  /* Added by Samson 9-29-98 for version 1.4 code */
const int AT_WHISPER = 82; /* Added by Samson 9-29-98 for version 1.4 code */
const int AT_DIVIDER = 83; /* Added by Samson 9-29-98 for version 1.4 code */
const int AT_MORPH = 84;   /* Added by Samson 9-29-98 for version 1.4 code */
const int AT_SHOUT = 85;   /* Added by Samson 9-29-98 for shout channel */
const int AT_RFLAGS = 86;  /* Added by Samson 12-20-98 for room flag display line */
const int AT_STYPE = 87;   /* Added by Samson 12-20-98 for sector display line */
const int AT_ANAME = 88;   /* Added by Samson 12-20-98 for filename display line */
const int AT_AUCTION = 89; /* Added by Samson 12-25-98 for auction channel */
const int AT_SCORE2 = 90;  /* Added by Samson 2-3-99 for DOTD code */
const int AT_SCORE3 = 91;  /* Added by Samson 2-3-99 for DOTD code */
const int AT_SCORE4 = 92;  /* Added by Samson 2-3-99 for DOTD code */
const int AT_WHO2 = 93; /* Added by Samson 2-3-99 for DOTD code */
const int AT_WHO3 = 94; /* Added by Samson 2-3-99 for DOTD code */
const int AT_WHO4 = 95; /* Added by Samson 2-3-99 for DOTD code */
const int AT_INTERMUD = 96;   /* Added by Samson 1-15-01 for Intermud3 Channels */
const int AT_HELP = 97; /* Added by Samson 1-15-01 for helpfiles */
const int AT_WHO5 = 98; /* Added by Samson 2-7-01 for guild names on who */
const int AT_SCORE5 = 99;  /* Added by Samson 1-14-02 */
const int AT_WHO6 = 100;   /* Added by Samson 1-14-02 */
const int AT_WHO7 = 101;   /* Added by Samson 1-14-02 */
const int AT_PRAC = 102;   /* Added by Samson 1-21-02 */
const int AT_PRAC2 = 103;  /* Added by Samson 1-21-02 */
const int AT_PRAC3 = 104;  /* Added by Samson 1-21-02 */
const int AT_PRAC4 = 105;  /* Added by Samson 1-21-02 */
const int AT_unused = 106; /* Added by Samson 2-27-02 */
const int AT_GUILDTALK = 107; /* Added by Tarl 28 Nov 02 */
const int AT_BOARD = 108;  /* Samson 10-14-03 */
const int AT_BOARD2 = 109; /* Samson 10-14-03 */
const int AT_BOARD3 = 110; /* Samson 10-14-03 */

/* Should ALWAYS be one more than the last numerical value in the list */
const int MAX_COLORS = 111;

extern const short default_set[MAX_COLORS];
#endif
