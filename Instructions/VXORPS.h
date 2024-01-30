#include "CompilableInstruction.h"

class VXORPS : public CompilableInstruction<VXORPS> {
public:
    VXORPS(const xed_decoded_inst_t *xedd) : CompilableInstruction(xedd) {}
private:
    void implementation(bool upper, bool compile_inline) {
        if (operands[0].reg() != operands[1].reg()) {
            movups(
                operands[0].toEncoderOperand(upper),
                operands[1].toEncoderOperand(upper));
        }
        xorps(
              operands[0].toEncoderOperand(upper),
              operands[2].toEncoderOperand(upper));
    }
};