#include "CompilableInstruction.h"
#include "xed/xed-decoded-inst-api.h"
#include "xed/xed-operand-accessors.h"

class VMOVD : public CompilableInstruction<VMOVD> {
public:
    VMOVD(uint64_t rip, uint8_t ilen, xed_decoded_inst_t xedd) : CompilableInstruction(rip, ilen, xedd) {}

    static const inline InstructionMetadata Metadata = {
        .iclass = XED_ICLASS_VMOVD,
        .operandSets = {
            { 
                .vectorLength = 64,
                .operands = {{ .operand = XED_ENCODER_OPERAND_TYPE_REG, .regClass = XED_REG_CLASS_XMM },
                { .operand = XED_ENCODER_OPERAND_TYPE_REG, .regClass = XED_REG_CLASS_GPR64 }}
            },
            { 
                .vectorLength = 64,
                .operands = {{ .operand = XED_ENCODER_OPERAND_TYPE_REG, .regClass = XED_REG_CLASS_XMM },
                { .operand = XED_ENCODER_OPERAND_TYPE_MEM }}
            },
            { 
                .vectorLength = 64,
                .operands = {{ .operand = XED_ENCODER_OPERAND_TYPE_REG, .regClass = XED_REG_CLASS_GPR64 },
                { .operand = XED_ENCODER_OPERAND_TYPE_REG, .regClass = XED_REG_CLASS_XMM }}
            },
            { 
                .vectorLength = 64,
                .operands = {{ .operand = XED_ENCODER_OPERAND_TYPE_MEM },
                { .operand = XED_ENCODER_OPERAND_TYPE_REG, .regClass = XED_REG_CLASS_XMM }}
            },
        }
    };
private:
    void implementation(bool upper, bool compile_inline) override {
        movd(operands[0].toEncoderOperand(upper), operands[1].toEncoderOperand(upper));

        if (operands[0].isXmm()) {
            zeroupperInternal(operands[0]);
        }
    }
};
