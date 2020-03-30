#ifndef CALL32_H
#define CALL32_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void *_call32_stack;
void call32_init();
void call32_simple(void *);
void call32_switch_stack(void (*ptr)(void *userdata), void *stack, void *userdata);

#ifdef __cplusplus
}

namespace call32 {
    inline void init() {
        call32_init();
    }

    inline void invokeSimple(void *x) {
        call32_simple(x);
    }

    template <typename T>
    inline void switchStack(T const &s, void *stack = _call32_stack) {
        call32_switch_stack(+[](void *userdata) {
            (*((T*) userdata))();
        }, stack, (void *) &s);
    }
};
#endif

#endif /* CALL32_H */
