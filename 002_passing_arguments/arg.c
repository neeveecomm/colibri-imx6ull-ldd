

#include<linux/init.h>
#include<linux/module.h>

int buffer = 0;
int data[4];

/************************* Module Init function***************************/

static int __init helloworld_init(void)
{
   pr_info("The data inside the buffer : %d \n",buffer);
   for(int i =0; i<4;i++){
	   printk(KERN_INFO "The data[%d]= %d \n",i,data[i]);}
		   return 0;
}

/*********************** Module exit function ***************************/

static void __exit helloworld_cleanup(void)
{
        pr_info("good bye world\n");
        printk(kERN_INFO "The data removed from the buffer : %d \n",buffer);
	  for(int i =0; i<4;i++){
          printk(KERN_INFO "The data[%d]= %d \n",i,data[i]);}

}

module_param(buffer, int, 0644);                       // initialize the arguments
module_param_array(data,int,NULL,0644);                 

module_init(helloworld_init);
module_exit(helloworld_cleanup);

MODULE_LICENSE("GPL");


