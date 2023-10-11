obj-m := plat_driver.o
KER_DIR=/media/neeveecomm/data1/deepak/rootfsDemo/lib/modules/6.1.0/build
all:
	$(MAKE) ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- -C $(KER_DIR) M=$(PWD) modules
clean:
	$(MAKE) ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- -C $(KER_DIR) M=$(PWD) clean

