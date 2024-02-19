#include "CompilableInstruction.h"
#include "xed/xed-reg-class-enum.h"

class VPERMILPS : public CompilableInstruction<VPERMILPS> {
public:
    VPERMILPS(uint64_t rip, uint8_t ilen, xed_decoded_inst_t xedd) : CompilableInstruction(rip, ilen, xedd) {}

    static const inline InstructionMetadata Metadata = {
        .iclass = XED_ICLASS_VPERMILPS,
        .operandSets = {
            // { 
            //     .vectorLength = 128,
            //     .operands = {{ .operand = XED_ENCODER_OPERAND_TYPE_REG, .regClass = XED_REG_CLASS_XMM },
            //     { .operand = XED_ENCODER_OPERAND_TYPE_REG, .regClass = XED_REG_CLASS_XMM },
            //     { .operand = XED_ENCODER_OPERAND_TYPE_MEM, .regClass = XED_REG_CLASS_INVALID }}
            // },
            // { 
            //     .vectorLength = 128,
            //     .operands = {{ .operand = XED_ENCODER_OPERAND_TYPE_REG, .regClass = XED_REG_CLASS_XMM },
            //     { .operand = XED_ENCODER_OPERAND_TYPE_REG, .regClass = XED_REG_CLASS_XMM },
            //     { .operand = XED_ENCODER_OPERAND_TYPE_REG, .regClass = XED_REG_CLASS_XMM }
            //     }
            // },
            // { 
            //     .vectorLength = 128,
            //     .operands = {{ .operand = XED_ENCODER_OPERAND_TYPE_REG, .regClass = XED_REG_CLASS_XMM },
            //     { .operand = XED_ENCODER_OPERAND_TYPE_REG, .regClass = XED_REG_CLASS_XMM },
            //     { .operand = XED_ENCODER_OPERAND_TYPE_IMM0, .regClass = XED_REG_CLASS_INVALID }
            //     }
            // }, // TODO tests
            // { 
            //     .vectorLength = 256,
            //     .operands = {{ .operand = XED_ENCODER_OPERAND_TYPE_REG, .regClass = XED_REG_CLASS_YMM },
            //     { .operand = XED_ENCODER_OPERAND_TYPE_REG, .regClass = XED_REG_CLASS_YMM },
            //     { .operand = XED_ENCODER_OPERAND_TYPE_MEM, .regClass = XED_REG_CLASS_INVALID }}
            // },
            // { 
            //     .vectorLength = 256,
            //     .operands = {{ .operand = XED_ENCODER_OPERAND_TYPE_REG, .regClass = XED_REG_CLASS_YMM },
            //     { .operand = XED_ENCODER_OPERAND_TYPE_REG, .regClass = XED_REG_CLASS_YMM },
            //     { .operand = XED_ENCODER_OPERAND_TYPE_REG, .regClass = XED_REG_CLASS_YMM }}
            // },
            // { 
            //     .vectorLength = 256,
            //     .operands = {{ .operand = XED_ENCODER_OPERAND_TYPE_REG, .regClass = XED_REG_CLASS_YMM },
            //     { .operand = XED_ENCODER_OPERAND_TYPE_REG, .regClass = XED_REG_CLASS_YMM },
            //     { .operand = XED_ENCODER_OPERAND_TYPE_IMM0, .regClass = XED_REG_CLASS_INVALID }}
            // }, // TODO tests
        }
    };
private:
    void implementation(bool upper, bool compile_inline) {
        if (!operands[2].isImmediate()) {
            debug_print("VPERMILPS: only immeddiate SRC2 (operand 2) argument is supported!\n");
            exit(1);
        }

        movups(operands[0].toEncoderOperand(upper), operands[1].toEncoderOperand(upper));
        shufps(
            operands[0].toEncoderOperand(upper),
            operands[1].toEncoderOperand(upper),
            operands[2].toEncoderOperand(upper));


        if (operands[0].isXmm()) {
            zeroupperInternal(operands[0]);
        }
    }
};
