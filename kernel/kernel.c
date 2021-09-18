static char *s;
int a = 10;
char c = 'c';
static char b;

void main() {
    char *video_memory = (char*) 0xb8000;
	b = 'a';
	s = "You are a boy! ";
	while (*s != 0) {
		*video_memory = *s++;
		video_memory += 2;
	}
}
