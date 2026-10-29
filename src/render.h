#ifndef DB_RENDER_H
#define DB_RENDER_H
#include <stdint.h>
#include <stdbool.h>
#include "assets.h"
#include "game.h"
extern const uint8_t *asset_banks[ASSET_BANKS];
void draw_game(void);
void draw_sprite(unsigned int index,int x,int y);
void project_world(fx x,fx y,fx z,int *sx,int *sy);
void render_reset_cache(void);
extern unsigned int render_fps;
#endif
