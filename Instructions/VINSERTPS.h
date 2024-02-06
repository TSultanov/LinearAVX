#include "CompilableInstruction.h"
#include "xed/xed-encoder-hl.h"

class VINSERTPS : public CompilableInstruction<VINSERTPS> {
public:
    VINSERTPS(uint64_t rip, uint8_t ilen, xed_decoded_inst_t xedd) : CompilableInstruction(rip, ilen, xedd) {}
private:
    void implementation(bool upper, bool compile_inline, ymm_t *ymm) {
        auto value = operands[3].imm8Value();

        if (operands[0].reg() == operands[1].reg() ) {
            insertps(
              operands[0].toEncoderOperand(upper),
              operands[2].toEncoderOperand(upper),
              operands[3].toEncoderOperand(upper));
        } else {
            withPreserveXmmReg(operands[1], [=]() {
                insertps(operands[1].toEncoderOperand(upper), operands[2].toEncoderOperand(upper), operands[3].toEncoderOperand(upper));
                movups(operands[0].toEncoderOperand(upper), operands[1].toEncoderOperand(upper));
            });
        }
    }
};
