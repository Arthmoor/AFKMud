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

#include "mud.h"
#include "chess.h"

extern char_data *supermob;

#define WHITE_BACKGROUND ""
#define BLACK_BACKGROUND ""
#define WHITE_FOREGROUND ""
#define BLACK_FOREGROUND ""

const char *big_pieces[MAX_PIECES][2] = {
   {"%s       ",
    "%s       "},
   {"%s  (-)  ",
    "%s  -|-  "},
   {"%s  ###  ",
    "%s  { }  "},
   {"%s  /-*- ",
    "%s / /   "},
   {"%s  () + ",
    "%s  {}-| "},
   {"%s   @   ",
    "%s  /+\\  "},
   {"%s ^^^^^ ",
    "%s  {@}  "},
   {"%s  [-]  ",
    "%s  -|-  "},
   {"%s  ###  ",
    "%s  [ ]  "},
   {"%s  /-*- ",
    "%s / /   "},
   {"%s  [] + ",
    "%s  {}-| "},
   {"%s   #   ",
    "%s  /+\\  "},
   {"%s ^^^^^ ",
    "%s  [#]  "}
};

const char small_pieces[MAX_PIECES + 1] = " prnbqkPRNBQK";

game_board_data::~game_board_data(  )
{
}

game_board_data::game_board_data(  )
{
   int x, y;

   for( x = 0; x < 8; ++x )
      for( y = 0; y < 8; ++y )
         this->board[x][y] = 0;

   this->board[0][0] = WHITE_ROOK;
   this->board[0][1] = WHITE_KNIGHT;
   this->board[0][2] = WHITE_BISHOP;
   this->board[0][3] = WHITE_QUEEN;
   this->board[0][4] = WHITE_KING;
   this->board[0][5] = WHITE_BISHOP;
   this->board[0][6] = WHITE_KNIGHT;
   this->board[0][7] = WHITE_ROOK;

   for( x = 0; x < 8; ++x )
      this->board[1][x] = WHITE_PAWN;

   for( x = 0; x < 8; ++x )
      this->board[6][x] = BLACK_PAWN;

   this->board[7][0] = BLACK_ROOK;
   this->board[7][1] = BLACK_KNIGHT;
   this->board[7][2] = BLACK_BISHOP;
   this->board[7][3] = BLACK_QUEEN;
   this->board[7][4] = BLACK_KING;
   this->board[7][5] = BLACK_BISHOP;
   this->board[7][6] = BLACK_KNIGHT;
   this->board[7][7] = BLACK_ROOK;
}

bool IS_WHITE( int x )
{
   if( x >= WHITE_PAWN && x <= WHITE_KING )
      return true;
   return false;
}

bool IS_BLACK( int x )
{
   if( x >= BLACK_PAWN && x <= BLACK_KING )
      return true;
   return false;
}

bool SAME_COLOR( game_board_data * board, int x1, int y1, int x2, int y2 )
{
   if( IS_WHITE( board->board[x1][y1] ) && IS_WHITE( board->board[x2][y2] ) )
      return true;
   if( IS_BLACK( board->board[x1][y1] ) && IS_BLACK( board->board[x2][y2] ) )
      return true;
   return false;
}

std::string print_big_board( game_board_data * board )
{
   std::string buf, buf2;
   int x, y;

   std::string s1 = "&Y&W";
   std::string s2 = "&z&z";

   std::string retbuf = WHITE_FOREGROUND "\r\n&g     1      2      3      4      5      6      7      8\r\n";

   for( x = 0; x < 8; ++x )
   {
      retbuf += "  ";
      for( y = 0; y < 8; ++y )
      {
         buf = std::format( "{}{}",
                   x % 2 == 0 ? ( y % 2 == 0 ? BLACK_BACKGROUND : WHITE_BACKGROUND ) :
                   ( y % 2 == 0 ? WHITE_BACKGROUND : BLACK_BACKGROUND ), big_pieces[board->board[x][y]][0] );
         buf2 = buf;
         buf2.append( IS_WHITE( board->board[x][y] ) ? s1 : s2 );
         retbuf.append( buf2 );
      }
      retbuf.append( BLACK_BACKGROUND "\r\n" );

      buf = std::format( WHITE_FOREGROUND "&g{} ", static_cast<char>( 'A' + x ) );
      retbuf.append( buf );
      for( y = 0; y < 8; ++y )
      {
         buf = std::format( "{}{}",
                   x % 2 == 0 ? ( y % 2 == 0 ? BLACK_BACKGROUND : WHITE_BACKGROUND ) :
                   ( y % 2 == 0 ? WHITE_BACKGROUND : BLACK_BACKGROUND ), big_pieces[board->board[x][y]][1] );
         buf2 = buf;
         buf2.append( IS_WHITE( board->board[x][y] ) ? s1 : s2 );
         retbuf.append( buf2 );
      }
      retbuf.append( BLACK_BACKGROUND "\r\n" );
   }

   return retbuf;
}

bool find_piece( game_board_data * board, int *x, int *y, int piece )
{
   int a, b;

   for( a = 0; a < 8; ++a )
   {
      for( b = 0; b < 8; ++b )
         if( board->board[a][b] == piece )
            break;
      if( board->board[a][b] == piece )
         break;
   }
   *x = a;
   *y = b;
   if( board->board[a][b] == piece )
      return true;
   return false;
}

bool king_in_check( game_board_data * board, int piece )
{
   int x = 0, y = 0, l, m;

   if( piece != WHITE_KING && piece != BLACK_KING )
      return false;

   if( !find_piece( board, &x, &y, piece ) )
      return false;

   if( x < 0 || y < 0 || x > 7 || y > 7 )
      return false;

   /*
    * pawns 
    */
   if( IS_WHITE( piece ) && x < 7 && ( ( y > 0 && IS_BLACK( board->board[x + 1][y - 1] ) ) || ( y < 7 && IS_BLACK( board->board[x + 1][y + 1] ) ) ) )
      return true;
   else if( IS_BLACK( piece ) && x > 0 && ( ( y > 0 && IS_WHITE( board->board[x - 1][y - 1] ) ) || ( y < 7 && IS_WHITE( board->board[x - 1][y + 1] ) ) ) )
      return true;
   /*
    * knights 
    */
   if( x - 2 >= 0 && y - 1 >= 0 &&
       ( ( board->board[x - 2][y - 1] == BLACK_KNIGHT && IS_WHITE( board->board[x][y] ) ) ||
         ( board->board[x - 2][y - 1] == WHITE_KNIGHT && IS_BLACK( board->board[x][y] ) ) ) )
      return true;
   if( x - 2 >= 0 && y + 1 < 8 &&
       ( ( board->board[x - 2][y + 1] == BLACK_KNIGHT && IS_WHITE( board->board[x][y] ) ) ||
         ( board->board[x - 2][y + 1] == WHITE_KNIGHT && IS_BLACK( board->board[x][y] ) ) ) )
      return true;

   if( x - 1 >= 0 && y - 2 >= 0 &&
       ( ( board->board[x - 1][y - 2] == BLACK_KNIGHT && IS_WHITE( board->board[x][y] ) ) ||
         ( board->board[x - 1][y - 2] == WHITE_KNIGHT && IS_BLACK( board->board[x][y] ) ) ) )
      return true;
   if( x - 1 >= 0 && y + 2 < 8 &&
       ( ( board->board[x - 1][y + 2] == BLACK_KNIGHT && IS_WHITE( board->board[x][y] ) ) ||
         ( board->board[x - 1][y + 2] == WHITE_KNIGHT && IS_BLACK( board->board[x][y] ) ) ) )
      return true;

   if( x + 1 < 8 && y - 2 >= 0 &&
       ( ( board->board[x + 1][y - 2] == BLACK_KNIGHT && IS_WHITE( board->board[x][y] ) ) ||
         ( board->board[x + 1][y - 2] == WHITE_KNIGHT && IS_BLACK( board->board[x][y] ) ) ) )
      return true;
   if( x + 1 < 8 && y + 2 < 8 &&
       ( ( board->board[x + 1][y + 2] == BLACK_KNIGHT && IS_WHITE( board->board[x][y] ) ) ||
         ( board->board[x + 1][y + 2] == WHITE_KNIGHT && IS_BLACK( board->board[x][y] ) ) ) )
      return true;

   if( x + 2 < 8 && y - 1 >= 0 &&
       ( ( board->board[x + 2][y - 1] == BLACK_KNIGHT && IS_WHITE( board->board[x][y] ) ) ||
         ( board->board[x + 2][y - 1] == WHITE_KNIGHT && IS_BLACK( board->board[x][y] ) ) ) )
      return true;
   if( x + 2 < 8 && y + 1 < 8 &&
       ( ( board->board[x + 2][y + 1] == BLACK_KNIGHT && IS_WHITE( board->board[x][y] ) ) ||
         ( board->board[x + 2][y + 1] == WHITE_KNIGHT && IS_BLACK( board->board[x][y] ) ) ) )
      return true;

   /*
    * horizontal/vertical long distance 
    */
   for( l = x + 1; l < 8; ++l )
      if( board->board[l][y] != NO_PIECE )
      {
         if( SAME_COLOR( board, x, y, l, y ) )
            break;
         if( board->board[l][y] == BLACK_QUEEN || board->board[l][y] == WHITE_QUEEN || board->board[l][y] == BLACK_ROOK || board->board[l][y] == WHITE_ROOK )
            return true;
         break;
      }
   for( l = x - 1; l >= 0; --l )
      if( board->board[l][y] != NO_PIECE )
      {
         if( SAME_COLOR( board, x, y, l, y ) )
            break;
         if( board->board[l][y] == BLACK_QUEEN || board->board[l][y] == WHITE_QUEEN || board->board[l][y] == BLACK_ROOK || board->board[l][y] == WHITE_ROOK )
            return true;
         break;
      }
   for( m = y + 1; m < 8; ++m )
      if( board->board[x][m] != NO_PIECE )
      {
         if( SAME_COLOR( board, x, y, x, m ) )
            break;
         if( board->board[x][m] == BLACK_QUEEN || board->board[x][m] == WHITE_QUEEN || board->board[x][m] == BLACK_ROOK || board->board[x][m] == WHITE_ROOK )
            return true;
         break;
      }
   for( m = y - 1; m >= 0; --m )
      if( board->board[x][m] != NO_PIECE )
      {
         if( SAME_COLOR( board, x, y, x, m ) )
            break;
         if( board->board[x][m] == BLACK_QUEEN || board->board[x][m] == WHITE_QUEEN || board->board[x][m] == BLACK_ROOK || board->board[x][m] == WHITE_ROOK )
            return true;
         break;
      }
   /*
    * diagonal long distance 
    */
   for( l = x + 1, m = y + 1; l < 8 && m < 8; ++l, ++m )
      if( board->board[l][m] != NO_PIECE )
      {
         if( SAME_COLOR( board, x, y, l, m ) )
            break;
         if( board->board[l][m] == BLACK_QUEEN || board->board[l][m] == WHITE_QUEEN || board->board[l][m] == BLACK_BISHOP || board->board[l][m] == WHITE_BISHOP )
            return true;
         break;
      }
   for( l = x - 1, m = y + 1; l >= 0 && m < 8; --l, ++m )
      if( board->board[l][m] != NO_PIECE )
      {
         if( SAME_COLOR( board, x, y, l, m ) )
            break;
         if( board->board[l][m] == BLACK_QUEEN || board->board[l][m] == WHITE_QUEEN || board->board[l][m] == BLACK_BISHOP || board->board[l][m] == WHITE_BISHOP )
            return true;
         break;
      }
   for( l = x + 1, m = y - 1; l < 8 && m >= 0; ++l, --m )
      if( board->board[l][m] != NO_PIECE )
      {
         if( SAME_COLOR( board, x, y, l, m ) )
            break;
         if( board->board[l][m] == BLACK_QUEEN || board->board[l][m] == WHITE_QUEEN || board->board[l][m] == BLACK_BISHOP || board->board[l][m] == WHITE_BISHOP )
            return true;
         break;
      }
   for( l = x - 1, m = y - 1; l >= 0 && m >= 0; --l, --m )
      if( board->board[l][m] != NO_PIECE )
      {
         if( SAME_COLOR( board, x, y, l, m ) )
            break;
         if( board->board[l][m] == BLACK_QUEEN || board->board[l][m] == WHITE_QUEEN || board->board[l][m] == BLACK_BISHOP || board->board[l][m] == WHITE_BISHOP )
            return true;
         break;
      }
   return false;
}

bool king_in_checkmate( game_board_data * board, int piece )
{
   int x = 0, y = 0, dx, dy, sk = 0;

   if( piece != WHITE_KING && piece != BLACK_KING )
      return false;

   if( !find_piece( board, &x, &y, piece ) )
      return false;

   if( x < 0 || y < 0 || x > 7 || y > 7 )
      return false;

   if( !king_in_check( board, board->board[x][y] ) )
      return false;

   dx = x + 1;
   dy = y + 1;
   if( dx < 8 && dy < 8 && board->board[dx][dy] == NO_PIECE )
   {
      sk = board->board[dx][dy] = board->board[x][y];
      board->board[x][y] = NO_PIECE;
      if( !king_in_check( board, sk ) )
      {
         board->board[x][y] = sk;
         board->board[dx][dy] = NO_PIECE;
         return false;
      }
      board->board[x][y] = sk;
      board->board[dx][dy] = NO_PIECE;
   }
   dx = x - 1;
   dy = y + 1;
   if( dx >= 0 && dy < 8 && board->board[dx][dy] == NO_PIECE )
   {
      sk = board->board[dx][dy] = board->board[x][y];
      board->board[x][y] = NO_PIECE;
      if( !king_in_check( board, sk ) )
      {
         board->board[x][y] = sk;
         board->board[dx][dy] = NO_PIECE;
         return false;
      }
      board->board[x][y] = sk;
      board->board[dx][dy] = NO_PIECE;
   }
   dx = x + 1;
   dy = y - 1;
   if( dx < 8 && dy >= 0 && board->board[dx][dy] == NO_PIECE )
   {
      sk = board->board[dx][dy] = board->board[x][y];
      board->board[x][y] = NO_PIECE;
      if( !king_in_check( board, sk ) )
      {
         board->board[x][y] = sk;
         board->board[dx][dy] = NO_PIECE;
         return false;
      }
      board->board[x][y] = sk;
      board->board[dx][dy] = NO_PIECE;
   }
   dx = x - 1;
   dy = y - 1;
   if( dx >= 0 && dy >= 0 && board->board[dx][dy] == NO_PIECE )
   {
      sk = board->board[dx][dy] = board->board[x][y];
      board->board[x][y] = NO_PIECE;
      if( !king_in_check( board, sk ) )
      {
         board->board[x][y] = sk;
         board->board[dx][dy] = NO_PIECE;
         return false;
      }
      board->board[x][y] = sk;
      board->board[dx][dy] = NO_PIECE;
   }
   dx = x;
   dy = y + 1;
   if( dy < 8 && board->board[dx][dy] == NO_PIECE )
   {
      sk = board->board[dx][dy] = board->board[x][y];
      board->board[x][y] = NO_PIECE;
      if( !king_in_check( board, sk ) )
      {
         board->board[x][y] = sk;
         board->board[dx][dy] = NO_PIECE;
         return false;
      }
      board->board[x][y] = sk;
      board->board[dx][dy] = NO_PIECE;
   }
   dx = x;
   dy = y - 1;
   if( dy >= 0 && board->board[dx][dy] == NO_PIECE )
   {
      sk = board->board[dx][dy] = board->board[x][y];
      board->board[x][y] = NO_PIECE;
      if( !king_in_check( board, sk ) )
      {
         board->board[x][y] = sk;
         board->board[dx][dy] = NO_PIECE;
         return false;
      }
      board->board[x][y] = sk;
      board->board[dx][dy] = NO_PIECE;
   }
   dx = x + 1;
   dy = y;
   if( dx < 8 && board->board[dx][dy] == NO_PIECE )
   {
      sk = board->board[dx][dy] = board->board[x][y];
      board->board[x][y] = NO_PIECE;
      if( !king_in_check( board, sk ) )
      {
         board->board[x][y] = sk;
         board->board[dx][dy] = NO_PIECE;
         return false;
      }
      board->board[x][y] = sk;
      board->board[dx][dy] = NO_PIECE;
   }
   dx = x - 1;
   dy = y;
   if( dx >= 0 && board->board[dx][dy] == NO_PIECE )
   {
      sk = board->board[dx][dy] = board->board[x][y];
      board->board[x][y] = NO_PIECE;
      if( !king_in_check( board, sk ) )
      {
         board->board[x][y] = sk;
         board->board[dx][dy] = NO_PIECE;
         return false;
      }
      board->board[x][y] = sk;
      board->board[dx][dy] = NO_PIECE;
   }
   return true;
}

int is_valid_move( char_data * ch, game_board_data * board, int x, int y, int dx, int dy )
{
   if( !ch )
   {
      bug( "%s: nullptr ch!", __func__ );
      return MOVE_INVALID;
   }

   if( !board )
   {
      bug( "%s: nullptr board!", __func__ );
      return MOVE_INVALID;
   }

   if( dx < 0 || dy < 0 || dx > 7 || dy > 7 )
      return MOVE_OFFBOARD;

   if( board->board[x][y] == NO_PIECE )
      return MOVE_INVALID;

   if( x == dx && y == dy )
      return MOVE_INVALID;

   if( IS_WHITE( board->board[x][y] ) && !str_cmp( board->player1, ch->name ) )
      return MOVE_WRONGCOLOR;
   if( IS_BLACK( board->board[x][y] ) && ( !ch || !str_cmp( board->player2, ch->name ) ) )
      return MOVE_WRONGCOLOR;

   switch ( board->board[x][y] )
   {
      case WHITE_PAWN:
      case BLACK_PAWN:
         if( IS_WHITE( board->board[x][y] ) && dx == x + 2 && x == 1 && dy == y && board->board[dx][dy] == NO_PIECE && board->board[x + 1][dy] == NO_PIECE )
            return MOVE_OK;
         else if( IS_BLACK( board->board[x][y] ) && dx == x - 2 && x == 6 && dy == y && board->board[dx][dy] == NO_PIECE && board->board[x - 1][dy] == NO_PIECE )
            return MOVE_OK;
         if( IS_WHITE( board->board[x][y] ) && dx != x + 1 )
            return MOVE_INVALID;
         else if( IS_BLACK( board->board[x][y] ) && dx != x - 1 )
            return MOVE_INVALID;
         if( dy != y && dy != y - 1 && dy != y + 1 )
            return MOVE_INVALID;
         if( dy == y )
         {
            if( board->board[dx][dy] == NO_PIECE )
               return MOVE_OK;
            else if( SAME_COLOR( board, x, y, dx, dy ) )
               return MOVE_SAMECOLOR;
            else
               return MOVE_BLOCKED;
         }
         else
         {
            if( board->board[dx][dy] == NO_PIECE )
               return MOVE_INVALID;
            else if( SAME_COLOR( board, x, y, dx, dy ) )
               return MOVE_SAMECOLOR;
            else if( board->board[dx][dy] != BLACK_KING && board->board[dx][dy] != WHITE_KING )
               return MOVE_TAKEN;
            else
               return MOVE_INVALID;
         }
         break;
      case WHITE_ROOK:
      case BLACK_ROOK:
      {
         int cnt;

         if( dx != x && dy != y )
            return MOVE_INVALID;

         if( dx == x )
         {
            for( cnt = y; cnt != dy; )
            {
               if( cnt != y && board->board[x][cnt] != NO_PIECE )
                  return MOVE_BLOCKED;
               if( dy > y )
                  ++cnt;
               else
                  --cnt;
            }
         }
         else if( dy == y )
         {
            for( cnt = x; cnt != dx; )
            {
               if( cnt != x && board->board[cnt][y] != NO_PIECE )
                  return MOVE_BLOCKED;
               if( dx > x )
                  ++cnt;
               else
                  --cnt;
            }
         }

         if( board->board[dx][dy] == NO_PIECE )
            return MOVE_OK;

         if( !SAME_COLOR( board, x, y, dx, dy ) )
            return MOVE_TAKEN;

         return MOVE_SAMECOLOR;
      }
         break;
      case WHITE_KNIGHT:
      case BLACK_KNIGHT:
         if( ( dx == x - 2 && dy == y - 1 ) ||
             ( dx == x - 2 && dy == y + 1 ) ||
             ( dx == x - 1 && dy == y - 2 ) ||
             ( dx == x - 1 && dy == y + 2 ) ||
             ( dx == x + 1 && dy == y - 2 ) || ( dx == x + 1 && dy == y + 2 ) || ( dx == x + 2 && dy == y - 1 ) || ( dx == x + 2 && dy == y + 1 ) )
         {
            if( board->board[dx][dy] == NO_PIECE )
               return MOVE_OK;
            if( SAME_COLOR( board, x, y, dx, dy ) )
               return MOVE_SAMECOLOR;
            return MOVE_TAKEN;
         }
         return MOVE_INVALID;
         break;
      case WHITE_BISHOP:
      case BLACK_BISHOP:
      {
         int l, m, blocked = false;

         if( dx == x || dy == y )
            return MOVE_INVALID;

         l = x;
         m = y;

         while( 1 )
         {
            if( dx > x )
               ++l;
            else
               --l;
            if( dy > y )
               ++m;
            else
               --m;
            if( l > 7 || m > 7 || l < 0 || m < 0 )
               return MOVE_INVALID;
            if( l == dx && m == dy )
               break;
            if( board->board[l][m] != NO_PIECE )
               blocked = true;
         }
         if( l != dx || m != dy )
            return MOVE_INVALID;

         if( blocked )
            return MOVE_BLOCKED;

         if( board->board[dx][dy] == NO_PIECE )
            return MOVE_OK;

         if( !SAME_COLOR( board, x, y, dx, dy ) )
            return MOVE_TAKEN;

         return MOVE_SAMECOLOR;
      }
         break;
      case WHITE_QUEEN:
      case BLACK_QUEEN:
      {
         int l, m, blocked = false;

         l = x;
         m = y;

         while( 1 )
         {
            if( dx > x )
               ++l;
            else if( dx < x )
               --l;
            if( dy > y )
               ++m;
            else if( dy < y )
               --m;
            if( l > 7 || m > 7 || l < 0 || m < 0 )
               return MOVE_INVALID;
            if( l == dx && m == dy )
               break;
            if( board->board[l][m] != NO_PIECE )
               blocked = true;
         }
         if( l != dx || m != dy )
            return MOVE_INVALID;

         if( blocked )
            return MOVE_BLOCKED;

         if( board->board[dx][dy] == NO_PIECE )
            return MOVE_OK;

         if( !SAME_COLOR( board, x, y, dx, dy ) )
            return MOVE_TAKEN;

         return MOVE_SAMECOLOR;
      }
         break;
      case WHITE_KING:
      case BLACK_KING:
      {
         int sp, sk;
         if( dx > x + 1 || dx < x - 1 || dy > y + 1 || dy < y - 1 )
            return MOVE_INVALID;
         sk = board->board[x][y];
         sp = board->board[dx][dy];
         board->board[x][y] = sp;
         board->board[dx][dy] = sk;
         if( king_in_check( board, sk ) )
         {
            board->board[x][y] = sk;
            board->board[dx][dy] = sp;
            return MOVE_CHECK;
         }
         board->board[x][y] = sk;
         board->board[dx][dy] = sp;
         if( board->board[dx][dy] == NO_PIECE )
            return MOVE_OK;
         if( SAME_COLOR( board, x, y, dx, dy ) )
            return MOVE_SAMECOLOR;
         return MOVE_TAKEN;
      }
         break;
      default:
         bug( "Invaild piece: %d", board->board[x][y] );
         return MOVE_INVALID;
   }

   if( ( IS_WHITE( board->board[x][y] ) && IS_WHITE( board->board[dx][dy] ) ) || ( IS_BLACK( board->board[x][y] ) && IS_BLACK( board->board[dx][dy] ) ) )
      return MOVE_SAMECOLOR;

   return MOVE_OK;
}

void init_chess( void )
{
}

void free_game( game_board_data * board )
{
   if( !board )
      return;

   if( !board->player1.empty(  ) )
   {
      if( char_data * ch = supermob->get_char_world( board->player1 ) ) // Added for bugfix - Findecano 23/11/07
      {
         ch->printf( "Your chess game has been stopped at %d total moves.\r\n", board->turn );
         ch->pcdata->game_board = nullptr;
      }
   }

   if( !board->player2.empty(  ) )
   {
      if( char_data * ch = supermob->get_char_world( board->player2 ) ) // Added for bugfix - Findecano 23/11/07
      {
         ch->printf( "Your chess game has been stopped at %d total moves.\r\n", board->turn );
         ch->pcdata->game_board = nullptr;
      }
   }
   deleteptr( board );
}

void free_all_chess_games( void )
{
   for( auto it = pclist.begin(); it != pclist.end(); )
   {
      char_data *ch = *it;
      ++it;

      free_game( ch->pcdata->game_board );
   }
}

void send_to_opp( std::string_view arg, char_data * ch, char_data * opp )
{
   if( opp )
   {
      if( ch->pcdata->game_board->type == TYPE_LOCAL )
         opp->print_fmt( "{}\r\n", arg );
   }
}

CMDF( do_chess )
{
   std::string arg;

   argument = one_argument( argument, arg );

   if( ch->isnpc(  ) )
   {
      ch->print( "NPC's can't be in chess games.\r\n" );
      return;
   }

   if( !str_cmp( arg, "begin" ) )
   {
      if( ch->pcdata->game_board )
      {
         ch->print( "You are already in a chess match.\r\n" );
         return;
      }

      game_board_data *board = new game_board_data;
      ch->pcdata->game_board = board;
      ch->pcdata->game_board->player1 = ch->name;
      ch->print( "You have started a game of chess.\r\n" );
      return;
   }

   if( !str_cmp( arg, "join" ) )
   {
      game_board_data *board = nullptr;
      char_data *vch;
      std::string arg2;

      if( ch->pcdata->game_board )
      {
         ch->print( "You are already in a game of chess.\r\n" );
         return;
      }

      argument = one_argument( argument, arg2 );
      if( arg2.empty(  ) )
      {
         ch->print( "Join whom in a chess match?\r\n" );
         return;
      }

      if( !( vch = ch->get_char_world( arg2 ) ) )
      {
         ch->print( "Cannot find that player.\r\n" );
         return;
      }

      if( vch->isnpc(  ) )
      {
         ch->print( "That player is an NPC, and cannot play games.\r\n" );
         return;
      }

      if( !( board = vch->pcdata->game_board ) )
      {
         ch->print( "That player is not playing a game.\r\n" );
         return;
      }

      if( !board->player2.empty(  ) )
      {
         ch->print( "That game already has two players.\r\n" );
         return;
      }

      board->player2 = ch->name;
      ch->pcdata->game_board = board;
      ch->print( "You have joined a game of chess.\r\n" );

      vch = ch->get_char_world( board->player1 );
      vch->print_fmt( "{} has joined your game.\r\n", ch->name );
      return;
   }

   if( !ch->pcdata->game_board )
   {
      ch->print( "Usage: chess <begin|cease|status|board|move|join>\r\n" );
      return;
   }

   if( !str_cmp( arg, "cease" ) )
   {
      free_game( ch->pcdata->game_board );
      return;
   }

   if( !str_cmp( arg, "status" ) )
   {
      game_board_data *board = ch->pcdata->game_board;

      if( board->player1.empty(  ) )
         ch->print( "There is no black player.\r\n" );
      else if( !str_cmp( board->player1, ch->name ) )
         ch->print( "You are black.\r\n" );
      else
         ch->print_fmt( "{} is black.\r\n", board->player1 );

      if( king_in_checkmate( board, BLACK_KING ) )
         ch->print( "The black king is in checkmate!\r\n" );
      else if( king_in_check( board, BLACK_KING ) )
         ch->print( "The black king is in check.\r\n" );

      if( board->player2.empty(  ) )
         ch->print( "There is no white player.\r\n" );
      else if( !str_cmp( board->player2, ch->name ) )
         ch->print( "You are white.\r\n" );
      else
         ch->print_fmt( "{} is white.\r\n", board->player2 );

      if( king_in_checkmate( board, WHITE_KING ) )
         ch->print( "The white king is in checkmate!\r\n" );
      else if( king_in_check( board, WHITE_KING ) )
         ch->print( "The white king is in check.\r\n" );

      if( board->player2.empty(  ) || board->player1.empty(  ) )
         return;

      ch->print_fmt( "{} turns.\r\n", board->turn );
      if( board->turn % 2 == 1 && !str_cmp( board->player1, ch->name ) )
      {
         ch->print_fmt( "It is {}'s turn.\r\n", board->player2 );
         return;
      }
      else if( board->turn % 2 == 0 && !str_cmp( board->player2, ch->name ) )
      {
         ch->print_fmt( "It is {}'s turn.\r\n", board->player1 );
         return;
      }
      else
      {
         ch->print( "It is your turn.\r\n" );
         return;
      }
      return;
   }

   if( !str_prefix( arg, "board" ) )
   {
      ch->print( print_big_board( ch->pcdata->game_board ) );
      return;
   }

   if( !str_prefix( arg, "move" ) )
   {
      char_data *opp;
      std::string opp_name;
      char a, b;
      int x, y, dx, dy, ret;

      if( ch->pcdata->game_board->player1.empty(  ) || ch->pcdata->game_board->player2.empty(  ) )
      {
         ch->print( "There is only 1 player.\r\n" );
         return;
      }

      if( ch->pcdata->game_board->turn < 0 )
      {
         ch->print( "The game hasn't started yet.\r\n" );
         return;
      }

      if( king_in_checkmate( ch->pcdata->game_board, BLACK_KING ) )
      {
         ch->print( "The black king has been checkmated, the game is over.\r\n" );
         return;
      }

      if( king_in_checkmate( ch->pcdata->game_board, WHITE_KING ) )
      {
         ch->print( "The white king has been checkmated, the game is over.\r\n" );
         return;
      }

      if( argument.empty(  ) )
      {
         ch->print( "Usage: chess move [piece to move] [where to move]\r\n" );
         return;
      }

      if( ch->pcdata->game_board->turn % 2 == 1 && !str_cmp( ch->pcdata->game_board->player1, ch->name ) )
      {
         ch->print( "It is not your turn.\r\n" );
         return;
      }

      if( ch->pcdata->game_board->turn % 2 == 0 && !str_cmp( ch->pcdata->game_board->player2, ch->name ) )
      {
         ch->print( "It is not your turn.\r\n" );
         return;
      }

      if( sscanf( argument.c_str(  ), "%c%d %c%d", &a, &y, &b, &dy ) != 4 )
      {
         ch->print( "Usage: chess move [dest] [source]\r\n" );
         return;
      }

      if( a < 'a' || a > 'h' || b < 'a' || b > 'h' || y < 1 || y > 8 || dy < 1 || dy > 8 )
      {
         ch->print( "Invalid move, use a-h, 1-8.\r\n" );
         return;
      }

      x = a - 'a';
      dx = b - 'a';
      --y;
      --dy;

      ret = is_valid_move( ch, ch->pcdata->game_board, x, y, dx, dy );
      if( ret == MOVE_OK || ret == MOVE_TAKEN )
      {
         game_board_data *board;
         int piece, destpiece;

         board = ch->pcdata->game_board;
         piece = board->board[x][y];
         destpiece = board->board[dx][dy];
         board->board[dx][dy] = piece;
         board->board[x][y] = NO_PIECE;

         if( king_in_check( board, IS_WHITE( board->board[dx][dy] ) ? WHITE_KING : BLACK_KING )
             && ( board->board[dx][dy] != WHITE_KING && board->board[dx][dy] != BLACK_KING ) )
         {
            board->board[dx][dy] = destpiece;
            board->board[x][y] = piece;
            ret = MOVE_INCHECK;
         }
         else
         {
            ++board->turn;
         }
      }

      if( !str_cmp( ch->name, ch->pcdata->game_board->player1 ) )
      {
         opp = ch->get_char_world( ch->pcdata->game_board->player2 );
         if( !opp )
            opp_name = ch->pcdata->game_board->player2;
      }
      else
      {
         opp = ch->get_char_world( ch->pcdata->game_board->player1 );
         if( !opp )
            opp_name = ch->pcdata->game_board->player1;
      }

      std::string buf;
      switch ( ret )
      {
         case MOVE_OK:
            ch->print( "Ok.\r\n" );
            buf = std::format( "{} has moved.\r\n", ch->name );
            send_to_opp( buf, ch, opp );
            break;

         case MOVE_INVALID:
            ch->print( "Invalid move.\r\n" );
            break;

         case MOVE_BLOCKED:
            ch->print( "You are blocked in that direction.\r\n" );
            break;

         case MOVE_TAKEN:
            ch->print( "You take the enemy's piece.\r\n" );
            buf = std::format( "{} has taken one of your pieces!", ch->name );
            send_to_opp( buf, ch, opp );
            break;

         case MOVE_CHECKMATE:
            ch->print( "That move would result in a checkmate.\r\n" );
            buf = std::format( "{} has attempted a move that would result in checkmate.", ch->name );
            send_to_opp( buf, ch, opp );
            break;

         case MOVE_OFFBOARD:
            ch->print( "That move would be off the board.\r\n" );
            break;

         case MOVE_SAMECOLOR:
            ch->print( "Your own piece blocks the way.\r\n" );
            break;

         case MOVE_CHECK:
            ch->print( "That move would result in a check.\r\n" );
            buf = std::format( "{} has made a move that would result in a check.", ch->name );
            send_to_opp( buf, ch, opp );
            break;

         case MOVE_WRONGCOLOR:
            ch->print( "That is not your piece.\r\n" );
            break;

         case MOVE_INCHECK:
            ch->print( "You are in check, you must save your king.\r\n" );
            break;

         default:
            bug( "%s: Unknown return value", __func__ );
            break;
      }
      return;
   }
   ch->print( "Usage: chess <begin|cease|status|board|move|join>\r\n" );
}
