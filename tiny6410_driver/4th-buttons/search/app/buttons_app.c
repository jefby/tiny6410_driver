/*
 *
 *
 *	buttons的用户程序：配合驱动程序使用
 *	Author:jefby
 *	Email:jef199006@gmail.com
 *
 *
 * */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>


int main(int argc,char **argv)
{
	int i;
	int ret;
	int fd;
	unsigned char keyval[8]={1,1,1,1,1,1,1,1};
	static int cnt = 0;

	//打开设备
	fd = open("/dev/buttons",0);
	if(fd < 0){
		printf("can't open /dev/buttons\n");
		return -1;
	}
	while(1){
		for(i=0;i<8;++i){
			ret = read(fd,keyval,sizeof(keyval));
			if(ret < 0){
				printf("read err.\n");
				return -1;
			}
			if(keyval[i] == 0){
				printf("%d:KEY%d entered.\n",cnt++,i+1);
			}
		}//for
	}//while(1)
	close(fd);
	return 0;
}

