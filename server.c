#include <ncurses.h>
#include <stdlib.h>
#include "conway.h"
#include "socket.h"

/*
 * Server Program
 */

int main() {
  // Listening for a client
  unsigned short port = 0;
  int server_socket = server_socket_open(&port);

  if (server_socket == -1) exit(-1);

  printf("Server listening on port %u\n",port);

  if (listen(server_socket, 1)) {
    perror("listen failed");
    exit(EXIT_FAILURE);
  }

  // Accepting the client
  int client_socket = server_socket_accept(server_socket);
  if (client_socket == -1) {
    perror("accept failed");
  }

  printf("Found opponent!\n");

  // Initialize ncurses
  initscr();
  noecho();
  cbreak();
  keypad(stdscr, true);
  curs_set(0);
  start_color();

  refresh();

  // Windows to draw the board and status bar
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

  // Cursor starts at top left of board
  move(2,1);

  // Create an empty board
  board_t board = create_board();

  // Five matches, score is 0/0, no bonus cells to start
  int matches = 5;
  int serverwins = 0;
  int clientwins = 0;
  int bonus = 0;

  // Store responses from client
  char * client_message;

  while (matches > 0) {
    // Tell the client to set the board, then set our own board
    send_message(client_socket,"setboard");
    set_board(10+bonus,RED,board,w_board,w_status);
    wclear(w_status);
    wprintw(w_status, "Waiting on opponent...");
    wrefresh(w_status);

    // Get the board back from the client
    board_t opponent_board = recv_board(client_socket);

    // Normalize the board
    for (int x = 0; x < BOARD_SIZE; x++) {
      for (int y = 0; y < BOARD_SIZE; y++) {
        if (opponent_board[x][y].alive) {
          if (board[x][y].alive && board[x][y].color != opponent_board[x][y].color) {
            board[x][y].alive = false;
            board[x][y].future = false;
            board[x][y].color = COLORLESS;
          } else {
            board[x][y].color = opponent_board[x][y].color;
            board[x][y].alive = true;
            board[x][y].future = true;
          }
        }
      }
    }

    // Send the client the new board
    free(opponent_board);
    send_board(client_socket,board);

    // Display the board
    print_board(board,w_board,w_status);
    wclear(w_status);
    wprintw(w_status,"Hit any key to begin.");
    wrefresh(w_status);
    getch();

    // Done displaying, waiting for opponent now
    wclear(w_status);
    wprintw(w_status,"Waiting on opponent...");
    wrefresh(w_status);

    // Client is ready to start
    client_message = receive_message(client_socket);
    if (client_message == NULL) {
      // Scratch that, client is disconnected actually
      endwin();
      printf("Connection lost.\n");
      return -1;
    }
    free(client_message);

    score_t score;
    // Score of the match

    for (int steps = 0; steps < 75; steps++) {
      // 75 times, update the board and tell the client to do so as well
      update_board(board);
      score = print_board(board,w_board,w_status);
      
      send_message(client_socket,"update");

      // Get the hash back from the client after each update
      // Currently unimplemented, the hash is just "hash"
      client_message = receive_message(client_socket);
      if (client_message == NULL) {
        endwin();
        printf("Connection lost.\n");
        return -1;
      }
      if (strcmp(client_message,"hash") != 0) {
        // If they didn't say "hash", tell them we're breaking up
        send_message(client_socket,"desynced");
        // I'm sorry, I just think I should see other clients
        endwin();
        // And you should meet some different servers
        printf("Desynced, giving up.\n");
        // I just don't think we can work out
        return -1;
      }
      free(client_message);
    }

    // The match is done, let's see who won
    if (score.diff > 0) {
      // We won, update the score
      serverwins++;
      matches--;

      // Nyeh nyeh, we won!
      send_message(client_socket,"swin");
      wclear(w_status);
      wprintw(w_status,"You won! Hit any key.");
      wrefresh(w_status);
      getch();

      // Done celebrating, wait for opponent to stop sulking
      wclear(w_status);
      wprintw(w_status,"Waiting on opponent...");
      wrefresh(w_status);
      client_message = receive_message(client_socket);
      if (client_message == NULL) {
        endwin();
        printf("Connection lost.\n");
        return -1;
      }
      free(client_message);
    } else if (score.diff < 0) {
      // Darn, we lost. Guess I'll update the score
      clientwins++;
      matches--;
      bonus += LOSS_BONUS;

      // Admit defeat
      send_message(client_socket,"cwin");
      wclear(w_status);
      wprintw(w_status,"You lost! Hit any key.");
      wrefresh(w_status);
      getch();

      // Wait for opponent to stop celebrating and get on with it
      wclear(w_status);
      wprintw(w_status,"Waiting on opponent...");
      wrefresh(w_status);
      client_message = receive_message(client_socket);
      if (client_message == NULL) {
        endwin();
        printf("Connection lost.\n");
        return -1;
      }
      free(client_message);
    } else if (score.diff == 0) {
      // We tied, tell the opponent
      send_message(client_socket,"tie");
      wclear(w_status);
      wprintw(w_status,"It's a tie! Hit any key.");
      wrefresh(w_status);
      getch();

      // Wait for opponent to get ready
      wclear(w_status);
      wprintw(w_status,"Waiting on opponent...");
      wrefresh(w_status);
      client_message = receive_message(client_socket);
      if (client_message == NULL) {
        endwin();
        printf("Connection lost.\n");
        return -1;
      }
      free(client_message);
    }
  }

  // All the matches are done! Now figure out who won
  if (serverwins < clientwins) {
    // It is not us who won
    send_message(client_socket,"cwin");
    endwin();
    printf("You lost the set. Better luck next time!\n");
    return 0;
  } else {
    // Oh! We won. Yippee
    send_message(client_socket,"swin");
    endwin();
    printf("You won the set!\n");
    return 0;
  }
}
