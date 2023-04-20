#include "../drivers.h"
#include "../../kernel/const.h"
#include "assert.h"

#include "random.h"
#include "sha2.h"
#include "aes/rijndael.h"

#define N_DERIV		16
#define NR_POOLS	32
#define MIN_SAMPLES	256		/* Number of samples needed in pool 0 for a re-seed. */

static unsigned long deriv[RANDOM_SOURCES][N_DERIV];
static int poolInd[RANDOM_SOURCES];
static SHA256_CTX poolCtx[NR_POOLS];
static unsigned samples = 0;
static int gotSeeded = 0;
static u8_t randomKey[2 * AES_BLOCKSIZE];
static u32_t countLow, countHigh;
static u32_t reseedCount;

void randomInit() {
	int i, j;

	for (i = 0; i < RANDOM_SOURCES; ++i) {
		for (j = 0; j < N_DERIV; ++j) {
			deriv[i][j] = 0;
		}
		poolInd[i] = 0;
	}
	for (i = 0; i < NR_POOLS; ++i) {
		SHA256_Init(&poolCtx[i]);
	}
	countLow = 0;
	countHigh = 0;
	reseedCount = 0;
}

int randomIsSeeded() {
	if (gotSeeded)
	  return 1;
	return 0;
}

static void reseed() {
	int i;
	SHA256_CTX ctx;
	u8_t digest[SHA256_DIGEST_LENGTH];

	if (samples < MIN_SAMPLES)
	  return;

	++reseedCount;
	SHA256_Init(&ctx);
	if (gotSeeded)
	  SHA256_Update(&ctx, randomKey, sizeof(randomKey));
	SHA256_Final(digest, &poolCtx[0]);
	SHA256_Update(&ctx, digest, sizeof(digest));
	SHA256_Init(&poolCtx[0]);
	for (i = 1; i < NR_POOLS; ++i) {
		if ((reseedCount & (1UL << (i - 1))) != 0)
		  break;
		SHA256_Final(digest, &poolCtx[i]);
		SHA256_Update(&ctx, digest, sizeof(digest));
		SHA256_Init(&poolCtx[i]);
	}
	SHA256_Final(digest, &ctx);
	memcpy(randomKey, &digest, sizeof(randomKey));
	samples = 0;
	gotSeeded = 1;
}

static void addSample(int source, unsigned long sample) {
	int i, poolNum;
	unsigned long d, v, di, min;

	/* Delete bad sample. Compute the Nth derivative. Delete the sample
	 * if any derivative is too small.
	 */
	min = (unsigned long) -1;
	v = sample;
	for (i = 0; i < N_DERIV; ++i) {
		di = deriv[source][i];
		
		/* Compute the difference */
		if (v >= di)
		  d = v - di;
		else
		  d = di - v;
		deriv[source][i] = v;
		v = d;
		if (v < min)
		  min = v;
	}
	if (min < 2) {
	  return;
	}

	poolNum = poolInd[source];
	SHA256_Update(&poolCtx[poolNum], (unsigned char *) &sample,
				sizeof(sample));
	if (poolNum == 0)
	  ++samples;

	++poolNum;
	if (poolNum >= NR_POOLS)
	  poolNum = 0;
	
	poolInd[source] = poolNum;
}

void randomUpdate(int source, unsigned short *buf, int count) {
	int i;

	if (source < 0 || source >= RANDOM_SOURCES)
	  panic("random", "randomUpdate: bad source", source);
	for (i = 0; i < count; ++i) {
		addSample(source, buf[i]);
	}
	reseed();
}

static void dataBlock(rd_keyinstance *keyPtr, void *data) {
	int r;
	u8_t input[AES_BLOCKSIZE];

	memset(input, '\0', sizeof(input));

	/* Do we want the output of the random numbers to be portable
	 * across platforms (for example for RSA signatures)? At the moment
	 * we don't do anything special. Encrypt the counter with the AES
	 * key.
	 */
	memcpy(input, &countLow, sizeof(countLow));
	memcpy(input + sizeof(countLow), &countHigh, sizeof(countHigh));
	r = rijndael_ecb_encrypt(keyPtr, input, data, AES_BLOCKSIZE, NULL);
	assert(r == AES_BLOCKSIZE);

	++countLow;
	if (countLow == 0)
	  ++countHigh;
}

void randomGetBytes(void *buf, size_t size) {
	int n, r;
	u8_t *cp;
	rd_keyinstance key;
	u8_t output[AES_BLOCKSIZE];

	r = rijndael_makekey(&key, sizeof(randomKey), randomKey);
	assert(r == 0);

	cp = buf;
	while (size > 0) {
		n = AES_BLOCKSIZE;
		if (n > size) {
			n = size;
			dataBlock(&key, output);
			memcpy(cp, output, n);
		} else {
			dataBlock(&key, cp);
		}
		cp += n;
		size -= n;
	}

	/* Generate new key */
	dataBlock(&key, randomKey);
	dataBlock(&key, randomKey + AES_BLOCKSIZE);
}

void randomPutBytes(void *buf, size_t size) {
	/* Add bits to pool zero */
	SHA256_Update(&poolCtx[0], buf, size);

	/* Assume that these bits are truely random. Increment samples
	 * with the number of bits.
	 */
	samples += size * 8;

	reseed();
}


