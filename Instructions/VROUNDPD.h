#include "CompilableInstruction.h"
#include "xed/xed-encoder-hl.h"

class VROUNDPD : public CompilableInstruction<VROUNDPD> {
public:
    VROUNDPD(uint64_t rip, uint8_t ilen, xed_decoded_inst_t xedd) : CompilableInstruction(rip, ilen, xedd) {}

    static const inline InstructionMetadata Metadata = {
        .iclass = XED_ICLASS_VROUNDPD,
        .operandSets = {
            { 
                .vectorLength = 128,
                .operands = {{ .operand = XED_ENCODER_OPERAND_TYPE_REG, .regClass = XED_REG_CLASS_XMM },
                { .operand = XED_ENCODER_OPERAND_TYPE_REG, .regClass = XED_REG_CLASS_XMM },
                { .operand = XED_ENCODER_OPERAND_TYPE_IMM0, .immBits = 8 }}
            },
            { 
                .vectorLength = 128,
                .operands = {{ .operand = XED_ENCODER_OPERAND_TYPE_REG, .regClass = XED_REG_CLASS_XMM },
                { .operand = XED_ENCODER_OPERAND_TYPE_MEM },
                { .operand = XED_ENCODER_OPERAND_TYPE_IMM0, .immBits = 8 }}
            },
            { 
                .vectorLength = 256,
                .operands = {{ .operand = XED_ENCODER_OPERAND_TYPE_REG, .regClass = XED_REG_CLASS_YMM },
                { .operand = XED_ENCODER_OPERAND_TYPE_REG, .regClass = XED_REG_CLASS_YMM },
                { .operand = XED_ENCODER_OPERAND_TYPE_IMM0, .immBits = 8 }}
            },
            { 
                .vectorLength = 256,
                .operands = {{ .operand = XED_ENCODER_OPERAND_TYPE_REG, .regClass = XED_REG_CLASS_YMM },
                { .operand = XED_ENCODER_OPERAND_TYPE_MEM },
                { .operand = XED_ENCODER_OPERAND_TYPE_IMM0, .immBits = 8 }}
            },
        }
    };
private:
    void implementation(bool upper, bool compile_inline) {
        roundpd(operands[0].toEncoderOperand(upper), operands[1].toEncoderOperand(upper), operands[2].toEncoderOperand(upper));

        if (operands[0].isXmm()) {
            zeroupperInternal(operands[0]);
        }
    }
};