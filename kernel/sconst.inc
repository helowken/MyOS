# Miscellaneous constants used in assembler code.
W			=	4		# Machine word size

# Offsets in struct Proc. They MUST match proc.h
P_STACKBASE	=	0
GSREG		=	P_STACKBASE
FSREG		=	GSREG + 2		# 386 introduces FS and GS segments
ESREG		=	FSREG + 2
DSREG		=	ESREG + 2
DIREG		=	DSREG + 2
SIREG		=	DIREG + W
BPREG		=	SIREG + W
STREG		=	BPREG + W
BXREG		=	STREG + W
DXREG		=	BXREG + W
CXREG		=	DXREG + W
AXREG		=	CXREG + W
RETADDR		=	AXREG + W
PCREG		=	RETADDR + W
CSREG		=	PCREG + W
PSWREG		=	CSREG + W
SPREG		=	PSWREG + W
SSREG		=	SPREG + W
P_STACKTOP	=	SSREG + W
P_LDT_SEL	=	P_STACKTOP
P_LDT		=	P_LDT_SEL + W

Msize		=	9				# Size of a message in 32-bit words
