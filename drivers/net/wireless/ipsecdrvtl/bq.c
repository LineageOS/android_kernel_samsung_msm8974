/*
   'src_compress_deflate_deflate.c' Obfuscated by COBF (Version 1.06 2006-01-07 by BB) at Fri Nov 13 10:03:51 2015
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
bb939));bba bbj bb2010{bb332{bb129 bb439;bb129 bb170;}bb293;bb332{
bb129 bb2234;bb129 bb22;}bb51;}bb472;bba bbj bb2347 bb2071;bba bbj
bb1996{bb472*bb1780;bbe bb532;bb2071*bb1734;}bb1765;bba bb129 bb1072;
bba bb1072 bb1397;bba bbt bb1356;bba bbj bb392{bb16 bb15;bbe bb367;
bb33*bb173;bb412 bb2191;bb33*bb1922;bbe bb188;bbe bb1389;bb153 bb1001
;bb153 bb588;bbe bb1948;bb9 bb958;bb9 bb2235;bb9 bb1850;bb33*bb158;
bb412 bb2297;bb1397*bb997;bb1397*bb397;bb9 bb514;bb9 bb1366;bb9 bb2202
;bb9 bb1756;bb9 bb1632;bb8 bb443;bb9 bb994;bb1356 bb2370;bbe bb1994;
bb9 bb191;bb9 bb2090;bb9 bb471;bb9 bb1317;bb9 bb2251;bb9 bb2123;bbe
bb126;bbe bb301;bb9 bb2255;bbe bb1888;bbj bb2010 bb1002[(2 * (256 +1 +29
)+1 )];bbj bb2010 bb1695[2 *30 +1 ];bbj bb2010 bb549[2 *19 +1 ];bbj bb1996
bb1993;bbj bb1996 bb1914;bbj bb1996 bb2128;bb129 bb1230[15 +1 ];bbe
bb543[2 * (256 +1 +29 )+1 ];bbe bb1523;bbe bb1999;bb156 bb1282[2 * (256 +1 +
29 )+1 ];bb1240*bb1741;bb9 bb1162;bb9 bb639;bb521*bb1664;bb412 bb1960;
bb412 bb2186;bb9 bb2300;bbe bb2055;bb129 bb102;bbe bb84;}bb192;bbb
bb2283 bbq((bb192*bbg));bbe bb2467 bbq((bb192*bbg,bbt bb429,bbt bb1144
));bbb bb1641 bbq((bb192*bbg,bb452*bb42,bb412 bb1334,bbe bb1146));bbb
bb2324 bbq((bb192*bbg));bbb bb2218 bbq((bb192*bbg,bb452*bb42,bb412
bb1334,bbe bb1146));bb40 bbh bb472 bb1766[(256 +1 +29 )+2 ]={{{12 },{8 }},{
{140 },{8 }},{{76 },{8 }},{{204 },{8 }},{{44 },{8 }},{{172 },{8 }},{{108 },{8 }},
{{236 },{8 }},{{28 },{8 }},{{156 },{8 }},{{92 },{8 }},{{220 },{8 }},{{60 },{8 }},
{{188 },{8 }},{{124 },{8 }},{{252 },{8 }},{{2 },{8 }},{{130 },{8 }},{{66 },{8 }},
{{194 },{8 }},{{34 },{8 }},{{162 },{8 }},{{98 },{8 }},{{226 },{8 }},{{18 },{8 }},
{{146 },{8 }},{{82 },{8 }},{{210 },{8 }},{{50 },{8 }},{{178 },{8 }},{{114 },{8 }}
,{{242 },{8 }},{{10 },{8 }},{{138 },{8 }},{{74 },{8 }},{{202 },{8 }},{{42 },{8 }}
,{{170 },{8 }},{{106 },{8 }},{{234 },{8 }},{{26 },{8 }},{{154 },{8 }},{{90 },{8 }
},{{218 },{8 }},{{58 },{8 }},{{186 },{8 }},{{122 },{8 }},{{250 },{8 }},{{6 },{8 }
},{{134 },{8 }},{{70 },{8 }},{{198 },{8 }},{{38 },{8 }},{{166 },{8 }},{{102 },{8
}},{{230 },{8 }},{{22 },{8 }},{{150 },{8 }},{{86 },{8 }},{{214 },{8 }},{{54 },{8
}},{{182 },{8 }},{{118 },{8 }},{{246 },{8 }},{{14 },{8 }},{{142 },{8 }},{{78 },{
8 }},{{206 },{8 }},{{46 },{8 }},{{174 },{8 }},{{110 },{8 }},{{238 },{8 }},{{30 },
{8 }},{{158 },{8 }},{{94 },{8 }},{{222 },{8 }},{{62 },{8 }},{{190 },{8 }},{{126 }
,{8 }},{{254 },{8 }},{{1 },{8 }},{{129 },{8 }},{{65 },{8 }},{{193 },{8 }},{{33 },
{8 }},{{161 },{8 }},{{97 },{8 }},{{225 },{8 }},{{17 },{8 }},{{145 },{8 }},{{81 },
{8 }},{{209 },{8 }},{{49 },{8 }},{{177 },{8 }},{{113 },{8 }},{{241 },{8 }},{{9 },
{8 }},{{137 },{8 }},{{73 },{8 }},{{201 },{8 }},{{41 },{8 }},{{169 },{8 }},{{105 }
,{8 }},{{233 },{8 }},{{25 },{8 }},{{153 },{8 }},{{89 },{8 }},{{217 },{8 }},{{57 }
,{8 }},{{185 },{8 }},{{121 },{8 }},{{249 },{8 }},{{5 },{8 }},{{133 },{8 }},{{69 }
,{8 }},{{197 },{8 }},{{37 },{8 }},{{165 },{8 }},{{101 },{8 }},{{229 },{8 }},{{21
},{8 }},{{149 },{8 }},{{85 },{8 }},{{213 },{8 }},{{53 },{8 }},{{181 },{8 }},{{
117 },{8 }},{{245 },{8 }},{{13 },{8 }},{{141 },{8 }},{{77 },{8 }},{{205 },{8 }},{
{45 },{8 }},{{173 },{8 }},{{109 },{8 }},{{237 },{8 }},{{29 },{8 }},{{157 },{8 }},
{{93 },{8 }},{{221 },{8 }},{{61 },{8 }},{{189 },{8 }},{{125 },{8 }},{{253 },{8 }}
,{{19 },{9 }},{{275 },{9 }},{{147 },{9 }},{{403 },{9 }},{{83 },{9 }},{{339 },{9 }
},{{211 },{9 }},{{467 },{9 }},{{51 },{9 }},{{307 },{9 }},{{179 },{9 }},{{435 },{
9 }},{{115 },{9 }},{{371 },{9 }},{{243 },{9 }},{{499 },{9 }},{{11 },{9 }},{{267 }
,{9 }},{{139 },{9 }},{{395 },{9 }},{{75 },{9 }},{{331 },{9 }},{{203 },{9 }},{{
459 },{9 }},{{43 },{9 }},{{299 },{9 }},{{171 },{9 }},{{427 },{9 }},{{107 },{9 }},
{{363 },{9 }},{{235 },{9 }},{{491 },{9 }},{{27 },{9 }},{{283 },{9 }},{{155 },{9 }
},{{411 },{9 }},{{91 },{9 }},{{347 },{9 }},{{219 },{9 }},{{475 },{9 }},{{59 },{9
}},{{315 },{9 }},{{187 },{9 }},{{443 },{9 }},{{123 },{9 }},{{379 },{9 }},{{251 }
,{9 }},{{507 },{9 }},{{7 },{9 }},{{263 },{9 }},{{135 },{9 }},{{391 },{9 }},{{71 }
,{9 }},{{327 },{9 }},{{199 },{9 }},{{455 },{9 }},{{39 },{9 }},{{295 },{9 }},{{
167 },{9 }},{{423 },{9 }},{{103 },{9 }},{{359 },{9 }},{{231 },{9 }},{{487 },{9 }}
,{{23 },{9 }},{{279 },{9 }},{{151 },{9 }},{{407 },{9 }},{{87 },{9 }},{{343 },{9 }
},{{215 },{9 }},{{471 },{9 }},{{55 },{9 }},{{311 },{9 }},{{183 },{9 }},{{439 },{
9 }},{{119 },{9 }},{{375 },{9 }},{{247 },{9 }},{{503 },{9 }},{{15 },{9 }},{{271 }
,{9 }},{{143 },{9 }},{{399 },{9 }},{{79 },{9 }},{{335 },{9 }},{{207 },{9 }},{{
463 },{9 }},{{47 },{9 }},{{303 },{9 }},{{175 },{9 }},{{431 },{9 }},{{111 },{9 }},
{{367 },{9 }},{{239 },{9 }},{{495 },{9 }},{{31 },{9 }},{{287 },{9 }},{{159 },{9 }
},{{415 },{9 }},{{95 },{9 }},{{351 },{9 }},{{223 },{9 }},{{479 },{9 }},{{63 },{9
}},{{319 },{9 }},{{191 },{9 }},{{447 },{9 }},{{127 },{9 }},{{383 },{9 }},{{255 }
,{9 }},{{511 },{9 }},{{0 },{7 }},{{64 },{7 }},{{32 },{7 }},{{96 },{7 }},{{16 },{7
}},{{80 },{7 }},{{48 },{7 }},{{112 },{7 }},{{8 },{7 }},{{72 },{7 }},{{40 },{7 }},
{{104 },{7 }},{{24 },{7 }},{{88 },{7 }},{{56 },{7 }},{{120 },{7 }},{{4 },{7 }},{{
68 },{7 }},{{36 },{7 }},{{100 },{7 }},{{20 },{7 }},{{84 },{7 }},{{52 },{7 }},{{
116 },{7 }},{{3 },{8 }},{{131 },{8 }},{{67 },{8 }},{{195 },{8 }},{{35 },{8 }},{{
163 },{8 }},{{99 },{8 }},{{227 },{8 }}};bb40 bbh bb472 bb2304[30 ]={{{0 },{5 }
},{{16 },{5 }},{{8 },{5 }},{{24 },{5 }},{{4 },{5 }},{{20 },{5 }},{{12 },{5 }},{{
28 },{5 }},{{2 },{5 }},{{18 },{5 }},{{10 },{5 }},{{26 },{5 }},{{6 },{5 }},{{22 },{
5 }},{{14 },{5 }},{{30 },{5 }},{{1 },{5 }},{{17 },{5 }},{{9 },{5 }},{{25 },{5 }},{
{5 },{5 }},{{21 },{5 }},{{13 },{5 }},{{29 },{5 }},{{3 },{5 }},{{19 },{5 }},{{11 },
{5 }},{{27 },{5 }},{{7 },{5 }},{{23 },{5 }}};bb40 bbh bb156 bb1764[512 ]={0 ,1
,2 ,3 ,4 ,4 ,5 ,5 ,6 ,6 ,6 ,6 ,7 ,7 ,7 ,7 ,8 ,8 ,8 ,8 ,8 ,8 ,8 ,8 ,9 ,9 ,9 ,9 ,9 ,9 ,9 ,9 ,10 ,10 ,10
,10 ,10 ,10 ,10 ,10 ,10 ,10 ,10 ,10 ,10 ,10 ,10 ,10 ,11 ,11 ,11 ,11 ,11 ,11 ,11 ,11 ,11 ,11
,11 ,11 ,11 ,11 ,11 ,11 ,12 ,12 ,12 ,12 ,12 ,12 ,12 ,12 ,12 ,12 ,12 ,12 ,12 ,12 ,12 ,12 ,12
,12 ,12 ,12 ,12 ,12 ,12 ,12 ,12 ,12 ,12 ,12 ,12 ,12 ,12 ,12 ,13 ,13 ,13 ,13 ,13 ,13 ,13 ,13
,13 ,13 ,13 ,13 ,13 ,13 ,13 ,13 ,13 ,13 ,13 ,13 ,13 ,13 ,13 ,13 ,13 ,13 ,13 ,13 ,13 ,13 ,13
,13 ,14 ,14 ,14 ,14 ,14 ,14 ,14 ,14 ,14 ,14 ,14 ,14 ,14 ,14 ,14 ,14 ,14 ,14 ,14 ,14 ,14 ,14
,14 ,14 ,14 ,14 ,14 ,14 ,14 ,14 ,14 ,14 ,14 ,14 ,14 ,14 ,14 ,14 ,14 ,14 ,14 ,14 ,14 ,14 ,14
,14 ,14 ,14 ,14 ,14 ,14 ,14 ,14 ,14 ,14 ,14 ,14 ,14 ,14 ,14 ,14 ,14 ,14 ,14 ,15 ,15 ,15 ,15
,15 ,15 ,15 ,15 ,15 ,15 ,15 ,15 ,15 ,15 ,15 ,15 ,15 ,15 ,15 ,15 ,15 ,15 ,15 ,15 ,15 ,15 ,15
,15 ,15 ,15 ,15 ,15 ,15 ,15 ,15 ,15 ,15 ,15 ,15 ,15 ,15 ,15 ,15 ,15 ,15 ,15 ,15 ,15 ,15 ,15
,15 ,15 ,15 ,15 ,15 ,15 ,15 ,15 ,15 ,15 ,15 ,15 ,15 ,15 ,0 ,0 ,16 ,17 ,18 ,18 ,19 ,19 ,20 ,
20 ,20 ,20 ,21 ,21 ,21 ,21 ,22 ,22 ,22 ,22 ,22 ,22 ,22 ,22 ,23 ,23 ,23 ,23 ,23 ,23 ,23 ,23 ,
24 ,24 ,24 ,24 ,24 ,24 ,24 ,24 ,24 ,24 ,24 ,24 ,24 ,24 ,24 ,24 ,25 ,25 ,25 ,25 ,25 ,25 ,25 ,
25 ,25 ,25 ,25 ,25 ,25 ,25 ,25 ,25 ,26 ,26 ,26 ,26 ,26 ,26 ,26 ,26 ,26 ,26 ,26 ,26 ,26 ,26 ,
26 ,26 ,26 ,26 ,26 ,26 ,26 ,26 ,26 ,26 ,26 ,26 ,26 ,26 ,26 ,26 ,26 ,26 ,27 ,27 ,27 ,27 ,27 ,
27 ,27 ,27 ,27 ,27 ,27 ,27 ,27 ,27 ,27 ,27 ,27 ,27 ,27 ,27 ,27 ,27 ,27 ,27 ,27 ,27 ,27 ,27 ,
27 ,27 ,27 ,27 ,28 ,28 ,28 ,28 ,28 ,28 ,28 ,28 ,28 ,28 ,28 ,28 ,28 ,28 ,28 ,28 ,28 ,28 ,28 ,
28 ,28 ,28 ,28 ,28 ,28 ,28 ,28 ,28 ,28 ,28 ,28 ,28 ,28 ,28 ,28 ,28 ,28 ,28 ,28 ,28 ,28 ,28 ,
28 ,28 ,28 ,28 ,28 ,28 ,28 ,28 ,28 ,28 ,28 ,28 ,28 ,28 ,28 ,28 ,28 ,28 ,28 ,28 ,28 ,28 ,29 ,
29 ,29 ,29 ,29 ,29 ,29 ,29 ,29 ,29 ,29 ,29 ,29 ,29 ,29 ,29 ,29 ,29 ,29 ,29 ,29 ,29 ,29 ,29 ,
29 ,29 ,29 ,29 ,29 ,29 ,29 ,29 ,29 ,29 ,29 ,29 ,29 ,29 ,29 ,29 ,29 ,29 ,29 ,29 ,29 ,29 ,29 ,
29 ,29 ,29 ,29 ,29 ,29 ,29 ,29 ,29 ,29 ,29 ,29 ,29 ,29 ,29 ,29 ,29 };bb40 bbh bb156
bb2118[258 -3 +1 ]={0 ,1 ,2 ,3 ,4 ,5 ,6 ,7 ,8 ,8 ,9 ,9 ,10 ,10 ,11 ,11 ,12 ,12 ,12 ,12 ,13 ,
13 ,13 ,13 ,14 ,14 ,14 ,14 ,15 ,15 ,15 ,15 ,16 ,16 ,16 ,16 ,16 ,16 ,16 ,16 ,17 ,17 ,17 ,17 ,
17 ,17 ,17 ,17 ,18 ,18 ,18 ,18 ,18 ,18 ,18 ,18 ,19 ,19 ,19 ,19 ,19 ,19 ,19 ,19 ,20 ,20 ,20 ,
20 ,20 ,20 ,20 ,20 ,20 ,20 ,20 ,20 ,20 ,20 ,20 ,20 ,21 ,21 ,21 ,21 ,21 ,21 ,21 ,21 ,21 ,21 ,
21 ,21 ,21 ,21 ,21 ,21 ,22 ,22 ,22 ,22 ,22 ,22 ,22 ,22 ,22 ,22 ,22 ,22 ,22 ,22 ,22 ,22 ,23 ,
23 ,23 ,23 ,23 ,23 ,23 ,23 ,23 ,23 ,23 ,23 ,23 ,23 ,23 ,23 ,24 ,24 ,24 ,24 ,24 ,24 ,24 ,24 ,
24 ,24 ,24 ,24 ,24 ,24 ,24 ,24 ,24 ,24 ,24 ,24 ,24 ,24 ,24 ,24 ,24 ,24 ,24 ,24 ,24 ,24 ,24 ,
24 ,25 ,25 ,25 ,25 ,25 ,25 ,25 ,25 ,25 ,25 ,25 ,25 ,25 ,25 ,25 ,25 ,25 ,25 ,25 ,25 ,25 ,25 ,
25 ,25 ,25 ,25 ,25 ,25 ,25 ,25 ,25 ,25 ,26 ,26 ,26 ,26 ,26 ,26 ,26 ,26 ,26 ,26 ,26 ,26 ,26 ,
26 ,26 ,26 ,26 ,26 ,26 ,26 ,26 ,26 ,26 ,26 ,26 ,26 ,26 ,26 ,26 ,26 ,26 ,26 ,27 ,27 ,27 ,27 ,
27 ,27 ,27 ,27 ,27 ,27 ,27 ,27 ,27 ,27 ,27 ,27 ,27 ,27 ,27 ,27 ,27 ,27 ,27 ,27 ,27 ,27 ,27 ,
27 ,27 ,27 ,27 ,28 };bb40 bbh bbe bb2408[29 ]={0 ,1 ,2 ,3 ,4 ,5 ,6 ,7 ,8 ,10 ,12 ,14 ,
16 ,20 ,24 ,28 ,32 ,40 ,48 ,56 ,64 ,80 ,96 ,112 ,128 ,160 ,192 ,224 ,0 };bb40 bbh bbe
bb2493[30 ]={0 ,1 ,2 ,3 ,4 ,6 ,8 ,12 ,16 ,24 ,32 ,48 ,64 ,96 ,128 ,192 ,256 ,384 ,512 ,
768 ,1024 ,1536 ,2048 ,3072 ,4096 ,6144 ,8192 ,12288 ,16384 ,24576 };bba bb13{
bb1326,bb2230,bb1794,bb2213}bb1869;bba bb1869( *bb2464)bbq((bb192*bbg
,bbe bb176));bb40 bbb bb2264 bbq((bb192*bbg));bb40 bb1869 bb2460 bbq(
(bb192*bbg,bbe bb176));bb40 bb1869 bb2231 bbq((bb192*bbg,bbe bb176));
bb40 bb1869 bb2004 bbq((bb192*bbg,bbe bb176));bb40 bbb bb2509 bbq((
bb192*bbg));bb40 bbb bb2073 bbq((bb192*bbg,bb9 bbp));bb40 bbb bb1309
bbq((bb16 bb15));bb40 bbe bb2513 bbq((bb16 bb15,bb33*bb42,bbt bb48));
bb40 bb9 bb2328 bbq((bb192*bbg,bb1356 bb2056));bba bbj bb2702{bb129
bb2512;bb129 bb2515;bb129 bb2461;bb129 bb2421;bb2464 bb2086;}bb2583;
bb40 bbh bb2583 bb1369[10 ]={{0 ,0 ,0 ,0 ,bb2460},{4 ,4 ,8 ,4 ,bb2231},{4 ,5 ,16
,8 ,bb2231},{4 ,6 ,32 ,32 ,bb2231},{4 ,4 ,16 ,16 ,bb2004},{8 ,16 ,32 ,32 ,bb2004},
{8 ,16 ,128 ,128 ,bb2004},{8 ,32 ,128 ,256 ,bb2004},{32 ,128 ,258 ,1024 ,bb2004},
{32 ,258 ,258 ,4096 ,bb2004}};bbj bb2347{bbe bb463;};bbe bb1148(bb15,
bb126,bb194,bb197)bb16 bb15;bbe bb126;bbh bbl*bb194;bbe bb197;{bb4
bb1079(bb15,bb126,8 ,15 ,bb798,0 ,bb194,bb197);}bbe bb1079(bb15,bb126,
bb588,bb466,bb975,bb301,bb194,bb197)bb16 bb15;bbe bb126;bbe bb588;bbe
bb466;bbe bb975;bbe bb301;bbh bbl*bb194;bbe bb197;{bb192*bbg;bbe
bb1389=0 ;bb40 bbh bbl*bb2655="\x31\x2e\x31\x2e\x33";bb521*bb1934;bbm(
bb194==0 ||bb194[0 ]!=bb2655[0 ]||bb197!=bb12(bb450)){bb4(-6 );}bbm(bb15
==0 )bb4(-2 );bb15->bb327=0 ;bbm(bb15->bb415==0 ){bb15->bb415=bb1211;bb15
->bb122=(bb72)0 ;}bbm(bb15->bb381==0 )bb15->bb381=bb1209;bbm(bb126==(-1
))bb126=6 ;bbm(bb466<0 ){bb1389=1 ;bb466=-bb466;}bbm(bb975<1 ||bb975>
bb292||bb588!=8 ||bb466<8 ||bb466>15 ||bb126<0 ||bb126>9 ||bb301<0 ||bb301>
2 ){bb4(-2 );}bbg=(bb192* )( * ((bb15)->bb415))((bb15)->bb122,(1 ),(bb12
(bb192)));bbm(bbg==0 )bb4(-4 );bb15->bb23=(bbj bb392* )bbg;bbg->bb15=
bb15;bbg->bb1389=bb1389;bbg->bb2235=bb466;bbg->bb958=1 <<bbg->bb2235;
bbg->bb1850=bbg->bb958-1 ;bbg->bb2202=bb975+7 ;bbg->bb1366=1 <<bbg->
bb2202;bbg->bb1756=bbg->bb1366-1 ;bbg->bb1632=((bbg->bb2202+3 -1 )/3 );
bbg->bb158=(bb33* )( * ((bb15)->bb415))((bb15)->bb122,(bbg->bb958),(2
 *bb12(bb153)));bbg->bb997=(bb1397* )( * ((bb15)->bb415))((bb15)->
bb122,(bbg->bb958),(bb12(bb1072)));bbg->bb397=(bb1397* )( * ((bb15)->
bb415))((bb15)->bb122,(bbg->bb1366),(bb12(bb1072)));bbg->bb1162=1 <<(
bb975+6 );bb1934=(bb521* )( * ((bb15)->bb415))((bb15)->bb122,(bbg->
bb1162),(bb12(bb129)+2 ));bbg->bb173=(bb1240* )bb1934;bbg->bb2191=(
bb412)bbg->bb1162* (bb12(bb129)+2L );bbm(bbg->bb158==0 ||bbg->bb997==0
||bbg->bb397==0 ||bbg->bb173==0 ){bb15->bb327=(bbl* )bb1097[2 -((-4 ))];
bb970(bb15);bb4(-4 );}bbg->bb1664=bb1934+bbg->bb1162/bb12(bb129);bbg->
bb1741=bbg->bb173+(1 +bb12(bb129)) *bbg->bb1162;bbg->bb126=bb126;bbg->
bb301=bb301;bbg->bb588=(bb153)bb588;bb4 bb1086(bb15);}bbe bb1217(bb15
,bb441,bb451)bb16 bb15;bbh bb33*bb441;bb9 bb451;{bb192*bbg;bb9 bb479=
bb451;bb9 bb11;bb1356 bb1167=0 ;bbm(bb15==0 ||bb15->bb23==0 ||bb441==0 ||
bb15->bb23->bb367!=42 )bb4(-2 );bbg=bb15->bb23;bb15->bb378=bb1018(bb15
->bb378,bb441,bb451);bbm(bb479<3 )bb4 0 ;bbm(bb479>((bbg)->bb958-(258 +3
+1 ))){bb479=((bbg)->bb958-(258 +3 +1 ));bb441+=bb451-bb479;}bb75(bbg->
bb158,bb441,bb479);bbg->bb191=bb479;bbg->bb443=(bb8)bb479;bbg->bb514=
bbg->bb158[0 ];(bbg->bb514=(((bbg->bb514)<<bbg->bb1632)^(bbg->bb158[1 ]
))&bbg->bb1756);bb91(bb11=0 ;bb11<=bb479-3 ;bb11++){((bbg->bb514=(((bbg
->bb514)<<bbg->bb1632)^(bbg->bb158[(bb11)+(3 -1 )]))&bbg->bb1756),bbg->
bb997[(bb11)&bbg->bb1850]=bb1167=bbg->bb397[bbg->bb514],bbg->bb397[
bbg->bb514]=(bb1072)(bb11));}bbm(bb1167)bb1167=0 ;bb4 0 ;}bbe bb1086(
bb15)bb16 bb15;{bb192*bbg;bbm(bb15==0 ||bb15->bb23==0 ||bb15->bb415==0
||bb15->bb381==0 )bb4(-2 );bb15->bb195=bb15->bb637=0 ;bb15->bb327=0 ;bb15
->bb1001=2 ;bbg=(bb192* )bb15->bb23;bbg->bb188=0 ;bbg->bb1922=bbg->
bb173;bbm(bbg->bb1389<0 ){bbg->bb1389=0 ;}bbg->bb367=bbg->bb1389?113 :42
;bb15->bb378=1 ;bbg->bb1948=0 ;bb2283(bbg);bb2509(bbg);bb4 0 ;}bbe bb1202
(bb15,bb126,bb301)bb16 bb15;bbe bb126;bbe bb301;{bb192*bbg;bb2464
bb2086;bbe bb18=0 ;bbm(bb15==0 ||bb15->bb23==0 )bb4(-2 );bbg=bb15->bb23;
bbm(bb126==(-1 )){bb126=6 ;}bbm(bb126<0 ||bb126>9 ||bb301<0 ||bb301>2 ){bb4
(-2 );}bb2086=bb1369[bbg->bb126].bb2086;bbm(bb2086!=bb1369[bb126].
bb2086&&bb15->bb195!=0 ){bb18=bb524(bb15,1 );}bbm(bbg->bb126!=bb126){
bbg->bb126=bb126;bbg->bb2123=bb1369[bb126].bb2515;bbg->bb2255=bb1369[
bb126].bb2512;bbg->bb1888=bb1369[bb126].bb2461;bbg->bb2251=bb1369[
bb126].bb2421;}bbg->bb301=bb301;bb4 bb18;}bb40 bbb bb2073(bbg,bbp)bb192
 *bbg;bb9 bbp;{{bbg->bb173[bbg->bb188++]=((bb153)(bbp>>8 ));};{bbg->
bb173[bbg->bb188++]=((bb153)(bbp&0xff ));};}bb40 bbb bb1309(bb15)bb16
bb15;{bbt bb22=bb15->bb23->bb188;bbm(bb22>bb15->bb396)bb22=bb15->
bb396;bbm(bb22==0 )bb4;bb75(bb15->bb620,bb15->bb23->bb1922,bb22);bb15
->bb620+=bb22;bb15->bb23->bb1922+=bb22;bb15->bb637+=bb22;bb15->bb396
-=bb22;bb15->bb23->bb188-=bb22;bbm(bb15->bb23->bb188==0 ){bb15->bb23->
bb1922=bb15->bb23->bb173;}}bbe bb524(bb15,bb176)bb16 bb15;bbe bb176;{
bbe bb2418;bb192*bbg;bbm(bb15==0 ||bb15->bb23==0 ||bb176>4 ||bb176<0 ){
bb4(-2 );}bbg=bb15->bb23;bbm(bb15->bb620==0 ||(bb15->bb127==0 &&bb15->
bb149!=0 )||(bbg->bb367==666 &&bb176!=4 )){bb4(bb15->bb327=(bbl* )bb1097
[2 -((-2 ))],((-2 )));}bbm(bb15->bb396==0 )bb4(bb15->bb327=(bbl* )bb1097[
2 -((-5 ))],((-5 )));bbg->bb15=bb15;bb2418=bbg->bb1948;bbg->bb1948=bb176
;bbm(bbg->bb367==42 ){bb9 bb1015=(8 +((bbg->bb2235-8 )<<4 ))<<8 ;bb9 bb2372
=(bbg->bb126-1 )>>1 ;bbm(bb2372>3 )bb2372=3 ;bb1015|=(bb2372<<6 );bbm(bbg
->bb191!=0 )bb1015|=0x20 ;bb1015+=31 -(bb1015%31 );bbg->bb367=113 ;bb2073(
bbg,bb1015);bbm(bbg->bb191!=0 ){bb2073(bbg,(bb9)(bb15->bb378>>16 ));
bb2073(bbg,(bb9)(bb15->bb378&0xffff ));}bb15->bb378=1L ;}bbm(bbg->bb188
!=0 ){bb1309(bb15);bbm(bb15->bb396==0 ){bbg->bb1948=-1 ;bb4 0 ;}}bb50 bbm
(bb15->bb149==0 &&bb176<=bb2418&&bb176!=4 ){bb4(bb15->bb327=(bbl* )bb1097
[2 -((-5 ))],((-5 )));}bbm(bbg->bb367==666 &&bb15->bb149!=0 ){bb4(bb15->
bb327=(bbl* )bb1097[2 -((-5 ))],((-5 )));}bbm(bb15->bb149!=0 ||bbg->bb471
!=0 ||(bb176!=0 &&bbg->bb367!=666 )){bb1869 bb2046;bb2046=( * (bb1369[
bbg->bb126].bb2086))(bbg,bb176);bbm(bb2046==bb1794||bb2046==bb2213){
bbg->bb367=666 ;}bbm(bb2046==bb1326||bb2046==bb1794){bbm(bb15->bb396==
0 ){bbg->bb1948=-1 ;}bb4 0 ;}bbm(bb2046==bb2230){bbm(bb176==1 ){bb2324(
bbg);}bb50{bb2218(bbg,(bbl* )0 ,0L ,0 );bbm(bb176==3 ){bbg->bb397[bbg->
bb1366-1 ]=0 ;bb961((bb33* )bbg->bb397,0 ,(bbt)(bbg->bb1366-1 ) *bb12( *
bbg->bb397));;}}bb1309(bb15);bbm(bb15->bb396==0 ){bbg->bb1948=-1 ;bb4 0
;}}};bbm(bb176!=4 )bb4 0 ;bbm(bbg->bb1389)bb4 1 ;bb2073(bbg,(bb9)(bb15->
bb378>>16 ));bb2073(bbg,(bb9)(bb15->bb378&0xffff ));bb1309(bb15);bbg->
bb1389=-1 ;bb4 bbg->bb188!=0 ?0 :1 ;}bbe bb970(bb15)bb16 bb15;{bbe bb367;
bbm(bb15==0 ||bb15->bb23==0 )bb4(-2 );bb367=bb15->bb23->bb367;bbm(bb367
!=42 &&bb367!=113 &&bb367!=666 ){bb4(-2 );}{bbm(bb15->bb23->bb173)( * ((
bb15)->bb381))((bb15)->bb122,(bb72)(bb15->bb23->bb173));};{bbm(bb15->
bb23->bb397)( * ((bb15)->bb381))((bb15)->bb122,(bb72)(bb15->bb23->
bb397));};{bbm(bb15->bb23->bb997)( * ((bb15)->bb381))((bb15)->bb122,(
bb72)(bb15->bb23->bb997));};{bbm(bb15->bb23->bb158)( * ((bb15)->bb381
))((bb15)->bb122,(bb72)(bb15->bb23->bb158));};( * ((bb15)->bb381))((
bb15)->bb122,(bb72)(bb15->bb23));bb15->bb23=0 ;bb4 bb367==113 ?(-3 ):0 ;}
bbe bb1187(bb132,bb186)bb16 bb132;bb16 bb186;{
#ifdef bb529
bb4(-2 );
#else
bb192*bb445;bb192*bb1870;bb521*bb1934;bbm(bb186==0 ||bb132==0 ||bb186->
bb23==0 ){bb4(-2 );}bb1870=bb186->bb23; *bb132= *bb186;bb445=(bb192* )(
 * ((bb132)->bb415))((bb132)->bb122,(1 ),(bb12(bb192)));bbm(bb445==0 )bb4
(-4 );bb132->bb23=(bbj bb392* )bb445; *bb445= *bb1870;bb445->bb15=
bb132;bb445->bb158=(bb33* )( * ((bb132)->bb415))((bb132)->bb122,(
bb445->bb958),(2 *bb12(bb153)));bb445->bb997=(bb1397* )( * ((bb132)->
bb415))((bb132)->bb122,(bb445->bb958),(bb12(bb1072)));bb445->bb397=(
bb1397* )( * ((bb132)->bb415))((bb132)->bb122,(bb445->bb1366),(bb12(
bb1072)));bb1934=(bb521* )( * ((bb132)->bb415))((bb132)->bb122,(bb445
->bb1162),(bb12(bb129)+2 ));bb445->bb173=(bb1240* )bb1934;bbm(bb445->
bb158==0 ||bb445->bb997==0 ||bb445->bb397==0 ||bb445->bb173==0 ){bb970(
bb132);bb4(-4 );}bb75(bb445->bb158,bb1870->bb158,bb445->bb958*2 *bb12(
bb153));bb75((bb33* )bb445->bb997,(bb33* )bb1870->bb997,bb445->bb958*
bb12(bb1072));bb75((bb33* )bb445->bb397,(bb33* )bb1870->bb397,bb445->
bb1366*bb12(bb1072));bb75(bb445->bb173,bb1870->bb173,(bb9)bb445->
bb2191);bb445->bb1922=bb445->bb173+(bb1870->bb1922-bb1870->bb173);
bb445->bb1664=bb1934+bb445->bb1162/bb12(bb129);bb445->bb1741=bb445->
bb173+(1 +bb12(bb129)) *bb445->bb1162;bb445->bb1993.bb1780=bb445->
bb1002;bb445->bb1914.bb1780=bb445->bb1695;bb445->bb2128.bb1780=bb445
->bb549;bb4 0 ;
#endif
}bb40 bbe bb2513(bb15,bb42,bb48)bb16 bb15;bb33*bb42;bbt bb48;{bbt bb22
=bb15->bb149;bbm(bb22>bb48)bb22=bb48;bbm(bb22==0 )bb4 0 ;bb15->bb149-=
bb22;bbm(!bb15->bb23->bb1389){bb15->bb378=bb1018(bb15->bb378,bb15->
bb127,bb22);}bb75(bb42,bb15->bb127,bb22);bb15->bb127+=bb22;bb15->
bb195+=bb22;bb4(bbe)bb22;}bb40 bbb bb2509(bbg)bb192*bbg;{bbg->bb2297=
(bb412)2L *bbg->bb958;bbg->bb397[bbg->bb1366-1 ]=0 ;bb961((bb33* )bbg->
bb397,0 ,(bbt)(bbg->bb1366-1 ) *bb12( *bbg->bb397));;bbg->bb2123=bb1369
[bbg->bb126].bb2515;bbg->bb2255=bb1369[bbg->bb126].bb2512;bbg->bb1888
=bb1369[bbg->bb126].bb2461;bbg->bb2251=bb1369[bbg->bb126].bb2421;bbg
->bb191=0 ;bbg->bb443=0L ;bbg->bb471=0 ;bbg->bb994=bbg->bb1317=3 -1 ;bbg->
bb1994=0 ;bbg->bb514=0 ;}bb40 bb9 bb2328(bbg,bb2056)bb192*bbg;bb1356
bb2056;{bbt bb2390=bbg->bb2251;bb950 bb33*bb517=bbg->bb158+bbg->bb191
;bb950 bb33*bb642;bb950 bbe bb22;bbe bb1226=bbg->bb1317;bbe bb1888=
bbg->bb1888;bb1356 bb2654=bbg->bb191>(bb1356)((bbg)->bb958-(258 +3 +1 ))?
bbg->bb191-(bb1356)((bbg)->bb958-(258 +3 +1 )):0 ;bb1397*bb997=bbg->bb997
;bb9 bb2548=bbg->bb1850;
#ifdef bb1076
bb950 bb33*bb1947=bbg->bb158+bbg->bb191+258 -1 ;bb950 bb129 bb2612= * (
bb521* )bb517;bb950 bb129 bb2189= * (bb521* )(bb517+bb1226-1 );
#else
bb950 bb33*bb1947=bbg->bb158+bbg->bb191+258 ;bb950 bb153 bb2389=bb517[
bb1226-1 ];bb950 bb153 bb2189=bb517[bb1226];
#endif
;bbm(bbg->bb1317>=bbg->bb2255){bb2390>>=2 ;}bbm((bb9)bb1888>bbg->bb471
)bb1888=bbg->bb471;;bb595{;bb642=bbg->bb158+bb2056;
#if ( defined( bb1076) && bb2662 == 258)
bbm( * (bb521* )(bb642+bb1226-1 )!=bb2189|| * (bb521* )bb642!=bb2612)bb1699
;;bb517++,bb642++;bb595{}bb110( * (bb521* )(bb517+=2 )== * (bb521* )(
bb642+=2 )&& * (bb521* )(bb517+=2 )== * (bb521* )(bb642+=2 )&& * (bb521*
)(bb517+=2 )== * (bb521* )(bb642+=2 )&& * (bb521* )(bb517+=2 )== * (
bb521* )(bb642+=2 )&&bb517<bb1947);;bbm( *bb517== *bb642)bb517++;bb22=
(258 -1 )-(bbe)(bb1947-bb517);bb517=bb1947-(258 -1 );
#else
bbm(bb642[bb1226]!=bb2189||bb642[bb1226-1 ]!=bb2389|| *bb642!= *bb517
|| * ++bb642!=bb517[1 ])bb1699;bb517+=2 ,bb642++;;bb595{}bb110( * ++
bb517== * ++bb642&& * ++bb517== * ++bb642&& * ++bb517== * ++bb642&& *
++bb517== * ++bb642&& * ++bb517== * ++bb642&& * ++bb517== * ++bb642&&
 * ++bb517== * ++bb642&& * ++bb517== * ++bb642&&bb517<bb1947);;bb22=
258 -(bbe)(bb1947-bb517);bb517=bb1947-258 ;
#endif
bbm(bb22>bb1226){bbg->bb2090=bb2056;bb1226=bb22;bbm(bb22>=bb1888)bb21
;
#ifdef bb1076
bb2189= * (bb521* )(bb517+bb1226-1 );
#else
bb2389=bb517[bb1226-1 ];bb2189=bb517[bb1226];
#endif
}}bb110((bb2056=bb997[bb2056&bb2548])>bb2654&&--bb2390!=0 );bbm((bb9)bb1226
<=bbg->bb471)bb4(bb9)bb1226;bb4 bbg->bb471;}bb40 bbb bb2264(bbg)bb192
 *bbg;{bb950 bbt bb11,bb82;bb950 bb1397*bb28;bbt bb1984;bb9 bb1238=
bbg->bb958;bb595{bb1984=(bbt)(bbg->bb2297-(bb412)bbg->bb471-(bb412)bbg
->bb191);bbm(bb1984==0 &&bbg->bb191==0 &&bbg->bb471==0 ){bb1984=bb1238;}
bb50 bbm(bb1984==(bbt)(-1 )){bb1984--;}bb50 bbm(bbg->bb191>=bb1238+((
bbg)->bb958-(258 +3 +1 ))){bb75(bbg->bb158,bbg->bb158+bb1238,(bbt)bb1238
);bbg->bb2090-=bb1238;bbg->bb191-=bb1238;bbg->bb443-=(bb8)bb1238;bb11
=bbg->bb1366;bb28=&bbg->bb397[bb11];bb595{bb82= * --bb28; *bb28=(
bb1072)(bb82>=bb1238?bb82-bb1238:0 );}bb110(--bb11);bb11=bb1238;bb28=&
bbg->bb997[bb11];bb595{bb82= * --bb28; *bb28=(bb1072)(bb82>=bb1238?
bb82-bb1238:0 );}bb110(--bb11);bb1984+=bb1238;}bbm(bbg->bb15->bb149==0
)bb4;;bb11=bb2513(bbg->bb15,bbg->bb158+bbg->bb191+bbg->bb471,bb1984);
bbg->bb471+=bb11;bbm(bbg->bb471>=3 ){bbg->bb514=bbg->bb158[bbg->bb191]
;(bbg->bb514=(((bbg->bb514)<<bbg->bb1632)^(bbg->bb158[bbg->bb191+1 ]))&
bbg->bb1756);}}bb110(bbg->bb471<(258 +3 +1 )&&bbg->bb15->bb149!=0 );}bb40
bb1869 bb2460(bbg,bb176)bb192*bbg;bbe bb176;{bb412 bb2343=0xffff ;
bb412 bb2225;bbm(bb2343>bbg->bb2191-5 ){bb2343=bbg->bb2191-5 ;}bb91(;;){
bbm(bbg->bb471<=1 ){;bb2264(bbg);bbm(bbg->bb471==0 &&bb176==0 )bb4 bb1326
;bbm(bbg->bb471==0 )bb21;};bbg->bb191+=bbg->bb471;bbg->bb471=0 ;bb2225=
bbg->bb443+bb2343;bbm(bbg->bb191==0 ||(bb412)bbg->bb191>=bb2225){bbg->
bb471=(bb9)(bbg->bb191-bb2225);bbg->bb191=(bb9)bb2225;{{bb1641(bbg,(
bbg->bb443>=0L ?(bb452* )&bbg->bb158[(bbt)bbg->bb443]:(bb452* )0 ),(
bb412)((bb8)bbg->bb191-bbg->bb443),(0 ));bbg->bb443=bbg->bb191;bb1309(
bbg->bb15);;};bbm(bbg->bb15->bb396==0 )bb4(0 )?bb1794:bb1326;};}bbm(bbg
->bb191-(bb9)bbg->bb443>=((bbg)->bb958-(258 +3 +1 ))){{{bb1641(bbg,(bbg
->bb443>=0L ?(bb452* )&bbg->bb158[(bbt)bbg->bb443]:(bb452* )0 ),(bb412)(
(bb8)bbg->bb191-bbg->bb443),(0 ));bbg->bb443=bbg->bb191;bb1309(bbg->
bb15);;};bbm(bbg->bb15->bb396==0 )bb4(0 )?bb1794:bb1326;};}}{{bb1641(
bbg,(bbg->bb443>=0L ?(bb452* )&bbg->bb158[(bbt)bbg->bb443]:(bb452* )0 ),
(bb412)((bb8)bbg->bb191-bbg->bb443),(bb176==4 ));bbg->bb443=bbg->bb191
;bb1309(bbg->bb15);;};bbm(bbg->bb15->bb396==0 )bb4(bb176==4 )?bb1794:
bb1326;};bb4 bb176==4 ?bb2213:bb2230;}bb40 bb1869 bb2231(bbg,bb176)bb192
 *bbg;bbe bb176;{bb1356 bb1167=0 ;bbe bb1799;bb91(;;){bbm(bbg->bb471<(
258 +3 +1 )){bb2264(bbg);bbm(bbg->bb471<(258 +3 +1 )&&bb176==0 ){bb4 bb1326;
}bbm(bbg->bb471==0 )bb21;}bbm(bbg->bb471>=3 ){((bbg->bb514=(((bbg->
bb514)<<bbg->bb1632)^(bbg->bb158[(bbg->bb191)+(3 -1 )]))&bbg->bb1756),
bbg->bb997[(bbg->bb191)&bbg->bb1850]=bb1167=bbg->bb397[bbg->bb514],
bbg->bb397[bbg->bb514]=(bb1072)(bbg->bb191));}bbm(bb1167!=0 &&bbg->
bb191-bb1167<=((bbg)->bb958-(258 +3 +1 ))){bbm(bbg->bb301!=2 ){bbg->bb994
=bb2328(bbg,bb1167);}}bbm(bbg->bb994>=3 ){;{bb156 bb22=(bbg->bb994-3 );
bb129 bb429=(bbg->bb191-bbg->bb2090);bbg->bb1664[bbg->bb639]=bb429;
bbg->bb1741[bbg->bb639++]=bb22;bb429--;bbg->bb1002[bb2118[bb22]+256 +1
].bb293.bb439++;bbg->bb1695[((bb429)<256 ?bb1764[bb429]:bb1764[256 +((
bb429)>>7 )])].bb293.bb439++;bb1799=(bbg->bb639==bbg->bb1162-1 );};bbg
->bb471-=bbg->bb994;bbm(bbg->bb994<=bbg->bb2123&&bbg->bb471>=3 ){bbg->
bb994--;bb595{bbg->bb191++;((bbg->bb514=(((bbg->bb514)<<bbg->bb1632)^
(bbg->bb158[(bbg->bb191)+(3 -1 )]))&bbg->bb1756),bbg->bb997[(bbg->bb191
)&bbg->bb1850]=bb1167=bbg->bb397[bbg->bb514],bbg->bb397[bbg->bb514]=(
bb1072)(bbg->bb191));}bb110(--bbg->bb994!=0 );bbg->bb191++;}bb50{bbg->
bb191+=bbg->bb994;bbg->bb994=0 ;bbg->bb514=bbg->bb158[bbg->bb191];(bbg
->bb514=(((bbg->bb514)<<bbg->bb1632)^(bbg->bb158[bbg->bb191+1 ]))&bbg
->bb1756);}}bb50{;{bb156 bb1891=(bbg->bb158[bbg->bb191]);bbg->bb1664[
bbg->bb639]=0 ;bbg->bb1741[bbg->bb639++]=bb1891;bbg->bb1002[bb1891].
bb293.bb439++;bb1799=(bbg->bb639==bbg->bb1162-1 );};bbg->bb471--;bbg->
bb191++;}bbm(bb1799){{bb1641(bbg,(bbg->bb443>=0L ?(bb452* )&bbg->bb158
[(bbt)bbg->bb443]:(bb452* )0 ),(bb412)((bb8)bbg->bb191-bbg->bb443),(0 ));
bbg->bb443=bbg->bb191;bb1309(bbg->bb15);;};bbm(bbg->bb15->bb396==0 )bb4
(0 )?bb1794:bb1326;};}{{bb1641(bbg,(bbg->bb443>=0L ?(bb452* )&bbg->
bb158[(bbt)bbg->bb443]:(bb452* )0 ),(bb412)((bb8)bbg->bb191-bbg->bb443
),(bb176==4 ));bbg->bb443=bbg->bb191;bb1309(bbg->bb15);;};bbm(bbg->
bb15->bb396==0 )bb4(bb176==4 )?bb1794:bb1326;};bb4 bb176==4 ?bb2213:
bb2230;}bb40 bb1869 bb2004(bbg,bb176)bb192*bbg;bbe bb176;{bb1356
bb1167=0 ;bbe bb1799;bb91(;;){bbm(bbg->bb471<(258 +3 +1 )){bb2264(bbg);
bbm(bbg->bb471<(258 +3 +1 )&&bb176==0 ){bb4 bb1326;}bbm(bbg->bb471==0 )bb21
;}bbm(bbg->bb471>=3 ){((bbg->bb514=(((bbg->bb514)<<bbg->bb1632)^(bbg->
bb158[(bbg->bb191)+(3 -1 )]))&bbg->bb1756),bbg->bb997[(bbg->bb191)&bbg
->bb1850]=bb1167=bbg->bb397[bbg->bb514],bbg->bb397[bbg->bb514]=(
bb1072)(bbg->bb191));}bbg->bb1317=bbg->bb994,bbg->bb2370=bbg->bb2090;
bbg->bb994=3 -1 ;bbm(bb1167!=0 &&bbg->bb1317<bbg->bb2123&&bbg->bb191-
bb1167<=((bbg)->bb958-(258 +3 +1 ))){bbm(bbg->bb301!=2 ){bbg->bb994=
bb2328(bbg,bb1167);}bbm(bbg->bb994<=5 &&(bbg->bb301==1 ||(bbg->bb994==3
&&bbg->bb191-bbg->bb2090>4096 ))){bbg->bb994=3 -1 ;}}bbm(bbg->bb1317>=3
&&bbg->bb994<=bbg->bb1317){bb9 bb2561=bbg->bb191+bbg->bb471-3 ;;{bb156
bb22=(bbg->bb1317-3 );bb129 bb429=(bbg->bb191-1 -bbg->bb2370);bbg->
bb1664[bbg->bb639]=bb429;bbg->bb1741[bbg->bb639++]=bb22;bb429--;bbg->
bb1002[bb2118[bb22]+256 +1 ].bb293.bb439++;bbg->bb1695[((bb429)<256 ?
bb1764[bb429]:bb1764[256 +((bb429)>>7 )])].bb293.bb439++;bb1799=(bbg->
bb639==bbg->bb1162-1 );};bbg->bb471-=bbg->bb1317-1 ;bbg->bb1317-=2 ;
bb595{bbm(++bbg->bb191<=bb2561){((bbg->bb514=(((bbg->bb514)<<bbg->
bb1632)^(bbg->bb158[(bbg->bb191)+(3 -1 )]))&bbg->bb1756),bbg->bb997[(
bbg->bb191)&bbg->bb1850]=bb1167=bbg->bb397[bbg->bb514],bbg->bb397[bbg
->bb514]=(bb1072)(bbg->bb191));}}bb110(--bbg->bb1317!=0 );bbg->bb1994=
0 ;bbg->bb994=3 -1 ;bbg->bb191++;bbm(bb1799){{bb1641(bbg,(bbg->bb443>=0L
?(bb452* )&bbg->bb158[(bbt)bbg->bb443]:(bb452* )0 ),(bb412)((bb8)bbg->
bb191-bbg->bb443),(0 ));bbg->bb443=bbg->bb191;bb1309(bbg->bb15);;};bbm
(bbg->bb15->bb396==0 )bb4(0 )?bb1794:bb1326;};}bb50 bbm(bbg->bb1994){;{
bb156 bb1891=(bbg->bb158[bbg->bb191-1 ]);bbg->bb1664[bbg->bb639]=0 ;bbg
->bb1741[bbg->bb639++]=bb1891;bbg->bb1002[bb1891].bb293.bb439++;
bb1799=(bbg->bb639==bbg->bb1162-1 );};bbm(bb1799){{bb1641(bbg,(bbg->
bb443>=0L ?(bb452* )&bbg->bb158[(bbt)bbg->bb443]:(bb452* )0 ),(bb412)((
bb8)bbg->bb191-bbg->bb443),(0 ));bbg->bb443=bbg->bb191;bb1309(bbg->
bb15);;};}bbg->bb191++;bbg->bb471--;bbm(bbg->bb15->bb396==0 )bb4 bb1326
;}bb50{bbg->bb1994=1 ;bbg->bb191++;bbg->bb471--;}};bbm(bbg->bb1994){;{
bb156 bb1891=(bbg->bb158[bbg->bb191-1 ]);bbg->bb1664[bbg->bb639]=0 ;bbg
->bb1741[bbg->bb639++]=bb1891;bbg->bb1002[bb1891].bb293.bb439++;
bb1799=(bbg->bb639==bbg->bb1162-1 );};bbg->bb1994=0 ;}{{bb1641(bbg,(bbg
->bb443>=0L ?(bb452* )&bbg->bb158[(bbt)bbg->bb443]:(bb452* )0 ),(bb412)(
(bb8)bbg->bb191-bbg->bb443),(bb176==4 ));bbg->bb443=bbg->bb191;bb1309(
bbg->bb15);;};bbm(bbg->bb15->bb396==0 )bb4(bb176==4 )?bb1794:bb1326;};
bb4 bb176==4 ?bb2213:bb2230;}
