/* Ship primitive encoder, derived from 0x80011DE8. No game data included.
 * Callers supply checked projected positions and original guest record reads. */
#ifndef WXL_SHIP_PACKET_H
#define WXL_SHIP_PACKET_H
#include <stdint.h>
#include <string.h>
#include "mod_plugins.h"
static unsigned wxl_ship_packet(uint32_t src,unsigned type,unsigned flags,
                                const uint32_t xy[4],uint32_t ambient,uint32_t out[13]) {
    if(type<1 || type>8) return 0;
    unsigned n=(type==1 || type==2 || type==5 || type==6)?3u:4u;
    unsigned texture=(type==2 || type==4 || type==6 || type==8);
    unsigned gouraud=type>=5;
    unsigned op=texture?(gouraud?(n==3?0x34:0x3c):(n==3?0x24:0x2c)):
                          (gouraud?(n==3?0x30:0x38):(n==3?0x20:0x28));
    unsigned stride=1u+texture+gouraud;
    unsigned color=texture?(n==3?24u:28u):12u;
    unsigned length=1u+n*(1u+texture)+(n-1u)*gouraud;
    memset(out,0,13u*sizeof *out);
    if(flags&4u)op|=2u;
    out[0]=length<<24;
    out[1]=(op<<24)|((texture && !gouraud)?ambient&0xffffffu:psx_mod_read_word(src+color)&0xffffffu);
    for(unsigned j=0;j<n;++j) {
        unsigned pos=2u+j*stride;
        out[pos]=xy[j];
        if(gouraud && j)out[pos-1]=psx_mod_read_word(src+color+4u*j)&0xffffffu;
        if(texture)out[pos+1]=psx_mod_read_half(src+(n==3?16u:18u)+2u*j);
    }
    if(texture) {
        out[3]|=(uint32_t)psx_mod_read_half(src+(n==3?12u:14u))<<16;
        out[3u+stride]|=(uint32_t)psx_mod_read_half(src+(n==3?14u:16u))<<16;
    }
    return length+1u;
}
#endif
