/*
 * fs/scfs/file.c
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

#include "scfs.h"

/*
 * Check validity of cinfo data(array).
 * It is called in scfs_open, failed, the file is treated non-compressed,
 * such as the one have no 'footer'.
 */
int scfs_check_cinfo(struct scfs_inode_info *sii, void *buf)
{
	struct scfs_cinfo *cinfo = buf;
	int prev_last_offset = 0;
	int cinfo_size = sii->cinfo_array_size;

	for (cinfo = buf; (unsigned long)cinfo < (unsigned long)buf + cinfo_size; cinfo++) {
		if (cinfo->offset < prev_last_offset || !cinfo->size ||
			cinfo->size > sii->cluster_size) {
			SCFS_PRINT("invalid cinfo, prev_last_offset : %d, "
				"offset : %d, size : %d\n", prev_last_offset,
				cinfo->offset, cinfo->size);
			return -1;
		}
		prev_last_offset = cinfo->offset + cinfo->size;
	}
	return 0;
}

static int scfs_open(struct inode *inode, struct file *file)
{
	struct scfs_sb_info *sbi = SCFS_S(inode->i_sb);
	struct scfs_inode_info *sii = SCFS_I(inode);
	struct scfs_file_info *fi;
	int ret = 0;
	struct file *lower_file;

	if (IS_WROPENED(sii)) {
		SCFS_PRINT("This file is already opened with 'WRITE' flag\n");
		return -EPERM;
	}

	fi = kmem_cache_zalloc(scfs_file_info_cache, GFP_KERNEL);
	if (!fi)
		return -ENOMEM;

	profile_add_kmcached(sizeof(struct scfs_file_info), sbi);

	file->private_data = fi;

	mutex_lock(&sii->cinfo_mutex);
	if (IS_INVALID_META(sii)) {
		SCFS_PRINT("meta is invalid, so we should re-load it\n");
		ret = scfs_reload_meta(file);
		if (ret) {
			SCFS_PRINT_ERROR("error in re-reading footer, err : %d\n", ret);
			goto out;
		}
	} else if (sii->compressed && !sii->cinfo_array) {
		/* 1st lower-open is for getting cinfo */
		ret = scfs_initialize_lower_file(file->f_dentry, &lower_file, O_RDONLY); 
		if (ret) {
			SCFS_PRINT_ERROR("err in get_lower_file %s\n",
				file->f_dentry->d_name.name);
			goto out;
		}
		scfs_set_lower_file(file, lower_file);

		SCFS_PRINT("info size = %d \n", sii->cinfo_array_size);
		ret = scfs_load_cinfo(sii, lower_file);
		if (ret) {
			SCFS_PRINT_ERROR("err in loading cinfo, ret : %d\n",
				file->f_dentry->d_name.name);
			fput(lower_file);
			goto out;
		}
		fput(lower_file);
	}

	ret = scfs_initialize_lower_file(file->f_dentry, &lower_file, file->f_flags); 
	if (ret) {
		SCFS_PRINT_ERROR("err in get_lower_file %s\n",
			file->f_dentry->d_name.name);

		goto out;
	}
	scfs_set_lower_file(file, lower_file);
out:
	if (!ret) {
		fsstack_copy_attr_all(inode, scfs_lower_inode(inode));
		if (file->f_flags & (O_RDWR | O_WRONLY))
			MAKE_WROPENED(sii);
	} else {
		scfs_set_lower_file(file, NULL);
		kmem_cache_free(scfs_file_info_cache, file->private_data);
		profile_sub_kmcached(sizeof(struct scfs_file_info), sbi);
		sii->cinfo_array = NULL;
	}
	mutex_unlock(&sii->cinfo_mutex);
	SCFS_PRINT("lower, dentry name : %s, count : %d, ret : %d\n",
		file->f_dentry->d_name.name, file->f_dentry->d_count, ret);
	
	return ret;
}

/*
 * scfs_file_release
 */
static int scfs_file_release(struct inode *inode, struct file *file)
{
	int ret;

	SCFS_PRINT("f:%s calling fput with lower_file\n",
			file->f_path.dentry->d_name.name);

	if (file->f_flags & (O_RDWR | O_WRONLY)) {
		CLEAR_WROPENED(SCFS_I(inode));
		ret = scfs_write_meta(file);
		if (ret)
			return ret;
	}

	fput(SCFS_F(file)->lower_file);
	kmem_cache_free(scfs_file_info_cache, SCFS_F(file));
	profile_sub_kmcached(sizeof(struct scfs_file_info), SCFS_S(inode->i_sb));

	return 0;
}

/*
 * scfs_readdir
 */
static int scfs_readdir(struct file *file, void *dirent, filldir_t filldir)
{
	struct file *lower_file = NULL;
	struct dentry *dentry = file->f_path.dentry;
	int ret = 0;

	lower_file = scfs_lower_file(file);
	lower_file->f_pos = file->f_pos;
	ret = vfs_readdir(lower_file, filldir, dirent);
	file->f_pos = lower_file->f_pos;
	if (ret >= 0)
		fsstack_copy_attr_atime(dentry->d_inode, lower_file->f_path.dentry->d_inode);

	return ret;
}

/*
 * scfs_unlocked_ioctl
 */
static long scfs_unlocked_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct file *lower_file;
	long ret = -ENOENT;

	lower_file = scfs_lower_file(file);

	if (!lower_file || !lower_file->f_op)
		goto out;
	if (lower_file->f_op->unlocked_ioctl)
		ret = lower_file->f_op->unlocked_ioctl(lower_file, cmd, arg);

out:
	return ret;
}

#ifdef CONFIG_COMPAT
/*
 * scfs_compat_ioctl
 */
static long scfs_compat_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct file *lower_file;
	long ret = -ENOIOCTLCMD;

	lower_file = scfs_lower_file(file);

	if (!lower_file || !lower_file->f_op)
		goto out;
	if (lower_file->f_op->compat_ioctl)
		ret = lower_file->f_op->compat_ioctl(lower_file, cmd, arg);

out:
	return ret;
}
#endif

static int scfs_flush(struct file *file, fl_owner_t id)
{
	struct file *lower_file = NULL;
	int ret = 0;

	lower_file = scfs_lower_file(file);
	if (lower_file && lower_file->f_op && lower_file->f_op->flush)
		ret = lower_file->f_op->flush(lower_file, id);

	return ret;
}

static int scfs_fsync(struct file *file, loff_t start, loff_t end, int datasync)
{
	int ret = 0;

	ret = scfs_write_meta(file);
	if(ret)
		return ret;
#ifdef SCFS_MULTI_THREAD_COMPRESSION
//	scfs_write_compress_all_cluster(SCFS_I(file->f_path.dentry->d_inode));
#endif

	ret = vfs_fsync(scfs_lower_file(file), datasync);

	return ret;
}

static int scfs_fasync(int fd, struct file *file, int flag)
{
	struct file *lower_file = NULL;
	int ret = 0;

	lower_file = scfs_lower_file(file);
	if (lower_file->f_op && lower_file->f_op->fasync)
		ret = lower_file->f_op->fasync(fd, lower_file, flag);

	return ret;
}

static const struct vm_operations_struct scfs_file_vm_ops = {
	.fault		= filemap_fault,
};

/*
 * SCFS doesn't have a writepage, so write with mmap has no effect.
 * First implementation was returning error when having VM_WRITE,
 * but some process in boot sequence uses mmap with VM_WRITE
 * - without write, just with flag - so, now using VM_WRITE is 
 * available.
 */
static int scfs_mmap(struct file *file, struct vm_area_struct *vma)
{
	struct address_space *mapping = file->f_mapping;

	if (!mapping->a_ops->readpage)
		return -ENOEXEC;

	SCFS_PRINT("file %s\n", file->f_path.dentry->d_name.name);	

	//if (file->f_mode & FMODE_WRITE) {
		/*
	if (vma->vm_flags & VM_WRITE) {
		SCFS_PRINT_ERROR("f_mode WRITE was set! error. "
			"f_mode %x (FMODE_READ: %x FMODE_WRITE %x)\n", 
			file->f_mode,
			file->f_mode & FMODE_READ,
			file->f_mode & FMODE_WRITE);
	SCFS_PRINT_ERROR("filename : %s\n", file->f_path.dentry->d_name.name);	
		return -EPERM;
	}
	*/

	file_accessed(file);
	vma->vm_ops = &scfs_file_vm_ops;
#if LINUX_VERSION_CODE < KERNEL_VERSION(3,10,0)
	vma->vm_flags |= VM_CAN_NONLINEAR;
#endif
 	SCFS_PRINT("VM flags: %lx "
 		"EXEC %lx IO %lx "
		"SEQ %lx RAND %lx "
		"READ %lx MAYREAD %lx "
		"WRITE %lx MAYWRITE %lx "
		"SHARED %lx MAYSHARE %lx\n",
		vma->vm_flags,
		vma->vm_flags & VM_EXECUTABLE, vma->vm_flags & VM_IO,
		vma->vm_flags & VM_SEQ_READ, vma->vm_flags & VM_RAND_READ,
		vma->vm_flags & VM_READ, vma->vm_flags & VM_MAYREAD,
		vma->vm_flags & VM_WRITE, vma->vm_flags & VM_MAYWRITE,
		vma->vm_flags & VM_SHARED, vma->vm_flags & VM_MAYSHARE);

	if (vma->vm_flags & VM_WRITE) {
		SCFS_PRINT("VM_WRITE: file %s flags %lx VM_MAYWRITE %lx\n", 
			file->f_path.dentry->d_name.name,
			vma->vm_flags,
			vma->vm_flags & VM_MAYWRITE);
	}

	return 0;
}


/*****************************/
/* file_operations structres */
/*****************************/

const struct file_operations scfs_dir_fops = {
	.llseek		= default_llseek,
	.read		= generic_read_dir,
	.readdir	= scfs_readdir,
	.unlocked_ioctl	= scfs_unlocked_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl	= scfs_compat_ioctl,
#endif
	.open		= scfs_open,
	.release	= scfs_file_release,
	.flush		= scfs_flush,
	.fsync		= scfs_fsync,
	.fasync		= scfs_fasync,
};

const struct file_operations scfs_file_fops = {
	.llseek		= generic_file_llseek,
	.read 		= do_sync_read,
	.aio_read 	= generic_file_aio_read,
	.write 		= do_sync_write,
	.aio_write 	= generic_file_aio_write,
	.unlocked_ioctl	= scfs_unlocked_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl	= scfs_compat_ioctl,
#endif
	.mmap		= scfs_mmap,
	.open 		= scfs_open,
	.release	= scfs_file_release,
	.flush		= scfs_flush,
	.fsync		= scfs_fsync,
	.fasync		= scfs_fasync,
};
