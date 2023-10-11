#include <linux/init.h>
#include <linux/module.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <uapi/linux/poll.h>
#include <linux/ioctl.h>
#include <linux/sysfs.h>
#include <linux/kobject.h>
#include <linux/err.h>
#include <linux/interrupt.h>
#include <linux/gpio.h>
#include <linux/platform_device.h>

#define WRITE_VALUE _IOW('a','a',int)
#define READ_VALUE _IOR('a','b',int)

#define DYNAMIC_ALLC 1

static dev_t first;
static struct class *chrdevcls;

static struct cdev *chrdev;

/************************************************************/

static irqreturn_t irq_handler(int irq, void *data)
{	                                                                     //Interrrupt handler
       	printk(KERN_INFO "Interrupt Occurred");
	return IRQ_HANDLED;
}

//---------------------------------------

struct kobject *kobj_ref;

int sys_value = 0;

int value = 0;

/********************************************/

static ssize_t sysfs_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf);

static ssize_t sysfs_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count);

struct kobj_attribute sys_attr = __ATTR(sys_value, 0660, sysfs_show, sysfs_store);

static ssize_t sysfs_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	pr_info("sysfs read \n");
	return sprintf(buf, "%d",sys_value);
}

static ssize_t sysfs_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count){
	pr_info("sys write \n");
	sscanf(buf, "%d", &sys_value);
	return count;
}
/*****************************************************/

static int chrdev_open(struct inode *node, struct file *filp)                                   //open file
{
	printk(KERN_INFO "device file is opened \n");
	return 0;
}
static int chrdev_release(struct inode *node, struct file *filp)                                //release
{
        printk(KERN_INFO "device file is closed \n");
        return 0;
}

static ssize_t chrdev_read(struct file *filp, char __user *buf, size_t len, loff_t *offset)      //read
{
	char data[64] = "hello world\n";
	printk(KERN_INFO "Read from device file \n");
	if(copy_to_user(buf,data,12)==0){
		return 12;
	}
	return -1;
}
static ssize_t chrdev_write(struct file *filp, const char __user *buf, size_t len, loff_t *offset)   //write
{
	char data[64] = {0};
	if(copy_from_user(data,buf,len)==0){
        printk(KERN_INFO "write to the device file : %s \n",data);
        return len;
	}
	return -1;
}

static __poll_t chrdev_poll(struct file *file, struct poll_table_struct *pt)                     //poll
{
        return POLLIN | POLLOUT; 
}

static long chrdev_ioctl(struct file *file, unsigned int command, unsigned long arg)
{
	switch(command){
		case WRITE_VALUE:
			if(copy_from_user(&value,(int *)arg, sizeof(value)))
			{
				pr_err("Data Write : Err!\n");
			}
			printk(KERN_INFO "value = %d\n",value);
			break;
		case READ_VALUE:
			if(copy_to_user((int *)arg, &value, sizeof(value)))
			{
				pr_err("Data Read : Err!\n");
			}
			break;
		default:
			printk(KERN_INFO "Default \n");
			break;
	}
	return 0;
}

/************ file operation structure *********************/

static struct file_operations chrdev_fops = {
        .owner = THIS_MODULE,
        .open = chrdev_open,
        .release = chrdev_release,
        .read = chrdev_read,
        .write = chrdev_write,
	.poll = chrdev_poll,
	.unlocked_ioctl = chrdev_ioctl,
};


static int chardrv_init(struct platform_device *pdev)
{
	int ret;
#ifdef DYNAMIC_ALLC
	ret = alloc_chrdev_region(&first,0,1,"chardevice");
#else
	first = MKDEV(240,0);
	ret = register_chrdev_region(first,1,"chardevice");
#endif
	if(ret < 0){
		printk(KERN_INFO "register for chardevice is failed : %d\n",ret);
		return ret; }
	printk(KERN_DEBUG "register for chardevice region is success \n");

	chrdevcls = class_create(THIS_MODULE, "chardevice");

	if(chrdevcls == NULL){
		printk(KERN_INFO "class creation is failed \n");
		goto cleanupcls;
	}

	if(device_create(chrdevcls,NULL,first,NULL,"chardevice")==NULL){
		printk(KERN_INFO "Device creation is failed \n");
		goto cleanupdevice;
	}

	chrdev = cdev_alloc();
	if(chrdev == NULL){
		printk(KERN_INFO "cdev allocation is failed \n");
		goto cleanup_device; }

	cdev_init(chrdev,&chrdev_fops);
	cdev_add(chrdev, first, 1);

	kobj_ref = kobject_create_and_add("demo_sysfs",kernel_kobj);

	if(sysfs_create_file(kobj_ref, &sys_attr.attr))
	{
		pr_err("cannot create sys file \n");
		goto cleanup_sys;
	}
     
        if(gpio_is_valid(33)==false){                                 //checking the gpio is valid or not
		pr_err("error : gpio is not valid \n");
		goto cleanup_device;
	}

	if(gpio_request(33,"GPIO 33 input") < 0)                      //requesting the GPIO
	{
                pr_err("error : The gpio is not requested \n");
	        goto cleanup_gpio; 
	}

	gpio_direction_input(33);

	if(request_irq(gpio_to_irq(33),(void *)irq_handler, IRQF_TRIGGER_FALLING,"chardevice",NULL))
	{
		printk(KERN_INFO "cannot register IRQ \n");
		goto cleanup_irq;
	}

	pr_info("Device driver insert has done \n");

	return 0;
cleanup_gpio:
        gpio_free(33);
cleanup_irq:
	gpio_free(33);
	free_irq(gpio_to_irq(33),NULL);
        kobject_put(kobj_ref);
        sysfs_remove_file(kernel_kobj, &sys_attr.attr);
cleanup_sys:
	kobject_put(kobj_ref);
	sysfs_remove_file(kernel_kobj, &sys_attr.attr);
cleanup_device:
        device_destroy(chrdevcls,first);
cleanupcls:
	class_destroy(chrdevcls);
cleanupdevice:
	unregister_chrdev_region(first,1);
	return -1;

}

static void chardrv_exit(struct platform_device *pdev)
{
	printk(KERN_INFO "The module removed successfully \n");
	free_irq(gpio_to_irq(33), NULL);
        kobject_put(kobj_ref); 
        sysfs_remove_file(kernel_kobj, &sys_attr.attr);
	cdev_del(chrdev);
	device_destroy(chrdevcls,first);
	class_destroy(chrdevcls);
	unregister_chrdev_region(first,1);
	return;
}
/*Function to probe the device */
static int sample_platform_driver_probe(struct platform_device *pdev)              
{
	printk(KERN_INFO "platform driver probe is called \n");

         return	chardrv_init(pdev);
}

static int sample_platform_driver_remove(struct platform_device *pdev)
{
	printk(KERN_INFO "Platform driver remove is called \n");
	chardrv_exit(pdev);
	return 0;
}

/*Function to match the driver with the device in the Device Tree */

static const struct of_device_id sample_dt_ids[] =
{
	{.compatible = "ldd-sample"},
	{},
};

/*platform driver structure */
static struct platform_driver sample_platform_driver = {
	.driver ={
		.name = "ldd-sample",
		.of_match_table = of_match_ptr(sample_dt_ids),
	},
	.probe = sample_platform_driver_probe,
	.remove = sample_platform_driver_remove,
};

/*Module initialization*/

module_platform_driver(sample_platform_driver);

//module_init(chardrv_init);
//module_exit(chardrv_exit);

MODULE_LICENSE("GPL");
