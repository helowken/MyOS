/* GCC places some program related infomation such as "switch instruction address" 
   in the ".rodata" section When compiling c source files. 


   Code of sys_call() in kernel/proc.c:
	```
	switch (function) {
	1a64:   83 7d e0 08             cmpl   $0x8,-0x20(%ebp)
	1a68:   0f 87 b1 00 00 00       ja     1b1f <sys_call+0x22a>
	1a6e:   8b 45 e0                mov    -0x20(%ebp),%eax
	1a71:   c1 e0 02                shl    $0x2,%eax
	1a74:   05 1c 66 00 00          add    $0x661c,%eax
	1a79:   8b 00                   mov    (%eax),%eax
	1a7b:   ff e0                   jmp    *%eax
		case SENDREC:	# 1
	1a7d:
		...
		case SEND:		# 2
	1a8e:
		...
		case RECEIVE:	# 3
	1aaf:
		...
		case NOTIFY:	# 4
	1add:
		...
		case ECHO:		# 8
	1af1:
		...
		default:
	1b1f:
		...
	```


	When PC goes to "1a74", it will add "0x661c" to "eax". "0x661c" is the start 
	address of the switch branch table stored in ".rodata" section:
	```
	0x0000661c <bogus+       0>:	0x00001b1f	0x00001a8e	0x00001aaf	0x00001a7d
	0x0000662c <bogus+      16>:	0x00001add	0x00001b1f	0x00001b1f	0x00001b1f
	0x0000663c <bogus+      32>:	0x00001af1
	```

	When PC goes to "1a79", it will dereference the address in "eax" with the "ds".
	Error occurs if "ds" does not equal to "cs" since normally ".rodata" is stored 
	with ".text" and referenced by "cs".

	Since we want to place "text" and "data" in memory separately, "ds" and "cs" 
	will be probably not the same. So we need this linker script to put the 
	".rodata" section with ".data" and ".bss".
 */


OUTPUT_FORMAT("elf32-i386", "elf32-i386",
			          "elf32-i386")
OUTPUT_ARCH(i386)

SECTIONS
{
  . = SEGMENT_START("text-segment", 0x400000) + SIZEOF_HEADERS;
  .text           :
  {
    *(.text)
  }
  PROVIDE (__etext = .);
  PROVIDE (_etext = .);
  PROVIDE (etext = .);

  . = DATA_SEGMENT_ALIGN (CONSTANT (MAXPAGESIZE), CONSTANT (COMMONPAGESIZE));
  .data           :
  {
    *(.data)
    SORT(CONSTRUCTORS)
  }
  .rodata         : 
  { 
	*(.rodata) 
  }
  _edata = .; 
  PROVIDE (edata = .);
  . = .;
  __bss_start = .;
  .bss            :
  {
   *(.bss)
   *(COMMON)
   . = ALIGN(. != 0 ? 64 / 8 : 1);
  }
  . = ALIGN(64 / 8);
  _end = .; 
  PROVIDE (end = .);
  . = DATA_SEGMENT_END (.);
}
