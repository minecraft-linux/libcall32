#ifndef CALL32_H
#define CALL32_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void call32_init();
void call32_simple(void *);

#ifdef __cplusplus
}

namespace call32 {
    inline void init() {
        call32_init();
    }

    inline void invokeSimple(void *x) {
        call32_simple(x);
    }
};
#endif

#endif /* CALL32_H */
