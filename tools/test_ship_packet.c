#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../src/ship_packet.h"
static unsigned char record[44];
uint16_t psx_mod_read_half(uint32_t a){uint16_t v;assert(a+2<=44);memcpy(&v,record+a,2);return v;}
uint32_t psx_mod_read_word(uint32_t a){uint32_t v;assert(a+4<=44);memcpy(&v,record+a,4);return v;}
static void put(unsigned a,uint32_t v){assert(a+4<=44);memcpy(record+a,&v,4);}
int main(void){
 const uint32_t xy[4]={0x00100020,0x00300040,0x00500060,0x00700080};
 const unsigned lengths[9]={0,5,8,6,10,7,10,9,13};
 const unsigned ops[9]={0,0x20,0x24,0x28,0x2c,0x30,0x34,0x38,0x3c};
 uint32_t out[13];
 for(unsigned t=1;t<=8;++t){
  for(unsigned flags=0;flags<8;++flags){
   memset(record,0,sizeof record);for(unsigned k=12;k<44;k+=4)put(k,0x7f000000u+k*0x010101u);
   unsigned n=wxl_ship_packet(0,t,flags,xy,0x00445566,out);
   assert(n==lengths[t] && (out[0]>>24)==n-1);
   assert(out[1]>>24==(ops[t]|((flags&4)?2u:0u)));
   for(unsigned k=n;k<13;++k)assert(out[k]==0);
  }
 }
 /* GT4: all colors, positions, UV/CLUT/TPAGE independently checked. */
 memset(record,0,sizeof record);
 put(14,0x56781234);put(18,0x22221111);put(22,0x44443333);
 put(28,0x00010203);put(32,0x00040506);put(36,0x00070809);put(40,0x000a0b0c);
 const uint32_t expected[13]={0x0c000000,0x3c010203,0x00100020,0x12341111,0x00040506,0x00300040,0x56782222,0x00070809,0x00500060,0x00003333,0x000a0b0c,0x00700080,0x00004444};
 assert(wxl_ship_packet(0,8,0,xy,0,out)==13 && memcmp(out,expected,sizeof out)==0);
 assert(wxl_ship_packet(0,9,0,xy,0,out)==0);
 puts("PASS: all eight ship packet sizes/opcodes, transparency and exact GT4 fields");
}
