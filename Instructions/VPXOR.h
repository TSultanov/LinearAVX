#include "CompilableInstruction.h"

class VPXOR : public CompilableInstruction<VPXOR> {
public:
    VPXOR(uint64_t rip, uint8_t ilen, xed_decoded_inst_t xedd) : CompilableInstruction(rip, ilen, xedd) {}
private:
    void implementation(bool upper, bool compile_inline) {
        if (operands[0].reg() == operands[1].reg()) {
             pxor(
              operands[0].toEncoderOperand(upper),
              operands[2].toEncoderOperand(upper));
        } else if (operands[0].reg() == operands[2].reg()) {
            withPreserveXmmReg(operands[1], [=]() {
                pxor(operands[1].toEncoderOperand(upper), operands[2].toEncoderOperand(upper));
                movups(operands[0].toEncoderOperand(upper), operands[1].toEncoderOperand(upper));
            });
        } else {
            movups(operands[0].toEncoderOperand(upper), operands[1].toEncoderOperand(upper));
            pxor(
              operands[0].toEncoderOperand(upper),
              operands[2].toEncoderOperand(upper));
        }       
    }
};
