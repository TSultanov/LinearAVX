#include "CompilableInstruction.h"

class VMOVLHPS : public CompilableInstruction<VMOVLHPS> {
public:
    VMOVLHPS(uint64_t rip, uint8_t ilen, xed_decoded_inst_t xedd) : CompilableInstruction(rip, ilen, xedd) {}

    static const inline InstructionMetadata Metadata = {
        .iclass = XED_ICLASS_VMOVLHPS,
        .operandSets = {
            { 
                .vectorLength = 128,
                .operands = {{ .operand = XED_ENCODER_OPERAND_TYPE_REG, .regClass = XED_REG_CLASS_XMM },
                { .operand = XED_ENCODER_OPERAND_TYPE_REG, .regClass = XED_REG_CLASS_XMM },
                { .operand = XED_ENCODER_OPERAND_TYPE_REG, .regClass = XED_REG_CLASS_XMM }}
            }
        }
    };
private:
    void implementation(bool upper, bool compile_inline) {
        map3opto2op(upper, [&](xed_encoder_operand_t const& op0, xed_encoder_operand_t const& op1) {
            movlhps(op0, op1);
        });

        if (operands[0].isXmm()) {
            zeroupperInternal(operands[0]);
        }
    }
};