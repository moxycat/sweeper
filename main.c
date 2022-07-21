#include <stdio.h>
#include <time.h>
#include <windows.h>
#include <commctrl.h>
#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>
//#include <SDL2/SDL_image.h>


#include "board.h"
#include "resource.h"

#define WIDTH 800
#define HEIGHT 600

#define TOP_PADDING    0
#define BOTTOM_PADDING 0
#define LEFT_PADDING   8
#define RIGHT_PADDING  8

SDL_Window *g_wind = NULL;
SDL_Renderer *g_renderer = NULL;
HWND g_hwnd = NULL;

UINT width = 8, height = 8, bombs = 10;
UINT user_flags = 0;
Board *board;
int board_3bv;

Uint64 game_start_tick, game_end_tick;

int quit = 0;

SDL_Surface *surf_from_file(char *file, SDL_PixelFormat *fmt) {
    SDL_Surface *s = SDL_LoadBMP(file);
    if (!s) return NULL;
    SDL_Surface *s1 = SDL_ConvertSurface(s, fmt, 0);
    SDL_FreeSurface(s);
    if (!s1) return NULL;
    return s1;
}

SDL_Texture *texture_from_file(char *file) {
    SDL_Texture *t = NULL;
    SDL_Surface *s = SDL_LoadBMP(file);
    if (!s) return NULL;
    t = SDL_CreateTextureFromSurface(g_renderer, s);
    SDL_FreeSurface(s);
    if (!t) return NULL;
    return t;
}

void draw_board(SDL_Texture *sheet, SDL_Rect wind_rect, SDL_Rect texture_rect, Board *board) {
    for (int i = 0; i < board->height; ++i) {
        for (int j = 0; j < board->width; ++j) {
            wind_rect.x = 16 * j;
            wind_rect.y = 16 * i;
            switch (board->cells[i][j].state) {
                case STATE_OPENED:
                case STATE_OPENED_SAFE:
                    switch (board->cells[i][j].type) {
                        case TILE_0: case TILE_1: case TILE_2:
                        case TILE_3: case TILE_4: case TILE_5:
                        case TILE_6: case TILE_7: case TILE_8:
                            texture_rect.y = 240 - 16 * board->cells[i][j].type; break;
                        case TILE_BOMB:
                            if (board->cells[i][j].state == STATE_OPENED_SAFE) texture_rect.y = 80;
                            else texture_rect.y = 48;
                            break;
                    }
                    break;
                case STATE_UNOPENED: texture_rect.y = 0; break;
                case STATE_FLAGGED: texture_rect.y = 16; break;
                case STATE_QUESTION: texture_rect.y = 32; break;
            }
            SDL_RenderCopy(g_renderer, sheet, &texture_rect, &wind_rect);
        }
    }
}

int check_board(Board *board) {
    int correct_flags = 0;
    for (int i = 0; i < board->height; ++i) {
        for (int j = 0; j < board->width; ++j) {
            switch (board->cells[i][j].type) {
                case TILE_BOMB:
                    if (board->cells[i][j].state == STATE_FLAGGED) correct_flags++;
                    else if (board->cells[i][j].state == STATE_OPENED) return -1; /* kaboom! */
                    break;
                default: break;
            }
        }
    }
    if (correct_flags == board->bombs) return 1; /* win! */
    return 0; /* nothing changed */
}

BOOL CALLBACK SettingsDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam) {
    BOOL succ;
    switch (Message) {
        case WM_INITDIALOG:
            SetDlgItemInt(hwnd, IDD_DIALOG1_WIDTH, width, FALSE);
            SetDlgItemInt(hwnd, IDD_DIALOG1_HEIGHT, height, FALSE);
            SetDlgItemInt(hwnd, IDD_DIALOG1_BOMBS, bombs, FALSE);
            return TRUE;
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDD_DIALOG1_SAVE:
                    //printf("aaa\n");
                    //printf("%d %d %d\n", width, height, bombs);
                    width = GetDlgItemInt(hwnd, IDD_DIALOG1_WIDTH, &succ, FALSE);
                    height = GetDlgItemInt(hwnd, IDD_DIALOG1_HEIGHT, &succ, FALSE);
                    bombs = GetDlgItemInt(hwnd, IDD_DIALOG1_BOMBS, &succ, FALSE);
                    //printf("%d %d %d\n", width, height, bombs);
                    if (width > 0 && height > 0 && bombs > 0 && bombs < width * height) EndDialog(hwnd, IDD_DIALOG1_SAVE);
                    break;
                case IDD_DIALOG1_CANCEL:
                    if (width > 0 && height > 0 && bombs > 0 && bombs < width * height) EndDialog(hwnd, IDD_DIALOG1_CANCEL);
                    break;
                default: break;
            }
            break;
        case WM_CLOSE:
            if (width > 0 && height > 0 && bombs > 0) EndDialog(hwnd, IDD_DIALOG1_CANCEL);
            break;
        default: return FALSE;
    }
    return TRUE;
}

BOOL CALLBACK ResultDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam) {
    int size;
    char *buf;
    switch (Message) {
        case WM_INITDIALOG:
            SetDlgItemTextA(hwnd, IDD_DIALOG2_RESULT, lParam > 0 ? "You win!" : "You lost!");
            size = snprintf(NULL, 0, "Time: %.04f sec", ((double)game_end_tick - (double)game_start_tick) / 1000);
            buf = malloc((size + 1) * sizeof(char));
            snprintf(buf, size + 1, "Time: %.04f sec", ((double)game_end_tick - (double)game_start_tick) / 1000);
            SetDlgItemTextA(hwnd, IDD_DIALOG2_TIME, buf);
            free(buf);
            size = snprintf(NULL, 0, "3BV: %d", board_3bv);
            buf = malloc((size + 1) * sizeof(char));
            snprintf(buf, size + 1, "3BV: %d", board_3bv);
            SetDlgItemTextA(hwnd, IDD_DIALOG2_3BV, buf);
            free(buf);
            return TRUE;
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case ID_OK: EndDialog(hwnd, ID_OK);
                default: break;
            }
            break;
        case WM_CLOSE: EndDialog(hwnd, ID_OK);
        default: return FALSE;
    }
    return TRUE;
}

int main(int argc, char **argv) {
    srand(time(NULL));
    //FreeConsole();
    if (SDL_Init(SDL_INIT_EVERYTHING) < 0) {
        printf("Failed to start SDL2.\n");
        return -1;
    }
    SDL_EventState(SDL_SYSWMEVENT, SDL_ENABLE);
    g_wind = SDL_CreateWindow("Minesweeper", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WIDTH, HEIGHT, SDL_WINDOW_SHOWN);
    if (!g_wind) {
        printf("Failed to create window.\n");
        return -1;
    }
    g_renderer = SDL_CreateRenderer(g_wind, -1, SDL_RENDERER_ACCELERATED);
    if (!g_renderer) {
        printf("Failed to create renderer.\n");
        return -1;
    }

    SDL_SysWMinfo wmi;
    SDL_VERSION(&wmi.version);
    if (!SDL_GetWindowWMInfo(g_wind, &wmi)) return 1;
    g_hwnd = wmi.info.win.window;
    
    HMENU menu = LoadMenuA(GetModuleHandleA(0), MAKEINTRESOURCEA(IDR_MENU1));
    SetMenu(g_hwnd, menu);
    

    SDL_Texture *sheet = texture_from_file("410.bmp");
    //SDL_Texture *numbers = texture_from_file("420.bmp");
    int cell_x, cell_y;
    int ret;
    SDL_Event ev;

    /* draw board */
    SDL_Rect wind_rect, texture_rect;
    wind_rect = (SDL_Rect){.x = 0, .y = 0, .w = 16, .h = 16};
    texture_rect = (SDL_Rect){.x = 0, .y = 0};
    SDL_QueryTexture(sheet, NULL, NULL, &texture_rect.w, &texture_rect.h);
    texture_rect.h /= 16;

    board = board_create(width, height, bombs);

    SDL_SetWindowSize(g_wind, 16 * board->width, 16 * board->height);
    SDL_UpdateWindowSurface(g_wind);

    SDL_SetRenderDrawColor(g_renderer, 192, 192, 192, 0);
    SDL_RenderClear(g_renderer);

    draw_board(sheet, wind_rect, texture_rect, board);

    SDL_RenderPresent(g_renderer);

    game_start_tick = SDL_GetTicks64();
    board_3bv = board_calc_3bv(board);
    
    while (!quit) {
        /* event handler */
        while (SDL_PollEvent(&ev) != 0) {
            switch (ev.type) {
                case SDL_SYSWMEVENT:
                    switch (ev.syswm.msg->msg.win.msg) {
                        case WM_COMMAND:
                            switch (LOWORD(ev.syswm.msg->msg.win.wParam)) {
                                case IDR_MENU1_GAME_EXIT: quit = 1; break;
                                case IDR_MENU1_GAME_SETTINGS:
                                    ret = DialogBoxParamA(GetModuleHandleA(0), MAKEINTRESOURCEA(IDD_DIALOG1), g_hwnd, SettingsDlgProc, 0);
                                    if (ret == IDD_DIALOG1_CANCEL) break;
                                    board_free(board);
                                    board = board_create(width, height, bombs);
                                    SDL_SetWindowSize(g_wind, 16 * board->width, 16 * board->height);
                                    SDL_UpdateWindowSurface(g_wind);
                                    game_start_tick = SDL_GetTicks64();
                                    board_3bv = board_calc_3bv(board);
                                    break;
                                case IDR_MENU1_GAME_NEW:
                                    board_free(board);
                                    board = board_create(width, height, bombs);
                                    SDL_SetWindowSize(g_wind, 16 * board->width, 16 * board->height);
                                    SDL_UpdateWindowSurface(g_wind);
                                    game_start_tick = SDL_GetTicks64();
                                    board_3bv = board_calc_3bv(board);
                                    break;
                                case IDR_MENU1_HELP_ABOUT: break;
                                default: break;
                            }
                        default: break;
                    }
                    break;
                case SDL_QUIT: quit = 1; break;
                case SDL_MOUSEMOTION: break;
                case SDL_KEYDOWN:
                    switch (ev.key.keysym.sym) {
                        case SDLK_ESCAPE: quit = 1; break;
                        case SDLK_r: board = board_regen(board); break;
                        case SDLK_F2:
                            ret = DialogBoxParamA(GetModuleHandleA(0), MAKEINTRESOURCEA(IDD_DIALOG1), g_hwnd, SettingsDlgProc, 0);
                            if (ret == IDD_DIALOG1_CANCEL) break;
                            board_free(board);
                            board = board_create(width, height, bombs);
                            SDL_SetWindowSize(g_wind, 16 * board->width, 16 * board->height);
                            SDL_UpdateWindowSurface(g_wind);
                            game_start_tick = SDL_GetTicks64();
                            board_3bv = board_calc_3bv(board);
                            break;
                        default: break;
                    }
                    break;
                case SDL_MOUSEBUTTONDOWN:
                    cell_x = ev.button.x / 16;
                    cell_y = ev.button.y / 16;
                    if (ev.button.button == SDL_BUTTON_LEFT) {
                        reveal(board, cell_x, cell_y);
                    }
                    else if (ev.button.button == SDL_BUTTON_RIGHT) {
                        int newstate = 0;
                        switch (board->cells[cell_y][cell_x].state) {
                            case STATE_UNOPENED:
                                newstate = STATE_FLAGGED;
                                user_flags++;
                                break;
                            case STATE_OPENED:
                                newstate = STATE_OPENED;
                                chord(board, cell_x, cell_y);
                                break;
                            case STATE_FLAGGED:
                                newstate = STATE_UNOPENED;
                                user_flags--;
                                break;
                            case STATE_QUESTION: newstate = STATE_UNOPENED; break;
                        }
                        board->cells[cell_y][cell_x].state = newstate;
                    }
                    else if (ev.button.button == SDL_BUTTON_MIDDLE) {
                        board->cells[cell_y][cell_x].state = STATE_QUESTION;
                    }
                    break;
                default: break;
            }
        }

        /* renderer */
        //SDL_SetRenderDrawColor(g_renderer, 192, 192, 192, 0);
        SDL_RenderClear(g_renderer);
        draw_board(sheet, wind_rect, texture_rect, board);
        SDL_RenderPresent(g_renderer);

        int result = check_board(board);
        if (result != 0) {
            game_end_tick = SDL_GetTicks64();
            SDL_RenderClear(g_renderer);
            board_reveal_entire(board);
            draw_board(sheet, wind_rect, texture_rect, board);
            SDL_RenderPresent(g_renderer);
            DialogBoxParamA(GetModuleHandleA(0), MAKEINTRESOURCEA(IDD_DIALOG2), g_hwnd, ResultDlgProc, result);
            board = board_regen(board);
            game_start_tick = SDL_GetTicks64();
            board_3bv = board_calc_3bv(board);
        }
        
    }
    
    board_free(board);
    DestroyMenu(menu);
    SDL_DestroyRenderer(g_renderer);
    SDL_DestroyWindow(g_wind);
    SDL_Quit();
    return 0;
}