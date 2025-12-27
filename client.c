#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include "conway.h"
#include "socket.h"

/*
 * Client Program
 */
 
int main(int argc, char ** argv) {
  // Check for proper arguments
  if (argc != 3) {
    fprintf(stderr, "Usage: %s <server name> <port>\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  // Read command line arguments
  char* server_name = argv[1];
  unsigned short port = atoi(argv[2]);

  // Connect to the server
  int socket = socket_connect(server_name, port);
  if (socket == -1) {
    perror("Failed to connect");
    exit(EXIT_FAILURE);
  }

  // Start with 0 bonus cells
  int bonus = 0;

  // Set up ncurses
  initscr();
  noecho();
  cbreak();
  keypad(stdscr, true);
  curs_set(0);
  start_color();
  refresh();

  // Set up windows for board and status line
  WINDOW * w_board = newwin(BOARD_SIZE,BOARD_SIZE,2,1);
  WINDOW * w_status = newwin(1,BOARD_SIZE,0,0);

  for (int x=0;x<=BOARD_SIZE+1;x++) {
    for (int y=1;y<=BOARD_SIZE+2;y++) {
      if ((x==0 || x==BOARD_SIZE+1) ||
      	  (y==1 || y==BOARD_SIZE+2)) {
	      mvaddch(y,x,'*');
      }
    }
  }

  // Cursor starts at top-left corner of screen
  move(2,1);

  // Create empty board to draw to the board window
  board_t board = create_board();

  // String to store the instructions given by the server
  char * matchinst;

  // Get instructions from server to either start a match, or end winning or losing
  while ((matchinst = receive_message(socket))) {
    if (matchinst == NULL) {
      // Server lost
      endwin();
      printf("Connection lost.\n");
      return -1;
    }

    if (strcmp(matchinst,"cwin") == 0) {
      // Client won
      endwin();
      printf("You won the set!\n");
      return 0;
    } else if (strcmp(matchinst,"swin") == 0) {
      // Server won
      endwin();
      printf("You lost the set. Better luck next time!\n");
      return 0;
    } else if (strcmp(matchinst,"setboard") == 0) {
      // Set the board for a new match
      set_board(10+bonus,BLUE,board,w_board,w_status);
      wclear(w_status);
      wprintw(w_status,"Waiting on opponent...");
      wrefresh(w_status);

      // Send the board to the server
      send_board(socket,board);

      // Get the finalized board from the server
      free(board);
      board = recv_board(socket);

      // Display it
      print_board(board,w_board,w_status);
      wclear(w_status);
      wprintw(w_status,"Hit any key to begin.");
      wrefresh(w_status);
      getch();
      wclear(w_status);
      wprintw(w_status,"Waiting on opponent...");
      wrefresh(w_status);

      // Ready to start the match!
      send_message(socket,"ready");

      // Store an instruction on whether to update or end the match
      char * updateinst;

      while ((updateinst = receive_message(socket))) {
        // Server is gone
        if (updateinst == NULL) {
          endwin();
          printf("Connection lost.\n");
          return -1;
        }

        if (strcmp(updateinst,"update") == 0) {
          // We need to update the board
          update_board(board);
          print_board(board,w_board,w_status);

          // Potential to send back a hash to ensure we're synced
          // Not implemented currently
          // The "hash" is just the word hash
          send_message(socket,"hash");
        } else if (strcmp(updateinst,"desynced") == 0) {
          // Server told us we're desynced
          // Can't currently happen because the hash is unimplemented
          endwin();
          printf("Desynced, giving up.\n");
          return -1;
        } else if (strcmp(updateinst,"cwin") == 0) {
          // Client won the match, yay
          wclear(w_status);
          wprintw(w_status,"You won! Hit any key.");
          wrefresh(w_status);
          getch();
          wclear(w_status);
          wprintw(w_status,"Waiting on opponent...");
          wrefresh(w_status);
          // Done celebrating, ready for another 
          send_message(socket,"ready");
          break; // Get another instruction on whether to start a new match
        } else if (strcmp(updateinst,"swin") == 0) {
          // We lost, boo
          wclear(w_status);
          wprintw(w_status,"You lost! Hit any key.");
          wrefresh(w_status);
          bonus += LOSS_BONUS;
          getch();
          wclear(w_status);
          wprintw(w_status,"Waiting on opponent...");
          wrefresh(w_status);
          // Done being sad, ready for another
          send_message(socket,"ready");
          break; // Get another instruction on whether to start a new match
        } else if (strcmp(updateinst,"tie") == 0) {
          wclear(w_status);
          wprintw(w_status,"It's a tie! Hit any key.");
          wrefresh(w_status);
          getch();
          wclear(w_status);
          wprintw(w_status,"Waiting on opponent...");
          wrefresh(w_status);
          send_message(socket,"ready");
          break; // Get another instruction on whether to start a new match
        }
        // Clean-up
        free(updateinst);
      }
    }
    // Clean-up
    free(matchinst);
  }
}
