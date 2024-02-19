#ifndef __MEMMANAGER_H__
#define __MEMMANAGER_H__

#ifdef __cplusplus
extern "C" {
#endif
#include <xed/xed-interface.h>
#include <xed/xed-encode.h>


#include <emmintrin.h>
volatile __m128 *get_ymm_storage();
uint8_t* alloc_executable(uint64_t size);
void jumptable_add_chunk(uint64_t location, void* chunk);
void* jumptable_get_chunk(uint64_t location);

#ifdef __cplusplus
}
#endif

#endif /* __MEMMANAGER_H__ */