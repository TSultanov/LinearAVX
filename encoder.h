#ifndef __ENCODER_H__
#define __ENCODER_H__

#include <xed/xed-interface.h>

void encode_instruction(xed_decoded_inst_t *xedd, uint8_t *buffer, const unsigned int ilen, unsigned int *olen);

#endif /* __ENCODER_H__ */