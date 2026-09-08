/* Experimental, opt-in road extension. Evidence: docs/draw-distance-proposal.md.
 * Guest code and near-road rendering remain authoritative and unchanged.
 */
#include "mod_plugins.h"
#include "cpu_state.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define MAX_VERTICES 32768u
#define MAX_FACES 8192u
#define PACKET_BYTES 40u
#define FAR_Z 64000
#define STATS_MAGIC 0x444C5857u

typedef struct { int32_t x, y, z; int valid, input_fits; } Projected;
typedef struct { uint32_t face; int32_t z; } FarFace;
typedef struct { int16_t r[9]; int32_t t[3], offset[3]; } Camera;
static Projected projected[MAX_VERTICES];
static FarFace selected[MAX_FACES];
static Camera camera;
static uint32_t stats, arena, camera_address, pending, camera_error;

/* TCP-readable stats: magic/version/frame-count/status, current/total packets,
 * DMA arena/camera, enable (write 0 or 1 for diagnostic A/B), capacity,
 * rejected projection/near-range/viewport counts, track index, OT, buffer.
 * Status: 0 OK, 1 invalid world, 2 unsupported camera, 3 display,
 * 4 allocation/OT, 5 invalid vertex index, 6 conflicting cameras, 7 disabled.
 */
static void stat(unsigned index, uint32_t value) {
    if (stats) psx_mod_write_word(stats + index * 4u, value);
}
static int ram(uint32_t address, uint32_t bytes) {
    uint32_t p = address & 0x1fffffffu;
    return p >= 0x10000u && p <= 0x200000u && bytes <= 0x200000u-p;
}
static int64_t floor_div(int64_t n, int64_t d) {
    int64_t q = n / d;
    return q - (n % d < 0);
}
static int fits16(int64_t n) { return n >= -32768 && n <= 32767; }
static Projected project(const int32_t vertex[3], const Camera *c) {
    Projected p = {0,0,0,0,1};
    int64_t v[3], eye[3];
    for (int j=0;j<3;++j) {
        v[j] = (int64_t)vertex[j] + c->offset[j];
        p.input_fits &= fits16(v[j]);
    }
    for (int j=0;j<3;++j) {
        int64_t n = (int64_t)c->t[j]*4096;
        for (int k=0;k<3;++k) n += (int64_t)c->r[j*3+k]*v[k];
        eye[j] = floor_div(n,4096);
    }
    if (eye[2] < 160 || eye[2] > FAR_Z) return p;
    int64_t x = 160 + floor_div(160*eye[0],eye[2]);
    int64_t y = 120 + floor_div(160*eye[1],eye[2]);
    /* No signed-GPU-coordinate wrapping. Clipping beyond this is unimplemented
     * enhancement coverage, explicitly counted rather than packed incorrectly. */
    if (x < -1024 || x > 1023 || y < -1024 || y > 1023) return p;
    p.x=(int32_t)x; p.y=(int32_t)y; p.z=(int32_t)eye[2]; p.valid=1;
    return p;
}
static uint32_t xy(Projected p) {
    return (uint16_t)p.x | ((uint32_t)(uint16_t)p.y << 16);
}
static int depth_compare(const void *a, const void *b) {
    const FarFace *x=a, *y=b;
    if (x->z != y->z) return x->z < y->z ? -1 : 1;
    return x->face < y->face ? -1 : x->face != y->face;
}
static uint32_t bucket(int32_t z) {
    uint32_t result=(uint32_t)z >> 2;
    return result < 8190u ? result : 8190u;
}
static void capture_camera(CPUState *cpu, uint32_t address) {
    (void)address;
    Camera next;
    if ((cpu->gte_ctrl[26]&0xffffu)!=160 || cpu->gte_ctrl[24]!=(160u<<16) ||
        cpu->gte_ctrl[25]!=(120u<<16)) { camera_error=3; pending=1; return; }
    uint32_t ptr=cpu->gpr[5], parent;
    if (!ram(ptr,0x48)) { camera_error=2; pending=1; return; }
    parent=psx_mod_read_word(ptr+0x44);
    if (!ram(parent,0x48) || psx_mod_read_word(parent+0x44)) {
        camera_error=2; pending=1; return;
    }
    memset(&next,0,sizeof next);
    for (unsigned j=0;j<9;++j) next.r[j]=(int16_t)psx_mod_read_half(parent+j*2);
    for (unsigned j=0;j<3;++j) {
        next.t[j]=(int32_t)psx_mod_read_word(parent+0x14+j*4);
        next.offset[j]=(int32_t)psx_mod_read_word(ptr+0x14+j*4);
    }
    if (pending && memcmp(&camera,&next,sizeof next)) camera_error=6;
    camera=next; camera_address=ptr; pending=1;
}
static void extend_track(CPUState *cpu, uint32_t address) {
    (void)cpu; (void)address;
    if (!pending || !stats) return;
    pending=0;
    stat(3,0xffffffffu); /* In-progress marker for coherent TCP sampling. */
    stat(2,psx_mod_read_word(stats+8)+1); stat(4,0); stat(7,camera_address);
    if (camera_error) { stat(3,camera_error); camera_error=0; return; }
    if (psx_mod_read_word(stats+32)!=1) { stat(3,7); return; }
    if (psx_mod_display_width()!=320 || psx_mod_display_height()!=240) {
        stat(3,3); return;
    }
    uint32_t world=psx_mod_read_word(0x80094a10u);
    if (!ram(world,32)) { stat(3,1); return; }
    uint32_t nv=psx_mod_read_word(world), nf=psx_mod_read_word(world+4);
    uint32_t vp=psx_mod_read_word(world+12), fp=psx_mod_read_word(world+16);
    if (!nv || nv>MAX_VERTICES || !nf || nf>MAX_FACES ||
        !ram(vp,nv*16) || !ram(fp,nf*20)) { stat(3,1); return; }
    unsigned buffer=psx_mod_read_half(0x80094c6cu);
    if (buffer>1 || !arena) { stat(3,4); return; }
    uint32_t ot=psx_mod_read_word(0x80094adcu+buffer*4);
    if (!ram(ot,8192*4) || (ot & 3)) { stat(3,4); return; }
    for (uint32_t i=0;i<nv;++i) {
        int32_t v[3];
        for (unsigned j=0;j<3;++j) v[j]=(int32_t)psx_mod_read_word(vp+i*16+j*4);
        projected[i]=project(v,&camera);
    }
    uint32_t count=0, invalid=0, near=0, outside=0;
    int margin=psx_mod_widescreen_x_margin();
    if (margin<0 || margin>160) { stat(3,3); return; }
    for (uint32_t i=0;i<nf;++i) {
        unsigned good=1, original=1;
        int minx=1024, maxx=-1024, miny=1024, maxy=-1024;
        uint16_t indices[4];
        for (unsigned j=0;j<4;++j) {
            indices[j]=psx_mod_read_half(fp+i*20+j*2);
            if (indices[j]>=nv) { stat(3,5); return; }
            Projected p=projected[indices[j]];
            good &= p.valid; original &= p.input_fits;
            if (p.x<minx) minx=p.x;
            if (p.x>maxx) maxx=p.x;
            if (p.y<miny) miny=p.y;
            if (p.y>maxy) maxy=p.y;
        }
        if (!good) { ++invalid; continue; }
        if (original) { ++near; continue; }
        if (maxx < -margin || minx > 319+margin || maxy<0 || miny>239) {
            ++outside; continue;
        }
        selected[count++]=(FarFace){i,projected[indices[2]].z};
    }
    qsort(selected,count,sizeof selected[0],depth_compare);
    uint32_t base=arena+buffer*MAX_FACES*PACKET_BYTES;
    for (uint32_t i=0;i<count;++i) {
        uint32_t face=fp+selected[i].face*20, packet=base+i*PACKET_BYTES;
        uint32_t texture=0x800c39ecu+(uint32_t)psx_mod_read_byte(face+14)*504;
        uint32_t page=psx_mod_read_word(texture);
        if (psx_mod_read_byte(face+15)&4) texture+=252;
        uint32_t words[10]={0};
        static const unsigned order[4]={1,0,2,3};
        uint32_t slot=ot+bucket(selected[i].z)*4;
        words[0]=0x09000000u | (psx_mod_read_word(slot)&0x00ffffffu);
        words[1]=0x2c000000u | (psx_mod_read_word(face+16)&0x00ffffffu);
        for (unsigned j=0;j<4;++j) {
            words[2+j*2]=xy(projected[psx_mod_read_half(face+order[j]*2)]);
            words[3+j*2]=psx_mod_read_half(texture+4+j*2);
        }
        words[3]|=page&0xffff0000u; words[5]|=page<<16;
        for (unsigned j=0;j<10;++j) psx_mod_write_word(packet+j*4,words[j]);
        psx_mod_write_word(slot,packet&0x00ffffffu);
    }
    stat(4,count); stat(5,psx_mod_read_word(stats+20)+count);
    stat(10,invalid); stat(11,near); stat(12,outside);
    stat(13,psx_mod_read_half(0x80094b8cu)); stat(14,ot); stat(15,buffer);
    stat(3,0);
}
static void activate_track_distance(void) {
    stats=psx_mod_alloc_guest_memory(64,16);
    arena=psx_mod_alloc_gpu_dma_memory(2*MAX_FACES*PACKET_BYTES,16);
    stat(0,STATS_MAGIC); stat(1,1); stat(3,arena?0:4);
    stat(6,arena); stat(8,1); stat(9,MAX_FACES);
    psx_mod_register_function_entry_plugin("wxl.track-distance",0x80013c78,capture_camera);
    psx_mod_register_function_entry_plugin("wxl.track-distance",0x800140ac,capture_camera);
    psx_mod_register_function_entry_plugin("wxl.track-distance",0x80014ad4,capture_camera);
    psx_mod_register_function_entry_plugin("wxl.track-distance",0x800684c4,extend_track);
}
PSX_MOD_CONSTRUCTOR(register_wxl_track_distance) {
    psx_mod_register_activation_plugin("wxl.track-distance",activate_track_distance);
}
