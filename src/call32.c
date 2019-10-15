#include "../include/call32/malloc32.h"
#include <stddef.h>
#include <sys/mman.h>
#include <string.h>

void *_call32_stack;
void *_call32_simple_d_offset;
extern void *_call32_simple_d_start;
extern void *_call32_simple_d_end;
extern void *_call32_simple_d_argdata;
extern void *_call32_simple_d_stack;

void _call32_init_asm();

void call32_init() {
    _call32_stack = malloc32(16 * 1024 * 1024);
    _call32_stack = (void *) ((size_t) _call32_stack + 16 * 1024 * 1024 - 0x10);
/*
    _call32_32bittramp = mmap(NULL, 16, PROT_READ|PROT_WRITE|PROT_EXEC, MAP_PRIVATE|MAP_ANONYMOUS|MAP_32BIT, 0, 0);
    unsigned char x32bittramp[] = {0xff, 0xd7, 0xcb};
    memcpy(_call32_32bittramp, x32bittramp, sizeof(_call32_32bittramp));

    _call32_32bittramp = (void *) ((size_t) _call32_32bittramp | (0x23L << 32));*/
    size_t size = (size_t) &_call32_simple_d_end - (size_t) &_call32_simple_d_start;
    _call32_simple_d_offset = mmap(NULL, size, PROT_READ|PROT_WRITE|PROT_EXEC, MAP_PRIVATE|MAP_ANONYMOUS|MAP_32BIT, 0, 0);
    memcpy(_call32_simple_d_offset, (void *) &_call32_simple_d_start, size);
    *(int32_t *) ((size_t) _call32_simple_d_offset + (size_t) &_call32_simple_d_argdata - (size_t) &_call32_simple_d_start) +=
            (int32_t) (size_t) _call32_simple_d_offset;
    *(void **) ((size_t) _call32_simple_d_offset + (size_t) &_call32_simple_d_stack - (size_t) &_call32_simple_d_start) =
            _call32_stack;

    _call32_init_asm();
}
