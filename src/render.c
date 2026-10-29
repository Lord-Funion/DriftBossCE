#include "game.h"
#include "render.h"
#include <graphx.h>
#include <stdio.h>
#include <string.h>

const uint8_t *asset_banks[ASSET_BANKS];
static const uint8_t light=2,dark=3,blue=4,cyan=5,gold=6;
static char text_buffer[48];
unsigned int render_fps;
/* Lazy native-RLET cache: keep hot art in RAM, away from slow flash reads.
 * It is cleared as a unit only when new artwork fills the 48 KiB arena.
 * Old DBAS files remain compatible. No heap allocations are needed. */
static uint8_t sprite_cache[49152];
static const gfx_rletsprite_t *cached_sprites[SPRITE_COUNT];
static unsigned int cache_used;
void render_reset_cache(void){memset(cached_sprites,0,sizeof(cached_sprites));cache_used=0;}
static const gfx_rletsprite_t *native_sprite(unsigned int index){
    const SpriteRef *s=&sprite_refs[index];const uint8_t *source=asset_banks[s->bank]+s->offset,*p=source+2;
    uint8_t row,height=source[1],width=source[0];unsigned int old_size;uint8_t *out,*begin;
    if(cached_sprites[index])return cached_sprites[index];
    for(row=0;row<height;row++){while(*p!=255){p+=2+p[1];}p++;}
    old_size=(unsigned int)(p-source);
    if(cache_used+old_size>sizeof(sprite_cache))render_reset_cache();
    begin=out=sprite_cache+cache_used;*out++=width;*out++=height;p=source+2;
    for(row=0;row<height;row++){
        uint8_t previous=0,start;
        while((start=*p++)!=255){uint8_t length=*p++;*out++=start-previous;*out++=length;
            memcpy(out,p,length);out+=length;p+=length;previous=start+length;}
        if(previous<width)*out++=width-previous;
    }
    cached_sprites[index]=(const gfx_rletsprite_t*)begin;cache_used+=(unsigned int)(out-begin);
    return cached_sprites[index];
}

/* Sparse scanline sprites, read straight from archived AppVars. Every copy
 * is clipped before writing VRAM; zero-length or off-screen runs are skipped.
 */
void draw_sprite(unsigned int index,int x,int y){
    const SpriteRef *s;const uint8_t *p;const gfx_rletsprite_t *native;
    if(index>=SPRITE_COUNT)return;s=&sprite_refs[index];
    if(!asset_banks[s->bank])return;
    p=asset_banks[s->bank]+s->offset;x+=s->ox;y+=s->oy;
    if(x>319||y>239||x+(int)p[0]<0||y+(int)p[1]<0)return;
    native=native_sprite(index);
    if(x>=0&&y>=0&&x+native->width<=320&&y+native->height<=240)gfx_RLETSprite_NoClip(native,x,(uint8_t)y);
    else gfx_RLETSprite(native,x,y);
}
void project_world(fx x,fx y,fx z,int *sx,int *sy){
    int dx=x-game.cam_x,dy=y-game.cam_y,dz=z-game.cam_z,a,b;
    /* Explicit shifts keep -Oz from choosing expensive division helpers.
     * Preserve truncation toward zero, including for negative coordinates. */
    dx=dx<0?-((-dx)>>4):dx>>4;
    dy=dy<0?-((-dy)>>4):dy>>4;
    dz=dz<0?-((-dz)>>4):dz>>4;
    /* 2.6 pixels/world-unit, original camera direction (60.685,71.106,70.502).
     * Orthographic projection replaces the browser's perspective renderer. */
    /* Culling bounds the sums below the signed 24-bit limit. */
    a=dx*-505+dz*434;b=dx*262+dz*306-dy*530;
    *sx=160+(a<0?-((-a)>>12):a>>12);
    *sy=142+(b<0?-((-b)>>12):b>>12);
}
static void text(const char *s,int x,int y,uint8_t color,uint8_t scale){
    gfx_SetTextScale(scale,scale);gfx_SetTextFGColor(dark);gfx_PrintStringXY(s,x+1,y+1);
    gfx_SetTextFGColor(color);gfx_PrintStringXY(s,x,y);
}
static void centered(const char *s,int y,uint8_t color,uint8_t scale){
    gfx_SetTextScale(scale,scale);text(s,(320-(int)gfx_GetStringWidth(s))/2,y,color,scale);
}
static void panel(int x,int y,int w,int h){
    gfx_SetColor(dark);gfx_FillRectangle(x,y,w,h);
    gfx_SetColor(cyan);gfx_Rectangle(x,y,w,h);gfx_SetColor(blue);gfx_Rectangle(x+1,y+1,w-2,h-2);
}
static void button(int x,int y,int w,const char *label,bool selected){
    gfx_SetColor(dark);gfx_FillRectangle(x+2,y+3,w,21);gfx_SetColor(selected?cyan:blue);gfx_FillRectangle(x,y,w,21);
    gfx_SetColor(light);gfx_HorizLine(x+2,y+2,w-4);gfx_SetTextScale(1,1);
    text(label,x+(w-(int)gfx_GetStringWidth(label))/2,y+7,light,1);
}
static void background(void){
    /* Fast stack-fill replaces 240 gradient-row checks and many rectangle
     * fills. Keep the original orange color; spend the frame budget on play. */
    gfx_FillScreen(background_rows[120]);
}
static void wallet(void){
    draw_sprite(SPR_COIN,14,15);snprintf(text_buffer,sizeof(text_buffer),"%lu",(unsigned long)game.save.coins);
    text(text_buffer,25,11,light,1);
}
static void menu_scene(void){
    int n,sx,sy;fx offset=game.frame%150*TILE_SIZE/150;
    game.cam_x=game.cam_y=game.cam_z=0;
    for(n=-5;n<6;n++){
        project_world(0,-19,(fx)n*TILE_SIZE+offset,&sx,&sy);
        draw_sprite(SPR_ROAD,sx,sy);
    }
    draw_sprite(SPR_CAR+game.menu_car*9,160,135);
}
static void draw_menu(void){
    uint8_t car=game.menu_car;
    menu_scene();draw_sprite(SPR_TITLE,160,48);wallet();
    text("<",92,123,light,2);text(">",214,123,light,2);
    centered(car_names[car],148,light,1);
    if(game.save.owned&(1u<<car)){
        centered(car==game.save.car?"SELECTED":"[ALPHA] SELECT CAR",162,gold,1);
    }else{
        snprintf(text_buffer,sizeof(text_buffer),"[ALPHA] BUY: %u COINS",car_prices[car]);centered(text_buffer,162,gold,1);
    }
    draw_sprite(SPR_BUTTON_PLAY,160,192);centered("[ENTER] PLAY",216,light,1);
    centered("ARROWS: CARS   [MODE] HELP",231,light,1);
    gfx_SetTextScale(1,1);text("BEST",248,10,light,1);snprintf(text_buffer,sizeof(text_buffer),"%lu",(unsigned long)game.save.best);text(text_buffer,248,21,gold,1);
}
static void draw_boosters(void){
    static const char *const labels[3]={"DOUBLE SCORE","CAR INSURANCE","COIN RUSH"};
    static const uint8_t prices[3]={25,50,75};uint8_t i;char small[20];
    menu_scene();wallet();panel(17,37,286,155);centered("SELECT BOOSTERS",49,light,1);
    for(i=0;i<3;i++){
        int x=25+i*92;bool on=(game.boosts&(1u<<i))!=0;
        gfx_SetColor(i==game.booster_cursor?gold:blue);gfx_Rectangle(x,71,86,83);
        gfx_SetColor(on?cyan:blue);gfx_FillRectangle(x+5,76,76,37);
        draw_sprite(SPR_BUTTON_BOOSTER_1+i,x+43,94);
        if(on)text("*",x+67,80,gold,1);
        snprintf(small,sizeof(small),"%s",i==0?"DOUBLE":i==1?"INSURANCE":"COIN RUSH");
        text(small,x+7,118,light,1);
        if(on)text("ACTIVE",x+17,137,gold,1);
        else if(game.save.boost_stock[i]){snprintf(small,sizeof(small),"FREE: %u",game.save.boost_stock[i]);text(small,x+12,137,gold,1);}
        else{snprintf(small,sizeof(small),"%u COINS",prices[i]);text(small,x+10,137,gold,1);}
    }
    (void)labels;centered("[ALPHA] TOGGLE / BUY",167,light,1);
    button(85,202,150,"[ENTER] START DRIFT",true);
}
static void draw_road(void){
    uint8_t order[MAX_TILES],count=0,i,j;int sx,sy,screen_x[MAX_TILES],screen_y[MAX_TILES];
    for(i=0;i<MAX_TILES;i++)if(game.tiles[i].active){
        Tile *t=&game.tiles[i];
        if(t->x-game.cam_x>FX(180)||t->x-game.cam_x<-FX(180)||t->z-game.cam_z>FX(180)||t->z-game.cam_z<-FX(180))continue;
        project_world(t->x,t->y,t->z,&sx,&sy);
        if(sx<-110||sx>430||sy<-90||sy>330)continue;
        screen_x[i]=sx;screen_y[i]=sy;
        j=count;
        while(j&&game.tiles[order[j-1]].x+game.tiles[order[j-1]].z>t->x+t->z){order[j]=order[j-1];j--;}
        order[j]=i;count++;
    }
    for(i=0;i<count;i++){
        uint8_t index=order[i];Tile *t=&game.tiles[index];
        draw_sprite(SPR_ROAD+t->skin*9+t->kind,screen_x[index],screen_y[index]);
    }
}
static void draw_trails(void){
    uint8_t i;int sx,sy,px,py,screen[MAX_TRAILS][2];
    for(i=0;i<MAX_TRAILS;i+=2){
        Trail *t=&game.trails[i];screen[i][0]=-1000;screen[i][1]=-1000;
        if(t->active&&t->x-game.cam_x<FX(180)&&game.cam_x-t->x<FX(180)&&t->z-game.cam_z<FX(180)&&game.cam_z-t->z<FX(180))project_world(t->x,t->y,t->z,&screen[i][0],&screen[i][1]);
    }
    gfx_SetColor(1);
    for(i=0;i<MAX_TRAILS;i+=2){
        uint8_t prev=(i+MAX_TRAILS-2)&(MAX_TRAILS-1);
        Trail *t=&game.trails[i],*p=&game.trails[prev];
        if(!t->active||!p->active||i==(game.trail_index&~1u))continue;
        if(t->x-p->x>FX(8)||p->x-t->x>FX(8)||t->z-p->z>FX(8)||p->z-t->z>FX(8))continue;
        sx=screen[i][0];sy=screen[i][1];px=screen[prev][0];py=screen[prev][1];
        if(sx<0||sx>319||sy<0||sy>239||px<0||px>319||py<0||py>239)continue;
        gfx_Line(sx-3,sy,px-3,py);gfx_Line(sx+3,sy,px+3,py);
    }
}
static void draw_play(void){
    uint8_t i;int sx,sy;
    bool hidden=game.y<game.cam_y-FX(4);
    if(hidden){project_world(game.shadow_x,game.y+102,game.shadow_z,&sx,&sy);draw_sprite(SPR_CAR+game.car*9+(game.angle+16)/32,sx,sy);}
    draw_road();draw_trails();
    if(!hidden){
        project_world(game.shadow_x,game.cam_y+FX(1.25),game.shadow_z,&sx,&sy);
        gfx_SetColor(10);gfx_FillEllipse(sx,sy+2,11,4);
        project_world(game.shadow_x,game.y+102,game.shadow_z,&sx,&sy);
        draw_sprite(SPR_CAR+game.car*9+(game.angle+16)/32,sx,sy);
    }
    for(i=0;i<MAX_COINS;i++)if(game.coins[i].active){
        Coin *c=&game.coins[i];project_world(c->x,c->y,c->z,&sx,&sy);
        draw_sprite(SPR_COIN_3D+(game.frame/5)%8,sx,sy);
    }
    wallet();snprintf(text_buffer,sizeof(text_buffer),"%lu",(unsigned long)game_score());centered(text_buffer,10,light,2);
    draw_sprite(SPR_BUTTON_PAUSE,302,15);
    if(game.score_ticks==0)centered("HOLD [2ND] TO DRIFT RIGHT",218,light,1);
    if(game.boosts){
        text(game.boosts&1?"2X":"",9,224,gold,1);text(game.boosts&2?"+1":"",36,224,gold,1);text(game.boosts&4?"COIN RUSH":"",63,224,gold,1);
    }
    if(render_fps){snprintf(text_buffer,sizeof(text_buffer),"%u FPS",render_fps);text(text_buffer,269,225,light,1);}
}
static void draw_over(void){
    draw_play();panel(45,52,230,151);draw_sprite(SPR_GAME_OVER,160,76);
    snprintf(text_buffer,sizeof(text_buffer),"SCORE %lu",(unsigned long)game_score());centered(text_buffer,101,light,2);
    snprintf(text_buffer,sizeof(text_buffer),"BEST %lu   COINS +%u",(unsigned long)game.save.best,game.run_coins);centered(text_buffer,127,gold,1);
    button(76,147,168,"[ENTER] TRY AGAIN",true);centered("[CLEAR] GARAGE",181,light,1);
}
static void draw_help(void){
    panel(13,14,294,210);centered("DRIFT BOSS",26,light,2);
    text("HOLD 2ND / ENTER: DRIFT RIGHT",25,59,light,1);
    text("RELEASE: DRIFT LEFT",25,74,light,1);
    text("TURN EARLY - THE CAR SLIDES!",25,94,gold,1);
    text("DODGE EDGES, RAMPS AND GAPS.",25,110,light,1);
    text("COLLECT COINS TO UNLOCK CARS.",25,126,light,1);
    text("ARROWS: CHOOSE CAR / BOOSTER",25,149,light,1);
    text("ALPHA: SELECT / BUY / TOGGLE",25,164,light,1);
    text("MODE: PAUSE   CLEAR: BACK",25,179,light,1);
    centered("[ENTER] BACK",204,gold,1);
}
void draw_game(void){
    background();
    switch(game.mode){
    case MODE_MENU:draw_menu();break;
    case MODE_BOOSTERS:draw_boosters();break;
    case MODE_PLAY:draw_play();break;
    case MODE_OVER:draw_over();break;
    case MODE_PAUSE:
        draw_play();panel(66,68,188,107);centered("PAUSED",83,light,2);
        centered("[ENTER] RESUME",113,light,1);centered("[MODE] HELP",132,light,1);centered("[CLEAR] GARAGE",151,gold,1);break;
    case MODE_HELP:draw_help();break;
    }
    if(game.toast_time){panel(18,190,284,24);centered(game.toast,198,gold,1);}
}
