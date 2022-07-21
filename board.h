#ifndef _BOARD_H_
#define _BOARD_H_

#include <stdio.h>
#include <stdlib.h>

enum TileTypes {
    TILE_0, TILE_1, TILE_2, TILE_3, TILE_4,
    TILE_5, TILE_6, TILE_7, TILE_8, TILE_BOMB
};

enum CellStates {
    STATE_UNOPENED, STATE_OPENED,
    STATE_OPENED_SAFE, STATE_FLAGGED,
    STATE_QUESTION
};
/*
STATE_OPENED_SAFE is for displaying the entire board after the game ends.
*/

typedef struct _cell {
    int type, state;
} Cell;

typedef struct _board {
    Cell **cells;
    int width, height, bombs;
} Board;

Board *board_create(int w, int h, int n);
void board_free(Board *board);
Board *board_regen(Board *old);
void reveal(Board *board, int x, int y);
void chord(Board *board, int x, int y);
void board_reveal_entire(Board *board);
int board_calc_3bv(Board *board);

#endif