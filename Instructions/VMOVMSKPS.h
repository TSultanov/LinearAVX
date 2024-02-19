#include "CompilableInstruction.h"

class VMOVMSKPS : public CompilableInstruction<VMOVMSKPS> {
public:
    VMOVMSKPS(uint64_t rip, uint8_t ilen, xed_decoded_inst_t xedd) : CompilableInstruction(rip, ilen, xedd) {}
private:
    void implementation(bool upper, bool compile_inline) {
        if (usesYmm()) {
            debug_print("VMOVMSKPS: YMM registers support is not implemented!\n");
            exit(1);
        }

        movmskps(operands[0].toEncoderOperand(upper), operands[1].toEncoderOperand(upper));
    }
};