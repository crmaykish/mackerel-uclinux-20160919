#include <stdio.h>
#include <unistd.h>

int main() {
	
	int i = 0;

	char *buffer;

	printf("Hello, Mackerel!\n");

	for (i = 0; i < 10000; i++) {
		printf("count: %d\n", i);

		buffer = (char *)malloc(1024);

		usleep(1000);

		free(buffer);
	}

	exit(0);
}
