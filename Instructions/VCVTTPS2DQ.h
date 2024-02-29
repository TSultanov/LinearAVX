#include "CompilableInstruction.h"

class VCVTTPS2DQ : public CompilableInstruction<VCVTTPS2DQ> {
public:
    VCVTTPS2DQ(uint64_t rip, uint8_t ilen, xed_decoded_inst_t xedd) : CompilableInstruction(rip, ilen, xedd) {}

    static const inline InstructionMetadata Metadata = {
        .iclass = XED_ICLASS_VCVTTPS2DQ,
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
        cvttps2dq(operands[0].toEncoderOperand(upper), operands[1].toEncoderOperand(upper));

        zeroupperInternal(operands[0]);
    }
};