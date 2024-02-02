#include "CompilableInstruction.h"

class VINSERTPS : public CompilableInstruction<VINSERTPS> {
public:
    VINSERTPS(uint64_t rip, const xed_decoded_inst_t *xedd) : CompilableInstruction(rip, xedd) {}
private:
    void implementation(bool upper, bool compile_inline, ymm_t *ymm) {
        auto value = operands[3].imm8Value();

        if (operands[0].reg() != operands[1].reg()) {
            movups(
                operands[0].toEncoderOperand(upper),
                operands[1].toEncoderOperand(upper));
        }
        insertps(
              operands[0].toEncoderOperand(upper),
              operands[2].toEncoderOperand(upper),
              operands[3].toEncoderOperand(upper));
    }
};
