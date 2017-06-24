/*
   'src_compress_LZS_lzsc.c' Obfuscated by COBF (Version 1.06 2006-01-07 by BB) at Fri Nov 13 10:03:51 2015
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
bba bbj bb1915 bb1915;bba bbj bb1915*bb496;bbd bb2269(bbb);bbb bb2211
(bb496 bb2,bbb*bb1354);bbk bb2249(bb496 bb2,bbf* *bb1770,bbf* *bb1835
,bbd*bb942,bbd*bb636,bbb*bb1354,bbk bb383,bbk bb2233);bbk bb2366(
bb496 bb2,bbf* *bb1770,bbf* *bb1835,bbd*bb942,bbd*bb636,bbb*bb1354,
bbk bb383);
#define bb203 1
#define bb202 0
#define bb1292 bb203
#define bb2453 bb202
bba bbj{bbf*bb1789;bbd bb1021;bbf*bb1774;bbd bb625;bbd bb1275;bbd
bb1776;bbk bb2115;bbf bb1966;bbf bb2052;bbk bb1705;bbd bb563;bbd
bb1890;bbk bb1297;bbd*bb2340;bbd bb2363;bbk bb1409;bbk bb2320;bbk
bb2117;bbk bb2166;bbk bb2125;bbk bb2183;bbk bb1054;bb125 bb1090;bbf
bb1724;bbf bb2050;bb125 bb2001;bbk bb2061;bbk bb1880;}bb2588;bba bbj{
bbf*bb1789;bbd bb1021;bbf*bb1774;bbd bb625;bbk bb2158;bbk bb1795;bbk
bb1206;bbk bb2652;bb125 bb1189;bbk bb1186;bbk bb1056;bb125 bb1080;
bb125 bb96;bbd bb479;bbk bb2030;}bb2613;bba bbj{bbf bb1893[2048 ];bbk
bb1927[2048 ];bbd bb2151[4096 ];bb2588 bb46;bbf bb2236[2048 ];bb2613 bb86
;bbf bb1188[64 ];}bb2112;bbj bb1915{bb2112*bb468;bb2112 bb14;bbk bb2180
;bbk bb1781;};bba bbj bb2664{bbk bb2385;bb125 bb48;}bb2528;bb40 bb2528
bb2228[24 ]={{0x0 ,0 },{0x0 ,0 },{0x0 ,2 },{0x1 ,2 },{0x2 ,2 },{0xC ,4 },{0xD ,4 },{
0xE ,4 },{0xF0 ,8 },{0xF1 ,8 },{0xF2 ,8 },{0xF3 ,8 },{0xF4 ,8 },{0xF5 ,8 },{0xF6 ,8 }
,{0xF7 ,8 },{0xF8 ,8 },{0xF9 ,8 },{0xFA ,8 },{0xFB ,8 },{0xFC ,8 },{0xFD ,8 },{0xFE
,8 }};bb40 bbk bb2557[5 ][3 ]={{1 ,1 ,1 },{1 ,1 ,1 },{0 ,0 ,1 },{0 ,0 ,1 },{0 ,1 ,1 }};
bb13{bb2301,bb2642,bb2567,bb2551,bb2466};bb13{bb2150,bb2422,bb2459,
bb2425,bb2519,bb2518,bb2501,bb2310,bb2440};bb40 bbb bb2289(bb496 bb2);
bb40 bbb bb2450(bb496 bb2);bb40 bbb bb1135(bb496 bb2);bb40 bbb bb2455
(bb496 bb2);bb40 bbb bb2210(bbd*bb2486,bbd bb2396);bb40 bbb bb2144(
bb496 bb2,bbk bb2229,bbd bb2167);bb40 bbb bb2198(bb496 bb2);bb40 bbk
bb2354(bb496 bb2);bb40 bbk bb2017(bb496 bb2);bb40 bbb bb2138(bb496 bb2
,bbf bbo);bbd bb2269(bbb){bb4 bb12(bb1915);}bb40 bbb bb2289(bb496 bb2
){bb2->bb14.bb46.bb2001=8 ;bb2->bb14.bb46.bb1724=0 ;bb2->bb14.bb46.
bb1409=bb2->bb14.bb46.bb1705=0 ;bb2->bb14.bb46.bb563=bb2->bb14.bb46.
bb1890=0 ;bb2->bb14.bb46.bb2183=bb2->bb14.bb46.bb1880=0 ;bb4;}bbb bb2211
(bb496 bb2,bbb*bb1354){bb2->bb468=(bb2112* )bb1354;bb2->bb14.bb46=bb2
->bb468->bb46;bb2->bb14.bb46.bb1275=0xFFFFFFFFL ;bb2->bb14.bb46.bb1776
=bb2->bb14.bb46.bb1275-1 ;bb2->bb14.bb46.bb2061=0 ;bb2289(bb2);bb2210(
bb2->bb468->bb2151,0xC0000000L );bb2->bb468->bb46=bb2->bb14.bb46;bb2->
bb14.bb86=bb2->bb468->bb86;bb2->bb14.bb86.bb2158=0 ;bb2198(bb2);bb2->
bb468->bb86=bb2->bb14.bb86;bb4;}bb40 bbb bb2210(bbd*bb2486,bbd bb2396
){bbk bbz;bb91(bbz=0 ;bbz<4096 ;bbz++) *bb2486++=bb2396;bb4;}bb40 bbb
bb2450(bb496 bb2){bb950 bbk bbz;bbd*bb2205;bbd bb2298;bb2298=
0xC0000000L ;bbm(bb2->bb14.bb46.bb1275!=0 )bb2298=0x40000000L ;bb2205=
bb2->bb468->bb2151;bb91(bbz=0 ;bbz<4096 ;bbz++,bb2205++)bbm(bb2->bb14.
bb46.bb1275- *bb2205>2048 -2 ) *bb2205=bb2298;bb4;}bb40 bbb bb1135(
bb496 bb2){bb950 bb125 bb2028;bbm(bb2->bb14.bb46.bb625==0 )bb2->bb14.
bb46.bb2061=1 ;bbm((bb2028=bb2->bb14.bb46.bb1090-bb2->bb14.bb46.bb2001
)>0 ){bb2->bb14.bb46.bb1724|=(bb2->bb14.bb46.bb1054>>bb2028);bb2->bb14
.bb46.bb2001=8 ; *bb2->bb14.bb46.bb1774++=bb2->bb14.bb46.bb1724;--bb2
->bb14.bb46.bb625;bb2->bb14.bb46.bb1724=0 ;bb2->bb14.bb46.bb1054&=((1
<<bb2028)-1 );bb2->bb14.bb46.bb1090=bb2028;bb1135(bb2);}bb50 bbm(
bb2028<0 ){bb2->bb14.bb46.bb1724|=(bb2->bb14.bb46.bb1054<<-bb2028);bb2
->bb14.bb46.bb2001-=bb2->bb14.bb46.bb1090;}bb50{bb2->bb14.bb46.bb1724
|=bb2->bb14.bb46.bb1054;bb2->bb14.bb46.bb2001=8 ; *bb2->bb14.bb46.
bb1774++=bb2->bb14.bb46.bb1724;--bb2->bb14.bb46.bb625;bb2->bb14.bb46.
bb1724=0 ;}bb4;}bb40 bbb bb2455(bb496 bb2){bb2->bb14.bb46.bb1054=(
0x180 );bb2->bb14.bb46.bb1090=(9 );bb1135(bb2);bbm(bb2->bb14.bb46.
bb2001!=8 ){ *bb2->bb14.bb46.bb1774++=bb2->bb14.bb46.bb1724;--bb2->
bb14.bb46.bb625;bbm(bb2->bb14.bb46.bb625==0 ){bb2->bb14.bb46.bb2061=1 ;
}}bb2->bb14.bb46.bb2001=8 ;bb2->bb14.bb46.bb1724=0 ;bb4;}bb40 bbb bb2144
(bb496 bb2,bbk bb2229,bbd bb2167){bbm(bb2->bb14.bb46.bb1880==0 ){bbm(
bb2229<128 ){bb2->bb14.bb46.bb1054=(0x180 |bb2229);bb2->bb14.bb46.
bb1090=(9 );bb1135(bb2);}bb50{bb2->bb14.bb46.bb1054=(0x1000 |bb2229);
bb2->bb14.bb46.bb1090=(13 );bb1135(bb2);}}bbm(bb2167>=23 ){bb2->bb14.
bb46.bb1054=(((1 <<4 )-1 ));bb2->bb14.bb46.bb1090=(4 );bb1135(bb2);bb2167
-=((1 <<4 )-1 );bb2->bb14.bb46.bb1890+=((1 <<4 )-1 );bb2->bb14.bb46.bb1880=
1 ;}bb50{bb2->bb14.bb46.bb1054=(bb2228[(bbk)bb2167].bb2385);bb2->bb14.
bb46.bb1090=(bb2228[(bbk)bb2167].bb48);bb1135(bb2);bb2->bb14.bb46.
bb1880=0 ;bb2->bb14.bb46.bb563=0 ;bb2->bb14.bb46.bb1890=0 ;bb2->bb1781=
bb2466;}bb4;}bb40 bbb bb2267(bb496 bb2,bbk bb383){bbm(bb2->bb14.bb46.
bb2183==1 ){bbm(bb2->bb14.bb46.bb563==0 ){bb2->bb14.bb46.bb1054=((bb2->
bb14.bb46.bb1966));bb2->bb14.bb46.bb1090=(9 );bb1135(bb2);bb2->bb1781=
bb2301;}bb50 bb2144(bb2,bb2->bb14.bb46.bb1409,bb2->bb14.bb46.bb563);}
bb2455(bb2);bbm((bb383&0x04 )==0 ){bbm(0 -bb2->bb14.bb46.bb1275>=2048 )bb2210
(bb2->bb468->bb2151,0xC0000000L );bb50 bbm(0x80000000L -bb2->bb14.bb46.
bb1275>=2048 )bb2210(bb2->bb468->bb2151,0x40000000L );bb2->bb14.bb46.
bb1275+=2048 ;bb2->bb14.bb46.bb1776+=2048 ;bb2289(bb2);}bb2->bb14.bb46.
bb2183=0 ;bb4;}bbk bb2249(bb496 bb2,bbf* *bb1770,bbf* *bb1835,bbd*
bb942,bbd*bb636,bbb*bb1354,bbk bb383,bbk bb2233){bb950 bbk bb2256;
bb950 bbk bb2107;bbk bb2242;bbk bb1077=0 ;bb2->bb468=(bb2112* )bb1354;
bb2->bb14.bb46=bb2->bb468->bb46;bb2->bb14.bb46.bb1789= *bb1770;bb2->
bb14.bb46.bb1021= *bb942;bb2->bb14.bb46.bb1774= *bb1835;bb2->bb14.
bb46.bb625= *bb636;bb2->bb14.bb46.bb2061=0 ;bb2->bb2180=0X0018 &bb383;
bbm(bb2->bb2180>0x0010 ){bb2->bb2180=0x0010 ;}bb2->bb2180>>=3 ;bbm( *
bb636<=15 )bb1077=0 ;bb50 bbm(bb2->bb14.bb46.bb1021!=0 ){bb2->bb14.bb46.
bb625-=15 ;bbm(bb2->bb14.bb46.bb2183==0 ){bb2->bb14.bb46.bb1966= *bb2->
bb14.bb46.bb1789++;--bb2->bb14.bb46.bb1021;++bb2->bb14.bb46.bb1275;++
bb2->bb14.bb46.bb1776;bb2->bb14.bb46.bb2115=(bbk)bb2->bb14.bb46.
bb1776&(2048 -1 );bb2->bb468->bb1893[(bbk)bb2->bb14.bb46.bb1275&(2048 -1
)]=bb2->bb14.bb46.bb1966;bb2->bb14.bb46.bb1705=(bb2->bb14.bb46.bb1705
<<8 )+bb2->bb14.bb46.bb1966;bb2->bb14.bb46.bb2183=1 ;}bb110((bb2->bb14.
bb46.bb1021!=0 )&&(bb2->bb14.bb46.bb2061==0 )){++bb2->bb14.bb46.bb1275;
++bb2->bb14.bb46.bb1776;bb2->bb14.bb46.bb2115=(bbk)bb2->bb14.bb46.
bb1776&(2048 -1 );bbm(((bb2->bb14.bb46.bb1275&0x7FFFFFFFL )==0 ))bb2450(
bb2);bb2->bb468->bb1893[(bbk)bb2->bb14.bb46.bb1275&(2048 -1 )]=bb2->
bb14.bb46.bb1966= *bb2->bb14.bb46.bb1789++;bb2->bb14.bb46.bb1705=(bb2
->bb14.bb46.bb1705<<8 )+bb2->bb14.bb46.bb1966;--bb2->bb14.bb46.bb1021;
bb2->bb14.bb46.bb2340=bb2->bb468->bb2151+((((bb2->bb14.bb46.bb1705)&
0xFF00 )>>4 )^((bb2->bb14.bb46.bb1705)&0x00FF ));bbm((bb2->bb14.bb46.
bb2363=bb2->bb14.bb46.bb1776- *bb2->bb14.bb46.bb2340)>2048 -2 ){bb2->
bb468->bb1927[bb2->bb14.bb46.bb2115]=0 ;bbm(bb2->bb14.bb46.bb563!=0 ){
bb2144(bb2,bb2->bb14.bb46.bb1409,bb2->bb14.bb46.bb563);}bb50{bb2->
bb14.bb46.bb1054=((bb2->bb14.bb46.bb1705>>8 ));bb2->bb14.bb46.bb1090=(
9 );bb1135(bb2);bb2->bb1781=bb2301;}}bb50{bb2->bb468->bb1927[bb2->bb14
.bb46.bb2115]=(bbk)bb2->bb14.bb46.bb2363;bbm(bb2->bb14.bb46.bb563!=0 ){
bbm((bb2->bb468->bb1893[(bbk)(((bbd)bb2->bb14.bb46.bb1297+bb2->bb14.
bb46.bb563+bb2->bb14.bb46.bb1890)&(bbd)(2048 -1 ))]==bb2->bb14.bb46.
bb1966)&&((bb2->bb14.bb46.bb563+bb2->bb14.bb46.bb1890)<(bbd)0xFFFFFFFFL
)){++bb2->bb14.bb46.bb563;bb2->bb1781=bb2567;bbm(bb2->bb14.bb46.
bb1880){bbm(bb2->bb14.bb46.bb563>=23 ){bb2->bb14.bb46.bb1054=(((1 <<4 )-
1 ));bb2->bb14.bb46.bb1090=(4 );bb1135(bb2);bb2->bb14.bb46.bb563-=((1 <<
4 )-1 );bb2->bb14.bb46.bb1890+=((1 <<4 )-1 );}}bb50 bbm(bb2->bb14.bb46.
bb563>=23 ){bbm(bb2->bb14.bb46.bb1409<128 ){bb2->bb14.bb46.bb1054=(
0x180 |bb2->bb14.bb46.bb1409);bb2->bb14.bb46.bb1090=(9 );bb1135(bb2);}
bb50{bb2->bb14.bb46.bb1054=(0x1000 |bb2->bb14.bb46.bb1409);bb2->bb14.
bb46.bb1090=(13 );bb1135(bb2);}bb2->bb14.bb46.bb1054=(((1 <<4 )-1 ));bb2
->bb14.bb46.bb1090=(4 );bb1135(bb2);bb2->bb14.bb46.bb563-=((1 <<4 )-1 );
bb2->bb14.bb46.bb1890+=((1 <<4 )-1 );bb2->bb14.bb46.bb1880=1 ;}}bb50 bbm(
bb2->bb14.bb46.bb1880){bb2->bb14.bb46.bb1054=(bb2228[(bbk)bb2->bb14.
bb46.bb563].bb2385);bb2->bb14.bb46.bb1090=(bb2228[(bbk)bb2->bb14.bb46
.bb563].bb48);bb1135(bb2);bb2->bb14.bb46.bb563=0 ;bb2->bb14.bb46.
bb1890=0 ;bb2->bb14.bb46.bb1880=0 ;bb2->bb1781=bb2466;}bb50 bbm(bb2->
bb14.bb46.bb563>=8 ){bb2144(bb2,bb2->bb14.bb46.bb1409,bb2->bb14.bb46.
bb563);}bb50{bb2107=0 ;bb2->bb14.bb46.bb2166=bb2->bb14.bb46.bb1409;
bb110((bb2->bb468->bb1927[bb2->bb14.bb46.bb1297]!=0 )&&(bb2107==0 )&&(
bb2->bb14.bb46.bb2125<bb2233)&&(bb2->bb14.bb46.bb2166<(bbk)(2048 -bb2
->bb14.bb46.bb563))){bb2->bb14.bb46.bb2166+=bb2->bb468->bb1927[bb2->
bb14.bb46.bb1297];++bb2->bb14.bb46.bb2125;bbm(bb2->bb14.bb46.bb2166<(
bbk)(2048 -bb2->bb14.bb46.bb563)){bb2->bb14.bb46.bb1297=bb2->bb14.bb46
.bb1297-bb2->bb468->bb1927[bb2->bb14.bb46.bb1297]&(2048 -1 );bbm(bb2->
bb468->bb1893[bb2->bb14.bb46.bb1297]==bb2->bb468->bb1893[bb2->bb14.
bb46.bb2320]){bb2107=1 ;bb91(bb2256=2 ,bb2242=(bb2->bb14.bb46.bb1297+2 )&
(2048 -1 );bb2256<=(bbk)bb2->bb14.bb46.bb563;bb2256++,bb2242=(bb2242+1 )&
(2048 -1 )){bbm(bb2->bb468->bb1893[bb2242]!=bb2->bb468->bb1893[(bb2->
bb14.bb46.bb2320+bb2256)&(2048 -1 )]){bb2107=0 ;bb21;}}}}}bbm(bb2107){
bb2->bb14.bb46.bb1409=bb2->bb14.bb46.bb2166;++bb2->bb14.bb46.bb563;
bb2->bb1781=bb2551;}bb50{bb2144(bb2,bb2->bb14.bb46.bb1409,bb2->bb14.
bb46.bb563);}}}bb50{bb2->bb14.bb46.bb2117=(bbk)bb2->bb14.bb46.bb2363;
bb2->bb14.bb46.bb2125=0 ;bb595{bb2->bb14.bb46.bb1297=(bbk)(bb2->bb14.
bb46.bb1776-bb2->bb14.bb46.bb2117&(2048 -1 ));bbm(bb2->bb468->bb1893[
bb2->bb14.bb46.bb1297]==(bbf)(bb2->bb14.bb46.bb1705>>8 )){bb2->bb14.
bb46.bb563=2 ;bb2->bb14.bb46.bb2320=bb2->bb14.bb46.bb2115;bb2->bb14.
bb46.bb1409=bb2->bb14.bb46.bb2117;bb2->bb1781=bb2642;bb21;}bb50{bb2->
bb14.bb46.bb2117+=bb2->bb468->bb1927[bb2->bb14.bb46.bb1297];++bb2->
bb14.bb46.bb2125;}}bb110((bb2->bb468->bb1927[bb2->bb14.bb46.bb1297]!=
0 )&&(bb2->bb14.bb46.bb2125<bb2233)&&(bb2->bb14.bb46.bb2117<2048 -2 ));
bbm(bb2->bb14.bb46.bb563==0 ){bb2->bb14.bb46.bb1054=((bb2->bb14.bb46.
bb1705>>8 ));bb2->bb14.bb46.bb1090=(9 );bb1135(bb2);bb2->bb1781=bb2301;
}}}bbm(bb2557[bb2->bb1781][bb2->bb2180]){ *bb2->bb14.bb46.bb2340=bb2
->bb14.bb46.bb1776;}}bbm(bb2->bb14.bb46.bb1021==0 ){bb1077=1 ;bbm(bb383
&0x01 ){bb2267(bb2,bb383);bb1077|=4 ;}}bbm((bb2->bb14.bb46.bb625==0 )||(
bb2->bb14.bb46.bb2061)){bbm(!(bb1077&1 )){bb1077=2 ;bbm(bb383&0x02 ){
bb2267(bb2,bb383);bb1077|=4 ;}}bb50{bb1077|=3 ;bbm((!(bb383&0x01 ))&&(
bb383&0x02 )){bb2267(bb2,bb383);bb1077|=4 ;}}}bb2->bb14.bb46.bb625+=15 ;
}bb50{bb1077=1 ;bbm(bb383&0x01 ){bb2267(bb2,bb383);bb1077|=4 ;}}bb2->
bb468->bb46=bb2->bb14.bb46; *bb1770=bb2->bb14.bb46.bb1789; *bb942=bb2
->bb14.bb46.bb1021; *bb1835=bb2->bb14.bb46.bb1774; *bb636=bb2->bb14.
bb46.bb625;bb4(bb1077);}bb40 bbb bb2198(bb496 bb2){bb2->bb14.bb86.
bb2158&=(2048 -1 );bb2->bb14.bb86.bb1080=bb2->bb14.bb86.bb1189=bb2->
bb14.bb86.bb96=0 ;bb2->bb14.bb86.bb1056=bb2->bb14.bb86.bb1795=bb2->
bb14.bb86.bb1186=0 ;bb2->bb14.bb86.bb479=0 ;bb2->bb14.bb86.bb2030=0 ;bb2
->bb14.bb86.bb1206=bb2150;bb2->bb14.bb86.bb2652=1 ;bb4;}bb40 bbk bb2354
(bb496 bb2){bbm((bb2->bb14.bb86.bb1021==0 )&&(bb2->bb14.bb86.bb1080==0
))bb2->bb14.bb86.bb1056=bb2453;bb50{bb2->bb14.bb86.bb1056=bb1292;bbm(
bb2->bb14.bb86.bb1080==0 ){bb2->bb14.bb86.bb1795= *bb2->bb14.bb86.
bb1789++;--bb2->bb14.bb86.bb1021;bb2->bb14.bb86.bb1080=7 ;bb2->bb14.
bb86.bb1186=(bb2->bb14.bb86.bb1795>127 )?1 :0 ;bb2->bb14.bb86.bb1795&=((
bbf)0x7F );}bb50{bb2->bb14.bb86.bb1186=(bb2->bb14.bb86.bb1795>>(bb2->
bb14.bb86.bb1080-1 ));--bb2->bb14.bb86.bb1080;bb2->bb14.bb86.bb1795&=(
(bbf)0xFF >>(8 -bb2->bb14.bb86.bb1080));}}bb4(bb2->bb14.bb86.bb1056);}
bb40 bbk bb2017(bb496 bb2){bbk bb2227;bb125 bb10;bbm(bb2->bb14.bb86.
bb1056==bb1292)bb2->bb14.bb86.bb1186=0 ;bb50 bb2->bb14.bb86.bb1056=
bb1292;bb110((bb2->bb14.bb86.bb1189>0 )&&(bb2->bb14.bb86.bb1056==
bb1292)){bbm((bb2->bb14.bb86.bb1021==0 )&&(bb2->bb14.bb86.bb1080==0 ))bb2
->bb14.bb86.bb1056=bb2453;bb50{bbm(bb2->bb14.bb86.bb1080==0 ){bb2->
bb14.bb86.bb1795= *bb2->bb14.bb86.bb1789++;--bb2->bb14.bb86.bb1021;
bb2->bb14.bb86.bb1080=8 ;}bb2227=bb2->bb14.bb86.bb1795;bbm((bb10=bb2->
bb14.bb86.bb1189-bb2->bb14.bb86.bb1080)>0 )bb2227<<=bb10;bb50 bb2227
>>=-bb10;bb2->bb14.bb86.bb1186|=bb2227;bb10=((((8 )<(bb2->bb14.bb86.
bb1189)?(8 ):(bb2->bb14.bb86.bb1189)))<(bb2->bb14.bb86.bb1080)?(((8 )<(
bb2->bb14.bb86.bb1189)?(8 ):(bb2->bb14.bb86.bb1189))):(bb2->bb14.bb86.
bb1080));bb2->bb14.bb86.bb1189-=bb10;bb2->bb14.bb86.bb1080-=bb10;bb2
->bb14.bb86.bb1795&=((bbf)0xFF >>(8 -bb2->bb14.bb86.bb1080));}}bb4(bb2
->bb14.bb86.bb1056);}bb40 bbb bb2138(bb496 bb2,bbf bbo){bbm(bb2->bb14
.bb86.bb625!=0 ){ *bb2->bb14.bb86.bb1774++=bbo;--bb2->bb14.bb86.bb625;
bb2->bb468->bb2236[bb2->bb14.bb86.bb2158++]=(bbf)bbo;bb2->bb14.bb86.
bb2158&=(2048 -1 );}}bbk bb2366(bb496 bb2,bbf* *bb1770,bbf* *bb1835,bbd
 *bb942,bbd*bb636,bbb*bb1354,bbk bb383){bbk bb2272=0 ;bbk bb1077=0 ;bb2
->bb468=(bb2112* )bb1354;bb2->bb14.bb86=bb2->bb468->bb86;bb2->bb14.
bb86.bb1789= *bb1770;bb2->bb14.bb86.bb1021= *bb942;bb2->bb14.bb86.
bb1774= *bb1835;bb2->bb14.bb86.bb625= *bb636;bbm(bb383&0x01 ){bb2198(
bb2);}bbm((bb2->bb14.bb86.bb1021!=0 )&&(bb2->bb14.bb86.bb625!=0 )){
bb110((bb2->bb14.bb86.bb625!=0 )&&((bb2->bb14.bb86.bb1021!=0 )||(bb2->
bb14.bb86.bb1080!=0 ))&&(bb2272==0 )){bbm(bb2->bb14.bb86.bb2030){bb110(
(bb2->bb14.bb86.bb625!=0 )&&(bb2->bb14.bb86.bb479>0 )){bb2->bb14.bb86.
bb96&=(2048 -1 );bb2138(bb2,bb2->bb468->bb2236[bb2->bb14.bb86.bb96++]);
--bb2->bb14.bb86.bb479;}bbm(bb2->bb14.bb86.bb479==0 )bb2->bb14.bb86.
bb2030=0 ;bb2->bb14.bb86.bb1206=bb2150;}bb50{bb350(bb2->bb14.bb86.
bb1206){bb17 bb2150:bb2354(bb2);bbm(bb2->bb14.bb86.bb1186==0 ){bb2->
bb14.bb86.bb1189=8 ;bb2->bb14.bb86.bb1206=bb2422;bb17 bb2422:bb2017(
bb2);bbm(bb2->bb14.bb86.bb1056==bb1292){bb2138(bb2,(bbf)bb2->bb14.
bb86.bb1186);bb2->bb14.bb86.bb1206=bb2150;}}bb50{bb2->bb14.bb86.
bb1206=bb2459;bb17 bb2459:bb2354(bb2);bbm(bb2->bb14.bb86.bb1056==
bb1292){bb2->bb14.bb86.bb1189=bb2->bb14.bb86.bb1186?7 :11 ;bb2->bb14.
bb86.bb1206=bb2425;bb17 bb2425:bb2017(bb2);bbm(bb2->bb14.bb86.bb1056
==bb1292){bb2->bb14.bb86.bb96=bb2->bb14.bb86.bb1186;bbm(bb2->bb14.
bb86.bb96==0 )bb2272=1 ;bb50{bb2->bb14.bb86.bb96=bb2->bb14.bb86.bb2158-
bb2->bb14.bb86.bb96;bb2->bb14.bb86.bb1189=2 ;bb2->bb14.bb86.bb1206=
bb2519;bb17 bb2519:bb2017(bb2);bbm(bb2->bb14.bb86.bb1056==bb1292){bb2
->bb14.bb86.bb479=2 +bb2->bb14.bb86.bb1186;bbm(bb2->bb14.bb86.bb479==5
){bb2->bb14.bb86.bb1189=2 ;bb2->bb14.bb86.bb1206=bb2518;bb17 bb2518:
bb2017(bb2);bbm(bb2->bb14.bb86.bb1056==bb1292){bb2->bb14.bb86.bb479+=
bb2->bb14.bb86.bb1186;bbm(bb2->bb14.bb86.bb479==8 ){bb2->bb14.bb86.
bb1189=4 ;bb2->bb14.bb86.bb1206=bb2501;bb17 bb2501:bb2017(bb2);bbm(bb2
->bb14.bb86.bb1056==bb1292){bb2->bb14.bb86.bb479+=bb2->bb14.bb86.
bb1186;bbm(bb2->bb14.bb86.bb479==23 ){bb595{bb17 bb2310:bb110((bb2->
bb14.bb86.bb625!=0 )&&(bb2->bb14.bb86.bb479>0 )){bb2->bb14.bb86.bb96&=(
2048 -1 );bb2138(bb2,bb2->bb468->bb2236[bb2->bb14.bb86.bb96++]);--bb2->
bb14.bb86.bb479;}bbm(bb2->bb14.bb86.bb625==0 ){bb2->bb14.bb86.bb1206=
bb2310;bb21;}bb50{bb2->bb14.bb86.bb1189=4 ;bb2->bb14.bb86.bb1206=
bb2440;bb17 bb2440:bb2017(bb2);bbm(bb2->bb14.bb86.bb1056==bb1292)bb2
->bb14.bb86.bb479+=bb2->bb14.bb86.bb1186;bb50 bb21;}}bb110(bb2->bb14.
bb86.bb1186==((1 <<4 )-1 ));}}}}}}bbm((bb2->bb14.bb86.bb1056==bb1292)&&(
bb2->bb14.bb86.bb1206!=bb2310)){bb2->bb14.bb86.bb2030=1 ;}}}}}}}}bbm(
bb2->bb14.bb86.bb2030){bb110((bb2->bb14.bb86.bb625!=0 )&&(bb2->bb14.
bb86.bb479>0 )){bb2->bb14.bb86.bb96&=(2048 -1 );bb2138(bb2,bb2->bb468->
bb2236[bb2->bb14.bb86.bb96++]);--bb2->bb14.bb86.bb479;}bbm(bb2->bb14.
bb86.bb479==0 )bb2->bb14.bb86.bb2030=0 ;bb2->bb14.bb86.bb1206=bb2150;}}
bbm(bb2272){bb2198(bb2);bb1077|=4 ;}bbm(bb2->bb14.bb86.bb1021==0 ){
bb1077|=1 ;}bbm(bb2->bb14.bb86.bb625==0 ){bb1077|=2 ;}bb2->bb468->bb86=
bb2->bb14.bb86; *bb1770=bb2->bb14.bb86.bb1789; *bb942=bb2->bb14.bb86.
bb1021; *bb1835=bb2->bb14.bb86.bb1774; *bb636=bb2->bb14.bb86.bb625;
bb4(bb1077);}
