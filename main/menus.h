#pragma once
#include <math.h>
#include <stdint.h>
#include <string.h>
#include <pic32mx.h>  /* Declarations of system-specific addresses etc */
#include "data.h"
#include "u32graphics.h"
#include "snakelogic.h"

typedef enum game_state{Start, Options, Level_diff, AI_diff, Score_board, Game, End_options, Name_input, Return_options, Cancel} game_state;
typedef enum button_type{Letter, Number, Enter, Caps, Back, Space} button_type; 
typedef struct Keyboard_button {
    Point const pos;
    button_type const type;
    char const *text;
} Keyboard_button;

typedef struct Options_button {
    uint8_t const option;
    char const *text;
} Options_button;

typedef struct Options_menu {
    Options_button const * options;
    uint8_t start;
    uint8_t index;
    uint8_t const len; 
    uint8_t const page_len;
} Options_menu;

game_state name_input(char *name);
game_state locator_menu(Options_menu * menu);