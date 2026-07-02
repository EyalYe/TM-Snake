/*
 * app_snake.c — Snake game for TaskMaster-C3.
 *
 * Controlled using the rotary encoder:
 *   - Turn CW to steer the snake's head clockwise (90 degrees right)
 *   - Turn CCW to steer the snake's head counter-clockwise (90 degrees left)
 *   - Press Select button to reset/restart the game
 *
 * Fullscreen display (no hint bar) with a 32x16 grid of 4x4 pixel blocks.
 */

#include "app.h"
#include "input.h"
#include "ui_frame.h"
#include "app_store.h"
#include "esp_timer.h"
#include "esp_random.h"
#include <stdio.h>

/* Require the app-API version this app is written against. */
TASKMASTER_REQUIRE_API(1, 1); /* Needs 1.1 for tick_ms periodic redraw support */

/* Configuration constants */
#define SNAKE_GRID_W            32
#define SNAKE_GRID_H            16
#define SNAKE_BLOCK_SIZE        3
#define SNAKE_BLOCK_STEP        4
#define SNAKE_MAX_LEN           256
#define SNAKE_GAME_TICK_MS      150
#define SNAKE_SPAWN_ATTEMPTS    100
#define SNAKE_START_LEN         3

/* UI Row Positions */
#define SNAKE_ROW_TITLE         0
#define SNAKE_ROW_SCORE         2
#define SNAKE_ROW_HIGH_SCORE    3
#define SNAKE_ROW_PROMPT        4

/* Coordinate Offsets for Screen Centering */
#define SNAKE_X_OFFSET          0
#define SNAKE_Y_OFFSET          0

/* Game states */
static bool s_game_started;
static bool s_game_over;
static int s_snake_x[SNAKE_MAX_LEN];
static int s_snake_y[SNAKE_MAX_LEN];
static int s_snake_len;

/* Directions: dx, dy */
static int s_dir_x;
static int s_dir_y;
static int s_next_dir_x;
static int s_next_dir_y;
static bool s_dir_changed_this_tick;

/* Game entities */
static int s_food_x;
static int s_food_y;
static uint32_t s_score;
static uint32_t s_high_score;

/* NVS storage */
static app_store_t s_store;
static uint32_t s_last_update_ms;

/* Forward declarations */
static void snake_init(void);
static void snake_on_event(uint8_t ev);
static void snake_render(void);
static void snake_exit(void);
static void init_game(void);
static void update_game_state(void);
static void spawn_food(void);
static void change_direction(bool cw);
static void draw_block(int gx, int gy);
static void draw_food(int gx, int gy);

/* Helper to draw a single filled snake block */
static void draw_block(int gx, int gy)
{
    lv_obj_t *rect = lv_obj_create(ui_frame_content());
    lv_obj_set_size(rect, SNAKE_BLOCK_SIZE, SNAKE_BLOCK_SIZE);
    lv_obj_set_pos(rect, SNAKE_X_OFFSET + gx * SNAKE_BLOCK_STEP, SNAKE_Y_OFFSET + gy * SNAKE_BLOCK_STEP);
    lv_obj_set_style_bg_color(rect, lv_color_white(), 0);
    lv_obj_set_style_bg_opa(rect, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(rect, 0, 0);
    lv_obj_set_style_radius(rect, 0, 0);
}

/* Helper to draw a hollow food block */
static void draw_food(int gx, int gy)
{
    lv_obj_t *rect = lv_obj_create(ui_frame_content());
    lv_obj_set_size(rect, SNAKE_BLOCK_SIZE, SNAKE_BLOCK_SIZE);
    lv_obj_set_pos(rect, SNAKE_X_OFFSET + gx * SNAKE_BLOCK_STEP, SNAKE_Y_OFFSET + gy * SNAKE_BLOCK_STEP);
    lv_obj_set_style_bg_opa(rect, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_color(rect, lv_color_white(), 0);
    lv_obj_set_style_border_width(rect, 1, 0);
    lv_obj_set_style_radius(rect, 0, 0);
}

/* Spawns food at a random unoccupied cell */
static void spawn_food(void)
{
    bool occupied;
    int attempts = 0;
    do {
        occupied = false;
        s_food_x = (int)(esp_random() % SNAKE_GRID_W);
        s_food_y = (int)(esp_random() % SNAKE_GRID_H);
        
        for (int i = 0; i < s_snake_len; i++) {
            if (s_snake_x[i] == s_food_x && s_snake_y[i] == s_food_y) {
                occupied = true;
                break;
            }
        }
        attempts++;
    } while (occupied && attempts < SNAKE_SPAWN_ATTEMPTS);
}

/* Resets game state to start a new match */
static void init_game(void)
{
    s_game_over = false;
    s_game_started = true;
    s_score = 0;
    s_snake_len = SNAKE_START_LEN;
    
    /* Start near screen center */
    const int start_x = SNAKE_GRID_W / 2;
    const int start_y = SNAKE_GRID_H / 2;
    
    /* Segment 0: Head, segments 1, 2: tail extending left */
    s_snake_x[0] = start_x;
    s_snake_y[0] = start_y;
    s_snake_x[1] = start_x - 1;
    s_snake_y[1] = start_y;
    s_snake_x[2] = start_x - 2;
    s_snake_y[2] = start_y;
    
    /* Default direction: RIGHT */
    s_dir_x = 1;
    s_dir_y = 0;
    s_next_dir_x = 1;
    s_next_dir_y = 0;
    s_dir_changed_this_tick = false;
    
    s_last_update_ms = (uint32_t)(esp_timer_get_time() / 1000);
    spawn_food();
}

/* Changes the direction buffer (CW or CCW turn) */
static void change_direction(bool cw)
{
    /* Only allow one direction change per game tick */
    if (s_dir_changed_this_tick) {
        return;
    }
    
    if (cw) {
        s_next_dir_x = -s_dir_y;
        s_next_dir_y = s_dir_x;
    } else {
        s_next_dir_x = s_dir_y;
        s_next_dir_y = -s_dir_x;
    }
    s_dir_changed_this_tick = true;
}

/* Advances the snake and checks for collisions */
static void update_game_state(void)
{
    uint32_t now = (uint32_t)(esp_timer_get_time() / 1000);
    if (s_game_over || !s_game_started) {
        return;
    }
    if (now - s_last_update_ms < SNAKE_GAME_TICK_MS) {
        return;
    }
    s_last_update_ms = now;

    /* Apply direction updates */
    s_dir_x = s_next_dir_x;
    s_dir_y = s_next_dir_y;
    s_dir_changed_this_tick = false;

    /* Calculate new head position */
    int new_head_x = s_snake_x[0] + s_dir_x;
    int new_head_y = s_snake_y[0] + s_dir_y;

    /* Grid boundary collision check */
    if (new_head_x < 0 || new_head_x >= SNAKE_GRID_W || new_head_y < 0 || new_head_y >= SNAKE_GRID_H) {
        s_game_over = true;
        if (s_score > s_high_score) {
            s_high_score = s_score;
            app_store_set_u32(&s_store, "highscore", s_high_score);
        }
        return;
    }

    /* Self collision check */
    for (int i = 0; i < s_snake_len; i++) {
        if (s_snake_x[i] == new_head_x && s_snake_y[i] == new_head_y) {
            s_game_over = true;
            if (s_score > s_high_score) {
                s_high_score = s_score;
                app_store_set_u32(&s_store, "highscore", s_high_score);
            }
            return;
        }
    }

    /* Food collision check */
    bool ate_food = (new_head_x == s_food_x && new_head_y == s_food_y);

    int next_len = s_snake_len;
    if (ate_food) {
        if (s_snake_len < SNAKE_MAX_LEN) {
            next_len++;
        }
        s_score++;
    }

    /* Shift snake coordinates */
    for (int i = next_len - 1; i > 0; i--) {
        s_snake_x[i] = s_snake_x[i - 1];
        s_snake_y[i] = s_snake_y[i - 1];
    }
    s_snake_x[0] = new_head_x;
    s_snake_y[0] = new_head_y;
    s_snake_len = next_len;

    if (ate_food) {
        spawn_food();
    }
}

/* Called when the app is launched */
static void snake_init(void)
{
    s_game_started = false;
    s_game_over = false;
    s_score = 0;
    
    app_store_open(&s_store, "snake");
    app_store_get_u32(&s_store, "highscore", &s_high_score, 0);
}

/* Handle input events */
static void snake_on_event(uint8_t ev)
{
    switch (ev) {
    case EV_ENCODER_CW:
        if (s_game_started && !s_game_over) {
            change_direction(true);
        }
        break;
    case EV_ENCODER_CCW:
        if (s_game_started && !s_game_over) {
            change_direction(false);
        }
        break;
    case EV_SELECT:
        /* SELECT resets or starts the game */
        init_game();
        break;
    case EV_ENCODER_CLICK:
        /* Encoder push can also trigger restart/start */
        init_game();
        break;
    default:
        break;
    }
}

/* Render screen */
static void snake_render(void)
{
    if (s_game_started && !s_game_over) {
        update_game_state();
    }

    lv_obj_clean(ui_frame_content());

    if (!s_game_started) {
        ui_text_row(SNAKE_ROW_TITLE, "     S N A K E");
        ui_text_row(SNAKE_ROW_SCORE, "Press SELECT to Start");
        ui_text_row(SNAKE_ROW_HIGH_SCORE, "Rotate knob to steer");
        char buf[32];
        snprintf(buf, sizeof(buf), "High Score: %u", (unsigned int)s_high_score);
        ui_text_row(SNAKE_ROW_PROMPT, buf);
    } else if (s_game_over) {
        ui_text_row(SNAKE_ROW_TITLE, "    GAME OVER");
        char score_buf[32];
        snprintf(score_buf, sizeof(score_buf), "Score: %u", (unsigned int)s_score);
        ui_text_row(SNAKE_ROW_SCORE, score_buf);
        char hs_buf[32];
        snprintf(hs_buf, sizeof(hs_buf), "High Score: %u", (unsigned int)s_high_score);
        ui_text_row(SNAKE_ROW_HIGH_SCORE, hs_buf);
        ui_text_row(SNAKE_ROW_PROMPT, "Press SELECT to Restart");
    } else {
        /* Draw the active game board */
        draw_food(s_food_x, s_food_y);
        for (int i = 0; i < s_snake_len; i++) {
            draw_block(s_snake_x[i], s_snake_y[i]);
        }
    }

    /* Force full-screen display (no hint bar) */
    ui_frame_set_hints(NULL);
}

/* Called when the app is exiting */
static void snake_exit(void)
{
    if (s_score > s_high_score) {
        s_high_score = s_score;
        app_store_set_u32(&s_store, "highscore", s_high_score);
    }
    app_store_close(&s_store);
}

static const device_app_t snake_app = {
    .name     = "Snake",
    .init     = snake_init,
    .on_event = snake_on_event,
    .render   = snake_render,
    .exit     = snake_exit,
    .available = NULL,                  /* Always show in the Launcher */
    .tick_ms  = SNAKE_GAME_TICK_MS,     /* Ask OS for periodic render ticks */
};

TASKMASTER_REGISTER_APP(snake_app);
