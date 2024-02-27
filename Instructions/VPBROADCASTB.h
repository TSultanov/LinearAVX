#include "CompilableInstruction.h"
#include "xed/xed-encoder-hl.h"
#include "xed/xed-reg-class-enum.h"
#include <unistd.h>

class VPBROADCASTB : public CompilableInstruction<VPBROADCASTB> {
public:
    const uint32_t shufConst32 = 0x0;
    const __m128 shufConst = _mm_set1_ps(*(float*)&shufConst32);
    VPBROADCASTB(uint64_t rip, uint8_t ilen, xed_decoded_inst_t xedd) : CompilableInstruction(rip, ilen, xedd) {}

    static const inline InstructionMetadata Metadata = {
        .iclass = XED_ICLASS_VPBROADCASTB,
        .operandSets = {
            { 
                .vectorLength = 8,
                .operands = {{ .operand = XED_ENCODER_OPERAND_TYPE_REG, .regClass = XED_REG_CLASS_XMM },
                { .operand = XED_ENCODER_OPERAND_TYPE_MEM, .regClass = XED_REG_CLASS_INVALID }}
            },
            { 
                .vectorLength = 128,
                .operands = {{ .operand = XED_ENCODER_OPERAND_TYPE_REG, .regClass = XED_REG_CLASS_XMM },
                { .operand = XED_ENCODER_OPERAND_TYPE_REG, .regClass = XED_REG_CLASS_XMM }}
            },
            { 
                .vectorLength = 8,
                .operands = {{ .operand = XED_ENCODER_OPERAND_TYPE_REG, .regClass = XED_REG_CLASS_YMM },
                { .operand = XED_ENCODER_OPERAND_TYPE_MEM, .regClass = XED_REG_CLASS_INVALID }}
            },
            { 
                .vectorLength = 256,
                .operands = {{ .operand = XED_ENCODER_OPERAND_TYPE_REG, .regClass = XED_REG_CLASS_YMM },
                { .operand = XED_ENCODER_OPERAND_TYPE_REG, .regClass = XED_REG_CLASS_XMM }}
            },
        }
    };
private:
    void implementation(bool upper, bool compile_inline) override {
        if (operands[1].isMemoryOperand()) {
            auto memOp = operands[1].toEncoderOperand(false);
            memOp.width_bits = 64;
            movq(operands[0].toEncoderOperand(upper), memOp);
        } else {
            movq(operands[0].toEncoderOperand(upper), operands[1].toEncoderOperand(false));
        }
        withFreeReg([&](xed_reg_enum_t tempReg) {
            mov(tempReg, (uint64_t)&shufConst);
            pshufb(operands[0].toEncoderOperand(upper), xed_mem_b(tempReg, 128));
        });

        if (operands[0].isXmm()) {
            zeroupperInternal(operands[0]);
        }
    }
};
