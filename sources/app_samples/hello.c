#include <stdio.h>
#include <unistd.h>
int main(void)
{
	while(1){
		printf("hello world!\r\n"); 
		sleep(2);
	}
	return 0;
}