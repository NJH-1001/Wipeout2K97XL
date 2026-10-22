/* Presentation-only geometry ownership. Original game state is read only.
 * Hook instruction identity is checked by the recompiler; GPU payload reuse
 * is checked again by geometry_packet_tag.h before an identity is consumed.
 */
#include "mod_plugins.h"
#include "gpu.h"
#include "cpu_state.h"
#include "geometry_projection.h"
#include "gpu_gl_renderer.h"
#include "gpu_render.h"
#include "billboard_projection.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* A pose belongs to a mesh instance, not its temporary GPU packet buffer.
 * A missing frame, changed mesh, or second transform of the same instance
 * invalidates its predecessor. All arrays are bounded host-only storage. */
#define GEOMETRY_POSES 4096u
typedef struct {
    uint32_t object, mesh, mesh_count, epoch, controls[32], previous_controls[32];
    int32_t offset[3], previous_offset[3];
    int32_t roll,previous_roll;
    unsigned ready, conflict;
} GeometryPose;
static GeometryPose poses[GEOMETRY_POSES];
static uint32_t geometry_epoch=1, track_camera;
static uint64_t projection_packets, projection_valid, projection_history, projection_ambiguous;
typedef struct {
    uint32_t object,face,epoch,count;
    int16_t vertices[25][3];
    GeometryProjection now[25],previous[25];
} GeometryFaceCache;
static GeometryFaceCache face_cache[2048];
static uint64_t projection_cache_hits,projection_gte_vertices;
static uint64_t billboard_packets,billboard_valid,billboard_history;
extern void wxl_far_geometry_reset(void);
extern int gte_precision_load_word(uint32_t,uint32_t,int32_t*,int32_t*,uint16_t*);
static void geometry_reset_poses(void) {
    memset(poses,0,sizeof(poses));
    memset(face_cache,0,sizeof(face_cache));
    wxl_far_geometry_reset();
}

static int geometry_ram(uint32_t addr,uint32_t bytes) {
    uint32_t p=addr&0x1fffffffu;
    return p>=0x10000u && p<0x200000u && bytes<=0x200000u-p;
}
static int controls_equal(const uint32_t a[32],const uint32_t b[32]) {
    return !memcmp(a,b,8*sizeof(uint32_t)) && !memcmp(a+24,b+24,3*sizeof(uint32_t));
}
static GeometryPose* geometry_pose(CPUState* cpu,uint32_t object,uint32_t mesh,uint32_t mesh_count,
                                   const int32_t offset[3],int32_t roll) {
    unsigned bucket=((object>>3)*2654435761u)&(GEOMETRY_POSES-1u);
    for (unsigned i=0;i<32;++i) {
        GeometryPose* p=&poses[(bucket+i)&(GEOMETRY_POSES-1u)];
        if (p->object && p->object!=object) continue;
        if (p->epoch!=geometry_epoch || !p->object) {
            p->ready=p->object && p->epoch+1u==geometry_epoch && p->mesh==mesh && p->mesh_count==mesh_count && !p->conflict;
            memcpy(p->previous_controls,p->controls,sizeof(p->controls));
            memcpy(p->previous_offset,p->offset,sizeof(p->offset));
            p->previous_roll=p->roll;p->roll=roll;
            memcpy(p->controls,cpu->gte_ctrl,sizeof(p->controls));
            memcpy(p->offset,offset,sizeof(p->offset));
            p->object=object; p->mesh=mesh; p->mesh_count=mesh_count;p->epoch=geometry_epoch; p->conflict=0;
        } else if (p->mesh!=mesh || p->mesh_count!=mesh_count || !controls_equal(p->controls,cpu->gte_ctrl) ||
                   memcmp(p->offset,offset,sizeof(p->offset)) || p->roll!=roll) {
            p->conflict=1; p->ready=0;
        }
        return p;
    }
    return NULL;
}
static void geometry_tick(CPUState* cpu,uint32_t pc) {
    (void)cpu; (void)pc;
    if (++geometry_epoch==0) {memset(poses,0,sizeof(poses));geometry_epoch=1;}
    gpu_geometry_set_frame(geometry_epoch);
}
static void geometry_camera(CPUState* cpu,uint32_t pc) {
    (void)pc;
    track_camera=cpu->gpr[5];
}
/* Match by exact current GTE output before attaching the old projection.
 * Ambiguous coincident vertices with different old positions fail closed. */
static void geometry_project_packet(CPUState* cpu,uint32_t packet,GeometryPose* pose,
                                    const int16_t (*current)[3],
                                    const int16_t (*previous)[3],unsigned count,
                                    uint32_t face,unsigned shared_count) {
    GeometryProjection a[30],b[30];
    projection_packets++;
    if (!pose || pose->conflict || !count || count>30) return;
    GeometryFaceCache* cached=NULL;
    if(shared_count && shared_count<=25 && shared_count<=count) {
        unsigned bucket=((face>>2)^(pose->object>>4))&2047u;
        for(unsigned i=0;i<8;++i) {
            GeometryFaceCache* c=&face_cache[(bucket+i)&2047u];
            if(c->epoch==geometry_epoch && (c->object!=pose->object || c->face!=face)) continue;
            cached=c;break;
        }
        if(cached && cached->epoch==geometry_epoch && cached->object==pose->object &&
            cached->face==face && cached->count==shared_count &&
            !memcmp(cached->vertices,current,shared_count*sizeof(current[0]))) {
            memcpy(a,cached->now,shared_count*sizeof(a[0]));
            memcpy(b,cached->previous,shared_count*sizeof(b[0]));
            projection_cache_hits++;
        } else if(cached) {
            if(!gte_project_geometry(pose->controls,cpu->gpr[31],current,shared_count,a)) return;
            projection_gte_vertices+=shared_count;
            if(pose->ready) {
                if(!gte_project_geometry(pose->previous_controls,cpu->gpr[31],previous,shared_count,b)) return;
                projection_gte_vertices+=shared_count;
            } else memset(b,0,shared_count*sizeof(b[0]));
            cached->epoch=geometry_epoch;cached->object=pose->object;cached->face=face;cached->count=shared_count;
            memcpy(cached->vertices,current,shared_count*sizeof(current[0]));
            memcpy(cached->now,a,shared_count*sizeof(a[0]));
            memcpy(cached->previous,b,shared_count*sizeof(b[0]));
        }
    }
    unsigned start=cached?shared_count:0;
    if(start<count) {
        if(!gte_project_geometry(pose->controls,cpu->gpr[31],current+start,count-start,a+start)) return;
        projection_gte_vertices+=count-start;
        if(pose->ready) {
            if(!gte_project_geometry(pose->previous_controls,cpu->gpr[31],previous+start,count-start,b+start)) return;
            projection_gte_vertices+=count-start;
        }
    }
    uint32_t op=psx_mod_read_word(packet+4)>>24;
    unsigned n=(op&8u)?4u:3u, stride=1u+!!(op&4u)+!!(op&16u);
    unsigned words=psx_mod_read_word(packet)>>24;
    if (words>12 || 1u+(n-1u)*stride>=words) return;
    unsigned matches[4], valid=0;
    for (unsigned i=0;i<n;++i) {
        uint32_t packed=psx_mod_read_word(packet+8u+i*stride*4u);
        matches[i]=count;
        for (unsigned j=0;j<count;++j)
            if (a[j].packed_xy==packed && !(a[j].flags&0x80460000u)) {
                matches[i]=j; break;
            }
        if (matches[i]<count) valid++;
    }
    if (valid==n) projection_valid++;
    if (!pose->ready) return;
    int32_t xy[12][2]={{0}};
    uint16_t mask=0;
    for (unsigned i=0;i<n;++i) {
        unsigned word=1u+i*stride;
        uint32_t packed=psx_mod_read_word(packet+8u+i*stride*4u);
        /* Fallback is per vertex. Freezing a whole polygon because one corner
         * crosses a clip/teleport guard separates its other, shared corners
         * from adjacent polygons that still have usable history. */
        xy[word][0]=(int16_t)packed*65536;
        xy[word][1]=(int16_t)(packed>>16)*65536;
        mask|=(uint16_t)(1u<<word);
        unsigned m=matches[i];
        if(m==count)continue;
        if(gte_geometry_correction_enabled()) {
            xy[word][0]=a[m].x16;xy[word][1]=a[m].y16;
        }
        if (b[m].flags&0x80460000u) continue;
        unsigned ambiguous=0;
        for (unsigned j=m+1;j<count;++j)
            if (a[j].packed_xy==a[m].packed_xy &&
                (b[j].x16!=b[m].x16 || b[j].y16!=b[m].y16)) {
                ambiguous=1;break;
            }
        if(ambiguous) {
            int32_t precise_x,precise_y;uint16_t depth;
            uint32_t addr=packet+8u+i*stride*4u;
            unsigned found=count;
            if(gte_precision_load_word(addr,a[m].packed_xy,&precise_x,&precise_y,&depth))
                for(unsigned j=0;j<count;++j)
                    if(a[j].packed_xy==a[m].packed_xy && a[j].x16==precise_x && a[j].y16==precise_y) {
                        if(found<count && (b[j].x16!=b[found].x16 || b[j].y16!=b[found].y16)) {found=count;break;}
                        found=j;
                    }
            if(found==count) {projection_ambiguous++;continue;}
            m=found;
            if(b[m].flags&0x80460000u) continue;
        }
        /* Reject a cut/teleport rather than sweep geometry across the screen. */
        if (llabs((int64_t)a[m].x16-b[m].x16)>INT64_C(96)*65536 ||
            llabs((int64_t)a[m].y16-b[m].y16)>INT64_C(96)*65536) continue;
        xy[word][0]=gte_geometry_correction_enabled()?b[m].x16:(int16_t)b[m].packed_xy*65536;
        xy[word][1]=gte_geometry_correction_enabled()?b[m].y16:(int16_t)(b[m].packed_xy>>16)*65536;
        mask|=(uint16_t)(1u<<word);
    }
    gpu_geometry_tag_previous(packet,xy,mask);
    projection_history++;
}
int wxl_geometry_projection_status_json(char* buf,int cap) {
    return snprintf(buf,(size_t)cap,
        "{\"epoch\":%u,\"packets\":%llu,\"validated\":%llu,\"history\":%llu,\"ambiguous\":%llu,\"cache_hits\":%llu,\"gte_vertices\":%llu,\"billboards\":[%llu,%llu,%llu]}",
        geometry_epoch,(unsigned long long)projection_packets,(unsigned long long)projection_valid,
        (unsigned long long)projection_history,(unsigned long long)projection_ambiguous,
        (unsigned long long)projection_cache_hits,(unsigned long long)projection_gte_vertices,
        (unsigned long long)billboard_packets,(unsigned long long)billboard_valid,(unsigned long long)billboard_history);
}
static void geometry_project_billboard(CPUState* cpu,uint32_t packet,uint32_t face,
                                       unsigned type,GeometryPose* pose,const int16_t (*v)[3]) {
    projection_packets++;
    billboard_packets++;
    if(!pose || pose->conflict)return;
    GeometryProjection now,old;
    uint32_t corners[4],previous_corners[4];
    if(!gte_project_geometry(pose->controls,cpu->gpr[31],v,1,&now) ||
       (now.flags&0x80460000u) ||
       !wxl_billboard_corners(face,type,now.packed_xy,now.depth,pose->roll,corners))return;
    projection_gte_vertices++;
    for(unsigned i=0;i<4;++i)
        if(psx_mod_read_word(packet+8u+i*8u)!=corners[i])return;
    projection_valid++;
    billboard_valid++;
    if(!pose->ready || !gte_project_geometry(pose->previous_controls,cpu->gpr[31],v,1,&old) ||
       (old.flags&0x80460000u) ||
       !wxl_billboard_corners(face,type,old.packed_xy,old.depth,pose->previous_roll,previous_corners))return;
    projection_gte_vertices++;
    int32_t xy[12][2]={{0}};
    for(unsigned i=0;i<4;++i) {
        int dx=(int16_t)corners[i]-(int16_t)previous_corners[i];
        int dy=(int16_t)(corners[i]>>16)-(int16_t)(previous_corners[i]>>16);
        if(dx < -96 || dx>96 || dy < -96 || dy>96)return;
        xy[1+i*2][0]=(int16_t)previous_corners[i]*65536;
        xy[1+i*2][1]=(int16_t)(previous_corners[i]>>16)*65536;
    }
    gpu_geometry_tag_previous(packet,xy,0xaau);projection_history++;billboard_history++;
}

typedef struct {uint32_t pc; unsigned packet,face,object;} GeometryStore;
static const GeometryStore geometry_stores[]={
    {0x80010234u,4,20,16},
    {0x80010398u,4,20,16},
    {0x800104d0u,4,20,16},
    {0x80010634u,4,20,16},
    {0x80010750u,4,20,16},
    {0x80010898u,4,20,16},
    {0x800109a8u,4,20,16},
    {0x80010adcu,4,20,16},
    {0x80010d38u,4,20,16},
    {0x80010f9cu,4,20,16},
    {0x800110e8u,4,20,16},
    {0x80011228u,4,20,16},
    {0x80011344u,4,20,16},
    {0x80011484u,4,20,16},
    {0x80011584u,4,20,16},
    {0x800116a8u,4,20,16},
    {0x8001179cu,4,20,16},
    {0x800118acu,4,20,16},
    {0x80011b08u,4,20,16},
    {0x80011d6cu,4,20,16},
    {0x80012038u,4,20,16},
    {0x800121c4u,4,20,16},
    {0x8001231cu,4,20,16},
    {0x800124a0u,4,20,16},
    {0x800125d0u,4,20,16},
    {0x8001272cu,4,20,16},
    {0x80012850u,4,20,16},
    {0x80012998u,4,20,16},
    {0x80012bf4u,4,20,16},
    {0x80012e58u,4,20,16},
    {0x80014048u,5,14,0},
    {0x80014438u,5,25,0},
    {0x80014508u,5,25,0},
    {0x800145d8u,5,25,0},
    {0x800146a8u,5,25,0},
    {0x8001479cu,5,25,0},
    {0x8001488cu,5,25,0},
    {0x8001497cu,5,25,0},
    {0x80014a6cu,5,25,0},
    {0x80015d04u,5,25,0},
    {0x80016100u,5,25,0},
};
static int geometry_race;
static int render60_enabled;
int wxl_render60_active(void) {
    return render60_enabled && geometry_race && gr_backend()==GR_BACKEND_OPENGL &&
        gpu_geometry_capture_enabled();
}
static void geometry_scene(CPUState* cpu,uint32_t pc) {
    (void)cpu;
    geometry_race=pc==0x8003ea68u;
    /* Menu previews use the same verified object renderer as the race. */
    gl_renderer_set_geometry_scene(1);
    memset(poses,0,sizeof(poses));
    track_camera=0;
    gpu_geometry_capture_clear();
}
static void geometry_store(CPUState* cpu,uint32_t pc) {
    if(!cpu || !gpu_geometry_capture_enabled() || gr_backend()!=GR_BACKEND_OPENGL) return;
    for(unsigned n=0;n<sizeof(geometry_stores)/sizeof(geometry_stores[0]);++n) {
        const GeometryStore* s=&geometry_stores[n];
        if(s->pc!=pc) continue;
        if(!geometry_race && !s->object)return;
        uint32_t object=s->object ? cpu->gpr[s->object] : psx_mod_read_word(0x80094a10u);
        uint32_t packet=cpu->gpr[s->packet], face=cpu->gpr[s->face];
        gpu_geometry_tag_packet(packet,object,face,pc);
        int16_t v[30][3],old[30][3];
        int32_t offset[3]={0,0,0};
        if (s->object) {
            if (!geometry_ram(object,0x34) || !geometry_ram(face,12)) return;
            uint32_t vertices=psx_mod_read_word(object+0x14);
            unsigned nv=psx_mod_read_half(object+0x10);
            int32_t roll=pc<0x80011de8u?(int32_t)cpu->gpr[19]:0;
            GeometryPose* pose=geometry_pose(cpu,object,vertices,nv,offset,roll);
            unsigned type=psx_mod_read_half(face), n=type==1||type==2||type==5||type==6?3u:4u;
            if (!pose || !nv || !geometry_ram(vertices,nv*8u)) return;
            if(type==10 || type==11) {
                if(!geometry_ram(face,16))return;
                unsigned index=psx_mod_read_half(face+4u);if(index>=nv)return;
                for(unsigned j=0;j<3;++j)v[0][j]=(int16_t)psx_mod_read_half(vertices+index*8u+j*2u);
                geometry_project_billboard(cpu,packet,face,type,pose,v);return;
            }
            if(type<1 || type>8)return;
            for (unsigned i=0;i<n;++i) {
                unsigned index=psx_mod_read_half(face+4u+i*2u);
                if (index>=nv) return;
                for (unsigned j=0;j<3;++j)
                    old[i][j]=v[i][j]=(int16_t)psx_mod_read_half(vertices+index*8u+j*2u);
            }
            geometry_project_packet(cpu,packet,pose,v,old,n,face,0);
        } else {
            if (!geometry_ram(object,0x14) || !geometry_ram(face,20) ||
                !geometry_ram(track_camera,0x48)) return;
            uint32_t vertices=psx_mod_read_word(object+12);
            for (unsigned j=0;j<3;++j) offset[j]=(int32_t)psx_mod_read_word(track_camera+0x14+j*4u);
            GeometryPose* pose=geometry_pose(cpu,object,vertices,psx_mod_read_word(object),offset,0);
            if (!pose) return;
            if (pc==0x80015d04u || pc==0x80016100u) {
                unsigned n=pc==0x80016100u?30u:25u;
                for (unsigned i=0;i<n;++i) for(unsigned j=0;j<3;++j) {
                    uint32_t addr=i<25?0x1f800010u+i*16u:0x1f800200u+(i-25u)*16u;
                    int32_t now=(int16_t)psx_mod_read_half(addr+j*2u);
                    int64_t was=(int64_t)now+pose->previous_offset[j]-pose->offset[j];
                    if (was < -32768 || was>32767) return;
                    v[i][j]=(int16_t)now;old[i][j]=(int16_t)was;
                }
                geometry_project_packet(cpu,packet,pose,v,old,n,face,25);
                return;
            }
            int64_t points[9][3];
            for (unsigned i=0;i<4;++i) {
                unsigned index=psx_mod_read_half(face+i*2u);
                if (!geometry_ram(vertices+index*16u,12)) return;
                for (unsigned j=0;j<3;++j)
                    points[i][j]=(int32_t)psx_mod_read_word(vertices+index*16u+j*4u);
            }
            /* Same nested signed midpoint construction as 0x800140AC.
             * All nine source points are available even when LOD changes. */
            static const unsigned pairs[5][2]={{0,1},{1,2},{2,3},{0,3},{5,7}};
            for (unsigned i=0;i<5;++i) for(unsigned j=0;j<3;++j) {
                int64_t sum=points[pairs[i][0]][j]+points[pairs[i][1]][j];
                points[4+i][j]=sum/2-(sum<0 && sum%2);
            }
            unsigned n=pc==0x80014048u?4u:9u;
            for (unsigned i=0;i<n;++i) for(unsigned j=0;j<3;++j) {
                v[i][j]=(int16_t)(points[i][j]+pose->offset[j]);
                old[i][j]=(int16_t)(points[i][j]+pose->previous_offset[j]);
            }
            geometry_project_packet(cpu,packet,pose,v,old,n,face,n);
        }
        return;
    }
}
static void activate_geometry_capture(void) {
    static int registered;
    if (registered) return;
    registered=1;
    gpu_geometry_capture_set(1);
    gpu_geometry_set_diagnostics(wxl_geometry_projection_status_json);
    gpu_geometry_set_reset_callback(geometry_reset_poses);
    psx_mod_register_function_entry_plugin("wxl.render60",0x8003ea68u,geometry_scene);
    psx_mod_register_function_entry_plugin("wxl.render60",0x8004da10u,geometry_scene);
    psx_mod_register_function_entry_plugin("wxl.render60",0x800684c4u,geometry_tick);
    psx_mod_register_function_entry_plugin("wxl.render60",0x80013c78u,geometry_camera);
    psx_mod_register_function_entry_plugin("wxl.render60",0x800140acu,geometry_camera);
    psx_mod_register_function_entry_plugin("wxl.render60",0x80014ad4u,geometry_camera);
    for(unsigned n=0;n<sizeof(geometry_stores)/sizeof(geometry_stores[0]);++n)
        psx_mod_register_function_entry_plugin("wxl.render60",geometry_stores[n].pc,geometry_store);
}
static void activate_render60(void) {
    activate_geometry_capture();
    char rate[16]="60";
    psx_mod_option_value("wxl.presentation.render60","render60","frame-rate",rate,sizeof rate);
    gl_renderer_set_geometry_rate(!strcmp(rate,"120")?120:!strcmp(rate,"90")?90:60);
    render60_enabled=1;
    gl_renderer_set_geometry_interpolation(1);
}
PSX_MOD_CONSTRUCTOR(register_wxl_render60) {
    psx_mod_register_activation_plugin("wxl.render60",activate_render60);
    /* Bounded developer census; no rendering mode or launcher claim yet. */
    const char* census=getenv("PSX_GEOMETRY_CAPTURE");
    if(census && census[0]=='1') activate_geometry_capture();
    const char* render=getenv("WXL_GEOMETRY_60FPS");
    if(render && render[0]=='1') activate_render60();
}
