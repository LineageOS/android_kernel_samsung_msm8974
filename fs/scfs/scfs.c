/*
 * fs/scfs/scfs.c
 *
 * Copyright (C) 2014 Samsung Electronics Co., Ltd.
 *   Authors: Sunghwan Yun <sunghwan.yun@samsung.com>
 *            Jongmin Kim <jm45.kim@samsung.com>
 *            Sangwoo Lee <sangwoo2.lee@samsung.com>
 *            Inbae Lee   <inbae.lee@samsung.com>
 *
 * This program has been developed as a stackable file system based on
 * the WrapFS, which was written by:
 *
 * Copyright (C) 1997-2003 Erez Zadok
 * Copyright (C) 2001-2003 Stony Brook University
 * Copyright (C) 2004-2006 International Business Machines Corp.
 *   Author(s): Michael A. Halcrow <mahalcro@us.ibm.com>
 *              Michael C. Thompson <mcthomps@us.ibm.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <linux/crypto.h>
#include <linux/nsproxy.h>
#include <linux/parser.h>
#include <linux/statfs.h>
#include "scfs.h"
#include <linux/lzo.h>
#include <linux/ctype.h>

struct kmem_cache *scfs_file_info_cache;
struct kmem_cache *scfs_dentry_info_cache;
struct kmem_cache *scfs_inode_info_cache;
struct kmem_cache *scfs_sb_info_cache;
struct kmem_cache *scfs_info_entry_list;

#ifdef SCFS_MULTI_THREAD_COMPRESSION
struct kmem_cache *scfs_cbm_cache;
#endif

/* LZO must be enabled */
#if (!defined(CONFIG_LZO_DECOMPRESS) || !defined(CONFIG_LZO_COMPRESS))
#error "LZO library needs to be enabled!"
#endif

const char *tfm_names[SCFS_COMP_TOTAL_TYPES] = 
{
	"none",		/* none */
	"lzo",		/* lzo */
	"zlib",		/* zlib */ 
	"deflate",
	"fastlzo"	/* lzo */
};

extern struct scfs_compressor *scfs_compressors[SCFS_COMP_TOTAL_TYPES];

void scfs_printk(const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	vprintk(fmt, args);
	va_end(args);
}

int scfs_load_cinfo(struct scfs_inode_info *sii, struct file *lower_file)
{
	struct scfs_sb_info *sbi = SCFS_S(sii->vfs_inode.i_sb);
	void *buf;
	loff_t pos;
	int ret;

	ASSERT(lower_file);
	ASSERT(sii->compressed);

	buf = scfs_cinfo_alloc(sii, sii->cinfo_array_size);
	if (!buf) {
		return -ENOMEM;
	}

	pos = i_size_read(sii->lower_inode) - sii->cinfo_array_size - CF_SIZE;
	ASSERT(pos > 0);

	ret = scfs_lower_read(lower_file, buf, sii->cinfo_array_size, &pos);
	if (ret < 0) {
		scfs_cinfo_free(sii, buf);
		return ret;
	}
	ret = 0;

	if (scfs_check_cinfo(sii, buf)) {
		SCFS_PRINT("treat this file as non-compressed one(missing footer).\n");
		sii->cinfo_array_size = 0;
		sii->flags &= ~SCFS_DATA_COMPRESSABLE;
		sii->compressed = 0;
		sii->cluster_size = sbi->options.cluster_size;
		sii->comp_type = sbi->options.comp_type;
		sii->cinfo_array = NULL;
		scfs_cinfo_free(sii, buf);
	} else
		sii->cinfo_array = buf;

	return ret;
}

int scfs_reload_meta(struct file *file)
{
	struct dentry *dentry = file->f_dentry;
	struct scfs_inode_info *sii = SCFS_I(dentry->d_inode);
	struct file *lower_file;
	int ret = 0;

	ASSERT(IS_INVALID_META(sii));

	ret = scfs_initialize_lower_file(dentry, &lower_file, O_RDONLY); 
	if (ret) {
		SCFS_PRINT_ERROR("err in get_lower_file %s\n",
			dentry->d_name.name);
		return ret;
	}

	ret = scfs_footer_read(dentry->d_inode, lower_file);
	if (ret) {
		SCFS_PRINT_ERROR("f:%s err in reading footer, ret : %d\n",
			dentry->d_name.name, ret);
		goto out;
	}

	SCFS_PRINT("f:%s info size = %d \n",
		dentry->d_name.name, sii->cinfo_array_size);

	if (sii->cinfo_array)
		scfs_cinfo_free(sii, sii->cinfo_array);

	ret = scfs_load_cinfo(sii, lower_file);
	if (ret) {
		SCFS_PRINT_ERROR("f:%s err in loading cinfo, ret : %d\n",
			dentry->d_name.name, ret);
		goto out;
	}

	CLEAR_META_INVALID(sii);
out:
	SCFS_PRINT("f:%s calling fput\n", dentry->d_name.name);
	fput(lower_file);

	return ret;
}

/*
 * get_cluster_info
 *
 * Parameters:
 * @inode: inode in VFS layer
 * @cluster_n: cluster number wanted to get info
 * @*clust_info: address of cluster info structure
 *
 * Return:
 * SCFS_SUCCESS if success, otherwise if error
 *
 * Description:
 * Calculate the position of the cluster info for a given cluster,
 * and return the address
 */
int get_cluster_info(struct file *file, int cluster_idx, 
		 struct scfs_cinfo *target)
{
	struct scfs_inode_info *sii = SCFS_I(file->f_dentry->d_inode);
	struct scfs_cinfo *cinfo;
	struct cinfo_entry *cinfo_entry;
	struct list_head *head, *tmp;
	int ret = 0;

	ASSERT(IS_COMPRESSABLE(sii));

	if (IS_INVALID_META(sii)) {
		SCFS_PRINT("f:%s meta invalid flag is set, "
					"let's reload.\n",
					file->f_path.dentry->d_name.name);
		ret = scfs_reload_meta(file);
		if (ret) {
			SCFS_PRINT_ERROR("f:%s error in re-reading footer, err : %d\n",
				file->f_path.dentry->d_name.name, ret);
			goto out;
		}
	}

	ret = -EINVAL;
	if (cluster_idx + 1 > CLUSTER_COUNT(sii)) {
		SCFS_PRINT_ERROR("f:%s size check err, "
			"cluster_idx %d cluster count of the file %d\n", 
			file->f_path.dentry->d_name.name, cluster_idx, CLUSTER_COUNT(sii));
		goto out;
	}

	if (cluster_idx * sizeof(struct scfs_cinfo) < sii->cinfo_array_size) {
		cinfo = (struct scfs_cinfo *)(sii->cinfo_array) + cluster_idx;
		ret = 0;
	} else {
		if (list_empty(&sii->cinfo_list)) {
			SCFS_PRINT_ERROR("cluster idx : %d, and info size : %d, but info list is empty!\n",
					cluster_idx, sii->cinfo_array_size);
			goto out;
		}
		list_for_each_safe(head, tmp, &sii->cinfo_list) {
			cinfo_entry = list_entry(head, struct cinfo_entry, entry);
			if (cinfo_entry->current_cluster_idx < cluster_idx) {
				SCFS_PRINT_ERROR("cluster idx : %d, and current_cluster_idx %d\n",
						cluster_idx, cinfo_entry->current_cluster_idx);
				goto out;
			}

			if (cinfo_entry->current_cluster_idx == cluster_idx) {
				cinfo = &cinfo_entry->cinfo;
				ret = 0;
				goto out;
			}
		}
		SCFS_PRINT_ERROR("f:%s invalid cluster idx : %d or cluster_info(size : %d)\n",
			file->f_path.dentry->d_name.name,
			cluster_idx, sii->cinfo_array_size);
		ret = -EIO;
	}
out:	
	if (!ret) {
		target->offset = cinfo->offset;
		target->size = cinfo->size;
	}

	return ret;
}

enum {
	scfs_opt_nocompress,
	scfs_opt_cluster_size,
	scfs_opt_comp_threshold,
	scfs_opt_comp_type,
 	scfs_opt_err,
};

static const match_table_t tokens = {
	{scfs_opt_nocompress, "nocomp"},
	{scfs_opt_cluster_size, "cluster_size=%u"},
	{scfs_opt_comp_threshold, "comp_threshold=%u"},
	{scfs_opt_comp_type, "comp_type=%s"},
 	{scfs_opt_err, NULL}
};
 
int scfs_parse_options(struct scfs_sb_info *sbi, char *options)
{
	int token;
	int option;
	char *p;
	char *type;
	substring_t args[MAX_OPT_ARGS];

	if (!options)
		return 0;

	while ((p = strsep(&options, ",")) != NULL) {
		if (!*p)
			continue;

		token = match_token(p, tokens, args);
		switch (token) {
		case scfs_opt_nocompress:
			sbi->options.flags &= ~SCFS_DATA_COMPRESSABLE;
			break;
		case scfs_opt_cluster_size:
			if (match_int(&args[0], &option))
				return 0;
			if (option > SCFS_CLUSTER_SIZE_MAX ||
				option < SCFS_CLUSTER_SIZE_MIN) {
				SCFS_PRINT_ERROR("cluster_size, out of range\n");
				return -EINVAL;
			}
			if (!IS_POW2(option)) {
				SCFS_PRINT_ERROR("cluster_size must be a power of 2\n");
				return -EINVAL;
			}
			sbi->options.cluster_size = option;
			break;
		case scfs_opt_comp_threshold:
			if (match_int(&args[0], &option))
				return 0;
			if (option > 100 || option < 0) {
				SCFS_PRINT_ERROR("threshold, out of range, "
					"it's a percent\n");
				return -EINVAL;
			}
			sbi->options.comp_threshold = option;
			break;
		case scfs_opt_comp_type:
			type = args[0].from;
			if (!strcmp(type, "lzo"))
				sbi->options.comp_type = SCFS_COMP_LZO;
/* disable bzip for now, crypto_alloc_comp doesn't work for some reason */
#if 0 //#ifdef CONFIG_CRYPTO_DEFLATE
			else if (!strcmp(type, "bzip2"))
				sbi->options.comp_type = BZIP2;
#endif
#ifdef CONFIG_CRYPTO_ZLIB
			else if (!strcmp(type, "zlib"))
				sbi->options.comp_type = SCFS_COMP_ZLIB;
#endif
#ifdef CONFIG_CRYPTO_FASTLZO
			else if (!strcmp(type, "fastlzo"))
				sbi->options.comp_type = SCFS_COMP_FASTLZO;
#endif
			else {
				SCFS_PRINT_ERROR("invalid compression type\n");
				return -EINVAL;
			}
			break;
 		default:
			SCFS_PRINT_ERROR("Unrecognized mount option [%s]\n", p);
			return -EINVAL;
		}
	}
	return 0;
}


void copy_mount_flags_to_inode_flags(struct inode *inode, struct super_block *sb)
{
	struct scfs_sb_info *sbi = SCFS_S(sb);
	struct scfs_inode_info *sii = SCFS_I(inode);

	sii->cluster_size = sbi->options.cluster_size;
	sii->comp_type = sbi->options.comp_type;
	if (sbi->options.flags & SCFS_DATA_COMPRESSABLE)
		sii->flags |= SCFS_DATA_COMPRESSABLE;
}

int scfs_initialize_lower_file(struct dentry *dentry, struct file **lower_file, int flags)
{
	const struct cred *cred;
	int ret = 0;
	struct dentry *lower_dentry = scfs_lower_dentry(dentry);
	struct vfsmount *lower_mnt = scfs_dentry_to_lower_mnt(dentry);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,10,0)
	const struct path path = {lower_mnt, lower_dentry};
#else
	/* dput and mntput is done in dentry_open if it returns an error */
	dget(lower_dentry);
	mntget(lower_mnt);
#endif
	cred = current_cred();

	if (flags == EMPTY_FLAG)
		flags = IS_RDONLY(lower_dentry->d_inode) ? O_RDONLY : O_RDWR;
	else
		flags &= ~O_APPEND;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,10,0)
	(*lower_file) = dentry_open(&path, flags, cred);
#else
	(*lower_file) = dentry_open(lower_dentry, lower_mnt, flags, cred);
#endif
	if (IS_ERR(*lower_file)) {
		ret = PTR_ERR(*lower_file);
		SCFS_PRINT_ERROR("lower dentry_open fail, name : %s, ret : %d\n",
			lower_dentry->d_name.name, ret);
			*lower_file = NULL;
			return ret;
	}
#ifdef CONFIG_SCFS_LOWER_PAGECACHE_INVALIDATION
	/* 16KB fixed-size lower readahead */
	(*lower_file)->f_flags |= O_SCFSLOWER;
	(*lower_file)->f_ra.ra_pages = 4;
#endif
	return ret;
}

/**
 * scfs_read_cluster
 *
 * Parameters:
 * @file: upper file
 * @page: upper page from SCFS inode mapping
 * @buf_c: buffer for compressed cluster
 * @buf_u: bufferer for uncompressed cluster
 * @compressed: whether cluster was compressed or not
 *
 * Return:
 * SCFS_SUCCESS if success, otherwise if error
 * *compressed = 1 if cluster was compressed
 *
 * Description:
 * - Read a cluster, and if it was compressed, decompress it.
 */

int scfs_read_cluster(struct file *file, struct page *page,
	char *buf_c, char **buf_u, int *compressed)
{
	struct scfs_inode_info *sii = SCFS_I(page->mapping->host);
	struct scfs_cinfo cinfo;
	struct file *lower_file = NULL;
	int cluster_idx = 0, ret = 0;
	int size = 0, last_cluster_idx = 0;
	loff_t i_size = 0, pos = 0, tmp, left;
	size_t actual = 0;

	/* check upper inode size */
	i_size = i_size_read(&sii->vfs_inode);
	if (!i_size) {
		SCFS_PRINT("file %s: i_size is zero, "
			"flags 0x%x sii->clust_info_size %d\n",
			file->f_path.dentry->d_name.name, sii->flags,
			sii->cinfo_array_size);
		unlock_page(page);
		return ret;
	} else if (page->index * PAGE_SIZE >= i_size) {
		SCFS_PRINT("file %s: page->idx out of bounds, "
			"page->idx %d i_size %lld\n",
			file->f_path.dentry->d_name.name, page->index, i_size);
		unlock_page(page);
		return ret;
	}

	tmp = i_size;
	left = do_div(tmp, sii->cluster_size);
	if (left)
		tmp++;

	last_cluster_idx = tmp - 1;
	cluster_idx = PAGE_TO_CLUSTER_INDEX(page, sii);

	if (cluster_idx > last_cluster_idx) {
			SCFS_PRINT_ERROR("file %s: cluster_idx out of range, "
				"clust %u of %u, i_size %lld, "
				"page->index %d\n",
				file->f_path.dentry->d_name.name,
				cluster_idx, last_cluster_idx, i_size, page->index);
			return -ERANGE;
	}

	if (IS_COMPRESSABLE(sii)) {
		mutex_lock(&sii->cinfo_mutex);
		ret = get_cluster_info(file, cluster_idx, &cinfo);
		mutex_unlock(&sii->cinfo_mutex);
		if (ret) {
			SCFS_PRINT_ERROR("err in get_cluster_info, ret : %d,"
				"i_size %lld\n", ret, i_size);
			return ret;
		}

		if (!cinfo.size || cinfo.size > sii->cluster_size) {
			SCFS_PRINT_ERROR("file %s: cinfo is invalid, "
				"clust %u of %u cinfo.size %u\n",
				file->f_path.dentry->d_name.name,
				cluster_idx, last_cluster_idx, cinfo.size);
			return -EINVAL;
		}

		/* decide if cluster was compressed */
		if (cinfo.size == sii->cluster_size) {
			*compressed = 0;
		} else {
			if (cluster_idx == last_cluster_idx && left == cinfo.size)
				*compressed = 0;
			else
				*compressed = 1;
		}
		size = cinfo.size;
		pos = (loff_t)cinfo.offset;
	} else {
		*compressed = 0;
		size = sii->cluster_size;

		if (cluster_idx == last_cluster_idx && left)
			size = left;

		pos = (loff_t)cluster_idx * sii->cluster_size;
	}
	
	lower_file = scfs_lower_file(file);
	if (!lower_file) {
		SCFS_PRINT_ERROR("file %s: lower file is null!\n",
			file->f_path.dentry->d_name.name);
		return -EINVAL;
	}

	/* vfs read, either cluster or page */
#ifdef SCFS_REMOVE_NO_COMPRESSED_UPPER_MEMCPY
	if (!*compressed) {
		buf_c = kmap(page);
		size -= (PGOFF_IN_CLUSTER(page, sii) * PAGE_SIZE);

		if (size > PAGE_SIZE) size = PAGE_SIZE;
		pos += (PGOFF_IN_CLUSTER(page, sii) * PAGE_SIZE);
	}
#endif
	ret = scfs_lower_read(lower_file, buf_c, size, &pos);	

#ifdef SCFS_REMOVE_NO_COMPRESSED_UPPER_MEMCPY
	if (!*compressed)
		kunmap(page);
#endif

	if (ret < 0) {
		SCFS_PRINT_ERROR("file %s: vfs_read failed, clust %d of %d, "
			"size %u, pos %lld, ret %d(0x%x), "
			"compressed %d, page->index %d,"
			"i_size %lld, sii->flags 0x%x, sii->cis %d\n",
			file->f_path.dentry->d_name.name, cluster_idx,
			last_cluster_idx, size, pos, ret, ret, *compressed,
			page->index, i_size, sii->flags, sii->cinfo_array_size);
		unlock_page(page);
		return ret;
	}
	ret = 0;

	/* decompress cluster if needed */
	if (*compressed) {
		actual = (size_t)sii->cluster_size;
		ret = scfs_decompress(sii->comp_type, buf_c, *buf_u,
			size, &actual);

		if (ret) {
			SCFS_PRINT_ERROR("file %s: decompress failed. "
				"clust %u of %u, offset %u size %u ret 0x%x "
				"buf_c 0x%x buf_u 0x%x\n",
				file->f_path.dentry->d_name.name, 
				cluster_idx, last_cluster_idx, cinfo.offset, 
				size, ret, buf_c, *buf_u);
			ClearPageUptodate(page);
			unlock_page(page);
			return ret;
		}
	}

	return ret;
}

/*
 * scfs_decompress
 *
 * Parameters:
 * @algo: algorithm type to use
 * @*buf_c: global buffer for compressed cluster data
 * @*buf_u: global buffer for decompressed cluster data
 * @len: compressed size of this cluster
 * @*actual: IN - full cluster size, OUT - decompressed size
 * Return:
 * SCFS_SUCCESS if success, otherwise if error
 *
 * Description:
 * Decompress a cluster. *actual needs to be set as size of the original
 * cluster by the caller.
 */
int scfs_decompress(enum comp_type algo, char *buf_c, char *buf_u, size_t len, 
	size_t *actual)
{
	int ret = 0;

	ASSERT(algo < SCFS_COMP_TOTAL_TYPES);

#ifdef CONFIG_SCFS_USE_CRYPTO
	ret = scfs_decompress_crypto((void *)buf_c, len, (void *)buf_u, actual, (int)algo);
	if (ret) {
		SCFS_PRINT("%s decompress error! "
			"ret %d len %d tmp_len %d\n", scfs_compressors[algo]->name,
			ret, len, *actual);
		ret = -EIO;
	}
#else // Use kernel libraries directly
	switch (algo) {
	case SCFS_COMP_LZO:
		ret = lzo1x_decompress_safe(buf_c, len, buf_u, actual);
		if (ret) {
			SCFS_PRINT_ERROR("lzo decompress error! "
					"ret %d len %d tmp_len %d\n",
					ret, len, *actual);
			ret = -EIO;
		}
		break;
	default:
		ret = scfs_decompress_crypto((void *)buf_c, len, (void *)buf_u, actual, (int)algo);
		if (ret) {
			SCFS_PRINT("%s decompress error! "
					"ret %d len %d tmp_len %d\n", scfs_compressors[algo]->name,
					ret, len, *actual);
			ret = -EIO;
		}
		break;
	}
#endif

	return ret;
}

/*
 * scfs_compress
 *
 * Parameters:
 * @algo: algorithm type to use
 * @*buf_c: global buffer for compressed cluster data
 * @*buf_u: global buffer for decompressed cluster data
 * @len: uncompressed size of this cluster
 * @*actual: IN - full cluster size, OUT - compressed size
 
 * Return:
 * SCFS_SUCCESS if success, otherwise if error
 *
 * Description:
 * Compress a cluster. *actual needs to be set as full cluster size
 * by the caller.
 */

int scfs_compress(enum comp_type algo, char *buf_c, char *buf_u, size_t len, 
	size_t *actual, void *workdata, struct scfs_sb_info *sbi)
{
	int ret = 0;

	ASSERT(algo < SCFS_COMP_TOTAL_TYPES);

#ifdef CONFIG_SCFS_USE_CRYPTO
	ret = scfs_compress_crypto((void *)buf_u, len, (void *)buf_c, actual, (int)algo);
	if (ret) {
		SCFS_PRINT("%s compress error! "
			"ret %d len %d tmp_len %d\n", scfs_compressors[algo]->name,
			ret, len, *actual);
			*actual = len; // We use raw data if compression was failed.
	}
#else	// Use kernel libraries directly
	switch (algo) {
	case SCFS_COMP_LZO:
		if (!workdata) {
			spin_lock(&sbi->workdata_lock);
			memset(sbi->scfs_workdata, 0, LZO1X_MEM_COMPRESS);
			ret = lzo1x_1_compress(buf_u, len, buf_c, actual, sbi->scfs_workdata);
			spin_unlock(&sbi->workdata_lock);
		} else {
			memset(workdata, 0, LZO1X_MEM_COMPRESS);
			ret = lzo1x_1_compress(buf_u, len, buf_c, actual, workdata);
		}

		if (ret) {
			SCFS_PRINT("lzo compress error! "
				"ret %d len %d tmp_len %d\n", ret, len, *actual);
			ret = -EIO;
		}
		break;
	default:
		ret = scfs_compress_crypto((void *)buf_u, len, (void *)buf_c, actual, (int)algo);
		if (ret) {
			SCFS_PRINT("%s compress error! "
					"ret %d len %d tmp_len %d\n", scfs_compressors[algo]->name,
					ret, len, *actual);
			*actual = len; // We use raw data if compression was failed.
		}
		break;
	}
#endif

	return ret;
}

struct page *scfs_alloc_mempool_buffer(struct scfs_sb_info *sbi)
{
	struct page *ret = mempool_alloc(sbi->mempool, 
			__GFP_NORETRY | __GFP_NOMEMALLOC | __GFP_NOWARN);
	
	if (ret != NULL)
		profile_add_mempooled(SCFS_MEMPOOL_SIZE, sbi);

	return ret;
}

void scfs_free_mempool_buffer(struct page *p, struct scfs_sb_info *sbi)
{
	if (!p)
		return;

	mempool_free(p, sbi->mempool);
	profile_sub_mempooled(SCFS_MEMPOOL_SIZE, sbi);
}

int scfs_check_space(struct scfs_sb_info *sbi, struct dentry *dentry)
{
	struct dentry *lower_dentry = scfs_lower_dentry(dentry);
	struct kstatfs buf;
	int ret = 0;
	size_t min_space = (atomic_read(&sbi->total_cluster_count) *
		sizeof(struct scfs_cinfo)) + (atomic_read(&sbi->current_file_count) *
		CF_SIZE) + atomic64_read(&sbi->current_data_size) + PAGE_SIZE;

	ret = lower_dentry->d_sb->s_op->statfs(lower_dentry, &buf);
	if (ret)
		return ret;

	if ((buf.f_bavail * PAGE_SIZE) < min_space) {
		SCFS_PRINT_ERROR("bavail = %lld, req_space = %lld\n", buf.f_bavail * PAGE_SIZE
			, min_space);
		ret = -ENOSPC;
	}
	
	return ret;
}

void sync_page_to_buffer(struct page *page, char *buffer)
{
	char *source_addr;

	source_addr = kmap_atomic(page);
	SCFS_PRINT(" buffer = %x , page address = %x\n", buffer, 
		buffer + (PAGE_SIZE * PGOFF_IN_CLUSTER(page, SCFS_I(page->mapping->host))));
	memcpy(buffer + (PAGE_SIZE * PGOFF_IN_CLUSTER(page, SCFS_I(page->mapping->host))),
		source_addr, PAGE_SIZE);	

	kunmap_atomic(source_addr);
}

void sync_page_from_buffer(struct page *page, char *buffer)
{
	char *dest_addr;

	dest_addr = kmap_atomic(page);

	SCFS_PRINT(" buffer = %x , page address = %x\n", buffer, 
		buffer + (PAGE_SIZE*PGOFF_IN_CLUSTER(page, SCFS_I(page->mapping->host))));
	memcpy(dest_addr, buffer + (PAGE_SIZE *
		PGOFF_IN_CLUSTER(page, SCFS_I(page->mapping->host))), PAGE_SIZE);	

	kunmap_atomic(dest_addr);
}

int scfs_write_cinfo(struct scfs_inode_info *sii, struct file *lower_file, loff_t *pos)
{
	struct scfs_sb_info *sbi = SCFS_S(sii->vfs_inode.i_sb);
	struct list_head *head, *tmp;
	struct cinfo_entry *cinfo_entry; 
	int ret, written = 0, cinfo_size = sizeof(struct scfs_cinfo);
	char *buf_pos = sii->cluster_buffer.u_buffer;;

	ASSERT(sii->compressed);

	if (sii->cinfo_array_size) {
		ASSERT(!list_empty(&sii->cinfo_list));
		cinfo_entry = list_entry(sii->cinfo_list.next, struct cinfo_entry, entry);
		ret = scfs_lower_write(lower_file, sii->cinfo_array,
			sizeof(struct scfs_cinfo) * cinfo_entry->current_cluster_idx, pos);
		if (ret < 0) {
			SCFS_PRINT_ERROR("f:%s write fail in writing" \
				"existing meta, ret : %d.\n",
				lower_file->f_dentry->d_name.name, ret);
			MAKE_META_INVALID(sii);
			return ret;
		} else {
			written += ret;
			ret = 0;
		}
	}

	list_for_each_safe(head, tmp, &sii->cinfo_list) {
		cinfo_entry = list_entry(head, struct cinfo_entry, entry);
		memcpy(buf_pos, &cinfo_entry->cinfo, cinfo_size);
		buf_pos += cinfo_size;
		list_del(&cinfo_entry->entry);
		kmem_cache_free(scfs_info_entry_list, cinfo_entry);
		profile_sub_kmcached(sizeof(cinfo_entry), sbi);

		if (buf_pos > sii->cluster_buffer.u_buffer +
				((sii->cluster_size * 2) - cinfo_size) ||
				list_empty(&sii->cinfo_list)) {
			ret = scfs_lower_write(lower_file,
				sii->cluster_buffer.u_buffer,
				(size_t)(buf_pos - sii->cluster_buffer.u_buffer),
				pos);
			if (ret < 0) {	
				SCFS_PRINT_ERROR("f:%s write fail in writing " \
					"new metas, ret : %d\n",
					lower_file->f_dentry->d_name.name, ret);
				MAKE_META_INVALID(sii);
				return ret;
			}
			written += ret;
			atomic_sub(ret / sizeof(struct scfs_cinfo),
				&sbi->total_cluster_count);
			buf_pos = sii->cluster_buffer.u_buffer;
		}
	}
	return written;
}

int scfs_write_meta(struct file *file)
{
	struct list_head *head = NULL, *tmp;
	struct cinfo_entry *last, *cinfo_entry = NULL;
	struct comp_footer cf = {0, };
	struct scfs_inode_info *sii = SCFS_I(file->f_dentry->d_inode);
	struct file *lower_file;
	struct scfs_sb_info *sbi = SCFS_S(sii->vfs_inode.i_sb);
	struct inode *lower_inode;
	struct iattr ia;
	int ret = 0;
	char *source = NULL;
	loff_t pos;
	size_t tmp_len;

#ifdef SCFS_MULTI_THREAD_COMPRESSION
	struct cinfo_entry *prev_info_entry = NULL;
#endif

	ret = scfs_initialize_lower_file(file->f_dentry, &lower_file, O_WRONLY); 
	if (ret) {
		SCFS_PRINT_ERROR("err in get_lower_file %s\n", file->f_dentry->d_name.name);
		return ret;
	}

	SCFS_PRINT("filename : %s\n", lower_file->f_dentry->d_name.name);

	mutex_lock(&sii->cinfo_mutex);
	if (list_empty(&sii->cinfo_list)) {
		SCFS_PRINT("cinfo_list is empty\n");
		mutex_unlock(&sii->cinfo_mutex);
		goto out;
	}
	last = list_entry(sii->cinfo_list.prev, struct cinfo_entry, entry);

#ifdef SCFS_MULTI_THREAD_COMPRESSION
	scfs_write_compress_all_cluster(sii, lower_file);
#endif
	/* if last cluster exists, we should write it first. */
	if (IS_COMPRESSABLE(sii)) {
	 	if (sii->cluster_buffer.original_size > 0) {
#ifdef SCFS_MULTI_THREAD_COMPRESSION
			/* update current cluster's offset using previous cluster */
			if (sii->is_inserted_to_sii_list) {
				prev_info_entry = list_entry(last->entry.prev,
					struct cinfo_entry, entry);
				last->cinfo.offset = prev_info_entry->cinfo.offset +
					prev_info_entry->cinfo.size;

				if (prev_info_entry->cinfo.size % SCFS_CLUSTER_ALIGN_BYTE)
					last->cinfo.offset += (SCFS_CLUSTER_ALIGN_BYTE -
						(prev_info_entry->cinfo.size % SCFS_CLUSTER_ALIGN_BYTE));
			}
#endif
			/* Set cinfo size as available buffer size because zlib care about
			 * available buf size. */
			last->cinfo.size = PAGE_CACHE_SIZE * 8;
			tmp_len = (size_t)last->cinfo.size;
			ret = scfs_compress(sii->comp_type, sii->cluster_buffer.c_buffer,
				sii->cluster_buffer.u_buffer,
				sii->cluster_buffer.original_size,
				&tmp_len,
				NULL, sbi);
			last->cinfo.size = (__u32)(tmp_len & 0xffff);
			if (ret) {
				SCFS_PRINT_ERROR("f:%s Compression failed." \
					"So, write uncompress data.\n",
				lower_file->f_dentry->d_name.name);
				goto free_out;;
			}
			last->pad = ALIGN(last->cinfo.size, SCFS_CLUSTER_ALIGN_BYTE) -
				last->cinfo.size;
			pos = (loff_t)last->cinfo.offset;

			if (last->cinfo.size <
					sii->cluster_buffer.original_size *
					sbi->options.comp_threshold / 100) {		
				source = sii->cluster_buffer.c_buffer;
				sii->compressed = 1;					
			} else {
				last->cinfo.size =
					sii->cluster_buffer.original_size;
				source = sii->cluster_buffer.u_buffer;
			}

			ret = scfs_lower_write(lower_file, source,
				last->cinfo.size + last->pad, &pos);
			if (ret < 0) {
				SCFS_PRINT_ERROR("f:%s writing last cluster buffer failed, ret : %d\n",
					lower_file->f_dentry->d_name.name, ret);
				MAKE_META_INVALID(sii);
				goto free_out;
			} else
				ret = 0;

			atomic64_sub(sii->cluster_buffer.original_size ,&sbi->current_data_size);
			sii->cluster_buffer.original_size = 0;
		}
		pos = ALIGN(last->cinfo.offset + last->cinfo.size, SCFS_CLUSTER_ALIGN_BYTE);

#ifdef SCFS_MULTI_THREAD_COMPRESSION
		sii->is_inserted_to_sii_list = 0;
#endif

		if (sii->compressed) {
#ifdef SCFS_MULTI_THREAD_COMPRESSION
			ret = scfs_get_comp_buffer(sii);
			if (ret < 0)
				goto free_out;
#endif
			ret = scfs_write_cinfo(sii, lower_file, &pos);
			if (ret < 0)
				goto free_out;
			cf.footer_size = ret;
		}
	} else { //file not compressed
		pos = i_size_read(&sii->vfs_inode);
		/* Remove fake cluster_info */
		cinfo_entry = list_entry(sii->cinfo_list.prev, struct cinfo_entry, entry);
		list_del(&cinfo_entry->entry);
		kmem_cache_free(scfs_info_entry_list, cinfo_entry);
		atomic_sub(1, &sbi->total_cluster_count);
	}
	cf.footer_size += CF_SIZE;
	cf.cluster_size = sii->cluster_size;
	cf.comp_type = sii->comp_type;
	cf.original_file_size = i_size_read(&sii->vfs_inode);
	cf.magic = SCFS_MAGIC;

	ret = scfs_lower_write(lower_file, (char*)&cf, CF_SIZE , &pos);
	if (ret < 0) {		
		SCFS_PRINT_ERROR("f:%s write fail, comp_footer, ret : %d",
			lower_file->f_dentry->d_name.name, ret);
		MAKE_META_INVALID(sii);
		goto free_out;
	} else
		ret = 0;

	lower_inode = lower_file->f_dentry->d_inode;
	/* file may have shrunk after append-write */
	if (pos < i_size_read(lower_inode)) {
		ia.ia_valid = ATTR_SIZE;
		ia.ia_size = pos;
		truncate_setsize(lower_inode, pos);

		mutex_lock(&lower_inode->i_mutex);
		ret = notify_change(lower_file->f_dentry, &ia);
		mutex_unlock(&lower_inode->i_mutex);
		if (ret) {
			SCFS_PRINT_ERROR("f:%s error in lower_truncate, %d",
				lower_file->f_dentry->d_name.name,
				ret);
			MAKE_META_INVALID(sii);
			goto free_out;
		}
	}
	if (cf.footer_size > CF_SIZE)
		MAKE_META_INVALID(sii);
	else
		sii->flags &= ~SCFS_DATA_COMPRESSABLE;

free_out:
	if (!list_empty(&sii->cinfo_list)) {
		list_for_each_safe(head, tmp, &sii->cinfo_list) {
			cinfo_entry = list_entry(head, struct cinfo_entry, entry);
			list_del(&cinfo_entry->entry);
			kmem_cache_free(scfs_info_entry_list, cinfo_entry);
			profile_sub_kmcached(sizeof(struct cinfo_entry), sbi);
		}
	}
	mutex_unlock(&sii->cinfo_mutex);
out:
	fput(lower_file);
	return ret;
}

struct cinfo_entry *scfs_alloc_cinfo_entry(unsigned int cluster_index,
		struct scfs_inode_info *sii)
{
	struct cinfo_entry *new_entry = NULL;
	struct scfs_sb_info *sbi = SCFS_S(sii->vfs_inode.i_sb);

	new_entry = kmem_cache_zalloc(scfs_info_entry_list, GFP_KERNEL);
	if (!new_entry) {
		SCFS_PRINT_ERROR("kmem_cache_zalloc ERROR.\n");
		return NULL;
	}

	profile_add_kmcached(sizeof(struct cinfo_entry), sbi);
	new_entry->current_cluster_idx = cluster_index;
	list_add_tail(&new_entry->entry, &sii->cinfo_list);
	atomic_add(1, &sbi->total_cluster_count);
	return new_entry;
}

int scfs_get_cluster_from_lower(struct file *file, struct scfs_cinfo clust_info)
{
	struct dentry *dentry = file->f_dentry;
	struct scfs_inode_info *sii = SCFS_I(dentry->d_inode);
	struct file *lower_file;
	loff_t pos = 0;
	int ret;

	ret = scfs_initialize_lower_file(dentry, &lower_file, O_RDONLY); 
	if (ret) {
		SCFS_PRINT_ERROR("err in get_lower_file %s\n", dentry->d_name.name);
		return ret;
	}

	if (clust_info.size > sii->cluster_size) {
		SCFS_PRINT_ERROR("f:%s clust_info.size out of bounds, size %d\n",
			lower_file->f_path.dentry->d_name.name, clust_info.size);
		return -EINVAL;
	}
	pos = clust_info.offset;

	if (IS_COMPRESSABLE(sii) && clust_info.size < sii->cluster_size) {
		//TODO pass appropriate algorithm, retrieved from file meta
		loff_t i_size = i_size_read(&sii->vfs_inode);

		if(do_div(i_size, sii->cluster_size) == clust_info.size) {
			ret = scfs_lower_read(lower_file, sii->cluster_buffer.u_buffer,
				clust_info.size, &pos);
			if (ret < 0) {
				SCFS_PRINT_ERROR("f:%s read failed, size %d pos %d ret = %d\n",
					lower_file->f_path.dentry->d_name.name,
					clust_info.size, (int)pos, ret);
				goto out;
			}
			ret = 0;

			sii->cluster_buffer.original_size = clust_info.size;
		} else {			
			size_t len = (size_t)sii->cluster_size;

			ret = scfs_lower_read(lower_file, sii->cluster_buffer.c_buffer,
				clust_info.size, &pos);
			if (ret < 0) {
				SCFS_PRINT_ERROR("f:%s read failed, size %d pos %d ret = %d\n",
					lower_file->f_path.dentry->d_name.name,
					clust_info.size, (int)pos, ret);
				goto out;
			}
			ret = 0;

 			ret = scfs_decompress(sii->comp_type, 
						sii->cluster_buffer.c_buffer, 
						sii->cluster_buffer.u_buffer, 
						clust_info.size,
						&len);
			if (ret) {
				SCFS_PRINT_ERROR("f:%s decompress lower cluster failed.\n",
					lower_file->f_path.dentry->d_name.name);
				goto out;
			}				
			sii->cluster_buffer.original_size = len;
		}
	} else {
		ret = scfs_lower_read(lower_file, sii->cluster_buffer.u_buffer,
			clust_info.size, &pos);
		if (ret < 0) {
			SCFS_PRINT_ERROR("f:%s vfs_read failed, size %d pos %d ret = %d\n",
				lower_file->f_path.dentry->d_name.name,
				clust_info.size, (int) pos, ret);
			goto out;
		}
		ret = 0;
		sii->cluster_buffer.original_size = clust_info.size;		
	}
out:
	fput(lower_file);
	return ret;
}

int scfs_get_comp_buffer(struct scfs_inode_info *sii)
{
	struct scfs_sb_info *sbi = SCFS_S(sii->vfs_inode.i_sb);

	if (!sii->cluster_buffer.u_buffer) {
		sii->cluster_buffer.u_page = alloc_pages(GFP_KERNEL, SCFS_MEMPOOL_ORDER + 1);
		if (!sii->cluster_buffer.u_page) {
			SCFS_PRINT_ERROR("u_page malloc failed\n");
			return -ENOMEM;
		}			
		sii->cluster_buffer.u_buffer = page_address(sii->cluster_buffer.u_page);

		if (!sii->cluster_buffer.u_buffer)
			return -ENOMEM;
		atomic_add(1, &sbi->current_file_count);
	}

	if (!sii->cluster_buffer.c_buffer) {
		sii->cluster_buffer.c_page = alloc_pages(GFP_KERNEL, SCFS_MEMPOOL_ORDER + 1);
		if (!sii->cluster_buffer.c_page) {
			SCFS_PRINT_ERROR("c_page malloc failed\n");
			return -ENOMEM;
		}				
		sii->cluster_buffer.c_buffer = page_address(sii->cluster_buffer.c_page);

		if (!sii->cluster_buffer.c_buffer)
			return -ENOMEM;
	}
	
	return 0;
}
 
int scfs_truncate(struct dentry *dentry, loff_t size)
{
	struct iattr ia = { .ia_valid = ATTR_SIZE, .ia_size = size };
	struct inode *inode = dentry->d_inode;
	struct scfs_inode_info *sii = SCFS_I(inode);
	struct dentry *lower_dentry = scfs_lower_dentry(dentry);
	struct list_head *cluster_info, *tmp;
	struct cinfo_entry *info_index;
	int ret = 0;

	if (size) {
		SCFS_PRINT_ERROR("only truncate to zero-size is allowd\n");
		return -EINVAL;
	}

	SCFS_PRINT("Truncate %s size to %lld\n", dentry->d_name.name, size);
	truncate_setsize(inode, ia.ia_size);
	mutex_lock(&sii->cinfo_mutex);

	list_for_each_safe(cluster_info, tmp, &sii->cinfo_list) {
		info_index = list_entry(cluster_info,
				struct cinfo_entry, entry);
		list_del(&info_index->entry);
		kmem_cache_free(scfs_info_entry_list, info_index);
		profile_sub_kmcached(sizeof(struct cinfo_entry), SCFS_S(inode->i_sb));
	}
	mutex_unlock(&sii->cinfo_mutex);

	mutex_lock(&lower_dentry->d_inode->i_mutex);
	ret = notify_change(lower_dentry, &ia);
	mutex_unlock(&lower_dentry->d_inode->i_mutex);
	if (ret)
		return ret;

	ret = scfs_initialize_file(dentry, inode);
	if (ret) {
		SCFS_PRINT_ERROR("f:%s err in initializing file, ret : %d\n",
			dentry->d_name.name, ret);
		MAKE_META_INVALID(sii);
		return ret;
	}
	if (sii->cinfo_array) {
		scfs_cinfo_free(sii, sii->cinfo_array);
		sii->cinfo_array = NULL;
	}
	sii->cinfo_array_size = 0;
	sii->upper_file_size = 0;
	sii->cluster_buffer.original_size = 0;
	sii->compressed = 0;
	if (SCFS_S(inode->i_sb)->options.flags & SCFS_DATA_COMPRESSABLE)
		sii->flags |= SCFS_DATA_COMPRESSABLE;
	else
		sii->flags &= ~SCFS_DATA_COMPRESSABLE;

	CLEAR_META_INVALID(sii);

	return ret;
}

/* This function returns read count on lower fs, not 0 when succeed */
ssize_t scfs_lower_read(struct file *file, char *buf, size_t count, loff_t *pos)
{
	int ret, read = 0, retry = 0;
	mm_segment_t  fs_save;

	fs_save = get_fs();

	while (read < count) {
		set_fs(get_ds());
		ret = vfs_read(file, buf + read, count - read, pos);
		set_fs(fs_save);
		if (ret < 0) {
			if (ret == -EINTR || ret == -EAGAIN) {
				SCFS_PRINT("still hungry, ret : %d, %lld/%lld\n",
						ret, read, count - read);
				continue;
			}

			SCFS_PRINT_ERROR("f:%s err in vfs_read, ret : %d\n",
				file->f_path.dentry->d_name.name, ret);
			return ret;
		}
		read += ret;
		if (++retry > SCFS_IO_MAX_RETRY) {
			SCFS_PRINT_ERROR("f:%s too many retries\n",
				file->f_path.dentry->d_name.name);
			return -EIO;
		}
	}
	return read;
}

ssize_t scfs_lower_write(struct file *file, char *buf, size_t count, loff_t *pos)
{
	int ret, written = 0, retry = 0;
	mm_segment_t fs_save;

	fs_save = get_fs();

	while (written < count) {
		set_fs(get_ds());
		ret = vfs_write(file, buf + written, count - written, pos);
		set_fs(fs_save);
		if (ret < 0) {
			if (ret == -EINTR || ret == -EAGAIN) {
				SCFS_PRINT("still hungry, ret : %d, %lld/%lld\n",
						ret, written, count - written);
				continue;
			}

			SCFS_PRINT_ERROR("f:%s err in vfs_write, ret : %d\n",
				file->f_path.dentry->d_name.name, ret);
			return ret;
		}
		written += ret;
		if (++retry > SCFS_IO_MAX_RETRY) {
			SCFS_PRINT_ERROR("f:%s too many retries\n",
				file->f_path.dentry->d_name.name);
			return -EIO;
		}
	}
	return written;
}

/**
 * inode_info_init_once
 *
 * Initializes the scfs_inode_info_cache when it is created
 */
void
inode_info_init_once(void *vptr)
{
	struct scfs_inode_info *sii = (struct scfs_inode_info *)vptr;

	inode_init_once(&sii->vfs_inode);
}


static struct scfs_cache_info {
	struct kmem_cache **cache;
	const char *name;
	size_t size;
	void (*ctor)(void *obj);
} scfs_cache_infos[] = {
	{
		.cache = &scfs_file_info_cache,
		.name = "scfs_file_cache",
		.size = sizeof(struct scfs_file_info),
	},
	{
		.cache = &scfs_dentry_info_cache,
		.name = "scfs_dentry_info_cache",
		.size = sizeof(struct scfs_dentry_info),
	},
	{
		.cache = &scfs_inode_info_cache,
		.name = "scfs_inode_cache",
		.size = sizeof(struct scfs_inode_info),
		.ctor = inode_info_init_once,
	},
	{
		.cache = &scfs_sb_info_cache,
		.name = "scfs_sb_cache",
		.size = sizeof(struct scfs_sb_info),
	},
	{
		.cache = &scfs_info_entry_list,
		.name = "scfs_info_entry_list",
		.size = sizeof(struct cinfo_entry),
	},
#ifdef SCFS_MULTI_THREAD_COMPRESSION
	{
		.cache = &scfs_cbm_cache,
		.name = "scfs_cbm_cache",
		.size = sizeof(struct scfs_cluster_buffer_mtc),
	},
#endif
};

void scfs_free_kmem_caches(void)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(scfs_cache_infos); i++) {
		struct scfs_cache_info *info;

		info = &scfs_cache_infos[i];
		if (*(info->cache))
			kmem_cache_destroy(*(info->cache));
	}
}

/**
 * scfs_init_kmem_caches
 *
 * Returns zero on success; non-zero otherwise
 */
int scfs_init_kmem_caches(void)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(scfs_cache_infos); i++) {
		struct scfs_cache_info *info;

		info = &scfs_cache_infos[i];
		*(info->cache) = kmem_cache_create(info->name, info->size,
				0, SLAB_HWCACHE_ALIGN, info->ctor);
		if (!*(info->cache)) {
			scfs_free_kmem_caches();
			SCFS_PRINT("kmem_cache_create failed\n",
					info->name);
			return -ENOMEM;
		}
	}
	return 0;
}

void *scfs_cinfo_alloc(struct scfs_inode_info *sii, unsigned long size)
{
	SCFS_PRINT("cinfo_alloc, size : %d\n", size);
	if (size >= PAGE_SIZE) {
		sii->flags |= SCFS_CINFO_OVER_PAGESIZE;
		profile_add_vmalloced(PAGE_ALIGN(size) + PAGE_SIZE,
			SCFS_S(sii->vfs_inode.i_sb));
		return vmalloc(size);
	} else {
		sii->flags &= ~SCFS_CINFO_OVER_PAGESIZE;
		profile_add_kmalloced(size,
			SCFS_S(sii->vfs_inode.i_sb));
		return kmalloc(size, GFP_KERNEL);
	}
}

void scfs_cinfo_free(struct scfs_inode_info *sii, const void *addr)
{
	SCFS_PRINT("cinfo_free, size : %d\n", sii->cinfo_array_size);
	if (sii->flags & SCFS_CINFO_OVER_PAGESIZE) {
		profile_sub_vmalloced(PAGE_ALIGN(sii->cinfo_array_size) + PAGE_SIZE,
			SCFS_S(sii->vfs_inode.i_sb));
		vfree(addr);
	} else {
		profile_sub_kmalloced(sii->cinfo_array_size, SCFS_S(sii->vfs_inode.i_sb));
		kfree(addr);
	}
}
