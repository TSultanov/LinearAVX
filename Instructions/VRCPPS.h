#include "CompilableInstruction.h"

class VRCPPS : public CompilableInstruction<VRCPPS> {
public:
    VRCPPS(uint64_t rip, uint8_t ilen, xed_decoded_inst_t xedd) : CompilableInstruction(rip, ilen, xedd) {}

    static const inline InstructionMetadata Metadata = {
        .iclass = XED_ICLASS_VRCPPS,
        .operandSets = {
            { 
                .vectorLength = 128,
                .operands = {
                    { .operand = XED_ENCODER_OPERAND_TYPE_REG, .regClass = XED_REG_CLASS_XMM },
                    { .operand = XED_ENCODER_OPERAND_TYPE_REG, .regClass = XED_REG_CLASS_XMM }
                }
            },
            { 
                .vectorLength = 128,
                .operands = {
                    { .operand = XED_ENCODER_OPERAND_TYPE_REG, .regClass = XED_REG_CLASS_XMM },
                    { .operand = XED_ENCODER_OPERAND_TYPE_MEM }
                }
            },
            { 
                .vectorLength = 256,
                .operands = {
                    { .operand = XED_ENCODER_OPERAND_TYPE_REG, .regClass = XED_REG_CLASS_YMM },
                    { .operand = XED_ENCODER_OPERAND_TYPE_REG, .regClass = XED_REG_CLASS_YMM }
                }
            },
            { 
                .vectorLength = 256,
                .operands = {
                    { .operand = XED_ENCODER_OPERAND_TYPE_REG, .regClass = XED_REG_CLASS_YMM },
                    { .operand = XED_ENCODER_OPERAND_TYPE_MEM }
                }
            },
        }
    };
private:
    void implementation(bool upper, bool compile_inline) {
        rcpps(operands[0].toEncoderOperand(upper), operands[1].toEncoderOperand(upper));

        if (operands[0].isXmm()) {
            zeroupperInternal(operands[0]);
        }
    }
};