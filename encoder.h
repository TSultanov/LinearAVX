#ifndef __ENCODER_H__
#define __ENCODER_H__

#ifdef __cplusplus
extern "C" {
#endif
#include <xed/xed-interface.h>
int reencode_instructions(uint8_t* instructionPointer);
#ifdef __cplusplus
}
#endif

#endif /* __ENCODER_H__ */