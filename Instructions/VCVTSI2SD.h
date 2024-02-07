#include "CompilableInstruction.h"

class VCVTSI2SD : public CompilableInstruction<VCVTSI2SD> {
public:
    VCVTSI2SD(uint64_t rip, uint8_t ilen, xed_decoded_inst_t xedd) : CompilableInstruction(rip, ilen, xedd) {}
private:
    void implementation(bool upper, bool compile_inline) {
        if (operands[0].reg() == operands[1].reg()) {
            cvtsi2sd(operands[0].toEncoderOperand(upper), operands[2].toEncoderOperand(upper));
        } else if (operands[0].reg() == operands[2].reg()) {
            withPreserveXmmReg(operands[1], [=]() {
                cvtsi2sd(operands[1].toEncoderOperand(upper), operands[2].toEncoderOperand(upper));
                movups(operands[0].toEncoderOperand(upper), operands[1].toEncoderOperand(upper));
            });
        } else {
            movups(operands[0].toEncoderOperand(upper), operands[1].toEncoderOperand(upper));
            cvtsi2sd(operands[0].toEncoderOperand(upper), operands[2].toEncoderOperand(upper));
        }

        zeroupperInternal(operands[0]);
    }
};