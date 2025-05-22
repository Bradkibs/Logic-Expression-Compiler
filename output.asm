section .data
    true_str db "TRUE", 0
    false_str db "FALSE", 0
    equals_str db " = ", 0
    newline db 10, 0
    var_values_str db "Variable values:", 10, 0
    result_text db "Result: ", 0
    completion_text db 10, "Completed evaluation of all expressions", 10, 0
    output_filename db "output.txt", 0
    result_header db "Logical Expression Results:", 10, 0
    evaluation_header db "========== Logical Expression Evaluation ==========
", 0
    variables_header db "Variables:
", 0
    expression_header db "Expression:
", 0
    ; Variables from the symbol table
    ; Temporary variables for evaluations
    t0 resd 1  ; Temporary variable for evaluations
    t1 resd 1  ; Temporary variable for evaluations
    t2 resd 1  ; Temporary variable for evaluations
    result_buffer resb 256  ; Buffer for result string
    var_buffer resb 64    ; Buffer for variable names
    step1 db "Evaluating expression: R = TRUE", 0
    step4 db "Evaluating expression: S = FALSE", 0
    step7 db "Evaluating expression: T = TRUE", 0
    step10 db "Evaluating expression: NEW = TRUE", 0
    step13 db "Evaluating expression: INTERESTING_VARIABLE = FALSE", 0
    step16 db "Evaluating expression: R AND S OR T AND NEW", 0

section .text
    global _start

; String length function
strlen:
    push rbx
    mov rbx, rdi
    xor rax, rax
.strlen_loop:
    cmp byte [rbx], 0
    je .strlen_end
    inc rax
    inc rbx
    jmp .strlen_loop
.strlen_end:
    pop rbx
    ret

; String copy function
strcpy:
    push rdi
    push rsi
    push rdx
.strcpy_loop:
    mov dl, [rsi]
    mov [rdi], dl
    cmp dl, 0
    je .strcpy_end
    inc rdi
    inc rsi
    jmp .strcpy_loop
.strcpy_end:
    pop rdx
    pop rsi
    pop rdi
    ret

_start:
    ; Initialize variables with their assigned values

    ; t0 = t1 OR t4
    mov eax, [t1]
    or eax, [t4]
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
    ; Create/Open output.txt file
    mov rax, 2          ; syscall: open
    mov rdi, output_filename ; filename pointer
    mov rsi, 65         ; O_WRONLY | O_CREAT
    mov rdx, 0666o      ; permissions
    syscall
    mov r12, rax        ; save file descriptor

    ; Write evaluation header
    mov rax, 1          ; syscall: write
    mov rdi, r12        ; file descriptor
    mov rsi, evaluation_header
    mov rdx, 51         ; length of header
    syscall

    ; Write variables section
    mov rax, 1          ; syscall: write
    mov rdi, r12        ; file descriptor
    mov rsi, variables_header
    mov rdx, 11         ; length of header
    syscall

    ; Write expression section
    mov rax, 1          ; syscall: write
    mov rdi, r12        ; file descriptor
    mov rsi, expression_header
    mov rdx, 13         ; length of header
    syscall

    mov rsi, result_buffer
    mov DWORD [rsi], "Resu"
    add rsi, 4
    mov DWORD [rsi], "lt: "
    add rsi, 4
    cmp DWORD [t0], 1
    je .result_true_txt
    mov DWORD [rsi], 'FALS'
    add rsi, 4
    mov BYTE [rsi], 'E'
    inc rsi
    jmp .result_done_txt
.result_true_txt:
    mov DWORD [rsi], 'TRUE'
    add rsi, 4
.result_done_txt:
    mov BYTE [rsi], 10
    inc rsi
    mov BYTE [rsi], 0
    mov rax, 1          ; syscall: write
    mov rdi, r12        ; file descriptor
    mov rsi, result_buffer
    mov rdx, 256        ; max buffer size
    syscall

    mov rax, 1          ; syscall: write
    mov rdi, r12        ; file descriptor
    mov rsi, completion_text
    mov rdx, 44         ; length
    syscall

    mov rax, 3          ; syscall: close
    mov rdi, r12        ; file descriptor
    syscall

    ; Exit program
    mov rax, 60     ; syscall number for exit
    xor rdi, rdi    ; exit code 0
    syscall
