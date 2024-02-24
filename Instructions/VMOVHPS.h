#include "CompilableInstruction.h"

class VMOVHPS : public CompilableInstruction<VMOVHPS> {
public:
    VMOVHPS(uint64_t rip, uint8_t ilen, xed_decoded_inst_t xedd) : CompilableInstruction(rip, ilen, xedd) {}

    static const inline InstructionMetadata Metadata = {
        .iclass = XED_ICLASS_VMOVHPS,
        .operandSets = {
            { 
                .vectorLength = 64,
                .operands = {{ .operand = XED_ENCODER_OPERAND_TYPE_REG, .regClass = XED_REG_CLASS_XMM },
                { .operand = XED_ENCODER_OPERAND_TYPE_REG, .regClass = XED_REG_CLASS_XMM },
                { .operand = XED_ENCODER_OPERAND_TYPE_MEM, }}
            },
            { 
                .vectorLength = 64,
                .operands = {{ .operand = XED_ENCODER_OPERAND_TYPE_MEM},
                { .operand = XED_ENCODER_OPERAND_TYPE_REG, .regClass = XED_REG_CLASS_XMM }}
            },
        }
    };
private:
    void implementation(bool upper, bool compile_inline) {
        if (operands.size() == 3) {
            map3opto2op(upper, [&](xed_encoder_operand_t const& op0, xed_encoder_operand_t const& op1) {
                movhps(op0, op1);
            });

            if (operands[0].isXmm()) {
                zeroupperInternal(operands[0]);
            }
        } else {
            movhps(operands[0].toEncoderOperand(upper), operands[1].toEncoderOperand(upper));
        }
    }
};