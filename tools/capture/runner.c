#include "cemu.h"
#include "asic.h"
#include "cpu.h"
#include "mem.h"
#include "control.h"
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
void gui_console_clear(void){}
void gui_console_printf(const char *format,...){(void)format;}
void gui_console_err_printf(const char *format,...){va_list ap;va_start(ap,format);vfprintf(stderr,format,ap);va_end(ap);}
asic_rev_t gui_handle_reset(const boot_ver_t *b,asic_rev_t a,asic_rev_t d,emu_device_t e,bool *p){(void)b;(void)a;(void)d;(void)e;*p=false;return ASIC_REV_M;}
static uint16_t u16(const uint8_t*p){return p[0]|p[1]<<8;}
static uint32_t u32(const uint8_t*p){return p[0]|p[1]<<8|p[2]<<16|(uint32_t)p[3]<<24;}
static uint64_t load_elf(const char *path,uint32_t entry){
    FILE *f=fopen(path,"rb");if(!f){perror(path);exit(1);}fseek(f,0,SEEK_END);long size=ftell(f);rewind(f);uint8_t *b=malloc(size);fread(b,1,size,f);fclose(f);
    uint32_t shoff=u32(b+32);uint16_t entsz=u16(b+46),count=u16(b+48);
    for(int i=0;i<count;i++){
        uint8_t*s=b+shoff+i*entsz;uint32_t type=u32(s+4),flags=u32(s+8),addr=u32(s+12),off=u32(s+16),len=u32(s+20);
        if((flags&2)&&len){void *p=phys_mem_ptr(addr,len);if(!p){fprintf(stderr,"Bad section %x %u\n",addr,len);exit(2);}if(type==8)memset(p,0,len);else memcpy(p,b+off,len);}
    }free(b);
    cpu.registers.SPL=0xd3f000;cpu.IEF1=cpu.IEF2=false;cpu.ADL=cpu.L=cpu.IL=true;
    cpu_flush(entry,true);return cpu.cycles;
}
int main(int argc,char**argv){
    if(argc<5){fprintf(stderr,"runner program.obj entry-hex patches.bin output-directory\n");return 1;}
    asic_init();mem.flash.size=SIZE_FLASH_MIN;mem.flash.block=calloc(1,mem.flash.size);asic_reset();set_cpu_clock(48000000);emu_set_run_rate(48000000);
    control.privileged=0xffffff;control.protectedStart=0;control.protectedEnd=0;control.stackLimit=0;
    uint32_t entry=(uint32_t)strtoul(argv[2],NULL,16);load_elf(argv[1],entry);
    if(argc>3){FILE *patch=fopen(argv[3],"rb");if(!patch){perror(argv[3]);return 5;}uint32_t count;fread(&count,4,1,patch);
        while(count--){uint32_t address,len;fread(&address,4,1,patch);fread(&len,4,1,patch);void*p=phys_mem_ptr(address,len);if(!p){fprintf(stderr,"Bad patch %x size %u\n",address,len);return 6;}fread(p,1,len,patch);}fclose(patch);}
    uint8_t phase=0;uint64_t last=cpu.cycles;uint64_t total=0;uint32_t iter=0;
    uint32_t histogram[8192]={0};unsigned frame=0;char path[512];snprintf(path,sizeof(path),"%s/frame-times.csv",argv[4]);FILE*times=fopen(path,"w");if(!times)return 8;
    while(iter++<20000000){
        emu_run(1000);total=sched_total_cycles();
        if(phase==2&&cpu.registers.PC>=0xd10000&&cpu.registers.PC<0xd30000)histogram[(cpu.registers.PC-0xd10000)>>4]++;
        uint8_t next=mem_peek_byte(0xd3ff00);
        if(next!=phase&&next>=2&&next<255){char path[512];snprintf(path,sizeof(path),"%s/frame-%04u.raw",argv[4],frame);FILE*f=fopen(path,"wb");if(!f)return 7;fwrite(mem.ram.block+0x40000,1,76800,f);fclose(f);fprintf(times,"%u,%llu\n",frame,(unsigned long long)total);frame++;mem.ram.block[0x3ff00]=0;}
        if(next!=phase){printf("phase %u cycles %llu delta %llu milliseconds %.3f pc %06X\n",next,(unsigned long long)total,(unsigned long long)(total-last),(total-last)/48000.,cpu.registers.PC);fflush(stdout);last=total;phase=next;}
        if(phase==255)break;
        if((cpu.registers.PC<0xd00000||cpu.registers.PC>=0xe00000)&&!(cpu.registers.PC>=0xe30800&&cpu.registers.PC<0xe30c00)&&cpu.registers.PC!=0x15c&&!(cpu.registers.PC<mem.flash.size&&mem.flash.block[cpu.registers.PC]==0xc9)){fprintf(stderr,"Unexpected PC %06X at cycle %u SP %06X return %06X\n",cpu.registers.PC,cpu.cycles,cpu.registers.SPL,mem_peek_long(cpu.registers.SPL));
            return 3;}
    }
    printf("finished phase %u cycles %llu; result %u tick %u pc %06X sp %06X\n",phase,(unsigned long long)total,mem_peek_byte(0xd3ff01),mem_peek_byte(0xd3ff02),cpu.registers.PC,cpu.registers.SPL);
    snprintf(path,sizeof(path),"%s/final-state.bin",argv[4]);FILE*dump=fopen(path,"wb");if(!dump)return 8;fwrite(mem.ram.block+0x3fe00,1,76,dump);fclose(dump);
    fclose(times);printf("Captured %u frames\n",frame);asic_free();return phase==255?0:4;
}
