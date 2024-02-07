#include "CompilableInstruction.h"

class VADDPD : public CompilableInstruction<VADDPD> {
public:
    VADDPD(uint64_t rip, uint8_t ilen, xed_decoded_inst_t xedd) : CompilableInstruction(rip, ilen, xedd) {}
private:
    void implementation(bool upper, bool compile_inline) {
        if (operands[0].reg() == operands[1].reg()) {
            addpd(operands[0].toEncoderOperand(upper), operands[2].toEncoderOperand(upper));
        } else if (operands[0].reg() == operands[2].reg()) {
            withPreserveXmmReg(operands[1], [=]() {
                addpd(operands[1].toEncoderOperand(upper), operands[2].toEncoderOperand(upper));
                movupd(operands[0].toEncoderOperand(upper), operands[1].toEncoderOperand(upper));
            });
        } else {
            movupd(operands[0].toEncoderOperand(upper), operands[1].toEncoderOperand(upper));
            addpd(operands[0].toEncoderOperand(upper), operands[2].toEncoderOperand(upper));
        }
    }
};