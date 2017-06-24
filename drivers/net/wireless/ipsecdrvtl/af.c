/*
   'src_compress_deflate_inftrees.c' Obfuscated by COBF (Version 1.06 2006-01-07 by BB) at Fri Nov 13 10:03:51 2015
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
#if ( defined( _WIN32) || defined( __WIN32__)) && ! defined( WIN32)
#define WIN32
#endif
#if defined( __GNUC__) || defined( WIN32) || defined( bb1261) ||  \
defined( bb1252)
#ifndef bb411
#define bb411
#endif
#endif
#if defined( __MSDOS__) && ! defined( bb168)
#define bb168
#endif
#if defined( bb168) && ! defined( bb411)
#define bb529
#endif
#ifdef bb168
#define bb1076
#endif
#if ( defined( bb168) || defined( bb1242) || defined( WIN32)) && !  \
defined( bb139)
#define bb139
#endif
#if defined( __STDC__) || defined( __cplusplus) || defined( bb1246)
#ifndef bb139
#define bb139
#endif
#endif
#ifndef bb139
#ifndef bbh
#define bbh
#endif
#endif
#if defined( __BORLANDC__) && ( __BORLANDC__ < 0x500)
#define bb1147
#endif
#ifndef bb292
#ifdef bb529
#define bb292 8
#else
#define bb292 9
#endif
#endif
#ifndef bbq
#ifdef bb139
#define bbq( bb419) bb419
#else
#define bbq( bb419) ()
#endif
#endif
bba bbf bb153;bba bbt bbe bb9;bba bbt bb8 bb25;bba bb153 bb33;bba bbl
bb452;bba bbe bb1132;bba bb9 bb165;bba bb25 bb167;
#ifdef bb139
bba bbb*bb72;bba bbb*bb189;
#else
bba bb153*bb72;bba bb153*bb189;
#endif
#ifdef __cplusplus
bbr"\x43"{
#endif
bba bb72( *bb525)bbq((bb72 bb122,bb9 bb512,bb9 bb48));bba bbb( *bb522
)bbq((bb72 bb122,bb72 bb1134));bbj bb392;bba bbj bb1222{bb33*bb127;
bb9 bb149;bb25 bb195;bb33*bb620;bb9 bb396;bb25 bb637;bbl*bb327;bbj
bb392*bb23;bb525 bb415;bb522 bb381;bb72 bb122;bbe bb1001;bb25 bb378;
bb25 bb1188;}bb450;bba bb450*bb16;bbr bbh bbl*bb1197 bbq((bbb));bbr
bbe bb524 bbq((bb16 bb15,bbe bb176));bbr bbe bb970 bbq((bb16 bb15));
bbr bbe bb1084 bbq((bb16 bb15,bbe bb176));bbr bbe bb957 bbq((bb16 bb15
));bbr bbe bb1217 bbq((bb16 bb15,bbh bb33*bb441,bb9 bb451));bbr bbe
bb1187 bbq((bb16 bb132,bb16 bb186));bbr bbe bb1086 bbq((bb16 bb15));
bbr bbe bb1202 bbq((bb16 bb15,bbe bb126,bbe bb301));bbr bbe bb1218 bbq
((bb16 bb15,bbh bb33*bb441,bb9 bb451));bbr bbe bb1198 bbq((bb16 bb15));
bbr bbe bb1044 bbq((bb16 bb15));bbr bbe bb1212 bbq((bb33*bb132,bb167*
bb318,bbh bb33*bb186,bb25 bb329));bbr bbe bb1181 bbq((bb33*bb132,
bb167*bb318,bbh bb33*bb186,bb25 bb329,bbe bb126));bbr bbe bb1203 bbq(
(bb33*bb132,bb167*bb318,bbh bb33*bb186,bb25 bb329));bba bb189 bb37;
bbr bb37 bb1241 bbq((bbh bbl*bb1038,bbh bbl*bb45));bbr bb37 bb1243 bbq
((bbe bb484,bbh bbl*bb45));bbr bbe bb1265 bbq((bb37 bb26,bbe bb126,
bbe bb301));bbr bbe bb1237 bbq((bb37 bb26,bb189 bb42,bbt bb22));bbr
bbe bb1223 bbq((bb37 bb26,bbh bb189 bb42,bbt bb22));bbr bbe bb1264 bbq
((bb37 bb26,bbh bbl*bb1267,...));bbr bbe bb1224 bbq((bb37 bb26,bbh bbl
 *bbg));bbr bbl*bb1270 bbq((bb37 bb26,bbl*bb42,bbe bb22));bbr bbe
bb1225 bbq((bb37 bb26,bbe bbo));bbr bbe bb1271 bbq((bb37 bb26));bbr
bbe bb1219 bbq((bb37 bb26,bbe bb176));bbr bb8 bb1229 bbq((bb37 bb26,
bb8 bb96,bbe bb1235));bbr bbe bb1269 bbq((bb37 bb26));bbr bb8 bb1248
bbq((bb37 bb26));bbr bbe bb1232 bbq((bb37 bb26));bbr bbe bb1236 bbq((
bb37 bb26));bbr bbh bbl*bb1221 bbq((bb37 bb26,bbe*bb1268));bbr bb25
bb1018 bbq((bb25 bb378,bbh bb33*bb42,bb9 bb22));bbr bb25 bb1207 bbq((
bb25 bb390,bbh bb33*bb42,bb9 bb22));bbr bbe bb1148 bbq((bb16 bb15,bbe
bb126,bbh bbl*bb194,bbe bb197));bbr bbe bb1161 bbq((bb16 bb15,bbh bbl
 *bb194,bbe bb197));bbr bbe bb1079 bbq((bb16 bb15,bbe bb126,bbe bb588
,bbe bb466,bbe bb975,bbe bb301,bbh bbl*bb194,bbe bb197));bbr bbe
bb1125 bbq((bb16 bb15,bbe bb466,bbh bbl*bb194,bbe bb197));bbr bbh bbl
 *bb1210 bbq((bbe bb18));bbr bbe bb1190 bbq((bb16 bb0));bbr bbh bb167
 *bb1205 bbq((bbb));
#ifdef __cplusplus
}
#endif
#define bb1019 1
#ifdef bb139
#if defined( bb1773)
#else
#endif
#endif
bba bbt bbl bb156;bba bb156 bb1240;bba bbt bb137 bb129;bba bb129 bb521
;bba bbt bb8 bb412;bbr bbh bbl*bb1097[10 ];
#if bb292 >= 8
#define bb798 8
#else
#define bb798 bb292
#endif
#ifdef bb168
#define bb435 0x00
#if defined( __TURBOC__) || defined( __BORLANDC__)
#if( __STDC__ == 1) && ( defined( bb1830) || defined( bb1810))
bbb bb974 bb1378(bbb*bb105);bbb*bb974 bb1384(bbt bb8 bb1772);
#else
#include"uncobf.h"
#include<alloc.h>
#include"cobf.h"
#endif
#else
#include"uncobf.h"
#include<malloc.h>
#include"cobf.h"
#endif
#endif
#ifdef WIN32
#define bb435 0x0b
#endif
#if ( defined( _MSC_VER) && ( _MSC_VER > 600))
#define bb1786( bb484, bb131) bb1823( bb484, bb131)
#endif
#ifndef bb435
#define bb435 0x03
#endif
#if defined( bb1572) && ! defined( _MSC_VER) && ! defined( bb1812)
#define bb1019
#endif
bba bb25( *bb987)bbq((bb25 bb499,bbh bb33*bb42,bb9 bb22));bb72 bb1211
bbq((bb72 bb122,bbt bb512,bbt bb48));bbb bb1209 bbq((bb72 bb122,bb72
bb939));bba bbj bb1740 bb154;bbj bb1740{bb332{bbj{bb153 bb1214;bb153
bb990;}bb531;bb9 bb1301;}bb523;bb9 bb634;};bbr bbe bb2080 bbq((bb165*
,bb165* ,bb154* * ,bb154* ,bb16));bbr bbe bb2062 bbq((bb9,bb9,bb165* ,
bb165* ,bb165* ,bb154* * ,bb154* * ,bb154* ,bb16));bbr bbe bb2025 bbq
((bb165* ,bb165* ,bb154* * ,bb154* * ,bb16));
#if ! defined( bb2319) && ! defined( bb139)
#define bb2319
#endif
bbj bb392{bbe bb463;};bb40 bbe bb2033 bbq((bb165* ,bb9,bb9,bbh bb165*
,bbh bb165* ,bb154* * ,bb165* ,bb154* ,bb9* ,bb165* ));bb40 bbh bb9
bb2447[31 ]={3 ,4 ,5 ,6 ,7 ,8 ,9 ,10 ,11 ,13 ,15 ,17 ,19 ,23 ,27 ,31 ,35 ,43 ,51 ,59 ,67 ,
83 ,99 ,115 ,131 ,163 ,195 ,227 ,258 ,0 ,0 };bb40 bbh bb9 bb2435[31 ]={0 ,0 ,0 ,0 ,0
,0 ,0 ,0 ,1 ,1 ,1 ,1 ,2 ,2 ,2 ,2 ,3 ,3 ,3 ,3 ,4 ,4 ,4 ,4 ,5 ,5 ,5 ,5 ,0 ,112 ,112 };bb40 bbh bb9
bb2484[30 ]={1 ,2 ,3 ,4 ,5 ,7 ,9 ,13 ,17 ,25 ,33 ,49 ,65 ,97 ,129 ,193 ,257 ,385 ,513 ,
769 ,1025 ,1537 ,2049 ,3073 ,4097 ,6145 ,8193 ,12289 ,16385 ,24577 };bb40 bbh bb9
bb2485[30 ]={0 ,0 ,0 ,0 ,1 ,1 ,2 ,2 ,3 ,3 ,4 ,4 ,5 ,5 ,6 ,6 ,7 ,7 ,8 ,8 ,9 ,9 ,10 ,10 ,11 ,11 ,
12 ,12 ,13 ,13 };bb40 bbe bb2033(bbp,bb11,bbg,bbs,bbw,bb47,bb82,bb1822,
bb1824,bb448)bb165*bbp;bb9 bb11;bb9 bbg;bbh bb165*bbs;bbh bb165*bbw;
bb154* *bb47;bb165*bb82;bb154*bb1822;bb9*bb1824;bb165*bb448;{bb9 bbc;
bb9 bbo[15 +1 ];bb9 bb20;bbe bb55;bbe bb44;bb950 bb9 bbz;bb950 bb9 bb77
;bb950 bbe bb6;bbe bb179;bb9 bb1215;bb950 bb165*bb28;bb154*bb87;bbj
bb1740 bb24={{{0 }},0 };bb154*bb298[15 ];bb950 bbe bbv;bb9 bb10[15 +1 ];
bb165*bb2154;bbe bb177;bb9 bb0;bb28=bbo; *bb28++=0 ; *bb28++=0 ; *bb28
++=0 ; *bb28++=0 ; *bb28++=0 ; *bb28++=0 ; *bb28++=0 ; *bb28++=0 ; *bb28++=
0 ; *bb28++=0 ; *bb28++=0 ; *bb28++=0 ; *bb28++=0 ; *bb28++=0 ; *bb28++=0 ; *
bb28++=0 ;bb28=bbp;bbz=bb11;bb595{bbo[ *bb28++]++;}bb110(--bbz);bbm(
bbo[0 ]==bb11){ *bb47=(bb154* )0 ; *bb82=0 ;bb4 0 ;}bb179= *bb82;bb91(
bb77=1 ;bb77<=15 ;bb77++)bbm(bbo[bb77])bb21;bb6=bb77;bbm((bb9)bb179<
bb77)bb179=bb77;bb91(bbz=15 ;bbz;bbz--)bbm(bbo[bbz])bb21;bb55=bbz;bbm(
(bb9)bb179>bbz)bb179=bbz; *bb82=bb179;bb91(bb177=1 <<bb77;bb77<bbz;
bb77++,bb177<<=1 )bbm((bb177-=bbo[bb77])<0 )bb4(-3 );bbm((bb177-=bbo[bbz
])<0 )bb4(-3 );bbo[bbz]+=bb177;bb10[1 ]=bb77=0 ;bb28=bbo+1 ;bb2154=bb10+2 ;
bb110(--bbz){ *bb2154++=(bb77+= *bb28++);}bb28=bbp;bbz=0 ;bb595{bbm((
bb77= *bb28++)!=0 )bb448[bb10[bb77]++]=bbz;}bb110(++bbz<bb11);bb11=
bb10[bb55];bb10[0 ]=bbz=0 ;bb28=bb448;bb44=-1 ;bbv=-bb179;bb298[0 ]=(
bb154* )0 ;bb87=(bb154* )0 ;bb0=0 ;bb91(;bb6<=bb55;bb6++){bbc=bbo[bb6];
bb110(bbc--){bb110(bb6>bbv+bb179){bb44++;bbv+=bb179;bb0=bb55-bbv;bb0=
bb0>(bb9)bb179?(bb9)bb179:bb0;bbm((bb20=1 <<(bb77=bb6-bbv))>bbc+1 ){
bb20-=bbc+1 ;bb2154=bbo+bb6;bbm(bb77<bb0)bb110(++bb77<bb0){bbm((bb20
<<=1 )<= * ++bb2154)bb21;bb20-= *bb2154;}}bb0=1 <<bb77;bbm( *bb1824+bb0
>1440 )bb4(-4 );bb298[bb44]=bb87=bb1822+ *bb1824; *bb1824+=bb0;bbm(bb44
){bb10[bb44]=bbz;bb24.bb523.bb531.bb990=(bb153)bb179;bb24.bb523.bb531
.bb1214=(bb153)bb77;bb77=bbz>>(bbv-bb179);bb24.bb634=(bb9)(bb87-bb298
[bb44-1 ]-bb77);bb298[bb44-1 ][bb77]=bb24;}bb50*bb47=bb87;}bb24.bb523.
bb531.bb990=(bb153)(bb6-bbv);bbm(bb28>=bb448+bb11)bb24.bb523.bb531.
bb1214=128 +64 ;bb50 bbm( *bb28<bbg){bb24.bb523.bb531.bb1214=(bb153)( *
bb28<256 ?0 :32 +64 );bb24.bb634= *bb28++;}bb50{bb24.bb523.bb531.bb1214=(
bb153)(bbw[ *bb28-bbg]+16 +64 );bb24.bb634=bbs[ *bb28++-bbg];}bb20=1 <<(
bb6-bbv);bb91(bb77=bbz>>bbv;bb77<bb0;bb77+=bb20)bb87[bb77]=bb24;bb91(
bb77=1 <<(bb6-1 );bbz&bb77;bb77>>=1 )bbz^=bb77;bbz^=bb77;bb1215=(1 <<bbv)-
1 ;bb110((bbz&bb1215)!=bb10[bb44]){bb44--;bbv-=bb179;bb1215=(1 <<bbv)-1
;}}}bb4 bb177!=0 &&bb55!=1 ?(-5 ):0 ;}bbe bb2080(bbo,bb1736,bb1808,bb1822
,bb0)bb165*bbo;bb165*bb1736;bb154* *bb1808;bb154*bb1822;bb16 bb0;{bbe
bb24;bb9 bb1824=0 ;bb165*bb448;bbm((bb448=(bb165* )( * ((bb0)->bb415))(
(bb0)->bb122,(19 ),(bb12(bb9))))==0 )bb4(-4 );bb24=bb2033(bbo,19 ,19 ,(
bb165* )0 ,(bb165* )0 ,bb1808,bb1736,bb1822,&bb1824,bb448);bbm(bb24==(-
3 ))bb0->bb327=(bbl* )"";bb50 bbm(bb24==(-5 )|| *bb1736==0 ){bb0->bb327=
(bbl* )"";bb24=(-3 );}( * ((bb0)->bb381))((bb0)->bb122,(bb72)(bb448));
bb4 bb24;}bbe bb2062(bb2252,bb2468,bbo,bb58,bb967,bb1052,bb1030,
bb1822,bb0)bb9 bb2252;bb9 bb2468;bb165*bbo;bb165*bb58;bb165*bb967;
bb154* *bb1052;bb154* *bb1030;bb154*bb1822;bb16 bb0;{bbe bb24;bb9
bb1824=0 ;bb165*bb448;bbm((bb448=(bb165* )( * ((bb0)->bb415))((bb0)->
bb122,(288 ),(bb12(bb9))))==0 )bb4(-4 );bb24=bb2033(bbo,bb2252,257 ,
bb2447,bb2435,bb1052,bb58,bb1822,&bb1824,bb448);bbm(bb24!=0 || *bb58==
0 ){bbm(bb24==(-3 ))bb0->bb327=(bbl* )"";bb50 bbm(bb24!=(-4 )){bb0->
bb327=(bbl* )"";bb24=(-3 );}( * ((bb0)->bb381))((bb0)->bb122,(bb72)(
bb448));bb4 bb24;}bb24=bb2033(bbo+bb2252,bb2468,0 ,bb2484,bb2485,
bb1030,bb967,bb1822,&bb1824,bb448);bbm(bb24!=0 ||( *bb967==0 &&bb2252>
257 )){bbm(bb24==(-3 ))bb0->bb327=(bbl* )"";bb50 bbm(bb24==(-5 )){bb0->
bb327=(bbl* )"";bb24=(-3 );}bb50 bbm(bb24!=(-4 )){bb0->bb327=(bbl* )"";
bb24=(-3 );}( * ((bb0)->bb381))((bb0)->bb122,(bb72)(bb448));bb4 bb24;}
( * ((bb0)->bb381))((bb0)->bb122,(bb72)(bb448));bb4 0 ;}
#ifdef bb2319
bb40 bbe bb2410=0 ;
#define bb2619 544
bb40 bb154 bb2510[bb2619];bb40 bb9 bb2201;bb40 bb9 bb2203;bb40 bb154*
bb2334;bb40 bb154*bb2339;
#else
bb40 bb9 bb2201=9 ;bb40 bb9 bb2203=5 ;bb40 bb154 bb2334[]={{{{96 ,7 }},
256 },{{{0 ,8 }},80 },{{{0 ,8 }},16 },{{{84 ,8 }},115 },{{{82 ,7 }},31 },{{{0 ,8 }},
112 },{{{0 ,8 }},48 },{{{0 ,9 }},192 },{{{80 ,7 }},10 },{{{0 ,8 }},96 },{{{0 ,8 }},
32 },{{{0 ,9 }},160 },{{{0 ,8 }},0 },{{{0 ,8 }},128 },{{{0 ,8 }},64 },{{{0 ,9 }},224
},{{{80 ,7 }},6 },{{{0 ,8 }},88 },{{{0 ,8 }},24 },{{{0 ,9 }},144 },{{{83 ,7 }},59 },
{{{0 ,8 }},120 },{{{0 ,8 }},56 },{{{0 ,9 }},208 },{{{81 ,7 }},17 },{{{0 ,8 }},104 },
{{{0 ,8 }},40 },{{{0 ,9 }},176 },{{{0 ,8 }},8 },{{{0 ,8 }},136 },{{{0 ,8 }},72 },{{{
0 ,9 }},240 },{{{80 ,7 }},4 },{{{0 ,8 }},84 },{{{0 ,8 }},20 },{{{85 ,8 }},227 },{{{
83 ,7 }},43 },{{{0 ,8 }},116 },{{{0 ,8 }},52 },{{{0 ,9 }},200 },{{{81 ,7 }},13 },{{{
0 ,8 }},100 },{{{0 ,8 }},36 },{{{0 ,9 }},168 },{{{0 ,8 }},4 },{{{0 ,8 }},132 },{{{0 ,
8 }},68 },{{{0 ,9 }},232 },{{{80 ,7 }},8 },{{{0 ,8 }},92 },{{{0 ,8 }},28 },{{{0 ,9 }}
,152 },{{{84 ,7 }},83 },{{{0 ,8 }},124 },{{{0 ,8 }},60 },{{{0 ,9 }},216 },{{{82 ,7 }
},23 },{{{0 ,8 }},108 },{{{0 ,8 }},44 },{{{0 ,9 }},184 },{{{0 ,8 }},12 },{{{0 ,8 }},
140 },{{{0 ,8 }},76 },{{{0 ,9 }},248 },{{{80 ,7 }},3 },{{{0 ,8 }},82 },{{{0 ,8 }},18
},{{{85 ,8 }},163 },{{{83 ,7 }},35 },{{{0 ,8 }},114 },{{{0 ,8 }},50 },{{{0 ,9 }},
196 },{{{81 ,7 }},11 },{{{0 ,8 }},98 },{{{0 ,8 }},34 },{{{0 ,9 }},164 },{{{0 ,8 }},2
},{{{0 ,8 }},130 },{{{0 ,8 }},66 },{{{0 ,9 }},228 },{{{80 ,7 }},7 },{{{0 ,8 }},90 },
{{{0 ,8 }},26 },{{{0 ,9 }},148 },{{{84 ,7 }},67 },{{{0 ,8 }},122 },{{{0 ,8 }},58 },{
{{0 ,9 }},212 },{{{82 ,7 }},19 },{{{0 ,8 }},106 },{{{0 ,8 }},42 },{{{0 ,9 }},180 },{
{{0 ,8 }},10 },{{{0 ,8 }},138 },{{{0 ,8 }},74 },{{{0 ,9 }},244 },{{{80 ,7 }},5 },{{{
0 ,8 }},86 },{{{0 ,8 }},22 },{{{192 ,8 }},0 },{{{83 ,7 }},51 },{{{0 ,8 }},118 },{{{0
,8 }},54 },{{{0 ,9 }},204 },{{{81 ,7 }},15 },{{{0 ,8 }},102 },{{{0 ,8 }},38 },{{{0 ,
9 }},172 },{{{0 ,8 }},6 },{{{0 ,8 }},134 },{{{0 ,8 }},70 },{{{0 ,9 }},236 },{{{80 ,7
}},9 },{{{0 ,8 }},94 },{{{0 ,8 }},30 },{{{0 ,9 }},156 },{{{84 ,7 }},99 },{{{0 ,8 }},
126 },{{{0 ,8 }},62 },{{{0 ,9 }},220 },{{{82 ,7 }},27 },{{{0 ,8 }},110 },{{{0 ,8 }},
46 },{{{0 ,9 }},188 },{{{0 ,8 }},14 },{{{0 ,8 }},142 },{{{0 ,8 }},78 },{{{0 ,9 }},
252 },{{{96 ,7 }},256 },{{{0 ,8 }},81 },{{{0 ,8 }},17 },{{{85 ,8 }},131 },{{{82 ,7 }
},31 },{{{0 ,8 }},113 },{{{0 ,8 }},49 },{{{0 ,9 }},194 },{{{80 ,7 }},10 },{{{0 ,8 }}
,97 },{{{0 ,8 }},33 },{{{0 ,9 }},162 },{{{0 ,8 }},1 },{{{0 ,8 }},129 },{{{0 ,8 }},65
},{{{0 ,9 }},226 },{{{80 ,7 }},6 },{{{0 ,8 }},89 },{{{0 ,8 }},25 },{{{0 ,9 }},146 },
{{{83 ,7 }},59 },{{{0 ,8 }},121 },{{{0 ,8 }},57 },{{{0 ,9 }},210 },{{{81 ,7 }},17 },
{{{0 ,8 }},105 },{{{0 ,8 }},41 },{{{0 ,9 }},178 },{{{0 ,8 }},9 },{{{0 ,8 }},137 },{{
{0 ,8 }},73 },{{{0 ,9 }},242 },{{{80 ,7 }},4 },{{{0 ,8 }},85 },{{{0 ,8 }},21 },{{{80
,8 }},258 },{{{83 ,7 }},43 },{{{0 ,8 }},117 },{{{0 ,8 }},53 },{{{0 ,9 }},202 },{{{
81 ,7 }},13 },{{{0 ,8 }},101 },{{{0 ,8 }},37 },{{{0 ,9 }},170 },{{{0 ,8 }},5 },{{{0 ,
8 }},133 },{{{0 ,8 }},69 },{{{0 ,9 }},234 },{{{80 ,7 }},8 },{{{0 ,8 }},93 },{{{0 ,8 }
},29 },{{{0 ,9 }},154 },{{{84 ,7 }},83 },{{{0 ,8 }},125 },{{{0 ,8 }},61 },{{{0 ,9 }}
,218 },{{{82 ,7 }},23 },{{{0 ,8 }},109 },{{{0 ,8 }},45 },{{{0 ,9 }},186 },{{{0 ,8 }}
,13 },{{{0 ,8 }},141 },{{{0 ,8 }},77 },{{{0 ,9 }},250 },{{{80 ,7 }},3 },{{{0 ,8 }},
83 },{{{0 ,8 }},19 },{{{85 ,8 }},195 },{{{83 ,7 }},35 },{{{0 ,8 }},115 },{{{0 ,8 }},
51 },{{{0 ,9 }},198 },{{{81 ,7 }},11 },{{{0 ,8 }},99 },{{{0 ,8 }},35 },{{{0 ,9 }},
166 },{{{0 ,8 }},3 },{{{0 ,8 }},131 },{{{0 ,8 }},67 },{{{0 ,9 }},230 },{{{80 ,7 }},7
},{{{0 ,8 }},91 },{{{0 ,8 }},27 },{{{0 ,9 }},150 },{{{84 ,7 }},67 },{{{0 ,8 }},123 }
,{{{0 ,8 }},59 },{{{0 ,9 }},214 },{{{82 ,7 }},19 },{{{0 ,8 }},107 },{{{0 ,8 }},43 },
{{{0 ,9 }},182 },{{{0 ,8 }},11 },{{{0 ,8 }},139 },{{{0 ,8 }},75 },{{{0 ,9 }},246 },{
{{80 ,7 }},5 },{{{0 ,8 }},87 },{{{0 ,8 }},23 },{{{192 ,8 }},0 },{{{83 ,7 }},51 },{{{
0 ,8 }},119 },{{{0 ,8 }},55 },{{{0 ,9 }},206 },{{{81 ,7 }},15 },{{{0 ,8 }},103 },{{{
0 ,8 }},39 },{{{0 ,9 }},174 },{{{0 ,8 }},7 },{{{0 ,8 }},135 },{{{0 ,8 }},71 },{{{0 ,9
}},238 },{{{80 ,7 }},9 },{{{0 ,8 }},95 },{{{0 ,8 }},31 },{{{0 ,9 }},158 },{{{84 ,7 }
},99 },{{{0 ,8 }},127 },{{{0 ,8 }},63 },{{{0 ,9 }},222 },{{{82 ,7 }},27 },{{{0 ,8 }}
,111 },{{{0 ,8 }},47 },{{{0 ,9 }},190 },{{{0 ,8 }},15 },{{{0 ,8 }},143 },{{{0 ,8 }},
79 },{{{0 ,9 }},254 },{{{96 ,7 }},256 },{{{0 ,8 }},80 },{{{0 ,8 }},16 },{{{84 ,8 }},
115 },{{{82 ,7 }},31 },{{{0 ,8 }},112 },{{{0 ,8 }},48 },{{{0 ,9 }},193 },{{{80 ,7 }}
,10 },{{{0 ,8 }},96 },{{{0 ,8 }},32 },{{{0 ,9 }},161 },{{{0 ,8 }},0 },{{{0 ,8 }},128
},{{{0 ,8 }},64 },{{{0 ,9 }},225 },{{{80 ,7 }},6 },{{{0 ,8 }},88 },{{{0 ,8 }},24 },{
{{0 ,9 }},145 },{{{83 ,7 }},59 },{{{0 ,8 }},120 },{{{0 ,8 }},56 },{{{0 ,9 }},209 },{
{{81 ,7 }},17 },{{{0 ,8 }},104 },{{{0 ,8 }},40 },{{{0 ,9 }},177 },{{{0 ,8 }},8 },{{{
0 ,8 }},136 },{{{0 ,8 }},72 },{{{0 ,9 }},241 },{{{80 ,7 }},4 },{{{0 ,8 }},84 },{{{0 ,
8 }},20 },{{{85 ,8 }},227 },{{{83 ,7 }},43 },{{{0 ,8 }},116 },{{{0 ,8 }},52 },{{{0 ,
9 }},201 },{{{81 ,7 }},13 },{{{0 ,8 }},100 },{{{0 ,8 }},36 },{{{0 ,9 }},169 },{{{0 ,
8 }},4 },{{{0 ,8 }},132 },{{{0 ,8 }},68 },{{{0 ,9 }},233 },{{{80 ,7 }},8 },{{{0 ,8 }}
,92 },{{{0 ,8 }},28 },{{{0 ,9 }},153 },{{{84 ,7 }},83 },{{{0 ,8 }},124 },{{{0 ,8 }},
60 },{{{0 ,9 }},217 },{{{82 ,7 }},23 },{{{0 ,8 }},108 },{{{0 ,8 }},44 },{{{0 ,9 }},
185 },{{{0 ,8 }},12 },{{{0 ,8 }},140 },{{{0 ,8 }},76 },{{{0 ,9 }},249 },{{{80 ,7 }},
3 },{{{0 ,8 }},82 },{{{0 ,8 }},18 },{{{85 ,8 }},163 },{{{83 ,7 }},35 },{{{0 ,8 }},
114 },{{{0 ,8 }},50 },{{{0 ,9 }},197 },{{{81 ,7 }},11 },{{{0 ,8 }},98 },{{{0 ,8 }},
34 },{{{0 ,9 }},165 },{{{0 ,8 }},2 },{{{0 ,8 }},130 },{{{0 ,8 }},66 },{{{0 ,9 }},229
},{{{80 ,7 }},7 },{{{0 ,8 }},90 },{{{0 ,8 }},26 },{{{0 ,9 }},149 },{{{84 ,7 }},67 },
{{{0 ,8 }},122 },{{{0 ,8 }},58 },{{{0 ,9 }},213 },{{{82 ,7 }},19 },{{{0 ,8 }},106 },
{{{0 ,8 }},42 },{{{0 ,9 }},181 },{{{0 ,8 }},10 },{{{0 ,8 }},138 },{{{0 ,8 }},74 },{{
{0 ,9 }},245 },{{{80 ,7 }},5 },{{{0 ,8 }},86 },{{{0 ,8 }},22 },{{{192 ,8 }},0 },{{{
83 ,7 }},51 },{{{0 ,8 }},118 },{{{0 ,8 }},54 },{{{0 ,9 }},205 },{{{81 ,7 }},15 },{{{
0 ,8 }},102 },{{{0 ,8 }},38 },{{{0 ,9 }},173 },{{{0 ,8 }},6 },{{{0 ,8 }},134 },{{{0 ,
8 }},70 },{{{0 ,9 }},237 },{{{80 ,7 }},9 },{{{0 ,8 }},94 },{{{0 ,8 }},30 },{{{0 ,9 }}
,157 },{{{84 ,7 }},99 },{{{0 ,8 }},126 },{{{0 ,8 }},62 },{{{0 ,9 }},221 },{{{82 ,7 }
},27 },{{{0 ,8 }},110 },{{{0 ,8 }},46 },{{{0 ,9 }},189 },{{{0 ,8 }},14 },{{{0 ,8 }},
142 },{{{0 ,8 }},78 },{{{0 ,9 }},253 },{{{96 ,7 }},256 },{{{0 ,8 }},81 },{{{0 ,8 }},
17 },{{{85 ,8 }},131 },{{{82 ,7 }},31 },{{{0 ,8 }},113 },{{{0 ,8 }},49 },{{{0 ,9 }},
195 },{{{80 ,7 }},10 },{{{0 ,8 }},97 },{{{0 ,8 }},33 },{{{0 ,9 }},163 },{{{0 ,8 }},1
},{{{0 ,8 }},129 },{{{0 ,8 }},65 },{{{0 ,9 }},227 },{{{80 ,7 }},6 },{{{0 ,8 }},89 },
{{{0 ,8 }},25 },{{{0 ,9 }},147 },{{{83 ,7 }},59 },{{{0 ,8 }},121 },{{{0 ,8 }},57 },{
{{0 ,9 }},211 },{{{81 ,7 }},17 },{{{0 ,8 }},105 },{{{0 ,8 }},41 },{{{0 ,9 }},179 },{
{{0 ,8 }},9 },{{{0 ,8 }},137 },{{{0 ,8 }},73 },{{{0 ,9 }},243 },{{{80 ,7 }},4 },{{{0
,8 }},85 },{{{0 ,8 }},21 },{{{80 ,8 }},258 },{{{83 ,7 }},43 },{{{0 ,8 }},117 },{{{0
,8 }},53 },{{{0 ,9 }},203 },{{{81 ,7 }},13 },{{{0 ,8 }},101 },{{{0 ,8 }},37 },{{{0 ,
9 }},171 },{{{0 ,8 }},5 },{{{0 ,8 }},133 },{{{0 ,8 }},69 },{{{0 ,9 }},235 },{{{80 ,7
}},8 },{{{0 ,8 }},93 },{{{0 ,8 }},29 },{{{0 ,9 }},155 },{{{84 ,7 }},83 },{{{0 ,8 }},
125 },{{{0 ,8 }},61 },{{{0 ,9 }},219 },{{{82 ,7 }},23 },{{{0 ,8 }},109 },{{{0 ,8 }},
45 },{{{0 ,9 }},187 },{{{0 ,8 }},13 },{{{0 ,8 }},141 },{{{0 ,8 }},77 },{{{0 ,9 }},
251 },{{{80 ,7 }},3 },{{{0 ,8 }},83 },{{{0 ,8 }},19 },{{{85 ,8 }},195 },{{{83 ,7 }},
35 },{{{0 ,8 }},115 },{{{0 ,8 }},51 },{{{0 ,9 }},199 },{{{81 ,7 }},11 },{{{0 ,8 }},
99 },{{{0 ,8 }},35 },{{{0 ,9 }},167 },{{{0 ,8 }},3 },{{{0 ,8 }},131 },{{{0 ,8 }},67 }
,{{{0 ,9 }},231 },{{{80 ,7 }},7 },{{{0 ,8 }},91 },{{{0 ,8 }},27 },{{{0 ,9 }},151 },{
{{84 ,7 }},67 },{{{0 ,8 }},123 },{{{0 ,8 }},59 },{{{0 ,9 }},215 },{{{82 ,7 }},19 },{
{{0 ,8 }},107 },{{{0 ,8 }},43 },{{{0 ,9 }},183 },{{{0 ,8 }},11 },{{{0 ,8 }},139 },{{
{0 ,8 }},75 },{{{0 ,9 }},247 },{{{80 ,7 }},5 },{{{0 ,8 }},87 },{{{0 ,8 }},23 },{{{
192 ,8 }},0 },{{{83 ,7 }},51 },{{{0 ,8 }},119 },{{{0 ,8 }},55 },{{{0 ,9 }},207 },{{{
81 ,7 }},15 },{{{0 ,8 }},103 },{{{0 ,8 }},39 },{{{0 ,9 }},175 },{{{0 ,8 }},7 },{{{0 ,
8 }},135 },{{{0 ,8 }},71 },{{{0 ,9 }},239 },{{{80 ,7 }},9 },{{{0 ,8 }},95 },{{{0 ,8 }
},31 },{{{0 ,9 }},159 },{{{84 ,7 }},99 },{{{0 ,8 }},127 },{{{0 ,8 }},63 },{{{0 ,9 }}
,223 },{{{82 ,7 }},27 },{{{0 ,8 }},111 },{{{0 ,8 }},47 },{{{0 ,9 }},191 },{{{0 ,8 }}
,15 },{{{0 ,8 }},143 },{{{0 ,8 }},79 },{{{0 ,9 }},255 }};bb40 bb154 bb2339[]={{
{{80 ,5 }},1 },{{{87 ,5 }},257 },{{{83 ,5 }},17 },{{{91 ,5 }},4097 },{{{81 ,5 }},5 }
,{{{89 ,5 }},1025 },{{{85 ,5 }},65 },{{{93 ,5 }},16385 },{{{80 ,5 }},3 },{{{88 ,5 }
},513 },{{{84 ,5 }},33 },{{{92 ,5 }},8193 },{{{82 ,5 }},9 },{{{90 ,5 }},2049 },{{{
86 ,5 }},129 },{{{192 ,5 }},24577 },{{{80 ,5 }},2 },{{{87 ,5 }},385 },{{{83 ,5 }},
25 },{{{91 ,5 }},6145 },{{{81 ,5 }},7 },{{{89 ,5 }},1537 },{{{85 ,5 }},97 },{{{93 ,
5 }},24577 },{{{80 ,5 }},4 },{{{88 ,5 }},769 },{{{84 ,5 }},49 },{{{92 ,5 }},12289 }
,{{{82 ,5 }},13 },{{{90 ,5 }},3073 },{{{86 ,5 }},193 },{{{192 ,5 }},24577 }};
#endif
bbe bb2025(bb58,bb967,bb1052,bb1030,bb0)bb165*bb58;bb165*bb967;bb154*
 *bb1052;bb154* *bb1030;bb16 bb0;{(bbb)bb0;
#ifdef bb2319
bbm(!bb2410){bbe bb6;bb9 bb20=0 ;bb165*bbo;bb165*bb448;bbm((bbo=(bb165
 * )( * ((bb0)->bb415))((bb0)->bb122,(288 ),(bb12(bb9))))==0 )bb4(-4 );
bbm((bb448=(bb165* )( * ((bb0)->bb415))((bb0)->bb122,(288 ),(bb12(bb9))))==
0 ){( * ((bb0)->bb381))((bb0)->bb122,(bb72)(bbo));bb4(-4 );}bb91(bb6=0 ;
bb6<144 ;bb6++)bbo[bb6]=8 ;bb91(;bb6<256 ;bb6++)bbo[bb6]=9 ;bb91(;bb6<280
;bb6++)bbo[bb6]=7 ;bb91(;bb6<288 ;bb6++)bbo[bb6]=8 ;bb2201=9 ;bb2033(bbo,
288 ,257 ,bb2447,bb2435,&bb2334,&bb2201,bb2510,&bb20,bb448);bb91(bb6=0 ;
bb6<30 ;bb6++)bbo[bb6]=5 ;bb2203=5 ;bb2033(bbo,30 ,0 ,bb2484,bb2485,&
bb2339,&bb2203,bb2510,&bb20,bb448);( * ((bb0)->bb381))((bb0)->bb122,(
bb72)(bb448));( * ((bb0)->bb381))((bb0)->bb122,(bb72)(bbo));bb2410=1 ;
}
#endif
 *bb58=bb2201; *bb967=bb2203; *bb1052=bb2334; *bb1030=bb2339;bb4 0 ;}
