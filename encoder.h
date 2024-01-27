#ifndef __ENCODER_H__
#define __ENCODER_H__

#ifdef __cplusplus
extern "C" {
#endif
#include <xed/xed-interface.h>
void encode_instruction(xed_decoded_inst_t *xedd, uint8_t *buffer, const unsigned int ilen, unsigned int *olen, uint64_t tid);
#ifdef __cplusplus
}
#endif

#endif /* __ENCODER_H__ */