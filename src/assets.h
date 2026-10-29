/* Generated from the original Math Playground Drift Boss assets. */
#ifndef DB_ASSETS_H
#define DB_ASSETS_H
#include <stdint.h>
typedef struct { uint16_t offset; uint8_t bank; int8_t ox, oy; } SpriteRef;
#define ASSET_BANKS 3
#define SPRITE_COUNT 133
#define SPR_CAR 0
#define SPR_ROAD 72
#define SPR_COIN_3D 117
#define SPR_TITLE 125
#define SPR_BUTTON_PLAY 126
#define SPR_GAME_OVER 127
#define SPR_BUTTON_PAUSE 128
#define SPR_COIN 129
#define SPR_BUTTON_BOOSTER_1 130
#define SPR_BUTTON_BOOSTER_2 131
#define SPR_BUTTON_BOOSTER_3 132
extern const SpriteRef sprite_refs[SPRITE_COUNT];
extern const uint16_t asset_bank_sizes[ASSET_BANKS];
extern const uint16_t game_palette[256];
extern const uint8_t background_rows[240];
#endif
