#include "CompilableInstruction.h"
#include "xed/xed-reg-class-enum.h"

class VINSERTF128 : public CompilableInstruction<VINSERTF128> {
public:
    VINSERTF128(uint64_t rip, uint8_t ilen, xed_decoded_inst_t xedd) : CompilableInstruction(rip, ilen, xedd) {}

    static const inline InstructionMetadata Metadata = {
        .iclass = XED_ICLASS_VINSERTF128,
        .operandSets = {
            { 
                .vectorLength = 128,
                .operands = {{ .operand = XED_ENCODER_OPERAND_TYPE_REG, .regClass = XED_REG_CLASS_YMM },
                { .operand = XED_ENCODER_OPERAND_TYPE_REG, .regClass = XED_REG_CLASS_YMM },
                { .operand = XED_ENCODER_OPERAND_TYPE_REG, .regClass = XED_REG_CLASS_XMM },
                { .operand = XED_ENCODER_OPERAND_TYPE_IMM0, .regClass = XED_REG_CLASS_INVALID, .immBits = 8, .setImmValue = true, .immValue = 0 }}
            },
            { 
                .vectorLength = 128,
                .operands = {{ .operand = XED_ENCODER_OPERAND_TYPE_REG, .regClass = XED_REG_CLASS_YMM },
                { .operand = XED_ENCODER_OPERAND_TYPE_REG, .regClass = XED_REG_CLASS_YMM },
                { .operand = XED_ENCODER_OPERAND_TYPE_REG, .regClass = XED_REG_CLASS_XMM },
                { .operand = XED_ENCODER_OPERAND_TYPE_IMM0, .regClass = XED_REG_CLASS_INVALID, .immBits = 8, .setImmValue = true, .immValue = 1 }}
            },
            { 
                .vectorLength = 128,
                .operands = {{ .operand = XED_ENCODER_OPERAND_TYPE_REG, .regClass = XED_REG_CLASS_YMM },
                { .operand = XED_ENCODER_OPERAND_TYPE_REG, .regClass = XED_REG_CLASS_YMM },
                { .operand = XED_ENCODER_OPERAND_TYPE_REG, .regClass = XED_REG_CLASS_XMM },
                { .operand = XED_ENCODER_OPERAND_TYPE_IMM0, .regClass = XED_REG_CLASS_INVALID, .immBits = 8, .setImmValue = true, .immValue = 0 }}
            },
            { 
                .vectorLength = 128,
                .operands = {{ .operand = XED_ENCODER_OPERAND_TYPE_REG, .regClass = XED_REG_CLASS_YMM },
                { .operand = XED_ENCODER_OPERAND_TYPE_REG, .regClass = XED_REG_CLASS_YMM },
                { .operand = XED_ENCODER_OPERAND_TYPE_REG, .regClass = XED_REG_CLASS_XMM },
                { .operand = XED_ENCODER_OPERAND_TYPE_IMM0, .regClass = XED_REG_CLASS_INVALID, .immBits = 8, .setImmValue = true, .immValue = 1 }}
            },
            { 
                .vectorLength = 128,
                .operands = {{ .operand = XED_ENCODER_OPERAND_TYPE_REG, .regClass = XED_REG_CLASS_YMM },
                { .operand = XED_ENCODER_OPERAND_TYPE_REG, .regClass = XED_REG_CLASS_YMM },
                { .operand = XED_ENCODER_OPERAND_TYPE_MEM, .regClass = XED_REG_CLASS_INVALID },
                { .operand = XED_ENCODER_OPERAND_TYPE_IMM0, .regClass = XED_REG_CLASS_INVALID, .immBits = 8, .setImmValue = true, .immValue = 0 }}
            },
            { 
                .vectorLength = 128,
                .operands = {{ .operand = XED_ENCODER_OPERAND_TYPE_REG, .regClass = XED_REG_CLASS_YMM },
                { .operand = XED_ENCODER_OPERAND_TYPE_REG, .regClass = XED_REG_CLASS_YMM },
                { .operand = XED_ENCODER_OPERAND_TYPE_MEM, .regClass = XED_REG_CLASS_INVALID },
                { .operand = XED_ENCODER_OPERAND_TYPE_IMM0, .regClass = XED_REG_CLASS_INVALID, .immBits = 8, .setImmValue = true, .immValue = 1 }}
            },
            { 
                .vectorLength = 128,
                .operands = {{ .operand = XED_ENCODER_OPERAND_TYPE_REG, .regClass = XED_REG_CLASS_YMM },
                { .operand = XED_ENCODER_OPERAND_TYPE_REG, .regClass = XED_REG_CLASS_YMM },
                { .operand = XED_ENCODER_OPERAND_TYPE_MEM, .regClass = XED_REG_CLASS_INVALID },
                { .operand = XED_ENCODER_OPERAND_TYPE_IMM0, .regClass = XED_REG_CLASS_INVALID, .immBits = 8, .setImmValue = true, .immValue = 0 }}
            },
            { 
                .vectorLength = 128,
                .operands = {{ .operand = XED_ENCODER_OPERAND_TYPE_REG, .regClass = XED_REG_CLASS_YMM },
                { .operand = XED_ENCODER_OPERAND_TYPE_REG, .regClass = XED_REG_CLASS_YMM },
                { .operand = XED_ENCODER_OPERAND_TYPE_MEM, .regClass = XED_REG_CLASS_INVALID },
                { .operand = XED_ENCODER_OPERAND_TYPE_IMM0, .regClass = XED_REG_CLASS_INVALID, .immBits = 8, .setImmValue = true, .immValue = 1 }}
            },
        }
    };
private:
    void implementation(bool upper, bool compile_inline) {
        auto value = operands[3].immValue();
        if(value) {
            if (upper) {
                movups(operands[0].toEncoderOperand(upper), operands[2].toEncoderOperand(false));
            } else {
                movups(operands[0].toEncoderOperand(upper), operands[1].toEncoderOperand(upper));
            }
        } else {
            if (!upper) {
                movups(operands[0].toEncoderOperand(upper), operands[2].toEncoderOperand(false));
            } else {
                movups(operands[0].toEncoderOperand(upper), operands[1].toEncoderOperand(upper));
            }
        }
    }
};
