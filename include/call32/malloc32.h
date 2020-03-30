#ifndef CALL32_MALLOC32_H
#define CALL32_MALLOC32_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

void *malloc32(uint32_t);
void *calloc32(uint32_t, uint32_t);
void *realloc32(void *, uint32_t);
void free32(void *);

#ifdef __cplusplus
}
#endif

#endif /* CALL32_MALLOC32_H */
