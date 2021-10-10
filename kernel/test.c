char c = 'Y';
static char *s = "ABCD";
int a = 4;

void main() {
	int i;

	/*
	 * Note:
	 * 0xb8000 is the left top.
	 * Because ds = 0x1000, so we need to adjust it to be 0xb7000
	 */
    char *video_memory = (char *) 0xb7000;
	*video_memory = c;

	char *p = video_memory + 2;
	for (i = 0; i < a; ++i) {
		*p = s[i];
		p += 2;
	}

	while(1) {
	}
}
