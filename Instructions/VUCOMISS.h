#include "CompilableInstruction.h"
#include "xed/xed-encoder-hl.h"

class VUCOMISS : public CompilableInstruction<VUCOMISS> {
public:
    VUCOMISS(uint64_t rip, uint8_t ilen, xed_decoded_inst_t xedd) : CompilableInstruction(rip, ilen, xedd) {}

    static const inline InstructionMetadata Metadata = {
        .iclass = XED_ICLASS_VUCOMISS,
        .operandSets = {
            { 
                .vectorLength = 32,
                .operands = {{ .operand = XED_ENCODER_OPERAND_TYPE_REG, .regClass = XED_REG_CLASS_XMM },
                { .operand = XED_ENCODER_OPERAND_TYPE_MEM, .regClass = XED_REG_CLASS_INVALID }}
            },
            { 
                .vectorLength = 128,
                .operands = {{ .operand = XED_ENCODER_OPERAND_TYPE_REG, .regClass = XED_REG_CLASS_XMM },
                { .operand = XED_ENCODER_OPERAND_TYPE_REG, .regClass = XED_REG_CLASS_XMM }
                }
            },
        }
    };
private:
    void implementation(bool upper, bool compile_inline) {
        ucomiss(operands[0].toEncoderOperand(upper), operands[1].toEncoderOperand(upper));
    }
};