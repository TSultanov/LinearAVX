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
    __m128 ymm0u;
    __m128 ymm0l_temp;
    __m128 ymm1u;
    __m128 ymm1l_temp;
    __m128 ymm2u;
    __m128 ymm2l_temp;
    __m128 ymm3u;
    __m128 ymm3l_temp;
    __m128 ymm4u;
    __m128 ymm4l_temp;
    __m128 ymm5u;
    __m128 ymm5l_temp;
    __m128 ymm6u;
    __m128 ymm6l_temp;
    __m128 ymm7u;
    __m128 ymm7l_temp;
    __m128 ymm8u;
    __m128 ymm8l_temp;
    __m128 ymm9u;
    __m128 ymm9l_temp;
    __m128 ymm10u;
    __m128 ymm10l_temp;
    __m128 ymm11u;
    __m128 ymm11l_temp;
    __m128 ymm12u;
    __m128 ymm12l_temp;
    __m128 ymm13u;
    __m128 ymm13l_temp;
    __m128 ymm14u;
    __m128 ymm14l_temp;
    __m128 ymm15u;
    __m128 ymm15l_temp;
} ymm_upper_t;

ymm_upper_t* get_ymm_for_thread(uint64_t tid);
uint8_t* encode_requests(std::vector<xed_encoder_request_t> requests);

#endif /* __MEMMANAGER_H__ */