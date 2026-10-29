#include <tice.h>
#include <graphx.h>
#include <keypadc.h>
#include <fileioc.h>
#include <sys/timers.h>
#include <sys/rtc.h>
#include <string.h>
#include <stdlib.h>
#include "game.h"
#include "render.h"

static uint8_t bank_handles[ASSET_BANKS];
static bool load_assets(void){
    char name[]="DBAS0";uint8_t i;
    for(i=0;i<ASSET_BANKS;i++){
        name[4]='0'+i;bank_handles[i]=ti_Open(name,"r");
        if(!bank_handles[i]||ti_GetSize(bank_handles[i])!=asset_bank_sizes[i])return false;
        asset_banks[i]=ti_GetDataPtr(bank_handles[i]);
        if(memcmp(asset_banks[i],"DBCEAS1\0",8))return false;
    }return true;
}
static void close_assets(void){uint8_t i;for(i=0;i<ASSET_BANKS;i++)if(bank_handles[i])ti_Close(bank_handles[i]);}
static void load_save(void){
    uint8_t handle=ti_Open("DBSAVE","r");SaveData data;
    if(handle){if(ti_GetSize(handle)==sizeof(data)&&ti_Read(&data,sizeof(data),1,handle)==1&&game_validate_save(&data))game.save=data;ti_Close(handle);}
    game.save.reserved=game.save.car;game.menu_car=game.save.car;
}
static void save_game(void){
    uint8_t handle;game.save.checksum=game_save_checksum(&game.save);
    handle=ti_Open("DBSAVE","w");if(handle){ti_Write(&game.save,sizeof(game.save),1,handle);ti_SetArchiveStatus(true,handle);ti_Close(handle);}
}
static uint16_t read_keys(void){
    uint16_t keys=0;kb_Scan();
    if(kb_Data[1]&kb_2nd||kb_Data[6]&kb_Enter)keys|=KEY_ACTION;
    if(kb_Data[7]&kb_Left)keys|=KEY_LEFT;
    if(kb_Data[7]&kb_Right)keys|=KEY_RIGHT;
    if(kb_Data[7]&kb_Up)keys|=KEY_UP;
    if(kb_Data[7]&kb_Down)keys|=KEY_DOWN;
    if(kb_Data[6]&kb_Clear)keys|=KEY_BACK;
    if(kb_Data[1]&kb_Mode)keys|=KEY_PAUSE;
    if(kb_Data[2]&kb_Alpha)keys|=KEY_SHOP;
    return keys;
}
int main(void){
    uint32_t previous,now,elapsed,accumulator=0,fps_start;uint16_t keys;uint8_t steps,fps_frames=0;
    game_init((uint32_t)rtc_Time()^0xD81F7B05u);load_save();
    if(!load_assets()){
        close_assets();os_ClrHome();os_PutStrFull("Drift Boss: missing data");os_NewLine();
        os_PutStrFull("Transfer DBAS0, DBAS1,");os_NewLine();os_PutStrFull("and DBAS2 with the game.");os_NewLine();
        os_PutStrFull("Press any key.");os_GetKey();return 1;
    }
    gfx_Begin();gfx_SetPalette(game_palette,sizeof(game_palette),0);gfx_SetDrawBuffer();
    gfx_SetTextTransparentColor(0);gfx_SetTextBGColor(0);gfx_SetTextConfig(gfx_text_noclip);
    timer_Disable(1);timer_Set(1,0);timer_Enable(1,TIMER_32K,TIMER_NOINT,TIMER_UP);
    previous=fps_start=timer_Get(1);render_reset_cache();
    for(;;){
        keys=read_keys();
        if(game.mode==MODE_MENU&&(keys&KEY_BACK)&&!(game.old_keys&KEY_BACK))break;
        now=timer_Get(1);elapsed=now-previous;previous=now;
        /* Bound catch-up work so a long frame cannot cause a CPU backlog. */
        if(elapsed>1638)elapsed=1638;accumulator+=elapsed*60;steps=0;
        while(accumulator>=32768&&steps<3){game_tick(keys);accumulator-=32768;steps++;}
        draw_game();gfx_SwapDraw();
        fps_frames++;
        if(timer_Get(1)-fps_start>=16384){uint32_t span=timer_Get(1)-fps_start;render_fps=((uint32_t)fps_frames*32768+span/2)/span;fps_frames=0;fps_start=timer_Get(1);}
        /* Target 30 rendered frames/second. Input is scanned once per frame;
         * hammering the hardware scan routine in this wait wastes CPU time. */
        while(timer_Get(1)-now<1092){}
    }
    timer_Disable(1);gfx_End();close_assets();save_game();os_ClrHome();return 0;
}
