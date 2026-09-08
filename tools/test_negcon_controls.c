#include "../src/negcon_controls.h"
#include <assert.h>
#include <stdio.h>
int main(void) {
 WxlControls c={0,2,2,0,1,1,1};
 PSXModNegconInput in={0}; PSXModNegconOutput out;
 in.buttons=0xffff; in.deadzone=3277;
 wxl_map_controls(&c,&in,&out); assert(out.buttons==0xffff && out.twist==128 && out.i==0);
 in.axis[0]=32767; in.axis[1]=-32767;
 wxl_map_controls(&c,&in,&out); assert(out.twist==255 && out.buttons==0xffff);
 in.axis[2]=-32767; in.axis[3]=-32767; in.axis[5]=16384;
 wxl_map_controls(&c,&in,&out); assert(!(out.buttons&0x0800) && (out.buttons&0x0400) && !(out.buttons&0x10)); assert(out.i==128 && out.l==0);
 in.axis[2]=32767; wxl_map_controls(&c,&in,&out); assert(!(out.buttons&0x0400) && out.l==255);
 for(int steer=0;steer<3;steer++) for(int pitch=0;pitch<3;pitch++)
 for(int brakes=0;brakes<3;brakes++) for(int shoulders=0;shoulders<3;shoulders++)
 for(int throttle=0;throttle<3;throttle++) for(int secondary=0;secondary<3;secondary++) {
  c=(WxlControls){steer,pitch,brakes,shoulders,throttle,secondary,1};
  in=(PSXModNegconInput){0}; in.buttons=0xffff; in.deadzone=3277;
  wxl_map_controls(&c,&in,&out); assert(out.buttons==0xffff && out.twist==128 && !out.i && !out.ii && !out.l);
  in.buttons=0xfbff; wxl_map_controls(&c,&in,&out);
  assert(shoulders==0 ? out.l==255 : out.l==0);
  if(shoulders==1) assert(!(out.buttons&0x0800));
 }
 c=(WxlControls){2,0,0,2,0,0,0}; in.buttons=0x3bff; in.axis[5]=32767;
 wxl_map_controls(&c,&in,&out); assert(out.twist==128 && out.i==0 && out.ii==255 && out.l==0);
 puts("PASS: approved preset, 729 neutral/shoulder combinations, disabled inputs");
}
