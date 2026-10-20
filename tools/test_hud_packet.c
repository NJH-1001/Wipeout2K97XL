#include <assert.h>
#include <stdio.h>
#include "../psxrecomp/runtime/include/ws_hud_packet.h"
static WsHudPacket tags[WS_HUD_PACKET_SLOTS];
int main(void) {
    uint32_t words[9]={0x2c808080,20,0,40,0,60,0,80,0};
    ws_hud_packet_set(tags,0x80010004,words,-1);
    assert(ws_hud_packet_anchor(tags,0x10004,words)==-1);
    assert(ws_hud_packet_anchor(tags,0xa0010004,words)==-1);
    words[8]++;
    assert(!ws_hud_packet_anchor(tags,0x10004,words));
    words[8]--;
    ws_hud_packet_set(tags,0x80010004,words,0);
    assert(!ws_hud_packet_anchor(tags,0x10004,words));
    ws_hud_packet_set(tags,0x80010004,words,1);
    ws_hud_packet_set(tags,0x80012004,words,-1); /* collision retains both owners */
    assert(ws_hud_packet_anchor(tags,0x10004,words)==1);
    assert(ws_hud_packet_anchor(tags,0x12004,words)==-1);
    words[0]=0x20808080; /* world flat triangle cannot be tagged */
    ws_hud_packet_set(tags,0x12004,words,1);
    assert(!ws_hud_packet_anchor(tags,0x12004,words));
    ws_hud_packet_set(tags,0xffffffff,words,1);
    ws_hud_packet_set(tags,0x1ffffc,words,1);
    assert(!ws_hud_packet_anchor(tags,0xffffffff,words));
    puts("PASS: aliases, payload reuse, clear, collision, opcode and bounds");
}
