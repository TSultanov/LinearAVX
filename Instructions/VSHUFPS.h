#include "CompilableInstruction.h"

class VSHUFPS : public CompilableInstruction<VSHUFPS> {
public:
    VSHUFPS(uint64_t rip, uint8_t ilen, xed_decoded_inst_t xedd) : CompilableInstruction(rip, ilen, xedd) {}
private:
    void implementation(bool upper, bool compile_inline) {
        if (operands[0].reg() == operands[1].reg()) {
            shufps(
                operands[0].toEncoderOperand(upper),
                operands[1].toEncoderOperand(upper),
                operands[3].toEncoderOperand(upper));
        } else {
            withPreserveXmmReg(operands[1], [=]() {
                shufps(
                    operands[1].toEncoderOperand(upper),
                    operands[2].toEncoderOperand(upper),
                    operands[3].toEncoderOperand(upper));
                movups(operands[0].toEncoderOperand(upper), operands[1].toEncoderOperand(upper));
            });
        }

        if (operands[0].isXmm()) {
            zeroupperInternal(operands[0]);
        }
    }
};
