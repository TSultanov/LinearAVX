#include "CompilableInstruction.h"
#include "xed/xed-encoder-hl.h"
#include "xed/xed-reg-class-enum.h"

class VSHUFPS : public CompilableInstruction<VSHUFPS> {
public:
    VSHUFPS(uint64_t rip, uint8_t ilen, xed_decoded_inst_t xedd) : CompilableInstruction(rip, ilen, xedd) {}

    static const inline InstructionMetadata Metadata = {
        .iclass = XED_ICLASS_VSHUFPS,
        .operandSets = {
            { 
                .vectorLength = 128,
                .operands = {{ .operand = XED_ENCODER_OPERAND_TYPE_REG, .regClass = XED_REG_CLASS_XMM },
                { .operand = XED_ENCODER_OPERAND_TYPE_REG, .regClass = XED_REG_CLASS_XMM },
                { .operand = XED_ENCODER_OPERAND_TYPE_MEM, .regClass = XED_REG_CLASS_INVALID },
                { .operand = XED_ENCODER_OPERAND_TYPE_IMM0, .regClass = XED_REG_CLASS_INVALID, .immBits = 8}}
            },
            { 
                .vectorLength = 128,
                .operands = {{ .operand = XED_ENCODER_OPERAND_TYPE_REG, .regClass = XED_REG_CLASS_XMM },
                { .operand = XED_ENCODER_OPERAND_TYPE_REG, .regClass = XED_REG_CLASS_XMM },
                { .operand = XED_ENCODER_OPERAND_TYPE_REG, .regClass = XED_REG_CLASS_XMM },
                { .operand = XED_ENCODER_OPERAND_TYPE_IMM0, .regClass = XED_REG_CLASS_INVALID, .immBits = 8 }}
            },
            { 
                .vectorLength = 256,
                .operands = {{ .operand = XED_ENCODER_OPERAND_TYPE_REG, .regClass = XED_REG_CLASS_YMM },
                { .operand = XED_ENCODER_OPERAND_TYPE_REG, .regClass = XED_REG_CLASS_YMM },
                { .operand = XED_ENCODER_OPERAND_TYPE_MEM, .regClass = XED_REG_CLASS_INVALID },
                { .operand = XED_ENCODER_OPERAND_TYPE_IMM0, .regClass = XED_REG_CLASS_INVALID, .immBits = 8 }}
            },
            { 
                .vectorLength = 256,
                .operands = {{ .operand = XED_ENCODER_OPERAND_TYPE_REG, .regClass = XED_REG_CLASS_YMM },
                { .operand = XED_ENCODER_OPERAND_TYPE_REG, .regClass = XED_REG_CLASS_YMM },
                { .operand = XED_ENCODER_OPERAND_TYPE_REG, .regClass = XED_REG_CLASS_YMM },
                { .operand = XED_ENCODER_OPERAND_TYPE_IMM0, .regClass = XED_REG_CLASS_INVALID, .immBits = 8 }}
            },
        }
    };
private:
    void implementation(bool upper, bool compile_inline) {
        map3opto2op(upper, [&](xed_encoder_operand_t const& op0, xed_encoder_operand_t const& op1) {
            shufps(op0, op1, operands[3].toEncoderOperand(upper));
        });

        if (operands[0].isXmm()) {
            zeroupperInternal(operands[0]);
        }
    }
};
