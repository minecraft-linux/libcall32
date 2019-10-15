#ifndef CALL32_MALLOC32_H
#define CALL32_MALLOC32_H

#include <stdint.h>

void *malloc32(uint32_t);
void *calloc32(uint32_t, uint32_t);
void *realloc32(void *, uint32_t);
void free32(void *);

#endif /* CALL32_MALLOC32_H */
