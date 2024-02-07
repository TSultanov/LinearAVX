#ifndef __ENCODER_H__
#define __ENCODER_H__

#include <_types/_uint64_t.h>
#ifdef __cplusplus
extern "C" {
#endif
#include <xed/xed-interface.h>
int reencode_instructions(uint8_t* instructionPointer);
#ifdef __cplusplus
}
#endif

#endif /* __ENCODER_H__ */