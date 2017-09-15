/*
   'ripemd.c' Obfuscated by COBF (Version 1.06 2006-01-07 by BB) at Fri Nov 13 10:03:51 2015
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
bba bbj{bbd bb5;bbd bb23[5 ];bbf bb105[64 ];}bb461;bbb bb1844(bb461*bbi
);bbb bb1337(bb461*bbi,bbh bbb*bb509,bbn bb5);bbb bb1899(bb461*bbi,
bbb*bb1);bbb bb1980(bbb*bb1,bbh bbb*bbx,bbn bb5);bbb bb2032(bbb*bb1,
bb62 bbx);
#ifdef __cplusplus
}
#endif
bb40 bbb bb1298(bbd bb23[5 ],bbh bbf bb99[64 ]){bb27(bb12(bbe)>=4 );{bbd
bb70,bb64,bb68,bb58,bb66,bb71,bb65,bb51,bb69,bb67;bbd bbv[16 ],bbz;
bb70=bb64=bb23[0 ];bb68=bb58=bb23[1 ];bb66=bb71=bb23[2 ];bb65=bb51=bb23[
3 ];bb69=bb67=bb23[4 ];bb91(bbz=0 ;bbz<16 ;bbz++,bb99+=4 )bbv[bbz]=(bb99[0
]|bb99[1 ]<<8 |bb99[2 ]<<16 |bb99[3 ]<<24 );bb64+=(bb58^bb71^bb51)+bbv[0 ];
bb64=((bb64)<<(11 )|(bb64)>>(32 -11 ))+bb67;bb71=((bb71)<<(10 )|(bb71)>>(
32 -10 ));bb67+=(bb64^bb58^bb71)+bbv[1 ];bb67=((bb67)<<(14 )|(bb67)>>(32 -
14 ))+bb51;bb58=((bb58)<<(10 )|(bb58)>>(32 -10 ));bb51+=(bb67^bb64^bb58)+
bbv[2 ];bb51=((bb51)<<(15 )|(bb51)>>(32 -15 ))+bb71;bb64=((bb64)<<(10 )|(
bb64)>>(32 -10 ));bb71+=(bb51^bb67^bb64)+bbv[3 ];bb71=((bb71)<<(12 )|(
bb71)>>(32 -12 ))+bb58;bb67=((bb67)<<(10 )|(bb67)>>(32 -10 ));bb58+=(bb71^
bb51^bb67)+bbv[4 ];bb58=((bb58)<<(5 )|(bb58)>>(32 -5 ))+bb64;bb51=((bb51)<<
(10 )|(bb51)>>(32 -10 ));bb64+=(bb58^bb71^bb51)+bbv[5 ];bb64=((bb64)<<(8 )|
(bb64)>>(32 -8 ))+bb67;bb71=((bb71)<<(10 )|(bb71)>>(32 -10 ));bb67+=(bb64^
bb58^bb71)+bbv[6 ];bb67=((bb67)<<(7 )|(bb67)>>(32 -7 ))+bb51;bb58=((bb58)<<
(10 )|(bb58)>>(32 -10 ));bb51+=(bb67^bb64^bb58)+bbv[7 ];bb51=((bb51)<<(9 )|
(bb51)>>(32 -9 ))+bb71;bb64=((bb64)<<(10 )|(bb64)>>(32 -10 ));bb71+=(bb51^
bb67^bb64)+bbv[8 ];bb71=((bb71)<<(11 )|(bb71)>>(32 -11 ))+bb58;bb67=((
bb67)<<(10 )|(bb67)>>(32 -10 ));bb58+=(bb71^bb51^bb67)+bbv[9 ];bb58=((
bb58)<<(13 )|(bb58)>>(32 -13 ))+bb64;bb51=((bb51)<<(10 )|(bb51)>>(32 -10 ));
bb64+=(bb58^bb71^bb51)+bbv[10 ];bb64=((bb64)<<(14 )|(bb64)>>(32 -14 ))+
bb67;bb71=((bb71)<<(10 )|(bb71)>>(32 -10 ));bb67+=(bb64^bb58^bb71)+bbv[
11 ];bb67=((bb67)<<(15 )|(bb67)>>(32 -15 ))+bb51;bb58=((bb58)<<(10 )|(bb58
)>>(32 -10 ));bb51+=(bb67^bb64^bb58)+bbv[12 ];bb51=((bb51)<<(6 )|(bb51)>>
(32 -6 ))+bb71;bb64=((bb64)<<(10 )|(bb64)>>(32 -10 ));bb71+=(bb51^bb67^
bb64)+bbv[13 ];bb71=((bb71)<<(7 )|(bb71)>>(32 -7 ))+bb58;bb67=((bb67)<<(
10 )|(bb67)>>(32 -10 ));bb58+=(bb71^bb51^bb67)+bbv[14 ];bb58=((bb58)<<(9 )|
(bb58)>>(32 -9 ))+bb64;bb51=((bb51)<<(10 )|(bb51)>>(32 -10 ));bb64+=(bb58^
bb71^bb51)+bbv[15 ];bb64=((bb64)<<(8 )|(bb64)>>(32 -8 ))+bb67;bb71=((bb71
)<<(10 )|(bb71)>>(32 -10 ));bb67+=(bb64&bb58|~bb64&bb71)+0x5a827999 +bbv[
7 ];bb67=((bb67)<<(7 )|(bb67)>>(32 -7 ))+bb51;bb58=((bb58)<<(10 )|(bb58)>>
(32 -10 ));bb51+=(bb67&bb64|~bb67&bb58)+0x5a827999 +bbv[4 ];bb51=((bb51)<<
(6 )|(bb51)>>(32 -6 ))+bb71;bb64=((bb64)<<(10 )|(bb64)>>(32 -10 ));bb71+=(
bb51&bb67|~bb51&bb64)+0x5a827999 +bbv[13 ];bb71=((bb71)<<(8 )|(bb71)>>(
32 -8 ))+bb58;bb67=((bb67)<<(10 )|(bb67)>>(32 -10 ));bb58+=(bb71&bb51|~
bb71&bb67)+0x5a827999 +bbv[1 ];bb58=((bb58)<<(13 )|(bb58)>>(32 -13 ))+bb64
;bb51=((bb51)<<(10 )|(bb51)>>(32 -10 ));bb64+=(bb58&bb71|~bb58&bb51)+
0x5a827999 +bbv[10 ];bb64=((bb64)<<(11 )|(bb64)>>(32 -11 ))+bb67;bb71=((
bb71)<<(10 )|(bb71)>>(32 -10 ));bb67+=(bb64&bb58|~bb64&bb71)+0x5a827999 +
bbv[6 ];bb67=((bb67)<<(9 )|(bb67)>>(32 -9 ))+bb51;bb58=((bb58)<<(10 )|(
bb58)>>(32 -10 ));bb51+=(bb67&bb64|~bb67&bb58)+0x5a827999 +bbv[15 ];bb51=
((bb51)<<(7 )|(bb51)>>(32 -7 ))+bb71;bb64=((bb64)<<(10 )|(bb64)>>(32 -10 ));
bb71+=(bb51&bb67|~bb51&bb64)+0x5a827999 +bbv[3 ];bb71=((bb71)<<(15 )|(
bb71)>>(32 -15 ))+bb58;bb67=((bb67)<<(10 )|(bb67)>>(32 -10 ));bb58+=(bb71&
bb51|~bb71&bb67)+0x5a827999 +bbv[12 ];bb58=((bb58)<<(7 )|(bb58)>>(32 -7 ))+
bb64;bb51=((bb51)<<(10 )|(bb51)>>(32 -10 ));bb64+=(bb58&bb71|~bb58&bb51)+
0x5a827999 +bbv[0 ];bb64=((bb64)<<(12 )|(bb64)>>(32 -12 ))+bb67;bb71=((
bb71)<<(10 )|(bb71)>>(32 -10 ));bb67+=(bb64&bb58|~bb64&bb71)+0x5a827999 +
bbv[9 ];bb67=((bb67)<<(15 )|(bb67)>>(32 -15 ))+bb51;bb58=((bb58)<<(10 )|(
bb58)>>(32 -10 ));bb51+=(bb67&bb64|~bb67&bb58)+0x5a827999 +bbv[5 ];bb51=(
(bb51)<<(9 )|(bb51)>>(32 -9 ))+bb71;bb64=((bb64)<<(10 )|(bb64)>>(32 -10 ));
bb71+=(bb51&bb67|~bb51&bb64)+0x5a827999 +bbv[2 ];bb71=((bb71)<<(11 )|(
bb71)>>(32 -11 ))+bb58;bb67=((bb67)<<(10 )|(bb67)>>(32 -10 ));bb58+=(bb71&
bb51|~bb71&bb67)+0x5a827999 +bbv[14 ];bb58=((bb58)<<(7 )|(bb58)>>(32 -7 ))+
bb64;bb51=((bb51)<<(10 )|(bb51)>>(32 -10 ));bb64+=(bb58&bb71|~bb58&bb51)+
0x5a827999 +bbv[11 ];bb64=((bb64)<<(13 )|(bb64)>>(32 -13 ))+bb67;bb71=((
bb71)<<(10 )|(bb71)>>(32 -10 ));bb67+=(bb64&bb58|~bb64&bb71)+0x5a827999 +
bbv[8 ];bb67=((bb67)<<(12 )|(bb67)>>(32 -12 ))+bb51;bb58=((bb58)<<(10 )|(
bb58)>>(32 -10 ));bb51+=((bb67|~bb64)^bb58)+0x6ed9eba1 +bbv[3 ];bb51=((
bb51)<<(11 )|(bb51)>>(32 -11 ))+bb71;bb64=((bb64)<<(10 )|(bb64)>>(32 -10 ));
bb71+=((bb51|~bb67)^bb64)+0x6ed9eba1 +bbv[10 ];bb71=((bb71)<<(13 )|(bb71
)>>(32 -13 ))+bb58;bb67=((bb67)<<(10 )|(bb67)>>(32 -10 ));bb58+=((bb71|~
bb51)^bb67)+0x6ed9eba1 +bbv[14 ];bb58=((bb58)<<(6 )|(bb58)>>(32 -6 ))+bb64
;bb51=((bb51)<<(10 )|(bb51)>>(32 -10 ));bb64+=((bb58|~bb71)^bb51)+
0x6ed9eba1 +bbv[4 ];bb64=((bb64)<<(7 )|(bb64)>>(32 -7 ))+bb67;bb71=((bb71)<<
(10 )|(bb71)>>(32 -10 ));bb67+=((bb64|~bb58)^bb71)+0x6ed9eba1 +bbv[9 ];
bb67=((bb67)<<(14 )|(bb67)>>(32 -14 ))+bb51;bb58=((bb58)<<(10 )|(bb58)>>(
32 -10 ));bb51+=((bb67|~bb64)^bb58)+0x6ed9eba1 +bbv[15 ];bb51=((bb51)<<(9
)|(bb51)>>(32 -9 ))+bb71;bb64=((bb64)<<(10 )|(bb64)>>(32 -10 ));bb71+=((
bb51|~bb67)^bb64)+0x6ed9eba1 +bbv[8 ];bb71=((bb71)<<(13 )|(bb71)>>(32 -13
))+bb58;bb67=((bb67)<<(10 )|(bb67)>>(32 -10 ));bb58+=((bb71|~bb51)^bb67)+
0x6ed9eba1 +bbv[1 ];bb58=((bb58)<<(15 )|(bb58)>>(32 -15 ))+bb64;bb51=((
bb51)<<(10 )|(bb51)>>(32 -10 ));bb64+=((bb58|~bb71)^bb51)+0x6ed9eba1 +bbv
[2 ];bb64=((bb64)<<(14 )|(bb64)>>(32 -14 ))+bb67;bb71=((bb71)<<(10 )|(bb71
)>>(32 -10 ));bb67+=((bb64|~bb58)^bb71)+0x6ed9eba1 +bbv[7 ];bb67=((bb67)<<
(8 )|(bb67)>>(32 -8 ))+bb51;bb58=((bb58)<<(10 )|(bb58)>>(32 -10 ));bb51+=((
bb67|~bb64)^bb58)+0x6ed9eba1 +bbv[0 ];bb51=((bb51)<<(13 )|(bb51)>>(32 -13
))+bb71;bb64=((bb64)<<(10 )|(bb64)>>(32 -10 ));bb71+=((bb51|~bb67)^bb64)+
0x6ed9eba1 +bbv[6 ];bb71=((bb71)<<(6 )|(bb71)>>(32 -6 ))+bb58;bb67=((bb67)<<
(10 )|(bb67)>>(32 -10 ));bb58+=((bb71|~bb51)^bb67)+0x6ed9eba1 +bbv[13 ];
bb58=((bb58)<<(5 )|(bb58)>>(32 -5 ))+bb64;bb51=((bb51)<<(10 )|(bb51)>>(32
-10 ));bb64+=((bb58|~bb71)^bb51)+0x6ed9eba1 +bbv[11 ];bb64=((bb64)<<(12 )|
(bb64)>>(32 -12 ))+bb67;bb71=((bb71)<<(10 )|(bb71)>>(32 -10 ));bb67+=((
bb64|~bb58)^bb71)+0x6ed9eba1 +bbv[5 ];bb67=((bb67)<<(7 )|(bb67)>>(32 -7 ))+
bb51;bb58=((bb58)<<(10 )|(bb58)>>(32 -10 ));bb51+=((bb67|~bb64)^bb58)+
0x6ed9eba1 +bbv[12 ];bb51=((bb51)<<(5 )|(bb51)>>(32 -5 ))+bb71;bb64=((bb64
)<<(10 )|(bb64)>>(32 -10 ));bb71+=(bb51&bb64|bb67&~bb64)+0x8f1bbcdc +bbv[
1 ];bb71=((bb71)<<(11 )|(bb71)>>(32 -11 ))+bb58;bb67=((bb67)<<(10 )|(bb67)>>
(32 -10 ));bb58+=(bb71&bb67|bb51&~bb67)+0x8f1bbcdc +bbv[9 ];bb58=((bb58)<<
(12 )|(bb58)>>(32 -12 ))+bb64;bb51=((bb51)<<(10 )|(bb51)>>(32 -10 ));bb64+=
(bb58&bb51|bb71&~bb51)+0x8f1bbcdc +bbv[11 ];bb64=((bb64)<<(14 )|(bb64)>>
(32 -14 ))+bb67;bb71=((bb71)<<(10 )|(bb71)>>(32 -10 ));bb67+=(bb64&bb71|
bb58&~bb71)+0x8f1bbcdc +bbv[10 ];bb67=((bb67)<<(15 )|(bb67)>>(32 -15 ))+
bb51;bb58=((bb58)<<(10 )|(bb58)>>(32 -10 ));bb51+=(bb67&bb58|bb64&~bb58)+
0x8f1bbcdc +bbv[0 ];bb51=((bb51)<<(14 )|(bb51)>>(32 -14 ))+bb71;bb64=((
bb64)<<(10 )|(bb64)>>(32 -10 ));bb71+=(bb51&bb64|bb67&~bb64)+0x8f1bbcdc +
bbv[8 ];bb71=((bb71)<<(15 )|(bb71)>>(32 -15 ))+bb58;bb67=((bb67)<<(10 )|(
bb67)>>(32 -10 ));bb58+=(bb71&bb67|bb51&~bb67)+0x8f1bbcdc +bbv[12 ];bb58=
((bb58)<<(9 )|(bb58)>>(32 -9 ))+bb64;bb51=((bb51)<<(10 )|(bb51)>>(32 -10 ));
bb64+=(bb58&bb51|bb71&~bb51)+0x8f1bbcdc +bbv[4 ];bb64=((bb64)<<(8 )|(
bb64)>>(32 -8 ))+bb67;bb71=((bb71)<<(10 )|(bb71)>>(32 -10 ));bb67+=(bb64&
bb71|bb58&~bb71)+0x8f1bbcdc +bbv[13 ];bb67=((bb67)<<(9 )|(bb67)>>(32 -9 ))+
bb51;bb58=((bb58)<<(10 )|(bb58)>>(32 -10 ));bb51+=(bb67&bb58|bb64&~bb58)+
0x8f1bbcdc +bbv[3 ];bb51=((bb51)<<(14 )|(bb51)>>(32 -14 ))+bb71;bb64=((
bb64)<<(10 )|(bb64)>>(32 -10 ));bb71+=(bb51&bb64|bb67&~bb64)+0x8f1bbcdc +
bbv[7 ];bb71=((bb71)<<(5 )|(bb71)>>(32 -5 ))+bb58;bb67=((bb67)<<(10 )|(
bb67)>>(32 -10 ));bb58+=(bb71&bb67|bb51&~bb67)+0x8f1bbcdc +bbv[15 ];bb58=
((bb58)<<(6 )|(bb58)>>(32 -6 ))+bb64;bb51=((bb51)<<(10 )|(bb51)>>(32 -10 ));
bb64+=(bb58&bb51|bb71&~bb51)+0x8f1bbcdc +bbv[14 ];bb64=((bb64)<<(8 )|(
bb64)>>(32 -8 ))+bb67;bb71=((bb71)<<(10 )|(bb71)>>(32 -10 ));bb67+=(bb64&
bb71|bb58&~bb71)+0x8f1bbcdc +bbv[5 ];bb67=((bb67)<<(6 )|(bb67)>>(32 -6 ))+
bb51;bb58=((bb58)<<(10 )|(bb58)>>(32 -10 ));bb51+=(bb67&bb58|bb64&~bb58)+
0x8f1bbcdc +bbv[6 ];bb51=((bb51)<<(5 )|(bb51)>>(32 -5 ))+bb71;bb64=((bb64)<<
(10 )|(bb64)>>(32 -10 ));bb71+=(bb51&bb64|bb67&~bb64)+0x8f1bbcdc +bbv[2 ];
bb71=((bb71)<<(12 )|(bb71)>>(32 -12 ))+bb58;bb67=((bb67)<<(10 )|(bb67)>>(
32 -10 ));bb58+=(bb71^(bb51|~bb67))+0xa953fd4e +bbv[4 ];bb58=((bb58)<<(9 )|
(bb58)>>(32 -9 ))+bb64;bb51=((bb51)<<(10 )|(bb51)>>(32 -10 ));bb64+=(bb58^
(bb71|~bb51))+0xa953fd4e +bbv[0 ];bb64=((bb64)<<(15 )|(bb64)>>(32 -15 ))+
bb67;bb71=((bb71)<<(10 )|(bb71)>>(32 -10 ));bb67+=(bb64^(bb58|~bb71))+
0xa953fd4e +bbv[5 ];bb67=((bb67)<<(5 )|(bb67)>>(32 -5 ))+bb51;bb58=((bb58)<<
(10 )|(bb58)>>(32 -10 ));bb51+=(bb67^(bb64|~bb58))+0xa953fd4e +bbv[9 ];
bb51=((bb51)<<(11 )|(bb51)>>(32 -11 ))+bb71;bb64=((bb64)<<(10 )|(bb64)>>(
32 -10 ));bb71+=(bb51^(bb67|~bb64))+0xa953fd4e +bbv[7 ];bb71=((bb71)<<(6 )|
(bb71)>>(32 -6 ))+bb58;bb67=((bb67)<<(10 )|(bb67)>>(32 -10 ));bb58+=(bb71^
(bb51|~bb67))+0xa953fd4e +bbv[12 ];bb58=((bb58)<<(8 )|(bb58)>>(32 -8 ))+
bb64;bb51=((bb51)<<(10 )|(bb51)>>(32 -10 ));bb64+=(bb58^(bb71|~bb51))+
0xa953fd4e +bbv[2 ];bb64=((bb64)<<(13 )|(bb64)>>(32 -13 ))+bb67;bb71=((
bb71)<<(10 )|(bb71)>>(32 -10 ));bb67+=(bb64^(bb58|~bb71))+0xa953fd4e +bbv
[10 ];bb67=((bb67)<<(12 )|(bb67)>>(32 -12 ))+bb51;bb58=((bb58)<<(10 )|(
bb58)>>(32 -10 ));bb51+=(bb67^(bb64|~bb58))+0xa953fd4e +bbv[14 ];bb51=((
bb51)<<(5 )|(bb51)>>(32 -5 ))+bb71;bb64=((bb64)<<(10 )|(bb64)>>(32 -10 ));
bb71+=(bb51^(bb67|~bb64))+0xa953fd4e +bbv[1 ];bb71=((bb71)<<(12 )|(bb71)>>
(32 -12 ))+bb58;bb67=((bb67)<<(10 )|(bb67)>>(32 -10 ));bb58+=(bb71^(bb51|~
bb67))+0xa953fd4e +bbv[3 ];bb58=((bb58)<<(13 )|(bb58)>>(32 -13 ))+bb64;
bb51=((bb51)<<(10 )|(bb51)>>(32 -10 ));bb64+=(bb58^(bb71|~bb51))+
0xa953fd4e +bbv[8 ];bb64=((bb64)<<(14 )|(bb64)>>(32 -14 ))+bb67;bb71=((
bb71)<<(10 )|(bb71)>>(32 -10 ));bb67+=(bb64^(bb58|~bb71))+0xa953fd4e +bbv
[11 ];bb67=((bb67)<<(11 )|(bb67)>>(32 -11 ))+bb51;bb58=((bb58)<<(10 )|(
bb58)>>(32 -10 ));bb51+=(bb67^(bb64|~bb58))+0xa953fd4e +bbv[6 ];bb51=((
bb51)<<(8 )|(bb51)>>(32 -8 ))+bb71;bb64=((bb64)<<(10 )|(bb64)>>(32 -10 ));
bb71+=(bb51^(bb67|~bb64))+0xa953fd4e +bbv[15 ];bb71=((bb71)<<(5 )|(bb71)>>
(32 -5 ))+bb58;bb67=((bb67)<<(10 )|(bb67)>>(32 -10 ));bb58+=(bb71^(bb51|~
bb67))+0xa953fd4e +bbv[13 ];bb58=((bb58)<<(6 )|(bb58)>>(32 -6 ))+bb64;bb51
=((bb51)<<(10 )|(bb51)>>(32 -10 ));bb70+=(bb68^(bb66|~bb65))+0x50a28be6 +
bbv[5 ];bb70=((bb70)<<(8 )|(bb70)>>(32 -8 ))+bb69;bb66=((bb66)<<(10 )|(
bb66)>>(32 -10 ));bb69+=(bb70^(bb68|~bb66))+0x50a28be6 +bbv[14 ];bb69=((
bb69)<<(9 )|(bb69)>>(32 -9 ))+bb65;bb68=((bb68)<<(10 )|(bb68)>>(32 -10 ));
bb65+=(bb69^(bb70|~bb68))+0x50a28be6 +bbv[7 ];bb65=((bb65)<<(9 )|(bb65)>>
(32 -9 ))+bb66;bb70=((bb70)<<(10 )|(bb70)>>(32 -10 ));bb66+=(bb65^(bb69|~
bb70))+0x50a28be6 +bbv[0 ];bb66=((bb66)<<(11 )|(bb66)>>(32 -11 ))+bb68;
bb69=((bb69)<<(10 )|(bb69)>>(32 -10 ));bb68+=(bb66^(bb65|~bb69))+
0x50a28be6 +bbv[9 ];bb68=((bb68)<<(13 )|(bb68)>>(32 -13 ))+bb70;bb65=((
bb65)<<(10 )|(bb65)>>(32 -10 ));bb70+=(bb68^(bb66|~bb65))+0x50a28be6 +bbv
[2 ];bb70=((bb70)<<(15 )|(bb70)>>(32 -15 ))+bb69;bb66=((bb66)<<(10 )|(bb66
)>>(32 -10 ));bb69+=(bb70^(bb68|~bb66))+0x50a28be6 +bbv[11 ];bb69=((bb69)<<
(15 )|(bb69)>>(32 -15 ))+bb65;bb68=((bb68)<<(10 )|(bb68)>>(32 -10 ));bb65+=
(bb69^(bb70|~bb68))+0x50a28be6 +bbv[4 ];bb65=((bb65)<<(5 )|(bb65)>>(32 -5
))+bb66;bb70=((bb70)<<(10 )|(bb70)>>(32 -10 ));bb66+=(bb65^(bb69|~bb70))+
0x50a28be6 +bbv[13 ];bb66=((bb66)<<(7 )|(bb66)>>(32 -7 ))+bb68;bb69=((bb69
)<<(10 )|(bb69)>>(32 -10 ));bb68+=(bb66^(bb65|~bb69))+0x50a28be6 +bbv[6 ];
bb68=((bb68)<<(7 )|(bb68)>>(32 -7 ))+bb70;bb65=((bb65)<<(10 )|(bb65)>>(32
-10 ));bb70+=(bb68^(bb66|~bb65))+0x50a28be6 +bbv[15 ];bb70=((bb70)<<(8 )|
(bb70)>>(32 -8 ))+bb69;bb66=((bb66)<<(10 )|(bb66)>>(32 -10 ));bb69+=(bb70^
(bb68|~bb66))+0x50a28be6 +bbv[8 ];bb69=((bb69)<<(11 )|(bb69)>>(32 -11 ))+
bb65;bb68=((bb68)<<(10 )|(bb68)>>(32 -10 ));bb65+=(bb69^(bb70|~bb68))+
0x50a28be6 +bbv[1 ];bb65=((bb65)<<(14 )|(bb65)>>(32 -14 ))+bb66;bb70=((
bb70)<<(10 )|(bb70)>>(32 -10 ));bb66+=(bb65^(bb69|~bb70))+0x50a28be6 +bbv
[10 ];bb66=((bb66)<<(14 )|(bb66)>>(32 -14 ))+bb68;bb69=((bb69)<<(10 )|(
bb69)>>(32 -10 ));bb68+=(bb66^(bb65|~bb69))+0x50a28be6 +bbv[3 ];bb68=((
bb68)<<(12 )|(bb68)>>(32 -12 ))+bb70;bb65=((bb65)<<(10 )|(bb65)>>(32 -10 ));
bb70+=(bb68^(bb66|~bb65))+0x50a28be6 +bbv[12 ];bb70=((bb70)<<(6 )|(bb70)>>
(32 -6 ))+bb69;bb66=((bb66)<<(10 )|(bb66)>>(32 -10 ));bb69+=(bb70&bb66|
bb68&~bb66)+0x5c4dd124 +bbv[6 ];bb69=((bb69)<<(9 )|(bb69)>>(32 -9 ))+bb65;
bb68=((bb68)<<(10 )|(bb68)>>(32 -10 ));bb65+=(bb69&bb68|bb70&~bb68)+
0x5c4dd124 +bbv[11 ];bb65=((bb65)<<(13 )|(bb65)>>(32 -13 ))+bb66;bb70=((
bb70)<<(10 )|(bb70)>>(32 -10 ));bb66+=(bb65&bb70|bb69&~bb70)+0x5c4dd124 +
bbv[3 ];bb66=((bb66)<<(15 )|(bb66)>>(32 -15 ))+bb68;bb69=((bb69)<<(10 )|(
bb69)>>(32 -10 ));bb68+=(bb66&bb69|bb65&~bb69)+0x5c4dd124 +bbv[7 ];bb68=(
(bb68)<<(7 )|(bb68)>>(32 -7 ))+bb70;bb65=((bb65)<<(10 )|(bb65)>>(32 -10 ));
bb70+=(bb68&bb65|bb66&~bb65)+0x5c4dd124 +bbv[0 ];bb70=((bb70)<<(12 )|(
bb70)>>(32 -12 ))+bb69;bb66=((bb66)<<(10 )|(bb66)>>(32 -10 ));bb69+=(bb70&
bb66|bb68&~bb66)+0x5c4dd124 +bbv[13 ];bb69=((bb69)<<(8 )|(bb69)>>(32 -8 ))+
bb65;bb68=((bb68)<<(10 )|(bb68)>>(32 -10 ));bb65+=(bb69&bb68|bb70&~bb68)+
0x5c4dd124 +bbv[5 ];bb65=((bb65)<<(9 )|(bb65)>>(32 -9 ))+bb66;bb70=((bb70)<<
(10 )|(bb70)>>(32 -10 ));bb66+=(bb65&bb70|bb69&~bb70)+0x5c4dd124 +bbv[10 ]
;bb66=((bb66)<<(11 )|(bb66)>>(32 -11 ))+bb68;bb69=((bb69)<<(10 )|(bb69)>>
(32 -10 ));bb68+=(bb66&bb69|bb65&~bb69)+0x5c4dd124 +bbv[14 ];bb68=((bb68)<<
(7 )|(bb68)>>(32 -7 ))+bb70;bb65=((bb65)<<(10 )|(bb65)>>(32 -10 ));bb70+=(
bb68&bb65|bb66&~bb65)+0x5c4dd124 +bbv[15 ];bb70=((bb70)<<(7 )|(bb70)>>(
32 -7 ))+bb69;bb66=((bb66)<<(10 )|(bb66)>>(32 -10 ));bb69+=(bb70&bb66|bb68
&~bb66)+0x5c4dd124 +bbv[8 ];bb69=((bb69)<<(12 )|(bb69)>>(32 -12 ))+bb65;
bb68=((bb68)<<(10 )|(bb68)>>(32 -10 ));bb65+=(bb69&bb68|bb70&~bb68)+
0x5c4dd124 +bbv[12 ];bb65=((bb65)<<(7 )|(bb65)>>(32 -7 ))+bb66;bb70=((bb70
)<<(10 )|(bb70)>>(32 -10 ));bb66+=(bb65&bb70|bb69&~bb70)+0x5c4dd124 +bbv[
4 ];bb66=((bb66)<<(6 )|(bb66)>>(32 -6 ))+bb68;bb69=((bb69)<<(10 )|(bb69)>>
(32 -10 ));bb68+=(bb66&bb69|bb65&~bb69)+0x5c4dd124 +bbv[9 ];bb68=((bb68)<<
(15 )|(bb68)>>(32 -15 ))+bb70;bb65=((bb65)<<(10 )|(bb65)>>(32 -10 ));bb70+=
(bb68&bb65|bb66&~bb65)+0x5c4dd124 +bbv[1 ];bb70=((bb70)<<(13 )|(bb70)>>(
32 -13 ))+bb69;bb66=((bb66)<<(10 )|(bb66)>>(32 -10 ));bb69+=(bb70&bb66|
bb68&~bb66)+0x5c4dd124 +bbv[2 ];bb69=((bb69)<<(11 )|(bb69)>>(32 -11 ))+
bb65;bb68=((bb68)<<(10 )|(bb68)>>(32 -10 ));bb65+=((bb69|~bb70)^bb68)+
0x6d703ef3 +bbv[15 ];bb65=((bb65)<<(9 )|(bb65)>>(32 -9 ))+bb66;bb70=((bb70
)<<(10 )|(bb70)>>(32 -10 ));bb66+=((bb65|~bb69)^bb70)+0x6d703ef3 +bbv[5 ];
bb66=((bb66)<<(7 )|(bb66)>>(32 -7 ))+bb68;bb69=((bb69)<<(10 )|(bb69)>>(32
-10 ));bb68+=((bb66|~bb65)^bb69)+0x6d703ef3 +bbv[1 ];bb68=((bb68)<<(15 )|
(bb68)>>(32 -15 ))+bb70;bb65=((bb65)<<(10 )|(bb65)>>(32 -10 ));bb70+=((
bb68|~bb66)^bb65)+0x6d703ef3 +bbv[3 ];bb70=((bb70)<<(11 )|(bb70)>>(32 -11
))+bb69;bb66=((bb66)<<(10 )|(bb66)>>(32 -10 ));bb69+=((bb70|~bb68)^bb66)+
0x6d703ef3 +bbv[7 ];bb69=((bb69)<<(8 )|(bb69)>>(32 -8 ))+bb65;bb68=((bb68)<<
(10 )|(bb68)>>(32 -10 ));bb65+=((bb69|~bb70)^bb68)+0x6d703ef3 +bbv[14 ];
bb65=((bb65)<<(6 )|(bb65)>>(32 -6 ))+bb66;bb70=((bb70)<<(10 )|(bb70)>>(32
-10 ));bb66+=((bb65|~bb69)^bb70)+0x6d703ef3 +bbv[6 ];bb66=((bb66)<<(6 )|(
bb66)>>(32 -6 ))+bb68;bb69=((bb69)<<(10 )|(bb69)>>(32 -10 ));bb68+=((bb66|
~bb65)^bb69)+0x6d703ef3 +bbv[9 ];bb68=((bb68)<<(14 )|(bb68)>>(32 -14 ))+
bb70;bb65=((bb65)<<(10 )|(bb65)>>(32 -10 ));bb70+=((bb68|~bb66)^bb65)+
0x6d703ef3 +bbv[11 ];bb70=((bb70)<<(12 )|(bb70)>>(32 -12 ))+bb69;bb66=((
bb66)<<(10 )|(bb66)>>(32 -10 ));bb69+=((bb70|~bb68)^bb66)+0x6d703ef3 +bbv
[8 ];bb69=((bb69)<<(13 )|(bb69)>>(32 -13 ))+bb65;bb68=((bb68)<<(10 )|(bb68
)>>(32 -10 ));bb65+=((bb69|~bb70)^bb68)+0x6d703ef3 +bbv[12 ];bb65=((bb65)<<
(5 )|(bb65)>>(32 -5 ))+bb66;bb70=((bb70)<<(10 )|(bb70)>>(32 -10 ));bb66+=((
bb65|~bb69)^bb70)+0x6d703ef3 +bbv[2 ];bb66=((bb66)<<(14 )|(bb66)>>(32 -14
))+bb68;bb69=((bb69)<<(10 )|(bb69)>>(32 -10 ));bb68+=((bb66|~bb65)^bb69)+
0x6d703ef3 +bbv[10 ];bb68=((bb68)<<(13 )|(bb68)>>(32 -13 ))+bb70;bb65=((
bb65)<<(10 )|(bb65)>>(32 -10 ));bb70+=((bb68|~bb66)^bb65)+0x6d703ef3 +bbv
[0 ];bb70=((bb70)<<(13 )|(bb70)>>(32 -13 ))+bb69;bb66=((bb66)<<(10 )|(bb66
)>>(32 -10 ));bb69+=((bb70|~bb68)^bb66)+0x6d703ef3 +bbv[4 ];bb69=((bb69)<<
(7 )|(bb69)>>(32 -7 ))+bb65;bb68=((bb68)<<(10 )|(bb68)>>(32 -10 ));bb65+=((
bb69|~bb70)^bb68)+0x6d703ef3 +bbv[13 ];bb65=((bb65)<<(5 )|(bb65)>>(32 -5 ))+
bb66;bb70=((bb70)<<(10 )|(bb70)>>(32 -10 ));bb66+=(bb65&bb69|~bb65&bb70)+
0x7a6d76e9 +bbv[8 ];bb66=((bb66)<<(15 )|(bb66)>>(32 -15 ))+bb68;bb69=((
bb69)<<(10 )|(bb69)>>(32 -10 ));bb68+=(bb66&bb65|~bb66&bb69)+0x7a6d76e9 +
bbv[6 ];bb68=((bb68)<<(5 )|(bb68)>>(32 -5 ))+bb70;bb65=((bb65)<<(10 )|(
bb65)>>(32 -10 ));bb70+=(bb68&bb66|~bb68&bb65)+0x7a6d76e9 +bbv[4 ];bb70=(
(bb70)<<(8 )|(bb70)>>(32 -8 ))+bb69;bb66=((bb66)<<(10 )|(bb66)>>(32 -10 ));
bb69+=(bb70&bb68|~bb70&bb66)+0x7a6d76e9 +bbv[1 ];bb69=((bb69)<<(11 )|(
bb69)>>(32 -11 ))+bb65;bb68=((bb68)<<(10 )|(bb68)>>(32 -10 ));bb65+=(bb69&
bb70|~bb69&bb68)+0x7a6d76e9 +bbv[3 ];bb65=((bb65)<<(14 )|(bb65)>>(32 -14 ))+
bb66;bb70=((bb70)<<(10 )|(bb70)>>(32 -10 ));bb66+=(bb65&bb69|~bb65&bb70)+
0x7a6d76e9 +bbv[11 ];bb66=((bb66)<<(14 )|(bb66)>>(32 -14 ))+bb68;bb69=((
bb69)<<(10 )|(bb69)>>(32 -10 ));bb68+=(bb66&bb65|~bb66&bb69)+0x7a6d76e9 +
bbv[15 ];bb68=((bb68)<<(6 )|(bb68)>>(32 -6 ))+bb70;bb65=((bb65)<<(10 )|(
bb65)>>(32 -10 ));bb70+=(bb68&bb66|~bb68&bb65)+0x7a6d76e9 +bbv[0 ];bb70=(
(bb70)<<(14 )|(bb70)>>(32 -14 ))+bb69;bb66=((bb66)<<(10 )|(bb66)>>(32 -10 ));
bb69+=(bb70&bb68|~bb70&bb66)+0x7a6d76e9 +bbv[5 ];bb69=((bb69)<<(6 )|(
bb69)>>(32 -6 ))+bb65;bb68=((bb68)<<(10 )|(bb68)>>(32 -10 ));bb65+=(bb69&
bb70|~bb69&bb68)+0x7a6d76e9 +bbv[12 ];bb65=((bb65)<<(9 )|(bb65)>>(32 -9 ))+
bb66;bb70=((bb70)<<(10 )|(bb70)>>(32 -10 ));bb66+=(bb65&bb69|~bb65&bb70)+
0x7a6d76e9 +bbv[2 ];bb66=((bb66)<<(12 )|(bb66)>>(32 -12 ))+bb68;bb69=((
bb69)<<(10 )|(bb69)>>(32 -10 ));bb68+=(bb66&bb65|~bb66&bb69)+0x7a6d76e9 +
bbv[13 ];bb68=((bb68)<<(9 )|(bb68)>>(32 -9 ))+bb70;bb65=((bb65)<<(10 )|(
bb65)>>(32 -10 ));bb70+=(bb68&bb66|~bb68&bb65)+0x7a6d76e9 +bbv[9 ];bb70=(
(bb70)<<(12 )|(bb70)>>(32 -12 ))+bb69;bb66=((bb66)<<(10 )|(bb66)>>(32 -10 ));
bb69+=(bb70&bb68|~bb70&bb66)+0x7a6d76e9 +bbv[7 ];bb69=((bb69)<<(5 )|(
bb69)>>(32 -5 ))+bb65;bb68=((bb68)<<(10 )|(bb68)>>(32 -10 ));bb65+=(bb69&
bb70|~bb69&bb68)+0x7a6d76e9 +bbv[10 ];bb65=((bb65)<<(15 )|(bb65)>>(32 -15
))+bb66;bb70=((bb70)<<(10 )|(bb70)>>(32 -10 ));bb66+=(bb65&bb69|~bb65&
bb70)+0x7a6d76e9 +bbv[14 ];bb66=((bb66)<<(8 )|(bb66)>>(32 -8 ))+bb68;bb69=
((bb69)<<(10 )|(bb69)>>(32 -10 ));bb68+=(bb66^bb65^bb69)+bbv[12 ];bb68=((
bb68)<<(8 )|(bb68)>>(32 -8 ))+bb70;bb65=((bb65)<<(10 )|(bb65)>>(32 -10 ));
bb70+=(bb68^bb66^bb65)+bbv[15 ];bb70=((bb70)<<(5 )|(bb70)>>(32 -5 ))+bb69
;bb66=((bb66)<<(10 )|(bb66)>>(32 -10 ));bb69+=(bb70^bb68^bb66)+bbv[10 ];
bb69=((bb69)<<(12 )|(bb69)>>(32 -12 ))+bb65;bb68=((bb68)<<(10 )|(bb68)>>(
32 -10 ));bb65+=(bb69^bb70^bb68)+bbv[4 ];bb65=((bb65)<<(9 )|(bb65)>>(32 -9
))+bb66;bb70=((bb70)<<(10 )|(bb70)>>(32 -10 ));bb66+=(bb65^bb69^bb70)+
bbv[1 ];bb66=((bb66)<<(12 )|(bb66)>>(32 -12 ))+bb68;bb69=((bb69)<<(10 )|(
bb69)>>(32 -10 ));bb68+=(bb66^bb65^bb69)+bbv[5 ];bb68=((bb68)<<(5 )|(bb68
)>>(32 -5 ))+bb70;bb65=((bb65)<<(10 )|(bb65)>>(32 -10 ));bb70+=(bb68^bb66^
bb65)+bbv[8 ];bb70=((bb70)<<(14 )|(bb70)>>(32 -14 ))+bb69;bb66=((bb66)<<(
10 )|(bb66)>>(32 -10 ));bb69+=(bb70^bb68^bb66)+bbv[7 ];bb69=((bb69)<<(6 )|
(bb69)>>(32 -6 ))+bb65;bb68=((bb68)<<(10 )|(bb68)>>(32 -10 ));bb65+=(bb69^
bb70^bb68)+bbv[6 ];bb65=((bb65)<<(8 )|(bb65)>>(32 -8 ))+bb66;bb70=((bb70)<<
(10 )|(bb70)>>(32 -10 ));bb66+=(bb65^bb69^bb70)+bbv[2 ];bb66=((bb66)<<(13
)|(bb66)>>(32 -13 ))+bb68;bb69=((bb69)<<(10 )|(bb69)>>(32 -10 ));bb68+=(
bb66^bb65^bb69)+bbv[13 ];bb68=((bb68)<<(6 )|(bb68)>>(32 -6 ))+bb70;bb65=(
(bb65)<<(10 )|(bb65)>>(32 -10 ));bb70+=(bb68^bb66^bb65)+bbv[14 ];bb70=((
bb70)<<(5 )|(bb70)>>(32 -5 ))+bb69;bb66=((bb66)<<(10 )|(bb66)>>(32 -10 ));
bb69+=(bb70^bb68^bb66)+bbv[0 ];bb69=((bb69)<<(15 )|(bb69)>>(32 -15 ))+
bb65;bb68=((bb68)<<(10 )|(bb68)>>(32 -10 ));bb65+=(bb69^bb70^bb68)+bbv[3
];bb65=((bb65)<<(13 )|(bb65)>>(32 -13 ))+bb66;bb70=((bb70)<<(10 )|(bb70)>>
(32 -10 ));bb66+=(bb65^bb69^bb70)+bbv[9 ];bb66=((bb66)<<(11 )|(bb66)>>(32
-11 ))+bb68;bb69=((bb69)<<(10 )|(bb69)>>(32 -10 ));bb68+=(bb66^bb65^bb69)+
bbv[11 ];bb68=((bb68)<<(11 )|(bb68)>>(32 -11 ))+bb70;bb65=((bb65)<<(10 )|(
bb65)>>(32 -10 ));bb65+=bb23[1 ]+bb71;bb23[1 ]=bb23[2 ]+bb51+bb69;bb23[2 ]=
bb23[3 ]+bb67+bb70;bb23[3 ]=bb23[4 ]+bb64+bb68;bb23[4 ]=bb23[0 ]+bb58+bb66
;bb23[0 ]=bb65;}}bbb bb1844(bb461*bbi){bb40 bbd bb23[5 ]={0x67452301 ,
0xefcdab89 ,0x98badcfe ,0x10325476 ,0xc3d2e1f0 };bbi->bb5=0 ;bb75(bbi->
bb23,bb23,bb12(bb23));}bbb bb1337(bb461*bbi,bbh bbb*bb509,bbn bb5){
bbh bbf*bbx=(bbh bbf* )bb509;bbn bb398=bbi->bb5%bb12(bbi->bb105);bbi
->bb5+=bb5;bbm(bb398){bbn bb11=bb12(bbi->bb105)-bb398;bb75(bbi->bb105
+bb398,bbx,((bb5)<(bb11)?(bb5):(bb11)));bbm(bb5<bb11)bb4;bbx+=bb11;
bb5-=bb11;bb1298(bbi->bb23,bbi->bb105);}bb91(;bb5>=bb12(bbi->bb105);
bb5-=bb12(bbi->bb105),bbx+=bb12(bbi->bb105))bb1298(bbi->bb23,bbx);
bb75(bbi->bb105,bbx,bb5);}bbb bb1899(bb461*bbi,bbb*bb1){bbd bb1061[2 ]
={(bbd)(bbi->bb5<<3 ),(bbd)(bbi->bb5>>29 )};bbf bb433[bb12(bb1061)];bbn
bbz;bb91(bbz=0 ;bbz<bb12(bb433);bbz++)bb433[bbz]=bb1061[bbz/4 ]>>((bbz%
4 ) *8 )&0xff ;{bbf bb1351[]={0x80 },bb1352[bb12(bbi->bb105)]={0 };bbn
bb398=bbi->bb5%bb12(bbi->bb105);bb1337(bbi,bb1351,1 );bb1337(bbi,
bb1352,(bb12(bbi->bb105) *2 -1 -bb12(bb433)-bb398)%bb12(bbi->bb105));}
bb1337(bbi,bb433,bb12(bb433));bb91(bbz=0 ;bbz<bb12(bbi->bb23);bbz++)((
bbf* )bb1)[bbz]=bbi->bb23[bbz/4 ]>>((bbz%4 ) *8 )&0xff ;}bbb bb1980(bbb*
bb1,bbh bbb*bbx,bbn bb5){bb461 bb82;bb1844(&bb82);bb1337(&bb82,bbx,
bb5);bb1899(&bb82,bb1);}bbb bb2032(bbb*bb1,bb62 bbx){bb1980(bb1,bbx,(
bbn)bb1133(bbx));}
