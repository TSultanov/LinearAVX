#include "CompilableInstruction.h"

class VADDPS : public CompilableInstruction<VADDPS> {
public:
    VADDPS(const xed_decoded_inst_t *xedd) : CompilableInstruction(xedd) {}
private:
    void implementation(bool upper, bool compile_inline) {
        if (operands[0].reg() == operands[1].reg()) {
            addps(operands[0].toEncoderOperand(upper), operands[2].toEncoderOperand(upper));
        } else {
            movups(operands[0].toEncoderOperand(upper), operands[1].toEncoderOperand(upper));
            addps(operands[0].toEncoderOperand(upper), operands[2].toEncoderOperand(upper));
        }
    }
};