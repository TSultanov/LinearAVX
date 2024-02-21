#include "CompilableInstruction.h"
#include "xed/xed-encoder-hl.h"
#include "xed/xed-reg-enum.h"
#include <optional>

class SHRX : public CompilableInstruction<SHRX> {
public:
    SHRX(uint64_t rip, uint8_t ilen, xed_decoded_inst_t xedd) : CompilableInstruction(rip, ilen, xedd) {
        usedRegs.insert(XED_REG_RCX);
    }

    static const inline InstructionMetadata Metadata = {
        .iclass = XED_ICLASS_SHRX,
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
            { 
                .vectorLength = 64,
                .operands = {{ .operand = XED_ENCODER_OPERAND_TYPE_REG, .regClass = XED_REG_CLASS_GPR64 },
                { .operand = XED_ENCODER_OPERAND_TYPE_MEM, .regClass = XED_REG_CLASS_INVALID },
                { .operand = XED_ENCODER_OPERAND_TYPE_REG, .regClass = XED_REG_CLASS_GPR64 }}
            },
        }
    };
private:

    xed_encoder_operand_t substRcx(xed_encoder_operand_t op, std::optional<xed_reg_enum_t>  subst) {
        if (!subst.has_value()) {
            return op;
        }

        if (op.type == XED_ENCODER_OPERAND_TYPE_REG && op.u.reg == XED_REG_RCX) {
            op.u.reg = *subst;
        }

        if (op.type == XED_ENCODER_OPERAND_TYPE_MEM) {
            if (op.u.mem.base == XED_REG_RCX) {
                op.u.mem.base = *subst;
            }
            if (op.u.mem.base == XED_REG_ECX) {
                op.u.mem.base = (xed_reg_enum_t)(*subst - XED_REG_RAX + XED_REG_EAX);
            }
            if (op.u.mem.index == XED_REG_RCX) {
                op.u.mem.index = *subst;
            }
            if (op.u.mem.index == XED_REG_ECX) {
                op.u.mem.index = (xed_reg_enum_t)(*subst - XED_REG_RAX + XED_REG_EAX);
            }
        }
        return op;
    }

    void implementation(bool upper, bool compile_inline) {
        std::optional<xed_reg_enum_t> rcxSubst;
        std::optional<xed_reg_enum_t> original;
        for (auto & op : operands) {
            if (op.reg() == XED_REG_RCX || op.reg() == XED_REG_ECX) {
                original = op.to64BitRegister();
            }
            if (op.isMemoryOperand()) {
                auto o = op.toEncoderOperand(false);
                if (o.u.mem.base == XED_REG_RCX || o.u.mem.base == XED_REG_ECX) {
                    original = o.u.mem.base;
                    if (*original >= XED_REG_EAX && *original <= XED_REG_EDI) {
                        *original = (xed_reg_enum_t)(*original - XED_REG_EAX + XED_REG_RAX);
                    }
                }
                if (o.u.mem.scale == XED_REG_RCX || o.u.mem.base == XED_REG_ECX) {
                    original = o.u.mem.index;
                    if (*original >= XED_REG_EAX && *original <= XED_REG_EDI) {
                        *original = (xed_reg_enum_t)(*original - XED_REG_EAX + XED_REG_RAX);
                    }
                }
            }
            if (original.has_value()) {
                rcxSubst = getUnusedReg();
                push(*rcxSubst);
                mov(xed_reg(*rcxSubst), xed_reg(*original));
                break;
            }
        }

        withFreeReg([&](xed_reg_enum_t tempReg) {
            if (operands[1].isMemoryOperand()) {
                mov(xed_reg(tempReg), substRcx(operands[1].toEncoderOperand(false), rcxSubst));
            } else {
                mov(xed_reg(tempReg), substRcx(xed_reg(operands[1].to64BitRegister()), rcxSubst));
            }

            push(XED_REG_RCX); // TODO: What if any of the registers in the original isntruction is RCX?

            mov(xed_reg(XED_REG_RCX), substRcx(xed_reg(operands[2].to64BitRegister()), rcxSubst));

            if (opWidth == 64) {
                shr(xed_reg(tempReg), xed_reg(XED_REG_CL));
            } else {
                shr(xed_reg((xed_reg_enum_t)(tempReg - XED_REG_RAX + XED_REG_EAX)), xed_reg(XED_REG_CL));
            }

            mov(substRcx(xed_reg(operands[0].to64BitRegister()), rcxSubst), xed_reg(tempReg));

            pop(XED_REG_RCX);
        });

        if (rcxSubst.has_value()) {
            mov(xed_reg(*original), xed_reg(*rcxSubst));
            pop(*rcxSubst);
            returnReg(*rcxSubst);
        }
    }
};