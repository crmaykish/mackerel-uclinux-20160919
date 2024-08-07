#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

#define TERM_CURSOR_HIDE "\033[?25l"
#define TERM_CURSOR_SHOW "\033[?25h"

#define BOARD_WIDTH 24
#define BOARD_HEIGHT 16
#define MAX_GENERATIONS 100
#define RANDOM_SEED_FILL 25

static uint8_t current[BOARD_WIDTH][BOARD_HEIGHT];
static uint8_t previous[BOARD_WIDTH][BOARD_HEIGHT];

static uint8_t cycles;
static uint16_t seed;
static uint8_t i, j;
static uint8_t in;

void init_board();
void draw_board();
void update_state();
uint8_t count_neighbors(uint8_t x, uint8_t y);
void rand_prompt();

void term_cursor_set_pos(uint8_t x, uint8_t y)
{
    printf("\033[%d;%dH", x, y);
}

void term_cursor_set_vis(bool visible)
{
    printf(visible ? TERM_CURSOR_SHOW : TERM_CURSOR_HIDE);
}

int main()
{
    term_cursor_set_vis(false);

    printf("\033[2J\033[H");

    init_board();

    while (cycles <= MAX_GENERATIONS)
    {
        update_state();
        draw_board();
        cycles++;

        // sleep(1);
    }

    printf("Game finished.\n");
    term_cursor_set_vis(true);
}

void init_board()
{
    cycles = 1;

    for (j = 0; j < BOARD_HEIGHT; ++j)
    {
        for (i = 0; i < BOARD_WIDTH; ++i)
        {
            previous[i][j] = ((rand() % 100) < RANDOM_SEED_FILL) ? 1 : 0;
        }
    }
}

void draw_board()
{
    term_cursor_set_pos(0, 0);

    printf("Conway's Game of Life | Generation: %d\n", cycles);

    for (j = 0; j < BOARD_HEIGHT; ++j)
    {
        for (i = 0; i < BOARD_WIDTH; ++i)
        {
            if (current[i][j] == 1)
            {
                putc('[', stdout);
                putc(']', stdout);
            }
            else
            {
                putc(' ', stdout);
                putc(' ', stdout);
            }
        }

        putc('\r', stdout);
        putc('\n', stdout);
    }
}

void update_state()
{
    uint8_t neighbors;

    // Clear current board
    memset(current, 0, BOARD_WIDTH * BOARD_HEIGHT);

    for (j = 0; j < BOARD_HEIGHT; ++j)
    {
        for (i = 0; i < BOARD_WIDTH; ++i)
        {
            neighbors = count_neighbors(i, j);

            if (neighbors == 3 && previous[i][j] == 0)
            {
                // Cell comes alive
                current[i][j] = 1;
            }
            else if ((neighbors == 2 || neighbors == 3) && previous[i][j] == 1)
            {
                // Cell stays alive
                current[i][j] = 1;
            }
            else
            {
                // Cell dies
                current[i][j] = 0;
            }
        }
    }

    // Copy current board to previous
    memcpy(previous, current, BOARD_WIDTH * BOARD_HEIGHT);
}

uint8_t count_neighbors(uint8_t x, uint8_t y)
{
    // Living neighbors
    uint8_t living = 0;

    if (x > 0 && y > 0)
        living += previous[x - 1][y - 1];

    if (x > 0)
        living += previous[x - 1][y];

    if (x > 0 && y < BOARD_HEIGHT - 1)
        living += previous[x - 1][y + 1];

    if (y > 0)
        living += previous[x][y - 1];

    if (y < BOARD_HEIGHT - 1)
        living += previous[x][y + 1];

    if (x < BOARD_WIDTH - 1 && y > 0)
        living += previous[x + 1][y - 1];

    if (x < BOARD_WIDTH - 1)
        living += previous[x + 1][y];

    if (x < BOARD_WIDTH - 1 && y < BOARD_HEIGHT - 1)
        living += previous[x + 1][y + 1];

    return living;
}
