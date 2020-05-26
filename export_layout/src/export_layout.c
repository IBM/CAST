/***************************************************************************
 *
 * Copyright IBM Corporation 2016,2017. All Rights Reserved
 *
 * This file is part of the CORAL export_layout kernel module.
 *
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions 
 * are met:
 *
 *  1. Redistributions of source code must retain the above copyright notice, 
 *     this list of conditions and the following disclaimer. 
 *  2. Redistributions in binary form must reproduce the above copyright 
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution. 
 *  3. The name of International Business Machines may not be used to endorse 
 *     or promote products derived from this software without specific prior 
 *     written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS "AS IS" AND ANY 
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, 
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR 
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR 
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, 
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, 
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, 
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR 
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF 
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Alternatively, provided that this notice is retained in full, this
 * software may be distributed under the terms of the GNU General
 * Public License ("GPL") as published by the Free Software Foundation;
 * either version 2 of the License, or (at your option) any later version,
 * in which case the provisions of the GPL apply INSTEAD OF those given above.
 *
 ****************************************************************************/

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/file.h>
#include <linux/exportfs.h>
#include <linux/slab.h>
#include <linux/version.h>
#include <linux/mutex.h>
#include <linux/sched.h>

#include "../include/export_layout.h"

#if (LINUX_VERSION_CODE >=  KERNEL_VERSION(4,8,0))
#include <linux/iomap.h>
#endif

#if (LINUX_VERSION_CODE >=  KERNEL_VERSION(4,18,0))
#define EXP_VERSION "1.8.1.2.R8"
#else
#define EXP_VERSION "1.8.1.2.R7"
#endif 

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Bryan Rosenburg");
MODULE_DESCRIPTION("Provides access to block layout functionality");
MODULE_VERSION(EXP_VERSION);

static int export_layout_debug = 0;
static int export_layout_callback_debug = 1;
module_param(export_layout_debug, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(export_layout_debug, "  Debug level (default 0=off).");
module_param(export_layout_callback_debug, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(export_layout_callback_debug, "  Debug level 1=on (default) 0=off");

static int export_layout_open(struct inode *, struct file *);
static int export_layout_release(struct inode *, struct file *);
static long export_layout_ioctl(struct file *, unsigned int, unsigned long);

static struct file_operations export_layout_ops = {
	.open = export_layout_open,
	.release = export_layout_release,
	.unlocked_ioctl = export_layout_ioctl,
};

#define DEVICE "export_layout"
#define CLASS "export_layout"

static int Major = -1;
static struct class *Class = NULL;
static struct device *Device = NULL;
static DEFINE_MUTEX(export_layout_lock);

struct transfer_info {
	int fd;
	u32 writing;
	u64 offset;
	u64 length;
	struct file *target;
	u32 iomap_count;
	u32 iomap_count_max;
	struct iomap *iomap;
};

// The VFS map_blocks() interface returns block device addresses in terms
// of 512-byte blocks, independent of the sector size of the device.
#define BASIC_BLOCK_SIZE 512

static int __init export_layout_init(void)
{
	int rc, major;
	struct class *class;
	struct device *device;

	major = register_chrdev(0, DEVICE, &export_layout_ops);
	if (major < 0) {
		printk(KERN_ALERT "%s: register_chrdev failed\n", __func__);
		rc = major;
		goto error;
	}
	Major = major;

	class = class_create(THIS_MODULE, CLASS);
	if (IS_ERR(class)) {
		printk(KERN_ALERT "%s: class_create failed\n", __func__);
		rc = PTR_ERR(class);
		goto error_unregister_device;
	}
	Class = class;

	device = device_create(Class, NULL, MKDEV(Major, 0), NULL, DEVICE);
	if (IS_ERR(device)) {
		printk(KERN_ALERT "%s: device_create failed\n", __func__);
		rc = PTR_ERR(device);
		goto error_destroy_class;
	}
	Device = device;

#if (LINUX_VERSION_CODE >=  KERNEL_VERSION(4,18,0))    
	printk(KERN_INFO
	       "%s: export_layout module major %d version %s  \n",
	       __func__,  Major, EXP_VERSION);
#else
    printk(KERN_INFO
	       "%s: export_layout module (built %s %s)  major %d version %s  \n",
	       __func__, __DATE__, __TIME__, Major, EXP_VERSION);
#endif 		   

	return 0;

 error_destroy_class:
	class_destroy(Class);
	Class = NULL;
 error_unregister_device:
	unregister_chrdev(Major, DEVICE);
	Major = -1;
 error:
	return rc;
}

static void __exit export_layout_exit(void)
{
	printk(KERN_INFO "%s: cleaning up\n", __func__);
	device_destroy(Class, MKDEV(Major, 0));
	Device = NULL;
	class_destroy(Class);
	Class = NULL;
	unregister_chrdev(Major, DEVICE);
	Major = -1;
}

#if (LINUX_VERSION_CODE >=  KERNEL_VERSION(3,18,0))
static bool export_layout_callback(struct file_lock *fl)
#else
static void export_layout_callback(struct file_lock *fl)
#endif
{
	if (export_layout_callback_debug){
		long int fd = (long int)fl->fl_owner;
		printk(KERN_DEBUG "%s: fl_pid mainpid=%d fd=%ld file=%p\n", __func__, fl->fl_pid, fd, fl->fl_file);
	}
		

	fl->fl_break_time = 0;	// Don't let the lease timeout.
#if (LINUX_VERSION_CODE >=  KERNEL_VERSION(3,18,0))
	return false;
#endif
}

static const struct lock_manager_operations export_layout_lm_ops = {
	.lm_break = export_layout_callback,
	.lm_change = lease_modify,
};

static int export_layout_set_callback(struct transfer_info *t)
{
	struct file_lock *fl;
	int rc;

	fl = locks_alloc_lock();
	if (!fl)
		return -ENOMEM;

	locks_init_lock(fl);
	fl->fl_lmops = &export_layout_lm_ops;
	fl->fl_flags = FL_LAYOUT;
	fl->fl_type = F_RDLCK;
	fl->fl_end = OFFSET_MAX;
	fl->fl_owner = (fl_owner_t)(long) t->fd;
	fl->fl_pid = current->tgid;  //thread group id
	fl->fl_file = t->target;

	rc = vfs_setlease(t->target, F_RDLCK, &fl, NULL);
	if (rc) {
		locks_free_lock(fl);
		return rc;
	}

	return 0;
}

static void export_layout_clear_callback(struct transfer_info *t)
{
	(void)vfs_setlease(t->target, F_UNLCK, NULL, (void **)&t);
}

static int export_layout_open(struct inode *inodep, struct file *filep)
{
	struct transfer_info *t;
	if (export_layout_debug>1)
		printk(KERN_DEBUG "%s: inode %p, file %p\n", __func__, inodep,
		       filep);
	t = kzalloc(sizeof(struct transfer_info), GFP_KERNEL);
	if (!t)
		return -ENOMEM;
	filep->private_data = t;
        try_module_get(THIS_MODULE);
	return 0;
}

static int export_layout_release(struct inode *inodep, struct file *filep)
{
	struct transfer_info *t;
	if (export_layout_debug>2)
		printk(KERN_DEBUG "%s: inode %p, file %p\n", __func__, inodep,
		       filep);

	mutex_lock(&export_layout_lock);
	t = (struct transfer_info *)filep->private_data;
	filep->private_data = NULL;
	mutex_unlock(&export_layout_lock);
	
	if (t->target) {
		if (export_layout_debug){
    	    printk(KERN_DEBUG "%s: completed fd=%d, file=%p name=%pd usecount=%ld pid=%d mainpid=%d\n",
		       __func__, t->fd,
		       t->target, t->target->f_path.dentry,atomic_long_read(&t->target->f_count),current->pid,current->tgid);
		}
		export_layout_clear_callback(t);
		kfree(t->iomap);
		t->iomap = NULL;
		fput(t->target);
		t->target = NULL;
	}else{
		if (export_layout_debug)
    	    printk(KERN_DEBUG "%s: completed fd=%d pid=%d mainpid=%d\n", __func__, t->fd, current->pid,current->tgid);
	}
	kfree(t);
        module_put(THIS_MODULE);
	return 0;
}

static long transfer_setup(struct file *filep,
			   unsigned int cmd, unsigned long p)
{
	struct transfer_info *t;
	struct export_layout_transfer_setup setup;
	struct inode *inode;
	struct export_operations const *ops;
	int rc;
	u32 devgen;
	u32 extent_size;
	struct export_layout_transfer_setup *setup_return;
	int i;
	struct iomap *map;
	u64 offset, length;

    mutex_lock(&export_layout_lock);
	t = (struct transfer_info *)filep->private_data;

	if (!t) {
		printk(KERN_ALERT "%s: close in progress\n", __func__);
		rc = -EINVAL;
		goto error_return_rc;
	}

	if (t->target != NULL) {
		printk(KERN_ALERT "%s: transfer in progress\n", __func__);
		rc = -EINVAL;
		goto error_return_rc;
	}

	if (copy_from_user(&setup, (void __user *)p, sizeof(setup))){
		printk(KERN_ALERT "%s: bad copy_from_user fd=%d file=%p \n", __func__,t->fd,t->target);	 
		rc = -EFAULT;
		goto error_return_rc;
	}

	t->fd = setup.fd;
	t->writing = setup.writing;
	t->offset = setup.offset;
	t->length = setup.length;

	t->target = fget(t->fd);
	if (!t->target){
		printk(KERN_ALERT "%s: bad fget fd=%d file=%p \n", __func__,t->fd,t->target);	 
		rc = -EBADF;
		goto error_return_rc;
	}
	if (export_layout_debug){
    	printk(KERN_DEBUG "%s: proceeding fd=%d, file=%p name=%pd pid=%d mainpid=%d writing=%d offset=0x%llx length=0x%llx setup.extent_count_max=%d\n",
		       __func__, t->fd, t->target, t->target->f_path.dentry,current->pid,current->tgid,t->writing,t->offset, t->length,
		       setup.extent_count_max);
	}

	rc = export_layout_set_callback(t);
	if (rc)
		goto error_release_target;

	inode = file_inode(t->target);
	ops = inode->i_sb->s_export_op;
	if (!ops->map_blocks || !ops->commit_blocks) {
		printk(KERN_ALERT "%s: target file system "
		       "does not support map_blocks/commit_blocks\n", __func__);
		rc = -ENOSYS;
		goto error_clear_callback;
	}

	if (!t->writing) {
		u64 size = i_size_read(inode);
		if (t->offset > size) {
			t->offset = size;
			t->length = 0;
		} else if ((t->offset + t->length) > size) {
			t->length = size - t->offset;
		}
	}

	t->iomap_count_max = 4;
	t->iomap_count = 0;
	t->iomap = (struct iomap *)kzalloc(t->iomap_count_max *
					   sizeof(struct iomap), GFP_KERNEL);
	if (!t->iomap) {
		rc = -ENOMEM;
		goto error_clear_callback;
	}

	offset = t->offset;
	length = t->length;
	while (length > 0) {
		if (t->iomap_count >= t->iomap_count_max) {
			struct iomap *oldmap = t->iomap;
			t->iomap_count_max *= 2;
			t->iomap = (struct iomap *)
			    kzalloc(t->iomap_count_max *
				    sizeof(struct iomap), GFP_KERNEL);
			if (!t->iomap) {
				kfree(oldmap);
				rc = -ENOMEM;
				goto error_clear_callback;
			}
			memcpy(t->iomap, oldmap,
			       t->iomap_count * sizeof(struct iomap));
			kfree(oldmap);
		}
		map = &t->iomap[t->iomap_count];

		rc = ops->map_blocks(inode, offset, length, map, t->writing,
				     &devgen);
		if (rc){
			printk(KERN_DEBUG "%s: rc=map_blocks fd=%d, file=%p name=%pd rc=%d iomap_count=%d offset=%llx length=%llx\n",
		       __func__, t->fd, t->target, t->target->f_path.dentry,rc,t->iomap_count,offset,length);
			goto error_free_iomap;
		}
#if (LINUX_VERSION_CODE >=  KERNEL_VERSION(4,18,0))
        if (map->offset < offset) {
			u64 delta = offset - map->offset;
			map->offset += delta;
			map->addr += (delta / BASIC_BLOCK_SIZE) * BASIC_BLOCK_SIZE;
			map->length -= delta;
		}
#else
		if (map->offset < offset) {
			u64 delta = offset - map->offset;
			map->offset += delta;
			map->blkno += (delta / BASIC_BLOCK_SIZE);
			map->length -= delta;
		}
#endif		


		if (map->length > length) {
			map->length = length;
		}

		offset += map->length;
		length -= map->length;
		// Keep this map only if it contains actual blocks
		if ((map->type == IOMAP_MAPPED) ||
		    (map->type == IOMAP_UNWRITTEN)) {
			t->iomap_count++;
		}
	}

	if (t->iomap_count > setup.extent_count_max) {
		/*
		 * User did not provide enough space. Clean up as if for a
		 * failure, but return the actual number of extents needed
		 * as a positive return value.
		 */
		rc = t->iomap_count;
		goto error_free_iomap;
	}

	extent_size = t->iomap_count * sizeof(struct export_layout_extent);
	setup_return = (struct export_layout_transfer_setup *)
	    kzalloc(sizeof(struct export_layout_transfer_setup) +
		    extent_size, GFP_KERNEL);
	if (!setup_return) {
		rc = -ENOMEM;
		goto error_free_iomap;
	}
	setup_return->extent_count = t->iomap_count;
	for (i = 0; i < t->iomap_count; i++) {
		setup_return->extent[i].file_offset = t->iomap[i].offset;

#if (LINUX_VERSION_CODE >=  KERNEL_VERSION(4,18,0))
        setup_return->extent[i].blkdev_offset =
		    t->iomap[i].addr +
		    (t->iomap[i].offset % BASIC_BLOCK_SIZE);
#else		
		setup_return->extent[i].blkdev_offset =
		    (t->iomap[i].blkno * BASIC_BLOCK_SIZE) +
		    (t->iomap[i].offset % BASIC_BLOCK_SIZE);
#endif			
		setup_return->extent[i].length = t->iomap[i].length;
	}

	if (copy_to_user
	    (&((struct export_layout_transfer_setup *)p)->extent_count,
	     &setup_return->extent_count, sizeof(__u32) + extent_size)) {
		printk(KERN_ALERT "%s: bad copy_to_user fd=%d file=%p \n", __func__,t->fd,t->target);	 
		rc = -EFAULT;
		goto error_free_setup_return;
	}
    if (export_layout_debug){
    	printk(KERN_DEBUG "%s: completed fd=%d, file=%p name=%pd usecount=%ld pid=%d mainpid=%d\n",
		       __func__, t->fd,
		       t->target, t->target->f_path.dentry,atomic_long_read(&t->target->f_count),current->pid,current->tgid);	
	}

	kfree(setup_return);
	mutex_unlock(&export_layout_lock);
	return 0;

 error_free_setup_return:
	kfree(setup_return);
 error_free_iomap:
	kfree(t->iomap);
	t->iomap = NULL;
 error_clear_callback:
	export_layout_clear_callback(t);
 error_release_target:
	fput(t->target);
	t->target = NULL;
error_return_rc:	
	mutex_unlock(&export_layout_lock);
	return rc;
}

static long transfer_finalize(struct file *filep,
			      unsigned int cmd, unsigned long p)
{
	struct transfer_info *t;
	struct export_layout_transfer_finalize final;
	struct inode *inode;
	struct export_operations const *ops;
	int rc=0;
	struct iattr iattr;

    mutex_lock(&export_layout_lock);
	t = (struct transfer_info *)filep->private_data;

	if (!t) {
		printk(KERN_ALERT "%s: close in progress\n", __func__);
		rc = -EINVAL;
		goto finalize_badrc;
	}

	if (t->target == NULL) {
		printk(KERN_ALERT "%s: no transfer in progress\n", __func__);
		rc = -EINVAL;
		goto finalize_badrc;
	}

	if (copy_from_user(&final, (void __user *)p, sizeof(final))) {
		printk(KERN_ALERT "%s: bad copy_from_user fd=%d file=%p \n", __func__,t->fd,t->target);
		rc = -EFAULT;
		goto finalize_badrc;
	}

	if (export_layout_debug){
    	printk(KERN_DEBUG "%s: proceeding fd=%d, file=%p name=%pd usecount=%ld status=%d pid=%d mainpid=%d\n",
		       __func__, t->fd,
		       t->target, t->target->f_path.dentry,atomic_long_read(&t->target->f_count),final.status,current->pid,current->tgid);	
	}		   

	inode = file_inode(t->target);
	ops = inode->i_sb->s_export_op;

	if ((final.status == 0) && t->writing) {
		iattr.ia_atime = current_time(inode);
		iattr.ia_ctime = current_time(inode);
		iattr.ia_mtime = current_time(inode);
		iattr.ia_valid = (ATTR_ATIME | ATTR_CTIME | ATTR_MTIME);
		if ((t->offset + t->length) > i_size_read(inode)) {
			iattr.ia_size = t->offset + t->length;
			iattr.ia_valid |= ATTR_SIZE;
		}
		rc = ops->commit_blocks(inode, t->iomap, t->iomap_count,
					&iattr);
	}

	export_layout_clear_callback(t);
	kfree(t->iomap);
	t->iomap = NULL;
    if (export_layout_debug){
    	printk(KERN_DEBUG "%s: completed rc=%d fd=%d, file=%p name=%pd usecount=%ld pid=%d mainpid=%d\n",
		       __func__, rc, t->fd,
		       t->target, t->target->f_path.dentry,atomic_long_read(&t->target->f_count),current->pid,current->tgid);	
	}
	fput(t->target);
	t->target = NULL;
finalize_badrc:	
    mutex_unlock(&export_layout_lock);
	return rc;
}

static long export_layout_ioctl(struct file *filep,
				unsigned int cmd, unsigned long p)
{
	switch (cmd) {

	case EXPORT_LAYOUT_IOC_TRANSFER_SETUP:
		return transfer_setup(filep, cmd, p);

	case EXPORT_LAYOUT_IOC_TRANSFER_FINALIZE:
		return transfer_finalize(filep, cmd, p);
	}

	if (export_layout_debug)
		printk(KERN_DEBUG "%s: invalid command file %p, cmd 0x%04x, %p\n",
		       __func__, filep, cmd, (void __user *)p);

	return -EINVAL;
}

module_init(export_layout_init);
module_exit(export_layout_exit);
