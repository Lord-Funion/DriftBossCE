/*
 * Native CE adaptation of the Math Playground Drift Boss browser build.
 * Track patterns, geometry, timing, prices and handling constants are traced
 * from that build. The fixed-point contact model replaces Babylon/Cannon.
 */
#include "game.h"
#include <string.h>
#include <stdlib.h>
#include <limits.h>

Game game;
const uint16_t car_prices[8]={0,200,250,300,350,400,450,500};
const char *const car_names[8]={"STANDARD","PICKUP","TAXI","POLICE","BOX TRUCK","ICE CREAM","AMBULANCE","FIRE TRUCK"};
static const uint8_t car_grip[8]={35,38,41,44,47,50,52,55};
static const uint8_t boost_prices[3]={25,50,75};
static const Tile *near_tiles[MAX_TILES];
static uint8_t near_count,tile_bound;
static bool use_near_tiles;
/* sin and cos of yaw 0..90 degrees, Q8. */
static const uint16_t sine[17]={0,25,50,74,98,121,142,162,181,198,213,226,237,245,251,255,256};
static fx absfx(fx x){return x<0?-x:x;}
static uint32_t next_random(uint32_t *state){uint32_t x=*state;x^=x<<13;x^=x>>17;x^=x<<5;return *state=x;}
static uint8_t rnd(uint8_t lo,uint8_t hi){return lo+(uint8_t)(next_random(&game.rng)%(hi-lo+1));}
static uint8_t crnd(uint8_t lo,uint8_t hi){return lo+(uint8_t)(next_random(&game.coin_rng)%(hi-lo+1));}
static Tile *serial_tile(uint16_t serial){uint8_t i;for(i=0;i<MAX_TILES;i++)if(game.tiles[i].active&&game.tiles[i].serial==serial)return &game.tiles[i];return NULL;}
static void spawn_coin(fx x,fx y,fx z){uint8_t i;for(i=0;i<MAX_COINS;i++)if(!game.coins[i].active){game.coins[i]=(Coin){x,y,z,1};return;}}
static void coins_on_tile(Tile *t,Tile *before,fx nx,fx nz){
    fx dx=0,dz=0,h=FX(4),h1=h,h2=h;
    if(t->x==before->x&&nx==t->x)dz=FX(5);
    else if(t->z==before->z&&nz==t->z)dx=FX(5);
    else return;
    switch(t->kind){case RAMP_L:case RAMP_R:h=1920;h1=FX(5);h2=FX(10);break;
    case BUMP_L:case BUMP_R:h=FX(7);h1=h2=FX(5);break;
    case VALLEY_L:case VALLEY_R:h=0;h1=h2=FX(3);break;default:break;}
    spawn_coin(t->x,t->y+h,t->z);
    spawn_coin(t->x+dx,t->y+h1,t->z+dz);
    spawn_coin(t->x-dx,t->y+h2,t->z-dz);
}
void game_spawn(fx x,fx y,fx z,uint8_t kind,uint8_t skin){
    uint8_t i,slot=0;uint16_t oldest=UINT16_MAX;
    Tile *prev=serial_tile(game.next_serial-1),*before=serial_tile(game.previous_serial);
    if(prev&&before&&prev->has_coin)coins_on_tile(prev,before,x,z);
    game.previous_serial=game.next_serial-1;
    for(i=0;i<MAX_TILES;i++){
        if(!game.tiles[i].active){slot=i;break;}
        if(game.tiles[i].serial<oldest){oldest=game.tiles[i].serial;slot=i;}
    }
    game.latest_x=x;game.latest_y=y-19;game.latest_z=z;
    game.tiles[slot]=(Tile){x,y-19,z,game.next_serial++,kind,skin,1,0};
    if(slot>=tile_bound)tile_bound=slot+1;
    game.coin_count++;
    if(game.coin_count==game.coin_every){
        game.coin_every=crnd(4,7);game.tiles[slot].has_coin=1;game.coin_count=0;
        if(crnd(0,1)){game.coin_count=game.coin_every-1;
            if(kind==RAMP_L||kind==RAMP_R)game.coin_count=game.coin_every>=5?game.coin_every-5:0;}
    }
    if(game.boosts&4)game.tiles[slot].has_coin=1;
}
static void line(uint8_t count,uint8_t type){
    uint8_t i;for(i=0;i<count;i++)game_spawn(game.latest_x-(game.block_right?TILE_SIZE:0),game.latest_y,
        game.latest_z-(game.block_right?0:TILE_SIZE),type,game.skin);
}
static void half_line(uint8_t count){uint8_t i;for(i=0;i<count;i++)line(1,i==count-1?FLAT:(game.block_right?HALF_R:HALF_L));}
static void zigzag(bool half){
    if(half)half_line(rnd(2,3));else line(rnd(1,2),FLAT);
    game.block_right^=1;
    if(half)half_line(rnd(2,3));else line(rnd(1,1),FLAT);
    game.block_right^=1;
    if(half)half_line(rnd(2,3));else line(rnd(1,2),FLAT);
    if(rnd(0,1)){game.block_right^=1;if(half)half_line(rnd(2,3));else line(rnd(1,1),FLAT);}
}
static void split_line(bool ramps){
    fx x=game.latest_x-(game.block_right?TILE_SIZE:0),z=game.latest_z-(game.block_right?0:TILE_SIZE),y=game.latest_y;
    uint8_t n,choice=0,wide=ramps?2:1;
    game_spawn(x,y,z,FLAT,game.skin);
    if(ramps)game_spawn(x-(game.block_right?0:TILE_SIZE),y,z-(game.block_right?TILE_SIZE:0),FLAT,game.skin);
    game_spawn(x-(game.block_right?0:wide*TILE_SIZE),y,z-(game.block_right?wide*TILE_SIZE:0),FLAT,game.skin);
    for(n=0;n<5;n++){
        if(game.block_right)x-=TILE_SIZE;else z-=TILE_SIZE;
        if(ramps&&n==1)choice=rnd(1,2);
        game_spawn(x,y,z,ramps?(n==1&&choice==1?(game.block_right?RAMP_R:RAMP_L):FLAT):(game.block_right?HALF_R:HALF_L),game.skin);
        game_spawn(x-(game.block_right?0:wide*TILE_SIZE),y,z-(game.block_right?wide*TILE_SIZE:0),
            ramps?(n==1&&choice==2?(game.block_right?RAMP_R:RAMP_L):FLAT):(game.block_right?HALF_R:HALF_L),game.skin);
        if(n==4){
            game_spawn(x-(ramps&&!game.block_right?TILE_SIZE:0),y,z-(ramps&&game.block_right?TILE_SIZE:0),FLAT,game.skin);
            game_spawn(x-(game.block_right?0:wide*TILE_SIZE),y,z-(game.block_right?wide*TILE_SIZE:0),FLAT,game.skin);
            game_spawn(x-(game.block_right?TILE_SIZE:wide*TILE_SIZE),y,z-(game.block_right?wide*TILE_SIZE:TILE_SIZE),FLAT,game.skin);
        }
    }
}
void game_generate_pattern(uint8_t pattern){
    uint8_t i,n=5,type;
    switch(pattern){
    case 0:line(rnd(2,3),FLAT);break;
    case 1:zigzag(false);break;
    case 4:half_line(rnd(3,5));break;
    case 6:split_line(true);break;
    case 8:split_line(false);break;
    case 9:zigzag(true);break;
    default:
        if(pattern==7)n=7;
        for(i=0;i<n;i++){
            type=FLAT;
            if((pattern==2&&i==2)||(pattern==5&&i==2))type=game.block_right?BUMP_R:BUMP_L;
            if((pattern==3&&i==2)||(pattern==5&&i==1))type=game.block_right?VALLEY_R:VALLEY_L;
            if(pattern==7&&i==1)type=game.block_right?RAMP_R:RAMP_L;
            line(1,type);
        }break;
    }
}
static void next_pattern(void){
    uint8_t lo=0,hi=game.difficulty,pattern;
    if(hi>9){lo=hi-9;hi=9;if(lo>3)lo=3;}
    pattern=rnd(lo,hi);game_generate_pattern(pattern);game.block_right^=1;
    if(++game.diff_count>2){game.diff_count=0;if(game.difficulty<240)game.difficulty++;}
    if(++game.skin_count>7){game.skin_count=0;game.skin=(game.skin+1)%5;}
}

/* Original collision-box tops, Q8: axis, center, projected half extent,
 * surface height at center, slope. Base planes remain under raised parts.
 */
typedef struct {int center,half,height,slope;uint8_t axis;} Surface;
static const Surface surfaces[9][3]={
    {{0,3200,320,0,0},{0,0,0,0,0},{0,0,0,0,0}},
    {{0,3200,320,0,0},{1280,1510,728,-102,1},{-1280,1510,728,102,1}},
    {{0,3200,320,0,0},{1280,1510,728,-102,0},{-1280,1510,728,102,0}},
    {{0,3200,-576,0,0},{2048,1510,-219,102,1},{-1920,1510,-117,-102,1}},
    {{0,3200,-576,0,0},{2048,1510,-219,102,0},{-1920,1510,-117,-102,0}},
    {{0,1600,320,0,0},{0,0,0,0,0},{0,0,0,0,0}},
    {{0,1600,320,0,1},{0,0,0,0,0},{0,0,0,0,0}},
    {{0,3200,320,0,0},{1280,2550,717,-74,1},{0,3110,759,-157,1}},
    {{0,3200,320,0,0},{1280,2550,717,-74,0},{0,3110,759,-157,0}}
};
fx game_floor(fx x,fx z,fx *sx,fx *sz){
    uint8_t i,j,count,limit=use_near_tiles?near_count:tile_bound;fx best=-FX(10000);*sx=*sz=0;
    for(i=0;i<limit;i++){
        const Tile *t=use_near_tiles?near_tiles[i]:&game.tiles[i];fx dx,dz;
        if(!t->active)continue;
        dx=x-t->x;dz=z-t->z;
        if(!t->active||absfx(dx)>FX(14)||absfx(dz)>FX(14))continue;
        if(t->kind==FLAT){
            if(absfx(dx)<=3200&&absfx(dz)<=3200&&t->y+320>best){best=t->y+320;*sx=*sz=0;}
            continue;
        }
        count=(t->kind==FLAT||t->kind==HALF_L||t->kind==HALF_R)?1:3;
        for(j=0;j<count;j++){
            const Surface *s=&surfaces[t->kind][j];fx u=(s->axis?dz:dx)-s->center,v=s->axis?dx:dz,h;
            if(absfx(u)>s->half||absfx(v)>FX(12.5))continue;
            h=t->y+s->height+(u*s->slope)/256;
            if(h>best){best=h;*sx=s->axis?0:s->slope;*sz=s->axis?s->slope:0;}
        }
    }return best;
}
uint32_t game_score(void){return (game.score_ticks+3)/6;}
void game_set_toast(const char *text){strncpy(game.toast,text,sizeof(game.toast)-1);game.toast[sizeof(game.toast)-1]=0;game.toast_time=120;}
uint32_t game_save_checksum(const SaveData *s){
    return s->magic^s->best^s->coins^s->days^((uint32_t)s->owned<<24)^((uint32_t)s->car<<16)^((uint32_t)s->boost_stock[0]<<8)^s->boost_stock[1]^((uint32_t)s->boost_stock[2]<<4)^0x7AB16C39u;
}
bool game_validate_save(const SaveData *s){return s->magic==0x31434244u&&s->car<8&&(s->owned&1)&&
    (s->owned&(1u<<s->car))&&s->boost_stock[0]<100&&s->boost_stock[1]<100&&s->boost_stock[2]<100&&
    s->coins<10000000u&&s->best<1000000u&&s->checksum==game_save_checksum(s);}
void game_init(uint32_t seed){
    near_count=tile_bound=0;use_near_tiles=false;
    memset(&game,0,sizeof(game));game.rng=seed?seed:1;game.coin_rng=game.rng^0x8AD9B117u;
    game.save.magic=0x31434244u;game.save.owned=1;game.save.boost_stock[0]=game.save.boost_stock[1]=game.save.boost_stock[2]=1;
    game.coin_every=5;game.previous_serial=UINT16_MAX;game.mode=MODE_MENU;
}
void game_start(uint8_t boosters){
    int i;game.car=game.save.car;game.boosts=boosters;game.score_ticks=0;game.run_coins=0;
    near_count=tile_bound=0;use_near_tiles=false;
    game.x=game.vx=game.vy=game.shadow_x=game.cam_x=0;
    game.y=FX(3);game.z=game.shadow_z=FX(50);game.vz=-FX(30);game.angle=game.steer=0;
    game.cam_y=game.cam_z=0;game.grounded=1;game.difficulty=game.diff_count=game.skin_count=game.skin=0;
    game.next_serial=0;game.previous_serial=UINT16_MAX;game.latest_x=game.latest_y=game.latest_z=0;
    game.coin_count=0;game.coin_every=5;game.physics_acc=0;game.trail_index=0;game.mode_time=0;
    memset(game.tiles,0,sizeof(game.tiles));memset(game.coins,0,sizeof(game.coins));memset(game.trails,0,sizeof(game.trails));
    for(i=-3;i<5;i++)game_spawn(0,0,-i*TILE_SIZE,FLAT,0);
    game.block_right=1;next_pattern();next_pattern();game.mode=MODE_PLAY;
    game.safe_x=0;game.safe_z=0;game.safe_y=FX(3);
}
static void revive(void){
    uint8_t i;fx best=FX(10000);Tile *safe=NULL;
    for(i=0;i<MAX_TILES;i++)if(game.tiles[i].active&&game.tiles[i].kind==FLAT){
        fx dx=absfx(game.tiles[i].x-game.x),dz=absfx(game.tiles[i].z-game.z),d=dx+dz;
        if(d<best){best=d;safe=&game.tiles[i];}
    }
    if(safe){game.x=game.shadow_x=safe->x;game.z=game.shadow_z=safe->z;game.y=safe->y+FX(3);}
    else {game.x=game.shadow_x=game.safe_x;game.z=game.shadow_z=game.safe_z;game.y=game.safe_y;}
    game.vx=game.vy=0;game.vz=-FX(30);game.steer=game.angle=0;game.grounded=1;
    game.boosts&=~2;game_set_toast("CAR INSURANCE USED");
}
static fx toward_zero(fx value,fx amount){if(value<0)return value+amount>0?0:value+amount;return value-amount<0?0:value-amount;}
static void physics_step(void){
    fx sx,sz,ground,h,oldx=game.x,oldz=game.z;
    int angle=game.angle>>4,side;fx sn=sine[angle],cs=sine[16-angle];
    game.x+=game.vx/60;game.z+=game.vz/60;game.vy-=128;game.y+=game.vy/60;
    ground=game_floor(game.x,game.z,&sx,&sz);
    /* Two rear wheel sensors, as in the published car-skeleton module. */
    for(side=-1;side<=1;side+=2){
        fx px=game.x+(side*410*cs+563*sn)/256,pz=game.z+(-side*410*sn+563*cs)/256,tx,tz;
        h=game_floor(px,pz,&tx,&tz);
        if(h>ground&&h<game.y+FX(3)){ground=h;sx=tx;sz=tz;}
    }
    game.grounded=0;
    if(ground>-FX(1000)&&game.y-FX(1)<=ground&&game.vy<FX(35)){
        fx delta=ground+128-game.y;
        if(delta<FX(5)){
            fx rising=(game.vx*sx+game.vz*sz)/256;
            game.y=ground+128;game.vy=rising;game.grounded=1;
            if(rising<0)game.vy=0;
            /* The original Cannon build applies four contact friction impulses.
             * Its 0.035 * 0.1 grip pair yields 0.42 velocity loss per step. */
            h=(fx)car_grip[game.car]*3072/1000;
            game.vx=toward_zero(game.vx,h);game.vz=toward_zero(game.vz,h);
            if(!sx&&!sz){game.safe_x=game.x;game.safe_z=game.z;game.safe_y=game.y+FX(2);}
        }
    }
    /* Cannon's default linear damping is 0.01 per second. */
    game.vx-=game.vx/6000;game.vz-=game.vz/6000;
    (void)oldx;(void)oldz;
}
static void rebase(void){
    fx dx=0,dz=0;uint8_t i;
    if(game.x<-FX(2000))dx=game.x;
    if(game.z<-FX(2000))dz=game.z;
    if(!dx&&!dz)return;
    game.x-=dx;game.shadow_x-=dx;game.cam_x-=dx;game.latest_x-=dx;game.safe_x-=dx;
    game.z-=dz;game.shadow_z-=dz;game.cam_z-=dz;game.latest_z-=dz;game.safe_z-=dz;
    for(i=0;i<MAX_TILES;i++)if(game.tiles[i].active){game.tiles[i].x-=dx;game.tiles[i].z-=dz;}
    for(i=0;i<MAX_COINS;i++)if(game.coins[i].active){game.coins[i].x-=dx;game.coins[i].z-=dz;}
    for(i=0;i<MAX_TRAILS;i++)if(game.trails[i].active){game.trails[i].x-=dx;game.trails[i].z-=dz;}
}
static void play_tick(bool action){
    uint8_t i,factor=28+game.difficulty;
    /* Real time scoring: original 10 points/second, starting past start line. */
    if(game.z<0||game.score_ticks){game.score_ticks+=(game.boosts&1)?2:1;game.steer=action?1:0;}
    if(game.y>game.cam_y-FX(10)){if(game.steer)game.vx=-FX(30);else game.vz=-FX(30);}
    /* A tick moves at most one unit; rear sensors extend less than 4 units.
     * A 20-unit center bound safely includes every possible contact (tile
     * half extent <=14). Build once, instead of rescanning 128 tiles for
     * every wheel and physics substep. */
    near_count=0;
    for(i=0;i<tile_bound;i++){
        const Tile *t=&game.tiles[i];
        if(t->active&&absfx(t->x-game.x)<=FX(20)&&absfx(t->z-game.z)<=FX(20))near_tiles[near_count++]=t;
    }
    use_near_tiles=true;
    if(game.difficulty>12)factor=40;game.physics_acc+=factor;
    while(game.physics_acc>=20){game.physics_acc-=20;physics_step();}
    use_near_tiles=false;
    game.shadow_x+=(game.x-game.shadow_x)/7;game.shadow_z+=(game.z-game.shadow_z)/7;
    if(game.steer){int a=game.angle+(256-game.angle+6)/7;if(a>255)a=255;game.angle=(uint8_t)a;}
    else game.angle-=game.angle/7+(game.angle>0&&game.angle<7);
    game.cam_x=game.shadow_x;game.cam_z=game.shadow_z>0?0:game.shadow_z;
    if(game.grounded)game.cam_y+=(game.y-448-game.cam_y)/8;
    for(i=0;i<MAX_TILES;i++)if(game.tiles[i].active){
        if(game.x-game.tiles[i].x<-FX(150)||game.z-game.tiles[i].z<-FX(150))game.tiles[i].active=0;
    }
    for(i=0;i<MAX_COINS;i++)if(game.coins[i].active){
        Coin *c=&game.coins[i];fx dx=(game.shadow_x-c->x)/16,dy=(game.y-c->y)/16,dz=(game.shadow_z-c->z)/16;
        if(absfx(dx)<80&&absfx(dy)<80&&absfx(dz)<80&&dx*dx+dy*dy+dz*dz<6400){c->active=0;game.save.coins++;if(game.run_coins<255)game.run_coins++;}
        if(game.x-c->x<-FX(100)||game.z-c->z<-FX(100))c->active=0;
    }
    if(game.grounded){
        fx dx=(game.latest_x-game.x)/256,dz=(game.latest_z-game.z)/256;
        if(absfx(dx)<250&&absfx(dz)<250&&dx*dx+dz*dz<62500)next_pattern();
        if(game.frame%3==0){Trail *t=&game.trails[game.trail_index];*t=(Trail){game.shadow_x,game.y-128,game.shadow_z,game.angle,1};game.trail_index=(game.trail_index+1)%MAX_TRAILS;}
    }
    if(game.y<game.cam_y-FX(20)){
        if(game.boosts&2)revive();
        else {game.mode=MODE_OVER;game.mode_time=0;if(game_score()>game.save.best)game.save.best=game_score();}
    }
    rebase();
}
void game_tick(uint16_t keys){
    uint16_t pressed=keys&~game.old_keys;uint8_t car;
    game.keys=keys;game.frame++;game.mode_time++;if(game.toast_time)game.toast_time--;
    switch(game.mode){
    case MODE_MENU:
        game.menu_car=game.save.car;
        if(pressed&(KEY_LEFT|KEY_RIGHT)){
            car=(game.save.reserved+(pressed&KEY_LEFT?7:1))%8;game.save.reserved=car;game.menu_car=car;
        }else game.menu_car=game.save.reserved%8;
        if(pressed&KEY_SHOP){
            car=game.menu_car;
            if(game.save.owned&(1u<<car)){game.save.car=car;game_set_toast("CAR SELECTED");}
            else if(game.save.coins>=car_prices[car]){game.save.coins-=car_prices[car];game.save.owned|=1u<<car;game.save.car=car;game_set_toast("CAR UNLOCKED");}
            else game_set_toast("COLLECT MORE COINS TO UNLOCK");
        }
        if(pressed&KEY_ACTION){if(game.save.owned&(1u<<game.menu_car))game.save.car=game.menu_car;game.mode=MODE_BOOSTERS;game.boosts=0;game.mode_time=0;}
        if(pressed&(KEY_HELP|KEY_PAUSE)){game.help_return=MODE_MENU;game.mode=MODE_HELP;}
        break;
    case MODE_BOOSTERS:
        if(pressed&KEY_LEFT)game.booster_cursor=(game.booster_cursor+2)%3;
        if(pressed&KEY_RIGHT)game.booster_cursor=(game.booster_cursor+1)%3;
        if(pressed&KEY_SHOP){
            uint8_t b=game.booster_cursor,bit=1u<<b;
            if(game.boosts&bit){game.boosts&=~bit;game.save.boost_stock[b]++;}
            else if(game.save.boost_stock[b]){game.save.boost_stock[b]--;game.boosts|=bit;}
            else if(game.save.coins>=boost_prices[b]){game.save.coins-=boost_prices[b];game.boosts|=bit;}
            else game_set_toast("NOT ENOUGH COINS");
        }
        if(pressed&KEY_ACTION)game_start(game.boosts);
        if(pressed&KEY_BACK){for(car=0;car<3;car++)if(game.boosts&(1u<<car))game.save.boost_stock[car]++;game.boosts=0;game.mode=MODE_MENU;}
        break;
    case MODE_PLAY:
        if(pressed&(KEY_PAUSE|KEY_BACK)){game.mode=MODE_PAUSE;game.mode_time=0;}
        else play_tick((keys&KEY_ACTION)!=0);
        break;
    case MODE_PAUSE:
        if(pressed&KEY_ACTION)game.mode=MODE_PLAY;
        if(pressed&KEY_BACK){if(game_score()>game.save.best)game.save.best=game_score();game.mode=MODE_MENU;game.save.reserved=game.save.car;}
        if(pressed&(KEY_HELP|KEY_PAUSE)){game.help_return=MODE_PAUSE;game.mode=MODE_HELP;}
        break;
    case MODE_OVER:
        if(game.mode_time>30&&(pressed&KEY_ACTION)){game.mode=MODE_BOOSTERS;game.boosts=0;game.mode_time=0;}
        if(pressed&KEY_BACK){game.mode=MODE_MENU;game.save.reserved=game.save.car;}
        break;
    case MODE_HELP:
        if(pressed&(KEY_ACTION|KEY_BACK|KEY_HELP|KEY_PAUSE))game.mode=game.help_return;
        break;
    }
    game.old_keys=keys;
}
