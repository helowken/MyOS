/* This is the Filp table. It is an intermediary between file descriptors and
 * inodes. A slot is free if flip_count == 0.
 */

typedef struct {
	mode_t filp_mode;	/* RW bits, telling how file is opened */
	int filp_flags;		/* Flags from open and fcntl */
	int filp_count;		/* How many file descriptors share this slot? */
	Inode *filp_ino;	/* Pointer to the inode */
	off_t filp_pos;		/* File position */

	/* The following fields are for select() and are owned by the generic
	 * select() code (i.e., fd-type-specific select() code can't touch these).
	 */
	int filp_selectors;		/* select()ing processes blocking on this fd */
	int filp_select_ops;	/* Interested in these SEL_* operations */

	/* Following are for fd-type-specific select() */
	int filp_pipe_select_ops;
} Filp;

EXTERN Filp filp[NR_FILPS];

#define FILP_CLOSED		0	/* filp_mode: associated device closed */

#define NIL_FILP	(Filp *) 0	/* Indicates absence of a filp slot */

