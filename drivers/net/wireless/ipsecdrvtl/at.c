/*
   'src_pm_pgpNetPMConfig.c' Obfuscated by COBF (Version 1.06 2006-01-07 by BB) at Fri Nov 13 10:03:51 2015
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
bbb bb1014(bby*bb53);bbu bb1049(bbf*bb53);bba bbj bb1025*bb1023;bba
bbj bb1064*bb1050;bba bbj bb1027*bb1039;bba bbj bb1069*bb1047;bba bbj
bb1048*bb1035;bba bbj bb1024*bb1063;bba bb13{bb579=0 ,bb604=1 ,bb610=2 ,
bb804=3 ,bb611=4 ,bb603=5 ,bb596=6 ,bb591=7 ,bb605=9 ,}bb436;bba bb13{bb632
=0 ,bb1026,bb626,bb1045,bb955,bb935,bb940,bb946,bb952,bb936,bb944,}
bb539;
#pragma pack(push, 8)
#ifdef _MSC_VER
#pragma warning (disable:4200)
#endif
bba bbf bb180[4 ];bba bb13{bb1668=0 ,bb1490=1 ,}bb1414;bba bb13{bb1549=0
,bb1738=1 ,bb1685=2 ,bb1455=3 ,bb1675=4 ,bb1516=5 ,bb1585=6 ,bb1540=7 ,
bb1630=8 ,bb1544=9 ,bb1464=10 ,bb1531=11 ,bb1542=12 ,bb1730=13 ,bb1731=14 ,
bb1446=15 ,bb1477=16 ,bb1422=17 ,bb1626=18 ,bb1704=19 ,bb1663=20 ,bb1520=21
,bb1529=22 ,bb1498=23 ,bb1629=24 ,bb1624=25 ,bb1473=26 ,bb1605=27 ,bb1749=
28 ,bb1597=29 ,bb1707=30 ,bb1654=16300 ,bb1631=16301 ,bb1743=16384 ,bb1562=
24576 ,bb1484=24577 ,bb1461=24578 ,bb1502=34793 ,bb1759=40500 ,}bb779;bba
bb13{bb1485=0 ,bb1548=1 ,bb1479=2 ,bb1448=3 ,bb1717=4 ,bb1413=5 ,bb1687=6 ,
bb1499=7 ,bb1555=8 ,bb1423=9 ,bb1463=21 ,bb1513=22 ,bb1538=23 ,bb1466=24 ,
bb1567=25 ,bb1530=26 ,bb1486=27 ,bb1757=28 ,bb1497=29 ,bb1510=80 ,}bb816;
bba bb13{bb1657=0 ,bb1714=1 ,bb1711=2 ,bb1507=3 ,bb1543=4 ,}bb1648;bba bb13
{bb1706=0 ,bb1382=1 ,bb1204=2 ,bb1260=3 ,bb1331=4 ,bb1085=61440 ,bb1400=
61441 ,bb1152=61443 ,bb1341=61444 ,}bb502;bba bb13{bb1718=0 ,bb1518=1 ,
bb1584=2 ,}bb1698;bba bb13{bb1416=0 ,bb1742,bb1453,bb1474,bb1590,bb1519
,bb1658,bb1489,bb1573,bb1512,bb1415,bb1713,}bb801;bba bb13{bb1465=0 ,
bb1403=2 ,bb1367=3 ,bb1753=4 ,bb1362=9 ,bb1340=12 ,bb1355=13 ,bb1357=14 ,
bb1391=249 ,}bb927;bba bb13{bb1359=0 ,bb1342=1 ,bb1393=2 ,bb1445=3 ,bb1651
=4 ,bb1399=5 ,bb1385=12 ,bb1361=13 ,bb1353=14 ,bb1408=61440 ,}bb500;bba bb13
{bb1363=1 ,bb1347=2 ,bb1380=3 ,bb1564=4 ,bb1628=5 ,bb1469=6 ,bb1451=7 ,
bb1493=8 ,bb1472=9 ,bb1563=10 ,bb1345=11 ,bb404=12 ,bb1381=13 ,bb399=240 ,
bb1388=(128 <<16 )|bb399,bb1386=(192 <<16 )|bb399,bb1374=(256 <<16 )|bb399,
bb1344=(128 <<16 )|bb404,bb1336=(192 <<16 )|bb404,bb1372=(256 <<16 )|bb404,
}bb900;bba bb13{bb1335=0 ,bb1525=1 ,bb1405=2 ,bb1370=3 ,bb1482=4 ,}bb896;
bba bb13{bb1459=0 ,bb1599=1 ,bb1227=2 ,bb623=3 ,bb1281=4 ,}bb833;bba bb13{
bb1601=0 ,bb1552=1 ,bb1425=2 ,bb1686=5 ,bb1723=7 ,}bb503;bba bb13{bb1449=0
,bb1539=1 ,bb1627=2 ,bb1602=3 ,bb1496=4 ,bb1702=5 ,bb1661=6 ,bb409=7 ,bb1571
=65001 ,bb402=240 ,bb1509=(128 <<16 )|bb402,bb1526=(192 <<16 )|bb402,bb1534
=(256 <<16 )|bb402,bb1570=(128 <<16 )|bb409,bb1581=(192 <<16 )|bb409,bb1638
=(256 <<16 )|bb409,}bb697;bba bb13{bb1491=0 ,bb1480=1 ,bb1678=2 ,bb1598=3 ,
bb1495=4 ,bb1551=5 ,bb1591=6 ,bb1665=65001 ,}bb867;bba bb13{bb1701=0 ,
bb1550=1 ,bb1677=2 ,bb1577=3 ,bb1674=4 ,bb1637=5 ,bb1579=64221 ,bb1642=
64222 ,bb1676=64223 ,bb1690=64224 ,bb1728=65001 ,bb1700=65002 ,bb1574=
65003 ,bb1462=65004 ,bb1739=65005 ,bb1511=65006 ,bb1535=65007 ,bb1501=
65008 ,bb1721=65009 ,bb1500=65010 ,}bb904;bba bb13{bb1712=0 ,bb1439=1 ,
bb1456=2 ,}bb901;bba bb13{bb1434=0 ,bb1750=1 ,bb1503=2 ,bb1703=3 ,}bb813;
bba bb13{bb1618=0 ,bb1444=1 ,bb1458=2 ,bb1669=3 ,bb1623=4 ,bb1662=5 ,bb1515
=21 ,bb1594=6 ,bb1640=7 ,bb1558=8 ,bb1752=1000 ,}bb490;bba bb13{bb1435=0 ,
bb1588=1 ,bb1683=2 ,}bb746;bba bb13{bb1608=0 ,bb1426=1 ,bb1729=2 ,bb1460=3
,bb1494=4 ,}bb689;bba bb13{bb1557=0 ,bb1692=1 ,bb1419=1001 ,bb1733=1002 ,}
bb820;bba bb13{bb1583=0 ,bb1170=1 ,bb1105=2 ,bb1100=3 ,bb1169=4 ,bb1180=5 ,
bb1129=6 ,bb1715=100 ,bb1604=101 ,}bb492;bba bbj bb408{bb900 bb299;bb500
bb621;bb502 bb45;}bb408;bba bbj bb401{bb927 bb1387;bb500 bb621;bb502
bb45;}bb401;bba bbj bb405{bb896 bb1046;}bb405;bba bbj bb498{bb904
bb1652;bb867 bb430;bb697 bb299;bbu bb1508;bb503 bb899;}bb498;bba bbj
bb489{bbu bb630;bb408 bb313;bbu bb776;bb401 bb578;bbu bb735;bb405
bb635;bb503 bb899;}bb489;bba bbj bb464{bb180 bb985;bb180 bb1245;bb833
bb104;bb332{bbj{bb401 bb47;bbf bb580[64 ];bbf bb570[64 ];}bb578;bbj{
bb408 bb47;bbf bb1253[32 ];bbf bb1266[32 ];bbf bb580[64 ];bbf bb570[64 ];
bbf bb1233[16 ];}bb313;bbj{bb405 bb47;}bb635;}bb298;}bb464;bba bbj{bbd
bb736,bb607;bbf bb1175:1 ;bbf bb1193:1 ;bbf bb104;bbk bb455;}bb190;bba
bbj bb533{bbd bb11;bb190 bbc[64 *2 ];}bb533;
#ifdef UNDER_CE
bba bb43 bb389;
#else
bba bb85 bb389;
#endif
bba bbj bb174{bbj bb174*bb1488, *bb1761;bbd bb29;bbd bb1150;bb190
bb943[64 ];bb492 bb530;bbd bb1396;bbk bb1101;bbd bb572;bbd bb918;bbd
bb821;bbf bb495;bbf bb1376;bbf bb1140;bbd bb1067;bbd bb1760;bb389
bb597;bbk bb1295;bb464 bb425[3 ];bb389 bb1592;bbf bb1528[40 ];bbd bb608
;bbd bb1603;}bb174;
#ifdef CONFIG_COMPAT
#include"uncobf.h"
#include<linux/compat.h>
#include"cobf.h"
#define bb1371 ( bb12( bbj bb174  *  )  *  2 - bb12( bb497)  *  2)
#endif
bba bbj bb406{bbj bb406*bb1737;bb190 bb506;}bb406;bba bbj bb756{bbu
bb507;bbu bb495;bbd bb29;bbd bb608;bbf bb1536;bbk bb1621;bbf*bb1566;
bbd bb1443;bbf*bb1522;bbd bb1735;bbf*bb1755;bbd bb1433;bbu bb1666;bbu
bb1596;bb406*bb132;bbu bb1546;bb689 bb1547;bbd bb1620;bb901 bb1725;
bb492 bb530;bbk bb1746;bbd bb1554;bb820 bb1424;bbd bb1670;bbd bb1645;
bb801 bb1440;bbf*bb1427;bbd bb1430;bb490 bb826;bbd bb1673;bbd bb1646;
bbd bb1428;bbd bb1719;bbd bb1517;bb498*bb1553;bbd bb1634;bb489*bb1532
;bbd bb1420;bbd bb1556;bbd bb1671;}bb756;bba bbj bb669{bbu bb507;bbd
bb29;bb190 bb506;}bb669;bba bbj bb887{bb174*bb321;bbu bb1593;bbf*
bb1716;bbd bb1727;}bb887;bba bbj bb707{bbd bb29;bb190 bb506;bbf bb1457
;bbf bb1470;}bb707;bba bbj bb852{bbu bb507;bbu bb1155;bbd bb29;bbf*
bb1649;bbd bb1568;}bb852;bba bbj bb671{bbd bb29;bbk bb1745;bbk bb1747
;bbd bb152;bbf*bb52;}bb671;bba bbj bb839{bbu bb1613;bbd bb29;bbd bb572
;bbd bb918;bbd bb821;}bb839;bba bbj bb710{bb779 bb1514;bbd bb29;bb816
bb1364;bbu bb1580;}bb710;bba bbj bb861{bbf bb1506;bbf bb1417;bbf
bb1619;bbf bb1612;bbf bb1696;bbf bb1617;bbf bb938;bbf bb1481;bbf
bb1748;bbf bb1545;bbf bb1438;bbf bb983;bbf bb992;bbf bb993;bbf bb1694
;bbf bb960;bbf bb982;bbf bb1763;bbf bb1471;bbf bb524;bbf bb1575;bbf
bb1679;bbf bb1559;bbf bb1709;bbf bb1437;bbf bb1454;bbf bb1441;}bb861;
bba bbj bb752{bbu bb1684;bbd bb501;bbd bb1751;bb813 bb1450;bbk bb1656
;bbu bb1537;bbu bb1586;bbu bb1672;bbu bb1475;bbu bb1655;bbu bb1682;
bbu bb1418;bbl bb1647[128 ];bbl bb1689[128 ];bbl bb1615[128 ];bbl bb1691
[256 ];bbl bb1659[128 ];bbl bb1468[128 ];bbd bb1614;bbf bb1587[8 ];bbf
bb1432[8 ];}bb752;bba bbj bb930{bbd bb29;bbd bb1412;}bb930;bba bbj
bb885{bbd bb29;bbu bb495;}bb885;bba bbj bb785{bbu bb1732;bbd bb534;
bbd bb1215;}bb785;bba bbj bb766{bbd bb29;bb490 bb826;bb746 bb1625;bbf
 *bb1565;bbd bb1616;}bb766;bba bb13{bb1421=0 ,bb1578,bb1688,bb1411,
bb1639,bb1560,bb1622,bb1758,bb1561,bb1607,bb1610,bb1708,bb1722,bb1478
,bb1429,bb1611,bb1487,bb1431,bb1644,bb1527,}bb661;bba bbj bb1667 bb859
;bba bb7( *bb1576)(bb859*bb1754,bbb*bb1726,bb661 bb327,bbb*bb76);
#pragma pack(pop)
#ifdef _WIN32
#ifdef UNDER_CE
#define bb505 bb1720 bb641("1:")
#else
#define bb505 bb641("\\\\.\\IPSecTL")
#endif
#else
#define bb627 "ipsecdrvtl"
#define bb505 "/dev/" bb627
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
#include"uncobf.h"
#include<linux/ioctl.h>
#include"cobf.h"
bba bbj{bb3 bb599;bbd bb586;bb3 bb583;bbd bb537;bbd bb371;}bb1196;
#ifdef CONFIG_COMPAT
#include"uncobf.h"
#include<linux/compat.h>
#include"cobf.h"
bba bbj{bb497 bb599;bbd bb586;bb497 bb583;bbd bb537;bbd bb371;}bb1291
;
#endif
#define bb1324 1
#endif
#pragma pack(push, 8)
bb13{bb1368=3 ,bb1395,bb1398,bb1442,};bba bbj{bbf bb106[4 ];}bb1289;bba
bbj{bbf bb106[4 ];}bb1280;bba bbj{bbd bb972;bbd bb29;}bb1306;bba bbj{
bbd bb131;bbf bb1262[8 ];}bb418;bba bb13{bb1239=0 ,bb1277,bb1294,bb1302
,bb1744}bb1276;bba bbj{bbf bb1158;bbd bb1106;bbf bb1402;}bb504;
#pragma pack(pop)
#pragma pack(push, 8)
bb13{bb1164=-5000 ,bb1141=-4000 ,bb1033=-4999 ,bb1062=-4998 ,bb1051=-4997
,bb1007=-4996 ,bb1184=-4995 ,bb1115=-4994 ,bb1139=-4993 ,bb1060=-4992 ,
bb1126=-4991 };bb7 bb1165(bb7 bb1168,bbd bb1151,bbl*bb1136);bba bbj{
bb174 bb182;bbd bb1231;bbd bb1116;bbd bb1407;bbd bb1113;bbd bb1283;
bbd bb1322;bbd bb1318;bbd bb1284;bbd bb1325;bbd bb1274;bbd bb1293;bbu
bb1259;bb43 bb597,bb1194,bb1208;bbf bb377[6 ];}bb161;bba bbj bb493{bbj
bb493*bb98;bbf bb104;bbk bb1319;bbk bb1320;bbk bb1315;bbk bb1316;}
bb442;bba bbj bb789{bbj bb789*bb98;bbj bb493*bb1176;bbd bb29;bbf bb377
[6 ];}bb417;bba bb13{bb1183=0 ,bb1600,bb1108,bb1036,bb1053}bb222;bba bbj
{bbd bb397;bbd bb371;bbd bb526;bb418*bb949;bb95 bb1013;}bb307;bba bbj
{bb504*bb475;bb417*bb1160;bbd bb614;bb442*bb553;bb95 bb629;bbn bb1154
;bbn bb567;bb161*bb528;bbu bb1310;bbk bb1195;bbk bb1143;bb307 bb1078;
}bb35, *bb1633;
#pragma pack(pop)
bba bbj bb998 bb1406, *bb83;bba bbj bb840{bbj bb840*bb326;bb3 bb482;
bbn bb590;bbd bb29;bbk bb455;bbn bb96;bb3 bb317;bbn bb473;bb3 bb555;
bbn bb566;bb3 bb1521;bb103 bb1404;bbf bb1346[6 ];bb103 bb1020;bb103
bb1174;bb103 bb538;bb103 bb550;}bb175, *bb88;bba bbj bb815{bbj bb815*
bb98;bb175*bb326;bbd bb29;bbk bb560;bbk bb1492;bbn bb1467;bbn bb1609;
bbk bb1452;}bb1476, *bb478;bbu bb1311(bb35* *bb1257);bbb bb1328(bb35*
bbi);bb222 bb1312(bb35*bb120,bb391 bb467,bb324 bb140,bb362 bb427,
bb319 bb201);bb222 bb1290(bb35*bb120,bb391 bb467,bb324 bb140,bb362
bb427,bb319 bb201);bb222 bb1299(bb35*bb120,bb175*bb52,bb83 bb78);
bb222 bb1279(bb35*bb120,bb175*bb52,bb83 bb78);bb7 bb1288(bb35*bb120,
bb175*bb52,bbd*bb106);bb7 bb1191(bb83 bb78,bb35*bb120,bb175*bb52,
bb161*bb321,bbu bb619,bbu bb971);bb7 bb1815(bb35*bbi,bb504*bb475);bbb
bb2092(bb35*bbi);bb7 bb1815(bb35*bbi,bb504*bb475){bbm(!bbi->bb475){
bbi->bb475=bb128(bb12( *bb475));bbm(!bbi->bb475)bb4 bb363;}bb75(bbi->
bb475,bb475,bb12( *bb475));bb4 bb101;}bbb bb2092(bb35*bbi){bbm(bbi->
bb475)bb109(bbi->bb475);bbi->bb475=bb93;}
