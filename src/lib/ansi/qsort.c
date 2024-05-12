#include <stdlib.h>

static int (*qcompar)(const char *, const char *);

static void qexchange(register char *p, register char *q, 
					register size_t n) {
	register int c;

	while (n-- > 0) {
		c = *p;
		*p++ = *q;
		*q++ = c;
	}
}

static void q3exchange(register char *p, register char *q,
					register char *r, register size_t n) {
	register int c;

	while (n-- > 0) {
		c = *p;
		*p++ = *r;
		*r++ = *q;
		*q++ = c;
	}
}

static void qsort1(char *a1, char *a2, register size_t size) {
	register char *left, *right;
	register char *leftEq, *rightEq;
	int cmp;

	for (;;) {
		if (a2 <= a1)
		  return;
		left = a1;
		right = a2;
		leftEq = rightEq = a1 + size * (((a2 - a1) + size) / (size * 2));
		/* Pick and element in the middle of the array.
		 * We will collect the equals around it.
		 * "leftEq" and "rightEq" indicate the left and right
		 * bounds of the equals respectively.
		 * Smaller elements end up left of it, larger elements and
		 * up right of it.
		 */
again:
		while (left < leftEq && (cmp = (*qcompar)(left, leftEq)) <= 0) {
			if (cmp < 0) {
				/* Leave it where it is */
				left += size;
			} else {
				/* Equal, so exchange with the element to
				 * the left of the "equal"-internal.
				 */
				leftEq -= size;
				qexchange(left, leftEq, size);
			}
		}
		while (right > rightEq) {
			if ((cmp = (*qcompar)(right, rightEq)) < 0) {
				/* Smaller, should go to left part. */
				if (left < leftEq) {
					/* Yes, we had a larger one at the 
					 * left, so we can just exchange.
					 */
					qexchange(left, right, size);
					left += size;
					right -= size;
					goto again;
				}
				/* No more room at the left part, so we
				 * move the "equal-interval" one place to the
				 * right, and the smaller element to the
				 * left of it.
				 * This is best expressed as a three-way
				 * exchange.
				 */
				rightEq += size;
				q3exchange(left, rightEq, right, size);
				leftEq += size;
				left = leftEq;
			} else if (cmp == 0) {
				/* Equal, so exchange with the element to
				 * the right of the "equal-interval"
				 */
				rightEq += size;
				qexchange(right, rightEq, size);
			} else {
				/* Just leave it */
				right -= size;
			}
		}
		if (left < leftEq) {
			/* Larger element to the left, but no more room,
			 * so move the "equal-interval" one place to the
			 * left, and the larger element to th right to it.
			 */
			leftEq -= size;
			q3exchange(right, leftEq, left, size);
			rightEq -= size;
			right = rightEq;
			goto again;
		}
		/* Now sort the "smaller" part */
		qsort1(a1, leftEq - size, size);
		/* and now the larger, saving a subroutine call
		 * because of the for(;;).
		 */
		a1 = rightEq + size;
	}
	/* NOT REACHED */
}

void qsort(void *base, size_t nmemb, size_t size, 
			int (*compar)(const void *, const void *)) {
	if (!nmemb)
	  return;
	qcompar = (int (*)(const char *, const char *)) compar;
	qsort1(base, (char *) base + (nmemb - 1) * size, size);
}



