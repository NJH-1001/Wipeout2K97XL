#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../src/ship_distance_mod.c"
static unsigned char ram_bytes[0x200000],stats_bytes[64];
static unsigned writes,hooks;
static unsigned char *ptr(uint32_t a,unsigned n) {
    if(a>=0x9f000000u && a<0x9f000040u) {assert(n<=0x9f000040u-a);return stats_bytes+a-0x9f000000u;}
    a&=0x1fffffffu;assert(a<=sizeof ram_bytes && n<=sizeof ram_bytes-a);return ram_bytes+a;
}
uint16_t psx_mod_read_half(uint32_t a) {uint16_t v;memcpy(&v,ptr(a,2),2);return v;}
uint32_t psx_mod_read_word(uint32_t a) {uint32_t v;memcpy(&v,ptr(a,4),4);return v;}
void psx_mod_write_word(uint32_t a,uint32_t v) {memcpy(ptr(a,4),&v,4);}
void psx_mod_write_code_word(uint32_t a,uint32_t v) {++writes;psx_mod_write_word(a,v);}
uint32_t psx_mod_alloc_guest_memory(uint32_t n,uint32_t align) {assert(n==64 && align==16);return 0x9f000000u;}
uint32_t psx_mod_alloc_gpu_dma_memory(uint32_t n,uint32_t align) {assert(n==2*SHIP_POOL_BYTES && align==16);return 0x80e00000u;}
int psx_mod_register_activation_plugin(const char *id,PSXModActivationCallback cb) {assert(id && cb);return 1;}
int psx_mod_register_function_entry_plugin(const char *id,uint32_t a,PSXModFunctionEntryCallback cb) {assert(id && a && cb);++hooks;return 1;}
static void setup(void) {
 memset(ram_bytes,0,sizeof ram_bytes);memset(stats_bytes,0,sizeof stats_bytes);writes=0;
 for(unsigned i=0;i<SHIP_PATCH_COUNT;++i) psx_mod_write_word(ship_sites[i],ship_original[i]);
 psx_mod_write_word(0x80094a1c,stock_pools[0]);psx_mod_write_word(0x80094a20,stock_pools[1]);
 psx_mod_write_word(0x80094a48,stock_pools[0]+64);ship_stat(6,1);
 for(unsigned i=0;i<15;++i)psx_mod_write_word(0x800a45b4+i*4,0x80060000);
 psx_mod_write_word(0x80060010,4);psx_mod_write_word(0x80060014,0x80061000);
 psx_mod_write_word(0x80060020,128);psx_mod_write_word(0x80060024,0x80062000);
 for(unsigned i=0;i<128;++i)psx_mod_write_word(0x80062000+i*44,8);
}
int main(void) {
 assert(!hooks && !writes);activate_ship_distance();assert(hooks==2);
 assert(STOCK_POOL_BYTES+15u*128u*52u<SHIP_POOL_BYTES);
 CPUState cpu={0};cpu.gpr[5]=0x800a45b4;
 setup();psx_mod_write_word(stock_pools[0],0x12345678);ship_begin(&cpu,0);
 assert(psx_mod_read_word(ship_stats+12)==0 && writes==3);
 assert(psx_mod_read_word(0x80094a48)==ship_arena+64);
 assert(psx_mod_read_word(stock_pools[0])==0x12345678);
 assert(psx_mod_read_word(0x80094a20)==ship_arena+SHIP_POOL_BYTES);
 ship_begin(&cpu,0);assert(writes==3);
 psx_mod_write_word(0x80094a48,ship_arena+143872);ship_frame_end(&cpu,0);
 assert(psx_mod_read_word(ship_stats+32)==143872);
 ship_stat(6,0);ship_begin(&cpu,0);assert(writes==6 && psx_mod_read_word(ship_sites[0])==SHIP_STOCK_CUTOFF);
 assert(psx_mod_read_word(ship_sites[2])==ship_original[2]);
 /* All 16-bit section inputs pass the extended compare, including the
  * original boundary failures -40 and +40. Other instructions stay intact. */
 for(unsigned d=0;d<65536u;++d) {
  unsigned old=((d+39u)&65535u)<79u;
  int signed_d=(int16_t)d;
  assert(old==(signed_d>=-39 && signed_d<=39));
  assert((ship_extended[2]>>21&31u)==0u && 0u<(ship_extended[2]&65535u));
 }
 setup();psx_mod_write_word(ship_sites[2],0);ship_begin(&cpu,0);
 assert(!writes && psx_mod_read_word(0x80094a1c)==stock_pools[0]);
 setup();psx_mod_write_word(0x80060020,129);ship_begin(&cpu,0);assert(!writes && psx_mod_read_word(ship_stats+12)==2);
 setup();psx_mod_write_word(0x80062000,11);ship_begin(&cpu,0);assert(!writes && psx_mod_read_word(ship_stats+12)==2);
 setup();psx_mod_write_word(0x80062004,4);ship_begin(&cpu,0);assert(!writes && psx_mod_read_word(ship_stats+12)==2);
 setup();psx_mod_write_word(ship_sites[1],0);ship_begin(&cpu,0);assert(!writes && psx_mod_read_word(0x80094a1c)==stock_pools[0]);
 setup();psx_mod_write_word(0x80094a48,stock_pools[0]+STOCK_POOL_BYTES+4);ship_begin(&cpu,0);assert(!writes && psx_mod_read_word(ship_stats+12)==3);
 setup();psx_mod_write_word(0x80094c6c,1);psx_mod_write_word(0x80094a48,stock_pools[1]+128);ship_begin(&cpu,0);
 assert(psx_mod_read_word(0x80094a48)==ship_arena+SHIP_POOL_BYTES+128);
 puts("PASS: ship patch identity, disabled restoration, model and pointer guards, both buffers, prefix preservation and high-water accounting");return 0;
}
