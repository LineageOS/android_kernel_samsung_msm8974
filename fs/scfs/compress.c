/*
 * This file is part of SCFS from UBIFS.
 *
 * Copyright (C) 2006-2008 Nokia Corporation.
 * Copyright (C) 2006, 2007 University of Szeged, Hungary
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 51
 * Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 *
 * Authors: Adrian Hunter
 *          Artem Bityutskiy (Битюцкий Артём)
 *          Zoltan Sogor
 */

/*
 * This file provides a single place to access to compression and
 * decompression.
 */

#include <linux/crypto.h>
#include "scfs.h"

/* Fake description object for the "none" compressor */
static struct scfs_compressor none_compr = {
	.compr_type = SCFS_COMP_NONE,
	.name = "none",
//	.capi_name = "",
};

static DEFINE_MUTEX(lzo_mutex);

static struct scfs_compressor lzo_compr = {
	.compr_type = SCFS_COMP_LZO,
	.comp_mutex = &lzo_mutex,
	.name = "lzo",
	.capi_name = "lzo",
};

static DEFINE_MUTEX(deflate_mutex);
static DEFINE_MUTEX(inflate_mutex);

static struct scfs_compressor zlib_compr = {
	.compr_type = SCFS_COMP_ZLIB,
	.comp_mutex = &deflate_mutex,
	.decomp_mutex = &inflate_mutex,
	.name = "zlib",
	.capi_name = "deflate",
};

/* All SCFS compressors */
struct scfs_compressor *scfs_compressors[SCFS_COMP_TOTAL_TYPES];

int scfs_compress_crypto(const void *in_buf, size_t in_len, void *out_buf, size_t *out_len,
		    int compr_type)
{
	int err = 0;
	struct scfs_compressor *compr = scfs_compressors[compr_type];
	unsigned int tmp_len;

	if (compr_type == SCFS_COMP_NONE)
		goto no_compr;

	if (compr->comp_mutex)
		mutex_lock(compr->comp_mutex);
	tmp_len = (unsigned int)*out_len;
	err = crypto_comp_compress(compr->cc, in_buf, in_len, out_buf,
				   &tmp_len);
	*out_len = (size_t)tmp_len;
	if (compr->comp_mutex)
		mutex_unlock(compr->comp_mutex);
	if (unlikely(err)) {
		SCFS_PRINT_ERROR("cannot compress %d bytes, compressor %s, "
			   "error %d, leave data uncompressed",
			   in_len, compr->name, err);
		err = 0;
		goto no_compr;
	}

	return err;

no_compr:
	memcpy(out_buf, in_buf, in_len);
	*out_len = in_len;
//	*compr_type = SCFS_COMP_NONE;
	return err;
}

int scfs_decompress_crypto(const void *in_buf, size_t in_len, void *out_buf,
		     size_t *out_len, int compr_type)
{
	int err;
	struct scfs_compressor *compr;
	unsigned int tmp_len;

	if (unlikely(compr_type < 0 || compr_type >= SCFS_COMP_TOTAL_TYPES)) {
		SCFS_PRINT_ERROR("invalid compression type %d", compr_type);
		return -EINVAL;
	}

	compr = scfs_compressors[compr_type];

	if (unlikely(!compr->capi_name)) {
		SCFS_PRINT_ERROR("%s compression is not compiled in", compr->name);
		return -EINVAL;
	}

	if (compr_type == SCFS_COMP_NONE) {
		memcpy(out_buf, in_buf, in_len);
		*out_len = in_len;
		return 0;
	}

	if (compr->decomp_mutex)
		mutex_lock(compr->decomp_mutex);
	tmp_len = (unsigned int)*out_len;
	err = crypto_comp_decompress(compr->cc, in_buf, in_len, out_buf,
				     &tmp_len);
	*out_len = (size_t)tmp_len;
	if (compr->decomp_mutex)
		mutex_unlock(compr->decomp_mutex);
	if (err)
		SCFS_PRINT_ERROR("cannot decompress %d bytes, compressor %s, "
			  "error %d", in_len, compr->name, err);

	return err;
}

/**
 * compr_init - initialize a compressor.
 * @compr: compressor description object
 *
 * This function initializes the requested compressor and returns zero in case
 * of success or a negative error code in case of failure.
 */
static int compr_init(struct scfs_compressor *compr)
{
	if (compr->capi_name) {
		compr->cc = crypto_alloc_comp(compr->capi_name, 0, 0);
		if (IS_ERR(compr->cc)) {
			SCFS_PRINT_ERROR("cannot initialize compressor %s, error %ld",
				  compr->name, PTR_ERR(compr->cc));
			return PTR_ERR(compr->cc);
		}
	}

	scfs_compressors[compr->compr_type] = compr;
	SCFS_PRINT("compr name %s(%d) got cc(%x)\n",
		compr->capi_name, compr->compr_type, compr->cc); 
	return 0;
}

/**
 * compr_exit - de-initialize a compressor.
 * @compr: compressor description object
 */
static void compr_exit(struct scfs_compressor *compr)
{
	if (compr->capi_name)
		crypto_free_comp(compr->cc);
	return;
}

int scfs_compressors_init(void)
{
	int err;

	err = compr_init(&lzo_compr);
	if (err)
		return err;

	err = compr_init(&zlib_compr);
	if (err)
		goto out_lzo;

	scfs_compressors[SCFS_COMP_NONE] = &none_compr;
	return 0;

out_lzo:
	compr_exit(&lzo_compr);
	return err;
}

void scfs_compressors_exit(void)
{
	compr_exit(&lzo_compr);
	compr_exit(&zlib_compr);
}
