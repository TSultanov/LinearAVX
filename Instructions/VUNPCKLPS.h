#include "CompilableInstruction.h"

class VUNPCKLPS : public CompilableInstruction<VUNPCKLPS> {
public:
    VUNPCKLPS(uint64_t rip, uint8_t ilen, xed_decoded_inst_t xedd) : CompilableInstruction(rip, ilen, xedd) {}
private:
    void implementation(bool upper, bool compile_inline) {
        if (operands[0].reg() == operands[1].reg()) {
            unpcklps(
                operands[0].toEncoderOperand(upper),
                operands[2].toEncoderOperand(upper));
        } else if (operands[0].reg() == operands[1].reg()) {
            withPreserveXmmReg(operands[1], [=]() {
                unpcklps(operands[1].toEncoderOperand(upper), operands[2].toEncoderOperand(upper));
                movups(operands[0].toEncoderOperand(upper), operands[1].toEncoderOperand(upper));
            });
        } else {
            movups(operands[0].toEncoderOperand(upper), operands[1].toEncoderOperand(upper));
            unpcklps(
                operands[0].toEncoderOperand(upper),
                operands[2].toEncoderOperand(upper));
        }

        if (operands[0].isXmm()) {
            zeroupperInternal(operands[0]);
        }
    }
};
