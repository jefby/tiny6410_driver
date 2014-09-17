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
	int ret = 0;
	int Oflag = 0;

	fd = open("/dev/buttons", O_RDWR);
	if (fd < 0) {
		perror("open device buttons");
		return -1;
	}
	
	while(1){
		read(fd,&val,1);
		printf("%d:KEY%02x enter.\n",cnt++,val);
		//sleep(1000);
	}
	close(fd);
	return 0;
}

