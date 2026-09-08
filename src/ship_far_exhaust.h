/* Distant exhaust: original layouts726AC/70EB0 and far ribbon71418.
 * Shadow history is presentation-only: no guest visibility or RNG mutation. */
typedef struct { int32_t left[3]; uint16_t rgb[3]; unsigned valid; } TrailPoint;
typedef struct { TrailPoint p[11]; unsigned head,ready,sequence; } TrailHistory;
static TrailHistory distant_trails[15];
static uint32_t trail_source,trail_ships;
static void capture_distant_trails(CPUState *cpu,uint32_t address) {
    (void)address;
    if(!wxl_ship_distance_active() || !cpu)return;
    uint32_t ships=psx_mod_read_word(0x80094d2cu),source=psx_mod_read_word(0x80094d34u);
    if(cpu->gpr[4]!=ships || !ram(ships,3600) || !ram(source,15u*0x900u))return;
    if(source!=trail_source || ships!=trail_ships){memset(distant_trails,0,sizeof distant_trails);trail_source=source;trail_ships=ships;}
    for(unsigned i=0;i<15;++i){
        uint32_t ship=ships+i*240u,ring=source+i*0x900u;
        TrailHistory *h=&distant_trails[i];
        if(psx_mod_read_word(ship+12)&0x200u){
            unsigned head=psx_mod_read_word(ring+12);
            if(head>=11){h->ready=0;continue;}
            h->head=head;h->ready=1;
            for(unsigned j=0;j<11;++j){
                uint32_t r=ring+16+j*0xd0u;TrailPoint *p=&h->p[j];p->valid=psx_mod_read_byte(r)!=0;
                for(unsigned k=0;k<3;++k){p->left[k]=(int32_t)psx_mod_read_word(r+8+k*4);p->rgb[k]=psx_mod_read_half(r+2+k*2);}
            }
            continue;
        }
        if(!h->ready){memset(h,0,sizeof *h);h->ready=1;}
        for(unsigned j=0;j<11;++j)for(unsigned k=0;k<3;++k)
            h->p[j].rgb[k]=h->p[j].rgb[k]>27?h->p[j].rgb[k]-27:0;
        h->head=(h->head+10u)%11u;
        TrailPoint *p=&h->p[h->head];p->valid=1;
        unsigned speed=psx_mod_read_half(ship+0x94)>>1;if(speed>255)speed=255;
        /* Same two guest palettes and1/8 accent frequency. A separate visual
         * sequence avoids consuming gameplay RNG for newly visible ships. */
        unsigned accent=((++h->sequence+i*3u)&7u)==0?3u:0u;
        for(unsigned k=0;k<3;++k){
            int64_t v=(int32_t)psx_mod_read_word(ship+64+k*4);
            v-=floor_div((int64_t)(int32_t)psx_mod_read_word(ship+16+k*4)*420,4096);
            if(k==1)v-=15;
            v-=floor_div((int64_t)(int32_t)psx_mod_read_word(ship+32+k*4)*40,4096);
            if(v<INT32_MIN || v>INT32_MAX){p->valid=0;break;}
            p->left[k]=(int32_t)v;
            p->rgb[k]=(uint16_t)((psx_mod_read_byte(ring+accent+k)*speed)>>8);
        }
    }
}
static uint32_t trail_color(const TrailPoint *p){return (p->rgb[0]&255u)|((p->rgb[1]&255u)<<8)|((p->rgb[2]&255u)<<16);}
static int trail_packet(Projected a,Projected b,uint32_t ca,uint32_t cb,uint32_t state,uint32_t out[13]) {
    if(!a.valid || !b.valid || a.x>1021 || b.x>1021)return 0;
    memset(out,0,13*sizeof *out);out[0]=0x09000000u;out[1]=state;
    out[2]=0x3a000000u|ca;out[3]=xy(a);out[4]=ca;a.x+=2;out[5]=xy(a);
    out[6]=cb;out[7]=xy(b);out[8]=cb;b.x+=2;out[9]=xy(b);return 1;
}
/* Enhancement width matches the original two pixels at the extended near
 * handoff (metric2900 *8 world units), then falls with perspective depth.
 * Keep the packet's integer envelope for canonical/native-resolution rendering;
 * the high-resolution renderer consumes these explicit fractional coordinates. */
static int32_t trail_width16(int32_t z) {
    return (int32_t)((int64_t)2*23200*65536/(z>23200?z:23200));
}
static void trail_precise_point(const int32_t world[3],int32_t out[2]) {
    int64_t eye[3];
    for(unsigned j=0;j<3;++j) {
        int64_t n=(int64_t)camera.t[j]*4096;
        for(unsigned k=0;k<3;++k)n+=(int64_t)camera.r[j*3+k]*((int64_t)world[k]+camera.offset[k]);
        eye[j]=floor_div(n,4096);
    }
    out[0]=(int32_t)(160*65536+floor_div(160*65536*eye[0],eye[2]));
    out[1]=(int32_t)(120*65536+floor_div(160*65536*eye[1],eye[2]));
}
static void trail_precision(FarPacket *p,const TrailPoint *a,const TrailPoint *b,Projected pa,Projected pb) {
    trail_precise_point(a->left,p->precise);
    trail_precise_point(b->left,p->precise+4);
    p->precise[2]=p->precise[0]+trail_width16(pa.z);p->precise[3]=p->precise[1];
    p->precise[6]=p->precise[4]+trail_width16(pb.z);p->precise[7]=p->precise[5];
    p->has_precise=1;
}
static void append_distant_trails(CPUState *cpu,int margin) {
    unsigned start=packet_count,error=0;stat(24,0);stat(25,0);
    if(!wxl_ship_distance_active() || psx_mod_read_byte(0x80093e30u))return;
    unsigned mode=psx_mod_read_word(0x80094c44u);if(mode==1 || mode==3)return;
    uint32_t ships=psx_mod_read_word(0x80094d2cu),source=psx_mod_read_word(0x80094d34u);
    if(ships!=trail_ships || source!=trail_source || !ram(source,15u*0x900u))return;
    for(unsigned i=0;i<15;++i){
        uint32_t ship=ships+i*240u;TrailHistory *h=&distant_trails[i];
        if(!h->ready || ship_metric(ship)<2900 || (psx_mod_read_half(ship+172)&0x8000u))continue;
        int32_t pos[3];for(unsigned k=0;k<3;++k)pos[k]=(int32_t)psx_mod_read_word(ship+64+k*4);
        Projected center=project(pos,&camera);if(!center.valid)continue;
        int64_t ratio=((int64_t)160<<16)/center.z;
        int64_t cue=floor_div((int64_t)(int32_t)cpu->gte_ctrl[28]+(int16_t)cpu->gte_ctrl[27]*ratio,4096);
        unsigned last=(h->head+10u)%11u,last2=(last+10u)%11u;
        for(unsigned j=h->head&1u;j<11;j+=2){
            if(!((j!=last && j!=last2 && cue<4000) || j==h->head))continue;
            unsigned next=(j+2)%11u;TrailPoint *a=&h->p[j],*b=&h->p[next];
            if(!a->valid || !b->valid || !(trail_color(a)|trail_color(b)))continue;
            Projected pa=project(a->left,&camera),pb=project(b->left,&camera);
            if(!pa.valid || !pb.valid || (pa.x< -margin-2 && pb.x< -margin-2) ||
               (pa.x>319+margin && pb.x>319+margin) || (pa.y<0 && pb.y<0) || (pa.y>239 && pb.y>239))continue;
            uint32_t state=psx_mod_read_word(source+i*0x900u+j*0xd0u+0xcc);
            if(state!=0xe1000620u){error=2;break;}
            if(packet_count>=MAX_PACKETS){error=1;break;}
            FarPacket *p=&packets[packet_count];
            if(!trail_packet(pa,pb,trail_color(a),trail_color(b),state,p->words))continue;
            trail_precision(p,a,b,pa,pb);
            /* Original71418 uses(RTPS last SZ/4 -160)/2 as its OT slot. */
            p->z=pb.z>640?((pb.z/4-160)/2)*4:0;++packet_count;
        }
        if(error)break;
    }
    if(error)packet_count=start;
    stat(24,packet_count-start);stat(25,error);
}
