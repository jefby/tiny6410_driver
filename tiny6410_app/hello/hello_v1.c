#include <unistd.h>

char msg[]="hello,world.";
int main(void)
{
	write(1,msg,sizeof(msg)/sizeof(char));
	_exit(0);
}
