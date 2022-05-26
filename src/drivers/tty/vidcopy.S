	.text
/*=====================================================================*
 *                mem2VidCopy                *
 *=====================================================================*
 * void mem2VidCopy(u16_t *src, unsigned dst, unsigned count);
 *
 * Copy count characters from kernel memory to video memory. Src is an
 * ordinary pointer to a word, but dst and count are character (word) 
 * based video offset and count. If src is null then screen memory is 
 * blanked by filling it with blankColor.
 */
	.globl	mem2VidCopy
	.type	mem2VidCopy, @function
mem2VidCopy:
	pushl	%ebp
	movl	%esp, %ebp
	pushl	%esi
	pushl	%edi
	pushl	%es
	movl	8(%ebp), %esi		/* src */
	movl	12(%ebp), %edi		/* dst */
	movl	16(%ebp), %edx		/* count */
	movw	vidSeg, %es			/* es = Segment containing video memory */
	cld							/* Make sure direction is up */
.mvcLoop:
	andl	vidMask, %edi		/* Wrap address, edi = mask(dst) */
	movl	%edx, %ecx			/* ecx = count */
	movl	vidSize, %eax		
	subl	%edi, %eax			/* eax = vidSize - mask(dst) */
	cmpl	%eax, %ecx			
	jbe	0f						
	movl	%eax, %ecx			/* ecx = min(count, vidSize - mask(dst)) */
0:	
	subl	%ecx, %edx			/* count -= ecx */
	shll	$1, %edi			/* Convert to byte address */
	addl	vidOff, %edi		/* To video memory */
	testl	%esi, %esi			/* if src == 0, means blank the screen */
	jz	.mvcBlank
.mvcCopy:
	rep	movsw					/* Copy words to video memory */
	jmp .mvcTest
.mvcBlank:
	movl blankColor, %eax		/* eax = blanking character */
	rep	stosw					/* Copy blanks to video memory */
.mvcTest:
	subl	vidOff, %edi		/* Back to a byte address */	
	shrl	$1, %edi			/* Back to a word address */
	testl	%edx, %edx			
	jnz	.mvcLoop				/* if count != 0, then jump to .mvcLoop */
.mvcDone:
	popl	%es
	popl	%edi
	popl	%esi
	popl	%ebp
	retl


/*=====================================================================*
 *                vid2VidCopy                *
 *=====================================================================*
 * void vid2VidCopy(unsigned src, unsigned dst, unsigned count);
 * 
 * Copy count characters from video memory to video memory. Handle overlap.
 * Used for scrolling, line or character insertion and deletion. Src, dst
 * and count are character (word) based video offsets and count.
 */
	.globl	vid2VidCopy
	.type	vid2VidCopy, @function
vid2VidCopy:
	pushl	%ebp
	movl	%esp, %ebp
	pushl	%esi
	pushl	%edi
	pushl	%es
	movl	8(%ebp), %esi		/* src */
	movl	12(%ebp), %edi		/* dst */
	movl	16(%ebp), %edx		/* count */
	movw	vidSeg, %es			/* es = Segment containing video memory */
	cmpl	%edi, %esi			/* Copy up or down? */
	jb	.vvcDown
.vvcUp:
	cld							/* Direction is up */
.vvcUpLoop:
	andl	vidMask, %esi		/* Wrap address, esi = mask(src) */
	andl	vidMask, %edi		/* Wrap address, edi = mask(dst) */
	movl	%edx, %ecx			/* ecx = count */
	movl	vidSize, %eax		/* eax = vidSize */
	subl	%esi, %eax			/* eax = vidSize - esi */
	cmpl	%eax, %ecx			
	jbe	0f
	movl	%eax, %ecx			/* ecx = min(count, vidSize - mask(src)) */
0:
	movl	vidSize, %eax		/* eax = vidSize */
	subl	%edi, %eax			/* eax = vidSize - edi */
	cmpl	%eax, %ecx			
	jbe	0f
	movl	%eax, %ecx			/* ecx = min(count, vidSize - esi, vidSize - edi) */
0:
	subl	%ecx, %edx			/* count -= ecx */
	calll	vvcCopy				/* Copy video words */
	testl	%edx, %edx			
	jnz	.vvcUpLoop				/* if count != 0, then jump to .vvcUpLoop */
	jmp	.vvcDone
.vvcDown:
	std							/* Direction is down */
	leal	-1(%esi,%edx,1), %esi	/* esi += count - 1, start copying at the top */
	leal	-1(%edi,%edx,1), %edi	/* edi += count - 1, start copying at the top */
.vvcDownLoop:
	andl	vidMask, %esi		/* Wrap address, esi = mask(src + count - 1) */
	andl	vidMask, %edi		/* Wrap address, edi = mask(dst + count - 1) */
	movl	%edx, %ecx			/* ecx = count */
	leal	1(%esi), %eax		/* eax = esi + 1 */
	cmpl	%eax, %ecx
	jbe	0f
	movl	%eax, %ecx			/* ecx = min(count, esi + 1) */
0:
	leal	1(%edi), %eax		/* eax = edi + 1 */
	cmpl	%eax, %ecx			
	jbe	0f
	movl	%eax, %ecx			/* ecx = min(count, esi + 1, edi + 1) */
0:
	subl	%ecx, %edx			/* count -= ecx */
	calll	vvcCopy				/* Copy video words */
	testl	%edx, %edx
	jnz	.vvcDownLoop			/* if count != 0, then jump to .vvcDownLoop */
	cld							/* Reset direction */
.vvcDone:
	popl	%es
	popl	%edi
	popl	%esi
	popl	%ebp
	retl

/* Copy video words. (Inner code of both the up and down copying loop.) */
	.type	vvcCopy, @function
vvcCopy:
	shll	$1, %esi			/* esi *= 2, convert to byte address */
	shll	$1, %edi			/* edi *= 2, convert to byte address */
	addl	vidOff, %esi		/* esi += vidOff, to video memory */
	addl	vidOff, %edi		/* edi += vidOff, to video memory */
	rep	movsw					/* Copy video words */
	subl	vidOff, %esi		/* esi -= vidOff, to byte address */
	subl	vidOff, %edi		/* edi -= vidOff, to byte address */
	shrl	$1, %esi			/* esi /= 2, back to word address */
	shrl	$1, %edi			/* edi /= 2, back to word address */
	retl














