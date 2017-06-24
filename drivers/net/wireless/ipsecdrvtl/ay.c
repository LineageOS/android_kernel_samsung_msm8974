/*
   'aes.c' Obfuscated by COBF (Version 1.06 2006-01-07 by BB) at Fri Nov 13 10:03:51 2015
*/
#include"cobf.h"
#ifdef _WIN32
#if defined( UNDER_CE) && defined( bb342) || ! defined( bb341)
#define bb343 1
#define bb355 1
#else
#define bb353 bb345
#define bb347 1
#define bb357 1
#endif
#define bb361 1
#include"uncobf.h"
#include<ndis.h>
#include"cobf.h"
#ifdef UNDER_CE
#include"uncobf.h"
#include<ndiswan.h>
#include"cobf.h"
#endif
#include"uncobf.h"
#include<stdio.h>
#include<basetsd.h>
#include"cobf.h"
bba bbt bbl bbf, *bb3;bba bbt bbe bbn, *bb81;bba bb137 bb125, *bb351;
bba bbt bbl bb41, *bb73;bba bbt bb137 bbk, *bb59;bba bbe bbu, *bb134;
bba bbh bbf*bb79;
#ifdef bb308
bba bbd bb60, *bb124;
#endif
#else
#include"uncobf.h"
#include<linux/module.h>
#include<linux/ctype.h>
#include<linux/time.h>
#include<linux/slab.h>
#include"cobf.h"
#ifndef bb117
#define bb117
#ifdef _WIN32
#include"uncobf.h"
#include<wtypes.h>
#include"cobf.h"
#else
#ifdef bb121
#include"uncobf.h"
#include<linux/types.h>
#include"cobf.h"
#else
#include"uncobf.h"
#include<stddef.h>
#include<sys/types.h>
#include"cobf.h"
#endif
#endif
#ifdef _WIN32
#ifdef _MSC_VER
bba bb113 bb242;
#endif
#else
bba bbe bbu, *bb134, *bb252;
#define bb203 1
#define bb202 0
bba bb219 bb238, *bb253, *bb270;bba bbe bb237, *bb286, *bb279;bba bbt
bbn, *bb81, *bb277;bba bb8 bb220, *bb233;bba bbt bb8 bb258, *bb231;
bba bb8 bb111, *bb250;bba bbt bb8 bb63, *bb226;bba bb63 bb257, *bb276
;bba bb63 bb208, *bb271;bba bb111 bb113, *bb259;bba bb249 bb285;bba
bb267 bb125;bba bb224 bb85;bba bb119 bb112;bba bb119 bb288;
#ifdef bb255
bba bb236 bb41, *bb73;bba bb254 bbk, *bb59;bba bb278 bbd, *bb31;bba
bb230 bb57, *bb114;
#else
bba bb248 bb41, *bb73;bba bb240 bbk, *bb59;bba bb264 bbd, *bb31;bba
bb234 bb57, *bb114;
#endif
bba bb41 bbf, *bb3, *bb213;bba bbk bb212, *bb247, *bb251;bba bbk bb256
, *bb223, *bb262;bba bbd bb60, *bb124, *bb205;bba bb85 bb39, *bb260, *
bb218;bba bbd bb209, *bb269, *bb221;bba bb112 bb214, *bb273, *bb283;
bba bb57 bb275, *bb274, *bb210;
#define bb143 bbb
bba bbb*bb241, *bb80;bba bbh bbb*bb243;bba bbl bb284;bba bbl*bb229;
bba bbh bbl*bb62;
#if defined( bb121)
bba bbe bb116;
#endif
bba bb116 bb19;bba bb19*bb265;bba bbh bb19*bb187;
#if defined( bb228) || defined( bb211)
bba bb19 bb38;bba bb19 bb115;
#else
bba bbl bb38;bba bbt bbl bb115;
#endif
bba bbh bb38*bb232;bba bb38*bb206;bba bb60 bb263, *bb225;bba bbb*
bb107;bba bb107*bb245;
#define bb227( bb36) bbj bb36##__ { bbe bb287; }; bba bbj bb36##__  * \
 bb36
bba bbj{bb39 bb185,bb244,bb207,bb239;}bb266, *bb272, *bb289;bba bbj{
bb39 bb10,bb177;}bb261, *bb246, *bb235;bba bbj{bb39 bb215,bb281;}
bb280, *bb217, *bb282;
#endif
bba bbh bbf*bb79;
#endif
bba bbf bb103;
#define IN
#define OUT
#ifdef _DEBUG
#define bb146( bbc) bb27( bbc)
#else
#define bb146( bbc) ( bbb)( bbc)
#endif
bba bbe bb160, *bb172;
#define bb294 0
#define bb316 1
#define bb300 2
#define bb323 3
#define bb352 4
bba bbe bb349;bba bbb*bb123;
#endif
#ifdef _WIN32
#ifndef UNDER_CE
#define bb32 bb356
#define bb43 bb334
bba bbt bb8 bb32;bba bb8 bb43;
#endif
#else
#endif
#ifdef _WIN32
bbb*bb128(bb32 bb48);bbb bb109(bbb* );bbb*bb138(bb32 bb159,bb32 bb48);
#else
#define bb128( bbc) bb147(1, bbc, bb142)
#define bb109( bbc) bb346( bbc)
#define bb138( bbc, bbp) bb147( bbc, bbp, bb142)
#endif
#ifdef _WIN32
#define bb27( bbc) bb344( bbc)
#else
#ifdef _DEBUG
bbe bb145(bbh bbl*bb99,bbh bbl*bb26,bbt bb216);
#define bb27( bbc) ( bbb)(( bbc) || ( bb145(# bbc, __FILE__, __LINE__ \
)))
#else
#define bb27( bbc) (( bbb)0)
#endif
#endif
bb43 bb305(bb43*bb325);
#ifndef _WIN32
bbe bb331(bbh bbl*bbg);bbe bb320(bbh bbl*bb20,...);
#endif
#ifdef _WIN32
bba bb336 bb95;
#define bb141( bbc) bb360( bbc)
#define bb144( bbc) bb348( bbc)
#define bb135( bbc) bb354( bbc)
#define bb133( bbc) bb359( bbc)
#else
bba bb335 bb95;
#define bb141( bbc) ( bbb)(  *  bbc = bb337( bbc))
#define bb144( bbc) (( bbb)0)
#define bb135( bbc) bb338( bbc)
#define bb133( bbc) bb339( bbc)
#endif
#ifdef __cplusplus
bbr"\x43"{
#endif
bba bbj{bbn bb434;bbd bb368[4 * (14 +1 )];}bb204;bbb bb1073(bb204*bbi,
bbh bbb*bb30,bbn bb100);bbb bb1436(bb204*bbi,bbh bbb*bb30,bbn bb100);
bbb bb1022(bb204*bbi,bbb*bb1,bbh bbb*bbx);bbb bb1636(bb204*bbi,bbb*
bb1,bbh bbb*bbx);
#ifdef __cplusplus
}
#endif
bb40 bbf bb416[256 ]={0x63 ,0x7c ,0x77 ,0x7b ,0xf2 ,0x6b ,0x6f ,0xc5 ,0x30 ,
0x01 ,0x67 ,0x2b ,0xfe ,0xd7 ,0xab ,0x76 ,0xca ,0x82 ,0xc9 ,0x7d ,0xfa ,0x59 ,0x47
,0xf0 ,0xad ,0xd4 ,0xa2 ,0xaf ,0x9c ,0xa4 ,0x72 ,0xc0 ,0xb7 ,0xfd ,0x93 ,0x26 ,
0x36 ,0x3f ,0xf7 ,0xcc ,0x34 ,0xa5 ,0xe5 ,0xf1 ,0x71 ,0xd8 ,0x31 ,0x15 ,0x04 ,0xc7
,0x23 ,0xc3 ,0x18 ,0x96 ,0x05 ,0x9a ,0x07 ,0x12 ,0x80 ,0xe2 ,0xeb ,0x27 ,0xb2 ,
0x75 ,0x09 ,0x83 ,0x2c ,0x1a ,0x1b ,0x6e ,0x5a ,0xa0 ,0x52 ,0x3b ,0xd6 ,0xb3 ,0x29
,0xe3 ,0x2f ,0x84 ,0x53 ,0xd1 ,0x00 ,0xed ,0x20 ,0xfc ,0xb1 ,0x5b ,0x6a ,0xcb ,
0xbe ,0x39 ,0x4a ,0x4c ,0x58 ,0xcf ,0xd0 ,0xef ,0xaa ,0xfb ,0x43 ,0x4d ,0x33 ,0x85
,0x45 ,0xf9 ,0x02 ,0x7f ,0x50 ,0x3c ,0x9f ,0xa8 ,0x51 ,0xa3 ,0x40 ,0x8f ,0x92 ,
0x9d ,0x38 ,0xf5 ,0xbc ,0xb6 ,0xda ,0x21 ,0x10 ,0xff ,0xf3 ,0xd2 ,0xcd ,0x0c ,0x13
,0xec ,0x5f ,0x97 ,0x44 ,0x17 ,0xc4 ,0xa7 ,0x7e ,0x3d ,0x64 ,0x5d ,0x19 ,0x73 ,
0x60 ,0x81 ,0x4f ,0xdc ,0x22 ,0x2a ,0x90 ,0x88 ,0x46 ,0xee ,0xb8 ,0x14 ,0xde ,0x5e
,0x0b ,0xdb ,0xe0 ,0x32 ,0x3a ,0x0a ,0x49 ,0x06 ,0x24 ,0x5c ,0xc2 ,0xd3 ,0xac ,
0x62 ,0x91 ,0x95 ,0xe4 ,0x79 ,0xe7 ,0xc8 ,0x37 ,0x6d ,0x8d ,0xd5 ,0x4e ,0xa9 ,0x6c
,0x56 ,0xf4 ,0xea ,0x65 ,0x7a ,0xae ,0x08 ,0xba ,0x78 ,0x25 ,0x2e ,0x1c ,0xa6 ,
0xb4 ,0xc6 ,0xe8 ,0xdd ,0x74 ,0x1f ,0x4b ,0xbd ,0x8b ,0x8a ,0x70 ,0x3e ,0xb5 ,0x66
,0x48 ,0x03 ,0xf6 ,0x0e ,0x61 ,0x35 ,0x57 ,0xb9 ,0x86 ,0xc1 ,0x1d ,0x9e ,0xe1 ,
0xf8 ,0x98 ,0x11 ,0x69 ,0xd9 ,0x8e ,0x94 ,0x9b ,0x1e ,0x87 ,0xe9 ,0xce ,0x55 ,0x28
,0xdf ,0x8c ,0xa1 ,0x89 ,0x0d ,0xbf ,0xe6 ,0x42 ,0x68 ,0x41 ,0x99 ,0x2d ,0x0f ,
0xb0 ,0x54 ,0xbb ,0x16 };bb40 bbn bb2155(bbn bb448){bb448<<=1 ;bbm(bb448&
0x0100 )bb448^=0x011b ;bb4 bb448;}bb40 bbd bb2148[256 ],bb2146[256 ],
bb2147[256 ],bb2145[256 ];bb40 bbf bb1031[256 ];bb40 bbd bb1803[256 ],
bb1802[256 ],bb1801[256 ],bb1800[256 ];bb40 bbb bb2122(){bbn bbz;bb91(
bbz=0 ;bbz<256 ;bbz++){bbn bb77=bb416[bbz];{bbn bb1883=bb2155(bb77),
bb2620=bb1883^bb77;bbn bb47=bb1883<<24 |bb77<<16 |bb77<<8 |bb2620;bb2148
[bbz]=bb47;bb2146[bbz]=((bb47)>>(8 )|(bb47)<<(32 -8 ));bb2147[bbz]=((
bb47)>>(16 )|(bb47)<<(32 -16 ));bb2145[bbz]=((bb47)>>(24 )|(bb47)<<(32 -24
));}bb1031[bb77]=bbz;{bbn bb2368=bb2155(bbz),bb2367=bb2155(bb2368),
bb2260=bb2155(bb2367),bb2621=bb2260^bbz,bb2578=bb2260^bb2368^bbz,
bb2577=bb2260^bb2367^bbz,bb2576=bb2260^bb2367^bb2368;bbn bb47=bb2576
<<24 |bb2621<<16 |bb2577<<8 |bb2578;bb1803[bb77]=bb47;bb1802[bb77]=((
bb47)>>(8 )|(bb47)<<(32 -8 ));bb1801[bb77]=((bb47)>>(16 )|(bb47)<<(32 -16 ));
bb1800[bb77]=((bb47)>>(24 )|(bb47)<<(32 -24 ));}}}bbb bb1073(bb204*bbi,
bbh bbb*bb30,bbn bb100){bbn bb1286,bb434,bbz;bb31 bb6=bbi->bb368;bb40
bbu bb1868=1 ;bbm(bb1868){bb2122();bb1868=0 ;}bb27(bb100==16 ||bb100==24
||bb100==32 );bb1286=bb100/4 ;bbi->bb434=bb434=bb1286+6 ;bb91(bbz=0 ;bbz<
bb1286;bbz++) *bb6++=(((bb3)((bb31)bb30+bbz))[3 ]|((bb3)((bb31)bb30+
bbz))[2 ]<<8 |((bb3)((bb31)bb30+bbz))[1 ]<<16 |((bb3)((bb31)bb30+bbz))[0 ]
<<24 );{bbn bbo=1 ;bb91(;bbz<4 * (bb434+1 );bbz++){bbd bb47= * (bb6-1 );
bbm(bbz%bb1286==0 ){bb47=(bb416[bb47>>24 ]^bb416[bb47>>16 &0xff ]<<24 ^
bb416[bb47>>8 &0xff ]<<16 ^bb416[bb47&0xff ]<<8 )^(bbo<<24 );bbo=bb2155(bbo
);}bb50 bbm(bb1286>6 &&bbz%bb1286==4 ){bb47=((bb47)>>(8 )|(bb47)<<(32 -8 ));
bb47=(bb416[bb47>>24 ]^bb416[bb47>>16 &0xff ]<<24 ^bb416[bb47>>8 &0xff ]<<
16 ^bb416[bb47&0xff ]<<8 );} *bb6= * (bb6-bb1286)^bb47;bb6++;}}}bbb
bb1436(bb204*bbi,bbh bbb*bb30,bbn bb100){bb204 bbw;bb31 bb6=bbi->
bb368;bbn bbz;bb1073(&bbw,bb30,bb100);bbi->bb434=bbw.bb434;bb91(bbz=0
;bbz<=bbw.bb434;bbz++){bb75(bb6+4 *bbz,bbw.bb368+4 * (bbw.bb434-bbz),16
);}bb91(bbz=1 ;bbz<bbw.bb434;bbz++){bb6+=4 ;bb6[0 ]=bb1803[bb416[bb6[0 ]
>>24 ]]^bb1802[bb416[bb6[0 ]>>16 &0xff ]]^bb1801[bb416[bb6[0 ]>>8 &0xff ]]^
bb1800[bb416[bb6[0 ]&0xff ]];;bb6[1 ]=bb1803[bb416[bb6[1 ]>>24 ]]^bb1802[
bb416[bb6[1 ]>>16 &0xff ]]^bb1801[bb416[bb6[1 ]>>8 &0xff ]]^bb1800[bb416[
bb6[1 ]&0xff ]];;bb6[2 ]=bb1803[bb416[bb6[2 ]>>24 ]]^bb1802[bb416[bb6[2 ]>>
16 &0xff ]]^bb1801[bb416[bb6[2 ]>>8 &0xff ]]^bb1800[bb416[bb6[2 ]&0xff ]];;
bb6[3 ]=bb1803[bb416[bb6[3 ]>>24 ]]^bb1802[bb416[bb6[3 ]>>16 &0xff ]]^
bb1801[bb416[bb6[3 ]>>8 &0xff ]]^bb1800[bb416[bb6[3 ]&0xff ]];;}}bbb bb1022
(bb204*bbi,bbb*bb1,bbh bbb*bbx){bbd bb541,bb198,bb364,bb787,bb1166,
bb54,bb89,bb1171;bbn bb434=bbi->bb434,bbz;bb31 bb6=(bb31)bbi->bb368;
bb541=(((bb3)((bb31)bbx))[3 ]|((bb3)((bb31)bbx))[2 ]<<8 |((bb3)((bb31)bbx
))[1 ]<<16 |((bb3)((bb31)bbx))[0 ]<<24 )^bb6[0 ];bb198=(((bb3)((bb31)bbx+1
))[3 ]|((bb3)((bb31)bbx+1 ))[2 ]<<8 |((bb3)((bb31)bbx+1 ))[1 ]<<16 |((bb3)((
bb31)bbx+1 ))[0 ]<<24 )^bb6[1 ];bb364=(((bb3)((bb31)bbx+2 ))[3 ]|((bb3)((
bb31)bbx+2 ))[2 ]<<8 |((bb3)((bb31)bbx+2 ))[1 ]<<16 |((bb3)((bb31)bbx+2 ))[0
]<<24 )^bb6[2 ];bb787=(((bb3)((bb31)bbx+3 ))[3 ]|((bb3)((bb31)bbx+3 ))[2 ]
<<8 |((bb3)((bb31)bbx+3 ))[1 ]<<16 |((bb3)((bb31)bbx+3 ))[0 ]<<24 )^bb6[3 ];
bb91(bbz=1 ;bbz<bb434;bbz++){bb6+=4 ;bb1166=bb2148[bb541>>24 ]^bb2146[(
bb198>>16 )&0xff ]^bb2147[(bb364>>8 )&0xff ]^bb2145[(bb787&0xff )]^bb6[0 ];
bb54=bb2148[bb198>>24 ]^bb2146[(bb364>>16 )&0xff ]^bb2147[(bb787>>8 )&
0xff ]^bb2145[(bb541&0xff )]^bb6[1 ];bb89=bb2148[bb364>>24 ]^bb2146[(
bb787>>16 )&0xff ]^bb2147[(bb541>>8 )&0xff ]^bb2145[(bb198&0xff )]^bb6[2 ];
bb1171=bb2148[bb787>>24 ]^bb2146[(bb541>>16 )&0xff ]^bb2147[(bb198>>8 )&
0xff ]^bb2145[(bb364&0xff )]^bb6[3 ];bb541=bb1166;bb198=bb54;bb364=bb89;
bb787=bb1171;}bb6+=4 ;bb1166=bb416[bb541>>24 ]<<24 ^bb416[bb198>>16 &0xff
]<<16 ^bb416[bb364>>8 &0xff ]<<8 ^bb416[bb787&0xff ]^bb6[0 ];bb54=bb416[
bb198>>24 ]<<24 ^bb416[bb364>>16 &0xff ]<<16 ^bb416[bb787>>8 &0xff ]<<8 ^
bb416[bb541&0xff ]^bb6[1 ];bb89=bb416[bb364>>24 ]<<24 ^bb416[bb787>>16 &
0xff ]<<16 ^bb416[bb541>>8 &0xff ]<<8 ^bb416[bb198&0xff ]^bb6[2 ];bb1171=
bb416[bb787>>24 ]<<24 ^bb416[bb541>>16 &0xff ]<<16 ^bb416[bb198>>8 &0xff ]<<
8 ^bb416[bb364&0xff ]^bb6[3 ];((bb31)bb1)[0 ]=(((bb3)(&bb1166))[3 ]|((bb3)(
&bb1166))[2 ]<<8 |((bb3)(&bb1166))[1 ]<<16 |((bb3)(&bb1166))[0 ]<<24 );((
bb31)bb1)[1 ]=(((bb3)(&bb54))[3 ]|((bb3)(&bb54))[2 ]<<8 |((bb3)(&bb54))[1
]<<16 |((bb3)(&bb54))[0 ]<<24 );((bb31)bb1)[2 ]=(((bb3)(&bb89))[3 ]|((bb3)(
&bb89))[2 ]<<8 |((bb3)(&bb89))[1 ]<<16 |((bb3)(&bb89))[0 ]<<24 );((bb31)bb1
)[3 ]=(((bb3)(&bb1171))[3 ]|((bb3)(&bb1171))[2 ]<<8 |((bb3)(&bb1171))[1 ]
<<16 |((bb3)(&bb1171))[0 ]<<24 );}bbb bb1636(bb204*bbi,bbb*bb1,bbh bbb*
bbx){bbd bb541,bb198,bb364,bb787,bb1166,bb54,bb89,bb1171;bbn bb434=
bbi->bb434,bbz;bb31 bb6=(bb31)bbi->bb368;bb541=(((bb3)((bb31)bbx))[3 ]
|((bb3)((bb31)bbx))[2 ]<<8 |((bb3)((bb31)bbx))[1 ]<<16 |((bb3)((bb31)bbx))[
0 ]<<24 )^bb6[0 ];bb198=(((bb3)((bb31)bbx+1 ))[3 ]|((bb3)((bb31)bbx+1 ))[2 ]
<<8 |((bb3)((bb31)bbx+1 ))[1 ]<<16 |((bb3)((bb31)bbx+1 ))[0 ]<<24 )^bb6[1 ];
bb364=(((bb3)((bb31)bbx+2 ))[3 ]|((bb3)((bb31)bbx+2 ))[2 ]<<8 |((bb3)((
bb31)bbx+2 ))[1 ]<<16 |((bb3)((bb31)bbx+2 ))[0 ]<<24 )^bb6[2 ];bb787=(((bb3)(
(bb31)bbx+3 ))[3 ]|((bb3)((bb31)bbx+3 ))[2 ]<<8 |((bb3)((bb31)bbx+3 ))[1 ]<<
16 |((bb3)((bb31)bbx+3 ))[0 ]<<24 )^bb6[3 ];bb91(bbz=1 ;bbz<bb434;bbz++){
bb6+=4 ;bb1166=bb1803[bb541>>24 ]^bb1802[bb787>>16 &0xff ]^bb1801[bb364>>
8 &0xff ]^bb1800[bb198&0xff ]^bb6[0 ];bb54=bb1803[bb198>>24 ]^bb1802[bb541
>>16 &0xff ]^bb1801[bb787>>8 &0xff ]^bb1800[bb364&0xff ]^bb6[1 ];bb89=
bb1803[bb364>>24 ]^bb1802[bb198>>16 &0xff ]^bb1801[bb541>>8 &0xff ]^bb1800
[bb787&0xff ]^bb6[2 ];bb1171=bb1803[bb787>>24 ]^bb1802[bb364>>16 &0xff ]^
bb1801[bb198>>8 &0xff ]^bb1800[bb541&0xff ]^bb6[3 ];bb541=bb1166;bb198=
bb54;bb364=bb89;bb787=bb1171;}bb6+=4 ;bb1166=bb1031[bb541>>24 ]<<24 ^
bb1031[bb787>>16 &0xff ]<<16 ^bb1031[bb364>>8 &0xff ]<<8 ^bb1031[bb198&0xff
]^bb6[0 ];bb54=bb1031[bb198>>24 ]<<24 ^bb1031[bb541>>16 &0xff ]<<16 ^bb1031
[bb787>>8 &0xff ]<<8 ^bb1031[bb364&0xff ]^bb6[1 ];bb89=bb1031[bb364>>24 ]<<
24 ^bb1031[bb198>>16 &0xff ]<<16 ^bb1031[bb541>>8 &0xff ]<<8 ^bb1031[bb787&
0xff ]^bb6[2 ];bb1171=bb1031[bb787>>24 ]<<24 ^bb1031[bb364>>16 &0xff ]<<16 ^
bb1031[bb198>>8 &0xff ]<<8 ^bb1031[bb541&0xff ]^bb6[3 ];((bb31)bb1)[0 ]=(((
bb3)(&bb1166))[3 ]|((bb3)(&bb1166))[2 ]<<8 |((bb3)(&bb1166))[1 ]<<16 |((
bb3)(&bb1166))[0 ]<<24 );((bb31)bb1)[1 ]=(((bb3)(&bb54))[3 ]|((bb3)(&bb54
))[2 ]<<8 |((bb3)(&bb54))[1 ]<<16 |((bb3)(&bb54))[0 ]<<24 );((bb31)bb1)[2 ]=
(((bb3)(&bb89))[3 ]|((bb3)(&bb89))[2 ]<<8 |((bb3)(&bb89))[1 ]<<16 |((bb3)(
&bb89))[0 ]<<24 );((bb31)bb1)[3 ]=(((bb3)(&bb1171))[3 ]|((bb3)(&bb1171))[
2 ]<<8 |((bb3)(&bb1171))[1 ]<<16 |((bb3)(&bb1171))[0 ]<<24 );}
