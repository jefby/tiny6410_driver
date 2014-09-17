/*
 *
 *	控制LED的应用程序,格式 ./led arg cmd
 *	arg=[0~4],cmd=[0.1]
 *	cmd=0表示关闭LED，cmd=1表示打开LED
 *	arg=0,全关或者全开LED灯
 *	arg=1~4表示打开或关闭指定的LED灯	
 *	Author:jefby
 *	Email:jef199006@gmail.com
 *
 **/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
int main(int argc, char **argv)
{
	int on;
	int led_no;
	int fd;
	/* 检查led 控制的两个参数，如果没有参数输入则退出。*/
	if (argc != 3 || sscanf(argv[1], "%d", &led_no) != 1 || sscanf(argv[2],"%d", &on) != 1 ||\
	on < 0 || on > 1 || led_no < 0 || led_no > 4) {
		fprintf(stderr, "Usage: leds led_no 0|1\n");
		exit(1);
	}
	/*打开/dev/leds 设备文件*/
	fd = open("/dev/leds0", 0);
	if (fd < 0) {
	fd = open("/dev/leds", 0);
	}
	if (fd < 0) {
	perror("open device leds");
	exit(1);
	}
	/*通过系统调用ioctl 和输入的参数控制led*/
	ioctl(fd, on, led_no);
	/*关闭设备句柄*/
	close(fd);
	return 0;
}
