/* Experimental, opt-in road/scenery extension. Evidence: docs/scenery-distance.md.
 * Guest code and near-road rendering remain authoritative and unchanged.
 */
#include "mod_plugins.h"
#include "mod_memory.h"
#include "cpu_state.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define MAX_VERTICES 32768u
#define MAX_FACES 8192u
#define MAX_PACKETS 12288u
#define PACKET_BYTES 52u
#define FAR_Z 64000
#define STATS_MAGIC 0x444C5857u

typedef struct { int32_t x, y, z; int valid, input_fits; } Projected;
typedef struct { uint32_t face; int32_t z; } FarFace;
typedef struct { uint32_t words[13]; int32_t z; int32_t precise[8]; unsigned has_precise; } FarPacket;
typedef struct { int16_t r[9]; int32_t t[3], offset[3]; } Camera;
static Projected projected[MAX_VERTICES];
static FarFace selected[MAX_FACES];
static FarPacket packets[MAX_PACKETS];
static uint32_t packet_count, seen_models[2048], seen_count, seen_overflow;
static Camera camera;
static uint32_t stats, arena, camera_address, pending, camera_error;
static int32_t billboard_roll;
static unsigned billboard_roll_valid;

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
        p.input_fits &= fits16(eye[j]);
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
static int packet_compare(const void *a, const void *b) {
    const FarPacket *x=a, *y=b;
    return x->z < y->z ? -1 : x->z > y->z;
}
static void note_model(CPUState *cpu, uint32_t address) {
    (void)address;
    for (uint32_t i=0;i<seen_count;++i) if (seen_models[i]==cpu->gpr[4]) return;
    if (seen_count<2048) seen_models[seen_count++]=cpu->gpr[4];
    else seen_overflow=1;
}
static int model_seen(uint32_t model) {
    for (uint32_t i=0;i<seen_count;++i) if (seen_models[i]==model) return 1;
    return 0;
}
/* Source record layout matches the ordinary scenery renderer 0x80010000.
 * Gouraud-textured source types 6/8 deliberately emit its flat-textured path.
 * Billboard types 10/11 use their distinct anchored-quad path below.
 */
static int scenery_packet(uint32_t src, unsigned type, unsigned flags,
                          const Projected *v, uint32_t out[10]) {
    unsigned nv=(type==1 || type==2 || type==5 || type==6)?3:4;
    unsigned textured=(type==2 || type==4 || type==6 || type==8);
    unsigned gouraud=(type==5 || type==7);
    unsigned opcode=textured?(nv==3?0x24:0x2c):gouraud?(nv==3?0x30:0x38):(nv==3?0x20:0x28);
    memset(out,0,10*sizeof *out);
    if (flags&4) opcode|=2;
    unsigned color=textured?(nv==3?24:28):12;
    out[0]=(textured?1+nv*2:gouraud?nv*2:1+nv)<<24;
    out[1]=(opcode<<24)|(psx_mod_read_word(src+color)&0xffffff);
    for (unsigned j=0;j<nv;++j) {
        out[textured||gouraud?2+j*2:2+j]=xy(v[j]);
        if (textured) out[3+j*2]=psx_mod_read_half(src+(nv==3?16:18)+j*2);
        if (gouraud && j) out[1+j*2]=psx_mod_read_word(src+12+j*4)&0xffffff;
    }
    if (textured) {
        out[3]|=(uint32_t)psx_mod_read_half(src+(nv==3?12:14))<<16;
        out[5]|=(uint32_t)psx_mod_read_half(src+(nv==3?14:16))<<16;
    }
    return (int)nv;
}
/* 0x80020128 computes the roll argument before the 0x80010000 call.
 * Capture at function entry, not a pre-delay-slot call trace. */
static void capture_scene_roll(CPUState *cpu, uint32_t address) {
    (void)address;
    uint32_t state=cpu->gpr[4];
    billboard_roll_valid=ram(state,0x76);
    if (billboard_roll_valid)
        billboard_roll=(psx_mod_read_word(state+12)&4)?(int16_t)psx_mod_read_half(state+0x74):0;
}
/* Facts recovered from 0x8007EF1C/0x8007EF6C and 0x8007F024. Read the
 * game's own quarter-wave table; no copied game data in this source. */
static int32_t billboard_sin(int32_t angle) {
    uint32_t a=(uint32_t)(angle<0?-angle:angle)&4095u;
    unsigned index=(a&1024u)?1023u-(a&1023u):(a&1023u);
    int32_t value=(int16_t)psx_mod_read_half(0x80091880u+index*2);
    if (a&2048u) value=-value;
    return angle<0?-value:value;
}
static int32_t billboard_cos(int32_t angle) {
    uint32_t a=(uint32_t)(angle<0?-angle:angle)&4095u;
    unsigned index=(a&1024u)?(a&1023u):1023u-(a&1023u);
    int32_t value=(int16_t)psx_mod_read_half(0x80091880u+index*2);
    return (a>=1024u && a<3072u)?-value:value;
}
/* 10 anchors the upper edge; 11 anchors the lower edge. The original
 * billboard path always emits opaque FT4, irrespective of source flags. */
static int billboard_packet(uint32_t src, unsigned type, Projected center,
                            int32_t roll, int margin, uint32_t out[10]) {
    int32_t width=(int16_t)psx_mod_read_half(src+6), height=(int16_t)psx_mod_read_half(src+8);
    int32_t texture_index=(int16_t)psx_mod_read_half(src+10);
    if (!center.valid || center.z<2048 || width<=0 || height<=0 || texture_index<0) return 0;
    uint32_t texture=psx_mod_read_word(0x800ce468u+(uint32_t)texture_index*4);
    if (!ram(texture,38)) return 0;
    int32_t hw=(width*160/center.z)>>1, h=height*160/center.z;
    int32_t dx=(int32_t)floor_div((int64_t)billboard_cos(roll)*hw,4096);
    int32_t dy=(int32_t)floor_div((int64_t)billboard_sin(roll)*hw,4096);
    int32_t hx=(int32_t)floor_div((int64_t)billboard_cos(1024-roll)*h,4096);
    int32_t hy=(int32_t)floor_div((int64_t)billboard_sin(1024-roll)*h,4096);
    Projected v[4]={center,center,center,center};
    unsigned anchor=type==10?0:2, other=2-anchor;
    v[anchor].x-=dx; v[anchor].y-=dy;
    v[anchor+1].x+=dx; v[anchor+1].y+=dy;
    int sign=type==10?-1:1;
    for (unsigned j=0;j<2;++j) {
        v[other+j].x=v[anchor+j].x+sign*hx;
        v[other+j].y=v[anchor+j].y-sign*hy;
    }
    int minx=1024,maxx=-1024,miny=1024,maxy=-1024;
    for (unsigned j=0;j<4;++j) {
        if (v[j].x < -1024 || v[j].x>1023 || v[j].y < -1024 || v[j].y>1023) return 0;
        if (v[j].x<minx) minx=v[j].x;
        if (v[j].x>maxx) maxx=v[j].x;
        if (v[j].y<miny) miny=v[j].y;
        if (v[j].y>maxy) maxy=v[j].y;
    }
    if (maxx < -margin || minx>319+margin || maxy<0 || miny>239) return 0;
    memset(out,0,40); out[0]=0x09000000u;
    out[1]=0x2c000000u|(psx_mod_read_word(src+12)&0xffffffu);
    for (unsigned j=0;j<4;++j) {
        out[2+j*2]=xy(v[j]);
        out[3+j*2]=(psx_mod_read_half(texture+22+j*4)&255u)|((psx_mod_read_half(texture+24+j*4)&255u)<<8);
    }
    out[3]|=(uint32_t)psx_mod_read_half(texture+4)<<16;
    out[5]|=(uint32_t)psx_mod_read_half(texture+2)<<16;
    return 1;
}
static void append_scenery(CPUState *cpu, int margin) {
    static const unsigned sizes[12]={0,16,28,16,32,24,36,28,44,0,16,16};
    uint32_t start=packet_count, models=0, billboards=0, billboard_added=0, error=0;
    stat(17,0); stat(18,0); stat(19,0); stat(20,0); stat(21,0);
    if (psx_mod_read_word(stats+64)!=1) return;
    if (seen_overflow) { stat(19,1); return; }
    /* Verified game transform: static scenery -> identity camera-offset node
     * -> root camera rotation. Do not generalize other hierarchies by guessing. */
    for (unsigned j=0;j<9;++j)
        if ((int16_t)psx_mod_read_half(camera_address+j*2)!=(j%4==0?4096:0)) {
            stat(19,2); return;
        }
    uint32_t table=psx_mod_read_word(0x80094a80u);
    if (!ram(table,4)) { stat(19,3); return; }
    uint32_t model=psx_mod_read_word(table);
    while (model) {
        if (models>=2048 || !ram(model,64) || !ram(table+models*4,4) ||
            psx_mod_read_word(table+models*4)!=model) { error=3; break; }
        ++models;
        uint32_t nv=psx_mod_read_half(model+0x10), np=psx_mod_read_half(model+0x20);
        uint32_t vp=psx_mod_read_word(model+0x14), src=psx_mod_read_word(model+0x24);
        uint32_t node=psx_mod_read_word(model+0x30);
        if (!nv || nv>8192 || !np || np>8192 || !ram(vp,nv*8) || !ram(node,72) ||
            psx_mod_read_word(node+0x44)!=camera_address) { error=3; break; }
        Camera transform={0};
        for (unsigned row=0;row<3;++row) {
            int64_t parent_t=0, local_t=0;
            for (unsigned k=0;k<3;++k) {
                parent_t+=(int64_t)camera.r[row*3+k]*camera.offset[k];
                local_t+=(int64_t)camera.r[row*3+k]*(int32_t)psx_mod_read_word(node+20+k*4);
            }
            /* 0x8001E8A8 composes these two parent levels separately. */
            int64_t t=(int64_t)camera.t[row]+floor_div(parent_t,4096)+floor_div(local_t,4096);
            if (t<INT32_MIN || t>INT32_MAX) { error=4; break; }
            transform.t[row]=(int32_t)t;
            for (unsigned col=0;col<3;++col) {
                int64_t value=0;
                for (unsigned k=0;k<3;++k) value+=(int64_t)camera.r[row*3+k]*(int16_t)psx_mod_read_half(node+(k*3+col)*2);
                value=floor_div(value,4096);
                if (!fits16(value)) { error=4; break; }
                transform.r[row*3+col]=(int16_t)value;
            }
        }
        if (error) break;
        for (unsigned j=0;j<nv;++j) {
            int32_t vertex[3];
            for (unsigned k=0;k<3;++k) vertex[k]=(int16_t)psx_mod_read_half(vp+j*8+k*2);
            projected[j]=project(vertex,&transform);
        }
        int seen=model_seen(model);
        for (unsigned i=0;i<np;++i) {
            if (!ram(src,4)) { error=5; break; }
            unsigned type=psx_mod_read_half(src), flags=psx_mod_read_half(src+2);
            if (type>=12 || !sizes[type] || !ram(src,sizes[type])) { error=5; break; }
            uint32_t next=src+sizes[type];
            if (type>=10) {
                unsigned vi=psx_mod_read_half(src+4);
                if (vi>=nv) { error=6; break; }
                Projected center=projected[vi];
                if (!billboard_roll_valid) ++billboards;
                else if (!(seen && center.input_fits)) {
                    uint32_t words[10];
                    if (billboard_packet(src,type,center,billboard_roll,margin,words)) {
                        if (packet_count>=MAX_PACKETS) { error=7; break; }
                        FarPacket *p=&packets[packet_count++];
                        memcpy(p->words,words,sizeof words); p->z=center.z; ++billboard_added;
                    }
                }
                src=next; continue;
            }
            unsigned n=(type==1 || type==2 || type==5 || type==6)?3:4;
            Projected v[4]; unsigned good=1, original=1; int32_t sumz=0;
            int minx=1024,maxx=-1024,miny=1024,maxy=-1024;
            for (unsigned j=0;j<n;++j) {
                unsigned vi=psx_mod_read_half(src+4+j*2);
                if (vi>=nv) { error=6; break; }
                v[j]=projected[vi]; good&=v[j].valid; original&=v[j].input_fits; sumz+=v[j].z;
                if (v[j].x<minx) minx=v[j].x;
                if (v[j].x>maxx) maxx=v[j].x;
                if (v[j].y<miny) miny=v[j].y;
                if (v[j].y>maxy) maxy=v[j].y;
            }
            if (error) break;
            if (good && !(seen && original) && maxx>=-margin && minx<=319+margin && maxy>=0 && miny<=239) {
                int64_t cross=(int64_t)(v[1].x-v[0].x)*(v[2].y-v[0].y)-(int64_t)(v[1].y-v[0].y)*(v[2].x-v[0].x);
                if (!(flags&1) || cross>0) {
                    if (packet_count>=MAX_PACKETS) { error=7; break; }
                    FarPacket *p=&packets[packet_count++];
                    scenery_packet(src,type,flags,v,p->words);
                    /* AVSZ3/4 ordering: widen its intermediate, retain ZSF. */
                    int32_t zsf=(int16_t)cpu->gte_ctrl[n==3?29:30];
                    if (zsf<=0) { error=8; break; }
                    p->z=(int32_t)floor_div((int64_t)sumz*zsf,1024);
                }
            }
            src=next;
        }
        if (error) break;
        model=psx_mod_read_word(model+0x3c);
    }
    if (error) packet_count=start; /* Whole added scenery pass is atomic. */
    stat(17,packet_count-start); stat(18,billboards); stat(19,error); stat(20,models);
    stat(21,error?0:billboard_added);
}
#include "ship_far_render.h"
#include "ship_far_exhaust.h"
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
static void extend_track_impl(CPUState *cpu, uint32_t address) {
    (void)cpu; (void)address;
    if (!pending || !stats) return;
    pending=0;
    stat(3,0xffffffffu); /* In-progress marker for coherent TCP sampling. */
    stat(2,psx_mod_read_word(stats+8)+1); stat(4,0); stat(7,camera_address);
    if (camera_error) { stat(3,camera_error); camera_error=0; return; }
    if (psx_mod_read_word(stats+32)!=1) { stat(3,7); return; }
    /* Damage shakes DISPENV.screen without changing the rendered scene.
     * PutDispEnv clips its scanout range (e.g. 238 lines for screen.y=2).
     * Validate both nominal DISPENV rectangles, not the clipped scanout.
     * Ghidra: 8004B53C (shake), 8007C4FC (range clamp), 80068318 (setup).
     */
    if (psx_mod_read_half(0x800d0624u)!=320 ||
        psx_mod_read_half(0x800d0626u)!=240 ||
        psx_mod_read_half(0x800d0638u)!=320 ||
        psx_mod_read_half(0x800d063au)!=240) {
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
    packet_count=0;
    memset(packets,0,sizeof packets);
    uint32_t base=arena+buffer*MAX_PACKETS*PACKET_BYTES;
    for (uint32_t i=0;i<count;++i) {
        uint32_t face=fp+selected[i].face*20;
        uint32_t texture=0x800c39ecu+(uint32_t)psx_mod_read_byte(face+14)*504;
        uint32_t page=psx_mod_read_word(texture);
        if (psx_mod_read_byte(face+15)&4) texture+=252;
        FarPacket *p=&packets[packet_count++];
        uint32_t *words=p->words; memset(words,0,sizeof p->words); p->z=selected[i].z;
        static const unsigned order[4]={1,0,2,3};
        words[0]=0x09000000u;
        words[1]=0x2c000000u | (psx_mod_read_word(face+16)&0x00ffffffu);
        for (unsigned j=0;j<4;++j) {
            words[2+j*2]=xy(projected[psx_mod_read_half(face+order[j]*2)]);
            words[3+j*2]=psx_mod_read_half(texture+4+j*2);
        }
        words[3]|=page&0xffff0000u; words[5]|=page<<16;
    }
    append_scenery(cpu,margin);
    append_distant_ships(cpu,margin,fp,nf);
    append_distant_trails(cpu,margin);
    qsort(packets,packet_count,sizeof packets[0],packet_compare);
    for (uint32_t i=0;i<packet_count;++i) {
        uint32_t packet=base+i*PACKET_BYTES, slot=ot+bucket(packets[i].z)*4;
        packets[i].words[0]|=psx_mod_read_word(slot)&0xffffffu;
        for (unsigned j=0;j<13;++j) psx_mod_write_word(packet+j*4,packets[i].words[j]);
        if (packets[i].has_precise) for(unsigned v=0;v<4;++v) {
            unsigned w=3+v*2;
            if (!psx_mod_gpu_vertex_set(packet+w*4,packets[i].words[w],
                    packets[i].precise[v*2],packets[i].precise[v*2+1])) stat(25,3);
        }
        psx_mod_write_word(slot,packet&0xffffffu);
    }
    stat(26,psx_mod_gpu_vertex_hits());
    stat(4,packet_count); stat(5,psx_mod_read_word(stats+20)+packet_count);
    stat(10,invalid); stat(11,near); stat(12,outside);
    stat(13,psx_mod_read_half(0x80094b8cu)); stat(14,ot); stat(15,buffer);
    stat(3,0);
}
static void extend_track(CPUState *cpu, uint32_t address) {
    extend_track_impl(cpu,address);
    seen_count=seen_overflow=billboard_roll_valid=0;
}
static void activate_track_distance(void) {
    stats=psx_mod_alloc_guest_memory(128,16);
    arena=psx_mod_alloc_gpu_dma_memory(2*MAX_PACKETS*PACKET_BYTES,16);
    stat(0,STATS_MAGIC); stat(1,5); stat(3,arena?0:4);
    stat(6,arena); stat(8,1); stat(9,MAX_PACKETS); stat(16,1);
    psx_mod_register_function_entry_plugin("wxl.track-distance",0x800726ac,capture_distant_trails);
    psx_mod_register_function_entry_plugin("wxl.track-distance",0x80010000,note_model);
    psx_mod_register_function_entry_plugin("wxl.track-distance",0x80020128,capture_scene_roll);
    psx_mod_register_function_entry_plugin("wxl.track-distance",0x80013c78,capture_camera);
    psx_mod_register_function_entry_plugin("wxl.track-distance",0x800140ac,capture_camera);
    psx_mod_register_function_entry_plugin("wxl.track-distance",0x80014ad4,capture_camera);
    psx_mod_register_function_entry_plugin("wxl.track-distance",0x800684c4,extend_track);
}
PSX_MOD_CONSTRUCTOR(register_wxl_track_distance) {
    psx_mod_register_activation_plugin("wxl.track-distance",activate_track_distance);
}
