#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

int main()
{
	int fd = 0;
	char buf[4]={0x1,0x2,0x3,0x4};
	char rbuf[4]={0};

	fd = open("/dev/leds",O_RDWR);

	if(fd < 0){
		printf("open /dev/leds error.\n");
		return -1;
	}
	
	write(fd,buf,4);
	read(fd,buf,4);
	printf("recieved %08x.\n",*(unsigned int*)rbuf);
	close(fd);

	return 0;
}
















