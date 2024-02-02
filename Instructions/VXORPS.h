#include "CompilableInstruction.h"

class VXORPS : public CompilableInstruction<VXORPS> {
public:
    VXORPS(uint64_t rip, const xed_decoded_inst_t *xedd) : CompilableInstruction(rip, xedd) {}
private:
    void implementation(bool upper, bool compile_inline, ymm_t *ymm) {
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
