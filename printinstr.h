#pragma once

#ifdef __cplusplus
extern "C" {
#endif
#include "xed/xed-decoded-inst.h"
void print_instr(xed_decoded_inst_t* xedd);
#ifdef __cplusplus
}
#endif