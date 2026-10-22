#include "../psxrecomp/runtime/include/geometry_packet_tag.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

int main(void) {
    GeometryPacketTag* tags=calloc(GEOMETRY_TAG_CAPACITY,sizeof(*tags));
    assert(tags);
    uint32_t payload[12]={0x2c808080u,1,2,3,4,5,6,7,8};
    GeometryIdentity a={0x80123400u,0x80123800u,7},out={0};
    assert(geometry_tag_set(tags,0x80011004u,a,payload,9));
    assert(geometry_tag_get(tags,0xa0011004u,payload,9,&out));
    assert(!memcmp(&a,&out,sizeof(a)));
    GeometryPacketTag* tag=(GeometryPacketTag*)geometry_tag_find(tags,0x80011004u,payload,9);
    assert(tag && !tag->previous_mask);
    tag->previous_mask=0xAAu;tag->previous[1][0]=123456;
    const GeometryPacketTag* alias=geometry_tag_find(tags,0xa0011004u,payload,9);
    assert(alias && alias->previous_mask==0xAAu && alias->previous[1][0]==123456);
    payload[8]^=1;
    assert(!geometry_tag_get(tags,0x80011004u,payload,9,&out));
    assert(!geometry_tag_find(tags,0x80011004u,payload,9));
    payload[8]^=1;
    assert(!geometry_tag_get(tags,0x80011004u,payload,8,&out));
    assert(!geometry_tag_set(tags,0x80011005u,a,payload,9));
    assert(!geometry_tag_set(tags,0x80011004u,a,payload,13));
    /* Expansion aperture addresses must never alias folded ordinary RAM. */
    GeometryIdentity far=a; far.face+=4;
    assert(geometry_tag_set(tags,0x80e11004u,far,payload,9));
    assert(geometry_tag_get(tags,0x80e11004u,payload,9,&out) && out.face==far.face);
    assert(geometry_tag_get(tags,0x80011004u,payload,9,&out) && out.face==a.face);
    /* Relocating a face to another temporary packet retains its identity. */
    assert(geometry_tag_set(tags,0x80022004u,a,payload,9));
    assert(geometry_tag_get(tags,0x80022004u,payload,9,&out) && out.face==a.face);
    /* Reusing that storage for another face replaces the identity. */
    assert(geometry_tag_set(tags,0x80022004u,far,payload,9));
    assert(!geometry_tag_find(tags,0x80022004u,payload,9)->previous_mask);
    assert(geometry_tag_get(tags,0x80022004u,payload,9,&out) && out.face==far.face);
    GeometryPacketTag consumed={0};
    assert(geometry_tag_consume(tags,0x80022004u,payload,9,&consumed));
    assert(consumed.identity.face==far.face);
    assert(!geometry_tag_consume(tags,0x80022004u,payload,9,&consumed));
    assert(!geometry_tag_get(tags,0x80022004u,payload,9,&out));
    assert(geometry_tag_set(tags,0x80022004u,a,payload,9));
    assert(geometry_tag_get(tags,0x80022004u,payload,9,&out) && out.face==a.face);
    memset(tags,0,GEOMETRY_TAG_CAPACITY*sizeof(*tags));
    uint32_t addresses[GEOMETRY_TAG_PROBES+1],count=0;
    for(uint32_t addr=4;count<GEOMETRY_TAG_PROBES+1 && addr<0x4000000;addr+=4)
        if(geometry_tag_hash(addr)==0) addresses[count++]=addr;
    assert(count==GEOMETRY_TAG_PROBES+1);
    for(uint32_t n=0;n<GEOMETRY_TAG_PROBES;++n)
        assert(geometry_tag_set(tags,addresses[n],a,payload,9));
    assert(!geometry_tag_set(tags,addresses[GEOMETRY_TAG_PROBES],a,payload,9));
    for(uint32_t n=0;n<GEOMETRY_TAG_PROBES;++n)
        assert(geometry_tag_get(tags,addresses[n],payload,9,&out));
    /* A triangle currently beyond any edge may have an intermediate visible
     * position. Keep it for the auxiliary pass, but reject a fully outside sweep. */
    for(unsigned edge=0;edge<4;++edge) {
        int64_t now[3][2]={{100*65536,100*65536},{101*65536,100*65536},{100*65536,101*65536}};
        int64_t old[3][2];memcpy(old,now,sizeof(old));
        for(unsigned v=0;v<3;++v)now[v][edge/2]=(edge%2?400:-100)*65536;
        assert(geometry_swept_intersects(now,old,3,0,319,0,239));
        memcpy(old,now,sizeof(old));
        assert(!geometry_swept_intersects(now,old,3,0,319,0,239));
    }
    int64_t edge[1][2]={{-1,0}},at[1][2]={{0,0}};
    assert(geometry_swept_intersects(edge,at,1,0,319,0,239));
    assert(!geometry_swept_intersects(edge,edge,1,0,319,0,239));
    at[0][0]=319*65536;at[0][1]=495*65536;
    assert(geometry_swept_intersects(at,at,1,-53,372,256,495));
    assert(!geometry_swept_intersects(at,at,1,-53,372,0,239));
    free(tags);
    puts("PASS: payload guards, aliases, aperture separation, reuse and collision saturation");
    return 0;
}
