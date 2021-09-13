#include "code.h"
#include "stdlib.h"
#include "util.h"
#include "sys/dir.h"
#include "limits.h"
#include "boot.h"

static off_t imageOffset, imageSize;
static u32_t (*vir2Sec)(u32_t vsec);		

// Simply add an absolute sector offset to vsec.
static u32_t flatVir2Sec(u32_t vsec) {
	return lowSector + imageOffset + vsec;
}

static char *selectImage(char *image) {
	size_t len;
	char *size;
	
	len = (strlen(image) + 1 + NAME_MAX + 1) * sizeof(char);
	image = strcpy(malloc(len), image);

	// TODO lookup from file system
	
	if (numPrefix(image, &size) && 
				*size++ == ':' &&
				numeric(size)) {
		vir2Sec = flatVir2Sec;
		imageOffset = a2l(image);
		imageSize = a2l(size);
		strcpy(image, "Minix");
		return image;
	}

	free(image);
	return NULL;
}

void bootMinix() {
	char *imageName, *image;

	imageName = getVarValue("image");
	printf("image env: %s\n", imageName);
	if ((image = selectImage(imageName)) == NULL)
	  return;

	printf("image: %s, offset: %d, size: %d\n", image, imageOffset, imageSize);

	// TODO handle error
	//execImage(image);

	free(image);
}
