#include "board.h"

/* Get the number of bombs in a 1 cell radius around a given cell. */
int neighbouring_bombs(Board *board, int x, int y) {
    int n = 0;
    for (int i = -1; i <= 1; ++i) {
        for (int j = -1; j <= 1; ++j) {
            int x1 = x + i;
            int y1 = y + j;
            if ((i == 0 && j == 0) || x1 < 0 || x1 >= board->width || y1 < 0 || y1 >= board->height) continue;
            if (board->cells[y1][x1].type == TILE_BOMB) n++;
        }
    }
    return n;
}

void board_free(Board *board) {
    for (int i = 0; i < board->height; ++i) {
        free(board->cells[i]);
    }
    free(board->cells);
    free(board);
    return;
}

/* Create a w by h board with n bombs on it. Call free() on the returned board when done. */
Board *board_create(int w, int h, int n) {
    if (n >= w * h) return NULL;
    Board *board = malloc(sizeof(Board));
    if (!board) return NULL;
    board->width = w;
    board->height = h;
    board->bombs = n;
    board->cells = malloc(h * sizeof(Cell));
    if (!board->cells) {
        free(board);
        return NULL;
    }
    for (int i = 0; i < h; ++i) {
        board->cells[i] = malloc(w * sizeof(Cell));
        if (!board->cells[i]) {
            for (int k = 0; k < i; ++k) {
                free(board->cells[k]);
            }
            free(board->cells);
            free(board);
            return NULL;
        }
        for (int j = 0; j < w; ++j) {
            board->cells[i][j].type = TILE_0;
            board->cells[i][j].state = STATE_UNOPENED;
        }
    }
    while (n > 0) {
        int x = rand() % w;
        int y = rand() % h;
        if (board->cells[y][x].type != TILE_BOMB) {
            board->cells[y][x].type = TILE_BOMB;
            n--;
        }
    }
    for (int i = 0; i < h; ++i) {
        for (int j = 0; j < w; ++j) {
            if (board->cells[i][j].type == TILE_BOMB) continue;
            board->cells[i][j].type = neighbouring_bombs(board, j, i);
        }
    }
    for (int i = 0; i < h; ++i) {
        for (int j = 0; j < w; ++j) {
            if (board->cells[i][j].type == TILE_BOMB) printf("* ");
            else if (board->cells[i][j].type == TILE_0) printf(". ");
            else printf("%d ", board->cells[i][j].type);
        }
        printf("\n");
    }
    return board;
}
/* rewrite this, possibly causes a memleak */
Board *board_regen(Board *old) {
    int w = old->width;
    int h = old->height;
    int n = old->bombs;
    board_free(old);
    Board *b = board_create(w, h, n);
    return b;
}

/* Reveals all cells touching 0 bombs recursively until a cell touching 1 or more bombs is reached. */
void reveal(Board *board, int x, int y) {
    if (x >= 0 && x < board->width && y >= 0 && y < board->height && !(board->cells[y][x].state == STATE_OPENED)) {
        board->cells[y][x].state = STATE_OPENED;
        if (neighbouring_bombs(board, x, y) == 0) {
            reveal(board, x - 1, y);
            reveal(board, x + 1, y);
            reveal(board, x, y - 1);
            reveal(board, x, y + 1);
            reveal(board, x - 1, y - 1);
            reveal(board, x - 1, y + 1);
            reveal(board, x + 1, y - 1);
            reveal(board, x + 1, y + 1);
        }
    }
}

void chord(Board *board, int x, int y) {
    int bombs = neighbouring_bombs(board, x, y);
    int flagged = 0;
    for (int i = -1; i <= 1; ++i) {
        for (int j = -1; j <= 1; ++j) {
            int x1 = x + i;
            int y1 = y + j;
            if ((i == 0 && j == 0) || x1 < 0 || x1 >= board->width || y1 < 0 || y1 >= board->height) continue;
            if (board->cells[y1][x1].state == STATE_FLAGGED) flagged++;
        }
    }
    //printf("%d %d\n", bombs, flagged);
    if (flagged == bombs) {
        for (int i = -1; i <= 1; ++i) {
            for (int j = -1; j <= 1; ++j) {
                int x1 = x + i;
                int y1 = y + j;
                if ((i == 0 && j == 0) || x1 < 0 || x1 >= board->width || y1 < 0 || y1 >= board->height) continue;
                //if (board->cells[y1][x1].type == TILE_BOMB) continue;
                if (board->cells[y1][x1].state == STATE_FLAGGED) continue;
                board->cells[y1][x1].state = STATE_OPENED;
            }
        }
    }
}
void floodfill_mark(Board *board, int x, int y) {
    for (int i = -1; i <= 1; ++i) {
        for (int j = -1; j <= 1; ++j) {
            int x1 = x + j;
            int y1 = y + i;
            if ((i == 0 && j == 0) || x1 < 0 || x1 >= board->width || y1 < 0 || y1 >= board->height) continue;
            if (board->cells[y1][x1].state <= 4) {
                board->cells[y1][x1].state *= 5;
                if (board->cells[y1][x1].type == TILE_0) floodfill_mark(board, x1, y1);
            }
        }
    }
}
int board_calc_3bv(Board *board) {
    return 0;
    int score = 0;
    for (int i = 0; i < board->height; ++i) {
        for (int j = 0; j < board->width; ++j) {
            if (board->cells[i][j].type == TILE_0) {
                /* marking is done by multiplying the state by 5 */
                if (board->cells[i][j].state > 4) continue;
                board->cells[i][j].state *= 5;
                score++;
                floodfill_mark(board, j, i);
            } else if (board->cells[i][j].type != TILE_BOMB) score++;
        }
    }
    for (int i = 0; i < board->height; ++i) {
        for (int j = 0; j < board->width; ++j) {
            board->cells[i][j].state /= 5;
        }
    }
    return score;
}



void board_reveal_entire(Board *board) {
    for (int i = 0; i < board->height; ++i) {
        for (int j = 0; j < board->width; ++j) {
            if (board->cells[i][j].state != STATE_OPENED)
                board->cells[i][j].state = STATE_OPENED_SAFE;
        }
    }
    return;
}