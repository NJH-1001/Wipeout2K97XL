#include <assert.h>
#include <stdio.h>
#include "gpu_clear_background.h"
int main(void) {
 GpuClearBackground b={0};uint16_t c=0;
 gpu_clear_background_note(&b,80,100,5<<10);
 assert(!gpu_clear_background_get(&b,0,240,&c));
 gpu_clear_background_note(&b,0,240,5<<10);
 assert(gpu_clear_background_get(&b,0,240,&c) && c==(5<<10));
 gpu_clear_background_note(&b,80,100,5<<10);
 assert(gpu_clear_background_get(&b,0,240,&c));
 assert(!gpu_clear_background_get(&b,256,240,&c));
 gpu_clear_background_note(&b,256,240,0);
 assert(gpu_clear_background_get(&b,256,240,&c) && c==0);
 gpu_clear_background_note(&b,80,1,0);
 assert(!gpu_clear_background_get(&b,0,240,&c));
 assert(!gpu_clear_background_get(&b,0,0,&c));
 puts("PASS: full clear, partial updates, buffer separation, unknown and mixed backgrounds");
}
