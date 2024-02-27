#include "CompilableInstruction.h"
#include "xed/xed-encoder-hl.h"
#include "xed/xed-reg-class-enum.h"
#include <unistd.h>

class VPMOVMSKB : public CompilableInstruction<VPMOVMSKB> {
public:
    VPMOVMSKB(uint64_t rip, uint8_t ilen, xed_decoded_inst_t xedd) : CompilableInstruction(rip, ilen, xedd) {}

    static const inline InstructionMetadata Metadata = {
        .iclass = XED_ICLASS_VPMOVMSKB,
        .operandSets = {
            { 
                .vectorLength = 32,
                .operands = {{ .operand = XED_ENCODER_OPERAND_TYPE_REG, .regClass = XED_REG_CLASS_GPR32 },
                { .operand = XED_ENCODER_OPERAND_TYPE_REG, .regClass = XED_REG_CLASS_XMM }}
            },
            { 
                .vectorLength = 32,
                .operands = {{ .operand = XED_ENCODER_OPERAND_TYPE_REG, .regClass = XED_REG_CLASS_GPR32 },
                { .operand = XED_ENCODER_OPERAND_TYPE_REG, .regClass = XED_REG_CLASS_YMM }}
            },
        }
    };
private:
    void implementation(bool upper, bool compile_inline) override {
        if (!upper) {
            pmovmskb(operands[0].toEncoderOperand(upper), operands[1].toEncoderOperand(upper));
        } else {
            withFreeReg([&](xed_reg_enum_t tempReg) {
                auto temp32 = to32bitGpr(tempReg);
                pmovmskb(xed_reg(temp32), operands[1].toEncoderOperand(upper));
                pushf();
                shl(xed_reg(temp32), xed_imm0(16, 8));
                or_i(operands[0].toEncoderOperand(upper), xed_reg(temp32));
                popf();
            });
        }
    }
};
