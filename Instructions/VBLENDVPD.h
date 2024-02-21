#include "CompilableInstruction.h"
#include "Operand.h"
#include "xed/xed-encoder-hl.h"
#include "xed/xed-reg-enum.h"
#include <vector>

class VBLENDVPD : public CompilableInstruction<VBLENDVPD> {
public:
    VBLENDVPD(uint64_t rip, uint8_t ilen, xed_decoded_inst_t xedd) : CompilableInstruction(rip, ilen, xedd) {}

    static const inline InstructionMetadata Metadata = {
        .iclass = XED_ICLASS_VBLENDVPD,
        .operandSets = {
            { 
                .vectorLength = 128,
                .operands = {{ .operand = XED_ENCODER_OPERAND_TYPE_REG, .regClass = XED_REG_CLASS_XMM },
                { .operand = XED_ENCODER_OPERAND_TYPE_REG, .regClass = XED_REG_CLASS_XMM },
                { .operand = XED_ENCODER_OPERAND_TYPE_MEM, .regClass = XED_REG_CLASS_INVALID },
                { .operand = XED_ENCODER_OPERAND_TYPE_REG, .regClass = XED_REG_CLASS_XMM },
                }
            },
            { 
                .vectorLength = 128,
                .operands = {{ .operand = XED_ENCODER_OPERAND_TYPE_REG, .regClass = XED_REG_CLASS_XMM },
                { .operand = XED_ENCODER_OPERAND_TYPE_REG, .regClass = XED_REG_CLASS_XMM },
                { .operand = XED_ENCODER_OPERAND_TYPE_REG, .regClass = XED_REG_CLASS_XMM },
                { .operand = XED_ENCODER_OPERAND_TYPE_REG, .regClass = XED_REG_CLASS_XMM },
                }
            },
            { 
                .vectorLength = 256,
                .operands = {{ .operand = XED_ENCODER_OPERAND_TYPE_REG, .regClass = XED_REG_CLASS_YMM },
                { .operand = XED_ENCODER_OPERAND_TYPE_REG, .regClass = XED_REG_CLASS_YMM },
                { .operand = XED_ENCODER_OPERAND_TYPE_MEM, .regClass = XED_REG_CLASS_INVALID },
                { .operand = XED_ENCODER_OPERAND_TYPE_REG, .regClass = XED_REG_CLASS_YMM },
                }
            },
            { 
                .vectorLength = 256,
                .operands = {{ .operand = XED_ENCODER_OPERAND_TYPE_REG, .regClass = XED_REG_CLASS_YMM },
                { .operand = XED_ENCODER_OPERAND_TYPE_REG, .regClass = XED_REG_CLASS_YMM },
                { .operand = XED_ENCODER_OPERAND_TYPE_REG, .regClass = XED_REG_CLASS_YMM },
                { .operand = XED_ENCODER_OPERAND_TYPE_REG, .regClass = XED_REG_CLASS_YMM },
                }
            },
        }
    };
private:
    void implementation(bool upper, bool compile_inline) {
        bool usesXmm0 = false;
        int xmm0operand = 0;

        int i = 0;
        for (auto& op : operands) {
            if (op.reg() == XED_REG_XMM0 || op.reg() == XED_REG_YMM0) {
                usesXmm0 = true;
                xmm0operand = i;
                i++;
            }
        }

        if (usesXmm0) {
            auto tempXmmReg = getUnusedXmmReg();
            withPreserveXmmReg(tempXmmReg, [=]() {
                movups(tempXmmReg, operands[xmm0operand].toEncoderOperand(upper));

                std::vector<xed_encoder_operand_t> tempOperands;
                for (int i = 0; i < operands.size(); i++) {
                    if (i == xmm0operand) {
                        tempOperands.push_back(xed_reg(tempXmmReg));
                    } else {
                        tempOperands.push_back(operands[i].toEncoderOperand(upper));
                    }
                }

                if (operands[0].reg() != operands[1].reg()) {
                    movups(
                        tempOperands[0],
                        tempOperands[1]);
                }

                withPreserveXmmReg(XED_REG_XMM0, [=]() {
                    movups(xed_reg(XED_REG_XMM0), operands[3].toEncoderOperand(upper));
                    blendvpd(
                        tempOperands[0],
                        tempOperands[2]);
                });

                if(xmm0operand == 0) {
                    movups(operands[0].toEncoderOperand(upper), tempOperands[0]);
                }
            });

            returnReg(tempXmmReg);
        } else {
            withPreserveXmmReg(XED_REG_XMM0, [=]() {
                movups(xed_reg(XED_REG_XMM0), operands[3].toEncoderOperand(upper));
                blendvpd(
                    operands[0].toEncoderOperand(upper),
                    operands[2].toEncoderOperand(upper));

            });
        }

        if (operands[0].isXmm()) {
            zeroupperInternal(operands[0]);
        }
    }
};
