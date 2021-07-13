/** ndisam cannot distinguish between code and data
 * just look at the address and lookup the ascii table for the data
 *
00000000  55                push ebp
00000001  89E5              mov ebp,esp
00000003  83EC10            sub esp,byte +0x10      
00000006  C745FC10000000    mov dword [ebp-0x4],0x10   ; 0x10 is an address for "Hello"
0000000D  90                nop
0000000E  C9                leave
0000000F  C3                ret
00000010  48                dec eax       ; 0x48='H'
00000011  656C              gs insb       ; 0x65='e' 0x6C='l'
00000013  6C                insb          ; 0x6C='l' 
00000014  6F                outsd         ; 0x6F='o'
00000015  0000              add [eax],al  ; 0x0='\0'
**/

void my_function() {
	char* my_string = "Hello";
}
