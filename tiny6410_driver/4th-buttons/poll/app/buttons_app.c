/*
 *
 *
 *	buttons的用户程序：配合驱动程序使用
 *	Author:jefby
 *	Email:jef199006@gmail.com
 *
 *
 * */
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/select.h>
#include <sys/time.h>
#include <poll.h>
#include <errno.h>

static int cnt=0;
int main(void)
{
	int buttons_fd;
	unsigned char buttons=0 ;
	struct pollfd fds[1];
	int ret = 0;

	buttons_fd = open("/dev/buttons", 0);
	if (buttons_fd < 0) {
		perror("open device buttons");
		return -1;
	}
	fds[0].fd = buttons_fd;
	fds[0].events = POLLIN ;
	while(1){
		ret = poll(fds,sizeof(fds)/sizeof(fds[0]),5000);
		if(ret == 0)
			printf("time out.\n");
		else{
			read(buttons_fd,&buttons,1);
			printf("%dKEY%02x entered.\n",cnt++,buttons);
		}
	}
	close(buttons_fd);
	return 0;
}

