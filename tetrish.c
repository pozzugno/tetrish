#include <stdio.h>
#include <time.h>
#include <stdbool.h>
#include <unistd.h>
#include <termios.h>
#include <sys/select.h>
#include <stdlib.h>
#include "piece.h"

#define BOARD_COLS          10
#define BOARD_ROWS          30
#define CHAR_EMPTY          '.'

/* All current status (context) of our tetris board, composed by... */
static struct TetrisBoard {
    char board[BOARD_ROWS][BOARD_COLS];     // ...the real board, a grid of points
    /* The grid is based on simple chars. There's a special char (CHAR_EMPTY) that
     * is used for empy positions. All other chars denotes the presence of a piece
     * (or what remains of a shape when complete lines are erased) */
    int fp_x0;                              // ...the x-offset of falling piece
    int fp_y0;                              // ...the y-offset of falling piece
    int fp_rot;                             // ...the rotation of falling piece
    const struct Piece *fp;                 // ...the shape of falling piece
} board;

static struct termios oldt;

/* Reset original terminal mode at exit */
void 
reset_terminal_mode(void) 
{
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
}

/* Set terminal in non canonical mode */
void 
set_conio_terminal_mode(void) 
{
    struct termios newt;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO); // Disable echo and canonical input
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
}

/* Function that clears the screen and move the cursor at the beginning position */
static inline void
clrscr(void)
{
    printf("\033[2J\033[H");    
    fflush(stdout);
}

/* Initialize the board, filling all the points with CHAR_EMPTY */
static void
board_init(struct TetrisBoard *b)
{
    for (int y = 0; y < BOARD_ROWS; y++) {
        for (int x = 0; x < BOARD_COLS; x++) {
            b->board[y][x] = CHAR_EMPTY;
        }
    }
    b->fp = NULL;       // The first falling piece must be created later
}

/* Print the board on the screen */
static void
board_print(struct TetrisBoard *b) 
{
    clrscr();
    for (int y = 0; y < BOARD_ROWS; y++) {
        for (int x = 0; x < BOARD_COLS; x++) {
            char c = b->board[y][x];
            /* Is there a point of falling piece in this position? */
            if (b->fp != NULL) {
                if ((x >= b->fp_x0) && (x < b->fp_x0 + piece_xl(b->fp, b->fp_rot))) {
                    if ((y >= b->fp_y0) && (y < b->fp_y0 + piece_yl(b->fp, b->fp_rot))) {
                        /* Yes, there's the falling piece here, but is it a real occupied position? */
                        int xp = x - b->fp_x0;
                        int yp = y - b->fp_y0;
                        char pc = piece_char(b->fp, b->fp_rot, xp, yp);
                        if (pc != CHAR_EMPTY) {
                            c = pc;
                        }
                    }
                }
            }
            putchar(c);
            putchar(c);
        }
        putchar('\n');
    }
}

/* Check if the @row is full */
static bool
board_row_is_full(struct TetrisBoard *b, unsigned int row)
{
    bool full = true;
    for (unsigned int x = 0; x < BOARD_COLS; x++) {
        if (b->board[row][x] == CHAR_EMPTY) {
            full = false;
            break;
        }

    }
    return full;
}

/* Delete all points of the @row */
static void
board_empty_row(struct TetrisBoard *b, unsigned int row)
{
    for (int x = 0; x < BOARD_COLS; x++) {
        b->board[row][x] = CHAR_EMPTY;
    }
}

/* Remove thw row @row, pushing down the upper rows */
static void
board_remove_row(struct TetrisBoard *b, unsigned int row)
{
    for (int y = row; y > 0; y--) {
        for (int x = 0; x < BOARD_COLS; x++) {
            b->board[y][x] = b->board[y - 1][x];
        }
    }
}

/* Merge falling piece with the main grid */
static void
merge_piece(struct TetrisBoard *b)
{
    /* Loop through all the points of the shape */
    for (int y = 0; y < piece_yl(b->fp, b->fp_rot); y++) {
        for (int x = 0; x < piece_xl(b->fp, b->fp_rot); x++) {
            /* Calc coordinates of main grid */
            int xb = b->fp_x0 + x;
            int yb = b->fp_y0 + y;
            /* There's some possibility the shape is partially outside the main grid */
            if ((yb >= 0) && (xb >= 0) && (yb < BOARD_ROWS) && (xb < BOARD_COLS))  {
                char pc = piece_char(b->fp, b->fp_rot, x, y);
                if (pc != CHAR_EMPTY) {
                    b->board[yb][xb] = pc;
                }
            }
        }
    }
    b->fp = NULL;
    /* Process completed lines */
    for (unsigned int y = 0; y < BOARD_ROWS; y++) {
        if (board_row_is_full(b, y)) {
            board_empty_row(b, y);
            board_print(b);
            sleep(1);
            board_remove_row(b, y);
            board_print(b);
        }
    }
}

/* Check if the falling piece can be moved to the position (@x0,@y0).
 * Return true, if collision is found. */
static bool
piece_collision(struct TetrisBoard *b, unsigned int x0, unsigned int y0, unsigned int rot)
{
    bool collision = false;
    /* Loop through all points until one collision is found */
    for (unsigned int x = 0; !collision && (x < piece_xl(b->fp, rot)); x++) {
        for (unsigned int y = 0; !collision && (y < piece_yl(b->fp, rot)); y++) {
            char pc = piece_char(b->fp, rot, x, y);
            if (pc != CHAR_EMPTY) {
                int xb = x0 + x;    // Point position on the board
                int yb = y0 + y;
                if ((xb < 0) || (xb >= BOARD_COLS)) {
                    collision = true;       // Piece must be completely IN the board
                } else if ((yb < 0) || (yb >= BOARD_ROWS)) {
                    collision = true;       // Piece must be completely IN the board
                } else if (b->board[yb][xb] != CHAR_EMPTY) {
                    collision = true;     // Collision
                }
            }
        }
    }
    return collision;
}

/* Create a new falling piece. Returns false if the new piece can't be created because
 * there's no space left (game over) */
bool
add_new_falling_piece(struct TetrisBoard *b)
{
    int r = rand() % PIECES_NUM;
    b->fp = pieces[r];
    b->fp_rot = 0;
    b->fp_x0 = 0;
    b->fp_y0 = 0;                
    return piece_collision(b, b->fp_x0, b->fp_y0, b->fp_rot) ? false : true;
}


int
main(void)
{
    /* Set terminal in non canonical mode so we can capture every single key press
     * without waiting for the Enter keypress */
    set_conio_terminal_mode();
    atexit(reset_terminal_mode);        /* Remember to restore original terminal settings */


    board_init(&board);
    add_new_falling_piece(&board);

    /* The pause/speed of the falling piece */
    unsigned long pause_ms = 1000;
    struct timeval pause;
    pause.tv_sec = pause_ms / 1000;
    pause.tv_sec = pause_ms % 1000 * 1000;
    
    while(1) {
        /* Refresh the screen */
        board_print(&board);
        
        /* Wait for a key press or timeput */
        fd_set rfds;
        FD_ZERO(&rfds);
        FD_SET(STDIN_FILENO, &rfds);
        int ret = select(STDIN_FILENO+1, &rfds, NULL, NULL, &pause);

        if (ret < 0) {
            perror("Error with select()\n");
            exit(-1);
        
        } else if (ret > 0) {
            /* Key press event before timeout */
            int c = getchar();      // This shouldn't be blocking
            if (c == 'h') {         // Left
                if (!piece_collision(&board, board.fp_x0 - 1, board.fp_y0, board.fp_rot)) {
                    board.fp_x0 -= 1;
                }
       
            } else if (c == 'l') {  // Right
                if (!piece_collision(&board, board.fp_x0 + 1, board.fp_y0, board.fp_rot)) {
                    board.fp_x0 += 1;
                }
        
            } else if (c == 'k') {  // Rotate
                unsigned int new_rot = piece_rotate(board.fp, board.fp_rot);
                if (!piece_collision(&board, board.fp_x0, board.fp_y0, new_rot)) {
                    board.fp_rot = new_rot;
                }

            } else if (c == 'j') {  // Move piece down of 1 position
                if (!piece_collision(&board, board.fp_x0, board.fp_y0 + 1, board.fp_rot)) {
                    board.fp_y0 += 1;
                } else {            // Piece reached the lower position
                    merge_piece(&board);
                    if (!add_new_falling_piece(&board)) {
                        return 0;
                    }
                }
        
            } else if (c == ' ') {  // Move piece completely down
                while(!piece_collision(&board, board.fp_x0, board.fp_y0 + 1, board.fp_rot)) {
                    board.fp_y0 += 1;
                }
                merge_piece(&board);
                if (!add_new_falling_piece(&board)) {
                    return 0;
                }
            }

        } else {
            // Timeout expired without any keypress, push the piece down
            if (!piece_collision(&board, board.fp_x0, board.fp_y0 + 1, board.fp_rot)) {
                board.fp_y0 += 1;
            } else {
                /* Piece reached its bottom line. Merge the falling piece with the main grid */
                merge_piece(&board);
                /* ...and add new falling piece */                
                if (!add_new_falling_piece(&board)) {
                    return 0;
                }
            }
            // Restore the pause
            pause.tv_sec = pause_ms / 1000;
            pause.tv_usec = pause_ms % 1000 * 1000;
        }
        
    }

    return 0;
}

