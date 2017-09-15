/*
 * Perform FIPS Integrity test on Kernel Crypto API
 * 
 * At build time, hmac(sha256) of crypto code, avaiable in different ELF sections 
 * of vmlinux file, is generated. vmlinux file is updated with built-time hmac
 * in a read-only data variable, so that it is available at run-time
 * 
 * At run time, hmac(sha256) is again calculated using crypto bytes of a running 
 * kernel. 
 * Run time hmac is compared to built time hmac to verify the integrity.
 *
 *
 * Author : Rohit Kothari (r.kothari@samsung.com) 
 * Date	  : 11 Feb 2014
 *
 * Copyright (c) 2014 Samsung Electronics
 *
 */

#include <linux/crypto.h>
#include <linux/kallsyms.h>
#include <linux/err.h>
#include <linux/scatterlist.h>
#include "internal.h" /* For Functional test macros */

static const char *
symtab[][3] = {{".text",      "first_crypto_text",   "last_crypto_text"  },
	      {".rodata",    "first_crypto_rodata", "last_crypto_rodata"},
	      {".init.text", "first_crypto_init",   "last_crypto_init"  },
	      {".exit.text", "first_crypto_exit",   "last_crypto_exit"  },
	      {"asm.text",      "first_crypto_asm_text",   "last_crypto_asm_text"  },
	      {"asm.rodata",    "first_crypto_asm_rodata", "last_crypto_asm_rodata"},
	      {"asm.init.text", "first_crypto_asm_init",   "last_crypto_asm_init"  },
	      {"asm.exit.text", "first_crypto_asm_exit",   "last_crypto_asm_exit"  }};

extern const char * get_builtime_crypto_hmac(void);

#ifdef FIPS_DEBUG
static int
dump_bytes(const char * section_name, const char * first_symbol, const char * last_symbol)
{
	u8 * start_addr = (u8 *) kallsyms_lookup_name (first_symbol);
	u8 * end_addr   = (u8 *) kallsyms_lookup_name (last_symbol);

	if (!start_addr || !end_addr || start_addr >= end_addr)
	{
		printk(KERN_ERR "FIPS(%s): Error Invalid Addresses in Section : %s, Start_Addr : %p , End_Addr : %p", 
                       __FUNCTION__,section_name, start_addr, end_addr);
		return -1;
	}
	
	printk(KERN_INFO "FIPS CRYPTO RUNTIME : Section - %s, %s : %p, %s : %p \n", section_name, first_symbol, start_addr, last_symbol, end_addr);

	print_hex_dump_bytes ("FIPS CRYPTO RUNTIME : ",DUMP_PREFIX_NONE, start_addr, end_addr - start_addr);

	return 0;
}
#endif


static int
query_symbol_addresses (const char * first_symbol, const char * last_symbol, 
                        unsigned long * start_addr,unsigned long * end_addr)
{
	unsigned long start = kallsyms_lookup_name (first_symbol);
	unsigned long end   = kallsyms_lookup_name (last_symbol);

#ifdef FIPS_DEBUG
	printk(KERN_INFO "FIPS CRYPTO RUNTIME :  %s : %p, %s : %p\n", first_symbol, (u8*)start, last_symbol, (u8*)end);
#endif

	if (!start || !end || start >= end)
	{
		printk(KERN_ERR "FIPS(%s): Error Invalid Addresses.", __FUNCTION__);
		return -1;
	}

	*start_addr = start;
	*end_addr   = end;
	
	return 0;

}

static int 
init_hash (struct hash_desc * desc) 
{
	struct crypto_hash * tfm = NULL;
	int ret = -1;

	/* Same as build time */
	const unsigned char * key = "The quick brown fox jumps over the lazy dog";

	tfm = crypto_alloc_hash ("hmac(sha256)", 0, 0);

	if (IS_ERR(tfm)) {
		printk(KERN_ERR "FIPS(%s): integ failed to allocate tfm %ld", __FUNCTION__, PTR_ERR(tfm));
		return -1;
	}

	ret = crypto_hash_setkey (tfm, key, strlen(key));

	if (ret) {
		printk(KERN_ERR "FIPS(%s): fail at crypto_hash_setkey", __FUNCTION__);		
		return -1;
	}

	desc->tfm   = tfm;
	desc->flags = 0;

	ret = crypto_hash_init (desc);

	if (ret) {
		printk(KERN_ERR "FIPS(%s): fail at crypto_hash_init", __FUNCTION__);		
		return -1;
	}
	
	return 0;
}

static int
finalize_hash (struct hash_desc *desc, unsigned char * out, unsigned int out_size)
{
	int ret = -1;

	if (!desc || !desc->tfm || !out || !out_size)
	{
		printk(KERN_ERR "FIPS(%s): Invalid args", __FUNCTION__);		
		return ret;
	}

	if (crypto_hash_digestsize(desc->tfm) > out_size)
	{
		printk(KERN_ERR "FIPS(%s): Not enough space for digest", __FUNCTION__);		
		return ret;
	}

	ret = crypto_hash_final (desc, out);

	if (ret)
	{
		printk(KERN_ERR "FIPS(%s): crypto_hash_final failed", __FUNCTION__);		
		return -1;
	}

	return 0;
}

static int
update_hash (struct hash_desc * desc, unsigned char * start_addr, unsigned int size)
{
	struct scatterlist sg;
	unsigned char * buf = NULL;
	unsigned char * cur = NULL;
	unsigned int bytes_remaining;
	unsigned int bytes;
	int ret = -1;
#if FIPS_FUNC_TEST == 2
	static int total = 0;
#endif

	buf = kmalloc (PAGE_SIZE, GFP_KERNEL);

	if (!buf)
	{
		printk(KERN_ERR "FIPS(%s): kmalloc failed", __FUNCTION__);		
		return ret;
	}

	bytes_remaining = size;
	cur = start_addr;

	while (bytes_remaining > 0)
	{
		if (bytes_remaining >= PAGE_SIZE)
			bytes = PAGE_SIZE;
		else
			bytes = bytes_remaining;

		memcpy (buf, cur, bytes);

		sg_init_one (&sg, buf, bytes);

#if FIPS_FUNC_TEST == 2
		if (total == 0)
		{
			printk(KERN_INFO "FIPS : Failing Integrity Test");		
			buf[bytes / 2] += 1;
		}
#endif

		ret = crypto_hash_update (desc, &sg, bytes);

		if (ret)
		{
			printk(KERN_ERR "FIPS(%s): crypto_hash_update failed", __FUNCTION__);		
			kfree(buf);
			buf = 0;
			return -1;
		}

		cur += bytes;

		bytes_remaining -= bytes;

#if FIPS_FUNC_TEST == 2
		total += bytes;
#endif
	}

	//printk(KERN_INFO "FIPS : total bytes = %d\n", total);		

	if (buf)
        {
		kfree(buf);
		buf = 0;
        }

	return 0;
}


int
do_integrity_check (void)
{
	int i,rows, err;
	unsigned long start_addr = 0;
	unsigned long end_addr   = 0;
	unsigned char runtime_hmac[32];
	struct hash_desc desc;
	const char * builtime_hmac = 0;
	unsigned int size = 0;

	err = init_hash (&desc);

	if (err)
	{
		printk (KERN_ERR "FIPS(%s): init_hash failed", __FUNCTION__);
		return -1;
	}

	rows = (unsigned int) sizeof (symtab) / sizeof (symtab[0]);

	for (i = 0; i < rows; i++)
	{
		err = query_symbol_addresses (symtab[i][1], symtab[i][2], &start_addr, &end_addr);

		if (err)
		{
			printk (KERN_ERR "FIPS(%s): Error to get start / end addresses", __FUNCTION__);
			crypto_free_hash (desc.tfm);
			return -1;
		}

#ifdef FIPS_DEBUG
		dump_bytes(symtab[i][0],  symtab[i][1], symtab[i][2]);
#endif

		size = end_addr - start_addr;

		err = update_hash (&desc, (unsigned char *)start_addr, size);	

		if (err)
		{
			printk (KERN_ERR "FIPS(%s): Error to update hash", __FUNCTION__);
			crypto_free_hash (desc.tfm);
			return -1;
		}
	}

	err = finalize_hash (&desc, runtime_hmac, sizeof(runtime_hmac));

	crypto_free_hash (desc.tfm);

	if (err)
	{
		printk (KERN_ERR "FIPS(%s): Error in finalize", __FUNCTION__);
		return -1;
	}


	builtime_hmac =  get_builtime_crypto_hmac();

	if (!builtime_hmac)
	{
		printk (KERN_ERR "FIPS(%s): Unable to retrieve builtime_hmac", __FUNCTION__);
		return -1;
	}

#ifdef FIPS_DEBUG
	print_hex_dump_bytes ("FIPS CRYPTO RUNTIME : runtime hmac  = ",DUMP_PREFIX_NONE, runtime_hmac, sizeof(runtime_hmac));
	print_hex_dump_bytes ("FIPS CRYPTO RUNTIME : builtime_hmac = ",DUMP_PREFIX_NONE, builtime_hmac , sizeof(runtime_hmac));
#endif

	if (!memcmp (builtime_hmac, runtime_hmac, sizeof(runtime_hmac))) 
	{
		printk (KERN_INFO "FIPS: Integrity Check Passed");
		return 0;
	}
	else
	{
		printk (KERN_ERR "FIPS(%s): Integrity Check Failed", __FUNCTION__);
		set_in_fips_err();
		return -1;
	}

	return -1;
}

EXPORT_SYMBOL_GPL(do_integrity_check);

#ifdef CONFIG_CRYPTO_FIPS_OLD_INTEGRITY_CHECK
/*
 * Integrity check code for crypto module.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 */
#include <crypto/hash.h>
#include <crypto/sha.h>
#include <linux/err.h>
#include <linux/scatterlist.h>
#include <asm-generic/sections.h>
#include <linux/vmalloc.h>
#include <linux/slab.h>

#include "internal.h"

static bool need_integrity_check = true;
extern long integrity_mem_reservoir;
//extern void free_bootmem(unsigned long addr, unsigned long size);

void do_integrity_check(void)
{
	u8 *rbuf = 0;
	u32 len;
	u8 hmac[SHA256_DIGEST_SIZE];
	struct hash_desc desc;
	struct scatterlist sg;
	u8 *key = "12345678";
    int i, step_len = PAGE_SIZE, err;
    u8 *pAllocBuf = 0;

    printk(KERN_INFO "FIPS: integrity start\n");
	
	if (unlikely(!need_integrity_check || in_fips_err())) {
        printk(KERN_INFO "FIPS: integrity check not needed\n");
		return;
	}
	rbuf = (u8*)phys_to_virt((unsigned long)CONFIG_CRYPTO_FIPS_INTEG_COPY_ADDRESS);
	
	if (*((u32 *) &rbuf[36]) != 0x016F2818) {
		printk(KERN_ERR "FIPS: invalid zImage magic number.");
		set_in_fips_err();
		goto err1;
	}

	if (*(u32 *) &rbuf[44] <= *(u32 *) &rbuf[40]) {
		printk(KERN_ERR "FIPS: invalid zImage calculated len");
		set_in_fips_err();
		goto err1;
	}

	len = *(u32 *) &rbuf[44] - *(u32 *) &rbuf[40];

    printk(KERN_INFO "FIPS: integrity actual zImageLen = %d\n", len);
    printk(KERN_INFO "FIPS: do kernel integrity check address: %lx \n", (unsigned long)rbuf);
	
	desc.tfm = crypto_alloc_hash("hmac(sha256)", 0, 0);

	if (IS_ERR(desc.tfm)) {
		printk(KERN_ERR "FIPS: integ failed to allocate tfm %ld\n",
		       PTR_ERR(desc.tfm));
		set_in_fips_err();
		goto err1;
	}
#if FIPS_FUNC_TEST == 2
    rbuf[1024] = rbuf[1024] + 1;
#endif
	crypto_hash_setkey(desc.tfm, key, strlen(key));

	pAllocBuf = kmalloc(step_len,GFP_KERNEL);
	if (!pAllocBuf) {
		printk(KERN_INFO "Fail to alloc memory, length %d\n", step_len);
		set_in_fips_err();
		goto err1;
	}

	err = crypto_hash_init(&desc);
	if (err) {
		printk(KERN_INFO "fail at crypto_hash_init\n");		
        set_in_fips_err();
		kfree(pAllocBuf);	
		goto err1;
	}

	for (i = 0; i < len; i += step_len) 	{

		//last is reached
		if (i + step_len >= len - 1)  {
			memcpy(pAllocBuf, &rbuf[i], len - i);
			sg_init_one(&sg, pAllocBuf, len - i);
			err = crypto_hash_update(&desc, &sg, len - i);
			if (err) {
				printk(KERN_INFO "Fail to crypto_hash_update1\n");
                set_in_fips_err();
				goto err;
			}
			err = crypto_hash_final(&desc, hmac);
			if (err) {
				printk(KERN_INFO "Fail to crypto_hash_final\n");
                set_in_fips_err();
				goto err;
			}
		} else {						
		    memcpy(pAllocBuf, &rbuf[i], step_len);
		    sg_init_one(&sg, pAllocBuf, step_len);
		    err = crypto_hash_update(&desc, &sg, step_len); 
	
		    if (err) {
			    printk(KERN_INFO "Fail to crypto_hash_update\n");
                set_in_fips_err();
			    goto err;
		    }
        }
	}
#if FIPS_FUNC_TEST == 2
    rbuf[1024] = rbuf[1024] - 1;
#endif
	if (!strncmp(hmac, &rbuf[len], SHA256_DIGEST_SIZE)) {
		printk(KERN_INFO "FIPS: integrity check passed\n");
	} else {
		printk(KERN_ERR "FIPS: integrity check failed. hmac:%lx, buf:%lx.\n",(long) hmac, (long)rbuf[len] );
		set_in_fips_err();
	}

 err:
	kfree(pAllocBuf);
	crypto_free_hash(desc.tfm);
 err1:
	need_integrity_check = false;
	
/*	if(integrity_mem_reservoir != 0) {
		printk(KERN_NOTICE "FIPS free integrity_mem_reservoir = %ld\n", integrity_mem_reservoir);
		free_bootmem((unsigned long)CONFIG_CRYPTO_FIPS_INTEG_COPY_ADDRESS, integrity_mem_reservoir);
	}
*/	
}

EXPORT_SYMBOL_GPL(do_integrity_check);
#endif //CONFIG_CRYPTO_FIPS_OLD_INTEGRITY_CHECK
