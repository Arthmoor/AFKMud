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
 *                              Chess Module                                *
 ****************************************************************************/

#ifndef __CHESS_H__
#define __CHESS_H__

struct game_board_data
{
   ~game_board_data(  );
   game_board_data(  );

   string player1;
   string player2;
   int board[8][8];
   int turn;
   int type;
};

void free_game( game_board_data * );

#define NO_PIECE	0

#define WHITE_PAWN	1
#define WHITE_ROOK	2
#define WHITE_KNIGHT	3
#define WHITE_BISHOP	4
#define WHITE_QUEEN	5
#define WHITE_KING	6

#define BLACK_PAWN	7
#define BLACK_ROOK	8
#define BLACK_KNIGHT	9
#define BLACK_BISHOP	10
#define BLACK_QUEEN	11
#define BLACK_KING	12

#define MAX_PIECES	13

#define IS_WHITE(x) ((x) >= WHITE_PAWN && (x) <= WHITE_KING)
#define IS_BLACK(x) ((x) >= BLACK_PAWN && (x) <= BLACK_KING)

#define MOVE_OK		0
#define MOVE_INVALID	1
#define MOVE_BLOCKED	2
#define MOVE_TAKEN	3
#define MOVE_CHECKMATE	4
#define MOVE_OFFBOARD	5
#define MOVE_SAMECOLOR	6
#define MOVE_CHECK	8
#define MOVE_WRONGCOLOR	9
#define MOVE_INCHECK	10

#define TYPE_LOCAL	1
#endif
