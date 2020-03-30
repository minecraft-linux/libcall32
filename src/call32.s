    .intel_syntax noprefix
    .text
    .globl _call32_simple_d_start
    .globl _call32_simple_d_end
    .globl _call32_simple_d_call64
    .globl _call32_simple_d_argdata
    .globl _call32_simple_d_stack
    .globl _call32_simple_d_offset
    .globl _call32_init_asm
    .globl call32_simple
    .globl call32_switch_stack

_call32_simple_d_start:
_call32_simple_d_call64:
	push    rbp
	mov     rax, 0
	lcall	[_call32_simple_d_argdata+rip]
	pop     rbp
	ret

_call32_simple_d_argdata:
.int _call32_simple_d_call32 - _call32_simple_d_call64
.int 0x23

_call32_simple_d_stack:
.quad 0

    .code32
_call32_simple_d_call32:
    call    edi
    lret
    .code64

_call32_simple_d_end:

_call32_init_asm:
    mov     ax, 0x2b
    mov     ds, ax
    mov     es, ax
    ret

call32_switch_stack:
	mov	    rax, rsp
    mov     rsp, rsi
    push    rax
    mov     rax, rdi
    mov     rdi, rdx
    call    rax
    pop     rsp
    ret

call32_simple:
    mov     rax, [_call32_simple_d_offset+rip]
    add     rax, (_call32_simple_d_call64 - _call32_simple_d_start)
    jmp     rax
