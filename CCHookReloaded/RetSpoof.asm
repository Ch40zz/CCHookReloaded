.686
.xmm
.model flat
.code

ALIAS <@SpoofCall12@72> = <SpoofCall12>

public SpoofCall12
SpoofCall12 PROC
	;
	; 00448AE2 | 83 C4 34              | add esp,34                         |
	; 00448B0C | 85 FF                 | test edi,edi                       |
	; 00448B0E | 74 06              <--| je et.448B16                       |
	; 00448B10 | 89 3D F0 29 02 01  |  |   mov dword ptr ds:[10229F0],edi   |
	; 00448B16 | 5F                 -->| pop edi                            |
	; 00448B17 | 5E                    | pop esi                            |
	; 00448B18 | C3                    | ret                                |
	; 
	; 
	; -------------------
	; SpoofCall:
	; ecx = VM_Call_vmMain return address (0x00448AE2)
	; edx = orig_vmMain
	; -------------------
	; 40 push 0 <- retaddr
	; 3C push 0 <- esi
	; 38 push 0 <- edi
	; 34 push a12
	; 30 push a11
	; 2C push a10
	; 28 push a9
	; 24 push a8
	; 20 push a7
	; 1C push a6
	; 18 push a5
	; 14 push a4
	; 10 push a3
	; C  push a2
	; 8  push a1
	; 4  push id
	; 0  retaddr
	
	; Preserve return address, esi, edi
	mov eax, [esp]
	mov [esp + 40h], eax
	mov [esp + 3Ch], esi
	mov [esp + 38h], edi

	; Set fake return address
	mov [esp], ecx
	jmp edx
SpoofCall12 ENDP

end