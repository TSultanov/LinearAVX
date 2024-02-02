#include "CompilableInstruction.h"

class VMOVAPS : public CompilableInstruction<VMOVAPS> {
public:
    VMOVAPS(uint64_t rip, const xed_decoded_inst_t *xedd) : CompilableInstruction(rip, xedd) {}
private:
    void implementation(bool upper, bool compile_inline, ymm_t *ymm) {
        movaps(operands[0].toEncoderOperand(upper), operands[1].toEncoderOperand(upper));
    }
};