#include "CompilableInstruction.h"

class VANDPS : public CompilableInstruction<VANDPS> {
public:
    VANDPS(uint64_t rip, uint8_t ilen, xed_decoded_inst_t xedd) : CompilableInstruction(rip, ilen, xedd) {}
private:
    void implementation(bool upper, bool compile_inline) {
        if (operands[0].reg() == operands[1].reg()) {
            andps(operands[0].toEncoderOperand(upper), operands[2].toEncoderOperand(upper));
        } else if (operands[0].reg() == operands[2].reg()) {
            withPreserveXmmReg(operands[1], [=]() {
                andps(operands[1].toEncoderOperand(upper), operands[2].toEncoderOperand(upper));
                movupd(operands[0].toEncoderOperand(upper), operands[1].toEncoderOperand(upper));
            });
        } else {
            movupd(operands[0].toEncoderOperand(upper), operands[1].toEncoderOperand(upper));
            andps(operands[0].toEncoderOperand(upper), operands[2].toEncoderOperand(upper));
        }

        if (operands[0].isXmm()) {
            zeroupperInternal(operands[0]);
        }
    }
};