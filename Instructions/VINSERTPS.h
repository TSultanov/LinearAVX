#include "CompilableInstruction.h"

class VINSERTPS : public CompilableInstruction<VINSERTPS> {
public:
    VINSERTPS(uint64_t rip, const xed_decoded_inst_t *xedd) : CompilableInstruction(rip, xedd) {}
private:
    void implementation(bool upper, bool compile_inline, ymm_t *ymm) {
        auto value = operands[3].imm8Value();

        if (operands[0].reg() == operands[1].reg() ) {
            insertps(
              operands[0].toEncoderOperand(upper),
              operands[2].toEncoderOperand(upper),
              operands[3].toEncoderOperand(upper));
        } else if (operands[0].reg() == operands[2].reg()) {
            // preseve REG1
            // do insertps from REG2 to REG1
            // do movups from REG1 to REG0
            // restore REG1
        } else {
            movups(
                operands[0].toEncoderOperand(upper),
                operands[1].toEncoderOperand(upper));
            insertps(
                operands[0].toEncoderOperand(upper),
                operands[2].toEncoderOperand(upper),
                operands[3].toEncoderOperand(upper));
        }
    }
};
