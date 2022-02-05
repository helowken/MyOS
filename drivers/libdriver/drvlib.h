#include "ibm/partition.h"

/* BIOS parameter table layout. */
#define bp_cylinders(t)		(* (u16_t *) (&(t)[0]))
#define bp_heads(t)			(* (u8_t *) (&(t)[2]))
#define bp_precomp(t)		(* (u16_t *) (&(t)[5]))
#define bp_sectors(t)		(* (u8_t *) (&(t)[14]))

#define DEV_PER_DRIVE	(1 + NR_PARTITIONS)
#define MINOR_d0p0s0	128
#define P_FLOPPY		0
#define P_PRIMARY		1
#define P_SUB			2
