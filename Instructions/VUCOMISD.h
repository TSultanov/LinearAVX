#include "CompilableInstruction.h"
#include "xed/xed-encoder-hl.h"

class VUCOMISD : public CompilableInstruction<VUCOMISD> {
public:
    VUCOMISD(uint64_t rip, uint8_t ilen, xed_decoded_inst_t xedd) : CompilableInstruction(rip, ilen, xedd) {}

    static const inline InstructionMetadata Metadata = {
        .iclass = XED_ICLASS_VUCOMISD,
        .operandSets = {
            { 
                .vectorLength = 64,
                .operands = {{ .operand = XED_ENCODER_OPERAND_TYPE_REG, .regClass = XED_REG_CLASS_XMM },
                { .operand = XED_ENCODER_OPERAND_TYPE_MEM, .regClass = XED_REG_CLASS_INVALID }}
            },
            { 
                .vectorLength = 64,
                .operands = {{ .operand = XED_ENCODER_OPERAND_TYPE_REG, .regClass = XED_REG_CLASS_XMM },
                { .operand = XED_ENCODER_OPERAND_TYPE_REG, .regClass = XED_REG_CLASS_XMM }
                }
            },
        }
    };
private:
    void implementation(bool upper, bool compile_inline) {
        ucomisd(operands[0].toEncoderOperand(upper), operands[1].toEncoderOperand(upper));
    }
};