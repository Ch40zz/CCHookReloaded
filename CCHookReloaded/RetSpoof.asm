.686p
.xmm
.model flat
.code

@SpoofCall12@72 PROC
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
@SpoofCall12@72 ENDP


@SpoofCall12_Steam@76 PROC
	;
	; 0043BF48 | 83 C4 34              | add esp, 34h                   |
	; 0043BF6A | 8B C8                 | mov ecx, eax                   |
	; 0043BF6C | 85 FF                 | test edi, edi                  |
	; 0043BF6E | A1 38 53 A7 00        | mov eax, currentVM             |
	; 0043BF73 | 0F 45 C7              | cmovnz  eax, edi               |
	; 0043BF76 | 5F                    | pop edi                        |
	; 0043BF77 | A3 38 53 A7 00        | mov currentVM, eax             |
	; 0043BF7C | 8B C1                 | mov eax, ecx                   |
	; 0043BF7E | 5E                    | pop esi                        |
	; 0043BF7F | 5D                    | pop ebp                        |
	; 0043BF80 | C3                    | ret                            |
	; 
	; 
	; -------------------
	; SpoofCall:
	; ecx = VM_Call_vmMain return address (0x0043BF48)
	; edx = orig_vmMain
	; -------------------
	; 44 push 0 <- retaddr
	; 40 push 0 <- ebp
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
	
	; Preserve return address, ebp, esi, edi
	mov eax, [esp]
	mov [esp + 44h], eax
	mov [esp + 40h], ebp
	mov [esp + 3Ch], esi
	mov [esp + 38h], edi

	; Set fake return address
	mov [esp], ecx
	jmp edx
@SpoofCall12_Steam@76 ENDP


@SpoofCall16@92 PROC
	;
	; 0042813B     83 C4 44            add     esp, 44h
	; 0042813E     8B C8               mov     ecx, eax
	; ...
	; 00428154     A1 10 C8 9C 00      mov     eax, dword_9CC810
	; 00428159     85 FF               test    edi, edi
	; 0042815B     0F 45 C7            cmovnz  eax, edi
	; 0042815E     A3 10 C8 9C 00      mov     dword_9CC810, eax
	; 00428163     8B C1               mov     eax, ecx
	; 00428165     8B 4D FC            mov     ecx, [ebp+var_4]
	; 00428168     5F                  pop     edi
	; 00428169     33 CD               xor     ecx, ebp        ; StackCookie
	; 0042816B     5E                  pop     esi
	; 0042816C     E8 59 0D 34 00      call    @__security_check_cookie@4 ; __security_check_cookie(x)
	; 00428171     8B E5               mov     esp, ebp
	; 00428173     5D                  pop     ebp
	; 00428174     C3                  retn
	; 
	; 
	; -------------------
	; SpoofCall:
	; ecx = VM_Call_vmMain return address
	; edx = orig_vmMain
	; -------------------
	; 54 push 0 <- retaddr
	; 50 push 0 <- ebp
	; 4C push 0 <- esi
	; 48 push 0 <- edi
	; 44 push a16
	; 40 push a15
	; 3C push a14
	; 38 push a13
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

	; Grab stack cookie and prepare xor key
	mov eax, [esp + 50h]
	add esp, 50h
	xor eax, esp		; esp == future ebp value
	mov [esp - 4h], eax	; ~~
	sub esp, 50h

	; Preserve return address, ebp, esi, edi
	mov eax, [esp]
	mov [esp + 54h], eax
	mov [esp + 50h], ebp
	;mov [esp + 4Ch], esi	; Trashed, overwritten by security cookie
	mov [esp + 48h], edi

	lea ebp, [esp + 50h]

	; Set fake return address
	mov [esp], ecx
	jmp edx
@SpoofCall16@92 ENDP

end