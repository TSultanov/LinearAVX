#include "CompilableInstruction.h"

class VANDPD : public CompilableInstruction<VANDPD> {
public:
    VANDPD(uint64_t rip, uint8_t ilen, xed_decoded_inst_t xedd) : CompilableInstruction(rip, ilen, xedd) {}
private:
    void implementation(bool upper, bool compile_inline) {
        if (operands[0].reg() == operands[1].reg()) {
            andpd(operands[0].toEncoderOperand(upper), operands[2].toEncoderOperand(upper));
        } else if (operands[0].reg() == operands[2].reg()) {
            withPreserveXmmReg(operands[1], [=]() {
                andpd(operands[1].toEncoderOperand(upper), operands[2].toEncoderOperand(upper));
                movupd(operands[0].toEncoderOperand(upper), operands[1].toEncoderOperand(upper));
            });
        } else {
            movupd(operands[0].toEncoderOperand(upper), operands[1].toEncoderOperand(upper));
            andpd(operands[0].toEncoderOperand(upper), operands[2].toEncoderOperand(upper));
        }

        if (operands[0].isXmm()) {
            zeroupperInternal(operands[0]);
        }
    }
};