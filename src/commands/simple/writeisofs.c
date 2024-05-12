
/* writeisofs - simple ISO9660-format-image writing utility */

#include <errno.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <ctype.h>
#include <ibm/partition.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/stat.h>

#define WriteField(fd, f)	Write(fd, &(f), sizeof(f))
#define min(a,b)	((a) < (b) ? (a) : (b))

#define FLAG_DIR		2

#define NAME_LEN		(DIRSIZ + 5)
#define ISO_NAME_LEN	12
#define PLATFORM_80X86	0

#define ISO_SECTOR		2048
#define VIRTUAL_SECTOR	512

#define CURRENT_DIR		"."
#define PARENT_DIR		".."

#define MIN_DIR_LEN		1
#define MAX_DIR_LEN		31

#define MAX_LEVEL		8

typedef unsigned char u_int8_t;
typedef unsigned short u_int16_t;
typedef unsigned long u_int32_t;

/* ===== CD (disk) data structures ===== 
 * Refer to: 
 *   https://wiki.osdev.org/ISO_9660
 */

/* Primary volume descriptor */
typedef struct {
	u_int8_t typeCode;	/* Always 0x01 for PVD */
	char id[5];		/* Always 'CD001' */
	u_int8_t version;	/* Always 0x01 */
	u_int8_t zero;
	char system[32];		
	char volume[32];
	u_int8_t zeroes1[8];
	u_int32_t volSize[2];	
	u_int8_t zeroes2[32];
	u_int16_t volSetSize[2];	
	u_int16_t volSeq[2];	
	u_int16_t logicalBlockSize[2];	
	u_int32_t pathTableSize[2];
	u_int32_t firstLittlePathTableLoc;
	u_int32_t secondLittlePathTableLoc;
	u_int32_t firstBigPathTableLoc;
	u_int32_t secondBigPathTableLoc;
	u_int8_t rootEntry[34];
	char volumeSet[128];
	char publisher[128];
	char preparer[128];
	char application[128];
	char copyrightFile[37];
	char abstractFile[37];
	char biblioFile[37];
	char createdTime[17];
	char modifiedTime[17];
	char expiredTime[17];
	char effectiveTime[17];
	u_int8_t fileStructVersion;	/* Always 0x01 */
	u_int8_t zero2;		/* Always 0x00 */
	u_int8_t zeroes3[512];	/* Contents not defined by ISO 9660. */
	u_int8_t zeroes4[653];	/* Reserved by ISO */
} PVD;

/* Boot record volume descriptor
 * Refer to: 
 *   https://web.archive.org/web/20180112220141/https://download.intel.com/support/motherboards/desktop/sb/specscdrom.pdf
 */
typedef struct {
	u_int8_t type;		/* 0 indicates a boot record. */
	char id[5];			/* Always "CD001" */
	u_int8_t version;	/* Always 0x01 */
	char system[32];	/* EL TORITO SPECIFICATION */
	u_int8_t zero[32];	/* Unused, must be 0 */
	u_int32_t bootCatalog;	/* Starting block address of boot catalog */
	u_int8_t zero2[1973];	/* Unused, must be 0 */
} BootRecord;

/* Boot catalog validation entry */
typedef struct {
	u_int8_t headerId;	/* Must be 0x01 */
	u_int8_t platform;	/* 0: 80x86; 1: Power PC; 2: Mac */
	u_int8_t zero[2];	/* Reservied, must be 0 */
	char idString[24];	/* ID String */
	u_int16_t checksum;
	u_int8_t keys[2];	/* 0x55AA */
} BCValidationEntry;

/* Boot catalog initial/default entry */
#define INDICATOR_BOOTABLE	0x88

#define BOOTMEDIA_NONE		0
#define BOOTMEDIA_120M		1
#define BOOTMEDIA_144M		2
#define BOOTMEDIA_288M		3
#define BOOTMEDIA_HARDDISK	4

typedef struct {
	u_int8_t indicator;	/* INDICATOR_BOOTABLE */
	u_int8_t mediaType;	/* BOOTMEDIA_* */
	u_int16_t loadSeg;	/* Load segment or 0 for default(0x7C0) */
	u_int8_t systemType;	/* From partition table */
	u_int8_t zero;		/* Unused, must 0 */
	u_int16_t sectors;
	u_int32_t startSector;
	u_int8_t zero2[20];	/* Unused, must 0 */
} BCInitialEntry;

/* Directory entry */
typedef struct {
	u_int8_t recordSize;
	u_int8_t extendedSize;
	u_int32_t dataSector[2];
	u_int32_t fileSize[2];
	u_int8_t year;
	u_int8_t month;
	u_int8_t day;
	u_int8_t hour;
	u_int8_t minute;
	u_int8_t second;
	u_int8_t offset;
	u_int8_t flags;
	u_int8_t interleaved;
	u_int8_t interleaveGap;
	u_int16_t volSeq[2];
	u_int8_t nameLen;
	char name[NAME_LEN];
} DirectoryEntry;


/* === Program (memory data structures === */
typedef struct Node {
	char name[NAME_LEN];
	int isDir;
	int pathTableRecord;
	struct Node *firstChild;	/* point to a child of dir */
	struct Node *next;	/* point to sibling */

	/* Filled out at i/o time */
	u_int32_t startSector;
	u_int32_t byteSize;
} Node;


static int harddiskEmulation = 0;
static int systemType = 0;


static ssize_t Write(int fd, void *buf, ssize_t len) {
	ssize_t r;

	if ((r = write(fd, buf, len)) != len) {
		if (r < 0) 
		  perror("write");
		fprintf(stderr, "failed or short write - aborting.\n");
		exit(1);
	}
	return len;
}

static off_t Lseek(int fd, off_t pos, int rel) {
	off_t r;

	if ((r = lseek(fd, pos, rel)) < 0) {
		perror("lseek");
		fprintf(stderr, "lseek failed - aborting.\n");
		exit(1);
	}
	return r;
}

static ssize_t writeSector(int fd, char *block, int *currentSector) {
	ssize_t w;

	w = Write(fd, block, ISO_SECTOR);
	++(*currentSector);
	return w;
}

static void seekSector(int fd, int sector, int *currentSector) {
	Lseek(fd, sector * ISO_SECTOR, SEEK_SET);
	*currentSector = sector; 
} 

static ssize_t seekWriteSector(int fd, int sector, char *block, int *currentSector) {
	seekSector(fd, sector, currentSector);
	return writeSector(fd, block, currentSector);
}

static ssize_t Read(int fd, void *buf, ssize_t len) {
	ssize_t r;

	if ((r = read(fd, buf, len)) != len) {
		if (r < 0)
		  perror("read");
		fprintf(stderr, "failed or short read.\n");
		exit(1);
	}
	return len;
}

static void little16(unsigned char *dest, u_int16_t src) {
	dest[0] = src & 0xFF;
	dest[1] = (src >> 8) & 0xFF;
}

static void little32(unsigned char *dest, u_int32_t src) {
	dest[0] = src & 0xFF;
	dest[1] = (src >> 8) & 0xFF;
	dest[2] = (src >> 16) & 0xFF;
	dest[3] = (src >> 24) & 0xFF;
}

static void big16(unsigned char *dest, u_int16_t src) {
	dest[1] = src & 0xFF;
	dest[0] = (src >> 8) & 0xFF;
}

static void big32(unsigned char *dest, u_int32_t src) {
	dest[3] = src & 0xFF;
	dest[2] = (src >> 8) & 0xFF;
	dest[1] = (src >> 16) & 0xFF;
	dest[0] = (src >> 24) & 0xFF;
}

static void both16(unsigned char *both, unsigned short i16) {
	unsigned char *little, *big;
	
	little = both;
	big = both + 2;

	little[0] = big[1] = i16 & 0xFF;
	little[1] = big[0] = (i16 >> 8) & 0xFF;
}

static void both32(unsigned char *both, unsigned long i32) {
	unsigned char *little, *big;

	little = both;
	big = both + 4; 

	little[0] = big[3] = i32 & 0xFF;
	little[1] = big[2] = (i32 >> 8) & 0xFF;
	little[2] = big[1] = (i32 >> 16) & 0xFF;
	little[3] = big[0] = (i32 >> 24) & 0xFF;
}

static int cmpf(const void *v1, const void *v2) {
	Node *n1, *n2;
	int i;
	char f1[NAME_LEN], f2[NAME_LEN];

	n1 = (Node *) v1;
	n2 = (Node *) v2;
	strcpy(f1, n1->name);
	strcpy(f2, n2->name);
	for (i = 0; i < strlen(f1); ++i) {
		f1[i] = toupper(f1[i]);
	}
	for (i = 0; i < strlen(f2); ++i) {
		f2[i] = toupper(f2[i]);
	}
	return -strcmp(f1, f2);
}

static void makeTree(Node *thisDir, char *name, int level) {
	DIR *dir;
	struct dirent *e;
	Node *dirNodes = NULL;
	int reservedDirNodes = 0, usedDirNodes = 0;
	Node *child;

	thisDir->firstChild = NULL;
	thisDir->isDir = 1;
	thisDir->startSector = 0xdeadbeef;

	if (level >= MAX_LEVEL) {
		fprintf(stderr, "ignoring entries in %s (too deep for iso9660)\n",
			name);
		return;
	}
	
	if ((dir = opendir(CURRENT_DIR)) == NULL) {
		perror("opendir");
		return;
	}

	/* How many entries do we need to allocate? */
	while (readdir(dir)) {
		++reservedDirNodes;
	}
	if (! reservedDirNodes) {
		closedir(dir);
		return;
	}

	if ((dirNodes = malloc(sizeof(*dirNodes) * reservedDirNodes)) == NULL) {
		fprintf(stderr, "couldn't allocate dirNodes (%d bytes)\n",
			sizeof(*dirNodes) * reservedDirNodes);
		exit(1);
	}

	/* Remember all entries in this dir */
	rewinddir(dir);

	child = dirNodes;
	while ((e = readdir(dir))) {
		struct stat st;
		mode_t type;

		if (strcmp(e->d_name, CURRENT_DIR) == 0 ||
			strcmp(e->d_name, PARENT_DIR) == 0)
		  continue;

		if (stat(e->d_name, &st) < 0) {
			perror(e->d_name);
			fprintf(stderr, "failed to stat file/dir\n");
			exit(1);
		}

		type = st.st_mode & S_IFMT;
		if (type != S_IFDIR && type != S_IFREG)
		  continue;

		++usedDirNodes;
		if (usedDirNodes > reservedDirNodes) {
			fprintf(stderr, "huh, directory entries appeared "
				"(not enough pre-allocated nodes; this can't happen) ?\n");
			exit(1);
		}

		if (type == S_IFDIR) {
			child->isDir = 1;
		} else {
			child->isDir = 0;
			child->firstChild = NULL;
		}
		strncpy(child->name, e->d_name, sizeof(child->name));
		++child;
	}

	closedir(dir);

	if (! usedDirNodes)
	  return;

	if ((dirNodes = realloc(dirNodes, usedDirNodes * sizeof(dirNodes))) == NULL) {
		fprintf(stderr, "realloc() of dirNodes failed - aborting\n");
		exit(1);
	}

	qsort(dirNodes, usedDirNodes, sizeof(*dirNodes), cmpf);
	
	child = dirNodes;

	while (usedDirNodes--) {
		child->next = thisDir->firstChild;
		thisDir->firstChild = child;
		if (child->isDir) {
			if (chdir(child->name) < 0) {
				perror(child->name);
			} else {
				makeTree(child, child->name, level + 1);
				if (chdir(PARENT_DIR) < 0) {
					perror("chdir() failed");
					fprintf(stderr, "couldn't chdir() to parent, aborting\n");
					exit(1);
				}
			}
		}
		++child;
	}
}

static void writeBootRecord(int fd, int *currentSector, 
							int bootCatalogSector) {
	int i;
	static BootRecord bootRecord;
	ssize_t w = 0;

	/* Boot record volume descriptor */
	memset(&bootRecord, 0, sizeof(bootRecord));
	strncpy(bootRecord.id, "CD001", 5);
	bootRecord.version = 1;
	bootRecord.bootCatalog = bootCatalogSector;
	strcpy(bootRecord.system, "EL TORITO SPECIFICATION");
	for (i = strlen(bootRecord.system); 
			i < sizeof(bootRecord.system); ++i) {
		bootRecord.system[i] = '\0';
	}

	w += WriteField(fd, bootRecord.type);
	w += WriteField(fd, bootRecord.id);
	w += WriteField(fd, bootRecord.version);
	w += WriteField(fd, bootRecord.system);
	w += WriteField(fd, bootRecord.zero);
	w += WriteField(fd, bootRecord.bootCatalog);
	w += WriteField(fd, bootRecord.zero2);

	if (w != ISO_SECTOR) 
	  fprintf(stderr, 
"WARNING: something went wrong - boot record (%d) isn't a sector size (%d)\n",
			w, ISO_SECTOR);

	++(*currentSector);
}

static void writeBootCatalog(int fd, int *currentSector, 
							int imageSector, int imageSectors) {
	static char buf[ISO_SECTOR];
	BCValidationEntry validate;
	BCInitialEntry initial;

	ssize_t written, rest;
	u_int16_t *v, sum = 0;
	int i;

	/* Write validation entry */
	memset(&validate, 0, sizeof(validate));
	validate.headerId = 1;
	validate.platform = PLATFORM_80X86;
	strcpy(validate.idString, "");
	validate.keys[0] = 0x55;
	validate.keys[1] = 0xAA;

	/* Checksum:
	 *   This sum of all the words in this record should be 0.
	 */
	v = (u_int16_t *) &validate;
	for (i = 0; i < sizeof(validate) / 2; ++i) {
		sum += v[i];
	}
	validate.checksum = 65535 - sum + 1;	/* Sum must be 0 */

	written = Write(fd, &validate, sizeof(validate));

	
	/* Write initial/default entry */
	memset(&initial, 0, sizeof(initial));
	initial.indicator = INDICATOR_BOOTABLE;
	if (harddiskEmulation) {
		initial.mediaType = BOOTMEDIA_HARDDISK;
		initial.systemType = systemType;
	} else {
		initial.mediaType = BOOTMEDIA_144M;
	}
	initial.sectors = imageSectors;//TODO
	initial.startSector = imageSector;

	written += Write(fd, &initial, sizeof(initial));

	/* Fill out the rest of the sector with 0's */
	if ((rest = ISO_SECTOR - (written % ISO_SECTOR))) {
		memset(buf, 0, sizeof(buf));
		written += Write(fd, buf, rest);
	}
	(*currentSector) += written / ISO_SECTOR;
}

static int writeBootImage(int bootFd, int fd, int *currentSector) {
	static char buf[1024 * 64];
	ssize_t chunk, written = 0, rest;
	int virtualSectors;

	while ((chunk = read(bootFd, buf, sizeof(buf))) > 0) {
		written = Write(fd, buf, chunk);
	}
	if (chunk < 0) {
		perror("read boot image");
		exit(1);
	}
	virtualSectors = written / VIRTUAL_SECTOR;
	if (written % VIRTUAL_SECTOR) //TODO
	  ++virtualSectors;

	if ((rest = ISO_SECTOR - (written % ISO_SECTOR))) {
		memset(buf, 0, sizeof(buf));
		written = Write(fd, buf, rest);
	}
	(*currentSector) += written / ISO_SECTOR;

	return virtualSectors;
}

static int getSystemType(int fd) {
	off_t oldPos;
	size_t size;
	ssize_t r;
	int type;
	PartitionEntry *pe;
	unsigned char bootSector[512];

	errno = 0;
	oldPos = lseek(fd, SEEK_CUR, 0);
	if (oldPos == -1 && errno != 0) {
		fprintf(stderr, "bootImage file is not seekable: %s\n",
					strerror(errno));
		exit(1);
	}
	size = sizeof(bootSector);
	r = read(fd, bootSector, size);
	if (r != size) {
		fprintf(stderr, "error reading bootImage file: %s\n",
			r < 0 ? strerror(errno) : "unexpected EOF");
		exit(1);
	}
	if (bootSector[size - 2] != 0x55 && bootSector[size - 1] != 0xAA) {
		fprintf(stderr, "bad magic in bootImage file\n");
		exit(1);
	}

	pe = (PartitionEntry *) &bootSector[PART_TABLE_OFF];
	type = pe->type;
	if (type == NO_PART) {
		fprintf(stderr, "first partition table entry is unused\n");
		exit(1);
	}
	if (! (pe->status && ACTIVE_FLAG)) {
		fprintf(stderr, "first partition table entry is not active\n");
		exit(1);
	}

	lseek(fd, SEEK_SET, oldPos);
	return type;
}

static ssize_t writeDirEntry(char *originName, u_int32_t sector,
							u_int32_t size, int isDir, int fd) {
	int nameLen, total = 0;
	DirectoryEntry entry;
	char copyName[NAME_LEN];

	memset(&entry, 0, sizeof(entry));

	if (strcmp(originName, CURRENT_DIR) == 0) {
		nameLen = 1;	/* Empty string */
	} else if (strcmp(originName, PARENT_DIR) == 0) {
		entry.name[0] = '\001';
		nameLen = 1;
	} else {
		int i;

		strcpy(copyName, originName);
		nameLen = strlen(copyName);

		if (nameLen > ISO_NAME_LEN) {
			fprintf(stderr, "%s: truncated, too long for iso9660\n", 
						copyName);
			nameLen = ISO_NAME_LEN;
			copyName[nameLen] = '\0';
		}

		strcpy(entry.name, copyName);
		for (i = 0; i < nameLen; ++i) {
			entry.name[i] = toupper(entry.name[i]);
		}

		/* Padding byte + system field */
		entry.name[nameLen] = '\0';
		entry.name[nameLen + 1] = '\0';
		entry.name[nameLen + 2] = '\0';
	}
	entry.nameLen = nameLen;	/* Original length */
	/* Excluding 'name', there are already 33 bytes, dir entry must always 
	 * start on an even byte number.
	 * If the 'nameLen' is even, a padding byte is needed which is 0.
	 */
	if (nameLen % 2 == 0)	
	  ++nameLen;	/* Length with padding byte */

	/* 2 extra bytes for 'system use'.. */
	entry.recordSize = 33 + nameLen;
	both32((unsigned char *) entry.dataSector, sector);
	both32((unsigned char *) entry.fileSize, size);

	if (isDir)
	  entry.flags = FLAG_DIR;

	/* Node date */
	both16((unsigned char *) entry.volSeq, 1);

	total += WriteField(fd, entry.recordSize);
	total += WriteField(fd, entry.extendedSize);
	total += WriteField(fd, entry.dataSector);
	total += WriteField(fd, entry.fileSize);
	total += WriteField(fd, entry.year);
	total += WriteField(fd, entry.month);
	total += WriteField(fd, entry.day);
	total += WriteField(fd, entry.hour);
	total += WriteField(fd, entry.minute);
	total += WriteField(fd, entry.second);
	total += WriteField(fd, entry.offset);
	total += WriteField(fd, entry.flags);
	total += WriteField(fd, entry.interleaved);
	total += WriteField(fd, entry.interleaveGap);
	total += WriteField(fd, entry.volSeq);
	total += WriteField(fd, entry.nameLen);
	total += Write(fd, entry.name, nameLen);

	if (total != entry.recordSize || total % 2 != 0) {
		printf("%2d, %2d!  ", total, entry.recordSize);
		printf("%3d = %3d - %2d + %2d\n",
			entry.recordSize, sizeof(entry), sizeof(entry.name), nameLen);
	}

	return entry.recordSize;
}

static void writeData(Node *parent, Node *currNode, int fd, 
				int *currentSector, int dirs, DirectoryEntry *rootEntry,
				int rootSize, int removeAfter) {
	static char buf[1024 * 1024];
	Node *c;
	ssize_t written = 0, rest;

	for (c = currNode->firstChild; c; c = c->next) {
		if (c->isDir && chdir(c->name) < 0) {
			perror(c->name);
			fprintf(stderr, "couldn't chdir to %s - aborting\n",
					c->name);
			exit(1);
		}
		writeData(currNode, c, fd, currentSector, dirs, rootEntry, 
					rootSize, removeAfter);
		if (c->isDir && chdir(PARENT_DIR) < 0) {
			perror("chdir to ..");
			fprintf(stderr, "couldn't chdir to parent, - aborting\n");
			exit(1);

		}
	}

	/* Write nodes depth-first, down-top */
	if (currNode->isDir && dirs) {	
		/* Dir */
		written = 0;
		currNode->startSector = *currentSector;
		written += writeDirEntry(CURRENT_DIR, currNode->startSector,
					currNode->byteSize, currNode->isDir, fd);
		if (parent) 
		  written += writeDirEntry(PARENT_DIR, parent->startSector,
					  parent->byteSize, parent->isDir, fd);	//TODO
		else
		  written += writeDirEntry(PARENT_DIR, currNode->startSector,
					  currNode->byteSize, currNode->isDir, fd);

		off_t cur1, cur2;
		ssize_t writtenBefore;

		for (c = currNode->firstChild; c; c = c->next) {
			cur1 = Lseek(fd, 0, SEEK_CUR);
			writtenBefore = written;
			written += writeDirEntry(c->name, c->startSector, 
						c->byteSize, c->isDir, fd);
			cur2 = Lseek(fd, 0, SEEK_CUR);
			if (cur1 / ISO_SECTOR != (cur2 - 1) / ISO_SECTOR) {
				/* Passed a sector boundary, argh!
				 *
				 * Zero-padding the sector and use the next one
				 * consecutive sector.
				 */
				Lseek(fd, cur1, SEEK_SET);
				written = writtenBefore;
				rest = ISO_SECTOR - (written % ISO_SECTOR);
				memset(buf, 0, rest);
				Write(fd, buf, rest);
				written += rest;
				written += writeDirEntry(c->name, c->startSector, 
							c->byteSize, c->isDir, fd);
			}
		}
		currNode->byteSize = written;
	} else if (! currNode->isDir && ! dirs) {
		/* File */
		struct stat st;
		ssize_t rem;
		int fileFd;
		ssize_t chunk;

		if (stat(currNode->name, &st) < 0) {
			perror(currNode->name);
			fprintf(stderr, "couldn't stat %s - aborting\n", currNode->name);
			exit(1);
		}
		if ((fileFd = open(currNode->name, O_RDONLY)) < 0) {
			perror(currNode->name);
			fprintf(stderr, "couldn't open %s - aborting\n", currNode->name);
			exit(1);
		}
		rem = st.st_size;
		currNode->startSector = *currentSector;

		while (rem > 0) {
			chunk = min(sizeof(buf), rem);
			Read(fileFd, buf, chunk);
			Write(fd, buf, chunk);
			rem -= chunk;
		}
		close(fileFd);

		currNode->byteSize = written = st.st_size;
		if (removeAfter && unlink(currNode->name) < 0) {
			perror("unlink");
			fprintf(stderr, "couldn't remove %s\n", currNode->name);
		}
	} else {
		/* Nothing to be done */
		return;
	}

	/* Fill out sector with zero bytes */
	if ((rest = ISO_SECTOR - (written % ISO_SECTOR))) {
		memset(buf, 0, sizeof(buf));
		Write(fd, buf, rest);
		written += rest;
	}

	/* Update dir size with padded size */
	if (currNode->isDir)
	  currNode->byteSize = written;

	*currentSector += written / ISO_SECTOR;
}

static void traverseTree(Node *root, int level, int isLittleEndian, int maxLevel,
						int *bytes, int fd, int parentRecord, int *recordNum) {
	Node *child;
	struct PTE {	/* The Path Table Entry */
		u_int8_t len;		/* Length of Directory Identifier */
		u_int8_t zero;		/* Extended Attribute Record Length */
		u_int32_t startSector;	/* Location of extent */
		u_int16_t parent;	/* Directory number of parent (an index in to the path table) */
	} pte;

	if (level == maxLevel) {
		int i;
		char newName[NAME_LEN];

		if (! root->isDir)
		  return;
		pte.zero = 0;
		if (level == 1) {
			/* Root */
			pte.len = 1;
			pte.parent = 1;
			root->name[0] = root->name[1] = '\0';
		} else {
			pte.len = strlen(root->name);
			pte.parent = parentRecord;
		}
		pte.startSector = root->startSector;
		root->pathTableRecord = (*recordNum)++;

		if (isLittleEndian) {
			little32((unsigned char *) &pte.startSector, pte.startSector);
			little16((unsigned char *) &pte.parent, pte.parent);
		} else {
			big32((unsigned char *) &pte.startSector, pte.startSector);
			big16((unsigned char *) &pte.parent, pte.parent);
		}

		*bytes += WriteField(fd, pte.len);
		*bytes += WriteField(fd, pte.zero);
		*bytes += WriteField(fd, pte.startSector);
		*bytes += WriteField(fd, pte.parent);
		if (pte.len % 2 == 0)	//TODO
		  root->name[pte.len++] = '\0';
		for (i = 0; i < pte.len; ++i) {
			newName[i] = toupper(root->name[i]);
		}
		*bytes += Write(fd, newName, pte.len);
		return;
	}

	for (child = root->firstChild; child; child = child->next) {
		if (child->isDir) 
		  traverseTree(child, level + 1, isLittleEndian, maxLevel, bytes, fd,
					  root->pathTableRecord, recordNum);
	}
}

static int makePathTables(Node *root, int isLittleEndian, int *bytes, int fd) {
	int level;
	static char block[ISO_SECTOR];
	int recordNum;

	recordNum = 1;
	*bytes = 0;

	for (level = 1; level <= MAX_LEVEL; ++level) {
		traverseTree(root, 1, isLittleEndian, level, bytes, fd, 1, &recordNum);
	}

	if (*bytes % ISO_SECTOR) {
		ssize_t x;
	
		x = ISO_SECTOR - (*bytes % ISO_SECTOR);
		memset(block, 0, sizeof(block));
		Write(fd, block, x);
		*bytes += x;
	}
	return *bytes / ISO_SECTOR;
}

int main(int argc, char **argv) {
	int currentSector = 0;
	int imageSector, imageSectors;
	int bootFd, fd, i, ch, numSectors;
	int removeAfter = 0;
	static char block[ISO_SECTOR];
	static PVD pvd;
	char *label = "ISO9660";
	struct tm *now;
	time_t nowTime;
	char timeStr[20], *prog;
	char *bootImage = NULL;
	struct Node root;
	int pvdSector;
	int bigPathTable, littlePathTable, pathTableBytes = 0;
	int dirSector, fileSector, endDir;
	int bootVolumeSector, bootCatalogSector;

	prog = argv[0];

	/* This check is to prevent compiler padding screwing up
	 * our format.
	 */
	if (sizeof(PVD) != ISO_SECTOR) {
		fprintf(stderr, "Something confusing happened at\n"
			"compile-time; PVD should be a sector size. %d != %d\n",
			sizeof(PVD), ISO_SECTOR);
		return 1;
	}

	while ((ch = getopt(argc, argv, "Rb:hl:")) != -1) {
		switch (ch) {
			case 'h':
				harddiskEmulation = 1;
				break;
			case 'l':
				label = optarg;
				break;
			case 'r':
				removeAfter = 1;
				break;
			case 'b':
				bootImage = optarg;
				if ((bootFd = open(bootImage, O_RDONLY)) < 0) {
					perror(bootImage);
					return 1;
				}
				break;
		}
	}

	argc -= optind;
	argv += optind;

	if (argc != 2) {
		fprintf(stderr, 
			"usage: %s [-l <label>] [-b <bootFloppyImage>] <dir> <isoFile>\n",
			prog);
		return 1;
	}

	/* Create .iso file */
	if ((fd = open(argv[1], O_WRONLY | O_TRUNC | O_CREAT, 0600)) < 0) {
		perror(argv[1]);
		return 1;
	}

	/* Go to where the iso has to be made from */
	if (chdir(argv[0]) < 0) {
		perror(argv[0]);
		return 1;
	}

	/* Collect dirs and files */
	fprintf(stderr, " * traversing input tree\n");
	makeTree(&root, "", 1);

	fprintf(stderr, " * writing initial zeros and pvd\n");

	/* First 16 sectors are zero */
	memset(block, 0, sizeof(block));
	for (i = 0; i < 16; ++i) {
		writeSector(fd, block, &currentSector);
	}

	/* Primary Volume Descriptor */
	memset(&pvd, 0, sizeof(pvd));
	pvd.typeCode = 1;
	strncpy(pvd.id, "CD001", 5);
	pvd.version = 1;
	strncpy(pvd.volume, label, sizeof(pvd.volume) - 1);
	for (i = strlen(pvd.volume); i < sizeof(pvd.volume); ++i) {
		pvd.volume[i] = ' ';
	}
	memset(pvd.system, ' ', sizeof(pvd.system));

	both16((unsigned char *) pvd.volSetSize, 1);
	both16((unsigned char *) pvd.volSeq, 1);
	both16((unsigned char *) pvd.logicalBlockSize, ISO_SECTOR);

	/* Fill time fields */
	time(&nowTime);
	now = gmtime(&nowTime);
	strftime(timeStr, sizeof(timeStr), "%Y%m%d%H%M%S000", now);
	memcpy(pvd.createdTime, timeStr, strlen(timeStr));
	memcpy(pvd.modifiedTime, timeStr, strlen(timeStr));
	memcpy(pvd.effectiveTime, timeStr, strlen(timeStr));
	strcpy(pvd.expiredTime, "0000000000000000");	/* Not specified */
	pvdSector = currentSector;

	writeSector(fd, (char *) &pvd, &currentSector);

	if (bootImage) {
		fprintf(stderr, " * writing boot record volume descriptor\n");
		bootVolumeSector = currentSector;
		writeBootRecord(fd, &currentSector, 0);
	}

	/* Volume descriptor set terminator */
	memset(block, 0, sizeof(block));
	block[0] = 255;
	strncpy(block, "CD001", 5);
	block[6] = 1;

	writeSector(fd, block, &currentSector);

	if (bootImage) {
		/* Write the boot catalog */
		fprintf(stderr, " * writing the boot catalog\n");
		bootCatalogSector = currentSector;
		if (harddiskEmulation)
		  systemType = getSystemType(bootFd);
		/* imageSector and imageSectors will be updated later. */
		writeBootCatalog(fd, &currentSector, 0, 0);

		/* Write boot image */
		fprintf(stderr, " * writing the boot image\n");
		imageSector = currentSector;
		imageSectors = writeBootImage(bootFd, fd, &currentSector);
		fprintf(stderr, 
			" * image: %d virtual sectors @ sector 0x%x\n",
			imageSectors, imageSector);
		close(bootFd);
	}


	/* Write all the file data */
	fileSector = currentSector;
	fprintf(stderr, " * writing file data\n");
	writeData(NULL, &root, fd, &currentSector, 0,
		(DirectoryEntry *) &pvd.rootEntry, sizeof(pvd.rootEntry),
		removeAfter);
	/* Now startSector and byteSize of each file node is filled. */

	/* Write out all the dir data */
	dirSector = currentSector;
	fprintf(stderr, " * writing dir data\n");
	writeData(NULL, &root, fd, &currentSector, 1,
		(DirectoryEntry *) &pvd.rootEntry, sizeof(pvd.rootEntry),
		removeAfter);
	/* Now startSector and byteSize of each dir node is filled. */

	endDir = currentSector;
	seekSector(fd, dirSector, &currentSector);
	fprintf(stderr, " * rewritting dir data\n");
	fflush(NULL);
	/* Rewrite dir nodes with determined startSector and byteSize */
	writeData(NULL, &root, fd, &currentSector, 1,
		(DirectoryEntry *) &pvd.rootEntry, sizeof(pvd.rootEntry),
		removeAfter);
	if (currentSector != endDir) 
	  fprintf(stderr, "warning: inconsistent directories - "
			"I have a bug! iso may be broken.\n");


	/* Now write the path table in both formats */
	fprintf(stderr, " * writing big-endian path table\n");
	bigPathTable = currentSector;
	currentSector += makePathTables(&root, 0, &pathTableBytes, fd);

	fprintf(stderr, " * writing little-endian path table\n");
	littlePathTable = currentSector;
	currentSector += makePathTables(&root, 1, &pathTableBytes, fd);

	/* This is the size of the iso filesystem for use in the PVD later */
	//TODO


	return 0;
}


