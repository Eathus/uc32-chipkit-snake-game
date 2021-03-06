#include "snakegame.h"

int mytime = 0x5957;
char textstring[] = "text, more text, and even more text!";

void tick( unsigned int * timep )
{
  /* Get current value, store locally */
  register unsigned int t = * timep;
  t += 1; /* Increment local copy */
  
  /* If result was not a valid BCD-coded time, adjust now */

  if( (t & 0x0000000f) >= 0x0000000a ) t += 0x00000006;
  if( (t & 0x000000f0) >= 0x00000060 ) t += 0x000000a0;
  /* Seconds are now OK */

  if( (t & 0x00000f00) >= 0x00000a00 ) t += 0x00000600;
  if( (t & 0x0000f000) >= 0x00006000 ) t += 0x0000a000;
  /* Minutes are now OK */

  if( (t & 0x000f0000) >= 0x000a0000 ) t += 0x00060000;
  if( (t & 0x00ff0000) >= 0x00240000 ) t += 0x00dc0000;
  /* Hours are now OK */

  if( (t & 0x0f000000) >= 0x0a000000 ) t += 0x06000000;
  if( (t & 0xf0000000) >= 0xa0000000 ) t = 0;
  /* Days are now OK */

  * timep = t; /* Store new value */
}

/* Interrupt Service Routine */
void user_isr( void )
{
  return;
}

/* Lab-specific initialization goes here */
/*
*	Function:	game_init
*	--------------------
*	doing necessary chikit startup routines to make sure it works 
* correctly
*
*	returns: void
*/
void game_init( void )
{
  float periodms = 0.01;
  int scale = 256;
  int ck_freq = 80000000;

  int period = ((periodms * ck_freq) / scale);

  T2CONCLR = 0x8000;
  T2CON = 0x0070;
  T2CONSET = 0x8000;
  
  T3CONCLR = 0x8000;
  T3CON = 0x0;
  T3CONSET = 0x8000;

  T4CONCLR = 0x8000;
  T4CON = 0x40;
  T4CONSET = 0x8000;
  
  PR2 = period;
  TMR2 = 0;
  TMR3 = 0;
  TMR4 = 0;
  /* Modifying the 8 least significant bits set as output*/
  TRISE &= 0xff00;
  /* Initialize port D with the definitions in the pic32mx.h file. Bits
  11 through 5 are set as input. */
  TRISD |= 0x0fe0;
  TRISF |= 0x0002;

  IPC(2) = (0x0 << 2 | 0x0) | (IPC(2) & ~0x1F); 
  IFS(0) &= (unsigned int)(~0x100);
  IEC(0) |= (0x100);
  asm volatile("ei");
  return;
}

/*
*	Function:	draw_level
*	--------------------
* draw a level based on difficulty level before 
* game starts  
*
* level: difficulty currently selecting
*
*	returns: void
*/
void draw_level(difficulty level){
  switch (level)
  {
  case Easy:{
    Image obs1 = {
      Row, 5, 12, HANG_OBSTICAL1
    };
    Image obs2 = {
      Row, 5, 17, HANG_OBSTICAL2
    };
    Image obs3 = {
      Row, 4, 6, HANG_OBSTICAL3
    };
    Image obs4 = {
      Row, 7, 21, HANG_OBSTICAL4
    };
    Image obs5 = {
      Row, 4, 8, HANG_OBSTICAL5
    };
    draw_image(SCREEN, &obs1, (Point){14, 0});
    draw_image(SCREEN, &obs2, (Point){44, 15});
    draw_image(SCREEN, &obs3, (Point){72, 26});
    draw_image(SCREEN, &obs4, (Point){91, 0});
    draw_image(SCREEN, &obs5, (Point){110, 0});
    break;
  }
  case Hard:{
      int i, j;
      draw_square(SCREEN, (Point){51, 5}, 9, On);
      draw_square(SCREEN, (Point){51, 20}, 9, On);
      draw_square(SCREEN, (Point){67, 5}, 9, On);
      draw_square(SCREEN, (Point){67, 20}, 9, On);
      break;
    }
  default:
    break;
  }

}

/*
*	Struct:	Dir_queue
*	-------------------
* queue structure (first in first out) for directions of snake
* used for direction buffer to buffer inputs
* and make sure a snake only gets to move every 2 pixel movements
*
* size: size of the queue
* last: current index of the last item in the queue 
* (< 0 if theres no items in the queue)
* queue: actual array for the queue
*/
typedef struct Dir_queue {
  uint8_t size;
  int8_t last;
  direction *queue;
} Dir_queue;

/*
*	Function:	qpush
*	---------------
* push direction to the back of the queue
*
* dir: direction snake wants to go
* queue: current queue
* replace: if replace is active (1) when queue is full 
* replace first direction in the queue (on index 0) with new direction 
*
*	returns: void
*/
void qpush(direction dir, Dir_queue *queue, uint8_t replace){
  if(queue->last < queue->size - 1) {
    if(queue->last < 0){
      queue->last++;
      queue->queue[queue->last] = dir;
    }
    else if (queue->queue[queue->last] != dir){
      queue->last++;
      queue->queue[queue->last] = dir;
    }
  }
  else if (replace) queue->queue[0] = dir;
}

/*
*	Function:	qpop
*	---------------
* pop direction to from front of queue 
* to get access to that direction
*
* queue: current queue
*
*	returns: direction in front of the queue 
* (according to first in first out order)
*/
uint8_t qpop(Dir_queue *queue){
  if(queue->last < 0) return -1; 
  int i;
  direction ret = queue->queue[0];
  for(i = 1; i <= queue->last; ++i){
    queue->queue[i-1] = queue->queue[i];
  }
  queue->last--;
  return ret;
}

/*
*	Function:	snake_game
*	--------------------
* main function controling the entire snake game
*
* ai: difficulty: level of the AI
* level: difficulty: level of the current level/obstiacls
* current_score: storage variable to keep track on the players
* current score
*
*	returns: where to go in the program after a game is over
*/
/* This function is called repetitively from the main program */
game_state snake_game(difficulty ai, difficulty level, int *current_score){
  if(level != None) draw_level(level);
  uint8_t ai_alive = ai;
  
  uint16_t snakes[128*4];
  
  //AI
  uint8_t ai_tracker[128*4];
  clear_frame(ai_tracker);
  Point ai_head[SEGMENT_SIZE];
  Point ai_tail[SEGMENT_SIZE];
  int ai_score = 0;
  if(ai_alive){
    ai_tail[0] = (Point){127, 14};
    spawn_snake(ai_tail, ai_head, 10, Left, snakes, SCREEN);  
    spawn_snake(ai_tail, ai_head, 10, Left, snakes, ai_tracker);  
  }


  direction ai_queue[SEGMENT_SIZE];
  Dir_queue ai_dir_buffer = {SEGMENT_SIZE, -1, ai_queue};
  uint8_t grow_ai = 0;

  //Player
  Point player_head[SEGMENT_SIZE];
  Point player_tail[SEGMENT_SIZE];

  player_tail[0] = (Point){0, 14};
  Point food_pos = {61, 14};
  spawn_snake(player_tail, player_head, 10, Right, snakes, SCREEN);

  //Game
  int update_counter = 0;
  direction player_queue[SEGMENT_SIZE];
  Dir_queue player_dir_buffer = {SEGMENT_SIZE, -1, player_queue};

  uint8_t frame_update = 0;
  toggle_food(SCREEN, On, &food_pos);
  uint8_t grow_player = 0;

  /* Variables to handle input */
  int btn;
  while (1){
    int inter_flag = (IFS(0) & 0x100) >> 8;
    btn = getbtns();
    if(inter_flag){
      IFS(0) &= (unsigned int)(~0x100);
      ++update_counter;
    }   
    /* Update input outside of check for update counter for best responsiveness*/
    //Player
    /* BTN4 - AND with corresponding bit*/
    if (btn & 0x8) {
      qpush(Left, &player_dir_buffer, 1);
    } 
    /* BTN3 - AND with corresponding bit*/
    if (btn & 0x4) {
      qpush(Right, &player_dir_buffer, 1);
    }
    /* BTN2 - AND with corresponding bit*/
    if(btn & 0x2) {
      qpush(Up, &player_dir_buffer, 1);
    }
    /* BTN1 - AND with corresponding bit*/
    if (btn & 0x1) {
      qpush(Down, &player_dir_buffer, 1);
    }

    if(update_counter == 5){
      update_counter = 0;
      if(frame_update == SEGMENT_SIZE){
        uint8_t dir_change = qpop(&player_dir_buffer);
        if(dir_change != -1) change_dir(dir_change, player_head, snakes);
        if(!ai_alive) frame_update = 0;
      }
      snake_state pstate = update_player_snake(player_head, player_tail, &food_pos, &grow_player, snakes, SCREEN);
      
      //AI
      if(ai_alive){
        snake_state ai_state;
        Point check_dists;
        int mistake_chance = 0;
        switch (ai)
        {
        case Hard:{
          check_dists = (Point){127, 31};
          break;
        }
        case Normal:
          check_dists = (Point){48, 10};
          mistake_chance = 10;
          break;
        case Easy:
          check_dists = (Point){16, 4};
          mistake_chance = 30;
          break;
        default:
          break;
        }
        if(frame_update == SEGMENT_SIZE){
          direction ai_dir;
          //makes snanke choose random direction once in a while without logic to decrease it's difficulty
          if(mistake_chance != 0 && abs(irand(player_tail, ai_head)) % 100 < mistake_chance){
            ai_dir = abs(irand(player_tail, ai_head)) % 4;
            int i;
            //make sure the snake doesn't randomly kill itself when randomly choosing direction
            Point ret[SEGMENT_SIZE];
            get_rotated(get_unit(*ai_head, snakes), ai_dir, ai_head, ret);
            for(i = 0; i < SEGMENT_SIZE; ++i){
              if(status_ahead(ai_dir, ret, i + 1, food_pos, SCREEN) == Dead){
                ai_dir = snake_ai(ai_head, food_pos, check_dists, snakes, SCREEN);
                break;
              }
            }
          }
          else ai_dir = snake_ai(ai_head, food_pos, check_dists, snakes, SCREEN);
          qpush(ai_dir, &ai_dir_buffer, 1);
          if(frame_update == SEGMENT_SIZE){
            uint8_t dir_change = qpop(&ai_dir_buffer);
            if(dir_change != -1) change_dir(dir_change, ai_head, snakes);
            frame_update = 0;
          }
        }
        ai_state = update_ai_snake(ai_head, ai_tail, &food_pos, &grow_ai, snakes, SCREEN, ai_tracker);
        switch (ai_state)
        {
        case Ate:
          if(pstate != Ate){
            ai_score += level + ai + 1;
            if(update_food(SCREEN, prand(player_tail, player_head), &food_pos)) return End_options;
            toggle_food(SCREEN, On, &food_pos);
          }
          break;
        case Dead:{
          int i, j;
          for(i = 0; i < 32; ++i){
            for(j = 0; j < 128; ++j){
              Point current = {j, i};
              if(get_pixel(ai_tracker, current))
                set_pixel(SCREEN, current, Off);
            }
          }
          ai_alive = 0;
        }
        default:
          break;
        }
      }

      switch (pstate){
        case Ate:
          *current_score += level + ai + 1;
          if(update_food(SCREEN, prand(player_tail, player_head), &food_pos)) return End_options;
          //if(update_food((Point){(HEAD_PLAYER[0].x + 5) %128, HEAD_PLAYER[0].y})) return Full;
          toggle_food(SCREEN, On, &food_pos);
        break;          
        case Dead: case Full:{
          clear_frame(SCREEN);
          game_state ret = End_options;
          int pscore_len;
          char *pscore = itoaconv(*current_score, &pscore_len);

          //Classic snake
          if(ai == None){
            write_row(SCREEN, 0, "   GAME OVER   ");
            write_row(SCREEN, 1, "SCORE:");
            write_string(SCREEN, (Point){16 - pscore_len, 1}, pscore, pscore_len);
          }
          //snake against AI if you lose you don't et to save your score
          else{
            if(*current_score < ai_score){
              write_row(SCREEN, 0, "   YOU LOSE    ");
              ret = End_options_lose;
            }
            else write_row(SCREEN, 0, "    YOU WIN    ");
            write_row(SCREEN, 1, "PLAYER:");
            write_string(SCREEN, (Point){16 - pscore_len, 1}, pscore, pscore_len);
            write_row(SCREEN, 2, "AI:");
            int ai_score_len;
            char *ai_score_string = itoaconv(ai_score, &ai_score_len);
            write_string(SCREEN, (Point){16 - ai_score_len, 2}, ai_score_string, ai_score_len);
          }
          wait_for_btn((Point){0, 24});
          return ret;
        }
        default:
          break;
      }

      update_disp(SCREEN); 
      frame_update++;
    }
  }
}