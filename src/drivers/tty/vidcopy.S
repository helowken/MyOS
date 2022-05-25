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
	movl	8(%ebp), %esi		/* source */
	movl	12(%ebp), %edi		/* dst */
	movl	16(%ebp), %edx		/* count */
	movw	vidSeg, %es			/* es = Segment containing video memory */
	cld							/* Make sure direction is up */
.mvcLoop:
	andl	vidMask, %edi		/* Wrap address */
	movl	%edx, %ecx			/* ecx = count */
	movl	vidSize, %eax		
	subl	%edi, %eax			/* eax = vidSize - dst */
	cmpl	%eax, %ecx			
	jbe	0f						
	movl	%eax, %ecx			/* ecx = min(count, vidSize - dst) */
0:	
	subl	%ecx, %edx			/* count -= ecx */
	shll	$1, %edi			/* Convert to byte address */
	addl	vidOff, %edi		/* To video memory */
	testl	%esi, %esi			/* if source == 0, means blank the screen */
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
	movl	8(%ebp), %esi		/* source */
	movl	12(%ebp), %edi		/* dst */
	movl	16(%ebp), %edx		/* count */
	movw	vidSeg, %es			/* es = Segment containing video memory */
	cmpl	%edi, %esi			/* Copy up or down? */
	jb	.vvcDown
.vvcUp:
	cld							/* Direction is up */
.vvcUpLoop:
	andl	vidMask, %esi		/* Wrap addresses */
	andl	vidMask, %edi
	movl	%edx, %ecx			/* ecx = count */
	movl	vidSize, %eax		/* eax = vidSize */
	subl	%esi, %eax			/* eax = vidSize - src */
	cmpl	%eax, %ecx			
	jbe	0f
	movl	%eax, %ecx			/* ecx = min(count, vidSize - src) */
0:
	movl	vidSize, %eax		/* eax = vidSize */
	subl	%edi, %eax			/* eax = vidSize - dst */
	cmpl	%eax, %ecx			
	jbe	0f
	movl	%eax, %ecx			/* ecx = min(vidSize - dst, min(count, vidSize - src)) */
0:
	subl	%ecx, %edx			/* count -= ecx */
	calll	vvcCopy				/* Copy video words */
	testl	%edx, %edx			
	jnz	.vvcUpLoop				/* if count != 0, then jump to .vvcUpLoop */
	jmp	.vvcDone
.vvcDown:
	std							/* Direction is down */
	leal /*TODO*/	


