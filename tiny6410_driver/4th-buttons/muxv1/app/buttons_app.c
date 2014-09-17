/*
 *
 *
 *	buttons的用户程序：配合驱动程序使用
 *	Author:jefby
 *	Email:jef199006@gmail.com
 *
 *
 * */

#include <sys/ioctl.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/select.h>
#include <sys/time.h>
#include <poll.h>
#include <errno.h>
#include <signal.h>
#include <sys/types.h>//getpid
#include <unistd.h>

static int cnt=0;
static int fd = 0;
static unsigned char val = 0;
void sig_fasync(int signum)
{
	read(fd,&val,1);
	printf("%d:KEY%02x enter.\n",cnt++,val);
}
int main(void)
{
	int ret = 0;
	int Oflag = 0;

	fd = open("/dev/buttons", 0);
	if (fd < 0) {
		perror("open device buttons");
		return -1;
	}
	signal(SIGIO,sig_fasync);//注册信号处理函数
	
	fcntl(fd,F_SETOWN,getpid());//把当前进程的ID号告诉驱动程序
	Oflag = fcntl(fd,F_GETFL);
	fcntl(fd,F_SETFL,Oflag | FASYNC);//设置异步通知标志,调用fasync
	while(1){
		sleep(1000);
	}
	close(fd);
	return 0;
}

