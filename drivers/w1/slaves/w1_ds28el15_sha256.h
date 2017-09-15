/*
 * 1-Wire SHA256 software implementation for the ds23el15 chip
 *
 * Copyright (C) 2013 maximintergrated
 *
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */


#ifndef _W1_DS28EL15_SHA256_H
#define _W1_DS28EL15_SHA256_H



#ifndef uchar
   typedef unsigned char uchar;
#endif

#ifndef ushort
   typedef unsigned short ushort;
#endif

#ifndef ulong
   typedef unsigned long ulong;
#endif


int compute_sha256(uchar* message, short length, ushort skipconst, ushort reverse, uchar* digest);
int verify_mac256(uchar* MT, short lenght, uchar* compare_MAC);
int compute_mac256(uchar* message, short length, uchar* MAC);
int calculate_nextsecret256(uchar* binding, uchar* partial, int page_num, uchar* man_id);
void set_secret(uchar *secret_data);
void set_romid(uchar *romid_data);


#endif
