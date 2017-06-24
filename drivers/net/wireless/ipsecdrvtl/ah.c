/*
   'hmac.c' Obfuscated by COBF (Version 1.06 2006-01-07 by BB) at Fri Nov 13 10:03:51 2015
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
bba bbj{bbd bb5;bbd bb23[4 ];bbf bb105[64 ];}bb457;bbb bb1862(bb457*bbi
);bbb bb1350(bb457*bbi,bbh bbb*bb509,bbn bb5);bbb bb1867(bb457*bbi,
bbb*bb1);bbb bb1903(bbb*bb1,bbh bbb*bbx,bbn bb5);bbb bb2023(bbb*bb1,
bb62 bbx);
#ifdef __cplusplus
}
#endif
#ifdef __cplusplus
bbr"\x43"{
#endif
bba bbj{bbd bb5;bbd bb23[5 ];bbf bb105[64 ];}bb458;bbb bb1898(bb458*bbi
);bbb bb1332(bb458*bbi,bbh bbb*bbx,bbn bb5);bbb bb1845(bb458*bbi,bbb*
bb1);bba bbj{bbd bb5;bbd bb23[8 ];bbf bb105[64 ];}bb410;bbb bb1865(
bb410*bbi);bbb bb1278(bb410*bbi,bbh bbb*bbx,bbn bb5);bbb bb1860(bb410
 *bbi,bbb*bb1);bba bb410 bb978;bbb bb1961(bb978*bbi);bbb bb1913(bb978
 *bbi,bbb*bb1);bba bbj{bbd bb5;bb57 bb23[8 ];bbf bb105[128 ];}bb315;bbb
bb1851(bb315*bbi);bbb bb1065(bb315*bbi,bbh bbb*bbx,bbn bb5);bbb bb1884
(bb315*bbi,bbb*bb1);bba bb315 bb640;bbb bb1837(bb640*bbi);bbb bb1855(
bb640*bbi,bbb*bb1);bba bb315 bb979;bbb bb1838(bb979*bbi);bbb bb1897(
bb979*bbi,bbb*bb1);bba bb315 bb965;bbb bb1854(bb965*bbi);bbb bb1885(
bb965*bbi,bbb*bb1);bbb bb1955(bbb*bb1,bbh bbb*bbx,bbn bb5);bbb bb1916
(bbb*bb1,bbh bbb*bbx,bbn bb5);bbb bb2020(bbb*bb1,bbh bbb*bbx,bbn bb5);
bbb bb1985(bbb*bb1,bbh bbb*bbx,bbn bb5);bbb bb1967(bbb*bb1,bbh bbb*
bbx,bbn bb5);bbb bb1988(bbb*bb1,bbh bbb*bbx,bbn bb5);bbb bb2082(bbb*
bb1,bb62 bbx);bbb bb2022(bbb*bb1,bb62 bbx);bbb bb2093(bbb*bb1,bb62 bbx
);bbb bb2084(bbb*bb1,bb62 bbx);bbb bb2059(bbb*bb1,bb62 bbx);bbb bb2089
(bbb*bb1,bb62 bbx);
#ifdef __cplusplus
}
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
#ifdef __cplusplus
bbr"\x43"{
#endif
bba bbj{bbd bb5;bbd bb23[5 ];bbf bb105[64 ];}bb460;bbb bb1847(bb460*bbi
);bbb bb1394(bb460*bbi,bbh bbb*bb509,bbn bb5);bbb bb1886(bb460*bbi,
bbb*bb1);bbb bb1923(bbb*bb1,bbh bbb*bbx,bbn bb5);bbb bb2065(bbb*bb1,
bb62 bbx);
#ifdef __cplusplus
}
#endif
#ifdef __cplusplus
bbr"\x43"{
#endif
bba bbb( *bb969)(bbb*bbi);bba bbb( *bb571)(bbb*bbi,bbh bbb*bbx,bbn bb5
);bba bbb( *bb574)(bbb*bbi,bbb*bb1);bba bbj{bbe bb131;bbn bb34;bbn
bb366;
#ifdef MWLAN_REMADY_FOR_MEMORY_ERROR_EXYNOS7420
bbn bb2052;bbn bb2050;
#endif
bb969 bb584;bb571 bb333;bb574 bb573;}bb633;bbb bb2021(bb633*bbi,bbe
bb131);bba bbj{bb633 bbp;bb332{bb457 bb992;bb458 bb983;bb410 bb993;
bb315 bb960;bb461 bb1866;bb460 bb982;}bbs;}bb465;bbb bb2045(bb465*bbi
,bbe bb131);bbb bb2054(bb465*bbi);bbb bb2087(bb465*bbi,bbe bb131);bbb
bb2043(bb465*bbi,bbh bbb*bbx,bbn bb5);bbb bb2038(bb465*bbi,bbb*bb1);
bbb bb2051(bbe bb131,bbb*bb1,bbh bbb*bbx,bbn bb5);bbb bb2116(bbe bb131
,bbb*bb1,bb62 bbx);bb62 bb2029(bbe bb131);
#ifdef __cplusplus
}
#endif
#ifdef __cplusplus
bbr"\x43"{
#endif
bba bbj{bb633 bbp;bb332{bb457 bb992;bb458 bb983;bb410 bb993;bb315
bb960;bb461 bb1866;bb460 bb982;}bb560,bb1383;}bb480;bbb bb2102(bb480*
bbi,bbe bb420);bbb bb2070(bb480*bbi,bbh bbb*bb30,bbn bb100);bbb bb2176
(bb480*bbi,bbe bb420,bbh bbb*bb30,bbn bb100);bbb bb2048(bb480*bbi,bbh
bbb*bbx,bbn bb5);bbb bb2072(bb480*bbi,bbb*bb1);bbb bb2172(bbe bb420,
bbh bbb*bb30,bbn bb100,bbb*bb1,bbh bbb*bbx,bbn bb5);bbb bb2265(bbe
bb420,bb62 bb30,bbb*bb1,bb62 bbx);
#ifdef __cplusplus
}
#endif
bbb bb2102(bb480*bbi,bbe bb420){bb2021(&bbi->bbp,bb420);}bbb bb2070(
bb480*bbi,bbh bbb*bb485,bbn bb100){bb633 bbp=bbi->bbp;bbb*bb560=&bbi
->bb560, *bb1383=&bbi->bb1383;bbh bbf*bb30=(bbh bbf* )bb485;bbf bb2344
[256 ],bb1301[256 ];bbp.bb584(bb560);bbm(bb100>bbp.bb34){bbp.bb333(
bb560,bb30,bb100);bb27(bbp.bb366<=bb12(bb2344));bbp.bb573(bb560,
bb2344);bb30=bb2344;bb100=bbp.bb366;bb27(bb100<=bbp.bb34);}{bbn bbz;
bb27(bbp.bb34<=bb12(bb1301));bb91(bbz=0 ;bbz<bbp.bb34;bbz++)bb1301[bbz
]=0x36 ^(bbz<bb100?bb30[bbz]:0 );bbp.bb584(bb560);bbp.bb333(bb560,
bb1301,bbp.bb34);}{bbn bbz;bb91(bbz=0 ;bbz<bbp.bb34;bbz++)bb1301[bbz]=
0x5c ^(bbz<bb100?bb30[bbz]:0 );bbp.bb584(bb1383);bbp.bb333(bb1383,
bb1301,bbp.bb34);}}bbb bb2176(bb480*bbi,bbe bb420,bbh bbb*bb30,bbn
bb100){bb2102(bbi,bb420);bb2070(bbi,bb30,bb100);}bbb bb2048(bb480*bbi
,bbh bbb*bbx,bbn bb5){bbi->bbp.bb333(&bbi->bb560,bbx,bb5);}bbb bb2072
(bb480*bbi,bbb*bb1){bb633 bbp=bbi->bbp;bbb*bb560=&bbi->bb560, *bb1383
=&bbi->bb1383;bbp.bb573(bb560,bb1);bbp.bb333(bb1383,bb1,bbp.bb366);
bbp.bb573(bb1383,bb1);}bbb bb2172(bbe bb420,bbh bbb*bb30,bbn bb100,
bbb*bb1,bbh bbb*bbx,bbn bb5){bb480 bb82;bb2176(&bb82,bb420,bb30,bb100
);bb2048(&bb82,bbx,bb5);bb2072(&bb82,bb1);}bbb bb2265(bbe bb420,bb62
bb30,bbb*bb1,bb62 bbx){bb2172(bb420,bb30,(bbn)bb1133(bb30),bb1,bbx,(
bbn)bb1133(bbx));}
