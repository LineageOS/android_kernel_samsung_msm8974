/*
   'aes_xcbc.c' Obfuscated by COBF (Version 1.06 2006-01-07 by BB) at Fri Nov 13 10:03:51 2015
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
bbb bb2109(bb631*bbi,bbh bbb*bb30,bbn bb100){bb204 bb2222;bbf bb1390[
16 ];bbi->bb5=0 ;bb27(bb100==16 );bb1073(&bb2222,bb30,bb100);bb961(bbi->
bb1894,0 ,16 );bb961(bb1390,1 ,16 );bb1022(&bb2222,bb1390,bb1390);bb961(
bbi->bb1256,2 ,16 );bb1022(&bb2222,bbi->bb1256,bbi->bb1256);bb961(bbi->
bb1983,3 ,16 );bb1022(&bb2222,bbi->bb1983,bbi->bb1983);bb1073(&bbi->
bb1156,bb1390,bb100);}bb40 bbb bb1298(bb631*bbi,bbh bbf*bbx){bbn bbz;
bb91(bbz=0 ;bbz<16 ;bbz++)bbi->bb1894[bbz]^=bbx[bbz];bb1022(&bbi->
bb1156,bbi->bb1894,bbi->bb1894);}bbb bb2162(bb631*bbi,bbh bbb*bb509,
bbn bb5){bbh bbf*bbx=(bbh bbf* )bb509;bbn bb398=bbi->bb5?(bbi->bb5-1 )%
16 +1 :0 ;bbi->bb5+=bb5;bbm(bb398){bbn bb11=16 -bb398;bb75(bbi->bb105+
bb398,bbx,((bb5)<(bb11)?(bb5):(bb11)));bbm(bb5<=bb11)bb4;bbx+=bb11;
bb5-=bb11;bb1298(bbi,bbi->bb105);}bb91(;bb5>16 ;bb5-=16 ,bbx+=16 )bb1298
(bbi,bbx);bb75(bbi->bb105,bbx,bb5);}bbb bb2170(bb631*bbi,bbb*bb1){bb3
bb6;bbn bbz,bb398=bbi->bb5?(bbi->bb5-1 )%16 +1 :0 ;bbm(bb398<16 ){bbi->
bb105[bb398++]=0x80 ;bb961(bbi->bb105+bb398,0 ,16 -bb398);bb6=bbi->
bb1983;}bb50 bb6=bbi->bb1256;bb91(bbz=0 ;bbz<16 ;bbz++)bbi->bb105[bbz]
^=bb6[bbz];bb1298(bbi,bbi->bb105);bb75(bb1,bbi->bb1894,16 );}
