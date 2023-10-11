#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <poll.h>
#include <sys/ioctl.h>

char write_buff[1024];
char read_buff[1024];

#define WRITE_VALUE _IOW('a','a',int*)
#define READ_VALUE _IOR('a','b',int*)

int main()
{
	int fd;
	struct pollfd pfd = {0};
	int ret;
	int value ,number;

	fd = open("/dev/chardevice", O_RDWR);

	if(fd<0){
		printf("cannot open the device file ... \n");
		return 0;
	}

	printf("Enter the value to send \n");
	scanf("%d",&number);
	printf("Writing value to Driver \n");

	ioctl(fd, WRITE_VALUE, &number);

	printf("Reading the value from Driver \n");

	ioctl(fd, READ_VALUE, &value);
	printf("value is %d \n",value);

	pfd.fd = fd;
	pfd.events = POLLIN | POLLOUT;

	ret = poll(&pfd,1,1000);

	if(pfd.revents & POLLIN) {
		 printf("Data Reading ...");
                ret = read(fd, read_buff, 1024);
		if (ret > 0){
			printf("Data received : %s",read_buff);
		}
	}
        if(pfd.revents & POLLOUT){
		printf("Enter the string to : ");
                scanf("%s",write_buff);
                ret = write(fd, write_buff, strlen(write_buff)+1);
		if(ret > 0)
                printf("writing done \n");

	}
	close(fd);
}
