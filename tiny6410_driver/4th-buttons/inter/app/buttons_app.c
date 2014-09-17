/*
 *
 *
 *	 buttons的用户程序：配合驱动程序使用
  *  Author:jefby
  *  Email:jef199006@gmail.com

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
	int press_cnt[4];
	//打开设备
	fd = open("/dev/buttons",0);
	if(fd < 0){
		printf("can't open /dev/buttons\n");
		return -1;
	}
	while(1){//循环读取，若read函数执行前无按键被按下，则此时进程处于休眠状态.只能使用按键K1~4
		ret = read(fd,press_cnt,sizeof(press_cnt));
		if(ret < 0){
			printf("read err.\n");
			continue;
		}
		for(i=0;i<sizeof(press_cnt)/sizeof(press_cnt[0]);++i){
			if(press_cnt[i])
				printf("K%d has been pressed %d times!\n",i+1,press_cnt[i]);
		}
	}
}

