#include <stdlib.h>
#include <ncurses.h>
#include <string.h>

#include "conway.h"

// Check if coordinates are out of bounds
bool outofbounds(int row, int column) {
  return row < 0 || row >= BOARD_SIZE || column < 0 || column >= BOARD_SIZE;
}

// Create a blank board
board_t create_board() {
  // A board is BOARD_SIZE cell pointers (rows)
  board_t board = (board_t) malloc(sizeof(cell_t *) * BOARD_SIZE);

  // Each row is BOARD_SIZE cells
  for (int row = 0; row < BOARD_SIZE; row++) {
    board[row] = (cell_t *) malloc(sizeof(cell_t) * BOARD_SIZE);
  }

  return board;
}

// Return a copy of a cell at a location
cell_t get_cell(board_t board, int row, int column) {
  if (outofbounds(row,column)) {
    cell_t out_of_bounds;
    out_of_bounds.alive = false;
    out_of_bounds.color = COLORLESS;
    return out_of_bounds;
  }

  cell_t boardcell = board[column][row];
  cell_t answer;
  answer.alive = boardcell.alive;
  answer.color = boardcell.color;
  return answer;
}

// Mark a cell to be made alive
void spawn(board_t board, int row, int column, int color) {
  if (outofbounds(row,column)) return;
  board[column][row].future = true;
  board[column][row].locked = true;
  board[column][row].color = color;
}

// Mark a cell to be killed
void kill(board_t board, int row, int column) {
  if (outofbounds(row,column)) return;
  board[column][row].future = false;
  board[column][row].locked = false;
}

// Mark a cell appropriately depending on the rules of conway
void update_cell(board_t board, int row, int column) {
  int neighbors = 0;
  int reds = 0;
  int blues = 0;
  for (int r = row-1; r <= row+1; r++) {
    for (int c = column-1; c <= column+1; c++) {
      if (r != row || c != column) {
        cell_t neighbor = get_cell(board,r,c);
        if (neighbor.alive) {
          neighbors++;
          if (neighbor.color == BLUE) blues++;
          if (neighbor.color == RED) reds++;
        }
      }
    }
  }

  cell_t target = get_cell(board, row, column);

  if (target.alive && neighbors != 2 && neighbors != 3) {
    kill(board, row, column);
  }

  if (!target.alive && neighbors == 3) {
    if (blues > reds) {
      spawn(board, row, column, BLUE);
    } else if (reds > blues) {
      spawn(board, row, column, RED);
    }
  }
}

// Update the board once
void update_board(board_t board) {
  for (int row = 0; row < BOARD_SIZE; row++) {
    for (int column = 0; column < BOARD_SIZE; column++) {
      update_cell(board, row, column);
    }
  }

  for (int row = 0; row < BOARD_SIZE; row++) {
    for (int column = 0; column < BOARD_SIZE; column++) {
      board[row][column].alive = board[row][column].future;
      board[row][column].future = board[row][column].alive;
    }
  }
}

// Destroy the board
void free_board(board_t board) {
  for (int row = 0; row < BOARD_SIZE; row++) {
    free(board[row]);
  }
  free(board);
}

// Render the board to an ncurses window
score_t display_board(board_t board, WINDOW * w_board) {
  score_t score;
  score.red = 0;
  score.blue = 0;
  
  wclear(w_board);
  
  //init_pair(RED,COLOR_RED,COLOR_BLACK);
  //init_pair(BLUE,COLOR_CYAN,COLOR_BLACK);
  init_pair(RED,COLOR_BLACK,COLOR_RED);
  init_pair(BLUE,COLOR_BLACK,COLOR_CYAN);
  for (int row = 0; row < BOARD_SIZE; row++) {
    for (int col = 0; col < BOARD_SIZE; col++) {
      cell_t cell = get_cell(board,row,col);
      if (cell.alive) {
        wattrset(w_board,COLOR_PAIR(cell.color));
        if (cell.color == RED) {
          score.red++;
          mvwaddch(w_board,row,col,'#');
        } else {
          score.blue++;
          mvwaddch(w_board,row,col,'@');
        }
      }
    }
  }
  wattrset(w_board,A_NORMAL);

  score.diff = score.red - score.blue;
  return score;
}

// Print out both the board and the status line
score_t print_board(board_t board, WINDOW * w_board, WINDOW * w_status) {

  score_t score = display_board(board, w_board);

  wclear(w_status);

  if (score.diff > 0) {
    wprintw(w_status,"Red is up by %d.",score.diff);
  } else if (score.diff < 0) {
    wprintw(w_status,"Blue is up by %d.",score.diff*-1);
  } else {
    wprintw(w_status,"Red and Blue are tied!");
  }

  wrefresh(w_board);
  wrefresh(w_status);

  return score;
}

// Allow the user to select a spot to place a cell
int place_cell(int color, board_t board, WINDOW * w_board, WINDOW * w_status) {
  wrefresh(w_board);
  int y;
  int x;
  getyx(w_board,y,x);
  int c;
  bool placing = true;
  while ((c=getch()) && placing) {
    switch (c) {
    case KEY_LEFT:
    case 'h':
      x--;
      break;
    case KEY_RIGHT:
    case 'l':
      x++;
      break;
    case KEY_UP:
    case 'k':
      y--;
      break;
    case KEY_DOWN:
    case 'j':
      y++;
      break;
    case 'b':
      x--;
      y++;
      break;
    case 'n':
      x++;
      y++;
      break;
    case 'y':
      x--;
      y--;
      break;
    case 'u':
      x++;
      y--;
      break;
    case 'z':
    case '\n':
    case '.':
    case KEY_ENTER:
      if (board[x][y].alive && board[x][y].color == color && !(board[x][y].locked)) {
        board[x][y].color = COLORLESS;
        board[x][y].alive = false;
        board[x][y].future = false;
	board[x][y].locked = false;

        display_board(board,w_board);
        wmove(w_board,y,x);
        wrefresh(w_board);

        return -1;
      } else if (!board[x][y].alive) {
        board[x][y].color = color;
        board[x][y].alive = true;
        board[x][y].future = true;
	board[x][y].locked = false;

        display_board(board,w_board);
        wmove(w_board,y,x);
        wrefresh(w_board);

        return 1;
      } else {
        break;
      }
    case 'q':
      return 0;
    }
    
    if (x<0) x=0;
    if (x>=BOARD_SIZE) x=BOARD_SIZE-1;
    if (y<0) y=0;
    if (y>=BOARD_SIZE) y=BOARD_SIZE-1;

    display_board(board,w_board);
    wmove(w_board,y,x);
    wrefresh(w_board);
  }
  return 1;
}

// Allow the user to set the board
void set_board(int count, int color, board_t board, WINDOW * w_board, WINDOW * w_status) {
  curs_set(1);
  for (int i = 0; i < count; i++) {
    wclear(w_status);
    wprintw(w_status,"%d cells remaining.",count-i);
    wrefresh(w_status);
    int cont = place_cell(color,board,w_board,w_status);
    if (cont == -1) {
      i-=2;
    }
    if (!cont) break;
  }
  curs_set(0);
}

// Send a board over a socket
void send_board(int fd, board_t board) {
  char * message = malloc(sizeof(char)*4);
  message[3]=0;
  for (int x = 0; x < BOARD_SIZE; x++) {
    for (int y = 0; y < BOARD_SIZE; y++) {
      if (board[x][y].alive) {
	message[0] = (char)(board[x][y].color);
	message[1] = (char)x;
	message[2] = (char)y;
	int success = send_message(fd,message);
	if (success == -1) exit(-1);
      }
    }
  }
  send_message(fd,"000");
  free(message);
}

// Receive a board over a socket
board_t recv_board(int fd) {
  board_t newboard = create_board();

  char * message;

  while (true) {
    message = receive_message(fd);

    if (message == NULL) exit(7);

    if (strcmp(message, "000") == 0) {
      return newboard;
    }

    newboard[(int)(message[1])][(int)(message[2])].color = (int)(message[0]);
    newboard[(int)(message[1])][(int)(message[2])].alive = true;
    newboard[(int)(message[1])][(int)(message[2])].future = true;
  }
}
