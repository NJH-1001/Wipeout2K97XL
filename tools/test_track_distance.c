/* Synthetic geometry only: bounds, disabled identity and FT4 integration tests. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../src/track_distance_experimental.c"
static unsigned char main_ram[0x200000], expansion[0x100000], dma_ram[0x200000];
static unsigned hooks, allocations;
uint32_t psx_mod_gpu_vertex_hits(void) { return 0; }
int psx_mod_gpu_vertex_set(uint32_t a,uint32_t word,int32_t x,int32_t y) {
    (void)a;(void)word;assert(x>=-1024*65536 && x<1024*65536);assert(y>=-1024*65536 && y<1024*65536);return 1;
}
/* Test doubles for optional presentation metadata; disabled mode must emit none. */
static int smoothing_active;
static unsigned geometry_tags;
int wxl_render60_active(void) { return smoothing_active; }
void gpu_geometry_tag_packet(uint32_t packet,uint32_t owner,uint32_t face,uint32_t part) {
    assert(smoothing_active && packet && owner && face); (void)part; ++geometry_tags;
}
void gpu_geometry_tag_previous(uint32_t packet,const int32_t previous[12][2],uint16_t mask) {
    assert(smoothing_active && packet && previous && mask); ++geometry_tags;
}
static int ship_active;
int wxl_ship_distance_active(void) { return ship_active; }
static unsigned char *memory(uint32_t a, unsigned n) {
    a &= 0x1fffffffu;
    if (a < sizeof main_ram) { assert(n <= sizeof main_ram-a); return main_ram+a; }
    if (a >= 0x1f000000 && a < 0x1f100000) return expansion+(a-0x1f000000);
    assert(a>=0xe00000 && a+n<=0x1000000); return dma_ram+(a-0xe00000);
}
uint8_t psx_mod_read_byte(uint32_t a) { return *memory(a,1); }
uint16_t psx_mod_read_half(uint32_t a) { uint16_t v; memcpy(&v,memory(a,2),2); return v; }
uint32_t psx_mod_read_word(uint32_t a) { uint32_t v; memcpy(&v,memory(a,4),4); return v; }
void psx_mod_write_word(uint32_t a,uint32_t v) { memcpy(memory(a,4),&v,4); }
uint32_t psx_mod_alloc_guest_memory(uint32_t n,uint32_t align) {
    assert(n<=sizeof expansion && align==16); ++allocations; return 0x9f000000;
}
uint32_t psx_mod_alloc_gpu_dma_memory(uint32_t n,uint32_t align) {
    assert(n<=sizeof dma_ram && align==16); ++allocations; return 0x80e00000;
}
int psx_mod_register_activation_plugin(const char *id,PSXModActivationCallback cb) {
    assert(strcmp(id,"wxl.track-distance")==0 && cb); return 1;
}
int psx_mod_register_function_entry_plugin(const char *id,uint32_t a,PSXModFunctionEntryCallback cb) {
    assert(id && a && cb); ++hooks; return 1;
}
uint32_t psx_mod_display_width(void) { return 320; }
static uint32_t visible_height=240;
uint32_t psx_mod_display_height(void) { return visible_height; }
int32_t psx_mod_widescreen_x_margin(void) { return 53; }
static void setup(unsigned faces) {
    memset(main_ram,0,sizeof main_ram);
    memset(&camera,0,sizeof camera);
    camera.r[0]=camera.r[4]=camera.r[8]=4096;
    uint32_t world=0x80010000, vp=0x80011000, fp=0x80020000, ot=0x80100000;
    psx_mod_write_word(0x80094a10,world);
    psx_mod_write_word(world,4); psx_mod_write_word(world+4,faces);
    psx_mod_write_word(world+12,vp); psx_mod_write_word(world+16,fp);
    psx_mod_write_word(0x80094adc,ot);
    for (unsigned i=0;i<8192;++i) psx_mod_write_word(ot+4*i,i?((ot+4*i-4)&0xffffff):0xffffff);
    const int32_t v[4][3]={{-1000,-1000,40000},{1000,-1000,40000},{-1000,1000,40000},{1000,1000,40000}};
    for (unsigned i=0;i<4;++i) memcpy(memory(vp+i*16,12),v[i],12);
    for (unsigned i=0;i<faces;++i) {
        uint16_t ix[4]={0,1,2,3}; memcpy(memory(fp+i*20,8),ix,8);
        psx_mod_write_word(fp+i*20+16,0x00123456);
    }
    psx_mod_write_word(0x800c39ec,0x12340056);
    psx_mod_write_word(0x800c39f0,0x44332211);
    psx_mod_write_word(0x800c39f4,0x88776655);
    psx_mod_write_word(0x800d0624,240u<<16|320u);
    psx_mod_write_word(0x800d0638,240u<<16|320u);
    pending=1; camera_error=0; stat(8,1);
}
int main(void) {
    assert(hooks==0 && allocations==0); /* Constructor alone is inert. */
    assert(floor_div(-1,4096)==-1 && floor_div(-4096,4096)==-1);
    Camera c={0}; c.r[0]=c.r[4]=c.r[8]=4096;
    int32_t v[3]={-1,0,160}; Projected p=project(v,&c);
    assert(p.valid && p.x==159 && p.y==120 && p.input_fits);
    v[2]=64000; assert(project(v,&c).valid && !project(v,&c).input_fits);
    v[2]=64001; assert(!project(v,&c).valid);
    v[2]=159; assert(!project(v,&c).valid);
    v[0]=INT32_MAX; v[2]=32000; assert(!project(v,&c).valid);
    assert(bucket(64000)==8190 && bucket(32760)==8190 && bucket(400)==100);
    activate_track_distance(); assert(hooks==7 && allocations==2);
    setup(1);
    CPUState cpu={0}; cpu.gpr[5]=0x80060000;
    cpu.gte_ctrl[24]=160u<<16; cpu.gte_ctrl[25]=120u<<16; cpu.gte_ctrl[26]=160;
    psx_mod_write_word(cpu.gpr[5]+0x44,0x80060100);
    memcpy(memory(0x80060100,18),camera.r,18);
    pending=0; capture_camera(&cpu,0); assert(pending && !camera_error);
    cpu.gte_ctrl[25]=150u<<16; capture_camera(&cpu,0); assert(camera_error==3);
    setup(1); extend_track(NULL,0);
    assert(psx_mod_read_word(stats+12)==0 && psx_mod_read_word(stats+16)==1);
    assert(psx_mod_read_word(arena+4)==0x2c123456);
    assert(psx_mod_read_word(arena+12)==0x12342211);
    assert(psx_mod_read_word(arena+20)==0x00564433);
    assert(psx_mod_read_word(arena+28)==0x6655 && psx_mod_read_word(arena+36)==0x8877);
    assert(psx_mod_read_word(0x80107ff8)==(arena&0xffffff));
    assert(psx_mod_read_word(0x80107ffc)==0x107ff8); /* Backdrop bucket untouched. */
    assert(psx_mod_read_word(arena+8)==(116u<<16|164u));
    /* Weapon shake changes scanout, never the nominal scene/projection. */
    setup(1); visible_height=238;
    psx_mod_write_word(0x800d0628,2u<<16|0xfffeu);
    psx_mod_write_word(0x800d063c,2u<<16|0xfffeu);
    extend_track(NULL,0);
    assert(psx_mod_read_word(stats+12)==0 && psx_mod_read_word(stats+16)==1);
    for (unsigned buffer=0;buffer<2;++buffer) {
        setup(1);
        psx_mod_write_word(0x800d0624+buffer*20,180u<<16|320u);
        extend_track(NULL,0);
        assert(psx_mod_read_word(stats+12)==3 && psx_mod_read_word(stats+16)==0);
    }
    visible_height=240;
    /* Scenery packet layouts: triangles/quads, colors, texture words and blend. */
    Projected sv[4]={{10,20,40000,1,0},{30,20,40000,1,0},{10,40,40000,1,0},{30,40,40000,1,0}};
    uint32_t out[10], src=0x80075000;
    for (unsigned i=0;i<12;++i) psx_mod_write_word(src+i*4,0x10203040+i);
    scenery_packet(src,2,4,sv,out);
    assert(out[0]==0x07000000 && out[1]==0x26203046);
    assert(out[3]==((psx_mod_read_word(src+12)<<16)|(psx_mod_read_half(src+16))));
    scenery_packet(src,8,0,sv,out);
    assert(out[0]==0x09000000 && out[1]==0x2c203047 && out[8]==xy(sv[3]));
    scenery_packet(src,7,0,sv,out);
    assert(out[0]==0x08000000 && out[1]==0x38203043 && out[7]==0x203046);
    scenery_packet(src,1,0,sv,out); assert(out[0]==0x04000000 && out[4]==xy(sv[2]));
    /* Range can overflow after rotation even with all three input axes in range. */
    Camera rotated={0}; rotated.r[0]=2896; rotated.r[2]=-2896;
    rotated.r[4]=4096; rotated.r[6]=2896; rotated.r[8]=2896;
    int32_t rv[3]={28000,0,28000};
    Projected rp=project(rv,&rotated); assert(rp.valid && !rp.input_fits && rp.z>32767);
    /* One unsubmitted scenery model, merged into the far road packet list. */
    setup(1); camera_address=0x80060000;
    memcpy(memory(camera_address,18),camera.r,18);
    psx_mod_write_word(0x80094a80,0x80070000);
    psx_mod_write_word(0x80070000,0x80071000);
    psx_mod_write_word(0x80071010,4); psx_mod_write_word(0x80071014,0x80072000);
    psx_mod_write_word(0x80071020,1); psx_mod_write_word(0x80071024,0x80073000);
    psx_mod_write_word(0x80071030,0x80074000);
    memcpy(memory(0x80074000,18),camera.r,18);
    psx_mod_write_word(0x80074044,camera_address); psx_mod_write_word(0x8007401c,45000);
    const int16_t mesh[4][4]={{-1000,-1000,0,0},{1000,-1000,0,0},{-1000,1000,0,0},{1000,1000,0,0}};
    memcpy(memory(0x80072000,sizeof mesh),mesh,sizeof mesh);
    const uint16_t primitive[16]={4,0,0,1,2,3,0,0x1234,0x56,0x2211,0x4433,0x6655,0x8877,0,0x3456,0x12};
    memcpy(memory(0x80073000,sizeof primitive),primitive,sizeof primitive);
    CPUState scpu={0}; scpu.gte_ctrl[29]=341; scpu.gte_ctrl[30]=256;
    extend_track(&scpu,0);
    assert(psx_mod_read_word(stats+16)==2 && psx_mod_read_word(stats+68)==1 && psx_mod_read_word(stats+76)==0);
    assert(psx_mod_read_word(arena+PACKET_BYTES+4)==0x2c123456); /* Scenery sorted after road in allocation, before it in drawing. */
    assert(psx_mod_read_word(arena+PACKET_BYTES)==(0x09000000|(arena&0xffffff)));
    psx_mod_write_word(0x8007401c,20000); pending=1;
    scpu.gpr[4]=0x80071000; note_model(&scpu,0);
    extend_track(&scpu,0); assert(psx_mod_read_word(stats+68)==0); /* Original eligible geometry is not duplicated. */
    pending=1; extend_track(&scpu,0); assert(psx_mod_read_word(stats+68)==1); /* Missing section candidate is added. */
    psx_mod_write_word(0x8007401c,45000);
    psx_mod_write_word(0x80071020,4096); psx_mod_write_word(0x80071024,0x80120000);
    for (unsigned j=0;j<4096;++j) memcpy(memory(0x80120000+j*32,32),primitive,32);
    psx_mod_write_word(0x80010004,MAX_FACES);
    for (unsigned j=1;j<MAX_FACES;++j) memcpy(memory(0x80020000+j*20,20),memory(0x80020000,20),20);
    memset(dma_ram,0xa5,sizeof dma_ram); pending=1; extend_track(&scpu,0);
    assert(psx_mod_read_word(stats+16)==MAX_PACKETS && psx_mod_read_word(stats+68)==4096);
    for (unsigned j=MAX_PACKETS*PACKET_BYTES;j<sizeof dma_ram;++j) assert(dma_ram[j]==0xa5);
    psx_mod_write_word(0x80010004,1); psx_mod_write_word(0x80071020,1);
    psx_mod_write_word(0x80071024,0x80073000);
    psx_mod_write_word(0x80073000,9); pending=1; extend_track(&scpu,0);
    assert(psx_mod_read_word(stats+68)==0 && psx_mod_read_word(stats+76)==5);
    /* Billboard anchoring, texture layout and bounds use synthetic data. */
    psx_mod_write_word(0x8009207c,0x10000000); /* table[1023] = 4096 */
    psx_mod_write_word(0x80091880,0); /* table[0] = 0 */
    psx_mod_write_word(0x800ce468,0x80076000);
    psx_mod_write_word(0x80076000,0x00560000);
    psx_mod_write_word(0x80076004,0x00001234);
    for (unsigned j=0;j<8;++j) {
        uint16_t value=(uint16_t)(j+1); memcpy(memory(0x80076016+j*2,2),&value,2);
    }
    const uint16_t sprite[8]={11,4,0,4000,8000,0,0x3456,0x12};
    memcpy(memory(0x80077000,16),sprite,16);
    Projected center={160,120,40000,1,0}; uint32_t bw[10];
    assert(billboard_cos(0)==4096 && billboard_sin(0)==0);
    assert(billboard_cos(1024)==0 && billboard_sin(1024)==4096);
    assert(billboard_cos(-1024)==0 && billboard_sin(-1024)==-4096);
    assert(billboard_packet(0x80077000,11,center,0,53,bw));
    assert(bw[1]==0x2c123456 && bw[3]==0x12340201 && bw[5]==0x00560403);
    assert(bw[2]==((88u<<16)|152u) && bw[8]==((120u<<16)|168u));
    assert(billboard_packet(0x80077000,10,center,0,53,bw));
    assert(bw[2]==((120u<<16)|152u) && bw[8]==((152u<<16)|168u));
    center.z=2047; assert(!billboard_packet(0x80077000,11,center,0,53,bw));
    center.z=40000; center.x=1023; assert(!billboard_packet(0x80077000,11,center,0,53,bw));
    setup(1); stat(8,0); extend_track(NULL,0);
    assert(psx_mod_read_word(stats+12)==7 && psx_mod_read_word(0x80107ff8)==0x107ff4);
    setup(1); camera_error=2; extend_track(NULL,0);
    assert(psx_mod_read_word(stats+12)==2 && psx_mod_read_word(0x80107ff8)==0x107ff4);
    setup(1); psx_mod_write_word(0x80020000,0xffff); extend_track(NULL,0);
    assert(psx_mod_read_word(stats+12)==5 && psx_mod_read_word(0x80107ff8)==0x107ff4);
    setup(MAX_FACES); memset(dma_ram,0xa5,sizeof dma_ram); extend_track(NULL,0);
    assert(psx_mod_read_word(stats+16)==MAX_FACES);
    for (unsigned i=MAX_FACES*PACKET_BYTES;i<sizeof dma_ram;++i) assert(dma_ram[i]==0xa5);
    uint32_t next=psx_mod_read_word(0x80107ff8); unsigned count=0;
    while (next>=0xe00000) {
        assert(next<0xe00000+MAX_FACES*PACKET_BYTES && (next-0xe00000)%PACKET_BYTES==0);
        uint32_t tag=psx_mod_read_word(next); assert(tag>>24==9);
        next=tag&0xffffff; assert(++count<=MAX_FACES);
    }
    assert(count==MAX_FACES && next==0x107ff4);

    /* A distant moving ship shares the far road's saturated OT bucket. */
    setup(1); ship_active=1; camera_address=0x80060000;
    memcpy(memory(camera_address,18),camera.r,18);
    psx_mod_write_word(0x80094d2c,0x80078000);
    psx_mod_write_word(0x800a45b4,0x80071000);
    psx_mod_write_word(0x80078004,0x80079000);
    psx_mod_write_word(0x8007800c,0x20);
    psx_mod_write_word(0x80078048,50000);
    psx_mod_write_word(0x80071010,4);psx_mod_write_word(0x80071014,0x80072000);
    psx_mod_write_word(0x80071020,1);psx_mod_write_word(0x80071024,0x80073000);
    const int16_t shipv[4][4]={{-400,-400,0,0},{400,-400,0,0},{-400,400,0,0},{400,400,0,0}};
    memcpy(memory(0x80072000,sizeof shipv),shipv,sizeof shipv);
    memcpy(memory(0x80073000,sizeof primitive),primitive,sizeof primitive);
    *memory(0x8002000f,1)=1;
    psx_mod_write_word(0x8009207c,0x10000000);
    for(unsigned j=0;j<192;++j){uint16_t q=4096;memcpy(memory(0x80092094+j*2,2),&q,2);}
    scpu.gte_ctrl[29]=341;scpu.gte_ctrl[30]=256;
    extend_track(&scpu,0);
    assert(psx_mod_read_word(stats+88)==1 && psx_mod_read_word(stats+92)==0);
    assert(psx_mod_read_word(stats+16)==2);
    assert(psx_mod_read_word(0x80107ff8)==((arena+PACKET_BYTES)&0xffffff));
    assert((psx_mod_read_word(arena+PACKET_BYTES)&0xffffff)==(arena&0xffffff));
    assert(psx_mod_read_word(arena+PACKET_BYTES+4)==0x2c123456);
    psx_mod_write_word(0x80078048,65000);packet_count=0;
    append_distant_ships(&scpu,53,0x80020000,1);assert(packet_count==0);
    psx_mod_write_word(0x80078048,20000);
    append_distant_ships(&scpu,53,0x80020000,1);assert(packet_count==0);
    psx_mod_write_word(0x80078048,50000);packet_count=MAX_PACKETS;
    append_distant_ships(&scpu,53,0x80020000,1);
    assert(packet_count==MAX_PACKETS && psx_mod_read_word(stats+92)==5);
    packet_count=0;psx_mod_write_word(0x80073004,128);
    append_distant_ships(&scpu,53,0x80020000,1);
    assert(packet_count==0 && psx_mod_read_word(stats+92)==2);
    ship_active=0;append_distant_ships(&scpu,53,0x80020000,1);assert(packet_count==0);

    /* Exhaust history never writes guest state; same27-step fade and420/40 anchor. */
    setup(1);ship_active=1;camera_address=0x80060000;memcpy(memory(camera_address,18),camera.r,18);
    psx_mod_write_word(0x80094d2c,0x80078000);psx_mod_write_word(0x80094d34,0x80120000);
    psx_mod_write_word(0x80078048,50000);psx_mod_write_word(0x80078018,4096);
    psx_mod_write_word(0x80078020,4096);psx_mod_write_word(0x80078094,510);
    psx_mod_write_word(0x80120000,0x00204080);
    for(unsigned j=0;j<11;++j)psx_mod_write_word(0x801200cc+j*0xd0,0xe1000620);
    CPUState tcpu={0};tcpu.gpr[4]=0x80078000;
    static unsigned char before_trail[0x200000];memcpy(before_trail,main_ram,sizeof main_ram);
    capture_distant_trails(&tcpu,0);
    assert(memcmp(before_trail,main_ram,sizeof main_ram)==0);
    TrailHistory *th=&distant_trails[0];TrailPoint *tp=&th->p[th->head];
    assert(tp->valid && tp->left[0]==-40 && tp->left[1]==-15 && tp->left[2]==49580);
    assert(tp->rgb[0]==127 && tp->rgb[1]==63 && tp->rgb[2]==31);
    unsigned oldhead=th->head;capture_distant_trails(&tcpu,0);
    assert(th->p[oldhead].rgb[0]==100 && th->p[oldhead].rgb[1]==36 && th->p[oldhead].rgb[2]==4);
    assert(memcmp(before_trail,main_ram,sizeof main_ram)==0);
    Projected ta={100,110,50000,1,0},tb={90,100,51000,1,0};uint32_t tw[13];
    assert(trail_packet(ta,tb,0x2040ff,0x102080,0xe1000620,tw));
    assert(tw[0]==0x09000000 && tw[1]==0xe1000620 && tw[2]==0x3a2040ff);
    assert(tw[3]==(110u<<16|100u) && tw[5]==(110u<<16|102u));
    assert(tw[7]==(100u<<16|90u) && tw[9]==(100u<<16|92u));
    assert(trail_width16(23200)==2*65536);
    assert(trail_width16(46400)==65536);
    assert(trail_width16(64000)<65536 && trail_width16(64000)>0);
    for(int z=23201;z<=64000;++z)assert(trail_width16(z)<=trail_width16(z-1));
    TrailPoint precise_a={{-40,-15,50000},{0},1},precise_b={{-30,-10,51000},{0},1};
    FarPacket precise_packet={0};trail_precision(&precise_packet,&precise_a,&precise_b,ta,tb);
    assert(precise_packet.has_precise && precise_packet.precise[0]<160*65536);
    assert(precise_packet.precise[2]-precise_packet.precise[0]==trail_width16(50000));
    assert(precise_packet.precise[6]-precise_packet.precise[4]==trail_width16(51000));
    ta.x=1022;assert(!trail_packet(ta,tb,0,0,0xe1000620,tw));
    capture_distant_trails(&tcpu,0);
    for(unsigned j=0;j<192;++j){uint16_t q=4096;memcpy(memory(0x80092094+j*2,2),&q,2);}
    tcpu.gte_ctrl[28]=4096u<<12;packet_count=0;
    append_distant_trails(&tcpu,53);
    assert(packet_count==1 && psx_mod_read_word(stats+96)==1 && psx_mod_read_word(stats+100)==0);
    assert(packets[0].words[1]==0xe1000620 && packets[0].words[2]>>24==0x3a);
    packet_count=MAX_PACKETS;append_distant_trails(&tcpu,53);
    assert(packet_count==MAX_PACKETS && psx_mod_read_word(stats+100)==1);
    packet_count=0;psx_mod_write_word(0x801200cc+th->head*0xd0,0);
    append_distant_trails(&tcpu,53);assert(packet_count==0 && psx_mod_read_word(stats+100)==2);
    psx_mod_write_word(0x80094c44,3);packet_count=0;append_distant_trails(&tcpu,53);assert(packet_count==0);
    ship_active=0;
    assert(geometry_tags==0);
    /* History is consecutive-frame only, keyed to owner and mesh. */
    smoothing_active=1;wxl_far_geometry_reset();far_frame=1;
    Camera first={0},second={0};second.t[0]=100;
    assert(!far_previous_pose(0x80078000,0x80079000,4,&first));
    far_frame=2;
    const Camera* old=far_previous_pose(0x80078000,0x80079000,4,&second);
    assert(old && old->t[0]==0);
    far_frame=4;assert(!far_previous_pose(0x80078000,0x80079000,4,&second));
    far_frame=5;assert(!far_previous_pose(0x80078000,0x80079000,5,&second));
    wxl_far_geometry_reset();assert(!far_previous_pose(0x80078000,0x80079000,5,&second));
    smoothing_active=0;
    puts("PASS: projection bounds, disabled identity, error atomicity, FT4 encoding, backdrop preservation, full-capacity packet chain");
    return 0;
}
