#include "../include/call32/malloc32.h"

#include <malloc.h>
#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#define MMAP_MAX_PAGES 100000

static void *mmap_start, *mmap_end;

void *malloc32(uint32_t s) {
    size_t page_size = sysconf(_SC_PAGE_SIZE);
    if (!mmap_start) {
        mmap_start = mmap((void *) 0x1000, page_size * MMAP_MAX_PAGES, PROT_READ | PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS|MAP_32BIT, 0, 0);
        mmap_end = (void *) ((size_t) mmap_start + page_size * MMAP_MAX_PAGES);
    }

    void *ret = mmap_start;
    void *end = (void *) ((size_t) ret + s);
    if (end >= mmap_end) {
        fprintf(stderr, "malloc32: out of memory\n");
        abort();
    }
    mmap_start = end;
    return ret;
}
void *calloc32(uint32_t a, uint32_t b) {
    void *r = malloc32(a * b);
    memset(r, 0, a * b);
    return r;
}
void *realloc32(void *p, uint32_t s) {
    void *r = malloc32(s);
    memcpy(r, p, s);
    free32(p);
    return r;
}
void free32(void *p) {
}
