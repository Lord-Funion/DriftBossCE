#include "game.h"
#include "input-trace.h"
#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#undef assert
#define assert(expr) do { if (!(expr)) { fprintf(stderr, "FAILED: %s (%s:%d)\n", #expr, __FILE__, __LINE__); exit(1); } } while (0)
static void test_rules(void){
    int i;fx sx,sz;SaveData s;
    game_init(9);assert(game_validate_save(&(SaveData){0})==false);
    s=game.save;s.checksum=game_save_checksum(&s);assert(game_validate_save(&s));s.coins++;assert(!game_validate_save(&s));
    game.save.coins=1000;game.save.reserved=2;game_tick(KEY_SHOP);assert(game.save.car==2&&game.save.coins==750&&(game.save.owned&4));
    game_tick(0);game_tick(KEY_ACTION);assert(game.mode==MODE_BOOSTERS);
    game_tick(0);game_tick(KEY_SHOP);assert(game.boosts==1&&game.save.boost_stock[0]==0);
    game_tick(0);game_tick(KEY_BACK);assert(game.mode==MODE_MENU&&game.save.boost_stock[0]==1);
    game_start(0);game_tick(KEY_PAUSE);assert(game.mode==MODE_PAUSE);game_tick(0);game_tick(KEY_PAUSE);assert(game.mode==MODE_HELP);
    game_tick(0);game_tick(KEY_ACTION);assert(game.mode==MODE_PAUSE);game_tick(0);game_tick(KEY_ACTION);assert(game.mode==MODE_PLAY);
    game_start(0);for(i=0;i<1200&&game.mode==MODE_PLAY;i++)game_tick(0);assert(game.mode==MODE_OVER);assert(game_score()>0&&game.save.best==game_score());
    game_start(2);for(i=0;i<500&&game.mode==MODE_PLAY;i++)game_tick(0);assert(!(game.boosts&2));
    game_init(1);game_spawn(0,0,0,FLAT,0);assert(game_floor(0,0,&sx,&sz)==301);assert(game_floor(FX(13),0,&sx,&sz)<-FX(1000));
    game_init(1);game_spawn(0,0,0,HALF_L,0);assert(game_floor(FX(5),0,&sx,&sz)>-FX(1000));assert(game_floor(FX(7),0,&sx,&sz)<-FX(1000));
    puts("Core rule tests passed: save validation, garage, boosters, pause, falling, insurance, road bounds.");
}

int main(void){
 test_rules();game_init(551);game_start(0);
 for(unsigned i=0;i<sizeof(inputs);i++)game_tick(inputs[i]);
 const int32_t expected[]={-204627,-6496,-251190,-7659,-9344,0,-203272,-251184,-203272,-1209,-251184,1624,29,271,4,7,107,642863516,-1795527};
 const int32_t actual[]={game.x,game.y,game.z,game.vx,game.vy,game.vz,game.shadow_x,game.shadow_z,game.cam_x,game.cam_y,game.cam_z,(int32_t)game.score_ticks,(int32_t)game.save.coins,(int32_t)game.save.best,game.mode,game.difficulty,game.next_serial,(int32_t)game.rng,(int32_t)game.coin_rng};
 for(unsigned i=0;i<19;i++){if(actual[i]!=expected[i]){fprintf(stderr,"Replay field %u: got %ld, expected %ld\n",i,(long)actual[i],(long)expected[i]);return 1;}}
 printf("PASS: %zu-tick replay, all 19 checked gameplay fields.\n",sizeof(inputs));return 0;
}
