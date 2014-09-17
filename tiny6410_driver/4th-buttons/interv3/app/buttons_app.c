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
#include <errno.h>

static int cnt=0;
int main(void)
{
	int buttons_fd;
	unsigned char buttons=0 ;

	buttons_fd = open("/dev/buttons", 0);
	if (buttons_fd < 0) {
		perror("open device buttons");
		return -1;
	}
	
	while(1){
		read(buttons_fd, &buttons, sizeof (buttons));
		printf("%dKEY%02x entered.\n",cnt++,buttons);
	}
	close(buttons_fd);
	return 0;
}

