/* Actual runtime aperture implementation; synthetic data only. */
#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "mod_memory.h"
#include "../psxrecomp/runtime/src/mod_gpu_memory.c.inc"
int main(void) {
    int32_t x=0,y=0;
    assert(!psx_mod_gpu_vertex_set(0x80010000,0,0,0));
    uint32_t a=psx_mod_gpu_dma_memory_alloc(16,4);
    assert(a==0x80e00000);
    uint32_t word=120u<<16|160u;memcpy(mod_gpu_dma_memory,&word,4);
    assert(psx_mod_gpu_vertex_set(a,word,160*65536+123,120*65536+456));
    assert(psx_mod_gpu_vertex_get(a&0xffffff,word,&x,&y));
    assert(x==160*65536+123 && y==120*65536+456);
    assert(!psx_mod_gpu_vertex_get(a,word+1,&x,&y));
    assert(!psx_mod_gpu_vertex_get(a+1,word,&x,&y));
    assert(!psx_mod_gpu_vertex_set(a+16,word,0,0));
    for(unsigned width=1;width<=4;width*=2) {
        assert(psx_mod_gpu_vertex_set(a,word,1,2));
        mod_vertex_invalidate(0,width);
        assert(!psx_mod_gpu_vertex_get(a,word,&x,&y));
    }
    assert(!psx_mod_gpu_vertex_set(a,word,1024*65536,0));
    puts("PASS: real aperture allocation, aliases, value validation, write invalidation and bounds");
}
