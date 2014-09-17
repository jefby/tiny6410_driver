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

int main(void)
{
	volatile int ret = 0;
	//以非阻塞方式打开
	fd = open("/dev/buttons", O_RDWR | O_NONBLOCK);
	if (fd < 0) {
		perror("open device buttons");
		return -1;
	}
	while(1){
		ret = read(fd,&val,1);
		printf("%d:KEY%02x enter.ret=%d\n",cnt++,val,ret);
		sleep(5);
	}
	close(fd);
	return 0;
}

