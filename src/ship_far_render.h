/* Included by the road/scenery collector so all far packets share depth order.
 * Geometry and material layouts: Ghidra 80021B90,8001E4FC,8001E8A8,80011DE8.
 * No original executable/data edits; transforms use current ship state. */
#include "ship_packet.h"
extern int wxl_ship_distance_active(void);
static int32_t ship_mul(int32_t a,int32_t b) {return (int32_t)floor_div((int64_t)a*b,4096);}
static void ship_rotation(uint32_t ship,int16_t r[9]) {
    int a=(int16_t)psx_mod_read_half(ship+0x70),b=(int16_t)psx_mod_read_half(ship+0x72),c=(int16_t)psx_mod_read_half(ship+0x74);
    int s= billboard_sin(a),t=billboard_sin(b),u=billboard_sin(c);
    int x=billboard_cos(a),y=billboard_cos(b),z=billboard_cos(c);
    int su=ship_mul(s,u),xt=ship_mul(x,t);
    r[0]=(int16_t)(ship_mul(x,z)+ship_mul(su,t));
    r[1]=(int16_t)(ship_mul(x,u)-ship_mul(ship_mul(s,t),z));
    r[2]=(int16_t)ship_mul(-s,y);r[3]=(int16_t)ship_mul(-u,y);
    r[4]=(int16_t)ship_mul(y,z);r[5]=(int16_t)-t;
    r[6]=(int16_t)(ship_mul(s,z)-ship_mul(xt,u));
    r[7]=(int16_t)(su+ship_mul(xt,z));r[8]=(int16_t)ship_mul(y,x);
}
static int ship_metric(uint32_t ship) {
    uint64_t n=0;
    for(unsigned k=0;k<3;++k){
        int64_t d=(int64_t)(int32_t)psx_mod_read_word(ship+64+k*4)+camera.offset[k];
        d=floor_div(d,8);if(d < -32767 || d>32767)return -1;
        n+=(uint64_t)(d*d);
    }
    if(n>INT32_MAX)return -1;
    if(!n)return 0;
    unsigned bits=0;for(uint32_t v=(uint32_t)n;v;v>>=1)++bits;
    unsigned lz=(32u-bits)&~1u;
    unsigned norm=lz<=24u?(unsigned)n>>(24u-lz):(unsigned)n<<(lz-24u);
    if(norm<64u || norm>255u)return -1;
    uint32_t v=psx_mod_read_half(0x80092094u+(norm-64u)*2u);
    return (int)((v<<((31u-lz)/2u))>>12);
}
static void append_distant_ships(CPUState *cpu,int margin,uint32_t fp,uint32_t nf) {
    static const unsigned sizes[9]={0,16,28,16,32,24,36,28,44};
    unsigned start=packet_count,added=0,error=0;
    stat(22,0);stat(23,0);
    if(!wxl_ship_distance_active())return;
    /* The far path currently covers ordinary races. Ghost recoloring and
     * split-screen cameras require independent evidence before expansion. */
    unsigned mode=psx_mod_read_word(0x80094c44u);
    if(mode==1 || mode==3){stat(23,1);return;}
    uint32_t ships=psx_mod_read_word(0x80094d2cu);
    if(!ram(ships,15u*240u)){stat(23,2);return;}
    for(unsigned j=0;j<9;++j)
        if((int16_t)psx_mod_read_half(camera_address+j*2)!=(j%4==0?4096:0)){stat(23,3);return;}
    for(unsigned i=0;i<15;++i){
        uint32_t ship=ships+i*240u;int metric=ship_metric(ship);
        if(metric<2900)continue; /* Native renderer owns the entire near range. */
        unsigned flags=psx_mod_read_word(ship+12),rf=psx_mod_read_half(ship+172);
        if((flags&6u)==4u || ((rf&0x8000u) && (int32_t)psx_mod_read_word(0x800d7f30u+i*4)<=10))continue;
        int mi=(int16_t)psx_mod_read_half(ship);
        if(mi<0 || mi>=15){error=2;break;}
        uint32_t model=psx_mod_read_word(0x800a45b4u+(unsigned)mi*4);
        if(!ram(model,64)){error=2;break;}
        unsigned nv=psx_mod_read_half(model+16),np=psx_mod_read_half(model+32);
        uint32_t vp=psx_mod_read_word(model+20),src=psx_mod_read_word(model+36);
        if(!nv || nv>128 || !np || np>128 || !ram(vp,nv*8)){error=2;break;}
        int16_t local[9];ship_rotation(ship,local);Camera transform={0};
        for(unsigned row=0;row<3;++row){
            int64_t a=0,b=0;
            for(unsigned k=0;k<3;++k){a+=(int64_t)camera.r[row*3+k]*camera.offset[k];b+=(int64_t)camera.r[row*3+k]*(int32_t)psx_mod_read_word(ship+64+k*4);}
            int64_t tr=(int64_t)camera.t[row]+floor_div(a,4096)+floor_div(b,4096);
            if(tr<INT32_MIN || tr>INT32_MAX){error=3;break;}
            transform.t[row]=(int32_t)tr;
            for(unsigned col=0;col<3;++col){
                int64_t v=0;for(unsigned k=0;k<3;++k)v+=(int64_t)camera.r[row*3+k]*local[k*3+col];
                v=floor_div(v,4096);if(!fits16(v)){error=3;break;}
                transform.r[row*3+col]=(int16_t)v;
            }
        }
        if(error)break;
        if(transform.t[2]<160 || transform.t[2]>FAR_Z+1024)continue;
        uint32_t section=psx_mod_read_word(ship+4);
        if(!ram(section,0x9c)){error=2;break;}
        int fi=(int16_t)psx_mod_read_half(section+0x8e);
        if(fi<0){error=4;break;}
        while((unsigned)fi<nf && !(psx_mod_read_byte(fp+(unsigned)fi*20+15)&1u))++fi;
        if(!(flags&0x20u))++fi;
        if((unsigned)fi>=nf){error=4;break;}
        uint32_t ambient=psx_mod_read_word(fp+(unsigned)fi*20+16);
        Projected pv[128];
        for(unsigned j=0;j<nv;++j){
            int32_t v[3];for(unsigned k=0;k<3;++k){v[k]=(int16_t)psx_mod_read_half(vp+j*8+k*2);if(v[k]<-512 || v[k]>512)error=2;}
            pv[j]=project(v,&transform);
        }
        if(error)break;
        for(unsigned j=0;j<np;++j){
            if(!ram(src,4)){error=2;break;}
            unsigned type=psx_mod_read_half(src),pf=psx_mod_read_half(src+2);
            if(type<1 || type>8 || !ram(src,sizes[type])){error=2;break;}
            unsigned n=(type==1 || type==2 || type==5 || type==6)?3:4,good=1;
            Projected v[4];uint32_t coords[4]={0};int sumz=0,minx=1024,maxx=-1024,miny=1024,maxy=-1024;
            for(unsigned k=0;k<n;++k){
                unsigned vi=psx_mod_read_half(src+4+k*2);if(vi>=nv){error=2;break;}
                v[k]=pv[vi];good&=v[k].valid;coords[k]=xy(v[k]);sumz+=v[k].z;
                if(v[k].x<minx)minx=v[k].x;
                if(v[k].x>maxx)maxx=v[k].x;
                if(v[k].y<miny)miny=v[k].y;
                if(v[k].y>maxy)maxy=v[k].y;
            }
            if(error)break;
            if(good && maxx>=-margin && minx<=319+margin && maxy>=0 && miny<=239){
                int64_t cross=(int64_t)(v[1].x-v[0].x)*(v[2].y-v[0].y)-(int64_t)(v[1].y-v[0].y)*(v[2].x-v[0].x);
                if(!(pf&1u) || cross>0){
                    if(packet_count>=MAX_PACKETS){error=5;break;}
                    int zsf=(int16_t)cpu->gte_ctrl[n==3?29:30];if(zsf<=0){error=3;break;}
                    FarPacket *p=&packets[packet_count++];wxl_ship_packet(src,type,pf,coords,ambient,p->words);
                    int64_t z=floor_div((int64_t)sumz*zsf,1024)-480;
                    p->z=z>0?(int32_t)z:0;++added;
                }
            }
            src+=sizes[type];
        }
        if(error)break;
    }
    if(error){packet_count=start;added=0;}
    stat(22,added);stat(23,error);
}
