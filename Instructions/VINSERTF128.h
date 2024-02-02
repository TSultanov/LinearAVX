#include "CompilableInstruction.h"

class VINSERTF128 : public CompilableInstruction<VINSERTF128> {
public:
    VINSERTF128(uint64_t rip, uint64_t rsp, const xed_decoded_inst_t *xedd) : CompilableInstruction(rip, rsp, xedd) {}
private:
    void implementation(bool upper, bool compile_inline, ymm_t *ymm) {
        movups(operands[0].toEncoderOperand(upper), operands[1].toEncoderOperand(upper));

        auto value = operands[3].imm8Value();
        if(value) {
            if (upper) {
                movups(operands[0].toEncoderOperand(upper), operands[2].toEncoderOperand(upper));
            }
        } else {
            if (!upper) {
                movups(operands[0].toEncoderOperand(upper), operands[2].toEncoderOperand(upper));
            }
        }
    }
};
