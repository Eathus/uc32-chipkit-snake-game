#pragma once
#include "u32graphics.h"
#include <stdint.h>   /* Declarations of uint_32 and the like */

#define SEGMENT_SIZE 2 

extern uint16_t SNAKES[128*4];

extern Point HEAD_PLAYER[SEGMENT_SIZE];
extern Point TAIL_PLAYER[SEGMENT_SIZE];

extern Point HEAD_AI[SEGMENT_SIZE];
extern Point TAIL_AI[SEGMENT_SIZE];

typedef enum direction{Right, Left, Up, Down} direction;

uint16_t get_ustripe(Point coordinates);
uint8_t get_unit(Point coordinates);

void set_ustripe(Point coordinates, uint16_t);
void set_unit(Point coordinates, uint8_t val);

void move_snakes();

void change_dir(direction dir, Point *head);