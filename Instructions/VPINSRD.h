#include "CompilableInstruction.h"
#include "xed/xed-encoder-hl.h"

class VPINSRD : public CompilableInstruction<VPINSRD> {
public:
    VPINSRD(uint64_t rip, uint8_t ilen, xed_decoded_inst_t xedd) : CompilableInstruction(rip, ilen, xedd) {}

    static const inline InstructionMetadata Metadata = {
        .iclass = XED_ICLASS_VPINSRD,
        .operandSets = {
            { 
                .vectorLength = 32,
                .operands = {{ .operand = XED_ENCODER_OPERAND_TYPE_REG, .regClass = XED_REG_CLASS_XMM },
                { .operand = XED_ENCODER_OPERAND_TYPE_REG, .regClass = XED_REG_CLASS_XMM },
                { .operand = XED_ENCODER_OPERAND_TYPE_REG, .regClass = XED_REG_CLASS_GPR32 },
                { .operand = XED_ENCODER_OPERAND_TYPE_IMM0, .immBits = 8 }
                }
            },
            { 
                .vectorLength = 32,
                .operands = {{ .operand = XED_ENCODER_OPERAND_TYPE_REG, .regClass = XED_REG_CLASS_XMM },
                { .operand = XED_ENCODER_OPERAND_TYPE_REG, .regClass = XED_REG_CLASS_XMM },
                { .operand = XED_ENCODER_OPERAND_TYPE_MEM },
                { .operand = XED_ENCODER_OPERAND_TYPE_IMM0, .immBits = 8 }
                }
            },
        }
    };
private:
    void implementation(bool upper, bool compile_inline) {
        map3opto2op(upper, [=](xed_encoder_operand_t const& op0, xed_encoder_operand_t const& op1) {
            pinsrd(op0, op1, operands[3].toEncoderOperand(upper));
        });

        if (operands[0].isXmm()) {
            zeroupperInternal(operands[0]);
        }
    }
};