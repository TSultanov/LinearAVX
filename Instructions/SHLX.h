#include "CompilableInstruction.h"
#include "xed/xed-reg-enum.h"

class SHLX : public CompilableInstruction<SHLX> {
public:
    SHLX(uint64_t rip, uint8_t ilen, xed_decoded_inst_t xedd) : CompilableInstruction(rip, ilen, xedd) {
        usedRegs.insert(XED_REG_RCX);
    }

    static const inline InstructionMetadata Metadata = {
        .iclass = XED_ICLASS_SHLX,
        .operandSets = {
            { 
                .vectorLength = 32,
                .operands = {{ .operand = XED_ENCODER_OPERAND_TYPE_REG, .regClass = XED_REG_CLASS_GPR32 },
                { .operand = XED_ENCODER_OPERAND_TYPE_REG, .regClass = XED_REG_CLASS_GPR32 },
                { .operand = XED_ENCODER_OPERAND_TYPE_REG, .regClass = XED_REG_CLASS_GPR32 }}
            },
            // { 
            //     .vectorLength = 32,
            //     .operands = {{ .operand = XED_ENCODER_OPERAND_TYPE_REG, .regClass = XED_REG_CLASS_GPR32 },
            //     { .operand = XED_ENCODER_OPERAND_TYPE_MEM, .regClass = XED_REG_CLASS_INVALID },
            //     { .operand = XED_ENCODER_OPERAND_TYPE_REG, .regClass = XED_REG_CLASS_GPR32 }}
            // },
            { 
                .vectorLength = 64,
                .operands = {{ .operand = XED_ENCODER_OPERAND_TYPE_REG, .regClass = XED_REG_CLASS_GPR64 },
                { .operand = XED_ENCODER_OPERAND_TYPE_REG, .regClass = XED_REG_CLASS_GPR64 },
                { .operand = XED_ENCODER_OPERAND_TYPE_REG, .regClass = XED_REG_CLASS_GPR64 }}
            },
            // { 
            //     .vectorLength = 64,
            //     .operands = {{ .operand = XED_ENCODER_OPERAND_TYPE_REG, .regClass = XED_REG_CLASS_GPR64 },
            //     { .operand = XED_ENCODER_OPERAND_TYPE_MEM, .regClass = XED_REG_CLASS_INVALID },
            //     { .operand = XED_ENCODER_OPERAND_TYPE_REG, .regClass = XED_REG_CLASS_GPR64 }}
            // },
        }
    };
private:
    void implementation(bool upper, bool compile_inline) {
        withFreeReg([=](xed_reg_enum_t tempReg) {
            if (operands[1].isMemoryOperand()) {
                mov(xed_reg(tempReg), operands[1].toEncoderOperand(false));
            }
            if (operands[1].is32BitRegister()) {
                mov(xed_reg(tempReg), xed_reg(operands[1].to64BitRegister()));
            }

            push(XED_REG_RCX); // TODO: What if any of the registers in the original isntruction is RCX?

            mov(xed_reg(XED_REG_RCX), xed_reg(operands[2].to64BitRegister()));

            if (opWidth == 64) {
                shl(xed_reg(tempReg), xed_reg(XED_REG_CL));
            } else {
                shl(xed_reg((xed_reg_enum_t)(tempReg - XED_REG_RAX + XED_REG_EAX)), xed_reg(XED_REG_CL));
            }

            mov(xed_reg(operands[0].to64BitRegister()), xed_reg(tempReg));

            pop(XED_REG_RCX);
        });
    }
};