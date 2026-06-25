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
 *                              Chess Module                                *
 ****************************************************************************/

#pragma once

constexpr int NO_PIECE = 0;

constexpr int WHITE_PAWN = 1;
constexpr int WHITE_ROOK = 2;
constexpr int WHITE_KNIGHT = 3;
constexpr int WHITE_BISHOP = 4;
constexpr int WHITE_QUEEN = 5;
constexpr int WHITE_KING = 6;

constexpr int BLACK_PAWN = 7;
constexpr int BLACK_ROOK = 8;
constexpr int BLACK_KNIGHT = 9;
constexpr int BLACK_BISHOP = 10;
constexpr int BLACK_QUEEN = 11;
constexpr int BLACK_KING = 12;

constexpr int MAX_PIECES = 13;

constexpr int MOVE_OK = 0;
constexpr int MOVE_INVALID = 1;
constexpr int MOVE_BLOCKED = 2;
constexpr int MOVE_TAKEN = 3;
constexpr int MOVE_CHECKMATE = 4;
constexpr int MOVE_OFFBOARD = 5;
constexpr int MOVE_SAMECOLOR = 6;
// What happened to 7?
constexpr int MOVE_CHECK = 8;
constexpr int MOVE_WRONGCOLOR = 9;
constexpr int MOVE_INCHECK = 10;

constexpr int TYPE_LOCAL = 1;

struct game_board_data
{
   ~game_board_data(  );
   game_board_data(  );

   std::string player1;    // Name of Player 1.
   std::string player2;    // Name of Player 2.
   int board[8][8];        // The actual board.
   int turn = 0;           // How many moves have been played so far.
   int type = TYPE_LOCAL;  // The type of game. This module used to be IMC2 aware until those networks all went offline.
};

void free_game( game_board_data * );
