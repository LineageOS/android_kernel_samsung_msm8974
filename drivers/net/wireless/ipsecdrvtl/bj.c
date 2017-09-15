/*
   'src_compress_deflate_zutil.c' Obfuscated by COBF (Version 1.06 2006-01-07 by BB) at Fri Nov 13 10:03:51 2015
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
bb939));bbh bbl*bb1097[10 ]={"\x6e\x65\x65\x64\x20\x64\x69\x63\x74\x69"
"\x6f\x6e\x61\x72\x79","\x73\x74\x72\x65\x61\x6d\x20\x65\x6e\x64","",""
"\x66\x69\x6c\x65\x20\x65\x72\x72\x6f\x72","\x73\x74\x72\x65\x61\x6d"
"\x20\x65\x72\x72\x6f\x72","\x64\x61\x74\x61\x20\x65\x72\x72\x6f\x72"
,"\x69\x6e\x73\x75\x66\x66\x69\x63\x69\x65\x6e\x74\x20\x6d\x65\x6d"
"\x6f\x72\x79","\x62\x75\x66\x66\x65\x72\x20\x65\x72\x72\x6f\x72",""
"\x69\x6e\x63\x6f\x6d\x70\x61\x74\x69\x62\x6c\x65\x20\x76\x65\x72\x73"
"\x69\x6f\x6e",""};bbj bb392{bbe bb463;};
#ifndef bb139
bbr bbb bb2721 bbq((bbe));
#endif
bbh bbl*bb1197(){bb4"\x31\x2e\x31\x2e\x33";}bbh bbl*bb1210(bb18)bbe
bb18;{bb4 bb1097[2 -(bb18)];}
#ifdef __TURBOC__
#if ( defined( __BORLANDC__) || ! defined( bb1572)) && ! defined(  \
bb411)
#define bb2391
#define bb2479 10
bb40 bbe bb2081=0 ;bba bbj bb2725{bb72 bb2452;bb72 bb2480;}bb2582;bb40
bb2582 bb1058[bb2479];bb72 bb1211(bb72 bb122,bbt bb512,bbt bb48){bb72
bb42=bb122;bb412 bb2335=(bb412)bb512*bb48;bbm(bb2335<65520L ){bb42=
bb1384(bb2335);bbm( * (bb129* )&bb42!=0 )bb4 bb42;}bb50{bb42=bb1384(
bb2335+16L );}bbm(bb42==bb93||bb2081>=bb2479)bb4 bb93;bb1058[bb2081].
bb2452=bb42; * ((bb129* )&bb42+1 )+=((bb129)((bb156* )bb42-0 )+15 )>>4 ; *
(bb129* )&bb42=0 ;bb1058[bb2081++].bb2480=bb42;bb4 bb42;}bbb bb1209(
bb72 bb122,bb72 bb939){bbe bb11;bbm( * (bb129* )&bb939!=0 ){bb1378(
bb939);bb4;}bb91(bb11=0 ;bb11<bb2081;bb11++){bbm(bb939!=bb1058[bb11].
bb2480)bb1699;bb1378(bb1058[bb11].bb2452);bb110(++bb11<bb2081){bb1058
[bb11-1 ]=bb1058[bb11];}bb2081--;bb4;}bb939=bb122;;}
#endif
#endif
#if defined( bb2717) && ! defined( bb411)
#define bb2391
#if (! defined( _MSC_VER) || ( _MSC_VER <= 600))
#define bb2531 bb2686
#define bb2631 bb2715
#endif
bb72 bb1211(bb72 bb122,bbt bb512,bbt bb48){bbm(bb122)bb122=0 ;bb4
bb2531((bb8)bb512,bb48);}bbb bb1209(bb72 bb122,bb72 bb939){bbm(bb122)bb122
=0 ;bb2631(bb939);}
#endif
#ifndef bb2391
#ifndef bb139
bbr bb189 bb138 bbq((bb9 bb512,bb9 bb48));bbr bbb bb109 bbq((bb72
bb939));
#endif
bb72 bb1211(bb122,bb512,bb48)bb72 bb122;bbt bb512;bbt bb48;{(bbb)bb122
;(bbb)bb512;(bbb)bb48;;bb4 bb93;}bbb bb1209(bb122,bb939)bb72 bb122;
bb72 bb939;{(bbb)bb122;(bbb)bb939;;bb4;}
#endif
