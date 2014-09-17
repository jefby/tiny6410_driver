#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

void delay(unsigned long n)
{
	while(n--);
}
int main()
{
	int fd = 0;
	char buf[4]={0x1,0x2,0x3,0x4};
	char rbuf;

	fd = open("/dev/leds",O_RDWR);

	if(fd < 0){
		printf("open /dev/leds error.\n");
		return -1;
	}
	
	write(fd,buf,1);
//	delay(10000);
	read(fd,buf,1);
	printf("recieved %x.\n",*(unsigned int*)buf);
	close(fd);

	return 0;
}
















