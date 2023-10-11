#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

char write_buff[1024];
char read_buff[1024];

int main()
{
	int fd;
	char option;

	int ret;
	int value ,number;

	fd = open("/dev/chardevice", O_RDWR);

	if(fd<0){
		printf("cannot open the device file ... \n");
		return 0;
	}


	while(1){
		printf("\n ---------Enter the options------------\n");
		printf("   [1] write\n   [2] read\n   [3] exit \n");
		scanf("%c",&option);
		printf("option you entered : %c \n",option);

		switch(option){
			case '1':
				printf("Enter the string to : ");
				scanf("%s",write_buff);
			        write(fd, write_buff, strlen(write_buff)+1);
				printf("writing done \n");
				break;
			case '2':
                                printf("Data Reading ...");
                                read(fd, read_buff, 1024);
                                printf("Done!\n\n");
                                printf("Data = %s\n\n", read_buff);
                                break;
                        case '3':
                                close(fd);
                                exit(1);
                                break;
                        default:
                                printf("Enter Valid option = %c\n",option);
                                break;
		} 
	} 
	close(fd);
}
