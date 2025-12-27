#pragma once

#include <stdbool.h>
#include <ncurses.h>

#include "socket.h"
#include "message.h"

#define BOARD_SIZE 50
#define LOSS_BONUS 15 // Bonus cells you get for losing

enum Color {
  COLORLESS,
  RED,
  BLUE
};

typedef struct cell {
  bool alive;  // Whether there's a live cell here
  bool future; // Whether the cell will be alive next turn
  int color;   // Which player owns the cell
  bool locked; // Whether a cell can be placed here by a player
} cell_t;

typedef struct score {
  int red;
  int blue;
  int diff;
} score_t;

typedef cell_t ** board_t;

bool outofbounds(int row, int column);

board_t create_board();

cell_t get_cell(board_t board, int row, int column);

void spawn(board_t board, int row, int column, int color);

void kill(board_t board, int row, int column);

void update_cell(board_t board, int row, int column);

void update_board(board_t board);

void free_board(board_t board);

score_t print_board(board_t board, WINDOW * w_board, WINDOW * w_status);

// Return whether a the player wishes to continue placing
int place_cell(int color, board_t board, WINDOW * w_board, WINDOW * w_status);

void set_board(int count, int color, board_t board, WINDOW * w_board, WINDOW * w_status);

void send_board(int fd, board_t board);

board_t recv_board(int fd);
