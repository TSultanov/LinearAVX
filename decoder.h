#pragma once

#ifdef __cplusplus
extern "C" {
#endif
#include "xed/xed-decoded-inst.h"
void decode_instruction(unsigned char *inst, xed_decoded_inst_t *xedd, uint32_t *olen);
#ifdef __cplusplus
}
#endif