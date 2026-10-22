#ifndef WXL_BILLBOARD_PROJECTION_H
#define WXL_BILLBOARD_PROJECTION_H
#include "mod_plugins.h"
#include <stdint.h>
/* Original 8007EF1C/8007F024 quarter-wave lookup; data stays on the disc. */
static int32_t wxl_billboard_sine(int32_t angle) {
    uint32_t a=(uint32_t)(angle<0?-(int64_t)angle:angle)&4095u;
    unsigned index=(a&1024u)?1023u-(a&1023u):(a&1023u);
    int32_t value=(int16_t)psx_mod_read_half(0x80091880u+index*2u);
    if(a&2048u)value=-value;
    return angle<0?-value:value;
}
static int32_t wxl_billboard_cosine(int32_t angle) {
    uint32_t a=(uint32_t)(angle<0?-(int64_t)angle:angle)&4095u;
    unsigned index=(a&1024u)?(a&1023u):1023u-(a&1023u);
    int32_t value=(int16_t)psx_mod_read_half(0x80091880u+index*2u);
    return a>=1024u && a<3072u?-value:value;
}
static int32_t wxl_billboard_shift12(int64_t n) {
    return (int32_t)(n/4096-(n<0 && n%4096));
}
/* Ghidra 80010AF8 / 80010D58: upper/lower anchored billboard quads. */
static int wxl_billboard_corners(uint32_t source,unsigned type,uint32_t center,
                                 uint16_t depth,int32_t roll,uint32_t out[4]) {
    int32_t width=(int16_t)psx_mod_read_half(source+6u);
    int32_t height=(int16_t)psx_mod_read_half(source+8u);
    if((type!=10 && type!=11) || depth<2048 || width<=0 || height<=0) return 0;
    int32_t half=(width*160/depth)>>1,h=height*160/depth;
    int32_t dx=wxl_billboard_shift12((int64_t)wxl_billboard_cosine(roll)*half);
    int32_t dy=wxl_billboard_shift12((int64_t)wxl_billboard_sine(roll)*half);
    int32_t hx=wxl_billboard_shift12((int64_t)wxl_billboard_cosine(1024-roll)*h);
    int32_t hy=wxl_billboard_shift12((int64_t)wxl_billboard_sine(1024-roll)*h);
    int32_t x[4],y[4],cx=(int16_t)center,cy=(int16_t)(center>>16);
    unsigned anchor=type==10?0u:2u,other=2u-anchor;
    x[anchor]=cx-dx;y[anchor]=cy-dy;x[anchor+1]=cx+dx;y[anchor+1]=cy+dy;
    int sign=type==10?-1:1;
    for(unsigned i=0;i<2;++i) {
        x[other+i]=x[anchor+i]+sign*hx;y[other+i]=y[anchor+i]-sign*hy;
    }
    for(unsigned i=0;i<4;++i) {
        if(x[i]<-1024 || x[i]>1023 || y[i]<-1024 || y[i]>1023)return 0;
        out[i]=(uint16_t)x[i]|((uint32_t)(uint16_t)y[i]<<16);
    }
    return 1;
}
#endif
