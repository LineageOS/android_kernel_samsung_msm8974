/*
   'kmd.c' Obfuscated by COBF (Version 1.06 2006-01-07 by BB) at Fri Nov 13 10:03:51 2015
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
#ifdef __cplusplus
bbr"\x43"{
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
bba bbj{bb204 bb1156;bbn bb5;bbf bb105[16 ];bbf bb1256[16 ];bbf bb1983[
16 ];bbf bb1894[16 ];}bb631;bbb bb2109(bb631*bbi,bbh bbb*bb30,bbn bb100
);bbb bb2162(bb631*bbi,bbh bbb*bbx,bbn bb5);bbb bb2170(bb631*bbi,bbb*
bb1);
#ifdef __cplusplus
}
#endif
#ifdef __cplusplus
bbr"\x43"{
#endif
bba bbj{bbf bb368[8 *16 ];}bb340;bbb bb1199(bb340*bbi,bbh bbb*bb30);bbb
bb1323(bb340*bbi,bbh bbb*bb30);bbb bb701(bb340*bbi,bbb*bb1,bbh bbb*
bbx);bba bbj{bb340 bb668,bb991,bb1831;}bb386;bbb bb1874(bb386*bbi,bbh
bbb*bb485);bbb bb1925(bb386*bbi,bbh bbb*bb485);bbb bb1841(bb386*bbi,
bbb*bb1,bbh bbb*bbx);bbb bb1977(bb386*bbi,bbb*bb1,bbh bbb*bbx);bba bbj
{bb340 bb668,bb991;}bb385;bbb bb1853(bb385*bbi,bbh bbb*bb485);bbb
bb2018(bb385*bbi,bbh bbb*bb485);bbb bb1873(bb385*bbi,bbb*bb1,bbh bbb*
bbx);bbb bb1929(bb385*bbi,bbb*bb1,bbh bbb*bbx);
#ifdef __cplusplus
}
#endif
#ifdef __cplusplus
bbr"\x43"{
#endif
bba bbj{bbd bb368[2 *16 ];}bb428;bbb bb1809(bb428*bbi,bbh bbb*bb30);bbb
bb1939(bb428*bbi,bbh bbb*bb30);bbb bb1782(bb428*bbi,bbb*bb1,bbh bbb*
bbx);
#ifdef __cplusplus
}
#endif
#ifdef __cplusplus
bbr"\x43"{
#endif
bba bbj{bbn bb434;bbd bb368[4 * (16 +1 )];}bb384;bbb bb1263(bb384*bbi,
bbh bbb*bb30,bbn bb100);bbb bb1827(bb384*bbi,bbh bbb*bb30,bbn bb100);
bbb bb1131(bb384*bbi,bbb*bb1,bbh bbb*bbx);
#ifdef __cplusplus
}
#endif
#ifdef __cplusplus
bbr"\x43"{
#endif
bba bbb( *bb382)(bbb*bbi,bbb*bb1,bbh bbb*bbx);bba bbj bb181 bb181;bba
bbb( *bb1804)(bb181*bbi,bb3 bb1,bb81 bb151,bb79 bbx,bbn bb5);bbj bb181
{bbe bb45;bbn bb34,bb100;bbf bb136[16 ];bbn bb94;bbf bb92[16 ];bb382
bb200;bb1804 bb333;bb332{bb340 bb1783;bb386 bb1768;bb385 bb1769;bb204
bb1156;bb428 bb938;bb384 bb1791;}bbo;};bbb bb1788(bb181*bbi,bbe bb45);
bbb bb1784(bb181*bbi,bbe bb2044,bbh bbb*bb30,bbh bbb*bb515);bbb bb2101
(bb181*bbi,bbe bb45,bbh bbb*bb30,bbh bbb*bb515);bbb bb1250(bb181*bbi,
bbb*bb1,bb81 bb151,bbh bbb*bbx,bbn bb5);bbu bb1876(bb181*bbi,bbb*bb1,
bb81 bb151);bbb bb2169(bbe bb45,bbh bbb*bb30,bbh bbb*bb515,bbb*bb1902
,bb81 bb151,bbh bbb*bbx,bbn bb5);bb62 bb1957(bbe bb299);bb62 bb2036(
bbe bb527);bb62 bb2178(bbe bb45);bba bbj bb184 bb184;bba bbb( *bb1771
)(bb184*bbi,bb3 bb1,bb81 bb151,bb79 bbx,bbn bb5);bbj bb184{bbe bb45;
bbn bb34,bb100;bbn bb414;bb332{bbj{bbj{bbf bb1004[16 ];bbn bb519;bbf
bb92[16 ];}bbw;bbj{bbf bb177[16 ];}bbc;}bb2240;bbj{bbn bb1787,bb1825;
bbj{bbf bb1004[16 ];bbn bb519;bbf bb92[16 ];}bbw;bbf bb541[16 ];bbj{bbf
bb136[16 ];bbn bb94;bbf bb92[16 ];}bbc;}bb510;}bbs;bb382 bb200;bb1771
bb333;bb332{bb340 bb1783;bb386 bb1768;bb385 bb1769;bb204 bb1156;bb428
bb938;bb384 bb1791;}bbo;};bbb bb2103(bb184*bbi,bbe bb45);bbb bb2207(
bb184*bbi,bbe bb1228,bbh bbb*bb30,bbh bbb*bb515,bbn bb1821,bbn bb414);
bbb bb2237(bb184*bbi,bbe bb45,bbh bbb*bb30,bbh bbb*bb515,bbn bb1821,
bbn bb414);bbb bb2083(bb184*bbi,bbe bb1228,bbh bbb*bb30,bbh bbb*
bb1201,bbn bb977,bbn bb414,bbn bb1055,bbn bb1178);bbb bb2127(bb184*
bbi,bbe bb45,bbh bbb*bb30,bbh bbb*bb1201,bbn bb977,bbn bb414,bbn
bb1055,bbn bb1178);bbb bb2193(bb184*bbi,bbh bbb*bbx,bbn bb5);bbb
bb2152(bb184*bbi,bbb*bb1,bb81 bb151,bbh bbb*bbx,bbn bb5);bbu bb2175(
bb184*bbi,bbb*bb1333);
#ifdef __cplusplus
}
#endif
#ifdef __cplusplus
bbr"\x43"{
#endif
bba bbj{bb181 bbo;bbf bb1390[16 ],bb1256[16 ];}bb481;bbb bb2026(bb481*
bbi,bbe bb1933);bbb bb2074(bb481*bbi,bbh bbb*bb30,bbn bb100);bbb
bb2141(bb481*bbi,bbe bb420,bbh bbb*bb30,bbn bb100);bbb bb2067(bb481*
bbi,bbh bbb*bbx,bbn bb5);bbb bb2042(bb481*bbi,bbb*bb1);bbb bb2143(bbe
bb420,bbh bbb*bb30,bbn bb100,bbb*bb1,bbh bbb*bbx,bbn bb5);bbb bb2258(
bbe bb420,bb62 bb30,bbb*bb1,bb62 bbx);
#ifdef __cplusplus
}
#endif
#ifdef __cplusplus
bbr"\x43"{
#endif
bba bbb( *bb1840)(bbb*bbi,bbh bbb*bb30,bbn bb100);bba bbj{bbe bb131;
bbn bb34;bbn bb366;bb1840 bb584;bb571 bb333;bb574 bb573;}bb2096;bba
bbj{bb2096 bbp;bb332{bb480 bb2382;bb631 bb2503;bb481 bb2365;}bbs;}
bb546;bbb bb2219(bb546*bbi,bbe bb131);bbb bb2243(bb546*bbi,bbh bbb*
bb30,bbn bb100);bbb bb1881(bb546*bbi,bbe bb131,bbh bbb*bb30,bbn bb100
);bbb bb1285(bb546*bbi,bbh bbb*bbx,bbn bb5);bbb bb1861(bb546*bbi,bbb*
bb1);bbb bb2239(bbe bb131,bbh bbb*bb30,bbn bb100,bbb*bb1,bbh bbb*bbx,
bbn bb5);bbb bb2380(bbe bb131,bb62 bb30,bbb*bb1,bb62 bbx);bb62 bb2278
(bbe bb131);
#ifdef __cplusplus
}
#endif
bbb bb2219(bb546*bbi,bbe bb131){bb2096 bbp={0 };bbp.bb131=bb131;bb350(
bb131&0xff00 ){bb17 0x1000 :{bb480*bb2311=&bbi->bbs.bb2382;bb2102(
bb2311,bb131&0xff );bbp.bb34=bb2311->bbp.bb34;bbp.bb366=bb2311->bbp.
bb366;}bbp.bb584=(bb1840)bb2070;bbp.bb333=(bb571)bb2048;bbp.bb573=(
bb574)bb2072;bb21;bb17 0x2000 :bbp.bb34=16 ;bbp.bb366=16 ;bbp.bb584=(
bb1840)bb2109;bbp.bb333=(bb571)bb2162;bbp.bb573=(bb574)bb2170;bb21;
bb17 0x3000 :{bb481*bb2371=&bbi->bbs.bb2365;bb2026(bb2371,bb131&0xff );
bbp.bb34=bb2371->bbo.bb34;bbp.bb366=bb2371->bbo.bb34;}bbp.bb584=(
bb1840)bb2074;bbp.bb333=(bb571)bb2067;bbp.bb573=(bb574)bb2042;bb21;
bb474:bb27(0 );}bbi->bbp=bbp;}bbb bb2243(bb546*bbi,bbh bbb*bb30,bbn
bb100){bbi->bbp.bb584(&bbi->bbs,bb30,bb100);}bbb bb1881(bb546*bbi,bbe
bb131,bbh bbb*bb30,bbn bb100){bb2219(bbi,bb131);bb2243(bbi,bb30,bb100
);}bbb bb1285(bb546*bbi,bbh bbb*bbx,bbn bb5){bbi->bbp.bb333(&bbi->bbs
,bbx,bb5);}bbb bb1861(bb546*bbi,bbb*bb1){bbi->bbp.bb573(&bbi->bbs,bb1
);}bbb bb2239(bbe bb131,bbh bbb*bb30,bbn bb100,bbb*bb1,bbh bbb*bbx,
bbn bb5){bb546 bb82;bb1881(&bb82,bb131,bb30,bb100);bb1285(&bb82,bbx,
bb5);bb1861(&bb82,bb1);}bbb bb2380(bbe bb131,bb62 bb30,bbb*bb1,bb62
bbx){bb2239(bb131,bb30,(bbn)bb1133(bb30),bb1,bbx,(bbn)bb1133(bbx));}
bb62 bb2278(bbe bb131){bb40 bbl bbg[32 ];bb350(bb131&0xff00 ){bb17
0x1000 :bb1313(bbg,"\x48\x4d\x41\x43\x5f\x25\x73",bb2029(bb131&0xff ));
bb4 bbg;bb17 0x2000 :bb4"\x41\x45\x53\x5f\x58\x43\x42\x43";bb17 0x3000
:bb1313(bbg,"\x43\x4d\x41\x43\x5f\x25\x73",bb1957(bb131&0xff ));bb4 bbg
;}bb4 bb93;}
