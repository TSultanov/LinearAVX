#pragma once

#include "xed/xed-encoder-hl.h"
#include "xed/xed-operand-type-enum.h"
#include "xed/xed-reg-class-enum.h"
#include "xed/xed-types.h"
#include <array>
#include <vector>
#ifdef __cplusplus
extern "C" {
#endif

#include "xed/xed-iclass-enum.h"
#include "xed/xed-operand-enum.h"

#ifdef __cplusplus
}
#endif

struct OperandMetadata {
    const xed_encoder_operand_type_t operand;
    const xed_reg_class_enum_t regClass;
    const xed_uint_t immBits;
    const bool setImmValue = false;
    const uint64_t immValue = 0;
};

struct OperandsMetadata {
    const xed_bits_t vectorLength;
    const std::vector<OperandMetadata> operands;
};

struct InstructionMetadata {
    const xed_iclass_enum_t iclass;
    const std::vector<OperandsMetadata> operandSets;
};