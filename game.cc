// The MIT License (MIT)
// Copyright (c) 2016 Sachin Shenoy 

// Player Isolation is a game played between two players, on a 5 by 5
// grid. The first player, player one, has white tokens, while player
// two has black token. The game starts by player one placing a white
// token on one of the 25 squares. After that, player two places black
// token on one of the empty squares. Further on each player, places 
// another of their token, making one of the possible queen type move
// from their current token position. This movement, can't jump any 
// already existing tokens. The first player who cannot play a legal
// move (during their turn) loses.

#include <stdio.h>
#include <iostream>
#include <queue>

using namespace std;

// Value to mark that the cell is out of valid board range.
const char BORDER = 5;

// Value to mark that the cell is empty. 
const char EMPTY = 0;

// Value to mark that the cell is occupied by player one.
const char P1 = 1;

// Value to mark that the cell is occupied by player two.
const char P2 = 2;

const int LOSS_VALUE = -1000;
const int WIN_VALUE = +1000;
const int INF = 1000000;
const int MOVES[] = {1, -1, 7, -7, 6, -6, 8, -8};
const int SCORE_PER_CELL = 16;

// Helper macros.
#define OPPONENT(p) ((p == P1) ? P2 : P1)
#define PLAYER(p) ((p == P1) ? '1' : '2')
#define POS_TO_X(pos) ((pos - 8) / 7)
#define POS_TO_Y(pos) ((pos - 8) % 7)
#define XY_TO_POS(x, y) (x*7+y+8)
#define DEBUG(x) ;

class Board {
 public:
  char board[49];
  int p1;
  int p2;

  Board();
  bool hasLost(int i);
  bool isLegal(int x, int y);
  void play(int x, int y, char player);
  void printBoard();
  void printPossibleMoves(char player);
};

Board::Board() {
  for (int i=0; i<7; ++i) {
    board[0*7+i] = board[i*7+0] = board[6*7+i] = board[i*7+6] = BORDER;
  }
  for (int i=1; i<6; ++i) {
    for (int j=1; j<6; ++j) {
      board[i*7+j] = EMPTY;
    }
  }
  p1 = p2 = 0;
}

bool Board::isLegal(int x, int y) {
  return board[x*7+y+8] == EMPTY;
}

void Board::play(int x, int y, char player) {
  int pos = x*7+y+8;
  board[pos] = player;
  if (player == P1)
    p1 = pos; 
  else
    p2 = pos; 
}

void Board::printBoard() {
  for (int i=1; i<6; ++i) {
    cout << "| ";
    for (int j=1; j<6; ++j) {
      char cell = board[i*7+j];
      if (cell == 0) {
        cout << "  | "; 
      } else {
        if (p1 == i*7+j || p2 == i*7+j) {
          cout << PLAYER(cell) << " | ";
        } else {
          cout << "X | ";
        }
      }
    }
    cout << endl;
  }
}

bool Board::hasLost(int i) {
  if (i == 0) return false;
  if (board[i+1] == 0 || board[i-1] == 0 || board[i+7] == 0 || board[i-7] == 0 ||
      board[i+6] == 0 || board[i-6] == 0 || board[i+8] == 0 || board[i-8] == 0)
    return false;
  return true; 
}

class Mirror {
 public:
  int getMove(Board* board, char player, int max_depth);

 private:
  Board* board;
};

int Mirror::getMove(Board* board, char player, int max_depth) {
  this->board = board;
  int pp = (player == P1) ? board->p2 : board->p1;
  int px = POS_TO_X(pp);
  int py = POS_TO_Y(pp);
  int x = 4 - px;
  int y = 4 - py;
  return XY_TO_POS(x, y); 
}

class Scorer {
 public:
  virtual int getScore(Board* board, char player) = 0;
};

class DijkstraScorer : public Scorer {
 public:
  int getScore(Board* board, char player);
 private:
  int dijkstra(Board* board, char player); 
};

int DijkstraScorer::getScore(Board* board, char player) {
  return dijkstra(board, player) - dijkstra(board, OPPONENT(player));
}

int DijkstraScorer::dijkstra(Board* board, char player) {
  queue<int> q;
  int steps[49];
  for (int i = 0; i < 49; ++i) steps[i] = -1;

  int total_cells = 0;
  int total_steps = 0;
  int pos = (player == P1) ? board->p1 : board->p2;
  q.push(pos);
  steps[pos] = 0;

  while (!q.empty()) { 
    int pos = q.front(); q.pop();
    total_steps += steps[pos]; 
    total_cells += 1;
    int step = steps[pos] + 1;
    for (int i = 0; i < 8; ++i) {
      int move = MOVES[i];
      int p = pos;
      while (true) {
        p += move;
        char& cell = board->board[p];
        if (cell != EMPTY) {
          break;
        }
        if (steps[p] != -1) {
          break;
        }
        steps[p] = step;
        q.push(p);
      }
    }
  }

  return total_cells * SCORE_PER_CELL - total_steps; 
}

class Negamax {
 public:
  Negamax();
  Negamax(Scorer* scorer);
  int getMove(Board* board, char player, int max_depth);
  int depth_count;

 private:
  Board* board;
  Scorer* scorer;
  int max_depth;

  int negamax(int ap_pos, int pp_pos, int depth, int alpha, int beta, int* best_move); 
  void printDebug(int depth, const string& action, int score);
  void printMove(int depth, int x, int y);
  bool hasLost(int pos) {
    return board->hasLost(pos);
  }
};

Negamax::Negamax() {
  this->scorer = new DijkstraScorer();
}

Negamax::Negamax(Scorer* scorer) {
  if (scorer == NULL) {
    scorer = new DijkstraScorer();
  }
  this->scorer = scorer;
}

int Negamax::getMove(Board* board, char player, int max_depth) {
  this->board = board; 
  this->max_depth = max_depth;
  this->depth_count = 0;
  int ap_pos = (player == P1) ? board->p1 : board->p2;
  int pp_pos = (player == P1) ? board->p2 : board->p1;
  int move = 0;
  negamax(ap_pos, pp_pos, 1, -INF, INF, &move);
  return move;
}

void Negamax::printDebug(int depth, const string& action, int score) {
  for (int i = 0; i < depth; ++i) cout << "  ";
  cout << depth << " " << action << " " << score << endl;
}

void Negamax::printMove(int depth, int x, int y) {
  for (int i = 0; i < depth; ++i) cout << "  ";
  cout << depth << " MOVE " << x << "," << y << endl;
}

int Negamax::negamax(int ap_pos, int pp_pos, int depth, int alpha, int beta, int* best_move) {
  if (hasLost(ap_pos)) {
    DEBUG(printDebug(depth, "LOST", LOSS_VALUE + depth));
    return LOSS_VALUE + depth;
  }

  char player = board->board[ap_pos];
  if (depth == max_depth) {
    depth_count++;
    int score = scorer->getScore(board, player);
    DEBUG(printDebug(depth, "SCORE", score));
    return score;
  }

  int best_score = -INF;
  char opponent = OPPONENT(player);

  for (int i = 0; i < 8; ++i) {
    int pos = ap_pos;
    int move = MOVES[i];
    while (true) {
      pos += move;
      char& cell = board->board[pos];
      if (cell != EMPTY) {
        break;
      }
      DEBUG(printMove(depth, POS_TO_X(pos), POS_TO_Y(pos)));
      cell = player;
      int score = -1 * negamax(pp_pos, pos, depth+1, -beta, -alpha, NULL);
      cell = 0;
      if (score > best_score) {
        best_score = score;
        if (best_move != NULL) {
            *best_move = pos;
        }
      }
      alpha = (alpha >= score) ? alpha : score;
      if (alpha >= beta) {
        DEBUG(printDebug(depth, "BEST", best_score));
        return best_score;
      }
    }
  }
  DEBUG(printDebug(depth, "BEST", best_score));
  return best_score;
}

void Board::printPossibleMoves(char player) {
  int ap_pos = (player == P1) ? p1 : p2;

  char brd[49];
  for (int i = 0; i < 49; ++i) {
    brd[i] = board[i];
  }

  for (int i = 0; i < 8; ++i) {
    int pos = ap_pos;
    int move = MOVES[i];
    while (true) {
      pos += move;
      char& cell = brd[pos];
      if (cell != EMPTY) {
        break;
      }
      cell = 3;
    }
  }
  for (int i=1; i<6; ++i) {
    cout << "| ";
    for (int j=1; j<6; ++j) {
      char cell = brd[i*7+j];
      if (cell == 0) {
        cout << "  | "; 
      } else {
        if (p1 == i*7+j || p2 == i*7+j) {
          cout << PLAYER(cell) << " | ";
        } else if (cell == 3) {
          cout << "* | ";
        } else {
          cout << "X | ";
        }
      }
    }
    cout << endl;
  }
}

void play_match(char player, Board& board);
 
int main(int argc, char* argv[]) {
  for (int i = 0; i < 5; ++i) {
    for (int j = 0; j < 5; ++j) {
      if (i == 0 && j == 0) continue;
      Board board;
      board.play(0, 0, P1);
      board.play(i, j, P2);
      play_match(P1, board);
    }
  }
  return 0;
}

void play_match(char player, Board& board) {
  int best_move;
  Negamax negamax;
  Negamax mirror;
  int count = 0;

  while (true) { 
    int ap_pos = (player == P1) ? board.p1 : board.p2;
    int pp_pos = (player == P1) ? board.p2 : board.p1;
    if (board.hasLost(ap_pos)) {
      cout << "Player:" << PLAYER(player) << " Lost." << endl;
      break;
    }
    
    int best_move;
    if (count % 2 == 0) {
      best_move = mirror.getMove(&board, player, 25);
    } else {
      best_move = negamax.getMove(&board, player, 25);
    }
    count++;
    int x, y; 
    x = POS_TO_X(best_move);
    y = POS_TO_Y(best_move);
    cout << "Moved " << PLAYER(player) << " M: " << x << ", " << y << endl;
    board.play(x, y, player);
    board.printBoard();
    cout << endl;
    player = OPPONENT(player);
  }
}
