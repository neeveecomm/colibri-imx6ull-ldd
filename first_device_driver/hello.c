
/* Sample first device driver 
   file - hello.c */

#include<linux/init.h>
#include<linux/module.h>

static int __init hello_init(void)             /* Module Init function */    
{
    printk(KERN_INFO "My first module is inserted\n");
    return 0;
}

static void __exit hello_exit(void)           /* Module exit function */
{
    printk(KERN_INFO "My first module is Removed\n");
}

module_init(hello_init);                      /* To register Init function */
module_exit(hello_exit);                      /* To register exit function */

MODULE_LICENSE("GPL");


