#include "CompilableInstruction.h"
#include "xed/xed-reg-enum.h"

class ANDN : public CompilableInstruction<ANDN> {
public:
    ANDN(uint64_t rip, uint8_t ilen, xed_decoded_inst_t xedd) : CompilableInstruction(rip, ilen, xedd) {}

    static const inline InstructionMetadata Metadata = {
        .iclass = XED_ICLASS_ANDN,
        .operandSets = {
            { 
                .vectorLength = 32,
                .operands = {
                    { .operand = XED_ENCODER_OPERAND_TYPE_REG, .regClass = XED_REG_CLASS_GPR32 },
                    { .operand = XED_ENCODER_OPERAND_TYPE_REG, .regClass = XED_REG_CLASS_GPR32 },
                    { .operand = XED_ENCODER_OPERAND_TYPE_REG, .regClass = XED_REG_CLASS_GPR32 },
                }
            },
            { 
                .vectorLength = 32,
                .operands = {
                    { .operand = XED_ENCODER_OPERAND_TYPE_REG, .regClass = XED_REG_CLASS_GPR32 },
                    { .operand = XED_ENCODER_OPERAND_TYPE_REG, .regClass = XED_REG_CLASS_GPR32 },
                    { .operand = XED_ENCODER_OPERAND_TYPE_MEM },
                }
            },
            { 
                .vectorLength = 64,
                .operands = {
                    { .operand = XED_ENCODER_OPERAND_TYPE_REG, .regClass = XED_REG_CLASS_GPR64 },
                    { .operand = XED_ENCODER_OPERAND_TYPE_REG, .regClass = XED_REG_CLASS_GPR64 },
                    { .operand = XED_ENCODER_OPERAND_TYPE_REG, .regClass = XED_REG_CLASS_GPR64 }
                }
            },
            { 
                .vectorLength = 64,
                .operands = {
                    { .operand = XED_ENCODER_OPERAND_TYPE_REG, .regClass = XED_REG_CLASS_GPR64 },
                    { .operand = XED_ENCODER_OPERAND_TYPE_REG, .regClass = XED_REG_CLASS_GPR64 },
                    { .operand = XED_ENCODER_OPERAND_TYPE_MEM },
                }
            },
        }
    };
private:
    void implementation(bool upper, bool compile_inline) {
        withFreeReg([&](xed_reg_enum_t tempReg) {
            mov(xed_reg(tempReg), xed_reg(operands[1].to64BitRegister()));

            if (opWidth == 64) {
                not_i(xed_reg(tempReg));
                and_i(xed_reg(tempReg), operands[2].toEncoderOperand(upper));
            } else {
                auto src1 = xed_reg((xed_reg_enum_t)(tempReg - XED_REG_RAX + XED_REG_EAX));
                not_i(src1);
                and_i(src1, operands[2].toEncoderOperand(upper));
            }

            mov(xed_reg(operands[0].to64BitRegister()), xed_reg(tempReg));
        });
    }
};