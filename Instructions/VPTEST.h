#include "CompilableInstruction.h"

class VPTEST : public CompilableInstruction<VPTEST> {
public:
    VPTEST(uint64_t rip, uint8_t ilen, xed_decoded_inst_t xedd) : CompilableInstruction(rip, ilen, xedd) {}

    static const inline InstructionMetadata Metadata = {
        .iclass = XED_ICLASS_VPTEST,
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
        ptest(operands[0].toEncoderOperand(upper), operands[1].toEncoderOperand(upper));
    }
};