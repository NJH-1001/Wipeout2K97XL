/* Opt-in ship visibility extension. See docs/enemy-ship-distance-proposal.md.
 * Uses the game's renderer, transforms, mode/hidden gates and packet formats. */
#include "mod_plugins.h"
#include "cpu_state.h"
#include <stdint.h>
#define SHIP_POOL_BYTES 196608u
#define STOCK_POOL_BYTES 44032u
#define SHIP_STOCK_CUTOFF 0x2a0204b0u
#define SHIP_FAR_CUTOFF 0x2a020b54u
static uint32_t ship_stats,ship_arena;
/* The original +/-39 section gate rejects in-range ships on long straights.
 * Preserve the branch and delay slot; SLTIU v0,zero,79 always admits the section.
 * Distance, positive depth, race mode and hidden-state checks still apply. */
#define SHIP_PATCH_COUNT 3u
static const uint32_t ship_sites[SHIP_PATCH_COUNT]={0x80021d1cu,0x80021d28u,0x80021e68u};
static const uint32_t ship_original[SHIP_PATCH_COUNT]={SHIP_STOCK_CUTOFF,SHIP_STOCK_CUTOFF,0x2c42004fu};
static const uint32_t ship_extended[SHIP_PATCH_COUNT]={SHIP_FAR_CUTOFF,SHIP_FAR_CUTOFF,0x2c02004fu};
static const uint32_t stock_pools[2]={0x800a49c4u,0x800af5c4u};
static void ship_stat(unsigned n,uint32_t v) {
    if(ship_stats) psx_mod_write_word(ship_stats+n*4,v);
}
static int ship_ram(uint32_t a,uint32_t n) {
    uint32_t p=a&0x1fffffffu;
    return p>=0x10000u && p<=0x200000u && n<=0x200000u-p;
}
static int ship_cutoff(int enabled) {
    uint32_t words[SHIP_PATCH_COUNT];
    for(unsigned i=0;i<SHIP_PATCH_COUNT;++i) {
        words[i]=psx_mod_read_word(ship_sites[i]);
        if(words[i]!=ship_original[i] && words[i]!=ship_extended[i]) return 0;
    }
    for(unsigned i=0;i<SHIP_PATCH_COUNT;++i) {
        uint32_t wanted=enabled?ship_extended[i]:ship_original[i];
        if(words[i]!=wanted) psx_mod_write_code_word(ship_sites[i],wanted);
    }
    ship_stat(7,enabled?1:0);
    return 1;
}
static int ship_models(uint32_t table) {
    if(table!=0x800a45b4u) return 0;
    unsigned max_p=0,max_v=0;
    for(unsigned i=0;i<15;++i) {
        uint32_t m=psx_mod_read_word(table+i*4);
        if(!ship_ram(m,64)) return 0;
        unsigned nv=psx_mod_read_half(m+16),np=psx_mod_read_half(m+32);
        uint32_t vp=psx_mod_read_word(m+20);
        if(!nv || nv>128 || !np || np>128 || !ship_ram(vp,nv*8)) return 0;
        for(unsigned j=0;j<nv;++j) for(unsigned k=0;k<3;++k) {
            int v=(int16_t)psx_mod_read_half(vp+j*8+k*2);
            if(v < -512 || v>512) return 0;
        }
        static const unsigned record_bytes[9]={0,16,28,16,32,24,36,28,44};
        uint32_t record=psx_mod_read_word(m+36);
        for(unsigned j=0;j<np;++j) {
            if(!ship_ram(record,4)) return 0;
            unsigned type=psx_mod_read_half(record);
            if(type<1 || type>8 || !ship_ram(record,record_bytes[type])) return 0;
            unsigned corners=(type==1 || type==2 || type==5 || type==6)?3:4;
            for(unsigned k=0;k<corners;++k)
                if(psx_mod_read_half(record+4+k*2)>=nv) return 0;
            record+=record_bytes[type];
        }
        if(np>max_p) max_p=np;
        if(nv>max_v) max_v=nv;
    }
    ship_stat(10,max_p);ship_stat(11,max_v);
    return 1;
}
static void ship_frame_end(struct CPUState *cpu,uint32_t address) {
    (void)cpu;(void)address;
    if(!ship_arena || !ship_stats) return;
    unsigned b=psx_mod_read_half(0x80094c6cu);
    if(b>1) return;
    uint32_t start=ship_arena+b*SHIP_POOL_BYTES;
    uint32_t cur=psx_mod_read_word(0x80094a48u);
    if(cur>=start && cur-start<=SHIP_POOL_BYTES) {
        uint32_t used=cur-start;
        ship_stat(14,used);
        if(used>psx_mod_read_word(ship_stats+32)) ship_stat(8,used);
    }
}
static void ship_begin(struct CPUState *cpu,uint32_t address) {
    (void)address;
    if(!ship_stats) return;
    ship_stat(2,psx_mod_read_word(ship_stats+8)+1);
    if(!ship_arena) {ship_stat(3,4);return;}
    /* Validate originals before touching either cutoff or pool pointer. */
    for(unsigned i=0;i<SHIP_PATCH_COUNT;++i) {
        uint32_t w=psx_mod_read_word(ship_sites[i]);
        if(w!=ship_original[i] && w!=ship_extended[i]) {ship_stat(3,1);return;}
    }
    if(psx_mod_read_word(ship_stats+24)!=1) {
        ship_cutoff(0);ship_stat(3,7);return;
    }
    if(!cpu || !ship_models(cpu->gpr[5])) {
        ship_cutoff(0);ship_stat(3,2);return;
    }
    unsigned b=psx_mod_read_half(0x80094c6cu);
    uint32_t bases[2]={psx_mod_read_word(0x80094a1cu),psx_mod_read_word(0x80094a20u)};
    uint32_t cur=psx_mod_read_word(0x80094a48u);
    int stock=bases[0]==stock_pools[0] && bases[1]==stock_pools[1];
    int expanded=bases[0]==ship_arena && bases[1]==ship_arena+SHIP_POOL_BYTES;
    if(b>1 || (!stock && !expanded) || cur<bases[b] ||
       cur-bases[b]>(stock?STOCK_POOL_BYTES:SHIP_POOL_BYTES)) {
        ship_cutoff(0);ship_stat(3,3);return;
    }
    if(stock) {
        /* Earlier packets retain their original RAM addresses in the OT.
         * Reserve their prefix; later packets use the larger pool. The game's
         * own frame submit/reset selects the other expanded pool next frame. */
        uint32_t used=cur-bases[b];
        psx_mod_write_word(0x80094a1cu,ship_arena);
        psx_mod_write_word(0x80094a20u,ship_arena+SHIP_POOL_BYTES);
        psx_mod_write_word(0x80094a48u,ship_arena+b*SHIP_POOL_BYTES+used);
        ship_stat(9,psx_mod_read_word(ship_stats+36)+1);
    }
    ship_cutoff(1);ship_stat(3,0);
}
static void activate_ship_distance(void) {
    ship_stats=psx_mod_alloc_guest_memory(64,16);
    ship_arena=psx_mod_alloc_gpu_dma_memory(2*SHIP_POOL_BYTES,16);
    ship_stat(0,0x534c5857u);ship_stat(1,2);ship_stat(4,ship_arena);
    ship_stat(5,SHIP_POOL_BYTES);ship_stat(6,1);
    psx_mod_register_function_entry_plugin("wxl.ship-distance",0x80021b90u,ship_begin);
    psx_mod_register_function_entry_plugin("wxl.ship-distance",0x800684c4u,ship_frame_end);
}
PSX_MOD_CONSTRUCTOR(register_wxl_ship_distance) {
    psx_mod_register_activation_plugin("wxl.ship-distance",activate_ship_distance);
}

/* Far packets participate in the road/scenery collector's shared sort. */
int wxl_ship_distance_active(void) {
    return ship_stats && ship_arena && psx_mod_read_word(ship_stats+24)==1 &&
           psx_mod_read_word(ship_stats+12)==0;
}
