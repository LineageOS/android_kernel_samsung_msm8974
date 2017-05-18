#
# Makefile for the Linux 2.6 eCryptfs
#

ifeq ($(CONFIG_ECRYPT_FS_VIRTUAL_FAT_XATTR),y)
EXTRA_CFLAGS += -DECRYPT_FS_VIRTUAL_FAT_XATTR
endif

obj-$(CONFIG_ECRYPT_FS) += ecryptfs.o

ecryptfs-objs := dentry.o file.o inode.o main.o super.o mmap.o read_write.o crypto.o keystore.o messaging.o miscdev.o kthread.o debug.o
