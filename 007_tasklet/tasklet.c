#include <linux/init.h>
#include <linux/module.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/err.h>
#include <linux/interrupt.h>
#include <linux/gpio.h>

#define DYNAMIC_ALLC 1

static dev_t first;
static struct class *chrdevcls;

static struct cdev *chrdev;

/*****************************************Tasklet*****************/

void tasklet_fun(unsigned long arg)                                      // tasklet function handler
{
	printk(KERN_INFO "Executing Tasklet function %ld", arg);
}

DECLARE_TASKLET(tasklet,NULL);                                             // Declare tasklet

/*********************************************************************/

static irqreturn_t irq_handler(int irq, void *data)
{	                                                                     //Interrrupt handler
       	printk(KERN_INFO "Interrupt Occurred");
  	tasklet_schedule(&tasklet);                                          //scheduling task to tasklet
	return IRQ_HANDLED;
}

int value = 0;

/********************************************/

static int chrdev_open(struct inode *node, struct file *filp)                    //open file
{
	printk(KERN_INFO "device file is opened \n");
	return 0;
}
static int chrdev_release(struct inode *node, struct file *filp)                  //release
{
        printk(KERN_INFO "device file is closed \n");
        return 0;
}

static ssize_t chrdev_read(struct file *filp, char __user *buf, size_t len, loff_t *offset)  //read
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
/********************* file operation structure ******************************/

static struct file_operations chrdev_fops = {
        .owner = THIS_MODULE,
        .open = chrdev_open,
        .release = chrdev_release,
        .read = chrdev_read,
        .write = chrdev_write,
};

/*************************** Module Init function ***************************/

static int chardrv_init(void)
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

	  tasklet_init(&tasklet,tasklet_fun,0);                   // Initialize the tasklet function
     
        if(gpio_is_valid(33)==false){                             //checking the gpio is valid or not
		pr_err("error : gpio is not valid \n");
		goto cleanup_device;
	}

	if(gpio_request(33,"GPIO 33 input") < 0)                  //requesting the GPIO
	{
                pr_err("error : The gpio is not requested \n");
	        goto cleanup_gpio; 
	}

	gpio_direction_input(33);                                  // make gpio direction as input

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
cleanup_device:
        device_destroy(chrdevcls,first);
cleanupcls:
	class_destroy(chrdevcls);
cleanupdevice:
	unregister_chrdev_region(first,1);
	return -1;

}

/******************** Module exit function ************************/

static void chardrv_exit(void)
{
	printk(KERN_INFO "The module removed successfully \n");
	tasklet_kill(&tasklet);
	free_irq(gpio_to_irq(33), NULL);
	cdev_del(chrdev);
	device_destroy(chrdevcls,first);
	class_destroy(chrdevcls);
	unregister_chrdev_region(first,1);
	return;
}

module_init(chardrv_init);
module_exit(chardrv_exit);

MODULE_LICENSE("GPL");
