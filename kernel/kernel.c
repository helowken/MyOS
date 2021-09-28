//char c = 'Y';
/*
static char *s;
int a = 10;
static char b;
*/

void main() {
    char *video_memory = (char*) 0xb8000;
	*video_memory = 'Y';
	while(1) {
	}
}
