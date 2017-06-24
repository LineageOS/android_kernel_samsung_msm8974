/*
   'src_ipsec_pgpIPsecESP.c' Obfuscated by COBF (Version 1.06 2006-01-07 by BB) at Fri Nov 13 10:03:51 2015
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
bba bb85 bb7;bb13{bb101=0 ,bb374=-12000 ,bb363=-11999 ,bb393=-11998 ,
bb686=-11997 ,bb724=-11996 ,bb770=-11995 ,bb911=-11994 ,bb788=-11992 ,
bb807=-11991 ,bb848=-11990 ,bb745=-11989 ,bb849=-11988 ,bb664=-11987 ,
bb680=-11986 ,bb771=-11985 ,bb714=-11984 ,bb883=-11983 ,bb666=-11982 ,
bb858=-11981 ,bb916=-11980 ,bb691=-11979 ,bb860=-11978 ,bb881=-11977 ,
bb609=-11976 ,bb872=-11975 ,bb670=-11960 ,bb929=-11959 ,bb917=-11500 ,
bb743=-11499 ,bb869=-11498 ,bb802=-11497 ,bb908=-11496 ,bb761=-11495 ,
bb652=-11494 ,bb781=-11493 ,bb906=-11492 ,bb897=-11491 ,bb825=-11490 ,
bb870=-11489 ,bb760=-11488 ,bb705=-11487 ,bb898=-11486 ,bb903=-11485 ,
bb681=-11484 ,bb712=-11483 ,bb846=-11482 ,bb662=-11481 ,bb715=-11480 ,
bb765=-11479 ,bb886=-11478 ,bb733=-11477 ,bb853=-11476 ,bb723=-11475 ,
bb812=-11474 ,bb660=-11473 ,bb873=-11472 ,bb803=-11460 ,bb717=-11450 ,
bb749=-11449 ,bb721=-11448 ,bb748=-11447 ,bb844=-11446 ,bb675=-11445 ,
bb806=-11444 ,bb829=-11443 ,bb720=-11440 ,bb876=-11439 ,bb932=-11438 ,
bb797=-11437 ,bb758=-11436 ,bb682=-11435 ,bb868=-11420 ,bb552=-11419 ,
bb589=-11418 ,bb657=-11417 ,bb644=-11416 ,bb679=-11415 ,bb799=-11414 ,
bb759=-11413 ,bb645=-11412 ,bb729=-11411 ,bb688=-11410 ,bb777=-11409 ,
bb910=-11408 ,bb753=-11407 ,bb919=-11406 ,bb905=-11405 ,bb817=-11404 ,
bb683=-11403 ,bb768=-11402 ,bb674=-11401 ,bb737=-11400 ,bb891=-11399 ,
bb841=-11398 ,bb772=-11397 ,bb694=-11396 ,bb808=-11395 ,bb726=-11394 ,
bb880=-11393 ,bb832=-11392 ,bb925=-11391 ,bb836=-11390 ,bb739=-11389 ,
bb928=-11388 ,bb734=-11387 ,bb805=-11386 ,bb775=-11385 ,bb713=-11384 ,
bb909=-11383 ,bb877=-11382 ,bb655=-11381 ,bb747=-11380 ,bb643=-11379 ,
bb843=-11378 ,bb762=-11377 ,bb831=-11376 ,bb795=-11375 ,bb882=-11374 ,
bb856=-11373 ,bb698=-11372 ,bb920=-11371 ,bb651=-11370 ,bb782=-11369 ,
bb827=-11368 ,bb769=-11367 ,bb912=-11366 ,bb757=-11365 ,bb647=-11364 ,
bb863=-11363 ,bb407=-11350 ,bb659=bb407,bb727=-11349 ,bb677=-11348 ,bb778
=-11347 ,bb656=-11346 ,bb915=-11345 ,bb703=-11344 ,bb888=-11343 ,bb875=-
11342 ,bb884=-11341 ,bb732=-11340 ,bb913=-11339 ,bb400=-11338 ,bb902=-
11337 ,bb690=bb400,bb818=-11330 ,bb923=-11329 ,bb855=-11328 ,bb878=-11327
,bb730=-11326 ,bb653=-11325 ,bb890=-11324 ,bb722=-11320 ,bb837=-11319 ,
bb773=-11318 ,bb784=-11317 ,bb650=-11316 ,bb676=-11315 ,bb767=-11314 ,
bb738=-11313 ,bb780=-11312 ,bb654=-11300 ,bb907=-11299 ,bb800=-11298 ,
bb718=-11297 ,bb865=-11296 ,bb830=-11295 ,bb857=-11294 ,bb667=-11293 ,
bb792=-11292 ,bb924=-11291 ,bb845=-11290 ,bb828=-11289 ,bb893=-11288 ,
bb851=-11287 ,bb810=-11286 ,bb648=-11285 ,bb693=-11284 ,bb754=-11283 ,
bb750=-11282 ,bb835=-11281 ,bb834=-11280 ,bb819=-11279 ,bb751=-11250 ,
bb793=-11249 ,bb700=-11248 ,bb755=-11247 ,bb786=-11246 ,bb862=-11245 ,
bb763=-11244 ,bb711=-11243 ,bb702=-11242 ,bb871=-11240 ,bb649=-11239 ,
bb744=-11238 ,bb790=-11237 ,bb687=-11150 ,bb725=-11100 ,bb796=-11099 ,
bb921=-11098 ,bb838=-11097 ,bb728=-11096 ,bb794=-11095 ,bb673=-11094 ,
bb895=-11093 ,bb822=-11092 ,bb695=-11091 ,bb931=-11090 ,bb706=-11089 ,
bb663=-11088 ,bb847=-11087 ,bb646=-11086 ,bb824=-11085 ,bb699=-11050 ,
bb742=-11049 ,bb708=-10999 ,bb809=-10998 ,bb866=-10997 ,bb716=-10996 ,
bb914=-10995 ,bb692=-10994 ,bb709=-10993 ,bb823=-10992 ,bb764=-10991 ,
bb672=-10990 ,bb783=-10989 ,bb894=-10988 ,bb892=-10979 ,bb665=-10978 ,
bb922=-10977 ,bb889=-10976 ,bb791=-10975 ,bb814=-10974 ,};bb13{bb582=1 ,};
bbb*bb518(bbd bb1249,bbd bb383);bb7 bb477(bbb*bb1005);bba bbj bb1025*
bb1023;bba bbj bb1064*bb1050;bba bbj bb1027*bb1039;bba bbj bb1069*
bb1047;bba bbj bb1048*bb1035;bba bbj bb1024*bb1063;bba bb13{bb579=0 ,
bb604=1 ,bb610=2 ,bb804=3 ,bb611=4 ,bb603=5 ,bb596=6 ,bb591=7 ,bb605=9 ,}
bb436;bba bb13{bb632=0 ,bb1026,bb626,bb1045,bb955,bb935,bb940,bb946,
bb952,bb936,bb944,}bb539;bba bbj bb470{bb3 bb76;bbd bb130;bbd bb183;
bbj bb470*bb98;}bby;bb7 bb487(bby*bb684,bbd bb933,bby*bb879,bbd bb864
,bbd bb559);bb7 bb551(bby*bbi,bbd bb96,bbh bbb*bb99,bbd bb48);bb7
bb600(bby*bbi,bbd bb96,bbb*bb132,bbd bb48);bbu bb811(bby*bbi,bbd bb96
,bbh bbb*bb99,bbd bb48);bb7 bb2124(bby*bb306,bbd*bb106);bb7 bb2185(
bby*bb90,bbu bb178,bbd bb501,bb539 bb299,bbh bbf*bb1349,bbf*bb92,
bb436 bb430,bbf*bb576,bbd bb106,bbd bb516,bby*bb61);bb7 bb2110(bby*
bb90,bbu bb178,bb539 bb299,bbh bbf*bb1349,bb436 bb430,bbf*bb576,bbd*
bb494,bbd*bb476,bbd*bb554,bby*bb61);
#define bb964 bb56(0x0800)
#define bb1173 bb56(0x0806)
#define bb963 bb56(0x01f4)
#define bb976 bb56(0x1194)
#define bb1130 bb56(0x4000)
#define bb1172 bb56(0x2000)
#define bb1145 bb56(0x1FFF)
#define bb1087( bb10) (( bb10) & bb56(0x2000 | 0x1FFF))
#define bb1032( bb10) ((( bb196( bb10)) & 0x1FFF) << 3)
#define bb1011( bb10) ((( bb10) & bb56(0x1FFF)) == 0)
#define bb511( bb10) (( bb10) & bb56(0x2000))
#define bb1068( bb10) (!( bb511( bb10)))
#pragma pack(push, 1)
bba bbj{bbf bb377[6 ];bbf bb1043[6 ];bbk bb387;}bb370, *bb391;bba bbj{
bbf bb463[6 ];bbk bb387;}bb1114, *bb1118;bba bbj{bbf bb962:4 ;bbf bb1123
:4 ;bbf bb1083;bbk bb379;bbk bb850;bbk bb602;bbf bb1037;bbf bb291;bbk
bb628;bbd bb314;bbd bb268;}bb330, *bb324;bba bbj{bbk bb1071;bbk bb1117
;bbf bb1119;bbf bb1088;bbk bb1093;bbf bb1091[6 ];bbd bb1070;bbf bb1121
[6 ];bbd bb1095;}bb1089, *bb1102;
#pragma pack(pop)
bba bbj{bbk bb290;bbk bb440;bbk bb1042;bbk bb328;}bb431, *bb362;bba
bbj{bbk bb290;bbk bb612;bbd bb568;bbd bb947;bbf bb96;bbf bb170;bbk
bb158;bbk bb328;bbk bb1041;}bb491, *bb319;bba bbj{bbf bb1110;bbf
bb1099;bbf bb1122;bbf bb1104;bbd bb1094;bbk bb1103;bbk bb383;bbd
bb1127;bbd bb1111;bbd bb1096;bbd bb1092;bbf bb1120[16 ];bbf bb1082[64 ]
;bbf bb26[128 ];bbf bb1128[64 ];}bb1081, *bb1075;bba bbj{bbd bb314;bbd
bb268;bbf bb934;bbf bb291;bbk bb941;}bb624, *bb593;
#if defined( _WIN32)
#define bb56( bbc) (((( bbc) & 0XFF00) >> 8) | ((( bbc) & 0X00FF) <<  \
8))
#define bb196( bbc) ( bb56( bbc))
#define bb456( bbc) (((( bbc) & 0XFF000000) >> 24) | ((( bbc) &  \
0X00FF0000) >> 8) | ((( bbc) & 0X0000FF00) << 8) | ((( bbc) &  \
0X000000FF) << 24))
#define bb513( bbc) ( bb456( bbc))
#endif
bbk bb951(bbh bbb*bb302);bbk bb704(bbh bbb*bb534,bbe bb22);bb7 bb616(
bby*bb90,bbf bb104,bby*bb61);bb7 bb696(bby*bb90,bbu bb178,bbf*bb424);
bb7 bb984(bby*bb61,bbf*bb403);bb7 bb989(bbh bbf*bb403,bby*bb61);bb7
bb561(bby*bb53,bbf bb104,bbd*bb968);bb7 bb954(bby*bb90,bbf bb104,bbf
bb424,bby*bb61);bbd bb535(bby*bb53);bbk bb569(bby*bb53);bbb bb547(bbk
bb152,bby*bb53);bbb bb565(bby*bb53);bbb bb1000(bby*bb53,bbd*bb29);bbb
bb1029(bby*bb53,bbd*bb29);bbb bb1059(bby*bb53,bbd bb29);bbb bb945(bby
 *bb53,bbd bb29);bbb bb1014(bby*bb53);bbu bb1049(bbf*bb53);bb13{
bb1164=-5000 ,bb1141=-4000 ,bb1033=-4999 ,bb1062=-4998 ,bb1051=-4997 ,
bb1007=-4996 ,bb1184=-4995 ,bb1115=-4994 ,bb1139=-4993 ,bb1060=-4992 ,
bb1126=-4991 };bb7 bb1165(bb7 bb1168,bbd bb1151,bbl*bb1136);bba bb13{
bb421,bb1524,}bb304;bbk bb1247(bb304 bb719,bbh bbf*bb469);bbd bb562(
bb304 bb719,bbh bbf*bb469);bbb bb1213(bbk bb159,bb304 bb581,bbf bb454
[2 ]);bbb bb1008(bbd bb159,bb304 bb581,bbf bb454[4 ]);
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
bba bbj{bbn bb434;bbd bb368[4 * (14 +1 )];}bb204;bbb bb1073(bb204*bbi,
bbh bbb*bb30,bbn bb100);bbb bb1436(bb204*bbi,bbh bbb*bb30,bbn bb100);
bbb bb1022(bb204*bbi,bbb*bb1,bbh bbb*bbx);bbb bb1636(bb204*bbi,bbb*
bb1,bbh bbb*bbx);
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
bb40 bbf bb2426[]={0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 };bb7 bb2124(bby*
bb306,bbd*bb106){bb7 bb18;bbd bb380;bbf bb76[8 ]; *bb106=0 ;bb18=bb561(
bb306,50 ,&bb380);bbm(((bb18)!=bb101))bb4 bb18;bb600(bb306,bb380,bb76,
8 ); *bb106=bb562(bb421,&(bb76[0 ]));bb4 bb18;}bb7 bb2185(bby*bb90,bbu
bb178,bbd bb501,bb539 bb299,bbh bbf*bb1349,bbf*bb92,bb436 bb430,bbf*
bb576,bbd bb106,bbd bb516,bby*bb61){bbd bb380;bbd bb163;bbd bb2442;
bbk bb152;bbd bb1339;bbd bb1777;bbd bb169;bbd bb148;bbd bb508;bbd
bb520;bbd bb1066;bbd bb157;bbd bb594;bbf bb995;bbf bb587;bbd bb100;
bbd bb2190;bbd bb34;bbd bb1017;bbf bb193=0 ;bbf bb438=0 ;bbf bb999[32 ];
bbf bb937[32 ];bbf bb2254[32 ];bbf bb2506[64 ];bbf bb598[8 +64 ];bbf*
bb1969=bb93;bbf*bb638=bb93;bby*bb74=bb93;bby*bb49=bb93;bb7 bb18=bb101
;bb181*bb557=bb93;bb546*bb548=bb93;bbe bb740=0x100000 ,bb423;bb152=
bb569(bb90);bb350(bb299){bb17 bb626:bb100=24 ;bb34=8 ;bb740|=0x11 ;bb21;
bb17 bb955:bb100=16 ;bb34=16 ;bb740|=0x80 ;bb21;bb17 bb952:bb100=16 ;bb34
=16 ;bb740|=0x91 ;bb21;bb17 bb936:bb100=24 ;bb34=16 ;bb740|=0x92 ;bb21;
bb17 bb944:bb100=32 ;bb34=16 ;bb740|=0x93 ;bb21;bb17 bb935:bb100=16 ;bb34
=16 ;bb740|=0x20 ;bb21;bb17 bb940:bb100=24 ;bb34=16 ;bb740|=0x21 ;bb21;
bb17 bb946:bb100=32 ;bb34=16 ;bb740|=0x22 ;bb21;bb17 bb632:bb100=0 ;bb34=
0 ;bb740|=0x00 ;bb21;bb474:bb4 bb589;}bb557=bb128(bb12( *bb557));bb1788
(bb557,bb740);bb696(bb90,bb178,&bb995);bb380=bb535(bb90);bb18=bb616(
bb90,50 ,bb61);bbm(((bb18)!=bb101))bb97 bb164;bbm(bb178)bb152+=bb380;
bb2190=bb152-bb380;bb1339=bb178?0 :bb380;bb1017=bb2190;bb1777=bb380+8 +
bb34;bbm(bb299!=bb632){bb3 bb1304=bb93;bb1784(bb557,0x0100 |0x1000000 ,
bb1349,bb92?bb92:bb2426);bbm(((bb18)!=bb101))bb97 bb164;bb74=bb90;
bb49=bb61;bb508=0 ;bb520=0 ;{bb157=bb74->bb130;bb594=0 ;bb110(bb157<=
bb1339&&bb74->bb98){bb594=bb157;bb74=bb74->bb98;bb157+=bb74->bb130;}
bb169=bb1339-bb594;bbm(bb157<=bb1339){bb18=bb393;bb97 bb164;}}{bb157=
bb49->bb183;bb594=0 ;bb110(bb157<=bb1777&&bb49->bb98){bb594=bb157;bb49
=bb49->bb98;bb157+=bb49->bb183;}bb148=bb1777-bb594;bbm(bb157<=bb1777){
bb18=bb393;bb97 bb164;}}bb110(bb1017>=bb34&&bb74){bbm(!bb49){bb18=
bb393;bb97 bb164;}bb508=0 ;bb520=0 ;bbm(bb74->bb130-bb169>=bb34&&bb49->
bb183-bb148>=bb34){bb1066=((bb74->bb130-bb169)<(bb49->bb183-bb148)?(
bb74->bb130-bb169):(bb49->bb183-bb148));bb1066=bb1066-bb1066%bb34;{
bbn bb366;bb1250(bb557,&bb49->bb76[bb148],&bb366,&bb74->bb76[bb169],
bb1066);bb1304=&bb49->bb76[bb148]+bb366;}bb148+=bb1066;bb169+=bb1066;
bb1017-=bb1066;bbm(bb169==bb74->bb130){bb74=bb74->bb98;bb169=0 ;}bbm(
bb148==bb49->bb183){bb49->bb130=bb49->bb183;bb49=bb49->bb98;bb148=0 ;}
}bb50{bbm(bb74->bb130-bb169<bb34){bb1969=bb999;bb110(bb1017>=bb34-
bb508&&bb508<bb34&&bb74){bb157=bb74->bb130-bb169;bbm(bb157>bb34-bb508
)bb157=bb34-bb508;bb75(&bb999[bb508],&bb74->bb76[bb169],bb157);bb508
+=bb157;bb169+=bb157;bb1017-=bb157;bbm(bb169==bb74->bb130){bb74=bb74
->bb98;bb169=0 ;}}}bb50{bb1969=&bb74->bb76[bb169];bb508=bb34;bb169+=
bb34;bb1017-=bb34;}bb638=bb49->bb183-bb148<bb34?bb937:&bb49->bb76[
bb148];bbm(bb508==bb34){bb508=0 ;{bbn bb366;bb1250(bb557,bb638,&bb366,
bb1969,bb34);bb1304=bb638+bb366;}bbm(bb638==bb937){bb520=bb49->bb183-
bb148;bb75(&bb49->bb76[bb148],bb937,bb520);bb49->bb130=bb49->bb183;
bb49=bb49->bb98;bbm(!bb49){bb18=bb393;bb97 bb164;}bb75(bb49->bb76,&
bb937[bb520],bb34-bb520);bb148=bb34-bb520;}bb50 bb148+=bb34;}}}{bbm(
bb1017){bbm(!bb74){bb18=bb1126;bb97 bb164;}bbm(bb74->bb130-bb169<
bb1017){bb75(&bb999[bb508],&bb74->bb76[bb169],bb74->bb130-bb169);
bb508+=bb74->bb130-bb169;bb1017-=bb74->bb130-bb169;bb74=bb74->bb98;
bb75(&bb999[bb508],bb74->bb76,bb1017);}bb50 bb75(&bb999[bb508],&bb74
->bb76[bb169],bb1017);bb508+=bb1017;}bbm(!bb49){bb18=bb393;bb97 bb164
;}bb638=bb49->bb183-bb148<bb34?bb937:&bb49->bb76[bb148];bbm(bb508<
bb34-1 ){bb587=bb34-bb508-2 ;bb91(bb163=0 ;bb163<bb587;bb163++)bb999[
bb163+bb508]=bb163+1 ;bb999[bb508+bb587]=bb587;bb999[bb508+bb587+1 ]=
bb995;{bbn bb366;bb1250(bb557,bb638,&bb366,bb999,bb34);bb1304=bb638+
bb366;}bbm(bb638==bb937){bb520=bb49->bb183-bb148;bb75(&bb49->bb76[
bb148],bb937,bb520);bb49->bb130=bb49->bb183;bb49=bb49->bb98;bb75(bb49
->bb76,&bb937[bb520],bb34-bb520);bb148=bb34-bb520;}bb50 bb148+=bb34;}
bb50{bb999[bb34-1 ]=1 ;{bbn bb366;bb1250(bb557,bb638,&bb366,bb999,bb34);
bb1304=bb638+bb366;}bbm(bb638==bb937){bb520=bb49->bb183-bb148;bb75(&
bb49->bb76[bb148],bb937,bb520);bb49->bb130=bb49->bb183;bb49=bb49->
bb98;bb75(bb49->bb76,&bb937[bb520],bb34-bb520);bb148=bb34-bb520;}bb50
bb148+=bb34;bb638=bb49->bb183-bb148<bb34?bb937:&bb49->bb76[bb148];{
bb91(bb163=0 ;bb163<bb34-2 ;bb163++)bb999[bb163]=bb163+2 ;bb587=bb34-1 ;
bb999[bb34-2 ]=bb587;bb999[bb34-1 ]=bb995;{bbn bb366;bb1250(bb557,bb638
,&bb366,bb999,bb34);bb1304=bb638+bb366;}}bbm(bb638==bb937){bb520=bb49
->bb183-bb148;bb75(&bb49->bb76[bb148],bb937,bb520);bb49->bb130=bb49->
bb183;bb49=bb49->bb98;bb75(bb49->bb76,&bb937[bb520],bb34-bb520);bb148
=bb34-bb520;}bb50 bb148+=bb34;}}bb152+=bb587+2 +8 +bb34;bb49->bb130=
bb148;bb27(bb1304);bb1876(bb557,bb1304,bb93);}bb50{bb18=bb487(bb90,
bb1339,bb61,bb380+8 ,bb2190);bbm(((bb18)!=bb101))bb97 bb164;bb2442=
bb380+8 +bb2190;bb587=3 -(bb2190+2 +3 )%4 ;bb91(bb163=0 ;bb163<bb587;bb163
++)bb2254[bb163]=bb163+1 ;bb2254[bb587]=bb587;bb2254[bb587+1 ]=bb995;
bb18=bb551(bb61,bb2442,bb2254,bb587+2 );bbm(((bb18)!=bb101))bb97 bb164
;bb152+=bb587+2 +8 ;}bb1008(bb106,bb421,&bb598[0 ]);bb1008(bb516,bb421,&
bb598[4 ]);bb75(&bb598[8 ],bb92?bb92:bb2426,bb34);bbm(bb92&&bb638)bb75(
bb92,bb638,bb34);bb18=bb551(bb61,bb380,bb598,8 +bb34);bbm(((bb18)!=
bb101))bb97 bb164;bb18=bb616(bb90,50 ,bb61);bbm(((bb18)!=bb101))bb97
bb164;bbm(bb178)bb945(bb61,bb501);bb547(bb152,bb61);bbm(bb430!=bb579){
bb350(bb430){bb17 bb604:bb193=16 ;bb438=16 ;bb423=0x1000 |0x10 ;bb21;bb17
bb610:bb193=20 ;bb438=20 ;bb423=0x1000 |0x20 ;bb21;bb17 bb603:bb193=32 ;
bb438=32 ;bb423=0x1000 |0x22 ;bb21;bb17 bb596:bb193=48 ;bb438=48 ;bb423=
0x1000 |0x23 ;bb21;bb17 bb591:bb193=64 ;bb438=64 ;bb423=0x1000 |0x24 ;bb21;
bb17 bb605:bb193=16 ;bb438=16 ;bb423=0x2000 ;bb21;bb17 bb611:bb193=20 ;
bb438=20 ;bb423=0x1000 |0x80 ;bb21;bb17 bb804:bb193=20 ;bb438=20 ;bb423=
0x1000 |0x30 ;bb21;bb474:bb18=bb552;bb97 bb164;}bbm(bb193>12 )bb193=12 ;
bb548=bb128(bb12( *bb548));bb1881(bb548,bb423,bb576,(bbn)bb438);bb148
=bb380;bb49=bb61;bb110(bb148>bb49->bb130){bb148-=bb49->bb130;bb49=
bb49->bb98;}bb157=bb49->bb130-bb148;bb1285(bb548,bb49->bb76+bb148,
bb157);bb110(bb157<bb152-bb380){bb49=bb49->bb98;bb157+=bb49->bb130;
bb1285(bb548,bb49->bb76,bb49->bb130);}bb1861(bb548,bb2506);bb18=bb551
(bb49,bb49->bb130,bb2506,bb193);bbm(((bb18)!=bb101))bb97 bb164;bb152
+=bb193;bb547(bb152,bb61);}bb565(bb61);bb164:bbm(bb557)bb109(bb557);
bbm(bb548)bb109(bb548);bb4 bb18;}bb7 bb2110(bby*bb90,bbu bb178,bb539
bb299,bbh bbf*bb1349,bb436 bb430,bbf*bb576,bbd*bb494,bbd*bb476,bbd*
bb554,bby*bb61){bbd bb380;bbd bb516=0 ;bbk bb152;bbd bb169;bbd bb148;
bbd bb508;bbd bb520;bbd bb1066;bbd bb1339;bbd bb1777;bbd bb157;bbd
bb594;bbd bb100;bbd bb34;bbd bb1017;bbf bb587;bbf bb995;bbf bb193=0 ;
bbf bb438=0 ;bbf*bb92=bb93;bbf bb1878[64 ];bbf bb999[32 ];bbf bb937[32 ];
bbf bb598[8 +64 ];bbf*bb1969=bb93;bbf*bb638=bb93;bby*bb74=bb93;bby*bb49
=bb93;bby*bb483=bb93;bb7 bb18=bb101;bb181*bb557=bb93;bb546*bb548=bb93
;bbe bb740=0x100000 ,bb423;bb152=bb569(bb90);bb350(bb299){bb17 bb626:
bb100=24 ;bb34=8 ;bb740|=0x11 ;bb21;bb17 bb955:bb100=16 ;bb34=16 ;bb740|=
0x80 ;bb21;bb17 bb952:bb100=16 ;bb34=16 ;bb740|=0x91 ;bb21;bb17 bb936:
bb100=24 ;bb34=16 ;bb740|=0x92 ;bb21;bb17 bb944:bb100=32 ;bb34=16 ;bb740|=
0x93 ;bb21;bb17 bb935:bb100=16 ;bb34=16 ;bb740|=0x20 ;bb21;bb17 bb940:
bb100=24 ;bb34=16 ;bb740|=0x21 ;bb21;bb17 bb946:bb100=32 ;bb34=16 ;bb740|=
0x22 ;bb21;bb17 bb632:bb100=0 ;bb34=0 ;bb740|=0x00 ;bb21;bb474:bb4 bb589;
}bb557=bb128(bb12( *bb557));bb1788(bb557,bb740);bb18=bb561(bb90,50 ,&
bb380);bbm(((bb18)!=bb101))bb97 bb164;bb600(bb90,bb380,bb598,8 +bb34);
bbm( *bb476<(bb12( *bb494)<<3 )-1 ) *bb476=(bb12( *bb494)<<3 )-1 ; *bb554
= *bb476-(bb12( *bb494)<<3 )+1 ;bb516=bb562(bb421,&bb598[4 ]);bbm(bb516<
 *bb554){bb18=bb1062;bb97 bb164;}bbm(bb516<= *bb476&& *bb494&1 <<(
bb516- *bb554)){bb18=bb1033;bb97 bb164;}bbm(bb430!=bb579){bb350(bb430
){bb17 bb604:bb193=16 ;bb438=16 ;bb423=0x1000 |0x10 ;bb21;bb17 bb610:
bb193=20 ;bb438=20 ;bb423=0x1000 |0x20 ;bb21;bb17 bb603:bb193=32 ;bb438=32
;bb423=0x1000 |0x22 ;bb21;bb17 bb596:bb193=48 ;bb438=48 ;bb423=0x1000 |
0x23 ;bb21;bb17 bb591:bb193=64 ;bb438=64 ;bb423=0x1000 |0x24 ;bb21;bb17
bb605:bb193=16 ;bb438=16 ;bb423=0x2000 ;bb21;bb17 bb611:bb193=20 ;bb438=
20 ;bb423=0x1000 |0x80 ;bb21;bb17 bb804:bb193=20 ;bb438=20 ;bb423=0x1000 |
0x30 ;bb21;bb474:bb18=bb552;bb97 bb164;}bbm(bb193>12 )bb193=12 ;bb548=
bb128(bb12( *bb548));bb1881(bb548,bb423,bb576,bb438);bb74=bb90;bb169=
bb380;bb157=bb74->bb130>(bbd)(bb152-bb193)?bb152-bb380-bb193:bb74->
bb130-bb169;bb1285(bb548,bb74->bb76+bb169,bb157);bb169+=bb157;bb110(
bb157<bb152-bb380-bb193){bb74=bb74->bb98;bb594=bb157;bb157+=bb74->
bb130;bb169=bb157<=bb152-bb380-bb193?bb74->bb130:bb152-bb380-bb193-
bb594;bb1285(bb548,bb74->bb76,bb169);}bb1861(bb548,bb1878);bbm(!bb811
(bb90,bb152-bb193,bb1878,bb193)){bb18=bb1051;bb97 bb164;}bb152-=bb193
;bbm(bb169+bb193>bb74->bb130)bb74->bb98->bb130=0 ;bb74->bb130=bb169;}
bbm(bb299!=bb632){bb3 bb1304=bb93;bb92=(bbf* )bb518(bb34,bb582);bbm(!
bb92){bb18=bb363;bb97 bb164;}bb75(bb92,&bb598[8 ],bb34);bb1784(bb557,
0x0200 |0x1000000 ,bb1349,bb92);bb74=bb90;bb49=bb61;bb508=0 ;bb520=0 ;
bb1339=bb380+8 +bb34;bb1017=bb152-bb1339;bb1777=bb178?0 :bb380;{bb157=
bb74->bb130;bb594=0 ;bb110(bb157<=bb1339&&bb74->bb98){bb594=bb157;bb74
=bb74->bb98;bb157+=bb74->bb130;}bb169=bb1339-bb594;bbm(bb157<=bb1339){
bb18=bb393;bb97 bb164;}}{bb157=bb49->bb183;bb594=0 ;bb110(bb157<=
bb1777&&bb49->bb98){bb594=bb157;bb49=bb49->bb98;bb157+=bb49->bb183;}
bb148=bb1777-bb594;bbm(bb157<=bb1777){bb18=bb393;bb97 bb164;}}bb110(
bb1017>=bb34&&bb74){bbm(!bb49){bb18=bb393;bb97 bb164;}bb508=0 ;bb520=0
;bbm(bb74->bb130-bb169>=bb34&&bb49->bb183-bb148>=bb34){bb1066=((bb74
->bb130-bb169)<(bb49->bb183-bb148)?(bb74->bb130-bb169):(bb49->bb183-
bb148));bb1066=bb1066-bb1066%bb34;{bbn bb366;bb1250(bb557,&bb49->bb76
[bb148],&bb366,&bb74->bb76[bb169],bb1066);bb1304=&bb49->bb76[bb148]+
bb366;}bb148+=bb1066;bb169+=bb1066;bb1017-=bb1066;bbm(bb169==bb74->
bb130){bb74=bb74->bb98;bb169=0 ;}bbm(bb148==bb49->bb183){bb49->bb130=
bb49->bb183;bb483=bb49;bb49=bb49->bb98;bb148=0 ;}}bb50{bbm(bb74->bb130
-bb169<bb34){bb1969=bb999;bb110(bb508<bb34&&bb74){bb157=bb74->bb130-
bb169;bbm(bb157>bb34-bb508)bb157=bb34-bb508;bb75(&bb999[bb508],&bb74
->bb76[bb169],bb157);bb508+=bb157;bb169+=bb157;bb1017-=bb157;bbm(
bb169==bb74->bb130){bb74=bb74->bb98;bb169=0 ;}}}bb50{bb1969=&bb74->
bb76[bb169];bb508=bb34;bb169+=bb34;bb1017-=bb34;}bb638=bb49->bb183-
bb148<bb34?bb937:&bb49->bb76[bb148];bbm(bb508==bb34){bb508=0 ;{bbn
bb366;bb1250(bb557,bb638,&bb366,bb1969,bb34);bb1304=bb638+bb366;}bbm(
bb638==bb937){bb520=bb49->bb183-bb148;bb75(&bb49->bb76[bb148],bb937,
bb520);bb49->bb130=bb49->bb183;bb483=bb49;bb49=bb49->bb98;bbm(!bb49){
bb18=bb393;bb97 bb164;}bb75(bb49->bb76,&bb937[bb520],bb34-bb520);
bb148=bb34-bb520;}bb50 bb148+=bb34;}}}bb27(bb1304);bb1876(bb557,
bb1304,bb93);}bb50{bb18=bb487(bb90,bb380+8 +bb34,bb61,bb178?0 :bb380,
bb152-bb380-8 );bbm(((bb18)!=bb101))bb97 bb164;bb49=bb61;bb483=bb93;
bb110(bb49->bb98){bbm(bb49->bb98->bb130==0 )bb21;bb483=bb49;bb49=bb49
->bb98;}bb148=bb49->bb130;}bbm(bb516> *bb476){ *bb494>>=bb516- *bb476
; *bb494&=0x7fffffff >>(bb516- *bb476-1 ); *bb476=bb516; *bb554= *bb476
-(bb12( *bb494)<<3 )+1 ;} *bb494|=1 <<(bb516- *bb554);bbm(bb148>1 ){bb587
=bb49->bb76[bb148-2 ];bb995=bb49->bb76[bb148-1 ];}bb50 bbm(bb148==1 ){
bb587=bb483->bb76[bb483->bb130-1 ];bb995=bb49->bb76[0 ];}bb50{bb587=
bb483->bb76[bb483->bb130-2 ];bb995=bb483->bb76[bb483->bb130-1 ];}bbm(
bb148>(bbd)(bb587+2 )){bb148-=bb587+2 ;bb49->bb130=bb148;}bb50{bbm(!
bb483){bb18=bb393;bb97 bb164;}bb49->bb130=0 ;bb483->bb130-=bb587+2 -
bb148;bb148=bb483->bb130;bb49=bb483;}bbm(bb995==4 &&!bb178){bb178=1 ;
bb18=bb487(bb61,bb380,bb61,0 ,bb152-(bb380+8 +bb34)-(bb587+2 ));bbm(((
bb18)!=bb101))bb97 bb164;}bb152-=(bb178?bb380:0 )+8 +bb34+bb587+2 ;bbm(!
bb178){bb18=bb954(bb90,50 ,bb995,bb61);bbm(((bb18)!=bb101))bb97 bb164;
}bb547(bb152,bb61);bb565(bb61);bb164:bbm(bb92)bb477(bb92);bbm(bb557)bb109
(bb557);bbm(bb548)bb109(bb548);bb4 bb18;}
