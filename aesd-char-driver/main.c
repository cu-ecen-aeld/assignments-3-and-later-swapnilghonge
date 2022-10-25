/**
 * @file aesdchar.c
 * @brief Functions and data related to the AESD char driver implementation
 *
 * Based on the implementation of the "scull" device driver, found in
 * Linux Device Drivers example code.
 *
 * @author Dan Walkes
 * @date 2019-10-22
 * @copyright Copyright (c) 2019
 *
 */

#include <linux/module.h>
#include <linux/syscalls.h>
#include <linux/init.h>
#include <linux/printk.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/fs.h> // file_operations
#include <asm/uaccess.h>
#include "aesdchar.h"
int aesd_major =   0; // use dynamic major
int aesd_minor =   0;

MODULE_AUTHOR("Swapnil Ghonge");
MODULE_LICENSE("Dual BSD/GPL");

struct aesd_dev aesd_device;

int aesd_open(struct inode *inode, struct file *filp)
{
   
    PDEBUG("open");
    try_module_get(THIS_MODULE);

    return 0;
}

int aesd_release(struct inode *inode, struct file *filp)
{
    
    PDEBUG("release");
    
    module_put(THIS_MODULE);

    return 0;
}

ssize_t aesd_read(struct file *filp, char __user *buf, size_t count,
                loff_t *f_pos)
{
    ssize_t retval = 0;
    size_t  entry_offset = 0;
    
    
    struct aesd_buffer_entry *entry = NULL;
    PDEBUG("read %zu bytes with offset %lld\n",count,*f_pos);
    
    mutex_lock(&aesd_device.lock);
    entry = aesd_circular_buffer_find_entry_offset_for_fpos(&aesd_device.buffer, *f_pos, &entry_offset);
    
    mutex_unlock(&aesd_device.lock);

    return retval;
}

ssize_t aesd_write(struct file *filp, const char __user *buf, size_t count,
                loff_t *f_pos)
{
    ssize_t retval = 0;
    const char *freed = NULL;
    PDEBUG("write %zu bytes with offset %lld\n",count,*f_pos);
    
    mutex_lock(&aesd_device.lock);    
   
    while(count)
    {
        get_user(aesd_device.partial_content[aesd_device.partial_size], &buf[retval]);
        retval++;
        aesd_device.partial_size++;
        count--;

        
        if(aesd_device.partial_content[aesd_device.partial_size - 1] == '\n')
        {
            
            struct aesd_buffer_entry entry;
            entry.buffptr = kmalloc(sizeof(char)*aesd_device.partial_size, GFP_KERNEL);
            
            entry.size = aesd_device.partial_size;

           
            memcpy((void *)entry.buffptr, aesd_device.partial_content, aesd_device.partial_size);

            
            freed = aesd_circular_buffer_add_entry(&aesd_device.buffer, &entry);
            
            if(freed)
            {
                kfree(freed);
                freed = NULL;
            }

            
            kfree(aesd_device.partial_content);
            aesd_device.partial_size = 0;
           
            if(count)
            {
                aesd_device.partial_content = kmalloc(sizeof(char)*count, GFP_KERNEL);
            }
        }
    }

   
    mutex_unlock(&aesd_device.lock);


    return retval;
}
struct file_operations aesd_fops = {
    .owner =    THIS_MODULE,
    .read =     aesd_read,
    .write =    aesd_write,
    .open =     aesd_open,
    .release =  aesd_release,
};

static int aesd_setup_cdev(struct aesd_dev *dev)
{
    int err, devno = MKDEV(aesd_major, aesd_minor);

    cdev_init(&dev->cdev, &aesd_fops);
    dev->cdev.owner = THIS_MODULE;
    dev->cdev.ops = &aesd_fops;
    err = cdev_add (&dev->cdev, devno, 1);
    if (err) {
        printk(KERN_ERR "Error %d adding aesd cdev\n", err);
    }
    return err;
}



int aesd_init_module(void)
{
    dev_t dev = 0;
    int result;
    result = alloc_chrdev_region(&dev, aesd_minor, 1,
            "aesdchar");
    aesd_major = MAJOR(dev);
    
    if (result < 0) {
        printk(KERN_WARNING "Can't get major %d\n", aesd_major);
        return result;
    }
    memset(&aesd_device,0,sizeof(struct aesd_dev));

    
    mutex_init(&aesd_device.lock);
  
  
    result = aesd_setup_cdev(&aesd_device);

    if( result ) 
    {
        unregister_chrdev_region(dev, 1);
    }
    
    
    return result;

}

void aesd_cleanup_module(void)
{
    dev_t devno = MKDEV(aesd_major, aesd_minor);

    cdev_del(&aesd_device.cdev);

    mutex_destroy(&aesd_device.lock);

    unregister_chrdev_region(devno, 1);
}



module_init(aesd_init_module);
module_exit(aesd_cleanup_module);
