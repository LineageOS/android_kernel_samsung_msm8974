/*
 * fs/scfs/scfs.h
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

#ifndef SCFS_HEADER_H
#define SCFS_HEADER_H

#include <linux/file.h>
#include <linux/fs_stack.h>
#include <linux/mount.h>
#include <linux/namei.h>
#include <linux/pagemap.h>
#include <linux/page-flags.h>
#include <linux/slab.h>
#include <linux/buffer_head.h>
#include <linux/backing-dev.h>
#include <linux/vmalloc.h>
#include <linux/time.h>
#include <linux/mempool.h>
#include <linux/version.h>
#include <linux/debugfs.h>
#include <linux/syscalls.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/path.h>

extern const struct address_space_operations scfs_aops;
extern const struct inode_operations scfs_symlink_iops;
extern const struct inode_operations scfs_dir_iops;
extern const struct inode_operations scfs_file_iops;
extern const struct file_operations scfs_dir_fops;
extern const struct file_operations scfs_file_fops;
extern const struct super_operations scfs_sops;
extern const struct dentry_operations scfs_dops;

extern struct kmem_cache *scfs_file_info_cache;
extern struct kmem_cache *scfs_dentry_info_cache;
extern struct kmem_cache *scfs_inode_info_cache;
extern struct kmem_cache *scfs_sb_info_cache;


/*****************/
/* debug options */
/*****************/

//#define SCFS_DEBUG
//#define SCFS_PROFILE
//#define SCFS_PROFILE_MEM
 

/*****************************************/
/* size configs, flags, error code, etc. */
/*****************************************/

/* magic values for sanity checking */
#define SCFS_SUPER_MAGIC		0x53305955
#define SCFS_MAGIC			SCFS_SUPER_MAGIC

/* cluster size */
#define SCFS_CLUSTER_ALIGN_BYTE		4
#define SCFS_CLUSTER_SIZE_DEF		(16 * 1024)
#define SCFS_CLUSTER_SIZE_MAX		(16 * 1024)
#define SCFS_CLUSTER_SIZE_MIN		(4 * 1024)

/* mount option & file status flags */
#define SCFS_DATA_RAW			0x00000001
#define SCFS_DATA_COMPRESSABLE		0x00000002
#define SCFS_META_XATTR			0x00000004
#define SCFS_CINFO_OVER_PAGESIZE	0x00000008

/* scfs meta status */
#define SCFS_INVALID_META		0x00000010
#define SCFS_MISSING_META		0x00000020
#define SCFS_WRITE_OPENED		0x00000080

/* mempool for cluster buffers */
/* i.e. flagship (16KB clusters): 32KB x 32, low-end (8KB): 16KB x 32 */
#define SCFS_MEMPOOL_COUNT		32
#define SCFS_MEMPOOL_ORDER		2
#define SCFS_MEMPOOL_SIZE		(1 << SCFS_MEMPOOL_ORDER << PAGE_SHIFT)
#define SCFS_MEMPOOL_SIZE_TOTAL		(SCFS_MEMPOOL_COUNT * SCFS_MEMPOOL_SIZE)

/* misc */
#define SCFS_IO_MAX_RETRY		10
#define IS_POW2(n)			(n != 0 && ((n & (n - 1)) == 0))
#define EMPTY_FLAG			-1

/* read performance tuning stuff */
#define MAX_BUFFER_CACHE		(SCFS_MEMPOOL_COUNT / 2)
#define SCFS_ASYNC_READ_PAGES
#define SCFS_READ_PAGES_PROFILE
#if (defined(SCFS_READ_PAGES_PROFILE) && defined(SCFS_ASYNC_READ_PAGES))
#define SCFS_ASYNC_READ_PROFILE
#endif

#ifdef SCFS_ASYNC_READ_PAGES
//#define SCFS_SMB_THREAD_CPU_AFFINITY
#define MAX_PAGE_BUFFER_SIZE_SMB	2048
/* read helper thread wakeup threshold : 2 ~ 4 smb_thread each in order */
#define SMB_THREAD_THRESHOLD_2		MAX_PAGE_BUFFER_SIZE_SMB / 64	// 32 pages
#define SMB_THREAD_THRESHOLD_3		MAX_PAGE_BUFFER_SIZE_SMB / 32	// 64 pages
#define SMB_THREAD_THRESHOLD_4		MAX_PAGE_BUFFER_SIZE_SMB / 16 	// 128 pages
#endif
 
//#define SCFS_NOTIFY_RANDOM_READ
//#define SCFS_REMOVE_NO_COMPRESSED_UPPER_MEMCPY

/* This flag is the switch to go to ver. 1.3 */
//#define SCFS_MULTI_THREAD_COMPRESSION		

/* using crypto module and multi-thread-compression are exclusive */
#ifdef CONFIG_SCFS_USE_CRYPTO
#undef SCFS_MULTI_THREAD_COMPRESSION
#endif

#ifdef SCFS_MULTI_THREAD_COMPRESSION
#define SMTC_THREAD_THRESHOLD_2		4	// # of clusters
#define SMTC_THREAD_THRESHOLD_3		8	// # of clusters
#define SMTC_THREAD_THRESHOLD_4		16	// # of clusters
#define SMTC_PENDING_THRESHOLD		8000
#endif
 

/*******************************/
/* compression & cluster stuff */
/*******************************/
struct scfs_compressor {
	int compr_type;
	const char *name;
	struct crypto_comp *cc;
	struct mutex *comp_mutex;
	struct mutex *decomp_mutex;
	const char *capi_name;
};

enum comp_type {
	SCFS_COMP_NONE = 0,
	SCFS_COMP_LZO,
	SCFS_COMP_ZLIB,
	SCFS_COMP_BZIP2,
	SCFS_COMP_FASTLZO,
	SCFS_COMP_TOTAL_TYPES,
};

struct scfs_cinfo
{
	__u32 offset; // byte offset of the start of this cluster
	__u32 size;	// byte length of the cluster
};

/*****************************************************************/
/* footer = scfs_cinfo * n_cluster + comp_footer (@ end of file) */
/*****************************************************************/
struct comp_footer
{
	__s32 footer_size;
	__s32 cluster_size;
	__s64 original_file_size;
	__s32 comp_type;
	__s32 magic;
};

/***************************************/
/* fs-wide structs (mount, inode, etc. */
/***************************************/

enum scfs_mode {
	SM_LowInval,		/* Lower page cache invalidation */
	__NR_SCFSMODE,
};

struct read_buffer_cache {
	struct page *u_page;
	struct page *c_page;
	int ino;
	int clust_num;
	int is_compressed;
	atomic_t is_used;
};

struct scfs_mount_options
{
	int flags;
	int cluster_size;
	int comp_threshold;
	enum comp_type comp_type;
};

struct scfs_sb_info
{
 	struct super_block *lower_sb;
	struct scfs_mount_options options;
	struct backing_dev_info bdi;
	mempool_t *mempool;
	/* for free (lower) space check */
	atomic_t current_file_count;	/* opened files */
	atomic_t total_cluster_count;	/* total clusters to be written to lower */
	atomic64_t current_data_size;	/* total data size in memory */

#if MAX_BUFFER_CACHE
	struct read_buffer_cache buffer_cache[MAX_BUFFER_CACHE];
	spinlock_t buffer_cache_lock;
	int read_buffer_index;
#endif

#ifndef CONFIG_SCFS_USE_CRYPTO
	void *scfs_workdata;
	spinlock_t workdata_lock;
#endif

#ifdef CONFIG_DEBUG_FS
	struct dentry *scfs_debugfs_root;
#endif

#ifdef SCFS_ASYNC_READ_PAGES
	atomic_t scfs_standby_readpage_count;
	u64 scfs_readpage_total_count;
	u64 scfs_readpage_io_count;
	u64 scfs_lowerpage_total_count;
	u64 scfs_lowerpage_reclaim_count;
	u64 scfs_lowerpage_alloc_count;
	u64 scfs_op_mode;
	u64 scfs_sequential_page_number;
	u64 buffer_cache_overflow_count_smb;
	u64 buffer_cache_reclaimed_before_used_count;

	/* when page_buffer_smb and file_buffer_smb is full, then this filling_index is
	set to MAX_PAGE_BUFFER_SIZE */
	u32 page_buffer_next_filling_index_smb;
	u32 page_buffer_next_io_index_smb;
	u32 max_page_buffer_size_smb;
	spinlock_t spinlock_smb;

	/* helper threads to process multiple read I/Os simultaneously */
	struct task_struct *smb_task[NR_CPUS];
	int smb_task_status[NR_CPUS];

	/* memory and file buffer for scfs_readpages, for queueing read requests */
	struct page *page_buffer_smb[MAX_PAGE_BUFFER_SIZE_SMB];
	struct file *file_buffer_smb[MAX_PAGE_BUFFER_SIZE_SMB];
#endif

#ifdef SCFS_MULTI_THREAD_COMPRESSION
	struct list_head sii_list;
	spinlock_t sii_list_lock;
	int cbm_list_total_count;
	/* helper threads to process multiple write I/Os simultaneously */
	struct task_struct *smtc_task[NR_CPUS];
	struct kmem_cache *scfs_cbm_cache;
	void *smtc_workdata[NR_CPUS];
	atomic_t smtc_idx;
	/* helper threads to write compressed cluster buffer */
	struct task_struct *smtc_writer_task;
	int smtc_writer_task_status;
#endif

#ifdef SCFS_PROFILE_MEM
	atomic_t mempooled_size;
	atomic_t kmalloced_size;
	atomic_t vmalloced_size;
	atomic_t kmcached_size;
#endif
};

struct cinfo_entry
{
	struct list_head entry;
	struct scfs_cinfo cinfo;
	unsigned int current_cluster_idx;
	int pad;
};

struct scfs_cluster_buffer
{
	struct page *u_page;
	struct page *c_page;
	char *u_buffer;
	char *c_buffer;
	size_t original_size;
};

#ifdef SCFS_MULTI_THREAD_COMPRESSION
struct scfs_cluster_buffer_mtc
{
	struct scfs_cluster_buffer entry;
	struct cinfo_entry *info_entry;
	struct list_head list;
	int is_compress_write_done;
};
#endif

struct scfs_inode_info 
{
	int flags;
	struct mutex lower_file_mutex;
	struct mutex cinfo_mutex;
	atomic_t lower_file_count;
	struct inode *lower_inode;
	void *cinfo_array;
	int cinfo_array_size;
	int cluster_size;
	enum comp_type comp_type;
	size_t upper_file_size;
 	struct scfs_cluster_buffer cluster_buffer;
 	struct list_head cinfo_list;
	unsigned char compressed;
#ifdef SCFS_MULTI_THREAD_COMPRESSION
	struct list_head cbm_list;
	struct list_head *cbm_list_comp;	/* cbm to compress */
	struct list_head *cbm_list_write;	/* cbm to write */
	int cbm_list_comp_count;		/* the number of cbm to compress */
	int cbm_list_write_count;		/* the number of cbm to write */
	struct list_head mtc_list;
	int is_inserted_to_sii_list;
#endif
 	struct inode vfs_inode;
	/* DO NOT ADD FIELDS BELOW vfs_inode */
};

struct scfs_file_info
{
	struct file *lower_file;
};

struct scfs_dentry_info
{
	spinlock_t lock;
	struct path lower_path;
};


/**************************/
/* macro helper functions */
/**************************/

#define SCFS_S(sb)	((struct scfs_sb_info *)(sb->s_fs_info))
#define SCFS_I(inode)	(container_of(inode, struct scfs_inode_info, vfs_inode))
#define SCFS_F(file)	((struct scfs_file_info *)(file->private_data))
#define SCFS_D(dent)	((struct scfs_dentry_info *)(dent->d_fsdata))

#define CF_SIZE			sizeof(struct comp_footer)
#define IS_COMPRESSABLE(sii)	(sii->flags & SCFS_DATA_COMPRESSABLE)
#define IS_INVALID_META(sii)	(sii->flags & SCFS_INVALID_META)	
#define IS_WROPENED(sii)	(sii->flags & SCFS_WRITE_OPENED)
#define MAKE_META_INVALID(sii)	(sii->flags |= SCFS_INVALID_META)
#define MAKE_WROPENED(sii)	(sii->flags |= SCFS_WRITE_OPENED)
#define CLEAR_META_INVALID(sii)	(sii->flags &= ~SCFS_INVALID_META)
#define CLEAR_WROPENED(sii)	(sii->flags &= ~SCFS_WRITE_OPENED)

#define CLUSTER_COUNT(sii) \
	DIV_ROUND_UP_ULL(i_size_read(&sii->vfs_inode), sii->cluster_size)

#define PGOFF_IN_CLUSTER(page, sii) \
	(page->index % (sii->cluster_size / PAGE_SIZE))

#define IS_CLUSTER_EXIST(sii, idx) \
	(sii->cinfo_array_size != 0 && \
	(idx+1) * sizeof(struct scfs_cinfo) <= sii->cinfo_array_size)

#define PAGE_TO_CLUSTER_INDEX(page, sii) \
	((page->index) / (sii->cluster_size / PAGE_SIZE))

#define ASSERT(x) { \
	if (!(x)) { \
		printk(KERN_ERR "assertion %s failed: file %s line %d\n", #x,\
			__FILE__, __LINE__);\
		BUG(); \
	}\
}

#ifdef SCFS_DEBUG
#define SCFS_PRINT(fmt, arg...) \
        scfs_printk(KERN_INFO "[SCFS] %s: " fmt, __func__, ## arg)

#define SCFS_PRINT_WARN(fmt, arg...) \
        scfs_printk(KERN_WARNING "[SCFS] WARN %s: " fmt, __func__, ## arg)

#define SCFS_DEBUG_START	SCFS_PRINT("_start_\n")
#define SCFS_DEBUG_END		SCFS_PRINT("_end_\n")
#else
#define SCFS_PRINT(fmt, arg...)

#define SCFS_PRINT_WARN(fmt, arg...)

#define SCFS_DEBUG_START
#define SCFS_DEBUG_END
#endif

#define SCFS_PRINT_ERROR(fmt, arg...) \
        scfs_printk(KERN_ERR "[SCFS] ERROR %s(%d): " fmt, __func__, __LINE__, ## arg)

#define SCFS_PRINT_ALWAYS(fmt, arg...) \
        scfs_printk(KERN_ERR "[SCFS] %s(%d): " fmt, __func__,__LINE__, ## arg)

#define list_to_page(head) (list_entry((head)->prev, struct page, lru))

/********************/
/* inline functions */
/********************/
#ifdef SCFS_PROFILE
static struct timeval start_time;
static struct timeval end_time;

static inline void
scfs_start_profile(struct timeval *start_time)
{	
	do_gettimeofday(start_time);
}

static inline void
scfs_end_profile(struct timeval *end_time)
{
	do_gettimeofday(end_time);
}

#define SCFS_START_PROFILE(start_time) \
	scfs_start_profile(start_time);

#define SCFS_END_PROFILE(start, end, target) \
	scfs_end_profile(end); \
	*target += timeval_compare(end, start);
	
//	printk(KERN_ERR "%s in %s(%d): %lld.%06lld seconds elapsed\n",fmt,__FUNCTION__, __LINE__ 
//			, (long long)(end_time.tv_sec - start_time.tv_sec), (long long)(end_time.tv_usec - start_time.tv_usec));

#else
#define SCFS_START_PROFILE() 
#define SCFS_END_PROFILE(fmt)
static inline void scfs_start_profile(void) { }
static inline void scfs_end_profile(void) { }
#endif

static inline struct dentry *scfs_lower_dentry(struct dentry *dentry)
{
	return ((struct scfs_dentry_info *)dentry->d_fsdata)->lower_path.dentry;
}

static inline void scfs_set_lower_dentry(struct dentry *dentry,
	struct dentry *lower_dentry)
{
	((struct scfs_dentry_info*)dentry->d_fsdata)->lower_path.dentry = lower_dentry;
}

static inline struct inode *scfs_lower_inode(const struct inode *i)
{
	return SCFS_I(i)->lower_inode;
}

static inline void scfs_set_lower_inode(struct inode *i, struct inode *val)
{
	SCFS_I(i)->lower_inode = val;
}

static inline struct file *scfs_lower_file(const struct file *f)
{
	return SCFS_F(f)->lower_file;
}

static inline void scfs_set_lower_file(struct file *f, struct file *val)
{
	SCFS_F(f)->lower_file = val;
}

static inline struct super_block *scfs_lower_super(const struct super_block *sb)
{
	return SCFS_S(sb)->lower_sb;
}

static inline void scfs_set_lower_super(struct super_block *sb,
	struct super_block *val)
{
	SCFS_S(sb)->lower_sb = val;
}

static inline struct vfsmount *scfs_dentry_to_lower_mnt(struct dentry *dentry)
{
	return ((struct scfs_dentry_info *)dentry->d_fsdata)->lower_path.mnt;
}

static inline void scfs_set_dentry_lower_mnt(struct dentry *dentry,
	struct vfsmount *lower_mnt)
{
	((struct scfs_dentry_info *)dentry->d_fsdata)->lower_path.mnt = lower_mnt;
}

#ifdef SCFS_PROFILE_MEM
static inline void profile_add_mempooled(int size, struct scfs_sb_info *sbi)
{
	atomic_add(size, &sbi->mempooled_size);
}

static inline void profile_add_kmalloced(int size, struct scfs_sb_info *sbi)
{
	atomic_add(size, &sbi->kmalloced_size);
}

static inline void profile_add_vmalloced(int size, struct scfs_sb_info *sbi)
{
	atomic_add(size, &sbi->vmalloced_size);
}

static inline void profile_add_kmcached(int size, struct scfs_sb_info *sbi)
{
	atomic_add(size, &sbi->kmcached_size);
}

static inline void profile_sub_mempooled(int size, struct scfs_sb_info *sbi)
{
	atomic_sub(size, &sbi->mempooled_size);
}

static inline void profile_sub_kmalloced(int size, struct scfs_sb_info *sbi)
{
	atomic_add(size, &sbi->kmalloced_size);
}

static inline void profile_sub_vmalloced(int size, struct scfs_sb_info *sbi)
{
	atomic_sub(size, &sbi->vmalloced_size);
}

static inline void profile_sub_kmcached(int size, struct scfs_sb_info *sbi)
{
	atomic_sub(size, &sbi->kmcached_size);
}
#else
static inline void profile_add_mempooled(int size, struct scfs_sb_info *sbi) {}
static inline void profile_add_kmalloced(int size, struct scfs_sb_info *sbi) {}
static inline void profile_add_vmalloced(int size, struct scfs_sb_info *sbi) {}
static inline void profile_add_kmcached(int size, struct scfs_sb_info *sbi) {}
static inline void profile_sub_mempooled(int size, struct scfs_sb_info *sbi) {}
static inline void profile_sub_kmalloced(int size, struct scfs_sb_info *sbi) {}
static inline void profile_sub_vmalloced(int size, struct scfs_sb_info *sbi) {}
static inline void profile_sub_kmcached(int size, struct scfs_sb_info *sbi) {}
#endif

/***********************/
/* function prototypes */
/***********************/

void scfs_printk(const char *fmt, ...);

int __init scfs_init_kthread(void);

void scfs_destroy_kthread(void);

int scfs_init_kmem_caches(void);

void scfs_free_kmem_caches(void);

int scfs_parse_options(struct scfs_sb_info *sbi, char *options);

struct inode *scfs_get_inode(struct inode *lower_inode, struct super_block *sb);

void copy_mount_flags_to_inode_flags(struct inode *inode, struct super_block *sb);

int scfs_get_lower_file(struct dentry *dentry, struct inode *inode, int flags);

void scfs_put_lower_file(struct inode *inode);

int get_cluster_info(struct file *file, int cluster_n, struct scfs_cinfo *target);

int get_cluster_info_from_list(struct inode *inode, int cluster_n,
		struct scfs_cinfo *target);

int scfs_truncate(struct dentry *dentry, loff_t size);

int scfs_read_cluster(struct file *file, struct page *page, char *buf_c,
	char **buf_u, int *compressed);

int scfs_decompress(enum comp_type algo, char *buf_c, char *buf_u,
	size_t len, size_t *actual);

int scfs_compress(enum comp_type algo, char *buf_c, char *buf_u, size_t len,
	size_t *actual, void *workdata, struct scfs_sb_info *sbi);

struct page *scfs_alloc_mempool_buffer(struct scfs_sb_info *sbi);

void scfs_free_mempool_buffer(struct page *p, struct scfs_sb_info *sbi);

struct scfs_cluster_buffer *scfs_get_cluster_buffer(struct page *page,
	struct scfs_inode_info *sii, struct file *file);

int scfs_get_comp_buffer(struct scfs_inode_info *sii);

struct scfs_cluster_buffer *scfs_alloc_cluster_buffer(unsigned int cluster_idx,
	struct scfs_inode_info *sii);

int scfs_check_space(struct scfs_sb_info *sbi, struct dentry *dentry);

void sync_page_to_buffer(struct page *page, char *buffer);

void sync_page_from_buffer(struct page *page, char *buffer);

int scfs_check_ready_to_comp(struct scfs_cluster_buffer *cluster_buffer);

int scfs_check_ready_to_write(struct scfs_cluster_buffer *cluster_buffer,
	struct scfs_inode_info *sii);

void scfs_free_cluster_buffer(struct scfs_cluster_buffer *cluster_buffer);

unsigned int scfs_get_lower_offset(unsigned int cluster_index,
	struct scfs_inode_info *sii);

int scfs_get_cluster_from_lower(struct file *lower_file, struct scfs_cinfo clust_info);

struct cinfo_entry *scfs_alloc_cinfo_entry(unsigned int cluster_index,
	struct scfs_inode_info *sii);

int scfs_write_pending_cluster_buffer(struct scfs_cluster_buffer *cluster_buffer,
	struct file *lower_file, struct scfs_inode_info *sii);

int scfs_write_cinfo(struct scfs_inode_info *sii, struct file *lower_file, loff_t *pos);

int scfs_write_meta(struct file *file);

ssize_t scfs_getxattr_lower(struct dentry *lower_dentry, const char *name,
			void *value, size_t size);

int scfs_footer_read(struct inode *inode, struct file *lower_file);

int scfs_reload_meta(struct file *file);

int scfs_initialize_file(struct dentry *scfs_dentry, struct inode *scfs_inode);

ssize_t scfs_lower_read(struct file *file, char *buf, size_t count, loff_t *pos);

ssize_t scfs_lower_write(struct file *file, char *buf, size_t count, loff_t *pos);

void *scfs_cinfo_alloc(struct scfs_inode_info *sii, unsigned long size);

void scfs_cinfo_free(struct scfs_inode_info *sii, const void *addr);

int scfs_initialize_lower_file(struct dentry *dentry, struct file **lower_file, int flags);

int scfs_make_header(struct file *lower_file, struct inode *scfs_inode);

int scfs_load_cinfo(struct scfs_inode_info *sii, struct file *lower_file);

int scfs_check_cinfo(struct scfs_inode_info *sii, void *buf);

#ifdef SCFS_ASYNC_READ_PAGES
void wakeup_smb_thread(struct scfs_sb_info *sbi);
int smb_init(struct scfs_sb_info *sbi);
void smb_destroy(struct scfs_sb_info *sbi);
int smb_thread(void *data);
#endif

#ifdef SCFS_MULTI_THREAD_COMPRESSION
extern void wakeup_smtc_thread(struct scfs_sb_info *sb_info);
extern int smtc_init(struct scfs_sb_info *sbi);
extern void smtc_destroy(struct scfs_sb_info *sbi);
extern int smtc_thread(void *info);
extern int scfs_compress_cluster(struct scfs_inode_info *sii,
	struct scfs_cluster_buffer_mtc *cbm, void *workdata);
//extern int scfs_write_compress_all_cluster(struct scfs_inode_info *sii);
extern int scfs_write_compress_all_cluster(struct scfs_inode_info *sii,
	struct file *lower_file);
extern int scfs_write_one_compress_cluster(struct scfs_inode_info *sii,
	struct scfs_cluster_buffer_mtc *cbm);
extern int smtc_writer_thread(void *info);
extern void wakeup_smtc_writer_thread(struct scfs_sb_info *sb_info);
#endif

/* compressor.c */
int scfs_compressors_init(void);
void scfs_compressors_exit(void);
int scfs_compress_crypto(const void *in_buf, size_t in_len, void *out_buf, size_t *out_len,
		    int compr_type);
int scfs_decompress_crypto(const void *buf, size_t len, void *out, size_t *out_len,
		     int compr_type);

 
#endif //SCFS_HEADER_H
