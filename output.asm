section .data
    true db 'true', 0
    false db 'false', 0
    newline db 10
    result_header db "Logical Expression Results:", 10, 0
    A resd 1  ; Variable A from logical expressions
    B resd 1  ; Variable B from logical expressions
    C resd 1  ; Variable C from logical expressions
    t0 resd 1  ; Temporary variable for evaluations
    t1 resd 1  ; Temporary variable for evaluations
    t2 resd 1  ; Temporary variable for evaluations
    result_buffer resb 128  ; Buffer for result string
    step1 db "Evaluating expression: B = TRUE", 0
    step5 db "Evaluating expression: C = FALSE", 0
    step9 db "Evaluating expression: B OR C", 0

section .text
    global _start

_start:
    ; Initialize symbol table
    mov DWORD [A], 1    ; A = TRUE
    mov DWORD [B], 0    ; B = FALSE
    mov DWORD [C], 0    ; C = FALSE

    ; t0 = B OR C
    mov eax, [B]
    or eax, [C]
    mov [t0], eax

    ; Print variable values
    mov rsi, result_buffer
    mov DWORD [rsi], 'A = '
    add rsi, 4
    cmp DWORD [A], 1
    jne .a_false
    mov DWORD [rsi], 'TRUE'
    add rsi, 4
    jmp .a_done
.a_false:
    mov DWORD [rsi], 'FALS'
    add rsi, 4
    mov BYTE [rsi], 'E'
    inc rsi
.a_done:
    mov BYTE [rsi], 10
    inc rsi
    mov DWORD [rsi], 'B = '
    add rsi, 4
    cmp DWORD [B], 1
    jne .b_false
    mov DWORD [rsi], 'TRUE'
    add rsi, 4
    jmp .b_done
.b_false:
    mov DWORD [rsi], 'FALS'
    add rsi, 4
    mov BYTE [rsi], 'E'
    inc rsi
.b_done:
    mov BYTE [rsi], 10
    inc rsi
    mov DWORD [rsi], 'C = '
    add rsi, 4
    cmp DWORD [C], 1
    jne .c_false
    mov DWORD [rsi], 'TRUE'
    add rsi, 4
    jmp .c_done
.c_false:
    mov DWORD [rsi], 'FALS'
    add rsi, 4
    mov BYTE [rsi], 'E'
    inc rsi
.c_done:
    mov BYTE [rsi], 10
    inc rsi
    ; Print result of expression
    mov DWORD [rsi], 'Resu'
    add rsi, 4
    mov DWORD [rsi], 'lt o'
    add rsi, 4
    mov DWORD [rsi], 'f ex'
    add rsi, 4
    mov DWORD [rsi], 'pres'
    add rsi, 4
    mov DWORD [rsi], 'sion'
    add rsi, 4
    mov WORD [rsi], ': '
    add rsi, 2
    cmp DWORD [t0], 1
    jne .result_false
    mov DWORD [rsi], 'TRUE'
    add rsi, 4
    jmp .result_done
.result_false:
    mov DWORD [rsi], 'FALS'
    add rsi, 4
    mov BYTE [rsi], 'E'
    inc rsi
.result_done:
    mov BYTE [rsi], 0
    ; Print the buffer
    mov rax, 1      ; syscall number for write
    mov rdi, 1      ; file descriptor 1 = STDOUT
    mov rsi, result_buffer
    mov rdx, 128     ; max buffer size
    syscall
    ; Exit program
    mov rax, 60     ; syscall number for exit
    xor rdi, rdi    ; exit code 0
    syscall
