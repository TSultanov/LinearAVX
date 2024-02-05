#include "CompilableInstruction.h"

class VMOVUPS : public CompilableInstruction<VMOVUPS> {
public:
    VMOVUPS(uint64_t rip, const xed_decoded_inst_t *xedd) : CompilableInstruction(rip, xedd) {}
private:
    void implementation(bool upper, bool compile_inline, ymm_t *ymm) {
        movups(operands[0].toEncoderOperand(upper), operands[1].toEncoderOperand(upper));

        if (operands[0].isXmm()) {
            zeroupperInternal(ymm, operands[0]);
        }
    }
};