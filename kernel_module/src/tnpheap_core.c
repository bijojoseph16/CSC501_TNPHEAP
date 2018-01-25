//////////////////////////////////////////////////////////////////////
//                             North Carolina State University
//
//
//
//                             Copyright 2016
//
////////////////////////////////////////////////////////////////////////
//
// This program is free software; you can redistribute it and/or modify it
// under the terms and conditions of the GNU General Public License,
// version 2, as published by the Free Software Foundation.
//
// This program is distributed in the hope it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
// more details.
//
// You should have received a copy of the GNU General Public License along with
// this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
//
////////////////////////////////////////////////////////////////////////
//
//   Author:  Hung-Wei Tseng
//
//   Description:
//     Skeleton of NPHeap Pseudo Device
//
////////////////////////////////////////////////////////////////////////

#include "tnpheap_ioctl.h"

#include <asm/uaccess.h>
#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/mm.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/poll.h>
#include <linux/mutex.h>
#include <linux/time.h>

//my changes
#include <linux/radix-tree.h>

//tree for the version numbers
RADIX_TREE(version_numbers, GFP_ATOMIC);

//keeps track of the transaction number
//don't worry about overflow, and don't check which transactions are still out there
__u64 tx_number = 0;

//mutex to protect incrementing the version numbers in the tree
DEFINE_MUTEX(version_numbers_mutex);

//mutex to protect the transaction number
DEFINE_MUTEX(tx_number_mutex);

struct miscdevice tnpheap_dev;

__u64 tnpheap_get_version(struct tnpheap_cmd __user *user_cmd)
{
    struct tnpheap_cmd cmd;
    __u64 offset;
    __u64 * version_number_p;
    if (copy_from_user(&cmd, user_cmd, sizeof(cmd)))
    {
        return -1 ;
    } 
    //get the required offset
    offset = cmd.offset;
    //search the tree for it, assuming for now that offset is the actual offset, 
    //and not the offset * the page size
    version_number_p = (__u64 *)radix_tree_lookup(&version_numbers, offset);
    if (version_number_p == NULL) {
        return 0;//0 stands for never before seen offset
    }
    return *version_number_p;
}

__u64 tnpheap_start_tx(struct tnpheap_cmd __user *user_cmd)
{
    struct tnpheap_cmd cmd;
    __u64 ret=0;
    if (copy_from_user(&cmd, user_cmd, sizeof(cmd)))
    {
        return -1 ;
    }    
    mutex_lock(&tx_number_mutex);
    ret = tx_number+1;
    tx_number++;
    mutex_unlock(&tx_number_mutex);
    return ret;
}

//return 1 when abort, 0 for success
//still maybe(?) need to add code for mapping the memory and allocating it using npheap
__u64 tnpheap_commit(struct tnpheap_cmd __user *user_cmd)
{
    struct tnpheap_cmd cmd;
    __u64 * version_number;
    __u64 ret=0;
    if (copy_from_user(&cmd, user_cmd, sizeof(cmd)))
    {
        return -1 ;
    }
    //for now assuming that each transaction only contains one object
    //lock the tree, then make sure the version number is the same
    mutex_lock(&version_numbers_mutex);
    version_number = (__u64 *)radix_tree_lookup(&version_numbers, cmd.offset);
    if (version_number == NULL) {
        version_number = (__u64 *) kmalloc(sizeof(__u64), GFP_ATOMIC);
        *version_number = cmd.version;
        radix_tree_insert(&version_numbers, cmd.offset, version_number);
        ret = 0;
    } else {
        (*version_number) = cmd.version;
        ret = 0;
    }
    mutex_unlock(&version_numbers_mutex);
    return ret;
}
__u64 tnpheap_ioctl(struct file *filp, unsigned int cmd,
                                unsigned long arg)
{
    switch (cmd) {
    case TNPHEAP_IOCTL_START_TX:
        return tnpheap_start_tx((void __user *) arg);
    case TNPHEAP_IOCTL_GET_VERSION:
        return tnpheap_get_version((void __user *) arg);
    case TNPHEAP_IOCTL_COMMIT:
        return tnpheap_commit((void __user *) arg);
    default:
        return -ENOTTY;
    }
}

static const struct file_operations tnpheap_fops = {
    .owner                = THIS_MODULE,
    .unlocked_ioctl       = tnpheap_ioctl,
};

struct miscdevice tnpheap_dev = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = "tnpheap",
    .fops = &tnpheap_fops,
};

static int __init tnpheap_module_init(void)
{
    int ret = 0;
    if ((ret = misc_register(&tnpheap_dev)))
        printk(KERN_ERR "Unable to register \"npheap\" misc device\n");
    else
        printk(KERN_ERR "\"npheap\" misc device installed\n");
    return 1;
}

static void __exit tnpheap_module_exit(void)
{
    misc_deregister(&tnpheap_dev);
    return;
}

MODULE_AUTHOR("Hung-Wei Tseng <htseng3@ncsu.edu>");
MODULE_LICENSE("GPL");
MODULE_VERSION("0.1");
module_init(tnpheap_module_init);
module_exit(tnpheap_module_exit);
