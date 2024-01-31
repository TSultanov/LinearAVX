#ifndef __MEMMANAGER_H__
#define __MEMMANAGER_H__

#ifdef __cplusplus
extern "C" {
#endif
#include <xed/xed-interface.h>
#include <xed/xed-encode.h>


#include <emmintrin.h>
typedef struct {
    __m128 u[16];
    __m128 l[16];
} ymm_t;

ymm_t* get_ymm_for_thread(uint64_t tid);
uint8_t* alloc_executable(uint64_t size);
void jumptable_add_chunk(uint64_t location, void* chunk);
void* jumptable_get_chunk(uint64_t location);

#ifdef __cplusplus
}
#endif

#endif /* __MEMMANAGER_H__ */