#include "CompilableInstruction.h"

class VADDPS : public CompilableInstruction<VADDPS> {
public:
    VADDPS(uint64_t rip, uint8_t ilen, const xed_decoded_inst_t *xedd) : CompilableInstruction(rip, ilen, xedd) {}
private:
    void implementation(bool upper, bool compile_inline, ymm_t *ymm) {
        if (operands[0].reg() == operands[1].reg()) {
            addps(operands[0].toEncoderOperand(upper), operands[2].toEncoderOperand(upper));
        } else {
            movups(operands[0].toEncoderOperand(upper), operands[1].toEncoderOperand(upper));
            addps(operands[0].toEncoderOperand(upper), operands[2].toEncoderOperand(upper));
        }
    }
};