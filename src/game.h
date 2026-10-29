#ifndef DB_GAME_H
#define DB_GAME_H
#include <stdint.h>
#include <stdbool.h>

/* World positions and velocities use signed Q8 fixed point. */
#ifdef __TICE__
/* eZ80 ADL registers are 24 bits: avoid software 32-bit arithmetic in the
 * frame loop. World rebasing keeps coordinates well inside this range. */
typedef int fx;
#else
typedef int32_t fx;
#endif
#define FX(n) ((fx)((n) * 256))
#define TILE_SIZE 6387 /* original 24.95 world units */
#define MAX_TILES 128
#define MAX_COINS 96
#define MAX_TRAILS 16
enum { FLAT, BUMP_L, BUMP_R, VALLEY_L, VALLEY_R, HALF_L, HALF_R, RAMP_L, RAMP_R };
enum { MODE_MENU, MODE_BOOSTERS, MODE_PLAY, MODE_PAUSE, MODE_OVER, MODE_HELP };
enum { KEY_ACTION=1, KEY_LEFT=2, KEY_RIGHT=4, KEY_UP=8, KEY_DOWN=16,
       KEY_BACK=32, KEY_PAUSE=64, KEY_SHOP=128, KEY_HELP=256 };

typedef struct { fx x,y,z; uint16_t serial; uint8_t kind,skin,active,has_coin; } Tile;
typedef struct { fx x,y,z; uint8_t active; } Coin;
typedef struct { fx x,y,z; uint8_t angle,active; } Trail;
typedef struct {
    uint32_t magic,best,coins,days;
    uint8_t owned,car,boost_stock[3],reserved;
    uint32_t checksum;
} SaveData;
typedef struct {
    Tile tiles[MAX_TILES]; Coin coins[MAX_COINS]; Trail trails[MAX_TRAILS];
    fx x,y,z,vx,vy,vz,shadow_x,shadow_z,cam_x,cam_y,cam_z;
    fx latest_x,latest_y,latest_z,safe_x,safe_y,safe_z;
    uint32_t rng,coin_rng,score_ticks,frame;
    uint16_t next_serial,previous_serial,keys,old_keys,mode_time;
    uint8_t mode,car,menu_car,booster_cursor,boosts,grounded,steer,angle;
    uint8_t block_right,skin,difficulty,diff_count,skin_count,coin_count,coin_every;
    uint8_t physics_acc,trail_index,run_coins,toast_time,help_return;
    char toast[36];
    SaveData save;
} Game;

extern Game game;
extern const uint16_t car_prices[8];
extern const char *const car_names[8];
void game_init(uint32_t seed);
void game_tick(uint16_t keys);
void game_start(uint8_t boosters);
void game_generate_pattern(uint8_t pattern);
void game_spawn(fx x,fx y,fx z,uint8_t kind,uint8_t skin);
fx game_floor(fx x,fx z,fx *slope_x,fx *slope_z);
uint32_t game_score(void);
uint32_t game_save_checksum(const SaveData *save);
bool game_validate_save(const SaveData *save);
void game_set_toast(const char *text);
#endif
