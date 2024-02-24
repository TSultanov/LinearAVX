#include "CompilableInstruction.h"

class BLSR : public CompilableInstruction<BLSR> {
public:
    BLSR(uint64_t rip, uint8_t ilen, xed_decoded_inst_t xedd) : CompilableInstruction(rip, ilen, xedd) {
    }

    static const inline InstructionMetadata Metadata = {
        .iclass = XED_ICLASS_BLSR,
        .operandSets = {
            { 
                .vectorLength = 32,
                .operands = {
                    { .operand = XED_ENCODER_OPERAND_TYPE_REG, .regClass = XED_REG_CLASS_GPR32 },
                    { .operand = XED_ENCODER_OPERAND_TYPE_REG, .regClass = XED_REG_CLASS_GPR32 },
                }
            },
            { 
                .vectorLength = 32,
                .operands = {
                    { .operand = XED_ENCODER_OPERAND_TYPE_REG, .regClass = XED_REG_CLASS_GPR32 },
                    { .operand = XED_ENCODER_OPERAND_TYPE_MEM },
                }
            },
            { 
                .vectorLength = 64,
                .operands = {
                    { .operand = XED_ENCODER_OPERAND_TYPE_REG, .regClass = XED_REG_CLASS_GPR64 },
                    { .operand = XED_ENCODER_OPERAND_TYPE_REG, .regClass = XED_REG_CLASS_GPR64 },
                }
            },
            { 
                .vectorLength = 64,
                .operands = {
                    { .operand = XED_ENCODER_OPERAND_TYPE_REG, .regClass = XED_REG_CLASS_GPR64 },
                    { .operand = XED_ENCODER_OPERAND_TYPE_MEM },
                }
            },
        }
    };
private:

    void implementation(bool upper, bool compile_inline) {
        mov(operands[0].toEncoderOperand(upper), operands[1].toEncoderOperand(upper));
        sub(operands[0].reg(), 1);
        and_i(operands[0].toEncoderOperand(upper), operands[1].toEncoderOperand(upper));
        // TODO: set carry flag
    }
};