#ifndef __MEMMANAGER_H__
#define __MEMMANAGER_H__

#ifdef __cplusplus
extern "C" {
#endif
#include <xed/xed-interface.h>
#include <xed/xed-encode.h>
#ifdef __cplusplus
}
#endif

#include <emmintrin.h>
#include <vector>

typedef struct {
    __m128 u[16];
    __m128 l[16];
} ymm_t;

ymm_t* get_ymm_for_thread(uint64_t tid);
uint8_t* encode_requests(std::vector<xed_encoder_request_t> requests);

#endif /* __MEMMANAGER_H__ */