/*
   'src_pm_ftp_nat.c' Obfuscated by COBF (Version 1.06 2006-01-07 by BB) at Fri Nov 13 10:03:51 2015
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
bba bb13{bb421,bb1524,}bb304;bbk bb1247(bb304 bb719,bbh bbf*bb469);
bbd bb562(bb304 bb719,bbh bbf*bb469);bbb bb1213(bbk bb159,bb304 bb581
,bbf bb454[2 ]);bbb bb1008(bbd bb159,bb304 bb581,bbf bb454[4 ]);
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
bbk bb951(bbh bbb*bb302);bbk bb704(bbh bbb*bb534,bbe bb22);bba bb85
bb7;bb13{bb101=0 ,bb374=-12000 ,bb363=-11999 ,bb393=-11998 ,bb686=-11997 ,
bb724=-11996 ,bb770=-11995 ,bb911=-11994 ,bb788=-11992 ,bb807=-11991 ,
bb848=-11990 ,bb745=-11989 ,bb849=-11988 ,bb664=-11987 ,bb680=-11986 ,
bb771=-11985 ,bb714=-11984 ,bb883=-11983 ,bb666=-11982 ,bb858=-11981 ,
bb916=-11980 ,bb691=-11979 ,bb860=-11978 ,bb881=-11977 ,bb609=-11976 ,
bb872=-11975 ,bb670=-11960 ,bb929=-11959 ,bb917=-11500 ,bb743=-11499 ,
bb869=-11498 ,bb802=-11497 ,bb908=-11496 ,bb761=-11495 ,bb652=-11494 ,
bb781=-11493 ,bb906=-11492 ,bb897=-11491 ,bb825=-11490 ,bb870=-11489 ,
bb760=-11488 ,bb705=-11487 ,bb898=-11486 ,bb903=-11485 ,bb681=-11484 ,
bb712=-11483 ,bb846=-11482 ,bb662=-11481 ,bb715=-11480 ,bb765=-11479 ,
bb886=-11478 ,bb733=-11477 ,bb853=-11476 ,bb723=-11475 ,bb812=-11474 ,
bb660=-11473 ,bb873=-11472 ,bb803=-11460 ,bb717=-11450 ,bb749=-11449 ,
bb721=-11448 ,bb748=-11447 ,bb844=-11446 ,bb675=-11445 ,bb806=-11444 ,
bb829=-11443 ,bb720=-11440 ,bb876=-11439 ,bb932=-11438 ,bb797=-11437 ,
bb758=-11436 ,bb682=-11435 ,bb868=-11420 ,bb552=-11419 ,bb589=-11418 ,
bb657=-11417 ,bb644=-11416 ,bb679=-11415 ,bb799=-11414 ,bb759=-11413 ,
bb645=-11412 ,bb729=-11411 ,bb688=-11410 ,bb777=-11409 ,bb910=-11408 ,
bb753=-11407 ,bb919=-11406 ,bb905=-11405 ,bb817=-11404 ,bb683=-11403 ,
bb768=-11402 ,bb674=-11401 ,bb737=-11400 ,bb891=-11399 ,bb841=-11398 ,
bb772=-11397 ,bb694=-11396 ,bb808=-11395 ,bb726=-11394 ,bb880=-11393 ,
bb832=-11392 ,bb925=-11391 ,bb836=-11390 ,bb739=-11389 ,bb928=-11388 ,
bb734=-11387 ,bb805=-11386 ,bb775=-11385 ,bb713=-11384 ,bb909=-11383 ,
bb877=-11382 ,bb655=-11381 ,bb747=-11380 ,bb643=-11379 ,bb843=-11378 ,
bb762=-11377 ,bb831=-11376 ,bb795=-11375 ,bb882=-11374 ,bb856=-11373 ,
bb698=-11372 ,bb920=-11371 ,bb651=-11370 ,bb782=-11369 ,bb827=-11368 ,
bb769=-11367 ,bb912=-11366 ,bb757=-11365 ,bb647=-11364 ,bb863=-11363 ,
bb407=-11350 ,bb659=bb407,bb727=-11349 ,bb677=-11348 ,bb778=-11347 ,bb656
=-11346 ,bb915=-11345 ,bb703=-11344 ,bb888=-11343 ,bb875=-11342 ,bb884=-
11341 ,bb732=-11340 ,bb913=-11339 ,bb400=-11338 ,bb902=-11337 ,bb690=bb400
,bb818=-11330 ,bb923=-11329 ,bb855=-11328 ,bb878=-11327 ,bb730=-11326 ,
bb653=-11325 ,bb890=-11324 ,bb722=-11320 ,bb837=-11319 ,bb773=-11318 ,
bb784=-11317 ,bb650=-11316 ,bb676=-11315 ,bb767=-11314 ,bb738=-11313 ,
bb780=-11312 ,bb654=-11300 ,bb907=-11299 ,bb800=-11298 ,bb718=-11297 ,
bb865=-11296 ,bb830=-11295 ,bb857=-11294 ,bb667=-11293 ,bb792=-11292 ,
bb924=-11291 ,bb845=-11290 ,bb828=-11289 ,bb893=-11288 ,bb851=-11287 ,
bb810=-11286 ,bb648=-11285 ,bb693=-11284 ,bb754=-11283 ,bb750=-11282 ,
bb835=-11281 ,bb834=-11280 ,bb819=-11279 ,bb751=-11250 ,bb793=-11249 ,
bb700=-11248 ,bb755=-11247 ,bb786=-11246 ,bb862=-11245 ,bb763=-11244 ,
bb711=-11243 ,bb702=-11242 ,bb871=-11240 ,bb649=-11239 ,bb744=-11238 ,
bb790=-11237 ,bb687=-11150 ,bb725=-11100 ,bb796=-11099 ,bb921=-11098 ,
bb838=-11097 ,bb728=-11096 ,bb794=-11095 ,bb673=-11094 ,bb895=-11093 ,
bb822=-11092 ,bb695=-11091 ,bb931=-11090 ,bb706=-11089 ,bb663=-11088 ,
bb847=-11087 ,bb646=-11086 ,bb824=-11085 ,bb699=-11050 ,bb742=-11049 ,
bb708=-10999 ,bb809=-10998 ,bb866=-10997 ,bb716=-10996 ,bb914=-10995 ,
bb692=-10994 ,bb709=-10993 ,bb823=-10992 ,bb764=-10991 ,bb672=-10990 ,
bb783=-10989 ,bb894=-10988 ,bb892=-10979 ,bb665=-10978 ,bb922=-10977 ,
bb889=-10976 ,bb791=-10975 ,bb814=-10974 ,};bba bbj bb470{bb3 bb76;bbd
bb130;bbd bb183;bbj bb470*bb98;}bby;bb7 bb487(bby*bb684,bbd bb933,bby
 *bb879,bbd bb864,bbd bb559);bb7 bb551(bby*bbi,bbd bb96,bbh bbb*bb99,
bbd bb48);bb7 bb600(bby*bbi,bbd bb96,bbb*bb132,bbd bb48);bbu bb811(
bby*bbi,bbd bb96,bbh bbb*bb99,bbd bb48);bb7 bb616(bby*bb90,bbf bb104,
bby*bb61);bb7 bb696(bby*bb90,bbu bb178,bbf*bb424);bb7 bb984(bby*bb61,
bbf*bb403);bb7 bb989(bbh bbf*bb403,bby*bb61);bb7 bb561(bby*bb53,bbf
bb104,bbd*bb968);bb7 bb954(bby*bb90,bbf bb104,bbf bb424,bby*bb61);bbd
bb535(bby*bb53);bbk bb569(bby*bb53);bbb bb547(bbk bb152,bby*bb53);bbb
bb565(bby*bb53);bbb bb1000(bby*bb53,bbd*bb29);bbb bb1029(bby*bb53,bbd
 *bb29);bbb bb1059(bby*bb53,bbd bb29);bbb bb945(bby*bb53,bbd bb29);
bbb bb1014(bby*bb53);bbu bb1049(bbf*bb53);
#if defined( bb308) && defined( _WIN32)
#include"uncobf.h"
#include<stdio.h>
#include"cobf.h"
#endif
bba bbj{bbu bb1155;bbd bb268;bbk bb290;bbk bb440;bbd bb2223;bbd bb1871
;bbd bb2008;}bb2560, *bb2679;bb40 bbd bb2066=0 ;bb40 bb2560 bb486[5 ];
bb40 bbe bb2295(bbd bb268,bbk bb290,bbk bb440){bbe bbz;bb91(bbz=0 ;bbz
<(bbe)(bb12(bb486)/bb12((bb486)[0 ]));bbz++){bbm(bb486[bbz].bb1155&&
bb486[bbz].bb268==bb268&&bb486[bbz].bb290==bb290&&bb486[bbz].bb440==
bb440)bb4 bbz;}bb4-1 ;}bb40 bbe bb2444(){bbd bb2069=0xFFFFFFFF ;bbe bbz
,bb2498=0 ;bb91(bbz=0 ;bbz<(bbe)(bb12(bb486)/bb12((bb486)[0 ]));bbz++){
bbm(!bb486[bbz].bb1155)bb4 bbz;bbm(bb486[bbz].bb2008>=bb2069)bb2498=
bbz;}bb4 bb2498;}bb40 bbe bb2628(bbd bb268,bbk bb290,bbk bb440,bbd
bb2223,bbd bb1871){bbe bbz=bb2444();bb486[bbz].bb1155=1 ;bb486[bbz].
bb268=bb268;bb486[bbz].bb290=bb290;bb486[bbz].bb440=bb440;bb486[bbz].
bb2223=bb2223;bb486[bbz].bb1871=bb1871;bb486[bbz].bb2008=bb2066;bb4
bbz;}bb40 bbe bb2438(bbe bbz,bbd bb2538,bbd bb2540){bb486[bbz].bb2223
=bb2538;bb486[bbz].bb1871+=bb2540;bbm(bb486[bbz].bb2008==bb2066)bb4
bbz;bb2066++;bb486[bbz].bb2008=bb2066;bbm(bb2066==0xFFFFFFFF ){bbe bb77
;bbd bb2069=0xFFFFFFFF ;bb91(bb77=0 ;bb77<(bbe)(bb12(bb486)/bb12((bb486
)[0 ]));bb77++){bbm(bb486[bb77].bb2008<bb2069)bb2069=bb486[bb77].
bb2008;}bb91(bb77=0 ;bb77<(bbe)(bb12(bb486)/bb12((bb486)[0 ]));bb77++)bb486
[bbz].bb2008-=bb2069;bb2066-=bb2069;}bb4 bbz;}bb40 bb3 bb2564(bb3
bb1338,bbd*bb2585){bbe bbz,bb379=0 ;bb91(bbz=0 ;bbz<4 ;){bbm( *bb1338++
==',')bbz++;bb379++;} *bb2585=bb379;bb4 bb1338;}bb40 bbb bb2643(bbl*
bb2206,bbd bb1098,bbd*bb2526){bbl bbg[32 ];bbd bb11=bb1313(bbg,"\x25"
"\x64\x2c\x25\x64\x2c\x25\x64\x2c\x25\x64\x2c",bb1098&0xff ,bb1098>>8 &
0xff ,bb1098>>16 &0xff ,bb1098>>24 &0xff );bb75(bb2206,bbg,bb11); *bb2526=
bb11;}bbe bb2499(bb324 bb140,bbd bb1098){bb319 bb201;bb3 bb1338;bb3
bb455;bbl bb2206[20 ];bbd bb2356;bbd bb379;bbd bb1879;bbe bb163;bbd
bb268;bbk bb290;bbk bb440;bbd bb568;bbd bb2232;bbd bb2257;bb201=(
bb319)(bb140+1 );bbm(bb196(bb201->bb612)==21 ){bb2257=(bb201->bb96>>4 )<<
2 ;bb1338=(bb3)bb201+bb2257;bb268=bb562(bb421,(bb3)&bb140->bb268);
bb290=bb196(bb201->bb290);bb440=bb196(bb201->bb612);bb568=bb562(bb421
,(bb3)&bb201->bb568);bb2232=bb568+bb196(bb140->bb379)-bb12(bb330)-
bb2257;bbm(bb1863(bb1338,"\x50\x4f\x52\x54\x20",5 )||bb1863(bb1338,""
"\x70\x6f\x72\x74\x20",5 )){bb1338+=5 ;bb455=bb2564(bb1338,&bb379);
bb2356=bb196(bb140->bb379)-bb12(bb330)-bb2257-5 -bb379;bb2643(bb2206,
bb513(bb1098),&bb1879);bb2443(bb1338+bb1879,bb455,bb2356);bb75(bb1338
,bb2206,bb1879);{bbd bb11=bb140->bb379;bb11=bb196(bb11)+bb1879-bb379;
bb140->bb379=bb56(bb11);} * (bb1338+bb1879+bb2356)=0 ;bb163=bb2295(
bb268,bb290,bb440);bbm(bb163==-1 ){bb163=bb2444();bb2628(bb268,bb290,
bb440,bb2232,bb1879-bb379);}bb50{bb1008(bb568+bb486[bb163].bb1871,
bb421,(bb3)&bb201->bb568);bb2438(bb163,bb2232,bb1879-bb379);}bb4
bb1879-bb379;}bb163=bb2295(bb268,bb290,bb440);bbm(bb163!=-1 ){bb1008(
bb568+bb486[bb163].bb1871,bb421,(bb3)&bb201->bb568);bb2438(bb163,
bb2232,0 );bbm(bb201->bb170&0x01 )bb486[bb163].bb1155=0 ;}}bb4 0 ;}bbe
bb2384(bb324 bb140){bb319 bb201;bbd bb314;bbk bb290;bbk bb440;bbd
bb2313;bb201=(bb319)(bb140+1 );bbm(bb196(bb201->bb290)==21 ){bbe bbz;
bb314=bb562(bb421,(bb3)&bb140->bb314);bb290=bb196(bb201->bb290);bb440
=bb196(bb201->bb612);bb2313=bb562(bb421,(bb3)&bb201->bb947);bbz=
bb2295(bb314,bb440,bb290);bbm(bbz!=-1 ){bb2313-=bb486[bbz].bb1871;
bb1008(bb2313,bb421,(bb3)&bb201->bb947);bb4 bb486[bbz].bb1871;}}bb4 0
;}
