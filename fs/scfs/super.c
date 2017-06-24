/*
 * fs/scfs/super.c
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

#include <linux/export.h>
#include <linux/statfs.h>
#include <linux/seq_file.h>
#include <linux/genhd.h>
#include <linux/module.h>
#include "scfs.h"
#include "../mount.h"
#include <linux/lzo.h>

#define SCFS_VERSION "1.2.19"

#if MAX_BUFFER_CACHE
//extern struct read_buffer_cache buffer_cache[];
//extern spinlock_t buffer_cache_lock;
#endif

#ifdef CONFIG_DEBUG_FS
int scfs_mounted = 1;
#endif

static struct kobject *scfs_kobj;
static const char * scfs_version = SCFS_VERSION;
extern const char *tfm_names[SCFS_COMP_TOTAL_TYPES];

static ssize_t version_show(struct kobject *kobj,
			    struct kobj_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, scfs_version);
}

static struct kobj_attribute version_attr = __ATTR_RO(version);

#ifdef CONFIG_SYSTEM_COMPRESSED
static const char * system_type = "scfs\n";
#else
static const char * system_type = "ext4\n";
#endif

static ssize_t system_type_show(struct kobject *kobj,
			    struct kobj_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, system_type);
}

static struct kobj_attribute system_type_attr = __ATTR_RO(system_type);

static const char * supported_comp_types = "lzo"
#if 0 //#ifdef CONFIG_CRYPTO_DEFLATE
	",bzip2"
#endif
#ifdef CONFIG_CRYPTO_ZLIB
	",zlib"
#endif
#ifdef CONFIG_CRYPTO_FASTLZO
	",fastlzo"
#endif
	"\n";

static ssize_t supported_comp_types_show(struct kobject *kobj,
			    struct kobj_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, supported_comp_types);
}

static struct kobj_attribute supported_comp_types_attr = __ATTR_RO(supported_comp_types);

static struct attribute *attributes[] = {
	&system_type_attr.attr,
	&version_attr.attr,
	&supported_comp_types_attr.attr,
	NULL,
};

static struct attribute_group attr_group = {
	.attrs = attributes,
};

static struct inode *scfs_alloc_inode(struct super_block *sb)
{
	struct scfs_inode_info *sii;

	sii = kmem_cache_alloc(scfs_inode_info_cache, GFP_KERNEL);
	if (!sii)
		return NULL;

	profile_add_kmcached(sizeof(struct scfs_inode_info), SCFS_S(sb));
	memset(sii, 0, offsetof(struct scfs_inode_info, vfs_inode));
	sii->vfs_inode.i_version = 1;
	if (SCFS_S(sb)->options.flags & SCFS_DATA_COMPRESSABLE)
		sii->flags |= SCFS_DATA_COMPRESSABLE;

	mutex_init(&sii->lower_file_mutex);
	mutex_init(&sii->cinfo_mutex);
	INIT_LIST_HEAD(&sii->cinfo_list);

#ifdef SCFS_MULTI_THREAD_COMPRESSION
	INIT_LIST_HEAD(&sii->cbm_list);
	sii->cbm_list_comp_count = 0;
	sii->cbm_list_write_count = 0;
	sii->is_inserted_to_sii_list = 0;
	sii->cbm_list_comp = NULL;
#endif
	
	return &sii->vfs_inode;
}

static void scfs_destroy_inode(struct inode *inode)
{
	struct scfs_sb_info *sbi = SCFS_S(inode->i_sb);
	struct scfs_inode_info *sii = SCFS_I(inode);

	if (sii->cinfo_array)
		scfs_cinfo_free(sii, sii->cinfo_array);

	if (!list_empty(&sii->cinfo_list)) {
		SCFS_PRINT_ERROR("cinfo list is not empty!\n");
		ASSERT(0);
	}

	if (sii->cluster_buffer.c_page) {
		__free_pages(sii->cluster_buffer.c_page, SCFS_MEMPOOL_ORDER + 1);
		sii->cluster_buffer.c_buffer = NULL;
		sii->cluster_buffer.c_page = NULL;
	} 

	if (sii->cluster_buffer.u_page) {
		__free_pages(sii->cluster_buffer.u_page, SCFS_MEMPOOL_ORDER + 1);
		sii->cluster_buffer.u_buffer = NULL;
		sii->cluster_buffer.u_page = NULL;
		atomic_sub(1, &sbi->current_file_count);
	}

	kmem_cache_free(scfs_inode_info_cache, sii);
	profile_sub_kmcached(sizeof(struct scfs_inode_info), SCFS_S(inode->i_sb));
}

static void scfs_evict_inode(struct inode *inode)
{
	struct inode *lower_inode;
	struct scfs_inode_info *sii = SCFS_I(inode);

	truncate_inode_pages(&inode->i_data, 0);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,10,0)
	clear_inode(inode);
#else
	end_writeback(inode);
#endif
	/* to conserve memory, evicted inode will throw out the cluster info */
	if (sii->cinfo_array) {
		scfs_cinfo_free(sii, sii->cinfo_array);
		sii->cinfo_array = NULL;
	}
	if (!list_empty(&sii->cinfo_list))
		SCFS_PRINT_ERROR("cinfo list is not empty!\n");

	lower_inode = scfs_lower_inode(inode);
	scfs_set_lower_inode(inode, NULL);
	iput(lower_inode);
}

static int scfs_statfs(struct dentry *dentry, struct kstatfs *buf)
{
	int ret;
	struct dentry *lower_dentry = scfs_lower_dentry(dentry);

	ret = lower_dentry->d_sb->s_op->statfs(lower_dentry, buf);
	if (ret)
		return ret;

	buf->f_type = SCFS_SUPER_MAGIC;

	return ret;
}

static int scfs_show_options(struct seq_file *m, struct dentry *root)
{
	struct scfs_sb_info *sbi = SCFS_S(root->d_sb);
	struct scfs_mount_options *opts = &sbi->options;

	if (!(opts->flags & SCFS_DATA_COMPRESSABLE))
		seq_printf(m, ",nocomp");
	if (opts->cluster_size)
		seq_printf(m, ",cluster_size=%u", opts->cluster_size);
	if (opts->comp_threshold >= 0)
		seq_printf(m, ",comp_threshold=%u", opts->comp_threshold);

	switch (opts->comp_type) {
	case SCFS_COMP_LZO:
		seq_printf(m, ",comp_type=lzo");
		break;
	case SCFS_COMP_BZIP2:
		seq_printf(m, ",comp_type=bzip2");
		break;
	case SCFS_COMP_ZLIB:
		seq_printf(m, ",comp_type=zlib");
		break;
	case SCFS_COMP_FASTLZO:
		seq_printf(m, ",comp_type=fastlzo");
		break;
	default:
		break;
	}

	return 0;
}

#ifdef CONFIG_DEBUG_FS
static int lower_path_show(struct seq_file *s, void *unused)
{
	seq_printf(s, "%s\n", (char *)(s->private));

	return 0;
}

static int lower_path_open(struct inode *inode, struct file *file)
{
	return single_open(file, lower_path_show, inode->i_private);
}

static const struct file_operations lower_path_ops = {
	.open		= lower_path_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};

/*
 * XXX : An error handling in this function is necessary?
 *
 */
static int scfs_debugfs_init(struct super_block *sb)
{
	struct scfs_sb_info *sbi = SCFS_S(sb);
	struct dentry *debugfs_root;
	struct scfs_dentry_info *d_info = (struct scfs_dentry_info *)(sb->s_root->d_fsdata);
	char root_name[100];

	if (!debugfs_initialized())
		return -ENODEV;

	memset(root_name, 0, 100);
	sprintf(root_name, "scfs_%d", scfs_mounted);	
	debugfs_root = debugfs_create_dir(root_name, NULL);
	if (!debugfs_root)
		return PTR_ERR(debugfs_root);

	sbi->scfs_debugfs_root = debugfs_root;
	if (!debugfs_create_file("lower_path", S_IRUGO, debugfs_root,
		(void *)(&d_info->lower_path.dentry->d_name.name), &lower_path_ops))
		goto fail;

#ifdef SCFS_PROFILE_MEM
	debugfs_create_atomic_t("scfs_kmalloced", S_IRUGO,
		debugfs_root, &sbi->kmalloced_size);
	debugfs_create_atomic_t("scfs_vmalloced", S_IRUGO,
		debugfs_root, &sbi->vmalloced_size);
	debugfs_create_atomic_t("scfs_mempooled_size", S_IRUGO,
		debugfs_root, &sbi->mempooled_size);
	debugfs_create_atomic_t("scfs_kmcached_size", S_IRUGO,
		debugfs_root, &sbi->kmcached_size);
#endif

#ifdef SCFS_ASYNC_READ_PAGES
	debugfs_create_u64("scfs_readpage_total_count", S_IRUGO,
		debugfs_root, &sbi->scfs_readpage_total_count);

	debugfs_create_u64("scfs_readpage_io_count", S_IRUGO,
		debugfs_root, &sbi->scfs_readpage_io_count);

	debugfs_create_u64("scfs_lowerpage_total_count", S_IRUGO,
		debugfs_root, &sbi->scfs_lowerpage_total_count);

	debugfs_create_u64("scfs_lowerpage_reclaim_count", S_IRUGO,
		debugfs_root, &sbi->scfs_lowerpage_reclaim_count);

	debugfs_create_u64("scfs_lowerpage_alloc_count", S_IRUGO,
		debugfs_root, &sbi->scfs_lowerpage_alloc_count);

	debugfs_create_u64("scfs_op_mode", S_IRUGO,
		debugfs_root, &sbi->scfs_op_mode);

	debugfs_create_u64("scfs_sequential_page_number", S_IRUGO | S_IWUGO,
		debugfs_root, &sbi->scfs_sequential_page_number);

	debugfs_create_u64("buffer_cache_overflow_count_smb", S_IRUGO,
		debugfs_root, &sbi->buffer_cache_overflow_count_smb);

	debugfs_create_u64("buffer_cache_reclaimed_before_used_count", S_IRUGO,
		debugfs_root, &sbi->buffer_cache_reclaimed_before_used_count);

	debugfs_create_atomic_t("scfs_standby_readpage_count", S_IRUGO,
		debugfs_root, &sbi->scfs_standby_readpage_count);

	debugfs_create_u32("page_buffer_next_filling_index_smb", S_IRUGO,
		debugfs_root, &sbi->page_buffer_next_filling_index_smb);

	debugfs_create_u32("page_buffer_next_io_index_smb", S_IRUGO,
		debugfs_root, &sbi->page_buffer_next_io_index_smb);

	debugfs_create_u32("max_page_buffer_size_smb", S_IRUGO,
		debugfs_root, &sbi->max_page_buffer_size_smb);

#ifdef CONFIG_SCFS_LOWER_PAGECACHE_INVALIDATION
	sbi->scfs_op_mode |= (1 << SM_LowInval);
#endif
#endif
	return 0;
fail:
	debugfs_remove_recursive(debugfs_root);
	return -ENOMEM;
}

static void scfs_debugfs_exit(struct scfs_sb_info *sbi)
{
	if (!sbi)
		return;

	debugfs_remove_recursive(sbi->scfs_debugfs_root);
}
#else
static inline int scfs_debugfs_init(struct super_block *sb)
{
	return 0;
}

static inline void scfs_debugfs_exit(struct scfs_sb_info *sbi) { }
#endif


static struct dentry *scfs_mount(struct file_system_type *fs_type, int flags,
				const char *dev_name, void *raw_data)
{
	struct super_block *sb;
	struct scfs_sb_info *sbi;
	struct scfs_dentry_info *root_info;
	struct inode *inode;
	struct path path;
	int ret, i;

	sbi = kzalloc(sizeof(struct scfs_sb_info), GFP_KERNEL);
	if (!sbi) {
		ret = -ENOMEM;
		return ERR_PTR(ret);
	}
	profile_add_kmalloced(sizeof(struct scfs_sb_info), sbi);

	/* set default values */
	sbi->options.flags |= SCFS_DATA_COMPRESSABLE;
	sbi->options.comp_threshold = 75;
	ret = scfs_parse_options(sbi, raw_data);
	if (ret) {
		goto out_free;
	}

	if (!sbi->options.cluster_size)
		sbi->options.cluster_size = SCFS_CLUSTER_SIZE_DEF;
	if (!sbi->options.comp_type)
		sbi->options.comp_type = SCFS_COMP_LZO;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,10,0)
	sb = sget(fs_type, NULL, set_anon_super, flags, NULL);
#else
	sb = sget(fs_type, NULL, set_anon_super, NULL);
#endif
	if (IS_ERR(sb)) {
		goto out_free;
	}

	ret = bdi_setup_and_register(&sbi->bdi, "scfs", BDI_CAP_MAP_COPY);
	if (ret)
		goto out_free;

	sb->s_bdi = &sbi->bdi;
	sb->s_bdi->ra_pages = VM_MAX_READAHEAD * 1024 / PAGE_CACHE_SIZE;
	sb->s_fs_info = sbi;

	sb->s_op = &scfs_sops;
	sb->s_d_op = &scfs_dops;
	
	ret= kern_path(dev_name, LOOKUP_FOLLOW | LOOKUP_DIRECTORY, &path);
	if (ret) {
		goto out_deactivate;
	}

	scfs_set_lower_super(sb, path.dentry->d_sb);
	sb->s_maxbytes = path.dentry->d_sb->s_maxbytes;
	sb->s_blocksize = path.dentry->d_sb->s_blocksize;
	sb->s_magic = SCFS_SUPER_MAGIC;

	inode = scfs_get_inode(path.dentry->d_inode, sb);
	if (IS_ERR(inode)) {
		goto out_pathput;
	}

	sb->s_root = d_make_root(inode);
	if (!sb->s_root) {
		ret = -ENOMEM;
		goto out_pathput;
	}
		
	root_info = kmem_cache_zalloc(scfs_dentry_info_cache, GFP_KERNEL);
	if (!root_info)
		goto out_pathput;

	profile_add_kmcached(sizeof(struct scfs_dentry_info), sbi);

	/* After these setting, when dput() is called with upper dentry
	 * that with lower dentry is also called in deactivate_super().
	 * So path_put() shouldn't be called from here */
	sb->s_root->d_fsdata = root_info;
	scfs_set_lower_dentry(sb->s_root, path.dentry);
	scfs_set_dentry_lower_mnt(sb->s_root, path.mnt);

	/* shared mempool for read/write cluster buffers. */
	sbi->mempool = mempool_create_page_pool(SCFS_MEMPOOL_COUNT, 
		SCFS_MEMPOOL_ORDER);
	if (!sbi->mempool) {
		SCFS_PRINT_ERROR("mempool create failed!\n");
		ret = -ENOMEM;
		goto out_deactivate;
	}

#if MAX_BUFFER_CACHE
	/* initialize read buffers */	
	for (i = 0; i < MAX_BUFFER_CACHE; i++) {
		sbi->buffer_cache[i].u_page = scfs_alloc_mempool_buffer(sbi);
		sbi->buffer_cache[i].c_page = scfs_alloc_mempool_buffer(sbi);
		if (sbi->buffer_cache[i].u_page == NULL ||
				sbi->buffer_cache[i].c_page == NULL) {
			SCFS_PRINT_ERROR("mempool alloc failed!\n");
			ret = -ENOMEM;
			goto out_deactivate;
		}
		sbi->buffer_cache[i].ino = -1;
		sbi->buffer_cache[i].clust_num = -1;
		sbi->buffer_cache[i].is_compressed = -1;
		atomic_set(&sbi->buffer_cache[i].is_used, -1);
	}
	spin_lock_init(&sbi->buffer_cache_lock);
#endif

#ifndef CONFIG_SCFS_USE_CRYPTO
	switch (sbi->options.comp_type) {
	case SCFS_COMP_LZO:
		sbi->scfs_workdata = vmalloc(LZO1X_MEM_COMPRESS);
		if (!sbi->scfs_workdata) {
			SCFS_PRINT_ERROR("vmalloc for lzo workmem failed, "
					"len %d\n", LZO1X_MEM_COMPRESS);
			ret = -ENOMEM;
			goto out_deactivate;
		}
		break;
	default:
		break;
	}
	spin_lock_init(&sbi->workdata_lock);
#endif

#ifdef SCFS_ASYNC_READ_PAGES
	spin_lock_init(&sbi->spinlock_smb);
	ret = smb_init(sbi);
	if (ret) {
		SCFS_PRINT_ERROR("failed to init scfs memory buffering\n");
		goto out_workdata;
	}
	sbi->max_page_buffer_size_smb = MAX_PAGE_BUFFER_SIZE_SMB;
#endif

#ifdef SCFS_MULTI_THREAD_COMPRESSION
	INIT_LIST_HEAD(&sbi->sii_list);
	spin_lock_init(&sbi->sii_list_lock);
	sbi->cbm_list_total_count = 0;

	ret = smtc_init(sbi);
	if (ret) {
		SCFS_PRINT_ERROR("failed to init scfs multi-thread compression\n");
		goto out_workdata;
	}
#endif

	ret = scfs_debugfs_init(sb);
	if (ret) {
		SCFS_PRINT_ERROR("failed to scfs_debugfs_init\n");
		goto out_workdata;
	}

	SCFS_PRINT_ALWAYS("v.%s\n (mempool:%dKBx%d,dev:%s,comp:%s)\n", 
		scfs_version,
		SCFS_MEMPOOL_SIZE / 1024, SCFS_MEMPOOL_COUNT,
		dev_name, tfm_names[sbi->options.comp_type]);

	scfs_mounted++;
	sb->s_flags |= MS_ACTIVE;
	return dget(sb->s_root);

out_pathput:
	path_put(&path);	
out_workdata:
	vfree(sbi->scfs_workdata);
out_deactivate:
	deactivate_locked_super(sb);
out_free:
	kfree(sbi);
	
	return ERR_PTR(ret);
}

static void scfs_kill_block_super(struct super_block *sb)
{
	struct scfs_sb_info *sbi = SCFS_S(sb);

	kill_anon_super(sb);
	if (!sbi)
		return;

#ifdef SCFS_MULTI_THREAD_COMPRESSION
	smtc_destroy(sbi);
#endif

#ifdef SCFS_ASYNC_READ_PAGES
	smb_destroy(sbi);
#endif

#ifdef CONFIG_SCFS_USE_CRYPTO
	if (sbi->scfs_workdata)
		vfree(sbi->scfs_workdata);
#endif

	if (sbi->mempool)
		mempool_destroy(sbi->mempool);

	scfs_debugfs_exit(sbi);
	bdi_destroy(&sbi->bdi);
	kmem_cache_free(scfs_sb_info_cache, sbi);
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,10,0)
static int scfs_d_revalidate(struct dentry *dentry, unsigned int flags)
{
	struct dentry *lower_dentry;
	struct vfsmount *lower_mnt;
	int ret = 1;

	if (flags & LOOKUP_RCU)
		return -ECHILD;

	lower_dentry = scfs_lower_dentry(dentry);
	lower_mnt = scfs_dentry_to_lower_mnt(dentry);
	if (!lower_dentry->d_op || !lower_dentry->d_op->d_revalidate)
		goto out;

	ret = lower_dentry->d_op->d_revalidate(lower_dentry, flags);

	if (dentry->d_inode) {
		struct inode *lower_inode =
			scfs_lower_inode(dentry->d_inode);

		fsstack_copy_attr_all(dentry->d_inode, lower_inode);
	}
out:
	return ret;
}
#else
static int scfs_d_revalidate(struct dentry *dentry, struct nameidata *nd)
{
	struct dentry *lower_dentry;
	struct vfsmount *lower_mnt;
	struct dentry *dentry_save = NULL;
	struct vfsmount *vfsmount_save = NULL;
	int ret = 1;

	if (nd && nd->flags & LOOKUP_RCU)
		return -ECHILD;

	lower_dentry = scfs_lower_dentry(dentry);
	lower_mnt = scfs_dentry_to_lower_mnt(dentry);
	if (!lower_dentry->d_op || !lower_dentry->d_op->d_revalidate)
		goto out;
	if (nd) {
		dentry_save = nd->path.dentry;
		vfsmount_save = nd->path.mnt;
		nd->path.dentry = lower_dentry;
		nd->path.mnt = lower_mnt;
	}
	ret = lower_dentry->d_op->d_revalidate(lower_dentry, nd);
	if (nd) {
		nd->path.dentry = dentry_save;
		nd->path.mnt = vfsmount_save;
	}
	if (dentry->d_inode) {
		struct inode *lower_inode =
			scfs_lower_inode(dentry->d_inode);

		fsstack_copy_attr_all(dentry->d_inode, lower_inode);
	}
out:
	return ret;
}
#endif

static void scfs_d_release(struct dentry *dentry)
{
	if (SCFS_D(dentry)) {
		if (scfs_lower_dentry(dentry)) {
			dput(scfs_lower_dentry(dentry));
			mntput(scfs_dentry_to_lower_mnt(dentry));
		}
		kmem_cache_free(scfs_dentry_info_cache,
				SCFS_D(dentry));

		profile_sub_kmcached(sizeof(struct scfs_dentry_info),
			SCFS_S(dentry->d_sb));
	}
	return;
}

static int scfs_remount_fs(struct super_block *sb, int *flags, char *options)
{
	struct super_block *lower_sb = SCFS_S(sb)->lower_sb;
	struct mount *mnt;
	struct dentry *dentry;
	char *dev_name = NULL, *dir_name;
	int start = PATH_MAX - 1;
	int ret = -EINVAL;
	
	dir_name = kmalloc(PATH_MAX, GFP_KERNEL);
	if (!dir_name)
		return -ENOMEM;

	if (!(*flags & MS_RDONLY))
		set_device_ro(lower_sb->s_bdev, 0);

	dir_name[start] = '\0';
	mnt = list_first_entry(&lower_sb->s_mounts, struct mount, mnt_instance);	
	if (mnt) {
		dev_name = kstrdup(mnt->mnt_devname, GFP_KERNEL);
		dentry = mnt->mnt_mountpoint;
		while (dentry->d_parent != dentry) {
			start -= dentry->d_name.len;
			if (start < 0) {
				kfree(dir_name);
				return -EINVAL;
			}

			memcpy(dir_name + start, dentry->d_name.name, dentry->d_name.len); 
			dir_name[--start] = '/';
			dentry = dentry->d_parent;
			if (dentry->d_parent == dentry && mnt->mnt_parent &&
				mnt->mnt_parent != mnt) {
				dentry = mnt->mnt_parent->mnt_mountpoint;
				mnt = mnt->mnt_parent;
			}
		}
		ret = do_mount(dev_name, dir_name + start, lower_sb->s_id,
			*flags | MS_REMOUNT, options);
	} else
		SCFS_PRINT_ERROR("remount fail, missing mnt\n");

	if (dev_name)
		kfree(dev_name);

	kfree(dir_name);
	return ret;
}

static struct file_system_type scfs_fs_type = {
	.owner		= THIS_MODULE,
	.name		= "scfs",
	.mount		= scfs_mount,
	.kill_sb	= scfs_kill_block_super, /* TODO */
	.fs_flags	= 0 /* TODO */
};

const struct super_operations scfs_sops = {
	 .alloc_inode		= scfs_alloc_inode,
	 .destroy_inode		= scfs_destroy_inode,
	 .evict_inode		= scfs_evict_inode,
	 .statfs		= scfs_statfs,
	 .remount_fs		= scfs_remount_fs,
	 .show_options		= scfs_show_options,
 };

 const struct dentry_operations scfs_dops = {
	.d_revalidate = scfs_d_revalidate,
	.d_release = scfs_d_release,
 };

/*************************/
/* sysfs & init routines */
/*************************/
static int do_sysfs_registration(void)
{
	int ret;

	scfs_kobj = kobject_create_and_add("scfs", fs_kobj);
	if (!scfs_kobj) {
		SCFS_PRINT_ERROR("unable to create scfs kset\n");
		ret = -ENOMEM;
		goto out;
	}
	ret = sysfs_create_group(scfs_kobj, &attr_group);
	if (ret) {
		SCFS_PRINT_ERROR("unable to create scfs version attributes\n");
		kobject_put(scfs_kobj);
	}

out:
	return ret;
}

static void do_sysfs_unregistration(void)
{
	sysfs_remove_group(scfs_kobj, &attr_group);
	kobject_put(scfs_kobj);
}

static int __init scfs_init(void)
{
	int ret = 0;

	ret = scfs_init_kmem_caches();
	if (ret) {
		SCFS_PRINT_ERROR("kmem_cache init failed\n");
		goto out;
	}

	ret = scfs_compressors_init();
	if (ret) {
		SCFS_PRINT_ERROR("compressor initialization was failed.\n");
		goto out_free_kmem_caches;
	}

	ret = do_sysfs_registration();
	if (ret) {
		SCFS_PRINT_ERROR("sysfs registration failed\n");
		goto out_do_scfs_compressor_exit;
	}

/*
	ret = scfs_init_kthread();
	if (ret) {
		SCFS_PRINT_ERROR("kthread initialization failed; "
		       "ret = [%d]\n", ret);
		goto out_do_sysfs_unregistration;
	}
	*/

	ret = register_filesystem(&scfs_fs_type);
	if (ret) {
		SCFS_PRINT_ERROR("failed to register filesystem\n");
		//goto out_free_kmem_caches;
		goto out_do_sysfs_unregistration;
	}
	goto out;
/*
out_destroy_kthread:
	scfs_destroy_kthread();
	*/
out_do_sysfs_unregistration:
	do_sysfs_unregistration();
out_do_scfs_compressor_exit:
	scfs_compressors_exit();
out_free_kmem_caches:
	scfs_free_kmem_caches();
out:
	return ret;
}

static void __exit scfs_exit(void)
{
	//scfs_destroy_kthread();
	do_sysfs_unregistration();
	unregister_filesystem(&scfs_fs_type);
	scfs_compressors_exit();
	scfs_free_kmem_caches();
}

/* late_initcall to let compressors initialize first */
late_initcall(scfs_init);
module_exit(scfs_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("System Memory Lab");
MODULE_DESCRIPTION("SCFS Module");
MODULE_VERSION(SCFS_VERSION);
