#include "CompilableInstruction.h"
#include "xed/xed-decoded-inst-api.h"
#include "xed/xed-operand-accessors.h"
#include <unistd.h>

class VMOVSD : public CompilableInstruction<VMOVSD> {
public:
    VMOVSD(uint64_t rip, uint8_t ilen, xed_decoded_inst_t xedd) : CompilableInstruction(rip, ilen, xedd) {}

    static const inline InstructionMetadata Metadata = {
        .iclass = XED_ICLASS_VMOVSD,
        .operandSets = {
            { 
                .vectorLength = 64,
                .operands = {{ .operand = XED_ENCODER_OPERAND_TYPE_REG, .regClass = XED_REG_CLASS_XMM },
                { .operand = XED_ENCODER_OPERAND_TYPE_REG, .regClass = XED_REG_CLASS_XMM },
                { .operand = XED_ENCODER_OPERAND_TYPE_REG, .regClass = XED_REG_CLASS_XMM },
                }
            },
            { 
                .vectorLength = 64,
                .operands = {{ .operand = XED_ENCODER_OPERAND_TYPE_REG, .regClass = XED_REG_CLASS_XMM },
                { .operand = XED_ENCODER_OPERAND_TYPE_MEM },
                }
            },
            { 
                .vectorLength = 64,
                .operands = {
                    { .operand = XED_ENCODER_OPERAND_TYPE_MEM },
                    { .operand = XED_ENCODER_OPERAND_TYPE_REG, .regClass = XED_REG_CLASS_XMM },
                }
            },
        }
    };
private:
    void implementation(bool upper, bool compile_inline) override {
        assert(operands.size() == 2 || operands.size() == 3);
        if (operands.size() == 2) {
            movsd(operands[0].toEncoderOperand(upper), operands[1].toEncoderOperand(upper));
        }
        else if (operands.size() == 3) {
            if (operands[0].reg() == operands[1].reg()) {
                movsd(operands[0].toEncoderOperand(upper), operands[2].toEncoderOperand(upper));
            } else if (operands[0].reg() == operands[2].reg()) {
                withPreserveXmmReg(operands[1], [=]() {
                    movsd(operands[1].toEncoderOperand(upper), operands[2].toEncoderOperand(upper));
                    movups(operands[0].toEncoderOperand(upper), operands[1].toEncoderOperand(upper));
                });
            } else {
                movups(operands[0].toEncoderOperand(upper), operands[1].toEncoderOperand(upper));
                movsd(operands[0].toEncoderOperand(upper), operands[2].toEncoderOperand(upper));
            }
        }

        if (operands[0].isXmm()) {
            zeroupperInternal(operands[0]);
        }
    }
};
