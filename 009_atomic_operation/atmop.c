#include <linux/init.h>
#include <linux/module.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include<linux/uaccess.h>
#include <linux/bitops.h>

unsigned long is_open;

static dev_t first;                   // Device number
static struct class *chrdevcls;       // Device class

static struct cdev *chrdev;           // Character device structure



static int chrdev_open(struct inode *node, struct file *filp)                    //open file
{
	printk(KERN_INFO "device file is opened \n");
	if(test_and_set_bit(0, &is_open))                                      // Attempt to acquire the lock
          {
           printk(KERN_INFO "File is already opened \n");
           return -EBUSY;
            }
	return 0;
}
static int chrdev_release(struct inode *node, struct file *filp)                  //release
{
        printk(KERN_INFO "device file is closed \n");
	clear_bit(0, &is_open);                                                  // Release the lock
        return 0;
}

static ssize_t chrdev_read(struct file *filp, char __user *buf, size_t len, loff_t *offset)  //read
{
	char data[64] = "hello world\n";
	printk(KERN_INFO "Read from device file \n");
	if(copy_to_user(buf,data,12)==0){                                        /* copy data from kernel space to user space */
		return 12;
	}
	return -1;
}
static ssize_t chrdev_write(struct file *filp, const char __user *buf, size_t len, loff_t *offset)   //write
{
	char data[64] = {0};
	if(copy_from_user(data,buf,len)==0){                                      /* copy data from user space to kernel space */
        printk(KERN_INFO "write to the device file : %s \n",data);
        return len;
	}
	return -1;
}

/* File operation structure */

static struct file_operations chrdev_fops = {
        .owner = THIS_MODULE,
        .open = chrdev_open,
        .release = chrdev_release,
        .read = chrdev_read,
        .write = chrdev_write,
};

/**************************************Module Init function********************************/

static int chardrv_init(void)
{
	int ret;
#ifdef DYNAMIC_ALLC
	ret = alloc_chrdev_region(&first,0,1,"chardevice");          /*Allocating Major number*/
#else
	first = MKDEV(240,0);                                        /* using fixed device number */
	ret = register_chrdev_region(first,1,"chardevice");          /* Register device number range */
#endif
	if(ret < 0){
		printk(KERN_INFO "register for chardevice is failed : %d\n",ret);
		return ret; }
	printk(KERN_DEBUG "register for chardevice region is success \n");

	chrdevcls = class_create(THIS_MODULE, "chardevice");         /* Creating a Device class */

	if(chrdevcls == NULL){
		printk(KERN_INFO "class creation is failed \n");
		goto cleanupcls;
	}
                                                                        /* creating a device */
	if(device_create(chrdevcls,NULL,first,NULL,"chardevice")==NULL){
		printk(KERN_INFO "Device creation is failed \n");
		goto cleanupdevice;
	}

	chrdev = cdev_alloc();                                        /* Allocate a cdev structure */
	if(chrdev == NULL){
		printk(KERN_INFO "cdev allocation is failed \n");
		goto cleanup_device; }

	cdev_init(chrdev,&chrdev_fops);                              /* Initialize the cdev with the file operation */
	cdev_add(chrdev, first, 1);                                  /* Add cdev to the system */

	pr_info("Device driver insert has done \n");

	return 0;

cleanup_device:
        device_destroy(chrdevcls,first);
cleanupcls:
	class_destroy(chrdevcls);
cleanupdevice:
	unregister_chrdev_region(first,1);
	return -1;

}

/***********************************Module exit function***********************************/

static void chardrv_exit(void)
{
	printk(KERN_INFO "The module removed successfully \n");
	cdev_del(chrdev);
	device_destroy(chrdevcls,first);
	class_destroy(chrdevcls);
	unregister_chrdev_region(first,1);
	return;
}

module_init(chardrv_init);
module_exit(chardrv_exit);

MODULE_LICENSE("GPL");
