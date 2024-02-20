#include "CompilableInstruction.h"

class VMOVMSKPD : public CompilableInstruction<VMOVMSKPD> {
public:
    VMOVMSKPD(uint64_t rip, uint8_t ilen, xed_decoded_inst_t xedd) : CompilableInstruction(rip, ilen, xedd) {}

    static const inline InstructionMetadata Metadata = {
        .iclass = XED_ICLASS_VMOVMSKPD,
        .operandSets = {
            { 
                .vectorLength = 32,
                .operands = {{ .operand = XED_ENCODER_OPERAND_TYPE_REG, .regClass = XED_REG_CLASS_GPR32 },
                { .operand = XED_ENCODER_OPERAND_TYPE_REG, .regClass = XED_REG_CLASS_XMM }}
            },
        }
    };
private:
    void implementation(bool upper, bool compile_inline) {
        if (usesYmm()) {
            debug_print("VMOVMSKPS: YMM registers support is not implemented!\n");
            exit(1);
        }

        movmskpd(operands[0].toEncoderOperand(upper), operands[1].toEncoderOperand(upper));
    }
};