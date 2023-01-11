#include "stdio.h"

int main(int argc, char *argv[]) {
	int i;

	for (i = 0; i < argc; ++i) {
		if (i > 0) 
		  printf(", ");
		printf("%d: %s", i, argv[i]);
	}
	printf("\n");
	return 0;
}
