#include "game.h"
#include "render.h"
#include "inputs.h"
#include <graphx.h>
static uint8_t frame_marker=2;
static void capture(void){draw_game();*(volatile uint8_t*)0xd3ff00=frame_marker++;if(frame_marker==255)frame_marker=2;while(*(volatile uint8_t*)0xd3ff00){};}
int main(void){
 asset_banks[0]=(void*)0x100000;asset_banks[1]=(void*)0x110000;asset_banks[2]=(void*)0x120000;
 gfx_Begin();gfx_SetPalette(game_palette,sizeof(game_palette),0);gfx_SetTextTransparentColor(0);gfx_SetTextBGColor(0);gfx_SetTextConfig(gfx_text_noclip);render_reset_cache();
 game_init(551);game.frame=70;capture();
 for(volatile unsigned c=1;c<8;c++){game_tick(KEY_RIGHT);game_tick(0);capture();}
 game_init(551);game_tick(KEY_ACTION);game_tick(0);capture();
 game_init(551);game_tick(KEY_HELP);capture();
 game_init(551);game_start(0);
 for(volatile unsigned i=0;i<sizeof(inputs);i++){game_tick(inputs[i]);if(i%4==0)capture();}
 for(volatile unsigned i=0;i<50;i++)game_tick(0);capture();
 volatile int32_t*out=(void*)0xd3fe00;
    out[0]=game.x;out[1]=game.y;out[2]=game.z;out[3]=game.vx;out[4]=game.vy;out[5]=game.vz;
    out[6]=game.shadow_x;out[7]=game.shadow_z;out[8]=game.cam_x;out[9]=game.cam_y;out[10]=game.cam_z;
    out[11]=game.score_ticks;out[12]=game.save.coins;out[13]=game.save.best;out[14]=game.mode;out[15]=game.difficulty;out[16]=game.next_serial;
    out[17]=game.rng;out[18]=game.coin_rng;
 *(volatile uint8_t*)0xd3ff00=255;for(;;){}
}
